// OBJGTAX.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2009, 2011, 2013, 2014, 2015, 2016, 2017
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
//
//
//
double SLAPI PPGoodsTaxEntry::GetVatRate() const
{
	return fdiv100i(VAT);
}

char * SLAPI PPGoodsTaxEntry::FormatVAT(char * pBuf, size_t bufLen) const
{
	long   fmt = MKSFMTD(0, 2, NMBF_NOTRAILZ | ALIGN_LEFT | NMBF_NOZERO);
	char   str[32];
	realfmt(fdiv100i(VAT), fmt, str); // @divtax
	return strnzcpy(pBuf, str, bufLen);
}

char * SLAPI PPGoodsTaxEntry::FormatSTax(char * pBuf, size_t bufLen) const
{
	long   fmt = MKSFMTD(0, 2, NMBF_NOTRAILZ | ALIGN_LEFT | NMBF_NOZERO);
	char   str[32];
	realfmt(fdiv100i(SalesTax), fmt, str); // @divtax
	return strnzcpy(pBuf, str, bufLen);
}

char * SLAPI PPGoodsTaxEntry::FormatExcise(char * pBuf, size_t bufLen) const
{
	SString temp_buf;
	temp_buf.Cat(fdiv100i(Excise), MKSFMTD(0, 2, NMBF_NOTRAILZ|ALIGN_LEFT|NMBF_NOZERO)); // @divtax
	if(Excise && Flags & GTAXF_ABSEXCISE)
		temp_buf.CatChar('$');
	return temp_buf.CopyTo(pBuf, bufLen);
}

int FASTCALL PPGoodsTax::ToEntry(PPGoodsTaxEntry * pEntry) const
{
	if(pEntry) {
		pEntry->TaxGrpID = ID;
		pEntry->VAT      = R0i(VAT      * 100L);  // @divtax
		pEntry->Excise   = R0i(Excise   * 100L);  // @divtax
		pEntry->SalesTax = R0i(SalesTax * 100L);  // @divtax
		pEntry->Flags = Flags;
		pEntry->Order = Order;
		pEntry->UnionVect = UnionVect;
	}
	return 1;
}

int FASTCALL PPGoodsTax::FromEntry(const PPGoodsTaxEntry * pEntry)
{
	//pEntry->TaxGrpID = ID;
	VAT      = fdiv100i(pEntry->VAT);      // @divtax
	Excise   = fdiv100i(pEntry->Excise);   // @divtax
	SalesTax = fdiv100i(pEntry->SalesTax); // @divtax
	Flags = pEntry->Flags;
	Order = pEntry->Order;
	UnionVect = pEntry->UnionVect;
	return 1;
}

SLAPI PPGoodsTaxPacket::PPGoodsTaxPacket() : SArray(sizeof(PPGoodsTaxEntry))
{
	MEMSZERO(Rec);
}

PPGoodsTaxPacket & FASTCALL PPGoodsTaxPacket::operator = (const PPGoodsTaxPacket & s)
{
	Rec = s.Rec;
	copy(s);
	return *this;
}

int SLAPI PPGoodsTaxPacket::PutEntry(int pos, const PPGoodsTaxEntry * pEntry)
{
	uint   i;
	long   max_idx = 0;
	PPGoodsTaxEntry * p_item;
	for(i = 0; enumItems(&i, (void**)&p_item);) {
		if(pos != (int)(i-1) && pEntry->Period.IsIntersect(p_item->Period))
			if(pEntry->OpID == p_item->OpID)
				return PPSetError(PPERR_GTAXENTINTRSCT);
		long idx = ((p_item->TaxGrpID & 0xff000000L) >> 24);
		max_idx = MAX(max_idx, idx);
	}
	if(pos < 0 || pos >= (int)getCount()) {
		PPGoodsTaxEntry item = *pEntry;
		item.TaxGrpID &= 0x00ffffffL;
		item.TaxGrpID |= ((max_idx+1) << 24);
		if(!insert(&item))
			return PPSetErrorSLib();
	}
	else {
		(*(PPGoodsTaxEntry*)at(pos)) = *pEntry;
	}
	return 1;
}

IMPL_CMPFUNC(PPGoodsTaxEntry, i1, i2)
{
	PPGoodsTaxEntry * p_i1 = (PPGoodsTaxEntry *)i1;
	PPGoodsTaxEntry * p_i2 = (PPGoodsTaxEntry *)i2;
	int    r = cmp_long((p_i1->TaxGrpID & 0x00ffffffL), (p_i2->TaxGrpID & 0x00ffffffL));
	r = NZOR(r, cmp_ulong(p_i1->Period.low, p_i2->Period.low));
	return NZOR(r, cmp_long(p_i1->OpID, p_i2->OpID));
}

int SLAPI PPGoodsTaxPacket::Sort()
{
	return sort(PTR_CMPFUNC(PPGoodsTaxEntry));
}
//
//
//
SLAPI GTaxVect::GTaxVect(int roundPrec)
{
	RoundPrec = roundPrec;
}

int FASTCALL GTaxVect::TaxToVect(int taxIdx) const
{
	if(taxIdx == 0)
		return 0;
	for(int i = 1; i <= N; i++)
		if(OrderVect[i] == (long)taxIdx)
			return i;
	return -1;
}

int FASTCALL GTaxVect::VectToTax(int idx) const
{
	return (idx > 0 && idx <= N) ? (int)OrderVect[idx] : (idx ? -1 : 0);
}

double SLAPI GTaxVect::GetTaxRate(long taxID, int * pIsAbs) const
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

double SLAPI GTaxVect::CalcTaxValByBase(int idx, double base) const
{
	double v = (AbsVect & (1 << idx)) ? Qtty : base;
	return round(RateVect[idx] * v, RoundPrec);
}
//
// amount - сумма, включающая налоги 1..n (n<=N)
// Расчитывает сумму без налогов (AmountAfterTaxes) GTaxVect::Vect[0]
//
int SLAPI GTaxVect::CalcForward(int n, double amount)
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
	return 1;
}
//
// amount - сумма, не включающая налоги (n+1)..N (1 <= n <= N)
// Расчитывает полную сумму со всеми налогами GTaxVect::Amount
//
int SLAPI GTaxVect::CalcBackward(int n, double amount)
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
	return 1;
}

void SLAPI GTaxVect::Calc_(PPGoodsTaxEntry * gtax, double amount, double qtty, long amtFlags, long excludeFlags)
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
					RateVect[i] = ((double)gtax->VAT) / (100L * 100L); // @divtax
				break;
			case GTAX_EXCISE:
				if(!(excludeFlags & GTAXVF_EXCISE))
					if(gtax->Flags & GTAXF_ABSEXCISE) {
						RateVect[i] = ((double)gtax->Excise) / 100L; // @divtax
						AbsVect |= (1 << i);
					}
					else
						RateVect[i] = ((double)gtax->Excise) / (100L * 100L); // @divtax
				break;
			case GTAX_SALES:
				if(!(excludeFlags & GTAXVF_SALESTAX))
					RateVect[i] = ((double)gtax->SalesTax) / (100L * 100L);   // @divtax
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

int SLAPI GTaxVect::CalcTI(const PPTransferItem * pTI, PPID opID, int tiamt, long exclFlags)
{
	int    ok = 1;
	const  PPCommConfig & r_ccfg = CConfig;
	int    calcti_costwovat_byprice = (r_ccfg.Flags & CCFLG_COSTWOVATBYSUM) ? 0 : 1;
	int    cost_wo_vat = 0;
	int    is_asset = 0;
	int    is_exclvat = 0;
	int    reval_assets = 0;
	long   amt_flags = ~0L;
	PPID   tax_grp_id = 0;
	PPGoodsTaxEntry gtx;
	PPObjGoods    gobj;
	PPObjGoodsTax gtobj;
	Goods2Tbl::Rec goods_rec;
	double amount = 0.0;
	double qtty   = fabs(pTI->Qtty());
	double tax_qtty = qtty;
	if(gobj.Fetch(labs(pTI->GoodsID), &goods_rec) > 0) {
		if(goods_rec.Flags & GF_ASSETS)
			is_asset = 1;
		tax_grp_id = goods_rec.TaxGrpID;
		if(!(exclFlags & GTAXVF_NOMINAL) && r_ccfg.DynGoodsTypeForSupplAgent && pTI->LotID && BillObj->GetSupplAgent(pTI->LotID) > 0) {
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
	if(!(exclFlags & GTAXVF_NOMINAL) && PPObjLocation::CheckWarehouseFlags(pTI->LocID, LOCF_VATFREE))
		exclFlags |= GTAXVF_VAT;
	if(pTI->IsCorrectionRcpt()) {
		if(!(exclFlags & GTAXVF_NOMINAL) && pTI->LotTaxGrpID)
			tax_grp_id = pTI->LotTaxGrpID;
		MEMSZERO(gtx);
		gtobj.Fetch(tax_grp_id, pTI->LotDate, 0, &gtx);

		const double q_pre = fabs(pTI->QuotPrice);
		qtty = fabs(pTI->Quantity_);
		const double q_diff = (qtty - q_pre);
		if(q_diff != 0.0) { // @v8.9.7 qtty-->q_diff
			const double cq = R2(pTI->Cost * qtty - pTI->RevalCost * q_pre);
			const double pq = R2(pTI->Price * qtty - pTI->Discount * q_pre);
			// @v8.9.8 amount = ((tiamt != TIAMT_COST) ? pq : cq) / q_diff;
			amount = ((tiamt != TIAMT_PRICE) ? cq : pq) / q_diff; // @v8.9.8 Для корректировки НДС всегда в ценах поступления
		}
		else
			amount = 0.0;
		qtty = q_diff;
	}
	else {
		//
		// При переоценке основных фондов в ценах реализации используем схему расчета
		// налогов, применяемую для цен поступления, поскольку такая переоценка - суть
		// изменение цен поступления (балансовой стоимости основных фондов).
		//
		if(pTI->Flags & PPTFR_REVAL)
			reval_assets = is_asset;
		if(!reval_assets && tiamt != TIAMT_ASSETEXPL && (tiamt == TIAMT_PRICE || (tiamt != TIAMT_COST && pTI->Flags & (PPTFR_SELLING|PPTFR_REVAL)))) {
			if(pTI->Flags & PPTFR_PRICEWOTAXES)
				amt_flags = GTAXVF_AFTERTAXES;
			if(pTI->Flags & PPTFR_COSTWOVAT && is_asset)
				amt_flags &= ~GTAXVF_VAT;
			const int  re = BIN(pTI->Flags & PPTFR_RMVEXCISE);
			if((r_ccfg.Flags & CCFLG_PRICEWOEXCISE) ? !re : re)
				exclFlags |= GTAXVF_SALESTAX;
			MEMSZERO(gtx);
			gtobj.Fetch(tax_grp_id, pTI->Date, opID, &gtx);
			if(!is_exclvat)
				amount = pTI->NetPrice();
		}
		else {
			if(!(exclFlags & GTAXVF_NOMINAL) && pTI->LotTaxGrpID)
				tax_grp_id = pTI->LotTaxGrpID;
			MEMSZERO(gtx);
			gtobj.Fetch(tax_grp_id, pTI->LotDate, 0, &gtx);
			if(pTI->Flags & PPTFR_COSTWOVAT) {
				amt_flags &= ~GTAXVF_VAT;
				if(calcti_costwovat_byprice)
					cost_wo_vat = 1;
			}
			if(tiamt == TIAMT_ASSETEXPL)
				amount = (pTI->Flags & PPTFR_ASSETEXPL) ? pTI->Cost : 0.0;
			else if(pTI->Flags & PPTFR_REVAL)
				amount = (reval_assets && tiamt != TIAMT_COST) ? pTI->NetPrice() : (pTI->Cost - pTI->RevalCost);
			else
				amount = pTI->Cost;
			if(!(pTI->Flags & PPTFR_COSTWSTAX))
				exclFlags |= GTAXVF_SALESTAX;
			if(!(exclFlags & GTAXVF_NOMINAL) && IsSupplVATFree(pTI->Suppl) > 0)
				exclFlags |= GTAXVF_VAT;
			if(gtx.Flags & GTAXF_NOLOTEXCISE)
				exclFlags |= GTAXVF_EXCISE;
		}
	}
	if(gtx.Flags & GTAXF_ABSEXCISE)
		gobj.MultTaxFactor(pTI->GoodsID, &tax_qtty);
	if(!(calcti_costwovat_byprice && cost_wo_vat))
		amount *= qtty;
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
SLAPI PPObjGoodsTax::PPObjGoodsTax(void * extraPtr) : PPObjReference(PPOBJ_GOODSTAX, extraPtr)
{
}

int SLAPI PPObjGoodsTax::Search(PPID id, PPGoodsTax * pRec)
{
	return PPObjReference::Search(id, pRec);
}

int SLAPI PPObjGoodsTax::Search(PPID id, PPGoodsTaxEntry * pEntry)
{
	PPGoodsTax raw_rec;
    int    ok = PPObjReference::Search(id, &raw_rec);
    if(ok <= 0)
		MEMSZERO(raw_rec);
    raw_rec.ToEntry(pEntry);
    return ok;
}

int SLAPI PPObjGoodsTax::Browse(void * extraPtr)
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
				if(Rt & PPR_INS && sample_id && P_List && ((PPObjGoodsTax *)P_Obj)->AddBySample(&id, sample_id) == cmOK)
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

// static
int SLAPI PPObjGoodsTax::IsIdentical(const PPGoodsTax * pRec1, const PPGoodsTax * pRec2)
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

int SLAPI PPObjGoodsTax::SearchIdentical(const PPGoodsTax * pPattern, PPID * pID, PPGoodsTax * pRec)
{
	int    ok = -1;
	PPID   id = 0;
	PPGoodsTax rec;
	ASSIGN_PTR(pID, 0);
	while(ok < 0 && ref->EnumItems(Obj, &id, &rec) > 0) {
		if(PPObjGoodsTax::IsIdentical(pPattern, &rec)) {
			ASSIGN_PTR(pRec, rec);
			ASSIGN_PTR(pID, id);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPObjGoodsTax::GetDefaultName(PPGoodsTax * rec, char * buf, size_t buflen)
{
	SString text;
	if(R6(rec->VAT) != 0)
		text.CatChar('V').Cat(rec->VAT, MKSFMTD(0, 2, NMBF_NOTRAILZ));
	if(R6(rec->Excise) != 0) {
		text.CatDiv(' ', 0, 1).CatChar('A').Cat(rec->Excise, MKSFMTD(0, 2, NMBF_NOTRAILZ));
		if(rec->Flags & GTAXF_ABSEXCISE)
			text.CatChar('$');
	}
	if(R6(rec->SalesTax) != 0)
		text.CatDiv(' ', 0, 1).CatChar('S').Cat(rec->SalesTax, MKSFMTD(0, 2, NMBF_NOTRAILZ));
	if(text.Empty())
		text.CatCharN('0', 3);
	strnzcpy(buf, text, buflen);
	return 1;
}

int SLAPI PPObjGoodsTax::GetBySheme(PPID * pID, double vat, double excise, double stax, long flags)
{
	PPID   id = 0;
	PPGoodsTax pattern, rec;
	MEMSZERO(pattern);
	pattern.VAT      = vat;
	pattern.Excise   = excise;
	pattern.SalesTax = stax;
	pattern.Flags    = flags;
	if(SearchIdentical(&pattern, &id, &rec) > 0) {
		ASSIGN_PTR(pID, id);
		return 1;
	}
	else {
		rec = pattern;
		GetDefaultName(&rec, rec.Name, sizeof(rec.Name));
		if(ref->AddItem(PPOBJ_GOODSTAX, &id, &rec, 0)) {
			ASSIGN_PTR(pID, id);
			return 1;
		}
		else
			return 0;
	}
}

// static
long SLAPI PPObjGoodsTax::GetDefaultOrder()
{
	long   list[6];
	list[0] = GTAX_EXCISE;
	list[1] = GTAX_VAT;
	list[2] = GTAX_SALES;
	return GetTranspositionNumber(list, 3);
}

int SLAPI PPObjGoodsTax::FormatOrder(long order, long unionVect, char * buf, size_t /*buflen*/)
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

int SLAPI PPObjGoodsTax::StrToOrder(const char * buf, long * pOrder, long * pUnionVect)
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

int SLAPI PPObjGoodsTax::HandleMsg(int msg, PPID objTypeID, PPID objID, void * extraPtr)
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
int SLAPI PPObjGoodsTax::PutPacket(PPID * pID, PPGoodsTaxPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			if(*pID) {
				THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
			}
			else {
				*pID = pPack->Rec.ID;
				THROW(ref->AddItem(Obj, pID, &pPack->Rec, 0));
			}
			THROW(ref->PutPropArray(Obj, *pID, GTGPRP_ENTRIES, pPack, 0));
		}
		else if(*pID) {
			THROW(ref->RemoveItem(Obj, *pID, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoodsTax::GetPacket(PPID id, PPGoodsTaxPacket * pPack)
{
	int    ok = -1;
	if(Search(id, &pPack->Rec) > 0) {
		ref->GetPropArray(Obj, id, GTGPRP_ENTRIES, pPack);
		PPGoodsTaxEntry * p_item;
		uint i;
		int  is_zero_excise = 1;
		if(pPack->Rec.Excise != 0)
			is_zero_excise = 0;
		else
			for(i = 0; pPack->enumItems(&i, (void**)&p_item);)
				if(p_item->Excise != 0) {
					is_zero_excise = 0;
					break;
				}
		for(i = 0; pPack->enumItems(&i, (void**)&p_item);) {
			SETFLAG(p_item->Flags, GTAXF_ZEROEXCISE, is_zero_excise);
			p_item->Flags |= GTAXF_ENTRY;
			long s = (long)i;
			p_item->TaxGrpID = ((id & 0x00ffffffL) | (s << 24));
		}
		SETFLAG(pPack->Rec.Flags, GTAXF_ZEROEXCISE, is_zero_excise);
		SETFLAG(pPack->Rec.Flags, GTAXF_USELIST, pPack->getCount());
		ok = 1;
	}
	return ok;
}

int SLAPI PPObjGoodsTax::SerializePacket(int dir, PPGoodsTaxPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	int32  c = (int32)pPack->getCount(); // @persistent
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, c, rBuf));
	for(int i = 0; i < c; i++) {
		PPGoodsTaxEntry item;
		if(dir > 0)
			item = *(PPGoodsTaxEntry *)pPack->at(i);
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
			THROW_SL(pPack->insert(&item));
		}
	}
	CATCHZOK
	return ok;
}

int  SLAPI PPObjGoodsTax::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	p->Data = new PPGoodsTaxPacket;
	THROW_MEM(p->Data);
	if(stream == 0) {
		THROW(GetPacket(id, (PPGoodsTaxPacket *)p->Data) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile((FILE*)stream, 0))
		THROW(SerializePacket(-1, (PPGoodsTaxPacket *)p->Data, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoodsTax::SearchAnalog(const PPGoodsTax * pSample, PPID * pID, PPGoodsTax * pRec)
{
	int    ok = -1;
	PPID   same_id = 0;
	if(ref->SearchSymb(Obj, &same_id, pSample->Symb, offsetof(PPGoodsTax, Symb)) > 0) {
		ok = 1;
	}
	else if(ref->SearchSymb(Obj, &same_id, pSample->Name, offsetof(PPGoodsTax, Name)) > 0) {
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

int  SLAPI PPObjGoodsTax::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data) {
		PPGoodsTaxPacket * p_pack = (PPGoodsTaxPacket *)p->Data;
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
					THROW(PutPacket(pID, p_pack, 1));
				}
			}
			else {
				;
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile((FILE*)stream, 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoodsTax::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPGoodsTaxPacket * p_pack = (PPGoodsTaxPacket *)p->Data;
		for(uint i = 0; i < p_pack->getCount(); i++) {
			PPGoodsTaxEntry * p_entry = (PPGoodsTaxEntry *)p_pack->at(i);
			THROW(ProcessObjRefInArray(PPOBJ_OPRKIND, &p_entry->OpID, ary, replace));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjGoodsTax, PPGoodsTaxPacket);

//static
int SLAPI PPObjGoodsTax::ReplaceGoodsTaxGrp()
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
	SLAPI  GTaxCache();
	int    SLAPI Get(PPID, LDATE, PPID, PPGoodsTaxEntry *);
	int    SLAPI GetByID(PPID, PPGoodsTaxEntry *);
	virtual int  SLAPI Dirty(PPID); // @sync_w
private:
	virtual int SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long) { return 0; }
	virtual int SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const { return 0; }
	int    SLAPI SearchEntry(PPID id, LDATE dt, PPID opID, PPGoodsTaxEntry * pEntry) const;
	int    SLAPI GetFromBase(PPID);
};

SLAPI GTaxCache::GTaxCache() : ObjCache(PPOBJ_GOODSTAX, sizeof(PPGoodsTaxEntry))
{
}

int SLAPI GTaxCache::Dirty(PPID id)
{
	RwL.WriteLock();
	uint   c = P_Ary->getCount();
	if(c) do {
		--c;
		PPID   item_id = ((PPGoodsTaxEntry*)P_Ary->at(c))->TaxGrpID;
		if((item_id & 0x00ffffffL) == id)
			P_Ary->atFree(c);
	} while(c);
	RwL.Unlock();
	return 1;
}

int SLAPI GTaxCache::GetFromBase(PPID id)
{
	int    ok = -1;
	PPObjGoodsTax    gt_obj;
	PPGoodsTaxPacket gt_pack;
	if(gt_obj.GetPacket(id, &gt_pack) > 0) {
		PPGoodsTaxEntry item, * p_item;
		for(uint i = 0; gt_pack.enumItems(&i, (void**)&p_item);)
			P_Ary->insert(p_item);
		MEMSZERO(item);
		gt_pack.Rec.ToEntry(&item);
		item.Flags &= ~GTAXF_USELIST;
		P_Ary->insert(&item);
		ok = 1;
	}
	return ok;
}

int SLAPI GTaxCache::GetByID(PPID id, PPGoodsTaxEntry * pEntry)
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

int SLAPI GTaxCache::SearchEntry(PPID id, LDATE dt, PPID opID, PPGoodsTaxEntry * pEntry) const
{
	int    ok = 0;
	const  PPID _id = (id & 0x00ffffffL);
	PPGoodsTaxEntry * p_item;
	for(uint i = 0; !ok && P_Ary->enumItems(&i, (void**)&p_item);)
		if(p_item->OpID == opID && (p_item->TaxGrpID & 0x00ffffffL) == _id && p_item->Period.CheckDate(dt)) {
			ASSIGN_PTR(pEntry, *p_item);
			ok = 1;
		}
	return ok;
}

int SLAPI GTaxCache::Get(PPID id, LDATE dt, PPID opID, PPGoodsTaxEntry * pEntry)
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

// static
int SLAPI PPObjGoodsTax::Fetch(PPID id, LDATE dt, PPID opID, PPGoodsTaxEntry * pEntry)
{
	memzero(pEntry, sizeof(*pEntry));
	GTaxCache * p_cache = GetDbLocalCachePtr <GTaxCache> (PPOBJ_GOODSTAX);
	return p_cache ? p_cache->Get(id, dt, opID, pEntry) : 0;
}

// static
int SLAPI PPObjGoodsTax::FetchByID(PPID id, PPGoodsTaxEntry * pEntry)
{
	memzero(pEntry, sizeof(*pEntry));
	GTaxCache * p_cache = GetDbLocalCachePtr <GTaxCache> (PPOBJ_GOODSTAX);
	return p_cache ? p_cache->GetByID(id, pEntry) : 0;
}

