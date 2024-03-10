// PPACTION.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2016, 2017, 2018, 2019, 2020, 2022, 2023, 2024
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop

SysJournal::SysJournal() : SysJournalTbl(), P_RsrvT((CConfig.Flags & CCFLG_RSRVSJ) ? new SjRsrvTbl : 0)
{
	PPLoadText(PPTXT_ERR_SYSJLOGGING, FmtBuf);
}

SysJournal::~SysJournal()
{
	ZDELETE(P_RsrvT);
}

int SysJournal::LogEvent(PPID action, PPID obj, PPID id, long extData, int use_ta)
{
	int    ok = -1;
	const LDATETIME now_dtm = getcurdatetime_();
	const PPID user_id = LConfig.UserID;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		data.Dt      = now_dtm.d;
		data.Tm      = now_dtm.t;
		data.UserID  = user_id;
		data.Action  = action;
		data.ObjType = obj;
		data.ObjID   = id;
		data.Extra   = extData;
		if(insertRec())
			ok = 1;
		if(ok <= 0) {
			PPSetError(PPERR_DBENGINE);
			{
				int    rsrv_ok = -1;
				if(P_RsrvT) {
					//
					// Следующий участок кода полностью повторяет внесение записи в SysJournal (см. выше),
					// с поправкой на то, что запись вносится в резервную таблицу.
					// Дата и время инициализируются тем же значением, с которым осуществлялась
					// самая первая попытка добавления записи в регулярную таблицу.
					//
					P_RsrvT->data.Dt      = now_dtm.d;
					P_RsrvT->data.Tm      = now_dtm.t;
					P_RsrvT->data.UserID  = user_id;
					P_RsrvT->data.Action  = action;
					P_RsrvT->data.ObjType = obj;
					P_RsrvT->data.ObjID   = id;
					P_RsrvT->data.Extra   = extData;
					if(P_RsrvT->insertRec())
						rsrv_ok = 1;
				}
				{
					//SString msg_buf, added_buf, temp_buf;
					PPGetLastErrorMessage(1, MsgBuf.Z());
					AddedBuf.Z().Cat(data.Action).Semicol().Cat(obj).Semicol().Cat(id).Semicol().Cat(extData).Semicol().Cat(MsgBuf);
					TempBuf.Z();
					if(rsrv_ok < 0)
						TempBuf.CatParStr("NR");
					else if(rsrv_ok > 0)
						TempBuf.CatParStr("R1");
					else
						TempBuf.CatParStr("R0");
					MsgBuf.Z().Printf(TempBuf.Space().Cat(FmtBuf), AddedBuf.cptr());
					PPLogMessage(PPFILNAM_ERR_LOG, MsgBuf, LOGMSGF_TIME|LOGMSGF_USER);
				}
			}
			ok = 0;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SysJournal::GetEvent(PPID action, long extraVal, int mode, LDATETIME * pMoment, int maxDays, SysJournalTbl::Rec * b)
{
	SysJournalTbl::Key0 k;
	DBQ  * dbq = 0;
	BExtQuery q(this, 0, 1);
	int    sp;
	MEMSZERO(k);
	if(pMoment) {
		k.Dt = pMoment->d;
		k.Tm = pMoment->t;
	}
	if(mode == -1 || mode == 0) {
		SETIFZ(k.Dt, MAXDATE);
		SETIFZ(k.Tm, MAXTIME);
		sp = spLt;
		dbq = & (*dbq && this->Dt <= k.Dt);
		if(maxDays > 0) {
			LDATE limit = plusdate((k.Dt == MAXDATE) ? getcurdate_() : k.Dt, -maxDays);
			dbq = & (*dbq && this->Dt >= limit);
		}
	}
	else {
		sp = spGt;
		dbq = & (*dbq && this->Dt >= k.Dt);
		if(maxDays > 0 && k.Dt != ZERODATE) {
			LDATE limit = plusdate(k.Dt, maxDays);
			dbq = & (*dbq && this->Dt <= limit);
		}
	}
	dbq = ppcheckfiltid(dbq, this->Action, action);
	dbq = ppcheckfiltid(dbq, this->Extra, extraVal); // @v10.7.5
	q.selectAll().where(*dbq);
	q.initIteration((mode <= 0), &k, sp);
	if(q.nextIteration() > 0) {
		CALLPTRMEMB(pMoment, Set(data.Dt, data.Tm));
		copyBufTo(b);
		return 1;
	}
	else
		return PPDbSearchError();
}

int SysJournal::GetLastUserEvent(PPID action, PPID userID, PPID sessID, const DateRange * pSrchPeriod, LDATETIME * pMoment, SysJournalTbl::Rec * pRec)
{
	SysJournalTbl::Key0 k;
	DBQ  * dbq = 0;
	BExtQuery q(this, 0, 1);
	MEMSZERO(k);
	if(pSrchPeriod) {
		k.Dt = pSrchPeriod->upp;
		dbq = & (*dbq && this->Dt >= pSrchPeriod->low);
	}
	k.Tm =  MAXTIME;
	SETIFZ(k.Dt, MAXDATE);
	dbq = & (*dbq && this->Dt <= k.Dt);
	dbq = ppcheckfiltid(dbq, this->Action, action);
	dbq = ppcheckfiltid(dbq, this->UserID, userID);
	dbq = ppcheckfiltid(dbq, this->Extra, sessID);
	q.selectAll().where(*dbq);
	q.initIteration(true, &k, spLt);
	if(q.nextIteration() > 0) {
		CALLPTRMEMB(pMoment, Set(data.Dt, data.Tm));
		copyBufTo(pRec);
		return 1;
	}
	else
		return PPDbSearchError();
}

int SysJournal::GetLastEvent(PPID action, long extraVal, LDATETIME * pMoment, int maxDays, SysJournalTbl::Rec * pRec)
{
	return GetEvent(action, extraVal, -1, pMoment, maxDays, pRec);
}

int SysJournal::GetLastObjUnifyEvent(PPID objType, PPID objID, PPID * pDestID, SysJournalTbl::Rec * pRec)
{
	int    ok = -1;
	SysJournalTbl::Key1 k;
	k.ObjType = objType;
	k.ObjID   = objID;
	k.Dt      = MAXDATE;
	k.Tm      = MAXTIME;
	if(search(1, &k, spLt) && k.ObjType == objType && k.ObjID == objID) {
		do {
			if(data.Action == PPACN_OBJUNIFY && data.Extra > 0) {
				copyBufTo(pRec);
				ASSIGN_PTR(pDestID, data.Extra);
				ok = 1;
			}
		} while(ok < 0 && search(1, &k, spPrev) && k.ObjType == objType && k.ObjID == objID);
	}
	return ok;
}

int SysJournal::GetObjEventList(PPID objType, PPID objID, const PPIDArray * pActList, SArray * pResultList)
{
	int    ok = -1;
	SysJournalTbl::Key1 k1;
	MEMSZERO(k1);
	k1.ObjType = objType;
	k1.ObjID = objID;
	BExtQuery q(this, 1);
	DBQ * dbq = &(this->ObjType == objType && this->ObjID == objID);
	dbq = ppcheckfiltidlist(dbq, this->Action, pActList);
	q.selectAll().where(*dbq);
	for(q.initIteration(false, &k1, spGe); q.nextIteration() > 0;) {
		if(pResultList) {
			THROW_SL(pResultList->insert(&data));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SysJournal::GetObjCreationEvent(PPID objType, PPID objID, SysJournalTbl::Rec * pRec)
{
	int    ok = -1;
	PPIDArray action_list;
	SArray result_list(sizeof(SysJournalTbl::Rec));
	if(objType == PPOBJ_BILL)
		action_list.add(PPACN_TURNBILL);
	else
		action_list.add(PPACN_OBJADD);
	if(GetObjEventList(objType, objID, &action_list, &result_list) > 0) {
		if(result_list.getCount()) {
			ASSIGN_PTR(pRec, *(SysJournalTbl::Rec *)result_list.at(result_list.getCount()-1));
			ok = 1;
		}
	}
	return ok;
}

int SysJournal::GetLastObjModifEvent(PPID objType, PPID objID, LDATETIME * pMoment, int * pCreation, SysJournalTbl::Rec * pRec)
{
	int    ok = -1, cr = 0;
	SysJournalTbl::Key1 k;
	k.ObjType = objType;
	k.ObjID = objID;
	k.Dt    = MAXDATE;
	k.Tm    = MAXTIME;
	while(ok < 0 && search(1, &k, spLt) && k.ObjType == objType && k.ObjID == objID) {
		if(objType == PPOBJ_BILL) {
			if(data.Action == PPACN_TURNBILL) {
				cr = 1;
				ok = 1;
			}
			else if(data.Action == PPACN_UPDBILL) {
				cr = 0;
				ok = 1;
			}
		}
		else {
			if(data.Action == PPACN_OBJADD) {
				cr = 1;
				ok = 1;
			}
			else if(data.Action == PPACN_OBJUPD) {
				cr = 0;
				ok = 1;
			}
			else if(oneof2(data.Action, PPACN_GOODSQUOTUPD, PPACN_QUOTUPD2) && objType == PPOBJ_GOODS) {
				cr = 0;
				ok = 1;
			}
		}
	}
	if(ok > 0) {
		CALLPTRMEMB(pMoment, Set(data.Dt, data.Tm));
		ASSIGN_PTR(pCreation, cr);
		copyBufTo(pRec);
	}
	else {
		CALLPTRMEMB(pMoment, Z());
	}
	return ok;
}

int SysJournal::GetNextObjEvent(PPID objType, PPID objID, const PPIDArray * pActAry, const LDATETIME & rSince, SysJournalTbl::Rec * pRec)
{
	int    ok = -1;
	SysJournalTbl::Key1 k;
	k.ObjType = objType;
	k.ObjID = objID;
	k.Dt    = rSince.d;
	k.Tm    = rSince.t;
	if(search(1, &k, spGt) && data.ObjType == objType && data.ObjID == objID) do {
		if(!SVectorBase::GetCount(pActAry) || pActAry->lsearch(data.Action)) {
			copyBufTo(pRec);
			ok = 1;
		}
	} while(ok < 0 && search(1, &k, spNext) && data.ObjType == objType && data.ObjID == objID);
	return ok;
}

int SysJournal::GetLastObjEvent(PPID objType, PPID objID, const PPIDArray * pActAry, LDATETIME * pMoment, SysJournalTbl::Rec * pRec)
{
	int    ok = -1;
	SysJournalTbl::Key1 k;
	k.ObjType = objType;
	k.ObjID = objID;
	k.Dt    = MAXDATE;
	k.Tm    = MAXTIME;
	const  PPID single_acn = pActAry ? pActAry->getSingle() : 0;
	if(single_acn) {
		BExtQuery q(this, 1);
		q.selectAll().where(this->ObjType == objType && this->ObjID == objID && this->Action == single_acn);
		q.initIteration(true, &k, spLt);
		if(q.nextIteration() > 0) {
			assert(data.ObjType == objType && data.ObjID == objID && data.Action == single_acn); // @paranoic
			if(data.ObjType == objType && data.ObjID == objID && data.Action == single_acn) // @paranoic
				ok = 1;
		}
	}
	else {
		if(search(1, &k, spLt) && k.ObjType == objType && k.ObjID == objID) {
			do {
				if(!pActAry || pActAry->getCount() == 0 || pActAry->lsearch(data.Action))
					ok = 1;
			} while(ok < 0 && search(1, &k, spPrev) && k.ObjType == objType && k.ObjID == objID);
		}
	}
	if(ok > 0) {
		CALLPTRMEMB(pMoment, Set(data.Dt, data.Tm));
		copyBufTo(pRec);
	}
	else {
		CALLPTRMEMB(pMoment, Z());
	}
	return ok;
}

int SysJournal::GetUpdatedConfigListSince(PPID cfgType, const LDATETIME & rSince, PPIDArray & rCfgIdList)
{
	int    ok = -1;
	SysJournalTbl::Key0 k;
	DBQ * dbq = 0;
	k.Dt = rSince.d;
	k.Tm = rSince.t;
	BExtQuery q(this, 0, 16);
	dbq = &(*dbq && this->Dt >= rSince.d && this->Action == PPACN_CONFIGUPDATED);
	dbq = ppcheckfiltid(dbq, this->ObjType, cfgType);
	q.select(this->Dt, this->Tm, this->ObjID, this->Action, 0L);
	q.where(*dbq);
	for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;) {
		if(cmp(rSince, data.Dt, data.Tm) < 0 && data.Action == PPACN_CONFIGUPDATED) {
			THROW(rCfgIdList.add(data.ObjType));
			ok = 1;
		}
	}
	rCfgIdList.sortAndUndup();
	CATCHZOK
	return ok;
}

int SysJournal::GetObjListByEventSince(PPID objType, const PPIDArray * pActList, const LDATETIME & rSince, PPIDArray & rObjList, TSVector <SysJournalTbl::Rec> * pRecList)
{
	int    ok = -1;
	SysJournalTbl::Key0 k;
	DBQ * dbq = 0;
	k.Dt = rSince.d;
	k.Tm = rSince.t;
	BExtQuery q(this, 0, 128);
	dbq = &(*dbq && this->Dt >= rSince.d);
	dbq = ppcheckfiltid(dbq, this->ObjType, objType);
	if(pRecList) {
		pRecList->clear();
		q.selectAll();
	}
	else
		q.select(this->Dt, this->Tm, this->ObjID, this->Action, 0L);
	q.where(*dbq);
	for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;) {
		if(cmp(rSince, data.Dt, data.Tm) < 0 && (!pActList || pActList->lsearch(data.Action))) {
			THROW(rObjList.add(data.ObjID));
			if(pRecList) {
				pRecList->insert(&data);
			}
			ok = 1;
		}
	}
	rObjList.sortAndUndup();
	CATCHZOK
	return ok;
}

int SysJournal::GetObjListByEventPeriod(PPID objType, PPID userID, const PPIDArray * pActList, const DateRange * pPeriod, PPIDArray & rObjList)
{
	rObjList.clear();
	int    ok = -1;
	const  uint acn_list_c = SVectorBase::GetCount(pActList);
	const  PPID single_acn = (acn_list_c == 1) ? pActList->get(0) : 0;
	SysJournalTbl::Key0 k;
	k.Dt = pPeriod ? pPeriod->low : ZERODATE;
	k.Tm = ZEROTIME;
	DBQ * dbq = 0;
	BExtQuery q(this, 0);
	dbq = &(this->ObjType == objType && daterange(this->Dt, pPeriod));
	dbq = ppcheckfiltid(dbq, this->UserID, userID);
	q.select(this->Dt, this->Tm, this->ObjID, this->Action, this->UserID, 0L).where(*dbq);
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
		if(single_acn) {
			if(data.Action == single_acn) {
				THROW_SL(rObjList.add(data.ObjID));
				ok = 1;
			}
		}
		else if(!acn_list_c || pActList->lsearch(data.Action)) {
			THROW_SL(rObjList.add(data.ObjID));
			ok = 1;
		}
	}
	rObjList.sortAndUndup();
	CATCHZOK
	return ok;
}

int SysJournal::GetObjRemovingEventListByPeriod(PPID objType, PPID userID, const DateRange * pPeriod, TSVector <SysJournalTbl::Rec> & rList)
{
	int    ok = -1;
	const  PPID single_acn = (objType == PPOBJ_BILL) ? PPACN_RMVBILL : PPACN_OBJRMV;
	SysJournalTbl::Key0 k;
	k.Dt = pPeriod ? pPeriod->low : ZERODATE;
	k.Tm = ZEROTIME;
	DBQ * dbq = 0;
	BExtQuery q(this, 0);
	dbq = &(this->ObjType == objType && this->Action ==  single_acn && daterange(this->Dt, pPeriod));
	dbq = ppcheckfiltid(dbq, this->UserID, userID);
	q.selectAll().where(*dbq);
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
		if(data.Action == single_acn) {
			THROW_SL(rList.insert(&data));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SysJournal::CheckObjForFilt(PPID objType, PPID objID, const SysJournalFilt * pFilt)
{
	int    ok = -1;
	if(pFilt && !pFilt->IsEmpty()) {
		if(CheckFiltID(pFilt->ObjType, objType) && CheckFiltID(pFilt->ObjID, objID)) {
			SysJournalTbl::Key1 k1;
			DBQ * dbq = 0;
			BExtQuery q(this, 1, 1); // Нам достаточно одной записи
			dbq = &(*dbq && this->ObjType == objType && this->ObjID == objID && daterange(this->Dt, &pFilt->Period));
			dbq = ppcheckfiltid(dbq, this->UserID, pFilt->UserID);
			dbq = &(*dbq && ppidlist(this->Action, &pFilt->ActionIDList));
			q.selectAll().where(*dbq);
			MEMSZERO(k1);
			k1.ObjType = objType;
			k1.ObjID = objID;
			k1.Dt = pFilt->Period.low;
			for(q.initIteration(false, &k1, spGe); ok < 0 && q.nextIteration() > 0;) {
				ok = 1;
			}
		}
	}
	else
		ok = 1;
	return ok;
}

int SysJournal::IsEventExists(PPID objType, PPID objID, PPID userID, const PPIDArray * pActList)
{
	int    ok = -1;
	SysJournalTbl::Key1 k;
	k.ObjType = objType;
	k.ObjID = objID;
	k.Dt    = MAXDATE;
	k.Tm    = MAXTIME;
	if(search(1, &k, spLt) && k.ObjType == objType && k.ObjID == objID) {
		do {
			if((!userID || userID == data.UserID) && (!SVectorBase::GetCount(pActList) || pActList->lsearch(data.Action)))
				ok = 1;
		} while(ok < 0 && search(1, &k, spPrev) && k.ObjType == objType && k.ObjID == objID);
	}
	return ok;
}

int SysJournal::DoMaintain(LDATE toDt, int recover, PPLogger * pLogger)
{
	int    ok = 1;
	SString fmt_buf, msg_buf, added_buf, temp_buf;
	IterCounter cntr;
	SysJournalTbl::Key0 k;
	{
		PPTransaction tra(1);
		THROW(tra);
		if(recover && P_RsrvT) {
			SjRsrvTbl::Key0 rk0;
			MEMSZERO(rk0);
			PPInitIterCounter(cntr, P_RsrvT);
			if(P_RsrvT->searchForUpdate(0, &rk0, spFirst)) {
				do {
					int    r = 0;
					SjRsrvTbl::Rec rsrv_rec;
					P_RsrvT->copyBufTo(&rsrv_rec);
					LDATETIME dtm;
					dtm.Set(rsrv_rec.Dt, rsrv_rec.Tm);
					data.Dt      = dtm.d;
					data.Tm      = dtm.t;
					data.UserID  = rsrv_rec.UserID;
					data.Action  = rsrv_rec.Action;
					data.ObjType = rsrv_rec.ObjType;
					data.ObjID   = rsrv_rec.ObjID;
					data.Extra   = rsrv_rec.Extra;
					if(insertRec()) {
						THROW_DB(P_RsrvT->deleteRec()); // @sfu
					}
					else {
						PPSetError(PPERR_DBENGINE);
						PPGetLastErrorMessage(1, msg_buf);
						added_buf.Z().Cat(rsrv_rec.Action).Semicol().Cat(rsrv_rec.ObjType).Semicol().Cat(rsrv_rec.ObjID).
							Semicol().Cat(rsrv_rec.Extra).Semicol().Cat(msg_buf);
						msg_buf.Z().Printf(PPLoadTextS(PPTXT_ERR_SYSJRECOVERING, fmt_buf), added_buf.cptr());
						if(pLogger)
							pLogger->Log(msg_buf);
						else
							PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
					}
					PPWaitPercent(cntr.Increment());
				} while(P_RsrvT->searchForUpdate(0, &rk0, spNext));
			}
			else if(BTROKORNFOUND && pLogger)
				pLogger->LogString(PPTXT_RSRVSYSJNOTHING, 0);
			THROW_DB(BTROKORNFOUND);
		}
		if(toDt) {
			SFsPath ps(DBTable::GetName());
			ps.Merge(0, SFsPath::fDrv|SFsPath::fDir, temp_buf);
			msg_buf.Printf(PPLoadTextS(PPTXT_DBMAINTAIN, fmt_buf), temp_buf.cptr());
			{
				MEMSZERO(k);
				BExtQuery q(this, 0);
				q.select(this->Dt, this->Tm, 0).where(this->Dt <= toDt);
				cntr.Init(q.countIterations(0, &k, spFirst));
			}
			k.Dt = toDt;
			k.Tm = MAXTIME;
			for(; searchForUpdate(0, &k, spLt) && k.Dt <= toDt; PPWaitPercent(cntr.Increment(), msg_buf)) {
				THROW_DB(deleteRec()); // @sfu
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

void SysJournal::Subst(SubstGrpSysJournal sgsj, PPID opID, PPID prmrID, PPID scndID, PPID * pID)
{
	PPID   id = 0;
	switch(sgsj) {
		case sgsjOp: id = opID; break;
		case sgsjUser: id = prmrID; break;
		case sgsjObjType: id = scndID; break;
	}
	ASSIGN_PTR(pID, id);
}

void SysJournal::GetSubstName(SubstGrpSysJournal sgsj, PPID id, char * pBuf, size_t bufLen)
{
	SString temp_buf;
	if(sgsj == sgsjOp)
		GetObjectName(PPOBJ_ACTION, id, temp_buf);
	else if(sgsj == sgsjUser) {
		PPSecur rec;
		PPObjSecur obj(PPOBJ_USR, 0);
		obj.Fetch(id, &rec);
		temp_buf = rec.Name;
	}
	else if(sgsj == sgsjObjType)
		GetObjectTitle(id, temp_buf);
	temp_buf.CopyTo(pBuf, bufLen);
}

void SysJournal::GetSubstName(SubstGrpSysJournal sgsj, PPID id, SString & rBuf)
{
	rBuf.Z();
	if(sgsj == sgsjOp)
		GetObjectName(PPOBJ_ACTION, id, rBuf);
	else if(sgsj == sgsjUser) {
		PPSecur rec;
		PPObjSecur obj(PPOBJ_USR, 0);
		if(obj.Fetch(id, &rec) > 0)
			rBuf = rec.Name;
		else
			ideqvalstr(id, rBuf);
	}
	else if(sgsj == sgsjObjType)
		GetObjectTitle(id, rBuf);
}
//
//
//
PPGta::PPGta()
{
	THISZERO();
}

GtaJournalCore::GtaJournalCore() : GtaJournalTbl()
{
}

static SString & GetGoodsCodeForOp(int op, PPID objType, SString & rBuf)
{
	(rBuf = "GTA").CatLongZ(op, 3);
	if(objType && oneof4(op, GTAOP_OBJGET, GTAOP_OBJADD, GTAOP_OBJMOD, GTAOP_OBJRMV))
		rBuf.CatLongZ(objType, 4);
	return rBuf;
}

void PPObjBill::InitGtaBlock()
{
	SETIFZ(P_ScObj, new PPObjSCard);
}

PPID FASTCALL PPObjBill::GetGtaGoodsID(const PPGta & rGta)
{
	PPID   goods_id = 0;
	SString code;
	BarcodeTbl::Rec bc_rec;
	long   assoc_key = MakeLong((int16)rGta.ObjId.Obj, (int16)rGta.Op);
	if(!GtaB.OpGoodsAssoc.Search(assoc_key, &goods_id, 0)) {
		if(GObj.P_Tbl->SearchBarcode(GetGoodsCodeForOp(rGta.Op, rGta.ObjId.Obj, code), &bc_rec) > 0)
			goods_id = bc_rec.GoodsID;
		if(goods_id) // Пока нет актуализации списка нулевые goods_id туда запихавать не будем
			GtaB.OpGoodsAssoc.Add(assoc_key, goods_id, 0);
	}
	if(!goods_id && rGta.ObjId.Obj) {
		assoc_key = MakeLong((int16)0, (int16)rGta.Op);
		if(!GtaB.OpGoodsAssoc.Search(assoc_key, &goods_id, 0)) {
			if(GObj.P_Tbl->SearchBarcode(GetGoodsCodeForOp(rGta.Op, 0, code), &bc_rec) > 0)
				goods_id = bc_rec.GoodsID;
			if(goods_id) // Пока нет актуализации списка нулевые goods_id туда запихавать не будем
				GtaB.OpGoodsAssoc.Add(assoc_key, goods_id, 0);
		}
	}
	return goods_id;
}

int PPObjBill::GetGtaGuaAssoc(const PPGta & rGta, PPObjBill::GtaBlock::GuaAssocItem & rAssoc)
{
	int    ok = 1;
	MEMSZERO(rAssoc);
	rAssoc.GuaID = rGta.GlobalUserID;
	if(rAssoc.GuaID) {
		uint   pos = 0;
		if(GtaB.GuaAssoc.lsearch(&rAssoc.GuaID, &pos, CMPF_LONG)) {
			GtaBlock::GuaAssocItem & r_item = GtaB.GuaAssoc.at(pos);
			assert(r_item.GuaID == rAssoc.GuaID);
			rAssoc = r_item;
		}
		else {
			SETIFZ(P_ScObj, new PPObjSCard);
			if(P_ScObj && P_ScObj->P_Tbl) {
				PPObjGlobalUserAcc gua_obj;
				PPGlobalUserAcc gua_rec;
				PPGlobalUserAccConfig gua_cfg;
				if(gua_obj.FetchConfig(&gua_cfg) > 0 && gua_cfg.SCardSerID && gua_obj.Fetch(rAssoc.GuaID, &gua_rec) > 0 && gua_rec.PersonID) {
					rAssoc.PersonID = gua_rec.PersonID;
					PPIDArray sc_list;
					P_ScObj->P_Tbl->GetListByPerson(gua_rec.PersonID, gua_cfg.SCardSerID, &sc_list);
					if(sc_list.getCount()) {
						PPID   sc_id = sc_list.get(0);
						PPID   acs_id = 0;
						rAssoc.SCardID = sc_id;
						if(gua_cfg.PersonKindID) {
							PPObjAccSheet acs_obj;
							PPAccSheet acs_rec;
							for(SEnum en = acs_obj.P_Ref->Enum(PPOBJ_ACCSHEET, 0); !acs_id && en.Next(&acs_rec) > 0;) {
								if(acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup == gua_cfg.PersonKindID)
									acs_id = acs_rec.ID;
							}
						}
						if(acs_id) {
							ArObj.P_Tbl->PersonToArticle(gua_rec.PersonID, acs_id, &rAssoc.ArID);
						}
					}
				}
			}
			GtaB.GuaAssoc.insert(&rAssoc);
		}
	}
	return ok;
}

int PPObjBill::InitGta(PPGta & rGta)
{
	int    ok = -1;
	GtaBlock::GuaAssocItem gua_assoc;
	GetGtaGuaAssoc(rGta, gua_assoc);
	rGta.SCardID = gua_assoc.SCardID;
	if(gua_assoc.SCardID) {
		const  PPID goods_id = GetGtaGoodsID(rGta);
		Goods2Tbl::Rec goods_rec;
		if(goods_id && GObj.Fetch(goods_id, &goods_rec) > 0) {
			QuotIdent qi(0, PPQUOTK_BASE, 0, gua_assoc.ArID);
			double quot = 0.0;
			GObj.GetQuotExt(goods_id, qi, &quot, 1);
			rGta.Quot = quot;
			if(quot != 0.0 && P_ScObj && P_ScObj->P_Tbl) {
				SCardTbl::Rec sc_rec;
				THROW(P_ScObj->P_Tbl->GetRest(gua_assoc.SCardID, ZERODATE, &rGta.SCardRest));
				if(P_ScObj->Search(gua_assoc.SCardID, &sc_rec) > 0)
					rGta.SCardMaxCredit = sc_rec.MaxCredit;
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::CheckInGta(const PPGta & rGta, int use_ta)
{
	int    ok = -1;
	GtaBlock::GuaAssocItem gua_assoc;
	GetGtaGuaAssoc(rGta, gua_assoc);
	if(gua_assoc.SCardID) {
		const  PPID goods_id = GetGtaGoodsID(rGta);
		Goods2Tbl::Rec goods_rec;
		if(goods_id && GObj.Fetch(goods_id, &goods_rec) > 0) {
			QuotIdent qi(0, PPQUOTK_BASE, 0, gua_assoc.ArID);
			double quot = 0.0;
			if(GObj.GetQuotExt(goods_id, qi, &quot, 1) > 0 && quot != 0.0) {
				SCardCore::OpBlock op_blk;
				op_blk.SCardID = gua_assoc.SCardID;
				op_blk.Dtm = rGta.Dtm;
				op_blk.Amount = (rGta.Count > 0) ? -(quot * rGta.Count) : -quot;
				THROW(P_ScObj->P_Tbl->PutOpBlk(op_blk, 0, use_ta));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int GtaJournalCore::CheckInOp(PPGta & rGta, int use_ta)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	{
		GtaJournalTbl::Rec rec;
		GtaJournalTbl::Key1 k1;
		CALLPTRMEMB(p_bobj, InitGtaBlock());
		PPTransaction tra(use_ta);
		THROW(tra);
		rec.Op = rGta.Op;
		rec.GlobalAccID = rGta.GlobalUserID;
		rec.Dt = rGta.Dtm.d;
		rec.Tm = rGta.Dtm.t;
		rec.ObjType = rGta.ObjId.Obj;
		rec.ObjID = rGta.ObjId.Id;
		rec.Count = rGta.Count;
		rec.Duration = rGta.Duration;
		{
			//
			// Суррогатная уникальность индекса по времени
			//
			LDATETIME dtm;
			dtm.Set(rec.Dt, rec.Tm);
			k1.GlobalAccID = rec.GlobalAccID;
			k1.Dt = dtm.d;
			k1.Tm = dtm.t;
			if(search(1, &k1, spEq)) {
				uint   inc_count = 0;
				do {
					dtm.addhs(1);
					k1.Dt = dtm.d;
					k1.Tm = dtm.t;
					inc_count++;
				} while(search(1, &k1, spEq));
				//
				// В случае, если время операции является текущим (как чаще всего и бывает)
				// необходимо задержать выполнение функции на величину искусственного смещения времени
				// для того, чтобы следующая операция не оказалась бы впереди предыдущей.
				//
				SDelay(inc_count * 10);
				//
				rGta.Dtm = dtm;
				rec.Dt = dtm.d;
				rec.Tm = dtm.t;
			}
		}
		THROW_DB(insertRecBuf(&rec));
		if(p_bobj) {
			THROW(p_bobj->CheckInGta(rGta, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
// ObjVersioningCore
//
ObjVersioningCore::ObjVersioningCore() : ObjVerTbl(), State(0)
{
}

int ObjVersioningCore::IsInited() const
{
	return BIN(State & stSCtxInited);
}

SSerializeContext & ObjVersioningCore::GetSCtx()
{
	return SCtx;
}

int ObjVersioningCore::InitSerializeContext(int use_ta)
{
	int    ok = 1;
	if(!(State & stSCtxInited)) {
		SCtx.Init(SSerializeContext::fSeparateDataStruct|SSerializeContext::fDontProcessAttachment, ZERODATE);
		const uint32 sysver = DS.GetMinCompatVersion();
		PPObjID oid_dummy;
		oid_dummy.Z();
		ObjVerTbl::Rec rec;
		SBuffer ctx_buf;
		if(SearchOid(oid_dummy, (long)sysver, &rec, &ctx_buf) > 0) {
			THROW(SCtx.SerializeStateOfContext(-1, ctx_buf));
		}
		else {
			Reference * p_ref = PPRef;
			PPObjBill * p_bobj = BillObj;
			if(p_bobj && p_ref) {
				PPObjGoods goods_obj;
				PPObjPerson psn_obj;
				PPObjSCard sc_obj; // @v10.5.3
				const DBTable * table_list[] = {
					p_ref,
					p_bobj->P_Tbl,
					&p_bobj->GetInvT(),
					goods_obj.P_Tbl,
					&goods_obj.P_Tbl->GetExtTbl_(),
					&goods_obj.P_Tbl->GetBcTbl_(),
					&goods_obj.P_Tbl->GetACodTbl_(),
					psn_obj.P_Tbl, 
					psn_obj.RegObj.P_Tbl,
					sc_obj.P_Tbl // @v10.5.3
				};
				for(uint i = 0; i < SIZEOFARRAY(table_list); i++) {
					const DBTable * p_tbl = table_list[i];
					THROW(SCtx.AddDbtDescr(p_tbl->GetTableName(), &p_tbl->GetFields(), 0));
				}
				THROW(SCtx.SerializeStateOfContext(+1, ctx_buf));
				{
					PPID   vv_id = 0;
					THROW(Add(&vv_id, oid_dummy, &ctx_buf, use_ta));
				}
			}
		}
		State |= stSCtxInited;
	}
	CATCHZOK
	return ok;
}

int ObjVersioningCore::Add(PPID * pID, PPObjID oid, SBuffer * pBuf, int use_ta)
{
	int    ok = 1;
	ObjVerTbl::Rec rec;
	ObjVerTbl::Rec last_ver_rec;
    rec.ObjType = oid.Obj;
    rec.ObjID = oid.Id;
	rec.SysVer = DS.GetMinCompatVersion();
	if(oid.Obj == 0 && oid.Id == 0) {
		rec.Ver = (long)rec.SysVer;
	}
	else {
		if(SearchOid(oid, 0, &last_ver_rec, 0) > 0)
			rec.Ver = last_ver_rec.Ver+1;
		else
			rec.Ver = 1;
	}
	size_t data_len = pBuf ? pBuf->GetAvailableSize() : 0;
	rec.Size = static_cast<int32>(data_len);
	copyBufFrom(&rec);
	if(data_len) {
		const void * p_data_buf = pBuf->GetBuf();
		THROW(writeLobData(this->Data, p_data_buf, data_len));
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW_DB(insertRec(0, pID));
		destroyLobData(this->Data); // @v10.2.11 @fix
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int ObjVersioningCore::Search(PPID id, PPObjID * pOid, long * pVer, SBuffer * pPackBuf)
{
	int    ok = -1;
	ObjVerTbl::Key0 k;
	k.ID = id;
	if(search(0, &k, spEq)) {
		CALLPTRMEMB(pOid, Set(data.ObjType, data.ObjID));
		ASSIGN_PTR(pVer, data.Ver);
		if(pPackBuf) {
			pPackBuf->Z();
			readLobData(this->Data, *pPackBuf);
		}
		destroyLobData(this->Data); // @v10.2.11 @fix
		ok = 1;
	}
	return ok;
}

int ObjVersioningCore::SearchOid(PPObjID oid, long ver, ObjVerTbl::Rec * pRec, SBuffer * pPackBuf)
{
	int    ok = -1;
	int    sp = 0;
	ObjVerTbl::Key1 k;
	k.ObjType = oid.Obj;
	k.ObjID = oid.Id;
	if(ver == 0) {
		k.Ver = MAXLONG;
		sp = spLt;
	}
	else {
		k.Ver = ver;
		sp = spEq;
	}
	if(search(1, &k, sp) > 0 && k.ObjType == oid.Obj && k.ObjID == oid.Id) {
		copyBufTo(pRec);
		if(pPackBuf) {
			pPackBuf->Z();
			readLobData(this->Data, *pPackBuf);
		}
		destroyLobData(this->Data); // @v10.2.11 @fix
		ok = 1;
	}
	return ok;
}

int ObjVersioningCore::GetNextVer(PPObjID oid, long * pVer)
{
	int    ok = 1;
	ObjVerTbl::Rec rec;
	long   ver = (SearchOid(oid, 0, &rec, 0) > 0) ? (rec.Ver + 1) : 1;
	ASSIGN_PTR(pVer, ver);
	return ok;
}
