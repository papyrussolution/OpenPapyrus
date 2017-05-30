// SC_TODO.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2010, 2011, 2016
// Part of StyloConduit project
// Экспорт/Импорт задач
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include "StyloConduit.h"

SCDBObjToDo::SCDBObjToDo(SpiiExchgContext * pCtx) : SCDBObject(pCtx)
{
	P_ExpTbl = 0;
	P_ImpTbl = 0;
	P_ExpPath = 0;
	P_ImpPath = 0;
}

SCDBObjToDo::~SCDBObjToDo()
{
	delete P_ExpTbl;
	delete P_ImpTbl;
	delete P_ExpPath;
	delete P_ImpPath;
}

int SCDBObjToDo::CreateImpTbl(const char * pFileName)
{
	DbfTable tbl(pFileName);
	int    num_flds = 0;
	DBFCreateFld fld_list[32];
	fld_list[num_flds++].Init("ID",        'N',  10, 0);
	fld_list[num_flds++].Init("PRIOR",     'N',   2, 0);
	fld_list[num_flds++].Init("COMPLETED", 'N',   1, 0);
	fld_list[num_flds++].Init("DUEDATE",   'D',   8, 0);
	fld_list[num_flds++].Init("COMPLDATE", 'D',   8, 0);
	fld_list[num_flds++].Init("DESCRIPT",  'C', 250, 0);
	fld_list[num_flds++].Init("MEMO",      'C', 250, 0);
	return ScCreateDbfTable(&tbl, num_flds, fld_list, P_Ctx->LogFile);
}

int SCDBObjToDo::Init(const char * pExpPath, const char * pImpPath)
{
	int    ok = 1;
	char   path[MAXPATH], fname[MAXPATH];

	ZDELETE(P_ExpPath);
	P_ExpPath = newStr(pExpPath);
	ZDELETE(P_ImpPath);
	P_ImpPath = newStr(pImpPath);

	setLastSlash(STRNSCPY(path, pExpPath));
	strcat(STRNSCPY(fname, path), "sp_todo.dbf");
	if(fileExists(fname)) {
		P_ExpTbl = new DbfTable(fname);
		THROW(ScCheckDbfOpening(P_ExpTbl, P_Ctx->LogFile));
	}

	setLastSlash(STRNSCPY(path, pImpPath));
	strcat(STRNSCPY(fname, path), "sp_todo.dbf");
	if(fileExists(fname))
		SFile::Remove(fname);
	THROW(CreateImpTbl(fname));
	P_ImpTbl = new DbfTable(fname);
	THROW(ScCheckDbfOpening(P_ImpTbl, P_Ctx->LogFile));

	THROW(ReadIdAssoc(P_Ctx->PalmCfg.PalmCompressedData()));
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

const char * SCDBObjToDo::P_ToDoAsscFileName = "ToDoSPII.dat";
const char * SCDBObjToDo::P_ToDoAsscFileName_A = "ToDoSPII_A.dat";

int SCDBObjToDo::ReadIdAssoc(int fromPalmCompressedFile)
{
	IdAsscList.freeAll();

	int    ok = 1;
	SyncTable stbl(0, fromPalmCompressedFile, P_Ctx);
	SyncTable::Stat stat;
	if(stbl.Find(P_ToDoAsscFileName, &stat, P_Ctx->PalmCfg.PalmCompressedData()) > 0 && stat.NumRecs > 0) {
		uint16 recno = 0;
		THROW(P_Ctx->PalmCfg.PalmCompressedData() || stbl.Open(P_ToDoAsscFileName, 0));
		for(recno = 0; recno < stat.NumRecs; recno++) {
			long   id = 0, shifted_id = 0;
			LAssoc rec;
			size_t rec_size = sizeof(rec);
			THROW(stbl.ReadRecByIdx(recno, 0, &rec, &rec_size));
			rec.Key = SyncHHToHostDWord(rec.Key);
			IdAsscList.Add(rec.Key, rec.Val, 0, 0);
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SCDBObjToDo::WriteIdAssoc(PROGRESSFN pFn)
{
	int    ok = 1;
	uint   i;

	const int do_compress = P_Ctx->PalmCfg.CompressData();
	const char * p_tbl_name = do_compress ? P_ToDoAsscFileName_A : P_ToDoAsscFileName;

	SyncTable stbl(do_compress, 0, P_Ctx);
	THROW(stbl.Open(p_tbl_name, SyncTable::oCreate));
	if(!do_compress)
		THROW(stbl.PurgeAll());
	for(i = 0; i < IdAsscList.getCount(); i++) {
		DWORD rec_id = 0;
		LAssoc & r_item = IdAsscList.at(i);
		r_item.Key = SyncHostToHHDWord(r_item.Key);
		THROW(stbl.AddRec(&rec_id, &r_item, sizeof(r_item)));
		WaitPercent(pFn, i+1, IdAsscList.getCount(), "Экспорт таблицы ассоциаций");
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

SCDBObjToDo::PalmRec * SCDBObjToDo::AllocPalmRec(HostRec * pRec, size_t * pBufLen)
{
	PalmRec * p_buf = 0;
	size_t  offs = 0;
	size_t  buf_len = sizeof(PalmRec)+strlen(pRec->Descr)+1+strlen(pRec->Note)+1;
	// @test {
	if((buf_len % 2) != 0)
		buf_len++;
	// } @test
	p_buf = (PalmRec *)SAlloc::C(1, buf_len);
	if(pRec->DueDate)
		p_buf->DueDate = ((pRec->DueDate.year() - 1904) << 9) | (pRec->DueDate.month() << 5) | pRec->DueDate.day();
	else
		p_buf->DueDate = 0xffffU;
	p_buf->DueDate = SyncHostToHHWord(p_buf->DueDate);
	p_buf->Priority = (uint8)pRec->Priority;
	if(pRec->Completed)
		p_buf->Priority |= 0x80;
	offs += sizeof(PalmRec);
	size_t len = strlen(pRec->Descr);
	strcpy(((char *)p_buf) + offs, pRec->Descr);
	offs += len+1;
	len = strlen(pRec->Note);
	strcpy(((char *)p_buf) + offs, pRec->Note);
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

int SCDBObjToDo::RecPalmToHost(PalmRec * pPalmRec, HostRec * pHostRec)
{
	memzero(pHostRec, sizeof(*pHostRec));
	uint16 d = SyncHHToHostWord(pPalmRec->DueDate);
	if(d != 0xffffU)
		pHostRec->DueDate = encodedate(d & 0x1f, (d & 0x1e0) >> 5, ((d & 0xfe00) >> 9) + 1904);
	pHostRec->Priority = (pPalmRec->Priority & 0x7f);
	pHostRec->Completed = (pPalmRec->Priority & 0x80) ? 1 : 0;
	const char * p_palm_descr = (const char *)(pPalmRec+1);
	STRNSCPY(pHostRec->Descr, p_palm_descr);
	size_t offs = sizeof(*pPalmRec) + strlen(p_palm_descr)+1;
	STRNSCPY(pHostRec->Note, ((char *)pPalmRec)+offs);
	return 1;
}

int SCDBObjToDo::Import(PROGRESSFN pFn, CSyncProperties * pProps)
{
	int    ok = 1;
	uint8 * p_buf = new uint8[1024];
	SyncTable stbl(0, P_Ctx->PalmCfg.PalmCompressedData(), P_Ctx);
	SyncTable::Stat stat;
	if(!(P_Ctx->PalmCfg.Flags & CFGF_EXPIMPTODO)) {
		SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: IMPORT TODO base declined by palm");
		ok = -1;
	}
	else if(P_ImpTbl) {
		//SyncTable todo(0); // debug
		//todo.Open("ToDoDB"); // debug
		//todo.PurgeAll(); // debug
		//SyncTable::DeleteTable("ToDoDB"); // debug
		//SyncTable::DeleteTable("ToDoSPII.dat"); // debug
		if(stbl.Find("ToDoDB", &stat, P_Ctx->PalmCfg.PalmCompressedData()) > 0 && stat.NumRecs > 0) {
			uint16 recno = 0;
			uint   i = 0;
			THROW(P_Ctx->PalmCfg.PalmCompressedData() || stbl.Open("ToDoDB", 0));
			for(i = 0; i < IdAsscList.getCount();) {
				int    dont_inc = 0;
				long   id = 0, shifted_id = 0;
				size_t rec_size = 1024;
				LAssoc & r_assc = IdAsscList.at(i);
				int    r = 0;
				if(P_Ctx->PalmCfg.PalmCompressedData()) {
					stbl.ReadRecByIdx(i, 0, p_buf, &rec_size);
					r = 1;
				}
				else
					r = stbl.ReadRecByID(r_assc.Val, p_buf, &rec_size);
				if(r) {
					HostRec host_rec;
					RecPalmToHost((PalmRec *)p_buf, &host_rec);
					//
					// Забираем с Palm'а только те записи, которые помечены признаком "Выполнено"
					//
					if(host_rec.Completed) {
						DbfRecord out_rec(P_ImpTbl);
						out_rec.empty();
						out_rec.put(1, r_assc.Key);
						out_rec.put(2, host_rec.Priority);
						out_rec.put(3, host_rec.Completed);
						out_rec.put(4, host_rec.DueDate);
						SCharToOem(host_rec.Descr);
						out_rec.put(6, host_rec.Descr);
						SCharToOem(host_rec.Note);
						out_rec.put(7, host_rec.Note);
						THROW(ScAddDbfRec(P_ImpTbl, &out_rec, P_Ctx->LogFile));
					}
				}
				else if(P_Ctx->LastErr == SYNCERR_NOT_FOUND) {
					// Запись удалена с Palm'а
					IdAsscList.atFree(i);
					dont_inc = 1;
				}
				else {
					CALLEXCEPT();
				}
				if(!dont_inc)
					i++;
				WaitPercent(pFn, i, IdAsscList.getCount(), "Импорт задач");
			}
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	delete p_buf;
	return ok;
}

int SCDBObjToDo::Export(PROGRESSFN pFn, CSyncProperties * pProps)
{
	int    ok = 1;
	PalmRec * p_out_buf = 0;
	if(!(P_Ctx->PalmCfg.Flags & CFGF_EXPIMPTODO)) {
		SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: EXPORT TODO base declined by palm");
		ok = -1;
	}
	else if(cmp(P_Ctx->PalmCfg.TmToDo, P_Ctx->HostCfg.TmToDo) >= 0) {
		SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: TODO base on palm is recently that host");
		ok = -1;
	}
	else if(P_ExpTbl && P_ExpTbl->getNumRecs() && P_ExpTbl->top()) {
		long   recno = 0, numrecs = P_ExpTbl->getNumRecs();
		int    fldn_id = 0;
		int    fldn_prior = 0;
		int    fldn_compl = 0;
		int    fldn_duedate = 0;
		int    fldn_descr = 0;
		int    fldn_memo = 0;
		P_ExpTbl->getFieldNumber("ID",        &fldn_id);
		P_ExpTbl->getFieldNumber("PRIOR",     &fldn_prior);
		P_ExpTbl->getFieldNumber("COMPLETED", &fldn_compl);
		P_ExpTbl->getFieldNumber("DUEDATE",   &fldn_duedate);
		P_ExpTbl->getFieldNumber("DESCRIPT",  &fldn_descr);
		P_ExpTbl->getFieldNumber("MEMO",      &fldn_memo);

		const int do_compress = P_Ctx->PalmCfg.CompressData();
		const char * p_tbl_name = do_compress ? "ToDoDB_A" : "ToDoDB";
		if(do_compress)
			IdAsscList.freeAll();
		else if(P_Ctx->PalmCfg.PalmCompressedData())
			THROW(ReadIdAssoc(0));
		SyncTable stbl(do_compress, 0, P_Ctx);
		THROW(stbl.Open(p_tbl_name, SyncTable::oCreate));
		do {
			size_t buf_len = 0;
			long   id = 0;
			long   temp_long = 0;
			DWORD  rec_id = 0;
			char   temp_buf[512];

			HostRec host_rec;
			DbfRecord rec(P_ExpTbl);
			P_ExpTbl->getRec(&rec);
			MEMSZERO(host_rec);
			rec.get(fldn_id, host_rec.ID);
			if(rec.get(fldn_prior, temp_long))
				host_rec.Priority = (int16)temp_long;
			if(rec.get(fldn_compl, temp_long))
				host_rec.Completed = (int16)temp_long;
			rec.get(fldn_duedate, host_rec.DueDate);
			rec.get(fldn_descr, temp_buf);
			STRNSCPY(host_rec.Descr, strip(temp_buf));
			rec.get(fldn_memo, temp_buf);
			STRNSCPY(host_rec.Note, strip(temp_buf));

			p_out_buf = AllocPalmRec(&host_rec, &buf_len); // @checkerr
			if(do_compress) {
				THROW(stbl.AddRec(&(rec_id = 0), p_out_buf, buf_len));
				IdAsscList.Add(host_rec.ID, 0, 0);
			}
			else {
				if(!IdAsscList.Search(host_rec.ID, &temp_long, 0)) {
					THROW(stbl.AddRec(&(rec_id = 0), p_out_buf, buf_len));
   					IdAsscList.Add(host_rec.ID, rec_id, 0);
				}
				else {
					rec_id = temp_long;
					// @todo Изменить запись по rec_id
				}
			}
			WaitPercent(pFn, ++recno, numrecs, "Экспорт задач");
			ZFREE(p_out_buf);
		} while(P_ExpTbl->next());
		stbl.Close();
		THROW(WriteIdAssoc(pFn));
		if(P_Ctx->PalmCfg.CompressData())
			P_Ctx->TransmitComprFile = 1;
		{
			char log_msg[128];
			sprintf(log_msg, "SPII OK: %ld TODO records exported", numrecs);
			SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
		}
	}
	else
		ok = -1;
	CATCH
		ok = 0;
	ENDCATCH
	delete p_out_buf;
	return ok;
}
