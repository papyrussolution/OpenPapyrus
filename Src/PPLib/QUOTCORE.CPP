// QUOTCORE.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2011, 2012, 2013, 2014, 2015, 2016, 2017
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
//
//
SLAPI QuotIdent::QuotIdent(PPID locID, PPID quotKindID, PPID curID, PPID arID)
{
	Flags = 0;
	Period.SetZero();
	Dt = ZERODATE;
	LocID = locID;
	QuotKindID = quotKindID;
	CurID = curID;
	ArID = arID;
	Qtty_ = SMathConst::Max;
	PrevBase = 0.0;
}

SLAPI QuotIdent::QuotIdent(LDATE dt, PPID locID, PPID quotKindID, PPID curID, PPID arID)
{
	Flags = 0;
	Period.SetZero();
	Dt = dt;
	LocID = locID;
	QuotKindID = quotKindID;
	CurID = curID;
	ArID = arID;
	Qtty_ = SMathConst::Max;
	PrevBase = 0.0;
}

SLAPI QuotIdent::QuotIdent(const QuotIdent & rS)
{
	Flags = rS.Flags;
	Period = rS.Period;
	Dt = rS.Dt;
	LocID = rS.LocID;
	QuotKindID = rS.QuotKindID;
	CurID = rS.CurID;
	ArID = rS.ArID;
	Qtty_ = rS.Qtty_;
	PrevBase = rS.PrevBase;
}

SLAPI QuotIdent::QuotIdent(const PPQuot & rS)
{
	Flags = (fUsePeriod|fUseQtty);
	Period = rS.Period;
	Dt = ZERODATE;
	LocID = rS.LocID;
	QuotKindID = rS.Kind;
	CurID = rS.CurID;
	ArID = rS.ArID;
	Qtty_ = rS.MinQtty;
	PrevBase = 0.0;
}

void SLAPI QuotIdent::SetBase(double b)
{
	PrevBase = b;
}

double SLAPI QuotIdent::GetPrevBase() const
{
	return PrevBase;
}

void FASTCALL QuotIdent::SetIdentPeriod(const DateRange * pPeriod)
{
	if(pPeriod) {
		Period = *pPeriod;
		Flags |= fUsePeriod;
	}
	else {
		Period.SetZero();
		Flags &= ~fUsePeriod;
	}
}

void FASTCALL QuotIdent::SetIdentQtty(const long * pQtty)
{
	if(pQtty) {
		Qtty_ = (double)*pQtty;
		Flags |= fUseQtty;
	}
	else {
		Qtty_ = SMathConst::Max;
		Flags &= ~fUseQtty;
	}
}
//
//
//
PPQuotItem_::PPQuotItem_()
{
	THISZERO();
}

PPQuotItem_::PPQuotItem_(const PPQuot & rS)
{
	THISZERO();
	GoodsID = rS.GoodsID;
	KindID = rS.Kind;
	LocID = rS.LocID;
	ArID = rS.ArID;
	CurID = rS.CurID;
	Period = rS.Period;
	MinQtty = rS.MinQtty;
	Dtm = rS.Dtm;
	Flags = rS.Flags;
	Val = rS.Quot;
}

IMPL_CMPFUNC(PPQuotItem_, i1, i2) { RET_CMPCASCADE6((const PPQuotItem_ *)i1, (const PPQuotItem_ *)i2, GoodsID, KindID, LocID, ArID, CurID, Period); }

PPQuotItemArray::PPQuotItemArray() : TSVector <PPQuotItem_> () // @v9.8.4 TSArray-->TSVector
{
}

int FASTCALL PPQuotItemArray::Add(const PPQuotItem_ & rItem)
{
	return SVector::insert(&rItem) ? 1 : PPSetErrorSLib(); // @v9.8.4 SArray-->SVector
}

int FASTCALL PPQuotItemArray::Add(const PPQuot & rQ)
{
	PPQuotItem_ item(rQ);
	return Add(item);
}

int SLAPI PPQuotItemArray::Set(const PPQuot & rQ, long flags)
{
	int    ok = -1;
	uint   pos = 0;
	PPQuotItem_ item(rQ);
	if(lsearch(&item, &pos, PTR_CMPFUNC(PPQuotItem_))) {
		PPQuotItem_ & r_ex_item = at(pos);
		if(flags == 0) {
			r_ex_item = item;
			ok = 2;
		}
		else if(flags & rfGt) {
			if(cmp(item.Dtm, r_ex_item.Dtm) > 0) {
				r_ex_item = item;
				ok = 2;
			}
		}
		else if(flags & rfLt) {
			if(cmp(item.Dtm, r_ex_item.Dtm) < 0) {
				r_ex_item = item;
				ok = 2;
			}
		}
	}
	else {
		ok = Add(item);
	}
	return ok;
}

void SLAPI PPQuotItemArray::Sort()
{
	SVector::sort(PTR_CMPFUNC(PPQuotItem_)); // @v9.8.4 SArray-->SVector
}
//
// PPQuot
//
// static
int FASTCALL PPQuot::IsQuotAcceptableForLoc(PPID locID, const PPIDArray & rLocList)
{
	int    yes = 0;
	if(locID == 0)
		yes = 1;
	else if(!rLocList.getCount() || rLocList.lsearch(locID))
		yes = 1;
	else {
		PPObjLocation loc_obj;
		LocationTbl::Rec loc_rec;
		if(loc_obj.Fetch(locID, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WAREHOUSEGROUP) {
			for(uint i = 0; !yes && i < rLocList.getCount(); i++) {
				int r = loc_obj.IsMemberOfGroup(rLocList.get(i), locID);
				if(r > 0)
					yes = 1;
				else if(r == 0) {
					; // @todo process error
				}
			}
		}
	}
	return yes;
}

SLAPI PPQuot::PPQuot(PPID goodsID)
{
	Clear();
	GoodsID = goodsID;
}

SLAPI PPQuot::PPQuot(const QuotationTbl::Rec & src)
{
	GetFromRec(src);
}

void SLAPI PPQuot::Clear()
{
	THISZERO();
}

PPQuot & FASTCALL PPQuot::operator = (const QuotationTbl::Rec & src)
{
	GetFromRec(src);
	return *this;
}

QuotIdent & FASTCALL PPQuot::MakeIdent(QuotIdent & rQi) const
{
	QuotIdent qi(LocID, Kind, CurID, ArID);
	qi.SetIdentPeriod(Period.IsZero() ? 0 : &Period);
	qi.SetIdentQtty(&MinQtty);
	rQi = qi;
	return rQi;
}

int SLAPI PPQuot::IsEqual(const PPQuot & rS, long flags) const
{
	if(!(flags & cmpNoID) && ID != rS.ID)
		return 0;
	else {
		const long  f = (Flags & fDbMask);
		const long  sf = (rS.Flags & fDbMask);
		if(/* @v7.8.8 f != sf || */ArID != rS.ArID || GoodsID != rS.GoodsID || LocID != rS.LocID ||
			Kind != rS.Kind || CurID != rS.CurID || MinQtty != rS.MinQtty || Period != rS.Period)
			return 0;
		else
			return (flags & cmpNoVal) ? 1 : BIN(Quot == rS.Quot && f == sf);
	}
	/*
	return BIN(ArticleID == rS.ArticleID && GoodsID == rS.GoodsID &&
		Location == rS.Location && Kind == rS.Kind && CurID == rS.CurID &&
		Flags == rS.Flags && MinQtty == rS.MinQtty && Quot == rS.Quot);
	*/
}

int SLAPI PPQuot::IsRelative() const
{
	return BIN(Flags & (fPctOnCost | fPctOnPrice | fPctOnAddition | fPctOnBase));
}

int SLAPI PPQuot::IsEmpty() const
{
	return BIN(Quot == 0.0 && !(Flags & (fPctDisabled|fZero)) && !IsRelative());
}

double SLAPI PPQuot::CalcPrice(double cost, double price) const
{
	double val = 0.0;
	if(Flags & fPctOnPrice)
		val = CalcSelling(price, Quot);
	else if(Flags & fPctOnCost)
		val = CalcSelling(cost, Quot);
	else if(Flags & fPctOnAddition)
		val = RoundUpPrice(price + ((price - cost) * fdiv100r(Quot)));
	else
		val = R5(Quot); // @v7.7.0 R2-->R5
	return val;
}

void FASTCALL PPQuot::GetFromRec(const QuotationTbl::Rec & src)
{
	Clear();
	ID      = src.ID;
	GoodsID = src.GoodsID;
	LocID   = src.Location;
	Kind    = src.Kind;
	CurID   = src.CurID;
	ArID    = src.ArID;
	Flags   = src.Flags;
	MinQtty = src.MinQtty;
	Quot    = src.Quot;
}

int FASTCALL PPQuot::GetValFromStr(const char * pStr)
{
	Flags = 0;
	Quot = 0.0;

	char   temp_buf[256];
	long   flags = 0;
	temp_buf[0] = 0;
	size_t len = strlen(strip(STRNSCPY(temp_buf, pStr)));
	if(len) {
		uchar  last_sign = (uchar)ToUpper866(temp_buf[len-1]);
		if(last_sign == 'C' || last_sign == 145 /* rus 866 'С' */) {
			flags |= PPQuot::fPctOnCost;
			temp_buf[len-1] = 0;
		}
		else if(last_sign == 'P' || last_sign == 144 /* rus 866 'Р'*/) {
			flags |= PPQuot::fPctOnPrice;
			temp_buf[len-1] = 0;
		}
		else if(last_sign == 'D' || last_sign == 132 /* rus 866 'Д'*/) {
			flags |= PPQuot::fPctOnAddition;
			temp_buf[len-1] = 0;
		}
		else if(last_sign == 'X' || last_sign == 149 /* rus 866 'Х'*/) {
			flags |= PPQuot::fPctDisabled;
			temp_buf[len-1] = 0;
		}
		else if(last_sign == 'Q' || last_sign == 138 /* rus 866 'К'*/) {
			flags |= PPQuot::fPctOnBase;
			temp_buf[len-1] = 0;
		}
		else if(last_sign == 'F') {
			flags |= PPQuot::fWithoutTaxes;
			temp_buf[len-1] = 0;
		}
		{
			double val = 0.0;
			if(sstreqi_ascii(temp_buf, "zero") || sstreqi_ascii(temp_buf, "z")) {
				flags |= PPQuot::fZero;
				val = 0.0;
			}
			else
				strtodoub(temp_buf, &val);
			Flags |= flags;
			Quot = val;
		}
	}
	return 1;
}

//static
SString & PPQuot::PutValToStr(double val, long flags, SString & rBuf)
{
	rBuf.Z();
	if(flags & PPQuot::fZero)
		rBuf.CatChar('Z');
	else {
		long   fmt = (flags & (fPctOnCost|fPctOnPrice|fPctOnAddition|fPctOnBase)) ?
			MKSFMTD(0, 2, NMBF_NOTRAILZ|NMBF_NOZERO) : MKSFMTD(0, 5, NMBF_NOZERO); // @v9.5.1 MKSFMTD(0, 2, NMBF_NOZERO)-->MKSFMTD(0, 5, NMBF_NOZERO)
		rBuf.Cat(val, fmt);
	}
	if(flags) {
		if(flags & PPQuot::fPctOnPrice)         rBuf.CatChar('P');
		else if(flags & PPQuot::fPctOnCost)     rBuf.CatChar('C');
		else if(flags & PPQuot::fPctOnAddition) rBuf.CatChar('D');
		else if(flags & PPQuot::fPctDisabled)   rBuf.CatChar('X');
		else if(flags & PPQuot::fPctOnBase)     rBuf.CatChar('Q');
		else if(flags & PPQuot::fWithoutTaxes)  rBuf.CatChar('F');
	}
	return rBuf;
}

//static
SString & PPQuot::PutValToStr(double val, long flags, const DateRange & rPeriod, double minQtty, SString & rBuf)
{
	PPQuot::PutValToStr(val, flags, rBuf);
	if(rBuf.NotEmpty() && (!rPeriod.IsZero() || minQtty > 0.0)) {
		rBuf.CatChar('(').Cat(rPeriod, 1).CatDiv(',', 2).Cat(minQtty).CatChar(')');
	}
	return rBuf;
}

SString & FASTCALL PPQuot::PutValToStr(SString & rBuf) const
{
	return PPQuot::PutValToStr(Quot, Flags, rBuf);
}

int FASTCALL PPQuot::CheckForVal(const RealRange * pRange) const
{
	if(pRange && !pRange->IsZero())
		if(!IsRelative() && !(Flags & PPQuot::fPctDisabled) && pRange->CheckVal(Quot))
			return 1;
		else
			return 0;
	else
		return 1;
}

int FASTCALL PPQuot::CheckForFilt(const QuotFilt * pFilt, const PPIDArray * pKindList) const
{
	if(!CheckFiltID(pFilt->GoodsID, GoodsID))
		return 0;
	// @v7.1.12 {
	if(pFilt->Flags & QuotFilt::fZeroArOnly) {
		if(ArID != 0)
			return 0;
	}
	else
	// } @v7.1.12
		if(!CheckFiltID(pFilt->ArID, ArID))
			return 0;
	if(!CheckFiltID(pFilt->CurID, CurID))
		return 0;
	if(pFilt->Flags & QuotFilt::fAllLocations && LocID != 0)
		return 0;
	if(!CheckFiltID(pFilt->LocID, LocID))
		return 0;
	if(!CheckFiltID(pFilt->QuotKindID, Kind))
		return 0;
	if(pKindList && pKindList->getCount() && !pKindList->lsearch(Kind))
		return 0;
	if(!CheckForVal(&pFilt->Val))
		return 0;
	if(pFilt->GoodsID) {
		if(GoodsID != pFilt->GoodsID)
			return 0;
	}
	else if(pFilt->GoodsGrpID) {
		PPObjGoods goods_obj;
		if(!goods_obj.BelongToGroup(GoodsID, pFilt->GoodsGrpID, 0))
			return 0;
	}
	return 1;
}

IMPL_CMPFUNC(PPQuot, i1, i2) { RET_CMPCASCADE4((const PPQuot*)i1, (const PPQuot*)i2, LocID, Kind, CurID, ArID); }
//
// PPQuotArray
//
SLAPI PPQuotArray::PPQuotArray(PPID goodsID) : TSVector <PPQuot>() // @v9.8.4 TSArray-->TSVector
{
	GoodsID = goodsID;
}

SLAPI PPQuotArray::PPQuotArray(const PPQuotArray & s) : TSVector <PPQuot> (s) // @v9.8.4 TSArray-->TSVector
{
	GoodsID = s.GoodsID;
}

PPQuotArray & FASTCALL PPQuotArray::operator = (const PPQuotArray & s)
{
	GoodsID = s.GoodsID;
	copy(s);
	return *this;
}

int SLAPI PPQuotArray::RemoveHangedLocations()
{
	int    ok = -1;
	PPQuot * p_quot;
	PPObjLocation loc_obj;
	for(uint i = 0; enumItems(&i, (void **)&p_quot);)
		if(p_quot->LocID && loc_obj.Search(p_quot->LocID) <= 0) {
			atFree(--i);
			ok = 1;
		}
	return ok;
}

int SLAPI PPQuotArray::SetManagedLocList(const PPIDArray * pList)
{
	if(pList)
		ManagedLocList = *pList;
	else
		ManagedLocList.freeAll();
	return 1;
}

int FASTCALL PPQuotArray::CanUpdateQuot(const PPQuot & rQuot) const
{
	//
	// @attention
	// До @v6.0.8 функция возвращала 0 если rQuot.Location == 0.
	// Теперь в этом случае будет допускаться изменение котировки.
	// Необходимо наблюдать за поведением синхронизации по котировкам.
	//
	return PPQuot::IsQuotAcceptableForLoc(rQuot.LocID, ManagedLocList);
}

int SLAPI PPQuotArray::SetQuot(const QuotIdent & rQi, double v, long flags, long minQtty, const DateRange * pPeriod)
{
	int    ret = 0;
	uint   pos = 0;
	QuotIdent temp_qi(rQi);
	temp_qi.SetIdentPeriod(pPeriod);
	temp_qi.SetIdentQtty(&minQtty); // @v7.6.7
	if(SearchQi(temp_qi, &pos)) {
		PPQuot & r_quot = at(pos);
		r_quot.Quot    = R6(v);
		r_quot.Flags   = flags;
		if(minQtty >= 0)
			r_quot.MinQtty = minQtty;
		r_quot.Period.Set(pPeriod);
		ret = pos+1;
	}
	else {
		PPQuot quot(GoodsID);
		quot.LocID   = rQi.LocID;
		quot.Kind    = rQi.QuotKindID;
		quot.CurID   = rQi.CurID;
		quot.ArID    = rQi.ArID;
		quot.Quot    = R6(v);
		quot.Flags   = flags;
		quot.MinQtty = (minQtty < 0) ? 0 : minQtty;
		quot.Period.Set(pPeriod);
		if(insert(&quot))
			ret = getCount();
	}
	return ret;
}

int FASTCALL PPQuotArray::SearchQiList(const QuotIdent & rQi, LongArray & rPosList) const
{
	rPosList.clear();

	int    ok = 0;
	uint   pos = 0;
	const  uint c = getCount();
	for(uint i = 0; i < c; i++) {
		const PPQuot & rq = at(i);
		if(rq.Kind == rQi.QuotKindID) {
			if(rq.LocID == rQi.LocID && rq.CurID == rQi.CurID && rq.ArID == rQi.ArID && (!rq.IsEmpty() || (rq.Flags & PPQuot::fPctDisabled))) {
				if((!(rQi.Flags & QuotIdent::fUsePeriod) || rq.Period == rQi.Period) && (!(rQi.Flags & QuotIdent::fUseQtty) || rq.MinQtty == (long)rQi.Qtty_)) {
					const double q_min_qtty = (double)rq.MinQtty;
					const double qtty_diff = rQi.Qtty_ - q_min_qtty;
					if(qtty_diff >= 0.0) {
						if(rQi.Dt) {
							if(rq.Period.CheckDate(rQi.Dt)) {
								rPosList.add((long)i);
								ok = 1;
							}
						}
						else {
							rPosList.add((long)i);
							ok = 1;
						}
					}
				}
			}
		}
	}
	return ok;
}

int FASTCALL PPQuotArray::SearchQi(const QuotIdent & rQi, uint * pPos) const
{
	int    ok = 0;
	uint   pos = 0;
	const  uint c = getCount();
	long   min_period_len = MAXLONG;
	double min_qtty_diff = SMathConst::Max;
	for(uint i = 0; i < c; i++) {
		const PPQuot & rq = at(i);
		if(rq.Kind == rQi.QuotKindID) {
			if(rq.LocID == rQi.LocID && rq.CurID == rQi.CurID && rq.ArID == rQi.ArID && (!rq.IsEmpty() || (rq.Flags & PPQuot::fPctDisabled))) {
				if((!(rQi.Flags & QuotIdent::fUsePeriod) || rq.Period == rQi.Period) && (!(rQi.Flags & QuotIdent::fUseQtty) || rq.MinQtty == (long)rQi.Qtty_)) {
					const double q_min_qtty = (double)rq.MinQtty;
					const double qtty_diff = rQi.Qtty_ - q_min_qtty;
					if(qtty_diff >= 0.0) {
						if(rQi.Dt) {
							//
							// При необходимости проверки котировки на дату, искомой является та котировка,
							// у которой период соответствует дате и является наиболее узким из всех остальных
							// котировок. "Узость" периода определяется функцией DateRange::GetLength().
							//
							if(rq.Period.CheckDate(rQi.Dt)) {
								long rq_len = rq.Period.GetLength();
								if(rq_len <= min_period_len && qtty_diff <= min_qtty_diff) { // @v7.6.7 (<)--(<=)
									min_period_len = rq_len;
									min_qtty_diff = qtty_diff;
									pos = i;
									ok = 1;
								}
							}
						}
						else {
							//
							// Если нет необходимости проверять адекватность котировки на дату, следует
							// выбрать котировку с минимальной разницей между запрашиваемым количеством и
							// ограничивающей величиной количества товара в котировке.
							//
							if(qtty_diff <= min_qtty_diff) {
								min_qtty_diff = qtty_diff;
								pos = i;
								ok = 1;
							}
						}
					}
				}
			}
		}
	}
	if(ok)
		ASSIGN_PTR(pPos, pos);
	return ok;
}

int SLAPI PPQuotArray::GetQuot(const QuotIdent & rIdent, PPQuot * pQuot) const
{
	int    ok = 0;
	uint   pos = 0;
	if(SearchQi(rIdent, &pos)) {
		ASSIGN_PTR(pQuot, at(pos));
		ok = 1;
	}
	else if(pQuot) {
		pQuot->Clear();
		pQuot->GoodsID = GoodsID;
		pQuot->LocID   = rIdent.LocID;
		pQuot->Kind    = rIdent.QuotKindID;
		pQuot->CurID   = rIdent.CurID;
		pQuot->ArID    = rIdent.ArID;
	}
	return ok;
}

int SLAPI PPQuotArray::SearchNearest(const QuotIdent & rQi, uint * pPos) const
{
	QuotIdent temp = rQi;
	SETIFZ(temp.Dt, getcurdate_());
	int    r = SearchQi(temp, pPos);
	if(!r && (rQi.LocID || rQi.ArID)) {
		if(rQi.ArID) {
			temp.ArID = 0;
			r = SearchQi(temp, pPos);
			temp.ArID = rQi.ArID;
		}
		if(!r && rQi.LocID) {
			PPID   loc_id = rQi.LocID;
			PPObjLocation loc_obj;
			LocationTbl::Rec loc_rec;
			do {
				int    r2 = loc_obj.Fetch(loc_id, &loc_rec);
				loc_id = (r2 > 0) ? loc_rec.ParentID : 0;
				temp.LocID = loc_id;
				r = SearchQi(temp, pPos);
				if(!r && rQi.ArID) {
					temp.ArID = 0;
					r = SearchQi(temp, pPos);
					temp.ArID = rQi.ArID;
				}
				temp.LocID = rQi.LocID;
			} while(!r && loc_id);
		}
	}
	return r;
}

int SLAPI PPQuotArray::GetResult(const QuotIdent & rIdent, double cost, double price, double * pResult) const
{
	int    ok = -1;
	uint   pos = 0;
	double q = 0.0;
	if(SearchNearest(rIdent, &pos) > 0) {
		const PPQuot & r_quot = at(pos);
		if(r_quot.Flags & PPQuot::fPctOnBase) {
			if(r_quot.Kind != PPQUOTK_BASE) {
				if(rIdent.GetPrevBase() > 0.0) {
					q = CalcSelling(rIdent.GetPrevBase(), r_quot.Quot);
					ok = 1;
				}
				else {
					QuotIdent qi = rIdent;
					qi.QuotKindID = PPQUOTK_BASE;
					uint   p_ = 0;
					if(SearchNearest(qi, &p_) > 0) {
						q = CalcSelling(at(p_).CalcPrice(cost, price), r_quot.Quot);
						ok = 1;
					}
				}
			}
			else {
				// Error "base on base"
			}
		}
		else {
			q = r_quot.CalcPrice(cost, price);
			ok = (r_quot.Flags & PPQuot::fPctDisabled) ? 2 : 1;
		}
	}
	ASSIGN_PTR(pResult, q);
	return ok;
}

void SLAPI PPQuotArray::Sort()
{
	SVector::sort(PTR_CMPFUNC(PPQuot)); // @v9.8.4 SArray-->SVector
}

int SLAPI PPQuotArray::IsDisabled(const QuotIdent & rIdent, const PPIDArray * pQKList, PPQuotArray * pParentQuots)
{
	int    r = 0, not_found = 0;
	uint   pos = 0;
	if(SearchNearest(rIdent, &pos) > 0) {
		PPQuot * p_quot = &at(pos);
		r = (p_quot->Flags & PPQuot::fPctDisabled) ? 1 : 0;
		for(uint i = 0; !r && enumItems(&i, (void**)&p_quot);) {
			pos = 0;
			if((p_quot->Flags & PPQuot::fPctDisabled) && rIdent.CurID == p_quot->CurID &&
				p_quot->Kind == rIdent.QuotKindID && (!pQKList || pQKList->lsearch(p_quot->Kind, &pos))) {
				if(!p_quot->ArID && rIdent.ArID)
					continue;
				r = (!rIdent.ArID && p_quot->ArID)  ? 1 : 0;
				r = (rIdent.LocID == 0 && p_quot->LocID != 0) ? 1 : r;
			}
		}
	}
	else
		not_found = 1;
	return (not_found && pParentQuots) ? pParentQuots->IsDisabled(rIdent, pQKList) : r;
}

int SLAPI PPQuotArray::Correct721(PPLogger * pLogger)
{
	int    ok = -1;
	uint   i;
	Sort();
	LongArray to_remove_list;
	const uint c = getCount();
	for(i = 0; i < c; i++) {
		PPQuot & r_q = at(i);
		for(uint j = i+1; j < c; j++) {
			PPQuot & r_q2 = at(j);
			if(r_q2.IsEqual(r_q, PPQuot::cmpNoID|PPQuot::cmpNoVal)) {
				if(r_q2.ID < r_q.ID)
					to_remove_list.addUnique((long)j);
				else
					to_remove_list.addUnique((long)i);
			}
		}
	}
	{
		to_remove_list.sort();
		i = to_remove_list.getCount();
		if(i) do {
			const uint qp = (uint)to_remove_list.get(--i);
			if(pLogger) {
				const PPQuot & r_q = at(qp);
				SString msg_buf;
				// Дублируемое значение котировки [@int '@quotkind' '@goods' @hex @real] - [@int @hex @real]
				PPFormatT(PPTXT_INVQUOTDUP, &msg_buf, r_q.ID, r_q.Kind, r_q.GoodsID, r_q.Flags, r_q.Quot, r_q.ID, r_q.Flags, r_q.Quot);
				pLogger->Log(msg_buf);
			}
			atFree(qp);
			ok = 1;
		} while(i);
	}
	return ok;
}

int SLAPI PPQuotArray::DebugLog() const
{
#ifndef NDEBUG // {
	SString temp_buf, quot_buf;
	temp_buf.Cat("QUOTARRAY").CatDiv(':', 2).Cat(GoodsID).CR();
	for(uint i = 0; i < getCount(); i++) {
		const PPQuot & r_q = at(i);
		temp_buf.Tab().Cat(r_q.Kind).CatDiv(';', 2).Cat(r_q.GoodsID).CatDiv(';', 2).Cat(r_q.LocID).
			CatDiv(';', 2).Cat(r_q.ArID).CatDiv(';', 2).Cat(r_q.CurID).CatDiv(';', 2).Cat(r_q.PutValToStr(quot_buf = 0)).CR();
	}
	temp_buf.CR();
	PPLogMessage(PPFILNAM_DEBUG_LOG, temp_buf, 0);
#endif // } !NDEBUG
	return 1;
}

static int SLAPI LogDeclineMsg(const PPQuot * pQuot, int quotCat)
{
	uint   msg_id = 0;
	SString fmt_buf, msg_buf, quot_buf;
	switch(quotCat) {
		case PPQC_SUPPLDEAL: msg_id = PPTXT_QUOTSDUPD_RTDECLINED; break;
		case PPQC_MATRIX: msg_id = PPTXT_QUOTMTXUPD_RTDECLINED; break;
		case PPQC_MATRIXRESTR: msg_id = PPTXT_QUOTMTXRUPD_RTDECLINED; break;
		//case PPQC_PRICE:
		default: msg_id = PPTXT_QUOTUPD_RTDECLINED; break;
	}
	PPObjQuotKind qk_obj;
	qk_obj.MakeCodeString(pQuot, quot_buf);
	msg_buf.Printf(PPLoadTextS(msg_id, fmt_buf), quot_buf.cptr());
	PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
	return -2;
}
//
// QuotationCore
//
SLAPI QuotationCore::QuotationCore() : QuotationTbl()
{
}

int SLAPI QuotationCore::Search(PPID id, void * b)
{
	return SearchByID(this, PPOBJ_QUOT, id, b);
}

int SLAPI QuotationCore::SetCurr(PPID * pID, const PPQuot * pQuot, int logSj, int use_ta)
{
	int    ok = -1;
	int    is_mtx_upd = 0, is_mtx_restr_upd = 0, is_suppl_deal_upd = 0;
	PPID   goods_id = 0; // для записи в SysJournal
	PPID   loc_id = 0;   // для записи в SysJournal
	PPID   ar_id = 0;    // для записи в SysJournal
	PPID   i = 0;
	QuotationTbl::Rec rec;
	PPObjGoods gobj;
	PPObjQuotKind qk_obj; // Для проверки прав доступа
	PPObjQuotKind::Special spc;
	PPObjQuotKind::GetSpecialKinds(&spc, 1);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pID && *pID) {
			THROW(Search(*pID, &rec) > 0);
			goods_id = rec.GoodsID;
			loc_id   = rec.Location;
			ar_id    = rec.ArID;
			if(pQuot && pQuot->GoodsID == rec.GoodsID && pQuot->LocID == rec.Location &&
				pQuot->Kind == rec.Kind && (pQuot->Flags & PPQuot::fDbMask) == (rec.Flags & PPQuot::fDbMask) &&
				pQuot->ArID == rec.ArID && pQuot->Quot == rec.Quot && pQuot->MinQtty == rec.MinQtty) {
				pQuot = 0;
			}
			else {
				if(rec.Kind == 0)
					ok = 1;
				else if(rec.Kind == spc.MtxID) {
					ok = qk_obj.CheckRights(QUOTRT_UPDMTX) ? (is_mtx_upd = 1) : LogDeclineMsg(pQuot, PPQC_MATRIX);
					//
					// Если в матрицу вводится новый элемент и существуют ограничения на матрицу,
					// то проверяем эти ограничения //
					// При выводе элементов из матрицы ограничения не проверяем.
					//
					if(spc.MtxRestrID && pQuot && pQuot->Quot > 0.0 && rec.Quot <= 0.0)
						THROW(gobj.CheckMatrixRestrict(pQuot->GoodsID, pQuot->LocID, (long)pQuot->Quot));
				}
				else if(rec.Kind == spc.MtxRestrID)
					ok = qk_obj.CheckRights(QUOTRT_UPDMTXRESTR) ? (is_mtx_restr_upd = 1) : LogDeclineMsg(pQuot, PPQC_MATRIXRESTR);
				else if(spc.IsSupplDealKind(rec.Kind)) {
					ok = qk_obj.CheckRights(QUOTRT_UPDSUPPLCOST) ? 1 : LogDeclineMsg(pQuot, PPQC_SUPPLDEAL);
					// @v7.1.10 {
					if(rec.Kind == spc.SupplDealID)
						is_suppl_deal_upd = 1;
					// } @v7.1.10
				}
				else
					ok = qk_obj.CheckRights(QUOTRT_UPDQUOTS) ? 1 : LogDeclineMsg(pQuot, PPQC_PRICE);
				if(ok > 0) {
					THROW(RemoveByID(this, *pID, 0)); // @v7.9.0
					// @v7.9.0 THROW_DB(deleteRec());
				}
				else
					pQuot = 0;
			}
		}
		else if(spc.MtxRestrID && pQuot && spc.MtxID && spc.MtxID == pQuot->Kind && pQuot->Quot > 0.0) {
			THROW(gobj.CheckMatrixRestrict(pQuot->GoodsID, pQuot->LocID, (long)pQuot->Quot));
		}
		if(pQuot && (pQuot->Quot != 0.0 || (pQuot->Flags & (PPQuot::fPctDisabled|PPQuot::fZero)) || pQuot->IsRelative())) {
			MEMSZERO(rec);
			rec.Dt = getcurdate_();
			THROW(IncDateKey(this, 1, rec.Dt, &rec.OprNo));
			rec.GoodsID   = pQuot->GoodsID;
			rec.Location  = pQuot->LocID;
			rec.Kind      = pQuot->Kind;
			rec.CurID     = pQuot->CurID;
			rec.Flags     = pQuot->Flags;
			rec.Flags     = (pQuot->Flags & PPQuot::fDbMask); // @v7.2.1 (& PPQuot::fDbMask)
			rec.ArID      = pQuot->ArID;
			rec.Quot      = pQuot->Quot;
			rec.MinQtty   = pQuot->MinQtty;
			rec.Actual    = 1;
			goods_id = rec.GoodsID;
			loc_id   = rec.Location;
			ar_id    = rec.ArID;
			if(rec.Kind)
				if(rec.Kind == spc.MtxID)
					ok = qk_obj.CheckRights(QUOTRT_UPDMTX) ? (is_mtx_upd = 1) : LogDeclineMsg(pQuot, PPQC_MATRIX);
				else if(rec.Kind == spc.MtxRestrID)
					ok = qk_obj.CheckRights(QUOTRT_UPDMTXRESTR) ? (is_mtx_restr_upd = 1) : LogDeclineMsg(pQuot, PPQC_MATRIXRESTR);
				else if(spc.IsSupplDealKind(rec.Kind)) {
					ok = qk_obj.CheckRights(QUOTRT_UPDSUPPLCOST) ? 1 : LogDeclineMsg(pQuot, PPQC_SUPPLDEAL);
					// @v7.1.10 {
					if(rec.Kind == spc.SupplDealID)
						is_suppl_deal_upd = 1;
					// } @v7.1.10
				}
				else
					ok = qk_obj.CheckRights(QUOTRT_UPDQUOTS) ? 1 : LogDeclineMsg(pQuot, PPQC_PRICE);
			else
				ok = 1;
			if(ok > 0) {
				copyBufFrom(&rec);
				THROW_DB(insertRec(0, &i));
				ASSIGN_PTR(pID, i);
			}
		}
		if(ok > 0) {
			if(logSj) {
				DS.LogAction(PPACN_GOODSQUOTUPD, PPOBJ_GOODS, goods_id, 0, 0);
				if(is_mtx_upd)
					DS.LogAction(PPACN_GOODSMTXUPD, PPOBJ_GOODS, goods_id, loc_id, 0);
				else if(is_mtx_restr_upd)
					DS.LogAction(PPACN_GOODSMTXRESTRUPD, PPOBJ_GOODS, goods_id, loc_id, 0);
				// @v7.1.10 {
				else if(is_suppl_deal_upd)
					DS.LogAction(PPACN_SUPPLDEALUPD, PPOBJ_GOODS, goods_id, ar_id, 0);
				// } @v7.1.10
			}
			if(is_mtx_upd)
				ok = 1000;
			else if(is_mtx_restr_upd)
				ok = 2000;
			else if(is_suppl_deal_upd)
				ok = 3000;
		}
		THROW(tra.Commit());
	}
	if(ok > 0) {
		DirtyGoodsID(goods_id);
		if(is_mtx_upd) {
			PPIDArray loc_list, goods_list;
			goods_list.add(goods_id);
			loc_list.add(loc_id);
			const int deferred = DS.IsDbCacheDeferredState(DBS.GetDbPathID());
			DirtyMatrix(&goods_list, &loc_list, deferred);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI QuotationCore::SetCurrList(PPQuotArray & rQuotList, const PPQuotArray * pTemplate, int noRmv, int use_ta)
{
	assert(!pTemplate || pTemplate->GoodsID == rQuotList.GoodsID);
	int    ok = 1, r = 0;
	int    is_updated = 0, is_mtx_updated = 0, is_mtx_restr_updated = 0, is_suppl_deal_upd = 0;
	PPIDArray mtx_loc_list, mtx_restr_loc_list, suppl_deal_ar_list;
	uint   i, pos;
	int    unupd = 0; // @debug
	PPQuot * p_quot;
	PPQuotArray templ(rQuotList.GoodsID);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pTemplate) {
			templ = *pTemplate;
		}
		else {
			GetCurrList(rQuotList.GoodsID, 0, 0, templ);
		}
		for(i = 0; rQuotList.enumItems(&i, (void**)&p_quot);) {
			p_quot->Flags &= ~PPQuot::fDbUpd; // @v7.2.1
			if((p_quot->Flags & (PPQuot::fPctDisabled|PPQuot::fZero)) || R2(p_quot->Quot) != 0.0) {
				if(rQuotList.CanUpdateQuot(*p_quot) > 0) {
					p_quot->GoodsID = rQuotList.GoodsID;
					const PPID loc_id = p_quot->LocID;
					const PPID ar_id = p_quot->ArID;
					QuotIdent qi(loc_id, p_quot->Kind, p_quot->CurID, p_quot->ArID);
					if(templ.SearchQi(qi, &(pos = 0)) > 0) {
						{
							PPQuot & r_templ_q = templ.at(pos);
							THROW(r = SetCurr(&r_templ_q.ID, p_quot, 0, 0));
						}
						templ.atFree(pos);
					}
					else {
						THROW(r = SetCurr(&p_quot->ID, p_quot, 0, 0));
					}
					if(r > 0) {
						p_quot->Flags |= PPQuot::fDbUpd; // @v7.2.1
						is_updated = 1;
						if(r == 1000) {
							mtx_loc_list.addUnique(loc_id);
							is_mtx_updated = 1;
						}
						else if(r == 2000) {
							mtx_restr_loc_list.addUnique(loc_id);
							is_mtx_restr_updated = 1;
						}
						else if(r == 3000) {
							suppl_deal_ar_list.addUnique(ar_id);
							is_suppl_deal_upd = 1;
						}
					}
				}
			}
		}
		if(!noRmv) {
			for(i = 0; i < templ.getCount(); i++) {
				PPQuot & r_quot = templ.at(i);
				if(rQuotList.CanUpdateQuot(r_quot) > 0) {
					THROW(r = SetCurr(&r_quot.ID, 0, 0, 0));
					if(r > 0) {
						is_updated = 1;
						if(r == 1000) { // @v8.2.5 @fix (r = 1000)-->(r == 1000)
							mtx_loc_list.addUnique(r_quot.LocID);
							is_mtx_updated = 1;
						}
						else if(r == 2000) {
							mtx_restr_loc_list.addUnique(r_quot.LocID);
							is_mtx_restr_updated = 1;
						}
						else if(r == 3000) {
							suppl_deal_ar_list.addUnique(r_quot.ArID);
							is_suppl_deal_upd = 1;
						}
					}
				}
			}
		}
		if(is_updated) {
			DS.LogAction(PPACN_GOODSQUOTUPD, PPOBJ_GOODS, rQuotList.GoodsID, 0, 0);
			if(is_mtx_updated) {
				uint c = mtx_loc_list.getCount();
				if(c) {
					do {
						DS.LogAction(PPACN_GOODSMTXUPD, PPOBJ_GOODS, rQuotList.GoodsID, mtx_loc_list.at(--c), 0);
					} while(c);
				}
				else
					DS.LogAction(PPACN_GOODSMTXUPD, PPOBJ_GOODS, rQuotList.GoodsID, -1, 0);
			}
			if(is_mtx_restr_updated) {
				uint c = mtx_restr_loc_list.getCount();
				if(c) {
					do {
						DS.LogAction(PPACN_GOODSMTXRESTRUPD, PPOBJ_GOODS, rQuotList.GoodsID, mtx_restr_loc_list.at(--c), 0);
					} while(c);
				}
				else
					DS.LogAction(PPACN_GOODSMTXRESTRUPD, PPOBJ_GOODS, rQuotList.GoodsID, -1, 0);
			}
			if(is_suppl_deal_upd) {
				uint c = suppl_deal_ar_list.getCount();
				if(c) {
					do {
						DS.LogAction(PPACN_SUPPLDEALUPD, PPOBJ_GOODS, rQuotList.GoodsID, suppl_deal_ar_list.at(--c), 0);
					} while(c);
				}
				else
					DS.LogAction(PPACN_SUPPLDEALUPD, PPOBJ_GOODS, rQuotList.GoodsID, -1, 0);
			}
		}
		THROW(tra.Commit());
	}
	if(is_updated)
		ok = 2;
	//
	// Note: Актуализацию кэша сделает функция SetCurr()
	//
	CATCHZOK
	return ok;
}

int SLAPI QuotationCore::GetCurrList(PPID goodsID, PPID quotKindID, PPID locID, PPQuotArray & rQuotList)
{
	int    ok = 1;
	QuotationTbl::Key2 k, k_;
	PPQuot    quot;
	k.GoodsID = goodsID;
	k.Actual  = 1;
	k.Dt      = ZERODATE;
	k.OprNo   = 0;
	k_ = k;
	PROFILE_START
	//
	// Предварительная проверка на существование котировок по товару дает выигрыш
	// в скорости выполнения.
	// На тестовых данных с большим количеством товаров ~70000 и небольшим количеством
	// котировок (несколько сотен) ускорение составило 33.7%
	//
	if(search(2, &k_, spGe) && k_.GoodsID == goodsID && k_.Actual) {
		BExtQuery q(this, 2);
		DBQ     * dbq = & (GoodsID == goodsID && Actual == 1L);
		dbq = ppcheckfiltid(dbq, this->Location, locID);
		dbq = ppcheckfiltid(dbq, this->Kind, quotKindID);
		q.select(ID, GoodsID, Location, Kind, CurID, Flags, ArID, Quot, MinQtty, 0L).where(*dbq);
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;)
			rQuotList.insert(&(quot = data));
	}
	PROFILE_END
	rQuotList.GoodsID = goodsID;
	rQuotList.Sort();
	return ok;
}

int SLAPI QuotationCore::ReplaceArticleRefs(PPID replacedID, PPID newID, int use_ta)
{
	int    ok = 1;
	uint   i = 0;
	PPIDArray list;
	QuotationTbl::Key0 k0;
	BExtQuery q(this, 0, 128);
	q.select(this->ID, 0L).where(this->ArID == replacedID);
	for(q.initIteration(0, memzero(&k0, sizeof(k0)), spFirst); q.nextIteration() > 0;)
		THROW(list.add(data.ID));
	if(list.getCount()) {
		PPTransaction tra(use_ta);
		THROW(tra);
		for(i = 0; i < list.getCount(); i++) {
			if(Search(list.at(i), 0) > 0) {
				data.ArID = newID;
				THROW_DB(updateRec());
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI QuotationCore::GetMatrixListByLoc(PPID mtxQkID, PPID locID, PPIDArray & rList, PPIDArray & rNegList)
{
	int    ok = 1;
	QuotationTbl::Key0 k0;
	BExtQuery q(this, 0, 256); // @v7.1.4 (256)
	q.select(this->GoodsID, this->Quot, 0).
		where(this->Kind == mtxQkID && this->Actual > 0L && this->Location == locID);
	MEMSZERO(k0);
	for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;) {
		int    r = 1;
		if(data.Quot > 0.0)
			r = rList.add(data.GoodsID);
		else if(data.Quot < 0.0)
			r = rNegList.add(data.GoodsID);
		THROW(r);
	}
	rList.sort();
	rNegList.sort();
	CATCHZOK
	return ok;
}
/*
	В матрице может быть товар или обыкновенная группа.
	Товар или группа может принадлежать матрице с признаком "Да" или "Нет",
	либо не принадлежать (Не определен).
	-----------------------------------------------------------------
	Следующая таблица задает результат принадлежности товара матрице в зависимости
	от номинальных значений котировок самого товара и его группы //

                        : Группа : Группа : Группа
                        : Да     : Нет    : Не определена
	-----------------------------------------------------
	Товар  Да           : 1        1        1
	Товар  Нет          : 0        0        0
	Товар  Не определен : 1        0        0

	Правило Да-Нет приоритета: Если каким либо образом один идентификатор имеет значения Да и Нет,
	  то приоритет отдается значению Да. Это правило обусловлено тем, что лишнее значение в списке
	  не так критично как отсутствие нужного значения.
*/
static int SLAPI Helper_GetMatrix(int quotProviderGeneration, void * pQuotProvider, PPID mtxQkID, PPID locID, PPIDArray * pList)
{
	int    ok = 1;
	PPIDArray final_list;
	assert(oneof2(quotProviderGeneration, 1, 2));
	assert(pQuotProvider);
	if(mtxQkID && pQuotProvider) {
		PPUserFuncProfiler ufp(PPUPRF_GETMTXLISTBYLOC); // @v8.1.3
		uint   i;
		PPObjGoods goods_obj;
		const  int ignore_folder_mtx = BIN(goods_obj.GetConfig().Flags & GCF_IGNOREFOLDERMATRIX);
		PPIDArray grp_list, terminal_grp_list;
		PPIDArray list, neg_list;
		PPIDArray inner_list;
		if(quotProviderGeneration == 2) {
			THROW(((Quotation2Core *)pQuotProvider)->GetMatrixListByLoc(mtxQkID, locID, list, neg_list));
		}
		else {
			THROW(((QuotationCore *)pQuotProvider)->GetMatrixListByLoc(mtxQkID, locID, list, neg_list));
		}
		//
		// Обработка иерархических складов {
		//
		if(locID) {
			PPID   loc_id = locID;
			PPObjLocation loc_obj;
			LocationTbl::Rec loc_rec;
			PPIDArray _list, _neg_list;
			do {
				int    r2 = loc_obj.Fetch(loc_id, &loc_rec);
				loc_id = (r2 > 0) ? loc_rec.ParentID : 0;
				_list.clear();
				_neg_list.clear();
				if(quotProviderGeneration == 2) {
					THROW(((Quotation2Core *)pQuotProvider)->GetMatrixListByLoc(mtxQkID, loc_id, _list, _neg_list));
				}
				else {
					THROW(((QuotationCore *)pQuotProvider)->GetMatrixListByLoc(mtxQkID, loc_id, _list, _neg_list));
				}
				//
				// Так как котировка по складу имеет больший приоритет, чем по родительскому складу, то
				// добавляем в loc_list только те элементы по родительскому складу, которые не числятся //
				// среди "определенно Нет" для указанного склада.
				//
				list.addExclusive(&_list, &neg_list); // @v8.0.4 addUniqueExclusive-->addExclusive
				//
				// Так как котировка по складу имеет больший приоритет, чем по родительскому складу, то
				// добавляем в list только те элементы по родительскому складу, которые не числятся //
				// среди "определенно Нет" для указанного склада.
				//
				// list уже включает в себя "определенно Да" для заданного склада и элементы "без склада".
				// В соответствии с правилом "Да-Нет приоритета" такая операция логична.
				//
				neg_list.addExclusive(&_neg_list, &_list); // @v8.0.4 addUniqueExclusive-->addExclusive
				//
				list.sortAndUndup();     // @v8.0.4 sort-->sortAndUndup
				neg_list.sortAndUndup(); // @v8.0.4 sort-->sortAndUndup
			} while(loc_id);
		}
		//
		// } Обработка иерархических складов
		//
		THROW(goods_obj.P_Tbl->GetGroupTerminalList(0, &terminal_grp_list, &grp_list));
		grp_list.sortAndUndup();
		terminal_grp_list.sortAndUndup();
		for(i = 0; i < list.getCount(); i++) {
			const PPID goods_id = list.get(i);
			if(grp_list.bsearch(goods_id)) {
				if(!ignore_folder_mtx || terminal_grp_list.bsearch(goods_id)) { // @v8.3.4
					inner_list.clear();
					THROW(GoodsIterator::GetListByGroup(goods_id, &inner_list));
					for(uint j = 0; j < inner_list.getCount(); j++) {
						const PPID inner_id = inner_list.get(j);
						if(!neg_list.bsearch(inner_id) && !list.bsearch(inner_id)) {
							//
							// Член группы, имеющей признак "Да" не является ни членом списка "Да",
							// ни членом списка "Нет", следовательно, он должен входить в финальный список.
							// Теоретически не возможно, но практически все может быть: если этот inner_id
							// является членом группы, входящей в neg_list, то по правилу "Да-Нет приоритета"
							// мы, все равно, поступаем правильно.
							//
							final_list.add(inner_id);
						}
						//
						// Если член группы имеет определенное значение (член list или neg_list),
						// то он попадет (не попадет) в список другим путем.
						//
					}
				}
			}
			else {
				//
				// ИД является товаром (не группа), находится в списке "определенно Да" -
				// следовательно вставляем его в финальный список.
				//
#ifndef NDEBUG
				{
					/*
					Goods2Tbl::Rec _test_rec;
					if(goods_obj.Fetch(goods_id, &_test_rec) > 0) {
						assert(_test_rec.Kind == PPGDSK_GOODS);
					}
					*/
				}
#endif
				final_list.add(goods_id);
			}
		}
		ufp.SetFactor(0, (double)final_list.getCount()); // @v8.1.3
		ufp.Commit(); // @v8.1.3
	}
	else
		ok = -1;
	CATCHZOK
	final_list.sort();
	ASSIGN_PTR(pList, final_list);
	return ok;
}

int SLAPI QuotationCore::GetMatrix(PPID mtxQkID, PPID locID, PPIDArray * pList)
{
	return Helper_GetMatrix(1, this, mtxQkID, locID, pList);
}
//
// QuotCache
//
struct QuotParamEntry {
	PPID   QkID;
	PPID   LocID;
	PPID   ArID;
	PPID   CurID;
};

class QuotCacheBase {
public:
	QuotCacheBase(int qpg) : QpList(sizeof(QuotParamEntry)), QuotProviderGeneration(qpg)
	{
		MEMSZERO(LStatData);
		AdvCookie = 0;
		HalfSize = 1024;
		PPObjGoods goods_obj;
		MtxQkID = goods_obj.GetConfig().MtxQkID;
		IgnoreFolderMtx = BIN(goods_obj.GetConfig().Flags & GCF_IGNOREFOLDERMATRIX); // @v8.3.4
		P_QuotProvider = 0;
	}
	int    SLAPI Implement_GetMatrix(int qpg, void * pProvider, ReadWriteLock & rRwl, PPID locID, PPIDArray * pResult);
	int    SLAPI Implement_DirtyMatrix(const PPIDArray * pGoodsList, PPIDArray * pMtxLocList, int deferred);
	static int FASTCALL Helper_CallDirtyMatrix(PPIDArray * pMtxGoodsList);

	struct MatrixEntry {
		PPID   LocID;
		PPIDArray Vect;
	};
protected:
	int    FASTCALL LogDirtyMatrix(PPID goodsID) const;
	int    SLAPI LogStat();
	void   FASTCALL SetQuotProvider(void * p)
	{
		P_QuotProvider = p;
	}

	const  int  QuotProviderGeneration;
	PPID   MtxQkID; // Вид котировки товарной матрицы
	int    IgnoreFolderMtx; // @v8.3.4 Если !0, то матрица по группе верхнего уровня игнорируется //
	long   AdvCookie;
	uint   HalfSize;
	void * P_QuotProvider;
	SArray QpList;
	TSCollection <MatrixEntry> Matrix;

	struct LStat {
		long   Hits;
		long   Misses;
		long   RmvItemsCount; // Количество удаленных элементов
		long   RmvItemsHits;  // Общее количество попаданий в удаленных элементах
	};
	LStat LStatData;
	PPIDArray DeferredGoodsList;
};

int SLAPI QuotCacheBase::Implement_GetMatrix(int qpg, void * pProvider, ReadWriteLock & rRwl, PPID locID, PPIDArray * pResult)
{
	int    ok = -1;
	const  PPIDArray * p_vect = 0;
	uint   pos = 0;
	{
		//rRwl.ReadLock();
		SRWLOCKER(rRwl, SReadWriteLocker::Read);
		if(MtxQkID) {
			if(Matrix.lsearch(&locID, &pos, CMPF_LONG)) {
				ASSIGN_PTR(pResult, Matrix.at(pos)->Vect);
				ok = 1;
			}
			else {
				//rRwl.Unlock();
				//rRwl.WriteLock();
				SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
				if(Matrix.lsearch(&locID, &pos, CMPF_LONG)) {
					ASSIGN_PTR(pResult, Matrix.at(pos)->Vect);
					ok = 1;
				}
				else {
					MatrixEntry * p_entry = new MatrixEntry;
					p_entry->LocID = locID;
					assert(pProvider && oneof2(qpg, 1, 2));
					if(pProvider && qpg) {
						if(qpg == 2) {
							((Quotation2Core *)pProvider)->GetMatrix(MtxQkID, locID, &p_entry->Vect);
						}
						else {
							((QuotationCore *)pProvider)->GetMatrix(MtxQkID, locID, &p_entry->Vect);
						}
						Matrix.insert(p_entry);
						ASSIGN_PTR(pResult, p_entry->Vect);
						ok = 2;
					}
				}
			}
		}
		else if(pResult)
			pResult->clear();
		//rRwl.Unlock();
	}
	return ok;
}

//static
int FASTCALL QuotCacheBase::Helper_CallDirtyMatrix(PPIDArray * pMtxGoodsList)
{
	int    ok = -1;
	if(pMtxGoodsList) {
		pMtxGoodsList->sortAndUndup();
		const uint c = pMtxGoodsList->getCount();
		if(c) {
			int    dirty_all = 0;
			PPObjGoods goods_obj;
			PPIDArray finish_list;
			Goods2Tbl::Rec goods_rec;
			const int ignore_folder_mtx = BIN(goods_obj.GetConfig().Flags & GCF_IGNOREFOLDERMATRIX);
			for(uint i = 0; !dirty_all && i < c; i++) {
				PPID   goods_id = pMtxGoodsList->get(i);
				if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
					if(goods_rec.Kind == PPGDSK_GOODS)
						finish_list.add(goods_id);
					else if(goods_rec.Kind == PPGDSK_GROUP) {
						if(!ignore_folder_mtx || !(goods_rec.Flags & GF_FOLDER)) // @v8.3.4
							dirty_all = 1;
						/*
						temp_list.clear();
						GoodsIterator::GetListByGroup(goods_id, &temp_list);
						finish_list.add(&temp_list);
						*/
					}
				}
			}
			if(dirty_all) {
				goods_obj.P_Tbl->DirtyMatrix(0, 0);
			}
			else if(finish_list.getCount()) {
				finish_list.sortAndUndup();
				goods_obj.P_Tbl->DirtyMatrix(&finish_list, 0);
			}
			ok = 1;
		}
	}
	return ok;
}

int SLAPI QuotCacheBase::Implement_DirtyMatrix(const PPIDArray * pGoodsList, PPIDArray * pMtxLocList, int deferred)
{
	const uint _threshould = 100;

	int    ok = -1;
	uint   pos;
	// @v7.1.4 (Блокировка вызывается в функции QuotationCore::DirtyMatrix()) Lock();
	if(!pGoodsList) {
		uint c = pMtxLocList ? pMtxLocList->getCount() : 0;
		if(c) {
			do {
				const PPID loc_id = pMtxLocList->get(--c);
				if(Matrix.lsearch(&loc_id, &(pos = 0), CMPF_LONG)) {
					Matrix.atFree(pos);
					ok = 1;
				}
			} while(c);
		}
		else {
			if(Matrix.getCount())
				ok = 1;
			Matrix.freeAll();
		}
		LogDirtyMatrix(0);
	}
	else if(MtxQkID && P_QuotProvider) {
		uint j;
		if(deferred) {
			if(Matrix.getCount()) {
				for(j = 0; j < pGoodsList->getCount(); j++) {
					DeferredGoodsList.add(labs(pGoodsList->get(j)));
				}
			}
		}
		else if(Matrix.getCount() && (pGoodsList->getCount() || DeferredGoodsList.getCount())) {
			PPIDArray goods_list;
			goods_list = *pGoodsList;
			for(j = 0; j < DeferredGoodsList.getCount(); j++) {
				goods_list.add(DeferredGoodsList.get(j));
			}
			goods_list.sortAndUndup();
			DeferredGoodsList.clear();
			//
			// Если количество товаров, для которых необходимо актуализировать матрицу превосходит
			// заданный предел _threshould, то дешевле просто "убить" кэш матрицы дабы потом снова заполнить.
			//
			if(goods_list.getCount() > _threshould) {
				Matrix.freeAll();
				LogDirtyMatrix(0);
				ok = 1;
			}
			else {
				// PPUserFuncProfiler ufp(PPUPRF_MTXCACHEACTUALIZE); // @v8.1.6
				// double ufp_factor = 0.0; // @v8.1.6
				PPObjGoods goods_obj;
				PPObjLocation loc_obj;
				PPIDArray loc_list;
				PPIDArray to_rmv_list;
				PPIDArray rel_list;
				LAssocArray loc_mtx_list, prev_loc_mtx_list;
				PPQuotArray ql;

				PPIDArray wh_list;
				loc_obj.GetWarehouseList(&wh_list);

				if(QuotProviderGeneration == 2) {
					QuotFilt qfilt;
					qfilt.QuotKindID = MtxQkID;
					// @v8.1.6 qfilt.Flags |= QuotFilt::fAllLocations;
					((Quotation2Core *)P_QuotProvider)->GetRelListByFilt(&qfilt, &rel_list);
					rel_list.sort();
				}
				const uint mc = Matrix.getCount();
				for(j = 0; j < goods_list.getCount(); j++) {
					PROFILE_START
					const  PPID goods_id = labs(goods_list.get(j));
					loc_list.clear();

					uint     k, n;
					StrAssocArray hier_list;
					// @v8.3.4 {
					if(IgnoreFolderMtx) {
						Goods2Tbl::Rec goods_rec;
                        if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
                        	hier_list.Add(goods_id, goods_rec.Name);
							if(goods_obj.Fetch(goods_rec.ParentID, &goods_rec) > 0) {
								hier_list.AddFast(goods_id, goods_rec.Name);
								hier_list.Reverse();
							}
                        }
					}
					else /* } @v8.3.4 */ {
						goods_obj.GetHierarchy(goods_id, &hier_list);
					}
					loc_mtx_list.clear();
					prev_loc_mtx_list.clear();
					for(k = 0; k < hier_list.getCount(); k++) {
						PPID   id = hier_list.at_WithoutParent(k).Id;
						ql.clear();
						if(QuotProviderGeneration == 2) {
							// @v8.0.9 ((Quotation2Core *)P_QuotProvider)->GetCurrList(id, MtxQkID, 0, ql);
							((Quotation2Core *)P_QuotProvider)->GetCurrListByRelList(id, rel_list, ql); // @v8.0.9
						}
						else
							((QuotationCore *)P_QuotProvider)->GetCurrList(id, MtxQkID, 0, ql);
						for(n = 0; n < ql.getCount(); n++) {
							const PPQuot & r_q = ql.at(n);
							long  par_val = 0;
							uint  pos = 0;
							prev_loc_mtx_list.Search(r_q.LocID, &par_val, &pos);
							long  val = (r_q.Quot > 0.0) ? 1 : ((r_q.Quot < 0.0) ? -1 : par_val);
							if(val) // @v7.3.0
								loc_mtx_list.Update(r_q.LocID, val, 0); // @v7.2.9 Add-->Update
							// ufp_factor += 1.0;
						}
						//
						// Унаследуем от родительской группы элементы по тем складам, которые в собственных котировках не определены
						//
						for(n = 0; n < prev_loc_mtx_list.getCount(); n++) {
							const  PPID loc_id = prev_loc_mtx_list.at(n).Key;
							long   val = 0;
							if(!loc_mtx_list.Search(loc_id, &val, 0) || val == 0) { // @v7.3.0 (|| val==0)
								loc_mtx_list.Add(loc_id, prev_loc_mtx_list.at(n).Val, 0);
							}
						}
						prev_loc_mtx_list = loc_mtx_list;
					}
					if(loc_mtx_list.getCount()) {
						to_rmv_list.clear(); // Список идентификаторов группирующих и инвалидных складов (их надо будет удалить из финального списка loc_mtx_list).
						int    zero_loc_pos = -1;
						//
						// Список loc_mtx_list содержит все значения матрицы для товара goodsID
						// с учетом иерархии товарных групп.
						//
						// Нам осталось учесть иерархию складов. Для этого не нулевое значение каждого группирующего
						// склада распространяем на его непосредственных потомков, которые не имеют собственного
						// определенного значения.
						//
						for(k = 0; k < loc_mtx_list.getCount(); k++) {
							const PPID loc_id = loc_mtx_list.at(k).Key;
							const PPID mtx_val = loc_mtx_list.at(k).Val;
							if(mtx_val != 0) { // ноль (неопределенное значение) не следует распространять на потомков.
								LocationTbl::Rec loc_rec, next_rec;
								if(loc_id == 0) {
									//
									// Нулевой склад (матрица для всех складов) обрабатываем специальным образом ниже.
									//
									zero_loc_pos = (int)k;
								}
								else if(loc_obj.Fetch(loc_id, &loc_rec) > 0) {
									if(loc_rec.Type == LOCTYP_WAREHOUSEGROUP) {
										for(SEnum en = loc_obj.P_Tbl->Enum(LOCTYP_WAREHOUSE, loc_id, 0); en.Next(&next_rec) > 0;) {
											if(oneof2(next_rec.Type, LOCTYP_WAREHOUSE, LOCTYP_WAREHOUSEGROUP)) {
												long   val = 0;
												uint   pos = 0;
												if(!loc_mtx_list.Search(next_rec.ID, &val, &pos))
													loc_mtx_list.Add(next_rec.ID, mtx_val, 0);
												else if(val == 0)
													loc_mtx_list.at(pos).Val = mtx_val;
											}
											// ufp_factor += 0.9;
										}
										to_rmv_list.add(loc_id);
									}
								}
								else
									to_rmv_list.add(loc_id);
							}
						}
						to_rmv_list.sortAndUndup();
						if(zero_loc_pos >= 0) {
							//
							// Распространяем общую матрицу (для всех складов) на те локации, для которых не определено значение
							// явно либо через родительский склад.
							//
							const PPID mtx_val = loc_mtx_list.at(zero_loc_pos).Val;
							for(k = 0; k < wh_list.getCount(); k++) {
								const PPID loc_id = wh_list.get(k);
								long   val = 0;
								uint   pos = 0;
								if(!loc_mtx_list.Search(loc_id, &val, &pos))
									loc_mtx_list.Add(loc_id, mtx_val, 0);
								else if(val == 0)
									loc_mtx_list.at(pos).Val = mtx_val;
							}
						}
						//
						// Наконец, удаляем из списка все элементы, имеющие "не матричное" значение (<=0) или относящиеся к группирующим складам
						//
						for(k = 0; k < loc_mtx_list.getCount(); k++) {
							const LAssoc & r_item = loc_mtx_list.at(k);
							if(r_item.Val > 0 && !to_rmv_list.bsearch(r_item.Key)) {
								// @v8.0.9 if(loc_list.addUnique(r_item.Key) > 0)
								if(loc_list.add(r_item.Key) > 0) // @v8.0.9
									ok = 1;
							}
						}
					}
					loc_list.sortAndUndup(); // @v8.0.9
					//
					for(uint i = 0; i < mc; i++) {
						MatrixEntry * p_entry = Matrix.at(i);
						if(p_entry) {
							if(loc_list.bsearch(p_entry->LocID)) {
								if(!p_entry->Vect.bsearch(goods_id))
									p_entry->Vect.ordInsert(goods_id, 0);
							}
							else
								p_entry->Vect.freeByKey(goods_id, 1);
						}
					}
					LogDirtyMatrix(goods_id);
					PROFILE_END
				}
				// ufp.SetFactor(1, ufp_factor); // @v8.1.6
				// ufp.Commit(); // @v8.1.6
			}
		}
	}
	// @v7.1.4 (Блокировка вызывается в функции QuotationCore::DirtyMatrix()) Unlock();
	return ok;
}

int SLAPI QuotCacheBase::LogStat()
{
	SString temp_buf;
	temp_buf.Cat(LStatData.Hits).Semicol().Cat(LStatData.Misses).Semicol().
		Cat(((double)LStatData.RmvItemsHits) / ((double)LStatData.RmvItemsCount), MKSFMTD(0, 6, 0));
	PPLogMessage(PPFILNAM_DEBUG_LOG, temp_buf, LOGMSGF_TIME/*|LOGMSGF_USER*/);
	return 1;
}

int FASTCALL QuotCacheBase::LogDirtyMatrix(PPID goodsID) const
{
	int    ok = 1;
	if(DS.CheckExtFlag(ECF_DEBUGDIRTYMTX)) {
		SString msg_buf;
		if(goodsID) {
			msg_buf.Cat(goodsID).Space().CatChar('(');
			const uint mc = Matrix.getCount();
			int   add_comma = 0;
			for(uint i = 0; i < mc; i++) {
				const MatrixEntry * p_entry = Matrix.at(i);
				if(p_entry->Vect.bsearch(goodsID)) {
					if(add_comma)
						msg_buf.CatDiv(',', 2);
					else
						add_comma = 1;
					msg_buf.Cat(p_entry->LocID);
				}
			}
			msg_buf.CatChar(')');
		}
		else {
			msg_buf = "ALL";
		}
		PPLogMessage(PPFILNAM_DIRTYMTX_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
	}
	else
		ok = -1;
	return ok;
}

class QuotCache : public ObjCache, public QuotCacheBase {
public:
	friend class QuotationCore;

	class CacheEntry {
	public:
		struct Entry {
			uint   ParamID;
			long   Flags;
			long   MinQtty;
			double Val;
		};
		void * SLAPI operator new(size_t sz, const PPQuotArray * pQuotList, QuotCache * pQuotCache)
		{
			const uint c = pQuotList->getCount();
			size_t s = sz + (pQuotList ? (sizeof(Entry) * c) : 0);
			CacheEntry * p = (CacheEntry *)new char[s];
			if(p) {
				memzero(p, s);
				p->GoodsID = pQuotList->GoodsID;
				for(uint i = 0; i < c; i++) {
					const PPQuot & r_quot = pQuotList->at(i);
					Entry & r_entry = ((Entry *)(p+1))[i];
					pQuotCache->GetQuotParamID(&r_quot, &r_entry.ParamID);
					r_entry.Flags = r_quot.Flags;
					r_entry.MinQtty = r_quot.MinQtty;
					r_entry.Val = r_quot.Quot;
				}
				p->EntryCount = (uint16)c;
			}
			return p;
		}
		int    SLAPI Get(PPQuotArray * pQuotList, PPID quotKindID, PPID locID, const QuotCache * pQuotCache)
		{
			UseCount++;
			pQuotList->GoodsID = GoodsID;
			pQuotList->freeAll();
			for(uint16 i = 0; i < EntryCount; i++) {
				PPQuot quot(GoodsID);
				const Entry & r_entry = ((Entry *)(this+1))[i];
				quot.Flags = r_entry.Flags;
				quot.MinQtty = r_entry.MinQtty;
				quot.Quot = r_entry.Val;
				pQuotCache->GetQuotParam(r_entry.ParamID, &quot);
				if((!quotKindID || quot.Kind == quotKindID) && (!locID || quot.LocID == locID))
					pQuotList->insert(&quot);
			}
			return 1;
		}
		PPID   GoodsID;
		long   UseCount;
		uint16 EntryCount;
	};
	class QuotCacheCollection : public SCollection {
	public:
		SLAPI  QuotCacheCollection() : SCollection() {}
		virtual void FASTCALL freeItem(void * pItem) { delete ((CacheEntry *)pItem); }
	};
	SLAPI  QuotCache();
	SLAPI ~QuotCache();
	void   FASTCALL SetTable(QuotationCore * pTbl)
	{
		P_Qc = pTbl;
		QuotCacheBase::SetQuotProvider(pTbl);
	}
	int    FASTCALL Put(const PPQuotArray *);
	int    SLAPI Get_(PPID goodsID, PPID quotKindID, PPID locID, PPQuotArray *); // non-virtual
	int    SLAPI GetQuotParamID(const PPQuot * pQuot, uint * pID);
	int    SLAPI GetQuotParam(uint id, PPQuot * pQuot) const;
	int    SLAPI BelongToMatrix(QuotationCore * pTbl, PPID goodsID, PPID locID);
	int    FASTCALL Dirty_(PPID goodsID); // @sync non-virtual
	int    SLAPI Clear_(); // @sync
	//int    SLAPI Lock()   { return RwL.WriteLock(); }
	//int    SLAPI Unlock() { return RwL.Unlock();    }
private:
	static int OnSysJ(int kind, const PPNotifyEvent * pEv, void * procExtPtr); // PPAdviseProc()

	virtual int  SLAPI FetchEntry(PPID id, ObjCacheEntry * pEntry, long extraData = 0)
	{
		return 0;
	}
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
	{
	}
	int    FASTCALL Search(PPID goodsID, uint * pPos) const
	{
		return P_Ary->bsearch(&goodsID, pPos, CMPF_LONG);
	}
	QuotationCore * P_Qc;
};

IMPL_CMPFUNC(GoodsQuotCacheEntry_UseCountDesc, i1, i2)
{
	// descending order
	return cmp_long(((QuotCache::CacheEntry *)i2)->UseCount, ((QuotCache::CacheEntry *)i1)->UseCount);
}

// typedef int (*PPAdviseProc)(int kind, const PPNotifyEvent * pEv, long procExtVal);
//static
int QuotCache::OnSysJ(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
{
	int    ok = -1;
	if(kind == PPAdviseBlock::evDirtyCacheBySysJ) {
		if(pEv->Action == PPACN_GOODSQUOTUPD) {
			if(oneof2(pEv->ObjType, PPOBJ_GOODS, PPOBJ_GOODSGROUP) && pEv->ObjID) {
				((QuotCache *)procExtPtr)->Dirty_(pEv->ObjID);
				ok = 1;
			}
		}
		else {
			static PPIDArray * p_mtx_goods_list = 0;
			//
			// CRITICAL_SECTION нужна чтобы защитить статический p_mtx_goods_list.
			//
			ENTER_CRITICAL_SECTION
			if(pEv->Action == PPACN_GOODSMTXUPD) {
				if(pEv->ObjID) {
					SETIFZ(p_mtx_goods_list, new PPIDArray);
					p_mtx_goods_list->add(pEv->ObjID);
				}
				ok = 1;
			}
			else if(pEv->Action == PPACN_MTXGOODSADD) {
				if(pEv->ObjID) {
					SETIFZ(p_mtx_goods_list, new PPIDArray);
					p_mtx_goods_list->add(pEv->ObjID);
				}
			}
			else if(pEv->IsFinish()) {
				ok = Helper_CallDirtyMatrix(p_mtx_goods_list);
				ZDELETE(p_mtx_goods_list);
			}
			LEAVE_CRITICAL_SECTION
		}
	}
	return ok;
}

static long _AdviseQuotCacheUpdate(long acn, PPAdviseProc proc, void * extraPtr)
{
	long   cookie = 0;
	PPAdviseBlock adv_blk;
	MEMSZERO(adv_blk);
	adv_blk.Kind = PPAdviseBlock::evDirtyCacheBySysJ;
	adv_blk.Action = acn;
	adv_blk.DbPathID = DBS.GetDbPathID();
	adv_blk.ObjType = 0;
	adv_blk.Proc = proc;
	adv_blk.ProcExtPtr = extraPtr;
	DS.Advise(&cookie, &adv_blk);
	return cookie;
}

SLAPI QuotCache::QuotCache() :
	ObjCache(PPOBJ_QUOT, sizeof(ObjCacheEntry), ObjCache::fOwnerArray), QuotCacheBase(1)
{
	P_Ary = new QuotCacheCollection();
	{
		long   cookie = 0;
		cookie = _AdviseQuotCacheUpdate(PPACN_GOODSQUOTUPD, QuotCache::OnSysJ, this);
		cookie = _AdviseQuotCacheUpdate(PPACN_GOODSMTXUPD, QuotCache::OnSysJ, this);
		cookie = _AdviseQuotCacheUpdate(PPACN_MTXGOODSADD, QuotCache::OnSysJ, this);
	}
}

SLAPI QuotCache::~QuotCache()
{
	// DS.Unadvise(AdvCookie); Приводит к зависанию при завершении сессии
	// @v6.7.1 (Приводит к зависанию при завершении сессии) LogStat();
}

int FASTCALL QuotCache::Put(const PPQuotArray * pItem)
{
	if(HalfSize) {
		if(P_Ary->getCount() >= (HalfSize * 2)) {
			P_Ary->sort(PTR_CMPFUNC(GoodsQuotCacheEntry_UseCountDesc));
			while(P_Ary->getCount() > HalfSize) {
				const uint pos = P_Ary->getCount()-1;
				LStatData.RmvItemsCount++;
				const long use_count = ((CacheEntry *)P_Ary->at(pos))->UseCount;
				LStatData.RmvItemsHits += use_count;
				P_Ary->atFree(pos);
			}
			P_Ary->sort(CMPF_LONG);
		}
	}
	CacheEntry * p_entry = new (pItem, this) CacheEntry;
	return BIN(p_entry && P_Ary->ordInsert(p_entry, 0, CMPF_LONG));
}

int SLAPI QuotCache::Get_(PPID goodsID, PPID quotKindID, PPID locID, PPQuotArray * pItem)
{
	int    ok = 1;
	uint   pos = 0;
	LStatData.Hits++;
	if(Search(goodsID, &pos)) {
		CacheEntry * p_entry = (CacheEntry *)P_Ary->at(pos);
		THROW(p_entry->Get(pItem, quotKindID, locID, this));
	}
	else {
		PPQuotArray temp_item;
		THROW(P_Qc->GetCurrList(goodsID, 0, 0, temp_item));
		THROW(Put(&temp_item));
		THROW(Get_(goodsID, quotKindID, locID, pItem)); // @recursion
		// test
#if _DEBUG // {
		{
			PPQuotArray test_item;
			THROW(Get_(goodsID, 0, 0, &test_item)); // @recursion
			assert(test_item.GoodsID == temp_item.GoodsID);
			assert(test_item.getCount() == temp_item.getCount());
			for(uint i = 0; i < test_item.getCount(); i++) {
				const PPQuot & test = test_item.at(i);
				const PPQuot & temp = temp_item.at(i);
				assert(test.ArID == temp.ArID && test.GoodsID == temp.GoodsID &&
					test.LocID == temp.LocID && test.CurID == temp.CurID &&
					test.Kind == temp.Kind && test.Flags == temp.Flags);
				assert(test.Quot == temp.Quot);
			}
		}
#endif // } _DEBUG
		//
		LStatData.Hits--;
		LStatData.Misses++;
	}
	CATCHZOK
	return ok;
}

int FASTCALL QuotCache::Dirty_(PPID goodsID)
{
	int    ok = -1;
	uint   pos = 0;
	{
		//Lock();
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		if(Search(goodsID, &pos)) {
			P_Ary->atFree(pos);
			ok = 1;
		}
		//Unlock();
	}
	return ok;
}

int SLAPI QuotCache::Clear_()
{
	int    ok = -1;
	uint   pos = 0;
	{
		//Lock();
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		CALLPTRMEMB(P_Ary, freeAll());
		ok = 1;
		//Unlock();
	}
	return ok;
}

int SLAPI QuotCache::GetQuotParamID(const PPQuot * pQuot, uint * pID)
{
	int    ok = 0;
	QuotParamEntry entry, * p_e;
	MEMSZERO(entry);
	entry.QkID  = pQuot->Kind;
	entry.LocID = pQuot->LocID;
	entry.ArID  = pQuot->ArID;
	entry.CurID = pQuot->CurID;
	for(uint i = 0; !ok && QpList.enumItems(&i, (void **)&p_e);)
		if(memcmp(p_e, &entry, sizeof(entry)) == 0) {
			ASSIGN_PTR(pID, i);
			ok = 1;
		}
	if(!ok && QpList.insert(&entry)) {
		ASSIGN_PTR(pID, QpList.getCount());
		ok = 1;
	}
	return ok;
}

int SLAPI QuotCache::GetQuotParam(uint id, PPQuot * pQuot) const
{
	if(id > 0 && id <= QpList.getCount()) {
		const QuotParamEntry * p_entry = (QuotParamEntry *)QpList.at(id-1);
		pQuot->Kind  = p_entry->QkID;
		pQuot->LocID = p_entry->LocID;
		pQuot->ArID  = p_entry->ArID;
		pQuot->CurID = p_entry->CurID;
		return 1;
	}
	else
		return 0;
}

int SLAPI QuotCache::BelongToMatrix(QuotationCore * pTbl, PPID goodsID, PPID locID)
{
	int    ok = -1;
	{
		//RwL.ReadLock();
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		if(MtxQkID) {
			const  PPIDArray * p_vect = 0;
			uint   pos = 0;
			if(Matrix.lsearch(&locID, &pos, CMPF_LONG)) {
				ok = Matrix.at(pos)->Vect.bsearch(labs(goodsID));
			}
			else {
				assert(pTbl);
				if(pTbl) {
					//RwL.Unlock();
					//RwL.WriteLock();
					SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
					if(Matrix.lsearch(&locID, &(pos = 0), CMPF_LONG)) {
						ok = Matrix.at(pos)->Vect.bsearch(labs(goodsID));
					}
					else {
						MatrixEntry * p_entry = new MatrixEntry;
						p_entry->LocID = locID;
						pTbl->GetMatrix(MtxQkID, locID, &p_entry->Vect);
						Matrix.insert(p_entry);
						ok = p_entry->Vect.bsearch(labs(goodsID));
					}
				}
			}
		}
		//RwL.Unlock();
	}
	return ok;
}

int SLAPI QuotationCore::GetMatrix(PPID locID, PPIDArray * pResult)
{
	QuotCache * p_cache = GetDbLocalCachePtr <QuotCache> (PPOBJ_QUOT);
	return p_cache ? p_cache->Implement_GetMatrix(1, this, p_cache->RwL, locID, pResult) : -1;
}

int SLAPI QuotationCore::BelongToMatrix(PPID goodsID, PPID locID)
{
	QuotCache * p_cache = GetDbLocalCachePtr <QuotCache> (PPOBJ_QUOT);
	return p_cache ? p_cache->BelongToMatrix(this, goodsID, locID) : -1;
}

int SLAPI QuotationCore::FetchList(PPID goodsID, PPQuotArray & rList)
{
	int    ok = 0;
	QuotCache * p_cache = GetDbLocalCachePtr <QuotCache> (PPOBJ_QUOT);
	if(p_cache) {
		rList.GoodsID = goodsID;
		{
			//p_cache->Lock();
			SRWLOCKER(p_cache->RwL, SReadWriteLocker::Write);
			p_cache->SetTable(this);
			const PPID qk_arg = 0;
			ok = p_cache->Get_(goodsID, qk_arg, 0, &rList);
			p_cache->SetTable(0);
			//p_cache->Unlock();
		}
	}
	else
		ok = -1;
	return ok;
}

int SLAPI QuotationCore::GetMatrixRestrict(PPID mtxRestrQkID, PPID goodsGrpID, PPID locID, int srchNearest, long * pResult)
{
	int    ok = -1, r = -1;
	long   val = 0;
	if(mtxRestrQkID) {
		double q = 0.0;
		QuotIdent qi(locID, mtxRestrQkID, 0, 0);
		PPQuot quot;
		PPQuotArray ary(goodsGrpID);
		if((r = FetchList(goodsGrpID, ary)) >= 0 || (r = GetCurrList(goodsGrpID, qi.QuotKindID, 0, ary)) > 0)
			ok = (r > 0) ? (srchNearest ? ary.GetResult(qi, 0, 0, &q) : ary.GetQuot(qi, &quot)) : 0;
		val = (long)(srchNearest ? q : quot.Quot);
	}
	ASSIGN_PTR(pResult, val);
	return ok;
}

int SLAPI QuotationCore::GetNearest(PPID goodsID, const QuotIdent & rIdent, PPQuot * pQuot, int useCache)
{
	int    ok = -1, r = -1;
	uint   pos = 0;
	PPQuotArray ary(goodsID);
	if((useCache && (r = FetchList(goodsID, ary)) >= 0) || (r = GetCurrList(goodsID, rIdent.QuotKindID, 0, ary)) > 0) {
		ok = (r > 0) ? ary.SearchNearest(rIdent, &pos) : 0;
		if(ok)
			ASSIGN_PTR(pQuot, ary.at(pos));
	}
	return ok;
}

int SLAPI QuotationCore::GetCurr(PPID goodsID, const QuotIdent & rIdent, double cost, double price, double * pQ, int useCache)
{
	int    ok = -1, r = -1;
	double q = 0.0;
	PPQuotArray ary(goodsID);
	if((useCache && (r = FetchList(goodsID, ary)) >= 0) || (r = GetCurrList(goodsID, rIdent.QuotKindID, 0, ary)) > 0)
		ok = (r > 0) ? ary.GetResult(rIdent, cost, price, &q) : 0;
	ASSIGN_PTR(pQ, q);
	return ok;
}

int SLAPI QuotationCore::DirtyGoodsID(PPID goodsID)
{
	int    ok = -1;
	QuotCache * p_cache = GetDbLocalCachePtr <QuotCache> (PPOBJ_QUOT, 0);
	if(p_cache) {
		p_cache->Dirty_(goodsID);
		ok = 1;
	}
	return ok;
}

int SLAPI QuotationCore::ClearCache()
{
	int    ok = -1;
	QuotCache * p_cache = GetDbLocalCachePtr <QuotCache> (PPOBJ_QUOT, 0);
	if(p_cache) {
		p_cache->Clear_();
		ok = 1;
	}
	return ok;
}

int SLAPI QuotationCore::DirtyMatrix(const PPIDArray * pGoodsList, PPIDArray * pMtxLocList, int deferred)
{
	int    ok = -1;
	QuotCache * p_cache = GetDbLocalCachePtr <QuotCache> (PPOBJ_QUOT, 0);
	if(p_cache) {
		//p_cache->Lock();
		SRWLOCKER(p_cache->RwL, SReadWriteLocker::Write);
		p_cache->SetTable(this);
		p_cache->Implement_DirtyMatrix(pGoodsList, pMtxLocList, deferred);
		p_cache->SetTable(0);
		//p_cache->Unlock();
		ok = 1;
	}
	return ok;
}
//
//
//
const LDATE QIISuppDate = {0x07CB0C1F}; // 31/12/1995

//static
int FASTCALL Quotation2Core::PeriodToPeriodIdx(const DateRange * pPeriod, int32 * pPeriodIdx)
{
	int    ok = 0;
	int32  idx = 0;
	if(pPeriod && !pPeriod->IsZero()) {
		const int lc = pPeriod->low.getclass();
		const int uc = pPeriod->upp.getclass();
		if(oneof2(lc, LDATE::cZero, LDATE::cNormal) && oneof2(uc, LDATE::cZero, LDATE::cNormal) && (pPeriod->upp && pPeriod->upp >= pPeriod->low)) {
			if((!pPeriod->low || pPeriod->low > QIISuppDate) && (!pPeriod->upp || pPeriod->upp > QIISuppDate)) {
				int16 lw = (int16)(pPeriod->low ? diffdate(pPeriod->low, QIISuppDate) : 0);
				int16 hw = (int16)(pPeriod->upp ? diffdate(pPeriod->upp, QIISuppDate) : 0);
				idx = MakeLong(lw, hw);
				ok = 1;
			}
		}
		if(!ok) {
			SString msg_buf;
			msg_buf.Cat(*pPeriod, 0);
			PPSetError(PPERR_INVQUOTPERIOD, msg_buf);
		}
	}
	else
		ok = 1;
	ASSIGN_PTR(pPeriodIdx, idx);
	return ok;
}

//static
int FASTCALL Quotation2Core::PeriodIdxToPeriod(int32 periodIdx, DateRange * pPeriod)
{
	int    ok = 0;
	DateRange period;
	period.SetZero();
	if(periodIdx) {
		int16 lw = LoWord(periodIdx);
		int16 hw = HiWord(periodIdx);
		if(lw)
			period.low = plusdate(QIISuppDate, lw);
		if(hw)
			period.upp = plusdate(QIISuppDate, hw);
		const int lc = period.low.getclass();
		const int uc = period.upp.getclass();
		if(oneof2(lc, LDATE::cZero, LDATE::cNormal) && oneof2(uc, LDATE::cZero, LDATE::cNormal) && (period.upp && period.upp >= period.low)) {
			if((!period.low || period.low > QIISuppDate) && (!period.upp || period.upp > QIISuppDate))
				ok = 1;
		}
		if(!ok) {
			SString msg_buf;
			msg_buf.Cat(period, 0);
			PPSetError(PPERR_INVQUOTPERIOD, msg_buf);
			period.SetZero();
		}
	}
	else
		ok = 1;
	ASSIGN_PTR(pPeriod, period);
	return ok;
}

SLAPI Quotation2Core::Quotation2Core() : Quotation2Tbl()
{
}

int SLAPI Quotation2Core::RecToQuot(const Quotation2Tbl::Rec * pRec, PPQuot & rQuot)
{
	rQuot.Clear();
	if(pRec->RelID)
		FetchRel(pRec->RelID, &rQuot);
	rQuot.GoodsID = pRec->GoodsID;
	rQuot.Dtm.Set(pRec->Dt, pRec->Tm);
	rQuot.Flags = pRec->Flags;
	if(pRec->Actual == 1)
		rQuot.Flags |= PPQuot::fActual;
	rQuot.RelID = pRec->RelID;
	rQuot.QTaID = pRec->TaID;
	rQuot.Quot = pRec->Quot;
	return 1;
}

int SLAPI Quotation2Core::RecToQuotRel(const Quotation2Tbl::Rec * pRec, PPQuot & rQuot)
{
	if(rQuot.RelID != pRec->RelID) {
		rQuot.Clear();
		if(pRec->RelID)
			FetchRel(pRec->RelID, &rQuot);
		rQuot.RelID = pRec->RelID;
	}
	rQuot.GoodsID = pRec->GoodsID;
	rQuot.Dtm.Set(pRec->Dt, pRec->Tm);
	rQuot.Flags = pRec->Flags;
	if(pRec->Actual == 1)
		rQuot.Flags |= PPQuot::fActual;
	rQuot.QTaID = pRec->TaID;
	rQuot.Quot = pRec->Quot;
	return 1;
}

int SLAPI Quotation2Core::GetMatrixRestrict(PPID mtxRestrQkID, PPID goodsGrpID, PPID locID, int srchNearest, long * pResult)
{
	int    ok = -1, r = -1;
	long   val = 0;
	if(mtxRestrQkID) {
		double q = 0.0;
		QuotIdent qi(locID, mtxRestrQkID, 0, 0);
		PPQuot quot;
		PPQuotArray ary(goodsGrpID);
		if((r = FetchList(goodsGrpID, ary)) >= 0 || (r = GetCurrList(goodsGrpID, qi.QuotKindID, 0, ary)) > 0)
			ok = (r > 0) ? (srchNearest ? ary.GetResult(qi, 0, 0, &q) : ary.GetQuot(qi, &quot)) : 0;
		val = (long)(srchNearest ? q : quot.Quot);
	}
	ASSIGN_PTR(pResult, val);
	return ok;
}

int SLAPI Quotation2Core::GetNearest(PPID goodsID, const QuotIdent & rIdent, PPQuot * pQuot, int useCache)
{
	int    ok = -1, r = -1;
	uint   pos = 0;
	PPQuotArray ary(goodsID);
	if((useCache && (r = FetchList(goodsID, ary)) >= 0) || (r = GetCurrList(goodsID, rIdent.QuotKindID, 0, ary)) > 0) {
		ok = (r > 0) ? ary.SearchNearest(rIdent, &pos) : 0;
		if(ok)
			ASSIGN_PTR(pQuot, ary.at(pos));
	}
	return ok;
}

int SLAPI Quotation2Core::GetCurrList(PPID goodsID, PPID quotKindID, PPID locID, PPQuotArray & rQuotList)
{
	int    ok = 1;
	PPQuot quot;
	Quotation2Tbl::Key3 k3, k3_;
	PROFILE_START
	if(quotKindID) {
		PPIDArray rel_list;
		{
			QuotFilt qfilt;
			qfilt.QuotKindID = quotKindID;
			if(locID)
				qfilt.LocID = locID;
			/* @v8.2.1 else
				qfilt.Flags |= QuotFilt::fAllLocations; */
			THROW(GetRelListByFilt(&qfilt, &rel_list));
		}
		rel_list.sort();
		for(uint i = 0; i < rel_list.getCount(); i++) {
			const PPID rel_id = rel_list.get(i);
			MEMSZERO(k3);
			k3.Actual = 1;
			k3.GoodsID = goodsID;
			k3.RelID = rel_id;
			if(search(3, &k3, spEq)) do {
				THROW(RecToQuot(&data, quot));
				if((!quotKindID || quot.Kind == quotKindID) && (!locID || quot.LocID == locID)) { // @paranoic
					THROW_SL(rQuotList.insert(&quot));
				}
			} while(search(3, &k3, spNext) && data.Actual == 1 && data.GoodsID == goodsID && data.RelID == rel_id);
		}
	}
	else {
		MEMSZERO(k3);
		k3.Actual = 1;
		k3.GoodsID = goodsID;
		k3_ = k3;
		//
		// Предварительная проверка на существование котировок по товару дает выигрыш
		// в скорости выполнения.
		//
		if(search(3, &k3, spGe) && data.Actual == 1 && data.GoodsID == goodsID) {
			BExtQuery q(this, 3, 256); // @v7.7.4 default(32)-->256
			q.selectAll().where(this->Actual == 1L && this->GoodsID == goodsID);
			for(q.initIteration(0, &k3_, spGe); q.nextIteration() > 0;) {
				THROW(RecToQuot(&data, quot));
				if((!quotKindID || quot.Kind == quotKindID) && (!locID || quot.LocID == locID)) {
					THROW_SL(rQuotList.insert(&quot));
				}
			}
		}
		else {
			THROW_DB(BTROKORNFOUND)
		}
	}
	rQuotList.GoodsID = goodsID;
	rQuotList.Sort();
	PROFILE_END
	CATCHZOK
	return ok;
}

int SLAPI Quotation2Core::GetCurrListByRelList(PPID goodsID, const PPIDArray & rRelList, PPQuotArray & rQuotList)
{
	int    ok = 1;
	PPQuot quot;
	Quotation2Tbl::Key3 k3;
	PROFILE_START
	for(uint i = 0; i < rRelList.getCount(); i++) {
		const PPID rel_id = rRelList.get(i);
		MEMSZERO(k3);
		k3.Actual = 1;
		k3.GoodsID = goodsID;
		k3.RelID = rel_id;
		if(search(3, &k3, spEq)) do {
			THROW(RecToQuot(&data, quot));
			THROW_SL(rQuotList.insert(&quot));
		} while(search(3, &k3, spNext) && data.Actual == 1 && data.GoodsID == goodsID && data.RelID == rel_id);
	}
	rQuotList.GoodsID = goodsID;
	rQuotList.Sort();
	PROFILE_END
	CATCHZOK
	return ok;
}

int SLAPI Quotation2Core::GetCurr(PPID goodsID, const QuotIdent & rIdent, double cost, double price, double * pQ, int useCache)
{
	int    ok = -1, r = -1;
	double q = 0.0;
	PPQuotArray ary(goodsID);
	if((useCache && (r = FetchList(goodsID, ary)) >= 0) || (r = GetCurrList(goodsID, rIdent.QuotKindID, 0, ary)) > 0)
		ok = (r > 0) ? ary.GetResult(rIdent, cost, price, &q) : 0;
	ASSIGN_PTR(pQ, q);
	return ok;
}

int SLAPI Quotation2Core::GetMatrixListByLoc(PPID mtxQkID, PPID locID, PPIDArray & rList, PPIDArray & rNegList)
{
	int    ok = 1;
	PPIDArray rel_list;
	QuotFilt qfilt;
	qfilt.QuotKindID = mtxQkID;
	if(locID)
		qfilt.LocID = locID;
	else
		qfilt.Flags |= QuotFilt::fAllLocations;
	THROW(GetRelListByFilt(&qfilt, &rel_list));
	rel_list.sort();
	{
		Quotation2Tbl::Key2 k2;
		for(uint i = 0; i < rel_list.getCount(); i++) {
			const PPID rel_id = rel_list.get(i);
			BExtQuery q(this, 2);
			q.select(this->GoodsID, this->Quot, 0L).where(this->Actual == 1L && this->RelID == rel_id);
			MEMSZERO(k2);
			k2.Actual = 1;
			k2.RelID = rel_id;
			for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
				int    r = 1;
				if(data.Quot > 0.0)
					r = rList.add(data.GoodsID);
				else if(data.Quot < 0.0)
					r = rNegList.add(data.GoodsID);
				THROW_SL(r);
			}
		}
	}
	rList.sort();
	rNegList.sort();
	CATCHZOK
	return ok;
}

int SLAPI Quotation2Core::Get(PPID goodsID, PPID relID, LDATETIME * pAfter, PPQuotArray * pList)
{
	int    ok = -1;
	Quotation2Tbl::Key0 k0;
	DBQ * dbq = 0;
	BExtQuery q(this, 0);
	MEMSZERO(k0);
	k0.GoodsID = goodsID;
	k0.RelID = relID;
	dbq = &(this->GoodsID == goodsID && this->RelID == relID);
	if(pAfter) {
		k0.Dt = pAfter->d;
		k0.Tm = pAfter->t;
		dbq = &(*dbq && this->Dt >= pAfter->d);
	}
	q.selectAll().where(*dbq);
	for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
		if(!pAfter || cmp(*pAfter, data.Dt, data.Tm) > 0) {
			if(pList) {
				PPQuot q;
				RecToQuot(&data, q);
				THROW_SL(pList->insert(&q));
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI Quotation2Core::GetRelListByFilt(const QuotFilt * pFilt, PPIDArray * pList)
{
	class SellerLocWorldCritBlock {
	public:
		SellerLocWorldCritBlock(PPID sellerLocWorldID)
		{
			WorldID = sellerLocWorldID;
		}
		int    FASTCALL Check(PPID locID)
		{
			int    ok = 0;
			if(WorldID) {
				LocationTbl::Rec loc_rec;
				if(LocObj.Fetch(locID, &loc_rec) > 0 && WObj.IsChildOf(loc_rec.CityID, WorldID))
					ok = 1;
			}
			else
				ok = 1;
			return ok;
		}
	private:
		PPID   WorldID;
		PPObjWorld WObj;
		PPObjLocation LocObj;
	};
	int    ok = -1;
	SellerLocWorldCritBlock * p_slwcb = 0;
	ObjIdListFilt loc_list;
	PPIDArray kind_list;
	if(pFilt->QuotKindID)
		kind_list.add(pFilt->QuotKindID);
	else {
		PPObjQuotKind qk_obj;
		PPQuotKind qk_rec;
		for(SEnum en = PPRef->Enum(PPOBJ_QUOTKIND, 0); en.Next(&qk_rec) > 0;) {
			int    cls = 0;
			if(qk_obj.Classify(qk_rec.ID, &cls) > 0 && cls == pFilt->QkCls)
				kind_list.add(qk_rec.ID);
		}
		kind_list.sortAndUndup();
	}
	if(pList)
		pList->clear();
	if(pFilt->Flags & QuotFilt::fAllLocations)
		loc_list.AddNotIgnoringZero(0);
	if(pFilt->LocID)
		loc_list.Add(pFilt->LocID);
	loc_list.Add(&pFilt->LocList.Get());
	if(loc_list.GetCount() == 0 && pFilt->SellerID) {
		PPID   seller_psn_id = ObjectToPerson(pFilt->SellerID, 0);
		loc_list.InitEmpty();
		if(seller_psn_id) {
			PPObjPerson psn_obj;
			PersonTbl::Rec psn_rec;
			PPIDArray _loc_list;
			if(psn_obj.GetDlvrLocList(seller_psn_id, &_loc_list) > 0) {
				loc_list.Set(&_loc_list);
			}
			else if(psn_obj.Search(seller_psn_id, &psn_rec) > 0) {
				loc_list.Add(psn_rec.MainLoc);
				loc_list.Add(psn_rec.RLoc);
			}
		}
	}
	if(!loc_list.IsExists() || loc_list.GetCount()) {
		const PPID single_qk_id = kind_list.getSingle();
		DBQ * dbq = 0;
		const LDATE eff_date = pFilt->EffDate.getactual(ZERODATE);
		dbq = ppcheckfiltid(dbq, RelT.QuotKindID, single_qk_id);
		dbq = ppcheckfiltid(dbq, RelT.CurID, pFilt->CurID);
		if(pFilt->Flags & QuotFilt::fZeroArOnly)
			dbq = &(*dbq && RelT.ArID == 0L);
		else
			dbq = ppcheckfiltid(dbq, RelT.ArID, pFilt->ArID);
		if(loc_list.GetCount() && loc_list.GetCount() <= 16) {
			dbq = ppcheckfiltidlist(dbq, RelT.LocID, &loc_list.Get());
		}
		Quot2RelTbl::Key1 k1;
		MEMSZERO(k1);
		if(single_qk_id) {
			k1.QuotKindID = single_qk_id;
			if(loc_list.GetSingle())
				k1.LocID = loc_list.GetSingle();
		}
		BExtQuery q(&RelT, 1);
		q.selectAll().where(*dbq);
		if(pFilt->SellerLocWorldID) {
			THROW_MEM(p_slwcb = new SellerLocWorldCritBlock(pFilt->SellerLocWorldID));
		}
		for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
			const PPID loc_id = RelT.data.LocID;
			if(kind_list.bsearch(RelT.data.QuotKindID) && loc_list.CheckID(loc_id) && (!p_slwcb || p_slwcb->Check(loc_id))) {
				if(eff_date) {
					DateRange period;
					Quotation2Core::PeriodIdxToPeriod(RelT.data.PeriodIdx, &period);
					if(!period.CheckDate(eff_date))
						continue;
				}
				if(pList)
					THROW_SL(pList->add(RelT.data.ID));
				ok = 1;
			}
		}
	}
	CATCHZOK
	delete p_slwcb;
	return ok;
}

int SLAPI Quotation2Core::GetRelByID(PPID relID, PPQuot * pVal)
{
	int    ok = 1;
	Quot2RelTbl::Rec rec;
	if(SearchByID(&RelT, 0, relID, &rec) > 0) {
		if(pVal) {
			pVal->ArID  = rec.ArID;
			pVal->Kind  = rec.QuotKindID;
			pVal->LocID = rec.LocID;
			pVal->CurID = rec.CurID;
			pVal->MinQtty = rec.MinQtty;
			Quotation2Core::PeriodIdxToPeriod(rec.PeriodIdx, &pVal->Period);
		}
		ok = 1;
	}
	else if(pVal) {
		pVal->ArID  = 0;
		pVal->Kind  = 0;
		pVal->LocID = 0;
		pVal->CurID = 0;
		pVal->MinQtty = 0;
		pVal->Period.SetZero();
	}
	return ok;
}

int SLAPI Quotation2Core::GetRel(const PPQuot * pVal, PPID * pID, int createIfNExists, int use_ta)
{
	int    ok = -1;
	Quot2RelTbl::Rec rec;
	Quot2RelTbl::Key1 k1;
	MEMSZERO(k1);
	k1.QuotKindID = pVal->Kind;
	k1.LocID = pVal->LocID;
	k1.ArID  = pVal->ArID;
	k1.CurID = pVal->CurID;
	Quotation2Core::PeriodToPeriodIdx(&pVal->Period, &k1.PeriodIdx); // @v7.3.5
	k1.MinQtty = pVal->MinQtty;
	int r = SearchByKey(&RelT, 1, &k1, &rec);
	THROW(r);
	if(r > 0) {
		ASSIGN_PTR(pID, rec.ID);
		ok = 1;
	}
	else if(createIfNExists) {
		MEMSZERO(rec);
		rec.QuotKindID = pVal->Kind;
		rec.LocID = pVal->LocID;
		rec.ArID  = pVal->ArID;
		rec.CurID = pVal->CurID;
		Quotation2Core::PeriodToPeriodIdx(&pVal->Period, &rec.PeriodIdx); // @v7.3.5
		rec.MinQtty = pVal->MinQtty;
		THROW(AddByID(&RelT, pID, &rec, use_ta));
		ok = 2;
	}
	CATCHZOK
	return ok;
}

int SLAPI Quotation2Core::GetMatrix(PPID mtxQkID, PPID locID, PPIDArray * pList)
{
	return Helper_GetMatrix(2, this, mtxQkID, locID, pList);
}

int SLAPI Quotation2Core::Set(PPQuotArray & rQList, long qtaID, const PPQuotArray * pTemplate, int noRmv, int use_ta)
{
	assert(!pTemplate || pTemplate->GoodsID == rQList.GoodsID);
	int    ok = 1, ta = 0, r = 0;
	int    is_updated = 0, is_mtx_updated = 0, is_mtx_restr_updated = 0, is_suppl_deal_upd = 0;
	PPIDArray mtx_loc_list, mtx_restr_loc_list, suppl_deal_ar_list;
	uint   i, pos;
	int    unupd = 0; // @debug
	PPQuotArray templ(rQList.GoodsID);
	THROW(PPStartTransaction(&ta, use_ta));
	if(pTemplate) {
		templ = *pTemplate;
	}
	else {
		GetCurrList(rQList.GoodsID, 0, 0, templ);
	}
	for(i = 0; i < rQList.getCount(); i++) {
		PPQuot & r_quot = rQList.at(i);
		r_quot.Flags &= ~PPQuot::fDbUpd; // @v7.2.1
		if((r_quot.Flags & (PPQuot::fPctDisabled|PPQuot::fZero)) || R2(r_quot.Quot) != 0.0) {
			if(r_quot.Kind == 0) {
				; // Инвалидное значение с нулевым видом котировки - должно быть удалено
			}
			else if(rQList.CanUpdateQuot(r_quot) > 0) {
				r_quot.GoodsID = rQList.GoodsID;
				const PPID loc_id = r_quot.LocID;
				const PPID ar_id = r_quot.ArID;
				{
					QuotIdent qi(loc_id, r_quot.Kind, r_quot.CurID, ar_id);
					// @v7.6.0 {
					// @v7.8.6 if(!r_quot.Period.IsZero())
						qi.SetIdentPeriod(&r_quot.Period);
					// } @v7.6.0
					qi.SetIdentQtty(&r_quot.MinQtty); // @v7.6.7
					if(templ.SearchQi(qi, &(pos = 0)) > 0)
						templ.atFree(pos);
				}
				THROW(r = Set(r_quot, qtaID, 0, 0));
				if(r > 0) {
					r_quot.Flags |= PPQuot::fDbUpd; // @v7.2.1
					is_updated = 1;
					if(r == 1000) {
						mtx_loc_list.addUnique(loc_id);
						is_mtx_updated = 1;
					}
					else if(r == 2000) {
						mtx_restr_loc_list.addUnique(loc_id);
						is_mtx_restr_updated = 1;
					}
					else if(r == 3000) {
						suppl_deal_ar_list.addUnique(ar_id);
						is_suppl_deal_upd = 1;
					}
				}
			}
		}
	}
	if(!noRmv) {
		for(i = 0; i < templ.getCount(); i++) {
			PPQuot & r_quot = templ.at(i);
			if(r_quot.Kind == 0) {
				; // @todo Инвалидное значение с нулевым видом котировки - должно быть удалено
			}
			else if(rQList.CanUpdateQuot(r_quot) > 0) {
				PPQuot zero_quot;
				zero_quot = r_quot;
				zero_quot.Quot = 0.0;
				zero_quot.Flags = 0;
				THROW(r = Set(zero_quot, qtaID, 0, 0));
				if(r > 0) {
					is_updated = 1;
					if(r == 1000) { // @v8.2.5 @fix (r = 1000)-->(r == 1000)
						mtx_loc_list.addUnique(r_quot.LocID);
						is_mtx_updated = 1;
					}
					else if(r == 2000) {
						mtx_restr_loc_list.addUnique(r_quot.LocID);
						is_mtx_restr_updated = 1;
					}
					else if(r == 3000) {
						suppl_deal_ar_list.addUnique(r_quot.ArID);
						is_suppl_deal_upd = 1;
					}
				}
			}
		}
	}
	if(is_updated) {
		DS.LogAction(PPACN_QUOTUPD2, PPOBJ_GOODS, rQList.GoodsID, qtaID, 0);
		if(is_mtx_updated) {
			uint c = mtx_loc_list.getCount();
			if(c) {
				do {
					DS.LogAction(PPACN_GOODSMTXUPD, PPOBJ_GOODS, rQList.GoodsID, mtx_loc_list.at(--c), 0);
				} while(c);
			}
			else
				DS.LogAction(PPACN_GOODSMTXUPD, PPOBJ_GOODS, rQList.GoodsID, -1, 0);
		}
		if(is_mtx_restr_updated) {
			uint c = mtx_restr_loc_list.getCount();
			if(c) {
				do {
					DS.LogAction(PPACN_GOODSMTXRESTRUPD, PPOBJ_GOODS, rQList.GoodsID, mtx_restr_loc_list.at(--c), 0);
				} while(c);
			}
			else
				DS.LogAction(PPACN_GOODSMTXRESTRUPD, PPOBJ_GOODS, rQList.GoodsID, -1, 0);
		}
		if(is_suppl_deal_upd) {
			uint c = suppl_deal_ar_list.getCount();
			if(c) {
				do {
					DS.LogAction(PPACN_SUPPLDEALUPD, PPOBJ_GOODS, rQList.GoodsID, suppl_deal_ar_list.at(--c), 0);
				} while(c);
			}
			else
				DS.LogAction(PPACN_SUPPLDEALUPD, PPOBJ_GOODS, rQList.GoodsID, -1, 0);
		}
	}
	THROW(PPCommitWork(&ta));
	if(is_updated)
		ok = 2;
	//
	// Note: Актуализацию кэша сделает функция Set(const PPQuot &, long, int)
	//
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI Quotation2Core::AdjustTime(Quotation2Tbl::Rec & rRec)
{
	int    ok = -1;
	LDATETIME dtm;
	if(!rRec.Dt)
		dtm = getcurdatetime_();
	else
		dtm.Set(rRec.Dt, rRec.Tm);
	Quotation2Tbl::Key0 k0;
	MEMSZERO(k0);
	k0.GoodsID = rRec.GoodsID;
	k0.RelID = rRec.RelID;
	k0.Dt = dtm.d;
	k0.Tm = dtm.t;
	while(search(0, &k0, spEq)) {
		k0.GoodsID = rRec.GoodsID;
		k0.RelID = rRec.RelID;
		dtm.addhs(1);
		k0.Dt = dtm.d;
		k0.Tm = dtm.t;
	}
	THROW_DB(BTROKORNFOUND);
	if(dtm.d != rRec.Dt || dtm.t != rRec.Tm) {
		rRec.Dt = dtm.d;
		rRec.Tm = dtm.t;
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI Quotation2Core::Set(const PPQuot & rQ, long qtaID, int logSj, int use_ta)
{
	int    ok = 1, ta = 0;
	PROFILE_START
	PPID   rel_id = 0;
	int    do_add_rec = 0;
	int    act_clr = 0; // Признак того, что с текущего актуального значения был снят признак актуальности
	int    is_mtx_upd = 0, is_mtx_restr_upd = 0, is_suppl_deal_upd = 0;
	double prev_qv = 0.0;
	long   prev_qf = 0;
	Quotation2Tbl::Key2 k2; // {Actual, RelID, GoodsID} (anysegnull dup mod);
	Quotation2Tbl::Rec last_actual_rec;
	Quotation2Tbl::Rec rec;
	DBRowIdArray ri_list;
	PPObjQuotKind qk_obj;
	PPQuotKind qk_rec;
	PPObjQuotKind::Special spc;
	PROFILE(PPObjQuotKind::GetSpecialKinds(&spc, 1));
	THROW(qk_obj.Fetch(rQ.Kind, &qk_rec) > 0);
	THROW(PPStartTransaction(&ta, use_ta));
	PROFILE(THROW(GetRel(&rQ, &rel_id, 1, 0) > 0));
	MEMSZERO(rec);
	rec.RelID = rel_id;
	rec.GoodsID = rQ.GoodsID;
	rec.Actual = 1;
	rec.Flags = (int16)(rQ.Flags & PPQuot::fDbMask);
	rec.Quot = rQ.Quot;
	rec.TaID = qtaID;

	PROFILE_START
	MEMSZERO(k2);
	k2.Actual = 1;
	k2.RelID = rel_id;
	k2.GoodsID = rQ.GoodsID;
	if(search(2, &k2, spEq)) { // @v8.2.6 searchForUpdate-->search
		do {
			DBRowId ri;
			THROW_DB(getPosition(&ri));
			THROW_SL(ri_list.insert(&ri));
			last_actual_rec = data;
		} while(search(2, &k2, spNext) && data.Actual && data.RelID == rel_id && data.GoodsID == rQ.GoodsID); // @v8.2.6 searchForUpdate-->search
	}
	PROFILE_END
	if(ri_list.getCount()) {
		//
		// Если так получилось (хотя это - сбойная ситуация), что у нас более одной актуальной записи,
		// то для всех кроме последней устанавливаем значение Actual = 0.
		//
		const uint c = ri_list.getCount();
		for(uint i = 0; i < c; i++) {
			if(i == (c-1)) {
				if(last_actual_rec.Quot != rQ.Quot || (last_actual_rec.Flags & PPQuot::fDbMask) != (rQ.Flags & PPQuot::fDbMask)) {
					do_add_rec = 1;
					prev_qv = last_actual_rec.Quot;
					prev_qf = last_actual_rec.Flags;
				}
				else {
					ok = -1;
					break;
				}
			}
			PROFILE_START
			THROW_DB(getDirectForUpdate(0, 0, ri_list.at(i)));
			data.Actual = 0;
			THROW_DB(updateRec()); // @sfu
			PROFILE_END
			act_clr = 1;
		}
	}
	else
		do_add_rec = 1;
	if(do_add_rec) {
		PROFILE_START
		if(rQ.Kind == 0)
			ok = 1;
		else if(rQ.Kind == spc.MtxID) {
			ok = qk_obj.CheckRights(QUOTRT_UPDMTX) ? (is_mtx_upd = 1) : LogDeclineMsg(&rQ, PPQC_MATRIX);
			//
			// Если в матрицу вводится новый элемент и существуют ограничения на матрицу,
			// то проверяем эти ограничения //
			// При выводе элементов из матрицы ограничения не проверяем.
			//
			if(spc.MtxRestrID && rQ.Quot > 0.0 && prev_qv <= 0.0) {
				PPObjGoods goods_obj;
				THROW(goods_obj.CheckMatrixRestrict(rQ.GoodsID, rQ.LocID, (long)rQ.Quot));
			}
		}
		else if(rQ.Kind == spc.MtxRestrID)
			ok = qk_obj.CheckRights(QUOTRT_UPDMTXRESTR) ? (is_mtx_restr_upd = 1) : LogDeclineMsg(&rQ, PPQC_MATRIXRESTR);
		else if(spc.IsSupplDealKind(rQ.Kind)) {
			ok = qk_obj.CheckRights(QUOTRT_UPDSUPPLCOST) ? 1 : LogDeclineMsg(&rQ, PPQC_SUPPLDEAL);
			if(rQ.Kind == spc.SupplDealID)
				is_suppl_deal_upd = 1;
		}
		else
			ok = qk_obj.CheckRights(QUOTRT_UPDQUOTS) ? 1 : LogDeclineMsg(&rQ, PPQC_PRICE);
		if(ok > 0) {
			const long mask = (PPQuot::fZero|PPQuot::fPctOnCost|PPQuot::fPctOnPrice|PPQuot::fPctOnAddition|PPQuot::fPctOnBase);
			if((rec.Flags & mask) || rec.Quot != 0.0) {
				THROW(AdjustTime(rec));
				THROW_DB(insertRecBuf(&rec));
				if(logSj)
					DS.LogAction(PPACN_QUOTUPD2, PPOBJ_GOODS, rQ.GoodsID, qtaID, 0);
			}
			else if(act_clr) {
				if(logSj)
					DS.LogAction(PPACN_QUOTRMV2, PPOBJ_GOODS, rQ.GoodsID, qtaID, 0);
			}
			if(logSj) {
				if(is_mtx_upd)
					DS.LogAction(PPACN_GOODSMTXUPD, PPOBJ_GOODS, rQ.GoodsID, rQ.LocID, 0);
				else if(is_mtx_restr_upd)
					DS.LogAction(PPACN_GOODSMTXRESTRUPD, PPOBJ_GOODS, rQ.GoodsID, rQ.LocID, 0);
				else if(is_suppl_deal_upd)
					DS.LogAction(PPACN_SUPPLDEALUPD, PPOBJ_GOODS, rQ.GoodsID, rQ.ArID, 0);
			}
			if(is_mtx_upd)
				ok = 1000;
			else if(is_mtx_restr_upd)
				ok = 2000;
			else if(is_suppl_deal_upd)
				ok = 3000;
			else
				ok = 1;
		}
		PROFILE_END
	}
	THROW(PPCommitWork(&ta));
	if(ok > 0) {
		DirtyGoodsID(rQ.GoodsID);
		if(is_mtx_upd) {
			PPIDArray goods_list, loc_list;
			goods_list.add(rQ.GoodsID);
			loc_list.add(rQ.LocID);
			const int deferred = DS.IsDbCacheDeferredState(DBS.GetDbPathID());
			DirtyMatrix(&goods_list, &loc_list, deferred);
		}
	}
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	PROFILE_END
	return ok;
}

int SLAPI Quotation2Core::GetGoodsList(PPID relID, int actualOnly, PPIDArray & rList)
{
	int    ok = -1;
	UintHashTable tab;
	if(actualOnly) {
		Quotation2Tbl::Key2 k2;
		MEMSZERO(k2);
		k2.Actual = 1;
		k2.RelID = relID;
		BExtQuery q(this, 2);
		q.select(this->GoodsID, 0).where(this->Actual == 1L && this->RelID == relID);
		for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
			tab.Add((ulong)data.GoodsID);
			rList.add(data.GoodsID);
		}
	}
	else {
		Quotation2Tbl::Key1 k1;
		MEMSZERO(k1);
		k1.RelID = relID;
		BExtQuery q(this, 1);
		q.select(this->GoodsID, 0).where(this->RelID == relID);
		for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
			tab.Add((ulong)data.GoodsID);
			rList.add(data.GoodsID);
		}
	}
	{
		ulong v = 0;
		if(tab.Enum(&v) > 0) {
			ok = 1;
			do {
				rList.add(v);
			} while(tab.Enum(&v) > 0);
		}
	}
	return ok;
}

int SLAPI Quotation2Core::RemoveAllForRel(PPID relID, const char * pPctMsg, int use_ta)
{
	int    ok = -1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		// THROW_DB(deleteFrom(this, 0, (this->RelID == relID)));
		{
			uint   lc = 0;
			Quotation2Tbl::Key1 k1;
			MEMSZERO(k1);
			k1.RelID = relID;
			BExtQuery q(this, 1);
			q.selectAll().where(this->RelID == relID);
			for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
				lc++;
			}
			{
				uint   j = 0;
				MEMSZERO(k1);
				k1.RelID = relID;
				if(search(1, &k1, spGe) && data.RelID == relID) do {
					THROW_DB(deleteRec());
					ok = 1;
					PPWaitPercent(++j, lc, pPctMsg);
				} while(search(1, &k1, spNext) && data.RelID == relID);
			}
		}
		THROW(RemoveByID(&RelT, relID, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Quotation2Core::RemoveAllForQuotKind(PPID quotKindID, int use_ta)
{
	int    ok = 1;
	{
		PPIDArray rel_list;
		SString wait_msg_buf;
		PPTransaction tra(use_ta);
		THROW(tra);
		{
			Quot2RelTbl::Key1 rk1;
			MEMSZERO(rk1);
			rk1.QuotKindID = quotKindID;
			BExtQuery qr(&RelT, 1);
			qr.selectAll().where(RelT.QuotKindID == quotKindID);
			for(qr.initIteration(0, &rk1, spGe); qr.nextIteration() > 0;) {
				assert(RelT.data.QuotKindID == quotKindID);
				rel_list.add(RelT.data.ID);
			}
		}
		const uint c = rel_list.getCount();
		for(uint i = 0; i < c; i++) {
			const PPID rel_id = rel_list.get(i);
			wait_msg_buf.Z().Cat(i+1).CatChar('/').Cat(c);
			THROW(RemoveAllForRel(rel_id, wait_msg_buf, 0));
			//PPWaitPercent(i+1, c);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Quotation2Core::GetAddressLocList(PPIDArray & rList)
{
	int    ok = -1;
	rList.clear();

	PPObjLocation loc_obj;
	LocationTbl::Rec loc_rec;
	Quot2RelTbl::Key2 k2;
	MEMSZERO(k2);
	BExtQuery q(&RelT, 2, 1024);
	q.select(RelT.LocID, RelT.QuotKindID, 0);
	PPID   prev_loc_id = 0;
	for(q.initIteration(0, &k2, spFirst); q.nextIteration() > 0;) {
		const PPID loc_id = RelT.data.LocID;
		if(loc_id != prev_loc_id) {
			if(loc_obj.Fetch(loc_id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_ADDRESS) {
				rList.add(loc_id);
				ok = 1;
			}
			prev_loc_id = loc_id;
		}
	}
	rList.sortAndUndup();
	return ok;
}

int SLAPI Quotation2Core::Helper_ReplaceRel(PPID replacedRelID, PPID newRelID, int use_ta)
{
	int    ok = 1;
	PPIDArray goods_list1, goods_list2;
	PPTransaction tra(use_ta);
	THROW(tra);
	{
		//
		// Для тех товаров, по которым есть значения по replacedRelID и нет по newRelID просто
		// перемещаем все котировки.
		//
		goods_list1.clear();
		goods_list2.clear();
		GetGoodsList(replacedRelID, 0, goods_list1);
		GetGoodsList(newRelID, 0, goods_list2);
		for(uint i = 0; i < goods_list1.getCount(); i++) {
			const PPID goods_id = goods_list1.get(i);
			if(!goods_list2.lsearch(goods_id)) {
				THROW_DB(updateFor(this, 0, (this->RelID == replacedRelID), set(this->RelID, dbconst(newRelID))));
			}
		}
	}
	{
		//
		// Теперь перемещаем АКТУАЛЬНЫЕ котировки с replacedRelID на newRelID если по newRelID
		// для товара НЕТ АКТУАЛЬНОЙ котировки.
		//
		goods_list1.clear();
		goods_list2.clear();
		GetGoodsList(replacedRelID, 1, goods_list1);
		GetGoodsList(newRelID, 1, goods_list2);
		for(uint i = 0; i < goods_list1.getCount(); i++) {
			const PPID goods_id = goods_list1.get(i);
			Quotation2Tbl::Key2 k2;
			MEMSZERO(k2);
			k2.Actual = 1;
			k2.RelID = replacedRelID;
			k2.GoodsID = goods_id;
			THROW_DB(searchForUpdate(2, &k2, spEq));
			if(!goods_list2.lsearch(goods_id)) {
				data.RelID = newRelID;
				THROW_DB(updateRec()); // @sfu
			}
		}
	}
	{
		//
		// Наконец, у нас (возможно) остались котировки, которые не могут однозначно трактоваться (должны ли они
		// принадлежать replacedRelID или newRelID.
		// Эту неоднозначность разрешаем очень прямолинейно - удаляем все оставшиеся котировки, принадлежащие replacedRelID.
		//
		THROW_DB(deleteFrom(this, 0, (this->RelID == replacedRelID)));
	}
	THROW(tra.Commit());
	CATCHZOK
	return ok;
}

int SLAPI Quotation2Core::ReplaceObj(PPID objType, PPID replacedID, PPID newID, int use_ta)
{
	int    ok = -1;
	TSVector <Quot2RelTbl::Rec> rel_list; // @v9.8.4 TSArray-->TSVector
	if(objType == PPOBJ_ARTICLE) {
		Quot2RelTbl::Key3 k3;
		MEMSZERO(k3);
		k3.ArID = replacedID;
		if(RelT.search(3, &k3, spEq)) do {
			rel_list.insert(&RelT.data);
		} while(RelT.search(3, &k3, spNext) && RelT.data.ArID == replacedID);
		if(rel_list.getCount()) {
			PPTransaction tra(use_ta);
			THROW(tra);
			for(uint i = 0; i < rel_list.getCount(); i++) {
				Quot2RelTbl::Rec & r_rec = rel_list.at(i);
				PPQuot q;
				q.Kind  = r_rec.QuotKindID;
				q.LocID = r_rec.LocID;
				q.ArID  = newID;
				q.CurID = r_rec.CurID;
				q.MinQtty = r_rec.MinQtty;
				PPID   new_rel_id = 0;
				THROW(GetRel(&q, &new_rel_id, 1, 0));
				THROW(Helper_ReplaceRel(r_rec.ID, new_rel_id, 0));
			}
			THROW(tra.Commit());
		}
	}
	else if(objType == PPOBJ_LOCATION) {
		Quot2RelTbl::Key2 k2;
		MEMSZERO(k2);
		k2.LocID = replacedID;
		if(RelT.search(2, &k2, spEq)) do {
			rel_list.insert(&RelT.data);
		} while(RelT.search(2, &k2, spNext) && RelT.data.LocID == replacedID);
		if(rel_list.getCount()) {
			PPTransaction tra(use_ta);
			THROW(tra);
			for(uint i = 0; i < rel_list.getCount(); i++) {
				Quot2RelTbl::Rec & r_rec = rel_list.at(i);
				PPQuot q;
				q.Kind  = r_rec.QuotKindID;
				q.LocID = newID;
				q.ArID  = r_rec.ArID;
				q.CurID = r_rec.CurID;
				q.MinQtty = r_rec.MinQtty;
				PPID   new_rel_id = 0;
				THROW(GetRel(&q, &new_rel_id, 1, 0));
				THROW(Helper_ReplaceRel(r_rec.ID, new_rel_id, 0));
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI Quotation2Core::VerifyRecord(Quotation2Tbl::Rec & rRec)
{
	int    ok = 1;
	Quot2RelTbl::Rec rel_rec;
	THROW_PP(rRec.RelID != 0, PPERR_QII_ZEROREL);
	THROW_PP(SearchByID(&RelT, 0, rRec.RelID, &rel_rec) > 0, PPERR_QII_NEXISTREL);
	THROW_PP(rRec.GoodsID != 0, PPERR_QII_ZEROGOODS);
	THROW_SL(checkdate(rRec.Dt, 0));
	THROW_SL(checktime(rRec.Tm));
	THROW_PP(oneof2(rRec.Actual, 0, 1), PPERR_QII_INVACTUAL);
	THROW_PP((rRec.Flags & ~PPQuot::fDbMask) == 0, PPERR_QII_INVFLAGS);
	{
		uint   w = (uint)(rRec.Flags & (PPQuot::fPctOnCost|PPQuot::fPctOnPrice|PPQuot::fPctOnAddition|PPQuot::fPctOnBase));
		//
		// Трюк с подсчетом ненулевых битов посредством обнуления последнего единичного бита X & (X-1) {
		//
		uint   popc = 0;
		for(; w != 0; w &= (w-1))
			popc++;
		// }
		THROW_PP(popc == 0 || popc == 1, PPERR_QII_AMBIGFLAGS);
	}
	THROW_PP(!(rRec.Flags & PPQuot::fZero) || rRec.Quot == 0.0, PPERR_QII_NZEROVAL);
	THROW_PP(IsValidIEEE(rRec.Quot), PPERR_QII_INVVAL);
	CATCHZOK
	return ok;
}

int SLAPI Quotation2Core::Verify()
{
	int    ok = 1;
	SString msg_buf;
	PPLogger logger;
	Quotation2Tbl::Rec rec;
	IterCounter cntr;
	PPInitIterCounter(cntr, this);
	PPWait(1);
	{
		PPObjQuotKind qk_obj;
		UintHashTable absence_rel_tab;
		PPQuotKind qk_rec;
		Quot2RelTbl::Key1 k1; // QuotKindID, LocID, ArID, CurID, PeriodIdx, MinQtty (unique); // #1 @v7.3.5
		MEMSZERO(k1);
		if(RelT.search(1, &k1, spFirst)) {
			PPID   prev_qk_id = RelT.data.QuotKindID-1;
			int    absence_qk = 0;
			do {
				const PPID qk_id = RelT.data.QuotKindID;
				if(qk_id != prev_qk_id) {
					int    r = qk_obj.Search(qk_id, &qk_rec);
					if(r > 0) {
						absence_qk = 0;
					}
					else if(r < 0) {
						absence_rel_tab.Add((ulong)RelT.data.ID);
						absence_qk = 1;
						msg_buf.Z().Cat(qk_id);
						PPSetError(PPERR_QII_HANGQKREL, msg_buf);
						logger.LogLastError();
					}
					else {
						absence_qk = -1;
					}
				}
				else if(absence_qk > 0) {
					absence_rel_tab.Add((ulong)RelT.data.ID);
				}
			} while(RelT.search(1, &k1, spNext));
		}
		{
			const uint _c = absence_rel_tab.GetCount();
			if(_c) {
				uint   i = 0;
				PPTransaction tra(1);
				THROW(tra);
				for(ulong u_rel_id = 0; absence_rel_tab.Enum(&u_rel_id) > 0;) {
					msg_buf.Z().Cat(++i).CatChar('/').Cat(_c);
					THROW(RemoveAllForRel((long)u_rel_id, 0, 0));
				}
				THROW(tra.Commit());
			}
		}
	}
	{
		//
		// Проверяем все записи таблицы
		//
		UintHashTable goods_list;
		PPID   prev_rel_id = 0;
		Quotation2Tbl::Key1 k1;
		MEMSZERO(k1);
		for(int sp = spFirst; search(1, &k1, sp); sp = spNext) {
			copyBufTo(&rec);
			if(prev_rel_id && rec.RelID != prev_rel_id) {
				goods_list.Clear();
			}
			else if(rec.Actual) {
				if(goods_list.Has((ulong)rec.GoodsID)) {
					msg_buf.Z().CatChar('[').Cat(rec.RelID).CatDiv(';', 2).Cat(rec.GoodsID).CatChar(']');
					PPSetError(PPERR_QII_DUPACTUALTAG, msg_buf);
					logger.LogLastError();
				}
				else {
					goods_list.Add((ulong)rec.GoodsID);
				}
			}
			if(!VerifyRecord(rec)) {
				logger.LogLastError();
			}
			prev_rel_id = rec.RelID;
			PPWaitPercent(cntr.Increment());
		}
	}
	PPWait(0);
	CATCHZOK
	return ok;
}
//
//
//
class Quot2RelCache : public ObjCacheHash {
public:
	struct Data : public ObjCacheEntry {
		long   QuotKindID;
		long   LocID;
		long   ArID;
		long   CurID;
		long   MinQtty;
		int32  PeriodIdx; // @v7.3.5
	};
	SLAPI  Quot2RelCache() : ObjCacheHash(PPOBJ_QUOT2REL, sizeof(Data), 512*1024, 4)
	{
		P_Qc = 0;
	}
	int    SLAPI Fetch(Quotation2Core * pTbl, PPID id, PPQuot * pVal)
	{
		int    ok = -1;
		ENTER_CRITICAL_SECTION // Критическая секция защищает указатель P_Qc который инициализируется внешним параметром
			// только на время исполнения данной функции.
		P_Qc = pTbl;
		ok = Get(id, pVal);
		P_Qc = 0;
		LEAVE_CRITICAL_SECTION
		return ok;
	}
private:
	virtual int SLAPI Quot2RelCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
	{
		int    ok = 1;
		Data * p_cache_rec = (Data *)pEntry;
		PPQuot q;
		if(P_Qc && P_Qc->GetRelByID(id, &q) > 0) {
			if(p_cache_rec) {
				p_cache_rec->ID = id;
				p_cache_rec->QuotKindID = q.Kind;
				p_cache_rec->LocID = q.LocID;
				p_cache_rec->ArID  = q.ArID;
				p_cache_rec->CurID = q.CurID;
				p_cache_rec->MinQtty = q.MinQtty;
				Quotation2Core::PeriodToPeriodIdx(&q.Period, &p_cache_rec->PeriodIdx); // @v7.3.5
			}
		}
		else
			ok = -1;
		return ok;
	}
	virtual void SLAPI Quot2RelCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
	{
		PPQuot * p_data_rec = (PPQuot *)pDataRec;
		const Data * p_cache_rec = (const Data *)pEntry;
		p_data_rec->Kind = p_cache_rec->QuotKindID;
		p_data_rec->LocID = p_cache_rec->LocID;
		p_data_rec->ArID  = p_cache_rec->ArID;
		p_data_rec->CurID = p_cache_rec->CurID;
		p_data_rec->MinQtty = p_cache_rec->MinQtty;
		Quotation2Core::PeriodIdxToPeriod(p_cache_rec->PeriodIdx, &p_data_rec->Period); // @v7.3.5
	}

	Quotation2Core * P_Qc; // @notowned
};

int SLAPI Quotation2Core::FetchRel(PPID relID, PPQuot * pVal)
{
	Quot2RelCache * p_cache = GetDbLocalCachePtr <Quot2RelCache> (PPOBJ_QUOT2REL);
	return p_cache ? p_cache->Fetch(this, relID, pVal) : -1;
}
//
//
//
//
// Quot2Cache
//
class Quot2Cache : public ObjCache, public QuotCacheBase {
public:
	friend class Quotation2Core;

	class CacheEntry {
	public:
		struct Entry {
			uint   ParamID;
			long   Flags;
			long   MinQtty;
			int32  PeriodIdx;
			double Val;
		};
		void * SLAPI operator new(size_t sz, const PPQuotArray * pQuotList, Quot2Cache * pQuotCache)
		{
			const uint c = pQuotList->getCount();
			size_t s = sz + (pQuotList ? (sizeof(Entry) * c) : 0);
			CacheEntry * p = (CacheEntry *)new char[s];
			if(p) {
				memzero(p, s);
				p->GoodsID = pQuotList->GoodsID;
				for(uint i = 0; i < c; i++) {
					const PPQuot & r_quot = pQuotList->at(i);
					Entry & r_entry = ((Entry *)(p+1))[i];
					pQuotCache->GetQuotParamID(&r_quot, &r_entry.ParamID);
					r_entry.Flags = r_quot.Flags;
					r_entry.MinQtty = r_quot.MinQtty; // @v6.9.5
					Quotation2Core::PeriodToPeriodIdx(&r_quot.Period, &r_entry.PeriodIdx); // @v7.3.5
					r_entry.Val = r_quot.Quot;
				}
				p->EntryCount = (uint16)c;
			}
			return p;
		}
		int    SLAPI Get(PPQuotArray * pQuotList, PPID quotKindID, PPID locID, const Quot2Cache * pQuotCache)
		{
			UseCount++;
			pQuotList->GoodsID = GoodsID;
			pQuotList->freeAll();
			for(uint16 i = 0; i < EntryCount; i++) {
				PPQuot quot(GoodsID);
				const Entry & r_entry = ((Entry *)(this+1))[i];
				quot.Flags = r_entry.Flags;
				quot.MinQtty = r_entry.MinQtty; // @v6.9.5
				Quotation2Core::PeriodIdxToPeriod(r_entry.PeriodIdx, &quot.Period); // @v7.3.5
				quot.Quot = r_entry.Val;
				pQuotCache->GetQuotParam(r_entry.ParamID, &quot);
				if((!quotKindID || quot.Kind == quotKindID) && (!locID || quot.LocID == locID))
					pQuotList->insert(&quot);
			}
			return 1;
		}
		PPID   GoodsID;
		long   UseCount;
		uint16 EntryCount;
	};
	class QuotCacheCollection : public SCollection {
	public:
		SLAPI  QuotCacheCollection() : SCollection() {}
		virtual void FASTCALL freeItem(void * pItem) { delete ((CacheEntry *)pItem); }
	};
	SLAPI  Quot2Cache();
	SLAPI ~Quot2Cache();
	void   FASTCALL SetTable(Quotation2Core * pTbl)
	{
		P_Qc = pTbl;
		QuotCacheBase::SetQuotProvider(pTbl);
	}
	int    FASTCALL Put(const PPQuotArray *);
	int    SLAPI Get_(PPID goodsID, PPID quotKindID, PPID locID, PPQuotArray *); // non-virtual
	int    SLAPI GetQuotParamID(const PPQuot * pQuot, uint * pID);
	int    SLAPI GetQuotParam(uint id, PPQuot * pQuot) const;
	int    SLAPI BelongToMatrix(Quotation2Core * pTbl, PPID goodsID, PPID locID);
	int    FASTCALL Dirty_(PPID goodsID); // @sync non-virtual
	int    SLAPI Clear_(); // @sync
	//int    SLAPI Lock()   { return RwL.WriteLock(); }
	//int    SLAPI Unlock() { return RwL.Unlock();    }
private:
	static int OnSysJ(int kind, const PPNotifyEvent * pEv, void * procExtPtr); // PPAdviseProc()

	virtual int  SLAPI FetchEntry(PPID id, ObjCacheEntry * pEntry, long extraData = 0)
	{
		return 0;
	}
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
	{
	}
	int    FASTCALL Search(PPID goodsID, uint * pPos) const
	{
		return P_Ary->bsearch(&goodsID, pPos, CMPF_LONG);
	}

	Quotation2Core * P_Qc;
};

IMPL_CMPFUNC(GoodsQuot2CacheEntry_UseCountDesc, i1, i2)
{
	// descending order
	return cmp_long(((Quot2Cache::CacheEntry *)i2)->UseCount, ((Quot2Cache::CacheEntry *)i1)->UseCount);
}

//static
int Quot2Cache::OnSysJ(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
{
	int    ok = -1;
	if(kind == PPAdviseBlock::evDirtyCacheBySysJ) {
		if(oneof2(pEv->Action, PPACN_GOODSQUOTUPD, PPACN_QUOTUPD2)) {
			if(oneof2(pEv->ObjType, PPOBJ_GOODS, PPOBJ_GOODSGROUP) && pEv->ObjID) {
				((Quot2Cache *)procExtPtr)->Dirty_(pEv->ObjID);
				ok = 1;
			}
		}
		else {
			static PPIDArray * p_mtx_goods_list = 0;
			//
			// CRITICAL_SECTION нужна чтобы защитить статический p_mtx_goods_list.
			//
			ENTER_CRITICAL_SECTION
			if(pEv->Action == PPACN_GOODSMTXUPD) {
				if(pEv->ObjID) {
					SETIFZ(p_mtx_goods_list, new PPIDArray);
					p_mtx_goods_list->add(pEv->ObjID);
				}
				ok = 1;
			}
			else if(pEv->Action == PPACN_MTXGOODSADD) {
				if(pEv->ObjID) {
					SETIFZ(p_mtx_goods_list, new PPIDArray);
					p_mtx_goods_list->add(pEv->ObjID);
				}
			}
			else if(pEv->IsFinish()) {
				ok = Helper_CallDirtyMatrix(p_mtx_goods_list);
				ZDELETE(p_mtx_goods_list);
			}
			LEAVE_CRITICAL_SECTION
		}
	}
	return ok;
}

SLAPI Quot2Cache::Quot2Cache() : ObjCache(PPOBJ_QUOT2, sizeof(ObjCacheEntry), ObjCache::fOwnerArray), QuotCacheBase(2)
{
	P_Ary = new QuotCacheCollection();
	{
		long   cookie = 0;
		cookie = _AdviseQuotCacheUpdate(PPACN_GOODSQUOTUPD, Quot2Cache::OnSysJ, this);
		cookie = _AdviseQuotCacheUpdate(PPACN_QUOTUPD2,     Quot2Cache::OnSysJ, this);
		cookie = _AdviseQuotCacheUpdate(PPACN_QUOTRMV2,     Quot2Cache::OnSysJ, this);
		cookie = _AdviseQuotCacheUpdate(PPACN_GOODSMTXUPD,  Quot2Cache::OnSysJ, this);
		cookie = _AdviseQuotCacheUpdate(PPACN_MTXGOODSADD,  Quot2Cache::OnSysJ, this);
	}
}

SLAPI Quot2Cache::~Quot2Cache()
{
	// DS.Unadvise(AdvCookie); Приводит к зависанию при завершении сессии
	// @v6.7.1 (Приводит к зависанию при завершении сессии) LogStat();
}

int FASTCALL Quot2Cache::Put(const PPQuotArray * pItem)
{
	if(HalfSize) {
		if(P_Ary->getCount() >= (HalfSize * 2)) {
			P_Ary->sort(PTR_CMPFUNC(GoodsQuot2CacheEntry_UseCountDesc));
			while(P_Ary->getCount() > HalfSize) {
				const uint pos = P_Ary->getCount()-1;
				LStatData.RmvItemsCount++;
				const long use_count = ((CacheEntry *)P_Ary->at(pos))->UseCount;
				LStatData.RmvItemsHits += use_count;
				P_Ary->atFree(pos);
			}
			P_Ary->sort(CMPF_LONG);
		}
	}
	CacheEntry * p_entry = new (pItem, this) CacheEntry;
	return BIN(p_entry && P_Ary->ordInsert(p_entry, 0, CMPF_LONG));
}

int SLAPI Quot2Cache::Get_(PPID goodsID, PPID quotKindID, PPID locID, PPQuotArray * pItem)
{
	int    ok = 1;
	uint   pos = 0;
	LStatData.Hits++;
	if(Search(goodsID, &pos)) {
		CacheEntry * p_entry = (CacheEntry *)P_Ary->at(pos);
		THROW(p_entry->Get(pItem, quotKindID, locID, this));
	}
	else {
		PPQuotArray temp_item;
		THROW(P_Qc->GetCurrList(goodsID, 0, 0, temp_item));
		THROW(Put(&temp_item));
		THROW(Get_(goodsID, quotKindID, locID, pItem)); // @recursion
		// test
#if _DEBUG // {
		{
			PPQuotArray test_item;
			THROW(Get_(goodsID, 0, 0, &test_item)); // @recursion
			assert(test_item.GoodsID == temp_item.GoodsID);
			assert(test_item.getCount() == temp_item.getCount());
			for(uint i = 0; i < test_item.getCount(); i++) {
				const PPQuot & test = test_item.at(i);
				const PPQuot & temp = temp_item.at(i);
				assert(test.ArID == temp.ArID && test.GoodsID == temp.GoodsID &&
					test.LocID == temp.LocID && test.CurID == temp.CurID &&
					test.Kind == temp.Kind && test.Flags == temp.Flags);
				assert(test.Quot == temp.Quot);
			}
		}
#endif // } _DEBUG
		//
		LStatData.Hits--;
		LStatData.Misses++;
	}
	CATCHZOK
	return ok;
}

int FASTCALL Quot2Cache::Dirty_(PPID goodsID)
{
	int    ok = -1;
	uint   pos = 0;
	{
		//Lock();
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		if(Search(goodsID, &pos)) {
			P_Ary->atFree(pos);
			ok = 1;
		}
		//Unlock();
	}
	return ok;
}

int SLAPI Quot2Cache::Clear_()
{
	int    ok = -1;
	uint   pos = 0;
	{
		//Lock();
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		CALLPTRMEMB(P_Ary, freeAll());
		ok = 1;
		//Unlock();
	}
	return ok;
}

int SLAPI Quot2Cache::GetQuotParamID(const PPQuot * pQuot, uint * pID)
{
	int    ok = 0;
	QuotParamEntry entry, * p_e;
	MEMSZERO(entry);
	entry.QkID  = pQuot->Kind;
	entry.LocID = pQuot->LocID;
	entry.ArID  = pQuot->ArID;
	entry.CurID = pQuot->CurID;
	for(uint i = 0; !ok && QpList.enumItems(&i, (void **)&p_e);)
		if(memcmp(p_e, &entry, sizeof(entry)) == 0) {
			ASSIGN_PTR(pID, i);
			ok = 1;
		}
	if(!ok && QpList.insert(&entry)) {
		ASSIGN_PTR(pID, QpList.getCount());
		ok = 1;
	}
	return ok;
}

int SLAPI Quot2Cache::GetQuotParam(uint id, PPQuot * pQuot) const
{
	if(id > 0 && id <= QpList.getCount()) {
		const QuotParamEntry * p_entry = (QuotParamEntry *)QpList.at(id-1);
		pQuot->Kind  = p_entry->QkID;
		pQuot->LocID = p_entry->LocID;
		pQuot->ArID  = p_entry->ArID;
		pQuot->CurID = p_entry->CurID;
		return 1;
	}
	else
		return 0;
}

int SLAPI Quot2Cache::BelongToMatrix(Quotation2Core * pTbl, PPID goodsID, PPID locID)
{
	int    ok = -1;
	{
		//RwL.ReadLock();
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		if(MtxQkID) {
			const  PPIDArray * p_vect = 0;
			uint   pos = 0;
			if(Matrix.lsearch(&locID, &pos, CMPF_LONG)) {
				ok = Matrix.at(pos)->Vect.bsearch(labs(goodsID));
			}
			else {
				assert(pTbl);
				if(pTbl) {
					//RwL.Unlock();
					//RwL.WriteLock();
					SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
					if(Matrix.lsearch(&locID, &(pos = 0), CMPF_LONG)) {
						ok = Matrix.at(pos)->Vect.bsearch(labs(goodsID));
					}
					else {
						MatrixEntry * p_entry = new MatrixEntry;
						p_entry->LocID = locID;
						pTbl->GetMatrix(MtxQkID, locID, &p_entry->Vect);
						Matrix.insert(p_entry);
						ok = p_entry->Vect.bsearch(labs(goodsID));
					}
				}
			}
		}
		//RwL.Unlock();
	}
	return ok;
}

int SLAPI Quotation2Core::FetchList(PPID goodsID, PPQuotArray & rList)
{
	int    ok = 0;
	Quot2Cache * p_cache = GetDbLocalCachePtr <Quot2Cache> (PPOBJ_QUOT2);
	if(p_cache) {
		rList.GoodsID = goodsID;
		{
			//p_cache->Lock();
			SRWLOCKER(p_cache->RwL, SReadWriteLocker::Write);
			p_cache->SetTable(this);
			const PPID qk_arg = 0;
			ok = p_cache->Get_(goodsID, qk_arg, 0, &rList);
			p_cache->SetTable(0);
			//p_cache->Unlock();
		}
	}
	else
		ok = -1;
	return ok;
}

int SLAPI Quotation2Core::GetMatrix(PPID locID, PPIDArray * pResult)
{
	Quot2Cache * p_cache = GetDbLocalCachePtr <Quot2Cache> (PPOBJ_QUOT2);
	return p_cache ? p_cache->Implement_GetMatrix(2, this, p_cache->RwL, locID, pResult) : -1;
}

int SLAPI Quotation2Core::BelongToMatrix(PPID goodsID, PPID locID)
{
	Quot2Cache * p_cache = GetDbLocalCachePtr <Quot2Cache> (PPOBJ_QUOT2);
	return p_cache ? p_cache->BelongToMatrix(this, goodsID, locID) : -1;
}

int SLAPI Quotation2Core::DirtyGoodsID(PPID goodsID)
{
	int    ok = -1;
	Quot2Cache * p_cache = GetDbLocalCachePtr <Quot2Cache> (PPOBJ_QUOT2, 0);
	if(p_cache) {
		p_cache->Dirty_(goodsID);
		ok = 1;
	}
	return ok;
}

int SLAPI Quotation2Core::ClearCache()
{
	int    ok = -1;
	Quot2Cache * p_cache = GetDbLocalCachePtr <Quot2Cache> (PPOBJ_QUOT2, 0);
	if(p_cache) {
		p_cache->Clear_();
		ok = 1;
	}
	return ok;
}

int SLAPI Quotation2Core::DirtyMatrix(const PPIDArray * pGoodsList, PPIDArray * pMtxLocList, int deferred)
{
	int    ok = -1;
	Quot2Cache * p_cache = GetDbLocalCachePtr <Quot2Cache> (PPOBJ_QUOT2, 0);
	if(p_cache) {
		//p_cache->Lock();
		SRWLOCKER(p_cache->RwL, SReadWriteLocker::Write);
		p_cache->SetTable(this);
		p_cache->Implement_DirtyMatrix(pGoodsList, pMtxLocList, deferred);
		p_cache->SetTable(0);
		//p_cache->Unlock();
		ok = 1;
	}
	return ok;
}
