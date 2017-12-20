// CRUKM.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2015, 2016, 2017
// @codepage windows-1251
// Интерфейс (асинхронный) к драйверу ККМ "Кристалл-УКМ" (ООО Кристалл Сервис)
//
#include <pp.h>
#pragma hdrstop

static int SLAPI LStrToZStr(char * pDest, const char * pSrc)
{
	size_t len = (size_t)((uchar *)pSrc)[0];
	memcpy(pDest, pSrc+1, len);
	pDest[len] = 0;
	return 1;
}

static int SLAPI ZStrToLStr(char * pDest, const char * pSrc, size_t lStrBufSize)
{
	size_t len = sstrlen(pSrc);
	len = MIN(MIN(len, lStrBufSize-1), 255);
	((uchar *)pDest)[0] = (uchar)len;
	memcpy(pDest+1, pSrc, len);
	return 1;
}

class ACS_CRUKM : public PPAsyncCashSession {
public:
	SLAPI  ACS_CRUKM(PPID n);
	virtual SLAPI ~ACS_CRUKM();
private:
	struct SessEntry {
		PPID   CashNo;
		PPID   SessNo;
		//
		// Используется в функции FinishImportSession для добавлени
		// в список закрываемых сессий тех сессий, которые поменяли свой статус
		// с незавершенной на завершенную (Temporary < 0).
		//
		PPID   SessID;
		//
		// Признак незавершенности сессии.
		// 0  - Сессия завершена (существует запись в таблице ZREPORT)
		// 1  - Сессия не завершена (не существует запись в таблице ZREPORT).
		//      Такая сессия не должна объединяться в суперсессии и списываться.
		//      Все чеки, относящиеся к этой сессии получают признак CCHKF_TEMPSESS,
		//      (этот признак не сохраняется в БД, а служит только для того, чтобы
		//      базовый класс PPAsyncCashSession рассматривал эти чеки как принадлежащие
		//      незавершенной сессии.
		// -1 - Сессия из незавершенной должна быть превращена в завершенную.
		//      Этот признак выставляется в функции GetSessionData, а используетс
		//      в функции FinishImportSession для добавления в список закрываемых сессий
		//      тех сессий, которые поменяли свой статус с незавершенной на завершенную.
		//
		int16  Temporary;
	};
	virtual int SLAPI ExportData(int updOnly);
	virtual int SLAPI GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int SLAPI ImportSession(int);
	virtual int SLAPI FinishImportSession(PPIDArray *);
	int    SLAPI SearchAbsentSess(SessEntry *, Cr_SaleTbl *, int numForwards);
	int    SLAPI ExportSCard(Cr_PosLoadTbl *, Cr_CredoPayTbl *, const PPIDArray * pLogNumList, int updOnly);
	int    SLAPI ExportOpByCrdCard(SCardCore *, PPID, const char *, Cr_CredoPayTbl *);
	int    SLAPI ProcessSCardByCheck(const Cr_SaleTbl::Rec *, PPID * pSCardID, int * pIsCreditCard);
	int    SLAPI AddSessEntry(PPID cashNo, PPID sessNo, PPID sessID = 0, int isTemporary = 0);
	// Called from GetSessionData
	int    SLAPI UpdateZReportData(Cr_ZReportTbl *);
	// Called from UpdateZReportData
	int    SLAPI CreateAbsentSession(PPID cashNo, PPID sessNo, Cr_ZReportTbl *, Cr_SaleTbl *);
	int    SLAPI SearchSCard(const char * pSrcCardCode, PPID * pSCardID);
	int    SLAPI ExportSection(const AsyncCashGoodsInfo *, Cr_PosLoadTbl::Rec *, int exportSect);

	SArray * P_TmpSessList;
	char   * PP_SalesFileName[2];

	Cr_CashDCrdTbl * P_DsCrdTbl;
	Cr_CashAuthTbl * P_CrCrdTbl;
	PPIDArray  ShopList;
	PPObjSCard SCrdObj;
	PPObjSCardSeries SCrdSObj;
	PPID   StatID;
};

SLAPI  ACS_CRUKM::ACS_CRUKM(PPID n) : PPAsyncCashSession(n)
{
	P_TmpSessList = 0;
	P_DsCrdTbl = 0;
	P_CrCrdTbl = 0;
	PP_SalesFileName[0] = 0;
	PP_SalesFileName[1] = 0;
}

SLAPI ACS_CRUKM::~ACS_CRUKM()
{
	delete P_TmpSessList;
	delete P_DsCrdTbl;
	delete P_CrCrdTbl;
	delete PP_SalesFileName[0];
	delete PP_SalesFileName[1];
}

class CM_CRUKM : public PPCashMachine {
public:
	SLAPI CM_CRUKM(PPID cashID) : PPCashMachine(cashID)
	{
		P_DsCrdTbl = 0;
		P_CSess = 0;
		P_ScObj = 0;
	}
	virtual SLAPI ~CM_CRUKM()
	{
		delete P_DsCrdTbl;
		delete P_CSess;
		delete P_ScObj;
	}
	PPAsyncCashSession * SLAPI AsyncInterface() { return new ACS_CRUKM(NodeID); }
private:
	Cr_CashDCrdTbl * P_DsCrdTbl;
	CSessionCore * P_CSess;
	PPObjSCard * P_ScObj;
	PPIDArray ShopList;
};

REGISTER_CMT(CRUKM,0,1);

int SLAPI ACS_CRUKM::ExportOpByCrdCard(SCardCore * pSc, PPID cardID, const char * pCardCode, Cr_CredoPayTbl * pScCrdTbl)
{
	int    ok = 1;
	//
	// Loading CredoPay (Credit Card operations)
	//
	if(pScCrdTbl) {
		Cr_CredoPayTbl::Rec crp_rec;
		Cr_CredoPayTbl::Key0 crp_k0;
		SCardOpTbl::Rec scop_rec;
		if(pSc->GetLastOpByCard(cardID, ZERODATE, &scop_rec) > 0) {
			MEMSZERO(crp_k0);
			STRNSCPY(crp_k0.BarCode, pCardCode);
			ZStrToLStr(crp_k0.BarCode, pCardCode, sizeof(crp_k0.BarCode));
			crp_k0.PayDate = scop_rec.Dt;
			crp_k0.PayTime = scop_rec.Tm;
			if(pScCrdTbl->searchForUpdate(0, &crp_k0, spEq)) {
				pScCrdTbl->copyBufTo(&crp_rec);
				if(crp_rec.RestSumma != scop_rec.Rest) {
					crp_rec.RestSumma = scop_rec.Rest;
					THROW_DB(pScCrdTbl->updateRecBuf(&crp_rec)); // @sfu
				}
			}
			else {
				MEMSZERO(crp_rec);
				ZStrToLStr(crp_rec.BarCode, pCardCode, sizeof(crp_rec.BarCode));
				crp_rec.PayDate = scop_rec.Dt;
				crp_rec.PayTime = scop_rec.Tm;
				crp_rec.PayType = 5;
				crp_rec.PaySumma  = scop_rec.Rest;
				crp_rec.RestSumma = scop_rec.Rest;
				crp_rec.DocDate = scop_rec.Dt;
				crp_rec.DiscCliID = cardID;
				THROW_DB(pScCrdTbl->insertRecBuf(&crp_rec));
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI ACS_CRUKM::ExportSCard(Cr_PosLoadTbl * pOutTbl, Cr_CredoPayTbl * pScCrdTbl, const PPIDArray * pLogNumList, int updOnly)
{
	int    ok = 1;
	PPID   ser_id = 0;
	PPSCardSeries ser_rec;
	AsyncCashSCardInfo  info;
	AsyncCashSCardsIterator * p_iter = 0;
	THROW_MEM(p_iter = new AsyncCashSCardsIterator(NodeID, updOnly, P_Dls, StatID));
	{
		PPTransaction tra(1);
		THROW(tra);
		MEMSZERO(ser_rec);
		for(ser_id = 0; SCrdSObj.EnumItems(&ser_id, &ser_rec) > 0;) {
			PPSCardSerPacket scs_pack;
			THROW(SCrdSObj.GetPacket(ser_id, &scs_pack) > 0);
			THROW(p_iter->Init(&scs_pack));
			while(p_iter->Next(&info) > 0) {
				AdjustSCardCode(info.Rec.Code);
				if(ser_rec.Flags & SCRDSF_CREDIT && !info.IsClosed)
					THROW(ExportOpByCrdCard(SCrdObj.P_Tbl, info.Rec.ID, info.Rec.Code, pScCrdTbl));
				for(uint i = 0; i < pLogNumList->getCount(); i++) {
					char   tempbuf[80];
					char   cardname[128], * p;
					PPID   log_num = pLogNumList->at(i);
					Cr_PosLoadTbl::Rec rec;
					MEMSZERO(rec);
					ZStrToLStr(rec.Cash_Code, ltoa(log_num, tempbuf, 10), sizeof(rec.Cash_Code));
					rec.DataType   = 11;
					// 0 - discount card, -1 - credit card
					rec.KeyLongint = (ser_rec.Flags & SCRDSF_CREDIT) ? -1 : 0;
					ZStrToLStr(rec.KeyString, info.Rec.Code, sizeof(rec.KeyString));
					tempbuf[0] = info.IsClosed ? '-' : '+';
					tempbuf[1] = 0;
					ZStrToLStr(rec.F_MODE, tempbuf, sizeof(rec.F_MODE));
					ZStrToLStr(rec.F_ARTIKL, info.Rec.Code, sizeof(rec.F_ARTIKL));
					rec.F_CONTR_COST = fdiv100i(info.Rec.PDis);
					rec.F_COST = info.Rec.MaxCredit;
					memzero(cardname, sizeof(cardname));
					p = stpcpy(cardname, info.Rec.Code);
					if(info.PsnName[0]) {
						*p++ = ':';
						p = stpcpy(p, info.PsnName);
					}
					ZStrToLStr(rec.F_FIRM_NAME, cardname, sizeof(rec.F_FIRM_NAME));
					ZStrToLStr(rec.F_S_COD, cardname, sizeof(rec.F_S_COD));
					rec.F_M_COD = info.Rec.ID;
					THROW_DB(pOutTbl->insertRecBuf(&rec));
					p_iter->SetStat();
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	delete p_iter;
	return ok;
}

int SLAPI ACS_CRUKM::ExportSection(const AsyncCashGoodsInfo * pInfo, Cr_PosLoadTbl::Rec * pRec, int exportSect)
{
	if(exportSect) {
		char sect_buf[32];
		ltoa(pInfo->DivN, sect_buf, 10);
		ZStrToLStr(pRec->F_GRCODE, sect_buf, sizeof(pRec->F_GRCODE));
	}
	return 1;
}

int SLAPI ACS_CRUKM::ExportData(int updOnly)
{
	int    ok = 1;
	int    check_dig = 0;
	int    distrib_path = 0, export_section = 0;
	uint   i;
	char   /*path_goods[MAXPATH], path_sccrd[MAXPATH],*/ tempbuf[80];
	SString path_goods, path_sccrd;
	PPIDArray  cash_n_list;
	PPUnit unit_rec;
	PPObjUnit  unit_obj;
	PPID   prev_goods_id = 0, log_num;
	PPAsyncCashNode cn_data;
	Cr_PosLoadTbl  * p_out_tbl = 0;
	Cr_CredoPayTbl * p_sccrd_tbl = 0;
	AsyncCashGoodsInfo       info;
	AsyncCashGoodsIterator * p_iter = 0;
	PPIDArray log_num_list;
	Cr_PosLoadTbl::Rec rec;

	PPWait(1);
	{
		//
		// Выясняем необходимость загрузки на кассу номеров секций
		// pp.ini [config] ExportCrukmSections=0|1
		//
		int    val = 0;
		PPIniFile ini_file;
		if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_EXPORTCRUKMSECT, &val) > 0 && val > 0)
			export_section = 1;
	}
	THROW(GetNodeData(&cn_data) > 0);
	cn_data.GetLogNumList(log_num_list);
	if(log_num_list.getCount() == 0)
		log_num_list.add(1);
	check_dig = BIN(GetGoodsCfg().Flags & GCF_BCCHKDIG);
	{
		StringSet ss;
		uint pos = 0;
		//
		// Making output files paths
		//
		if(GetExpPathSet(&ss) > 0 && ss.get(&pos, path_goods) > 0) {
			SString fname;
			path_goods.RmvLastSlash();
			THROW_PP_S(::access(path_goods, 0) == 0, PPERR_NEXISTPATH, path_goods);
			path_sccrd = path_goods;
			PPGetFileName(PPFILNAM_POSLOAD_BTR, fname);
			path_goods.SetLastSlash().Cat(fname);
			PPGetFileName(PPFILNAM_CREDOPAY_BTR, fname);
			path_sccrd.SetLastSlash().Cat(fname);
			distrib_path = 1;
		}
		else {
			THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_POSLOAD_BTR, path_goods.Z()));
			THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CREDOPAY_BTR, path_sccrd.Z()));
		}
		if(fileExists(path_goods))
			for(int i = 0; i < 10; i++)
				if(!SFile::Remove(path_goods))
					SDelay(500);
		//
		// Creating (if needed) and opening table Cr_PosLoad
		//
		THROW(CheckTblPtr(p_out_tbl = new Cr_PosLoadTbl(path_goods)));
		//
		// Creating (if needed) and opening table Cr_CredoPay
		//
		THROW(CheckTblPtr(p_sccrd_tbl = new Cr_CredoPayTbl(path_sccrd)));
	}
	if(!P_Dls)
		THROW_MEM(P_Dls = new DeviceLoadingStat);
	P_Dls->StartLoading(&StatID, dvctCashs, NodeID, 1);
	THROW_MEM(p_iter = new AsyncCashGoodsIterator(NodeID, (updOnly ? ACGIF_UPDATEDONLY : 0), SinceDlsID, P_Dls));
	{
		PPTransaction tra(1);
		THROW(tra);
		for(i = 0; i < log_num_list.getCount(); i++) {
			log_num = log_num_list.at(i);
			MEMSZERO(rec);
			ZStrToLStr(rec.Cash_Code, ltoa(log_num, tempbuf, 10), sizeof(rec.Cash_Code));
			rec.DataType = 0;
			THROW_DB(p_out_tbl->insertRecBuf(&rec));
		}
		while(p_iter->Next(&info) > 0) {
			const size_t bclen = strlen(info.BarCode);
			if(bclen) {
				if(bclen != 3 && bclen != (7+check_dig) && bclen < (size_t)(12+check_dig))
					padleft(info.BarCode, '0', (12+check_dig) - bclen);
				AddCheckDigToBarcode(info.BarCode);
			}
			for(i = 0; i < log_num_list.getCount(); i++) {
				log_num = log_num_list.at(i);
				if(info.ID != prev_goods_id) {
					MEMSZERO(rec);
					ZStrToLStr(rec.Cash_Code, ltoa(log_num, tempbuf, 10), sizeof(rec.Cash_Code));
					rec.DataType   = 1;
					rec.KeyLongint = info.ID;
					ZStrToLStr(rec.F_MODE, "+", sizeof(rec.F_MODE));
					ExportSection(&info, &rec, export_section);
					ZStrToLStr(rec.F_FIRM_NAME, info.Name, sizeof(rec.F_FIRM_NAME));
					ZStrToLStr(rec.F_ARTIKL,    ltoa(info.ID, tempbuf, 10), sizeof(rec.F_ARTIKL));
					ZStrToLStr(rec.F_CODE,      ltoa(info.ID, tempbuf, 10), sizeof(rec.F_CODE));
					if(bclen && info.UnitPerPack == 1)
						ZStrToLStr(rec.F_BAR_CODE, info.BarCode, sizeof(rec.F_BAR_CODE));
					unit_obj.Fetch(info.UnitID, &unit_rec);
					ZStrToLStr(rec.F_UNIT, unit_rec.Name, sizeof(rec.F_UNIT));
					ZStrToLStr(rec.F_CURRENCY,  "RUB", sizeof(rec.F_CURRENCY));
					ZStrToLStr(rec.F_ENABLEX,   "E", sizeof(rec.F_ENABLEX));
					rec.F_COST      = info.Price;
					rec.F_PRESISION = (unit_rec.Flags & PPUnit::IntVal) ? 1 : fpow10i(-3);
					//
					// @v3.10.0 Загрузка фиксированных цен на товары,
					// предоставляемых по дисконтным картам
					//
					if(info.ExtQuot > 0) {
						rec.F_EXTRA_SUM = info.ExtQuot;
						ZStrToLStr(rec.F_SERIA, ltoa(1 /* one */, tempbuf, 10), sizeof(rec.F_ARTIKL));
					}
					THROW_DB(p_out_tbl->insertRecBuf(&rec));
					//
					// Загрузка запрета на скидку для товара (предельная цена).
					//
					if(info.NoDis > 0 || info.NoDis < 0) {
						int    rmv_limit = BIN(info.NoDis < 0);
						MEMSZERO(rec);
						ZStrToLStr(rec.Cash_Code, ltoa(log_num, tempbuf, 10), sizeof(rec.Cash_Code));
						rec.DataType   = 7;
						rec.KeyLongint = info.ID;
						ZStrToLStr(rec.KeyString, ltoa(info.ID, tempbuf, 10), sizeof(rec.KeyString));
						ZStrToLStr(rec.F_MODE, rmv_limit ? "-" : "+", sizeof(rec.F_MODE));
						ExportSection(&info, &rec, export_section);
						ZStrToLStr(rec.F_ARTIKL, ltoa(info.ID, tempbuf, 10), sizeof(rec.F_ARTIKL));
						ZStrToLStr(rec.F_CODE, ltoa(info.ID, tempbuf, 10), sizeof(rec.F_CODE));
						rec.F_CONTR_COST = rmv_limit ? 0 : 100; // Проценты от цены
						THROW_DB(p_out_tbl->insertRecBuf(&rec));
					}
				}
				if(bclen != 0 && (info.ID == prev_goods_id || info.UnitPerPack != 1)) {
					MEMSZERO(rec);
					ZStrToLStr(rec.Cash_Code, ltoa(log_num, tempbuf, 10), sizeof(rec.Cash_Code));
					rec.DataType = 2;
					rec.KeyLongint = 0L;
					ZStrToLStr(rec.KeyString, info.BarCode, sizeof(rec.KeyString));
					ZStrToLStr(rec.F_ARTIKL,  info.BarCode, sizeof(rec.F_ARTIKL));
					ZStrToLStr(rec.F_CODE,    ltoa(info.ID, tempbuf, 10), sizeof(rec.F_CODE));
					ZStrToLStr(rec.F_MODE, "+", sizeof(rec.F_MODE));
					ExportSection(&info, &rec, export_section);
					rec.F_QUANT     = info.UnitPerPack;
					rec.F_PRESISION = fpow10i(-3);
					THROW_DB(p_out_tbl->insertRecBuf(&rec));
				}
			}
	   		prev_goods_id = info.ID;
			PPWaitPercent(p_iter->GetIterCounter());
		}
		ZDELETE(p_iter);
		/*THROW(ReadEquipConfig(&eq_cfg));
		if(eq_cfg.CashiersGrpID) {
			char  rights[36];
			PPID  cashier_id;
			PPSecur    cashier_rec;
			PPObjSecur obj_secur(PPOBJ_USR, 0);
			memset(rights, '.', sizeof(rights));
			rights[35] = 0;
			while(obj_secur.EnumItems(&cashier_id, &cashier_rec) > 0)
				if(cashier_rec.ParentID == eq_cfg.CashiersGrpID && cashier_rec.ID != PPUSR_MASTER) {
					uint   pos = 0;
					char * p_name = 0, name[40], password[20];
					const  char * p_mode = 0;
					StringSet ss(':', 0);
					ss.add(cashier_rec.Name);
					ss.get(&pos, name, sizeof(name));
					ss.get(&pos, password, sizeof(password));
					if(name[0] == '-') {
						p_mode = "-";
						p_name = name + 1;
					}
					else {
						p_mode = "+";
						p_name = name;
					}
					for(i = 0; i < log_num_list.getCount(); i++) {
						log_num = log_num_list.at(i);
						MEMSZERO(rec);
						ZStrToLStr(rec.Cash_Code, ltoa(log_num, tempbuf, 10), sizeof(rec.Cash_Code));
						rec.DataType = 3;
						rec.KeyLongint = 0L;
						ZStrToLStr(rec.KeyString, ltoa(cashier_rec.ID, tempbuf, 10), sizeof(rec.KeyString));
						ZStrToLStr(rec.F_MODE,      p_mode,   sizeof(rec.F_MODE));
						ZStrToLStr(rec.F_FIRM_NAME, password, sizeof(rec.F_FIRM_NAME));
						ZStrToLStr(rec.F_ARTIKL,    p_name,   sizeof(rec.F_ARTIKL));
						ZStrToLStr(rec.F_S_COD,     rights,   sizeof(rec.F_S_COD));
						rec.F_M_COD = cashier_rec.ID;
						THROW_DB(p_out_tbl->insertRec());
					}
				}
		}*/
		THROW(tra.Commit());
	}
	THROW(ExportSCard(p_out_tbl, p_sccrd_tbl, &log_num_list, updOnly));
	if(!distrib_path) {
		ZDELETE(p_out_tbl);
		THROW(DistributeFile(path_goods, 0));
	}
	if(StatID)
		P_Dls->FinishLoading(StatID, 1, 1);
	CATCHZOK
	PPWait(0);
	delete p_iter;
	delete p_out_tbl;
	delete p_sccrd_tbl;
	return ok;
}

int SLAPI ACS_CRUKM::AddSessEntry(PPID cashNo, PPID sessNo, PPID sessID, int temporary)
{
	int    ok = 1, found = 0;
	uint   i;
	SessEntry sess_entry, * p_sess_entry;
	sess_entry.CashNo = cashNo;
	sess_entry.SessNo = sessNo;
	sess_entry.SessID = sessID;
	sess_entry.Temporary = temporary;
	if(P_TmpSessList == 0)
		THROW_MEM(P_TmpSessList = new SArray(sizeof(SessEntry)));
	for(i = 0; !found && P_TmpSessList->enumItems(&i, (void **)&p_sess_entry);)
		if(p_sess_entry->CashNo == cashNo && p_sess_entry->SessNo == sessNo)
			found = 1;
	if(!found)
		THROW_SL(P_TmpSessList->insert(&sess_entry));
	CATCHZOK
	return ok;
}

int SLAPI ACS_CRUKM::SearchAbsentSess(SessEntry * pSessEntry, Cr_SaleTbl * pTbl, int numForwards)
{
	int    ok = 1;
	PPID   sess_id = 0, sess_no = 0;
	PPID   z = MAX(pSessEntry->SessNo-12, 0);
	Cr_SaleTbl::Key8 k8;
	for(sess_no = pSessEntry->SessNo; sess_no > z; sess_no--)
		if(CS.SearchByNumber(&sess_id, NodeID, pSessEntry->CashNo, sess_no, ZERODATE) <= 0) {
			k8.Cash_Code = pSessEntry->CashNo;
			k8.NSmena    = sess_no;
			if(pTbl->search(8, &k8, spEq))
				THROW(AddSessEntry(pSessEntry->CashNo, sess_no, 0, 0));
		}
	z = (pSessEntry->SessNo+numForwards);
	for(sess_no = pSessEntry->SessNo+1; sess_no <= z; sess_no++) {
		k8.Cash_Code = pSessEntry->CashNo;
		k8.NSmena    = sess_no;
		if(pTbl->search(8, &k8, spEq))
			THROW(AddSessEntry(pSessEntry->CashNo, sess_no, 0, 1));
	}
	CATCHZOK
	return ok;
}

int SLAPI ACS_CRUKM::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd /*=0*/)
{
	int    ok = 1;
	uint   i, j;
	PPIDArray log_num_list;
	ShopList.freeAll();
	ZDELETE(P_DsCrdTbl);
	ZDELETE(P_CrCrdTbl);
	ZDELETE(PP_SalesFileName[0]);
	ZDELETE(PP_SalesFileName[1]);

	int    sess_count = 0, is_forward = 0;
	int    num_forwards = 0;
	PPID   sess_id = 0;
	SString imp_path;
	SString fname, file_name, srch_path;

	SessEntry * p_sess_entry = 0;
	SArray max_sess_list(sizeof(SessEntry));
	PPAsyncCashNode cn_data;
	//SDirEntry fb;
	//PPFileNameArray fary;
	SFileEntryPool fep;
	SDirec sdirec;

	Cr_ZReportTbl * p_zr_tbl = 0;
	Cr_ZReportTbl::Key0 k0;

	THROW(GetNodeData(&cn_data) > 0);
	cn_data.GetLogNumList(log_num_list);
	THROW_PP(cn_data.ImpFiles.NotEmpty(), PPERR_INVFILESET);
	(imp_path = cn_data.ImpFiles).SetLastSlash();
	//
	//
	//
	(fname = imp_path).Cat("CASHDCRD.BTR");
	if(::fileExists(fname)) {
		THROW_MEM(P_DsCrdTbl = new Cr_CashDCrdTbl(fname));
		{
			Cr_CashDCrdTbl::Key0 dc_k0;
			BExtQuery q(P_DsCrdTbl, 0);
			q.select(P_DsCrdTbl->ShopIndex, 0L);
			MEMSZERO(dc_k0);
			for(q.initIteration(0, &dc_k0, spFirst); q.nextIteration() > 0;)
				THROW(ShopList.addUnique(P_DsCrdTbl->data.ShopIndex));
		}
	}
	(fname = imp_path).Cat("CASHAUTH.BTR");
	if(::fileExists(fname)) {
		THROW_MEM(P_CrCrdTbl = new Cr_CashAuthTbl(fname));
		{
			Cr_CashAuthTbl::Key0 cc_k0;
			BExtQuery q(P_CrCrdTbl, 0);
			q.select(P_CrCrdTbl->ShopIndex, 0L);
			MEMSZERO(cc_k0);
			for(q.initIteration(0, &cc_k0, spFirst); q.nextIteration() > 0;)
				THROW(ShopList.addUnique(P_CrCrdTbl->data.ShopIndex));
		}
	}
	//
	//
	//
	PPGetFileName(PPFILNAM_ZREPORT_BTR, file_name);
	(fname = imp_path).Cat(file_name);
	THROW_PP(::fileExists(fname), PPERR_INVFILESET);
	THROW_MEM(p_zr_tbl = new Cr_ZReportTbl(fname));
	THROW_DB(CheckTblPtr(p_zr_tbl));
	//
	// Searching file with sales (last CQ?????.BTR)
	// Assigned to PP_SalesFileName
	//
	/*PPGetFileName(PPFILNAM_CQXXXXXX_BTR, file_name); // cq??????.btr
	(srch_path = imp_path).Cat(file_name);
	for(sdirec.Init(srch_path); sdirec.Next(&fb) > 0;)
		THROW_SL(fary.ordInsert(&fb, 0, PTR_CMPFUNC(SDirEntry_Time)));
	THROW_PP(fary.getCount(), PPERR_INVFILESET);*/
	//(fname = imp_path).Cat(fary.at(fary.getCount()-1).FileName);
	fep.Scan(imp_path, "cq??????.btr", 0);
	fep.Sort(fep.scByAccsTime|fep.scDesc);
	THROW_PP(fep.GetCount(), PPERR_INVFILESET);
	fep.Get(0, 0, &fname);

	THROW_PP(::fileExists(fname), PPERR_INVFILESET);
	THROW_MEM(PP_SalesFileName[0] = newStr(fname));
	//if(fary.getCount() > 1) {
	if(fep.GetCount() > 1) {
		//(fname = imp_path).Cat(fary.at(fary.getCount()-2).FileName);
		fep.Get(1, 0, &fname);
		if(::fileExists(fname))
			THROW_MEM(PP_SalesFileName[1] = newStr(fname));
	}
	//
	//
	//
	THROW(UpdateZReportData(p_zr_tbl)); // @??? (@v4.0.7 see call SearchAbsentSess below)
	k0.CashNumber = MAXLONG;
	k0.ZNumber    = MAXLONG;
	if(p_zr_tbl->search(0, &k0, spLe))
		do {
			sess_id = 0;
			PPID   cash_no = p_zr_tbl->data.CashNumber;
			if(!log_num_list.getCount() || log_num_list.lsearch(cash_no)) {
				int    found = 0;
				PPID   sess_no = p_zr_tbl->data.ZNumber;
				int    sess_found = BIN(CS.SearchByNumber(&sess_id, NodeID, cash_no, sess_no, p_zr_tbl->data.ZDate) > 0);
				if(!sess_found || CS.data.Temporary) {
					int    temporary = 0;
					if(sess_found && CS.data.Temporary) {
						THROW(CS.ResetTempSessTag(sess_id, 0));
						temporary = -1;
					}
					THROW(AddSessEntry(cash_no, sess_no, sess_id, temporary));
				}
				for(i = 0; max_sess_list.enumItems(&i, (void**)&p_sess_entry);)
					if(p_sess_entry->CashNo == cash_no) {
						if(p_sess_entry->SessNo < sess_no)
							p_sess_entry->SessNo = sess_no;
						found = 1;
					}
				if(!found) {
					SessEntry sess_entry;
					sess_entry.CashNo = cash_no;
					sess_entry.SessNo = sess_no;
					sess_entry.SessID = 0;
					sess_entry.Temporary = 0;
					THROW_SL(max_sess_list.insert(&sess_entry));
				}
			}
		} while(p_zr_tbl->search(0, &k0, spPrev));
	//
	// Searching sessions for wich absents record in cr_zreport records,
	// but exists checks in cr_sale table
	//
	num_forwards = CheckCnFlag(CASHF_IMPORTCHECKSWOZR) ? 3 : 0;
	for(j = 0; j < 2; j++)
		if(PP_SalesFileName[j] != 0) {
			Cr_SaleTbl sales_tbl(PP_SalesFileName[j]);
			for(i = 0; max_sess_list.enumItems(&i, (void**)&p_sess_entry);)
				THROW(SearchAbsentSess(p_sess_entry, &sales_tbl, num_forwards));
		}
	CATCH
		ok = 0;
		ZDELETE(P_TmpSessList);
		sess_count = 0;
	ENDCATCH
	delete p_zr_tbl;
	sess_count = P_TmpSessList ? P_TmpSessList->getCount() : 0;
	ASSIGN_PTR(pSessCount, sess_count);
	ASSIGN_PTR(pIsForwardSess, is_forward);
	return ok;
}

int SLAPI ACS_CRUKM::CreateAbsentSession(PPID cashNo, PPID sessNo, Cr_ZReportTbl * pZReportTbl, Cr_SaleTbl * pSalesTbl)
{
	int    ok = -1;
	Cr_ZReportTbl::Key0 k0;
	k0.CashNumber = cashNo;
	k0.ZNumber    = sessNo;
	if(pZReportTbl->search(0, &k0, spEq) == 0) {
		if(BTROKORNFOUND) {
			LDATETIME last;
			last.SetZero();
			Cr_SaleTbl::Key8 k8;
			BExtQuery q(pSalesTbl, 8);
			q.select(pSalesTbl->DateOperation, pSalesTbl->Times, 0L).
				where(pSalesTbl->Cash_Code == cashNo && pSalesTbl->NSmena == sessNo);
			k8.Cash_Code = cashNo;
			k8.NSmena = sessNo;
			for(q.initIteration(0, &k8, spEq); q.nextIteration() > 0;)
				if(cmp(last, pSalesTbl->data.DateOperation, pSalesTbl->data.Times) < 0)
					last.Set(pSalesTbl->data.DateOperation, pSalesTbl->data.Times);
			pZReportTbl->clearDataBuf();
			pZReportTbl->data.CashNumber = cashNo;
			pZReportTbl->data.ZNumber = sessNo;
			pZReportTbl->data.ZDate = last.d;
			pZReportTbl->data.ZTime = last.t;
			ok = pZReportTbl->insertRec() ? 1 : PPSetErrorDB();
		}
		else
			ok = PPSetErrorDB();
	}
	return ok;
}

int SLAPI ACS_CRUKM::UpdateZReportData(Cr_ZReportTbl * pZReportTbl)
{
	for(int i = 0; i < 2; i++)
		if(PP_SalesFileName[i] != 0) {
			Cr_SaleTbl sales_tbl(PP_SalesFileName[i]);
			Cr_SaleTbl::Key8 k8;
			k8.Cash_Code = MAXLONG;
			k8.NSmena = MAXLONG;
			if(sales_tbl.search(8, &k8, spLe))
				for(PPID cash_no = sales_tbl.data.Cash_Code; cash_no > 0; cash_no--) {
					k8.Cash_Code = cash_no;
					k8.NSmena    = MAXLONG;
					if(sales_tbl.search(8, &k8, spLe) && cash_no == sales_tbl.data.Cash_Code) {
						PPID z = MAX(sales_tbl.data.NSmena - 12, 0);
						for(PPID sess_no = sales_tbl.data.NSmena - 1; sess_no > z; sess_no--) {
							k8.Cash_Code = cash_no;
							k8.NSmena = sess_no;
							if(sales_tbl.search(8, &k8, spEq))
								if(!CreateAbsentSession(cash_no, sess_no, pZReportTbl, &sales_tbl))
									return 0;
						}
					}
				}
		}
	return 1;
}

/*
static char * SLAPI MakeMsgByCheck(const LDATETIME * pDT, long cashCode, long chkNumber, char * pBuf)
{
	char * p = pBuf;
	p += strlen(datefmt(&pDT->d, DATF_DMY, p));
	p = catdiv(p, ':', 1);
	if(pDT->t) {
		p += strlen(timefmt(pDT->t, TIMF_HMS | TIMF_MSEC, p));
		p = catdiv(p, ':', 1);
	}
	p += strlen(ltoa(cashCode, p, 10));
	p = catdiv(p, ':', 1);
	p += strlen(ltoa(chkNumber, p, 10));
	return pBuf;
}
*/

static SString & SLAPI MakeMsgByCheck(const LDATETIME * pDT, long cashCode, long chkNumber, SString & rBuf)
{
	return rBuf.Z().Cat(*pDT, DATF_DMY, TIMF_HMS|TIMF_MSEC).CatDiv(':', 1).Cat(cashCode).CatDiv(':', 1).Cat(chkNumber);
}

static void SLAPI WaitMsgByCheck(const LDATETIME * pDT, long cashCode, long chkNumber)
{
	SString msg_buf;
	PPWaitMsg(MakeMsgByCheck(pDT, cashCode, chkNumber, msg_buf));
}

int SLAPI ACS_CRUKM::FinishImportSession(PPIDArray * pSessList)
{
	int    ok = 1;
	ZDELETE(P_DsCrdTbl);
	ZDELETE(P_CrCrdTbl);
	ShopList.freeAll();
	if(P_TmpSessList) {
		SessEntry * p_sess_entry;
		for(uint i = 0; P_TmpSessList->enumItems(&i, (void **)&p_sess_entry);)
			if(p_sess_entry->Temporary < 0)
				pSessList->addUnique(p_sess_entry->SessID);
	}
	return ok;
}

int SLAPI ACS_CRUKM::SearchSCard(const char * pSrcCardCode, PPID * pSCardID)
{
	int    ok = -1;
	char   card_code[32];
	SCardTbl::Rec card_rec;
	LStrToZStr(card_code, pSrcCardCode);
	strip(card_code);
	if(SCrdObj.SearchCode(0, card_code, &card_rec) > 0) {
		ASSIGN_PTR(pSCardID, card_rec.ID);
		ok = 1;
	}
	else if(CheckCnFlag(CASHF_EXPCHECKD)) {
		//
		// Так как номера карточек могли быть переданы в кассовый модуль с
		// искусственно добавленной контрольной цифрой, то пытаемся убрать
		// контрольную цифру и найти карту по измененному номеру.
		//
		size_t len = strlen(card_code);
		if(len == 13) {
			card_code[len-1] = 0;
			if(SCrdObj.SearchCode(0, card_code, &card_rec) > 0) {
				ASSIGN_PTR(pSCardID, card_rec.ID);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI ACS_CRUKM::ProcessSCardByCheck(const Cr_SaleTbl::Rec * pInRec, PPID * pSCardID, int * pIsCreditCard)
{
	int    ok = -1;
	uint   j;
	PPID   card_id = 0;
	ASSIGN_PTR(pIsCreditCard, 0);
	//
	// Searching credit card
	//
	if(P_CrCrdTbl)
		for(j = 0; !card_id && j < ShopList.getCount(); j++) {
			Cr_CashAuthTbl::Key0 ca_k0;
			MEMSZERO(ca_k0);
			ca_k0.ShopIndex   = (int16)ShopList.at(j);
			ca_k0.CashNumber  = (int16)pInRec->Cash_Code;
			ca_k0.ZNumber     = (int16)pInRec->NSmena;
			ca_k0.CheckNumber = pInRec->Ck_Number;
			if(P_CrCrdTbl->search(0, &ca_k0, spGe) && ca_k0.CashNumber == pInRec->Cash_Code &&
				ca_k0.ZNumber == pInRec->NSmena && ca_k0.CheckNumber == pInRec->Ck_Number)
				if(SearchSCard(P_CrCrdTbl->data.CardNumber, &card_id) > 0) {
					ASSIGN_PTR(pIsCreditCard, 1);
					ok = 1;
				}
		}
	//
	// Searching discount card
	//
	if(P_DsCrdTbl)
		for(j = 0; !card_id && j < ShopList.getCount(); j++) {
			Cr_CashDCrdTbl::Key0 cd_k0;
			MEMSZERO(cd_k0);
			cd_k0.ShopIndex   = (int16)ShopList.at(j);
			cd_k0.CashNumber  = (int16)pInRec->Cash_Code;
			cd_k0.ZNumber     = (int16)pInRec->NSmena;
			cd_k0.CheckNumber = pInRec->Ck_Number;
			if(P_DsCrdTbl->search(0, &cd_k0, spGe) && cd_k0.CashNumber == pInRec->Cash_Code &&
				cd_k0.ZNumber == pInRec->NSmena && cd_k0.CheckNumber == pInRec->Ck_Number)
				if(SearchSCard(P_DsCrdTbl->data.CardNumber, &card_id) > 0)
					ok = 1;
		}
	ASSIGN_PTR(pSCardID, card_id);
	return ok;
}

int SLAPI ACS_CRUKM::ImportSession(int sessIdx)
{
	int    ok = -1, r, i;
	int    is_tables_created = 0;
	LDATE  last_date = ZERODATE;
	SString msg_buf;
	Cr_SaleTbl::Key8 k8;
	const SessEntry * p_entry = (SessEntry*)P_TmpSessList->at(sessIdx);

	for(i = 0; i < 2; i++) {
		if(PP_SalesFileName[i] && fileExists(PP_SalesFileName[i])) {
			Cr_SaleTbl in_tbl(PP_SalesFileName[i]);
			k8.Cash_Code = p_entry->CashNo;
			k8.NSmena    = p_entry->SessNo;
			if(in_tbl.search(8, &k8, spEq)) {
				if(!is_tables_created) {
					THROW(CreateTables());
					is_tables_created = 1;
				}
				do {
					Cr_SaleTbl::Rec in_rec;
					int    sign;
					PPID   chk_id = 0;
					double qtty, price, discount;
					double line_amount;
					char   tmpbuf[64];
					in_tbl.copyBufTo(&in_rec);
					if(in_rec.DateOperation > last_date)
						last_date = in_rec.DateOperation;
					if(last_date && labs(diffdate(&last_date, &in_rec.DateOperation, 0)) > 3)
						continue;
					LStrToZStr(tmpbuf, in_rec.Operation);
					sign = (toupper(tmpbuf[0]) == 'R') ? 0 : 1;
					THROW(r = SearchTempCheckByCode(in_rec.Cash_Code, in_rec.Ck_Number));
					if(r > 0)
						chk_id = P_TmpCcTbl->data.ID;
					else {
						LDATETIME dm;
						dm.Set(in_rec.DateOperation, in_rec.Times);
						PPID   card_id = 0;
						int    is_credit_card = 0;
						long   f = (p_entry->Temporary > 0) ? CCHKF_TEMPSESS : 0;
						THROW(ProcessSCardByCheck(&in_rec, &card_id, &is_credit_card));
						while(SearchTempCheckByTime(in_rec.Cash_Code, &dm) > 0) {
							int    h, m, s, hs;
							decodetime(&h, &m, &s, &hs, &dm.t);
							THROW_PP_S(hs < 99, PPERR_DUPTEMPCHECK, MakeMsgByCheck(&dm, in_rec.Cash_Code, in_rec.Ck_Number, msg_buf));
							dm.t = encodetime(h, m, s, hs + 1);
						}
						WaitMsgByCheck(&dm, in_rec.Cash_Code, in_rec.Ck_Number);
						THROW(AddTempCheck(&chk_id, in_rec.NSmena, f, in_rec.Cash_Code, in_rec.Ck_Number, 0, card_id, &dm, 0, 0));
					}
					qtty = (sign == 0) ? -fabs(in_rec.Quant) : fabs(in_rec.Quant);
					SetupTempCcLineRec(0, chk_id, in_rec.Ck_Number, in_rec.DateOperation, 0, in_rec.Code);
					if(in_rec.Code) {
						line_amount = (in_rec.Price * qtty - (in_rec.Ck_Disc + in_rec.Ck_Disg));
						price    = R2((qtty != 0) ? (line_amount / qtty) : line_amount);
						discount = 0L; // in_rec.Ck_Disc
					}
					else {
						line_amount = in_rec.Summa;
						price    = R2((qtty != 0) ? (in_rec.Summa / qtty) : in_rec.Summa);
						discount = 0L; // in_rec.Ck_Disc
					}
					line_amount = R2(line_amount);
					SetTempCcLineValues(0, qtty, price, discount);
					THROW_DB(P_TmpCclTbl->insertRec());
					THROW(AddTempCheckAmounts(chk_id, line_amount, in_rec.Ck_Disc + in_rec.Ck_Disg));
				} while(in_tbl.search(8, &k8, spNext) && k8.Cash_Code == p_entry->CashNo && k8.NSmena == p_entry->SessNo);
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}
