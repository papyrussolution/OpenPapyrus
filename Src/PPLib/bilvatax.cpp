// BILVATAX.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2005, 2006, 2007, 2009, 2010, 2015, 2016, 2017, 2018, 2019, 2020, 2025
// @codepage UTF-8
// Расчет НДС по документам, соответствующим заданному запросу
//
#include <pp.h>
#pragma hdrstop

IMPL_CMPFUNC(BVATAccm, p1, p2)
{
	int    r = cmp_long(static_cast<const BVATAccm *>(p2)->IsVatFree, static_cast<const BVATAccm *>(p1)->IsVatFree); // descending order
	return NZOR(r, cmp_double(static_cast<const BVATAccm *>(p1)->PRate, static_cast<const BVATAccm *>(p2)->PRate));
}

IMPL_CMPFUNC(BVATAccm_DiffByCVat, p1, p2)
{
	int    r = cmp_long(static_cast<const BVATAccm *>(p2)->IsVatFree, static_cast<const BVATAccm *>(p1)->IsVatFree); // descending order
	return NZOR(r, cmp_double(static_cast<const BVATAccm *>(p1)->CRate, static_cast<const BVATAccm *>(p2)->CRate));
}

BVATAccm::BVATAccm()
{
	THISZERO();
}

BVATAccmArray::BVATAccmArray(uint aFlags) : TSVector <BVATAccm> (), Flags(aFlags)
{
}

BVATAccmArray::BVATAccmArray(const BVATAccmArray & rS) : TSVector <BVATAccm>(rS), Flags(rS.Flags)
{
}

int BVATAccmArray::Add(const BVATAccm * pItem, int dontRound)
{
	int    ok = 1;
	uint   p = 0;
	BVATAccm item = *pItem;
	if(!dontRound) {
		item.Cost     = R2(item.Cost);
		item.Price    = R2(item.Price);
		item.CVATSum  = R2(item.CVATSum);
		item.PVATSum  = R2(item.PVATSum);
		item.PTrnovr  = R2(item.PTrnovr);
		item.Discount = R2(item.Discount);
	}
	CompFunc cfunc = (Flags & BVATF_DIFFBYCRATE) ? PTR_CMPFUNC(BVATAccm_DiffByCVat) : PTR_CMPFUNC(BVATAccm);
	if(bsearch(&item, &p, cfunc)) {
		BVATAccm & r_item = at(p);
		r_item.Cost     += item.Cost;
		r_item.Price    += item.Price;
		r_item.CVATSum  += item.CVATSum;
		r_item.PVATSum  += item.PVATSum;
		r_item.PTrnovr  += item.PTrnovr;
		r_item.Discount += item.Discount;
	}
	else if(!ordInsert(&item, 0, cfunc))
		ok = PPSetErrorSLib();
	return ok;
}

int BVATAccmArray::Add(const PPTransferItem & rTi, PPID opID)
{
	int    ok = 1;
	BVATAccm item;
	GTaxVect gtv;
	item.IsVatFree = IsVataxableSuppl(rTi.Suppl) ? 0 : 1;
	if(item.IsVatFree && Flags & BVATF_SUMZEROVAT) {
	   	gtv.CalcTI(rTi, opID, TIAMT_PRICE);
		item.PRate   = gtv.GetTaxRate(GTAX_VAT, 0);
		item.Cost    = 0.0;
		item.CVATSum = 0.0;
	   	item.Price   = gtv.GetValue(GTAXVF_AFTERTAXES | GTAXVF_VAT | GTAXVF_EXCISE);
		item.PVATSum = gtv.GetValue(GTAXVF_VAT);
		THROW(Add(&item));
		gtv.CalcTI(rTi, opID, TIAMT_COST, GTAXVF_VAT);
		item.CRate   = gtv.GetTaxRate(GTAX_VAT, 0);
		item.Cost    = gtv.GetValue(GTAXVF_AFTERTAXES | GTAXVF_VAT);
		item.CVATSum = gtv.GetValue(GTAXVF_VAT);
	   	item.Price   = 0.0;
		item.PVATSum = 0.0;
		THROW(Add(&item));
	}
	else {
	   	gtv.CalcTI(rTi, opID, TIAMT_PRICE);
		item.PRate   = gtv.GetTaxRate(GTAX_VAT, 0);
		item.Price   = gtv.GetValue(GTAXVF_AFTERTAXES | GTAXVF_VAT | GTAXVF_EXCISE);
		item.PVATSum = gtv.GetValue(GTAXVF_VAT);
		gtv.CalcTI(rTi, opID, TIAMT_COST);
		item.CRate   = gtv.GetTaxRate(GTAX_VAT, 0);
		item.Cost    = gtv.GetValue(GTAXVF_AFTERTAXES | GTAXVF_VAT);
		item.CVATSum = gtv.GetValue(GTAXVF_VAT);
		THROW(Add(&item));
	}
	CATCHZOK
	return ok;
}

void BVATAccmArray::Scale_(double part, int useRounding)
{
	BVATAccm * p_item;
	if(part != 1.0) {
		for(uint i = 0; enumItems(&i, (void **)&p_item);) {
			p_item->Cost    = p_item->Cost    * part;
			p_item->Price   = p_item->Price   * part;
			p_item->CVATSum = p_item->CVATSum * part;
			p_item->PVATSum = p_item->PVATSum * part;
			if(useRounding) {
				p_item->Cost    = R2(p_item->Cost);
				p_item->Price   = R2(p_item->Price);
				p_item->CVATSum = R2(p_item->CVATSum);
				p_item->PVATSum = R2(p_item->PVATSum);
			}
		}
	}
}

int BVATAccmArray::CalcBill(const PPBillPacket & rPack)
{
	int    ok = 1;
	bool   inited = false;
	const  PPID op_type_id = GetOpType(rPack.Rec.OpID);
	//
	// Условие (!pPack->GetTCount() && pPack->Rec.Flags & BILLF_BILLF_FIXEDAMOUNTS)
	// введено для обработки документов консолидирующей передачи из других разделов.
	//
	if(oneof2(op_type_id, PPOPT_ACCTURN, PPOPT_PAYMENT) || (!rPack.GetTCount() && rPack.Rec.Flags & BILLF_FIXEDAMOUNTS)) {
		PPObjAmountType amtt_obj;
		PPAmountType amtt_rec;
		AmtEntry * p_ae;
		double amt = rPack.GetBaseAmount();
		int    num_vat_rates = 0;
		bool   is_there_stax = false;
		{
			for(uint i = 0; rPack.Amounts.enumItems(&i, (void **)&p_ae);) {
				if(p_ae->Amt != 0.0 && amtt_obj.Fetch(p_ae->AmtTypeID, &amtt_rec) > 0)
					if(amtt_rec.IsTax(GTAX_VAT))
						num_vat_rates++;
					else if(!is_there_stax && amtt_rec.IsTax(GTAX_SALES)) {
						amt -= p_ae->Amt;
						is_there_stax = true;
					}
			}
		}
		if(num_vat_rates) {
			for(uint i = 0; rPack.Amounts.enumItems(&i, (void **)&p_ae);) {
				if(p_ae->Amt != 0.0 && amtt_obj.Fetch(p_ae->AmtTypeID, &amtt_rec) > 0) {
					if(amtt_rec.IsTax(GTAX_VAT)) {
						BVATAccm item;
						if(amtt_rec.TaxRate) {
							const double rate = fdiv100i(amtt_rec.TaxRate);
							const double vat = p_ae->Amt;
							const double a = (num_vat_rates > 1) ? (vat / SalesTaxMult(rate)) : amt;
							item.IsVatFree = 0;
							item.PRate   = item.CRate   = rate;
							item.CVATSum = item.PVATSum = vat;
							item.Cost    = item.Price   = a;
						}
						THROW(Add(&item));
						inited = true;
					}
				}
			}
		}
	}
	if(!inited) {
		for(uint i = 0; i < rPack.GetTCount(); i++) {
			const PPTransferItem & r_ti = rPack.ConstTI(i);
			THROW(Add(r_ti, rPack.Rec.OpID));
			inited = true;
		}
	}
	ok = inited ? 1 : -1;
	CATCHZOK
	return ok;
}

int BVATAccmArray::CalcBill(PPID id)
{
	int    r = -1;
	PPObjBill * p_bobj(BillObj);
	PPTransferItem ti;
	BillTbl::Rec bill_rec;
	if(p_bobj->Search(id, &bill_rec) > 0) {
		for(int rbybill = 0; (r = p_bobj->trfr->EnumItems(id, &rbybill, &ti)) > 0;)
			if(!Add(ti, bill_rec.OpID))
				return 0;
	}
	return BIN(r);
}

int BVATAccmArray::CalcCCheckLineArray(const CCheckLineArray & rList)
{
	int    ok = 1;
	const  uint _c = rList.getCount();
	if(_c) {
		const  LDATE now_date = getcurdate_();
		for(uint i = 0; i < _c; i++) {
			const CCheckLineTbl::Rec & r_ln_rec = rList.at(i);
			const double ln_p = intmnytodbl(r_ln_rec.Price);
			const double ln_q = r_ln_rec.Quantity;
			{
				BVATAccm bva_item;
				PPGoodsTaxEntry gtx;
				if(GObj.FetchTaxEntry2(r_ln_rec.GoodsID, 0/*lotID*/, 0/*taxPayerID*/, now_date, 0L, &gtx) > 0) // @v12.2.3 LConfig.OperDate-->now_date
					bva_item.PRate = gtx.GetVatRate();
				bva_item.PTrnovr  += ln_q * (ln_p - r_ln_rec.Dscnt);
				bva_item.Discount += ln_q * r_ln_rec.Dscnt;
				THROW(Add(&bva_item));
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;	
}

int BVATAccmArray::IsVataxableSuppl(PPID suppl) { return (!(Flags & BVATF_IGNORESUPPL) && suppl && IsSupplVATFree(suppl) > 0) ? 0 : 1; }
