// V_TSESS.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>
//
// @ModuleDef(PPViewTSession)
//
IMPLEMENT_PPFILT_FACTORY(TSession); SLAPI TSessionFilt::TSessionFilt() : PPBaseFilt(PPFILT_TSESSION, 0, 0)
{
	SetFlatChunk(offsetof(TSessionFilt, ReserveStart),
		offsetof(TSessionFilt, Reserve)-offsetof(TSessionFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

TSessionFilt & FASTCALL TSessionFilt::operator = (const TSessionFilt & s)
{
	Copy(&s, 1);
	return *this;
}

int SLAPI TSessionFilt::CheckIdle(long flags) const
{
	if(Ft_Idle == 0)
		return 1;
	else if(Ft_Idle < 0)
		return (flags & TSESF_IDLE) ? 0 : 1;
	else
		return (flags & TSESF_IDLE) ? 1 : 0;
}

int SLAPI TSessionFilt::CheckStatus(int status) const
{
	return BIN(!StatusFlags || (StatusFlags & (1 << status)));
}

int SLAPI TSessionFilt::GetStatusList(PPIDArray * pList) const
{
	if(StatusFlags) {
		for(int i = 1; i <= 5; i++)
			if(StatusFlags & (1 << i))
				pList->addUnique(i);
		return 1;
	}
	else
		return -1;
}
//
//
//
SLAPI TSessionViewItem::TSessionViewItem()
{
	Init();
}

TSessionViewItem & SLAPI TSessionViewItem::Init()
{
	memzero((TSessionTbl::Rec *)this, sizeof(TSessionTbl::Rec));
	CipItem.Clear();
	WrOffBillID = 0;
	return *this;
}
//
//
//
PPViewTSession::UhttStoreExt & PPViewTSession::UhttStoreExt::Clear()
{
	ID = 0;
	SfList.Clear();
	return *this;
}

PPViewTSession::IterBlock::IterBlock()
{
	Init(0);
}

PPViewTSession::IterBlock & FASTCALL PPViewTSession::IterBlock::Init(int order)
{
	Order = order;
	CipList.Clear();
	WrOffBillList.clear();
	CurItem.Init();
	return *this;
}
//
SLAPI PPViewTSession::PPViewTSession() : PPView(&TSesObj, &Filt), P_TempTbl(0), P_LastAnlzFilt(0), P_UhttsPack(0), State(0)
{
	ImplementFlags |= implOnAddSetupPos;
}

SLAPI PPViewTSession::~PPViewTSession()
{
	delete P_UhttsPack;
	delete P_TempTbl;
	delete P_LastAnlzFilt;
}

PPBaseFilt * SLAPI PPViewTSession::CreateFilt(void * extraPtr) const
{
	PPBaseFilt * p_base_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_TSESSION, &p_base_filt)) {
		((TSessionFilt *)p_base_filt)->StatusFlags |= 0x000000ffL;
		if(((long)extraPtr) == TSESK_PLAN)
			((TSessionFilt *)p_base_filt)->Flags |= TSessionFilt::fManufPlan;
	}
	return p_base_filt;
}

void * SLAPI PPViewTSession::GetEditExtraParam()
{
	int    kind = 0;
	if(Filt.Flags & TSessionFilt::fSuperSessOnly)
		kind = TSESK_SUPERSESS;
	else if(Filt.Flags & TSessionFilt::fManufPlan)
		kind = TSESK_PLAN;
	else if(Filt.Flags & TSessionFilt::fSubSess)
		kind = TSESK_SUBSESS;
	return PPObjTSession::MakeExtraParam(Filt.SuperSessID, PrcList.GetSingle(), kind);
}

#define GRP_PRCTECH 2

int PPViewTSession::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class TSessFiltDialog : public TDialog {
	public:
		TSessFiltDialog(uint dlgId) : TDialog(dlgId/*DLG_TSESSFILT*/)
		{
 			SetupCalPeriod(CTLCAL_TSESSFILT_STPERIOD, CTL_TSESSFILT_STPERIOD);
		 	SetupCalPeriod(CTLCAL_TSESSFILT_FNPERIOD, CTL_TSESSFILT_FNPERIOD);
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_TSESSFILT_IDLE)) {
				const long temp_long = GetClusterData(CTL_TSESSFILT_IDLE);
				PrcTechCtrlGroup * p_grp = (PrcTechCtrlGroup *)getGroup(GRP_PRCTECH);
				CALLPTRMEMB(p_grp, setIdleStatus(this, BIN(temp_long > 0)));
				clearEvent(event);
			}
		}
	};
	int    ok = -1, valid_data = 0;
	uint   dlg_id = 0;
	TSessFiltDialog * dlg = 0;
	PrcTechCtrlGroup::Rec ptcg_rec;
	TSessionFilt * p_filt = 0;
	THROW(Filt.IsA(pBaseFilt));
	p_filt = (TSessionFilt *)pBaseFilt;
	dlg_id = (p_filt->Flags & TSessionFilt::fManufPlan) ? DLG_TSESSPLANFILT : DLG_TSESSFILT;
	THROW(CheckDialogPtr(&(dlg = new TSessFiltDialog(dlg_id))));
	MEMSZERO(ptcg_rec);
	ptcg_rec.PrcID = p_filt->PrcID;
	if(p_filt->Flags & TSessionFilt::fManufPlan) {
		//SetupPPObjCombo(dlg, CTLSEL_TSESSFILT_PRC, PPOBJ_PROCESSOR, p_filt->PrcID, 0, PRCEXDF_GROUP);
 		dlg->addGroup(GRP_PRCTECH, new PrcTechCtrlGroup(CTLSEL_TSESSFILT_PRC, 0, 0, 0, 0, 0));
		if(p_filt->PrcID == 0)
			ptcg_rec.PrcParentID = PRCEXDF_GROUP;
 		dlg->setGroupData(GRP_PRCTECH, &ptcg_rec);
	}
	else {
 		dlg->addGroup(GRP_PRCTECH, new PrcTechCtrlGroup(CTLSEL_TSESSFILT_PRC, CTLSEL_TSESSFILT_TECH,
 			CTL_TSESSFILT_ST_GOODS, CTLSEL_TSESSFILT_AR, CTLSEL_TSESSFILT_AR2, cmSelTechByGoods));
		ptcg_rec.TechID = p_filt->TechID;
		ptcg_rec.ArID   = p_filt->ArID;
		ptcg_rec.Ar2ID  = p_filt->Ar2ID;
		ptcg_rec.IdleStatus = BIN(p_filt->Ft_Idle > 0);
 		dlg->setGroupData(GRP_PRCTECH, &ptcg_rec);
		PrcTechCtrlGroup * p_grp = (PrcTechCtrlGroup *)dlg->getGroup(GRP_PRCTECH);
		CALLPTRMEMB(p_grp, setIdleStatus(dlg, BIN(p_filt->Ft_Idle > 0)));
	}
	dlg->AddClusterAssocDef(CTL_TSESSFILT_STATUS, 0, (1 << TSESST_PLANNED));
	dlg->AddClusterAssoc(CTL_TSESSFILT_STATUS, 1, (1 << TSESST_PENDING));
	dlg->AddClusterAssoc(CTL_TSESSFILT_STATUS, 2, (1 << TSESST_INPROCESS));
	dlg->AddClusterAssoc(CTL_TSESSFILT_STATUS, 3, (1 << TSESST_CLOSED));
	dlg->AddClusterAssoc(CTL_TSESSFILT_STATUS, 4, (1 << TSESST_CANCELED));
	dlg->SetClusterData(CTL_TSESSFILT_STATUS, p_filt->StatusFlags);

	dlg->AddClusterAssoc(CTL_TSESSFILT_FLAGS, 0, TSessionFilt::fSuperSessOnly);
	dlg->SetClusterData(CTL_TSESSFILT_FLAGS, p_filt->Flags);

	dlg->AddClusterAssocDef(CTL_TSESSFILT_IDLE,  0,  0);
	dlg->AddClusterAssoc(CTL_TSESSFILT_IDLE,  1, -1);
	dlg->AddClusterAssoc(CTL_TSESSFILT_IDLE,  2,  1);
	dlg->SetClusterData(CTL_TSESSFILT_IDLE, p_filt->Ft_Idle);

	SetPeriodInput(dlg, CTL_TSESSFILT_STPERIOD, &p_filt->StPeriod);
	dlg->setCtrlData(CTL_TSESSFILT_STTIME, &p_filt->StTime);
	SetPeriodInput(dlg, CTL_TSESSFILT_FNPERIOD, &p_filt->FnPeriod);
	dlg->setCtrlData(CTL_TSESSFILT_FNTIME, &p_filt->FnTime);
	SetupStringCombo(dlg, CTLSEL_TSESSFILT_ORDER, PPTXT_TSESSORDER, p_filt->Order);
	while(!valid_data && ExecView(dlg) == cmOK) {
		if(p_filt->Flags & TSessionFilt::fManufPlan) {
			dlg->getCtrlData(CTLSEL_TSESSFILT_PRC, &p_filt->PrcID);
		}
		else {
			dlg->getGroupData(GRP_PRCTECH, &ptcg_rec);
			p_filt->PrcID  = ptcg_rec.PrcID;
			p_filt->TechID = ptcg_rec.TechID;
			p_filt->ArID   = ptcg_rec.ArID;
			p_filt->Ar2ID  = ptcg_rec.Ar2ID;
		}
		long   temp_long = 0;
		dlg->GetClusterData(CTL_TSESSFILT_STATUS, &p_filt->StatusFlags);
		dlg->GetClusterData(CTL_TSESSFILT_FLAGS, &p_filt->Flags);
		dlg->GetClusterData(CTL_TSESSFILT_IDLE, &temp_long);
		p_filt->Ft_Idle = (int16)temp_long;
		GetPeriodInput(dlg, CTL_TSESSFILT_STPERIOD, &p_filt->StPeriod);
		dlg->getCtrlData(CTL_TSESSFILT_STTIME, &p_filt->StTime);
		if(!p_filt->StPeriod.low)
			p_filt->StTime = ZEROTIME;
		GetPeriodInput(dlg, CTL_TSESSFILT_FNPERIOD, &p_filt->FnPeriod);
		dlg->getCtrlData(CTL_TSESSFILT_FNTIME, &p_filt->FnTime);
		if(!p_filt->FnPeriod.upp)
			p_filt->FnTime = ZEROTIME;
		dlg->getCtrlData(CTLSEL_TSESSFILT_ORDER, &p_filt->Order);
		ok = valid_data = 1;
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

void SLAPI PPViewTSession::MakeTempRec(const TSessionTbl::Rec * pSrcRec, TempOrderTbl::Rec * pDestRec)
{
	memzero(pDestRec, sizeof(*pDestRec));
	pDestRec->ID = pSrcRec->ID;
	SString temp_buf, st_buf, fin_buf;
	st_buf.Cat(*(LDATETIME *)&pSrcRec->StDt, DATF_YMD|DATF_CENTURY, TIMF_HMS);
	fin_buf.Cat(*(LDATETIME *)&pSrcRec->FinDt, DATF_YMD|DATF_CENTURY, TIMF_HMS);
	if(Filt.Order == TSessionFilt::ordByFnTime) {
		temp_buf.Cat(fin_buf).Cat(st_buf);
	}
	else {
		if(Filt.Order == TSessionFilt::ordByPrc) {
			ProcessorTbl::Rec prc_rec;
			if(TSesObj.GetPrc(pSrcRec->PrcID, &prc_rec, 0, 1) > 0)
				temp_buf.Cat(prc_rec.Name);
			else
				ideqvalstr(pSrcRec->PrcID, temp_buf);
		}
		else if(Filt.Order == TSessionFilt::ordByMainGoods) {
			TechTbl::Rec tec_rec;
			if(TSesObj.GetTech(pSrcRec->TechID, &tec_rec, 1) > 0)
				GetGoodsName(tec_rec.GoodsID, temp_buf);
			else
				ideqvalstr(0, temp_buf);
		}
		else if(Filt.Order == TSessionFilt::ordByAr) {
			GetArticleName(pSrcRec->ArID, temp_buf);
		}
		else if(Filt.Order == TSessionFilt::ordByAmount) {
			temp_buf.Cat(pSrcRec->Amount, MKSFMTD(20, 2, 0));
		}
		temp_buf.Cat(st_buf).Cat(fin_buf);
	}
	temp_buf.CopyTo(pDestRec->Name, sizeof(pDestRec->Name));
}

int SLAPI PPViewTSession::IsTempTblNeeded() const
{
	return BIN(Filt.Order || (Filt.StPeriod.low && Filt.StTime) || (Filt.FnPeriod.upp && Filt.FnTime) || Filt.ArID || PrcList.GetCount() > 1);
}

int SLAPI PPViewTSession::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	{
		PPObjProcessor prc_obj;
		PPIDArray prc_list, local_prc_list;
		ZDELETE(P_TempTbl);
		BExtQuery::ZDelete(&P_IterQuery);
		State = 0;
		PrcList.Set(0);
		Filt.StPeriod.Actualize(ZERODATE);
		Filt.FnPeriod.Actualize(ZERODATE);
		if(Filt.Flags & TSessionFilt::fCurrent && Filt.PrcID) {
			LDATETIME curdtm = getcurdatetime_();
			PPID   sess_id = 0;
			if(TSesObj.IsProcessorBusy(Filt.PrcID, 0, TSESK_SUPERSESS, curdtm, 1, &sess_id) > 0) {
				Filt.SuperSessID = sess_id;
			}
			else {
				Filt.StPeriod.Set(plusdate(curdtm.d, -1), plusdate(curdtm.d, 1));
				Filt.Flags |= TSessionFilt::fSuperSessOnly;
			}
		}
		if(Filt.SuperSessID) {
			TSessionTbl::Rec rec;
			if(TSesObj.Search(Filt.SuperSessID, &rec) > 0) {
				if(!(rec.Flags & TSESF_SUPERSESS) && !(Filt.Flags & TSessionFilt::fSubSess))
					State |= stSuperSessIsSimple;
			}
		}
		if(Filt.UhttStoreID) {
			PPObjUhttStore uhtts_obj;
			if(P_UhttsPack)
				P_UhttsPack->destroy();
			else
				THROW_MEM(P_UhttsPack = new PPUhttStorePacket);
			if(uhtts_obj.GetPacket(Filt.UhttStoreID, P_UhttsPack) > 0) {
				PPObjGlobalUserAcc gua_obj;
				PPObjPerson psn_obj;
				PPObjPersonRelType prt_obj;
				PPIDArray gua_list;
				PPIDArray psn_list;
				PPID   rel_type_id = 0;
				const  PPID store_owner_id = P_UhttsPack->Rec.PersonID;
				if(prt_obj.SearchBySymb("UHTT_PSNREL_MASTER", &rel_type_id, 0) > 0) {
					LAssocArray rel_list;
					psn_obj.P_Tbl->GetRelList(store_owner_id, &rel_list, 1 /* reverse */);
					for(uint i = 0; i < rel_list.getCount(); i++) {
						const LAssoc & r_item = rel_list.at(i);
						if(r_item.Val == rel_type_id)
							psn_list.add(r_item.Key);
					}
				}
				psn_list.add(store_owner_id);
				psn_list.sortAndUndup();
				for(uint pi = 0; pi < psn_list.getCount(); pi++) {
					const PPID psn_id = psn_list.get(pi);
					if(gua_obj.SearchByAssociatedPersonID(psn_id, gua_list) > 0) {
						for(uint i = 0; i < gua_list.getCount(); i++) {
							const PPID gua_id = gua_list.get(i);
							if(prc_obj.GetListByOwnerGuaID(gua_id, local_prc_list) > 0)
								prc_list.add(&local_prc_list);
						}
					}
				}
				prc_list.sortAndUndup();
				if(P_UhttsPack->Sd.GetCount()) {
					THROW(P_UhttsPack->GetSelectorListInfo(0, ExtSfTitleList));
				}
			}
			else
				ZDELETE(P_UhttsPack);
			if(prc_list.getCount() == 0) {
				State |= stEmpty;
			}
			else {
				PrcList.Set(&prc_list);
			}
		}
		else if(Filt.PrcID) {
			ProcessorTbl::Rec prc_rec;
			if(!Filt.SuperSessID && !(Filt.Flags & TSessionFilt::fSuperSessOnly) && TSesObj.GetPrc(Filt.PrcID, &prc_rec, 0, 1) > 0 && prc_rec.Kind == PPPRCK_GROUP) {
				prc_list.clear();
				prc_obj.GetChildIDList(Filt.PrcID, 1, &prc_list); // @recursion
				prc_list.add(Filt.PrcID);
				prc_list.sortAndUndup();
			}
			else
				prc_list.add(Filt.PrcID);
			PrcList.Set(&prc_list);
		}
		if(IsTempTblNeeded()) {
			THROW(P_TempTbl = CreateTempOrderFile());
			if(!(State & stEmpty)) {
				PPIDArray prc_id_list;
				if(PrcList.GetCount())
					prc_id_list = PrcList.Get();
				else
					prc_id_list.add(0L);
				BExtInsert bei(P_TempTbl);
				TSessionTbl * p_t = TSesObj.P_Tbl;
				PPTransaction tra(ppDbDependTransaction, 1);
				THROW(tra);
				for(uint i = 0; i < prc_id_list.getCount(); i++) {
					TSessionTbl::Rec rec;
					union {;
						TSessionTbl::Key2 k2; // prc
						TSessionTbl::Key4 k4; // prc
						TSessionTbl::Key5 k5; // tech
					} k;
					const  PPID prc_id = prc_id_list.get(i);
					int    idx = 0;
					DBQ * dbq = 0;
					MEMSZERO(k);
					if(prc_id) {
						idx = 4;
						k.k4.PrcID = prc_id;
						k.k4.StDt = Filt.StPeriod.low;
					}
					else if(Filt.TechID) {
						idx = 5;
						k.k5.TechID = Filt.TechID;
						k.k5.StDt = Filt.StPeriod.low;
					}
					else {
						idx = 2;
						k.k2.StDt = Filt.StPeriod.low;
					}
					dbq = ppcheckfiltid(dbq, p_t->PrcID, prc_id);
					dbq = ppcheckfiltid(dbq, p_t->TechID, Filt.TechID);
					dbq = &(*dbq && daterange(p_t->StDt, &Filt.StPeriod));
					dbq = &(*dbq && daterange(p_t->FinDt, &Filt.FnPeriod));
					dbq = ppcheckfiltid(dbq, p_t->ArID, Filt.ArID);
					dbq = ppcheckfiltid(dbq, p_t->Ar2ID, Filt.Ar2ID);

					BExtQuery q(p_t, idx);
					q.selectAll().where(*dbq);
					for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
						//if(TSesObj.Search(p_t->data.ID, &rec) > 0 && TSesObj.CheckForFilt(&Filt, p_t->data.ID, &rec)) {
						p_t->copyBufTo(&rec);
						if(TSesObj.CheckForFilt(&Filt, rec.ID, &rec)) {
							TempOrderTbl::Rec temp_rec;
							MakeTempRec(&rec, &temp_rec);
							THROW_DB(bei.insert(&temp_rec));
						}
					}
				}
				THROW_DB(bei.flash());
				THROW(tra.Commit());
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewTSession::InitIteration(int order)
{
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	Ib.Init(order);

	int    ok = 1, idx = 0;
	if(!(State & stEmpty)) {
		if(P_TempTbl) {
			TempOrderTbl::Key1 k1;
			P_IterQuery = new BExtQuery(P_TempTbl, 1);
			MEMSZERO(k1);
			Counter.Init(P_IterQuery->countIterations(0, &k1, spFirst));
			MEMSZERO(k1);
			P_IterQuery->initIteration(0, &k1, spFirst);
		}
		else {
			union {
				TSessionTbl::Key0 k0; // ID
				TSessionTbl::Key2 k2; // StDt
				TSessionTbl::Key4 k4; // PrcID
				TSessionTbl::Key5 k5; // TechID
				TSessionTbl::Key6 k6; // ParentID
			} k, k_;
			TSessionTbl * p_t = TSesObj.P_Tbl;
			DBQ  * dbq = 0;
			MEMSZERO(k);
			if(Filt.SuperSessID) {
				if(State & stSuperSessIsSimple) {
					idx = 0;
					k.k0.ID = Filt.SuperSessID;
					dbq = &(p_t->ID == Filt.SuperSessID);
				}
				else {
					idx = 6;
					k.k6.ParentID = Filt.SuperSessID;
					dbq = ppcheckfiltid(dbq, p_t->ParentID, Filt.SuperSessID);
				}
			}
			else {
				if(PrcList.GetSingle()) {
					idx = 4;
					k.k4.PrcID = PrcList.GetSingle();
					k.k4.StDt = Filt.StPeriod.low;
				}
				else if(Filt.TechID) {
					idx = 5;
					k.k5.TechID = Filt.TechID;
					k.k5.StDt = Filt.StPeriod.low;
				}
				else {
					idx = 2;
					k.k2.StDt = Filt.StPeriod.low;
				}
				dbq = ppcheckfiltid(dbq, p_t->PrcID, PrcList.GetSingle());
				dbq = ppcheckfiltid(dbq, p_t->TechID, Filt.TechID);
				dbq = &(*dbq && daterange(p_t->StDt, &Filt.StPeriod));
				dbq = &(*dbq && daterange(p_t->FinDt, &Filt.FnPeriod));
				dbq = ppcheckfiltid(dbq, p_t->ArID, Filt.ArID);
				dbq = ppcheckfiltid(dbq, p_t->Ar2ID, Filt.Ar2ID);
			}
			THROW_MEM(P_IterQuery = new BExtQuery(p_t, idx, 8));
			P_IterQuery->selectAll().where(*dbq);
			k_ = k;
			Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
			P_IterQuery->initIteration(0, &k, spGe);
		}
	}
	CATCH
		ok = 0;
		BExtQuery::ZDelete(&P_IterQuery);
	ENDCATCH
	return ok;
}

int FASTCALL PPViewTSession::NextIteration(TSessionViewItem * pItem)
{
	int    ok = -1;
	if(!(State & stEmpty) && P_IterQuery) {
		if(Ib.Order & ordfWithCip && Ib.CipList.NextIteration(Ib.CurItem.CipItem) > 0) {
			ASSIGN_PTR(pItem, Ib.CurItem);
			ok = 1;
		}
		else if(Ib.Order & ordfWithBill && Ib.WrOffBillList.getPointer() < Ib.WrOffBillList.getCount()) {
			Ib.CurItem.WrOffBillID = Ib.WrOffBillList.get(Ib.WrOffBillList.getPointer());
			Ib.WrOffBillList.incPointer();
			ASSIGN_PTR(pItem, Ib.CurItem);
			ok = 1;
		}
		else {
			while(ok < 0 && P_IterQuery->nextIteration() > 0) {
				Counter.Increment();
				int    do_finish = 0;
				if(P_TempTbl) {
					if(TSesObj.Search(P_TempTbl->data.ID, (TSessionTbl::Rec *)&Ib.CurItem) > 0) {
						do_finish = 1;
					}
				}
				else {
					TSesObj.P_Tbl->copyBufTo((TSessionTbl::Rec *)&Ib.CurItem);
					if((Filt.SuperSessID || Filt.CheckStatus(Ib.CurItem.Status)) && Filt.CheckIdle(Ib.CurItem.Flags)) {
						if(Filt.Flags & TSessionFilt::fManufPlan && !(Ib.CurItem.Flags & TSESF_PLAN))
							continue;
						if(!(Filt.Flags & TSessionFilt::fManufPlan) && Ib.CurItem.Flags & TSESF_PLAN)
							continue;
						if(Filt.StPeriod.low && Filt.StTime && Ib.CurItem.StDt == Filt.StPeriod.low && Ib.CurItem.StTm < Filt.StTime)
							continue;
						if(Filt.FnPeriod.upp && Filt.FnTime && Ib.CurItem.FinDt == Filt.FnPeriod.upp && Ib.CurItem.FinTm > Filt.FnTime)
							continue;
						do_finish = 1;
					}
				}
				if(do_finish) {
					if(Ib.Order & ordfWithCip) {
						PPCheckInPersonMngr ci_mgr;
						ci_mgr.GetList(PPCheckInPersonItem::kTSession, Ib.CurItem.ID, Ib.CipList);
						Ib.CipList.InitIteration();
						Ib.CipList.NextIteration(Ib.CurItem.CipItem);
					}
					else if(Ib.Order & ordfWithBill) {
						PPObjBill * p_bobj = BillObj;
						Ib.WrOffBillList.clear();
						for(PPID memb_id = 0; p_bobj->EnumMembersOfPool(PPASS_TSESSBILLPOOL, Ib.CurItem.ID, &memb_id) > 0;) {
							BillTbl::Rec bill_rec;
							if(p_bobj->Search(memb_id, &bill_rec) > 0) {
								Ib.WrOffBillList.add(bill_rec.ID);
							}
						}
						if(Ib.WrOffBillList.getCount()) {
							Ib.CurItem.WrOffBillID = Ib.WrOffBillList.get(0);
							Ib.WrOffBillList.incPointer();
						}
						else
							Ib.CurItem.WrOffBillID = 0;
					}
					ASSIGN_PTR(pItem, Ib.CurItem);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int SLAPI PPViewTSession::GetUhttStoreExtension(const TSessionTbl::Rec & rItem, PPViewTSession::UhttStoreExt & rExt)
{
    int    ok = 1;
    rExt.Clear();
    rExt.ID = rItem.ID;

	PPUhttStoreSelDescr::Entry sd_entry;
	SString ext_sd_text;
	long   ext_sd_id = 0;
	long   ext_sd_parent_id = 0;
	PPObjTag tag_obj;
	ObjTagItem tag_item;
	ProcessorTbl::Rec prc_rec;

	for(uint i = 0; i < ExtSfTitleList.getCount(); i++) {
		StrAssocArray::Item item = ExtSfTitleList.at_WithoutParent(i);
		const uint sd_pos = (item.Id - 1);
		assert(sd_pos < P_UhttsPack->Sd.GetCount());
		ext_sd_text = 0;
		ext_sd_id = 0;
		ext_sd_parent_id = 0;
		if(P_UhttsPack->Sd.GetEntry(sd_pos, sd_entry)) {
			switch(sd_entry.Attr) {
				case PPUhttStoreSelDescr::attrName:
					ext_sd_id = rItem.ID;
					ext_sd_text = 0; // @todo Description
					break;
				case PPUhttStoreSelDescr::attrTag:
					if(sd_entry.TagID && tag_obj.FetchTag(rItem.ID, sd_entry.TagID, &tag_item) > 0) {
						if(tag_item.TagDataType == OTTYP_ENUM) {
							tag_item.GetEnumData(&ext_sd_id, &ext_sd_parent_id, &ext_sd_text, 0);
						}
						else {
							tag_item.GetInt(&ext_sd_id);
							tag_item.GetStr(ext_sd_text);
						}
					}
					break;
				case PPUhttStoreSelDescr::attrPeriod:
                    ext_sd_id = (long)rItem.StDt.v;
                    ext_sd_text.Cat(rItem.StDt, DATF_YMD|DATF_CENTURY);
					break;
				case PPUhttStoreSelDescr::attrProcessor:
					ext_sd_id = rItem.PrcID;
					if(TSesObj.GetPrc(rItem.PrcID, &prc_rec, 0, 1) > 0) {
						ext_sd_text = prc_rec.Name;
					}
					break;
				case PPUhttStoreSelDescr::attrCity:
					if(TSesObj.GetPrc(rItem.PrcID, &prc_rec, 0, 1) > 0 && prc_rec.LocID) {
						PPObjLocation loc_obj;
						loc_obj.GetCity(prc_rec.LocID, &ext_sd_id, &ext_sd_text, 1);
					}
					break;
			}
		}
		if(ext_sd_parent_id)
			rExt.SfList.AddFast(ext_sd_id, ext_sd_parent_id, ext_sd_text);
		else
			rExt.SfList.AddFast(ext_sd_id, ext_sd_text);
	}
    return ok;
}

DBQuery * SLAPI PPViewTSession::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	static DbqStringSubst status_subst(5); // @global @threadsafe

	uint   brw_id = 0;
	if(Filt.Flags & TSessionFilt::fManufPlan)
		brw_id = BROWSER_TSESS_MANUFPLAN;
	else if(Filt.Flags & TSessionFilt::fSubSess && Filt.SuperSessID)
		brw_id = BROWSER_TSUBSESSION;
	else
		brw_id = BROWSER_TSESSION;
	TempOrderTbl * p_ord = 0;
	TSessionTbl  * p_tsst = 0;
	DBQuery * q  = 0;
	DBQ     * dbq = 0;
	DBE  * dbe_status = 0, * dbe_diff_qtty = 0;
	DBE    dbe_tech, dbe_prc, dbe_ar, dbe_goods;
	PPIDArray status_list;
	if(P_TempTbl)
		THROW(CheckTblPtr(p_ord = new TempOrderTbl(P_TempTbl->GetName())));
	THROW(CheckTblPtr(p_tsst = new TSessionTbl));
	dbe_status = & enumtoa(p_tsst->Status, 5, status_subst.Get(PPTXT_TSESS_STATUS));
	dbe_diff_qtty = &(p_tsst->ActQtty - p_tsst->PlannedQtty);
	PPDbqFuncPool::InitObjNameFunc(dbe_tech,  PPDbqFuncPool::IdObjNameTech, p_tsst->TechID);
	PPDbqFuncPool::InitObjNameFunc(dbe_prc,   PPDbqFuncPool::IdObjNamePrc,  p_tsst->PrcID);
	PPDbqFuncPool::InitObjNameFunc(dbe_ar,    PPDbqFuncPool::IdObjNameAr,   p_tsst->ArID);
	PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoodsByTech, p_tsst->TechID);
	THROW_MEM(q = new DBQuery);
	q->syntax |= DBQuery::t_select;
	q->addField(p_tsst->ID);          //  #0
	q->addField(p_tsst->Num);         //  #1
	q->addField(*dbe_status);         //  #2
	q->addField(dbe_prc);             //  #3
	q->addField(dbe_tech);            //  #4
	q->addField(p_tsst->StDt);        //  #5
	q->addField(p_tsst->StTm);        //  #6
	q->addField(p_tsst->FinDt);       //  #7
	q->addField(p_tsst->FinTm);       //  #8
	q->addField(p_tsst->PlannedQtty); //  #9
	q->addField(p_tsst->ActQtty);     //  #10
	q->addField(*dbe_diff_qtty);      //  #11
	q->addField(p_tsst->Incomplete);  //  #12

	q->addField(p_tsst->Amount);      //  #13
	q->addField(dbe_ar);              //  #14
	q->addField(dbe_goods);           //  #15

	q->addField(p_tsst->Memo);        //  #16

	delete dbe_status;
	delete dbe_diff_qtty;
	if(p_ord) {
		q->from(p_ord, p_tsst, 0L);
		dbq = & (*dbq && p_tsst->ID == p_ord->ID);
	}
	else {
		q->from(p_tsst, 0L);
		if(Filt.SuperSessID) {
			if(State & stSuperSessIsSimple)
				dbq = &(p_tsst->ID == Filt.SuperSessID);
			else
				dbq = ppcheckfiltid(dbq, p_tsst->ParentID,  Filt.SuperSessID);
		}
		else {
			dbq = ppcheckfiltid(dbq, p_tsst->PrcID, PrcList.GetSingle());
			dbq = ppcheckfiltid(dbq, p_tsst->TechID, Filt.TechID);
			dbq = &(*dbq && daterange(p_tsst->StDt, &Filt.StPeriod));
			dbq = &(*dbq && daterange(p_tsst->FinDt, &Filt.FnPeriod));
			dbq = ppcheckfiltid(dbq, p_tsst->ArID, Filt.ArID);
			dbq = ppcheckfiltid(dbq, p_tsst->Ar2ID, Filt.Ar2ID);
			dbq = ppcheckflag(dbq, p_tsst->Flags, TSESF_IDLE, Filt.Ft_Idle);
			if(Filt.GetStatusList(&status_list) > 0)
				dbq = & (*dbq && ppidlist(p_tsst->Status, &status_list));
			dbq = ppcheckflag(dbq, p_tsst->Flags, TSESF_SUPERSESS,
				(Filt.Flags & TSessionFilt::fSuperSessOnly) ? 1 : 0);
		}
		dbq = ppcheckflag(dbq, p_tsst->Flags, TSESF_PLAN, (Filt.Flags & TSessionFilt::fManufPlan) ? +1 : -1);
	}
	q->where(*dbq);
	if(p_ord)
		q->orderBy(p_ord->Name, 0L);
	else {
		if(Filt.SuperSessID)
			if(State & stSuperSessIsSimple)
				q->orderBy(p_tsst->ID, 0L);
			else
				q->orderBy(p_tsst->ParentID, 0L);
		else if(PrcList.GetSingle())
			q->orderBy(p_tsst->PrcID, p_tsst->StDt, 0L);
		else if(Filt.TechID)
			q->orderBy(p_tsst->TechID, p_tsst->StDt, 0L);
		else
			q->orderBy(p_tsst->StDt, 0L);
	}
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		if(Filt.SuperSessID) {
			GetObjectName(PPOBJ_TSESSION, Filt.SuperSessID, *pSubTitle, 1);
		}
		else {
			if(Filt.PrcID) {
				pSubTitle->CatDivIfNotEmpty(';', 0);
				GetObjectName(PPOBJ_PROCESSOR, Filt.PrcID, *pSubTitle, 1);
			}
			if(Filt.TechID) {
				pSubTitle->CatDivIfNotEmpty(';', 0);
				GetObjectName(PPOBJ_TECH, Filt.TechID, *pSubTitle, 1);
			}
		}
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete p_tsst;
			delete p_ord;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

void SLAPI PPViewTSession::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		const PPTSessConfig & r_cfg = TSesObj.GetConfig();
		if(r_cfg.ViewRefreshPeriod)
			pBrw->SetRefreshPeriod(r_cfg.ViewRefreshPeriod);
	}
}

int SLAPI PPViewTSession::WriteOff(PPID sessID)
{
	int    ok = -1, r;
	if(sessID) {
		PUGL   pugl;
		PPIDArray sess_list;
		sess_list.add(sessID);
		THROW(r = TSesObj.WriteOff(&sess_list, &pugl, 1));
		if(r > 0) {
			ok = 1;
		}
		else if(r == -2) {
			pugl.ClearActions();
			pugl.OPcug = PCUG_CANCEL;
			ProcessUnsuffisientList(DLG_MSGNCMPL4, &pugl);
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewTSession::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	PPID   id = pHdr ? *(PPID *)pHdr : 0;
	TSessionTbl::Rec rec;
	if(id && TSesObj.Search(id, &rec) > 0)
		if(rec.Flags & TSESF_SUPERSESS) {
			TSessionFilt filt;
			filt.SuperSessID = id;
			ViewTSession(&filt);
		}
		else {
			TSessLineFilt filt(id);
			ViewTSessLine(&filt);
			ok = 1;
		}
	return ok;
}

int SLAPI PPViewTSession::CalcTotal(TSessionTotal * pTotal)
{
	TSessionViewItem item;
	TSessionTotal total;
	MEMSZERO(total);
	for(InitIteration(0); NextIteration(&item) > 0;) {
		total.Count++;
		long   act_timing = diffdatetimesec(item.FinDt, item.FinTm, item.StDt, item.StTm);
		SETMAX(act_timing, 0);
		total.Duration += act_timing;
	}
	ASSIGN_PTR(pTotal, total);
	return 1;
}

int SLAPI PPViewTSession::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_TSESSTOTAL);
	TSessionTotal total;
	CalcTotal(&total);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlLong(CTL_TSESSTOTAL_COUNT, total.Count);
		{
			long   h, m, s;
			SString temp_buf;
			if(total.Duration > 0) {
				h = total.Duration / 3600;
				m = (total.Duration % 3600) / 60;
				s = (total.Duration % 60);
				temp_buf.Z().CatLongZ(h, 2).CatChar(':').CatLongZ(m, 2).CatChar(':').CatLongZ(s, 2);
			}
			dlg->setCtrlString(CTL_TSESSTOTAL_DURATION, temp_buf);
		}
		ExecViewAndDestroy(dlg);
	}
	return -1;
}

int SLAPI PPViewTSession::Print(const void * pHdr)
{
	PPID   id = pHdr ? *(PPID *)pHdr : 0;
	if(id) {
		PPFilt pf(id);
		PPAlddPrint(REPORT_TSESSION, &pf);
	}
	return -1;
}

int SLAPI PPViewTSession::PrintList(const void * pHdr)
{
	int    ok = -1;
	int    use_selection = 0;
	if(Filt.Ft_Idle <= 0) {
		TSessionViewItem item;
		PPCheckInPersonMngr ci_mgr;
		for(InitIteration(0); !use_selection && NextIteration(&item) > 0;) {
			ProcessorTbl::Rec prc_rec;
			if(TSesObj.GetPrc(item.PrcID, &prc_rec, 1, 1) > 0 && prc_rec.Flags & PRCF_ALLOWCIP) {
				PPCheckInPersonArray cip_list;
				if(ci_mgr.GetList(PPCheckInPersonItem::kTSession, item.ID, cip_list) > 0)
					use_selection |= 0x01;
			}
			{
				PPID memb_id = 0;
				if(BillObj->EnumMembersOfPool(PPASS_TSESSBILLPOOL, item.ID, &memb_id) > 0)
					use_selection |= 0x02;
			}
		}
	}
	if(use_selection) {
		uint   s = 0;
		if(SelectorDialog(DLG_SELPRNTSES, CTL_SELPRNTSES_WHAT, &s) > 0) {
			if(s == 1)
				Helper_Print(REPORT_TSESSCIPVIEW, ordfWithCip);
			else if(s == 2)
				Helper_Print(REPORT_TSESSBILLVIEW, ordfWithBill);
			else
				Helper_Print((Filt.Ft_Idle > 0) ? REPORT_TSESSIDLEVIEW : REPORT_TSESSVIEW, 0);
		}
	}
	else
		Helper_Print((Filt.Ft_Idle > 0) ? REPORT_TSESSIDLEVIEW : REPORT_TSESSVIEW, 0);
	return ok;
}

int SLAPI PPViewTSession::Recover()
{
	int    ok = -1;
	PPIDArray id_list;
	TSessionViewItem item;
	PPWait(1);
	for(InitIteration(0); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
		id_list.add(item.ID);
	}
	const uint _c = id_list.getCount();
	if(_c) {
		PPTransaction tra(1);
		THROW(tra);
		for(uint i = 0; i < _c; i++) {
			THROW(TSesObj.Correct(id_list.get(i), 0));
			PPWaitPercent(i+1, _c);
		}
		THROW(tra.Commit());
	}
	PPWait(0);
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewTSession::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		TSessionViewItem item;
		const  PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWait(1);
		for(InitIteration(0); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			objid_ary.Add(PPOBJ_TSESSION, item.ID);
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		PPWait(0);
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewTSession::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *(PPID *)pHdr : 0;
		TSessionTbl::Rec rec;
		switch(ppvCmd) {
			case PPVCMD_INPUTCHAR:
				ok = -1;
				{
					PPInputStringDialogParam isd_param;
					SString code;
					if(pHdr)
						code.CatChar(*(const char *)pHdr);
					PPLoadText(PPTXT_ADDSESSBYPRC, isd_param.Title);
					isd_param.Flags |= PPInputStringDialogParam::fDisableSelection;
					if(InputStringDialog(&isd_param, code) > 0) {
						ProcessorTbl::Rec prc_rec;
						if(TSesObj.GetPrcByCode(code, &prc_rec) > 0) {
							int    sess_kind = (Filt.Flags & TSessionFilt::fSuperSessOnly) ? TSESK_SUPERSESS : TSESK_SESSION;
							PPID   id = 0;
							if(TSesObj.Add(&id, Filt.SuperSessID, prc_rec.ID, sess_kind) > 0)
								ok = 1;
						}
					}
				}
				break;
			case PPVCMD_EDITTECH:
				ok = -1;
				if(TSesObj.Search(id, &rec) > 0 && rec.TechID) {
					PPObjTech tec_obj;
					if(tec_obj.Edit(&rec.TechID, 0) == cmOK)
						ok = 1;
				}
				break;
			case PPVCMD_EDITPRC:
				ok = -1;
				if(TSesObj.Search(id, &rec) > 0 && rec.PrcID) {
					PPObjProcessor prc_obj;
					if(prc_obj.Edit(&rec.PrcID, 0) == cmOK)
						ok = 1;
				}
				break;
			case PPVCMD_VIEWSUBSESS:
				ok = -1;
				if(TSesObj.Search(id, &rec) > 0) {
					TSessionFilt filt;
					filt.SuperSessID = id;
					filt.Flags |= TSessionFilt::fSubSess;
					::ViewTSession(&filt);
				}
				break;
			case PPVCMD_VIEWTSESS:
				ok = -1;
				if(TSesObj.Search(id, &rec) > 0 && rec.PrcID) {
					TSessionFilt filt;
					filt.PrcID = rec.PrcID;
					if(!filt.IsEqual(&Filt, 1))
						::ViewTSession(&filt);
				}
				break;
			case PPVCMD_GOODSREST:
				ok = -1;
				if(id) {
					TSessLineFilt filt(id, 0, 1);
					ViewTSessLine(&filt);
				}
				break;
			case PPVCMD_TSESSANLZ:
				ok = -1;
				if(id) {
					int    r = 1;
					TSessAnlzFilt flt;
					if(P_LastAnlzFilt)
						flt = *P_LastAnlzFilt;
					PPViewTSessAnlz v_tsa;
					if(Filt.Flags & TSessionFilt::fManufPlan)
						flt.PlanSessID = id;
					if(v_tsa.EditBaseFilt(&flt) > 0) {
						flt.SessIdList.FreeAll();
						if(!flt.PlanSessID)
							if(flt.Flags & TSessAnlzFilt::fAll) {
								flt.SetOuterTSessFilt(&Filt);
								TSessionViewItem item;
								for(InitIteration(0); NextIteration(&item) > 0;)
									flt.SessIdList.Add(item.ID);
							}
							else
								flt.SessIdList.Add(id);
						PPView::Execute(PPVIEW_TSESSANLZ, &flt, PPView::exefModeless, 0);
						SETIFZ(P_LastAnlzFilt, new TSessAnlzFilt);
						*P_LastAnlzFilt = flt;
					}
				}
				break;
			case PPVCMD_COMPLETE:
				{
					PPID   tses_id = id;
					if(id)
						tses_id = id;
					else if(Filt.SuperSessID && Filt.Flags & Filt.fSubSess)
						tses_id = Filt.SuperSessID;
					if(tses_id) {
						ok = TSesObj.Complete(tses_id, 1);
						if(!ok)
							PPError();
					}
				}
				break;
			case PPVCMD_WROFFSESS:
				ok = WriteOff(id);
				break;
			case PPVCMD_WROFFBILLS:
				ok = -1;
				ViewBillsByPool(PPASS_TSESSBILLPOOL, id);
				break;
			case PPVCMD_DFCTBILLS:
				ok = -1;
				ViewBillsByPool(PPASS_TSDBILLPOOL, id);
				break;
			case PPVCMD_CHECKPAN:
				ok = -1;
				TSesObj.CallCheckPaneBySess(id);
				break;
			case PPVCMD_DORECOVER:
				ok = Recover();
				break;
			case PPVCMD_PRINTLIST:
				ok = -1;
				PrintList(0);
				break;
			case PPVCMD_TRANSMIT:
				ok = Transmit(0);
				break;
			// @vmiller
			case PPVCMD_SENDSMS:
				ok = SendAutoSms();
				break;
			case PPVCMD_EXPORTUHTT:
				ok = ExportUhtt();
				break;
		}
	}
	return ok;
}

int SLAPI PPViewTSession::ExportUhtt()
{
	int    ok = -1;
	SString msg_buf, fmt_buf, temp_buf, loc_text_buf;
	SString tsess_text;
	PPLogger logger;
	PPUhttClient uhtt_cli;
	PPWait(1);
	THROW(uhtt_cli.Auth());
	{
		SString img_path;
		ObjLinkFiles lf(PPOBJ_TSESSION);
		TSessionViewItem item;
		for(InitIteration(0); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
			const PPID _id = item.ID;
			ProcessorTbl::Rec prc_rec;
			TSessionPacket pack;
			if(TSesObj.GetPacket(_id, &pack, 0) > 0) {
				TSesObj.MakeName(&pack.Rec, tsess_text.Z());
				if(item.PrcID && TSesObj.GetPrc(item.PrcID, &prc_rec, 0, 0) > 0) {
					UhttProcessorPacket uhtt_prc_pack;
					if(prc_rec.Code[0] == 0) {
						logger.Log(PPFormatT(PPTXT_UHTTEXPTSES_NOPRCCODE, &msg_buf, tsess_text.cptr()));
					}
					else if(uhtt_cli.GetProcessorByCode(prc_rec.Code, uhtt_prc_pack) > 0) {
						const long uhtt_prc_id = uhtt_prc_pack.ID;
						long    uhtt_tses_id = 0;
						UhttTSessionPacket uhtt_pack, ret_pack;
						uhtt_pack.ID = uhtt_tses_id;
						S_GUID uuid;
						const ObjTagItem * p_tag_uuid = pack.TagL.GetItem(PPTAG_TSESS_UUID);
						if(!p_tag_uuid || !p_tag_uuid->GetGuid(&uuid)) {
							uuid.Generate();
							ObjTagItem tag_uuid;
							int   _local_error = 0;
							if(!tag_uuid.SetGuid(PPTAG_TSESS_UUID, &uuid))
								_local_error = 1;
							else if(!PPRef->Ot.PutTag(PPOBJ_TSESSION, pack.Rec.ID, &tag_uuid, 1))
								_local_error = 1;
							if(_local_error) {
								logger.LogLastError();
								uuid.SetZero();
							}
						}
						else {
							if(uhtt_cli.GetTSessionByUUID(uuid, ret_pack) > 0) {
								uhtt_tses_id = ret_pack.ID;
							}
						}
						if(!uuid.IsZero()) {
							LDATETIME dtm;
							uhtt_pack.ID = uhtt_tses_id;
							#define CPYFLD(f) uhtt_pack.f = pack.Rec.f
							CPYFLD(Num);
							CPYFLD(ParentID);
							CPYFLD(Status);
							CPYFLD(Flags);
							#undef CPYFLD
							uhtt_pack.PrcID = uhtt_prc_id;
							// @todo uhtt_pack.TechID
							uhtt_pack.StTime = dtm.Set(pack.Rec.StDt, pack.Rec.StTm);
							uhtt_pack.FinTime = dtm.Set(pack.Rec.FinDt, pack.Rec.FinTm);
							uhtt_pack.SetMemo(pack.Rec.Memo);
							// @v8.8.0 {
							pack.Ext.GetExtStrData(PRCEXSTR_DETAILDESCR, temp_buf.Z());
							uhtt_pack.SetDetail(temp_buf);
							// } @v8.8.0
							{
								for(uint i = 0; i < pack.Lines.getCount(); i++) {
									const TSessLineTbl::Rec & r_item = pack.Lines.at(i);
									int   uhtt_goods_id = 0;
									if(uhtt_cli.GetUhttGoods(r_item.GoodsID, 0, &uhtt_goods_id, 0) > 0) {
										LDATETIME dtm;
										UhttTSessLine * p_new_item = new UhttTSessLine;
										THROW_MEM(p_new_item);
										p_new_item->OprNo = r_item.OprNo;
										p_new_item->GoodsID = uhtt_goods_id;
										p_new_item->LotID = 0;
										p_new_item->UserID = 0;
										p_new_item->Sign = r_item.Sign;
										p_new_item->Tm = dtm.Set(r_item.Dt, r_item.Tm);
										p_new_item->Flags = r_item.Flags;
										p_new_item->Qtty = r_item.Qtty;
										p_new_item->WtQtty = r_item.WtQtty;
										p_new_item->Price = r_item.Price;
										p_new_item->Discount = r_item.Discount;
										p_new_item->Expiry = r_item.Expiry;
										p_new_item->Serial = r_item.Serial;
										THROW_SL(uhtt_pack.Places.insert(p_new_item));
									}
									else {
										PPGetLastErrorMessage(1, msg_buf);
										logger.Log(msg_buf);
									}
								}
							}
							{
								for(uint i = 0; i < pack.CiList.GetCount(); i++) {
									const PPCheckInPersonItem & r_item = pack.CiList.Get(i);
									UhttCipPacket * p_new_item = new UhttCipPacket;
									THROW_MEM(p_new_item);
									p_new_item->PersonID = 0; // @todo
									p_new_item->Num = r_item.Num;
									p_new_item->RegCount = r_item.RegCount;
									p_new_item->CiCount = r_item.CiCount;
									p_new_item->Flags = r_item.Flags;
									p_new_item->RegTm = r_item.RegDtm;
									p_new_item->CiTm = r_item.CiDtm;
									p_new_item->Amount = r_item.Amount;
									p_new_item->CCheckID = 0; //
									p_new_item->SCardID = 0; // @todo
									pack.CiList.GetMemo(i, temp_buf.Z());
									p_new_item->SetMemo(temp_buf);
									p_new_item->SetPlaceCode(r_item.PlaceCode);
									THROW_SL(uhtt_pack.Cips.insert(p_new_item));
								}
							}
							{
								uuid.ToStr(S_GUID::fmtIDL, temp_buf.Z());
								UhttTagItem * p_new_item = new UhttTagItem("TSESSUUID", temp_buf);
								THROW_MEM(p_new_item);
								THROW_SL(uhtt_pack.TagList.insert(p_new_item));
							}
							{
								const ObjTagItem * p_descr_tag = pack.TagL.GetItem(PPTAG_TSESS_DESCR);
								if(p_descr_tag && p_descr_tag->GetStr(temp_buf) && temp_buf.NotEmptyS()) {
									UhttTagItem * p_new_item = new UhttTagItem("TSESSDESCR", temp_buf);
									THROW_MEM(p_new_item);
									THROW_SL(uhtt_pack.TagList.insert(p_new_item));
								}
							}
							if(uhtt_cli.CreateTSession(&uhtt_tses_id, uhtt_pack) > 0) {
								logger.Log(PPFormatT(PPTXT_UHTTEXPTSES_EXPORTED, &msg_buf, tsess_text.cptr()));
								if(uhtt_tses_id && pack.Rec.Flags & TSESF_HASIMAGES) {
									lf.Load(pack.Rec.ID, 0L);
									lf.At(0, img_path.Z());
									if(img_path.NotEmptyS()) {
										if(uhtt_cli.SetObjImage("TSESSION", uhtt_tses_id, img_path)) {
											logger.Log(PPFormatT(PPTXT_LOG_UHTT_TSESSSETIMG, &msg_buf, tsess_text.cptr()));
										}
										else {
											logger.Log(PPFormatT(PPTXT_LOG_UHTT_TSESSSETIMGFAULT, &msg_buf, tsess_text.cptr(), uhtt_cli.GetLastMessage().cptr()));
										}
									}
								}
							}
							else {
								logger.Log(PPFormatT(PPTXT_UHTTEXPTSES_EEXPORT, &msg_buf, tsess_text.cptr(), uhtt_cli.GetLastMessage().cptr()));
							}
						}
					}
					else {
						// Не удалось идентифицировать процессор сессии по коду
					}
				}
			}
			else {
				logger.LogLastError();
			}
		}
	}
	PPWait(0);
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

// @vmiller
int SLAPI PPViewTSession::SendAutoSms()
{
	PPAlbatrosConfig albtr_cfg;
	SString msg;
 	PPLogger logger;
 	StrAssocArray psn_list;
 	StrAssocArray phone_list;
	StrAssocArray tsess_list;
	//
	// Но перед этим должна быть проверка на разрешенный период отправки сообщений
	//
 	if(PPAlbatrosCfgMngr::Get(&albtr_cfg) > 0) {
		GetSmsLists(psn_list, phone_list, tsess_list);
 		if(phone_list.getCount()) {
			if(!PPObjSmsAccount::BeginDelivery(albtr_cfg.Hdr.SmsAccID, psn_list, phone_list, PPOBJ_TSESSION, tsess_list))
 				PPError();
 		}
 		else {
			PPLoadText(PPTXT_SMS_NOPHONENUMBER, msg);
 			logger.Log(msg);
 		}
	}
	return 1;
}

// @vmiller
int SLAPI PPViewTSession::GetSmsLists(StrAssocArray & rPsnList, StrAssocArray & rPhoneList, StrAssocArray & rTSessIdArr)
{
	size_t i = 0;
 	SString buf, phone;
	TSessionViewItem item;

	for(InitIteration(ordByDefault); NextIteration(&item) > 0;) {
		PPELinkArray elink_list;
		PPID pers_id = ObjectToPerson(item.ArID);
		if(PersonCore::GetELinks(pers_id, &elink_list) > 0) {
			buf.Z().Cat(item.ID);
			rTSessIdArr.Add(i, buf);
			elink_list.GetItem(PPELK_MOBILE, phone.Z());
			if(phone.Empty())
				elink_list.GetItem(PPELK_WORKPHONE, phone.Z());
 			if(phone.NotEmpty()) {
 				buf.Z().Cat(pers_id);
 				rPhoneList.Add(i, phone);
				rPsnList.Add(i, buf);
 				i++;
 			}
 		}
	}
	return 1;
}


int SLAPI ViewTSession(const TSessionFilt * pFilt) { return PPView::Execute(PPVIEW_TSESSION, pFilt, 1, 0); }
int SLAPI ViewManufPlan(const TSessionFilt * pFilt) { return PPView::Execute(PPVIEW_TSESSION, pFilt, 1, (void *)TSESK_PLAN); }
//
// @ModuleDef(PPViewTSessLine)
//
SLAPI TSessLineFilt::TSessLineFilt(PPID sessID, PPID goodsID, int showRest)
{
	THISZERO();
	TSesList.Add(sessID);
	GoodsID = goodsID;
	SETFLAG(Flags, fOutRest, showRest);
}

int SLAPI TSessLineFilt::Init()
{
	TSesList.Set(0);
	return 1;
}

SLAPI PPViewTSessLine::PPViewTSessLine() : P_TempTbl(0), NewGoodsGrpID(0)
{
}

SLAPI PPViewTSessLine::~PPViewTSessLine()
{
	delete P_TempTbl;
}

const TSessLineFilt * SLAPI PPViewTSessLine::GetFilt() const
{
	return &Filt;
}

int SLAPI PPViewTSessLine::IsTempTblNeeded()
{
	if(Filt.TSesList.GetCount() > 1)
		return 1;
	else {
		Goods2Tbl::Rec goods_rec;
		if(Filt.GoodsID && GObj.Fetch(Filt.GoodsID, &goods_rec) > 0)
			if(goods_rec.Kind == PPGDSK_GROUP || (goods_rec.Kind == PPGDSK_GOODS && goods_rec.Flags & GF_GENERIC))
				return 1;
	}
	return 0;
}

// @v8.6.6 PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TSessLine);

int SLAPI PPViewTSessLine::Init(const TSessLineFilt * pFilt)
{
	int    ok = 1;
	TSessLineTbl * p_temp_tbl = 0;
	Filt = *pFilt;
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempTbl);
	Counter.Init();
	{
		GoodsIdList.freeAll();
		Goods2Tbl::Rec goods_rec;
		if(Filt.GoodsID) {
			GoodsIdList.add(Filt.GoodsID);
			if(GObj.Fetch(Filt.GoodsID, &goods_rec) > 0) {
				if(goods_rec.Kind == PPGDSK_GROUP || (goods_rec.Kind == PPGDSK_GOODS && goods_rec.Flags & GF_GENERIC)) {
					GoodsFilt goods_flt;
					goods_flt.GrpID = Filt.GoodsID;
					GoodsIdList.freeAll();
					THROW(GoodsIterator::GetListByFilt(&goods_flt, &GoodsIdList));
				}
			}
			GoodsIdList.sort();
		}
	}
	if(IsTempTblNeeded()) {
		THROW(p_temp_tbl = CreateTempFile <TSessLineTbl> ());
		{
			TSessLineViewItem item;
			BExtInsert bei(p_temp_tbl);
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			for(InitIteration(); NextIteration(&item) > 0;) {
				THROW_DB(bei.insert(&item));
			}
			THROW_DB(bei.flash());
			THROW(tra.Commit());
			P_TempTbl = p_temp_tbl;
		}
	}
	CATCH
		delete p_temp_tbl;
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPViewTSessLine::CreateIterQuery()
{
	int    ok  = 1;
	int    idx = 1;
	union {
		TSessLineTbl::Key1 k1;
		TSessLineTbl::Key2 k2;
	} k, k_;
	MEMSZERO(k);
	if(P_TempTbl) {
		if(P_IterQuery == 0) {
			P_IterQuery = new BExtQuery(P_TempTbl, idx, 16);
			P_IterQuery->selectAll();
			PPInitIterCounter(Counter, P_TempTbl);
			P_IterQuery->initIteration(0, &k, spFirst);
		}
		else {
			//
			// Если функция NextIteration попытается снова вызвать
			// создание запроса, то мы ответим ей отказом и таким образом
			// избежим зацикливания.
			// При этом мы полагаемся на то, что InitIteration обнулила P_IterQuery.
			//
			ok = -1;
		}
	}
	else {
		BExtQuery::ZDelete(&P_IterQuery);
		PPID   sess_id = 0, goods_id = 0;
		if(SessListIter < 0) {
			if(GoodsListIter >= 0)
				if(GoodsListIter < (long)GoodsIdList.getCount())
					goods_id = GoodsIdList.at(GoodsListIter++);
				else
					ok = -1;
		}
		else if(SessListIter < (long)Filt.TSesList.GetCount())
			sess_id = Filt.TSesList.Get().at(SessListIter++);
		else
			ok = -1;
		if(ok > 0) {
			TSessLineTbl * p_t = &TSesObj.P_Tbl->Lines;
			DBQ  * dbq = 0;
			if(sess_id) {
				idx = 1;
				dbq = &(p_t->TSessID == sess_id);
				dbq = ppcheckfiltid(dbq, p_t->GoodsID, GoodsIdList.getSingle());
				k.k1.TSessID = sess_id;
			}
			else if(goods_id) {
				idx = 2;
				dbq = &(p_t->GoodsID == goods_id);
				k.k2.GoodsID = goods_id;
			}
			P_IterQuery = new BExtQuery(p_t, idx, 16);
			P_IterQuery->selectAll().where(*dbq);
			k_ = k;
			Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
			P_IterQuery->initIteration(0, &k, spGe);
		}
	}
	return ok;
}

int SLAPI PPViewTSessLine::InitIteration()
{
	int    ok = 1;
	BExtQuery::ZDelete(&P_IterQuery);
	GoodsListIter = Filt.GoodsID ? 0 : -1;
	SessListIter  = Filt.TSesList.IsExists() ? 0 : -1;
	return 1;
}

int FASTCALL PPViewTSessLine::NextIteration(TSessLineViewItem * pItem)
{
	do {
		while(P_IterQuery && P_IterQuery->nextIteration() > 0) {
			TSessLineTbl::Rec rec;
			if(P_TempTbl)
				P_TempTbl->copyBufTo(&rec);
			else {
				TSesObj.P_Tbl->Lines.copyBufTo(&rec);
				if(Filt.Flags & TSessLineFilt::fOutRest) {
					if(!(rec.Flags & TSESLF_OUTREST))
						continue;
				}
				else {
					if(rec.Flags & TSESLF_OUTREST)
						continue;
				}
				if(GoodsListIter >= 0 && !GoodsIdList.bsearch(rec.GoodsID))
					continue;
				//
				// Если список сессий не определен, но отчет поднимается по множеству сессий,
				// то проверяем каждую сессию на удовлетворение ограничения по периоду начала
				//
				if(Filt.GoodsID && !Filt.TSesList.IsExists() && !Filt.StPeriod.IsZero()) {
					TSessionTbl::Rec ses_rec;
					if(TSesObj.Search(rec.TSessID, &ses_rec) > 0) {
						if(!Filt.StPeriod.CheckDate(ses_rec.StDt) || ses_rec.Flags & TSESF_PLAN)
							continue;
					}
					else
						continue;
				}
			}
			Counter.Increment();
			ASSIGN_PTR(pItem, rec);
			return 1;
		}
	} while(CreateIterQuery() > 0);
	return -1;
}

struct TSessLineTotal {
	long   Count;
	double Qtty;
	double Amount;
	double Discount;
};

int SLAPI PPViewTSessLine::ViewTotal()
{
	int    ok = -1;
	TDialog * dlg = 0;
	TSessLineTotal total;
	MEMSZERO(total);
	TSessLineViewItem item;
	for(InitIteration(); NextIteration(&item) > 0;) {
		total.Count++;
		total.Qtty += item.Qtty;
		total.Amount += (item.Price - item.Discount) * item.Qtty;
		total.Discount += (item.Discount * item.Qtty);
	}
	dlg = new TDialog(DLG_TSESSLNTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlLong(CTL_TSESSLNTOTAL_COUNT,  total.Count);
		dlg->setCtrlReal(CTL_TSESSLNTOTAL_QTTY,   total.Qtty);
		dlg->setCtrlReal(CTL_TSESSLNTOTAL_AMOUNT, total.Amount);
		dlg->setCtrlReal(CTL_TSESSLNTOTAL_DSCNT,  total.Discount);
		ExecViewAndDestroy(dlg);
	}
	else
		ok = 0;
	return ok;
}

DBQuery * SLAPI PPViewTSessLine::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	static DbqStringSubst sign_subst(3); // @global @threadsafe

	uint   brw_id = 0;
	TSessLineTbl * p_tslt = 0;
	TSessionTbl  * p_tst = 0;
	DBQuery * q  = 0;
	DBQ     * dbq = 0;
	DBE    dbe_user, dbe_prc, dbe_datetime, dbe_goods, dbe_phqtty, dbe_flags;
	DBE  * p_dbe_price  = 0;
	DBE  * p_dbe_amount = 0;
	DBE  * p_dbe_sign   = 0;
	if(P_TempTbl) {
		brw_id = BROWSER_TSESSLINEGOODS;
		THROW(CheckTblPtr(p_tslt = new TSessLineTbl(P_TempTbl->GetName())));
	}
	else {
		if(Filt.GoodsID)
			brw_id = BROWSER_TSESSLINEGOODS;
		else if(Filt.Flags & TSessLineFilt::fOutRest)
			brw_id = BROWSER_TSESSLINEREST;
		else
			brw_id = BROWSER_TSESSLINE;
		THROW(CheckTblPtr(p_tslt = new TSessLineTbl));
	}
	// IdTSesLnPhQtty

	PPDbqFuncPool::InitObjNameFunc(dbe_user,  PPDbqFuncPool::IdObjNameUser, p_tslt->UserID);
	PPDbqFuncPool::InitObjNameFunc(dbe_flags, PPDbqFuncPool::IdTSesLnFlags, p_tslt->Flags);
	p_dbe_sign   = & enumtoa(p_tslt->Sign, 3, sign_subst.Get(PPTXT_TSESSLN_SIGN));
	p_dbe_price  = &(p_tslt->Price - p_tslt->Discount);
	p_dbe_amount = &((p_tslt->Price - p_tslt->Discount) * p_tslt->Qtty);
	q = & select(p_tslt->TSessID, p_tslt->OprNo, 0); // #0, #1
	q->addField(*p_dbe_sign);            // #2
	PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoods, p_tslt->GoodsID);
	q->addField(dbe_goods);            // #3
	q->addField(p_tslt->Qtty);         // #4
	q->addField(p_tslt->Serial);       // #5
	q->addField(p_tslt->Dt);           // #6
	q->addField(p_tslt->Tm);           // #7
	q->addField(dbe_user);             // #8
	q->addField(*p_dbe_price);         // #9
	{
		dbe_phqtty.init();
		dbe_phqtty.push(p_tslt->GoodsID);
		dbe_phqtty.push(p_tslt->Flags);
		dbe_phqtty.push(p_tslt->Qtty);
		dbe_phqtty.push(p_tslt->WtQtty);
		dbe_phqtty.push((DBFunc)PPDbqFuncPool::IdTSesLnPhQtty);
		q->addField(dbe_phqtty);       // #10
	}
	q->addField(dbe_flags);            // #11
	q->addField(*p_dbe_amount);        // #12
	if(Filt.GoodsID || P_TempTbl) {
		THROW(CheckTblPtr(p_tst = new TSessionTbl));
		PPDbqFuncPool::InitObjNameFunc(dbe_prc, PPDbqFuncPool::IdObjNamePrc, p_tst->PrcID);
		q->addField(dbe_prc);          // #13
		q->addField(p_tst->Num);       // #14
		{
			dbe_datetime.init();
			dbe_datetime.push(p_tst->StDt);
			dbe_datetime.push(p_tst->StTm);
			dbe_datetime.push((DBFunc)PPDbqFuncPool::IdDateTime);
			q->addField(dbe_datetime); // #15
		}
	}
	delete p_dbe_sign;
	delete p_dbe_amount;
	delete p_dbe_price;
	if(!P_TempTbl) {
		dbq = ppcheckfiltid(dbq, p_tslt->TSessID, Filt.TSesList.GetSingle());
		dbq = ppcheckfiltid(dbq, p_tslt->GoodsID, GoodsIdList.getSingle());
		dbq = ppcheckflag(dbq, p_tslt->Flags, TSESLF_OUTREST, (Filt.Flags & TSessLineFilt::fOutRest) ? +1 : -1);
	}
	if(Filt.GoodsID || P_TempTbl) {
		dbq = &(*dbq && p_tst->ID == p_tslt->TSessID);
		q->from(p_tslt, p_tst, 0);
		if(P_TempTbl) {
			q->where(*dbq).orderBy(p_tslt->TSessID, p_tslt->Dt, 0L);
		}
		else {
			dbq = &(*dbq && daterange(p_tst->StDt, &Filt.StPeriod));
			dbq = ppcheckflag(dbq, p_tst->Flags, TSESF_PLAN, -1);
			q->where(*dbq).orderBy(p_tslt->GoodsID, p_tslt->Dt, 0L);
		}
	}
	else {
		q->from(p_tslt, 0).where(*dbq).orderBy(p_tslt->TSessID, p_tslt->Dt, 0L);
	}
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		GetObjectName(PPOBJ_TSESSION, Filt.TSesList.GetSingle(), *pSubTitle, 0);
		if(Filt.GoodsID) {
			SString goods_name;
			pSubTitle->CatDivIfNotEmpty('-', 1).Cat(GetGoodsName(Filt.GoodsID, goods_name));
		}
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete p_tslt;
			delete p_tst;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

void SLAPI PPViewTSessLine::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw && Filt.GoodsID && GoodsIdList.getSingle() == 0) {
		// pBrw->InsColumnWord(1, PPWORD_GOODS, 3, 0, 0, 0);
		pBrw->InsColumn(1, "@ware", 3, 0, 0, 0);
	}
}

int SLAPI PPViewTSessLine::TranslateBrwHdr(const void * pRow, BrwHdr * pHdr)
{
	if(pRow) {
		ASSIGN_PTR(pHdr, *(BrwHdr *)pRow);
		return 1;
	}
	else
		return 0;
}

int SLAPI PPViewTSessLine::Print(const void *)
{
	return Helper_Print(Filt.GoodsID ? REPORT_TSESSLINEGOODS : REPORT_TSESSLINE, 0);
}

int SLAPI PPViewTSessLine::AddItemExt(PPID tsesID, PPViewBrowser * pBrw)
{
	int    ok = -1;
	ExtGoodsSelDialog * dlg = 0;
	if(tsesID) {
		if(!TSesObj.CheckRights(TSESRT_ADDLINE))
			ok = PPErrorZ();
		else if(CheckDialogPtrErr(&(dlg = new ExtGoodsSelDialog(0, NewGoodsGrpID)))) {
			TIDlgInitData tidi;
			PPIDArray goods_list;
			TGSArray tgs_list;
			TSessionTbl::Rec tses_rec;
			ProcessorTbl::Rec prc_rec;
			MEMSZERO(prc_rec);
			if(TSesObj.Search(tsesID, &tses_rec) > 0) {
				if(TSesObj.GetPrc(tses_rec.PrcID, &prc_rec, 1, 1) > 0)
					dlg->setLocation(prc_rec.LocID);
			}
			else
				MEMSZERO(tses_rec);
			const int free_goods_sel = BIN(TSesObj.GetConfig().Flags & PPTSessConfig::fFreeGoodsSelection);
			if(!free_goods_sel && TSesObj.GetGoodsStrucList(tsesID, 1, &tgs_list) > 0 && tgs_list.GetGoodsList(&goods_list) > 0) {
				dlg->setSelectionByGoodsList(&goods_list);
				dlg->setDTS(&tidi);
			}
			else if(NewGoodsGrpID == 0) {
				//GetDefScaleData(&tidi);
				dlg->setDTS(&tidi);
			}
			while(ExecView(dlg) == cmOK) {
				if(dlg->getDTS(&tidi) > 0) {
					long   oprno = 0;
					if(TSesObj.EditLine(tsesID, &oprno, tidi.GoodsID, tidi.Serial, tidi.Quantity) > 0) {
						CALLPTRMEMB(pBrw, Update());
						ok = 1;
					}
				}
			}
			delete dlg;
		}
	}
	return ok;
}

int SLAPI PPViewTSessLine::AddItemByCode(const char * pInitStr)
{
	int    ok = -1;
	PPID   sess_id = Filt.TSesList.GetSingle();
	if(sess_id) {
		if(!TSesObj.CheckRights(TSESRT_ADDLINE))
			ok = PPErrorZ();
		else {
			long   oprno = 0;
			SString code;
			Goods2Tbl::Rec goods_rec;
			double qtty = 0.0;
			int    init_chr = pInitStr ? pInitStr[0] : 0;
			if(GObj.SelectGoodsByBarcode(init_chr, 0, &goods_rec, &qtty, &code) > 0) {
				TIDlgInitData tidi;
				SETFLAG(tidi.Flags, TIDIF_AUTOQTTY, 1);
				tidi.GoodsID  = goods_rec.ID;
				if(TSesObj.EditLine(sess_id, &(oprno = 0), tidi.GoodsID, 0, qtty) > 0)
					ok = 1;
			}
			else {
				PPObjTSession::SelectBySerialParam ssp(sess_id, code);
				int    r2 = TSesObj.SelectBySerial(&ssp);
				if(r2 == 1) {
					if(TSesObj.EditLine(sess_id, &(oprno = 0), ssp.GoodsID, ssp.Serial, ssp.Qtty) > 0)
						ok = 1;
				}
				else if(oneof2(r2, 2, -2))
					PPError();
			}
		}
	}
	return ok;
}

int SLAPI PPViewTSessLine::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	BrwHdr hdr;
	long   oprno = 0;
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_INPUTCHAR:
				ok = (AddItemByCode((const char *)pHdr) > 0) ? 1 : -1;
				break;
			case PPVCMD_ADDITEM:
				if(Filt.TSesList.GetSingle())
					ok = (TSesObj.EditLine(Filt.TSesList.GetSingle(), &oprno, 0, 0, 0) > 0) ? 1 : -1;
				break;
			case PPVCMD_EDITITEM:
				if(TranslateBrwHdr(pHdr, &hdr) && hdr.OprNo)
					ok = (TSesObj.EditLine(hdr.TSesID, &hdr.OprNo, 0, 0, 0) > 0) ? 1 : -1;
				break;
			case PPVCMD_DELETEITEM:
				if(TranslateBrwHdr(pHdr, &hdr) && hdr.OprNo) {
					TSessionTbl::Rec sess_rec;
					if(TSesObj.CheckRights(TSESRT_DELLINE) && TSesObj.Search(hdr.TSesID, &sess_rec) > 0) {
						if(!sess_rec.Incomplete && !(TSesObj.GetConfig().Flags & PPTSessConfig::fAllowLinesInWrOffSessions))
							PPError(PPERR_RMVLNCOMPLTSESS);
						else {
							if(CONFIRM(PPCFM_DELETE)) {
								ok = TSesObj.PutLine(hdr.TSesID, &hdr.OprNo, 0, 1) ? 1 : PPErrorZ();
							}
							else
								ok = -1;
						}
					}
					else
						ok = PPErrorZ();
				}
				break;
			case PPVCMD_EXTADDGOODSITEM:
				if(Filt.TSesList.GetSingle())
					ok = AddItemExt(Filt.TSesList.GetSingle(), pBrw);
				break;
			case PPVCMD_COMPLETE:
				{
					const PPID tses_id = (TranslateBrwHdr(pHdr, &hdr) && hdr.TSesID) ? hdr.TSesID : Filt.TSesList.GetSingle();
					if(tses_id) {
						ok = TSesObj.Complete(tses_id, 1);
						if(!ok)
							PPError();
					}
				}
				break;
			case PPVCMD_VIEWLOTS:
				ok = -1;
				if(TranslateBrwHdr(pHdr, &hdr) && hdr.OprNo) {
					TSessLineTbl::Rec line_rec;
					if(TSesObj.P_Tbl->SearchLine(hdr.TSesID, hdr.OprNo, &line_rec) > 0 && line_rec.GoodsID)
						::ViewLots(line_rec.GoodsID, 0, 0, 0, 0);
				}
				break;
			case PPVCMD_EDITTSESS:
				ok = -1;
				if(TranslateBrwHdr(pHdr, &hdr) && hdr.TSesID)
					TSesObj.Edit(&hdr.TSesID, 0);
				break;
			case PPVCMD_PRINTLABEL:
				if(TranslateBrwHdr(pHdr, &hdr) && hdr.OprNo)
					TSesObj.PrintBarLabel(hdr.TSesID, hdr.OprNo, 0, 0);
				break;
		}
	}
	return ok;
}

int SLAPI PPViewTSession::GetSelectorListInfo(StrAssocArray & rList) const
{
	if(P_UhttsPack && P_UhttsPack->Sd.GetCount()) {
		rList = ExtSfTitleList;
		return 1;
	}
	else {
		rList.Clear();
		return 0;
	}
}

int SLAPI PPViewTSession::GetSelectorListItem(long handler, PPUhttStoreSelDescr::Entry & rEntry) const
{
	int    ok = 0;
	if(P_UhttsPack && handler > 0 && handler <= (long)P_UhttsPack->Sd.GetCount())
		ok = P_UhttsPack->Sd.GetEntry(handler-1, rEntry);
	return ok;
}

int SLAPI ViewTSessLine(const TSessLineFilt * pFilt)
{
	int    ok = 1, view_in_use = 0;
	int    modeless = GetModelessStatus();
	TSessLineFilt flt;
	PPViewTSessLine * p_v = new PPViewTSessLine;
	PPViewBrowser * p_prev_win = modeless ? (PPViewBrowser *)PPFindLastBrowser() : 0;
	if(pFilt)
		flt = *pFilt;
	else if(p_prev_win)
		flt = *((PPViewTSessLine *)(p_prev_win->P_View))->GetFilt();
	else
		flt.Init();
	THROW(p_v->Init(&flt));
	PPCloseBrowser(p_prev_win);
	THROW(p_v->Browse(modeless));
	if(modeless || pFilt)
		view_in_use = 1;
	CATCHZOKPPERR
	if(!modeless || !ok || !view_in_use)
		delete p_v;
	return ok;
}
//
// Implementation of PPALDD_TSession
//
PPALDD_CONSTRUCTOR(TSession)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjTSession;
	}
}

PPALDD_DESTRUCTOR(TSession)
{
	Destroy();
	delete (PPObjTSession *)Extra[0].Ptr;
}

int PPALDD_TSession::InitData(PPFilt & rFilt, long rsrv)
{
	if(rFilt.ID == H.ID)
		return DlRtm::InitData(rFilt, rsrv);
	MEMSZERO(H);
	H.ID = rFilt.ID;
	TSessionTbl::Rec rec, super_rec;
	SString temp_buf;
	PPObjTSession * p_ses_obj = (PPObjTSession *)(Extra[0].Ptr);
	if(p_ses_obj->Search(rFilt.ID, &rec) > 0) {
		H.ID = rec.ID;
		H.ParentID = rec.ParentID;
		H.Number   = rec.Num;
		H.TechID   = rec.TechID;
		H.PrcID    = rec.PrcID;
		H.CCheckID = rec.CCheckID_; // @v8.3.0
		H.SCardID  = rec.SCardID;   // @v8.3.0
		H.LinkBillID = rec.LinkBillID; // @v8.3.0
		H.StDt     = rec.StDt;
		H.StTm     = rec.StTm;
		timefmt(rec.StTm, TIMF_HMS, H.StTmTxt);
		H.FinDt    = rec.FinDt;
		H.FinTm    = rec.FinTm;
		timefmt(rec.FinTm, TIMF_HMS, H.FinTmTxt);
		H.Incomplete = rec.Incomplete;
		H.Status   = rec.Status;
		H.Idle     = BIN(rec.Flags & TSESF_IDLE);
		p_ses_obj->MakeName(&rec, temp_buf);
		temp_buf.CopyTo(H.Name, sizeof(H.Name));
		if(rec.ParentID && p_ses_obj->Search(rec.ParentID, &super_rec) > 0) {
			p_ses_obj->MakeName(&rec, temp_buf);
			temp_buf.CopyTo(H.SuperName, sizeof(H.SuperName));
		}
		p_ses_obj->GetStatusText(rec.Status, temp_buf);
		temp_buf.CopyTo(H.StatusText, sizeof(H.StatusText));
		H.Flags    = rec.Flags;
		H.ArID     = rec.ArID;
		H.Ar2ID    = rec.Ar2ID;
		H.PlannedTiming = rec.PlannedTiming;
		H.PlannedQtty   = rec.PlannedQtty;
		long   act_timing = diffdatetimesec(rec.FinDt, rec.FinTm, rec.StDt, rec.StTm);
		SETMAX(act_timing, 0);
		H.ActTiming = act_timing;
		H.ActQtty   = rec.ActQtty;
		{
			long h, m, s;
			if(rec.PlannedTiming > 0) {
				h = rec.PlannedTiming / 3600;
				m = (rec.PlannedTiming % 3600) / 60;
				s = (rec.PlannedTiming % 60);
				temp_buf.Z().CatLongZ(h, 2).CatChar(':').CatLongZ(m, 2).CatChar(':').CatLongZ(s, 2);
			}
			else
				temp_buf.Z();
			temp_buf.CopyTo(H.PlnTimingText, sizeof(H.PlnTimingText));
			if(act_timing > 0) {
				h = act_timing / 3600;
				m = (act_timing % 3600) / 60;
				s = (act_timing % 60);
				temp_buf.Z().CatLongZ(h, 2).CatChar(':').CatLongZ(m, 2).CatChar(':').CatLongZ(s, 2);
			}
			else
				temp_buf.Z();
			temp_buf.CopyTo(H.ActTimingText, sizeof(H.ActTimingText));
		}
		H.Amount = rec.Amount;
		STRNSCPY(H.Memo, rec.Memo);
		return DlRtm::InitData(rFilt, rsrv);
	}
	return -1;
}

void PPALDD_TSession::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_STR(n)  (**(SString **)rS.GetPtr(pApl->Get(n)))
	#define _RET_INT     (*(int *)rS.GetPtr(pApl->Get(0)))

	_RET_INT = 0;
	if(pF->Name == "?GetWrOffMemberByOp") {
		PPObjBill * p_bobj = BillObj;
		if(p_bobj) {
			PPObjOprKind op_obj;
			PPID   op_id = 0;
			if(op_obj.SearchBySymb(_ARG_STR(1), &op_id) > 0) {
				for(PPID memb_id = 0; p_bobj->EnumMembersOfPool(PPASS_TSESSBILLPOOL, H.ID, &memb_id) > 0;) {
					BillTbl::Rec bill_rec;
					if(p_bobj->Search(memb_id, &bill_rec) > 0 && bill_rec.OpID == op_id) {
						_RET_INT = memb_id;
						break;
					}
				}
			}
		}
	}
	else if(pF->Name == "?GetTag") {
		_RET_INT = PPObjTag::Helper_GetTag(PPOBJ_TSESSION, H.ID, _ARG_STR(1));
	}
}
//
// Implementation of PPALDD_TSessionView
//
PPALDD_CONSTRUCTOR(TSessionView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(TSessionView)
{
	Destroy();
}

int PPALDD_TSessionView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(TSession, rsrv);
	H.FltSuperID = p_filt->SuperSessID;
	H.FltPrcID   = p_filt->PrcID;
	H.FltTechID  = p_filt->TechID;
	H.FltArID    = p_filt->ArID;
	H.FltAr2ID   = p_filt->Ar2ID;
	H.FltStBeg   = p_filt->StPeriod.low;
	H.FltStEnd   = p_filt->StPeriod.upp;
	H.FltFnBeg   = p_filt->FnPeriod.low;
	H.FltFnEnd   = p_filt->FnPeriod.upp;
	H.Ft_Idle    = p_filt->Ft_Idle;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_TSessionView::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER_ORD(TSession, sortId);
}

int PPALDD_TSessionView::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(TSession);
	I.SessID = item.ID;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_TSessionView::Destroy()
{
	DESTROY_PPVIEW_ALDD(TSession);
}
//
// Implementation of PPALDD_TSessionCipView
//
PPALDD_CONSTRUCTOR(TSessionCipView)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(TSessionCipView)
{
	Destroy();
}

int PPALDD_TSessionCipView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(TSession, rsrv);
	H.FltSuperID = p_filt->SuperSessID;
	H.FltPrcID   = p_filt->PrcID;
	H.FltTechID  = p_filt->TechID;
	H.FltArID    = p_filt->ArID;
	H.FltAr2ID   = p_filt->Ar2ID;
	H.FltStBeg   = p_filt->StPeriod.low;
	H.FltStEnd   = p_filt->StPeriod.upp;
	H.FltFnBeg   = p_filt->FnPeriod.low;
	H.FltFnEnd   = p_filt->FnPeriod.upp;
	H.Ft_Idle    = p_filt->Ft_Idle;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_TSessionCipView::InitIteration(long iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER_ORD(TSession, sortId & PPViewTSession::ordfWithCip);
}

int PPALDD_TSessionCipView::NextIteration(long iterId)
{
	START_PPVIEW_ALDD_ITER(TSession);
	I.SessID = item.ID;
	I.CipID  = item.CipItem.ID;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_TSessionCipView::Destroy()
{
	DESTROY_PPVIEW_ALDD(TSession);
}
//
// Implementation of PPALDD_TSessionBillView
//
PPALDD_CONSTRUCTOR(TSessionBillView)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(TSessionBillView)
{
	Destroy();
}

int PPALDD_TSessionBillView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(TSession, rsrv);
	H.FltSuperID = p_filt->SuperSessID;
	H.FltPrcID   = p_filt->PrcID;
	H.FltTechID  = p_filt->TechID;
	H.FltArID    = p_filt->ArID;
	H.FltAr2ID   = p_filt->Ar2ID;
	H.FltStBeg   = p_filt->StPeriod.low;
	H.FltStEnd   = p_filt->StPeriod.upp;
	H.FltFnBeg   = p_filt->FnPeriod.low;
	H.FltFnEnd   = p_filt->FnPeriod.upp;
	H.Ft_Idle    = p_filt->Ft_Idle;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_TSessionBillView::InitIteration(long iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER_ORD(TSession, sortId & PPViewTSession::ordfWithBill);
}

int PPALDD_TSessionBillView::NextIteration(long iterId)
{
	START_PPVIEW_ALDD_ITER(TSession);
	I.SessID = item.ID;
	I.BillID = item.WrOffBillID;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_TSessionBillView::Destroy()
{
	DESTROY_PPVIEW_ALDD(TSession);
}
//
// Implementation of PPALDD_TSessLineView
//
PPALDD_CONSTRUCTOR(TSessLineView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(TSessLineView)
{
	Destroy();
}

int PPALDD_TSessLineView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA(TSessLine, rsrv);
	H.FltSessID  = p_filt->TSesList.GetSingle();
	H.FltGoodsID = p_filt->GoodsID;
	H.FltFlags   = p_filt->Flags;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_TSessLineView::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(TSessLine);
}

int PPALDD_TSessLineView::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(TSessLine);
	I.SessID  = item.TSessID;
	I.GoodsID = item.GoodsID;
	I.UserID  = item.UserID;
	I.OprNo   = item.OprNo;
	I.Sign    = item.Sign;
	I.Dt      = item.Dt;
	I.Tm      = item.Tm;
	I.Flags   = item.Flags;
	I.Qtty    = item.Qtty;
	I.Price   = item.Price;
	STRNSCPY(I.Serial, item.Serial);
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_TSessLineView::Destroy()
{
	DESTROY_PPVIEW_ALDD(TSessLine);
}
//
//
//
#define USE_IMPL_DL6ICLS_PPFiltTSession
#define USE_IMPL_DL6ICLS_PPViewTSession
#include "..\rsrc\dl600\ppifc_auto.cpp"

DL6_IC_CONSTRUCTION_EXTRA(PPFiltTSession, DL6ICLS_PPFiltTSession_VTab, TSessionFilt);
//
// Interface IPpyFilt_TSession implementation
//
void DL6ICLS_PPFiltTSession::SetStPeriod(LDATE low, LDATE upp) { IMPL_PPIFC_EXTPTR(TSessionFilt)->StPeriod.Set(low, upp); }
void DL6ICLS_PPFiltTSession::SetFnPeriod(LDATE low, LDATE upp) { IMPL_PPIFC_EXTPTR(TSessionFilt)->FnPeriod.Set(low, upp); }
int32 DL6ICLS_PPFiltTSession::get_Order() { IMPL_PPIFC_GETPROP(TSessionFilt, Order); }
void  DL6ICLS_PPFiltTSession::put_Order(int32 value) { IMPL_PPIFC_PUTPROP(TSessionFilt, Order); }
int32 DL6ICLS_PPFiltTSession::get_SuperSessID() { IMPL_PPIFC_GETPROP(TSessionFilt, SuperSessID); }
void  DL6ICLS_PPFiltTSession::put_SuperSessID(int32 value) { IMPL_PPIFC_PUTPROP(TSessionFilt, SuperSessID); }
int32 DL6ICLS_PPFiltTSession::get_PrcID() { IMPL_PPIFC_GETPROP(TSessionFilt, PrcID); }
void  DL6ICLS_PPFiltTSession::put_PrcID(int32 value) { IMPL_PPIFC_PUTPROP(TSessionFilt, PrcID); }
int32 DL6ICLS_PPFiltTSession::get_TechID() { IMPL_PPIFC_GETPROP(TSessionFilt, TechID); }
void  DL6ICLS_PPFiltTSession::put_TechID(int32 value) { IMPL_PPIFC_PUTPROP(TSessionFilt, TechID); }
int32 DL6ICLS_PPFiltTSession::get_ArID() { IMPL_PPIFC_GETPROP(TSessionFilt, ArID); }
void  DL6ICLS_PPFiltTSession::put_ArID(int32 value) { IMPL_PPIFC_PUTPROP(TSessionFilt, ArID); }
int32 DL6ICLS_PPFiltTSession::get_Ar2ID() { IMPL_PPIFC_GETPROP(TSessionFilt, Ar2ID); }
void  DL6ICLS_PPFiltTSession::put_Ar2ID(int32 value) { IMPL_PPIFC_PUTPROP(TSessionFilt, Ar2ID); }
SDateRange DL6ICLS_PPFiltTSession::get_StPeriod() { return DateRangeToOleDateRange(((TSessionFilt *)ExtraPtr)->StPeriod); }
LTIME DL6ICLS_PPFiltTSession::get_StTime() { IMPL_PPIFC_GETPROP(TSessionFilt, StTime); }
void  DL6ICLS_PPFiltTSession::put_StTime(LTIME value) { IMPL_PPIFC_PUTPROP(TSessionFilt, StTime); }
SDateRange DL6ICLS_PPFiltTSession::get_FnPeriod() { return DateRangeToOleDateRange(((TSessionFilt *)ExtraPtr)->FnPeriod); }
LTIME DL6ICLS_PPFiltTSession::get_FnTime() { IMPL_PPIFC_GETPROP(TSessionFilt, FnTime); }
void  DL6ICLS_PPFiltTSession::put_FnTime(LTIME value) { IMPL_PPIFC_PUTPROP(TSessionFilt, FnTime); }
int32 DL6ICLS_PPFiltTSession::get_StatusFlags() { IMPL_PPIFC_GETPROP(TSessionFilt, StatusFlags); }
void  DL6ICLS_PPFiltTSession::put_StatusFlags(int32 value) { IMPL_PPIFC_PUTPROP(TSessionFilt, StatusFlags); }
PpyVTSessionFlags DL6ICLS_PPFiltTSession::get_Flags() { IMPL_PPIFC_GETPROP_CAST(TSessionFilt, StatusFlags, PpyVTSessionFlags); }
void  DL6ICLS_PPFiltTSession::put_Flags(PpyVTSessionFlags value) { IMPL_PPIFC_PUTPROP(TSessionFilt, StatusFlags); }
int32 DL6ICLS_PPFiltTSession::get_Ft_Idle() { IMPL_PPIFC_GETPROP(TSessionFilt, Ft_Idle); }
void  DL6ICLS_PPFiltTSession::put_Ft_Idle(int32 value) { IMPL_PPIFC_PUTPROP_CAST(TSessionFilt, Ft_Idle, int16); }
//
//
//
DL6_IC_CONSTRUCTION_EXTRA(PPViewTSession, DL6ICLS_PPViewTSession_VTab, PPViewTSession);
//
// Interface IPapyrusView implementation
//
IUnknown* DL6ICLS_PPViewTSession::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltTSession", 0, (void **)&p_filt) ? p_filt : (IUnknown *)RaiseAppErrorPtr();
}

int32 DL6ICLS_PPViewTSession::Init(IUnknown* pFilt)
{
	IPpyFilt_TSession * p_ifc_filt = 0;
	S_GUID uuid;
	THROW_INVARG(pFilt);
	THROW(GetInnerUUID("IPpyFilt_TSession", uuid));
	THROW(SUCCEEDED(pFilt->QueryInterface(uuid, (void **)&p_ifc_filt)));
	THROW(((PPViewTSession *)ExtraPtr)->Init_((const TSessionFilt *)GetExtraPtrByInterface(p_ifc_filt)));
	CATCH
		AppError = 1;
	ENDCATCH
	CALLTYPEPTRMEMB(IUnknown, p_ifc_filt, Release());
	return !AppError;
}

int32 DL6ICLS_PPViewTSession::InitIteration(int32 order)
{
	return ((PPViewTSession *)ExtraPtr)->InitIteration(order);
}

int32 DL6ICLS_PPViewTSession::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SString temp_buf;
	SPpyO_TSession * p_item = (SPpyO_TSession *)item;
	TSessionViewItem inner_item;
	if(IMPL_PPIFC_EXTPTR(PPViewTSession)->NextIteration(&inner_item) > 0) {
		SString temp_buf;
		p_item->RecTag = PPVIEWITEM_TSESSION;
#define FLD(f) p_item->f = inner_item.f
		FLD(ID);
		FLD(ParentID);
		FLD(Num);
		FLD(TechID);
		FLD(PrcID);
		p_item->StDt = (OleDate)inner_item.StDt;
		p_item->StTm = (OleDate)inner_item.StTm;
		p_item->FinDt = (OleDate)inner_item.FinDt;
		p_item->FinTm = (OleDate)inner_item.FinTm;
		FLD(Incomplete);
		FLD(Status);
		p_item->Flags = (PpyOTSessionFlags)inner_item.Flags;
		FLD(ArID);
		FLD(Ar2ID);
		FLD(PlannedTiming);
		FLD(PlannedQtty);
		FLD(ActQtty);
		FLD(OrderLotID);
		FLD(PrevSessID);
		FLD(Amount);
		FLD(LinkBillID);
		FLD(SCardID);
		FLD(ToolingTime);
		(temp_buf = inner_item.Memo).CopyToOleStr(&p_item->Memo);
#undef FLD
		ok = 1;
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewTSession::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewTSession::GetTotal(PPYVIEWTOTAL total)
{
	IMPL_PPIFC_EXTPTRVAR(PPViewTSession);
	if(p_ext && total) {
		TSessionTotal inner_total;
		SPpyVT_TSession * p_total = (SPpyVT_TSession *)total;
		if(p_ext->CalcTotal(&inner_total)) {
			p_total->Count    = inner_total.Count;
			p_total->Duration = inner_total.Duration;
		}
		else
			AppError = 1;
	}
	return !AppError;
}
//
//
//
int SLAPI PPObjTSession::ConvertPacket(const UhttTSessionPacket * pSrc, long flags, TSessionPacket & rDest)
{
	int    ok = 1;
	rDest.destroy();
	if(pSrc) {
		uint   i;
		PPObjTag tag_obj;
		S_GUID   uuid;
		uuid.SetZero();
        for(i = 0; i < pSrc->TagList.getCount(); i++) {
			const UhttTagItem * p_uhtt_tag_item = pSrc->TagList.at(i);
			PPID   tag_id = 0;
			PPObjectTag tag_rec;
			if(tag_obj.SearchBySymb(p_uhtt_tag_item->Symb, &tag_id, &tag_rec) > 0) {
				assert(tag_id == tag_rec.ID); // @paranoic
				ObjTagItem tag_item;
				tag_item.TagID = tag_id;
				THROW(tag_item.SetStr(tag_id, p_uhtt_tag_item->Value));
				THROW(rDest.TagL.PutItem(tag_id, &tag_item));
				if(tag_id == PPTAG_TSESS_UUID) {
                    tag_item.GetGuid(&uuid);
				}
			}
        }
		rDest.Rec.Status = pSrc->Status;
		rDest.Rec.Flags = pSrc->Flags;
		rDest.Rec.StDt = pSrc->StTime;
		rDest.Rec.StTm = pSrc->StTime;
		rDest.Rec.FinDt = pSrc->FinTime;
		rDest.Rec.FinTm = pSrc->FinTime;
		STRNSCPY(rDest.Rec.Memo, pSrc->Memo);
		for(i = 0; i < pSrc->Cips.getCount(); i++) {
			const UhttCipPacket * p_uhtt_cip = pSrc->Cips.at(i);
			if(p_uhtt_cip) {
				PPCheckInPersonItem cip_item;
				uint   cip_pos = 0;
				cip_item.Kind = PPCheckInPersonItem::kTSession;
				cip_item.PrmrID = 0; // this tsession id
				if(p_uhtt_cip->Flags & PPCheckInPersonItem::fAnonym) {
					cip_item.SetAnonym();
				}
				else {
					// @todo setup person
				}
				cip_item.Flags = p_uhtt_cip->Flags;
				cip_item.RegCount = p_uhtt_cip->RegCount;
				cip_item.CiCount = p_uhtt_cip->CiCount;
				cip_item.RegDtm = p_uhtt_cip->RegTm;
				cip_item.CiDtm = p_uhtt_cip->CiTm;
				cip_item.Amount = p_uhtt_cip->Amount;
				STRNSCPY(cip_item.PlaceCode, p_uhtt_cip->PlaceCode);
				THROW(rDest.CiList.AddItem(cip_item, 0, &cip_pos));
				rDest.CiList.SetMemo(cip_pos, p_uhtt_cip->Memo);
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTSession::ImportUHTT()
{
	int    ok = -1;
	SString msg_buf, fmt_buf, temp_buf, loc_text_buf;
	SString tsess_text;
	PPLogger logger;
	PPUhttClient uhtt_cli;
	PPWait(1);
	THROW(uhtt_cli.Auth());
	{
		PPID   prc_id = 0;
		LDATETIME since;
		since.SetZero();

		TSCollection <UhttTSessionPacket> uhtt_tses_list;
		THROW(uhtt_cli.GetTSessionByPrc(prc_id, since, uhtt_tses_list));
		for(uint i = 0; i < uhtt_tses_list.getCount(); i++) {
			const UhttTSessionPacket * p_uhtt_tses_pack = uhtt_tses_list.at(i);
			if(p_uhtt_tses_pack) {
				int    do_turn = 0;
				TSessionPacket pack, org_pack;
				S_GUID uuid;
				THROW(ConvertPacket(p_uhtt_tses_pack, 0, pack));
				const ObjTagItem * p_uuid_tag = pack.TagL.GetItem(PPTAG_TSESS_UUID);
				if(p_uuid_tag && p_uuid_tag->GetGuid(&uuid)) {
					TSessionTbl::Rec rec;
					if(SearchByGuid(uuid, &rec) > 0) {
						PPID   org_pack_id = rec.ID;
						if(GetPacket(org_pack_id, &org_pack, 0) > 0) {
							PPCheckInPersonConfig cipc(*this, org_pack);
							THROW(cipc);
							org_pack.CiList.Clear();
							for(uint j = 0; j < pack.CiList.GetCount(); j++) {
								const PPCheckInPersonItem & r_cip_item = pack.CiList.Get(j);
                                THROW(org_pack.CiList.AddItem(r_cip_item, &cipc, 0));
							}
							do_turn = 1;
							THROW(PutPacket(&org_pack_id, &org_pack, 1));
						}
					}
				}
			}
		}
	}
	PPWait(0);
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}
