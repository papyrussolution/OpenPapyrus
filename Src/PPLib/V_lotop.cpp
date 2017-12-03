// V_LOTOP.CPP
// Copyright (c) A.Sobolev 1999, 2000-2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(LotOp); SLAPI LotOpFilt::LotOpFilt() : PPBaseFilt(PPFILT_LOTOP, 0, 0)
{
	SetFlatChunk(offsetof(LotOpFilt, ReserveStart),
		offsetof(LotOpFilt, Reserve)-offsetof(LotOpFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

SLAPI PPViewLotOp::PPViewLotOp() : PPView(0, &Filt, PPVIEW_LOTOP), P_BObj(BillObj), State(0)
{
	SETFLAG(State, stAccsCost, P_BObj->CheckRights(BILLRT_ACCSCOST));
	DefReportId = REPORT_LOTOPS;
}

int SLAPI PPViewLotOp::Init_(const PPBaseFilt * pFilt)
{
	State &= ~stCtrlX;
	return Helper_InitBaseFilt(pFilt);
}

int SLAPI PPViewLotOp::InitIteration()
{
	int    ok = 1;
	DBQ  * dbq = 0;
	DateRange period;
	TransferTbl::Key2 k;
	Transfer * p_tt = P_BObj->trfr;
	BExtQuery::ZDelete(&P_IterQuery);
	THROW_MEM(P_IterQuery = new BExtQuery(p_tt, 2));
	P_IterQuery->selectAll();
	period.SetZero();
	AdjustPeriodToRights(period, 0);
	if(Filt.Flags & LotOpFilt::fZeroLotOps)
		dbq = & (p_tt->LotID == 0L && daterange(p_tt->Dt, &period) &&
			(p_tt->Flags & PPTFR_RECEIPT) == PPTFR_RECEIPT);
	else {
		ReceiptTbl::Rec lot_rec;
		THROW(GetLotRec(&lot_rec) > 0);
		if(lot_rec.GoodsID < 0)
			dbq = & (p_tt->LotID == Filt.LotID && daterange(p_tt->Dt, &period));
		else
			dbq = & (p_tt->LotID == Filt.LotID && daterange(p_tt->Dt, &period) && p_tt->LocID == lot_rec.LocID);
	}
	P_IterQuery->where(*dbq);
	MEMSZERO(k);
	k.LotID = Filt.LotID;
	k.Dt = period.low;
	P_IterQuery->initIteration(0, &k, spGe);
	CATCHZOK
	return ok;
}

int FASTCALL PPViewLotOp::NextIteration(LotOpViewItem * pItem)
{
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		if(pItem) {
			P_BObj->trfr->copyBufTo(pItem);
			//*pItem = P_Trfr->data;
			if(!(State & stAccsCost))
				pItem->Cost = 0.0;
		}
		return 1;
	}
	return -1;
}

int SLAPI PPViewLotOp::GetLotRec(ReceiptTbl::Rec * pRec)
{
	int    ok = -1;
	if(Filt.LotID && P_BObj->trfr->Rcpt.Search(Filt.LotID, pRec) > 0)
		ok = 1;
	else
		memzero(pRec, sizeof(ReceiptTbl::Rec));
	return ok;
}

static void SLAPI dbqf_lotopladingbill_i(int option, DBConst * result, DBConst * params)
{
	PPID   bill_id = params[0].lval;
	if(bill_id) {
		BillTbl::Rec rec;
		if(BillObj->Search(bill_id, &rec) > 0) {
			if(rec.OpID == 0 && rec.LinkBillID)
				bill_id = rec.LinkBillID;
		}
	}
	result->init(bill_id);
}

// static
int PPViewLotOp::DynFuncLadingBill = 0;

DBQuery * SLAPI PPViewLotOp::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DbqFuncTab::RegisterDyn(&DynFuncLadingBill, BTS_INT, dbqf_lotopladingbill_i, 1, BTS_INT);

	PPID   loc_id = 0;
	uint   brw_id = 0;
	DateRange period;
	TransferTbl * trf = 0;
	BillTbl     * bll = 0;
	BillTbl     * lnk_bll = 0;
	DBQuery     * q   = 0;
	DBE    dbe_oprkind;
	DBE    dbe_ar;
	DBE    dbe_price;
	DBE    dbe_goods;
	DBE    dbe_bill;
	DBConst zero_cost;
	zero_cost.init(0.0);
	period.SetZero();
	AdjustPeriodToRights(period, 0);
	THROW(CheckTblPtr(trf = new TransferTbl));
	THROW(CheckTblPtr(bll = new BillTbl));
	PPDbqFuncPool::InitObjNameFunc(dbe_ar, PPDbqFuncPool::IdObjNameAr, bll->Object);
	{
		dbe_price.init();
		dbe_price.push(bll->OpID);
		dbe_price.push(trf->LotID);
		dbe_price.push(trf->Dt);
		dbe_price.push(trf->OprNo);
		dbe_price.push(trf->Cost);
		dbe_price.push(trf->Price);
		dbe_price.push(trf->Discount);
		dbe_price.push((DBFunc)PPDbqFuncPool::IdTrfrPrice);
	}
	if(Filt.Flags & LotOpFilt::fZeroLotOps) {
		brw_id = BROWSER_ZEROLOTOPS;
		PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoods, trf->GoodsID);
		if(State & stAccsCost)
			q = &select(trf->Dt, trf->OprNo, bll->Code, dbe_ar, trf->Quantity,
				trf->Rest, trf->Cost, dbe_price, dbe_goods, 0L);
		else
			q = &select(trf->Dt, trf->OprNo, bll->Code, dbe_ar, trf->Quantity,
				trf->Rest, zero_cost, dbe_price, dbe_goods, 0L);
		q->from(trf, bll, 0L).where(trf->LotID == 0L && daterange(trf->Dt, &period) &&
			(trf->Flags & PPTFR_RECEIPT) == PPTFR_RECEIPT && bll->ID == trf->BillID).
			orderBy(trf->LotID, trf->Dt, trf->OprNo, 0L);
	}
	else {
		ReceiptTbl::Rec lot_rec;
		PPObjGoods goods_obj;
		THROW(P_BObj->trfr->Rcpt.Search(Filt.LotID, &lot_rec) > 0);
		if(goods_obj.CheckFlag(lot_rec.GoodsID, GF_USEINDEPWT))
			brw_id = BROWSER_OPSBYLOT_WT;
		else
			brw_id = BROWSER_OPSBYLOT;
		PPDbqFuncPool::InitObjNameFunc(dbe_oprkind, PPDbqFuncPool::IdObjNameOprKind, bll->OpID);
		q = &select(trf->Dt, 0L);    // #0
		q->addField(trf->OprNo);     // #1
		q->addField(bll->Code);      // #2
		q->addField(dbe_ar);         // #3
		q->addField(trf->Quantity);  // #4
		q->addField(trf->Rest);      // #5
		if(State & stAccsCost)
			q->addField(trf->Cost);  // #6
		else
			q->addField(zero_cost);  // #6
		q->addField(dbe_price);      // #7
		q->addField(dbe_oprkind);    // #8
		q->addField(trf->WtQtty);    // #9
		q->addField(trf->WtRest);    // #10
		q->from(trf, bll, 0L);
		if(lot_rec.GoodsID < 0) {
			PPDbqFuncPool::InitLongFunc(dbe_bill, DynFuncLadingBill, trf->BillID);
			q->where(trf->LotID == Filt.LotID && daterange(trf->Dt, &period) && (bll->ID += dbe_bill));
		}
		else {
			loc_id = lot_rec.LocID;
			q->where(trf->LotID == Filt.LotID && daterange(trf->Dt, &period) &&
				trf->LocID == loc_id && (bll->ID += trf->BillID));
		}
		q->orderBy(trf->LotID, trf->Dt, trf->OprNo, 0L);
	}
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		*pSubTitle = 0;
		if(Filt.LotID) {
			ReceiptTbl::Rec lot_rec;
			if(P_BObj->trfr->Rcpt.Search(Filt.LotID, &lot_rec) > 0) {
				SString temp_buf;
				pSubTitle->Cat(GetGoodsName(lot_rec.GoodsID, temp_buf)).CatDiv('-', 1);
				GetLocationName(lot_rec.LocID, temp_buf);
				pSubTitle->Cat(temp_buf);
			}
		}
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete bll;
			delete trf;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewLotOp::SearchOp(const BrwHdr * pHdr, TransferTbl::Rec * pRec)
{
	if(pHdr) {
		TransferTbl::Key1 k;
		k.Dt = pHdr->Dt;
		k.OprNo = pHdr->OprNo;
		return SearchByKey(P_BObj->trfr, 1, &k, pRec);
	}
	else
		return 0;
}

int SLAPI PPViewLotOp::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr * p_hdr = pHdr ? (BrwHdr *)pHdr : 0;
		switch(ppvCmd) {
			case PPVCMD_EDITBILL:
				ok = -1;
				if(p_hdr && P_BObj && P_BObj->trfr) {
					TransferTbl::Rec rec;
					if(SearchOp(p_hdr, &rec) > 0) {
						PPID   bill_id = rec.BillID;
						BillTbl::Rec bill_rec;
						if(P_BObj->Search(bill_id, &bill_rec) > 0) {
							if(bill_rec.OpID == 0 && bill_rec.LinkBillID)
								bill_id = bill_rec.LinkBillID;
							int    r = P_BObj->Edit(&bill_id, 0);
							if(r == 0)
								ok = PPErrorZ();
							else if(r > 0)
								ok = 1;
						}
					}
				}
				break;
			case PPVCMD_MOVLOTOP:
				ok = MoveOp(p_hdr);
				break;
			case PPVCMD_DORECOVER:
				ok = Recover(p_hdr);
				break;
		}
	}
	return ok;
}

class RcvrTrfrDialog : public TDialog {
public:
	RcvrTrfrDialog() : TDialog(DLG_RCVRTRFR)
	{
	}
	int setDTS(const TransferTbl::Rec * pData)
	{
		Data = *pData;
		AddClusterAssoc(CTL_RCVRTRFR_WHAT, 0, 1);
		AddClusterAssoc(CTL_RCVRTRFR_WHAT, 1, 2);
		AddClusterAssoc(CTL_RCVRTRFR_WHAT, 2, 3);
		AddClusterAssoc(CTL_RCVRTRFR_WHAT, 3, 4);
		SetClusterData(CTL_RCVRTRFR_WHAT, 1);
		SString temp_buf;
		temp_buf.CatHex(Data.Flags);
		setCtrlString(CTL_RCVRTRFR_VAL, temp_buf);
		return 1;
	}
	int getDTS(TransferTbl::Rec * pData)
	{
		long   v = 0;
		SString temp_buf;
		GetClusterData(CTL_RCVRTRFR_WHAT, &v);
		getCtrlString(CTL_RCVRTRFR_VAL, temp_buf);
		if(v == 1) {
			sscanf(temp_buf.Strip(), "%lx", &Data.Flags);
		}
		else if(v == 2) {
			int    sign = (Data.Quantity < 0) ? -1 : 1;
			Data.Quantity = fabs(temp_buf.ToReal()) * sign;
		}
		else if(v == 3) {
			Data.Price = temp_buf.ToReal();
		}
		else if(v == 4) {
			Data.Discount = temp_buf.ToReal();
		}
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_RCVRTRFR_WHAT)) {
			long   v = 0;
			GetClusterData(CTL_RCVRTRFR_WHAT, &v);
			SString temp_buf;
			if(v == 1)
				temp_buf.CatHex(Data.Flags);
			else if(v == 2)
				temp_buf.Cat(Data.Quantity, MKSFMTD(0, 6, NMBF_NOTRAILZ));
			else if(v == 3)
				temp_buf.Cat(Data.Price, MKSFMTD(0, 5, NMBF_NOTRAILZ));
			else if(v == 4)
				temp_buf.Cat(Data.Discount, MKSFMTD(0, 5, NMBF_NOTRAILZ));
			setCtrlString(CTL_RCVRTRFR_VAL, temp_buf);
			clearEvent(event);
		}
	}
	TransferTbl::Rec Data;
};

int SLAPI PPViewLotOp::Recover(const BrwHdr * pHdr)
{
	int    ok = -1;
	RcvrTrfrDialog * dlg = 0;
	if(PPMaster && CConfig.Flags & CCFLG_DEBUG) {
		dlg = new RcvrTrfrDialog;
		TransferTbl::Rec rec;
		if(SearchOp(pHdr, &rec) > 0) {
			dlg->setDTS(&rec);
			if(ExecView(dlg) == cmOK) {
				long   v = 0;
				dlg->getDTS(&rec);
				dlg->GetClusterData(CTL_RCVRTRFR_WHAT, &v);
				DBUpdateSet * p_upd_set = 0;
				Transfer * p_tt = P_BObj->trfr;
				if(v == 1)
					p_upd_set = &set(p_tt->Flags, dbconst(rec.Flags));
				else if(v == 2)
					p_upd_set = &set(p_tt->Quantity, dbconst(rec.Quantity));
				else if(v == 3)
					p_upd_set = &set(p_tt->Price, dbconst(rec.Price));
				else if(v == 4)
					p_upd_set = &set(p_tt->Discount, dbconst(rec.Discount));
				if(p_upd_set) {
					THROW_DB(updateFor(p_tt, 1, (p_tt->Dt == pHdr->Dt && p_tt->OprNo == pHdr->OprNo), *p_upd_set));
				}
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPViewLotOp::MoveOp(const BrwHdr * pHdr)
{
	int    ok = -1;
	TransferTbl::Rec rec;
	if(SearchOp(pHdr, &rec) > 0) {
		const  PPID src_id = rec.LotID;
		//PPID   dest_id = 0;
		if((Filt.Flags & LotOpFilt::fZeroLotOps) || (src_id && !(rec.Flags & (PPTFR_REVAL|PPTFR_RECEIPT)))) {
			PPObjBill::SelectLotParam slp(rec.GoodsID, rec.LocID, src_id, 0);
			//while(SelectLot(rec.LocID, rec.GoodsID, src_id, &dest_id, 0) > 0) {
			while(P_BObj->SelectLot2(slp) > 0) {
				if(slp.RetLotID) {
					ok = P_BObj->trfr->MoveLotOp(src_id, pHdr->Dt, pHdr->OprNo, slp.RetLotID, 1) ? 1 : PPErrorZ();
					break;
				}
			}
		}
	}
	if(ok > 0)
		BaseState |= bsOuterChangesStatus;
	return ok;
}

int SLAPI PPViewLotOp::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	BrwHdr * p_hdr = pHdr ? (BrwHdr *)pHdr : 0;
	if(p_hdr)
		if(Filt.Flags & LotOpFilt::fZeroLotOps) {
			if(MoveOp(p_hdr) > 0)
				ok = 1;
		}
		else {
			TransferTbl::Rec rec;
			if(P_BObj->trfr->SearchMirror(p_hdr->Dt, p_hdr->OprNo, &rec) > 0 && rec.LotID) {
				Filt.LotID = rec.LotID;
				ChangeFilt(1, pBrw);
				ok = 1;
			}
		}
	return ok;
}

int SLAPI ViewOpersByLot(PPID lotID, int withZeroLotID)
{
	int    ok = -1;
	LotOpFilt flt;
	PPViewLotOp * p_v = new PPViewLotOp;
	THROW_MEM(p_v);
	flt.LotID = lotID;
	SETFLAG(flt.Flags, LotOpFilt::fZeroLotOps, withZeroLotID);
	PPWait(1);
	THROW(p_v->Init_(&flt));
	THROW(p_v->Browse(0));
	if(p_v->GetOuterChangesStatus())
		ok = 1;
	CATCHZOKPPERR
	delete p_v;
	return ok;
}
//
// Implementation of PPALDD_LotOps
//
PPALDD_CONSTRUCTOR(LotOps)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(LotOps)
{
	Destroy();
}

int PPALDD_LotOps::InitData(PPFilt & rFilt, long rsrv)
{
	PPViewLotOp * p_v = 0;
	if(rsrv) {
		p_v = (PPViewLotOp*)rFilt.Ptr;
		Extra[1].Ptr = p_v;
	}
	else {
		p_v = new PPViewLotOp;
		Extra[0].Ptr = p_v;
		p_v->Init_((LotOpFilt*)rFilt.Ptr);
	}
	ReceiptTbl::Rec lot_rec;
	p_v->GetLotRec(&lot_rec);
	H.LotID = lot_rec.ID;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_LotOps::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(LotOp);
}

int PPALDD_LotOps::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(LotOp);
	I.BillID   = item.BillID;
	I.CurID    = item.CurID;
	I.Dt       = item.Dt;
	I.OprNo    = (short)item.OprNo;
	I.Qtty     = item.Quantity;
	I.Rest     = item.Rest;
	I.Cost     = TR5(item.Cost);
	I.Price    = TR5(item.Price);
	I.Discount = TR5(item.Discount);
	I.CurPrice = TR5(item.CurPrice);
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_LotOps::Destroy()
{
	DESTROY_PPVIEW_ALDD(LotOp);
}

