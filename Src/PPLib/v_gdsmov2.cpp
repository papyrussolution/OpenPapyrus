// V_GDSMOV2.CPP
// Copyright (c) A.Starodub 2011, 2015, 2016, 2018, 2019
//
#include <pp.h>
#pragma hdrstop

SLAPI PPViewGoodsMov2::PPViewGoodsMov2() : PPView(0, &Filt, PPVIEW_GOODSMOV2), P_BObj(BillObj), P_TempTbl(0), PrintWoPacks(0)
{
	ImplementFlags |= implUseServer;
	DefReportId = REPORT_GOODSMOV2;
	PPLoadString("inrest", InRestText);
	PPLoadString("outrest", OutRestText);
}

SLAPI PPViewGoodsMov2::~PPViewGoodsMov2()
{
	delete P_TempTbl;
}

PPBaseFilt * SLAPI PPViewGoodsMov2::CreateFilt(void * extraPtr) const
{
	BillFilt * p_filt = 0;
	PPView::CreateFiltInstance(PPFILT_GOODSMOV, (PPBaseFilt**)&p_filt);
	return p_filt;
}

// virtual
int SLAPI PPViewGoodsMov2::EditBaseFilt(PPBaseFilt * pFilt)
{
	DIALOG_PROC_BODY(GoodsMovFiltDialog, (GoodsMovFilt*)pFilt);
}

// virtual
int SLAPI PPViewGoodsMov2::GetTabTitle(long opID, SString & rBuf)
{
	if(opID) {
		if(opID == -1)
			rBuf = InRestText;
		else if(opID == 10000)
			rBuf = OutRestText;
		else
			GetObjectName(PPOBJ_OPRKIND, opID, rBuf);
	}
	return 1;
}

class GoodsMovCrosstab : public Crosstab {
public:
	explicit SLAPI  GoodsMovCrosstab(PPViewGoodsMov2 * pV) : Crosstab(), P_V(pV)
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
		return (pVal && /*typ == MKSTYPE(S_INT, 4) &&*/ P_V) ? P_V->GetTabTitle(*(const long *)pVal, rBuf) : 0;
	}

	PPViewGoodsMov2 * P_V;
	PPObjGoods GObj;
};

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempGoodsMov2);

int SLAPI PPViewGoodsMov2::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	int    is_link = 0;
	uint   i;
	const  int accs_cost = P_BObj->CheckRights(BILLRT_ACCSCOST);
	SString temp_buf;
	GCTFilt temp_filt;
	AdjGdsGrpng agg;
	GoodsGrpngEntry * p_entry;
	TempGoodsMov2Tbl::Rec rec;
	TempGoodsMov2Tbl * p_tbl = 0;
	PPOprKind op_rec;
	GoodsFilt gf;
	GoodsIterator  iter((GoodsFilt*)0, 0);
	Goods2Tbl::Rec gr;
	PPObjGoods gobj;

	THROW(Helper_InitBaseFilt(pFilt));
	THROW(p_tbl = CreateTempFile());
	Total.Init();
	Filt.Period.Actualize(ZERODATE); // @v8.6.8
	temp_filt.Period       = Filt.Period;
	temp_filt.LocList      = Filt.LocList;
	temp_filt.SupplID      = Filt.SupplID;
	temp_filt.GoodsGrpID   = Filt.GoodsGrpID;
	temp_filt.SupplAgentID = Filt.SupplAgentID; // AHTOXA
	temp_filt.OpID         = Filt.OpID;
	SETFLAG(temp_filt.Flags, OPG_LABELONLY, Filt.Flags & GoodsMovFilt::fLabelOnly);
	temp_filt.Flags |= (uint)(OPG_CALCINREST | OPG_CALCOUTREST | OPG_SETTAXES | OPG_PROCESSGENOP);
	if(Filt.Flags & GoodsMovFilt::fCostWoVat)
		temp_filt.Flags |= OPG_SETCOSTWOTAXES;
	{
		ObjRestrictArray op_list;
		BExtInsert bei(p_tbl);
		GoodsGrpngArray ary;
		SArray gds_op_list(sizeof(TempGoodsMov2Tbl::Rec));
		PPTransaction tra(ppDbDependTransaction, 1);
		GetGenericOpList(Filt.OpID, &op_list);
		THROW(tra);
		THROW(agg.BeginGoodsGroupingProcess(&temp_filt));
		gf.GrpID   = Filt.GoodsGrpID;
		gf.SupplID = Filt.SupplID;
		gf.BrandList.Add(Filt.BrandID);
		for(iter.Init(&gf, 0); iter.Next(&gr) > 0;) {
			THROW(PPCheckUserBreak());
			if(!(gr.Flags & GF_GENERIC)) {
				MEMSZERO(rec);
				temp_filt.GoodsID = gr.ID;
				gds_op_list.clear();
				ary.clear();
				rec.GoodsID  = gr.ID;
				THROW(ary.ProcessGoodsGrouping(&temp_filt, &agg));
				for(i = 0; ary.enumItems(&i, (void**)&p_entry);) {
					int no_upd_lot_op = 0;
					if(!accs_cost)
						p_entry->Cost = 0;
					if(!oneof2(p_entry->OpID, -1, 10000) && CheckOpFlags(p_entry->OpID, OPKF_NOUPDLOTREST, 0))
						no_upd_lot_op = 1;
					if(!no_upd_lot_op && p_entry->Quantity) {
						uint   pos = 0;
						rec.OpID     = p_entry->OpID;
						rec.Qtty     = p_entry->Sign * p_entry->Quantity;
						rec.Cost     = p_entry->Sign * p_entry->Cost;
						rec.Price    = p_entry->Sign * p_entry->Price;
						rec.Discount = p_entry->Sign * p_entry->Discount;
						rec.Amount   = p_entry->Sign * (rec.Price - rec.Discount);
						if(gds_op_list.lsearch(&rec, &pos, PTR_CMPFUNC(long), sizeof(long)) > 0) {
							TempGoodsMov2Tbl::Rec * p_rec = (TempGoodsMov2Tbl::Rec*)gds_op_list.at(pos);
							p_rec->Qtty     += rec.Qtty;
							p_rec->Cost     += rec.Cost;
							p_rec->Price    += rec.Price;
							p_rec->Discount += rec.Discount;
							p_rec->Amount   += rec.Amount;
						}
						else
							gds_op_list.insert(&rec);
						{
							if(rec.OpID == -1) {
								Total.InRestQtty    += rec.Qtty;
								Total.InRestPhQtty  += rec.Qtty * gr.PhUPerU;
								Total.InRestCost    += rec.Cost;
								Total.InRestPrice   += rec.Price;
							}
							else if(rec.OpID == 10000) {
								Total.OutRestQtty   += rec.Qtty;
								Total.OutRestPhQtty += rec.Qtty * gr.PhUPerU;
								Total.OutRestCost   += rec.Cost;
								Total.OutRestPrice  += rec.Price;
							}
						}
					}
				}
				for(i = 0; i < gds_op_list.getCount(); i++) {
					TempGoodsMov2Tbl::Rec * p_rec = (TempGoodsMov2Tbl::Rec*)gds_op_list.at(i);
					STRNSCPY(p_rec->GoodsName, gr.Name);
					gobj.FetchSingleBarcode(p_rec->GoodsID, temp_buf.Z()); // @v7.0.0 GetSingleBarcode-->FetchSingleBarcode
					temp_buf.CopyTo(p_rec->Barcode, sizeof(p_rec->Barcode));
					THROW_DB(bei.insert(p_rec));
				}
			}
			PPWaitPercent(iter.GetIterCounter());
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
		{
			Crosstab * p_prev_ct = P_Ct;
			P_Ct = 0;
			{
				uint price_word_id = 0;
				SString title;
				const DBField * p_price_fld = 0;
				DBFieldList total_list;

				THROW_MEM(P_Ct = new GoodsMovCrosstab(this)); // Crosstab
				P_Ct->SetTable(p_tbl, p_tbl->OpID);
				P_Ct->AddIdxField(p_tbl->GoodsID);
				P_Ct->AddInheritedFixField(p_tbl->GoodsName);
				P_Ct->AddInheritedFixField(p_tbl->Barcode);
				P_Ct->SetSortIdx("GoodsName", 0);
				// @v9.1.4 PPGetWord(PPWORD_QTTY, 0, title);
				PPLoadString("qtty", title); // @v9.1.4
				P_Ct->AddAggrField(p_tbl->Qtty, Crosstab::afSum, title, MKSFMTD(0, 3, NMBF_NOZERO));
				if(Filt.PriceKind == GoodsMovFilt::prkCost)
					p_price_fld = &p_tbl->Cost;
				else if(Filt.PriceKind == GoodsMovFilt::prkPrice)
					p_price_fld = &p_tbl->Amount;
				else
					p_price_fld = &p_tbl->Price;
				PPGetWord(PPWORD_SUM, 0, title.Z());
				P_Ct->AddAggrField(*p_price_fld, Crosstab::afSum, title, MKSFMTD(8, 2, NMBF_NOZERO));
				total_list.Add(p_tbl->Qtty);
				total_list.Add(*p_price_fld);
				P_Ct->AddTotalRow(total_list, 0, PPGetWord(PPWORD_TOTAL, 0, title.Z()));
				THROW(P_Ct->Create(1));
				ok = 1;
			}
			ZDELETE(p_prev_ct);
		}
	}
	ZDELETE(P_TempTbl);
	P_TempTbl = p_tbl;
	CATCH
		ok = 0;
		ZDELETE(p_tbl);
	ENDCATCH
	agg.EndGoodsGroupingProcess();
	return ok;
}

double SLAPI PPViewGoodsMov2::GetUnitsPerPack(PPID goodsID)
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

int SLAPI PPViewGoodsMov2::InitIteration(IterOrder ord)
{
	int    ok = 1;
	TempGoodsMov2Tbl::Key1 k1, k1_;
	MEMSZERO(k1);
	BExtQuery::ZDelete(&P_IterQuery);
	THROW_MEM(P_IterQuery = new BExtQuery(P_TempTbl, 1, 10));
	P_IterQuery->selectAll();
	k1_ = k1;
	Counter.Init(P_IterQuery->countIterations(0, &k1_, spFirst));
	P_IterQuery->initIteration(0, &k1, spFirst);
	CATCHZOK
	return ok;
}

int FASTCALL PPViewGoodsMov2::NextIteration(GoodsMov2ViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		if(pItem) {
			memzero(pItem, sizeof(GoodsMov2ViewItem));
			memcpy(pItem, &P_TempTbl->data, sizeof(TempGoodsMov2Tbl::Rec));
			pItem->UnitsPerPack = GetUnitsPerPack(pItem->GoodsID);
			Counter.Increment();
		}
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewGoodsMov2::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	BrwHdr hdr;
	MEMSZERO(hdr);
	GetEditIds(pHdr, &hdr, (pBrw) ? pBrw->GetCurColumn() : 0);
	if(hdr.GoodsID) {
		OpGroupingFilt op_grpng_flt;
		op_grpng_flt.Period  = Filt.Period;
		op_grpng_flt.LocList = Filt.LocList;
		op_grpng_flt.SupplID = Filt.SupplID;
		op_grpng_flt.SupplAgentID = Filt.SupplAgentID;
		op_grpng_flt.GoodsID = hdr.GoodsID;
		op_grpng_flt.OpID    = hdr.OpID;
		op_grpng_flt.Flags  |= OpGroupingFilt::fCalcRest;
		ViewOpGrouping(&op_grpng_flt);
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewGoodsMov2::EditGoods(PPID goodsID)
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

// virtual
int SLAPI PPViewGoodsMov2::Print(const void *)
{
	int    ok = 1;
	uint   rpt_id = REPORT_GOODSMOV2;
	PPReportEnv env;
	PView pv(this);
	PPAlddPrint(rpt_id, &pv, &env);
	ok = 1;
	return ok;
}

void SLAPI PPViewGoodsMov2::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, SetTempGoodsGrp(Filt.GoodsGrpID));
}

DBQuery * SLAPI PPViewGoodsMov2::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * p_q = PPView::CrosstabDbQueryStub;
	SString loc_names, subtitle;
	uint   brw_id = BROWSER_GOODSMOV_CT;
	subtitle.CatDivIfNotEmpty('-', 1).Cat(GetExtLocationName(Filt.LocList, 2, loc_names));
	ASSIGN_PTR(pBrwId, brw_id);
	ASSIGN_PTR(pSubTitle, subtitle);
	return p_q;
}

// virtual
int SLAPI PPViewGoodsMov2::ViewTotal()
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

void SLAPI PPViewGoodsMov2::GetEditIds(const void * pRow, PPViewGoodsMov2::BrwHdr * pHdr, long col)
{
	BrwHdr hdr;
	MEMSZERO(hdr);
	if(pRow) {
		if(P_Ct) {
			uint   tab_idx = col;
			int    r = (tab_idx > 1) ? P_Ct->GetTab((tab_idx - 2) / 2, &hdr.OpID) : (hdr.OpID = Filt.OpID, 1);
			if(r > 0)
				P_Ct->GetIdxFieldVal(0, pRow, &hdr.GoodsID, sizeof(hdr.GoodsID));
		}
		else
			hdr = *(BrwHdr*)pRow;
	}
	ASSIGN_PTR(pHdr, hdr);
}

int SLAPI PPViewGoodsMov2::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr hdr;
		MEMSZERO(hdr);
		GetEditIds(pHdr, &hdr, (pBrw) ? pBrw->GetCurColumn() : 0);
		ok = -2;
		switch(ppvCmd) {
			case PPVCMD_ADDTOBASKET:
				AddGoodsToBasket(hdr.GoodsID, Filt.LocList.GetSingle());
				break;
			case PPVCMD_EDITGOODS:
				ok = EditGoods(hdr.GoodsID);
				break;
			case PPVCMD_VIEWLOTS:
				if(hdr.GoodsID)
					::ViewLots(hdr.GoodsID, Filt.LocList.GetSingle(), Filt.SupplID, 0, 1);
				break;
		}
	}
	return ok;
}

int SLAPI PPViewGoodsMov2::SerializeState(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = -1;
	SString temp_buf;
	THROW(PPView::SerializeState(dir, rBuf, pCtx));
	THROW(Total.Serialize(dir, rBuf, pCtx));
	THROW(SerializeDbTableByFileName <TempGoodsMov2Tbl> (dir, &P_TempTbl, rBuf, pCtx));
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
		if(ind == 0 && P_TempTbl) {
			THROW_MEM(P_Ct = new GoodsMovCrosstab(this));
			THROW(P_Ct->Read(P_TempTbl, rBuf, pCtx));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI ViewGoodsMov(int modeless)
{
	int    ok = -1;
	GoodsMovFilt  filt;
	PPViewGoodsMov * p_v = new PPViewGoodsMov;
	PPViewBrowser * p_prev_win = modeless ? (PPViewBrowser *)PPFindLastBrowser() : 0;
	if(p_prev_win) {
		THROW(filt.Copy(p_prev_win->P_View->GetBaseFilt(), 1));
	}
	THROW_MEM(p_v);
	if(p_v->EditBaseFilt(&filt) > 0) {
		ZDELETE(p_v);
		if(filt.Flags & GoodsMovFilt::fUseOldAlg)
			ok = PPView::Execute(PPVIEW_GOODSMOV, &filt, PPView::exefModeless, 0);
		else
			ok = PPView::Execute(PPVIEW_GOODSMOV2, &filt, PPView::exefModeless, 0);
	}
	CATCHZOKPPERR
	ZDELETE(p_v);
	return ok;
}

//
// Implementation of PPALDD_GoodsMov2
//
PPALDD_CONSTRUCTOR(GoodsMov2)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(GoodsMov2) { Destroy(); }

int PPALDD_GoodsMov2::InitData(PPFilt & rFilt, long rsrv)
{
	PPViewGoodsMov2 * p_v = 0;
	if(rsrv) {
		p_v = (PPViewGoodsMov2*)rFilt.Ptr;
		Extra[1].Ptr = p_v;
	}
	else {
		p_v = new PPViewGoodsMov2;
		Extra[0].Ptr = p_v;
		p_v->Init_((GoodsMovFilt*)rFilt.Ptr);
	}
	SString temp_buf;
	const GoodsMovFilt * p_flt  = (const GoodsMovFilt*)(p_v->GetBaseFilt());
	H.FltBeg  = p_flt->Period.low;
	H.FltEnd  = p_flt->Period.upp;
	H.fLabelOnly    = (p_flt->Flags & GoodsMovFilt::fLabelOnly) ? 1 : 0;
	H.fCostWoVat    = (p_flt->Flags & GoodsMovFilt::fCostWoVat) ? 1 : 0;
	PPFormatPeriod(&p_flt->Period, temp_buf).CopyTo(H.Period, sizeof(H.Period));
	H.FltLocID      = p_flt->LocList.GetSingle();
	H.FltSupplID    = p_flt->SupplID;
	H.FltOpID       = p_flt->OpID;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsMov2::InitIteration(long iterId, int sortId, long rsrv)
{
	PPViewGoodsMov2 * p_v = (PPViewGoodsMov2 *)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	return p_v->InitIteration((PPViewGoodsMov2::IterOrder)SortIdx) ? 1 : 0;
}

int PPALDD_GoodsMov2::NextIteration(long iterId)
{
	IterProlog(iterId, 0);
	PPViewGoodsMov2 * p_v = (PPViewGoodsMov2*)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	GoodsMov2ViewItem item;
	if(p_v->NextIteration(&item) > 0) {
		long qttyf = (LConfig.Flags & CFGFLG_USEPACKAGE && !p_v->PrintWoPacks) ?
			MKSFMTD(0, 3, QTTYF_COMPLPACK | NMBF_NOTRAILZ) :
			MKSFMTD(0, 3, QTTYF_NUMBER | NMBF_NOTRAILZ);
		I.GoodsID = item.GoodsID;
		I.Qtty   = item.Qtty;
		I.Cost   = item.Cost;
		I.Price = item.Price;
		QttyToStr(item.Qtty, item.UnitsPerPack, qttyf, I.CQtty, 1);
		I.OpID = item.OpID;
		{
			SString op_name;
			p_v->GetTabTitle(item.OpID, op_name);
			op_name.CopyTo(I.OpName, sizeof(I.OpName));
		}
		return DlRtm::NextIteration(iterId);
	}
	else
		return -1;
}

void PPALDD_GoodsMov2::Destroy()
{
	delete (PPViewGoodsMov*)Extra[0].Ptr;
	Extra[0].Ptr = Extra[1].Ptr = 0;
}
