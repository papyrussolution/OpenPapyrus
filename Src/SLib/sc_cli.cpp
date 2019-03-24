// SC_CLI.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2010, 2011, 2016, 2017, 2019
// Part of StyloConduit project
// Экспорт/Импорт клиентов
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include "StyloConduit.h"
#include <stddef.h>

IMPL_CMPFUNC(CLIENTID, i1, i2)
{
	const SCDBObjClient::IdxRec * cl1 = static_cast<const SCDBObjClient::IdxRec *>(i1);
	const SCDBObjClient::IdxRec * cl2 = static_cast<const SCDBObjClient::IdxRec *>(i2);
	return CMPSIGN(cl1->ID, cl2->ID);
}

IMPL_CMPFUNC(CLIENTNAM, i1, i2)
{
	int r = 0;
	const SCDBObjClient::IdxRec * cl1 = static_cast<const SCDBObjClient::IdxRec *>(i1);
	const SCDBObjClient::IdxRec * cl2 = static_cast<const SCDBObjClient::IdxRec *>(i2);
	if((r = stricmp866(cl1->Name, cl2->Name)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else
		return 0;
}

SCDBObjClient::SCDBObjClient(SpiiExchgContext * pCtx) : SCDBObject(pCtx)
{
	P_CliTbl = 0;
	P_AdrTbl = 0;
}

SCDBObjClient::~SCDBObjClient()
{
	delete P_CliTbl;
	delete P_AdrTbl;
}

const SCDBTblEntry * SCDBObjClient::GetDefinition(uint * pEntryCount) const
{
	static const SCDBTblEntry def[] = {
		{"Client.tbl", 2, offsetof(PalmConfig, TmClient), 0, 0},
		{"ClID.idx",   0, 0, 0, PTR_CMPFUNC(CLIENTID)},
		{"ClNam.idx",  0, 0, 0, PTR_CMPFUNC(CLIENTNAM)}
	};
	ASSIGN_PTR(pEntryCount, 3);
	return def;
}

int SCDBObjClient::Init(const char * pExpPath, const char * pImpPath)
{
	int    ok = 1;
	char   path[MAXPATH], fname[MAXPATH];
	setLastSlash(STRNSCPY(path, pExpPath));
	P_CliTbl = new DbfTable(strcat(STRNSCPY(fname, path), "sp_cli.dbf"));
	P_AdrTbl = new DbfTable(strcat(STRNSCPY(fname, path), "sp_cliad.dbf"));
	LoadAddrList();
	return ok;
}

int SCDBObjClient::LoadAddrList()
{
	int    ok = 1;
	AddrList.freeAll();
	if(P_AdrTbl && P_AdrTbl->top())
		do {
			AddrItem item;
			MEMSZERO(item);
			DbfRecord rec(P_AdrTbl);
			P_AdrTbl->getRec(&rec);
			rec.get(1, item.AdrID);
			rec.get(2, item.CliID);
			get_str_from_dbfrec(&rec, 3, 1, item.Addr, sizeof(item.Addr));
			AddrList.insert(&item);
		} while(P_AdrTbl->next());
	return ok;
}

SCDBObjClient::PalmRec * SCDBObjClient::AllocPalmRec(uint addrCount, size_t * pBufLen)
{
	const size_t buf_len = sizeof(PalmRec) + addrCount * sizeof(ClientAddr);
	PalmRec * p_buf = (PalmRec *)SAlloc::C(1, buf_len);
	if(p_buf)
		p_buf->AddrCount = addrCount;
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

SCDBObjClient::PalmRec700 * SCDBObjClient::AllocPalmRec700(uint addrCount, size_t * pBufLen)
{
	const size_t buf_len = sizeof(PalmRec700) + addrCount * sizeof(ClientAddr);
	PalmRec700 * p_buf = (PalmRec700 *)SAlloc::C(1, buf_len);
	if(p_buf)
		p_buf->AddrCount = addrCount;
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

int SCDBObjClient::GetAddressList(long cliID, TSVector <ClientAddr> * pList) // @v9.8.4 TSArray-->TSVector
{
	const  uint max_addr_count = 100;

	int    ok = 1;
	AddrItem * p_item;
	pList->clear();
	for(uint i = 0; pList->getCount() <= max_addr_count && AddrList.enumItems(&i, (void **)&p_item);) {
		if(p_item->CliID == cliID) {
			ClientAddr dest_item;
			MEMSZERO(dest_item);
			dest_item.ID = SyncHostToHHDWord(p_item->AdrID);
			STRNSCPY(dest_item.Loc, p_item->Addr);
			pList->insert(&dest_item);
		}
	}
	return ok;
}

int SCDBObjClient::Export(PROGRESSFN pFn, CSyncProperties * pProps)
{
	int    ok = 1;
	long   numrecs = 0, recno = 0;
	char   log_msg[128];
	union {
		PalmRec    * p_out_buf;
		PalmRec700 * p_out_buf700;
	} b;
	uint32 ver = P_Ctx->PalmCfg.Ver;
	if(ver >= 700)
		b.p_out_buf700 = 0;
	else
		b.p_out_buf = 0;
	if(!ForceExportObsoleteData && cmp(P_Ctx->PalmCfg.TmClient, P_Ctx->HostCfg.TmClient) >= 0) {
		SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: CLIENT base on palm is recently that host");
		ok = -1;
	}
	else if(P_CliTbl && P_CliTbl->getNumRecs() && P_CliTbl->top()) {
		numrecs = P_CliTbl->getNumRecs();
		SyncTable stbl(P_Ctx->PalmCfg.CompressData(), 0, P_Ctx);
		THROW(InitTable(&stbl));
		do {
			size_t buf_len = 0;
			uint   i;
			long   cli_id = 0, temp_id = 0;
			double temp_dbl = 0;
			TSVector <ClientAddr> addr_list; // @v9.8.4 TSArray-->TSVector
			IdxRec idx_rec;

			int fldn_id = 0, fldn_name = 0, fldn_code = 0, fldn_inn = 0, fldn_qk = 0, fldn_debt = 0, fldn_flags = 0;
			P_CliTbl->getFieldNumber("ID",       &fldn_id);
			P_CliTbl->getFieldNumber("NAME",     &fldn_name);
			P_CliTbl->getFieldNumber("CODE",     &fldn_code);
			P_CliTbl->getFieldNumber("INN",      &fldn_inn);
			P_CliTbl->getFieldNumber("QUOTKIND", &fldn_qk);
			P_CliTbl->getFieldNumber("DEBT",     &fldn_debt);
			P_CliTbl->getFieldNumber("FLAGS",    &fldn_flags);

			MEMSZERO(idx_rec);
			//
			// Извлекаем запись клиента из входного файла
			// Результат - p_out_buf
			//
			DbfRecord rec(P_CliTbl);
			P_CliTbl->getRec(&rec);
			rec.get(fldn_id, cli_id);
			GetAddressList(cli_id, &addr_list); // @checkerr
			if(ver >= 700) {
				b.p_out_buf700 = AllocPalmRec700(addr_list.getCount(), &buf_len); // @checkerr
				b.p_out_buf700->ID = SyncHostToHHDWord(cli_id);
				idx_rec.ID = cli_id;
				get_str_from_dbfrec(&rec, fldn_name, 1, idx_rec.Name, sizeof(idx_rec.Name));
				get_str_from_dbfrec(&rec, fldn_name, 1, b.p_out_buf700->Name, sizeof(b.p_out_buf700->Name));
				get_str_from_dbfrec(&rec, fldn_code, 1, b.p_out_buf700->Code, sizeof(b.p_out_buf700->Code));
				rec.get(fldn_flags, temp_id);
				b.p_out_buf700->Flags = SyncHostToHHDWord(temp_id);
				rec.get(fldn_qk, temp_id);
				b.p_out_buf700->QuotKindID = SyncHostToHHDWord(temp_id);
				rec.get(fldn_debt, temp_dbl);
				b.p_out_buf700->Debt      = dbltopalmintmoney(temp_dbl);
				{
					b.p_out_buf700->AddrCount = SyncHostToHHWord(addr_list.getCount());
					for(i = 0; i < addr_list.getCount(); i++)
						((ClientAddr *)(b.p_out_buf700+1))[i] = addr_list.at(i);
				}
				//
				// Заносим запись клиента в базу на Palm'е
				//
				THROW(stbl.AddRec(&idx_rec.RecID, b.p_out_buf700, buf_len));
			}
			else {
				b.p_out_buf = AllocPalmRec(addr_list.getCount(), &buf_len); // @checkerr
				b.p_out_buf->ID = SyncHostToHHDWord(cli_id);
				idx_rec.ID = cli_id;
				get_str_from_dbfrec(&rec, fldn_name, 1, idx_rec.Name, sizeof(idx_rec.Name));
				get_str_from_dbfrec(&rec, fldn_name, 1, b.p_out_buf->Name, sizeof(b.p_out_buf->Name));
				get_str_from_dbfrec(&rec, fldn_code, 1, b.p_out_buf->Code, sizeof(b.p_out_buf->Code));
				rec.get(fldn_qk, temp_id);
				b.p_out_buf->QuotKindID = SyncHostToHHDWord(temp_id);
				rec.get(fldn_debt, temp_dbl);
				b.p_out_buf->Debt      = dbltopalmintmoney(temp_dbl);
				{
					b.p_out_buf->AddrCount = SyncHostToHHWord(addr_list.getCount());
					for(i = 0; i < addr_list.getCount(); i++)
						((ClientAddr *)(b.p_out_buf+1))[i] = addr_list.at(i);
				}
				//
				// Заносим запись клиента в базу на Palm'е
				//
				THROW(stbl.AddRec(&idx_rec.RecID, b.p_out_buf, buf_len));
			}
			recno++;
			THROW(stbl.Reopen(-1, recno));
			IdxList.insert(&idx_rec); // @checkerr
			WaitPercent(pFn, recno, numrecs, "Экспорт справочника клиентов");
			if(ver >= 700) {
				ZDELETE(b.p_out_buf700);
			}
			else {
				ZDELETE(b.p_out_buf);
			}
		} while(P_CliTbl->next());
		stbl.Close();
		if(!P_Ctx->PalmCfg.CompressData()) {
			THROW(ExportIndexes(pFn, &IdxList));
		}
		else
			P_Ctx->TransmitComprFile = 1;
		{
			sprintf(log_msg, "SPII OK: %ld CLIENT records exported", numrecs);
			SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
		}
	}
	else
		ok = -1;
	CATCH
		ok = 0;
		{
			char log_msg[128];
			sprintf(log_msg, "SPII ERR: CLIENT export failed");
			SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
		}
	ENDCATCH
	if(ver >= 700)
		delete b.p_out_buf700;
	else
		delete b.p_out_buf;
	return ok;
}
//
//
//
IMPL_CMPFUNC(CLIDEBT, i1, i2)
{
	int    r = 0;
	const SCDBObjClientDebt::IdxRec * cl1 = static_cast<const SCDBObjClientDebt::IdxRec *>(i1);
	const SCDBObjClientDebt::IdxRec * cl2 = static_cast<const SCDBObjClientDebt::IdxRec *>(i2);
	if(cl1->CliID < cl2->CliID)
		return -1;
	else if(cl1->CliID > cl2->CliID)
		return 1;
	else if(cl1->Dt < cl2->Dt)
		return -1;
	else if(cl1->Dt > cl2->Dt)
		return 1;
	else if((r = stricmp866(cl1->Code, cl2->Code)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else
		return 0;
}

IMPL_CMPFUNC(SCDBObjClientDebtTempRec, i1, i2)
{
	int r = 0;
	const SCDBObjClientDebt::TempRec * cl1 = static_cast<const SCDBObjClientDebt::TempRec *>(i1);
	const SCDBObjClientDebt::TempRec * cl2 = static_cast<const SCDBObjClientDebt::TempRec *>(i2);
	if(cl1->ClientID < cl2->ClientID)
		return -1;
	else if(cl1->ClientID > cl2->ClientID)
		return 1;
	else if(cl1->Dt < cl2->Dt)
		return -1;
	else if(cl1->Dt > cl2->Dt)
		return 1;
	else if((r = stricmp866(cl1->Code, cl2->Code)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else
		return 0;
}

SCDBObjClientDebt::SCDBObjClientDebt(SpiiExchgContext * pCtx) : SCDBObject(pCtx)
{
	P_CliDebtTbl = 0;
}

SCDBObjClientDebt::~SCDBObjClientDebt()
{
	delete P_CliDebtTbl;
}

int SCDBObjClientDebt::Init(const char * pExpPath, const char * pImpPath)
{
	/* @v9.5.7 
	char   path[MAXPATH], fname[MAXPATH];
	setLastSlash(STRNSCPY(path, pExpPath));
	P_CliDebtTbl = new DbfTable(strcat(STRNSCPY(fname, path), "sp_clidb.dbf")); 
	*/
	// @v9.5.7 {
	SString file_name;
	(file_name = pExpPath).SetLastSlash().Cat("sp_clidb.dbf");
	P_CliDebtTbl = new DbfTable(file_name);
	// } @v9.5.7 
	return ReadData();
}

int SCDBObjClientDebt::ReadData()
{
	int    ok = -1;
	char   log_msg[128];
	const  LDATE  base_date = encodedate(31, 12, 1995);
	TempList.freeAll();
	if(P_CliDebtTbl && P_CliDebtTbl->getNumRecs() && P_CliDebtTbl->top()) {
		int fldn_cliid = 0;
		int fldn_billid = 0;
		int fldn_code = 0;
		int fldn_date = 0;
		int fldn_amt = 0;
		int fldn_debt = 0;
		P_CliDebtTbl->getFieldNumber("CLIID",    &fldn_cliid);
		P_CliDebtTbl->getFieldNumber("BILLID",   &fldn_billid);
		P_CliDebtTbl->getFieldNumber("BILLCODE", &fldn_code);
		P_CliDebtTbl->getFieldNumber("BILLDATE", &fldn_date);
		P_CliDebtTbl->getFieldNumber("BILLAMT",  &fldn_amt);
		P_CliDebtTbl->getFieldNumber("BILLDEBT", &fldn_debt);

		do {
			LDATE   dt;
			double  temp_dbl;
			TempRec temp_rec;
			MEMSZERO(temp_rec);

			DbfRecord rec(P_CliDebtTbl);
			P_CliDebtTbl->getRec(&rec);

			rec.get(fldn_cliid, temp_rec.ClientID);
			get_str_from_dbfrec(&rec, fldn_code, 1, temp_rec.Code, sizeof(temp_rec.Code));
			rec.get(fldn_date, dt);
			temp_rec.Dt = (int16)diffdate(dt, base_date);

			rec.get(fldn_amt, temp_dbl);
			temp_rec.Amount = dbltopalmintmoney(temp_dbl);

			rec.get(fldn_debt, temp_dbl);
			temp_rec.Debt = dbltopalmintmoney(temp_dbl);
			if(!TempList.insert(&temp_rec)) {
				sprintf(log_msg, "SPII ERR: Not enough memory (SCDBObjClientDebt)");
				SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
				CALLEXCEPT();
			}
		} while(P_CliDebtTbl->next());
		TempList.sort(PTR_CMPFUNC(SCDBObjClientDebtTempRec));
		ok = 1;
	}
	CATCH
		ok = 0;
		{
			sprintf(log_msg, "SPII ERR: DEBT export failed");
			SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
		}
	ENDCATCH
	return ok;
}

SCDBObjClientDebt::PalmRec * SCDBObjClientDebt::AllocPalmRec(size_t * pBufLen)
{
	PalmRec * p_buf = 0;
	size_t  buf_len = sizeof(PalmRec);
	p_buf = (PalmRec *)SAlloc::C(1, buf_len);
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

int SCDBObjClientDebt::Export(PROGRESSFN pFn, CSyncProperties * pProps)
{
	int    ok = 1;
	char   log_msg[128];
	PalmRec * p_out_buf = 0;
	if(!(P_Ctx->PalmCfg.Flags & CFGF_LOADDEBTS)) {
		SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: DEBT base declined by palm");
		ok = -1;
	}
	else if(!ForceExportObsoleteData && cmp(P_Ctx->PalmCfg.TmCliDebt, P_Ctx->HostCfg.TmCliDebt) >= 0) {
		SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: DEBT base on palm is recently that host");
		ok = -1;
	}
	if(ok > 0 && TempList.getCount()) {
		uint i;
		SyncTable stbl(P_Ctx->PalmCfg.CompressData(), 0, P_Ctx);
		THROW(stbl.DeleteTable("CliDebt.tbl"));
		THROW(stbl.Open("CliDebt.tbl", SyncTable::oCreate));
		for(i = 0; i < TempList.getCount(); i++) {
			size_t buf_len = 0;
			IdxRec idx_rec;
			MEMSZERO(idx_rec);

			TempRec & r_rec = TempList.at(i);

			p_out_buf = AllocPalmRec(&buf_len); // @checkerr
			p_out_buf->ClientID = SyncHostToHHDWord(r_rec.ClientID);
			idx_rec.CliID = r_rec.ClientID;
			STRNSCPY(p_out_buf->Code, r_rec.Code);
			STRNSCPY(idx_rec.Code, r_rec.Code);
			p_out_buf->Dt = SyncHostToHHWord(r_rec.Dt);
			idx_rec.Dt    = r_rec.Dt;
			p_out_buf->Amount = r_rec.Amount;
			p_out_buf->Debt   = r_rec.Debt;
			THROW(stbl.AddRec(&idx_rec.RecID, p_out_buf, buf_len));
			THROW(stbl.Reopen(-1, i+1));
			WaitPercent(pFn, i+1, TempList.getCount(), "Экспорт долговых документов");
			ZDELETE(p_out_buf);
		}
		if(P_Ctx->PalmCfg.CompressData())
			P_Ctx->TransmitComprFile = 1;
		stbl.Close();
		{
			sprintf(log_msg, "SPII OK: %u DEBT records exported", TempList.getCount());
			SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
		}
	}
	else
		ok = -1;
	CATCH
		ok = 0;
		{
			sprintf(log_msg, "SPII ERR: DEBT export failed");
			SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
		}
	ENDCATCH
	delete p_out_buf;
	return ok;
}
//
//
//
SCDBObjSell::SCDBObjSell(SpiiExchgContext * pCtx) : SCDBObject(pCtx), P_Tbl(0)
{
}

SCDBObjSell::~SCDBObjSell()
{
	delete P_Tbl;
}

IMPL_CMPFUNC(SCDBObjSellTempRec, i1, i2)
{
	const SCDBObjSell::TempRec * r1 = static_cast<const SCDBObjSell::TempRec *>(i1);
	const SCDBObjSell::TempRec * r2 = static_cast<const SCDBObjSell::TempRec *>(i2);
	if(r1->ClientID < r2->ClientID)
		return -1;
	else if(r1->ClientID > r2->ClientID)
		return 1;
	else if(r1->ClientAddrID < r2->ClientAddrID)
		return -1;
	else if(r1->ClientAddrID > r2->ClientAddrID)
		return 1;
	else if(r1->GoodsID < r2->GoodsID)
		return -1;
	else if(r1->GoodsID > r2->GoodsID)
		return 1;
	else if(r1->Date < r2->Date)
		return -1;
	else if(r1->Date > r2->Date)
		return 1;
	else
		return 0;
}

IMPL_CMPFUNC(SCDBObjSellIdxRec, i1, i2)
{
	const SCDBObjSell::IdxRec * r1 = static_cast<const SCDBObjSell::IdxRec *>(i1);
	const SCDBObjSell::IdxRec * r2 = static_cast<const SCDBObjSell::IdxRec *>(i2);
	if(r1->CliID < r2->CliID)
		return -1;
	else if(r1->CliID > r2->CliID)
		return 1;
	else if(r1->AddrID < r2->AddrID)
		return -1;
	else if(r1->AddrID > r2->AddrID)
		return 1;
	else if(r1->GoodsID < r2->GoodsID)
		return -1;
	else if(r1->GoodsID > r2->GoodsID)
		return 1;
	else
		return 0;
}

int SCDBObjSell::ReadData()
{
	int    ok = -1;
	char   log_msg[128];
	const  LDATE  base_date = encodedate(31, 12, 1995);
	TempList.freeAll();
	if(P_Tbl && P_Tbl->getNumRecs() && P_Tbl->top()) {
		int fldn_cliid = 0;
		int fldn_addrid = 0;
		int fldn_goodsid = 0;
		int fldn_date = 0;
		int fldn_qtty = 0;
		P_Tbl->getFieldNumber("CLIID",   &fldn_cliid);
		P_Tbl->getFieldNumber("ADDRID",  &fldn_addrid);
		P_Tbl->getFieldNumber("GOODSID", &fldn_goodsid);
		P_Tbl->getFieldNumber("DATE",    &fldn_date);
		P_Tbl->getFieldNumber("QTTY",    &fldn_qtty);
		do {
			LDATE   dt;
			double  temp_dbl;
			TempRec temp_rec;
			MEMSZERO(temp_rec);
			DbfRecord rec(P_Tbl);
			P_Tbl->getRec(&rec);
			rec.get(fldn_cliid, temp_rec.ClientID);
			rec.get(fldn_addrid, temp_rec.ClientAddrID);
			rec.get(fldn_goodsid, temp_rec.GoodsID);
			rec.get(fldn_date, dt);
			temp_rec.Date = (int16)diffdate(dt, base_date);
			rec.get(fldn_qtty, temp_dbl);
			temp_rec.Qtty = dbltopalmintmoney(temp_dbl);
			if(!TempList.insert(&temp_rec)) {
				sprintf(log_msg, "SPII ERR: Not enough memory (SCDBObjSell)");
				SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
				CALLEXCEPT();
			}
		} while(P_Tbl->next());
		TempList.sort(PTR_CMPFUNC(SCDBObjSellTempRec));
		ok = 1;
	}
	CATCH
		ok = 0;
		{
			sprintf(log_msg, "SPII ERR: sell export failed");
			SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
		}
	ENDCATCH
	return ok;
}

int SCDBObjSell::Init(const char * pExpPath, const char * pImpPath)
{
	char   path[MAXPATH], fname[MAXPATH];
	setLastSlash(STRNSCPY(path, pExpPath));
	P_Tbl = new DbfTable(strcat(STRNSCPY(fname, path), "sp_sell.dbf"));
	int    ok = ReadData();
	return ok;
}

SCDBObjSell::PalmRec * SCDBObjSell::AllocPalmRec(const TSVector <TempRec> * pPool, size_t * pBufLen) // @v9.8.4 TSArray-->TSVector
{
	PalmRec * p_buf = 0;
	size_t buf_len = 0;
	if(pPool->getCount()) {
		buf_len = sizeof(PalmRec) + pPool->getCount() * sizeof(SalesItem);
		p_buf = static_cast<PalmRec *>(SAlloc::C(1, buf_len));
		if(p_buf) {
			p_buf->ItemsCount = SyncHostToHHWord(pPool->getCount());
			p_buf->ClientID = SyncHostToHHDWord(pPool->at(0).ClientID);
			p_buf->ClientAddrID = SyncHostToHHDWord(pPool->at(0).ClientAddrID);
			p_buf->GoodsID = SyncHostToHHDWord(pPool->at(0).GoodsID);
			for(uint i = 0; i < pPool->getCount(); i++) {
				reinterpret_cast<SalesItem *>(p_buf+1)[i].Date = SyncHostToHHWord(pPool->at(i).Date);
				reinterpret_cast<SalesItem *>(p_buf+1)[i].Qtty = pPool->at(i).Qtty;
			}
		}
	}
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

int SCDBObjSell::GetNextPool(uint * pCurPos, TSVector <TempRec> * pPool) // @v9.8.4 TSArray-->TSVector
{
	pPool->freeAll();
	uint   i = *pCurPos;
	if(i < TempList.getCount()) {
		const TempRec p = TempList.at(i);
		while(i < TempList.getCount()) {
			TempRec r = TempList.at(i);
			if(r.ClientID == p.ClientID && r.ClientAddrID == p.ClientAddrID && r.GoodsID == p.GoodsID) {
				pPool->insert(&r);
				i++;
			}
			else
				break;
		}
		*pCurPos = i;
		return 1;
	}
	return 0;
}

int SCDBObjSell::LogErrRec(const PalmRec * pRec)
{
	char buf[512];
	sprintf(buf, "CliID=%ld, AddrID=%ld, GoodsID=%ld, Count=%d",
		SyncHHToHostDWord(pRec->ClientID), SyncHHToHostDWord(pRec->ClientAddrID),
		SyncHHToHostDWord(pRec->GoodsID), SyncHHToHostWord(pRec->ItemsCount));
	SyncTable::LogMessage(P_Ctx->LogFile, buf);
	return 1;
}

int SCDBObjSell::Export(PROGRESSFN pFn, CSyncProperties * pProps)
{
	int    ok = 1;
	char   log_msg[256];
	long   recno = 0, numrecs = 0, num_out_recs = 0;
	PalmRec * p_out_buf = 0;
	IdxList.freeAll();
	if(!(P_Ctx->PalmCfg.Flags & CFGF_LOADSELLS)) {
		SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: SELL base declined by palm");
		ok = -1;
	}
	else if(cmp(P_Ctx->PalmCfg.TmCliSell, P_Ctx->HostCfg.TmCliSell) >= 0) {
		SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: SELL base on palm is recently that host");
		ok = -1;
	}
	else if(TempList.getCount()) {
		long   last_err = 0;
		uint   cur_pos = 0;
		TSVector <TempRec> pool; // @v9.8.4 TSArray-->TSVector
		numrecs = TempList.getCount();
		SyncTable stbl(P_Ctx->PalmCfg.CompressData(), 0, P_Ctx);
		THROW(stbl.DeleteTable("CliSell.tbl"));
		THROW(stbl.Open("CliSell.tbl", SyncTable::oCreate));
		while(GetNextPool(&cur_pos, &pool)) {
			IdxRec idx_rec;
			size_t buf_len = 0;
			p_out_buf = AllocPalmRec(&pool, &buf_len);
			if(p_out_buf) {
				MEMSZERO(idx_rec);
				idx_rec.CliID = pool.at(0).ClientID;
				idx_rec.AddrID = pool.at(0).ClientAddrID;
				idx_rec.GoodsID = pool.at(0).GoodsID;
				THROW(stbl.AddRec(&idx_rec.RecID, p_out_buf, buf_len));
				num_out_recs++;
				THROW(stbl.Reopen(-1, num_out_recs));
			}
			recno += pool.getCount();
			WaitPercent(pFn, recno, numrecs, "Экспорт статистики продаж");
			ZDELETE(p_out_buf);
		}
		if(P_Ctx->PalmCfg.CompressData())
			P_Ctx->TransmitComprFile = 1;
		stbl.Close();
		{
			sprintf(log_msg, "SPII OK: %ld SELL records exported", num_out_recs);
			SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
		}
	}
	else
		ok = -1;
	CATCH
		ok = 0;
		{
			sprintf(log_msg, "SPII ERR: SELL export failed");
			SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
			if(p_out_buf)
				LogErrRec(p_out_buf);
		}
	ENDCATCH
	delete p_out_buf;
	return ok;
}

