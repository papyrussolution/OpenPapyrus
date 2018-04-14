// V_SCARD.CPP
// Copyright (c) A.Sobolev, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>
//
//
//
int SLAPI Helper_EditSCardFilt(SCardFilt * pFilt, int cascade);
//
//
//
SCardSelPrcssrParam::SCardSelPrcssrParam()
{
	Init();
}

int SCardSelPrcssrParam::IsEmpty() const
{
	return (!Flags && !FlagsSet && !FlagsReset && DtEnd == ZERODATE && !NewSerID && !AutoGoodsID &&
		!(PeriodTerm && PeriodCount) && (Discount <= 0.0 || !(Flags & fZeroDiscount)));
}

void SCardSelPrcssrParam::Init()
{
	Ver = DS.GetVersion();
	Flags = 0;
	FlagsSet = 0;
	FlagsReset = 0;
	PeriodTerm = 0;
	PeriodCount = 0;
	Discount = 0.0;
	DtEnd = ZERODATE;
	NewSerID = 0;
	AutoGoodsID = 0;
	SelFilt.Init(1, 0);
}

int SCardSelPrcssrParam::Validate(PPID srcSeriesID)
{
	int    ok = 1;
	SString msg_buf;
	const  long valid_flags = PPObjSCard::GetValidFlags();
	THROW_SL(checkdate(DtEnd, 1));
	if(AutoGoodsID) {
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		THROW(goods_obj.Fetch(AutoGoodsID, &goods_rec) > 0);
		THROW_PP_S(goods_rec.Kind == PPGDSK_GOODS && !(goods_rec.Flags & GF_GENERIC), PPERR_INVSCARDAUTOGOODS, goods_rec.Name);
	}
	if(srcSeriesID && NewSerID && srcSeriesID != NewSerID) {
		PPObjSCardSeries scs_obj;
		PPSCardSeries src_scs_rec, dest_scs_rec;
		THROW(scs_obj.Fetch(srcSeriesID, &src_scs_rec) > 0);
		THROW(scs_obj.Fetch(NewSerID, &dest_scs_rec) > 0);
		THROW_PP(src_scs_rec.GetType() == dest_scs_rec.GetType(), PPERR_UNABLEMOVSC_DIFFSERTYPE);
	}
	THROW_PP_S((FlagsSet   & ~valid_flags) == 0, PPERR_INVSCARDFLAGS, msg_buf.Z().CatHex(FlagsSet));
	THROW_PP_S((FlagsReset & ~valid_flags) == 0, PPERR_INVSCARDFLAGS, msg_buf.Z().CatHex(FlagsReset));

	for(uint i = 0; i < 32; i++) {
		const long t = (1 << i);
		if(t & valid_flags) {
			THROW_PP_S((FlagsSet & FlagsReset & t) == 0, PPERR_AMBIGSCARDSETRESETFLAGS, msg_buf.Z().CatHex(FlagsSet).CatChar('-').CatHex(FlagsReset));
		}
	}
	THROW_PP(Discount >= 0.0 && Discount <= 100.0, PPERR_PERCENTINPUT);
	CATCHZOK
	return ok;
}

int SCardSelPrcssrParam::Write(SBuffer & rBuf, long) const
{
	int    ok = 1;
	THROW_SL(rBuf.Write(Ver));
	THROW_SL(rBuf.Write(Flags));
	THROW_SL(rBuf.Write(DtEnd));
	THROW_SL(rBuf.Write(NewSerID));
	THROW_SL(rBuf.Write(AutoGoodsID));
	THROW_SL(rBuf.Write(FlagsSet));
	THROW_SL(rBuf.Write(FlagsReset));
	THROW_SL(rBuf.Write(PeriodTerm));
	THROW_SL(rBuf.Write(PeriodCount));
	THROW_SL(rBuf.Write(Discount));
	THROW_SL(SelFilt.Write(rBuf, 0));
	CATCHZOK
	return ok;
}

int SCardSelPrcssrParam::Read(SBuffer & rBuf, long)
{
	int    ok = 1;
	uint32 ver_test = 0;
	THROW_SL(rBuf.GetAvailableSize());
	THROW_SL(rBuf.Read(ver_test));
	Ver.Set(ver_test);
	if(Ver.IsLt(7, 6, 12)) {
		Ver = DS.GetVersion();
		Flags = ver_test;
		AutoGoodsID = 0;
		THROW_SL(rBuf.Read(DtEnd));
		THROW_SL(rBuf.Read(NewSerID));
		FlagsSet = 0;
		FlagsReset = 0;
		PeriodTerm = 0;
		PeriodCount = 0;
		Discount = 0.0;
		Flags &= ~fZeroDiscount;
	}
	else if(Ver.IsLt(7, 7, 2)){
		THROW_SL(rBuf.Read(Flags));
		THROW_SL(rBuf.Read(DtEnd));
		THROW_SL(rBuf.Read(NewSerID));
		THROW_SL(rBuf.Read(AutoGoodsID));
		FlagsSet = 0;
		FlagsReset = 0;
		PeriodTerm = 0;
		PeriodCount = 0;
		Discount = 0.0;
		Flags &= ~fZeroDiscount;
	}
	else {
		THROW_SL(rBuf.Read(Flags));
		THROW_SL(rBuf.Read(DtEnd));
		THROW_SL(rBuf.Read(NewSerID));
		THROW_SL(rBuf.Read(AutoGoodsID));
		THROW_SL(rBuf.Read(FlagsSet));
		THROW_SL(rBuf.Read(FlagsReset));
		THROW_SL(rBuf.Read(PeriodTerm));
		THROW_SL(rBuf.Read(PeriodCount));
		THROW_SL(rBuf.Read(Discount));
	}
	THROW_SL(SelFilt.Read(rBuf, 0));
	CATCHZOK
	return ok;
}
//
// @ModuleDef(PPViewSCard)
//
IMPLEMENT_PPFILT_FACTORY(SCard); SLAPI SCardFilt::SCardFilt() : PPBaseFilt(PPFILT_SCARD, 0, 3), // @v7.7.11 1-->2 // @v8.4.2 2-->3
	P_SjF(0), P_ExludeOwnerF(0), P_OwnerF(0)
{
	SetFlatChunk(offsetof(SCardFilt, ReserveStart),
		offsetof(SCardFilt, Reserve) - offsetof(SCardFilt, ReserveStart) + sizeof(Reserve));
	SetBranchSString(offsetof(SCardFilt, Number));
	SetBranchObjIdListFilt(offsetof(SCardFilt, ScsList)); // @v8.4.2
	SetBranchBaseFiltPtr(PPFILT_SYSJOURNAL, offsetof(SCardFilt, P_SjF));
	SetBranchBaseFiltPtr(PPFILT_SCARD, offsetof(SCardFilt, P_ExludeOwnerF)); // @v8.4.2
	SetBranchBaseFiltPtr(PPFILT_PERSON, offsetof(SCardFilt, P_OwnerF)); // @v8.4.2
	Init(1, 0);
}

SCardFilt & FASTCALL SCardFilt::operator = (const SCardFilt & rS)
{
	Copy(&rS, 1);
	return *this;
}

PPID SLAPI SCardFilt::GetOwnerPersonKind() const
{
	PPID   pk_id = 0;
	if(ScsList.IsExists()) {
		PPObjSCardSeries scs_obj;
		PPSCardSeries scs_rec;
		for(uint i = 0; i < ScsList.GetCount(); i++) {
			const PPID scs_id = ScsList.Get(i);
			if(scs_obj.Fetch(scs_id, &scs_rec) > 0 && scs_rec.PersonKindID) {
				if(pk_id && scs_rec.PersonKindID != pk_id) { // @ambiguity
					pk_id = 0;
					break;
				}
				else
					pk_id = scs_rec.PersonKindID;
			}
		}
	}
	else if(SeriesID) {
		PPObjSCardSeries scs_obj;
		PPSCardSeries scs_rec;
		if(scs_obj.Fetch(SeriesID, &scs_rec) > 0 && scs_rec.PersonKindID) {
			pk_id = scs_rec.PersonKindID;
		}
	}
	if(!pk_id) {
		PPSCardConfig scs_cfg;
		if(PPObjSCardSeries::FetchConfig(&scs_cfg) > 0 && scs_cfg.PersonKindID)
			pk_id = scs_cfg.PersonKindID;
		else
			pk_id = GetSellPersonKind();
	}
	return pk_id;
}

int SLAPI SCardFilt::ReadPreviosVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	struct SCardFilt_v1 : public PPBaseFilt {
		SLAPI  SCardFilt_v1() : PPBaseFilt(PPFILT_SCARD, 0, 1)
		{
			SetFlatChunk(offsetof(SCardFilt, ReserveStart),
				offsetof(SCardFilt, Reserve) - offsetof(SCardFilt, ReserveStart) + sizeof(Reserve));
			SetBranchSString(offsetof(SCardFilt, Number));
			Init(1, 0);
		}
		uint8  ReserveStart[16];   // @anchor
		DateRange IssuePeriod;
		DateRange ExpiryPeriod;
		DateRange TrnovrPeriod;
		PPID   SeriesID;
		PPID   PersonID;
		PPID   EmployerID;
		RealRange PDisR;
		RealRange TurnoverR;
		int16  Ft_Inherited;
		int16  Ft_Closed;
		long   Order;
		long   Flags;
		long   Reserve;            // @anchor Заглушка для отмера плоского участка фильтра
		SString Number;
	};
	struct SCardFilt_v2 : public PPBaseFilt {
		SLAPI  SCardFilt_v2() : PPBaseFilt(PPFILT_SCARD, 0, 2)
		{
			P_SjF = 0;
			SetFlatChunk(offsetof(SCardFilt, ReserveStart),
				offsetof(SCardFilt, Reserve) - offsetof(SCardFilt, ReserveStart) + sizeof(Reserve));
			SetBranchSString(offsetof(SCardFilt, Number));
			SetBranchBaseFiltPtr(PPFILT_SYSJOURNAL, offsetof(SCardFilt, P_SjF));
			Init(1, 0);
		}
		uint8  ReserveStart[16]; // @anchor
		DateRange IssuePeriod;
		DateRange ExpiryPeriod;
		DateRange TrnovrPeriod;
		PPID   SeriesID;
		PPID   PersonID;
		PPID   EmployerID;
		RealRange PDisR;
		RealRange TurnoverR;
		int16  Ft_Inherited;
		int16  Ft_Closed;
		long   Order;
		long   Flags;
		long   Reserve;       // @anchor Заглушка для отмера плоского участка фильтра
		SString Number;
		SysJournalFilt * P_SjF;
	};
#define CPYFLD(f) f = fv.f
	if(ver == 1) {
		SCardFilt_v1 fv;
		THROW(fv.Read(rBuf, 0));
		CPYFLD(IssuePeriod);
		CPYFLD(ExpiryPeriod);
		CPYFLD(TrnovrPeriod);
		CPYFLD(SeriesID);
		CPYFLD(PersonID);
		CPYFLD(EmployerID);
		CPYFLD(PDisR);
		CPYFLD(TurnoverR);
		CPYFLD(Ft_Inherited);
		CPYFLD(Ft_Closed);
		CPYFLD(Order);
		CPYFLD(Flags);
		CPYFLD(Reserve);
		CPYFLD(Number);
		ok = 1;
	}
	else if(ver == 2) {
		SCardFilt_v2 fv;
		THROW(fv.Read(rBuf, 0));
		CPYFLD(IssuePeriod);
		CPYFLD(ExpiryPeriod);
		CPYFLD(TrnovrPeriod);
		CPYFLD(SeriesID);
		CPYFLD(PersonID);
		CPYFLD(EmployerID);
		CPYFLD(PDisR);
		CPYFLD(TurnoverR);
		CPYFLD(Ft_Inherited);
		CPYFLD(Ft_Closed);
		CPYFLD(Order);
		CPYFLD(Flags);
		CPYFLD(Reserve);
		CPYFLD(Number);
		THROW(PPBaseFilt::CopyBaseFiltPtr(PPFILT_SYSJOURNAL, fv.P_SjF, (PPBaseFilt **)&P_SjF));
		ok = 1;
	}
#undef CPYFLD
	CATCHZOK
	return ok;
}

PPViewSCard::PreprocessScRecBlock::PreprocessScRecBlock() : ScID(0), InTurnover(0.0), Turnover(0.0)
{
}

SLAPI PPViewSCard::PPViewSCard() : PPView(&SCObj, &Filt, PPVIEW_SCARD), P_StffObj(0), P_TmpTbl(0), P_TempOrd(0)
{
	DefReportId = REPORT_SCARDLIST;
}

SLAPI PPViewSCard::~PPViewSCard()
{
	delete P_StffObj;
	delete P_TmpTbl;
	delete P_TempOrd;
}

PPBaseFilt * SLAPI PPViewSCard::CreateFilt(void * extraPtr) const
{
	PPObjSCardSeries scs_obj;
	SCardFilt * p_filt = new SCardFilt;
	if(((long)extraPtr) && scs_obj.Search(((long)extraPtr), 0) > 0)
		p_filt->SeriesID = scs_obj.GetSingle();
	else
		p_filt->SeriesID = scs_obj.GetSingle();
	return p_filt;
}

int SLAPI PPViewSCard::IsTempTblNeeded() const
{
	return BIN(!Filt.TrnovrPeriod.IsZero() || Filt.EmployerID || Filt.Number.NotEmpty() ||
		(Filt.Flags & (SCardFilt::fSinceLastPDisUpdating|SCardFilt::fShowOwnerAddrDetail)) ||
		(Filt.P_SjF && !Filt.P_SjF->IsEmpty()) || (Filt.P_ExludeOwnerF && !Filt.P_ExludeOwnerF->IsEmpty()) ||
		SeriesList.GetCount() > 1);
}

int SLAPI PPViewSCard::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	ZDELETE(P_TmpTbl);
	BExtQuery::ZDelete(&P_IterQuery);
	List.Clear();
	ExcludeOwnerList.Set(0); // @v8.4.2
	Counter.Init();
	SETMAX(Filt.PDisR.low, 0.0);
	SETMAX(Filt.PDisR.upp, 0.0);
	Filt.IssuePeriod.Actualize(ZERODATE);
	Filt.ExpiryPeriod.Actualize(ZERODATE);
	Filt.TrnovrPeriod.Actualize(ZERODATE);
	// @v9.6.6 Filt.MinTurnover = R2(Filt.MinTurnover);
	// @v9.6.6 Filt.MaxTurnover = R2(Filt.MaxTurnover);
	Filt.TurnoverR.Round(2); // @v9.6.6
	if(Filt.EmployerID)
		SETIFZ(P_StffObj, new PPObjStaffList);
	// @v9.8.9 {
	StrPool.ClearS();
	{
		uint   i;
		PPObjSCardSeries scs_obj;
		PPIDArray temp_series_list;
		PPIDArray finish_series_list;
		temp_series_list.addnz(Filt.SeriesID);
		for(i = 0; i < Filt.ScsList.GetCount(); i++)
			temp_series_list.addnz(Filt.ScsList.Get(i));
		temp_series_list.sortAndUndup();
		for(i = 0; i < temp_series_list.getCount(); i++) {
			PPSCardSeries scs_rec;
			const PPID scs_id = temp_series_list.get(i);
			scs_obj.GetChildList(scs_id, finish_series_list);
		}
		if(finish_series_list.getCount()) {
			finish_series_list.sortAndUndup();
			SeriesList.Set(&finish_series_list);
		}
		else 
			SeriesList.Set(0); 
	}
	// } @v9.8.9 
	if(Filt.P_ExludeOwnerF && !Filt.P_ExludeOwnerF->IsEmpty()) {
		PPIDArray owner_list;
        PPViewSCard temp_view;
		SCardViewItem temp_item;
        SCardFilt temp_filt;
        temp_filt = *Filt.P_ExludeOwnerF;
        temp_filt.Flags |= SCardFilt::fNoTempTable;
        THROW(temp_view.Init_(Filt.P_ExludeOwnerF));
        for(temp_view.InitIteration(); temp_view.NextIteration(&temp_item) > 0;) {
			if(temp_item.PersonID)
				THROW_SL(owner_list.add(temp_item.PersonID));
        }
        owner_list.sortAndUndup();
        ExcludeOwnerList.Set(&owner_list);
	}
	if(IsTempTblNeeded() || Filt.Flags & SCardFilt::fNoTempTable) {
		//
		// При (Filt.Flags & SCardFilt::fNoTempTable) формируется список this->List
		//
		THROW(CreateTempTable());
	}
	CATCHZOK
	return ok;
}

//PP_CREATE_TEMP_FILE_PROC(CreateTempFile, SCard);
// @v8.6.6 PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempSCard);

int SLAPI PPViewSCard::GetPDisUpdateDate(PPID cardID, LDATE before, LDATE * pDate)
{
	SysJournalTbl::Key0 k;
	k.Dt = NZOR(before, MAXDATE);
	k.Tm = ZEROTIME;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	BExtQuery q(p_sj, 0, 1);
	q.select(p_sj->Dt, p_sj->Action, 0L).where(p_sj->Dt < k.Dt && p_sj->ObjType == PPOBJ_SCARD && p_sj->ObjID == cardID);
	for(q.initIteration(1, &k, spLe); q.nextIteration() > 0;)
		if(oneof2(p_sj->data.Action, PPACN_OBJADD, PPACN_SCARDDISUPD)) {
			ASSIGN_PTR(pDate, p_sj->data.Dt);
			return 1;
		}
	ASSIGN_PTR(pDate, ZERODATE);
	return -1;
}

int SLAPI PPViewSCard::CreateTempTable()
{
	ZDELETE(P_TmpTbl);

	int    ok = 1;
	int    use_ct_list = 0;
	IterCounter cntr;
	RAssocArray ct_list;
	BExtQuery * q = 0;
	DBQ * dbq = 0;
	BExtInsert * p_bei = 0;
	SCardTbl * p_c = SCObj.P_Tbl;
	if(!(Filt.Flags & SCardFilt::fNoTempTable)) {
		THROW(P_TmpTbl = CreateTempFile <TempSCardTbl> ());
	}
	// @v9.8.9 dbq = ppcheckfiltid(dbq, p_c->SeriesID, Filt.SeriesID);
	dbq = ppcheckfiltid(dbq, p_c->SeriesID, SeriesList.GetSingle()); // @v9.8.9
	dbq = ppcheckfiltid(dbq, p_c->PersonID, Filt.PersonID);
	dbq = ppcheckfiltid(dbq, p_c->LocID, Filt.LocID); // @v9.4.5
	dbq = &daterange(p_c->Dt, &Filt.IssuePeriod);
	dbq = &daterange(p_c->Expiry, &Filt.ExpiryPeriod);
	if(!Filt.PersonID && Filt.EmployerID)
		dbq = & (*dbq && p_c->PersonID > 0L);
	{
		StrAssocArray n_list;
		SCardTbl::Rec rec_;
		PPIDArray incl_list;
		int    use_list = 0;
		if(P_TmpTbl)
			THROW_MEM(p_bei = new BExtInsert(P_TmpTbl));
		if(Filt.Number.NotEmpty()) {
			SCObj.GetListBySubstring(Filt.Number, /*Filt.SeriesID*/SeriesList.GetSingle(), &n_list, BIN(Filt.Flags & SCardFilt::fNumberFromBeg));
			for(uint i = 0; i < n_list.getCount(); i++) {
				StrAssocArray::Item n_list_item = n_list.at_WithoutParent(i);
				incl_list.addUnique(n_list_item.Id);
			}
			use_list = 1;
		}
		if(Filt.P_SjF && !Filt.P_SjF->IsEmpty()) {
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			PPIDArray local_list;
			Filt.P_SjF->Period.Actualize(ZERODATE);
			THROW(p_sj->GetObjListByEventPeriod(PPOBJ_SCARD, Filt.P_SjF->UserID,
				&Filt.P_SjF->ActionIDList, &Filt.P_SjF->Period, local_list));
			if(use_list) {
				incl_list.intersect(&local_list);
				use_list = 1;
			}
		}
		if(use_list) {
			for(uint i = 0; i < incl_list.getCount(); i++) {
				const PPID sc_id = incl_list.get(i);
				if(SCObj.Search(sc_id, &rec_) > 0) {
					TempSCardTbl::Rec rec;
					if(PreprocessTempRec(&rec_, &rec, (use_ct_list ? &ct_list : 0))) {
						if(p_bei) {
							THROW_DB(p_bei->insert(&rec));
						}
						else {
							THROW_SL(List.Add(rec.ID, rec.SeriesID, rec.Code));
						}
					}
				}
				PPWaitPercent(cntr.Increment());
				THROW(PPCheckUserBreak());
			}
		}
		else {
			int    idx = 2;
			union {
				SCardTbl::Key2 k2;
				SCardTbl::Key3 k3;
				SCardTbl::Key4 k4;
			} k, k_;
			MEMSZERO(k);
			if(Filt.LocID) {
				idx = 4;
				k.k4.LocID = Filt.LocID;
			}
			else if(Filt.PersonID) {
				idx = 3;
				k.k3.PersonID = Filt.PersonID;
			}
			else {
				idx = 2;
				k.k2.SeriesID = SeriesList.GetSingle(); //Filt.SeriesID;
			}
			THROW_MEM(q = new BExtQuery(p_c, idx));
			q->selectAll().where(*dbq);
			k_ = k;
			cntr.Init(q->countIterations(0, &k_, spGe));
			for(q->initIteration(0, &k, spGe); q->nextIteration();) {
				TempSCardTbl::Rec rec;
				p_c->copyBufTo(&rec_);
				if(PreprocessTempRec(&rec_, &rec, (use_ct_list ? &ct_list : 0))) {
					if(p_bei) {
						THROW_DB(p_bei->insert(&rec));
					}
					else {
						THROW_SL(List.Add(rec.ID, rec.SeriesID, rec.Code));
					}
				}
				PPWaitPercent(cntr.Increment());
				THROW(PPCheckUserBreak());
			}
		}
		if(p_bei)
			THROW_DB(p_bei->flash());
	}
	if(P_TmpTbl) {
		THROW(CreateOrderTable(Filt.Order, &P_TempOrd));
	}
	CATCH
		BExtQuery::ZDelete(&q);
		ZDELETE(P_TmpTbl);
		ZDELETE(P_TempOrd);
		ok = 0;
	ENDCATCH
	delete p_bei;
	delete q;
	return ok;
}

int SLAPI PPViewSCard::CheckForFilt(const SCardTbl::Rec * pRec, PreprocessScRecBlock * pBlk)
{
	int    ok = 1;
	double in_turnover = 0.0;
	double turnover = 0.0;
	const int wo_owner = BIN(Filt.Flags & SCardFilt::fWoOwner);
	/* @v9.8.9 if(Filt.ScsList.IsExists()) {
		THROW(Filt.ScsList.CheckID(pRec->SeriesID));
	}
	else {
		THROW(!Filt.SeriesID || pRec->SeriesID == Filt.SeriesID);
	} */
	// @v9.8.9 {
	if(SeriesList.IsExists()) {
		THROW(SeriesList.CheckID(pRec->SeriesID));
	}
	// } @v9.8.9 
	THROW(!Filt.PersonID || pRec->PersonID == Filt.PersonID);
	THROW(!Filt.LocID || pRec->LocID == Filt.LocID); // @v9.4.5
	THROW(Filt.Ft_Closed <= 0 || (pRec->Flags & SCRDF_CLOSED));
	THROW(Filt.Ft_Closed >= 0 || !(pRec->Flags & SCRDF_CLOSED));
	THROW(Filt.Ft_Inherited <= 0 || (pRec->Flags & SCRDF_INHERITED));
	THROW(Filt.Ft_Inherited >= 0 || !(pRec->Flags & SCRDF_INHERITED));
	THROW(!wo_owner || !pRec->PersonID);
	if(Filt.PDisR.low != Filt.PDisR.upp || Filt.PDisR.low > 0.0) {
		THROW(pRec->PDis >= Filt.PDisR.low);
		THROW(Filt.PDisR.upp <= 0.0 || pRec->PDis <= Filt.PDisR.upp)
	}
	THROW(/*!pRec->Dt ||*/ Filt.IssuePeriod.CheckDate(pRec->Dt)); // @v9.6.2 @fix /**/
	THROW(/*!pRec->Expiry ||*/ Filt.ExpiryPeriod.CheckDate(pRec->Expiry)); // @v9.6.2 @fix /**/
	THROW(!ExcludeOwnerList.IsExists() || !ExcludeOwnerList.CheckID(pRec->PersonID));
	{
		size_t ss_len = Filt.Number.Len();
		if(ss_len) {
			if(Filt.Flags & SCardFilt::fNumberFromBeg) {
				THROW(strncmp(pRec->Code, Filt.Number, ss_len) == 0);
			}
			else {
				THROW(ExtStrSrch(pRec->Code, Filt.Number));
			}
		}
	}
	if(Filt.EmployerID) {
		SETIFZ(P_StffObj, new PPObjStaffList);
		if(P_StffObj) {
			THROW(pRec->PersonID && P_StffObj->GetPostByPersonList(pRec->PersonID, Filt.EmployerID, 1, 0) > 0);
		}
	}
	if(Filt.P_SjF && !Filt.P_SjF->IsEmpty()) {
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		THROW(!p_sj || p_sj->CheckObjForFilt(PPOBJ_SCARD, pRec->ID, Filt.P_SjF) > 0);
	}
	if(!Filt.TrnovrPeriod.IsZero() || Filt.Flags & SCardFilt::fSinceLastPDisUpdating) {
		DateRange period = Filt.TrnovrPeriod;
		if(Filt.Flags & SCardFilt::fSinceLastPDisUpdating)
			GetPDisUpdateDate(pRec->ID, period.upp, &period.low);
		SCObj.GetTurnover(*pRec, PPObjSCard::gtalgDefault, period, 0, &in_turnover, &turnover);
	}
	// @v8.5.3 @fix {
	else {
		in_turnover = pRec->InTrnovr;
		turnover = pRec->Turnover;
	}
	// } @v8.5.3 @fix
	if(Filt.TurnoverR.low != Filt.TurnoverR.upp || Filt.TurnoverR.low != 0.0) {
		THROW(Filt.TurnoverR.low == 0.0 || turnover >= Filt.TurnoverR.low)
		THROW(Filt.TurnoverR.upp == 0.0 || turnover <= Filt.TurnoverR.upp)
	}
	CATCHZOK
	if(pBlk) {
		pBlk->ScID = pRec->ID;
		pBlk->InTurnover = in_turnover;
		pBlk->Turnover = turnover;
	}
	return ok;
}

int SLAPI PPViewSCard::PreprocessTempRec(const SCardTbl::Rec * pSrcRec, TempSCardTbl::Rec * pDestRec, RAssocArray * pTrnovrList/*, int calcTrnovr*/)
{
	int    ok = 1;
	PreprocessScRecBlock pblk;
	THROW(pSrcRec);
	THROW(pDestRec);
	THROW(CheckForFilt(pSrcRec, &pblk));
#define CPYFLD(f) pDestRec->f = pSrcRec->f
	memzero(pDestRec, sizeof(*pDestRec));
	CPYFLD(ID);
	CPYFLD(SeriesID);
	CPYFLD(PersonID);
	CPYFLD(LocID); // @v9.4.0
	CPYFLD(Flags);
	CPYFLD(Dt);
	CPYFLD(Expiry);
	CPYFLD(PDis);
	CPYFLD(AutoGoodsID);
	CPYFLD(MaxCredit);
	CPYFLD(Turnover);
	CPYFLD(Rest);
	CPYFLD(InTrnovr);
	CPYFLD(UsageTmStart);
	CPYFLD(UsageTmEnd);
	STRNSCPY(pDestRec->Code, pSrcRec->Code);
	// @v9.4.0 STRNSCPY(pDestRec->Password, pSrcRec->Password);
#undef CPYFLD
	pDestRec->InTrnovr = pblk.InTurnover;
	pDestRec->Turnover = pblk.Turnover;
	if(Filt.Flags & SCardFilt::fShowOwnerAddrDetail) {
		PersonTbl::Rec psn_rec;
		PPID   addr_id = 0;
		if(pDestRec->PersonID && PsnObj.Search(pDestRec->PersonID, &psn_rec) > 0) {
			SString temp_buf, temp_buf2, result_buf;
			{
				PPELinkArray ela;
				PsnObj.P_Tbl->GetELinks(psn_rec.ID, &ela);
				ela.GetPhones(3, result_buf.Z());
				//result_buf.CopyTo(pDestRec->Phone, sizeof(pDestRec->Phone));
				StrPool.AddS(result_buf, &pDestRec->PhoneP); // @v9.8.9
			}
			addr_id = NZOR(psn_rec.RLoc, psn_rec.MainLoc);
			LocationTbl::Rec loc_rec;
			if(addr_id && PsnObj.LocObj.Search(addr_id, &loc_rec) > 0) {
				LocationCore::GetAddress(loc_rec, 0, temp_buf);
				if(temp_buf.NotEmptyS()) {
					//temp_buf.CopyTo(pDestRec->Address, sizeof(pDestRec->Address));
					StrPool.AddS(temp_buf, &pDestRec->AddressP); // @v9.8.9
					Las.Recognize(temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
					if(Las.GetText(PPLocAddrStruc::tZip, temp_buf)) {
						temp_buf.Transf(CTRANSF_OUTER_TO_INNER); //.CopyTo(pDestRec->ZIP, sizeof(pDestRec->ZIP));
						StrPool.AddS(result_buf, &pDestRec->ZipP); // @v9.8.9
					}
					if(Las.GetText(PPLocAddrStruc::tLocalArea, temp_buf)) {
						result_buf.Z();
						if(Las.GetText(PPLocAddrStruc::tLocalAreaKind, temp_buf2))
							result_buf.Cat(temp_buf2).Space();
						result_buf.Cat(temp_buf).Transf(CTRANSF_OUTER_TO_INNER); //.CopyTo(pDestRec->LocalArea, sizeof(pDestRec->LocalArea));
						StrPool.AddS(result_buf, &pDestRec->LocalAreaP); // @v9.8.9
					}
					if(Las.GetText(PPLocAddrStruc::tCity, temp_buf)) {
						result_buf.Z();
						if(Las.GetText(PPLocAddrStruc::tCityKind, temp_buf2))
							result_buf.Cat(temp_buf2).Space();
						result_buf.Cat(temp_buf).Transf(CTRANSF_OUTER_TO_INNER); //.CopyTo(pDestRec->City, sizeof(pDestRec->City));
						StrPool.AddS(result_buf, &pDestRec->CityP); // @v9.8.9
					}
					if(Las.GetText(PPLocAddrStruc::tStreet, temp_buf)) {
						result_buf.Z();
						if(Las.GetText(PPLocAddrStruc::tStreetKind, temp_buf2))
							result_buf.Cat(temp_buf2).Space();
						result_buf.Cat(temp_buf).Transf(CTRANSF_OUTER_TO_INNER); //.CopyTo(pDestRec->Street, sizeof(pDestRec->Street));
						StrPool.AddS(result_buf, &pDestRec->StreetP); // @v9.8.9
					}
					if(Las.GetText(PPLocAddrStruc::tHouse, temp_buf)) {
						result_buf.Z();
						if(Las.GetText(PPLocAddrStruc::tHouseKind, temp_buf2))
							result_buf.Cat(temp_buf2).Space();
						result_buf.Cat(temp_buf);
						if(Las.GetText(PPLocAddrStruc::tHouseAddendum, temp_buf)) {
							result_buf.Space();
							if(Las.GetText(PPLocAddrStruc::tHouseAddendumKind, temp_buf2))
								result_buf.Cat(temp_buf2).Space();
							result_buf.Cat(temp_buf);
						}
						result_buf.Transf(CTRANSF_OUTER_TO_INNER); //.CopyTo(pDestRec->House, sizeof(pDestRec->House));
						StrPool.AddS(result_buf, &pDestRec->HouseP); // @v9.8.9
					}
					if(Las.GetText(PPLocAddrStruc::tApart, temp_buf)) {
						result_buf.Z();
						if(Las.GetText(PPLocAddrStruc::tApartKind, temp_buf2))
							result_buf.Cat(temp_buf2).Space();
						result_buf.Cat(temp_buf).Transf(CTRANSF_OUTER_TO_INNER); //.CopyTo(pDestRec->Apart, sizeof(pDestRec->Apart));
						StrPool.AddS(result_buf, &pDestRec->ApartP); // @v9.8.9
					}
					if(Las.GetText(PPLocAddrStruc::tAddendum, temp_buf)) {
						temp_buf.Transf(CTRANSF_OUTER_TO_INNER); //.CopyTo(pDestRec->AddrAddend, sizeof(pDestRec->AddrAddend));
						StrPool.AddS(temp_buf, &pDestRec->AddrAddendP); // @v9.8.9
					}
				}
			}
		}
		if(Filt.Flags & SCardFilt::fWithAddressOnly && !addr_id)
			ok = 0;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewSCard::CreateOrderTable(long ord, TempOrderTbl ** ppTbl)
{
	int    ok = 1;
	TempOrderTbl * p_o = 0;
	ZDELETE(P_TempOrd);
	*ppTbl = 0;
	if(ord == OrdBySeries_Code)
		ok = -1;
	else {
		SCardViewItem item;
		THROW(p_o = CreateTempOrderFile());
		{
			BExtInsert bei(p_o);
			for(InitIteration(); NextIteration(&item) > 0;) {
				SCardTbl::Rec sc_rec;
				TempOrderTbl::Rec ord_rec;
				MEMSZERO(sc_rec);
				sc_rec.ID       = item.ID;
				sc_rec.PersonID = item.PersonID;
				sc_rec.PDis     = item.PDis;
				sc_rec.Turnover = item.Turnover;
				sc_rec.Expiry   = item.Expiry;
				MakeTempOrdEntry(ord, &sc_rec, &ord_rec);
				THROW_DB(bei.insert(&ord_rec));
			}
			THROW_DB(bei.flash());
		}
		*ppTbl = p_o;
		p_o = 0;
	}
	CATCHZOK
	delete p_o;
	return ok;
}

int SLAPI PPViewSCard::MakeTempOrdEntry(long ord, const SCardTbl::Rec * pRec, TempOrderTbl::Rec * pOrdRec)
{
	int    ok = -1;
	if(pRec && pOrdRec) {
		SString temp_buf, psn_name;
		const double large_val = 1e12;
		const char * p_fmt = "%030.8lf";
		TempOrderTbl::Rec ord_rec;
		MEMSZERO(ord_rec);
		ord_rec.ID = pRec->ID;
		if(ord == OrdByPerson) {
			GetPersonName(pRec->PersonID, temp_buf);
			temp_buf.CopyTo(ord_rec.Name, sizeof(ord_rec.Name));
		}
		else if(ord == OrdByDiscount)
			sprintf(ord_rec.Name, p_fmt, large_val-pRec->PDis);
		else if(ord == OrdByTrnovr)
			sprintf(ord_rec.Name, p_fmt, large_val-pRec->Turnover);
		// @v8.4.2 {
		else if(ord == OrdByExpiry) {
			GetPersonName(pRec->PersonID, psn_name);
			temp_buf.Z();
			if(pRec->Expiry)
				temp_buf.Cat(pRec->Expiry, DATF_NODIV|DATF_YMD|DATF_CENTURY);
			else
				temp_buf.CatCharN('0', 8);
			temp_buf.Cat(psn_name).CopyTo(ord_rec.Name, sizeof(ord_rec.Name));
		}
		// } @v8.4.2
		ASSIGN_PTR(pOrdRec, ord_rec);
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewSCard::UpdateTempTable(PPIDArray * pIdList)
{
	int    ok = 1;
	if(P_TmpTbl || P_TempOrd) {
		if(pIdList) {
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			for(uint i = 0; i < pIdList->getCount(); i++) {
				PPID _id = pIdList->at(i);
				TempSCardTbl::Rec temp_rec;
				if(SCObj.P_Tbl->search(0, &_id, spEq) > 0 && PreprocessTempRec(&SCObj.P_Tbl->data, &temp_rec, 0)) {
					PPID   temp_key = _id;
					THROW(PPSetDbRecordByKey(P_TmpTbl, 0, &temp_key, &temp_rec, 0));
					if(P_TempOrd) {
						TempOrderTbl::Rec ord_rec;
						MakeTempOrdEntry(Filt.Order, &SCObj.P_Tbl->data, &ord_rec);
						temp_key = _id;
						THROW(PPSetDbRecordByKey(P_TempOrd, 0, &temp_key, &ord_rec, 0));
					}
				}
				else {
					if(P_TmpTbl) {
						THROW_DB(deleteFrom(P_TmpTbl, 0, P_TmpTbl->ID == _id));
					}
					if(P_TempOrd) {
						THROW_DB(deleteFrom(P_TempOrd, 0, P_TempOrd->ID == _id));
					}
				}
			}
			THROW(tra.Commit());
		}
		else if(P_TmpTbl)
			THROW(CreateTempTable());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewSCard::InitIteration()
{
	int    ok = 1;
	DBQ  * dbq = 0;
	BExtQuery::ZDelete(&P_IterQuery);
	if(P_TempOrd) {
		TempOrderTbl::Key1 k;
		MEMSZERO(k);
		THROW_MEM(P_IterQuery = new BExtQuery(P_TempOrd, 1));
		P_IterQuery->selectAll();
		P_IterQuery->initIteration(0, &k, spFirst);
		PPInitIterCounter(Counter, P_TempOrd);
	}
	else {
		int    idx = 2;
		union {
			SCardTbl::Key2 k2;
			SCardTbl::Key3 k3;
			SCardTbl::Key4 k4;
		} k, k_;
		MEMSZERO(k);
		if(P_TmpTbl) {
			idx = 2;
			P_IterQuery = new BExtQuery(P_TmpTbl, idx);
			P_IterQuery->selectAll().where(*dbq);
			PPInitIterCounter(Counter, P_TmpTbl);
		}
		else {
			if(Filt.LocID) {
				idx = 4;
				k.k4.LocID = Filt.LocID;
			}
			else if(Filt.PersonID) {
				idx = 3;
				k.k3.PersonID = Filt.PersonID;
			}
			else {
				idx = 2;
				k.k2.SeriesID = SeriesList.GetSingle(); //Filt.SeriesID;
			}
			k_ = k;
			SCardTbl * p_c = SCObj.P_Tbl;
			P_IterQuery = new BExtQuery(p_c, idx);
			dbq = ppcheckfiltid(dbq, p_c->SeriesID, SeriesList.GetSingle()/*Filt.SeriesID*/);
			if(Filt.Flags & SCardFilt::fWoOwner)
				dbq = & (*dbq && p_c->PersonID == 0L);
			else
				dbq = ppcheckfiltid(dbq, p_c->PersonID, Filt.PersonID);
			dbq = ppcheckfiltid(dbq, p_c->LocID, Filt.LocID); // @v9.4.5
			dbq = & (*dbq && realrange(p_c->PDis, Filt.PDisR.low, Filt.PDisR.upp));
			dbq = & (*dbq && realrange(p_c->Turnover, Filt.TurnoverR.low, Filt.TurnoverR.upp));
			P_IterQuery->selectAll().where(*dbq);
			//k2.SeriesID = Filt.SeriesID;
			Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
		}
		P_IterQuery->initIteration(0, &k, spGe);
	}
	CATCHZOK
	return ok;
}

int FASTCALL PPViewSCard::NextIteration(SCardViewItem * pItem)
{
	while(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		Counter.Increment();
		SCardTbl::Rec rec;
		TempSCardTbl::Rec temp_rec;
		int    r = 0;
		if(P_TempOrd) {
			PPID sc_id = P_TempOrd->data.ID;
			if(P_TmpTbl) {
				if(SearchByID(P_TmpTbl, 0, sc_id, &temp_rec) > 0) {
					memcpy(&rec, &temp_rec, sizeof(rec));
					r = 1;
				}
			}
			else if(SearchByID(SCObj.P_Tbl, 0, sc_id, &rec) > 0) {
				r = 1;
			}
		}
		else if(P_TmpTbl) {
			P_TmpTbl->copyBufTo(&temp_rec);
			memcpy(&rec, &temp_rec, sizeof(rec));
			r = 1;
		}
		else {
			SCObj.P_Tbl->copyBufTo(&rec);
			r = 1;
		}
		if(r > 0 && CheckForFilt(&rec, 0)) {
			if(pItem) {
				*((SCardTbl::Rec *)pItem) = rec;
				pItem->EmployerID = 0;
				if(pItem->PersonID && !(Filt.Flags & SCardFilt::fNoEmployer)) {
					PersonPostArray post_list;
					SETIFZ(P_StffObj, new PPObjStaffList);
					if(P_StffObj->GetPostByPersonList(pItem->PersonID, 0, 1, &post_list) > 0) {
						PPStaffEntry sl_rec;
						if(P_StffObj->Fetch(post_list.at(0).StaffID, &sl_rec) > 0)
							pItem->EmployerID = sl_rec.OrgID;
					}
				}
			}
			return 1;
		}
	}
	return -1;
}

class SCardFiltDialog : public TDialog {
public:
	SCardFiltDialog() : TDialog(DLG_SCARDFLT)
	{
		LastFlagsState = 0;
		SetupCalPeriod(CTLCAL_SCARDFLT_TRNOVRPRD, CTL_SCARDFLT_TRNOVRPRD);
		SetupCalPeriod(CTLCAL_SCARDFLT_ISSUE, CTL_SCARDFLT_ISSUE);
		SetupCalPeriod(CTLCAL_SCARDFLT_EXPIRY, CTL_SCARDFLT_EXPIRY);
	}
	int    setDTS(const SCardFilt *);
	int    getDTS(SCardFilt *);
private:
	DECL_HANDLE_EVENT;
	void   SetupPerson(PPID series);
	void   SetupCtrls();

	long   LastFlagsState;
	SCardFilt Data;
	PPObjSCardSeries ObjSCardSer;
};

int SCardFiltDialog::setDTS(const SCardFilt * pData)
{
	if(!RVALUEPTR(Data, pData))
		MEMSZERO(Data);
	const double min_dis = R2(fdiv100r(Data.PDisR.low));
	const double max_dis = R2(fdiv100r(Data.PDisR.upp));
	SetupPPObjCombo(this, CTLSEL_SCARDFLT_SERIES, PPOBJ_SCARDSERIES, Data.SeriesID, 0);
	SetupPPObjCombo(this, CTLSEL_SCARDFLT_EMPLOYER, PPOBJ_PERSON, Data.EmployerID, OLW_CANINSERT, (void *)PPPRK_EMPLOYER);
	SetupPerson(Data.SeriesID);
	SetPeriodInput(this, CTL_SCARDFLT_ISSUE,  &Data.IssuePeriod);
	SetPeriodInput(this, CTL_SCARDFLT_EXPIRY, &Data.ExpiryPeriod);
	SetPeriodInput(this, CTL_SCARDFLT_TRNOVRPRD, &Data.TrnovrPeriod);
	AddClusterAssoc(CTL_SCARDFLT_SINCE, 0, SCardFilt::fSinceLastPDisUpdating);
	SetClusterData(CTL_SCARDFLT_SINCE, Data.Flags);
	{
		long   _f = 0;
		if(Data.Ft_Closed < 0)
			_f |= 0x01;
		else if(Data.Ft_Closed > 0)
			_f |= 0x02;
		SETFLAG(_f, 0x04, (Data.Flags & SCardFilt::fShowOwnerAddrDetail));
		SETFLAG(_f, 0x08, (Data.Flags & SCardFilt::fWithAddressOnly));
		AddClusterAssoc(CTL_SCARDFLT_FLAGS, 0, 0x01);
		AddClusterAssoc(CTL_SCARDFLT_FLAGS, 1, 0x02);
		AddClusterAssoc(CTL_SCARDFLT_FLAGS, 2, 0x04);
		AddClusterAssoc(CTL_SCARDFLT_FLAGS, 3, 0x08);
		SetClusterData(CTL_SCARDFLT_FLAGS, _f);
		DisableClusterItem(CTL_SCARDFLT_FLAGS, 3, !(_f & 0x04));
		LastFlagsState = _f;
	}
	SetRealRangeInput(this, CTL_SCARDFLT_PDISRANGE,   min_dis, max_dis);
	SetRealRangeInput(this, CTL_SCARDFLT_TRNOVRRANGE, &Data.TurnoverR);

	AddClusterAssocDef(CTL_SCARDFLT_ORD,  0, PPViewSCard::OrdBySeries_Code);
	AddClusterAssoc(CTL_SCARDFLT_ORD,  1, PPViewSCard::OrdByDiscount);
	AddClusterAssoc(CTL_SCARDFLT_ORD,  2, PPViewSCard::OrdByTrnovr);
	AddClusterAssoc(CTL_SCARDFLT_ORD,  3, PPViewSCard::OrdByPerson);
	AddClusterAssoc(CTL_SCARDFLT_ORD,  4, PPViewSCard::OrdByExpiry);
	SetClusterData(CTL_SCARDFLT_ORD, Data.Order);
	setCtrlString(CTL_SCARDFLT_NUMBER, Data.Number);
	AddClusterAssoc(CTL_SCARDFLT_WOOWNER, 0, SCardFilt::fWoOwner);
	SetClusterData(CTL_SCARDFLT_WOOWNER, Data.Flags);
	SetupCtrls();
	return 1;
}

int SCardFiltDialog::getDTS(SCardFilt * pData)
{
	int    ok = 1;
	double min_dis = R2(fdiv100r(Data.PDisR.low));
	double max_dis = R2(fdiv100r(Data.PDisR.upp));
	//long   v = 0;
	getCtrlData(CTLSEL_SCARDFLT_SERIES,   &Data.SeriesID);
	getCtrlData(CTLSEL_SCARDFLT_EMPLOYER, &Data.EmployerID);
	getCtrlData(CTLSEL_SCARDFLT_PERSON,   &Data.PersonID);
	GetPeriodInput(this, CTL_SCARDFLT_ISSUE,  &Data.IssuePeriod);
	GetPeriodInput(this, CTL_SCARDFLT_EXPIRY, &Data.ExpiryPeriod);
	GetPeriodInput(this, CTL_SCARDFLT_TRNOVRPRD, &Data.TrnovrPeriod);
	GetClusterData(CTL_SCARDFLT_SINCE, &Data.Flags);
	GetRealRangeInput(this, CTL_SCARDFLT_PDISRANGE, &min_dis, &max_dis);
	Data.PDisR.Set(R0(min_dis * 100), R0(max_dis * 100));
	GetRealRangeInput(this, CTL_SCARDFLT_TRNOVRRANGE, &Data.TurnoverR);
	{
		long   _f = 0;
		GetClusterData(CTL_SCARDFLT_FLAGS, &_f);
		if(_f & 0x01)
			Data.Ft_Closed = -1;
		else if(_f & 0x02)
			Data.Ft_Closed = 1;
		else
			Data.Ft_Closed = 0;
		SETFLAG(Data.Flags, SCardFilt::fShowOwnerAddrDetail, (_f & 0x04));
		SETFLAG(Data.Flags, SCardFilt::fWithAddressOnly,     (_f & 0x08));
	}
	GetClusterData(CTL_SCARDFLT_ORD, &Data.Order);
	getCtrlString(CTL_SCARDFLT_NUMBER, Data.Number);
	GetClusterData(CTL_SCARDFLT_WOOWNER, &Data.Flags);
	ASSIGN_PTR(pData, Data);
	return ok;
}

IMPL_HANDLE_EVENT(SCardFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_SCARDFLT_SERIES)) {
		getCtrlData(CTLSEL_SCARDFLT_SERIES, &Data.SeriesID);
		SetupPerson(getCtrlLong(CTLSEL_SCARDFLT_SERIES));
	}
	else if(event.isCbSelected(CTLSEL_SCARDFLT_PERSON))
		Data.PersonID = getCtrlLong(CTLSEL_SCARDFLT_PERSON);
	else if(event.isCmd(cmClusterClk) && event.isCtlEvent(CTL_SCARDFLT_WOOWNER))
		SetupCtrls();
	else if(event.isCmd(cmClusterClk) && event.isCtlEvent(CTL_SCARDFLT_FLAGS)) {
		long   _f = 0;
		GetClusterData(CTL_SCARDFLT_FLAGS, &_f);
		if(_f != LastFlagsState) {
			const long preserve__f = _f;
			if((_f & 0x01) != (LastFlagsState & 0x01)) {
				if(_f & 0x01)
					_f &= ~0x02;
			}
			else if((_f & 0x02) != (LastFlagsState & 0x02)) {
				if(_f & 0x02)
					_f &= ~0x01;
			}
			LastFlagsState = _f;
			if(_f != preserve__f)
				SetClusterData(CTL_SCARDFLT_FLAGS, _f);
			DisableClusterItem(CTL_SCARDFLT_FLAGS, 3, !(_f & 0x04));
		}
	}
	else if(event.isCmd(cmSysjFilt2)) {
		SysJournalFilt sj_filt;
		RVALUEPTR(sj_filt, Data.P_SjF);
		sj_filt.ObjType = PPOBJ_SCARD;
		if(EditSysjFilt2(&sj_filt) > 0) {
			SETIFZ(Data.P_SjF, new SysJournalFilt);
			ASSIGN_PTR(Data.P_SjF, sj_filt);
		}
		if(Data.P_SjF) {
			//
			// Функция SysJournalFilt::IsEmpty считает фильтр, в котором установлен ObjType
			// не пустым. В данном случае это - не верно.
			//
			Data.P_SjF->ObjType = 0;
			if(Data.P_SjF->IsEmpty()) {
				ZDELETE(Data.P_SjF);
			}
			else
				Data.P_SjF->ObjType = PPOBJ_SCARD;
		}
	}
	else if(event.isCmd(cmExcludeOwnerScFilt) && !(Data.Flags & SCardFilt::fInnerFilter)) {
		SCardFilt eosc_filt;
		if(Data.P_ExludeOwnerF)
			eosc_filt = *Data.P_ExludeOwnerF;
		eosc_filt.Flags |= SCardFilt::fInnerFilter; // @v8.4.3
		if(Helper_EditSCardFilt(&eosc_filt, 1) > 0) {
			SETIFZ(Data.P_ExludeOwnerF, new SCardFilt);
			ASSIGN_PTR(Data.P_ExludeOwnerF, eosc_filt);
		}
	}
	else
		return;
	clearEvent(event);
}

void SCardFiltDialog::SetupCtrls()
{
	GetClusterData(CTL_SCARDFLT_WOOWNER, &Data.Flags);
	if(Data.Flags & SCardFilt::fWoOwner) {
		PPID   psn_kind_id = 0, series;
		PPSCardSerPacket pack;
		getCtrlData(CTLSEL_SCARDFLT_SERIES, &series);
		Data.PersonID = 0;
		SetupPerson(series);
	}
	disableCtrl(CTLSEL_SCARDFLT_PERSON, BIN(Data.Flags & SCardFilt::fWoOwner));
	enableCommand(cmExcludeOwnerScFilt, BIN(!(Data.Flags & SCardFilt::fInnerFilter))); // @v8.4.3
	disableCtrl(CTL_SCARDFLT_ORD, BIN(Data.Flags & SCardFilt::fInnerFilter)); // @v8.4.3
}

void SCardFiltDialog::SetupPerson(PPID series)
{
	PPID   psn_kind_id = Data.GetOwnerPersonKind(); // @v9.8.0
	/* @v9.8.0
	PPSCardSerPacket pack;
	if(series && ObjSCardSer.GetPacket(series, &pack) > 0)
		psn_kind_id = pack.Rec.PersonKindID;
	*/
	SETIFZ(psn_kind_id, PPPRK_CLIENT);
	SetupPersonCombo(this, CTLSEL_SCARDFLT_PERSON, Data.PersonID, OLW_CANINSERT|OLW_LOADDEFONOPEN, psn_kind_id, 0); // @v9.8.0 -OLW_LOADDEFONOPEN
}

int SLAPI Helper_EditSCardFilt(SCardFilt * pFilt, int cascade)
{
	int    ok = -1;
	SCardFiltDialog * dlg = new SCardFiltDialog;
	if(CheckDialogPtrErr(&dlg)) {
		if(cascade)
			dlg->ToCascade();
		if(dlg->setDTS(pFilt)) {
			while(ok <= 0 && ExecView(dlg) == cmOK)
				if(dlg->getDTS(pFilt))
					ok = 1;
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SLAPI PPViewSCard::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = 0;
	if(Filt.IsA(pBaseFilt)) {
		ok = SCObj.CheckRights(PPR_READ) ? Helper_EditSCardFilt((SCardFilt *)pBaseFilt, 0) : PPErrorZ();
	}
	return ok;
}

void * SLAPI PPViewSCard::GetEditExtraParam()
{ 
	//return (void *)Filt.SeriesID; 
	return (void *)SeriesList.GetSingle();
}

int SLAPI PPViewSCard::DeleteItem(PPID id)
{
	int    ok = -1;
	RemoveAllDialog * p_dlg = 0;
	if(id)
		ok = (SCObj.RemoveObjV(id, 0, PPObject::rmv_default, 0) > 0) ? 1 : -1;
	else {
		if(SCObj.CheckRights(PPR_DEL)) {
			int    valid_data = 0;
			RemoveAllParam param;
			param.Action = RemoveAllParam::aMoveToGroup;
			param.DestGrpID = 0;
			if(CheckDialogPtrErr(&(p_dlg = new RemoveAllDialog(DLG_SCARDRMVALL))) > 0) {
				p_dlg->setDTS(&param);
				while(!valid_data && ExecView(p_dlg) == cmOK) {
					if(p_dlg->getDTS(&param))
						valid_data = 1;
				}
				if(valid_data == 1) {
					PPWait(1);
					SCardViewItem item;
					PPIDArray id_list;
					for(InitIteration(); NextIteration(&item) > 0;)
						if(item.ID)
							id_list.addUnique(item.ID);
					for(uint i = 0; i < id_list.getCount(); i++) {
						int r = 0;
						if(param.Action == RemoveAllParam::aMoveToGroup) {
							SCardTbl::Rec scard_rec;
							if(SCObj.Search(id_list.at(i), &scard_rec) > 0) {
								scard_rec.SeriesID = param.DestGrpID;
								r = SCObj.P_Tbl->Update(scard_rec.ID, &scard_rec, 1);
							}
						}
						else if(param.Action == RemoveAllParam::aRemoveAll)
							r = SCObj.RemoveObjV(id_list.at(i), 0, PPObject::use_transaction | PPObject::no_wait_indicator, 0);
						ok = 1;
						PPWaitPercent(i, id_list.getCount());
					}
					PPWait(0);
				}
			}
		}
		else
			ok = PPErrorZ();
	}
	delete p_dlg;
	return ok;
}

int SLAPI PPViewSCard::RecalcRests()
{
	int    ok = -1;
	if(/*Filt.SeriesID*/SeriesList.GetSingle()) {
		PPWait(1);
		if(SCObj.CheckRights(PPR_MOD) && SCObj.P_Tbl->RecalcRestsBySeries(/*Filt.SeriesID*/SeriesList.GetSingle(), 1))
			ok = 1;
		else
			ok = PPErrorZ();
		PPWait(0);
	}
	return ok;
}

int SLAPI PPViewSCard::RenameDup(PPIDArray * pIdList)
{
	int    ok = -1;
	uint   replace_trnovr = 1;
	CALLPTRMEMB(pIdList, freeAll());
	if(/*Filt.SeriesID*/SeriesList.GetSingle() && SelectorDialog(DLG_RENMDUPLSC, CTL_RENMDUPLSC_FLAGS, &replace_trnovr) > 0) {
		SCardTbl::Key1 k1;
		LAssocArray dupl_ary;
		SCardViewItem item;
		PPViewSCard v_sc;
		THROW(v_sc.Init_(&Filt));
		PPWait(1);
		for(v_sc.InitIteration(); v_sc.NextIteration(&item) > 0;) {
			MEMSZERO(k1);
			STRNSCPY(k1.Code, item.Code);
			for(; SCObj.P_Tbl->search(1, &k1, spGt) > 0 && stricmp866(item.Code, k1.Code) == 0;)
				if(SCObj.P_Tbl->data.ID != item.ID) {
					THROW_SL(dupl_ary.Add(item.ID, SCObj.P_Tbl->data.ID, 0));
					break;
				}
		}
		{
			SString code;
			SString temp_buf;
			CCheckCore & r_cc = *SCObj.P_CcTbl;
			PPTransaction tra(1);
			THROW(tra);
			for(uint i = 0; i < dupl_ary.getCount(); i++) {
				size_t p = 0;
				PPID   dupl_scard = dupl_ary.at(i).Key;
				PPID   dest_scard = dupl_ary.at(i).Val;
				long   code_postfx;
				SCardTbl::Rec rec;
				MEMSZERO(rec);
				MEMSZERO(k1);
				THROW(SCObj.Search(dupl_scard, &rec) > 0);
				code = rec.Code;
				if(code.StrChr('#', &p))
					code.Trim(p + 1);
				else {
					code.CatChar('#');
					p = code.Len() - 1;
				}
				code.CopyTo(k1.Code, sizeof(k1.Code));
				for(code_postfx = 1; SCObj.P_Tbl->search(1, &k1, spGt) > 0 && strnicmp866(code, k1.Code, sstrlen(code)) == 0;) {
					long   n = 0;
					temp_buf = SCObj.P_Tbl->data.Code;
					code_postfx = ((n = temp_buf.ShiftLeft(p + 1).ToLong() + 1) >= code_postfx) ? n : code_postfx;
				}
				// @v9.4.9 sprintf(rec.Code, "%s%ld", code.cptr(), code_postfx);
				(temp_buf = code).Cat(code_postfx).CopyTo(rec.Code, sizeof(rec.Code)); // @v9.4.9
				THROW(SCObj.P_Tbl->Update(dupl_scard, &rec, 0));
				if(replace_trnovr)
					THROW(r_cc.ReplaceSCard(dest_scard, dupl_scard, 0));
				CALLPTRMEMB(pIdList, add(dupl_scard));
			}
			THROW(tra.Commit());
		}
		ok = 1;
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int SLAPI PPViewSCard::RecalcTurnover()
{
	int    ok = -1;
	if(CONFIRM(PPCFM_RECALCSCARDTRNOVR)) {
		if(SCObj.IsCreditSeries(/*Filt.SeriesID*/SeriesList.GetSingle())) {
			THROW(ok = RecalcRests());
		}
		else {
			CCheckCore & r_cc = *SCObj.P_CcTbl;
			THROW(SCObj.CheckRights(PPR_MOD));
			PPWait(1);
			THROW(r_cc.RecalcSCardsTurnover(1));
			ok = 1;
			PPWait(0);
		}
	}
	CATCHZOKPPERR
	return ok;
}

struct SCardChrgCrdParam {
	enum {
		actionAdd = 0,
		actionExtendTo,
		actionUhttSync
	};
	LDATE  Dt;
	long   Action;
	double Amount;
};

static int SLAPI EditChargeCreditParam(int enableUhttSync, SCardChrgCrdParam * pData)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_SCCHRGCRD);
	if(CheckDialogPtrErr(&dlg)) {
		ushort v = 0;
		dlg->SetupCalDate(CTLCAL_SCCHRGCRD_DATE, CTL_SCCHRGCRD_DATE);
		dlg->setCtrlData(CTL_SCCHRGCRD_DATE, &pData->Dt);
		dlg->setCtrlData(CTL_SCCHRGCRD_AMOUNT, &pData->Amount);
		dlg->AddClusterAssocDef(CTL_SCCHRGCRD_WHAT, 0, SCardChrgCrdParam::actionAdd);
		dlg->AddClusterAssoc(CTL_SCCHRGCRD_WHAT, 1, SCardChrgCrdParam::actionExtendTo);
		dlg->AddClusterAssoc(CTL_SCCHRGCRD_WHAT, 2, SCardChrgCrdParam::actionUhttSync);
		dlg->DisableClusterItem(CTL_SCCHRGCRD_WHAT, 2, !enableUhttSync);
		dlg->SetClusterData(CTL_SCCHRGCRD_WHAT, pData->Action);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			pData->Action = dlg->GetClusterData(CTL_SCCHRGCRD_WHAT);
			dlg->getCtrlData(CTL_SCCHRGCRD_DATE, &pData->Dt);
			if(!checkdate(pData->Dt, 0))
				PPErrorByDialog(dlg, CTL_SCCHRGCRD_DATE, PPERR_SLIB);
			else {
				dlg->getCtrlData(CTL_SCCHRGCRD_AMOUNT, &pData->Amount);
				if(pData->Action != SCardChrgCrdParam::actionExtendTo && (
					(pData->Amount != 0.0 && fabs(pData->Amount) < fpow10i(-2)) || fabs(pData->Amount) > fpow10i(6) ||
					(pData->Action == SCardChrgCrdParam::actionExtendTo && pData->Amount < 0.0))) {
					PPErrorByDialog(dlg, CTL_SCCHRGCRD_AMOUNT, PPERR_INVAMOUNT);
				}
				else
					ok = valid_data = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SLAPI PPViewSCard::ChargeCredit()
{
	int    ok = -1;
	int    scst = 0;
	int    uhtt_sync = 0;
	PPUhttClient * p_uhtt_cli = 0;
	if(/*Filt.SeriesID*/SeriesList.GetSingle()) {
		PPObjSCardSeries scs_obj;
		PPSCardSeries scs_rec;
		if(scs_obj.Fetch(/*Filt.SeriesID*/SeriesList.GetSingle(), &scs_rec) > 0) {
			scst = scs_rec.GetType();
			if(scs_rec.Flags & SCRDSF_UHTTSYNC)
				uhtt_sync = 1;
		}
	}
	if(oneof2(scst, scstCredit, scstBonus)) {
		SCardChrgCrdParam param;
		MEMSZERO(param);
		param.Dt = LConfig.OperDate;
		THROW(SCObj.CheckRights(SCRDRT_ADDOPS));
		if(EditChargeCreditParam(uhtt_sync, &param) > 0) {
			PPLogger logger;
			long   inc = 0;
			TSVector <SCardCore::UpdateRestNotifyEntry> urn_list; // @v9.8.4 TSArray-->TSVector
			SCardViewItem item;
			PPWait(1);
			if(uhtt_sync) {
				THROW_MEM(p_uhtt_cli = new PPUhttClient);
				if(!p_uhtt_cli->Auth()) {
					ZDELETE(p_uhtt_cli);
					logger.LogLastError();
				}
			}
			{
				PPTransaction tra(1);
				THROW(tra);
				for(InitIteration(); NextIteration(&item) > 0;) {
					double rest = 0.0;
					double amount = 0.0;
					UhttSCardPacket scp;
					SCardOpTbl::Rec scop_rec;
					MEMSZERO(scop_rec);
					scop_rec.SCardID = item.ID;
					scop_rec.Dt = param.Dt;
					getcurtime(&scop_rec.Tm);
					scop_rec.Tm.v += inc;
					inc += 200;
					scop_rec.UserID = LConfig.User;
					if(param.Action == SCardChrgCrdParam::actionUhttSync) {
						if(uhtt_sync && p_uhtt_cli) {
							double uhtt_rest = 0.0;
							THROW(SCObj.P_Tbl->GetRest(item.ID, ZERODATE, &rest));
							if(p_uhtt_cli->GetSCardByNumber(item.Code, scp) && p_uhtt_cli->GetSCardRest(scp.Code, 0, uhtt_rest)) {
								if(rest != uhtt_rest) {
									scop_rec.Amount = R2(uhtt_rest - rest);
									THROW(SCObj.P_Tbl->PutOpRec(&scop_rec, &urn_list, 0));
									ok = 1;
								}
							}
							else
								logger.LogLastError();
						}
					}
					else if(param.Action == SCardChrgCrdParam::actionExtendTo) {
						if(param.Amount >= 0.0) {
							if(p_uhtt_cli) {
								int    uhtt_error = 0;
								if(p_uhtt_cli->GetSCardByNumber(item.Code, scp) && p_uhtt_cli->GetSCardRest(scp.Code, 0, rest)) {
									if(rest != param.Amount) {
										amount = R2(param.Amount - rest);
										if(amount > 0.0) {
											if(!p_uhtt_cli->DepositSCardAmount(scp.Code, amount))
												uhtt_error = 1;
										}
										else if(amount < 0.0) {
											if(fabs(amount) > (rest + scp.Overdraft) || !p_uhtt_cli->WithdrawSCardAmount(scp.Code, fabs(amount)))
												uhtt_error = 1;
										}
									}
								}
								else
									uhtt_error = 1;
								if(uhtt_error)
									logger.LogLastError();
								else if(amount != 0.0) {
									scop_rec.Amount = amount;
									THROW(SCObj.P_Tbl->PutOpRec(&scop_rec, &urn_list, 0));
									ok = 1;
								}
							}
							else {
								THROW(SCObj.P_Tbl->GetRest(item.ID, ZERODATE, &rest));
								if(rest != param.Amount) {
									scop_rec.Amount = R2(param.Amount - rest);
									THROW(SCObj.P_Tbl->PutOpRec(&scop_rec, &urn_list, 0));
									ok = 1;
								}
							}
						}
					}
					else {
						amount = R2(param.Amount);
						if(amount != 0.0) {
							if(p_uhtt_cli) {
								int    uhtt_error = 0;
								if(p_uhtt_cli->GetSCardByNumber(item.Code, scp)) {
									if(amount > 0.0) {
										if(!p_uhtt_cli->DepositSCardAmount(scp.Code, amount))
											uhtt_error = 1;
									}
									else if(amount < 0.0) {
										if(!p_uhtt_cli->GetSCardRest(scp.Code, 0, rest) || fabs(amount) > (rest + scp.Overdraft) || !p_uhtt_cli->WithdrawSCardAmount(scp.Code, fabs(amount)))
											uhtt_error = 1;
									}
								}
								else
									uhtt_error = 1;
								if(uhtt_error)
									logger.LogLastError();
								else {
									scop_rec.Amount = amount;
									THROW(SCObj.P_Tbl->PutOpRec(&scop_rec, &urn_list, 0));
									ok = 1;
								}
							}
							else {
								scop_rec.Amount = amount;
								THROW(SCObj.P_Tbl->PutOpRec(&scop_rec, &urn_list, 0));
								ok = 1;
							}
						}
					}
					PPWaitPercent(GetCounter());
				}
				THROW(tra.Commit());
			}
			SCObj.FinishSCardUpdNotifyList(urn_list);
			PPWait(0);
		}
	}
	CATCHZOKPPERR
	delete p_uhtt_cli;
	return ok;
}

int SLAPI PPViewSCard::ChangeDiscount()
{
	int    ok = -1;
	TDialog * dlg = 0;
	PPSCardSeries ser_rec;
	if(/*Filt.SeriesID*/SeriesList.GetSingle() && SearchObject(PPOBJ_SCARDSERIES, /*Filt.SeriesID*/SeriesList.GetSingle(), &ser_rec) > 0) {
		int    valid_data = 0;
		double pct = 0.0;
		SCardViewItem item;
		THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_CHGSCARDDISC))));
		dlg->setCtrlData(CTL_CHGSCARDDISC_VAL, &pct);
		for(valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			dlg->getCtrlData(CTL_CHGSCARDDISC_VAL, &pct);
			pct = R6(pct);
			if(pct <= 0.0 || pct > 50.0)
				PPError(PPERR_USERINPUT, 0);
			else
				valid_data = 1;
		}
		delete dlg;
		dlg = 0;
		if(valid_data) {
			PPIDArray id_list;
			THROW(SCObj.CheckRights(PPR_MOD));
			PPWait(1);
			for(InitIteration(); NextIteration(&item) > 0;) {
				id_list.add(item.ID);
			}
			id_list.sortAndUndup();
			if(id_list.getCount()) {
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < id_list.getCount(); i++) {
					const PPID sc_id = id_list.get(i);
					THROW(SCObj.P_Tbl->UpdateDiscount(sc_id, pct, 0));
					PPWaitPercent(i+1, id_list.getCount());
				}
				THROW(tra.Commit());
			}
			PPWait(0);
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
int SLAPI PPViewSCard::ReplaceCardInChecks(PPID destCardID)
{
	int    ok = -1;
	TDialog * dlg = 0;
	SCardTbl::Rec dest_card_rec, src_card_rec;
	MEMSZERO(src_card_rec);
	THROW(SCObj.CheckRights(SCRDRT_BINDING));
	if(SCObj.Search(destCardID, &dest_card_rec) > 0 && !SCObj.IsCreditSeries(dest_card_rec.SeriesID)) {
		CCheckCore & r_cc = *SCObj.P_CcTbl;
		THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_REPLCARD))));
		dlg->setCtrlData(CTL_REPLCARD_DESTCARD, dest_card_rec.Code);
		dlg->setCtrlData(CTL_REPLCARD_SRCCARD, src_card_rec.Code);
		dlg->disableCtrl(CTL_REPLCARD_DESTCARD, 1);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			//dlg->getCtrlData(CTL_REPLCARD_DESTCARD, dest_card_rec.Code);
			dlg->getCtrlData(CTL_REPLCARD_SRCCARD, src_card_rec.Code);
			if(*strip(src_card_rec.Code) && SCObj.SearchCode(0, src_card_rec.Code, &src_card_rec) > 0) {
				if(!r_cc.ReplaceSCard(dest_card_rec.ID, src_card_rec.ID, 1))
					PPError();
				else
					ok = valid_data = 1;
			}
			else {
				PPError(PPERR_SCARDNOTFOUND, src_card_rec.Code);
				dlg->selectCtrl(CTL_REPLCARD_SRCCARD);
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPViewSCard::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		SCardViewItem item;
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWait(1);
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			objid_ary.Add(PPOBJ_SCARD, item.ID);
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int SLAPI PPViewSCard::OnExecBrowser(PPViewBrowser * pBrw)
{
	pBrw->SetupToolbarCombo(PPOBJ_SCARDSERIES, /*Filt.SeriesID*/SeriesList.GetSingle(), 0, 0);
	return -1;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	PPViewSCard * p_view = (PPViewSCard *)extraPtr;
	return p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle) : -1;
}

int SLAPI PPViewSCard::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle)
{
	int    ok = -1;
	if(pData && pCellStyle && col >= 0) {
		const PPID sc_id = *(const PPID *)pData;
		SCardTbl::Rec sc_rec;
		if(col == 0) { // card number
			if(SCObj.Fetch(sc_id, &sc_rec) > 0) {
				if(sc_rec.Flags & SCRDF_CLOSED) {
					if(sc_rec.Flags & SCRDF_NEEDACTIVATION) {
						pCellStyle->Flags |= BrowserWindow::CellStyle::fCorner;
						pCellStyle->Color = GetColorRef(SClrOrange);
						ok = 1;
					}
					else {
						pCellStyle->Flags |= BrowserWindow::CellStyle::fCorner;
						pCellStyle->Color = GetColorRef(SClrRed);
						ok = 1;
					}
				}
			}
		}
		else if(col == 1) { // series
			if(SCObj.Fetch(sc_id, &sc_rec) > 0) {
				if(sc_rec.Flags & SCRDF_INHERITED) {
					pCellStyle->Flags |= BrowserWindow::CellStyle::fCorner;
					pCellStyle->Color = GetColorRef(SClrAqua);
					ok = 1;
				}
			}
		}
		else if(col == 3) { // expiry
			if(SCObj.Fetch(sc_id, &sc_rec) > 0) {
				if(sc_rec.Expiry && sc_rec.Expiry <= getcurdate_()) {
					pCellStyle->Flags |= BrowserWindow::CellStyle::fCorner;
					pCellStyle->Color = GetColorRef(SClrRed);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

void SLAPI PPViewSCard::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		if(Filt.Flags & SCardFilt::fShowOwnerAddrDetail && P_TmpTbl) {
			pBrw->InsColumn(-1, "@phone",     14, 0L, 0, 0); // @v9.6.1 #12-->#14
			pBrw->InsColumn(-1, "@address",   15, 0L, 0, 0); // @v9.6.1 #13-->#15
			pBrw->InsColumn(-1, "@postzip",   16, 0L, 0, 0); // @v9.6.1 #14-->#16
			pBrw->InsColumn(-1, "@region",    17, 0L, 0, 0); // @v9.6.1 #15-->#17
			pBrw->InsColumn(-1, "@city",      18, 0L, 0, 0); // @v9.6.1 #16-->#18
			pBrw->InsColumn(-1, "@street",    19, 0L, 0, 0); // @v9.6.1 #17-->#19
			pBrw->InsColumn(-1, "@house",     20, 0L, 0, 0); // @v9.6.1 #18-->#20
			pBrw->InsColumn(-1, "@apartment", 21, 0L, 0, 0); // @v9.6.1 #19-->#21
			pBrw->InsColumn(-1, "@addendum",  22, 0L, 0, 0); // @v9.6.1 #20-->#22
		}
		pBrw->SetCellStyleFunc(CellStyleFunc, this);
	}
}

DBQuery * SLAPI PPViewSCard::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_SCARD;
	SString sub_title;
	PPSCardSeries ser_rec;
	DBQuery * q = 0;
	DBQ  * dbq = 0;
	DBE  * dbe_dis = 0;
	DBE    dbe_psn;
	DBE    dbe_ser;
	DBE    dbe_autogoods;
	DBE    dbe_phone; // @v9.6.1
	DBE    dbe_memo;  // @v9.6.1
	SCardTbl * p_c = 0;
	TempSCardTbl * p_t = 0;
	TempOrderTbl * p_ot = 0;
	if(SearchObject(PPOBJ_SCARDSERIES, /*Filt.SeriesID*/SeriesList.GetSingle(), &ser_rec) > 0) {
		if(ser_rec.Flags & (SCRDSF_CREDIT|SCRDSF_BONUS))
			if(Filt.TrnovrPeriod.IsZero())
				brw_id = BROWSER_SCARDCRD;
			else
				brw_id = BROWSER_SCARDCRD_TRNOVR;
	}
	else
		MEMSZERO(ser_rec);
	if(Filt.PersonID)
		GetObjectName(PPOBJ_PERSON, Filt.PersonID, sub_title, 1);
	if(/*Filt.SeriesID*/SeriesList.GetSingle())
		sub_title.CatDivIfNotEmpty('-', 1).Cat(ser_rec.Name);
	if(P_TempOrd) {
		THROW(CheckTblPtr(p_ot = new TempOrderTbl(P_TempOrd->GetName())));
	}
	else
		THROW(CreateOrderTable(Filt.Order, &p_ot));
	if(P_TmpTbl) {
		THROW(CheckTblPtr(p_t = new TempSCardTbl(P_TmpTbl->GetName())));
		PPDbqFuncPool::InitObjNameFunc(dbe_psn, PPDbqFuncPool::IdObjNamePerson,   p_t->PersonID);
		PPDbqFuncPool::InitObjNameFunc(dbe_ser, PPDbqFuncPool::IdObjNameSCardSer, p_t->SeriesID);
		PPDbqFuncPool::InitObjNameFunc(dbe_autogoods, PPDbqFuncPool::IdObjNameGoods, p_t->AutoGoodsID);
		// @v9.6.1 {
		PPDbqFuncPool::InitFunc2Arg(dbe_phone, PPDbqFuncPool::IdSCardExtString, p_t->ID, dbconst((long)PPSCardPacket::extssPhone));
		PPDbqFuncPool::InitFunc2Arg(dbe_memo, PPDbqFuncPool::IdSCardExtString, p_t->ID, dbconst((long)PPSCardPacket::extssMemo));
		// } @v9.6.1
		dbe_dis = & (p_t->PDis / 100);
		q = & select(
			p_t->ID,        // #0
			p_t->Code,      // #1
			p_t->Expiry,    // #2
			*dbe_dis,       // #3
			p_t->MaxCredit, // #4
			p_t->Turnover,  // #5
			p_t->InTrnovr,  // #6
			dbe_psn,        // #7
			p_t->Rest,      // #8
			p_t->Dt,        // #9
			dbe_ser,        // #10
			dbe_autogoods,  // #11
			dbe_phone,      // #12 @v9.6.1
			dbe_memo,       // #13 @v9.6.1
			0L);
		if(Filt.Flags & SCardFilt::fShowOwnerAddrDetail) {
			/*
			q->addField(p_t->Phone);      // #14 @v9.6.1 #12-->#14
			q->addField(p_t->Address);    // #15 @v9.6.1 #13-->#15
			q->addField(p_t->ZIP);        // #16 @v9.6.1 #14-->#16
			q->addField(p_t->LocalArea);  // #17 @v9.6.1 #15-->#17
			q->addField(p_t->City);       // #18 @v9.6.1 #16-->#18
			q->addField(p_t->Street);     // #19 @v9.6.1 #17-->#19
			q->addField(p_t->House);      // #20 @v9.6.1 #18-->#20
			q->addField(p_t->Apart);      // #21 @v9.6.1 #19-->#21
			q->addField(p_t->AddrAddend); // #22 @v9.6.1 #20-->#22
			*/
			DBE    dbe_phone__; 
			DBE    dbe_address;
			DBE    dbe_zip;
			DBE    dbe_localarea;
			DBE    dbe_city;
			DBE    dbe_street;
			DBE    dbe_house;
			DBE    dbe_apart;
			DBE    dbe_addraddend;
			{
				PPDbqFuncPool::InitStrPoolRefFunc(dbe_phone__, p_t->PhoneP, &StrPool);
				PPDbqFuncPool::InitStrPoolRefFunc(dbe_address, p_t->AddressP, &StrPool);
				PPDbqFuncPool::InitStrPoolRefFunc(dbe_zip, p_t->ZipP, &StrPool);
				PPDbqFuncPool::InitStrPoolRefFunc(dbe_localarea, p_t->LocalAreaP, &StrPool);
				PPDbqFuncPool::InitStrPoolRefFunc(dbe_city, p_t->CityP, &StrPool);
				PPDbqFuncPool::InitStrPoolRefFunc(dbe_street, p_t->StreetP, &StrPool);
				PPDbqFuncPool::InitStrPoolRefFunc(dbe_house, p_t->HouseP, &StrPool);
				PPDbqFuncPool::InitStrPoolRefFunc(dbe_apart, p_t->ApartP, &StrPool);
				PPDbqFuncPool::InitStrPoolRefFunc(dbe_addraddend, p_t->AddrAddendP, &StrPool);
				q->addField(dbe_phone__);    //  #14 
				q->addField(dbe_address);    //  #15
				q->addField(dbe_zip);        //  #16 
				q->addField(dbe_localarea);  //  #17 
				q->addField(dbe_city);       //  #18 
				q->addField(dbe_street);     //  #19 
				q->addField(dbe_house);      //  #20 
				q->addField(dbe_apart);      //  #21 
				q->addField(dbe_addraddend); //  #22 
			}
		}
		if(p_ot)
			q->from(p_ot, p_t, 0L);
		else
			q->from(p_t, 0L);
		if(p_ot)
			dbq = & (*dbq && p_ot->ID == p_t->ID);
		q->where(*dbq);
		if(!p_ot) {
			if(/*Filt.SeriesID*/SeriesList.GetSingle())
				q->orderBy(p_t->SeriesID, p_t->Code, 0L);
			else if(!Filt.PersonID)
				q->orderBy(p_t->Code, p_t->SeriesID, 0L);
		}
		else
			q->orderBy(p_ot->Name, 0L);
	}
	else {
		THROW(CheckTblPtr(p_c = new SCardTbl));
		PPDbqFuncPool::InitObjNameFunc(dbe_psn, PPDbqFuncPool::IdObjNamePerson, p_c->PersonID);
		PPDbqFuncPool::InitObjNameFunc(dbe_ser, PPDbqFuncPool::IdObjNameSCardSer, p_c->SeriesID);
		PPDbqFuncPool::InitObjNameFunc(dbe_autogoods, PPDbqFuncPool::IdObjNameGoods, p_c->AutoGoodsID);
		// @v9.6.1 {
		PPDbqFuncPool::InitFunc2Arg(dbe_phone, PPDbqFuncPool::IdSCardExtString, p_c->ID, dbconst((long)PPSCardPacket::extssPhone));
		PPDbqFuncPool::InitFunc2Arg(dbe_memo, PPDbqFuncPool::IdSCardExtString, p_c->ID, dbconst((long)PPSCardPacket::extssMemo));
		// } @v9.6.1
		dbe_dis = & (p_c->PDis / 100);
		q = & select(
			p_c->ID,        // #0
			p_c->Code,      // #1
			p_c->Expiry,    // #2
			*dbe_dis,       // #3
			p_c->MaxCredit, // #4
			p_c->Turnover,  // #5
			p_c->InTrnovr,  // #6
			dbe_psn,        // #7
			p_c->Rest,      // #8
			p_c->Dt,        // #9
			dbe_ser,        // #10
			dbe_autogoods,  // #11
			dbe_phone,      // #12 @v9.6.1
			dbe_memo,       // #13 @v9.6.1
			0L);
		if(p_ot)
			q->from(p_ot, p_c, 0L);
		else
			q->from(p_c, 0L);

		dbq = ppcheckfiltid(dbq, p_c->SeriesID, /*Filt.SeriesID*/SeriesList.GetSingle());
		if(Filt.Flags & SCardFilt::fWoOwner)
			dbq = & (*dbq && p_c->PersonID == 0L);
		else
			dbq = ppcheckfiltid(dbq, p_c->PersonID, Filt.PersonID);
		dbq = ppcheckfiltid(dbq, p_c->LocID, Filt.LocID); // @v9.4.5
		dbq = ppcheckflag(dbq, p_c->Flags, SCRDF_CLOSED, Filt.Ft_Closed);
		dbq = & (*dbq && daterange(p_c->Dt, &Filt.IssuePeriod));
		dbq = & (*dbq && daterange(p_c->Expiry, &Filt.ExpiryPeriod));
		dbq = & (*dbq && realrange(p_c->PDis, Filt.PDisR.low, Filt.PDisR.upp));
		dbq = & (*dbq && realrange(p_c->Turnover, Filt.TurnoverR.low, Filt.TurnoverR.upp));
		if(p_ot)
			dbq = & (*dbq && p_ot->ID == p_c->ID);
		q->where(*dbq);
		if(!p_ot) {
			if(/*Filt.SeriesID*/SeriesList.GetSingle())
				q->orderBy(p_c->SeriesID, p_c->Code, 0L);
			else if(!Filt.PersonID && !Filt.LocID)
				q->orderBy(p_c->Code, p_c->SeriesID, 0L);
		}
		else
			q->orderBy(p_ot->Name, 0L);
	}
	delete dbe_dis;
	THROW(CheckQueryPtr(q));
	ASSIGN_PTR(pBrwId, brw_id);
	ASSIGN_PTR(pSubTitle, sub_title);
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete p_c;
			delete p_ot;
			delete p_t;
		}
	ENDCATCH
	return q;
}

int SLAPI EditSCardFlags(long * pSetFlags, long * pResetFlags)
{
	class SCardFlagsDialog : public TDialog {
	public:
		SCardFlagsDialog() : TDialog(DLG_SCARDFL)
		{
			setDTS(0, 0);
		}
		int    setDTS(long setFlags, long resetFlags)
		{
			AddClusterAssoc(CTL_SCARDFL_SET, 0, SCRDF_INHERITED);
			AddClusterAssoc(CTL_SCARDFL_SET, 1, SCRDF_CLOSED);
			AddClusterAssoc(CTL_SCARDFL_SET, 2, SCRDF_CLOSEDSRV);
			AddClusterAssoc(CTL_SCARDFL_SET, 3, SCRDF_NOGIFT);
			SetClusterData(CTL_SCARDFL_SET, setFlags);
			AddClusterAssoc(CTL_SCARDFL_RESET, 0, SCRDF_INHERITED);
			AddClusterAssoc(CTL_SCARDFL_RESET, 1, SCRDF_CLOSED);
			AddClusterAssoc(CTL_SCARDFL_RESET, 2, SCRDF_CLOSEDSRV);
			AddClusterAssoc(CTL_SCARDFL_RESET, 3, SCRDF_NOGIFT);
			SetClusterData(CTL_SCARDFL_RESET, resetFlags);
			return 1;
		}
		int    getDTS(long * pSetFlags, long * pResetFlags)
		{
			int    ok = 1;
			long   v1 = 0, v2 = 0;
			GetClusterData(CTL_SCARDFL_SET,   &v1);
			GetClusterData(CTL_SCARDFL_RESET, &v2);
			for(uint i = 0; i < 32; i++)
				THROW_PP(!((v1 & v2) & (1UL << i)), PPERR_SCARDFCONFLICT);
			GetClusterData(CTL_SCARDFL_SET,   &v1);
			GetClusterData(CTL_SCARDFL_RESET, &v2);
			ASSIGN_PTR(pSetFlags, v1);
			ASSIGN_PTR(pResetFlags, v2);
			CATCHZOK
			return ok;
		}
	};
	int    ok = -1, valid_data = 0;
	SCardFlagsDialog * dlg = new SCardFlagsDialog();
	THROW_MEM(dlg);
	THROW(CheckDialogPtr(&dlg));
	while(!valid_data && ExecView(dlg) == cmOK)
		if(dlg->getDTS(pSetFlags, pResetFlags))
			ok = valid_data = 1;
		else
			PPError();
	CATCHZOK
	delete dlg;
	return ok;
}

int SLAPI PPViewSCard::ChangeFlags()
{
	int    ok = -1;
	long   set = 0, reset = 0;
	THROW(ok = EditSCardFlags(&set, &reset));
	if(ok > 0 && (set || reset)) {
		SCardViewItem item;
		PPWait(1);
		{
			PPTransaction tra(1);
			THROW(tra);
			for(InitIteration(); NextIteration(&item) > 0;) {
				long sav = item.Flags;
				for(uint p = 0; p < 32; p++) {
					ulong t = (1UL << p);
					if(set & t)
						item.Flags |= t;
					if(reset & t)
						item.Flags &= ~t;
				}
				if(sav != item.Flags)
					THROW(SCObj.SetFlags(item.ID, item.Flags, 0));
				PPWaitPercent(Counter);
			}
			THROW(tra.Commit());
		}
		ok = 1;
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

SCardSelPrcssrDialog::SCardSelPrcssrDialog(PPViewSCard * pView, int editSCardFilt) : TDialog(DLG_FLTSCARDCHNG)
{
	P_View = pView;
	EditSCardFilt = editSCardFilt;
	enableCommand(cmSCardFilt, EditSCardFilt);
	SetupCalCtrl(CTLCAL_FLTSCARDCHNG_DTEND, this, CTL_FLTSCARDCHNG_DTEND, 0);
}

int SCardSelPrcssrDialog::setDTS(const SCardSelPrcssrParam * pData)
{
	if(!RVALUEPTR(Data, pData))
		Data.Init();
	AddClusterAssoc(CTL_FLTSCARDCHNG_FS, 0, SCRDF_INHERITED);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FS, 1, SCRDF_CLOSED);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FS, 2, SCRDF_CLOSEDSRV);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FS, 3, SCRDF_NOGIFT);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FS, 4, SCRDF_NEEDACTIVATION);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FS, 5, SCRDF_AUTOACTIVATION);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FS, 6, SCRDF_NOTIFYDISCOUNT);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FS, 7, SCRDF_NOTIFYWITHDRAW);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FS, 8, SCRDF_NOTIFYDRAW);
	SetClusterData(CTL_FLTSCARDCHNG_FS, Data.FlagsSet);

	AddClusterAssoc(CTL_FLTSCARDCHNG_FR, 0, SCRDF_INHERITED);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FR, 1, SCRDF_CLOSED);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FR, 2, SCRDF_CLOSEDSRV);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FR, 3, SCRDF_NOGIFT);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FR, 4, SCRDF_NEEDACTIVATION);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FR, 5, SCRDF_AUTOACTIVATION);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FR, 6, SCRDF_NOTIFYDISCOUNT);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FR, 7, SCRDF_NOTIFYWITHDRAW);
	AddClusterAssoc(CTL_FLTSCARDCHNG_FR, 8, SCRDF_NOTIFYDRAW);
	SetClusterData(CTL_FLTSCARDCHNG_FR, Data.FlagsReset);

	setCtrlData(CTL_FLTSCARDCHNG_DTEND, &Data.DtEnd);
	AddClusterAssoc(CTL_FLTSCARDCHNG_ZEXPIRY, 0, SCardSelPrcssrParam::fZeroExpiry); // @v9.6.2
	SetClusterData(CTL_FLTSCARDCHNG_ZEXPIRY, Data.Flags); // @v9.6.2
	SetupPPObjCombo(this, CTLSEL_FLTSCARDCHNG_SER, PPOBJ_SCARDSERIES, Data.NewSerID, 0, 0);
	SetupPPObjCombo(this, CTLSEL_FLTSCARDCHNG_AG,  PPOBJ_GOODS, Data.AutoGoodsID, OLW_CANINSERT|OLW_LOADDEFONOPEN, 0);

	SetupStringCombo(this, CTLSEL_FLTSCARDCHNG_PRD, PPTXT_CYCLELIST, Data.PeriodTerm);
	setCtrlUInt16(CTL_FLTSCARDCHNG_PRDC, Data.PeriodCount);

	setCtrlReal(CTL_FLTSCARDCHNG_DSCNT, Data.Discount);
	AddClusterAssoc(CTL_FLTSCARDCHNG_ZDSCNT, 0, SCardSelPrcssrParam::fZeroDiscount);
	SetClusterData(CTL_FLTSCARDCHNG_ZDSCNT, Data.Flags);

	return 1;
}

int SCardSelPrcssrDialog::getDTS(SCardSelPrcssrParam * pData)
{
 	ushort sel = 0;
 	int    ok = -1;

	GetClusterData(CTL_FLTSCARDCHNG_FS, &Data.FlagsSet);
	GetClusterData(CTL_FLTSCARDCHNG_FR, &Data.FlagsReset);

 	getCtrlData(sel = CTL_FLTSCARDCHNG_DTEND, &Data.DtEnd);
 	THROW_SL(checkdate(Data.DtEnd, 1));
	GetClusterData(CTL_FLTSCARDCHNG_ZEXPIRY, &Data.Flags); // @v9.6.2

 	sel = CTL_FLTSCARDCHNG_FLAGS;
 	getCtrlData(CTLSEL_FLTSCARDCHNG_SER, &Data.NewSerID);
	getCtrlData(CTLSEL_FLTSCARDCHNG_AG, &Data.AutoGoodsID);

	Data.PeriodTerm = (int16)getCtrlLong(CTLSEL_FLTSCARDCHNG_PRD);
	Data.PeriodCount = (int16)getCtrlUInt16(CTL_FLTSCARDCHNG_PRDC);

	Data.Discount = getCtrlReal(CTL_FLTSCARDCHNG_DSCNT);
	GetClusterData(CTL_FLTSCARDCHNG_ZDSCNT, &Data.Flags);
	if(Data.Flags & SCardSelPrcssrParam::fZeroDiscount)
		Data.Discount = 0.0;

	THROW(Data.Validate(0));
 	THROW_PP(!Data.IsEmpty(), PPERR_SCARDPRCSSRPARAM);
 	THROW_PP(!EditSCardFilt || Data.SelFilt.IsEmpty() == 0, PPERR_SCARDFILTEMPTY);
 	ASSIGN_PTR(pData, Data);
 	ok = 1;
 	CATCH
 		ok = (selectCtrl(sel), 0);
 	ENDCATCH
 	return ok;
 }

IMPL_HANDLE_EVENT(SCardSelPrcssrDialog)
{
 	TDialog::handleEvent(event);
 	if(event.isCmd(cmSCardFilt) && P_View)
 		P_View->EditBaseFilt(&Data.SelFilt);
	else if(event.isClusterClk(CTL_FLTSCARDCHNG_ZDSCNT)) {
		GetClusterData(CTL_FLTSCARDCHNG_ZDSCNT, &Data.Flags);
		disableCtrl(CTL_FLTSCARDCHNG_DSCNT, BIN(Data.Flags & SCardSelPrcssrParam::fZeroDiscount));
	}
	else if(event.isClusterClk(CTL_FLTSCARDCHNG_ZEXPIRY)) {
		GetClusterData(CTL_FLTSCARDCHNG_ZEXPIRY, &Data.Flags);
		disableCtrl(CTL_FLTSCARDCHNG_DTEND, BIN(Data.Flags & SCardSelPrcssrParam::fZeroExpiry));
	}
	else
		return;
	clearEvent(event);
}

int SLAPI PPViewSCard::ProcessSelection(SCardSelPrcssrParam * pParam, PPLogger * pLog)
{
	int    ok = -1, valid_data = 0;
	SCardSelPrcssrDialog * dlg = 0;
	SCardSelPrcssrParam param;
	PPObjSCardSeries scs_obj;
	PPObjGoods goods_obj;
	SString scard_name, msg_buf, fmt_buf, new_scard_name, temp_buf, temp_buf2;
	THROW(SCObj.CheckRights(SCRDRT_MASSCHANGE));
	if(pParam && pParam->IsEmpty() == 0) {
		param = *pParam;
		valid_data = 1;
		if(!param.SelFilt.IsEmpty())
			THROW(Init_(&param.SelFilt));
	}
	else {
		THROW(CheckDialogPtr(&(dlg = new SCardSelPrcssrDialog(this, 0))));
		dlg->setDTS(&param);
		while(!valid_data && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&param) > 0)
				valid_data = 1;
			else
				PPError();
	}
	THROW(param.Validate(0));
	if(valid_data) {
		PPIDArray sc_id_list;
		// @v9.6.3 param.DtEnd = param.DtEnd.getactual(ZERODATE);
		PPWait(1);
		{
			SCardViewItem item;
			for(InitIteration(); NextIteration(&item) > 0;) {
				if(param.NewSerID) {
					//
					// Проверка допустимости переноса между сериями //
					//
					THROW(param.Validate(item.SeriesID));
				}
				sc_id_list.addUnique(item.ID);
			}
		}
		SCardTbl::Rec rec;
		const long fset = param.FlagsSet;
		const long freset = param.FlagsReset;
		const long valid_flags = PPObjSCard::GetValidFlags();
		PPIDArray dirty_list;
		const int expiry_date_cls = param.DtEnd.getclass();
		{
			PPTransaction tra(1);
			THROW(tra);
			for(uint i = 0; i < sc_id_list.getCount(); i++) {
				const  PPID sc_id = sc_id_list.get(i);
				//PPSCardSeries org_scs_rec;
				PPSCardSerPacket org_scs_pack;
				if(SCObj.Search(sc_id, &rec) > 0) {
					int    upd = 0;
					int    upd_discount = 0;
					const  long preserve_pdis = rec.PDis;
					const  long preserve_flags = rec.Flags;
					THROW(scs_obj.GetPacket(rec.SeriesID, &org_scs_pack) > 0);
					//
					// Прежде всего нормализуем наследуемую запись
					//
					THROW(SCObj.SetInheritance(&org_scs_pack, &rec));
					if(param.AutoGoodsID && rec.AutoGoodsID != param.AutoGoodsID) {
						rec.AutoGoodsID = param.AutoGoodsID;
						upd = 1;
					}
					if(fset || freset) {
						for(uint p = 0; p < 32; p++) {
							const long t = (1L << p);
							if(t & valid_flags) {
								if(fset & t)
									rec.Flags |= t;
								else if(freset & t)
									rec.Flags &= ~t;
							}
						}
						if(preserve_flags != rec.Flags)
							upd = 1;
					}
					if((param.Discount > 0.0 && param.Discount <= 100.0) || param.Flags & param.fZeroDiscount) {
						if(param.Flags & param.fZeroDiscount)
							rec.PDis = 0;
						else if(param.Discount > 0.0 && param.Discount <= 100.0)
							rec.PDis = (long)(R6(param.Discount) * 100.0);
						if(rec.PDis != preserve_pdis) {
							rec.Flags &= ~SCRDF_INHERITED; // Форсированно снимаем признак наследования //
							upd = 1;
							upd_discount = 1;
						}
					}
					if(param.Flags & param.fZeroExpiry && rec.Expiry) {
						rec.Expiry = ZERODATE;
						rec.Flags &= ~SCRDF_INHERITED; // Форсированно снимаем признак наследования //
						upd = 1;
					}
					else if(param.DtEnd) {
						LDATE  new_expiry_date = rec.Expiry;
						if(expiry_date_cls == LDATE::cNormal) {
							new_expiry_date = param.DtEnd;
						}
						else if(expiry_date_cls == LDATE::cSpecial) {
							if(rec.Expiry) {
								new_expiry_date = param.DtEnd.getactual(rec.Expiry);
							}
						}
						if(checkdate(new_expiry_date, 0) && new_expiry_date != rec.Expiry) {
							rec.Expiry = new_expiry_date;
							rec.Flags &= ~SCRDF_INHERITED; // Форсированно снимаем признак наследования //
							upd = 1;
						}
					}
					if(param.PeriodTerm && param.PeriodCount) {
						if(rec.PeriodTerm != param.PeriodTerm) {
							rec.PeriodTerm = param.PeriodTerm;
							upd = 1;
						}
						if(rec.PeriodCount != param.PeriodCount) {
							rec.PeriodCount = param.PeriodCount;
							upd = 1;
						}
					}
					if(param.NewSerID && param.NewSerID != rec.SeriesID) {
						//PPSCardSeries new_ser_rec;
						PPSCardSerPacket new_ser_pack;
						(scard_name = org_scs_pack.Rec.Name).CatChar('-').Cat(rec.Code);
						if(scs_obj.GetPacket(param.NewSerID, &new_ser_pack) > 0) {
							if(new_ser_pack.Rec.GetType() == org_scs_pack.Rec.GetType()) {
								SCardTbl::Key2 k2;
								MEMSZERO(k2);
								k2.SeriesID = param.NewSerID;
								STRNSCPY(k2.Code, rec.Code);
								if(SCObj.P_Tbl->search(2, &k2, spEq)) {
									(new_scard_name = new_ser_pack.Rec.Name).CatChar('-').Cat(rec.Code);
									PPLoadText(PPTXT_LOG_SCARDEXISTS, fmt_buf);
									msg_buf.Printf(fmt_buf, new_scard_name.cptr());
									upd = 0;
								}
								else {
									THROW_DB(BTROKORNFOUND);
									rec.SeriesID = param.NewSerID;
									SCObj.SetInheritance(&new_ser_pack, &rec); // Нормализация записи наследуемой карты после установки серии
									upd = 1;
								}
							}
							else {
								PPLoadText(PPTXT_LOG_UNCOMPSCARDSER, fmt_buf);
								msg_buf.Printf(fmt_buf, scard_name.cptr(), (const char *)new_ser_pack.Rec.Name);
								upd = 0;
							}
							if(!upd)
								if(pLog)
									pLog->Log(msg_buf);
								else
									PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
						}
						else {
							PPLoadText(PPTXT_LOG_INVSCARDSER, fmt_buf);
							ideqvalstr(param.NewSerID, temp_buf.Z());
							PPGetLastErrorMessage(1, temp_buf2);
							msg_buf.Printf(fmt_buf, temp_buf.cptr(), scard_name.cptr(), temp_buf2.cptr());
							if(pLog)
								pLog->Log(msg_buf);
							else
								PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
							upd = 0;
						}
					}
					if(upd) {
						THROW(SCObj.P_Tbl->Update(rec.ID, &rec, 0));
						if(upd_discount)
							DS.LogAction(PPACN_SCARDDISUPD, PPOBJ_SCARD, rec.ID, preserve_pdis, 0);
						dirty_list.add(rec.ID);
						ok = 1;
					}
				}
				PPWaitPercent(i+1, sc_id_list.getCount());
			}
			THROW(tra.Commit());
		}
		{
			for(uint i = 0; i < dirty_list.getCount(); i++) {
				SCObj.Dirty(dirty_list.get(i));
			}
		}
	}
	CATCHZOKPPERR
	CALLPTRMEMB(pLog, Save(PPFILNAM_INFO_LOG, 0));
	delete dlg;
	PPWait(0);
	return ok;
}

int SLAPI PPViewSCard::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	BrwHdr hdr;
	PPIDArray id_list;
	if(pHdr)
		hdr = *(BrwHdr*)pHdr;
	else
		MEMSZERO(hdr);
	if(ppvCmd == PPVCMD_ADDITEM) {
		//
		// Перехватываем обработку команды до PPView::ProcessCommand
		//
		PPID   id = 0;
		PPObjSCard::AddParam param(/*Filt.SeriesID*/SeriesList.GetSingle(), Filt.PersonID);
		// @v9.4.5 {
		if(!Filt.PersonID && Filt.LocID)
			param.LocID = Filt.LocID;
		// } @v9.4.5
		ok = (SCObj.Edit(&id, param) == cmOK) ? 1 : -1;
		if(ok > 0 && ImplementFlags & implOnAddSetupPos && pBrw) {
			pBrw->Update();
			pBrw->search2(&id, CMPF_LONG, srchFirst, 0);
			ok = -1; // pBrw не должен теперь обновлять содержимое таблицы
		}
	}
	else {
		ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
		if(ok == -2) {
			switch(ppvCmd) {
				case PPVCMD_FINDANDEDIT:
					ok = -1;
					{
						PPID   id = 0;
						PPObjSCard::AddParam param(/*Filt.SeriesID*/SeriesList.GetSingle(), Filt.PersonID);
						if(SCObj.FindAndEdit(&id, &param) > 0)
							ok = 1;
					}
					break;
				case PPVCMD_CCHECKS:
					ok = -1;
					if(hdr.ID) {
						CCheckFilt flt;
						flt.SCardID = hdr.ID;
						flt.Period = Filt.TrnovrPeriod;
						ok = ViewCCheck(&flt, 0);
					}
					break;
				case PPVCMD_OPERATIONS:
					ok = ViewOps(hdr.ID);
					break;
				case PPVCMD_CHARGE:
					ok = ChargeCredit();
					break;
				case PPVCMD_TRANSMIT:
					Transmit(hdr.ID);
					break;
				case PPVCMD_CHNGDSCNT:
					ok = ChangeDiscount();
					break;
				case PPVCMD_CALCTRNOVR:
					ok = RecalcTurnover();
					break;
				case PPVCMD_AUTOFILL:
					//ok = Filt.SeriesID ? SCObj.AutoFill(Filt.SeriesID, 1) : -1;
					ok = SeriesList.GetSingle() ? SCObj.AutoFill(SeriesList.GetSingle(), 1) : -1;
					break;
				case PPVCMD_DELETEALL:
					ok = DeleteItem(0);
					break;
				case PPVCMD_REPLACECARD:
					if(hdr.ID)
						ok = ReplaceCardInChecks(hdr.ID);
					break;
				case PPVCMD_AFILLDEFPSN: // @unused
					ok = -1;
					break;
				/* @v7.7.3
				case PPVCMD_CHNGFLAGS:
					ok = ChangeFlags();
					break;
				*/
				case PPVCMD_RENAMEDUP:
					ok = RenameDup(&id_list);
					break;
				case PPVCMD_PROCESSSELECTION:
					{
						PPLogger logger;
						if(ProcessSelection(0, &logger) > 0)
							ChangeFilt(1, pBrw);
					}
					break;
				case PPVCMD_TB_CBX_SELECTED:
					ok = -1;
					{
						PPID   ser_id = 0;
						if(pBrw && pBrw->GetToolbarComboData(&ser_id) && /*Filt.SeriesID*/SeriesList.GetSingle() != ser_id) {
							Filt.ScsList.Set(0); // @v9.8.9
							Filt.SeriesID = ser_id;
							ok = ChangeFilt(1, pBrw);
						}
					}
					break;
			}
		}
	}
	if(ok > 0 && (oneof7(ppvCmd, PPVCMD_CHARGE, PPVCMD_CHNGDSCNT, PPVCMD_AUTOFILL,
		PPVCMD_DELETEALL, PPVCMD_AFILLDEFPSN, PPVCMD_CHNGFLAGS, PPVCMD_RENAMEDUP))) {
		UpdateTempTable(id_list.getCount() ? &id_list : 0);
	}
	else if(ok > 0 && oneof6(ppvCmd, PPVCMD_EDITITEM, PPVCMD_DELETEITEM, PPVCMD_ADDITEM,
		PPVCMD_CCHECKS, PPVCMD_OPERATIONS, PPVCMD_REPLACECARD)) {
		id_list.add(hdr.ID);
		ok = UpdateTempTable(&id_list);
	}
	return ok;
}

int SLAPI PPViewSCard::ViewTotal()
{
	int    ok = 1;
	SCardTotal total;
	MEMSZERO(total);
	SCardViewItem item;
	PPWait(1);
	for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
		total.Count++;
		total.Turnover += (item.Turnover - item.InTrnovr);
	}
	PPWait(0);
	TDialog * dlg = new TDialog(DLG_SCARDTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlLong(CTL_SCARDTOTAL_COUNT, total.Count);
		dlg->setCtrlReal(CTL_SCARDTOTAL_TRNOVR, total.Turnover);
		ok = ExecViewAndDestroy(dlg);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPViewSCard::ViewOps(PPID cardID)
{
	if(cardID) {
		if(SCObj.CheckRights(SCRDRT_VIEWOPS)) {
			SCardOpFilt filt;
			filt.SCardID = cardID;
			filt.Period = Filt.TrnovrPeriod;
			return PPView::Execute(PPVIEW_SCARDOP, &filt, 0, 0);
		}
		else
			return PPErrorZ();
	}
	else
		return -1;
}

int SLAPI ViewSCard(const SCardFilt * pFilt, int asModeless) { return PPView::Execute(PPVIEW_SCARD, pFilt, (asModeless ? PPView::exefModeless : 0), 0); }
//
// @ModuleDef(PPViewSCardOp)
//
IMPLEMENT_PPFILT_FACTORY(SCardOp); SLAPI SCardOpFilt::SCardOpFilt() : PPBaseFilt(PPFILT_SCARDOP, 0, 1)
{
	SetFlatChunk(offsetof(SCardOpFilt, ReserveStart),
		offsetof(SCardOpFilt, Reserve) - offsetof(SCardOpFilt, ReserveStart) + sizeof(Reserve));
	Init(1, 0);
}

SLAPI PPViewSCardOp::PPViewSCardOp() : PPView(0, &Filt, PPVIEW_SCARDOP)
{
	DefReportId = REPORT_SCARDOPLIST;
}

int SLAPI PPViewSCardOp::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	Counter.Init(0UL);
	if(Helper_InitBaseFilt(pFilt)) {
        Filt.Period.Actualize(ZERODATE); // @v9.5.11
	}
	else
		ok = 0;
	return ok;
}

PPBaseFilt * SLAPI PPViewSCardOp::CreateFilt(void * extraPtr) const
{
	SCardOpFilt * p_filt = new SCardOpFilt;
	p_filt->SCardID = (long)extraPtr;
	return p_filt;
}

int SLAPI PPViewSCardOp::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class SCardOpFiltDialog : public TDialog {
	public:
		SCardOpFiltDialog() : TDialog(DLG_SCOPFLT)
		{
			SetupCalPeriod(CTLCAL_SCOPFLT_PERIOD, CTL_SCOPFLT_PERIOD);
		}
		int    setDTS(const SCardOpFilt * pData)
		{
			int    ok = 1;
			Data = *pData;
			PPObjSCard::Filt sc_filt;
			sc_filt.SeriesID = Data.SCardSerID;
			SetPeriodInput(this, CTL_SCOPFLT_PERIOD, &Data.Period);
			SetupPPObjCombo(this, CTLSEL_SCOPFLT_SCSER, PPOBJ_SCARDSERIES, Data.SCardSerID, OLW_LOADDEFONOPEN);
			SetupPPObjCombo(this, CTLSEL_SCOPFLT_SC,    PPOBJ_SCARD,       Data.SCardID, OLW_LOADDEFONOPEN, &sc_filt);
			SetRealRangeInput(this, CTL_SCOPFLT_AMTR, &Data.AmtR);
			AddClusterAssocDef(CTL_SCOPFLT_ORD, 0, PPViewSCardOp::ordByDate);
			AddClusterAssoc(CTL_SCOPFLT_ORD, 1, PPViewSCardOp::ordByCard);
			AddClusterAssoc(CTL_SCOPFLT_ORD, 2, PPViewSCardOp::ordByOwner);
			AddClusterAssoc(CTL_SCOPFLT_ORD, 3, PPViewSCardOp::ordByAmount);
			SetClusterData(CTL_SCOPFLT_ORD, Data.Order);
			return ok;
		}
		int    getDTS(SCardOpFilt * pData)
		{
			int    ok = 1;
			THROW(GetPeriodInput(this, CTL_SCOPFLT_PERIOD, &Data.Period));
			Data.SCardSerID = getCtrlLong(CTLSEL_SCOPFLT_SCSER);
			Data.SCardID    = getCtrlLong(CTLSEL_SCOPFLT_SC);
			GetRealRangeInput(this, CTL_SCOPFLT_AMTR, &Data.AmtR);
			GetClusterData(CTL_SCOPFLT_ORD, &Data.Order);
			ASSIGN_PTR(pData, Data);
			CATCHZOK
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_SCOPFLT_SCSER)) {
				PPID   ser_id  = getCtrlLong(CTLSEL_SCOPFLT_SCSER);
				PPID   card_id = getCtrlLong(CTLSEL_SCOPFLT_SC);
				PPObjSCard::Filt sc_filt;
				sc_filt.SeriesID = ser_id;
				SetupPPObjCombo(this, CTLSEL_SCOPFLT_SC, PPOBJ_SCARD, card_id, 0, &sc_filt);
				clearEvent(event);
			}
		}
		SCardOpFilt Data;
	};
	if(!Filt.IsA(pBaseFilt))
		return 0;
	SCardOpFilt * p_filt = (SCardOpFilt *)pBaseFilt;
	DIALOG_PROC_BODY(SCardOpFiltDialog, p_filt);
}

int SLAPI PPViewSCardOp::InitIteration()
{
	IterPos.Set(Filt.Period.low, ZEROTIME);
	return 1;
}

int FASTCALL PPViewSCardOp::NextIteration(SCardOpViewItem * pItem)
{
	return (SCObj.P_Tbl->EnumOpByCard(Filt.SCardID, &IterPos, pItem) > 0 &&
		(!Filt.Period.upp || IterPos.d <= Filt.Period.upp)) ? 1 : -1;
}

int SLAPI PPViewSCardOp::CalcTotal(SCardOpTotal * pTotal)
{
	int    ok = 1;
	if(pTotal) {
		memzero(pTotal, sizeof(*pTotal));
		SCardOpViewItem item;
		// @v8.6.11 SCardOpTbl::Rec scop_rec;
		if(Filt.Period.low) {
			/* @v8.6.11
			if(SCObj.P_Tbl->GetLastOpByCard(Filt.SCardID, Filt.Period.low, &scop_rec) > 0)
				pTotal->InRest = scop_rec.Rest;
			*/
			// @v8.6.11 {
			LDATE lo_date = plusdate(Filt.Period.low, -1);
			SCObj.P_Tbl->GetRest(Filt.SCardID, lo_date, &pTotal->InRest);
			// } @v8.6.11
		}
		for(InitIteration(); NextIteration(&item) > 0;) {
			pTotal->Count++;
			if(item.Amount > 0)
				pTotal->Debit += item.Amount;
			else
				pTotal->Credit += -item.Amount;
		}
		pTotal->OutRest = pTotal->InRest + pTotal->Debit - pTotal->Credit;
	}
	else
		ok = -1;
	return ok;
}

static int SLAPI EditSCardOp(SCardCore::OpBlock & rBlk)
{
	class SCardOpDialog : public TDialog {
	public:
		SCardOpDialog(int freezing) : TDialog(freezing ? DLG_SCARDOPFRZ : DLG_SCARDOP)
		{
			Freezing = freezing;
			OrgExpiry = ZERODATE;
			OrgFreezingPeriod.SetZero();
			SetupCalDate(CTLCAL_SCARDOP_DT, CTL_SCARDOP_DT);
			SetupCalPeriod(CTLCAL_SCARDOP_FRZPERIOD, CTL_SCARDOP_FRZPERIOD);
			SetupTimePicker(this, CTL_SCARDOP_TM, CTLTM_SCARDOP_TM);
			SrcRest = 0.0;
			DestRest = 0.0;
		}
		int    setDTS(const SCardCore::OpBlock * pData)
		{
			SrcRest = 0.0;
			DestRest = 0.0;

			double rest = 0.0;
			SString temp_buf;
			SCardTbl::Rec sc_rec;
			disableCtrls(BIN((pData->LinkOi.Obj == PPOBJ_CCHECK && pData->LinkOi.Id) || pData->Flags & D.fEdit),
				CTL_SCARDOP_DT, CTL_SCARDOP_TM, CTL_SCARDOP_DESTCARDNO, 0);
			disableCtrl(CTL_SCARDOP_AMOUNT, (pData->LinkOi.Obj == PPOBJ_CCHECK && pData->LinkOi.Id));
			D = *pData;
			if(D.Flags & D.fEdit) {
				SrcRest = D.PrevRest;
				DestRest = D.PrevRest;
				if(Freezing)
					OrgFreezingPeriod = D.FreezingPeriod;
			}
			if(ScObj.Search(D.SCardID, &sc_rec) > 0) {
				OrgExpiry = sc_rec.Expiry;
				setCtrlData(CTL_SCARDOP_CARDNO, sc_rec.Code);
				if(sc_rec.PersonID) {
					GetPersonName(sc_rec.PersonID, temp_buf);
					setCtrlString(CTL_SCARDOP_OWNER, temp_buf);
				}
				if(!(D.Flags & D.fEdit))
					ScObj.P_Tbl->GetRest(D.SCardID, MAXDATE, &SrcRest);
				temp_buf.Z().Cat(SrcRest - D.Amount, MKSFMTD(0, 2, NMBF_NOZERO));
				setStaticText(CTL_SCARDOP_ST_REST, temp_buf);
			}
			if(Freezing) {
				SetPeriodInput(this, CTL_SCARDOP_FRZPERIOD, &D.FreezingPeriod);
				setCtrlDate(CTL_SCARDOP_EXPIRY, OrgExpiry);
			}
			else if(D.DestSCardID && ScObj.Search(D.DestSCardID, &sc_rec) > 0) {
				setCtrlData(CTL_SCARDOP_DESTCARDNO, sc_rec.Code);
				if(sc_rec.PersonID) {
					GetPersonName(sc_rec.PersonID, temp_buf);
					setCtrlString(CTL_SCARDOP_DESTOWNER, temp_buf);
				}
				if(!(D.Flags & D.fEdit))
					ScObj.P_Tbl->GetRest(D.DestSCardID, MAXDATE, &DestRest);
				temp_buf.Z().Cat(DestRest + D.Amount, MKSFMTD(0, 2, NMBF_NOZERO));
				setStaticText(CTL_SCARDOP_ST_DESTREST, temp_buf);
			}
			enableCommand(cmSCardOpCheck, BIN(D.LinkOi.Obj == PPOBJ_CCHECK && D.LinkOi.Id));
			setCtrlDatetime(CTL_SCARDOP_DT, CTL_SCARDOP_TM, D.Dtm);
			setCtrlReal(CTL_SCARDOP_AMOUNT, D.Amount);
			selectCtrl(Freezing ? CTL_SCARDOP_FRZPERIOD : CTL_SCARDOP_AMOUNT);
			return 1;
		}
		int    getDTS(SCardCore::OpBlock * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			SString temp_buf;
			getCtrlDatetime(sel = CTL_SCARDOP_DT, CTL_SCARDOP_TM, D.Dtm);
			THROW_SL(checkdate(D.Dtm.d, 1));
			if(Freezing) {
				THROW(GetPeriodInput(this, sel = CTL_SCARDOP_FRZPERIOD, &D.FreezingPeriod));
				THROW(D.CheckFreezingPeriod(OrgExpiry));
			}
			else {
				getCtrlData(sel = CTL_SCARDOP_AMOUNT, &D.Amount);
				THROW_PP(D.Amount != 0.0, PPERR_INVAMOUNT);
				getCtrlString(sel = CTL_SCARDOP_DESTCARDNO, temp_buf.Z());
				if(temp_buf.NotEmptyS()) {
					SCardTbl::Rec sc_rec;
					THROW(ScObj.SearchCode(0, temp_buf, &sc_rec) > 0);
					THROW_PP_S(ScObj.IsCreditCard(sc_rec.ID), PPERR_SCARDISNTCREDIT, sc_rec.Code);
					sel = CTL_SCARDOP_AMOUNT;
					THROW_PP(D.Amount > 0.0, PPERR_SCTRANSFAMT);
				}
			}
			ASSIGN_PTR(pData, D);
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			SString dest_card_no, temp_buf;
			TDialog::handleEvent(event);
			if(event.isCmd(cmSCardOpCheck)) {
				CCheckCore & r_cc = *ScObj.P_CcTbl;
				if((D.LinkOi.Obj == PPOBJ_CCHECK && D.LinkOi.Id) && r_cc.Search(D.LinkOi.Id) > 0)
					CCheckPane(0, D.LinkOi.Id);
			}
			else if(event.isCmd(cmInputUpdated)) {
				if(event.isCtlEvent(CTL_SCARDOP_DESTCARDNO)) {
					getCtrlString(CTL_SCARDOP_DESTCARDNO, dest_card_no);
					if(dest_card_no.NotEmptyS()) {
						SCardTbl::Rec sc_rec;
						if(ScObj.SearchCode(0, dest_card_no, &sc_rec) > 0 && ScObj.IsCreditSeries(sc_rec.SeriesID)) {
							D.DestSCardID = sc_rec.ID;
							if(sc_rec.PersonID) {
								GetPersonName(sc_rec.PersonID, temp_buf);
								setCtrlString(CTL_SCARDOP_DESTOWNER, temp_buf);
							}
							if(!(D.Flags & D.fEdit))
								ScObj.P_Tbl->GetRest(D.DestSCardID, MAXDATE, &DestRest);
							temp_buf.Z().Cat(DestRest + D.Amount, MKSFMTD(0, 2, NMBF_NOZERO));
							setStaticText(CTL_SCARDOP_ST_DESTREST, temp_buf);
						}
						else {
							D.DestSCardID = 0;
							DestRest = 0.0;
							setCtrlString(CTL_SCARDOP_DESTOWNER, temp_buf.Z());
						}
					}
				}
				else if(event.isCtlEvent(CTL_SCARDOP_AMOUNT)) {
					D.Amount = getCtrlReal(CTL_SCARDOP_AMOUNT);
					temp_buf.Z().Cat(SrcRest + D.Amount, MKSFMTD(0, 2, NMBF_NOZERO)); // @v9.0.4 @fix (-D.Amount)-->(+D.Amount)
					setStaticText(CTL_SCARDOP_ST_REST, temp_buf);
					temp_buf.Z();
					if(D.DestSCardID) {
						temp_buf.Z().Cat(DestRest - D.Amount, MKSFMTD(0, 2, NMBF_NOZERO)); // @v9.0.4 @fix (+D.Amount)-->(-D.Amount)
					}
					setStaticText(CTL_SCARDOP_ST_DESTREST, temp_buf);
				}
				else if(event.isCtlEvent(CTL_SCARDOP_FRZPERIOD)) {
					if(checkdate(OrgExpiry, 0)) {
						long org_delta = 0;
						if(D.Flags & D.fEdit && SCardCore::OpBlock::CheckFreezingPeriod(OrgFreezingPeriod, ZERODATE)) {
							org_delta = OrgFreezingPeriod.GetLength() + 1;
						}
						if(GetPeriodInput(this, CTL_SCARDOP_FRZPERIOD, &D.FreezingPeriod) && D.CheckFreezingPeriod(OrgExpiry)) {
							LDATE new_expiry = plusdate(OrgExpiry, D.FreezingPeriod.GetLength() + 1 - org_delta);
							setCtrlDate(CTL_SCARDOP_EXPIRY, new_expiry);
						}
						else
							setCtrlDate(CTL_SCARDOP_EXPIRY, OrgExpiry);
					}
				}
				else
					return;
			}
			else
				return;
			clearEvent(event);
		}
		SCardCore::OpBlock D;
		PPObjSCard ScObj;
		int    Freezing;
		LDATE  OrgExpiry;
		DateRange OrgFreezingPeriod;
		double SrcRest;
		double DestRest;
	};
	int    ok = -1;
	SCardOpDialog * dlg = new SCardOpDialog(BIN(rBlk.Flags & rBlk.fFreezing));
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(&rBlk);
		if(rBlk.Flags & rBlk.fEdit && !(rBlk.Flags & rBlk.fFreezing))
			DisableOKButton(dlg);
		while(ok < 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&rBlk))
				ok = 1;
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SLAPI PPViewSCardOp::AddItem(int freezing)
{
	int    ok = -1;
	if(Filt.SCardID) {
		SCardTbl::Rec sc_rec;
		THROW(SCObj.CheckRights(SCRDRT_ADDOPS));
		THROW(SCObj.Search(Filt.SCardID, &sc_rec) > 0);
		THROW_PP(!freezing || checkdate(sc_rec.Expiry, 0), PPERR_SCOPFRZONZEXPIRY);
		{
			SCardCore::OpBlock blk;
			blk.Dtm = getcurdatetime_();
			blk.SCardID = Filt.SCardID;
			if(freezing)
				blk.Flags |= SCardCore::OpBlock::fFreezing;
			while(ok <= 0 && EditSCardOp(blk) > 0) {
				TSVector <SCardCore::UpdateRestNotifyEntry> urn_list; // @v9.8.4 TSArray-->TSVector
				if(SCObj.PutUhttOp(Filt.SCardID, blk.Amount) != 0 && SCObj.P_Tbl->PutOpBlk(blk, &urn_list, 1)) {
#ifndef NDEBUG
					SCObj.FinishSCardUpdNotifyList(urn_list);
#else
					// @todo (необходимо уточнить кто должен посылать уведомления: uhtt или мы) SCObj.FinishSCardUpdNotifyList(urn_list);
#endif // !NDEBUG
					ok = 1;
				}
				else
					ok = PPErrorZ();
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

static IMPL_DBE_PROC(dbqf_scardop_extobj_ii)
{
	char   buf[64];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		PPID   obj_type = params[0].lval;
		PPID   obj_id = params[1].lval;
		SString text_buf;
		if(obj_type == PPOBJ_CCHECK) {
			if(obj_id) {
				PPObjSCard sc_obj;
				CCheckTbl::Rec cc_rec;
				if(sc_obj.P_CcTbl->Search(obj_id, &cc_rec) > 0) {
					CCheckCore::MakeCodeString(&cc_rec, text_buf);
				}
			}
		}
		else if(obj_type == PPOBJ_BILL) {
			if(obj_id) {
				BillTbl::Rec bill_rec;
				if(BillObj->Fetch(obj_id, &bill_rec) > 0)
					PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName, text_buf);
			}
		}
		text_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

// static
int PPViewSCardOp::DynFuncExtObjName = 0;

DBQuery * SLAPI PPViewSCardOp::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DbqFuncTab::RegisterDyn(&DynFuncExtObjName, BTS_STRING, dbqf_scardop_extobj_ii, 2, BTS_INT, BTS_INT);

	uint   brw_id = BROWSER_SCARDOP;
	DBQuery * q = 0;
	DBE    dbe_extobj;
	DBE    dbe_sc_code;
	DBE    dbe_scowner_name;
	DBE    dbe_frzprd;
	DBQ  * dbq = 0;
	SCardOpTbl * p_c = new SCardOpTbl;
	SCardTbl * p_s = 0;
	THROW(CheckTblPtr(p_c));
	{
		dbe_extobj.init();
		dbe_extobj.push(p_c->LinkObjType);
		dbe_extobj.push(p_c->LinkObjID);
		dbe_extobj.push((DBFunc)DynFuncExtObjName);
	}
	PPDbqFuncPool::InitObjNameFunc(dbe_sc_code, PPDbqFuncPool::IdObjCodeSCard, p_c->SCardID);
	PPDbqFuncPool::InitObjNameFunc(dbe_scowner_name, PPDbqFuncPool::IdSCardOwnerName, p_c->SCardID);
	{
		dbe_frzprd.init();
		dbe_frzprd.push(p_c->FreezingStart);
		dbe_frzprd.push(p_c->FreezingEnd);
		dbe_frzprd.push((DBFunc)PPDbqFuncPool::IdDateRange);
	}
	dbq = ppcheckfiltid(dbq, p_c->SCardID, Filt.SCardID);
	dbq = & (*dbq && daterange(p_c->Dt, &Filt.Period));
	// @v8.2.8 {
	if(Filt.SCardSerID) {
		p_s = new SCardTbl;
		dbq = & (*dbq && p_c->SCardID == p_s->ID && p_s->SeriesID == Filt.SCardSerID);
	}
	dbq = & (*dbq && realrange(p_c->Amount, Filt.AmtR.low, Filt.AmtR.upp));
	// } @v8.2.8
	q = & select(
		p_c->SCardID,     // #0
		p_c->Dt,          // #1
		p_c->Tm,          // #2
		p_c->Amount,      // #3
		p_c->Rest,        // #4
		dbe_extobj,       // #5
		dbe_sc_code,      // #6
		dbe_scowner_name, // #7
		dbe_frzprd,       // #8
		0L).from(p_c, p_s, 0L).where(*dbq).orderBy(p_c->SCardID, p_c->Dt, p_c->Tm, 0L);
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		SString card_name;
		SCardTbl::Rec card_rec;
		if(Filt.SCardID && SCObj.P_Tbl->Search(Filt.SCardID, &card_rec) > 0) {
			GetObjectName(PPOBJ_SCARDSERIES, card_rec.SeriesID, card_name);
			card_name.CatDiv(':', 1).Cat(card_rec.Code);
		}
		*pSubTitle = card_name;
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete p_c;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewSCardOp::Recover()
{
	int    ok = -1;
	SCardOpTbl & t = SCObj.P_Tbl->ScOp;
	CCheckCore & r_cc = *SCObj.P_CcTbl;

	struct FaultItem {
		PPID   CCheckID;
		PPID   SCardID;
		LDATETIME Dtm;
	};

	SArray fault_list(sizeof(FaultItem));
	PPLogger logger;
	SString msg_buf, fmt_buf, temp_buf;
	SCardOpTbl::Key0 k0;
	//SCardID, Dt, Tm (unique mod);
	MEMSZERO(k0);
	PPWait(1);
	if(t.search(0, &k0, spFirst)) do {
		if(t.data.LinkObjType == PPOBJ_CCHECK) {
			PPID   sc_id = t.data.SCardID;
			PPID   cc_id = t.data.LinkObjID;
			CCheckPacket cc_pack;
			if(r_cc.LoadPacket(cc_id, 0, &cc_pack) > 0) {
				if(cc_pack.Rec.SCardID != sc_id && !cc_pack.AL_Const().SearchAddedID(sc_id, 0)) {
					{
						// PPTXT_INVSCOPCHECKLINK      "Чек '@zstr', на который ссылается операция по карте '@scard' не содержит ссылку на эту карту"
						PPLoadText(PPTXT_INVSCOPCHECKLINK, fmt_buf);
						CCheckCore::MakeCodeString(&cc_pack.Rec, temp_buf.Z());
						PPFormat(fmt_buf, &msg_buf, temp_buf.cptr(), sc_id);
						logger.Log(msg_buf);
					}
					{
						FaultItem fi;
						fi.SCardID = sc_id;
						fi.Dtm.Set(t.data.Dt, t.data.Tm);
						fi.CCheckID = cc_id;
						fault_list.insert(&fi);
					}
				}
			}
		}
	} while(t.search(0, &k0, spNext));
	if(DS.CheckExtFlag(ECF_AVERAGE)) {
		if(fault_list.getCount()) {
			fault_list.sort(CMPF_LONG);
			const uint flc = fault_list.getCount();
			PPTransaction tra(1);
			THROW(tra);
			for(uint i = 0; i < flc; i++) {
				const FaultItem & r_item = *(FaultItem *)fault_list.at(i);
				if(i == 0 || r_item.CCheckID != ((FaultItem *)fault_list.at(i-1))->CCheckID) {
					THROW(SCObj.P_Tbl->RemoveOpByCheck(r_item.CCheckID, 0));
					{
						CCheckPacket cc_pack;
						THROW(r_cc.LoadPacket(r_item.CCheckID, 0, &cc_pack) > 0);
						THROW(r_cc.TurnSCardPayment(&cc_pack, CCheckCore::tscpfSkipUhtt|CCheckCore::tscpfCorrection, 0));
					}
				}
				PPWaitPercent(i, flc);
			}
			THROW(tra.Commit());
		}
	}
	PPWait(0);
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int SLAPI PPViewSCardOp::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	SCardOpTbl::Rec rec;
	const  struct Hdr {
		PPID   SCardID;
		LDATETIME Dtm;
	} * p_hdr = (const Hdr *)pHdr;
	ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = (AddItem(0) > 0) ? 1 : -1;
				break;
			case PPVCMD_ADDFREEZING:
				ok = (AddItem(1) > 0) ? 1 : -1;
				break;
			case PPVCMD_EDITITEM:
				ok = -1;
				if(p_hdr) {
					if(SCObj.CheckRights(SCRDRT_VIEWOPS)) {
						SCardCore::OpBlock blk;
						if(p_hdr->SCardID && SCObj.P_Tbl->GetOp(p_hdr->SCardID, p_hdr->Dtm, &blk) > 0)
							for(int valid = 0; !valid && EditSCardOp(blk) > 0;) {
								TSVector <SCardCore::UpdateRestNotifyEntry> urn_list; // @v9.8.4 TSArray-->TSVector
								if(SCObj.P_Tbl->PutOpBlk(blk, &urn_list, 1)) {
									SCObj.FinishSCardUpdNotifyList(urn_list);
									ok = valid = 1;
								}
								else
	   		    					ok = PPErrorZ();
							}
					}
					else
						ok = PPErrorZ();
				}
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				if(pHdr) {
					if(p_hdr->SCardID && SCObj.P_Tbl->SearchOp(p_hdr->SCardID, p_hdr->Dtm.d, p_hdr->Dtm.t, &rec) > 0 &&
						CONFIRM(PPCFM_DELETE))
						if(SCObj.CheckRights(SCRDRT_RMVOPS) && SCObj.P_Tbl->RemoveOp(p_hdr->SCardID, p_hdr->Dtm.d, p_hdr->Dtm.t, 1))
							ok = 1;
						else
							ok = PPErrorZ();
				}
				break;
			case PPVCMD_DORECOVER:
				ok = Recover();
				break;
		}
	}
	return ok;
}
//
// Implementation of PPALDD_SCardSeries
//
PPALDD_CONSTRUCTOR(SCardSeries)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(SCardSeries) { Destroy(); }

int PPALDD_SCardSeries::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjSCardSeries scs_obj;
		PPSCardSeries rec;
		if(scs_obj.Fetch(rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			STRNSCPY(H.Name, rec.Name);
			H.IssueDate = rec.Issue;
			H.Expiry = rec.Expiry;
			H.PDis = rec.PDis;
			H.Overdraft = rec.MaxCredit;
			H.Flags = rec.Flags;
			H.fCredit = BIN(rec.Flags & SCRDSF_CREDIT);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_SCardSerView
//
PPALDD_CONSTRUCTOR(SCardSerView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(SCardSerView) { Destroy(); }

int PPALDD_SCardSerView::InitData(PPFilt & rFilt, long rsrv)
{
	Extra[1].Ptr = (SCardSeriesView *)rFilt.Ptr;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_SCardSerView::InitIteration(PPIterID iterId, int /*sortId*/, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	PPObjSCardSeriesListWindow * p_v = (PPObjSCardSeriesListWindow *)Extra[1].Ptr;
	return BIN(p_v && p_v->InitIteration());
}

int PPALDD_SCardSerView::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	PPObjSCardSeriesListWindow * p_v = (PPObjSCardSeriesListWindow *)Extra[1].Ptr;
	PPSCardSeries rec;
	if(p_v && p_v->NextIteration(&rec) > 0) {
		STRNSCPY(I.Name, rec.Name);
		// @v9.8.9 STRNSCPY(I.CodeTempl, rec.CodeTempl);
		I.QuotKindID = rec.QuotKindID_s;
		I.PsnKindID  = rec.PersonKindID;
		I.IssueDate  = rec.Issue;
		I.Expiry     = rec.Expiry;
		I.PDis       = fdiv100i(rec.PDis);
		I.MaxCredit  = rec.MaxCredit;
		I.fCredit    = BIN(rec.Flags & SCRDSF_CREDIT);
		return DlRtm::NextIteration(iterId);
	}
	return -1;
}

void PPALDD_SCardSerView::Destroy()
{
	Extra[1].Ptr = 0;
}
//
// Implementation of PPALDD_SCard
//
struct DL600_SCardExt {
	DL600_SCardExt()
	{
		CrEventSurID = 0;
		AcEventSurID = 0;
		MEMSZERO(ScRec);
	}
	PPObjSCard ScObj;
	long   CrEventSurID;
	long   AcEventSurID;
	SCardTbl::Rec ScRec;
};

PPALDD_CONSTRUCTOR(SCard)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new DL600_SCardExt;
	}
}

PPALDD_DESTRUCTOR(SCard)
{
	Destroy();
	delete (DL600_SCardExt *)Extra[0].Ptr;
}

int PPALDD_SCard::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		DL600_SCardExt * p_ext = (DL600_SCardExt *)Extra[0].Ptr;
		p_ext->CrEventSurID = 0;
		p_ext->AcEventSurID = 0;
		if(p_ext && p_ext->ScObj.Search(rFilt.ID, &p_ext->ScRec) > 0) {
			SString temp_buf;
			PPObjSCardSeries scs_obj;
			PPSCardSeries scs_rec;
			H.ID = p_ext->ScRec.ID;
			H.SeriesID = p_ext->ScRec.SeriesID;
			H.OwnerReqID = p_ext->ScRec.PersonID;
			H.AutoGoodsID = p_ext->ScRec.AutoGoodsID;
			STRNSCPY(H.Code, p_ext->ScRec.Code);
			H.IssueDate = p_ext->ScRec.Dt;
			H.Expiry = p_ext->ScRec.Expiry;
			H.UsageTmStart = p_ext->ScRec.UsageTmStart;
			H.UsageTmEnd   = p_ext->ScRec.UsageTmEnd;
			if(p_ext->ScRec.UsageTmStart || p_ext->ScRec.UsageTmEnd) {
				temp_buf.Z().Cat(p_ext->ScRec.UsageTmStart, TIMF_HM);
				if(p_ext->ScRec.UsageTmEnd != p_ext->ScRec.UsageTmStart) {
					temp_buf.CatCharN('.', 2);
					if(p_ext->ScRec.UsageTmEnd)
						temp_buf.Cat(p_ext->ScRec.UsageTmEnd, TIMF_HM);
				}
				temp_buf.CopyTo(H.UsageTmStr, sizeof(H.UsageTmStr));
			}
			H.PeriodTerm = p_ext->ScRec.PeriodTerm;
			H.PeriodCount = p_ext->ScRec.PeriodCount;
			H.PDis   = fdiv100i(p_ext->ScRec.PDis);
			H.Overdraft = p_ext->ScRec.MaxCredit;
			H.Debit = p_ext->ScRec.InTrnovr;
			H.Credit = p_ext->ScRec.Turnover;
			H.Rest = p_ext->ScRec.Rest;
			PPObjSCard::CalcSCardHash(p_ext->ScRec.Code, temp_buf.Z()).CopyTo(H.Hash, sizeof(H.Hash));
			const long _flags = p_ext->ScRec.Flags;
			H.Flags = _flags;
			H.fClosed    = BIN(_flags & SCRDF_CLOSED);
			H.fInherited = BIN(_flags & SCRDF_INHERITED);
			H.fNoGift    = BIN(_flags & SCRDF_NOGIFT);
			H.fNeedActivation = BIN(_flags & SCRDF_NEEDACTIVATION);
			H.fAutoActivation = BIN(_flags & SCRDF_AUTOACTIVATION);
			if(scs_obj.Fetch(p_ext->ScRec.SeriesID, &scs_rec) > 0)
				H.fCredit = BIN(scs_rec.Flags & SCRDSF_CREDIT);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

void PPALDD_SCard::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_STR(n)  (**(SString **)rS.GetPtr(pApl->Get(n)))
	#define _ARG_INT(n)  (*(int *)rS.GetPtr(pApl->Get(n)))
	#define _RET_DBL     (*(double *)rS.GetPtr(pApl->Get(0)))
	#define _RET_INT     (*(int *)rS.GetPtr(pApl->Get(0)))
	#define _RET_STR     (**(SString **)rS.GetPtr(pApl->Get(0)))

	DL600_SCardExt * p_ext = (DL600_SCardExt *)(Extra[0].Ptr);
	SysJournalTbl::Rec sj_rec;
	if(pF->Name == "?GetCreationEvent") {
		long   sur_id = 0;
		if(p_ext) {
			sur_id = p_ext->CrEventSurID;
			if(!sur_id) {
				SysJournal * p_sj = DS.GetTLA().P_SysJ;
				if(p_sj && p_sj->GetObjCreationEvent(PPOBJ_SCARD, H.ID, &sj_rec) > 0) {
					DS.GetTLA().SurIdList.Add(&sur_id, &sj_rec, sizeof(sj_rec));
					p_ext->CrEventSurID = sur_id;
				}
				else
					p_ext->CrEventSurID = -1;
			}
			else if(sur_id < 0)
				sur_id = 0;
		}
		_RET_INT = sur_id;
	}
	else if(pF->Name == "?GetActivationEvent") {
		long   sur_id = 0;
		if(p_ext) {
			sur_id = p_ext->AcEventSurID;
			if(!sur_id) {
				SysJournal * p_sj = DS.GetTLA().P_SysJ;
				PPIDArray acn_list;
				acn_list.add(PPACN_SCARDACTIVATED);
				LDATETIME activation_moment;
				if(p_sj && p_sj->GetLastObjEvent(PPOBJ_SCARD, H.ID, &acn_list, &activation_moment, &sj_rec) > 0) {
					DS.GetTLA().SurIdList.Add(&sur_id, &sj_rec, sizeof(sj_rec));
					p_ext->AcEventSurID = sur_id;
				}
				else
					p_ext->AcEventSurID = -1;
			}
			else if(sur_id < 0)
				sur_id = 0;
		}
		_RET_INT = sur_id;
	}
}
//
// Implementation of PPALDD_SCardList
//
PPALDD_CONSTRUCTOR(SCardList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(SCardList) { Destroy(); }

int PPALDD_SCardList::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(SCard, rsrv);
	H.FltTrnovrBeg  = p_filt->TrnovrPeriod.low;
	H.FltTrnovrEnd  = p_filt->TrnovrPeriod.upp;
	H.FltSeriesID   = p_filt->SeriesID;
	H.FltOwnerReqID = p_filt->PersonID;
	H.FltEmplrReqID = p_filt->EmployerID;
	H.FltFlags      = p_filt->Flags;
	H.Ft_Inherited  = p_filt->Ft_Inherited;
	H.Ft_Closed     = p_filt->Ft_Closed;
	H.FltMinPDis    = R2(p_filt->PDisR.low / 100.0);
	H.FltMaxPDis    = R2(p_filt->PDisR.upp / 100.0);
	H.FltMinTurnover = p_filt->TurnoverR.low;
	H.FltMaxTurnover = p_filt->TurnoverR.upp;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_SCardList::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(SCard);
}

int PPALDD_SCardList::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(SCard);
	I.SCardID = item.ID;
	I.EmployerID = item.EmployerID;
	I.Debit   = item.InTrnovr;
	I.Credit  = item.Turnover;
	I.Rest    = item.Rest;
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_SCardList::Destroy() { DESTROY_PPVIEW_ALDD(SCard); }
//
// Implementation of PPALDD_SCardOpList
//
PPALDD_CONSTRUCTOR(SCardOpList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(SCardOpList) { Destroy(); }

int PPALDD_SCardOpList::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(SCardOp, rsrv);
	{
		SCardOpTotal total;
		SCardTbl::Rec sc_rec;
		p_v->CalcTotal(&total);
		H.InRest = total.InRest;
		H.OutRest = total.OutRest;
		if(SearchObject(PPOBJ_SCARD, p_filt->SCardID, &sc_rec) > 0 && sc_rec.PersonID) {
			PPObjStaffList sl_obj;
			PersonPostArray post_list;
			if(sl_obj.GetPostByPersonList(sc_rec.PersonID, 0, 1, &post_list) > 0) {
				PPStaffEntry sl_rec;
				if(sl_obj.Fetch(post_list.at(0).StaffID, &sl_rec) > 0)
					H.EmployerID = sl_rec.OrgID;
			}
		}
	}
	H.SCardID = p_filt->SCardID;
	H.Beg     = p_filt->Period.low;
	H.End     = p_filt->Period.upp;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_SCardOpList::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(SCardOp);
}

int PPALDD_SCardOpList::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(SCardOp);
	I.Dt      = item.Dt;
	I.Tm      = item.Tm;
	I.CheckID = (item.LinkObjType == PPOBJ_CCHECK) ? item.LinkObjID : 0;
	I.UserID  = item.UserID;
	I.BillID  = (item.LinkObjType == PPOBJ_BILL) ? item.LinkObjID : 0;
	I.Flags   = item.Flags;
	I.Amount  = item.Amount;
	I.Rest    = item.Rest;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_SCardOpList::Destroy() { DESTROY_PPVIEW_ALDD(SCardOp); }
//
//
//
IMPLEMENT_PPFILT_FACTORY(UhttSCardOp); SLAPI UhttSCardOpFilt::UhttSCardOpFilt() : PPBaseFilt(PPFILT_UHTTSCARDOP, 0, 1)
{
	SetFlatChunk(offsetof(UhttSCardOpFilt, ReserveStart),
		offsetof(UhttSCardOpFilt, Reserve) - offsetof(UhttSCardOpFilt, ReserveStart) + sizeof(Reserve));
	Init(1, 0);
}

SLAPI PPViewUhttSCardOp::PPViewUhttSCardOp() : PPView(0, &Filt, PPVIEW_UHTTSCARDOP)
{
	ImplementFlags |= implBrowseArray;
}

SLAPI PPViewUhttSCardOp::~PPViewUhttSCardOp()
{
	List.freeAll();
}

int SLAPI PPViewUhttSCardOp::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	List.freeAll();
	THROW(Helper_InitBaseFilt(pFilt));
	{
		SString     temp_buf;
		ObjTagCore  tc;
		ObjTagItem  tag;
		THROW(Filt.GlobalAccID);
		THROW(tc.GetTag(PPOBJ_GLOBALUSERACC, Filt.GlobalAccID, PPTAG_GUA_SCARDPREFIX, &tag));
		ScPrefix = tag.Val.PStr;
		THROW(!ScPrefix.Empty());
		{
			PPViewCCheck     cc_view;
			CCheckViewItem   cc_item;
			CCheckFilt       cc_filt;
			PPObjCSession    cs_obj;
			CSessionTbl::Rec cs_rec;
			SCardTbl::Rec    sc_rec;
			cc_filt.Period = Filt.Period;
			cc_view.Init_(&cc_filt);
			cc_view.InitIteration(0);
			while(cc_view.NextIteration(&cc_item) > 0) {
				if(ScObj.Search(cc_item.SCardID, &sc_rec) > 0) {
					temp_buf = sc_rec.Code;
					if(temp_buf.CmpPrefix(ScPrefix, 0) == 0) {
						UhttSCardOpViewItem item;
						MEMSZERO(item);
						item.SCardID = cc_item.SCardID;
						item.Dtm.d = cc_item.Dt;
						item.Dtm.t = cc_item.Tm;
						item.SellAmt = MONEYTOLDBL(cc_item.Amount);
						item.SellCount = 1;
						if(cs_obj.Search(cc_item.SessID, &cs_rec) > 0)
							item.DlvrLocID = cs_rec.CashNumber;
						Helper_AddItem(item);
					}
				}
			}
		}
		{
			SCardOpTbl tbl;
			SCardOpTbl::Rec rec;
			BExtQuery  iter_query(&tbl, 1);
			DBQ        * dbq = 0;
			char       k[MAXKEYLEN];
			memzero(&k, sizeof(k));
			Filt.Period.Actualize(ZERODATE);
			dbq = &daterange(tbl.Dt, &Filt.Period);
			iter_query.selectAll().where(*dbq);
			iter_query.initIteration(0, &k, spGe);
			while(iter_query.nextIteration() > 0) {
				tbl.copyBufTo(&rec);
				SCardTbl::Rec sc_rec;
				if(ScObj.Fetch(rec.SCardID, &sc_rec) > 0) {
					temp_buf = sc_rec.Code;
					if(temp_buf.CmpPrefix(ScPrefix, 0) == 0) {
						UhttSCardOpViewItem item;
						MEMSZERO(item);
						item.Dtm.d = rec.Dt;
						item.Dtm.t = rec.Tm;
						item.SCardID = rec.SCardID;
						if(rec.Amount > 0) {
							item.ChargeAmt = rec.Amount;
							item.ChargeCount = 1;
						}
						else {
							item.WithdrawAmt = -rec.Amount;
							item.WithdrawCount = 1;
						}
						item.SCardRest = rec.Rest;
						Helper_AddItem(item);
					}
				}
			}
		}
		UhttSCardOpViewItem total;
		CalcTotal(&total, Filt.Grp);
		if(Filt.Grp == UhttSCardOpFilt::gNone)
			List.sort(PTR_CMPFUNC(LDATETIME));
		else if(Filt.Grp == UhttSCardOpFilt::gTotal)
			List.freeAll();
		else if(Filt.Grp == UhttSCardOpFilt::gDate)
			List.sort(PTR_CMPFUNC(LDATE));
		List.insert(&total);
	}
	CATCHZOK
	return ok;
}

#define _INCREASE_VAL(dest, src)   \
	dest.SellAmt += src.SellAmt;  \
	dest.SellCount += src.SellCount;  \
	dest.ChargeAmt += src.ChargeAmt;  \
	dest.ChargeCount += src.ChargeCount;  \
	dest.WithdrawAmt += src.WithdrawAmt;  \
	dest.WithdrawCount += src.WithdrawCount;

int SLAPI PPViewUhttSCardOp::Helper_AddItem(UhttSCardOpViewItem & rItem)
{
	int    ok = 1;
	switch(Filt.Grp) {
		case UhttSCardOpFilt::gNone:
		case UhttSCardOpFilt::gTotal:
			List.insert(&rItem);
			break;
		case UhttSCardOpFilt::gDate:
			{
				uint idx = 0;
				THROW(rItem.Dtm.d);
				int r = List.lsearch(&rItem.Dtm.d, &idx, PTR_CMPFUNC(LDATE), 0, 0);
				if(r == 1) {
					UhttSCardOpViewItem & r_item = List.at(idx);
					_INCREASE_VAL(r_item, rItem);
				}
				else {
					/* Расчет остатка по всем картам на дату */
					rItem.SCardRest = 0;
					{
						SString       temp_buf;
						PPViewSCard   sc_view;
						SCardViewItem sc_item;
						SCardFilt     sc_filt;
						THROW(sc_view.Init_(&sc_filt));
						THROW(sc_view.InitIteration());
						while(sc_view.NextIteration(&sc_item) > 0) {
							temp_buf = sc_item.Code;
							if(temp_buf.CmpPrefix(ScPrefix, 0) == 0) {
								double rest = 0;
								THROW(ScObj.P_Tbl->GetRest(sc_item.ID, rItem.Dtm.d, &rest));
								rItem.SCardRest += rest;
							}
						}
					}
					List.insert(&rItem);
				}
			}
			break;
		case UhttSCardOpFilt::gSCard:
			{
				uint idx = 0;
				uint offs = offsetof(UhttSCardOpViewItem, SCardID);
				THROW(rItem.SCardID);
				int r = List.lsearch(&rItem.SCardID, &idx, CMPF_LONG, offs, 0);
				if(r == 1) {
					UhttSCardOpViewItem & r_item = List.at(idx);
					_INCREASE_VAL(r_item, rItem);
				}
				else {
					THROW(ScObj.P_Tbl->GetRest(rItem.SCardID, Filt.Period.upp, &rItem.SCardRest));
					List.insert(&rItem);
				}
			}
			break;
		case UhttSCardOpFilt::gDlvrLoc:
			{
				uint idx = 0;
				uint offs = offsetof(UhttSCardOpViewItem, DlvrLocID);
				THROW(rItem.DlvrLocID);
				int r = List.lsearch(&rItem.DlvrLocID, &idx, CMPF_LONG, offs, 0);
				if(r == 1) {
					UhttSCardOpViewItem & r_item = List.at(idx);
					_INCREASE_VAL(r_item, rItem);
				}
				else {
					rItem.SCardRest = 0;
					List.insert(&rItem);
				}
			}
			break;
		default:
			break;
	}
	CATCHZOK
	return ok;
}

#undef _INCREASE_VAL

int SLAPI PPViewUhttSCardOp::InitIteration()
{
	Counter.Init(List.getCount());
	return 1;
}

int FASTCALL PPViewUhttSCardOp::NextIteration(UhttSCardOpViewItem * pItem)
{
	int    ok = -1;
	if(pItem && Counter < List.getCount()) {
		ASSIGN_PTR(pItem, List.at(Counter));
		Counter.Increment();
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewUhttSCardOp::CalcTotal(UhttSCardOpViewItem * pTotal, int grp) const
{
	int   ok = 0;
	if(pTotal) {
		MEMSZERO(*pTotal);
		for(uint i = 0; i < List.getCount(); i++) {
			UhttSCardOpViewItem & r_item = List.at(i);
			pTotal->SellAmt += r_item.SellAmt;
			pTotal->SellCount += r_item.SellCount;
			pTotal->ChargeAmt += r_item.ChargeAmt;
			pTotal->ChargeCount += r_item.ChargeCount;
			pTotal->WithdrawAmt += r_item.WithdrawAmt;
			pTotal->WithdrawCount += r_item.WithdrawCount;
			if(grp == UhttSCardOpFilt::gSCard)
				pTotal->SCardRest += r_item.SCardRest;
		}
		ok = 1;
	}
	return ok;
}
//
// Implementation of PPALDD_UhttSCardOpArray
//
PPALDD_CONSTRUCTOR(UhttSCardOpArray)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(UhttSCardOpArray) { Destroy(); }

int PPALDD_UhttSCardOpArray::InitData(PPFilt & rFilt, long rsrv)
{
	TSArray <UhttSCardOpViewItem> * p_list = (TSArray <UhttSCardOpViewItem> *)rFilt.Ptr;
	Extra[0].Ptr = p_list;
	MEMSZERO(H);
	if(p_list && p_list->getCount()) {
		UhttSCardOpViewItem & r_item = p_list->at(p_list->getEnd());
		H.TotalSellCnt = r_item.SellCount;
		H.TotalSellAmt = r_item.SellAmt;
		H.TotalChargeCnt = r_item.ChargeCount;
		H.TotalChargeAmt = r_item.ChargeAmt;
		H.TotalWithdrawCnt = r_item.WithdrawCount;
		H.TotalWithdrawAmt = r_item.WithdrawAmt;
		H.TotalSCardRest = r_item.SCardRest;
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_UhttSCardOpArray::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	TSArray <UhttSCardOpViewItem> * p_list = (TSArray <UhttSCardOpViewItem> *)Extra[0].Ptr;
	CALLPTRMEMB(p_list, setPointer(0));
	return 1;
}

int PPALDD_UhttSCardOpArray::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	TSArray <UhttSCardOpViewItem> * p_list = (TSArray <UhttSCardOpViewItem> *)Extra[0].Ptr;
	if(p_list && p_list->getCount() && ((int)p_list->getPointer() < p_list->getEnd())) {
		UhttSCardOpViewItem & r_item = p_list->at(p_list->getPointer());
		I.SCardID = r_item.SCardID;
		I.DlvrLocID = r_item.DlvrLocID;
		I.Dt = r_item.Dtm.d;
		I.Tm = r_item.Dtm.t;
		I.SellCount = r_item.SellCount;
		I.SellAmt = r_item.SellAmt;
		I.ChargeCount = r_item.ChargeCount;
		I.ChargeAmt = r_item.ChargeAmt;
		I.WithdrawCount = r_item.WithdrawCount;
		I.WithdrawAmt = r_item.WithdrawAmt;
		I.SCardRest = r_item.SCardRest;
		ok = DlRtm::NextIteration(iterId);
		p_list->incPointer();
	}
	return ok;
}
