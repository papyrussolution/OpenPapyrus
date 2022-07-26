// ECR930.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2007, 2008, 2010, 2011, 2015, 2016, 2019, 2020, 2022
// @codepage windows-1251
// Интерфейс (асинхронный) с ККМ ЭКР-4110
//
// Since @v3.2.0 under construction
//
#include <pp.h>
#pragma hdrstop

class ACS_ECR930 : public CS_1 {
public:
	ACS_ECR930(PPID n) : CS_1(n) {}
	virtual int ExportData(int);
	virtual int GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int ImportSession(int);
private:
	int    ConvertWareList(int);
	int    FlashCheck(CCheckTbl::Rec *, SArray *);
	enum {
		warebase = 0,
		warelist
	};
};

class CM_ECR930 : public PPCashMachine {
public:
	CM_ECR930(PPID cashID) : PPCashMachine(cashID) {}
	PPAsyncCashSession * AsyncInterface() { return new ACS_ECR930(NodeID); }
};

REGISTER_CMT(ECR930, false, true);

int ACS_ECR930::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd /*=0*/)
{
	int r = GetFileSet(0, 2);
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

int ACS_ECR930::FlashCheck(CCheckTbl::Rec * chk, SArray * rows)
{
	int    ok = 1, r;
	int    h, m, s, ts;
	PPID   id = 0;
	double sum = 0, dscnt = 0;
	RowEntry * e;
	uint   i;
	if(rows->getCount()) {
		for(i = 0; rows->enumItems(&i, (void **)&e);) {
			sum += e->qtty * (e->price + e->dscnt);
			dscnt += e->qtty * e->dscnt;
		}
		//
		// Этот кассовый аппарат прописывает в поле времени только
		// часы и минуты, что не позволяет уникально идентифицировать
		// чек по дате и времени. Проставляем секунды как остаток от
		// деления номера чека на 60.
		//
		LDATETIME dm; dm.Set(chk->Dt, chk->Tm);
		decodetime(&h, &m, &s, &ts, &dm.t);
		dm.t = encodetime(h, m, (int)(chk->Code % 60), 0);
		THROW(r = AddTempCheck(&id, chk->SessID, 0, chk->CashID, chk->Code, chk->UserID, 0 /* cardID */, dm, sum, dscnt));
		if(r > 0) {
			for(i = 0; rows->enumItems(&i, (void **)&e);) {
				SetupTempCcLineRec(0, id, chk->Code, dm.d, e->div, e->goods);
				// @v10.7.3 SetTempCcLineValues(0, e->qtty, e->price, e->dscnt, 0/*pLnExtStrings*/);
				STRNSCPY(P_TmpCclTbl->data.BarCode, e->barcode);
				// @v10.7.3 THROW_DB(P_TmpCclTbl->insertRec());
				THROW(SetTempCcLineValuesAndInsert(P_TmpCclTbl, e->qtty, e->price, e->dscnt, 0/*pLnExtStrings*/)); // @v10.7.3
			}
		}
	}
	rows->freeAll();
	CATCHZOK
	return ok;
}

int ACS_ECR930::ConvertWareList(int num)
{
	int    ok = 1;
	long   prevChk = -1;
	DbfTable * dbft = OpenDBFTable(num, warelist);
	BarcodeTbl    * bct = 0;
	BarcodeTbl::Rec bcr;
	RowEntry        row;
	CCheckTbl::Rec  checkr;
	SArray          rows(sizeof(RowEntry));
	THROW(dbft);
	THROW(bct = CreateTmpBarToID(num, warebase, 10, 1, 11));
	{
		PPTransaction tra(1);
		THROW(tra);
		if(dbft->top())
			do {
				long   cshr, chk, div, z;
				LDATE  dt;
				LTIME  tm;
				char   gcode[24], bck[24], * c;
				double qtty, price, dscnt;
				DbfRecord dbfr(dbft);
				THROW(dbft->getRec(&dbfr));
				dbfr.get( 1, cshr);
				dbfr.get( 2, z); // ???
				dbfr.get( 3, chk);
				dbfr.get( 4, div);
				dbfr.get( 5, dt);
				dbfr.get( 6, tm);
				dbfr.get( 9, gcode, sizeof(gcode));
				dbfr.get(10, qtty);
				dbfr.get(11, price);
				dbfr.get(12, dscnt);
				if(chk != prevChk) {
					if(prevChk > 0) {
						THROW(FlashCheck(&checkr, &rows));
					}
					prevChk = chk;
					MEMSZERO(checkr);
					checkr.Code = chk;
					checkr.CashID = 1; // ???
					checkr.SessID = z; // ???
					checkr.Dt = dt;
					checkr.Tm = tm;
				}
				memzero(bck, sizeof(bck));
				c = gcode;
				while(*c == '0' || *c == ' ')
					c++;
				strip(strcpy(bck, c));
				if(bck[0]) {
					int    r = SearchByKey(bct, 0, bck, &bcr);
					THROW(r);
					THROW_PP(r > 0, PPERR_BARCODENFOUND);
					MEMSZERO(row);
					row.chk   = chk;
					row.div   = div;
					row.goods = bcr.GoodsID;
					STRNSCPY(row.barcode, gcode);
					row.qtty  = qtty;
					row.price = (bcr.Qtty != 0) ? (price / bcr.Qtty) : price;
					row.dscnt = dscnt;
					THROW_SL(rows.insert(&row));
				}
			} while(dbft->next());
		if(prevChk > 0) {
			THROW(FlashCheck(&checkr, &rows));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	delete dbft;
	delete bct;
	return ok;
}

int ACS_ECR930::ImportSession(int num)
{
	int    ok = 1;
	THROW(CreateTables());
	THROW(ConvertWareList(num));
	CATCHZOK
	return ok;
}

int ACS_ECR930::ExportData(int)
{
	int    ok = 1;
	DbfTable * p_out_tbl = 0;
	SString path;
	AsyncCashGoodsInfo info;
	AsyncCashGoodsIterator * p_iter = 0;
	PPWaitStart();
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_GOODS_DBF, path));
	THROW(p_out_tbl = CreateDbfTable(DBFS_ECREXPORT, path, 1));
	THROW_MEM(p_iter = new AsyncCashGoodsIterator(NodeID, 0, SinceDlsID, 0));
	while(p_iter->Next(&info) > 0) {
		int intg = 0;
		DbfRecord dbfr(p_out_tbl);
		dbfr.put( 1, satof(info.BarCode)); // @v10.7.9 atof-->satof
		dbfr.put( 2, info.Name);
		dbfr.put( 3, (info.Price * info.UnitPerPack));
		dbfr.put( 7, intg);    // Штучный товар
		dbfr.put(10, info.ID); // ИД товара
		dbfr.put(11, info.UnitPerPack);
		THROW_PP(p_out_tbl->appendRec(&dbfr), PPERR_DBFWRFAULT);
	}
	PPWaitStop();
	CATCHZOK
	delete p_out_tbl;
	delete p_iter;
	return ok;
}
