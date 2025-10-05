// CSESS.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2010, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2024, 2025
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
// CSessionCore
//
CSessionCore::CSessionCore() : CSessionTbl()
{
}

int CSessionCore::Search(PPID id, CSessionTbl::Rec * pRec) { return SearchByID(this, PPOBJ_CSESSION, id, pRec); }
int CSessionCore::SetSessIncompletness(PPID id, int grade, int use_ta) { return updateFor(this, use_ta, (this->ID == id), set(this->Incomplete, dbconst((long)grade))) ? 1 : PPSetErrorDB(); }
int CSessionCore::ResetTempSessTag(PPID id, int use_ta) { return updateFor(this, use_ta, (this->ID == id), set(this->Temporary, dbconst(0L))) ? 1 : PPSetErrorDB(); }

int CSessionCore::SearchByNumber(PPID * pID, PPID cashNodeID, long cashN, long sessN, LDATE dt)
{
	CSessionTbl::Key2 k;
	k.CashNodeID = cashNodeID;
	k.CashNumber = cashN;
	k.SessNumber = sessN;
	k.Dt = MAXDATE;
	while(search(2, &k, spLt) && k.CashNodeID == cashNodeID && k.CashNumber == cashN && k.SessNumber == sessN) {
		if(dt == 0 || labs(diffdate(&k.Dt, &dt, 0)) < 7) {
			ASSIGN_PTR(pID, data.ID);
			return 1;
		}
	}
	ASSIGN_PTR(pID, 0);
	return -1;
}

int CSessionCore::SearchLast(PPID cashNodeID, int incompl, PPID * pID, CSessionTbl::Rec * pRec)
{
	int    ok = -1;
	PPObjCashNode cn_obj;
	PPCashNode cn_rec;
	if(cn_obj.Search(cashNodeID, &cn_rec) > 0) {
		const int do_skip_current = BIN(incompl >- 1000);
		if(incompl > 1000)
			incompl = incompl % 1000;
		CSessionTbl::Key1 k1;
		MEMSZERO(k1);
		k1.CashNodeID = cashNodeID;
		k1.Dt = MAXDATE;
		k1.Tm = MAXTIME;
		if(search(1, &k1, spLt) && data.CashNodeID == cashNodeID) {
			do {
				if(data.Incomplete <= incompl && checkdate(data.Dt) && checktime(data.Tm)) {
					if(!do_skip_current || !(cn_rec.Flags & CASHF_SYNC) || data.ID != cn_rec.CurSessID) {
						ASSIGN_PTR(pID, data.ID);
						ASSIGN_PTR(pRec, data);
						ok = 1;
					}
				}
			} while(ok < 0 && search(1, &k1, spPrev) && data.CashNodeID == cashNodeID);
		}
	}
	else
		ok = 0;
	return ok;
}

int CSessionCore::HasChildren(PPID sessID)
{
	int    ok = -1;
	CSessionTbl::Key3 k;
	MEMSZERO(k);
	k.SuperSessID = sessID;
	if(search(3, &k, spGt) && k.SuperSessID == sessID)
		ok = 1;
	return (BTROKORNFOUND) ? ok : PPSetErrorDB();
}

int CSessionCore::GetSubSessList(PPID superSessID, PPIDArray * pList)
{
	int    ok = -1;
	CSessionTbl::Key3 k;
	MEMSZERO(k);
	k.SuperSessID = superSessID;
	while(search(3, &k, spGt) && k.SuperSessID == superSessID)
		if(pList->addUnique(data.ID) > 0)
			ok = 1;
	return (BTROKORNFOUND) ? ok : PPSetErrorDB();
}

int CSessionCore::GetLastNumber(PPID cashNodeID, long cashN, long * pNumber, CSessionTbl::Rec * pRec)
{
	int    ok = 1;
	CSessionTbl::Key2 k;
	k.CashNodeID = cashNodeID;
	k.CashNumber = cashN;
	k.SessNumber = MAXLONG;
	k.Dt = MAXDATE;
	if(search(2, &k, spLt) && k.CashNodeID == cashNodeID && k.CashNumber == cashN) {
		ASSIGN_PTR(pRec, data);
		ASSIGN_PTR(pNumber, k.SessNumber);
	}
	else {
		ASSIGN_PTR(pNumber, 0);
		ok = -1;
	}
	return ok;
}

int CSessionCore::SetSessDateTime(PPID sessID, const LDATETIME & rDtm, int use_ta)
{
	int    ok = 1;
	CSessionTbl::Key0 k0;
	CSessionTbl::Rec rec;
	LDATETIME dtm = rDtm;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		k0.ID = sessID;
		THROW(SearchByKey(this, 0, &k0, &rec) > 0);
		if(rec.Dt == dtm.d && rec.Tm == dtm.t) {
			ok = -1;
		}
		else {
			rec.Dt = dtm.d;
			rec.Tm = dtm.t;
			THROW_DB(rereadForUpdate(0, &k0));
			THROW_DB(updateRecBuf(&rec)); // @sfu
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CSessionCore::GetIncompleteSessList(int grade, PPID cashNodeID, PPIDArray * pSessList)
{
	int    ok = -1;
	int    idx = 0;
	CSessionTbl::Key0 k0;
	CSessionTbl::Key1 k1;
	void * p_k = 0;
	DBQ  * dbq = 0;
	if(cashNodeID) {
		dbq = & (*dbq && this->CashNodeID == cashNodeID);
		idx = 1;
		MEMSZERO(k1);
		p_k = &k1;
	}
	else {
		MEMSZERO(k0);
		p_k = &k0;
	}
	dbq = &(*dbq && this->Incomplete == static_cast<long>(grade) && this->Temporary == 0L);
	BExtQuery q(this, idx);
	q.select(this->ID, 0L).where(*dbq);
	for(q.initIteration(false, p_k, spFirst); ok && q.nextIteration() > 0;)
		if(!pSessList->add(data.ID))
			ok = 0;
	return ok ? (pSessList->getCount() ? 1 : -1) : 0;
}

int CSessionCore::CheckUniqueDateTime(PPID superSessID, long posNumber, LDATE * pDt, LTIME * pTm)
{
	//
	// Проверяем уникальность даты и времени для новой записи.
	// Если они не уникальны, то прибавляем по одной микросекунде
	// до тех пор, пока не найдем уникальное время.
	//
	int    ok = 1;
	LDATE  dt = *pDt;
	LTIME  tm = *pTm;
	CSessionTbl::Key3 k3;
	k3.SuperSessID = superSessID;
	k3.CashNumber = posNumber;
	k3.Dt = dt;
	k3.Tm = tm;
	while(search(3, &k3, spEq)) {
		int    h, m, s, ts;
		decodetime(&h, &m, &s, &ts, &tm);
		if(ts < 999)
			ts++;
		else {
			ts = 0;
			if(s < 59)
				s++;
			else {
				s = 0;
				if(m < 59)
					m++;
				else {
					m = 0;
					if(h < 23)
						h++;
					else {
						h = 0;
						plusdate(&dt, 1, 0);
					}
				}
			}
		}
		tm = encodetime(h, m, s, ts);
		k3.SuperSessID = superSessID;
		k3.CashNumber = posNumber;
		k3.Dt = dt;
		k3.Tm = tm;
		ok = 2;
	}
	*pDt = dt;
	*pTm = tm;
	return ok;
}

int CSessionCore::CheckUniqueDateTime(PPID cashNodeID, LDATE * pDt, LTIME * pTm)
{
	//
	// Проверяем уникальность даты и времени для новой записи.
	// Если они не уникальны, то прибавляем по одной микросекунде
	// до тех пор, пока не найдем уникальное время.
	//
	int    ok = 1;
	LDATE  dt = *pDt;
	LTIME  tm = *pTm;
	CSessionTbl::Key1 k1;
	k1.CashNodeID = cashNodeID;
	k1.Dt = dt;
	k1.Tm = tm;
	while(search(1, &k1, spEq)) {
		int    h, m, s, ts;
		decodetime(&h, &m, &s, &ts, &tm);
		if(ts < 999)
			ts++;
		else {
			ts = 0;
			if(s < 59)
				s++;
			else {
				s = 0;
				if(m < 59)
					m++;
				else {
					m = 0;
					if(h < 23)
						h++;
					else {
						h = 0;
						plusdate(&dt, 1, 0);
					}
				}
			}
		}
		tm = encodetime(h, m, s, ts);
		k1.CashNodeID = cashNodeID;
		k1.Dt = dt;
		k1.Tm = tm;
		ok = 2;
	}
	*pDt = dt;
	*pTm = tm;
	return ok;
}

int CSessionCore::CreateSess(PPID * pID, PPID cashNodeID, long cashN, long sessN, LDATETIME dtm, int temporary)
{
	CSessionTbl::Rec rec;
	rec.CashNodeID = cashNodeID;
	rec.CashNumber = cashN;
	rec.SessNumber = sessN;
	rec.Dt = dtm.d;
	rec.Tm = dtm.t;
	rec.Temporary = BIN(temporary);
	CheckUniqueDateTime(cashNodeID, &rec.Dt, &rec.Tm);
	return AddObjRecByID(this, PPOBJ_CSESSION, pID, &rec, 0);
}

int CSessionCore::AttachToSuperSess(PPID superSessID, const PPIDArray & rSessList, int use_ta)
{
	int    ok = -1;
	PPTransaction tra(use_ta);
	CSessionTbl::Rec super_rec, sub_rec;
	THROW(tra);
	THROW(SearchByID_ForUpdate(this, PPOBJ_CSESSION, superSessID, &super_rec));
	{
		LDATE last_date = super_rec.Dt;
		LTIME last_time = super_rec.Tm;
		for(uint i = 0; i < rSessList.getCount(); i++) {
			PPID   sub_sess_id = rSessList.get(i);
			if(sub_sess_id && SearchByID_ForUpdate(this, PPOBJ_CSESSION, sub_sess_id, &sub_rec) > 0) {
				if(sub_rec.Dt > last_date || (sub_rec.Dt == last_date && sub_rec.Tm > last_time)) {
					last_date = sub_rec.Dt;
					last_time = sub_rec.Tm;
				}
				super_rec.Amount   += sub_rec.Amount;
				super_rec.Discount += sub_rec.Discount;
				super_rec.BnkAmount += sub_rec.BnkAmount;       // @CSCardAmount
				super_rec.CSCardAmount += sub_rec.CSCardAmount;
				sub_rec.SuperSessID = super_rec.ID;
				THROW_DB(updateRecBuf(&sub_rec)); // @sfu
				ok = 1;
			}
		}
		if(ok > 0) {
			CheckUniqueDateTime(super_rec.CashNodeID, &last_date, &last_time);
			THROW(SearchByID_ForUpdate(this, PPOBJ_CSESSION, superSessID, 0));
			super_rec.Dt = last_date;
			super_rec.Tm = last_time;
			THROW_DB(updateRecBuf(&super_rec)); // @sfu
		}
	}
	THROW(tra.Commit());
	CATCHZOK
	return ok;
}

int CSessionCore::CreateSuperSess(PPID * pID, PPID cashNodeID, const PPIDArray & rSessList, int use_ta)
{
	int    ok = 1;
	PPID   super_id = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(!pID || *pID == 0) {
			CSessionTbl::Rec super_rec;
			long   last_number = 0;
			GetLastNumber(cashNodeID, 0, &last_number);
			super_rec.CashNodeID = cashNodeID;
			super_rec.CashNumber = 0;
			super_rec.Incomplete = CSESSINCMPL_CHECKS;
			super_rec.SessNumber = last_number + 1;
			THROW(AddObjRecByID(this, PPOBJ_CSESSION, &super_id, &super_rec, 0));
		}
		else
			super_id = *pID;
		THROW(AttachToSuperSess(super_id, rSessList, 0));
		THROW(tra.Commit());
		ASSIGN_PTR(pID, super_id);
	}
	CATCHZOK
	return ok;
}

int CSessionCore::UpdateTotal(PPID id, const CSessTotal * pTotal, int wrOffSum, int completness, int use_ta)
{
	int    ok = -1;
	CSessionTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(id, &rec) > 0);
		if(wrOffSum) {
			rec.WrOffAmount += pTotal->WrOffAmount;
			rec.AggrAmount   = pTotal->AggrAmount;
			rec.AggrRest     = pTotal->AggrRest;
		}
		else {
			rec.Amount     = pTotal->Amount;
			rec.Discount   = pTotal->Discount;
			rec.BnkAmount  = pTotal->BnkAmount; // @CSCardAmount
			rec.CSCardAmount = pTotal->CSCardAmount;
		}
		if(completness >= 0)
			rec.Incomplete = completness;
		THROW_DB(updateRecBuf(&rec));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CSessionCore::GetTempAsyncSessList(PPID nodeID, const DateRange * pPeriod, PPIDArray * pSessList)
{
	int    ok = -1;
	CSessionTbl::Key1 k1;
	BExtQuery q(this, 1);
	MEMSZERO(k1);
	k1.CashNodeID = nodeID;
	k1.Dt = pPeriod ? pPeriod->low : ZERODATE;
	q.select(ID, Dt, Temporary, 0L).where(CashNodeID == nodeID);
	for(q.initIteration(false, &k1); q.nextIteration();) {
		if(data.Temporary > 0 && (!pPeriod || pPeriod->CheckDate(data.Dt))) {
			CALLPTRMEMB(pSessList, addUnique(data.ID));
			ok = 1;
		}
	}
	return ok;
}

#define ASYNCSESS_BYLASTDAYS 7L

int CSessionCore::GetActiveSessList(PPID locID, ObjIdListFilt * pActiveSessList)
{
	int    ok = -1;
	if(pActiveSessList) {
		DateRange prd;
		PPObjCashNode cn_obj;
		PPCashNode cn_rec;
		prd.SetDate(getcurdate_());
		plusdate(&prd.low, -ASYNCSESS_BYLASTDAYS, 0);
		PPIDArray async_sess_list;
		for(SEnum en = cn_obj.P_Ref->Enum(PPOBJ_CASHNODE, 0); en.Next(&cn_rec) > 0;) {
			if(!locID || cn_rec.LocID == locID)
				if(PPCashMachine::IsSyncCMT(cn_rec.CashType))
					pActiveSessList->Add(cn_rec.CurSessID);
				else
					GetTempAsyncSessList(cn_rec.ID, &prd, &async_sess_list);
		}
		for(uint i = 0; i < async_sess_list.getCount(); i++)
			pActiveSessList->Add(async_sess_list.at(i));
		ok = 1;
	}
	return ok;
}
