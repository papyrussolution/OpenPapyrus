// PPARRAY.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8 // @v10.4.5
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
//
//
SLAPI PPCycleFilt::PPCycleFilt() : Cycle(0), NumCycles(0)
{
}

PPCycleFilt & SLAPI PPCycleFilt::Z()
{
	Cycle = 0;
	NumCycles = 0;
	return *this;
	//*reinterpret_cast<uint32 *>(this) = 0;
}

int FASTCALL PPCycleFilt::operator == (const PPCycleFilt & rS) const
{
	return (Cycle == rS.Cycle && NumCycles == rS.NumCycles);
}

int SLAPI PPCycleFilt::operator !() const
{
	return (Cycle == 0);
}
//
// PPCycleArray
//
SLAPI PPCycleArray::PPCycleArray() : TSVector <DateRange> (), Cycle(0), NumCycles(0) // @v9.8.4 TSArray-->TSVector
{
	Period.Z();
}

SLAPI PPCycleArray::PPCycleArray(LDATE beg, LDATE end, int cycle, int numCycles) : TSVector <DateRange> () // @v9.8.4 TSArray-->TSVector
{
	init(beg, end, cycle, numCycles);
}

int SLAPI PPCycleArray::init(LDATE beg, LDATE end, int cycle, int numCycles)
{
	Period.Set(beg, end);
	Cycle  = abs(cycle);
	NumCycles = numCycles;
	freeAll();
	if(Cycle == 0) {
		insert(&Period);
	}
	else {
		int    fw = BIN(cycle >= 0);
		if(Period.low == 0) {
			if(Period.upp == 0) {
				#ifdef __IE__
				Period.upp = LConfig.OperDate;
				#else
				Period.upp = getcurdate_();
				#endif
			}
			fw = 0;
		}
		if(NumCycles == 0) {
			if((fw && Period.upp == 0) || (!fw && Period.low == 0))
				NumCycles = 1;
		}
		LDATE  dt = fw ? Period.low : Period.upp;
		for(int i = 0; (NumCycles ? (i < NumCycles) : (fw ? (dt <= Period.upp) : (dt >= Period.low))); i++) {
			DateRange c;
			if(fw) {
				c.low = dt;
				plusperiod(&dt, Cycle, 1, 0);
				c.upp = plusdate(dt, -1);
				if(!insert(&c))
					return PPSetErrorSLib();
			}
			else {
				c.upp = dt;
				plusperiod(&dt, Cycle, -1, 0);
				c.low = plusdate(dt, 1);
				if(!atInsert(0, &c))
					return PPSetErrorSLib();
			}
		}
		if(getCount()) {
			if(fw) {
				dt = at(getCount() - 1).upp;
				if(Period.upp && dt > Period.upp && getCount() > 1) {
					atFree(getCount()-1);
					dt = at(getCount() - 1).upp;
				}
				Period.upp = dt;
			}
			else {
				dt = at(0).low;
				if(Period.low && dt < Period.low && getCount() > 1) {
					atFree(0);
					dt = at(0).low;
				}
				Period.low = dt;
			}
		}
	}
	SETIFZ(NumCycles, getCount());
	return 1;
}

int SLAPI PPCycleArray::init(const DateRange * pRange, int cycle, int numCycles)
{
	return init(pRange->low, pRange->upp, cycle, numCycles);
}

int SLAPI PPCycleArray::init(const DateRange * pRange, PPCycleFilt cycl)
{
	return init(pRange->low, pRange->upp, cycl.Cycle, cycl.NumCycles);
}

int SLAPI PPCycleArray::init2(DateRange * pPeriod, PPCycleFilt * pCycl)
{
	int    ok = -1;
	if(!*pCycl)
		freeAll();
	else {
		init(pPeriod, *pCycl);
		getCycleParams(pPeriod, pCycl);
		ok = 1;
	}
	return ok;
}

int SLAPI PPCycleArray::concat(const PPCycleArray * pSrc)
{
	int    ok = 1;
	if(Cycle == pSrc->Cycle) { // @v10.3.0 @fix (Cycle = pSrc->Cycle)-->(Cycle == pSrc->Cycle)
		Period.Set(MIN(Period.low, pSrc->Period.low), MAX(Period.upp, pSrc->Period.upp));
		NumCycles += pSrc->NumCycles;
		for(uint i = 0; i < pSrc->getCount(); i++)
			ordInsert(&pSrc->at(i), 0, PTR_CMPFUNC(LDATE));
	}
	else
		ok = 0;
	return ok;
}

void SLAPI PPCycleArray::getCycleParams(DateRange * pRange, PPCycleFilt * pCycl) const
{
	ASSIGN_PTR(pRange, Period);
	if(pCycl) {
		pCycl->Cycle = Cycle;
		pCycl->NumCycles = NumCycles;
	}
}

int SLAPI PPCycleArray::getPeriod(uint pos, DateRange * pPeriod) const
{
	int    ok = 1;
	DateRange dr;
	if(pos < count)
		dr = at(pos);
	else {
		dr.Z();
		ok = 0;
	}
	ASSIGN_PTR(pPeriod, dr);
	return ok;
}

int SLAPI PPCycleArray::searchDate(LDATE dt, uint * pPos) const
{
	ASSIGN_PTR(pPos, 0);
	for(uint i = 0; i < getCount(); i++) {
		if(at(i).CheckDate(dt)) {
			ASSIGN_PTR(pPos, i);
			return 1;
		}
		else if(dt < at(i).low)
			return 0;
	}
	return 0;
}

int SLAPI PPCycleArray::searchPeriodByDate(LDATE dt, DateRange * pPeriod) const
{
	uint   pos = 0;
	return searchDate(dt, &pos) ? getPeriod(pos, pPeriod) : -1;
}

void SLAPI PPCycleArray::formatCycle(LDATE dt, char * pBuf, size_t bufLen) const
{
	SString temp_buf;
	uint   pos = 0;
	int    d, m, y;
	if(searchDate(dt, &pos)) {
		DateRange period = at(pos);
		if(Cycle == PRD_DAY)
			temp_buf.Cat(dt, DATF_DMY | DATF_CENTURY);
		else if(Cycle == PRD_WEEK)
			temp_buf.Cat(pos+1).Cat("-я нед.");
		else if(Cycle == PRD_MONTH) {
			if(dt.day() == 1) {
				SString month_txt;
				SGetMonthText(dt.month(), MONF_SHORT, month_txt);
				temp_buf.Cat(/*getMonthText(dt.month(), MONF_SHORT, month_txt)*/month_txt).Space().Cat(dt.year());
			}
			else
				temp_buf.Cat(pos+1).Cat("-й мес");
		}
		else if(Cycle == PRD_QUART) {
			decodedate(&d, &m, &y, &period.low);
			if(d == 1 && oneof4(m, 1, 4, 7, 10)) {
				int    q = (dt.month()-1) / 3;
				if(q == 0)
					temp_buf.CatChar('I');
				else if(q == 1)
					temp_buf.CatCharN('I', 2);
				else if(q == 2)
					temp_buf.CatCharN('I', 3);
				else if(q == 3)
					temp_buf.CatChar('I').CatChar('V');
				temp_buf.Space().Cat(dt.year());
			}
			else
				temp_buf.Cat(pos+1).Cat("-й кв.");
		}
		else if(Cycle == PRD_SEMIAN) {
			decodedate(&d, &m, &y, &period.low);
			if(d == 1 && (m == 1 || m == 7))
				temp_buf.Cat(((dt.month()-1) / 6) + 1).Cat("-е полугод").Space().Cat(dt.year());
			else
				temp_buf.Cat(pos+1).Cat("-е полугод.");
		}
		else if(Cycle == PRD_ANNUAL) {
			decodedate(&d, &m, &y, &period.low);
			if(d == 1 && m == 1)
				temp_buf.Cat(y);
			else
				temp_buf.Cat(pos+1).Cat("-й год");
		}
	}
	temp_buf.Transf(CTRANSF_UTF8_TO_INNER); // @v10.4.5 CTRANSF_OUTER_TO_INNER-->CTRANSF_UTF8_TO_INNER
	strnzcpy(pBuf, temp_buf, bufLen);
}

int SLAPI PPCycleArray::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, reinterpret_cast<SVector *>(this), rBuf)); // @v10.3.9 @fix (SArray-->SVector)
	THROW_SL(pCtx->Serialize(dir, Period.low, rBuf));
	THROW_SL(pCtx->Serialize(dir, Period.upp, rBuf));
	THROW_SL(pCtx->Serialize(dir, Cycle, rBuf));
	THROW_SL(pCtx->Serialize(dir, NumCycles, rBuf));
	CATCHZOK
	return ok;
}
//
//
//
SLAPI ObjRestrictItem::ObjRestrictItem() : ObjID(0), Flags(0)
{
}

SLAPI ObjRestrictItem::ObjRestrictItem(PPID objID, long flags) : ObjID(objID), Flags(flags)
{
}

SLAPI ObjRestrictArray::ObjRestrictArray() : /*TSArray*/TSVector <ObjRestrictItem> ()
{
}

SLAPI ObjRestrictArray::ObjRestrictArray(const ObjRestrictArray & s) : /*TSArray*/TSVector <ObjRestrictItem> (s)
{
}

ObjRestrictArray & FASTCALL ObjRestrictArray::operator = (const ObjRestrictArray & s)
{
	copy(s);
	return *this;
}

int FASTCALL ObjRestrictArray::Helper_MergeItems(const ObjRestrictArray * pS)
{
	int    ok = 1;
	if(pS) {
		for(uint i = 0; i < pS->getCount(); i++) {
			const ObjRestrictItem & r_si = pS->at(i);
			uint  _p = 0;
			if(SearchItemByID(r_si.ObjID, &_p)) {
				at(_p).Flags |= r_si.Flags;
			}
			else {
				insert(&r_si);
			}
		}
	}
	return ok;
}

int SLAPI ObjRestrictArray::Merge(const ObjRestrictArray * pS, long options)
{
	int    ok = 1;
    if(options & moEmptyListIsFull) {
		if(pS == 0 || pS->getCount() == 0) {
            freeAll();
            ok = 100;
		}
		else
			Helper_MergeItems(pS);
    }
    else if(SVectorBase::GetCount(pS))
		Helper_MergeItems(pS);
    return ok;
}

int SLAPI ObjRestrictArray::Add(PPID objID, long flags, uint * pPos)
{
	ObjRestrictItem item(objID, flags);
	if(insert(&item)) {
		ASSIGN_PTR(pPos, getCount()-1);
		return 1;
	}
	else
		return PPSetErrorSLib();
}

int FASTCALL ObjRestrictArray::CheckUniqueID(PPID objID) const
{
	return SearchItemByID(objID, 0) ? 0 : 1;
}

int SLAPI ObjRestrictArray::UpdateItemByID(PPID objID, long itemFlags)
{
	int    ok = 1;
	uint   pos = 0;
	if(SearchItemByID(objID, &pos))
		at(pos).Flags = itemFlags;
	else
		ok = Add(objID, itemFlags, 0);
	return ok;
}

int FASTCALL ObjRestrictArray::RemoveItemByID(PPID objID)
{
	uint   pos = 0;
	if(SearchItemByID(objID, &pos)) {
		atFree(pos);
		return 1;
	}
	else
		return -1;
}

int SLAPI ObjRestrictArray::SearchItemByID(PPID objID, uint * pPos) const
{
	return lsearch(&objID, pPos, CMPF_LONG);
}

int SLAPI ObjRestrictArray::CheckFlag(PPID objID, long f) const
{
	uint   p = 0;
	return BIN(lsearch(&objID, &p, CMPF_LONG) && at(p).Flags & f);
}
//
//
//
SLAPI ObjIdListFilt::ObjIdListFilt() : P_List(0)
{
}

SLAPI ObjIdListFilt::ObjIdListFilt(const ObjIdListFilt & s) : P_List(0)
{
	Set(s.P_List);
}

SLAPI ObjIdListFilt::~ObjIdListFilt()
{
	Set(0);
}

ObjIdListFilt & FASTCALL ObjIdListFilt::operator = (const ObjIdListFilt & s)
{
	if(&s != this)
		Set(s.P_List);
	return *this;
}

ObjIdListFilt & FASTCALL ObjIdListFilt::operator = (long value)
{
	CALLPTRMEMB(P_List, clear());
	Add(value);
	return *this;
}

int FASTCALL ObjIdListFilt::IsEqual(const ObjIdListFilt & s) const
{
	return P_List ? P_List->IsEqual(s.P_List) : (s.P_List ? 0 : 1);
}

ObjIdListFilt & SLAPI ObjIdListFilt::InitEmpty()
{
	SETIFZ(P_List, new PPIDArray);
	return *this;
}

int SLAPI ObjIdListFilt::IsEmpty() const
{
	return BIN(!P_List || !P_List->getCount() || (P_List->getCount() == 1 && P_List->at(0) == 0));
}

int FASTCALL ObjIdListFilt::CheckID(PPID id) const
{
	return BIN(!P_List || P_List->lsearch(id));
}

int FASTCALL ObjIdListFilt::Get(PPIDArray & rResult) const
{
	int    ok = 1;
	rResult.clear();
	if(P_List && P_List->getCount()) {
		/* @v9.8.5 for(uint i = 0; i < P_List->getCount(); i++)
			rResult.add(P_List->get(i));*/
		rResult = *P_List; // @v9.8.5
		rResult.sortAndUndup();
	}
	else
		ok = -1;
	return ok;
}

PPID SLAPI ObjIdListFilt::GetSingle() const
{
	return P_List ? P_List->getSingle() : 0;
}

PPID FASTCALL ObjIdListFilt::Get(uint pos) const
{
	return (P_List && pos >= 0 && pos < P_List->getCount()) ? P_List->at(pos) : 0;
}

int SLAPI ObjIdListFilt::SetSingle(PPID id)
{
	Set(0);
	return Add(id);
}

uint SLAPI ObjIdListFilt::IncPointer()
{
	return P_List ? P_List->incPointer() : 0;
}

int FASTCALL ObjIdListFilt::CopyTo(PPIDArray * pList) const
{
	if(pList)
		if(P_List)
			*pList = *P_List;
		else
			pList->clear(); // @v10.6.3 freeAll()-->clear()
	return 1;
}

void FASTCALL ObjIdListFilt::Set(const PPIDArray * pList)
{
	ZDELETE(P_List);
	if(pList)
		P_List = new PPIDArray(*pList);
}

#if 0 // {
int SLAPI ObjIdListFilt::Add(PPID id, int ignoreZero /* = 1 */)
{
	int    ok = -1;
	if(!ignoreZero || id != 0) {
		if(!SETIFZ(P_List, new PPIDArray))
			ok = PPSetErrorNoMem();
		else
			ok = P_List->addUnique(id);
	}
	return ok;
}
#endif // } 0

int FASTCALL ObjIdListFilt::Add(PPID id)
{
	int    ok = -1;
	if(id != 0) {
		if(!SETIFZ(P_List, new PPIDArray))
			ok = PPSetErrorNoMem();
		else
			ok = P_List->addUnique(id);
	}
	return ok;
}

int FASTCALL ObjIdListFilt::AddNotIgnoringZero(PPID id)
{
	int    ok = -1;
	if(!SETIFZ(P_List, new PPIDArray))
		ok = PPSetErrorNoMem();
	else
		ok = P_List->addUnique(id);
	return ok;
}

int SLAPI ObjIdListFilt::Update(uint pos, PPID newId, int ignoreZero /* = 1 */)
{
	int    ok = -1;
	if(!ignoreZero || newId != 0) {
		if(!SETIFZ(P_List, new PPIDArray))
			ok = PPSetErrorNoMem();
		else if(pos >= 0 && pos < P_List->getCount())
			ok = P_List->at(pos) = newId;
	}
	return ok;
}

int FASTCALL ObjIdListFilt::Add(const PPIDArray * pList)
{
	int    ok = -1;
	if(pList && pList->getCount()) {
		if(!SETIFZ(P_List, new PPIDArray))
			ok = PPSetErrorNoMem();
		else
			ok = P_List->addUnique(pList);
	}
	return ok;
}

int SLAPI ObjIdListFilt::Remove(PPID id, int binary /*= 0 */)
{
	uint p = 0;
	return (Search(id, &p, binary) > 0) ? P_List->atFree(p) : -1;
}

int SLAPI ObjIdListFilt::RemoveByIdx(uint idx)
	{ return (P_List && idx < P_List->getCount()) ? P_List->atFree(idx) : 0; }
ObjIdListFilt & SLAPI ObjIdListFilt::Z() { CALLPTRMEMB(P_List, Z()); return *this; } // @v10.6.8 FreeAll-->Z freeAll()-->Z()
int SLAPI ObjIdListFilt::Sort() { return P_List ? (P_List->sort(), 1) : -1; }
uint SLAPI ObjIdListFilt::GetCount() const { return SVectorBase::GetCount(P_List); }
int SLAPI ObjIdListFilt::Search(PPID id, uint * pPos, int binary) const
	{ return P_List ? (binary ? P_List->bsearch(id, pPos) : P_List->lsearch(id, pPos)) : 0; }

int SLAPI ObjIdListFilt::Intersect(const ObjIdListFilt * pList, int binary /*=0*/)
{
	if(P_List) {
		if(!pList)
			Z();
		else {
			PPIDArray list;
			pList->CopyTo(&list);
			P_List->intersect(&list, binary);
		}
	}
	return 1;
}

int FASTCALL ObjIdListFilt::Write(SBuffer & rBuf) const
{
	int    ok = 1;
	long   is_exists = IsExists();
	THROW_SL(rBuf.Write(is_exists));
	if(is_exists)
		THROW_SL(rBuf.Write(P_List, 0));
	CATCHZOK
	return ok;
}

int FASTCALL ObjIdListFilt::Read(SBuffer & rBuf)
{
	int    ok = 1;
	long   is_exists = 0;
	THROW_SL(rBuf.Read(is_exists));
	if(is_exists) {
		THROW(InitEmpty());
		THROW_SL(rBuf.Read(P_List, 0));
	}
	else
		Set(0);
	CATCHZOK
	return ok;
}

int SLAPI ObjIdListFilt::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	uint8  ind = 0;
	if(dir > 0) {
		uint c = GetCount();
		if(!IsExists())
			ind = 1;
		else if(c == 0)
			ind = 2;
		else if(c == 1)
			ind = 3;
		else
			ind = 0;
		THROW_SL(rBuf.Write(&ind, sizeof(ind)));
		if(ind == 3) {
			long   single = P_List->get(0);
			THROW_SL(pCtx->Serialize(dir, single, rBuf));
		}
		else if(ind == 0) {
			THROW_SL(pCtx->Serialize(dir, P_List, rBuf));
		}
	}
	else if(dir < 0) {
		THROW_SL(rBuf.Read(&ind, sizeof(ind)));
		if(ind == 1) {
			Set(0);
		}
		else if(ind == 2) {
			Set(0);
			InitEmpty();
		}
		else if(ind == 3) {
			long   single = 0;
			THROW_SL(pCtx->Serialize(dir, single, rBuf));
			SetSingle(single);
		}
		else {
			InitEmpty();
			THROW_SL(pCtx->Serialize(dir, P_List, rBuf));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI PPObjID::IsZero() const
{
	return (Obj == 0 && Id == 0);
}

PPObjID & PPObjID::Z()
{
	Obj = 0;
	Id = 0;
	return *this;
}

PPObjID PPObjID::Set(PPID objType, PPID objID)
{
	Obj = objType;
	Id = objID;
	return *this;
}

int SLAPI PPObjID::IsEqual(PPID objType, PPID objID) const { return (Obj == objType && Id == objID); }
int FASTCALL PPObjID::operator == (PPObjID s) const { return (Obj == s.Obj && Id == s.Id); }
int FASTCALL PPObjID::operator != (PPObjID s) const { return (Obj != s.Obj || Id != s.Id); }
PPObjID::operator double() const { return ((Obj * 4.0E+9) + (double)Id); }

PPObjID & FASTCALL PPObjID::operator = (double oid)
{
	Id = (long)fmod(fabs(oid), 4.0E+9);
	Obj = (long)(fabs(oid) / 4.0E+9);
	return *this;
}

SString & FASTCALL PPObjID::ToStr(SString & rBuf) const
{
	rBuf.Z();
	if(!DS.GetObjectTypeSymb(Obj, rBuf)) 
		rBuf.Cat("Obj").CatChar('[').Cat(Obj).CatChar(']');
	if(Id)
		rBuf.CatChar('(').Cat(Id).CatChar(')');
	return rBuf;
}

int FASTCALL PPObjID::FromStr(const char * pStr)
{
	int    ok = 0;
	SStrScan scan(pStr);
	SString temp_buf;
	scan.Skip();
	if(scan.GetIdent(temp_buf)) {
		long   extra = 0;
		PPID   lval = DS.GetObjectTypeBySymb(temp_buf, &extra);
		if(lval) {
			Obj = lval;
			ok = 1;
			scan.Skip();
			if(scan[0] == '(') {
				scan.Incr();
				if(scan.GetNumber(temp_buf)) {
					lval = temp_buf.ToLong();
					scan.Skip();
					if(scan[0] == ')') {
						Id = lval;
						ok = 2;
					}
				}
			}
		}
	}
	return ok;
}
//
//
//
SLAPI PPObjIDArray::PPObjIDArray() : TSVector <PPObjID>() // @v9.8.4 TSArray-->TSVector
{
	DoReplace = 0;
}

int SLAPI PPObjIDArray::Search(PPObjID key, uint * pPos) const
{
    return lsearch(&key, pPos, PTR_CMPFUNC(_2long));
}

int SLAPI PPObjIDArray::Add(PPID objType, PPID objID)
{
	PPObjID item;
	item.Set(objType, objID);
	return insert(&item) ? 1 : PPSetErrorSLib();
}

int SLAPI PPObjIDArray::Add_NotPreprocess(PPID objType, PPID objID)
{
	int    ok = 1;
	PPObjID item;
	item.Set(objType, objID);
	const uint pos = getCount();
	if(insert(&item)) {
		DontPreprocPosList.add(pos);
	}
	else
		ok = PPSetErrorSLib();
	return ok;
}

int FASTCALL PPObjIDArray::Is_NotPreprocess_Pos(uint pos) const
{
	return DontPreprocPosList.lsearch(static_cast<long>(pos)) ? 1 : 0;
}

int SLAPI PPObjIDArray::Add(PPID objType, const PPIDArray & rList)
{
	int    ok = 1;
	PPObjID item;
	item.Obj = objType;
	for(uint i = 0; ok && i < rList.getCount(); i++) {
		item.Id = rList.get(i);
		if(!insert(&item))
			ok = PPSetErrorSLib();
	}
	return ok;
}

int SLAPI PPObjIDArray::ProcessRef(PPID objType, PPID * pObjID)
{
	int    ok = -1;
	if(objType)
		if(!DoReplace)
			ok = Add(objType, *pObjID);
		else {
			PPObjID * p_o;
			for(uint p = 0; ok < 0 && enumItems(&p, (void **)&p_o);)
				if(p_o->Obj) {
					p_o->Obj = 0;
					*pObjID = p_o->Id;
					ok = 1;
				}
		}
	return ok;
}
//
//
//
struct SurKey {
	long   Id;
	SBaseBuffer Val;
};

SurKeyArray::SurKeyArray() : SArray(sizeof(SurKey), aryDataOwner|aryEachItem), LastId(0)
{
}

/*virtual*/void FASTCALL SurKeyArray::freeItem(void * pItem)
{
	SurKey * p_item = static_cast<SurKey *>(pItem);
	p_item->Val.Destroy();
}

int SurKeyArray::Add(long * pID, const void * pData, size_t dataLen)
{
	int    ok = 1;
	SurKey item;
	item.Id = ++LastId;
	item.Val.Init();
	THROW_SL(item.Val.Alloc(dataLen));
	memcpy(item.Val.P_Buf, pData, dataLen);
	THROW_SL(insert(&item));
	CATCHZOK
	ASSIGN_PTR(pID, item.Id);
	return ok;
}

int SurKeyArray::Remove(long id)
{
	uint pos = 0;
	if(lsearch(&id, &pos, PTR_CMPFUNC(long))) {
		atFree(pos);
		return 1;
	}
	else
		return 0;
}

const void * SurKeyArray::Get(long id, size_t * pDataLen) const
{
	uint pos = 0;
	if(lsearch(&id, &pos, PTR_CMPFUNC(long))) {
		const SurKey * p_item = static_cast<const SurKey *>(at(pos));
		ASSIGN_PTR(pDataLen, p_item->Val.Size);
		return p_item->Val.P_Buf;
	}
	else {
		ASSIGN_PTR(pDataLen, 0);
		return 0;
	}
}
//
//
//
SLAPI PPStringSetSCD::PPStringSetSCD() : StringSet("/&")
{
}
