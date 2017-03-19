// TRANSFER.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2014, 2015, 2016
// @codepage windows-1251
// @Kernel
//
#include <pp.h>
#pragma hdrstop

SLAPI Transfer::Transfer() : TransferTbl()
{
	__DontCheckQttyInUpdateTransferItem__ = 0;
	P_LcrT = 0;
	const int lcrusage = CConfig.LcrUsage;
	if(oneof2(lcrusage, 1, 2))
		P_LcrT = new LotCurRestTbl;
}

SLAPI Transfer::~Transfer()
{
	delete P_LcrT;
}

int SLAPI Transfer::EnumByLot(PPID lotID, LDATE * date, long *oprno, TransferTbl::Rec * pRec)
{
	int    r = Search(lotID, *date, *oprno, spGt);
	if(r > 0) {
		copyBufTo(pRec);
		*date  = data.Dt;
		*oprno = data.OprNo;
	}
	return r;
}

int SLAPI Transfer::EnumByLot(PPID lotID, DateIter * pIter, TransferTbl::Rec * pRec)
{
	assert(pIter != 0);
	int    r = Search(lotID, pIter->dt, pIter->oprno, spGt);
	if(r > 0) {
		copyBufTo(pRec);
		return pIter->Advance(data.Dt, data.OprNo);
	}
	return r;
}

int SLAPI Transfer::IsDeadLot(PPID lotID)
{
	LDATE  dt = ZERODATE;
	long   oprno = 0;
	int    is_dead = 1;
	while(is_dead && EnumByLot(lotID, &dt, &oprno) > 0)
		if(!(data.Flags & PPTFR_RECEIPT))
			is_dead = 0;
	return is_dead ? +1 : -1;
}

int SLAPI Transfer::SearchByBill(PPID billID, int reverse, short rByBill, TransferTbl::Rec * pRec)
{
	TransferTbl::Key0 k0;
	k0.BillID  = billID;
	k0.Reverse = reverse;
	k0.RByBill = rByBill;
	int    r = SearchByKey(this, 0, &k0, pRec);
	if(r < 0)
		PPErrCode = PPERR_TRFRBYBILLNFOUND;
	return r;
}

int SLAPI Transfer::RecByBill(PPID billID, short * rByBill)
{
	int    ok = 1;
	int    sp_mode = spEq;
	PPID   rbb = *rByBill;
	if(rbb == 0) {
		sp_mode  = spLt;
		*rByBill = MAXSHORT;
	}
	TransferTbl::Key0 k0;
	k0.BillID  = billID;
	k0.Reverse = 0;
	k0.RByBill = *rByBill;
	PPErrCode  = 0;
	if(search(0, &k0, sp_mode) && k0.BillID == billID) {
		*rByBill = (rbb == 0) ? (k0.RByBill+1) : k0.RByBill;
	}
	else if(BTROKORNFOUND) {
		if(rbb == 0)
			*rByBill = 1;
		else
			ok = (PPErrCode = PPERR_TRFRBYBILLNFOUND, 0);
	}
	else
		ok = PPSetErrorDB();
	return ok;
}

int SLAPI Transfer::SearchMirror(LDATE dt, long oprno, TransferTbl::Rec * mirror)
{
	int    ok = -1;
	TransferTbl::Key1 k1;
	k1.Dt = dt;
	k1.OprNo = oprno;
	if(search(1, &k1, spEq)) {
		TransferTbl::Key0 k0;
		k0.BillID = data.BillID;
		k0.Reverse = (data.Reverse == 0) ? 1 : 0;
		k0.RByBill = data.RByBill;
		if(search(0, &k0, spEq) && data.LotID) {
			copyBufTo(mirror);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI Transfer::IsCompletedLot(PPID lotID)
{
	int    ok = -1;
	PPID   org_lot_id = 0;
	if(Rcpt.SearchOrigin(lotID, &org_lot_id, 0, 0) > 0) {
		PPIDArray lot_list;
		Rcpt.GatherChilds(org_lot_id, &lot_list, 0, 0);
		lot_list.atInsert(0, &org_lot_id);
		for(uint i = 0; i < lot_list.getCount(); i++) {
			TransferTbl::Rec trfr_rec;
			PPID   lot_id = lot_list.at(i);
			for(DateIter di; ok < 0 && EnumByLot(lot_id, &di, &trfr_rec) > 0;)
				if(trfr_rec.Flags & PPTFR_MODIF && trfr_rec.Flags & PPTFR_PLUS)
					ok = 1;
		}
	}
	return ok;
}

/*inline*/ int SLAPI Transfer::GetOprNo(LDATE date, long *oprno)
	{ return IncDateKey(this, 1, date, oprno); }

inline int SLAPI Transfer::GetLotOprNo(LDATE date, long *oprno)
	{ return IncDateKey(&Rcpt, 1, date, oprno); }
//
// Процедура AddLotItem при добавлении записи обнуляет текущий остаток.
// Изменение остатка делает процедура _UpdateForward.
//
int SLAPI Transfer::AddLotItem(PPTransferItem * ti, PPID forceID)
{
	int    ok = 1;
	ReceiptTbl::Rec lot_rec;
	PPID   prev_lot = ti->LotID;
	ti->LotID = 0;
	if(!forceID) {
#if 0 // @v8.9.5 { Отключаем (временно) слияние при возврате на исходный склад (из-за ЕГАИС и справок Б)
		for(PPID id = prev_lot; id && !ti->LotID; id = lot_rec.PrevLotID) {
			THROW(Rcpt.Search(id, &lot_rec) > 0);
			if(!(ti->Flags & (PPTFR_PCKGGEN | PPTFR_PCKG))) {
				//
				// В случае циклической межскладской операции лот не генерируется,
				// а происходит возврат в изначальный лот. Исключение составляет
				// ситуация, при которой цены лота не совпадают с ценами операции.
				//
				if(lot_rec.LocID == ti->LocID) {
					if(!dbl_cmp(lot_rec.Cost, ti->Cost) && !dbl_cmp(lot_rec.Price, ti->Price)) {
						ti->LotID = id;
						ti->Flags &= ~PPTFR_RECEIPT;
					}
				}
			}
		}
#endif // } 0 @v8.9.5
	}
	if(ti->LotID == 0) {
		MEMSZERO(lot_rec);
		lot_rec.ID          = forceID;
		lot_rec.BillID      = ti->BillID;
		lot_rec.LocID       = ti->LocID;
		lot_rec.Dt          = ti->Date;
		THROW(GetLotOprNo(lot_rec.Dt, &lot_rec.OprNo));
		lot_rec.PrevLotID   = prev_lot;
		lot_rec.GoodsID     = ti->GoodsID;
		lot_rec.QCertID     = ti->QCert;
		lot_rec.UnitPerPack = ti->UnitPerPack;
		lot_rec.Quantity    = ti->Quantity_;
		lot_rec.WtQtty      = (float)R6(ti->WtQtty);
		lot_rec.WtRest      = 0;
		lot_rec.Rest        = 0;
		lot_rec.SupplID     = ti->Suppl;
		lot_rec.Expiry      = ti->Expiry;
		lot_rec.Cost        = R5(ti->Cost);
		lot_rec.ExtCost     = R5(ti->ExtCost);
		lot_rec.Price       = R5(ti->Price);
		lot_rec.InTaxGrpID  = ti->LotTaxGrpID;
		SETFLAG(lot_rec.Flags, LOTF_COSTWOVAT,    ti->Flags & PPTFR_COSTWOVAT);
		SETFLAG(lot_rec.Flags, LOTF_COSTWSTAX,    ti->Flags & PPTFR_COSTWSTAX);
		SETFLAG(lot_rec.Flags, LOTF_PRICEWOTAXES, ti->Flags & PPTFR_PRICEWOTAXES);
		SETFLAG(lot_rec.Flags, LOTF_PCKG,         ti->Flags & PPTFR_PCKG);
		SETFLAG(lot_rec.Flags, LOTF_ORDRESERVE,   ti->TFlags & PPTransferItem::tfOrdReserve);
		SETFLAG(lot_rec.Flags, LOTF_CLOSEDORDER,  ti->Flags & PPTFR_CLOSEDORDER);
		if((ti->Flags & (PPTFR_MODIF|PPTFR_PLUS)) == (PPTFR_MODIF|PPTFR_PLUS) && !(ti->Flags & PPTFR_PCKG))
			lot_rec.Flags |= LOTF_COMPLETE;
		if(lot_rec.Flags & LOTF_CLOSEDORDER)
			lot_rec.Closed = 1;
		THROW(Rcpt.Add(&ti->LotID, &lot_rec, 0));
	}
	CATCH
		ti->LotID = prev_lot;
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI Transfer::SearchReval(PPID lotID, LDATE date, long oprno, TransferTbl::Rec * b)
{
	int    r = 1;
	if(date)
		while((r = EnumByLot(lotID, &date, &oprno)) > 0)
			if(data.Flags & PPTFR_REVAL)
				return (copyBufTo(b), 1);
	return r ? -1 : 0;
}

int SLAPI Transfer::UpdateFwRevalCostAndPrice(PPID lotID, LDATE dt, long oprno, double cost, double price, uint * pUF)
{
	int    ok = 1, r;
	uint   uf = 0;
	THROW(r = SearchReval(lotID, dt, oprno));
	if(r > 0) {
		SETFLAG(uf, TRUCLF_UPDCOST,  dbl_cmp(data.Cost, cost));
		SETFLAG(uf, TRUCLF_UPDPRICE, dbl_cmp(data.Price, price));
		if(uf & TRUCLF_UPDCP) {
			data.Cost  = TR5(cost);
			data.Price = TR5(price);
			THROW_DB(updateRec());
		}
	}
	else {
		THROW(Rcpt.Search(lotID) > 0);
		SETFLAG(uf, TRUCLF_UPDCOST,  dbl_cmp(Rcpt.data.Cost,  cost));
		SETFLAG(uf, TRUCLF_UPDPRICE, dbl_cmp(Rcpt.data.Price, price));
		if(uf & TRUCLF_UPDCP) {
			Rcpt.data.Cost  = R5(cost);
			Rcpt.data.Price = R5(price);
			THROW_DB(Rcpt.updateRec());
		}
	}
	if(uf & TRUCLF_UPDCP) {
		THROW(r = EnumByLot(lotID, &dt, &oprno));
		THROW_PP(r < 0, PPERR_FWREVAL);
	}
	ASSIGN_PTR(pUF, uf);
	CATCHZOK
	return ok;
}

SLAPI Transfer::GetLotPricesCache::GetLotPricesCache(LDATE dt, const PPIDArray * pLocList)
{
	Date = plusdate(dt, 1);
	//
	// Получаем список видов операции переоценки
	//
	PPIDArray op_list;
	UintHashTable bill_list;
	PPOprKind op_rec;
	{
		for(SEnum en = PPRef->EnumByIdxVal(PPOBJ_OPRKIND, 1, PPOPT_GOODSREVAL); en.Next(&op_rec) > 0;)
			op_list.addUnique(op_rec.ID);
	}
	{
		//
		// Так как модификация может быть рекомплектацией с функцией переоценки, то
		// придется учесть и это
		//
		for(SEnum en = PPRef->EnumByIdxVal(PPOBJ_OPRKIND, 1, PPOPT_GOODSMODIF); en.Next(&op_rec) > 0;)
			op_list.addUnique(op_rec.ID);
	}
	//
	// Находим список документов переоценки с датой, не превышающей Date
	//
	{
		DateRange period;
		period.Set(ZERODATE, Date);
		for(uint i = 0; i < op_list.getCount(); i++) {
			BillTbl::Rec bill_rec;
			for(DateIter di(Date); BillObj->P_Tbl->EnumByOpr(op_list.get(i), &di, &bill_rec) > 0;) {
				if(!pLocList || pLocList->lsearch(bill_rec.LocID))
					bill_list.Add((ulong)bill_rec.ID);
			}
		}
		/*
		 Этот участок выполняетмя медленнее, чем тот, что выше.
		for(uint i = 0; i < op_list.getCount(); i++) {
			BillTbl::Rec bill_rec;
			for(SEnum en = BillObj->tbl->EnumByOp(op_list.get(i), &period, 0); en.Next(&bill_rec) > 0;) {
				if(!pLocList || pLocList->lsearch(bill_rec.LocID))
					bill_list.Add((ulong)bill_rec.ID);
			}
		}
		*/
	}
	//
	// По каждому документу извлекаем идентификаторы лотов и складываем их
	// без дублирования в список RLotList
	//
	{
		Transfer * t = BillObj->trfr;
		for(ulong bill_id_ = 0; bill_list.Enum(&bill_id_);) {
			const PPID bill_id = (long)bill_id_;
			BExtQuery q(t, 0, 128);
			TransferTbl::Key0 k0;
			MEMSZERO(k0);
			k0.BillID = bill_id;
			q.select(t->LotID, t->Flags, 0L).where(t->BillID == bill_id && t->LotID > 0L);
			for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;)
				if(t->data.Flags & PPTFR_REVAL)
					RLotList.add(t->data.LotID); // @v8.1.0 addUnique-->add
		}
		RLotList.sortAndUndup(); // @v8.1.0 sort-->sortAndUndup
	}
}

int SLAPI Transfer::GetLotPricesCache::Get(ReceiptTbl::Rec * pLotRec)
{
	return RLotList.bsearch(pLotRec->ID) ? BillObj->trfr->GetLotPrices(pLotRec, plusdate(Date, -1), 0) : -1;
}

int SLAPI Transfer::GetLotPrices(ReceiptTbl::Rec * pLotRec, LDATE date, long oprno)
{
	int    r = -1;
	if(oprno == 0)
		plusdate(&date, 1, 0); // Новая цена вступает в силу на дату переоценки
	TransferTbl::Key2 k;
	BExtQuery q(this, 2, 16);
	q.select(Flags, Cost, Price, 0L).where(LotID == pLotRec->ID && Dt >= date);
	k.LotID = pLotRec->ID;
	k.Dt    = date;
	k.OprNo = oprno;
	for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
		if(data.Flags & PPTFR_REVAL) {
			pLotRec->Cost  = TR5(data.Cost);
			pLotRec->Price = TR5(data.Price);
			r = 1;
			break;
		}
	}
	return r;
}
//
// GoodsRestVal, GoodsRestParam
//
SLAPI GoodsRestVal::GoodsRestVal(const ReceiptTbl::Rec * pLotRec, double r)
{
	Init(pLotRec, r);
}

void SLAPI GoodsRestVal::Init(const ReceiptTbl::Rec * pLotRec, double r)
{
	Count = 0;
	if(pLotRec) {
		LotID = pLotRec->ID; // @v8.1.1
		LocID = pLotRec->LocID;
		UnitsPerPack = pLotRec->UnitPerPack;
		Cost  = pLotRec->Cost;
		Price = pLotRec->Price;
	}
	else {
		LotID = 0;
		LocID = 0;
		UnitsPerPack = 0.0;
		Cost = 0.0;
		Price = 0.0;
	}
	Deficit = 0.0;
	DraftRcpt = 0.0; // @v9.5.8 @fix
	Rest = r;
	Serial[0] = 0;
	LotTagText[0] = 0;
}

SLAPI GoodsRestParam::GoodsRestParam()
{
	CalcMethod = 0;
	Flags_ = 0;
	DiffParam = _diffNone;
	Date = Md_ = ZERODATE;
	OprNo = 0;
	Mo_ = 0;
	LocID = GoodsID = SupplID = AgentID = 0;
	GoodsTaxGrpID = 0;
	DiffLotTagID = 0;
	P_SupplAgentBillList = 0;
	InitVal();
}

void SLAPI GoodsRestParam::Init()
{
	CalcMethod = 0;
	Flags_ = 0;
	DiffParam = _diffNone;
	Date = ZERODATE;
	OprNo = 0;
	LocID = 0;
	GoodsID = 0;
	SupplID = 0;
	AgentID = 0;
	QuotKindID = 0;
	DiffLotTagID = 0;
	LocList.clear();
	Total.Init(0, 0.0);
	P_SupplAgentBillList = 0;
	Md_ = ZERODATE;
	Mo_ = 0;
	GoodsTaxGrpID = 0;

	InitVal();
}

GoodsRestParam & FASTCALL GoodsRestParam::operator = (const GoodsRestParam & src)
{
	copy(src);
	CalcMethod = src.CalcMethod;
	Flags_     = src.Flags_;
	DiffParam  = src.DiffParam; // @v8.1.0
	Date       = src.Date;
	OprNo      = src.OprNo;
	LocID      = src.LocID;
	GoodsID    = src.GoodsID;
	SupplID    = src.SupplID;
	AgentID    = src.AgentID;
	Total      = src.Total;
	Md_        = ZERODATE; //src.Md;
	Mo_        = 0; //src.Mo;
	GoodsTaxGrpID = src.GoodsTaxGrpID;
	DiffLotTagID = src.DiffLotTagID;
	LocList.copy(src.LocList);
	//
	// Копируем сам указатель, поскольку GoodsRestParam не является владельцем
	// объекта, на который этот указатель ссылается.
	//
	P_SupplAgentBillList = src.P_SupplAgentBillList;
	return *this;
}

void SLAPI GoodsRestParam::InitVal()
{
	Md_ = ZERODATE;
	Mo_ = 0;
	clear();
	Total.Init(0, 0.0);
	if(GoodsID) {
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		if(goods_obj.Fetch(GoodsID, &goods_rec) > 0)
			GoodsTaxGrpID = goods_rec.TaxGrpID;
	}
}

void SLAPI GoodsRestParam::Set(const GoodsRestFilt & rF)
{
	Init();
	const  long ff = rF.Flags;
	const  int quot_usage = rF.GetQuotUsage();
	if(rF.QuotKindID) {
		if(rF.QuotKindID && quot_usage == 1) {
			Flags_ |= fPriceByQuot;
			QuotKindID = rF.QuotKindID;
		}
		else if(rF.QuotKindID && quot_usage == 2) {
			Flags_ |= fCostByQuot;
			QuotKindID = rF.QuotKindID;
		}
	}
	SETFLAG(Flags_, fCWoVat, ff & GoodsRestFilt::fCWoVat);
	SETFLAG(Flags_, fZeroAgent, ff & GoodsRestFilt::fZeroSupplAgent);
	SETFLAG(Flags_, fLabelOnly, ff & GoodsRestFilt::fLabelOnly);
	CalcMethod = rF.CalcMethod;
	DiffParam  = rF.DiffParam;
	if(ff & GoodsRestFilt::fEachLocation)
		DiffParam |= GoodsRestParam::_diffLoc;
	Date    = rF.Date;
	SupplID = (ff & GoodsRestFilt::fWoSupplier) ? 0 : rF.SupplID;
}

double FASTCALL GoodsRestParam::GetRestByLoc(PPID locID) const
{
	double rest = 0.0;
	if(DiffParam & _diffLoc)
		for(uint i = 0; i < getCount(); i++)
			if(at(i).LocID == locID)
				rest += at(i).Rest;
	return rest;
}

int FASTCALL GoodsRestParam::CanMerge(const GoodsRestVal * v, const GoodsRestVal * a) const
{
	// @v8.1.0 {
	int    yes = 1;
	if(DiffParam & _diffLoc && a->LocID != v->LocID)
		yes = 0;
	else if(DiffParam & _diffCost && a->Cost != v->Cost)
		yes = 0;
	else if(DiffParam & _diffPrice && a->Price != v->Price)
		yes = 0;
	else if(DiffParam & _diffPack && a->UnitsPerPack != v->UnitsPerPack)
		yes = 0;
	else if(DiffParam & _diffSerial && strcmp(a->Serial, v->Serial) != 0)
		yes = 0;
	else if(DiffByTag() && strcmp(a->LotTagText, v->LotTagText) != 0)
		yes = 0;
	return yes;
	// } @v8.1.0
	/* @v8.1.0
	return (
		!(Flags & diffByCost  && a->Cost  != v->Cost) &&
		!(Flags & diffByPrice && a->Price != v->Price) &&
		!(Flags & diffByPack  && a->UnitsPerPack != v->UnitsPerPack) &&
		!(Flags & diffByLoc   && a->LocID != v->LocID) &&
		!(Flags & fDiffBySerial && strcmp(a->Serial, v->Serial) != 0) &&
		!(DiffByTag() && strcmp(a->LotTagText, v->LotTagText) != 0) // @v7.2.8
	);
	*/
}

int SLAPI GoodsRestParam::AddToItem(int p, LDATE dt, long opn, GoodsRestVal * add)
{
	if(p == -1) {
		add->Count = 1;
		if(!insert(add))
			return PPSetErrorSLib();
	}
	else {
		GoodsRestVal * v = (p >= 0) ? &at(p) : &Total;
		int    uc = (v->Cost  != add->Cost);
		int    up = (v->Price != add->Price);
		double nr = v->Rest + add->Rest;
		v->Count++;
		v->UnitsPerPack = add->UnitsPerPack;
		v->LocID = add->LocID;
		if(up || uc) {
			if(CalcMethod == pcmLastLot) {
   		        if(!Md_ || dt > Md_ || (dt == Md_ && opn >= Mo_)) {
					v->LotID = add->LotID; // @v8.1.1
					v->Cost  = add->Cost;
				   	v->Price = add->Price;
					Md_ = dt;  // @v8.0.6
					Mo_ = opn; // @v8.0.6
			   	}
			}
			else if(CalcMethod == pcmFirstLot) {
				if(!Md_ || dt < Md_ || (dt == Md_ && opn <= Mo_)) {
					v->LotID = add->LotID; // @v8.1.1
					v->Cost  = add->Cost;
					v->Price = add->Price;
					Md_ = dt;  // @v8.0.6
					Mo_ = opn; // @v8.0.6
				}
			}
			else if(nr != 0.0) {
				if(uc)
					v->Cost  = (v->Cost  * v->Rest + add->Cost  * add->Rest) / nr;
			   	if(up)
					v->Price = (v->Price * v->Rest + add->Price * add->Rest) / nr;
   	        }
		}
		v->Rest = nr;
	}
	if(!Md_) {
		Md_ = dt;
		Mo_ = opn;
	}
	return 1;
}

int SLAPI GoodsRestParam::AddLot(Transfer * pTrfr, const ReceiptTbl::Rec * pLotRec, double rest, LDATE orgLotDate)
{
	int    merge;
	uint   i;
	GoodsRestVal add(pLotRec, rest);
	GoodsRestVal * p_val;
	const  int costwovat = BIN(pLotRec->Flags & LOTF_COSTWOVAT);
	const  int pricewotaxes = BIN(pLotRec->Flags & LOTF_PRICEWOTAXES);
	const  int byquot_price = BIN(QuotKindID > 0 && Flags_ & GoodsRestParam::fPriceByQuot);
	const  int byquot_cost  = BIN(QuotKindID > 0 && Flags_ & GoodsRestParam::fCostByQuot);
	const  int setcostwovat = BIN(Flags_ & fCWoVat);
	if(costwovat || pricewotaxes || byquot_price || byquot_cost|| setcostwovat) {
		double tax_factor = 1.0;
		PPObjGoods gobj;
		if(byquot_price || byquot_cost) {
			double q_price;
			const QuotIdent qi(QIDATE(getcurdate_()), (DiffParam & _diffLoc) ? pLotRec->LocID : LocID, QuotKindID);
			if(gobj.GetQuotExt(pLotRec->GoodsID, qi, add.Cost, add.Price, &q_price, 1) > 0) {
				if(byquot_price)
					add.Price = q_price;
				else if(byquot_cost)
					add.Cost = q_price;
			}
		}
		if(costwovat || pricewotaxes || setcostwovat) {
			const  int is_asset = BIN(gobj.IsAsset(labs(pLotRec->GoodsID)) > 0);
			int    vat_free = -1;
			gobj.MultTaxFactor(pLotRec->GoodsID, &tax_factor);
			if(costwovat) {
				if(vat_free < 0)
					vat_free = IsLotVATFree(*pLotRec);
				if(!orgLotDate) {
					pTrfr->Rcpt.GetOriginDate(pLotRec, &orgLotDate);
				}
				gobj.AdjCostToVat(pLotRec->InTaxGrpID, GoodsTaxGrpID, orgLotDate, tax_factor, &add.Cost, 1, vat_free);
				if(is_asset)
					gobj.AdjCostToVat(pLotRec->InTaxGrpID, GoodsTaxGrpID, orgLotDate, tax_factor, &add.Price, 1, vat_free);
			}
			if(pricewotaxes)
				gobj.AdjPriceToTaxes(GoodsTaxGrpID, tax_factor, &add.Price, 1);
			if(setcostwovat) {
				PPID   in_tax_grp_id = NZOR(pLotRec->InTaxGrpID, GoodsTaxGrpID);
				if(vat_free < 0)
					vat_free = IsLotVATFree(*pLotRec);
				long   amt_fl = 0;
				GTaxVect vect;
				PPGoodsTaxEntry gt;
				if(!orgLotDate)
					pTrfr->Rcpt.GetOriginDate(pLotRec, &orgLotDate);
				if(gobj.GTxObj.Fetch(in_tax_grp_id, orgLotDate, 0L, &gt) > 0) {
					amt_fl = ~GTAXVF_SALESTAX;
					long excl_fl = (vat_free > 0) ? GTAXVF_VAT : 0;
					vect.Calc_(&gt, add.Cost, tax_factor, amt_fl, excl_fl);
					add.Cost -= vect.GetValue(GTAXVF_VAT);
					if(is_asset) {
						vect.Calc_(&gt, add.Price, tax_factor, amt_fl, excl_fl);
						add.Price -= vect.GetValue(GTAXVF_VAT);
					}
				}
				if(!is_asset && gobj.GTxObj.FetchByID(GoodsTaxGrpID, &gt) > 0) {
					amt_fl = (CCFLG_PRICEWOEXCISE & CConfig.Flags) ? ~GTAXVF_SALESTAX : GTAXVF_BEFORETAXES;
					vect.Calc_(&gt, add.Price, tax_factor, amt_fl, 0);
					add.Price = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_EXCISE);
				}
			}
		}
	}
	if(DiffParam & (_diffPrice|_diffCost|_diffPack|_diffLoc|_diffSerial) || DiffByTag()) {
		// @v8.1.0 if(Flags & fDiffBySerial) {
		if(DiffParam & _diffSerial) { // @v8.1.0
			PPObjBill * p_bobj = BillObj;
			SString serial;
			if(p_bobj->GetSerialNumberByLot(pLotRec->ID, serial, 1) > 0)
				serial.CopyTo(add.Serial, sizeof(add.Serial));
			// @v8.1.12 {
			else if(pLotRec->PrevLotID) {
				ReceiptTbl::Rec org_lot_rec;
				if(pTrfr->Rcpt.SearchOrigin(pLotRec->PrevLotID, 0, 0, &org_lot_rec)) {
					if(p_bobj->GetSerialNumberByLot(org_lot_rec.ID, serial, 1) > 0)
						serial.CopyTo(add.Serial, sizeof(add.Serial));
				}
			}
			// } @v8.1.12
		}
		// @v7.2.8 {
		PPID   diff_tag_id = DiffByTag();
		if(diff_tag_id) {
			SString tag_buf;
			ObjTagItem tag_item;
			PPObjTag tag_obj; // @v8.1.0
			if(tag_obj.FetchTag(pLotRec->ID, diff_tag_id, &tag_item) > 0) { // @v8.1.0
			// @v8.1.0 if(PPRef->Ot.GetTag(PPOBJ_LOT, pLotRec->ID, diff_tag_id, &tag_item) > 0) {
			   tag_item.GetStr(tag_buf);
			   tag_buf.CopyTo(add.LotTagText, sizeof(add.LotTagText));
			}
		}
		// } @v7.2.8
		for(i = 0, merge = -1; merge == -1 && enumItems(&i, (void**)&p_val);) {
			if(CanMerge(p_val, &add))
				merge = i-1;
		}
		if(!AddToItem(merge, pLotRec->Dt, pLotRec->OprNo, &add))
			return 0;
	}
	// Updating total
	AddToItem(-2, pLotRec->Dt, pLotRec->OprNo, &add);
	return 1;
}

int SLAPI GoodsRestParam::CheckBill(const ReceiptTbl::Rec * pRec, LDATE * pOrgLotDate) const
{
	int    ok = 1;
	LDATE  org_lot_date = ZERODATE;
	PPID   org_bill_id = 0;
	//
	// Условия fLabelOnly и AgentID не проверяются для лотов заказов
	//
	if(pRec->GoodsID > 0 && ((Flags_ & (fLabelOnly|fZeroAgent)) || AgentID)) {
		PPObjBill * p_bobj = BillObj;
		if(!p_bobj->trfr->Rcpt.GetOriginDate(pRec, &org_lot_date, &org_bill_id))
			ok = 0;
		else if(Flags_ & fLabelOnly && !p_bobj->P_Tbl->HasWLabel(org_bill_id))
			ok = 0;
		else if(AgentID || Flags_ & fZeroAgent) {
			if(P_SupplAgentBillList) {
				if(!P_SupplAgentBillList->lsearch(org_bill_id))
					ok = 0;
			}
			else {
				PPBillExt b_ext;
				if(p_bobj->FetchExt(org_bill_id, &b_ext) > 0) {
					if(b_ext.AgentID) {
						if(Flags_ & fZeroAgent || b_ext.AgentID != AgentID)
							ok = 0;
					}
					else if(!(Flags_ & fZeroAgent))
						ok = 0;
				}
				else if(!(Flags_ & fZeroAgent))
					ok = 0;
			}
		}
	}
	ASSIGN_PTR(pOrgLotDate, org_lot_date);
	return ok;
}
//
//
//
int SLAPI Transfer::CalcAssetDeprec(PPID lotID, const DateRange * pPeriod, double * pDeprec)
{
	int    ok = 1;
	double deprec = 0.0, rest = 0.0;
	ReceiptTbl::Rec lot_rec;
	if(Rcpt.Search(lotID, &lot_rec) > 0) {
		LDATE  dt = ZERODATE;
		double endprice = R5(lot_rec.Price);
		if(pPeriod && pPeriod->low)
			dt = pPeriod->low;
		GetLotPrices(&lot_rec, dt, 0);
		rest = R5(lot_rec.Price);
		lot_rec.Price = R5(endprice);
		if(pPeriod && pPeriod->upp) {
			dt = pPeriod->upp;
			GetLotPrices(&lot_rec, dt, 0);
			deprec = R5(lot_rec.Price);
		}
		else
			deprec = endprice;
		deprec = rest - deprec;
		ASSIGN_PTR(pDeprec, deprec);
	}
	else
		ok = 0;
	return ok;
}

void SLAPI GoodsRestParam::DivRestPrices()
{
	if(CalcMethod == pcmSum) {
		Total.Cost  *= Total.Rest;
		Total.Price *= Total.Rest;
		GoodsRestVal * p_val;
		for(uint i = 0; enumItems(&i, (void**)&p_val);) {
			p_val->Cost  *= p_val->Rest;
			p_val->Price *= p_val->Rest;
		}
	}
}

static int FASTCALL CR_MakeLocList(GoodsRestParam * pGrParam, PPIDArray * pList)
{
	int    ok = 1;
	if(pGrParam->LocList.getCount())
		pList->copy(pGrParam->LocList);
	else if(pGrParam->LocID)
		pList->add(pGrParam->LocID);
	else {
		PPObjLocation loc_obj;
		loc_obj.GetWarehouseList(pList);
	}
	return ok;
}

int SLAPI ReceiptCore::Helper_GetList(PPID goodsID, PPID locID, PPID supplID, LDATE beforeDt,
	int closedTag, int nzRestOnly, SArray * pRecList)
{
	int    ok = 1;
	ReceiptTbl::Key3 k3;
	MEMSZERO(k3);
	BExtQuery q(this, 3);
	DBQ * dbq = 0;
	k3.Closed = closedTag;
	k3.GoodsID = goodsID;
	k3.LocID = locID;
	dbq = &(this->Closed == (long)closedTag && this->GoodsID == goodsID);
	dbq = ppcheckfiltid(dbq, this->LocID, locID);
	if(beforeDt)
		dbq = &(*dbq && this->Dt <= beforeDt);
	dbq = ppcheckfiltid(dbq, this->SupplID, supplID);
	if(nzRestOnly)
		dbq = &(*dbq && this->Rest > 0.0);
	q.selectAll().where(*dbq);
	for(q.initIteration(0, &k3, spGe); q.nextIteration() > 0;) {
		THROW_MEM(pRecList->insert(&data));
	}
	CATCHZOK
	return ok;
}

int SLAPI ReceiptCore::GetList(PPID goodsID, PPID locID, PPID supplID, LDATE beforeDt,
	int openedOnly, int nzRestOnly, SArray * pRecList)
{
	int    ok = 1;
	THROW(Helper_GetList(goodsID, locID, supplID, beforeDt, 0, nzRestOnly, pRecList));
	if(!openedOnly) {
		THROW(Helper_GetList(goodsID, locID, supplID, beforeDt, 1, nzRestOnly, pRecList));
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::GetCurRest(GoodsRestParam * pGrParam)
{
	int    ok = 1, r = 1;
	PPIDArray loc_list;
	PROFILE_START
	pGrParam->InitVal();
	CR_MakeLocList(pGrParam, &loc_list);
	if(pGrParam->GoodsID == 0 && pGrParam->SupplID) {
		ReceiptTbl::Key5 k5;
		DBQ * dbq = 0;
		BExtQuery q(&Rcpt, 5);
		MEMSZERO(k5);
		k5.SupplID = pGrParam->SupplID;
		dbq = &(Rcpt.SupplID == pGrParam->SupplID && Rcpt.Closed == 0L);
		q.selectAll().where(*dbq);
		loc_list.sort();
		for(q.initIteration(0, &k5, spGe); q.nextIteration() > 0;) {
			const ReceiptTbl::Rec & r_lot_rec = Rcpt.data;
			if((r_lot_rec.GoodsID >= 0) || !(r_lot_rec.Flags & LOTF_CLOSEDORDER)) {
				if(loc_list.lsearch(r_lot_rec.LocID)) {
					LDATE  org_lot_date = ZERODATE;
					if(pGrParam->CheckBill(&r_lot_rec, &org_lot_date))
						THROW(pGrParam->AddLot(this, &r_lot_rec, r_lot_rec.Rest, org_lot_date));
				}
			}
		}
	}
	else if(pGrParam->GoodsID) {
		const int opened_only = (pGrParam->GoodsID < 0) ? 0 : 1;
		LotArray lot_list;
		for(uint j = 0; j < loc_list.getCount(); j++) {
			lot_list.clear();
			THROW(Rcpt.GetList(pGrParam->GoodsID, loc_list.get(j), pGrParam->SupplID, ZERODATE, opened_only, 1, &lot_list));
			for(uint i = 0; i < lot_list.getCount(); i++) {
				const ReceiptTbl::Rec & r_lot_rec = lot_list.at(i);
				if((r_lot_rec.GoodsID >= 0) || !(r_lot_rec.Flags & LOTF_CLOSEDORDER)) { // @v6.4.4
					LDATE  org_lot_date = ZERODATE;
					if(pGrParam->CheckBill(&r_lot_rec, &org_lot_date))
						THROW(pGrParam->AddLot(this, &r_lot_rec, r_lot_rec.Rest, org_lot_date));
				}
			}
		}
	}
	pGrParam->DivRestPrices();
	PROFILE_END
	CATCHZOK
	return ok;
}

int SLAPI Transfer::GetRest(GoodsRestParam * pGrParam)
{
	int    ok = 1;
	if(pGrParam->Date == 0)
		ok = GetCurRest(pGrParam);
	else {
		int    tag_closed = -1;
		double rest = 0.0;
		PROFILE_START
		pGrParam->InitVal();
		do {
			//
			// Цикл do {} while(tag_closed == 0)
			// реализован чтобы использовать индекс ReceiptTbl::Key3 в случае, если
			// заданы одновременно (pGrParam->GoodsID > 0 && pGrParam->LocID). В этом случае
			// придется пробежать сначала по открытым лотам (Closed==0), а потом по закрытым (Closed==1).
			// Если (pGrParam->GoodsID > 0 && pGrParam->LocID) не выполняются, то tag_closed принимает
			// значение -1, что автоматически выведет из цикла после первой итерации.
			//
			DBQ  * dbq = 0;
			int    idx;
			union {
				ReceiptTbl::Key2 k2;
				ReceiptTbl::Key3 k3;
				ReceiptTbl::Key5 k5;
			} k;
			MEMSZERO(k);
			{
				if(pGrParam->GoodsID > 0 && pGrParam->LocID) {
					tag_closed = (tag_closed == -1) ? 0 : 1;
					idx = 3;
					k.k3.Closed = tag_closed;
					k.k3.GoodsID = pGrParam->GoodsID;
					k.k3.LocID = pGrParam->LocID;
					dbq     = & (Rcpt.Closed == (long)tag_closed && Rcpt.GoodsID == pGrParam->GoodsID && Rcpt.LocID == pGrParam->LocID);
				}
				else if(pGrParam->GoodsID || pGrParam->SupplID == 0) {
					idx     = 2;
					k.k2.GoodsID = pGrParam->GoodsID;
					dbq     = & (Rcpt.GoodsID == pGrParam->GoodsID);

				}
				else {
					idx     = 5;
					k.k5.SupplID = pGrParam->SupplID;
					// @v8.5.0 dbq     = & (Rcpt.SupplID == pGrParam->SupplID);
				}
				dbq = ppcheckfiltid(dbq, Rcpt.SupplID, pGrParam->SupplID); // @v8.5.0
			}
			assert(oneof3(tag_closed, -1, 0, 1));
			dbq = & (*dbq && Rcpt.Dt <= pGrParam->Date);
			if(tag_closed == -1)
				dbq = ppcheckfiltid(dbq, Rcpt.LocID, pGrParam->LocID);
			if(pGrParam->GoodsID >= 0)
				dbq = & (*dbq && Rcpt.CloseDate > pGrParam->Date);
			else
				dbq = & (*dbq && Rcpt.Closed == 0L);
			/* @v8.5.0
			if(pGrParam->SupplID && idx == 2)
				dbq = & (*dbq && Rcpt.SupplID == pGrParam->SupplID);
			*/
			BExtQuery q(&Rcpt, idx);
			q.select(Rcpt.ID, Rcpt.BillID, Rcpt.LocID, Rcpt.SupplID, Rcpt.PrevLotID, Rcpt.Dt, Rcpt.OprNo,
				Rcpt.UnitPerPack, Rcpt.Cost, Rcpt.Price, Rcpt.Flags, Rcpt.InTaxGrpID, 0L).where(*dbq);
			const long _oprno = (pGrParam->OprNo > 0) ? (pGrParam->OprNo-1) : MAXLONG; // @v7.6.2 (-1) потому, что GetRest берет остаток по условию spLe
			for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
				ReceiptTbl::Rec lot_rec;
				Rcpt.copyBufTo(&lot_rec);
				LDATE  org_lot_date = ZERODATE;
				if(pGrParam->LocList.getCount() && !pGrParam->LocList.lsearch(lot_rec.LocID))
					continue;
				if(!ObjRts.CheckLocID(lot_rec.LocID, 0))
					continue;
				if(pGrParam->CheckBill(&lot_rec, &org_lot_date)) {
					THROW(GetRest(lot_rec.ID, pGrParam->Date, _oprno, &rest));
					if(rest != 0.0) {
						THROW(GetLotPrices(&lot_rec, pGrParam->Date));
						THROW(pGrParam->AddLot(this, &lot_rec, rest, org_lot_date));
					}
				}
			}
			assert(oneof3(tag_closed, -1, 0, 1));
		} while(tag_closed == 0);
		pGrParam->DivRestPrices();
		PROFILE_END
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::Search(PPID lot, LDATE date, long oprno, int spMode)
{
	TransferTbl::Key2 k;
	k.LotID = lot;
	k.Dt    = date;
	k.OprNo = oprno;
	return (search(2, &k, spMode) && k.LotID == lot) ? 1 : PPDbSearchError();
}

int SLAPI Transfer::GetRest(PPID lotID, LDATE date, long oprno, double * pRest, double * pPhRest)
{
	TransferTbl::Key2 k;
	k.LotID = lotID;
	k.Dt    = date;
	k.OprNo = oprno;
	int    r = (search(2, &k, spLe) && k.LotID == lotID) ? 1 : PPDbSearchError();
	if(r > 0) {
		ASSIGN_PTR(pRest, R6(data.Rest));
		ASSIGN_PTR(pPhRest, R6(data.WtRest));
		return 1;
	}
	else {
		ASSIGN_PTR(pRest, 0.0);
		ASSIGN_PTR(pPhRest, 0.0);
		return r ? (PPErrCode = PPERR_NOPERSBYLOT, -1) : 0;
	}
}

int SLAPI Transfer::GetRest(PPID lotID, LDATE dt, double * pRest, double * pPhRest)
{
	return GetRest(lotID, dt, MAXLONG, pRest, pPhRest);
}

int SLAPI Transfer::GetBounds(PPID lot, LDATE date, long oprno, double * pMinusDelta, double * pPlusDelta)
{
	int    ok = 1, r;
	double down, up;
	double qtty, rest;
	if(oprno < 0)
		oprno = MAXLONG;
	if(pPlusDelta) {
		THROW(Rcpt.Search(lot) > 0);
		qtty = Rcpt.data.Quantity;
	}
	if((r = Search(lot, date, oprno, spLt)) > 0) {
		down = data.Rest;
		if(pPlusDelta)
			up = qtty - down;
		while((r = EnumByLot(lot, &date, &oprno)) > 0) {
			rest = data.Rest;
			if(rest < down)
				down = rest;
			if(pPlusDelta && (qtty - rest) < up)
				up = (qtty - rest);
		}
	}
	else
		down = up = 0.0;
	THROW(r);
	CATCH
		ok = 0;
		down = up = 0.0;
	ENDCATCH
	ASSIGN_PTR(pMinusDelta, down);
	ASSIGN_PTR(pPlusDelta, up);
	return ok;
}

int SLAPI Transfer::GetAvailableGoodsRest(PPID goodsID, PPID locID, const DateRange & rPeriod, double ignoreEpsilon, double * pRest)
{
	int    ok = 1;
	const  double _epsilon = (ignoreEpsilon > 0.0) ? ignoreEpsilon : 0.0;
	double lot_rest, rest = 0.0;
	LotArray lot_list;
	THROW(Rcpt.GetListOfOpenedLots(1, goodsID, locID, rPeriod.upp, &lot_list));
	for(uint i = 0; i < lot_list.getCount(); i++) {
		const ReceiptTbl::Rec & r_rec = lot_list.at(i);
		if(r_rec.Dt >= rPeriod.low) {
			THROW(GetBounds(r_rec.ID, rPeriod.upp, -1, &lot_rest, 0));
			if(lot_rest > _epsilon) // @v9.4.8
				rest += lot_rest;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pRest, rest);
	return ok;
}

int SLAPI Transfer::PreprocessCorrectionExp(PPTransferItem & rTi, const PPIDArray & rBillChain)
{
	int   ok = 1;
	if(rTi.IsCorrectionExp()) {
		double org_qtty = 0.0;
		double org_price = 0.0;
		GetOriginalValuesForCorrection(rTi, rBillChain, &org_qtty, &org_price);
		rTi.QuotPrice = org_qtty;
		rTi.RevalCost = org_price;
		ok = rTi.PreprocessCorrectionExp();
	}
	else
		ok = -1;
	return ok;
}

int SLAPI Transfer::UpdateForward(PPID lotID, LDATE dt, long oprno, int check, double * pAddendum, double * pPhAdd)
{
	int    ok = 1;
	int    r = 1, valid = 1;
	double neck    = fabs(*pAddendum);
	double ph_neck = fabs(*pPhAdd);
	if(check || neck != 0.0 || ph_neck != 0.0) {
		if(!check && P_LcrT) {
			LcrBlock lcr(LcrBlock::opUpdate, P_LcrT, 0);
			THROW(lcr.Update(lotID, dt, *pAddendum));
		}
		while(lotID && (r = EnumByLot(lotID, &dt, &oprno)) > 0) {
			if(check) {
				neck    = MIN(neck,    data.Rest);
				ph_neck = MIN(ph_neck, data.WtRest);
			}
			data.Rest   = R6(data.Rest + *pAddendum);
			if(data.Rest < 0.0) {
				THROW_PP(check, PPERR_FWLOTRESTBOUND);
				valid = 0;
			}
			data.WtRest = (float)R6(data.WtRest + *pPhAdd);
			//
			// Если необходима проверка на неотрицательность
			// остатка в физических единицах, то комментарии со следующего
			// блока необходимо убрать
			//
			/*
			if(data.WtRest < 0) {
				THROW(check, PPERR_FWLOTRESTBOUND);
				valid = 0;
			}
			*/
			if(!check) {
				THROW_DB(updateRec());
				if(!(CConfig.Flags & CCFLG_TRFR_DONTRECALCREVAL))
					THROW(!(data.Flags & PPTFR_REVAL) || BillObj->RecalcTurns(data.BillID, 0, 0));
			}
			ok = 2;
		}
		THROW_DB(r);
		if(check) {
			*pAddendum = neck;
			*pPhAdd = ph_neck;
		}
		else
			data.Dt = dt;
	}
	if(!valid) {
		PPSetError(PPERR_FWLOTRESTBOUND); // @v9.4.3
		ok = -1;
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::UpdateForward(const TransferTbl::Rec & rRec, double addendum, double phAddend)
{
	int    ok = 1;
	int    is_recomplete = BIN((rRec.Flags & (PPTFR_REVAL|PPTFR_MODIF)) == (PPTFR_REVAL|PPTFR_MODIF));
	if(!IsUnlimWoLot(rRec) && (!(rRec.Flags & PPTFR_REVAL) || (rRec.Flags & PPTFR_CORRECTION) || is_recomplete)) {
		if(is_recomplete) {
			addendum = 0.0;
			if(!(rRec.Flags & PPTFR_INDEPPHQTTY))
				phAddend = 0.0;
		}
		LDATE  dt = rRec.Dt;
		LDATE  last_op_date = ZERODATE;
		long   o  = rRec.OprNo;
		int    upd_lot_rest = 0;
		if(addendum != 0.0 || phAddend != 0.0) {
			if(rRec.LotID) {
				int    r;
				THROW(r = UpdateForward(rRec.LotID, dt, o, 0, &addendum, &phAddend));
				if(r != 1) // r == 1 означает, что не было обнаружено ни одной операции > {dt; o}
					last_op_date = data.Dt; // Дата последней операции
				const PPID goods_id = (rRec.Flags & PPTFR_SHADOW) ? -labs(rRec.GoodsID) : rRec.GoodsID;
				THROW(UpdateCurRest(goods_id, rRec.LocID, addendum));
				upd_lot_rest = 1;
			}
		}
		else if(rRec.Flags & PPTFR_ORDER) {
			upd_lot_rest = 1;
		}
		if(upd_lot_rest) {
			ReceiptTbl::Rec lot_rec;
			int    bad_lot = 0;
			THROW(SearchByID_ForUpdate(&Rcpt, PPOBJ_LOT, rRec.LotID, &lot_rec) > 0);
			double prev_rest = lot_rec.Rest;
			lot_rec.Rest = R6(lot_rec.Rest + addendum);
			lot_rec.WtRest = (float)R6(lot_rec.WtRest + phAddend);
			if(lot_rec.Rest < 0.0 && (bad_lot = BIN(prev_rest < 0.0 && PPMaster && CConfig.Flags & CCFLG_DEBUG)) == 0) {
				PPSetObjError(PPERR_LOTRESTBOUND, PPOBJ_GOODS, labs(lot_rec.GoodsID));
				CALLEXCEPT();
			}
			else {
				if(lot_rec.Rest > 0.0) {
					if(!bad_lot) {
						lot_rec.Closed    = 0;
						lot_rec.CloseDate = MAXDATE;
					}
				}
				else if(!bad_lot) { // lot_rec.Rest == 0
					//
					// Без этого участка кода дата закрытия лота устанавливалась не верно в
					// случае удаления операции, увеличивающей остаток.
					//
					if(!last_op_date)
						if(addendum < 0.0 && rRec.Quantity > 0.0 && Search(rRec.LotID, rRec.Dt, rRec.OprNo, spLt))
							last_op_date = data.Dt;
						else
							last_op_date = dt;
					//
					lot_rec.Closed      = 1;
					lot_rec.CloseDate   = last_op_date;
					lot_rec.Flags |= LOTF_CLOSEDORDER;
				}
				THROW_DB(Rcpt.updateRecBuf(&lot_rec)); // @sfu
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::GetLocGoodsList(PPID locID, UintHashTable & rList)
{
	int    ok = 1;
	if(locID) {
		CurRestTbl::Key1 k1;
		MEMSZERO(k1);
		k1.LocID = locID;
		BExtQuery q(&CRest, 1, 512);
		q.select(CRest.GoodsID, 0L).where(CRest.LocID == locID && CRest.GoodsID > 0L);
		for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
			THROW_SL(rList.Add((ulong)CRest.data.GoodsID));
		}
	}
	else {
		CurRestTbl::Key0 k0;
		MEMSZERO(k0);
		BExtQuery q(&CRest, 0, 1024);
		q.select(CRest.GoodsID, 0L).where(CRest.GoodsID > 0L);
		PPID   prev_goods_id = 0;
		for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
			const PPID goods_id = CRest.data.GoodsID;
			if(goods_id != prev_goods_id) {
				THROW_SL(rList.Add((ulong)goods_id));
			}
			prev_goods_id = goods_id;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::UpdateCurRest(PPID goodsID, PPID loc, double addendum)
{
	//
	// @v6.6.11 Сделаны изменения для того, чтобы записи с нулевым остатком не
	// удалялись. Они понадобятся для быстрой идентификации товаров, которые
	// когда либо присутствовали на складе.
	//
	int    ok = 1;
	CurRestTbl::Key0 k;
	k.GoodsID  = goodsID;
	k.LocID = loc;
	if(CRest.searchForUpdate(0, &k, spEq)) {
		CRest.data.Rest = R6(CRest.data.Rest + addendum);
		if(CRest.data.Rest < 0.0) {
			if(goodsID < 0) {
				//
				// Для заказов при достижении отрицательного текущего остатка
				// не выдаем ошибку, но обнуляем результирующее значение.
				// Такая "мягкость" связана с тем, что форсированно закрытые
				// заказы не увеличивают текущие остатки, но имеют ненулевой
				// остаток по лоту. При попытке же списать такой заказ можем
				// нарваться на ошибку в данной точке.
				//
				CRest.data.Rest = 0.0;
			}
			else {
				PPObject::SetLastErrObj(PPOBJ_GOODS, labs(goodsID));
				CALLEXCEPT_PP(PPERR_LOTRESTBOUND);
			}
		}
		THROW_DB(CRest.updateRec()); // @sfu
	}
	else if(BTRNFOUND) {
		if((CRest.data.Rest = R6(addendum)) >= 0.0) {
			CRest.data.LocID = loc;
			CRest.data.GoodsID  = goodsID;
			PPObject::SetLastErrObj(PPOBJ_GOODS, labs(goodsID));
			THROW_PP(CRest.data.Rest >= 0.0, PPERR_LOTRESTBOUND);
			THROW_DB(CRest.insertRec());
		}
		else {
			PPObject::SetLastErrObj(PPOBJ_GOODS, labs(goodsID));
			CALLEXCEPT_PP(PPERR_LOTRESTBOUND);
		}
	}
	else
		CALLEXCEPT_PP(PPERR_DBENGINE);
	CATCHZOK
	return ok;
}

long FASTCALL MASK_TFR_FLAGS(long f)
{
	return (f & (PPTFR_RECEIPT | PPTFR_SELLING | PPTFR_REVAL | PPTFR_SHADOW | PPTFR_PARTSTRUSED | PPTFR_ONORDER |
		PPTFR_ORDER | PPTFR_UNLIM | PPTFR_ODDGOODS | PPTFR_FORCESUPPL | PPTFR_PLUS | PPTFR_MINUS | PPTFR_MODIF |
		PPTFR_QUOT | PPTFR_RMVEXCISE | PPTFR_ACK | PPTFR_NODISCOUNT | PPTFR_COSTWOVAT | PPTFR_COSTWSTAX | PPTFR_PCKG |
		PPTFR_PCKGGEN | PPTFR_PRICEWOTAXES | PPTFR_ASSETEXPL | PPTFR_INDEPPHQTTY | PPTFR_FIXMODIFCOST |
		PPTFR_CORRECTION | PPTFR_LOTSYNC)); // @v8.0.3 PPTFR_LOTSYNC
}
//
// Ненулевое поле ti->RByBill является признаком зеркальной проводки
//
// Договоримся, что в случае межскладской передачи в любом случае первой из двух зеркальных проводок,
// должна идти проводка по РАСХОДУ. Это очень важное замечание так как, в случае автоматического провода
// зеркальной проводки знак количества меняется на противоположный.
//
int SLAPI Transfer::AddItem(PPTransferItem * ti, int16 & rByBill, int use_ta)
{
	int    ok = 1, r;
	int    _reverse = 0; // @v8.0.3
	// @v8.0.3 const  int rbb = ti->RByBill;
	const  int reval = BIN(ti->Flags & PPTFR_REVAL);
	ReceiptTbl::Rec lot_rec;
	SString msg_buf, fmt_buf;
	double rest = 0.0;
	double ph_rest = 0.0;
	double qtty = ti->Quantity_;
	TransferTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		// @v8.0.3 THROW(RecByBill(ti->BillID, &ti->RByBill));
		// @v8.0.3 {
		assert(rByBill >= 0 && rByBill < 32000);
		if(ti->RByBill == 0) {
			ti->RByBill = ++rByBill;
			{
				//
				// Теоретически, в таблице Transfer не должно быть записей с ключем {ti->BillID, 0, ti->RByBill}.
				// Однако, учитывая то, что механизм новый и за хранение последнего rByBill отвечает другая таблица (Bill)
				// подстрахуемся использованием старого варианта.
				//
				TransferTbl::Key0 k0;
				k0.BillID  = ti->BillID;
				k0.Reverse = 0;
				k0.RByBill = ti->RByBill;
				if(search(0, &k0, spEq)) {
					PPFormatT(PPTXT_TRFRRBYBILLCONFLICT, &msg_buf, ti->BillID, ti->RByBill);
					PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
					{
						int16 new_rbb = 0;
						THROW(RecByBill(ti->BillID, &new_rbb));
						rByBill = ti->RByBill = new_rbb;
					}
				}
			}
		}
		else {
			TransferTbl::Key0 k0;
			k0.BillID  = ti->BillID;
			k0.Reverse = 0;
			k0.RByBill = ti->RByBill;
			r = search(0, &k0, spEq);
			if(ti->TFlags & PPTransferItem::tfForceNew) {
				THROW_PP(r == 0, PPERR_TRFRBYBILLBUSY);
				THROW_DB(oneof2(BtrError, BE_EOF, BE_KEYNFOUND));
			}
			else {
				THROW_PP(r, PPERR_TRFRBYBILLNFOUND);
				_reverse = 1;
			}
		}
		// } @v8.0.3
		//
		// Следующий участок кода форсирует генерацию лота в том случае, если операция является приходной,
		// не установлен флаг PPTFR_RECEIPT и лот операции нулевой либо его цены не совпадают с ценами операции.
		//
		if(ti->IsUnlimWoLot()) {
			MEMSZERO(lot_rec);
			ti->LotID = 0;
			ti->Flags &= ~PPTFR_RECEIPT;
			THROW_PP(!reval, PPERR_REVALONUNLIM);
		}
		else {
			if(ti->IsLotRet()) {
				if(ti->LotID) {
					THROW(Rcpt.Search(ti->LotID, &lot_rec) > 0);
					if(lot_rec.LocID != ti->LocID || lot_rec.GoodsID != ti->GoodsID) {
						ti->LotID = 0;
						ti->Flags |= PPTFR_RECEIPT;
					}
				}
				else
					ti->Flags |= PPTFR_RECEIPT;
			}
			if(ti->Flags & PPTFR_RECEIPT) {
				PPID   force_lot_id = 0;
				if(ti->TFlags & PPTransferItem::tfForceLotID && ti->LotID)
					force_lot_id = ti->LotID;
				THROW(AddLotItem(ti, force_lot_id));
			}
			THROW(GetRest(ti->LotID, ti->Date, &rest, &ph_rest));
			THROW_PP_S((rest >= 0.0 && ph_rest >= 0.0) ||
				((rest + ti->Quantity_) >= 0.0 && !(ti->Flags & PPTFR_RECEIPT)), PPERR_LOTRESTINVALID, ti->LotID); // @v8.0.9 (|| (rest + ti->Quantity_) >= 0.0 ...)
		}
		if(reval) {
			LDATE  dt = plusdate(ti->Date, 1);
			if(!ti->IsRecomplete())
				THROW_PP((ti->Flags & (PPTFR_ASSETEXPL|PPTFR_CORRECTION) || ti->Price != ti->Discount || ti->Cost != ti->RevalCost), PPERR_ZEROREVAL);
			THROW(UpdateFwRevalCostAndPrice(ti->LotID, dt, 0, ti->Cost, ti->Price, 0));
			if(ti->Flags & PPTFR_CORRECTION) {
				THROW(Rcpt.Search(ti->LotID, &lot_rec) > 0); // @v8.9.0 @fix (перенесено снизу вверх)
				THROW_PP(ti->Cost != ti->RevalCost || ti->Quantity_ != lot_rec.Quantity, PPERR_ZEROTICORRECTION);
				THROW_PP(qtty >= 0.0, PPERR_INVQTTY);
				// @v8.9.0 @fix THROW(Rcpt.Search(ti->LotID, &lot_rec) > 0);
				qtty = ti->Quantity_ - lot_rec.Quantity;
				ti->SetSignFlags(0, (qtty < 0.0) ? TISIGN_MINUS : TISIGN_PLUS);
			}
			else {
				qtty = 0.0;
			}
		}
		MEMSZERO(rec);
		rec.LocID    = ti->LocID;
		rec.Dt       = ti->Date;
		THROW(GetOprNo(rec.Dt, &rec.OprNo));
		rec.BillID   = ti->BillID;
		rec.RByBill  = ti->RByBill;
		rec.Reverse  = _reverse; // @v8.0.3 (rbb ? 1 : 0)-->_reverse
		rec.CorrLoc  = ti->CorrLoc;
		rec.LotID    = ti->LotID;
		rec.GoodsID  = ti->GoodsID;
		rec.Flags    = MASK_TFR_FLAGS(ti->Flags);
		rec.Quantity = qtty; // @v7.8.10 (reval ? 0.0 : ti->Quantity_)-->qtty
		rec.WtQtty   = (ti->Flags & PPTFR_INDEPPHQTTY) ? (float)R6(ti->WtQtty) : 0;
		if(ti->IsUnlimWoLot()) {
			rec.Rest = 0.0;
			rec.WtRest = 0.0f;
		}
		else {
			PPObject::SetLastErrObj(PPOBJ_GOODS, labs(rec.GoodsID));
			rec.Rest = R6(rest + rec.Quantity);
			rec.WtRest = (float)R6(ph_rest + rec.WtQtty);
			THROW_PP(rec.Rest >= 0, PPERR_LOTRESTBOUND);
		}
		rec.CurID = ti->CurID;
		ti->ConvertMoney(&rec);
		THROW_DB(insertRecBuf(&rec));
		THROW(UpdateForward(rec, rec.Quantity, rec.WtQtty));
		if((ti->Flags & PPTFR_UNITEINTR) && !rec.Reverse) {
			PPID   save_lot = ti->LotID;
			double save_dis = ti->Discount;
			THROW_PP(!(ti->Flags & PPTFR_UNLIM), PPERR_UNLIMINTROP);
			ti->Price   -= ti->Discount;
			ti->Discount = 0.0;
			(msg_buf = 0).Cat(ti->CorrLoc);
			THROW_PP_S(ti->CorrLoc && ti->CorrLoc != ti->LocID, PPERR_INVINTRCORRLOC, msg_buf);
			if(ti->CorrLoc)
				Exchange(&ti->LocID, &ti->CorrLoc);
			ti->Quantity_ = -ti->Quantity_;
			ti->WtQtty   = -ti->WtQtty;
			ti->Flags   &= ~PPTFR_UNITEINTR;
			ti->TFlags  &= ~PPTransferItem::tfForceNew; // @v8.0.3
			INVERSEFLAG(ti->Flags, PPTFR_RECEIPT);
			//
			// Откладываем обработку ошибки в AddItem до восстановления значений ti
			//
			r = AddItem(ti, rByBill, 0); // @recursion
			INVERSEFLAG(ti->Flags, PPTFR_RECEIPT);
			ti->Flags   |= PPTFR_UNITEINTR;
			ti->Quantity_ = -ti->Quantity_;
			ti->WtQtty   = -ti->WtQtty;
			ti->Discount = save_dis;
			ti->Price   += save_dis;
			if(ti->CorrLoc)
				Exchange(&ti->LocID, &ti->CorrLoc);
			ti->LotID = save_lot;
			THROW(r);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
Transfer::LcrBlock::LcrBlock(int op, LotCurRestTbl * pTbl, BExtInsert * pBei)
{
	assert(oneof3(op, opTest, opRecalc, opUpdate));
	Op = op;
	P_Tbl = pTbl;
	P_Bei = pBei;
	InitLot(0);
}

int Transfer::LcrBlock::InitLot(PPID lotID)
{
	int    ok = 1;
	LotID = lotID;
	LastDate = ZERODATE;
	LastRest = 0.0;
	CurTrfrRest = 0.0;
	List.clear();
	if(Op == opRecalc && P_Tbl && lotID) {
		THROW_DB(deleteFrom(P_Tbl, 0, (P_Tbl->LotID == lotID)));
	}
	CATCHZOK
	return ok;
}

int Transfer::LcrBlock::Update(PPID lotID, LDATE dt, double addendum)
{
	//
	// Алгоритм, реализуемый функцией, обрабатывает записи по лоту lotID
	// в хронологическом порядке:
	// 1. Сначала, если необходимо, заполняются "дырки" от последней записи до даты (dt-1)
	// 2. Затем модифицируется или добавляется запись на дату dt
	// 3. Пересчитываются форвардные записи
	//
	int    ok = 1;
	if(Op == opUpdate && P_Tbl && addendum != 0.0) {
		const int16 dti = WorkDate::ShrinkDate(dt);
		double last_rest = 0.0;
		LotCurRestTbl::Key0 k0;
		THROW(InitLot(lotID));
		{
			//
			// Необходимо оглянуться назад. Если есть запись с остатком по этому лоту
			// за дату, отстоящую от dt более чем на один день, то следует заполнить
			// дырки (то есть дни, когда не было операций по лоту) остатком из той самой
			// последеней записи.
			//
			k0.LotID = lotID;
			k0.D = dti;
			if(P_Tbl->search(0, &k0, spLt) && P_Tbl->data.LotID == lotID) {
				last_rest = P_Tbl->data.Rest;
				if((dti - P_Tbl->data.D) > 1) {
					for(int16 _d = (P_Tbl->data.D+1); _d < dti; _d++) {
						THROW(AddItem(statusAddRec, WorkDate::ExpandDate(_d), 0.0, last_rest));
					}
				}
			}
		}
		{
			//
			// Обрабатываем дату операции
			//
			k0.LotID = lotID;
			k0.D = dti;
			if(P_Tbl->searchForUpdate(0, &k0, spEq) && P_Tbl->data.LotID == lotID) {
				last_rest = (P_Tbl->data.Rest + addendum);
				THROW(AddItem(statusUpdRec, dt, P_Tbl->data.Rest, last_rest));
			}
			else {
				THROW(PPDbSearchError());
				last_rest += addendum;
				THROW(AddItem(statusAddRec, dt, 0.0, last_rest));
			}
		}
		{
			//
			// Обрабатываем форвардные записи
			//
			k0.LotID = lotID;
			k0.D = dti;
			while(P_Tbl->searchForUpdate(0, &k0, spGt) && P_Tbl->data.LotID == lotID) {
				THROW(AddItem(statusUpdRec, WorkDate::ExpandDate(P_Tbl->data.D), P_Tbl->data.Rest, P_Tbl->data.Rest + addendum));
			}
			THROW(PPDbSearchError());
		}
		THROW(FinishLot());
	}
	CATCHZOK
	return ok;
}

int Transfer::LcrBlock::AddItem(int status, LDATE dt, double exRest, double newRest)
{
	Item item;
	item.Dt = dt;
	item.Status = status;
	item.ExRest = exRest;
	item.ValidRest = newRest;
	return List.insert(&item) ? 1 : PPSetErrorSLib();
}

int FASTCALL Transfer::LcrBlock::Process(const TransferTbl::Rec & rTrfrRec)
{
	int    ok = 1;
	CurTrfrRest += rTrfrRec.Quantity;
	if(LastDate && LastDate != rTrfrRec.Dt) {
		THROW(AddItem(statusAddRec, LastDate, 0.0, LastRest));
		for(LDATE _d = plusdate(LastDate, 1); _d < rTrfrRec.Dt; _d = plusdate(_d, 1)) {
			THROW(AddItem(statusAddRec, _d, 0.0, LastRest));
		}
	}
	LastDate = rTrfrRec.Dt;
	LastRest = CurTrfrRest;
	CATCHZOK
	return ok;
}

int Transfer::LcrBlock::FinishLot()
{
	int    ok = 1;
	uint   i;
	int    inner_bei = 0; // Признак того, что функция создала собственный экземпляр BExtInsert
	BExtInsert * p_bei = 0;
	if(LastDate && oneof2(Op, opRecalc, opTest)) {
		THROW(AddItem(statusAddRec, LastDate, 0.0, LastRest));
	}
	if(P_Tbl) {
		if(Op == opTest) {
			const double epsilon = 1E-6;
			for(i = 0; i < List.getCount(); i++) {
				Item & r_item = List.at(i);
				if(oneof2(r_item.Status, statusAddRec, statusUpdRec)) {
					int    status = statusNone;
					LotCurRestTbl::Key0 k0;
					k0.LotID = LotID;
					k0.D = WorkDate::ShrinkDate(r_item.Dt);
					int r = SearchByKey(P_Tbl, 0, &k0, 0);
					if(r > 0) {
						r_item.ExRest = P_Tbl->data.Rest;
						if(fabs(r_item.ExRest - r_item.ValidRest) > epsilon) {
							r_item.Status = statusInvRest;
						}
					}
					else if(r < 0)
						r_item.Status = statusAbsence;
					else
						r_item.Status = statusDb;
				}
			}
			//
			// Проверка на наличие лишних записей.
			// В массиве TestList у нас содержатся все необходимые записи.
			// Извлекаем из базы данных все имеющиеся записи и ищем среди них те,
			// которых нет в TestList - это и будут лишние записи. Вставляем их в TestList
			// с кодом ошибки errWaste.
			//
			BExtQuery q(P_Tbl, 0);
			LotCurRestTbl::Key0 k0;
			k0.LotID = LotID;
			k0.D = 0;
			q.select(P_Tbl->D, P_Tbl->Rest, 0L).where(P_Tbl->LotID == LotID);
			for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
				LDATE dt = WorkDate::ExpandDate(P_Tbl->data.D);
				uint pos = 0;
				if(!List.lsearch(&dt, &pos, CMPF_LONG) || List.at(pos).Status == statusRmvRec)
					THROW(AddItem(statusWaste, dt, P_Tbl->data.Rest, 0.0));
			}
			//
			// Наконец сортируем массив дабы клиенту было удобнее с ним работать
			//
			List.sort(CMPF_LONG);
		}
		else if(oneof2(Op, opRecalc, opUpdate)) {
			if(!P_Bei) {
				THROW_MEM(p_bei = new BExtInsert(P_Tbl));
				inner_bei = 1;
			}
			else
				p_bei = P_Bei;
			for(i = 0; i < List.getCount(); i++) {
				Item & r_item = List.at(i);
				if(oneof2(r_item.Status, statusUpdRec, statusRmvRec)) {
					LotCurRestTbl::Key0 k0;
					k0.LotID = LotID;
					k0.D = WorkDate::ShrinkDate(r_item.Dt);
					THROW_DB(P_Tbl->searchForUpdate(0, &k0, spEq));
					if(r_item.Status == statusUpdRec) {
						P_Tbl->data.Rest = r_item.ValidRest;
						THROW_DB(P_Tbl->updateRec()); // @sfu
					}
					else if(r_item.Status == statusRmvRec) {
						THROW_DB(P_Tbl->deleteRec()); // @sfu
					}
				}
				else if(r_item.Status == statusAddRec) {
					LotCurRestTbl::Rec rec;
					MEMSZERO(rec);
					rec.LotID = LotID;
					rec.D = WorkDate::ShrinkDate(r_item.Dt);
					rec.Rest = r_item.ValidRest;
					THROW_DB(p_bei->insert(&rec));
				}
			}
			THROW_DB(p_bei->flash());
		}
	}
	CATCHZOK
	if(!ok)
		PPSaveErrContext();
	if(inner_bei)
		delete p_bei;
	if(!ok)
		PPRestoreErrContext();
	return ok;
}

int Transfer::LcrBlock::HasError() const
{
	int    yes = 0;
	const  uint c = List.getCount();
	for(uint i = 0; !yes && i < c; i++) {
		const int status = List.at(i).Status;
		if(oneof4(status, statusInvRest, statusAbsence, statusWaste, statusDb))
			yes = 1;
	}
	return yes;
}

int Transfer::LcrBlock::TranslateErr(PPLotFaultArray * pLfa) const
{
	int    ok = -1;
	const uint c = List.getCount();
	for(uint i = 0; i < c; i++) {
		const Item & r_item = List.at(i);
		if(r_item.Status > 0) {
			PPLotFault f;
			MEMSZERO(f);
			f.Dt = r_item.Dt;
			f.ActualVal = r_item.ExRest;
			f.ValidVal = r_item.ValidRest;
			switch(r_item.Status) {
				case statusInvRest:
					f.Fault = PPLotFault::LcrInvRest;
					break;
				case statusAbsence:
					f.Fault = PPLotFault::LcrAbsence;
					{
						LDATE prev_date = r_item.Dt;
						while((i+1) < c) {
							const Item & r_item2 = List.at(i+1);
							if(r_item2.Status == r_item.Status && r_item2.Dt == plusdate(prev_date, 1)) {
								prev_date = f.EndDate = r_item2.Dt;
								i++;
							}
							else
								break;
						}
					}
					break;
				case statusWaste:
					f.Fault = PPLotFault::LcrWaste;
					{
						LDATE prev_date = r_item.Dt;
						while((i+1) < c) {
							const Item & r_item2 = List.at(i+1);
							if(r_item2.Status == r_item.Status && r_item2.Dt == plusdate(prev_date, 1)) {
								prev_date = f.EndDate = r_item2.Dt;
								i++;
							}
							else
								break;
						}
					}
					break;
				case statusDb:
					f.Fault = PPLotFault::LcrDb;
					break;
			}
			if(f.Fault) {
				THROW_SL(pLfa->insert(&f));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::Helper_RecalcLotCRest(PPID lotID, BExtInsert * pBei, int forceRebuild)
{
	int    ok = -1;
	if(P_LcrT && lotID) {
		int    do_rebuild = 1;
		if(!forceRebuild) {
			LotCurRestTbl::Key0 k0;
			k0.LotID = lotID;
			k0.D = 0;
			if(P_LcrT->search(0, &k0, spGe) && P_LcrT->data.LotID == lotID) {
				TransferTbl::Key2 k;
				BExtQuery q(this, 2, 16);
				q.select(this->Dt, this->Quantity, 0L).where(this->LotID == lotID);
				k.LotID = lotID;
				k.Dt    = ZERODATE;
				k.OprNo = 0;
				LcrBlock lcr(LcrBlock::opTest, P_LcrT, pBei);
				THROW(lcr.InitLot(lotID));
				for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
					THROW(lcr.Process(data));
				}
				THROW(lcr.FinishLot());
				if(!lcr.HasError())
					do_rebuild = 0;
			}
		}
		if(do_rebuild) {
			TransferTbl::Key2 k;
			BExtQuery q(this, 2, 16);
			q.select(this->Dt, this->Quantity, 0L).where(this->LotID == lotID);
			k.LotID = lotID;
			k.Dt    = ZERODATE;
			k.OprNo = 0;
			LcrBlock lcr(LcrBlock::opRecalc, P_LcrT, pBei);
			THROW(lcr.InitLot(lotID));
			for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
				THROW(lcr.Process(data));
			}
			THROW(lcr.FinishLot());
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::GetLcrList(LDATE dt, UintHashTable * pLotList, RAssocArray * pRestList)
{
	int    ok = 1;
	if(P_LcrT && dt) {
		const int16 dti = WorkDate::ShrinkDate(dt);
		LotCurRestTbl::Key1 k1;
		k1.D = dti;
		k1.LotID = 0;
		BExtQuery q(P_LcrT, 1);
		if(pRestList)
			q.select(P_LcrT->LotID, P_LcrT->Rest, 0L);
		else
			q.select(P_LcrT->LotID, 0L);
		q.where(P_LcrT->D == (long)dti);
		for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
			if(pLotList) {
				THROW_SL(pLotList->Add((ulong)P_LcrT->data.LotID));
			}
			if(pRestList) {
				THROW_SL(pRestList->Add(P_LcrT->data.LotID, P_LcrT->data.Rest, 0, 0));
			}
		}
		CALLPTRMEMB(pRestList, SortByKey());
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI Transfer::UpdateCascadeLot(PPID lotID, PPID ownBillID, TrUCL_Param * p, uint flags, int use_ta)
{
	int    ok = 1, r;
	PPID   prev_lot_id = 0;
	ReceiptTbl::Rec org_lot_rec;
	LDATE  dt = ZERODATE;
	long   oprno = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Rcpt.Search(lotID, &org_lot_rec) > 0);
		prev_lot_id = org_lot_rec.PrevLotID;
		if(flags & TRUCLF_UPDGOODS) {
			double delta = ((org_lot_rec.GoodsID >= 0) || !(org_lot_rec.Flags & LOTF_CLOSEDORDER)) ? org_lot_rec.Rest : 0.0;
			THROW(UpdateCurRest(org_lot_rec.GoodsID, org_lot_rec.LocID, /*-org_lot_rec.Rest*/-delta));
			THROW(UpdateCurRest(p->GoodsID, org_lot_rec.LocID, /*org_lot_rec.Rest*/delta));
		}
		while((r = Rcpt.EnumRefs(lotID, &dt, &oprno)) > 0) {
			if(Rcpt.data.ID != lotID) {
				TrUCL_Param tmp_param = *p;
				THROW(UpdateCascadeLot(Rcpt.data.ID, ownBillID, &tmp_param, (flags & ~TRUCLF_UPDPRICE), 0)); // @recursion
			}
		}
		THROW(r);
		if(flags & (TRUCLF_UPDGOODS|TRUCLF_UPDCP)) {
			TransferTbl::Rec en_trfr_rec;
			for(dt = ZERODATE, oprno = 0; EnumByLot(lotID, &dt, &oprno, &en_trfr_rec) > 0;) {
				const  PPID bill_id = en_trfr_rec.BillID;
				if(bill_id != ownBillID) { // @v9.0.10
					int    is_const_c_reval = 1; // Cost was not modified
					int    is_const_p_reval = 1; // Price was not modified
					double new_cost;  // Undefined if is_const_c_reval;
					double new_price; // Undefined if is_const_p_reval;
					if(en_trfr_rec.Flags & PPTFR_REVAL) {
						DBRowId pos;
						ReceiptTbl::Rec tmp_lot_rec = org_lot_rec;
						const double old_cost  = TR5(en_trfr_rec.Cost);
						const double old_price = TR5(en_trfr_rec.Price);
						//
						// Saving table position {
						//
						THROW_DB(getPosition(&pos));
						//
						// Initializing vars is_const_c_reval and is_const_p_reval
						//
						THROW(GetLotPrices(&tmp_lot_rec, en_trfr_rec.Dt, en_trfr_rec.OprNo));
						new_cost  = R5(tmp_lot_rec.Cost);
						new_price = R5(tmp_lot_rec.Price);
						if(dbl_cmp(old_cost, new_cost))
							is_const_c_reval = 0;
						if(dbl_cmp(old_price, new_price))
							is_const_p_reval = 0;
						//
						// } Restoring table position
						//
						THROW_DB(getDirect(0, 0, pos));
						copyBufTo(&en_trfr_rec);
					}
					if(flags & TRUCLF_UPDGOODS)
						en_trfr_rec.GoodsID = p->GoodsID;
					if(flags & TRUCLF_UPDCOST)
						en_trfr_rec.Cost = TR5(p->Cost);
					if(flags & TRUCLF_UPDPRICE) {
						if(en_trfr_rec.Flags & PPTFR_REVAL)
							en_trfr_rec.Price = TR5(p->Price);
						else {
							const double p0 = TR5(en_trfr_rec.Price);
							const double d0 = TR5(en_trfr_rec.Discount);
							en_trfr_rec.Price    = TR5(p->Price);
							en_trfr_rec.Discount = (en_trfr_rec.Flags & PPTFR_RECEIPT && !prev_lot_id) ? 0 : TR5(p->Price - p0 + d0);
						}
					}
					THROW_DB(updateRecBuf(&en_trfr_rec));
					//
					// Если изменилась цена поступления или цена реализации, то
					// обновляем проводки по всем документам кроме собственно
					// документа, по которому был приход. Здесь мы полагаемся на
					// косвенные (но вполне надежные) признаки: предыдущий лот равен
					// нулю и товарная проводка порождает лот.
					//
					// @v9.0.10 if((flags & TRUCLF_UPDCP) && (!prev_lot_id || !(en_trfr_rec.Flags & PPTFR_RECEIPT))) { // @fix prev_lot_id-->!prev_lot_id
					if(flags & TRUCLF_UPDCP) { // @v9.0.10
						// @v9.0.10 assert(bill_id != ownBillID);
						PPObjBill * p_bobj = BillObj;
						DateIter diter;
						BillTbl::Rec bill_rec;
						THROW(p_bobj->RecalcTurns(bill_id, 0, 0));
						while(p_bobj->P_Tbl->EnumLinks(bill_id, &diter, BLNK_PAYMENT, &bill_rec) > 0) {
							THROW(p_bobj->RecalcTurns(bill_rec.ID, 0, 0));
						}
					}
					if(!is_const_c_reval && flags & TRUCLF_UPDCOST) {
						p->LotCost = R5(org_lot_rec.Cost);
						p->Cost = new_cost;
						flags &= ~TRUCLF_UPDCOST;
					}
					if(!is_const_p_reval && flags & TRUCLF_UPDPRICE) {
						p->LotPrice = R5(org_lot_rec.Price);
						p->Price = new_price;
						flags &= ~TRUCLF_UPDPRICE;
					}
				}
			}
		}
		if(flags & TRUCLF_UPDGOODS)
			org_lot_rec.GoodsID = p->GoodsID;
		if(flags & TRUCLF_UPDSUPPL)
			org_lot_rec.SupplID = p->SupplID;
		if(flags & TRUCLF_UPDTAXGRP)
			org_lot_rec.InTaxGrpID = p->LotTaxGrpID;
		if(flags & TRUCLF_UPDCOST) {
			p->LotCost = p->Cost;
			org_lot_rec.Cost = R5(p->LotCost);
		}
		if(flags & TRUCLF_UPDPRICE) {
			p->LotPrice = p->Price;
			org_lot_rec.Price = R5(p->LotPrice);
		}
		THROW_DB(Rcpt.Update(lotID, &org_lot_rec, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::UpdateReceipt(PPID lotID, PPTransferItem * ti, PPID prevLotID, long flags)
{
	int    ok = 1;
	uint   ucl_flags = 0;
	ReceiptTbl::Rec lot_rec;
	THROW(Rcpt.Search(lotID, &lot_rec) > 0);
	if(ti->CorrLoc && ti->BillID != lot_rec.BillID) {
		//
		// Если лот был создан документом, отличным от документа, которому соответствует параметр ti,
		// то никаких модификаций лота не делаем. Эта ситуация возникает при циклическом межскладе,
		// когда фактически лот не создается, а лишь меняется его остаток. Вообще метод, которым мы
		// определяем такую ситуацию является косвенным (зато очень быстрым). Более корректным было-бы
		// выяснить, существуют-ли проводки по этому лоту, сделанные до проводки, определяющей ti.
		//
	   	;
	}
	else {
		{
			ReceiptTbl::Rec _rr = lot_rec;
			if(lot_rec.GoodsID != ti->GoodsID)
				ucl_flags |= TRUCLF_UPDGOODS;
			if(lot_rec.SupplID != ti->Suppl)
				ucl_flags |= TRUCLF_UPDSUPPL;
			if(lot_rec.InTaxGrpID != ti->LotTaxGrpID)
				ucl_flags |= TRUCLF_UPDTAXGRP;
			THROW(GetLotPrices(&_rr, _rr.Dt, 1));
			if(dbl_cmp(_rr.Cost, ti->Cost))
				ucl_flags |= TRUCLF_UPDCOST;
			if(dbl_cmp(_rr.Price, ti->Price))
				ucl_flags |= TRUCLF_UPDPRICE;
		}
		if(ucl_flags) {
			TrUCL_Param ucl_param;
			MEMSZERO(ucl_param);
			ucl_param.GoodsID     = ti->GoodsID;
			ucl_param.SupplID     = ti->Suppl;
			ucl_param.LotTaxGrpID = ti->LotTaxGrpID;
			ucl_param.Cost        = ti->Cost;
			ucl_param.Price       = ti->Price;
			ucl_param.LotCost     = R5(lot_rec.Cost);
			ucl_param.LotPrice    = R5(lot_rec.Price);
			if(!(flags & fUpdEnableUpdChildLot)) {
				if(lot_rec.PrevLotID == lot_rec.ID)
					lot_rec.PrevLotID = 0;
				THROW_PP(lot_rec.PrevLotID == 0, PPERR_UPDCHILDLOT);
			}
			THROW(UpdateCascadeLot(lotID, ti->BillID, &ucl_param, ucl_flags, 0));
			lot_rec.GoodsID    = ti->GoodsID;
			lot_rec.SupplID    = ti->Suppl;
			lot_rec.InTaxGrpID = ti->LotTaxGrpID;
			lot_rec.Cost       = R5(ucl_param.LotCost);
			lot_rec.Price      = R5(ucl_param.LotPrice);
			lot_rec.ExtCost    = R5(ti->ExtCost);
		}
		if(prevLotID)
			lot_rec.PrevLotID = prevLotID;
		if(lot_rec.Dt != ti->Date) {
			lot_rec.Dt = ti->Date;
			THROW(GetLotOprNo(lot_rec.Dt, &lot_rec.OprNo));
		}
		SETFLAG(lot_rec.Flags, LOTF_COSTWOVAT,   ti->Flags & PPTFR_COSTWOVAT);
		SETFLAG(lot_rec.Flags, LOTF_COSTWSTAX,   ti->Flags & PPTFR_COSTWSTAX);
		SETFLAG(lot_rec.Flags, LOTF_ORDRESERVE,  ti->TFlags & PPTransferItem::tfOrdReserve);
		SETFLAG(lot_rec.Flags, LOTF_CLOSEDORDER, ti->Flags & PPTFR_CLOSEDORDER);
		if(lot_rec.Flags & LOTF_CLOSEDORDER)
			lot_rec.Closed = 1;
		lot_rec.UnitPerPack = ti->UnitPerPack;
		lot_rec.QCertID     = ti->QCert;
		lot_rec.Expiry      = ti->Expiry;
		PPObject::SetLastErrObj(PPOBJ_GOODS, labs(lot_rec.GoodsID));
		THROW_PP((lot_rec.Quantity = ti->Quantity_) > 0.0, PPERR_LOTRESTBOUND);
		lot_rec.WtQtty = (float)R6(ti->WtQtty);
		THROW(Rcpt.Update(lotID, &lot_rec, 0));
	}
	CATCHZOK
	return ok;
}
//
// Функция UpdateItem может изменить только шесть полей структуры
// PPTransferItem: UnitPerPack, Quantity, Cost, Price, Discount, GoodsID.
// Наибольшую сложность представляет модификация поля Quantity, так как она требует
// пересчета остатков (возможно форвардных). Все изменения делаются по всем необходимым
// таблицам. Следует отметить, что модификация ценовых полей происходит просто как
// изменение соответствующих полей таблиц данных, но не как операция переоценки.
// В параметре ti должны быть корректно установлены поля BillID и RByBill.
// По этим полям будет осуществляться первоначальный поиск модифицируемых записей.
// В считанной записи TransferTbl::Rec проверяется поле Flags на флажок PPTFR_RECEIPT.
// Если этот флажок установлен, то будет модифицироваться приходная запись.
//
int SLAPI Transfer::UpdateItem(PPTransferItem * ti, int16 & rByBill, long flags, int use_ta)
{
	return UpdateItem(ti, rByBill, 0, flags, use_ta);
}
//
// Функция UpdateItem умеет выправлять несоответствующие зеркальные проводки.
// Для этой цели необходимо параметр reverse установить в значение 1.
//
int SLAPI Transfer::UpdateItem(PPTransferItem * ti, int16 & rRByBill, int reverse, long flags, int use_ta)
{
	int    ok = 1, r;
	short  _rbb = ti->RByBill;
	double rest    = 0.0;
	double ph_rest = 0.0;
	double qtty    = 0.0;
	double ph_qtty = 0.0;
	double new_qtty    = ti->Quantity_;
	double new_ph_qtty = R6(ti->WtQtty);
	long   upd_oprno   = 0;
	int    sav = 0;
	double save_dis;
	DBRowId pos;
	TransferTbl::Rec rec;
	ReceiptTbl::Rec lot_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(reverse == 0) {
			THROW(SearchByBill(ti->BillID, 0, _rbb, &rec) > 0);
			const PPID   org_lot_id  = rec.LotID; // @v9.0.0
			const LDATE  org_dt      = rec.Dt;
			const long   org_oprno   = rec.OprNo;
			const double org_qtty    = rec.Quantity;
			const double org_ph_qtty = R6(rec.WtQtty);
			rec.GoodsID = ti->GoodsID;
			rest    = rec.Rest;
			ph_rest = R6(rec.WtRest);
			if(!(rec.Flags & PPTFR_REVAL)) {
				if(!__DontCheckQttyInUpdateTransferItem__ && !ti->IsCorrectionExp()) {
					THROW_PP((org_qtty * new_qtty) > 0, PPERR_INVQTTY);
					THROW_PP((rec.WtQtty * new_ph_qtty) >= 0, PPERR_INVQTTY);
				}
				qtty    = new_qtty - org_qtty;
				ph_qtty = new_ph_qtty - R6(rec.WtQtty);
				if(rec.Flags & PPTFR_RECEIPT) {
					if(ti->LotID || org_lot_id) {
						ti->LotID = org_lot_id;
						if(ti->Date != rec.Dt) {
							LDATE temp_dt = rec.Dt;
							long  temp_oprno = rec.OprNo;
							if(EnumByLot(ti->LotID, &temp_dt, &temp_oprno, 0) > 0)
								THROW_PP(ti->Date < temp_dt, PPERR_INVLOTDTUPD);
							THROW(GetOprNo(ti->Date, &upd_oprno));
							rec.OprNo = upd_oprno;
							// @v9.0.0 {
							if(P_LcrT) {
								LcrBlock lcr(LcrBlock::opUpdate, P_LcrT, 0);
								THROW(lcr.Update(org_lot_id, org_dt, -org_qtty));
								THROW(lcr.Update(org_lot_id, ti->Date, new_qtty));
							}
							// } @v9.0.0
						}
						THROW(UpdateReceipt(rec.LotID, ti, 0, flags));
					}
				}
				else if(ti->LotID != org_lot_id || ti->Date != org_dt) {
					qtty    = new_qtty;
					ph_qtty = new_ph_qtty;
					if(ti->IsUnlimWoLot()) {
						rest   = 0.0;
						rec.Dt = ti->Date;
						THROW(GetOprNo(ti->Date, &upd_oprno));
						rec.OprNo = upd_oprno;
					}
					else {
						THROW(Rcpt.Search(ti->LotID) > 0);
						const LDATE lot_dt = Rcpt.data.Dt;
						THROW(UpdateForward(rec, -org_qtty, -org_ph_qtty));
						if(ti->Date == lot_dt || ti->Date != org_dt) {
							THROW(GetOprNo(ti->Date, &upd_oprno));
							rec.OprNo = upd_oprno;
						}
						rec.LotID = ti->LotID;
						rec.Dt    = ti->Date;
						THROW(r = GetRest(ti->LotID, rec.Dt, rec.OprNo, &rest, &ph_rest));
						if(r > 0 && data.Dt == org_dt && data.OprNo == org_oprno) {
							rest    -= org_qtty;
							ph_rest -= org_ph_qtty;
						}
					}
				}
			}
			else { // (rec.Flags & PPTFR_REVAL)
				uint   uf = 0;
				THROW_PP(ti->Date == rec.Dt, PPERR_REVALDTUPD);
				THROW_PP(ti->Flags & (PPTFR_ASSETEXPL|PPTFR_CORRECTION) || ti->Price != ti->Discount || ti->Cost != ti->RevalCost, PPERR_ZEROREVAL);
				THROW(UpdateFwRevalCostAndPrice(rec.LotID, plusdate(rec.Dt, 1), 0, ti->Cost, ti->Price, &uf));
				if(ti->Flags & PPTFR_INDEPPHQTTY && ti->IsRecomplete()) {
					//
					// При рекомплектации количество измениться не может, однако может измениться //
					// независимое физическое количество.
					//
					ph_qtty = new_ph_qtty - R6(rec.WtQtty);
				}
				if(ti->Flags & PPTFR_CORRECTION) {
					THROW(Rcpt.Search(ti->LotID, &lot_rec) > 0);
					THROW_PP(ti->Cost != ti->RevalCost || ti->Quantity_ != lot_rec.Quantity, PPERR_ZEROTICORRECTION);
					THROW_PP(qtty >= 0.0, PPERR_INVQTTY);
					new_qtty = ti->Quantity_ - lot_rec.Quantity;
					qtty     = new_qtty - rec.Quantity;
					ti->SetSignFlags(0, (new_qtty < 0.0) ? TISIGN_MINUS : TISIGN_PLUS);
				}
				else {
					qtty = new_qtty = 0.0;
				}
			}
			THROW(SearchByBill(ti->BillID, 0, _rbb, 0) > 0);
			data.LotID = ti->LotID;
			//
			// @v9.0.12 Из-за изменения техники каскадной модификации лотов это присвоение необходимо
			//
			if(rec.Flags & PPTFR_RECEIPT) {
				data.GoodsID = ti->GoodsID;
			}
			//
			data.Dt    = ti->Date;
			if(upd_oprno)
				data.OprNo = upd_oprno;
			data.CurID = ti->CurID;
			ti->ConvertMoney(&data);
			if(ti->IsUnlimWoLot()) {
				data.Rest   = 0.0;
				data.WtRest = 0.0;
			}
			else {
				PPObject::SetLastErrObj(PPOBJ_GOODS, labs(data.GoodsID));
				THROW_PP((data.Rest = R6(rest + qtty)) >= 0.0, PPERR_LOTRESTBOUND);
				data.WtRest = (float)R6(ph_rest + ph_qtty);
			}
			data.Quantity = new_qtty;
			data.WtQtty   = (float)R6(new_ph_qtty);
			data.Flags = MASK_TFR_FLAGS(ti->Flags);
			THROW_DB(updateRec());
			THROW(UpdateForward(rec, qtty, ph_qtty));
		}
		//
		// Ищем зеркальную проводку
		//
		_rbb = ti->RByBill;
		if((r = SearchByBill(ti->BillID, 1, _rbb, &rec)) > 0) {
			int    is_row_rebuilded = 0;
			upd_oprno = 0;
			THROW_DB(getPosition(&pos));
			rec.GoodsID  = ti->GoodsID;
			new_qtty     = -new_qtty;
			new_ph_qtty  = -new_ph_qtty;
			ti->Quantity_ = -ti->Quantity_;
			ti->WtQtty   = -ti->WtQtty;
			if(!__DontCheckQttyInUpdateTransferItem__) {
				THROW_PP((rec.Quantity * new_qtty) > 0, PPERR_INVQTTY);
				THROW_PP((rec.WtQtty * new_ph_qtty) >= 0, PPERR_INVQTTY);
			}
			qtty    = new_qtty - rec.Quantity;
			ph_qtty = new_ph_qtty - R6(rec.WtQtty);
			if(rec.Flags & PPTFR_RECEIPT) {
				if(ti->Date != rec.Dt) {
					LDATE  temp_dt = rec.Dt;
					long   temp_oprno = rec.OprNo;
					if(EnumByLot(rec.LotID, &temp_dt, &temp_oprno, 0) > 0)
						THROW_PP(ti->Date < temp_dt, PPERR_INVLOTDTUPD);
					THROW(GetOprNo(ti->Date, &upd_oprno));
					rec.Dt    = ti->Date;
					rec.OprNo = upd_oprno;
				}
				save_dis     = ti->Discount;
				ti->Price   -= ti->Discount;
				ti->Discount = 0;
				sav          = 1;
				THROW(UpdateReceipt(rec.LotID, ti, ti->LotID, fUpdEnableUpdChildLot));
			}
			else if(rec.LotID) {
				//
				// Этот участок кода работает в случае, если зеркальная запись
				// не генерирует собственный лот, а вливается в существующий
				//
				ReceiptTbl::Rec tmp_lot_rec;
				MEMSZERO(tmp_lot_rec);
				tmp_lot_rec.ID = rec.LotID;
				//
				// Если цены изменились, то операция уже не может вливаться в
				// существующий лот, а должна генерировать собственный.
				// При этом мы удаляем запись и вставляем ее снова, но уже с признаком PPTFR_RECEIPT.
				//
				THROW(Rcpt.Search(rec.LotID, &tmp_lot_rec) > 0);
				THROW(GetLotPrices(&tmp_lot_rec, rec.Dt, rec.OprNo));
				if(dbl_cmp(tmp_lot_rec.Cost, ti->Cost) != 0 || dbl_cmp(tmp_lot_rec.Price, ti->NetPrice()) != 0) {
					THROW(UpdateForward(rec, -rec.Quantity, -rec.WtQtty));
					THROW_DB(getDirect(0, 0, pos));
					THROW_DB(deleteRec());
					THROW_PP(!(ti->Flags & PPTFR_UNLIM), PPERR_UNLIMINTROP);
					{
						const PPID   save_lot     = ti->LotID;
						const double save_dis     = ti->Discount;
						const double save_qtty    = ti->Quantity_;
						const double save_ph_qtty = ti->WtQtty;
						ti->LotID    = 0;
						ti->Price   -= ti->Discount;
						ti->Discount = 0.0;
						if(ti->CorrLoc)
							Exchange(&ti->LocID, &ti->CorrLoc);
						ti->Quantity_ = fabs(ti->Quantity_);
						ti->WtQtty   = fabs(ti->WtQtty);
						ti->Flags   &= ~PPTFR_UNITEINTR;
						INVERSEFLAG(ti->Flags, PPTFR_RECEIPT);
						//
						// Откладываем обработку ошибки в AddItem до восстановления значений ti
						//
						r = AddItem(ti, rRByBill, 0);
						INVERSEFLAG(ti->Flags, PPTFR_RECEIPT);
						ti->Flags   |= PPTFR_UNITEINTR;
						ti->Quantity_ = save_qtty;
						ti->WtQtty   = save_ph_qtty;
						ti->Discount = save_dis;
						ti->Price   += save_dis;
						if(ti->CorrLoc)
							Exchange(&ti->LocID, &ti->CorrLoc);
						ti->LotID    = save_lot;
						is_row_rebuilded = 1;
					}
					THROW(r);
				}
			}
			if(!is_row_rebuilded) {
				THROW(UpdateForward(rec, qtty, ph_qtty));
				THROW_DB(getDirect(0, 0, pos));
				ti->ConvertMoney(&data);
				PPObject::SetLastErrObj(PPOBJ_GOODS, labs(data.GoodsID));
				THROW_PP((data.Rest = R6(data.Rest + qtty)) >= 0, PPERR_LOTRESTBOUND);
				data.WtRest = (float)R6(data.WtRest + ph_qtty);
				data.Dt = ti->Date;
				if(upd_oprno)
					data.OprNo = upd_oprno;
				data.Quantity = new_qtty;
				data.WtQtty   = (float)R6(new_ph_qtty);
				THROW_DB(updateRec());
			}
		}
		THROW(r);
		THROW(tra.Commit());
	}
	CATCHZOK
	if(sav) {
		ti->Discount = save_dis;
		ti->Price   += save_dis;
	}
	return ok;
}

int SLAPI Transfer::RemoveItem(PPID billID, int rByBill, int force, int use_ta)
{
	return RemoveItem(billID, 0, rByBill, force, use_ta);
}
//
// Функция _RemoveItem сделана с прицелом на удаление висячей
// зеркальной записи. Для удаления такой записи необходимо
// параметр reverse установить в значение 1.
//
int SLAPI Transfer::RemoveItem(PPID bill, int reverse, short rByBill, int force, int use_ta)
{
	int    ok = 1, r;
	LDATE  dt;
	long   oprno;
	PPID   remove_lot_id = 0;
	DBRowId pos;
	TransferTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchByBill(bill, BIN(reverse), rByBill, &rec) > 0);
		THROW_DB(getPosition(&pos));
		if(rec.Flags & PPTFR_RECEIPT) {
			if(rec.LotID) {
				{
					//
					// Проверяем, чтобы не осталось ни одной операции по удаляемому лоту.
					// Хотя функция UpdateForward косвенно выполняет такую проверку,
					// она не может засечь оставшуюся форвардную переоценку. Именно на
					// этот и подобные ему случаи и рассчитана данная проверка.
					//
					dt = rec.Dt;
					oprno = rec.OprNo;
					while((r = EnumByLot(rec.LotID, &dt, &oprno)) > 0)
						if(data.Flags & PPTFR_REVAL) {
							THROW_DB(deleteRec());
						}
						else {
							CALLEXCEPT_PP(PPERR_FWLOTRESTBOUND);
						}
					THROW(r);
				}
				//
				// Проверка на случай если по ошибке выставлен флаг PPTFR_RECEIPT, но
				// строка ни к какому лоту не привязана
				//
				THROW(r = Rcpt.EnumRefs(rec.LotID, &(dt = ZERODATE), &(oprno = 0)));
				THROW_PP(r < 0, PPERR_DELPARENTLOT);
				remove_lot_id = rec.LotID;
			}
			else
				rec.Flags &= ~PPTFR_RECEIPT;
		}
		else if(rec.Flags & PPTFR_REVAL) {
			if(!force) {
				uint uf = 0;
				THROW(UpdateFwRevalCostAndPrice(rec.LotID, rec.Dt, rec.OprNo, TR5(rec.Cost), TR5(rec.Price), &uf));
			}
		}
		if(!force) {
			THROW(UpdateForward(rec, -rec.Quantity, -rec.WtQtty));
		}
		if(remove_lot_id)
			THROW(Rcpt.Remove(rec.LotID, 0));
		THROW_DB(getDirectForUpdate(0, 0, pos)); // getDirect-->getDirectForUpdate
		THROW_DB(deleteRec()); // @sfu
		if(reverse == 0 && (r = SearchByBill(bill, 1, rByBill, &rec)) > 0) {
			remove_lot_id = 0;
			THROW_DB(getPosition(&pos));
			if(rec.Flags & PPTFR_RECEIPT) {
				dt = rec.Dt;
				oprno = rec.OprNo;
				while((r = EnumByLot(rec.LotID, &dt, &oprno)) > 0)
					if(data.Flags & PPTFR_REVAL) {
						THROW_DB(deleteRec());
					}
					else
						CALLEXCEPT_PP(PPERR_FWLOTRESTBOUND);
				THROW(r = Rcpt.EnumRefs(rec.LotID, &(dt = ZERODATE), &(oprno = 0)));
				// Если существует порожденный лот, то родителя не удаляем
				if(r < 0)
					remove_lot_id = rec.LotID;
			}
			if(!force) {
				THROW(UpdateForward(rec, -rec.Quantity, -rec.WtQtty));
			}
			if(remove_lot_id)
				THROW(Rcpt.Remove(rec.LotID, 0));
			THROW_DB(getDirectForUpdate(0, 0, pos));
			THROW_DB(deleteRec()); // @sfu
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::SubtractBillQtty(PPID billID, PPID lotID, double * pRest)
{
	if(billID && lotID) {
		TransferTbl::Key0 k;
		BExtQuery q(this, 0);
		q.select(this->Quantity, 0L).where(this->BillID == billID && this->Reverse == 0L && this->LotID == lotID);
		k.BillID  = billID;
		k.Reverse = k.RByBill = 0;
		for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;)
			*pRest -= data.Quantity;
	}
	return 1;
}
//
// Примитив для перемещения операции с одного лота на другой.
// Производит минимум проверок. По заданным параметром param данным
// перемещает операцию param->TrRec с лота SrcLot (SrcLotID == TrRec.LotID)
// на лот DestLotID (DestLot.GoodsID == SrcLot.GoodsID).
//
int SLAPI Transfer::MoveOp(LotOpMovParam * param, int use_ta)
{
	int    ok = 1;
	long   oprno = 0;
	double q, down_lim, up_lim;
	ReceiptTbl::Rec src_lot = param->SrcLot;
	PPTransferItem ti;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		ti.SetupByRec(&param->TrRec);
		SetupItemByLot(&ti, &src_lot, 1, param->TrRec.OprNo);
		THROW_PP(!(ti.Flags & PPTFR_REVAL),   PPERR_CANTMOVREVALOP);
		THROW_PP(!(ti.Flags & PPTFR_RECEIPT), PPERR_CANTMOVRCPTOP);
		q = ti.Quantity_;
		oprno = (param->DestLot.Dt == param->TrRec.Dt) ? -1 : param->TrRec.OprNo;
		THROW(GetBounds(param->DestLotID, param->TrRec.Dt, oprno, &down_lim, &up_lim));
		PPObject::SetLastErrObj(PPOBJ_GOODS, labs(ti.GoodsID));
		THROW_PP(((q > 0 || -q <= down_lim) || (q < 0 || q <= up_lim)), PPERR_LOTRESTBOUND);
		THROW(ti.SetupLot(param->DestLotID, &param->DestLot, TISL_ADJPRICE));
		{
			PPObjBill * p_bobj = BillObj;
			PPObjBill::TBlock tb_; // @v8.0.3
			THROW(p_bobj->BeginTFrame(ti.BillID, tb_)); // @v8.0.3
			THROW(UpdateItem(&ti, tb_.Rbb(), 0, 0));
			THROW(p_bobj->RecalcTurns(ti.BillID, 0, 0));
			THROW(p_bobj->FinishTFrame(ti.BillID, tb_)); // @v8.0.3
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::MergeLots(LotOpMovParam * param, uint flags, int use_ta)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	LDATE  dt = ZERODATE;
	long   oprno = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(EnumByLot(param->DestLotID, &dt, &oprno) > 0) {
			PPTransferItem ti;
			ReceiptTbl::Rec dest_lot = param->SrcLot;
			double src_cost  = R5(param->SrcLot.Cost);
			double src_price = R5(param->SrcLot.Price);
			double src_qtty  = param->SrcLot.Quantity;
			double sum_qtty  = param->DestLot.Quantity + src_qtty;
			PPObjBill::TBlock tb_; // @v8.0.3

			copyBufTo(&param->TrRec);
			ti.SetupByRec(&param->TrRec);
			THROW(SetupItemByLot(&ti, &dest_lot, 1, param->TrRec.OprNo));
			if(sum_qtty > 0.0) {
				if(flags & TMLOF_AVGCOST)
					ti.Cost = R2((ti.Cost * ti.Quantity_ + src_cost * src_qtty) / sum_qtty);
				if(flags & TMLOF_AVGPRICE)
					ti.Price = R2((ti.Price * ti.Quantity_ + src_price * src_qtty) / sum_qtty);
			}
			ti.Quantity_ = sum_qtty;
			THROW(p_bobj->BeginTFrame(ti.BillID, tb_)); // @v8.0.3
			THROW(UpdateItem(&ti, tb_.Rbb(), 0, 0));
			THROW(p_bobj->FinishTFrame(ti.BillID, tb_)); // @v8.0.3
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::InitLotOpMovParam(PPID srcLotID, PPID destLotID, LotOpMovParam * pParam, int zeroSrcLotID)
{
	int    ok = 1;
	memzero(pParam, sizeof(*pParam));
	pParam->SrcLotID  = srcLotID;
	pParam->DestLotID = destLotID;
	THROW_PP(srcLotID != destLotID, PPERR_INCOMPATLOTS);
	if(!zeroSrcLotID) {
		THROW(Rcpt.Search(srcLotID, &pParam->SrcLot) > 0);
	}
	else
		pParam->SrcLotID = 0;
	THROW(Rcpt.Search(destLotID, &pParam->DestLot) > 0);
	if(!zeroSrcLotID) {
		THROW_PP(pParam->SrcLot.GoodsID == pParam->DestLot.GoodsID &&
			pParam->SrcLot.LocID == pParam->DestLot.LocID, PPERR_INCOMPATLOTS);
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::MoveLotOp(PPID srcLotID, LDATE dt, long oprno, PPID destLotID, int use_ta)
{
	int    ok = 1;
	LotOpMovParam param;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(InitLotOpMovParam(srcLotID, destLotID, &param, 1));
		if(Search(srcLotID, dt, oprno, spEq) > 0) {
			if(srcLotID == 0 && data.Flags & PPTFR_RECEIPT) {
				//
				// Этот блок позволяет просто изменить значение поля LotID на destLotID в записи TransferTbl.
				// Это бывает необходимо в связи с ошибками из-за которых запись, порождающая лот, потеряла
				// ссылку на этот лот. Такая ошибка в частности возникла в v2.4.2
				//
				THROW_PP(data.GoodsID == param.DestLot.GoodsID && data.LocID == param.DestLot.LocID, PPERR_INCOMPATLOTS);
				data.LotID = destLotID;
				THROW_DB(updateRec());
			}
			else {
				copyBufTo(&param.TrRec);
				THROW_PP(!(param.TrRec.Flags & PPTFR_REVAL), PPERR_CANTMOVREVALOP);
				THROW_PP(!(param.TrRec.Flags & PPTFR_RECEIPT), PPERR_CANTMOVRCPTOP);
				THROW(MoveOp(&param, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::MoveLotOps(PPID srcLotID, PPID destLotID, long flags, int use_ta)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	LotOpMovParam param;
	LDATE  dt;
	long   oprno;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(InitLotOpMovParam(srcLotID, destLotID, &param, 0));
		if(flags & TMLOF_ADDLOTS) {
			THROW(MergeLots(&param, flags, 0));
			THROW(Rcpt.Search(destLotID, &param.DestLot) > 0);
			if(!(flags & TMLOF_NORECALCDESTBILL))
				THROW(p_bobj->RecalcTurns(param.DestLot.BillID, 0, 0));
		}
		for(dt = MAXDATE, oprno = MAXLONG; Search(srcLotID, dt, oprno, spLt) > 0;) {
			copyBufTo(&param.TrRec);
			dt = param.TrRec.Dt;
			oprno = param.TrRec.OprNo;
			long f = param.TrRec.Flags;
			if(f & (PPTFR_REVAL|PPTFR_RECEIPT)) {
				if(flags & TMLOF_RMVSRCLOT || (f & PPTFR_REVAL && flags & TMLOF_RMVREVAL)) {
					const PPID bill_id = param.TrRec.BillID;
					THROW(RemoveItem(bill_id, param.TrRec.RByBill, 0, 0));
					THROW(p_bobj->RecalcTurns(bill_id, 0, 0));
				}
			}
			else {
				THROW(MoveOp(&param, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Transfer::GetLastOpByGoods(PPID goodsID, LDATE beforeDt, long beforeOprNo, TransferTbl::Rec * pRec)
{
	int    ok = -1;
	TransferTbl::Key3 k3;
	k3.GoodsID = goodsID;
	k3.Dt = beforeDt;
	k3.OprNo = beforeOprNo;
	if(search(3, &k3, spLt) && k3.GoodsID == goodsID) {
		copyBufTo(pRec);
		ok = 1;
	}
	else
		ok = PPDbSearchError();
	return ok;
}
