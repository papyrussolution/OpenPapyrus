// CRYSTAZS.CPP
// Copyright (c) V.Nasonov 2004, 2006, 2007, 2008, 2010, 2011, 2015, 2016
// @codepage windows-1251
// Интерфейс (асинхронный) к драйверу "Кристалл-АЗС"
//
#include <pp.h>
#pragma hdrstop
#include <process.h>

#define CRAZS_CHK_RETURN     -1
#define CRAZS_PAYMENT_CASH    0
class ACS_CRAZS : public PPAsyncCashSession {
public:
	SLAPI  ACS_CRAZS(PPID n) : PPAsyncCashSession(n)
	{
		ChkRepPeriod.SetZero();
	}
	virtual int SLAPI ExportData(int updOnly);
	virtual int SLAPI GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int SLAPI ImportSession(int);
private:
	int    SLAPI FlashCheck(CCheckTbl::Rec * pChkRec, SArray * pRows);
	int    SLAPI ConvertWareList(PPID cashNo);
	DateRange ChkRepPeriod;
	PPIDArray LogNumList;
	SString PathRpt;
	SString PathImp;
};

class CM_CRAZS : public PPCashMachine {
public:
	SLAPI CM_CRAZS(PPID cashID) : PPCashMachine(cashID) {}
	PPAsyncCashSession * SLAPI AsyncInterface() { return new ACS_CRAZS(NodeID); }
};

REGISTER_CMT(CRAZS,0,1);

int SLAPI ACS_CRAZS::ExportData(int updOnly)
{
	int    ok = 1;
	int    check_dig = 0, add_chkdig = 0;
	SString buf;
	SString path_goods;
	SString path_imp_exe;
	PPGoodsConfig  gcfg;
	GoodsCore      gds_core;
	Goods2Tbl::Rec gds_rec;
	PPID   prev_goods_id = 0;
	PPAsyncCashNode cn_data;
	AsyncCashGoodsInfo info;
	AsyncCashGoodsIterator * p_iter = 0;
	FILE * p_file = 0;

	THROW(GetNodeData(&cn_data) > 0);
	THROW(PPObjGoods::ReadConfig(&gcfg));
	check_dig  = (gcfg.Flags & GCF_BCCHKDIG) ? 1 : 0;
	add_chkdig = (cn_data.Flags & CASHF_EXPCHECKD && !check_dig) ? 1 : 0;

	PPWait(1);
	THROW(PPGetFileName(PPFILNAM_CRAZS_IMP_EXE, path_imp_exe));
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CRAZS_IMP_TXT, path_goods));
	PPSetAddedMsgString(path_goods);
	THROW_PP(p_file = fopen(path_goods, onecstr('w')), PPERR_CANTOPENFILE);
	THROW_MEM(p_iter = new AsyncCashGoodsIterator(NodeID, (updOnly ? ACGIF_UPDATEDONLY : 0), SinceDlsID, 0));
	if(!updOnly) {
		THROW(PPLoadText(PPTXT_CRAZS_CMD_CLEARALL, buf));
	}
	else
		buf.Space() = 0;
	THROW_PP(fprintf(p_file, "%s\n", (const char *)buf) > 0, PPERR_EXPFILEWRITEFAULT);
	while(p_iter->Next(&info) > 0) {
		size_t bclen;
	   	if(info.ID != prev_goods_id) {
			char   tmp_buf[80], * p = 0;
			if(prev_goods_id)
				THROW_PP(fprintf(p_file, "%s\n", (const char *)buf) > 0, PPERR_EXPFILEWRITEFAULT);
			(buf = 0).Cat(info.ID);
			STRNSCPY(tmp_buf, info.Name);
			SOemToChar(tmp_buf);
			for(p = tmp_buf; (p = strchr(p, ',')) != 0; *p = '.');
			buf.Comma().Cat(tmp_buf); // Наименование товара
			intfmt((long)(info.Price * 100), 0, tmp_buf);
			buf.Comma().Cat((long)(info.Price * 100)); // Цена товара
			if(gds_core.Search(info.ID, &gds_rec) > 0 && gds_rec.ParentID &&
				gds_core.Search(gds_rec.ParentID, &gds_rec) > 0) {
				STRNSCPY(tmp_buf, gds_rec.Name);
				SOemToChar(tmp_buf);
				for(p = tmp_buf; (p = strchr(p, ',')) != 0; * p = '.');
			}
			else
				tmp_buf[0] = 0;
			buf.Comma().Cat(tmp_buf);  // Наименование группы товаров
			buf.Comma().Cat(1000000L); // Остаток
		}
		if((bclen = strlen(info.BarCode)) != 0) {
			info.AdjustBarcode(check_dig);
			size_t len = strlen(info.BarCode);
			if(add_chkdig && len > 3 && !gcfg.IsWghtPrefix(info.BarCode))
				AddBarcodeCheckDigit(info.BarCode);
			buf.Comma().Cat(info.BarCode);
		}
	   	prev_goods_id = info.ID;
		PPWaitPercent(p_iter->GetIterCounter());
	}
	if(prev_goods_id)
		THROW_PP(fprintf(p_file, "%s\n", (const char *)buf) > 0, PPERR_EXPFILEWRITEFAULT);
	SFile::ZClose(&p_file);
	THROW(DistributeFile(path_goods, 0));
	{
		StringSet ss(';', 0);
		if(GetExpPathSet(&ss) > 0) {
			char   cur_dir[MAXPATH], path[MAXPATH];
			const  char empty = 0;
			char   temp_path_goods[MAXPATH];
			STRNSCPY(temp_path_goods, path_goods);
			replacePath(temp_path_goods, &empty, 1);
			GetCurrentDirectory(sizeof(cur_dir), cur_dir); // @unicodeproblem
			for(uint i = 0; ss.get(&i, path, sizeof(path)) > 0;) {
				SPathStruc::ReplacePath(path_imp_exe, path, 1);
				if(fileExists(path_imp_exe)) {
					::SetCurrentDirectory(path); // @unicodeproblem
					SPathStruc::ReplacePath(path_imp_exe, &empty, 1);
					spawnl(_P_WAIT, path_imp_exe, path_imp_exe, temp_path_goods, NULL);
				}
			}
			::SetCurrentDirectory(cur_dir); // @unicodeproblem
		}
	}
	CATCH
		SFile::ZClose(&p_file);
		ok = 0;
	ENDCATCH
	PPWait(0);
	return ok;
}

int SLAPI ACS_CRAZS::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd /*=0*/)
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(!pPrd) {
		dlg = new TDialog(DLG_SELSESSRNG);
		if(CheckDialogPtr(&dlg, 1)) {
			SString dt_buf;
			ChkRepPeriod.low = LConfig.OperDate;
			ChkRepPeriod.upp = LConfig.OperDate;
			SetupCalCtrl(CTLCAL_DATERNG_PERIOD, dlg, CTL_DATERNG_PERIOD, 1);
			SetPeriodInput(dlg, CTL_DATERNG_PERIOD, &ChkRepPeriod);
			PPWait(0);
			for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
				if(dlg->getCtrlString(CTL_DATERNG_PERIOD, dt_buf) && getperiod(dt_buf, &ChkRepPeriod) && !ChkRepPeriod.IsZero()) {
					if(!ChkRepPeriod.upp)
					ChkRepPeriod.upp = plusdate(LConfig.OperDate, 2);
					if(diffdate(ChkRepPeriod.upp, ChkRepPeriod.low) >= 0)
						ok = valid_data = 1;
				}
				if(ok < 0)
					PPErrorByDialog(dlg, CTL_DATERNG_PERIOD, PPERR_INVPERIODINPUT);
			}
			PPWait(1);
		}
	}
	else {
		ChkRepPeriod = *pPrd;
		ok = 1;
	}
	if(ok > 0) {
		PPAsyncCashNode acn;
		THROW(GetNodeData(&acn) > 0);
		acn.GetLogNumList(&LogNumList);
		THROW_PP(acn.ImpFiles.NotEmpty(), PPERR_INVFILESET);
		THROW(PPGetFileName(PPFILNAM_CRAZS_EXP_TXT, PathRpt));
		SPathStruc::ReplacePath(PathRpt, acn.ImpFiles, 1);
		PathImp = acn.ImpFiles;
	}
	CATCHZOK
	*pSessCount = ok > 0 ? 1 : 0;
	*pIsForwardSess = 1;
	delete dlg;
	return ok;
}

struct AZSChkRowEntry {
	long   chk;
	PPID   goods;
	double qtty;
	double price;
	char   barcode[24]; // @v8.8.0 [16]-->[24]
};

int SLAPI ACS_CRAZS::FlashCheck(CCheckTbl::Rec * pChkRec, SArray * pRows)
{
	int    ok = -1;
	PPID   id = 0;
	double sum = 0.0;
	AZSChkRowEntry * p_entry;
	uint i;
	if(pRows->getCount()) {
		LDATETIME dm; dm.Set(pChkRec->Dt, pChkRec->Tm);
		for(i = 0; pRows->enumItems(&i, (void**)&p_entry);)
			sum   += p_entry->qtty * p_entry->price;
		THROW(ok = AddTempCheck(&id, pChkRec->SessID, pChkRec->Flags, pChkRec->CashID,
			pChkRec->Code, pChkRec->UserID, pChkRec->SCardID, &dm, sum, 0));
		if(ok > 0) {
			for(i = 0; pRows->enumItems(&i, (void**)&p_entry);) {
				SetupTempCcLineRec(0, id, pChkRec->Code, dm.d, 0, p_entry->goods);
				SetTempCcLineValues(0, p_entry->qtty, p_entry->price, 0.0);
				THROW_DB(P_TmpCclTbl->insertRec());
			}
		}
	}
	pRows->freeAll();
	CATCHZOK
	return ok;
}

static char * SLAPI GetString(FILE * pFile, char * buf, size_t bufsize)
{
	size_t i = 0;
	if(buf) {
		for(char c = 0; (c = fgetc(pFile))!= '\n' && !feof(pFile) && i < bufsize - 1; i++)
			buf[i] = c;
		buf[i] = 0;
	}
	return buf;
}

int SLAPI ACS_CRAZS::ConvertWareList(PPID cashNo)
{
	int    ok = 1, ta = 0;
	uint   i;
	long   chk_no = 0;
	char   buf[256];
	LDATETIME last_chk_dttm;
	CCheckTbl::Rec  chk_rec;
	SArray rows(sizeof(AZSChkRowEntry));
	FILE * p_file = 0;

	last_chk_dttm.Set(MAXDATE, ZEROTIME);
	PPSetAddedMsgString(PathRpt);
	THROW_PP(p_file = fopen(PathRpt, onecstr('r')), PPERR_CANTOPENFILE);
	THROW(PPStartTransaction(&ta, 1));
	while(* GetString(p_file, buf, sizeof(buf))) {
		long   nsmena, chk_type, banking;
		PPID   goods_id;
		double qtty, price;
		LDATE  dt;
		LTIME  tm;
		char   barcode[32];
		AZSChkRowEntry row;
		StringSet ss('\t', 0);
		ss.add(buf);
		i = 0;
		ss.get(&i, buf, sizeof(buf));           // Дата
		strtodate(strip(buf), DATF_DMY, &dt);
		ss.get(&i, buf, sizeof(buf));           // Врем
		strtotime(strip(buf), TIMF_HMS, &tm);
		ss.get(&i, buf, sizeof(buf));           // Номер смены
		strtolong(buf, &nsmena);
		if(cmp(last_chk_dttm, dt, tm) != 0) {
			if(last_chk_dttm.d != MAXDATE) {
				THROW(FlashCheck(&chk_rec, &rows));
			}
			MEMSZERO(chk_rec);
			chk_rec.Code   = ++chk_no;
			chk_rec.CashID = cashNo;
			chk_rec.SessID = nsmena;
			chk_rec.Dt     = dt;
			chk_rec.Tm     = tm;
			last_chk_dttm.Set(dt, tm);
		}
		ss.get(&i, buf, sizeof(buf));           // Тип чека (продажа/возврат)
		strtolong(buf, &chk_type);
		ss.get(&i, buf, sizeof(buf));           // Код товара
		strtolong(buf, &goods_id);
		ss.get(&i, buf, sizeof(buf));           // Наименование товара (пропускаем)
		ss.get(&i, barcode, sizeof(barcode));   // Штрихкод
		ss.get(&i, buf, sizeof(buf));           // Наименование группы товаров (пропускаем)
		ss.get(&i, buf, sizeof(buf));           // Количество
		strtodoub(buf, &qtty);
		ss.get(&i, buf, sizeof(buf));           // Цена
		strtodoub(buf, &price);
		ss.get(&i, buf, sizeof(buf));           // Тип операции (продажа/возврат)
		strtolong(buf, &banking);
		if(banking != CRAZS_PAYMENT_CASH)
			chk_rec.Flags |= CCHKF_BANKING;
		if(chk_type == CRAZS_CHK_RETURN) {
			chk_rec.Flags |= CCHKF_RETURN;
			qtty = -fabs(qtty);
		}
		else
			qtty = fabs(qtty);
		MEMSZERO(row);
		row.chk   = chk_no;
		row.goods = goods_id;
		row.qtty  = qtty;
		row.price = fdiv100r(price);
		STRNSCPY(row.barcode, strip(barcode));
		THROW_SL(rows.insert(&row));
	}
	if(last_chk_dttm.d != MAXDATE) {
		THROW(FlashCheck(&chk_rec, &rows));
	}
	THROW(PPCommitWork(&ta));
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	SFile::ZClose(&p_file);
	return ok;
}

int SLAPI ACS_CRAZS::ImportSession(int)
{
	int    ok = -1;
	const  char empty = 0;
	SString exe_name;
	SString def_name;
	LDATE  oper_date, end = ChkRepPeriod.upp;
	if(!end)
		end = plusdate(LConfig.OperDate, 2);
	THROW(PPGetFileName(PPFILNAM_CRAZS_EXP_EXE, exe_name));
	THROW(PPGetFileName(PPFILNAM_CRAZS_EXP_DEF, def_name));
	SPathStruc::ReplacePath(exe_name, PathImp, 1);
	SPathStruc::ReplacePath(def_name, PathImp, 1);
	PPSetAddedMsgString(exe_name);
	THROW_PP(fileExists(exe_name), PPERR_CRAZS_IMPCHECKS);
	SPathStruc::ReplacePath(exe_name, &empty, 1);
	PPSetAddedMsgString(def_name);
	THROW_PP(fileExists(def_name), PPERR_CRAZS_IMPCHECKS);
	SPathStruc::ReplacePath(def_name, &empty, 1);
	THROW(CreateTables());
	for(oper_date = ChkRepPeriod.low; oper_date <= end; oper_date = plusdate(oper_date, 1)) {
		int   y, m, d;
		uint  pos;
		char  buf[8], prot_name[16];
		memzero(prot_name, sizeof(prot_name));
		if(fileExists(PathRpt))
			SFile::Remove(PathRpt);
		decodedate(&d, &m, &y, &oper_date);
		intfmt(d, 0, buf);
		if(strlen(buf) < 2)
			padleft(buf, '0', 2 - strlen(buf));
		strcpy(prot_name, buf);
		THROW(PPGetSubStr(PPTXT_MONTHES_3SYM_ENGLISH, m - 1, prot_name + 2, 4));
		intfmt(y % 100, 0, buf);
		if(strlen(buf) < 2)
			padleft(buf, '0', 2 - strlen(buf));
		strcpy(prot_name + 5, buf);
		for(pos = 0; pos < LogNumList.getCount(); pos++) {
			int c;
			intfmt(LogNumList.at(pos), 0, buf);
			if(strlen(buf) < 2)
				padleft(buf, '0', 2 - strlen(buf));
			prot_name[7] = buf[0];
			prot_name[8] = '.';
			prot_name[9] = buf[1];
			for(c = 99; c > 0; c--) {
				char prot_path[MAXPATH];
				intfmt(c, 0, buf);
				if(strlen(buf) < 2)
					padleft(buf, '0', 2 - strlen(buf));
				strcpy(prot_name + 10, buf);
				strcpy(prot_path, prot_name);
				replacePath(prot_path, PathImp, 1);
				if(fileExists(prot_path))
					break;
			}
			if(c > 0) {
				char  cur_dir[MAXPATH];
				GetCurrentDirectory(sizeof(cur_dir), cur_dir);
				::SetCurrentDirectory(PathImp); // @unicodeproblem
				spawnl(_P_WAIT, exe_name, exe_name, def_name, prot_name, NULL);
				::SetCurrentDirectory(cur_dir); // @unicodeproblem
				PPSetAddedMsgString(PathRpt);
				THROW_PP(fileExists(PathRpt), PPERR_CRAZS_IMPCHECKS);
				THROW(ConvertWareList(LogNumList.at(pos)));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

