// ACCOUNT.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2015, 2016, 2017, 2018, 2019
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

int SLAPI AccIDToAcct(PPID id, int ord, Acct * pAcct)
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
//
// PPAccountPacket
//
SLAPI PPAccountPacket::PPAccountPacket()
{
	Init();
}

void SLAPI PPAccountPacket::Init()
{
	MEMSZERO(Rec);
	CurList.freeAll();
	GenList.freeAll();
}

PPAccountPacket & FASTCALL PPAccountPacket::operator = (const PPAccountPacket & src)
{
	Rec = src.Rec;
	CurList.copy(src.CurList);
	GenList.copy(src.GenList);
	return *this;
}
//
// Account
//

#if 0 // @v9.0.4 {

// static
int SLAPI AccountCore::GenerateCode(PPAccount * pRec)
{
	char   buf[48];
	Acct   acct;
	acct.ac = pRec->Ac;
	acct.sb = pRec->Sb;
	acct.ar = 0;
	AccToStr(&acct, ACCF_DEFAULT, buf);
	STRNSCPY(pRec->Code, buf);
	return 1;
}

SLAPI AccountCore::AccountCore() : AccountTbl()
{
}

int SLAPI AccountCore::GenerateNumber(PPAccount * pRec)
{
	int    ok = -1, r;
	int    start = 0, finish = 0;
	if(!pRec)
		return PPSetErrorInvParam();
	pRec->Ac = 0;
	pRec->Sb = 0;
	if(pRec->Type == ACY_OBAL) {
		start = 1000;
		finish = 1999;
	}
	else if(pRec->Type == ACY_REGISTER) {
		start = 2000;
		finish = 9999;
	}
	else if(pRec->Type == ACY_AGGR) {
		start = 10000;
		finish = 11999;
	}
	else if(pRec->Type == ACY_ALIAS) {
		start = 12000;
		finish = 13999;
	}
	else if(pRec->Type == ACY_BUDGET) {
		start  = 14000;
		finish = 21999;
	}
	if(start > 0)
		for(int ac = start; ok < 0 && ac <= finish; ac++)
			if((r = SearchNum(ac, 0, 0L)) < 0) {
				pRec->Ac = ac;
				pRec->Sb = 0;
				ok = 1;
			}
			else if(r == 0)
				ok = 0;
	if(ok < 0)
		ok = PPSetError(PPERR_CANTGENACCNUMBER);
	return ok;
}

int SLAPI AccountCore::GetCurList(int ac, int sb, PPIDArray * pAccList, PPIDArray * pCurList)
{
	AccountTbl::Key1 k1;
	k1.Ac = ac;
	k1.Sb = sb;
	k1.CurID = 0;
	while(search(1, &k1, spGt) && k1.Ac == ac && k1.Sb == sb)
		if(data.CurID) {
			if(pCurList && !pCurList->addUnique(data.CurID))
				return 0;
			if(pAccList && !pAccList->addUnique(data.ID))
				return 0;
		}
	return (BTROKORNFOUND) ? 1 : PPSetErrorDB();
}

int SLAPI AccountCore::GetCurList(PPID accID, PPIDArray * pAccList, PPIDArray * pCurList)
{
	if(Search(accID) > 0) {
		if(data.Flags & ACF_CURRENCY)
			return GetCurList(data.Ac, data.Sb, pAccList, pCurList);
		else {
			CALLPTRMEMB(pCurList, addUnique(0L));
			CALLPTRMEMB(pAccList, addUnique(accID));
			return 1;
		}
	}
	return -1;
}

int SLAPI AccountCore::GetIntersectCurList(PPID accID_1, PPID accID_2, PPIDArray * pCurList)
{
	PPIDArray cur_list, cur_list1;
	GetCurList(accID_1, 0, &cur_list);
	GetCurList(accID_2, 0, &cur_list1);
	if(accID_2)
		if(accID_1)
			cur_list.intersect(&cur_list1);
		else
			cur_list.copy(cur_list1);
	if(pCurList)
		pCurList->copy(cur_list);
	return 1;
}

int SLAPI AccountCore::GetPacket(PPID id, PPAccountPacket * pAccPack)
{
	int    ok = 1;
	MEMSZERO(pAccPack->Rec);
	pAccPack->CurList.freeAll();
	pAccPack->GenList.freeAll();
	if(Search(id, &pAccPack->Rec) > 0) {
		THROW_PP(pAccPack->Rec.CurID == 0, PPERR_RACCSCURACC);
		if(pAccPack->Rec.CurID == 0 && pAccPack->Rec.Flags & ACF_CURRENCY)
			THROW(GetCurList(pAccPack->Rec.Ac, pAccPack->Rec.Sb, 0, &pAccPack->CurList));
		if(pAccPack->Rec.Type == ACY_AGGR)
			THROW(PPRef->GetPropArray(PPOBJ_ACCOUNT, id, ACCPRP_GENACCLIST, &pAccPack->GenList));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI AccountCore::AddCurRecord(PPAccount * pBaseRec, PPID curID)
{
	PPAccount cur_acc_rec = *pBaseRec;
	cur_acc_rec.ID = 0;
	cur_acc_rec.CurID = curID;
	cur_acc_rec.Limit = cur_acc_rec.Overdraft = 0;
	return insertRecBuf(&cur_acc_rec) ? 1 : PPSetErrorDB();
}

int SLAPI AccountCore::GetAggrNumber(PPAccount * pRec)
{
	for(int i = MINGENACCNUMBER; i <= MAXGENACCNUMBER; i++) {
		if(SearchNum(i, 0, 0L) < 0) {
			pRec->Ac = i;
			return 1;
		}
	}
	return 0;
}

int SLAPI AccountCore::PutPacket(PPID * pID, PPAccountPacket * pAccPack, int use_ta)
{
	int    ok = 1;
	uint   i;
	int16  acc_type = 0;
	PPIDArray cur_acc_list;
	PPIDArray cur_list;
	PPAccount cur_acc_rec, acc_rec;
	if(pAccPack) {
		THROW_PP(pAccPack->Rec.CurID == 0, PPERR_WACCSCURACC);
		if(*strip(pAccPack->Rec.Code) == 0 || pAccPack->Rec.Type == ACY_BAL)
			AccountCore::GenerateCode(&pAccPack->Rec);
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID == 0) {
			PPID   new_id = 0;
			acc_type = pAccPack->Rec.Type;
			if(acc_type == ACY_AGGR) {
				if(pAccPack->Rec.Ac == 0)
					GetAggrNumber(&pAccPack->Rec);
			}
			THROW(AddObjRecByID(this, PPOBJ_ACCOUNT, &new_id, &pAccPack->Rec, 0));
			pAccPack->Rec.ID = new_id;
			ASSIGN_PTR(pID, new_id);
			if(pAccPack->Rec.CurID == 0 && pAccPack->CurList.getCount())
				for(i = 0; i < pAccPack->CurList.getCount(); i++)
					THROW(AddCurRecord(&pAccPack->Rec, pAccPack->CurList.at(i)));
			DS.LogAction(PPACN_OBJADD, PPOBJ_ACCOUNT, new_id, 0, 0);
		}
		else if(pAccPack) {
			THROW(Search(*pID) > 0);
			pAccPack->Rec.ID = *pID;
			acc_type = pAccPack->Rec.Type;
			THROW_DB(updateRecBuf(&pAccPack->Rec));
			if(pAccPack->Rec.CurID == 0) {
				THROW(GetCurList(pAccPack->Rec.Ac, pAccPack->Rec.Sb, &cur_acc_list, &cur_list));
				for(i = 0; i < pAccPack->CurList.getCount(); i++) {
					PPID   cur_id = pAccPack->CurList.at(i);
					uint   pos = 0;
					if(cur_list.lsearch(cur_id, &pos)) {
						PPID   acc_id = cur_acc_list.at(pos);
						THROW(Search(acc_id, &cur_acc_rec) > 0);
						cur_acc_rec.Ac = pAccPack->Rec.Ac;
						cur_acc_rec.Sb = pAccPack->Rec.Sb;
						STRNSCPY(cur_acc_rec.Name, pAccPack->Rec.Name);
						cur_acc_rec.AccSheetID = pAccPack->Rec.AccSheetID;
						cur_acc_rec.Kind  = pAccPack->Rec.Kind;
						cur_acc_rec.Limit = cur_acc_rec.Overdraft = 0;
						THROW_DB(updateRecBuf(&cur_acc_rec));
						cur_list.atFree(pos);
						cur_acc_list.atFree(pos);
					}
					else
						THROW(AddCurRecord(&pAccPack->Rec, cur_id));
				}
				for(i = 0; i < cur_acc_list.getCount(); i++) {
					THROW_DB(deleteFrom(this, 0, (this->ID == cur_acc_list.get(i))));
				}
			}
			DS.LogAction(PPACN_OBJUPD, PPOBJ_ACCOUNT, *pID, 0, 0);
		}
		else {
			THROW(Search(*pID, &acc_rec) > 0);
			acc_type = acc_rec.Type;
			if(acc_rec.Sb == 0) {
				THROW_PP(HasAnySubacct(acc_rec.Ac) <= 0, PPERR_ACCHASBRANCH);
				THROW(Search(*pID, &acc_rec) > 0);
			}
			THROW_DB(deleteRec());
			if(acc_rec.CurID == 0) {
				THROW(GetCurList(acc_rec.Ac, acc_rec.Sb, &cur_acc_list, 0));
				for(i = 0; i < cur_acc_list.getCount(); i++) {
					THROW_DB(deleteFrom(this, 0, (this->ID == cur_acc_list.get(i))));
				}
			}
		}
		if(acc_type == ACY_AGGR)
			THROW(PPRef->PutPropArray(PPOBJ_ACCOUNT, *pID, ACCPRP_GENACCLIST, pAccPack ? &pAccPack->GenList : 0, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI AccountCore::Remove(PPID id, int use_ta)
{
	return PutPacket(&id, 0, use_ta);
}

int SLAPI AccountCore::Search(PPID id, PPAccount * pRec)
{
	return SearchByID(this, PPOBJ_ACCOUNT, id, pRec);
}

int SLAPI AccountCore::SearchBase(PPID curAccID, PPID * pBaseAccID, void * b)
{
	int    ok = 1;
	PPAccount rec;
	ASSIGN_PTR(pBaseAccID, 0);
	THROW(Search(curAccID, &rec) > 0);
	if(rec.CurID)
		THROW_PP(SearchNum(rec.Ac, rec.Sb, 0L, &rec) > 0, PPERR_BASEACCNFOUND);
	copyBufTo(b);
	ASSIGN_PTR(pBaseAccID, rec.ID);
	CATCHZOK
	return ok;
}

int SLAPI AccountCore::SearchCur(PPID accID, PPID curID, PPID * pCurAccID, void * b)
{
	int    ok = 1;
	PPAccount rec;
	ASSIGN_PTR(pCurAccID, 0);
	THROW(Search(accID, &rec) > 0);
	if(rec.CurID != curID)
		THROW_PP(SearchNum(rec.Ac, rec.Sb, curID, &rec) > 0, PPERR_CURACCNFOUND);
	copyBufTo(b);
	ASSIGN_PTR(pCurAccID, rec.ID);
	CATCHZOK
	return ok;
}

int SLAPI AccountCore::GetSubacctList(int ac, int sb, PPID curID, PPIDArray * pList)
{
	AccountTbl::Key1 k1;
	k1.Ac = ac;
	k1.Sb = (sb >= 0) ? sb : 0;
	k1.CurID = 0;
	for(int sp = spGe; search(1, &k1, sp) && k1.Ac == ac && (sb < 0 || k1.Sb == sb); sp = spGt)
		if((curID < 0 || k1.CurID == curID) && !pList->addUnique(data.ID))
			return 0;
	if(BTROKORNFOUND)
		return pList->getCount() ? 1 : (PPErrCode = PPERR_ACCNFOUND, -1);
	else
		return PPSetErrorDB();
}

int SLAPI AccountCore::HasAnySubacct(int ac)
{
	AccountTbl::Key1 k1;
	k1.Ac = ac;
	k1.Sb = 0;
	k1.CurID = 0;
	while(search(1, &k1, spGt) && k1.Ac == ac)
		if(k1.Sb != 0)
			return 1;
	return PPDbSearchError();
}

int SLAPI AccountCore::SearchCode(const char * pCode, PPID curID, PPAccount * pRec)
{
	AccountTbl::Key2 k2;
	MEMSZERO(k2);
	STRNSCPY(k2.Code, pCode);
	for(int sp = spGe; search(2, &k2, sp) && stricmp866(pCode, k2.Code) == 0; sp = spGt)
		if(k2.CurID == curID)
			return (copyBufTo(pRec), 1);
	return (BTROKORNFOUND) ? (PPSetError(PPERR_ACCNFOUND, pCode), -1) : PPSetErrorDB();
}

int SLAPI AccountCore::InitAccSheetForAcctID(AcctID * pAcctId, PPID * pAccSheetID)
{
	int    r = 1;
	*pAccSheetID = 0;
	if(pAcctId->ac)
		if(Search(pAcctId->ac) > 0) {
			*pAccSheetID = data.AccSheetID;
			if(*pAccSheetID == 0)
				pAcctId->ar = 0;
		}
		else
			r = 0;
	return r;
}

int SLAPI AccountCore::ParseString(const char * pStr, int tok[])
{
	int    i = 0;
	char   temp_buf[64];
	char * p = strtok(STRNSCPY(temp_buf, pStr), ".,");
	if(p)
		do {
			if(i == 0 && (!isdec(p[0]) || p[0] == '0'))
				tok[i++] = (SearchCode(p, 0L, 0) > 0) ? data.Ac : 0;
			else
				tok[i++] = atoi(p);
		} while(i < 3 && (p = strtok(0, ".,")) != 0);
	while(i < 3)
		tok[i++] = 0;
	return 1;
}

#endif // } 0 @v9.0.4
//
// ArticleCore
//
SLAPI ArticleCore::ArticleCore() : ArticleTbl()
{
}

int SLAPI ArticleCore::Search(PPID id, void * b)
{
	return SearchByID(this, PPOBJ_ARTICLE, id, b);
}

int SLAPI ArticleCore::SearchName(PPID accSheetID, const char * pName, void * b)
{
	ArticleTbl::Key2 k2;
	MEMSZERO(k2);
	k2.AccSheetID = accSheetID;
	STRNSCPY(k2.Name, pName);
	return SearchByKey(this, 2, &k2, b);
}

int SLAPI ArticleCore::EnumBySheet(PPID accSheetID, long * pArticleNo, void * b)
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

int SLAPI ArticleCore::Count(PPID accSheetID, long * pCount)
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

int SLAPI ArticleCore::GetListBySheet(PPID accSheetID, PPIDArray * pList, long * pCount)
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
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
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

int SLAPI ArticleCore::GetListByGroup(PPID grpArID, PPIDArray * pList)
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

int SLAPI ArticleCore::_SearchNum(PPID accSheetID, long articleNo, int spMode, void * b)
{
	ArticleTbl::Key1 k;
	k.AccSheetID = accSheetID;
	k.Article = articleNo;
	if(search(1, &k, spMode) && k.AccSheetID == accSheetID)
		return (copyBufTo(b), 1);
	return (BTROKORNFOUND) ? (PPErrCode = PPERR_ARTICLENFOUND, -1) : PPSetErrorDB();
}

int SLAPI ArticleCore::SearchFreeNum(PPID accSheetID, long * pAr, void * b)
{
	int    r;
	if(*pAr)
		r = _SearchNum(accSheetID, *pAr, spEq, b);
	else {
		r = _SearchNum(accSheetID, MAXLONG, spLt, b);
		if(r > 0)
			*pAr = data.Article+1;
		else if(r < 0)
			*pAr = 1;
	}
	return r;
}

int SLAPI ArticleCore::SearchObjRef(PPID sheet, PPID id, ArticleTbl::Rec * b)
{
	ArticleTbl::Key3 k;
	k.AccSheetID = sheet;
	k.ObjID = id;
	return SearchByKey(this, 3, &k, b);
}

int SLAPI ArticleCore::PersonToArticle(PPID personID, PPID accSheetID, PPID * pArID)
{
	int    ok = -1;
	PPID   ar_id = 0;
	if(accSheetID && SearchObjRef(accSheetID, personID, 0) > 0) {
		ar_id = data.ID;
		ok = 1;
	}
	ASSIGN_PTR(pArID, ar_id);
	return ok;
}

int SLAPI ArticleCore::SearchNum(PPID sheet, long ar, void * b)
{
	return _SearchNum(sheet, ar, ar ? spEq : spGe, b);
}

int SLAPI ArticleCore::Add(PPID * pID, void * b, int use_ta)
{
	return AddObjRecByID(this, PPOBJ_ARTICLE, pID, b, use_ta);
}

int SLAPI ArticleCore::Update(PPID id, void * b, int use_ta)
{
	return UpdateByID(this, PPOBJ_ARTICLE, id, b, use_ta);
}

int SLAPI ArticleCore::Remove(PPID id, int use_ta)
{
   return RemoveByID(this, id, use_ta);
}
//
// AcctRel
//
SLAPI AcctRel::AcctRel() : AcctRelTbl()
{
}

int SLAPI AcctRel::Search(PPID id, AcctRelTbl::Rec * pRec)
{
	return SearchByID(this, PPOBJ_ACCTREL, id, pRec);
}

int SLAPI AcctRel::SearchAcctID(const AcctID * pAcctId, AcctRelTbl::Rec * pRec)
{
	AcctRelTbl::Key1 k1;
	k1.AccID     = pAcctId->ac;
	k1.ArticleID = pAcctId->ar;
	return SearchByKey(this, 1, &k1, pRec);
}

int SLAPI AcctRel::SearchNum(int closed, const Acct * pAcct, PPID curID, AcctRelTbl::Rec * pRec)
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

int SLAPI AcctRel::OpenAcct(PPID * pID, const Acct * pAcct, PPID curID, const AcctID * pAcctId, int accKind, int accsLevel)
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

int SLAPI AcctRel::CloseAcct(PPID id, int use_ta)
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

int SLAPI AcctRel::ReplaceAcct(int oldAc, int oldSb, int newAc, int newSb)
{
	int    ok = 1;
	AcctRelTbl::Key3 k;
	MEMSZERO(k);
	k.Ac = oldAc;
	k.Sb = oldSb;
	/* @v10.3.0 for(int sp = spGe; searchForUpdate(3, &k, sp) && k.Ac == oldAc && k.Sb == oldSb; sp = spGt) {
		data.Ac = newAc;
		data.Sb = newSb;
		THROW_DB(updateRec()); // @sfu-r
	}*/
	// @v10.3.0 {
	if(searchForUpdate(3, &k, spGe) && k.Ac == oldAc && k.Sb == oldSb) do {
		data.Ac = newAc;
		data.Sb = newSb;
		THROW_DB(updateRec()); // @sfu-r
	} while(searchForUpdate(3, &k, spNext) && k.Ac == oldAc && k.Sb == oldSb);
	// } @v10.3.0 
	THROW_DB(BTROKORNFOUND);
	CATCHZOK
	return ok;
}

int SLAPI AcctRel::EnumByAcc(PPID accID, PPID * pArID, AcctRelTbl::Rec * pRec)
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

int SLAPI AcctRel::EnumByArticle(PPID arID, PPID * pAccID, AcctRelTbl::Rec * pRec)
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

SEnumImp * SLAPI AcctRel::Enum(int keyN, PPID keyID)
{
	union {
		AcctRelTbl::Key1 k1;
		AcctRelTbl::Key2 k2;
	} k;
	MEMSZERO(k);
	SEnumImp * p_enum = 0;
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
	q->initIteration(0, &k, spGe);
	THROW(EnumList.RegisterIterHandler(q, &h));
	THROW_MEM(p_enum = new PPTblEnum <AcctRel>(this, h));
	CATCH
		EnumList.DestroyIterHandler(h);
	ENDCATCH
	return p_enum;
}

SEnumImp * SLAPI AcctRel::EnumByAcc(PPID accID)
	{ return Enum(1, accID); }
SEnumImp * SLAPI AcctRel::EnumByArticle(PPID arID)
	{ return Enum(2, arID); }
//
//
//
class AcctRelCache : public ObjCache {
public:
	SLAPI  AcctRelCache() : ObjCache(PPOBJ_ACCTREL, sizeof(Data)) {}
private:
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
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

int SLAPI AcctRelCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
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

void SLAPI AcctRelCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
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

int SLAPI AcctRel::Fetch(PPID id, AcctRelTbl::Rec * pRec)
{
	AcctRelCache * p_cache = GetDbLocalCachePtr <AcctRelCache> (PPOBJ_ACCTREL);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}
