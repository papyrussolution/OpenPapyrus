// OBJQUOTK.CPP
// Copyright (c) A.Sobolev 1998-2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2019, 2020, 2021, 2023, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>

PPQuotKind2::PPQuotKind2()
{
	THISZERO();
}

bool PPQuotKind2::HasWeekDayRestriction() const { return (DaysOfWeek && ((DaysOfWeek & 0x7f) != 0x7f)); }
bool PPQuotKind2::CheckWeekDay(LDATE dt) const { return (!DaysOfWeek || !dt || (DaysOfWeek & (1 << (dayofweek(&dt, 1)-1)))); }

void PPQuotKind2::SetTimeRange(const TimeRange & rRange)
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
}

bool PPQuotKind2::GetTimeRange(TimeRange & rRange) const
{
	rRange.Z();
	if(BeginTm) {
		int    h = PTR8C(&BeginTm)[0];
		int    m = PTR8C(&BeginTm)[1];
		if(h >= 0 && h <= 23 && m >= 0 && m <= 59)
			rRange.low = encodetime(h, m, 0, 0);
	}
	if(EndTm) {
		int    h = PTR8C(&EndTm)[0];
		int    m = PTR8C(&EndTm)[1];
		if(h >= 0 && h <= 23 && m >= 0 && m <= 59)
			rRange.upp = encodetime(h, m, 0, 0);
	}
	if(rRange.upp == 0 && rRange.low)
		rRange.upp = MAXDAYTIMESEC;
	return (rRange.low || rRange.upp);
}

int PPQuotKind2::GetAmtRange(RealRange * pRange) const
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

int PPQuotKind2::SetAmtRange(const RealRange * pRange)
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

int PPQuotKind2::GetRestrText(SString & rBuf) const
{
	SString temp_buf;
	rBuf.Z();
	if(!Period.IsZero()) {
		rBuf.Cat(Period, 1);
	}
	TimeRange tr;
	if(GetTimeRange(tr)) {
		rBuf.CatDivIfNotEmpty(';', 2);
		if(tr.low)
			rBuf.Cat(tr.low, TIMF_HM);
		rBuf.CatCharN('.', 2);
		if(tr.upp)
			rBuf.Cat(tr.upp, TIMF_HM);
	}
	if(DaysOfWeek) {
		rBuf.CatDivIfNotEmpty(';', 2);
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
			rBuf.CatDivIfNotEmpty(';', 2);
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
PPQuotKindPacket::PPQuotKindPacket()
{
	// @v10.6.5 Init();
}

void PPQuotKindPacket::Init()
{
	MEMSZERO(Rec);
}

int PPQuotKindPacket::GetCalculatedQuot(double cost, double basePrice, double * pQuot, long * pFlags) const
{
	int    ok = -1;
	double quot = 0.0;
	const  double discount = Rec.Discount;
	const  long   flags    = Rec.Flags;
	if(basePrice > 0.0 || (cost && flags & QUOTKF_PCTDISONCOST)) {
		if(discount != 0.0 || (flags & (QUOTKF_ABSDIS | QUOTKF_PCTDISONCOST))) {
			if(flags & QUOTKF_ABSDIS)
				quot = basePrice - discount;
			else if(flags & QUOTKF_PCTDISONCOST)
				quot = cost - fdiv100r(cost * discount);
			else
				quot = basePrice - fdiv100r(basePrice * discount);
			quot = PPObjQuotKind::RoundUpPrice(Rec.ID, quot); // @v10.9.11 QuotKindRounding
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
PPObjQuotKind::Special::Special(CtrOption ctrOption)
{
	THISZERO();
	if(oneof2(ctrOption, ctrInitialize, ctrInitializeWithCache)) {
		PPObjQuotKind::GetSpecialKinds(this, BIN(ctrOption == ctrInitializeWithCache));
	}
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

void PPObjQuotKind::Special::GetDefaults(int quotCls, PPID qkID, PPID * pAcsID, PPID * pDefQkID, long * pQkSelExtra) const
{
	PPID   acc_sheet_id = 0;
	long   qk_sel_extra = 1;
	PPID   new_qk_id = qkID;
	switch(quotCls) {
		case PPQuot::clsSupplDeal:
			acc_sheet_id = GetSupplAccSheet();
			qk_sel_extra = QuotKindFilt::fSupplDeal;
			if(!oneof3(new_qk_id, SupplDealID, SupplDevDnID, SupplDevUpID))
				new_qk_id = SupplDealID;
			break;
		case PPQuot::clsMtx:
			qk_sel_extra = QuotKindFilt::fGoodsMatrix;
			new_qk_id = MtxID;
			break;
		case PPQuot::clsPredictCoeff:
			qk_sel_extra = QuotKindFilt::fPredictCoeff;
			new_qk_id = PredictCoeffID;
			break;
		default:
			{
				PPObjQuotKind qk_obj;
				PPQuotKind qk_rec;
				if(new_qk_id && qk_obj.Fetch(new_qk_id, &qk_rec) > 0 && qk_rec.AccSheetID)
					acc_sheet_id = qk_rec.AccSheetID;
				else
					acc_sheet_id = GetSellAccSheet();
				qk_sel_extra = 1;
			}
			break;
	}
	ASSIGN_PTR(pAcsID, acc_sheet_id);
	ASSIGN_PTR(pDefQkID, new_qk_id);
	ASSIGN_PTR(pQkSelExtra, qk_sel_extra);
}

/*static*/PPID PPObjQuotKind::GetDefaultAccSheetID(int cls)
{
	return (cls == PPQuot::clsSupplDeal) ? GetSupplAccSheet() : ((cls == PPQuot::clsGeneral) ? GetSellAccSheet() : 0);
}

/*static*/double PPObjQuotKind::RoundUpPrice(PPID quotKindID, double p)
{
	int    dir = 0;
	double prec = 0.01;
	int    use_default_rounding = 1;
	if(quotKindID) {
		PPObjQuotKind qk_obj;
		PPQuotKind qk_rec;
		if(qk_obj.Fetch(quotKindID, &qk_rec) > 0 && qk_rec.Flags & QUOTKF_USEROUNDING) {
			dir = qk_rec.RoundingDir;
			prec = qk_rec.RoundingPrec;
			use_default_rounding = 0;
		}
	}
	if(use_default_rounding) {
		const  PPConfig & r_cfg = LConfig;
		long   rd = (r_cfg.Flags & (CFGFLG_ROUNDUP | CFGFLG_ROUNDDOWN));
		prec = r_cfg.RoundPrec;
		if(rd == 0 || rd == (CFGFLG_ROUNDUP | CFGFLG_ROUNDDOWN))
			dir = 0;
		else if(rd & CFGFLG_ROUNDUP)
			dir = 1;
		else if(rd & CFGFLG_ROUNDDOWN)
			dir = -1;
	}
	return PPRound(p, prec, dir);	
}

PPObjQuotKind::PPObjQuotKind(/*void * extraPtr*/) : PPObjReference(PPOBJ_QUOTKIND, /*extraPtr*/0)
{
	ImplementFlags |= implStrAssocMakeList;
}

int PPObjQuotKind::Classify(PPID id, int * pCls)
{
	int    ok = -1;
	int    cls = PPQuot::clsGeneral;
	PPQuotKind rec;
	if(id && Fetch(id, &rec) > 0) {
		const PPObjQuotKind::Special spc(PPObjQuotKind::Special::ctrInitializeWithCache);
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

int PPObjQuotKind::MakeReserved(long flags)
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
			// Здесь нельзя использовать PutPacket поскольку добавляется запись с предопределенным идентификатором.
			//
			THROW(StoreItem(Obj, 0, &pack.Rec, 1));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjQuotKind::GetListByOp(PPID opID, LDATE dt, PPIDArray * pList)
{
	PPObjGoods goods_obj;
	const  int  intrexpnd = IsIntrExpndOp(opID);
	const  PPID matrix_qk_id = goods_obj.GetConfig().MtxQkID;
	SVector list(sizeof(PPQuotKind));
	P_Ref->LoadItems(Obj, list);
	for(uint qkidx = 0; qkidx < list.getCount(); qkidx++) {
		const PPQuotKind * p_item = static_cast<const PPQuotKind *>(list.at(qkidx));
		const  PPID id = p_item->ID;
		if(ObjRts.CheckQuotKindID(id, 0)) {
			//
			// Если котировка не матричная И ("не для документов" либо нам это безразлично)
			//
			if(id != matrix_qk_id && !(p_item->Flags & QUOTKF_NOTFORBILL) && p_item->Period.CheckDate(dt)) {
				//
				// Если для вида котировки определен вид операции, то безусловно проверяем
				// соответствие нашей операции этому виду.
				//
				if(p_item->OpID) {
					if(IsOpBelongTo(opID, p_item->OpID))
						pList->addUnique(id);
				}
				else {
					//
					// В противном случае, для внутренней передачи используется только базовая котировка
					//
					if(intrexpnd) {
						if(id == PPQUOTK_BASE)
							pList->addUnique(id);
					}
					else {
						//
						// Остальные виды операций безусловно могут использовать этот вид котировки
						//
						pList->addUnique(id);
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
	const PPQuotKind * k1 = static_cast<const PPQuotKind *>(i1);
	const PPQuotKind * k2 = static_cast<const PPQuotKind *>(i2);
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

int PPObjQuotKind::ArrangeList(const LDATETIME & rDtm, PPIDArray & rQkList, long flags)
{
	int    ok = 1;
	const  uint c = rQkList.getCount();
	if(c) {
		uint i;
		SArray qk_list(sizeof(PPQuotKind));
		for(i = 0; i < c; i++) {
			PPQuotKind qk_rec;
			const  PPID qk_id = rQkList.get(i);
			if(Fetch(qk_id, &qk_rec) > 0) {
				int    suited = 1;
				TimeRange tmr;
				int    trcr = 0;
				if(rDtm.t && qk_rec.GetTimeRange(tmr) && !(flags & RTLPF_USEQUOTWTIME && (trcr = tmr.Check(rDtm.t)) != 0))
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
			rQkList.addUnique(static_cast<const PPQuotKind *>(qk_list.at(i))->ID);
	}
	return ok;
}

int PPObjQuotKind::Helper_GetRtlList(const LDATETIME & rDtm, PPIDArray * pList, PPIDArray * pTmList, long flags)
{
	int    ok = -1;
	if(pList || pTmList) {
		SArray qk_list(sizeof(PPQuotKind));
		PPIDArray tm_list;
		const  PPID sell_acc_sheet = GetSellAccSheet();
		PPQuotKind  qkr;
		for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&qkr) > 0;) {
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
				if(qkr.GetTimeRange(tmr)) {
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
				pList->addUnique(static_cast<const PPQuotKind *>(qk_list.at(i))->ID);
		}
		ASSIGN_PTR(pTmList, tm_list);
		ok = 1;
	}
	return  ok;
}

int PPObjQuotKind::GetRetailQuotList(LDATETIME dtm, PPIDArray * pList, long flags)
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
					if(flags & RTLPF_IGNCONDQUOTS) {
						if(qk_rec.GetTimeRange(tmr) || !qk_rec.Period.IsZero() || qk_rec.HasWeekDayRestriction())
							suited = 0;
					}
					else {
						if(qk_rec.GetTimeRange(tmr) && dtm.t && !(flags & RTLPF_USEQUOTWTIME && (trcr = tmr.Check(dtm.t)) != 0))
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
					} // @v10.0.01
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

int PPObjQuotKind::GetCalculatedQuot(PPID id, double cost, double basePrice, double * pQuot, long * pFlags)
{
	PPQuotKindPacket pack;
	return (Fetch(id, &pack.Rec) > 0) ? pack.GetCalculatedQuot(cost, basePrice, pQuot, pFlags) : 0;
}

int PPObjQuotKind::IsPacketEq(const PPQuotKindPacket & rS1, const PPQuotKindPacket & rS2, long flags)
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
	CMP_MEMB(RoundingPrec);   // @v10.9.10
	CMP_MEMB(RoundingDir);    // @v10.9.10
#undef CMP_MEMBS
#undef CMP_MEMB
	return 1;
}

int PPObjQuotKind::GetPacket(PPID id, PPQuotKindPacket * pPack)
{
	int    ok = 1;
	PPQuotKindPacket pack;
	THROW(Search(id, &pack.Rec) > 0);
	ASSIGN_PTR(pPack, pack);
	CATCHZOK
	return ok;
}

int PPObjQuotKind::PutPacket(PPID * pID, PPQuotKindPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				PPQuotKindPacket org_pack;
				THROW(GetPacket(*pID, &org_pack) > 0);
				if(!IsPacketEq(*pPack, org_pack, 0)) {
					THROW(CheckDupName(*pID, pPack->Rec.Name));
					THROW(CheckRights(PPR_MOD));
					THROW(P_Ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
				}
			}
			else {
				THROW(CheckRights(PPR_DEL));
				{
					PPObjGoods goods_obj;
					THROW(goods_obj.P_Tbl->RemoveAllQuotForQuotKind(*pID, 0));
					THROW(P_Ref->RemoveItem(Obj, *pID, 0));
					THROW(RemoveSync(*pID));
				}
			}
		}
		else {
			THROW(CheckRights(PPR_INS));
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			*pID = pPack->Rec.ID;
			THROW(P_Ref->AddItem(Obj, pID, &pPack->Rec, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

// /*virtual*/int  PPObjQuotKind::Remove(PPID id, long, uint options /* = rmv_default */)

/*virtual*/int  PPObjQuotKind::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options/* = rmv_default*/, void * pExtraParam)
{
	int    r = -1;
	THROW(CheckRights(PPR_DEL));
	if(id) {
		// Блокировка возможности удаления специальных видов котировок
		const PPObjQuotKind::Special spc(PPObjQuotKind::Special::ctrInitialize);
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
	if(!(options & PPObject::user_request) || PPMessage(mfConf|mfYesNo, PPCFM_DELQUOTKIND) == cmYes) {
		int    use_ta = (options & PPObject::use_transaction) ? 1 : 0;
		PPObjGoods goods_obj;
		PPWaitStart();
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(goods_obj.P_Tbl->RemoveAllQuotForQuotKind(id, 0));
			THROW(P_Ref->RemoveItem(Obj, id, 0));
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
	PPWaitStop();
	return r;
}

int PPObjQuotKind::SearchSymb(PPID * pID, const char * pSymb)
{
	return P_Ref->SearchSymb(Obj, pID, pSymb, offsetof(PPQuotKind, Symb));
}

StrAssocArray * PPObjQuotKind::MakeStrAssocList(void * extraPtr)
{
	StrAssocArray * p_list = new StrAssocArray;
	QuotKindFilt filt;
	filt.Flags = reinterpret_cast<long>(extraPtr);
	MakeList(&filt, p_list);
	return p_list;
}

IMPL_CMPFUNC(_QUOTK_LIST_ENTRY, i1, i2) { return stricmp866(static_cast<const PPObjQuotKind::ListEntry *>(i1)->Name, static_cast<const PPObjQuotKind::ListEntry *>(i2)->Name); }

IMPL_CMPFUNC(PPQuotKind, i1, i2)
{
	const PPQuotKind * p1 = static_cast<const PPQuotKind *>(i1);
	const PPQuotKind * p2 = static_cast<const PPQuotKind *>(i2);
	if(p1->Rank < p2->Rank)
		return +1;
	else if(p1->Rank > p2->Rank)
		return -1;
	else
		return stricmp866(p1->Name, p2->Name);
}

SArray * PPObjQuotKind::MakeListByIDList(const PPIDArray * pList)
{
	PPObjQuotKind::ListEntry entry;
	PPID   id = 0;
	uint   i;
	SString temp_buf;
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
				PPLoadString("basequote", temp_buf);
			else
				temp_buf = qk_rec.Name;
			STRNSCPY(entry.Name, temp_buf);
			THROW_SL(p_ary->insert(&entry));
		}
	}
	p_ary->sort(PTR_CMPFUNC(_QUOTK_LIST_ENTRY));
	CATCH
		ZDELETE(p_ary);
	ENDCATCH
	return p_ary;
}

QuotKindFilt::QuotKindFilt()
{
	THISZERO();
}

int PPObjQuotKind::MakeList(const QuotKindFilt * pFilt, StrAssocArray * pList)
{
	int    ok = -1, i;
	SString temp_buf;
	PPIDArray id_list;
	PPQuotKind qk_rec;
	SVector rec_list(sizeof(PPQuotKind));
	const PPObjQuotKind::Special spc(PPObjQuotKind::Special::ctrInitializeWithCache);
	if(pFilt->Flags & QuotKindFilt::fSupplDeal) {
		const  PPID spc_qk_list[] = { spc.SupplDealID, spc.SupplDevUpID, spc.SupplDevDnID };
		for(i = 0; i < SIZEOFARRAY(spc_qk_list); i++) {
			if(Search(spc_qk_list[i], &qk_rec) > 0) {
				THROW_SL(rec_list.insert(&qk_rec));
			}
		}
	}
	else if(pFilt->Flags & (QuotKindFilt::fGoodsMatrix|QuotKindFilt::fGoodsMatrixRestrict)) {
		const int is_matrix = BIN(pFilt->Flags & QuotKindFilt::fGoodsMatrix);
		PPGoodsConfig goods_cfg;
		PPObjGoods::ReadConfig(&goods_cfg);
		if(Search((is_matrix ? goods_cfg.MtxQkID : goods_cfg.MtxRestrQkID), &qk_rec) > 0) {
			THROW_SL(rec_list.insert(&qk_rec));
		}
	}
	else if(pFilt->Flags & QuotKindFilt::fPredictCoeff) {
		if(Search(spc.PredictCoeffID, &qk_rec) > 0) {
			THROW_SL(rec_list.insert(&qk_rec));
		}
	}
	else {
		int    intrexpnd = pFilt->OpID ? IsIntrExpndOp(pFilt->OpID) : 0;
		THROW(P_Ref->LoadItems(Obj, rec_list));
		if(pFilt->Flags & QuotKindFilt::fAddBase || intrexpnd) {
			const  PPID base_id = PPQUOTK_BASE;
			if(!rec_list.lsearch(&base_id, 0, CMPF_LONG, offsetof(PPQuotKind, ID))) {
				MEMSZERO(qk_rec);
				qk_rec.ID = PPQUOTK_BASE;
				PPLoadString("basequote", temp_buf);
				STRNSCPY(qk_rec.Name, temp_buf);
				THROW_SL(rec_list.insert(&qk_rec));
			}
		}
		rec_list.sort((pFilt->Flags & QuotKindFilt::fSortByRankName) ? PTR_CMPFUNC(PPQuotKind_RankName) : PTR_CMPFUNC(PPQuotKind));
		for(i = rec_list.getCount()-1; i >= 0; i--) {
			const PPQuotKind * p_rec = static_cast<const PPQuotKind *>(rec_list.at(i));
			if(!(pFilt->Flags & QuotKindFilt::fAll) && spc.IsSupplDealKind(p_rec->ID))
				rec_list.atFree(i);
			else if((p_rec->Flags & QUOTKF_NOTFORBILL) && (pFilt->Flags & QuotKindFilt::fExclNotForBill))
				rec_list.atFree(i);
			else if(pFilt->OpID) {
				if(intrexpnd) {
					if(p_rec->ID != PPQUOTK_BASE && p_rec->OpID && !IsOpBelongTo(pFilt->OpID, p_rec->OpID))
						rec_list.atFree(i);
				}
				else if(p_rec->OpID && !IsOpBelongTo(pFilt->OpID, p_rec->OpID))
					rec_list.atFree(i);
			}
		}
		if(pFilt->MaxItems > 0)
			while(rec_list.getCount() > (uint)pFilt->MaxItems)
				rec_list.freeLast();
	}
	if(pFilt && !(pFilt->Flags & QuotKindFilt::fIgnoreRights)) {
		uint   c = rec_list.getCount();
        if(c) do {
			const  PPID qk_id = ((PPQuotKind *)rec_list.at(--c))->ID;
			if(!ObjRts.CheckQuotKindID(qk_id, 0))
				rec_list.atFree(c);
        } while(c);
	}
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
	int    ok = 1;
	int    pctdis = 0;
	int    absdis = 0;
	int    pctdisoncost = 0;
	double v = 0.0;
	char   buf[32];
	if(pDlg->getCtrlData(ctl, buf)) {
		SString pattern;
		(pattern = "CcСс").Transf(CTRANSF_UTF8_TO_INNER); // module int utf8 format (lat 'c' and russian 'с')
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
		else if((p = sstrchr(buf, '$')) != 0) {
			absdis = 1;
			strcpy(p, p + 1);
		}
		p = buf + sstrlen(buf);
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
			PTR32(buf)[0] = 0;
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
	DECL_DIALOG_DATA(PPQuotKindPacket);
public:
	explicit QuotKindDialog(PPObjReference * pObjRef) : TDialog(DLG_QUOTKIND), P_ObjRef(pObjRef)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		PPIDArray  op_type_list;
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_QUOTKIND_NAME, Data.Rec.Name);
		setCtrlData(CTL_QUOTKIND_SYMB, Data.Rec.Symb);
		setCtrlData(CTL_QUOTKIND_ID,   &Data.Rec.ID);
		disableCtrl(CTL_QUOTKIND_ID, (int)Data.Rec.ID || !PPMaster);
		op_type_list.addzlist(PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, PPOPT_GOODSORDER,
			PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_GENERIC, PPOPT_DRAFTQUOTREQ, 0); // @v10.3.11 PPOPT_DRAFTRECEIPT // @v10.5.7 PPOPT_DRAFTQUOTREQ
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
		// @v10.9.10 @construction {
		{
			long   rp = 0;
			if(Data.Rec.Flags & QUOTKF_USEROUNDING) {
				if(Data.Rec.RoundingDir == 0)
					rp = 1;
				else if(Data.Rec.RoundingDir < 0)
					rp = 2;
				else // if(Data.Rec.RoundingDir > 0)
					rp = 3;
			}
			AddClusterAssocDef(CTL_QUOTKIND_RNDGDIR, 0, 0);
			AddClusterAssoc(CTL_QUOTKIND_RNDGDIR, 1, 1);
			AddClusterAssoc(CTL_QUOTKIND_RNDGDIR, 2, 2);
			AddClusterAssoc(CTL_QUOTKIND_RNDGDIR, 3, 3);
			SetClusterData(CTL_QUOTKIND_RNDGDIR, rp);
			setCtrlReal(CTL_QUOTKIND_RNDGPREC, Data.Rec.RoundingPrec);
		}
		// } @v10.9.10 @construction
		UpdateView();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(sel = CTL_QUOTKIND_NAME, Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name) != 0, PPERR_NAMENEEDED);
		THROW(P_ObjRef->CheckDupName(Data.Rec.ID, Data.Rec.Name));
		getCtrlData(sel = CTL_QUOTKIND_SYMB,  Data.Rec.Symb);
		THROW(P_ObjRef->P_Ref->CheckUniqueSymb(P_ObjRef->Obj, Data.Rec.ID, Data.Rec.Symb, offsetof(PPQuotKind, Symb)))
		getCtrlData(CTL_QUOTKIND_ID,    &Data.Rec.ID);
		getCtrlData(CTLSEL_QUOTKIND_OP, &Data.Rec.OpID);
		getCtrlData(CTLSEL_QUOTKIND_ACCSHEET, &Data.Rec.AccSheetID);
		GetClusterData(CTL_QUOTKIND_FLAGS, &Data.Rec.Flags);
		GetDiscount(this, CTL_QUOTKIND_DISCOUNT, &Data.Rec);
		getCtrlData(CTL_QUOTKIND_RANK,  &Data.Rec.Rank);
		// @v10.9.10 @construction {
		{
			long   rp = GetClusterData(CTL_QUOTKIND_RNDGDIR);
			if(rp == 0) {
				Data.Rec.Flags &= ~QUOTKF_USEROUNDING;
				Data.Rec.RoundingDir = 0;
				Data.Rec.RoundingPrec = 0.01;
			}
			else {
				Data.Rec.Flags |= QUOTKF_USEROUNDING;
				if(rp == 1)
					Data.Rec.RoundingDir = 0;
				else if(rp == 2)
					Data.Rec.RoundingDir = -1;
				else if(rp == 3)
					Data.Rec.RoundingDir = +1;
				else {
					// Получено неожиданное значение - на всякий случай блокируем округление
					Data.Rec.Flags &= ~QUOTKF_USEROUNDING;
					Data.Rec.RoundingDir = 0;
				}
				getCtrlData(CTL_QUOTKIND_RNDGPREC, &Data.Rec.RoundingPrec);
			}
		}
		// } @v10.9.10 @construction
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = 0;
			selectCtrl(sel);
		ENDCATCH
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_QUOTKIND_FLAGS) || event.isCbSelected(CTLSEL_QUOTKIND_ACCSHEET) || event.isClusterClk(CTL_QUOTKIND_RNDGDIR))
			UpdateView();
		else if(event.isCmd(cmQuotKindRestr))
			EditRestr();
		else
			return;
		clearEvent(event);
	}
	void   UpdateView()
	{
		int   not_retailed = 0;
		getDTS(0);
		const  PPID  acs_id = Data.Rec.AccSheetID;
		const int   not_sell_accsheet = BIN(acs_id && acs_id != GetSellAccSheet() && acs_id != GetAgentAccSheet());
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
		// @v10.9.10 {
		{
			const long rp = GetClusterData(CTL_QUOTKIND_RNDGDIR);
			disableCtrl(CTL_QUOTKIND_RNDGPREC, rp == 0);
		}
		// } @v10.9.10 
	}
	void   SetTimePeriod(TDialog * pDlg);
	int    GetTimePeriod(TDialog * pDlg);
	int    EditRestr();

	PPObjReference * P_ObjRef;
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
	char   buf[64];
	pDlg->getCtrlData(CTL_QKRESTR_TIMERANGE, buf);
	if(buf[0]) {
		char * p = 0;
		LTIME  beg_tm, end_tm;
		THROW((p = sstrchr(strip(buf), '.')) != 0 && p[1] == '.');
		*p = 0;
		strtotime(buf, 0, &beg_tm);
		THROW(((beg_tm.hour() >= 0 && beg_tm.hour() <= 23) || (beg_tm.hour() == 24 && beg_tm.minut() == 0)) &&
			beg_tm.minut() >= 0 && beg_tm.minut() <= 60);
		strtotime(p + 2, 0, &end_tm);
		THROW(((end_tm.hour() >= 0 && end_tm.hour() <= 23) || (end_tm.hour() == 24 && end_tm.minut() == 0)) &&
			end_tm.minut() >= 0 && end_tm.minut() <= 60);
		if(end_tm == ZEROTIME)
			end_tm = encodetime(24, 0, 0, 0);
		if(beg_tm == ZEROTIME && end_tm.hour() == 24)
			end_tm = ZEROTIME;
		p = PTRCHR_(&Data.Rec.BeginTm);
		*p = beg_tm.hour();
		*(p + 1) = beg_tm.minut();
		p = PTRCHR_(&Data.Rec.EndTm);
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
	if(CheckDialogPtrErr(&dlg)) {
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

int PPObjQuotKind::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel;
	int    valid_data = 0;
	bool   is_new = false;
	PPQuotKindPacket pack;
	QuotKindDialog * p_dlg = 0;
	THROW(CheckDialogPtr(&(p_dlg = new QuotKindDialog(this))));
	THROW(EditPrereq(pID, p_dlg, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	else {
		pack.Init();
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

int PPObjQuotKind::Browse(void * extraPtr) { return RefObjView(this, PPDS_CRRQUOTKIND, 0); }

int PPObjQuotKind::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE && _obj == PPOBJ_OPRKIND) {
		int    r;
		for(PPID id = 0; ok == DBRPL_OK && (r = P_Ref->EnumItems(Obj, &id)) > 0;)
			if(reinterpret_cast<const PPQuotKind *>(&P_Ref->data)->OpID == _id)
				ok = RetRefsExistsErr(Obj, id);
		if(ok == DBRPL_OK && r == 0)
			ok = DBRPL_ERROR;
	}
	return DBRPL_OK;
}

IMPL_DESTROY_OBJ_PACK(PPObjQuotKind, PPQuotKindPacket);

int PPObjQuotKind::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(p && p->Data) {
		PPQuotKindPacket * p_pack = static_cast<PPQuotKindPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_OPRKIND,  &p_pack->Rec.OpID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_ACCSHEET, &p_pack->Rec.AccSheetID, ary, replace));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjQuotKind::SerializePacket(int dir, PPQuotKindPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int PPObjQuotKind::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjQuotKind, PPQuotKindPacket>(this, p, id, stream, pCtx); }

int PPObjQuotKind::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPQuotKindPacket * p_pack = static_cast<PPQuotKindPacket *>(p->Data);
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				if((p_pack->Rec.ID && p_pack->Rec.ID < PP_FIRSTUSRREF) ||
					P_Ref->SearchSymb(Obj, &same_id, p_pack->Rec.Name, offsetof(PPQuotKind, Name)) > 0 ||
					P_Ref->SearchSymb(Obj, &same_id, p_pack->Rec.Symb, offsetof(PPQuotKind, Symb)) > 0) {
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
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int PPObjQuotKind::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	return EditSpcRightFlags(DLG_RTQUOTK, 0, 0, bufSize, rt, pDlg);
}

SString & PPObjQuotKind::MakeCodeString(const PPQuot * pQuot, SString & rBuf)
{
	rBuf.Z();
	if(pQuot) {
		PPQuotKind qk_rec;
		SString temp_buf;
		if(Fetch(pQuot->Kind, &qk_rec) > 0)
			rBuf.Cat(qk_rec.Name);
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
				temp_buf.Z().Cat(cur_rec.Name);
			else
				ideqvalstr(pQuot->CurID, temp_buf.Z());
			rBuf.CatDiv('-', 1).Cat(temp_buf);
		}
		rBuf.CatDiv('=', 1).Cat(pQuot->PutValToStr(temp_buf));
	}
	else
		rBuf = "null";
	return rBuf;
}
//
//
//
class QuotKindCache : public ObjCache {
public:
	QuotKindCache() : ObjCache(PPOBJ_QUOTKIND, sizeof(Data)), SymbList(PPOBJ_QUOTKIND), RtlListInited(0),
		Sk(PPObjQuotKind::Special::ctrDefault)
	{
	}
	int    FetchRtlList(PPIDArray & rList, PPIDArray & rTmList);
	int    FetchBySymb(const char * pSymb, PPID * pID)
	{
		return SymbList.FetchBySymb(pSymb, pID);
	}
	int    FetchSpecialKinds(PPObjQuotKind::Special * pSk);
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
	virtual void FASTCALL Dirty(PPID id); // @sync_w

	int    RtlListInited;
	PPIDArray RtlQkList;   // Список видов котировок с признаком QUOTKF_RETAILED
	PPIDArray RtlTmQkList; // Список видов котировок с признаком QUOTKF_RETAILED, имеющих ограничение к применению по дате/времени
	ReadWriteLock RtlLock;
	ReadWriteLock SkLock;
public:
	struct Data : public ObjCacheEntry {
		double Discount;
		double RoundingPrec;   // @v10.9.10 (Flags & QUOTKF_USEROUNDING) Точность округления
		DateRange Period;
		IntRange AmtRestr;
		PPID   OpID;
		long   Flags;
		PPID   AccSheetID;
		int16  BeginTm;
		int16  EndTm;
		int16  Rank;
		int16  RoundingDir;    // @v10.9.10 (Flags & QUOTKF_USEROUNDING) Направление округления (-1 down, +1 up, 0 nearest)
		uint8  DaysOfWeek;
		uint8  Pad[3];         // @alignment
	};

	RefSymbArray SymbList;
	PPObjQuotKind::Special Sk;
};

void FASTCALL QuotKindCache::Dirty(PPID id)
{
	ObjCache::Dirty(id);
	SymbList.Dirty(id);
	{
		SRWLOCKER(RtlLock, SReadWriteLocker::Read);
		if(RtlListInited) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			RtlQkList.clear();
			RtlTmQkList.clear();
			RtlListInited = 0;
		}
	}
}

int QuotKindCache::FetchRtlList(PPIDArray & rList, PPIDArray & rTmList)
{
	int    ok = 1;
	{
		SRWLOCKER(RtlLock, SReadWriteLocker::Read);
		if(!RtlListInited) {
			PPObjQuotKind qk_obj;
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			qk_obj.Helper_GetRtlList(ZERODATETIME, &RtlQkList, &RtlTmQkList, 0);
			RtlListInited = 1;
		}
		rList = RtlQkList;
		rTmList = RtlTmQkList;
	}
	return ok;
}

int QuotKindCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjQuotKind qk_obj;
	PPQuotKind rec;
	if(qk_obj.Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
		CPY_FLD(Discount);
		CPY_FLD(RoundingPrec); // @v10.9.11
		CPY_FLD(Period);
		CPY_FLD(AmtRestr);
		CPY_FLD(OpID);
		CPY_FLD(Flags);
		CPY_FLD(AccSheetID);
		CPY_FLD(BeginTm);
		CPY_FLD(EndTm);
		CPY_FLD(Rank);
		CPY_FLD(RoundingDir); // @v10.9.11
		CPY_FLD(DaysOfWeek);
#undef CPY_FLD
		ok = PutName(rec.Name, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void QuotKindCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPQuotKind * p_data_rec = static_cast<PPQuotKind *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
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
	CPY_FLD(RoundingPrec); // @v10.9.11
	CPY_FLD(RoundingDir);  // @v10.9.11
#undef CPY_FLD
	GetName(pEntry, p_data_rec->Name, sizeof(p_data_rec->Name));
}

int QuotKindCache::FetchSpecialKinds(PPObjQuotKind::Special * pSk)
{
	{
		SRWLOCKER(SkLock, SReadWriteLocker::Read);
		if(Sk.Flags & PPObjQuotKind::Special::fInited) {
			ASSIGN_PTR(pSk, Sk);
		}
		else {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(Sk.Flags & PPObjQuotKind::Special::fInited) {
				ASSIGN_PTR(pSk, Sk);
			}
			else {
				PPObjQuotKind::GetSpecialKinds(&Sk, 0 /* @! strictrly 0 - without cache */);
				assert(Sk.Flags & PPObjQuotKind::Special::fInited);
				ASSIGN_PTR(pSk, Sk);
			}
		}
	}
	return 1;
}

IMPL_OBJ_FETCH(PPObjQuotKind, PPQuotKind, QuotKindCache);

int PPObjQuotKind::FetchRtlList(PPIDArray & rList, PPIDArray & rTmList)
{
	QuotKindCache * p_cache = GetDbLocalCachePtr <QuotKindCache> (PPOBJ_QUOTKIND);
	if(p_cache) {
		return p_cache->FetchRtlList(rList, rTmList);
	}
	else {
		return Helper_GetRtlList(ZERODATETIME, &rList, &rTmList, 0);
	}
}

int PPObjQuotKind::FetchBySymb(const char * pSymb, PPID * pID)
{
	QuotKindCache * p_cache = GetDbLocalCachePtr <QuotKindCache> (PPOBJ_QUOTKIND);
	return p_cache ? p_cache->FetchBySymb(pSymb, pID) : SearchBySymb(pSymb, pID, 0);
}

/*static*/int FASTCALL PPObjQuotKind::GetSpecialKinds(Special * pRec, int useCache)
{
	if(useCache) {
		QuotKindCache * p_cache = GetDbLocalCachePtr <QuotKindCache> (PPOBJ_QUOTKIND);
		if(p_cache)
			return p_cache ? p_cache->FetchSpecialKinds(pRec) : PPObjQuotKind::GetSpecialKinds(pRec, 0);
	}
	else {
		PPObjQuotKind::Special rec(PPObjQuotKind::Special::ctrDefault);
		PPObjGoods goods_obj;
		rec.MtxID = goods_obj.GetConfig().MtxQkID;
		rec.MtxRestrID = goods_obj.GetConfig().MtxRestrQkID;
		//
		const  PPThreadLocalArea & r_tla = DS.GetConstTLA();
		rec.SupplDealID  = r_tla.SupplDealQuotKindID;
		rec.SupplDevUpID = r_tla.SupplDevUpQuotKindID;
		rec.SupplDevDnID = r_tla.SupplDevDnQuotKindID;
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
