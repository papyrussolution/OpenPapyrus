// OBJASSOC.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2015, 2016
// @Kernel
//
#include <pp.h>
#pragma hdrstop

SLAPI ObjAssoc::ObjAssoc() : ObjAssocTbl()
{
}

#define MAX_DEADLOCK_TRY 10
#define DEADLOCK_TIMEOUT 1000

int SLAPI ObjAssoc::Add(PPID * pID, ObjAssocTbl::Rec * b, int use_ta)
{
	int    deadlock_try = 0;
	do {
		int    r = AddByID(this, pID, b, use_ta);
		if(!r && PPErrCode == PPERR_DBENGINE && BtrError == BE_DEADLOCK) {
			delay(DEADLOCK_TIMEOUT);
		}
		else
			return r;
	} while((++deadlock_try) < MAX_DEADLOCK_TRY);
	return 0;
}

int SLAPI ObjAssoc::Update(PPID id, ObjAssocTbl::Rec * b, int use_ta)
{
	int    deadlock_try = 0;
	do {
		int    r = UpdateByID(this, PPOBJ_OBJASSOC, id, b, use_ta);
		if(!r && PPErrCode == PPERR_DBENGINE && BtrError == BE_DEADLOCK) {
			delay(DEADLOCK_TIMEOUT);
		}
		else
			return r;
	} while((++deadlock_try) < MAX_DEADLOCK_TRY);
	return 0;
}

int SLAPI ObjAssoc::Remove(PPID id, int use_ta)
{
	int    deadlock_try = 0;
	do {
		int    r = RemoveByID(this, id, use_ta);
		if(!r && PPErrCode == PPERR_DBENGINE && BtrError == BE_DEADLOCK) {
			delay(DEADLOCK_TIMEOUT);
		}
		else
			return r;
	} while((++deadlock_try) < MAX_DEADLOCK_TRY);
	return 0;
}

int SLAPI ObjAssoc::Search(PPID id, ObjAssocTbl::Rec * pRec)
{
	return SearchByID(this, PPOBJ_OBJASSOC, id, pRec);
}

int SLAPI ObjAssoc::SearchNum(PPID asscTyp, PPID prmr, long num, ObjAssocTbl::Rec * pRec)
{
	return _SearchNum(asscTyp, prmr, num, num ? spEq : spGe, pRec);
}

int SLAPI ObjAssoc::_SearchNum(PPID asscTyp, PPID prmr, long num, int spMode, ObjAssocTbl::Rec * b)
{
	ObjAssocTbl::Key3 k;
	k.AsscType  = asscTyp;
	k.PrmrObjID = prmr;
	k.InnerNum  = num;
	if(search(3, &k, spMode) && k.AsscType == asscTyp && k.PrmrObjID == prmr)
		return (copyBufTo(b), 1);
	return PPDbSearchError();
}

int SLAPI ObjAssoc::SearchFreeNum(PPID asscTyp, PPID prmr, long * pNum, ObjAssocTbl::Rec * b)
{
	int    r;
	if(*pNum)
		r = _SearchNum(asscTyp, prmr, *pNum, spEq, b);
	else {
		BExtQuery q(this, 3, 128);
		ObjAssocTbl::Key3 k;
		k.AsscType  = asscTyp;
		k.PrmrObjID = prmr;
		k.InnerNum  = 1;
		long   prev_num = 0, num = 0;
		q.select(this->InnerNum, 0L).where(this->AsscType == asscTyp && this->PrmrObjID == prmr);
		for(q.initIteration(0, &k, spGe); !num && q.nextIteration() > 0;) {
			if(data.InnerNum > prev_num+1)
				num = prev_num+1;
			prev_num = data.InnerNum;
		}
		SETIFZ(num, prev_num+1);
		ASSIGN_PTR(pNum, num);
		r = 1;
	}
	return r;
}

int SLAPI ObjAssoc::GetByPrmr_Helper(PPID asscTyp, PPID prmrID, PPIDArray * pList, long * pCount, SArray * pItemsList)
{
	int    ok = -1;
	long   count = 0;
	ObjAssocTbl::Key1 k;
	k.AsscType  = asscTyp;
	k.PrmrObjID = prmrID;
	k.ScndObjID = -MAXLONG;
	BExtQuery * p_q = 0;
	if(pItemsList) {
		pItemsList->freeAll();
		p_q = new BExtQuery(this, 1);
		p_q->selectAll();
	}
	else {
		p_q = new BExtQuery(this, 1, 128);
		p_q->select(this->ScndObjID, 0L);
	}
	p_q->where(this->AsscType == asscTyp && this->PrmrObjID == prmrID);
	for(p_q->initIteration(0, &k, spGe); p_q->nextIteration() > 0;) {
		count++;
		if(pList)
			THROW(pList->add(data.ScndObjID));
		if(pItemsList)
			THROW_SL(pItemsList->insert(&data));
		ok = 1;
	}
	ASSIGN_PTR(pCount, count);
	CATCHZOK
	delete p_q;
	return ok;
}

int SLAPI ObjAssoc::GetListByPrmr(PPID asscTyp, PPID prmrID, PPIDArray * pList)
{
	return GetByPrmr_Helper(asscTyp, prmrID, pList, 0, 0);
}

int SLAPI ObjAssoc::Count(PPID asscTyp, PPID prmrID, long * pCount)
{
	return GetByPrmr_Helper(asscTyp, prmrID, 0, pCount, 0);
}

int SLAPI ObjAssoc::GetItemsListByPrmr(PPID asscTyp, PPID prmrID, SArray * pList)
{
	return (pList && pList->getItemSize() == sizeof(ObjAssocTbl::Rec)) ?
		GetByPrmr_Helper(asscTyp, prmrID, 0, 0, pList) : (PPErrCode = PPERR_INVPARAM, 0);
}

int SLAPI ObjAssoc::EnumByPrmr(PPID asscTyp, PPID prmrID, PPID * pScndID, ObjAssocTbl::Rec * b)
{
	ObjAssocTbl::Key1 k;
	k.AsscType  = asscTyp;
	k.PrmrObjID = prmrID;
	k.ScndObjID = *pScndID;
	if(search(1, &k, spGt) && k.AsscType == asscTyp && k.PrmrObjID == prmrID)
		return ((*pScndID = k.ScndObjID), copyBufTo(b), 1);
	return PPDbSearchError();
}

SEnumImp * SLAPI ObjAssoc::Enum(PPID asscType, PPID keyID, int byScnd)
{
	union {
		ObjAssocTbl::Key1 k1;
		ObjAssocTbl::Key2 k2;
	} k;
	MEMSZERO(k);
	long   h = -1;
	BExtQuery * q = new BExtQuery(this, byScnd ? 2 : 1);
	q->selectAll();
	if(byScnd) {
		k.k2.AsscType = asscType;
		k.k2.ScndObjID = keyID;
		k.k2.PrmrObjID = -MAXLONG;
		q->where(this->AsscType == asscType && this->ScndObjID == keyID);
	}
	else {
		k.k1.AsscType = asscType;
		k.k1.PrmrObjID = keyID;
		k.k1.ScndObjID = -MAXLONG;
		q->where(this->AsscType == asscType && this->PrmrObjID == keyID);
	}
	q->initIteration(0, &k, spGe);
	return EnumList.RegisterIterHandler(q, &h) ? (new PPTblEnum <ObjAssoc> (this, h)) : 0;
}

int SLAPI ObjAssoc::AddArray(PPID asscTyp, PPID prmrID, const SArray * pList, int preRemove, int use_ta)
{
	int    ok = 1;
	THROW_PP(asscTyp && prmrID, PPERR_INVPARAM);
	THROW_PP(!pList || pList->getItemSize() == sizeof(ObjAssocTbl::Rec), PPERR_INVPARAM);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(preRemove) {
			THROW(Remove(asscTyp, prmrID, 0, 0));
		}
		if(pList && pList->getCount()) {
			BExtInsert bei(this);
			for(uint i = 0; i < pList->getCount(); i++) {
				ObjAssocTbl::Rec rec;
				memcpy(&rec, pList->at(i), sizeof(rec));
				rec.AsscType  = asscTyp;
				rec.PrmrObjID = prmrID;
				SETIFZ(rec.InnerNum, i+1);
				THROW_DB(bei.insert(&rec));
			}
			THROW_DB(bei.flash());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI ObjAssoc::AddArray(PPID asscTyp, const LAssocArray * pList, int preRemove, int use_ta)
{
	int    ok = 1;
	THROW_PP(asscTyp, PPERR_INVPARAM);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(preRemove) {
			THROW(Remove(asscTyp, 0, 0, 0));
		}
		if(pList && pList->getCount()) {
			BExtInsert bei(this);
			for(uint i = 0; i < pList->getCount(); i++) {
				ObjAssocTbl::Rec rec;
				MEMSZERO(rec);
				rec.AsscType = asscTyp;
				rec.PrmrObjID = pList->at(i).Key;
				rec.ScndObjID = pList->at(i).Val;
				rec.InnerNum = 1;
				THROW_DB(bei.insert(&rec));
			}
			THROW_DB(bei.flash());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI ObjAssoc::GetList(PPID asscTyp, LAssocArray * pList)
{
	int    ok = -1;
	ObjAssocTbl::Key1 k;
	k.AsscType  = asscTyp;
	k.PrmrObjID = 0;
	k.ScndObjID = 0;
	BExtQuery q(this, 1, 128);
	q.select(this->PrmrObjID, this->ScndObjID, 0L).where(this->AsscType == asscTyp);
	for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
		ok = 1;
		if(pList) {
			THROW_SL(pList->Add(data.PrmrObjID, data.ScndObjID, 0, 0));
		}
		else if(ok > 0)
			break;
	}
	CATCHZOK
	return ok;
}

int SLAPI ObjAssoc::Remove(PPID asscTyp, PPID prmr, PPID scnd, int use_ta)
{
	//
	// Возможны следующие случаи:
	// 1. (asscTyp != 0, prmr != 0, scnd != 0) Удаление одной записи с заданными координатами
	// 2. (asscTyp != 0, prmr != 0, scnd == 0) Удаление всех ассоциаций типа asscTyp, в которых
	//    объект prmr участвует как первичный
	// 3. (asscTyp != 0, prmr == 0, scnd != 0) Удаление всех ассоциаций типа asscTyp, в которых
	//    объект scnd участвует как вторичный
	// 4. (asscTyp != 0, prmr == 0, scnd == 0) Удаление всех ассоциаций типа asscTyp
	// 5. (asscTyp == 0) Удаление всех записей из таблицы
	//
	DBQ * dbq = 0;
	if(asscTyp) {
		dbq = &(this->AsscType == asscTyp);
		if(prmr)
			if(scnd)
				dbq = &(*dbq &&this->PrmrObjID == prmr && this->ScndObjID == scnd);
			else
				dbq = &(*dbq && this->PrmrObjID == prmr);
		else if(scnd)
			dbq = &(*dbq && this->ScndObjID == scnd);
	}
	else // Удаление всех записей из таблицы (dbq = 0)
		;
	return deleteFrom(this, use_ta, *dbq) ? 1 : 0;
}

int SLAPI ObjAssoc::Search(PPID asscTyp, PPID prmr, PPID scnd, ObjAssocTbl::Rec * b)
{
	ObjAssocTbl::Key1 k;
	k.AsscType  = asscTyp;
	k.PrmrObjID = prmr;
	k.ScndObjID = scnd;
	return SearchByKey(this, 1, &k, b);
}

