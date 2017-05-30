// V_SYSJ.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop

void SysJournalViewItem::Clear()
{
	memzero(this, sizeof(SysJournalTbl::Rec));
	ID        = 0;
	GrpText1  = 0;
	GrpCount  = 0;
	AvgEvTime = 0;
}

IMPLEMENT_PPFILT_FACTORY(SysJournal); SLAPI SysJournalFilt::SysJournalFilt() : PPBaseFilt(PPFILT_SYSJOURNAL, 0, 2)
{
	SetFlatChunk(offsetof(SysJournalFilt, ReserveStart),
		offsetof(SysJournalFilt, ActionIDList)-offsetof(SysJournalFilt, ReserveStart));
	SetBranchSArray(offsetof(SysJournalFilt, ActionIDList));
	Init(1, 0);
}

int  SLAPI SysJournalFilt::IsEmpty() const
{
	if(!Period.IsZero())
		return 0;
	else if(UserID)
		return 0;
	else if(ObjType)
		return 0;
	else if(ActionIDList.getCount())
		return 0;
	else
		return 1;
}
//
//
//
SysJFiltDialog::SysJFiltDialog(uint resID) : TDialog(resID)
{
	SetupCalPeriod(CTLCAL_SYSJFILT_PERIOD, CTL_SYSJFILT_PERIOD);
}

IMPL_HANDLE_EVENT(SysJFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmSysJActionList)) {
		ListToListData l2l_data(PPOBJ_ACTION, (void *)-1, &Filt.ActionIDList);
		l2l_data.TitleStrID = 0; // PPTXT_XXX;
		if(ListToListDialog(&l2l_data) > 0)
			if(Filt.ActionIDList.isList()) {
				SetComboBoxListText(this, CTLSEL_SYSJFILT_ACTION);
				disableCtrl(CTLSEL_SYSJFILT_ACTION, 1);
			}
			else {
				setCtrlLong(CTLSEL_SYSJFILT_ACTION, Filt.ActionIDList.getSingle());
				disableCtrl(CTLSEL_SYSJFILT_ACTION, 0);
			}
	}
	else if(event.isCbSelected(CTLSEL_SYSJFILT_ACTION))
		Filt.ActionIDList.setSingleNZ(getCtrlLong(CTLSEL_SYSJFILT_ACTION));
	else if(event.isCbSelected(CTLSEL_SYSJFILT_SUBST) || event.isCbSelected(CTLSEL_SYSJFILT_SUBSTDT))
		setupCtrls();
	else
		return;
	clearEvent(event);
}

int SysJFiltDialog::setupCtrls()
{
	int    subst_used = 0;
	getCtrlData(CTLSEL_SYSJFILT_SUBST, &Filt.Sgsj);
	getCtrlData(CTLSEL_SYSJFILT_SUBSTDT, &Filt.Sgd);
	subst_used = BIN(Filt.Sgsj != sgsjNone || Filt.Sgd != sgdNone);
	DisableClusterItem(CTL_SYSJFILT_FLAGS, 0, subst_used);
	if(subst_used) {
		Filt.Flags &= ~SysJournalFilt::fShowObjects;
		SetClusterData(CTL_SYSJFILT_FLAGS, Filt.Flags);
	}
	return 1;
}

int SysJFiltDialog::setDTS(const SysJournalFilt * pFilt)
{
	Filt = *pFilt;
	SetPeriodInput(this, CTL_SYSJFILT_PERIOD, &Filt.Period);
	SetupPPObjCombo(this, CTLSEL_SYSJFILT_USER, PPOBJ_USR, Filt.UserID, 0, 0);
	SetupPPObjCombo(this, CTLSEL_SYSJFILT_ACTION, PPOBJ_ACTION, 0, 0, 0);
	if(Id == DLG_SYSJFILT) {
		SetupObjListCombo(this, CTLSEL_SYSJFILT_OBJ, Filt.ObjType);
		AddClusterAssoc(CTL_SYSJFILT_FLAGS, 0, SysJournalFilt::fShowObjects);
		SetClusterData(CTL_SYSJFILT_FLAGS, Filt.Flags);
	}
	if(Filt.ActionIDList.isList()) {
		SetComboBoxListText(this, CTLSEL_SYSJFILT_ACTION);
		disableCtrl(CTLSEL_SYSJFILT_ACTION, 1);
	}
	else {
		setCtrlLong(CTLSEL_SYSJFILT_ACTION, Filt.ActionIDList.getSingle());
		disableCtrl(CTLSEL_SYSJFILT_ACTION, 0);
	}
	SetupStringCombo(this, CTLSEL_SYSJFILT_SUBST, PPTXT_SUBSTSYSJLIST, Filt.Sgsj);
	SetupSubstDateCombo(this, CTLSEL_SYSJFILT_SUBSTDT, Filt.Sgd);
	setupCtrls();
	return 1;
}

int SysJFiltDialog::getDTS(SysJournalFilt * pFilt)
{
	if(!GetPeriodInput(this, CTL_SYSJFILT_PERIOD, &Filt.Period))
		return PPErrorByDialog(this, CTL_SYSJFILT_PERIOD);
	else {
		getCtrlData(CTLSEL_SYSJFILT_USER, &Filt.UserID);
		if(!Filt.ActionIDList.isList())
			Filt.ActionIDList.setSingleNZ(getCtrlLong(CTLSEL_SYSJFILT_ACTION));
		if(Id == DLG_SYSJFILT) {
			getCtrlData(CTLSEL_SYSJFILT_OBJ, &Filt.ObjType);
			GetClusterData(CTL_SYSJFILT_FLAGS, &Filt.Flags);
			getCtrlData(CTL_SYSJFILT_SUBST, &Filt.Sgsj);
			getCtrlData(CTL_SYSJFILT_SUBSTDT, &Filt.Sgd);
		}
		*pFilt = Filt;
		return 1;
	}
}
//
//
//
PPBaseFilt * SLAPI PPViewSysJournal::CreateFilt(void * extraPtr) const
{
	SysJournalFilt * p_filt = new SysJournalFilt;
	p_filt->Period.SetDate(getcurdate_());
	return p_filt;
}

int SLAPI PPViewSysJournal::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	DIALOG_PROC_BODY_P1(SysJFiltDialog, DLG_SYSJFILT, (SysJournalFilt *)pBaseFilt);
}

SLAPI PPViewSysJournal::PPViewSysJournal() : PPView(0, &Filt, PPVIEW_SYSJOURNAL)
{
	ImplementFlags |= implUseServer;
	P_Tbl      = new SysJournal;
	P_TmpTbl   = 0;
	P_SubstTbl = 0;
	P_NamesTbl = 0;
	P_ObjColl = new ObjCollection;
	LockUpByNotify = 0;
	LastRefreshDtm.SetZero();
}

SLAPI PPViewSysJournal::~PPViewSysJournal()
{
	delete P_Tbl;
	delete P_TmpTbl;
	delete P_SubstTbl;
	delete P_NamesTbl;
	delete P_ObjColl;
}

int SLAPI PPViewSysJournal::SerializeState(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(PPView::SerializeState(dir, rBuf, pCtx));
	THROW(SerializeDbTableByFileName <SysJournalTbl> (dir, &P_TmpTbl, rBuf, pCtx));
	THROW(SerializeDbTableByFileName <TempDoubleIDTbl>  (dir, &P_NamesTbl, rBuf, pCtx));
	CATCHZOK
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, SysJournal);
PP_CREATE_TEMP_FILE_PROC(CreateSubstFile, TempSysJournal);
PP_CREATE_TEMP_FILE_PROC(CreateNamesFile, TempDoubleID);

int SLAPI PPViewSysJournal::IsTempTblNeeded() const
{
	return BIN(Filt.ActionIDList.isList() || (Filt.Flags & SysJournalFilt::fShowObjects) || Filt.Sgsj != sgsjNone || Filt.Sgd != sgdNone);
}

int FASTCALL PPViewSysJournal::CheckRecForFilt(const SysJournalTbl::Rec * pRec)
{
	int    ok = 1;
	if(pRec) {
		if(Filt.ObjType && pRec->ObjType != Filt.ObjType)
			return 0;
		if(Filt.ObjID && pRec->ObjID != Filt.ObjID)
			return 0;
		if(Filt.UserID && pRec->UserID != Filt.UserID)
			return 0;
		if(!Filt.Period.CheckDate(pRec->Dt))
			return 0;
		if(Filt.ActionIDList.getCount() && !Filt.ActionIDList.lsearch(pRec->Action))
			return 0;
		if(Filt.DayOfWeek != 0 && Filt.DayOfWeek != dayofweek(&pRec->Dt, 1))
			return 0;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPViewSysJournal::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	SString msg_buf;
	BExtInsert * p_bei = 0;
	AverageEventTimePrcssr * p_avg_ev_prcssr = 0;
	Counter.Init();
	ZDELETE(P_TmpTbl);
	ZDELETE(P_NamesTbl);
	ZDELETE(P_SubstTbl);
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	LastRefreshDtm = getcurdatetime_();
	if(IsTempTblNeeded()) {
		if(InitIteration()) {
			uint   i = 0;
			if(Filt.Sgsj != sgsjNone || Filt.Sgd != sgdNone) {
				THROW(P_SubstTbl = CreateSubstFile());
				THROW_MEM(p_avg_ev_prcssr = new AverageEventTimePrcssr);
			}
			else {
				if(Filt.ActionIDList.isList()) {
					THROW(P_TmpTbl = CreateTempFile());
					THROW_MEM(p_bei = new BExtInsert(P_TmpTbl));
				}
				if(Filt.Flags & SysJournalFilt::fShowObjects) {
					THROW(P_NamesTbl = CreateNamesFile());
				}
			}
			{
				PPLoadText(PPTXT_VBLD_SYSJ, msg_buf);
				PPTransaction tra(ppDbDependTransaction, 1);
				THROW(tra);
				while(P_IterQuery->nextIteration() > 0) {
					PPID id = 0;
					THROW(PPCheckUserBreak());
					if(Filt.Sgsj != sgsjNone || Filt.Sgd != sgdNone) {
						if(CheckRecForFilt(&P_Tbl->data) > 0) {
							TempSysJournalTbl::Key0 k0;
							TempSysJournalTbl::Rec temp_rec;

							MEMSZERO(temp_rec);
							// temp_rec.ID  = P_Tbl->data.Dt.v;
							// temp_rec.ID2 = P_Tbl->data.Tm.v;
							P_Tbl->Subst(Filt.Sgsj, P_Tbl->data.Action, P_Tbl->data.UserID, P_Tbl->data.ObjType, &temp_rec.ID);
					 		if(Filt.Sgd != sgdNone) {
								SString temp_buf;
								LTIME tm = ZEROTIME;
								LDATE dt = ZERODATE;
								ShrinkSubstDateExt(Filt.Sgd, P_Tbl->data.Dt, P_Tbl->data.Tm, &dt, &tm);
								FormatSubstDateExt(Filt.Sgd, dt, tm, temp_buf);
								temp_buf.CopyTo(temp_rec.DtSubst, sizeof(temp_rec.DtSubst));
								dt.v = (Filt.Sgd == sgdHour) ? tm.v : dt.v;
								temp_rec.Dt  = dt;
								temp_rec.ID2 = dt.v;
					 		}
							else {
								datefmt(&temp_rec.Dt, DATF_DMY, temp_rec.DtSubst);
								temp_rec.Dt = P_Tbl->data.Dt;
							}
							P_Tbl->GetSubstName(Filt.Sgsj, temp_rec.ID, temp_rec.Name, sizeof(temp_rec.Name));
							k0.ID  = temp_rec.ID;
							k0.ID2 = temp_rec.ID2;
							temp_rec.Count = 1;
							if(P_SubstTbl->searchForUpdate(0, &k0, spEq) > 0) {
								P_SubstTbl->data.Count++;
								P_SubstTbl->updateRec();
							}
							else
								P_SubstTbl->insertRecBuf(&temp_rec);
							CALLPTRMEMB(p_avg_ev_prcssr, Add(temp_rec.ID, temp_rec.ID2, P_Tbl->data.Dt, P_Tbl->data.Tm));
						}
					}
					else {
						if(p_bei)
							THROW_DB(p_bei->insert(&P_Tbl->data));
						if(P_NamesTbl && P_Tbl->data.ObjID) {
							TempDoubleIDTbl::Key1 k1;
							k1.ScndID = P_Tbl->data.ObjID;
							k1.PrmrID = P_Tbl->data.ObjType;
							if(!P_NamesTbl->search(1, &k1, spEq)) {
								TempDoubleIDTbl::Rec  nm_rec;
								MEMSZERO(nm_rec);
								PPObject * ppobj = P_ObjColl->GetObjectPtr(P_Tbl->data.ObjType);
								nm_rec.PrmrID = P_Tbl->data.ObjType;
								nm_rec.ScndID = P_Tbl->data.ObjID;
								CALLPTRMEMB(ppobj, GetName(nm_rec.ScndID, nm_rec.Name, sizeof(nm_rec.Name)));
								THROW_DB(P_NamesTbl->insertRecBuf(&nm_rec));
							}
						}
					}
					Counter.Increment();
					PPWaitPercent(Counter, msg_buf);
				}
				if(p_avg_ev_prcssr) {
					AverageEventTimePrcssr::Item * p_avg_item = 0;
					for(uint pos = 0; p_avg_ev_prcssr->Enum(&pos, &p_avg_item) > 0;) {
						TempSysJournalTbl::Key0 k0;
						MEMSZERO(k0);
						k0.ID  = p_avg_item->ID1;
						k0.ID2 = p_avg_item->ID2;
						if(P_SubstTbl->searchForUpdate(0, &k0, spEq) > 0) {
							P_SubstTbl->data.AvgEvTime = p_avg_item->GetAverage();
							THROW_DB(P_SubstTbl->updateRec()); // @sfu
						}
					}
				}
				if(p_bei)
					THROW_DB(p_bei->flash());
				THROW(tra.Commit());
			}
		}
	}
	CATCH
		ok = 0;
		ZDELETE(P_NamesTbl);
	ENDCATCH
	ZDELETE(P_IterQuery);
	ZDELETE(p_bei);
	ZDELETE(p_avg_ev_prcssr);
	if(ok == 0)
		ZDELETE(P_TmpTbl);
	return ok;
}

int SLAPI PPViewSysJournal::InitIteration()
{
	int    ok = 1;
	int    idx = 0;
	DBQ  * dbq = 0;
	union {
		SysJournalTbl::Key0 k0;
		SysJournalTbl::Key1 k1;
		TempSysJournalTbl::Key1 tk1;
	} k, ks;
	MEMSZERO(k);
	ZDELETE(P_IterQuery);
	if(P_SubstTbl) {
		P_IterQuery = new BExtQuery(P_SubstTbl, 1);
		P_IterQuery->selectAll();
		k.tk1.Dt = ZERODATE;
		k.tk1.ID = 0;
		k.tk1.ID2 = 0;
	}
	else if(P_TmpTbl) {
		P_IterQuery = new BExtQuery(P_TmpTbl, 0);
		P_IterQuery->selectAll();
	}
	else {
		if(Filt.ObjType && Filt.ObjID)
			idx = 1;
		else
			idx = 0;
		P_IterQuery = new BExtQuery(P_Tbl, idx);
		P_IterQuery->selectAll();
		dbq = & daterange(P_Tbl->Dt, &Filt.Period);
		dbq = ppcheckfiltid(dbq, P_Tbl->ObjType, Filt.ObjType);
		dbq = ppcheckfiltid(dbq, P_Tbl->ObjID, Filt.ObjID);
		dbq = ppcheckfiltid(dbq, P_Tbl->UserID, Filt.UserID);
		dbq = ppcheckfiltidlist(dbq, P_Tbl->Action, &Filt.ActionIDList);
		P_IterQuery->where(*dbq);
		if(idx == 0) {
			k.k0.Dt = Filt.Period.low;
			k.k0.Tm = Filt.BegTm;
		}
		else {
			k.k1.ObjType = Filt.ObjType;
			k.k1.ObjID   = Filt.ObjID;
			k.k1.Dt      = Filt.Period.low;
			k.k1.Tm      = Filt.BegTm;
		}
	}
	ks = k;
	Counter.Init(P_IterQuery->countIterations(0, &ks, spGe));
	P_IterQuery->initIteration(0, &k, spGe);
	return ok;
}

int FASTCALL PPViewSysJournal::NextIteration(SysJournalViewItem * pItem)
{
	while(pItem && P_IterQuery && P_IterQuery->nextIteration() > 0) {
		if(P_SubstTbl) {
			pItem->Clear();
			pItem->ID       = P_SubstTbl->data.ID;
			pItem->Dt       = P_SubstTbl->data.Dt;
			pItem->GrpCount = P_SubstTbl->data.Count;
			pItem->GrpText1 = P_SubstTbl->data.DtSubst;
			STRNSCPY(pItem->ObjName, P_SubstTbl->data.Name);
			{
				LDATETIME dtm;
				dtm.SetZero();
				long days = dtm.settotalsec(P_SubstTbl->data.AvgEvTime);
				if(days)
					pItem->AvgEvTime.Cat(days).CatChar('d').Space();
				pItem->AvgEvTime.Cat(dtm.t, TIMF_HMS);
			}
			return 1;
		}
		else {
			SysJournalTbl * p_t = NZOR(P_TmpTbl, P_Tbl);
			p_t->copyBufTo(pItem);
			if(p_t->data.ObjID && P_NamesTbl) {
				TempDoubleIDTbl::Key1  k1;
				k1.ScndID = p_t->data.ObjID;
				k1.PrmrID = p_t->data.ObjType;
				if(P_NamesTbl->search(1, &k1, spEq))
					STRNSCPY(pItem->ObjName, P_NamesTbl->data.Name);
				else
					pItem->ObjName[0] = 0;
			}
			else
				pItem->ObjName[0] = 0;
			if(!Filt.BegTm || p_t->data.Dt > Filt.Period.low || p_t->data.Tm >= Filt.BegTm) {
				Counter.Increment();
				PPWaitPercent(Counter);
				return 1;
			}
		}
	}
	return -1;
}

DBQuery * SLAPI PPViewSysJournal::CreateBrowserQuery(uint * pBrwId, SString *)
{
	int    add_dbe = 0;
	uint   brw_id = (Filt.Flags & SysJournalFilt::fShowObjects) ? BROWSER_SYSJ_OBJ : BROWSER_SYSJ;
	DBQuery * q = 0;
	DBE    dbe_user;
	DBE    dbe_objtitle;
	DBE    dbe_action;
	DBE    dbe_person; // @v7.5.11
	DBE  * dbe_extra = 0;
	DBE    dbe_avg_tm;
	DBQ  * dbq = 0;
	TempSysJournalTbl * p_t = 0;
	SysJournalTbl * sj  = new SysJournalTbl(P_TmpTbl ? (const char *)P_TmpTbl->fileName : (const char *)0);
	TempDoubleIDTbl * nm  = 0;
	if(P_SubstTbl) {
		brw_id = BROWSER_SYSJ_SUBST;
		THROW_MEM(p_t = new TempSysJournalTbl((const char *)P_SubstTbl->fileName));
		THROW(CheckTblPtr(p_t));

		PPDbqFuncPool::InitObjNameFunc(dbe_avg_tm, PPDbqFuncPool::IdDurationToTime, p_t->Count);
		q = & select(
			p_t->ID,      // #0
			p_t->ID2,     // #1
			p_t->Dt,      // #2
			p_t->DtSubst, // #3
			p_t->Name,    // #4
			p_t->Count,   // #5
			dbe_avg_tm,   // #6
			0L).from(p_t, 0L);
		q->orderBy(p_t->DtSubst, p_t->Name, 0L);
	}
	else {
		THROW(CheckTblPtr(sj));
		PPDbqFuncPool::InitObjNameFunc(dbe_user,   PPDbqFuncPool::IdObjNameUser, sj->UserID);
		PPDbqFuncPool::InitObjNameFunc(dbe_action, PPDbqFuncPool::IdSysJActionName, sj->Action);
		PPDbqFuncPool::InitObjNameFunc(dbe_person, PPDbqFuncPool::IdUsrPersonName, sj->UserID); // @v7.5.11
		PPDbqFuncPool::InitLongFunc(dbe_objtitle, PPDbqFuncPool::IdObjTitle, sj->ObjType);
		if(Filt.Flags & SysJournalFilt::fShowObjects && Filt.ActionIDList.getSingle() == PPACN_SCARDDISUPD)
			dbe_extra = & (sj->Extra / 100);
		else
			dbe_extra = & (sj->Extra * 1);
		dbq = & daterange(sj->Dt, &Filt.Period);
		dbq = ppcheckfiltid(dbq, sj->UserID, Filt.UserID);
		dbq = ppcheckfiltidlist(dbq, sj->Action, &Filt.ActionIDList);
		dbq = ppcheckfiltid(dbq, sj->ObjType, Filt.ObjType);
		dbq = ppcheckfiltid(dbq, sj->ObjID, Filt.ObjID);
		dbq = ppcheckweekday(dbq, sj->Dt, Filt.DayOfWeek);
		if(Filt.BegTm)
			dbq = & (*dbq && sj->Tm >= (long)Filt.BegTm);
		if(Filt.Flags & SysJournalFilt::fShowObjects) {
			THROW(CheckTblPtr(nm = P_NamesTbl ? new TempDoubleIDTbl(P_NamesTbl->fileName) : new TempDoubleIDTbl));
			dbq = & (*dbq && (nm->PrmrID == sj->ObjType && (nm->ScndID += sj->ObjID)));
			q = & select(
				sj->ObjType,  // #0
				sj->ObjID,    // #1
				*dbe_extra,   // #2
				sj->Action,   // #3
				sj->Dt,       // #4
				sj->Tm,       // #5
				dbe_user,     // #6
				dbe_action,   // #7
				sj->Extra,    // #8
				dbe_objtitle, // #9
				dbe_person,   // #10
				nm->Name,     // #11 // @v7.5.11 #10-->#11
				0L).from(sj, nm, 0L);
		}
		else
			q = & select(
				sj->ObjType,  // #0
				sj->ObjID,    // #1
				*dbe_extra,   // #2
				sj->Action,   // #3
				sj->Dt,       // #4
				sj->Tm,       // #5
				dbe_user,     // #6
				dbe_action,   // #7
				sj->Extra,    // #8
				dbe_objtitle, // #9
				dbe_person,   // #10
				0L).from(sj, 0L);
		ZDELETE(dbe_extra);
		q->where(*dbq);
		if(Filt.ObjType && Filt.ObjID)
			q->orderBy(sj->ObjType, sj->ObjID, sj->Dt, 0L);
		else
			q->orderBy(sj->Dt, sj->Tm, 0L);
	}
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete p_t;
			delete nm;
			delete sj;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewSysJournal::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr hdr;
		if(!RVALUEPTR(hdr, (PPViewSysJournal::BrwHdr *)pHdr))
			MEMSZERO(hdr);
		if(ppvCmd == PPVCMD_VIEWHISTORY) {
			if(hdr.Obj == PPOBJ_BILL && oneof2(hdr.Action, PPACN_UPDBILL, PPACN_RMVBILL)) {
				ViewBillHistory(R0i(hdr.Extra));
				ok = -1;
			}
		}
		else if(ppvCmd == PPVCMD_VIEWGOODSHISTORY) {
			if(oneof2(hdr.Obj, PPOBJ_GOODS, PPOBJ_GOODSGROUP) && oneof2(hdr.Action, PPACN_OBJUPD, PPACN_OBJRMV)) {
				PPObjGoods goods_obj;
				goods_obj.ViewVersion(R0i(hdr.Extra));
				ok = -1;
			}
		}
		else if(ppvCmd == PPVCMD_RESTOREGOODS) {
			if(oneof2(hdr.Obj, PPOBJ_GOODS, PPOBJ_GOODSGROUP) && oneof2(hdr.Action, PPACN_OBJUPD, PPACN_OBJRMV)) {
				ok = -1;
			}
		}
		else if(ppvCmd == PPVCMD_COMPARE) {
			if(hdr.Obj == PPOBJ_BILL && oneof2(hdr.Action, PPACN_UPDBILL, PPACN_RMVBILL)) {
				ok = -1;
				long   hist_id = R0i(hdr.Extra);
				if(hdr.Id && hist_id) {
					PPIDArray rh_bill_list;
					rh_bill_list.add(hist_id);
					if((ok = ViewGoodsBillCmp(hdr.Id, rh_bill_list, 1, ISHIST_RIGHTBILL)) == 0)
						PPError();
				}
			}
		}
		else if(ppvCmd == PPVCMD_TRANSMIT && (Filt.Flags & SysJournalFilt::fShowObjects)) {
			Transmit();
			ok = -1;
		}
		else if(ppvCmd == PPVCMD_SPCFUNC) {
			ok = -1;
			if(hdr.Obj == PPOBJ_PERSONPOST && hdr.Id && hdr.Action == PPACN_OBJRMV) {
				PPObjStaffList sl_obj;
				PPPsnPostPacket pack;
				pack.Rec.ID = hdr.Id;
				while(sl_obj.EditPostDialog(&pack, PPObjStaffList::epdfRecover) > 0) {
					PPID   id = 0;
					if(sl_obj.PutPostPacket(&id, &pack, 1))
						break;
					else
						PPError();
				}
			}
		}
		else if(ppvCmd == PPVCMD_REFRESH) {
			ok = -1;
			int    do_refresh = 1;
			if(GetServerInstId() && !(BaseState & bsServerInst)) {
				PPJobSrvClient * p_cli = 0;
				if((p_cli = DS.GetClientSession(0)) != 0) {
					PPJobSrvCmd cmd;
					PPJobSrvReply reply;
					if(cmd.StartWriting(PPSCMD_REFRESHVIEW) && cmd.Write(GetServerInstId())) {
						cmd.FinishWriting();
						if(p_cli->Exec(cmd, reply)) {
							SString reply_buf;
							reply.StartReading(&reply_buf);
							if(reply.CheckRepError()) {
								ok = reply_buf.ToLong();
								do_refresh = 0;
							}
						}
					}
				}
			}
			if(do_refresh) {
				LDATETIME since = LastRefreshDtm;
				LastRefreshDtm = getcurdatetime_();
				if(IsTempTblNeeded()) {
					if(RefreshTempTable(since) > 0)
						ok = 1;
				}
				else
					ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPViewSysJournal::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	BrwHdr hdr;
	if(!RVALUEPTR(hdr, (PPViewSysJournal::BrwHdr *)pHdr))
		MEMSZERO(hdr);
	if(Filt.Sgsj != sgsjNone || Filt.Sgd != sgdNone) {
		int    r = 1;
		PPID   id = hdr.Obj, id2 = hdr.Id;
		SysJournalFilt   f;
		PPViewSysJournal v;
		f = Filt;
		f.Sgsj = sgsjNone;
		f.Sgd  = sgdNone;
		if(Filt.Sgd != sgdNone) {
			LDATE dt = ZERODATE;
			TempSysJournalTbl::Key0 k0;
			MEMSZERO(k0);
			k0.ID  = id;
			k0.ID2 = id2;
			if(P_SubstTbl->search(0, &k0, spEq) > 0) {
				LTIME tm;
				TimeRange tm_prd;
				dt = P_SubstTbl->data.Dt;
				tm.v = dt.v;
				if(Filt.Sgd == sgdWeekDay)
					f.DayOfWeek = (int16)dt.v;
				else {
					ExpandSubstDateExt(Filt.Sgd, dt, tm, &f.Period, &tm_prd);
					f.BegTm = tm_prd.low;
				}
			}
			else
				r = 0;
		}
		if(r > 0) {
			if(Filt.Sgsj == sgsjOp)
				f.ActionIDList.add(id);
			else if(Filt.Sgsj == sgsjUser)
				f.UserID = id;
			else if(Filt.Sgsj == sgsjObjType)
				f.ObjType = id;
			PPView::Execute(PPVIEW_SYSJOURNAL, &f, 0, 0);
		}
	}
	else if(oneof2(hdr.Action, PPACN_OBJTAGADD, PPACN_OBJTAGUPD)) {
		ObjTagItem tag_item;
		if(PPRef->Ot.GetTag(hdr.Obj, hdr.Id, (PPID)hdr.Extra, &tag_item) > 0) {
			EditObjTagItem(hdr.Obj, hdr.Id, &tag_item, 0);
		}
		else {
			EditObj((PPObjID*)&hdr);
		}
	}
	else if(!oneof2(hdr.Action, PPACN_OBJRMV, PPACN_RMVBILL))
		EditObj((PPObjID*)&hdr);
	return -1;
}

int SLAPI PPViewSysJournal::Print(const void *)
{
	uint rpt_id = (Filt.Flags & SysJournalFilt::fShowObjects) ? REPORT_SYSJOBJ : REPORT_SYSJ;
	if(Filt.Sgsj != sgsjNone || Filt.Sgd != sgdNone) {
		if(Filt.Sgsj != sgsjNone && Filt.Sgd != sgdNone)
            rpt_id = REPORT_SYSJSUBST2;
		else
			rpt_id = REPORT_SYSJSUBST;
	}
	return Helper_Print(rpt_id, 0);
}

int SLAPI PPViewSysJournal::EditObj(PPObjID * pObjID)
{
	return pObjID ? EditPPObj(pObjID->Obj, pObjID->Id) : -1;
}

int SLAPI PPViewSysJournal::ViewBillHistory(PPID histID)
{
	int    ok = -1;
	if(histID) {
		HistBillCore hb_core;
		PPBillPacket pack;
		PPHistBillPacket hb_pack;
		THROW(hb_core.GetPacket(histID, &hb_pack) > 0);
		THROW(hb_pack.ConvertToBillPack(&pack));
		THROW(::EditGoodsBill(&pack, PPObjBill::efNoUpdNotif));
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewSysJournal::Transmit()
{
	int    ok = -1;
	LockUpByNotify = 1;
	if(P_NamesTbl) {
		ObjTransmitParam param;
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			BExtQuery   nmq(P_NamesTbl, 0);
			IterCounter cntr;
			TempDoubleIDTbl::Key1  k0;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			PPWait(1);
			nmq.select(P_NamesTbl->PrmrID, P_NamesTbl->ScndID, 0L);
			cntr.Init(P_NamesTbl);
			MEMSZERO(k0);
			for(nmq.initIteration(0, &k0, spGt); nmq.nextIteration() > 0; cntr.Increment()) {
				PPObjID oi;
				oi.Set(P_NamesTbl->data.PrmrID, P_NamesTbl->data.ScndID);
				if(!objid_ary.lsearch(&oi, 0, PTR_CMPFUNC(_2long)))
					objid_ary.Add(oi.Obj, oi.Id);
				PPWaitPercent(cntr);
			}
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	PPWait(0);
	LockUpByNotify = 0;
	return ok;
}

int SLAPI PPViewSysJournal::RefreshTempTable(LDATETIME since)
{
	int    ok = -1;
	if((P_TmpTbl || P_NamesTbl) && IsTempTblNeeded()) {
		SysJournalTbl::Key0 k0;
		k0.Dt = since.d;
		k0.Tm = since.t;
		BExtQuery q(P_Tbl, 0, 128);
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		q.selectAll().where(P_Tbl->Dt >= since.d);
		for(q.initIteration(0, &k0, spGt); q.nextIteration() > 0;) {
			if(cmp(since, P_Tbl->data.Dt, P_Tbl->data.Tm) < 0) {
				SysJournalTbl::Rec rec;
				P_Tbl->copyBufTo(&rec);
				MEMSZERO(k0);
				k0.Dt = rec.Dt;
				k0.Tm = rec.Tm;
				if(CheckRecForFilt(&rec)) {
					if(P_TmpTbl) {
						if(P_TmpTbl->searchForUpdate(0, &k0, spEq) > 0) {
							THROW_DB(P_TmpTbl->updateRecBuf(&rec)); // @sfu
						}
						else {
							THROW_DB(P_TmpTbl->insertRecBuf(&rec));
						}
						// @v8.5.11 ok = 1;
					}
					if(P_NamesTbl) {
						TempDoubleIDTbl::Key1 k1;
						const PPID obj_type = rec.ObjType;
						const PPID obj_id = rec.ObjID;
						k1.ScndID = obj_id;
						k1.PrmrID = obj_type;
						{
							char   name_buf[256];
							PPObject * ppobj = P_ObjColl->GetObjectPtr(obj_type);
							name_buf[0] = 0;
							CALLPTRMEMB(ppobj, GetName(obj_id, name_buf, sizeof(name_buf)));
							if(!P_NamesTbl->search(1, &k1, spEq)) {
								TempDoubleIDTbl::Rec nm_rec;
								MEMSZERO(nm_rec);
								nm_rec.PrmrID = obj_type;
								nm_rec.ScndID = obj_id;
								STRNSCPY(nm_rec.Name, name_buf);
								THROW_DB(P_NamesTbl->insertRecBuf(&nm_rec));
								// @v8.5.11 ok = 1;
							}
							else if(strcmp(P_NamesTbl->data.Name, name_buf) != 0) {
								STRNSCPY(P_NamesTbl->data.Name, name_buf);
								THROW_DB(P_NamesTbl->updateRec());
							}
						}
	 				}
					ok = 1; // @v8.5.11
				}
				else if(P_TmpTbl && P_TmpTbl->searchForUpdate(0, &k0, spEq) > 0) {
					THROW_DB(P_TmpTbl->deleteRec()); // @sfu
					ok = 1;
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewSysJournal::HandleNotifyEvent(int kind, const PPNotifyEvent * pEv, PPViewBrowser * pBrw, void * extraProcPtr)
{
	int    ok = -1, update = 0;
	if(!LockUpByNotify) {
		LDATETIME last_dtm;
		if(pEv) {
			if(pEv->IsFinish() && kind == PPAdviseBlock::evSysJournalChanged) {
				// @v8.9.11 last_dtm = *(LDATETIME*)pEv->ExtInt;
				last_dtm = pEv->ExtDtm; // @v8.9.11
				ok = ProcessCommand(PPVCMD_REFRESH, 0, 0);
				if(pBrw && ok > 0)
					pBrw->Update();
				update = 1;
			}
		}
	}
	return ok;
}

//virtual
int SLAPI PPViewSysJournal::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw && Filt.Flags & SysJournalFilt::fShowObjects && Filt.ActionIDList.getSingle() == PPACN_SCARDDISUPD)
		pBrw->InsColumnWord(7, PPWORD_PCTDIS, 2, 0, MKSFMTD(0, 2, NMBF_NOZERO | NMBF_NOTRAILZ), 0);
	CALLPTRMEMB(pBrw, Advise(PPAdviseBlock::evSysJournalChanged, 0, -1, 0));
	return 1;
}
//
//
//
int SLAPI ViewSysJournal(PPID objType, PPID objID, int _modeless)
{
	SysJournalFilt flt;
	flt.ObjType = objType;
	flt.ObjID = objID;
	return ViewSysJournal(&flt, _modeless);
}

int SLAPI ViewSysJournal(const SysJournalFilt * pFilt, int asModeless)
{
	return PPView::Execute(PPVIEW_SYSJOURNAL, pFilt, 1, 0);
}
//
// Implementation of PPALDD_SysJournalEntry
//
PPALDD_CONSTRUCTOR(SysJournalEntry)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
}

PPALDD_DESTRUCTOR(SysJournalEntry)
{
	Destroy();
}

int PPALDD_SysJournalEntry::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.SurID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		size_t data_len = 0;
		MEMSZERO(H);
		const SysJournalTbl::Rec * p_sj_rec = (const SysJournalTbl::Rec *)DS.GetTLA().SurIdList.Get(rFilt.ID, &data_len);
		if(p_sj_rec && data_len == sizeof(*p_sj_rec)) {
			H.SurID = rFilt.ID;
			H.UserID  = p_sj_rec->UserID;
			H.ObjType = p_sj_rec->ObjType;
			H.ObjID   = p_sj_rec->ObjID;
			H.ActionID = p_sj_rec->Action;
			H.Dt       = p_sj_rec->Dt;
			H.Tm       = p_sj_rec->Tm;
			H.Extra    = p_sj_rec->Extra; // @v8.1.11
			SString temp_buf;
			if(PPLoadString(PPSTR_ACTION, H.ActionID, temp_buf))
				temp_buf.CopyTo(H.ActionName, sizeof(H.ActionName));
			else
				ltoa(H.ActionID, H.ActionName, 10);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_SysJournal
//
PPALDD_CONSTRUCTOR(SysJournal)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(SysJournal)
{
	Destroy();
}

int PPALDD_SysJournal::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(SysJournal, rsrv);
	H.FltBeg          = p_filt->Period.low;
	H.FltEnd          = p_filt->Period.upp;
	H.FltUserID       = p_filt->UserID;
	H.FltObjType      = p_filt->ObjType;
	H.FltAction       = p_filt->ActionIDList.getSingle();
	H.FltSysJSubst    = p_filt->Sgsj;
	H.FltDateSubst    = p_filt->Sgd;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_SysJournal::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(SysJournal);
}

int PPALDD_SysJournal::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(SysJournal);
	I.UserID   = item.UserID;
	I.ObjType  = item.ObjType;
	I.ObjID    = item.ObjID;
	I.ActionID = item.Action;
	I.Dt       = item.Dt;
	I.Tm       = item.Tm;
	I.GrpCount = item.GrpCount;
	item.AvgEvTime.CopyTo(I.AvgEvTime, sizeof(I.AvgEvTime));
	item.GrpText1.CopyTo(I.GrpText1, sizeof(I.GrpText1));
	SString temp_buf;
	if(PPLoadString(PPSTR_ACTION, I.ActionID, temp_buf))
		temp_buf.CopyTo(I.ActionName, sizeof(I.ActionName));
	else
		ltoa(item.Action, I.ActionName, 10);
	STRNSCPY(I.ObjName, item.ObjName);
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_SysJournal::Destroy()
{
	DESTROY_PPVIEW_ALDD(SysJournal);
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(GtaJournal); SLAPI GtaJournalFilt::GtaJournalFilt() : PPBaseFilt(PPFILT_GTAJOURNAL, 0, 0)
{
	SetFlatChunk(offsetof(GtaJournalFilt, ReserveStart),
		offsetof(GtaJournalFilt, ActionIDList)-offsetof(GtaJournalFilt, ReserveStart));
	SetBranchSArray(offsetof(GtaJournalFilt, ActionIDList));
	Init(1, 0);
}

int  SLAPI GtaJournalFilt::IsEmpty() const
{
	if(!Period.IsZero())
		return 0;
	else if(GlobalUserID)
		return 0;
	else if(Oi.Obj)
		return 0;
	else if(ActionIDList.getCount())
		return 0;
	else
		return 1;
}

SLAPI PPViewGtaJournal::PPViewGtaJournal() : PPView(0, &Filt, PPVIEW_GTAJOURNAL)
{
	P_TmpTbl = 0;
	P_NamesTbl = 0;
	P_ObjColl = new ObjCollection;
	LockUpByNotify = 0;
	LastRefreshDtm.SetZero();
}

SLAPI PPViewGtaJournal::~PPViewGtaJournal()
{
	delete P_ObjColl;
	delete P_TmpTbl;
	delete P_NamesTbl;
}

PPBaseFilt * SLAPI PPViewGtaJournal::CreateFilt(void * extraPtr) const
{
	GtaJournalFilt * p_filt = new GtaJournalFilt;
	p_filt->Period.SetDate(getcurdate_());
	return p_filt;
}

int SLAPI PPViewGtaJournal::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class GtaJFiltDialog : public TDialog {
	public:
		GtaJFiltDialog() : TDialog(DLG_GTAJFILT)
		{
			SetupCalPeriod(CTLCAL_GTAJFILT_PERIOD, CTL_GTAJFILT_PERIOD);
		}
		int    setDTS(const GtaJournalFilt * pData)
		{
			Data = *pData;
			SetPeriodInput(this, CTL_GTAJFILT_PERIOD, &Data.Period);
			SetupPPObjCombo(this, CTLSEL_GTAJFILT_USER, PPOBJ_GLOBALUSERACC, Data.GlobalUserID, 0, 0);
			SetupPPObjCombo(this, CTLSEL_GTAJFILT_ACTION, PPOBJ_GTACTION, 0, 0, 0);
			if(Id == DLG_GTAJFILT) {
				SetupObjListCombo(this, CTLSEL_GTAJFILT_OBJ, Data.Oi.Obj);
				AddClusterAssoc(CTL_GTAJFILT_FLAGS, 0, SysJournalFilt::fShowObjects);
				SetClusterData(CTL_GTAJFILT_FLAGS, Data.Flags);
			}
			if(Data.ActionIDList.isList()) {
				SetComboBoxListText(this, CTLSEL_GTAJFILT_ACTION);
				disableCtrl(CTLSEL_GTAJFILT_ACTION, 1);
			}
			else {
				setCtrlLong(CTLSEL_GTAJFILT_ACTION, Data.ActionIDList.getSingle());
				disableCtrl(CTLSEL_GTAJFILT_ACTION, 0);
			}
			return 1;
		}
		int    getDTS(GtaJournalFilt * pData)
		{
			if(!GetPeriodInput(this, CTL_GTAJFILT_PERIOD, &Data.Period))
				return PPErrorByDialog(this, CTL_GTAJFILT_PERIOD);
			else {
				getCtrlData(CTLSEL_GTAJFILT_USER, &Data.GlobalUserID);
				if(!Data.ActionIDList.isList())
					Data.ActionIDList.setSingleNZ(getCtrlLong(CTLSEL_GTAJFILT_ACTION));
				getCtrlData(CTLSEL_GTAJFILT_OBJ, &Data.Oi.Obj);
				GetClusterData(CTL_GTAJFILT_FLAGS, &Data.Flags);
				*pData = Data;
				return 1;
			}
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmSysJActionList)) {
				ListToListData l2l_data(PPOBJ_GTACTION, (void *)-1, &Data.ActionIDList);
				l2l_data.TitleStrID = 0; // PPTXT_XXX;
				if(ListToListDialog(&l2l_data) > 0)
					if(Data.ActionIDList.isList()) {
						SetComboBoxListText(this, CTLSEL_GTAJFILT_ACTION);
						disableCtrl(CTLSEL_GTAJFILT_ACTION, 1);
					}
					else {
						setCtrlLong(CTLSEL_GTAJFILT_ACTION, Data.ActionIDList.getSingle());
						disableCtrl(CTLSEL_GTAJFILT_ACTION, 0);
					}
			}
			else if(event.isCbSelected(CTLSEL_GTAJFILT_ACTION))
				Data.ActionIDList.setSingleNZ(getCtrlLong(CTLSEL_GTAJFILT_ACTION));
			else
				return;
			clearEvent(event);
		}
		GtaJournalFilt Data;
	};
	if(!Filt.IsA(pBaseFilt))
		return 0;
	DIALOG_PROC_BODY(GtaJFiltDialog, (GtaJournalFilt *)pBaseFilt);
}

int SLAPI PPViewGtaJournal::IsTempTblNeeded() const
{
	return BIN(Filt.ActionIDList.isList() || (Filt.Flags & SysJournalFilt::fShowObjects));
}

PP_CREATE_TEMP_FILE_PROC(CreateTempGtaFile, GtaJournal);

int SLAPI PPViewGtaJournal::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	BExtInsert * p_bei = 0;
	Counter.Init();
	ZDELETE(P_TmpTbl);
	ZDELETE(P_NamesTbl);
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	LastRefreshDtm = getcurdatetime_();
	if(IsTempTblNeeded()) {
		if(InitIteration()) {
			uint   i = 0;
			if(Filt.ActionIDList.isList()) {
				THROW(P_TmpTbl = CreateTempGtaFile());
				THROW_MEM(p_bei = new BExtInsert(P_TmpTbl));
			}
			if(Filt.Flags & GtaJournalFilt::fShowObjects) {
				THROW(P_NamesTbl = CreateNamesFile());
			}
			{
				PPTransaction tra(ppDbDependTransaction, 1);
				THROW(tra);
				while(P_IterQuery->nextIteration() > 0) {
					PPID id = 0;
					THROW(PPCheckUserBreak());
					if(p_bei)
						THROW_DB(p_bei->insert(&T.data));
					if(P_NamesTbl && T.data.ObjID) {
						TempDoubleIDTbl::Key1 k1;
						k1.ScndID = T.data.ObjID;
						k1.PrmrID = T.data.ObjType;
						if(!P_NamesTbl->search(1, &k1, spEq)) {
							TempDoubleIDTbl::Rec  nm_rec;
							MEMSZERO(nm_rec);
							PPObject * ppobj = P_ObjColl->GetObjectPtr(T.data.ObjType);
							nm_rec.PrmrID = T.data.ObjType;
							nm_rec.ScndID = T.data.ObjID;
							CALLPTRMEMB(ppobj, GetName(nm_rec.ScndID, nm_rec.Name, sizeof(nm_rec.Name)));
							THROW_DB(P_NamesTbl->insertRecBuf(&nm_rec));
						}
					}
					Counter.Increment();
					PPWaitPercent(Counter);
				}
				if(p_bei)
					THROW_DB(p_bei->flash());
				THROW(tra.Commit());
			}
		}
	}
	CATCH
		ok = 0;
		ZDELETE(P_NamesTbl);
	ENDCATCH
	ZDELETE(P_IterQuery);
	ZDELETE(p_bei);
	if(ok == 0)
		ZDELETE(P_TmpTbl);
	return ok;
}

int SLAPI PPViewGtaJournal::InitIteration()
{
	int    ok = 1;
	int    idx = 0;
	DBQ  * dbq = 0;
	union {
		GtaJournalTbl::Key0 k0;
		GtaJournalTbl::Key3 k3;
	} k, ks;
	MEMSZERO(k);
	ZDELETE(P_IterQuery);
	if(P_TmpTbl) {
		P_IterQuery = new BExtQuery(P_TmpTbl, 0);
		P_IterQuery->selectAll();
	}
	else {
		if(Filt.Oi.Obj && Filt.Oi.Id)
			idx = 3;
		else
			idx = 0;
		P_IterQuery = new BExtQuery(&T, idx);
		P_IterQuery->selectAll();
		dbq = & daterange(T.Dt, &Filt.Period);
		dbq = ppcheckfiltid(dbq, T.ObjType, Filt.Oi.Obj);
		dbq = ppcheckfiltid(dbq, T.ObjID, Filt.Oi.Id);
		dbq = ppcheckfiltid(dbq, T.GlobalAccID, Filt.GlobalUserID);
		dbq = ppcheckfiltidlist(dbq, T.Op, &Filt.ActionIDList);
		P_IterQuery->where(*dbq);
		if(idx == 0) {
			k.k0.Dt = Filt.Period.low;
			k.k0.Tm = Filt.BegTm;
		}
		else {
			k.k3.ObjType = Filt.Oi.Obj;
			k.k3.ObjID   = Filt.Oi.Id;
			k.k3.Dt      = Filt.Period.low;
			k.k3.Tm      = Filt.BegTm;
		}
	}
	ks = k;
	Counter.Init(P_IterQuery->countIterations(0, &ks, spGe));
	P_IterQuery->initIteration(0, &k, spGe);
	return ok;
}

int FASTCALL PPViewGtaJournal::NextIteration(GtaJournalViewItem * pItem)
{
	while(pItem && P_IterQuery && P_IterQuery->nextIteration() > 0) {
		GtaJournalTbl * p_t = NZOR(P_TmpTbl, &T);
		p_t->copyBufTo(pItem);
		pItem->ObjName = 0;
		if(p_t->data.ObjID && P_NamesTbl) {
			TempDoubleIDTbl::Key1  k1;
			k1.ScndID = p_t->data.ObjID;
			k1.PrmrID = p_t->data.ObjType;
			if(P_NamesTbl->search(1, &k1, spEq))
				pItem->ObjName = P_NamesTbl->data.Name;
		}
		if(!Filt.BegTm || p_t->data.Dt > Filt.Period.low || p_t->data.Tm >= Filt.BegTm) {
			Counter.Increment();
			PPWaitPercent(Counter);
			return 1;
		}
	}
	return -1;
}

DBQuery * SLAPI PPViewGtaJournal::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	int    add_dbe = 0;
	uint   brw_id = (Filt.Flags & GtaJournalFilt::fShowObjects) ? BROWSER_GTAJ_OBJ : BROWSER_GTAJ;
	DBQuery * q = 0;
	DBE    dbe_user;
	DBE    dbe_objtitle;
	DBE    dbe_action;
	DBQ  * dbq = 0;
	GtaJournalTbl * t  = new GtaJournalTbl(P_TmpTbl ? (const char *)P_TmpTbl->fileName : (const char *)0);
	TempDoubleIDTbl * nm  = 0;

	THROW(CheckTblPtr(t));
	PPDbqFuncPool::InitObjNameFunc(dbe_user,   PPDbqFuncPool::IdObjNameGlobalUser, t->GlobalAccID);
	PPDbqFuncPool::InitObjNameFunc(dbe_action, PPDbqFuncPool::IdGtaJActionName, t->Op);
	PPDbqFuncPool::InitLongFunc(dbe_objtitle, PPDbqFuncPool::IdObjTitle, t->ObjType);
	dbq = & daterange(t->Dt, &Filt.Period);
	dbq = ppcheckfiltid(dbq, t->GlobalAccID, Filt.GlobalUserID);
	dbq = ppcheckfiltidlist(dbq, t->Op, &Filt.ActionIDList);
	dbq = ppcheckfiltid(dbq, t->ObjType, Filt.Oi.Obj);
	dbq = ppcheckfiltid(dbq, t->ObjID, Filt.Oi.Id);
	dbq = ppcheckweekday(dbq, t->Dt, Filt.DayOfWeek);
	if(Filt.BegTm)
		dbq = & (*dbq && t->Tm >= (long)Filt.BegTm);
	if(Filt.Flags & GtaJournalFilt::fShowObjects) {
		THROW(CheckTblPtr(nm = P_NamesTbl ? new TempDoubleIDTbl(P_NamesTbl->fileName) : new TempDoubleIDTbl));
		dbq = & (*dbq && (nm->PrmrID == t->ObjType && (nm->ScndID += t->ObjID)));
		q = & select(
			t->ObjType,   // #0
			t->ObjID,     // #1
			t->Op,        // #2
			t->Dt,        // #3
			t->Tm,        // #4
			dbe_user,     // #5
			dbe_action,   // #6
			dbe_objtitle, // #7
			nm->Name,     // #8
			0L).from(t, nm, 0L);
	}
	else
		q = & select(
			t->ObjType,   // #0
			t->ObjID,     // #1
			t->Op,        // #2
			t->Dt,        // #3
			t->Tm,        // #4
			dbe_user,     // #5
			dbe_action,   // #6
			dbe_objtitle, // #7
			0L).from(t, 0L);
	q->where(*dbq);
	if(Filt.Oi.Obj && Filt.Oi.Id)
		q->orderBy(t->ObjType, t->ObjID, t->Dt, 0L);
	else
		q->orderBy(t->Dt, t->Tm, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete nm;
			delete t;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewGtaJournal::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pHdr) {
		BrwHdr hdr = *(PPViewGtaJournal::BrwHdr *)pHdr;
		ok = EditPPObj(hdr.Obj, hdr.Id);
	}
	return ok;
}

