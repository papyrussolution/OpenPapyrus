// V_SELL.CPP
// Copyright (c) A.Starodub, A.Sobolev 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
//
#include <pp.h>
#pragma hdrstop
//
//
//
IMPLEMENT_PPFILT_FACTORY(PredictSales); PredictSalesFilt::PredictSalesFilt() : PPBaseFilt(PPFILT_PREDICTSALES, 0, 0)
{
	SetFlatChunk(offsetof(PredictSalesFilt, ReserveStart),
		offsetof(PredictSalesFilt, SubstName) - offsetof(PredictSalesFilt, ReserveStart));
	SetBranchSString(offsetof(PredictSalesFilt, SubstName));
	SetBranchObjIdListFilt(offsetof(PredictSalesFilt, LocList));
	SetBranchObjIdListFilt(offsetof(PredictSalesFilt, GoodsIdList));
	Init(1, 0);
}

void PredictSalesFilt::SetGoodsList(const PPIDArray * pList, const char * pSubstText)
{
	GoodsIdList.Set(pList);
	GoodsID = 0;
	SubstName = pSubstText;
}
//
// PPViewPredictSales
//
PPViewPredictSales::PPViewPredictSales() : PPView(0, &Filt, PPVIEW_PREDICTSALES, 0, REPORT_PSALESVIEW), P_TempTbl(0)
{
}

PPViewPredictSales::~PPViewPredictSales()
{
	delete P_TempTbl;
}

int PPViewPredictSales::InitCycleList(const PPIDArray * pGoodsList)
{
	int    ok = -1;
	DateRange period;
	CycleList.freeAll();
	if(Filt.Cycle && Tbl.GetPeriod(&Filt.LocList, pGoodsList, &period) > 0) {
		if(Filt.Watershed) {
			CycleList.init(period.low, Filt.Watershed, -Filt.Cycle, 0);
			PPCycleArray c2;
			if(Filt.Period.upp)
				c2.init(plusdate(Filt.Watershed, 1), Filt.Period.upp, Filt.Cycle, 0);
			else
				c2.init(plusdate(Filt.Watershed, 1), ZERODATE, Filt.Cycle, 1);
			CycleList.concat(&c2);
		}
		else
			CycleList.init(&period, Filt.Cycle, 0);
		CycleList.getCycleParams(&Filt.Period, 0);
		ok = 1;
	}
	if(ok < 0)
		Filt.Cycle = 0;
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempPredictSales);

int PPViewPredictSales::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	uint   i;
	PsiArray list;
	ObjIdListFilt loc_list;
	BExtInsert * p_bei = 0;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	ZDELETE(P_TempTbl);
	ErrAvg = 0.0;
	MEMSZERO(TP);
	if(!Filt.GoodsIdList.IsExists())
		Filt.GoodsIdList.Add(Filt.GoodsID);
	if(Filt.LocList.IsEmpty()) {
		loc_list.Add(-1);
		Filt.LocList.Add(-1);
	}
	else
		loc_list = Filt.LocList;
	InitCycleList(&Filt.GoodsIdList.Get());
	THROW(Tbl.GetSeries(Filt.GoodsIdList.Get(), loc_list, &Filt.Period, &list));
	if(CycleList.getCount()) {
		PsiArray list2;
		list.ShrinkByCycleList(&CycleList, &list2);
		list.copy(list2);
	}
	{
		LDATE  prev = ZERODATE;
		PredictSalesItem * p_item;
		THROW(P_TempTbl = CreateTempFile());
		THROW_MEM(p_bei = new BExtInsert(P_TempTbl));
		for(i = 0; list.enumItems(&i, (void **)&p_item);) {
			TempPredictSalesTbl::Rec rec;
			// @v10.6.8 @ctr MEMSZERO(rec);
			const LDATE this_date = p_item->Dt;
			rec.Dt = this_date;
			rec.Qtty = p_item->Qtty;
			rec.Amt  = p_item->Amount;
			THROW_DB(p_bei->insert(&rec));
			if(Filt.Flags & PredictSalesFilt::fShowNoData) {
				if(prev && diffdate(this_date, prev) > 1) {
					for(LDATE dt = plusdate(prev, 1); dt < this_date; dt = plusdate(dt, 1)) {
						MEMSZERO(rec);
						rec.Dt = dt;
						rec.Flags |= 0x0001;
						THROW_DB(p_bei->insert(&rec));
					}
				}
			}
			prev = this_date;
		}
		THROW_DB(p_bei->flash());
	}
	CATCH
		ok = 0;
		ZDELETE(P_TempTbl);
	ENDCATCH
	return ok;
}

int PPViewPredictSales::InitIteration(int aOrder)
{
	int    ok = 1;
	BExtQuery::ZDelete(&P_IterQuery);
	CurIterOrd = aOrder;
	if(P_TempTbl) {
		TempPredictSalesTbl::Key0 tk0, tk0_;
		THROW_MEM(P_IterQuery = new BExtQuery(P_TempTbl, 0));
		P_IterQuery->selectAll();
		MEMSZERO(tk0);
		tk0_ = tk0;
		Counter.Init(P_IterQuery->countIterations(0, &tk0_, spFirst));
		P_IterQuery->initIteration(false, &tk0, spFirst);
	}
	CATCHZOK
	return ok;
}

int FASTCALL PPViewPredictSales::NextIteration(PredictSalesViewItem *pItem)
{
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		PredictSalesViewItem item;
		MEMSZERO(item);
		const TempPredictSalesTbl::Rec & r_src_rec = P_TempTbl->data;
		item.Dt = r_src_rec.Dt;
		item.Qtty = r_src_rec.Qtty;
		item.Amt = r_src_rec.Amt;
		item.QttyPredict = r_src_rec.QttyPredict;
		item.QttyAbsErr = r_src_rec.QttyAbsErr;
		item.QttyPctErr = r_src_rec.QttyPctErr;
		item.AmtPredict = r_src_rec.AmtPredict;
		item.AmtAbsErr = r_src_rec.AmtAbsErr;
		item.AmtPctErr = r_src_rec.AmtPctErr;
		Counter.Increment();
		ASSIGN_PTR(pItem, item);
		return 1;
	}
	return -1;
}

void PPViewPredictSales::FormatCycle(LDATE dt, char * pBuf, size_t bufLen) const
{
	// @todo use PPView::Helper_FormatCycle
	if(Filt.Cycle)
		CycleList.formatCycle(dt, pBuf, bufLen);
	else
		ASSIGN_PTR(pBuf, 0);
}

int PPViewPredictSales::CalcTotal(PredictSalesTotal * pTotal)
{
	double qttysqsum = 0.0;
	double amtsqsum  = 0.0;
	memzero(pTotal, sizeof(*pTotal));
	PredictSalesViewItem item;
	for(InitIteration(); NextIteration(&item) > 0;) {
		pTotal->Count++;
		pTotal->QttySum += item.Qtty;
		pTotal->AmtSum += item.Amt;
		qttysqsum += item.Qtty * item.Qtty;
		amtsqsum += item.Amt * item.Amt;
	}
	if(pTotal->Count) {
		pTotal->QttyAverage = pTotal->QttySum / pTotal->Count;
		pTotal->AmtAverage  = pTotal->AmtSum  / pTotal->Count;
		if(pTotal->Count > 1) {
			pTotal->QttySigma = (qttysqsum - pTotal->QttyAverage * pTotal->QttySum) / (pTotal->Count - 1);
			pTotal->QttyStdDev = sqrt(pTotal->QttySigma);
			pTotal->AmtSigma = (amtsqsum - pTotal->AmtAverage * pTotal->AmtSum) / (pTotal->Count - 1);
			pTotal->AmtStdDev = sqrt(pTotal->AmtSigma);
		}
	}
	return 1;
}

int PPViewPredictSales::RecalcGoods(const BrwHdr * pHdr)
{
	int    ok = -1, r;
	if(pHdr && pHdr->GoodsID) {
		PrcssrPrediction prcssr;
		PrcssrPrediction::Param param;
		prcssr.InitParam(&param);
		param.GoodsID = pHdr->GoodsID;
		DateRange period = param.GetPeriod();
		THROW(r = prcssr.GetLastUpdate(pHdr->GoodsID, Filt.LocList, &period.upp));
		if(r < 0) {
			period.upp = getcurdate_();
		}
		param.SetPeriod(period);
		param.Replace  = PrcssrPrediction::Param::rsUpdateOnly;
		param.Process |= PrcssrPrediction::Param::prcsFillSales|PrcssrPrediction::Param::prcsFillHoles|PrcssrPrediction::Param::prcsFillModel;
		if(prcssr.EditParam(&param) > 0) {
			PPWaitStart();
			THROW(prcssr.Init(&param));
			THROW(prcssr.Run());
			PPWaitStop();
			ok = 1;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewPredictSales::TestPrediction(const BrwHdr *)
{
	return -1;
}

int PPViewPredictSales::ViewGraph(PPViewBrowser * pBrw)
{
	struct PlotDataEntry {
		LDATE  Dt;
		double Qtty;
		double Amt;
	};
	int    ok = -1;
	const  int col = pBrw ? pBrw->GetCurColumn() : -1;
	int    amt_or_qtty = (col >= 5 && col <= 8) ? 1 : 0;
	SString fmt_buf, temp_buf;
	SArray data_list(sizeof(PlotDataEntry));
	Generator_GnuPlot plot(0);
	Generator_GnuPlot::PlotParam param;
	double yticinc = 0.0;
	{
		RealRange qtty_range;
		RealRange amt_range;
		qtty_range.Set(SMathConst::Max, -SMathConst::Max);
		amt_range.Set(SMathConst::Max, -SMathConst::Max);
		PredictSalesViewItem item;
		for(InitIteration(); NextIteration(&item) > 0;) {
			PlotDataEntry entry;
			MEMSZERO(entry);
			entry.Dt = item.Dt;
			entry.Qtty = item.Qtty;
			entry.Amt = item.Amt;
			qtty_range.SetupMinMax(item.Qtty);
			amt_range.SetupMinMax(item.Amt);
			data_list.insert(&entry);
		}
		yticinc = amt_or_qtty ? (amt_range.GetDistance() / 30) : (qtty_range.GetDistance() / 30);
		yticinc = PPRound(yticinc, 10.0, +1);
	}
	if(data_list.getCount()) {
		LDATE low_date = static_cast<const PlotDataEntry *>(data_list.at(0))->Dt;
		const int dow = dayofweek(&low_date, 1);
		if(dow != 1)
			low_date = plusdate(low_date, -(dow-1));
		param.Flags |= Generator_GnuPlot::PlotParam::fLines;
		plot.Preamble();
		plot.SetTitle((temp_buf = pBrw->getTitle()).Transf(CTRANSF_INNER_TO_OUTER));
		plot.SetDateTimeFormat(Generator_GnuPlot::axX);
		plot.SetGrid();
		{
			int axis = Generator_GnuPlot::axX;
			plot.UnsetTics(axis);
			Generator_GnuPlot::StyleTics tics;
			tics.Rotate = 90;
			tics.Font.Size = 8;
			PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIALCYR, tics.Font.Face);
			plot.SetTics(axis, &tics);
			plot.SetTicsInc(axis, 7*3600*24, low_date, ZERODATE);
			PPGetSubStr(PPTXT_PLOT_SALES, 0, temp_buf);
			plot.SetAxisTitle(axis, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
		}
		{
			int axis = Generator_GnuPlot::axY;
			plot.UnsetTics(axis);
			Generator_GnuPlot::StyleTics tics;
			tics.Rotate = 0;
			tics.Font.Size = 8;
			PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIALCYR, tics.Font.Face);
			plot.SetTics(axis, &tics);
			plot.SetTicsInc(axis, yticinc);
			PPGetSubStr(PPTXT_PLOT_SALES, amt_or_qtty ? 2 : 1, fmt_buf);
			{
				SString unit_buf;
				if(amt_or_qtty) {
					if(LConfig.BaseCurID)
						GetCurSymbText(LConfig.BaseCurID, unit_buf);
					else
						unit_buf = "RUB";
				}
				else {
					if(Filt.GoodsID) {
						PPObjGoods goods_obj;
						Goods2Tbl::Rec goods_rec;
						PPUnit unit_rec;
						if(goods_obj.Fetch(Filt.GoodsID, &goods_rec) > 0 && goods_obj.FetchUnit(goods_rec.UnitID, &unit_rec) > 0)
							unit_buf = unit_rec.Name;
					}
					if(unit_buf.IsEmpty())
						PPGetSubStr(PPTXT_PLOT_SALES, 3, unit_buf);
				}
				temp_buf.Printf(fmt_buf, unit_buf.cptr());
			}
			plot.SetAxisTitle(axis, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
		}
		{
			PPGpPlotItem item(plot.GetDataFileName(), 0, PPGpPlotItem::sLines);
			item.Style.SetLine(GetColorRef(amt_or_qtty ? SClrBlue : SClrGreen), 1.5);
			item.AddDataIndex(1);
			item.AddDataIndex(amt_or_qtty ? 3 : 2);
			plot.AddPlotItem(item);
		}
		plot.Plot(&param);
		plot.StartData(1);
		for(uint i = 0; i < data_list.getCount(); i++) {
			const PlotDataEntry * p_entry = static_cast<const PlotDataEntry *>(data_list.at(i));
			plot.PutData(p_entry->Dt);
			plot.PutData(p_entry->Qtty);
			plot.PutData(p_entry->Amt);
			plot.PutEOR();
		}
		plot.PutEndOfData();
		ok = plot.Run();
	}
	return ok;
}

void PPViewPredictSales::ViewTotal()
{
	PredictSalesTotal total;
	CalcTotal(&total);
	TDialog * dlg = new TDialog(DLG_PSALESTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		double qtty_trnovr = fdivnz(1.0, total.QttyAverage);
		double amt_trnovr  = fdivnz(1.0, total.AmtAverage);
		double qtty_var    = fdivnz(total.QttyStdDev, total.QttyAverage);
		double amt_var     = fdivnz(total.AmtStdDev,  total.AmtAverage);
		dlg->setCtrlData(CTL_PSALESTOTAL_COUNT,  &total.Count);
		dlg->setCtrlData(CTL_PSALESTOTAL_QTYSUM, &total.QttySum);
		dlg->setCtrlData(CTL_PSALESTOTAL_QTYAVG, &total.QttyAverage);
		dlg->setCtrlData(CTL_PSALESTOTAL_QTYTRN, &qtty_trnovr);
		dlg->setCtrlData(CTL_PSALESTOTAL_QTYSIGM, &total.QttySigma);
		dlg->setCtrlData(CTL_PSALESTOTAL_QTYSTDDV, &total.QttyStdDev);
		dlg->setCtrlData(CTL_PSALESTOTAL_QTYVAR, &qtty_var);

		dlg->setCtrlData(CTL_PSALESTOTAL_AMTSUM, &total.AmtSum);
		dlg->setCtrlData(CTL_PSALESTOTAL_AMTAVG, &total.AmtAverage);
		dlg->setCtrlData(CTL_PSALESTOTAL_AMTTRN, &amt_trnovr);
		dlg->setCtrlData(CTL_PSALESTOTAL_AMTSIGM, &total.AmtSigma);
		dlg->setCtrlData(CTL_PSALESTOTAL_AMTSTDDV, &total.AmtStdDev);
		dlg->setCtrlData(CTL_PSALESTOTAL_AMTVAR, &amt_var);
		ExecViewAndDestroy(dlg);
	}
}

#define GRP_GOODS 1
#define GRP_LOC   2

int PPViewPredictSales::EditBaseFilt(PPBaseFilt * pBaseFilt /*PredictSalesFilt * pFilt*/)
{
	int    ok = -1;
	TDialog * dlg = 0;
	PredictSalesFilt * p_filt = 0;
	THROW(Filt.IsA(pBaseFilt));
	p_filt = static_cast<PredictSalesFilt *>(pBaseFilt);
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PSALESFLT))));
	{
		dlg->SetupCalPeriod(CTLCAL_PSALESFLT_PERIOD, CTL_PSALESFLT_PERIOD);
		dlg->SetupCalDate(CTLCAL_PSALESFLT_WATERSHED, CTL_PSALESFLT_WATERSHED);
		SetPeriodInput(dlg, CTL_PSALESFLT_PERIOD, &p_filt->Period);
		dlg->addGroup(GRP_GOODS, new GoodsCtrlGroup(CTLSEL_PSALESFLT_GGROUP, CTLSEL_PSALESFLT_GOODS));
		GoodsCtrlGroup::Rec rec(p_filt->GoodsGrpID, p_filt->GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
		dlg->setGroupData(1, &rec); // 1 == GRP_GOODS
		if(p_filt->GoodsIdList.GetCount() > 1) {
			dlg->disableCtrls(1, CTLSEL_PSALESFLT_GOODS, CTLSEL_PSALESFLT_GGROUP, 0);
			if(Filt.SubstName.NotEmptyS())
				SetComboBoxLinkText(dlg, CTLSEL_PSALESFLT_GOODS, Filt.SubstName);
			else
				SetComboBoxListText(dlg, CTLSEL_PSALESFLT_GOODS);
		}
		dlg->addGroup(GRP_LOC, new LocationCtrlGroup(CTLSEL_PSALESFLT_LOC, 0, 0, cmLocList, 0, 0, 0));
		LocationCtrlGroup::Rec loc_rec(&p_filt->LocList);
		dlg->setGroupData(GRP_LOC, &loc_rec);
		dlg->setCtrlData(CTL_PSALESFLT_CYCLE, &p_filt->Cycle);
		dlg->setCtrlData(CTL_PSALESFLT_WATERSHED, &p_filt->Watershed);
		dlg->AddClusterAssoc(CTL_PSALESFLT_FLAGS, 0, PredictSalesFilt::fShowNoData);
		dlg->SetClusterData(CTL_PSALESFLT_FLAGS, p_filt->Flags);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			PredictSalesFilt filt;
			filt =*p_filt;
			if(!GetPeriodInput(dlg, CTL_PSALESFLT_PERIOD, &filt.Period))
				PPErrorByDialog(dlg, CTL_PSALESFLT_PERIOD);
			else if(dlg->getGroupData(GRP_GOODS, &rec) > 0) {
				if(!p_filt->GoodsIdList.IsExists()) {
					filt.GoodsGrpID = rec.GrpID;
					filt.GoodsID    = rec.GoodsID;
				}
				else if(p_filt->GoodsIdList.GetSingle() && rec.GoodsID) {
					filt.GoodsGrpID = rec.GrpID;
					filt.GoodsID    = rec.GoodsID;
					filt.GoodsIdList.Set(0);
					filt.GoodsIdList.Add(rec.GoodsID);
				}
				if(filt.GoodsID == 0 && !p_filt->GoodsIdList.IsExists())
					PPErrorByDialog(dlg, CTLSEL_PSALESFLT_GOODS, PPERR_GOODSNEEDED);
				else {
					dlg->getGroupData(GRP_LOC, &loc_rec);
					filt.LocList = loc_rec.LocList;
					dlg->getCtrlData(CTL_PSALESFLT_CYCLE, &filt.Cycle);
					dlg->getCtrlData(CTL_PSALESFLT_WATERSHED, &filt.Watershed);
					dlg->GetClusterData(CTL_PSALESFLT_FLAGS, &filt.Flags);
					ASSIGN_PTR(p_filt, filt);
					ok = valid_data = 1;
				}
			}
			else
				PPErrorByDialog(dlg, CTL_PSALESFLT_GOODS);
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPViewPredictSales::ConvertHdr(const void * pHdr, BrwHdr * pOut) const
{
	if(pOut) {
		pOut->GoodsID = Filt.GoodsID;
		pOut->LocID = Filt.LocList.GetSingle();
		pOut->Dt = pHdr ? *static_cast<const LDATE *>(pHdr) : ZERODATE;
	}
	return BIN(pHdr);
}

int PPViewPredictSales::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr hdr;
		ConvertHdr(pHdr, &hdr);
		switch(ppvCmd) {
			case PPVCMD_VIEWOPGRPNG:
				ok = -1;
				if(hdr.Dt) {
					uint   p = 0;
					OpGroupingFilt op_grpng_flt;
					if(Filt.Cycle && CycleList.searchDate(hdr.Dt, &p))
						CycleList.getPeriod(p, &op_grpng_flt.Period);
					else
						op_grpng_flt.Period.SetDate(hdr.Dt);
					op_grpng_flt.GoodsID = hdr.GoodsID;
					op_grpng_flt.LocList = Filt.LocList;
					op_grpng_flt.Flags |= OpGroupingFilt::fCalcRest;
					ViewOpGrouping(&op_grpng_flt);
				}
				break;
			case PPVCMD_GRAPH:
				ok = -1;
				ViewGraph(pBrw);
				break;
			case PPVCMD_REBUILD:
				ok = -1;
				if(hdr.GoodsID && RecalcGoods(&hdr) > 0)
					ok = 1;
				break;
			case PPVCMD_TEST:
				ok = -1;
				if(hdr.Dt && TestPrediction(&hdr) > 0)
					ok = 1;
				break;
		}
	}
	return ok;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	if(pData && pStyle && paintAction == BrowserWindow::paintNormal) {
		const long flags = *reinterpret_cast<const long *>(PTR8C(pData)+sizeof(LDATE));
		if(flags & 0x0001) {
			pStyle->Color = GetGrayColorRef(0.9f);
			pStyle->Flags = 0;
			ok = 1;
		}
	}
	return ok;
}

void PPViewPredictSales::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, SetCellStyleFunc(CellStyleFunc, 0));
}

DBQuery * PPViewPredictSales::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * p_q = 0;
	uint   brw_id = BROWSER_PSALESGOODS;
	DBQ * dbq = 0;
	TempPredictSalesTbl * tt = 0;
	if(P_TempTbl) {
		THROW(CheckTblPtr(tt = new TempPredictSalesTbl(P_TempTbl->GetName())));
		p_q = & select(
			tt->Dt,           // #0
			tt->Flags,        // #1
			tt->Qtty,         // #2
			tt->QttyPredict,  // #3
			tt->QttyAbsErr,   // #4
			tt->QttyPctErr,   // #5
			tt->Amt,          // #6
			tt->AmtPredict,   // #7
			tt->AmtAbsErr,    // #8
			tt->AmtPctErr,    // #9
			0L).from(tt, 0L);
	}
	THROW(CheckQueryPtr(p_q));
	if(pSubTitle) {
		if(Filt.GoodsID || Filt.GoodsIdList.IsExists()) {
			SString goods_name, subtitle;
			GetExtLocationName(Filt.LocList, 3, subtitle);
			if(Filt.GoodsID)
				GetGoodsName(Filt.GoodsID, goods_name);
			else if(Filt.GoodsIdList.GetSingle())
				GetGoodsName(Filt.GoodsIdList.GetSingle(), goods_name);
			else if(Filt.GoodsIdList.IsExists())
				goods_name = Filt.SubstName;
			*pSubTitle = subtitle.CatDiv('-', 1).Cat(goods_name);
		}
	}
	CATCH
		if(p_q)
			ZDELETE(p_q);
		else
			delete tt;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_q;
}
//
//
//
int FillPredictSales()
{
	int    ok = -1;
	PrcssrPrediction prcssr;
	PrcssrPrediction::Param param;
	prcssr.InitParam(&param);
	if(prcssr.EditParam(&param) > 0) {
		PPWaitStart();
		ok = (prcssr.Init(&param) && prcssr.Run()) ? 1 : PPErrorZ();
		PPWaitStop();
	}
	return ok;
}

int TestPredictSales()
{
	int    ok = -1;
	PrcssrPrediction prcssr;
	PrcssrPrediction::Param param;
	prcssr.InitParam(&param);
	param.Process = param.prcsTest;
	if(prcssr.EditParam(&param) > 0) {
		PPWaitStart();
		ok = (prcssr.Init(&param) && prcssr.Run()) ? 1 : PPErrorZ();
		PPWaitStop();
	}
	return ok;
}

int ViewPredictSales(PredictSalesFilt * pFilt) { return PPView::Execute(PPVIEW_PREDICTSALES, pFilt, PPView::exefModeless, 0); }
//
// Implementation of PPALDD_PredictSales
//
PPALDD_CONSTRUCTOR(PredictSales)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PredictSales) { Destroy(); }

int PPALDD_PredictSales::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(PredictSales, rsrv);
	H.FltGoodsID = p_filt->GoodsID;
	H.FltLocID = p_filt->LocList.GetSingle();
	H.FltBeg = p_filt->Period.low;
	H.FltEnd = p_filt->Period.upp;
	H.FltCycle = p_filt->Cycle;
	H.Watershed = p_filt->Watershed;
	H.Flags = p_filt->Flags;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PredictSales::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(PredictSales);
}

int PPALDD_PredictSales::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(PredictSales);
	p_v->FormatCycle(item.Dt, I.CycleText, sizeof(I.CycleText));
	I.Dt       = item.Dt;
	I.Qtty        = item.Qtty;
	I.QttyPredict = item.QttyPredict;
	I.QttyAbsErr  = item.QttyAbsErr;
	I.QttyPctErr  = item.QttyPctErr;
	I.Amt        = item.Amt;
	I.AmtPredict = item.AmtPredict;
	I.AmtAbsErr  = item.AmtAbsErr;
	I.AmtPctErr  = item.AmtPctErr;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_PredictSales::Destroy() { DESTROY_PPVIEW_ALDD(PredictSales); }
