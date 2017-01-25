// ILBPACK.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
//
//
//
static void __LogDebugMessage(ObjTransmContext * pCtx, const char * pMsg)
{
//#ifndef NDEBUG
	if(pCtx && !isempty(pMsg)) {
		pCtx->OutReceivingMsg(pMsg);
	}
//#endif
}

SLAPI ILTI::ILTI(const PPTransferItem * pTI)
{
	THISZERO();
	if(pTI)
		Init__(pTI);
}

int SLAPI ILTI::HasDeficit() const
{
	return BIN(fabs(Rest) >= BillCore::GetQttyEpsilon());
}

int SLAPI ILTI::Setup(PPID goodsID, int sign, double qtty, double cost, double price)
{
	GoodsID = goodsID;
	if(sign > 0)
		SetQtty(qtty, 0.0, PPTFR_PLUS);
	else if(sign < 0)
		SetQtty(qtty, 0.0, PPTFR_MINUS);
	Cost = cost;
	Price = price;
	return 1;
}

int SLAPI ILTI::SetQtty(double qtty, double wtQtty, long flags)
{
	if(flags & PPTFR_PLUS)
		qtty = fabs(qtty);
	else if(flags & PPTFR_MINUS)
		qtty = -fabs(qtty);
	Quantity = Rest = qtty;
	Flags |= flags;
	return 1;
}

void FASTCALL ILTI::Init__(const PPTransferItem * ti)
{
	const double qtty = ti->Qtty();
	BillID      = 0;
	GoodsID     = ti->GoodsID;
	//
	// @v7.6.1
	// При необходимости поля LotSyncID и LotMirrID должны инициализироваться после вызова ILTI::Init
	//
	LotSyncID   = 0;
	LotMirrID   = 0;
	//
	UnitPerPack = ti->UnitPerPack;
	double nq   = R6(Quantity + qtty);
	if(nq > 0.0) {
		Cost = (Cost * Quantity + ti->Cost * qtty) / nq;
		const double mp = Price * Quantity;
		Price = (((ti->Flags & PPTFR_REVAL) ? ti->Price : ti->NetPrice()) * qtty + mp) / nq;
	}
	else {
		Cost = ti->Cost;
		Price = (ti->Flags & PPTFR_REVAL) ? ti->Price : ti->NetPrice();
	}
	CurPrice   = ti->CurPrice;
	if(ti->Flags & PPTFR_ORDER) {
		QuotPrice = ti->Price; // @v9.2.9 Для заказа в QuotPrice кладем базовую цену дабы на принимающей стороне вычленить скидку
	}
	else
		QuotPrice  = ti->QuotPrice;
	Quantity  += qtty;
   	Rest      += qtty;
	Flags      = ti->Flags;
	Suppl      = ti->Suppl;
	QCert      = ti->QCert;
	Expiry     = ti->Expiry;
	InTaxGrpID = ti->LotTaxGrpID;
	RByBill    = ti->RByBill; // @v8.0.3
	memzero(Reserve, sizeof(Reserve)); // @v8.0.3
}
//
//
//
struct _LotCmp {
	_LotCmp & init(uint p, const ReceiptTbl::Rec * lotr, double cost, double price)
	{
		pos        = p;
		lot        = lotr->ID;
		cost_diff  = (cost  != 0.0) ? fabs(cost  - R5(lotr->Cost))  : 0.0;
		price_diff = (price != 0.0) ? fabs(price - R5(lotr->Price)) : 0.0;
		return *this;
	}
	uint   pos;
	PPID   lot;
	double cost_diff;
	double price_diff;
};

IMPL_CMPFUNC(_LotCmp, p1, p2)
{
	int    si = 0;
	CMPCASCADE3(si, (_LotCmp*)p1, (_LotCmp*)p2, cost_diff, price_diff, pos);
	return si;
}

int SLAPI PPObjBill::OrderLots(const PPBillPacket * pPack, PPIDArray * pLots, PPID genGoodsID, double cost, double price, double qtty)
{
	int    ok = 1;
	uint   i;
	ReceiptTbl::Rec   lotr;
	TSArray <_LotCmp> _lots;
	for(i = 0; qtty < 0.0 && i < pLots->getCount(); i++) {
		PPID   lot_id = pLots->at(i);
		double rest = 0.0, ratio;
		THROW(pPack->BoundsByLot(lot_id, 0, -1, &rest, 0));
		if(rest > 0.0) {
			_LotCmp lc;
			THROW(trfr->Rcpt.Search(lot_id, &lotr) > 0);
			if(genGoodsID)
				if(GObj.IsGoodsCompatibleByUnit(lotr.GoodsID, genGoodsID, &ratio) > 0)
					rest /= ratio;
				else
					continue;
			THROW(trfr->GetLotPrices(&lotr, pPack->Rec.Dt));
			THROW_SL(_lots.ordInsert(&lc.init(i, &lotr, cost, price), 0, PTR_CMPFUNC(_LotCmp)));
			if(lc.cost_diff == 0 && lc.price_diff == 0)
				qtty += rest;
		}
   	}
	pLots->freeAll();
	for(i = 0; i < _lots.getCount(); i++)
		THROW_SL(pLots->add(_lots.at(i).lot));
	CATCHZOK
	return ok;
}

struct CmpGenLots {
	SLAPI  CmpGenLots(PPID lot, LDATE d, long oprNo)
	{
		lotID = lot;
		dt = d;
		o = oprNo;
	}
	LDATE  dt;
	long   o;
	PPID   lotID;
};

static IMPL_CMPFUNC(CmpGenLots, i1, i2)
{
	return memcmp(i1, i2, sizeof(CmpGenLots));
}

static int SLAPI SetupTI(PPTransferItem * pTI, const PPBillPacket * pPack, PPID goodsID, PPID lotID)
{
	int    ok = 1;
	memzero(pTI, sizeof(*pTI));
	THROW(pTI->Init(&pPack->Rec, 1, -1));
	THROW(pTI->SetupGoods(goodsID, 0));
	THROW(pTI->SetupLot(lotID, 0, 0));
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::Helper_ConvertILTI_Subst(ILTI * ilti, PPBillPacket * pPack, IntArray * pRows,
	double * pQtty, long flags, const GoodsReplacementArray * pGra, char * pSerial)
{
	int    ok = 1, r;
	uint   i;
	PPID   goods_id = 0;
	double qtty = *pQtty;
	PPIDArray lots;
	GRI    local_gri(ilti->GoodsID);
	const  GRI * p_gri = pGra ? pGra->Search(ilti->GoodsID) : 0;
	const  int   by_serial = isempty(strip(pSerial)) ? 0 : 1;
	const  LDATE dt = pPack->Rec.Dt;
	const  PPID  loc_id = pPack->Rec.LocID;
	{
		LotArray lot_list;
		SArray cgla(sizeof(CmpGenLots));
		if(p_gri) {
			for(i = 0; i < p_gri->getCount(); i++) {
				goods_id = p_gri->GetSrcID(i);
				r = 0;
				lot_list.clear();
				trfr->Rcpt.GetListOfOpenedLots(-1, goods_id, loc_id, dt, &lot_list);
				for(uint j = 0; j < lot_list.getCount(); j++) {
					const ReceiptTbl::Rec & r_lot_rec = lot_list.at(j);
					CmpGenLots cgls(r_lot_rec.ID, r_lot_rec.Dt, r_lot_rec.OprNo);
					cgla.ordInsert(&cgls, 0, PTR_CMPFUNC(CmpGenLots));
					r = 1;
				}
				if(r)
					local_gri.Add(goods_id, p_gri->GetQtty(i), p_gri->GetRatio(i));
			}
		}
		else {
			RAssocArray goods_list;
			GObj.GetSubstList(ilti->GoodsID, BIN(flags & CILTIF_USESUBST_STRUCONLY), goods_list);
			for(i = 0; i < goods_list.getCount(); i++) {
				goods_id = goods_list.at(i).Key;
				r = 0;
				lot_list.clear();
				trfr->Rcpt.GetListOfOpenedLots(-1, goods_id, loc_id, dt, &lot_list);
				for(uint j = 0; j < lot_list.getCount(); j++) {
					const ReceiptTbl::Rec & r_lot_rec = lot_list.at(j);
					CmpGenLots cgls(r_lot_rec.ID, r_lot_rec.Dt, r_lot_rec.OprNo);
					cgla.ordInsert(&cgls, 0, PTR_CMPFUNC(CmpGenLots));
					r = 1;
				}
				if(r)
					local_gri.Add(goods_id, SMathConst::Max, goods_list.at(i).Val);
			}
		}
		for(i = 0; i < cgla.getCount(); i++)
			lots.addUnique(((CmpGenLots*)cgla.at(i))->lotID);
	}
	if(flags & CILTIF_OPTMZLOTS)
		THROW(OrderLots(pPack, &lots, ilti->GoodsID, ilti->Cost, ilti->Price, qtty));
	for(i = 0; qtty < 0.0 && i < lots.getCount(); i++) {
		PPID   lot_id = lots.at(i);
		ReceiptTbl::Rec lot_rec;
		THROW(trfr->Rcpt.Search(lot_id, &lot_rec) > 0);
		if(!by_serial || CmpSnrWithLotSnr(lot_id, pSerial)) {
			goods_id = lot_rec.GoodsID;
			uint   gri_pos = 0;
			if(local_gri.GetPosByGoods(goods_id, &gri_pos)) {
				double ratio    = local_gri.GetRatio(gri_pos);
				double max_qtty = local_gri.GetQtty(gri_pos);
				if(max_qtty > 0.0) {
					double rest = 0.0;
					THROW(pPack->BoundsByLot(lot_id, 0, -1, &rest, 0));
					if(rest > 0.0) {
						double rq  = qtty * ratio;
						double q   = (rest < -rq) ? rest : -rq;
						PPTransferItem ti;
						THROW(SetupTI(&ti, pPack, goods_id, lot_id));
						ti.Quantity_ = -q;
						ti.Discount = (ilti->Price > 0.0) ? (ti.Price - ilti->Price) : 0.0;
						THROW(pPack->InsertRow(&ti, pRows));
						qtty += q / ratio;
						local_gri.Add(goods_id, -fabs(q), local_gri.GetRatio(gri_pos));
					}
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pQtty, qtty);
	return ok;
}

int SLAPI PPObjBill::CmpSnrWithLotSnr(PPID lotID, const char * pSerial)
{
	SString serial;
	GetSerialNumberByLot(lotID, serial, 0);
	return (pSerial && serial.Cmp(pSerial, 0) == 0);
}

int SLAPI PPObjBill::AdjustIntrPrice(const PPBillPacket * pPack, PPID goodsID, double * pAdjPrice)
{
	int    ok = -1;
	double adj_intr_price_val = 0.0;
	int    intr = IsIntrOp(pPack->Rec.OpID);
	int    adj_intr_price = 0;
	PPID   adj_intr_loc = 0;
	LocationTbl::Rec loc_rec;
	if(intr == INTRRCPT) {
		if(LocObj.Fetch(pPack->Rec.LocID, &loc_rec) > 0 && loc_rec.Flags & LOCF_ADJINTRPRICE) {
			adj_intr_price = 1;
			adj_intr_loc = pPack->Rec.LocID;
		}
	}
	else if(intr == INTREXPND) {
		PPID loc_id = PPObjLocation::ObjToWarehouse(pPack->Rec.Object);
		if(loc_id && LocObj.Fetch(loc_id, &loc_rec) > 0 && loc_rec.Flags & LOCF_ADJINTRPRICE) {
			adj_intr_price = 1;
			adj_intr_loc = loc_id;
		}
	}
	if(adj_intr_price) {
		double p = 0.0;
		if(trfr->Rcpt.GetGoodsPrice(goodsID, adj_intr_loc, pPack->Rec.Dt, GPRET_INDEF, &p, 0) > 0) {
			adj_intr_price_val = p;
			if(adj_intr_price_val > 0.0)
				ok = 1;
		}
	}
	ASSIGN_PTR(pAdjPrice, adj_intr_price_val);
	return ok;
}

static SString & _MakeNSyncMsg(const ILTI * pIlti, SString & rMsgBuf, const char * pReason)
{
	// PPTXT_SYNCLOT_ROWNSYNC       "Строка преобразована без синхронизации. Причина:"
	SString fmt_buf;
	(rMsgBuf = 0).CatChar('\t').Cat(pIlti->RByBill).CatDiv(',', 2).Cat(pIlti->LotSyncID).CatDiv(',', 2).Cat(pIlti->LotMirrID).
		CatDiv(',', 2).Cat(pIlti->GoodsID);
	PPLoadText(PPTXT_SYNCLOT_ROWNSYNC, fmt_buf);
	fmt_buf.Space();
	if(pReason)
		fmt_buf.Cat(pReason);
	else
		fmt_buf.CatChar('-');
	rMsgBuf.CatDiv(':', 1).Cat(fmt_buf);
	return rMsgBuf;
}

static SString & _MakeNAvlLotMsg(const ILTI * pIlti, SString & rMsgBuf)
{
	// PPTXT_SYNCLOT_LOTNAVL        "Среди доступных лотов не найдено синхронизированного"
	SString fmt_buf;
	(rMsgBuf = 0).Tab().Cat(pIlti->RByBill).CatDiv(',', 2).Cat(pIlti->LotSyncID).CatDiv(',', 2).Cat(pIlti->LotMirrID).
		CatDiv(',', 2).Cat(pIlti->GoodsID);
	rMsgBuf.CatDiv(':', 1).Cat(PPLoadTextS(PPTXT_SYNCLOT_LOTNAVL, fmt_buf));
	return rMsgBuf;
}

static int _RowNSyncDiag(const ILTI * pIlti, const IntArray & rRows, long flags)
{
	int    ok = 1;
	SString msg_buf;
	if(rRows.getCount() != 1) {
		if(flags & CILTIF_REPFULLSYNCPROBLEM) {
			PPLogMessage(PPFILNAM_SYNCLOT_LOG, _MakeNSyncMsg(pIlti, msg_buf, "rows.getCount()!=1"), LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		}
		ok = 0;
	}
	else if(pIlti->RByBill == 0) {
		if(flags & CILTIF_REPFULLSYNCPROBLEM) {
			PPLogMessage(PPFILNAM_SYNCLOT_LOG, _MakeNSyncMsg(pIlti, msg_buf, "ilti->RByBill==0"), LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		}
		ok = 0;
	}
	return ok;
}

class AmtAjustment {
public:
	AmtAjustment(const BillTotalData & rTotal, double vatRate, const PPBillPacket & rBp, const IntArray & rRows) :
		SrcPriceAmount(rTotal.Amounts.Get(PPAMT_SELLING, 0) - rTotal.Amounts.Get(PPAMT_DISCOUNT, 0)),
		SrcPriceVat(rTotal.Amounts.Get(PPAMT_VATAX, 0)),
		VatRate(vatRate),
		//MinVatDiv(GetMinVatDivisor(fdiv100r(vatRate), 4)),
		R_Bp(rBp),
		R_Rows(rRows)
	{
		P_Btb = 0;
		RunningPriceAmount = 0.0;
		RunningPriceVat = 0.0;
		DiscountList.dim(rRows.getCount());
		ResetDiscountList();
	}
	~AmtAjustment()
	{
		delete P_Btb;
	}
	void  ResetDiscountList()
	{
		for(uint ridx = 0; ridx < R_Rows.getCount(); ridx++) {
			const PPTransferItem & r_ti = R_Bp.ConstTI(R_Rows.get(ridx));
			DiscountList[ridx] = R5(r_ti.Discount);
		}
	}
	struct PossibilityRange {
		PossibilityRange(uint position, ulong step)
		{
			Position = position;
			LowEdge = 0;
			UppEdge = 0;
			Step = step;
		}
		int     Verify() const
		{
			int    ok = 1;
			if(LowEdge || UppEdge) {
				if(Step == 0)
					ok = 0;
				else if(LowEdge > UppEdge)
					ok = 0;
				else if(((UppEdge - LowEdge) % Step) != 0)
					ok = 0;
			}
			return ok;
		}
		int     GetList(UlongArray & rList) const
		{
			int    ok = 1;
			rList.clear();
			assert(Verify());
			if(LowEdge || UppEdge) {
				for(ulong v = LowEdge; v <= UppEdge; v += Step) {
					rList.insert(&v);
				}
			}
			else
				ok = -1;
			return ok;
		}
		static int Helper_AddPossibilityItem(TSCollection <UlongArray> & rSrcList, uint level, UlongArray * pEnum)
		{
			int   ok = -1;
			if(level < rSrcList.getCount()) {
				UlongArray * p_src_item = rSrcList.at(level);
				if(p_src_item->getPointer() < p_src_item->getCount()) {
					ulong v = p_src_item->at(p_src_item->getPointer());
					THROW_SL(pEnum->insert(&v));
					int   r = Helper_AddPossibilityItem(rSrcList, level+1, pEnum);
					THROW(r);
					ok = 1;
					if(r < 0) {
						p_src_item->incPointer();
						if(p_src_item->getPointer() >= p_src_item->getCount()) {
							p_src_item->setPointer(0);
							ok = -1;
						}
					}
				}
			}
			CATCHZOK
			return ok;
		}
		static int GetPossibilitiesCollection(const TSArray <PossibilityRange> & rPrl, TSCollection <UlongArray> & rEnumList)
		{
			int    ok = 1;
			rEnumList.freeAll();
			TSCollection <UlongArray> vlist;
			ulong  _c = 1;
			for(uint n = 0; ok > 0 && n < rPrl.getCount(); n++) {
				UlongArray * p_new_list = vlist.CreateNewItem(0);
				rPrl.at(n).GetList(*p_new_list);
				p_new_list->setPointer(0);
				_c *= p_new_list->getCount();
				if(_c > 10000) 
					ok = -1;
			}
			if(ok > 0) {
				int    r;
				do {
					UlongArray * p_enum = rEnumList.CreateNewItem(0);
					r = Helper_AddPossibilityItem(vlist, 0, p_enum);
				} while(r > 0);
				assert(rEnumList.getCount() == _c);
				for(uint i = 0; i < rEnumList.getCount(); i++) {
					assert(rEnumList.at(i)->getCount() == rPrl.getCount());
				}
			}
			return ok;
		}
		uint    Position;
		ulong   LowEdge;
		ulong   UppEdge;
		ulong   Step;
	};
	int   ProcessRows(TSArray <PossibilityRange> * pPrL)
	{
		int    ok = 1;
		uint   ridx;
		UlongArray min_vat_div_list;
		UlongArray step_mult_list;
		RunningPriceAmount = 0.0;
		RunningPriceVat = 0.0;
		TotalRunning.Clear();
		if(pPrL)
			RowPrecList.clear();
		ZDELETE(P_Btb);
		THROW_MEM(P_Btb = new BillTotalBlock(&TotalRunning, R_Bp.Rec.OpID, 0, R_Bp.OutAmtType, BTC_CALCSALESTAXES));
		for(ridx = 0; ridx < R_Rows.getCount(); ridx++) {
			PPTransferItem part_ti = R_Bp.ConstTI(R_Rows.get(ridx));
			const double _part_qtty = fabs(part_ti.Quantity_);
			if(pPrL) {
				ulong  _step_mult = 1;
				long   _sl = (long)R0(log10(_part_qtty));
				if(_sl < 0 || (_sl == 0 && _part_qtty < 1.0)) {
					_step_mult = (ulong)R0(1.0 / _part_qtty);
					_sl = 0;
				}
				else if(_sl > 0) {
					SETMIN(_sl, 2); // @v9.4.3 4-->2
					_step_mult = (ulong)R0(fpow10i(_sl+1) / _part_qtty);
				}
				assert(_step_mult != 0);
				uint   vdp = 4 + _sl;
				ulong  min_vat_div = GetMinVatDivisor(fdiv100r(VatRate), vdp);
				RowPrecList.add((long)vdp);
				min_vat_div_list.insert(&min_vat_div);
				step_mult_list.insert(&_step_mult);
			}
			part_ti.Discount = DiscountList[ridx];
			P_Btb->Add(&part_ti);
		}
		P_Btb->Finish(0);
		RunningPriceAmount = TotalRunning.Amounts.Get(PPAMT_SELLING, 0) - TotalRunning.Amounts.Get(PPAMT_DISCOUNT, 0);
		RunningPriceVat = TotalRunning.Amounts.Get(PPAMT_VATAX, 0);
		if(IsInappropriateResult() && VatRate != 0.0 && pPrL) {
			const  double delta = SrcPriceAmount - GetRunningPriceAmount();
			TSArray <PossibilityRange> pr_list;
			for(ridx = 0; ridx < R_Rows.getCount(); ridx++) {
				const PPTransferItem & r_ti = R_Bp.ConstTI(R_Rows.get(ridx));
				const double _part_qtty = fabs(r_ti.Quantity_);
				double _low_rv = (r_ti.Price - DiscountList[ridx]) * _part_qtty;
				double _upp_rv = _low_rv + delta;
				ExchangeToOrder(&_low_rv, &_upp_rv);
				const ulong  min_vat_div = min_vat_div_list.at(ridx);
				PossibilityRange pr(ridx, min_vat_div);
				if(_part_qtty != 0.0) {
					const ulong _step_mult = step_mult_list.at(ridx);
					const ulong _step = min_vat_div * _step_mult;
					const ulong _shift = 4;
					const long rp = RowPrecList.get(ridx);
					const double _m = fpow10i(rp);
					{
						ulong  p   = (ulong)R0i(_low_rv * _m);
						ulong  n   = p / _step;
						ulong  mod = p % _step;
						pr.LowEdge = ((n-_shift) * _step);
					}
					{
						ulong  p   = (ulong)R0i(_upp_rv * _m);
						ulong  n   = p / _step;
						ulong  mod = p % _step;
						pr.UppEdge = ((n+_shift+1) * _step);
					}
					pr.Step = _step;
				}
				assert(pr.Verify());
				pr_list.insert(&pr);
			}
			assert(pr_list.getCount() == R_Rows.getCount());
			ASSIGN_PTR(pPrL, pr_list);
			ok = 2;
		}
		CATCHZOK
		return ok;
	}
	int TryAdjust(const UlongArray * pValueList)
	{
		int    ok = 1;
		assert(pValueList && pValueList->getCount() == R_Rows.getCount());
		assert(RowPrecList.getCount() == R_Rows.getCount());
		THROW(pValueList && pValueList->getCount() == R_Rows.getCount());
		ResetDiscountList();
		for(uint ridx = 0; ridx < R_Rows.getCount(); ridx++) {
			const long rp = RowPrecList.get(ridx);
			const double var_amount = round(pValueList->at(ridx) / fpow10i(rp), rp);
			const PPTransferItem & r_ti = R_Bp.ConstTI(R_Rows.get(ridx));
			const double _part_qtty = fabs(r_ti.Quantity_);
			const double _p = (var_amount / _part_qtty);
			DiscountList[ridx]  = R5(r_ti.Price - _p);
		}
		CATCHZOK
		return ok;
	}
	int  SetupTi(const UlongArray * pValueList, PPBillPacket * pPack) const
	{
		int    ok = 1;
		assert(pValueList && pValueList->getCount() == R_Rows.getCount());
		THROW(pValueList && pValueList->getCount() == R_Rows.getCount());
		for(uint ridx = 0; ridx < R_Rows.getCount(); ridx++) {
			const long rp = RowPrecList.get(ridx);
			const double var_amount = round(pValueList->at(ridx) / fpow10i(rp), rp);
			PPTransferItem & r_ti = pPack->TI(R_Rows.get(ridx));
			const double _part_qtty = fabs(r_ti.Quantity_);
			const double _p = (var_amount / _part_qtty);
			r_ti.Discount  = R5(r_ti.Price - _p);
		}
		CATCHZOK
		return ok;
	}
	const double SrcPriceAmount;
	const double SrcPriceVat;
	const double VatRate;
	//const ulong  MinVatDiv;
	const PPBillPacket & R_Bp;
	const IntArray & R_Rows;
	//
	BillTotalBlock * P_Btb;
	BillTotalData TotalRunning;
	RealArray DiscountList; // [R_Rows.getCount]
	LongArray RowPrecList; // [R_Rows.getCount]

	double GetRunningPriceAmount() const
	{
		return RunningPriceAmount;
	}
	double GetRunningPriceVat() const
	{
		return RunningPriceVat;
	}
	double GetDelta() const
	{
		return (SrcPriceAmount - RunningPriceAmount);
	}
	double GetVatDelta() const
	{
		return (SrcPriceVat - RunningPriceVat);
	}
	int    IsInappropriateResult() const
	{
		int    result = 0;
		if(fabs(GetDelta()) >= 1.0E-3)
			result |= 0x01;
		if(fabs(GetVatDelta()) >= 1.0E-3)
			result |= 0x02;
		return result;
	}
private:
	double RunningPriceAmount;
	double RunningPriceVat;
};

int SLAPI PPObjBill::ConvertILTI(ILTI * ilti, PPBillPacket * pPack, IntArray * pRows, uint flags,
	const char * pSerial, const GoodsReplacementArray * pGra)
{
	int    ok = 1;
	int    full_sync = 0; // Признак того, что сформированная строка документа полностью идентична ilti
	uint   i = 0;
	IntArray rows;
	StringSet * p_excl_serial = 0;
	PPTransferItem ti;
	double q, rest;
	double qtty = R6(ilti->Rest);
	double vatrate = 0.0;
	PPIDArray  lots;
	ReceiptTbl::Rec lotr;
	Goods2Tbl::Rec goods_rec;
	PPGoodsTaxEntry gtx;
	MEMSZERO(lotr);

	SString msg_buf, temp_buf;
	SString serial;
	(serial = pSerial).Strip();
	int    by_serial = BIN(!(flags & CILTIF_EXCLUDESERIAL) && serial.NotEmpty());
	THROW_PP_S(GObj.Fetch(labs(ilti->GoodsID), &goods_rec) > 0, PPERR_NEXISTGOODSINBILL, ilti->GoodsID);
	if(GObj.FetchTax(goods_rec.ID, pPack->Rec.Dt, 0L, &gtx) > 0)
		vatrate = gtx.GetVatRate();
	if(goods_rec.Flags & GF_UNLIM) {
		THROW(SetupTI(&ti, pPack, ilti->GoodsID, 0));
		ti.Quantity_ = (flags & CILTIF_ABSQTTY) ? fabs(qtty) : qtty;
		ti.Cost     = ilti->Cost;
		ti.Price    = ilti->Price;
		if(ti.CurID)
			ti.CurPrice = ilti->CurPrice;
		if(flags & CILTIF_QUOT)
			ti.Flags |= PPTFR_QUOT;
		THROW(pPack->InsertRow(&ti, &rows));
		if(flags & CILTIF_SYNC) {
			if(_RowNSyncDiag(ilti, rows, flags)) {
				const uint ti_pos = (uint)rows.at(0);
				pPack->TI(ti_pos).RByBill = ilti->RByBill;
				full_sync = 1;
			}
		}
		ilti->Rest = 0.0;
	}
	else {
		PPID   sync_lot_id = (flags & CILTIF_USESYNCLOT && ilti->LotSyncID) ? ilti->LotSyncID : 0;
		//
		// Подготовительные операции для выравнивания цены при межскладском приходе
		// по последнему лоту на складе-получателе
		//
		double adj_intr_price_val = 0.0;
		AdjustIntrPrice(pPack, ilti->GoodsID, &adj_intr_price_val);
		//
		if(oneof4(pPack->OprType, PPOPT_GOODSORDER, PPOPT_GOODSACK, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT)) { // @v7.5.10 PPOPT_DRAFTTRANSIT
			MEMSZERO(ti);
			double lot_price = 0.0;
			THROW(trfr->Rcpt.GetCurrentGoodsPrice(labs(ilti->GoodsID), pPack->Rec.LocID, GPRET_MOSTRECENT, &lot_price, &lotr));
			THROW(ti.Init(&pPack->Rec, 1, -1));
			THROW(ti.SetupGoods(ilti->GoodsID, TISG_SETPWOTF));
			ti.Price = ilti->Price;
			if(pPack->IsDraft())
				ti.Cost = ilti->Cost;
			else {
				long tisl = TISL_ADJPRICE;
				if(flags & CILTIF_ZERODSCNT && ti.Price > 0.0)
					tisl |= TISL_IGNPRICE;
				THROW(ti.SetupLot(lotr.ID, &lotr, tisl));
			}
			ti.LotID = 0;
			ti.Quantity_ = (flags & CILTIF_ABSQTTY) ? fabs(qtty) : qtty;
			if(ilti->Flags & PPTFR_ORDER) {
				if(ilti->QuotPrice > 0.0) {
					ti.Discount = ilti->QuotPrice - ilti->Price;
					ti.Price = ilti->QuotPrice;
				}
			}
			else {
				if(flags & CILTIF_QUOT)
					ti.Flags |= PPTFR_QUOT;
			}
			{
				IntArray temp_rows;
				THROW(pPack->InsertRow(&ti, &temp_rows));
				pPack->SnL.AddNumber(&temp_rows, serial);
				if(flags & CILTIF_SYNC && rows.getCount() == 1) {
					/* @todo строки таких документов пока не будем считать полностью синхронизированными. Надо доделать синхронизацию лотов заказов.
					ti.RByBill = ilti->RByBill;
					full_sync = 1;
					*/
				}
				for(uint i = 0; i < temp_rows.getCount(); i++) {
					THROW_SL(rows.insert(&temp_rows.at(i)));
				}
			}
			ilti->Rest = 0.0;
			qtty = 0.0;
		}
		else if(pPack->OprType == PPOPT_GOODSREVAL) {
			THROW_PP(sync_lot_id, PPERR_CANTACCEPTBILLRVL_SYNCLOT);
			THROW(trfr->Rcpt.Search(sync_lot_id, &lotr) > 0);
			THROW(ti.Init(&pPack->Rec, 1, -1));
			THROW(ti.SetupGoods(ilti->GoodsID, 0));
			THROW(ti.SetupLot(sync_lot_id, &lotr, 0));
			ti.Cost = ilti->Cost;
			ti.Price = ilti->Price;
			THROW(pPack->InsertRow(&ti, &rows));
			ilti->Rest = 0.0;
			qtty = 0.0;
			full_sync = 1;
		}
		else if(pPack->OprType == PPOPT_CORRECTION) {
			THROW_PP(sync_lot_id, PPERR_CANTACCEPTBILLRVL_SYNCLOT);
			THROW(trfr->Rcpt.Search(sync_lot_id, &lotr) > 0);
			THROW(ti.Init(&pPack->Rec, 1, -1));
			THROW(ti.SetupGoods(ilti->GoodsID, 0));
			THROW(ti.SetupLot(sync_lot_id, &lotr, 0));
			ti.Cost = ilti->Cost;
			ti.Price = ilti->Price;
			ti.Quantity_ = ilti->Quantity;
			THROW(pPack->InsertRow(&ti, &rows));
			ilti->Rest = 0.0;
			qtty = 0.0;
			full_sync = 1;
		}
		else if(qtty < 0.0) {
			uint   pass = 0;
			// @v9.2.7 {
			BillTotalData total_src;
			const double org_qtty = fabs(ilti->Quantity);
			const double org_sum_price = ilti->Price * org_qtty;
			if(flags & CILTIF_CAREFULLYALIGNPRICE) {
				const double __q = -qtty;
				PPTransferItem total_ti;
				THROW(SetupTI(&total_ti, pPack, ilti->GoodsID, 0));
				total_ti.Quantity_ = (flags & CILTIF_ABSQTTY) ? __q : -__q;
				const double price = (adj_intr_price_val > 0.0) ? adj_intr_price_val : ilti->Price;
				total_ti.Cost = ilti->Cost;
				total_ti.Price = ilti->Price;
				if(total_ti.CurID)
					total_ti.CurPrice = ilti->CurPrice;
                BillTotalBlock btb(&total_src, pPack->Rec.OpID, 0, pPack->OutAmtType, BTC_CALCSALESTAXES);
                btb.Add(&total_ti);
                btb.Finish(0);
			}
			// } @v9.2.7
			{
				LotArray lot_list;
				if(flags & CILTIF_EXCLUDESERIAL && !isempty(pSerial)) {
					THROW_MEM(p_excl_serial = new StringSet(';', pSerial));
				}
				THROW(trfr->Rcpt.GetListOfOpenedLots(-1, ilti->GoodsID, pPack->Rec.LocID, pPack->Rec.Dt, &lot_list));
				for(i = 0; i < lot_list.getCount(); i++) {
					const PPID lot_id = lot_list.at(i).ID;
					if(!p_excl_serial || GetSerialNumberByLot(lot_id, temp_buf, 0) <= 0 || !p_excl_serial->search(temp_buf, 0, 0))
						lots.add(lot_id);
				}
			}
			if(flags & CILTIF_OPTMZLOTS) {
				THROW(OrderLots(pPack, &lots, 0, ilti->Cost, ilti->Price, qtty));
			}
			if(sync_lot_id) {
				uint sync_lot_pos = 0;
				if(!lots.lsearch(sync_lot_id, &sync_lot_pos)) {
					sync_lot_id = 0;
					if(flags & CILTIF_REPFULLSYNCPROBLEM) {
						PPLogMessage(PPFILNAM_SYNCLOT_LOG, _MakeNAvlLotMsg(ilti, msg_buf), LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
					}
				}
				else if(sync_lot_pos > 0) {
					lots.swap(sync_lot_pos, 0); // Перемещаем искомый лот в начало списка дабы использовать его по-возможности в первую очередь.
				}
			}
			const double _qtty_epsilon = BillCore::GetQttyEpsilon();
			for(pass = 0; pass < 2; pass++) {
				//
				// Цикл прогоняем в два прохода для того, чтобы сначала использовать лоты строго по серийному номеру,
				// а затем, если можно, без учета серийного номера
				//
				for(i = 0; qtty < (-_qtty_epsilon) && i < lots.getCount(); i++) { // @v9.3.9 (qtty < 0)-->(qtty < (-_qtty_epsilon))
					PPID   lot_id = lots.get(i);
					if(!by_serial || CmpSnrWithLotSnr(lot_id, serial)) {
						THROW(pPack->BoundsByLot(lot_id, 0, -1, &rest, 0));
						// @v9.3.9 if(rest > 0.0) {
						if(rest >= _qtty_epsilon) { // @v9.3.9
							q = (rest < -qtty) ? rest : -qtty;
							THROW(SetupTI(&ti, pPack, ilti->GoodsID, lot_id));
							ti.Quantity_ = (flags & CILTIF_ABSQTTY) ? q : -q;
							const double price = (adj_intr_price_val > 0.0) ? adj_intr_price_val : ilti->Price;
							if((flags & CILTIF_ZERODSCNT) && !(flags & CILTIF_QUOT))
								ti.Discount = 0.0;
							else if(flags & CILTIF_ALLOWZPRICE && price == 0.0)
								ti.Discount = ti.Price;
							else
								ti.Discount = (price > 0.0) ? (ti.Price - price) : 0.0;
							if(flags & CILTIF_QUOT)
								ti.SetupQuot(price, 1);
							if(ti.CurID)
								ti.CurPrice = ilti->CurPrice;
							THROW(pPack->InsertRow(&ti, &rows));

							qtty += q;
						}
					}
				}
				if(!by_serial || !(flags & CILTIF_SUBSTSERIAL))
					break;
				else
					by_serial = 0;
			}
			// @v9.3.5 if(qtty == 0.0) {
			// @v9.3.9 if(feqeps(qtty, 0.0, 1.0E-8)) { // @v9.3.5
			// @v9.4.0 if(qtty < (-_qtty_epsilon)) { // @v9.3.9
			if(feqeps(qtty, 0.0, _qtty_epsilon)) { // @v9.4.0 
				if(flags & CILTIF_SYNC && sync_lot_id) {
					if(_RowNSyncDiag(ilti, rows, flags)) {
						const uint ti_pos = (uint)rows.at(0);
						if(pPack->TI(ti_pos).LotID == sync_lot_id) {
							pPack->TI(ti_pos).RByBill = ilti->RByBill;
							full_sync = 1;
						}
						else {
							if(flags & CILTIF_REPFULLSYNCPROBLEM) {
								_MakeNSyncMsg(ilti, msg_buf, "TI(ti_pos).LotID!=sync_lot_id");
								PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
							}
						}
					}
				}
				// @v9.2.7 {
				if(flags & CILTIF_CAREFULLYALIGNPRICE && rows.getCount() > 1) {
					AmtAjustment _aa(total_src, vatrate, *pPack, rows);
					TSArray <AmtAjustment::PossibilityRange> prl;
					const  int prr = _aa.ProcessRows(&prl);
					THROW(prr);
					int   iar = _aa.IsInappropriateResult();
					if(iar && prr == 2) {
						const double _org_delta = _aa.GetDelta();
						const double _org_vat_delta = _aa.GetVatDelta();
						TSCollection <UlongArray> enum_list;
						double _min_delta = MAXDOUBLE;
						double _min_vat_delta = MAXDOUBLE;
						double _best_from_bad_min_vat_delta = MAXDOUBLE;
						UlongArray appropriate_pos_list;
						uint   _optimal_zero_delta_pos = 0;
						uint   _best_from_bad_pos = 0;
						int    gpcr = AmtAjustment::PossibilityRange::GetPossibilitiesCollection(prl, enum_list);
						THROW(gpcr);
						if(gpcr > 0) { // @v9.4.3
							for(uint k = 0; k < enum_list.getCount(); k++) {
								const UlongArray * p_item = enum_list.at(k);
								_aa.TryAdjust(p_item);
								_aa.ProcessRows(0);
								const double _delta = _aa.GetDelta();
								const double _vat_delta = _aa.GetVatDelta();
								if(_aa.IsInappropriateResult()) {
									if(feqeps(_delta, 0.0, 1.0E-10)) {
										if(fabs(_vat_delta) < _min_vat_delta) {
											_min_vat_delta = fabs(_vat_delta);
											_optimal_zero_delta_pos = k+1;
										}
									}
									else if(!_optimal_zero_delta_pos) {
										if(fabs(_delta) < _min_delta) {
											_min_delta = fabs(_delta);
											if(fabs(_vat_delta) < _best_from_bad_min_vat_delta) {
												_best_from_bad_min_vat_delta = fabs(_vat_delta);
												_best_from_bad_pos = k+1;
											}
										}
									}
								}
								else {
									appropriate_pos_list.insert(&k);
									break;
								}
							}
							if(appropriate_pos_list.getCount()) {
								uint ap = appropriate_pos_list.at(0);
								const UlongArray * p_item = enum_list.at(ap);
								THROW(_aa.SetupTi(p_item, pPack));
							}
							else if(_optimal_zero_delta_pos > 0) {
								const UlongArray * p_item = enum_list.at(_optimal_zero_delta_pos-1);
								THROW(_aa.SetupTi(p_item, pPack));
							}
							/* (если применить - будет хуже) else if(_best_from_bad_pos > 0) {
								const UlongArray * p_item = enum_list.at(_best_from_bad_pos-1);
								THROW(_aa.SetupTi(p_item, pPack));
							}*/
						}
					}
				}
				// } @v9.2.7
			}
			else {
				if(flags & CILTIF_SYNC) {
					if(flags & CILTIF_REPFULLSYNCPROBLEM) {
						(temp_buf = 0).CatEq("deficit", qtty, MKSFMTD(0, 8, NMBF_NOTRAILZ));
						_MakeNSyncMsg(ilti, msg_buf, temp_buf);
						PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
					}
				}
				if(qtty < 0.0 && flags & CILTIF_USESUBST) {
					THROW(Helper_ConvertILTI_Subst(ilti, pPack, &rows, &qtty, flags, pGra, /*serial*/0));
				}
			}
		}
		else if(qtty > 0.0) {
			if(ilti->Flags & PPTFR_RECEIPT || oneof2(pPack->OprType, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT)) {
				MEMSZERO(ti);
				if(ilti->Flags & PPTFR_FORCESUPPL) {
					ti.Flags |= PPTFR_FORCESUPPL;
					ti.Suppl  = ilti->Suppl;
				}
				THROW(ti.Init(&pPack->Rec, 1, 1));
				THROW(ti.SetupGoods(ilti->GoodsID, 0));
				THROW(ti.SetupLot(0, 0, 0));
				SETFLAGBYSAMPLE(ti.Flags, PPTFR_COSTWOVAT,    ilti->Flags);
				SETFLAGBYSAMPLE(ti.Flags, PPTFR_PRICEWOTAXES, ilti->Flags);
				ti.UnitPerPack = ilti->UnitPerPack;
				ti.Quantity_   = qtty;
				ti.Cost        = ilti->Cost;
				ti.Price       = (adj_intr_price_val > 0.0) ? adj_intr_price_val : ilti->Price;
				if(ti.CurID)
					ti.CurPrice = ilti->CurPrice;
				ti.Discount    = 0.0;
				ti.QCert       = ilti->QCert;
				ti.Expiry      = ilti->Expiry;
				ti.LotTaxGrpID = ilti->InTaxGrpID;
				{
					uint i;
					IntArray temp_rows;
					ObjTagList tag_list;
					THROW(pPack->InsertRow(&ti, &temp_rows));
					pPack->SnL.AddNumber(&temp_rows, serial);
					if(ti.Flags & PPTFR_RECEIPT && flags & CILTIF_INHLOTTAGS) {
						const  LDATE sd = MAXDATE;
						ReceiptTbl::Rec tmpl_lot_rec;
						if(trfr->Rcpt.GetLastLot(ti.GoodsID, -labs(ti.LocID), sd, &tmpl_lot_rec) > 0) {
 							ObjTagList inh_tag_list;
							GetTagListByLot(tmpl_lot_rec.ID, 1, &inh_tag_list);
							const uint tc = inh_tag_list.GetCount();
							if(tc) {
								PPObjTag tag_obj;
								PPObjectTag tag_rec;
								for(i = 0; i < tc; i++) {
									const ObjTagItem * p_tag = inh_tag_list.GetItemByPos(i);
									if(p_tag && tag_obj.Fetch(p_tag->TagID, &tag_rec) > 0 && tag_rec.Flags & OTF_INHERITABLE)
										tag_list.PutItem(p_tag->TagID, p_tag);
								}
							}
						}
 					}
					for(i = 0; i < temp_rows.getCount(); i++) {
						const int row_idx = temp_rows.at(i);
						if(tag_list.GetCount()) {
							pPack->LTagL.Set(row_idx, &tag_list);
						}
						THROW_SL(rows.insert(&row_idx));
					}
					if(flags & CILTIF_SYNC) {
						if(_RowNSyncDiag(ilti, rows, flags)) {
							const uint ti_pos = (uint)rows.at(0);
							pPack->TI(ti_pos).RByBill = ilti->RByBill;
							full_sync = 1;
						}
					}
				}
				qtty = 0.0;
			}
			else {
				//
				// Если необходимо оприходовать некоторое количество товара без образования лота (возвраты, например),
				// то перебираем все последние лоты и заносим в них столько товара, чтобы остаток не превысил первоначальное количество.
				//
				const long f1 = (ilti->Flags & PPTFR_PRICEWOTAXES);
				// @v8.0.3 {
				if(sync_lot_id && trfr->Rcpt.Search(sync_lot_id, &lotr) > 0) {
					//
					// Синхронизированный лот пытаемся использовать в первую очередь
					//
					const long f2 = (lotr.Flags & LOTF_PRICEWOTAXES);
					if((f1 && f2) || (!f1 && !f2)) {
						THROW(pPack->BoundsByLot(lotr.ID, 0, -1, &rest, &q));
						SETMIN(q, qtty);
						if(q > 0.0) {
							MEMSZERO(ti);
							THROW(ti.Init(&pPack->Rec, 1, 1));
							THROW(ti.SetupGoods(ilti->GoodsID, 0));
							ti.Price = fabs(ilti->Price);
							THROW(ti.SetupLot(lotr.ID, &lotr, 0));
							ti.Quantity_ = q;
							THROW(pPack->InsertRow(&ti, &rows));
							qtty -= q;
						}
					}
					if(qtty == 0.0 && flags & CILTIF_SYNC && rows.getCount() == 1 && ilti->RByBill) {
						const uint ti_pos = (uint)rows.at(0);
						if(pPack->TI(ti_pos).LotID == sync_lot_id) { // @paranoic
							pPack->TI(ti_pos).RByBill = ilti->RByBill;
							full_sync = 1;
						}
					}
				}
				// } @v8.0.3
				LDATE  dt    = pPack->Rec.Dt;
				long   oprno = MAXLONG;
				while(qtty > 0.0 && trfr->Rcpt.EnumLastLots(ilti->GoodsID, pPack->Rec.LocID, &dt, &oprno, &lotr) > 0) {
					if(!by_serial || CmpSnrWithLotSnr(lotr.ID, serial)) {
						const long f2 = (lotr.Flags & LOTF_PRICEWOTAXES);
						if((f1 && f2) || (!f1 && !f2)) {
							THROW(pPack->BoundsByLot(lotr.ID, 0, -1, &rest, &q));
							SETMIN(q, qtty);
							if(q > 0.0) {
								MEMSZERO(ti);
								THROW(ti.Init(&pPack->Rec, 1, 1));
								THROW(ti.SetupGoods(ilti->GoodsID, 0));
								ti.Price = fabs(ilti->Price);
								THROW(ti.SetupLot(lotr.ID, &lotr, 0));
								ti.Quantity_ = q;
								THROW(pPack->InsertRow(&ti, &rows));
								qtty -= q;
							}
						}
					}
				}
			}
		}
		ilti->Rest = R6(qtty);
	}
	CATCHZOK
	delete p_excl_serial;
	if(pRows) {
		for(i = 0; i < rows.getCount(); i++)
			pRows->insert(&rows.at(i));
	}
	return (ok && full_sync) ? 100 : ok;
}
//
//
//
SLAPI ComplItem::ComplItem()
{
	THISZERO();
}
//
//
//
struct GRII {
	PPID   SrcID;
	double Qtty;   // Количество товара SrcID, которое может быть израсходовано для компенсации
	double Ratio;  // Отношение, в котором следует использовать товар SrcID для компенсации
};

SLAPI GRI::GRI(PPID destID) : SArray(sizeof(GRII))
	{ DestID = destID; }
PPID FASTCALL GRI::GetSrcID(uint i) const
	{ return ((GRII *)at(i))->SrcID; }
double FASTCALL GRI::GetQtty(uint i) const
	{ return ((GRII *)at(i))->Qtty; }
double FASTCALL GRI::GetRatio(uint i) const
	{ return ((GRII *)at(i))->Ratio; }

int SLAPI GRI::GetPosByGoods(PPID goodsID, uint * pPos) const
{
	uint   pos = 0;
	if(lsearch(&goodsID, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pPos, pos);
		return 1;
	}
	else
		return 0;
}

int SLAPI GRI::Add(PPID srcID, double qtty, double ratio)
{
	int    ok = 0;
	uint   pos = 0;
	if(lsearch(&srcID, &pos, CMPF_LONG)) {
		((GRII *)at(pos))->Qtty += qtty;
		ok = 2;
	}
	else {
		GRII item;
		item.SrcID = srcID;
		item.Qtty = qtty;
		item.Ratio = ratio;
		ok = insert(&item) ? 1 : PPSetErrorSLib();
	}
	return ok;
}

SLAPI GoodsReplacementArray::GoodsReplacementArray() : TSCollection <GRI> ()
{
}

const GRI * SLAPI GoodsReplacementArray::Search(PPID destID) const
{
	for(uint i = 0; i < getCount(); i++) {
		const GRI * p_item = at(i);
		if(p_item->DestID == destID)
			return p_item;
	}
	return 0;
}

GRI * SLAPI GoodsReplacementArray::SearchNC(PPID destID)
{
	for(uint i = 0; i < getCount(); i++) {
		GRI * p_item = at(i);
		if(p_item->DestID == destID)
			return p_item;
	}
	return 0;
}

int SLAPI GoodsReplacementArray::Add(PPID destID, PPID srcID, double qtty, double ratio)
{
	int    ok = 2;
	GRI * p_gri = SearchNC(destID);
	if(p_gri) {
		THROW(p_gri->Add(srcID, qtty, ratio));
	}
	else {
		THROW_MEM(p_gri = new GRI(destID));
		THROW(p_gri->Add(srcID, qtty, ratio));
		THROW_SL(insert(p_gri));
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI ILBillPacket::ILBillPacket() : PPBill()
{
	IlbFlags = 0;
	LocObj = 0;
}

SLAPI ILBillPacket::~ILBillPacket()
{
	destroy();
}

void SLAPI ILBillPacket::destroy()
{
	PPBill::BaseDestroy();
	IlbFlags = 0;
	LocObj  = 0;
	Lots.freeAll();
	Turns.freeAll();
	AdvList.Clear();
	OrderBillList.freeAll();
	InvList.freeAll();
	LTagL.Release(); // @v7.3.5
	BTagL.Destroy(); // @v7.3.5
}

int SLAPI ILBillPacket::SearchGoodsID(PPID goodsID, uint * pPos) const
{
	*pPos = 0;
	return Lots.lsearch(&goodsID, pPos, CMPF_LONG, offsetof(ILTI, GoodsID));
}

int SLAPI ILBillPacket::SearchRByBill(int rbb, uint * pPos) const
{
	int    ok = 0;
	if(rbb) {
		for(uint i = 0; !ok && i < Lots.getCount(); i++) {
			const ILTI & r_item = Lots.at(i);
			if(r_item.RByBill == rbb) {
				ASSIGN_PTR(pPos, i);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI ILBillPacket::Load(PPID billID, long flags, PPID cvtToOpID /*=0*/)
{
	int    ok = 1, rbybill, r;
	const  int free_amt = (flags & ILBPF_LOADAMTNOTLOTS) ? 0 : 1;
	PPID   cvt_to_opid = 0;
	PPOprKind cvt_op_rec;
	const  PPID cvt_op_typeid = cvtToOpID ? GetOpType(cvtToOpID, &cvt_op_rec) : 0;
	PPAccTurn at;
	PPTransferItem ti;
	destroy();
	PPObjBill * p_bobj = BillObj;
	PPBillPacket bpack;
	THROW(p_bobj->P_Tbl->Extract(billID, this) > 0);
	const PPID op_type_id = GetOpType(Rec.OpID);
	cvt_to_opid = (cvtToOpID && (cvt_op_typeid != PPOPT_GENERIC) && cvtToOpID != Rec.OpID &&
		oneof2(cvt_op_typeid, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT)) ? cvtToOpID : 0;
	if(cvt_to_opid) {
		// @v7.7.3 THROW(bpack.CreateBlank(cvtToOpID, Rec.LinkBillID, Rec.LocID, 0));
		// @v7.7.3
		// @v8.9.6 {
		PPID   dest_loc_id = Rec.LocID;
		PPID   dest_ar_id = 0;
		if(cvt_op_typeid == PPOPT_GOODSRECEIPT) {
			if(IsIntrExpndOp(Rec.OpID)) {
				dest_loc_id = PPObjLocation::ObjToWarehouse(Rec.Object);
				SETIFZ(dest_loc_id, Rec.LocID);
				if(Rec.Object2) {
					PPID    acs_id = 0;
					PPID    psn_id = ObjectToPerson(Rec.Object2, &acs_id);
					if(psn_id) {
						PPObjArticle ar_obj;
						PPObjAccSheet acs_obj;
						PPAccSheet acs_rec;
						if(acs_obj.Fetch(acs_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup == PPPRK_MAIN)
							ar_obj.P_Tbl->PersonToArticle(psn_id, cvt_op_rec.AccSheetID, &dest_ar_id);
					}
				}
			}
			// @v9.0.4 {
			else if(GetOpType(Rec.OpID) == PPOPT_GOODSEXPEND) {
				if(P_Freight && P_Freight->DlvrAddrID) {
					PPObjLocation loc_obj;
					LocationTbl::Rec loc_rec;
					if(loc_obj.Search(P_Freight->DlvrAddrID, &loc_rec) > 0 && loc_rec.Code[0]) {
						PPID   temp_loc_id = 0;
						if(loc_obj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, loc_rec.Code, &temp_loc_id, &loc_rec) > 0)
							dest_loc_id = temp_loc_id;
					}
				}
			}
			// } @v9.0.4
		}
		// } @v8.9.6
		{
			THROW(bpack.CreateBlank_WithoutCode(cvtToOpID, Rec.LinkBillID, dest_loc_id, 0)); // @v8.9.6 Rec.LocID-->dest_loc_id
			char   bill_code[64];
			STRNSCPY(bill_code, Rec.Code);
			STRNSCPY(bpack.Rec.Code, BillCore::GetCode(bill_code));
		}
		// } @v7.7.3
		bpack.Rec.ID         = Rec.ID;
		bpack.Rec.Dt         = Rec.Dt;
		bpack.Rec.BillNo     = Rec.BillNo;
		bpack.Rec.DueDate    = Rec.DueDate;
		bpack.Rec.StatusID   = Rec.StatusID;
		bpack.Rec.UserID     = Rec.UserID;
		bpack.Rec.MainOrgID  = Rec.MainOrgID;
		bpack.Rec.CurID      = Rec.CurID;
		bpack.Rec.CRate      = Rec.CRate;
		bpack.Rec.SCardID    = Rec.SCardID;
		// @v8.9.6 {
		if(dest_ar_id)
			bpack.Rec.Object = dest_ar_id;
		// } @v8.9.6
		SETFLAG(bpack.Rec.Flags, BILLF_FIXEDAMOUNTS, Rec.Flags & BILLF_FIXEDAMOUNTS);
		STRNSCPY(bpack.Rec.Memo, Rec.Memo);
		bpack.Amounts        = Amounts;
		bpack.Rec.Amount     = Rec.Amount;
		*(PPBill*)this = *(PPBill*)&bpack;
	}
	LocObj = PPObjLocation::WarehouseToObj(Rec.LocID);
	if(op_type_id == PPOPT_ACCTURN) {
		//
		// Список Amounts не должен содержать никаких сумм, кроме фиксированных
		// и ручных (см. ниже) (если не установлен флаг ILBPF_LOADAMTNOTLOTS)
		//
		if(!(Rec.Flags & BILLF_FIXEDAMOUNTS) && free_amt)
			Amounts.freeAll();
		for(rbybill = 0; (r = p_bobj->atobj->P_Tbl->EnumByBill(billID, &rbybill, &at)) > 0;) {
			at.Opr = Rec.OpID;
			memcpy(at.BillCode, Rec.Code, sizeof(at.BillCode));
			THROW_SL(Turns.insert(&at));
		}
		THROW(p_bobj->LoadAdvList(billID, Rec.OpID, &AdvList));
	}
	else if(free_amt) {
		//
		// Список Amounts не должен содержать никаких сумм, кроме фиксированных
		// и ручных (см. ниже) (если не установлен флаг ILBPF_LOADAMTNOTLOTS)
		//
		Amounts.freeAll();
		//
		int    row_idx = -1;
		if(op_type_id == PPOPT_INVENTORY) {
			InventoryTbl::Rec rec;
			for(SEnum en = p_bobj->GetInvT().Enum(billID); en.Next(&rec) > 0;) {
				THROW_SL(InvList.insert(&rec));
			}
		}
		if(oneof3(op_type_id, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT)) {
			if(p_bobj->P_CpTrfr) {
				CpTrfrExt ext;
				for(rbybill = 0; (r = p_bobj->P_CpTrfr->EnumItems(billID, &rbybill, &ti, &ext)) > 0;) {
					ILTI ilti(&ti);
					row_idx = Lots.getCount();
					THROW_SL(Lots.insert(&ilti));
					THROW(ClbL.AddNumber(row_idx, ext.Clb));
					THROW(SnL.AddNumber(row_idx, ext.PartNo));
				}
			}
		}
		else {
			SString clb;
			for(rbybill = 0; (r = p_bobj->trfr->EnumItems(billID, &rbybill, &ti)) > 0;) {
				ILTI   ilti;
				if(cvt_to_opid) {
					THROW(bpack.InsertRow(&ti, 0));
					ti = bpack.TI(bpack.GetTCount() - 1);
					ti.SetSignFlags(cvt_to_opid, 1);
					ti.SetupSign(cvt_to_opid);
					if(op_type_id == PPOPT_GOODSRECEIPT && cvt_op_typeid == PPOPT_GOODSEXPEND) {
						//
						// Если приход преобразуется в расход, то цену реализации конвертированного
						// документа приравниваем цене поступления оригинального документа //
						//
						ti.Price = ti.Cost;
						ti.Discount = 0.0;
					}
					// @v9.0.8 {
					else if(op_type_id == PPOPT_GOODSEXPEND && cvt_op_typeid == PPOPT_GOODSRECEIPT) {
						ti.Cost = ti.NetPrice();
						ti.Price = ti.Cost;
						ti.Discount = 0.0;
					}
					// } @v9.0.8
					//
					// В данном случае требуется корректно установить флаги, так как поменялся знак операции
					//
					if((op_type_id == PPOPT_GOODSRECEIPT && cvt_op_typeid == PPOPT_GOODSEXPEND) || (op_type_id == PPOPT_GOODSEXPEND && cvt_op_typeid == PPOPT_GOODSRECEIPT))
						ti.Init(&Rec, 0, 1);
				}
				ilti.Init__(&ti);
				// @v7.6.1 {
				if(CConfig.Flags2 & CCFLG2_SYNCLOT && !cvt_to_opid) {
					TransferTbl::Rec mirr_rec;
					ilti.LotSyncID = ti.LotID;
					ilti.LotMirrID = (p_bobj->trfr->SearchByBill(ti.BillID, 1, ti.RByBill, &mirr_rec) > 0) ? mirr_rec.LotID : 0;
				}
				// } @v7.6.1
				row_idx = Lots.getCount();
				THROW_SL(Lots.insert(&ilti));
				// @v7.3.5 {
				{
					ObjTagList tag_list;
					p_bobj->GetTagListByLot(ti.LotID, 1, &tag_list);
					LTagL.Set(row_idx, tag_list.GetCount() ? &tag_list : 0);
				}
				// } @v7.3.5
				if(p_bobj->GetClbNumberByLot(ti.LotID, 0, clb) > 0) {
					THROW(ClbL.AddNumber(row_idx, clb));
				}
				if(p_bobj->GetSerialNumberByLot(ti.LotID, clb, 0) > 0) {
					THROW(SnL.AddNumber(row_idx, clb));
				}
			}
			THROW(r);
			//
			// Загружаем идентификаторы документов заказов, которые "закрываются" данным документом
			//
			if(!cvt_to_opid)
				p_bobj->P_Tbl->GetListOfOrdersByLading(billID, &OrderBillList);
			THROW(bpack.InitAmounts((int)0));
			if(Rec.Flags & BILLF_FIXEDAMOUNTS) {
				if(cvt_to_opid) {
					THROW(bpack.SumAmounts(&Amounts));
				}
				else
					THROW(p_bobj->P_Tbl->GetAmountList(billID, &Amounts));
			}
			if(cvt_to_opid)
				Rec.Amount = bpack.GetAmount(0);
		}
	}
	if(!(Rec.Flags & BILLF_FIXEDAMOUNTS) && free_amt) {
		AmtList temp_list;
		PPObjAmountType amt_obj;
		PPAmountType amt_type;
		p_bobj->P_Tbl->GetAmountList(billID, &temp_list);
		for(uint i = 0; i < temp_list.getCount(); i++) {
			const AmtEntry & r_amt_entry = temp_list.at(i);
			if(amt_obj.Fetch(r_amt_entry.AmtTypeID, &amt_type) > 0 && amt_type.Flags & PPAmountType::fManual)
				Amounts.Put(&r_amt_entry, 1, 1);
		}
	}
	// @v7.3.5 {
	BTagL.Destroy();
	THROW(p_bobj->GetTagList(billID, &BTagL));
	// } @v7.3.5
	CATCHZOK
	return ok;
}

static SString & __Debug_TraceLotSync(const ILBillPacket & rIPack, const PPBillPacket * pPack, SString & rBuf)
{
	ILTI * p_ilti = 0;
	LongArray pos_list;
	(rBuf = 0).CatChar('{').Cat(rIPack.Rec.ID).CatChar('/');
	rBuf.Cat(pPack ? pPack->Rec.ID : (long)-1);
	for(uint i = 0; rIPack.Lots.enumItems(&i, (void**)&p_ilti);) {
		pos_list.clear();
		if(pPack) {
			for(uint j = 0; j < pPack->GetTCount(); j++) {
				const PPTransferItem & r_ti = pPack->TI(j);
				if(r_ti.SrcIltiPos == (int)i)
					pos_list.add(j);
			}
		}
		const uint plc = pos_list.getCount();
        rBuf.Space().CatChar('[');
        rBuf.Cat(p_ilti->LotSyncID);
        if(plc) {
        	rBuf.CatChar('/');
            for(uint j = 0; j < plc; j++) {
            	const PPTransferItem & r_ti = pPack->TI(pos_list.get(j));
            	if(j)
					rBuf.Space();
                rBuf.Cat(r_ti.LotID);
            }
        }
        else {
        	rBuf.CatChar('$');
        }
		rBuf.CatChar(']');
	}
	if(pPack) {
        rBuf.Space().CatChar('(');
		for(uint j = 0; j < pPack->GetTCount(); j++) {
			const PPTransferItem & r_ti = pPack->TI(j);
			if(j)
				rBuf.Space();
			rBuf.Cat(r_ti.SrcIltiPos);
		}
		rBuf.CatChar(')');
	}
	return rBuf.CatChar('}');
}

int SLAPI ILBillPacket::ConvertToBillPacket(PPBillPacket & rPack, int * pWarnLevel, ObjTransmContext * pCtx, int use_ta)
{
	assert(rPack.Rec.ID >= 0);
	const int trace_sync_lot = BIN(DS.CheckExtFlag(ECF_TRACESYNCLOT));
	const int _update = BIN(rPack.Rec.ID > 0);

	int    ok = 1, warn = 0;
	SString msg_buf, fmt_buf, bill_descr_buf, temp_buf;
	SString goods_name;
	SString org_serial;
	SString org_clb_number, clb_number;
	uint   i;
	const  long   fmask = BILLF_TOTALDISCOUNT|BILLF_WHITELABEL|BILLF_FIXEDAMOUNTS|BILLF_SHIPPED/*| BILLF_RMVEXCISE*/;
	const  long   fmask2 = BILLF2_BHT|BILLF2_TSESSPAYM|BILLF2_DECLINED|BILLF2_RECADV_ACCP|BILLF2_RECADV_DECL|BILLF2_EDIAR_AGR|BILLF2_EDIAR_DISAGR; // @v9.0.1
	PPID   op_type_id = 0;
	ILTI * p_ilti;
	PPAccTurn * p_at, at;
	PPObjBill * p_bobj = BillObj;
	//AccountCore & acc  = p_bobj->atobj->tbl->AccT;
	PPObjAccount acc_obj;
	if(!_update) {
		THROW(rPack.CreateBlank(Rec.OpID, Rec.LinkBillID, Rec.LocID, use_ta));
	}
	else if(pCtx->P_ThisDbDivPack && rPack.Rec.OpID) {
		//
		// @v8.5.11
		// При изменении синхронизированного документа существует следующая проблема:
		// если документ был ранее принят как межскладской приход, посредством конвертации внутренней передачи,
		// то пришедшая настоящая внутренняя передача должна обратно отконвертировать документ.
		//
		// Просто, через колено, меняем вид операции. На тестах работает, но возможны нюансы.
		//
		if(rPack.Rec.OpID == pCtx->P_ThisDbDivPack->Rec.IntrRcptOpr && IsIntrOp(Rec.OpID) == INTREXPND) {
			rPack.Rec.OpID = Rec.OpID;
		}
		else {
			// Обратное преобразование не допускаем ни при каких обстоятельствах
			THROW_PP(IsIntrOp(rPack.Rec.OpID) != INTREXPND || Rec.OpID != pCtx->P_ThisDbDivPack->Rec.IntrRcptOpr, PPERR_CANTACCEPTBILLMOD_INTREXP);
		}
	}
	const int is_intr_expnd = IsIntrExpndOp(rPack.Rec.OpID); // @v8.9.0
	rPack.Rec.Object  = Rec.Object;
	rPack.Rec.Object2 = Rec.Object2;
	rPack.Rec.LocID   = Rec.LocID;
	rPack.Rec.Dt      = Rec.Dt;
	rPack.Rec.CurID   = Rec.CurID;
	rPack.Rec.Amount  = BR2(Rec.Amount);
	rPack.Rec.Flags  |= (Rec.Flags & fmask);
	rPack.Rec.Flags2 |= (Rec.Flags2 & fmask2); // @v9.0.1
	rPack.Rec.LastRByBill = Rec.LastRByBill; // @v8.0.3
	rPack.Rec.EdiOp   = Rec.EdiOp; // @v9.0.1
	rPack.SetFreight(P_Freight);
	//
	// Calling of function CreateBlank above can automaticaly set flag BILLF_WHITELABEL.
	// We must set this flag in accordance with source packet.
	//
	SETFLAG(rPack.Rec.Flags, BILLF_WHITELABEL, Rec.Flags & BILLF_WHITELABEL);
	STRNSCPY(rPack.Rec.Code, Rec.Code);
	STRNSCPY(rPack.Rec.Memo, Rec.Memo);
	rPack.BTagL = BTagL; // @v7.3.5
	rPack.Ext = Ext;
	op_type_id = GetOpType(rPack.Rec.OpID);
	rPack.Amounts.Put(&Amounts, 0, 1); // Теперь суммы (ручные) конвертируем для всех типов операций
	PPObjBill::MakeCodeString(&rPack.Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_descr_buf); // @v8.4.10
	if(oneof2(op_type_id, PPOPT_ACCTURN, PPOPT_PAYMENT)) {
		if(op_type_id == PPOPT_PAYMENT || CheckOpFlags(rPack.Rec.OpID, OPKF_EXTACCTURN)) {
			rPack.Amounts.Put(PPAMT_MAIN, 0L, rPack.GetAmount(), 0, 1);
			THROW(p_bobj->FillTurnList(&rPack));
		}
		else {
			for(i = 0; Turns.enumItems(&i, (void**)&p_at);) {
				rPack.CreateAccTurn(&at);
				at.DbtID = p_at->DbtID;
				at.CrdID = p_at->CrdID;
				THROW(acc_obj.InitAccSheetForAcctID(&at.DbtID, &at.DbtSheet));
				THROW(acc_obj.InitAccSheetForAcctID(&at.CrdID, &at.CrdSheet));
				at.Amount = p_at->Amount;
				THROW_SL(rPack.Turns.insert(&at));
			}
		}
		if(P_AdvRep) {
			THROW_MEM(rPack.P_AdvRep = new PPAdvanceRep);
			*rPack.P_AdvRep = *P_AdvRep;
			rPack.Rec.Flags |= BILLF_ADVANCEREP;
		}
		for(i = 0; i < AdvList.GetCount(); i++)
			rPack.AdvList.Add(&AdvList.Get(i));
	}
	else if(op_type_id == PPOPT_INVENTORY) {
		THROW_PP(!_update, PPERR_CANTACCEPTBILLMOD_INV); // @v8.0.3
		{
			InventoryCore & r_line_tbl = p_bobj->GetInvT();
			PPID   id = rPack.Rec.ID;
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(p_bobj->P_Tbl->Edit(&id, &rPack, 0));
			rPack.Rec.ID = id;
			for(i = 0; i < InvList.getCount(); i++) {
				long   oprno = 0;
				InventoryTbl::Rec i_rec = InvList.at(i);
				i_rec.BillID = id;
				i_rec.Flags &= ~INVENTF_WRITEDOFF; // Признак списания в разделе-отправителе в получателе должен убираться.
				THROW(r_line_tbl.Set(id, &oprno, &i_rec, 0));
			}
			THROW(p_bobj->P_Tbl->Edit(&id, &rPack, 0));
			THROW(tra.Commit());
		}
	}
	else {
		const int ccflg_synclot = BIN(CConfig.Flags2 & CCFLG2_SYNCLOT);
		ObjTagList lot_tag_list;
		IntArray rows;
		const long ciltif_const_ = CILTIF_USESYNCLOT|CILTIF_OPTMZLOTS|CILTIF_SUBSTSERIAL|CILTIF_ALLOWZPRICE|CILTIF_SYNC;
		const long ciltif = _update ? (ciltif_const_|CILTIF_MOD) : ciltif_const_;
		if(trace_sync_lot) {
			__Debug_TraceLotSync(*this, &rPack, msg_buf);
			PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		}
		if(ccflg_synclot) {
			THROW_PP(!_update || rPack.Rec.Flags2 & BILLF2_FULLSYNC, PPERR_CANTACCEPTBILLMOD_NFS); // @v8.0.3
			rPack.Rec.Flags2 |= BILLF2_FULLSYNC;
			// @v8.4.10 {
			{
				// @log PPTXT_SYNCLOT_TRYBILL        "Попытка приема документа [%s] с синхронизацией по лотам"
                msg_buf.Printf(PPLoadTextS(PPTXT_SYNCLOT_TRYBILL, fmt_buf), (const char *)bill_descr_buf);
                PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
			}
			// } @v8.4.10
		}
		else {
			THROW_PP(!_update, PPERR_CANTACCEPTBILLMOD_LOTSYNC); // @v8.0.3
		}
		if(_update) {
			//
			// Удаляем из модифицируемого документа строки, не имеющие соответствия в this
			// и (или) не имеющие признака PPTFR_LOTSYNC.
			//
			i = rPack.GetTCount();
			if(i) do {
				const PPTransferItem & r_ti = rPack.ConstTI(--i);
				uint   ipos = 0;
				if(!(r_ti.Flags & PPTFR_LOTSYNC)) {
					// @v8.4.10 {
					{
						// @log PPTXT_SYNCLOT_UPDROWNSYNCRMV "Строка изменяемого документа [%s] не синхронизирована и должна быть заменена"
                        (temp_buf = bill_descr_buf).CatDiv(';', 2).Cat(r_ti.RByBill).CatDiv('-').Cat(r_ti.LotID);
                        msg_buf.Printf(PPLoadTextS(PPTXT_SYNCLOT_UPDROWNSYNCRMV, fmt_buf), (const char *)temp_buf);
                        PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
					}
					// } @v8.4.10
					rPack.RemoveRow(i);
				}
				else if(!SearchRByBill(r_ti.RByBill, &ipos)) {
					rPack.RemoveRow(i);
				}
			} while(i);
		}
		for(i = 0; Lots.enumItems(&i, (void**)&p_ilti);) {
			uint   j;
			rows.clear();
			ClbL.GetNumber(i-1, &org_clb_number);
			SnL.GetNumber(i-1, &org_serial);
			const ObjTagList * p_org_lot_tag_list = LTagL.Get(i-1);
			//
			// Трансформируем идентификаторы лотов из чужого раздела в соответствующие нашему разделу
			//
			const PPID preserve_frgn_lot_id = p_ilti->LotSyncID;
			const PPID preserve_frgn_lot_mirr_id = p_ilti->LotMirrID;
			if(ccflg_synclot) {
				int    rl;
				PPID   sync_primary_lot_id = 0;
				PPID   sync_primary_lot_mirr_id = 0;
				if(p_ilti->LotSyncID) {
					THROW(rl = pCtx->ResolveDependedNonObject(PPOBJ_LOT, p_ilti->LotSyncID, &sync_primary_lot_id));
					p_ilti->LotSyncID = sync_primary_lot_id;
					if(rl < 0 || !sync_primary_lot_id) {
						if(!(p_ilti->Flags & PPTFR_RECEIPT)) {
							// @v8.4.10 {
							{
								// @log PPTXT_SYNCLOT_NSYNCMAIN      "Не удалось разрешить синхронизацию лота [%s]"
								GetGoodsName(p_ilti->GoodsID, goods_name);
								(temp_buf = bill_descr_buf).CatDiv(';', 2).Cat(p_ilti->RByBill).CatDiv('-').Cat(preserve_frgn_lot_id).CatDiv('-').Cat(goods_name);
								msg_buf.Printf(PPLoadTextS(PPTXT_SYNCLOT_NSYNCMAIN, fmt_buf), (const char *)temp_buf);
								PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
							}
							// } @v8.4.10
							rPack.Rec.Flags2 &= ~BILLF2_FULLSYNC;
						}
					}
				}
				if(p_ilti->LotMirrID) {
					THROW(rl = pCtx->ResolveDependedNonObject(PPOBJ_LOT, p_ilti->LotMirrID, &sync_primary_lot_mirr_id));
					if(rl < 0 || !sync_primary_lot_mirr_id) {
						// @v8.4.10 {
						{
							// @log PPTXT_SYNCLOT_NSYNCMIRR      "Не удалось разрешить синхронизацию зеркального лота [%s]"
							GetGoodsName(p_ilti->GoodsID, goods_name);
							(temp_buf = bill_descr_buf).CatDiv(';', 2).Cat(p_ilti->RByBill).CatDiv('-').Cat(preserve_frgn_lot_mirr_id).
								CatDiv('-').Cat(goods_name);
							msg_buf.Printf(PPLoadTextS(PPTXT_SYNCLOT_NSYNCMIRR, fmt_buf), (const char *)temp_buf);
							PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
						}
						// } @v8.4.10
					}
					p_ilti->LotMirrID = sync_primary_lot_mirr_id;
				}
				THROW_PP(!_update || rPack.Rec.Flags2 & BILLF2_FULLSYNC, PPERR_CANTACCEPTBILLMOD_NFSLOT); // @v8.0.3
			}
			else {
				p_ilti->LotSyncID = 0;
				p_ilti->LotMirrID = 0;
			}
			{
				int    do_add = 1;
				if(_update) {
					uint   tipos = 0;
					if(rPack.SearchTI(p_ilti->RByBill, &tipos)) {
						PPTransferItem & r_ti = rPack.TI(tipos);
						r_ti.SrcIltiPos = i; // @v8.5.7 Сохраним соответствие номера строки в this со строками в rPack
						if(r_ti.Flags & PPTFR_LOTSYNC) {
							//
							long   slfl = 0;
							THROW_PP(labs(p_ilti->GoodsID) == r_ti.GoodsID, PPERR_CANTACCEPTBILLMOD_GOODS);
							THROW(r_ti.SetupGoods(p_ilti->GoodsID, TISG_SETPWOTF) > 0);
							if(op_type_id == PPOPT_GOODSREVAL) {
								THROW(r_ti.SetupLot(p_ilti->LotSyncID, 0, slfl));
								r_ti.Cost = p_ilti->Cost;
								r_ti.Price = p_ilti->Price;
							}
							else if(op_type_id == PPOPT_CORRECTION) {
								THROW(r_ti.SetupLot(p_ilti->LotSyncID, 0, slfl));
								r_ti.Cost = p_ilti->Cost;
								r_ti.Price = p_ilti->Price;
								r_ti.Quantity_ = p_ilti->Quantity;
							}
							else {
								r_ti.Cost = p_ilti->Cost;
								r_ti.Price = p_ilti->Price;
								r_ti.Discount = 0.0;
								if(r_ti.Flags & PPTFR_RECEIPT) {
									r_ti.QCert = p_ilti->QCert;
									r_ti.UnitPerPack = p_ilti->UnitPerPack;
									r_ti.Expiry = p_ilti->Expiry;

									THROW(rPack.ClbL.AddNumber(tipos, org_clb_number));
									THROW(rPack.SnL.AddNumber(tipos, org_serial));
									THROW(rPack.LTagL.Set(tipos, p_org_lot_tag_list));
									slfl |= (TISL_IGNCOST|TISL_IGNPRICE|TISL_IGNPACK|TISL_IGNQCERT|TISL_IGNEXPIRY);
								}
								else {
									slfl |= TISL_ADJPRICE;
								}
								THROW(r_ti.SetupLot(p_ilti->LotSyncID, 0, slfl));
								r_ti.Quantity_ = p_ilti->Quantity;
							}
							do_add = 0;
						}
						else {
							rPack.RemoveRow(tipos);
						}
					}
				}
				if(do_add) {
					const int rconv = p_bobj->ConvertILTI(p_ilti, &rPack, &rows, ciltif, org_serial);
					p_ilti->LotSyncID = preserve_frgn_lot_id;
					p_ilti->LotMirrID = preserve_frgn_lot_mirr_id;
					THROW(rconv);
					if(rconv != 100)
						rPack.Rec.Flags2 &= ~BILLF2_FULLSYNC;
					for(j = 0; j < rows.getCount(); j++) {
						int    rj = rows.at(j);
						PPTransferItem & r_ti = rPack.TI(rj);
						r_ti.SrcIltiPos = i; // @v7.6.1 Сохраним соответствие номера строки в this со строками в rPack
						/* @debug
						{
							(msg_buf = 0).CatEq("r_ti.SrcIltiPos", (long)r_ti.SrcIltiPos);
							__LogDebugMessage(pCtx, msg_buf);
						}
						*/
						const  PPID lot_id = r_ti.LotID;
						// @v8.0.3 {
						if(rconv == 100) {
							r_ti.Flags |= PPTFR_LOTSYNC;
						}
						r_ti.TFlags |= PPTransferItem::tfForceNew;
						// } @v8.0.3
						if(p_bobj->GetClbNumberByLot(lot_id, 0, clb_number) > 0) {
							THROW(rPack.ClbL.AddNumber(rj, clb_number));
						}
						else {
							THROW(rPack.ClbL.AddNumber(rj, org_clb_number));
						}
						// @v8.9.0 {
						if(is_intr_expnd) {
							SString local_serial;
							if(p_bobj->GetSerialNumberByLot(lot_id, local_serial, 0) > 0) {
								THROW(rPack.SnL.AddNumber(rj, local_serial));
							}
						}
						// } @v8.9.0
						if(p_bobj->GetTagListByLot(lot_id, 1 /* skip reserved */, &lot_tag_list) > 0) {
							THROW(rPack.LTagL.Set(rj, lot_tag_list.GetCount() ? &lot_tag_list : 0));
						}
						else {
							THROW(rPack.LTagL.Set(rj, p_org_lot_tag_list));
						}
					}
				}
			}
			if(warn < 2 && R6(p_ilti->Rest) != 0.0)
				warn = 2;
		}
		if(Rec.Flags & BILLF_RMVEXCISE) {
			long   sav_cconf_flags = DS.SetLCfgFlags(CConfig.Flags | CCFLG_TGGLEXCSNPRICE);
			rPack.Rec.Flags |= (BILLF_TGGLEXCSNPRICE | BILLF_TOTALDISCOUNT);
			rPack.SetTotalDiscount(0.0, 0, 1);
			DS.SetLCfgFlags(sav_cconf_flags);
		}
		if(Rec.Flags & BILLF_FIXEDAMOUNTS) {
			AmtEntry * p_ae = 0;
			for(i = 0; Amounts.enumItems(&i, (void**)&p_ae);)
				if(p_ae->AmtTypeID != PPAMT_PAYMENT)
					rPack.Amounts.Put(p_ae, 1, 1);
		}
		THROW(rPack.InitAmounts(0));
		rPack.Pays.copy(Pays);
		// @v8.0.3 {
		if(_update) {
			for(i = 0; i < rPack.Pays.getCount(); i++) {
				PayPlanTbl::Rec & r_paym_rec = rPack.Pays.at(i);
				r_paym_rec.BillID = rPack.Rec.ID;
			}
		}
		// } @v8.0.3
		if(!(pCtx->Cfg.Flags & DBDXF_DONTCVTTOTALDIS)) {
			if(Rec.Flags & BILLF_TOTALDISCOUNT) {
				AmtEntry mandis(PPAMT_MANDIS, 0, rPack.Amounts.Get(PPAMT_DISCOUNT, 0));
				rPack.Amounts.Put(&mandis, 1, 1);
			}
		}
		THROW(p_bobj->FillTurnList(&rPack));
		if(trace_sync_lot) {
			__Debug_TraceLotSync(*this, &rPack, msg_buf);
			PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		}
		if(warn < 1 && R6(rPack.GetAmount() - BR2(Rec.Amount)) != 0)
			warn = 1;
	}
	CATCHZOK
	ASSIGN_PTR(pWarnLevel, warn);
	return ok;
}

int SLAPI ILBillPacket::SerializeLots(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir < 0)
		Lots.freeAll();
	int32  c = Lots.getCount(); // @persistent
	THROW_SL(pSCtx->Serialize(dir, c, rBuf));
	for(int32 i = 0; i < c; i++) {
		ILTI item;
		if(dir > 0)
			item = Lots.at(i);
		THROW_SL(pSCtx->Serialize(dir, item.BillID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.GoodsID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.LotSyncID, rBuf)); // @v7.6.1
		THROW_SL(pSCtx->Serialize(dir, item.LotMirrID, rBuf)); // @v7.6.1
		THROW_SL(pSCtx->Serialize(dir, item.UnitPerPack, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Quantity, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Rest, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Cost, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Price, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.CurPrice, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.QuotPrice, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Flags, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Suppl, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.QCert, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Expiry, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.InTaxGrpID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.RByBill, rBuf)); // @v8.0.3
		THROW_SL(pSCtx->SerializeBlock(dir, sizeof(item.Reserve), item.Reserve, rBuf, 1)); // @v8.0.3
		if(dir < 0)
			THROW_SL(Lots.insert(&item));
	}
	CATCHZOK
	return ok;
}
//
//
//
PP_CREATE_TEMP_FILE_PROC(CreateTempDeficitTable, TempDeficit);

SLAPI BillTransmDeficit::BillTransmDeficit()
{
	// @v8.1.5 Period.SetZero();
	DiffGoodsBySuppl = 1;
	Tbl = CreateTempDeficitTable();
	BObj = BillObj;
}

SLAPI BillTransmDeficit::~BillTransmDeficit()
{
	delete Tbl;
	DBRemoveTempFiles();
}

BillTransmDeficit::LocPeriod * BillTransmDeficit::GetLocPeriod(PPID locID)
{
	BillTransmDeficit::LocPeriod * p_result = 0;
	uint   pos = 0;
	if(LocPeriodList_.lsearch(&locID, &pos, CMPF_LONG)) {
		p_result = &LocPeriodList_.at(pos);
	}
	else {
		LocPeriod new_item;
		new_item.LocID = locID;
		new_item.P.SetZero();
		pos = LocPeriodList_.getCount();
		if(LocPeriodList_.insert(&new_item))
			p_result = &LocPeriodList_.at(pos);
		else
			PPSetErrorSLib();
	}
	return p_result;
}

int SLAPI BillTransmDeficit::Search(PPID locID, PPID goodsID, PPID supplID, TempDeficitTbl::Rec * pRec)
{
	TempDeficitTbl::Key0 k;
	k.Location = locID;
	k.GoodsID = goodsID;
	k.SupplID = supplID;
	return SearchByKey(Tbl, 0, &k, pRec);
}

int SLAPI BillTransmDeficit::_CompleteGoodsRest(PPID locID, PPID goodsID, SArray * pRecList, uint startPos, double supplQtty)
{
	int    ok = 1;
	const  int round_prec = GObj.CheckFlag(goodsID, GF_INTVAL) ? 0 : 6;
	const  LocPeriod * p_lc = GetLocPeriod(locID);
	int    zero_deficit = 0;
	double rest = 0.0, partitial_rest = 0.0;
	TempDeficitTbl::Rec * p_rec;
	DateRange lot_period;
	THROW(p_lc);
	lot_period.Set(ZERODATE, /*Period.low*/p_lc->P.low);
	THROW(BillObj->trfr->GetAvailableGoodsRest(goodsID, locID, lot_period, 0.0, &rest));
	{
		for(uint i = startPos; pRecList->enumItems(&i, (void**)&p_rec);) {
			if(p_rec->SupplID == 0) {
				p_rec->Rest = rest;
				if(p_rec->Rest >= p_rec->Req)
					zero_deficit = 1;
			}
			else if(supplQtty != 0.0)
				if(i < pRecList->getCount()) {
					p_rec->Rest = round(rest * p_rec->Req / supplQtty, round_prec);
					partitial_rest += p_rec->Rest;
				}
				else
					p_rec->Rest = round(rest - partitial_rest, round_prec);
			else
				p_rec->Rest = 0.0;
		}
	}
	{
		const int32 has_deficit = zero_deficit ? 0 : 1;
		for(uint i = startPos; pRecList->enumItems(&i, (void**)&p_rec);)
			p_rec->HasDeficit = has_deficit;
	}
	CATCHZOK
	return ok;
}

int SLAPI BillTransmDeficit::CompleteGoodsRest()
{
	int    ok = 1;
	double suppl_qtty = 0.0;
	TempDeficitTbl::Rec rec, prev_rec, * p_rec;
	TempDeficitTbl::Key0 k;
	SArray rec_list(sizeof(rec));
	uint   start_pos = 0;
	{
		MEMSZERO(k);
		MEMSZERO(prev_rec);
		int    prev_rec_inited = 0;
		BExtQuery q(Tbl, 0);
		q.selectAll();
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
			Tbl->copyBufTo(&rec);
			if(prev_rec_inited && (prev_rec.Location != rec.Location || prev_rec.GoodsID != rec.GoodsID)) {
				THROW(_CompleteGoodsRest(prev_rec.Location, prev_rec.GoodsID, &rec_list, start_pos, suppl_qtty));
				suppl_qtty = 0.0;
				start_pos = rec_list.getCount();
			}
			if(rec.SupplID)
				suppl_qtty += rec.Req;
			THROW_SL(rec_list.insert(&rec));
			prev_rec = rec;
			prev_rec_inited = 1;
		}
		if(prev_rec_inited && start_pos < rec_list.getCount())
			THROW(_CompleteGoodsRest(prev_rec.Location, prev_rec.GoodsID, &rec_list, start_pos, suppl_qtty));
	}
	for(uint i = 0; rec_list.enumItems(&i, (void**)&p_rec);) {
		THROW(Search(p_rec->Location, p_rec->GoodsID, p_rec->SupplID, 0) > 0);
		THROW_DB(Tbl->updateRecBuf(p_rec));
	}
	CATCHZOK
	return ok;
}

int SLAPI BillTransmDeficit::GetGoodsDeficitList(PPIDArray * pList)
{
	int    ok = -1;
	if(pList) {
		TempDeficitTbl::Key0 k;
		BExtQuery q(Tbl, 0);
		q.selectAll().where(Tbl->SupplID == 0L && Tbl->HasDeficit > 0L);
		MEMSZERO(k);
		for(q.initIteration(0, &k, spFirst); q.nextIteration() > 0;) {
			if(Tbl->data.Req > Tbl->data.Rest) {
				pList->addUnique(Tbl->data.GoodsID);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI BillTransmDeficit::PrintTotalDeficit(ObjTransmContext * pCtx)
{
	TempDeficitTbl::Key0 k;
	BExtQuery q(Tbl, 0);
	q.selectAll().where(Tbl->SupplID == 0L && Tbl->HasDeficit > 0L);
	MEMSZERO(k);
	SString fmt_buf, msg_buf, goods_name;
	long   c = 0; // counter
	for(q.initIteration(0, &k, spFirst); q.nextIteration() > 0; c++) {
		if(!c) {
			pCtx->OutReceivingMsg(PPLoadTextS(PPTXT_BTP_TOTAL, fmt_buf));
			PPLoadText(PPTXT_BDR_LINE, fmt_buf = 0); // загруженное значение fmt_buf далее будет использоваться в итерациях
		}
		double deficit = Tbl->data.Req - Tbl->data.Rest;
		if(deficit > 0.0 && fmt_buf.NotEmpty()) {
			GetGoodsName(Tbl->data.GoodsID, goods_name);
			msg_buf.Printf(fmt_buf, (const char *)goods_name, deficit, Tbl->data.ReqCost, Tbl->data.ReqPrice);
			pCtx->OutReceivingMsg(msg_buf);
		}
	}
	return 1;
}

int SLAPI BillTransmDeficit::CalcReqSalesTax(ILTI * pIlti, LDATE dt, PPID opID, double * pSalesTax)
{
	double stax = 0.0;
	if(pIlti->Flags & PPTFR_RMVEXCISE) {
		PPGoodsTaxEntry gtx;
		GObj.FetchTax(labs(pIlti->GoodsID), dt, opID, &gtx);
		if(gtx.SalesTax > 0) {
			double st_rate = fdiv100i(gtx.SalesTax); // @divtax
			double net_price_rate = pIlti->Price * st_rate;
			if(CConfig.Flags & CCFLG_PRICEWOEXCISE)
				stax = net_price_rate / (100.0 + st_rate);
			else
				stax = fdiv100r(-net_price_rate);
		}
	}
	ASSIGN_PTR(pSalesTax, stax);
	return 1;
}

int SLAPI BillTransmDeficit::AddRec(ILTI * pIlti, const char * pClbNumber,
	BillTbl::Rec * pBillRec, PPID supplID, double qtty)
{
	TempDeficitTbl::Rec tdt_rec;
	MEMSZERO(tdt_rec);
	tdt_rec.Location = pBillRec->LocID;
	tdt_rec.GoodsID  = pIlti->GoodsID;
	tdt_rec.SupplID  = supplID;
	tdt_rec.Rest     = 0;
	tdt_rec.Req      = qtty;
	tdt_rec.ReqCost  = pIlti->Cost;
	tdt_rec.ReqPrice = pIlti->Price;
	tdt_rec.UnitPerPack = pIlti->UnitPerPack;
	CalcReqSalesTax(pIlti, pBillRec->Dt, pBillRec->OpID, &tdt_rec.ReqSalesTax);
	if(pClbNumber && *pClbNumber)
		STRNSCPY(tdt_rec.Clb, pClbNumber);
	return Tbl->insertRecBuf(&tdt_rec) ? 1 : PPSetErrorDB();
}

int SLAPI BillTransmDeficit::UpdateRec(TempDeficitTbl::Rec * pTdtr,
	ILTI * pIlti, const char * pClbNumber, BillTbl::Rec * pBillRec, double qtty)
{
	double sum_qtty = R6(pTdtr->Req + qtty);
	if(sum_qtty != 0.0) {
		double stax = 0.0;
		CalcReqSalesTax(pIlti, pBillRec->Dt, pBillRec->OpID, &stax);
		pTdtr->ReqCost  = (pIlti->Cost * qtty + pTdtr->ReqCost * pTdtr->Req) / sum_qtty;
		pTdtr->ReqPrice = (pIlti->Price * qtty + pTdtr->ReqPrice * pTdtr->Req) / sum_qtty;
		pTdtr->ReqSalesTax = (stax * qtty + pTdtr->ReqSalesTax * pTdtr->Req) / sum_qtty;
	}
	pTdtr->Req += qtty;
	SETIFZ(pTdtr->UnitPerPack, pIlti->UnitPerPack);
	if(pClbNumber && *pClbNumber && pTdtr->Clb[0] == 0)
		STRNSCPY(pTdtr->Clb, pClbNumber);
	return Tbl->updateRecBuf(pTdtr) ? 1 : PPSetErrorDB();
}

int SLAPI BillTransmDeficit::AddItem(ILTI * pIlti, const char * pClbNumber, BillTbl::Rec * pBillRec, int skipped)
{
	int    ok = 1;
	if(pIlti->Quantity < 0.0) {
		TempDeficitTbl::Rec tdt_rec;
		// Registering date range
		LocPeriod * p_lc = GetLocPeriod(pBillRec->LocID);
		THROW(p_lc);
		if(p_lc->P.low == 0 || p_lc->P.low > pBillRec->Dt)
			p_lc->P.low = pBillRec->Dt;
		if(p_lc->P.upp < pBillRec->Dt)
			p_lc->P.upp = pBillRec->Dt;
		// Searching total record for goods
		double qtty = R6(fabs(skipped ? pIlti->Quantity : pIlti->Rest));
		if(qtty > 0.0) {
			if(Search(pBillRec->LocID, pIlti->GoodsID, 0, &tdt_rec) > 0) {
				THROW(UpdateRec(&tdt_rec, pIlti, pClbNumber, pBillRec, qtty));
			}
			else
				THROW(AddRec(pIlti, pClbNumber, pBillRec, 0, qtty));
			if(DiffGoodsBySuppl) {
				PPID   suppl_id = pIlti->Suppl;
				if(!suppl_id) {
					PPObjArticle ar_obj;
					THROW(ar_obj.GetMainOrgAsSuppl(&suppl_id, 0, 0));
				}
				if(suppl_id) {
					// Searching record for {goods; suppl} combination
					if(Search(pBillRec->LocID, pIlti->GoodsID, suppl_id, &tdt_rec) > 0) {
						THROW(UpdateRec(&tdt_rec, pIlti, pClbNumber, pBillRec, qtty));
					}
					else
						THROW(AddRec(pIlti, pClbNumber, pBillRec, suppl_id, qtty));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI BillTransmDeficit::InitDeficitBill(PPBillPacket * pPack, PPID oprKind, LDATE dt, PPID locID, PPID supplID)
{
	DS.SetLocation(locID);
	if(pPack->CreateBlank(oprKind, 0, locID, 1)) {
		pPack->Rec.Dt     = dt;
		pPack->Rec.Object = supplID;
		pPack->Rec.Memo[0] = 'N';
		pPack->Rec.Memo[1] = '2';
		pPack->Rec.Memo[2] = 0;
		return 1;
	}
	else
		return 0;
}

int SLAPI BillTransmDeficit::TurnDeficit(PPID locID, LDATE dt, double pctAddition, ObjTransmContext * pCtx)
{
	int    ok = 1;
	int    first_rec = 1;
	const  PPID save_loc = LConfig.Location;
	const  PPID receipt_op = (pCtx && pCtx->Cfg.DfctRcptOpID) ? pCtx->Cfg.DfctRcptOpID : GetReceiptOp(); // @v7.7.0 pCtx->Cfg.DfctRcptOpID
	PPID   prev_suppl_id = 0;
	PPBillPacket pack;
	TempDeficitTbl::Key1 k;
	MEMSZERO(k);
	k.Location = locID;
	while(Tbl->search(1, &k, spGt) && Tbl->data.Location == locID) {
		const PPID suppl_id = Tbl->data.SupplID;
		if(suppl_id) {
			if(!first_rec) {
				if(prev_suppl_id != suppl_id) {
					THROW(BObj->__TurnPacket(&pack, 0, 1, 1));
					THROW(InitDeficitBill(&pack, receipt_op, dt, locID, suppl_id));
					prev_suppl_id = suppl_id;
				}
			}
			else {
				THROW(InitDeficitBill(&pack, receipt_op, dt, locID, suppl_id));
			}
			if(Tbl->data.HasDeficit) {
				double deficit = Tbl->data.Req - Tbl->data.Rest;
				if(deficit > 0.0) {
					PPTransferItem ti;
					THROW(ti.Init(&pack.Rec));
					THROW(ti.SetupGoods(k.GoodsID));
					ti.SetupLot(0, 0, 0);
					ti.Quantity_ = deficit;
					ti.UnitPerPack = R3(Tbl->data.UnitPerPack);
					ti.Price       = R2(Tbl->data.ReqPrice);
					if(pctAddition != 0.0)
						ti.Cost = R2(100.0 * (Tbl->data.ReqPrice - Tbl->data.ReqSalesTax) / (100.0 + pctAddition));
					else
						ti.Cost = R2(Tbl->data.ReqCost);
					THROW(pack.InsertRow(&ti, 0));
					THROW(pack.ClbL.AddNumber(pack.GetTCount()-1, Tbl->data.Clb));
				}
			}
			first_rec = 0;
		}
	}
	THROW(BObj->__TurnPacket(&pack, 0, 1, 1));
	//
	// Восстановление активного склада, в принципе, должно осуществляться //
	// после блока CATCH-ENDCATCH, однако, функция SetLocation затирает
	// информацию о последней ошибке Btrieve. По-тому пришлось вынести
	// вызов этой функции наверх.
	//
	DS.SetLocation(save_loc);
	CATCHZOK
	return ok;
}

int SLAPI BillTransmDeficit::TurnDeficitDialog(double * pPctAddition)
{
	class TDeficitDialog : public PPListDialog {
	public:
		TDeficitDialog() : PPListDialog(DLG_TDEFICIT, CTL_TDEFICIT_LIST)
		{
			PctAddition = 0.0;
			SetupCalDate(CTLCAL_TDEFICIT_DATE, CTL_TDEFICIT_DATE);
		}
		int setDTS(TSArray <LocPeriod> & rList, double pctAddition)
		{
			OrgLocPeriodList = rList;
			LocPeriodList = rList;
			PctAddition = pctAddition;
			setCtrlReal(CTL_TDEFICIT_PCTADD, PctAddition);
			updateList(-1);
			return 1;
		}
		int getDTS(TSArray <LocPeriod> & rList, double * pPctAddition)
		{
			rList = LocPeriodList;
			PctAddition = getCtrlReal(CTL_TDEFICIT_PCTADD);
			ASSIGN_PTR(pPctAddition, PctAddition);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			PPListDialog::handleEvent(event);
			if(event.isCmd(cmLBItemFocused)) {
				long   pos = 0, id = 0;
				if(getCurItem(&pos, &id) > 0 && pos >= 0 && pos < (long)LocPeriodList.getCount()) {
					const BillTransmDeficit::LocPeriod & r_item = LocPeriodList.at(pos);
					setCtrlDate(CTL_TDEFICIT_DATE, r_item.P.low);
				}
			}
		}
		virtual int setupList()
		{
			int    ok = 1;
			StringSet ss(SLBColumnDelim);
			SString temp_buf;
			for(uint i = 0; i < LocPeriodList.getCount(); i++) {
				ss.clear(0);
				const BillTransmDeficit::LocPeriod & r_item = LocPeriodList.at(i);
				GetLocationName(r_item.LocID, temp_buf = 0);
				ss.add(temp_buf);
				(temp_buf = 0).Cat(r_item.P.low, DATF_DMY);
				ss.add(temp_buf);
				THROW(addStringToList(i+1, ss.getBuf()));
			}
			CATCHZOK
			return ok;
		}
		virtual int editItem(long pos, long id)
		{
			int    ok = -1;
			if(pos >= 0 && pos < (long)LocPeriodList.getCount()) {
				BillTransmDeficit::LocPeriod & r_item = LocPeriodList.at(pos);
				LDATE dt = getCtrlDate(CTL_TDEFICIT_DATE);
				if(!checkdate(dt, 0)) {
					PPError(PPERR_SLIB);
				}
				else {
					const BillTransmDeficit::LocPeriod & r_org_item = OrgLocPeriodList.at(pos);
					if(dt > r_org_item.P.low) {
						SString temp_buf;
						temp_buf.Cat(r_org_item.P.low, DATF_DMY);
						PPError(PPERR_TOOHIGHDATE, temp_buf);
					}
					else {
						r_item.P.low = dt;
						ok = 1;
					}
				}
			}
			return ok;
		}

		TSArray <LocPeriod> LocPeriodList, OrgLocPeriodList;
		double PctAddition;
	};
	int    ok = -1;
	if(!CS_SERVER) { // @v8.5.2
		double pct_add = *pPctAddition;
		TDeficitDialog * dlg = new TDeficitDialog;
		if(CheckDialogPtr(&dlg, 1)) {
			dlg->setDTS(LocPeriodList_, pct_add);
			while(ok < 0 && ExecView(dlg) == cmOK) {
				dlg->getDTS(LocPeriodList_, &pct_add);
				*pPctAddition = pct_add;
				ok = 1;
			}
		}
		else
			ok = 0;
		delete dlg;
	}
	return ok;
}

int SLAPI BillTransmDeficit::ProcessDeficit(ObjTransmContext * pCtx, int * pNextPassNeeded)
{
	int    ok = 1;
	if(pCtx->Cfg.Flags & (DBDXF_TURNTOTALDEFICITE | DBDXF_CALCTOTALDEFICITE)) {
		if(CompleteGoodsRest()) {
			if(ok) {
				if(pCtx->Cfg.Flags & DBDXF_TURNTOTALDEFICITE) {
					double pct_add = 0.0;
					if((pCtx->Cfg.Flags & DBDXF_TWOPASSRCV) || TurnDeficitDialog(&pct_add) > 0) {
						for(uint i = 0; i < LocPeriodList_.getCount(); i++) {
							// @v8.1.5 LDATE  dt = Period.low;
							const LDATE dt = LocPeriodList_.at(i).P.low;
							const PPID loc_id = LocPeriodList_.at(i).LocID;
							if(pCtx->Cfg.Flags & DBDXF_TWOPASSRCV) {
								if((ok = TurnDeficit(loc_id, dt, fdiv100i(pCtx->Cfg.PctAdd), pCtx)) > 0)
									ASSIGN_PTR(pNextPassNeeded, 1);
							}
							else
								ok = TurnDeficit(loc_id, dt, pct_add, pCtx);
						}
					}
				}
				else if(pCtx->Cfg.Flags & DBDXF_SUBSTDEFICITGOODS) {
					PPIDArray goods_list;
					if(GetGoodsDeficitList(&goods_list) > 0 && goods_list.getCount() > 0) {
						ResolveGoodsItemList list;
						list = goods_list;
						PPWait(0);
						if(ResolveGoodsDlg(&list, RESOLVEGF_MAXLIKEGOODS|RESOLVEGF_SHOWRESOLVED) > 0) {
							SString msg, buf, buf2, new_goods_name;
							ObjSyncTbl sync_tbl;
							PPLoadString(PPSTR_TEXT, PPTXT_LOG_GOODSNEWSYNC, msg);
							for(uint i = 0; i < list.getCount(); i++) {
								PPID prev_id = list.at(i).GoodsID;
								PPID new_id = list.at(i).ResolvedGoodsID;
								if(prev_id && new_id && prev_id != new_id) {
									ObjSyncTbl::Key1 k1;
									MEMSZERO(k1);
									k1.ObjType = PPOBJ_GOODS;
									k1.ObjID   = new_id;
									k1.DBID    = (short)LConfig.DBDiv;
									if(sync_tbl.searchForUpdate(1, &k1, spEq) > 0) {
										sync_tbl.data.ObjID = 0;
										sync_tbl.updateRec(); // @sfu
										(new_goods_name = 0).Space();
										(buf2 = 0).Space();
										buf.Printf((const char*)msg, (const char *)new_goods_name, new_id, (const char*)buf2, 0);
										pCtx->OutReceivingMsg(buf);
									}
									k1.ObjID = prev_id;
									if(sync_tbl.searchForUpdate(1, &k1, spEq) > 0) {
										sync_tbl.data.ObjID = new_id;
										sync_tbl.updateRec(); // @sfu
										GetGoodsName(new_id, new_goods_name);
										buf.Printf((const char*)msg, list.at(i).GoodsName, prev_id, (const char*)new_goods_name, new_id);
										pCtx->OutReceivingMsg(buf);
									}
								}
							}
						}
					}
				}
			}
			if(ok)
				PrintTotalDeficit(pCtx);
		}
		else
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

// static
int SLAPI PPObjBill::TotalTransmitProblems(ObjTransmContext * pCtx, int * pNextPassNeeded)
{
	int    ok = -1;
	if(pCtx && pCtx->P_Btd) {
		ok = pCtx->P_Btd->ProcessDeficit(pCtx, pNextPassNeeded);
		ZDELETE(pCtx->P_Btd);
	}
	if(!ok)
		PPError();
	return ok;
}

int SLAPI PPObjBill::RegisterTransmitProblems(PPBillPacket * pPack, ILBillPacket * pIlBp, int skipped, ObjTransmContext * pCtx)
{
	double new_amt = pPack->GetAmount();
	double org_amt = BR2(pIlBp->Rec.Amount);
	if(R6(new_amt - org_amt) != 0.0 || org_amt == 0.0 || skipped) {
		uint   i;
		ILTI * p_ilti;
		SETIFZ(pCtx->P_Btd, new BillTransmDeficit);
		SString msg_buf, fmt_buf, bill_code, clb_number;
		PPObjBill::MakeCodeString(&pPack->Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_code);
		msg_buf.Printf(PPLoadTextS(PPTXT_BTP_HEADER, fmt_buf), (const char *)bill_code, org_amt, new_amt);
		pCtx->OutReceivingMsg(msg_buf);
		PPID   ilb_id = pIlBp->Rec.ID;
		if(!pCtx->P_Btd->LookedBills.lsearch(ilb_id)) {
			for(i = 0; pIlBp->Lots.enumItems(&i, (void**)&p_ilti);) {
				ILTI   ilti = *p_ilti;
				pIlBp->ClbL.GetNumber(i-1, &clb_number);
				if(R6(ilti.Rest) != 0.0) {
					PPFormatT(PPTXT_BTP_LINE, &msg_buf, ilti.GoodsID, ilti.GoodsID, ilti.Rest, ilti.Cost, ilti.Price, ilti.Suppl);
					pCtx->OutReceivingMsg(msg_buf);
				}
				if(!pCtx->P_Btd->AddItem(p_ilti, clb_number, &pIlBp->Rec, skipped))
					pCtx->OutputLastError();
			}
			pCtx->P_Btd->LookedBills.add(ilb_id);
		}
		if(skipped)
			pCtx->OutputString(PPTXT_BTP_NOTTURNED, 0);
	}
	return 1;
}

int SLAPI PPObjBill::SerializePacket(int dir, ILBillPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	long   ff;
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW(pPack->Ext.Serialize(dir, rBuf, pSCtx));
	THROW(pPack->SerializeLots(dir, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->LocObj, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &pPack->Amounts, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &pPack->Pays, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &pPack->OrderBillList, rBuf));
	THROW(pPack->LTagL.Serialize(dir, rBuf, pSCtx)); // @v7.3.5
	THROW(pPack->BTagL.Serialize(dir, rBuf, pSCtx)); // @v7.3.5
	THROW(pPack->AdvList.Serialize(dir, rBuf, pSCtx));
	if(dir > 0) {
		ff = (GetOpType(pPack->Rec.OpID) == PPOPT_ACCTURN) ? SBuffer::ffAryCount32 : (SBuffer::ffAryCount32|SBuffer::ffAryForceEmpty);
		THROW_SL(rBuf.Write(&pPack->Turns, ff));
		THROW(pPack->ClbL.Write(rBuf));
		THROW(pPack->SnL.Write(rBuf));
		{
			uint32 sz = pPack->Rent.IsEmpty() ? 0 : sizeof(pPack->Rent);
			THROW_SL(pSCtx->SerializeBlock(+1, sz, &pPack->Rent, rBuf, 1));
		}
		THROW_SL(pSCtx->SerializeBlock(+1, sizeof(*pPack->P_Freight), pPack->P_Freight, rBuf, 1));
		THROW_SL(pSCtx->SerializeBlock(+1, sizeof(*pPack->P_AdvRep), pPack->P_AdvRep, rBuf, 1));
	}
	else if(dir < 0) {
		int    r;
		ff = (GetOpType(pPack->Rec.OpID) == PPOPT_ACCTURN) ? SBuffer::ffAryCount32 : (SBuffer::ffAryCount32|SBuffer::ffAryForceEmpty);
		THROW_SL(rBuf.Read(&pPack->Turns, ff));
		THROW(pPack->ClbL.Read(rBuf));
		THROW(pPack->SnL.Read(rBuf));
		THROW_SL(pSCtx->SerializeBlock(-1, sizeof(pPack->Rent), &pPack->Rent, rBuf, 1));
		{
			PPFreight freight;
			THROW_SL(pSCtx->SerializeBlock(-1, sizeof(*pPack->P_Freight), &freight, rBuf, 1));
			if(!freight.IsEmpty()) {
				THROW_MEM(SETIFZ(pPack->P_Freight, new PPFreight));
				*pPack->P_Freight = freight;
			}
			else
				ZDELETE(pPack->P_Freight);
		}
		{
			PPAdvanceRep adv_rep;
			THROW_SL(r = pSCtx->SerializeBlock(-1, sizeof(*pPack->P_AdvRep), &adv_rep, rBuf, 1));
			if(r > 0) {
				THROW_MEM(SETIFZ(pPack->P_AdvRep, new PPAdvanceRep));
				*pPack->P_AdvRep = adv_rep;
			}
		}
	}
	THROW_SL(GetInvT().SerializeArrayOfRecords(dir, &pPack->InvList, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::AcceptLotSync(const PPBillPacket & rBp, const ILBillPacket & rIBp, ObjTransmContext * pCtx, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(uint i = 0; i < rBp.GetTCount(); i++) {
			const PPTransferItem & r_ti = rBp.TI(i);
			if(r_ti.SrcIltiPos > 0 && r_ti.SrcIltiPos <= (int)rIBp.Lots.getCount()) {
				ILTI & r_ilti = rIBp.Lots.at(r_ti.SrcIltiPos-1);
				PPObjID frgn_objid;
				if(r_ti.Flags & PPTFR_RECEIPT) {
					if(rIBp.IlbFlags & ILBillPacket::ilbfConvertedIntrExp) {
						//
						// Для преобразованной в межскладской приход внутренней передачи
						// используем синхронизацию r_ilti.LotMirrID
						//
						if(r_ilti.LotMirrID) {
							frgn_objid.Set(PPOBJ_LOT, r_ilti.LotMirrID);
							THROW(pCtx->AcceptDependedNonObject(frgn_objid, r_ti.LotID, 0, 0));
						}
						else {
							// __LogDebugMessage(pCtx, "r_ilti.LotMirrID == 0"); // @debug
						}
					}
					else {
						if(r_ilti.LotSyncID) {
							frgn_objid.Set(PPOBJ_LOT, r_ilti.LotSyncID);
							THROW(pCtx->AcceptDependedNonObject(frgn_objid, r_ti.LotID, 0, 0));
						}
						else {
							// __LogDebugMessage(pCtx, "r_ilti.LotSyncID == 0"); // @debug
						}
					}
				}
				else if(r_ti.Flags & PPTFR_UNITEINTR) {
					//
					// Для полностью принятой внутренней передачи товара необходимо синхронизировать
					// лот зеркальной записи (по r_ilti.LotMirrID)
					//
					TransferTbl::Rec mirr_rec;
					if(trfr->SearchByBill(r_ti.BillID, 1, r_ti.RByBill, &mirr_rec) > 0 && mirr_rec.Flags & PPTFR_RECEIPT) {
						frgn_objid.Set(PPOBJ_LOT, r_ilti.LotMirrID);
						THROW(pCtx->AcceptDependedNonObject(frgn_objid, mirr_rec.LotID, 0, 0));
					}
				}
			}
			/*
			else {
				// @debug {
				SString msg_buf;
				(msg_buf = 0).CatEq("r_ti.SrcIltiPos", (long)r_ti.SrcIltiPos).Space().Cat("out of packet.TI");
				__LogDebugMessage(pCtx, msg_buf); // !!!!!!!!!!!!!!
				// } @debug
			}
			*/
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	const  int trace_sync_lot = BIN(DS.CheckExtFlag(ECF_TRACESYNCLOT));

	int    ok = 1, r;
	const PPConfig & r_cfg = LConfig;
	PPID   err_id   = 0; // ИД документа, вызвавшего ошибку
	int    err_code = 0; // Код ошибки
	SString err_bill_code; // Номер документа, вызвавшего ошибку
	SString msg_buf, fmt_buf;
	uint   i;
	PPTransferItem * p_ti;
	ILBillPacket   * p_pack = (ILBillPacket*)p->Data;
	const short save_rlz_order = r_cfg.RealizeOrder;
	if(p_pack) {
		if(stream == 0) {
			int    skip = 0, warn = 0;
			PPBillPacket bp;
			const  PPID   op_type_id = GetOpType(p_pack->Rec.OpID);
			if(oneof2(pCtx->Cfg.RealizeOrder, RLZORD_FIFO, RLZORD_LIFO))
				DS.SetRealizeOrder(pCtx->Cfg.RealizeOrder);
			if(pCtx->Flags & ObjTransmContext::fConsolid) {
				p_pack->Rec.Flags |= BILLF_FIXEDAMOUNTS;
				p_pack->Lots.freeAll();
			}
			if(*pID == 0) {
				if(oneof3(op_type_id, PPOPT_GOODSRETURN, PPOPT_PAYMENT, PPOPT_CORRECTION)) {
					if(p_pack->Rec.LinkBillID && Search(p_pack->Rec.LinkBillID) <= 0) {
						SString bill_code;
						PPObjBill::MakeCodeString(&p_pack->Rec, 0, bill_code);
						pCtx->OutReceivingMsg(msg_buf.Printf(PPLoadTextS(PPTXT_BTP_NOLINKBILL, fmt_buf), (const char *)bill_code));
						skip = 1;
					}
				}
				else
					bp.Rec.LinkBillID = 0;
				if(!skip) {
					bp.Rec.UserID = r_cfg.User;
					err_code = PPTXT_ERRACCEPTBILL_CONVERT;
					err_id = p_pack->Rec.ID;
					PPObjBill::MakeCodeString(&p_pack->Rec, 0, err_bill_code);
					THROW(p_pack->ConvertToBillPacket(bp, &warn, pCtx, 1));
					bp.ProcessFlags |= PPBillPacket::pfForeignSync; // @v8.0.3
					//
					// Функция ConvertToBillPacket могла инициализировать статус документа
					// в значение для нового документа (из конфигурации). Это - не верно.
					// Необходимо чтобы документ имел тот же статус, что и в разделе-отправителе.
					//
					if(p_pack->Rec.StatusID)
						bp.Rec.StatusID = p_pack->Rec.StatusID;
					if(warn >= 2 && pCtx->Cfg.Flags & DBDXF_SKIPINCOMPLBILL)
						skip = 1;
					RegisterTransmitProblems(&bp, p_pack, skip, pCtx);
					if(!skip) {
						PPObjBill::MakeCodeString(&bp.Rec, 0, err_bill_code);
						if(op_type_id != PPOPT_INVENTORY) {
							bp.Rec.ID = 0;
							for(i = 0; bp.EnumTItems(&i, &p_ti);) {
								p_ti->BillID  = 0;
								if(!(bp.Rec.Flags2 & BILLF2_FULLSYNC)) // @v8.0.3
									p_ti->RByBill = 0;
							}
							if(p_pack->OrderBillList.getCount()) {
								SString ord_bill_code;
								for(i = 0; i < p_pack->OrderBillList.getCount(); i++) {
									PPID   order_bill_id = p_pack->OrderBillList.get(i);
									PPBillPacket ord_pack;
									if(ExtractPacket(order_bill_id, &ord_pack) > 0) {
										PPObjBill::MakeCodeString(&ord_pack.Rec, 0, ord_bill_code);
										(msg_buf = ord_bill_code).Space().CatCharN('>', 2).Space().Cat(err_bill_code);
										if(bp.AttachToOrder(&ord_pack) > 0)
											pCtx->OutputString(PPTXT_BINDLADINGTOORD_OK, msg_buf);
										else
											pCtx->OutputString(PPTXT_BINDLADINGTOORD_NONE, msg_buf);
									}
									else {
										ideqvalstr(order_bill_id, (msg_buf = ord_bill_code).Space().CatCharN('>', 2).Space());
										pCtx->OutputString(PPTXT_BINDLADINGTOORD_ORDNF, msg_buf);
									}
								}
							}
							{
								PPTransaction tra(1);
								THROW(tra);
								err_code = PPTXT_ERRACCEPTBILL;
								err_id = bp.Rec.ID;
								if(trace_sync_lot) {
									__Debug_TraceLotSync(*p_pack, &bp, msg_buf);
									PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
								}
								THROW(TurnPacket(&bp, 0));
								if(trace_sync_lot) {
									__Debug_TraceLotSync(*p_pack, &bp, msg_buf);
									PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
								}
								THROW(AcceptLotSync(bp, *p_pack, pCtx, 0));
								if(trace_sync_lot) {
									__Debug_TraceLotSync(*p_pack, &bp, msg_buf);
									PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
								}
								THROW(tra.Commit());
								ok = 101; // @ObjectCreated
							}
						}
					}
					else
						ok = -1;
				}
				else
					ok = -1;
				*pID = bp.Rec.ID;
			}
			else {
				//
				// Изменение документа
				//
				PPTransaction tra(1);
				THROW(tra);
				if(oneof2(op_type_id, PPOPT_ACCTURN, PPOPT_PAYMENT) || IsDraftOp(p_pack->Rec.OpID)) {
					bp.Rec.UserID = r_cfg.User;
					err_code = PPTXT_ERRACCEPTBILL_CONVERT;
					err_id = p_pack->Rec.ID;
					PPObjBill::MakeCodeString(&p_pack->Rec, 0, err_bill_code);
					THROW(p_pack->ConvertToBillPacket(bp, &warn, pCtx, 0));
					err_id = bp.Rec.ID;
					PPObjBill::MakeCodeString(&bp.Rec, 0, err_bill_code);
					if(warn >= 2) {
						if(pCtx->Cfg.Flags & DBDXF_SKIPINCOMPLBILL)
							skip = 1;
					}
					RegisterTransmitProblems(&bp, p_pack, skip, pCtx);
					if(!skip) {
						BillTbl::Rec bill_rec;
						const PPID op_id = bp.Rec.OpID;
						bp.Rec.ID = *pID;
						//
						// Для draft-документов необходимо сохранить оригинальное значение
						// признака списания документа.
						//
						if(IsDraftOp(op_id) && Search(*pID, &bill_rec) > 0) {
							err_code = PPTXT_ERRACCEPTBILL_INCOMPATOP;
							THROW(bill_rec.OpID == op_id);
							int    org_wroff_tag = BIN(bill_rec.Flags & BILLF_WRITEDOFF);
							SETFLAG(bp.Rec.Flags, BILLF_WRITEDOFF, org_wroff_tag);
						}
						//
						// Для долговых и зачетных документов пересчитываем сумму долга, так как
						// в базе-приемнике оплаты (зачеты) по документу могут быть не такими,
						// как в базе-отправителе
						//
						if(CheckOpFlags(op_id, OPKF_NEEDPAYMENT) || CheckOpFlags(op_id, OPKF_RECKON)) {
							double paym = 0.0;
							double amt = bp.GetAmount();
							P_Tbl->CalcPayment(*pID, 1, 0, bp.Rec.CurID, &paym);
							bp.Amounts.Put(PPAMT_PAYMENT, bp.Rec.CurID, paym, 1, 1);
							bp.Rec.PaymAmount = paym; // @v8.5.8
							SETFLAG(bp.Rec.Flags, BILLF_PAYOUT, R2(paym - amt) >= 0);
						}
						err_code = PPTXT_ERRACCEPTBILL;
						THROW(r = UpdatePacket(&bp, 0));
						if(r > 0)
							ok = 102; // @ObjectUpdated
					}
				}
				//
				// Частичная модификация товарных документов
				//
				else if(oneof6(op_type_id, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, PPOPT_GOODSMODIF, PPOPT_GOODSRETURN, PPOPT_GOODSREVAL, PPOPT_CORRECTION)) {
					// @v8.0.3 PPOPT_GOODSRETURN, PPOPT_GOODSREVAL, @v8.5.4 PPOPT_CORRECTION
					int    to_update = 0;
					int    to_turn = 0;
					int    to_update_rec = 0;
					int    do_update_withoutlots = 1;
					uint   i;
					PPObjAmountType amtt_obj;
					PPAmountType amtt_rec;
					err_code = PPTXT_ERRACCEPTBILL_CONVERT;
					err_id = p_pack->Rec.ID;
					PPObjBill::MakeCodeString(&p_pack->Rec, 0, err_bill_code);
					THROW(ExtractPacket(*pID, &bp, 0) > 0);
					// @v8.0.3 {
					if(CConfig.Flags2 & CCFLG2_SYNCLOT && GetConfig().Flags & BCF_ACCEPTGOODSBILLCHG) {
						if(bp.Rec.Flags2 & BILLF2_FULLSYNC) {
							err_code = PPTXT_ERRACCEPTBILL_CONVERT;
							THROW(p_pack->ConvertToBillPacket(bp, &warn, pCtx, 0));
							assert(bp.Rec.ID == *pID);
							bp.ProcessFlags |= PPBillPacket::pfForeignSync;
							SETMAX(bp.Rec.LastRByBill, p_pack->Rec.LastRByBill);
							THROW(bp.InitAmounts(0));
							THROW(FillTurnList(&bp));
							if(trace_sync_lot) {
								__Debug_TraceLotSync(*p_pack, &bp, msg_buf);
								PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
							}
							THROW(r = UpdatePacket(&bp, 0));
							if(trace_sync_lot) {
								__Debug_TraceLotSync(*p_pack, &bp, msg_buf);
								PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
							}
							THROW(AcceptLotSync(bp, *p_pack, pCtx, 0));
							if(trace_sync_lot) {
								__Debug_TraceLotSync(*p_pack, &bp, msg_buf);
								PPLogMessage(PPFILNAM_SYNCLOT_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
							}
							if(r > 0)
								ok = 102; // @ObjectUpdated
							do_update_withoutlots = 0;
						}
						else {
							pCtx->OutReceivingMsg(msg_buf.Printf(PPLoadTextS(PPTXT_NACCEPTBILLMOD_NFS, fmt_buf), (const char *)err_bill_code));
						}
					}
					// } @v8.0.3
					if(do_update_withoutlots) {
						if(!bp.Ext.IsEqual(p_pack->Ext)) {
							bp.Ext = p_pack->Ext;
							to_update = 1;
						}
						if(p_pack->P_Freight && (!bp.P_Freight || !bp.P_Freight->IsEqual(*p_pack->P_Freight))) {
							bp.SetFreight(p_pack->P_Freight);
							to_update = 1;
						}
						if(p_pack->Rec.StatusID != bp.Rec.StatusID || p_pack->Rec.Object2 != bp.Rec.Object2) {
							SETFLAG(bp.Rec.Flags, BILLF_WHITELABEL, BIN(p_pack->Rec.Flags & BILLF_WHITELABEL)); // @v7.7.4
							to_update_rec = 1;
						}
						//
						// Тонкий момент: флаг BILLF_SHIPPED меняется только в одну сторону: снимать нелья - только устанавливать
						//
						if(!(bp.Rec.Flags & BILLF_SHIPPED) && p_pack->Rec.Flags & BILLF_SHIPPED) {
							bp.Rec.Flags |= BILLF_SHIPPED;
							to_update_rec = 1;
						}
						// @v7.7.4 {
						if((bp.Rec.Flags & BILLF_WHITELABEL) != (p_pack->Rec.Flags & BILLF_WHITELABEL)) {
							SETFLAGBYSAMPLE(bp.Rec.Flags, BILLF_WHITELABEL, p_pack->Rec.Flags); // @v8.1.0 @fix
							to_update_rec = 1;
						}
						// } @v7.7.4
						for(i = 0; i < p_pack->Amounts.getCount(); i++) {
							AmtEntry & r_entry = p_pack->Amounts.at(i);
							if(amtt_obj.Fetch(r_entry.AmtTypeID, &amtt_rec) > 0 && amtt_rec.Flags & PPAmountType::fManual) {
								double val = bp.Amounts.Get(r_entry.AmtTypeID, r_entry.CurID);
								if(R6(val-r_entry.Amt) != 0.0) {
									bp.Amounts.Put(&r_entry, 0, 1);
									to_turn = 1;
								}
							}
						}
						err_code = PPTXT_ERRACCEPTBILL;
						err_id = bp.Rec.ID;
						PPObjBill::MakeCodeString(&bp.Rec, 0, err_bill_code);
						if(to_turn) {
							THROW(bp.InitAmounts(0));
							THROW(FillTurnList(&bp))
							THROW(r = UpdatePacket(&bp, 0));
							if(r > 0)
								ok = 102; // @ObjectUpdated
						}
						else {
							if(to_update_rec) {
								SETFLAG(bp.Rec.Flags, BILLF_FREIGHT, bp.P_Freight);
								if(!(bp.Rec.Flags & BILLF_SHIPPED) && p_pack->Rec.Flags & BILLF_SHIPPED)
									bp.Rec.Flags |= BILLF_SHIPPED;
								bp.Rec.StatusID = p_pack->Rec.StatusID;
								bp.Rec.Object2 = p_pack->Rec.Object2;
								THROW(P_Tbl->EditRec(pID, &bp.Rec, 0));
								ok = 102; // @ObjectUpdated
							}
							if(to_update) {
								THROW(P_Tbl->PutExtraData(*pID, &bp.Ext, 0));
								THROW(P_Tbl->SetFreight(*pID, bp.P_Freight, 0));
								ok = 102; // @ObjectUpdated
							}
						}
					}
				}
				THROW(tra.Commit());
			}
		}
		else {
			SBuffer buffer;
			// @v7.6.1 {
			{
				ILTI * p_ilti;
				PPTransaction tra(-1); // @v7.9.11 use_ta 1-->-1 (автоматическое определение необходимости транзакции по состоянию DbSession)
				THROW(tra);
				for(uint i = 0; p_pack->Lots.enumItems(&i, (void**)&p_ilti);) {
					PPCommSyncID commid;
					PPObjID objid;
					if(p_ilti->LotSyncID) {
						THROW(pCtx->RegisterDependedNonObject(objid.Set(PPOBJ_LOT, p_ilti->LotSyncID), &commid.SetZero(), 0));
					}
					if(p_ilti->LotMirrID) {
						THROW(pCtx->RegisterDependedNonObject(objid.Set(PPOBJ_LOT, p_ilti->LotMirrID), &commid.SetZero(), 0));
					}
				}
				THROW(tra.Commit());
			}
			// } @v7.6.1
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile((FILE*)stream, 0, 0))
		}
	}
	CATCH
		*pID = 0;
		ok = -1;
		if(err_code)
			pCtx->OutputAcceptErrMsg(err_code, err_id, err_bill_code);
	ENDCATCH
	DS.SetRealizeOrder(save_rlz_order);
	return ok;
}

int SLAPI PPObjBill::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	ILBillPacket * p_pack = new ILBillPacket;
	THROW_MEM(p_pack);
	if(stream == 0) {
		THROW(p_pack->Load(id, (pCtx->Flags & ObjTransmContext::fNotTrnsmLots) ? ILBPF_LOADAMTNOTLOTS : 0, pCtx->Extra));
		{
			const PPID op_id = p_pack->Rec.OpID;
			const PPID op_type_id = GetOpType(op_id);
			int exp = IsExpendOp(op_id);
			if(op_type_id == PPOPT_GOODSRECEIPT)
				p->Priority = 200;
			else if(exp == 0)
				p->Priority = 220;
			else if(op_type_id == PPOPT_GOODSMODIF)
				p->Priority = 240;
			else if(exp > 0)
				p->Priority = 280;
			else
				p->Priority = 300;
		}
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile((FILE*)stream, 0))
		THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
	}
	CATCH
		ok = 0;
		ZDELETE(p_pack);
	ENDCATCH
	p->Data = p_pack;
	return ok;
}

int SLAPI PPObjBill::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		uint   i;
		PPID   goods_id = 0;
		PPAccTurn * at;
		AmtEntry  * p_ae;
		ILBillPacket * p_pack = (ILBillPacket*)p->Data;
		ILTI * ilti;
		THROW(
			ProcessObjRefInArray(PPOBJ_OPRKIND,  &p_pack->Rec.OpID,        ary, replace) &&
			ProcessObjRefInArray(PPOBJ_ARTICLE,  &p_pack->LocObj,          ary, replace) &&
			ProcessObjRefInArray(PPOBJ_LOCATION, &p_pack->Rec.LocID,       ary, replace) &&
			ProcessObjRefInArray(PPOBJ_ARTICLE,  &p_pack->Rec.Object,      ary, replace) &&
			ProcessObjRefInArray(PPOBJ_ARTICLE,  &p_pack->Rec.Object2,     ary, replace) &&
			ProcessObjRefInArray(PPOBJ_CURRENCY, &p_pack->Rec.CurID,       ary, replace) &&
			ProcessObjRefInArray(PPOBJ_BILL,     &p_pack->Rec.LinkBillID,  ary, replace) &&
			ProcessObjRefInArray(PPOBJ_SCARD,    &p_pack->Rec.SCardID,     ary, replace) &&
			ProcessObjRefInArray(PPOBJ_PERSON,   &p_pack->Rec.MainOrgID,   ary, replace) &&
			ProcessObjRefInArray(PPOBJ_BILLSTATUS, &p_pack->Rec.StatusID,  ary, replace)
		);
		for(i = 0; p_pack->Turns.enumItems(&i, (void**)&at);) {
			THROW(
				ProcessObjRefInArray(PPOBJ_ACCOUNT2, &at->DbtID.ac, ary, replace) &&
				ProcessObjRefInArray(PPOBJ_ARTICLE, &at->DbtID.ar, ary, replace) &&
				ProcessObjRefInArray(PPOBJ_ACCOUNT2, &at->CrdID.ac, ary, replace) &&
				ProcessObjRefInArray(PPOBJ_ARTICLE, &at->CrdID.ar, ary, replace)
			);
		}
		for(i = 0; p_pack->Lots.enumItems(&i, (void**)&ilti);) {
			goods_id = labs(ilti->GoodsID);
			THROW(ProcessObjRefInArray(PPOBJ_GOODS,   &goods_id,    ary, replace));
			if(replace)
				ilti->GoodsID = (ilti->GoodsID < 0) ? -goods_id : goods_id;
			// @v7.6.1 {
			{
				//
				// Идентификаторы лотов не должны замещаться поскольку этим занимается функция PPObjBill::Write
				//
				PPID   lot_id = ilti->LotSyncID;
				PPID   mirr_lot_id = ilti->LotMirrID;
				THROW(ProcessObjRefInArray(PPOBJ_LOT,      &lot_id, ary, replace));
				THROW(ProcessObjRefInArray(PPOBJ_LOT,      &mirr_lot_id, ary, replace));
			}
			// } @v7.6.1
			THROW(ProcessObjRefInArray(PPOBJ_ARTICLE,  &ilti->Suppl, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_QCERT,    &ilti->QCert, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_GOODSTAX, &ilti->InTaxGrpID, ary, replace));
		}
		if(p_pack->InvList.getCount()) {
			SString fmt_buf, msg_buf, goods_name;
			for(i = 0; i < p_pack->InvList.getCount(); i++) {
				InventoryTbl::Rec & r_inv_rec = p_pack->InvList.at(i);
				goods_id = labs(r_inv_rec.GoodsID);
				PPID   foreign_id = goods_id;
				THROW(ProcessObjRefInArray(PPOBJ_GOODS, &goods_id, ary, replace));
				if(replace)
					r_inv_rec.GoodsID = (r_inv_rec.GoodsID < 0) ? -goods_id : goods_id;
				for(uint j = 0; j < i; j++) {
					InventoryTbl::Rec & r_rec2 = p_pack->InvList.at(j);
					if(r_rec2.GoodsID == r_inv_rec.GoodsID) {
						if(pCtx->Cfg.Flags & DBDXF_UNITEINVDUPREC) {
							r_rec2.Quantity  += r_inv_rec.Quantity;
							r_rec2.StockRest += r_inv_rec.StockRest;
							INVENT_SETDIFFSIGN(r_rec2.Flags, r_rec2.StockRest);
							p_pack->InvList.atFree(i--);
						}
						else {
							msg_buf = 0;
							GetGoodsName(goods_id, goods_name = 0);
							msg_buf.Printf(PPLoadTextS(PPTXT_DUPRCPTINVGOODS, fmt_buf), foreign_id, goods_id, (const char *)goods_name);
							pCtx->OutReceivingMsg(msg_buf);
							if(pCtx->Cfg.Flags & DBDXF_SKIPINCOMPLBILL) {
								pCtx->OutputString(PPTXT_BTP_NOTTURNED, 0);
								ok = -2;
							}
						}
						break;
					}
				}
			}
		}
		for(i = 0; p_pack->Amounts.enumItems(&i, (void**)&p_ae);)
			THROW(ProcessObjRefInArray(PPOBJ_AMOUNTTYPE, &p_ae->AmtTypeID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_ARTICLE, &p_pack->Ext.AgentID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_ARTICLE, &p_pack->Ext.PayerID, ary, replace));
		if(p_pack->P_AdvRep) {
			for(i = 0; i < sizeof(p_pack->P_AdvRep->Rcp) / sizeof(p_pack->P_AdvRep->Rcp[0]); i++) {
				ProcessObjRefInArray(PPOBJ_BILL,    &p_pack->P_AdvRep->Rcp[i].BillID,   ary, replace);
				ProcessObjRefInArray(PPOBJ_ARTICLE, &p_pack->P_AdvRep->Rcp[i].ObjectID, ary, replace);
			}
		}
		for(i = 0; i < p_pack->AdvList.GetCount(); i++) {
			PPAdvBillItem & abi = p_pack->AdvList.Get(i);
			ProcessObjRefInArray(PPOBJ_ADVBILLKIND, &abi.AdvBillKindID, ary, replace);
			ProcessObjRefInArray(PPOBJ_BILL,        &abi.AdvBillID, ary, replace);
			ProcessObjRefInArray(PPOBJ_ACCOUNT2,    &abi.AccID,     ary, replace);
			ProcessObjRefInArray(PPOBJ_ARTICLE,     &abi.ArID,      ary, replace);
		}
		PPFreight * p_fr = p_pack->P_Freight;
		if(p_fr) {
			ProcessObjRefInArray(PPOBJ_LOCATION, &p_fr->DlvrAddrID, ary, replace);
			ProcessObjRefInArray(PPOBJ_WORLD,    &p_fr->PortOfLoading, ary, replace);
			ProcessObjRefInArray(PPOBJ_WORLD,    &p_fr->PortOfDischarge, ary, replace);
			ProcessObjRefInArray(PPOBJ_PERSON,   &p_fr->CaptainID, ary, replace);
			ProcessObjRefInArray(PPOBJ_PERSON,   &p_fr->AgentID, ary, replace);
			ProcessObjRefInArray(PPOBJ_TRANSPORT, &p_fr->ShipID, ary, replace);
			ProcessObjRefInArray(PPOBJ_LOCATION, &p_fr->StorageLocID, ary, replace); // @v8.8.6
		}
		for(i = 0; i < p_pack->OrderBillList.getCount(); i++) {
			PPID & r_ord_bill_id = p_pack->OrderBillList.at(i);
			ProcessObjRefInArray(PPOBJ_BILL, &r_ord_bill_id, ary, replace);
		}
		THROW(p_pack->LTagL.ProcessObjRefs(ary, replace)); // @v7.3.5
		THROW(p_pack->BTagL.ProcessObjRefs(ary, replace)); // @v7.3.5
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjBill, ILBillPacket);

int SLAPI PPObjBill::NeedTransmit(PPID id, const DBDivPack & rDestDbDivPack, ObjTransmContext * pCtx)
{
	int    ok = -1, r;
	uint   msg_id = 0;
	BillTbl::Rec bill_rec, link_rec;
	MEMSZERO(bill_rec);
	if(Search(id, &bill_rec) > 0 && bill_rec.OpID && !(bill_rec.Flags & BILLF_CASH)) {
		if(!bill_rec.StatusID || !CheckStatusFlag(bill_rec.StatusID, BILSTF_DENY_TRANSM)) {
			PPID   op_id = bill_rec.OpID;
			PPOprKind op_rec;
			PPID   op_type_id = GetOpType(op_id, &op_rec);
			if(op_type_id == PPOPT_ACCTURN) {
				if(op_rec.Flags & OPKF_EXTACCTURN) {
					if(rDestDbDivPack.ResponsibleForLoc(bill_rec.LocID, 0))
						ok = 1;
				}
				else {
					SString temp_buf;
					rDestDbDivPack.GetExtStrData(DBDIVEXSTR_ACCLIST, temp_buf);
					if(temp_buf.NotEmpty()) {
						AccTurnCore * p_atc = atobj->P_Tbl;
						for(int i = 0; ok < 0 && (r = p_atc->EnumByBill(bill_rec.ID, &i, 0)) > 0;) {
							THROW(p_atc->AccRel.Search(p_atc->data.Acc) > 0);
							if(IsAccBelongToList((Acct*)&p_atc->AccRel.data.Ac, 0, temp_buf))
								ok = 1;
							else {
								THROW(p_atc->AccRel.Search(p_atc->data.CorrAcc) > 0);
								if(IsAccBelongToList((Acct*)&p_atc->AccRel.data.Ac, 1, temp_buf))
									ok = 1;
							}
						}
						THROW(r);
					}
				}
			}
			else if(op_type_id == PPOPT_PAYMENT) {
				//
				// Зачетные оплаты не передаем
				//
				PPID   paym_pool_owner_id = 0;
				if(IsMemberOfPool(bill_rec.ID, PPASS_PAYMBILLPOOL, &paym_pool_owner_id) > 0) {
					msg_id = PPTXT_LOG_NTRANS_BILLRECKON;
					ok = -1;
				}
				else if(rDestDbDivPack.ResponsibleForLoc(bill_rec.LocID, 0))
					if(Search(bill_rec.LinkBillID, &bill_rec) > 0)
						if(rDestDbDivPack.ResponsibleForLoc(bill_rec.LocID, 0))
							ok = 1;
			}
			else if(op_type_id == PPOPT_INVENTORY)
				ok = 1;
			// @v8.0.3 {
			else if(oneof2(op_type_id, PPOPT_GOODSREVAL, PPOPT_CORRECTION)) { // @v8.5.4 PPOPT_CORRECTION
				if(CConfig.Flags2 & CCFLG2_SYNCLOT)
					ok = 1;
				else
					msg_id = PPTXT_LOG_NTRANS_BILLRVLLS;
			}
			// } @v8.0.3
			else if((bill_rec.Flags & (BILLF_GEXPEND|BILLF_GRECEIPT|BILLF_GMODIF)) ||
				oneof5(op_type_id, PPOPT_GOODSORDER, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_GOODSACK)) {
				if(rDestDbDivPack.ResponsibleForLoc(bill_rec.LocID, 0))
					ok = 1;
				else if(IsIntrExpndOp(op_id)) {
					if(rDestDbDivPack.ResponsibleForLoc(PPObjLocation::ObjToWarehouse(bill_rec.Object), 0))
						ok = 1;
					else
						msg_id = PPTXT_LOG_NTRANS_BILLINTREX;
				}
				else {
					msg_id = PPTXT_LOG_NTRANS_BILLLOC;
				}
				if(ok > 0) {
					//
					// Не следует передавать межскладской приход (образованый в результате передачи межскладского
					// расхода) тому разделу, который одновременно отвечает и за локацию-поставщика и за
					// локацию-получателя. Пускай раздел, с которого была сделана передача, сам
					// переносит этот межсклад в такой раздел.
					//
					if(IsIntrOp(op_id) == INTRRCPT && CConfig.IntrReceiptOp == op_id)
						if(rDestDbDivPack.ResponsibleForLoc(PPObjLocation::ObjToWarehouse(bill_rec.Object), 0)) {
							msg_id = PPTXT_LOG_NTRANS_BILLINTRRC;
							ok = -1;
						}
					//
					// Не следует передавать документы списания инвентаризации, поскольку инвентаризация //
					// сама передается как документ и подлежит списанию в другом разделе
					//
					if(bill_rec.LinkBillID && !(pCtx->Cfg.Flags & DBDXF_SENDINVWROFFBILLS) &&
						Search(bill_rec.LinkBillID, &link_rec) > 0 && GetOpType(link_rec.OpID) == PPOPT_INVENTORY) {
						msg_id = PPTXT_LOG_NTRANS_BILLINVWRO;
						ok = -1;
					}
					//
					// Не следует передавать документы списания кассовых сессий если должны
					// передаваться сами кассовые сессии (за исключением случая, когда в разделе-
					// приемнике установлен флаг DBDIVF_RCVCSESSANDWROFFBILLS (@v4.6.4)).
					//
					PPID   sess_id = 0;
					if(pCtx->Cfg.Flags & DBDXF_SENDCSESSION && !(rDestDbDivPack.Rec.Flags & DBDIVF_RCVCSESSANDWROFFBILLS) &&
						IsMemberOfPool(id, PPASS_CSESSBILLPOOL, &sess_id) > 0) {
						msg_id = PPTXT_LOG_NTRANS_BILLCSWRO;
						ok = -1;
					}
				}
			}
		}
	}
	if(ok < 0 && msg_id && pCtx) {
		SString fmt_buf, msg_buf, bill_buf;
		PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_buf);
		pCtx->Output(msg_buf.Printf(PPLoadTextS(msg_id, fmt_buf), (const char *)bill_buf));
	}
	CATCHZOK
	return ok;
}
