// PPUTIL.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @Kernel
//
#include <pp.h>
#pragma hdrstop
// @v9.6.3 #include <idea.h>

// Prototype
DBFCreateFld * SLAPI LoadDBFStruct(uint rezID, uint * pNumFlds);

int FASTCALL dbl_cmp(double v1, double v2)
{
	const double diff = round(v1 - v2, 6);
	if(diff < 0)
		return -1;
	else if(diff > 0)
		return 1;
	else
		return 0;
}

IMPL_CMPFUNC(PPLBItem, i1, i2) { return stricmp866(((char*)i1)+sizeof(long), ((char*)i2)+sizeof(long)); }

IMPL_CMPFUNC(PPTLBItem, i1, i2)
{
	const PPID parent_id1 = *(long *)i1;
	const PPID parent_id2 = *(long *)i2;
	if(parent_id1 > parent_id2)
		return 1;
	else if(parent_id1 < parent_id2)
		return -1;
	int    cmp = stricmp866(((char*)i1)+sizeof(long)*2, ((char*)i2)+sizeof(long)*2);
	if(cmp > 0)
		return 1;
	else if(cmp < 0)
		return -1;
	return 0;
}

long SLAPI CheckXORFlags(long v, long f1, long f2) { return ((v & f1) ^ (v & f2)) ? ((v & f1) ? f1 : f2) : 0; }
long SLAPI SetXORFlags(long v, long f1, long f2, long f) { return ((v & ~(f1 | f2)) | f); }

int FASTCALL PPInitIterCounter(IterCounter & rCntr, DBTable * pTbl)
{
	RECORDNUMBER num_recs = 0;
	return (!pTbl || pTbl->getNumRecs(&num_recs)) ? (rCntr.Init(num_recs), 1) : PPSetErrorDB();
}

SString & SLAPI DateToStr(LDATE dt, SString & rBuf)
{
	rBuf.Z();
	if(dt) {
		//char   temp[256];//, txt_month[64];
		SString txt_month;
		SGetMonthText(dt.month(), MONF_CASEGEN, txt_month);
		//sprintf(temp, "%02d %s %d Ј.", dt.day(), /*getMonthText(dt.month(), MONF_CASEGEN|MONF_OEM, txt_month)*/txt_month.cptr(), dt.year());
		rBuf.CatLongZ((long)dt.day(), 2).Space().Cat(txt_month).Space().Cat(dt.year()).Space().Cat("г.");
		rBuf.Transf(CTRANSF_OUTER_TO_INNER);
	}
	return rBuf;
}

SString & SLAPI MoneyToStr(double nmb, long fmt, SString & rBuf)
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

SString & SLAPI VatRateStr(double rate, SString & rBuf)
{
	PPLoadString("vat", rBuf);
	return rBuf.Space().Cat(R0i(rate)).CatChar('%');
}

int FASTCALL PPGetSubStr(const char * pStr, int idx, SString & rDest)
{
	return rDest.GetSubFrom(pStr, ';', idx);
}

int FASTCALL PPGetSubStr(const char * pStr, int idx, char * pBuf, size_t bufLen)
{
	SString temp;
	int    ok = temp.GetSubFrom(pStr, ';', idx);
	temp.CopyTo(pBuf, bufLen);
	return ok;
}

int FASTCALL PPGetSubStrById(int strId, int subId, SString & rBuf)
{
	int    ok = 0;
	rBuf.Z();
	SString line_buf, item_buf, id_buf, txt_buf;
	if(PPLoadText(strId, line_buf)) {
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

int FASTCALL PPSearchSubStr(const char * pStr, int * pIdx, const char * pTestStr, int ignoreCase)
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

int FASTCALL PPCmpSubStr(const char * pStr, int idx, const char * pTestStr, int ignoreCase)
{
	SString temp;
	if(PPGetSubStr(pStr, idx, temp))
		if(__CompareStrings(temp, pTestStr, 0, ignoreCase))
			return 1;
	return 0;
}

int FASTCALL PPGetSubStr(uint strID, int idx, SString & rDest)
{
	SString temp;
	int    ok = PPLoadText(strID, temp) ? PPGetSubStr(temp, idx, rDest) : 0;
	if(!ok)
		rDest.Z();
	return ok;
}

int FASTCALL PPGetSubStr(uint strID, int idx, char * pBuf, size_t bufLen)
{
	SString temp;
	int    ok = PPLoadText(strID, temp) ? PPGetSubStr(temp, idx, pBuf, bufLen) : 0;
	if(!ok && pBuf)
		pBuf[0] = 0;
	return ok;
}

char * FASTCALL PPGetWord(uint wordId /* PPWORD_XXX */, int ansiCoding, char * pBuf, size_t bufLen)
{
	SString temp_buf;
	PPLoadText(wordId, temp_buf);
	if(ansiCoding)
		temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
	temp_buf.CopyTo(pBuf, bufLen);
	return pBuf;
}

SString & FASTCALL PPGetWord(uint wordId /* PPWORD_XXX */, int ansiCoding, SString & rBuf)
{
	PPLoadText(wordId, rBuf);
	if(ansiCoding)
		rBuf.Transf(CTRANSF_INNER_TO_OUTER);
	return rBuf;
}

SString & FASTCALL ideqvalstr(long id, SString & rBuf)
{
	return rBuf.CatEq("ID", id);
}

char * FASTCALL ideqvalstr(long id, char * pBuf, size_t bufLen)
{
	SString temp_buf;
	ideqvalstr(id, temp_buf).CopyTo(pBuf, bufLen);
	return pBuf;
}
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
PPDimention & PPDimention::Reset()
{
	Length = 0;
	Width = 0;
	Height = 0;
	return *this;
}

int    PPDimention::operator !() const { return BIN(Length == 0 && Width == 0 && Height == 0); }
int    FASTCALL PPDimention::IsEqual(const PPDimention & rS) const { return BIN(Length == rS.Length && Width == rS.Width && Height == rS.Height); }
int    FASTCALL PPDimention::operator == (const PPDimention & rS) const { return IsEqual(rS); }
int    FASTCALL PPDimention::operator != (const PPDimention & rS) const { return !IsEqual(rS); }
double SLAPI PPDimention::CalcVolumeM() const { return (fdiv1000i(Width) * fdiv1000i(Length) * fdiv1000i(Height)); }
double SLAPI PPDimention::CalcVolumeMM() const { return (Width * Length * Height); }

int SLAPI PPDimention::SetVolumeM(double volume)
{
	Width  = 100L;
	Height = 100L;
	Length = R0i(volume * fpow10i(5));
	return 1;
}

int SLAPI PPDimention::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
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
			else if(r == 0) {
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
			else if(r == 0) {
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
int FASTCALL SearchByKey(DBTable * pTbl, int idx, void * pKey, void * pData)
{
	int    ok = 1;
	if(pTbl->search(idx, pKey, spEq))
		pTbl->copyBufTo(pData);
	else
		ok = PPDbSearchError();
	return ok;
}

int FASTCALL SearchByKey_ForUpdate(DBTable * pTbl, int idx, void * pKey, void * pData)
{
	int    ok = 1;
	if(pTbl->searchForUpdate(idx, pKey, spEq))
		pTbl->copyBufTo(pData);
	else
		ok = PPDbSearchError();
	return ok;
}

int SLAPI PPSetDbRecordByKey(DBTable * pTbl, int idx, void * pKey, const void * pData, int use_ta)
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

int FASTCALL SearchByID(DBTable * pTbl, PPID objType, PPID id, void * b)
{
	int    ok = -1;
	if(pTbl) {
		if(id && pTbl->search(0, &id, spEq)) {
			pTbl->copyBufTo(b);
			ok = 1;
		}
		else if(!id || BTRNFOUND)
			ok = (PPSetObjError(PPERR_OBJNFOUND, objType, id), -1);
		else
			ok = PPSetErrorDB();
	}
	return ok;
}

int FASTCALL SearchByID_ForUpdate(DBTable * pTbl, PPID objType, PPID id, void * b)
{
	int    ok = -1;
	if(pTbl) {
		if(id && pTbl->searchForUpdate(0, &id, spEq)) {
			pTbl->copyBufTo(b);
			ok = 1;
		}
		else if(!id || BTRNFOUND)
			ok = (PPSetObjError(PPERR_OBJNFOUND, objType, id), -1);
		else
			ok = PPSetErrorDB();
	}
	return ok;
}

int FASTCALL AddByID(DBTable * tbl, PPID * pID, void * b, int use_ta)
{
	int    ok = 1;
	PPID   tmp_id = DEREFPTRORZ(pID);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		tbl->copyBufFrom(b);
		THROW_DB(tbl->insertRec(0, &tmp_id));
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pID, tmp_id);
	return ok;
}

int FASTCALL AdjustNewObjID(DBTable * tbl, PPID objType, void * b)
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
				*(long *)b = potential_key+inc;
			ok = 2;
		}
	}
	CATCHZOK
	return ok;
}

int FASTCALL AddObjRecByID(DBTable * tbl, PPID objType, PPID * pID, void * b, int use_ta)
{
	int    ok = 1;
	PPID   tmp_id = DEREFPTRORZ(pID);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ok = AdjustNewObjID(tbl, objType, b));
		tbl->copyBufFrom(b);
		THROW_DB(tbl->insertRec(0, &tmp_id));
		THROW(tra.Commit());
	}
	if(b)
		*(long *)b = tmp_id;
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

int FASTCALL UpdateByID(DBTable * pTbl, PPID objType, PPID id, void * b, int use_ta)
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

int FASTCALL UpdateByID_Cmp(DBTable * pTbl, PPID objType, PPID id, void * b, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchByID_ForUpdate(pTbl, objType, id, 0) > 0);
		if(!pTbl->GetFields().IsEqualRecords(b, pTbl->getDataBuf())) {
			THROW_DB(pTbl->updateRecBuf(b)); // @sfu
		}
		else
			ok = -1;
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int FASTCALL RemoveByID(DBTable * tbl, PPID id, int use_ta)
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

int FASTCALL IncDateKey(DBTable * tbl, int idx, LDATE date, long * pOprNo)
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

TempOrderTbl * SLAPI CreateTempOrderFile()
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

DBQ * FASTCALL ppcheckfiltid(DBQ * pDbq, DBItem & rDbi, PPID fltID)
{
	if(fltID)
		pDbq = &(*pDbq && rDbi == fltID);
	return pDbq;
}

DBQ * FASTCALL ppcheckfiltidlist(DBQ * pDbq, DBItem & rDbi, const PPIDArray * pList)
{
	if(pList)
		if(pList->isList())
			return &(*pDbq && ppidlist(rDbi, pList));
		else
			return ppcheckfiltid(pDbq, rDbi, pList->getSingle());
	else
		return pDbq;
}

DBQ * FASTCALL ppcheckflag(DBQ * pDbq, DBItem & rDbi, long mask, int test)
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

DBQ * FASTCALL ppcheckweekday(DBQ * pDbq, DBItem & rDbi, int dayOfWeek)
{
	if(dayOfWeek) {
		assert(rDbi.baseType() == BTS_DATE);
		DBE * p_dbe = new DBE;
		p_dbe->init();
		p_dbe->push(rDbi);
		p_dbe->push(dbq_weekday);
		pDbq = &(*pDbq && *p_dbe == (long)dayOfWeek);
		delete p_dbe;
	}
	return pDbq;
}
//
// Check functions
//
int FASTCALL CheckTblPtr(DBTable * tbl)
{
	if(tbl == 0)
		return PPSetErrorNoMem();
	else if(tbl->IsOpened() == 0)
		return PPSetErrorDB();
	else
		return 1;
}

int FASTCALL CheckQueryPtr(DBQuery * q)
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
SLAPI PPTblEnumList::PPTblEnumList()
{
}

SLAPI PPTblEnumList::~PPTblEnumList()
{
	for(uint i = 0; i < Tab.getCount(); i++)
		DestroyIterHandler((long)i);
}

int SLAPI PPTblEnumList::RegisterIterHandler(BExtQuery * pQ, long * pHandle)
{
	int    ok = 1;
	long   handle = -1;
	for(uint i = 0; handle < 0 && i < Tab.getCount(); i++)
		if(Tab.at(i) == 0) {
			Tab.at(i) = pQ;
			handle = (long)i;
		}
	if(handle < 0) {
		Tab.insert(&pQ);
		handle = (long)(Tab.getCount()-1);
	}
	ASSIGN_PTR(pHandle, handle);
	return ok;
}

int SLAPI PPTblEnumList::DestroyIterHandler(long handle)
{
	int    ok = -1;
	uint   pos = (uint)handle;
	if(pos < Tab.getCount()) {
		BExtQuery * p_q = (BExtQuery *)Tab.at(pos);
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
	uint   pos = (uint)handle;
	if(pos < Tab.getCount()) {
		BExtQuery * p_q = (BExtQuery *)Tab.at(pos);
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
PPID SLAPI GetSupplAccSheet()
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

PPID SLAPI GetSellAccSheet()
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

PPID SLAPI GetSellPersonKind()
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

PPID SLAPI GetAgentAccSheet()
{
	PPID   agent_acs_id = 0;
	PPThreadLocalArea & r_tla = DS.GetTLA();
	if(r_tla.AgentAccSheetID > 0) {
		agent_acs_id = r_tla.AgentAccSheetID;
	}
	else if(r_tla.AgentAccSheetID == 0) {
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		for(SEnum en = acs_obj.ref->Enum(PPOBJ_ACCSHEET, 0); !agent_acs_id && en.Next(&acs_rec) > 0;) {
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
	// Если BillObj == 0, то PPObjLocation::Fetch
	// пытается обратиться к нему и завешивает систему
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
		ok = GetObjectName(PPOBJ_LOCATION, locID, rBuf, 0);
	return ok;
}

SString & SLAPI GetExtLocationName(const ObjIdListFilt & rLocList, size_t maxItems, SString & rBuf)
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
				GetObjectName(PPOBJ_LOCATION, rLocList.Get().at(i), rBuf, 1);
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
	DbfTable     * p_tbl  = 0;
	DBFCreateFld * p_flds = 0;
	SString file_name = fName;
	if(::fileExists(file_name)) {
		if(forceReplace) {
			SPathStruc::ReplaceExt(file_name, "DBK", 1);
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

int FASTCALL CheckFiltID(PPID flt, PPID id)
{
	return (!flt || flt == id);
}

/* @v6.0.1 @unused
int SLAPI BarCodeCheckDigit(char * bc)
{
	int    p, c = 0, len = sstrlen(bc);
	for(p = 0; p < len; p += 2)
		c += bc[p] - '0';
	c *= 3;
	for(p = 1; p < len; p += 2)
		c += bc[p] - '0';
	return (10 - c % 10) % 10;
}
*/

char * FASTCALL QttyToStr(double qtty, double upp, long fmt, char * buf, int noabs)
{
	uint   f = SFMTFLAG(fmt);
	double ipart, fract;
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

SString & SLAPI GetCurSymbText(PPID curID, SString & rBuf)
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
SLAPI PPSymbTranslator::PPSymbTranslator(uint strID /*=PPSSYM_SYMB*/) : ErrorCode(0)
{
	if(!PPLoadString(PPSTR_SYMB, strID, Coll))
		ErrorCode = PPErrCode;
}

int SLAPI PPSymbTranslator::operator !() const
{
	return ErrorCode ? (PPErrCode = ErrorCode, 1) : 0;
}

/*
static char * SLAPI nextStr(const char * pColl, size_t * pPos, char * pBuf)
{
	size_t p = *pPos;
	if(pColl[p]) {
		char * b = pBuf;
		while(pColl[p] != 0 && pColl[p] != ';')
			*b++ = pColl[p++];
		*b = 0;
		if(pColl[p])
			p++;
		*pPos = p;
		return pBuf;
	}
	else
		return 0;
}
*/

char * SLAPI PPSymbTranslator::NextStr(size_t * pPos, char * pBuf) const
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

long SLAPI PPSymbTranslator::Translate(SStrScan & rScan)
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

long SLAPI PPSymbTranslator::Translate(const char * pString, size_t * pNextPos, uint /*flags*/)
{
	long   v = 0;
	int    count = 0;
	size_t max_len = 0;
	const size_t start_pos = DEREFPTRORZ(pNextPos);
	size_t p = start_pos;
	char * b, sub[256], temp[128];
	while(pString[p] == ' ' || pString[p] == '\t')
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

int SLAPI PPSymbTranslator::Retranslate(long sym, char * s, size_t bufLen) const
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
	} while((p = strchr(p, ';'))++ != 0);
	return PPSetError(PPERR_UNDEFSYMB);
}

int SLAPI PPSymbTranslator::Retranslate(long sym, SString & rBuf) const
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
	} while((p = strchr(p, ';'))++ != 0);
	return PPSetError(PPERR_UNDEFSYMB);
}
//
// DateIter
//
SLAPI DateIter::DateIter() : dt(ZERODATE), end(ZERODATE), oprno(0)
{
}

SLAPI DateIter::DateIter(long start, long finish)
{
	Init(start, finish);
}

SLAPI DateIter::DateIter(const DateRange * pPeriod)
{
	Init(pPeriod);
}

void SLAPI DateIter::Init()
{
	dt = ZERODATE;
	end = ZERODATE;
	oprno = 0;
}

void SLAPI DateIter::Init(long start, long finish)
{
	dt.v  = start;
	end.v = finish;
	oprno = 0;
}

void SLAPI DateIter::Init(const DateRange * pPeriod)
{
	if(pPeriod)
		Init(pPeriod->low, pPeriod->upp);
	else
		Init();
}

int FASTCALL DateIter::Advance(LDATE d, long o)
{
	dt = d;
	oprno = o;
	return IsEnd() ? -1 : 1;
}

int SLAPI DateIter::IsEnd() const
{
	return (end && dt > end);
}

int FASTCALL DateIter::Cmp(const DateIter & rS) const { RET_CMPCASCADE2(this, &rS, dt, oprno); }
//
// Loading DBF structure from resource
//
DBFCreateFld * SLAPI LoadDBFStruct(uint rezID, uint * pNumFlds)
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
			p_flds[i].Size = (uchar)p_rez->getUINT();
			p_flds[i].Prec = (uchar)p_rez->getUINT();
		}
	}
	CATCH
		ZDELETEARRAY(p_flds);
		num_flds = 0;
	ENDCATCH
	ASSIGN_PTR(pNumFlds, num_flds);
	return p_flds;
}

int FASTCALL LoadSdRecord(uint rezID, SdRecord * pRec, int headerOnly /*=0*/)
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
			fld.Init();
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
			SLS.ExpandString(fld.Descr, CTRANSF_UTF8_TO_OUTER); // @v9.1.4
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
SLAPI PPExtStringStorage::PPExtStringStorage() : Re("<[0-9]+>")
{
}

int SLAPI PPExtStringStorage::Excise(SString & rLine, int fldID)
{
	int    ok = -1;
	if(rLine.NotEmpty()) {
		SStrScan scan(rLine);
		SString temp_buf;
		while(ok <= 0 && Re.Find(&scan)) {
			scan.Get(temp_buf).TrimRight().ShiftLeft();
			const size_t tag_offs = scan.IncrLen();
			if(temp_buf.ToLong() == fldID) {
				if(Re.Find(&scan))
					rLine.Excise(tag_offs, scan.Offs - tag_offs);
				else
					rLine.Trim(tag_offs);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPExtStringStorage::Put(SString & rLine, int fldID, const char * pBuf)
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

int SLAPI PPExtStringStorage::Put(SString & rLine, int fldID, const SString & rBuf)
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

int SLAPI PPExtStringStorage::Get(const SString & rLine, int fldID, SString & rBuf)
{
	int    ok = -2;
	rBuf.Z();
	if(rLine.NotEmpty()) {
		SStrScan scan(rLine);
		SString & r_temp_buf = SLS.AcquireRvlStr(); // @v9.9.4
		while(ok <= 0 && Re.Find(&scan)) {
			scan.Get(r_temp_buf).TrimRight().ShiftLeft();
			size_t tag_offs = scan.IncrLen();
			if(r_temp_buf.ToLong() == fldID) {
				size_t start = scan.Offs;
				if(Re.Find(&scan))
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

int SLAPI PPExtStringStorage::Enum(const SString & rLine, uint * pPos, int * pFldID, SString & rBuf)
{
	rBuf.Z();

	int    ok = -1;
	int    fld_id = 0;
    uint   pos = DEREFPTRORZ(pPos);
    if(pos < rLine.Len()) {
		SStrScan scan(rLine);
		scan.Incr(pos);
		if(Re.Find(&scan)) {
			SString temp_buf;
			scan.Get(temp_buf).TrimRight().ShiftLeft();
			size_t tag_offs = scan.IncrLen();
			fld_id = temp_buf.ToLong();
			size_t start = scan.Offs;
			if(Re.Find(&scan)) {
				//pos = scan.Offs+scan.Len;
				pos = scan.Offs;
				rBuf.CopyFromN(rLine+start, scan.Offs-start);
			}
			else {
				pos = rLine.Len();
				rBuf.CopyFrom(rLine+start);
			}
			ok = 1;
		}
    }
    ASSIGN_PTR(pPos, pos);
    ASSIGN_PTR(pFldID, fld_id);
    return ok;
}

int FASTCALL PPGetExtStrData(int fldID, int defFldID, const SString & rLine, SString & rBuf)
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

int FASTCALL PPGetExtStrData(int fldID, const SString & rLine, SString & rBuf)
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

int FASTCALL PPCmpExtStrData(int fldID, const SString & rLine1, const SString & rLine2, long options)
{
    SString buf1, buf2;
    PPExtStringStorage ess;
    ess.Get(rLine1, fldID, buf1);
    ess.Get(rLine2, fldID, buf2);
	return buf1.Cmp(buf2, BIN(options & srchNoCase));
}

int FASTCALL PPPutExtStrData(int fldID, SString & rLine, const char * pBuf)
{
	PPExtStringStorage ess;
	return ess.Put(rLine, fldID, pBuf);
}

int FASTCALL PPPutExtStrData(int fldID, SString & rLine, const SString & rBuf)
{
	PPExtStringStorage ess;
	return ess.Put(rLine, fldID, rBuf);
}
//
//
//
SLAPI  PPExtStrContainer::PPExtStrContainer() {}
int    SLAPI PPExtStrContainer::GetExtStrData(int fldID, SString & rBuf) const { return PPGetExtStrData(fldID, ExtString, rBuf); }
int    SLAPI PPExtStrContainer::PutExtStrData(int fldID, const char * pStr) { return PPPutExtStrData(fldID, ExtString, pStr); }
int    SLAPI PPExtStrContainer::SerializeB(int dir, SBuffer & rBuf, SSerializeContext * pSCtx) { return pSCtx->Serialize(dir, ExtString, rBuf) ? 1 : PPSetErrorSLib(); }
void   FASTCALL PPExtStrContainer::SetBuffer(const char * pSrc) { ExtString = pSrc; }
const  SString & SLAPI PPExtStrContainer::GetBuffer() const { return ExtString; }
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
static int SLAPI _F2_(const BillTbl::Rec * pBillRec, const ArticleTbl::Rec * pArRec, ReferenceTbl::Rec * pRefRec)
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

int SLAPI PPChainDatabase(const char * pPassword)
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
		MEMSZERO(bill_rec);
		id_max = 0;
		r = p_bobj->P_Tbl->search(0, &id_max, spLast);
		if(r == 0) {
			if(BTRNFOUND)
				id_max = 1;
			else
				CALLEXCEPT_PP(PPERR_DBENGINE);
		}
		IdeaRandMem(&id_delta, sizeof(id_delta));
		id_delta = (labs(id_delta) % 23) + 1;
		bill_rec.ID = id_max + id_delta;

		IdeaRandMem(bill_rec.Code, sizeof(bill_rec.Code)-1);
		for(r = 0; r < sizeof(bill_rec.Code)-1; r++)
			bill_rec.Code[r] = (char)(labs(bill_rec.Code[r]) % 93 + 33);
		bill_rec.OpID = PPOPK_UNASSIGNED;

		// 3.
		MEMSZERO(ar_rec);
		id_max = 0;
		r = p_bobj->atobj->P_Tbl->Art.search(0, &id_max, spLast);
		THROW_DB(r || BTROKORNFOUND);
		if(r <= 0)
			id_max = 1;
		IdeaRandMem(&id_delta, sizeof(id_delta));
		id_delta = (labs(id_delta) % 37) + 1;
		ar_rec.ID = id_max + id_delta;

		IdeaRandMem(ar_rec.Name, sizeof(ar_rec.Name)-1);
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
			p_bobj->P_Tbl->copyBufFrom(&bill_rec);
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

static int SLAPI ProcessDatabaseChain(PPObjBill * pBObj, Reference * pRef, int mode, const char * pPassword, const char * pSrcEncPw, const char * pDestEncPw, int use_ta)
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
	IdeaRandMem(&ref_rec,  sizeof(ref_rec));
	IdeaRandMem(&ref_rec2, sizeof(ref_rec2));
	IdeaRandMem(&bill_rec, sizeof(bill_rec));
	IdeaRandMem(&ar_rec,   sizeof(ar_rec));
	pw.Obfuscate();
	return ok;
}

int SLAPI PPUnchainDatabase(const char * pPassword) { return ProcessDatabaseChain(BillObj, PPRef, pdbcmUnchain, pPassword, 0, 0, 1); }
int SLAPI PPCheckDatabaseChain() { return ProcessDatabaseChain(BillObj, PPRef, pdbcmVerify, 0, 0, 0, 1); }

int SLAPI PPReEncryptDatabaseChain(PPObjBill * pBObj, Reference * pRef, const char * pSrcEncPw, const char * pDestEncPw, int use_ta)
{
	return ProcessDatabaseChain(pBObj, pRef, pdbcmReEncrypt, 0, pSrcEncPw, pDestEncPw, use_ta);
}
//
//
//
static int SLAPI LoadDbqStringSubst(uint strID, size_t numItems, size_t strSize, uint8 * pBuf, void ** ppItems)
{
	for(uint idx = 0; idx < numItems; idx++) {
		char   item_buf[128];
		long   id = 0;
		char * p = 0;
		char   temp_buf[32];
		if(PPGetSubStr(strID, idx, item_buf, sizeof(item_buf)) > 0) {
			if((p = strchr(item_buf, ',')) != 0) {
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
		uint8 * ptr = pBuf+(sizeof(int16)+strSize)*idx;
		*(int16 *)ptr = (int16)id;
		strnzcpy((char *)(ptr+sizeof(int16)), p, strSize);
		ppItems[idx] = ptr;
	}
	return 1;
}

SLAPI DbqStringSubst::DbqStringSubst(size_t numItems) : NumItems(numItems), IsInited(0)
{
	Items = new Subst[NumItems];
	memzero(Items, sizeof(Subst) * NumItems);
	P_Items = new Subst*[NumItems];
	memzero(P_Items, sizeof(Subst*) * NumItems);
}

SLAPI DbqStringSubst::~DbqStringSubst()
{
	delete [] Items;
	delete [] P_Items;
}

void FASTCALL DbqStringSubst::Init(uint strID)
{
	if(!IsInited) {
		ENTER_CRITICAL_SECTION
		if(!IsInited)
			LoadDbqStringSubst(strID, NumItems, sizeof(((Subst *)0)->Str), (uint8 *)Items, (void **)P_Items);
		LEAVE_CRITICAL_SECTION
	}
}

char ** FASTCALL DbqStringSubst::Get(uint strID)
{
	Init(strID);
	return (char **)P_Items;
}
//
//
//
int SLAPI AdjustPeriodToSubst(SubstGrpDate sgd, DateRange * pPeriod)
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

int SLAPI ShrinkSubstDateExt(SubstGrpDate sgd, LDATE orgDt, LTIME orgTm, LDATE * pDestDt, LTIME * pDestTm)
{
	int    ok = 1;
	if(sgd == sgdHour) {
		int    h = orgTm.hour(), m = orgTm.minut();
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

int SLAPI ShrinkSubstDate(SubstGrpDate sgd, LDATE orgDt, LDATE * pDestDt)
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

int SLAPI ExpandSubstDateExt(SubstGrpDate sgd, LDATE dt, LTIME tm, DateRange * pPeriod, TimeRange * pTmPeriod)
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

int SLAPI ExpandSubstDate(SubstGrpDate sgd, LDATE dt, DateRange * pPeriod)
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

void SLAPI FormatSubstDateExt(SubstGrpDate sgd, LDATE dt, LTIME tm, SString & rBuf, long dtFmt /*=0*/, long tmFmt /*=0*/)
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

void SLAPI FormatSubstDate(SubstGrpDate sgd, LDATE dt, SString & rBuf, long fmt /*=0*/)
{
	char   buf[256];
	memzero(buf, sizeof(buf));
	FormatSubstDate(sgd, dt, buf, sizeof(buf));
	rBuf = buf;
}

void SLAPI FormatSubstDate(SubstGrpDate sgd, LDATE dt, char * pBuf, size_t bufLen, long fmt /*=0*/)
{
	char   temp[128], * p = temp;
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

static SString & SLAPIV Helper_PPFormat(const SString & rFmt, SString * pBuf, /*...*/va_list pArgList)
{
	enum {
		tokInt = 1,   // int
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
		tokLocAddr    // locaddr
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
						GetObjectName(PPOBJ_ACCOUNT2, obj_id, buf, 1);
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
							PPQuotKind qk_rec;
							if(qk_obj.Fetch(obj_id, &qk_rec) > 0)
								buf.Cat(qk_rec.Name);
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
						GetObjectName(PPOBJ_BILL, obj_id, buf, 1);
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
						GetObjectName(PPOBJ_TSESSION, obj_id, buf, 1);
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
					// @v8.2.8 {
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
					// } @v8.2.8
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

SString & SLAPIV PPFormat(const SString & rFmt, SString * pBuf, ...)
{
	va_list arg_list;
	va_start(arg_list, pBuf);
	Helper_PPFormat(rFmt, pBuf, arg_list);
	va_end(arg_list);
	return *pBuf;
}

SString & SLAPIV PPFormatT(int textCode, SString * pBuf, ...)
{
	SString fmt_buf;
	PPLoadText(textCode, fmt_buf);
	va_list arg_list;
	va_start(arg_list, pBuf);
	Helper_PPFormat(fmt_buf, pBuf, arg_list);
	va_end(arg_list);
	return *pBuf;
}

SString & SLAPIV PPFormatS(int textGroup, int textCode, SString * pBuf, ...)
{
	SString fmt_buf;
	PPLoadString(textGroup, textCode, fmt_buf);
	va_list arg_list;
	va_start(arg_list, pBuf);
	Helper_PPFormat(fmt_buf, pBuf, arg_list);
	va_end(arg_list);
	return *pBuf;
}

int SLAPI WaitForExists(const char * pPath, int whileExists /* = 1 */, int notifyTimeout /* = 5000 */)
{
	int    ok = 1, stop = 0;
	if(pPath) {
		int exists = fileExists(pPath) ? 1 : 0;
		if((exists && whileExists) || (!exists && !whileExists)) {
			DirChangeNotification * p_dc_notify = 0;
			SString    path = pPath;
			SPathStruc paths(path);
			if(paths.Nam.Len() == 0) {
				path.RmvLastSlash().Dot();
				paths.Split(path);
			}
			paths.Merge(0, SPathStruc::fNam|SPathStruc::fExt, (path = 0));
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

int SLAPI WaitNewFile(const char * pDir, SString & rFile, int notifyTimeout /* =5000 */)
{
	int    ok = 1, stop = 0;
	if(pDir) {
		int exists = 0;
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
					if(!entry.IsFolder() && cmp(entry.AccessTime, beg_dtm) > 0) {
						(rFile = pDir).SetLastSlash().Cat(entry.FileName);
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

int FASTCALL __CopyFileByPath(const char * pSrcPath, const char * pDestPath, const char * pFileName)
{
	SString src, dest;
	(src = pSrcPath).SetLastSlash().Cat(pFileName);
	(dest = pDestPath).SetLastSlash().Cat(pFileName);
	return fileExists(src) ? copyFileByName(src, dest) : PPSetError(PPERR_NOSRCFILE, src);
}

int FASTCALL CopyDataStruct(const char *pSrc, const char *pDest, const char *pFileName)
{
	return __CopyFileByPath(pSrc, pDest, pFileName) ? 1 : PPErrorZ();
}

int SLAPI PPValidateXml()
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
int SLAPI XMLFillDTDEntitys(void * pWriter)
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
/*
	Проверка правильности указания корреспондентского счёта:

	Алгоритм проверки корреспондентского счёта с помощью БИКа банка:
	1. Для проверки контрольной суммы перед корреспондентским счётом добавляются "0" и два знака БИКа банка, начиная с пятого знака.
	2. Вычисляется контрольная сумма со следующими весовыми коэффициентами: (7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1)
	3. Вычисляется контрольное число как остаток от деления контрольной суммы на 10
	4. Контрольное число сравнивается с нулём. В случае их равенства корреспондентский счёт считается правильным.
*/
int CheckCorrAcc(const char * pCode, const char * pBic)
{
	int    ok = 0;
	const size_t len = sstrlen(pCode);
	const size_t bic_len = sstrlen(pBic);
	if(len == 20 && bic_len >= 6) {
		int    r = 1;
		size_t i;
		for(i = 0; r && i < len; i++) {
			if(pCode[i] < '0' || pCode[i] > '9')
				r = 0;
		}
		if(r) {
			const int8 w[] = {7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1};
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
/*
	Проверка правильности указания расчётного счёта:

	Алгоритм проверки расчётного счёта с помощью БИКа банка:
	1. Для проверки контрольной суммы перед расчётным счётом добавляются три последние цифры БИКа банка.
	2. Вычисляется контрольная сумма со следующими весовыми коэффициентами: (7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1)
	3. Вычисляется контрольное число как остаток от деления контрольной суммы на 10
	4. Контрольное число сравнивается с нулём. В случае их равенства расчётного счёт считается правильным.
*/
int CheckBnkAcc(const char * pCode, const char * pBic)
{
	int    ok = 0;
	const size_t len = sstrlen(pCode);
	const size_t bic_len = sstrlen(pBic);
	if(len == 20 && bic_len >= 3) {
		int    r = 1;
		size_t i;
		for(i = 0; r && i < len; i++) {
			if(pCode[i] < '0' || pCode[i] > '9')
				r = 0;
		}
		if(r) {
			const int8 w[] = {7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1,3,7,1};
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
	Проверка правильности указания ОКПО:

	Алгоритм проверки ОКПО:
	1. Вычисляется контрольная сумма по 7-и цифрам со следующими весовыми коэффициентами: (1,2,3,4,5,6,7).
	2. Вычисляется контрольное число(1) как остаток от деления контрольной суммы на 11.
	3. Вычисляется контрольная сумма по 7-и цифрам со следующими весовыми коэффициентами: (3,4,5,6,7,8,9).
	4. Вычисляется контрольное число(2) как остаток от деления контрольной суммы на 11.
		Если остаток от деления равен 10-ти, то контрольному числу(2) присваивается ноль.
	5. Если контрольное число(1) больше девяти, то восьмой знак ОКПО сравнивается с контрольным числом(2),
		иначе восьмой знак ОКПО сравнивается с контрольным числом(1). В случае их равенства ОКПО считается правильным.
*/
int CheckOKPO(const char * pCode)
{
	int    ok = 0;
	size_t i;
	const  size_t len = sstrlen(pCode);
	if(len == 8) {
		int    r = 1;
		for(i = 0; r && i < len; i++) {
			if(pCode[i] < '0' || pCode[i] > '9')
				r = 0;
		}
		if(r) {
			const int8 w1[] = {1,2,3,4,5,6,7};
			const int8 w2[] = {3,4,5,6,7,8,9};
			ulong  sum1 = 0, sum2 = 0;
			for(i = 0; i < len-1; i++) {
				sum1 += (w1[i] * (pCode[i]-'0'));
			}
			for(i = 0; i < len-1; i++) {
				sum2 += (w2[i] * (pCode[i]-'0'));
			}
			int    cd1 = (sum1 % 11);
			int    cd2 = (sum2 % 11);
			if(cd2 == 10)
				cd2 = 0;
			if(cd1 > 9)
				ok = BIN((pCode[len-1]-'0') == cd2);
			else
				ok = BIN((pCode[len-1]-'0') == cd1);
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
#if 0 // @v8.7.4 Замещено фунцией SCalcCheckDigit()
int CheckINN(const char * pCode)
{
	int    ok = 0;
	const size_t len = sstrlen(pCode);
	size_t i;
	if(len) {
		int    r = 1;
		for(i = 0; r && i < len; i++) {
			if(pCode[i] < '0' || pCode[i] > '9')
				r = 0;
		}
		if(r) {
			if(len == 10) {
				const int8 w[] = {2,4,10,3,5,9,4,6,8,0};
				ulong  sum = 0;
				for(i = 0; i < len; i++) {
					sum += (w[i] * (pCode[i]-'0'));
				}
				int    cd = (sum % 11) % 10;
				ok = BIN((pCode[len-1]-'0') == cd);
			}
			else if(len == 12) {
				const int8 w1[] = {7,2,4,10,3,5,9,4,6,8,0};
				const int8 w2[] = {3,7,2,4,10,3,5,9,4,6,8,0};
				ulong  sum1 = 0, sum2 = 0;
				for(i = 0; i < len-1; i++) {
					sum1 += (w1[i] * (pCode[i]-'0'));
				}
				for(i = 0; i < len; i++) {
					sum2 += (w2[i] * (pCode[i]-'0'));
				}
				int    cd1 = (sum1 % 11) % 10;
				int    cd2 = (sum2 % 11) % 10;
				ok = BIN((pCode[len-2]-'0') == cd1 && (pCode[len-1]-'0') == cd2);
			}
		}
	}
	return ok;
}
#endif // } 0 @v8.7.4
//
//
//
#if 0 // {

extern "C" __declspec(dllexport) int cdecl UnixToDos(const char * pWildcard, long flags)
{
	int    ok = -1;
	SString file_name, temp_file_name, line_buf, msg_buf;
	SString test_line_buf;
	SDirEntry entry;
	SPathStruc ps(pWildcard);
	for(SDirec dir(pWildcard, 0); dir.Next(&entry) > 0;) {
		ps.Nam = entry.FileName;
		ps.Ext.Z();
		ps.Merge(0, file_name);
		SFile file_in(file_name, SFile::mRead);
		if(file_in.IsValid()) {
			int    test_ok = 1;

			ps.Nam.Z();
			ps.Ext.Z();
			ps.Merge(0, temp_file_name);
			MakeTempFileName(temp_file_name, "U2D", "TMP", 0, temp_file_name);

			SFile file_out(temp_file_name, SFile::mWrite|SFile::mBinary);
			THROW_SL(file_out.IsValid());
			while(file_in.ReadLine(line_buf)) {
				THROW_SL(file_out.WriteLine(line_buf.Chomp().CRB()));
			}
			file_out.Close();
			file_in.Close();
			{
				SFile file_test_in(file_name, SFile::mRead);
				SFile file_test_out(temp_file_name, SFile::mRead);
				THROW_SL(file_test_in.IsValid());
				THROW_SL(file_test_out.IsValid());
				while(file_test_in.ReadLine(line_buf)) {
					if(!file_test_out.ReadLine(test_line_buf) || line_buf.Chomp().Cmp(test_line_buf.Chomp(), 0) != 0)
						test_ok = 0;
				}
				if(test_ok && file_test_out.ReadLine(test_line_buf) != 0) {
					test_ok = 0;
				}
				if(test_ok) {
					file_test_in.Close();
					file_test_out.Close();
					THROW_SL(SFile::Remove(file_name));
					THROW_SL(SFile::Rename(temp_file_name, file_name));
					fprintf(stderr, "File '%s' was converted\n", (const char *)file_name);
				}
				else {
					fprintf(stderr, "Error comparing original file '%s' with converted file '%s'\n",
						(const char *)file_name, (const char *)temp_file_name);
				}
			}
		}
		else {
			PPSetErrorSLib();
			PPGetLastErrorMessage(1, msg_buf);
			fprintf(stderr, msg_buf);
		}
	}
	CATCH
		PPGetLastErrorMessage(1, msg_buf);
		fprintf(stderr, msg_buf);
		ok = 0;
	ENDCATCH
	return ok;
}

#endif // } 0

//
//
//
#include <ppsoapclient.h>

//static
int SLAPI PPUhttClient::TestUi_GetLocationListByPhone()
{
	int    ok = -1;
	PPLogger logger;
	SString phone_buf, log_buf;
	while(InputStringDialog(0, phone_buf) > 0) {
		PPUhttClient uhtt_cli;
		TSCollection <UhttLocationPacket> uhtt_loc_list;
		THROW(uhtt_cli.Auth());
		{
			int r = uhtt_cli.GetLocationListByPhone(phone_buf, uhtt_loc_list);
			if(r > 0 && uhtt_loc_list.getCount()) {
				for(uint i = 0; i < uhtt_loc_list.getCount(); i++) {
					const UhttLocationPacket * p_uhtt_loc_item = uhtt_loc_list.at(i);
					log_buf.Z().
						CatEq("id", (long)p_uhtt_loc_item->ID).CatDiv(';', 2).
						CatEq("phone", p_uhtt_loc_item->Phone).CatDiv(';', 2).
						CatEq("contact", p_uhtt_loc_item->Contact);
					logger.Log(log_buf);
				}
			}
			else if(r == 0) {
				logger.LogLastError();
			}
			else {
				logger.Log(log_buf = "nothing");
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

//static
int SLAPI PPUhttClient::TestUi_GetQuotByLoc()
{
	int    ok = -1;
	PPLogger logger;
	SString code_buf, log_buf;
	while(InputStringDialog(0, code_buf) > 0) {
		PPUhttClient uhtt_cli;
		THROW(uhtt_cli.Auth());
		{
			UhttLocationPacket uhtt_loc;
            if(uhtt_cli.GetLocationByCode(code_buf, uhtt_loc)) {
				UhttQuotFilter filt;
				TSCollection <UhttQuotPacket> uhtt_list;
				filt.LocationID = uhtt_loc.ID;
				int r = uhtt_cli.GetQuot(filt, uhtt_list);
				if(r > 0 && uhtt_list.getCount()) {
					for(uint i = 0; i < uhtt_list.getCount(); i++) {
						const UhttQuotPacket * p_uhtt_item = uhtt_list.at(i);
						log_buf.Z().
							CatEq("goodsid", (long)p_uhtt_item->GoodsID).CatDiv(';', 2).
							CatEq("value", p_uhtt_item->Value).CatDiv(';', 2).
							CatEq("loc", (long)p_uhtt_item->LocID);
						logger.Log(log_buf);
					}
				}
				else if(r == 0) {
					logger.LogLastError();
				}
				else {
					logger.Log(log_buf = "nothing");
				}
            }
            else {
            	logger.LogLastError();
            }
		}
	}
	CATCHZOKPPERR
	return ok;
}

SLAPI PPUhttClient::PPUhttClient() : State(0), P_DestroyFunc(0)
{
	SString temp_buf;
 	{
		PPGetFilePath(PPPATH_BIN, "PPSoapUhtt.dll", temp_buf);
		P_Lib = new SDynLibrary(temp_buf);
		if(P_Lib && !P_Lib->IsValid()) {
			ZDELETE(P_Lib);
		}
		if(P_Lib) {
			P_DestroyFunc = (void *)P_Lib->GetProcAddr("UhttDestroyResult");
		}
	}
	PPAlbatrosConfig cfg;
	DS.FetchAlbatrosConfig(&cfg);
	//Urn = cfg.UhttUrn.NotEmpty() ? (const char *)cfg.UhttUrn : 0; // "urn:http.service.universehtt.ru";
	cfg.GetExtStrData(ALBATROSEXSTR_UHTTURN, temp_buf);
	Urn = temp_buf.NotEmpty() ? temp_buf.cptr() : 0; // "urn:http.service.universehtt.ru";
	//UrlBase = cfg.UhttUrlPrefix.NotEmpty() ? cfg.UhttUrlPrefix.cptr() : 0; //"http://uhtt.ru/UHTTDispatcher/axis/Plugin_UHTT_SOAPService";
	cfg.GetExtStrData(ALBATROSEXSTR_UHTTURLPFX, temp_buf);
	UrlBase = temp_buf.NotEmpty() ? temp_buf.cptr() : 0; //"http://uhtt.ru/UHTTDispatcher/axis/Plugin_UHTT_SOAPService";
	cfg.GetExtStrData(ALBATROSEXSTR_UHTTACC, temp_buf);
	//if(cfg.UhttAccount.NotEmpty())
	if(temp_buf.NotEmpty())
		State |= stHasAccount;
}

SLAPI PPUhttClient::~PPUhttClient()
{
}

int SLAPI PPUhttClient::HasAccount() const
{
	return (State & stHasAccount);
}

int SLAPI PPUhttClient::IsAuth() const
{
	return (State & stAuth);
}

int SLAPI PPUhttClient::PreprocessResult(const void * pResult, const PPSoapClientSession & rSess)
{
	LastMsg = rSess.GetMsg();
    return BIN(pResult);
}

int SLAPI PPUhttClient::Auth()
{
	Token.Z();
	State &= ~stAuth;
	int    ok = 1;
	if(P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTAUTH_PROC func = (UHTTAUTH_PROC)P_Lib->GetProcAddr("UhttAuth");
		if(func) {
			SString pw;
			sess.Setup(UrlBase);
			PPAlbatrosConfig cfg;
			DS.FetchAlbatrosConfig(&cfg);
			cfg.GetPassword(ALBATROSEXSTR_UHTTPASSW, pw);
			cfg.GetExtStrData(ALBATROSEXSTR_UHTTACC, temp_buf);
			SString * p_token = func(sess, /*cfg.UhttAccount*/temp_buf, pw.Transf(CTRANSF_INNER_TO_UTF8));
			(pw = 0).Align(64, ALIGN_RIGHT); // Забиваем пароль пробелами
			if(PreprocessResult(p_token, sess)) {
				Token = *p_token;
				State |= stAuth;
				DestroyResult((void **)&p_token);
			}
			else
				ok = PPSetError(PPERR_UHTTSVCFAULT, LastMsg);
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetLocationByID(int id, UhttLocationPacket & rResult)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETLOCATIONBYID_PROC func = (UHTTGETLOCATIONBYID_PROC)P_Lib->GetProcAddr("UhttGetLocationByID");
		if(func) {
			sess.Setup(UrlBase);
			UhttLocationPacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rResult = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetLocationByCode(const char * pCode, UhttLocationPacket & rResult)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETLOCATIONBYCODE_PROC func = (UHTTGETLOCATIONBYCODE_PROC)P_Lib->GetProcAddr("UhttGetLocationByCode");
		if(func) {
			sess.Setup(UrlBase);
			UhttLocationPacket * p_result = func(sess, Token, pCode);
			if(PreprocessResult(p_result, sess)) {
				rResult = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetLocationListByPhone(const char * pPhone, TSCollection <UhttLocationPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETLOCATIONLISTBYPHONE_PROC func = (UHTTGETLOCATIONLISTBYPHONE_PROC)P_Lib->GetProcAddr("UhttGetLocationListByPhone");
		if(func) {
			sess.Setup(UrlBase);
			TSCollection <UhttLocationPacket> * p_result = func(sess, Token, pPhone);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetBrandByName(const char * pName, TSCollection <UhttBrandPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTGETBRANDBYNAME_PROC func = (UHTTGETBRANDBYNAME_PROC)P_Lib->GetProcAddr("UhttGetBrandByName");
		if(func) {
			sess.Setup(UrlBase);
			(temp_buf = pName).Transf(CTRANSF_INNER_TO_UTF8);
			TSCollection <UhttBrandPacket> * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

//typedef int (*UHTTGETGOODSREFLIST_PROC)(PPSoapClientSession & rSess, const char * pToken, TSArray <UhttCodeRefItem> & rList);

int SLAPI PPUhttClient::GetUhttGoodsRefList(LAssocArray & rList, StrAssocArray * pByCodeList)
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
			const PPID goods_id = labs(rList.at(i).Key);
			goods_obj.ReadBarcodes(goods_id, bc_list);
			if(bc_list.getCount()) {
				for(uint j = 0; j < bc_list.getCount(); j++) {
					const BarcodeTbl::Rec & r_bc_item = bc_list.at(j);
					if(sstrlen(r_bc_item.Code) != 19) { // @v10.0.0 Алкогольные коды пропускаем
						assert(goods_id == r_bc_item.GoodsID); // @paranoic
						UhttCodeRefItem ref_item;
						ref_list.insert(&ref_item.Set(goods_id, r_bc_item.Code));
					}
				}
			}
		}
	}
	if(ref_list.getCount()) {
		if(State & stAuth && P_Lib) {
			PPSoapClientSession sess;
			UHTTGETGOODSREFLIST_PROC func = (UHTTGETGOODSREFLIST_PROC)P_Lib->GetProcAddr("UhttGetGoodsRefList");
			if(func) {
				sess.Setup(UrlBase);
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

int SLAPI PPUhttClient::GetUhttGoodsList(PPID goodsID, long flags, TSCollection <UhttGoodsPacket> & rResult)
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

int SLAPI PPUhttClient::GetUhttGoods(PPID goodsID, long flags, int * pUhttID, UhttGoodsPacket * pUhttResult)
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

int SLAPI PPUhttClient::ResolveGoodsByUhttID(int uhttID, UhttGoodsPacket * pUhttResult, PPID * pGoodsID, SString * pCode)
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

int SLAPI PPUhttClient::GetPersonByID(int id, UhttPersonPacket & rResult)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETPERSONBYID_PROC func = (UHTTGETPERSONBYID_PROC)P_Lib->GetProcAddr("UhttGetPersonByID");
		if(func) {
			sess.Setup(UrlBase);
			UhttPersonPacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rResult = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetPersonByName(const char * pName, TSCollection <UhttPersonPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTGETPERSONBYNAME_PROC func = (UHTTGETPERSONBYNAME_PROC)P_Lib->GetProcAddr("UhttGetPersonByName");
		if(func) {
			sess.Setup(UrlBase);
			(temp_buf = pName).Transf(CTRANSF_INNER_TO_UTF8);
			TSCollection <UhttPersonPacket> * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetGoodsByID(int id, UhttGoodsPacket & rResult)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETGOODSBYID_PROC func = (UHTTGETGOODSBYID_PROC)P_Lib->GetProcAddr("UhttGetGoodsByID");
		if(func) {
			sess.Setup(UrlBase);
			UhttGoodsPacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rResult = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetGoodsByCode(const char * pCode, TSCollection <UhttGoodsPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETGOODSBYCODE_PROC func = (UHTTGETGOODSBYCODE_PROC)P_Lib->GetProcAddr("UhttGetGoodsByCode");
		if(func) {
			sess.Setup(UrlBase);
			TSCollection <UhttGoodsPacket> * p_result = func(sess, Token, pCode);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetGoodsByName(const char * pName, TSCollection <UhttGoodsPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTGETGOODSBYNAME_PROC func = (UHTTGETGOODSBYNAME_PROC)P_Lib->GetProcAddr("UhttGetGoodsByName");
		if(func) {
			sess.Setup(UrlBase);
			(temp_buf = pName).Transf(CTRANSF_INNER_TO_UTF8);
			TSCollection <UhttGoodsPacket> * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetGoodsArCode(const char * pBarcode, const char * pPersonINN, SString & rArCode)
{
	int    ok = 0;
	rArCode.Z();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString bcode, inn;
		UHTTGETGOODSARCODE_PROC func = (UHTTGETGOODSARCODE_PROC)P_Lib->GetProcAddr("UhttGetGoodsArCode");
		if(func) {
			sess.Setup(UrlBase);
			(bcode = pBarcode).Transf(CTRANSF_INNER_TO_UTF8);
			(inn = pPersonINN).Transf(CTRANSF_INNER_TO_UTF8);
			SString * p_result = func(sess, Token, bcode, inn);
			if(PreprocessResult(p_result, sess)) {
				rArCode = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetGoodsRestList(int uhttGoodsID, TSCollection <UhttGoodsRestListItem> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString bcode, inn;
		UHTTGETGOODSRESTLIST_PROC func = (UHTTGETGOODSRESTLIST_PROC)P_Lib->GetProcAddr("UhttGetGoodsRestList");
		if(func) {
			sess.Setup(UrlBase);
			TSCollection <UhttGoodsRestListItem> * p_result = func(sess, Token, uhttGoodsID);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::CreateStandaloneLocation(long * pID, const UhttLocationPacket & rPack)
{
	int    ok = 0;
	int    id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTCREATESTANDALONELOCATION_PROC func = (UHTTCREATESTANDALONELOCATION_PROC)P_Lib->GetProcAddr("UhttCreateStandaloneLocation");
		if(func) {
			sess.Setup(UrlBase);
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult((void **)&p_result);
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int SLAPI PPUhttClient::CreateGoods(long * pID, const UhttGoodsPacket & rPack)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTCREATEGOODS_PROC func = (UHTTCREATEGOODS_PROC)P_Lib->GetProcAddr("UhttCreateGoods");
		if(func) {
			sess.Setup(UrlBase);
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult((void **)&p_result);
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int SLAPI PPUhttClient::SetObjImage(const char * pObjTypeSymb, PPID uhttObjID, const char * pFileName)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTSETIMAGEBYID_PROC func = (UHTTSETIMAGEBYID_PROC)P_Lib->GetProcAddr("UhttSetImageByID");
		if(func) {
			UhttDocumentPacket doc_pack;
			doc_pack.SetFile(pFileName);
			doc_pack.ObjTypeSymb = pObjTypeSymb;
			doc_pack.UhttObjID = uhttObjID;
			sess.Setup(UrlBase);
			UhttStatus * p_result = func(sess, Token, doc_pack);
			if(PreprocessResult(p_result, sess)) {
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

// @Muxa {
int SLAPI PPUhttClient::GetSpecSeriesByPeriod(const char * pPeriod, TSCollection <UhttSpecSeriesPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTGETSPECSERIESBYPERIOD_PROC func = (UHTTGETSPECSERIESBYPERIOD_PROC)P_Lib->GetProcAddr("UhttGetSpecSeriesByPeriod");
		if(func) {
			sess.Setup(UrlBase);
			(temp_buf = pPeriod).Transf(CTRANSF_INNER_TO_UTF8);
			TSCollection <UhttSpecSeriesPacket> * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::CreateSpecSeries(long * pID, const UhttSpecSeriesPacket & rPack)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTCREATESPECSERIES_PROC func = (UHTTCREATESPECSERIES_PROC)P_Lib->GetProcAddr("UhttCreateSpecSeries");
		if(func) {
			sess.Setup(UrlBase);
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult((void **)&p_result);
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int SLAPI PPUhttClient::GetSCardByNumber(const char * pNumber, UhttSCardPacket & rResult)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTGETSCARDBYNUMBER_PROC func = (UHTTGETSCARDBYNUMBER_PROC)P_Lib->GetProcAddr("UhttGetSCardByNumber");
		if(func) {
			sess.Setup(UrlBase);
			(temp_buf = pNumber).Transf(CTRANSF_INNER_TO_UTF8);
			UhttSCardPacket * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				rResult = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
			else
				PPSetError(PPERR_UHTTSVCFAULT, LastMsg);
		}
	}
	return ok;
}

int SLAPI PPUhttClient::CreateBill(UhttBillPacket & rPack)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTCREATEBILL_PROC func = (UHTTCREATEBILL_PROC)P_Lib->GetProcAddr("UhttCreateBill");
		if(func) {
			sess.Setup(UrlBase);
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

int SLAPI PPUhttClient::GetBill(const UhttBillFilter & rFilt, TSCollection <UhttBillPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTGETBILL_PROC func = (UHTTGETBILL_PROC)P_Lib->GetProcAddr("UhttGetBill");
		if(func) {
			sess.Setup(UrlBase);
			TSCollection <UhttBillPacket> * p_result = func(sess, Token, rFilt);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetQuot(const UhttQuotFilter & rFilt, TSCollection <UhttQuotPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTGETQUOT_PROC func = (UHTTGETQUOT_PROC)P_Lib->GetProcAddr("UhttGetQuot");
		if(func) {
			sess.Setup(UrlBase);
			TSCollection <UhttQuotPacket> * p_result = func(sess, Token, rFilt);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::SetQuot(const UhttQuotPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTSETQUOT_PROC func = (UHTTSETQUOT_PROC)P_Lib->GetProcAddr("UhttSetQuot");
		if(func) {
			sess.Setup(UrlBase);
			ok = func(sess, Token, rPack);
			if(!PreprocessResult((void *)ok, sess)) {
				PPSetError(PPERR_UHTTSVCFAULT, LastMsg);
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::SetQuotList(const TSCollection <UhttQuotPacket> & rList, TSCollection <UhttStatus> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTSETQUOTLIST_PROC func = (UHTTSETQUOTLIST_PROC)P_Lib->GetProcAddr("UhttSetQuotList");
		if(func) {
			sess.Setup(UrlBase);
			TSCollection <UhttStatus> * p_result = func(sess, Token, rList);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
				ok = 1;
			}
			else
				PPSetError(PPERR_UHTTSVCFAULT, LastMsg);
		}
	}
	return ok;
}

int SLAPI PPUhttClient::CreateSCardCheck(const char * pLocSymb, const char * pSCardNumber, const UhttCheckPacket & rPack)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTCREATESCARDCHECK_PROC func = (UHTTCREATESCARDCHECK_PROC)P_Lib->GetProcAddr("UhttCreateSCardCheck");
		if(func) {
			sess.Setup(UrlBase);
			UhttStatus * p_result = func(sess, Token, pLocSymb, pSCardNumber, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult((void **)&p_result);
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::DepositSCardAmount(const char * pNumber, const double amount)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTDEPOSITSCARDAMOUNT_PROC func = (UHTTDEPOSITSCARDAMOUNT_PROC)P_Lib->GetProcAddr("UhttDepositSCardAmount");
		if(func) {
			sess.Setup(UrlBase);
			UhttStatus * p_result = func(sess, Token, pNumber, amount);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					ok = 1;
				}
				else {
					PPSetError(PPERR_UHTTSVCFAULT, (LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER));
				}
				DestroyResult((void **)&p_result);
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::WithdrawSCardAmount(const char * pNumber, const double amount)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTWITHDRAWSCARDAMOUNT_PROC func = (UHTTWITHDRAWSCARDAMOUNT_PROC)P_Lib->GetProcAddr("UhttWithdrawSCardAmount");
		if(func) {
			sess.Setup(UrlBase);
			UhttStatus * p_result = func(sess, Token, pNumber, amount);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					ok = 1;
				}
				else {
					PPSetError(PPERR_UHTTSVCFAULT, (LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER));
				}
				DestroyResult((void **)&p_result);
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetSCardRest(const char * pNumber, const char * pDate, double & rRest)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTGETSCARDREST_PROC func = (UHTTGETSCARDREST_PROC)P_Lib->GetProcAddr("UhttGetSCardRest");
		if(func) {
			sess.Setup(UrlBase);
			int    result = func(sess, Token, pNumber, pDate, rRest);
			if(PreprocessResult((void *)BIN(result == 1), sess))
				ok = 1;
		}
	}
	return ok;
}

// } @Muxa

int SLAPI PPUhttClient::FileVersionAdd(const char * pFileName, const char * pKey,
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
		SString temp_buf;
		//typedef int (*UHTTADDFILEVERSION_PROC)(PPSoapClientSession & rSess, const char * pToken, int transferID, const char * pKey, const char * pLabel, const char * pMemo);
		UHTTADDFILEVERSION_PROC func = (UHTTADDFILEVERSION_PROC)P_Lib->GetProcAddr("UhttAddFileVersion");
		if(func) {
			int64 file_size = 0;
			SFile f(pFileName, SFile::mRead|SFile::mBinary);
			THROW_SL(f.IsValid());
			THROW_SL(f.CalcSize(&file_size));
			{
				SPathStruc ps;
				const size_t chunk_size = (size_t)MIN((3*64*1024), file_size);
				const size_t tail_size = (size_t)(file_size % chunk_size);
				const size_t chunk_count = (size_t)(file_size / chunk_size); // Количество отрезков одинакового размера (без хвоста)
				STempBuffer buffer(chunk_size);
				THROW_SL(buffer.IsValid());
				ps.Split(pFileName);
				ps.Merge(SPathStruc::fNam|SPathStruc::fExt, temp_buf);
				temp_buf.ToUtf8();
				THROW(StartTransferData(temp_buf, file_size, chunk_count + BIN(tail_size), &transfer_id));
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

int SLAPI PPUhttClient::StartTransferData(const char * pName, int64 totalRawSize, int32 chunkCount, int * pTransferID)
{
	int    ok = 0;
	int    transfer_id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString temp_buf;
		UHTTSTARTDATATRANSFER_PROC func = (UHTTSTARTDATATRANSFER_PROC)P_Lib->GetProcAddr("UhttStartDataTransfer");
		if(func) {
			sess.Setup(UrlBase);
			(temp_buf = pName).Transf(CTRANSF_INNER_TO_UTF8);
			transfer_id = func(sess, Token, temp_buf, totalRawSize, chunkCount);
			if(PreprocessResult((void *)transfer_id, sess))
				ok = 1;
		}
	}
	ASSIGN_PTR(pTransferID, transfer_id);
	return ok;
}

int SLAPI PPUhttClient::TransferData(int transferID, int chunkNumber, size_t rawChunkSize, const void * pBinaryChunkData)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString temp_buf;
		UHTTTRANSFERDATA_PROC func = (UHTTTRANSFERDATA_PROC)P_Lib->GetProcAddr("UhttTransferData");
		if(func) {
			sess.Setup(UrlBase);
			temp_buf.EncodeMime64(pBinaryChunkData, (size_t)rawChunkSize);
			int    result = func(sess, Token, transferID, chunkNumber, (int64)rawChunkSize, (char *)(const char *)temp_buf); // @badcast
			if(PreprocessResult((void *)result, sess))
				ok = 1;
		}
	}
	return ok;
}

int SLAPI PPUhttClient::FinishTransferData(int transferID)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		// typedef int (*UHTTFINISHTRANSFERDATA_PROC)(PPSoapClientSession & rSess, const char * pToken, int transferID);
		UHTTFINISHTRANSFERDATA_PROC func = (UHTTFINISHTRANSFERDATA_PROC)P_Lib->GetProcAddr("UhttFinishTransferData");
		if(func) {
			sess.Setup(UrlBase);
			int    result = func(sess, Token, transferID);
			if(PreprocessResult((void *)result, sess))
				ok = 1;
		}
	}
	return ok;
}

void FASTCALL PPUhttClient::DestroyResult(void ** ppResult)
{
	if(P_DestroyFunc) {
		((UHTT_DESTROYRESULT)P_DestroyFunc)(*ppResult);
		*ppResult = 0;
	}
}
//
//
//
int SLAPI PPUhttClient::ConvertLocationPacket(const UhttLocationPacket & rUhttPack, LocationTbl::Rec & rLocRec) const
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

int SLAPI PPUhttClient::ConvertPersonPacket(const UhttPersonPacket & rUhttPack, PPID kindID, PPPersonPacket & rPsnPack) const
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

int SLAPI PPUhttClient::GetVersionList(const char * pKey, TSCollection <UhttDCFileVersionInfo> & rResult, SVerT * pMinVer)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString bcode, inn;
		UHTTDCGETFILEVERSIONLIST_PROC func = (UHTTDCGETFILEVERSIONLIST_PROC)P_Lib->GetProcAddr("UhttDCGetFileVersionList");
		if(func) {
			sess.Setup(UrlBase);
			TSCollection <UhttDCFileVersionInfo> * p_result = func(sess, Token, pKey);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
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

// static
int SLAPI PPUhttClient::ViewNewVerList(int showSelDlg)
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
				ListBoxSelDialog(&list, PPTXT_TITLE_NEWVERLIST, 0, 0);
			}
			ok = 1;
		}
	}
	return ok;
}
//
//
//
int SLAPI PPUhttClient::GetWorkbookItemByID(int id, UhttWorkbookItemPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETWORKBOOKITEMBYID_PROC func = (UHTTGETWORKBOOKITEMBYID_PROC)P_Lib->GetProcAddr("UhttGetWorkbookItemByID");
		if(func) {
			sess.Setup(UrlBase);
			UhttWorkbookItemPacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetWorkbookContentByID_ToFile(int id, const char * pFileName)
{
	int    ok = 0;
	SString file_name_to_remove;
	UhttDocumentPacket * p_result = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETWORKBOOKCONTENTBYID_PROC func = (UHTTGETWORKBOOKCONTENTBYID_PROC)P_Lib->GetProcAddr("UhttGetWorkbookContentByID");
		if(func) {
			sess.Setup(UrlBase);
			UhttDocumentPacket * p_result = func(sess, Token, id);
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
				DestroyResult((void **)&p_result);
				p_result = 0;
				ok = 1;
			}
		}
	}
	CATCH
		if(p_result)
			DestroyResult((void **)&p_result);
		if(file_name_to_remove.NotEmpty())
			SFile::Remove(file_name_to_remove);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPUhttClient::GetWorkbookItemByCode(const char * pCode, UhttWorkbookItemPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETWORKBOOKITEMBYCODE_PROC func = (UHTTGETWORKBOOKITEMBYCODE_PROC)P_Lib->GetProcAddr("UhttGetWorkbookItemByCode");
		if(func) {
			sess.Setup(UrlBase);
			UhttWorkbookItemPacket * p_result = func(sess, Token, pCode);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetWorkbookListByParentCode(const char * pParentCode, TSCollection <UhttWorkbookItemPacket> & rResult)
{
	// UHTTGETWORKBOOKLISTBYPARENTCODE_PROC
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETWORKBOOKLISTBYPARENTCODE_PROC func = (UHTTGETWORKBOOKLISTBYPARENTCODE_PROC)P_Lib->GetProcAddr("UhttGetWorkbookListByParentCode");
		if(func) {
			sess.Setup(UrlBase);
			TSCollection <UhttWorkbookItemPacket> * p_result = func(sess, Token, pParentCode);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::CreateWorkbookItem(long * pID, const UhttWorkbookItemPacket & rPack)
{
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTCREATEWORKBOOKITEM_PROC func = (UHTTCREATEWORKBOOKITEM_PROC)P_Lib->GetProcAddr("UhttCreateWorkbookItem");
		if(func) {
			sess.Setup(UrlBase);
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult((void **)&p_result);
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int SLAPI PPUhttClient::SetWorkbookContentByID(int id, const char * pFileName)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTSETWORKBOOKCONTENTBYID_PROC func = (UHTTSETWORKBOOKCONTENTBYID_PROC)P_Lib->GetProcAddr("UhttSetWorkbookContentByID");
		if(func) {
			UhttDocumentPacket doc_pack;
			doc_pack.SetFile(pFileName);
			doc_pack.ObjTypeSymb = "WORKBOOK";
			doc_pack.UhttObjID = id;
			sess.Setup(UrlBase);
			UhttStatus * p_result = func(sess, Token, doc_pack);
			if(PreprocessResult(p_result, sess)) {
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetStyloDeviceByID(int id, UhttStyloDevicePacket & rPack)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETSTYLODEVICEBYID_PROC func = (UHTTGETSTYLODEVICEBYID_PROC)P_Lib->GetProcAddr("UhttGetStyloDeviceByID");
		if(func) {
			sess.Setup(UrlBase);
			UhttStyloDevicePacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetStyloDeviceByCode(const char * pCode, UhttStyloDevicePacket & rPack)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETSTYLODEVICEBYCODE_PROC func = (UHTTGETSTYLODEVICEBYCODE_PROC)P_Lib->GetProcAddr("UhttGetStyloDeviceByCode");
		if(func) {
			SString temp_buf;
			sess.Setup(UrlBase);
			(temp_buf = pCode).Transf(CTRANSF_INNER_TO_UTF8);
			UhttStyloDevicePacket * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::CreateStyloDevice(long * pID, const UhttStyloDevicePacket & rPack)
{
	int    ok = 0;
	int    id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTCREATESTYLODEVICE_PROC func = (UHTTCREATESTYLODEVICE_PROC)P_Lib->GetProcAddr("UhttCreateStyloDevice");
		if(func) {
			sess.Setup(UrlBase);
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult((void **)&p_result);
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int SLAPI PPUhttClient::GetProcessorByID(long id, UhttProcessorPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETPROCESSORBYID_PROC func = (UHTTGETPROCESSORBYID_PROC)P_Lib->GetProcAddr("UhttGetProcessorByID");
		if(func) {
			sess.Setup(UrlBase);
			UhttProcessorPacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetProcessorByCode(const char * pCode, UhttProcessorPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETPROCESSORBYCODE_PROC func = (UHTTGETPROCESSORBYCODE_PROC)P_Lib->GetProcAddr("UhttGetProcessorByCode");
		if(func) {
			SString temp_buf;
			sess.Setup(UrlBase);
			(temp_buf = pCode).Transf(CTRANSF_INNER_TO_UTF8);
			UhttProcessorPacket * p_result = func(sess, Token, temp_buf);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::CreateProcessor(long * pID, const UhttProcessorPacket & rPack)
{
	int    ok = 0;
	int    id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTCREATEPROCESSOR_PROC func = (UHTTCREATEPROCESSOR_PROC)P_Lib->GetProcAddr("UhttCreateProcessor");
		if(func) {
			sess.Setup(UrlBase);
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult((void **)&p_result);
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int SLAPI PPUhttClient::GetTSessionByID(long id, UhttTSessionPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETTSESSIONBYID_PROC func = (UHTTGETTSESSIONBYID_PROC)P_Lib->GetProcAddr("UhttGetTSessionByID");
		if(func) {
			sess.Setup(UrlBase);
			UhttTSessionPacket * p_result = func(sess, Token, id);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetTSessionByUUID(const S_GUID & rUuid, UhttTSessionPacket & rPack)
{
	int    ok = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETTSESSIONBYUUID_PROC func = (UHTTGETTSESSIONBYUUID_PROC)P_Lib->GetProcAddr("UhttGetTSessionByUUID");
		if(func) {
			sess.Setup(UrlBase);
			UhttTSessionPacket * p_result = func(sess, Token, rUuid);
			if(PreprocessResult(p_result, sess)) {
				rPack = *p_result;
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::GetTSessionByPrc(long prcID, const LDATETIME & rSince, TSCollection <UhttTSessionPacket> & rResult)
{
	int    ok = 0;
	rResult.freeAll();
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTGETTSESSIONBYPRC_PROC func = (UHTTGETTSESSIONBYPRC_PROC)P_Lib->GetProcAddr("UhttGetTSessionByPrc");
		if(func) {
			sess.Setup(UrlBase);
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
				DestroyResult((void **)&p_result);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPUhttClient::CreateTSession(long * pID, const UhttTSessionPacket & rPack)
{
	int    ok = 0;
	int    id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		SString lib_path;
		SString temp_buf, url;
		UHTTCREATETSESSION_PROC func = (UHTTCREATETSESSION_PROC)P_Lib->GetProcAddr("UhttCreateTSession");
		if(func) {
			sess.Setup(UrlBase);
			UhttStatus * p_result = func(sess, Token, rPack);
			if(PreprocessResult(p_result, sess)) {
				if(p_result->Code > 0) {
					id = p_result->Code;
					ok = 1;
				}
				else {
					(LastMsg = p_result->Msg).Transf(CTRANSF_UTF8_TO_INNER);
				}
				DestroyResult((void **)&p_result);
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}
//
//
//
int SLAPI TestCURL()
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

int SLAPI PPUhttClient::SendSms(const TSCollection <UhttSmsPacket> & rList, TSCollection <UhttStatus> & rResult)
{
	rResult.freeAll();
	int    ok = 0;
	PPID   id = 0;
	if(State & stAuth && P_Lib) {
		PPSoapClientSession sess;
		UHTTSENDSMS_PROC func = (UHTTSENDSMS_PROC)P_Lib->GetProcAddr("UhttSendSms");
		if(func) {
			sess.Setup(UrlBase);
			TSCollection <UhttStatus> * p_result = func(sess, Token, rList);
			if(PreprocessResult(p_result, sess)) {
				TSCollection_Copy(rResult, *p_result);
				DestroyResult((void **)&p_result);
				ok = 1;
			}
			else {
				PPSetError(PPERR_UHTTSVCFAULT, LastMsg.Transf(CTRANSF_UTF8_TO_INNER));
			}
		}
	}
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
PPXmlFileDetector::PPXmlFileDetector() : SXmlSaxParser(SXmlSaxParser::fStartElement), 
	ElementCount(0), Result(0), P_ShT(PPGetStringHash(PPSTR_HASHTOKEN))
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

//virtual 
int PPXmlFileDetector::StartElement(const char * pName, const char ** ppAttrList)
{
	ElementCount++;
	int    ok = 1;
    int    tok = 0;
	int    do_continue = 0;
    if(P_ShT) {
		uint   _ut = 0;
		uint   colon_pos = 0;
		SString & r_temp_buf = SLS.AcquireRvlStr();
		(r_temp_buf = pName).ToLower();
		if(r_temp_buf.StrChr(':', &colon_pos))
			r_temp_buf.ShiftLeft(colon_pos+1);
		P_ShT->Search(r_temp_buf, &_ut, 0);
		tok = _ut;
		if(ElementCount == 1) {
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
			}
		}
    }
	if(!do_continue) {
		SaxStop();
	}
	return ok;
}

