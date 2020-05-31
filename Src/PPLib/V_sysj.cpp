// V_SYSJ.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
//
#include <pp.h>
#pragma hdrstop

static IMPL_CMPFUNC(PPViewSysJournal_EvVerEntry, i1, i2)
{
	int    si = memcmp(i1, i2, sizeof(PPObjID));
	SETIFZ(si, cmp(static_cast<const PPViewSysJournal::EvVerEntry *>(i1)->Dtm, static_cast<const PPViewSysJournal::EvVerEntry *>(i2)->Dtm));
	return si;
}

void SysJournalViewItem::Clear()
{
	memzero(this, sizeof(SysJournalTbl::Rec));
	ID = 0;
	GrpCount = 0;
	ObjName.Z();
	GrpText1.Z();
	AvgEvTime.Z();
}

IMPLEMENT_PPFILT_FACTORY(SysJournal); SLAPI SysJournalFilt::SysJournalFilt() : PPBaseFilt(PPFILT_SYSJOURNAL, 0, 2)
{
	SetFlatChunk(offsetof(SysJournalFilt, ReserveStart),
		offsetof(SysJournalFilt, ActionIDList)-offsetof(SysJournalFilt, ReserveStart));
	SetBranchSVector(offsetof(SysJournalFilt, ActionIDList)); // @v9.8.4 SetBranchSArray-->SetBranchSVector
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
		ListToListData l2l_data(PPOBJ_ACTION, reinterpret_cast<void *>(-1), &Filt.ActionIDList);
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
		SetupCtrls();
	else
		return;
	clearEvent(event);
}

void SysJFiltDialog::SetupCtrls()
{
	int    subst_used = 0;
	getCtrlData(CTLSEL_SYSJFILT_SUBST, &Filt.Sgsj);
	getCtrlData(CTLSEL_SYSJFILT_SUBSTDT, &Filt.Sgd);
	subst_used = BIN(Filt.Sgsj != sgsjNone || Filt.Sgd != sgdNone);
	DisableClusterItem(CTL_SYSJFILT_FLAGS, 0, subst_used);
	DisableClusterItem(CTL_SYSJFILT_FLAGS, 1, subst_used); // @v9.9.2
	if(subst_used) {
		Filt.Flags &= ~SysJournalFilt::fShowObjects;
		SetClusterData(CTL_SYSJFILT_FLAGS, Filt.Flags);
	}
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
		AddClusterAssoc(CTL_SYSJFILT_FLAGS, 1, SysJournalFilt::fShowHistoryObj); // @v9.9.2
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
	SetupCtrls();
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
	DIALOG_PROC_BODY_P1(SysJFiltDialog, DLG_SYSJFILT, static_cast<SysJournalFilt *>(pBaseFilt));
}

SLAPI PPViewSysJournal::PPViewSysJournal() : PPView(0, &Filt, PPVIEW_SYSJOURNAL), P_TmpTbl(0), P_SubstTbl(0), /*P_NamesTbl(0),*/
	LockUpByNotify(0), LastRefreshDtm(ZERODATETIME), P_Tbl(new SysJournal), P_ObjColl(new ObjCollection)
{
	ImplementFlags |= implUseServer;
}

SLAPI PPViewSysJournal::~PPViewSysJournal()
{
	delete P_Tbl;
	delete P_TmpTbl;
	delete P_SubstTbl;
	// @v9.9.0 delete P_NamesTbl;
	delete P_ObjColl;
}

int SLAPI PPViewSysJournal::SerializeState(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(PPView::SerializeState(dir, rBuf, pCtx));
	THROW(SerializeDbTableByFileName <SysJournalTbl> (dir, &P_TmpTbl, rBuf, pCtx));
	// @v9.9.0 THROW(SerializeDbTableByFileName <TempDoubleIDTbl>  (dir, &P_NamesTbl, rBuf, pCtx));
	THROW_SL(StrPool.SerializeS(dir, rBuf, pCtx));
	THROW_SL(pCtx->Serialize(dir, &ObjNameList, rBuf));
	THROW_SL(pCtx->Serialize(dir, &EvVerList, rBuf)); // @v9.9.2
	CATCHZOK
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, SysJournal);
PP_CREATE_TEMP_FILE_PROC(CreateSubstFile, TempSysJournal);
// @v9.9.0 PP_CREATE_TEMP_FILE_PROC(CreateNamesFile, TempDoubleID);

int SLAPI PPViewSysJournal::IsTempTblNeeded() const
{
	return BIN(Filt.ActionIDList.isList() || (Filt.Flags & (SysJournalFilt::fShowObjects|SysJournalFilt::fShowHistoryObj)) || Filt.Sgsj != sgsjNone || Filt.Sgd != sgdNone);
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
	SString temp_buf;
	BExtInsert * p_bei = 0;
	PPIDArray modrmv_acn_list;
	AverageEventTimePrcssr * p_avg_ev_prcssr = 0;
	ObjVersioningCore * p_ovc = 0; // @v9.9.2
	SBuffer ov_buf; // @v9.9.2
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	Counter.Init();
	ZDELETE(P_TmpTbl);
	// @v9.9.0 ZDELETE(P_NamesTbl);
	ZDELETE(P_SubstTbl);
	THROW(Helper_InitBaseFilt(pFilt));
	// @v9.9.2 {
	if(Filt.Flags & Filt.fShowHistoryObj) {
		p_ovc = PPRef->P_OvT;
		if(p_ovc->InitSerializeContext(1))
			modrmv_acn_list.addzlist(PPACN_OBJUPD, PPACN_OBJRMV, PPACN_UPDBILL, PPACN_RMVBILL, 0);
		else
			p_ovc = 0;
	}
	// } @v9.9.2
	StrPool.ClearS(); // @v9.9.0
	ObjNameList.clear(); // @v9.9.0
	EvVerList.clear(); // @v9.9.2
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
				// @v9.9.0 if(Filt.Flags & SysJournalFilt::fShowObjects) { THROW(P_NamesTbl = CreateNamesFile()); }
			}
			{
				PPLoadText(PPTXT_VBLD_SYSJ, msg_buf);
				PPTransaction tra(ppDbDependTransaction, 1);
				THROW(tra);
				while(P_IterQuery->nextIteration() > 0) {
					PPID id = 0;
					const SysJournalTbl::Rec & r_rec = P_Tbl->data;
					THROW(PPCheckUserBreak());
					if(Filt.Sgsj != sgsjNone || Filt.Sgd != sgdNone) {
						if(CheckRecForFilt(&r_rec) > 0) {
							TempSysJournalTbl::Key0 k0;
							TempSysJournalTbl::Rec temp_rec;
							// @v10.7.5 @ctr MEMSZERO(temp_rec);
							// temp_rec.ID  = r_rec.Dt.v;
							// temp_rec.ID2 = r_rec.Tm.v;
							P_Tbl->Subst(Filt.Sgsj, r_rec.Action, r_rec.UserID, r_rec.ObjType, &temp_rec.ID);
					 		if(Filt.Sgd != sgdNone) {
								LTIME tm = ZEROTIME;
								LDATE dt = ZERODATE;
								ShrinkSubstDateExt(Filt.Sgd, r_rec.Dt, r_rec.Tm, &dt, &tm);
								FormatSubstDateExt(Filt.Sgd, dt, tm, temp_buf.Z());
								// @v9.9.0 temp_buf.CopyTo(temp_rec.DtSubst, sizeof(temp_rec.DtSubst));
								StrPool.AddS(temp_buf, &temp_rec.DtSubstP); // @v9.9.0
								dt.v = (Filt.Sgd == sgdHour) ? tm.v : dt.v;
								temp_rec.Dt  = dt;
								temp_rec.ID2 = dt.v;
					 		}
							else {
								// @v9.9.0 datefmt(&temp_rec.Dt, DATF_DMY, temp_rec.DtSubst);
								temp_buf.Z().Cat(temp_rec.Dt, DATF_DMY); // @v9.9.0
								StrPool.AddS(temp_buf, &temp_rec.DtSubstP); // @v9.9.0
								temp_rec.Dt = r_rec.Dt;
							}
							// @v9.9.0 P_Tbl->GetSubstName(Filt.Sgsj, temp_rec.ID, temp_rec.Name, sizeof(temp_rec.Name));
							P_Tbl->GetSubstName(Filt.Sgsj, temp_rec.ID, temp_buf); // @v9.9.0
							StrPool.AddS(temp_buf, &temp_rec.NameP); // @v9.9.0
							k0.ID  = temp_rec.ID;
							k0.ID2 = temp_rec.ID2;
							temp_rec.Count = 1;
							if(P_SubstTbl->searchForUpdate(0, &k0, spEq) > 0) {
								P_SubstTbl->data.Count++;
								P_SubstTbl->updateRec();
							}
							else
								P_SubstTbl->insertRecBuf(&temp_rec);
							CALLPTRMEMB(p_avg_ev_prcssr, Add(temp_rec.ID, temp_rec.ID2, r_rec.Dt, r_rec.Tm));
						}
					}
					else {
						if(Filt.Flags & Filt.fShowObjects) {
							if(!oneof2(r_rec.Action, PPACN_OBJRMV, PPACN_RMVBILL)) { // @v9.9.2
								PPObjNamePEntry objn_entry;
								objn_entry.Set(r_rec.ObjType, r_rec.ObjID);
								if(objn_entry.Obj) {
									objn_entry.NameP = 0;
									uint   objn_pos = 0;
									if(!ObjNameList.bsearch(&objn_entry, &objn_pos, PTR_CMPFUNC(PPObjID))) {
										char   name_buf[256];
										PPObject * ppobj = P_ObjColl->GetObjectPtr(objn_entry.Obj);
										if(ppobj && ppobj->GetName(objn_entry.Id, name_buf, sizeof(name_buf)) > 0) {
											temp_buf = name_buf;
											StrPool.AddS(temp_buf, &objn_entry.NameP);
											ObjNameList.ordInsert(&objn_entry, 0, PTR_CMPFUNC(PPObjID));
										}
									}
								}
							}
						}
						// @v9.9.2
						if(Filt.Flags & Filt.fShowHistoryObj && p_ovc && r_rec.Extra) {
							temp_buf.Z();
							if(oneof4(r_rec.Action, PPACN_OBJRMV, PPACN_RMVBILL, PPACN_OBJUPD, PPACN_UPDBILL) && 
								oneof4(r_rec.ObjType, PPOBJ_BILL, PPOBJ_GOODS, PPOBJ_PERSON, PPOBJ_SCARD)) {
								SSerializeContext & r_sctx = p_ovc->GetSCtx();
								long   vv = 0;
								EvVerEntry ev_entry;
								MEMSZERO(ev_entry);
								ov_buf.Z();
								if(p_ovc->Search(r_rec.Extra, &ev_entry, &vv, &ov_buf) > 0 && ev_entry.IsEqual(r_rec.ObjType, r_rec.ObjID)) {
									PPObject * ppobj = P_ObjColl->GetObjectPtr(ev_entry.Obj);
									if(ev_entry.Obj == PPOBJ_BILL) {
										PPBillPacket pack;
										PPObjBill * p_bobj = static_cast<PPObjBill *>(ppobj);
										if(p_bobj->SerializePacket__(-1, &pack, ov_buf, &r_sctx)) {
											pack.ProcessFlags |= (PPBillPacket::pfZombie|PPBillPacket::pfUpdateProhibited); // @v9.9.12
											PPObjBill::MakeCodeString(&pack.Rec, PPObjBill::mcsAddObjName|PPObjBill::mcsAddOpName, temp_buf);
											if(r_rec.Action == PPACN_RMVBILL)
												ev_entry.Flags |= ev_entry.fAmtDn;
											else {
												BillTbl::Rec next_bill_rec;
												next_bill_rec.ID = 0;
												SysJournalTbl::Rec next_rec;
												if(p_sj && p_sj->GetNextObjEvent(ev_entry.Obj, ev_entry.Id, &modrmv_acn_list, ev_entry.Dtm, &next_rec) > 0) {
													assert(next_rec.ObjType == r_rec.ObjType && next_rec.ObjID == r_rec.ObjID);
													PPBillPacket next_pack;
													EvVerEntry next_ev_entry;
													MEMSZERO(next_ev_entry);
													ov_buf.Z();
													if(p_ovc->Search(next_rec.Extra, &next_ev_entry, &vv, &ov_buf) > 0 && ev_entry.IsEqual(next_rec.ObjType, next_rec.ObjID)) {
														if(p_bobj->SerializePacket__(-1, &next_pack, ov_buf, &r_sctx)) {
															next_pack.ProcessFlags |= (PPBillPacket::pfZombie | PPBillPacket::pfUpdateProhibited); // @v9.9.12
															next_bill_rec = next_pack.Rec;
														}
													}
												}
												else if(r_rec.Action != PPACN_RMVBILL) {
													if(p_bobj->Fetch(r_rec.ObjID, &next_bill_rec) <= 0)
														next_bill_rec.ID = 0;
												}
												if(next_bill_rec.ID) {
													if(next_bill_rec.Amount < pack.Rec.Amount)
														ev_entry.Flags |= ev_entry.fAmtDn;
													else if(next_bill_rec.Amount > pack.Rec.Amount)
														ev_entry.Flags |= ev_entry.fAmtUp;
												}
											}
										}
									}
									else if(ev_entry.Obj == PPOBJ_GOODS) {
										PPGoodsPacket pack;
										if(static_cast<PPObjGoods *>(ppobj)->SerializePacket(-1, &pack, ov_buf, &r_sctx, 0)) {
											temp_buf = pack.Rec.Name;
										}
									}
									else if(ev_entry.Obj == PPOBJ_PERSON) {
										PPPersonPacket pack;
										if(static_cast<PPObjPerson *>(ppobj)->SerializePacket(-1, &pack, ov_buf, &r_sctx)) {
											temp_buf = pack.Rec.Name;
										}
									}
									else if(ev_entry.Obj == PPOBJ_SCARD) {
										PPSCardPacket pack;
										if(static_cast<PPObjSCard *>(ppobj)->SerializePacket(-1, &pack, ov_buf, &r_sctx)) {
											temp_buf = pack.Rec.Code;
										}
									}
								}
								if(temp_buf.NotEmpty()) {
									ev_entry.Dtm.Set(r_rec.Dt, r_rec.Tm);
									StrPool.AddS(temp_buf, &ev_entry.NameP);
									EvVerList.ordInsert(&ev_entry, 0, PTR_CMPFUNC(PPViewSysJournal_EvVerEntry));
								}
							}
						}
						// } @v9.9.2
						if(p_bei)
							THROW_DB(p_bei->insert(&r_rec));
						/* @v9.9.0 if(P_NamesTbl && r_rec.ObjID) {
							TempDoubleIDTbl::Key1 k1;
							k1.ScndID = r_rec.ObjID;
							k1.PrmrID = r_rec.ObjType;
							if(!P_NamesTbl->search(1, &k1, spEq)) {
								TempDoubleIDTbl::Rec  nm_rec;
								MEMSZERO(nm_rec);
								PPObject * ppobj = P_ObjColl->GetObjectPtr(r_rec.ObjType);
								nm_rec.PrmrID = r_rec.ObjType;
								nm_rec.ScndID = r_rec.ObjID;
								CALLPTRMEMB(ppobj, GetName(nm_rec.ScndID, nm_rec.Name, sizeof(nm_rec.Name)));
								THROW_DB(P_NamesTbl->insertRecBuf(&nm_rec));
							}
						}*/
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
	ObjNameList.sort(PTR_CMPFUNC(PPObjID)); // @v9.9.0
	CATCHZOK
	BExtQuery::ZDelete(&P_IterQuery);
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
	BExtQuery::ZDelete(&P_IterQuery);
	if(P_SubstTbl) {
		P_IterQuery = new BExtQuery(P_SubstTbl, 1);
		P_IterQuery->selectAll();
		k.tk1.Dt = ZERODATE;
		k.tk1.ID = 0;
		k.tk1.ID2 = 0;
	}
	else if(P_TmpTbl) {
		P_IterQuery = new BExtQuery(P_TmpTbl, 0, 512); // @v10.0.01 0)-->0, 512)
		P_IterQuery->selectAll();
	}
	else {
		if(Filt.ObjType && Filt.ObjID)
			idx = 1;
		else
			idx = 0;
		P_IterQuery = new BExtQuery(P_Tbl, idx, 512); // @v10.0.01 0)-->0, 512)
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
	SString temp_buf;
	while(pItem && P_IterQuery && P_IterQuery->nextIteration() > 0) {
		if(P_SubstTbl) {
			pItem->Clear();
			pItem->ID       = P_SubstTbl->data.ID;
			pItem->Dt       = P_SubstTbl->data.Dt;
			pItem->GrpCount = P_SubstTbl->data.Count;
			// @v9.9.0 pItem->GrpText1 = P_SubstTbl->data.DtSubst;
			// @v9.9.0 STRNSCPY(pItem->ObjName, P_SubstTbl->data.Name);
			StrPool.GetS(P_SubstTbl->data.DtSubstP, pItem->GrpText1); // @v9.9.0
			StrPool.GetS(P_SubstTbl->data.NameP, pItem->ObjName); // @v9.9.0
			{
				LDATETIME dtm = ZERODATETIME;
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
			// @v9.9.0 {
			if(!oneof2(p_t->data.Action, PPACN_OBJRMV, PPACN_RMVBILL)) { // @v9.9.2
				PPObjID oid;
				uint   objn_pos = 0;
				oid.Set(p_t->data.ObjType, p_t->data.ObjID);
				if(ObjNameList.lsearch(&oid, &objn_pos, PTR_CMPFUNC(PPObjID)))
					StrPool.GetS(ObjNameList.at(objn_pos).NameP, pItem->ObjName);
			}
			// } @v9.9.0
			/* @v9.9.0 if(p_t->data.ObjID && P_NamesTbl) {
				TempDoubleIDTbl::Key1  k1;
				k1.ScndID = p_t->data.ObjID;
				k1.PrmrID = p_t->data.ObjType;
				if(P_NamesTbl->search(1, &k1, spEq))
					STRNSCPY(pItem->ObjName, P_NamesTbl->data.Name);
				else
					pItem->ObjName[0] = 0;
			}
			else
				pItem->ObjName[0] = 0; */
			if(!Filt.BegTm || p_t->data.Dt > Filt.Period.low || p_t->data.Tm >= Filt.BegTm) {
				Counter.Increment();
				PPWaitPercent(Counter);
				return 1;
			}
		}
	}
	return -1;
}

int SLAPI PPViewSysJournal::GetObjName(const PPObjID & rOid, SString & rBuf) const
{
	rBuf.Z();
	uint   p = 0;
	if(ObjNameList.bsearch(&rOid, &p, PTR_CMPFUNC(PPObjID))) {
		const PPObjNamePEntry & r_entry = ObjNameList.at(p);
		StrPool.GetS(r_entry.NameP, rBuf);
	}
	return rBuf.NotEmpty() ? 1 : -1;
}

static IMPL_DBE_PROC(dbqf_objnamefromlist_ppvsj_iip)
{
	char   buf[256];
	if(option == CALC_SIZE) {
		result->init(sizeof(buf));
	}
	else {
		PPObjID oid;
		oid.Set(params[0].lval, params[1].lval);
		const PPViewSysJournal * p_view = static_cast<const PPViewSysJournal *>(params[2].ptrval);
		if(p_view) {
			SString & r_temp_buf = SLS.AcquireRvlStr(); // @v10.5.3 revolver
			p_view->GetObjName(oid, r_temp_buf);
			STRNSCPY(buf, r_temp_buf);
		}
		else
			buf[0] = 0;
		result->init(buf);
	}
}

int SLAPI PPViewSysJournal::GetEvVerText(const EvVerEntry & rKey, SString & rBuf) const
{
	rBuf.Z();
	uint   p = 0;
	if(EvVerList.bsearch(&rKey, &p, PTR_CMPFUNC(PPViewSysJournal_EvVerEntry))) {
		const EvVerEntry & r_entry = EvVerList.at(p);
		StrPool.GetS(r_entry.NameP, rBuf);
	}
	return rBuf.NotEmpty() ? 1 : -1;
}

static IMPL_DBE_PROC(dbqf_evvertextfromlist_ppvsj_iidtp)
{
	char   buf[256];
	if(option == CALC_SIZE) {
		result->init(sizeof(buf));
	}
	else {
		PPViewSysJournal::EvVerEntry key;
		key.Set(params[0].lval, params[1].lval);
		key.Dtm.Set(params[2].dval, params[3].tval);
		const PPViewSysJournal * p_view = static_cast<const PPViewSysJournal *>(params[4].ptrval);
		if(p_view) {
			SString temp_buf;
			p_view->GetEvVerText(key, temp_buf);
			STRNSCPY(buf, temp_buf);
		}
		else
			buf[0] = 0;
		result->init(buf);
	}
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewSysJournal * p_view = static_cast<PPViewSysJournal *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int SLAPI PPViewSysJournal::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle) {
		const  BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < p_def->getCountI()) {
			const BroColumn & r_col = p_def->at(col);
			const PPViewSysJournal::BrwHdr * p_hdr = static_cast<const PPViewSysJournal::BrwHdr *>(pData);
			if(r_col.OrgOffs == 7) { // Action
				if(oneof2(p_hdr->Action, PPACN_OBJRMV, PPACN_RMVBILL))
					ok = pStyle->SetFullCellColor(LightenColor(GetColorRef(SClrRed), 0.8f));
				else if(oneof2(p_hdr->Action, PPACN_OBJADD, PPACN_TURNBILL))
					ok = pStyle->SetFullCellColor(LightenColor(GetColorRef(SClrBlue), 0.8f));
				else if(oneof2(p_hdr->Action, PPACN_OBJUPD, PPACN_UPDBILL))
					ok = pStyle->SetFullCellColor(LightenColor(GetColorRef(SClrOrange), 0.8f));
			}
			else if(r_col.OrgOffs == 12) { // History column
				if(Filt.Flags & Filt.fShowHistoryObj) {
					uint   p = 0;
					EvVerEntry key;
					key.Set(p_hdr->Obj, p_hdr->Id);
					key.Dtm = p_hdr->Dtm;
					if(EvVerList.bsearch(&key, &p, PTR_CMPFUNC(PPViewSysJournal_EvVerEntry))) {
						const EvVerEntry & r_entry = EvVerList.at(p);
						if(r_entry.Flags & EvVerEntry::fAmtDn)
							ok = pStyle->SetLeftTopCornerColor(GetColorRef(SClrRed));
						else if(r_entry.Flags & EvVerEntry::fAmtUp)
							ok = pStyle->SetLeftTopCornerColor(GetColorRef(SClrGreen));
					}
				}
			}
		}
	}
	return ok;
}

/*virtual*/void SLAPI PPViewSysJournal::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		if(Filt.Flags & SysJournalFilt::fShowObjects) {
			if(Filt.ActionIDList.getSingle() == PPACN_SCARDDISUPD)
				pBrw->InsColumnWord(7, PPWORD_PCTDIS, 2, 0, MKSFMTD(0, 2, NMBF_NOZERO | NMBF_NOTRAILZ), 0);
			pBrw->InsColumn(-1, "@object", 11, 0, 0, 0);
		}
		if(Filt.Flags & SysJournalFilt::fShowHistoryObj) {
			pBrw->InsColumn(-1, "@history", 12, 0, 0, 0);
		}
		pBrw->Advise(PPAdviseBlock::evSysJournalChanged, 0, -1, 0);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

/*static*/int PPViewSysJournal::DynFuncObjNameFromList = DbqFuncTab::RegisterDynR(BTS_STRING, dbqf_objnamefromlist_ppvsj_iip, 3, BTS_INT, BTS_INT, BTS_PTR);;
/*static*/int PPViewSysJournal::DynFuncEvVerTextFromList = DbqFuncTab::RegisterDynR(BTS_STRING, dbqf_evvertextfromlist_ppvsj_iidtp, 5, BTS_INT, BTS_INT, BTS_DATE, BTS_TIME, BTS_PTR);

DBQuery * SLAPI PPViewSysJournal::CreateBrowserQuery(uint * pBrwId, SString *)
{
	int    add_dbe = 0;
	uint   brw_id = BROWSER_SYSJ;
	DBQuery * q = 0;
	DBE    dbe_user;
	DBE    dbe_objtitle;
	DBE    dbe_action;
	DBE    dbe_person;
	DBE  * dbe_extra = 0;
	DBE    dbe_avg_tm;
	DBE    dbe_dtsubst; // @v9.9.0
	DBE    dbe_objname; // @v9.9.0
	DBE    dbe_evvertext; // @v9.9.2
	DBQ  * dbq = 0;
	TempSysJournalTbl * p_t = 0;
	SysJournalTbl * sj  = new SysJournalTbl(P_TmpTbl ? P_TmpTbl->GetName().cptr() : static_cast<const char *>(0));
	if(P_SubstTbl) {
		brw_id = BROWSER_SYSJ_SUBST;
		THROW_MEM(p_t = new TempSysJournalTbl(P_SubstTbl->GetName().cptr()));
		THROW(CheckTblPtr(p_t));
		PPDbqFuncPool::InitObjNameFunc(dbe_avg_tm, PPDbqFuncPool::IdDurationToTime, p_t->Count);
		// @v9.9.0 {
		THROW_MEM(q = new DBQuery);
		q->syntax |= DBQuery::t_select;
		q->addField(p_t->ID);  // #0
		q->addField(p_t->ID2); // #1
		q->addField(p_t->Dt);  // #2
		{
			PPDbqFuncPool::InitStrPoolRefFunc(dbe_dtsubst, p_t->DtSubstP, &StrPool);
			q->addField(dbe_dtsubst); // #3
		}
		{
			PPDbqFuncPool::InitStrPoolRefFunc(dbe_objname, p_t->NameP, &StrPool);
			q->addField(dbe_objname); // #4
		}
		q->addField(p_t->Count); // #5
		q->addField(dbe_avg_tm); // #6
		q->from(p_t, 0L);
		// } @v9.9.0
		/* @v9.9.0 q = & select(
			p_t->ID,      // #0
			p_t->ID2,     // #1
			p_t->Dt,      // #2
			p_t->DtSubst, // #3
			p_t->Name,    // #4
			p_t->Count,   // #5
			dbe_avg_tm,   // #6
			0L).from(p_t, 0L);*/
		// @v9.9.0 q->orderBy(p_t->DtSubst, p_t->Name, 0L);
	}
	else {
		THROW(CheckTblPtr(sj));
		PPDbqFuncPool::InitObjNameFunc(dbe_user,   PPDbqFuncPool::IdObjNameUser, sj->UserID);
		PPDbqFuncPool::InitObjNameFunc(dbe_action, PPDbqFuncPool::IdSysJActionName, sj->Action);
		PPDbqFuncPool::InitObjNameFunc(dbe_person, PPDbqFuncPool::IdUsrPersonName, sj->UserID);
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
			dbq = & (*dbq && sj->Tm >= static_cast<long>(Filt.BegTm));
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
			0L);
		{
			dbe_objname.init();
			dbe_evvertext.init();
			if(Filt.Flags & SysJournalFilt::fShowObjects) {
				//THROW(CheckTblPtr(nm = P_NamesTbl ? new TempDoubleIDTbl(P_NamesTbl->GetName()) : new TempDoubleIDTbl));
				//dbq = & (*dbq && (nm->PrmrID == sj->ObjType && (nm->ScndID += sj->ObjID)));
				dbe_objname.push(sj->ObjType);
				dbe_objname.push(sj->ObjID);
				dbe_objname.push(dbconst(static_cast<const void *>(this)));
				dbe_objname.push(static_cast<DBFunc>(DynFuncObjNameFromList));
			}
			else
				dbe_objname.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
			if(Filt.Flags & SysJournalFilt::fShowHistoryObj) {
				dbe_evvertext.push(sj->ObjType);
				dbe_evvertext.push(sj->ObjID);
				dbe_evvertext.push(sj->Dt);
				dbe_evvertext.push(sj->Tm);
				dbe_evvertext.push(dbconst(static_cast<const void *>(this)));
				dbe_evvertext.push(static_cast<DBFunc>(DynFuncEvVerTextFromList));
			}
			else
				dbe_evvertext.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
		}
		q->addField(dbe_objname);   // #11
		q->addField(dbe_evvertext); // #12
		q->from(sj, 0L);
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
		if(!RVALUEPTR(hdr, static_cast<const PPViewSysJournal::BrwHdr *>(pHdr)))
			MEMSZERO(hdr);
		if(ppvCmd == PPVCMD_VIEWHISTORY) {
			if(hdr.Obj == PPOBJ_BILL && oneof2(hdr.Action, PPACN_UPDBILL, PPACN_RMVBILL)) {
				LDATETIME ev_dtm = hdr.Dtm;
				ViewBillHistory(R0i(hdr.Extra), ev_dtm);
				ok = -1;
			}
			// @v10.5.3 {
			else if(oneof2(hdr.Obj, PPOBJ_GOODS, PPOBJ_GOODSGROUP) && oneof2(hdr.Action, PPACN_OBJUPD, PPACN_OBJRMV)) {
				PPObjGoods goods_obj;
				goods_obj.ViewVersion(R0i(hdr.Extra));
				ok = -1;
			}
			else if(hdr.Obj == PPOBJ_PERSON && oneof2(hdr.Action, PPACN_OBJUPD, PPACN_OBJRMV)) {
				PPObjPerson psn_obj;
				psn_obj.ViewVersion(R0i(hdr.Extra));
				ok = -1;
			}
			else if(hdr.Obj == PPOBJ_SCARD && oneof2(hdr.Action, PPACN_OBJUPD, PPACN_OBJRMV)) {
				PPObjSCard sc_obj;
				sc_obj.ViewVersion(R0i(hdr.Extra));
				ok = -1;
			}
			// } @v10.5.3 
		}
		/* @v10.5.3 else if(ppvCmd == PPVCMD_VIEWGOODSHISTORY) {
			if(oneof2(hdr.Obj, PPOBJ_GOODS, PPOBJ_GOODSGROUP) && oneof2(hdr.Action, PPACN_OBJUPD, PPACN_OBJRMV)) {
				PPObjGoods goods_obj;
				goods_obj.ViewVersion(R0i(hdr.Extra));
				ok = -1;
			}
		}*/
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
					if((ok = ViewGoodsBillCmp(hdr.Id, rh_bill_list, 1, ISHIST_RIGHTBILL, 0, &hdr.Dtm)) == 0)
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
	if(!RVALUEPTR(hdr, static_cast<const PPViewSysJournal::BrwHdr *>(pHdr)))
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
			EditObj(reinterpret_cast<PPObjID *>(&hdr));
		}
	}
	else if(!oneof2(hdr.Action, PPACN_OBJRMV, PPACN_RMVBILL))
		EditObj(reinterpret_cast<PPObjID *>(&hdr));
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

int SLAPI PPViewSysJournal::EditObj(const PPObjID * pObjID)
{
	return pObjID ? EditPPObj(pObjID->Obj, pObjID->Id) : -1;
}

int SLAPI PPViewSysJournal::ViewBillHistory(PPID histID, LDATETIME evDtm)
{
	int    ok = -1;
	if(histID) {
		int   do_use_old_tech = 0;
		if(!!evDtm) {
			LDATETIME moment;
			PPIDArray acn_list;
			acn_list.add(PPACN_EVENTTOKEN);
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			if(p_sj && p_sj->GetLastObjEvent(PPOBJ_EVENTTOKEN, PPEVTOK_OBJHIST9811, &acn_list, &moment) > 0) {
				if(cmp(moment, evDtm) > 0)
					do_use_old_tech = 1;
			}
		}
		if(!do_use_old_tech) {
			SBuffer buf;
			PPBillPacket pack;
			ObjVersioningCore * p_ovc = PPRef->P_OvT;
			if(p_ovc && p_ovc->InitSerializeContext(1)) {
				SSerializeContext & r_sctx = p_ovc->GetSCtx();
				PPObjID oid;
				long   vv = 0;
				THROW(p_ovc->Search(histID, &oid, &vv, &buf) > 0);
				THROW(BillObj->SerializePacket__(-1, &pack, buf, &r_sctx));
				pack.ProcessFlags |= (PPBillPacket::pfZombie | PPBillPacket::pfUpdateProhibited); // @v9.9.12
				if(GetOpType(pack.Rec.OpID) == PPOPT_INVENTORY) {
					THROW(BillObj->EditInventory(&pack, 0));
				}
				else {
					THROW(::EditGoodsBill(&pack, PPObjBill::efNoUpdNotif));
				}
				ok = 1;
			}
		}
		else {
			HistBillCore hb_core;
			PPBillPacket pack;
			PPHistBillPacket hb_pack;
			THROW(hb_core.GetPacket(histID, &hb_pack) > 0);
			THROW(hb_pack.ConvertToBillPack(&pack));
			THROW(::EditGoodsBill(&pack, PPObjBill::efNoUpdNotif));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewSysJournal::Transmit()
{
	int    ok = -1;
	LockUpByNotify = 1;
	// @v9.9.0 {
	if(ObjNameList.getCount()) {
		ObjTransmitParam param;
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			PPWait(1);
			IterCounter cntr;
			cntr.Init(ObjNameList.getCount());
			for(uint i = 0; i < ObjNameList.getCount(); i++) {
				const PPObjNamePEntry & r_entry = ObjNameList.at(i);
				if(!objid_ary.lsearch(&r_entry, 0, PTR_CMPFUNC(_2long)))
					objid_ary.Add(r_entry.Obj, r_entry.Id);
				PPWaitPercent(cntr);
			}
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
	}
	// } @v9.9.0
	/* @v9.9.0 if(P_NamesTbl) {
		ObjTransmitParam param;
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			BExtQuery   nmq(P_NamesTbl, 0);
			IterCounter cntr;
			TempDoubleIDTbl::Key1  k0;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			PPWait(1);
			nmq.select(P_NamesTbl->PrmrID, P_NamesTbl->ScndID, 0L);
			PPInitIterCounter(cntr, P_NamesTbl);
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
	}*/
	CATCHZOKPPERR
	PPWait(0);
	LockUpByNotify = 0;
	return ok;
}

int SLAPI PPViewSysJournal::RefreshTempTable(LDATETIME since)
{
	int    ok = -1;
	if(P_TmpTbl || Filt.Flags & Filt.fShowObjects) {
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
					// @v9.9.0 @todo заменить следующий блок
					/* @v9.9.0 if(P_NamesTbl) {
						TempDoubleIDTbl::Key1 k1;
						const PPID obj_type = rec.ObjType;
						const PPID obj_id = rec.ObjID;
						k1.ScndID = obj_id;
						k1.PrmrID = obj_type;
						{
							char   name_buf[256];
							PPObject * ppobj = P_ObjColl->GetObjectPtr(obj_type);
							PTR32(name_buf)[0] = 0;
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
	 				}*/
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
				last_dtm = pEv->ExtDtm;
				ok = ProcessCommand(PPVCMD_REFRESH, 0, 0);
				if(pBrw && ok > 0)
					pBrw->Update();
				update = 1;
			}
		}
	}
	return ok;
}
//
//
//
int FASTCALL ViewSysJournal(PPID objType, PPID objID, int _modeless)
{
	SysJournalFilt flt;
	flt.ObjType = objType;
	flt.ObjID = objID;
	return ViewSysJournal(&flt, _modeless);
}

int FASTCALL ViewSysJournal(const SysJournalFilt * pFilt, int asModeless) { return PPView::Execute(PPVIEW_SYSJOURNAL, pFilt, 1, 0); }
//
// Implementation of PPALDD_SysJournalEntry
//
PPALDD_CONSTRUCTOR(SysJournalEntry) { InitFixData(rscDefHdr, &H, sizeof(H)); }
PPALDD_DESTRUCTOR(SysJournalEntry) { Destroy(); }

int PPALDD_SysJournalEntry::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.SurID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		size_t data_len = 0;
		MEMSZERO(H);
		const SysJournalTbl::Rec * p_sj_rec = static_cast<const SysJournalTbl::Rec *>(DS.GetTLA().SurIdList.Get(rFilt.ID, &data_len));
		if(p_sj_rec && data_len == sizeof(*p_sj_rec)) {
			H.SurID = rFilt.ID;
			H.UserID  = p_sj_rec->UserID;
			H.ObjType = p_sj_rec->ObjType;
			H.ObjID   = p_sj_rec->ObjID;
			H.ActionID = p_sj_rec->Action;
			H.Dt       = p_sj_rec->Dt;
			H.Tm       = p_sj_rec->Tm;
			H.Extra    = p_sj_rec->Extra;
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
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(SysJournal) { Destroy(); }

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

void PPALDD_SysJournal::Destroy() { DESTROY_PPVIEW_ALDD(SysJournal); }
//
//
//
IMPLEMENT_PPFILT_FACTORY(GtaJournal); SLAPI GtaJournalFilt::GtaJournalFilt() : PPBaseFilt(PPFILT_GTAJOURNAL, 0, 0)
{
	SetFlatChunk(offsetof(GtaJournalFilt, ReserveStart),
		offsetof(GtaJournalFilt, ActionIDList)-offsetof(GtaJournalFilt, ReserveStart));
	SetBranchSVector(offsetof(GtaJournalFilt, ActionIDList)); // @v9.8.4 SetBranchSArray-->SetBranchSVector
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

SLAPI PPViewGtaJournal::PPViewGtaJournal() : PPView(0, &Filt, PPVIEW_GTAJOURNAL), P_TmpTbl(0), LockUpByNotify(0), LastRefreshDtm(ZERODATETIME), P_ObjColl(new ObjCollection)
{
}

SLAPI PPViewGtaJournal::~PPViewGtaJournal()
{
	delete P_ObjColl;
	delete P_TmpTbl;
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
			RVALUEPTR(Data, pData);
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
				ListToListData l2l_data(PPOBJ_GTACTION, reinterpret_cast<void *>(-1), &Data.ActionIDList);
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
	DIALOG_PROC_BODY(GtaJFiltDialog, static_cast<GtaJournalFilt *>(pBaseFilt));
}

int SLAPI PPViewGtaJournal::IsTempTblNeeded() const
{
	return BIN(Filt.ActionIDList.isList() || (Filt.Flags & SysJournalFilt::fShowObjects));
}

PP_CREATE_TEMP_FILE_PROC(CreateTempGtaFile, GtaJournal);

int SLAPI PPViewGtaJournal::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	SString temp_buf;
	BExtInsert * p_bei = 0;
	Counter.Init();
	ZDELETE(P_TmpTbl);
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	LastRefreshDtm = getcurdatetime_();
	StrPool.ClearS(); // @v9.9.0
	ObjNameList.clear(); // @v9.9.0
	if(IsTempTblNeeded()) {
		if(InitIteration()) {
			uint   i = 0;
			if(Filt.ActionIDList.isList()) {
				THROW(P_TmpTbl = CreateTempGtaFile());
				THROW_MEM(p_bei = new BExtInsert(P_TmpTbl));
			}
			// @v9.9.0 if(Filt.Flags & GtaJournalFilt::fShowObjects) { THROW(P_NamesTbl = CreateNamesFile()); }
			{
				PPTransaction tra(ppDbDependTransaction, 1);
				THROW(tra);
				while(P_IterQuery->nextIteration() > 0) {
					PPID id = 0;
					THROW(PPCheckUserBreak());
					if(Filt.Flags & Filt.fShowObjects) {
						PPObjNamePEntry objn_entry;
						objn_entry.Set(T.data.ObjType, T.data.ObjID);
						if(objn_entry.Obj) {
							objn_entry.NameP = 0;
							uint   objn_pos = 0;
							if(!ObjNameList.bsearch(&objn_entry, &objn_pos, PTR_CMPFUNC(PPObjID))) {
								char   name_buf[256];
								PPObject * ppobj = P_ObjColl->GetObjectPtr(objn_entry.Obj);
								if(ppobj && ppobj->GetName(objn_entry.Id, name_buf, sizeof(name_buf)) > 0) {
									temp_buf = name_buf;
									StrPool.AddS(temp_buf, &objn_entry.NameP);
									ObjNameList.ordInsert(&objn_entry, 0, PTR_CMPFUNC(PPObjID));
								}
							}
						}
					}
					if(p_bei)
						THROW_DB(p_bei->insert(&T.data));
					Counter.Increment();
					PPWaitPercent(Counter);
				}
				if(p_bei)
					THROW_DB(p_bei->flash());
				THROW(tra.Commit());
			}
		}
	}
	CATCHZOK
	BExtQuery::ZDelete(&P_IterQuery);
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
	BExtQuery::ZDelete(&P_IterQuery);
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
		pItem->ObjName.Z();
		// @v9.9.0 {
		PPObjID oid;
		uint   objn_pos = 0;
		oid.Set(p_t->data.ObjType, p_t->data.ObjID);
		if(ObjNameList.lsearch(&oid, &objn_pos, PTR_CMPFUNC(PPObjID))) {
			StrPool.GetS(ObjNameList.at(objn_pos).NameP, pItem->ObjName);
		}
		// } @v9.9.0
		/* @v9.9.0 if(p_t->data.ObjID && P_NamesTbl) {
			TempDoubleIDTbl::Key1  k1;
			k1.ScndID = p_t->data.ObjID;
			k1.PrmrID = p_t->data.ObjType;
			if(P_NamesTbl->search(1, &k1, spEq))
				pItem->ObjName = P_NamesTbl->data.Name;
		}*/
		if(!Filt.BegTm || p_t->data.Dt > Filt.Period.low || p_t->data.Tm >= Filt.BegTm) {
			Counter.Increment();
			PPWaitPercent(Counter);
			return 1;
		}
	}
	return -1;
}

int SLAPI PPViewGtaJournal::GetObjName(const PPObjID & rOid, SString & rBuf) const
{
	rBuf.Z();
	uint   p = 0;
	if(ObjNameList.bsearch(&rOid, &p, PTR_CMPFUNC(PPObjID))) {
		const PPObjNamePEntry & r_entry = ObjNameList.at(p);
		StrPool.GetS(r_entry.NameP, rBuf);
	}
	return rBuf.NotEmpty() ? 1 : -1;
}

static IMPL_DBE_PROC(dbqf_objnamefromlist_ppvgtaj_iip)
{
	char   buf[256];
	if(option == CALC_SIZE) {
		result->init(sizeof(buf));
	}
	else {
		PPObjID oid;
		oid.Set(params[0].lval, params[1].lval);
		const PPViewGtaJournal * p_view = static_cast<const PPViewGtaJournal *>(params[2].ptrval);
		if(p_view) {
			SString temp_buf;
			p_view->GetObjName(oid, temp_buf);
			STRNSCPY(buf, temp_buf);
		}
		else
			buf[0] = 0;
		result->init(buf);
	}
}

/*static*/int PPViewGtaJournal::DynFuncObjNameFromList = DbqFuncTab::RegisterDynR(BTS_STRING, dbqf_objnamefromlist_ppvgtaj_iip, 3, BTS_INT, BTS_INT, BTS_PTR);

DBQuery * SLAPI PPViewGtaJournal::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	int    add_dbe = 0;
	uint   brw_id = (Filt.Flags & GtaJournalFilt::fShowObjects) ? BROWSER_GTAJ_OBJ : BROWSER_GTAJ;
	DBQuery * q = 0;
	DBE    dbe_user;
	DBE    dbe_objtitle;
	DBE    dbe_action;
	DBE    dbe_objname; // @v9.9.0
	DBQ  * dbq = 0;
	GtaJournalTbl * t  = new GtaJournalTbl(P_TmpTbl ? P_TmpTbl->GetName().cptr() : static_cast<const char *>(0));
	// @v9.9.0 TempDoubleIDTbl * nm  = 0;
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
		// @v9.9.0 THROW(CheckTblPtr(nm = P_NamesTbl ? new TempDoubleIDTbl(P_NamesTbl->GetName()) : new TempDoubleIDTbl));
		// @v9.9.0 dbq = & (*dbq && (nm->PrmrID == t->ObjType && (nm->ScndID += t->ObjID)));
		{
			dbe_objname.init();
			dbe_objname.push(t->ObjType);
			dbe_objname.push(t->ObjID);
			dbe_objname.push(dbconst(this));
			dbe_objname.push(static_cast<DBFunc>(DynFuncObjNameFromList));
		}
		q = & select(
			t->ObjType,   // #0
			t->ObjID,     // #1
			t->Op,        // #2
			t->Dt,        // #3
			t->Tm,        // #4
			dbe_user,     // #5
			dbe_action,   // #6
			dbe_objtitle, // #7
			// @v9.9.0 nm->Name,     // #8
			dbe_objname,  // #8 // @v9.9.0
			0L).from(t, /*nm,*/ 0L);
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
			// @v9.9.0 delete nm;
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
		BrwHdr hdr = *static_cast<const PPViewGtaJournal::BrwHdr *>(pHdr);
		ok = EditPPObj(hdr.Obj, hdr.Id);
	}
	return ok;
}

