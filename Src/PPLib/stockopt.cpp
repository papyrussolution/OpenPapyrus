// STOCKOPT.CPP
// Copirught (c) A.Sobolev 2011, 2015, 2016
//
#include <pp.h>
#pragma hdrstop

#define PPERR_STKOPT_UNDEFRATE  20001
#define PPERR_STKOPT_UNDEFGOODS 20002

PPStockOpt::Config::Config()
{
	MaxItems = 0;
	RateOfRet = 0.0;
	ExpirySafetyFactor = 0.5;
	MaxCost = 0.0;
	MaxSales = 0.0;
	memzero(Reserve, sizeof(Reserve));
}
//
//
//
PPStockOpt::Item::Item(PPID goodsID)
{
	THISZERO();
	if(goodsID)
		GoodsID = goodsID;
}

int PPStockOpt::Item::CanProcess() const
{
	return BIN((Flags & fPreproc) && !(Flags & (fUndefDemand|fUndefCost|fUndefPrice)));
}
//
//
//
PPStockOpt::GoodsResult::GoodsResult()
{
	THISZERO();
}
//
//
//
PPStockOpt::TotalResult::TotalResult()
{
	Init();
}

void PPStockOpt::TotalResult::Init()
{
	THISZERO();
}

PPStockOpt::TotalResult & PPStockOpt::TotalResult::Add(const PPStockOpt::Item & rGi, const GoodsResult & rGr)
{
	if(rGr.R > 0.0)
		Count++;
	Income += rGr.Income;
	Expend += rGr.Expend;
	SumCost += rGr.R * rGi.Cost;
	SalesPerDay += rGi.AvgD * rGi.Price;
	return *this;
}

PPStockOpt::TotalResult & PPStockOpt::TotalResult::Sub(const PPStockOpt::Item & rGi, const GoodsResult & rGr)
{
	if(rGr.R <= 0.0)
		Count--;
	Income -= rGr.Income;
	Expend -= rGr.Expend;
	SumCost -= (rGr.R * rGi.Cost);
	SalesPerDay -= (rGi.AvgD * rGi.Price);
	return *this;
}

PPStockOpt::PPStockOpt()
{
}

PPStockOpt::~PPStockOpt()
{
}

class StockOptCfgDialog : public TDialog {
public:
	StockOptCfgDialog(uint dlgId) : TDialog(dlgId)
	{
	}
	int    setDTS(PPStockOpt::Config * pData)
	{
		int    ok = 1;
		RVALUEPTR(Data.SoCfg, pData);
		DataSrc = 0;
		setCtrlReal(CTL_STOCKOPTCFG_RATE, Data.SoCfg.RateOfRet * 100.0);
		setCtrlReal(CTL_STOCKOPTCFG_EXPSAFC, Data.SoCfg.ExpirySafetyFactor);
		setCtrlReal(CTL_STOCKOPTCFG_MAXCOST, Data.SoCfg.MaxCost);
		setCtrlReal(CTL_STOCKOPTCFG_MAXSALES, Data.SoCfg.MaxSales);
		setCtrlLong(CTL_STOCKOPTCFG_MAXITEMS, Data.SoCfg.MaxItems);
		return ok;
	}
	int    setDTS(StockOptFilt * pData)
	{
		int    ok = 1;
		Data = *pData;
		setDTS((PPStockOpt::Config *)0);
		DataSrc = 1;

		AddClusterAssoc(CTL_STOCKOPTCFG_MODE, 0, StockOptFilt::modeGoods);
		AddClusterAssoc(CTL_STOCKOPTCFG_MODE, -1, StockOptFilt::modeGoods);
		AddClusterAssoc(CTL_STOCKOPTCFG_MODE, 1, StockOptFilt::modePreproc);
		AddClusterAssoc(CTL_STOCKOPTCFG_MODE, 2, StockOptFilt::modeOptimum);
		SetClusterData(CTL_STOCKOPTCFG_MODE, Data.Mode);
		return ok;
	}
	int    getDTS(PPStockOpt::Config * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		Data.SoCfg.RateOfRet = getCtrlReal(sel = CTL_STOCKOPTCFG_RATE) / 100.0;
		THROW_SL(checkfrange(Data.SoCfg.RateOfRet, 0.01, 1.0));
		Data.SoCfg.ExpirySafetyFactor = getCtrlReal(sel = CTL_STOCKOPTCFG_EXPSAFC);
		THROW_SL(checkfrange(Data.SoCfg.ExpirySafetyFactor, 0.1, 1.0));
		Data.SoCfg.MaxCost = getCtrlReal(sel = CTL_STOCKOPTCFG_MAXCOST);
		THROW_SL(checkfrange(Data.SoCfg.MaxCost, 0.0, 1.0E10));
		Data.SoCfg.MaxSales = getCtrlReal(sel = CTL_STOCKOPTCFG_MAXSALES);
		THROW_SL(checkfrange(Data.SoCfg.MaxSales, 0.0, 1.0E9));
		Data.SoCfg.MaxItems = (uint)getCtrlLong(sel = CTL_STOCKOPTCFG_MAXITEMS);
		THROW_SL(checkirange((long)Data.SoCfg.MaxItems, 0, 1000000));
		ASSIGN_PTR(pData, Data.SoCfg);
		CATCH
			ok = PPErrorByDialog(this, sel, PPERR_SLIB);
		ENDCATCH
		return ok;
	}
	int    getDTS(StockOptFilt * pData)
	{
		int    ok = 1;
		if(getDTS((PPStockOpt::Config *)0)) {
			Data.Mode = GetClusterData(CTL_STOCKOPTCFG_MODE);
			ASSIGN_PTR(pData, Data);
		}
		return ok;
	}
private:
	StockOptFilt Data;
	int    DataSrc; // 0 - cfg, 1 - filt
};

//static
int PPStockOpt::EditConfig()
{
	int    ok = -1;
	Config cfg;
	ReadConfig(&cfg);
	if(PPDialogProcBody <StockOptCfgDialog, PPStockOpt::Config> (DLG_STOCKOPTCFG, &cfg) > 0) {
		if(!WriteConfig(&cfg))
			ok = PPErrorZ();
		else
			ok = 1;
	}
	return ok;
}

struct __StockOptConfig {   // @persistent
	PPID   Tag;             // Const=PPOBJ_CONFIG
	PPID   ID;              // Const=PPCFG_MAIN
	PPID   Prop;            // Const=PPPRP_STOCKOPTCONFIG
	uint   MaxItems;        //
	double RateOfRet;       //
	double ExpirySafetyFactor; //
	double MaxCost;         //
	double MaxSales;        //
	uint8  Reserve[36];     //
};

// static
int PPStockOpt::WriteConfig(const PPStockOpt::Config * pCfg)
{
	int    ok = 1;
	if(pCfg) {
		__StockOptConfig rec, prev_rec;
		MEMSZERO(rec);
		rec.Tag = PPOBJ_CONFIG;
		rec.ID = PPCFG_MAIN;
		rec.Prop = PPPRP_STOCKOPTCFG;
		rec.MaxItems = pCfg->MaxItems;
		rec.RateOfRet = pCfg->RateOfRet;
		rec.ExpirySafetyFactor = pCfg->ExpirySafetyFactor;
		rec.MaxCost = pCfg->MaxCost;
		rec.MaxSales = pCfg->MaxSales;
		{
			int    is_new = 1;
			PPTransaction tra(1);
			THROW(tra);
			if(PPRef->GetProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_STOCKOPTCFG, &prev_rec, sizeof(prev_rec)) > 0) {
				is_new = 0;
				if(memcmp(&prev_rec, &rec, sizeof(rec)) == 0)
					ok = -1;
			}
			if(ok > 0) {
				THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_STOCKOPTCFG, &rec, sizeof(rec), 0));
				DS.LogAction(is_new ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_STOCKOPT, 0, 0, 0);
			}
			THROW(tra.Commit());
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

// static
int PPStockOpt::ReadConfig(PPStockOpt::Config * pCfg)
{
	int    ok = -1;
	__StockOptConfig rec;
	if(PPRef->GetProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_STOCKOPTCFG, &rec, sizeof(rec)) > 0) {
		if(pCfg) {
			pCfg->MaxItems = rec.MaxItems;
			pCfg->RateOfRet = rec.RateOfRet;
			pCfg->ExpirySafetyFactor = rec.ExpirySafetyFactor;
			pCfg->MaxCost = rec.MaxCost;
			pCfg->MaxSales = rec.MaxSales;
		}
		ok = 1;
	}
	return ok;
}

int PPStockOpt::Test()
{
	int    ok = 1;
	STabFile tabf("d:/papyrus/src/pptest/data/stockopt.tab");
	STabFile out_tabf("d:/papyrus/src/pptest/out/stockopt-result.tab", 1);
	STab   param_tab;
	STab   goods_tab;
	THROW(tabf.LoadTab("Param", param_tab));
	THROW(tabf.LoadTab("Goods", goods_tab));
	{
		STab::Row row;
		Config cfg;
		if(param_tab.GetCount()) {
			double v;
			param_tab.GetRow(0, row);
			if(row.Get(0, v))
				cfg.RateOfRet = v / 100.;
			if(row.Get(1, v) && v >= 0.0 && v < 1.0)
				cfg.ExpirySafetyFactor = v;
			if(row.Get(2, v))
				cfg.MaxCost = v;
			if(row.Get(3, v))
				cfg.MaxSales = v;
			SetConfig(&cfg);
		}
	}
	{
		for(uint i = 0; i < goods_tab.GetCount(); i++) {
			double v;
			STab::Row row;
			PPStockOpt::Item item;
			if(goods_tab.GetRow(i, row)) {
				if(row.Get(0, v))
					item.GoodsID = (long)v;
				if(row.Get(1, v))
					item.Cost = v;
				if(row.Get(2, v))
					item.Price = v;
				if(row.Get(3, v))
					item.AvgD = v;
				if(row.Get(4, v))
					item.Pckg = v;
				if(row.Get(5, v))
					item.ExpiryPeriod = (int16)v;
				if(item.GoodsID)
					AddGoodsItem(item);
			}
		}
	}
	THROW(Run());
	{
		STab result_tab;
		for(uint i = 0; i < Result.getCount(); i++) {
			const GoodsResult & r_result = Result.at(i);
			STab::Row row;
			row.Add(r_result.GoodsID);
			row.Add(r_result.State);
			row.Add(r_result.Bounds.low);
			row.Add(r_result.Bounds.upp);
			row.Add(r_result.R0);
			row.Add(r_result.R0p);
			row.Add(r_result.R);
			row.Add(r_result.Period);
			row.Add(r_result.Income);
			row.Add(r_result.Expend);
			row.Add(r_result.LastDelta);
			result_tab.AddRow(row);
		}
		out_tabf.WriteTab("Goods", &result_tab);
	}
	/*
	{
		STab total_tab;
		STab::Row row;
		row.
	}
	*/
	CATCHZOK
	return ok;
}

int PPStockOpt::SetConfig(const Config * pCfg)
{
	int    ok = 1;
	RVALUEPTR(Cfg, pCfg);
	return ok;
}

const PPStockOpt::Config & PPStockOpt::GetConfig() const
{
	return Cfg;
}

int PPStockOpt::AddGoodsItem(const PPStockOpt::Item & rItem)
{
	int    ok = 1;
	Items.insert(&rItem);
	return ok;
}

int PPStockOpt::UpdateGoodsItem(uint pos, const PPStockOpt::Item & rItem)
{
	int    ok = -1;
	if(pos < Items.getCount()) {
		Items.at(pos) = rItem;
		ok = 1;
	}
	return ok;
}

const TSArray <PPStockOpt::Item> & PPStockOpt::GetItems() const
{
	return Items;
}

const TSArray <PPStockOpt::GoodsResult> & PPStockOpt::GetResult() const
{
	return Result;
}

const PPStockOpt::TotalResult & PPStockOpt::GetTotal() const
{
	return Total;
}

int PPStockOpt::CheckTotalRestrictions(const TotalResult & rTotal)
{
	if(Cfg.MaxCost > 0.0 && rTotal.SumCost > Cfg.MaxCost)
		return 0;
	else if(Cfg.MaxSales > 0.0 && rTotal.SalesPerDay > Cfg.MaxSales)
		return 0;
	return 1;
}

int PPStockOpt::Run()
{
	int    ok = 1, r;
	uint   item_pos;
	Total.Init();
	Result.clear();
	DecrIndex.clear();
	for(item_pos = 0; item_pos < Items.getCount(); item_pos++) {
		GoodsResult result;
		PPStockOpt::Item & r_item = Items.at(item_pos);
		THROW(r = PreprocessGoods(r_item));
		if(r > 0) {
			THROW(ProcessGoods(r_item, pgOptimize, result));
		}
		Total.Add(r_item, result);
		THROW_SL(Result.insert(&result));
		DecrIndex.Add(item_pos+1, result.LastDelta, 0);
	}
	DecrIndex.SortByVal();
	{
		const uint di_count = DecrIndex.getCount();
		while(!CheckTotalRestrictions(Total)) {
			for(uint j = 0; j < di_count; j++) {
				double delta = DecrIndex.at(j).Val;
				if(delta < SMathConst::Max && delta > 0.0) {
					item_pos = DecrIndex.at(j).Key - 1;
					PPStockOpt::Item & r_item = Items.at(item_pos);
					GoodsResult & r_result = Result.at(item_pos);
					Total.Sub(r_item, r_result);
					THROW(r = ProcessGoods(r_item, pgDecrement, r_result));
					if(r > 0) {
						delta = r_result.LastDelta;
						Total.Add(r_item, r_result);
						DecrIndex.at(j).Val = delta;
						for(uint k = j+1; k < di_count; k++) {
							if(DecrIndex.at(k).Val < delta)
								DecrIndex.swap(k, j);
							else
								break;
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

double PPStockOpt::GetRate() const
{
	return Cfg.RateOfRet / 365.;
}

double FASTCALL PPStockOpt::Evaluate(const PPStockOpt::Item & rItem, int evv, double param) const
{
	double result = 0.0;
	switch(evv) {
		case evvOptRest:
			result = (rItem.AvgD / GetRate()) * log(rItem.Price / rItem.Cost);
			break;
		case evvOptRestDiscr:
			{
				double v = Evaluate(rItem, evvOptRest, param);
				if(rItem.Pckg > 0.0) {
					double v0, v1;
					double ip;
					double frac = modf(v / rItem.Pckg, &ip);
					if(frac != 0.0) {
						double p = ip * rItem.Pckg;
						v0 = Evaluate(rItem, evvProfit, p);
						v1 = Evaluate(rItem, evvProfit, p + rItem.Pckg);
						if(v0 < v1)
							result = (p + rItem.Pckg);
						else
							result = p;
					}
					else
						result = v;
				}
			}
			break;
		case evvMinValAdd:
			if(rItem.Pckg > 0.0) {
				result = (rItem.AvgD / rItem.Pckg) * exp(GetRate() * rItem.Pckg / rItem.AvgD) / GetRate();
			}
			break;
		case evvMinDemand:
			{
			}
			break;
		case evvIncome:
			result = rItem.Price * param;
			break;
		case evvExpend:
			{
				double r = GetRate();
				result = rItem.Cost * rItem.AvgD * ((exp(r * param / rItem.AvgD) - 1) / r);
			}
			break;
		case evvProfit:
			result = Evaluate(rItem, evvIncome, param) - Evaluate(rItem, evvExpend, param);
			break;
		case evvMinStock:
			{
				result = param;
				if(result < rItem.MinRest) {
					double addendum = rItem.MinRest - result;
					if(rItem.Pckg > 0.0)
						addendum = round(addendum, rItem.Pckg, +1);
					result += addendum;
				}
			}
			break;
		case evvMaxStock:
			{
				if(rItem.ExpiryPeriod > 0) {
					double ep = rItem.ExpiryPeriod * Cfg.ExpirySafetyFactor;
					result = ep * rItem.AvgD;
					double addendum = result - param;
					if(rItem.Pckg > 0.0)
						addendum = round(addendum, rItem.Pckg, -1);
					result += addendum;
				}
				else
					result = SMathConst::Max;
			}
			break;
	}
	return result;
}

int PPStockOpt::Evaluate(const PPStockOpt::Item & rItem, int evv, double param, GoodsResult & rResult) const
{
	int    ok = 1;
	rResult.GoodsID = rItem.GoodsID;
	switch(evv) {
		case evvAll:
			rResult.R = param;
			rResult.R0 = Evaluate(rItem, evvOptRest, param);
			rResult.R0p = Evaluate(rItem, evvOptRestDiscr, param);
			rResult.Income = Evaluate(rItem, evvIncome, param);
			rResult.Expend = Evaluate(rItem, evvExpend, param);
			break;
		case evvTarget:
			rResult.R = param;
			rResult.Income = Evaluate(rItem, evvIncome, param);
			rResult.Expend = Evaluate(rItem, evvExpend, param);
			break;
		case evvOptRest:
			rResult.R0 = Evaluate(rItem, evv, param);
			break;
		case evvOptRestDiscr:
			rResult.R0p = Evaluate(rItem, evv, param);
			break;
		case evvMinValAdd:
			{
			}
			break;
		case evvMinDemand:
			{
			}
			break;
		case evvIncome:
			rResult.R = param;
			rResult.Income = Evaluate(rItem, evv, param);
			break;
		case evvExpend:
			rResult.R = param;
			rResult.Expend = Evaluate(rItem, evv, param);
			break;
		case evvProfit:
			rResult.R = param;
			rResult.Income = Evaluate(rItem, evv, param);
			rResult.Expend = Evaluate(rItem, evv, param);
			break;
		case evvMinStock:
			rResult.Bounds.low = Evaluate(rItem, evv, param);
			break;
		case evvMaxStock:
			rResult.Bounds.upp = Evaluate(rItem, evv, param);
			break;
		case evvStockBounds:
			rResult.Bounds.low = Evaluate(rItem, evvMinStock, param);
			rResult.Bounds.upp = Evaluate(rItem, evvMaxStock, param);
			break;
	}
	return ok;
}

int PPStockOpt::PreprocessGoods(PPStockOpt::Item & rItem)
{
	int    ok = 1;
	rItem.Flags |= PPStockOpt::Item::fPreproc;
	THROW_PP(Cfg.RateOfRet > 0.0, PPERR_STKOPT_UNDEFRATE);
	THROW_PP(rItem.GoodsID > 0, PPERR_STKOPT_UNDEFGOODS);
	if(rItem.AvgD <= 0.0) {
		rItem.Flags |= PPStockOpt::Item::fUndefDemand;
		ok = -1;
	}
	if(rItem.Cost <= 0.0) {
		rItem.Flags |= PPStockOpt::Item::fUndefCost;
		ok = -1;
	}
	if(rItem.Price <= 0.0) {
		rItem.Flags |= PPStockOpt::Item::fUndefPrice;
		ok = -1;
	}
	if(rItem.Pckg <= 0.0) {
		if(rItem.AvgD > 1.0) {
			rItem.Pckg = round(rItem.AvgD, 2.0, +1);
		}
		else
			rItem.Pckg = 1.0;
		rItem.Flags |= PPStockOpt::Item::fPckgSynth;
	}
	CATCHZOK
	return ok;
}

int PPStockOpt::ProcessGoods(const PPStockOpt::Item & rItem, long mode, GoodsResult & rResult) const
{
	int    ok = 1;
	if(rItem.CanProcess()) {
		rResult.GoodsID = rItem.GoodsID;
		if(mode == pgOptimize) {
			Evaluate(rItem, evvStockBounds, rItem.InRest, rResult);
			if(rResult.Bounds.GetDistance() < 0.0) {
				rResult.State |= GoodsResult::stMinMaxBoundError;
			}
			if(rResult.Bounds.GetDistance() == 0.0) {
				rResult.State |= (GoodsResult::stMinBound | GoodsResult::stMaxBound);
				rResult.R = rResult.Bounds.low;
			}
			else {
				double opt = Evaluate(rItem, evvOptRestDiscr, 0.0);
				if(opt >= rResult.Bounds.upp) {
					opt = rResult.Bounds.upp;
					Evaluate(rItem, evvTarget, opt, rResult);
					rResult.State |= GoodsResult::stMaxBound;
					rResult.LastDelta = rResult.Income - rResult.Expend - Evaluate(rItem, evvProfit, rResult.R-rItem.Pckg);
				}
				else if(opt <= rResult.Bounds.low) {
					opt = rResult.Bounds.low;
					Evaluate(rItem, evvTarget, opt, rResult);
					rResult.State |= GoodsResult::stMinBound;
					rResult.LastDelta = SMathConst::Max;
				}
				else {
					Evaluate(rItem, evvTarget, opt, rResult);
					rResult.LastDelta = rResult.Income - rResult.Expend - Evaluate(rItem, evvProfit, rResult.R-rItem.Pckg);
				}
			}
		}
		else if(mode == pgDecrement) {
			if(!(rResult.State & GoodsResult::stMinBound)) {
				double opt = rResult.R - rItem.Pckg;
				if(opt <= rResult.Bounds.low) {
					opt = rResult.Bounds.low;
					Evaluate(rItem, evvTarget, opt, rResult);
					rResult.State |= GoodsResult::stMinBound;
					rResult.LastDelta = SMathConst::Max;
				}
				else {
					Evaluate(rItem, evvTarget, opt, rResult);
					rResult.LastDelta = rResult.Income - rResult.Expend - Evaluate(rItem, evvProfit, rResult.R-rItem.Pckg);
				}
			}
			else
				ok = -1;
		}
	}
	else
		ok = -1;
	return ok;
}
//
// @ModuleDef(PPViewStockOpt)
//
IMPLEMENT_PPFILT_FACTORY(StockOpt); SLAPI StockOptFilt::StockOptFilt() : PPBaseFilt(PPFILT_STOCKOPT, 0, 1)
{
	SetFlatChunk(offsetof(StockOptFilt, ReserveStart),
		offsetof(StockOptFilt, Reserve) - offsetof(StockOptFilt, ReserveStart) + sizeof(Reserve));
	Init(1, 0);
	Mode = modeGoods;
}

PPViewStockOpt::PPViewStockOpt() : PPView(0, &Filt, PPVIEW_STOCKOPT)
{
	ImplementFlags |= implBrowseArray;
}

PPViewStockOpt::~PPViewStockOpt()
{
}

PPBaseFilt * SLAPI PPViewStockOpt::CreateFilt(void * extraPtr) const
{
	StockOptFilt * p_filt = new StockOptFilt;
	if(p_filt) {
		PPStockOpt::ReadConfig(&p_filt->SoCfg);
	}
	return p_filt;
}

int SLAPI PPViewStockOpt::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return PPErrorZ();
	StockOptFilt * p_filt = (StockOptFilt *)pBaseFilt;
	return p_filt ? PPDialogProcBody <StockOptCfgDialog, StockOptFilt> (DLG_STOCKOPTFLT, p_filt) : 0;
}

int SLAPI PPViewStockOpt::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	if(!Helper_InitBaseFilt(pBaseFilt))
		ok = 0;
	return ok;
}

#define CTLGRP_GOODS 1

class StockOptItemDialog : public TDialog {
public:
	StockOptItemDialog() : TDialog(DLG_STOCKOPTITEM)
	{
		addGroup(CTLGRP_GOODS, new GoodsCtrlGroup(CTLSEL_SOI_GOODSGRP, CTLSEL_SOI_GOODS));
	}
	int    setDTS(const PPStockOpt::Item * pData)
	{
		int    ok = 1;
		Data = *pData;
		GoodsCtrlGroup::Rec grp_rec;
		MEMSZERO(grp_rec);
		grp_rec.GoodsID = Data.GoodsID;
		grp_rec.Flags   = (GoodsCtrlGroup::enableSelUpLevel|GoodsCtrlGroup::disableEmptyGoods);
		setGroupData(CTLGRP_GOODS, &grp_rec);

		setCtrlReal(CTL_SOI_COST, Data.Cost);
		setCtrlReal(CTL_SOI_PRICE, Data.Price);
		setCtrlReal(CTL_SOI_DEMAND, Data.AvgD);
		setCtrlReal(CTL_SOI_MINREST, Data.MinRest);
		setCtrlReal(CTL_SOI_INREST, Data.InRest);
		setCtrlReal(CTL_SOI_PCKG, Data.Pckg);
		setCtrlLong(CTL_SOI_EXPIRYPERIOD, Data.ExpiryPeriod);
		return ok;
	}
	int    getDTS(PPStockOpt::Item * pData)
	{
		int    ok = 1;
		GoodsCtrlGroup::Rec grp_rec;
		THROW(getGroupData(CTLGRP_GOODS, &grp_rec));
		Data.GoodsID = grp_rec.GoodsID;

		Data.Cost = getCtrlReal(CTL_SOI_COST);
		Data.Price = getCtrlReal(CTL_SOI_PRICE);
		Data.AvgD = getCtrlReal(CTL_SOI_DEMAND);
		Data.MinRest = getCtrlReal(CTL_SOI_MINREST);
		Data.InRest = getCtrlReal(CTL_SOI_INREST);
		Data.Pckg = getCtrlReal(CTL_SOI_PCKG);
		Data.ExpiryPeriod = (int16)getCtrlLong(CTL_SOI_EXPIRYPERIOD);
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = PPErrorZ();
		ENDCATCH
		return ok;
	}
private:
	PPStockOpt::Item Data;
};

int SLAPI PPViewStockOpt::EditItem(PPStockOpt::Item * pItem)
{
	DIALOG_PROC_BODY(StockOptItemDialog, pItem);
}

int SLAPI PPViewStockOpt::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const long _pos = *(long *)pBlk->P_SrcData;
		PPStockOpt::Item * p_item = (_pos >= 0 && _pos < (long)So.GetItems().getCount()) ? &So.GetItems().at(_pos) : 0;
		PPStockOpt::GoodsResult * p_result = (_pos >= 0 && _pos < (long)So.GetResult().getCount()) ? &So.GetResult().at(_pos) : 0;
		switch(pBlk->ColumnN) {
			case 0:
				pBlk->Set(p_item ? p_item->GoodsID : (p_result ? p_result->GoodsID : 0));
				break;
			case 1:
				GetGoodsName(p_item ? p_item->GoodsID : (p_result ? p_result->GoodsID : 0), pBlk->TempBuf);
				pBlk->Set(pBlk->TempBuf);
				break;
			case 2:
				pBlk->Set(p_item ? p_item->Cost : 0.0);
				break;
			case 3:
				pBlk->Set(p_item ? p_item->Price : 0.0);
				break;
			case 4:
				pBlk->Set(p_item ? p_item->AvgD : 0.0);
				break;
			case 5:
				pBlk->Set(p_item ? p_item->Pckg : 0.0);
				break;
			case 6:
				pBlk->Set(p_item ? p_item->MinRest : 0.0);
				break;
			case 7:
				pBlk->Set(p_item ? p_item->ExpiryPeriod : (int32)0);
				break;
			case 8:
				pBlk->Set(p_item ? p_item->InRest : 0.0);
				break;
			case 21:
				pBlk->Set(p_result ? p_result->Bounds.low : 0.0);
				break;
			case 22:
				pBlk->Set(p_result ? p_result->Bounds.upp : 0.0);
				break;
			case 23:
				pBlk->Set(p_result ? p_result->R0 : 0.0);
				break;
			case 24:
				pBlk->Set(p_result ? p_result->R0p : 0.0);
				break;
			case 25:
				pBlk->Set(p_result ? p_result->R : 0.0);
				break;
			case 26:
				pBlk->Set(p_result ? (long)R0(p_result->Period) : 0L);
				break;
			case 27:
				pBlk->Set(p_result ? p_result->Income : 0.0);
				break;
			case 28:
				pBlk->Set(p_result ? p_result->Expend : 0.0);
				break;
			case 29:
				pBlk->Set(p_result ? (p_result->Income - p_result->Expend) : 0.0);
				break;
			case 30:
				pBlk->Set(p_result ? p_result->LastDelta : 0.0);
				break;
		}
	}
	return ok;
}

// static
int PPViewStockOpt::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewStockOpt * p_v = (PPViewStockOpt *)pBlk->ExtraPtr;
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

int SLAPI PPViewStockOpt::PreprocessBrowser(PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewStockOpt::GetDataForBrowser, this);
		ok = 1;
	}
	return ok;
}

SArray * SLAPI PPViewStockOpt::Helper_CreateBrowserArray()
{
	LongArray * p_list = new LongArray;
	if(Filt.Mode == Filt.modeGoods) {
		const uint c = So.GetItems().getCount();
		for(uint i = 0; i < c; i++)
			p_list->add(i);
	}
	else if(oneof2(Filt.Mode, Filt.modePreproc, Filt.modeOptimum)) {
		const uint c = So.GetResult().getCount();
		for(uint i = 0; i < c; i++)
			p_list->add(i);
	}
	return p_list;
}

SArray * SLAPI PPViewStockOpt::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = 0;
	if(Filt.Mode == Filt.modeGoods) {
		brw_id = BROWSER_STOCKOPT_ITEMS;
	}
	else if(oneof2(Filt.Mode, Filt.modePreproc, Filt.modeOptimum)) {
		brw_id = BROWSER_STOCKOPT_RESULT;
	}
	else {
		brw_id = BROWSER_STOCKOPT_ITEMS;
	}
	ASSIGN_PTR(pBrwId, brw_id);
	return Helper_CreateBrowserArray();
}

int SLAPI PPViewStockOpt::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	long   _pos = pHdr ? *(long*)pHdr : 0;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				{
					TIDlgInitData tidi;
					ExtGoodsSelDialog * dlg = 0;
					if(CheckDialogPtr(&(dlg = new ExtGoodsSelDialog(0, 0)))) {
						while(ExecView(dlg) == cmOK) {
							if(dlg->getDTS(&tidi) > 0 && tidi.GoodsID) {
								PPStockOpt::Item item(tidi.GoodsID);
								if(EditItem(&item) > 0 && item.GoodsID) {
									So.AddGoodsItem(item);
									ok = 1;
								}
							}
						}
					}
					delete dlg;
				}
				break;
			case PPVCMD_EDITITEM:
				ok = -1;
				if(_pos >= 0 && _pos < (long)So.GetItems().getCount()) {
					PPStockOpt::Item item = So.GetItems().at(_pos);
					if(EditItem(&item) > 0) {
						So.UpdateGoodsItem(_pos, item);
						ok = 1;
					}
				}
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				break;
		}
	}
	if(ok > 0 && oneof3(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM, PPVCMD_DELETEITEM)) {
		AryBrowserDef * p_def = (AryBrowserDef *)pBrw->getDef();
		if(p_def)
			p_def->setArray(Helper_CreateBrowserArray(), 0, 1);
		ok = 1;
	}
	return ok;
}
