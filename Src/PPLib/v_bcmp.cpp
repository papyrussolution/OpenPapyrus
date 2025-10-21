// V_BCMP.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2015, 2016, 2017, 2019, 2020, 2022, 2024, 2025
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewGoodsBillCmp)
//
IMPLEMENT_PPFILT_FACTORY(GoodsBillCmp); GoodsBillCmpFilt::GoodsBillCmpFilt() : PPBaseFilt(PPFILT_GOODSBILLCMP, 0, 1)
{
	SetFlatChunk(offsetof(GoodsBillCmpFilt, ReserveStart), offsetof(GoodsBillCmpFilt, Reserve)+sizeof(Reserve)-offsetof(GoodsBillCmpFilt, ReserveStart));
	SetBranchSVector(offsetof(GoodsBillCmpFilt, LhBillList));
	SetBranchSVector(offsetof(GoodsBillCmpFilt, RhBillList));
	Init(1, 0);
}

GoodsBillCmpFilt & FASTCALL GoodsBillCmpFilt::operator = (const GoodsBillCmpFilt & s)
{
	Copy(&s, 1);
	return *this;
}

PPViewGoodsBillCmp::PPViewGoodsBillCmp() : PPView(0, &Filt, PPVIEW_GOODSBILLCMP, 0, REPORT_GOODSBILLCMP), P_TempTbl(0), P_BObj(BillObj), IterIdx(1)
{
}

PPViewGoodsBillCmp::~PPViewGoodsBillCmp()
{
	delete P_TempTbl;
}

PPBaseFilt * PPViewGoodsBillCmp::CreateFilt(const void * extraPtr) const
{
	GoodsBillCmpFilt * p_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_GOODSBILLCMP, reinterpret_cast<PPBaseFilt **>(&p_filt))) {
		if(extraPtr) {
			const GoodsBillCmpFilt * p_sample_filt = static_cast<const GoodsBillCmpFilt *>(extraPtr);
			if(p_filt->IsA(p_sample_filt))
				*p_filt = *p_sample_filt;
		}
	}
	return static_cast<PPBaseFilt *>(p_filt);
}

int PPViewGoodsBillCmp::GetBillCodes(const GoodsBillCmpFilt * pFilt, SString & rLhCode, SString & rRhCode)
{
	int    r = 0;
	const  PPID lh_bill_id = pFilt->LhBillList.getCount() ? pFilt->LhBillList.get(0) : 0;
	const  PPID rh_bill_id = pFilt->RhBillList.getCount() ? pFilt->RhBillList.get(0) : 0;
	BillTbl::Rec bill_rec;
	if(oneof2(pFilt->WhatBillIsHistory, ISHIST_LEFTBILL, ISHIST_BOTHBILL)) {
		if(1) {
			ObjVersioningCore * p_ovc = PPRef->P_OvT;
			if(p_ovc && p_ovc->InitSerializeContext(1)) {
				SSerializeContext & r_sctx = p_ovc->GetSCtx();
				SBuffer buf;
				PPBillPacket pack;
				PPObjID oid;
				long   vv = 0;
				if(p_ovc->Search(lh_bill_id, &oid, &vv, &buf) > 0 && P_BObj->SerializePacket__(-1, &pack, buf, &r_sctx)) {
					bill_rec = pack.Rec;
					r = 1;
				}
			}
		}
		else {
			HistBillTbl::Rec hb_rec;
			r = Hb_.Search(lh_bill_id, &hb_rec);
			HistBillCore::HBRecToBRec(&hb_rec, &bill_rec);
		}
	}
	else {
		r = P_BObj->Search(lh_bill_id, &bill_rec);
	}
	if(r > 0)
		PPObjBill::MakeCodeString(&bill_rec, 0, rLhCode);
	else
		rLhCode.Cat(lh_bill_id);
	if(pFilt->LhBillList.getCount() > 1)
		rLhCode.Insert(0, "*");
	if(oneof2(pFilt->WhatBillIsHistory, ISHIST_RIGHTBILL, ISHIST_BOTHBILL)) {
		if(1) {
			ObjVersioningCore * p_ovc = PPRef->P_OvT;
			if(p_ovc && p_ovc->InitSerializeContext(1)) {
				SSerializeContext & r_sctx = p_ovc->GetSCtx();
				SBuffer buf;
				PPBillPacket pack;
				PPObjID oid;
				long   vv = 0;
				if(p_ovc->Search(rh_bill_id, &oid, &vv, &buf) > 0 && P_BObj->SerializePacket__(-1, &pack, buf, &r_sctx)) {
					bill_rec = pack.Rec;
					r = 1;
				}
			}
		}
		else {
			HistBillTbl::Rec hb_rec;
			r = Hb_.Search(rh_bill_id, &hb_rec);
			HistBillCore::HBRecToBRec(&hb_rec, &bill_rec);
		}
	}
	else
		r = P_BObj->Search(rh_bill_id, &bill_rec);
	if(r > 0)
		PPObjBill::MakeCodeString(&bill_rec, 0, rRhCode);
	else
		rRhCode.Cat(rh_bill_id);
	if(pFilt->RhBillList.getCount() > 1)
		rRhCode.Insert(0, "*");
	return 1;
}

int PPViewGoodsBillCmp::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	ushort v = 0;
	SString lh_code, rh_code;
	TDialog * dlg = 0;
	GoodsBillCmpFilt * p_filt = static_cast<GoodsBillCmpFilt *>(pBaseFilt);
	THROW(Filt.IsA(pBaseFilt));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_GBILLCMPFLT))));
	GetBillCodes(p_filt, lh_code, rh_code);
	dlg->setCtrlString(CTL_GBILLCMPFLT_LHBILL, lh_code);
	dlg->setCtrlString(CTL_GBILLCMPFLT_RHBILL, rh_code);
	dlg->disableCtrls(1, CTL_GBILLCMPFLT_LHBILL, CTL_GBILLCMPFLT_RHBILL, 0);
	SETFLAG(v, 0x01, p_filt->Flags & GoodsBillCmpFilt::fDiffQttyOnly);
	dlg->setCtrlData(CTL_GBILLCMPFLT_FLAGS, &v);
	v = 0;
	if(p_filt->Order == GoodsBillCmpFilt::ordByGoodsName)
		v = 0;
	else if(p_filt->Order == GoodsBillCmpFilt::ordByDiffQtty)
		v = 1;
	else if(p_filt->Order == GoodsBillCmpFilt::ordByDiffPrice)
		v = 2;
	else
		v = 0;
	dlg->setCtrlData(CTL_GBILLCMPFLT_ORDER, &v);
	for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		dlg->getCtrlData(CTL_GBILLCMPFLT_FLAGS, &(v = 0));
		SETFLAG(p_filt->Flags, GoodsBillCmpFilt::fDiffQttyOnly, v & 0x01);
		dlg->getCtrlData(CTL_GBILLCMPFLT_ORDER, &(v = 0));
		if(v == 0)
			p_filt->Order = GoodsBillCmpFilt::ordByGoodsName;
		else if(v == 1)
			p_filt->Order = GoodsBillCmpFilt::ordByDiffQtty;
		else if(v == 2)
			p_filt->Order = GoodsBillCmpFilt::ordByDiffPrice;
		else
			p_filt->Order = GoodsBillCmpFilt::ordByGoodsName;
		ok = valid_data = 1;
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPViewGoodsBillCmp::PutBillToTempTable(PPID billID, int side /* 1 - lh, 2 - rh */, int isHistory, const LDATETIME & rSjTime)
{
	int    ok = 1, r = 0;
	PPBillPacket pack;
	THROW_INVARG(oneof2(side, 1, 2));
	if(isHistory) {
		int   do_use_old_tech = 0;
		if(!!rSjTime) {
			LDATETIME moment;
			PPIDArray acn_list;
			acn_list.add(PPACN_EVENTTOKEN);
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			if(p_sj && p_sj->GetLastObjEvent(PPOBJ_EVENTTOKEN, PPEVTOK_OBJHIST9811, &acn_list, &moment) > 0) {
				if(cmp(moment, rSjTime) > 0)
					do_use_old_tech = 1;
			}
		}
		if(!do_use_old_tech) {
			SBuffer buf;
			ObjVersioningCore * p_ovc = PPRef->P_OvT;
			if(p_ovc && p_ovc->InitSerializeContext(1)) {
				SSerializeContext & r_sctx = p_ovc->GetSCtx();
				PPObjID oid;
				long   vv = 0;
				THROW(p_ovc->Search(billID, &oid, &vv, &buf) > 0);
				THROW(BillObj->SerializePacket__(-1, &pack, buf, &r_sctx));
				pack.ProcessFlags |= (PPBillPacket::pfZombie | PPBillPacket::pfUpdateProhibited);
				r = 1;
			}
		}
		else {
			PPHistBillPacket h_pack;
			if((r = Hb_.GetPacket(billID, &h_pack)) > 0)
				r = h_pack.ConvertToBillPack(&pack);
		}
	}
	else
		r = P_BObj->ExtractPacket(billID, &pack);
	if(r > 0) {
		SString temp_buf;
		PPTransferItem * p_ti;
		for(uint i = 0; pack.EnumTItems(&i, &p_ti);) {
			PPID   goods_id = labs(p_ti->GoodsID);
			double qtty = fabs(p_ti->Qtty());
			double price = (p_ti->Flags & PPTFR_SELLING) ? p_ti->NetPrice() : p_ti->Cost;
			TempGoodsBillCmpTbl::Key0 k0;
			TempGoodsBillCmpTbl::Rec rec;
			k0.GoodsID = goods_id;
			if(SearchByKey_ForUpdate(P_TempTbl, 0, &k0, &rec) > 0) {
				if(side == 1) {
					rec.LhPrice = ((rec.LhPrice * rec.LhQtty) + (price * qtty)) / (rec.LhQtty + qtty);
					rec.LhQtty += qtty;
				}
				else if(side == 2) {
					rec.RhPrice = ((rec.RhPrice * rec.RhQtty) + (price * qtty)) / (rec.RhQtty + qtty);
					rec.RhQtty += qtty;
				}
				rec.DiffQtty  = R6(rec.LhQtty - rec.RhQtty);
				rec.DiffPrice = R6(rec.LhPrice - rec.RhPrice);
				rec.IsDiffQtty  = BIN(rec.DiffQtty  != 0.0);
				rec.IsDiffPrice = BIN(rec.DiffPrice != 0.0);
				THROW_DB(P_TempTbl->updateRecBuf(&rec)); // @sfu
			}
			else {
				rec.Clear();
				rec.GoodsID = goods_id;
				STRNSCPY(rec.GoodsName, GetGoodsName(goods_id, temp_buf));
				GObj.GetSingleBarcode(goods_id, 0, temp_buf);
				temp_buf.CopyTo(rec.Barcode, sizeof(rec.Barcode));
				if(side == 1) {
					rec.LhPrice = price;
					rec.LhQtty  = qtty;
				}
				else if(side == 2) {
					rec.RhPrice = price;
					rec.RhQtty  = qtty;
				}
				rec.DiffQtty    = rec.LhQtty - rec.RhQtty;
				rec.DiffPrice   = rec.LhPrice - rec.RhPrice;
				rec.IsDiffQtty  = BIN(rec.DiffQtty  != 0.0);
				rec.IsDiffPrice = BIN(rec.DiffPrice != 0.0);
				THROW_DB(P_TempTbl->insertRecBuf(&rec));
			}
		}
	}
	CATCHZOK
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempGoodsBillCmp);

int PPViewGoodsBillCmp::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	ZDELETE(P_TempTbl);
	THROW(P_TempTbl = CreateTempFile());
	{
		uint   i;
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(i = 0; i < Filt.LhBillList.getCount(); i++) {
			LDATETIME ev_dtm = (Filt.LhBillList.getCount() == 1 && Filt.WhatBillIsHistory == ISHIST_LEFTBILL) ? Filt.LhSingleEvDtm : ZERODATETIME;
			THROW(PutBillToTempTable(Filt.LhBillList.get(i), 1, oneof2(Filt.WhatBillIsHistory, ISHIST_LEFTBILL, ISHIST_BOTHBILL), ev_dtm));
		}
		for(i = 0; i < Filt.RhBillList.getCount(); i++) {
			LDATETIME ev_dtm = (Filt.RhBillList.getCount() == 1 && Filt.WhatBillIsHistory == ISHIST_RIGHTBILL) ? Filt.RhSingleEvDtm : ZERODATETIME;
			THROW(PutBillToTempTable(Filt.RhBillList.get(i), 2, oneof2(Filt.WhatBillIsHistory, ISHIST_RIGHTBILL, ISHIST_BOTHBILL), ev_dtm));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPViewGoodsBillCmp::InitIteration()
{
	int    ok = 1;
	BtrDbKey k_;
	BtrDbKey k__;
	BExtQuery::ZDelete(&P_IterQuery);
	THROW_PP(P_TempTbl, PPERR_PPVIEWNOTINITED);
	Counter.Init();
	if(Filt.Order == GoodsBillCmpFilt::ordByGoodsName)
		IterIdx = 1;
	else if(Filt.Order == GoodsBillCmpFilt::ordByDiffQtty)
		IterIdx = 2;
	else if(Filt.Order == GoodsBillCmpFilt::ordByDiffPrice)
		IterIdx = 3;
	else
		IterIdx = 1;
	P_IterQuery = new BExtQuery(P_TempTbl, IterIdx, 24);
	P_IterQuery->selectAll();
	if(Filt.Flags & GoodsBillCmpFilt::fDiffQttyOnly)
		P_IterQuery->where(P_TempTbl->IsDiffQtty > 0.0);
	Counter.Init(P_IterQuery->countIterations(0, k__, spFirst));
	P_IterQuery->initIteration(false, k_, spFirst);
	CATCHZOK
	return ok;
}

int FASTCALL PPViewGoodsBillCmp::NextIteration(GoodsBillCmpViewItem * pItem)
{
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		P_TempTbl->CopyBufTo(pItem);
		return 1;
	}
	return -1;
}

DBQuery * PPViewGoodsBillCmp::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_GBILLCMP;
	DBQuery * q = 0;
	TempGoodsBillCmpTbl * tbl = new TempGoodsBillCmpTbl(P_TempTbl->GetName());
	DBE * dbe_minus_qtty  = &(tbl->DiffQtty * -1);
	DBE * dbe_minus_price = &(tbl->DiffPrice * -1);
	q = & select(
		tbl->GoodsID,     // #00
		tbl->GoodsName,   // #01
		tbl->Barcode,     // #02
		tbl->LhQtty,      // #03
		tbl->RhQtty,      // #04
		tbl->DiffQtty,    // #05
		*dbe_minus_qtty,  // #06
		tbl->LhPrice,     // #07
		tbl->RhPrice,     // #08
		tbl->DiffPrice,   // #09
		*dbe_minus_price, // #10
		0L).from(tbl, 0L);
	delete dbe_minus_qtty;
	delete dbe_minus_price;
	if(Filt.Flags & GoodsBillCmpFilt::fDiffQttyOnly)
		q->where(tbl->IsDiffQtty > 0.0);
	if(Filt.Order == GoodsBillCmpFilt::ordByGoodsName)
		q->orderBy(tbl->GoodsName, 0L);
	else if(Filt.Order == GoodsBillCmpFilt::ordByDiffQtty)
		q->orderBy(tbl->DiffQtty, 0L);
	else if(Filt.Order == GoodsBillCmpFilt::ordByDiffPrice)
		q->orderBy(tbl->DiffPrice, 0L);
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		SString lh_code, rh_code;
		GetBillCodes(&Filt, lh_code, rh_code);
		*pSubTitle = 0;
		pSubTitle->Cat(lh_code).Space().Cat("<->").Space().Cat(rh_code);
	}
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete tbl;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewGoodsBillCmp::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		int    r = 0;
		PPID   goods_id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_EDITGOODS:
				ok = -1;
				if(goods_id && GObj.Edit(&goods_id, 0) == cmOK)
					ok = 1;
				break;
			case PPVCMD_PUTTOBASKET:
				ok = -1;
				if(goods_id) {
					AddGoodsToBasket(goods_id, 0, 1, 0);
				}
				break;
			case PPVCMD_PUTTOBASKETALL:
				ok = -1;
				{
					int    col = pBrw->GetCurColumn();
					AddToBasketAll(oneof2(col, 4, 8) ? +1 : -1);
				}
				break;
		}
	}
	return ok;
}
//
//
//
int PPViewGoodsBillCmp::Print(const void * pHdr)
{
	return Helper_Print(REPORT_GOODSBILLCMP, Filt.Order);
}

int PPViewGoodsBillCmp::AddToBasketAll(int diffSign)
{
	int    ok = -1, r;
	RAssocArray list;
	{
		GoodsBillCmpViewItem item;
		for(InitIteration(); NextIteration(&item) > 0;) {
			if(item.DiffQtty > 0.0 && diffSign > 0) {
				list.Add(item.GoodsID, fabs(item.DiffQtty));
			}
			else if(item.DiffQtty < 0.0 && diffSign < 0) {
				list.Add(item.GoodsID, fabs(item.DiffQtty));
			}
		}
	}
	const  PPID lh_bill_id = Filt.LhBillList.getCount() ? Filt.LhBillList.get(0) : 0;
	if(list.getCount() && lh_bill_id) {
		SelBasketParam param;
		BillTbl::Rec bill_rec;
		THROW(P_BObj->Fetch(lh_bill_id, &bill_rec) > 0);
		THROW(r = GetBasketByDialog(&param, "GoodsBillCmp"));
		if(r > 0) {
			for(uint i = 0; i < list.getCount(); i++) {
				const  PPID goods_id = list.at(i).Key;
				const  double qtty = list.at(i).Val;
				ILTI   i_i;
				ReceiptTbl::Rec lot_rec;
				THROW(::GetCurGoodsPrice(goods_id, bill_rec.LocID, GPRET_MOSTRECENT, 0, &lot_rec) != GPRET_ERROR);
				i_i.GoodsID     = goods_id;
				i_i.UnitPerPack = lot_rec.UnitPerPack;
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
			THROW(GoodsBasketDialog(param, 1));
		}
	}
	CATCHZOKPPERR
	return ok;
}

void PPViewGoodsBillCmp::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_GBILLCMPTTL);
	long   count = 0;
	long   diff_count = 0;
	GoodsBillCmpViewItem item;
	for(InitIteration(); NextIteration(&item) > 0;) {
		count++;
		if(item.IsDiffQtty)
			diff_count++;
	}
	if(CheckDialogPtrErr(&dlg)) {
		SString lh_code, rh_code;
		GetBillCodes(&Filt, lh_code, rh_code);
		dlg->setCtrlString(CTL_GBILLCMPTTL_LHBILL, lh_code);
		dlg->setCtrlString(CTL_GBILLCMPTTL_RHBILL, rh_code);
		dlg->setCtrlLong(CTL_GBILLCMPTTL_COUNT, count);
		dlg->setCtrlLong(CTL_GBILLCMPTTL_DIFCOUNT, diff_count);
		ExecViewAndDestroy(dlg);
	}
}

int ViewGoodsBillCmp(PPID lhBillID, const PPIDArray & rRhBillList, int _modeless, int whatBillIsHistory, const LDATETIME * pLhEvDtm, const LDATETIME * pRhEvDtm)
{
	GoodsBillCmpFilt flt;
	flt.LhBillList.add(lhBillID);
	flt.RhBillList = rRhBillList;
	flt.Order = GoodsBillCmpFilt::ordByGoodsName;
	flt.WhatBillIsHistory = whatBillIsHistory;
	if(whatBillIsHistory == ISHIST_LEFTBILL) {
		RVALUEPTR(flt.LhSingleEvDtm, pLhEvDtm);
	}
	if(whatBillIsHistory == ISHIST_RIGHTBILL && flt.RhBillList.getCount() == 1) {
		RVALUEPTR(flt.RhSingleEvDtm, pRhEvDtm);
	}
	return PPView::Execute(PPVIEW_GOODSBILLCMP, 0, 0, &flt);
}
//
// Implementation of PPALDD_GoodsBillCmp
//
PPALDD_CONSTRUCTOR(GoodsBillCmp)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsBillCmp) { Destroy(); }

int PPALDD_GoodsBillCmp::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(GoodsBillCmp, rsrv);
	if(p_filt->LhBillList.getCount())
		H.LhBillID = p_filt->LhBillList.get(0);
	if(p_filt->RhBillList.getCount())
		H.RhBillID = p_filt->RhBillList.get(0);
	H.fOnlyDiffQtty = BIN(p_filt->Flags & GoodsBillCmpFilt::fDiffQttyOnly);
	return DlRtm::InitData(rFilt, rsrv);
}

void PPALDD_GoodsBillCmp::Destroy() { DESTROY_PPVIEW_ALDD(GoodsBillCmp); }
int  PPALDD_GoodsBillCmp::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/) { INIT_PPVIEW_ALDD_ITER(GoodsBillCmp); }

int PPALDD_GoodsBillCmp::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(GoodsBillCmp);
	I.GoodsID   = item.GoodsID;
	I.LhQtty    = item.LhQtty;
	I.LhPrice   = item.LhPrice;
	I.RhQtty    = item.RhQtty;
	I.RhPrice   = item.RhPrice;
	I.DiffQtty  = item.DiffQtty;
	I.DiffPrice = item.DiffPrice;
	FINISH_PPVIEW_ALDD_ITER();
}

