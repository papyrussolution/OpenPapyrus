// V_PANLZ.CPP
// Copyright (c) A.Starodub 2005, 2006, 2007, 2008, 2009, 2010, 2015, 2016, 2017, 2018, 2019, 2020, 2024
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewPriceAnlz)
//

//
// @todo
// 1. За базу принимать:
//    - цену по одному из складов
//    - контрактную цену
//    - среднюю цену по всем складам
// 2. Фильтрация:
//    - список складов
//    - исключать строки в которых нет контрактной цены
// 3. Оптимизировать расчет по поставщику (предварительно выбрать позиции, которые приходят
//    от этого поставщика, а затем перебирать эти позиции).
//
IMPLEMENT_PPFILT_FACTORY(PriceAnlz); PriceAnlzFilt::PriceAnlzFilt() : PPBaseFilt(PPFILT_PRICEANLZ, 0, 0)
{
	SetFlatChunk(offsetof(PriceAnlzFilt, ReserveStart),
		offsetof(PriceAnlzFilt, LocList)-offsetof(PriceAnlzFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(PriceAnlzFilt, LocList));
	Init(1, 0);
}

PriceAnlzFilt & FASTCALL PriceAnlzFilt::operator = (const PriceAnlzFilt & s)
{
	Copy(&s, 1);
	return *this;
}
//
//
//
PPViewPriceAnlz::PPViewPriceAnlz() : PPView(0, &Filt, 0, 0, REPORT_PRICEANLZ), P_TempTbl(0)
{
}

PPViewPriceAnlz::~PPViewPriceAnlz()
{
	ZDELETE(P_TempTbl);
}

PPBaseFilt * PPViewPriceAnlz::CreateFilt(const void * extraPtr) const
{
	PriceAnlzFilt * p_filt = new PriceAnlzFilt;
	p_filt->BaseCost = PriceAnlzFilt::bcByContract;
	return p_filt;
}

class PriceAnlzFiltDialog : public TDialog {
public:
	enum {
		ctlgroupGoodsFilt = 1,
		ctlgroupLoc       = 2
	};
	PriceAnlzFiltDialog() : TDialog(DLG_PANLZFLT)
	{
		addGroup(ctlgroupGoodsFilt, new GoodsFiltCtrlGroup(0, CTLSEL_PANLZFLT_GOODSGRP, cmGoodsFilt));
		addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_PANLZFLT_LOC, 0, 0, cmLocList, 0, 0, 0));
		SetupCalPeriod(CTLCAL_PANLZFLT_PERIOD, CTL_PANLZFLT_PERIOD);
	}
	int    setDTS(const PriceAnlzFilt *);
	int    getDTS(PriceAnlzFilt *);
private:
	DECL_HANDLE_EVENT;
	void   setupBaseLoc(long baseCost);

	PriceAnlzFilt Data;
};

IMPL_HANDLE_EVENT(PriceAnlzFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_PANLZFLT_BASE)) {
		setupBaseLoc(getCtrlLong(CTL_PANLZFLT_BASE));
		clearEvent(event);
	}
}

void PriceAnlzFiltDialog::setupBaseLoc(long baseCost)
{
	disableCtrl(CTLSEL_PANLZFLT_BASELOC, baseCost != PriceAnlzFilt::bcByLoc);
	if(baseCost != PriceAnlzFilt::bcByLoc)
		setCtrlData(CTLSEL_PANLZFLT_BASELOC, 0);
}

int PriceAnlzFiltDialog::setDTS(const PriceAnlzFilt * pData)
{
	RVALUEPTR(Data, pData);
	SetPeriodInput(this, CTL_PANLZFLT_PERIOD, &Data.Period);
	SetupArCombo(this, CTLSEL_PANLZFLT_SUPPL, Data.SupplID, 0, GetSupplAccSheet(), sacfDisableIfZeroSheet);
	GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, 0, 0, GoodsCtrlGroup::enableSelUpLevel);
	setGroupData(ctlgroupGoodsFilt, &gf_rec);
	LocationCtrlGroup::Rec loc_rec(&Data.LocList);
	setGroupData(ctlgroupLoc, &loc_rec);
	AddClusterAssoc(CTL_PANLZFLT_COSTALG,  0, PriceAnlzFilt::caByFirstLot);
	AddClusterAssoc(CTL_PANLZFLT_COSTALG,  1, PriceAnlzFilt::caByAverageLot);
	AddClusterAssocDef(CTL_PANLZFLT_COSTALG,  2, PriceAnlzFilt::caByLastLot);
	AddClusterAssoc(CTL_PANLZFLT_COSTALG,  3, PriceAnlzFilt::caByMinLot);
	AddClusterAssoc(CTL_PANLZFLT_COSTALG,  4, PriceAnlzFilt::caByMinLoc);
	SetClusterData(CTL_PANLZFLT_COSTALG, Data.CostAlg);
	AddClusterAssocDef(CTL_PANLZFLT_BASE,  0, PriceAnlzFilt::bcByLoc);
	AddClusterAssoc(CTL_PANLZFLT_BASE,  1, PriceAnlzFilt::bcByContract);
	AddClusterAssoc(CTL_PANLZFLT_BASE,  2, PriceAnlzFilt::bcByAvgLocs);
	SetClusterData(CTL_PANLZFLT_BASE, Data.BaseCost);
	AddClusterAssoc(CTL_PANLZFLT_FLAGS, 0, PriceAnlzFilt::fShowDiffAsPrc);
	AddClusterAssoc(CTL_PANLZFLT_FLAGS, 1, PriceAnlzFilt::fExclWOCntrCost);
	AddClusterAssoc(CTL_PANLZFLT_FLAGS, 2, PriceAnlzFilt::fExclWOCostOREqualBaseCost);
	AddClusterAssoc(CTL_PANLZFLT_FLAGS, 3, PriceAnlzFilt::fDivideBySuppl);
	SetupPPObjCombo(this, CTLSEL_PANLZFLT_BASELOC, PPOBJ_LOCATION, Data.BaseLoc, 0, 0);
	setupBaseLoc(Data.BaseCost);
	SetClusterData(CTL_PANLZFLT_FLAGS, Data.Flags);
	return 1;
}

int PriceAnlzFiltDialog::getDTS(PriceAnlzFilt * pData)
{
	int    ok = 1;
	uint   sel = 0;
	GoodsFiltCtrlGroup::Rec gf_rec;
	LocationCtrlGroup::Rec loc_rec;
	GetClusterData(CTL_PANLZFLT_BASE, &Data.BaseCost);
	getCtrlData(sel = CTLSEL_PANLZFLT_BASELOC, &Data.BaseLoc);
	THROW_PP(Data.BaseCost != PriceAnlzFilt::bcByLoc || Data.BaseLoc != 0, PPERR_LOCNEEDED);
	THROW(GetPeriodInput(this, sel = CTL_PANLZFLT_PERIOD, &Data.Period));
	getCtrlData(CTLSEL_PANLZFLT_SUPPL,    &Data.SupplID);
	THROW(getGroupData(ctlgroupGoodsFilt, &gf_rec));
	Data.GoodsGrpID = gf_rec.GoodsGrpID;
	THROW(getGroupData(ctlgroupLoc, &loc_rec));
	Data.LocList = loc_rec.LocList;
	GetClusterData(CTL_PANLZFLT_COSTALG, &Data.CostAlg);
	GetClusterData(CTL_PANLZFLT_FLAGS,    &Data.Flags);
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

int PPViewPriceAnlz::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	PriceAnlzFilt * p_filt = static_cast<PriceAnlzFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(PriceAnlzFiltDialog, p_filt);
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempPriceAnlz);

int PPViewPriceAnlz::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1, use_ta = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	PPWaitStart();
	BExtQuery::ZDelete(&P_IterQuery);
	{
		struct _E {
			PPID   LocID;
			PPID   SupplID;
			uint   NumLots;
			LDATE  MaxDt;
			double Cost;
			double Base;
			double Contract;
		};
		uint   i, j, k;
		GoodsFilt goods_flt;
		Goods2Tbl::Rec goods_rec;
		PPIDArray loc_list;
		PPIDArray suppl_list;
		SArray cost_ary(sizeof(_E));
		SString ar_name;
		const  PPID suppl_deal_qk_id = DS.GetTLA().SupplDealQuotKindID;
		ZDELETE(P_TempTbl);
		THROW(P_TempTbl = CreateTempFile());
		{
			PPObjLocation loc_obj;
			BExtInsert bei(P_TempTbl);
			bool is_loclist_restricted = false;
			PPTransaction tra(ppDbDependTransaction, use_ta);
			THROW(tra);
			goods_flt.GrpID   = Filt.GoodsGrpID;
			goods_flt.SupplID = Filt.SupplID;
			if(Filt.LocList.GetCount()) {
				is_loclist_restricted = true;
		 		loc_list.copy(Filt.LocList.Get());
			}
			else {
				THROW(loc_obj.GetWarehouseList(&loc_list, &is_loclist_restricted));
			}
			for(GoodsIterator giter(&goods_flt, 0); giter.Next(&goods_rec) > 0;) {
				int    skip = 0;
				THROW(PPCheckUserBreak());
				if(!(goods_rec.Flags & GF_GENERIC) && (Filt.GoodsGrpID || !GObj.IsAsset(goods_rec.ID))) {
					uint   pos = 0;
					ReceiptTbl::Rec lot_rec;
					cost_ary.clear();
					suppl_list.clear();
					ReceiptTbl & r_t = BillObj->trfr->Rcpt;
					ReceiptTbl::Key2 k2;
					BExtQuery q(&r_t, 2);
					DBQ * dbq = &(r_t.GoodsID == goods_rec.ID && daterange(r_t.Dt, &Filt.Period) && r_t.PrevLotID == 0L);
					dbq = ppcheckfiltid(dbq, r_t.SupplID, Filt.SupplID);
					if(is_loclist_restricted) // @v10.9.9
						dbq = ppcheckfiltidlist(dbq, r_t.LocID, &loc_list);
					q.select(r_t.ID, r_t.PrevLotID, r_t.SupplID, r_t.LocID, r_t.Dt, r_t.Cost, 0L).where(*dbq);
					MEMSZERO(k2);
					k2.GoodsID = goods_rec.ID;
					k2.Dt = Filt.Period.low;
					for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
						int    found = 0;
						double base_cost = 0.0;
						_E   _e;
						_E   * p_e = 0;
						r_t.copyBufTo(&lot_rec);
						MEMSZERO(_e);
						LAssoc srch(lot_rec.LocID, (Filt.Flags & PriceAnlzFilt::fDivideBySuppl) ? lot_rec.SupplID : Filt.SupplID);
						if(cost_ary.lsearch(&srch, &(pos = 0), PTR_CMPFUNC(_2long))) {
							p_e = static_cast<_E *>(cost_ary.at(pos));
							found = 1;
						}
						if(lot_rec.Cost > 0.0) {
							if(found) {
								p_e->NumLots++;
								p_e->MaxDt = MAX(p_e->MaxDt, lot_rec.Dt);
								if(Filt.CostAlg == PriceAnlzFilt::caByAverageLot)
									p_e->Cost = (p_e->Cost + lot_rec.Cost) / p_e->NumLots;
								else if(Filt.CostAlg == PriceAnlzFilt::caByMinLot)
									p_e->Cost = MIN(p_e->Cost, lot_rec.Cost);
								else if(oneof2(Filt.CostAlg, PriceAnlzFilt::caByLastLot, PriceAnlzFilt::caByMinLoc))
									p_e->Cost = lot_rec.Cost;
								if(Filt.BaseCost == PriceAnlzFilt::bcByLoc) {
									if(p_e->LocID == Filt.BaseLoc)
										p_e->Base = lot_rec.Cost;
								}
								else if(Filt.BaseCost == PriceAnlzFilt::bcByAvgLocs)
									p_e->Base = lot_rec.Cost;
							}
							else {
								_e.LocID    = lot_rec.LocID;
								_e.SupplID  = (Filt.Flags & PriceAnlzFilt::fDivideBySuppl) ? lot_rec.SupplID : Filt.SupplID;
								_e.MaxDt    = lot_rec.Dt;
								_e.NumLots  = 1;
								_e.Cost     = lot_rec.Cost;
								if(Filt.BaseCost == PriceAnlzFilt::bcByLoc) {
									if(_e.LocID == Filt.BaseLoc)
										_e.Base = lot_rec.Cost;
								}
								else if(Filt.BaseCost == PriceAnlzFilt::bcByAvgLocs)
									_e.Base = lot_rec.Cost;
								THROW_SL(cost_ary.insert(&_e));
								THROW_SL(suppl_list.addUnique(_e.SupplID));
							}
						}
					}
					_E * p_e, * p_e2;
					if(Filt.CostAlg == PriceAnlzFilt::caByMinLoc) {
						for(i = 0; i < suppl_list.getCount(); i++) {
							double extr = SMathConst::Max;
							const  PPID suppl_id = suppl_list.get(i);
							for(j = 0; cost_ary.enumItems(&j, (void **)&p_e);)
								if(p_e->SupplID == suppl_id && p_e->Cost > 0.0 && p_e->Cost < extr)
									extr = p_e->Cost;
							if(extr > 0.0 && extr < SMathConst::Max) {
								for(j = 0; cost_ary.enumItems(&j, (void **)&p_e);)
									if(p_e->SupplID == suppl_id)
										p_e->Cost = extr;
							}
							else {
								extr = -1.0; // @debug
							}
						}
					}
					if(Filt.BaseCost == PriceAnlzFilt::bcByContract) {
						for(i = 0; i < suppl_list.getCount(); i++) {
							const  PPID suppl_id = suppl_list.get(i);
							for(j = 0; cost_ary.enumItems(&j, (void **)&p_e);) {
								if(p_e->SupplID == suppl_id) {
									const QuotIdent qi(p_e->LocID, suppl_deal_qk_id, 0, suppl_id);
									PPSupplDeal sd;
									int    r;
									THROW(r = GObj.GetSupplDeal(goods_rec.ID, qi, &sd, 1));
									if(r > 0)
										p_e->Contract = p_e->Base = (sd.IsDisabled ? -1.0 : sd.Cost);
									else
										p_e->Base = 0.0;
								}
							}
						}
					}
					else {
						if(Filt.Flags & PriceAnlzFilt::fExclWOCntrCost) {
							for(i = 0; i < suppl_list.getCount(); i++) {
								const  PPID suppl_id = suppl_list.get(i);
								for(j = 0; cost_ary.enumItems(&j, (void **)&p_e);) {
									if(p_e->SupplID == suppl_id) {
										const QuotIdent qi(p_e->LocID, suppl_deal_qk_id, 0, suppl_id);
										PPSupplDeal sd;
										int    r;
										THROW(r = GObj.GetSupplDeal(goods_rec.ID, qi, &sd, 1));
										if(r > 0)
											p_e->Contract = (sd.IsDisabled ? -1.0 : sd.Cost);
									}
								}
							}
						}
						if(Filt.BaseCost == PriceAnlzFilt::bcByLoc) {
							for(i = 0; i < suppl_list.getCount(); i++) {
								const  PPID suppl_id = suppl_list.get(i);
								for(j = 0; cost_ary.enumItems(&j, (void **)&p_e);)
									if(p_e->SupplID == suppl_id && p_e->LocID == Filt.BaseLoc && p_e->Base > 0.0) {
										double base = p_e->Base;
										for(k = 0; cost_ary.enumItems(&k, (void **)&p_e2);)
											if(p_e2->SupplID == suppl_id)
												p_e->Base = base;
										break;
									}
							}
						}
						else if(Filt.BaseCost == PriceAnlzFilt::bcByAvgLocs) {
							for(i = 0; i < suppl_list.getCount(); i++) {
								const  PPID suppl_id = suppl_list.get(i);
								double base = 0.0;
								uint   base_count = 0;
								for(j = 0; cost_ary.enumItems(&j, (void **)&p_e);)
									if(p_e->SupplID == suppl_id && p_e->Base > 0.0) {
										base += p_e->Base;
										base_count++;
									}
								if(base_count) {
									base = base / base_count;
									for(j = 0; cost_ary.enumItems(&j, (void **)&p_e);)
										if(p_e->SupplID == suppl_id)
											p_e->Base = base;
								}
							}
						}
					}
					if(Filt.Flags & PriceAnlzFilt::fExclWOCntrCost) {
						skip = 1;
						for(j = 0; skip && cost_ary.enumItems(&j, (void **)&p_e);)
							if(p_e->Contract != 0.0)
								skip = 0;
					}
					if(!skip) {
						TempPriceAnlzTbl::Rec temp_rec;
						temp_rec.GoodsID = goods_rec.ID;
						STRNSCPY(temp_rec.GoodsName, goods_rec.Name);
						for(j = 0; cost_ary.enumItems(&j, (void **)&p_e);) {
							temp_rec.TabID    = p_e->LocID;
							temp_rec.SupplID  = p_e->SupplID;
							temp_rec.Cost     = p_e->Cost;
							temp_rec.ContractCost = p_e->Base;
							temp_rec.CostDiff = (p_e->Base == -1.0) ? 0.0 : (p_e->Cost - p_e->Base);
							if(Filt.Flags & PriceAnlzFilt::fShowDiffAsPrc && temp_rec.CostDiff != 0.0)
								temp_rec.CostDiff = (temp_rec.ContractCost == 0.0) ? 100.0 : (temp_rec.CostDiff / temp_rec.ContractCost) * 100.0;
							if(Filt.Flags & PriceAnlzFilt::fDivideBySuppl && p_e->SupplID) {
								GetArticleName(p_e->SupplID, ar_name);
								ar_name.CopyTo(temp_rec.SupplName, sizeof(temp_rec.SupplName));
							}
							THROW_DB(bei.insert(&temp_rec));
						}
					}
				}
				PPWaitPercent(giter.GetIterCounter());
			}
			THROW_DB(bei.flash());
			THROW(tra.Commit());
		}
	}
	{
		class PriceAnlzCrosstab : public Crosstab {
		public:
			PriceAnlzCrosstab(PPViewPriceAnlz * pV) : Crosstab(), P_V(pV)
			{
			}
			virtual BrowserWindow * CreateBrowser(uint brwId, int dataOwner)
			{
				PPViewBrowser * p_brw = new PPViewBrowser(brwId, CreateBrowserQuery(), P_V, dataOwner);
				SetupBrowserCtColumns(p_brw);
				return p_brw;
			}
		protected:
			virtual void GetTabTitle(const void * pVal, TYPEID typ, SString & rBuf) const
			{
				if(pVal && /*typ == MKSTYPE(S_INT, 4) &&*/ P_V) 
					P_V->GetTabTitle(*static_cast<const long *>(pVal), rBuf);
			}
			PPViewPriceAnlz * P_V;
		};
		ZDELETE(P_Ct);
		if(P_TempTbl) {
			THROW_MEM(P_Ct = new PriceAnlzCrosstab(this));
			P_Ct->SetTable(P_TempTbl, P_TempTbl->TabID);
			P_Ct->AddIdxField(P_TempTbl->GoodsID);
			P_Ct->AddIdxField(P_TempTbl->SupplID);
			P_Ct->SetSortIdx("GoodsName", "SupplName", 0L);
			P_Ct->AddInheritedFixField(P_TempTbl->GoodsName);
			P_Ct->AddInheritedFixField(P_TempTbl->SupplName);
			P_Ct->AddAggrField(P_TempTbl->ContractCost);
			P_Ct->AddAggrField(P_TempTbl->Cost);
			P_Ct->AddAggrField(P_TempTbl->CostDiff);
			THROW(P_Ct->Create(use_ta));
		}
	}
	CATCH
		ZDELETE(P_Ct);
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	PPWaitStop();
	return ok;
}

int PPViewPriceAnlz::InitIteration()
{
	TempPriceAnlzTbl::Key0 k, k_;
	BExtQuery::ZDelete(&P_IterQuery);
	MEMSZERO(k);
	P_IterQuery = new BExtQuery(P_TempTbl, 0);
	P_IterQuery->selectAll();
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(false, &k, spGe);
	return 1;
}

int FASTCALL PPViewPriceAnlz::NextIteration(PriceAnlzViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		Counter.Increment();
		ASSIGN_PTR(pItem, P_TempTbl->data);
		ok = 1;
	}
	return ok;
}

DBQuery * PPViewPriceAnlz::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = 0;
	DBQuery * p_q = 0;
	if(P_Ct) {
		brw_id = BROWSER_PRICEANLZ;
		p_q = PPView::CrosstabDbQueryStub;
	}
	if(pSubTitle) {
		SString obj_name, temp_buf;
		if(Filt.Flags & PriceAnlzFilt::fShowDiffAsPrc) {
			// @v9.6.3 (*pSubTitle = "Разница цен в %").ToOem();
			PPLoadString("priceanlzfilt_fshowdiffasprc", *pSubTitle); // @v9.6.3
		}
		if(Filt.SupplID) {
			PPLoadText(PPTXT_SUPPLTXT, temp_buf);
			GetArticleName(Filt.SupplID, obj_name);
			pSubTitle->CatDivIfNotEmpty(';', 2).CatEq(temp_buf, obj_name);
		}
		if(Filt.GoodsGrpID) {
			// @v10.7.10 PPGetWord(PPWORD_GROUP, 0, temp_buf);
			PPLoadStringS("group", temp_buf); // @v10.7.10
			GetObjectName(PPOBJ_GOODSGROUP, Filt.GoodsGrpID, obj_name);
			pSubTitle->CatDivIfNotEmpty(';', 2).CatEq(temp_buf, obj_name);
		}
		GetExtLocationName(Filt.LocList, 2, temp_buf);
		pSubTitle->CatDivIfNotEmpty('-', 1).Cat(temp_buf);
		pSubTitle->CatDivIfNotEmpty('-', 1).Cat(Filt.Period);
	}
	ASSIGN_PTR(pBrwId, brw_id);
	return p_q;
}

void PPViewPriceAnlz::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetTempGoodsGrp(Filt.GoodsGrpID);
		if(Filt.Flags & PriceAnlzFilt::fDivideBySuppl)
			pBrw->InsColumn(1, "@supplier", 4, 0, MKSFMT(15, 0), 0);
	}
}

/*virtual*/int PPViewPriceAnlz::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	struct _E {
		PPID   ID;
		PPID   GoodsID;
		PPID   SupplID;
	};
	uint   tab_idx = (pBrw) ? pBrw->GetCurColumn() : 0;
	PPID   g_id = (pHdr) ? static_cast<const _E *>(pHdr)->GoodsID : 0;
	PPID   suppl_id = (pHdr) ? static_cast<const _E *>(pHdr)->SupplID : 0;
	PPID   loc_id = 0;
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);

	if(tab_idx > 0)
		P_Ct->GetTab((tab_idx - ((Filt.Flags & PriceAnlzFilt::fDivideBySuppl) ? 2 : 1)) / 3, &loc_id);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_EDITGOODS:
				if(g_id)
					ok = GObj.Edit(&g_id, 0);
				break;
			case PPVCMD_QUOTS:
				ok = GObj.EditQuotations(g_id, loc_id, -1L , NZOR(Filt.SupplID, suppl_id), PPQuot::clsGeneral);
				break;
			case PPVCMD_VIEWLOTS:
				{
                    LotFilt lot_filt;
					if(loc_id)
						lot_filt.LocList.Add(loc_id);
					else
						lot_filt.LocList = Filt.LocList;
					lot_filt.GoodsID = g_id;
					lot_filt.Period  = Filt.Period;
					lot_filt.SupplID = NZOR(Filt.SupplID, suppl_id);
					ok = ViewLots(&lot_filt, 0, 0);
				}
				break;
			case PPVCMD_SETCONTRACTPRICES:
				ok = SetContractPrices();
				break;
		}
	}
	return ok;
}

int PPViewPriceAnlz::SetContractPrices()
{
	int    ok = -1;
	uint   cfm_msg = Filt.LocList.GetCount() ? PPCFM_SETCONTRACTPRICESSEL : PPCFM_SETCONTRACTPRICES;
	if(oneof3(Filt.CostAlg, PriceAnlzFilt::caByMinLot, PriceAnlzFilt::caByMinLoc, PriceAnlzFilt::caByLastLot) &&
		(Filt.Flags & PriceAnlzFilt::fDivideBySuppl) && CONFIRMCRIT(cfm_msg)) {
		struct _E {
			PPID   SupplID;
			PPID   GoodsID;
			double Cost;
		};
		PriceAnlzViewItem item;
		SArray suppl_cost_ary(sizeof(_E));
		PPIDArray locs_ary;
		PPWaitStart();
		MEMSZERO(item);
		if(Filt.LocList.GetCount())
			locs_ary.copy(Filt.LocList.Get());
		else
			locs_ary.add(0L);
		for(InitIteration(); NextIteration(&item) > 0;) {
			uint   pos = 0;
			double cost = 0.0;
			LAssoc k(item.SupplID, item.GoodsID);
			if(suppl_cost_ary.lsearch(&k, &pos, PTR_CMPFUNC(_2long))) {
				_E * p_e = static_cast<_E *>(suppl_cost_ary.at(pos));
				p_e->Cost = (item.Cost < p_e->Cost) ? item.Cost : p_e->Cost;
			}
			else {
				_E _e;
				_e.SupplID = item.SupplID;
				_e.GoodsID = item.GoodsID;
				_e.Cost    = item.Cost;
				THROW_SL(suppl_cost_ary.insert(&_e));
			}
			PPWaitPercent(GetCounter());
		}
		PPWaitStart();
		for(uint i = 0; i < suppl_cost_ary.getCount(); i++) {
			int    r = 0;
			_E   * p_e = (_E*)suppl_cost_ary.at(i);
			if(p_e->Cost != 0.0) {
				for(uint j = 0; j < locs_ary.getCount(); j++) {
					const QuotIdent qi(locs_ary.at(j), 0, 0, p_e->SupplID);
					PPSupplDeal sd;
					THROW(r = GObj.GetSupplDeal(p_e->GoodsID, qi, &sd, 1));
					sd.Cost = p_e->Cost;
					THROW(GObj.SetSupplDeal(p_e->GoodsID, qi, &sd, 1));
				}
			}
			PPWaitPercent(i + 1, suppl_cost_ary.getCount());
		}
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

void PPViewPriceAnlz::GetTabTitle(PPID tabID, SString & rBuf)
{
	rBuf.Z();
	if(tabID && P_TempTbl)
		GetLocationName(tabID, rBuf);
}
//
// Implementation of PPALDD_PriceAnlz
//
PPALDD_CONSTRUCTOR(PriceAnlz)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PriceAnlz) { Destroy(); }

int PPALDD_PriceAnlz::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(PriceAnlz, rsrv);
	H.FltSupplID    = p_filt->SupplID;
	H.FltPeriod_beg = p_filt->Period.low;
	H.FltPeriod_end = p_filt->Period.upp;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PriceAnlz::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(PriceAnlz);
}

int PPALDD_PriceAnlz::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(PriceAnlz);
	I.LocID        = item.TabID;
	I.GoodsID      = item.GoodsID;
	I.ContractCost = item.ContractCost;
	I.Cost = item.Cost;
	I.CostDiff     = item.CostDiff;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_PriceAnlz::Destroy() { DESTROY_PPVIEW_ALDD(PriceAnlz); }
