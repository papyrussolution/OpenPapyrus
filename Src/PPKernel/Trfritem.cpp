// TRFRITEM.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2015, 2016, 2017
// @Kernel
//
#include <pp.h>
#pragma hdrstop

static inline int Impl_IsUnlimWoLot(long flags)
{
	if(CConfig.Flags & CCFLG_GENLOTONUNLIMORDER)
		return BIN((flags & PPTFR_ACK) || (flags & PPTFR_UNLIM && !(flags & (PPTFR_ORDER|PPTFR_SHADOW))));
	else
		return BIN(flags & (PPTFR_UNLIM|PPTFR_ACK));
}

SLAPI PPTransferItem::PPTransferItem()
{
	THISZERO();
	Init(0, 1, TISIGN_UNDEF);
}

SLAPI PPTransferItem::PPTransferItem(const BillTbl::Rec * pBillRec, int forceSign)
{
	THISZERO();
	Init(pBillRec, 1, forceSign);
}

int SLAPI PPTransferItem::IsUnlimWoLot() const
{
	return Impl_IsUnlimWoLot(Flags);
}

int SLAPI IsUnlimWoLot(const TransferTbl::Rec & rRec)
{
	return Impl_IsUnlimWoLot(rRec.Flags);
}

int SLAPI PPTransferItem::IsLotRet() const
{
	return (!(Flags & (PPTFR_UNLIM|PPTFR_ACK|PPTFR_REVAL|PPTFR_RECEIPT|PPTFR_DRAFT)) && (Flags & PPTFR_PLUS));
}

//static
int FASTCALL PPTransferItem::IsRecomplete(long flags)
{
	return BIN((flags & (PPTFR_MODIF|PPTFR_REVAL)) == (PPTFR_MODIF|PPTFR_REVAL));
}

int SLAPI PPTransferItem::IsRecomplete() const
{
	return PPTransferItem::IsRecomplete(Flags);
}

void SLAPI PPTransferItem::InitShadow(const BillTbl::Rec * pBillRec, const PPTransferItem * pOrder)
{
	THISZERO();
	if(pBillRec) {
		Date     = pBillRec->Dt;
		BillID   = pOrder->BillID;
		RByBill  = 0;
		LocID    = pBillRec->LocID;
		Flags    = PPTFR_SHADOW;
		if(pOrder->TFlags & tfOrdReserve)
			TFlags |= tfOrdReserve;
		SetupGoods(-labs(pOrder->GoodsID), 0);
		LotID    = pOrder->LotID;
		OrdLotID = 0; // @ordlotid
	}
}

double SLAPI PPTransferItem::GetEffCorrectionExpQtty() const
{
	double eff_qtty = 0.0;
	if(IsCorrectionExp())
		eff_qtty = fabs(QuotPrice) - fabs(Quantity_);
	else
		eff_qtty = Quantity_;
	return eff_qtty;
}

int SLAPI PPTransferItem::PreprocessCorrectionExp()
{
	int   ok = 1;
	if(IsCorrectionExp()) {
		//int SLAPI Transfer::GetOriginalValuesForCorrection(const PPTransferItem & rTi, const PPIDArray & rBillChain, double * pOrgQtty, double * pOrgPrice)
		double qtty = fabs(QuotPrice) - fabs(Quantity_);
		Flags &= ~(PPTFR_PLUS|PPTFR_MINUS);
		if(qtty <= 0.0)
			Flags |= PPTFR_MINUS;
		else
			Flags |= PPTFR_PLUS;
		Quantity_ = qtty;
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPTransferItem::Init(const BillTbl::Rec * pBillRec, int zeroRByBill, int forceSign)
{
	int    ok = 1;
	int    intr = 0, noupdlotrest = 0;
	const  int force_no_rcpt = BIN(TFlags & tfForceNoRcpt);
	TFlags = 0;
	if(pBillRec) {
		if(zeroRByBill)
			RByBill = 0;
		PPOprKind op_rec;
		const PPID op_id  = pBillRec->OpID;
		const PPID op_type_id = GetOpType(op_id, &op_rec);
		Date     = pBillRec->Dt;
		BillID   = pBillRec->ID;
		LocID    = pBillRec->LocID;
		CurID    = (int16)pBillRec->CurID;
		if(CheckOpFlags(op_id, OPKF_NOUPDLOTREST) && CConfig.Flags & CCFLG_USENOUPDRESTOPFLAG)
			noupdlotrest = 1;
		SETFLAG(Flags, PPTFR_SELLING, IsSellingOp(op_id) > 0);
		SETFLAG(Flags, PPTFR_RECEIPT, (oneof2(op_type_id, PPOPT_GOODSORDER, PPOPT_GOODSRECEIPT) ||
			(Flags & PPTFR_PCKG && Flags & PPTFR_MODIF)) && !noupdlotrest && !force_no_rcpt);
		SETFLAG(Flags, PPTFR_ACK,   op_type_id == PPOPT_GOODSACK || noupdlotrest);
		SETFLAG(Flags, PPTFR_DRAFT, oneof3(op_type_id, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT));
		if(op_type_id == PPOPT_GOODSRECEIPT) {
			SETFLAG(Flags, PPTFR_COSTWSTAX, pBillRec->Flags & BILLF_RMVEXCISE);
		}
		else if(op_type_id == PPOPT_GOODSEXPEND) {
			if(Flags & PPTFR_PCKGGEN && CheckOpFlags(op_id, OPKF_PCKGMOUNTING))
				Flags |= PPTFR_UNITEINTR;
		}
		else if(op_type_id == PPOPT_CORRECTION) {
			if(op_rec.LinkOpID && GetOpType(op_rec.LinkOpID) == PPOPT_GOODSEXPEND)
				Flags |= PPTFR_CORRECTION;
			else
				Flags |= (PPTFR_REVAL|PPTFR_CORRECTION);
		}
		if(IsIntrOp(op_id)) {
			intr = 1;
			if(IsIntrExpndOp(op_id))
				Flags |= PPTFR_UNITEINTR;
			CorrLoc = PPObjLocation::ObjToWarehouse(pBillRec->Object);
			THROW_PP(!(LocID && CorrLoc == LocID), PPERR_PRIMEQFOREIN);
		}
		else if(op_type_id == PPOPT_GOODSREVAL) {
			Flags |= PPTFR_REVAL;
			if(GetOpSubType(op_id) == OPSUBT_ASSETEXPL)
				Flags |= PPTFR_ASSETEXPL;
		}
		else if(op_type_id == PPOPT_GOODSORDER) {
			Flags |= PPTFR_ORDER;
			SETFLAG(TFlags, tfOrdReserve, CheckOpFlags(op_id, OPKF_ORDRESERVE));
		}
		else if(op_type_id == PPOPT_GOODSMODIF)
			Flags |= PPTFR_MODIF;
		else if(op_id == 0)
			Flags |= PPTFR_SHADOW;
		THROW(SetSignFlags(op_id, forceSign));
		if(pBillRec->Flags & BILLF_RECOMPLETE && Flags & PPTFR_PLUS)
			Flags &= ~PPTFR_RECEIPT;
		if(Flags & (PPTFR_ORDER | PPTFR_SHADOW) && GoodsID)
			GoodsID = -labs(GoodsID);
		THROW(SetupGoods(GoodsID, 0));
		THROW_PP(!(Flags & PPTFR_UNLIM && intr), PPERR_UNLIMINTROP);
		if(Flags & PPTFR_RECEIPT) {
			if(!(Flags & PPTFR_FORCESUPPL))
				Suppl = pBillRec->Object;
			LotDate = pBillRec->Dt;
		}
	}
	Rest_     = R6(Rest_);
	Quantity_ = R6(Quantity_);
	Price    = TR5(Price);
	Cost     = TR5(Cost);
	Discount = TR5(Discount);
	CATCHZOK
	return ok;
}

int SLAPI PPTransferItem::InitAccturnInvoice(const PPBillPacket * pPack)
{
	if(oneof2(pPack->OprType, PPOPT_ACCTURN, PPOPT_PAYMENT) || (
		pPack->OprType == PPOPT_GOODSEXPEND && pPack->Rec.Flags & BILLF_FIXEDAMOUNTS && pPack->GetTCount() == 0)) {
		PPObjGoods gobj;
		BarcodeTbl::Rec bc_rec;
		if(gobj.SearchByBarcode(CConfig.PrepayInvoiceGoodsCode, &bc_rec, 0, 0) > 0) {
			Init(&pPack->Rec, 0);
			SetupGoods(bc_rec.GoodsID);
			SETFLAG(Flags, PPTFR_RMVEXCISE, pPack->Rec.Flags & BILLF_RMVEXCISE);
			Quantity_ = 1.0;
			if(CurID)
				CurPrice = pPack->GetAmount();
			Cost  = pPack->GetBaseAmount();
			Price = pPack->GetBaseAmount();
			LotDate = pPack->Rec.Dt;
			return 1;
		}
	}
	return -1;
}

int SLAPI PPTransferItem::SetSignFlags(PPID op, int forceSign)
{
	int    ok = 1;
	const  long   f = (Flags & (PPTFR_PLUS | PPTFR_MINUS));
	const  PPID   op_type_id = GetOpType(op);
	if(op_type_id == PPOPT_GOODSMODIF) {
		// Область использования параметра forceSign {
		if(forceSign == TISIGN_UNDEF) {
			if(f) {
				if(f != (PPTFR_PLUS | PPTFR_MINUS)) {
					SETFLAG(Flags, PPTFR_RECEIPT, f & PPTFR_PLUS);
					return 1;
				}
				else
					Flags &= ~(PPTFR_PLUS | PPTFR_MINUS);
			}
			THROW_PP(forceSign || IsRecomplete(), PPERR_GMODIFITEMSIGN);
		}
		Flags &= ~(PPTFR_PLUS | PPTFR_MINUS);
		if(forceSign == TISIGN_PLUS)
			Flags |= (PPTFR_PLUS | PPTFR_RECEIPT);
		else if(forceSign == TISIGN_RECOMPLETE)
			Flags |= (PPTFR_MODIF | PPTFR_REVAL);
		else if(forceSign == TISIGN_MINUS)
			Flags |= PPTFR_MINUS;
		// }
	}
	else if(Flags & PPTFR_PCKG && Flags & PPTFR_MODIF)
		Flags |= PPTFR_PLUS;
	else if(Flags & PPTFR_CORRECTION) {
		Flags &= ~(PPTFR_PLUS | PPTFR_MINUS);
		if(forceSign == TISIGN_PLUS)
			Flags |= PPTFR_PLUS;
		else if(forceSign == TISIGN_MINUS)
			Flags |= PPTFR_MINUS;
	}
	else {
		Flags &= ~(PPTFR_PLUS | PPTFR_MINUS);
		if(op_type_id == PPOPT_DRAFTEXPEND)
			Flags |= PPTFR_MINUS;
		else if(op_type_id == PPOPT_DRAFTRECEIPT)
			Flags |= PPTFR_PLUS;
		else {
			int    r = IsExpendOp(op);
			if(r > 0)
				Flags |= PPTFR_MINUS;
			else if(r == 0)
				Flags |= PPTFR_PLUS;
		}
	}
	CATCHZOK
	return ok;
}

// static
int FASTCALL PPTransferItem::GetSign(PPID op, long flags)
{
	int    sign = TISIGN_UNDEF;
	if(PPTransferItem::IsRecomplete(flags))
		sign = TISIGN_RECOMPLETE;
	else {
		if(oneof2(op, PPOPK_EDI_STOCK, PPOPK_EDI_SHOPCHARGEON))
			sign = TISIGN_ASIS;
		else {
			const long f = (flags & (PPTFR_PLUS | PPTFR_MINUS));
			if(f == 0) {
				const int r = IsExpendOp(op);
				if(r > 0)
					sign = TISIGN_MINUS;
				else if(r == 0)
					sign = TISIGN_PLUS;
				else
					sign = TISIGN_UNDEF;
			}
			else if(f & PPTFR_MINUS)
				sign = TISIGN_MINUS;
			else if(f & PPTFR_PLUS)
				sign = TISIGN_PLUS;
			else
				sign = TISIGN_UNDEF;
		}
	}
	return sign;
}

int FASTCALL PPTransferItem::GetSign(PPID op) const
{
	return PPTransferItem::GetSign(op, Flags);
}

double FASTCALL PPTransferItem::SQtty(PPID op) const
{
	double result = 0.0;
	if(IsCorrectionExp())
		result = GetEffCorrectionExpQtty();
	else if(Flags & PPTFR_REVAL && !(Flags & PPTFR_CORRECTION)) // @v7.8.10 && !(Flags & PPTFR_CORRECTION)
		result = Rest_;
	else {
		const int _s = GetSign(op);
		// @v9.3.1 {
		if(_s == TISIGN_ASIS)
			result = Quantity_;
		else // } @v9.3.1
			result = (_s >= 0) ? fabs(Quantity_) : -fabs(Quantity_);
	}
	return result;
}

void FASTCALL PPTransferItem::SetupSign(PPID op)
{
	if(IsCorrectionExp()) {
		;
	}
	else if(!(Flags & PPTFR_REVAL) || (Flags & PPTFR_CORRECTION)) { // @v7.8.10 || (Flags & PPTFR_CORRECTION)
		Quantity_ = SQtty(op);
		if(Flags & PPTFR_INDEPPHQTTY)
			WtQtty = (GetSign(op) >= 0) ? fabs(WtQtty) : -fabs(WtQtty);
	}
}

int SLAPI PPTransferItem::SetupGoods(PPID goodsID, uint flags)
{
	int    ok = 1, ac = 0;
	GoodsID = goodsID;
	Flags  &= ~(PPTFR_ODDGOODS | PPTFR_UNLIM);
	if(goodsID) {
		PPObjGoods gobj;
		Goods2Tbl::Rec goods_rec;
		if((ok = gobj.Fetch(labs(goodsID), &goods_rec)) > 0) {
			long   f = goods_rec.Flags;
			SETFLAG(Flags, PPTFR_ODDGOODS,    f & GF_ODD);
			SETFLAG(Flags, PPTFR_UNLIM,       f & GF_UNLIM);
			SETFLAG(Flags, PPTFR_INDEPPHQTTY, f & GF_USEINDEPWT);
			if(f & GF_GENERIC) {
				CALLEXCEPT_PP_S(PPERR_INVGENERICGOODSOP, goods_rec.Name);
			}
			if(Flags & PPTFR_UNLIM) {
				if(IsUnlimWoLot())
					Flags &= ~PPTFR_RECEIPT;
				THROW_PP(!(Flags & PPTFR_REVAL), PPERR_REVALONUNLIM);
			}
			if(f & GF_AUTOCOMPL)
				ac = 1;
			if(flags & TISG_SETPWOTF)
				if(((Flags & PPTFR_RECEIPT) || (f & GF_UNLIM)) && !RByBill)
					SETFLAG(Flags, PPTFR_PRICEWOTAXES, f & GF_PRICEWOTAXES);
		}
	}
	else
		ok = -1;
	CATCHZOK
	if(!ac)
		Flags &= ~PPTFR_AUTOCOMPL;
	return ok;
}

int SLAPI PPTransferItem::SetupLot(PPID lotID, const ReceiptTbl::Rec * pLotRec, uint fl)
{
	int    ok = 1;
	ReceiptTbl::Rec lot_rec;
	Transfer * p_trfr = BillObj->trfr;
	MEMSZERO(lot_rec);
	if(lotID == 0)
		LotID = 0;
	else {
		if(!RVALUEPTR(lot_rec, pLotRec)) {
			THROW(p_trfr->Rcpt.Search(lotID, &lot_rec) > 0);
			pLotRec = &lot_rec;
		}
		LotID = pLotRec->ID;
		if(!(Flags & PPTFR_FORCESUPPL) && !(Flags & PPTFR_RECEIPT && Suppl))
			Suppl = pLotRec->SupplID;
		if(Flags & PPTFR_RECEIPT)
			LotDate = Date;
		else
			THROW(p_trfr->Rcpt.GetOriginDate(pLotRec, &LotDate));
		if(Flags & PPTFR_REVAL) {
			long oprno = MAXLONG;
			if(BillID && RByBill > 0) {
				THROW(p_trfr->SearchByBill(BillID, 0, RByBill, 0) > 0);
				oprno = p_trfr->data.OprNo+1;
			}
			THROW(p_trfr->GetRest(lotID, Date, oprno, &Rest_));
			if(Flags & PPTFR_CORRECTION) {
				QuotPrice = lot_rec.Quantity;
			}
		}
	}
	if(pLotRec) {
		if(!(fl & TISL_IGNPACK) && pLotRec->UnitPerPack > 0.0)
			UnitPerPack = pLotRec->UnitPerPack;
		if(!(fl & TISL_IGNQCERT))
			QCert = pLotRec->QCertID;
		if(!(fl & TISL_IGNEXPIRY) && (!Expiry || !TESTFLAG(Flags, Flags, PPTFR_DRAFT|PPTFR_PLUS)))
			Expiry = pLotRec->Expiry;
		if(!(Flags & PPTFR_RECEIPT))
			LotTaxGrpID = pLotRec->InTaxGrpID;
		if(!(Flags & PPTFR_RECEIPT)) {
			SETFLAG(Flags, PPTFR_COSTWOVAT, pLotRec->Flags & LOTF_COSTWOVAT);
			SETFLAG(Flags, PPTFR_COSTWSTAX, pLotRec->Flags & LOTF_COSTWSTAX);
			SETFLAG(Flags, PPTFR_PCKG,      pLotRec->Flags & LOTF_PCKG);
			SETFLAG(Flags, PPTFR_PRICEWOTAXES, pLotRec->Flags & LOTF_PRICEWOTAXES);
		}
	   	if(!(fl & TISL_IGNCOST) || !(fl & TISL_IGNPRICE) || IsLotRet()) {
			THROW(p_trfr->GetLotPrices(&lot_rec, Date));
			double c = R5(lot_rec.Cost);
			double p = R5(lot_rec.Price);
			if(Flags & PPTFR_REVAL) {
				RevalCost = c;
				Discount = p;
			}
			if(IsLotRet()) {
	   		    Cost = c;
				if(!IsCorrectionExp()) { // @v9.4.3
					if(!(Flags & PPTFR_MODIF && Flags & PPTFR_PLUS) && Price >= 0.0)
						Discount += (p - Price);
					else
						Discount = 0.0;
	   				Price = p;
				}
			}
			else {
				if(!(fl & TISL_IGNCOST))
					Cost = c;
				if(!(fl & TISL_IGNPRICE)) {
					if(fl & TISL_ADJPRICE && Price > 0.0)
					   	Discount += (p - Price);
					Price = p;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

PPTransferItem & FASTCALL PPTransferItem::operator = (const PPTransferItem & src)
{
	memcpy(this, &src, sizeof(*this));
	return *this;
}

#if 0 // @v8.0.3 {
int FASTCALL PPTransferItem::IsModified(const PPTransferItem * p) const
{
#define RETIFNEQ(f) if((f)!=p->f) return 1
	RETIFNEQ(Date);
	RETIFNEQ(BillID);
	RETIFNEQ(RByBill);
	RETIFNEQ(CurID);  // @v8.0.3
	RETIFNEQ(LocID);
	RETIFNEQ(GoodsID);
	RETIFNEQ(LotID);
	RETIFNEQ(CorrLoc);
	RETIFNEQ(UnitPerPack);
	RETIFNEQ(Quantity_);
	RETIFNEQ(WtQtty);
	RETIFNEQ(Cost);
	RETIFNEQ(Price);
	RETIFNEQ(Discount);
	RETIFNEQ(QCert);
	RETIFNEQ(Expiry);
	RETIFNEQ(LocTransfTm);
	if((Flags & ~PPTFR_UNITEINTR) != (p->Flags & ~PPTFR_UNITEINTR))
		return 1;
	if(Flags & PPTFR_REVAL) {
		RETIFNEQ(Rest_);
		RETIFNEQ(RevalCost);
	}
	if(Flags & PPTFR_RECEIPT) {
		RETIFNEQ(Suppl);
		RETIFNEQ(LotTaxGrpID);
		RETIFNEQ(ExtCost);
	}
#undef  RETIFNEQ
	return 0;
}
#endif // } 0 @v8.0.3

int FASTCALL PPTransferItem::IsEqual(const PPTransferItem & rS) const
{
#define RETIFNEQ(f) if((f)!=rS.f) return 0
	RETIFNEQ(Date);
	RETIFNEQ(BillID);
	RETIFNEQ(RByBill);
	RETIFNEQ(CurID);  // @v8.0.3
	RETIFNEQ(LocID);
	RETIFNEQ(GoodsID);
	RETIFNEQ(LotID);
	RETIFNEQ(CorrLoc);
	RETIFNEQ(UnitPerPack);
	RETIFNEQ(Quantity_);
	RETIFNEQ(WtQtty);
	RETIFNEQ(Cost);
	RETIFNEQ(Price);
	RETIFNEQ(Discount);
	RETIFNEQ(QCert);
	RETIFNEQ(Expiry);
	RETIFNEQ(LocTransfTm); // @v8.0.3
	if((Flags & ~PPTFR_UNITEINTR) != (rS.Flags & ~PPTFR_UNITEINTR))
		return 0;
	if(Flags & PPTFR_REVAL) {
		RETIFNEQ(Rest_);
		RETIFNEQ(RevalCost);
	}
	if(Flags & PPTFR_RECEIPT) {
		RETIFNEQ(Suppl);
		RETIFNEQ(LotTaxGrpID);
		RETIFNEQ(ExtCost);
	}
#undef  RETIFNEQ
	return 1;
}

double FASTCALL PPTransferItem::CalcAmount(int zeroCost) const
{
	double v;
	if(Flags & PPTFR_REVAL) {
		if(Flags & PPTFR_CORRECTION)
			v = (Cost * (Quantity_ - QuotPrice));
		else
			v = ((Price - Discount) * Rest_);
	}
	else if(IsCorrectionExp()) {
		const double q = GetEffCorrectionExpQtty();
		v = q * ((Flags & PPTFR_SELLING) ? (Price - Discount) : (zeroCost ? 0.0 : Cost));
	}
	else {
		const double q = (Flags & PPTFR_MODIF) ? SQtty(0) : fabs(Quantity_);
		v = q * ((Flags & PPTFR_SELLING) ? (Price - Discount) : (zeroCost ? 0.0 : Cost));
	}
	return R2(v);
}

int SLAPI PPTransferItem::SetupQuot(double quot, int set)
{
	int    ok = 1;
	if(Flags & PPTFR_CORRECTION) {
		ok = -1;
	}
	else if(set) {
		Flags |= PPTFR_QUOT;
		QuotPrice = quot;
	}
	else {
		Flags &= ~PPTFR_QUOT;
		QuotPrice = 0.0;
	}
	return ok;
}

double SLAPI PPTransferItem::RoundPrice(double price, double roundPrec, int roundDir, long flags) const
{
	if(price > 0.0) {
		CalcPriceParam param;
		param.GoodsID    = GoodsID;
		param.InTaxGrpID = LotTaxGrpID;
		param.Dt = Date;
		param.Cost = Cost;   // @unused
		param.Price = Price; // @unused
		param.RoundPrec = roundPrec;
		param.RoundDir  = roundDir;
		SETFLAG(param.Flags, CalcPriceParam::fCostWoVat, Flags & PPTFR_COSTWOVAT);
		SETFLAG(param.Flags, CalcPriceParam::fRoundVat, flags & valfRoundVat);
		price = param.Calc(price, 0, 0, 0);
	}
	return price;
}

int SLAPI PPTransferItem::Valuation(const PPBillConfig & rCfg, int calcOnly, double * pResult)
{
	int    ok = -1;
	double new_price = 0.0;
	if(rCfg.ValuationQuotKindID) {
		double _cost = Cost;
		QuotIdent qi(QIDATE(Date), LocID, rCfg.ValuationQuotKindID, 0, Suppl);
		PPObjGoods goods_obj;
		const PPID goods_id = labs(GoodsID);
		if(rCfg.Flags & BCF_VALUATION_BYCONTRACT && DS.GetConstTLA().SupplDealQuotKindID) {
			QuotIdent qic(QIDATE(Date), LocID, DS.GetConstTLA().SupplDealQuotKindID, 0, Suppl);
			double c = 0.0;
			if(goods_obj.GetQuotExt(goods_id, qic, _cost, Price, &c, 1) > 0 && c > 0.0)
				_cost = c;
		}
		int    r = goods_obj.GetQuotExt(goods_id, qi, _cost, Price, &new_price, 1);
		THROW(r);
		if(r > 0 && new_price > 0.0) {
			long   flags = (rCfg.Flags & BCF_VALUATION_RNDVAT) ? PPTransferItem::valfRoundVat : 0;
			new_price = RoundPrice(new_price, rCfg.ValuationRndPrec, rCfg.ValuationRndDir, flags);
			if(!calcOnly) {
				if((Flags & (PPTFR_RECEIPT|PPTFR_UNITEINTR)) || (Flags & PPTFR_DRAFT && Flags & PPTFR_PLUS)) {
					Price = new_price;
					Flags |= PPTFR_QUOT;
				}
			}
			ok = 1;
		}
		else
			new_price = 0.0;
	}
	CATCHZOK
	ASSIGN_PTR(pResult, new_price);
	return ok;
}

#if 0 // {
int SLAPI PPTransferItem::Valuation(PPID quotKindID, double roundPrec, int roundDir, long flags, double * pResult)
{
	int    ok = -1;
	QuotIdent qi(LocID, quotKindID, 0, Suppl);
	double new_price = 0.0;
	PPObjGoods goods_obj;
	int    r = goods_obj.GetQuotExt(GoodsID, &qi, Cost, Price, &new_price, 1);
	THROW(r);
	if(r > 0 && new_price > 0.0) {
		new_price = RoundPrice(new_price, roundPrec, roundDir, flags);
		if(!(flags & valfCalcOnly)) {
			if((Flags & (PPTFR_RECEIPT|PPTFR_UNITEINTR)) || (Flags & PPTFR_DRAFT && Flags & PPTFR_PLUS)) {
				Price = new_price;
				Flags |= PPTFR_QUOT;
			}
		}
		ok = 1;
	}
	else
		new_price = 0.0;
	CATCHZOK
	ASSIGN_PTR(pResult, new_price);
	return ok;
}
#endif // } 0

double SLAPI PPTransferItem::NetPrice() const
{
	return (Price - Discount);
}

double SLAPI PPTransferItem::Qtty() const
{
	return (Flags & PPTFR_REVAL && !(Flags & PPTFR_CORRECTION)) ? Rest_ : Quantity_;
}

double SLAPI PPTransferItem::GetOrgCost() const
{
	return (Cost - ExtCost);
}

void SLAPI PPTransferItem::SetOrgCost(double c)
{
	Cost = TR5(c + ExtCost);
}

void SLAPI PPTransferItem::SetZeroCost()
{
	Cost = ExtCost = 0.0;
}

double SLAPI PPTransferItem::CalcCurAmount() const
{
	double v;
	if(Flags & PPTFR_REVAL && !(Flags & PPTFR_CORRECTION))
		v = CurPrice * Rest_;
	else
		v = CurPrice * ((Flags & PPTFR_MODIF) ? SQtty(0) : fabs(Quantity_));
	return R2(v);
}

void FASTCALL PPTransferItem::ConvertMoney(TransferTbl::Rec * pRec) const
{
	if(Flags & PPTFR_REVAL) {
		pRec->Cost  = TR5(RevalCost);
		pRec->Price = TR5(Discount);
		pRec->Discount = 0.0;
	}
	else {
		pRec->Cost  = TR5(Cost);
		pRec->Price = TR5(Price);
		pRec->Discount = TR5(Discount);
	}
	pRec->CurPrice = TR5(CurPrice);
	pRec->QuotPrice = TR5(QuotPrice);
}
