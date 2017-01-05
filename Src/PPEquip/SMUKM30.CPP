// SMUKM30.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2007, 2008, 2010, 2011, 2013, 2015, 2016
// @codepage windows-1251
//
// Интерфейс (асинхронный) к драйверу ККМ "СуперМаг УКМ" (IPC)
//
#include <pp.h>
#pragma hdrstop

class ACS_SMUKM30 : public CS_1 {
public:
	SLAPI  ACS_SMUKM30(PPID n) : CS_1(n) {}
	virtual int SLAPI ExportData(int updOnly);
	virtual int SLAPI GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int SLAPI ImportSession(int);
private:
	int    SLAPI ConvertWareList(int);
	int    SLAPI FlashCheck(CCheckTbl::Rec * chk, SArray * rows);
	enum {
		warelist = 0
	};
};

class CM_SMUKM30 : public PPCashMachine {
public:
	SLAPI CM_SMUKM30(PPID cashID) : PPCashMachine(cashID) {}
	PPAsyncCashSession * SLAPI AsyncInterface() { return new ACS_SMUKM30(NodeID); }
};

REGISTER_CMT(SMUKM30,0,1);

int SLAPI ACS_SMUKM30::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd /*=0*/)
{
	int    r = GetFileSet(0, 1);
	*pSessCount = r ? NumEntries : 0;
	*pIsForwardSess = 0;
	return r;
}

struct RowEntry {
	long   chk;
	long   div;
	PPID   goods;
	char   barcode[24]; // @v8.8.0 [16]-->[24]
	double qtty;
	double price;
	double dscnt;
};

int SLAPI ACS_SMUKM30::FlashCheck(CCheckTbl::Rec * pChkRec, SArray * pRows)
{
	int    ok = 1, r;
	PPID   id = 0;
	double sum = 0, dscnt = 0;
	RowEntry * p_entry;
	uint i;
	if(pRows->getCount()) {
		LDATETIME dm; dm.Set(pChkRec->Dt, pChkRec->Tm);
		for(i = 0; pRows->enumItems(&i, (void**)&p_entry);) {
			sum += p_entry->qtty * (p_entry->price + p_entry->dscnt);
			dscnt += p_entry->qtty * p_entry->dscnt;
		}
		THROW(r = AddTempCheck(&id, pChkRec->SessID, 0, pChkRec->CashID,
			pChkRec->Code, pChkRec->UserID, 0 /* cardID */, &dm, sum, dscnt));
		if(r > 0) {
			for(i = 0; pRows->enumItems(&i, (void**)&p_entry);) {
				SetupTempCcLineRec(0, id, pChkRec->Code, dm.d, p_entry->div, p_entry->goods);
				SetTempCcLineValues(0, p_entry->qtty, p_entry->price, p_entry->dscnt);
				STRNSCPY(P_TmpCclTbl->data.BarCode, p_entry->barcode);
				THROW_DB(P_TmpCclTbl->insertRec());
			}
		}
	}
	pRows->freeAll();
	CATCHZOK
	return ok;
}

int SLAPI ACS_SMUKM30::ConvertWareList(int num)
{
	int    ok = 1;
	long   prev_chk = -1;
	DbfTable * p_dbft = OpenDBFTable(num, warelist);
	RowEntry row;
	CCheckTbl::Rec chk_rec;
	SArray rows(sizeof(RowEntry));
	THROW(p_dbft);
	{
		PPTransaction tra(1);
		THROW(tra);
		if(p_dbft->top()) {
			do {
				int    sign;
				PPID   goods_id;
				long   csh, cshr, chk, div = 0, z;
				int    itime;
				LDATE  dt;
				LTIME  tm;
				double qtty, price;
				DbfRecord dbfr(p_dbft);
				THROW(p_dbft->getRec(&dbfr));
				dbfr.get( 2, csh);
				dbfr.get( 3, z);
				dbfr.get( 4, chk);
				dbfr.get( 6, dt);
				dbfr.get( 7, itime);
				dbfr.get(16, cshr);
				tm = encodetime(itime / 100, itime % 100, (int)(chk % 60), 0);
				chk = z * 100000L + chk;

				dbfr.get( 8, goods_id);
				dbfr.get(10, qtty);
				dbfr.get(11, price);
				dbfr.get(18, sign);
				if(sign == 0)
					qtty = -qtty;
				if(chk != prev_chk) {
					if(prev_chk > 0)
						THROW(FlashCheck(&chk_rec, &rows));
   					prev_chk = chk;
					MEMSZERO(chk_rec);
					chk_rec.Code   = chk;
					chk_rec.CashID = csh;
					chk_rec.SessID = z;
					chk_rec.Dt     = dt;
					chk_rec.Tm     = tm;
				}
				if(goods_id) {
					MEMSZERO(row);
					row.chk   = chk;
					row.div   = div;
					row.goods = goods_id;
					row.qtty  = qtty;
					row.price = price;
					row.dscnt = 0;
					THROW_SL(rows.insert(&row));
				}
			} while(p_dbft->next());
			if(prev_chk > 0)
				THROW(FlashCheck(&chk_rec, &rows));
		}
		else
			ok = -1;
		THROW(tra.Commit());
	}
	CATCHZOK
	delete p_dbft;
	return ok;
}

int SLAPI ACS_SMUKM30::ImportSession(int num)
{
	int ok = -1, r;
	if(fileExists(P_Entries[num].fn[warelist])) {
		THROW(CreateTables());
		THROW(r = ConvertWareList(num));
		if(r > 0)
			ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI ACS_SMUKM30::ExportData(int updOnly)
{
	int        ok = 1;
	int        add_chkdig = 0;
	int        yr, mn, dy, hr, mi, se, ts;
	DBFDate    dbdt;
	int        dbtm;
	DbfTable * p_out_tbl_goods = 0, * p_out_tbl_barcode = 0;
	//char     path_goods[MAXPATH], path_barcode[MAXPATH], path[MAXPATH];
	SString    path_goods;
	SString    path_barcode;
	SString    path;
	uint       tag_fname_id = 0;
	PPGoodsConfig goods_cfg;
	PPUnit     unit_rec;
	PPObjUnit  unit_obj;
	PPID       prev_goods_id = 0;
	LDATETIME  curdtm = getcurdatetime_();
	PPObjCashNode cn_obj;
	PPAsyncCashNode cn_data;
	AsyncCashGoodsInfo       info;
	AsyncCashGoodsIterator * p_iter = 0;

	decodedate(&dy, &mn, &yr, &curdtm.d);
	decodetime(&hr, &mi, &se, &ts, &curdtm.t);
	dbdt.year  = yr;
	dbdt.month = mn;
	dbdt.day   = dy;
	dbtm = hr * 100 + mi;

	PPWait(1);
	THROW(cn_obj.GetAsync(NodeID, &cn_data) > 0);
	THROW(PPObjGoods::ReadConfig(&goods_cfg));
	const size_t check_dig  = (goods_cfg.Flags & GCF_BCCHKDIG) ? 1 : 0;
	add_chkdig = (cn_data.Flags & CASHF_EXPCHECKD && !check_dig) ? 1 : 0;

	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_PLUCASH_DBF, path_goods));
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_BAR_DBF, path_barcode));
	if(cn_data.DrvVerMajor == 2 && cn_data.DrvVerMinor == 420)
		tag_fname_id = updOnly ? PPFILNAM_CASH_CNG : PPFILNAM_CASH_UPD;
	else
		tag_fname_id = updOnly ? PPFILNAM_CASH_UPD : PPFILNAM_CASH_CNG;
	THROW(PPGetFilePath(PPPATH_OUT, tag_fname_id, path));
	{
		FILE * ff = fopen(path, onecstr('w'));
		SFile::ZClose(&ff);
	}
	THROW(p_out_tbl_goods = CreateDbfTable(DBFS_SMUKM_GOODS_EXPORT, path_goods, 1));
	THROW(p_out_tbl_barcode = CreateDbfTable(DBFS_SMUKM_BAR_EXPORT, path_barcode, 1));
	THROW_MEM(p_iter = new AsyncCashGoodsIterator(NodeID, (updOnly ? ACGIF_UPDATEDONLY : 0), SinceDlsID, 0));
	{
		PPTransaction tra(1);
		THROW(tra);
		while(p_iter->Next(&info) > 0) {
			char   tempbuf[80];
			size_t bclen;
	   		if(info.ID != prev_goods_id) {
				DbfRecord dbfrG(p_out_tbl_goods);
				dbfrG.empty();
				dbfrG.put(1,  ltoa(info.ID, tempbuf, 10));
				dbfrG.put(2,  info.Name);
		   		unit_obj.Fetch(info.UnitID, &unit_rec);
				dbfrG.put(3,  unit_rec.Name);
				dbfrG.put(4,  fpow10i(-3));
				dbfrG.put(11, "NOSIZE");
				dbfrG.put(12, (int)0); // Группа 1
				dbfrG.put(13, (int)0); // Группа 2
				dbfrG.put(14, (int)0); // Группа 3
				dbfrG.put(15, (int)0); // Группа 4
				dbfrG.put(16, (int)0); // Группа 5
				dbfrG.put(17, info.Price);
				dbfrG.put(21, 1);
				dbfrG.put(22, &dbdt);
				dbfrG.put(23, dbtm);
				THROW_PP(p_out_tbl_goods->appendRec(&dbfrG), PPERR_DBFWRFAULT);
			}
			bclen = strlen(info.BarCode);
			if(bclen) {
				DbfRecord dbfrB(p_out_tbl_barcode);
				dbfrB.empty();
				if(bclen != 3 && bclen != (7+check_dig) && bclen < (12+check_dig))
					padleft(info.BarCode, '0', (12+check_dig) - bclen);
				size_t l = strlen(info.BarCode);
				if(add_chkdig && l > 3 && !goods_cfg.IsWghtPrefix(info.BarCode))
					AddBarcodeCheckDigit(info.BarCode);
				dbfrB.put(1, info.BarCode);
				dbfrB.put(2, ltoa(info.ID, tempbuf, 10));
				dbfrB.put(3, "NOSIZE");
				dbfrB.put(4, info.UnitPerPack);
				THROW_PP(p_out_tbl_barcode->appendRec(&dbfrB), PPERR_DBFWRFAULT);
			}
	   		prev_goods_id = info.ID;
			PPWaitPercent(p_iter->GetIterCounter());
		}
		THROW(tra.Commit());
	}
	ZDELETE(p_out_tbl_barcode);
	ZDELETE(p_out_tbl_goods);
	THROW(DistributeFile(path_goods, 0));
	THROW(DistributeFile(path_barcode, 0));
	THROW(DistributeFile(path,  0));
	if(cn_data.DrvVerMajor == 2 && cn_data.DrvVerMinor == 420)
		tag_fname_id = updOnly ? PPFILNAM_CASH_UPD : PPFILNAM_CASH_CNG;
	else
		tag_fname_id = updOnly ? PPFILNAM_CASH_CNG : PPFILNAM_CASH_UPD;
	THROW(PPGetFilePath(PPPATH_OUT, tag_fname_id, path));
	THROW(DistributeFile(path,  1));
	PPWait(0);
	CATCHZOK
	delete p_out_tbl_barcode;
	delete p_out_tbl_goods;
	return ok;
}
