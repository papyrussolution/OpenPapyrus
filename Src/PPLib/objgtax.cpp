// OBJGTAX.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2009, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

PPGoodsTaxEntry::PPGoodsTaxEntry()
{
	THISZERO();
}

int FASTCALL PPGoodsTaxEntry::IsEq(const PPGoodsTaxEntry & rS) const
{
#define CMP_FLD(f) if(f != rS.f) return 0;
	CMP_FLD(TaxGrpID);
	CMP_FLD(Period);
	CMP_FLD(OpID);
	CMP_FLD(VAT);
	CMP_FLD(Excise);
	CMP_FLD(SalesTax);
	CMP_FLD(Flags);
	CMP_FLD(Order);
	CMP_FLD(UnionVect);
	return 1;
#undef CMP_FLD
}

double PPGoodsTaxEntry::GetVatRate() const { return fdiv100i(VAT); }

char * PPGoodsTaxEntry::FormatVAT(char * pBuf, size_t bufLen) const
{
	const  long fmt = MKSFMTD(0, 2, NMBF_NOTRAILZ | ALIGN_LEFT | NMBF_NOZERO);
	char   str[32];
	realfmt(fdiv100i(VAT), fmt, str); // @divtax
	return strnzcpy(pBuf, str, bufLen);
}

char * PPGoodsTaxEntry::FormatSTax(char * pBuf, size_t bufLen) const
{
	const  long fmt = MKSFMTD(0, 2, NMBF_NOTRAILZ | ALIGN_LEFT | NMBF_NOZERO);
	char   str[32];
	realfmt(fdiv100i(SalesTax), fmt, str); // @divtax
	return strnzcpy(pBuf, str, bufLen);
}

char * PPGoodsTaxEntry::FormatExcise(char * pBuf, size_t bufLen) const
{
	SString temp_buf;
	temp_buf.Cat(fdiv100i(Excise), MKSFMTD(0, 2, NMBF_NOTRAILZ|ALIGN_LEFT|NMBF_NOZERO)); // @divtax
	if(Excise && Flags & GTAXF_ABSEXCISE)
		temp_buf.CatChar('$');
	return temp_buf.CopyTo(pBuf, bufLen);
}

PPGoodsTax2::PPGoodsTax2()
{
	THISZERO();
}

void FASTCALL PPGoodsTax::ToEntry(PPGoodsTaxEntry * pEntry) const
{
	if(pEntry) {
		pEntry->TaxGrpID = ID;
		pEntry->VAT      = R0i(VAT * 100L);  // @divtax
		pEntry->Excise   = R0i(Excise   * 100L);  // @divtax
		pEntry->SalesTax = R0i(SalesTax * 100L);  // @divtax
		pEntry->Flags = Flags;
		pEntry->Order = Order;
		pEntry->UnionVect = UnionVect;
	}
}

void FASTCALL PPGoodsTax::FromEntry(const PPGoodsTaxEntry * pEntry)
{
	//pEntry->TaxGrpID = ID;
	VAT      = fdiv100i(pEntry->VAT);      // @divtax
	Excise   = fdiv100i(pEntry->Excise);   // @divtax
	SalesTax = fdiv100i(pEntry->SalesTax); // @divtax
	Flags = pEntry->Flags;
	Order = pEntry->Order;
	UnionVect = pEntry->UnionVect;
}

PPGoodsTaxPacket::PPGoodsTaxPacket() : SVector(sizeof(PPGoodsTaxEntry))
{
}

PPGoodsTaxPacket & FASTCALL PPGoodsTaxPacket::operator = (const PPGoodsTaxPacket & s)
{
	Rec = s.Rec;
	copy(s);
	return *this;
}

uint PPGoodsTaxPacket::GetCount() const { return SVector::getCount(); }
PPGoodsTaxEntry & FASTCALL PPGoodsTaxPacket::Get(uint idx) const { return *static_cast<PPGoodsTaxEntry *>(SVector::at(idx)); }
int  FASTCALL PPGoodsTaxPacket::Insert(const PPGoodsTaxEntry & rEntry) { return SVector::insert(&rEntry) ? 1 : PPSetErrorSLib(); }
SVector * PPGoodsTaxPacket::vecptr() { return this; }

int PPGoodsTaxPacket::PutEntry(int pos, const PPGoodsTaxEntry * pEntry)
{
	int    ok = 1;
	if(pEntry) {
		long   max_idx = 0;
		for(uint i = 0; i < getCount(); i++) {
			const PPGoodsTaxEntry & r_item = Get(i);
			if(pos != (int)i && pEntry->Period.IsIntersect(r_item.Period))
				if(pEntry->OpID == r_item.OpID)
					return PPSetError(PPERR_GTAXENTINTRSCT);
			long idx = ((r_item.TaxGrpID & 0xff000000L) >> 24);
			max_idx = MAX(max_idx, idx);
		}
		if(pos < 0 || pos >= static_cast<int>(getCount())) {
			PPGoodsTaxEntry item = *pEntry;
			item.TaxGrpID &= 0x00ffffffL;
			item.TaxGrpID |= ((max_idx+1) << 24);
			THROW_SL(insert(&item));
		}
		else {
			Get(pos) = *pEntry;
		}
	}
	else {
		assert(pos >= 0 && pos < static_cast<int>(getCount()));
		THROW_SL(atFree(static_cast<uint>(pos)));
	}
	CATCHZOK
	return ok;
}

IMPL_CMPFUNC(PPGoodsTaxEntry, i1, i2)
{
	const PPGoodsTaxEntry * p_i1 = static_cast<const PPGoodsTaxEntry *>(i1);
	const PPGoodsTaxEntry * p_i2 = static_cast<const PPGoodsTaxEntry *>(i2);
	int    r = cmp_long((p_i1->TaxGrpID & 0x00ffffffL), (p_i2->TaxGrpID & 0x00ffffffL));
	r = NZOR(r, cmp_ulong(p_i1->Period.low, p_i2->Period.low));
	return NZOR(r, cmp_long(p_i1->OpID, p_i2->OpID));
}

void PPGoodsTaxPacket::Sort()
{
	sort(PTR_CMPFUNC(PPGoodsTaxEntry));
}
//
//
//
GTaxVect::GTaxVect(int roundPrec) : RoundPrec(roundPrec)
{
}

int FASTCALL GTaxVect::TaxToVect(int taxIdx) const
{
	if(taxIdx == 0)
		return 0;
	for(int i = 1; i <= N; i++)
		if(OrderVect[i] == static_cast<long>(taxIdx))
			return i;
	return -1;
}

int FASTCALL GTaxVect::VectToTax(int idx) const
{
	return (idx > 0 && idx <= N) ? static_cast<int>(OrderVect[idx]) : (idx ? -1 : 0);
}

double GTaxVect::GetTaxRate(long taxID, int * pIsAbs) const
{
	int    is_abs = 0;
	double rate = 0.0;
	if(taxID >= 1 && taxID <= N) {
		int    v = TaxToVect((int)taxID);
		if(AbsVect & (1 << v)) {
			is_abs = 1;
			rate = RateVect[v];
		}
		else {
			is_abs = 0;
			rate = (RateVect[v] * 100L);
		}
	}
	ASSIGN_PTR(pIsAbs, is_abs);
	return rate;
}

double FASTCALL GTaxVect::GetValue(long flags) const
{
	if(flags & GTAXVF_BEFORETAXES)
		return Amount;
	else {
		double amount = 0.0;
		for(int i = 0; i <= N; i++)
			if(flags & (1 << i))
				amount += Vect[TaxToVect(i)];
		return amount;
	}
}

double GTaxVect::CalcTaxValByBase(int idx, double base) const
{
	const double v = (AbsVect & (1 << idx)) ? Qtty : base;
	return round(RateVect[idx] * v, RoundPrec);
}
//
// amount - сумма, включающая налоги 1..n (n<=N)
// Расчитывает сумму без налогов (AmountAfterTaxes) GTaxVect::Vect[0]
//
void GTaxVect::CalcForward(int n, double amount)
{
	Vect[0] = amount;
	double prev_sum = 0.0;
	for(int i = n; i >= 1; i--) {
		double rate = RateVect[i];
		if(UnionVect & (1 << i)) {
			int    j;
			double base_amt = 0.0;
			for(j = i; j > 0 && (UnionVect & (1 << j)); j--)
				if(!(AbsVect & (1 << i)))
					rate += RateVect[j-1];
			base_amt  = round((1 - 1 / (1 + 1 / rate)) * (amount - prev_sum), RoundPrec);
			Vect[i]   = CalcTaxValByBase(i, base_amt);
			prev_sum += Vect[i];
			Vect[0]  -= Vect[i];
			for(j = i; j > 0 && (UnionVect & (1 << j)); j--) {
				Vect[j-1] = CalcTaxValByBase(j-1, base_amt);
				prev_sum += Vect[j-1];
				Vect[0]  -= Vect[j-1];
			}
			i = j;
		}
		else {
			if(AbsVect & (1 << i))
				Vect[i] = rate * Qtty;
			else if(rate == 0)
				Vect[i] = 0;
			else
				Vect[i] = round(((amount-prev_sum) * rate) / (1+rate), RoundPrec);
			prev_sum += Vect[i];
			Vect[0]  -= Vect[i];
		}
	}
}
//
// amount - сумма, не включающая налоги (n+1)..N (1 <= n <= N)
// Расчитывает полную сумму со всеми налогами GTaxVect::Amount
//
void GTaxVect::CalcBackward(int n, double amount)
{
	Amount = round(amount, RoundPrec);
	double prev_sum = 0.0;
	for(int i = n; i <= N; i++) {
		double base_amt = amount + prev_sum;
		for(int j = i; j > 0 && (UnionVect & (1 << j)); j--)
			base_amt -= Vect[j-1];
		Vect[i] = CalcTaxValByBase(i, base_amt);
		prev_sum += Vect[i];
		Amount   += Vect[i];
	}
}

void GTaxVect::Calc_(PPGoodsTaxEntry * gtax, double amount, double qtty, long amtFlags, long excludeFlags)
{
	int    i;
	amount = round(amount, RoundPrec);
	Qtty = qtty;
	N = 3;
	memzero(Vect,      sizeof(Vect));
	memzero(RateVect,  sizeof(RateVect));
	memzero(OrderVect, sizeof(OrderVect));
	Amount    = 0.0;
	AbsVect   = 0;
	UnionVect = 0;
	SETIFZ(gtax->Order, PPObjGoodsTax::GetDefaultOrder());
	GetNthTransposition(OrderVect+1, N, gtax->Order);
	UnionVect = (gtax->UnionVect << 1);
	for(i = 1; i <= N; i++) {
		switch(OrderVect[i]) {
			case GTAX_VAT:
				if(!(excludeFlags & GTAXVF_VAT))
					RateVect[i] = static_cast<double>(gtax->VAT) / (100.0 * 100.0); // @divtax
				break;
			case GTAX_EXCISE:
				if(!(excludeFlags & GTAXVF_EXCISE))
					if(gtax->Flags & GTAXF_ABSEXCISE) {
						RateVect[i] = fdiv100i(gtax->Excise); // @divtax
						AbsVect |= (1 << i);
					}
					else
						RateVect[i] = static_cast<double>(gtax->Excise) / (100.0 * 100.0); // @divtax
				break;
			case GTAX_SALES:
				if(!(excludeFlags & GTAXVF_SALESTAX))
					RateVect[i] = static_cast<double>(gtax->SalesTax) / (100.0 * 100.0);   // @divtax
				break;
		}
	}
	if(oneof2(amtFlags, GTAXVF_BEFORETAXES, ~0L)) { // Base as Amount
		Amount = amount;
		CalcForward((int)N, amount);
	}
	else if(oneof2(amtFlags, GTAXVF_AFTERTAXES, 0)) { // Base as Amount_after_taxes
		Vect[0] = amount;
		CalcBackward(1, amount);
	}
	else {
		int    done = 0;
		// Сумма не включает некоторые налоги
		// Если сумма не включает налог n, то она не включает и
		// налоги с индексами > n
		if(amtFlags & GTAXVF_BEFORETAXES) {
			for(i = 1; !done && i <= N; i++) {
				if(!(amtFlags & (1 << VectToTax(i)))) {
					CalcForward(i-1, amount);
					CalcBackward(i, amount);
					done = 1;
				}
			}
			if(!done) {
				//
				// Эквивалент GTAXVF_BEFORETAXES
				//
				Amount = amount;
				CalcForward((int)N, amount);
			}
		}
		else {
			//
			// Сумма включает некоторые налоги
			// Если сумма включает налог n, то она включает и
			// налоги с индексами < n
			//
			for(i = (int)N; !done && i >= 1; i--) {
				if(amtFlags & (1 << VectToTax(i))) {
					CalcForward(i, amount);
					CalcBackward(i+1, amount);
					done = 1;
				}
			}
			if(!done) {
				//
				// Эквивалент GTAXVF_AFTERTAXES
				//
				Vect[0] = amount;
				CalcBackward(1, amount);
			}
		}
	}
}

int GTaxVect::CalcTI(const PPTransferItem & rTi, PPID opID, int tiamt, long exclFlags, int correctionFlag/*= 0*/)
{
	int    ok = 1;
	const  PPCommConfig & r_ccfg = CConfig;
	PPObjBill * p_bobj = BillObj;
	const  int calcti_costwovat_byprice = (r_ccfg.Flags & CCFLG_COSTWOVATBYSUM) ? 0 : 1;
	const  LDATE date_of_relevance = checkdate(rTi.LotDate) ? rTi.LotDate : rTi.Date; // @v10.5.8 Драфт-документы имеют нулевой LotDate
	int    cost_wo_vat = 0;
	int    is_asset = 0;
	int    is_exclvat = 0;
	int    reval_assets = 0;
	long   amt_flags = ~0L;
	PPID   tax_grp_id = 0;
	PPGoodsTaxEntry gtx;
	PPObjGoods    gobj;
	Goods2Tbl::Rec goods_rec;
	double amount = 0.0;
	double qtty   = fabs(rTi.Qtty());
	double tax_qtty = qtty;
	if(gobj.Fetch(labs(rTi.GoodsID), &goods_rec) > 0) {
		if(goods_rec.Flags & GF_ASSETS)
			is_asset = 1;
		tax_grp_id = goods_rec.TaxGrpID;
		if(!(exclFlags & GTAXVF_NOMINAL) && r_ccfg.DynGoodsTypeForSupplAgent && rTi.LotID && p_bobj->GetSupplAgent(rTi.LotID) > 0) {
			PPObjGoodsType gt_obj;
			PPGoodsType gt_rec;
			if(gt_obj.Fetch(r_ccfg.DynGoodsTypeForSupplAgent, &gt_rec) > 0 && gt_rec.Flags & GTF_EXCLVAT)
				is_exclvat = 1;
		}
		else if(goods_rec.Flags & GF_EXCLVAT)
			is_exclvat = 1;
	}
	else
		MEMSZERO(goods_rec);
	if(!(exclFlags & GTAXVF_NOMINAL) && PPObjLocation::CheckWarehouseFlags(rTi.LocID, LOCF_VATFREE))
		exclFlags |= GTAXVF_VAT;
	if(rTi.IsCorrectionRcpt()) {
		if(!(exclFlags & GTAXVF_NOMINAL) && rTi.LotTaxGrpID)
			tax_grp_id = rTi.LotTaxGrpID;
		gobj.GTxObj.Fetch(tax_grp_id, date_of_relevance, 0, &gtx);
		const double q_pre = fabs(rTi.QuotPrice);
		qtty = fabs(rTi.Quantity_);
		const double q_diff = (qtty - q_pre);
		const double cq = R2(rTi.Cost * qtty - rTi.RevalCost * q_pre);
		const double pq = R2(rTi.Price * qtty - rTi.Discount * q_pre);
		if(q_diff != 0.0) {
			qtty = q_diff;
		}
		else {
			;
		}
		amount = ((tiamt != TIAMT_PRICE) ? cq : pq) / qtty;
	}
	else if(rTi.IsCorrectionExp()) {
		BillTbl::Rec bill_rec;
		BillTbl::Rec link_bill_rec;
		LDATE   tax_date = rTi.Date;
		if(p_bobj->Fetch(rTi.BillID, &bill_rec) > 0 && bill_rec.LinkBillID && p_bobj->Fetch(bill_rec.LinkBillID, &link_bill_rec) > 0)
			tax_date = link_bill_rec.Dt;
		gobj.GTxObj.Fetch(tax_grp_id, tax_date, 0, &gtx);
		const double q_pre = fabs(rTi.QuotPrice);
		qtty = fabs(rTi.Quantity_);
		const double q_diff = (qtty - q_pre);
		if(correctionFlag == 0) { // @v11.7.0
			if(tiamt == TIAMT_COST) {
				const double pq = R2(rTi.Cost * (qtty - q_pre));
				if(q_diff != 0.0)
					qtty = q_diff;
				amount = pq / qtty;
			}
			else {
				const double pq = R2(rTi.NetPrice() * qtty - rTi.RevalCost * q_pre);
				if(q_diff != 0.0)
					qtty = q_diff;
				amount = pq / qtty;
			}
		}
		// @v11.7.0 {
		else if(correctionFlag < 0) {
			qtty = q_pre;
			if(tiamt == TIAMT_COST)
				amount = R2(rTi.Cost);
			else
				amount = rTi.RevalCost;
		}
		else if(correctionFlag > 0) {
			if(tiamt == TIAMT_COST)
				amount = R2(rTi.Cost);
			else
				amount = rTi.NetPrice();
		}
		// } @v11.7.0 
	}
	else {
		//
		// При переоценке основных фондов в ценах реализации используем схему расчета
		// налогов, применяемую для цен поступления, поскольку такая переоценка - суть
		// изменение цен поступления (балансовой стоимости основных фондов).
		//
		if(rTi.Flags & PPTFR_REVAL)
			reval_assets = is_asset;
		if(!reval_assets && tiamt != TIAMT_ASSETEXPL && (tiamt == TIAMT_PRICE || (tiamt != TIAMT_COST && rTi.Flags & (PPTFR_SELLING|PPTFR_REVAL)))) {
			if(rTi.Flags & PPTFR_PRICEWOTAXES)
				amt_flags = GTAXVF_AFTERTAXES;
			if(rTi.Flags & PPTFR_COSTWOVAT && is_asset)
				amt_flags &= ~GTAXVF_VAT;
			const int  re = BIN(rTi.Flags & PPTFR_RMVEXCISE);
			if((r_ccfg.Flags & CCFLG_PRICEWOEXCISE) ? !re : re)
				exclFlags |= GTAXVF_SALESTAX;
			gobj.GTxObj.Fetch(tax_grp_id, rTi.Date, opID, &gtx);
			if(!is_exclvat)
				amount = rTi.NetPrice();
		}
		else {
			if(!(exclFlags & GTAXVF_NOMINAL) && rTi.LotTaxGrpID)
				tax_grp_id = rTi.LotTaxGrpID;
			gobj.GTxObj.Fetch(tax_grp_id, date_of_relevance, 0, &gtx);
			if(rTi.Flags & PPTFR_COSTWOVAT) {
				amt_flags &= ~GTAXVF_VAT;
				if(calcti_costwovat_byprice)
					cost_wo_vat = 1;
			}
			if(tiamt == TIAMT_ASSETEXPL)
				amount = (rTi.Flags & PPTFR_ASSETEXPL) ? rTi.Cost : 0.0;
			else if(rTi.Flags & PPTFR_REVAL)
				amount = (reval_assets && tiamt != TIAMT_COST) ? rTi.NetPrice() : (rTi.Cost - rTi.RevalCost);
			else
				amount = rTi.Cost;
			if(!(rTi.Flags & PPTFR_COSTWSTAX))
				exclFlags |= GTAXVF_SALESTAX;
			if(!(exclFlags & GTAXVF_NOMINAL) && IsSupplVATFree(rTi.Suppl) > 0)
				exclFlags |= GTAXVF_VAT;
			if(gtx.Flags & GTAXF_NOLOTEXCISE)
				exclFlags |= GTAXVF_EXCISE;
		}
	}
	if(gtx.Flags & GTAXF_ABSEXCISE)
		gobj.MultTaxFactor(rTi.GoodsID, &tax_qtty);
	if(!(calcti_costwovat_byprice && cost_wo_vat)) {
		amount *= qtty;
	}
	Calc_(&gtx, amount, tax_qtty, amt_flags, exclFlags);
	if(calcti_costwovat_byprice && cost_wo_vat) {
		for(int i = 0; i < N; i++)
			Vect[i] *= qtty;
		Amount *= qtty;
	}
	return ok;
}
//
//
//
PPObjGoodsTax::PPObjGoodsTax(void * extraPtr) : PPObjReference(PPOBJ_GOODSTAX, extraPtr)
{
}

int PPObjGoodsTax::IsPacketEq(const PPGoodsTaxPacket & rS1, const PPGoodsTaxPacket & rS2, long flags)
{
#define CMP_MEMB(m)  if(rS1.Rec.m != rS2.Rec.m) return 0;
#define CMP_MEMBS(m) if(!sstreq(rS1.Rec.m, rS2.Rec.m)) return 0;
	CMP_MEMB(ID);
	CMP_MEMB(VAT);
	CMP_MEMB(Excise);
	CMP_MEMB(SalesTax);
	CMP_MEMB(Flags);
	CMP_MEMB(Order);
	CMP_MEMB(UnionVect);
	CMP_MEMBS(Name);
	CMP_MEMBS(Symb);
#undef CMP_MEMBS
#undef CMP_MEMB
	const uint _c1 = rS1.GetCount();
	if(_c1 != rS2.GetCount()) {
		return 0;
	}
	else {
		for(uint i = 0; i < _c1; i++) {
			if(!rS1.Get(i).IsEq(rS2.Get(i))) {
				return 0;
			}
		}
	}
	return 1;
}

int PPObjGoodsTax::Search(PPID id, PPGoodsTax * pRec)
{
	return PPObjReference::Search(id, pRec);
}

int PPObjGoodsTax::Search(PPID id, PPGoodsTaxEntry * pEntry)
{
	PPGoodsTax raw_rec;
    int    ok = PPObjReference::Search(id, &raw_rec);
    if(ok <= 0)
		MEMSZERO(raw_rec);
    raw_rec.ToEntry(pEntry);
    return ok;
}

int PPObjGoodsTax::Browse(void * extraPtr)
{
	class GoodsTaxView : public ObjViewDialog {
	public:
		GoodsTaxView(PPObjGoodsTax * pObj) : ObjViewDialog(DLG_GOODSTAXVIEW, pObj, 0)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			ObjViewDialog::handleEvent(event);
			if(event.isKeyDown(kbAltF2) || event.isCmd(cmaAltInsert)) {
				PPID   id = 0, sample_id = getCurrID();
				if(Rt & PPR_INS && sample_id && P_List && static_cast<PPObjGoodsTax *>(P_Obj)->AddBySample(&id, sample_id) == cmOK)
					updateList(id);
				clearEvent(event);
			}
		}
	};
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		GoodsTaxView * dlg = new GoodsTaxView(this);
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}

/*static*/int PPObjGoodsTax::IsIdentical(const PPGoodsTax * pRec1, const PPGoodsTax * pRec2)
{
	if(dbl_cmp(pRec1->VAT, pRec2->VAT) == 0 && dbl_cmp(pRec1->Excise, pRec2->Excise) == 0 &&
		dbl_cmp(pRec1->SalesTax, pRec2->SalesTax) == 0 && pRec1->Flags == pRec2->Flags &&
		pRec1->UnionVect == pRec2->UnionVect) {
		const long ord1 = NZOR(pRec1->Order, PPObjGoodsTax::GetDefaultOrder());
		const long ord2 = NZOR(pRec2->Order, PPObjGoodsTax::GetDefaultOrder());
		if(ord1 == ord2)
			return 1;
	}
	return 0;
}

int PPObjGoodsTax::SearchIdentical(const PPGoodsTax * pPattern, PPID * pID, PPGoodsTax * pRec)
{
	int    ok = -1;
	PPID   id = 0;
	PPGoodsTax rec;
	ASSIGN_PTR(pID, 0);
	while(ok < 0 && P_Ref->EnumItems(Obj, &id, &rec) > 0) {
		if(PPObjGoodsTax::IsIdentical(pPattern, &rec)) {
			ASSIGN_PTR(pRec, rec);
			ASSIGN_PTR(pID, id);
			ok = 1;
		}
	}
	return ok;
}

void PPObjGoodsTax::GetDefaultName(const PPGoodsTax * pRec, char * buf, size_t buflen)
{
	SString text;
	if(R6(pRec->VAT) != 0)
		text.CatChar('V').Cat(pRec->VAT, MKSFMTD(0, 2, NMBF_NOTRAILZ));
	if(R6(pRec->Excise) != 0) {
		text.CatDivIfNotEmpty(' ', 0).CatChar('A').Cat(pRec->Excise, MKSFMTD(0, 2, NMBF_NOTRAILZ));
		if(pRec->Flags & GTAXF_ABSEXCISE)
			text.CatChar('$');
	}
	if(R6(pRec->SalesTax) != 0)
		text.CatDivIfNotEmpty(' ', 0).CatChar('S').Cat(pRec->SalesTax, MKSFMTD(0, 2, NMBF_NOTRAILZ));
	if(text.IsEmpty())
		text.CatCharN('0', 3);
	strnzcpy(buf, text, buflen);
}

int PPObjGoodsTax::GetByScheme(PPID * pID, double vat, double excise, double stax, long flags, int use_ta)
{
	int    ok = 1;
	PPID   id = 0;
	PPGoodsTax pattern, rec;
	pattern.VAT      = vat;
	pattern.Excise   = excise;
	pattern.SalesTax = stax;
	pattern.Flags    = flags;
	if(SearchIdentical(&pattern, &id, &rec) > 0) {
		ASSIGN_PTR(pID, id);
	}
	else {
		rec = pattern;
		GetDefaultName(&rec, rec.Name, sizeof(rec.Name));
		if(P_Ref->AddItem(PPOBJ_GOODSTAX, &id, &rec, use_ta)) {
			ASSIGN_PTR(pID, id);
		}
		else
			ok = 0;
	}
	return ok;
}

/*static*/long PPObjGoodsTax::GetDefaultOrder()
{
	long   list[6];
	list[0] = GTAX_EXCISE;
	list[1] = GTAX_VAT;
	list[2] = GTAX_SALES;
	return GetTranspositionNumber(list, 3);
}

int PPObjGoodsTax::FormatOrder(long order, long unionVect, char * buf, size_t /*buflen*/)
{
	long   list[8];
	const  int N = 3;
	int    i;
	long   mask = 0;
	for(i = 0; i < N; i++)
		mask |= (1 << i);
	unionVect &= mask;
	if(GetNthTransposition(list, N, order)) {
		char * b = buf;
		for(i = 0; i < N; i++) {
			if(i < (N-1) && (unionVect & (1 << (i+1))))
				*b++ = '[';
			if(list[i] == GTAX_VAT)
				*b = 'V';
			else if(list[i] == GTAX_EXCISE)
				*b = 'A';
			else if(list[i] == GTAX_SALES)
				*b = 'S';
			b++;
			if(unionVect & (1 << i) && !(unionVect & (1 << (i+1))))
				*b++ = ']';
		}
		*b = 0;
		return 1;
	}
	buf[0] = 0;
	return 0;
}

int PPObjGoodsTax::StrToOrder(const char * buf, long * pOrder, long * pUnionVect)
{
	const  int N = 3;
	long   list[6];
	long   order = 0, union_vect = 0;
	char   temp[32];
	STRNSCPY(temp, buf);
	if(*strip(temp) == 0)
		order = PPObjGoodsTax::GetDefaultOrder();
	else {
		int p = 0, brace_opened = -1;
		for(int i = 0; i < N; i++) {
			char c = toupper(temp[p++]);
			if(c == '[') {
				brace_opened = i;
				c = toupper(temp[p++]);
			}
			else if(brace_opened >= 0 && c == ']') {
				brace_opened = -1;
				c = toupper(temp[p++]);
			}
			if(c == 'V' || c == '1')
				list[i] = GTAX_VAT;
			else if(c == 'A' || c == '2')
				list[i] = GTAX_EXCISE;
			else if(c == 'S' || c == '3')
				list[i] = GTAX_SALES;
			else {
				ASSIGN_PTR(pOrder, 0);
				ASSIGN_PTR(pUnionVect, 0);
				return 0;
			}
			if(brace_opened >= 0 && brace_opened < i)
				union_vect |= (1 << i);
		}
		order = GetTranspositionNumber(list, N);
	}
	ASSIGN_PTR(pOrder, order);
	ASSIGN_PTR(pUnionVect, union_vect);
	return (order == 0) ? 0 : 1;
}

int PPObjGoodsTax::HandleMsg(int msg, PPID objTypeID, PPID objID, void * extraPtr)
{
	if(msg == DBMSG_OBJREPLACE)
		if(objTypeID == PPOBJ_GOODSTAX) {
			PPID    dest_id = objID;
			if(!BroadcastObjMessage(DBMSG_OBJREPLACE, Obj, dest_id, extraPtr /*srcID*/))
				return DBRPL_ERROR;
		}
	return DBRPL_OK;
}
/*
	Формат хранения пакета PPGoodsTaxPacket

	Reference (PPOBJ_GOODSTAX, ID): Rec
	Property  (PPOBJ_GOODSTAX, ID, GTGPRP_ENTRIES): PPGoodsTaxEntry[]
*/
int PPObjGoodsTax::PutPacket(PPID * pID, PPGoodsTaxPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				PPGoodsTaxPacket org_pack;
				THROW(GetPacket(*pID, &org_pack) > 0);
				if(IsPacketEq(*pPack, org_pack, 0)) {
					ok = -1;
				}
				else {
					THROW(CheckRights(PPR_MOD));
					THROW(CheckDupName(*pID, pPack->Rec.Name));
					THROW(CheckDupSymb(*pID, pPack->Rec.Symb));
					THROW(P_Ref->UpdateItem(Obj, *pID, &pPack->Rec, 0, 0));
					THROW(P_Ref->PutPropArray(Obj, *pID, GTGPRP_ENTRIES, pPack->vecptr(), 0));
					DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
				}
			}
			else {
				THROW(CheckRights(PPR_DEL));
				THROW(P_Ref->RemoveItem(Obj, *pID, 0));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
			}
			if(ok > 0)
				Dirty(*pID);
		}
		else if(pPack) {
			THROW(CheckRights(PPR_INS));
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			THROW(CheckDupSymb(*pID, pPack->Rec.Symb));
			*pID = pPack->Rec.ID;
			THROW(P_Ref->AddItem(Obj, pID, &pPack->Rec, 0));
			THROW(P_Ref->PutPropArray(Obj, *pID, GTGPRP_ENTRIES, pPack->vecptr(), 0));
			pPack->Rec.ID = *pID;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsTax::GetPacket(PPID id, PPGoodsTaxPacket * pPack)
{
	int    ok = -1;
	if(PPCheckGetObjPacketID(Obj, id)) { // @v10.3.6
		if(Search(id, &pPack->Rec) > 0) {
			P_Ref->GetPropArray(Obj, id, GTGPRP_ENTRIES, pPack->vecptr());
			uint i;
			int  is_zero_excise = 1;
			if(pPack->Rec.Excise != 0)
				is_zero_excise = 0;
			else {
				for(i = 0; i < pPack->GetCount(); i++) {
					if(pPack->Get(i).Excise != 0) {
						is_zero_excise = 0;
						break;
					}
				}
			}
			for(i = 0; i < pPack->GetCount(); i++) {
				PPGoodsTaxEntry & r_entry = pPack->Get(i);
				SETFLAG(r_entry.Flags, GTAXF_ZEROEXCISE, is_zero_excise);
				r_entry.Flags |= GTAXF_ENTRY;
				long s = static_cast<long>(i+1);
				r_entry.TaxGrpID = ((id & 0x00ffffffL) | (s << 24));
			}
			SETFLAG(pPack->Rec.Flags, GTAXF_ZEROEXCISE, is_zero_excise);
			SETFLAG(pPack->Rec.Flags, GTAXF_USELIST, pPack->GetCount());
			ok = 1;
		}
	}
	return ok;
}

int PPObjGoodsTax::SerializePacket(int dir, PPGoodsTaxPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	int32  c = static_cast<int32>(pPack->GetCount()); // @persistent
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, c, rBuf));
	for(int i = 0; i < c; i++) {
		PPGoodsTaxEntry item;
		if(dir > 0)
			item = pPack->Get(i);
		THROW_SL(pSCtx->Serialize(dir, item.TaxGrpID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Period.low, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Period.upp, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.OpID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.VAT,  rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Excise,   rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.SalesTax, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Flags, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Order, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.UnionVect, rBuf));
		if(dir < 0) {
			THROW_SL(pPack->Insert(item));
		}
	}
	CATCHZOK
	return ok;
}

int  PPObjGoodsTax::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjGoodsTax, PPGoodsTaxPacket>(this, p, id, stream, pCtx); }

int PPObjGoodsTax::SearchAnalog(const PPGoodsTax * pSample, PPID * pID, PPGoodsTax * pRec)
{
	int    ok = -1;
	PPID   same_id = 0;
	if(P_Ref->SearchSymb(Obj, &same_id, pSample->Symb, offsetof(PPGoodsTax, Symb)) > 0) {
		ok = 1;
	}
	else if(P_Ref->SearchSymb(Obj, &same_id, pSample->Name, offsetof(PPGoodsTax, Name)) > 0) {
		ok = 1;
	}
	if(ok > 0) {
		if(pRec) {
			THROW(Search(same_id, pRec) > 0);
		}
	}
	CATCH
		same_id = 0;
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pID, same_id);
	return ok;
}

int  PPObjGoodsTax::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data) {
		PPGoodsTaxPacket * p_pack = static_cast<PPGoodsTaxPacket *>(p->Data);
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				if((p_pack->Rec.ID && p_pack->Rec.ID < PP_FIRSTUSRREF) || SearchAnalog(&p_pack->Rec, &same_id, 0) > 0) {
					PPGoodsTax same_rec;
					if(Search(same_id, &same_rec) > 0) {
						ASSIGN_PTR(pID, same_id);
					}
					else
						same_id = 0;
				}
				if(same_id == 0) {
					if(p_pack->Rec.ID >= PP_FIRSTUSRREF)
						p_pack->Rec.ID = 0;
					if(!PutPacket(pID, p_pack, 1)) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTGOODSTAX, p_pack->Rec.ID, p_pack->Rec.Name);
						ok = -1;
					}
				}
			}
			else {
				// @v10.2.6 {
				if(!PutPacket(pID, p_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTGOODSTAX, p_pack->Rec.ID, p_pack->Rec.Name);
					ok = -1;
				}
				// } @v10.2.6 
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsTax::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPGoodsTaxPacket * p_pack = static_cast<PPGoodsTaxPacket *>(p->Data);
		for(uint i = 0; i < p_pack->GetCount(); i++) {
			PPGoodsTaxEntry & r_entry = p_pack->Get(i);
			THROW(ProcessObjRefInArray(PPOBJ_OPRKIND, &r_entry.OpID, ary, replace));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjGoodsTax, PPGoodsTaxPacket);

/*static*/int PPObjGoodsTax::ReplaceGoodsTaxGrp()
{
	int    ok = -1;
	PPID   src_id = 0, dest_id = 0;
	TDialog * dlg = new TDialog(DLG_REPLGTX);
	if(CheckDialogPtrErr(&dlg)) {
		SetupPPObjCombo(dlg, CTLSEL_REPLGTX_DEST, PPOBJ_GOODSTAX, dest_id, 0, 0);
		SetupPPObjCombo(dlg, CTLSEL_REPLGTX_SRC,  PPOBJ_GOODSTAX, src_id,  0, 0);
		while(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTLSEL_REPLGTX_SRC,  &src_id);
			dlg->getCtrlData(CTLSEL_REPLGTX_DEST, &dest_id);
			if(dest_id == src_id)
				PPError(PPERR_REPLSAMEOBJ, 0);
			else if(dest_id == 0 || src_id == 0)
				PPError(PPERR_REPLZEROOBJ, 0);
			else {
				PPGoodsTax gtx_s, gtx_d;
				if(SearchObject(PPOBJ_GOODSTAX, src_id, &gtx_s) > 0 &&
					SearchObject(PPOBJ_GOODSTAX, dest_id, &gtx_d) > 0) {
					int r = 1;
					if(!PPObjGoodsTax::IsIdentical(&gtx_s, &gtx_d)) {
						if(!CONFIRM(PPCFM_UNITENEQGTX))
							r = -1;
					}
					if(r > 0)
						if(PPObject::ReplaceObj(PPOBJ_GOODSTAX, dest_id, src_id, PPObject::use_transaction|PPObject::user_request)) {
							ok = 1;
							if(SearchObject(PPOBJ_GOODSTAX, dest_id) <= 0)
								dest_id = 0;
							SetupPPObjCombo(dlg, CTLSEL_REPLGTX_SRC, PPOBJ_GOODSTAX, src_id, 0, 0);
							SetupPPObjCombo(dlg, CTLSEL_REPLGTX_DEST, PPOBJ_GOODSTAX, dest_id, 0, 0);
						}
						else
							PPError();
				}
				else
					PPError();
			}
		}
	}
//turistti
	delete dlg;
	return ok;
}
//
//
//
class GTaxCache : public ObjCache {
public:
	GTaxCache();
	int    Get(PPID, LDATE, PPID, PPGoodsTaxEntry *);
	int    GetByID(PPID, PPGoodsTaxEntry *);
	virtual void FASTCALL Dirty(PPID); // @sync_w
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long)
	{
		return 0;
	}
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
	{
	}
	int    SearchEntry(PPID id, LDATE dt, PPID opID, PPGoodsTaxEntry * pEntry) const;
	int    GetFromBase(PPID);
};

GTaxCache::GTaxCache() : ObjCache(PPOBJ_GOODSTAX, sizeof(PPGoodsTaxEntry))
{
}

void FASTCALL GTaxCache::Dirty(PPID id)
{
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		uint   c = P_Ary->getCount();
		if(c) do {
			--c;
			PPID   item_id = static_cast<const PPGoodsTaxEntry *>(P_Ary->at(c))->TaxGrpID;
			if((item_id & 0x00ffffffL) == id)
				P_Ary->atFree(c);
		} while(c);
	}
}

int GTaxCache::GetFromBase(PPID id)
{
	int    ok = -1;
	PPObjGoodsTax gt_obj;
	PPGoodsTaxPacket gt_pack;
	if(gt_obj.GetPacket(id, &gt_pack) > 0) {
		PPGoodsTaxEntry item;
		for(uint i = 0; i < gt_pack.GetCount(); i++)
			P_Ary->insert(&gt_pack.Get(i));
		gt_pack.Rec.ToEntry(&item);
		item.Flags &= ~GTAXF_USELIST;
		P_Ary->insert(&item);
		ok = 1;
	}
	return ok;
}

int GTaxCache::GetByID(PPID id, PPGoodsTaxEntry * pEntry)
{
	int    ok = -1;
	if(id) {
		uint   pos = 0;
		if(P_Ary->lsearch(&id, &pos, CMPF_LONG, offsetof(PPGoodsTaxEntry, TaxGrpID))) {
			ASSIGN_PTR(pEntry, *(PPGoodsTaxEntry *)P_Ary->at(pos));
			ok = 1;
		}
		else if(!GetFromBase(id & 0x00ffffffL))
			ok = 0;
		else if(P_Ary->lsearch(&id, &(pos = 0), CMPF_LONG, offsetof(PPGoodsTaxEntry, TaxGrpID))) {
			ASSIGN_PTR(pEntry, *(PPGoodsTaxEntry *)P_Ary->at(pos));
			ok = 1;
		}
	}
	return ok;
}

int GTaxCache::SearchEntry(PPID id, LDATE dt, PPID opID, PPGoodsTaxEntry * pEntry) const
{
	int    ok = 0;
	const  PPID _id = (id & 0x00ffffffL);
	PPGoodsTaxEntry * p_item;
	for(uint i = 0; !ok && P_Ary->enumItems(&i, (void **)&p_item);)
		if(p_item->OpID == opID && (p_item->TaxGrpID & 0x00ffffffL) == _id && p_item->Period.CheckDate(dt)) {
			ASSIGN_PTR(pEntry, *p_item);
			ok = 1;
		}
	return ok;
}

int GTaxCache::Get(PPID id, LDATE dt, PPID opID, PPGoodsTaxEntry * pEntry)
{
	int    ok = -1;
	if(id) {
		if(dt == 0 && opID == 0)
			ok = GetByID((id & 0x00ffffffL), pEntry);
		else {
			if(SearchEntry(id, dt, opID, pEntry) || (opID && SearchEntry(id, dt, 0, pEntry)))
				ok = 1;
			else if(!GetFromBase(id))
				ok = 0;
			else if(SearchEntry(id, dt, opID, pEntry) || (opID && SearchEntry(id, dt, 0, pEntry)))
				ok = 1;
		}
	}
	return ok;
}

/*static*/int PPObjGoodsTax::Fetch(PPID id, LDATE dt, PPID opID, PPGoodsTaxEntry * pEntry)
{
	memzero(pEntry, sizeof(*pEntry));
	GTaxCache * p_cache = GetDbLocalCachePtr <GTaxCache> (PPOBJ_GOODSTAX);
	return p_cache ? p_cache->Get(id, dt, opID, pEntry) : 0;
}

/*static*/int PPObjGoodsTax::FetchByID(PPID id, PPGoodsTaxEntry * pEntry)
{
	memzero(pEntry, sizeof(*pEntry));
	GTaxCache * p_cache = GetDbLocalCachePtr <GTaxCache> (PPOBJ_GOODSTAX);
	return p_cache ? p_cache->GetByID(id, pEntry) : 0;
}
