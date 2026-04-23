// GCT.CPP
// Copyright (c) A.Sobolev, A.Starodub 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2009, 2010, 2011, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2024, 2025, 2026
// @codepage UTF-8
// Построение перекрестной отчетности по товарным операциям
//
#include <pp.h>
#pragma hdrstop
//
// Отчет о товарообороте
//
IMPLEMENT_PPFILT_FACTORY(GoodsTrnovr);

static int EditGoodsTrnovrFilt(GoodsTrnovrFilt * pFilt)
{
	class GCTFiltDialog : public WLDialog {
		DECL_DIALOG_DATA(GCTFilt);
	public:
		enum {
			ctlgroupGoodsFilt = 1,
			ctlgroupLoc       = 2
		};
		GCTFiltDialog(uint dlgID, int forceGoods) : WLDialog(dlgID, CTL_GTO_LABEL), ForceGoodsSelection(forceGoods)
		{
			addGroup(ctlgroupGoodsFilt, new GoodsFiltCtrlGroup(CTLSEL_GTO_GOODS, CTLSEL_GTO_GGRP, cmGoodsFilt));
			addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_GTO_LOC, 0, 0, cmLocList, 0, 0, 0));
			SetupCalPeriod(CTLCAL_GTO_PERIOD, CTL_GTO_PERIOD);
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			PPIDArray types;
			SetPeriodInput(this, CTL_GTO_PERIOD, Data.Period);
			{
				LocationCtrlGroup::Rec l_rec(&Data.LocList);
				setGroupData(ctlgroupLoc, &l_rec);
			}
			SetupArCombo(this, CTLSEL_GTO_SUPPL, Data.SupplID, OLW_LOADDEFONOPEN, GetSupplAccSheet(), sacfDisableIfZeroSheet);
			types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN,
				PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, PPOPT_GENERIC, 0L);
			SetupOprKindCombo(this, CTLSEL_GTO_OPR, Data.OpID, 0, &types, 0);
			GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, Data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
			if(ForceGoodsSelection)
				gf_rec.Flags |= GoodsCtrlGroup::disableEmptyGoods;
			setGroupData(ctlgroupGoodsFilt, &gf_rec);
			setWL(BIN(Data.Flags & OPG_LABELONLY));
			AddClusterAssocDef(CTL_GTO_ORDER, 0, GCTFilt::ordByDate);
			AddClusterAssoc(CTL_GTO_ORDER, 1, GCTFilt::ordByGoods);
			AddClusterAssoc(CTL_GTO_ORDER, 2, GCTFilt::ordByArticle);
			SetClusterData(CTL_GTO_ORDER, Data.Order);
			if(getCtrlView(CTL_GTO_GENGGRP))
				setCtrlUInt16(CTL_GTO_GENGGRP, BIN(Data.Flags & OPG_GRPBYGENGOODS));
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			ushort v;
			GoodsFiltCtrlGroup::Rec gf_rec;
			// AHTOXA {
			LocationCtrlGroup::Rec l_rec;
			getGroupData(ctlgroupLoc, &l_rec);
			Data.LocList = l_rec.LocList;
			// } AHTOXA
			THROW(GetPeriodInput(this, sel = CTL_GTO_PERIOD, &Data.Period));
			THROW(AdjustPeriodToRights(Data.Period, true));
			getCtrlData(CTLSEL_GTO_SUPPL, &Data.SupplID);
			getCtrlData(CTLSEL_GTO_OPR,   &Data.OpID);
			THROW(getGroupData(sel = ctlgroupGoodsFilt, &gf_rec));
			Data.GoodsGrpID = gf_rec.GoodsGrpID;
			Data.GoodsID    = gf_rec.GoodsID;
			SETFLAG(Data.Flags, OPG_LABELONLY, getWL());
			GetClusterData(CTL_GTO_ORDER, &Data.Order);
			if(getCtrlView(CTL_GTO_GENGGRP)) {
				getCtrlData(CTL_GTO_GENGGRP, &v);
				SETFLAG(Data.Flags, OPG_GRPBYGENGOODS, v);
			}
			else
				Data.Flags &= ~OPG_GRPBYGENGOODS;
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		int    ForceGoodsSelection;
	};
	DIALOG_PROC_BODY_P2(GCTFiltDialog, DLG_GTO, (int)(pFilt->Flags & OPG_FORCEGOODS), pFilt);
}

PPViewGoodsTrnovr::PPViewGoodsTrnovr() : PPView(0, &Filt, PPVIEW_GOODSTRNOVR, implBrowseArray, 0), P_DsList(0)
{
}

PPViewGoodsTrnovr::~PPViewGoodsTrnovr()
{
	delete P_DsList;
}

/*virtual*/int PPViewGoodsTrnovr::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	if(Filt.IsA(pBaseFilt)) {
		GoodsTrnovrFilt * p_filt = static_cast<GoodsTrnovrFilt *>(pBaseFilt);
		ok = EditGoodsTrnovrFilt(p_filt);
	}
	else
		ok = 0;
	return ok;
}

/*virtual*/int PPViewGoodsTrnovr::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	LDATE  dt = ZERODATE;
	GoodsTrnovrViewItem entry;
	GoodsTrnovrViewItem total;
	SString wait_msg;
	IterCounter cntr;
	GoodsGrpngEntry * p_entry;
	PPOprKind op_rec;
	PPOprKind link_op_rec;
	GCTFilt filt;
	AdjGdsGrpng agg;
	int    zero;
	GoodsGrpngArray gga;

	THROW(Helper_InitBaseFilt(pBaseFilt));
	Filt.Period.Actualize(ZERODATE);
	if(P_DsList)
		P_DsList->clear();
	else {
		THROW_MEM(P_DsList = new SArray(sizeof(GoodsTrnovrViewItem)));
	}
	filt = Filt; // AHTOXA
	cntr.Init(diffdate(&Filt.Period.upp, &Filt.Period.low, 0) + 1);
	dt = filt.Period.low;
	if(!checkdate(dt)) {
		THROW(BillObj->P_Tbl->GetFirstDate(0, &dt) > 0);
	}
	SETIFZ(filt.Period.upp, getcurdate_());
	MEMSZERO(total);
	if(Filt.Flags & OPG_DONTSHOWPRGRSBAR)
		PPLoadText(PPTXT_CALCOPGRPNG, wait_msg);
	while(dt <= Filt.Period.upp) {
		zero = 1;
		MEMSZERO(entry);
		filt.Period.SetDate(dt);
		filt.Flags |= OPG_PROCESSRECKONING;
		entry.Dt = dt;
		datefmt(&dt, DATF_DMY, entry.Title);
		gga.Reset();
		THROW(agg.BeginGoodsGroupingProcess(filt));
		THROW(gga.ProcessGoodsGrouping(filt, &agg));
		for(uint i = 0; gga.enumItems(&i, (void **)&p_entry);) {
			if(!oneof3(p_entry->OpID, -1, 10000, 0)) {
				if(IsIntrExpndOp(p_entry->OpID))
					entry.XpndIntr += p_entry->Cost;
				else if(IsIntrOp(p_entry->OpID) == INTRRCPT)
					entry.RcptIntr += p_entry->Cost;
				else {
					THROW(GetOpData(p_entry->OpID, &op_rec));
					if(p_entry->OpTypeID == PPOPT_GOODSRECEIPT)
						entry.RcptSuppl += p_entry->Cost;
					else if(p_entry->Link == 0) {
						if(CheckOpFlags(p_entry->OpID, OPKF_PROFITABLE)) {
							if(op_rec.AccSheetID)
								entry.XpndClient += p_entry->Price;
							else
								entry.XpndRetail += p_entry->Price;
						}
					}
					else {
						THROW(GetOpData(p_entry->Link, &link_op_rec));
						if(p_entry->OpTypeID == PPOPT_GOODSRETURN) {
							if(link_op_rec.OpTypeID == PPOPT_GOODSRECEIPT)
								entry.RetSuppl += p_entry->Cost;
							else if(link_op_rec.Flags & OPKF_PROFITABLE) {
								if(link_op_rec.AccSheetID)
									entry.RetClient += p_entry->Price;
								else
									entry.RetRetail += p_entry->Price;
							}
						}
						else if(p_entry->OpTypeID == PPOPT_PAYMENT) {
							if(CheckOpFlags(p_entry->Link, OPKF_PROFITABLE))
								entry.PayClient += p_entry->Price;
						}
					}
				}
				entry.Income += p_entry->Income();
			}
		}
		if(entry.RcptSuppl) {
			zero = 0;
			total.RcptSuppl += entry.RcptSuppl;
		}
		// @todo 01/05/2005 Внутренняя передача на списке складов
		if(filt.LocList.GetCount() && entry.RcptIntr) {
			zero = 0;
			total.RcptIntr += entry.RcptIntr;
		}
		if(entry.RetRetail) {
			zero = 0;
			total.RetRetail += entry.RetRetail;
		}
		if(entry.RetClient) {
			zero = 0;
			total.RetClient += entry.RetClient;
		}
		if(entry.RetSuppl) {
			zero = 0;
			total.RetSuppl += entry.RetSuppl;
		}
		if(entry.XpndRetail) {
			zero = 0;
			total.XpndRetail += entry.XpndRetail;
		}
		if(entry.XpndClient) {
			zero = 0;
			total.XpndClient += entry.XpndClient;
		}
		// @todo 01/05/2005 Внутренняя передача на списке складов
		if(filt.LocList.GetCount() && entry.XpndIntr) {
			zero = 0;
			total.XpndIntr += entry.XpndIntr;
		}
		if(entry.PayClient) {
			zero = 0;
			total.PayClient += entry.PayClient;
		}
		if(entry.Income) {
			zero = 0;
			total.Income += entry.Income;
		}
		if(!zero || !(filt.Flags & OPG_IGNOREZERO)) {
			THROW_SL(P_DsList->insert(&entry));
		}
		plusdate(&dt, 1, 0);
		agg.EndGoodsGroupingProcess();
		PPWaitPercent(cntr.Increment(), wait_msg);
	}
	PPGetWord(PPWORD_TOTAL, 0, total.Title, sizeof(total.Title));
	THROW_SL(P_DsList->insert(&total));
	agg.EndGoodsGroupingProcess();
	//THROW(P_Items = MakeGoodsTurnover());
	//Cntr.Init(P_DsList->getCount() ? (P_DsList->getCount()-1) : 0);
	CATCH
		ZDELETE(P_DsList);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewGoodsTrnovr::InitIteration()
{
	Counter.Init(SVectorBase::GetCount(P_DsList));
	return 1;
}

int FASTCALL PPViewGoodsTrnovr::NextIteration(GoodsTrnovrViewItem * pItem)
{
	int    ok = -1;
	while(ok < 0 && P_DsList && P_DsList->testPointer()) {
		const  GoodsTrnovrViewItem * p_iter_item = static_cast<const GoodsTrnovrViewItem *>(P_DsList->at(P_DsList->getPointer()));
		if(p_iter_item) {
			ASSIGN_PTR(pItem, *p_iter_item);
			P_DsList->incPointer();
			ok = 1;
		}
	}
	return ok;
}

/*virtual*/SArray * PPViewGoodsTrnovr::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_result = new SArray(*P_DsList);
	const  uint res_id = Filt.LocList.GetCount() ? BROWSER_GOODSTURNOVER2_BYLOC : BROWSER_GOODSTURNOVER2;
	ASSIGN_PTR(pBrwId, res_id);
	return p_result;
}

/*virtual*/int PPViewGoodsTrnovr::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -2;
	ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		//
	}
	return ok;
}

/*virtual*/int PPViewGoodsTrnovr::Print(const void *)
{
	int    ok = 1;
	uint   rpt_id = Filt.LocList.GetCount() ? REPORT_LGOODSTRNOVR : REPORT_GOODSTRNOVR;
	PPReportEnv env(0, 0);
	PPAlddPrint(rpt_id, PView(this), &env);
	return ok;
}

/*virtual*/int PPViewGoodsTrnovr::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	const GoodsTrnovrViewItem * p_item = static_cast<const GoodsTrnovrViewItem *>(pHdr);
	if(p_item && checkdate(p_item->Dt)) {
		OpGroupingFilt op_grpng_flt;
		op_grpng_flt.Period.SetDate(p_item->Dt);
		op_grpng_flt.LocList = Filt.LocList;
		op_grpng_flt.SupplID = Filt.SupplID;
		op_grpng_flt.GoodsGrpID = Filt.GoodsGrpID;
		op_grpng_flt.GoodsID    = Filt.GoodsID;
		ViewOpGrouping(&op_grpng_flt);
	}
	return ok;
}

/*virtual*/void PPViewGoodsTrnovr::ViewTotal()
{
}

int PPViewGoodsTrnovr::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	/*
browser GOODSTURNOVER2_BYLOC north(100), 3, 1, "@{view_goodstrnovr}", OWNER|GRID, 0 // @v12.5.11 
{
	"@date",                          1, zstring(48), 0, 8
	group "@goodsreceipt" {
		"@supplier",                  2, double, NMBF_NOZERO, 9.2
		"@oprcategory_intrrcpt_2",    3, double, NMBF_NOZERO, 9.2
	}
	group "@return" {
		"@selling_retail",            4, double, NMBF_NOZERO, 9.2
		"@client",                    5, double, NMBF_NOZERO, 9.2
		"@supplier",                  6, double, NMBF_NOZERO, 9.2
	}
	group "@expend" {
		"@selling_retail",            7, double, NMBF_NOZERO, 9.2
		"@client",                    8, double, NMBF_NOZERO, 9.2
		"@oprcategory_intrexpnd_2",   9, double, NMBF_NOZERO, 9.2
	}
	"@payment",                      10, double, NMBF_NOZERO, 9.2
	"@income",                       11, double, NMBF_NOZERO, 9.2
	toolbar TB_FIRST_TOOLBAR {
		kbEnter, "@{opgrouping}\tEnter", TBBM_OPGRPNG
		kbF7,    "@{print}\tF7",         PPDV_PRINTER01
	}
}
	*/ 
	int    ok = 1;
	assert(pBlk->P_SrcData && pBlk->P_DestData); // Функция вызывается только из одной локации и эти members != 0 равно как и pBlk != 0
	const  GoodsTrnovrViewItem * p_item = static_cast<const GoodsTrnovrViewItem *>(pBlk->P_SrcData);
	int    r = 0;
	switch(pBlk->ColumnN) {
		case  0: break; // @?
		case  1: pBlk->Set(p_item->Title); break; // date
		case  2: pBlk->Set(p_item->RcptSuppl); break; // goodsreceipt - supplier
		case  3: pBlk->Set(p_item->RcptIntr); break; // goodsreceipt - oprcategory_intrrcpt_2
		case  4: pBlk->Set(p_item->RetRetail); break; // return - selling_retail
		case  5: pBlk->Set(p_item->RetClient); break; // return - client
		case  6: pBlk->Set(p_item->RetSuppl); break; // return - supplier
		case  7: pBlk->Set(p_item->XpndRetail); break; // expend - selling_retail
		case  8: pBlk->Set(p_item->XpndClient); break; // expend - client
		case  9: pBlk->Set(p_item->XpndIntr); break; // expend - oprcategory_intrexpnd_2
		case 10: pBlk->Set(p_item->PayClient); break; // payment
		case 11: pBlk->Set(p_item->Income); break; // income
	}
	return ok;
}

void PPViewGoodsTrnovr::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc([](SBrowserDataProcBlock * pBlk) -> int
			{
				return (pBlk && pBlk->ExtraPtr) ? static_cast<PPViewGoodsTrnovr *>(pBlk->ExtraPtr)->_GetDataForBrowser(pBlk) : 0;				
			}, this);
	}
}
//
// @ModuleDef(PPViewGoodsTrnovr)
//
#if 0 // @v12.5.12 {
class PPViewGoodsTrnovr { // @todo @20260117 Перевести на PPView
public:
	PPViewGoodsTrnovr();
	~PPViewGoodsTrnovr();
	const  GoodsTrnovrFilt * GetFilt() const;
	int    EditFilt(GoodsTrnovrFilt *);
	int    Init(const GoodsTrnovrFilt *);
	int    InitIteration();
	int    FASTCALL NextIteration(GoodsTrnovrViewItem *);
	int    GetIterationCount(long *, long *);
	int    ViewGrouping(LDATE);
	int    Browse(int modeless);
	int    Print();
private:
	SArray * CreateBrowserQuery();

	SArray * P_Items;
	GoodsTrnovrFilt Filt;
	IterCounter Cntr;
};
//
// GoodsTrnovrBrowser
//
class GoodsTrnovrBrowser : public BrowserWindow {
public:
	GoodsTrnovrBrowser(uint rezID, SArray * a, PPViewGoodsTrnovr *pView, GCTFilt * f, int dataOwner) :
		BrowserWindow(rezID, a, 0), P_View(pView), IsDataOwner(dataOwner)
	{
		const GoodsTrnovrFilt * p_filt = P_View ? static_cast<PPViewGoodsTrnovr *>(P_View)->GetFilt() : 0;
		if(p_filt)
			PPObjGoodsGroup::SetOwner(p_filt->GoodsGrpID, 0, reinterpret_cast<long>(this)); // @x64crit
	}
	~GoodsTrnovrBrowser()
	{
		const GoodsTrnovrFilt * p_filt = P_View ? static_cast<PPViewGoodsTrnovr *>(P_View)->GetFilt() : 0;
		if(p_filt)
			PPObjGoodsGroup::RemoveTempAlt(p_filt->GoodsGrpID, reinterpret_cast<long>(this)); // @x64crit
		if(IsDataOwner)
			delete P_View;
	}
	PPViewGoodsTrnovr * P_View;
private:
	DECL_HANDLE_EVENT;
	LDATE  GetDate();

	int    IsDataOwner;
};

LDATE GoodsTrnovrBrowser::GetDate()
{
	struct _H {
		char str_dt[48]; // @v12.5.11 [12]-->[48]
	};
	const _H * h_dt = static_cast<const _H *>(getCurItem());
	LDATE  dt = ZERODATE;
	if(h_dt)
		strtodate(h_dt->str_dt, DATF_DMY, &dt);
	return dt;
}

IMPL_HANDLE_EVENT(GoodsTrnovrBrowser)
{
	LDATE  dt;
   	BrowserWindow::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmPrint:
				P_View->Print();
				break;
			case cmaEdit:
				dt = GetDate();
				if(dt)
					P_View->ViewGrouping(dt);
				break;
			default:
				return;
		}
	}
	else if(TVKEYDOWN) {
		switch(TVKEY) {
			case kbF7:
				P_View->Print();
				break;
			default:
				return;
		}
	}
	else
		return;
	clearEvent(event);
}

PPViewGoodsTrnovr::PPViewGoodsTrnovr() : P_Items(0)
{
}

PPViewGoodsTrnovr::~PPViewGoodsTrnovr()
{
	delete P_Items;
}

const GoodsTrnovrFilt * PPViewGoodsTrnovr::GetFilt() const { return &Filt; }

int PPViewGoodsTrnovr::EditFilt(GoodsTrnovrFilt * pFilt)
{
	return EditGoodsTrnovrFilt(pFilt);
}

int PPViewGoodsTrnovr::Init(const GoodsTrnovrFilt * pFilt)
{
	int    ok = 1;
	LDATE  dt = ZERODATE;
	GoodsTrnovrViewItem entry;
	GoodsTrnovrViewItem total;
	SString wait_msg;
	IterCounter cntr;
	GoodsGrpngEntry * p_entry;
	PPOprKind op_rec;
	PPOprKind link_op_rec;
	GCTFilt filt;
	AdjGdsGrpng agg;
	int    zero;
	GoodsGrpngArray gga;
	if(!RVALUEPTR(Filt, pFilt))
		Filt.Init(1, 0);
	if(P_Items)
		P_Items->freeAll();
	else {
		THROW_MEM(P_Items = new SArray(sizeof(GoodsTrnovrViewItem)));
	}
	filt = Filt; // AHTOXA
	cntr.Init(diffdate(&Filt.Period.upp, &Filt.Period.low, 0) + 1);
	dt = filt.Period.low;
	if(!dt)
		THROW(BillObj->P_Tbl->GetFirstDate(0, &dt) > 0);
	SETIFZ(filt.Period.upp, getcurdate_());
	MEMSZERO(total);
	if(Filt.Flags & OPG_DONTSHOWPRGRSBAR)
		PPLoadText(PPTXT_CALCOPGRPNG, wait_msg);
	while(dt <= Filt.Period.upp) {
		zero = 1;
		MEMSZERO(entry);
		filt.Period.SetDate(dt);
		filt.Flags |= OPG_PROCESSRECKONING;
		entry.Dt = dt;
		datefmt(&dt, DATF_DMY, entry.Title);
		gga.Reset();
		THROW(agg.BeginGoodsGroupingProcess(filt));
		THROW(gga.ProcessGoodsGrouping(filt, &agg));
		for(uint i = 0; gga.enumItems(&i, (void **)&p_entry);) {
			if(!oneof3(p_entry->OpID, -1, 10000, 0)) {
				if(IsIntrExpndOp(p_entry->OpID))
					entry.XpndIntr += p_entry->Cost;
				else if(IsIntrOp(p_entry->OpID) == INTRRCPT)
					entry.RcptIntr += p_entry->Cost;
				else {
					THROW(GetOpData(p_entry->OpID, &op_rec));
					if(p_entry->OpTypeID == PPOPT_GOODSRECEIPT)
						entry.RcptSuppl += p_entry->Cost;
					else if(p_entry->Link == 0) {
						if(CheckOpFlags(p_entry->OpID, OPKF_PROFITABLE)) {
							if(op_rec.AccSheetID)
								entry.XpndClient += p_entry->Price;
							else
								entry.XpndRetail += p_entry->Price;
						}
					}
					else {
						THROW(GetOpData(p_entry->Link, &link_op_rec));
						if(p_entry->OpTypeID == PPOPT_GOODSRETURN) {
							if(link_op_rec.OpTypeID == PPOPT_GOODSRECEIPT)
								entry.RetSuppl += p_entry->Cost;
							else if(link_op_rec.Flags & OPKF_PROFITABLE) {
								if(link_op_rec.AccSheetID)
									entry.RetClient += p_entry->Price;
								else
									entry.RetRetail += p_entry->Price;
							}
						}
						else if(p_entry->OpTypeID == PPOPT_PAYMENT) {
							if(CheckOpFlags(p_entry->Link, OPKF_PROFITABLE))
								entry.PayClient += p_entry->Price;
						}
					}
				}
				entry.Income += p_entry->Income();
			}
		}
		if(entry.RcptSuppl) {
			zero = 0;
			total.RcptSuppl += entry.RcptSuppl;
		}
		// @todo 01/05/2005 Внутренняя передача на списке складов
		if(filt.LocList.GetCount() && entry.RcptIntr) {
			zero = 0;
			total.RcptIntr += entry.RcptIntr;
		}
		if(entry.RetRetail) {
			zero = 0;
			total.RetRetail += entry.RetRetail;
		}
		if(entry.RetClient) {
			zero = 0;
			total.RetClient += entry.RetClient;
		}
		if(entry.RetSuppl) {
			zero = 0;
			total.RetSuppl += entry.RetSuppl;
		}
		if(entry.XpndRetail) {
			zero = 0;
			total.XpndRetail += entry.XpndRetail;
		}
		if(entry.XpndClient) {
			zero = 0;
			total.XpndClient += entry.XpndClient;
		}
		// @todo 01/05/2005 Внутренняя передача на списке складов
		if(filt.LocList.GetCount() && entry.XpndIntr) {
			zero = 0;
			total.XpndIntr += entry.XpndIntr;
		}
		if(entry.PayClient) {
			zero = 0;
			total.PayClient += entry.PayClient;
		}
		if(entry.Income) {
			zero = 0;
			total.Income += entry.Income;
		}
		if(!zero || !(filt.Flags & OPG_IGNOREZERO))
			THROW_SL(P_Items->insert(&entry));
		plusdate(&dt, 1, 0);
		agg.EndGoodsGroupingProcess();
		PPWaitPercent(cntr.Increment(), wait_msg);
	}
	PPGetWord(PPWORD_TOTAL, 0, total.Title, sizeof(total.Title));
	THROW_SL(P_Items->insert(&total));
	agg.EndGoodsGroupingProcess();
	//THROW(P_Items = MakeGoodsTurnover());
	Cntr.Init(P_Items->getCount() ? (P_Items->getCount()-1) : 0);
	CATCH
		ZDELETE(P_Items);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewGoodsTrnovr::InitIteration()
{
	int    ok = 1;
	const  uint _c = SVectorBase::GetCount(P_Items);
	Cntr.Init(_c ? (_c-1) : 0);
	return ok;
}

int FASTCALL PPViewGoodsTrnovr::NextIteration(GoodsTrnovrViewItem * pItem)
{
	int    ok = -1;
	if(P_Items->getCount() && Cntr < (ulong)(P_Items->getCountI()-1)) {
		const GoodsTrnovrViewItem * goods_trnovr_item = static_cast<const GoodsTrnovrViewItem *>(P_Items->at(Cntr));
		strtodate(goods_trnovr_item->Title, DATF_DMY, &pItem->Dt);
		pItem->RcptSuppl  = goods_trnovr_item->RcptSuppl;
		pItem->RcptIntr   = goods_trnovr_item->RcptIntr;
		pItem->RetRetail  = goods_trnovr_item->RetRetail;
		pItem->RetClient  = goods_trnovr_item->RetClient;
		pItem->RetSuppl   = goods_trnovr_item->RetSuppl;
		pItem->XpndRetail = goods_trnovr_item->XpndRetail;
		pItem->XpndClient = goods_trnovr_item->XpndClient;
		pItem->XpndIntr   = goods_trnovr_item->XpndIntr;
		pItem->PayClient  = goods_trnovr_item->PayClient;
		pItem->Income      = goods_trnovr_item->Income;
		Cntr.Increment();
		ok = 1;
	}
	return ok;
}

int PPViewGoodsTrnovr::GetIterationCount(long * pNumIteration, long * pIterCount)
{
	ASSIGN_PTR(pNumIteration, Cntr.GetTotal());
	ASSIGN_PTR(pIterCount, Cntr);
	return 1;
}

int PPViewGoodsTrnovr::ViewGrouping(LDATE dt)
{
	int    ok = -1;
	OpGroupingFilt op_grpng_flt;
	op_grpng_flt.Period.SetDate(dt);
	op_grpng_flt.LocList = Filt.LocList;
	op_grpng_flt.SupplID = Filt.SupplID;
	op_grpng_flt.GoodsGrpID = Filt.GoodsGrpID;
	op_grpng_flt.GoodsID    = Filt.GoodsID;
	ViewOpGrouping(&op_grpng_flt);
	return ok;
}

SArray * PPViewGoodsTrnovr::CreateBrowserQuery()
{
	SArray * ary = new SArray(sizeof(GoodsTrnovrViewItem));
	CALLPTRMEMB(ary, copy(*P_Items));
	return ary;
}

int PPViewGoodsTrnovr::Browse(int modeless)
{
	int    ok = 1;
	uint   res_id = 0;
	GoodsTrnovrBrowser * p_brw = 0;
	SArray * ary = 0;
	PPWaitStart();
	THROW(ary = CreateBrowserQuery());
	res_id = Filt.LocList.GetCount() ? BROWSER_GOODSTURNOVER_BYLOC : BROWSER_GOODSTURNOVER;
	THROW_MEM(p_brw = new GoodsTrnovrBrowser(res_id, ary, this, &Filt, modeless));
	PPWaitStop();
	PPOpenBrowser(p_brw, modeless);
	CATCHZOK
	if(!modeless || !ok)
		delete p_brw;
	return ok;
}

int PPViewGoodsTrnovr::Print()
{
	int    ok = -1;
	PPAlddPrint(Filt.LocList.GetCount() ? REPORT_LGOODSTRNOVR : REPORT_GOODSTRNOVR, PView(this), 0);
	return ok;
}

int ViewGoodsTurnover(long)
{
	int    ok = 1;
	int    view_in_use = 0;
	const  bool modeless = GetModelessStatus();
	GoodsTrnovrFilt flt;
	PPViewGoodsTrnovr * p_v = new PPViewGoodsTrnovr;
	BrowserWindow * p_prev_win = modeless ? PPFindLastBrowser() : 0;
	if(p_prev_win)
		flt = *static_cast<const GoodsTrnovrBrowser *>(p_prev_win)->P_View->GetFilt();
	else
		flt.Period.SetDate(getcurdate_());
	while(p_v->EditFilt(&flt) > 0) {
		PPWaitStart();
		flt.Flags |= static_cast<uint>(OPG_IGNOREZERO);
		flt.Flags |= OPG_DONTSHOWPRGRSBAR;
		THROW(p_v->Init(&flt));
		PPCloseBrowser(p_prev_win);
		THROW(p_v->Browse(modeless));
		if(modeless) {
			view_in_use = 1;
			break;
		}
	}
	CATCHZOKPPERR
	if(!modeless || !ok || !view_in_use)
		delete p_v;
	return ok;
}
#endif // } @v12.5.12
//
// Implementation of PPALDD_GoodsTurnovr
//
PPALDD_CONSTRUCTOR(GoodsTurnovr)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsTurnovr) { Destroy(); }

int PPALDD_GoodsTurnovr::InitData(PPFilt & rFilt, long rsrv)
{
	SString loc_name;
	SString name_buf;
	INIT_PPVIEW_ALDD_DATA_U(GoodsTrnovr, rsrv);
	GetLocationName(p_filt->LocList.GetSingle(), loc_name);
	loc_name.CopyTo(H.FltLocName, sizeof(H.FltLocName));
	GetSupplText(p_filt->SupplID, name_buf);
	name_buf.CopyTo(H.FltSupplName, sizeof(H.FltSupplName));
	H.FltBeg = p_filt->Period.low;
	H.FltEnd = p_filt->Period.upp;
	H.FltGoodsGrpID = p_filt->GoodsGrpID;
	return DlRtm::InitData(rFilt, rsrv);
}

void PPALDD_GoodsTurnovr::Destroy() { DESTROY_PPVIEW_ALDD(GoodsTrnovr); }
int  PPALDD_GoodsTurnovr::InitIteration(PPIterID iterId, int sortId, long/*rsrv*/) { INIT_PPVIEW_ALDD_ITER(GoodsTrnovr); }

int PPALDD_GoodsTurnovr::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(GoodsTrnovr);
	I.Dt  = item.Dt;
	I.RcptSuppl   = item.RcptSuppl;
	I.RcptIntr    = item.RcptIntr;
	I.RetRetail   = item.RetRetail;
	I.RetClient   = item.RetClient;
	I.RetSuppl    = item.RetSuppl;
	I.ExpndRetail = item.XpndRetail;
	I.ExpndClient = item.XpndClient;
	I.ExpndIntr   = item.XpndIntr;
	I.PayClient   = item.PayClient;
	I.Income      = item.Income;
	FINISH_PPVIEW_ALDD_ITER();
}

