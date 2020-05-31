// V_GDSMOV.CPP
// Copyright (c) A.Sobolev, A.Starodub 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2008, 2009, 2010, 2011, 2014, 2015, 2016, 2017, 2018, 2019, 2020
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(GoodsMov); SLAPI GoodsMovFilt::GoodsMovFilt() : PPBaseFilt(PPFILT_GOODSMOV, 0, 1)
{
	SetFlatChunk(offsetof(GoodsMovFilt, ReserveStart), offsetof(GoodsMovFilt, LocList) - offsetof(GoodsMovFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(GoodsMovFilt, LocList));
	Init(1, 0);
}

GoodsMovFilt & FASTCALL GoodsMovFilt::operator = (const GoodsMovFilt & s)
{
	Copy(&s, 0);
	return *this;
}
//
//
//
SLAPI GoodsMovTotal::GoodsMovTotal()
{
	Init();
}

void SLAPI GoodsMovTotal::Init()
{
	THISZERO();
}

int SLAPI GoodsMovTotal::IsEmpty() const
{
	return !(InRestQtty || InRestPhQtty || InRestCost || InRestPrice || OutRestQtty || OutRestPhQtty || OutRestCost || OutRestPrice);
}

int SLAPI GoodsMovTotal::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, InRestQtty,    rBuf));
	THROW_SL(pCtx->Serialize(dir, InRestPhQtty,  rBuf));
	THROW_SL(pCtx->Serialize(dir, InRestCost,    rBuf));
	THROW_SL(pCtx->Serialize(dir, InRestPrice,   rBuf));
	THROW_SL(pCtx->Serialize(dir, OutRestQtty,   rBuf));
	THROW_SL(pCtx->Serialize(dir, OutRestPhQtty, rBuf));
	THROW_SL(pCtx->Serialize(dir, OutRestCost,   rBuf));
	THROW_SL(pCtx->Serialize(dir, OutRestPrice,  rBuf));
	CATCHZOK
	return ok;
}
//
// @ModuleDecl(PPViewGoodsMov)
//
SLAPI PPViewGoodsMov::PPViewGoodsMov() : PPView(0, &Filt, PPVIEW_GOODSMOV), P_BObj(BillObj), P_TempTbl(0), P_GGIter(0), PrintWoPacks(0)
{
	ImplementFlags |= implUseServer;
	DefReportId = REPORT_GOODSMOV;
}

SLAPI PPViewGoodsMov::~PPViewGoodsMov()
{
	delete P_GGIter;
	delete P_TempTbl;
}
//
//
//
#define GRP_GOODSFILT  1
#define GRP_LOC        2

GoodsMovFiltDialog::GoodsMovFiltDialog() : WLDialog(DLG_GDSMOV, CTL_GTO_LABEL)
{
	addGroup(GRP_GOODSFILT, new GoodsFiltCtrlGroup(CTLSEL_GTO_GOODS, CTLSEL_GTO_GGRP, cmGoodsFilt));
	addGroup(GRP_LOC, new LocationCtrlGroup(CTLSEL_GTO_LOC, 0, 0, cmLocList, 0, 0, 0));
	SetupCalCtrl(CTLCAL_GTO_PERIOD, this, CTL_GTO_PERIOD, 1);
}

IMPL_HANDLE_EVENT(GoodsMovFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmClusterClk) && event.isCtlEvent(CTL_GTO_OLDALG)) {
		SetupCtrls();
		clearEvent(event);
	}
}

void GoodsMovFiltDialog::SetupCtrls()
{
	long flags = 0;
	GetClusterData(CTL_GTO_OLDALG, &flags);
	const int to_disable = BIN(flags & GoodsMovFilt::fUseOldAlg);
	disableCtrl(CTL_GTO_PRICEKIND, to_disable);
	disableCtrl(CTLSEL_GTO_OPR, to_disable);
}

int GoodsMovFiltDialog::setDTS(const GoodsMovFilt * pData)
{
	if(!RVALUEPTR(Data, pData))
		Data.Init(1, 0);
	PPIDArray types;
	SetPeriodInput(this, CTL_GTO_PERIOD, &Data.Period);
	LocationCtrlGroup::Rec l_rec(&Data.LocList);
	setGroupData(GRP_LOC, &l_rec);
	SetupArCombo(this, CTLSEL_GTO_SUPPL, Data.SupplID, OLW_LOADDEFONOPEN, GetSupplAccSheet(), sacfDisableIfZeroSheet);
	GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, 0, 0, GoodsCtrlGroup::enableSelUpLevel);
	setGroupData(GRP_GOODSFILT, &gf_rec);
	setWL((Data.Flags & GoodsMovFilt::fLabelOnly) ? 1 : 0);
	AddClusterAssoc(CTL_GTO_FLAGS, 0, GoodsMovFilt::fCostWoVat);
	AddClusterAssoc(CTL_GTO_FLAGS, 1, GoodsMovFilt::fPriceWoVat); // @v10.6.6
	SetClusterData(CTL_GTO_FLAGS, Data.Flags);
	SetupArCombo(this, CTLSEL_GTO_SUPPLAG, Data.SupplAgentID, OLW_LOADDEFONOPEN|OLW_CANINSERT, GetAgentAccSheet(), sacfDisableIfZeroSheet);
	SetupPPObjCombo(this, CTLSEL_GTO_BRAND,   PPOBJ_BRAND,   Data.BrandID, OLW_LOADDEFONOPEN, 0);
	{
		PPIDArray gen_op_list;
		gen_op_list.add(PPOPT_GENERIC);
		SetupOprKindCombo(this, CTLSEL_GTO_OPR, Data.OpID, 0, &gen_op_list, OLW_CANEDIT);
		AddClusterAssoc(CTL_GTO_OLDALG, 0, GoodsMovFilt::fUseOldAlg);
		SetClusterData(CTL_GTO_OLDALG, Data.Flags);

		AddClusterAssocDef(CTL_GTO_PRICEKIND,  0, GoodsMovFilt::prkBasePrice);
		AddClusterAssoc(CTL_GTO_PRICEKIND,  1, GoodsMovFilt::prkCost);
		AddClusterAssoc(CTL_GTO_PRICEKIND,  2, GoodsMovFilt::prkPrice);
		SetClusterData(CTL_GTO_PRICEKIND, (long)Data.PriceKind);
	}
	disableCtrl(CTL_GTO_OLDALG, BIN(Data.Flags & GoodsMovFilt::fInited));
	return 1;
}

int GoodsMovFiltDialog::getDTS(GoodsMovFilt * pData)
{
	int    ok = 1;
	GoodsFiltCtrlGroup::Rec gf_rec;
	LocationCtrlGroup::Rec l_rec;
	getGroupData(GRP_LOC, &l_rec);
	Data.LocList = l_rec.LocList;
	THROW(GetPeriodInput(this, CTL_GTO_PERIOD, &Data.Period));
	THROW(AdjustPeriodToRights(Data.Period, 0));
	getCtrlData(CTLSEL_GTO_SUPPL, &Data.SupplID);
	THROW(getGroupData(GRP_GOODSFILT, &gf_rec));
	Data.GoodsGrpID = gf_rec.GoodsGrpID;
	//Data.GoodsID    = rec.GoodsID;
	SETFLAG(Data.Flags, GoodsMovFilt::fLabelOnly, getWL());
	GetClusterData(CTL_GTO_FLAGS, &Data.Flags);
	getCtrlData(CTL_GTO_SUPPLAG, &Data.SupplAgentID);
	getCtrlData(CTL_GTO_BRAND,   &Data.BrandID);
	{
		long v = 0;
		getCtrlData(CTLSEL_GTO_OPR, &Data.OpID);
		GetClusterData(CTL_GTO_OLDALG, &Data.Flags);
		GetClusterData(CTL_GTO_PRICEKIND, &v);
		Data.PriceKind = (int16)v;
	}
	Data.Flags |= GoodsMovFilt::fInited;
	*pData = Data;
	CATCHZOKPPERR
	return ok;
}

/*virtual*/int SLAPI PPViewGoodsMov::EditBaseFilt(PPBaseFilt * pFilt)
{
	DIALOG_PROC_BODY(GoodsMovFiltDialog, static_cast<GoodsMovFilt *>(pFilt));
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempGoodsMov);
PP_CREATE_TEMP_FILE_PROC(CreateTempFile2, TempGoodsMov2);

int SLAPI PPViewGoodsMov::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	int    is_link = 0;
	uint   i;
	const  int accs_cost = P_BObj->CheckRights(BILLRT_ACCSCOST);
	GCTFilt temp_filt;
	AdjGdsGrpng agg;
	GoodsGrpngEntry * p_entry;
	PPOprKind op_rec;
	GoodsFilt gf;
	GoodsIterator  iter(static_cast<GoodsFilt *>(0), 0);
	Goods2Tbl::Rec gr;
	PPObjGoods gobj;
	TempGoodsMovTbl * p_tbl = CreateTempFile();

	THROW(Helper_InitBaseFilt(pFilt));
	THROW(p_tbl);
	Total.Init();
	Filt.Period.Actualize(ZERODATE);
	temp_filt.Period = Filt.Period;
	temp_filt.LocList = Filt.LocList;
	temp_filt.SupplID = Filt.SupplID;
	temp_filt.GoodsGrpID = Filt.GoodsGrpID;
	temp_filt.SupplAgentID = Filt.SupplAgentID; // AHTOXA
	SETFLAG(temp_filt.Flags, OPG_LABELONLY, Filt.Flags & GoodsMovFilt::fLabelOnly);
	temp_filt.Flags |= (OPG_CALCINREST | OPG_CALCOUTREST | OPG_SETTAXES);
	if(Filt.Flags & GoodsMovFilt::fCostWoVat)
		temp_filt.Flags |= OPG_SETCOSTWOTAXES;
	// @v10.6.6 {
	if(Filt.Flags & GoodsMovFilt::fPriceWoVat)
		temp_filt.Flags |= OPG_SETPRICEWOTAXES;
	// } @v10.6.6 
	{
		BExtInsert bei(p_tbl);
		GoodsGrpngArray ary;
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		THROW(agg.BeginGoodsGroupingProcess(temp_filt));
		gf.GrpID   = Filt.GoodsGrpID;
		gf.SupplID = Filt.SupplID;
		gf.BrandList.Add(Filt.BrandID);
		for(iter.Init(&gf, 0); iter.Next(&gr) > 0;) {
			THROW(PPCheckUserBreak());
			if(!(gr.Flags & GF_GENERIC)) {
				TempGoodsMovTbl::Rec rec;
				// @v10.6.6 @ctr MEMSZERO(rec);
				temp_filt.GoodsID = gr.ID;
				ary.clear();
				THROW(ary.ProcessGoodsGrouping(temp_filt, &agg));
				for(i = 0; ary.enumItems(&i, (void **)&p_entry);) {
					int no_upd_lot_op = 0;
					if(!accs_cost)
						p_entry->Cost = 0;
					if(p_entry->OpID == -1) {
						rec.InRest_Qtty  += p_entry->Quantity;
						rec.InRest_Cost  += p_entry->Cost;
						rec.InRest_Price += p_entry->Price;
					}
					else if(p_entry->OpID == 10000) {
						rec.OutRest_Qtty  += p_entry->Quantity;
						rec.OutRest_Cost  += p_entry->Cost;
						rec.OutRest_Price += p_entry->Price;
					}
					else {
						is_link = BIN(GetOpData(p_entry->Link, &op_rec));
						if(CheckOpFlags(p_entry->OpID, OPKF_NOUPDLOTREST, 0))
							no_upd_lot_op = 1;
						if(!no_upd_lot_op) {
							if(IsIntrOp(p_entry->OpID) == INTRRCPT) {
								rec.ARcpt_Qtty  += p_entry->Quantity;
								rec.ARcpt_Cost  += p_entry->Cost;
								rec.ARcpt_Price += p_entry->Price;
							}
							else if(IsIntrExpndOp(p_entry->OpID)) {
								rec.Expnd_Qtty  += p_entry->Quantity;
								rec.Expnd_Cost  += p_entry->Cost;
								rec.Expnd_Price += p_entry->Price;
							}
							else if(p_entry->OpTypeID == PPOPT_GOODSMODIF) {
								if(p_entry->Sign > 0) {
									rec.ARcpt_Qtty  += p_entry->Quantity;
									rec.ARcpt_Cost  += p_entry->Cost;
									rec.ARcpt_Price += p_entry->Price;
								}
								else if(p_entry->Sign < 0) {
									rec.Expnd_Qtty  += p_entry->Quantity;
									rec.Expnd_Cost  += p_entry->Cost;
									rec.Expnd_Price += p_entry->Price;
								}
							}
							else if(p_entry->OpTypeID == PPOPT_GOODSRETURN) {
								if(is_link) {
									if(op_rec.OpTypeID == PPOPT_GOODSEXPEND) {
										rec.ARcpt_Qtty  += p_entry->Quantity;
										rec.ARcpt_Cost  += p_entry->Cost;
										rec.ARcpt_Price += p_entry->Price;
									}
									else if(op_rec.OpTypeID == PPOPT_GOODSRECEIPT) {
										rec.Expnd_Qtty  += p_entry->Quantity;
										rec.Expnd_Cost  += p_entry->Cost;
										rec.Expnd_Price += p_entry->Price;
									}
								}
								else if(p_entry->Sign > 0) {
									rec.ARcpt_Qtty  += p_entry->Quantity;
									rec.ARcpt_Cost  += p_entry->Cost;
									rec.ARcpt_Price += p_entry->Price;
								}
								else if(p_entry->Sign < 0) {
									rec.Expnd_Qtty  += p_entry->Quantity;
									rec.Expnd_Cost  += p_entry->Cost;
									rec.Expnd_Price += p_entry->Price;
								}
							}
							else if(p_entry->OpTypeID == PPOPT_PAYMENT) {
								if(is_link && op_rec.Flags & OPKF_NEEDPAYMENT && IsExpendOp(op_rec.ID) > 0) {
									rec.Rlz_Qtty    += p_entry->Quantity;
									rec.Rlz_Cost    += p_entry->Cost;
									rec.Rlz_Price   += p_entry->Price;
								}
							}
							else if(p_entry->OpTypeID == PPOPT_GOODSRECEIPT) {
								rec.Rcpt_Qtty  += p_entry->Quantity;
								rec.Rcpt_Cost  += p_entry->Cost;
								rec.Rcpt_Price += p_entry->Price;
							}
							else if(p_entry->OpTypeID == PPOPT_GOODSEXPEND) {
								if(CheckOpFlags(p_entry->OpID, OPKF_PROFITABLE)) {
									rec.SRlz_Qtty  += p_entry->Quantity;
									rec.SRlz_Cost  += p_entry->Cost;
									rec.SRlz_Price += p_entry->Price;
								}
								else {
									rec.Expnd_Qtty  += p_entry->Quantity;
									rec.Expnd_Cost  += p_entry->Cost;
									rec.Expnd_Price += p_entry->Price;
								}
							}
						}
					}
					if(p_entry->OpID != -1 && p_entry->OpID != 10000 && p_entry->OpID && !no_upd_lot_op) {
						int expnd = -1;
						if(p_entry->OpID == PPOPK_INTRRECEIPT)
							expnd = 0;
						else if(oneof3(p_entry->OpTypeID, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, PPOPT_GOODSRETURN))
							expnd = BIN(IsExpendOp(p_entry->OpID) > 0);
						else if(p_entry->OpTypeID == PPOPT_GOODSMODIF) {
							if(p_entry->Sign > 0)
								expnd = 0;
							else if(p_entry->Sign < 0)
								expnd = 1;
						}
				   		if(expnd == 1) {
							rec.TExpnd_Qtty  += p_entry->Quantity;
							rec.TExpnd_Cost  += p_entry->Cost;
							rec.TExpnd_Price += p_entry->Price;
						}
						else if(expnd == 0) {
							rec.TRcpt_Qtty  += p_entry->Quantity;
							rec.TRcpt_Cost  += p_entry->Cost;
							rec.TRcpt_Price += p_entry->Price;
						}
					}
				}
				if(rec.InRest_Qtty || rec.OutRest_Qtty || rec.Rcpt_Qtty || rec.ARcpt_Qtty || rec.Expnd_Qtty || rec.Rlz_Qtty || rec.TExpnd_Qtty || rec.TRcpt_Qtty) {
					SString temp_buf;
					rec.Grp     = gr.ParentID;
					rec.GoodsID = gr.ID;
					rec.PhUPerU = gr.PhUPerU;
					Total.InRestQtty    += rec.InRest_Qtty;
					Total.InRestPhQtty  += rec.InRest_Qtty * rec.PhUPerU;
					Total.InRestCost    += rec.InRest_Cost;
					Total.InRestPrice   += rec.InRest_Price;
					Total.OutRestQtty   += rec.OutRest_Qtty;
					Total.OutRestPhQtty += rec.OutRest_Qtty * rec.PhUPerU;
					Total.OutRestCost   += rec.OutRest_Cost;
					Total.OutRestPrice  += rec.OutRest_Price;
					STRNSCPY(rec.GoodsName, gr.Name);
					gobj.GetSingleBarcode(rec.GoodsID, temp_buf);
					temp_buf.CopyTo(rec.Barcode, sizeof(rec.Barcode));
					THROW_DB(bei.insert(&rec));
				}
			}
			PPWaitPercent(iter.GetIterCounter());
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
	}
	CATCH
		ok = 0;
		ZDELETE(p_tbl);
	ENDCATCH
	agg.EndGoodsGroupingProcess();
	P_TempTbl = p_tbl;
	return ok;
}

int SLAPI PPViewGoodsMov::InitIterQuery(PPID grpID)
{
	// @v10.6.8 char   k_[MAXKEYLEN];
	BtrDbKey k__; // @v10.6.8 
	TempGoodsMovTbl::Key0 k0;
	TempGoodsMovTbl::Key1 k1;
	void * k;
	int    sp_mode = spFirst;
   	// @v10.6.8 @ctr memzero(k_, sizeof(k_));
	k = k__;
	delete P_IterQuery;
	P_IterQuery = new BExtQuery(P_TempTbl, IterIdx, 10);
	P_IterQuery->selectAll();
	if(grpID) {
		P_IterQuery->where(P_TempTbl->Grp == grpID);
		k = MEMSZERO(k0);
		k0.Grp = grpID;
		sp_mode = spGe;
	}
	else {
		k = MEMSZERO(k1);
		sp_mode = spFirst;
	}
	P_IterQuery->initIteration(0, k, sp_mode);
	return 1;
}

int SLAPI PPViewGoodsMov::InitGroupNamesList()
{
	IterGrpName = 0;
	P_GGIter = new GoodsGroupIterator((PPObjGoodsGroup::IsAlt(Filt.GoodsGrpID) > 0) ? 0 : Filt.GoodsGrpID);
	return P_GGIter ? 1 : PPSetErrorNoMem();
}

int SLAPI PPViewGoodsMov::InitIteration(IterOrder ord)
{
	int    ok = 1;

	IterCount = 0;
	NumIters  = 0;
	IterIdx   = 0;
	IterGrpName = 0;
	ZDELETE(P_GGIter);
	BExtQuery::ZDelete(&P_IterQuery);

	if(P_TempTbl) {
		RECORDNUMBER num_recs = 0;
		P_TempTbl->getNumRecs(&num_recs);
		NumIters = num_recs;
		if(ord == OrdByDefault)
			IterIdx = 1;
		else if(ord == OrdByGoodsName)
			IterIdx = 1;
		else if(ord == OrdByGrp_GoodsName) {
			IterIdx = 0;
			InitGroupNamesList();
		}
		else
			IterIdx = 1;
		if(IterIdx != 0)
			ok = InitIterQuery(0);
		else
			ok = NextOuterIteration();
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPViewGoodsMov::NextOuterIteration()
{
	PPID   grp_id = 0;
	if(P_GGIter && P_GGIter->Next(&grp_id, IterGrpName) > 0) {
		InitIterQuery(grp_id);
		return 1;
	}
	else
		return -1;
}

double SLAPI PPViewGoodsMov::GetUnitsPerPack(PPID goodsID)
{
	double pack = 0.0;
	ReceiptCore & rcpt = P_BObj->trfr->Rcpt;
	if(rcpt.GetLastLot(goodsID, Filt.LocList.GetSingle(), MAXDATE, 0) > 0) // AHTOXA
		pack = rcpt.data.UnitPerPack;
	else if(Filt.LocList.GetCount())
		if(rcpt.GetLastLot(goodsID, 0, MAXDATE, 0) > 0)
			pack = rcpt.data.UnitPerPack;
	return pack;
}

int FASTCALL PPViewGoodsMov::NextIteration(GoodsMovViewItem * pItem)
{
	do {
		if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
			if(pItem) {
				memzero(pItem, sizeof(GoodsMovViewItem));
#define CPY_FLD(fd, fs) pItem->fd = P_TempTbl->data.fs
				CPY_FLD(GoodsID, GoodsID);
				pItem->UnitsPerPack = GetUnitsPerPack(pItem->GoodsID);
				pItem->P_GoodsGrpName = IterGrpName;
				CPY_FLD(PhPerU, PhUPerU);
				CPY_FLD(InRest_Qtty, InRest_Qtty);
				CPY_FLD(InRest_Cost, InRest_Cost);
				CPY_FLD(InRest_Price, InRest_Price);

				CPY_FLD(Rcpt_Qtty, Rcpt_Qtty);
				CPY_FLD(Rcpt_Cost, Rcpt_Cost);
				CPY_FLD(Rcpt_Price, Rcpt_Price);

				CPY_FLD(_Rcpt_Qtty, ARcpt_Qtty);
				CPY_FLD(_Rcpt_Cost, ARcpt_Cost);
				CPY_FLD(_Rcpt_Price, ARcpt_Price);

				CPY_FLD(Rlz_Qtty, Rlz_Qtty);
				CPY_FLD(Rlz_Cost, Rlz_Cost);
				CPY_FLD(Rlz_Price, Rlz_Price);

				CPY_FLD(SRlz_Qtty, SRlz_Qtty);
				CPY_FLD(SRlz_Cost, SRlz_Cost);
				CPY_FLD(SRlz_Price, SRlz_Price);

				CPY_FLD(Expnd_Qtty,  Expnd_Qtty);
				CPY_FLD(Expnd_Cost,  Expnd_Cost);
				CPY_FLD(Expnd_Price, Expnd_Price);

				CPY_FLD(TRcpt_Qtty, TRcpt_Qtty);
				CPY_FLD(TRcpt_Cost, TRcpt_Cost);
				CPY_FLD(TRcpt_Price, TRcpt_Price);

				CPY_FLD(TExpnd_Qtty, TExpnd_Qtty);
				CPY_FLD(TExpnd_Cost, TExpnd_Cost);
				CPY_FLD(TExpnd_Price, TExpnd_Price);

				CPY_FLD(OutRest_Qtty, OutRest_Qtty);
				CPY_FLD(OutRest_Cost, OutRest_Cost);
				CPY_FLD(OutRest_Price, OutRest_Price);
#undef CPY_FLD
			}
			IterCount++;
			return 1;
		}
	} while(NextOuterIteration() > 0);
	return -1;
}

int SLAPI PPViewGoodsMov::GetIterationCount(long * pNumIterations, long * pLastCount)
{
	ASSIGN_PTR(pNumIterations, NumIters);
	ASSIGN_PTR(pLastCount, IterCount);
	return 1;
}

int SLAPI PPViewGoodsMov::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	PPID   goods_id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	if(goods_id) {
		OpGroupingFilt op_grpng_flt;
		op_grpng_flt.Period  = Filt.Period;
		op_grpng_flt.LocList = Filt.LocList;
		op_grpng_flt.SupplID = Filt.SupplID;
		op_grpng_flt.SupplAgentID = Filt.SupplAgentID;
		op_grpng_flt.GoodsID = goods_id;
		op_grpng_flt.Flags  |= OpGroupingFilt::fCalcRest;
		ViewOpGrouping(&op_grpng_flt);
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewGoodsMov::EditGoods(PPID goodsID)
{
	int    ok = -1;
	if(goodsID > 0) {
		PPObjGoods gobj;
		if((ok = gobj.Edit(&goodsID, 0)) == cmOK)
			ok = 1;
		else if(ok)
			ok = -1;
	}
	return ok;
}

/*virtual*/int SLAPI PPViewGoodsMov::Print(const void *)
{
	class PrintGoodsMovDialog : public TDialog {
	public:
		PrintGoodsMovDialog() : TDialog(DLG_PRNGOODSMOV)
		{
			disableCtrl(CTL_PRNGOODSMOV_SHRTMOVF, 1);
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_PRNGOODSMOV_WHAT)) {
				ushort what = getCtrlUInt16(CTL_PRNGOODSMOV_WHAT);
				disableCtrl(CTL_PRNGOODSMOV_SHRTMOVF, (what == 0));
				clearEvent(event);
			}
		}
	};
	int    ok = 1;
	uint   rpt_id = 0;
	int    avprice = 0, disable_grouping = 0, shrt_mov_prn_what = 0;
	ushort v = 0, what = 0;
	PrintGoodsMovDialog * p_dlg = 0;
	PPReportEnv env;
	PView  pv(this);
	THROW(CheckDialogPtr(&(p_dlg = new PrintGoodsMovDialog())));
	p_dlg->setCtrlData(CTL_PRNGOODSMOV_WHAT, &what);
	SETFLAG(v, 0x0001, avprice);
	SETFLAG(v, 0x0002, disable_grouping);
	SETFLAG(v, 0x0004, PrintWoPacks);
	p_dlg->setCtrlData(CTL_PRNGOODSMOV_AVPRICE, &v);
	if(ExecView(p_dlg) == cmOK) {
		p_dlg->getCtrlData(CTL_PRNGOODSMOV_WHAT, &what);
		p_dlg->getCtrlData(CTL_PRNGOODSMOV_AVPRICE, &v);
		avprice          = BIN(v & 0x01);
		disable_grouping = BIN(v & 0x02);
		PrintWoPacks     = BIN(v & 0x04);
		if(what == 0)
			rpt_id = avprice ? REPORT_GOODSMOVP : REPORT_GOODSMOV;
		else {
			p_dlg->getCtrlData(CTL_PRNGOODSMOV_SHRTMOVF, &shrt_mov_prn_what);
			if(shrt_mov_prn_what == 2)
				rpt_id = REPORT_SHRTGOODSMOVPRICE;
			else if(shrt_mov_prn_what == 1)
				rpt_id = REPORT_SHRTGOODSMOVCOST;
			else
				rpt_id = avprice ? REPORT_SHRTGOODSMOVP : REPORT_SHRTGOODSMOV;
		}
	}
	else
		rpt_id = 0;
	delete p_dlg;
	p_dlg = 0;
	if(rpt_id) {
		if(disable_grouping /*|| Filt.GoodsGrpID*/)
			env.Sort = PPViewGoodsMov::OrdByGoodsName;
		else
			env.Sort = PPViewGoodsMov::OrdByGrp_GoodsName;
		env.PrnFlags = disable_grouping ? SReport::DisableGrouping : 0;
		PPAlddPrint(rpt_id, &pv, &env);
		ok = 1;
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}

void SLAPI PPViewGoodsMov::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, SetTempGoodsGrp(Filt.GoodsGrpID));
}

DBQuery * SLAPI PPViewGoodsMov::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * p_q = 0;
	SString loc_names, subtitle;
	uint   brw_id = BROWSER_GOODSMOV;
	TempGoodsMovTbl * p_t = new TempGoodsMovTbl(P_TempTbl->GetName());
	p_q = & select(
		p_t->GoodsID,      // #00
		p_t->GoodsName,    // #01
		p_t->InRest_Qtty,  // #02
		p_t->InRest_Cost,  // #03
		p_t->Rcpt_Qtty,    // #04
		p_t->Rcpt_Cost,    // #05
		p_t->ARcpt_Qtty,   // #06
		p_t->ARcpt_Cost,   // #07
		p_t->SRlz_Qtty,    // #08
		p_t->SRlz_Price,   // #09
		p_t->Expnd_Qtty,   // #10
		p_t->Expnd_Cost,   // #11
		p_t->OutRest_Qtty, // #12
		p_t->OutRest_Cost, // #13
		p_t->Barcode,      // #14
		0L).from(p_t, 0L).orderBy(p_t->GoodsName, 0L);
	THROW(CheckQueryPtr(p_q));
	subtitle.CatDivIfNotEmpty('-', 1).Cat(GetExtLocationName(Filt.LocList, 2, loc_names));
	ASSIGN_PTR(pBrwId, brw_id);
	ASSIGN_PTR(pSubTitle, subtitle);
	CATCH
		if(p_q) {
			ZDELETE(p_q);
		}
		else
			delete p_t;
	ENDCATCH
	return p_q;
}

/*virtual*/int SLAPI PPViewGoodsMov::ViewTotal()
{
	int    ok = 1;
	TDialog * p_dlg = 0;
	THROW_MEM(p_dlg = new TDialog(DLG_GDSMOVT));
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setCtrlData(CTL_GDSMOVT_INRESTQTY,    &Total.InRestQtty);
	p_dlg->setCtrlData(CTL_GDSMOVT_INRESTPHQTY,  &Total.InRestPhQtty);
	p_dlg->setCtrlData(CTL_GDSMOVT_INRESTCOST,   &Total.InRestCost);
	p_dlg->setCtrlData(CTL_GDSMOVT_INRESTPRICE,  &Total.InRestPrice);
	p_dlg->setCtrlData(CTL_GDSMOVT_OUTRESTQTY,   &Total.OutRestQtty);
	p_dlg->setCtrlData(CTL_GDSMOVT_OUTRESTPHQTY, &Total.OutRestPhQtty);
	p_dlg->setCtrlData(CTL_GDSMOVT_OUTRESTCOST,  &Total.OutRestCost);
	p_dlg->setCtrlData(CTL_GDSMOVT_OUTRESTPRICE, &Total.OutRestPrice);
	ExecViewAndDestroy(p_dlg);
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewGoodsMov::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		ok = -2;
		switch(ppvCmd) {
			case PPVCMD_ADDTOBASKET:
				AddGoodsToBasket(id, Filt.LocList.GetSingle());
				break;
			case PPVCMD_EDITGOODS:
				ok = EditGoods(id);
				break;
			case PPVCMD_VIEWLOTS:
				if(id)
					::ViewLots(id, Filt.LocList.GetSingle(), Filt.SupplID, 0, 1);
				break;
		}
	}
	return ok;
}

int SLAPI PPViewGoodsMov::SerializeState(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int  ok = 1;
	THROW(PPView::SerializeState(dir, rBuf, pCtx));
	THROW(Total.Serialize(dir, rBuf, pCtx));
	THROW_SL(pCtx->Serialize(dir, IterCount,   rBuf));
	THROW_SL(pCtx->Serialize(dir, NumIters,    rBuf));
	THROW_SL(pCtx->Serialize(dir, IterIdx,     rBuf));
	THROW_SL(pCtx->Serialize(dir, IterGrpName, rBuf));
	THROW(SerializeDbTableByFileName <TempGoodsMovTbl> (dir, &P_TempTbl, rBuf, pCtx));
	CATCHZOK
	return ok;
}
//
// Implementation of PPALDD_GoodsMov
//
PPALDD_CONSTRUCTOR(GoodsMov)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsMov) { Destroy(); }

int PPALDD_GoodsMov::InitData(PPFilt & rFilt, long rsrv)
{
	PPViewGoodsMov * p_v = 0;
	if(rsrv) {
		p_v = static_cast<PPViewGoodsMov *>(rFilt.Ptr);
		Extra[1].Ptr = p_v;
	}
	else {
		p_v = new PPViewGoodsMov;
		Extra[0].Ptr = p_v;
		p_v->Init_(static_cast<const GoodsMovFilt *>(rFilt.Ptr));
	}
	SString temp_buf;
	const GoodsMovFilt * p_flt  = static_cast<const GoodsMovFilt *>(p_v->GetBaseFilt());
	H.FltBeg  = p_flt->Period.low;
	H.FltEnd  = p_flt->Period.upp;
	H.fLabelOnly    = BIN(p_flt->Flags & GoodsMovFilt::fLabelOnly);
	H.fCostWoVat    = BIN(p_flt->Flags & GoodsMovFilt::fCostWoVat);
	PPFormatPeriod(&p_flt->Period, temp_buf).CopyTo(H.Period, sizeof(H.Period));
	H.FltLocID      = p_flt->LocList.GetSingle();
	H.FltSupplID    = p_flt->SupplID;
	H.FltGoodsGrpID = p_flt->GoodsGrpID;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsMov::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	PPViewGoodsMov * p_v = static_cast<PPViewGoodsMov *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	return p_v->InitIteration(static_cast<PPViewGoodsMov::IterOrder>(SortIdx)) ? 1 : 0;
}

int PPALDD_GoodsMov::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	PPViewGoodsMov * p_v = static_cast<PPViewGoodsMov *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	GoodsMovViewItem item;
	if(p_v->NextIteration(&item) > 0) {
		long qttyf = (LConfig.Flags & CFGFLG_USEPACKAGE && !p_v->PrintWoPacks) ?
			MKSFMTD(0, 3, QTTYF_COMPLPACK | NMBF_NOTRAILZ) :
			MKSFMTD(0, 3, QTTYF_NUMBER | NMBF_NOTRAILZ);

		I.GoodsID = item.GoodsID;
		STRNSCPY(I.GoodsGrpName, item.P_GoodsGrpName);
		I.PhPerU = item.PhPerU;

		I.InRest_Qtty  = item.InRest_Qtty;
		I.InRest_Cost  = item.InRest_Cost;
		I.InRest_Price = item.InRest_Price;
		QttyToStr(item.InRest_Qtty, item.UnitsPerPack, qttyf, I.InRest_CQtty, 1);

		I.OutRest_Qtty  = item.OutRest_Qtty;
		I.OutRest_Cost  = item.OutRest_Cost;
		I.OutRest_Price = item.OutRest_Price;
		QttyToStr(item.OutRest_Qtty, item.UnitsPerPack, qttyf, I.OutRest_CQtty, 1);

		I.Rcpt_Qtty  = item.Rcpt_Qtty;
		I.Rcpt_Cost  = item.Rcpt_Cost;
		I.Rcpt_Price = item.Rcpt_Price;
		QttyToStr(item.Rcpt_Qtty, item.UnitsPerPack, qttyf, I.Rcpt_CQtty, 1);

		I._Rcpt_Qtty  = item._Rcpt_Qtty;
		I._Rcpt_Cost  = item._Rcpt_Cost;
		I._Rcpt_Price = item._Rcpt_Price;
		QttyToStr(item._Rcpt_Qtty, item.UnitsPerPack, qttyf, I._Rcpt_CQtty, 1);

		I.Rlz_Qtty  = item.Rlz_Qtty;
		I.Rlz_Cost  = item.Rlz_Cost;
		I.Rlz_Price = item.Rlz_Price;
		QttyToStr(item.Rlz_Qtty, item.UnitsPerPack, qttyf, I.Rlz_CQtty, 1);

		I.SRlz_Qtty  = item.SRlz_Qtty;
		I.SRlz_Cost  = item.SRlz_Cost;
		I.SRlz_Price = item.SRlz_Price;
		QttyToStr(item.SRlz_Qtty, item.UnitsPerPack, qttyf, I.SRlz_CQtty, 1);

		I.Expnd_Qtty  = item.Expnd_Qtty;
		I.Expnd_Cost  = item.Expnd_Cost;
		I.Expnd_Price = item.Expnd_Price;
		QttyToStr(item.Expnd_Qtty, item.UnitsPerPack, qttyf, I.Expnd_CQtty, 1);

		I.TExpnd_Qtty  = item.TExpnd_Qtty;
		I.TExpnd_Cost  = item.TExpnd_Cost;
		I.TExpnd_Price = item.TExpnd_Price;
		QttyToStr(item.TExpnd_Qtty, item.UnitsPerPack, qttyf, I.TExpnd_CQtty, 1);

		I.TRcpt_Qtty  = item.TRcpt_Qtty;
		I.TRcpt_Cost  = item.TRcpt_Cost;
		I.TRcpt_Price = item.TRcpt_Price;
		QttyToStr(item.TRcpt_Qtty, item.UnitsPerPack, qttyf, I.TRcpt_CQtty, 1);
		return DlRtm::NextIteration(iterId);
	}
	else
		return -1;
}

void PPALDD_GoodsMov::Destroy()
{
	delete static_cast<PPViewGoodsMov *>(Extra[0].Ptr);
	Extra[0].Ptr = Extra[1].Ptr = 0;
}

