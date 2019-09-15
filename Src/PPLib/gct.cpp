// GCT.CPP
// Copyright (c) A.Sobolev, A.Starodub 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2009, 2010, 2011, 2015, 2016, 2017, 2018, 2019
// @codepage UTF-8
// Построение перекрестной отчетности по товарным операциям
//
#include <pp.h>
#pragma hdrstop
//
// Отчет о товарообороте
//
//
// GoodsTrnovrBrowser
//
class GoodsTrnovrBrowser : public BrowserWindow {
public:
	GoodsTrnovrBrowser(uint rezID, SArray * a, PPViewGoodsTrnovr *pView, GCTFilt * f, int dataOwner) :
		BrowserWindow(rezID, a), P_View(pView), IsDataOwner(dataOwner)
	{
		const GoodsTrnovrFilt * p_filt = P_View ? static_cast<PPViewGoodsTrnovr *>(P_View)->GetFilt() : 0;
		if(p_filt)
			PPObjGoodsGroup::SetOwner(p_filt->GoodsGrpID, 0, (long)this);
	}
	~GoodsTrnovrBrowser()
	{
		const GoodsTrnovrFilt * p_filt = P_View ? static_cast<PPViewGoodsTrnovr *>(P_View)->GetFilt() : 0;
		if(p_filt)
			PPObjGoodsGroup::RemoveTempAlt(p_filt->GoodsGrpID, (long)this);
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
		char str_dt[12];
	};
	const _H * h_dt = static_cast<const _H *>(view->getCurItem());
	LDATE dt = ZERODATE;
	if(h_dt)
		strtodate(h_dt->str_dt, DATF_DMY, &dt);
	return dt;
}

IMPL_HANDLE_EVENT(GoodsTrnovrBrowser)
{
	LDATE dt;
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
//
// @ModuleDef(PPViewGoodsTrnovr)
//
SLAPI PPViewGoodsTrnovr::PPViewGoodsTrnovr() : P_Items(0)
{
}

SLAPI PPViewGoodsTrnovr::~PPViewGoodsTrnovr()
{
	delete P_Items;
}

const GoodsTrnovrFilt * SLAPI PPViewGoodsTrnovr::GetFilt() const { return &Filt; }

int SLAPI PPViewGoodsTrnovr::EditFilt(GoodsTrnovrFilt * pFilt)
{
	class GCTFiltDialog : public WLDialog {
	public:
		enum {
			ctlgroupGoodsFilt = 1,
			ctlgroupLoc       = 2
		};
		GCTFiltDialog(uint dlgID, int forceGoods) : WLDialog(dlgID, CTL_GTO_LABEL), ForceGoodsSelection(forceGoods)
		{
			addGroup(ctlgroupGoodsFilt, new GoodsFiltCtrlGroup(CTLSEL_GTO_GOODS, CTLSEL_GTO_GGRP, cmGoodsFilt));
			addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_GTO_LOC, 0, 0, cmLocList, 0, 0, 0));
			SetupCalCtrl(CTLCAL_GTO_PERIOD, this, CTL_GTO_PERIOD, 1);
		}
		int   setDTS(const GCTFilt * lf)
		{
			data = *lf;
			PPIDArray types;
			SetPeriodInput(this, CTL_GTO_PERIOD, &data.Period);
			{
				LocationCtrlGroup::Rec l_rec(&data.LocList);
				setGroupData(ctlgroupLoc, &l_rec);
			}
			SetupArCombo(this, CTLSEL_GTO_SUPPL, data.SupplID, OLW_LOADDEFONOPEN, GetSupplAccSheet(), sacfDisableIfZeroSheet);
			types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN,
				PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, PPOPT_GENERIC, 0L);
			SetupOprKindCombo(this, CTLSEL_GTO_OPR, data.OpID, 0, &types, 0);
			GoodsFiltCtrlGroup::Rec gf_rec(data.GoodsGrpID, data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
			if(ForceGoodsSelection)
				gf_rec.Flags |= GoodsCtrlGroup::disableEmptyGoods;
			setGroupData(ctlgroupGoodsFilt, &gf_rec);
			setWL((data.Flags & OPG_LABELONLY) ? 1 : 0);
			AddClusterAssocDef(CTL_GTO_ORDER, 0, GCTFilt::ordByDate);
			AddClusterAssoc(CTL_GTO_ORDER, 1, GCTFilt::ordByGoods);
			AddClusterAssoc(CTL_GTO_ORDER, 2, GCTFilt::ordByArticle);
			SetClusterData(CTL_GTO_ORDER, data.Order);
			if(getCtrlView(CTL_GTO_GENGGRP))
				setCtrlUInt16(CTL_GTO_GENGGRP, BIN(data.Flags & OPG_GRPBYGENGOODS));
			return 1;
		}
		int   getDTS(GCTFilt * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			ushort v;
			GoodsFiltCtrlGroup::Rec gf_rec;
			// AHTOXA {
			LocationCtrlGroup::Rec l_rec;
			getGroupData(ctlgroupLoc, &l_rec);
			data.LocList = l_rec.LocList;
			// } AHTOXA
			THROW(GetPeriodInput(this, sel = CTL_GTO_PERIOD, &data.Period));
			THROW(AdjustPeriodToRights(data.Period, 1));
			getCtrlData(CTLSEL_GTO_SUPPL, &data.SupplID);
			getCtrlData(CTLSEL_GTO_OPR,   &data.OpID);
			THROW(getGroupData(sel = ctlgroupGoodsFilt, &gf_rec));
			data.GoodsGrpID = gf_rec.GoodsGrpID;
			data.GoodsID    = gf_rec.GoodsID;
			SETFLAG(data.Flags, OPG_LABELONLY, getWL());
			GetClusterData(CTL_GTO_ORDER, &data.Order);
			if(getCtrlView(CTL_GTO_GENGGRP)) {
				getCtrlData(CTL_GTO_GENGGRP, &v);
				SETFLAG(data.Flags, OPG_GRPBYGENGOODS, v);
			}
			else
				data.Flags &= ~OPG_GRPBYGENGOODS;
			ASSIGN_PTR(pData, data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		GCTFilt data;
		int    ForceGoodsSelection;
	};
	DIALOG_PROC_BODY_P2(GCTFiltDialog, DLG_GTO, (int)(pFilt->Flags & OPG_FORCEGOODS), pFilt);
}

int SLAPI PPViewGoodsTrnovr::Init(const GoodsTrnovrFilt * pFilt)
{
	int    ok = 1;
	LDATE dt = ZERODATE;
	GoodsTrnovrViewItem entry, total;
	SString wait_msg;
	IterCounter cntr;
	GoodsGrpngEntry * e;
	PPOprKind opk, lopk;
	GCTFilt f;
	AdjGdsGrpng agg;
	int    zero;
	GoodsGrpngArray gga;
	if(!RVALUEPTR(Filt, pFilt))
		Filt.Init(1, 0);
	if(P_Items)
		P_Items->freeAll();
	else {
		THROW_MEM(P_Items = new SArray(sizeof(entry)));
	}
	f = Filt; // AHTOXA
	cntr.Init(diffdate(&Filt.Period.upp, &Filt.Period.low, 0) + 1);
	dt = f.Period.low;
	if(!dt)
		THROW(BillObj->P_Tbl->GetFirstDate(0, &dt) > 0);
	SETIFZ(f.Period.upp, LConfig.OperDate);
	MEMSZERO(total);
	if(Filt.Flags & OPG_DONTSHOWPRGRSBAR)
		PPLoadText(PPTXT_CALCOPGRPNG, wait_msg);
	while(dt <= Filt.Period.upp) {
		zero = 1;
		MEMSZERO(entry);
		f.Period.SetDate(dt);
		f.Flags |= OPG_PROCESSRECKONING;
		entry.Dt = dt;
		datefmt(&dt, DATF_DMY, entry.Title);
		gga.Reset();
		THROW(agg.BeginGoodsGroupingProcess(&f));
		THROW(gga.ProcessGoodsGrouping(&f, &agg));
		for(uint i = 0; gga.enumItems(&i, (void **)&e);) {
			if(oneof3(e->OpID, -1, 10000, 0))
				continue;
			if(IsIntrExpndOp(e->OpID))
				entry.XpndIntr += e->Cost;
			else if(IsIntrOp(e->OpID) == INTRRCPT)
				entry.RcptIntr += e->Cost;
			else {
				THROW(GetOpData(e->OpID, &opk));
				if(e->OpTypeID == PPOPT_GOODSRECEIPT)
					entry.RcptSuppl += e->Cost;
				else if(e->Link == 0) {
					if(CheckOpFlags(e->OpID, OPKF_PROFITABLE))
						if(opk.AccSheetID)
							entry.XpndClient += e->Price;
						else
							entry.XpndRetail += e->Price;
				}
				else {
					THROW(GetOpData(e->Link, &lopk));
					if(e->OpTypeID == PPOPT_GOODSRETURN) {
						if(lopk.OpTypeID == PPOPT_GOODSRECEIPT)
							entry.RetSuppl += e->Cost;
						else if(lopk.Flags & OPKF_PROFITABLE)
							if(lopk.AccSheetID)
								entry.RetClient += e->Price;
							else
								entry.RetRetail += e->Price;
					}
					else if(e->OpTypeID == PPOPT_PAYMENT) {
						if(CheckOpFlags(e->Link, OPKF_PROFITABLE))
							entry.PayClient += e->Price;
					}
				}
			}
			entry.Income += e->Income();
		}
		if(entry.RcptSuppl)
			zero = 0, total.RcptSuppl += entry.RcptSuppl;
		// @todo 01/05/2005 Внутренняя передача на списке складов
		if(f.LocList.GetCount() && entry.RcptIntr)
			zero = 0, total.RcptIntr += entry.RcptIntr;
		if(entry.RetRetail)
			zero = 0, total.RetRetail += entry.RetRetail;
		if(entry.RetClient)
			zero = 0, total.RetClient += entry.RetClient;
		if(entry.RetSuppl)
			zero = 0, total.RetSuppl += entry.RetSuppl;
		if(entry.XpndRetail)
			zero = 0, total.XpndRetail += entry.XpndRetail;
		if(entry.XpndClient)
			zero = 0, total.XpndClient += entry.XpndClient;
		// @todo 01/05/2005 Внутренняя передача на списке складов
		if(f.LocList.GetCount() && entry.XpndIntr)
			zero = 0, total.XpndIntr += entry.XpndIntr;
		if(entry.PayClient)
			zero = 0, total.PayClient += entry.PayClient;
		if(entry.Income)
			zero = 0, total.Income += entry.Income;
		if(!zero || !(f.Flags & OPG_IGNOREZERO))
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

int SLAPI PPViewGoodsTrnovr::InitIteration()
{
	int    ok = 1;
	Cntr.Init((P_Items && P_Items->getCount()) ? (P_Items->getCount()-1) : 0);
	return ok;
}

int FASTCALL PPViewGoodsTrnovr::NextIteration(GoodsTrnovrViewItem * pItem)
{
	int    ok = -1;
	if(P_Items->getCount() && Cntr < (ulong)((long)P_Items->getCount()-1)) {
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

int SLAPI PPViewGoodsTrnovr::GetIterationCount(long * pNumIteration, long * pIterCount)
{
	ASSIGN_PTR(pNumIteration, Cntr.GetTotal());
	ASSIGN_PTR(pIterCount, Cntr);
	return 1;
}

int SLAPI PPViewGoodsTrnovr::ViewGrouping(LDATE dt)
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

#if 0 // @v9.3.6 {
SArray * SLAPI PPViewGoodsTrnovr::MakeGoodsTurnover()
{
	GoodsTrnovrViewItem entry, total;
	uint   i;
	SString wait_msg;
	long   iter_count = 0, num_iters = 0;
	GoodsGrpngEntry * e;
	PPOprKind opk, lopk;
	GCTFilt f;
	AdjGdsGrpng agg;
	int    zero;
	SArray * ary = new SArray(sizeof(entry));
	GoodsGrpngArray gga;
	f = Filt; // AHTOXA
	num_iters = diffdate(&Filt.Period.upp, &Filt.Period.low, 0) + 1;
	LDATE dt = f.Period.low;
	THROW_MEM(ary);
	if(!dt)
		THROW(BillObj->tbl->GetFirstDate(0, &dt) > 0);
	SETIFZ(f.Period.upp, LConfig.OperDate);
	MEMSZERO(total);
	if(Filt.Flags & OPG_DONTSHOWPRGRSBAR)
		PPLoadText(PPTXT_CALCOPGRPNG, wait_msg);
	while(dt <= Filt.Period.upp) {
		zero = 1;
		MEMSZERO(entry);
		f.Period.SetDate(dt);
		f.Flags |= OPG_PROCESSRECKONING;
		entry.Dt = dt;
		datefmt(&dt, DATF_DMY, entry.Title);
		gga.freeAll();
		THROW(BeginGoodsGroupingProcess(&f, &agg));
		THROW(gga.ProcessGoodsGrouping(&f, &agg));
		for(i = 0; gga.enumItems(&i, (void **)&e);) {
			if(oneof3(e->OpID, -1, 10000, 0))
				continue;
			if(IsIntrExpndOp(e->OpID))
				entry.XpndIntr += e->Cost;
			else if(IsIntrOp(e->OpID) == INTRRCPT)
				entry.RcptIntr += e->Cost;
			else {
				THROW(GetOpData(e->OpID, &opk));
				if(e->OpTypeID == PPOPT_GOODSRECEIPT)
					entry.RcptSuppl += e->Cost;
				else if(e->Link == 0) {
					if(CheckOpFlags(e->OpID, OPKF_PROFITABLE))
						if(opk.AccSheetID)
							entry.XpndClient += e->Price;
						else
							entry.XpndRetail += e->Price;
				}
				else {
					THROW(GetOpData(e->Link, &lopk));
					if(e->OpTypeID == PPOPT_GOODSRETURN) {
						if(lopk.OpTypeID == PPOPT_GOODSRECEIPT)
							entry.RetSuppl += e->Cost;
						else if(lopk.Flags & OPKF_PROFITABLE)
							if(lopk.AccSheetID)
								entry.RetClient += e->Price;
							else
								entry.RetRetail += e->Price;
					}
					else if(e->OpTypeID == PPOPT_PAYMENT) {
						if(CheckOpFlags(e->Link, OPKF_PROFITABLE))
							entry.PayClient += e->Price;
					}
				}
			}
			entry.Income += e->Income();
		}
		if(entry.RcptSuppl)
			zero = 0, total.RcptSuppl += entry.RcptSuppl;
		// @todo 01/05/2005 Внутренняя передача на списке складов
		if(f.LocList.GetCount() && entry.RcptIntr)
			zero = 0, total.RcptIntr += entry.RcptIntr;
		if(entry.RetRetail)
			zero = 0, total.RetRetail += entry.RetRetail;
		if(entry.RetClient)
			zero = 0, total.RetClient += entry.RetClient;
		if(entry.RetSuppl)
			zero = 0, total.RetSuppl += entry.RetSuppl;
		if(entry.XpndRetail)
			zero = 0, total.XpndRetail += entry.XpndRetail;
		if(entry.XpndClient)
			zero = 0, total.XpndClient += entry.XpndClient;
		// @todo 01/05/2005 Внутренняя передача на списке складов
		if(f.LocList.GetCount() && entry.XpndIntr)
			zero = 0, total.XpndIntr += entry.XpndIntr;
		if(entry.PayClient)
			zero = 0, total.PayClient += entry.PayClient;
		if(entry.Income)
			zero = 0, total.Income += entry.Income;
		if(!zero || !(f.Flags & OPG_IGNOREZERO))
			THROW_SL(ary->insert(&entry));
		plusdate(&dt, 1, 0);
		EndGoodsGroupingProcess(&agg);
		PPWaitPercent(iter_count++, num_iters, wait_msg);
	}
	PPGetWord(PPWORD_TOTAL, 0, total.Title, sizeof(total.Title));
	THROW_SL(ary->insert(&total));
	CATCH
		ZDELETE(ary);
	ENDCATCH
	EndGoodsGroupingProcess(&agg);
	return ary;
}
#endif // } 0 @v9.3.6

SArray * SLAPI PPViewGoodsTrnovr::CreateBrowserQuery()
{
	SArray * ary = new SArray(sizeof(GoodsTrnovrViewItem));
	CALLPTRMEMB(ary, copy(*P_Items));
	return ary;
}

int SLAPI PPViewGoodsTrnovr::Browse(int modeless)
{
	int    ok = 1;
	uint   res_id = 0;
	GoodsTrnovrBrowser * p_brw = 0;
	SArray * ary = 0;
	PPWait(1);
	THROW(ary = CreateBrowserQuery());
	res_id = Filt.LocList.GetCount() ? BROWSER_GOODSTURNOVER_BYLOC : BROWSER_GOODSTURNOVER;
	THROW_MEM(p_brw = new GoodsTrnovrBrowser(res_id, ary, this, &Filt, modeless));
	PPWait(0);
	PPOpenBrowser(p_brw, modeless);
	CATCHZOK
	if(!modeless || !ok)
		delete p_brw;
	return ok;
}

int SLAPI PPViewGoodsTrnovr::Print()
{
	int    ok = -1;
	PView  pv(this);
	PPAlddPrint(Filt.LocList.GetCount() ? REPORT_LGOODSTRNOVR : REPORT_GOODSTRNOVR, &pv);
	return ok;
}

int SLAPI ViewGoodsTurnover(long)
{
	int    ok = 1, view_in_use = 0;
	int    modeless = GetModelessStatus();
	GCTFilt flt;
	PPViewGoodsTrnovr * p_v = new PPViewGoodsTrnovr;
	BrowserWindow * p_prev_win = modeless ? PPFindLastBrowser() : 0;
	if(p_prev_win)
		flt = *static_cast<const GoodsTrnovrBrowser *>(p_prev_win)->P_View->GetFilt();
	else
		flt.Period.SetDate(LConfig.OperDate);
	while(p_v->EditFilt(&flt) > 0) {
		PPWait(1);
		flt.Flags |= (uint)OPG_IGNOREZERO;
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
	SString loc_name, name_buf;
	INIT_PPVIEW_ALDD_DATA(GoodsTrnovr, rsrv);
	GetLocationName(p_filt->LocList.GetSingle(), loc_name);
	loc_name.CopyTo(H.FltLocName, sizeof(H.FltLocName));
	GetSupplText(p_filt->SupplID, name_buf);
	name_buf.CopyTo(H.FltSupplName, sizeof(H.FltSupplName));
	H.FltBeg = p_filt->Period.low;
	H.FltEnd = p_filt->Period.upp;
	H.FltGoodsGrpID = p_filt->GoodsGrpID;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsTurnovr::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(GoodsTrnovr);
}

int PPALDD_GoodsTurnovr::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(GoodsTrnovr);
	I.Dt          = item.Dt;
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

void PPALDD_GoodsTurnovr::Destroy()
{
	DESTROY_PPVIEW_ALDD(GoodsTrnovr);
}
