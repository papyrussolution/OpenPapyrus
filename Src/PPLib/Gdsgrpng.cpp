// GDSGRPNG.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
// @Kernel
// Группировка операций по товару
//
#include <pp.h>
#pragma hdrstop

GoodsGrpngEntry::GoodsGrpngEntry()
{
	THISZERO();
}

void FASTCALL GoodsGrpngEntry::SetOp(const PPOprKind & rOpRec)
{
	OpID     = rOpRec.ID;
	OpTypeID = rOpRec.OpTypeID;
	Link     = rOpRec.LinkOpID;
}

double FASTCALL GoodsGrpngEntry::IncomeByOpr(PPID opID) const
{
	const double income = (Price - Cost);
	return (IsExpendOp(opID) > 0) ? income : -income;
}

static PPID _IsProfitableOp(int byPayment, PPID opr, PPID oprt, PPID link)
{
	int    inc_calc_method;
	if(byPayment > 0)
		inc_calc_method = INCM_BYPAYMENT;
	else if(byPayment == 0)
		inc_calc_method = INCM_BYSHIPMENT;
	else
		inc_calc_method = CConfig.IncomeCalcMethod;
	if(opr && opr != -1 && opr != 10000 && IsIntrOp(opr) != INTRRCPT)
		switch(inc_calc_method) {
			case INCM_BYSHIPMENT:
				if(CheckOpFlags(opr, OPKF_PROFITABLE) && oprt != PPOPT_PAYMENT)
					return opr;
				break;
			case INCM_BYPAYMENT:
				if(oprt != PPOPT_PAYMENT) {
					if(oprt == PPOPT_GOODSRETURN)
						return 0;
					else if(CheckOpFlags(opr, OPKF_PROFITABLE, OPKF_NEEDPAYMENT))
						return opr;
				}
				else if(link && CheckOpFlags(link, OPKF_PROFITABLE))
					return link;
				break;
			case INCM_DEFAULT:
			default:
				if(CheckOpFlags(opr, OPKF_PROFITABLE))
					return (oprt == PPOPT_PAYMENT) ? link : opr;
				break;
		}
	return 0;
}

PPID FASTCALL GoodsGrpngEntry::IsProfitable(int incomeCalcMethod /*=-1*/) const
{
	return _IsProfitableOp(incomeCalcMethod, OpID, OpTypeID, Link);
}

double FASTCALL GoodsGrpngEntry::Income(int incomeCalcMethod /*=-1*/) const
{
	const  PPID op_id = IsProfitable(incomeCalcMethod);
	return op_id ? IncomeByOpr(op_id) : 0L;
}

/*static*/int GoodsGrpngEntry::GetSign(PPID opID, int16 rawSign, int * pSign)
{
	int    ok = -1;
	int    sign = 0;
	const  PPID op_id = opID;
	if(op_id == -1)
		sign = 1;
	else if(op_id == 10000)
		sign = -1;
	else {
		PPOprKind op_rec;
		int    op_type = GetOpType(op_id, &op_rec);
		if(op_type == PPOPT_GOODSMODIF)
			sign = NZOR(rawSign, +1);
		else {
			if(op_type == PPOPT_CORRECTION) {
				if(op_rec.LinkOpID && GetOpType(op_rec.LinkOpID) == PPOPT_GOODSEXPEND)
					sign = 1; // Корректировка расхода
				else 
					sign = -1; // Корректировка прихода
			}
			else {
				const int intr = IsIntrOp(op_id);
				if(oneof4(op_type, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN, PPOPT_GOODSREVAL) || intr) {
					sign = (IsExpendOp(op_id) > 0 || intr == INTREXPND) ? -1 : 1;
					ok = 1;
				}
			}
		}
	}
	ASSIGN_PTR(pSign, sign);
	return ok;
}

int FASTCALL GoodsGrpngEntry::GetSign(int * pSign) const
{
	return GoodsGrpngEntry::GetSign(OpID, Sign, pSign);
}

int GoodsGrpngEntry::Compose(const GoodsGrpngEntry & rAddendum)
{
	int    ok = 1;
	//int    this_sign = 0;
	//int    other_sign = 0;
	//GetSign(&this_sign);
	//rAddendum.GetSign(&other_sign);
	//const double qtty_sum = Quantity + rAddendum.Quantity;
	Quantity += rAddendum.Quantity;
	Cost += rAddendum.Cost;
	Price += rAddendum.Price;
	Discount += rAddendum.Discount;
	Amount += rAddendum.Amount;
	ExtCost += rAddendum.ExtCost;
	ExtPrice += rAddendum.ExtPrice;
	ExtDis += rAddendum.ExtDis;
	//CostPaymPart += rAddendum.CostPaymPart;
	return ok;
}

IMPL_CMPFUNC(GGAKey, i1, i2)
{
	const GoodsGrpngEntry * k1 = static_cast<const GoodsGrpngEntry *>(i1);
	const GoodsGrpngEntry * k2 = static_cast<const GoodsGrpngEntry *>(i2);
	if(k1->LotID < k2->LotID) return -1;
	if(k1->LotID > k2->LotID) return 1;
	if(k1->OpID < k2->OpID) return -1;
	if(k1->OpID > k2->OpID) return 1;
	if(k1->Sign < k2->Sign) return -1;
	if(k1->Sign > k2->Sign) return 1;

	if(k1->LotTaxGrpID < k2->LotTaxGrpID) return -1;
	if(k1->LotTaxGrpID > k2->LotTaxGrpID) return 1;
	if(k1->GoodsTaxGrpID < k2->GoodsTaxGrpID) return -1;
	if(k1->GoodsTaxGrpID > k2->GoodsTaxGrpID) return 1;
	const long m  = (GGEF_VATFREE|GGEF_TOGGLESTAX|GGEF_PRICEWOTAXES|GGEF_RECKONING|GGEF_LOCVATFREE|GGEF_PAYMBYPAYOUTLOT);
	const long f1 = (k1->Flags & m);
	const long f2 = (k2->Flags & m);
	if(f1 < f2)
		return -1;
	else if(f1 > f2)
		return 1;
	else
		return 0;
}

GoodsGrpngArray::GoodsGrpngArray(PPLogger * pLogger) : SVector(sizeof(GoodsGrpngEntry)),
	P_BObj(BillObj), ExtCostAmtID(0), ExtPriceAmtID(0), ExtDisAmtID(0), P_Logger(pLogger), ErrDetected(0), P_PplBlk(0)
{
}

GoodsGrpngArray::~GoodsGrpngArray()
{
	ZDELETE(P_PplBlk);
}

void GoodsGrpngArray::Reset()
{
	ExtCostAmtID = ExtPriceAmtID = ExtDisAmtID = 0;
	ErrDetected = 0;
	ZDELETE(P_PplBlk);
	clear();
}

int GoodsGrpngArray::WasErrDetected() const
{
	return ErrDetected;
}

GoodsGrpngEntry & FASTCALL GoodsGrpngArray::at(uint idx)
{
	return *static_cast<GoodsGrpngEntry *>(SVector::at(idx));
}

const GoodsGrpngEntry & FASTCALL GoodsGrpngArray::Get(uint idx) const
{
	return *static_cast<const GoodsGrpngEntry *>(SVector::at(idx));
}

int GoodsGrpngArray::Search(const GoodsGrpngEntry * pGGE, uint * p)
{
	GoodsGrpngEntry k;
	k = *pGGE;
	if(!(k.Flags & GGEF_DIFFBYTAX)) {
		k.LotTaxGrpID   = 0;
		k.GoodsTaxGrpID = 0;
		k.Flags = (k.Flags & ~(GGEF_VATFREE|GGEF_TOGGLESTAX|GGEF_PRICEWOTAXES|GGEF_COSTWSTAX|GGEF_LOCVATFREE));
	}
	return bsearch(&k, p, PTR_CMPFUNC(GGAKey));
}

int GoodsGrpngArray::Insert(const GoodsGrpngEntry * pEntry, uint * p)
{
	return ordInsert(pEntry, p, PTR_CMPFUNC(GGAKey)) ? 1 : PPSetErrorSLib();
}

const GoodsGrpngEntry * GoodsGrpngArray::GetInRest() const // @v11.5.11
{
	const GoodsGrpngEntry * p_result = 0;
	for(uint i = 0; !p_result && i < getCount(); i++) {
		const GoodsGrpngEntry & r_entry = Get(i);
		if(r_entry.IsInRest())
			p_result = &r_entry;
	}
	return p_result;
}

const GoodsGrpngEntry * GoodsGrpngArray::GetOutRest() const // @v11.5.11
{
	const GoodsGrpngEntry * p_result = 0;
	for(uint i = 0; !p_result && i < getCount(); i++) {
		const GoodsGrpngEntry & r_entry = Get(i);
		if(r_entry.IsOutRest())
			p_result = &r_entry;
	}
	return p_result;
}
int GoodsGrpngArray::GetInput(GoodsGrpngEntry & rEntry) const // @v11.5.11
{
	int    ok = -1;
	GoodsGrpngEntry result;
	for(uint i = 0; i < getCount(); i++) {
		const GoodsGrpngEntry & r_entry = Get(i);
		if(!r_entry.IsInRest() && !r_entry.IsOutRest()) {
			int   sign = 0;
			r_entry.GetSign(&sign);
			if(sign > 0) {
				result.Compose(r_entry);
				ok = 1;
			}
		}
	}
	return ok;
}

int GoodsGrpngArray::GetOutput(GoodsGrpngEntry & rEntry) const // @v11.5.11
{
	int    ok = -1;
	GoodsGrpngEntry result;
	for(uint i = 0; i < getCount(); i++) {
		const GoodsGrpngEntry & r_entry = Get(i);
		if(!r_entry.IsInRest() && !r_entry.IsOutRest()) {
			int   sign = 0;
			r_entry.GetSign(&sign);
			if(sign < 0) {
				result.Compose(r_entry);
				ok = 1;
			}
		}
	}
	return ok;
}

int AdjGdsGrpng::MakeBillIDList(const GCTFilt * pF, const PPIDArray * pOpList, int byReckon)
{
	int    ok = 1, r = 1;
	uint   i = 0;
	PPID * p_op_id, id;
	for(i = 0; r && pOpList->enumItems(&i, (void **)&p_op_id);) {
		//
		// Сначала найдем идентификаторы всех документов,
		// оплаты по которым попадают в заданный период и занесем
		// их в отсортированный массив links без дублирования.
		//
		BillTbl::Key2 k;
		uint   j;
		PPIDArray   links;
		BillCore  * p_t = BillObj->P_Tbl;
		BExtQuery q(p_t, 2, 64);
		DBQ * dbq = & (p_t->OpID == *p_op_id && daterange(p_t->Dt, &pF->Period));
		dbq = ppcheckfiltidlist(dbq, p_t->LocID, &pF->LocList.Get());
		q.select(p_t->ID, p_t->LinkBillID, p_t->LocID, 0L).where(*dbq);
		k.OpID   = *p_op_id;
		k.Dt     = pF->Period.low;
		k.BillNo = 0;
		for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;) {
			if(ObjRts.CheckLocID(p_t->data.LocID, 0)) {
				if(byReckon) {
					if(p_t->IsMemberOfPool(p_t->data.ID, PPASS_PAYMBILLPOOL, &(id = 0)) <= 0)
						id = 0;
				}
				else
					id = p_t->data.LinkBillID;
				if(id && !links.bsearch(id))
					THROW_SL(links.ordInsert(id, 0));
			}
		}
		//
		// Теперь из массива links выбираем идентификаторы только тех
		// документов, которые имеют дату, предшествующую заданному периоду
		// и одновременно засечем временной диапазон таких документов.
		//
		for(j = 0; j < links.getCount(); j++) {
			LDATE _d;
			id = links.at(j);
			BillTbl::Rec bill_rec;
			THROW(p_t->Search(id, &bill_rec) > 0);
			if(ObjRts.CheckLocID(bill_rec.LocID, 0)) {
				_d = bill_rec.Dt;
				if(_d < pF->Period.low && (!(pF->Flags & OPG_LABELONLY) ||
					(bill_rec.Flags & BILLF_WHITELABEL)) && pF->ArList.CheckID(bill_rec.Object)) {
					THROW_SL(BillList.ordInsert(id, 0));
					Period.AdjustToDate(_d);
				}
			}
		}
	}
	THROW(r);
	CATCHZOK
	return ok;
}

AdjGdsGrpng::AdjGdsGrpng()
{
	Period.Z();
}

int AdjGdsGrpng::CorrectionList(const GCTFilt & rF)
{
	int    ok = 1;
	PPID   op_id = 0;
	PPOprKind op_rec;
	PPIDArray op_list;
	PPObjBill * p_bobj = BillObj;
	BillCore * p_bc = p_bobj->P_Tbl;
	BillTbl::Rec bill_rec;
	CorrectionBillList.clear();
	for(op_id = 0; EnumOperations(PPOPT_CORRECTION, &op_id, &op_rec) > 0;) {
		PPOprKind link_op_rec;
		if(GetOpData(op_rec.LinkOpID, &link_op_rec) > 0) {
			if(link_op_rec.Flags & OPKF_PROFITABLE) {
				THROW_SL(op_list.add(op_rec.ID));
			}
			else if(rF.Flags & OPG_COSTBYPAYM && link_op_rec.OpTypeID == PPOPT_GOODSRECEIPT) {
				THROW_SL(op_list.add(op_rec.ID));
			}
		}
	}
	for(uint opidx = 0; opidx < op_list.getCount(); opidx++) {
		const  PPID correction_op_id = op_list.get(opidx);
		DateRange dr;
		dr.low = rF.Period.low;
		dr.upp = ZERODATE;
		for(SEnum en = p_bc->EnumByOp(correction_op_id, &dr, 0); en.Next(&bill_rec) > 0;) {
			const  PPID correction_bill_id = bill_rec.ID;
			if(bill_rec.LinkBillID) {
				if(p_bobj->Fetch(bill_rec.LinkBillID, &bill_rec) > 0 && rF.Period.CheckDate(bill_rec.Dt)) {
					CorrectionBillList.Add(bill_rec.ID, correction_bill_id);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int AdjGdsGrpng::PrevPaymentList(const GCTFilt & rF)
{
	int    ok = 1;
	PPID   op_id = 0;
	PPObjOprKind op_obj;
	PPOprKind op_rec;
	PPIDArray op_list;
	Period.low = MAXDATE;
	Period.upp = ZERODATE;
	BillList.freeAll();
	SupplAgentBillList.clear();
	//
	// Найдем все коды операций оплат по продажам приносящим доход
	//
	for(op_id = 0; EnumOperations(PPOPT_PAYMENT, &op_id, &op_rec) > 0;) {
		PPOprKind link_op_rec;
		if(GetOpData(op_rec.LinkOpID, &link_op_rec) > 0) {
			if(link_op_rec.Flags & OPKF_NEEDPAYMENT) {
				if(link_op_rec.Flags & OPKF_PROFITABLE) {
					THROW_SL(op_list.add(op_rec.ID));
				}
				else if(rF.Flags & OPG_COSTBYPAYM && link_op_rec.OpTypeID == PPOPT_GOODSRECEIPT) {
					THROW_SL(op_list.add(op_rec.ID));
				}
			}
		}
	}
	THROW(MakeBillIDList(&rF, &op_list, 0));
	if(rF.Flags & OPG_PROCESSRECKONING) {
		op_list.freeAll();
		//
		// Найдем коды зачитывающих оплат
		//
		for(op_id = 0; EnumOperations(0, &op_id, &op_rec) > 0;) {
			int    r = 0;
			if(op_rec.Flags & OPKF_RECKON) {
				if(op_rec.Flags & OPKF_PROFITABLE)
					r = 1;
				else if(rF.Flags & OPG_COSTBYPAYM && op_rec.OpTypeID == PPOPT_GOODSRECEIPT)
					r = 1;
			}
			if(r) {
				PPReckonOpEx rox;
				if(op_obj.GetReckonExData(op_rec.ID, &rox))
					op_list.addUnique(&rox.OpList);
			}
		}
		THROW(MakeBillIDList(&rF, &op_list, 1));
	}
	if(rF.SupplAgentID) {
		THROW(BillObj->P_Tbl->GetBillListByExt(rF.SupplAgentID, 0L, SupplAgentBillList));
		// @v8.1.0 (сортировку теперь выполняет GetBillListByExt) SupplAgentBillList.sort();
	}
	// AHTOXA {
	if(rF.BillList.IsExists()) {
		uint i = BillList.getCount();
		if(i) do {
			const  PPID _id = BillList.get(--i);
			if(!rF.BillList.CheckID(_id))
				BillList.atFree(i);
		} while(i);
	}
	// } AHTOXA
	CATCH
		ok = 0;
		BillList.freeAll();
	ENDCATCH
	return ok;
}

int AdjGdsGrpng::BeginGoodsGroupingProcess(const GCTFilt & rFilt)
{
	int    ok = 1;
	THROW(PrevPaymentList(rFilt));
	if(rFilt.Flags & OPG_MERGECORRECTION) {
		THROW(CorrectionList(rFilt));
	}
	CATCHZOK
	return ok;
}

int AdjGdsGrpng::EndGoodsGroupingProcess()
{
	return 1;
}
//
//
//
class GCT_Iterator {
public:
	GCT_Iterator(const GCTFilt & rF, const DateRange * pDR, const AdjGdsGrpng * pAgg);
	~GCT_Iterator();
	int    First(TransferTbl::Rec *);
	int    Next(TransferTbl::Rec *);
private:
	int    TrfrQuery(PPID lotID, TransferTbl::Rec * pOuterRec);
	int    AcceptTrfrRec(const TransferTbl::Rec *, TransferTbl::Rec * pOuterRec);
	GCTFilt Filt;
	DateRange Period;
	Transfer   * Trfr;
	BExtQuery  * trfr_q;
	BExtQuery  * rcpt_q;
	const AdjGdsGrpng * P_Agg;
};

GCT_Iterator::GCT_Iterator(const GCTFilt & rF, const DateRange * pDR, const AdjGdsGrpng * pAgg) : P_Agg(pAgg), trfr_q(0), rcpt_q(0), Filt(rF)
{
	if(!RVALUEPTR(Period, pDR))
		Period.Z();
	Trfr = BillObj->trfr;
}

GCT_Iterator::~GCT_Iterator()
{
	delete trfr_q;
	delete rcpt_q;
}

int GCT_Iterator::AcceptTrfrRec(const TransferTbl::Rec * pRec, TransferTbl::Rec * pOuterRec)
{
	int    ok = 1;
	PPID   lot_id = pRec->LotID;
	if(!Filt.BillList.CheckID(pRec->BillID) || (Filt.SupplAgentID && !lot_id))
		ok = -1;
	else if(!Filt.LotsPeriod.IsZero() || Filt.SupplAgentID) {
		ReceiptTbl::Rec lot_rec;
		while(lot_id && Trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
			if(Trfr->Rcpt.data.PrevLotID)
				lot_id = lot_rec.PrevLotID;
			else if(Filt.LotsPeriod.CheckDate(lot_rec.Dt) && (!Filt.SupplAgentID || (P_Agg && P_Agg->SupplAgentBillList.bsearch(lot_rec.BillID))))
				lot_id = 0;
			else {
				ok = -1;
				lot_id = 0;
			}
		}
		if(lot_id)
			ok = -1;
	}
	if(ok > 0)
		Trfr->copyBufTo(pOuterRec);
	return ok;
}

int GCT_Iterator::TrfrQuery(PPID lotID, TransferTbl::Rec * pOuterRec)
{
	Transfer * p_tfr = Trfr;
	BExtQuery * q = new BExtQuery(p_tfr, lotID ? 2 : (Filt.GoodsID ? 3 : 1), 128);
	if(q == 0)
		return PPSetErrorNoMem();
	else {
		union {
			TransferTbl::Key1 k1;
			TransferTbl::Key2 k2;
			TransferTbl::Key3 k3;
		} k;
		MEMSZERO(k);
		DBQ  * dbq = 0;
		if(lotID) {
			dbq = &(p_tfr->LotID == lotID && daterange(p_tfr->Dt, &Period));
			k.k2.LotID = lotID;
			k.k2.Dt = Period.low;
		}
		else if(Filt.GoodsID) {
			dbq = &(p_tfr->GoodsID == Filt.GoodsID && daterange(p_tfr->Dt, &Period));
			k.k3.GoodsID = Filt.GoodsID;
			k.k3.Dt = Period.low;
		}
		else {
			dbq = & daterange(p_tfr->Dt, &Filt.Period);
			k.k1.Dt = Filt.Period.low;
		}
		if(!lotID && !Filt.LocList.IsEmpty())
			dbq = ppcheckfiltidlist(dbq, p_tfr->LocID, &Filt.LocList.Get());
		q->select(p_tfr->Dt, p_tfr->BillID, p_tfr->LotID, p_tfr->LocID, p_tfr->Flags, p_tfr->Quantity,
			p_tfr->Rest, p_tfr->Cost, p_tfr->Price, p_tfr->Discount, 0L).where(*dbq);
		delete trfr_q;
		trfr_q = q;
		for(trfr_q->initIteration(false, &k, spGt); trfr_q->nextIteration() > 0;)
			if(AcceptTrfrRec(&p_tfr->data, pOuterRec) > 0)
				return 1;
		return -1;
	}
}

int GCT_Iterator::First(TransferTbl::Rec * pRec)
{
	DBQ  * dbq = 0;
	PPID   lot_id = 0;
	ReceiptTbl * rt = & Trfr->Rcpt;
	if(Filt.SupplID) {
   		struct {
			union {
				PPID goods;
			   	PPID suppl;
			};
			LDATE dt;
			long  oprno;
	   	} rk; // #2 : #5
		if((rcpt_q = new BExtQuery(rt, Filt.GoodsID ? 2 : 5, 256)) == 0)
			return PPSetErrorNoMem();
		if(Filt.GoodsID) {
	   	    dbq = &(rt->GoodsID == Filt.GoodsID);
			if(Period.upp)
				dbq = &(*dbq && rt->Dt <= Period.upp);
			dbq = &(*dbq && rt->SupplID == Filt.SupplID);
			rk.goods = Filt.GoodsID;
		}
		else {
   		    dbq = &(rt->SupplID == Filt.SupplID);
			if(Period.upp)
				dbq = &(*dbq && rt->Dt <= Period.upp);
   	    	rk.suppl = Filt.SupplID;
		}
	   	rk.dt = ZERODATE;
		rk.oprno = 0;
		dbq = ppcheckfiltidlist(dbq, rt->LocID, &Filt.LocList.Get());
		rcpt_q->select(rt->ID, 0L).where(*dbq);
		rcpt_q->initIteration(false, &rk, spGt);
	}
	if(!rcpt_q || rcpt_q->nextIteration() > 0)
		do {
			if(rcpt_q)
				lot_id = rt->data.ID;
	   		if(TrfrQuery(lot_id, pRec) > 0)
				return 1;
		} while(rcpt_q && rcpt_q->nextIteration() > 0);
	return -1;
}

int GCT_Iterator::Next(TransferTbl::Rec * pRec)
{
	while(trfr_q->nextIteration() > 0)
		if(AcceptTrfrRec(&Trfr->data, pRec) > 0)
			return 1;
	while(rcpt_q && rcpt_q->nextIteration() > 0)
   		if(TrfrQuery(Trfr->Rcpt.data.ID, pRec) > 0)
			return 1;
	return -1;
}
//
//
//
int GoodsGrpngArray::CalcRest(GoodsRestParam & rP, const PPOprKind & rOpRec, double phuperu)
{
	if(P_BObj->trfr->GetRest(rP)) {
		GoodsGrpngEntry entry;
		entry.SetOp(rOpRec);
		entry.Quantity = rP.Total.Rest;
		entry.Volume   = rP.Total.Rest * phuperu;
		entry.Cost     = rP.Total.Cost;
		entry.Price    = rP.Total.Price;
		entry.Discount = rP.Total.Count;
		entry.Amount   = 0;
		entry.Sign     = 1;
		return AddEntry(&entry);
	}
	else
		return 0;
}

int FASTCALL GoodsGrpngArray::AddEntry(GoodsGrpngEntry * pEntry)
{
	uint   pos = 0;
	const  PPID   lot_tax_grp_id = pEntry->LotTaxGrpID;
	const  PPID   goods_tax_grp_id = pEntry->GoodsTaxGrpID;
	const  long   flags = pEntry->Flags;
	const  double qtty = pEntry->Quantity;
	if(flags & GGEF_CALCBYPRICE) {
		const int cfg_price_wo_excise = BIN(CConfig.Flags & CCFLG_PRICEWOEXCISE);
		// @v11.5.5 pEntry->Cost     *= qtty;
		// @v11.5.5 pEntry->Price    *= qtty;
		// @v11.5.5 pEntry->Discount *= qtty;
		if(flags & GGEF_COSTWOVAT)
			GObj.AdjCostToVat(lot_tax_grp_id, goods_tax_grp_id, pEntry->LotDate, pEntry->TaxFactor, &pEntry->Cost, 1);
		if(flags & GGEF_SETCOSTWOTAXES)
			GObj.AdjCostToVat(lot_tax_grp_id, goods_tax_grp_id, pEntry->LotDate, pEntry->TaxFactor, &pEntry->Cost, 0, BIN(flags & GGEF_VATFREE));
		if(flags & GGEF_PRICEWOTAXES) {
			int    excl_stax = 0;
			int    re = BIN(flags & GGEF_TOGGLESTAX);
			if(pEntry->OpID != -1L && pEntry->OpID != 10000L) {
				pEntry->Price -= pEntry->Discount;
				pEntry->Discount = 0.0;
			}
			if(cfg_price_wo_excise ? !re : re)
				excl_stax = 1;
			GObj.AdjPriceToTaxes(goods_tax_grp_id, pEntry->TaxFactor, &pEntry->Price, excl_stax);
		}
		// @v10.6.6 {
		if(flags & GGEF_SETPRICEWOTAXES_) {
			//
			// Расчет цены реализации без НДС
			//
			const long amt_fl = cfg_price_wo_excise ? ~GTAXVF_SALESTAX : GTAXVF_BEFORETAXES; // ! не уверен в этой строке в контексте блока if(flags & GGEF_PRICEWOTAXES) {}
			GTaxVect vect;
			PPGoodsTaxEntry gtx;
			if(GObj.GTxObj.FetchByID(goods_tax_grp_id, &gtx) > 0) {
				const int discount_sign = fsign(pEntry->Discount);
				vect.Calc_(&gtx, fabs(pEntry->Price), pEntry->TaxFactor, amt_fl, 0);
				pEntry->Price = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_EXCISE);
				vect.Calc_(&gtx, fabs(pEntry->Discount), pEntry->TaxFactor, amt_fl, 0);
				pEntry->Discount = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_EXCISE) * discount_sign;
			}
		}
		// } @v10.6.6
		pEntry->Cost     *= qtty; // @v11.5.5 
		pEntry->Price    *= qtty; // @v11.5.5 
		pEntry->Discount *= qtty; // @v11.5.5 
	}
	if(Search(pEntry, &pos)) {
		GoodsGrpngEntry & e = at(pos);
		e.Quantity += qtty;
		e.Volume   += pEntry->Volume;
		e.TaxFactor += pEntry->TaxFactor;
		e.Cost     += pEntry->Cost;
		e.Price    += pEntry->Price;
		e.Amount   += pEntry->Amount;
		e.ExtCost  += pEntry->ExtCost;
		e.ExtPrice += pEntry->ExtPrice;
		e.ExtDis   += pEntry->ExtDis;
		if(oneof2(pEntry->OpID, -1L, 10000L)) {
			e.Discount = 0.0;
			e.Count += static_cast<long>(pEntry->Discount);
		}
		else {
			e.Discount += pEntry->Discount;
			e.Count++;
			e.LnCount += pEntry->LnCount;
			e.AvgLn = (e.Count == 0) ? 0 : (e.LnCount / e.Count);
		}
	}
	else {
		GoodsGrpngEntry entry;
		entry.LotID    = pEntry->LotID;
		entry.OpID     = pEntry->OpID;
		entry.Sign     = pEntry->Sign;
		if(flags & GGEF_DIFFBYTAX) {
			entry.LotDate       = pEntry->LotDate;
			entry.LotTaxGrpID   = lot_tax_grp_id;
			entry.GoodsTaxGrpID = goods_tax_grp_id;
			entry.Flags = (flags & ~(GGEF_PRICEWOTAXES | GGEF_INTERNAL));
		}
		else
			entry.Flags = (flags & ~(GGEF_VATFREE|GGEF_TOGGLESTAX|GGEF_PRICEWOTAXES|GGEF_INTERNAL|GGEF_LOCVATFREE));
		entry.OpTypeID = pEntry->OpTypeID;
		entry.Link     = pEntry->Link;
		entry.Quantity = qtty;
		entry.Volume   = pEntry->Volume;
		entry.TaxFactor = pEntry->TaxFactor;
		entry.Cost     = pEntry->Cost;
		entry.Price    = pEntry->Price;
		entry.Amount   = pEntry->Amount;
		entry.ExtCost  = pEntry->ExtCost;
		entry.ExtPrice = pEntry->ExtPrice;
		entry.ExtDis   = pEntry->ExtDis;
		if(entry.OpID == -1L || entry.OpID == 10000L) {
			entry.Discount = 0.0;
			entry.Count = static_cast<long>(pEntry->Discount);
		}
		else {
			entry.Discount = pEntry->Discount;
			entry.LnCount = pEntry->LnCount;
			entry.Count    = 1;
			entry.AvgLn = entry.LnCount;
		}
		if(!Insert(&entry, 0))
			return 0;
	}
	return 1;
}

GoodsGrpngArray::AddEntryBlock::AddEntryBlock() : Part(0.0), Flags(0)
{
}

int GoodsGrpngArray::Calc_(const GCTFilt & rF, const AdjGdsGrpng * pAgg, TransferTbl::Rec * pTrfrRec, PPID taxGrpID, double phuperu, double taxFactor)
{
	int    ok = 1, r;
	BillCore * p_bc = P_BObj->P_Tbl;
	AddEntryBlock blk;
	// @v10.6.8 @ctr MEMSZERO(blk);
	blk.Part = 1.0;
	blk.TrfrRec = *pTrfrRec;
	if(rF.Flags & OPG_SETCOSTWOTAXES)
		blk.Flags |= GGEF_SETCOSTWOTAXES;
	// @v10.6.6 {
	if(rF.Flags & OPG_SETPRICEWOTAXES)
		blk.Flags |= GGEF_SETPRICEWOTAXES_;
	// } @v10.6.6
	if(rF.Flags & OPG_DIFFBYTAX)
		blk.Flags |= GGEF_DIFFBYTAX;
	if(rF.Flags & OPG_PROCESSBYLOTS)
		blk.Flags |= GGEF_BYLOT;
	if(rF.Flags & OPG_COSTBYPAYM)
		blk.Flags |= GGEF_COSTBYPAYM;
	const  PPID bill_id = pTrfrRec->BillID;
	BillTbl::Rec bill_rec;
	THROW(r = P_BObj->Fetch(bill_id, &bill_rec));
	if(r < 0) {
		if(P_Logger)
			P_Logger->LogLastError();
		else
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_USER|LOGMSGF_TIME);
		++ErrDetected;
	}
	else {
		const  PPID   op_id  = bill_rec.OpID;
		const  double amount = BR2(bill_rec.Amount);
		if((!(rF.Flags & OPG_LABELONLY) || (bill_rec.Flags & BILLF_WHITELABEL)) && ObjRts.CheckOpID(op_id, PPR_READ)) {
			const int r2 = rF.AcceptIntr3(bill_rec);
			if(r2) {
				PPID   l_tax_grp_id = 0;
				PPGoodsTaxEntry  gtx;
				TSArray <AddEntryBlock> blk_list;
				THROW((r = P_BObj->trfr->Rcpt.Search(pTrfrRec->LotID, &blk.LotRec)));
				if(r < 0)
					MEMSZERO(blk.LotRec);
				SETFLAG(pTrfrRec->Flags, PPTFR_COSTWOVAT, blk.LotRec.Flags & LOTF_COSTWOVAT);
				THROW(GetOpData(op_id, &blk.OpRec));
				if(pTrfrRec->Flags & PPTFR_REVAL) {
					TransferTbl::Rec tr = *pTrfrRec;
					if(!(pTrfrRec->Flags & PPTFR_CORRECTION)) {
						P_BObj->trfr->GetLotPrices(&blk.LotRec, tr.Dt/*, tr.OprNo*/);
						tr.Quantity = tr.Rest;
						tr.Cost  = TR5(blk.LotRec.Cost  - tr.Cost);
						tr.Price = TR5(blk.LotRec.Price - tr.Price);
					}
					blk.TrfrRec = tr;
				}
				// @v10.7.5 {
				else if(rF.Flags & OPG_MERGECORRECTION) {
					uint cid_pos = 0;
					long cid = 0;
					if(!pAgg || pAgg->CorrectionBillList.Search(bill_id, &cid, &cid_pos)) {
						PPIDArray cor_bill_list;
						for(DateIter di(/*&pFilt->Period*/0); p_bc->EnumLinks(bill_id, &di, BLNK_CORRECTION, &bill_rec) > 0;) {
							cor_bill_list.add(bill_rec.ID);
						}
						if(cor_bill_list.getCount()) {
							PPBillPacket cor_bp;
							for(uint i = 0; i < cor_bill_list.getCount(); i++) {
								const  PPID cor_bill_id = cor_bill_list.get(i);
								THROW(P_BObj->ExtractPacket(cor_bill_id, &cor_bp) > 0);
								for(uint j = 0; j < cor_bp.GetTCount(); j++) {
									const PPTransferItem & r_other = cor_bp.ConstTI(j);
									if(r_other.LotID == pTrfrRec->LotID) {
										blk.TrfrRec.Quantity = r_other.Qtty();
										blk.TrfrRec.Cost = r_other.Cost;
										blk.TrfrRec.Price = r_other.Price;
										break;
									}
								}
							}
						}
					}
				}
				// } @v10.7.5
				if(PPObjGoodsTax::Fetch(taxGrpID, pTrfrRec->Dt, op_id, &gtx) > 0)
					l_tax_grp_id = gtx.TaxGrpID;
				if(!(rF.Flags & OPG_ADJPAYM)) {
					if(r2 == 3) {
						blk.OpRec.ID = PPOPK_INTRRECEIPT;
						blk.OpRec.OpTypeID = PPOPT_GOODSRECEIPT;
						blk.OpRec.LinkOpID = 0;
						blk.Flags |= GGEF_SUPPRDISCOUNT;
						THROW_SL(blk_list.insert(&blk));
						blk.OpRec.ID = op_id;
						blk.Flags &= ~GGEF_SUPPRDISCOUNT;
					}
					else if(r2 == 2) {
						THROW_SL(blk_list.insert(&blk));
						blk.OpRec.ID = PPOPK_INTRRECEIPT;
						blk.OpRec.OpTypeID = PPOPT_GOODSRECEIPT;
						blk.OpRec.LinkOpID = 0;
						blk.Flags |= GGEF_INTRREVERSE;
						blk.Flags |= GGEF_SUPPRDISCOUNT;
						THROW_SL(blk_list.insert(&blk));
						blk.OpRec.ID = op_id;
						blk.Flags &= ~(GGEF_SUPPRDISCOUNT|GGEF_INTRREVERSE);
					}
					else {
						THROW_SL(blk_list.insert(&blk));
					}
				}
				if(blk.OpRec.Flags & OPKF_NEEDPAYMENT && amount != 0.0) {
					const  PPID link_op = blk.OpRec.ID;
					const  PPID bill_id = pTrfrRec->BillID;
					for(DateIter di(&rF.Period); p_bc->EnumLinks(bill_id, &di, BLNK_PAYMENT, &bill_rec) > 0;) {
						GetOpData(bill_rec.OpID, &blk.OpRec);
						if(blk.OpRec.OpTypeID != PPOPT_PAYMENT || !IsLockPaymStatus(bill_rec.StatusID)) {
							// @v10.7.6 {
							int    do_skip_paym = 0;
							if(rF.Flags & OPG_MERGECORRECTION) {
								PPID   pool_id = 0;
								if(P_BObj->IsMemberOfPool(bill_rec.ID, PPASS_PAYMBILLPOOL, &pool_id) > 0) {
									BillTbl::Rec pool_bill_rec;
									if(P_BObj->Fetch(pool_id, &pool_bill_rec) > 0 && GetOpType(pool_bill_rec.OpID) == PPOPT_CORRECTION)
										do_skip_paym = 1;
								}
							}
							// } @v10.7.6
							if(!do_skip_paym) {
								blk.Part = round(BR2(bill_rec.Amount) / amount, 12);
								THROW_SL(blk_list.insert(&blk));
							}
						}
					}
					if(rF.Flags & OPG_PROCESSRECKONING) {
						for(PPID reck_id = 0; p_bc->EnumMembersOfPool(PPASS_PAYMBILLPOOL, bill_id, &reck_id) > 0;) {
							if(P_BObj->Fetch(reck_id, &bill_rec) > 0 && rF.Period.CheckDate(bill_rec.Dt)) {
								GetOpData(bill_rec.OpID, &blk.OpRec);
								blk.Part = round(BR2(bill_rec.Amount) / amount, 12);
								// Искусственно устанавливаем связанную операцию, указывающую на зачетную операцию (отгрузка)
								const  PPID save_link_op = blk.OpRec.LinkOpID;
								const long save_blk_fl = blk.Flags;
								blk.OpRec.LinkOpID = link_op;
								blk.Flags |= GGEF_RECKONING;
								THROW_SL(blk_list.insert(&blk));
								blk.OpRec.LinkOpID = save_link_op;
								blk.Flags = save_blk_fl;
							}
						}
					}
				}
				for(uint i = 0; i < blk_list.getCount(); i++) {
					const AddEntryBlock & r_blk = blk_list.at(i);
					double cost_paym_part = 1.0;
					GoodsGrpngEntry entry;
					entry.Flags = GGEF_CALCBYPRICE;
					if(r_blk.TrfrRec.Flags & PPTFR_COSTWOVAT)
						entry.Flags |= GGEF_COSTWOVAT;
					else if(r_blk.TrfrRec.Flags & PPTFR_COSTWSTAX)
						entry.Flags |= GGEF_COSTWSTAX;
					if(IsLotVATFree(r_blk.LotRec))
						entry.Flags |= GGEF_VATFREE;
					if(PPObjLocation::CheckWarehouseFlags(r_blk.TrfrRec.LocID, LOCF_VATFREE))
						entry.Flags |= GGEF_LOCVATFREE;
					if(r_blk.TrfrRec.Flags & PPTFR_RMVEXCISE)
						entry.Flags |= GGEF_TOGGLESTAX;
					if(r_blk.TrfrRec.Flags & PPTFR_PRICEWOTAXES)
						entry.Flags |= GGEF_PRICEWOTAXES;
					if(r_blk.Flags & GGEF_DIFFBYTAX)
						entry.Flags |= GGEF_DIFFBYTAX;
					if(r_blk.Flags & GGEF_SETCOSTWOTAXES)
						entry.Flags |= GGEF_SETCOSTWOTAXES;
					// @v10.6.6 {
					if(r_blk.Flags & GGEF_SETPRICEWOTAXES_)
						entry.Flags |= GGEF_SETPRICEWOTAXES_;
					// } @v10.6.6
					if(r_blk.Flags & GGEF_RECKONING)
						entry.Flags |= GGEF_RECKONING;
					if(r_blk.Flags & GGEF_INTRREVERSE)
						entry.Flags |= GGEF_INTRREVERSE;
					if(r_blk.Flags & GGEF_SUPPRDISCOUNT)
						entry.Flags |= GGEF_SUPPRDISCOUNT;
					entry.SetOp(r_blk.OpRec);
					entry.LotID    = (r_blk.Flags & GGEF_BYLOT) ? r_blk.LotRec.ID : 0;
					entry.Quantity = fabs(r_blk.TrfrRec.Quantity);
					entry.Sign     = (r_blk.TrfrRec.Flags & PPTFR_PLUS) ? 1 : ((r_blk.TrfrRec.Flags & PPTFR_MINUS) ? -1 : 0);
					if(entry.Sign == 0) {
						//
						// Аварийный случай: знак операции не удается определить по флагам.
						//
						entry.Sign = (r_blk.TrfrRec.Quantity >= 0.0) ? 1 : -1;
					}
					if(r_blk.Part != 1.0)
						entry.Quantity *= r_blk.Part;
					entry.GoodsTaxGrpID = l_tax_grp_id;
					entry.Cost     = TR5(r_blk.TrfrRec.Cost);
					if(r_blk.Flags & GGEF_COSTBYPAYM && r_blk.TrfrRec.LotID) {
						if(r_blk.TrfrRec.Flags & PPTFR_RECEIPT /*&& r_blk.TrfrRec.Reverse == 0*/ && GetOpType(entry.OpID) == PPOPT_PAYMENT) {
							PPObjBill::EprBlock epr;
							DateRange _p;
							_p.Set(ZERODATE, plusdate(rF.Period.low, -1));
							P_BObj->GetExpendedPartOfReceipt(r_blk.TrfrRec.LotID, &_p, 0, epr);
							if(epr.Amount != 0.0) {
								entry.Cost *= fabs(epr.Payout / epr.Amount);
								entry.Flags |= GGEF_PAYMBYPAYOUTLOT;
							}
						}
						else {
							double part = 1.0;
							THROW_MEM(SETIFZ(P_PplBlk, new PPObjBill::PplBlock(rF.Period, 0, 0)));
							P_BObj->GetPayoutPartOfLot(r_blk.TrfrRec.LotID, *P_PplBlk, &part);
							entry.Cost *= part;
						}
					}
					entry.Price    = TR5(r_blk.TrfrRec.Price);
					entry.Discount = TR5(r_blk.TrfrRec.Discount);
					entry.Volume    = entry.Quantity * phuperu;
					entry.TaxFactor = entry.Quantity * taxFactor;
					P_BObj->trfr->Rcpt.GetOriginDate(&r_blk.LotRec, &entry.LotDate);
					PPGoodsTaxEntry  gtx;
					if(PPObjGoodsTax::Fetch(NZOR(r_blk.LotRec.InTaxGrpID, l_tax_grp_id), entry.LotDate, 0, &gtx) > 0)
						entry.LotTaxGrpID = gtx.TaxGrpID;
					//
					if(entry.Flags & GGEF_SUPPRDISCOUNT) {
						entry.Price -= entry.Discount;
						entry.Discount = 0.0;
					}
					//
					THROW(AddEntry(&entry));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

struct AddBillEntry {
	AddBillEntry * Init(PPID billID, const PPOprKind & rOpRec, PPID linkBillOpID, double amt, double part)
	{
		BillID = billID;
		OpID = rOpRec.ID;
		OpTypeID = rOpRec.OpTypeID;
		OpLinkOpID = rOpRec.LinkOpID;
		ReckonOpID = linkBillOpID;
		Flags = 0;
		Amount = amt;
		Part = part;
		return this;
	}
	AddBillEntry * InitIntrReverse(PPID billID, double amt)
	{
		BillID = billID;
		OpID = PPOPK_INTRRECEIPT;
		OpTypeID = PPOPT_GOODSRECEIPT;
		OpLinkOpID = 0;
		ReckonOpID = 0;
		Flags = fIntrReverseReverse;
		Amount = amt;
		Part = 1.0;
		return this;
	}

	enum {
		fIntrReverseReverse = 0x0001,
		fSuppressDiscount   = 0x0002
	};
	PPID   BillID;
	PPID   OpID;
	PPID   OpTypeID;
	PPID   OpLinkOpID;
	PPID   ReckonOpID;
	long   Flags;
	double Amount;
	double Part;
};

int GoodsGrpngArray::_ProcessBillGrpng(GCTFilt * pFilt)
{
	int    ok = 1, idx;
	IterCounter counter;
	SString wait_msg, period_buf;
	PPID   op_type_id;
	BillCore * p_bill = P_BObj->P_Tbl;
	DBQ * dbq = 0;
	TSArray <AddBillEntry> bill_entry_list;
	AddBillEntry bill_entry;
	union {
		BillTbl::Key1 k1;
		BillTbl::Key3 k3;
	} k, k_;
	const  PPID single_ar_id = pFilt->ArList.GetSingle();
	if(single_ar_id) {
		//
		// Начиная с v1.9.2 при фильтрации по объекту эта функция возможно
		// будет выдавать не совсем корректные результаты поскольку оплаты
		// могут в качестве объекта содержать значение, отличное от
		// связанного документа.
		// Сразу эта некорректность не исправлена из-за того, что
		// фактически эта функция никогда не вызывается с фильтрацией по
		// объекту (см. GoodsGrpngArray::ProcessGoodsGrouping).
		//
		dbq = & (p_bill->Object == single_ar_id);
		idx = 3;
		k.k3.Object = single_ar_id;
		k.k3.Dt     = pFilt->Period.low;
		k.k3.BillNo = 0;
	}
	else {
		idx = 1;
		k.k1.Dt = pFilt->Period.low;
		k.k1.BillNo = 0;
	}
	if(!pFilt->ShipmentPeriod.IsZero())
		dbq = & (*dbq && (daterange(p_bill->Dt, &pFilt->Period) || daterange(p_bill->Dt, &pFilt->ShipmentPeriod)));
	else
		dbq = & (*dbq && daterange(p_bill->Dt, &pFilt->Period));
	BExtQuery q(p_bill, idx, 256);
	q.select(p_bill->ID, p_bill->OpID, p_bill->Dt, p_bill->LocID, p_bill->Amount,
		p_bill->Object, p_bill->LinkBillID, p_bill->Flags, p_bill->StatusID, 0L).where(*dbq);
	k_ = k;
	counter.Init(q.countIterations(0, &k_, spGt));
	PPLoadTextS(PPTXT_CALCOPGRPNG, wait_msg).CatDiv(':', 1).Cat(pFilt->Period, 1);
	for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;) {
		BillTbl::Rec bill_rec;
		p_bill->copyBufTo(&bill_rec);
		PPID op_id = bill_rec.OpID;
		if((!(pFilt->Flags & OPG_LABELONLY) || (bill_rec.Flags & BILLF_WHITELABEL)) && ObjRts.CheckOpID(op_id, PPR_READ) && pFilt->BillList.CheckID(bill_rec.ID)) {
			PPOprKind op_rec;
			op_type_id = GetOpType(op_id);
			if(oneof6(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN, PPOPT_PAYMENT, PPOPT_GOODSREVAL, PPOPT_GOODSMODIF)) {
				const int r2 = pFilt->AcceptIntr3(bill_rec);
				if(r2) {
					op_id = bill_rec.OpID; // Функция AcceptIntr2 могла изменить вид операции записи
					DateRange shpm_prd = pFilt->ShipmentPeriod;
					DateRange period   = pFilt->Period;
					double amount = BR2(bill_rec.Amount);
					const  int calc_bill_rows = BIN(pFilt->Flags & OPG_CALCBILLROWS);
					GetOpData(op_id, &op_rec);
					if(IsOpPaym(op_id)) {
						if((shpm_prd.IsZero() || period.CheckDate(bill_rec.Dt)) && !IsLockPaymStatus(bill_rec.StatusID)) {
							int    r = 1;
							double part = 1.0;
							PPID   pool_id = 0, id = 0;
							BillTbl::Rec link_bill_rec;
							THROW(r = P_BObj->Fetch(bill_rec.LinkBillID, &link_bill_rec));
							if(shpm_prd.CheckDate(link_bill_rec.Dt)) {
								if(r > 0) {
									part = fdivnz(amount, BR2(link_bill_rec.Amount));
									id = link_bill_rec.ID;
								}
								bill_entry_list.insert(bill_entry.Init(id, op_rec, 0, amount, part));
								if(pFilt->Flags & OPG_PROCESSRECKONING) {
									if(P_BObj->IsMemberOfPool(bill_rec.ID, PPASS_PAYMBILLPOOL, &pool_id) > 0) {
										THROW(r = P_BObj->Fetch(pool_id, &link_bill_rec));
										if(r > 0) {
											part = fdivnz(amount, BR2(link_bill_rec.Amount));
											id = link_bill_rec.ID;
										}
										else {
											part = 1.0;
											id = 0;
										}
										bill_entry_list.insert(bill_entry.Init(id, op_rec, link_bill_rec.OpID, amount, part));
									}
								}
							}
						}
					}
					else if(shpm_prd.CheckDate(bill_rec.Dt)) {
						if(r2 == 3) {
							op_rec.ID = PPOPK_INTRRECEIPT;
							op_rec.OpTypeID = PPOPT_GOODSRECEIPT;
							op_rec.LinkOpID = 0;
							bill_entry.InitIntrReverse(bill_rec.ID, amount);
							bill_entry.Flags &= ~AddBillEntry::fIntrReverseReverse;
							bill_entry.Flags |= AddBillEntry::fSuppressDiscount;
							bill_entry_list.insert(&bill_entry);
						}
						else {
							bill_entry.Init(bill_rec.ID, op_rec, 0, amount, 1.0);
							if(r2 == 2) {
								//bill_entry.Flags |= AddBillEntry::fSuppressDiscount;
								bill_entry_list.insert(&bill_entry);
								op_rec.ID = PPOPK_INTRRECEIPT;
								op_rec.OpTypeID = PPOPT_GOODSRECEIPT;
								op_rec.LinkOpID = 0;
								bill_entry.InitIntrReverse(bill_rec.ID, amount);
								bill_entry.Flags |= AddBillEntry::fSuppressDiscount;
								bill_entry_list.insert(&bill_entry);
							}
							else {
								bill_entry_list.insert(&bill_entry);
							}
						}
					}
				}
			}
			else if((pFilt->OpID || (pFilt->Flags & OPG_INCLACCOPS)) && op_type_id == PPOPT_ACCTURN) {
				const int r2 = pFilt->AcceptIntr3(bill_rec);
				if(r2) {
					op_id = bill_rec.OpID;
					double amount = BR2(bill_rec.Amount);
					GetOpData(op_id, &op_rec);
					bill_entry.Init(bill_rec.ID, op_rec, 0, amount, 1.0);
					bill_entry_list.insert(&bill_entry);
				}
			}
		}
		if(!(pFilt->Flags & OPG_DONTSHOWPRGRSBAR))
			PPWaitPercent(counter.Increment(), wait_msg);
	}
	for(uint i = 0; i < bill_entry_list.getCount(); i++) {
		const AddBillEntry & r_bill_entry = bill_entry_list.at(i);
		GoodsGrpngEntry entry;
		entry.OpID = r_bill_entry.OpID;
		entry.OpTypeID = r_bill_entry.OpTypeID;
		entry.Link = r_bill_entry.OpLinkOpID;
		entry.Amount = r_bill_entry.Amount;
		if(r_bill_entry.ReckonOpID) {
			entry.Link = r_bill_entry.ReckonOpID;
			entry.Flags |= GGEF_RECKONING;
		}
		if(r_bill_entry.BillID) {
			AmtList amt_list;
			P_BObj->P_Tbl->GetAmountList(r_bill_entry.BillID, &amt_list);
			entry.Cost     = amt_list.Get(PPAMT_BUYING,   0L/*@curID*/) * r_bill_entry.Part;
			entry.Price    = amt_list.Get(PPAMT_SELLING,  0L/*@curID*/) * r_bill_entry.Part;
			entry.Discount = amt_list.Get(PPAMT_DISCOUNT, 0L/*@curID*/) * r_bill_entry.Part;
			if(r_bill_entry.Flags & AddBillEntry::fIntrReverseReverse) {
				entry.Flags |= GGEF_INTRREVERSE;
			}
			if(r_bill_entry.Flags & AddBillEntry::fSuppressDiscount) {
				entry.Price -= entry.Discount;
				entry.Discount = 0.0;
			}
			if(ExtCostAmtID)
				entry.ExtCost   = amt_list.Get(ExtCostAmtID,  0L/*@curID*/) * r_bill_entry.Part;
			if(ExtPriceAmtID)
				entry.ExtPrice  = amt_list.Get(ExtPriceAmtID, 0L/*@curID*/) * r_bill_entry.Part;
			if(ExtDisAmtID) {
				entry.ExtDis    = amt_list.Get(ExtDisAmtID,   0L/*@curID*/) * r_bill_entry.Part;
				entry.ExtPrice -= entry.ExtDis;
			}
		}
		if(pFilt->Flags & OPG_CALCBILLROWS) {
			Transfer::BillTotal bt;
			MEMSZERO(bt);
			P_BObj->trfr->CalcBillTotal(r_bill_entry.BillID, &bt, 0);
			entry.LnCount = bt.LineCount;
		}
		THROW(AddEntry(&entry));
	}
	bill_entry_list.freeAll();
	//
	if(pFilt->Flags & (OPG_CALCINREST|OPG_CALCOUTREST)) {
		GoodsGrpngEntry entry;
		GoodsRestTotal  gr_total;
		GoodsRestFilt   gr_flt;
		PPViewGoodsRest gr_view;
		PPOprKind       r_op_rec;
		// @v10.6.4 MEMSZERO(r_op_rec);
		if(pFilt->Flags & OPG_CALCINREST) {
			r_op_rec.ID = -1;
			entry.SetOp(r_op_rec);
			if(pFilt->Period.low) {
				gr_flt.Init(1, 0);
				gr_flt.Date   = pFilt->Period.low;
				plusdate(&gr_flt.Date, -1, 0);
				gr_flt.LocList = pFilt->LocList; // AHTOXA
				gr_flt.Flags |= GoodsRestFilt::fCalcTotalOnly;
				gr_flt.WaitMsgID = PPTXT_CALCINREST;
				THROW(gr_view.Init_(&gr_flt));
				gr_view.GetTotal(&gr_total);
				entry.Quantity = gr_total.Quantity;
				entry.Cost     = gr_total.SumCost;
				entry.Price    = gr_total.SumPrice;
				entry.Discount = gr_total.Count;
				if(ExtCostAmtID)
					entry.ExtCost = gr_total.Amounts.Get(ExtCostAmtID, 0  /* curID */);
				if(ExtPriceAmtID)
					entry.ExtPrice = gr_total.Amounts.Get(ExtPriceAmtID, 0  /* curID */);
			}
			AddEntry(&entry);
		}
		if(pFilt->Flags & OPG_CALCOUTREST) {
			r_op_rec.ID = 10000L;
			gr_flt.Init(1, 0);
			gr_flt.Date   = pFilt->Period.upp;
			gr_flt.LocList = pFilt->LocList; // AHTOXA
			gr_flt.Flags |= GoodsRestFilt::fCalcTotalOnly;
			gr_flt.WaitMsgID = PPTXT_CALCOUTREST;
			THROW(gr_view.Init_(&gr_flt));
			gr_view.GetTotal(&gr_total);
			entry.SetOp(r_op_rec);
			entry.Quantity = gr_total.Quantity;
			entry.Cost     = gr_total.SumCost;
			entry.Price    = gr_total.SumPrice;
			entry.Discount = gr_total.Count;
			if(ExtCostAmtID)
				entry.ExtCost = gr_total.Amounts.Get(ExtCostAmtID, 0  /* curID */);
			if(ExtPriceAmtID)
				entry.ExtPrice = gr_total.Amounts.Get(ExtPriceAmtID, 0  /* curID */);
			AddEntry(&entry);
		}
	}
	CATCHZOK
	return ok;
}

int FASTCALL GoodsGrpngArray::IsLockPaymStatus(PPID statusID) const
{
	return (LockPaymStatusList.IsExists() && LockPaymStatusList.CheckID(statusID));
}

int GoodsGrpngArray::ProcessGoodsGrouping(const GCTFilt & rFilt, const AdjGdsGrpng * pAgg)
{
	int    ok = 1;
	uint   i;
	GCTFilt filt = rFilt;
	GoodsGrpngEntry * p_entry;
	ObjRestrictArray * p_rtloclist = DS.GetTLA().Rights.P_LocList;
	{
		PPObjBillStatus bs_obj;
		PPBillStatus bs_rec;
		LockPaymStatusList.Set(0);
		for(SEnum en = PPRef->Enum(PPOBJ_BILLSTATUS, 0); en.Next(&bs_rec) > 0;)
			if(bs_rec.Flags & BILSTF_LOCK_PAYMENT)
				LockPaymStatusList.Add(bs_rec.ID);
	}
	if(p_rtloclist) {
		if(filt.LocList.GetCount() && p_rtloclist->getCount()) {
			const PPIDArray & a_ary = filt.LocList.Get();
			for(i = a_ary.getCount(); i > 0; i--) {
				if(!p_rtloclist->SearchItemByID(a_ary.at(i-1), 0))
					filt.LocList.Remove(i-1);
			}
		}
		if(filt.LocList.GetCount() == 0)
			for(i = 0; i < p_rtloclist->getCount(); i++)
				filt.LocList.Add(p_rtloclist->at(i).ObjID);
	}
	if(!filt.GoodsGrpID && !filt.GoodsID && !filt.SupplID && !filt.SupplAgentID) {
		if(filt.ExtGoodsTypeID) {
			PPObjGoodsType gt_obj;
			PPGoodsType gt_rec;
			if(gt_obj.Fetch(filt.ExtGoodsTypeID, &gt_rec) > 0) {
				ExtCostAmtID  = gt_rec.AmtCost;
				ExtPriceAmtID = gt_rec.AmtPrice;
				ExtDisAmtID   = gt_rec.AmtDscnt;
			}
		}
		THROW(_ProcessBillGrpng(&filt));
	}
	else {
		PPIDArray goods_list;
		if(filt.GoodsGrpID || filt.GoodsID) {
			if(!filt.GoodsID)
				GoodsIterator::GetListByGroup(filt.GoodsGrpID, &goods_list);
			else if(GObj.IsGeneric(filt.GoodsID))
				GObj.GetGenericList(filt.GoodsID, &goods_list);
			else
				goods_list.add(filt.GoodsID);
			//
			// Необходимо гарантировать, что в списке нет нулевых элементов
			// Причина - расчетный цикл (ниже) считает нулевой идентификатор
			// сигналом для специального расчета (по поставщику или агенту поставщика).
			//
			uint pos = 0;
			while(goods_list.lsearch(0, &pos))
				goods_list.atFree(pos);
		}
		else // filt.SupplID || filt.SupplAgentID // по поставщику ИЛИ по агенту поставщика
			goods_list.add(0L);
		for(i = 0; i < goods_list.getCount(); i++) {
			const  PPID goods_id = goods_list.get(i);
			const  PPID save_filt_goods_id = filt.GoodsID;
			filt.GoodsID = goods_id;
			PPID   tax_grp_id = 0;
			double phuperu = 0.0;
			double tax_factor = 1.0;
			Goods2Tbl::Rec goods_rec;
			PPGoodsTaxEntry gtx;
			// @v10.2.5 (ctr) MEMSZERO(gtx);
			if(filt.GoodsID && GObj.Fetch(filt.GoodsID, &goods_rec) > 0) {
				PPUnit urec;
				GoodsExtTbl::Rec gext_rec;
				phuperu = (GObj.FetchUnit(goods_rec.UnitID, &urec) > 0 && urec.Flags & PPUnit::Physical) ? 1.0 : goods_rec.PhUPerU;
				if(goods_rec.Flags & GF_TAXFACTOR && GObj.P_Tbl->GetExt(filt.GoodsID, &gext_rec) > 0 && gext_rec.TaxFactor > 0)
					tax_factor = gext_rec.TaxFactor;
				if(filt.Flags & (OPG_SETTAXES | OPG_NOZEROEXCISE) && GObj.FetchTax(filt.GoodsID, ZERODATE, 0L, &gtx) > 0)
					tax_grp_id = gtx.TaxGrpID;
			}
			if(!(filt.Flags & OPG_NOZEROEXCISE) || !(gtx.Flags & GTAXF_ZEROEXCISE)) {
				TransferTbl::Rec trfr_rec;
				if(filt.Flags & (OPG_CALCINREST|OPG_CALCOUTREST)) {
					GoodsRestParam gp;
					PPOprKind      op_rec;
					gp.CalcMethod = GoodsRestParam::pcmSum;
					// @v10.4.10 @fix gp.LocList    = filt.LocList.Get();
					filt.LocList.Get(gp.LocList); // @v10.4.10 @fix
					gp.GoodsID    = filt.GoodsID;
					gp.SupplID    = filt.SupplID;
					gp.AgentID    = filt.SupplAgentID;
					if(gp.AgentID && pAgg)
						gp.P_SupplAgentBillList = &pAgg->SupplAgentBillList;
					if(filt.Flags & OPG_SETCOSTWOTAXES)
						gp.Flags_ |= GoodsRestParam::fCWoVat;
					// @v10.6.6 {
					if(filt.Flags & OPG_SETPRICEWOTAXES)
						gp.Flags_ |= GoodsRestParam::fPWoVat;
					// } @v10.6.6
					if(filt.Flags & OPG_CALCINREST) {
						gp.Date = plusdate(filt.Period.low, -1);
						op_rec.ID  = -1;
						THROW(CalcRest(gp, op_rec, phuperu));
					}
					if(filt.Flags & OPG_CALCOUTREST) {
						gp.Date = filt.Period.upp;
						op_rec.ID  = 10000L;
						THROW(CalcRest(gp, op_rec, phuperu));
					}
				}
				if(pAgg && pAgg->BillList.getCount()) {
					filt.Flags |= OPG_ADJPAYM;
					GCT_Iterator gcti(filt, &pAgg->Period, pAgg);
					if(gcti.First(&trfr_rec) > 0)
						do {
							if(pAgg->BillList.bsearch(trfr_rec.BillID))
								THROW(Calc_(filt, pAgg, &trfr_rec, tax_grp_id, phuperu, tax_factor));
						} while(gcti.Next(&trfr_rec) > 0);
					filt.Flags &= ~OPG_ADJPAYM;
				}
				{
					GCT_Iterator gcti(filt, &filt.Period, pAgg);
					if(gcti.First(&trfr_rec) > 0)
						do {
							THROW(Calc_(filt, pAgg, &trfr_rec, tax_grp_id, phuperu, tax_factor));
						} while(gcti.Next(&trfr_rec) > 0);
				}
			}
			filt.GoodsID = save_filt_goods_id;
		}
	}
	for(i = 0; enumItems(&i, (void **)&p_entry);) {
		p_entry->Price -= p_entry->Discount;
	}
	if(rFilt.Flags & OPG_PROCESSGENOP && IsGenericOp(rFilt.OpID) > 0) {
		PPObjOprKind op_obj;
		ObjRestrictArray gen_list;
		THROW(op_obj.GetGenericList(rFilt.OpID, &gen_list));
		for(i = 0; enumItems(&i, (void **)&p_entry);) {
			if(gen_list.SearchItemByID(p_entry->OpID, 0)) {
				if(gen_list.CheckFlag(p_entry->OpID, GOIF_NEGATIVE)) {
					if(p_entry->Sign > 0)
						p_entry->Sign = -1;
					else if(p_entry->Sign < 0)
						p_entry->Sign = 1;
				}
			}
			else if(p_entry->OpID != -1 && p_entry->OpID != 10000)
				atFree(--i);
		}
	}
	CATCH
		ok = 0;
		freeAll();
	ENDCATCH
	return ok;
}

void GoodsGrpngArray::InitOpNames()
{
	GoodsGrpngEntry * p_entry;
	SString temp_buf;
	for(uint i = 0; enumItems(&i, (void **)&p_entry);) {
		uint strid = 0;
		temp_buf.Z();
		switch(p_entry->OpID) {
			case -1:                strid = PPTXT_INREST;      break;
			case 10000:             strid = PPTXT_OUTREST;     break;
			case PPOPK_INTRRECEIPT: strid = PPTXT_INTRRECEIPT; break;
			default:
				GetOpName(p_entry->OpID, temp_buf);
				if(p_entry->OpTypeID == PPOPT_GOODSMODIF && p_entry->Sign)
					temp_buf.Space().CatChar((p_entry->Sign > 0) ? '+' : '-');
				break;
		}
		if(strid)
			PPLoadText(strid, temp_buf);
		temp_buf.CopyTo(p_entry->OpName, sizeof(p_entry->OpName));
	}
}
