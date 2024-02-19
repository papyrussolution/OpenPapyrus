// EL92AQ.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2006, 2010, 2011, 2015, 2016, 2019, 2022, 2024
// @codepage windows-1251
// Интерфейс (асинхронный) с ККМ Електроника-92-Аквариус
//
#include <pp.h>
#pragma hdrstop

class ACS_EL92AQ : public CS_1 {
public:
	ACS_EL92AQ(PPID n);
	virtual int ExportData(int);
	virtual int GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int ImportSession(int);
private:
	int    ConvertCashList(int);
	int    ConvertWareList(int);

	int    Amounts100;
	enum {
		warebase = 0,
		cashlist,
		warelist
	};
};

ACS_EL92AQ::ACS_EL92AQ(PPID n) : CS_1(n)
{
	PPCashNode node;
	if(SearchObject(PPOBJ_CASHNODE, NodeID, &node) > 0)
		if(node.DrvVerMajor < 2 || (node.DrvVerMajor == 2 && node.DrvVerMinor < 87))
			Amounts100 = 0;
		else
			Amounts100 = 1;
	else
		Amounts100 = 0;
}

class CM_EL92AQ : public PPCashMachine {
public:
	CM_EL92AQ(PPID cashID) : PPCashMachine(cashID) {}
	PPAsyncCashSession * AsyncInterface() { return new ACS_EL92AQ(NodeID); }
};

REGISTER_CMT(EL92AQ, false, true);

int ACS_EL92AQ::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd /*=0*/)
{
	int r = GetFileSet(0, 3);
	*pSessCount = r ? NumEntries : 0;
	*pIsForwardSess = 1;
	return r;
}

int TranslateStringToDbfFields(DbfTable * pTbl, uint strID, int * pFldIdxList, int fldIdxListLen)
{
	int    i = 0, valid_count = 0;
	SString temp_buf, sub;
	if(PPLoadText(strID, temp_buf)) {
		StringSet ss(';', temp_buf);
		for(uint pos = 0; ss.get(&pos, sub);)
			if(i < fldIdxListLen) {
				int    fld_no = 0;
				pTbl->getFieldNumber(sub, &fld_no);
				pFldIdxList[i++] = fld_no;
				if(fld_no > 0)
					valid_count++;
			}
		return valid_count;
	}
	else
		return 0;
}

int ACS_EL92AQ::ConvertCashList(int num)
{
	int         ok = 1, ta = 0;
	DbfTable  * dbft = 0;
	int fld_idx_list[32];

	THROW(dbft = OpenDBFTable(num, cashlist));
	THROW(PPStartTransaction(&ta, 1));
	THROW(TranslateStringToDbfFields(dbft, PPTXT_EL92AQCASHLIST, fld_idx_list, 32));
	if(dbft->top()) {
		do {
			long   csh, chk, usr, opr;
			PPID   check_id = 0;
			LDATETIME dttm;
			double a, amt = 0.0;
			DbfRecord dbfr(dbft);
			THROW(dbft->getRec(&dbfr));
			dbfr.get(fld_idx_list[0], dttm.d);
			dbfr.get(fld_idx_list[1], dttm.t);
			dbfr.get(fld_idx_list[2], csh);
			dbfr.get(fld_idx_list[3], chk);
			dbfr.get(fld_idx_list[4], usr);
			dbfr.get(fld_idx_list[5], opr);
			dbfr.get(fld_idx_list[6], a);
			dbfr.get(fld_idx_list[10], amt);
			amt += a;
			dbfr.get(fld_idx_list[7], a);
			amt += a;
			dbfr.get(fld_idx_list[8], a);
			amt += a;
			dbfr.get(fld_idx_list[9], a);
			amt += a;
			amt = R2(fdiv100r(amt));
			if(opr == 0) {
				THROW(AddTempCheck(&check_id, 0, 0, csh, chk, usr, 0, dttm, amt, 0));
			}
			else if(opr == 2) {
				THROW(AddTempCheck(&check_id, chk, CCHKF_ZCHECK, csh, chk, usr, 0, dttm, amt, 0));
			}
		} while(dbft->next());
	}
	THROW(PPCommitWork(&ta));
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	delete dbft;
	return ok;
}

int ACS_EL92AQ::ConvertWareList(int num)
{
	int    ok = 1, ta = 0, r;
	int    fld_idx_list[32];
	DbfTable * p_in_tbl = OpenDBFTable(num, warelist);
	BarcodeTbl    * p_bc_tbl = 0;
	BarcodeTbl::Rec bc_rec;
	BExtInsert      bei(P_TmpCclTbl);
	THROW(p_in_tbl);
	THROW(p_bc_tbl = CreateTmpBarToID(num, warebase, 7, 4, 8));
	THROW(PPStartTransaction(&ta, 1));
	THROW(TranslateStringToDbfFields(p_in_tbl, PPTXT_EL92AQWARELIST, fld_idx_list, 32));
	if(p_in_tbl->top())
		do {
			long   _bank, _check, _divnum;
			char   _gcode[24];
			double _qtty, _rate, _price, _extra;
			DbfRecord dbfr(p_in_tbl);
			THROW(p_in_tbl->getRec(&dbfr));
			dbfr.get(fld_idx_list[0], _gcode, sizeof(_gcode));
			dbfr.get(fld_idx_list[1], _price);
			dbfr.get(fld_idx_list[2], _divnum);
			dbfr.get(fld_idx_list[3], _bank);
			dbfr.get(fld_idx_list[4], _check);
			dbfr.get(fld_idx_list[5], _qtty);
			dbfr.get(fld_idx_list[6], _rate);
			dbfr.get(fld_idx_list[7], _extra);
			_price /= 100L;
			_extra /= 100L;
			THROW(r = SearchTempCheckByCode(_bank, _check));
			if(r > 0) {
				char   bck[24], * c;
				memzero(bck, sizeof(bck));
				c = _gcode;
				while(*c == '0' || *c == ' ')
					c++;
				strip(strcpy(bck, c));
				if(bck[0]) {
					TempCCheckLineTbl::Rec rec;
					THROW_DB(p_bc_tbl->search(0, &bck, spEq) || BTRNFOUND);
					THROW_PP(!BTRNFOUND, PPERR_BARCODENFOUND);
					p_bc_tbl->copyBufTo(&bc_rec);
					rec.CheckID   = P_TmpCcTbl->data.ID;
					rec.CheckCode = P_TmpCcTbl->data.Code;
					rec.Dt        = P_TmpCcTbl->data.Dt;
					rec.DivID     = (int16)_divnum;
					rec.GoodsID   = bc_rec.GoodsID;
					if(bc_rec.Qtty != 0 && bc_rec.Qtty != 1)
						_qtty *= bc_rec.Qtty;
					rec.Quantity  = _qtty;
					STRNSCPY(rec.BarCode, _gcode);
					if(bc_rec.Qtty != 0 && bc_rec.Qtty != 1)
						_price /= bc_rec.Qtty;
					rec.Price = dbltointmny(_price);
					if(_qtty != 0) {
						if(_extra != 0)
							_extra = -_extra;
						rec.Dscnt = _extra / fabs(_qtty);
					}
					THROW_DB(bei.insert(&rec));
				}
			}
		} while(p_in_tbl->next());
	THROW_DB(bei.flash());
	THROW(PPCommitWork(&ta));
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	delete p_in_tbl;
	delete p_bc_tbl;
	return ok;
}

int ACS_EL92AQ::ImportSession(int num)
{
	int    ok = 1;
	THROW(CreateTables());
	THROW(ConvertCashList(num));
	THROW(ConvertWareList(num));
	CATCHZOK
	return ok;
}

int ACS_EL92AQ::ExportData(int)
{
	int    ok = 1;
	DbfTable * p_out_tbl = 0;
	SString path;
	AsyncCashGoodsInfo       info;
	AsyncCashGoodsIterator * p_iter = 0;

	PPWaitStart();
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_WAREBASE_DBF, path));
	THROW(p_out_tbl = CreateDbfTable(DBFS_MNEXPORT, path, 1));
	THROW_MEM(p_iter = new AsyncCashGoodsIterator(NodeID, 0, SinceDlsID, 0));
	while(p_iter->Next(&info) > 0) {
		char   intg = 'F';
		double price = (info.Price * info.UnitPerPack);
		//if(Amounts100 == 0)
			price *= 100L;
		DbfRecord dbfr(p_out_tbl);
		dbfr.put(1, info.Name);
		dbfr.put(2, price);
		dbfr.put(3, 1000.0);
		dbfr.put(4, satof(info.BarCode)); // @v10.7.9 atof-->satof
		dbfr.put(5, 1L); // Номер отдела
		dbfr.put(6, &intg);  // Штучный товар
		dbfr.put(7, info.ID);
		dbfr.put(8, info.UnitPerPack);
		THROW_PP(p_out_tbl->appendRec(&dbfr), PPERR_DBFWRFAULT);
		PPWaitPercent(p_iter->GetIterCounter());
	}
	PPWaitStop();
	CATCHZOK
	delete p_iter;
	delete p_out_tbl;
	return ok;
}
