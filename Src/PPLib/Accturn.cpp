// ACCTURN.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2017, 2018, 2019
// @codepage windows-1251
// @Kernel
//
#include <pp.h>
#pragma hdrstop

TLP_IMPL(AccTurnCore, AccTurnCore::FrrlData, Frrl);

SLAPI AccTurnCore::AccTurnCore() : AccTurnTbl(), Frrl(0) /*, AccT(*AccObj.P_Tbl)*/
{
}

int SLAPI AccTurnCore::AcctIDToRel(const AcctID * pAcctId, PPID * pAccRelID)
{
	int    r = AccRel.SearchAcctID(pAcctId);
	*pAccRelID = (r > 0) ? AccRel.data.ID : 0;
	return r;
}

int SLAPI AccTurnCore::AcctRelToID(PPID relID, AcctID * pAcctId, PPID * pAccSheetID)
{
	int    ok = -1;
	PPID   acc_sheet_id = 0;
	AcctRelTbl::Rec acrel_rec;
	if(AccRel.Search(relID, &acrel_rec) > 0) {
		if(pAcctId) {
			pAcctId->ac = acrel_rec.AccID;
			pAcctId->ar = acrel_rec.ArticleID;
		}
		acc_sheet_id = (Art.Search(acrel_rec.ArticleID) > 0) ? Art.data.AccSheetID : 0;
		ok = 1;
	}
	else {
		CALLPTRMEMB(pAcctId, Clear());
	}
	ASSIGN_PTR(pAccSheetID, acc_sheet_id);
	return ok;
}

int SLAPI AccTurnCore::UpdateItemInExtGenAccList(PPID objID, long f, PPID accID, ObjRestrictArray * pAccList, PPIDArray * pCurList)
{
	int    ok = 1;
	THROW(pAccList->UpdateItemByID(objID, f));
	THROW(!pCurList || AccObj.GetCurList(accID, 0, pCurList));
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::GetExtentAccListByGen(PPID genAccID, ObjRestrictArray * pAccList, PPIDArray * pCurList)
{
	int    ok = 1;
	ObjRestrictItem * p_item;
	PPAccountPacket pack;
	PPAccount acc_rec;
	PPIDArray temp_acc_list;
	THROW(AccObj.GetPacket(genAccID, &pack) > 0);
	THROW_PP(pack.Rec.Type == ACY_AGGR, PPERR_ACCNGEN);
	for(uint i = 0; pack.GenList.enumItems(&i, reinterpret_cast<void **>(&p_item));) {
		long   f = p_item->Flags & ~(ACGF_ACO1GRP | ACGF_ACO2GRP);
		int    aco = abs(GetAcoByGenFlags(p_item->Flags));
		if(aco == ACO_3) {
			AcctID acctid;
			if(AcctRelToID(p_item->ObjID, &acctid, 0) > 0)
				THROW(UpdateItemInExtGenAccList(p_item->ObjID, f, acctid.ac, pAccList, pCurList));
		}
		else if(AccObj.Search(p_item->ObjID, &acc_rec) > 0) {
			if(aco == ACO_1) {
				const int16 sb = NZOR(acc_rec.A.Sb, -1);
				temp_acc_list.clear();
				THROW(AccObj.GetSubacctList(acc_rec.A.Ac, sb, 0L, &temp_acc_list));
				for(uint j = 0; j < temp_acc_list.getCount(); j++) {
					const PPID acc_id = temp_acc_list.at(j);
					THROW(UpdateItemInExtGenAccList(acc_id, (f|ACGF_ACO2GRP), acc_id, pAccList, pCurList));
				}
			}
			else if(aco == ACO_2) {
				THROW(UpdateItemInExtGenAccList(p_item->ObjID, (f|ACGF_ACO2GRP), p_item->ObjID, pAccList, pCurList));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::GetBaseAcctID(const AcctID & rCurAcctId, AcctID * pBaseAcctId)
{
	PPID   base_acc_id = 0;
	if(AccObj.SearchBase(rCurAcctId.ac, &base_acc_id, 0) > 0) {
		pBaseAcctId->ac = base_acc_id;
		pBaseAcctId->ar = rCurAcctId.ar;
		return 1;
	}
	else
		return 0;
}

int SLAPI AccTurnCore::ConvertAcct(const Acct * pAcct, PPID curID, AcctID * pAcctId, PPID * pSheetID)
{
	int    ok = 1;
	PPID   sheet_id = 0;
	pAcctId->Clear();
	PPAccount acc_rec;
	THROW(ok = AccObj.FetchNum(pAcct->ac, pAcct->sb, curID, &acc_rec));
	if(ok > 0) {
		sheet_id   = acc_rec.AccSheetID;
		pAcctId->ac = acc_rec.ID;
		if(pAcct->ar) {
			THROW(ok = Art.SearchNum(sheet_id, pAcct->ar));
			if(ok > 0)
				pAcctId->ar = Art.data.ID;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pSheetID, sheet_id);
	return ok;
}

int SLAPI AccTurnCore::ConvertAcctID(const AcctID & rAci, Acct * pAcct, PPID * pCurID, int useCache)
{
	int    ok = 1;
	pAcct->Clear();
	PPAccount acc_rec;
	if(useCache) {
		PPObjAccount acc_obj;
		THROW(ok = acc_obj.Fetch(rAci.ac, &acc_rec));
	}
	else {
		THROW(ok = AccObj.Search(rAci.ac, &acc_rec));
	}
	if(ok > 0) {
		pAcct->ac = acc_rec.A.Ac;
		pAcct->sb = acc_rec.A.Sb;
		ASSIGN_PTR(pCurID, acc_rec.CurID);
		if(rAci.ar) {
			ArticleTbl::Rec ar_rec;
			AcctRelTbl::Rec acr_rec;
			if(useCache) {
				PPObjArticle ar_obj;
				THROW(ok = ar_obj.Fetch(rAci.ar, &ar_rec));
			}
			else {
				THROW(ok = Art.Search(rAci.ar, &ar_rec));
			}
			if(ok > 0)
				pAcct->ar = ar_rec.Article;
			else if(AccRel.SearchAcctID(&rAci, &acr_rec) > 0)
				pAcct->ar = acr_rec.Ar;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::ConvertStr(const char * pStr, PPID curID, Acct * pAcct, AcctID * pAcctId, PPID * pAccSheetID)
{
	int    ok = 1, r, tok[3], hasbranch = 0;
	PPID   acc_id = 0, sheet_id = 0, ar_id  = 0;
	PPAccount acc_rec;
	pAcct->Clear();
	AccObj.ParseString(pStr, tok);
	THROW_PP(tok[0], PPERR_ACCNFOUND);
	THROW(r = AccObj.FetchNum(tok[0], 0, curID, &acc_rec));
	THROW_PP_S(r > 0, PPERR_BALNOTEXISTS, pStr);
	acc_id    = acc_rec.ID;
	sheet_id  = acc_rec.AccSheetID;
	hasbranch = BIN(AccObj.HasAnySubacct(tok[0]) > 0);
	pAcct->ac  = tok[0];
	if(tok[1]) {
		if(hasbranch) {
			THROW(AccObj.FetchNum(tok[0], tok[1], curID, &acc_rec) > 0); // @v6.0.9
			acc_id    = acc_rec.ID;
			sheet_id  = acc_rec.AccSheetID;
			pAcct->sb = tok[1];
			tok[1]    = tok[2];
		}
		if(tok[1]) {
			THROW_PP(sheet_id, PPERR_ACCHASNTBRANCHES);
			THROW(Art.SearchNum(sheet_id, tok[1]));
			ar_id     = Art.data.ID;
			pAcct->ar = tok[1];
		}
	}
	ASSIGN_PTR(pAccSheetID, sheet_id);
	if(pAcctId) {
		pAcctId->ac = acc_id;
		pAcctId->ar = ar_id;
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::GetAcctRest(LDATE date, PPID accrel, double * pRest, int incoming)
{
	int    ok = 1;
	double rest = 0.0;
	AccTurnTbl::Key1 k1;
	if(accrel) {
		if(!incoming)
			plusdate(&date, 1, 0);
		k1.Acc   = accrel;
		k1.Dt    = date;
		k1.OprNo = 0;
		if(search(1, &k1, spLt) && k1.Acc == accrel)
			rest = MONEYTOLDBL(data.Rest);
		else if(!BTROKORNFOUND)
			ok = PPSetErrorDB();
	}
	ASSIGN_PTR(pRest, rest);
	return ok;
}

int SLAPI AccTurnCore::GetBalRest(LDATE dt, PPID accID, double * pDbt, double * pCrd, uint flags)
{
	int    ok = 1;
	double dbt = 0.0, crd = 0.0;
	if(flags & BALRESTF_INCOMING && dt)
		dt = plusdate(dt, -1);
	PPAccount acc_rec;
	if(AccObj.Search(accID, &acc_rec) > 0) {
		//
		// Рекурсия по субсчетам
		//
		if(flags & BALRESTF_ACO1GROUPING) {
			PPIDArray acc_list;
			AccObj.GetSubacctList(acc_rec.A.Ac, -1, acc_rec.CurID, &acc_list);
			for(uint i = 0; i < acc_list.getCount(); i++) {
				double d = 0.0, c = 0.0;
				const  uint fl = (flags & ~(BALRESTF_ACO1GROUPING | BALRESTF_INCOMING));
				GetBalRest(dt, acc_list.at(i), &d, &c, fl);
				dbt += d;
				crd += c;
			}
			if(flags & BALRESTF_SPREAD && acc_rec.Kind != ACT_AP) {
				dbt -= crd;
				if(dbt < 0) {
					crd = -dbt;
					dbt = 0;
				}
				else
					crd = 0;
			}
		}
		//
		// Неразвернутое сальдо
		//
		else if(!(flags & BALRESTF_SPREAD)) {
			ok = BalTurn.GetRest(accID, dt, &dbt, &crd);
		}
		//
		// Развернутое сальдо
		//
		else if(acc_rec.Kind == ACT_AP && acc_rec.AccSheetID) {
			AcctRelTbl::Key1 k;
			BExtQuery q(&AccRel, 1);
			q.selectAll().where(AccRel.AccID == accID && AccRel.Closed == 0L);
			k.AccID = accID;
			k.ArticleID = 0;
			for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
				double rest = 0.0;
				GetAcctRest(dt, AccRel.data.ID, &rest, 0);
				if(rest > 0)
					dbt += R6(rest);
				else
					crd -= R6(rest);
			}
		}
		else {
			ok = BalTurn.GetRest(accID, dt, &dbt, &crd);
			dbt -= crd;
			if(dbt < 0) {
				crd = -dbt;
				dbt = 0;
			}
			else
				crd = 0;
		}
	}
	ASSIGN_PTR(pDbt, dbt);
	ASSIGN_PTR(pCrd, crd);
	return ok;
}

void SLAPI AccTurnCore::GetAccRelIDs(const AccTurnTbl::Rec * pRec, PPID * pDbtRelID, PPID * pCrdRelID) const
{
	ASSIGN_PTR(pDbtRelID, pRec->Reverse ? pRec->CorrAcc : pRec->Acc);
	ASSIGN_PTR(pCrdRelID, pRec->Reverse ? pRec->Acc : pRec->CorrAcc);
}

int SLAPI AccTurnCore::ConvertRec(const AccTurnTbl::Rec * pRec, PPAccTurn * pAturn, int useCache)
{
	int    ok = 1;
	PPID   acc_id = 0, cur_id = 0;
	AcctRelTbl::Rec acr_rec;
	PPAccount acc_rec;
	SETIFZ(pRec, &data);
	int    reverse = pRec->Reverse;
	memzero(pAturn, sizeof(*pAturn));
	acc_id = reverse ? pRec->CorrAcc : pRec->Acc;
	if(useCache) {
		THROW(AccRel.Fetch(acc_id, &acr_rec) > 0);
	}
	else {
		THROW(AccRel.Search(acc_id, &acr_rec) > 0);
	}
	cur_id = acr_rec.CurID;
	(*(AcctID*)&pAturn->DbtID.ac) = (*(AcctID*)&acr_rec.AccID);
	acc_id = reverse ? pRec->Acc : pRec->CorrAcc;
	if(acc_id) {
		if(useCache) {
			THROW(AccRel.Fetch(acc_id, &acr_rec) > 0);
		}
		else {
			THROW(AccRel.Search(acc_id, &acr_rec) > 0);
		}
		(*(AcctID*)&pAturn->CrdID.ac) = (*(AcctID*)&acr_rec.AccID);
	}
	pAturn->Flags = 0;
	if(useCache) {
		THROW(AccObj.Fetch(pAturn->DbtID.ac, &acc_rec) > 0);
	}
	else {
		THROW(AccObj.Search(pAturn->DbtID.ac, &acc_rec) > 0);
	}
	pAturn->DbtSheet = acc_rec.AccSheetID;
	if(acc_rec.Type == ACY_OBAL) {
		pAturn->Flags |= PPAF_OUTBAL;
		if(acc_id)
			pAturn->Flags |= PPAF_OUTBAL_TRANSFER;
	}
	if(acc_rec.Type == ACY_REGISTER)
		pAturn->Flags |= PPAF_REGISTER;
	if(!(pAturn->Flags & PPAF_REGISTER) && (!(pAturn->Flags & PPAF_OUTBAL) || (pAturn->Flags & PPAF_OUTBAL_TRANSFER))) {
		if(useCache) {
			THROW(AccObj.Fetch(pAturn->CrdID.ac, &acc_rec) > 0);
		}
		else {
			THROW(AccObj.Search(pAturn->CrdID.ac, &acc_rec) > 0);
		}
		pAturn->CrdSheet = acc_rec.AccSheetID;
	}
	pAturn->Date    = pRec->Dt;
	pAturn->BillID  = pRec->Bill;
	pAturn->RByBill = pRec->RByBill;
	pAturn->CurID   = cur_id;
	pAturn->CRate   = 0.0;
	pAturn->Amount  = MONEYTOLDBL(pRec->Amount);
	pAturn->Opr     = 0;
	if(pAturn->Flags & PPAF_OUTBAL) {
		if(!(pAturn->Flags & PPAF_OUTBAL_TRANSFER)) {
			if(pAturn->Amount < 0.0) {
				pAturn->Amount = -pAturn->Amount;
				pAturn->Flags |= PPAF_OUTBAL_WITHDRAWAL;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::AccBelongToOrd(PPID accRelID, int ord, const Acct * pAcct, PPID curID, int useCache)
{
	int    ok = 1;
	if(ord) {
		AcctRelTbl::Rec acr_rec;
		int    r;
		if(accRelID)
			r = useCache ? AccRel.Fetch(accRelID, &acr_rec) : AccRel.Search(accRelID, &acr_rec);
		else {
			// @v10.6.4 MEMSZERO(acr_rec);
			r = 1;
		}
		if(r > 0) {
			if(curID >= 0 && acr_rec.CurID != curID)
				ok = -1;
			else {
				int    match = 0;
				if(acr_rec.Ac == pAcct->ac)
					match = (acr_rec.Sb == pAcct->sb) ? ((acr_rec.Ar == pAcct->ar) ? ACO_3 : ACO_2) : ACO_1;
				if(ord == ACO_1)
					ok = (match > 0) ? 1 : -1;
				else if(ord == ACO_2)
					ok = (match > 1) ? 1 : -1;
				else
					ok = (match > 2) ? 1 : -1;
			}
		}
		else
			ok = 0;
	}
	return ok;
}
//
// @unused
// Функция NormalyzeAcc приводит счет pAccID к нормальному виду.
// То есть, заменяет *pAccID на счет в базовой валюте. Параметр
// aco определяет о счете какого порядка идет речь. Если aco == ACO_3
// то предполагается, что *pAccID -> AcctRel.ID, в противном случае
// *pAccID -> Account.ID.
// По указателю pCurID присваивается ид валюты, к которой относилс
// счет *pAccID до нормализации.
//
int SLAPI AccTurnCore::NormalyzeAcc(int aco, PPID * pAccID, PPID * pCurID)
{
	int    ok = 1;
	if(oneof2(aco, ACO_1, ACO_2)) {
		PPAccount acc_rec;
		THROW(AccObj.Search(*pAccID, &acc_rec) > 0);
		ASSIGN_PTR(pCurID, acc_rec.CurID);
		if(acc_rec.CurID)
			THROW(AccObj.SearchBase(*pAccID, pAccID, 0) > 0);
	}
	else if(aco == ACO_3) {
		THROW(AccRel.Search(*pAccID) > 0);
		ASSIGN_PTR(pCurID, AccRel.data.CurID);
		if(AccRel.data.CurID) {
			Acct acct;
			acct.ac = AccRel.data.Ac;
			acct.sb = AccRel.data.Sb;
			acct.ar = AccRel.data.Ar;
			THROW(AccRel.SearchNum(0, &acct, 0L) > 0);
			*pAccID = AccRel.data.ID;
		}
	}
	else {
		CALLEXCEPT_PP(PPERR_INVPARAM);
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::IdentifyAcc(long * pAco, PPID * pAccID, PPID curID, PPID personRelID, PPIDArray * pAccList)
{
	int    ok = 1;
	PPID   cur_id = 0;
	int    sb = -1;
	int    aco = *pAco;
	PPAccount acc_rec;
	if(aco == ACO_1 || (aco == ACO_2 && curID < 0)) {
		if(curID < 0) {
			cur_id = -1;
			if(aco == ACO_2)
				THROW(AccObj.SearchBase(*pAccID, pAccID, 0) > 0);
		}
		else {
			cur_id = curID;
			if(aco == ACO_2)
				THROW(AccObj.SearchCur(*pAccID, curID, pAccID, 0) > 0);
		}
		THROW(AccObj.Search(*pAccID, &acc_rec) > 0);
		sb = (aco == ACO_1) ? -1 : acc_rec.A.Sb;
		THROW(AccObj.GetSubacctList(acc_rec.A.Ac, sb, cur_id, pAccList));
		if(curID < 0 && aco == ACO_2)
			THROW(pAccList->addUnique(*pAccID));
		if(pAccList->getCount() == 1) {
			aco = ACO_2;
			*pAccID = pAccList->at(0);
		}
		ok = pAccList->getCount();
	}
	else if(aco == ACO_2) {
		THROW(AccObj.SearchCur(*pAccID, curID, pAccID, 0) > 0);
	}
	else if(aco == ACO_3) {
		AcctRelTbl::Rec acr_rec;
		PPIDArray ar_list, temp_list;
		THROW(AccRel.Search(*pAccID, &acr_rec) > 0);
		const PPID acc_id = acr_rec.AccID;
		if(personRelID) {
			PPObjArticle ar_obj;
			ar_obj.GetRelPersonList(acr_rec.ArticleID, personRelID, 1, &temp_list);
			ar_list.addUnique(&temp_list);
		}
		ar_list.addUnique(acr_rec.ArticleID);
		for(uint j = 0; j < ar_list.getCount(); j++) {
			AcctID acctid;
			acctid.ac = acc_id;
			acctid.ar = ar_list.get(j);
			if(curID < 0) {
				temp_list.clear();
				THROW(AccObj.SearchBase(acctid.ac, &acctid.ac, 0) > 0);
				THROW(AccObj.GetCurList(acctid.ac, &temp_list, 0));
				THROW_SL(temp_list.addUnique(acctid.ac));
				for(uint i = 0; i < temp_list.getCount(); i++) {
					acctid.ac = temp_list.get(i);
					if(AccRel.SearchAcctID(&acctid, &acr_rec) > 0)
						THROW_SL(pAccList->addUnique(acr_rec.ID));
				}
			}
			else {
				THROW(AccObj.SearchCur(acctid.ac, curID, &acctid.ac, 0) > 0);
				if(AccRel.SearchAcctID(&acctid, &acr_rec) > 0)
					THROW_SL(pAccList->addUnique(acr_rec.ID));
			}
		}
		if(pAccList->getCount() == 1)
			*pAccID = pAccList->at(0);
		ok = pAccList->getCount();
	}
	else {
		CALLEXCEPT_PP(PPERR_INVPARAM);
	}
	*pAco = aco;
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::GetAcctCurID(int aco, PPID accID, PPID * pCurID)
{
	int    ok = 1;
	ASSIGN_PTR(pCurID, 0);
	if(aco == ACO_3) {
		if(AccRel.Search(accID) > 0) {
			ASSIGN_PTR(pCurID, AccRel.data.CurID);
		}
		else
			ok = 0;
	}
	else {
		PPAccount acc_rec;
		if(AccObj.Search(accID, &acc_rec) > 0) {
			ASSIGN_PTR(pCurID, acc_rec.CurID);
		}
		else
			ok = 0;
	}
	return ok;
}

int SLAPI AccTurnCore::RemoveEmptyAcctRels()
{
	int    ok = 1;
	int    found;
	PPID   rel = 0;
	PPWait(1);
	{
		PPTransaction tra(1);
		THROW(tra);
		while(AccRel.search(0, &rel, spGt)) {
			char msg_buf[64];
			reinterpret_cast<const Acct *>(&AccRel.data.Ac)->ToStr(ACCF_DEFAULT, msg_buf);
			PPWaitMsg(msg_buf);
			AccTurnTbl::Key1 k;
			MEMSZERO(k);
			k.Acc = rel;
			THROW_DB((found = search(1, &k, spGe)) != 0 || BTRNFOUND);
			if(!found || k.Acc != rel)
				THROW_DB(AccRel.deleteRec());
		}
		THROW(tra.Commit());
	}
	PPWait(0);
	CATCHZOKPPERR
	return ok;
}

int SLAPI AccTurnCore::UpdateRelsArRef(PPID arID, long arNo, int use_ta)
{
	int    ok = 1;
	PPID   acc_id = 0;
	AcctRelTbl::Rec rel_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Art.Search(arID) > 0);
		for(acc_id = 0; AccRel.EnumByArticle(arID, &acc_id, &rel_rec) > 0;) {
			if(!rel_rec.Closed) {
				AcctRelTbl::Key3 dk;
				dk.Ac = rel_rec.Ac;
				dk.Sb = rel_rec.Sb;
				dk.Ar = arNo;
				dk.CurID  = rel_rec.CurID;
				dk.Closed = rel_rec.Closed;
				if(AccRel.search(3, &dk, spEq))
					if(rel_rec.AccID == AccRel.data.AccID && rel_rec.ID != AccRel.data.ID)
						if(Art.Search(AccRel.data.ArticleID) > 0) {
							THROW(UpdateRelsArRef(Art.data.ID, Art.data.Article, 0)); // @recursion
						}
						else {
							AccRel.data.Closed = 1;
							THROW_DB(AccRel.updateRec());
							continue;
						}
					else
						continue;
				if(AccRel.Search(rel_rec.ID) > 0) {
					AccRel.data.Ar = arNo;
					THROW_DB(AccRel.updateRec());
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::IsFRRLocked()
{
	return BIN(Frrl && Frrl->Counter > 0);
}
//
// @v6.2.3 Заменено чтение записей с блокировками на SearchByID_ForUpdate
//
int SLAPI AccTurnCore::LockFRR(PPID accRelID, LDATE dt)
{
	int    ok = -1;
	if(IsFRRLocked()) {
		PPID   k = accRelID;
		PPID   acc_id = 0;
		if(SearchByID_ForUpdate(&AccRel, PPOBJ_ACCTREL, accRelID, 0) > 0) {
			THROW(Frrl->AccrelIdList.addUnique(accRelID));
			acc_id = AccRel.data.AccID;
			AccRel.data.Flags |= ACRF_FRRL;
			if(AccRel.data.FRRL_Date == 0 || dt < AccRel.data.FRRL_Date)
				AccRel.data.FRRL_Date = dt;
			THROW_DB(AccRel.updateRec()); // @sfu
			/*
			if(SearchByID_ForUpdate(&AccT, PPOBJ_ACCOUNT, acc_id, 0) > 0) {
				AccT.data.Flags |= ACF_FRRL;
				if(AccT.data.FRRL_Date == 0 || dt < AccT.data.FRRL_Date)
					AccT.data.FRRL_Date = dt;
				THROW_DB(AccT.updateRec()); // @sfu
			}
			*/
			THROW(AccObj.LockFRR(acc_id, dt, 0 /*lock*/));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI AccTurnCore::LockingFRR(int lock, int * pFRRL_Tag, int use_ta)
{
	int    ok = 1, frrl_tag = 0;
	THROW_INVARG(pFRRL_Tag);
	if(!Frrl) {
		TLP_OPEN(Frrl);
		Frrl->Counter = 0;
	}
	if(lock > 0) {
		Frrl->Counter++;
		frrl_tag = 1;
	}
	else if(*pFRRL_Tag == 0)
		ok = -1;
	else if(Frrl->Counter > 0) {
		Frrl->Counter--;
		if(lock == 0 && Frrl->Counter == 0) {
			uint   i;
			PPID   acc_id = 0, k;
			PPIDArray acc_list;
			PPAccount acc_rec;
			DBRowId pos;
			PPTransaction tra(use_ta);
			THROW(tra);
			for(i = 0; i < Frrl->AccrelIdList.getCount(); i++) {
				acc_id = Frrl->AccrelIdList.at(i);
				if(SearchByID(&AccRel, PPOBJ_ACCTREL, acc_id, 0) > 0) { // @v8.2.0 SearchByID_ForUpdate-->SearchByID
					THROW(acc_list.addUnique(AccRel.data.AccID));
					if(AccRel.data.Flags & ACRF_FRRL) {
						k = acc_id;
						THROW_DB(AccRel.getPosition(&pos));
						THROW(RecalcRest(acc_id, AccRel.data.FRRL_Date, 0, 0, 0));
						THROW_DB(AccRel.getDirectForUpdate(-1, 0, pos)); // @v8.2.0 getDirect-->getDirectForUpdate
						AccRel.data.Flags &= ~ACRF_FRRL;
						AccRel.data.FRRL_Date = ZERODATE;
						THROW_DB(AccRel.updateRec()); // @sfu
					}
				}
			}
			for(i = 0; i < acc_list.getCount(); i++) {
				acc_id = acc_list.at(i);
				if(AccObj.Search(acc_id, &acc_rec) > 0 && acc_rec.Flags & ACF_FRRL) { // @v8.2.0 SearchByID_ForUpdate-->SearchByID
					THROW(RecalcBalance(acc_id, acc_rec.Frrl_Date, 0));
					THROW(AccObj.LockFRR(acc_id, ZERODATE, 1 /*unlock*/));
				}
			}
			Frrl->AccrelIdList.freeAll();
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	ASSIGN_PTR(pFRRL_Tag, frrl_tag);
	return ok;
}

IMPL_CMPFUNC(__Acct, k1, k2) { RET_CMPCASCADE3(static_cast<const Acct *>(k1), static_cast<const Acct *>(k2), ac, sb, ar); }

int SLAPI AccTurnCore::SortGenAccList(ObjRestrictArray * pGenList)
{
	struct _ENTRY {
		int16 ac;
		int16 sb;
		long  ar;
		PPID  id;
		long  flags;
	} entry, * p_entry;
	int    ok = 1;
	uint   i;
	SArray temp_list(sizeof(_ENTRY));
	ObjRestrictItem item, * p_item;
	for(i = 0; pGenList->enumItems(&i, (void **)&p_item);) {
		if(abs(GetAcoByGenFlags(p_item->Flags)) == ACO_3) {
			AcctRelTbl::Rec arel_rec;
			if(AccRel.Search(p_item->ObjID, &arel_rec) > 0) {
				entry.ac = arel_rec.Ac;
				entry.sb = arel_rec.Sb;
				entry.ar = arel_rec.Ar;
				entry.id = p_item->ObjID;
				entry.flags = p_item->Flags;
				THROW_SL(temp_list.ordInsert(&entry, 0, PTR_CMPFUNC(__Acct)));
			}
		}
		else {
			PPAccount rec;
			if(AccObj.Search(p_item->ObjID, &rec) > 0) {
				entry.ac = rec.A.Ac;
				entry.sb = rec.A.Sb;
				entry.ar = 0;
				entry.id = p_item->ObjID;
				entry.flags = p_item->Flags;
				THROW_SL(temp_list.ordInsert(&entry, 0, PTR_CMPFUNC(__Acct)));
			}
		}
	}
	pGenList->freeAll();
	for(i = 0; temp_list.enumItems(&i, (void **)&p_entry);) {
		item.ObjID = p_entry->id;
		item.Flags = p_entry->flags;
		THROW_SL(pGenList->insert(&item));
	}
	CATCHZOK
	return ok;
}

inline int SLAPI AccTurnCore::_OprNo(LDATE date, long * pOprNo)
{
	return IncDateKey(this, 2, date, pOprNo);
}

int SLAPI AccTurnCore::_UpdateForward(LDATE date, long oprno, PPID accRel, const AccTurnParam & rParam)
{
	int    ok = 1;
	if(!IsFRRLocked() && rParam.Amt != 0.0) {
		int    sp = spGt;
		AccTurnTbl::Key1 k1;
		double rest;
		k1.Acc   = accRel;
		k1.Dt    = date;
		k1.OprNo = oprno;
		//
		// Ради ускорения проводок первую запись берем по условию 'больше',
		// а все остальные - одну за другой. При этом изменяем записи без
		// сохранения текущей позиции поскольку поле Rest не входит ни в один
		// индекс.
		//
		while(search(1, &k1, sp) && k1.Acc == accRel) { 
			DBRowId _dbpos;
			THROW_DB(getPosition(&_dbpos));
			THROW_DB(getDirectForUpdate(1, &k1, _dbpos));
			LDBLTOMONEY(rest = MONEYTOLDBL(data.Rest) + rParam.Amt, data.Rest);
			if(DS.RestCheckingStatus(-1)) {
				THROW_PP(rest >= rParam.Low, PPERR_FWTURNLOWBOUND);
				THROW_PP(rest <= rParam.Upp, PPERR_FWTURNUPPBOUND);
			}
			THROW_DB(updateRec()); // @sfu
			sp = spNext;
		}
		THROW_DB(BTROKORNFOUND);
	}
	CATCHZOK
	return ok;
}
//
// Дата проверена, accRel, corrAccRel - ид-ры существующих записей,
// param содержит корректно установленные значени
//
// Заносит проводку в AccTurnCore и модифицирует AcctRel без транзакции
//
int SLAPI AccTurnCore::_Turn(const PPAccTurn * pAt, PPID accRel, PPID corrAccRel, const AccTurnParam & rParam)
{
	int    ok = 1;
	long   oprno;
	double rest;
	THROW(GetAcctRest(pAt->Date, accRel, &rest, 0));
	rest += rParam.Amt;
	if(DS.RestCheckingStatus(-1)) {
		THROW_PP(rest >= rParam.Low, PPERR_TURNLOWBOUND);
		THROW_PP(rest <= rParam.Upp, PPERR_TURNUPPBOUND);
	}
	THROW(_OprNo(pAt->Date, &oprno));
	clearDataBuf();
	if(rParam.Side == PPDEBIT) {
		data.Bal     = pAt->DbtID.ac;
		data.Reverse = 0;
		LDBLTOMONEY(rParam.Amt, data.Amount);
	}
	else {
		data.Bal     = pAt->CrdID.ac;
		data.Reverse = 1;
		LDBLTOMONEY(-rParam.Amt, data.Amount);
	}
	data.Acc     = accRel;
	data.Dt      = pAt->Date;
	data.OprNo   = oprno;
	data.Bill    = pAt->BillID;
	data.RByBill = pAt->RByBill;
	data.CorrAcc = corrAccRel;
	LDBLTOMONEY(rest, data.Rest);
	THROW_DB(insertRec());
	THROW(_UpdateForward(pAt->Date, MAXLONG, accRel, rParam));
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::EnumByBill(PPID billID, int * pRByBill, PPAccTurn * pAt)
{
	int    ok = -1;
	AccTurnTbl::Key0 k0;
	k0.Bill    = billID;
	k0.Reverse = 0;
	k0.RByBill = *pRByBill;
	if(search(0, &k0, spGt) && k0.Bill == billID && k0.Reverse == 0) {
		*pRByBill = k0.RByBill;
		ok = pAt ? ConvertRec(0, pAt, 0) : 1;
	}
	else
		ok = PPDbSearchError();
	return ok;
}

int SLAPI AccTurnCore::_RecByBill(PPID billID, short * pRByBill)
{
	int    spMode;
	AccTurnTbl::Key0 k0;
	if(*pRByBill == 0) {
		spMode     = spLt;
		k0.Bill    = billID /*+1*/;
		k0.Reverse = 0;
		k0.RByBill = BASE_RBB_BIAS /*0*/;
	}
	else {
		spMode     = spEq;
		k0.Bill    = billID;
		k0.Reverse = 0;
		k0.RByBill = *pRByBill;
	}
	if(search(0, &k0, spMode)) {
		if(k0.Bill == billID)
			*pRByBill = k0.RByBill+1;
		else if(*pRByBill)
			return PPSetError(PPERR_TURNBYBILLNFOUND);
		else
			*pRByBill = 1;
		return 1;
	}
	else if(!BTRNFOUND)
		return PPSetErrorDB();
	else if(*pRByBill)
		return PPSetError(PPERR_TURNBYBILLNFOUND);
	*pRByBill = 1;
	return 1;
}

int SLAPI AccTurnCore::GetBill(PPAccTurn * pAt)
{
	return pAt->BillID ? _RecByBill(pAt->BillID, &pAt->RByBill) : PPSetError(PPERR_INVBILLID);
}

static int SLAPI ValidateAccKind(int k)
{
	return oneof3(k, ACT_ACTIVE, ACT_PASSIVE, ACT_AP) ? 1 : PPSetError(PPERR_ACTNDEF);
}

int SLAPI AccTurnCore::GetAcctRel(PPID accID, PPID arID, AcctRelTbl::Rec * pRec, int createIfNExists, int use_ta)
{
	int    ok = -1;
	AcctID acctid;
	acctid.ac = accID;
	acctid.ar = arID;
	if(AccRel.SearchAcctID(&acctid, pRec) > 0) {
		ok = 1;
	}
	else if(createIfNExists) {
		PPID   acr_id = 0;
		Acct   acct;
		PPAccount acc_rec;
		ArticleTbl::Rec ar_rec;
		int    kind = 0;
		PPTransaction tra(use_ta);
		THROW(tra);
		//
		// Проверяем существование счета
		//
		THROW(AccObj.Search(accID, &acc_rec) > 0);
		THROW(ValidateAccKind(kind = acc_rec.Kind));
		acct.ac = acc_rec.A.Ac;
		acct.sb = acc_rec.A.Sb;
		//
		// Проверяем существование статьи
		//
		if(arID) {
			THROW(Art.Search(arID, &ar_rec) > 0);
			acct.ar = ar_rec.Article;
		}
		else
			acct.ar = 0;
		THROW(AccRel.OpenAcct(&acr_id, &acct, acc_rec.CurID, &acctid, kind));
		THROW(tra.Commit());
		ok = 2;
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::_ProcessAcct(int side, PPID curID, const AcctID & rAcctId, PPID * pAccRelID, AccTurnParam * p)
{
	int    ok = 1, r, kind;
	Acct   acct;
	PPAccount acc_rec;
	//
	// Проверяем существование счета
	//
	THROW(AccObj.Search(rAcctId.ac, &acc_rec) > 0);
	THROW_PP(acc_rec.CurID == curID, PPERR_INCOMPACCWITHCUR);
	THROW(ValidateAccKind(kind = acc_rec.Kind));
	acct.ac = acc_rec.A.Ac;
	acct.sb = acc_rec.A.Sb;
	p->Low  = acc_rec.Overdraft;
	p->Upp  = acc_rec.Limit;
	//
	// Проверяем существование статьи
	//
	if(rAcctId.ar) {
		THROW(Art.Search(rAcctId.ar) > 0);
		acct.ar = Art.data.Article;
	}
	else
		acct.ar = 0;
	SetupAccTurnParam(p, side, kind);
	//
	// Ищем ссылку на соответствие {счет, статья} в AcctRel
	// и если ссылки нет, то открываем новую
	//
	THROW(r = AcctIDToRel(&rAcctId, pAccRelID));
	if(r > 0) {
		THROW_PP(AccRel.data.Closed == 0, PPERR_ACCTCLOSED);
	}
	else {
		// @v10.1.12 @debug {
		/*
		AcctRelTbl::Rec ex_rec;
		THROW(r = AccRel.SearchNum(0, &acct, curID, &ex_rec));
		if(r > 0) {
		}
		*/
		// } @v10.1.12 
		THROW(AccRel.OpenAcct(pAccRelID, &acct, curID, &rAcctId, kind));
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::SetupAccTurnParam(AccTurnParam * p, int side, int kind)
{
	double overdraft = p->Low, limit = p->Upp;
	//
	// Уточняем знак суммы и предельные остатки
	//
	p->Side = side;
	if(side == PPCREDIT)
		p->Amt = -p->Amt;
	//
	// Теперь сумму p->amt можно смело прибавлять к оборотам
	// и остаткам. Однако, номинальная сумма проводки должна быть
	// приведена к своему изначальному виду такой же операцией :
	// { if(side == PPCREDIT) { p->amt = -p->amt; } }
	//
	if(kind == ACT_PASSIVE) {
		p->Low = limit     ? -limit : -SMathConst::Max;
		p->Upp = overdraft ? overdraft : 0.0;
	}
	else if(kind == ACT_ACTIVE) {
		p->Low = overdraft ? -overdraft : 0.0;
		p->Upp = limit     ? limit : SMathConst::Max;
	}
	else {
		p->Low = overdraft ? -overdraft : -SMathConst::Max;
		p->Upp = limit     ? limit : SMathConst::Max;
	}
	return 1;
}
//
//
//
int SLAPI AccTurnCore::_RollbackTurn(int side, LDATE date, long oprNo, PPID bal, PPID rel, double amt)
{
	int    ok = 1;
	int    kind;
	PPAccount acc_rec;
	AccTurnParam p;
	p.Amt = R2(amt);
	THROW(AccObj.Search(bal, &acc_rec) > 0);
	THROW(ValidateAccKind(kind = acc_rec.Kind));
	p.Low = acc_rec.Overdraft;
	p.Upp = acc_rec.Limit;
	SetupAccTurnParam(&p, side,  kind);
	THROW(LockFRR(rel, date));
	THROW(BalTurn.RollbackTurn(bal, date, &p, 0));
	THROW(_UpdateForward(date, oprNo, rel, p));
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::_UpdateTurn(PPID billID, short rByBill, double newAmt, double cRate, int use_ta)
{
	int    ok = 1;
	const  int do_remove = BIN(newAmt == 0.0);
	int    zero_crd_acc = 0;
	AccTurnTbl::Key0 k0;
	LDATE  date;
	PPID   dbt_acc_id, crd_acc_id;
	PPID   dbt_rel_id, crd_rel_id;
	long   dbt_oprno,  crd_oprno;
	double amt;  // Оригинальная сумма проводки
	double _amt; // Сумма, на которую изменяются форвардные остатки и балансы
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		k0.Bill    = billID;
		k0.RByBill = rByBill;
		k0.Reverse = 0;
		if(!search(0, &k0, spEq)) {
			CALLEXCEPT_PP((BTROKORNFOUND) ? PPERR_TURNBYBILLNFOUND : PPERR_DBENGINE);
		}
		date = data.Dt;
		amt  = MONEYTOLDBL(data.Amount);
		_amt = newAmt - amt;
		dbt_rel_id = data.Acc;
		crd_rel_id = data.CorrAcc;
		dbt_acc_id = data.Bal;
		dbt_oprno  = data.OprNo;

		if(crd_rel_id == 0) {
			PPAccount acc_rec;
			THROW(AccObj.Search(dbt_acc_id, &acc_rec) > 0);
			if(acc_rec.Type != ACY_BAL && crd_rel_id == 0)
				zero_crd_acc = 1;
		}
		if(do_remove) {
			THROW(rereadForUpdate(0, 0));
			THROW_DB(deleteRec()); // @sfu
		}
		else if(_amt != 0.0) {
			THROW(rereadForUpdate(0, 0));
			LDBLTOMONEY(newAmt, data.Amount);
			LDBLTOMONEY(MONEYTOLDBL(data.Rest) + _amt, data.Rest);
			THROW_DB(updateRec()); // @sfu
		}
		if(!zero_crd_acc) {
			k0.Bill    = billID;
			k0.RByBill = rByBill;
			k0.Reverse = 1;
			if(!search(0, &k0, spEq)) {
				CALLEXCEPT_PP((BTROKORNFOUND) ? PPERR_TURNBYBILLNFOUND : PPERR_DBENGINE);
			}
			THROW_PP(data.Acc == crd_rel_id && data.CorrAcc == dbt_rel_id &&
				dbl_cmp(MONEYTOLDBL(data.Amount), amt) == 0 && data.Dt == date, PPERR_TURNMIRRORINGFAULT);
			crd_acc_id = data.Bal;
			crd_oprno  = data.OprNo;
			if(do_remove) {
				THROW(rereadForUpdate(0, 0));
				THROW_DB(deleteRec()); // @sfu
			}
			else if(_amt != 0.0) {
				THROW(rereadForUpdate(0, 0));
				LDBLTOMONEY(newAmt, data.Amount);
				LDBLTOMONEY(MONEYTOLDBL(data.Rest) - _amt, data.Rest);
				THROW_DB(updateRec()); // @sfu
			}
		}
		THROW(_RollbackTurn(PPDEBIT, date, dbt_oprno, dbt_acc_id, dbt_rel_id, _amt));
		if(!zero_crd_acc)
			THROW(_RollbackTurn(PPCREDIT, date, crd_oprno, crd_acc_id, crd_rel_id, _amt));
		if(rByBill <= BASE_RBB_BIAS) {
			k0.Bill    = billID;
			k0.RByBill = rByBill+BASE_RBB_BIAS;
			k0.Reverse = 0;
			if(search(0, &k0, spEq)) {
				double base_amt = R2(newAmt * cRate);
				THROW(_UpdateTurn(billID, rByBill+BASE_RBB_BIAS, base_amt, 0, 0)); // @recursion
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::RollbackTurn(PPID billID, short rByBill, int use_ta)
{
	if(rByBill > BASE_RBB_BIAS)
		return PPSetError(PPERR_UPDBASEACCTURN);
	else
		return _UpdateTurn(billID, rByBill, 0.0, 0.0, use_ta);
}

int SLAPI AccTurnCore::UpdateAmount(PPID billID, short rByBill, double newAmt, double cRate, int use_ta)
{
	if(rByBill > BASE_RBB_BIAS)
		return PPSetError(PPERR_UPDBASEACCTURN);
	else
		return _UpdateTurn(billID, rByBill, newAmt, cRate, use_ta);
}
//
//
//
int SLAPI AccTurnCore::Turn(PPAccTurn * pAturn, int use_ta)
{
	int    ok = 1;
	const  int zero_crd_acc = BIN((pAturn->Flags & (PPAF_REGISTER|PPAF_OUTBAL)) && !(pAturn->Flags & PPAF_OUTBAL_TRANSFER));
	PPID   dbt_rel = 0, crd_rel = 0;
	AccTurnParam dbt_param, crd_param;
	THROW_PP(pAturn->Amount != 0.0, PPERR_INVTURNAMOUNT);
	dbt_param.Amt = crd_param.Amt = R2(pAturn->Amount);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pAturn->RByBill < BASE_RBB_BIAS) {
			THROW(GetBill(pAturn));
		}
		THROW(_ProcessAcct(PPDEBIT,  pAturn->CurID, pAturn->DbtID, &dbt_rel, &dbt_param));
		if(!zero_crd_acc) {
			THROW(_ProcessAcct(PPCREDIT, pAturn->CurID, pAturn->CrdID, &crd_rel, &crd_param));
		}
		else
			crd_rel = 0;
		THROW(LockFRR(dbt_rel, pAturn->Date));
		if(!zero_crd_acc) {
			THROW(LockFRR(crd_rel, pAturn->Date));
		}
		THROW(BalTurn.Turn(pAturn->DbtID.ac, pAturn->Date, &dbt_param, 0));
		if(!zero_crd_acc) {
			THROW(BalTurn.Turn(pAturn->CrdID.ac, pAturn->Date, &crd_param, 0));
		}
		THROW(_Turn(pAturn, dbt_rel, crd_rel, dbt_param));
		if(!zero_crd_acc) {
			THROW(_Turn(pAturn, crd_rel, dbt_rel, crd_param));
		}
		if(pAturn->CurID) {
			PPAccTurn base_aturn = *pAturn;
			base_aturn.RByBill  += BASE_RBB_BIAS;
			base_aturn.CurID  = 0;
			base_aturn.Amount = R2(pAturn->Amount * pAturn->CRate);
			THROW(GetBaseAcctID(pAturn->DbtID, &base_aturn.DbtID));
			if(!zero_crd_acc) {
				THROW(GetBaseAcctID(pAturn->CrdID, &base_aturn.CrdID));
			}
			THROW(Turn(&base_aturn, 0)); // @recursion
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::CalcRest(int aco, PPID accID, const DateRange * pPeriod, double * pInRest, double * pOutRest)
{
	int    ok = 1;
	DateRange period = *pPeriod;
	SETIFZ(period.upp, MAXDATE);
	if(aco == ACO_3) {
		if(pInRest)
			THROW(GetAcctRest(period.low, accID, pInRest, 1));
		if(pOutRest)
			THROW(GetAcctRest(period.upp, accID, pOutRest, 0));
	}
	else {
		double dbt, crd;
		if(pInRest) {
			THROW(GetBalRest(period.low, accID, &dbt, &crd, BALRESTF_INCOMING));
			dbt -= crd;
			*pInRest = dbt;
		}
		if(pOutRest) {
			THROW(GetBalRest(period.upp, accID, &dbt, &crd, 0));
			dbt -= crd;
			*pOutRest = dbt;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::CalcComplexRestOnGenList(ObjRestrictArray * pGenList, PPID curID, const DateRange * pRange, AmtList * pInRest, AmtList * pOutRest)
{
	ObjRestrictItem * p_item;
	for(uint i = 0; pGenList->enumItems(&i, (void **)&p_item);)
		if(!CalcComplexRest(GetAcoByGenFlags(p_item->Flags), p_item->ObjID, curID, 0, pRange, pInRest, pOutRest))
			return 0;
	return 1;
}

int SLAPI AccTurnCore::CalcComplexRest(long aco, PPID accID, PPID curID, PPID personRelID, const DateRange * pRange, AmtList * pInRest, AmtList * pOutRest)
{
	int    ok = 1;
	const  int mult = (aco < 0) ? -1 : 1;
	uint   i;
	PPID   cur_id = 0;
	DateRange period = *pRange;
	PPIDArray acc_list;
	double inrest = 0.0;
	double outrest = 0.0;
	double * p_inrest  = pInRest  ? &inrest  : 0;
	double * p_outrest = pOutRest ? &outrest : 0;
	aco = abs(aco);
	THROW(IdentifyAcc(&aco, &accID, curID, personRelID, &acc_list));
	SETIFZ(period.upp, MAXDATE);
	if(acc_list.getCount() == 0) {
		if(curID < 0) {
			THROW(GetAcctCurID(aco, accID, &cur_id));
		}
		else
			cur_id = curID;
		THROW(CalcRest(aco, accID, &period, p_inrest, p_outrest));
		CALLPTRMEMB(pInRest, Add(0, cur_id, inrest * mult));
		CALLPTRMEMB(pOutRest, Add(0, cur_id, outrest * mult));
	}
	else {
		for(i = 0; i < acc_list.getCount(); i++) {
			const PPID acc_id = acc_list.at(i);
			if(curID < 0) {
				THROW(GetAcctCurID(aco, acc_id, &cur_id));
			}
			else
				cur_id = curID;
			THROW(CalcRest(aco, acc_id, &period, p_inrest, p_outrest));
			CALLPTRMEMB(pInRest, Add(0, cur_id, inrest * mult));
			CALLPTRMEMB(pOutRest, Add(0, cur_id, outrest * mult));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI AccTurnCore::ReplaceArticle(PPID dest, PPID src)
{
	int    ok = 1;
	PPID   dest_rel_id, src_rel_id;
	DBRowId pos, p;
	AcctRelTbl::Key2 k_;
	AccTurnTbl::Key1 atk;
	AccTurnTbl::Key2 atk2;
	AcctRelTbl::Rec dest_rel_rec;
	for(PPID acc_id = 0; AccRel.EnumByArticle(dest, &acc_id, &dest_rel_rec) > 0;) {
		int    maybe_zero_crd_acc = 0;
		PPAccount acc_rec;
		THROW(AccObj.Search(dest_rel_rec.AccID, &acc_rec) > 0);
		if(acc_rec.Type != ACY_BAL)
			maybe_zero_crd_acc = 1;
		dest_rel_id = dest_rel_rec.ID;
		THROW_DB(AccRel.getPosition(&pos));
		k_.ArticleID = src;
		k_.AccID = acc_id;
		if(AccRel.search(2, &k_, spEq)) {
			src_rel_id = AccRel.data.ID;
			atk.Acc = dest_rel_id;
			atk.Dt = ZERODATE;
			atk.OprNo = 0;
			while(search(1, &atk, spGt) && atk.Acc == dest_rel_id) { // @v8.2.0 searchForUpdate-->search
				int    sp;
				THROW_DB(getPosition(&p));
				THROW_DB(getDirectForUpdate(-1, 0, p)); // @v8.2.0
				data.Acc = src_rel_id;
				THROW_DB(updateRec()); // @sfu
				THROW_DB(getDirect(2, &atk2, p));
				sp = data.Reverse ? spPrev : spNext;
				if(search(&atk2, sp) && data.CorrAcc == dest_rel_id) { // @v8.2.0 searchForUpdate-->search
					THROW_DB(rereadForUpdate(2, &atk2)); // @v8.2.0
					data.CorrAcc = src_rel_id;
					THROW_DB(updateRec()); // @sfu
				}
				else
					THROW_PP(sp == spNext && maybe_zero_crd_acc, PPERR_TURNMIRRORINGFAULT);
			}
			THROW_DB(AccRel.getDirectForUpdate(-1, 0, pos)); // @v8.2.0 getDirect-->getDirectForUpdate
			THROW_DB(AccRel.deleteRec()); // @sfu
			THROW(RecalcRest(src_rel_id, ZERODATE, 0, 0, 0));
		}
		else {
			THROW_DB(BTRNFOUND);
			THROW(Art.Search(src) > 0);
			THROW_DB(updateFor(&AccRel, 0, (AccRel.ID == dest_rel_rec.ID),
				set(AccRel.ArticleID, dbconst(src)).set(AccRel.Ar, dbconst(Art.data.Article))));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::SearchAccRef(PPID _id, int removeUnusedRel)
{
	int    ok = -1;
	AcctRelTbl::Rec rel_rec;
	for(SEnum en = AccRel.EnumByAcc(_id); en.Next(&rel_rec) > 0;) {
		if(!rel_rec.Closed) {
			AccTurnTbl::Key1 atk;
			MEMSZERO(atk);
			atk.Acc = rel_rec.ID;
			if(search(1, &atk, spGt) && atk.Acc == rel_rec.ID)
				ok = 1;
			else {
				THROW_DB(BTROKORNFOUND);
				if(removeUnusedRel)
					THROW_DB(deleteFrom(&AccRel, 0, AccRel.ID == rel_rec.ID));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::SearchArticleRef(PPID _id, int removeUnusedRel, PPID * pBillID)
{
	int    ok = -1;
	PPID   bill_id = 0;
	AcctRelTbl::Rec rel_rec;
	for(SEnum en = AccRel.EnumByArticle(_id); en.Next(&rel_rec) > 0;) {
		if(!rel_rec.Closed) {
			AccTurnTbl::Key1 atk;
			MEMSZERO(atk);
			atk.Acc = rel_rec.ID;
			if(search(1, &atk, spGt) && atk.Acc == rel_rec.ID) {
				bill_id = data.Bill;
				ok = 1;
			}
			else {
				THROW_DB(BTROKORNFOUND);
				if(removeUnusedRel) {
					THROW_DB(deleteFrom(&AccRel, 0, AccRel.ID == rel_rec.ID));
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pBillID, bill_id);
	return ok;
}
//
//
//
int SLAPI AccTurnCore::Repair(long flags, int (*MsgProc)(int msgCode, PPID accID, PPID billID, LDATE dt, long oprno, void * paramPtr), void * paramPtr)
{
	int    ok = 1, r;
	{
		PPTransaction tra(1);
		THROW(tra);
		THROW(r = Helper_Repair(flags, 0, MsgProc, paramPtr));
		if(r > 0)
			THROW(r = Helper_Repair(flags, 1, MsgProc, paramPtr));
		ok = r;
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::Helper_Repair(long flags, int reverse, int (*MsgProc)(int msgCode, PPID accID,
	PPID billID, LDATE dt, long oprno, void * paramPtr), void * paramPtr)
{
	int    ok = -1, r;
	RECORDNUMBER numrecs = 0, count = 0;
	AccTurnTbl::Key2 k2;
	MEMSZERO(k2);
	getNumRecs(&numrecs);
	BExtQuery q(this, 2);
	q.selectAll().where(this->Reverse == (long)reverse);
	for(q.initIteration(0, &k2, spFirst); q.nextIteration() > 0;) {
		int    msg_code = 0;
		AccTurnTbl::Key0 k0;
		AccTurnTbl::Rec rec, mirror;
		copyBufTo(&rec);
		PPID dbt_rel_id = rec.Acc;
		PPID crd_rel_id = rec.CorrAcc;
		PPID dbt_acc_id = rec.Bal;
		int  zero_crd_acc = 0;
		if(crd_rel_id == 0) {
			PPAccount acc_rec;
			THROW(r = AccObj.Search(dbt_acc_id, &acc_rec));
			if(r > 0) {
				if(acc_rec.Type != ACY_BAL && crd_rel_id == 0)
					zero_crd_acc = 1;
			}
			else if(MsgProc(PPERR_EATURN_BL_2ACC, rec.Acc, rec.Bill, rec.Dt, rec.OprNo, paramPtr) > 0) {
				if(flags & ATRPRF_RMVZEROACCLINK)
					THROW(RollbackTurn(rec.Bill, rec.RByBill, 0));
				continue;
			}
			else
				break;
		}
		if(!zero_crd_acc) {
			DBRowId pos;
			k0.Bill    = rec.Bill;
			k0.RByBill = rec.RByBill;
			k0.Reverse = reverse ? 0 : 1;
			if(!search(0, &k0, spEq))
				msg_code = PPERR_EATURN_NOMIRROR;
			else {
				THROW_DB(getPosition(&pos));
				copyBufTo(&mirror);
				if(reverse == 0 && (mirror.Acc != crd_rel_id || mirror.CorrAcc != dbt_rel_id ||
					dbl_cmp(MONEYTOLDBL(mirror.Amount), MONEYTOLDBL(rec.Amount)) != 0 ||
					mirror.Dt != rec.Dt)) {
					msg_code = PPERR_EATURN_BADMIRROR;
				}
			}
			if(msg_code) {
				if(MsgProc(msg_code, rec.Acc, rec.Bill, rec.Dt, rec.OprNo, paramPtr) <= 0)
					break;
				if(flags & ATRPRF_REPAIRMIRROR) {
					if(msg_code == PPERR_EATURN_NOMIRROR) {
						mirror = rec;
						THROW(_OprNo(mirror.Dt, &mirror.OprNo));
						mirror.Acc = rec.CorrAcc;
						mirror.CorrAcc = rec.Acc;
						mirror.Reverse = reverse ? 0 : 1;
						THROW_DB(insertRecBuf(&mirror));
					}
					else if(msg_code == PPERR_EATURN_BADMIRROR) {
						mirror.Acc = rec.CorrAcc;
						mirror.CorrAcc = rec.Acc;
						mirror.Reverse = reverse ? 0 : 1;
						MONEYTOMONEY(rec.Amount, mirror.Amount);
						if(mirror.Dt != rec.Dt) {
							//
							// Индекс, содержащий поле Dt - не модифицируемый.
							// В связи с этим, если дата не верная, придется удалить запись
							// и вставить снова.
							//
							THROW_DB(getDirectForUpdate(-1, 0, pos));
							THROW_DB(deleteRec()); // @sfu
							mirror.Dt = rec.Dt;
							THROW(_OprNo(mirror.Dt, &mirror.OprNo));
							THROW_DB(insertRecBuf(&mirror));
						}
						else {
							THROW_DB(getDirectForUpdate(-1, 0, pos));
							THROW_DB(updateRecBuf(&mirror)); // @sfu
						}
					}
				}
			}
		}
		if(reverse == 0) {
			msg_code = 0;
			if(flags & ATRPRF_CHECKBILLLINK) {
				THROW(r = BillObj->Search(rec.Bill));
				if(r < 0) {
					msg_code = PPERR_EATURN_BL_2BILL;
					if(MsgProc(msg_code, rec.Acc, rec.Bill, rec.Dt, rec.OprNo, paramPtr) <= 0)
						break;
				}
			}
			if(flags & ATRPRF_CHECKACCLINK) {
				THROW(r = AccRel.Search(rec.Acc));
				if(r < 0) {
					msg_code = PPERR_EATURN_BL_2ACC;
					if(MsgProc(msg_code, rec.Acc, rec.Bill, rec.Dt, rec.OprNo, paramPtr) <= 0)
						break;
				}
			}
			if((msg_code == PPERR_EATURN_BL_2BILL && flags & ATRPRF_RMVZEROBILLLINK) ||
				(msg_code == PPERR_EATURN_BL_2ACC && flags & ATRPRF_RMVZEROACCLINK)) {
				THROW(RollbackTurn(rec.Bill, rec.RByBill, 0));
			}
		}
		count++;
		PPWaitPercent(count * (reverse + 1), numrecs * 2);
	}
	ok = 1;
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::RecalcRest(PPID accRelID, LDATE startDate,
	int (*MsgProc)(PPID, LDATE, long, double, void * paramPtr), void * paramPtr, int use_ta)
{
	int    ok = 1, correct = 0, reply;
	int    msg_done = 0;
	double r, amt, rest = 0.0;
	DBRowId pos;
	AccTurnTbl::Key1 k;
	k.Acc   = accRelID;
	k.Dt    = startDate;
	k.OprNo = 0;
	if(startDate) {
		THROW(GetAcctRest(startDate, accRelID, &rest, 1));
	}
	if(!correct) {
		BExtQuery q(this, 1, 64);
		q.select(this->Dt, this->OprNo, this->Amount, this->Rest, this->Reverse, 0L).
		where(this->Acc == accRelID && this->Dt >= startDate);
		for(q.initIteration(0, &k, spGt); !correct && q.nextIteration() > 0;) {
			amt = MONEYTOLDBL(data.Amount);
			r   = MONEYTOLDBL(data.Rest);
			if(data.Reverse == 0)
				rest += amt;
			else
				rest -= amt;
			if(R6(rest - r) != 0.0) {
				if(data.Reverse == 0)
					rest -= amt;
				else
					rest += amt;
				k.Acc = accRelID;
				k.Dt = data.Dt;
				k.OprNo = data.OprNo;
				correct = 1;
			}
		}
	}
	if(correct) {
		PPTransaction tra(use_ta);
		THROW(tra);
		if(search(1, &k, spGe) && k.Acc == accRelID)
			do {
				amt = MONEYTOLDBL(data.Amount);
				r   = MONEYTOLDBL(data.Rest);
				if(data.Reverse == 0)
					rest += amt;
				else
					rest -= amt;
				double delta = R6(rest - r);
				if(delta != 0.0) {
					if(MsgProc) {
						if(!msg_done) {
							THROW_DB(getPosition(&pos));
							reply = MsgProc(k.Acc, k.Dt, k.OprNo, delta, paramPtr);
							THROW_DB(getDirect(1, &k, pos));
							msg_done = 1;
						}
					}
					else
						reply = 1;
					if(reply > 0) {
						LDBLTOMONEY(rest, data.Rest);
						THROW_DB(updateRec());
					}
					else if(reply < 0)
						break;
				}
			} while(search(1, &k, spNext) && k.Acc == accRelID);
		THROW_DB(BTROKORNFOUND);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::UpdateAccNum(PPID accID, int newAc, int newSb, int use_ta)
{
	int    ok = 1;
	int    vadd = 0; // Признак того, что необходимо добавить искусственный счет 1-го порядка
	PPID   id;
	int16  old_ac, old_sb;
	PPAccountPacket acc_pack, add_acc_pack;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(AccObj.GetPacket(accID, &acc_pack) > 0);
		old_ac = acc_pack.Rec.A.Ac;
		old_sb = acc_pack.Rec.A.Sb;
		if(acc_pack.Rec.A.Sb == 0) {
			//
			// Если изменяемый счет был одновременно счетом первого порядка, то придется искусственно добавить
			// вместо него новый счет первого порядка, дабы субсчета этого счета не подвисли.
			//
			add_acc_pack = acc_pack;
			add_acc_pack.Rec.ID = 0L;
			add_acc_pack.Rec.A.Sb = 0;
			add_acc_pack.Rec.OpenDate = LConfig.OperDate;
			add_acc_pack.Rec.Type     = acc_pack.Rec.Type;
			vadd = 1;
		}
		acc_pack.Rec.A.Ac = newAc;
		acc_pack.Rec.A.Sb = newSb;
		THROW(AccObj.PutPacket(&accID, &acc_pack, 0));
		if(vadd && AccObj.SearchNum(add_acc_pack.Rec.A.Ac, add_acc_pack.Rec.A.Sb, 0, 0) < 0) { // @v9.2.11 Искусственно создаем счет только в случае, если не было других дубликатов
			THROW(AccObj.PutPacket(&(id = 0), &add_acc_pack, 0));
		}
		//
		// Теперь необходимо изменить все ссылки AcctRel.Ac и AcctRel.Sb на этот счет.
		//
		THROW(AccRel.ReplaceAcct(old_ac, old_sb, newAc, newSb));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

SLAPI RecoverBalanceParam::RecoverBalanceParam() : BalAccID(0), Flags(0)
{
	Period.Z();
}

int SLAPI AccTurnCore::RecalcBalance(const RecoverBalanceParam * pParam, PPLogger & rLogger)
{
	int    ok = 1;
	PPID   bal_id = pParam->BalAccID;
	if(bal_id) {
		THROW(_RecalcBalance(bal_id, pParam, rLogger));
	}
	else {
		PPIDArray bal_list;
		PPAccount rec;
		for(SEnum en = AccObj.Enum(0); en.Next(&rec) > 0;) {
			THROW_SL(bal_list.add(rec.ID));
		}
		bal_list.sortAndUndup();
		for(uint i = 0; i < bal_list.getCount(); i++) {
			bal_id = bal_list.get(i);
			THROW(_RecalcBalance(bal_id, pParam, rLogger));
		}
	}
	CATCH
		rLogger.LogLastError();
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI AccTurnCore::_CheckBalance(PPID accID, LDATE dt, double dbt, double crd,
	char * pAccStr, int correct, PPLogger & rLogger, int use_ta)
{
	int    err = 0;
	SString fmt_buf, log_msg;
	char   sdate[32], sdbt[32], sbdbt[32], scrd[32];
	BalanceTbl::Key1 bk;
	bk.AccID = accID;
	bk.Dt  = dt;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(BalTurn.search(1, &bk, spEq)) {
			if(dbl_cmp(MONEYTOLDBL(BalTurn.data.DbtRest), dbt) != 0)
				err = 1; // Debit error !!!
			if(dbl_cmp(MONEYTOLDBL(BalTurn.data.CrdRest), crd) != 0)
				err = 2; // Credit error !!!
			if(err) {
				PPLoadString(PPMSG_ERROR, ((err == 1) ? PPERR_BALCORRUPT_DBT : PPERR_BALCORRUPT_CRD), fmt_buf);
				datefmt(&dt, DATF_DMY, sdate);
				if(err == 1) {
					realfmt(MONEYTOLDBL(BalTurn.data.DbtRest), MKSFMTD(0, 2, NMBF_TRICOMMA), sbdbt);
					realfmt(dbt, MKSFMTD(0, 2, NMBF_TRICOMMA), sdbt);
				}
				else {
					realfmt(MONEYTOLDBL(BalTurn.data.CrdRest), MKSFMTD(0, 2, NMBF_TRICOMMA), sbdbt);
					realfmt(crd, MKSFMTD(0, 2, NMBF_TRICOMMA), sdbt);
				}
				rLogger.Log(log_msg.Printf(fmt_buf, sdate, sbdbt, sdbt, pAccStr));
				if(correct) {
					LDBLTOMONEY(dbt, BalTurn.data.DbtRest);
					LDBLTOMONEY(crd, BalTurn.data.CrdRest);
					THROW_DB(BalTurn.updateRec());
				}
			}
		}
		else if(BTRNFOUND) {
			err = 3; // Balance for date not found !!!
			PPLoadString(PPMSG_ERROR, PPERR_BALCORRUPT_ABS, fmt_buf);
			datefmt(&dt, DATF_DMY, sdate);
			realfmt(dbt, MKSFMTD(0, 2, NMBF_TRICOMMA), sdbt);
			realfmt(crd, MKSFMTD(0, 2, NMBF_TRICOMMA), scrd);
			rLogger.Log(log_msg.Printf(fmt_buf, sdate, pAccStr, sdbt, scrd));
			if(correct) {
				BalTurn.data.Dt = dt;
				BalTurn.data.AccID = accID;
				LDBLTOMONEY(dbt, BalTurn.data.DbtRest);
				LDBLTOMONEY(crd, BalTurn.data.CrdRest);
				THROW_DB(BalTurn.insertRec());
			}
		}
		else {
			CALLEXCEPT_PP(PPERR_DBENGINE);
		}
		THROW(tra.Commit());
	}
	CATCH
		return 0;
	ENDCATCH
	return (err > 0) ? err : -1;
}

int SLAPI AccTurnCore::RecalcBalance(PPID accID, LDATE startDate, int use_ta)
{
	int    ok = 1;
	double dbt = 0.0, crd = 0.0;
	LDATE  prev;
	PPLogger logger(PPLogger::fDisableOutput);
	BExtQuery q(this, 3, 64);
	AccTurnTbl::Key3 k;
	BalanceTbl::Key1 bk;
	k.Bal   = accID;
	k.Dt    = startDate;
	k.OprNo = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(startDate)
			THROW(GetBalRest(startDate, accID, &dbt, &crd, BALRESTF_INCOMING));
		prev = ZERODATE;
		q.select(this->Dt, this->Reverse, this->Amount, 0L).where(this->Bal == accID && this->Dt >= startDate);
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
			if(data.Dt != prev) {
				if(prev)
					THROW(_CheckBalance(accID, prev, dbt, crd, 0, 1, logger, 0));
				prev = data.Dt;
			}
			if(data.Reverse)
				crd = R2(crd + MONEYTOLDBL(data.Amount));
			else
				dbt = R2(dbt + MONEYTOLDBL(data.Amount));
		}
		if(prev)
			THROW(_CheckBalance(accID, prev, dbt, crd, 0, 1, logger, 0));
		//
		// Проверка на отсутствие записей баланса за дни, в которые не было
		// проводок по этому балансовому счету
		//
		bk.AccID = accID;
		bk.Dt  = startDate;
		while(BalTurn.search(1, &bk, spGt) && bk.AccID == accID) {
			k.Bal   = accID;
			k.Dt    = bk.Dt;
			k.OprNo = 0;
			if(!search(3, &k, spGe) || k.Bal != accID || k.Dt != bk.Dt)
				THROW_DB(BalTurn.deleteRec());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::_RecalcBalance(PPID balID, const RecoverBalanceParam * pParam, PPLogger & rLogger)
{
	int    ok = 1;
	int    err, errflag = 0;
	int    correct_flag = BIN(pParam->Flags & RecoverBalanceParam::fCorrect);
	SString fmt_buf, log_msg;
	char   sdate[32], sdbt[32], scrd[32], sbal[32];
	double dbt = 0.0, crd = 0.0;
	Acct   acct;
	PPAccount acc_rec;
	AccTurnTbl::Key3 k;
	BalanceTbl::Key1 bk;
	k.Bal   = balID;
	k.Dt    = pParam->Period.low;
	k.OprNo = 0;
	THROW(AccObj.Search(balID, &acc_rec) > 0);
	acct.ac = acc_rec.A.Ac;
	acct.sb = acc_rec.A.Sb;
	acct.ar = 0;
	acct.ToStr(ACCF_BAL | ACCF_DELDOT, sbal);
	if(pParam->Period.low) {
		THROW(GetBalRest(pParam->Period.low, balID, &dbt, &crd, BALRESTF_INCOMING));
	}
	{
		LDATE  prev = ZERODATE;
		BExtQuery q(this, 3, 64);
		q.select(this->Dt, this->Reverse, this->Amount, 0L).
			where(this->Bal == balID && daterange(this->Dt, &pParam->Period));
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
			if(data.Dt != prev) {
				if(prev) {
					THROW(err = _CheckBalance(balID, prev, dbt, crd, sbal, correct_flag, rLogger, 1));
					if(err < 0)
						err = 0;
					else
						errflag = 1;
				}
				PPWaitMsg((log_msg = sbal).CatCharN(' ', 2).Cat(data.Dt));
				prev = data.Dt;
			}
			if(data.Reverse)
				crd = R2(crd + MONEYTOLDBL(data.Amount));
			else
				dbt = R2(dbt + MONEYTOLDBL(data.Amount));
		}
		if(prev) {
			THROW(err = _CheckBalance(balID, prev, dbt, crd, sbal, correct_flag, rLogger, 1));
			if(err < 0)
				err = 0;
			else
				errflag = 1;
		}
	}
	//
	// Проверка на отсутствие записей баланса за дни, в которые не было
	// проводок по этому балансовому счету
	//
	{
		PPTransaction tra(1);
		THROW(tra);
		bk.AccID = balID;
		bk.Dt  = pParam->Period.low;
		while(BalTurn.search(1, &bk, spGt) && bk.AccID == balID && pParam->Period.CheckDate(bk.Dt)) {
			k.Bal   = balID;
			k.Dt    = bk.Dt;
			k.OprNo = 0;
			if(!search(3, &k, spGe) || k.Bal != balID || k.Dt != bk.Dt) {
				PPLoadString(PPMSG_ERROR, PPERR_BALCORRUPT_EXREC, fmt_buf);
				datefmt(&bk.Dt, DATF_DMY, sdate);
				realfmt(MONEYTOLDBL(BalTurn.data.DbtRest), MKSFMTD(0, 2, NMBF_TRICOMMA), sdbt);
				realfmt(MONEYTOLDBL(BalTurn.data.CrdRest), MKSFMTD(0, 2, NMBF_TRICOMMA), scrd);
				rLogger.Log(log_msg.Printf(fmt_buf, sdate, sdbt, scrd, sbal));
				if(correct_flag)
					THROW_DB(BalTurn.deleteRec());
			}
			PPWaitMsg((log_msg = sbal).CatChar('^').Space().Cat(bk.Dt));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	if(!errflag) {
		PPLoadString(PPMSG_ERROR, PPERR_BALCORRUPT_OK, fmt_buf);
		rLogger.Log(log_msg.Printf(fmt_buf, sbal));
	}
	return ok;
}
//
// Поддержка функции валютной переоценки
//
SLAPI CurRevalParam::CurRevalParam()
{
	Init();
}

void SLAPI CurRevalParam::Init()
{
	Dt = ZERODATE;
	MEMSZERO(CorrAcc);
	MEMSZERO(NegCorrAcc);
	LocID = LConfig.Location;
	Flags = 0;
}

CurRevalParam & FASTCALL CurRevalParam::operator = (const CurRevalParam & src)
{
	Dt = src.Dt;
	CorrAcc = src.CorrAcc;
	NegCorrAcc = src.NegCorrAcc;
	LocID = src.LocID;
	Flags = src.Flags;
	AccList.copy(src.AccList);
	CRateList.copy(src.CRateList);
	return *this;
}

int SLAPI AccTurnCore::RevalCurRest(const CurRevalParam & rParam, const Acct * pAcc, const PPIDArray * pCurList, int use_ta)
{
	int    ok = 1;
	uint   i;
	AcctID base_acc_id, cur_acc_id;
	PPID   base_acc_rel_id;
	PPID   cur_acc_rel_id;
	double base_rest, cur_rest, new_base_rest = 0.0;
	{
		PPObjBill * p_bobj = BillObj;
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ConvertAcct(pAcc, 0L, &base_acc_id, 0));
		THROW(AcctIDToRel(&base_acc_id, &base_acc_rel_id));
		THROW(GetAcctRest(rParam.Dt, base_acc_rel_id, &base_rest, 0));
		for(i = 0; i < pCurList->getCount(); i++) {
			PPID   cur_id = pCurList->at(i);
			double crate = rParam.CRateList.Get(PPAMT_CRATE, cur_id);
			THROW(ConvertAcct(pAcc, cur_id, &cur_acc_id, 0));
			THROW(AcctIDToRel(&cur_acc_id, &cur_acc_rel_id));
			THROW(GetAcctRest(rParam.Dt, cur_acc_rel_id, &cur_rest, 0));
			new_base_rest += R2(cur_rest * crate);
		}
		if(base_rest != new_base_rest) {
			PPBillPacket pack;
			long   flags = 0;
			PPAccTurn * p_at = 0;
			THROW(p_bobj->atobj->CreateBlankAccTurn(PPOPK_GENERICACCTURN, &pack, &flags, 0));
			pack.Rec.LocID = rParam.LocID;
			p_at = & pack.Turns.at(0);
			pack.Rec.CurID = p_at->CurID = 0L;
			pack.Rec.Dt    = p_at->Date  = rParam.Dt;
			if(base_rest > new_base_rest) {
				const Acct * p_acct = rParam.NegCorrAcc.ac ? &rParam.NegCorrAcc : &rParam.CorrAcc;
				THROW(ConvertAcct(p_acct, 0L, &p_at->DbtID, &p_at->DbtSheet));
				p_at->CrdID    = base_acc_id;
				p_at->Amount   = base_rest - new_base_rest;
			}
			else {
				THROW(ConvertAcct(&rParam.CorrAcc, 0L, &p_at->CrdID, &p_at->CrdSheet));
				p_at->DbtID    = base_acc_id;
				p_at->Amount   = new_base_rest - base_rest;
			}
			pack.Rec.Amount = BR2(p_at->Amount);
			pack.Rec.CRate  = 0;
			THROW(p_bobj->TurnPacket(&pack, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI AccTurnCore::RevalCurRests(const CurRevalParam * pParam)
{
	int    ok = 1;
	PPIDArray sub_list;
	PPIDArray cur_list;
	PPAccount acc_rec;
	{
		PPTransaction tra(1);
		THROW(tra);
		for(uint i = 0; i < pParam->AccList.getCount(); i++) {
			if(AccObj.Search(pParam->AccList.at(i), &acc_rec) > 0) {
				AcctRelTbl::Key3 k;
				Acct prev_acct;
				int    ac = acc_rec.A.Ac;
				int    sb = acc_rec.A.Sb;
				cur_list.clear();
				sub_list.clear();
				if(sb == 0)
					sb = -1;
				prev_acct.Clear();
				MEMSZERO(k);
				k.Ac = ac;
				k.Sb = sb ? sb : 0;
				while(AccRel.search(3, &k, spGt) && k.Ac == ac && (!sb || k.Sb == sb)) {
					if(AccRel.data.CurID && !AccRel.data.Closed && Art.Search(AccRel.data.ArticleID) > 0) {
						Acct acct = *(Acct*)&k;
						if(acct.ac != prev_acct.ac || acct.sb != prev_acct.sb || acct.ar != prev_acct.ar) {
							if(prev_acct.ac != 0)
								THROW(RevalCurRest(*pParam, &prev_acct, &cur_list, 0));
							prev_acct = acct;
							cur_list.freeAll();
						}
						cur_list.add(AccRel.data.CurID);
					}
				}
				if(prev_acct.ac != 0)
					THROW(RevalCurRest(*pParam, &prev_acct, &cur_list, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
