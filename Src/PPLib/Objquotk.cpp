// OBJQUOTK.CPP
// Copyright (c) A.Sobolev 1998-2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>

PPQuotKind2::PPQuotKind2()
{
	THISZERO();
}

int SLAPI PPQuotKind2::HasWeekDayRestriction() const
{
	return BIN(DaysOfWeek && ((DaysOfWeek & 0x7f) != 0x7f));
}

int SLAPI PPQuotKind2::CheckWeekDay(LDATE dt) const
{
	return (!DaysOfWeek || !dt || (DaysOfWeek & (1 << (dayofweek(&dt, 1)-1))));
}

int SLAPI PPQuotKind2::SetTimeRange(const TimeRange & rRange)
{
	BeginTm = 0;
	EndTm = 0;
	if(rRange.low) {
		PTR8(&BeginTm)[0] = rRange.low.hour();
		PTR8(&BeginTm)[1] = rRange.low.minut();
	}
	if(rRange.upp) {
		PTR8(&EndTm)[0] = rRange.upp.hour();
		PTR8(&EndTm)[1] = rRange.upp.minut();
	}
	return 1;
}

int SLAPI PPQuotKind2::GetTimeRange(TimeRange & rRange) const
{
	rRange.low = ZEROTIME;
	rRange.upp = ZEROTIME;
	if(BeginTm) {
		int    h = PTR8(&BeginTm)[0];
		int    m = PTR8(&BeginTm)[1];
		if(h >= 0 && h <= 23 && m >= 0 && m <= 59)
			rRange.low = encodetime(h, m, 0, 0);
	}
	if(EndTm) {
		int    h = PTR8(&EndTm)[0];
		int    m = PTR8(&EndTm)[1];
		if(h >= 0 && h <= 23 && m >= 0 && m <= 59)
			rRange.upp = encodetime(h, m, 0, 0);
	}
	if(rRange.upp == 0 && rRange.low)
		rRange.upp = encodetime(23, 59, 59, 0);
	/* @v7.0.6
	if(rRange.low > rRange.upp)
		memswap(&rRange.low, &rRange.upp, sizeof(rRange.low));
	*/
	return (rRange.low || rRange.upp) ? 1 : -1;
}

int SLAPI PPQuotKind2::GetAmtRange(RealRange * pRange) const
{
	if(AmtRestr.IsZero()) {
		CALLPTRMEMB(pRange, SetVal(0.0));
		return -1;
	}
	else {
		if(pRange) {
			pRange->low = intmnytodbl(AmtRestr.low);
			pRange->upp = intmnytodbl(AmtRestr.upp);
		}
		return 1;
	}
}

int SLAPI PPQuotKind2::SetAmtRange(const RealRange * pRange)
{
	if(pRange && !pRange->IsZero()) {
		AmtRestr.low = dbltointmny(pRange->low);
		AmtRestr.upp = dbltointmny(pRange->upp);
		return 1;
	}
	else {
		AmtRestr = (int)0;
		return -1;
	}
}

int SLAPI PPQuotKind2::GetRestrText(SString & rBuf) const
{
	SString temp_buf;
	rBuf = 0;
	if(!Period.IsZero()) {
		rBuf.Cat(Period, 1);
	}
	TimeRange tr;
	if(GetTimeRange(tr) > 0) {
		rBuf.CatDiv(';', 2, 1);
		if(tr.low)
			rBuf.Cat(tr.low, TIMF_HM);
		rBuf.CatCharN('.', 2);
		if(tr.upp)
			rBuf.Cat(tr.upp, TIMF_HM);
	}
	if(DaysOfWeek) {
		rBuf.CatDiv(';', 2, 1);
		int first_dow = 1;
		for(uint i = 0; i < 7; i++) {
			if(DaysOfWeek & (1 << i)) {
				GetDayOfWeekText(2, i+1, temp_buf);
				if(!first_dow)
					rBuf.Space();
				rBuf.Cat(temp_buf);
				first_dow = 0;
			}
		}
	}
	RealRange amt_restr;
	if(GetAmtRange(&amt_restr) > 0) {
		if(amt_restr.low != amt_restr.upp || amt_restr.low != 0.0) {
			long   flags = NMBF_NOZERO|NMBF_TRICOMMA;
			SETSFMTPRC(flags, 2);
			rBuf.CatDiv(';', 2, 1);
			rBuf.Cat(amt_restr.low, flags);
			if(amt_restr.upp != amt_restr.low)
				rBuf.CatCharN('.', 2).Cat(amt_restr.upp, flags);
		}
	}
	return rBuf.NotEmpty() ? 1 : -1;
}
//
//
//
SLAPI PPQuotKindPacket::PPQuotKindPacket()
{
	Init();
}

int SLAPI PPQuotKindPacket::Init()
{
	MEMSZERO(Rec);
	return 1;
}

int SLAPI PPQuotKindPacket::GetCalculatedQuot(double cost, double basePrice, double * pQuot, long * pFlags) const
{
	int    ok = -1;
	double quot = 0.0;
	double discount = Rec.Discount;
	long   flags    = Rec.Flags;
	if(basePrice > 0.0 || (cost && flags & QUOTKF_PCTDISONCOST)) {
		if(discount || (flags & (QUOTKF_ABSDIS | QUOTKF_PCTDISONCOST))) {
			if(flags & QUOTKF_ABSDIS)
				quot = basePrice - discount;
			else if(flags & QUOTKF_PCTDISONCOST)
				quot = cost - fdiv100r(cost * discount);
			else
				quot = basePrice - fdiv100r(basePrice * discount);
			quot = RoundUpPrice(quot);
			ok = 1;
		}
	}
	ASSIGN_PTR(pQuot, quot);
	ASSIGN_PTR(pFlags, flags);
	return ok;
}

PPQuotKindPacket & FASTCALL PPQuotKindPacket::operator = (const PPQuotKindPacket & aPack)
{
	Rec = aPack.Rec;
	return *this;
}
//
//
//
PPObjQuotKind::Special::Special()
{
	THISZERO();
}

int FASTCALL PPObjQuotKind::Special::IsSupplDealKind(PPID qkID) const
{
	return oneof3(qkID, SupplDealID, SupplDevUpID, SupplDevDnID);
}

int FASTCALL PPObjQuotKind::Special::GetCategory(PPID qkID) const
{
	if(qkID == 0)
		return PPQC_UNKN;
	else if(qkID == MtxID)
		return PPQC_MATRIX;
	else if(qkID == MtxRestrID)
		return PPQC_MATRIXRESTR;
	else if(qkID == PredictCoeffID)
		return PPQC_PREDICTCOEFF;
	else if(oneof3(qkID, SupplDealID, SupplDevUpID, SupplDevDnID))
		return PPQC_SUPPLDEAL;
	else
		return PPQC_PRICE;
}

//static
PPID SLAPI PPObjQuotKind::GetDefaultAccSheetID(int cls)
{
	return (cls == PPQuot::clsSupplDeal) ? GetSupplAccSheet() : ((cls == PPQuot::clsGeneral) ? GetSellAccSheet() : 0);
}

SLAPI PPObjQuotKind::PPObjQuotKind(void * extraPtr) : PPObjReference(PPOBJ_QUOTKIND, extraPtr)
{
	ImplementFlags |= implStrAssocMakeList;
}

int SLAPI PPObjQuotKind::Classify(PPID id, int * pCls)
{
	int    ok = -1;
	int    cls = PPQuot::clsGeneral;
	PPQuotKind rec;
	if(id && Fetch(id, &rec) > 0) {
		PPObjQuotKind::Special spc;
		PPObjQuotKind::GetSpecialKinds(&spc, 1);
		if(oneof2(id, spc.MtxID, spc.MtxRestrID))
			cls = PPQuot::clsMtx;
		else if(oneof3(id, spc.SupplDealID, spc.SupplDevUpID, spc.SupplDevDnID))
			cls = PPQuot::clsSupplDeal;
		else if(id == spc.PredictCoeffID)
			cls = PPQuot::clsPredictCoeff;
		else
			cls = PPQuot::clsGeneral;
		ok = 1;
	}
	ASSIGN_PTR(pCls, cls);
	return ok;
}

int SLAPI PPObjQuotKind::MakeReserved(long flags)
{
	int    ok = 1;
	uint   num_recs, i;
	SString name, symb;
	TVRez * p_rez = P_SlRez;
	THROW_PP(p_rez, PPERR_RESFAULT);
	THROW_PP(p_rez->findResource(ROD_QUOTKIND, PP_RCDATA), PPERR_RESFAULT);
	THROW_PP(num_recs = p_rez->getUINT(), PPERR_RESFAULT);
	for(i = 0; i < num_recs; i++) {
		PPQuotKindPacket pack;
		PPQuotKind temp_rec;
		PPID   id = p_rez->getUINT();
		p_rez->getString(name, 2);
		PPExpandString(name, CTRANSF_UTF8_TO_INNER); // @v9.2.1
		p_rez->getString(symb, 2);
		pack.Rec.ID = id;
		name.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
		symb.CopyTo(pack.Rec.Symb, sizeof(pack.Rec.Symb));
		if(id && Search(id, &temp_rec) <= 0 && SearchBySymb(symb, 0, &temp_rec) <= 0) {
			//
			// «десь нельз€ использовать PutPacket поскольку добавл€етс€ запись
			// с предопределенным идентификатором.
			//
			THROW(EditItem(Obj, 0, &pack.Rec, 1));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjQuotKind::GetListByOp(PPID opID, LDATE dt, PPIDArray * pList)
{
	int    intrexpnd = IsIntrExpndOp(opID);
	PPObjGoods goods_obj;
	const  PPID matrix_qk_id = goods_obj.GetConfig().MtxQkID;
	{
		SArray list(sizeof(PPQuotKind));
		ref->LoadItems(Obj, &list);
		PPQuotKind * p_item;
		for(uint i = 0; list.enumItems(&i, (void **)&p_item);) {
			const PPID id = p_item->ID;
			if(ObjRts.CheckQuotKindID(id, 0)) { // @v8.9.7
				//
				// ≈сли котировка не матрична€ » ("не дл€ документов" либо нам это безразлично)
				//
				if(id != matrix_qk_id && !(p_item->Flags & QUOTKF_NOTFORBILL) && p_item->Period.CheckDate(dt)) {
					//
					// ≈сли дл€ вида котировки определен вид операции, то безусловно провер€ем
					// соответствие нашей операции этому виду.
					//
					if(p_item->OpID) {
						if(IsOpBelongTo(opID, p_item->OpID) > 0)
							pList->addUnique(id);
					}
					else {
						//
						// ¬ противном случае, дл€ внутренней передачи используетс€ только базова€ котировка
						//
						if(intrexpnd) {
							if(id == PPQUOTK_BASE)
								pList->addUnique(id);
						}
						else {
							//
							// ќстальные виды операций безусловно могут использовать этот вид котировки
							//
							pList->addUnique(id);
						}
					}
				}
			}
		}
	}
	return pList->getCount() ? 1 : -1;
}

//IMPL_CMPFUNC(RankNName, i1, i2);

IMPL_CMPFUNC(PPQuotKind_RankName, i1, i2)
{
	int    cmp = 0;
	PPQuotKind * k1 = (PPQuotKind *)i1;
	PPQuotKind * k2 = (PPQuotKind *)i2;
	if(k1->ID == PPQUOTK_BASE && k2->ID != PPQUOTK_BASE)
		cmp = -1;
	else if(k1->ID != PPQUOTK_BASE && k2->ID == PPQUOTK_BASE)
		cmp = 1;
	else if(k1->Rank < k2->Rank)
		cmp = 1;
	else if(k1->Rank > k2->Rank)
		cmp = -1;
	else
		cmp = stricmp866(k1->Name, k2->Name);
	return cmp;
}

int SLAPI PPObjQuotKind::ArrangeList(const LDATETIME & rDtm, PPIDArray & rQkList, long flags)
{
	int    ok = 1;
	const  uint c = rQkList.getCount();
	if(c) {
		uint i;
		SArray qk_list(sizeof(PPQuotKind));
		for(i = 0; i < c; i++) {
			PPQuotKind qk_rec;
			const PPID qk_id = rQkList.get(i);
			if(Fetch(qk_id, &qk_rec) > 0) {
				int    suited = 1;
				TimeRange tmr;
				int    trcr = 0;
				if(rDtm.t && qk_rec.GetTimeRange(tmr) > 0 && !(flags & RTLPF_USEQUOTWTIME && (trcr = tmr.Check(rDtm.t)) != 0))
					suited = 0;
				else if(rDtm.d && !qk_rec.Period.IsZero()) {
					if(!qk_rec.Period.CheckDate(rDtm.d))
						suited = 0;
					else if(qk_rec.HasWeekDayRestriction()) {
						if(trcr == 2) {
							if(!qk_rec.CheckWeekDay(plusdate(rDtm.d, -1)))
								suited = 0;
						}
						else {
							if(!qk_rec.CheckWeekDay(rDtm.d))
								suited = 0;
						}
					}
				}
				if(suited)
					qk_list.insert(&qk_rec);
			}
		}
		qk_list.sort(PTR_CMPFUNC(PPQuotKind_RankName));
		rQkList.clear();
		for(i = 0; i < qk_list.getCount(); i++)
			rQkList.addUnique(((PPQuotKind *)qk_list.at(i))->ID);
	}
	return ok;
}

int SLAPI PPObjQuotKind::Helper_GetRtlList(const LDATETIME & rDtm, PPIDArray * pList, PPIDArray * pTmList, long flags)
{
	int    ok = -1;
	if(pList || pTmList) {
		SArray qk_list(sizeof(PPQuotKind));
		PPIDArray tm_list;
		const PPID sell_acc_sheet = GetSellAccSheet();
		PPQuotKind  qkr;
		for(SEnum en = ref->Enum(Obj, 0); en.Next(&qkr) > 0;) {
			int    suited = 0;
			int    is_tm = 0;
			if(qkr.ID == PPQUOTK_BASE && flags & RTLPF_PRICEBYQUOT)
				suited = 1;
			else if(qkr.Flags & QUOTKF_RETAILED && (!qkr.AccSheetID || qkr.AccSheetID == sell_acc_sheet)) {
				suited = 1;
				TimeRange tmr;
				if(!qkr.Period.IsZero()) {
					is_tm = 1;
					if(rDtm.d && !qkr.Period.CheckDate(rDtm.d))
						suited = 0;
				}
				if(qkr.GetTimeRange(tmr) > 0) {
					is_tm = 1;
					int    trcr = 0;
					if(rDtm.t && !(flags & RTLPF_USEQUOTWTIME && (trcr = tmr.Check(rDtm.t)) != 0))
						suited = 0;
				}
			}
			if(suited) {
				qk_list.insert(&qkr);
				if(is_tm && pTmList)
					tm_list.add(qkr.ID);
			}
		}
		if(pList) {
			qk_list.sort(PTR_CMPFUNC(PPQuotKind_RankName));
			pList->clear();
			for(uint i = 0; i < qk_list.getCount(); i++)
				pList->addUnique(((PPQuotKind *)qk_list.at(i))->ID);
		}
		ASSIGN_PTR(pTmList, tm_list);
		ok = 1;
	}
	return  ok;
}

int SLAPI PPObjQuotKind::GetRetailQuotList(LDATETIME dtm, PPIDArray * pList, long flags)
{
	int    ok = 1;
	if(flags & RTLPF_USEQKCACHE) {
		PPQuotKind qk_rec;
		PPIDArray list, tm_list;
		FetchRtlList(list, tm_list);
		if(flags & RTLPF_PRICEBYQUOT) {
			if(Fetch(PPQUOTK_BASE, &qk_rec) > 0) {
				PPID   id = PPQUOTK_BASE;
				list.atInsert(0, &id);
			}
		}
		const uint tmc = tm_list.getCount();
		if(tmc && (dtm.d || dtm.t)) {
			for(uint i = 0; i < tmc; i++) {
				int    suited = 1;
				if(Fetch(tm_list.get(i), &qk_rec) > 0) {
					TimeRange tmr;
					int    trcr = 0;
					if(qk_rec.GetTimeRange(tmr) > 0 && dtm.t && !(flags & RTLPF_USEQUOTWTIME && (trcr = tmr.Check(dtm.t)) != 0))
						suited = 0;
					else if(dtm.d) {
						if(!qk_rec.Period.IsZero() && !qk_rec.Period.CheckDate(dtm.d))
							suited = 0;
						else if(qk_rec.HasWeekDayRestriction()) {
							if(trcr == 2) {
								if(!qk_rec.CheckWeekDay(plusdate(dtm.d, -1)))
									suited = 0;
							}
							else {
								if(!qk_rec.CheckWeekDay(dtm.d))
									suited = 0;
							}
						}
					}
				}
				else
					suited = 0;
				if(!suited)
					list.freeByKey(qk_rec.ID, 0);
			}
		}
		ASSIGN_PTR(pList, list);
	}
	else
		ok = Helper_GetRtlList(dtm, pList, 0, (flags & ~RTLPF_USEQKCACHE));
	return ok;
}

int SLAPI PPObjQuotKind::GetCalculatedQuot(PPID id, double cost, double basePrice, double * pQuot, long * pFlags)
{
	PPQuotKindPacket pack;
	return (Fetch(id, &pack.Rec) > 0) ? pack.GetCalculatedQuot(cost, basePrice, pQuot, pFlags) : 0;
}

int SLAPI PPObjQuotKind::IsPacketEq(const PPQuotKindPacket & rS1, const PPQuotKindPacket & rS2, long flags)
{
#define CMP_MEMB(m)  if(rS1.Rec.m != rS2.Rec.m) return 0;
#define CMP_MEMBS(m) if(strcmp(rS1.Rec.m, rS2.Rec.m) != 0) return 0;
	CMP_MEMB(ID);
	CMP_MEMBS(Name);
	CMP_MEMBS(Symb);
	CMP_MEMB(SerialTerm);
	CMP_MEMB(AmtRestr);
	CMP_MEMB(Discount);
	CMP_MEMB(Period);
	CMP_MEMB(BeginTm);
	CMP_MEMB(EndTm);
	CMP_MEMB(Rank);
	CMP_MEMB(DaysOfWeek);
	CMP_MEMB(UsingWSCard);
	CMP_MEMB(Flags);
	CMP_MEMB(OpID);
	CMP_MEMB(AccSheetID);
#undef CMP_MEMBS
#undef CMP_MEMB
	return 1;
}

int SLAPI PPObjQuotKind::GetPacket(PPID id, PPQuotKindPacket * pPack)
{
	int    ok = 1;
	PPQuotKindPacket pack;
	THROW(Search(id, &pack.Rec) > 0);
	ASSIGN_PTR(pPack, pack);
	CATCHZOK
	return ok;
}

int SLAPI PPObjQuotKind::PutPacket(PPID * pID, PPQuotKindPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				PPQuotKindPacket org_pack;
				THROW(GetPacket(*pID, &org_pack) > 0); // @v8.3.6
				if(!IsPacketEq(*pPack, org_pack, 0)) { // @v8.3.6
					THROW(CheckDupName(*pID, pPack->Rec.Name));
					THROW(CheckRights(PPR_MOD)); // @v7.9.5
					THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
				}
			}
			else {
				THROW(CheckRights(PPR_DEL)); // @v7.9.5
				{
					PPObjGoods goods_obj;
					THROW(goods_obj.P_Tbl->RemoveAllQuotForQuotKind(*pID, 0));
					THROW(ref->RemoveItem(Obj, *pID, 0));
					THROW(RemoveSync(*pID));
				}
			}
		}
		else {
			THROW(CheckRights(PPR_INS)); // @v7.9.5
			THROW(CheckDupName(*pID, pPack->Rec.Name)); // @v8.3.6
			*pID = pPack->Rec.ID;
			THROW(ref->AddItem(Obj, pID, &pPack->Rec, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

// virtual
//int  SLAPI PPObjQuotKind::Remove(PPID id, long, uint options /* = rmv_default */)
//virtual
int  SLAPI PPObjQuotKind::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options/* = rmv_default*/, void * pExtraParam)
{
	int    r = -1;
	THROW(CheckRights(PPR_DEL));
	// @v9.2.9 Ѕлокировка возможности удалени€ специальных видов котировок {
	if(id) {
		PPObjQuotKind::Special spc;
		PPObjQuotKind::GetSpecialKinds(&spc, 0);
		const char * p_spcqk_symb = 0;
		if(id == spc.MtxID)
			p_spcqk_symb = "qkspc_goodsmatrix";
		else if(id == spc.MtxRestrID)
			p_spcqk_symb = "qkspc_goodsmatrixrestr";
		else if(id == spc.PredictCoeffID)
			p_spcqk_symb = "qkspc_predictcoeff";
		else if(id == spc.SupplDealID)
			p_spcqk_symb = "qkspc_suppldeal";
		else if(id == spc.SupplDevDnID)
			p_spcqk_symb = "qkspc_suppldealdown";
		else if(id == spc.SupplDevUpID)
			p_spcqk_symb = "qkspc_suppldealup";
		if(p_spcqk_symb) {
			SString spc_qk_text;
			PPLoadString(p_spcqk_symb, spc_qk_text);
			THROW_PP_S(!p_spcqk_symb, PPERR_UNABLERMVSPCQUOTKIND, spc_qk_text);
		}
	}
	// } @v9.2.9
	if(!(options & PPObject::user_request) || PPMessage(mfConf|mfYesNo, PPCFM_DELQUOTKIND, 0) == cmYes) {
		int    use_ta = (options & PPObject::use_transaction) ? 1 : 0;
		PPObjGoods goods_obj;
		PPWait(1);
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(goods_obj.P_Tbl->RemoveAllQuotForQuotKind(id, 0));
			THROW(ref->RemoveItem(Obj, id, 0));
			THROW(RemoveSync(id));
			THROW(tra.Commit());
		}
		r = 1;
	}
	CATCH
		r = 0;
		if(options & PPObject::user_request)
			PPError();
	ENDCATCH
	PPWait(0);
	return r;
}

int SLAPI PPObjQuotKind::SearchSymb(PPID * pID, const char * pSymb)
{
	return ref->SearchSymb(Obj, pID, pSymb, offsetof(PPQuotKind, Symb));
}

StrAssocArray * SLAPI PPObjQuotKind::MakeStrAssocList(void * extraPtr)
{
	StrAssocArray * p_list = new StrAssocArray;
	QuotKindFilt filt;
	filt.Flags = (long)extraPtr;
	MakeList(&filt, p_list);
	return p_list;
}

IMPL_CMPFUNC(_QUOTK_LIST_ENTRY, i1, i2)
{
	return stricmp866(((PPObjQuotKind::ListEntry *)i1)->Name, ((PPObjQuotKind::ListEntry *)i2)->Name);
}

IMPL_CMPFUNC(PPQuotKind, i1, i2)
{
	PPQuotKind * p1 = (PPQuotKind *)i1;
	PPQuotKind * p2 = (PPQuotKind *)i2;
	if(p1->Rank < p2->Rank)
		return +1;
	else if(p1->Rank > p2->Rank)
		return -1;
	else
		return stricmp866(p1->Name, p2->Name);
}

SArray * SLAPI PPObjQuotKind::MakeListByIDList(const PPIDArray * pList)
{
	PPObjQuotKind::ListEntry entry;
	PPID   id = 0;
	uint   i;
	SArray * p_ary = 0;
	THROW_MEM(p_ary = new SArray(sizeof(PPObjQuotKind::ListEntry)));
	for(i = 0; i < pList->getCount(); i++) {
		PPQuotKind qk_rec;
		PPID   qk_id = pList->at(i);
		int    r;
		if((r = Fetch(qk_id, &qk_rec)) > 0 || qk_id == PPQUOTK_BASE) {
			MEMSZERO(entry);
			entry.ID = qk_id;
			if(r < 0)
				PPGetWord(PPWORD_BASEQUOT, 0, entry.Name, sizeof(entry.Name));
			else
				STRNSCPY(entry.Name, qk_rec.Name);
			THROW_SL(p_ary->insert(&entry));
		}
	}
	p_ary->sort(PTR_CMPFUNC(_QUOTK_LIST_ENTRY));
	CATCH
		ZDELETE(p_ary);
	ENDCATCH
	return p_ary;
}

SLAPI QuotKindFilt::QuotKindFilt()
{
	THISZERO();
}

int SLAPI PPObjQuotKind::MakeList(const QuotKindFilt * pFilt, StrAssocArray * pList)
{
	int    ok = -1, i;
	SArray * p_ary = 0;
	PPIDArray id_list;
	PPQuotKind qk_rec;
	SArray rec_list(sizeof(PPQuotKind));
	PPObjQuotKind::Special spc;
	GetSpecialKinds(&spc, 1);
	if(pFilt->Flags & QuotKindFilt::fSupplDeal) {
		if(Search(spc.SupplDealID, &qk_rec) > 0)
			rec_list.insert(&qk_rec);
		if(Search(spc.SupplDevUpID, &qk_rec) > 0)
			rec_list.insert(&qk_rec);
		if(Search(spc.SupplDevDnID, &qk_rec) > 0)
			rec_list.insert(&qk_rec);
	}
	else if(pFilt->Flags & (QuotKindFilt::fGoodsMatrix|QuotKindFilt::fGoodsMatrixRestrict)) {
		const int is_matrix = BIN(pFilt->Flags & QuotKindFilt::fGoodsMatrix);
		PPGoodsConfig goods_cfg;
		PPObjGoods::ReadConfig(&goods_cfg);
		if(Search((is_matrix ? goods_cfg.MtxQkID : goods_cfg.MtxRestrQkID), &qk_rec) > 0)
			rec_list.insert(&qk_rec);
	}
	else if(pFilt->Flags & QuotKindFilt::fPredictCoeff) {
		if(Search(spc.PredictCoeffID, &qk_rec) > 0)
			rec_list.insert(&qk_rec);
	}
	else {
		int    intrexpnd = pFilt->OpID ? IsIntrExpndOp(pFilt->OpID) : 0;
		THROW(ref->LoadItems(Obj, &rec_list));
		if(pFilt->Flags & QuotKindFilt::fAddBase || intrexpnd) {
			const PPID base_id = PPQUOTK_BASE;
			if(!rec_list.lsearch(&base_id, 0, CMPF_LONG, offsetof(PPQuotKind, ID))) {
				MEMSZERO(qk_rec);
				qk_rec.ID = PPQUOTK_BASE;
				PPGetWord(PPWORD_BASEQUOT, 0, qk_rec.Name, sizeof(qk_rec.Name));
				rec_list.insert(&qk_rec);
			}
		}
		rec_list.sort((pFilt->Flags & QuotKindFilt::fSortByRankName) ? PTR_CMPFUNC(PPQuotKind_RankName) : PTR_CMPFUNC(PPQuotKind));
		for(i = rec_list.getCount()-1; i >= 0; i--) {
			PPQuotKind * p_rec = (PPQuotKind *)rec_list.at(i);
			if(!(pFilt->Flags & QuotKindFilt::fAll) && spc.IsSupplDealKind(p_rec->ID))
				rec_list.atFree(i);
			else if((p_rec->Flags & QUOTKF_NOTFORBILL) && (pFilt->Flags & QuotKindFilt::fExclNotForBill))
				rec_list.atFree(i);
			else if(pFilt->OpID) {
				if(intrexpnd) {
					if(p_rec->ID != PPQUOTK_BASE && p_rec->OpID && IsOpBelongTo(pFilt->OpID, p_rec->OpID) <= 0)
						rec_list.atFree(i);
				}
				else if(p_rec->OpID && IsOpBelongTo(pFilt->OpID, p_rec->OpID) <= 0)
					rec_list.atFree(i);
			}
		}
		if(pFilt->MaxItems > 0)
			while(rec_list.getCount() > (uint)pFilt->MaxItems)
				rec_list.freeLast();
	}
	// @v8.9.4 {
	if(pFilt && !(pFilt->Flags & QuotKindFilt::fIgnoreRights)) {
		uint   c = rec_list.getCount();
        if(c) do {
			const PPID qk_id = ((PPQuotKind *)rec_list.at(--c))->ID;
			if(!ObjRts.CheckQuotKindID(qk_id, 0))
				rec_list.atFree(c);
        } while(c);
	}
	// } @v8.9.4
	if(pList) {
		PPQuotKind * p_rec;
		for(uint j = 0; rec_list.enumItems(&j, (void **)&p_rec);)
			THROW_SL(pList->Add(p_rec->ID, p_rec->Name));
	}
	ok = rec_list.getCount() ? 1 : -1;
	CATCHZOK
	return ok;
}
// } AHTOXA

static void SetDiscount(TDialog * pDlg, uint ctl, const PPQuotKind * pRec)
{
	SString buf;
	double dis = R2(pRec->Discount);
	buf.Cat(dis, MKSFMTD(18, 2, NMBF_NOZERO));
	if(dis != 0 || (pRec->Flags & (QUOTKF_ABSDIS | QUOTKF_PCTDISONCOST))) {
		if(pRec->Flags & QUOTKF_ABSDIS)
			buf.CatChar('$');
		else if(pRec->Flags & QUOTKF_PCTDISONCOST)
			buf.CatChar('C');
		else if(dis != 0)
			buf.CatChar('%');
	}
	pDlg->setCtrlString(ctl, buf);
}

static int GetDiscount(TDialog * pDlg, uint ctl, PPQuotKind * pRec)
{
	int    ok = 1, pctdis = 0, absdis = 0, pctdisoncost = 0;
	double v = 0.0;
	char   buf[32];
	if(pDlg->getCtrlData(ctl, buf)) {
		SString pattern;
		(pattern = "Cc—с").Transf(CTRANSF_OUTER_TO_INNER);
		strip(buf);
		char * p = strpbrk(buf, "%/");
		if(p) {
			pctdis = 1;
			strcpy(p, p + 1);
		}
		else if((p = strpbrk(buf, pattern)) != 0) {
			pctdisoncost = 1;
			strcpy(p, p + 1);
		}
		else if((p = strchr(buf, '$')) != 0) {
			absdis = 1;
			strcpy(p, p + 1);
		}
		p = buf + strlen(buf);
		if(pctdis)
			*p++ = '%';
		else if(pctdisoncost)
			*p++ = 'C';
		else
			*p++ = '$';
		*p = 0;
		strtodoub(buf, &v);
		v = R2(v);
		if(v == 0 && !absdis)
			buf[0] = 0;
		pDlg->setCtrlData(ctl, buf);
	}
	else
		ok = 0;
	SETFLAG(pRec->Flags, QUOTKF_ABSDIS, absdis || (v && !pctdis && !pctdisoncost));
	if(!(pRec->Flags & QUOTKF_ABSDIS))
		SETFLAG(pRec->Flags, QUOTKF_PCTDISONCOST, pctdisoncost);
	pRec->Discount = v;
	return ok;
}

class QuotKindDialog : public TDialog {
public:
	QuotKindDialog(PPObjReference * pRef) : TDialog(DLG_QUOTKIND)
	{
		P_Ref = pRef;
	}
 	int    setDTS(const PPQuotKindPacket * pPack);
	int    getDTS(PPQuotKindPacket * pPack);
private:
	DECL_HANDLE_EVENT;
	void   UpdateView();
	void   SetTimePeriod(TDialog * pDlg);
	int    GetTimePeriod(TDialog * pDlg);
	int    EditRestr();

	PPObjReference * P_Ref;
	PPQuotKindPacket Data;
};

void QuotKindDialog::SetTimePeriod(TDialog * pDlg)
{
	SString  temp_buf;
	TimeRange tmr;
	Data.Rec.GetTimeRange(tmr);
	if(tmr.low)
		temp_buf.Cat(tmr.low, TIMF_HM);
	if(tmr.upp && tmr.upp != tmr.low)
		temp_buf.CatCharN('.', 2).Cat(tmr.upp, TIMF_HM);
	//setCtrlString(CTL_QUOTKIND_TIMEPERIOD, temp_buf);
	pDlg->setCtrlString(CTL_QKRESTR_TIMERANGE, temp_buf);
}

int QuotKindDialog::GetTimePeriod(TDialog * pDlg)
{
	int    ok = 1;
	char   buf[32];
	//getCtrlData(CTL_QUOTKIND_TIMEPERIOD, buf);
	pDlg->getCtrlData(CTL_QKRESTR_TIMERANGE, buf);
	if(buf[0]) {
		char * p = 0;
		LTIME  beg_tm, end_tm;
		THROW((p = strchr(strip(buf), '.')) != 0 && p[1] == '.');
		*p = 0;
		strtotime(buf, 0, &beg_tm);
		THROW(((beg_tm.hour() >= 0 && beg_tm.hour() <= 23) || (beg_tm.hour() == 24 && beg_tm.minut() == 0)) &&
			beg_tm.minut() >= 0 && beg_tm.minut() <= 60);
		strtotime(p + 2, 0, &end_tm);
		THROW(((end_tm.hour() >= 0 && end_tm.hour() <= 23) || (end_tm.hour() == 24 && end_tm.minut() == 0)) &&
			end_tm.minut() >= 0 && end_tm.minut() <= 60);
		if(end_tm == ZEROTIME)
			end_tm = encodetime(24, 0, 0, 0);
		// @v7.0.6 THROW(beg_tm.hour() < end_tm.hour() || (beg_tm.hour() == end_tm.hour() && beg_tm.minut() < end_tm.minut()));
		if(beg_tm == ZEROTIME && end_tm.hour() == 24)
			end_tm = ZEROTIME;
		p = (char *)&Data.Rec.BeginTm;
		*p = beg_tm.hour();
		*(p + 1) = beg_tm.minut();
		p = (char *)&Data.Rec.EndTm;
		*p = end_tm.hour();
		*(p + 1) = end_tm.minut();
	}
	else
		Data.Rec.BeginTm = Data.Rec.EndTm = 0;
	CATCH
		//ok = PPErrorByDialog(this, CTL_QUOTKIND_TIMEPERIOD, PPERR_INVTIMEINPUT);
		ok = PPErrorByDialog(pDlg, CTL_QKRESTR_TIMERANGE, PPERR_INVTIMEINPUT);
	ENDCATCH
	return ok;
}

int QuotKindDialog::EditRestr()
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_QUOTKINDRESTR);
	if(CheckDialogPtr(&dlg, 1)) {
		RealRange amt_range;
		dlg->SetupCalPeriod(CTLCAL_QKRESTR_PERIOD, CTL_QKRESTR_PERIOD);
		SetPeriodInput(dlg, CTL_QKRESTR_PERIOD, &Data.Rec.Period);
		Data.Rec.GetAmtRange(&amt_range);
		SetRealRangeInput(dlg, CTL_QKRESTR_AMTRANGE, &amt_range);

		dlg->AddClusterAssoc(CTL_QKRESTR_WEEKDAYS, 0, 0x0001);
		dlg->AddClusterAssoc(CTL_QKRESTR_WEEKDAYS, 1, 0x0002);
		dlg->AddClusterAssoc(CTL_QKRESTR_WEEKDAYS, 2, 0x0004);
		dlg->AddClusterAssoc(CTL_QKRESTR_WEEKDAYS, 3, 0x0008);
		dlg->AddClusterAssoc(CTL_QKRESTR_WEEKDAYS, 4, 0x0010);
		dlg->AddClusterAssoc(CTL_QKRESTR_WEEKDAYS, 5, 0x0020);
		dlg->AddClusterAssoc(CTL_QKRESTR_WEEKDAYS, 6, 0x0040);
		dlg->SetClusterData(CTL_QKRESTR_WEEKDAYS, Data.Rec.DaysOfWeek);
		SetTimePeriod(dlg);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			if(!GetPeriodInput(dlg, CTL_QKRESTR_PERIOD, &Data.Rec.Period)) {
				PPError();
			}
			else if(GetTimePeriod(dlg)) { // GetTimePeriod() @>> PPError()
				GetRealRangeInput(dlg, CTL_QKRESTR_AMTRANGE, &amt_range);
				Data.Rec.SetAmtRange(&amt_range);
				Data.Rec.DaysOfWeek = (uint8)dlg->GetClusterData(CTL_QKRESTR_WEEKDAYS);
				ok = 1;
			}
		}
		{
			SString restr_text;
			Data.Rec.GetRestrText(restr_text);
			setStaticText(CTL_QUOTKIND_ST_RESTR, restr_text);
		}
	}
	else
		ok = 0;
	return ok;
}

int QuotKindDialog::setDTS(const PPQuotKindPacket * pPack)
{
	PPIDArray  op_type_list;

	Data = *pPack;
	setCtrlData(CTL_QUOTKIND_NAME, Data.Rec.Name);
	setCtrlData(CTL_QUOTKIND_SYMB, Data.Rec.Symb);
	setCtrlData(CTL_QUOTKIND_ID,   &Data.Rec.ID);
	disableCtrl(CTL_QUOTKIND_ID, (int)Data.Rec.ID || !PPMaster);

	op_type_list.addzlist(PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, PPOPT_GOODSORDER,
		PPOPT_DRAFTEXPEND, PPOPT_GENERIC, 0);
	SetupOprKindCombo(this, CTLSEL_QUOTKIND_OP, Data.Rec.OpID, 0, &op_type_list, 0);
	SetupPPObjCombo(this, CTLSEL_QUOTKIND_ACCSHEET, PPOBJ_ACCSHEET, Data.Rec.AccSheetID, OLW_CANINSERT, 0);
	setCtrlData(CTL_QUOTKIND_RANK, &Data.Rec.Rank);
	SetDiscount(this, CTL_QUOTKIND_DISCOUNT, &Data.Rec);
	AddClusterAssoc(CTL_QUOTKIND_FLAGS, 0, QUOTKF_NOTFORBILL);
	AddClusterAssoc(CTL_QUOTKIND_FLAGS, 1, QUOTKF_EXTPRICEBYBASE);
	AddClusterAssoc(CTL_QUOTKIND_FLAGS, 2, QUOTKF_RETAILED);
	SetClusterData(CTL_QUOTKIND_FLAGS, Data.Rec.Flags);
	{
		SString restr_text;
		Data.Rec.GetRestrText(restr_text);
		setStaticText(CTL_QUOTKIND_ST_RESTR, restr_text);
	}
	UpdateView();
	return 1;
}

int QuotKindDialog::getDTS(PPQuotKindPacket * pPack)
{
	int    ok = 1;
	uint   ctl = 0;
	getCtrlData(CTL_QUOTKIND_NAME, Data.Rec.Name);
	ctl = CTL_QUOTKIND_NAME;
	THROW_PP(*strip(Data.Rec.Name) != 0, PPERR_NAMENEEDED);
	ctl = CTL_QUOTKIND_NAME;
	THROW(P_Ref->CheckDupName(Data.Rec.ID, Data.Rec.Name));
	ctl = CTL_QUOTKIND_SYMB;
	getCtrlData(CTL_QUOTKIND_SYMB,  Data.Rec.Symb);
	THROW(P_Ref->ref->CheckUniqueSymb(P_Ref->Obj, Data.Rec.ID, Data.Rec.Symb, offsetof(PPQuotKind, Symb)))
	getCtrlData(CTL_QUOTKIND_ID,    &Data.Rec.ID);
	getCtrlData(CTLSEL_QUOTKIND_OP, &Data.Rec.OpID);
	getCtrlData(CTLSEL_QUOTKIND_ACCSHEET, &Data.Rec.AccSheetID);
	GetClusterData(CTL_QUOTKIND_FLAGS, &Data.Rec.Flags);
	GetDiscount(this, CTL_QUOTKIND_DISCOUNT, &Data.Rec);
	getCtrlData(CTL_QUOTKIND_RANK,  &Data.Rec.Rank);
	ASSIGN_PTR(pPack, Data);
	CATCH
		ok = 0;
		selectCtrl(ctl);
	ENDCATCH
	return ok;
}

void QuotKindDialog::UpdateView()
{
	int   not_retailed = 0;
	getDTS(0);
	PPID  acs_id = Data.Rec.AccSheetID;
	int   not_sell_accsheet = BIN(acs_id && acs_id != GetSellAccSheet() && acs_id != GetAgentAccSheet());
	if(not_sell_accsheet) {
		Data.Rec.Flags &= ~QUOTKF_RETAILED;
		SetClusterData(CTL_QUOTKIND_FLAGS, Data.Rec.Flags);
	}
	DisableClusterItem(CTL_QUOTKIND_FLAGS, 2, not_sell_accsheet);
	not_retailed = (Data.Rec.Flags & QUOTKF_RETAILED) ? 0 : 1;
	if(not_retailed) {
		// @v7.4.0 Data.Rec.BeginTm = Data.Rec.EndTm = 0;
	}
	disableCtrl(CTL_QUOTKIND_PERIOD, Data.Rec.ID == PPQUOTK_BASE);
	ShowCalCtrl(CTLCAL_QUOTKIND_PERIOD, this, !(Data.Rec.ID == PPQUOTK_BASE));
	disableCtrl(CTL_QUOTKIND_TIMEPERIOD, Data.Rec.ID == PPQUOTK_BASE || not_retailed);
}

IMPL_HANDLE_EVENT(QuotKindDialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_QUOTKIND_FLAGS) || event.isCbSelected(CTLSEL_QUOTKIND_ACCSHEET))
		UpdateView();
	else if(event.isCmd(cmQuotKindRestr))
		EditRestr();
	else
		return;
	clearEvent(event);
}

int SLAPI PPObjQuotKind::Edit(PPID * pID, void * extraPtr)
{
	int    r = cmCancel, ok = 1, valid_data = 0, is_new = 0;
	PPQuotKindPacket pack;
	QuotKindDialog * p_dlg = 0;
	THROW(CheckDialogPtr(&(p_dlg = new QuotKindDialog(this))));
	THROW(EditPrereq(pID, p_dlg, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	else {
		THROW(pack.Init());
	}
	p_dlg->setDTS(&pack);
	while(!valid_data && (r = ExecView(p_dlg)) == cmOK) {
		if(p_dlg->getDTS(&pack) > 0) {
			valid_data = 1;
			if(*pID)
				*pID = pack.Rec.ID;
			THROW(PutPacket(pID, &pack, 1));
			if(!is_new)
				Dirty(*pID);
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok ? r : 0;
}

int SLAPI PPObjQuotKind::Browse(void * extraPtr)
{
	return RefObjView(this, PPDS_CRRQUOTKIND, 0);
}

int SLAPI PPObjQuotKind::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE && _obj == PPOBJ_OPRKIND) {
		int    r;
		for(PPID id = 0; ok == DBRPL_OK && (r = ref->EnumItems(Obj, &id)) > 0;)
			if(((PPQuotKind*)&ref->data)->OpID == _id)
				ok = RetRefsExistsErr(Obj, id);
		if(ok == DBRPL_OK && r == 0)
			ok = DBRPL_ERROR;
	}
	return DBRPL_OK;
}

IMPL_DESTROY_OBJ_PACK(PPObjQuotKind, PPQuotKindPacket);

int SLAPI PPObjQuotKind::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(p && p->Data) {
		PPQuotKindPacket * p_pack = (PPQuotKindPacket *)p->Data;
		THROW(ProcessObjRefInArray(PPOBJ_OPRKIND,  &p_pack->Rec.OpID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_ACCSHEET, &p_pack->Rec.AccSheetID, ary, replace));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjQuotKind::SerializePacket(int dir, PPQuotKindPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int SLAPI PPObjQuotKind::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new PPQuotKindPacket);
	if(stream == 0) {
		THROW(GetPacket(id, (PPQuotKindPacket *)p->Data) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile((FILE*)stream, 0))
		THROW(SerializePacket(-1, (PPQuotKindPacket *)p->Data, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjQuotKind::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPQuotKindPacket * p_pack = (PPQuotKindPacket *)p->Data;
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				if((p_pack->Rec.ID && p_pack->Rec.ID < PP_FIRSTUSRREF) ||
					ref->SearchSymb(Obj, &same_id, p_pack->Rec.Name, offsetof(PPQuotKind, Name)) > 0 ||
					ref->SearchSymb(Obj, &same_id, p_pack->Rec.Symb, offsetof(PPQuotKind, Symb)) > 0) {
					PPQuotKind same_rec;
					if(Search(same_id, &same_rec) > 0 && (same_rec.OpID == p_pack->Rec.OpID || p_pack->Rec.ID == PPQUOTK_BASE)) {
						if(!(p->Flags & PPObjPack::fDispatcher)) {
							if(!PutPacket(&same_id, p_pack, 1)) {
								pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTQUOTKIND, p_pack->Rec.ID, p_pack->Rec.Name);
								ok = -1;
							}
						}
						ASSIGN_PTR(pID, same_id);
					}
					else
						same_id = 0;
				}
				if(same_id == 0) {
					if(p_pack->Rec.ID >= PP_FIRSTUSRREF)
						p_pack->Rec.ID = 0;
					else
						*pID = p_pack->Rec.ID;
					if(!CheckRights(PPR_INS) || !PutPacket(pID, p_pack, 1)) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTQUOTKIND, p_pack->Rec.ID, p_pack->Rec.Name);
						ok = -1;
					}
				}
			}
			else {
				if(!(p->Flags & PPObjPack::fDispatcher)) {
					p_pack->Rec.ID = *pID;
					if(!CheckRights(PPR_MOD) || !PutPacket(pID, p_pack, 1)) { // @v7.9.5 CheckRights
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTQUOTKIND, p_pack->Rec.ID, p_pack->Rec.Name);
						ok = -1;
					}
				}
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

int SLAPI PPObjQuotKind::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	return EditSpcRightFlags(DLG_RTQUOTK, 0, 0, bufSize, rt, pDlg);
}

int SLAPI PPObjQuotKind::MakeCodeString(const PPQuot * pQuot, SString & rBuf)
{
	rBuf = 0;
	if(pQuot) {
		PPQuotKind qk_rec;
		SString temp_buf;
		if(Fetch(pQuot->Kind, &qk_rec) > 0) {
			rBuf.Cat(qk_rec.Name);
		}
		else
			ideqvalstr(pQuot->Kind, rBuf);
		rBuf.CatDiv('-', 1).Cat(GetGoodsName(pQuot->GoodsID, temp_buf));
		GetLocationName(pQuot->LocID, temp_buf);
		rBuf.CatDiv('-', 1).Cat(temp_buf);
		if(pQuot->ArID) {
			GetArticleName(pQuot->ArID, temp_buf);
			rBuf.CatDiv('-', 1).Cat(temp_buf);
		}
		if(pQuot->CurID) {
			PPObjCurrency cur_obj;
			PPCurrency cur_rec;
			if(cur_obj.Fetch(pQuot->CurID, &cur_rec) > 0)
				(temp_buf = 0).Cat(cur_rec.Name);
			else
				ideqvalstr(pQuot->CurID, temp_buf = 0);
			rBuf.CatDiv('-', 1).Cat(temp_buf);
		}
		rBuf.CatDiv('=', 1).Cat(pQuot->PutValToStr(temp_buf));
	}
	else
		rBuf = "null";
	return 1;
}
//
//
//
class QuotKindCache : public ObjCache {
public:
	SLAPI  QuotKindCache() : ObjCache(PPOBJ_QUOTKIND, sizeof(Data)), SymbList(PPOBJ_QUOTKIND)
	{
		RtlListInited = 0;
	}
	int    SLAPI FetchRtlList(PPIDArray & rList, PPIDArray & rTmList);
	int    SLAPI FetchBySymb(const char * pSymb, PPID * pID)
	{
		return SymbList.FetchBySymb(pSymb, pID);
	}
	int    SLAPI FetchSpecialKinds(PPObjQuotKind::Special * pSk);
private:
	virtual int SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual int SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
	virtual int SLAPI Dirty(PPID id); // @sync_w

	int    RtlListInited;
	PPIDArray RtlQkList;   // —писок видов котировок с признаком QUOTKF_RETAILED
	PPIDArray RtlTmQkList; // —писок видов котировок с признаком QUOTKF_RETAILED, имеющих
		// ограничение к применению по дате/времени
	ReadWriteLock RtlLock;
	ReadWriteLock SkLock;
public:
	struct Data : public ObjCacheEntry {
		double Discount;
		DateRange  Period;
		int16  BeginTm;
		int16  EndTm;
		int16  Rank;
		uint8  DaysOfWeek;
		uint8  Pad; // @alignment
		PPID   OpID;
		long   Flags;
		PPID   AccSheetID;
		IntRange AmtRestr;
	};

	RefSymbArray SymbList;
	PPObjQuotKind::Special Sk;
};

int SLAPI QuotKindCache::Dirty(PPID id)
{
	int    ok = 1;
	ObjCache::Dirty(id);
	SymbList.Dirty(id);
	// @v9.0.9 {
	{
		RtlLock.ReadLock();
		if(RtlListInited) {
			RtlLock.Unlock();
			RtlLock.WriteLock();
			RtlQkList.clear();
			RtlTmQkList.clear();
			RtlListInited = 0;
		}
		RtlLock.Unlock();
	}
	// } @v9.0.9
	return ok;
}

int SLAPI QuotKindCache::FetchRtlList(PPIDArray & rList, PPIDArray & rTmList)
{
	int    ok = 1;
	RtlLock.ReadLock();
	if(!RtlListInited) {
		RtlLock.Unlock();
		RtlLock.WriteLock();
		PPObjQuotKind qk_obj;
		qk_obj.Helper_GetRtlList(ZERODATETIME, &RtlQkList, &RtlTmQkList, 0);
		RtlListInited = 1;
	}
	rList = RtlQkList;
	rTmList = RtlTmQkList;
	RtlLock.Unlock();
	return ok;
}

int SLAPI QuotKindCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = (Data *)pEntry;
	PPObjQuotKind qk_obj;
	PPQuotKind rec;
	if(qk_obj.Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
		CPY_FLD(Discount);
		CPY_FLD(Period);
		CPY_FLD(BeginTm);
		CPY_FLD(EndTm);
		CPY_FLD(Rank);
		CPY_FLD(DaysOfWeek);
		CPY_FLD(OpID);
		CPY_FLD(Flags);
		CPY_FLD(AccSheetID);
		CPY_FLD(AmtRestr);
#undef CPY_FLD
		ok = PutName(rec.Name, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

int SLAPI QuotKindCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPQuotKind * p_data_rec = (PPQuotKind *)pDataRec;
	const Data * p_cache_rec = (const Data *)pEntry;
	memzero(p_data_rec, sizeof(*p_data_rec));
#define CPY_FLD(Fld) p_data_rec->Fld=p_cache_rec->Fld
	p_data_rec->Tag = PPOBJ_QUOTKIND;
	CPY_FLD(ID);
	CPY_FLD(Discount);
	CPY_FLD(Period);
	CPY_FLD(BeginTm);
	CPY_FLD(EndTm);
	CPY_FLD(Rank);
	CPY_FLD(DaysOfWeek);
	CPY_FLD(OpID);
	CPY_FLD(Flags);
	CPY_FLD(AccSheetID);
	CPY_FLD(AmtRestr);
#undef CPY_FLD
	GetName(pEntry, p_data_rec->Name, sizeof(p_data_rec->Name));
	return 1;
}

int SLAPI QuotKindCache::FetchSpecialKinds(PPObjQuotKind::Special * pSk)
{
	SkLock.ReadLock();
	if(Sk.Flags & PPObjQuotKind::Special::fInited) {
		ASSIGN_PTR(pSk, Sk);
	}
	else {
		SkLock.Unlock();
		SkLock.WriteLock();
		if(Sk.Flags & PPObjQuotKind::Special::fInited) {
			ASSIGN_PTR(pSk, Sk);
		}
		else {
			PPObjQuotKind::GetSpecialKinds(&Sk, 0 /* @! strictrly 0 - without cache */);
			assert(Sk.Flags & PPObjQuotKind::Special::fInited);
			ASSIGN_PTR(pSk, Sk);
		}
	}
	SkLock.Unlock();
	return 1;
}

IMPL_OBJ_FETCH(PPObjQuotKind, PPQuotKind, QuotKindCache);

int SLAPI PPObjQuotKind::FetchRtlList(PPIDArray & rList, PPIDArray & rTmList)
{
	QuotKindCache * p_cache = GetDbLocalCachePtr <QuotKindCache> (PPOBJ_QUOTKIND);
	if(p_cache) {
		return p_cache->FetchRtlList(rList, rTmList);
	}
	else {
		return Helper_GetRtlList(ZERODATETIME, &rList, &rTmList, 0);
	}
}

int SLAPI PPObjQuotKind::FetchBySymb(const char * pSymb, PPID * pID)
{
	QuotKindCache * p_cache = GetDbLocalCachePtr <QuotKindCache> (PPOBJ_QUOTKIND);
	if(p_cache) {
		return p_cache->FetchBySymb(pSymb, pID);
	}
	else {
		return SearchBySymb(pSymb, pID, 0);
	}
}

//static
int FASTCALL PPObjQuotKind::GetSpecialKinds(Special * pRec, int useCache)
{
	if(useCache) {
		QuotKindCache * p_cache = GetDbLocalCachePtr <QuotKindCache> (PPOBJ_QUOTKIND);
		if(p_cache) {
			return p_cache ? p_cache->FetchSpecialKinds(pRec) : PPObjQuotKind::GetSpecialKinds(pRec, 0);
		}
	}
	else {
		Special rec;
		//
		PPObjGoods goods_obj;
		rec.MtxID = goods_obj.GetConfig().MtxQkID;
		rec.MtxRestrID = goods_obj.GetConfig().MtxRestrQkID;
		//
		rec.SupplDealID = DS.GetConstTLA().SupplDealQuotKindID;
		rec.SupplDevUpID = DS.GetConstTLA().SupplDevUpQuotKindID;
		rec.SupplDevDnID = DS.GetConstTLA().SupplDevDnQuotKindID;
		//
		PPPredictConfig predict_cfg;
		Predictor::GetPredictCfg(&predict_cfg);
		rec.PredictCoeffID = predict_cfg.CorrectKoeff;
		//
		rec.Flags |= Special::fInited;
		ASSIGN_PTR(pRec, rec);
	}
	return 1;
}

