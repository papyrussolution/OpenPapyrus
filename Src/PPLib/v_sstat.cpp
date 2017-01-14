// V_SSTAT.CPP
// Copyright (c) A.Starodub, A.Sobolev 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
//
//
//
IMPLEMENT_PPFILT_FACTORY(SStat); SLAPI SStatFilt::SStatFilt() : PPBaseFilt(PPFILT_SSTAT, 0, 1)
{
	SetFlatChunk(offsetof(SStatFilt, ReserveStart), offsetof(SStatFilt, LocList)-offsetof(SStatFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(SStatFilt, LocList));
	Init(1, 0);
}

SStatFilt & FASTCALL SStatFilt::operator = (const SStatFilt & s)
{
	Copy(&s, 1);
	return *this;
}

int SLAPI SStatFilt::IsEqualExceptOrder(const SStatFilt * pFilt) const
{
	if(pFilt && Period.IsEqual(pFilt->Period) && GoodsGrpID == pFilt->GoodsGrpID &&
		Flags == pFilt->Flags && Sgg == pFilt->Sgg && Cycl == pFilt->Cycl &&
		OrdTerm == pFilt->OrdTerm && DlvrTerm == pFilt->DlvrTerm && SupplID == pFilt->SupplID &&
		DefInsurStock == pFilt->DefInsurStock && UpRestriction == pFilt->UpRestriction &&
		RestDate == pFilt->RestDate && LocList.IsEqual(pFilt->LocList)) {
		return 1;
	}
	else
		return 0;
}

void SLAPI SStatFilt::SetupCfgOptions(const PPPredictConfig & rCfg)
{
	_CFlags = rCfg.Flags & (PPPredictConfig::fRoundPckgUp | PPPredictConfig::fRoundPckgDn |
		PPPredictConfig::fZeroPckgUp | PPPredictConfig::fPrefStockPckg | PPPredictConfig::fPrefLotPckg |
		PPPredictConfig::fUseInsurStock | PPPredictConfig::fMinStockAsMinOrder);
	_Method = rCfg.Method;
	_P = rCfg.P;
	_MinP = rCfg.MinP;
	_TrustCriterion = rCfg.TrustCriterion;
}

int SLAPI SStatFilt::GetPckgUse() const
{
	return PPPredictConfig::_GetPckgUse(_CFlags);
}

int SLAPI SStatFilt::GetPckgRounding() const
{
	return PPPredictConfig::_GetPckgRounding(_CFlags);
}

void SLAPI SStatFilt::SetPckgUse(int t)
{
	long   v = PPPredictConfig::_SetPckgUse(_CFlags, t);
	(_CFlags &= ~(PPPredictConfig::fPrefStockPckg | PPPredictConfig::fPrefLotPckg)) |= v;
}

void SLAPI SStatFilt::SetPckgRounding(int t)
{
	long   v = PPPredictConfig::_SetPckgRounding(_CFlags, t);
	(_CFlags &= ~(PPPredictConfig::fRoundPckgUp | PPPredictConfig::fRoundPckgDn)) |= v;
}
//
//
//
SLAPI PPViewSStat::PPViewSStat() : PPView(0, &Filt, PPVIEW_SSTAT)
{
	P_TempTbl = 0;
	P_TempOrd = 0;
	P_VGr = 0;
	MEMSZERO(PrCfg);
	PrcssrPrediction::GetPredictCfg(&PrCfg);
}

SLAPI PPViewSStat::~PPViewSStat()
{
	ZDELETE(P_TempTbl);
	ZDELETE(P_TempOrd);
	ZDELETE(P_VGr);
	DBRemoveTempFiles();
}

#define GRP_CYCLE     1
#define GRP_GOODSFILT 2
#define GRP_LOC       3

class SStatOrderFiltDialog : public TDialog {
public:
	SStatOrderFiltDialog() : TDialog(DLG_SSTATORDFLT)
	{
		SetupCalDate(CTLCAL_SSTATFLT_DATE, CTL_SSTATFLT_DATE);
		SetupCalDate(CTLCAL_SSTATFLT_RESTDATE, CTL_SSTATFLT_RESTDATE);
		MEMSZERO(PrCfg);
		PrcssrPrediction::GetPredictCfg(&PrCfg);
		showCtrl(CTL_SSTATFLT_MTX_IND, 0);
		if(LConfig.Flags & CFGFLG_USEGOODSMATRIX) {
			showCtrl(CTL_SSTATFLT_MTX_IND, 1);
			SetCtrlBitmap(CTL_SSTATFLT_MTX_IND, BM_MATRIX);
		}
	}
	int    setDTS(const SStatFilt * pData)
	{
		Data = *pData;

		int    ok = 1, disable_arcode_usage = 0;
		ushort v;
		LDATE  init_date = Data.Period.upp;
		PPID   prev_grp_level = 0;
		PPObjGoods goods_obj;

		goods_obj.GetParentID(Data.GoodsGrpID, &prev_grp_level);
		addGroup(GRP_GOODSFILT, new GoodsFiltCtrlGroup(0, CTLSEL_SSTATFLT_GGROUP, cmGoodsFilt));
		GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, 0, 0, GoodsCtrlGroup::enableSelUpLevel, prev_grp_level);
		setGroupData(GRP_GOODSFILT, &gf_rec);
		SetupArCombo(this, CTLSEL_SSTATFLT_SUPPL, Data.SupplID, 0, GetSupplAccSheet(), sacfDisableIfZeroSheet);
		addGroup(GRP_LOC, new LocationCtrlGroup(CTLSEL_SSTATFLT_LOC, 0, 0, cmLocList, 0, 0, 0));
		LocationCtrlGroup::Rec loc_rec(&Data.LocList);
		setGroupData(GRP_LOC, &loc_rec);
		setCtrlData(CTL_SSTATFLT_DATE, &init_date);
		setCtrlData(CTL_SSTATFLT_RESTDATE, &Data.RestDate);

		CycleCtrlGroup::Rec cycle_rec;
		CycleCtrlGroup * p_grp = new CycleCtrlGroup(CTLSEL_SSTATFLT_CYCLE, CTL_SSTATFLT_PDC, CTL_SSTATFLT_NUMCYCLES, 0);
		addGroup(GRP_CYCLE, p_grp);
		cycle_rec.C = Data.Cycl;
		setGroupData(GRP_CYCLE, &cycle_rec);
		setCtrlData(CTL_SSTATFLT_ORDP, &Data.OrdTerm);
		setCtrlData(CTL_SSTATFLT_DLVRP, &Data.DlvrTerm);
		setCtrlData(CTL_SSTATFLT_UPRESTR, &Data.UpRestriction);
		setCtrlData(CTL_SSTATFLT_DEFINSSTOCK, &Data.DefInsurStock);
		AddClusterAssoc(CTL_SSTATFLT_SORTORDER, -1, PPViewSStat::OrdByGoodsName);
		AddClusterAssoc(CTL_SSTATFLT_SORTORDER,  0, PPViewSStat::OrdByGoodsName);
		AddClusterAssoc(CTL_SSTATFLT_SORTORDER,  1, PPViewSStat::OrdByCount);
		AddClusterAssoc(CTL_SSTATFLT_SORTORDER,  2, PPViewSStat::OrdByQttyAvg);
		AddClusterAssoc(CTL_SSTATFLT_SORTORDER,  3, PPViewSStat::OrdByPriceAvg);
		SetClusterData(CTL_SSTATFLT_SORTORDER, Data.Order);

		AddClusterAssoc(CTL_SSTATFLT_FLAGS2, 0, SStatFilt::fUseInsurStock);
		SetClusterData(CTL_SSTATFLT_FLAGS2, Data.Flags);
		if(CConfig.Flags & CCFLG_USEARGOODSCODE) {
			if(PrCfg.FixArCodes & (SStatFilt::fExtByArCode | SStatFilt::fRestrictByArCode)) {
				Data.Flags |= (PrCfg.FixArCodes & (SStatFilt::fExtByArCode | SStatFilt::fRestrictByArCode));
			}
			v = (Data.Flags & SStatFilt::fExtByArCode) ? 1 : ((Data.Flags & SStatFilt::fRestrictByArCode) ? 2 : 0);
		}
		else {
			v = 0;
			disable_arcode_usage = 1;
		}
		disableCtrl(CTL_SSTATFLT_ARCODEUSAGE, disable_arcode_usage);
		setCtrlUInt16(CTL_SSTATFLT_ARCODEUSAGE, v);
		return ok;
	}
	int    getDTS(SStatFilt * pData)
	{
		int    ok = 1;
		uint   sel_id = 0;
		LDATE  init_date;
		CycleCtrlGroup::Rec cycle_rec;
		LocationCtrlGroup::Rec loc_rec;
		GoodsFiltCtrlGroup::Rec gf_rec;

		THROW(getGroupData(GRP_CYCLE, &cycle_rec));
		Data.Cycl = cycle_rec.C;
		getCtrlData(CTL_SSTATFLT_DATE,      &init_date);
		getCtrlData(CTL_SSTATFLT_RESTDATE,  &Data.RestDate);
		getGroupData(GRP_GOODSFILT, &gf_rec);
		Data.GoodsGrpID = gf_rec.GoodsGrpID;
		getGroupData(GRP_LOC, &loc_rec);
		Data.LocList = loc_rec.LocList;
		if(Data.LocList.IsExists() && Data.LocList.IsEmpty())
			Data.LocList.Set(0);
		getCtrlData(CTLSEL_SSTATFLT_SUPPL,  &Data.SupplID);
		getCtrlData(CTL_SSTATFLT_ORDP,      &Data.OrdTerm);
		getCtrlData(CTL_SSTATFLT_DLVRP,     &Data.DlvrTerm);
		getCtrlData(CTL_SSTATFLT_UPRESTR, &Data.UpRestriction);
		getCtrlData(CTL_SSTATFLT_DEFINSSTOCK, &Data.DefInsurStock);
		Data.Period.Set(ZERODATE, (Data.Cycl.Cycle && !init_date) ? LConfig.OperDate : init_date);
		GetClusterData(CTL_SSTATFLT_SORTORDER, &Data.Order);
		GetClusterData(CTL_SSTATFLT_FLAGS2, &Data.Flags);
		Data.Flags &= ~(SStatFilt::fExtByArCode | SStatFilt::fRestrictByArCode);
		if(CConfig.Flags & CCFLG_USEARGOODSCODE) {
			ushort v = getCtrlUInt16(CTL_SSTATFLT_ARCODEUSAGE);
			if(v == 1)
				Data.Flags |= SStatFilt::fExtByArCode;
			else if(v == 2)
				Data.Flags |= SStatFilt::fRestrictByArCode;
		}
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = PPErrorByDialog(this, sel_id, -1);
		ENDCATCH
		return ok;
	}
private:
	class SStatOrderFiltExtDialog : public TDialog {
	public:
		SStatOrderFiltExtDialog() : TDialog(DLG_SSTATORDFLTEXT)
		{
		}
		int    setDTS(const SStatFilt * pData)
		{
			Data = *pData;

			int    ok = 1;
			setCtrlUInt16(CTL_SSTATFLT_OVERRCFG, BIN(Data.Flags & SStatFilt::fOverrideCfgParams));

			AddClusterAssoc(CTL_SSTATFLT_METHOD,  0, PRMTHD_SIMPLEAVERAGE);
			AddClusterAssoc(CTL_SSTATFLT_METHOD, -1, PRMTHD_SIMPLEAVERAGE);
			AddClusterAssoc(CTL_SSTATFLT_METHOD,  1, PRMTHD_LSLIN);
			SetClusterData(CTL_SSTATFLT_METHOD, Data._Method);

			AddClusterAssoc(CTL_SSTATFLT_USEPCKG,  0, PPPredictConfig::pckgDontUse);
			AddClusterAssoc(CTL_SSTATFLT_USEPCKG, -1, PPPredictConfig::pckgDontUse);
			AddClusterAssoc(CTL_SSTATFLT_USEPCKG,  1, PPPredictConfig::pckgPrefStock);
			AddClusterAssoc(CTL_SSTATFLT_USEPCKG,  2, PPPredictConfig::pckgPrefLot);
			SetClusterData(CTL_SSTATFLT_USEPCKG, Data.GetPckgUse());

			AddClusterAssoc(CTL_SSTATFLT_ROUNDPCKG,  0, PPPredictConfig::pckgRoundUp);
			AddClusterAssoc(CTL_SSTATFLT_ROUNDPCKG, -1, PPPredictConfig::pckgRoundUp);
			AddClusterAssoc(CTL_SSTATFLT_ROUNDPCKG,  1, PPPredictConfig::pckgRoundDn);
			AddClusterAssoc(CTL_SSTATFLT_ROUNDPCKG,  2, PPPredictConfig::pckgRoundNear);
			SetClusterData(CTL_SSTATFLT_ROUNDPCKG, Data.GetPckgRounding());

			setCtrlData(CTL_SSTATFLT_TRUST,  &Data._TrustCriterion);
			setCtrlData(CTL_SSTATFLT_P, &Data._P);
			setCtrlData(CTL_SSTATFLT_PMIN, &Data._MinP);
			AddClusterAssoc(CTL_SSTATFLT_CFLAGS, 0, PPPredictConfig::fZeroPckgUp);
			AddClusterAssoc(CTL_SSTATFLT_CFLAGS, 1, PPPredictConfig::fUseInsurStock);
			AddClusterAssoc(CTL_SSTATFLT_CFLAGS, 2, PPPredictConfig::fMinStockAsMinOrder);
			SetClusterData(CTL_SSTATFLT_CFLAGS, Data._CFlags);

			SetupCtrls();
			return ok;
		}
		int    getDTS(SStatFilt * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			long   temp_long;
			SETFLAG(Data.Flags, SStatFilt::fOverrideCfgParams, getCtrlUInt16(CTL_SSTATFLT_OVERRCFG));
			GetClusterData(CTL_SSTATFLT_METHOD, &Data._Method);
			GetClusterData(CTL_SSTATFLT_USEPCKG, &(temp_long = 0));
			Data.SetPckgUse(temp_long);
			GetClusterData(CTL_SSTATFLT_ROUNDPCKG, &(temp_long = 0));
			Data.SetPckgRounding(temp_long);
			getCtrlData(sel = CTL_SSTATFLT_TRUST,  &Data._TrustCriterion);
			THROW_PP(Data._TrustCriterion >= 0 && Data._TrustCriterion <= 100, PPERR_PERCENTINPUT);
			getCtrlData(sel = CTL_SSTATFLT_P, &Data._P);
			THROW_PP(Data._P >= 0, PPERR_INVMODELPARAM);
			getCtrlData(sel = CTL_SSTATFLT_PMIN, &Data._MinP);
			THROW_PP(Data._MinP >= 0 && Data._MinP <= Data._P, PPERR_INVPREDICT_PMIN)
			GetClusterData(CTL_SSTATFLT_CFLAGS, &Data._CFlags);
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel, -1);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_SSTATFLT_OVERRCFG)) {
				SetupCtrls();
				clearEvent(event);
			}
		}
		void SetupCtrls()
		{
			disableCtrls(!getCtrlUInt16(CTL_SSTATFLT_OVERRCFG),
				CTL_SSTATFLT_METHOD, CTL_SSTATFLT_USEPCKG, CTL_SSTATFLT_ROUNDPCKG,
				CTL_SSTATFLT_TRUST, CTL_SSTATFLT_P, CTL_SSTATFLT_CFLAGS, CTL_SSTATFLT_PMIN, 0);
		}

		SStatFilt Data;
	};
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmaMore)) {
			PPDialogProcBody <SStatOrderFiltExtDialog, SStatFilt> (&Data);
			clearEvent(event);
		}
	}
	SStatFilt Data;
	PPPredictConfig PrCfg;
};

int SLAPI PPViewSStat::EditDlvrOrderFilt(SStatFilt * pFilt)
{
	DIALOG_PROC_BODY(SStatOrderFiltDialog, pFilt);
}

PPBaseFilt * SLAPI PPViewSStat::CreateFilt(void * extraPtr) const
{
	SStatFilt * p_filt = new SStatFilt;
	p_filt->LocList.Add(LConfig.Location);
	p_filt->Flags |= SStatFilt::fSkipZeroNhCount;
	if(((long)extraPtr) & 0x0001) {
		p_filt->Flags |= (SStatFilt::fSupplOrderForm | SStatFilt::fRoundOrderToPack);
		if(PrCfg.Flags & PPPredictConfig::fUseInsurStock) {
			p_filt->Flags |= SStatFilt::fUseInsurStock;
			p_filt->DefInsurStock = PrCfg.DefInsurStock;
		}
		p_filt->SetupCfgOptions(PrCfg);
	}
	return p_filt;
}

int SLAPI PPViewSStat::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1, valid_data = 0;
	PPID   prev_grp_level = 0;
	TDialog * dlg = 0;
	SStatFilt * p_filt = 0;
	THROW(Filt.IsA(pBaseFilt));
	p_filt = (SStatFilt *)pBaseFilt;
	if(p_filt->Flags & SStatFilt::fSupplOrderForm) {
		ok = EditDlvrOrderFilt(p_filt);
		if(!(p_filt->Flags & SStatFilt::fOverrideCfgParams))
			p_filt->SetupCfgOptions(PrCfg);
	}
	else {
		THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_SSTATFLT)), 0));
		if(p_filt->GoodsGrpID) {
			Goods2Tbl::Rec grp_rec;
			if(GObj.Fetch(p_filt->GoodsGrpID, &grp_rec) > 0)
				prev_grp_level = grp_rec.ParentID;
		}
		dlg->addGroup(GRP_GOODSFILT, new GoodsFiltCtrlGroup(0, CTLSEL_SSTATFLT_GGROUP, cmGoodsFilt));
		GoodsFiltCtrlGroup::Rec gf_rec(p_filt->GoodsGrpID, 0, 0, GoodsCtrlGroup::enableSelUpLevel, prev_grp_level);
		dlg->setGroupData(GRP_GOODSFILT, &gf_rec);
		dlg->addGroup(GRP_LOC, new LocationCtrlGroup(CTLSEL_SSTATFLT_LOC, 0, 0, cmLocList, 0, 0, 0));
		LocationCtrlGroup::Rec loc_rec(&p_filt->LocList);
		dlg->setGroupData(GRP_LOC, &loc_rec);
		SetupSubstGoodsCombo(dlg, CTLSEL_SSTATFLT_SUBST, p_filt->Sgg);
		SetupCalCtrl(CTLCAL_SSTATFLT_PERIOD, dlg, CTL_SSTATFLT_PERIOD, 1);
		SetPeriodInput(dlg, CTL_SSTATFLT_PERIOD, &p_filt->Period);

		CycleCtrlGroup::Rec cycle_rec;
		CycleCtrlGroup * p_grp = new CycleCtrlGroup(CTLSEL_SSTATFLT_CYCLE, CTL_SSTATFLT_PDC, CTL_SSTATFLT_NUMCYCLES, CTL_SSTATFLT_PERIOD);
		dlg->addGroup(GRP_CYCLE, p_grp);
		cycle_rec.C = Filt.Cycl;
		dlg->setGroupData(GRP_CYCLE, &cycle_rec);

		dlg->AddClusterAssoc(CTL_SSTATFLT_FLAGS, 0, SStatFilt::fSkipZeroNhCount);
		dlg->SetClusterData(CTL_SSTATFLT_FLAGS, p_filt->Flags);
		dlg->AddClusterAssoc(CTL_SSTATFLT_ORDER, 0, OrdByGoodsName);
		dlg->AddClusterAssoc(CTL_SSTATFLT_ORDER, 1, OrdByCount);
		dlg->AddClusterAssoc(CTL_SSTATFLT_ORDER, 2, OrdByQttyAvg);
		dlg->AddClusterAssoc(CTL_SSTATFLT_ORDER, 3, OrdByQttyVar);
		dlg->AddClusterAssoc(CTL_SSTATFLT_ORDER, 4, OrdByAmtAvg);
		dlg->AddClusterAssoc(CTL_SSTATFLT_ORDER, 5, OrdByAmtVar);
		dlg->AddClusterAssoc(CTL_SSTATFLT_ORDER, 6, OrdByQttyAvgCount);
		dlg->SetClusterData(CTL_SSTATFLT_ORDER, p_filt->Order);
		SetRealRangeInput(dlg, CTL_SSTATFLT_COUNT,   &p_filt->CountRange,   2);
		SetRealRangeInput(dlg, CTL_SSTATFLT_QTTYAVG, &p_filt->QttyAvgRange, 2);
		SetRealRangeInput(dlg, CTL_SSTATFLT_AMTAVG,  &p_filt->AmtAvgRange,  2);
		SetRealRangeInput(dlg, CTL_SSTATFLT_QTTYVAR, &p_filt->QttyVarRange, 2);
		while(!valid_data && ExecView(dlg) == cmOK)
			if(!GetPeriodInput(dlg, CTL_SSTATFLT_PERIOD, &p_filt->Period))
				PPErrorByDialog(dlg, CTL_SSTATFLT_PERIOD, -1);
			else {
				dlg->getGroupData(GRP_CYCLE, &cycle_rec);
				p_filt->Cycl = cycle_rec.C;
				dlg->getGroupData(GRP_GOODSFILT, &gf_rec);
				p_filt->GoodsGrpID = gf_rec.GoodsGrpID;
				dlg->getGroupData(GRP_LOC, &loc_rec);
				p_filt->LocList = loc_rec.LocList;
				if(p_filt->LocList.IsExists() && p_filt->LocList.IsEmpty())
					p_filt->LocList.Set(0);
				dlg->getCtrlData(CTLSEL_SSTATFLT_SUBST, &p_filt->Sgg);
				dlg->GetClusterData(CTL_SSTATFLT_FLAGS, &p_filt->Flags);
				dlg->GetClusterData(CTL_SSTATFLT_ORDER, &p_filt->Order);
				GetRealRangeInput(dlg, CTL_SSTATFLT_COUNT,   &p_filt->CountRange);
				GetRealRangeInput(dlg, CTL_SSTATFLT_QTTYAVG, &p_filt->QttyAvgRange);
				GetRealRangeInput(dlg, CTL_SSTATFLT_AMTAVG,  &p_filt->AmtAvgRange);
				GetRealRangeInput(dlg, CTL_SSTATFLT_QTTYVAR, &p_filt->QttyVarRange);
				ok = valid_data = 1;
			}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int SLAPI PPViewSStat::CreateOrderTable(long ord, TempOrderTbl ** ppTbl, int use_ta)
{
	int    ok = 1;
	ZDELETE(P_TempOrd);

	*ppTbl = 0;
	if(!P_TempTbl || oneof2(ord, OrdByDefault, OrdByGoodsName) || CycleList.getCount())
		return -1;

	IterCounter cntr;
	TempOrderTbl * p_o = 0;
	TempOrderTbl::Rec ord_rec;
	TempGoodsStatTbl::Key0 k;
	TempGoodsStatTbl * p_t = P_TempTbl;
	BExtQuery q(P_TempTbl, 0);
	PPID   prev_goods_id = 0;
	cntr.Init(p_t);
	THROW(p_o = CreateTempOrderFile());
	{
		BExtInsert bei(p_o);
		q.select(p_t->GoodsID, p_t->GoodsName, p_t->Dt, p_t->Count, p_t->QttyAvg, p_t->QttyVar,
			p_t->AmtAvg, p_t->AmtVar, p_t->PriceAvg, 0L);
		MEMSZERO(k);
		PPTransaction tra(ppDbDependTransaction, use_ta);
		THROW(tra);
		for(q.initIteration(0, &k, spFirst); q.nextIteration() > 0;) {
			const double large_val = 1e12;
			const char * p_fmt = "%030.8lf";
			if(p_t->data.GoodsID != prev_goods_id) {
				if((Filt.CountRange.IsZero() || Filt.CountRange.CheckX(p_t->data.Count)) &&
					(Filt.QttyAvgRange.IsZero() || Filt.QttyAvgRange.CheckX(p_t->data.QttyAvg)) &&
					(Filt.AmtAvgRange.IsZero()  || Filt.AmtAvgRange.CheckX(p_t->data.AmtAvg)) &&
					(Filt.QttyVarRange.IsZero() || Filt.QttyVarRange.CheckX(p_t->data.QttyVar))) {
					MEMSZERO(ord_rec);
					ord_rec.ID = p_t->data.GoodsID;
					// ≈сли установлены циклы, то допускаетс€ сортировка только по наименованию //
					if(ord == OrdByGoodsName || CycleList.getCount()) {
						char dt_buf[16];
						strnzcpy(ord_rec.Name, p_t->data.GoodsName, 48);
						strcat(ord_rec.Name, datefmt(&p_t->data.Dt, DATF_YMD|DATF_CENTURY, dt_buf));
					}
					if(ord == OrdByCount)
						sprintf(ord_rec.Name, p_fmt, (double)large_val-p_t->data.Count);
					else if(ord == OrdByQttyAvg)
						sprintf(ord_rec.Name, p_fmt, large_val-p_t->data.QttyAvg);
					else if(ord == OrdByQttyVar)
						sprintf(ord_rec.Name, p_fmt, p_t->data.QttyVar);
					else if(ord == OrdByAmtAvg)
						sprintf(ord_rec.Name, p_fmt, large_val-p_t->data.AmtAvg);
					else if(ord == OrdByAmtVar)
						sprintf(ord_rec.Name, p_fmt, p_t->data.AmtVar);
					else if(ord == OrdByPriceAvg)
						sprintf(ord_rec.Name, p_fmt, large_val-p_t->data.PriceAvg);
					else if(ord == OrdByQttyAvgCount)
						sprintf(ord_rec.Name, "%015.8lf%015.8lf", p_t->data.QttyAvg, large_val-p_t->data.Count);
					THROW_DB(bei.insert(&ord_rec));
				}
			}
			prev_goods_id = p_t->data.GoodsID;
			PPWaitPercent(cntr.Increment());
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
		*ppTbl = p_o;
		p_o = 0;
	}
	CATCHZOK
	delete p_o;
	return ok;
}
//
//
//
int SLAPI PPViewSStat::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1, use_ta = 1;
	SStatFilt prev_filt = Filt;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	Filt.Period.Actualize(ZERODATE); // @v8.6.9
	Filt.RestDate = Filt.RestDate.getactual(ZERODATE); // @v8.6.9
	CreatedBillList.clear(); // @v8.9.11
	if(P_TempTbl && prev_filt.IsEqualExceptOrder(&Filt)) {
		int create_ot = ((prev_filt.CountRange.low != Filt.CountRange.low || prev_filt.CountRange.upp != Filt.CountRange.upp) ||
			(prev_filt.QttyAvgRange.low != Filt.QttyAvgRange.low || prev_filt.QttyAvgRange.upp != Filt.QttyAvgRange.upp) ||
			(prev_filt.AmtAvgRange.low != Filt.AmtAvgRange.low || prev_filt.AmtAvgRange.upp != Filt.AmtAvgRange.upp) ||
			(prev_filt.QttyVarRange.low != Filt.QttyVarRange.low || prev_filt.QttyVarRange.upp != Filt.QttyVarRange.upp)) ? 1 : 0;
		if(prev_filt.Order != Filt.Order || create_ot)
			THROW(CreateOrderTable(Filt.Order, &P_TempOrd, use_ta));
	}
	else {
		Gsl.Clear();
		ZDELETE(P_TempTbl);
		ZDELETE(P_TempOrd);
		ZDELETE(P_VGr);
		ZDELETE(P_Ct);
		if(Filt.Flags & SStatFilt::fSupplOrderForm) {
			Filt.Flags &= ~SStatFilt::fSkipZeroNhCount;
			GoodsRestFilt gr_filt;
			THROW_MEM(P_VGr = new PPViewGoodsRest);
			gr_filt.Date = Filt.RestDate;
			gr_filt.GoodsGrpID = Filt.GoodsGrpID;
			gr_filt.LocList = Filt.LocList;
			gr_filt.SupplID = Filt.SupplID;
			gr_filt.Sgg = Filt.Sgg;
			gr_filt.Flags |= (GoodsRestFilt::fNullRest|GoodsRestFilt::fWoSupplier|GoodsRestFilt::fForceNullRest); // @v8.6.6 GoodsRestFilt::fForceNullRest
			SETFLAG(gr_filt.Flags, GoodsRestFilt::fExtByArCode,      Filt.Flags & SStatFilt::fExtByArCode);
			SETFLAG(gr_filt.Flags, GoodsRestFilt::fRestrictByArCode, Filt.Flags & SStatFilt::fRestrictByArCode);
			if(Filt.Flags & SStatFilt::fUseInsurStock)
				gr_filt.Flags |= GoodsRestFilt::fShowMinStock;
			SETFLAG(gr_filt.Flags, GoodsRestFilt::fUseGoodsMatrix, BIN(LConfig.Flags & CFGFLG_USEGOODSMATRIX));
			THROW(P_VGr->Init_(&gr_filt));
		}
		THROW(CreateTempTable(use_ta));
		THROW(CreateOrderTable(Filt.Order, &P_TempOrd, use_ta));
		{
			class SStatCrosstab : public Crosstab {
			public:
				SLAPI  SStatCrosstab(PPViewSStat * pV) : Crosstab()
				{
					P_V = pV;
				}
				virtual BrowserWindow * SLAPI CreateBrowser(uint brwId, int dataOwner)
				{
					PPViewBrowser * p_brw = new PPViewBrowser(brwId, CreateBrowserQuery(), P_V, dataOwner);
					SetupBrowserCtColumns(p_brw);
					return p_brw;
				}
			protected:
				virtual int SLAPI SetupFixFields(int initialCall)
				{
					if(!initialCall) {
						DBTable * p_tbl = GetResultTable();
						if(p_tbl) {
							PPID   goods_id = 0;
							if(p_tbl->getFieldValByName("GoodsID", &goods_id, 0)) {
								GoodsRestViewItem gr_item;
								const SStatFilt * p_filt = (const SStatFilt *)P_V->GetBaseFilt();
								if(P_V->GetRestItem(goods_id, &p_filt->LocList, &gr_item) > 0) {
									p_tbl->setFieldValByName("Rest",    &gr_item.Rest);
									p_tbl->setFieldValByName("Predict", &gr_item.Predict);
									p_tbl->setFieldValByName("Order",   &gr_item.SupplOrder);
								}
							}
						}
					}
					return 1;
				}
				PPViewSStat * P_V;
			};
			if(Filt.Flags & SStatFilt::fSupplOrderForm && CycleList.getCount()) {
				THROW_MEM(P_Ct = new SStatCrosstab(this));
				P_Ct->SetTable(P_TempTbl, P_TempTbl->Dt);
				P_Ct->AddIdxField(P_TempTbl->GoodsID);
				P_Ct->SetSortIdx("GoodsName", 0);
				P_Ct->AddInheritedFixField(P_TempTbl->GoodsName);
				P_Ct->AddFixField("Rest",    T_DOUBLE);
				P_Ct->AddFixField("Predict", T_DOUBLE);
				P_Ct->AddFixField("Order",   T_DOUBLE);
				P_Ct->AddAggrField(P_TempTbl->Count);
				P_Ct->AddAggrField(P_TempTbl->QttyAvg);
				THROW(P_Ct->Create(use_ta));
			}
		}
	}
	CATCH
		ZDELETE(P_Ct);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPViewSStat::InitIteration()
{
	int    ok = 1;
	ZDELETE(P_IterQuery);
	if(P_TempOrd) {
		TempOrderTbl::Key1 k;
		THROW_MEM(P_IterQuery = new BExtQuery(P_TempOrd, 1, 64));
		P_IterQuery->selectAll();
		MEMSZERO(k);
		P_IterQuery->initIteration(0, &k, spFirst);
		Counter.Init(P_TempOrd);
	}
	else if(P_TempTbl) {
		DBQ * dbq = 0;
		TempGoodsStatTbl::Key1 k1;
		THROW_MEM(P_IterQuery = new BExtQuery(P_TempTbl, 1, 64));
		P_IterQuery->selectAll();
		MEMSZERO(k1);
		P_IterQuery->initIteration(0, &k1, spFirst);
		Counter.Init(P_TempTbl);
	}
	else
		ok = 0;
	CATCH
		ZDELETE(P_IterQuery);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPViewSStat::RecToViewItem(const TempGoodsStatTbl::Rec * pRec, SStatViewItem * pItem)
{
	SStatViewItem item;
	MEMSZERO(item);
	item.GoodsID      = pRec->GoodsID;
	STRNSCPY(item.GoodsName, pRec->GoodsName);
	item.Dt           = pRec->Dt;
	item.Count        = pRec->Count;
	item.QttyAvg      = pRec->QttyAvg;
	item.QttySigma    = pRec->QttySigma;
	item.QttyVar      = pRec->QttyVar;
	item.QttyTrnovr   = pRec->QttyTrnovr;
	item.AmtAvg       = pRec->AmtAvg;
	item.AmtSigma     = pRec->AmtSigma;
	item.AmtVar       = pRec->AmtVar;
	item.AmtTrnovr    = pRec->AmtTrnovr;
	item.LastCalcDate = pRec->End;
	item.PriceAvg     = pRec->PriceAvg;
	if(Filt.Flags & SStatFilt::fSupplOrderForm && P_VGr) {
		GoodsRestViewItem gr_item;
		const ObjIdListFilt * p_loc_list = Filt.LocList.GetSingle() ? &Filt.LocList : 0;
		if(GetRestItem(item.GoodsID, p_loc_list, &gr_item) > 0) {
			item.Rest = gr_item.Rest;
			item.Predict = gr_item.Predict;
			item.SupplOrder = gr_item.SupplOrder;
			item.MinStock = gr_item.MinStock;
			item.IsPredictTrust = gr_item.IsPredictTrust; // @v7.0.0
			item.CostAvg = gr_item.Cost; // @v7.1.7
		}
	}
	ASSIGN_PTR(pItem, item);
	return 1;
}

int SLAPI PPViewSStat::NextIteration(SStatViewItem * pItem)
{
	if(P_TempTbl)
		while(P_IterQuery && P_IterQuery->nextIteration() > 0) {
			if(P_TempOrd) {
				TempGoodsStatTbl::Key0 k;
				k.GoodsID = P_TempOrd->data.ID;
				k.Dt      = ZERODATE;
				if(P_TempTbl->search(0, &k, spEq) == 0)
					continue;
			}
			TempGoodsStatTbl::Rec rec;
			rec = P_TempTbl->data;
			if(!Filt.CountRange.IsZero() && Filt.CountRange.CheckX(rec.Count) <= 0)
				continue;
			if(!Filt.QttyAvgRange.IsZero() && Filt.QttyAvgRange.CheckX(rec.QttyAvg) <= 0)
				continue;
			if(!Filt.AmtAvgRange.IsZero()  && Filt.AmtAvgRange.CheckX(rec.AmtAvg) <= 0)
				continue;
			if(!Filt.QttyVarRange.IsZero() && Filt.QttyVarRange.CheckX(rec.QttyVar) <= 0)
				continue;
			RecToViewItem(&rec, pItem);
			Counter.Increment();
			return 1;
		}
	return -1;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempGoodsStat);

int SLAPI PPViewSStat::AddStat(PPID goodsID, LDATE dt, int setTotal, const PredictSalesStat * pStat, BExtInsert * pBei)
{
	int    ok = 1;
	if(pStat->Count || !(Filt.Flags & SStatFilt::fSkipZeroNhCount)) {
		TempGoodsStatTbl::Rec rec;
		MEMSZERO(rec);
		rec.GoodsID  = goodsID;
		rec.Dt = dt;
		GObj.GetSubstText(goodsID, Filt.Sgg, &Gsl, rec.GoodsName, sizeof(rec.GoodsName));
		rec.Count     = pStat->Count;
		rec.QttyAvg   = setTotal ? pStat->QttySum : pStat->GetAverage(PSSV_QTTY);
		rec.QttySigma = pStat->GetSigma(PSSV_QTTY);
		rec.QttyVar   = pStat->GetVar(PSSV_QTTY);
		rec.QttyTrnovr = pStat->GetTrnovr(PSSV_QTTY);
		rec.AmtAvg    = setTotal ? pStat->AmtSum : pStat->GetAverage(PSSV_AMT);
		rec.AmtSigma  = pStat->GetSigma(PSSV_AMT);
		rec.AmtVar    = pStat->GetVar(PSSV_AMT);
		rec.AmtTrnovr = pStat->GetTrnovr(PSSV_AMT);
		rec.PriceAvg  = fdivnz(rec.AmtAvg, rec.QttyAvg);
		if(!pBei->insert(&rec))
			ok = PPSetErrorDB();
	}
	return ok;
}

int SLAPI PPViewSStat::AddStatByCycles(PPID goodsID, const PPIDArray * pIdList, int setTotal, BExtInsert * pBei, PredictSalesStat * pStat)
{
	PsiArray  series;
	PredictSalesStat pss;
	if(PsT.GetSeries(*pIdList, Filt.LocList, &Filt.Period, &series) > 0) {
		for(uint i = 0; i < CycleList.getCount(); i++) {
			DateRange p;
			CycleList.getPeriod(i, &p);
			pss.Init();
			pss.Step(&series, &p);
			pss.Finish();
			if(!AddStat(goodsID, p.low, setTotal, &pss, pBei))
				return 0;
		}
		if(pStat) {
			pStat->Init();
			pStat->Step(&series, 0);
			pStat->Finish();
		}
	}
	return 1;
}

double SLAPI __CalcOrderQuantity(
	const double rest,
	const double prediction,
	//
	// ƒва взаимоисключающих показател€: если minStockDays == 0 используетс€ minStockQtty
	//
	const int    minStockDays, // —траховочный запас в дн€х
	const double minStockQtty, // —траховочный запас в торговых единицах
	//
	const int calcWithoutStat,
	const int minStockAsMinOrder,
	const int useInsurStock)
{
	double order = 0.0;
	if(calcWithoutStat)
		order = (minStockQtty > 0.0) ? (minStockQtty - rest) : 0; // @v9.1.1 (min_stock_qtty)-->(min_stock_qtty > 0.0)
	else if(minStockAsMinOrder && minStockDays <= 0) {
		// @v9.1.1 order = (prediction > minStockQtty) ? (prediction - rest) : (minStockQtty - rest);
		// @v9.1.1 {
		if(prediction > minStockQtty) {
			order = prediction - rest;
			if(useInsurStock && minStockQtty > 0.0)
				order += minStockQtty;
		}
		else {
			order = minStockQtty - rest;
		}
		// } @v9.1.1
	}
	else {
		order = prediction - rest;
		if(useInsurStock && minStockQtty > 0.0)
			order += minStockQtty;
	}
    return order;
}

int SLAPI PPViewSStat::CreateTempTable(int use_ta)
{
	int    ok = 1;
	const PPConfig & r_cfg = LConfig;
	Predictor predictor;
	GoodsIterator g_iter;
	Goods2Tbl::Rec goods_rec;
	PredictSalesStat pss;
	PPIDArray id_list;
	ZDELETE(P_TempTbl);
	THROW(P_TempTbl = CreateTempFile());
	{
		BExtInsert bei(P_TempTbl);
		CycleList.init2(&Filt.Period, &Filt.Cycl);
		PPTransaction tra(ppDbDependTransaction, use_ta);
		THROW(tra);
		if(Filt.Sgg) {
			//
			// –асчет с подстановкой товара. ѕодстановка исключает расчет заказов поставщику.
			//
			long   c = 0;
			uint   i;
			IterCounter cntr;
			PPID   iter_id = 0;
			PPIDArray subst_list, loc_list;
			PPObjGoods::SubstBlock sgg_blk;
			sgg_blk.ExclParentID = Filt.GoodsGrpID;

			Gsl.Clear();
			Gsl.SaveAssoc = 1;
			if(Filt.LocList.IsExists())
				Filt.LocList.CopyTo(&loc_list);
			else
				PsT.GetLocList(&loc_list);
			for(g_iter.Init(Filt.GoodsGrpID, 0); g_iter.Next(&goods_rec) > 0;) {
				PPID   subst_id = 0;
				THROW(PPCheckUserBreak());
				if(Filt.Sgg == sggLocation) {
					if(CycleList.getCount() == 0) {
						for(uint i = 0; i < loc_list.getCount(); i++) {
							ObjIdListFilt objlf_loc_list;
							objlf_loc_list.Add(loc_list.at(i), 0);
							if(PsT.CalcStat(goods_rec.ID, objlf_loc_list, &Filt.Period, 0, &pss) > 0) {
								sgg_blk.LocID = pss.LocID;
								THROW(GObj.SubstGoods(goods_rec.ID, &subst_id, Filt.Sgg, &sgg_blk, &Gsl));
								c++;
							}
						}
					}
				}
				else {
					assert(sgg_blk.LocID == 0);
					THROW(GObj.SubstGoods(goods_rec.ID, &subst_id, Filt.Sgg, &sgg_blk, &Gsl));
					c++;
				}
				PPWaitPercent(g_iter.GetIterCounter());
			}
			pss.Init();
			Gsl.GetSubstList(subst_list);
			cntr.Init(c);
			for(i = 0; i < subst_list.getCount(); i++) {
				PPID   subst_id = subst_list.get(i);
				id_list.clear();
				THROW(Gsl.GetGoodsBySubstID(subst_id, &id_list));
				if(id_list.getCount()) {
					if(CycleList.getCount() > 0) {
						THROW(AddStatByCycles(subst_id, &id_list, 0, &bei, 0));
					}
					else {
						ObjIdListFilt objlf_loc_list;
						if(Filt.Sgg == sggLocation)
							objlf_loc_list.Add(subst_id & ~GOODSSUBSTMASK, 0);
						else
							objlf_loc_list.Set(&loc_list);
						if(PsT.CalcStat(id_list, objlf_loc_list, &Filt.Period, &pss) > 0)
							THROW(AddStat(subst_id, ZERODATE, 0, &pss, &bei));
					}
				}
				PPWaitPercent(cntr.Add(id_list.getCount()));
			}
		}
		else if(Filt.Flags & SStatFilt::fSupplOrderForm && P_VGr) {
			//
			// ‘ормирование таблицы дл€ расчета заказов поставщикам.
			// “аблица формируетс€ по товарам, перечисленным в таблице остатков товаров (PPViewGoodsRest).
			// ≈сли задан расчет по циклам (CycleList) или период расчета статистики Filt.Period,
			// то прогноз строитс€ на основе данных статистики продаж за выбранный период по простому среднему,
			// иначе - по стандартной процедуре прогнозировани€ спроса (класс Predictor).
			//
			const int use_matrix = BIN(r_cfg.Flags & CFGFLG_USEGOODSMATRIX);
			const int minstock_as_minorder = BIN(PrCfg.Flags & PPPredictConfig::fMinStockAsMinOrder);
			const int use_insur_stock = BIN(Filt.Flags & SStatFilt::fUseInsurStock);
			GoodsRestViewItem item;
			long   num_days = 0;
			{
				const LDATE dt = NZOR(Filt.Period.upp, r_cfg.OperDate);
				for(long i = 1; i <= Filt.OrdTerm + Filt.DlvrTerm; i++)
					if(predictor.IsWorkDay(&Filt.LocList, plusdate(dt, i)))
						num_days++;
			}
			Predictor::EvalParam ep(&Filt);
			for(P_VGr->InitIteration(); P_VGr->NextIteration(&item) > 0; PPWaitPercent(P_VGr->GetCounter())) {
				int    calc_ord_wo_stat = 0; // расчитывать заказ дл€ товаров, по которым нет статистики.
				double prediction = 0.0, order = 0.0;
				//double ord_predict = 0.0;
				//
				// ƒва взаимоисключающих показател€: если min_stock_days == 0 используетс€ min_stock_qtty
				//
				double min_stock_qtty = 0.0; // —траховочный запас в торговых единицах
				int    min_stock_days = 0;   // —траховочный запас в дн€х
				double sales_per_day = 0.0;
				int    can_trust = 1;
				THROW(PPCheckUserBreak());
				if(use_insur_stock || minstock_as_minorder) {
					GoodsStockExt gse;
					if(GObj.GetStockExt(item.GoodsID, &gse, 1) > 0)
						min_stock_qtty = gse.GetMinStock(item.LocID);
					if(min_stock_qtty == 0.0 && Filt.DefInsurStock > 0)
						min_stock_days = Filt.DefInsurStock;
				}
				if(CycleList.getCount()) {
					PredictSalesStat pss;
					id_list.clear();
					id_list.add(item.GoodsID);
					THROW(AddStatByCycles(item.GoodsID, &id_list, 1, &bei, &pss));
					sales_per_day = pss.GetAverage(PSSV_QTTY);
					prediction = sales_per_day * num_days;
					if(min_stock_days > 0)
						min_stock_qtty = sales_per_day * min_stock_days;
				}
				else {
					int    r = 0;
					pss.Init();
					if(Filt.Period.IsZero() || Filt.Period.upp) {
						r = PsT.GetStat(item.GoodsID, Filt.LocList, &pss);
						DateRange period;
						int    add_days = Filt.OrdTerm + Filt.DlvrTerm - 1;
						if(min_stock_days > 0) {
							add_days += min_stock_days;
							//
							// ‘ункци€ расчета прогноза учтет мин. запас в дн€х. —ледовательно, нам
							// необходимо обнулить мин. запас во избежании двойного его учета.
							//
							min_stock_qtty = 0.0;
						}
						period.low = NZOR(Filt.RestDate, r_cfg.OperDate);
						period.upp = plusdate(period.low, add_days);
						ep.Set(&Filt.LocList, item.GoodsID, period);
						ep.LoadUpDate = Filt.Period.upp;
						if(predictor.Predict_(ep, &prediction, 0, &can_trust) <= 0 && (use_matrix || minstock_as_minorder)) // @v9.1.1 (|| minstock_as_minorder)
							calc_ord_wo_stat = 1;
					}
					else {
						if((r = PsT.CalcStat(item.GoodsID, Filt.LocList, &Filt.Period, 0, &pss)) < 0)
							calc_ord_wo_stat = 1;
						sales_per_day = pss.GetAverage(PSSV_QTTY);
						prediction = sales_per_day * num_days;
						if(min_stock_days > 0)
							min_stock_qtty = sales_per_day * min_stock_days;
					}
					THROW(AddStat(item.GoodsID, ZERODATE, 0, &pss, &bei));
				}
				if(can_trust || calc_ord_wo_stat) {
					order = __CalcOrderQuantity(item.Rest, prediction, min_stock_days, min_stock_qtty,
						can_trust ? 0 : calc_ord_wo_stat, minstock_as_minorder, use_insur_stock);
					order = R0(order);
					if(order > 0.0) {
						int    use_pckg = Filt.GetPckgUse();
						int    rnd_pckg = Filt.GetPckgRounding();
						double pckg = 0.0;
						GoodsStockExt gse;
						if(use_pckg == PPPredictConfig::pckgPrefStock) {
							if(GObj.GetStockExt(item.GoodsID, &gse, 1) > 0 && gse.Package > 0.0)
								pckg = gse.Package;
							else if(item.UnitPerPack > 0.0)
								pckg = item.UnitPerPack;
						}
						else if(use_pckg == PPPredictConfig::pckgPrefLot) {
							if(item.UnitPerPack > 0.0)
								pckg = item.UnitPerPack;
							else if(GObj.GetStockExt(item.GoodsID, &gse, 1) > 0 && gse.Package > 0.0)
								pckg = gse.Package;
						}
						if(pckg > 0.0) {
							double ipart;
							double fract = R6(modf(R6(order / pckg), &ipart));
							if(fract) {
								if(rnd_pckg == PPPredictConfig::pckgRoundUp || (rnd_pckg == PPPredictConfig::pckgRoundNear && fract >= 0.5))
									ipart += 1.0;
							}
							if(ipart == 0.0 && Filt._CFlags & PPPredictConfig::fZeroPckgUp)
								ipart = 1.0;
							order = ipart * pckg;
						}
					}
					else
						order = 0.0;
				}
				THROW(P_VGr->SetSupplOrderValues(item.GoodsID, item.LocID, prediction, min_stock_qtty, order, can_trust));
			}
		}
		else {
			//
			// ќбыкновенна€ таблица анализа статистики (без подстановки товара и без
			// расчета заказов поставщику)
			//
			for(g_iter.Init(Filt.GoodsGrpID, 0); g_iter.Next(&goods_rec) > 0; PPWaitPercent(g_iter.GetIterCounter())) {
				THROW(PPCheckUserBreak());
				if(CycleList.getCount()) {
					id_list.clear();
					id_list.add(goods_rec.ID);
					THROW(AddStatByCycles(goods_rec.ID, &id_list, 0, &bei, 0));
				}
				else {
					int    r;
					if(Filt.Period.IsZero())
						r = PsT.GetStat(goods_rec.ID, Filt.LocList, &pss);
					else
						r = PsT.CalcStat(goods_rec.ID, Filt.LocList, &Filt.Period, 0, &pss);
					if(r > 0)
						THROW(AddStat(goods_rec.ID, ZERODATE, 0, &pss, &bei));
				}
			}
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
	}
	CATCH
		ok = 0;
		ZDELETE(P_TempTbl);
	ENDCATCH
	return ok;
}

DBQuery * SLAPI PPViewSStat::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = 0;
	TempGoodsStatTbl * p_tt = 0;
	TempGoodsRestTbl * p_gr = 0;
	TempOrderTbl * ot = 0;
	DBQuery * p_q = 0;
	if(P_Ct) {
		brw_id = BROWSER_SSTAT_SORD_CT;
		p_q = PPView::CrosstabDbQueryStub;
	}
	else {
		DBFieldList fld_list;
		long   ff = (LConfig.Flags & CFGFLG_USEPACKAGE) ? (QTTYF_COMPLPACK | QTTYF_FRACTION | NMBF_NOZERO) : (QTTYF_FRACTION | NMBF_NOZERO);
		if(Filt.Flags & SStatFilt::fSupplOrderForm)
			brw_id = BROWSER_SSTAT_SORD;
		else
			brw_id = CycleList.getCount() ? BROWSER_SSTAT_CYCLE : BROWSER_SSTAT;
		if(P_TempTbl) {
			if(P_TempOrd)
				THROW(CheckTblPtr(ot = new TempOrderTbl(P_TempOrd->fileName)));
			THROW(CheckTblPtr(p_tt = new TempGoodsStatTbl(P_TempTbl->fileName)));
			fld_list.Add(p_tt->GoodsID);     //  #0
			fld_list.Add(p_tt->Dt);          //  #1
			fld_list.Add(p_tt->GoodsName);   //  #2
			fld_list.Add(p_tt->Count);       //  #3
			fld_list.Add(p_tt->QttyAvg);     //  #4
			fld_list.Add(p_tt->QttySigma);   //  #5
			fld_list.Add(p_tt->QttyVar);     //  #6
			fld_list.Add(p_tt->QttyTrnovr);  //  #7
			fld_list.Add(p_tt->AmtAvg);      //  #8
			fld_list.Add(p_tt->AmtSigma);    //  #9
			fld_list.Add(p_tt->AmtVar);      // #10
			fld_list.Add(p_tt->AmtTrnovr);   // #11
			p_q = & select(fld_list);
			if(Filt.Flags & SStatFilt::fSupplOrderForm && P_VGr) {
				DBE    cq;
				SString file_name;
				P_VGr->GetTableName(file_name);
				THROW(CheckTblPtr(p_gr = new TempGoodsRestTbl(file_name)));
				{
					cq.init();
					cq.push(p_gr->Quantity);
					cq.push(p_gr->UnitPerPack);
					DBConst dbc_long;
					dbc_long.init(1L);
					cq.push(dbc_long);
					dbc_long.init(ff);
					cq.push(dbc_long); // Formatting flags
					cq.push((DBFunc)PPDbqFuncPool::IdCQtty);
					p_q->addField(cq);               // #12
				}
				{
					cq.init();
					cq.push(p_gr->Predict);
					cq.push(p_gr->UnitPerPack);
					cq.push(p_gr->IsPredictTrust);
					DBConst dbc_long;
					dbc_long.init(ff);
					cq.push(dbc_long); // Formatting flags
					cq.push((DBFunc)PPDbqFuncPool::IdCQtty);
					p_q->addField(cq);               // #13
				}
				{
					cq.init();
					cq.push(p_gr->SupplOrder);
					cq.push(p_gr->UnitPerPack);
					DBConst dbc_long;
					dbc_long.init(1L);
					cq.push(dbc_long);
					dbc_long.init(ff);
					cq.push(dbc_long); // Formatting flags
					cq.push((DBFunc)PPDbqFuncPool::IdCQtty);
					p_q->addField(cq);               // #14
				}
				p_q->addField(p_gr->IsPredictTrust); // #15
				p_q->addField(p_tt->PriceAvg);       // #16
				p_q->addField(p_gr->MinStock);       // #17
				p_q->addField(p_gr->Cost);           // #18 @v7.1.7 ÷ена поступлени€ //
				if(ot)
					p_q->from(ot, p_tt, p_gr, 0L).where(p_tt->GoodsID == ot->ID && p_gr->GoodsID == p_tt->GoodsID).orderBy(ot->Name, 0L);
				else {
					DBQ * dbq = 0;
					dbq = & (*dbq && p_gr->GoodsID == p_tt->GoodsID);
					if(!Filt.CountRange.IsZero())
						dbq = & (*dbq && realrange(p_tt->Count, Filt.CountRange.low, Filt.CountRange.upp));
					if(!Filt.QttyAvgRange.IsZero())
						dbq = & (*dbq && realrange(p_tt->QttyAvg, Filt.QttyAvgRange.low, Filt.QttyAvgRange.upp));
					if(!Filt.AmtAvgRange.IsZero())
						dbq = & (*dbq && realrange(p_tt->AmtAvg, Filt.AmtAvgRange.low, Filt.AmtAvgRange.upp));
					if(!Filt.QttyVarRange.IsZero())
						dbq = & (*dbq && realrange(p_tt->QttyVar, Filt.QttyVarRange.low, Filt.QttyVarRange.upp));
					p_q->from(p_tt, p_gr, 0L).where(*dbq).orderBy(p_tt->GoodsName, 0L);
				}
			}
			else if(ot)
				p_q->from(ot, p_tt, 0L).where(p_tt->GoodsID == ot->ID).orderBy(ot->Name, 0L);
			else {
				DBQ * dbq = 0;
				if(!Filt.CountRange.IsZero())
					dbq = & (*dbq && realrange(p_tt->Count, Filt.CountRange.low, Filt.CountRange.upp));
				if(!Filt.QttyAvgRange.IsZero())
					dbq = & (*dbq && realrange(p_tt->QttyAvg, Filt.QttyAvgRange.low, Filt.QttyAvgRange.upp));
				if(!Filt.AmtAvgRange.IsZero())
					dbq = & (*dbq && realrange(p_tt->AmtAvg, Filt.AmtAvgRange.low, Filt.AmtAvgRange.upp));
				if(!Filt.QttyVarRange.IsZero())
					dbq = & (*dbq && realrange(p_tt->QttyVar, Filt.QttyVarRange.low, Filt.QttyVarRange.upp));
				p_q->from(p_tt, 0L).where(*dbq).orderBy(p_tt->GoodsName, 0L);
			}
			THROW(CheckQueryPtr(p_q));
		}
	}
	if(pSubTitle) {
		GetExtLocationName(Filt.LocList, 3, *pSubTitle);
		pSubTitle->CatDiv('-', 1).Cat(Filt.Period);
	}
	CATCH
		if(p_q)
			ZDELETE(p_q);
		else {
			delete p_tt;
			delete p_gr;
			delete ot;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_q;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = (PPViewBrowser *)extraPtr;
	if(p_brw) {
		PPViewSStat * p_view = (PPViewSStat *)p_brw->P_View;
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int SLAPI PPViewSStat::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle) {
		const  DBQBrowserDef * p_def = (DBQBrowserDef *)pBrw->getDef();
		if(p_def && col >= 0 && col < (long)p_def->getCount()) {
			const BroColumn & r_col = p_def->at(col);
			const PPID goods_id = P_Ct ? ((PPID *)pData)[1] : ((PPID *)pData)[0];
			if(col == 0) { // Ќаименование товара
				const TagFilt & r_tag_filt = GObj.GetConfig().TagIndFilt;
				SColor clr;
				if(!r_tag_filt.IsEmpty() && r_tag_filt.SelectIndicator(goods_id, clr) > 0) {
					pStyle->Flags |= BrowserWindow::CellStyle::fLeftBottomCorner;
					pStyle->Color2 = (COLORREF)clr;
					ok = 1;
				}
			}
			else if(!P_Ct) {
				if(r_col.OrgOffs == 13) { // ѕрогноз
					const  DBQuery * p_q = p_def->getQuery();
					short  is_trust = 0;
					long   stat_count = -1;
					int    is_new_goods = 0;
					if(p_q) {
						if(p_q->fldCount > 15) { // #15 - поле запроса, содержащее признак довери€ к прогнозу
							size_t offs = 0;
							for(uint i = 0; i < 15; i++)
								offs += stsize(p_q->flds[i].type);
							memcpy(&is_trust, PTR8(pData) + offs, sizeof(short));
						}
						if(p_q->fldCount > 3) { // #03 - поле запроса, содержащее количество элементов статистики
							size_t offs = 0;
							for(uint i = 0; i < 3; i++)
								offs += stsize(p_q->flds[i].type);
							memcpy(&stat_count, PTR8(pData) + offs, sizeof(long));
						}
					}
					if(stat_count == 0) {
						PPID  loc_id = Filt.LocList.GetSingle();
						ReceiptCore & r_rcpt = BillObj->trfr->Rcpt;
						if(r_rcpt.GetLastLot(goods_id, loc_id, MAXDATE, 0) <= 0)
							is_new_goods = 1;
					}
					if(is_new_goods) {
						pStyle->Color = (COLORREF)SColor(SClrBlue);
						pStyle->Flags = BrowserWindow::CellStyle::fCorner;
						ok = 1;
					}
					else if(!is_trust) {
						pStyle->Color = RGB(0x91, 0x91, 0x91);
						pStyle->Flags = BrowserWindow::CellStyle::fCorner;
						ok = 1;
					}
				}
			}
		}
	}
	return ok;
}

int SLAPI PPViewSStat::PreprocessBrowser(PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pBrw->SetTempGoodsGrp(Filt.GoodsGrpID) > 0)
		ok = 1;
	pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	return ok;
}
//
//
//
int SLAPI PPViewSStat::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	PPID   goods_id = pHdr ? (P_Ct ? ((PPID *)pHdr)[1] : ((PPID *)pHdr)[0]) : 0;
	if(goods_id) {
		PredictSalesFilt ps_filt;
		ps_filt.Period = Filt.Period;
		ps_filt.Flags |= PredictSalesFilt::fShowNoData;
		if(Filt.Sgg) {
			char   subst_text[128];
			PPIDArray id_list;
			Gsl.GetGoodsBySubstID(goods_id, &id_list);
			GObj.GetSubstText(goods_id, Filt.Sgg, &Gsl, subst_text, sizeof(subst_text));
			ps_filt.SetGoodsList(&id_list, subst_text);
			ps_filt.LocList = Filt.LocList;
			if(Filt.Sgg == sggLocation)
				ps_filt.LocList.Add(goods_id & ~GOODSSUBSTMASK, 0);
		}
		else {
			ps_filt.GoodsGrpID = Filt.GoodsGrpID;
			ps_filt.GoodsID = goods_id;
			ps_filt.LocList = Filt.LocList;
		}
		ok = ViewPredictSales(&ps_filt);
	}
	return ok;
}

int SLAPI PPViewSStat::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   goods_id = pHdr ? (P_Ct ? ((PPID *)pHdr)[1] : ((PPID *)pHdr)[0]) : 0;
		switch(ppvCmd) {
			case PPVCMD_EDITGOODS:
				ok = -1;
				if(goods_id && GObj.Edit(&goods_id, 0) == cmOK)
					ok = 1;
				break;
			case PPVCMD_VIEWQUOT:
			case PPVCMD_VIEWSUPPLQUOT:
			case PPVCMD_VIEWGOODSMATRIX:
			case PPVCMD_VIEWPREDICTCOEFF:
				ok = -1;
				if(goods_id) {
					int    quot_cls = 0;
					if(ppvCmd == PPVCMD_VIEWSUPPLQUOT)
						quot_cls = PPQuot::clsSupplDeal;
					else if(ppvCmd == PPVCMD_VIEWGOODSMATRIX)
						quot_cls = PPQuot::clsMtx;
					else if(ppvCmd == PPVCMD_VIEWPREDICTCOEFF)
						quot_cls = PPQuot::clsPredictCoeff;
					else
						quot_cls = PPQuot::clsGeneral;
					GObj.EditQuotations(goods_id, Filt.LocList.GetSingle(), -1L, 0, quot_cls);
				}
				break;
			case PPVCMD_VIEWLOTS:
				ok = -1;
				if(goods_id > 0)
					::ViewLots(goods_id, Filt.LocList.GetSingle(), 0L, 0, 1);
				break;
			case PPVCMD_PUTTOBASKET:
				ok = -1;
				{
					SStatViewItem item;
					if(GetItem(goods_id, &item) > 0)
						AddGoodsToBasket(item.GoodsID, Filt.LocList.GetSingle(), item.SupplOrder, 0);
				}
				break;
			case PPVCMD_PUTTOBASKETALL:
				ok = -1;
				ConvertLinesToBasket();
				break;
			case PPVCMD_EDITVAL:
				ok = -1;
				{
					PPID ct_id = (pHdr && P_Ct) ? *(PPID *)pHdr : 0;
					if(EditOrder(ct_id, goods_id) > 0)
						ok = 1;
				}
				break;
			case PPVCMD_MAKEPURCHASEBILL:
				ok = -1;
				AddPurchaseBill();
				break;
			case PPVCMD_VIEWBILLS:
				ok = -1;
				ViewCreatedBills();
				break;
		}
	}
	return ok;
}

int SLAPI PPViewSStat::GetRestItem(PPID goodsID, const ObjIdListFilt * pLocList, GoodsRestViewItem * pItem)
{
	return P_VGr ? P_VGr->GetItem(goodsID, pLocList, pItem) : 0;
}

int SLAPI PPViewSStat::GetItem(PPID goodsID, SStatViewItem * pItem)
{
	if(goodsID && P_TempTbl) {
		TempGoodsStatTbl::Key0 k0;
		MEMSZERO(k0);
		k0.GoodsID = goodsID;
		if(P_TempTbl->search(0, &k0, spEq)) {
			RecToViewItem(&P_TempTbl->data, pItem);
			return 1;
		}
	}
	return -1;
}

int SLAPI PPViewSStat::EditOrder(PPID ctID, PPID goodsID)
{
	// Crosstab
	int    ok = -1;
	TempGoodsRestTbl * p_gr = 0;
	if(Filt.Flags & SStatFilt::fSupplOrderForm && P_VGr) {
		SString file_name;
		TempGoodsRestTbl::Key3 k;
		MEMSZERO(k);
		k.GoodsID = goodsID;
		P_VGr->GetTableName(file_name);
		THROW(CheckTblPtr(p_gr = new TempGoodsRestTbl(file_name)));
		if(p_gr->search(3, &k, spGe) > 0 && k.GoodsID == goodsID && Filt.LocList.CheckID(k.LocID)) {
			double qtty = p_gr->data.SupplOrder;
			if(InputQttyDialog(0, 0, &qtty) > 0) {
				p_gr->data.SupplOrder = qtty;
				THROW_DB(p_gr->updateRec());
				CALLPTRMEMB(P_Ct, SetFixFieldValByCTID(ctID, 2, &qtty));
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	ZDELETE(p_gr);
	return ok;
}

int SLAPI PPViewSStat::Print(const void *)
{
	int    ok = 1;
	uint   rpt_id = (Filt.Flags & SStatFilt::fSupplOrderForm) ?
		(P_Ct ? REPORT_SSTATSUPPLORD_CT : REPORT_SSTATSUPPLORD) : REPORT_SSTATGGROUP;
	PPReportEnv env;
	PView  pv(this);
	env.Sort = Filt.Order;
	if(!oneof2(env.Sort, OrdByDefault, OrdByGoodsName))
		env.PrnFlags = SReport::DisableGrouping;
	PPAlddPrint(rpt_id, &pv, &env);
	return ok;
}

int SLAPI PPViewSStat::ConvertLinesToBasket()
{
	int    ok = -1, r = -1, convert = 0;
	SelBasketParam param;
	param.SelPrice = 1;
	if(Filt.Sgg == sggNone && (r = GetBasketByDialog(&param, GetSymb())) > 0) {
		SStatViewItem ss_item;
		PPIDArray goods_id_list;
		const PPID loc_id = Filt.LocList.GetSingle();
		PPWait(1);
		for(InitIteration(); NextIteration(&ss_item) > 0;) {
			const PPID goods_id = ss_item.GoodsID;
			if(!goods_id_list.lsearch(goods_id) && ss_item.SupplOrder != 0.0 && GObj.CheckSpecQuot(Filt.SupplID, goods_id, loc_id, 0)) {
				ILTI   i_i;
				ReceiptTbl::Rec lot_rec;
				MEMSZERO(lot_rec);
				THROW(::GetCurGoodsPrice(goods_id, loc_id, GPRET_MOSTRECENT, 0, &lot_rec) != GPRET_ERROR);
				i_i.GoodsID     = goods_id;
				//i_i.UnitPerPack = gr_item.UnitPerPack;
				i_i.Cost        = R5(lot_rec.Cost);
				if(param.SelPrice == 1)
					i_i.Price = R5(lot_rec.Cost);
				else if(param.SelPrice == 2)
					i_i.Price = R5(lot_rec.Price);
				else if(param.SelPrice == 3)
					i_i.Price = R5(lot_rec.Price);
				else
					i_i.Price = R5(lot_rec.Price);
				i_i.CurPrice = 0.0;
				i_i.Flags    = 0;
				i_i.Suppl    = lot_rec.SupplID;
				i_i.QCert    = lot_rec.QCertID;
				i_i.Expiry   = lot_rec.Expiry;
				i_i.Quantity = fabs(ss_item.SupplOrder);
				THROW(param.Pack.AddItem(&i_i, 0, param.SelReplace));
				goods_id_list.add(goods_id);
			}
		}
		PPWait(0);
		THROW(GoodsBasketDialog(param, 1));
	}
	else
		THROW(r);
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int SLAPI PPViewSStat::CreatePurchaseBill(LDATE docDt, int autoOrder, PPBillPacket * pPack, int useTa)
{
	int    ok = -1;
	const  PPID op_id = PrCfg.PurchaseOpID;
	const  PPID loc_id = NZOR(Filt.LocList.GetSingle(), (autoOrder ? LConfig.Location : 0));
	if(op_id && loc_id) {
		ulong  lines_count = 0;
		SStatViewItem ss_item;
		PPIDArray goods_id_list;
		BillFilt flt;
		flt.LocList.Add(loc_id);
		flt.OpID       = op_id;
		flt.AccSheetID = GetSupplAccSheet();
		flt.ObjectID   = Filt.SupplID;
		flt.Period.upp = docDt;
		PPOprKind op_rec;
		const PPID suppl_deal_qk_id = (GetOpData(op_id, &op_rec) > 0 && op_rec.ExtFlags & OPKFX_USESUPPLDEAL) ? DS.GetConstTLA().SupplDealQuotKindID : 0;
		THROW(BillObj->CheckRights(PPR_INS));
		THROW(pPack->CreateBlankByFilt(op_id, &flt, -1));
		// @v8.6.9 {
		if(autoOrder) {
			SString temp_buf, memo_buf;
			PPLoadString("autoorder", temp_buf);
			(memo_buf = 0).CatChar('#').Cat(temp_buf).Space().Cat(getcurdatetime_(), DATF_DMY, TIMF_HMS);
			STRNSCPY(pPack->Rec.Memo, memo_buf);
		}
		// } @v8.6.9
		for(InitIteration(); NextIteration(&ss_item) > 0;) {
			const PPID goods_id = ss_item.GoodsID;
			if(!goods_id_list.lsearch(goods_id) && ss_item.SupplOrder != 0.0 && GObj.CheckSpecQuot(Filt.SupplID, goods_id, loc_id, 0)) {
				int    accept = 1;
				if(autoOrder) {
					GoodsRestViewItem gr_item;
					accept = BIN(GetRestItem(ss_item.GoodsID, &Filt.LocList, &gr_item) > 0 && gr_item.IsPredictTrust);
				}
				if(accept) {
					PPTransferItem ti;
					ReceiptTbl::Rec lot_rec;
					MEMSZERO(lot_rec);
					THROW(::GetCurGoodsPrice(goods_id, loc_id, GPRET_MOSTRECENT, 0, &lot_rec) != GPRET_ERROR);
					THROW(ti.Init(&pPack->Rec));
					ti.SetupGoods(goods_id);
					ti.SetupLot(0, 0, 0);
					ti.Quantity_   = fabs(ss_item.SupplOrder);
					ti.Cost        = R5(lot_rec.Cost);
					if(suppl_deal_qk_id && pPack->Rec.Object) {
						const QuotIdent qic(pPack->Rec.LocID, suppl_deal_qk_id, 0, pPack->Rec.Object);
						double c = 0.0;
						if(GObj.GetQuotExt(goods_id, qic, lot_rec.Cost, lot_rec.Price, &c, 1) > 0 && c > 0.0)
							ti.Cost = R5(c);
					}
					ti.Price       = R5(lot_rec.Price);
					ti.Suppl       = lot_rec.SupplID;
					ti.QCert       = lot_rec.QCertID;
					THROW(pPack->InsertRow(&ti, 0));
					goods_id_list.add(goods_id);
					lines_count++;
				}
			}
		}
		pPack->InitAmounts();
		if(pPack->GetTCount())
			ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewSStat::AddPurchaseBill()
{
	int    ok = -1;
	PPBillPacket pack;
	if(CreatePurchaseBill(ZERODATE, 0, &pack, 0) > 0) {
		if(::EditGoodsBill(&pack, 0) == cmOK) {
			/*
			if(autoOrder) {
				SString memo;
				LDATETIME cur_dtm;
				getcurdatetime(&cur_dtm);
				(memo = "@autoorder").Space().Cat(cur_dtm).Dot().Space().CatEq("ѕериод заказа дней", (long)Filt.OrdTerm);
				memo.ToOem();
				memo.CopyTo(pack.Rec.Memo, sizeof(pack.Rec.Memo));
			}
			*/
			THROW(BillObj->TurnPacket(&pack, 1));
			CreatedBillList.addUnique(pack.Rec.ID);
			ok = 1;
		}
		else
			pack.UngetCounter();
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewSStat::ViewCreatedBills()
{
	int    ok = -1;
	if(CreatedBillList.getCount()) {
		PPViewBill view;
		BillFilt filt;
		filt.List.Set(&CreatedBillList);
		PPView::Execute(PPVIEW_BILL, &filt, 1, 0);
	}
	return ok;
}

struct _OrdArEntry {
	PPID   ArID;
	PPID   GoodsGrpID;       // ->Goods2.ID
	PPID   LocID;            // ->Location.ID
	PPID   MngrID;           // ->Person.ID ћенеджер, закрепленный за этой группой закупки
	int16  OrdPrdDays;       //
	int16  DuePrdDays;       //
	DateRepeating Dr;        // ѕериодичность заказа
};

static SString & MakeOrderAutocreationTag(const _OrdArEntry & rEntry, LDATE dt, SString & rBuf)
{
    return (rBuf = 0).Cat("PCZ").CatChar('.').Cat(rEntry.ArID).CatChar('.').Cat(rEntry.GoodsGrpID).CatChar('.').
		Cat(rEntry.LocID).CatChar('.').Cat(rEntry.MngrID).CatChar('.').Cat(dt, MKSFMT(0, DATF_YMD|DATF_CENTURY|DATF_NODIV));
}

// static
int SLAPI PPViewSStat::CreateDraftBySupplOrders(const SStatFilt * pFilt)
{
	_OrdArEntry ord_entry;

	int    ok = 1, r = 0;
	SString temp_buf, fmt_buf, msg_buf;
	SArray ar_list(sizeof(_OrdArEntry));
	PPLogger log;
	PPPredictConfig pr_cfg;
	PPBillPacket * p_pack = 0;
	SStatFilt  * p_filt = 0;
	PPObjArticle ar_obj;
	PPViewSStat view;
	PPIDArray list;
	PrcssrPrediction::GetPredictCfg(&pr_cfg);
	LDATE  cur_dt = getcurdate_();
	THROW_MEM(p_filt = (SStatFilt*)view.CreateFilt((void *)1));
	if(pFilt)
		r = p_filt->Copy(pFilt, 0);
	else
		r = view.EditBaseFilt(p_filt);
	PPWait(1);
	if(r > 0) {
		PPObjBill * p_bobj = BillObj;
		if(p_filt->SupplID)
			list.add(p_filt->SupplID);
		else
			THROW(ar_obj.P_Tbl->GetListBySheet(GetSupplAccSheet(), &list, 0));
		for(uint i = 0; i < list.getCount(); i++) {
			PPID   article_id = list.at(i);
			PPSupplAgreement suppl_agt;
			if(ar_obj.GetSupplAgreement(article_id, &suppl_agt, 1) > 0 && (suppl_agt.Flags & AGTF_AUTOORDER)) {
				for(uint j = 0; j < suppl_agt.OrderParamList.getCount(); j++) {
                    const PPSupplAgreement::OrderParamEntry & r_entry = suppl_agt.OrderParamList.at(j);
                    if(r_entry.OrdPrdDays && r_entry.Dr.Prd) {
						PPID   loc_id = 0;
						if(r_entry.LocID == 0 && p_filt->LocList.GetSingle())
							loc_id = p_filt->LocList.GetSingle();
						else if(p_filt->LocList.CheckID(r_entry.LocID))
							loc_id = r_entry.LocID;
						if(loc_id) {
							MEMSZERO(ord_entry);
							ord_entry.ArID = article_id;
							ord_entry.GoodsGrpID = r_entry.GoodsGrpID;
							ord_entry.LocID = loc_id;
							ord_entry.MngrID = r_entry.MngrID;
							ord_entry.OrdPrdDays = r_entry.OrdPrdDays;
							ord_entry.DuePrdDays = r_entry.Fb.DuePrdDays;
							ord_entry.Dr = r_entry.Dr;
							THROW_SL(ar_list.insert(&ord_entry));
						}
                    }
					else {
						GetArticleName(article_id, temp_buf = 0);
						msg_buf.Printf(PPLoadStringS(PPMSG_ERROR, PPERR_AUTOORDINVPARAM, fmt_buf), article_id, temp_buf.cptr());
						log.Log(msg_buf);
					}
				}
			}
		}
		{
			TSCollection <PPBillPacket> bill_pack_list;
			{
				const LDATE base_date = encodedate(1, 1, cur_dt.year());
				for(uint i = 0; i < ar_list.getCount(); i++) {
					ord_entry = *(_OrdArEntry*)ar_list.at(i);
					LDATE  doc_dt = ZERODATE;
					DateRepIterator dr_iter(ord_entry.Dr, base_date);
					do {
						doc_dt = dr_iter.Next();
					} while(doc_dt != ZERODATE && doc_dt < cur_dt);
					if(doc_dt == cur_dt) {
						p_filt->OrdTerm = ord_entry.OrdPrdDays;
						p_filt->SetupCfgOptions(pr_cfg);
						p_filt->SupplID = ord_entry.ArID;
						p_filt->GoodsGrpID = ord_entry.GoodsGrpID;
						if(view.Init_(p_filt) > 0) {
							int    r;
							THROW_MEM(p_pack = new PPBillPacket);
							r = view.CreatePurchaseBill(doc_dt, 1, p_pack, 1);
							if(ord_entry.DuePrdDays >= 0)
								p_pack->Rec.DueDate = plusdate(p_pack->Rec.Dt, ord_entry.DuePrdDays);
							if(r > 0) {
								int    skip = 0;
								MakeOrderAutocreationTag(ord_entry, doc_dt, temp_buf);
								PPIDArray ex_bill_list;
                                PPRef->Ot.SearchObjectsByStr(PPOBJ_BILL, PPTAG_BILL_AUTOCREATION, temp_buf, &ex_bill_list);
                                for(uint j = 0; !skip && j < ex_bill_list.getCount(); j++) {
                                	const PPID ex_bill_id = ex_bill_list.get(j);
                                	BillTbl::Rec ex_bill_rec;
									if(p_bobj->Search(ex_bill_id, &ex_bill_rec) > 0) {
										// PPTXT_SUPPLORDEXISTS         "ƒокумент заказа поставщику '%s' был создан ранее"
										PPObjBill::MakeCodeString(&ex_bill_rec, PPObjBill::mcsAddLocName|PPObjBill::mcsAddOpName|PPObjBill::mcsAddObjName, temp_buf);
										msg_buf.Printf(PPLoadStringS(PPSTR_TEXT, PPTXT_SUPPLORDEXISTS, fmt_buf), temp_buf.cptr());
										log.Log(msg_buf);
										skip = 1;
									}
                                }
                                if(!skip) {
									ObjTagItem tag;
									if(tag.SetStr(PPTAG_BILL_AUTOCREATION, MakeOrderAutocreationTag(ord_entry, doc_dt, temp_buf))) {
										p_pack->BTagL.PutItem(PPTAG_BILL_AUTOCREATION, &tag);
									}
									else {
										log.LogLastError();
									}
									THROW_SL(bill_pack_list.insert(p_pack));
									p_pack = 0;
                                }
							}
							else if(r == 0)
								log.LogLastError();
						}
					}
					PPWaitPercent(i + 1, ar_list.getCount());
				}
			}
			if(bill_pack_list.getCount()) {
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < bill_pack_list.getCount(); i++) {
					PPBillPacket * p_item = bill_pack_list.at(i);
					if(p_item) {
						if(p_bobj->TurnPacket(p_item, 0)) {
							PPObjBill::MakeCodeString(&p_item->Rec, PPObjBill::mcsAddLocName|PPObjBill::mcsAddOpName|PPObjBill::mcsAddObjName, temp_buf);
							msg_buf.Printf(PPLoadStringS(PPSTR_TEXT, PPTXT_SUPPLORDERCREATED, fmt_buf), temp_buf.cptr());
							log.Log(msg_buf);
						}
						else
							log.LogLastError();
					}
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCHZOKPPERR
	ZDELETE(p_filt);
	ZDELETE(p_pack);
	PPWait(0);
	return ok;
}

int SLAPI PPViewSStat::CalcTotal(SStatTotal * pTotal)
{
	int    ok = 1;
	if(pTotal) {
		memzero(pTotal, sizeof(*pTotal));
		SStatViewItem item;
		for(InitIteration(); NextIteration(&item) > 0;) {
			pTotal->LinesCount++;
			pTotal->Count += item.Count;
			pTotal->Qtty  += item.QttySum;
			pTotal->Amount += item.AmtSum;
			const double oq = item.SupplOrder;
			if(oq > 0.0) {
				pTotal->OrderCount++;
				pTotal->OrderQtty   += oq;
				pTotal->OrderAmount += (oq * item.PriceAvg);
				pTotal->OrderCost   += (oq * item.CostAvg); // @v7.1.7
				if(!item.IsPredictTrust)
					pTotal->UncertCount++;
			}
		}
	}
	return ok;
}

// virtual
int SLAPI PPViewSStat::ViewTotal()
{
	int    ok = 1;
	SStatTotal total;
	TDialog * dlg = 0;
	CalcTotal(&total);
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_SSTATTOTAL)), 1));
	dlg->setCtrlLong(CTL_SSTATTOTAL_LNCOUNT,   total.LinesCount);
	dlg->setCtrlLong(CTL_SSTATTOTAL_COUNT,     total.Count);
	dlg->setCtrlReal(CTL_SSTATTOTAL_QTTY,      total.Qtty);
	dlg->setCtrlReal(CTL_SSTATTOTAL_AMOUNT,    total.Amount);
	dlg->setCtrlLong(CTL_SSTATTOTAL_ORDCOUNT,  total.OrderCount);
	dlg->setCtrlReal(CTL_SSTATTOTAL_ORDQTTY,   total.OrderQtty);
	dlg->setCtrlReal(CTL_SSTATTOTAL_ORDCOST,   total.OrderCost); // @v7.1.7
	dlg->setCtrlReal(CTL_SSTATTOTAL_ORDAMOUNT, total.OrderAmount);
	dlg->setCtrlLong(CTL_SSTATTOTAL_UNCRTCNT,  total.UncertCount);
	ExecViewAndDestroy(dlg);
	CATCHZOKPPERR
	return ok;
}
//
// Implementation of PPALDD_SStatView
//
PPALDD_CONSTRUCTOR(SStatView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(SStatView)
{
	Destroy();
}

int PPALDD_SStatView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(SStat, rsrv);
	H.FltBeg   = p_filt->Period.low;
	H.FltEnd   = p_filt->Period.upp;
	H.FltCycle = p_filt->Cycl.Cycle;
	H.FltNumCycles = p_filt->Cycl.NumCycles;
	H.LocID      = p_filt->LocList.GetSingle();
	H.GoodsGrpID = p_filt->GoodsGrpID;
	H.SupplID  = p_filt->SupplID;
	H.RestDate = p_filt->RestDate;
	H.OrdTerm  = p_filt->OrdTerm;
	H.DlvrTerm = p_filt->DlvrTerm;
	H.Flags    = p_filt->Flags;
	H.fSkipZeroNhCount  = BIN(p_filt->Flags & SStatFilt::fSkipZeroNhCount);
	H.fSupplOrderForm   = BIN(p_filt->Flags & SStatFilt::fSupplOrderForm);
	H.fRoundOrderToPack = BIN(p_filt->Flags & SStatFilt::fRoundOrderToPack);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_SStatView::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(SStat);
}

int PPALDD_SStatView::NextIteration(PPIterID iterId, long rsrv)
{
	START_PPVIEW_ALDD_ITER(SStat);
	I.GoodsID   = item.GoodsID;
	STRNSCPY(I.GoodsName, item.GoodsName);
	I.Dt        = item.Dt;
	I.CycleText[0] = 0;
	I.Count      = item.Count;
	I.QttySum    = 0;
	I.QttyAvg    = item.QttyAvg;
	I.QttySigma  = item.QttySigma;
	I.QttyVar    = item.QttyVar;
	I.QttyTrnovr = item.QttyTrnovr;
	I.AmtSum     = 0;
	I.AmtAvg     = item.AmtAvg;
	I.AmtSigma   = item.AmtSigma;
	I.AmtVar     = item.AmtVar;
	I.AmtTrnovr  = item.AmtTrnovr;
	I.Rest       = item.Rest;
	I.Predict    = item.Predict;
	I.SupplOrder = item.SupplOrder;
	I.PriceAvg   = item.PriceAvg;
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

int PPALDD_SStatView::Destroy()
{
	DESTROY_PPVIEW_ALDD(PredictSales);
}

