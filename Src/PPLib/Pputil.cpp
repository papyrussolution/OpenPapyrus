// PPUTIL.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>
#include <AccCtrl.h>
#include <AclAPI.h>

DBFCreateFld * LoadDBFStruct(uint rezID, uint * pNumFlds); // @prototype

int FASTCALL dbl_cmp(double v1, double v2) { return CMPSIGN(R6(v1 - v2), 0.0); }
IMPL_CMPFUNC(PPLBItem, i1, i2) { return stricmp866(static_cast<const char *>(i1)+sizeof(long), static_cast<const char *>(i2)+sizeof(long)); }

IMPL_CMPFUNC(PPTLBItem, i1, i2)
{
	const  PPID parent_id1 = *static_cast<const long *>(i1);
	const  PPID parent_id2 = *static_cast<const long *>(i2);
	if(parent_id1 > parent_id2)
		return 1;
	else if(parent_id1 < parent_id2)
		return -1;
	int    cmp = stricmp866(static_cast<const char *>(i1)+sizeof(long)*2, static_cast<const char *>(i2)+sizeof(long)*2);
	if(cmp > 0)
		return 1;
	else if(cmp < 0)
		return -1;
	return 0;
}

long STDCALL CheckXORFlags(long v, long f1, long f2) { return ((v & f1) ^ (v & f2)) ? ((v & f1) ? f1 : f2) : 0; }
long STDCALL SetXORFlags(long v, long f1, long f2, long f) { return ((v & ~(f1 | f2)) | f); }

int FASTCALL PPInitIterCounter(IterCounter & rCntr, DBTable * pTbl)
{
	RECORDNUMBER num_recs = 0;
	return (!pTbl || pTbl->getNumRecs(&num_recs)) ? (rCntr.Init(num_recs), 1) : PPSetErrorDB();
}

SString & DateToStr(LDATE dt, SString & rBuf)
{
	rBuf.Z();
	if(dt) {
		SString txt_month;
		SGetMonthText(dt.month(), MONF_CASEGEN, txt_month);
		SString & r_yr_buf = SLS.AcquireRvlStr();
		(r_yr_buf = "г.").Transf(CTRANSF_UTF8_TO_INNER);
		rBuf.CatLongZ(dt.day(), 2).Space().Cat(txt_month).Space().Cat(dt.year()).Space().Cat(r_yr_buf);
		rBuf.Transf(CTRANSF_OUTER_TO_INNER);
	}
	return rBuf;
}

SString & MoneyToStr(double nmb, long fmt, SString & rBuf)
{
	char   temp[128];
	int    word_idx = 0;
	SString word;
	double ipart;
	long   fraction = R0i(modf(R2(nmb), &ipart) * 100L);
	rBuf = numbertotext(ipart, fmt, temp);
	if(fraction >= 5 && fraction <= 20)
		word_idx = 0;
	else if((fraction % 10) == 1)
		word_idx = 1;
	else if((fraction % 10) > 1 && (fraction % 10) < 5)
		word_idx = 2;
	else
		word_idx = 0;
	PPGetSubStr(PPTXT_WORDS_MONEY_CENT, word_idx, word); //"копеек;копейка;копейки"
	return rBuf.Space().CatLongZ(fraction, 2).Space().Cat(word);
}

double FASTCALL SalesTaxMult(double rate) { return (rate / (100.0 + rate)); }
double FASTCALL CalcVATRate(double base, double vat_sum) { return fdivnz(100.0 * vat_sum, base - vat_sum); }
SString & VatRateStr(double rate, SString & rBuf) { return PPLoadStringS("vat", rBuf).Space().Cat(R0i(rate)).CatChar('%'); }
int    PPGetSubStr(const char * pStr, int idx, SString & rDest) { return rDest.GetSubFrom(pStr, ';', idx); }

int PPGetSubStr(const char * pStr, int idx, char * pBuf, size_t bufLen)
{
	SString & r_temp_buf = SLS.AcquireRvlStr();
	int    ok = r_temp_buf.GetSubFrom(pStr, ';', idx);
	r_temp_buf.CopyTo(pBuf, bufLen);
	return ok;
}

int PPGetSubStrById(int strId, int subId, SString & rBuf)
{
	int    ok = 0;
	rBuf.Z();
	SString line_buf;
	if(PPLoadText(strId, line_buf)) {
		SString item_buf, id_buf, txt_buf;
		for(int idx = 0; !ok && PPGetSubStr(line_buf, idx, item_buf) > 0; idx++) {
			long   id = 0;
			if(item_buf.Divide(',', id_buf, txt_buf) > 0)
				id = id_buf.ToLong();
			else {
				id = (idx+1);
				txt_buf = item_buf;
			}
			if(id == subId) {
				rBuf = txt_buf;
				ok = 1;
			}
		}
	}
	return ok;
}

static int FASTCALL __CompareStrings(const char * pStr, const char * pTest, size_t maxCmpLen, int ignoreCase)
{
	if(maxCmpLen) {
		if(ignoreCase) {
			if(strnicmp866(pStr, pTest, maxCmpLen) == 0)
				return 1;
		}
		else if(strncmp(pStr, pTest, maxCmpLen) == 0)
			return 1;
	}
	else if(ignoreCase) {
		if(stricmp866(pStr, pTest) == 0)
			return 1;
	}
	else if(strcmp(pStr, pTest) == 0)
		return 1;
	return 0;
}

int PPSearchSubStr(const char * pStr, int * pIdx, const char * pTestStr, int ignoreCase)
{
	int    idx = -1;
	uint   pos = 0;
	SString temp_buf;
	StringSet ss(';', pStr);
	for(int i = 0; idx < 0 && ss.get(&pos, temp_buf); i++)
		if(__CompareStrings(temp_buf, pTestStr, 0, ignoreCase))
			idx = i;
	ASSIGN_PTR(pIdx, idx);
	return (idx < 0) ? 0 : 1;
}

int PPCmpSubStr(const char * pStr, int idx, const char * pTestStr, int ignoreCase)
{
	SString temp;
	if(PPGetSubStr(pStr, idx, temp))
		if(__CompareStrings(temp, pTestStr, 0, ignoreCase))
			return 1;
	return 0;
}

int PPGetSubStr(uint strID, int idx, SString & rDest)
{
	rDest.Z();
	SString temp;
	return PPLoadText(strID, temp) ? PPGetSubStr(temp, idx, rDest) : 0;
}

int PPGetSubStr(uint strID, int idx, char * pBuf, size_t bufLen)
{
	SString temp;
	int    ok = PPLoadText(strID, temp) ? PPGetSubStr(temp, idx, pBuf, bufLen) : 0;
	if(!ok && pBuf)
		pBuf[0] = 0;
	return ok;
}

char * PPGetWord(uint wordId /* PPWORD_XXX */, int ansiCoding, char * pBuf, size_t bufLen)
{
	SString temp_buf;
	PPLoadText(wordId, temp_buf);
	if(ansiCoding)
		temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
	temp_buf.CopyTo(pBuf, bufLen);
	return pBuf;
}

SString & PPGetWord(uint wordId /* PPWORD_XXX */, int ansiCoding, SString & rBuf)
{
	PPLoadText(wordId, rBuf);
	if(ansiCoding)
		rBuf.Transf(CTRANSF_INNER_TO_OUTER);
	return rBuf;
}

SString & FASTCALL ideqvalstr(long id, SString & rBuf) { return rBuf.CatEq("ID", id); }
char * STDCALL ideqvalstr(long id, char * pBuf, size_t bufLen) { return ideqvalstr(id, SLS.AcquireRvlStr()).CopyTo(pBuf, bufLen); }
//
//
//
OptimalAmountDamper::OptimalAmountDamper() : Pos(-1), ExtPos(-1), OptQtty(0.0), MaxPrice(0.0)
{
}

int OptimalAmountDamper::GetOptimal(long & rPos, long * pExtPos, double * pOptQtty, double * pMaxPrice) const
{
	rPos = Pos;
	if(Pos >= 0) {
		ASSIGN_PTR(pExtPos, ExtPos);
		ASSIGN_PTR(pOptQtty, OptQtty);
		ASSIGN_PTR(pMaxPrice, MaxPrice);
		return 1;
	}
	else
		return 0;
}

int OptimalAmountDamper::Probe(double qtty, double price, long pos, long extPos)
{
	int    ok = -1;
	if(qtty > 0.0 && price > 0.0) {
		if(Pos < 0) {
			ok = 1;
		}
		else if(qtty != OptQtty || price != MaxPrice) {
			if(qtty == OptQtty) {
				if(price > MaxPrice)
					ok = 1;
			}
			else {
				double qf = ffrac(qtty);
				double oqf = ffrac(OptQtty);
				if(qf == 0.0 && oqf != 0.0) {
					ok = 1;
				}
				else if(qf == 0.0 && oqf == 0.0) {
					if(qtty < OptQtty) {
						ok = 1;
					}
				}
				else if(qtty >= 1.0 && qtty < OptQtty)
					ok = 1;
			}
		}
	}
	if(ok > 0) {
		OptQtty = qtty;
		MaxPrice = price;
		Pos = pos;
		ExtPos = extPos;
	}
	return ok;
}
//
//
//
PPDimention & PPDimention::Z()
{
	Length = 0;
	Width = 0;
	Height = 0;
	return *this;
}

int    PPDimention::operator !() const { return BIN(Length == 0 && Width == 0 && Height == 0); }
bool   FASTCALL PPDimention::IsEq(const PPDimention & rS) const { return (Length == rS.Length && Width == rS.Width && Height == rS.Height); }
bool   FASTCALL PPDimention::operator == (const PPDimention & rS) const { return IsEq(rS); }
bool   FASTCALL PPDimention::operator != (const PPDimention & rS) const { return !IsEq(rS); }
double PPDimention::CalcVolumeM() const { return (fdiv1000i(Width) * fdiv1000i(Length) * fdiv1000i(Height)); }
double PPDimention::CalcVolumeMM() const { return (Width * Length * Height); }

void PPDimention::SetVolumeM(double volume)
{
	Width  = 100L;
	Height = 100L;
	Length = R0i(volume * fpow10i(5));
}

int PPDimention::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, Length, rBuf));
	THROW_SL(pCtx->Serialize(dir, Width, rBuf));
	THROW_SL(pCtx->Serialize(dir, Height, rBuf));
	CATCHZOK
	return ok;
}
//
// Transaction managment
//
int FASTCALL PPStartTransaction(int * pTa, int use_ta)
{
	*pTa = 0;
	int    ok = 1;
	if(use_ta < 0) {
		use_ta = (DBS.GetTaState() == 1) ? 0 : 1;
	}
	if(use_ta) {
		if(DBS.GetTLA().StartTransaction())
			*pTa = 1;
		else
			ok = PPSetErrorDB();
	}
	return ok;
}

int FASTCALL PPCommitWork(int * pTa)
{
	int    ok = 1;
	if(*pTa) {
		if(DBS.GetTLA().CommitWork())
			*pTa = 0;
		else
			ok = PPSetErrorDB();
	}
	return ok;
}

int FASTCALL PPRollbackWork(int * pTa)
{
	if(*pTa) {
		*pTa = 0;
		if(!DBS.GetTLA().RollbackWork())
			return PPSetErrorDB();
	}
	return 1;
}

PPTransaction::PPTransaction(PPDbDependTransaction dbDepend, int use_ta) : Ta(0), Err(0) { Start(dbDepend, use_ta); }
PPTransaction::PPTransaction(int use_ta) : Ta(0), Err(0) { Start(use_ta); }

PPTransaction::~PPTransaction()
{
	if(Ta)
		DBS.GetTLA().RollbackWork();
}

int PPTransaction::operator !()
{
	return Err ? (PPSetErrorDB(), 1) : 0;
}

int PPTransaction::Start(int use_ta)
{
	int    ok = 1;
	if(Ta)
		ok = -1;
	else if(Err)
		ok = 0;
	else {
		if(use_ta < 0) {
			use_ta = (DBS.GetTaState() == 1) ? 0 : 1;
		}
		if(use_ta) {
			int    r = DBS.GetTLA().StartTransaction();
			if(r > 0)
				Ta = 1;
			else if(!r) {
				Err = 1;
				ok = PPSetErrorDB();
			}
		}
		else
			ok = -1;
	}
	return ok;
}

int PPTransaction::Start(PPDbDependTransaction dbDepend, int use_ta)
{
	int    ok = 1;
	if(Ta)
		ok = -1;
	else if(Err)
		ok = 0;
	else {
		if(use_ta < 0) {
			use_ta = (DBS.GetTaState() == 1) ? 0 : 1;
		}
		if(use_ta) {
			int    r = (dbDepend == ppDbDependTransaction) ? DBS.GetTLA().StartTransaction_DbDepend() : DBS.GetTLA().StartTransaction();
			if(r > 0)
				Ta = 1;
			else if(!r) {
				Err = 1;
				ok = PPSetErrorDB();
			}
		}
		else
			ok = -1;
	}
	return ok;
}

int PPTransaction::Commit()
{
	int    ok = 1;
	if(Ta) {
		if(DBS.GetTLA().CommitWork())
			Ta = 0;
		else {
			Err = 1;
			ok = PPSetErrorDB();
		}
	}
	return ok;
}

int PPTransaction::Rollback()
{
	int    ok = 1;
	if(Ta) {
		if(DBS.GetTLA().RollbackWork())
			Ta = 0;
		else {
			Err = 1;
			ok = PPSetErrorDB();
		}
	}
	return ok;
}
//
// Helper database functions
//
int STDCALL SearchByKey(DBTable * pTbl, int idx, void * pKey, void * pData)
{
	int    ok = 1;
	if(pTbl->search(idx, pKey, spEq))
		pTbl->CopyBufTo(pData);
	else
		ok = PPDbSearchError();
	return ok;
}

int STDCALL SearchByKey_ForUpdate(DBTable * pTbl, int idx, void * pKey, void * pData)
{
	int    ok = 1;
	if(pTbl->searchForUpdate(idx, pKey, spEq))
		pTbl->CopyBufTo(pData);
	else
		ok = PPDbSearchError();
	return ok;
}

int PPSetDbRecordByKey(DBTable * pTbl, int idx, void * pKey, const void * pData, int use_ta)
{
	int    ok = -1;
	if(pTbl) {
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pTbl->searchForUpdate(idx, pKey, spEq)) {
			if(pData) {
				THROW_DB(pTbl->updateRecBuf(pData)); // @sfu
				ok = 1;
			}
			else {
				THROW_DB(pTbl->deleteRec()); // @sfu
				ok = 3;
			}
		}
		else {
			THROW(PPDbSearchError());
			if(pData) {
				THROW_DB(pTbl->insertRecBuf(pData));
				ok = 2;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int STDCALL SearchByID(DBTable * pTbl, PPID objType, PPID id, void * b)
{
	int    ok = -1;
	if(pTbl) {
		if(id && pTbl->search(0, &id, spEq)) {
			pTbl->CopyBufTo(b);
			ok = 1;
		}
		else if(!id || BTRNFOUND)
			ok = (PPSetObjError(PPERR_OBJNFOUND, objType, id), -1);
		else
			ok = PPSetErrorDB();
	}
	return ok;
}

int STDCALL SearchByID_ForUpdate(DBTable * pTbl, PPID objType, PPID id, void * b)
{
	int    ok = -1;
	if(pTbl) {
		if(id && pTbl->searchForUpdate(0, &id, spEq)) {
			pTbl->CopyBufTo(b);
			ok = 1;
		}
		else if(!id || BTRNFOUND) {
			ok = (PPSetObjError(PPERR_OBJNFOUND, objType, id), -1);
		}
		else
			ok = PPSetErrorDB();
	}
	return ok;
}

int STDCALL AddByID(DBTable * tbl, PPID * pID, void * b, int use_ta)
{
	int    ok = 1;
	PPID   tmp_id = DEREFPTRORZ(pID);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		tbl->CopyBufFrom(b);
		THROW_DB(tbl->insertRec(0, &tmp_id));
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pID, tmp_id);
	return ok;
}

int STDCALL AdjustNewObjID(DBTable * tbl, PPID objType, void * b)
{
	int    ok = 1;
	if(objType) {
		long   inc = 0;
		long   potential_key = MAXLONG;
		if(tbl->searchKey(0, &potential_key, spLe))
			++potential_key;
		else {
			THROW_DB(BTROKORNFOUND);
			potential_key = 1;
		}
		{
			ObjSyncTbl::Rec objsync_rec;
			int    r2;
			while((r2 = DS.GetTLA().P_ObjSync->SearchPrivate(objType, potential_key+inc, 0, &objsync_rec)) > 0) {
				//
				// Мы должны одновременно прочитать запись (блокировка страницы в транзакции) и удостовериться //
				// в том, что наш ключ не перехвачен другим пользователем
				// @! Важно: здесь происходит изменение содержимого буфера записи tbl.
				//    Вызывающая функция не должна закладываться на то, что какие-то данные для новой записи
				//    содержатся в буфере tbl
				//
				long temp_key;
				do {
					++inc;
					temp_key = potential_key+inc;
				} while(tbl->search(0, &temp_key, spEq));
			}
			THROW(r2);
		}
		if(inc > 0) {
			if(b)
				*static_cast<long *>(b) = potential_key+inc;
			ok = 2;
		}
	}
	CATCHZOK
	return ok;
}

int STDCALL AddObjRecByID(DBTable * tbl, PPID objType, PPID * pID, void * b, int use_ta)
{
	int    ok = 1;
	PPID   tmp_id = DEREFPTRORZ(pID);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ok = AdjustNewObjID(tbl, objType, b));
		tbl->CopyBufFrom(b);
		THROW_DB(tbl->insertRec(0, &tmp_id));
		THROW(tra.Commit());
	}
	if(b)
		*static_cast<long *>(b) = tmp_id;
	CATCHZOK
	if(ok == 2 && CConfig.Flags & CCFLG_DEBUG) {
		SString msg_buf, fmt_buf, obj_title;
		GetObjectTitle(objType, obj_title);
		msg_buf.Printf(PPLoadTextS(PPTXT_LOG_ADDOBJREC_JUMPED_ID, fmt_buf), obj_title.cptr());
		PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
	}
	ASSIGN_PTR(pID, tmp_id);
	return ok;
}

int STDCALL UpdateByID(DBTable * pTbl, PPID objType, PPID id, const void * b, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchByID_ForUpdate(pTbl, objType, id, 0) > 0);
		THROW_DB(pTbl->updateRecBuf(b)); // @sfu
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int STDCALL UpdateByID_Cmp(DBTable * pTbl, PPID objType, PPID id, void * b, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchByID_ForUpdate(pTbl, objType, id, 0) > 0);
		if(!pTbl->GetFields().IsEqualRecords(b, pTbl->getDataBufConst())) {
			THROW_DB(pTbl->updateRecBuf(b)); // @sfu
		}
		else
			ok = -1;
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int STDCALL RemoveByID(DBTable * tbl, PPID id, int use_ta)
{
	int    ok = 1;
	if(tbl) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchByID_ForUpdate(tbl, 0, id, 0) > 0);
		THROW_DB(tbl->deleteRec()); // @sfu
		THROW(tra.Commit());
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int STDCALL IncDateKey(DBTable * tbl, int idx, LDATE date, long * pOprNo)
{
	int    ok = 1;
	struct {
		LDATE dt;
		long  oprno;
	} k;
	k.dt    = date;
	k.oprno = MAXLONG;
	if(tbl->searchKey(idx, &k, spLt))
		*pOprNo = (k.dt == date) ? (k.oprno + 1) : 1;
	else {
		*pOprNo = 1;
		if(!BTRNFOUND)
			ok = PPSetErrorDB();
	}
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(_CreateTempOrderFile, TempOrder);

TempOrderTbl * CreateTempOrderFile()
{
	return _CreateTempOrderFile();
}

DBQ & FASTCALL ppidlist(DBItem & dbi, const PPIDArray * pList)
{
	DBQ * p_dbq = 0;
	if(pList)
		for(uint i = 0; i < pList->getCount(); i++) {
			if(i == 0)
				p_dbq = & (dbi == pList->get(i));
			else
				p_dbq = & (*p_dbq || dbi == pList->get(i));
		}
	return *p_dbq;
}

DBQ * STDCALL ppcheckfiltid(DBQ * pDbq, DBItem & rDbi, PPID fltID)
{
	if(fltID)
		pDbq = &(*pDbq && rDbi == fltID);
	return pDbq;
}

DBQ * STDCALL ppcheckfiltidlist(DBQ * pDbq, DBItem & rDbi, const PPIDArray * pList)
{
	if(pList)
		if(pList->isList())
			return &(*pDbq && ppidlist(rDbi, pList));
		else
			return ppcheckfiltid(pDbq, rDbi, pList->getSingle());
	else
		return pDbq;
}

DBQ * STDCALL ppcheckflag(DBQ * pDbq, DBItem & rDbi, long mask, int test)
{
	if(test > 0 || test < 0) {
		DBE * p_dbe = &(rDbi & mask);
		if(test > 0)
			pDbq = &(*pDbq && *p_dbe == mask);
		else
			pDbq = &(*pDbq && *p_dbe == 0L);
		//
		//p_dbe->destroy(); Эта операция не должна выполняться, ибо разрушает используемые структуры
		//
		delete p_dbe;
	}
	return pDbq;
}

DBQ * STDCALL ppcheckweekday(DBQ * pDbq, DBItem & rDbi, int dayOfWeek)
{
	if(dayOfWeek) {
		assert(rDbi.baseType() == BTS_DATE);
		DBE * p_dbe = new DBE;
		p_dbe->init();
		p_dbe->push(rDbi);
		p_dbe->push(dbq_weekday);
		pDbq = &(*pDbq && *p_dbe == static_cast<long>(dayOfWeek));
		delete p_dbe;
	}
	return pDbq;
}
//
// Check functions
//
int FASTCALL CheckTblPtr(const DBTable * tbl)
{
	if(tbl == 0)
		return PPSetErrorNoMem();
	else if(tbl->IsOpened() == 0)
		return PPSetErrorDB();
	else
		return 1;
}

int FASTCALL CheckQueryPtr(const DBQuery * q)
{
	if(q == 0)
		return PPSetErrorNoMem();
	else if(q->error)
		return PPSetError(PPERR_DBQUERY);
	else
		return 1;
}
//
// PPTblEnumList
//
PPTblEnumList::PPTblEnumList()
{
}

PPTblEnumList::~PPTblEnumList()
{
	for(uint i = 0; i < Tab.getCount(); i++)
		DestroyIterHandler(static_cast<long>(i));
}

int PPTblEnumList::RegisterIterHandler(BExtQuery * pQ, long * pHandle)
{
	int    ok = 1;
	long   handle = -1;
	for(uint i = 0; handle < 0 && i < Tab.getCount(); i++) {
		if(Tab.at(i) == 0) {
			Tab.at(i) = pQ;
			handle = static_cast<long>(i);
		}
	}
	if(handle < 0) {
		Tab.insert(&pQ);
		assert(Tab.getCount());
		handle = Tab.getCountI()-1;
	}
	ASSIGN_PTR(pHandle, handle);
	return ok;
}

int PPTblEnumList::DestroyIterHandler(long handle)
{
	int    ok = -1;
	uint   pos = static_cast<uint>(handle);
	if(pos < Tab.getCount()) {
		BExtQuery * p_q = static_cast<BExtQuery *>(Tab.at(pos));
		if(p_q) {
			delete p_q;
			Tab.at(pos) = 0;
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL PPTblEnumList::NextIter(long handle)
{
	int    ok = 0;
	uint   pos = static_cast<uint>(handle);
	if(pos < Tab.getCount()) {
		BExtQuery * p_q = static_cast<BExtQuery *>(Tab.at(pos));
		if(p_q)
			if(p_q->nextIteration() > 0)
				ok = 1;
			else
				ok = -1;
	}
	return ok;
}
//
// Special helper functions
//
PPID GetSupplAccSheet()
{
	const  PPCommConfig & r_ccfg = CConfig;
	PPID   acs_id = r_ccfg.SupplAccSheet;
	if(!acs_id) {
		PPOprKind op_rec;
		if(r_ccfg.ReceiptOp && GetOpData(r_ccfg.ReceiptOp, &op_rec) > 0)
			acs_id = op_rec.AccSheetID;
		if(!acs_id) {
			if(GetOpData(PPOPK_RECEIPT, &op_rec) > 0)
				acs_id = op_rec.AccSheetID;
		}
	}
	return acs_id;
}

PPID GetSellAccSheet()
{
	const  PPCommConfig & r_ccfg = CConfig;
	PPID   acc_sheet_id = 0;
	if(r_ccfg.SellAccSheet == 0) {
		PPOprKind opk;
		acc_sheet_id = GetOpData(PPOPK_SELL, &opk) ? opk.AccSheetID : 0;
	}
	else
		acc_sheet_id = r_ccfg.SellAccSheet;
	if(!acc_sheet_id)
		PPSetError(PPERR_UNDEFCLIACCSHEET);
	return acc_sheet_id;
}

PPID GetSellPersonKind()
{
    PPID   pk_id = 0;
    const  PPID acs_id = GetSellAccSheet();
    if(acs_id) {
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		if(acs_obj.Fetch(acs_id, &acs_rec) > 0) {
			if(acs_rec.Assoc == PPOBJ_PERSON) {
				pk_id = acs_rec.ObjGroup;
			}
		}
    }
    return pk_id;
}

PPID GetAgentAccSheet()
{
	PPID   agent_acs_id = 0;
	PPThreadLocalArea & r_tla = DS.GetTLA();
	if(r_tla.AgentAccSheetID > 0) {
		agent_acs_id = r_tla.AgentAccSheetID;
	}
	else if(r_tla.AgentAccSheetID == 0) {
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		for(SEnum en = acs_obj.P_Ref->Enum(PPOBJ_ACCSHEET, 0); !agent_acs_id && en.Next(&acs_rec) > 0;) {
			if(acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup == PPPRK_AGENT)
				agent_acs_id = acs_rec.ID;
		}
		r_tla.AgentAccSheetID = NZOR(agent_acs_id, -1);
	}
	if(agent_acs_id == 0)
		PPSetError(PPERR_UNDEFAGENTACCSHEET);
	return agent_acs_id;
}

SString & FASTCALL GetCurUserName(SString & rBuf)
{
	rBuf = DS.GetConstTLA().UserName;
	return rBuf;
}

int FASTCALL GetLocationName(PPID locID, SString & rBuf)
{
	int    ok = -1;
	//
	// Если BillObj == 0, то PPObjLocation::Fetch пытается обратиться к нему и завешивает систему
	//
	if(BillObj) {
		PPObjLocation loc_obj;
		LocationTbl::Rec loc_rec;
		if(loc_obj.Fetch(locID, &loc_rec) > 0) {
			rBuf = loc_rec.Name;
			if(!rBuf.NotEmptyS())
				rBuf = loc_rec.Code;
			if(!rBuf.NotEmptyS())
				ideqvalstr(locID, rBuf.Z());
			ok = 1;
		}
		else
			ideqvalstr(locID, rBuf.Z());
	}
	else
		ok = GetObjectName(PPOBJ_LOCATION, locID, rBuf);
	return ok;
}

SString & GetExtLocationName(const ObjIdListFilt & rLocList, size_t maxItems, SString & rBuf)
{
	if(rLocList.IsEmpty()) {
		PPLoadText(PPTXT_ALLWAREHOUSES, rBuf);
	}
	else {
		PPObjLocation loc_obj; // @frame
		for(uint i = 0; i < rLocList.GetCount(); i++) {
			if(i > 0)
				rBuf.CatDiv(';', 2);
			if(maxItems && i >= maxItems) {
				rBuf.Dot().Dot();
				break;
			}
			else
				CatObjectName(PPOBJ_LOCATION, rLocList.Get().at(i), rBuf);
		}
	}
	return rBuf;
}

int FASTCALL GetExtLocationName(PPID locID, SString & rBuf)
{
	int    ok = 1;
	if(locID)
		ok = GetLocationName(locID, rBuf);
	else
		PPLoadText(PPTXT_ALLWAREHOUSES, rBuf);
	return ok;
}

DbfTable * FASTCALL PPOpenDbfTable(const char * pPath)
{
	DbfTable * p_tbl = new DbfTable(pPath);
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(!p_tbl->isOpened()) {
		ZDELETE(p_tbl);
		//PPSetError(PPERR_DBFOPFAULT, pPath);
		PPSetErrorSLib();
	}
	return p_tbl;
}

DbfTable * FASTCALL CreateDbfTable(uint rezID, const char * fName, int forceReplace)
{
	uint   num_flds = 0;
	int    exists = 0;
	DbfTable * p_tbl  = 0;
	DBFCreateFld * p_flds = 0;
	SString file_name(fName);
	if(::fileExists(file_name)) {
		if(forceReplace) {
			SFsPath::ReplaceExt(file_name, "DBK", 1);
			SFile::Remove(file_name);
			SFile::Rename(fName, file_name);
		}
		else
			exists = 1;
	}
	if(!exists) {
		THROW_MEM(p_tbl = new DbfTable(fName));
		THROW(p_flds = LoadDBFStruct(rezID, &num_flds));
		THROW_PP_S(p_tbl->create(num_flds, p_flds), PPERR_DBFCRFAULT, fName);
		ZDELETE(p_tbl);
	}
	THROW(p_tbl = PPOpenDbfTable(fName));
	CATCH
		ZDELETE(p_tbl);
	ENDCATCH
	delete p_flds;
	return p_tbl;
}

int FASTCALL CheckFiltID(PPID flt, PPID id) { return (!flt || flt == id); }

char * FASTCALL QttyToStr(double qtty, double upp, long fmt, char * buf, int noabs)
{
	const  uint f = SFMTFLAG(fmt);
	double ipart;
	double fract;
	int    sign = 0;
	if(qtty < 0.0) {
		sign = 1;
		qtty = -qtty;
	}
	upp   = R3(upp);
	qtty  = R3(qtty);
	fract = R6(modf(qtty, &ipart));
	if(f & QTTYF_FRACTION && fract != 0)
		SETSFMTPRC(fmt, 3);
	char * p = buf;
	if(f & NMBF_NOZERO && qtty == 0)
		*p++ = 0;
	else {
		if(sign && noabs)
			*p++ = '-';
		if(f & QTTYF_NUMBER || !(f & QTTYF_MASK) || upp == 0.0 || upp == 1.0)
			realfmt(qtty, fmt, p);
		else {
			fract = R6(modf(R6(qtty / upp), &ipart));
			if(f & QTTYF_SIMPLPACK) {
				if(fract == 0) {
					p    += sstrlen(intfmt((long)ipart, 0, p));
					*p++  = '/';
					realfmt(upp, MKSFMTD(0, 3, NMBF_NOTRAILZ), p);
				}
				else
					realfmt(qtty, fmt, p);
			}
			else if(f & QTTYF_COMPLPACK && ipart != 0) {
				p    += sstrlen(intfmt((long)ipart, 0, p));
				*p++  = '/';
				p    += sstrlen(realfmt(upp, MKSFMTD(0, 3, NMBF_NOTRAILZ), p));
				if(fract != 0 && (fract = fmod(qtty, upp)) != 0) {
					*p++ = '+';
					realfmt(fract, MKSFMTD(0, 3, NMBF_NOTRAILZ), p);
				}
			}
			else
				realfmt(qtty, fmt, p);
		}
	}
	return _commfmt(fmt, buf);
}

SString & GetCurSymbText(PPID curID, SString & rBuf)
{
	rBuf.Z();
	if(curID >= 0) {
		PPID   cur_id = NZOR(curID, LConfig.BaseCurID);
		if(cur_id) {
			PPObjCurrency cur_obj;
			PPCurrency cur_rec;
			if(cur_obj.Fetch(cur_id, &cur_rec) > 0) {
				if(curID)
					rBuf = cur_rec.Symb;
				else
					rBuf.CatChar('*').Cat(cur_rec.Symb);
			}
			else
				ideqvalstr(cur_id, rBuf);
		}
	}
	return rBuf;
}
//
// PPSymbTranslator
// @todo Перевести реализацию на SString и SStrScan
//
PPSymbTranslator::PPSymbTranslator(uint strID /*=PPSSYM_SYMB*/) : ErrorCode(0)
{
	if(!PPLoadString(PPSTR_SYMB, strID, Coll))
		ErrorCode = PPErrCode;
}

bool PPSymbTranslator::operator !() const { return ErrorCode ? (PPErrCode = ErrorCode, true) : false; }

char * PPSymbTranslator::NextStr(size_t * pPos, char * pBuf) const
{
	size_t p = *pPos;
	if(Coll.C(p)) {
		char * b = pBuf;
		while(Coll.C(p) != 0 && Coll.C(p) != ';')
			*b++ = Coll.C(p++);
		*b = 0;
		if(Coll.C(p))
			p++;
		*pPos = p;
		return pBuf;
	}
	else
		return 0;
}

long PPSymbTranslator::Translate(SStrScan & rScan)
{
	long   v = 0;
	int    count = 0;
	size_t max_len = 0;
	char * b, sub[256], temp[128];
	rScan.Len = 0;
	rScan.Skip(SStrScan::wsSpace|SStrScan::wsTab);
	for(size_t bp = 0; /*!v &&*/ (b = NextStr(&bp, sub)) != 0;) {
		count++;
		while(/*!v &&*/ *b) {
			size_t len = 0;
			for(b = strip(STRNSCPY(temp, b)); *b != 0 && *b != ','; b++)
				len++;
			if(*b == ',')
				b++;
			if(strnicmp866(rScan, temp, len) == 0) {
				if(len > max_len) {
					max_len = len;
					v = count;
				}
			}
		}
	}
	if(v == 0)
		PPSetError(PPERR_UNDEFSYMB, rScan);
	else
		rScan.Len = max_len;
	return v;
}

long PPSymbTranslator::Translate(const char * pString, size_t * pNextPos, uint /*flags*/)
{
	long   v = 0;
	int    count = 0;
	size_t max_len = 0;
	const size_t start_pos = DEREFPTRORZ(pNextPos);
	size_t p = start_pos;
	char * b, sub[256], temp[128];
	while(oneof2(pString[p], ' ', '\t'))
		p++;
	size_t next_pos = p;
	for(size_t bp = 0; /*!v &&*/ (b = NextStr(&bp, sub)) != 0;) {
		count++;
		while(/*!v && */*b) {
			size_t len = 0;
			for(b = strip(STRNSCPY(temp, b)); *b != 0 && *b != ','; b++)
				len++;
			if(*b == ',')
				b++;
			if(strnicmp866(pString + p, temp, len) == 0) {
				if(len > max_len) {
					max_len = len;
					v = count;
				}
			}
		}
	}
	if(v == 0) {
		next_pos = start_pos;
		PPSetError(PPERR_UNDEFSYMB, pString+p);
	}
	else {
		next_pos += max_len;
	}
	ASSIGN_PTR(pNextPos, next_pos);
	return v;
}

int PPSymbTranslator::Retranslate(long sym, char * s, size_t bufLen) const
{
	const  char * p = Coll;
	int    count = 0;
	do {
		if(++count == sym) {
			size_t i = 0;
			while(oneof2(*p, ' ', '\t'))
				p++;
			while((!bufLen || i < (bufLen - 1)) && !oneof5(*p, 0, ',', ';', ' ', '\t'))
				s[i++] = *p++;
			s[i] = 0;
			return 1;
		}
	} while((p = sstrchr(p, ';'))++ != 0);
	return PPSetError(PPERR_UNDEFSYMB);
}

int PPSymbTranslator::Retranslate(long sym, SString & rBuf) const
{
	const  char * p = Coll;
	int    count = 0;
	rBuf.Z();
	do {
		if(++count == sym) {
			while(oneof2(*p, ' ', '\t'))
				p++;
			while(!oneof5(*p, 0, ',', ';', ' ', '\t'))
				rBuf.CatChar(*p++);
			return 1;
		}
	} while((p = sstrchr(p, ';'))++ != 0);
	return PPSetError(PPERR_UNDEFSYMB);
}
//
// DateIter
//
DateIter::DateIter() : dt(ZERODATE), end(ZERODATE), oprno(0)
{
}

DateIter::DateIter(long start, long finish)
{
	Init(start, finish);
}

DateIter::DateIter(const DateRange * pPeriod)
{
	Init(pPeriod);
}

void DateIter::Init()
{
	dt = ZERODATE;
	end = ZERODATE;
	oprno = 0;
}

void DateIter::Init(long start, long finish)
{
	dt.v  = start;
	end.v = finish;
	oprno = 0;
}

void DateIter::Init(const DateRange * pPeriod)
{
	if(pPeriod)
		Init(pPeriod->low, pPeriod->upp);
	else
		Init();
}

int DateIter::Advance(LDATE d, long o)
{
	dt = d;
	oprno = o;
	return IsEnd() ? -1 : 1;
}

bool DateIter::IsEnd() const { return (end && dt > end); }
int FASTCALL DateIter::Cmp(const DateIter & rS) const { RET_CMPCASCADE2(this, &rS, dt, oprno); }
//
// Loading DBF structure from resource
//
DBFCreateFld * LoadDBFStruct(uint rezID, uint * pNumFlds)
{
	uint   i, num_flds = 0;
	char   name[32];
	DBFCreateFld * p_flds = 0;
	TVRez * p_rez = P_SlRez;
	if(p_rez) {
		THROW_PP(p_rez->findResource(rezID, PP_RCDATA), PPERR_RESFAULT);
		THROW_PP(num_flds = p_rez->getUINT(), PPERR_RESFAULT);
		THROW_MEM(p_flds = new DBFCreateFld[num_flds]);
		for(i = 0; i < num_flds; i++) {
			STRNSCPY(p_flds[i].Name, newStr(p_rez->getString(name)));
			p_flds[i].Type = p_rez->getChar();
			p_flds[i].Size = static_cast<uchar>(p_rez->getUINT());
			p_flds[i].Prec = static_cast<uchar>(p_rez->getUINT());
		}
	}
	CATCH
		ZDELETEARRAY(p_flds);
		num_flds = 0;
	ENDCATCH
	ASSIGN_PTR(pNumFlds, num_flds);
	return p_flds;
}

int STDCALL LoadSdRecord(uint rezID, SdRecord * pRec, int headerOnly /*=0*/)
{
	int    ok = 1;
	PROFILE_START
	TVRez * p_rez = P_SlRez;
	pRec->Clear();
	THROW_PP(p_rez, PPERR_REZNOTINITED);
	THROW_PP(p_rez->findResource(rezID, PP_RCDECLRECORD), PPERR_RESFAULT);
	pRec->ID = rezID;
	p_rez->getString(pRec->Name, 0);
	if(!headerOnly) {
		uint   num_flds = 0, i;
		SString fld_name, fld_descr;
		SdbField fld;
		THROW_PP(num_flds = p_rez->getUINT(), PPERR_RESFAULT);
		for(i = 0; i < num_flds; i++) {
			fld.Z();
			uint   fld_id = p_rez->getUINT();
			p_rez->getString(fld.Name, 0);
			uint t_ = p_rez->getUINT();
			uint s_ = p_rez->getUINT();
			uint p_ = p_rez->getUINT();
            fld.T.Typ = p_ ? MKSTYPED(t_, s_, p_) : MKSTYPE(t_, s_);
			uint fl_ = p_rez->getUINT();
			uint fp_ = p_rez->getUINT();
			uint ff_ = p_rez->getUINT();
			fld.OuterFormat = fp_ ? MKSFMTD(fl_, fp_, ff_) : MKSFMT(fl_, ff_);
			p_rez->getString(fld.Descr, 0);
			SLS.ExpandString(fld.Descr, CTRANSF_UTF8_TO_OUTER);
			THROW_SL(pRec->AddField(&fld_id, &fld));
		}
	}
	CATCHZOK
	PROFILE_END
	return ok;
}
//
//
//
PPExtStringStorage::PPExtStringStorage() : Re("<[0-9]+>", cpANSI, SRegExp2::syntaxDefault, 0)
{
}

int PPExtStringStorage::Excise(SString & rLine, int fldID)
{
	int    ok = -1;
	if(rLine.NotEmpty()) {
		SStrScan scan(rLine);
		SString temp_buf;
		while(ok <= 0 && Re.Find(&scan, 0)) {
			scan.Get(temp_buf).TrimRight().ShiftLeft();
			const size_t tag_offs = scan.IncrLen();
			if(temp_buf.ToLong() == fldID) {
				if(Re.Find(&scan, 0))
					rLine.Excise(tag_offs, scan.Offs - tag_offs);
				else
					rLine.Trim(tag_offs);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPExtStringStorage::Put(SString & rLine, int fldID, const char * pBuf)
{
	int    ok = -2;
	while(Excise(rLine, fldID) > 0) {
		ok = 1;
	}
	if(!isempty(pBuf)) {
		rLine.CatChar('<').Cat(fldID).CatChar('>').Cat(pBuf);
		ok = 1;
	}
	return ok;
}

int PPExtStringStorage::Put(SString & rLine, int fldID, const SString & rBuf)
{
	int    ok = -2;
	while(Excise(rLine, fldID) > 0) {
		ok = 1;
	}
	if(rBuf.NotEmpty()) {
		rLine.CatChar('<').Cat(fldID).CatChar('>').Cat(rBuf);
		ok = 1;
	}
	return ok;
}

int PPExtStringStorage::Get(const SString & rLine, int fldID, SString & rBuf)
{
	int    ok = -2;
	rBuf.Z();
	if(rLine.NotEmpty()) {
		SStrScan scan(rLine);
		SString & r_temp_buf = SLS.AcquireRvlStr();
		while(ok <= 0 && Re.Find(&scan, 0)) {
			scan.Get(r_temp_buf).TrimRight().ShiftLeft();
			size_t tag_offs = scan.IncrLen();
			if(r_temp_buf.ToLong() == fldID) {
				size_t start = scan.Offs;
				if(Re.Find(&scan, 0))
					rBuf.CopyFromN(rLine+start, scan.Offs-start);
				else
					rBuf.CopyFrom(rLine+start);
				ok = 1;
			}
			else
				ok = 0;
		}
	}
	return ok;
}

int PPExtStringStorage::Enum(const SString & rLine, uint * pPos, int * pFldID, SString & rBuf)
{
	rBuf.Z();
	int    ok = -1;
	int    fld_id = 0;
    uint   pos = DEREFPTRORZ(pPos);
    if(pos < rLine.Len()) {
		SStrScan scan(rLine);
		scan.Incr(pos);
		if(Re.Find(&scan, 0)) {
			SString temp_buf;
			scan.Get(temp_buf).TrimRight().ShiftLeft();
			size_t tag_offs = scan.IncrLen();
			fld_id = temp_buf.ToLong();
			size_t start = scan.Offs;
			if(Re.Find(&scan, 0)) {
				//pos = scan.Offs+scan.Len;
				pos = (uint)scan.Offs;
				rBuf.CopyFromN(rLine+start, scan.Offs-start);
			}
			else {
				pos = (uint)rLine.Len();
				rBuf.CopyFrom(rLine+start);
			}
			ok = 1;
		}
    }
    ASSIGN_PTR(pPos, pos);
    ASSIGN_PTR(pFldID, fld_id);
    return ok;
}

int STDCALL PPGetExtStrData_def(int fldID, int defFldID, const SString & rLine, SString & rBuf)
{
	int    ok = -1;
	int    r = 0;
	if(rLine.NotEmpty()) {
		PPExtStringStorage ess;
		r = ess.Get(rLine, fldID, rBuf);
	}
	else {
		rBuf.Z();
		r = -2;
	}
	if(r > 0)
		ok = 1;
	else if(r == -2 && fldID == defFldID) {
		rBuf = rLine;
		ok = 1;
	}
	return ok;
}

int STDCALL PPGetExtStrData(int fldID, const SString & rLine, SString & rBuf)
{
	int    ok = -1;
	int    r = 0;
	if(rLine.NotEmpty()) {
		PPExtStringStorage ess;
		r = ess.Get(rLine, fldID, rBuf);
	}
	else {
		rBuf.Z();
		r = -2;
	}
	if(r > 0)
		ok = 1;
	return ok;
}

int STDCALL PPCmpExtStrData(int fldID, const SString & rLine1, const SString & rLine2, long options)
{
    SString buf1, buf2;
    PPExtStringStorage ess;
    ess.Get(rLine1, fldID, buf1);
    ess.Get(rLine2, fldID, buf2);
	return buf1.Cmp(buf2, BIN(options & srchNoCase));
}

int STDCALL PPPutExtStrData(int fldID, SString & rLine, const char * pBuf)
{
	PPExtStringStorage ess;
	return ess.Put(rLine, fldID, pBuf);
}

int STDCALL PPPutExtStrData(int fldID, SString & rLine, const SString & rBuf)
{
	PPExtStringStorage ess;
	return ess.Put(rLine, fldID, rBuf);
}
//
//
//
PPExtStrContainer::PPExtStrContainer() {}
PPExtStrContainer & PPExtStrContainer::Z() { ExtString.Z(); return *this; }
bool   FASTCALL PPExtStrContainer::Copy(const PPExtStrContainer & rS) { ExtString = rS.ExtString; return true; }
int    PPExtStrContainer::GetExtStrData(int fldID, SString & rBuf) const { return PPGetExtStrData(fldID, ExtString, rBuf); }
int    PPExtStrContainer::PutExtStrData(int fldID, const char * pStr) { return PPPutExtStrData(fldID, ExtString, pStr); }
int    PPExtStrContainer::SerializeB(int dir, SBuffer & rBuf, SSerializeContext * pSCtx) { return pSCtx->Serialize(dir, ExtString, rBuf) ? 1 : PPSetErrorSLib(); }
void   FASTCALL PPExtStrContainer::SetBuffer(const char * pSrc) { ExtString = pSrc; }
const  SString & PPExtStrContainer::GetBuffer() const { return ExtString; }

bool PPExtStrContainer::IsEq(const PPExtStrContainer & rS, int fldCount, const int * pFldList) const
{
	bool   yes = true;
	assert(fldCount > 0);
	if(fldCount > 0) {
		assert(pFldList != 0);
		SString s1;
		SString s2;
		for(int i = 0; yes && i < fldCount; i++) {
			const int fld_id = pFldList[i];
			PPGetExtStrData(fld_id, ExtString, s1);
			PPGetExtStrData(fld_id, rS.ExtString, s2);
			if(s1 != s2)
				yes = false;
		}
	}
	return yes;
}
//
//
//
/*
Reference
{
	ObjType = PPOBJ_UNASSIGNED
	ObjID   = 1
	ObjName = F2(BillRec, ArticleRec)
	AltText = Password for removing chaining
	Val1    = BillID -> Bill.ID
	Val2    = ArID -> Article.ID
}

Bill
{
	ID:          Any <- Reference.Val1
	Code[0..8]:  Random Data
	Code[9]:     0
	OprKind:     PPOPK_UNASSIGNED
	Object:      ArID -> Article.ID
}

Article
{
	ID:          Any <- Reference.Val2
	AccSheetID:  PPACSH_UNASSIGNED
	Article:     1
	ObjID:       BillID -> Bill.ID
	Name[0..46]: Random Data
	Name[47]:    0
}
*/
static int _F2_(const BillTbl::Rec * pBillRec, const ArticleTbl::Rec * pArRec, ReferenceTbl::Rec * pRefRec)
{
	const size_t ar_buf_offs = 10;
	memzero(pRefRec->ObjName, sizeof(pRefRec->ObjName));
	const size_t len = MIN(sizeof(pArRec->Name), sizeof(pRefRec->ObjName))-1;
	for(size_t i = 0; i < len; i++) {
		char s = pArRec->Name[i];
		if(i >= ar_buf_offs && i < (ar_buf_offs + sizeof(pBillRec->Code) - 1))
			s += pBillRec->Code[i-ar_buf_offs];
		pRefRec->ObjName[i] = s;
	}
	return 1;
}

int PPChainDatabase(const char * pPassword)
{
	// 1. Проверить связку базы.
	//    Если база связана (с нарушением или без) то ничего не делаем
	// 2. Создаем образ записи в таблице Bill
	//    Идентификатор записи выбираем по следующему правилу:
	//    - получаем максимальный ид в таблице (id_max)
	//    - получаем случайное число в диапазоне [1..23] (id_delta)
	//    - BillID = id_max + id_delta
	// 3. Создаем образ записи в таблице Article
	//    Идентификатор записи выбираем по следующему правилу:
	//    - получаем максимальный ид в таблице (id_max)
	//    - получаем случайное число в диапазоне [1..37] (id_delta)
	//    - ArticleID = id_max + id_delta
	// 4. Связываем записи BillRec и ArticleRec идентификаторами
	//    (Bill.Object = Article.ID, Article.ObjID = Bill.ID)
	// 5. Создаем образ записи в таблице Reference
	// 6. Расчитываем функцию F2(BillRec, ArticleRec)
	// 7. Сохраняем в транзакции все три записи в своих таблицах
	//
	int    ok = 1, r;
	PPID   id_max, id_delta;
	ReferenceTbl::Rec ref_rec;
	BillTbl::Rec      bill_rec;
	ArticleTbl::Rec   ar_rec;

	// 1.
	if(PPCheckDatabaseChain() < 0) {

		PPObjBill * p_bobj = BillObj;
		Reference * p_ref = PPRef;
		// 2.
		id_max = 0;
		r = p_bobj->P_Tbl->search(0, &id_max, spLast);
		if(!r) {
			if(BTRNFOUND)
				id_max = 1;
			else
				CALLEXCEPT_PP(PPERR_DBENGINE);
		}
		// @v11.1.1 IdeaRandMem(&id_delta, sizeof(id_delta));
		SObfuscateBuffer(&id_delta, sizeof(id_delta)); // @v11.1.1 
		id_delta = (labs(id_delta) % 23) + 1;
		bill_rec.ID = id_max + id_delta;

		// @v11.1.1 IdeaRandMem(bill_rec.Code, sizeof(bill_rec.Code)-1);
		SObfuscateBuffer(bill_rec.Code, sizeof(bill_rec.Code)-1); // @v11.1.1 
		for(r = 0; r < sizeof(bill_rec.Code)-1; r++)
			bill_rec.Code[r] = (char)(labs(bill_rec.Code[r]) % 93 + 33);
		bill_rec.OpID = PPOPK_UNASSIGNED;

		// 3.
		ar_rec.Clear();
		id_max = 0;
		r = p_bobj->atobj->P_Tbl->Art.search(0, &id_max, spLast);
		THROW_DB(r || BTROKORNFOUND);
		if(r <= 0)
			id_max = 1;
		// @v11.1.1 IdeaRandMem(&id_delta, sizeof(id_delta));
		SObfuscateBuffer(&id_delta, sizeof(id_delta)); // @v11.1.1 
		id_delta = (labs(id_delta) % 37) + 1;
		ar_rec.ID = id_max + id_delta;

		// @v11.1.1 IdeaRandMem(ar_rec.Name, sizeof(ar_rec.Name)-1);
		SObfuscateBuffer(ar_rec.Name, sizeof(ar_rec.Name)-1); // @v11.1.1 
		IdeaEncrypt("FA", ar_rec.Name, sizeof(ar_rec.Name)-1);
		for(r = 0; r < sizeof(ar_rec.Name)-1; r++)
			ar_rec.Name[r] = (abs(ar_rec.Name[r]) % 93) + 33;
		ar_rec.AccSheetID = PPACSH_UNASSIGNED;
		ar_rec.Article = 1;

		// 4.
		bill_rec.Object = ar_rec.ID;
		ar_rec.ObjID = bill_rec.ID;

		// 5.
		MEMSZERO(ref_rec);
		ref_rec.ObjType = PPOBJ_UNASSIGNED;
		ref_rec.ObjID   = 1;
		ref_rec.Val1    = bill_rec.ID;
		ref_rec.Val2    = ar_rec.ID;
		Reference::Encrypt(Reference::crymRef2, pPassword, ref_rec.Symb, sizeof(ref_rec.Symb));
		// 6.
		_F2_(&bill_rec, &ar_rec, &ref_rec);
		IdeaEncrypt(0, ref_rec.ObjName, sizeof(ref_rec.ObjName)-1);

		// 7.
		{
			PPTransaction tra(1);
			THROW(tra);
			p_bobj->P_Tbl->CopyBufFrom(&bill_rec);
			THROW_DB(p_bobj->P_Tbl->insertRec());
			THROW_DB(p_bobj->atobj->P_Tbl->Art.insertRecBuf(&ar_rec));
			THROW_DB(p_ref->insertRecBuf(&ref_rec));
			THROW(tra.Commit());
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

enum {
	pdbcmVerify    = 0,
	pdbcmUnchain   = 1,
	pdbcmReEncrypt = 2,
};

static int ProcessDatabaseChain(PPObjBill * pBObj, Reference * pRef, int mode, const char * pPassword, const char * pSrcEncPw, const char * pDestEncPw, int use_ta)
{
	int    ok = -1;
	SString pw;
	ReferenceTbl::Rec ref_rec, ref_rec2;
	BillTbl::Rec      bill_rec;
	ArticleTbl::Rec   ar_rec;
	if(CurDict->GetCapability() & DbProvider::cSQL)
		ok = 1;
	else {
		uint   i;
		ArticleCore & r_ar_tbl = pBObj->atobj->P_Tbl->Art;
		if(pRef->GetItem(PPOBJ_UNASSIGNED, 1L, &ref_rec) > 0) {
			int    r = pBObj->P_Tbl->Search(ref_rec.Val1, &bill_rec);
			THROW(r);
			THROW_PP(r > 0, PPERR_DBCHA_BILLABSENCE);
			const size_t len = sstrlen(bill_rec.Code);
			memzero(bill_rec.Code+len, sizeof(bill_rec.Code)-len);
			THROW_PP(r_ar_tbl.Search(ref_rec.Val2, &ar_rec) > 0, PPERR_DBCHA_ARABSENCE);
			_F2_(&bill_rec, &ar_rec, &ref_rec2);
			IdeaDecrypt(/*0*/pSrcEncPw, ref_rec.ObjName, sizeof(ref_rec.ObjName)-1);
			for(i = 0; ok && i < sizeof(ref_rec.ObjName)-1; i++) {
				THROW_PP(ref_rec.ObjName[i] == ref_rec2.ObjName[i], PPERR_DBCHA_INVDATA);
			}
			THROW_PP(bill_rec.Object == ar_rec.ID && ar_rec.ObjID == bill_rec.ID, PPERR_DBCHA_INVLINKS);
			if(mode == pdbcmReEncrypt) {
				THROW(Reference::Helper_Decrypt_(Reference::crymRef2, pSrcEncPw, ref_rec.Symb, sizeof(ref_rec.Symb), pw));
				THROW(Reference::Helper_Encrypt_(Reference::crymRef2, pDestEncPw, pw, ref_rec.Symb, sizeof(ref_rec.Symb)));
				IdeaEncrypt(pDestEncPw, ref_rec.ObjName, sizeof(ref_rec.ObjName)-1);
				THROW(pRef->UpdateItem(PPOBJ_UNASSIGNED, 1L, &ref_rec, 0, use_ta));
			}
			else if(mode == pdbcmUnchain) {
				THROW(Reference::Decrypt(Reference::crymRef2, ref_rec.Symb, sizeof(ref_rec.Symb), pw));
				THROW_PP(pPassword && pw.CmpNC(pPassword) != 0, PPERR_DBCHA_INVPASSWORD);
				{
					PPTransaction tra(use_ta);
					THROW(tra);
					THROW(pRef->RemoveItem(PPOBJ_UNASSIGNED, 1L, 0));
					THROW_DB(pBObj->P_Tbl->deleteRec());
					THROW_DB(r_ar_tbl.deleteRec());
					THROW(tra.Commit());
				}
			}
			ok = 1;
		}
		else {
			DateIter di;
			long   ar_no = 0;
			THROW_PP(pBObj->P_Tbl->EnumByOpr(PPOPK_UNASSIGNED, &di, &bill_rec) < 0, PPERR_DBCHA_REFABSENCE);
			THROW_PP(r_ar_tbl.EnumBySheet(PPACSH_UNASSIGNED, &ar_no, &ar_rec) < 0, PPERR_DBCHA_REFABSENCE);
		}
	}
	CATCHZOK
	// @v11.1.1 IdeaRandMem(&ref_rec,  sizeof(ref_rec));
	SObfuscateBuffer(&ref_rec,  sizeof(ref_rec)); // @v11.1.1 
	// @v11.1.1 IdeaRandMem(&ref_rec2, sizeof(ref_rec2));
	SObfuscateBuffer(&ref_rec2, sizeof(ref_rec2)); // @v11.1.1 
	// @v11.1.1 IdeaRandMem(&bill_rec, sizeof(bill_rec));
	SObfuscateBuffer(&bill_rec, sizeof(bill_rec)); // @v11.1.1 
	// @v11.1.1 IdeaRandMem(&ar_rec,   sizeof(ar_rec));
	SObfuscateBuffer(&ar_rec,   sizeof(ar_rec)); // @v11.1.1 
	pw.Obfuscate();
	return ok;
}

int PPUnchainDatabase(const char * pPassword) { return ProcessDatabaseChain(BillObj, PPRef, pdbcmUnchain, pPassword, 0, 0, 1); }
int PPCheckDatabaseChain() { return ProcessDatabaseChain(BillObj, PPRef, pdbcmVerify, 0, 0, 0, 1); }

int PPReEncryptDatabaseChain(PPObjBill * pBObj, Reference * pRef, const char * pSrcEncPw, const char * pDestEncPw, int use_ta)
{
	return ProcessDatabaseChain(pBObj, pRef, pdbcmReEncrypt, 0, pSrcEncPw, pDestEncPw, use_ta);
}
//
//
//
DbqStringSubst::DbqStringSubst(size_t numItems) : NumItems(numItems), IsInited(0)
{
	Items = new Subst[NumItems];
	memzero(Items, sizeof(Subst) * NumItems);
	P_Items = new Subst*[NumItems];
	memzero(P_Items, sizeof(Subst*) * NumItems);
}

DbqStringSubst::~DbqStringSubst()
{
	delete [] Items;
	delete [] P_Items;
}

void FASTCALL DbqStringSubst::Init(uint strID)
{
	if(!IsInited) {
		ENTER_CRITICAL_SECTION
		if(!IsInited) {
			{
				const size_t str_size = sizeof(static_cast<Subst *>(0)->Str);
				uint8 * p_items_buf = reinterpret_cast<uint8 *>(Items);
				void ** pp_items = reinterpret_cast<void **>(P_Items);
				char   item_buf[256];
				char   temp_buf[32];
				for(uint idx = 0; idx < NumItems; idx++) {
					long   id = 0;
					char * p = 0;
					if(PPGetSubStr(strID, idx, item_buf, sizeof(item_buf)) > 0) {
						if((p = sstrchr(item_buf, ',')) != 0) {
							*p++ = 0;
							id = atol(item_buf);
						}
						else {
							p = item_buf;
							id = idx + 1;
						}
					}
					else {
						id = idx+1;
						itoa(id, temp_buf, 10);
						p = temp_buf;
					}
					uint8 * ptr = p_items_buf+(sizeof(int16)+str_size)*idx;
					*reinterpret_cast<int16 *>(ptr) = (int16)id;
					strnzcpy(reinterpret_cast<char *>(ptr+sizeof(int16)), p, str_size);
					pp_items[idx] = ptr;
				}
			}
		}
		LEAVE_CRITICAL_SECTION
	}
}

char ** FASTCALL DbqStringSubst::Get(uint strID)
{
	Init(strID);
	return reinterpret_cast<char **>(P_Items);
}
//
//
//
int AdjustPeriodToSubst(SubstGrpDate sgd, DateRange * pPeriod)
{
	DateRange p = *pPeriod;
	DateRange low, upp;
	if(ExpandSubstDate(sgd, p.low, &low) > 0 && ExpandSubstDate(sgd, p.upp, &upp) > 0) {
		p.low = low.low;
		p.upp = upp.upp;
		*pPeriod = p;
		return 1;
	}
	return -1;
}

int ShrinkSubstDateExt(SubstGrpDate sgd, LDATE orgDt, LTIME orgTm, LDATE * pDestDt, LTIME * pDestTm)
{
	int    ok = 1;
	if(sgd == sgdHour) {
		int    h = orgTm.hour();
		int    m = orgTm.minut();
		LTIME tm;
		if(m < 15) m = 0;
		else if(m < 30)	m = 15;
		else if(m < 45)	m = 30;
		else m = 45;
		tm = encodetime(h, m, 0, 0);
		ASSIGN_PTR(pDestTm, tm);
	}
	else
		ok = ShrinkSubstDate(sgd, orgDt, pDestDt);
	return ok;
}

int ShrinkSubstDate(SubstGrpDate sgd, LDATE orgDt, LDATE * pDestDt)
{
	int    ok = 1;
	LDATE  dt = orgDt;
	if(sgd) {
		int    d, m, y;
		decodedate(&d, &m, &y, &orgDt);
		if(sgd == sgdMonth)
			encodedate(1, m, y, &dt);
		else if(sgd == sgdQuart)
			encodedate(1, ((m - 1) / 3) * 3 + 1, y, &dt);
		else if(sgd == sgdYear)
			encodedate(1, 1, y, &dt);
		else if(sgd == sgdWeek) {
			int dow = dayofweek(&orgDt, 1);
			dt = (dow > 1) ? plusdate(orgDt, -(dow-1)) : orgDt;
		}
		else if(sgd == sgdWeekDay)
			dt.v = dayofweek(&orgDt, 1);
		else
			ok = -1;
	}
	else
		ok = -1;
	ASSIGN_PTR(pDestDt, dt);
	return ok;
}

int ExpandSubstDateExt(SubstGrpDate sgd, LDATE dt, LTIME tm, DateRange * pPeriod, TimeRange * pTmPeriod)
{
	int    ok = 1;
	if(sgd == sgdHour) {
		int    h1 = tm.hour();
		int    m1 = tm.minut();
		int    h2 = h1;
		int    m2 = 0;
		TimeRange tm_prd;
		if(m1 < 15) {
			m1 = 0;
			m2 = 15;
		}
		else if(m1 < 30) {
			m1 = 15;
			m2 = 30;
		}
		else if(m1 < 45) {
			m1 = 30;
			m2 = 45;
		}
		else {
			m1 = 45;
			m2 = 0;
			h2 = (h1 < 23) ? (h1 + 1) : 0;
		}
		tm_prd.low = encodetime(h1, m1, 0, 0);
		tm_prd.upp = encodetime(h2, m2, 0, 0);
		ASSIGN_PTR(pTmPeriod, tm_prd);
	}
	else
		ok = ExpandSubstDate(sgd, dt, pPeriod);
	return ok;
}

int ExpandSubstDate(SubstGrpDate sgd, LDATE dt, DateRange * pPeriod)
{
	int    ok = 1;
	DateRange period;
	period.SetDate(dt);
	if(sgd) {
		int    d, m, y;
		decodedate(&d, &m, &y, &dt);
		if(sgd == sgdMonth) {
			encodedate(1, m, y, &period.low);
			encodedate(dayspermonth(m, y), m, y, &period.upp);
		}
		else if(sgd == sgdQuart) {
			m = ((m - 1) / 3) * 3 + 1;
			encodedate(1, m, y, &period.low);
			encodedate(dayspermonth(m+2, y), m+2, y, &period.upp);
		}
		else if(sgd == sgdYear) {
			encodedate(1, m, y, &period.low);
			encodedate(dayspermonth(12, y), 12, y, &period.upp);
		}
		else if(sgd == sgdWeekDay) {
			RVALUEPTR(period, pPeriod);
		}
		else if(sgd == sgdWeek) {
			int dow = dayofweek(&dt, 1);
			period.low = (dow > 1) ? plusdate(dt, -(dow-1)) : dt;
			period.upp = (dow < 7) ? plusdate(dt, 7-dow) : dt;
		}
		else
			ok = -1;
	}
	else
		ok = -1;
	ASSIGN_PTR(pPeriod, period);
	return ok;
}

void FormatSubstDateExt(SubstGrpDate sgd, LDATE dt, LTIME tm, SString & rBuf, long dtFmt /*=0*/, long tmFmt /*=0*/)
{
	char   buf[256];
	memzero(buf, sizeof(buf));
	if(sgd == sgdHour) {
		const long format = (tmFmt == 0) ? TIMF_HM : tmFmt;
		int    h = tm.hour();
		int    m = tm.minut();
		rBuf.Cat(tm, format).Dot().Dot();
		if(m < 15) m = 15;
		else if(m < 30) m = 30;
		else if(m < 45) m = 45;
		else {
			h = (h < 23) ? h + 1 : 0;
			m = 0;
		}
		rBuf.Cat(encodetime(h, m, 0, 0), format);
	}
	else {
		FormatSubstDate(sgd, dt, buf, sizeof(buf));
		rBuf = buf;
	}
}

void FormatSubstDate(SubstGrpDate sgd, LDATE dt, SString & rBuf, long fmt /*=0*/)
{
	char   buf[256];
	memzero(buf, sizeof(buf));
	FormatSubstDate(sgd, dt, buf, sizeof(buf));
	rBuf = buf;
}

void FormatSubstDate(SubstGrpDate sgd, LDATE dt, char * pBuf, size_t bufLen, long fmt /*=0*/)
{
	char   temp[128];
	char * p = temp;
	SString temp_buf;
	int    d = 0, m = 0, y = 0;
	long format = (fmt) ? fmt : DATF_DMY|DATF_CENTURY;
	decodedate(&d, &m, &y, &dt);
	if(sgd == sgdMonth) {
		//p = p + sstrlen(getMonthText(m, MONF_SHORT | MONF_OEM, temp));
		//*p++ = ' ';
		//itoa(y, p, 10);
		SGetMonthText(m, MONF_SHORT|MONF_OEM, temp_buf);
		temp_buf.Space().Cat(y);
		STRNSCPY(temp, temp_buf);
	}
	else if(sgd == sgdQuart) {
		int q = (m-1) / 3;
		if(q == 0)
			*p++ = 'I';
		else if(q == 1) { *p++ = 'I'; *p++ = 'I'; }
		else if(q == 2) { *p++ = 'I'; *p++ = 'I'; *p++ = 'I'; }
		else if(q == 3) { *p++ = 'I'; *p++ = 'V'; }
		*p++ = ' ';
		itoa(y, p, 10);
	}
	else if(sgd == sgdYear)
		itoa(y, temp, 10);
	else if(sgd == sgdWeekDay) {
		format = (format >= 1 && format <= 4) ? format : 3;
		GetDayOfWeekText(format, dt.v, temp_buf);
		temp_buf.ToOem().CopyTo(temp, sizeof(temp));
	}
	else
		datefmt(&dt, format, temp);
	strnzcpy(pBuf, temp, bufLen);
}

static SString & CDECL Helper_PPFormat(const SString & rFmt, SString * pBuf, /*...*/va_list pArgList)
{
	enum {
		tokInt64 = 1, // int64
		tokInt,       // int
		tokHex,       // hex
		tokReal,      // real
		tokSStr,      // sstr
		tokZStr,      // zstr
		tokDate,      // date
		tokTime,      // time
		tokDateTime,  // datetime
		tokPerson,    // person
		tokGoods,     // goods
		tokUnit,      // unit
		tokAccount,   // acc
		tokArticle,   // article
		tokLoc,       // loc
		tokBill,      // bill
		tokPrc,       // prc
		tokTech,      // tech
		tokTSess,     // tsess
		tokPeriod,    // period
		tokSalCharge, // salcharge
		tokPPErr,     // pperr
		tokStaffCal,  // staffcal
		tokPsnOpKind, // psnopkind
		tokDebtDim,   // debtdim
		tokQuotKind,  // quotkind
		tokSCard,     // scard
		tokTag,       // tag
		tokObjTitle,  // objtitle
		tokLocAddr,   // locaddr
	};
	//va_list arg_list;
	//va_start(arg_list, pBuf);

	PPSymbTranslator st(PPSSYM_FORMAT);
	SString temp_buf, buf;
	SStrScan scan(rFmt);
	int    c = *scan;
	if(c) {
		do {
			if(c == '@' && scan[1] != '{') {
				scan.Incr();
				long   sym  = st.Translate(scan);
				PPID   obj_id = 0;
				switch(sym) {
					case tokInt64: buf.Cat(va_arg(pArgList, int64)); break;
					case tokInt: buf.Cat(va_arg(pArgList, long)); break;
					case tokHex: buf.Cat(va_arg(pArgList, long)); break; // @todo HEX
					case tokReal: buf.Cat(va_arg(pArgList, double), MKSFMTD(0, 6, NMBF_NOTRAILZ)); break;
					case tokSStr: buf.Cat(va_arg(pArgList, SString)); break;
					case tokZStr: buf.Cat(va_arg(pArgList, const char *)); break;
					case tokDate: buf.Cat(va_arg(pArgList, LDATE)); break;
					case tokPeriod: buf.Cat(va_arg(pArgList, DateRange), 1); break;
					case tokTime: buf.Cat(va_arg(pArgList, LTIME)); break;
					case tokDateTime: buf.Cat(va_arg(pArgList, LDATETIME)); break;
						//
						//
						//
					case tokPerson:
						obj_id = va_arg(pArgList, PPID);
						GetPersonName(obj_id, temp_buf);
						buf.Cat(temp_buf);
						break;
					case tokGoods:
						obj_id = va_arg(pArgList, PPID);
						GetGoodsName(obj_id, temp_buf);
						buf.Cat(temp_buf);
						break;
					case tokUnit:
						obj_id = va_arg(pArgList, PPID);
						{
							PPObjUnit unit_obj;
							PPUnit unit_rec;
							if(unit_obj.Fetch(obj_id, &unit_rec) > 0)
								buf.Cat(unit_rec.Name);
							else
								ideqvalstr(obj_id, buf);
						}
						break;
					case tokAccount:
						obj_id = va_arg(pArgList, PPID);
						CatObjectName(PPOBJ_ACCOUNT2, obj_id, buf);
						break;
					case tokArticle:
						obj_id = va_arg(pArgList, PPID);
						GetArticleName(obj_id, temp_buf);
						buf.Cat(temp_buf);
						break;
					case tokDebtDim:
						obj_id = va_arg(pArgList, PPID);
						{
							PPObjDebtDim dd_obj;
							PPDebtDim dd_rec;
							if(dd_obj.Fetch(obj_id, &dd_rec) > 0)
								buf.Cat(dd_rec.Name);
							else
								ideqvalstr(obj_id, buf);
						}
						break;
					case tokQuotKind:
						obj_id = va_arg(pArgList, PPID);
						{
							PPObjQuotKind qk_obj;
							PPQuotKindPacket qk_pack;
							if(qk_obj.Fetch(obj_id, &qk_pack) > 0)
								buf.Cat(qk_pack.Rec.Name);
							else
								ideqvalstr(obj_id, buf);
						}
						break;
					case tokLoc:
						obj_id = va_arg(pArgList, PPID);
						GetLocationName(obj_id, temp_buf);
						buf.Cat(temp_buf);
						break;
					case tokLocAddr:
						{
							size_t preserve_buf_len = buf.Len();
							obj_id = va_arg(pArgList, PPID);
							PPObjLocation loc_obj;
							LocationTbl::Rec loc_rec;
							if(loc_obj.Fetch(obj_id, &loc_rec) > 0) {
								buf.Cat(loc_rec.Name);
								LocationCore::GetAddress(loc_rec, 0, temp_buf.Z());
								if(temp_buf.NotEmptyS())
									buf.CatDivIfNotEmpty(';', 2).Cat(temp_buf);
							}
							if(buf.Len() == preserve_buf_len)
								ideqvalstr(obj_id, buf);
						}
						break;
					case tokBill:
						obj_id = va_arg(pArgList, PPID);
						CatObjectName(PPOBJ_BILL, obj_id, buf);
						break;
					case tokPrc:
						obj_id = va_arg(pArgList, PPID);
						{
							PPObjProcessor prc_obj;
							ProcessorTbl::Rec prc_rec;
							if(prc_obj.Fetch(obj_id, &prc_rec) > 0)
								buf.Cat(prc_rec.Name);
							else
								ideqvalstr(obj_id, buf);
						}
						break;
					case tokTech:
						obj_id = va_arg(pArgList, PPID);
						{
							PPObjTech tec_obj;
							TechTbl::Rec tec_rec;
							if(tec_obj.Fetch(obj_id, &tec_rec) > 0)
								buf.Cat(tec_rec.Code);
							else
								ideqvalstr(obj_id, buf);
						}
						break;
					case tokTSess:
						obj_id = va_arg(pArgList, PPID);
						CatObjectName(PPOBJ_TSESSION, obj_id, buf);
						break;
					case tokSalCharge:
						obj_id = va_arg(pArgList, PPID);
						{
							PPObjSalCharge sc_obj;
							PPSalChargePacket sc_pack;
							if(sc_obj.Fetch(obj_id, &sc_pack) > 0)
								buf.Cat(sc_pack.Rec.Name);
							else
								ideqvalstr(obj_id, buf);
						}
						break;
					case tokStaffCal:
						obj_id = va_arg(pArgList, PPID);
						{
							PPObjStaffCal sc_obj;
							PPStaffCal sc_rec;
							if(sc_obj.Fetch(obj_id, &sc_rec) > 0)
								buf.Cat(sc_rec.Name);
							else
								ideqvalstr(obj_id, buf);
						}
						break;
					case tokPsnOpKind:
						obj_id = va_arg(pArgList, PPID);
						{
							PPObjPsnOpKind pok_obj;
							PPPsnOpKind pok_rec;
							if(pok_obj.Fetch(obj_id, &pok_rec) > 0)
								buf.Cat(pok_rec.Name);
							else
								ideqvalstr(obj_id, buf);
						}
						break;
					case tokSCard:
						obj_id = va_arg(pArgList, PPID);
						{
							PPObjSCard sc_obj;
							SCardTbl::Rec sc_rec;
							if(sc_obj.Fetch(obj_id, &sc_rec) > 0) {
								buf.Cat(sc_rec.Code);
							}
							else
								ideqvalstr(obj_id, buf);
						}
						break;
					case tokTag:
						obj_id = va_arg(pArgList, PPID);
						{
							PPObjTag tag_obj;
							PPObjectTag tag_rec;
							if(tag_obj.Fetch(obj_id, &tag_rec) > 0) {
								buf.Cat(tag_rec.Name);
							}
							else
								ideqvalstr(obj_id, buf);
						}
						break;
					case tokObjTitle:
						obj_id = va_arg(pArgList, PPID);
						buf.Cat(GetObjectTitle(obj_id, temp_buf.Z()));
						break;
					case tokPPErr:
						PPGetMessage(mfError, va_arg(pArgList, int), 0, 1, temp_buf);
						buf.Cat(temp_buf);
						break;
					default:
						scan.Get(temp_buf);
						buf.Cat(temp_buf);
						break;
				}
				if(scan.Len)
					scan.IncrLen();
				else
					scan.Incr();
			}
			else {
				buf.CatChar(c);
				scan.Incr();
			}
			c = *scan;
		} while(c);
	}
	//va_end(arg_list);
	ASSIGN_PTR(pBuf, buf);
	return *pBuf;
}

SString & CDECL PPFormat(const SString & rFmt, SString * pBuf, ...)
{
	va_list arg_list;
	va_start(arg_list, pBuf);
	Helper_PPFormat(rFmt, pBuf, arg_list);
	va_end(arg_list);
	return *pBuf;
}

SString & CDECL PPFormatT(int textCode, SString * pBuf, ...)
{
	SString fmt_buf;
	PPLoadText(textCode, fmt_buf);
	va_list arg_list;
	va_start(arg_list, pBuf);
	Helper_PPFormat(fmt_buf, pBuf, arg_list);
	va_end(arg_list);
	return *pBuf;
}

SString & CDECL PPFormatS(int textGroup, int textCode, SString * pBuf, ...)
{
	SString fmt_buf;
	PPLoadString(textGroup, textCode, fmt_buf);
	va_list arg_list;
	va_start(arg_list, pBuf);
	Helper_PPFormat(fmt_buf, pBuf, arg_list);
	va_end(arg_list);
	return *pBuf;
}

int WaitForExists(const char * pPath, int whileExists /* = 1 */, int notifyTimeout /* = 5000 */)
{
	int    ok = 1;
	int    stop = 0;
	if(pPath) {
		const bool exists = fileExists(pPath);
		if((exists && whileExists) || (!exists && !whileExists)) {
			DirChangeNotification * p_dc_notify = 0;
			SString path(pPath);
			SFsPath paths(path);
			if(paths.Nam.Len() == 0) {
				path.RmvLastSlash().Dot();
				paths.Split(path);
			}
			paths.Merge(0, SFsPath::fNam|SFsPath::fExt, (path = 0));
			path.SetLastSlash();
			p_dc_notify = new DirChangeNotification(path, 0, FILE_NOTIFY_CHANGE_FILE_NAME);
			while(!stop) {
				if(p_dc_notify->Wait(notifyTimeout) > 0) {
					stop = fileExists(pPath) ? (whileExists ? 0 : 1) : (whileExists ? 1 : 0);
					if(!stop)
						p_dc_notify->Next();
				}
				else
					stop = fileExists(pPath) ? (whileExists ? 0 : 1) : (whileExists ? 1 : 0);
				if(!PPCheckUserBreak()) {
					ok = -1;
					stop = 1;
				}
			}
			ZDELETE(p_dc_notify);
		}
	}
	else
		ok = 0;
	return ok;
}

int WaitNewFile(const char * pDir, SString & rFile, int notifyTimeout /* =5000 */)
{
	int    ok = 1;
	int    stop = 0;
	if(pDir) {
		LDATETIME beg_dtm;
		SString path;
		DirChangeNotification * p_dc_notify = 0;
		path.CopyFrom(pDir).SetLastSlash().Cat("*.*");
		getcurdatetime(&beg_dtm);
		p_dc_notify = new DirChangeNotification(pDir, 0, FILE_NOTIFY_CHANGE_FILE_NAME);
		while(!stop) {
			if(p_dc_notify->Wait(notifyTimeout) > 0) {
				SDirEntry entry;
				SDirec sdir(path);
				while(sdir.Next(&entry) > 0) {
					LDATETIME iter_dtm;
					iter_dtm.SetNs100(entry.AccsTm_);
					if(!entry.IsFolder() && cmp(iter_dtm, beg_dtm) > 0) {
						entry.GetNameA(pDir, rFile);
						stop = ok = 1;
					}
				}
				if(!stop)
					p_dc_notify->Next();
			}
			if(!PPCheckUserBreak()) {
				ok = -1;
				stop = 1;
			}
		}
		ZDELETE(p_dc_notify);
	}
	else
		ok = 0;
	return ok;
}

int __CopyFileByPath(const char * pSrcPath, const char * pDestPath, const char * pFileName)
{
	SString src, dest;
	(src = pSrcPath).SetLastSlash().Cat(pFileName);
	(dest = pDestPath).SetLastSlash().Cat(pFileName);
	return fileExists(src) ? copyFileByName(src, dest) : PPSetError(PPERR_NOSRCFILE, src);
}

int CopyDataStruct(const char * pSrc, const char * pDest, const char * pFileName)
{
	return __CopyFileByPath(pSrc, pDest, pFileName) ? 1 : PPErrorZ();
}

int PPValidateXml()
{
    int    ok = -1;
    SString xsd_file_name;
    SString xml_file_name;
    SString msg_buf;
    PPLogger logger;
    TDialog * dlg = new TDialog(DLG_XMLVLD);
    THROW(CheckDialogPtr(&dlg));
	FileBrowseCtrlGroup::Setup(dlg, CTLBRW_XMLVLD_XSDFILE, CTL_XMLVLD_XSDFILE, 1, 0, PPTXT_FILPAT_XSD,
		FileBrowseCtrlGroup::fbcgfFile|FileBrowseCtrlGroup::fbcgfSaveLastPath);
	dlg->SetupInputLine(CTL_XMLVLD_XSDFILE, MKSTYPE(S_ZSTRING, 1024), MKSFMT(1024, 0));
	FileBrowseCtrlGroup::Setup(dlg, CTLBRW_XMLVLD_XMLFILE, CTL_XMLVLD_XMLFILE, 2, 0, PPTXT_FILPAT_XML,
		FileBrowseCtrlGroup::fbcgfFile|FileBrowseCtrlGroup::fbcgfSaveLastPath);
	dlg->SetupInputLine(CTL_XMLVLD_XMLFILE, MKSTYPE(S_ZSTRING, 1024), MKSFMT(1024, 0));
    while(ExecView(dlg) == cmOK) {
        dlg->getCtrlString(CTL_XMLVLD_XSDFILE, xsd_file_name);
        dlg->getCtrlString(CTL_XMLVLD_XMLFILE, xml_file_name);
        if(!fileExists(xsd_file_name))
			PPError(PPERR_SLIB);
		else if(!fileExists(xml_file_name))
			PPError(PPERR_SLIB);
		else {
			SXmlValidationMessageList vlist;
			SXml::Validate(xsd_file_name, xml_file_name, &vlist);
            if(vlist.GetMessageCount()) {
				for(uint i = 0; i < vlist.GetMessageCount(); i++) {
					int msg_type = 0;
					vlist.GetMessageByIdx(i, &msg_type, msg_buf);
					logger.Log(msg_buf);
				}
            }
		}
    }
    CATCHZOK
    delete dlg;
	return ok;
}

#if 0 // { replaced by XMLWriteSpecSymbEntities(void *)
int XMLFillDTDEntitys(void * pWriter)
{
	// amp$&#38;#38;.exclam$&#33;.sharp$&#35;.pct$&#37;.apostr$&#39;.comma$&#44;.dot$&#46;.fwsl$&#47;.lt$&#38;#60;.\
	// eq$&#61;.gt$&#62;.ques$&#63;.lsq$&#91;.bksl$&#92;.rsq$&#93;"
	int    ok = 1;
	SString temp_buf, ent, subst;
	if(pWriter && PPLoadText(PPTXT_XMLSYMBENTITIES, temp_buf)) {
		StringSet ss('.', temp_buf);
		for(uint i = 0; ss.get(&i, temp_buf);) {
			temp_buf.Divide('$', ent, subst);
			xmlTextWriterWriteDTDEntity((xmlTextWriter *)pWriter, 0, ent.ucptr(), 0, 0, 0, subst.ucptr());
		}
	}
	else
		ok = 0;
	return ok;
}
#endif // } 0
//
//
//
/*
Проверка правильности указания номера страхования свидетельства ПФ:

Алгоритм проверки номера свидетельства страхования ПФ:
1. Вычисляется контрольная сумма по первым 9-ти цифрам со следующими весовыми коэффициентами: (9,8,7,6,5,4,3,2,1).
2. Вычисляется контрольное число как остаток от деления контрольной суммы на 101
3. Контрольное число сравнивается с двумя последними цифрами номера страхования свидетельства ПФ.
	В случае их равенства номер страхования свидетельства ПФ считается правильным.

*/
// not implemented yet
// 
// Проверка правильности указания корреспондентского счёта:
// 
// Алгоритм проверки корреспондентского счёта с помощью БИКа банка:
// 1. Для проверки контрольной суммы перед корреспондентским счётом добавляются "0" и два знака БИКа банка, начиная с пятого знака.
// 2. Вычисляется контрольная сумма со следующими весовыми коэффициентами: (7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1)
// 3. Вычисляется контрольное число как остаток от деления контрольной суммы на 10
// 4. Контрольное число сравнивается с нулём. В случае их равенства корреспондентский счёт считается правильным.
// 
int CheckCorrAcc(const char * pCode, const char * pBic)
{
	int    ok = 0;
	const size_t len = sstrlen(pCode);
	const size_t bic_len = sstrlen(pBic);
	if(len == 20 && bic_len >= 6) {
		int    r = 1;
		size_t i;
		for(i = 0; r && i < len; i++) {
			if(!isdec(pCode[i]))
				r = 0;
		}
		if(r) {
			static const int8 w[] = {7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1};
			ulong  sum = 0;
			sum += w[0] * 0;
			sum += w[1] * (pBic[4]-'0');
			sum += w[2] * (pBic[5]-'0');
			for(i = 0; i < len; i++) {
				sum += (w[i+3] * (pCode[i]-'0'));
			}
			ok = BIN((sum % 10) == 0);
		}
	}
	return ok;
}
// 
// Проверка правильности указания расчётного счёта:
// 
// Алгоритм проверки расчётного счёта с помощью БИКа банка:
// 1. Для проверки контрольной суммы перед расчётным счётом добавляются три последние цифры БИКа банка.
// 2. Вычисляется контрольная сумма со следующими весовыми коэффициентами: (7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1)
// 3. Вычисляется контрольное число как остаток от деления контрольной суммы на 10
// 4. Контрольное число сравнивается с нулём. В случае их равенства расчётного счёт считается правильным.
// 
int CheckBnkAcc(const char * pCode, const char * pBic)
{
	int    ok = 0;
	const size_t len = sstrlen(pCode);
	const size_t bic_len = sstrlen(pBic);
	if(len == 20 && bic_len >= 3) {
		int    r = 1;
		size_t i;
		for(i = 0; r && i < len; i++) {
			if(!isdec(pCode[i]))
				r = 0;
		}
		if(r) {
			static const int8 w[] = {7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1};
			ulong  sum = 0;
			sum += w[0] * (pBic[bic_len-3]-'0');
			sum += w[1] * (pBic[bic_len-2]-'0');
			sum += w[2] * (pBic[bic_len-1]-'0');
			for(i = 0; i < len; i++) {
				sum += (w[i+3] * (pCode[i]-'0'));
			}
			ok = BIN((sum % 10) == 0);
		}
	}
	return ok;
}
/*
	Проверка правильности указания ИНН:

	Для 10-ти и для 12-ти значного ИНН существуют разные алгоритмы проверки правильности указания ИНН.

	Для 10-ти значного ИНН алгоритм проверки выглядит следующим образом:
	1. Вычисляется контрольная сумма со следующими весовыми коэффициентами: (2,4,10,3,5,9,4,6,8,0)
	2. Вычисляется контрольное число как остаток от деления контрольной суммы на 11
	3. Если контрольное число больше 9, то контрольное число вычисляется как остаток от деления контрольного числа на 10
	4. Контрольное число проверяется с десятым знаком ИНН. В случае их равенства ИНН считается правильным.

	Для 12-ти значного ИНН алгоритм проверки выглядит следующим образом:
	1. Вычисляется контрольная сумма по 11-ти знакам со
		следующими весовыми коэффициентами: (7,2,4,10,3,5,9,4,6,8,0)
	2. Вычисляется контрольное число(1) как остаток от деления контрольной суммы на 11
	3. Если контрольное число(1) больше 9, то контрольное число(1) вычисляетс
		как остаток от деления контрольного числа(1) на 10
	4. Вычисляется контрольная сумма по 12-ти знакам со
		следующими весовыми коэффициентами: (3,7,2,4,10,3,5,9,4,6,8,0).
	5. Вычисляется контрольное число(2) как остаток от деления контрольной суммы на 11
	6. Если контрольное число(2) больше 9, то контрольное число(2) вычисляется как
		остаток от деления контрольного числа(2) на 10
	7. Контрольное число(1) проверяется с одиннадцатым знаком ИНН и контрольное число(2)
		проверяется с двенадцатым знаком ИНН. В случае их равенства ИНН считается правильным.
*/
//
//
//
PPUhttClient::PPUhttClient() : State(0), P_DestroyFunc(0)
{
	SString temp_buf;
 	{
		PPGetFilePath(PPPATH_BIN, "PPSoapUhtt.dll", temp_buf);
		P_Lib = new SDynLibrary(temp_buf);
		if(P_Lib && !P_Lib->IsValid()) {
			ZDELETE(P_Lib);
		}
		if(P_Lib) {
			P_DestroyFunc = P_Lib->GetProcAddr("UhttDestroyResult");
		}
	}
	PPAlbatrossConfig cfg;
	DS.FetchAlbatrosConfig(&cfg);
	cfg.GetExtStrData(ALBATROSEXSTR_UHTTURLPFX, temp_buf);
	UrlBase = temp_buf.NotEmpty() ? temp_buf.cptr() : 0; //"http://uhtt.ru/UHTTDispatcher/axis/Plugin_UHTT_SOAPService";
	if(UrlBase.IsEmpty())
		State |= stDefaultServer;
	cfg.GetExtStrData(ALBATROSEXSTR_UHTTACC, temp_buf);
	if(temp_buf.NotEmpty())
		State |= stHasAccount;
}

PPUhttClient::~PPUhttClient()
{
}

int PPUhttClient::GetState() const { return State; }

int PPUhttClient::PreprocessResult(const void * pResult, const PPSoapClientSession & rSess)
{
	LastMsg = rSess.GetMsg();
    return BIN(pResult);
}

FARPROC PPUhttClient::GetFuncEntryAndSetupSess(const char * pFuncName, PPSoapClientSession & rSess)
{
	FARPROC func = P_Lib ? P_Lib->GetProcAddr(pFuncName) : 0;
	if(func) {
		bool   is_host_available = true;
		SString host_name;
		rSess.Setup(UrlBase);
		if(UrlBase.NotEmpty()) {
			InetUrl url(UrlBase);
			url.GetComponent(InetUrl::cHost, 0, host_name);
			if(host_name.NotEmpty()) {
				if(!DS.GetHostAvailability(host_name))
					is_host_available = false;
			}
		}
		else {
			host_name = "uhtt.ru";
			if(!DS.GetHostAvailability(host_name))
				is_host_available = false;
		}
		if(!is_host_available) {
			func = 0;
			PPSetError(PPERR_HOSTUNAVAILABLE, host_name);
		}
	}
	return func;
}

int PPUhttClient::Auth()
{
	Token.Z();
	State &= ~stAuth;
	int    ok = 1;
	PPSoapClientSession sess;
	UHTTAUTH_PROC func = reinterpret_cast<UHTTAUTH_PROC>(GetFuncEntryAndSetupSess("UhttAuth", sess));
	if(func) {
		SString temp_buf;
		SString pw;
		PPAlbatrossConfig cfg;
		DS.FetchAlbatrosConfig(&cfg);
		cfg.GetPassword(ALBATROSEXSTR_UHTTPASSW, pw);
		cfg.GetExtStrData(ALBATROSEXSTR_UHTTACC, temp_buf);
		SString * p_token = func(sess, /*cfg.UhttAccount*/temp_buf, pw.Transf(CTRANSF_INNER_TO_UTF8));
		pw.Z().Align(64, ALIGN_RIGHT); // Забиваем пароль пробелами
		if(PreprocessResult(p_token, sess)) {
			Token = *p_token;
			State |= stAuth;
			DestroyResult((void **)&p_token);
		}
		else
			ok = PPSetError(PPERR_UHTTSVCFAULT, LastMsg);
	}
	return ok;
}

int PPUhttClient::GetLocationByID(int id, UhttLocationPacket & rResult)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETLOCATIONBYID_PROC func = reinterpret_cast<UHTTGETLOCATIONBYID_PROC>(GetFuncEntryAndSetupSess("UhttGetLocationByID", sess));
		if(func) {
			UhttLocationPacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rResult = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetLocationByCode(const char * pCode, UhttLocationPacket & rResult)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETLOCATIONBYCODE_PROC func = reinterpret_cast<UHTTGETLOCATIONBYCODE_PROC>(GetFuncEntryAndSetupSess("UhttGetLocationByCode", sess));
		if(func) {
			UhttLocationPacket * p_result = func(sess, Token, pCode);
			if(PreprocessResult(p_result, sess)) {
				rResult = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetLocationListByPhone(const char * pPhone, TSCollection <UhttLocationPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETLOCATIONLISTBYPHONE_PROC func = reinterpret_cast<UHTTGETLOCATIONLISTBYPHONE_PROC>(GetFuncEntryAndSetupSess("UhttGetLocationListByPhone", sess));
		if(func) {
			TSCollection <UhttLocationPacket> * p_result = func(sess, Token, pPhone);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetBrandByName(const char * pName, TSCollection <UhttBrandPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETBRANDBYNAME_PROC func = reinterpret_cast<UHTTGETBRANDBYNAME_PROC>(GetFuncEntryAndSetupSess("UhttGetBrandByName", sess));
		if(func) {
			SString temp_buf;
			(temp_buf = pName).Transf(CTRANSF_INNER_TO_UTF8);
			TSCollection <UhttBrandPacket> * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

//typedef int (*UHTTGETGOODSREFLIST_PROC)(PPSoapClientSession & rSess, const char * pToken, TSArray <UhttCodeRefItem> & rList);

int PPUhttClient::GetUhttGoodsRefList(LAssocArray & rList, StrAssocArray * pByCodeList)
{
	CALLPTRMEMB(pByCodeList, Z());

	int    ok = -1;
	SString temp_buf;
	PPObjGoods goods_obj;
	BarcodeArray bc_list;
	TSArray <UhttCodeRefItem> ref_list;
	{
		for(uint i = 0; i < rList.getCount(); i++) {
			rList.at(i).Val = 0;
			const  PPID goods_id = labs(rList.at(i).Key);
			goods_obj.ReadBarcodes(goods_id, bc_list);
			if(bc_list.getCount()) {
				for(uint j = 0; j < bc_list.getCount(); j++) {
					const BarcodeTbl::Rec & r_bc_item = bc_list.at(j);
					if(sstrlen(r_bc_item.Code) != 19) { // Алкогольные коды пропускаем
						assert(goods_id == r_bc_item.GoodsID); // @paranoic
						UhttCodeRefItem ref_item;
						ref_list.insert(&ref_item.Set(goods_id, r_bc_item.Code));
					}
				}
			}
		}
	}
	if(ref_list.getCount()) {
		if(State & stAuth) {
			PPSoapClientSession sess;
			UHTTGETGOODSREFLIST_PROC func = reinterpret_cast<UHTTGETGOODSREFLIST_PROC>(GetFuncEntryAndSetupSess("UhttGetGoodsRefList", sess));
			if(func) {
				int result = func(sess, Token, ref_list);
				if(PreprocessResult((void *)result, sess)) {
					if(result > 0) {
						for(uint i = 0; i < ref_list.getCount(); i++) {
							const UhttCodeRefItem & r_ref_item = ref_list.at(i);
							if(r_ref_item.UhttID) {
								long su_id = 0;
								uint su_pos = 0;
								if(rList.Search(r_ref_item.PrivateID, &su_id, &su_pos)) {
									rList.at(su_pos).Val = r_ref_item.UhttID;
									CALLPTRMEMB(pByCodeList, Add(r_ref_item.UhttID, r_ref_item.Code, 0 /* dont replace dup */));
									ok = 1;
								}
							}
						}
					}
					else
						ok = -1;
				}
				else
					ok = 0;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetUhttGoodsList(PPID goodsID, long flags, TSCollection <UhttGoodsPacket> & rResult)
{
	int    ok = 1;
	int    uhtt_goods_id = 0;
	SString org_goods_name;
	PPObjGoods goods_obj;
	BarcodeArray bc_list;
	goods_obj.ReadBarcodes(labs(goodsID), bc_list);
	const uint c = bc_list.getCount();
	GetGoodsName(goodsID, org_goods_name);
	THROW_PP_S(c, PPERR_UHTT_LOCALGOODSNHBARCODE, org_goods_name);
	for(uint i = 0; !uhtt_goods_id && i < c; i++) {
		TSCollection <UhttGoodsPacket> uhtt_goods_list;
		if(GetGoodsByCode(bc_list.at(i).Code, uhtt_goods_list)) {
			if(uhtt_goods_list.getCount() == 1) {
				UhttGoodsPacket * p_new_pack = rResult.CreateNewItem();
				THROW_SL(p_new_pack);
				*p_new_pack = *uhtt_goods_list.at(0);
			}
			else if(uhtt_goods_list.getCount() == 0) {
				; // @logwarn
			}
			else {
				; // @logwarn
			}
		}
	}
	THROW_PP_S(rResult.getCount(), PPERR_UHTT_LOCALGOODSSYNCFAIL, org_goods_name);
	CATCHZOK
	return ok;
}

int PPUhttClient::GetUhttGoods(PPID goodsID, long flags, int * pUhttID, UhttGoodsPacket * pUhttResult)
{
	int    ok = 0;
	int    uhtt_goods_id = 0;
	SString temp_buf;
	PPObjGoods goods_obj;
	BarcodeArray bc_list;
	goods_obj.ReadBarcodes(labs(goodsID), bc_list);
	const uint c = bc_list.getCount();
	if(c == 0) {
		GetGoodsName(goodsID, temp_buf);
		PPSetError(PPERR_UHTT_LOCALGOODSNHBARCODE, temp_buf);
	}
	else {
		for(uint i = 0; !uhtt_goods_id && i < c; i++) {
			TSCollection <UhttGoodsPacket> uhtt_goods_list;
			if(GetGoodsByCode(bc_list.at(i).Code, uhtt_goods_list)) {
				if(uhtt_goods_list.getCount() == 1) {
					uhtt_goods_id = uhtt_goods_list.at(0)->ID;
					ASSIGN_PTR(pUhttResult, *uhtt_goods_list.at(0));
					ok = 1;
				}
				else if(uhtt_goods_list.getCount() == 0) {
					; // @logwarn
				}
				else {
					; // @logwarn
				}
			}
		}
		if(!uhtt_goods_id) {
			GetGoodsName(goodsID, temp_buf);
			PPSetError(PPERR_UHTT_LOCALGOODSSYNCFAIL, temp_buf);
		}
	}
	ASSIGN_PTR(pUhttID, uhtt_goods_id);
	return ok;
}

int PPUhttClient::ResolveGoodsByUhttID(int uhttID, UhttGoodsPacket * pUhttResult, PPID * pGoodsID, SString * pCode)
{
	int    ok = -1;
	PPID   goods_id = 0;
	SString resolved_code;
	UhttGoodsPacket uhtt_result;
	if(State & stAuth && P_Lib) {
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		if(GetGoodsByID(uhttID, uhtt_result) > 0) {
			for(uint i = 0; ok < 0 && i < uhtt_result.BarcodeList.getCount(); i++) {
				UhttGoodsPacket::Barcode & r_bc = uhtt_result.BarcodeList.at(i);
				BarcodeTbl::Rec bc_rec;
				if(goods_obj.SearchByBarcode(r_bc.Code, &bc_rec, &goods_rec, 1) > 0) {
					goods_id = goods_rec.ID;
					resolved_code = r_bc.Code;
					ok = 1;
				}
			}
		}
	}
	/*
	CATCHZOK
	*/
	ASSIGN_PTR(pGoodsID, goods_id);
	ASSIGN_PTR(pCode, resolved_code);
	ASSIGN_PTR(pUhttResult, uhtt_result);
	return ok;
}

int PPUhttClient::GetPersonByID(int id, UhttPersonPacket & rResult)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETPERSONBYID_PROC func = reinterpret_cast<UHTTGETPERSONBYID_PROC>(GetFuncEntryAndSetupSess("UhttGetPersonByID", sess));
		if(func) {
			UhttPersonPacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rResult = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetPersonByName(const char * pName, TSCollection <UhttPersonPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETPERSONBYNAME_PROC func = reinterpret_cast<UHTTGETPERSONBYNAME_PROC>(GetFuncEntryAndSetupSess("UhttGetPersonByName", sess));
		if(func) {
			SString temp_buf;
			(temp_buf = pName).Transf(CTRANSF_INNER_TO_UTF8);
			TSCollection <UhttPersonPacket> * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetGoodsByID(int id, UhttGoodsPacket & rResult)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETGOODSBYID_PROC func = reinterpret_cast<UHTTGETGOODSBYID_PROC>(GetFuncEntryAndSetupSess("UhttGetGoodsByID", sess));
		if(func) {
			UhttGoodsPacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rResult = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetGoodsByCode(const char * pCode, TSCollection <UhttGoodsPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETGOODSBYCODE_PROC func = reinterpret_cast<UHTTGETGOODSBYCODE_PROC>(GetFuncEntryAndSetupSess("UhttGetGoodsByCode", sess));
		if(func) {
			TSCollection <UhttGoodsPacket> * p_result = func(sess, Token, pCode);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetGoodsByName(const char * pName, TSCollection <UhttGoodsPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETGOODSBYNAME_PROC func = reinterpret_cast<UHTTGETGOODSBYNAME_PROC>(GetFuncEntryAndSetupSess("UhttGetGoodsByName", sess));
		if(func) {
			SString temp_buf;
			(temp_buf = pName).Transf(CTRANSF_INNER_TO_UTF8);
			TSCollection <UhttGoodsPacket> * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetGoodsArCode(const char * pBarcode, const char * pPersonINN, SString & rArCode)
{
	int    ok = 0;
	rArCode.Z();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETGOODSARCODE_PROC func = reinterpret_cast<UHTTGETGOODSARCODE_PROC>(GetFuncEntryAndSetupSess("UhttGetGoodsArCode", sess));
		if(func) {
			SString bcode, inn;
			(bcode = pBarcode).Transf(CTRANSF_INNER_TO_UTF8);
			(inn = pPersonINN).Transf(CTRANSF_INNER_TO_UTF8);
			SString * p_result = func(sess, Token, bcode, inn);
			if(PreprocessResult(p_result, sess)) {
				rArCode = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetGoodsRestList(int uhttGoodsID, TSCollection <UhttGoodsRestListItem> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETGOODSRESTLIST_PROC func = reinterpret_cast<UHTTGETGOODSRESTLIST_PROC>(GetFuncEntryAndSetupSess("UhttGetGoodsRestList", sess));
		if(func) {
			TSCollection <UhttGoodsRestListItem> * p_result = func(sess, Token, uhttGoodsID);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::CreateStandaloneLocation(long * pID, const UhttLocationPacket & rPack)
{
	int    ok = 0;
	int    id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTCREATESTANDALONELOCATION_PROC func = reinterpret_cast<UHTTCREATESTANDALONELOCATION_PROC>(GetFuncEntryAndSetupSess("UhttCreateStandaloneLocation", sess));
		if(func) {
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int PPUhttClient::CreateGoods(long * pID, const UhttGoodsPacket & rPack)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTCREATEGOODS_PROC func = reinterpret_cast<UHTTCREATEGOODS_PROC>(GetFuncEntryAndSetupSess("UhttCreateGoods", sess));
		if(func) {
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int PPUhttClient::SetObjImage(const char * pObjTypeSymb, PPID uhttObjID, const char * pFileName)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTSETIMAGEBYID_PROC func = reinterpret_cast<UHTTSETIMAGEBYID_PROC>(GetFuncEntryAndSetupSess("UhttSetImageByID", sess));
		if(func) {
			UhttDocumentPacket doc_pack;
			doc_pack.SetFile(pFileName);
			doc_pack.ObjTypeSymb = pObjTypeSymb;
			doc_pack.UhttObjID = uhttObjID;
			UhttStatus * p_result = func(sess, Token, doc_pack);
			if(PreprocessResult(p_result, sess)) {
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

// @Muxa {
int PPUhttClient::GetSpecSeriesByPeriod(const char * pPeriod, TSCollection <UhttSpecSeriesPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETSPECSERIESBYPERIOD_PROC func = reinterpret_cast<UHTTGETSPECSERIESBYPERIOD_PROC>(GetFuncEntryAndSetupSess("UhttGetSpecSeriesByPeriod", sess));
		if(func) {
			SString temp_buf;
			(temp_buf = pPeriod).Transf(CTRANSF_INNER_TO_UTF8);
			TSCollection <UhttSpecSeriesPacket> * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::CreateSpecSeries(long * pID, const UhttSpecSeriesPacket & rPack)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTCREATESPECSERIES_PROC func = reinterpret_cast<UHTTCREATESPECSERIES_PROC>(GetFuncEntryAndSetupSess("UhttCreateSpecSeries", sess));
		if(func) {
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int PPUhttClient::CreateSCard(UhttSCardPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTCREATESCARD_PROC func = reinterpret_cast<UHTTCREATESCARD_PROC>(GetFuncEntryAndSetupSess("UhttCreateSCard", sess));
		if(func) {
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					//id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	return ok;
}

int PPUhttClient::GetSCardByNumber(const char * pNumber, UhttSCardPacket & rResult)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETSCARDBYNUMBER_PROC func = reinterpret_cast<UHTTGETSCARDBYNUMBER_PROC>(GetFuncEntryAndSetupSess("UhttGetSCardByNumber", sess));
		if(func) {
			SString temp_buf;
			(temp_buf = pNumber).Transf(CTRANSF_INNER_TO_UTF8);
			UhttSCardPacket * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				rResult = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
			else
				PPSetError(PPERR_UHTTSVCFAULT, LastMsg);
		}
	}
	return ok;
}

int PPUhttClient::CreateBill(UhttBillPacket & rPack)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTCREATEBILL_PROC func = reinterpret_cast<UHTTCREATEBILL_PROC>(GetFuncEntryAndSetupSess("UhttCreateBill", sess));
		if(func) {
			int result = func(sess, Token, rPack);
			if(PreprocessResult((void *)result, sess))
				ok = 1;
			else {
				PPSetError(PPERR_UHTTSVCFAULT, LastMsg);
			}
		}
	}
	return ok;
}

int PPUhttClient::GetBill(const UhttBillFilter & rFilt, TSCollection <UhttBillPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETBILL_PROC func = reinterpret_cast<UHTTGETBILL_PROC>(GetFuncEntryAndSetupSess("UhttGetBill", sess));
		if(func) {
			TSCollection <UhttBillPacket> * p_result = func(sess, Token, rFilt);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetQuot(const UhttQuotFilter & rFilt, TSCollection <UhttQuotPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETQUOT_PROC func = reinterpret_cast<UHTTGETQUOT_PROC>(GetFuncEntryAndSetupSess("UhttGetQuot", sess));
		if(func) {
			TSCollection <UhttQuotPacket> * p_result = func(sess, Token, rFilt);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::SetQuot(const UhttQuotPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTSETQUOT_PROC func = reinterpret_cast<UHTTSETQUOT_PROC>(GetFuncEntryAndSetupSess("UhttSetQuot", sess));
		if(func) {
			ok = func(sess, Token, rPack);
			if(!PreprocessResult((void *)ok, sess)) {
				PPSetError(PPERR_UHTTSVCFAULT, LastMsg);
			}
		}
	}
	return ok;
}

int PPUhttClient::SetQuotList(const TSCollection <UhttQuotPacket> & rList, TSCollection <UhttStatus> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTSETQUOTLIST_PROC func = reinterpret_cast<UHTTSETQUOTLIST_PROC>(GetFuncEntryAndSetupSess("UhttSetQuotList", sess));
		if(func) {
			TSCollection <UhttStatus> * p_result = func(sess, Token, rList);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
			else
				PPSetError(PPERR_UHTTSVCFAULT, LastMsg);
		}
	}
	return ok;
}

int PPUhttClient::CreateSCardCheck(const char * pLocSymb, const char * pSCardNumber, const UhttCheckPacket & rPack)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTCREATESCARDCHECK_PROC func = reinterpret_cast<UHTTCREATESCARDCHECK_PROC>(GetFuncEntryAndSetupSess("UhttCreateSCardCheck", sess));
		if(func) {
			UhttStatus * p_result = func(sess, Token, pLocSymb, pSCardNumber, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	return ok;
}

int PPUhttClient::DepositSCardAmount(const char * pNumber, const double amount)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTDEPOSITSCARDAMOUNT_PROC func = reinterpret_cast<UHTTDEPOSITSCARDAMOUNT_PROC>(GetFuncEntryAndSetupSess("UhttDepositSCardAmount", sess));
		if(func) {
			UhttStatus * p_result = func(sess, Token, pNumber, amount);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					ok = 1;
				}
				else {
					PPSetError(PPERR_UHTTSVCFAULT, (LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER));
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	return ok;
}

int PPUhttClient::WithdrawSCardAmount(const char * pNumber, const double amount)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTWITHDRAWSCARDAMOUNT_PROC func = reinterpret_cast<UHTTWITHDRAWSCARDAMOUNT_PROC>(GetFuncEntryAndSetupSess("UhttWithdrawSCardAmount", sess));
		if(func) {
			UhttStatus * p_result = func(sess, Token, pNumber, amount);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					ok = 1;
				}
				else {
					PPSetError(PPERR_UHTTSVCFAULT, (LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER));
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	return ok;
}

int PPUhttClient::GetSCardRest(const char * pNumber, const char * pDate, double & rRest)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETSCARDREST_PROC func = reinterpret_cast<UHTTGETSCARDREST_PROC>(GetFuncEntryAndSetupSess("UhttGetSCardRest", sess));
		if(func) {
			int    result = func(sess, Token, pNumber, pDate, rRest);
			if(PreprocessResult((void *)BIN(result == 1), sess))
				ok = 1;
		}
	}
	return ok;
}

// } @Muxa

int PPUhttClient::FileVersionAdd(const char * pFileName, const char * pKey,
	const char * pVersionLabel, const char * pVersionMemo, SDataMoveProgressProc pp, void * pExtra)
{
	int    ok = 1;
	int    transfer_id = 0;
	int    pp_reply = 0;
	int    pp_quite = 0;
	int    pp_cancel = 0;
	SDataMoveProgressInfo scfd;
	SString dest_display_name; // Текст, обозначающий место назначения отправки файла (для процедуры SDataMoveProgressProc)
	THROW_SL(fileExists(pFileName));
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTADDFILEVERSION_PROC func = reinterpret_cast<UHTTADDFILEVERSION_PROC>(P_Lib->GetProcAddr("UhttAddFileVersion"));
		if(func) {
			SString temp_buf;
			int64 file_size = 0;
			SFile f(pFileName, SFile::mRead|SFile::mBinary);
			THROW_SL(f.IsValid());
			THROW_SL(f.CalcSize(&file_size));
			{
				SFsPath ps;
				const size_t chunk_size = (size_t)MIN((3*64*1024), file_size);
				const size_t tail_size = (size_t)(file_size % chunk_size);
				const size_t chunk_count = (size_t)(file_size / chunk_size); // Количество отрезков одинакового размера (без хвоста)
				STempBuffer buffer(chunk_size);
				THROW_SL(buffer.IsValid());
				ps.Split(pFileName);
				ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
				temp_buf.ToUtf8();
				THROW(StartTransferData(temp_buf, file_size, (int32)(chunk_count + BIN(tail_size)), &transfer_id));
				assert(tail_size < chunk_size);
				//
				dest_display_name = pKey;
				scfd.P_Src  = pFileName;
				scfd.P_Dest = dest_display_name;
				scfd.SizeTotal = file_size;
				scfd.SizeDone  = 0UL;
				scfd.ExtraPtr = pExtra;
				if(!pp_quite && pp) {
					pp_reply = pp(&scfd);
					if(oneof2(pp_reply, SPRGRS_CANCEL, SPRGRS_STOP))
						pp_cancel = 1;
					else if(pp_reply == SPRGRS_QUITE)
						pp_quite = 1;
				}
				if(!pp_cancel) {
					for(size_t i = 0; !pp_cancel && i < chunk_count; i++) {
						assert(buffer.GetSize() == chunk_size);
						THROW_SL(f.ReadV(buffer, chunk_size));
						THROW(TransferData(transfer_id, (int)(i+1), chunk_size, buffer));
						if(!pp_quite && pp) {
							scfd.SizeDone += chunk_size;
							pp_reply = pp(&scfd);
							if(oneof2(pp_reply, SPRGRS_CANCEL, SPRGRS_STOP))
								pp_cancel = 1;
							else if(pp_reply == SPRGRS_QUITE)
								pp_quite = 1;
						}
					}
					if(!pp_cancel && tail_size) {
						THROW_SL(f.ReadV(buffer, tail_size));
						THROW(TransferData(transfer_id, (int)(chunk_count+1), tail_size, buffer));
						if(!pp_quite && pp) {
							scfd.SizeDone += tail_size;
							pp_reply = pp(&scfd);
							if(oneof2(pp_reply, SPRGRS_CANCEL, SPRGRS_STOP))
								pp_cancel = 1;
							else if(pp_reply == SPRGRS_QUITE)
								pp_quite = 1;
						}
					}
					if(!pp_cancel) {
						assert(f.Tell64() == ((int64)chunk_count * (int64)chunk_size + (int64)tail_size));
						THROW(FinishTransferData(transfer_id));
						{
							SString key, label, memo;
							(key = pKey).Transf(CTRANSF_INNER_TO_UTF8);
							(label = pVersionLabel).Transf(CTRANSF_INNER_TO_UTF8);
							(memo = pVersionMemo).Transf(CTRANSF_INNER_TO_UTF8);
							sess.Setup(UrlBase);
							int    result = func(sess, Token, transfer_id, key, label, memo);
							if(PreprocessResult((void *)result, sess))
								ok = 1;
						}
					}
					else
						ok = -1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPUhttClient::StartTransferData(const char * pName, int64 totalRawSize, int32 chunkCount, int * pTransferID)
{
	int    ok = 0;
	int    transfer_id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTSTARTDATATRANSFER_PROC func = reinterpret_cast<UHTTSTARTDATATRANSFER_PROC>(GetFuncEntryAndSetupSess("UhttStartDataTransfer", sess));
		if(func) {
			SString temp_buf;
			(temp_buf = pName).Transf(CTRANSF_INNER_TO_UTF8);
			transfer_id = func(sess, Token, temp_buf, totalRawSize, chunkCount);
			if(PreprocessResult((void *)transfer_id, sess))
				ok = 1;
		}
	}
	ASSIGN_PTR(pTransferID, transfer_id);
	return ok;
}

int PPUhttClient::TransferData(int transferID, int chunkNumber, size_t rawChunkSize, const void * pBinaryChunkData)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTTRANSFERDATA_PROC func = reinterpret_cast<UHTTTRANSFERDATA_PROC>(GetFuncEntryAndSetupSess("UhttTransferData", sess));
		if(func) {
			SString temp_buf;
			temp_buf.EncodeMime64(pBinaryChunkData, (size_t)rawChunkSize);
			int    result = func(sess, Token, transferID, chunkNumber, (int64)rawChunkSize, (char *)temp_buf.cptr()); // @badcast
			if(PreprocessResult(reinterpret_cast<void *>(result), sess))
				ok = 1;
		}
	}
	return ok;
}

int PPUhttClient::FinishTransferData(int transferID)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTFINISHTRANSFERDATA_PROC func = reinterpret_cast<UHTTFINISHTRANSFERDATA_PROC>(GetFuncEntryAndSetupSess("UhttFinishTransferData", sess));
		if(func) {
			int    result = func(sess, Token, transferID);
			if(PreprocessResult(reinterpret_cast<void *>(result), sess))
				ok = 1;
		}
	}
	return ok;
}

void FASTCALL PPUhttClient::DestroyResult(void ** ppResult)
{
	if(P_DestroyFunc) {
		static_cast<UHTT_DESTROYRESULT>(P_DestroyFunc)(*ppResult);
		*ppResult = 0;
	}
}
//
//
//
int PPUhttClient::ConvertLocationPacket(const UhttLocationPacket & rUhttPack, LocationTbl::Rec & rLocRec) const
{
	int    ok = 1;
	SString temp_buf;
	MEMSZERO(rLocRec);
	rLocRec.Type = LOCTYP_ADDRESS;
	rLocRec.Latitude = rUhttPack.Latitude;
	rLocRec.Longitude = rUhttPack.Longitude;
	rUhttPack.GetCode(temp_buf).CopyTo(rLocRec.Code, sizeof(rLocRec.Code));
	rUhttPack.Name.CopyTo(rLocRec.Name, sizeof(rLocRec.Name));
	LocationCore::SetExField(&rLocRec, LOCEXSTR_ZIP, rUhttPack.PostalCode);
	LocationCore::SetExField(&rLocRec, LOCEXSTR_SHORTADDR, rUhttPack.Address);
	LocationCore::SetExField(&rLocRec, LOCEXSTR_PHONE, rUhttPack.Phone);
	LocationCore::SetExField(&rLocRec, LOCEXSTR_CONTACT, rUhttPack.Contact);
	return ok;
}

int PPUhttClient::ConvertPersonPacket(const UhttPersonPacket & rUhttPack, PPID kindID, PPPersonPacket & rPsnPack) const
{
	int    ok = 1;
	if(rUhttPack.Name[0]) {
		uint   p;
		SString temp_buf;
		rUhttPack.Name.CopyTo(rPsnPack.Rec.Name, sizeof(rPsnPack.Rec.Name));
		rPsnPack.Kinds.addUnique(NZOR(kindID, PPPRK_UNKNOWN));
		if((temp_buf = rUhttPack.INN).NotEmptyS())
			rPsnPack.AddRegister(PPREGT_TPID, temp_buf);
		rPsnPack.AddRegister(PPREGT_UHTTCLID, rUhttPack.GetUhttContragentCode(temp_buf));
		for(p = 0; rUhttPack.PhoneList.get(&p, temp_buf);) {
			if(temp_buf.NotEmptyS())
				rPsnPack.ELA.AddItem(PPELK_WORKPHONE, temp_buf);
		}
		for(p = 0; rUhttPack.EMailList.get(&p, temp_buf);) {
			if(temp_buf.NotEmptyS())
				rPsnPack.ELA.AddItem(PPELK_EMAIL, temp_buf);
		}
		for(p = 0; rUhttPack.UrlList.get(&p, temp_buf);) {
			if(temp_buf.NotEmptyS())
				rPsnPack.ELA.AddItem(PPELK_WWW, temp_buf);
		}
		if(rUhttPack.AddrList.getCount()) {
			PPObjLocation loc_obj;
			for(p = 0; p < rUhttPack.AddrList.getCount(); p++) {
				const UhttPersonPacket::AddressP * p_uhtt_addr = rUhttPack.AddrList.at(p);
				if(p_uhtt_addr) {
					PPLocationPacket loc_pack, _loc_pack;
					if(ConvertLocationPacket(*p_uhtt_addr, loc_pack) > 0) {
						PPID   _loc_id = 0;
						if(loc_obj.P_Tbl->SearchCode(LOCTYP_ADDRESS, loc_pack.Code, &_loc_id, &_loc_pack) > 0) {

						}
						else {
							if(p_uhtt_addr->Kind == 1) {
								rPsnPack.Loc = loc_pack;
							}
							else if(p_uhtt_addr->Kind == 2) {
								rPsnPack.RLoc = loc_pack;
							}
							else if(p_uhtt_addr->Kind == 3) {
								rPsnPack.AddDlvrLoc(loc_pack);
							}
						}
					}
				}
			}
		}
	}
	else
		ok = -1;
	return ok;
}

int PPUhttClient::GetCommonMqsConfig(PPAlbatrossConfig & rCfg)
{
	int    ok = -1;
	PPSoapClientSession sess;
	UHTTGETCOMMONMQSCONFIG_PROC func = reinterpret_cast<UHTTGETCOMMONMQSCONFIG_PROC>(GetFuncEntryAndSetupSess("UhttGetCommonMqsConfig", sess));
	if(func) {
		SString * p_result_buf = func(sess);
		LastMsg = sess.GetMsg();
		if(p_result_buf) {
			if(PPAlbatrosCfgMngr::ParseCommonMqsConfigPacket(*p_result_buf, &rCfg) > 0) {
				ok = 1;
			}
			DestroyResult(reinterpret_cast<void **>(&p_result_buf));
		}
	}
	return ok;
}

int PPUhttClient::GetVersionList(const char * pKey, TSCollection <UhttDCFileVersionInfo> & rResult, SVerT * pMinVer)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTDCGETFILEVERSIONLIST_PROC func = reinterpret_cast<UHTTDCGETFILEVERSIONLIST_PROC>(GetFuncEntryAndSetupSess("UhttDCGetFileVersionList", sess));
		if(func) {
			TSCollection <UhttDCFileVersionInfo> * p_result = func(sess, Token, pKey);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				if(pMinVer) {
					long i = 0, count = rResult.getCount();
					SString buf;
					for(i = count - 1; i >= 0; i--) {
						int    mj = 0, mi = 0, rev = 0;
						int    free_item = 1;
						uint   j = 0;
						UhttDCFileVersionInfo * p_vi = rResult.at(i);
						StringSet ss('.', p_vi->Label);
						if(ss.get(&j, buf)) {
							mj = buf.ToLong();
							if(ss.get(&j, buf)) {
								mi = buf.ToLong();
								if(ss.get(&j, buf)) {
									rev = buf.ToLong();
									SVerT cur_ver;
									cur_ver.Set(mj, mi, rev);
									if(cur_ver.Cmp(pMinVer) > 0)
										free_item = 0;
								}
							}
						}
						if(free_item)
							rResult.atFree(i);
					}
				}
				if(rResult.getCount() <= 0)
					ok = -1;
				else
					ok = 1;
			}
		}
	}
	return ok;
}

/*static*/int PPUhttClient::ViewNewVerList(int showSelDlg)
{
	int    ok = 0;
	TSCollection <UhttDCFileVersionInfo> ver_list;
	PPUhttClient uhtt_client;
	if(uhtt_client.Auth()) {
		uint mj = 0, mi = 0, rev = 0;
		SVerT min_ver;
		PPVersionInfo ver_info = DS.GetVersionInfo();
		ver_info.GetVersion(&mj, &mi, &rev);
		min_ver.Set(mj, mi, rev);
		if(uhtt_client.GetVersionList("papyrus-setup-server", ver_list, &min_ver) > 0) {
			if(showSelDlg == 1) {
				StrAssocArray list;
				for(uint i = 0; i < ver_list.getCount(); i++)
					list.Add(i + 1, 0, ver_list.at(i)->Label);
				ListBoxSelDialog::Run(&list, PPTXT_TITLE_NEWVERLIST, 0);
			}
			ok = 1;
		}
	}
	return ok;
}
//
//
//
int PPUhttClient::GetWorkbookItemByID(int id, UhttWorkbookItemPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETWORKBOOKITEMBYID_PROC func = reinterpret_cast<UHTTGETWORKBOOKITEMBYID_PROC>(GetFuncEntryAndSetupSess("UhttGetWorkbookItemByID", sess));
		if(func) {
			UhttWorkbookItemPacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetWorkbookContentByID_ToFile(int id, const char * pFileName)
{
	int    ok = 0;
	SString file_name_to_remove;
	UhttDocumentPacket * p_result = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETWORKBOOKCONTENTBYID_PROC func = reinterpret_cast<UHTTGETWORKBOOKCONTENTBYID_PROC>(GetFuncEntryAndSetupSess("UhttGetWorkbookContentByID", sess));
		if(func) {
			p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				if(!isempty(pFileName)) {
					SFile f(pFileName, SFile::mWrite|SFile::mBinary);
					THROW_SL(f.IsValid());
					file_name_to_remove = f.GetName();
					if(p_result->Size) {
						size_t decoded_size = 0;
						STempBuffer tbuf(p_result->ContentMime.Len() * 3 / 2);
						THROW_SL(tbuf.IsValid());
						THROW_SL(p_result->ContentMime.DecodeMime64(tbuf, tbuf.GetSize(), &decoded_size));
						assert(decoded_size <= p_result->ContentMime.Len());
						THROW_SL(f.Write(tbuf, decoded_size));
					}
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
				p_result = 0;
				ok = 1;
			}
		}
	}
	CATCH
		if(p_result)
			DestroyResult(reinterpret_cast<void **>(&p_result));
		SFile::Remove(file_name_to_remove);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPUhttClient::GetWorkbookItemByCode(const char * pCode, UhttWorkbookItemPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETWORKBOOKITEMBYCODE_PROC func = reinterpret_cast<UHTTGETWORKBOOKITEMBYCODE_PROC>(GetFuncEntryAndSetupSess("UhttGetWorkbookItemByCode", sess));
		if(func) {
			UhttWorkbookItemPacket * p_result = func(sess, Token, pCode);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetWorkbookListByParentCode(const char * pParentCode, TSCollection <UhttWorkbookItemPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETWORKBOOKLISTBYPARENTCODE_PROC func = reinterpret_cast<UHTTGETWORKBOOKLISTBYPARENTCODE_PROC>(GetFuncEntryAndSetupSess("UhttGetWorkbookListByParentCode", sess));
		if(func) {
			TSCollection <UhttWorkbookItemPacket> * p_result = func(sess, Token, pParentCode);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::CreateWorkbookItem(long * pID, const UhttWorkbookItemPacket & rPack)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTCREATEWORKBOOKITEM_PROC func = reinterpret_cast<UHTTCREATEWORKBOOKITEM_PROC>(GetFuncEntryAndSetupSess("UhttCreateWorkbookItem", sess));
		if(func) {
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int PPUhttClient::SetWorkbookContentByID(int id, const char * pFileName)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTSETWORKBOOKCONTENTBYID_PROC func = reinterpret_cast<UHTTSETWORKBOOKCONTENTBYID_PROC>(GetFuncEntryAndSetupSess("UhttSetWorkbookContentByID", sess));
		if(func) {
			UhttDocumentPacket doc_pack;
			doc_pack.SetFile(pFileName);
			doc_pack.ObjTypeSymb = "WORKBOOK";
			doc_pack.UhttObjID = id;
			UhttStatus * p_result = func(sess, Token, doc_pack);
			if(PreprocessResult(p_result, sess)) {
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetStyloDeviceByID(int id, UhttStyloDevicePacket & rPack)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETSTYLODEVICEBYID_PROC func = reinterpret_cast<UHTTGETSTYLODEVICEBYID_PROC>(GetFuncEntryAndSetupSess("UhttGetStyloDeviceByID", sess));
		if(func) {
			UhttStyloDevicePacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetStyloDeviceByCode(const char * pCode, UhttStyloDevicePacket & rPack)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETSTYLODEVICEBYCODE_PROC func = reinterpret_cast<UHTTGETSTYLODEVICEBYCODE_PROC>(GetFuncEntryAndSetupSess("UhttGetStyloDeviceByCode", sess));
		if(func) {
			SString temp_buf;
			(temp_buf = pCode).Transf(CTRANSF_INNER_TO_UTF8);
			UhttStyloDevicePacket * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::CreateStyloDevice(long * pID, const UhttStyloDevicePacket & rPack)
{
	int    ok = 0;
	int    id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTCREATESTYLODEVICE_PROC func = reinterpret_cast<UHTTCREATESTYLODEVICE_PROC>(GetFuncEntryAndSetupSess("UhttCreateStyloDevice", sess));
		if(func) {
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int PPUhttClient::GetProcessorByID(long id, UhttProcessorPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETPROCESSORBYID_PROC func = reinterpret_cast<UHTTGETPROCESSORBYID_PROC>(GetFuncEntryAndSetupSess("UhttGetProcessorByID", sess));
		if(func) {
			UhttProcessorPacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetProcessorByCode(const char * pCode, UhttProcessorPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETPROCESSORBYCODE_PROC func = reinterpret_cast<UHTTGETPROCESSORBYCODE_PROC>(GetFuncEntryAndSetupSess("UhttGetProcessorByCode", sess));
		if(func) {
			SString temp_buf;
			(temp_buf = pCode).Transf(CTRANSF_INNER_TO_UTF8);
			UhttProcessorPacket * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::CreateProcessor(long * pID, const UhttProcessorPacket & rPack)
{
	int    ok = 0;
	int    id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTCREATEPROCESSOR_PROC func = reinterpret_cast<UHTTCREATEPROCESSOR_PROC>(GetFuncEntryAndSetupSess("UhttCreateProcessor", sess));
		if(func) {
			SString temp_buf;
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int PPUhttClient::GetTSessionByID(long id, UhttTSessionPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETTSESSIONBYID_PROC func = reinterpret_cast<UHTTGETTSESSIONBYID_PROC>(GetFuncEntryAndSetupSess("UhttGetTSessionByID", sess));
		if(func) {
			UhttTSessionPacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetTSessionByUUID(const S_GUID & rUuid, UhttTSessionPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETTSESSIONBYUUID_PROC func = reinterpret_cast<UHTTGETTSESSIONBYUUID_PROC>(GetFuncEntryAndSetupSess("UhttGetTSessionByUUID", sess));
		if(func) {
			UhttTSessionPacket * p_result = func(sess, Token, rUuid);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::GetTSessionByPrc(long prcID, const LDATETIME & rSince, TSCollection <UhttTSessionPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTGETTSESSIONBYPRC_PROC func = reinterpret_cast<UHTTGETTSESSIONBYPRC_PROC>(GetFuncEntryAndSetupSess("UhttGetTSessionByPrc", sess));
		if(func) {
			UhttTimestamp since;
			since = rSince;
			TSCollection <UhttTSessionPacket> * p_result = func(sess, Token, prcID, !rSince ? 0 : &since);
			if(PreprocessResult(p_result, sess)) {
				for(uint i = 0; i < p_result->getCount(); i++) {
					if(p_result->at(i)) {
						UhttTSessionPacket * p_pack = rResult.CreateNewItem();
						*p_pack = *p_result->at(i);
					}
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPUhttClient::CreateTSession(long * pID, const UhttTSessionPacket & rPack)
{
	int    ok = 0;
	int    id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTCREATETSESSION_PROC func = reinterpret_cast<UHTTCREATETSESSION_PROC>(GetFuncEntryAndSetupSess("UhttCreateTSession", sess));
		if(func) {
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

UhttTagItem * PPUhttClient::GetUhttTagText(PPID objType, PPID objID, PPID tagID, const char * pTagSymb)
{
	UhttTagItem * p_result = 0;
	if(tagID && objType && objID && !isempty(pTagSymb)) {
		Reference * p_ref = PPRef;
		ObjTagItem tag_item;
		if(p_ref->Ot.GetTag(objType, objID, tagID, &tag_item) > 0) {
			SString temp_buf;
			if(tag_item.TagDataType == OTTYP_ENUM && tag_item.TagEnumID && IS_DYN_OBJTYPE(tag_item.TagEnumID)) {
				Reference2Tbl::Rec en_rec;
				if(p_ref->GetItem(tag_item.TagEnumID, tag_item.Val.IntVal, &en_rec) > 0) {
					if(en_rec.Val2) { // ref to parent item
						(temp_buf = "/h|").Cat(en_rec.ObjName);
						for(PPID next_id = en_rec.Val2; next_id != 0;) {
							if(p_ref->GetItem(tag_item.TagEnumID, next_id, &en_rec) > 0) {
								temp_buf.Cat("/h>").Cat(en_rec.ObjName);
								next_id = en_rec.Val2;
							}
							else
								break;
						}
					}
					else
						temp_buf = en_rec.ObjName;
				}
			}
			else
				tag_item.GetStr(temp_buf);
			if(temp_buf.NotEmptyS()) {
				p_result = new UhttTagItem(pTagSymb, temp_buf);
			}
		}
	}
	return p_result;
}
//
//
//
int TestCURL()
{
	int    ok = 1;
#if 0 // {
	SString wr_file_name;
    SBuffer buffer;
    {
		PPGetFilePath(PPPATH_OUT, "test-curl-get.txt", wr_file_name);
		SFile wr_stream(/*buffer*/wr_file_name, SFile::mWrite);
		ScURL c;
		ok = c.HttpGet("http://agner.org", 0, &wr_stream);
    }
#endif // } 0
    {
#if 0 // 0 {
    	// http://posttestserver.com/post.php
		PPGetFilePath(PPPATH_OUT, "test-curl-post.txt", wr_file_name);
		SFile wr_stream(/*buffer*/wr_file_name, SFile::mWrite);
		ScURL c;
		SString req = "name=daniel&project=curl";
		ok = c.HttpPost("http://posttestserver.com/post.php", 0, req, &wr_stream);
#endif // } 0
    }
    return ok;
}

int PPUhttClient::SendSms(const TSCollection <UhttSmsPacket> & rList, TSCollection <UhttStatus> & rResult)
{
	rResult.freeAll();
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth) {
		PPSoapClientSession sess;
		UHTTSENDSMS_PROC func = reinterpret_cast<UHTTSENDSMS_PROC>(GetFuncEntryAndSetupSess("UhttSendSms", sess));
		if(func) {
			TSCollection <UhttStatus> * p_result = func(sess, Token, rList);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult(reinterpret_cast<void **>(&p_result));
				ok = 1;
			}
			else {
				PPSetError(PPERR_UHTTSVCFAULT, LastMsg.Transf(CTRANSF_UTF8_TO_INNER));
			}
		}
	}
	return ok;
}

int TestUhttClient()
{
	int    ok = 1;
	PPLogger logger;
	SString log_buf;
	SString code_buf;
	SString temp_buf;
	PPUhttClient uhtt_cli;
	log_buf.Z().Cat("Universe-HTT client ctr").CatDiv(':', 2);
	log_buf.Cat((uhtt_cli.GetState() & PPUhttClient::stHasAccount) ? "has-account" : "no-account");
	log_buf.Space().Cat((uhtt_cli.GetState() & PPUhttClient::stDefaultServer) ? "default-server" : "own-server");
	PPLogMessage(PPFILNAM_TEST_LOG, log_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
	{
		PPAlbatrossConfig alb_cfg;
		log_buf.Z().Cat("PPUhttClient::GetCommonMqsConfig");
		if(uhtt_cli.GetCommonMqsConfig(alb_cfg) > 0) {
			log_buf.Space().Cat("ok");
			alb_cfg.GetExtStrData(ALBATROSEXSTR_MQC_HOST, temp_buf);
			log_buf.Space().Cat(temp_buf);
			alb_cfg.GetExtStrData(ALBATROSEXSTR_MQC_VIRTHOST, temp_buf);
			log_buf.Space().Cat(temp_buf); 
			alb_cfg.GetExtStrData(ALBATROSEXSTR_MQC_USER, temp_buf);
			log_buf.Space().Cat(temp_buf);
			ok = 1;
		}
		else
			log_buf.Space().Cat("fail");
		PPLogMessage(PPFILNAM_TEST_LOG, log_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
	}
	{
		THROW(uhtt_cli.Auth());
		log_buf.Z().Cat("PPUhttClient::Auth").Space().Cat("ok");
		PPLogMessage(PPFILNAM_TEST_LOG, log_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
		{
			//
			// Get Quot By Loc
			//
			code_buf = "AG1";
			UhttLocationPacket uhtt_loc;
			log_buf.Z().Cat("PPUhttClient::GetLocationByCode").CatParStr(code_buf);
			int r = 0;
            if(uhtt_cli.GetLocationByCode(code_buf, uhtt_loc)) {
				log_buf.Space().Cat("ok");
				PPLogMessage(PPFILNAM_TEST_LOG, log_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
				UhttQuotFilter filt;
				TSCollection <UhttQuotPacket> uhtt_list;
				filt.LocationID = uhtt_loc.ID;
				log_buf.Z().Cat("PPUhttClient::GetQuot");
				r = uhtt_cli.GetQuot(filt, uhtt_list);
				if(r > 0 && uhtt_list.getCount()) {
					log_buf.Space().Cat("ok");
					PPLogMessage(PPFILNAM_TEST_LOG, log_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
					for(uint i = 0; i < uhtt_list.getCount(); i++) {
						const UhttQuotPacket * p_uhtt_item = uhtt_list.at(i);
						temp_buf.Z().
							CatEq("goodsid", p_uhtt_item->GoodsID).CatDiv(';', 2).
							CatEq("value", p_uhtt_item->Value).CatDiv(';', 2).
							CatEq("loc", p_uhtt_item->LocID);
						log_buf.CR().Cat(temp_buf);
					}
				}
				else
					log_buf.Cat((r == 0) ? "fail" : "nothing");
            }
            else
				log_buf.Space().Cat("fail");
			PPLogMessage(PPFILNAM_TEST_LOG, log_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
			if(!r)
				PPLogMessage(PPFILNAM_TEST_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
		}
		{
			//
			// Get Location List by Phone
			//
			code_buf = "777777";
			{
				TSCollection <UhttLocationPacket> uhtt_loc_list;
				log_buf.Z().Cat("PPUhttClient::GetLocationListByPhone").CatParStr(code_buf);
				int r = uhtt_cli.GetLocationListByPhone(code_buf, uhtt_loc_list);
				if(r > 0 && uhtt_loc_list.getCount()) {
					log_buf.Space().Cat("ok");
					for(uint i = 0; i < uhtt_loc_list.getCount(); i++) {
						const UhttLocationPacket * p_uhtt_loc_item = uhtt_loc_list.at(i);
						temp_buf.Z().
							CatEq("id", p_uhtt_loc_item->ID).CatDiv(';', 2).
							CatEq("phone", p_uhtt_loc_item->Phone).CatDiv(';', 2).
							CatEq("contact", p_uhtt_loc_item->Contact);
						log_buf.CR().Cat(temp_buf);
					}
				}
				else 
					log_buf.Cat((r == 0) ? "fail" : "nothing");
				PPLogMessage(PPFILNAM_TEST_LOG, log_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
				if(!r)
					PPLogMessage(PPFILNAM_TEST_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
			}
		}
	}
	CATCH
		PPLogMessage(PPFILNAM_TEST_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
		ok = 0;
	ENDCATCH
	return ok;
}
//
// Papyrus PlugIn
//
struct PPI_Capability {
	uint   Ver;
    int    Flags;
	char   Name[128];
    uint8  Reserve[64];
};

struct PPI_EDI_InitBlock {
	const char * P_Server;
	int    Port;
	const char * P_Password;
    const char * P_LogFileName;
    const char * P_TempPath;
};

typedef void * PPI_OBJECT;

#define PPI_OBT_VOID       0
#define PPI_OBT_STRINGLIST 1

int    PPI_GetObjectType(PPI_OBJECT);
int    PPI_List_GetCount(PPI_OBJECT, int * pCount);
int    PPI_GetCapability(PPI_Capability * pCapability);
int    PPI_ReqFunctional(int function, int * pReply);
int    PPI_GetErrorMessage(char * pMsgBuf, uint * pBufLen);
int    PPI_EDI_Init(const PPI_EDI_InitBlock * pBlk);
PPI_OBJECT * PPI_EDI_GetMessageList(int ediMsgType);
//
//
//
PPXmlFileDetector::PPXmlFileDetector() : SXmlSaxParser(SXmlSaxParser::fStartElement), ElementCount(0), Result(0), 
	P_ShT(PPGetStringHash(PPSTR_HASHTOKEN)), P_ShT_C(PPGetStringHash(PPSTR_HASHTOKEN_C))
{
}

PPXmlFileDetector::~PPXmlFileDetector()
{
}

int PPXmlFileDetector::Run(const char * pFileName, int * pResult)
{
	ElementCount = 0;
	Result = 0;
	int    ok = ParseFile(pFileName);
	ASSIGN_PTR(pResult, Result);
	return ok;
}

/*virtual*/int PPXmlFileDetector::StartElement(const char * pName, const char ** ppAttrList)
{
	enum {
		outofhttokCAM    = 30001,
	};
	ElementCount++;
	int    ok = 1;
    int    tok = 0;
	bool   do_continue = false;
	// @v12.4.4 {
	if(sstreq(pName, "CAM")) { 
		// ?? Result = SAP_Order_Cocacola;
		TokPath.push(outofhttokCAM);
		do_continue = true;
	}
	else if(sstreq(pName, "ORDERS")) {
		if(TokPath.getPointer() == 1 && TokPath.peek() == outofhttokCAM) {
			Result = SAP_Order_Cocacola;
		}
	}
	// } @v12.4.4 
    if(P_ShT) {
		uint   _ut = 0;
		uint   _ut2 = 0; // @v11.9.4
		size_t colon_pos = 0;
		SString & r_temp_buf = SLS.AcquireRvlStr();
		r_temp_buf = pName;
		// @v12.0.2 {
		if(r_temp_buf.IsLegalUtf8()) {
			r_temp_buf.Utf8ToLower();
		}
		// } @v12.0.2 
		// @v12.0.2 r_temp_buf.ToLower();
		if(r_temp_buf.SearchChar(':', &colon_pos))
			r_temp_buf.ShiftLeft(colon_pos+1);
		P_ShT->Search(r_temp_buf, &_ut, 0);
		tok = _ut;
		if(ElementCount == 1) {
			if(tok)
				TokPath.push(tok);
			switch(tok) {
				case PPHS_ORDERS:
				case PPHS_ORDRSP:
				case PPHS_DESADV:
				case PPHS_ALCDES:
				case PPHS_RECADV:
				case PPHS_PARTIN: Result = Eancom; break;
				case PPHS_EDIMESSAGE: Result = KonturEdi; break;
				case PPHS_DOCUMENTS: Result = EgaisDoc; break;
				case PPHS_PPPP_START: Result = PpyAsyncPosIx; break;
				case PPHS_URLSET: Result = Sitemap; break;
				case PPHS_PROJECT: Result = ProjectAbstract; break;
				case PPHS_RESOURCES: Result = ResourcesAbstract; break;
				case PPHS_VALUES: Result = ValuesAbstract; break;
				case PPHS_CHEQUE: Result = EgaisCheque; break;
				case PPHS_TIMEZONES: Result = TimezonesAbstract; break;
				case 0: // @v11.9.4
					{
						// PPHSC_RU_DOCUMENT
						P_ShT_C->Search(/*r_temp_buf*/pName, &_ut2, 0); // @v12.0.2 r_temp_buf-->pName (хэш-таблица P_ShT_C чувствительна к регистру)
						if(_ut2 == PPHSC_RU_FILE) {
							Result = NalogRu_Generic;
							for(uint i = 0; ppAttrList[i] != 0; i += 2) {
								const char * p_attr = ppAttrList[i];
								const char * p_text_data = ppAttrList[i+1];
								//
							}
						}
					}
					break;
			}
		}
    }
	if(!do_continue) {
		SaxStop();
	}
	return ok;
}
//
//
//
PPTokenRecognizer::PPTokenRecognizer() : STokenRecognizer()
{
}

/*virtual*/int PPTokenRecognizer::PostImplement(ImplementBlock & rIb, const uchar * pToken, int len, SNaturalTokenArray & rResultList, SNaturalTokenStat * pStat)
{
	int    ok = 1;
#if(_MSC_VER >= 1900)
	if(rIb.DecCount >= 5 && rIb.DecCount <= 15) { // i don't sure in correctness of [5..15] limit
		if(PhnL.Parse(reinterpret_cast<const char *>(pToken), "RU")) {
			rResultList.AddTok(SNTOK_PHONE, 0.9f, 0/*flags*/);
		}
		else {
			//
			// Рекогнайзер STokenRecognizer умеет распознавать номера телефонов. Однако, я предполагаю, что
			// libphonenumber делает это лучше. Но слово "предполагаю" здесь ключевое - то есть, я не уверен. В связи с этим не понятно
			// как трактовать ситуацию, когда STokenRecognizer увидел номер телефона, а libphonenumber - нет.
			// Пока оставлю без изменений, но, возможно, надо будет удалять значение SNTOK_PHONE из списка rResultList.
			//
			rResultList.AddTok(SNTOK_PHONE, 0.0f, 0/*flags*/);
		}
	}
#endif
	return ok;
}
//
// 
// 
PPVerHistory::Info::Info()
{
}

PPVerHistory::PPVerHistory()
{
}

int PPVerHistory::Read(const char * pDataPath, Info * pInfo)
{
	int    ok = -1;
	if(pDataPath) {
		constexpr size_t hdr_size = sizeof(PPVerHistory::Header);
		size_t len = 0;
		SString fname;
		SString path;
		PPGetFileName(PPFILNAM_VERHIST, fname);
		(path = pDataPath).SetLastSlash().Cat(fname);
		if(fileExists(path) != 0) {
			SFile  f;
			uint32 crc  = 0;
			Header hdr;
			MEMSZERO(hdr);
			THROW_SL(f.Open(path, SFile::mRead|SFile::mBinary) > 0);
			THROW_PP_S(f.Read(&hdr, hdr_size, &len) > 0 && len == hdr_size, PPERR_INVHDRSIZE, path);
			THROW_PP_S(hdr.Signature == PPConst::Signature_VerHist, PPERR_VERHISTINVSIGN, path);
			THROW_SL(f.CalcCRC(hdr_size, &crc));
			SLibError = SLERR_INVALIDCRC;
			THROW_SL(crc == hdr.CRC);
			if(pInfo) {
				pInfo->MinVer = hdr.MinVer;
				pInfo->CurVer = hdr.CurVer;
				pInfo->DbUUID = hdr.DbUUID;
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPVerHistory::Write(const char * pDataPath, const Info * pInfo)
{
	int    ok = -1;
	uint32 crc = 0;
	SString temp_buf;
	SString path;
	SVerT hdr_minv;
	(path = pDataPath).SetLastSlash().Cat(PPGetFileName(PPFILNAM_VERHIST, temp_buf));
	SFile  f;
	Header hdr;
	Record rec;
	THROW_INVARG(pInfo);
	PPSetAddedMsgString(path);
	if(fileExists(path)) {
		THROW_SL(f.Open(path, SFile::mDenyWrite|SFile::mReadWrite|SFile::mBinary) > 0);
	}
	else {
		THROW_SL(f.Open(path, SFile::mDenyWrite|SFile::mWrite|SFile::mBinary) > 0);
		f.Close();
		THROW_SL(f.Open(path, SFile::mDenyWrite|SFile::mReadWriteTrunc|SFile::mBinary) > 0);
	}
	MEMSZERO(hdr);
	MEMSZERO(rec);
	f.Seek(0);
	hdr.Signature = PPConst::Signature_VerHist;
	hdr.CRC       = crc;
	hdr.CurVer    = pInfo->CurVer;
	hdr.MinVer    = pInfo->MinVer;
	hdr.DbUUID    = pInfo->DbUUID;
	THROW_SL(f.Write(&hdr, sizeof(hdr)) > 0);
	{
		MACAddr addr;
		rec.Dtm = getcurdatetime_();
		rec.Ver = pInfo->CurVer;
		DS.GetMachineID(&addr, 0);
		addr.ToStr(0, temp_buf).CopyTo(rec.Machine, sizeof(rec.Machine));
		f.Seek(0, SEEK_END);
		THROW_SL(f.Write(&rec, sizeof(rec)) > 0);
		THROW_SL(f.CalcCRC(sizeof(hdr), &crc) > 0);
		f.Seek(0);
		hdr.CRC = crc;
		THROW_SL(f.Write(&hdr, sizeof(hdr)) > 0);
	}
	CATCHZOK
	return ok;
}

/*static*/int PPVerHistory::Log(const char * pDataPath, const char * pLogPath)
{
	int    ok = 0;
	SString path;
	SString fname;
	PPGetFileName(PPFILNAM_VERHIST, fname);
	path.CopyFrom(pDataPath).SetLastSlash().Cat(fname);
	if(fileExists(path)) {
		int    j = 0;
		int    m = 0;
		int    r = 0;
		uint32 crc = 0;
		const char * p_rectitle = "date time;mac address;version\n";
		const char * p_hdrtitle = "current version;minimum version\n";
		SString buf;
		Header hdr;
		Record rec;
		SFile  f;
		SFile  logf;
		MEMSZERO(hdr);
		MEMSZERO(rec);
		THROW_SL(f.Open(path, SFile::mRead|SFile::mBinary) > 0);
		THROW_SL(f.Read(&hdr, sizeof(hdr)) > 0);
		THROW_SL(f.CalcCRC(sizeof(hdr), &crc));
		SLibError = SLERR_INVALIDCRC;
		THROW_SL(crc == hdr.CRC);
		PPGetFileName(PPFILNAM_VERHISTLONG, (fname = 0));
		(path = 0).CopyFrom(pLogPath).SetLastSlash().Cat(fname);
		THROW_SL(logf.Open(path, SFile::mWrite) > 0);
		hdr.CurVer.Get(&j, &m, &r);
		buf.CatDotTriplet(j, m, r).Semicol();
		hdr.MinVer.Get(&j, &m, &r);
		buf.CatDotTriplet(j, m, r).CR();
		THROW_SL(logf.WriteLine(p_hdrtitle) > 0);
		THROW_SL(logf.WriteLine(buf));
		THROW_SL(logf.WriteLine(p_rectitle) > 0);
		while(f.Read(&rec, sizeof(rec)) > 0) {
			rec.Ver.Get(&(j = 0), &(m = 0), &(r = 0));
			buf.Z().Cat(rec.Dtm).Semicol().Cat(rec.Machine).Semicol().CatDotTriplet(j, m, r).CR();
			THROW_SL(logf.WriteLine(buf) > 0);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

/*static*/int PPVerHistory::Log()
{
	int    ok = -1;
	SString arg_buf;
	if(PPSession::GetStartUpOption(PPSession::cmdlVerHist, arg_buf) && arg_buf.NotEmptyS()) {
		SString data_path;
		SString log_path;
		PPIniFile ini_file;
		PPID   dbentry_id = 0;
		PPDbEntrySet2 dbes;
		DbLoginBlock dlb;
		PPVerHistory verh;
		dbes.ReadFromProfile(&ini_file);
		THROW_SL(dbentry_id = dbes.GetBySymb(arg_buf, &dlb));
		dlb.GetAttr(DbLoginBlock::attrDbPath, data_path);
		PPGetPath(PPPATH_LOG, log_path);
		THROW(verh.Log(data_path, log_path) > 0);
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
// Эксперименты с архиватором restic
//
class PPResticInterface { // @v12.4.1 @construction
public:
	struct RepoParam {
		RepoParam()
		{
		}
		RepoParam & Z()
		{
			Appellation.Z();
			Loc.Z();
			Auth.Z();
			Pw.Z();
			SsIncl.Z();
			SsExcl.Z();
			return *this;
		}
		int    ReadIniFile(const char * pFileName)
		{
			Z();
			int    ok = 0;
			SString temp_buf;
			PPIniFile ini_file(pFileName, 0, 0, 0/*useIniBuf*/); // Аргумент useIniBuf должен быть 0 иначе не отработает выборка строк без конструкции a=b из секции
				// функцией SIniFile::GetEntries() с параметром storeAllString != 0.
			if(ini_file.IsValid()) {
				{
					ini_file.GetParam("general", "name", temp_buf);
					if(temp_buf.IsEmpty()) {
						ini_file.GetParam("general", "appellation", temp_buf);
					}
					if(temp_buf.NotEmptyS())
						Appellation = temp_buf;
				}
				{
					ini_file.GetParam("general", "password", temp_buf);
					if(temp_buf.NotEmptyS()) {
						Pw = temp_buf;
					}
				}
				{
					ini_file.GetParam("general", "destination", temp_buf);
					if(temp_buf.NotEmptyS()) {
						Loc = temp_buf;
					}
				}
				ini_file.GetEntries2("include", &SsIncl, SIniFile::gefStoreAllString);
				ini_file.GetEntries2("exclude", &SsExcl, SIniFile::gefStoreAllString);
				if(Loc.NotEmpty() && SsIncl.IsCountGreaterThan(0)) {
					ok = 1;
				}
			}
			return ok;
		}
		SString Appellation;
		SString Loc;
		SString Auth;
		SString Pw;
		StringSet SsIncl;
		StringSet SsExcl;
	};
	struct Snapshot {
		struct Summary { // @flat
			Summary() 
			{
				THISZERO();
			}
			uint64 UedBackupStartTm;
			uint64 UedBackupEndTm;
			uint   FilesNew;
			uint   FilesChanged;
			uint   FilesUnmodified;
			uint   DirsNew;
			uint   DirsChanged;
			uint   DirsUnmodified;
			uint   DataBlobs;
			uint   TreeBlobs;
			uint   DataAdded;
			uint   DataAddedPacked;
			uint   TotalFilesProcessed;
			uint   TotalBytesProcessed;
		};
		Snapshot() : UedTime(0), ShortId(0)
		{
		}
		uint64 UedTime;
		binary256 Tree;
		SString HostName;
		SString UserName;
		SVerT  ResticVer;
		binary256 Id;
		uint32 ShortId;
		StringSet SsPaths;
		Summary S;
	};
	struct Entry {
		uint32 Dummy;
	};
	PPResticInterface(PPLogger * pLogger);
	~PPResticInterface();
	int    CreateRepo(const RepoParam & rP);
	//
	// ARG(dryMode IN): фактически ничего не копирует, но лишь показывает что будет копировать.
	//
	int    Backup(const RepoParam & rP, bool dryMode);
	int    Restore(const RepoParam & rP);
	int    GetSnapshotList(const RepoParam & rP, TSCollection <Snapshot> * pResult);
	int    GetEntryList(const RepoParam & rP, uint32 snapshotShortId, TSCollection <Entry> * pResult);
private:
	int    MakeProcessObj(const RepoParam & rP, SlProcess & rPrc, bool captureOutput);
	int    GetExePath(SString & rBuf);
	enum {
		spofStdOut = 0x0001,
		spofStdErr = 0x0002
	};
	int    ShowProcessOuput(SlProcess::Result & rR, uint flags/*spofXXX*/);
	int    ReadProcessJsonOutput(SlProcess::Result & rR, TSCollection <SJson> & rList);
	int    Helper_GrantRightToRepoPath(const char * pPathUtf8); // @recursive
	int    Helper_CreateRepoDir(const SString & rDir);

	PPLogger * P_Logger; // @notowned
	SString DebugLogFileName;
};

PPResticInterface::PPResticInterface(PPLogger * pLogger) : P_Logger(pLogger)
{
	PPGetFilePath(PPPATH_LOG, "restic-debug.log", DebugLogFileName);
}

PPResticInterface::~PPResticInterface()
{
}

int PPResticInterface::GetExePath(SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	SString temp_buf("restic.exe");
	//
	SFsPath ps(temp_buf);
	if(ps.Drv.IsEmpty() || ps.Dir.IsEmpty()) {
		ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
		if(SlProcess::FindFullPathByProcessFileName(temp_buf, rBuf)) {
			ok = 1;
		}
		else
			rBuf.Z();
	}
	return ok;
}

int PPResticInterface::MakeProcessObj(const RepoParam & rP, SlProcess & rPrc, bool captureOutput)
{
	int    ok = -1;
	SString temp_buf;
	SString module_path;
	GetExePath(module_path);
	if(module_path.NotEmpty() /*&& rP.Loc.NotEmpty()*/) {
		rPrc.SetPath(module_path);
		if(rP.Loc.NotEmpty()) {
			rPrc.AddArg("-r"); // "--repo"
			rPrc.AddArg(rP.Loc);
			//rPrc.AddEnv("RESTIC_REPOSITORY", rP.Loc);
			if(rP.Pw.NotEmpty()) {
				rPrc.AddEnv("RESTIC_PASSWORD", rP.Pw);
			}
			else {
				rPrc.AddArg("--insecure-no-password");
			}
		}
		else {
			rPrc.AddArg("--help");
		}
		{
			PPGetFilePath(PPPATH_LOG, "restic-internal-debug.log", temp_buf);
			rPrc.AddEnv("DEBUG_LOG", temp_buf);
		}
		if(captureOutput)
			rPrc.SetFlags(SlProcess::fCaptureStdErr|SlProcess::fCaptureStdOut);
		rPrc.SetFlags(SlProcess::fDefaultErrorMode);
		//rPrc.SetWorkingDir(L"d:\\"); // @experimental @debug
		ok = 1;
	}
	return ok;
}

int PPResticInterface::ReadProcessJsonOutput(SlProcess::Result & rR, TSCollection <SJson> & rList)
{
	int    ok = -1;
	if(!!rR.HStdOutRd) {
		SString temp_buf;
		DWORD  actual_size;
		char   cbuf[4096];
		SBuffer buffer;
		for(;;) {
			actual_size = 0;
			int rfr = ::ReadFile(rR.HStdOutRd, cbuf, sizeof(cbuf)-1, &actual_size, NULL);
			if(rfr && actual_size) {
				buffer.Write(cbuf, actual_size);
				ok = 1;
			}
			else
				break;
		}
		if(ok > 0) {
			SString log_file_name;
			PPGetFilePath(PPPATH_LOG, "restic-output.log", log_file_name);
			SFile f_log(log_file_name, SFile::mAppend);
			SFile f_(buffer, SFile::mRead|SFile::mBinary);
			while(f_.ReadLine(temp_buf, SFile::rlfChomp)) {
				f_log.WriteLine(temp_buf.CR());
				SJson * p_new_js = SJson::Parse(temp_buf);
				if(p_new_js) {
					rList.insert(p_new_js);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int PPResticInterface::ShowProcessOuput(SlProcess::Result & rR, uint flags)
{
	int    ok = -1;
	if(P_Logger) {
		SString temp_buf;
		DWORD  actual_size;
		char   cbuf[4096];
		SBuffer buffer;
		if((!flags || flags & spofStdOut) && !!rR.HStdOutRd) {
			buffer.Z();
			for(;;) {
				actual_size = 0;
				int rfr = ::ReadFile(rR.HStdOutRd, cbuf, sizeof(cbuf)-1, &actual_size, NULL);
				if(rfr && actual_size) {
					buffer.Write(cbuf, actual_size);
					ok = 1;
				}
				else
					break;
			}
			if(ok > 0) {
				SFile f_(buffer, SFile::mRead|SFile::mBinary);
				while(f_.ReadLine(temp_buf, SFile::rlfChomp)) {
					P_Logger->Log(temp_buf);
				}
			}
		}
		//
		if((!flags || flags & spofStdErr) && !!rR.HStdErrRd) {
			buffer.Z();
			for(;;) {
				actual_size = 0;
				int rfr = ::ReadFile(rR.HStdErrRd, cbuf, sizeof(cbuf)-1, &actual_size, NULL);
				if(rfr && actual_size) {
					buffer.Write(cbuf, actual_size);
					ok = 1;
				}
				else
					break;
			}
			if(ok > 0) {
				SFile f_(buffer, SFile::mRead|SFile::mBinary);
				while(f_.ReadLine(temp_buf, SFile::rlfChomp)) {
					P_Logger->Log(temp_buf);
				}
			}
		}
	}
	return ok;
}

bool GrantFullAccessForDirectoryToCurrentUser(const SString & rPathUtf8); // @prototype(sprocess.cpp)

int PPResticInterface::Helper_GrantRightToRepoPath(const char * pPathUtf8) // @recursive
{
	int    ok = -1;
	SString temp_buf;
	if(SFile::IsDir(pPathUtf8)) {
		SString path(pPathUtf8);
		THROW(GrantFullAccessForDirectoryToCurrentUser(path));
		ok = 1;
		{
			path.SetLastSlash();
			(temp_buf = path).Cat("*.*");
			SDirEntry de;
			for(SDirec dir(temp_buf, 1); dir.Next(&de);) {
				if(de.IsFolder()) {
					de.GetNameUtf8(temp_buf);
					path.Cat(temp_buf);
					THROW(Helper_GrantRightToRepoPath(path)); // @recursion
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPResticInterface::Helper_CreateRepoDir(const SString & rDir)
{
	int    ok = -1;
	SECURITY_DESCRIPTOR * p_sd = 0;
	ACL * p_new_dacl = 0;
	SECURITY_ATTRIBUTES sa;
	SECURITY_ATTRIBUTES * p_sa = 0;
	if(true) {
		S_WinSID current_user_sid;
		if(SlProcess::GetCurrentUserSid(current_user_sid)) {	
			EXPLICIT_ACCESS_W ea;
			MEMSZERO(ea);
			ea.grfAccessPermissions = GENERIC_ALL|STANDARD_RIGHTS_ALL|SPECIFIC_RIGHTS_ALL;
			ea.grfAccessMode = SET_ACCESS;
			ea.grfInheritance = CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE;
			ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
			ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
			//ea.Trustee.ptstrName = (LPWSTR)pUserSid;
			ea.Trustee.ptstrName = static_cast<LPWSTR>(current_user_sid.GetPtr());
			{
				// Создаем новый DACL
				const DWORD seia_r = ::SetEntriesInAclW(1, &ea, 0/*p_old_dacl*/, &p_new_dacl);
				THROW(seia_r == ERROR_SUCCESS); // std::wcerr << L"SetEntriesInAcl failed: " << seia_r << std::endl;
				//
				p_sd = (SECURITY_DESCRIPTOR *)::LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
				THROW(::InitializeSecurityDescriptor(p_sd, SECURITY_DESCRIPTOR_REVISION));
				THROW(::SetSecurityDescriptorDacl(p_sd, TRUE/*bDaclPresent flag*/, p_new_dacl, FALSE/*not a default DACL*/));
				//
				{
					MEMSZERO(sa);
					sa.nLength = sizeof(sa);
					sa.lpSecurityDescriptor = p_sd;
					p_sa = &sa;
				}
			}
		}
	}
	{
		int    cdr = 0;
		if(p_sa) {
			cdr = SFile::CreateDirSA(rDir, p_sa);
		}
		else {
			cdr = SFile::CreateDir(rDir);
		}
		if(cdr) {
			ok = 1;
		}
		else {
			ok = 0;
		}
	}
	CATCHZOK
	if(p_sd) {
		::LocalFree(p_sd);
	}
	if(p_new_dacl) {
		::LocalFree(p_new_dacl);
	}
	return ok;
}

int PPResticInterface::CreateRepo(const RepoParam & rP)
{
	// restic init --repo restic-repo-papyrus-main
	//
	// set RESTIC_REPOSITORY=D:\__BACKUP__\Papyrus\restic-repo-test01
	// set RESTIC_PASSWORD=repo-password
	//
	int    ok = 0;
	int    prr = 0;
	SString temp_buf;
	SlProcess prc;
	if(rP.Loc.IsEmpty()) {
		; // @todo @err // Directory is undefined
	}
	else if(!rP.Loc.IsLegalUtf8()) {
		; // @todo @err // Directory is not in utf8-encoding
	}
	else if(SFile::IsDir(rP.Loc)) {
		ok = -1; // @todo @err // Directory already exists // -1 - signal to not remove directory because the return is not greater than 0 
	}
	else {
		if(!Helper_CreateRepoDir(rP.Loc)) {
			; // @todo @err
		}
		/*else if(!GrantFullAccessForDirectoryToCurrentUser(rP.Loc)) {
			; // @todo @err
		}*/
		if(MakeProcessObj(rP, prc, true) > 0) {
			//prc.SetFlags(SlProcess::fReadOutputIntoInternalBuf);
			prc.AddArg("init");
			// (нет такой опции у init) prc.AddArg("--force");
			SlProcess::Result pr;
			prr = prc.Run(&pr);
			if(DebugLogFileName.NotEmpty()) {
				temp_buf.Z().Cat(pr.AppNameUtf8).Space().Cat(pr.CmdLineUtf8);
				PPLogMessage(DebugLogFileName, temp_buf, LOGMSGF_TIME|LOGMSGF_COMP);
			}
			if(prr) {
				ShowProcessOuput(pr, _FFFF32);
				if(true/*Helper_GrantRightToRepoPath(rP.Loc)*/) {
					ok = 1;
				}
				else {
					ok = 0;
				}
			}
		}
	}
	return ok;
}

int PPResticInterface::GetSnapshotList(const RepoParam & rP, TSCollection <Snapshot> * pResult)
{
	int    ok = -1;
	int    prr = 0;
	SString temp_buf;
	SlProcess prc;
	if(MakeProcessObj(rP, prc, true) > 0) {
		prc.AddArg("snapshots");
		prc.AddArg("--json");
		SlProcess::Result pr;
		prr = prc.Run(&pr);
		if(DebugLogFileName.NotEmpty()) {
			temp_buf.Z().Cat(pr.AppNameUtf8).Space().Cat(pr.CmdLineUtf8);
			PPLogMessage(DebugLogFileName, temp_buf, LOGMSGF_TIME|LOGMSGF_COMP);
		}
		if(prr) {
			ShowProcessOuput(pr, spofStdErr);
			TSCollection <SJson> js_list;
			if(ReadProcessJsonOutput(pr, js_list) > 0) {
				struct SnapshotEntry {
					UED    UedTime;
					binary256 Tree;
					SString HostName;
					SString UserName;
					SVerT  ResticVer;
					binary256 Id;
					uint32 ShortId;
					StringSet SsPaths;
					struct Summary {
						UED    BackupStartTm;
						UED    BackupEndTm;
						uint   FilesNew;
						uint   FilesChanged;
						uint   FilesUnmodified;
						uint   DirsNew;
						uint   DirsChanged;
						uint   DirsUnmodified;
						uint   DataBlobs;
						uint   TreeBlobs;
						uint   DataAdded;
						uint   DataAddedPacked;
						uint   TotalFilesProcessed;
						uint   TotalBytesProcessed;
					};
				};
				/*
					{
						"time": "2025-10-05T16:51:02.2343749+03:00",
						"tree": "47eb1ff8473ede2fefdf64b61b1d98e23ba98e30607481d81c29b01f060718ed",
						"paths": [
							"D:\\Papyrus\\ppy\\log"
						],
						"hostname": "sobolev7",
						"username": "PETROGLIF\\SOBOLEV",
						"program_version": "restic 0.18.0",
						"summary": {
							"backup_start": "2025-10-05T16:51:02.2343749+03:00",
							"backup_end": "2025-10-05T16:51:04.4125445+03:00",
							"files_new": 0,
							"files_changed": 0,
							"files_unmodified": 0,
							"dirs_new": 3,
							"dirs_changed": 0,
							"dirs_unmodified": 0,
							"data_blobs": 0,
							"tree_blobs": 4,
							"data_added": 1945,
							"data_added_packed": 1450,
							"total_files_processed": 0,
							"total_bytes_processed": 0
						},
						"id": "9ff715bc5a68599468bac7134940a547007f01687098cdd0016b2f28ca1b6bde",
						"short_id": "9ff715bc"
					},
				*/ 
				for(uint i = 0; i < js_list.getCount(); i++) {
					const SJson * p_js = js_list.at(i);
					if(SJson::IsArray(p_js)) {
						for(const SJson * p_js_item = p_js->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								
							}
						}
					}
				}
				ok = 1;
			}
		}
	}
	return ok;
}

int PPResticInterface::GetEntryList(const RepoParam & rP, uint32 snapshotShortId, TSCollection <Entry> * pResult)
{
	int    ok = -1;
	int    prr = 0;
	SString temp_buf;
	SlProcess prc;
	if(MakeProcessObj(rP, prc, true) > 0) {
		prc.AddArg("ls");
		if(snapshotShortId) {
			temp_buf.Z().CatHex(static_cast<ulong>(snapshotShortId));
			if(temp_buf.Len() < 8) {
				temp_buf.ShiftRight(8-temp_buf.Len(), '0');
			}
			prc.AddArg(temp_buf);
		}
		else {
			prc.AddArg("latest");
		}
		prc.AddArg("--json");
		SlProcess::Result pr;
		prr = prc.Run(&pr);
		if(DebugLogFileName.NotEmpty()) {
			temp_buf.Z().Cat(pr.AppNameUtf8).Space().Cat(pr.CmdLineUtf8);
			PPLogMessage(DebugLogFileName, temp_buf, LOGMSGF_TIME|LOGMSGF_COMP);
		}
		if(prr) {
			TSCollection <SJson> js_list;
			if(ReadProcessJsonOutput(pr, js_list) > 0) {
				;
			}
		}
	}
	return ok;
}

int PPResticInterface::Backup(const RepoParam & rP, bool dryMode)
{
	int    ok = 0;
	int    prr = 0;
	SString temp_buf;
	SlProcess prc;
	SString fn_incl;
	SString fn_excl;
	SString temp_path;
	PPGetPath(PPPATH_TEMP, temp_path);
	THROW(rP.Loc.NotEmpty()); // @todo @err
	THROW(rP.SsIncl.IsCountGreaterThan(0)); // @todo @err
	THROW(MakeProcessObj(rP, prc, false) > 0);
	{
		//restic -r D:\__BACKUP__\Papyrus\restic-repo-papyrus-main backup --files-from D:\__BACKUP__\Papyrus\restic-repo-papyrus-main-include --exclude-file D:\__BACKUP__\Papyrus\restic-repo-papyrus-main-exclude
		prc.AddArg("backup");
		if(dryMode) {
			prc.AddArg("--dry-run");
		}
		//prc.AddArg("--verbose");

		// "D:\\__BACKUP__\\Papyrus\\restic-repo-papyrus-main-include"
		// "D:\\__BACKUP__\\Papyrus\\restic-repo-papyrus-main-exclude"

		{
			{
				long tfc = 0;
				PPMakeTempFileName("restic-bu-incl", 0, &tfc, fn_incl);
				fn_incl.TrimRightChr('.');
				SFile f_temp(fn_incl, SFile::mWrite);
				THROW_SL(f_temp.IsValid());
				for(uint ssp = 0; rP.SsIncl.get(&ssp, temp_buf);) {
					THROW(temp_buf.IsLegalUtf8()); // @todo @err
					temp_buf.Strip().CR();
					THROW_SL(f_temp.WriteLine(temp_buf));
				}
			}
			prc.AddArg("--files-from");
			//fn_incl = "D:\\__BACKUP__\\Papyrus\\restic-repo-papyrus-main-include"; // @debug
			prc.AddArg(fn_incl);
		}
		if(rP.SsExcl.IsCountGreaterThan(0)) {
			long tfc = 0;
			PPMakeTempFileName("restic-bu-excl", 0, &tfc, fn_excl);
			fn_excl.TrimRightChr('.');
			SFile f_temp(fn_excl, SFile::mWrite);
			THROW_SL(f_temp.IsValid());
			for(uint ssp = 0; rP.SsExcl.get(&ssp, temp_buf);) {
				THROW(temp_buf.IsLegalUtf8()); // @todo @err
				temp_buf.Strip().CR();
				THROW_SL(f_temp.WriteLine(temp_buf));
			}
			prc.AddArg("--exclude-file");
			//fn_excl = "D:\\__BACKUP__\\Papyrus\\restic-repo-papyrus-main-exclude"; // @debug
			prc.AddArg(fn_excl);
		}
		prc.AddPrivilege(sprvlgSecurity);
		prc.AddPrivilege(sprvlgTCB);
		prc.AddPrivilege(sprvlgBackup);
		prc.AddPrivilege(sprvlgRestore);
		prc.AddPrivilege(sprvlgManageVolume);
		{
			//SDelay(500); // @debug
			if(P_Logger) {
				SString cd;
				SFile::GetCurrentDir(cd);
				temp_buf.Z().Cat("Current directory").CatDiv(':', 2).Cat(cd);
				P_Logger->Log(temp_buf);
			}
			SlProcess::Result pr;
			//prc.SetCurrentUserAsImpersUser(); // @experimental
			prr = prc.Run(&pr);
			if(prr) {
				::WaitForSingleObject(pr.HProcess, INFINITE);
			}
			if(DebugLogFileName.NotEmpty()) {
				temp_buf.Z().Cat(pr.AppNameUtf8).Space().Cat(pr.CmdLineUtf8);
				PPLogMessage(DebugLogFileName, temp_buf, LOGMSGF_TIME|LOGMSGF_COMP);
			}
			if(prr) {
				ShowProcessOuput(pr, spofStdErr);
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int TestRestic()
{
	int    ok = 1;
	SString temp_buf;
	PPLogger logger;
	PPResticInterface rifc(&logger);
	PPResticInterface::RepoParam rp;
	PPGetFilePath(PPPATH_BIN, "backup-config-01.ini", temp_buf);
	if(rp.ReadIniFile(temp_buf)) {
		SlProcess::EnablePrivilege(NULL, SE_RESTORE_NAME);
		SlProcess::EnablePrivilege(NULL, SE_BACKUP_NAME);
		//rp.Loc = "D:/__TEMP__/RESTIC-EXPERIMENTS/test-repo";
		//rp.Pw = "restic-repo-pw";
		//
		rifc.CreateRepo(rp);
		rifc.Backup(rp, false);
		rifc.GetSnapshotList(rp, 0);
		rifc.GetEntryList(rp, 0, 0);
	}
	return ok;
}
