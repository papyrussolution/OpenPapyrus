// V_GREST.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>

SLAPI GoodsRestViewItem::GoodsRestViewItem()
{
	THISZERO();
}

IMPLEMENT_PPFILT_FACTORY(GoodsRest); SLAPI GoodsRestFilt::GoodsRestFilt() : PPBaseFilt(PPFILT_GOODSREST, 0, 0)
{
	SetFlatChunk(offsetof(GoodsRestFilt, ReserveStart),
		offsetof(GoodsRestFilt, Reserve)-offsetof(GoodsRestFilt, ReserveStart)+sizeof(Reserve));
	SetBranchObjIdListFilt(offsetof(GoodsRestFilt, LocList));
	SetBranchObjIdListFilt(offsetof(GoodsRestFilt, GoodsList));
	Init(1, 0);
}

/*virtual*/int SLAPI GoodsRestFilt::Describe(long flags, SString & rBuf) const
{
	PutObjMembToBuf(PPOBJ_ARTICLE,     SupplID,    STRINGIZE(SupplID),    rBuf);
	PutObjMembToBuf(PPOBJ_GOODSGROUP,  GoodsGrpID, STRINGIZE(GoodsGrpID), rBuf);
	PutObjMembToBuf(PPOBJ_QUOTKIND,    QuotKindID, STRINGIZE(QuotKindID), rBuf);
	PutObjMembToBuf(PPOBJ_ARTICLE,     AgentID,    STRINGIZE(AgentID),    rBuf);
	PutObjMembToBuf(PPOBJ_BRAND,       BrandID,    STRINGIZE(BrandID),    rBuf);

	PutMembToBuf(&PrgnPeriod,          STRINGIZE(PrgnPeriod),    rBuf);
	PutMembToBuf(Date,                 STRINGIZE(Date),          rBuf);
	PutMembToBuf(DeficitDt,            STRINGIZE(DeficitDt),     rBuf);
	PutMembToBuf((long)ExhaustTerm,    STRINGIZE(ExhaustTerm),   rBuf);
	PutMembToBuf((long)AmtType,        STRINGIZE(AmtType),       rBuf);
	PutMembToBuf((long)(Flags2 & f2CalcPrognosis),  STRINGIZE(CalcPrognosis), rBuf); // @v9.5.8 CalcPrognosis-->Flags2
	{
		SString buf;
		switch(CalcMethod) {
			case GoodsRestParam::pcmAvg: buf = STRINGIZE(pcmAvg); break;
			case GoodsRestParam::pcmFirstLot: buf = STRINGIZE(pcmFirstLot); break;
			case GoodsRestParam::pcmLastLot: buf = STRINGIZE(pcmLastLot); break;
			case GoodsRestParam::pcmSum: buf = STRINGIZE(pcmSum); break;
			case GoodsRestParam::pcmDiff: buf = STRINGIZE(pcmDiff); break;
			case GoodsRestParam::pcmMostRecent: buf = STRINGIZE(pcmMostRecent); break;
		}
		PutMembToBuf(buf,  STRINGIZE(CalcMethod),  rBuf);
	}
	PutSggMembToBuf(Sgg, STRINGIZE(Sgg), rBuf);
	PutObjMembListToBuf(PPOBJ_GOODS,    &GoodsList, STRINGIZE(GoodsList), rBuf);
	PutObjMembListToBuf(PPOBJ_LOCATION, &LocList,   STRINGIZE(LocList),   rBuf);
	{
		long id = 0;
		StrAssocArray flag_list;
#define __FLG(f) if(Flags & f) flag_list.Add(id++, #f)
		__FLG(fBarCode);
		__FLG(fNullRest);
		__FLG(fCalcOrder);
		__FLG(fPriceByQuot);
		__FLG(fUnderMinStock);
		__FLG(fDisplayWoPacks);
		__FLG(fNullRestsOnly);
		__FLG(fLabelOnly);
		__FLG(fNoZeroOrderOnly);
		__FLG(fCalcTotalOnly);
		__FLG(fEachLocation);
		__FLG(fComplPackQtty);
		__FLG(fCWoVat);
		__FLG(fCalcDeficit);
		__FLG(fWoSupplier);
		__FLG(fShowMinStock);
		__FLG(fShowDraftReceipt);
		__FLG(fCalcSStatSales);
		__FLG(fOuterGsl);
		__FLG(fUseGoodsMatrix);
		__FLG(fExtByArCode);
		__FLG(fRestrictByArCode);
		__FLG(fCrosstab);
		__FLG(fShowGoodsMatrixBelongs);
#undef __FLG
		PutFlagsMembToBuf(&flag_list, STRINGIZE(Flags), rBuf);
	}
	return 1;
}

int SLAPI GoodsRestFilt::IsEqualExcept(const GoodsRestFilt & rS, long flags) const
{
#define NEQ_FLD(f) (f) != (rS.f)
	if(NEQ_FLD(CalcMethod))
		return 0;
	if(NEQ_FLD(Flags))
		return 0;
	if(NEQ_FLD(ExtViewFlags))
		return 0;
	if(NEQ_FLD(AmtType))
		return 0;
	if(NEQ_FLD(Flags2)) // @v9.5.8 CalcPrognosis-->Flags2
		return 0;
	if(NEQ_FLD(PrgnPeriod.low))
		return 0;
	if(NEQ_FLD(PrgnPeriod.upp))
		return 0;
	if(NEQ_FLD(Date))
		return 0;
	if(NEQ_FLD(SupplID))
		return 0;
	if(NEQ_FLD(GoodsGrpID))
		return 0;
	if(NEQ_FLD(QuotKindID))
		return 0;
	if(NEQ_FLD(AgentID))
		return 0;
	if(NEQ_FLD(BrandID))
		return 0;
	if(NEQ_FLD(Sgg))
		return 0;
	if(NEQ_FLD(DeficitDt))
		return 0;
	if(!LocList.IsEqual(rS.LocList))
		return 0;
	if(!GoodsList.IsEqual(rS.GoodsList))
		return 0;
	if(!(flags & eqxExhaustTerm))
		if(NEQ_FLD(ExhaustTerm))
			return 0;
#undef NEQ_FLD
	return 1;
}

int SLAPI GoodsRestFilt::SetQuotUsage(int v)
{
	int    ok = -1;
	const long preserve_flags = Flags;
	const long preserve_flags2 = Flags2;
	if(v == 0) {
		Flags &= ~fPriceByQuot;
		Flags2 &= ~f2CostByQuot;
	}
	else if(v == 1) {
		Flags |= fPriceByQuot;
		Flags2 &= ~f2CostByQuot;
	}
	else if(v == 2) {
		Flags &= ~fPriceByQuot;
		Flags2 |= f2CostByQuot;
	}
	else
		ok = 0;
	return !ok ? 0 : ((Flags == preserve_flags && Flags2 == preserve_flags2) ? -1 : +1);
}

int SLAPI GoodsRestFilt::GetQuotUsage() const
{
	int    result = 0;
	if(!(Flags & fPriceByQuot)) {
		if(Flags2 & f2CostByQuot)
			result = 2;
	}
	else {
		if(!(Flags2 & f2CostByQuot))
			result = 1;
	}
	return result;
}
//
//
//
SLAPI GoodsRestTotal::GoodsRestTotal()
{
	Init();
}

void SLAPI GoodsRestTotal::Init()
{
	Count = 0;
	PctAddedVal = 0.0;
	Quantity = PhQtty = Order = SumCost = SumPrice = 0.0;
	SumCVat = SumPVat = 0.0;
	DraftRcpt = SumDraftCost = SumDraftPrice = 0.0;
	Amounts.freeAll();
}

GoodsRestTotal & FASTCALL GoodsRestTotal::operator = (const GoodsRestTotal & src)
{
	Count         = src.Count;
	Quantity      = src.Quantity;
	PhQtty        = src.PhQtty;
	Order         = src.Order;
	SumCost       = src.SumCost;
	SumPrice      = src.SumPrice;
	SumCVat       = src.SumCVat;
	SumPVat       = src.SumPVat;
	PctAddedVal   = src.PctAddedVal;
	DraftRcpt     = src.DraftRcpt;
	SumDraftCost  = src.SumDraftCost;
	SumDraftPrice = src.SumDraftPrice;
	Amounts.copy(src.Amounts);
	return *this;
}

int GoodsRestTotal::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, Count,         rBuf));
	THROW_SL(pCtx->Serialize(dir, Quantity,      rBuf));
	THROW_SL(pCtx->Serialize(dir, PhQtty,        rBuf));
	THROW_SL(pCtx->Serialize(dir, Order,         rBuf));
	THROW_SL(pCtx->Serialize(dir, SumCost,       rBuf));
	THROW_SL(pCtx->Serialize(dir, SumPrice,      rBuf));
	THROW_SL(pCtx->Serialize(dir, SumCVat,       rBuf));
	THROW_SL(pCtx->Serialize(dir, SumPVat,       rBuf));
	THROW_SL(pCtx->Serialize(dir, PctAddedVal,   rBuf));
	THROW_SL(pCtx->Serialize(dir, DraftRcpt,     rBuf));
	THROW_SL(pCtx->Serialize(dir, SumDraftCost,  rBuf));
	THROW_SL(pCtx->Serialize(dir, SumDraftPrice, rBuf));
	THROW_SL(pCtx->Serialize(dir, &Amounts,      rBuf));
	CATCHZOK
	return ok;
}
//
//
//
#define DEFAULT_GROUPRESTCALCTHRESHOLD 100

SLAPI PPViewGoodsRest::PPViewGoodsRest() : PPView(0, &Filt, PPVIEW_GOODSREST), P_GGIter(0), P_Tbl(0), P_BObj(BillObj),
	P_Predictor(0), P_TempOrd(0), P_OpGrpngFilt(0), Flags(0), ScalePrefixID(0), SellOpID(0), LastCacheCounter(0),
	GroupCalcThreshold(CConfig.GRestCalcThreshold), P_Rpe(0)
{
	DefReportId = REPORT_GOODSREST;
	SETFLAG(Flags, fAccsCost, P_BObj->CheckRights(BILLRT_ACCSCOST));
	if(GroupCalcThreshold <= 0 || GroupCalcThreshold > 1000)
		GroupCalcThreshold = DEFAULT_GROUPRESTCALCTHRESHOLD;
	ImplementFlags |= implUseServer;
}

SLAPI PPViewGoodsRest::~PPViewGoodsRest()
{
	delete P_GGIter;
	delete P_Tbl;
	delete P_Predictor;
	delete P_TempOrd;
	delete P_Rpe;
	if(!(BaseState & bsServerInst))
		DBRemoveTempFiles();
	ZDELETE(P_OpGrpngFilt);
}

PPBaseFilt * PPViewGoodsRest::CreateFilt(void * extraPtr) const
{
	const PPConfig & r_cfg = LConfig;
	GoodsRestFilt * p_filt = new GoodsRestFilt;
	p_filt->LocList.Add(r_cfg.Location);
	if((reinterpret_cast<long>(extraPtr)) & 0x0001)
		p_filt->Flags2 |= GoodsRestFilt::f2CalcPrognosis; // // @v9.5.8 CalcPrognosis-->Flags2
	if(r_cfg.Flags & CFGFLG_USEGOODSMATRIX)
		p_filt->Flags |= GoodsRestFilt::fUseGoodsMatrix;
	PPAccessRestriction accsr;
	p_filt->GoodsGrpID = ObjRts.GetAccessRestriction(accsr).OnlyGoodsGrpID;
	return p_filt;
}

void SLAPI PPViewGoodsRest::SetGsl(const GoodsSubstList * pOuterGsl)
{
	Gsl.Init(1, pOuterGsl);
}
//
//
//
int SLAPI PPViewGoodsRest::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	GoodsRestFilt prev_filt = Filt;
	THROW(Helper_InitBaseFilt(pFilt) > 0);
	Filt.Date = Filt.Date.getactual(ZERODATE);
	Filt.DeficitDt = Filt.DeficitDt.getactual(ZERODATE);
	Filt.PrgnPeriod.Actualize(ZERODATE);
	if(Flags & fOnceInited && Flags & fExhaustTermInited && Filt.IsEqualExcept(prev_filt, GoodsRestFilt::eqxExhaustTerm)) {
		ok = 1;
	}
	else {
		PPUserFuncProfiler ufp(Filt.Date ? PPUPRF_VIEW_GREST_DT : PPUPRF_VIEW_GREST);
		Flags &= ~(fTotalInited | fExclAltFold | fScalePrefixAltGroup);
		ScalePrefixID = 0;
		SellOpID = CConfig.RetailOp;
		Total.Init();
		ZDELETE(P_Predictor);
		ZDELETE(P_Rpe); // @v10.3.2
		GoodsIDs.Clear();
		StrPool.ClearS();
		if(Filt.Flags & GoodsRestFilt::fNullRestsOnly)
			Filt.Flags |= GoodsRestFilt::fNullRest;
		if(Filt.Flags & GoodsRestFilt::fNoZeroOrderOnly)
			Filt.Flags |= GoodsRestFilt::fCalcOrder;
		// @v10.3.2 {
		if(Filt.Flags2 & GoodsRestFilt::f2RetailPrice && (Filt.GetQuotUsage() != 1 || !Filt.QuotKindID)) {
			const PPID loc_id = Filt.LocList.GetSingle();
			if(loc_id) {
				RetailPriceExtractor::ExtQuotBlock eqb(Filt.QuotKindID);
				long   rtlpf = 0;
				P_Rpe = new RetailPriceExtractor(loc_id, &eqb, 0, ZERODATETIME, rtlpf);
			}
		}
		// } @v10.3.2
		Goods2Tbl::Rec gg_rec;
		if(Filt.GoodsGrpID && GObj.Fetch(Filt.GoodsGrpID, &gg_rec) > 0) {
			if(gg_rec.Flags & GF_FOLDER && gg_rec.Flags & GF_EXCLALTFOLD)
				Flags |= fExclAltFold;
			else if(gg_rec.Flags & GF_ALTGROUP) {
				if(GObj.GetConfig().Flags & GCF_USESCALEBCPREFIX) {
					PPObjScale sc_obj;
					LAssocArray bcp_list;
					if(sc_obj.GetListWithBcPrefix(&bcp_list) > 0) {
						for(uint i = 0; i < bcp_list.getCount(); i++) {
							PPScalePacket sc_pack;
							if(sc_obj.Fetch(bcp_list.at(i).Key, &sc_pack) > 0 && sc_pack.Rec.AltGoodsGrp == Filt.GoodsGrpID) {
								Flags |= fScalePrefixAltGroup;
								ScalePrefixID = sc_pack.Rec.ID;
								break;
							}
						}
					}
				}
			}
		}
		if(Filt.LocList.GetCount()) {
			PPIDArray loc_list;
			PPObjLocation loc_obj;
			THROW(loc_obj.ResolveWarehouseList(&Filt.LocList.Get(), loc_list));
			LocList.Set(&loc_list);
		}
		else
			LocList.Set(0);
		if(!(Filt.Flags & GoodsRestFilt::fOuterGsl))
			Gsl.Init(1, 0);
		if(Filt.WaitMsgID)
			PPLoadText(Filt.WaitMsgID, WaitMsg);
		{
			double prf_measure = 0.0;
			PROFILE(ok = CreateTempTable(1, &prf_measure));
			ufp.SetFactor(0, prf_measure);
		}
		ufp.Commit();
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsRest::ViewLots(PPID __id, const BrwHdr * pHdr, int orderLots)
{
	int    ok = -1;
	GoodsRestViewItem item;
	if(pHdr || GetItem(__id, &item) > 0) {
		const PPID goods_id = pHdr ? pHdr->GoodsID : item.GoodsID;
		const PPID loc_id = pHdr ? pHdr->LocID : item.LocID;
		SString serial(pHdr ? 0 : item.Serial);
		serial.Strip();
		GoodsRestFilt filt = Filt;
		if(Filt.Sgg == sggLocation) {
			filt.Sgg = sggNone;
			filt.LocList.Set(0);
			filt.LocList.Add(goods_id & ~GOODSSUBSTMASK);
			ok = ViewGoodsRest(&filt, 0);
		}
		else if(Filt.Sgg && Gsl.GetGoodsBySubstID(goods_id, &filt.GoodsList) > 0) {
			if(goods_id || filt.GoodsList.GetCount()) {
				filt.Sgg = sggNone;
				if(Filt.Flags & GoodsRestFilt::fEachLocation) {
					filt.LocList.Set(0);
					filt.LocList.Add(loc_id);
				}
				if(Filt.Sgg == sggSuppl)
					filt.SupplID = (goods_id & ~GOODSSUBSTMASK);
				else if(Filt.Sgg == sggSupplAgent) {
					filt.AgentID = (goods_id & ~GOODSSUBSTMASK);
					if(!filt.AgentID)
						filt.Flags |= GoodsRestFilt::fZeroSupplAgent;
				}
				ok = ViewGoodsRest(&filt, 0);
			}
		}
		else if(goods_id) {
			LotFilt lot_flt;
			lot_flt.GoodsID = goods_id;
			PPID   temp_loc_id = 0;
			if(Filt.DiffParam & GoodsRestParam::_diffExpiry && item.Expiry) {
				lot_flt.ExpiryPrd.SetDate(item.Expiry);
			}
			if(orderLots) {
				lot_flt.Flags |= LotFilt::fOrders;
				if(Filt.Flags & GoodsRestFilt::fEachLocation)
					temp_loc_id = loc_id;
				lot_flt.ClosedTag = 1; // Opened only
			}
			else {
				temp_loc_id = (pHdr || item.Rest > 0.0) ? loc_id : 0;
				// @v10.6.11 {
				if(!LocList.GetSingle() && !(Filt.Flags & GoodsRestFilt::fEachLocation))
					temp_loc_id = 0;
				// } @v10.6.11 
				if(P_Ct && (!loc_id || (!LocList.GetSingle() && !(Filt.Flags & GoodsRestFilt::fEachLocation))))
					temp_loc_id = 0;
			}
			if(temp_loc_id)
				lot_flt.LocList.Add(temp_loc_id);
			else
				lot_flt.LocList = Filt.LocList;
			if(!(Filt.DiffLotTagID && Filt.DiffParam == GoodsRestParam::_diffLotTag)) // @v9.7.11
				lot_flt.PutExtssData(LotFilt::extssSerialText, serial);
			ok = ::ViewLots(&lot_flt, BIN(orderLots), 1);
		}
	}
	return ok;
}

#define GRP_LOC        1
#define GRP_GOODSFILT  2

class GoodsRestFiltDlg : public WLDialog {
	DECL_DIALOG_DATA(GoodsRestFilt);
public:
	GoodsRestFiltDlg() : WLDialog(DLG_GOODSREST, CTL_GOODSREST_WL)
	{
		addGroup(GRP_LOC, new LocationCtrlGroup(CTLSEL_GOODSREST_LOC, 0, 0, cmLocList, 0, LocationCtrlGroup::fEnableSelUpLevel, 0));
		addGroup(GRP_GOODSFILT, new GoodsFiltCtrlGroup(0, CTLSEL_GOODSREST_GGRP, cmGoodsFilt));
		SetupCalDate(CTLCAL_GOODSREST_DATE, CTL_GOODSREST_DATE);
		SetupCalPeriod(CTLCAL_GOODSREST_DRAFTPRD, CTL_GOODSREST_DRAFTPRD);
	}
	int    setDTS(const GoodsRestFilt *);
	int    getDTS(GoodsRestFilt *);
private:
	DECL_HANDLE_EVENT;
	int    editAdvOptions();
	void   SetupCtrls();
	int    SetupCrosstab();
};

int GoodsRestFiltDlg::SetupCrosstab()
{
	DisableClusterItem(CTL_GOODSREST_DIFFBYLOC, 1, !(Data.Flags & GoodsRestFilt::fEachLocation));
	if(!(Data.Flags & GoodsRestFilt::fEachLocation)) {
		Data.Flags &= ~GoodsRestFilt::fCrosstab;
		SetClusterData(CTL_GOODSREST_DIFFBYLOC, Data.Flags);
	}
	return 1;
}

int GoodsRestFiltDlg::setDTS(const GoodsRestFilt * pFilt)
{
	if(pFilt->Flags2 & GoodsRestFilt::f2CalcPrognosis) // @v9.5.8 CalcPrognosis-->Flags2
		Data.Init(1, 0);
	else
		Data = *pFilt;
	int    ok = 1;
	setCtrlData(CTL_GOODSREST_DATE, &Data.Date);
	LocationCtrlGroup::Rec loc_rec(&Data.LocList);
	setGroupData(GRP_LOC, &loc_rec);
	SetupArCombo(this, CTLSEL_GOODSREST_SUPPL, Data.SupplID, OLW_LOADDEFONOPEN, GetSupplAccSheet(), sacfDisableIfZeroSheet);
	GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, 0, 0, GoodsCtrlGroup::enableSelUpLevel);
	setGroupData(GRP_GOODSFILT, &gf_rec);
	SetupPPObjCombo(this, CTLSEL_GOODSREST_BRAND, PPOBJ_BRAND, Data.BrandID, OLW_LOADDEFONOPEN);
	SetupPPObjCombo(this, CTLSEL_GOODSREST_QUOTK, PPOBJ_QUOTKIND, ((Data.QuotKindID > 0) ? Data.QuotKindID : 0L), 0, reinterpret_cast<void *>(1));
	disableCtrl(CTLSEL_GOODSREST_QUOTK, Data.GetQuotUsage() ? 0 : 1);
	SetupSubstGoodsCombo(this, CTLSEL_GOODSREST_SUBST, Data.Sgg);
	AddClusterAssoc(CTL_GOODSREST_DIFFBYLOC, 0, GoodsRestFilt::fEachLocation);
	AddClusterAssoc(CTL_GOODSREST_DIFFBYLOC, 1, GoodsRestFilt::fCrosstab);
	SetClusterData(CTL_GOODSREST_DIFFBYLOC, Data.Flags);
	/* @v8.2.3
	//
	// При установленном флаге GoodsRestFilt::barCode (экспорт данных)
	// выбор остается только между первым и последним лотами, однако
	// если по-уму, то перед экспортом данных следует сделать унификацию цен.
	//
	if(Data.Flags & GoodsRestFilt::fBarCode)
		if(oneof2(Data.CalcMethod, 1, 2))
			v = Data.CalcMethod - 1;
		else
			v = (LConfig.RealizeOrder == RLZORD_FIFO) ? 0 : 1;
	else
		v = (Data.CalcMethod <= 2) ? Data.CalcMethod : 0;
	setCtrlData(CTL_GOODSREST_METHOD, &v);
	*/
	AddClusterAssocDef(CTL_GOODSREST_METHOD,  0, GoodsRestParam::pcmAvg);
	AddClusterAssoc(CTL_GOODSREST_METHOD,  1, GoodsRestParam::pcmFirstLot);
	AddClusterAssoc(CTL_GOODSREST_METHOD,  2, GoodsRestParam::pcmLastLot);
	SetClusterData(CTL_GOODSREST_METHOD, Data.CalcMethod);
	//
	// В диалоге флаги расчета сумм НДС в цп и цр устанавливаются (снимаются) одновременно
	//
	SETFLAG(Data.Flags, GoodsRestFilt::fCalcCVat, BIN(Data.Flags & (GoodsRestFilt::fCalcCVat|GoodsRestFilt::fCalcPVat)));
	SETFLAG(Data.Flags, GoodsRestFilt::fCalcPVat, BIN(Data.Flags & (GoodsRestFilt::fCalcCVat|GoodsRestFilt::fCalcPVat)));

	AddClusterAssoc(CTL_GOODSREST_FLAGS,  0, GoodsRestFilt::fNullRest);
	AddClusterAssoc(CTL_GOODSREST_FLAGS,  1, GoodsRestFilt::fNullRestsOnly);
	AddClusterAssoc(CTL_GOODSREST_FLAGS,  2, GoodsRestFilt::fNoZeroOrderOnly);
	AddClusterAssoc(CTL_GOODSREST_FLAGS,  3, GoodsRestFilt::fCalcOrder);
	AddClusterAssoc(CTL_GOODSREST_FLAGS,  4, GoodsRestFilt::fBarCode);
	AddClusterAssoc(CTL_GOODSREST_FLAGS,  5, GoodsRestFilt::fCWoVat);
	AddClusterAssoc(CTL_GOODSREST_FLAGS,  6, GoodsRestFilt::fCalcCVat);
	AddClusterAssoc(CTL_GOODSREST_FLAGS,  7, GoodsRestFilt::fUnderMinStock);
	AddClusterAssoc(CTL_GOODSREST_FLAGS,  8, GoodsRestFilt::fDisplayWoPacks);
	AddClusterAssoc(CTL_GOODSREST_FLAGS,  9, GoodsRestFilt::fShowMinStock);
	AddClusterAssoc(CTL_GOODSREST_FLAGS, 10, GoodsRestFilt::fShowDraftReceipt);
	AddClusterAssoc(CTL_GOODSREST_FLAGS, 11, GoodsRestFilt::fUseGoodsMatrix);
	AddClusterAssoc(CTL_GOODSREST_FLAGS, 12, GoodsRestFilt::fShowGoodsMatrixBelongs);
	SetClusterData(CTL_GOODSREST_FLAGS, Data.Flags);
	// @v9.5.8 AddClusterAssoc(CTL_GOODSREST_BYQUOT, 0, GoodsRestFilt::fPriceByQuot);
	// @v9.5.8 SetClusterData(CTL_GOODSREST_BYQUOT, Data.Flags);
	// @v9.5.8 {
	AddClusterAssocDef(CTL_GOODSREST_BYQUOT2, 0, 0);
	AddClusterAssoc(CTL_GOODSREST_BYQUOT2, 1, 1);
	AddClusterAssoc(CTL_GOODSREST_BYQUOT2, 2, 2);
	SetClusterData(CTL_GOODSREST_BYQUOT2, Data.GetQuotUsage());
	// } @v9.5.8
	// @v10.3.2 {
	AddClusterAssoc(CTL_GOODSREST_RTLPRICES, 0, GoodsRestFilt::f2RetailPrice);
	SetClusterData(CTL_GOODSREST_RTLPRICES, Data.Flags2);
	// } @v10.3.2
	AddClusterAssoc(CTL_GOODSREST_SPLIT, 0, GoodsRestParam::_diffCost);
	AddClusterAssoc(CTL_GOODSREST_SPLIT, 1, GoodsRestParam::_diffPrice);
	AddClusterAssoc(CTL_GOODSREST_SPLIT, 2, GoodsRestParam::_diffPack);
	AddClusterAssoc(CTL_GOODSREST_SPLIT, 3, GoodsRestParam::_diffExpiry);     // @v9.7.11
	AddClusterAssoc(CTL_GOODSREST_SPLIT, 4, GoodsRestParam::_diffSerial);
	AddClusterAssoc(CTL_GOODSREST_SPLIT, 5, GoodsRestParam::_diffLotTag);     // @v9.7.11
	SetClusterData(CTL_GOODSREST_SPLIT, Data.DiffParam);
	// @v9.7.11 {
	{
		ObjTagFilt lot_tag_filt(PPOBJ_LOT, ObjTagFilt::fOnlyTags);
		SetupObjTagCombo(this, CTLSEL_GOODSREST_LOTTAG, Data.DiffLotTagID, 0, &lot_tag_filt);
		disableCtrl(CTLSEL_GOODSREST_LOTTAG, Data.DiffParam != GoodsRestParam::_diffLotTag);
	}
	// } @v9.7.11
	setWL(BIN(Data.Flags & GoodsRestFilt::fLabelOnly));
	SetPeriodInput(this, CTL_GOODSREST_DRAFTPRD, &Data.DraftRcptPrd);
	SetupCtrls();
	SetupCrosstab();
	return ok;
}

int GoodsRestFiltDlg::getDTS(GoodsRestFilt * pFilt)
{
	int    ok = 1;
	uint   sel = 0;
	LocationCtrlGroup::Rec loc_rec;
	getGroupData(GRP_LOC, &loc_rec);
	Data.LocList = loc_rec.LocList;
	getCtrlData(sel = CTL_GOODSREST_DATE, &Data.Date);
	if(Data.Date) {
		DateRange temp;
		THROW_SL(checkdate(&Data.Date));
		temp.SetDate(Data.Date);
		THROW(AdjustPeriodToRights(temp, 1));
		Data.Date = temp.low;
	}
	getCtrlData(CTLSEL_GOODSREST_SUPPL, &Data.SupplID);
	{
		GoodsFiltCtrlGroup::Rec gf_rec;
		THROW(getGroupData(GRP_GOODSFILT, &gf_rec));
		Data.GoodsGrpID = gf_rec.GoodsGrpID;
	}
	getCtrlData(CTLSEL_GOODSREST_BRAND, &Data.BrandID);
	getCtrlData(CTLSEL_GOODSREST_QUOTK, &Data.QuotKindID);
	getCtrlData(CTLSEL_GOODSREST_SUBST, &Data.Sgg);
	GetClusterData(CTL_GOODSREST_DIFFBYLOC, &Data.Flags);
	Data.CalcMethod = GetClusterData(CTL_GOODSREST_METHOD);
	GetClusterData(CTL_GOODSREST_FLAGS,  &Data.Flags);
	//
	// В диалоге флаги расчета сумм НДС в цп и цр устанавливаются (снимаются) одновременно
	//
	SETFLAG(Data.Flags, GoodsRestFilt::fCalcCVat, BIN(Data.Flags & (GoodsRestFilt::fCalcCVat|GoodsRestFilt::fCalcPVat)));
	SETFLAG(Data.Flags, GoodsRestFilt::fCalcPVat, BIN(Data.Flags & (GoodsRestFilt::fCalcCVat|GoodsRestFilt::fCalcPVat)));
	// @v9.5.8 GetClusterData(CTL_GOODSREST_BYQUOT, &Data.Flags);
	// @v9.5.8 {
	{
		const long quot_usage = GetClusterData(CTL_GOODSREST_BYQUOT2);
		Data.SetQuotUsage(quot_usage);
	}
	// } @v9.5.8
	GetClusterData(CTL_GOODSREST_RTLPRICES, &Data.Flags2); // @v10.3.2
	if(Data.Flags & GoodsRestFilt::fNullRestsOnly)
		Data.Flags |= GoodsRestFilt::fNullRest;
	Data.DiffParam = GetClusterData(CTL_GOODSREST_SPLIT);
	if(Data.Flags & GoodsRestFilt::fCalcOrder) {
		Data.DiffParam &= ~(GoodsRestParam::_diffCost|GoodsRestParam::_diffPrice|GoodsRestParam::_diffPack|GoodsRestParam::_diffSerial|GoodsRestParam::_diffLotTag|GoodsRestParam::_diffExpiry);
	}
	// @v9.7.11 {
	if(Data.DiffParam == GoodsRestParam::_diffLotTag) {
		getCtrlData(CTLSEL_GOODSREST_LOTTAG, &Data.DiffLotTagID);
		THROW_PP(Data.DiffLotTagID, PPERR_LOTTAGNEEDED_DIFFPARAM);
	}
	// } @v9.7.11
	SETFLAG(Data.Flags, GoodsRestFilt::fLabelOnly, getWL());
	Data.Flags2 &= ~GoodsRestFilt::f2CalcPrognosis; // @v9.5.8 CalcPrognosis-->Flags2
	GetPeriodInput(this, CTL_GOODSREST_DRAFTPRD, &Data.DraftRcptPrd);
	ASSIGN_PTR(pFilt, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

void GoodsRestFiltDlg::SetupCtrls()
{
	int    disable_draft_rcpt = BIN(!DS.GetTLA().Cc.DraftRcptOp || (Data.Flags & GoodsRestFilt::fCalcDeficit));
	long   prev_flags = Data.Flags;
	disableCtrl(CTL_GOODSREST_SPLIT, Data.Flags & GoodsRestFilt::fCalcDeficit);
	GetClusterData(CTL_GOODSREST_FLAGS, &Data.Flags);
	if(disable_draft_rcpt)
		Data.Flags &= ~GoodsRestFilt::fShowDraftReceipt;
	if(Data.Flags & GoodsRestFilt::fShowDraftReceipt) {
		Data.Flags &= ~GoodsRestFilt::fCalcDeficit;
		if(!(prev_flags & GoodsRestFilt::fShowDraftReceipt)) {
			LDATE dt = ZERODATE;
			getCtrlData(CTL_GOODSREST_DATE, &dt);
			Data.DraftRcptPrd.Set(dt, ZERODATE);
			SetPeriodInput(this, CTL_GOODSREST_DRAFTPRD, &Data.DraftRcptPrd);
		}
	}
	else {
		Data.DraftRcptPrd.Z();
		SetPeriodInput(this, CTL_GOODSREST_DRAFTPRD, &Data.DraftRcptPrd);
	}
	disableCtrl(CTL_GOODSREST_DRAFTPRD, !BIN(Data.Flags & GoodsRestFilt::fShowDraftReceipt));
	::EnableWindow(::GetDlgItem(H(), CTLCAL_GOODSREST_DRAFTPRD), BIN(Data.Flags & GoodsRestFilt::fShowDraftReceipt));
	DisableClusterItem(CTL_GOODSREST_FLAGS, 10, disable_draft_rcpt);
	enableCommand(cmAdvOptions, !(Data.Flags & GoodsRestFilt::fShowDraftReceipt));
	SetClusterData(CTL_GOODSREST_FLAGS, Data.Flags);
}

IMPL_HANDLE_EVENT(GoodsRestFiltDlg)
{
	WLDialog::handleEvent(event);
	if(event.isClusterClk(CTL_GOODSREST_BYQUOT2)) {
		Data.SetQuotUsage(GetClusterData(CTL_GOODSREST_BYQUOT2));
		const long quot_usage = Data.GetQuotUsage();
		if(oneof2(quot_usage, 1, 2)) {
			if(Data.QuotKindID < 0)
				Data.QuotKindID = 0L;
		}
		else if(Data.QuotKindID >= 0)
			Data.QuotKindID = -1L;
		disableCtrl(CTLSEL_GOODSREST_QUOTK, (quot_usage ? 0 : 1));
		DisableClusterItem(CTL_GOODSREST_RTLPRICES, 0, quot_usage == 1);
	}
	else if(event.isClusterClk(CTL_GOODSREST_RTLPRICES)) {
		const long preserve_flags2 = Data.Flags2;
		GetClusterData(CTL_GOODSREST_RTLPRICES, &Data.Flags2);
		Data.SetQuotUsage(GetClusterData(CTL_GOODSREST_BYQUOT2));
		const long quot_usage = Data.GetQuotUsage();
		if(quot_usage == 1) {
			if(Data.Flags2 & Data.f2RetailPrice) {
				Data.Flags2 &= ~Data.f2RetailPrice;
				SetClusterData(CTL_GOODSREST_RTLPRICES, Data.Flags2);
			}
		}
		else
			DisableClusterItem(CTL_GOODSREST_BYQUOT2, 1, (Data.Flags2 & Data.f2RetailPrice));
	}
	else if(event.isClusterClk(CTL_GOODSREST_FLAGS))
		SetupCtrls();
	else if(event.isClusterClk(CTL_GOODSREST_DIFFBYLOC)) {
		GetClusterData(CTL_GOODSREST_DIFFBYLOC, &Data.Flags);
		SetupCrosstab();
	}
	else if(event.isClusterClk(CTL_GOODSREST_SPLIT)) {
		Data.DiffParam = GetClusterData(CTL_GOODSREST_SPLIT);
		disableCtrl(CTLSEL_GOODSREST_LOTTAG, Data.DiffParam != GoodsRestParam::_diffLotTag);
	}
	else if(event.isCmd(cmAdvOptions))
		editAdvOptions();
	else
		return;
	clearEvent(event);
}

int GoodsRestFiltDlg::editAdvOptions()
{
	int    ok = -1;
	ushort v = 0;
	TDialog * p_dlg = new TDialog(DLG_GRESTFLTA);
	if(CheckDialogPtrErr(&p_dlg)) {
		p_dlg->SetupCalDate(CTLCAL_GRESTFLTA_DEFICDT, CTL_GRESTFLTA_DEFICDT);
		SetupArCombo(p_dlg, CTLSEL_GRESTFLTA_AGENT, Data.AgentID, OLW_LOADDEFONOPEN|OLW_CANINSERT, GetAgentAccSheet(), sacfDisableIfZeroSheet);
		p_dlg->setCtrlData(CTL_GRESTFLTA_DEFICDT, &Data.DeficitDt);
		p_dlg->AddClusterAssoc(CTL_GRESTFLTA_CALCDEFIC, 0, GoodsRestFilt::fCalcDeficit);
		p_dlg->AddClusterAssoc(CTL_GRESTFLTA_CALCDEFIC, 1, GoodsRestFilt::fCalcUncompleteSess);
		p_dlg->SetClusterData(CTL_GRESTFLTA_CALCDEFIC, Data.Flags);
		p_dlg->setCtrlData(CTL_GRESTFLTA_PRGNDAYS, &Data.PrgnTerm);
		p_dlg->AddClusterAssoc(CTL_GRESTFLTA_EVF, 0, GoodsRestFilt::evfShowLastSaleDate);
		p_dlg->SetClusterData(CTL_GRESTFLTA_EVF, Data.ExtViewFlags);
		p_dlg->AddClusterAssoc(CTL_GRESTFLTA_UHTTEXPF, 0, GoodsRestFilt::uefRest);
		p_dlg->AddClusterAssoc(CTL_GRESTFLTA_UHTTEXPF, 1, GoodsRestFilt::uefPrice);
		p_dlg->AddClusterAssoc(CTL_GRESTFLTA_UHTTEXPF, 2, GoodsRestFilt::uefZeroAbsPrices);
		p_dlg->SetClusterData(CTL_GRESTFLTA_UHTTEXPF, Data.UhttExpFlags);
		SetupPPObjCombo(p_dlg, CTLSEL_GRESTFLTA_UHTTLOC, PPOBJ_LOCATION, Data.UhttExpLocID, 0, 0);
		while(ok < 0 && ExecView(p_dlg) == cmOK) {
			p_dlg->getCtrlData(CTLSEL_GRESTFLTA_AGENT,  &Data.AgentID);
			p_dlg->getCtrlData(CTL_GRESTFLTA_DEFICDT,   &Data.DeficitDt);
			p_dlg->GetClusterData(CTL_GRESTFLTA_CALCDEFIC, &Data.Flags);
			p_dlg->getCtrlData(CTL_GRESTFLTA_PRGNDAYS, &Data.PrgnTerm);
			Data.ExtViewFlags = static_cast<uint16>(p_dlg->GetClusterData(CTL_GRESTFLTA_EVF));
			Data.UhttExpFlags = static_cast<uint16>(p_dlg->GetClusterData(CTL_GRESTFLTA_UHTTEXPF));
			Data.UhttExpLocID = p_dlg->getCtrlLong(CTLSEL_GRESTFLTA_UHTTLOC);
			if(!checkdate(Data.DeficitDt, 1))
				PPErrorByDialog(p_dlg, CTL_GRESTFLTA_DEFICDT, PPERR_SLIB);
			else
				ok = 1;
		}
	}
	else
		ok = 0;
	delete p_dlg;
	if(Data.Flags & GoodsRestFilt::fCalcDeficit) {
		Data.DiffParam &= ~(GoodsRestParam::_diffCost|GoodsRestParam::_diffPrice|GoodsRestParam::_diffPack|GoodsRestParam::_diffSerial|GoodsRestParam::_diffLotTag|GoodsRestParam::_diffExpiry);
		SetClusterData(CTL_GOODSREST_SPLIT, Data.DiffParam);
	}
	SetupCtrls();
	return ok;
}
//
//
//
class GoodsRestWPrgnFltDlg : public TDialog {
public:
	GoodsRestWPrgnFltDlg() : TDialog(DLG_GRWPRGNFLT)
	{
		addGroup(GRP_GOODSFILT,  new GoodsFiltCtrlGroup(0, CTLSEL_GRWPRGNFLT_GRPID, cmGoodsFilt));
		addGroup(GRP_LOC, new LocationCtrlGroup(CTLSEL_GRWPRGNFLT_LOC, 0, 0, cmLocList, 0, LocationCtrlGroup::fEnableSelUpLevel, 0));
		SetupCalDate(CTLCAL_GRWPRGNFLT_DATE, CTL_GRWPRGNFLT_DATE);
		SetupCalPeriod(CTLCAL_GRWPRGNFLT_PRGNPRD, CTL_GRWPRGNFLT_PRGNPRD);
	}
	int    setDTS(const GoodsRestFilt *);
	int    getDTS(GoodsRestFilt *);
private:
	DECL_HANDLE_EVENT;
	GoodsRestFilt Filt;
};

IMPL_HANDLE_EVENT(GoodsRestWPrgnFltDlg)
{
	TDialog::handleEvent(event);
	if(event.isKeyDown(kbF2)) {
		LDATE  dt = getCtrlDate(CTL_GRWPRGNFLT_DATE);
		SETIFZ(dt, LConfig.OperDate);
		TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_GRWPRGNFLT_PRGNPRD));
		int    num_days = p_il ? atoi(p_il->getText()) : 0;
		if(num_days > 0 && dt) {
			DateRange period;
			period.Set(plusdate(dt, 1), plusdate(dt, num_days));
			SetPeriodInput(this, CTL_GRWPRGNFLT_PRGNPRD, &period);
		}
		clearEvent(event);
	}
}

int GoodsRestWPrgnFltDlg::setDTS(const GoodsRestFilt * pFilt)
{
	int    ok = 1;
	LDATE  last_update = ZERODATE;
	if(!RVALUEPTR(Filt, pFilt))
		Filt.Init(1, 0);
	disableCtrl(CTL_GRWPRGNFLT_FILLDATE, 1);
	PredictSalesCore psc;
	psc.GetTblUpdateDt(&last_update);
	setCtrlData(CTL_GRWPRGNFLT_FILLDATE, &last_update);
	SetPeriodInput(this, CTL_GRWPRGNFLT_PRGNPRD, &Filt.PrgnPeriod);
	setCtrlData(CTL_GRWPRGNFLT_DATE, &Filt.Date);
	GoodsFiltCtrlGroup::Rec gf_rec(Filt.GoodsGrpID, 0, 0, GoodsCtrlGroup::enableSelUpLevel);
	setGroupData(GRP_GOODSFILT, &gf_rec);
	LocationCtrlGroup::Rec loc_rec(&Filt.LocList);
	setGroupData(GRP_LOC, &loc_rec);
	SetupArCombo(this, CTLSEL_GRWPRGNFLT_SUPPL, Filt.SupplID, OLW_LOADDEFONOPEN, GetSupplAccSheet(), sacfDisableIfZeroSheet);
	AddClusterAssoc(CTL_GRWPRGNFLT_FLAGS, 0, GoodsRestFilt::fNullRest);
	AddClusterAssoc(CTL_GRWPRGNFLT_FLAGS, 1, GoodsRestFilt::fNoZeroOrderOnly);
	AddClusterAssoc(CTL_GRWPRGNFLT_FLAGS, 2, GoodsRestFilt::fCalcOrder);
	AddClusterAssoc(CTL_GRWPRGNFLT_FLAGS, 3, GoodsRestFilt::fUnderMinStock);
	AddClusterAssoc(CTL_GRWPRGNFLT_FLAGS, 4, GoodsRestFilt::fUseGoodsMatrix);
	SetClusterData(CTL_GRWPRGNFLT_FLAGS, Filt.Flags);
	setCtrlData(CTL_GRWPRGNFLT_EXHTERM, &Filt.ExhaustTerm);
	return ok;
}

int GoodsRestWPrgnFltDlg::getDTS(GoodsRestFilt * pFilt)
{
	int    ok = -1;
	uint   sel = 0;
	GoodsFiltCtrlGroup::Rec gf_rec;
	LocationCtrlGroup::Rec loc_rec;
	getCtrlData(sel = CTL_GRWPRGNFLT_DATE, &Filt.Date);
	THROW_SL(checkdate(Filt.Date, 1));
	if(Filt.Date) {
		DateRange temp;
		temp.SetDate(Filt.Date);
		THROW(AdjustPeriodToRights(temp, 1));
		Filt.Date = temp.low;
	}
	THROW(getGroupData(GRP_GOODSFILT, &gf_rec));
	Filt.GoodsGrpID = gf_rec.GoodsGrpID;
	getGroupData(GRP_LOC, &loc_rec);
	Filt.LocList = loc_rec.LocList;
	if(Filt.LocList.IsExists() && Filt.LocList.IsEmpty())
		Filt.LocList.Set(0);
	getCtrlData(CTLSEL_GRWPRGNFLT_SUPPL,  &Filt.SupplID);
	THROW(GetPeriodInput(this, sel = CTL_GRWPRGNFLT_PRGNPRD, &Filt.PrgnPeriod));
	THROW_PP(!Filt.PrgnPeriod.IsZero(), PPERR_PRGNPRDNEEDED);
	GetClusterData(CTL_GRWPRGNFLT_FLAGS, &Filt.Flags);
	getCtrlData(CTL_GRWPRGNFLT_EXHTERM, &Filt.ExhaustTerm);
	Filt.Flags2 |= GoodsRestFilt::f2CalcPrognosis; // @v9.5.8 CalcPrognosis-->Flags2
	ok = 1;
	ASSIGN_PTR(pFilt, Filt);
	CATCHZOKPPERRBYDLG
	return ok;
}

int SLAPI PPViewGoodsRest::EditBaseFilt(PPBaseFilt * pFilt)
{
	if(!Filt.IsA(pFilt))
		return 0;
	GoodsRestFilt * p_filt = static_cast<GoodsRestFilt *>(pFilt);
	if(p_filt->Flags2 & GoodsRestFilt::f2CalcPrognosis) { // @v9.5.8 CalcPrognosis-->Flags2
		DIALOG_PROC_BODY(GoodsRestWPrgnFltDlg, p_filt);
	}
	else {
		DIALOG_PROC_BODY(GoodsRestFiltDlg, p_filt);
	}
}
//
//
//
PPViewGoodsRest::CacheItem::CacheItem()
{
	THISZERO();
}

SLAPI PPViewGoodsRest::Cache::Cache()
{
}

PPViewGoodsRest::Cache & SLAPI PPViewGoodsRest::Cache::Clear()
{
	TSVector <PPViewGoodsRest::CacheItem>::freeAll(); // @v9.8.4 TSArray-->TSVector
	SStrGroup::DestroyS();
	return *this;
}

int SLAPI PPViewGoodsRest::Cache::SetupCacheItemSerial(PPViewGoodsRest::CacheItem & rItem, const char * pSerial)
	{ return AddS(pSerial, &rItem.SerialP); }
int SLAPI PPViewGoodsRest::Cache::GetCacheItemSerial(const PPViewGoodsRest::CacheItem & rItem, SString & rBuf) const
	{ return GetS(rItem.SerialP, rBuf); }
int SLAPI PPViewGoodsRest::Cache::SetupCacheItemLotTag(PPViewGoodsRest::CacheItem & rItem, const char * pTagVal)
	{ return AddS(pTagVal, &rItem.LotTagP); }
int SLAPI PPViewGoodsRest::Cache::GetCacheItemLotTag(const PPViewGoodsRest::CacheItem & rItem, SString & rBuf) const
	{ return GetS(rItem.LotTagP, rBuf); }

IMPL_CMPFUNC(GoodsRestCacheItem, _i1, _i2)
{
	RET_CMPCASCADE5(static_cast<const PPViewGoodsRest::CacheItem *>(_i1), static_cast<const PPViewGoodsRest::CacheItem *>(_i2), GoodsID, LocID, Cost, Price, UnitPerPack);
}

void SLAPI PPViewGoodsRest::InitCache()
{
	MaxCacheItems = 128*1024;
	CacheDelta = 4096;
	MEMSZERO(CacheStat);
	LastCacheCounter = 0;
	CacheBuf.Clear();
}

struct _lru_item_tag { // @flat
	uint   counter;
	uint   pos;
};

IMPL_CMPFUNC(_lru_item_tag, _i1, _i2)
{
	const _lru_item_tag * i1 = static_cast<const _lru_item_tag *>(_i1);
	const _lru_item_tag * i2 = static_cast<const _lru_item_tag *>(_i2);
	return CMPFUNC(int16, &i2->pos, &i1->pos); // descending order
}

double SLAPI PPViewGoodsRest::GetDeficit(PPID goodsID)
{
	double deficit = 0.0;
	if(Filt.Flags & GoodsRestFilt::fCalcDeficit)
		if(!ExclDeficitList.lsearch(goodsID) && DeficitList.Search(goodsID, &deficit, 0))
			ExclDeficitList.add(goodsID);
	return deficit;
}

double SLAPI PPViewGoodsRest::GetDraftRcptByLocList(PPID goodsID, const PPIDArray * pLocList, int addToExclList /*=1*/)
{
	double rest = 0.0;
	const  uint _c = SVectorBase::GetCount(pLocList);
	if(!_c)
		rest += GetDraftReceipt(goodsID, 0, addToExclList);
	else
		for(uint i = 0; i < _c; i++)
			rest += GetDraftReceipt(goodsID, pLocList->get(i), addToExclList);
	return rest;
}

double SLAPI PPViewGoodsRest::EnumDraftRcpt(PPID goodsID, uint * pIdx, DraftRcptItem * pItem)
{
	int    ok = -1;
	uint   pos = DEREFPTRORZ(pIdx);
	DraftRcptItem item;
	// @v10.8.0 @ctr MEMSZERO(item);
	for(; ok < 0 && DraftRcptList.lsearch(&goodsID, &pos, CMPF_LONG); pos++) {
		item = DraftRcptList.at(pos);
		if(ExclDraftRcptList.lsearch(&item, 0, PTR_CMPFUNC(_2long)) <= 0) {
			ExclDraftRcptList.Add(item.GoodsID, item.LocID, 0);
			ASSIGN_PTR(pIdx, pos);
			ASSIGN_PTR(pItem, item);
			ok = 1;
		}
	}
	return ok;
}

double SLAPI PPViewGoodsRest::GetDraftReceipt(PPID goodsID, PPID locID, int addToExclList /*=1*/)
{
	double rest = 0.0;
	if(goodsID && (Filt.Flags & GoodsRestFilt::fShowDraftReceipt)) {
		DraftRcptItem srch_item;
		// @v10.8.0 @ctr MEMSZERO(srch_item);
		srch_item.GoodsID = goodsID;
		srch_item.LocID   = locID;
		if(!ExclDraftRcptList.lsearch(&srch_item, 0, PTR_CMPFUNC(_2long))) {
			uint   pos = 0;
			if(DraftRcptList.bsearch(&srch_item, &pos, PTR_CMPFUNC(DraftRcptItem))) {
				rest = DraftRcptList.at(pos).Qtty;
				if(addToExclList)
					ExclDraftRcptList.Add(goodsID, locID, 0);
			}
		}
	}
	return rest;
}

double SLAPI PPViewGoodsRest::GetUncompleteSessQttyByLocList(PPID goodsID, const PPIDArray * pLocList, int addToExclList /*=1*/)
{
	double rest = 0.0;
	const  uint _c = SVectorBase::GetCount(pLocList);
	if(!_c)
		rest += GetUncompleteSessQtty(goodsID, 0, addToExclList);
	else
		for(uint i = 0; i < _c; i++)
			rest += GetUncompleteSessQtty(goodsID, pLocList->get(i), addToExclList);
	return rest;
}

double SLAPI PPViewGoodsRest::EnumUncompleteSessQtty(PPID goodsID, uint * pIdx, DraftRcptItem * pItem)
{
	int    ok = -1;
	uint   pos = DEREFPTRORZ(pIdx);
	DraftRcptItem item;
	// @v10.8.0 @ctr MEMSZERO(item);
	for(; ok < 0 && UncompleteSessQttyList.lsearch(&goodsID, &pos, PTR_CMPFUNC(long)) > 0; pos++) {
		item = DraftRcptList.at(pos);
		if(ExclUncompleteSessQttyList.lsearch(&item, 0, PTR_CMPFUNC(_2long)) <= 0) {
			ExclUncompleteSessQttyList.Add(item.GoodsID, item.LocID, 0);
			ASSIGN_PTR(pIdx, pos);
			ASSIGN_PTR(pItem, item);
			ok = 1;
		}
	}
	return ok;
}

double SLAPI PPViewGoodsRest::GetUncompleteSessQtty(PPID goodsID, PPID locID, int addToExclList /*=1*/)
{
	double rest = 0.0;
	if(goodsID && (Filt.Flags & GoodsRestFilt::fCalcUncompleteSess)) {
		DraftRcptItem srch_item;
		// @v10.8.0 @ctr MEMSZERO(srch_item);
		srch_item.GoodsID = goodsID;
		srch_item.LocID   = locID;
		if(!ExclUncompleteSessQttyList.lsearch(&srch_item, 0, PTR_CMPFUNC(_2long))) {
			uint   pos = 0;
			if(UncompleteSessQttyList.bsearch(&srch_item, &pos, PTR_CMPFUNC(DraftRcptItem))) {
				rest = UncompleteSessQttyList.at(pos).Qtty;
				if(addToExclList)
					ExclUncompleteSessQttyList.Add(goodsID, locID, 0);
			}
		}
	}
	return rest;
}

int SLAPI PPViewGoodsRest::FlashCacheItem(BExtInsert * bei, const PPViewGoodsRest::CacheItem & rItem)
{
	int    ok = 1;
	if(P_Tbl) {
		if(rItem.DBPos) {
			THROW_DB(P_Tbl->getDirectForUpdate(-1, 0, rItem.DBPos));
			TempGoodsRestTbl::Rec pattern;
			P_Tbl->copyBufTo(&pattern);
			TempGoodsRestTbl::Rec & r_rec = P_Tbl->data;
			if(!Filt.Sgg)
				P_Tbl->data.UnitPerPack = rItem.UnitPerPack;
			r_rec.Quantity    = rItem.Rest;
			r_rec.PhQtty      = rItem.PhRest;
			r_rec.Ord         = rItem.Order;
			r_rec.Cost        = (Flags & fAccsCost) ? rItem.Cost : 0.0;
			r_rec.Price       = rItem.Price;
			r_rec.DraftRcpt   = rItem.DraftRcpt;
			if(rItem.Deficit)
				r_rec.Deficit = rItem.Deficit;
			r_rec.QttyWithDeficit = r_rec.Quantity - r_rec.Deficit;
			r_rec.SumPrice    = rItem.Price * (r_rec.QttyWithDeficit + r_rec.DraftRcpt);
			r_rec.SumCost     = (Flags & fAccsCost) ? (rItem.Cost * (r_rec.QttyWithDeficit + r_rec.DraftRcpt)) : 0.0;
			/* @v9.8.2 if(Flags & fAccsCost && rItem.Cost)
				r_rec.PctAddedVal = (float)R5(100.0 * (rItem.Price - rItem.Cost) / rItem.Cost);
			else
				r_rec.PctAddedVal = 0.0f;*/
			r_rec.SumCVat     = (Flags & fAccsCost) ? (r_rec.SumCVat + rItem.SumCVat) : 0.0;
			r_rec.SumPVat     = (r_rec.SumPVat + rItem.SumPVat);
			if(memcmp(&r_rec, &pattern, sizeof(pattern))) {
				THROW_DB(P_Tbl->updateRec()); // @sfu
			}
			else
				CacheStat.NotChangedUpdateCount++;
			CacheStat.UpdateCount++;
		}
		else {
			const PPID goods_id = rItem.GoodsID;
			SString temp_buf;
			Goods2Tbl::Rec goods_rec;
			TempGoodsRestTbl::Rec rec;
			// @v10.6.4 MEMSZERO(rec);
			rec.GoodsID = goods_id;
			rec.LotID   = rItem.LotID;
			rec.LocID   = rItem.LocID;
			if(Filt.Sgg) {
				THROW(GObj.GetSubstText(goods_id, Filt.Sgg, &Gsl, temp_buf));
				STRNSCPY(rec.GoodsName, temp_buf);
			}
			else {
				if(GObj.Fetch(goods_id, &goods_rec) > 0) {
					STRNSCPY(rec.GoodsName, goods_rec.Name);
					if(Flags & fExclAltFold)
						GObj.P_Tbl->GetExclusiveAltParent(rec.GoodsID, Filt.GoodsGrpID, &rec.GoodsGrp);
					else
						rec.GoodsGrp = goods_rec.ParentID;
					rec.UnitID = goods_rec.UnitID;
					rec.PhUnitID = goods_rec.PhUnitID;
					if(Filt.Flags & GoodsRestFilt::fUnderMinStock || (!Filt.Sgg && Filt.Flags & GoodsRestFilt::fShowMinStock)) {
						GoodsStockExt gse;
						if(GObj.GetStockExt(goods_id, &gse, 1) > 0)
							rec.MinStock = gse.GetMinStock(rItem.LocID);
					}
					if(Filt.ExtViewFlags & GoodsRestFilt::evfShowLastSaleDate) {
						if(SellOpID) {
							LDATE   last_sell_date = ZERODATE;
							TransferTbl & r_t = *P_BObj->trfr;
							TransferTbl::Key3 k3;
							k3.GoodsID = goods_rec.ID;
							k3.Dt = MAXDATE;
							k3.OprNo = MAXLONG;
							if(r_t.search(3, &k3, spLt) && r_t.data.GoodsID == goods_id) do {
								BillTbl::Rec bill_rec;
								if(P_BObj->Fetch(r_t.data.BillID, &bill_rec) > 0 && bill_rec.OpID == SellOpID) {
									if(Filt.LocList.CheckID(r_t.data.LocID))
										last_sell_date = r_t.data.Dt;
								}
							} while(!last_sell_date && r_t.search(3, &k3, spLt) && r_t.data.GoodsID == goods_id);
							rec.LastSellDate = last_sell_date;
						}
					}
				}
				if(Flags & fScalePrefixAltGroup && GObj.GenerateScaleBarcode(rec.GoodsID, ScalePrefixID, temp_buf) > 0) {
					// @v9.8.3 temp_buf.CopyTo(rec.BarCode, sizeof(rec.BarCode));
					StrPool.AddS(temp_buf, &rec.BarcodeSP); // @v9.8.3
				}
				else {
					GObj.FetchSingleBarcode(rec.GoodsID, temp_buf.Z());
					// @v9.8.3 temp_buf.CopyTo(rec.BarCode, sizeof(rec.BarCode));
					StrPool.AddS(temp_buf, &rec.BarcodeSP); // @v9.8.3
				}
				rec.UnitPerPack = rItem.UnitPerPack;
				rec.Expiry = rItem.Expiry; // @v9.7.11
			}
			rec.Quantity    = rItem.Rest;
			rec.PhQtty      = rItem.PhRest;
			rec.Ord         = rItem.Order;
			rec.Cost        = (Flags & fAccsCost) ? rItem.Cost : 0.0;
			rec.Price       = rItem.Price;
			rec.Deficit     = rItem.Deficit;
			rec.DraftRcpt   = rItem.DraftRcpt;
			rec.SumPrice    = rItem.Price * (rItem.Rest + rItem.DraftRcpt - rItem.Deficit);
			rec.SumCost     = (Flags & fAccsCost) ? (rItem.Cost * (rItem.Rest + rItem.DraftRcpt - rItem.Deficit)) : 0.0;
			/* @v9.8.2 if(Flags & fAccsCost && rItem.Cost)
				rec.PctAddedVal = (float)((rItem.Price - rItem.Cost) / rItem.Cost * 100.0);
			else
				rec.PctAddedVal = 0.0f; */
			rec.SumCVat     = (Flags & fAccsCost) ? rItem.SumCVat : 0.0;
			rec.SumPVat     = rItem.SumPVat;
			// @v9.7.11 {
			if(Filt.DiffParam & GoodsRestParam::_diffLotTag && Filt.DiffLotTagID) {
				CacheBuf.GetCacheItemLotTag(rItem, temp_buf);
				// @v9.8.3 STRNSCPY(rec.Serial, temp_buf);
				StrPool.AddS(temp_buf, &rec.SerialSP); // @v9.8.3
			}
			else { // } @v9.7.11
				CacheBuf.GetCacheItemSerial(rItem, temp_buf);
				// @v9.8.3 STRNSCPY(rec.Serial, temp_buf);
				StrPool.AddS(temp_buf, &rec.SerialSP); // @v9.8.3
			}
			if((Filt.Flags2 & GoodsRestFilt::f2CalcPrognosis) || Filt.Flags & GoodsRestFilt::fCalcSStatSales || Filt.PrgnTerm > 0) { // @v9.5.8 CalcPrognosis-->Flags2
				int    can_trust = 0;
				double predict = 0.0;
				PredictSalesStat stat;
				Predictor::EvalParam ep;
				THROW_MEM(SETIFZ(P_Predictor, new Predictor));
				if(Filt.Flags2 & GoodsRestFilt::f2CalcPrognosis) { // @v9.5.8 CalcPrognosis-->Flags2
					char * p = 0;
					THROW(P_Predictor->Predict_(ep.Set(&LocList, rec.GoodsID, Filt.PrgnPeriod), &predict, &stat, &can_trust));
					rec.RestInDays = (long)roundnev(rec.Quantity * stat.GetTrnovr(PSSV_QTTY), 0);
					if(Filt.ExhaustTerm) {
						if(rec.Quantity <= 0.0) {
							LDATE  this_date = NZOR(Filt.Date, LConfig.OperDate);
							LDATE  last_date = ZERODATE;
							TransferTbl::Rec trfr_rec;
							LDATE before_dt = this_date;
							long  before_opn = 0;
							while(!last_date && P_BObj->trfr->GetLastOpByGoods(rec.GoodsID, before_dt, before_opn, &trfr_rec) > 0) {
								if(LocList.CheckID(trfr_rec.LocID)) {
									last_date = trfr_rec.Dt;
								}
								else {
									before_dt = trfr_rec.Dt;
									before_opn = trfr_rec.OprNo;
								}
							}
							rec.RestInDays = last_date ? diffdate(last_date, this_date) :  -1000;
						}
						Flags |= fExhaustTermInited;
					}
				}
				else if(Filt.PrgnTerm > 0) {
					DateRange prgn_period;
					prgn_period.low = plusdate(NZOR(Filt.Date, getcurdate_()), 1);
					prgn_period.upp = plusdate(prgn_period.low, Filt.PrgnTerm-1);
					THROW(P_Predictor->Predict_(ep.Set(&LocList, rec.GoodsID, prgn_period), &predict, &stat, &can_trust));
					rec.Predict = (long)roundnev(predict, 0);
				}
				else {
					THROW(P_Predictor->GetStat(rec.GoodsID, LocList, &stat));
				}
				rec.Predict    = (long)roundnev(predict, 0);
				rec.IsPredictTrust = BIN(can_trust);
				rec.SStatSales = stat.GetAverage(PSSV_QTTY);
			}
			rec.QttyWithDeficit = rec.Quantity - rec.Deficit;
			rec.SubstAsscCount  = Filt.Sgg ? Gsl.GetAssocCount(rec.GoodsID) : 0;
			if(bei) {
				THROW_DB(bei->insert(&rec));
			}
			else
				THROW_DB(P_Tbl->insertRecBuf(&rec));
			CacheStat.InsertCount++;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsRest::UpdateGoods(PPID goodsID)
{
	int    ok = 1;
	if(!Filt.Sgg) {
		Goods2Tbl::Rec goods_rec;
		SString bc_buf;
		PPID   group_id = 0;
		double min_stock = 0.0;
		if(GObj.Fetch(goodsID, &goods_rec) > 0) {
			if(Flags & fExclAltFold)
				GObj.P_Tbl->GetExclusiveAltParent(goodsID, Filt.GoodsGrpID, &group_id);
			else
				group_id = goods_rec.ParentID;
		}
		else
			MEMSZERO(goods_rec);
		if(Flags & fScalePrefixAltGroup && GObj.GenerateScaleBarcode(goodsID, ScalePrefixID, bc_buf) > 0) {
			;
		}
		else {
			GObj.FetchSingleBarcode(goodsID, bc_buf.Z());
		}
		{
			TempGoodsRestTbl::Key3 k3;
			MEMSZERO(k3);
			k3.GoodsID = goodsID;
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			if(P_Tbl->searchForUpdate(3, &k3, spGe) && P_Tbl->data.GoodsID == goodsID) do {
				TempGoodsRestTbl::Rec rec;
				P_Tbl->copyBufTo(&rec);
				STRNSCPY(rec.GoodsName, goods_rec.Name);
				rec.GoodsGrp = group_id;
				rec.UnitID = goods_rec.UnitID;
				rec.PhUnitID = goods_rec.PhUnitID;
				// @v9.8.3 bc_buf.CopyTo(rec.BarCode, sizeof(rec.BarCode));
				StrPool.AddS(bc_buf, &rec.BarcodeSP); // @v9.8.3
				if(Filt.Flags & GoodsRestFilt::fUnderMinStock || (!Filt.Sgg && Filt.Flags & GoodsRestFilt::fShowMinStock)) {
					GoodsStockExt gse;
					if(GObj.GetStockExt(goodsID, &gse, 1) > 0)
						rec.MinStock = gse.GetMinStock(rec.LocID);
				}
				THROW_DB(P_Tbl->updateRecBuf(&rec)); // @sfu
			} while(P_Tbl->searchForUpdate(3, &k3, spNext) && P_Tbl->data.GoodsID == goodsID);
			THROW(tra.Commit());
		}
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewGoodsRest::FlashCacheItems(uint count)
{
	int    ok = 1;
	uint   pos, i;
	if(P_Tbl) {
		SString wait_msg_buf;
		PPLoadText(PPTXT_WAIT_FLASHCACHEITEMS, wait_msg_buf);
		BExtInsert bei(P_Tbl);
		if(count) {
			PROFILE_START
			SVector lru_array(sizeof(_lru_item_tag)); // @v9.9.1 SArray-->SVector
			SVector lru_pos_array(sizeof(uint)); // @v9.9.1 SArray-->SVector
			const uint _c = CacheBuf.getCount();
			for(i = 0; i < _c; i++) {
				_lru_item_tag lru_item;
				lru_item.counter = CacheBuf.at(i).Counter;
				lru_item.pos = i;
				THROW_SL(lru_array.insert(&lru_item));
				PPWaitPercent(i+1, _c, wait_msg_buf);
			}
			lru_array.sort(PTR_CMPFUNC(uint));
			for(i = 0, count = MIN(count, lru_array.getCount()); i < count; i++) {
				pos = ((_lru_item_tag*)lru_array.at(i))->pos;
				THROW(FlashCacheItem(&bei, CacheBuf.at(pos)));
				THROW_SL(lru_pos_array.insert(&pos));
			}
			lru_array.freeAll();
			lru_pos_array.sort(PTR_CMPFUNC(int));
			for(int rev_i = lru_pos_array.getCount() - 1; rev_i >= 0; rev_i--)
				CacheBuf.atFree(*(uint *)lru_pos_array.at(rev_i));
			PROFILE_END
		}
		else {
			const uint _c = CacheBuf.getCount();
			for(i = 0; i < _c; i++) {
				THROW(FlashCacheItem(&bei, CacheBuf.at(i)));
				PPWaitPercent(i+1, _c, wait_msg_buf);
			}
			CacheBuf.Clear();
		}
		THROW(bei.flash());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsRest::CompareNthCacheItem(uint n, const PPViewGoodsRest::CacheItem * pItem) const
{
	int    ok = 0;
	if(n < CacheBuf.getCount()) {
		const CacheItem & r_nthitem = CacheBuf.at(n);
		if(r_nthitem.GoodsID == pItem->GoodsID)
			ok = (Filt.Flags & GoodsRestFilt::fEachLocation) ? BIN(r_nthitem.LocID == pItem->LocID) : 1;
	}
	return ok;
}

int FASTCALL PPViewGoodsRest::AddCacheItem(PPViewGoodsRest::CacheItem & rItem)
{
	int    ok = 1;
	PROFILE_START
	CacheItem * p_item = 0;
	TempGoodsRestTbl * p_tbl = P_Tbl;
	if(p_tbl) {
		uint   pos_ = 0, found = 0, i = 0;
		SString serial, serial2;
		SString tag_val, tag_val2;
		CacheStat.SearchCount++;
		CompFunc cf = (Filt.Flags & GoodsRestFilt::fEachLocation) ? PTR_CMPFUNC(_2long) : CMPF_LONG;
		CacheBuf.GetCacheItemSerial(rItem, serial);
		if(Filt.DiffParam & GoodsRestParam::_diffLotTag && Filt.DiffLotTagID)
			CacheBuf.GetCacheItemLotTag(rItem, tag_val);
		if(CacheBuf.bsearch(&rItem.GoodsID, &pos_, cf)) {
			for(i = pos_; i > 0 && CompareNthCacheItem(i-1, &rItem);)
				i--;
			while(!p_item && CompareNthCacheItem(i, &rItem)) {
				CacheItem & r_grci = CacheBuf.at(i);
				int   _eq = 1;
				if(Filt.DiffParam & GoodsRestParam::_diffCost && r_grci.Cost != rItem.Cost)
					_eq = 0;
				else if(Filt.DiffParam & GoodsRestParam::_diffPrice && r_grci.Price != rItem.Price)
					_eq = 0;
				else if(Filt.DiffParam & GoodsRestParam::_diffPack && r_grci.UnitPerPack != rItem.UnitPerPack)
					_eq = 0;
				else if(Filt.DiffParam & GoodsRestParam::_diffExpiry && r_grci.Expiry != rItem.Expiry)
					_eq = 0;
				else if(Filt.DiffParam & GoodsRestParam::_diffSerial) {
					CacheBuf.GetCacheItemSerial(r_grci, serial2);
					if(serial != serial2)
						_eq = 0;
				}
				else if(Filt.DiffParam & GoodsRestParam::_diffLotTag && Filt.DiffLotTagID) {
					CacheBuf.GetCacheItemLotTag(r_grci, tag_val2);
					if(tag_val != tag_val2)
						_eq = 0;
				}
				if(_eq)
					p_item = &r_grci;
				else
					i++;
			}
		}
		if(!p_item) {
			CacheStat.CacheMissesCount++;
			TempGoodsRestTbl::Key3 k3;
			k3.GoodsID = rItem.GoodsID;
			k3.LocID   = rItem.LocID;
			if(p_tbl->search(3, &k3, spEq)) do {
				if(Filt.DiffParam & GoodsRestParam::_diffCost && p_tbl->data.Cost != rItem.Cost)
					found = 0;
				else if(Filt.DiffParam & GoodsRestParam::_diffPrice && p_tbl->data.Price != rItem.Price)
					found = 0;
				else if(Filt.DiffParam & GoodsRestParam::_diffPack && p_tbl->data.UnitPerPack != rItem.UnitPerPack)
					found = 0;
				// @v9.7.11 {
				else if(Filt.DiffParam & GoodsRestParam::_diffExpiry && p_tbl->data.Expiry != rItem.Expiry)
					found = 0;
				// } @v9.7.11
				else {
					StrPool.GetS(p_tbl->data.SerialSP, serial2); // @v9.8.3
					if(Filt.DiffParam & GoodsRestParam::_diffSerial && serial != serial2/*p_tbl->data.Serial*/) // @v9.8.3 p_tbl->data.Serial-->serial2
						found = 0;
					else
						found = 1;
				}
			} while(!found && p_tbl->search(&k3, spNext) && k3.GoodsID == rItem.GoodsID && k3.LocID == rItem.LocID);
			if(found) {
				CacheItem new_item;
				DBRowId db_pos;
				THROW_DB(p_tbl->getPosition(&db_pos));
				new_item.GoodsID  = p_tbl->data.GoodsID;
				new_item.LocID    = p_tbl->data.LocID;
				new_item.LotID    = p_tbl->data.LotID;
				new_item.UnitPerPack = p_tbl->data.UnitPerPack;
				new_item.Order    = p_tbl->data.Ord;
				new_item.Rest     = p_tbl->data.Quantity;
				new_item.PhRest   = p_tbl->data.PhQtty;
				new_item.Cost     = (Flags & fAccsCost) ? p_tbl->data.Cost : 0.0;
				new_item.Price    = p_tbl->data.Price;
				new_item.SumCVat  = p_tbl->data.SumCVat;
				new_item.SumPVat  = p_tbl->data.SumPVat;
				new_item.SerialP  = rItem.SerialP;
				new_item.Expiry   = p_tbl->data.Expiry; // @v9.7.11
				new_item.DBPos    = db_pos;
				if(CacheBuf.getCount() >= MaxCacheItems)
					THROW(FlashCacheItems(CacheDelta));
				THROW_SL(CacheBuf.ordInsert(&new_item, &pos_, PTR_CMPFUNC(GoodsRestCacheItem)));
				p_item = &CacheBuf.at(pos_);
			}
			else
				CacheStat.DbMissesCount++;
		}
	}
	const int lru_counter = 0; // Переключатель для сравнительного тестирования производительности
	if(p_item) {
		double sum_rest   = p_item->Rest   + rItem.Rest;
		double sum_phrest = p_item->PhRest + rItem.PhRest;
		double sum_cost   = (Flags & fAccsCost) ? (rItem.Cost * rItem.Rest) : 0.0;
		double sum_price  = rItem.Price * rItem.Rest;
		p_item->UnitPerPack = rItem.UnitPerPack;
		p_item->Order      += rItem.Order;
		p_item->LotID     = rItem.LotID;
		if(Filt.CalcMethod == GoodsRestParam::pcmAvg && sum_rest != 0.0) {
			if(p_item->Cost != rItem.Cost)
				p_item->Cost = (Flags & fAccsCost) ? ((p_item->Cost * p_item->Rest + sum_cost) / sum_rest) : 0.0;
			if(p_item->Price != rItem.Price)
				p_item->Price = (p_item->Price * p_item->Rest + sum_price) / sum_rest;
		}
		p_item->SumCVat += rItem.SumCVat;
		p_item->SumPVat += rItem.SumPVat;
		p_item->Rest   = sum_rest;
		p_item->PhRest = sum_phrest;
		if(lru_counter)
			p_item->Counter = ++LastCacheCounter;
		else
			p_item->Counter++;
		p_item->Deficit   += rItem.Deficit;
		p_item->DraftRcpt += rItem.DraftRcpt;
	}
	else {
		rItem.DBPos.SetZero();
		if(lru_counter)
			rItem.Counter = ++LastCacheCounter;
		else
			rItem.Counter = 1;
		if(CacheBuf.getCount() >= MaxCacheItems)
			THROW(FlashCacheItems(CacheDelta));
		THROW_SL(CacheBuf.ordInsert(&rItem, 0, PTR_CMPFUNC(GoodsRestCacheItem)));
	}
	CATCHZOK
PROFILE_END
	return ok;
}

int SLAPI PPViewGoodsRest::AddGoodsThruCache(PPID goodsID, PPID locID, int isSubst,
	double order, double phUPerU, const GoodsRestVal * pGRV, BExtInsert * pBei)
{
	if(Filt.Flags & GoodsRestFilt::fUnderMinStock) {
		GoodsStockExt gse;
		if(GObj.GetStockExt(goodsID, &gse, 1) <= 0 || gse.GetMinStock(locID) <= MAX(0.0, pGRV->Rest))
			return 1;
	}
	CacheItem grci;
	grci.GoodsID = goodsID;
	grci.LocID   = locID;
	grci.LotID   = pGRV->LotID;
	grci.Order   = order;
	grci.UnitPerPack = pGRV->UnitsPerPack;
	grci.Rest     = pGRV->Rest;
	grci.PhRest   = pGRV->Rest * phUPerU;
	grci.Cost     = (Flags & fAccsCost) ? pGRV->Cost : 0L;
	grci.Price    = pGRV->Price;
	grci.Deficit  = pGRV->Deficit;
	grci.DraftRcpt = pGRV->DraftRcpt;
	CacheBuf.SetupCacheItemSerial(grci, pGRV->Serial);
	CacheBuf.SetupCacheItemLotTag(grci, pGRV->LotTagText); // @v9.7.11
	if(Filt.CalcMethod == GoodsRestParam::pcmMostRecent)
		::GetCurGoodsPrice(goodsID, locID, GPRET_MOSTRECENT, &grci.Price, 0);
	if(Filt.Flags & GoodsRestFilt::fCalcTotalOnly)
		return AddTotal(grci);
	else if(isSubst)
		return AddCacheItem(grci);
	else
		return FlashCacheItem(pBei, grci);
}

int SLAPI PPViewGoodsRest::GetLastLot_p(PPID goodsID, PPID locID, PPID supplID, LDATE dt, ReceiptTbl::Rec * pRec)
{
	int    ok = 0;
	PROFILE_START
	ReceiptCore * p_rt = & P_BObj->trfr->Rcpt;
	if(supplID) {
		ReceiptTbl::Key2 k;
		DBQ * dbq = 0;
		BExtQuery q(p_rt, 2, 1);
		q.selectAll();
		dbq = & (p_rt->GoodsID == goodsID);
		if(dt)
			dbq = & (*dbq && p_rt->Dt <= dt);
		dbq = ppcheckfiltid(dbq, p_rt->LocID,   locID);
		dbq = ppcheckfiltid(dbq, p_rt->SupplID, supplID);
		k.GoodsID = goodsID;
		k.Dt = NZOR(dt, MAXDATE);
		k.OprNo = MAXLONG;
		q.where(*dbq);
		q.initIteration(1, &k, spLe);
		if(q.nextIteration() > 0) {
			p_rt->copyBufTo(pRec);
			ok = 1;
		}
	}
	else {
		if(p_rt->GetLastLot(goodsID, locID, NZOR(dt, MAXDATE), pRec) > 0)
			ok = 1;
	}
	PROFILE_END
	return ok;
}

int SLAPI PPViewGoodsRest::GetLastLot_(PPID goodsID, PPID locID, ReceiptTbl::Rec & rRec)
{
	MEMSZERO(rRec);
	int    ok = GetLastLot_p(goodsID, locID, Filt.SupplID, LConfig.OperDate, &rRec);
	const  int quot_usage = (Filt.QuotKindID > 0) ? Filt.GetQuotUsage() : 0;
	if(oneof2(quot_usage, 1, 2)) {
		const QuotIdent qi(NZOR(locID, Filt.LocList.GetSingle()), Filt.QuotKindID);
		double qv = 0.0;
		if(GObj.GetQuotExt(goodsID, qi, rRec.Cost, rRec.Price, &qv, 1) > 0) {
			if(quot_usage == 1)
				rRec.Price = qv;
			else if(quot_usage == 2)
				rRec.Cost = qv;
			ok = 2;
		}
	}
	return ok;
}

int SLAPI PPViewGoodsRest::CalcOrder(PPID goodsID, GoodsRestParam * pOutData)
{
	int    ok = 1;
	GoodsRestParam p;
	p.Set(Filt, 0);
	p.CalcMethod = GoodsRestParam::pcmAvg;
	p.GoodsID = -labs(goodsID);
	p.LocID   = LocList.GetSingle();
	if(!LocList.IsEmpty())
		p.LocList = LocList.Get();
	p.SupplID = 0;
	if(P_BObj->trfr->GetRest(p)) {
		*pOutData = p;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPViewGoodsRest::AddTotal(const PPViewGoodsRest::CacheItem & rItem)
{
	int    ok = 1;
	PPID   amt_type_cost  = PPAMT_BUYING;
	PPID   amt_type_price = PPAMT_SELLING;
	double sum_cost  = rItem.Cost * (rItem.Rest - rItem.Deficit);
	double sum_price = rItem.Price * (rItem.Rest - rItem.Deficit);
	PPObjGoodsType gtobj;
	Goods2Tbl::Rec goods_rec;
	if(Filt.Flags & GoodsRestFilt::fCalcTotalOnly && GoodsIDs.Add(rItem.GoodsID) > 0)
		Total.Count++;
	Total.Quantity += (rItem.Rest + rItem.DraftRcpt);
	Total.Order    += rItem.Order;
	Total.PhQtty   += rItem.PhRest;
	Total.SumCost  += sum_cost;
	Total.SumPrice += sum_price;
	Total.SumCVat  += rItem.SumCVat;
	Total.SumPVat  += rItem.SumPVat;
	if(GObj.Fetch(rItem.GoodsID, &goods_rec) > 0) {
		PPGoodsType gt;
		if(!oneof2(goods_rec.GoodsTypeID, 0, PPGT_DEFAULT) && gtobj.Fetch(goods_rec.GoodsTypeID, &gt) > 0) {
			if(gt.AmtCost)
				amt_type_cost = gt.AmtCost;
			if(gt.AmtPrice)
				amt_type_price = gt.AmtPrice;
		}
	}
	if(Flags & fAccsCost)
		Total.Amounts.Add(amt_type_cost, 0L/*@curID*/, sum_cost, 0);
	Total.Amounts.Add(amt_type_price, 0L/*@curID*/, sum_price, 0);
	return ok;
}

int SLAPI PPViewGoodsRest::ProcessGoods(PPID goodsID, BExtInsert * pBei, const PPIDArray * pAgentBillList)
{
	int    ok = 1;
	const  long ff = Filt.Flags;
	const  int  each_loc   = BIN(ff & GoodsRestFilt::fEachLocation);
	const  int  draft_rcpt = BIN(ff & GoodsRestFilt::fShowDraftReceipt);
	const  int  calc_uncompl_sess = BIN(ff & GoodsRestFilt::fCalcUncompleteSess);
	const int quot_usage = (Filt.QuotKindID > 0) ? Filt.GetQuotUsage() : 0;
	PPID   goods_id = goodsID;
	int    is_subst = 0;
	double order = 0.0, ph_u_per_u = 0.0, min_stock = 0.0;
	GoodsRestParam p;
	GoodsRestParam ord_p;
	p.Set(Filt, P_Rpe);
	if(each_loc)
		p.DiffParam |= GoodsRestParam::_diffLoc;
	p.LocID  = LocList.GetSingle();
	if(!LocList.IsEmpty())
		p.LocList = LocList.Get();
	p.GoodsID = goods_id;
	THROW(PPCheckUserBreak());
	if(ff & (GoodsRestFilt::fNoZeroOrderOnly|GoodsRestFilt::fCalcOrder)) {
		ord_p.Set(Filt, 0);
		ord_p.Flags_ &= ~(GoodsRestParam::fCostByQuot|GoodsRestParam::fPriceByQuot|GoodsRestParam::fCWoVat|GoodsRestParam::fLabelOnly);
		ord_p.CalcMethod = GoodsRestParam::pcmAvg;
		ord_p.SupplID    = 0;
		ord_p.GoodsID    = -labs(goods_id);
		ord_p.LocID      = LocList.GetSingle();
		if(!LocList.IsEmpty())
			ord_p.LocList = LocList.Get();
		THROW(P_BObj->trfr->GetRest(ord_p));
		order = ord_p.Total.Rest;
		if(order == 0.0 && (ff & GoodsRestFilt::fNoZeroOrderOnly))
			return 1;
	}
	if(ff & GoodsRestFilt::fUnderMinStock) {
		GoodsStockExt gse;
		if(GObj.GetStockExt(goods_id, &gse, 1) <= 0 || (min_stock = gse.GetMaxMinStock(0)) <= 0)
			return 1;
	}
	p.QuotKindID = Filt.QuotKindID;
	p.AgentID    = Filt.AgentID;
	p.P_SupplAgentBillList = pAgentBillList;
	if(Filt.Sgg == sggLocation)
		p.DiffParam |= GoodsRestParam::_diffLoc;
	THROW(P_BObj->trfr->GetRest(p));
	p.Total.Rest -= GetUncompleteSessQttyByLocList(p.GoodsID, &p.LocList, 0);
	if(draft_rcpt && p.Total.Rest == 0.0)
		p.Total.DraftRcpt = GetDraftRcptByLocList(goods_id, &p.LocList, 0);
	if(order || p.Total.Rest || min_stock > 0.0 || p.Total.DraftRcpt || (ff & GoodsRestFilt::fNullRest)) {
		//
		// Пассивный товар с нулевым остатком не учитываем
		//
		if(order == 0.0 && p.Total.Rest == 0.0 && min_stock <= 0.0 && GObj.CheckFlag(goods_id, GF_PASSIV))
			return 1;
		double deficit = GetDeficit(goods_id);
		ReceiptTbl::Rec rrec;
		GObj.GetPhUPerU(goods_id, 0, &ph_u_per_u);
		if(p.Total.Rest == 0.0 && p.Total.DraftRcpt == 0.0) {
			// В цикле для каждого склада Х
			const  PPID temp_loc_id = LocList.GetSingle();
			const  int llr = GetLastLot_(goodsID, temp_loc_id, rrec);
			if(llr > 0 || (ff & (GoodsRestFilt::fUseGoodsMatrix|GoodsRestFilt::fForceNullRest))) {
				p.Total.Init(&rrec);
				p.Total.Deficit = deficit;
				if(Filt.Sgg) {
					PPObjGoods::SubstBlock sgg_blk;
					sgg_blk.ExclParentID = Filt.GoodsGrpID;
					if(llr > 0) {
						sgg_blk.LocID = rrec.LocID;
						sgg_blk.P_LotRec = &rrec;
					}
					THROW(GObj.SubstGoods(goodsID, &goods_id, Filt.Sgg, &sgg_blk, &Gsl));
					is_subst = 1;
				}
				THROW(AddGoodsThruCache(goods_id, LocList.GetSingle(), is_subst, order, ph_u_per_u, &p.Total, pBei));
			}
			// }
		}
		else if(!(ff & GoodsRestFilt::fNullRestsOnly)) {
			if(p.getCount()) {
				GoodsRestVal * p_val = 0;
				PPIDArray ord_loc_list;
				uint   i;
				if(each_loc && (order != 0.0 || draft_rcpt)) {
					for(i = 0; DraftRcptList.lsearch(&goodsID, &i, CMPF_LONG); i++) {
						const PPID loc_id = DraftRcptList.at(i).LocID;
						if(loc_id)
							ord_loc_list.add(loc_id);
					}
					for(i = 0; i < ord_p.getCount(); i++)
						ord_loc_list.add(ord_p.at(i).LocID);
					ord_loc_list.sortAndUndup();
				}
				for(uint j = 0; p.enumItems(&j, reinterpret_cast<void **>(&p_val));) {
					PPID   loc_id = 0;
					double temp_order = order;
					if(each_loc) {
						loc_id = p_val->LocID;
						temp_order = ord_p.GetRestByLoc(loc_id);
						ord_loc_list.freeByKey(loc_id, 1 /* binary */);
					}
					else
						loc_id = LocList.GetSingle();
					p_val->Deficit   = deficit;
					p_val->DraftRcpt = GetDraftReceipt(goodsID, loc_id);
					p_val->Rest -= GetUncompleteSessQtty(goodsID, loc_id);
					if(Filt.Sgg) {
						PPObjGoods::SubstBlock sgg_blk;
						sgg_blk.ExclParentID = Filt.GoodsGrpID;
						sgg_blk.LocID = p_val->LocID;
						sgg_blk.LotID = p_val->LotID;
						THROW(GObj.SubstGoods(goodsID, &goods_id, Filt.Sgg, &sgg_blk, &Gsl));
						is_subst = 1;
					}
					THROW(AddGoodsThruCache(goods_id, loc_id, is_subst, temp_order, ph_u_per_u, p_val, pBei));
				}
				if(each_loc && (order != 0.0 || draft_rcpt)) {
					for(i = 0; i < ord_loc_list.getCount(); i++) {
						const PPID loc_id = ord_loc_list.get(i);
						GoodsRestVal temp_val;
						temp_val.LocID = loc_id;
						temp_val.DraftRcpt = GetDraftReceipt(goodsID, loc_id);
						if(Filt.Sgg) {
							PPObjGoods::SubstBlock sgg_blk;
							sgg_blk.ExclParentID = Filt.GoodsGrpID;
							sgg_blk.LocID = loc_id;
							sgg_blk.LotID = p_val->LotID;
							THROW(GObj.SubstGoods(goodsID, &goods_id, Filt.Sgg, &sgg_blk, &Gsl));
							is_subst = 1;
						}
						THROW(AddGoodsThruCache(goods_id, loc_id, is_subst, ord_p.GetRestByLoc(loc_id), ph_u_per_u, &temp_val, pBei));
					}
				}
			}
			else if((draft_rcpt || calc_uncompl_sess) && each_loc) {
				DraftRcptItem item;
				// @v10.8.0 @ctr MEMSZERO(item);
				if(draft_rcpt) {
					for(uint pos = 0; EnumDraftRcpt(goods_id, &pos, &item) > 0;) {
						if(item.LocID != 0) {
							p.Total.DraftRcpt = item.Qtty;
							const int llr = GetLastLot_(goodsID, item.LocID, rrec);
							p.Total.Cost  = rrec.Cost;
							p.Total.Price = rrec.Price;
							if(Filt.Sgg) {
								PPObjGoods::SubstBlock sgg_blk;
								sgg_blk.ExclParentID = Filt.GoodsGrpID;
								sgg_blk.LocID = item.LocID;
								if(llr > 0)
									sgg_blk.P_LotRec = &rrec;
								// Здесь не следует точно идентифицировать лот в sgg_blk по скольку фактически лота нет (rrec -
								// просто последний лот).
								THROW(GObj.SubstGoods(goodsID, &goods_id, Filt.Sgg, &sgg_blk, &Gsl));
								is_subst = 1;
							}
							THROW(AddGoodsThruCache(goods_id, item.LocID, is_subst, order, ph_u_per_u, &p.Total, pBei));
						}
					}
				}
				if(calc_uncompl_sess) {
					for(uint pos = 0; EnumUncompleteSessQtty(goods_id, &pos, &item) > 0;) {
						if(item.LocID != 0) {
							p.Total.Rest -= item.Qtty;
							int    llr = GetLastLot_(goodsID, item.LocID, rrec);
							p.Total.Cost  = rrec.Cost;
							p.Total.Price = rrec.Price;
							if(Filt.Sgg) {
								PPObjGoods::SubstBlock sgg_blk;
								sgg_blk.ExclParentID = Filt.GoodsGrpID;
								sgg_blk.LocID = item.LocID;
								if(llr > 0)
									sgg_blk.P_LotRec = &rrec;
								// Здесь не следует точно идентифицировать лот в sgg_blk по скольку фактически лота нет (rrec -
								// просто последний лот).
								THROW(GObj.SubstGoods(goodsID, &goods_id, Filt.Sgg, &sgg_blk, &Gsl));
								is_subst = 1;
							}
							THROW(AddGoodsThruCache(goods_id, item.LocID, is_subst, order, ph_u_per_u, &p.Total, pBei));
						}
					}
				}
			}
			else {
				PPObjGoods::SubstBlock sgg_blk;
				sgg_blk.ExclParentID = Filt.GoodsGrpID;
				sgg_blk.LocID = p.Total.LocID;
				p.Total.Deficit   = deficit;
				p.Total.DraftRcpt = GetDraftReceipt(goods_id, p.LocID);
				if(p.Total.DraftRcpt > 0.0 && p.Total.Rest == 0.0) {
					if(GetLastLot_(goodsID, p.LocID, rrec) > 0) {
						p.Total.Cost  = rrec.Cost;
						p.Total.Price = rrec.Price;
						sgg_blk.P_LotRec = &rrec;
					}
				}
				if(Filt.Sgg) {
					THROW(GObj.SubstGoods(goodsID, &goods_id, Filt.Sgg, &sgg_blk, &Gsl));
					is_subst = 1;
				}
				THROW(AddGoodsThruCache(goods_id, p.LocID, is_subst, order, ph_u_per_u, &p.Total, pBei));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsRest::ProcessGroup(const PPIDArray * pGrpGoodsList)
{
	int    ok = 1;
	PPIDArray * p_agent_bill_list = 0;
	BExtInsert * p_bei = 0;
	PPIDArray goods_list;
	const PPIDArray * p_goods_list = 0;
	const uint32 preserve_filt_diff_param = Filt.DiffParam;
	if(Filt.Sgg == sggCVatRate)
        Filt.DiffParam |= GoodsRestParam::_diffLotID;
	if(pGrpGoodsList)
		p_goods_list = pGrpGoodsList;
	else {
		GoodsFilt goods_flt;
		Goods2Tbl::Rec grec;
		if(Filt.SupplID || Filt.GoodsGrpID || Filt.BrandID) {
			goods_flt.GrpID   = Filt.GoodsGrpID;
			goods_flt.SupplID = Filt.SupplID;
			goods_flt.BrandList.Add(Filt.BrandID);
		}
		p_goods_list = &goods_list;
		for(GoodsIterator giter(&goods_flt, 0); giter.Next(&grec) > 0;)
			if(!(grec.Flags & GF_GENERIC))
				THROW_SL(goods_list.add(grec.ID));
	}
	if(p_goods_list) {
		const uint gc = p_goods_list->getCount();
		if(gc) {
			if(P_Tbl)
				THROW_MEM(p_bei = new BExtInsert(P_Tbl));
			if(Filt.AgentID && !(Filt.Flags & GoodsRestFilt::fZeroSupplAgent)) {
				THROW_MEM(p_agent_bill_list = new PPIDArray);
				THROW(P_BObj->P_Tbl->GetBillListByExt(Filt.AgentID, 0L, *p_agent_bill_list));
			}
			for(uint i = 0; i < gc; i++) {
				const PPID goods_id = p_goods_list->get(i);
				THROW(PPCheckUserBreak());
				if(Filt.GoodsGrpID || !GObj.IsAsset(goods_id))
					THROW(ProcessGoods(goods_id, p_bei, p_agent_bill_list));
				PPWaitPercent(i+1, gc, WaitMsg);
			}
			if(p_bei) {
				THROW_DB(p_bei->flash());
				ZDELETE(p_bei);
			}
		}
	}
	CATCH
		PPSaveErrContext();
		ZDELETE(p_bei);
		PPRestoreErrContext();
		ok = 0;
	ENDCATCH
	delete p_agent_bill_list;
	delete p_bei;
	Filt.DiffParam = preserve_filt_diff_param;
	return ok;
}

PPViewGoodsRest::ProcessLotBlock::ProcessLotBlock(int forceUseLotRest) : ForceUseLotRest(BIN(forceUseLotRest)), P_GrpGoodsList(0), P_LpCache(0)
{
}

PPViewGoodsRest::ProcessLotBlock::~ProcessLotBlock()
{
	delete P_LpCache;
}

int SLAPI PPViewGoodsRest::InitProcessLotBlock(ProcessLotBlock & rBlk, const PPIDArray * pGrpGoodsList)
{
	int    ok = 1;
	rBlk.ExtBillList.freeAll();
	rBlk.GrpGoodsList.freeAll();
	ZDELETE(rBlk.P_LpCache);
	if(!(Filt.Flags & GoodsRestFilt::fZeroSupplAgent))
		THROW(P_BObj->P_Tbl->GetBillListByExt(Filt.AgentID, 0L, rBlk.ExtBillList));
	// @v8.1.0 (сортировку теперь выполняет GetBillListByExt) rBlk.ExtBillList.sort();
	if(Filt.Date) {
		THROW_MEM(rBlk.P_LpCache = new Transfer::GetLotPricesCache(Filt.Date, &LocList.Get()));
	}
	rBlk.P_GrpGoodsList = pGrpGoodsList;
	if(Filt.GoodsGrpID && !rBlk.P_GrpGoodsList) {
		GoodsIterator::GetListByGroup(Filt.GoodsGrpID, &rBlk.GrpGoodsList);
		rBlk.GrpGoodsList.sort();
		rBlk.P_GrpGoodsList = &rBlk.GrpGoodsList;
	}
	rBlk.Cntr.Init((ulong)0);
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsRest::Helper_ProcessLot(ProcessLotBlock & rBlk, ReceiptTbl::Rec & rRec)
{
	int    ok = 1;
	const  PPID goods_id = rRec.GoodsID;
	int    vat_free = -1;
	PPID   tax_grp_id = 0;
	PPID   org_lot_bill_id = 0;
	LDATE  org_lot_date = ZERODATE;
	double tax_factor = 1.0;
	double ph_u_per_u = 0.0;
	double rest = 0.0;
	Goods2Tbl::Rec goods_rec;
	if(rBlk.Cntr.GetTotal()) {
		PPWaitPercent(rBlk.Cntr.Increment());
	}
	else {
		PPWaitDate(rRec.Dt);
	}
	if(goods_id < 0 && (rRec.Closed || rRec.Flags & LOTF_CLOSEDORDER))
		ok = -1;
	else if(LocList.GetCount() && !LocList.Search(rRec.LocID, 0))
		ok = -1;
	else if(!ObjRts.CheckLocID(rRec.LocID, 0))
		ok = -1;
	else if(rBlk.P_GrpGoodsList && !rBlk.P_GrpGoodsList->bsearch(labs(goods_id)))
		ok = -1;
	else if(Filt.Flags & GoodsRestFilt::fLabelOnly) {
		if(!P_BObj->P_Tbl->HasWLabel(rRec.BillID))
			ok = -1;
	}
	else if(!Filt.GoodsGrpID && GObj.IsAsset(labs(goods_id)) > 0)
		ok = -1;
	else if(goods_id > 0 && (Filt.AgentID || Filt.Flags & GoodsRestFilt::fZeroSupplAgent)) { // Агента поставщика не проверяем для лотов заказов
		THROW(P_BObj->trfr->Rcpt.GetOriginDate(&rRec, &org_lot_date, &org_lot_bill_id));
		if(Filt.Flags & GoodsRestFilt::fZeroSupplAgent) {
			PPBillExt b_ext;
			if(P_BObj->FetchExt(org_lot_bill_id, &b_ext) > 0 && b_ext.AgentID)
				ok = -1;
		}
		else if(!rBlk.ExtBillList.bsearch(org_lot_bill_id))
			ok = -1;
	}
	if(ok > 0 && Filt.BrandID) {
		// @v10.6.4 Goods2Tbl::Rec goods_rec;
		if(GObj.Fetch(labs(goods_id), &goods_rec) <= 0 || goods_rec.BrandID != Filt.BrandID)
			ok = -1;
	}
	if(ok > 0) {
		CacheItem grci;
		PROFILE_START
		if(!Filt.Date || rBlk.ForceUseLotRest) // Current rest
			rest = rRec.Rest;
		else {
			THROW(P_BObj->trfr->GetRest(rRec.ID, Filt.Date, &rest));
		}
		PROFILE_END
		rest -= GetUncompleteSessQtty(goods_id, rRec.LocID, 1);
		if(rest != 0.0 || (Filt.Flags & GoodsRestFilt::fNullRest && goods_id > 0)) {
			if(Filt.Date) {
				PROFILE_START
				if(rBlk.P_LpCache)
					rBlk.P_LpCache->Get(&rRec);
				else
					THROW(P_BObj->trfr->GetLotPrices(&rRec, Filt.Date));
				PROFILE_END
			}
			grci.GoodsID = goods_id;
			grci.LotID   = rRec.ID;
			grci.LocID   = rRec.LocID;
			grci.Expiry  = rRec.Expiry; // @v9.7.11
			if(!Filt.Sgg)
				grci.UnitPerPack = rRec.UnitPerPack;
			grci.Cost  = (Flags & fAccsCost) ? R5(rRec.Cost) : 0.0;
			grci.Price = R5(rRec.Price);
			if(Filt.CalcMethod == GoodsRestParam::pcmMostRecent)
				::GetCurGoodsPrice(goods_id, rRec.LocID, GPRET_MOSTRECENT, &grci.Price, 0);
			const int quot_usage = (Filt.QuotKindID > 0) ? Filt.GetQuotUsage() : 0;
			if((rRec.Flags & (LOTF_COSTWOVAT|LOTF_PRICEWOTAXES)) || oneof2(quot_usage, 1, 2) || P_Rpe) { // @v10.3.2 P_Rpe
				// @v10.3.2 {
				if(P_Rpe) {
					assert(Filt.Flags2 & Filt.f2RetailPrice);
					assert(quot_usage != 1);
					RetailExtrItem  rtl_ext_item;
					if(P_Rpe->GetPrice(rRec.GoodsID, 0, 0.0, &rtl_ext_item))
						grci.Price = rtl_ext_item.Price;
					else
						grci.Price = 0.0; // Явно сигнализируем о том, что цены нет
				}
				// } @v10.3.2
				if(oneof2(quot_usage, 1, 2)) {
					double qv = 0.0;
					const QuotIdent qi(rRec.LocID, Filt.QuotKindID);
					if(GObj.GetQuotExt(rRec.GoodsID, qi, grci.Cost, grci.Price, &qv, 1) > 0) {
						if(quot_usage == 2)
							grci.Cost = qv;
						else if(quot_usage == 1 && !P_Rpe) // @v10.3.2 !P_Rpe
							grci.Price = qv;
					}
				}
				if(rRec.Flags & (LOTF_COSTWOVAT|LOTF_PRICEWOTAXES)) {
					tax_grp_id = 0;
					MEMSZERO(goods_rec);
					if(GObj.Fetch(rRec.GoodsID, &goods_rec) > 0)
						tax_grp_id = goods_rec.TaxGrpID;
					GObj.MultTaxFactor(rRec.GoodsID, &tax_factor);
					if(rRec.Flags & LOTF_COSTWOVAT) {
						if(vat_free < 0)
							vat_free = IsLotVATFree(rRec);
						if(!org_lot_date)
							THROW(P_BObj->trfr->Rcpt.GetOriginDate(&rRec, &org_lot_date));
						GObj.AdjCostToVat(rRec.InTaxGrpID, tax_grp_id, org_lot_date, tax_factor, &grci.Cost, 1, vat_free);
					}
					if(rRec.Flags & LOTF_PRICEWOTAXES)
						GObj.AdjPriceToTaxes(tax_grp_id, tax_factor, &grci.Price, 1);
				}
			}
			if(grci.GoodsID > 0)
				grci.Rest = R6(rest);
			else
				grci.Order = R6(rest);
			if(Filt.Flags & (GoodsRestFilt::fCWoVat|GoodsRestFilt::fCalcCVat|GoodsRestFilt::fCalcPVat)) {
				double _cost_wo_vat = grci.Cost;
				double _price_wo_vat = grci.Price;
				double _cvat = 0.0;
				double _pvat = 0.0;
				PPID   in_tax_grp_id = rRec.InTaxGrpID;
				if(grci.Cost != 0.0 && Filt.Flags & (GoodsRestFilt::fCWoVat|GoodsRestFilt::fCalcCVat))
					if(vat_free < 0)
						vat_free = IsLotVATFree(rRec);
				tax_grp_id = (GObj.Fetch(goods_id, &goods_rec) > 0) ? goods_rec.TaxGrpID : 0;
				SETIFZ(in_tax_grp_id, tax_grp_id);
				if(tax_factor != 1.0)
					GObj.MultTaxFactor(goods_id, &tax_factor);
				GTaxVect vect;
				PPGoodsTaxEntry gtx;
				// Calculating cost without VAT
				if(grci.Cost != 0.0 && Filt.Flags & (GoodsRestFilt::fCWoVat|GoodsRestFilt::fCalcCVat)) {
					if(!org_lot_date)
						THROW(P_BObj->trfr->Rcpt.GetOriginDate(&rRec, &org_lot_date));
					if(GObj.GTxObj.Fetch(in_tax_grp_id, org_lot_date, 0L, &gtx) > 0) {
						vect.Calc_(&gtx, grci.Cost, tax_factor, ~GTAXVF_SALESTAX, (vat_free > 0) ? GTAXVF_VAT : 0);
						_cvat = vect.GetValue(GTAXVF_VAT);
						_cost_wo_vat = grci.Cost - vect.GetValue(GTAXVF_VAT);
					}
				}
				// Calculating price without VAT and SalesTax
				if(GObj.GTxObj.FetchByID(tax_grp_id, &gtx) > 0) {
					const long amt_fl = (CCFLG_PRICEWOEXCISE & CConfig.Flags) ? ~GTAXVF_SALESTAX : GTAXVF_BEFORETAXES;
					vect.Calc_(&gtx, grci.Price, tax_factor, amt_fl, 0);
					_price_wo_vat = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_EXCISE);
					_pvat = grci.Price - _price_wo_vat;
				}
				if(Filt.Flags & GoodsRestFilt::fCWoVat) {
					grci.Cost = _cost_wo_vat;
					grci.Price = _price_wo_vat;
				}
				if(Filt.Flags & GoodsRestFilt::fCalcCVat) {
					grci.SumCVat = _cvat * grci.Rest;
					grci.SumPVat = _pvat * grci.Rest;
				}
			}
			grci.GoodsID = labs(grci.GoodsID);
			GObj.GetPhUPerU(grci.GoodsID, 0, &ph_u_per_u);
			grci.PhRest = grci.Rest * ph_u_per_u;
			grci.Deficit = GetDeficit(grci.GoodsID);
			if(Filt.DiffParam & GoodsRestParam::_diffSerial) {
				SString serial;
				P_BObj->GetSerialNumberByLot(rRec.ID, serial, 0);
				if(serial.Empty() && rRec.PrevLotID) {
					ReceiptTbl::Rec org_lot_rec;
					if(P_BObj->trfr->Rcpt.SearchOrigin(rRec.PrevLotID, 0, 0, &org_lot_rec))
						P_BObj->GetSerialNumberByLot(org_lot_rec.ID, serial, 0);
				}
				CacheBuf.SetupCacheItemSerial(grci, serial);
			}
			// @v9.7.11 {
			if(Filt.DiffParam == GoodsRestParam::_diffLotTag && Filt.DiffLotTagID) {
				SString tag_val;
				PPRef->Ot.GetTagStr(PPOBJ_LOT, rRec.ID, Filt.DiffLotTagID, tag_val);
				/*
				if(tag_val.Empty() && rRec.PrevLotID) {
					ReceiptTbl::Rec org_lot_rec;
					if(P_BObj->trfr->Rcpt.SearchOrigin(rRec.PrevLotID, 0, 0, &org_lot_rec))
						P_BObj->GetSerialNumberByLot(org_lot_rec.ID, serial, 0);
				}
				*/
				CacheBuf.SetupCacheItemLotTag(grci, tag_val);
			}
			// } @v9.7.11
			if(Filt.Sgg) {
				PPObjGoods::SubstBlock sgg_blk;
				sgg_blk.ExclParentID = Filt.GoodsGrpID;
				sgg_blk.LocID = grci.LocID;
				sgg_blk.P_LotRec = &rRec;
				THROW(GObj.SubstGoods(grci.GoodsID, &grci.GoodsID, Filt.Sgg, &sgg_blk, &Gsl));
			}
			PROFILE_START
			if(Filt.Flags & GoodsRestFilt::fCalcTotalOnly) {
				THROW(AddTotal(grci));
			}
			else
				THROW(AddCacheItem(grci));
			PROFILE_END
		}
	}
	CATCHZOK
	return ok;
}

IMPL_CMPFUNC(ReceiptTbl_DtOprNo, i1, i2) { RET_CMPCASCADE2(static_cast<const ReceiptTbl::Rec *>(i1), static_cast<const ReceiptTbl::Rec *>(i2), Dt, OprNo); }

PPViewGoodsRest::LotQueryBlock::LotQueryBlock() : Idx(-1), SpMode(-1), Reverse(0), P_Q(0)
{
	// @v10.6.8 @ctr memzero(Key, sizeof(Key));
}

PPViewGoodsRest::LotQueryBlock::~LotQueryBlock()
{
	BExtQuery::ZDelete(&P_Q);
}

int SLAPI PPViewGoodsRest::MakeLotQuery(LotQueryBlock & rBlk, int lcr, long lowId, long uppId)
{
	assert(!lcr || Filt.Date);
	int    ok = 1;
	uint   query_buf_capacity = 256;
	ReceiptCore * r_t = & P_BObj->trfr->Rcpt;
	union {
		ReceiptTbl::Key0 k0;
		ReceiptTbl::Key1 k1;
		ReceiptTbl::Key2 k2;
		ReceiptTbl::Key3 k3;
		ReceiptTbl::Key5 k5;
		ReceiptTbl::Key7 k7;
	} kx;
	MEMSZERO(kx);
	DBQ  * dbq = 0;
	const  PPID single_loc_id = LocList.GetSingle();
	if(lowId && uppId) {
		assert(lowId < uppId);
		rBlk.Idx = 0;
		rBlk.SpMode = spGe;
		kx.k0.ID = lowId;
		dbq = &(r_t->ID >= lowId && r_t->ID <= uppId);
		query_buf_capacity = MIN(256, (uppId-lowId+1));
	}
	else {
		if(!lcr && Filt.CalcMethod == GoodsRestParam::pcmLastLot)
			rBlk.Reverse = 1;
		if(!Filt.Date || lcr) {
			const int null_rest = BIN(!lcr && Filt.Flags & GoodsRestFilt::fNullRest);
			if(single_loc_id) {
				rBlk.Idx = 7;
				kx.k7.LocID = single_loc_id;
				kx.k7.Closed = 0;
				dbq = & (r_t->LocID == single_loc_id);
				if(rBlk.Reverse) {
					kx.k7.Closed = null_rest;
					kx.k7.Dt    = MAXDATE;
					kx.k7.OprNo = MAXLONG;
					rBlk.SpMode = spLt;
				}
				else
					rBlk.SpMode = spGt;
			}
			else {
				rBlk.Idx = 3;
				if(rBlk.Reverse) {
					kx.k3.Closed  = null_rest;
					kx.k3.GoodsID = MAXLONG;
					kx.k3.LocID   = MAXLONG;
					kx.k3.Dt      = MAXDATE;
					kx.k3.OprNo   = MAXLONG;
					rBlk.SpMode = spLt;
				}
				else {
					if(Filt.Flags & GoodsRestFilt::fCalcOrder)
						kx.k3.GoodsID = -MAXLONG;
					rBlk.SpMode = spGt;
				}
			}
			if(!null_rest)
				dbq = & (*dbq && r_t->Closed == 0L);
		}
		else if(Filt.SupplID && !(Filt.Flags & GoodsRestFilt::fWoSupplier)) {
			rBlk.Idx = 5;
			dbq = & (r_t->SupplID == Filt.SupplID);
			kx.k5.SupplID = Filt.SupplID;
			if(rBlk.Reverse) {
				kx.k5.Dt    = MAXDATE;
				kx.k5.OprNo = MAXLONG;
				rBlk.SpMode = spLt;
			}
			else
				rBlk.SpMode = spGt;
		}
		else if(single_loc_id) {
			rBlk.Idx = 7;
			kx.k7.LocID = single_loc_id;
			dbq = & (r_t->LocID == single_loc_id);
			if(rBlk.Reverse) {
				kx.k7.Closed = 1000;
				kx.k7.Dt    = MAXDATE;
				kx.k7.OprNo = MAXLONG;
				rBlk.SpMode = spLt;
			}
			else
				rBlk.SpMode = spGt;
		}
		else {
			rBlk.Idx = 1;
			if(rBlk.Reverse) {
				kx.k1.Dt    = MAXDATE;
				kx.k1.OprNo = MAXLONG;
				rBlk.SpMode = spLt;
			}
			else
				rBlk.SpMode = spGt;
		}
	}
	if(Filt.Date)
		dbq = & (*dbq && r_t->Dt <= Filt.Date);
	if(!oneof2(rBlk.Idx, 3, 7))
		dbq = ppcheckfiltidlist(dbq, r_t->LocID, &LocList.Get());
	if(Filt.SupplID && rBlk.Idx != 5 && !(Filt.Flags & GoodsRestFilt::fWoSupplier))
		dbq = & (*dbq && r_t->SupplID == Filt.SupplID);
	if(!(Filt.Flags & GoodsRestFilt::fCalcOrder))
		dbq = & (*dbq && r_t->GoodsID > 0L);
	if(!lcr && !(Filt.Flags & GoodsRestFilt::fNullRest))
		if(Filt.Date)
			dbq = & (*dbq && r_t->CloseDate >= Filt.Date);
		else if(!oneof2(rBlk.Idx, 3, 7))
			dbq = & (*dbq && r_t->Closed == 0L);
	//
	// Гарантируем, что Idx и SpMode инициализированы
	//
	assert(rBlk.Idx != -1);
	assert(rBlk.SpMode != -1);
	//
	THROW_MEM(rBlk.P_Q = new BExtQuery(r_t, rBlk.Idx, query_buf_capacity));
	MakeLotSelectFldList(*rBlk.P_Q, *r_t).where(*dbq);
	CATCHZOK
	memcpy(rBlk.Key_, &kx, sizeof(kx));
	return ok;
}

//static
BExtQuery & FASTCALL PPViewGoodsRest::MakeLotSelectFldList(BExtQuery & rQ, const ReceiptTbl & rT)
{
	return rQ.select(rT.ID, rT.BillID, rT.Dt, rT.OprNo, rT.GoodsID, rT.LocID, rT.SupplID,
		rT.InTaxGrpID, rT.Flags, rT.UnitPerPack, rT.PrevLotID, rT.Cost, rT.Price,
		rT.Rest, rT.Closed, rT.Expiry, 0L); // @v9.7.11 rT.Expiry
}

int SLAPI PPViewGoodsRest::SelectLcrLots(const PPIDArray & rIdList, const UintHashTable & rLcrList, SVector & rList) // @v9.8.8 SArray-->SVector
{
	int    ok = 1;
	ReceiptCore & r_t = P_BObj->trfr->Rcpt;
	const uint id_count = rIdList.getCount();
	if(id_count < 3) {
		for(uint i = 0; i < id_count; i++) {
			if(r_t.Search(rIdList.get(i), 0) > 0) {
				THROW_SL(rList.insert(&r_t.data));
			}
		}
	}
	else {
		LotQueryBlock q_blk;
		THROW(MakeLotQuery(q_blk, 0, rIdList.get(0), rIdList.getLast()));
		for(q_blk.P_Q->initIteration(q_blk.Reverse, q_blk.Key_, q_blk.SpMode); q_blk.P_Q->nextIteration() > 0;) {
			if(rLcrList.Has(r_t.data.ID))
				THROW_SL(rList.insert(&r_t.data));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsRest::ProcessLots2(const PPIDArray * pGrpGoodsList)
{
	int    ok = 1, r;
	ReceiptCore * r_t = & P_BObj->trfr->Rcpt;
	PROFILE_START
	UintHashTable lcr_lot_list;
	RAssocArray lcr_rest_list;
	SString msg_buf;
	if(CConfig.LcrUsage == 2 && (r = P_BObj->trfr->GetLcrList(Filt.Date, &lcr_lot_list, &lcr_rest_list)) > 0) {
		IterCounter cntr;
		SVector lot_list(sizeof(ReceiptTbl::Rec)); // @v9.8.8 SArray-->SVector
		THROW(r);
		{
			const long max_hole = 5;
			PPIDArray id_list;
			cntr.Init(lcr_lot_list.GetCount());
			PPLoadText(PPTXT_GRESTLOTEXTRACT, msg_buf);
			for(ulong lot_id_ = 0, lot_id_prev = 0; lcr_lot_list.Enum(&lot_id_); lot_id_prev = lot_id_) {
				if((lot_id_ - lot_id_prev) > max_hole) {
					THROW(SelectLcrLots(id_list, lcr_lot_list, lot_list));
					id_list.clear();
				}
				id_list.add(static_cast<long>(lot_id_));
				PPWaitPercent(cntr.Increment(), msg_buf);
			}
			THROW(SelectLcrLots(id_list, lcr_lot_list, lot_list));
		}
		lot_list.sort(PTR_CMPFUNC(ReceiptTbl_DtOprNo));
		{
			const uint lc = lot_list.getCount();
			for(uint i = 0; i < lc; i++) {
				ReceiptTbl::Rec & r_lot_rec = *static_cast<ReceiptTbl::Rec *>(lot_list.at(i));
				double rest = 0.0;
				r = lcr_rest_list.Search(r_lot_rec.ID, &rest, 0, 1);
				assert(r);
				if(r)
					r_lot_rec.Rest = rest;
				else
					r_lot_rec.Rest = 0.0; // @err Такого не должно быть.
			}
		}
		{
			//
			// Открытые лоты, последняя операция по которым предшествовала дате Filt.Date.
			// Такие лоты не входят в индекс.
			//
			LotQueryBlock q_blk;
			SString temp_buf;
			PPLoadText(PPTXT_GRESTOPENEDLOTEXTRACT, msg_buf);
			THROW(MakeLotQuery(q_blk, 1, 0, 0));
			for(q_blk.P_Q->initIteration(q_blk.Reverse, q_blk.Key_, q_blk.SpMode); q_blk.P_Q->nextIteration() > 0;) {
				if(!lcr_lot_list.Has(static_cast<ulong>(r_t->data.ID)))
					THROW_SL(lot_list.insert(&r_t->data));
				PPWaitMsg((temp_buf = msg_buf).Space().Cat(r_t->data.Dt));
			}
		}
		if(Filt.Flags & GoodsRestFilt::fNullRest) {
			//
			// Товары с нулевым остатком придется обрабатывать специальным образом
			//
			PPLoadText(PPTXT_GRESTNULLRESTPROCESS, WaitMsg);
			PPWaitMsg(WaitMsg);
			//
			// Сначала извлекаем список товаров, которые когда-либо были на складе (складах)
			//
			UintHashTable full_goods_list;
			const uint loc_count = LocList.GetCount();
			if(loc_count) {
				for(uint i = 0; i < loc_count; i++) {
					THROW(P_BObj->trfr->GetLocGoodsList(LocList.Get().get(i), full_goods_list));
				}
			}
			else
				THROW(P_BObj->trfr->GetLocGoodsList(0, full_goods_list));
			if(pGrpGoodsList) {
				//
				// Если есть ограничение по списку товаров, то убираем из full_goods_list
				// позиции, которые не входят в ограничивающий список.
				//
				for(ulong id_ = 0; full_goods_list.Enum(&id_);) {
					if(!pGrpGoodsList->bsearch(static_cast<long>(id_)))
						full_goods_list.Remove(id_);
				}
			}
			{
				//
				// Далее, выбрасываем из full_goods_list те позиции, по которым есть ненулевые остатки
				// в списке лотов.
				//
				const uint lc = lot_list.getCount();
				for(uint i = 0; i < lc; i++) {
					const ReceiptTbl::Rec * p_rec = static_cast<const ReceiptTbl::Rec *>(lot_list.at(i));
					const PPID goods_id = p_rec->GoodsID;
					if(goods_id > 0 && p_rec->Rest > 0.0)
						full_goods_list.Remove(static_cast<ulong>(goods_id));
				}
			}
			{
				//
				// Наконец, производим расчет по списку товаров, остаток по которым нулевой.
				//
				PPIDArray goods_list;
				for(ulong id_ = 0; full_goods_list.Enum(&id_);)
					goods_list.add(static_cast<PPID>(id_));
				THROW(ProcessGroup(&goods_list));
			}
		}
		if(!(Filt.Flags & GoodsRestFilt::fNullRestsOnly)) {
			uint lc = lot_list.getCount();
			if(lc) {
				lot_list.sort(PTR_CMPFUNC(ReceiptTbl_DtOprNo));
				ProcessLotBlock blk(1);
				THROW(InitProcessLotBlock(blk, pGrpGoodsList));
				blk.Cntr.Init(lc);
				if(Filt.CalcMethod == GoodsRestParam::pcmLastLot) {
					do {
						ReceiptTbl::Rec & r_rec = *static_cast<ReceiptTbl::Rec *>(lot_list.at(--lc));
						if(r_rec.Rest > 0.0) {
							THROW(Helper_ProcessLot(blk, r_rec));
						}
					} while(lc);
				}
				else {
					for(uint i = 0; i < lc; i++) {
						ReceiptTbl::Rec & r_rec = *static_cast<ReceiptTbl::Rec *>(lot_list.at(i));
						if(r_rec.Rest > 0.0) {
							THROW(Helper_ProcessLot(blk, r_rec));
						}
					}
				}
			}
		}
	}
	else {
		LotQueryBlock q_blk;
		ProcessLotBlock blk(0);
		THROW(MakeLotQuery(q_blk, 0, 0, 0));
		THROW(InitProcessLotBlock(blk, pGrpGoodsList));
		for(q_blk.P_Q->initIteration(q_blk.Reverse, q_blk.Key_, q_blk.SpMode); q_blk.P_Q->nextIteration() > 0;) {
			ReceiptTbl::Rec lot_rec;
			r_t->copyBufTo(&lot_rec);
			THROW(Helper_ProcessLot(blk, lot_rec));
		}
	}
	PROFILE_END
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsRest::GetTabTitle(long tabID, SString & rBuf)
{
	if(tabID) {
		LocationTbl::Rec loc_rec;
		if(LocObj.Search(tabID, &loc_rec) > 0)
			rBuf.CopyFrom(loc_rec.Name);
		else
			rBuf.Z().Cat(tabID);
	}
	return 1;
}

class GoodsRestCrosstab : public Crosstab {
public:
	struct TotalItem {
		double Sum;
		long   MtxCount;
		long   NzMtxCount;
		double RelNzToMtx;
	};
	SLAPI  GoodsRestCrosstab(PPViewGoodsRest * pV) : Crosstab(), P_V(pV)
	{
	}
	virtual BrowserWindow * SLAPI CreateBrowser(uint brwId, int dataOwner)
	{
		PPViewBrowser * p_brw = new PPViewBrowser(brwId, CreateBrowserQuery(), P_V, dataOwner);
		SetupBrowserCtColumns(p_brw);
		return p_brw;
	}
private:
	virtual int SLAPI GetTabTitle(const void * pVal, TYPEID typ, SString & rBuf) const
	{
		return (pVal && /*typ == MKSTYPE(S_INT, 4) &&*/ P_V) ? P_V->GetTabTitle(*static_cast<const long *>(pVal), rBuf) : 0;
	}
	virtual int SLAPI CalcSummary(int action, CalcSummaryBlock & rBlk)
	{
		PPID loc_id = 0;
		PPID goods_id = 0;
		TotalItem * p_total_item = static_cast<TotalItem *>(rBlk.P_ExtData);
		assert(p_total_item);
		if(action == 0) {
			int in_mtx = 0;
			if(oneof3(rBlk.TotalItemPos, 1, 2, 3)) {
				GetTab(rBlk.CtValPos, &loc_id);
				GetIdxFieldVal(0, 0, &goods_id, sizeof(goods_id));
				if(GObj.P_Tbl->BelongToMatrix(goods_id, loc_id) > 0)
					in_mtx = 1;
			}
			switch(rBlk.TotalItemPos) {
				case 0:
					rBlk.Result += rBlk.CellVal;
					break;
				case 1:
					if(in_mtx) {
						rBlk.Result += 1.0;
						if(p_total_item) {
							p_total_item->MtxCount++;
							if(rBlk.CellVal > 0.0)
								p_total_item->NzMtxCount++;
						}
					}
					break;
				case 2:
					if(in_mtx) {
						if(rBlk.CellVal > 0.0)
							rBlk.Result += 1.0;
						if(p_total_item) {
							p_total_item->MtxCount++;
							if(rBlk.CellVal > 0.0)
								p_total_item->NzMtxCount++;
						}
					}
					break;
				case 3:
					if(p_total_item) {
						if(in_mtx) {
							p_total_item->MtxCount++;
							if(rBlk.CellVal > 0.0)
								p_total_item->NzMtxCount++;
							p_total_item->RelNzToMtx = fdivnz(p_total_item->NzMtxCount, p_total_item->MtxCount);
						}
						rBlk.Result = 100.0 * fdivnz(p_total_item->NzMtxCount, p_total_item->MtxCount);
					}
					else
						rBlk.Result = 0.0;
					break;
			}
		}
		else if(action == 1) {
			if(rBlk.TotalItemPos == 3) {
				if(p_total_item) {
					rBlk.Result = 100.0 * fdivnz(p_total_item->NzMtxCount, p_total_item->MtxCount);
					//
					// @6.2.x AHTOXA (полагаемся на то, что все 3 колонки уже обработаны и обнуляем данные для след.строки)
					//
					p_total_item->NzMtxCount = p_total_item->MtxCount = 0;
				}
			}
		}
		return 1;
	}
	PPViewGoodsRest * P_V;
	PPObjGoods GObj;
};

int SLAPI PPViewGoodsRest::CreateTempTable(int use_ta, double * pPrfMeasure)
{
	int    ok = 1;
	int    use_goods_iterator = 0;
	int    loc_list_empty = LocList.IsEmpty();
	double prf_measure = 0.0;
	double prf_measure_coeff = 1.0;
	DateRange period;
	PPIDArray group_goods_list;
	const  PPIDArray * p_group_goods_list = 0;
	ZDELETE(P_Tbl);
	DeficitList.freeAll();
	ExclDeficitList.freeAll();
	DraftRcptList.freeAll();
	ExclDraftRcptList.freeAll();
	UncompleteSessQttyList.freeAll();
	ExclUncompleteSessQttyList.freeAll();
	if(P_BObj) {
		if(Filt.Flags & GoodsRestFilt::fCalcDeficit) {
			prf_measure_coeff *= 1.1;
			period.Set(Filt.DeficitDt, Filt.Date);
			THROW(P_BObj->GetDeficitList(&period, (loc_list_empty ? 0 : &LocList.Get()), &DeficitList));
		}
		if(Filt.Flags & GoodsRestFilt::fShowDraftReceipt) {
			prf_measure_coeff *= 1.05;
			period = Filt.DraftRcptPrd;
			if(period.IsZero())
				period.Set(LConfig.OperDate, ZERODATE);
			THROW(P_BObj->GetDraftRcptList(&period, (loc_list_empty ? 0 : &LocList.Get()), &DraftRcptList));
		}
	}
	if(Filt.Flags & GoodsRestFilt::fCalcUncompleteSess) {
		prf_measure_coeff *= 1.1;
		CCheckTbl.GetActiveExpendByLocList(&LocList, &UncompleteSessQttyList);
#if 0 // @v8.5.2 кассовые чеки полностью отражают текущие продажи - драфт-документы не нужны {
		if(P_BObj) {
			uint   i;
			PPIDArray cashn_list, loc_list;
			PPObjCashNode obj_cashn;
			PPObjLocation obj_loc;
			if(loc_list_empty)
				obj_loc.GetWarehouseList(&loc_list);
			else
				LocList.CopyTo(&loc_list);
			for(i = 0; i < loc_list.getCount(); i++)
				obj_cashn.GetListByLoc(loc_list.at(i), cashn_list);
			for(i = 0; i < cashn_list.getCount(); i++) {
				PPGenCashNode cashn_rec;
				if(obj_cashn.Get(cashn_list.at(i), &cashn_rec) > 0 && cashn_rec.CurRestBillID) {
					PPBillPacket bpack;
					if(P_BObj->ExtractPacket(cashn_rec.CurRestBillID, &bpack) > 0) {
						PPTransferItem * p_ti = 0;
						for(uint j = 0; bpack.EnumTItems(&j, &p_ti) > 0;) {
							uint pos = 0;
							DraftRcptItem dr_item;
							dr_item.GoodsID = p_ti->GoodsID;
							dr_item.LocID   = p_ti->LocID;
							dr_item.Qtty    = -p_ti->Qtty();
							if(UncompleteSessQttyList.lsearch(&dr_item, &pos, PTR_CMPFUNC(_2long)) > 0)
								UncompleteSessQttyList.at(pos).Qtty += dr_item.Qtty;
							else
								UncompleteSessQttyList.insert(&dr_item);
						}
					}
				}
			}
		}
#endif // } 0 // @v8.5.2
		UncompleteSessQttyList.sort(PTR_CMPFUNC(_2long));
	}
	if(!(Filt.Flags & GoodsRestFilt::fCalcTotalOnly)) {
		THROW(P_Tbl = CreateTempFile <TempGoodsRestTbl> ());
	}
	else {
		prf_measure_coeff *= 0.95;
		use_ta = 0;
	}
	{
		PPTransaction tra(ppDbDependTransaction, use_ta);
		THROW(tra);
		InitCache();
		if(!Filt.GoodsList.IsEmpty()) {
			p_group_goods_list = &Filt.GoodsList.Get();
			THROW(ProcessGroup(p_group_goods_list));
		}
		else {
			if(Filt.GoodsList.IsEmpty() && (Filt.GoodsGrpID || Filt.SupplID || Filt.BrandID ||
				(Filt.Flags & (GoodsRestFilt::fUseGoodsMatrix|GoodsRestFilt::fExtByArCode|GoodsRestFilt::fRestrictByArCode)))) {
				RECORDNUMBER num_goods = 0, grp_count = 0;
				GObj.P_Tbl->getNumRecs(&num_goods);
				PPIDArray ext_goods_list;
				GoodsFilt goods_flt;
				goods_flt.GrpID   = Filt.GoodsGrpID;
				goods_flt.SupplID = Filt.SupplID;
				goods_flt.BrandList.Add(Filt.BrandID);
				goods_flt.LocList = LocList;
				SETFLAG(goods_flt.Flags, GoodsFilt::fRestrictByMatrix, Filt.Flags & GoodsRestFilt::fUseGoodsMatrix);
				if(Filt.Flags & GoodsRestFilt::fExtByArCode) {
					GoodsFilt goods_flt_ext;
					goods_flt_ext.Flags |= GoodsFilt::fShowArCode;
					SETFLAG(goods_flt_ext.Flags, GoodsFilt::fRestrictByMatrix, Filt.Flags & GoodsRestFilt::fUseGoodsMatrix);
					goods_flt_ext.CodeArID = Filt.SupplID;
					if(!goods_flt_ext.CodeArID)
						goods_flt_ext.Flags |= GoodsFilt::fShowOwnArCode;
					THROW(GoodsIterator::GetListByFilt(&goods_flt_ext, &ext_goods_list));
				}
				else if(Filt.Flags & GoodsRestFilt::fRestrictByArCode) {
					goods_flt.Flags |= GoodsFilt::fShowArCode;
					goods_flt.CodeArID = Filt.SupplID;
					if(!goods_flt.CodeArID)
						goods_flt.Flags |= GoodsFilt::fShowOwnArCode;
					goods_flt.SupplID = 0; // В случае ограничения по артикулу убираем ограничение по поставщику.
				}
				goods_flt.Flags |= GoodsFilt::fIncludeIntr;
				THROW(GoodsIterator::GetListByFilt(&goods_flt, &group_goods_list));
				group_goods_list.add(&ext_goods_list);
				group_goods_list.sortAndUndup();
				grp_count = group_goods_list.getCount();
				if((Filt.Flags & (GoodsRestFilt::fNullRest|GoodsRestFilt::fShowDraftReceipt)) || (num_goods && (((1000L * grp_count) / num_goods) < (ulong)GroupCalcThreshold)))
					use_goods_iterator = 1;
				else
					use_goods_iterator = 0;
				p_group_goods_list = &group_goods_list;
			}
			// Условие Filt.Sgg != sggSuppl нужно из-за того, что
			// при расчете остатков по товарам информация о поставщике теряется.
			if(Filt.Sgg != sggSuppl && (use_goods_iterator || (Filt.Flags &
				(GoodsRestFilt::fNullRest|GoodsRestFilt::fShowDraftReceipt|GoodsRestFilt::fNoZeroOrderOnly|GoodsRestFilt::fUnderMinStock)))) {
				THROW(ProcessGroup(p_group_goods_list));
			}
			else {
				THROW(ProcessLots2(p_group_goods_list));
			}
		}
		THROW(FlashCacheItems(0));
		if(Filt.Flags & GoodsRestFilt::fCalcDeficit && !(Filt.Flags & GoodsRestFilt::fNullRest)) {
			PPObjGoods::SubstBlock sgg_blk;
			sgg_blk.ExclParentID = Filt.GoodsGrpID;

			GoodsFilt g_filt;
			g_filt.GrpID   = Filt.GoodsGrpID;
			g_filt.BrandList.Add(Filt.BrandID);
			g_filt.SupplID = Filt.SupplID;
			g_filt.LotPeriod.Set(Filt.Date, ZERODATE);
			for(uint i = 0; i < DeficitList.getCount(); i++) {
				PPID   goods_id = DeficitList.at(i).Key;
				if(!ExclDeficitList.lsearch(goods_id) && GObj.CheckForFilt(&g_filt, goods_id) > 0) {
					int    is_subst = 0;
					double ph_u_per_u = 0.0;
					GoodsRestVal grv;
					ReceiptTbl::Rec lot_rec;
					// @v10.6.4 MEMSZERO(lot_rec);
					GObj.GetPhUPerU(goods_id, 0, &ph_u_per_u);
					::GetCurGoodsPrice(goods_id, LocList.GetSingle(), GPRET_MOSTRECENT | GPRET_OTHERLOC, &grv.Price, &lot_rec);
					grv.Rest    = 0.0;
					grv.Cost    = lot_rec.Cost;
					grv.Deficit = DeficitList.at(i).Val;
					if(Filt.Sgg) {
						THROW(GObj.SubstGoods(goods_id, &goods_id, Filt.Sgg, &sgg_blk, &Gsl));
						is_subst = 1;
					}
					THROW(AddGoodsThruCache(goods_id, LocList.GetSingle(), is_subst, 0, ph_u_per_u, &grv, 0));
				}
			}
		}
		if(Filt.Flags & GoodsRestFilt::fCalcTotalOnly)
			Flags |= fTotalInited;
		//
		// Здесь транзакцию завершаем. Создание кросс-таблицы осуществляется в собственной транзакции.
		//
		THROW(tra.Commit());
		{
			Flags |= fOnceInited;
			//
			// Предыдущий кросстаб можно будет разрушить только после успешного создания нового.
			// В противном случае возникают побочные эффекты. Например, новая таблица кросстаба
			// создается с идентификатором таким же, что и существующая. В результате появляются совершенно
			// непонятные ошибки (из-за того, что новая таблица создается как клон существующей).
			//
			Crosstab * p_prev_ct = P_Ct;
			P_Ct = 0;
			if(Filt.Flags & GoodsRestFilt::fCrosstab) {
				RECORDNUMBER nr;
				THROW_DB(P_Tbl->getNumRecs(&nr));
				if(nr > 0) {
					prf_measure_coeff *= 1.2;
					SString temp_buf;
					DBFieldList total_list;
					THROW_MEM(P_Ct = new GoodsRestCrosstab(this)); // Crosstab
					P_Ct->SetTable(P_Tbl, P_Tbl->LocID);
					P_Ct->AddIdxField(P_Tbl->GoodsID);
					P_Ct->SetSortIdx("GoodsName", 0);
					P_Ct->AddInheritedFixField(P_Tbl->GoodsName);
					//P_Ct->AddInheritedFixField(P_Tbl->LocID);
					P_Ct->AddAggrField(P_Tbl->Quantity);
					total_list.Add(P_Tbl->Quantity);
					P_Ct->AddTotalRow(total_list, sizeof(GoodsRestCrosstab::TotalItem), PPGetWord(PPWORD_TOTAL, 0, temp_buf));
					P_Ct->AddTotalColumn(P_Tbl->Quantity, sizeof(GoodsRestCrosstab::TotalItem), PPGetWord(PPWORD_TOTAL, 0, temp_buf));
					if(Filt.Flags & GoodsRestFilt::fShowGoodsMatrixBelongs) {
						PPGetSubStr(PPTXT_GREST_CT_TOTAL, 0, temp_buf);
						P_Ct->AddTotalRow(total_list, sizeof(GoodsRestCrosstab::TotalItem), temp_buf);
						PPGetSubStr(PPTXT_GREST_CT_TOTAL, 1, temp_buf);
						P_Ct->AddTotalRow(total_list, sizeof(GoodsRestCrosstab::TotalItem), temp_buf);
						PPGetSubStr(PPTXT_GREST_CT_TOTAL, 2, temp_buf);
						P_Ct->AddTotalRow(total_list, sizeof(GoodsRestCrosstab::TotalItem), temp_buf);

						PPGetSubStr(PPTXT_GREST_CT_TOTAL, 0, temp_buf);
						P_Ct->AddTotalColumn(P_Tbl->Quantity, sizeof(GoodsRestCrosstab::TotalItem), temp_buf);
						PPGetSubStr(PPTXT_GREST_CT_TOTAL, 1, temp_buf);
						P_Ct->AddTotalColumn(P_Tbl->Quantity, sizeof(GoodsRestCrosstab::TotalItem), temp_buf);
						PPGetSubStr(PPTXT_GREST_CT_TOTAL, 2, temp_buf);
						P_Ct->AddTotalColumn(P_Tbl->Quantity, sizeof(GoodsRestCrosstab::TotalItem), temp_buf);
					}
					THROW(P_Ct->Create(use_ta));
				}
				ok = 1;
			}
			ZDELETE(p_prev_ct);
		}
		{
			if(Filt.Flags & GoodsRestFilt::fCalcTotalOnly)
				prf_measure = Total.Count;
			else
				prf_measure = CacheStat.InsertCount + (((double)CacheStat.UpdateCount) * 0.3);
			prf_measure *= prf_measure_coeff;
		}
	}
	CATCH
		ZDELETE(P_Ct);
		ZDELETE(P_Tbl);
		ok = 0;
	ENDCATCH
	GoodsIDs.Clear();
	CacheBuf.Clear();
	ASSIGN_PTR(pPrfMeasure, prf_measure);
	return ok;
}

// AHTOXA {
int SLAPI PPViewGoodsRest::CreateOrderTable(IterOrder ord, TempOrderTbl ** ppTbl)
{
	int    ok = -1;
	TempOrderTbl * p_o = 0;
	BExtInsert * p_bei = 0;
	if(oneof4(ord, OrdByPrice, OrdByGrp_Price, OrdByBarCode, OrdByGrp_BarCode)) {
		SString temp_buf;
		SString code_buf;
		TempGoodsRestTbl::Key0 k;
		TempGoodsRestTbl * p_t = P_Tbl;
		BExtQuery q(p_t, 0, 64);
		THROW(p_o = CreateTempOrderFile());
		THROW_MEM(p_bei = new BExtInsert(p_o));
		q.select(p_t->ID__, p_t->GoodsID, p_t->GoodsGrp, /*p_t->BarCode*/p_t->BarcodeSP, p_t->Price, 0L); // @v9.8.3 p_t->BarCode-->p_t->BarcodeSP
		MEMSZERO(k);
		for(q.initIteration(0, &k, spFirst); q.nextIteration() > 0;) {
			TempOrderTbl::Rec ord_rec;
			// @v10.7.9 @ctr MEMSZERO(ord_rec);
			ord_rec.ID = p_t->data.ID__;
			if(ord == OrdByPrice)
				sprintf(ord_rec.Name, "%055.8lf", p_t->data.Price);
			else if(ord == OrdByBarCode) {
				StrPool.GetS(p_t->data.BarcodeSP, code_buf); // @v9.8.3
				sprintf(ord_rec.Name, "%-63s", /*p_t->data.BarCode*/code_buf.cptr());
			}
			else if(ord == OrdByGrp_Price || ord == OrdByGrp_BarCode) {
				// @v9.5.5 GetGoodsName(p_t->data.GoodsGrp, grp_name);
				GObj.FetchNameR(p_t->data.GoodsGrp, temp_buf); // @v9.5.5
				temp_buf.Trim(48);
				if(ord == OrdByGrp_Price)
					sprintf(ord_rec.Name, "%-48s%010.5lf", temp_buf.cptr(), p_t->data.Price);
				else if(ord == OrdByGrp_BarCode) {
					StrPool.GetS(p_t->data.BarcodeSP, code_buf); // @v9.8.3
					sprintf(ord_rec.Name, "%-48s%-15s", temp_buf.cptr(), /*p_t->data.BarCode*/code_buf.cptr());
				}
			}
			THROW_DB(p_bei->insert(&ord_rec));
		}
		THROW_DB(p_bei->flash());
		*ppTbl = p_o;
		p_o = 0;
		ok = 1;
	}
	CATCHZOK
	delete p_o;
	delete p_bei;
	return ok;
}
// } AHTOXA
//
//
//
int SLAPI PPViewGoodsRest::InitIterQuery(PPID grpID)
{
	int    ok = -1;
	BExtQuery::ZDelete(&P_IterQuery);
	TempGoodsRestTbl * p_t = P_Tbl;
	if(p_t) {
		// @v10.6.8 char   k_[MAXKEYLEN];
		BtrDbKey k__; // @v10.6.8
		TempGoodsRestTbl::Key2 k2;
		int    sp_mode = spFirst;
   		// @v10.6.8 @ctr memzero(k_, sizeof(k_));
		void * k = k__;
		DBQ * dbq = 0;
		if(Filt.ExhaustTerm > 0) {
			dbq = &(*dbq && p_t->RestInDays >= 0L && p_t->RestInDays <= (long)Filt.ExhaustTerm);
		}
		else if(Filt.ExhaustTerm < 0) {
			dbq = &(*dbq && p_t->RestInDays >= (long)Filt.ExhaustTerm && p_t->RestInDays < 0L);
		}
		P_IterQuery = new BExtQuery(p_t, IterIdx, 16);
		P_IterQuery->selectAll().where(*dbq);
		if(grpID) {
			P_IterQuery->where(p_t->GoodsGrp == grpID);
			if(IterIdx == 2) {
				MEMSZERO(k2);
				k2.GoodsGrp = grpID;
				k = &k2;
				sp_mode = spGe;
			}
		}
		P_IterQuery->initIteration(0, k, sp_mode);
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewGoodsRest::InitGroupNamesList()
{
	IterGrpName.Z();
	PPID   init_parent_id = 0;
	Goods2Tbl::Rec grp_rec;
	if(Filt.GoodsGrpID && GObj.Fetch(Filt.GoodsGrpID, &grp_rec) > 0) {
		init_parent_id = Filt.GoodsGrpID;
		if(grp_rec.Flags & GF_FOLDER) {
			//
			// Если хотя бы одна группа в каталоге - альтернативная, то сортирующий список
			// групп строим по всему множеству обыкновенных групп (init_parent_id = 0)
			//
			PPIDArray term_grp_list;
			GObj.P_Tbl->GetGroupTerminalList(Filt.GoodsGrpID, &term_grp_list, 0);
			for(uint i = 0; i < term_grp_list.getCount(); i++)
				if(GObj.IsAltGroup(term_grp_list.get(i)) > 0) {
					init_parent_id = 0;
					break;
				}
		}
		else if(grp_rec.Flags & GF_ALTGROUP)
			init_parent_id = 0;
	}
	P_GGIter = new GoodsGroupIterator(init_parent_id);
	return P_GGIter ? 1 : PPSetErrorNoMem();
}

int SLAPI PPViewGoodsRest::InitIteration(IterOrder ord)
{
	int    ok = 1;
	IterIdx = 0;
	IterGrpName.Z();
	ZDELETE(P_GGIter);
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempOrd);
	THROW(P_Tbl);
	THROW(CreateOrderTable(ord, &P_TempOrd));
	if(P_TempOrd) {
		BExtQuery::ZDelete(&P_IterQuery);
		TempOrderTbl::Key1 k;
		THROW_MEM(P_IterQuery = new BExtQuery(P_TempOrd, 1, 64));
		P_IterQuery->selectAll();
		MEMSZERO(k);
		P_IterQuery->initIteration(0, &k, spFirst);
		IterIdx = 4;
	}
	else {
		if(ord == OrdByGoodsID)
			IterIdx = 3;
		else if(ord == OrdByGoodsName)
			IterIdx = 1;
		else if(ord == OrdByGrp_GoodsName) {
			IterIdx = 2;
			InitGroupNamesList();
		}
		else
			IterIdx = 0;
		if(IterIdx != 2)
			InitIterQuery(0);
		else
			NextOuterIteration();
	}
	PPInitIterCounter(Counter, P_Tbl);
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsRest::NextOuterIteration()
{
	PPID   grp_id = 0;
	if(P_GGIter && P_GGIter->Next(&grp_id, IterGrpName) > 0) {
		InitIterQuery(grp_id);
		return 1;
	}
	else
		return -1;
}

int SLAPI PPViewGoodsRest::InitAppBuff(const TempGoodsRestTbl::Rec * pRec, GoodsRestViewItem * pItem)
{
	int    ok = -1;
	if(pItem) {
		SString temp_buf;
		memzero(pItem, sizeof(GoodsRestViewItem));
		pItem->GoodsID      = pRec->GoodsID;
		pItem->LocID        = pRec->LocID;
		pItem->LotID        = pRec->LotID;
		pItem->GoodsGrpID   = pRec->GoodsGrp;
		IterGrpName.CopyTo(pItem->GoodsGrpName, sizeof(pItem->GoodsGrpName));
		pItem->UnitPerPack  = pRec->UnitPerPack;
		pItem->Rest         = pRec->Quantity;
		pItem->Deficit      = pRec->Deficit;
		pItem->PhRest       = pRec->PhQtty;
		pItem->Order        = pRec->Ord;
		pItem->Cost         = (Flags & fAccsCost) ? pRec->Cost : 0.0;
		pItem->Price        = pRec->Price;
		pItem->SumCost      = pItem->Cost * pItem->Rest;
		pItem->SumPrice     = pItem->Price * pItem->Rest;
		pItem->SumCVat      = pRec->SumCVat;
		pItem->SumPVat      = pRec->SumPVat;
		// @v9.8.2 pItem->PctAddedVal  = pRec->PctAddedVal;
		// @v9.8.2 {
		if(Flags & fAccsCost && pItem->Cost)
			pItem->PctAddedVal = (float)R5(100.0 * (pItem->Price - pItem->Cost) / pItem->Cost);
		else
			pItem->PctAddedVal = 0.0f;
		// } @v9.8.2
		pItem->IsPredictTrust = pRec->IsPredictTrust;
		pItem->Predict      = pRec->Predict;
		pItem->RestInDays   = pRec->RestInDays;
		pItem->MinStock     = pRec->MinStock;
		pItem->SupplOrder   = pRec->SupplOrder;
		pItem->SStatSales   = pRec->SStatSales;
		pItem->DraftRcpt    = pRec->DraftRcpt;
		pItem->LastSellDate = pRec->LastSellDate;
		pItem->Expiry       = pRec->Expiry; // @v9.7.11
		pItem->SubstAsscCount = pRec->SubstAsscCount;
		// @v9.8.3 STRNSCPY(pItem->Serial, pRec->Serial);
		// @v9.8.3 {
		StrPool.GetS(pRec->SerialSP, temp_buf);
		STRNSCPY(pItem->Serial, temp_buf);
		// } @v9.8.3
		STRNSCPY(pItem->GoodsName, pRec->GoodsName);
		{
			PPUnit unit_rec;
			if(UObj.Fetch(pRec->UnitID, &unit_rec) > 0)
				STRNSCPY(pItem->UnitName, unit_rec.Name);
		}
		ok = 1;
	}
	return ok;
}

int FASTCALL PPViewGoodsRest::NextIteration(GoodsRestViewItem * pItem)
{
	SString temp_buf;
	if(P_Tbl)
		do {
			if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
				if(P_TempOrd) {
					PPID k = P_TempOrd->data.ID;
					if(P_Tbl->search(0, &k, spEq) == 0)
						continue;
				}
				if(InitAppBuff(&P_Tbl->data, pItem) > 0) {
					if(IterIdx > 3 || IterIdx == 0) {
						// @v9.5.5 GetGoodsName(P_Tbl->data.GoodsGrp, IterGrpName);
						GObj.FetchNameR(P_Tbl->data.GoodsGrp, IterGrpName); // @v9.5.5
						if(pItem)
							IterGrpName.CopyTo(pItem->GoodsGrpName, sizeof(pItem->GoodsGrpName));
					}
				}
				Counter.Increment();
				return 1;
			}
		} while(NextOuterIteration() > 0);
	return -1;
}

int SLAPI PPViewGoodsRest::GetItem(PPID goodsID, PPID locID, GoodsRestViewItem * pItem)
{
	TempGoodsRestTbl::Key3 k3;
	k3.GoodsID = goodsID;
	k3.LocID = locID;
	int    sp = locID ? spEq : spGe;
	if(P_Tbl->search(3, &k3, sp) && (sp == spEq || k3.GoodsID == goodsID)) {
		InitAppBuff(&P_Tbl->data, pItem);
		return 1;
	}
	else
		return 0;
}

int SLAPI PPViewGoodsRest::GetItem(PPID goodsID, const ObjIdListFilt * pLocList, GoodsRestViewItem * pItem)
{
	TempGoodsRestTbl::Key3 k3;
	k3.GoodsID = goodsID;
	k3.LocID = 0;
	if(P_Tbl->search(3, &k3, spGe) && k3.GoodsID == goodsID &&
		(!pLocList || pLocList->IsEmpty() || pLocList->Search(k3.LocID, 0) > 0)) {
		InitAppBuff(&P_Tbl->data, pItem);
		return 1;
	}
	else
		return 0;
}

int SLAPI PPViewGoodsRest::GetItem(PPID __id, GoodsRestViewItem * pItem)
{
	TempGoodsRestTbl::Key0 k0;
	k0.ID__ = __id;
	if(P_Tbl->search(0, &k0, spEq)) {
		InitAppBuff(&P_Tbl->data, pItem);
		return 1;
	}
	else
		return 0;
}

int SLAPI PPViewGoodsRest::GetGoodsStat(PPID goodsID, const ObjIdListFilt & rLocList, PredictSalesStat * pStat)
{
	int    ok = 1;
	PredictSalesStat stat;
	if(!P_Predictor)
		THROW_MEM(P_Predictor = new Predictor);
	THROW(P_Predictor->GetStat(goodsID, rLocList, &stat));
	CATCHZOK
	ASSIGN_PTR(pStat, stat);
	return ok;
}

int SLAPI PPViewGoodsRest::GetTableName(SString & rBuf) const
{
	return P_Tbl ? ((rBuf = P_Tbl->GetName()), 1) : (rBuf.Z(), 0);
}

int SLAPI PPViewGoodsRest::SetSupplOrderValues(PPID goodsID, PPID locID, double predict, double minStock, double order, int canTrust)
{
	TempGoodsRestTbl::Key3 k3;
	k3.GoodsID = goodsID;
	k3.LocID = locID;
	int    ok = SearchByKey(P_Tbl, 3, &k3, 0);
	if(ok > 0) {
		TempGoodsRestTbl::Rec & rec = P_Tbl->data;
		if(predict < 0.0) {
			canTrust = 1;
			predict = 0.0;
		}
		rec.IsPredictTrust = canTrust;
		rec.Predict = R0i(predict);
		rec.MinStock = minStock;
		rec.SupplOrder = order;
		ok = P_Tbl->updateRec() ? 1 : PPSetErrorDB();
	}
	return ok;
}

int SLAPI PPViewGoodsRest::GetGoodsBarcode(PPID goodsID, char * barcode, size_t buflen)
{
	return GObj.GetSingleBarcode(goodsID, barcode, buflen);
}

/* @v9.2.2 (replaced by GoodsCore::GetGoodsCodeInAltGrp)
int SLAPI PPViewGoodsRest::GetGoodsNumByAlterGroup(PPID goodsID, PPID grpID, long * pNum)
{
	int    ok = -1;
	long   goods_num = 0;
	if(grpID && GObj.IsAltGroup(grpID) > 0)
		if(PPRef->Assc.Search(PPASS_ALTGOODSGRP, grpID, goodsID) > 0) {
			goods_num = PPRef->Assc.data.InnerNum;
			ok = 1;
		}
	ASSIGN_PTR(pNum, goods_num);
	return ok;
}*/

int SLAPI PPViewGoodsRest::GetTotal(GoodsRestTotal * pTotal)
{
	if(!(Flags & fTotalInited)) {
		Total.Init();
		if(!CalcTotal(&Total))
			return 0;
	}
	ASSIGN_PTR(pTotal, Total);
	Flags |= fTotalInited;
	return 1;
}

int SLAPI PPViewGoodsRest::CalcTotal(GoodsRestTotal * pTotal)
{
	int    ok = 1;
	TempGoodsRestTbl * p_tbl = P_Tbl;
	if(p_tbl) {
		PPObjGoodsType gtobj;
		TempGoodsRestTbl::Key3 k3;
		PROFILE_START
		BExtQuery q(p_tbl, 3);
		q.select(p_tbl->GoodsID, p_tbl->Quantity, p_tbl->PhQtty, p_tbl->Cost,
			p_tbl->Price, p_tbl->SumCVat, p_tbl->SumPVat, p_tbl->Ord, p_tbl->Deficit, p_tbl->DraftRcpt, 0L);
		k3.GoodsID = 0;
		for(q.initIteration(0, &k3, spFirst); q.nextIteration() > 0;) {
			const TempGoodsRestTbl::Rec & r_rec = p_tbl->data;
			double phuperu = 0.0;
			Goods2Tbl::Rec goods_rec;
			PPID   amt_type_cost  = PPAMT_BUYING;
			PPID   amt_type_price = PPAMT_SELLING;
			const double qtty = r_rec.Quantity - r_rec.Deficit;
			const double sum_cost = (Flags & fAccsCost) ? (r_rec.Cost * qtty) : 0.0;
			const double sum_price = (r_rec.Price * qtty);
			GObj.GetPhUPerU(r_rec.GoodsID, 0, &phuperu);
			pTotal->Count++;
			pTotal->Quantity += qtty;
			pTotal->Order    += r_rec.Ord;
			pTotal->PhQtty   += r_rec.PhQtty - r_rec.Deficit * phuperu;
			pTotal->SumCost  += sum_cost;
			pTotal->SumPrice += sum_price;
			pTotal->SumCVat  += r_rec.SumCVat;
			pTotal->SumPVat  += r_rec.SumPVat;
			pTotal->DraftRcpt += r_rec.DraftRcpt;
			pTotal->SumDraftCost  += (Flags & fAccsCost) ? (r_rec.Cost * r_rec.DraftRcpt) : 0.0;
			pTotal->SumDraftPrice += r_rec.Price * r_rec.DraftRcpt;
			if(GObj.Fetch(r_rec.GoodsID, &goods_rec) > 0) {
				const PPID goods_type_id = goods_rec.GoodsTypeID;
				if(goods_type_id && goods_type_id != PPGT_DEFAULT) {
					PPGoodsType gt;
					if(gtobj.Fetch(goods_type_id, &gt) > 0) {
						if(gt.AmtCost)
							amt_type_cost = gt.AmtCost;
						if(gt.AmtPrice)
							amt_type_price = gt.AmtPrice;
					}
				}
			}
			pTotal->Amounts.Add(amt_type_cost,  0L/*@curID*/, sum_cost, 0);
			pTotal->Amounts.Add(amt_type_price, 0L/*@curID*/, sum_price, 0);
		}
		if(pTotal->SumCost > 0)
			pTotal->PctAddedVal = (100.0 * (pTotal->SumPrice - pTotal->SumCost) / pTotal->SumCost);
		PROFILE_END
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPViewGoodsRest::ViewPrediction(PPID goodsID, PPID /*locID*/)
{
	DateRange prd = Filt.PrgnPeriod;
	if(!(Filt.Flags2 & GoodsRestFilt::f2CalcPrognosis) && DateRangeDialog(0, 0, &prd) > 0) { // @v9.5.8 CalcPrognosis-->Flags2
		if(prd.IsZero())
			prd.Set(LConfig.OperDate, plusdate(LConfig.OperDate, 6));
		Filt.PrgnPeriod = prd;
		double predict = 0.0;
		if(GetEstimatedSales(&LocList, goodsID, &prd, &predict) > 0) {
			TDialog * p_dlg = new TDialog(DLG_VIEWPRGN);
			if(CheckDialogPtrErr(&p_dlg)) {
				SString goods_name;
				p_dlg->setCtrlString(CTL_VIEWPRGN_GOODS, GetGoodsName(goodsID, goods_name));
				SetPeriodInput(p_dlg, CTL_VIEWPRGN_PRDPRGN, &prd);
				p_dlg->setCtrlData(CTL_VIEWPRGN_PROGNOSIS, &predict);
				p_dlg->disableCtrls(1, CTL_VIEWPRGN_GOODS, CTL_VIEWPRGN_PRDPRGN, CTL_VIEWPRGN_PROGNOSIS, 0);
				ExecViewAndDestroy(p_dlg);
			}
		}
	}
	return -1;
}
//
//
//
int SLAPI PPViewGoodsRest::ConvertLinesToBasket()
{
	int    ok = -1, r = 0;
	int    convert = BIN(Filt.Sgg == sggNone);
	if(convert) {
		SelBasketParam param;
		SETFLAG(param.Flags, SelBasketParam::fEnableFillUpToMinStock, Filt.Flags & (GoodsRestFilt::fUnderMinStock|GoodsRestFilt::fShowMinStock));
		THROW(r = GetBasketByDialog(&param, GetSymb()));
		if(r > 0) {
			GoodsRestViewItem item;
			PPWait(1);
			for(InitIteration(PPViewGoodsRest::OrdByGoodsName); NextIteration(&item) > 0;) {
				if(!(item.GoodsID & GOODSSUBSTMASK)) {
					int    fill_to_min_stock = BIN(param.Flags & SelBasketParam::fFillUpToMinStock);
					double qtty = 0;
					if(param.Flags & SelBasketParam::fFillUpToMinStock) {
						GoodsStockExt gse;
						if(GObj.GetStockExt(item.GoodsID, &gse, 1 /* useCache */) > 0)
							qtty = gse.GetMinStock(item.LocID) - fabs(item.Rest);
					}
					else
						qtty = fabs(item.Rest);
					if(!fill_to_min_stock || qtty > 0.0) {
						ILTI   i_i;
						ReceiptTbl::Rec lot_rec;
						// @v10.6.4 MEMSZERO(lot_rec);
						THROW(::GetCurGoodsPrice(item.GoodsID, item.LocID, GPRET_MOSTRECENT, 0, &lot_rec) != GPRET_ERROR);
						i_i.GoodsID     = item.GoodsID;
						i_i.UnitPerPack = item.UnitPerPack;
						i_i.Cost        = R5(lot_rec.Cost);
						if(param.SelPrice == 1)
							i_i.Price = lot_rec.Cost;
						else if(param.SelPrice == 2)
							i_i.Price = lot_rec.Price;
						else if(param.SelPrice == 3)
							i_i.Price = lot_rec.Price;
						else
							i_i.Price = lot_rec.Price;
						i_i.Price    = R5(i_i.Price);
						i_i.CurPrice = 0.0;
						i_i.Flags    = 0;
						i_i.Suppl    = lot_rec.SupplID;
						i_i.QCert    = lot_rec.QCertID;
						i_i.Expiry   = lot_rec.Expiry;
						i_i.Quantity = qtty;
						i_i.Rest     = i_i.Quantity;
						THROW(param.Pack.AddItem(&i_i, 0, param.SelReplace));
					}
				}
				PPWaitPercent(GetCounter());
			}
			PPWait(0);
			THROW(GoodsBasketDialog(param, 1));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int SLAPI PPViewGoodsRest::OnExecBrowser(PPViewBrowser * pBrw)
{
	int    disable_group_selection = 0;
	PPAccessRestriction accsr;
	if(ObjRts.GetAccessRestriction(accsr).OnlyGoodsGrpID && accsr.CFlags & PPAccessRestriction::cfStrictOnlyGoodsGrp) {
		disable_group_selection = 1;
	}
	if(!disable_group_selection)
		pBrw->SetupToolbarCombo(PPOBJ_GOODSGROUP, Filt.GoodsGrpID, OLW_CANSELUPLEVEL, 0);
	return -1;
}

void SLAPI PPViewGoodsRest::GetEditIds(const void * pRow, PPID * pLocID, PPID * pGoodsID, long col)
{
	PPID   loc_id   = 0;
	PPID   goods_id = 0;
	if(pRow) {
		if(P_Ct) {
			uint   tab_idx = col;
			int    r = (tab_idx > 0) ? P_Ct->GetTab(tab_idx - 1, &loc_id) : 1;
			if(r > 0)
				P_Ct->GetIdxFieldVal(0, pRow, &goods_id, sizeof(goods_id));
		}
		else {
			BrwHdr hdr = *static_cast<const BrwHdr *>(pRow);
			loc_id   = hdr.LocID;
			goods_id = hdr.GoodsID;
		}
	}
	ASSIGN_PTR(pLocID, loc_id);
	ASSIGN_PTR(pGoodsID, goods_id);
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, void * extraPtr)
{
	PPViewGoodsRest * p_view = static_cast<PPViewGoodsRest *>(extraPtr);
	return p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pCellStyle) : -1;
}

int SLAPI PPViewGoodsRest::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle)
{
	int    ok = -1;
	if(pData && pCellStyle && col >= 0) {
		const TagFilt & r_tag_filt = GObj.GetConfig().TagIndFilt;
		if(Filt.Flags & GoodsRestFilt::fShowGoodsMatrixBelongs || !r_tag_filt.IsEmpty()) {
			int    is_crosst = IsCrosstab();
			int    accept = 0;
			PPID   goods_id = 0, loc_id = 0;
			if((col == 0 && is_crosst == 0) || (col && is_crosst)) {
				GetEditIds(pData, &loc_id, &goods_id, col);
				if(goods_id && (!is_crosst || loc_id))
					accept = 1;
			}
			if(accept) {
				if(Filt.Flags & GoodsRestFilt::fShowGoodsMatrixBelongs) {
					if(GObj.P_Tbl->BelongToMatrix(goods_id, loc_id) > 0)
						ok = pCellStyle->SetLeftTopCornerColor(GetColorRef(SClrGreen));
					else
						ok = pCellStyle->SetLeftTopCornerColor(GetColorRef(SClrRed));
				}
				if(!r_tag_filt.IsEmpty()) {
					SColor clr;
					if(r_tag_filt.SelectIndicator(goods_id, clr) > 0)
						ok = pCellStyle->SetLeftBottomCornerColor(static_cast<COLORREF>(clr));
				}
			}
		}
	}
	return ok;
}

/*virtual*/void SLAPI PPViewGoodsRest::PreprocessBrowser(PPViewBrowser * pBrw)
{
	uint   brw_id = 0;
	int    deficit_col = 0, minstock_col = 0, draft_rcpt_col = 0;
	const long   fmt_qtty = MKSFMTD(0, 3, NMBF_NOZERO | NMBF_NOTRAILZ | ALIGN_RIGHT);
	const long   fmt_pct  = MKSFMTD(0, 2, NMBF_NOZERO | ALIGN_RIGHT);
	const long   fmt_amt  = MKSFMTD(0, 2, NMBF_NOZERO | ALIGN_RIGHT);
	SString temp_buf;
	CALLPTRMEMB(pBrw, SetTempGoodsGrp(Filt.GoodsGrpID));
	if(P_Ct == 0) {
		if(Filt.Flags2 & GoodsRestFilt::f2CalcPrognosis) { // @v9.5.8 CalcPrognosis-->Flags2
			if(Filt.Flags & (GoodsRestFilt::fCalcOrder | GoodsRestFilt::fNoZeroOrderOnly)) {
				brw_id = BROWSER_GOODSRESTORDER_PRGN;
				deficit_col = minstock_col = 6;
			}
			else
				deficit_col = minstock_col = 5;
		}
		else if(Filt.Flags & (GoodsRestFilt::fCalcOrder|GoodsRestFilt::fNoZeroOrderOnly))
			deficit_col = minstock_col = 4;
		else if(Filt.Flags & GoodsRestFilt::fBarCode)
			deficit_col = minstock_col = 3;
		else if(DS.CheckExtFlag(ECF_GOODSRESTPACK))
			deficit_col = minstock_col = 2;
		else
			deficit_col = minstock_col = 3;
		draft_rcpt_col = deficit_col;
		if(Flags & fAccsCost) {
			// @v10.5.8 pBrw->InsColumnWord(-1, PPWORD_PCTADDEDVAL, 17, 0, fmt_pct, 0);
			pBrw->InsColumn(-1, "@extrachargepct",  17, 0, fmt_pct, 0); // @v10.5.8
		}
		if(Filt.Flags & GoodsRestFilt::fCalcCVat && Flags & fAccsCost)
			pBrw->InsColumn(-1, "@vatc", 27, 0, fmt_amt, 0);
		if(Filt.Flags & GoodsRestFilt::fCalcPVat)
			pBrw->InsColumn(-1, "@vatp", 28, 0, fmt_amt, 0);
		if(Filt.Flags & GoodsRestFilt::fCalcDeficit) {
			pBrw->InsColumn(deficit_col++, "@deficit", 18, 0, fmt_qtty, 0);
			pBrw->InsColumnWord(deficit_col, PPWORD_RESTWITHDEFICIT, 19, 0, fmt_qtty, 0);
		}
		if(Filt.Flags & GoodsRestFilt::fShowDraftReceipt) {
			pBrw->InsColumnWord(draft_rcpt_col++, PPWORD_DRAFTRCPT, 22, 0, fmt_qtty, 0);
			pBrw->InsColumnWord(draft_rcpt_col, PPWORD_TOTAL, 23, 0, fmt_qtty, 0);
		}
		if(Filt.Sgg)
			pBrw->InsColumnWord(-1, PPWORD_SUBSTASSCCOUNT, 20, 0, fmt_qtty, 0);
		if(!Filt.Sgg && Filt.Flags & GoodsRestFilt::fShowMinStock) {
			// @v10.5.9 pBrw->InsColumnWord(minstock_col, PPWORD_MINSTOCK, 21, 0, fmt_qtty, 0);
			pBrw->InsColumn(minstock_col, "@minstock", 21, 0, fmt_qtty, 0); // @v10.5.9
		}
		{
			int    _col = 1;
			if(!(Filt.Flags2 & GoodsRestFilt::f2CalcPrognosis) && Filt.Flags & GoodsRestFilt::fEachLocation) { // @v9.5.8 CalcPrognosis-->Flags2
				// @v9.0.2 pBrw->InsColumnWord(1, PPWORD_WAREHOUSE, 25, 0, 0, 0);
				pBrw->InsColumn(_col++, "@warehouse", 25, 0, 0, 0); // @v9.0.2
			}
			if(Filt.DiffParam & GoodsRestParam::_diffSerial || (Filt.DiffParam & GoodsRestParam::_diffLotTag && Filt.DiffLotTagID))
				pBrw->InsColumn(_col++, "@serial", 26, 0, 0, 0);
			// @v9.7.11 {
			if(Filt.DiffParam & GoodsRestParam::_diffExpiry) {
				pBrw->InsColumn(_col++, "@expirationdate", 30, 0, 0, 0);
			}
			// } @v9.7.11
		}
		if(Filt.ExtViewFlags & GoodsRestFilt::evfShowLastSaleDate)
			pBrw->InsColumn(-1, "@lastselldate", 29, 0, MKSFMT(0, DATF_DMY|ALIGN_RIGHT), 0);
	}
	CALLPTRMEMB(pBrw, SetCellStyleFunc(CellStyleFunc, this));
}

/*virtual*/DBQuery * SLAPI PPViewGoodsRest::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	if(!P_Tbl)
		return 0;
	long   ff = 0;
	IterOrder ord = PPViewGoodsRest::OrdByDefault;
	if(Filt.Flags & GoodsRestFilt::fDisplayWoPacks)
		ff = QTTYF_FRACTION | NMBF_NOZERO;
	else if(LConfig.Flags & CFGFLG_USEPACKAGE)
		ff = QTTYF_COMPLPACK | QTTYF_FRACTION | NMBF_NOZERO;
	else
		ff = QTTYF_FRACTION | NMBF_NOZERO;

	uint   brw_id = 0;
	TempGoodsRestTbl * tbl = 0;
	TempOrderTbl * p_ot = 0;
	DBQuery * q = 0;
	DBQ  * dbq = 0;
	DBE    cq;
	DBE    dbe_loc;
	DBE    dbe_pct_add;
	DBE    dbe_barcode;
	DBE    dbe_serial;
	DBE  * dbe_rest_total = 0;
	if(P_Ct == 0) {
		THROW_MEM(tbl = new TempGoodsRestTbl(P_Tbl->GetName()));
		if(Filt.Flags2 & GoodsRestFilt::f2CalcPrognosis) { // @v9.5.8 CalcPrognosis-->Flags2
			if(Filt.Flags & (GoodsRestFilt::fCalcOrder | GoodsRestFilt::fNoZeroOrderOnly))
				brw_id = BROWSER_GOODSRESTORDER_PRGN;
			else
				brw_id = BROWSER_GOODSREST_PRGN;
		}
		else {
			if(Filt.Flags & (GoodsRestFilt::fCalcOrder | GoodsRestFilt::fNoZeroOrderOnly))
				if(Filt.Flags & GoodsRestFilt::fBarCode)
					brw_id = BROWSER_BCGDSRESTORDER;
				else
					brw_id = BROWSER_GOODSRESTORDER;
			else
				if(Filt.Flags & GoodsRestFilt::fBarCode)
					brw_id = BROWSER_BCGDSREST;
				else if(DS.CheckExtFlag(ECF_GOODSRESTPACK))
					brw_id = BROWSER_GOODSREST_UPP;
				else
					brw_id = BROWSER_GOODSREST;
		}
		if(oneof4(ord, OrdByPrice, OrdByGrp_Price, OrdByBarCode, OrdByGrp_BarCode))
			THROW(CreateOrderTable(ord, &p_ot));
		THROW_MEM(q = new DBQuery);
		q->syntax |= DBQuery::t_select;
		q->addField(tbl->ID__);           //  #0

		q->addField(tbl->GoodsID);        //  #1
		q->addField(tbl->LocID);          //  #2
		q->addField(tbl->GoodsName);      //  #3
		{
			PPDbqFuncPool::InitStrPoolRefFunc(dbe_barcode, tbl->BarcodeSP, &StrPool);
			q->addField(/*tbl->BarCode*/dbe_barcode); //  #4 // @v9.8.3 tbl->BarCode-->dbe_barcode
		}
		q->addField(tbl->Quantity);       //  #5
		q->addField(tbl->PhQtty);         //  #6
		q->addField(tbl->Ord);            //  #7
		q->addField(tbl->Predict);        //  #8
		q->addField(tbl->UnitPerPack);    //  #9
		{
			cq.init();
			cq.push(tbl->Quantity);
			cq.push(tbl->UnitPerPack);
			DBConst dbc_long;
			dbc_long.init(1L);
			cq.push(dbc_long); // Trust
			dbc_long.init(ff);
			cq.push(dbc_long); // Formatting flags
			cq.push(static_cast<DBFunc>(PPDbqFuncPool::IdCQtty));
			q->addField(cq);             // #10
		}
		{
			cq.init();
			cq.push(tbl->Predict);
			cq.push(tbl->UnitPerPack);
			cq.push(tbl->IsPredictTrust);
			DBConst dbc_long;
			dbc_long.init(ff);
			cq.push(dbc_long); // Formatting flags
			cq.push(static_cast<DBFunc>(PPDbqFuncPool::IdCQtty));
			q->addField(cq);             // #11
		}
		q->addField(tbl->Cost);            // #12
		q->addField(tbl->Price);           // #13
		q->addField(tbl->SumCost);         // #14
		q->addField(tbl->SumPrice);        // #15
		q->addField(tbl->RestInDays);      // #16
		{
			if(Flags & fAccsCost)
				PPDbqFuncPool::InitPctFunc(dbe_pct_add, tbl->Price, tbl->Cost, 2);
			else
				PPDbqFuncPool::InitPctFunc(dbe_pct_add, tbl->Price, tbl->Price, 2);
			// @v9.8.2 q->addField(tbl->PctAddedVal);     // #17
			q->addField(dbe_pct_add);            // #17 @v9.8.2
		}
		q->addField(tbl->Deficit);         // #18
		q->addField(tbl->QttyWithDeficit); // #19
		q->addField(tbl->SubstAsscCount);  // #20
		q->addField(tbl->MinStock);        // #21
		{
			cq.init();
			cq.push(tbl->DraftRcpt);
			cq.push(tbl->UnitPerPack);
			DBConst dbc_long;
			dbc_long.init(1L);
			cq.push(dbc_long); // Trust
			dbc_long.init(ff);
			cq.push(dbc_long); // Formatting flags
			cq.push(static_cast<DBFunc>(PPDbqFuncPool::IdCQtty));
			q->addField(cq);             // #22
		}
		{
			dbe_rest_total = &(tbl->Quantity + tbl->DraftRcpt);
			if(ff & QTTYF_COMPLPACK) {
				cq.init();
				cq.push(*dbe_rest_total);
				cq.push(tbl->UnitPerPack);
				DBConst dbc_long;
				dbc_long.init(1L);
				cq.push(dbc_long); // Trust
				dbc_long.init(ff);
				cq.push(dbc_long); // Formatting flags
				cq.push(static_cast<DBFunc>(PPDbqFuncPool::IdCQtty));
				q->addField(cq);               // #23
			}
			else
				q->addField(*dbe_rest_total);  // #23
		}
		q->addField(tbl->SStatSales);      // #24
		if(Filt.Flags & GoodsRestFilt::fEachLocation) {
			PPDbqFuncPool::InitObjNameFunc(dbe_loc, PPDbqFuncPool::IdObjNameLoc, tbl->LocID);
			q->addField(dbe_loc);          // #25
		}
		else {
			q->addField(tbl->ID__);        // #25 @stub
		}
		{
			PPDbqFuncPool::InitStrPoolRefFunc(dbe_serial, tbl->SerialSP, &StrPool);
			q->addField(/*tbl->Serial*/dbe_serial);  // #26 // @v9.8.3 tbl->Serial-->dbe_serial
		}
		q->addField(tbl->SumCVat);         // #27
		q->addField(tbl->SumPVat);         // #28
		q->addField(tbl->LastSellDate);    // #29
		q->addField(tbl->Expiry);          // #30 @v9.7.11
		if(Filt.ExhaustTerm > 0) {
			dbq = &(*dbq && tbl->RestInDays >= 0L && tbl->RestInDays <= (long)Filt.ExhaustTerm);
		}
		else if(Filt.ExhaustTerm < 0) {
			dbq = &(*dbq && tbl->RestInDays >= (long)Filt.ExhaustTerm && tbl->RestInDays < 0L);
		}
		if(p_ot) {
			dbq = & (*dbq && p_ot->ID == tbl->ID__);
			q->from(p_ot, tbl, 0L).where(*dbq).orderBy(p_ot->Name, 0L);
		}
		else {
			q->from(tbl, 0L).where(*dbq).orderBy(tbl->GoodsName, tbl->LocID, 0L);
		}
		THROW(CheckQueryPtr(q));
	}
	else {
		brw_id = BROWSER_GOODSREST_CROSSTAB;
		q = PPView::CrosstabDbQueryStub;
	}
	if(pSubTitle) {
		SString loc_names;
		*pSubTitle = 0;
		if(Filt.Flags2 & GoodsRestFilt::f2CalcPrognosis) // @v9.5.8 CalcPrognosis-->Flags2
			pSubTitle->Cat(Filt.PrgnPeriod);
		else {
			if(Filt.Date != ZERODATE)
				pSubTitle->CatDivIfNotEmpty('-', 1).Cat(Filt.Date, DATF_DMY|DATF_CENTURY);
			pSubTitle->CatDivIfNotEmpty('-', 1).Cat(GetExtLocationName(Filt.LocList, 2, loc_names));
			if(Filt.SupplID)
				GetObjectName(PPOBJ_ARTICLE, Filt.SupplID, pSubTitle->CatDivIfNotEmpty('-', 1), 1);
			if(Filt.GoodsGrpID)
				GetObjectName(PPOBJ_GOODSGROUP, Filt.GoodsGrpID, pSubTitle->CatDivIfNotEmpty('-', 1), 1);
		}
	}
	// }
	CATCH
		if(q) {
			ZDELETE(q);
		}
		else {
			delete tbl;
			delete p_ot;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	if(!(ff & QTTYF_COMPLPACK))
		delete dbe_rest_total;
	return q;
}

/*virtual*/int SLAPI PPViewGoodsRest::ViewTotal()
{
	class GoodsRestTotalDialog : public AmtListDialog {
	public:
		GoodsRestTotalDialog(GoodsRestTotal * pTotal, PPViewGoodsRest * pV) :
			AmtListDialog(DLG_GRESTTOTAL, CTL_GRESTTOTAL_AMTLIST, 0, &pTotal->Amounts, 0, 0, 0), P_Total(pTotal), P_V(pV)
		{
			setCtrlData(CTL_GRESTTOTAL_COUNT,  &pTotal->Count);
			setCtrlData(CTL_GRESTTOTAL_REST,   &pTotal->Quantity);
			setCtrlData(CTL_GRESTTOTAL_PHREST, &pTotal->PhQtty);
			setCtrlData(CTL_GRESTTOTAL_COST,   &pTotal->SumCost);
			setCtrlData(CTL_GRESTTOTAL_PRICE,  &pTotal->SumPrice);
			setCtrlReal(CTL_GRESTTOTAL_CVAT,    pTotal->SumCVat);
			setCtrlReal(CTL_GRESTTOTAL_PVAT,    pTotal->SumPVat);
			setCtrlData(CTL_GRESTTOTAL_PCTADD, &pTotal->PctAddedVal);
			setCtrlData(CTL_GRESTTOTAL_DRAFTRCPT,  &pTotal->DraftRcpt);
			setCtrlData(CTL_GRESTTOTAL_DRAFCOST,   &pTotal->SumDraftCost);
			setCtrlData(CTL_GRESTTOTAL_DRAFPRICE,  &pTotal->SumDraftPrice);
		}
	protected:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmPrint) && P_V) {
				P_V->PrintTotal(P_Total);
				clearEvent(event);
			}
		}
		PPViewGoodsRest * P_V;
		GoodsRestTotal  * P_Total;
	};
	GoodsRestTotal total;
	PPWait(1);
	if(GetTotal(&total)) {
		PPWait(0);
		GoodsRestTotalDialog * dlg = new GoodsRestTotalDialog(&total, this);
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
	}
	PPWait(0);
	return 1;
}

int SLAPI PPViewGoodsRest::ExportUhtt(int silent)
{
	int    ok = -1;
	const  PPID src_loc_id = NZOR(Filt.UhttExpLocID, Filt.LocList.GetSingle());
	SString msg_buf, fmt_buf, name_buf, temp_buf;
	PPLogger logger;
	if(src_loc_id) {
		long   exp_options = 0;
		SETFLAG(exp_options, GoodsRestFilt::uefRest, Filt.UhttExpFlags & GoodsRestFilt::uefRest);
		SETFLAG(exp_options, GoodsRestFilt::uefPrice, Filt.UhttExpFlags & GoodsRestFilt::uefPrice);
		SETFLAG(exp_options, GoodsRestFilt::uefZeroAbsPrices, Filt.UhttExpFlags & GoodsRestFilt::uefZeroAbsPrices);
		if(silent) {
			SETIFZ(exp_options, GoodsRestFilt::uefRest);
		}
		else {
			TDialog * dlg = 0;
			long   _s = 0;
			long   _f = 0;
			if((exp_options & (GoodsRestFilt::uefRest|GoodsRestFilt::uefPrice)) == (GoodsRestFilt::uefRest|GoodsRestFilt::uefPrice))
				_s = 3;
			else if(exp_options & GoodsRestFilt::uefPrice)
				_s = 2;
			else if(exp_options & GoodsRestFilt::uefRest)
				_s = 1;
			if(exp_options & GoodsRestFilt::uefZeroAbsPrices)
				_f |= 0x0001;

			THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_GRESTUHTTEXP))));
			dlg->AddClusterAssocDef(CTL_GRESTUHTTEXP_F, 0, 1);
			dlg->AddClusterAssoc(CTL_GRESTUHTTEXP_F, 1, 2);
			dlg->AddClusterAssoc(CTL_GRESTUHTTEXP_F, 2, 3);
			dlg->SetClusterData(CTL_GRESTUHTTEXP_F, _s);

			dlg->AddClusterAssoc(CTL_GRESTUHTTEXP_FLAGS, 0, 0x0001);
			dlg->SetClusterData(CTL_GRESTUHTTEXP_FLAGS, _f);
			//@erik v10.7.13 {
			dlg->AddClusterAssoc(CTL_GRESTUHTTEXP_CH, 0, PPViewGoodsRest::expDirUhtt);
			dlg->AddClusterAssoc(CTL_GRESTUHTTEXP_CH, 1, PPViewGoodsRest::expDirVkMarket);
			dlg->SetClusterData(CTL_GRESTUHTTEXP_CH, PPViewGoodsRest::expDirUhtt);
			// } @erik v10.7.13
			if(ExecView(dlg) == cmOK) {
				//@erik v10.7.13 {
				long exp_direction;
				dlg->GetClusterData(CTL_GRESTUHTTEXP_CH, &exp_direction); // @erik v10.7.13
				if(exp_direction==PPViewGoodsRest::expDirVkMarket) {
					TSVector<BrwHdr> uniq_id_and_loc_id_list;
					PPVkClient vk_client;
					SString img_path;
					ObjLinkFiles lf(PPOBJ_GOODS);
					PPWait(1);
					{
						GoodsRestViewItem item;
						for(InitIteration(OrdByDefault); NextIteration(&item)>0;){
							BrwHdr goods_rest_item_header = {0, item.GoodsID, item.LocID};
							if(!uniq_id_and_loc_id_list.lsearch(&goods_rest_item_header, 0, PTR_CMPFUNC(_2long), sizeof(PPID)))
								uniq_id_and_loc_id_list.insert(&goods_rest_item_header);
						}	
					}
					if(uniq_id_and_loc_id_list.getCount()) {
						VkStruct vk_struct;
						PPObjGlobalUserAcc gua_obj;
						PPGlobalUserAcc gua_rec;
						PPGlobalUserAccPacket gua_pack;
						Reference ref;
						PPObjTag obj_tag;
						PPID vk_grp_tag_id, vk_page_tag_id, tag_vk_goods_id;
						obj_tag.FetchBySymb("SMGRPID", &vk_grp_tag_id);
						obj_tag.FetchBySymb("SMPAGEID", &vk_page_tag_id);
						obj_tag.FetchBySymb("VK_GOODS_ID", &tag_vk_goods_id);
						gua_obj.SearchBySymb("vk_acc", 0, &gua_rec);
						if(gua_obj.GetPacket(gua_rec.ID, &gua_pack)>0) {
							if(gua_pack.TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, vk_struct.Token)>0) {
								if(gua_pack.TagL.GetItemStr(vk_page_tag_id, vk_struct.PageId)>0) {
									if(gua_pack.TagL.GetItemStr(vk_grp_tag_id, vk_struct.GroupId)>0) {
										BrwHdr goods_rest_item_hdr;
										Goods2Tbl::Rec goods_rec; 
										GoodsRestViewItem rest_item;
										PPGoodsPacket pack;
										ObjTagItem stored_obj_tag_item;
										LongArray stored_vk_goods_id_list;
										THROW(vk_client.Market_Get(vk_struct, temp_buf));
										THROW_SL(vk_client.ParceGoodsItemList(temp_buf, stored_vk_goods_id_list));
										for(uint i = 0; i<uniq_id_and_loc_id_list.getCount(); i++) {
											goods_rest_item_hdr = uniq_id_and_loc_id_list.at(i);
											if(GObj.Search(goods_rest_item_hdr.GoodsID, &goods_rec) > 0) {
												GetItem(goods_rest_item_hdr.GoodsID, goods_rest_item_hdr.LocID, &rest_item);
												ref.Ot.GetTag(PPOBJ_GOODS, goods_rest_item_hdr.GoodsID, tag_vk_goods_id, &stored_obj_tag_item);
												lf.Load(goods_rest_item_hdr.GoodsID, 0L);
												lf.At(0, img_path.Z());
												if(img_path.NotEmptyS()) {
													vk_struct.LinkFilePath = img_path;
													vk_struct.LinkFileType = 1;
												}
												else {
													logger.Log(PPFormatT(PPTXT_LOG_UHTT_GOODSNOIMG, &msg_buf, goods_rest_item_hdr.GoodsID)); // PPTXT_LOG_UHTT_GOODSNOIMG "Для товара @goods нет изображения"
													img_path = vk_struct.LinkFilePath = "D:\\Papyrus\\ppy\\DD\\nophoto.png";
													vk_struct.LinkFileType = 1;
													if(!fileExists(img_path)) {
														logger.Log(PPFormatT(PPTXT_LOG_VK_NODEFIMG, &msg_buf, goods_rest_item_hdr.GoodsID)); // "Изображение по-умолчанию не найдено!"
														continue;
													}
												}

												pack.Rec.ID = goods_rest_item_hdr.GoodsID; // @trick
												pack.Rec.Flags |= GF_EXTPROP; // @trick
												if(GObj.GetValueAddedData(goods_rest_item_hdr.GoodsID, &pack)>0) {
													if(pack.GetExtStrData(GDSEXSTR_A, temp_buf)>0)
														msg_buf.Z().Cat(temp_buf);
													else {
														logger.Log(PPFormatT(PPTXT_LOG_VK_NODESCRFORGOODS, &msg_buf, goods_rest_item_hdr.GoodsID)); // "Изображение по-умолчанию не найдено!"
														continue;
													}
												}
												else {
													logger.Log(PPFormatT(PPTXT_LOG_VK_ADDFIELDSFORGOODSPROBLEM, &msg_buf, goods_rest_item_hdr.GoodsID)); // "Изображение по-умолчанию не найдено!"
													continue;
												}

												PPID vk_goods_id = 0;
												stored_obj_tag_item.GetInt(&vk_goods_id);
												if(stored_vk_goods_id_list.lsearch(vk_goods_id) <= 0) {
													vk_goods_id = 0;
												}
												if(vk_client.AddGoodToMarket(vk_struct, goods_rec, msg_buf, rest_item.Price, 0.0, vk_goods_id, temp_buf.Z()) > 0) {
													json_t * p_json_doc = 0;
													THROW_SL(json_parse_document(&p_json_doc, temp_buf.cptr())==JSON_OK);
													for(json_t * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
														if(p_cur->Type==json_t::tOBJECT) {
															for(const json_t * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
																if(p_obj->Text.IsEqiAscii("response")) {
																	const json_t *p_response_node = p_obj->P_Child;
																	if(p_response_node->P_Child) {
																		for(const json_t * p_response_item = p_response_node->P_Child; p_response_item; p_response_item = p_response_item->P_Next) {
																			if(p_response_item->Text.IsEqiAscii("market_item_id")) {
																				temp_buf = p_response_item->P_Child->Text.Unescape();
																				ObjTagItem new_obj_tag_item;
																				new_obj_tag_item.SetStr(tag_vk_goods_id, temp_buf);
																				if(new_obj_tag_item!=stored_obj_tag_item)
																					ref.Ot.PutTag(PPOBJ_GOODS, goods_rec.ID, &new_obj_tag_item, 0);
																			}
																		}
																	}
																}
															}
														}
													}
													ZDELETE(p_json_doc);
												}
												PPWaitPercent(i, uniq_id_and_loc_id_list.getCount());
											}
										}
									}
									else {
										logger.Log(PPFormatT(PPTXT_LOG_VK_NOGROUPIDINGUA, &msg_buf));
									}
								}
								else {
									logger.Log(PPFormatT(PPTXT_LOG_VK_NOPAGEIDINGUA, &msg_buf));
								}
							}
							else {
								logger.Log(PPFormatT(PPTXT_LOG_VK_NOTOKENINGUA, &msg_buf));
							}
						}
						else {
							logger.Log(PPFormatT(PPTXT_LOG_VK_NOGUA, &msg_buf));
						}
						PPWait(0);
					}
				}
				else {   // } @erik v10.7.13
					_s = dlg->GetClusterData(CTL_GRESTUHTTEXP_F);
					exp_options = 0;
					if(_s==1)
						exp_options |= GoodsRestFilt::uefRest;
					else if(_s==2)
						exp_options |= GoodsRestFilt::uefPrice;
					else if(_s==3)
						exp_options |= (GoodsRestFilt::uefPrice|GoodsRestFilt::uefRest);
					else
						exp_options |= GoodsRestFilt::uefRest;

					_f = dlg->GetClusterData(CTL_GRESTUHTTEXP_FLAGS);
					if(_f&0x0001)
						exp_options |= GoodsRestFilt::uefZeroAbsPrices;
				}
			}
			else
				exp_options = 0;
			delete dlg;
		}
		if(exp_options & (GoodsRestFilt::uefPrice|GoodsRestFilt::uefRest|GoodsRestFilt::uefZeroAbsPrices)) {
			PPID   suppl_id = Filt.SupplID;
			int    uhtt_src_loc_id = 0;
			int    uhtt_dlvr_loc_id = 0;
			GoodsRestViewItem view_item;
			BarcodeArray bc_list;
			PPLocationPacket loc_pack;
			UhttBillPacket uhtt_pack;
			UhttLocationPacket uhtt_loc_pack;
			PPUhttClient uhtt_cli;
			THROW(uhtt_cli.Auth());
			THROW(LocObj.Fetch(src_loc_id, &loc_pack) > 0);
			THROW_PP_S(!isempty(loc_pack.Code), PPERR_LOCSYMBUNDEF, loc_pack.Name);
			THROW(uhtt_cli.GetLocationByCode(loc_pack.Code, uhtt_loc_pack) > 0);
			uhtt_src_loc_id = uhtt_loc_pack.ID;
			if(exp_options & GoodsRestFilt::uefRest) {
				if(suppl_id) {
					PPID suppl_psn_id = ObjectToPerson(suppl_id);
					if(suppl_psn_id) {
						PPObjPerson psn_obj;
						PPPersonPacket psn_pack;
						if(psn_obj.GetPacket(suppl_psn_id, &psn_pack, 0) > 0) {
							if(psn_pack.Loc.Code[0] && uhtt_cli.GetLocationByCode(psn_pack.Loc.Code, uhtt_loc_pack) > 0) {
								uhtt_dlvr_loc_id = uhtt_loc_pack.ID;
							}
							else if(psn_pack.RLoc.Code[0] && uhtt_cli.GetLocationByCode(psn_pack.RLoc.Code, uhtt_loc_pack) > 0) {
								uhtt_dlvr_loc_id = uhtt_loc_pack.ID;
							}
							else {
								for(uint dlp = 0; !uhtt_dlvr_loc_id && psn_pack.EnumDlvrLoc(&dlp, &loc_pack);) {
									if(loc_pack.Code[0] && uhtt_cli.GetLocationByCode(loc_pack.Code, uhtt_loc_pack) > 0) {
										uhtt_dlvr_loc_id = uhtt_loc_pack.ID;
									}
								}
							}
						}
					}
				}
				if(!uhtt_dlvr_loc_id) {
					if(uhtt_cli.GetLocationByCode("ADDRESS-ZERO", uhtt_loc_pack) > 0) {
						uhtt_dlvr_loc_id = uhtt_loc_pack.ID;
					}
				}
				//
				uhtt_pack.LocID = uhtt_src_loc_id;
				uhtt_pack.ArID = 0;
				uhtt_pack.DlvrLocID = uhtt_dlvr_loc_id;
				uhtt_pack.Code = "TEST-GOODSREST";
				uhtt_pack.Dtm = NZOR(Filt.Date, getcurdate_());
				uhtt_pack.OpSymb = "GOODSREST";
			}
			PPWait(1);
			{
				LongArray uniq_id_list;
				StrAssocArray code_ref_list;
				LAssocArray ref_list; // Key - papyrus id, Val - uhtt id
				for(InitIteration(); NextIteration(&view_item) > 0;) {
					uniq_id_list.add(labs(view_item.GoodsID));
				}
				uniq_id_list.sortAndUndup();
				if(uniq_id_list.getCount()) {
					for(uint i = 0; i < uniq_id_list.getCount(); i++) {
						ref_list.Add(uniq_id_list.get(i), 0, 0);
					}
					THROW(uhtt_cli.GetUhttGoodsRefList(ref_list, &code_ref_list));
					ref_list.Sort();
				}
				if(ref_list.getCount()) {
					TSCollection <UhttQuotPacket> uhtt_quot_list;
					PPIDArray report_goods_list;
					for(InitIteration(); NextIteration(&view_item) > 0;) {
						const  PPID goods_id = labs(view_item.GoodsID);
						long   uhtt_goods_id = 0;
						report_goods_list.add(goods_id);
						if(ref_list.BSearch(goods_id, &uhtt_goods_id, 0) && uhtt_goods_id > 0) {
							if(exp_options & GoodsRestFilt::uefRest) {
								UhttBillPacket::BillItem uhtt_bill_item;
								uhtt_bill_item.BillID = 0;
								uhtt_bill_item.GoodsID = uhtt_goods_id;
								uhtt_bill_item.Quantity = view_item.Rest;
								uhtt_bill_item.Cost = view_item.Cost;
								uhtt_bill_item.Price = view_item.Price;
								uhtt_bill_item.Discount = 0.0;
								uhtt_bill_item.Amount = 0.0;
								THROW_SL(uhtt_pack.Items.insert(&uhtt_bill_item));
							}
							if(exp_options & GoodsRestFilt::uefPrice) {
								UhttQuotPacket * p_uhtt_qp = uhtt_quot_list.CreateNewItem();
								THROW_SL(p_uhtt_qp);
								p_uhtt_qp->GoodsID = uhtt_goods_id;
								p_uhtt_qp->LocID = uhtt_src_loc_id;
								p_uhtt_qp->Value = view_item.Price;
								if(view_item.Price == 0.0 && Filt.Flags & GoodsRestFilt::fPriceByQuot && Filt.QuotKindID > 0) {
									//
									// Если цена нулевая и определена по котировке, то необходимо убедиться, что
									// на товар действительно установлена нулевая котировка, и, если это - так,
									// то передать на сервер ноль вместе с флагом PPQuot::fZero.
									//
									const QuotIdent qi(src_loc_id, Filt.QuotKindID);
									double test_price = 0.0;
									if(GObj.GetQuotExt(goods_id, qi, &test_price, 1) > 0 && test_price == 0.0)
										p_uhtt_qp->Flags |= PPQuot::fZero;
								}
								/*
								if(!uhtt_cli.SetQuot(uhtt_qp)) {
									PPGetMessage(mfError, PPERR_UHTT_SETQUOTFAULT, 0, 1, fmt_buf);
									(temp_buf = uhtt_cli.GetLastMessage()).Space().CatChar('\'').Cat(view_item.GoodsName).CatChar('\'');
									msg_buf.Printf(fmt_buf, temp_buf.cptr());
									logger.Log(msg_buf);
								}
								*/
							}
						}
						PPWaitPercent(GetCounter());
					}
					if(exp_options & GoodsRestFilt::uefZeroAbsPrices) {
						UhttQuotFilter ex_quot_filt;
						TSCollection <UhttQuotPacket> ex_uhtt_quot_list;
						ex_quot_filt.LocationID = uhtt_src_loc_id;
						int r = uhtt_cli.GetQuot(ex_quot_filt, ex_uhtt_quot_list);
						if(r > 0 && ex_uhtt_quot_list.getCount()) {
							for(uint i = 0; i < ex_uhtt_quot_list.getCount(); i++) {
								const UhttQuotPacket * p_uhtt_item = ex_uhtt_quot_list.at(i);
								PPID   local_goods_id = 0;
								if(!(ref_list.SearchByVal(p_uhtt_item->GoodsID, &local_goods_id, 0) && local_goods_id > 0) || !uniq_id_list.bsearch(local_goods_id)) {
									//
									// @paranoic {
									// Технически это не возможно, но проверим нет ли среди уже
									// готовых элементов массива uhtt_quot_list того, у которого GoodsID равен добавляемому
									//
									int    s = 0;
									for(uint j = 0; !s && j < uhtt_quot_list.getCount(); j++) {
										const UhttQuotPacket * p_s_item = uhtt_quot_list.at(j);
										if(p_s_item && p_s_item->GoodsID == p_uhtt_item->GoodsID)
											s = 1;
									}
									assert(s == 0);
									// } @paranoic
									if(!s) {
										UhttQuotPacket * p_uhtt_qp = uhtt_quot_list.CreateNewItem();
										THROW_SL(p_uhtt_qp);
										p_uhtt_qp->GoodsID = p_uhtt_item->GoodsID;
										p_uhtt_qp->LocID = uhtt_src_loc_id;
										p_uhtt_qp->Value = 0.0;
									}
								}
							}
						}
					}
					if(exp_options & (GoodsRestFilt::uefPrice|GoodsRestFilt::uefZeroAbsPrices) && uhtt_quot_list.getCount()) {
						TSCollection <UhttStatus> set_quot_result_list;
                        if(uhtt_cli.SetQuotList(uhtt_quot_list, set_quot_result_list)) {
							if(set_quot_result_list.getCount() > 1) {
								PPGetMessage(mfError, PPERR_UHTT_SETQUOTFAULT, 0, 1, fmt_buf);
								for(uint i = 1; i < set_quot_result_list.getCount(); i++) {
									const UhttStatus * p_status = set_quot_result_list.at(i);
									if(p_status) {
										if(p_status->Id)
											GetGoodsName(p_status->Id, name_buf);
										else
											name_buf.Z().CatEq("goodsid", 0L);
										(temp_buf = p_status->Msg).Space().CatChar('\'').Cat(name_buf).CatChar('\'');
										msg_buf.Printf(fmt_buf, temp_buf.cptr());
										logger.Log(msg_buf);
									}
								}
							}
                        }
                        else {
                        }
					}
					if(exp_options & GoodsRestFilt::uefRest) {
						THROW(uhtt_cli.CreateBill(uhtt_pack));
					}
				}
			}
			PPWait(0);
		}
	}
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

/*virtual*/int SLAPI PPViewGoodsRest::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    update = 0;
	int    ok = (ppvCmd != PPVCMD_DETAIL && ppvCmd != PPVCMD_PRINT) ? PPView::ProcessCommand(ppvCmd, pHdr, pBrw) : -2;
	if(ok == -2) {
		BrwHdr hdr;
		if(pHdr && ppvCmd != PPVCMD_INPUTCHAR)
			hdr = *static_cast<const PPViewGoodsRest::BrwHdr *>(pHdr);
		else
			MEMSZERO(hdr);
		switch(ppvCmd) {
			case PPVCMD_INPUTCHAR:
				ok = -1;
				if(pHdr && pBrw) {
					char   init_char = static_cast<const char *>(pHdr)[0];
					if(isdec(init_char)) {
						Goods2Tbl::Rec goods_rec;
						double qtty = 0.0;
						if(GObj.SelectGoodsByBarcode(init_char, Filt.SupplID, &goods_rec, &qtty, 0) > 0) {
							if(pBrw->search2(&goods_rec.ID, CMPF_LONG, srchFirst, sizeof(long)) > 0)
								ok = 1;
						}
					}
					else if(init_char == '*' || IsLetter866(init_char) || isalpha(init_char))
						ok = -2;
				}
				break;
			case PPVCMD_DETAIL:
				GetEditIds(pHdr, &hdr.LocID, &hdr.GoodsID, (pBrw) ? pBrw->GetCurColumn() : 0);
				ok = ViewLots(hdr.__ID, P_Ct ? &hdr : 0, 0);
				break;
			case PPVCMD_VIEWORDERLOTS:
				GetEditIds(pHdr, &hdr.LocID, &hdr.GoodsID, (pBrw) ? pBrw->GetCurColumn() : 0);
				ok = ViewLots(hdr.__ID, P_Ct ? &hdr : 0, 1);
				break;
			case PPVCMD_VIEWGOODSCARD:
				ok = -1;
				{
					GetEditIds(pHdr, &hdr.LocID, &hdr.GoodsID, (pBrw) ? pBrw->GetCurColumn() : 0);
					TrfrAnlzFilt flt;
					flt.GoodsID = hdr.GoodsID;
					if(hdr.LocID)
						flt.LocList.Add(hdr.LocID);
					else
						flt.LocList = LocList;
					flt.Flags  |= TrfrAnlzFilt::fGByDate;
					ViewTrfrAnlz(&flt);
				}
				break;
			case PPVCMD_EDITGOODS:
				ok = -1;
				{
					GetEditIds(pHdr, &hdr.LocID, &hdr.GoodsID, (pBrw) ? pBrw->GetCurColumn() : 0);
					Goods2Tbl::Rec goods_rec;
					if(hdr.GoodsID && GObj.Fetch(hdr.GoodsID, &goods_rec) > 0) {
						if(goods_rec.Kind == PPGDSK_PCKGTYPE) {
							PPObjPckgType pt_obj;
							if(pt_obj.Edit(&hdr.GoodsID, 0) == cmOK)
								update = ok = 1;
						}
						else if(GObj.Edit(&hdr.GoodsID, 0) == cmOK)
							update = ok = 1;
						if(update)
							UpdateGoods(hdr.GoodsID);
					}
				}
				break;
			case PPVCMD_PUTTOBASKET:
				{
					GetEditIds(pHdr, &hdr.LocID, &hdr.GoodsID, (pBrw) ? pBrw->GetCurColumn() : 0);
					double price_to_basket = 0.0;
					GoodsRestViewItem item;
					if(GetItem(hdr.__ID, &item) > 0) {
                        if(Filt.AmtType == 1)
							price_to_basket = item.Cost;
                        else if(Filt.AmtType == 2)
							price_to_basket = item.Price;
					}
					ok = AddGoodsToBasket(hdr.GoodsID, LocList.GetSingle(), 0.0, price_to_basket);
				}
				break;
			case PPVCMD_PUTTOBASKETALL:
				ok = ConvertLinesToBasket();
				break;
			case PPVCMD_VIEWOPGRPNG:
				ok = -1;
				GetEditIds(pHdr, &hdr.LocID, &hdr.GoodsID, (pBrw) ? pBrw->GetCurColumn() : 0);
				if(hdr.GoodsID) {
					PPViewOpGrouping v;
					SETIFZ(P_OpGrpngFilt, new OpGroupingFilt);
					if(P_OpGrpngFilt) {
						P_OpGrpngFilt->LocList.Set(0);
						P_OpGrpngFilt->LocList.Add(hdr.LocID);
						P_OpGrpngFilt->SupplID = Filt.SupplID;
						P_OpGrpngFilt->GoodsID = hdr.GoodsID;
						if(v.EditBaseFilt(P_OpGrpngFilt) > 0)
							ViewOpGrouping(P_OpGrpngFilt);
					}
					else
						PPError(PPERR_NOMEM);
				}
				break;
			case PPVCMD_VIEWFORECAST:
				GetEditIds(pHdr, &hdr.LocID, &hdr.GoodsID, (pBrw) ? pBrw->GetCurColumn() : 0);
				ok = ViewPrediction(hdr.GoodsID, hdr.LocID);
				break;
			case PPVCMD_VIEWSELLTAB:
				ok = -1;
				{
					GetEditIds(pHdr, &hdr.LocID, &hdr.GoodsID, (pBrw) ? pBrw->GetCurColumn() : 0);
					PredictSalesFilt flt;
					flt.GoodsID = hdr.GoodsID;
					if(hdr.LocID)
						flt.LocList.Add(hdr.LocID);
					else
						flt.LocList = LocList;
					DateRange pp = Filt.PrgnPeriod;
					if(pp.low && pp.upp) {
						flt.Cycle = (int16)diffdate(pp.upp, pp.low)+1;
						flt.Watershed = plusdate(pp.low, -1);
						flt.Period.upp = pp.upp;
					}
					ViewPredictSales(&flt);
				}
				break;
			case PPVCMD_SETCONTRACTPRICES:
				ok = SetContractPrices();
				break;
			case PPVCMD_VIEWQUOT:
			case PPVCMD_VIEWSUPPLQUOT:
			case PPVCMD_VIEWGOODSMATRIX:
				ok = -1;
				if(!Filt.Sgg) {
					GetEditIds(pHdr, &hdr.LocID, &hdr.GoodsID, (pBrw) ? pBrw->GetCurColumn() : 0);
					int    quot_cls = (ppvCmd == PPVCMD_VIEWSUPPLQUOT) ? PPQuot::clsSupplDeal :
						((ppvCmd == PPVCMD_VIEWGOODSMATRIX) ? PPQuot::clsMtx : PPQuot::clsGeneral);
					ok = GObj.EditQuotations(hdr.GoodsID, hdr.LocID, -1L, 0, quot_cls);
					const int quot_usage = (Filt.QuotKindID > 0) ? Filt.GetQuotUsage() : 0;
					if(ok > 0 && (quot_usage || (Filt.Flags & (GoodsRestFilt::fUseGoodsMatrix|GoodsRestFilt::fShowGoodsMatrixBelongs))))
						update = 1;
				}
				break;
			case PPVCMD_EXPORTUHTT:
				ok = -1;
				ExportUhtt(0);
				break;
			case PPVCMD_PRINT:
				{
					int use_option_dlg = 1;
					ok = Print(&use_option_dlg);
				}
				break;
			case PPVCMD_TB_CBX_SELECTED:
				ok = -1;
				{
					PPID   grp_id = 0;
					if(pBrw && pBrw->GetToolbarComboData(&grp_id) && Filt.GoodsGrpID != grp_id) {
						Filt.GoodsGrpID = grp_id;
						ok = ChangeFilt(1, pBrw);
					}
				}
				break;
			default:
				break;
		}
	}
	if(ok > 0 && update > 0)
		pBrw->Update();
	return ok;
}

int SLAPI ViewGoodsRest(const GoodsRestFilt * pFilt, int calcPrognosis) { return PPView::Execute(PPVIEW_GOODSREST, pFilt, 1, reinterpret_cast<void *>(BIN(calcPrognosis))); }

int SLAPI GoodsRestTest()
{
	int    ok = -1, iter_count = 0;
	PPIniFile ini_file;
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_GOODSRESTTEST, &iter_count);
	if(iter_count) {
		GoodsRestFilt gr_filt;
		gr_filt.Date = getcurdate_();
		for(int c = 0; c < iter_count; c++) {
			PPViewGoodsRest * p_v = new PPViewGoodsRest;
			p_v->Init_(&gr_filt);
			delete p_v;
		}
		ok = 1;
	}
	return ok;
}
//
// Printing
//
int SLAPI PPViewGoodsRest::PrintTotal(const GoodsRestTotal * pTotal)
{
	GoodsRestTotalPrintData grtpd;
	grtpd.P_Total = pTotal;
	grtpd.P_Filt = &Filt;
	PView pv(&grtpd);
	return PPAlddPrint(REPORT_GOODSRESTTOTAL, &pv, 0);
}

struct GoodsRestReport {
	enum { // flags
		fSortByPrice   = 0x0001,
		fPrintBarCode  = 0x0002,
		fInventory     = 0x0004,
		fInPacks       = 0x0008,
		fDisableGrpng  = 0x0010,
		fSkipRestData  = 0x0020,
		fSortByBarCode = 0x0040,
		fSpecial       = 0x0080
	};
};

int SLAPI PPViewGoodsRest::SetContractPrices()
{
	int    ok = -1;
	uint   cfm_msg = LocList.GetCount() ? PPCFM_SETCONTRACTPRICESSEL : PPCFM_SETCONTRACTPRICES;
	if(Filt.SupplID && CONFIRMCRIT(cfm_msg)) {
		struct _E { // @flat
			PPID   GoodsID;
			double Cost;
		};
		GoodsRestViewItem item;
		SVector suppl_cost_ary(sizeof(_E));
		PPIDArray locs_ary;
		PPWait(1);
		// @v10.8.2 @ctr MEMSZERO(item);
		if(LocList.GetCount())
			locs_ary.copy(LocList.Get());
		else
			locs_ary.add(0L);
		for(InitIteration(); NextIteration(&item) > 0;) {
			uint   pos = 0;
			double cost = 0.0;
			if(suppl_cost_ary.lsearch(&item.GoodsID, &pos, PTR_CMPFUNC(long)) > 0) {
				_E * p_e = static_cast<_E *>(suppl_cost_ary.at(pos));
				p_e->Cost = (item.Cost < p_e->Cost) ? item.Cost : p_e->Cost;
			}
			else {
				_E _e;
				_e.GoodsID = item.GoodsID;
				_e.Cost    = item.Cost;
				THROW_SL(suppl_cost_ary.insert(&_e));
			}
			PPWaitPercent(GetCounter());
		}
		PPWait(1);
		for(uint i = 0; i < suppl_cost_ary.getCount(); i++) {
			int    r = 0;
			_E   * p_e = static_cast<_E *>(suppl_cost_ary.at(i));
			if(p_e->Cost != 0.0) {
				for(uint j = 0; j < locs_ary.getCount(); j++) {
					const QuotIdent qi(locs_ary.at(j), 0, 0, Filt.SupplID);
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
	PPWait(0);
	return ok;
}

int SLAPI PPViewGoodsRest::Print(const void * pExtra)
{
	int    ok = 1;
	const  int use_option_dlg = pExtra ? *static_cast<const int *>(pExtra) : 0;
	PPViewGoodsRest::IterOrder ord = OrdByDefault;
	long   flags = 0;
	int    reply = -1;
	int    price_type = Filt.AmtType;
	uint   rpt_id = 0;
	ushort v = 0;
	TDialog * dlg = 0;
	if(Filt.Flags2 & GoodsRestFilt::f2CalcPrognosis) { // @v9.5.8 CalcPrognosis-->Flags2
		rpt_id = REPORT_GOODSRESTPRGN;
		price_type = 2;
	}
	else if(Filt.Flags & GoodsRestFilt::fEachLocation) {
		if(Filt.Sgg)
			rpt_id = REPORT_GOODSRESTEL_SGG;
		else
			rpt_id = (Filt.Flags & GoodsRestFilt::fShowDraftReceipt) ? REPORT_GOODSRESTELD : REPORT_GOODSRESTEL;
		if(!Filt.Sgg && (!Filt.GoodsGrpID || GObj.IsAltGroup(Filt.GoodsGrpID) <= 0))
			ord = PPViewGoodsRest::OrdByGrp_GoodsName;
		else
			ord = PPViewGoodsRest::OrdByGoodsName;
	}
	else if(Filt.Flags & GoodsRestFilt::fShowDraftReceipt)
		rpt_id = REPORT_GOODSRESTD;
	else {
		SETFLAG(flags, GoodsRestReport::fPrintBarCode, Filt.Flags & GoodsRestFilt::fBarCode);
		if(use_option_dlg) {
			THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PRNGDSREST))));
			v = Filt.AmtType;
			dlg->setCtrlData(CTL_PRNGDSREST_PRICE, &v);
			v = 0;
			dlg->AddClusterAssoc(CTL_PRNGDSREST_FLAGS, 0, GoodsRestReport::fPrintBarCode);
			dlg->AddClusterAssoc(CTL_PRNGDSREST_FLAGS, 1, GoodsRestReport::fInPacks);
			dlg->AddClusterAssoc(CTL_PRNGDSREST_FLAGS, 2, GoodsRestReport::fDisableGrpng);
			dlg->SetClusterData(CTL_PRNGDSREST_FLAGS, flags);
			reply = (ExecView(dlg) == cmOK) ? 1 : -1;
			dlg->getCtrlData(CTL_PRNGDSREST_PRICE, &v);
			price_type = v;
			dlg->getCtrlData(CTL_PRNGDSREST_SORT, &v);
			SETFLAG(flags, GoodsRestReport::fSortByPrice,   v & 0x0001);
			SETFLAG(flags, GoodsRestReport::fSortByBarCode, v & 0x0002);
			dlg->GetClusterData(CTL_PRNGDSREST_FLAGS, &flags);
			ZDELETE(dlg);
			if(reply <= 0)
				return 0;
		}
		else
			price_type = Filt.AmtType;
		if(price_type > 6 || price_type < 0)
			price_type = 0;
		if(Filt.Sgg)
			flags |= GoodsRestReport::fDisableGrpng;
		if(price_type >= 0 && price_type <= 2) {
			if((flags & GoodsRestReport::fPrintBarCode) || (Filt.Flags & GoodsRestFilt::fCalcOrder)) {
				SETIFZ(price_type, 2);
				rpt_id  = (flags & GoodsRestReport::fPrintBarCode) ? REPORT_SGOODSRESTBC : REPORT_FULLGOODSREST;
			}
			else
				rpt_id = price_type ? REPORT_SGOODSREST : REPORT_GOODSREST;
		}
		else if(price_type == 3) {
			rpt_id = REPORT_INVENTORY;
			flags |= (GoodsRestReport::fPrintBarCode | GoodsRestReport::fSkipRestData);
		}
		else if(price_type == 4) {
			rpt_id = REPORT_INVENTORY;
			flags |= GoodsRestReport::fPrintBarCode;
		}
		else if(price_type == 5) {
			rpt_id = REPORT_SELLSHEET;
			price_type = 2;
		}
		else if(price_type == 6) {
			rpt_id = REPORT_SPCGOODSREST;
			price_type = 2;
		}
		if(flags & GoodsRestReport::fSortByPrice) {
			if(flags & GoodsRestReport::fDisableGrpng)
				ord = PPViewGoodsRest::OrdByPrice;
			else
				ord = PPViewGoodsRest::OrdByGrp_Price;
		}
		else if(flags & GoodsRestReport::fSortByBarCode) {
			if(flags & GoodsRestReport::fDisableGrpng)
				ord = PPViewGoodsRest::OrdByBarCode;
			else
				ord = PPViewGoodsRest::OrdByGrp_BarCode;
		}
		else if(flags & GoodsRestReport::fDisableGrpng)
			ord = PPViewGoodsRest::OrdByGoodsName;
		else
			ord = PPViewGoodsRest::OrdByGrp_GoodsName;
	}
	if(rpt_id) {
		PPReportEnv env;
		PView  pf(this);
		pf.ID  = price_type;
		if(flags & GoodsRestReport::fInPacks)
			pf.ID |= 0x10000l;
		env.Sort = ord;
		env.PrnFlags = (flags & GoodsRestReport::fDisableGrpng) ? (SReport::DisableGrouping | SReport::NoRepError) : SReport::NoRepError;
		THROW(PPAlddPrint(rpt_id, &pf, &env));
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewGoodsRest::SerializeState(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	SString temp_buf;
	THROW(PPView::SerializeState(dir, rBuf, pCtx));
	THROW(GoodsIDs.Serialize(dir, rBuf, pCtx));
	THROW(Gsl.Serialize(dir, rBuf, pCtx));
	THROW(Total.Serialize(dir, rBuf, pCtx));
	THROW_SL(pCtx->Serialize(dir, WaitMsg, rBuf));
	THROW_SL(pCtx->Serialize(dir, GroupCalcThreshold, rBuf));
	THROW_SL(pCtx->Serialize(dir, Flags, rBuf));
	THROW_SL(pCtx->Serialize(dir, ScalePrefixID, rBuf));
	THROW_SL(pCtx->Serialize(dir, IterGrpName, rBuf));
	THROW_SL(pCtx->Serialize(dir, MaxCacheItems, rBuf));
	THROW_SL(pCtx->Serialize(dir, CacheDelta, rBuf));
	THROW_SL(pCtx->Serialize(dir, LastCacheCounter, rBuf));
	THROW_SL(pCtx->Serialize(dir, &DeficitList, rBuf));
	THROW_SL(pCtx->Serialize(dir, &ExclDeficitList, rBuf));
	THROW_SL(pCtx->Serialize(dir, &DraftRcptList, rBuf));
	THROW_SL(pCtx->Serialize(dir, &ExclDraftRcptList, rBuf));
	THROW_SL(pCtx->Serialize(dir, &UncompleteSessQttyList, rBuf));
	THROW_SL(pCtx->Serialize(dir, &ExclUncompleteSessQttyList, rBuf));
	THROW_SL(LocList.Serialize(dir, rBuf, pCtx));
	THROW(StrPool.SerializeS(dir, rBuf, pCtx)); // @v9.8.3
	THROW(SerializeDbTableByFileName <TempOrderTbl>     (dir, &P_TempOrd, rBuf, pCtx));
	THROW(SerializeDbTableByFileName <TempGoodsRestTbl> (dir, &P_Tbl,     rBuf, pCtx));
	if(dir > 0) {
		uint8 ind = P_Ct ? 0 : 1;
		THROW_SL(rBuf.Write(ind));
		if(P_Ct) {
			THROW(P_Ct->Write(rBuf, pCtx));
		}
	}
	else if(dir < 0) {
		uint8 ind = 0;
		ZDELETE(P_Ct);
		THROW_SL(rBuf.Read(ind));
		if(ind == 0 && (Filt.Flags & GoodsRestFilt::fCrosstab) && P_Tbl) {
			THROW_MEM(P_Ct = new GoodsRestCrosstab(this));
			THROW(P_Ct->Read(P_Tbl, rBuf, pCtx));
		}
	}
	CATCHZOK
	return ok;
}
//
// Implementation of PPALDD_GoodsRest
//
PPALDD_CONSTRUCTOR(GoodsRest)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsRest) { Destroy(); }

int PPALDD_GoodsRest::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(GoodsRest, rsrv);
	MEMSZERO(H);
	H.Dt         = p_filt->Date;
	H.FltPrgnBeg = p_filt->PrgnPeriod.low;
	H.FltPrgnEnd = p_filt->PrgnPeriod.upp;
	H.FltLocID   = p_filt->LocList.GetSingle();
	H.FltSupplID    = p_filt->SupplID;
	H.FltGoodsGrpID = p_filt->GoodsGrpID;
	H.PriceType = (rFilt.ID & 0x0FFFFl);
	H.InPacks   = BIN(rFilt.ID & 0x10000l);
	H.Flags     = p_filt->Flags;
	H.fNullRest  = BIN(p_filt->Flags & GoodsRestFilt::fNullRest);
	H.fCalcOrder = BIN(p_filt->Flags & GoodsRestFilt::fCalcOrder);
	H.fNoZeroOrderOnly = BIN(p_filt->Flags & GoodsRestFilt::fNoZeroOrderOnly);
	H.fEachLocation    = BIN(p_filt->Flags & GoodsRestFilt::fEachLocation);
	H.fUnderMinStock   = BIN(p_filt->Flags & GoodsRestFilt::fUnderMinStock);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsRest::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	//INIT_PPVIEW_ALDD_ITER(GoodsRest);
	PPViewGoodsRest * p_v = static_cast<PPViewGoodsRest *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	p_v->InitIteration(static_cast<PPViewGoodsRest::IterOrder>(SortIdx));
	return 1;
}

int PPALDD_GoodsRest::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(GoodsRest);
	I.GoodsID = item.GoodsID;
	I.LocID   = item.LocID;
	STRNSCPY(I.GoodsName, item.GoodsName);
	I.Rest    = item.Rest;
	I.PhRest  = item.PhRest;
	I.Cost    = item.Cost;
	I.Price   = item.Price;
	I.UnitsPerPack = item.UnitPerPack;
	I.SCost   = item.SumCost;
	I.SPrice  = item.SumPrice;
	I.PctAddedVal = item.PctAddedVal;
	I.Order   = item.Order;
	I.Prognosis = item.Predict;
	I.MinStock = item.MinStock;
	I.RestInDays = item.RestInDays;
	I.Deficit  = item.Deficit;
	I.DraftRcpt = item.DraftRcpt;
	STRNSCPY(I.GoodsGrpName, item.GoodsGrpName);
	I.SubstAsscCount = item.SubstAsscCount;
	QttyToStr(I.Rest, I.UnitsPerPack, H.InPacks ? (QTTYF_COMPLPACK | QTTYF_FRACTION) : QTTYF_FRACTION, I.CRest, 1);
	I.LastSellDate = item.LastSellDate;
	if(H.FltGoodsGrpID || H.Flags & GoodsRestFilt::fShowGoodsMatrixBelongs) {
		PPObjGoods goods_obj;
		if(H.FltGoodsGrpID) {
			// @v9.2.2 p_v->GetGoodsNumByAlterGroup(item.GoodsID, H.FltGoodsGrpID, &I.AltNo);
			goods_obj.P_Tbl->GetGoodsCodeInAltGrp(item.GoodsID, H.FltGoodsGrpID, &I.AltNo); // @v9.2.2
		}
		if(H.Flags & GoodsRestFilt::fShowGoodsMatrixBelongs) {
			PPID   loc_id = item.LocID;
			if(loc_id == 0) {
				const GoodsRestFilt * p_filt = (GoodsRestFilt *)p_v->GetBaseFilt();
				loc_id = p_filt ? p_filt->LocList.GetSingle() : 0;
			}
			I.MatrixStatus = BIN(goods_obj.P_Tbl->BelongToMatrix(item.GoodsID, loc_id) > 0);
		}
	}
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_GoodsRest::Destroy() { DESTROY_PPVIEW_ALDD(GoodsRest); }
//
// Implementation of PPALDD_GoodsRestTotal
//
PPALDD_CONSTRUCTOR(GoodsRestTotal)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsRestTotal) { Destroy(); }

int PPALDD_GoodsRestTotal::InitData(PPFilt & rFilt, long rsrv)
{
	GoodsRestTotalPrintData * p_data = 0;
	if(rsrv) {
		p_data = (GoodsRestTotalPrintData *)rFilt.Ptr;
		Extra[1].Ptr = p_data;
	}
	else
		return 0;
	H.Count    = p_data->P_Total->Count;
	H.SumCost  = p_data->P_Total->SumCost;
	H.SumPrice = p_data->P_Total->SumPrice;
	H.Quantity = p_data->P_Total->Quantity;
	H.PhQtty   = p_data->P_Total->PhQtty;
	H.FltDate       = p_data->P_Filt->Date;
	H.FltLocID      = p_data->P_Filt->LocList.GetSingle();
	H.FltSupplID    = p_data->P_Filt->SupplID;
	H.FltGoodsGrpID = p_data->P_Filt->GoodsGrpID;
	H.FltQuotKindID = p_data->P_Filt->QuotKindID;
	H.Flags         = p_data->P_Filt->Flags;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsRestTotal::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	I.LineNo = 0;
	return 1;
}

int PPALDD_GoodsRestTotal::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	GoodsRestTotalPrintData * p_data = static_cast<GoodsRestTotalPrintData *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	AmtEntry * p_item;
	uint n = (uint)I.LineNo;
	if(p_data->P_Total->Amounts.enumItems(&n, (void **)&p_item) > 0) {
		I.LineNo = n;
		I.AmtTypeID = p_item->AmtTypeID;
		I.Amt = p_item->Amt;
	}
	else if(n == 0) {
		I.LineNo = 1;
		I.AmtTypeID = 0;
		I.Amt = p_data->P_Total->Count;
	}
	else
		return -1;
	return DlRtm::NextIteration(iterId);
}

void PPALDD_GoodsRestTotal::Destroy()
{
	Extra[0].Ptr = 0;
	Extra[1].Ptr = 0;
}
