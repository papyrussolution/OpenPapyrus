// V_TSESS.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>
//
// @ModuleDef(PPViewTSession)
//
void TSessionFilt::Helper_Init()
{
	SetFlatChunk(offsetof(TSessionFilt, ReserveStart),
		offsetof(TSessionFilt, Reserve)-offsetof(TSessionFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

IMPLEMENT_PPFILT_FACTORY(TSession); TSessionFilt::TSessionFilt() : PPBaseFilt(PPFILT_TSESSION, 0, 0)
{
	Helper_Init();
}

TSessionFilt::TSessionFilt(const TSessionFilt & rS) : PPBaseFilt(PPFILT_TSESSION, 0, 0)
{
	Helper_Init();
	Copy(&rS, 1);
}

TSessionFilt & FASTCALL TSessionFilt::operator = (const TSessionFilt & rS)
{
	Copy(&rS, 1);
	return *this;
}

bool FASTCALL TSessionFilt::CheckIdle(long flags) const
{
	return (!Ft_Idle) ? true : ((Ft_Idle < 0) ? ((flags & TSESF_IDLE) ? false : true) : ((flags & TSESF_IDLE) ? true : false));
}

bool FASTCALL TSessionFilt::CheckWrOff(long flags) const
{
	return (!Ft_WritedOff) ? true : ((Ft_WritedOff < 0) ? ((flags & TSESF_WRITEDOFF) ? false : true) : ((flags & TSESF_WRITEDOFF) ? true : false));
}

bool FASTCALL TSessionFilt::CheckStatus(int status) const { return LOGIC(!StatusFlags || (StatusFlags & (1 << status))); }

int FASTCALL TSessionFilt::GetStatusList(PPIDArray & rList) const
{
	if(StatusFlags) {
		for(int i = 1; i <= 5; i++)
			if(StatusFlags & (1 << i))
				rList.addUnique(i);
		return 1;
	}
	else
		return -1;
}
//
//
//
TSessionViewItem::TSessionViewItem() : WrOffBillID(0)
{
}

TSessionViewItem & TSessionViewItem::Z()
{
	memzero(static_cast<TSessionTbl::Rec *>(this), sizeof(TSessionTbl::Rec));
	CipItem.Z();
	WrOffBillID = 0;
	SMemo.Z();
	return *this;
}
//
//
//
PPViewTSession::UhttStoreExt::UhttStoreExt() : ID(0)
{
}

PPViewTSession::UhttStoreExt & PPViewTSession::UhttStoreExt::Z()
{
	ID = 0;
	SfList.Z();
	return *this;
}

PPViewTSession::IterBlock::IterBlock()
{
	Init(0);
}

PPViewTSession::IterBlock & FASTCALL PPViewTSession::IterBlock::Init(int order)
{
	Order = order;
	CipList.Z();
	WrOffBillList.clear();
	CurItem.Z();
	return *this;
}
//
PPViewTSession::PPViewTSession() : PPView(&TSesObj, &Filt, PPVIEW_TSESSION, implOnAddSetupPos, 0), P_TempTbl(0), P_LastAnlzFilt(0), P_UhttsPack(0), State(0)
{
}

PPViewTSession::~PPViewTSession()
{
	delete P_UhttsPack;
	delete P_TempTbl;
	delete P_LastAnlzFilt;
}

PPBaseFilt * PPViewTSession::CreateFilt(const void * extraPtr) const
{
	PPBaseFilt * p_base_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_TSESSION, &p_base_filt)) {
		static_cast<TSessionFilt *>(p_base_filt)->StatusFlags |= 0x000000ffL;
		if(reinterpret_cast<long>(extraPtr) == TSESK_PLAN)
			static_cast<TSessionFilt *>(p_base_filt)->Flags |= TSessionFilt::fManufPlan;
	}
	return p_base_filt;
}

void * PPViewTSession::GetEditExtraParam()
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

int PPViewTSession::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class TSessFiltDialog : public TDialog {
		DECL_DIALOG_DATA(TSessionFilt);
		enum {
			ctlgroupPrcTech = 2
		};
	public:
		explicit TSessFiltDialog(uint dlgId) : TDialog(dlgId/*DLG_TSESSFILT*/)
		{
 			SetupCalPeriod(CTLCAL_TSESSFILT_STPERIOD, CTL_TSESSFILT_STPERIOD);
		 	SetupCalPeriod(CTLCAL_TSESSFILT_FNPERIOD, CTL_TSESSFILT_FNPERIOD);
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			int    ok = 1;
			PrcTechCtrlGroup::Rec ptcg_rec;
			//
			ptcg_rec.PrcID = Data.PrcID;
			if(Data.Flags & TSessionFilt::fManufPlan) {
				PrcTechCtrlGroup * p_grp = new PrcTechCtrlGroup(CTLSEL_TSESSFILT_PRC, 0, 0, 0, 0, 0);
				THROW_SL(p_grp);
				p_grp->enableTechSelUpLevel(true); // @v11.7.6
 				addGroup(ctlgroupPrcTech, p_grp);
				if(Data.PrcID == 0)
					ptcg_rec.PrcParentID = PRCEXDF_GROUP;
 				setGroupData(ctlgroupPrcTech, &ptcg_rec);
			}
			else {
				PrcTechCtrlGroup * p_grp = new PrcTechCtrlGroup(CTLSEL_TSESSFILT_PRC, CTLSEL_TSESSFILT_TECH,
 					CTL_TSESSFILT_ST_GOODS, CTLSEL_TSESSFILT_AR, CTLSEL_TSESSFILT_AR2, cmSelTechByGoods);
				THROW_SL(p_grp);
				p_grp->setIdleStatus(this, (Data.Ft_Idle > 0));
				p_grp->enableTechSelUpLevel(true); // @v11.7.6
 				addGroup(ctlgroupPrcTech, p_grp);
				ptcg_rec.TechID = Data.TechID;
				ptcg_rec.ArID   = Data.ArID;
				ptcg_rec.Ar2ID  = Data.Ar2ID;
				ptcg_rec.IdleStatus = (Data.Ft_Idle > 0);
 				setGroupData(ctlgroupPrcTech, &ptcg_rec);
			}
			AddClusterAssocDef(CTL_TSESSFILT_STATUS, 0, (1 << TSESST_PLANNED));
			AddClusterAssoc(CTL_TSESSFILT_STATUS, 1, (1 << TSESST_PENDING));
			AddClusterAssoc(CTL_TSESSFILT_STATUS, 2, (1 << TSESST_INPROCESS));
			AddClusterAssoc(CTL_TSESSFILT_STATUS, 3, (1 << TSESST_CLOSED));
			AddClusterAssoc(CTL_TSESSFILT_STATUS, 4, (1 << TSESST_CANCELED));
			SetClusterData(CTL_TSESSFILT_STATUS, Data.StatusFlags);
			AddClusterAssoc(CTL_TSESSFILT_FLAGS, 0, TSessionFilt::fSuperSessOnly);
			SetClusterData(CTL_TSESSFILT_FLAGS, Data.Flags);
			AddClusterAssocDef(CTL_TSESSFILT_IDLE,  0,  0);
			AddClusterAssoc(CTL_TSESSFILT_IDLE,  1, -1);
			AddClusterAssoc(CTL_TSESSFILT_IDLE,  2,  1);
			SetClusterData(CTL_TSESSFILT_IDLE, Data.Ft_Idle);
			// @v11.0.6 {
			AddClusterAssocDef(CTL_TSESSFILT_WROFF, 0,  0);
			AddClusterAssocDef(CTL_TSESSFILT_WROFF, 1, -1);
			AddClusterAssocDef(CTL_TSESSFILT_WROFF, 2,  1);
			SetClusterData(CTL_TSESSFILT_WROFF, Data.Ft_WritedOff);
			// } @v11.0.6 
			SetPeriodInput(this, CTL_TSESSFILT_STPERIOD, Data.StPeriod);
			setCtrlData(CTL_TSESSFILT_STTIME, &Data.StTime);
			SetPeriodInput(this, CTL_TSESSFILT_FNPERIOD, Data.FnPeriod);
			setCtrlData(CTL_TSESSFILT_FNTIME, &Data.FnTime);
			SetupStringCombo(this, CTLSEL_TSESSFILT_ORDER, PPTXT_TSESSORDER, Data.Order);
			SetupPPObjCombo(this, CTLSEL_TSESSFILT_GDSGRP, PPOBJ_GOODSGROUP, Data.GoodsGroupID, OLW_CANSELUPLEVEL|OLW_WORDSELECTOR); // @v11.7.8
			//
			CATCHZOK
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			if(Data.Flags & TSessionFilt::fManufPlan) {
				getCtrlData(CTLSEL_TSESSFILT_PRC, &Data.PrcID);
			}
			else {
				PrcTechCtrlGroup::Rec ptcg_rec;
				getGroupData(ctlgroupPrcTech, &ptcg_rec);
				Data.PrcID  = ptcg_rec.PrcID;
				Data.TechID = ptcg_rec.TechID;
				Data.ArID   = ptcg_rec.ArID;
				Data.Ar2ID  = ptcg_rec.Ar2ID;
			}
			long   temp_long = 0;
			GetClusterData(CTL_TSESSFILT_STATUS, &Data.StatusFlags);
			GetClusterData(CTL_TSESSFILT_FLAGS, &Data.Flags);
			GetClusterData(CTL_TSESSFILT_IDLE, &temp_long);
			Data.Ft_Idle = static_cast<int16>(temp_long);
			// @v11.0.6 {
			temp_long = 0;
			GetClusterData(CTL_TSESSFILT_WROFF, &temp_long);
			Data.Ft_WritedOff = static_cast<int16>(temp_long);
			// } @v11.0.6 
			GetPeriodInput(this, CTL_TSESSFILT_STPERIOD, &Data.StPeriod);
			getCtrlData(CTL_TSESSFILT_STTIME, &Data.StTime);
			if(!Data.StPeriod.low)
				Data.StTime = ZEROTIME;
			GetPeriodInput(this, CTL_TSESSFILT_FNPERIOD, &Data.FnPeriod);
			getCtrlData(CTL_TSESSFILT_FNTIME, &Data.FnTime);
			if(!Data.FnPeriod.upp)
				Data.FnTime = ZEROTIME;
			getCtrlData(CTLSEL_TSESSFILT_ORDER, &Data.Order);
			getCtrlData(CTLSEL_TSESSFILT_GDSGRP, &Data.GoodsGroupID); // @v11.7.8
			//
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_TSESSFILT_IDLE)) {
				const long temp_long = GetClusterData(CTL_TSESSFILT_IDLE);
				PrcTechCtrlGroup * p_grp = static_cast<PrcTechCtrlGroup *>(getGroup(ctlgroupPrcTech));
				CALLPTRMEMB(p_grp, setIdleStatus(this, (temp_long > 0)));
				clearEvent(event);
			}
		}
	};
	int    ok = -1;
	int    valid_data = 0;
	uint   dlg_id = 0;
	TSessFiltDialog * dlg = 0;
	TSessionFilt * p_filt = 0;
	THROW(Filt.IsA(pBaseFilt));
	p_filt = static_cast<TSessionFilt *>(pBaseFilt);
	dlg_id = (p_filt->Flags & TSessionFilt::fManufPlan) ? DLG_TSESSPLANFILT : DLG_TSESSFILT;
	THROW(CheckDialogPtr(&(dlg = new TSessFiltDialog(dlg_id))));
	//
	dlg->setDTS(p_filt);
	while(!valid_data && ExecView(dlg) == cmOK) {
		dlg->getDTS(p_filt);
		ok = valid_data = 1;
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

void PPViewTSession::MakeTempRec(const TSessionTbl::Rec * pSrcRec, TempOrderTbl::Rec * pDestRec)
{
	memzero(pDestRec, sizeof(*pDestRec));
	pDestRec->ID = pSrcRec->ID;
	SString temp_buf, st_buf, fin_buf;
	st_buf.Cat(*reinterpret_cast<const LDATETIME *>(&pSrcRec->StDt), DATF_YMD|DATF_CENTURY, TIMF_HMS);
	fin_buf.Cat(*reinterpret_cast<const LDATETIME *>(&pSrcRec->FinDt), DATF_YMD|DATF_CENTURY, TIMF_HMS);
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

bool PPViewTSession::IsTempTblNeeded() const
{
	return (Filt.Order || (Filt.StPeriod.low && Filt.StTime) || (Filt.FnPeriod.upp && Filt.FnTime) || Filt.ArID || PrcList.GetCount() > 1);
}

int PPViewTSession::MakeDraftIdList(PPIDArray & rList)
{
	int    ok = 1;
	rList.Z();
	{
		PPIDArray prc_id_list;
		const  PPID single_tec_id = TechList.GetSingle();
		if(PrcList.GetCount())
			prc_id_list = PrcList.Get();
		else
			prc_id_list.add(0L);
		TSessionTbl * p_t = TSesObj.P_Tbl;
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
			else if(single_tec_id) {
				idx = 5;
				k.k5.TechID = single_tec_id;
				k.k5.StDt = Filt.StPeriod.low;
			}
			else {
				idx = 2;
				k.k2.StDt = Filt.StPeriod.low;
			}
			dbq = ppcheckfiltid(dbq, p_t->PrcID, prc_id);
			dbq = ppcheckfiltid(dbq, p_t->TechID, single_tec_id);
			dbq = &(*dbq && daterange(p_t->StDt, &Filt.StPeriod));
			dbq = &(*dbq && daterange(p_t->FinDt, &Filt.FnPeriod));
			dbq = ppcheckfiltid(dbq, p_t->ArID, Filt.ArID);
			dbq = ppcheckfiltid(dbq, p_t->Ar2ID, Filt.Ar2ID);

			BExtQuery q(p_t, idx);
			q.selectAll().where(*dbq);
			for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
				//if(TSesObj.Search(p_t->data.ID, &rec) > 0 && TSesObj.CheckForFilt(&Filt, p_t->data.ID, &rec)) {
				p_t->CopyBufTo(&rec);
				if(TSesObj.CheckForFilt(&Filt, rec.ID, &rec, PPObjTSession::cfffDraft)) {
					rList.add(rec.ID);
				}
			}
		}
	}
	rList.sortAndUndup();
	ok = rList.getCount() ? 1 : -1;
	//CATCHZOK
	return ok;
}

int PPViewTSession::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	{
		PPObjProcessor prc_obj;
		PPIDArray prc_list, local_prc_list;
		IdList.Z(); // @v11.7.9
		ZDELETE(P_TempTbl);
		BExtQuery::ZDelete(&P_IterQuery);
		State = 0;
		PrcList.Set(0);
		TechList.Set(0); // @v11.7.6
		Filt.StPeriod.Actualize(ZERODATE);
		Filt.FnPeriod.Actualize(ZERODATE);
		if(Filt.Flags & TSessionFilt::fCurrent && Filt.PrcID) {
			const LDATETIME now_dtm = getcurdatetime_();
			PPID   sess_id = 0;
			if(TSesObj.IsProcessorBusy(Filt.PrcID, 0, TSESK_SUPERSESS, now_dtm, 1, &sess_id) > 0) {
				Filt.SuperSessID = sess_id;
			}
			else {
				Filt.StPeriod.Set(plusdate(now_dtm.d, -1), plusdate(now_dtm.d, 1));
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
					const  PPID psn_id = psn_list.get(pi);
					if(gua_obj.SearchByAssociatedPersonID(psn_id, gua_list) > 0) {
						for(uint i = 0; i < gua_list.getCount(); i++) {
							const  PPID gua_id = gua_list.get(i);
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
			if(!prc_list.getCount())
				State |= stEmpty;
			else
				PrcList.Set(&prc_list);
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
		if(Filt.TechID) {
			PPIDArray tec_list;
			TSesObj.TecObj.GetTerminalChildList(Filt.TechID, tec_list);
			if(tec_list.getCount())
				TechList.Set(&tec_list);
		}
		{
			bool temp_tbl_is_done = false;
			TSessionTbl::Rec rec;
			// @v11.7.9 {
			if(Filt.GoodsGroupID) {
				PPIDArray goods_list;
				PPIDArray draft_tsess_list;
				PPIDArray tsess_by_goods_list;
				GoodsIterator::GetListByGroup(Filt.GoodsGroupID, &goods_list);
				THROW(P_TempTbl = CreateTempOrderFile());
				if(goods_list.getCount()) {
					goods_list.sortAndUndup();
					MakeDraftIdList(draft_tsess_list);
					TSesObj.GetListByGoodsInLines(goods_list, &draft_tsess_list, tsess_by_goods_list);
					draft_tsess_list = tsess_by_goods_list;
					if(draft_tsess_list.getCount()) {
						uint i = draft_tsess_list.getCount();
						if(i) {
							BExtInsert bei(P_TempTbl);
							PPTransaction tra(ppDbDependTransaction, 1);
							THROW(tra);
							do {
								const  PPID tsess_id = draft_tsess_list.get(--i);
								if(TSesObj.Search(tsess_id, &rec) > 0) {
									if(!TSesObj.CheckForFilt(&Filt, tsess_id, &rec)) {
										draft_tsess_list.atFree(i);
									}
									else {
										TempOrderTbl::Rec temp_rec;
										MakeTempRec(&rec, &temp_rec);
										THROW_DB(bei.insert(&temp_rec));
									}
								}
							} while(i);
							THROW_DB(bei.flash());
							THROW(tra.Commit());
						}
						IdList.Set(&draft_tsess_list);
						if(IdList.GetCount() == 0)
							State |= stEmpty;
					}
					else
						State |= stEmpty;
				}
				else {
					State |= stEmpty;
				}
				temp_tbl_is_done = true;
			}
			// } @v11.7.9 
			if(!temp_tbl_is_done && IsTempTblNeeded()) {
				THROW(P_TempTbl = CreateTempOrderFile());
				if(!(State & stEmpty)) {
					PPIDArray prc_id_list;
					const  PPID single_tec_id = TechList.GetSingle();
					if(PrcList.GetCount())
						prc_id_list = PrcList.Get();
					else
						prc_id_list.add(0L);
					BExtInsert bei(P_TempTbl);
					TSessionTbl * p_t = TSesObj.P_Tbl;
					PPTransaction tra(ppDbDependTransaction, 1);
					THROW(tra);
					for(uint i = 0; i < prc_id_list.getCount(); i++) {
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
						else if(single_tec_id) {
							idx = 5;
							k.k5.TechID = single_tec_id;
							k.k5.StDt = Filt.StPeriod.low;
						}
						else {
							idx = 2;
							k.k2.StDt = Filt.StPeriod.low;
						}
						dbq = ppcheckfiltid(dbq, p_t->PrcID, prc_id);
						dbq = ppcheckfiltid(dbq, p_t->TechID, single_tec_id);
						dbq = &(*dbq && daterange(p_t->StDt, &Filt.StPeriod));
						dbq = &(*dbq && daterange(p_t->FinDt, &Filt.FnPeriod));
						dbq = ppcheckfiltid(dbq, p_t->ArID, Filt.ArID);
						dbq = ppcheckfiltid(dbq, p_t->Ar2ID, Filt.Ar2ID);

						BExtQuery q(p_t, idx);
						q.selectAll().where(*dbq);
						for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
							//if(TSesObj.Search(p_t->data.ID, &rec) > 0 && TSesObj.CheckForFilt(&Filt, p_t->data.ID, &rec)) {
							p_t->CopyBufTo(&rec);
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
	}
	CATCHZOK
	return ok;
}

int PPViewTSession::InitIteration(int order)
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
			P_IterQuery->initIteration(false, &k1, spFirst);
		}
		else {
			const  PPID single_tech_id = TechList.GetSingle();
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
				else if(single_tech_id) {
					idx = 5;
					k.k5.TechID = single_tech_id;
					k.k5.StDt = Filt.StPeriod.low;
				}
				else {
					idx = 2;
					k.k2.StDt = Filt.StPeriod.low;
				}
				dbq = ppcheckfiltid(dbq, p_t->PrcID, PrcList.GetSingle());
				dbq = ppcheckfiltid(dbq, p_t->TechID, single_tech_id);
				dbq = &(*dbq && daterange(p_t->StDt, &Filt.StPeriod));
				dbq = &(*dbq && daterange(p_t->FinDt, &Filt.FnPeriod));
				dbq = ppcheckfiltid(dbq, p_t->ArID, Filt.ArID);
				dbq = ppcheckfiltid(dbq, p_t->Ar2ID, Filt.Ar2ID);
			}
			THROW_MEM(P_IterQuery = new BExtQuery(p_t, idx, 8));
			P_IterQuery->selectAll().where(*dbq);
			k_ = k;
			Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
			P_IterQuery->initIteration(false, &k, spGe);
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
		else if(Ib.Order & ordfWithBill && Ib.WrOffBillList.testPointer()) {
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
					if(TSesObj.Search(P_TempTbl->data.ID, static_cast<TSessionTbl::Rec *>(&Ib.CurItem)) > 0) {
						do_finish = 1;
					}
				}
				else {
					TSesObj.P_Tbl->CopyBufTo(static_cast<TSessionTbl::Rec *>(&Ib.CurItem));
					if((Filt.SuperSessID || Filt.CheckStatus(Ib.CurItem.Status)) && Filt.CheckIdle(Ib.CurItem.Flags) && Filt.CheckWrOff(Ib.CurItem.Flags)) {
						if(Filt.Flags & TSessionFilt::fManufPlan && !(Ib.CurItem.Flags & TSESF_PLAN))
							continue;
						else if(!(Filt.Flags & TSessionFilt::fManufPlan) && Ib.CurItem.Flags & TSESF_PLAN)
							continue;
						else if(Filt.StPeriod.low && Filt.StTime && Ib.CurItem.StDt == Filt.StPeriod.low && Ib.CurItem.StTm < Filt.StTime)
							continue;
						else if(Filt.FnPeriod.upp && Filt.FnTime && Ib.CurItem.FinDt == Filt.FnPeriod.upp && Ib.CurItem.FinTm > Filt.FnTime)
							continue;
						else {
							do_finish = 1;
						}
					}
				}
				if(do_finish) {
					// @v11.0.4 {
					PPProcessorPacket::ExtBlock ext;
					TSesObj.GetExtention(Ib.CurItem.ID, &ext);
					ext.GetExtStrData(PRCEXSTR_MEMO, Ib.CurItem.SMemo);
					// } @v11.0.4 
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

int PPViewTSession::GetUhttStoreExtension(const TSessionTbl::Rec & rItem, PPViewTSession::UhttStoreExt & rExt)
{
    int    ok = 1;
    rExt.Z();
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
		ext_sd_text.Z();
		ext_sd_id = 0;
		ext_sd_parent_id = 0;
		if(P_UhttsPack->Sd.GetEntry(sd_pos, sd_entry)) {
			switch(sd_entry.Attr) {
				case PPUhttStoreSelDescr::attrName:
					ext_sd_id = rItem.ID;
					ext_sd_text.Z(); // @todo Description
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
                    ext_sd_id = static_cast<long>(rItem.StDt.v);
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
		if(ext_sd_parent_id) {
			rExt.SfList.AddFast(ext_sd_id, ext_sd_parent_id, ext_sd_text);
		}
		else
			rExt.SfList.AddFast(ext_sd_id, ext_sd_text);
	}
    return ok;
}

static IMPL_DBE_PROC(dbqf_tsess_memo_ip)
{
	char   buf[256];
	if(option == CALC_SIZE) {
		result->init(sizeof(buf));
	}
	else {
		buf[0] = 0;
		const  PPID  id = params[0].lval;
		PPObjTSession * p_tses_obj = static_cast<PPObjTSession *>(const_cast<void *>(params[1].ptrval));
		if(p_tses_obj) {
			PPProcessorPacket::ExtBlock ext;
			if(p_tses_obj->GetExtention(id, &ext) > 0) {
				SString & r_temp_buf = SLS.AcquireRvlStr();
				ext.GetExtStrData(PRCEXSTR_MEMO, r_temp_buf);
				r_temp_buf.CopyTo(buf, sizeof(buf));
			}
		}
		result->init(buf);
	}
}

int PPViewTSession::DynFuncMemo = DbqFuncTab::RegisterDynR(BTS_STRING, dbqf_tsess_memo_ip, 2, BTS_INT, BTS_PTR);

DBQuery * PPViewTSession::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
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
	DBQ * dbq = 0;
	DBE  * dbe_status = 0, * dbe_diff_qtty = 0;
	DBE    dbe_tech, dbe_prc, dbe_ar, dbe_goods;
	DBE    dbe_memo;
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
	{
		dbe_memo.init();
		dbe_memo.push(p_tsst->ID);
		dbe_memo.push(dbconst(&TSesObj));
		dbe_memo.push(static_cast<DBFunc>(PPViewTSession::DynFuncMemo));
	}
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
	// @v11.0.4 q->addField(p_tsst->Memo);        //  #16
	q->addField(dbe_memo);            //  #16 // @v11.0.4
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
			dbq = ppcheckfiltidlist(dbq, p_tsst->TechID, TechList.GetP());
			dbq = &(*dbq && daterange(p_tsst->StDt, &Filt.StPeriod));
			dbq = &(*dbq && daterange(p_tsst->FinDt, &Filt.FnPeriod));
			dbq = ppcheckfiltid(dbq, p_tsst->ArID, Filt.ArID);
			dbq = ppcheckfiltid(dbq, p_tsst->Ar2ID, Filt.Ar2ID);
			dbq = ppcheckflag(dbq, p_tsst->Flags, TSESF_IDLE, Filt.Ft_Idle);
			dbq = ppcheckflag(dbq, p_tsst->Flags, TSESF_WRITEDOFF, Filt.Ft_WritedOff); // @v11.0.6
			if(Filt.GetStatusList(status_list) > 0)
				dbq = & (*dbq && ppidlist(p_tsst->Status, &status_list));
			dbq = ppcheckflag(dbq, p_tsst->Flags, TSESF_SUPERSESS, (Filt.Flags & TSessionFilt::fSuperSessOnly) ? 1 : 0);
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
		else if(TechList.GetSingle())
			q->orderBy(p_tsst->TechID, p_tsst->StDt, 0L);
		else
			q->orderBy(p_tsst->StDt, 0L);
	}
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		if(Filt.SuperSessID) {
			CatObjectName(PPOBJ_TSESSION, Filt.SuperSessID, *pSubTitle);
		}
		else {
			if(Filt.PrcID) {
				pSubTitle->CatDivIfNotEmpty(';', 0);
				CatObjectName(PPOBJ_PROCESSOR, Filt.PrcID, *pSubTitle);
			}
			if(Filt.TechID) {
				pSubTitle->CatDivIfNotEmpty(';', 0);
				CatObjectName(PPOBJ_TECH, Filt.TechID, *pSubTitle);
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

void PPViewTSession::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		const PPTSessConfig & r_cfg = TSesObj.GetConfig();
		if(r_cfg.ViewRefreshPeriod)
			pBrw->SetRefreshPeriod(r_cfg.ViewRefreshPeriod);
	}
}

int PPViewTSession::WriteOff(PPID sessID)
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

int PPViewTSession::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
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

int PPViewTSession::CalcTotal(TSessionTotal * pTotal)
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

void PPViewTSession::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_TSESSTOTAL);
	TSessionTotal total;
	CalcTotal(&total);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlLong(CTL_TSESSTOTAL_COUNT, total.Count);
		{
			SString temp_buf;
			if(total.Duration > 0) {
				long   h = total.Duration / 3600;
				long   m = (total.Duration % 3600) / 60;
				long   s = (total.Duration % 60);
				temp_buf.Z().CatLongZ(h, 2).Colon().CatLongZ(m, 2).Colon().CatLongZ(s, 2);
			}
			dlg->setCtrlString(CTL_TSESSTOTAL_DURATION, temp_buf);
		}
		ExecViewAndDestroy(dlg);
	}
}

int PPViewTSession::Print(const void * pHdr)
{
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	if(id)
		PPAlddPrint(REPORT_TSESSION, PPFilt(id), 0);
	return -1;
}

int PPViewTSession::PrintList(const void * pHdr)
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

int PPViewTSession::Recover()
{
	int    ok = -1;
	PPIDArray id_list;
	TSessionViewItem item;
	PPWaitStart();
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
	PPWaitStop();
	CATCHZOKPPERR
	return ok;
}

int PPViewTSession::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		TSessionViewItem item;
		const  PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWaitStart();
		for(InitIteration(0); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			objid_ary.Add(PPOBJ_TSESSION, item.ID);
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		PPWaitStop();
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewTSession::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		TSessionTbl::Rec rec;
		switch(ppvCmd) {
			case PPVCMD_INPUTCHAR:
				ok = -1;
				{
					PPInputStringDialogParam isd_param;
					SString code;
					if(pHdr)
						code.CatChar(*static_cast<const char *>(pHdr));
					PPLoadText(PPTXT_ADDSESSBYPRC, isd_param.Title);
					isd_param.Flags |= PPInputStringDialogParam::fDisableSelection;
					if(InputStringDialog(isd_param, code) > 0) {
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
					if(!filt.IsEq(&Filt, 1))
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
					RVALUEPTR(flt, P_LastAnlzFilt);
					PPViewTSessAnlz v_tsa;
					if(Filt.Flags & TSessionFilt::fManufPlan)
						flt.PlanSessID = id;
					if(v_tsa.EditBaseFilt(&flt) > 0) {
						flt.SessIdList.Z();
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
						ok = TSesObj.CompleteSession(tses_id, 1);
						if(!ok)
							PPError();
					}
				}
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
			case PPVCMD_WROFFSESS: ok = WriteOff(id); break;
			case PPVCMD_TRANSMIT: ok = Transmit(0); break;
			case PPVCMD_SENDSMS: ok = SendAutoSms(); break; // @vmiller
			case PPVCMD_EXPORTUHTT: ok = ExportUhtt(); break;
		}
	}
	return ok;
}

int PPViewTSession::ExportUhtt()
{
	int    ok = -1;
	SString msg_buf, fmt_buf, temp_buf, loc_text_buf;
	SString tsess_text;
	PPLogger logger;
	PPUhttClient uhtt_cli;
	PPWaitStart();
	THROW(uhtt_cli.Auth());
	{
		SString img_path;
		ObjLinkFiles lf(PPOBJ_TSESSION);
		TSessionViewItem item;
		for(InitIteration(0); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
			const  PPID _id = item.ID;
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
								uuid.Z();
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
							// @v11.0.4 uhtt_pack.SetMemo(pack.Rec.Memo); 
							// @v11.0.4 {
							pack.Ext.GetExtStrData(PRCEXSTR_MEMO, temp_buf.Z());
							uhtt_pack.SetMemo(temp_buf);
							// } @v11.0.4 
							pack.Ext.GetExtStrData(PRCEXSTR_DETAILDESCR, temp_buf.Z());
							uhtt_pack.SetDetail(temp_buf);
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
	PPWaitStop();
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

// @vmiller
int PPViewTSession::SendAutoSms()
{
	PPAlbatrossConfig albtr_cfg;
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
int PPViewTSession::GetSmsLists(StrAssocArray & rPsnList, StrAssocArray & rPhoneList, StrAssocArray & rTSessIdArr)
{
	size_t i = 0;
 	SString buf, phone;
	TSessionViewItem item;

	for(InitIteration(ordByDefault); NextIteration(&item) > 0;) {
		PPELinkArray elink_list;
		PPID pers_id = ObjectToPerson(item.ArID);
		if(PersonCore::GetELinks(pers_id, elink_list) > 0) {
			buf.Z().Cat(item.ID);
			rTSessIdArr.Add(i, buf);
			elink_list.GetItem(PPELK_MOBILE, phone.Z());
			if(phone.IsEmpty())
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


int ViewTSession(const TSessionFilt * pFilt) { return PPView::Execute(PPVIEW_TSESSION, pFilt, 1, 0); }
int ViewManufPlan(const TSessionFilt * pFilt) { return PPView::Execute(PPVIEW_TSESSION, pFilt, 1, (void *)TSESK_PLAN); }
//
// @ModuleDef(PPViewTSessLine)
//
TSessLineFilt::TSessLineFilt(PPID sessID, PPID goodsID, int showRest)
{
	THISZERO();
	TSesList.Add(sessID);
	GoodsID = goodsID;
	SETFLAG(Flags, fOutRest, showRest);
}

int TSessLineFilt::Init()
{
	TSesList.Set(0);
	return 1;
}

PPViewTSessLine::PPViewTSessLine() : PPView(0, 0, 0, 0, 0), P_TempTbl(0), NewGoodsGrpID(0)
{
}

PPViewTSessLine::~PPViewTSessLine()
{
	delete P_TempTbl;
}

const TSessLineFilt * PPViewTSessLine::GetFilt() const { return &Filt; }

bool PPViewTSessLine::IsTempTblNeeded()
{
	if(Filt.TSesList.GetCount() > 1)
		return true;
	else {
		Goods2Tbl::Rec goods_rec;
		if(Filt.GoodsID && GObj.Fetch(Filt.GoodsID, &goods_rec) > 0)
			if(goods_rec.Kind == PPGDSK_GROUP || (goods_rec.Kind == PPGDSK_GOODS && goods_rec.Flags & GF_GENERIC))
				return true;
	}
	return false;
}

int PPViewTSessLine::Init(const TSessLineFilt * pFilt)
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

int PPViewTSessLine::CreateIterQuery()
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
			P_IterQuery->initIteration(false, &k, spFirst);
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
			P_IterQuery->initIteration(false, &k, spGe);
		}
	}
	return ok;
}

int PPViewTSessLine::InitIteration()
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
				P_TempTbl->CopyBufTo(&rec);
			else {
				TSesObj.P_Tbl->Lines.CopyBufTo(&rec);
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

void PPViewTSessLine::ViewTotal()
{
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
}

DBQuery * PPViewTSessLine::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	static DbqStringSubst sign_subst(3); // @global @threadsafe

	uint   brw_id = 0;
	TSessLineTbl * p_tslt = 0;
	TSessionTbl  * p_tst = 0;
	DBQuery * q  = 0;
	DBQ * dbq = 0;
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
		dbe_phqtty.push(static_cast<DBFunc>(PPDbqFuncPool::IdTSesLnPhQtty));
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
			dbe_datetime.push(static_cast<DBFunc>(PPDbqFuncPool::IdDateTime));
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
		GetObjectName(PPOBJ_TSESSION, Filt.TSesList.GetSingle(), *pSubTitle);
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

void PPViewTSessLine::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw && Filt.GoodsID && GoodsIdList.getSingle() == 0) {
		// pBrw->InsColumnWord(1, PPWORD_GOODS, 3, 0, 0, 0);
		pBrw->InsColumn(1, "@ware", 3, 0, 0, 0);
	}
}

int PPViewTSessLine::TranslateBrwHdr(const void * pRow, BrwHdr * pHdr)
{
	if(pRow) {
		ASSIGN_PTR(pHdr, *static_cast<const BrwHdr *>(pRow));
		return 1;
	}
	else
		return 0;
}

int PPViewTSessLine::Print(const void *)
{
	return Helper_Print(Filt.GoodsID ? REPORT_TSESSLINEGOODS : REPORT_TSESSLINE, 0);
}

int PPViewTSessLine::AddItemExt(PPID tsesID, PPViewBrowser * pBrw)
{
	int    ok = -1;
	ExtGoodsSelDialog * dlg = 0;
	if(tsesID) {
		if(!TSesObj.CheckRights(TSESRT_ADDLINE))
			ok = PPErrorZ();
		else {
			const long egsd_flags = ExtGoodsSelDialog::GetDefaultFlags();
			if(CheckDialogPtrErr(&(dlg = new ExtGoodsSelDialog(0, NewGoodsGrpID, egsd_flags)))) {
				TIDlgInitData tidi;
				PPIDArray goods_list;
				TGSArray tgs_list;
				TSessionTbl::Rec tses_rec;
				ProcessorTbl::Rec prc_rec;
				if(TSesObj.Search(tsesID, &tses_rec) > 0) {
					if(TSesObj.GetPrc(tses_rec.PrcID, &prc_rec, 1, 1) > 0)
						dlg->setLocation(prc_rec.LocID);
				}
				else
					MEMSZERO(tses_rec);
				const bool free_goods_sel = LOGIC(TSesObj.GetConfig().Flags & PPTSessConfig::fFreeGoodsSelection);
				if(!free_goods_sel && TSesObj.GetGoodsStrucList(tsesID, 1, 0, &tgs_list) > 0 && tgs_list.GetGoodsList(&goods_list) > 0) {
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
	}
	return ok;
}

int PPViewTSessLine::AddItemByCode(const char * pInitStr)
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
			else if(code.NotEmptyS()) { // @v10.8.10 @fix if(code.NotEmptyS())
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

int PPViewTSessLine::AddCompletion(PPID sessID)
{
	int    ok = -1;
	long   h_lnenum = -1;
	PPID   tec_goods_id = 0;
	PPID   tec_struc_id = 0;
	TSessionTbl::Rec tses_rec;
	if(TSesObj.Search(sessID, &tses_rec) > 0) {
		int    add_tec_goods_before_complete = 0;
		TSessLineTbl::Rec line_rec;
		ProcessorTbl::Rec prc_rec;
		TechTbl::Rec tec_rec;
		THROW_PP(!(tses_rec.Flags & TSESF_WRITEDOFF), PPERR_TSESSWRITEDOFF);
		THROW_PP(!(tses_rec.Flags & TSESF_SUPERSESS), PPERR_INVOPONTSUPERSESS);
		THROW(TSesObj.PrcObj.GetRecWithInheritance(tses_rec.PrcID, &prc_rec) > 0);
		if(TSesObj.TecObj.Fetch(tses_rec.TechID, &tec_rec) > 0) {
			tec_goods_id = tec_rec.GoodsID;
			tec_struc_id = tec_rec.GStrucID;
			uint   ln_count = 0;
			for(TSesObj.P_Tbl->InitLineEnum(sessID, &h_lnenum); TSesObj.P_Tbl->NextLineEnum(h_lnenum, &line_rec) > 0;) {
				ln_count++;
				break;
			}
			if(!ln_count && tec_goods_id && tec_struc_id) {
				add_tec_goods_before_complete = 1;
			}
		}
		if(add_tec_goods_before_complete) {
			long   oprno = 0;
			if(TSesObj.EditLine(sessID, &oprno, tec_goods_id, 0, 0.0) > 0) {
				THROW(TSesObj.CompleteSession(sessID, 1));
				ok = 1;
			}
		}
		else {
			THROW(TSesObj.CompleteSession(sessID, 1));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	TSesObj.P_Tbl->DestroyIter(h_lnenum);
	return ok;
}

int PPViewTSessLine::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	BrwHdr hdr;
	long   oprno = 0;
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_INPUTCHAR:
				ok = (AddItemByCode(static_cast<const char *>(pHdr)) > 0) ? 1 : -1;
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
			case PPVCMD_EDITGOODS: // @v11.0.7
				if(TranslateBrwHdr(pHdr, &hdr) && hdr.TSesID && hdr.OprNo) {
					TSessLineTbl::Rec line_rec;
					if(TSesObj.P_Tbl->SearchLine(hdr.TSesID, hdr.OprNo, &line_rec) > 0 && line_rec.GoodsID) {
						if(GObj.Edit(&line_rec.GoodsID, 0) == cmOK)
							ok = 1;
					}
				}
				break;
			case PPVCMD_COMPLETE:
				{
					const  PPID tses_id = (TranslateBrwHdr(pHdr, &hdr) && hdr.TSesID) ? hdr.TSesID : Filt.TSesList.GetSingle();
					if(tses_id) {
						ok = AddCompletion(tses_id); // @v10.8.10
						/* @v10.8.10 ok = TSesObj.CompleteSession(tses_id, 1);
						if(!ok)
							PPError();*/
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

int PPViewTSession::GetSelectorListInfo(StrAssocArray & rList) const
{
	if(P_UhttsPack && P_UhttsPack->Sd.GetCount()) {
		rList = ExtSfTitleList;
		return 1;
	}
	else {
		rList.Z();
		return 0;
	}
}

int PPViewTSession::GetSelectorListItem(long handler, PPUhttStoreSelDescr::Entry & rEntry) const
{
	int    ok = 0;
	if(P_UhttsPack && handler > 0 && handler <= (long)P_UhttsPack->Sd.GetCount())
		ok = P_UhttsPack->Sd.GetEntry(handler-1, rEntry);
	return ok;
}

int ViewTSessLine(const TSessLineFilt * pFilt)
{
	int    ok = 1;
	int    view_in_use = 0;
	const  bool modeless = GetModelessStatus();
	TSessLineFilt flt;
	PPViewTSessLine * p_v = new PPViewTSessLine;
	PPViewBrowser * p_prev_win = modeless ? static_cast<PPViewBrowser *>(PPFindLastBrowser()) : 0;
	if(pFilt)
		flt = *pFilt;
	else if(p_prev_win)
		flt = *static_cast<PPViewTSessLine *>(p_prev_win->P_View)->GetFilt();
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
	delete static_cast<PPObjTSession *>(Extra[0].Ptr);
}

int PPALDD_TSession::InitData(PPFilt & rFilt, long rsrv)
{
	if(rFilt.ID == H.ID)
		return DlRtm::InitData(rFilt, rsrv);
	MEMSZERO(H);
	H.ID = rFilt.ID;
	TSessionTbl::Rec super_rec;
	TSessionPacket pack; // @v11.0.4 (TSessionTbl::Rec rec)-->(TSessionPacket pack)
	SString temp_buf;
	PPObjTSession * p_ses_obj = static_cast<PPObjTSession *>(Extra[0].Ptr);
	if(p_ses_obj->GetPacket(rFilt.ID, &pack, 0) > 0) {
		H.ID = pack.Rec.ID;
		H.ParentID = pack.Rec.ParentID;
		H.Number   = pack.Rec.Num;
		H.TechID   = pack.Rec.TechID;
		H.PrcID    = pack.Rec.PrcID;
		H.CCheckID = pack.Rec.CCheckID_;
		H.SCardID  = pack.Rec.SCardID;
		H.LinkBillID = pack.Rec.LinkBillID;
		H.StDt     = pack.Rec.StDt;
		H.StTm     = pack.Rec.StTm;
		timefmt(pack.Rec.StTm, TIMF_HMS, H.StTmTxt);
		H.FinDt    = pack.Rec.FinDt;
		H.FinTm    = pack.Rec.FinTm;
		timefmt(pack.Rec.FinTm, TIMF_HMS, H.FinTmTxt);
		H.Incomplete = pack.Rec.Incomplete;
		H.Status   = pack.Rec.Status;
		H.Idle     = BIN(pack.Rec.Flags & TSESF_IDLE);
		p_ses_obj->MakeName(&pack.Rec, temp_buf);
		temp_buf.CopyTo(H.Name, sizeof(H.Name));
		if(pack.Rec.ParentID && p_ses_obj->Search(pack.Rec.ParentID, &super_rec) > 0) {
			p_ses_obj->MakeName(&super_rec, temp_buf); // @v11.0.4 @fix rec-->super_rec
			temp_buf.CopyTo(H.SuperName, sizeof(H.SuperName));
		}
		p_ses_obj->GetStatusText(pack.Rec.Status, temp_buf);
		temp_buf.CopyTo(H.StatusText, sizeof(H.StatusText));
		H.Flags    = pack.Rec.Flags;
		H.ArID     = pack.Rec.ArID;
		H.Ar2ID    = pack.Rec.Ar2ID;
		H.PlannedTiming = pack.Rec.PlannedTiming;
		H.PlannedQtty   = pack.Rec.PlannedQtty;
		long   act_timing = diffdatetimesec(pack.Rec.FinDt, pack.Rec.FinTm, pack.Rec.StDt, pack.Rec.StTm);
		SETMAX(act_timing, 0);
		H.ActTiming = act_timing;
		H.ActQtty   = pack.Rec.ActQtty;
		{
			long h, m, s;
			if(pack.Rec.PlannedTiming > 0) {
				h = pack.Rec.PlannedTiming / 3600;
				m = (pack.Rec.PlannedTiming % 3600) / 60;
				s = (pack.Rec.PlannedTiming % 60);
				temp_buf.Z().CatLongZ(h, 2).Colon().CatLongZ(m, 2).Colon().CatLongZ(s, 2);
			}
			else
				temp_buf.Z();
			temp_buf.CopyTo(H.PlnTimingText, sizeof(H.PlnTimingText));
			if(act_timing > 0) {
				h = act_timing / 3600;
				m = (act_timing % 3600) / 60;
				s = (act_timing % 60);
				temp_buf.Z().CatLongZ(h, 2).Colon().CatLongZ(m, 2).Colon().CatLongZ(s, 2);
			}
			else
				temp_buf.Z();
			temp_buf.CopyTo(H.ActTimingText, sizeof(H.ActTimingText));
		}
		H.Amount = pack.Rec.Amount;
		{
			pack.Ext.GetExtStrData(PRCEXSTR_MEMO, temp_buf.Z());
			STRNSCPY(H.Memo, temp_buf);
		}
		return DlRtm::InitData(rFilt, rsrv);
	}
	return -1;
}

void PPALDD_TSession::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))

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

PPALDD_DESTRUCTOR(TSessionView) { Destroy(); }

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

void PPALDD_TSessionView::Destroy() { DESTROY_PPVIEW_ALDD(TSession); }
//
// Implementation of PPALDD_TSessionCipView
//
PPALDD_CONSTRUCTOR(TSessionCipView)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(TSessionCipView) { Destroy(); }

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

void PPALDD_TSessionCipView::Destroy() { DESTROY_PPVIEW_ALDD(TSession); }
//
// Implementation of PPALDD_TSessionBillView
//
PPALDD_CONSTRUCTOR(TSessionBillView)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(TSessionBillView) { Destroy(); }

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

void PPALDD_TSessionBillView::Destroy() { DESTROY_PPVIEW_ALDD(TSession); }
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

PPALDD_DESTRUCTOR(TSessLineView) { Destroy(); }

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

void PPALDD_TSessLineView::Destroy() { DESTROY_PPVIEW_ALDD(TSessLine); }
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
	return CreateInnerInstance("PPFiltTSession", 0, reinterpret_cast<void **>(&p_filt)) ? p_filt : static_cast<IUnknown *>(RaiseAppErrorPtr());
}

int32 DL6ICLS_PPViewTSession::Init(IUnknown* pFilt)
{
	IPpyFilt_TSession * p_ifc_filt = 0;
	S_GUID uuid;
	THROW_INVARG(pFilt);
	THROW(GetInnerUUID("IPpyFilt_TSession", uuid));
	THROW(SUCCEEDED(pFilt->QueryInterface(uuid, (void **)&p_ifc_filt)));
	THROW(static_cast<PPViewTSession *>(ExtraPtr)->Init_(static_cast<const TSessionFilt *>(GetExtraPtrByInterface(p_ifc_filt))));
	CATCH
		AppError = 1;
	ENDCATCH
	CALLTYPEPTRMEMB(IUnknown, p_ifc_filt, Release());
	return !AppError;
}

int32 DL6ICLS_PPViewTSession::InitIteration(int32 order)
{
	return static_cast<PPViewTSession *>(ExtraPtr)->InitIteration(order);
}

int32 DL6ICLS_PPViewTSession::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SString temp_buf;
	SPpyO_TSession * p_item = static_cast<SPpyO_TSession *>(item);
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
		p_item->StDt = inner_item.StDt.GetOleDate();
		p_item->StTm = (OleDate)inner_item.StTm;
		p_item->FinDt = inner_item.FinDt.GetOleDate();
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
		// @v11.0.4 (temp_buf = inner_item.Memo).CopyToOleStr(&p_item->Memo);
		inner_item.SMemo.CopyToOleStr(&p_item->Memo); // @v11.0.4 
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
		SPpyVT_TSession * p_total = static_cast<SPpyVT_TSession *>(total);
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
int PPObjTSession::ConvertPacket(const UhttTSessionPacket * pSrc, long flags, TSessionPacket & rDest)
{
	int    ok = 1;
	rDest.Z();
	if(pSrc) {
		uint   i;
		PPObjTag tag_obj;
		S_GUID   uuid;
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
		// @v11.0.4 STRNSCPY(rDest.Rec.Memo, pSrc->Memo);
		rDest.Ext.PutExtStrData(PRCEXSTR_MEMO, pSrc->Memo); // @v11.0.4
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

int PPObjTSession::ImportUHTT()
{
	int    ok = -1;
	SString msg_buf, fmt_buf, temp_buf, loc_text_buf;
	SString tsess_text;
	PPLogger logger;
	PPUhttClient uhtt_cli;
	PPWaitStart();
	THROW(uhtt_cli.Auth());
	{
		PPID   prc_id = 0;
		LDATETIME since = ZERODATETIME;
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
							org_pack.CiList.Z();
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
	PPWaitStop();
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}
