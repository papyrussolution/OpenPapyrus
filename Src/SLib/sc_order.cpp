// SC_ORDER.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2010, 2016, 2019
// Part of StyloConduit project
// Экспорт/Импорт документов
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include "StyloConduit.h"

SCDBObjOrder::SCDBObjOrder(SpiiExchgContext * pCtx) : SCDBObject(pCtx), P_HdrTbl(0), P_LineTbl(0)
{
}

SCDBObjOrder::~SCDBObjOrder()
{
	delete P_HdrTbl;
	delete P_LineTbl;
}

int SCDBObjOrder::CreateHdrTbl(const char * pFileName)
{
	DbfTable tbl(pFileName);
	int    num_flds = 0;
	DBFCreateFld fld_list[32];
	fld_list[num_flds++].Init("ID",        'N',  10, 0);
	fld_list[num_flds++].Init("CLIENTID",  'N',  10, 0);
	fld_list[num_flds++].Init("DATE",      'D',   8, 0);
	fld_list[num_flds++].Init("DISCOUNT",  'N',   5, 1);
	fld_list[num_flds++].Init("NUMBER",    'C',  12, 0);
	fld_list[num_flds++].Init("SUM",       'N',  12, 2);
	fld_list[num_flds++].Init("COMMENT",   'C', 160, 0);
	fld_list[num_flds++].Init("DLVRADRID", 'N',  10, 0);
	fld_list[num_flds++].Init("QKID",      'N',  10, 0);
	fld_list[num_flds++].Init("LOCID",     'N',  10, 0);
	return ScCreateDbfTable(&tbl, num_flds, fld_list, P_Ctx->LogFile);
}

int SCDBObjOrder::CreateLineTbl(const char * pFileName)
{
	DbfTable tbl(pFileName);
	int    num_flds = 0;
	DBFCreateFld fld_list[32];
	fld_list[num_flds++].Init("ID",       'N', 10, 0);
	fld_list[num_flds++].Init("BILLID",   'N', 10, 0);
	fld_list[num_flds++].Init("GOODSID",  'N', 10, 0);
	fld_list[num_flds++].Init("QUANTITY", 'N', 12, 3);
	fld_list[num_flds++].Init("PRICE",    'N', 12, 2);
	return ScCreateDbfTable(&tbl, num_flds, fld_list, P_Ctx->LogFile);
}

int SCDBObjOrder::Init(const char * pExpPath, const char * pImpPath)
{
	int    ok = 1;
	int    hdr_tbl_absence = 0;
	char   path[MAXPATH], fname[MAXPATH];
	setLastSlash(STRNSCPY(path, pImpPath));

	strcat(STRNSCPY(fname, path), "sp_bill.dbf");
	if(!fileExists(fname)) {
		THROW(CreateHdrTbl(fname));
		hdr_tbl_absence = 1;
	}
	P_HdrTbl = new DbfTable(fname);
	//
	// Инициализируем LastID для того, чтобы при добавлении новых записей
	// смещать ИД документов на значение LastID.
	//
	LastID = 0;
	if(!hdr_tbl_absence && P_HdrTbl->top()) do {
		int32 id;
		DbfRecord rec(P_HdrTbl);
		P_HdrTbl->getRec(&rec);
		rec.get(1, id);
		if(id > LastID)
			LastID = id;
	} while(P_HdrTbl->next());
	IdAsscList.freeAll();
	strcat(STRNSCPY(fname, path), "sp_bitem.dbf");
	if(hdr_tbl_absence)
		SFile::Remove(fname);
	if(!fileExists(fname))
		THROW(CreateLineTbl(fname));
	P_LineTbl = new DbfTable(fname);
	CATCHZOK
	return ok;
}

int SCDBObjOrder::PutHdrRec(void * pRec)
{
	int    ok = 1;
	long   id = 0, shifted_id = 0;
	uint32 ver = P_Ctx->PalmCfg.Ver;
	LDATE dt = ZERODATE;
	union  {
		const OrdHdr    * p_rec;
		const OrdHdr700 * p_rec700;
	} b;
	DbfRecord out_rec(P_HdrTbl);

	if(ver >= 700) {
		b.p_rec700 = static_cast<const OrdHdr700 *>(pRec);
		id = SyncHHToHostDWord(b.p_rec700->ID);
		shifted_id = id + LastID;
		IdAsscList.Add(id, shifted_id, 0, 0);

		out_rec.put(1, shifted_id);
		out_rec.put(2, (long)SyncHHToHostDWord(b.p_rec700->ClientID));
		dt = plusdate(encodedate(31, 12, 1995), SyncHHToHostWord(b.p_rec700->Date));
		out_rec.put(3, dt);
		out_rec.put(4, palmintmoneytodbl(b.p_rec700->PctDis));
		out_rec.put(5, b.p_rec700->Code);
		out_rec.put(6, palmintmoneytodbl(b.p_rec700->Amount));
		out_rec.put(7, b.p_rec700->Memo);
		out_rec.put(8, (long)SyncHHToHostDWord(b.p_rec700->DlvrAddrID));
		out_rec.put(9, (long)SyncHHToHostDWord(b.p_rec700->QuotKindID));
		out_rec.put(10, (long)SyncHHToHostDWord(b.p_rec700->LocID));
	}
	else {
		b.p_rec = static_cast<const OrdHdr *>(pRec);
		id = SyncHHToHostDWord(b.p_rec->ID);
		shifted_id = id + LastID;
		IdAsscList.Add(id, shifted_id, 0, 0);

		out_rec.put(1, shifted_id);
		out_rec.put(2, (long)SyncHHToHostDWord(b.p_rec->ClientID));
		dt = plusdate(encodedate(31, 12, 1995), SyncHHToHostWord(b.p_rec->Date));
		out_rec.put(3, dt);
		out_rec.put(4, palmintmoneytodbl(b.p_rec->PctDis));
		out_rec.put(5, b.p_rec->Code);
		out_rec.put(6, palmintmoneytodbl(b.p_rec->Amount));
		out_rec.put(7, b.p_rec->Memo);
		out_rec.put(8, (long)SyncHHToHostDWord(b.p_rec->DlvrAddrID));
		out_rec.put(9, (long)SyncHHToHostDWord(b.p_rec->QuotKindID));
		out_rec.put(10, 0L);
	}
	THROW(ScAddDbfRec(P_HdrTbl, &out_rec, P_Ctx->LogFile));
	CATCHZOK
	return ok;
}

int SCDBObjOrder::ImportHdr(PROGRESSFN pFn)
{
	const char * p_tbl_name = "OrdHdr.tbl";
	int    ok = 1;
	uint32 ver = P_Ctx->PalmCfg.Ver;
	SyncTable stbl(0, P_Ctx->PalmCfg.PalmCompressedData(), P_Ctx);
	SyncTable::Stat stat;
	if(stbl.Find(p_tbl_name, &stat, P_Ctx->PalmCfg.PalmCompressedData()) > 0 && stat.NumRecs > 0) {
		uint16 recno = 0;
		THROW(P_Ctx->PalmCfg.PalmCompressedData() || stbl.Open(p_tbl_name, 0));
		for(recno = 0; recno < stat.NumRecs; recno++) {
			union {
				OrdHdr    rec;
				OrdHdr700 rec700;
			} b;

			if(ver >= 700) {
				size_t rec_size = sizeof(b.rec700);
				THROW(stbl.ReadRecByIdx(recno, 0, &b.rec700, &rec_size));
				THROW(PutHdrRec(&b.rec700));
			}
			else {
				size_t rec_size = sizeof(b.rec);
				THROW(stbl.ReadRecByIdx(recno, 0, &b.rec, &rec_size));
				THROW(PutHdrRec(&b.rec));
			}
			WaitPercent(pFn, recno+1, stat.NumRecs, "Импорт документов");
		}
	}
	CATCHZOK
	stbl.Close();
	return ok;
}

int SCDBObjOrder::PutLineRec(void * pRec)
{
	int    ok = 1;
	char   log_buf[256];
	uint32 ver = P_Ctx->PalmCfg.Ver;
	long   ord_id = 0, ord_shifted_id = 0;
	long   goods_id = 0;
	double qtty = 0, price = 0;
	union {
		const OrdLine *  p_rec;
		const OrdLine158 * p_rec158;
	} b;
	if(ver < 158)
		b.p_rec = static_cast<const OrdLine *>(pRec);
	else
		b.p_rec158 = static_cast<const OrdLine158 *>(pRec);

	DbfRecord out_rec(P_LineTbl);

	out_rec.put(1, 0); // LineID is not used
	if(ver < 158) {
		ord_id   = SyncHHToHostDWord(b.p_rec->OrderID);
		goods_id = (long)SyncHHToHostDWord(b.p_rec->GoodsID);
		qtty     = palmintmoneytodbl(b.p_rec->Qtty);
		price    = palmintmoneytodbl(b.p_rec->Price);
	}
	else {
		ord_id   = SyncHHToHostDWord(b.p_rec158->OrderID);
		goods_id = (long)SyncHHToHostDWord(b.p_rec158->GoodsID);
		qtty     = palmintmoneytodbl(b.p_rec158->Qtty);
		price    = palmintmoneytodbl(b.p_rec158->Price);
	}
	if(IdAsscList.Search(ord_id, &ord_shifted_id, 0)) {
		out_rec.put(2, ord_shifted_id);
		out_rec.put(3, goods_id);
		out_rec.put(4, qtty);
		out_rec.put(5, price);
		THROW(ScAddDbfRec(P_LineTbl, &out_rec, P_Ctx->LogFile));
	}
	else {
		sprintf(log_buf, "SPII FAIL: Can't resolve order_id %ld", ord_id);
		SyncTable::LogMessage(P_Ctx->LogFile, log_buf);
	}
	CATCHZOK
	return ok;
}

int SCDBObjOrder::ImportLine(PROGRESSFN pFn)
{
	const char * p_tbl_name = "OrdLine.tbl";

	int    ok = 1;
	SyncTable stbl(0, P_Ctx->PalmCfg.PalmCompressedData(), P_Ctx);
	SyncTable::Stat stat;
	if(stbl.Find(p_tbl_name, &stat, P_Ctx->PalmCfg.PalmCompressedData()) > 0 && stat.NumRecs > 0) {
		uint16 recno = 0;
		uint32 ver = P_Ctx->PalmCfg.Ver;
		THROW(P_Ctx->PalmCfg.PalmCompressedData() || stbl.Open(p_tbl_name, 0));
		for(recno = 0; recno < stat.NumRecs; recno++) {
			union {
				OrdLine rec;
				OrdLine158 rec158;
			} b;
			size_t  rec_size = 0;
			if(ver < 158) {
				rec_size = sizeof(b.rec);
				THROW(stbl.ReadRecByIdx(recno, 0, &b.rec, &rec_size));
				THROW(PutLineRec(&b.rec));
			}
			else {
				rec_size = sizeof(b.rec158);
				THROW(stbl.ReadRecByIdx(recno, 0, &b.rec158, &rec_size));
				THROW(PutLineRec(&b.rec158));
			}
			WaitPercent(pFn, recno+1, stat.NumRecs, "Импорт строк документов");
		}
	}
	CATCHZOK
	return ok;
}

int SCDBObjOrder::Import(PROGRESSFN pFn, CSyncProperties * pProps)
{
	char   sem_fname[MAXPATH];
	strcat(setLastSlash(strcpy(sem_fname, pProps->m_PathName)), "OUT");
	strcat(setLastSlash(sem_fname), "sp_ready");
	SFile::Remove(sem_fname);
	int    ok = (ImportHdr(pFn) && ImportLine(pFn));
	if(ok) {
		FILE * f = fopen(sem_fname, "w");
		if(f) {
			fclose(f);
			f = 0;
		}
		else {
			char log_buf[256];
			sprintf(log_buf, "SPII FAIL: Can't create semaphore file %s", sem_fname);
			SyncTable::LogMessage(P_Ctx->LogFile, log_buf);
		}
	}
	return ok;
}
//
//
//
SCDBObjCliInv::SCDBObjCliInv(SpiiExchgContext * pCtx) : SCDBObject(pCtx), P_HdrTbl(0), P_LineTbl(0)
{
}

SCDBObjCliInv::~SCDBObjCliInv()
{
	delete P_HdrTbl;
	delete P_LineTbl;
}

int SCDBObjCliInv::CreateHdrTbl(const char * pFileName)
{
	DbfTable tbl(pFileName);
	int    num_flds = 0;
	DBFCreateFld fld_list[32];
	fld_list[num_flds++].Init("ID",        'N',  10, 0);
	fld_list[num_flds++].Init("DATE",      'D',   8, 0);
	fld_list[num_flds++].Init("NUMBER",    'C',  12, 0);
	fld_list[num_flds++].Init("CLIENTID",  'N',  10, 0);
	fld_list[num_flds++].Init("DLVRADRID", 'N',  10, 0);
	fld_list[num_flds++].Init("COMMENT",   'C', 160, 0);
	return ScCreateDbfTable(&tbl, num_flds, fld_list, P_Ctx->LogFile);
}

int SCDBObjCliInv::CreateLineTbl(const char * pFileName)
{
	DbfTable tbl(pFileName);
	int    num_flds = 0;
	DBFCreateFld fld_list[32];
	fld_list[num_flds++].Init("INVID",    'N', 10, 0);
	fld_list[num_flds++].Init("GOODSID",  'N', 10, 0);
	fld_list[num_flds++].Init("QUANTITY", 'N', 12, 3);
	return ScCreateDbfTable(&tbl, num_flds, fld_list, P_Ctx->LogFile);
}

int SCDBObjCliInv::Init(const char * pExpPath, const char * pImpPath)
{
	int    ok = 1;
	int    hdr_tbl_absence = 0;
	char   path[MAXPATH], fname[MAXPATH];
	setLastSlash(STRNSCPY(path, pImpPath));

	strcat(STRNSCPY(fname, path), "sp_invh.dbf");
	if(!fileExists(fname)) {
		THROW(CreateHdrTbl(fname));
		hdr_tbl_absence = 1;
	}
	P_HdrTbl = new DbfTable(fname);
	//
	// Инициализируем LastID для того, чтобы при добавлении новых записей
	// смещать ИД документов на значение LastID.
	//
	LastID = 0;
	if(!hdr_tbl_absence && P_HdrTbl->top())
		do {
			int32 id;
			DbfRecord rec(P_HdrTbl);
			P_HdrTbl->getRec(&rec);
			rec.get(1, id);
			if(id > LastID)
				LastID = id;
		} while(P_HdrTbl->next());
	IdAsscList.freeAll();

	strcat(STRNSCPY(fname, path), "sp_invl.dbf");
	if(hdr_tbl_absence)
		SFile::Remove(fname);
	if(!fileExists(fname))
		THROW(CreateLineTbl(fname));
	P_LineTbl = new DbfTable(fname);
	CATCHZOK
	return ok;
}

int SCDBObjCliInv::PutHdrRec(void * pRec)
{
	int    ok = 1;
	const  OrdHdr * p_rec = static_cast<const OrdHdr *>(pRec);
	DbfRecord out_rec(P_HdrTbl);
	LDATE  dt = ZERODATE;
	long   id = SyncHHToHostDWord(p_rec->ID);
	long   shifted_id = id + LastID;
	IdAsscList.Add(id, shifted_id, 0, 0);
	out_rec.put(1, shifted_id);
	dt = plusdate(encodedate(31, 12, 1995), SyncHHToHostWord(p_rec->Date));
	out_rec.put(2, dt);
	out_rec.put(3, p_rec->Code);
	out_rec.put(4, (long)SyncHHToHostDWord(p_rec->ClientID));
	out_rec.put(5, (long)SyncHHToHostDWord(p_rec->DlvrAddrID));
	out_rec.put(6, p_rec->Memo);
	THROW(ScAddDbfRec(P_HdrTbl, &out_rec, P_Ctx->LogFile));
	CATCHZOK
	return ok;
}

int SCDBObjCliInv::ImportHdr(PROGRESSFN pFn)
{
	const char * p_tbl_name = "InvHdr.tbl";
	int    ok = 1;
	SyncTable stbl(0, P_Ctx->PalmCfg.PalmCompressedData(), P_Ctx);
	SyncTable::Stat stat;
	if(stbl.Find(p_tbl_name, &stat, P_Ctx->PalmCfg.PalmCompressedData()) > 0 && stat.NumRecs > 0) {
		uint16 recno = 0;
		THROW(P_Ctx->PalmCfg.PalmCompressedData() || stbl.Open(p_tbl_name, 0));
		for(recno = 0; recno < stat.NumRecs; recno++) {
			OrdHdr rec;
			size_t rec_size = sizeof(rec);
			THROW(stbl.ReadRecByIdx(recno, 0, &rec, &rec_size));
			THROW(PutHdrRec(&rec));
			WaitPercent(pFn, recno+1, stat.NumRecs, "Импорт инвентаризаций");
		}
	}
	CATCHZOK
	return ok;
}

int SCDBObjCliInv::PutLineRec(void * pRec)
{
	int    ok = 1;
	char   log_buf[256];
	long   ord_id = 0, ord_shifted_id = 0;
	const  OrdLine * p_rec = static_cast<const OrdLine *>(pRec);
	DbfRecord out_rec(P_LineTbl);
	ord_id = SyncHHToHostDWord(p_rec->InvID);
	if(IdAsscList.Search(ord_id, &ord_shifted_id, 0)) {
		out_rec.put(1, ord_shifted_id);
		out_rec.put(2, (long)SyncHHToHostDWord(p_rec->GoodsID));
		out_rec.put(3, palmintmoneytodbl(p_rec->Qtty));
		THROW(ScAddDbfRec(P_LineTbl, &out_rec, P_Ctx->LogFile));
	}
	else {
		sprintf(log_buf, "SPII FAIL: Can't resolve inventory_id %ld", ord_id);
		SyncTable::LogMessage(P_Ctx->LogFile, log_buf);
	}
	CATCHZOK
	return ok;
}

int SCDBObjCliInv::ImportLine(PROGRESSFN pFn)
{
	const char * p_tbl_name = "InvLine.tbl";
	int    ok = 1;
	SyncTable stbl(0, P_Ctx->PalmCfg.PalmCompressedData(), P_Ctx);
	SyncTable::Stat stat;
	if(stbl.Find(p_tbl_name, &stat, P_Ctx->PalmCfg.PalmCompressedData()) > 0 && stat.NumRecs > 0) {
		uint16 recno = 0;
		THROW(P_Ctx->PalmCfg.PalmCompressedData() || stbl.Open(p_tbl_name, 0));
		for(recno = 0; recno < stat.NumRecs; recno++) {
			OrdLine rec;
			size_t  rec_size = sizeof(rec);
			THROW(stbl.ReadRecByIdx(recno, 0, &rec, &rec_size));
			THROW(PutLineRec(&rec));
			WaitPercent(pFn, recno+1, stat.NumRecs, "Импорт строк инвентаризаций");
		}
	}
	CATCHZOK
	return ok;
}

int SCDBObjCliInv::Import(PROGRESSFN pFn, CSyncProperties * pProps)
{
	return (ImportHdr(pFn) && ImportLine(pFn));
}
