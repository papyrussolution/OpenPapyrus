// ACCOUNT.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2015, 2016, 2017, 2018, 2019, 2020, 2022, 2024
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
//
//
int FASTCALL GetAcctName(const Acct * acct, PPID curID, long fmt, SString & rBuf)
{
	AcctID acctid;
	rBuf.Z();
	return (BillObj->atobj->ConvertAcct(acct, curID, &acctid, 0) > 0) ? GetAcctIDName(acctid, fmt, rBuf) : -1;
}

int FASTCALL GetAcctIDName(const AcctID & rAci, long, SString & rBuf)
{
	int    ok = -1;
	rBuf.Z();
	if(rAci.ar) {
		PPObjArticle ar_obj;
		ArticleTbl::Rec ar_rec;
		if(ar_obj.Fetch(rAci.ar, &ar_rec) > 0) {
			rBuf = ar_rec.Name;
			ok = 1;
		}
	}
	else {
		PPObjAccount acc_obj;
		PPAccount acc_rec;
		if(acc_obj.Fetch(rAci.ac, &acc_rec) > 0) {
			rBuf = acc_rec.Name;
			ok = 1;
		}
	}
	return ok;
}

int AccIDToAcct(PPID id, int ord, Acct * pAcct)
{
	int    ok = 1;
	PPObjAccTurn atobj(0);
	if(ord == ACO_3) {
		THROW(atobj.P_Tbl->AccRel.Search(id) > 0);
		*pAcct = atobj.P_Tbl->AccRel.data;
	}
	else {
		PPAccount acc_rec;
		THROW(atobj.P_Tbl->AccObj.Search(id, &acc_rec) > 0);
		*pAcct = acc_rec;
	}
	CATCH
		memzero(pAcct, sizeof(*pAcct));
		ok = 0;
	ENDCATCH
	return ok;
}

PPAccount::PPAccount()
{
	THISZERO();
}
//
// PPAccountPacket
//
PPAccountPacket::PPAccountPacket()
{
}

PPAccountPacket & PPAccountPacket::Z()
{
	MEMSZERO(Rec);
	CurList.Z();
	GenList.clear();
	return *this;
}
//
// ArticleCore
//
ArticleCore::ArticleCore() : ArticleTbl()
{
}

int ArticleCore::Search(PPID id, void * b) { return SearchByID(this, PPOBJ_ARTICLE, id, b); }

int ArticleCore::SearchName(PPID accSheetID, const char * pName, void * b)
{
	ArticleTbl::Key2 k2;
	MEMSZERO(k2);
	k2.AccSheetID = accSheetID;
	STRNSCPY(k2.Name, pName);
	return SearchByKey(this, 2, &k2, b);
}

int ArticleCore::EnumBySheet(PPID accSheetID, long * pArticleNo, void * b)
{
	ArticleTbl::Key1 k;
	k.AccSheetID = accSheetID;
	k.Article = *pArticleNo;
	if(search(1, &k, spGt) && k.AccSheetID == accSheetID) {
		*pArticleNo = data.Article;
		copyBufTo(b);
		return 1;
	}
	return PPDbSearchError();
}

int ArticleCore::Count(PPID accSheetID, long * pCount)
{
	ArticleTbl::Key1 k;
	k.AccSheetID = accSheetID;
	k.Article = 0;
	BExtQuery q(this, 1, 128);
	q.select(this->ID, 0L).where(this->AccSheetID == accSheetID);
	long   c = q.countIterations(0, &k, spGe);
	ASSIGN_PTR(pCount, c);
	return 1;
}

int ArticleCore::GetListBySheet(PPID accSheetID, PPIDArray * pList, long * pCount)
{
	int    ok = -1;
	long   count = 0;
	CALLPTRMEMB(pList, clear());
	if(accSheetID) {
		ArticleTbl::Key1 k;
		k.AccSheetID = accSheetID;
		k.Article = 0;
		BExtQuery q(this, 1, 128);
		q.select(this->ID, 0L).where(this->AccSheetID == accSheetID);
		for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
			count++;
			CALLPTRMEMB(pList, add(data.ID));
		}
		if(count) {
			CALLPTRMEMB(pList, sortAndUndup());
			ok = 1;
		}
	}
	ASSIGN_PTR(pCount, count);
	return ok;
}

int ArticleCore::GetListByGroup(PPID grpArID, PPIDArray * pList)
{
	int    ok = 1;
	if(pList) {
		PPIDArray temp_list;
		if(!PPRef->Assc.GetListByPrmr(PPASS_GROUPARTICLE, grpArID, &temp_list) || !pList->addUnique(&temp_list))
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

int ArticleCore::_SearchNum(PPID accSheetID, long articleNo, int spMode, ArticleTbl::Rec * pRec)
{
	ArticleTbl::Key1 k;
	k.AccSheetID = accSheetID;
	k.Article = articleNo;
	if(search(1, &k, spMode) && k.AccSheetID == accSheetID)
		return (copyBufTo(pRec), 1);
	else
		return (BTROKORNFOUND) ? (PPErrCode = PPERR_ARTICLENFOUND, -1) : PPSetErrorDB();
}

int ArticleCore::SearchFreeNum(PPID accSheetID, long * pAr, ArticleTbl::Rec * pRec)
{
	int    r;
	if(*pAr)
		r = _SearchNum(accSheetID, *pAr, spEq, pRec);
	else {
		r = _SearchNum(accSheetID, MAXLONG, spLt, pRec);
		if(r > 0)
			*pAr = data.Article+1;
		else if(r < 0)
			*pAr = 1;
	}
	return r;
}

int ArticleCore::SearchObjRef(PPID sheet, PPID id, ArticleTbl::Rec * b)
{
	ArticleTbl::Key3 k;
	k.AccSheetID = sheet;
	k.ObjID = id;
	return SearchByKey(this, 3, &k, b);
}

bool ArticleCore::PersonToArticle(PPID personID, PPID accSheetID, PPID * pArID)
{
	bool   ok = false;
	PPID   ar_id = 0;
	ArticleTbl::Rec rec;
	if(personID && accSheetID && SearchObjRef(accSheetID, personID, &rec) > 0) {
		ar_id = rec.ID;
		ok = true;
	}
	ASSIGN_PTR(pArID, ar_id);
	return ok;
}

int ArticleCore::LocationToArticle(PPID locID, PPID accSheetID, PPID * pArID)
{
	int    ok = -1;
	PPID   ar_id = 0;
	ArticleTbl::Rec rec;
	if(accSheetID && SearchObjRef(accSheetID, locID, &rec) > 0) {
		ar_id = rec.ID;
		ok = 1;
	}
	ASSIGN_PTR(pArID, ar_id);
	return ok;
}

int ArticleCore::SearchNum(PPID sheet, long ar, ArticleTbl::Rec * pRec) { return _SearchNum(sheet, ar, ar ? spEq : spGe, pRec); }
int ArticleCore::Add(PPID * pID, void * b, int use_ta) { return AddObjRecByID(this, PPOBJ_ARTICLE, pID, b, use_ta); }
int ArticleCore::Update(PPID id, const void * pRec, int use_ta) { return UpdateByID(this, PPOBJ_ARTICLE, id, pRec, use_ta); }
int ArticleCore::Remove(PPID id, int use_ta) { return RemoveByID(this, id, use_ta); }
//
// AcctRel
//
AcctRel::AcctRel() : AcctRelTbl()
{
}

int AcctRel::Search(PPID id, AcctRelTbl::Rec * pRec) { return SearchByID(this, PPOBJ_ACCTREL, id, pRec); }

int AcctRel::SearchAcctID(const AcctID * pAcctId, AcctRelTbl::Rec * pRec)
{
	AcctRelTbl::Key1 k1;
	k1.AccID     = pAcctId->ac;
	k1.ArticleID = pAcctId->ar;
	return SearchByKey(this, 1, &k1, pRec);
}

int AcctRel::SearchNum(int closed, const Acct * pAcct, PPID curID, AcctRelTbl::Rec * pRec)
{
	int    sp;
	AcctRelTbl::Key3 k3;
	k3.Ac = pAcct->ac;
	k3.Sb = pAcct->sb;
	k3.Ar = pAcct->ar;
	k3.CurID = curID;
	if(closed >= 0) {
		k3.Closed = closed;
		sp = spEq;
	}
	else {
		k3.Closed = MAXSHORT; // @v8.9.8 MAXINT-->MAXSHORT
		sp = spLe;
	}
	if(search(3, &k3, sp) && (closed >= 0 || (k3.Ac == pAcct->ac && k3.Sb == pAcct->sb && k3.Ar == pAcct->ar && k3.CurID == curID)))
		return (copyBufTo(pRec), 1);
	else
		return PPDbSearchError();
}

int AcctRel::OpenAcct(PPID * pID, const Acct * pAcct, PPID curID, const AcctID * pAcctId, int accKind, int accsLevel)
{
	clearDataBuf();
	data.AccID    = pAcctId->ac;
	data.ArticleID = pAcctId->ar;
	data.Ac       = pAcct->ac;
	data.Sb       = pAcct->sb;
	data.Ar       = pAcct->ar;
	data.CurID    = curID;
	data.Kind     = accKind;
	data.AccessLevel = accsLevel;
	return insertRec(0, pID) ? 1 : PPSetErrorDB();
}

int AcctRel::CloseAcct(PPID id, int use_ta)
{
	int    ok = 1;
	int    r, closed;
	Acct   acct;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(id) > 0);
		THROW_PP(data.Closed == 0, PPERR_ACCTCLOSED);
		acct = *reinterpret_cast<const Acct *>(&data.Ac);
		THROW(r = SearchNum(-1, &acct, data.CurID));
		closed = (r > 0) ? (data.Closed+1) : 1;
		THROW(Search(id) > 0);
		data.Closed = closed;
		THROW_DB(updateRec());
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int AcctRel::ReplaceAcct(int oldAc, int oldSb, int newAc, int newSb)
{
	int    ok = 1;
	AcctRelTbl::Key3 k;
	MEMSZERO(k);
	k.Ac = oldAc;
	k.Sb = oldSb;
	if(searchForUpdate(3, &k, spGe) && k.Ac == oldAc && k.Sb == oldSb) do {
		data.Ac = newAc;
		data.Sb = newSb;
		THROW_DB(updateRec()); // @sfu-r
	} while(searchForUpdate(3, &k, spNext) && k.Ac == oldAc && k.Sb == oldSb);
	THROW_DB(BTROKORNFOUND);
	CATCHZOK
	return ok;
}

int AcctRel::EnumByAcc(PPID accID, PPID * pArID, AcctRelTbl::Rec * pRec)
{
	AcctRelTbl::Key1 k;
	PPID   ar_id = (*pArID < 0) ? 0 : *pArID;
	k.AccID = accID;
	k.ArticleID = ar_id;
	int    sp = ar_id ? spGt : spGe;
	if(search(1, &k, sp) && k.AccID == accID) {
		copyBufTo(pRec);
		*pArID = data.ArticleID;
		return 1;
	}
	else
		return PPDbSearchError();
}

int AcctRel::EnumByArticle(PPID arID, PPID * pAccID, AcctRelTbl::Rec * pRec)
{
	AcctRelTbl::Key2 k;
	PPID   acc_id = (*pAccID < 0) ? 0 : *pAccID;
	k.ArticleID = arID;
	k.AccID = acc_id;
	int    sp = acc_id ? spGt : spGe;
	if(search(2, &k, sp) && k.ArticleID == arID) {
		copyBufTo(pRec);
		*pAccID = data.AccID;
		return 1;
	}
	else
		return PPDbSearchError();
}

SEnum::Imp * AcctRel::Enum(int keyN, PPID keyID)
{
	union {
		AcctRelTbl::Key1 k1;
		AcctRelTbl::Key2 k2;
	} k;
	MEMSZERO(k);
	SEnum::Imp * p_enum = 0;
	long   h = -1;
	BExtQuery * q = 0;
	THROW_INVARG(oneof2(keyN, 1, 2));
	THROW_MEM(q = new BExtQuery(this, keyN));
	q->selectAll();
	if(keyN == 1) {
		k.k1.AccID = keyID;
		q->where(this->AccID == keyID);
	}
	else if(keyN == 2) {
		k.k2.ArticleID = keyID;
		q->where(this->ArticleID == keyID);
	}
	q->initIteration(false, &k, spGe);
	THROW(EnumList.RegisterIterHandler(q, &h));
	THROW_MEM(p_enum = new PPTblEnum <AcctRel>(this, h));
	CATCH
		EnumList.DestroyIterHandler(h);
	ENDCATCH
	return p_enum;
}

SEnum::Imp * AcctRel::EnumByAcc(PPID accID) { return Enum(1, accID); }
SEnum::Imp * AcctRel::EnumByArticle(PPID arID) { return Enum(2, arID); }
//
//
//
class AcctRelCache : public ObjCache {
public:
	AcctRelCache() : ObjCache(PPOBJ_ACCTREL, sizeof(Data)) {}
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		PPID   AccID;
		PPID   ArticleID;
		PPID   CurID;
		int16  Ac;
		int16  Sb;
		long   Ar;
		int16  Kind;
		int16  Closed;
		long   Flags;
	};
};

int AcctRelCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	AcctRel * p_tbl = &BillObj->atobj->P_Tbl->AccRel;
	AcctRelTbl::Rec rec;
	if(p_tbl->Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
		CPY_FLD(AccID);
		CPY_FLD(ArticleID);
		CPY_FLD(CurID);
		CPY_FLD(Ac);
		CPY_FLD(Sb);
		CPY_FLD(Ar);
		CPY_FLD(Kind);
		CPY_FLD(Closed);
		CPY_FLD(Flags);
#undef CPY_FLD
	}
	else
		ok = -1;
	return ok;
}

void AcctRelCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	AcctRelTbl::Rec * p_data_pack = static_cast<AcctRelTbl::Rec *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
#define CPY_FLD(Fld) p_data_pack->Fld=p_cache_rec->Fld
	CPY_FLD(ID);
	CPY_FLD(AccID);
	CPY_FLD(ArticleID);
	CPY_FLD(CurID);
	CPY_FLD(Ac);
	CPY_FLD(Sb);
	CPY_FLD(Ar);
	CPY_FLD(Kind);
	CPY_FLD(Closed);
	CPY_FLD(Flags);
#undef CPY_FLD
}

int AcctRel::Fetch(PPID id, AcctRelTbl::Rec * pRec)
{
	AcctRelCache * p_cache = GetDbLocalCachePtr <AcctRelCache> (PPOBJ_ACCTREL);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}
