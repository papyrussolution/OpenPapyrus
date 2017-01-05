// PSALES.CPP
// Copyright (c) A.Sobolev 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2014, 2015, 2016
//
#include <pp.h>
#pragma hdrstop
//
// PredictSalesStat
//
SLAPI PredictSalesStat::PredictSalesStat(PPID coeffQkID, const  PPQuotArray * pCoeffQList) : CoeffQkID(coeffQkID)
{
	P_CoeffQList = pCoeffQList;
	P_List = 0;
	memzero(&LocID, offsetof(PredictSalesStat, Flags) - offsetof(PredictSalesStat, LocID) + sizeof(Flags));
}

SLAPI PredictSalesStat::~PredictSalesStat()
{
	delete P_List;
}

int SLAPI PredictSalesStat::Init()
{
	ZDELETE(P_List);
	memzero(&LocID, offsetof(PredictSalesStat, Flags) - offsetof(PredictSalesStat, LocID) + sizeof(Flags));
	return 1;
}

PredictSalesStat & FASTCALL PredictSalesStat::operator = (const PredictSalesStat & s)
{
	ZDELETE(P_List);
	memcpy(this, &s, sizeof(*this));
	P_List = 0;
	if(s.P_List)
		P_List = new SArray(*s.P_List);
	return *this;
}

struct LsEntry {
	int16  Day;
	int16  Reserve; // @alignment
	double Qtty;
	double Amt;
};

int FASTCALL PredictSalesStat::Step(const PredictSalesItem * pItem)
{
	int    ok = 1;
	// @v7.8.12 {
	int    skip = 0;
	double coeff = 1.0;
	if(P_CoeffQList && CoeffQkID && P_CoeffQList->getCount()) {
		uint   qpos = 0;
		QuotIdent qi(pItem->Dt, LocID, CoeffQkID, 0, 0);
		if(P_CoeffQList->SearchNearest(qi, &qpos) > 0) {
			const PPQuot & r_q = P_CoeffQList->at(qpos);
			if(!r_q.Period.IsZero()) {
				if(r_q.Flags & PPQuot::fZero)
					skip = 1;
				else if(r_q.Quot > 0.0 && r_q.Quot <= 10.0) {
					coeff = r_q.Quot;
				}
			}
		}
	}
	// } @v7.8.12
	if(!skip) {
		Count++;
		if(!FirstPointDate || diffdate(FirstPointDate, pItem->Dt) > 0)
			FirstPointDate = pItem->Dt;
		const double qtty = pItem->Qtty * coeff;
		const double amt = pItem->Amount * coeff;
		QttySum   += qtty;
		QttySqSum += qtty * qtty;
		AmtSum    += amt;
		AmtSqSum  += amt * amt;
		if(Flags & PSSF_USELSSLIN) {
			SETIFZ(P_List, new SArray(sizeof(LsEntry), 32, O_ARRAY));
			THROW_MEM(P_List);
			LsEntry entry;
			PredictSalesCore::ShrinkDate(pItem->Dt, &entry.Day);
			entry.Qtty = qtty;
			entry.Amt = amt;
			THROW_SL(P_List->insert(&entry));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PredictSalesStat::Step(const PsiArray * pList, const DateRange * pPeriod)
{
	PredictSalesItem * p_si;
	for(uint i = 0; pList->enumItems(&i, (void **)&p_si);)
		if(!pPeriod || pPeriod->CheckDate(p_si->Dt))
			if(!Step(p_si))
				return 0;
	return 1;
}

int SLAPI PredictSalesStat::Finish()
{
	if(Flags & PSSF_USELSSLIN) {
		if(P_List && P_List->getCount() > 1) {
			P_List->sort(PTR_CMPFUNC(int16));
			uint   i;
			LMIDX  dim = (LMIDX)P_List->getCount();
			LVect  x_vect;
			LVect  y_vect;
			x_vect.init(dim);
			y_vect.init(dim);
			LsEntry * p_entry;
			for(i = 0; P_List->enumItems(&i, (void **)&p_entry);) {
				x_vect.set(i-1, p_entry->Day);
				y_vect.set(i-1, p_entry->Qtty);
			}
			QttyLss.Solve(x_vect, y_vect);
			for(i = 0; P_List->enumItems(&i, (void **)&p_entry);)
				y_vect.set(i-1, p_entry->Amt);
			AmtLss.Solve(x_vect, y_vect);
			ZDELETE(P_List);
		}
	}
	return 1;
}

double SLAPI PredictSalesStat::Predict(int pssValType, LDATE dt, double * pYErr) const
{
	if(Flags & PSSF_USELSSLIN && Count > 1) {
		int16 day = 0;
		PredictSalesCore::ShrinkDate(dt, &day);
		if(pssValType == PSSV_QTTY)
			return QttyLss.Estimation(day, pYErr);
		else if(pssValType == PSSV_AMT)
			return AmtLss.Estimation(day, pYErr);
	}
	return 0;
}

double FASTCALL PredictSalesStat::GetAverage(int pssValType) const
{
	if(Count)
		if(pssValType == PSSV_QTTY)
			return (QttySum / Count);
		else if(pssValType == PSSV_AMT)
			return (AmtSum / Count);
	return 0;
}

double FASTCALL PredictSalesStat::GetSigma(int pssValType) const
{
	if(Count > 1)
		if(pssValType == PSSV_QTTY)
			return sqrt((QttySqSum - GetAverage(pssValType) * QttySum) / (Count - 1));
		else if(pssValType == PSSV_AMT)
			return sqrt((AmtSqSum - GetAverage(pssValType) * AmtSum) / (Count - 1));
	return 0;
}

double FASTCALL PredictSalesStat::GetVar(int pssValType) const
{
	return fdivnz(GetSigma(pssValType), GetAverage(pssValType));
}

double FASTCALL PredictSalesStat::GetTrnovr(int pssValType) const
{
	return fdivnz(1, GetAverage(pssValType));
}
//
// PredictSalesCore
//
SLAPI PredictSalesCore::PredictSalesCore() : PredictSalesTbl()
{
	IsLocTabUpdated = 0;
	IsHldTabUpdated = 0;
	P_LocTab = 0;
	P_HldTab = 0;
	P_SaveHldTab = 0;
	ReadLocTab();
	ReadHolidays();
}

SLAPI PredictSalesCore::~PredictSalesCore()
{
	delete P_LocTab;
	delete P_HldTab;
	delete P_SaveHldTab;
}

int SLAPI PredictSalesCore::CheckTableStruct()
{
	int    ok = 1;
	RECORDSIZE sz = 0;
	int16  num_keys = 0;
	THROW_DB(StT.getRecSize(&sz));
	THROW_PP(sz == sizeof(GoodsStatTbl::Rec), PPERR_GOODSSTATTBLINVSTRUCT);
	THROW_DB(StT.getNumKeys(&num_keys));
	THROW_PP(num_keys == 2, PPERR_GOODSSTATTBLINVSTRUCT);
	CATCHZOK
	return ok;
}

int SLAPI PredictSalesCore::SaveHolidays()
{
	ZDELETE(P_SaveHldTab);
	if(P_HldTab)
		P_SaveHldTab = new SArray(*P_HldTab);
	return 1;
}

int SLAPI PredictSalesCore::RestoreHolidays()
{
	ZDELETE(P_HldTab);
	if(P_SaveHldTab)
		P_HldTab = new SArray(*P_SaveHldTab);
	return 1;
}

int SLAPI PredictSalesCore::SearchHoliday(int16 locIdx, int16 day, uint * pPos) const
{
	HldTabEntry entry;
	entry.LocIdx = locIdx;
	entry.Day = day;
	return BIN(P_HldTab && P_HldTab->bsearch(&entry, pPos, CMPF_LONG));
}

int FASTCALL PredictSalesCore::SearchHoliday(HldTabEntry entry) const
{
	return BIN(P_HldTab->bsearch(&entry, 0, CMPF_LONG));
}

int SLAPI PredictSalesCore::SetHldEntry(const HldTabEntry * pEntry, int rmv)
{
	int    ok = -1;
	SETIFZ(P_HldTab, new SArray(sizeof(HldTabEntry)));
	if(P_HldTab) {
		uint   pos = 0;
		if(SearchHoliday(pEntry->LocIdx, pEntry->Day, &pos)) {
			if(rmv) {
				P_HldTab->atFree(pos);
				IsHldTabUpdated = 1;
				ok = 1;
			}
		}
		else if(!rmv) {
			P_HldTab->ordInsert(pEntry, &(pos = 0), CMPF_LONG);
			IsHldTabUpdated = 1;
			ok = 1;
		}
	}
	else
		ok = PPSetErrorNoMem();
	return ok;
}

int SLAPI PredictSalesCore::SetHoliday(PPID locID, LDATE dt, int rmv)
{
	HldTabEntry entry;
	ShrinkHoliday(dt, &entry.Day);
	if(entry.Day == 0)
		return -1;
	else if(locID == 0) {
		entry.LocIdx = 0;
		return SetHldEntry(&entry, rmv);
	}
	else {
		int    r = rmv ? ShrinkLoc(locID, &entry.LocIdx) : (AddLocEntry(locID, &entry.LocIdx), 1);
		return (r > 0) ? SetHldEntry(&entry, rmv) : -1;
	}
}

int SLAPI PredictSalesCore::EnumHolidays(PPID locID, LDATE * pDt)
{
	if(P_HldTab) {
		HldTabEntry entry;
		if(locID == 0)
			entry.LocIdx = 0;
		else if(!ShrinkLoc(locID, &entry.LocIdx))
			return 0;
		ShrinkHoliday(*pDt, &entry.Day);
		HldTabEntry * p_entry;
		for(uint i = 0; P_HldTab->enumItems(&i, (void **)&p_entry);)
			if(p_entry->LocIdx == entry.LocIdx) {
				if(p_entry->Day > entry.Day) {
					ExpandHoliday(p_entry->Day, pDt);
					return 1;
				}
			}
			/*
			else if(p_entry->LocIdx > entry.LocIdx)
				break;
			*/
	}
	return 0;
}

int SLAPI PredictSalesCore::ShrinkHoliday(LDATE dt, int16 * pDay)
{
	int    ok = 1;
	int16  day = 0;
	if(!dt)
		day = -MAXSHORT;
	else {
		int kind = PPHolidays::GetKind(dt);
		if(kind == 1)
			ok = ShrinkDate(dt, &day);
		else if(kind == 2) {
			ShrinkDate(encodedate(dt.day(), dt.month(), 1996), &day);
			day = -day-100;
		}
		else if(kind == 3)
			day = (int16)-abs(dt.day());
		else
			ok = 0;
	}
	ASSIGN_PTR(pDay, day);
	return ok;
}

int SLAPI PredictSalesCore::ExpandHoliday(int16 day, LDATE * pDt)
{
	LDATE  dt = ZERODATE;
	if(day == -MAXSHORT)
		dt = ZERODATE;
	else if(day <= -100) {
		ExpandDate(-day-100, &dt);
		encodedate(dt.day(), dt.month(), 0, &dt);
	}
	else if(day < 0)
		encodedate(-day, 0, 0, &dt);
	else
		ExpandDate(day, &dt);
	ASSIGN_PTR(pDt, dt);
	return 1;
}

int SLAPI PredictSalesCore::SetWeekdayAsHoliday(PPID locID, int dayOfWeek /* 1..7 */, int rmv)
{
	HldTabEntry entry;
	if(locID == 0)
		entry.LocIdx = 0;
	else if(!ShrinkLoc(locID, &entry.LocIdx))
		return -1;
	ShrinkHoliday(encodedate(dayOfWeek, 0, 0), &entry.Day);
	return SetHldEntry(&entry, rmv);
}

int SLAPI PredictSalesCore::SetDayOfYearAsHoliday(PPID locID, int day, int month, int rmv)
{
	HldTabEntry entry;
	if(locID == 0)
		entry.LocIdx = 0;
	else if(!ShrinkLoc(locID, &entry.LocIdx))
		return -1;
	ShrinkHoliday(encodedate(day, month, 0), &entry.Day);
	return SetHldEntry(&entry, rmv);
}

int SLAPI PredictSalesCore::IsHoliday(const ObjIdListFilt * pLocList, LDATE dt)
{
	int    r = 0;
	if(P_HldTab && pLocList) {
		const  int all_locs = BIN(pLocList->IsEmpty());
		int    d, m, y;
		int16  day, dayofyear, loc_idx = 0;
		int    dw = dayofweek(&dt, 1);
		const PPIDArray & r_loc_list = pLocList->Get();
		decodedate(&d, &m, &y, &dt);
		ShrinkDate(encodedate(d, m, 1996), &dayofyear);
		dayofyear = -dayofyear-100;
		ShrinkDate(dt, &day);
		for(uint i = 0; all_locs || i < r_loc_list.getCount(); i++) {
			if(!all_locs) {
				ShrinkLoc(r_loc_list.at(i), &loc_idx);
				if(SearchHoliday(loc_idx, day, 0))
					r = 1;
				if(SearchHoliday(loc_idx, dayofyear, 0))
					r = 1;
				if(SearchHoliday(loc_idx, -dw, 0))
					r = 1;
			}
			if(SearchHoliday(0, day, 0))
				r = 1;
			if(SearchHoliday(0, dayofyear, 0))
				r = 1;
			if(SearchHoliday(0, -dw, 0))
				r = 1;
			if(all_locs || !r)
				break;
		}
	}
	return r;
}

int SLAPI PredictSalesCore::IsHoliday(PPID locID, LDATE dt) const
{
	int    r = 0;
	assert(locID != 0);
	if(P_HldTab) {
		int16  loc_idx = 0;
		ShrinkLoc(locID, &loc_idx);
		r = IsHolidayByLocIdx(loc_idx, dt);
	}
	return r;
}
//
// Descr: Эта функция по спецификации эквивалентна функции
//   PredictSalesCore::IsHoliday(const ObjIdListFilt * pLocList, LDATE dt).
//   Из-за того, что она слишком часто вызывается при построении таблицы продаж
//   определяем ее тело с условием максимизации быстродействия: экономим на
//   том, что вызывающая функция не должна будет формировать искусственный список
//   из одного элемента, а так же за счет некоторой оптимизации кода на исключении
//   универсальности.
//
int SLAPI PredictSalesCore::IsHolidayByLocIdx(int16 locIdx, LDATE dt) const
{
	int    r = 0;
	assert(locIdx != 0);
	if(P_HldTab) {
		HldTabEntry hte;
		hte.LocIdx = 0;
		int    d, m, y;
		int16  day;
		decodedate(&d, &m, &y, &dt);
		ShrinkDate(dt, &day);
		hte.Day = day;
		hte.LocIdx = locIdx;
		if(SearchHoliday(hte))
			r = 1;
		else {
			int16  dayofyear;
			ShrinkDate(encodedate(d, m, 1996), &dayofyear);
			dayofyear = -dayofyear-100;
			hte.Day = dayofyear;
			if(SearchHoliday(hte))
				r = 1;
			else {
				int16 dw = -(int16)dayofweek(&dt, 1);
				hte.Day = dw;
				if(SearchHoliday(hte))
					r = 1;
				else {
					hte.LocIdx = 0;
					hte.Day = day;
					if(SearchHoliday(0, day, 0))
						r = 1;
					else {
						hte.Day = dayofyear;
						if(SearchHoliday(hte))
							r = 1;
						else {
							hte.Day = dw;
							if(SearchHoliday(hte))
								r = 1;
						}
					}
				}
			}
		}
	}
	return r;
}

int SLAPI PredictSalesCore::ClearHolidays()
{
	ZDELETE(P_HldTab);
	IsHldTabUpdated = 1;
	return 1;
}

int SLAPI PredictSalesCore::WriteHolidays(int use_ta)
{
	int    ok = 1;
	if(P_HldTab) {
		HldTabEntry * p_entry;
		BExtInsert bei(this);
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW_DB(deleteFrom(this, 0, this->RType == (long)PSRECTYPE_HOLIDAY));
		for(uint i = 0; P_HldTab->enumItems(&i, (void **)&p_entry);) {
			PredictSalesTbl::Rec rec;
			MEMSZERO(rec);
			rec.RType = PSRECTYPE_HOLIDAY;
			rec.Loc = p_entry->LocIdx;
			rec.Dt = p_entry->Day;
			THROW_DB(bei.insert(&rec));
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PredictSalesCore::ReadHolidays()
{
	int    ok = 1;
	SETIFZ(P_HldTab, new SArray(sizeof(HldTabEntry)));
	PredictSalesTbl::Key0 k0;
	BExtQuery q(this, 0, 128);
	THROW_MEM(P_HldTab);
	q.select(this->Loc, this->Dt, 0L).where(this->RType == (long)PSRECTYPE_HOLIDAY);
	MEMSZERO(k0);
	k0.RType = PSRECTYPE_HOLIDAY;
	k0.Loc = -MAXSHORT;
	for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
		HldTabEntry entry;
		entry.LocIdx = data.Loc;
		entry.Day = data.Dt;
		THROW_SL(P_HldTab->insert(&entry));
	}
	P_HldTab->sort(CMPF_LONG);
	CATCHZOK
	return ok;
}

int SLAPI PredictSalesCore::AddLocEntry(PPID locID, int16 * pLocIdx)
{
	LocTabEntry * p_entry, entry;
	int16  max_idx = 0;
	SETIFZ(P_LocTab, new SArray(sizeof(LocTabEntry)));
	for(uint i = 0; P_LocTab->enumItems(&i, (void **)&p_entry);) {
		if(p_entry->LocIdx > max_idx)
			max_idx = p_entry->LocIdx;
		if(p_entry->LocID == locID) {
			ASSIGN_PTR(pLocIdx, p_entry->LocIdx);
			return 2;
		}
	}
	entry.LocID = locID;
	entry.LocIdx = max_idx+1;
	P_LocTab->insert(&entry);
	IsLocTabUpdated = 1;
	ASSIGN_PTR(pLocIdx, entry.LocIdx);
	return 1;
}

int SLAPI PredictSalesCore::WriteLocTab(int use_ta)
{
	int    ok = 1;
	PredictSalesTbl::Key0 k0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		MEMSZERO(k0);
		k0.RType = PSRECTYPE_LOCTAB;
		if(search(0, &k0, spGe) && k0.RType == PSRECTYPE_LOCTAB) do {
			THROW_DB(deleteRec());
		} while(search(0, &k0, spNext) && k0.RType == PSRECTYPE_LOCTAB);
		if(P_LocTab) {
			LocTabEntry * p_entry;
			for(uint i = 0; P_LocTab->enumItems(&i, (void **)&p_entry);) {
				PredictSalesTbl::Rec rec;
				MEMSZERO(rec);
				rec.RType = PSRECTYPE_LOCTAB;
				rec.GoodsID = p_entry->LocID;
				rec.Loc = p_entry->LocIdx;
				THROW_DB(insertRecBuf(&rec));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PredictSalesCore::ReadLocTab()
{
	int    ok = 1;
	ZDELETE(P_LocTab);
	PredictSalesTbl::Key0 k0;
	MEMSZERO(k0);
	k0.RType = PSRECTYPE_LOCTAB;
	BExtQuery q(this, 0, 128);
	q.select(this->Loc, this->GoodsID, 0L).where(this->RType == (long)PSRECTYPE_LOCTAB);
	THROW_MEM(P_LocTab = new SArray(sizeof(LocTabEntry)));
	for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
		LocTabEntry entry;
		entry.LocID  = data.GoodsID;
		entry.LocIdx = data.Loc;
		THROW_SL(P_LocTab->insert(&entry));
	}
	{
		PPObjLocation loc_obj;
		LocationTbl::Rec loc_rec;
		for(SEnum en = loc_obj.P_Tbl->Enum(LOCTYP_WAREHOUSE, 0, LocationCore::eoIgnoreParent); en.Next(&loc_rec) > 0;) {
			int16 loc_idx = 0;
			AddLocEntry(loc_rec.ID, &loc_idx);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PredictSalesCore::GetLocList(PPIDArray * pList) const
{
	LocTabEntry * p_entry;
	for(uint i = 0; P_LocTab->enumItems(&i, (void **)&p_entry);)
		pList->add(p_entry->LocID);
	return 1;
}

int FASTCALL PredictSalesCore::ShrinkLoc(PPID locID, int16 * pLocIdx) const
{
	if(P_LocTab) {
		uint   pos = 0;
		if(P_LocTab->lsearch(&locID, &pos, CMPF_LONG)) {
			ASSIGN_PTR(pLocIdx, ((LocTabEntry *)P_LocTab->at(pos))->LocIdx);
			return 1;
		}
	}
	ASSIGN_PTR(pLocIdx, 0);
	return 0;
}

int FASTCALL PredictSalesCore::ExpandLoc(int16 locIdx, PPID * pLocID)
{
	LocTabEntry * p_entry;
	if(P_LocTab)
		for(uint i = 0; P_LocTab->enumItems(&i, (void **)&p_entry);) {
			if(p_entry->LocIdx == locIdx) {
				ASSIGN_PTR(pLocID, p_entry->LocID);
				return 1;
			}
		}
	ASSIGN_PTR(pLocID, 0);
	return 0;
}

int FASTCALL PredictSalesCore::ShrinkDate(LDATE dt, int16 * pSDt)
{
	int16  diff = (!dt) ? 0 : ((dt == MAXDATE) ? MAXSHORT : (int16)diffdate(dt, encodedate(31, 12, 1995)));
	ASSIGN_PTR(pSDt, diff);
	return 1;
}

int FASTCALL PredictSalesCore::ExpandDate(int16 sdt, LDATE * pDt)
{
	LDATE  dt = (sdt <= 0) ? ZERODATE : ((sdt == MAXSHORT) ? MAXDATE : plusdate(encodedate(31, 12, 1995), sdt));
	ASSIGN_PTR(pDt, dt);
	return 1;
}

int SLAPI PredictSalesCore::Finish(int locTabOnly, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(IsLocTabUpdated)
			THROW(WriteLocTab(0));
		if(!locTabOnly) {
			if(IsHldTabUpdated)
				THROW(WriteHolidays(0));
		}
		THROW(tra.Commit());
		IsLocTabUpdated = 0;
		if(!locTabOnly)
			IsHldTabUpdated = 0;
	}
	CATCHZOK
	return ok;
}

int SLAPI PredictSalesCore::SetValue(PPID locID, PPID goodsID, LDATE dt, double qtty, double amount)
{
	long   f = (qtty == 0) ? PRSALF_ZERO : 0;
	return AddItem(PSRECTYPE_DAY, locID, goodsID, dt, qtty, amount, f);
}

int SLAPI PredictSalesCore::Remove(const PredictSalesTbl::Rec * pRec)
{
	return deleteFrom(this, 0, this->RType == (long)pRec->RType && this->Loc == (long)pRec->Loc &&
		this->GoodsID == (long)pRec->GoodsID && this->Dt == (long)pRec->Dt) ? 1 : PPSetErrorDB();
}

int SLAPI PredictSalesCore::AddItem(long typ, PPID locID, PPID goodsID,
	LDATE dt, double qtty, double amount, long f)
{
	int    ok = 1;
	int16  loc_idx = 0;
	int16  dt_idx = 0;
	PredictSalesTbl::Key0 k0;

	AddLocEntry(locID, &loc_idx);
	ShrinkDate(dt, &dt_idx);

	k0.RType = (int16)typ;
	k0.Loc = loc_idx;
	k0.GoodsID = goodsID;
	k0.Dt = dt_idx;

	if(searchForUpdate(0, &k0, spEq)) {
		data.Quantity = (float)qtty;
		data.Amount = (float)amount;
		data.Flags = (int16)f;
		ok = updateRec() ? 2 : PPSetErrorDB(); // @sfu
	}
	else {
		data.RType = (int16)typ;
		data.GoodsID = goodsID;
		data.Loc = loc_idx;
		data.Dt = dt_idx;
		data.Quantity = (float)qtty;
		data.Amount = (float)amount;
		data.Flags = (int16)f;
		ok = insertRec() ? 1 : PPSetErrorDB();
	}
	return ok;
}

void * SLAPI PredictSalesCore::SetKey(PredictSalesTbl::Key0 * pKey, int typ, PPID locID, PPID goodsID, LDATE dt)
{
	if(pKey) {
		memzero(pKey, sizeof(*pKey));
		pKey->RType   = typ;
		pKey->GoodsID = goodsID;
		ShrinkLoc(locID, &pKey->Loc);
		ShrinkDate(dt, &pKey->Dt);
	}
	return pKey;
}

int SLAPI PredictSalesCore::SearchItem(int typ, PPID locID, PPID goodsID, LDATE dt, void * pRec)
{
	PredictSalesTbl::Key0 k;
	return SearchByKey(this, 0, SetKey(&k, typ, locID, goodsID, dt), pRec);
}

int SLAPI PredictSalesCore::RemovePeriod(PPID locID, PPID goodsID, const DateRange * pPeriod, int use_ta)
{
	int    ok = 1, ta = 0;
	THROW(PPStartTransaction(&ta, use_ta));
	if(locID < 0) {
		LocTabEntry * p_entry;
		if(P_LocTab)
			for(uint i = 0; P_LocTab->enumItems(&i, (void **)&p_entry);)
				if(p_entry->LocID >= 0)
					THROW(RemovePeriod(p_entry->LocID, goodsID, pPeriod, 0)); // @recursion
	}
	else {
		int16  loc_idx = 0, lowdt = 0, maxdt = MAXSHORT;
		ShrinkLoc(locID, &loc_idx);
		if(pPeriod) {
			ShrinkDate(pPeriod->low, &lowdt);
			if(pPeriod->upp)
				ShrinkDate(pPeriod->upp, &maxdt);
		}
		// @v8.0.11 {
		{
			// RType, Loc, GoodsID, Dt (unique mod);
			PredictSalesTbl::Key0 k0;
			k0.RType = PSRECTYPE_DAY;
			k0.Loc = loc_idx;
			k0.GoodsID = goodsID;
			k0.Dt = lowdt;
			if(search(0, &k0, spGe) && data.RType == PSRECTYPE_DAY && data.Loc == loc_idx && data.GoodsID == goodsID && data.Dt <= maxdt) do {
				THROW_DB(deleteRec());
			} while(search(0, &k0, spNext) && data.RType == PSRECTYPE_DAY && data.Loc == loc_idx && data.GoodsID == goodsID && data.Dt <= maxdt);
		}
		// } @v8.0.11
		/*
		THROW_DB(deleteFrom(this, 0, this->RType == (long)PSRECTYPE_DAY && this->Loc == (long)loc_idx &&
			this->GoodsID == (long)goodsID && this->Dt >= (long)lowdt && this->Dt <= (long)maxdt));
		*/
	}
	THROW(PPCommitWork(&ta));
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PredictSalesCore::GetFirstDate(PPID locID, PPID goodsID, LDATE * pDt)
{
	int    ok = -1;
	int16  loc_idx = 0;
	PredictSalesTbl::Key0 k;
	SetKey(&k, PSRECTYPE_DAY, locID, goodsID, ZERODATE);
	ShrinkLoc(locID, &loc_idx);
	if(search(0, &k, spGe) && k.RType == PSRECTYPE_DAY && k.Loc == loc_idx && k.GoodsID == goodsID) {
		ExpandDate(data.Dt, pDt);
		ok = 1;
	}
	else
		ASSIGN_PTR(pDt, ZERODATE);
	return ok;
}

int SLAPI PredictSalesCore::GetPeriod(const ObjIdListFilt * pLocList, PPID goodsID, DateRange * pPeriod)
{
	DateRange p;
	PPIDArray loc_list;
	if(pLocList == 0 || !pLocList->IsExists())
		GetLocList(&loc_list);
	else
		loc_list = pLocList->Get();
	p.SetZero();
	for(uint i = 0; i < loc_list.getCount(); i++) {
		int16  loc_idx = 0;
		PPID   loc_id = loc_list.at(i);
		LDATE  dt = ZERODATE;
		PredictSalesTbl::Key0 k;
		SetKey(&k, PSRECTYPE_DAY, loc_id, goodsID, ZERODATE);
		ShrinkLoc(loc_id, &loc_idx);
		if(search(0, &k, spGe) && k.RType == PSRECTYPE_DAY && k.Loc == loc_idx && k.GoodsID == goodsID) {
			ExpandDate(data.Dt, &dt);
			p.low = (dt < p.low || p.low == ZERODATE) ? dt : p.low;
		}
		SetKey(&k, PSRECTYPE_DAY, loc_id, goodsID, ZERODATE);
		k.Dt = MAXSHORT;
		if(search(0, &k, spLe) && k.RType == PSRECTYPE_DAY && k.Loc == loc_idx && k.GoodsID == goodsID) {
			ExpandDate(data.Dt, &(dt = ZERODATE));
			p.upp = (dt > p.upp) ? dt : p.upp;
		}
	}
	ASSIGN_PTR(pPeriod, p);
	return (p.low && p.upp) ? 1 : -1;
}

int SLAPI PredictSalesCore::GetPeriod(const ObjIdListFilt * pLocList, const PPIDArray * pGoodsList, DateRange * pPeriod)
{
	int    ok = -1;
	DateRange period;
	period.SetZero();
	for(uint i = 0; i < pGoodsList->getCount(); i++) {
		DateRange p;
		if(GetPeriod(pLocList, pGoodsList->at(i), &p) > 0) {
			if(!period.low || (p.low && period.low > p.low))
				period.low = p.low;
			SETMAX(period.upp, p.upp);
			ok = 1;
		}
	}
	ASSIGN_PTR(pPeriod, period);
	return ok;
}

int SLAPI PredictSalesCore::GetTblUpdateDt(LDATE * pDt)
{
	int    ok = 1;
	LDATE  dt = ZERODATE;
	PredictSalesTbl::Key0 k0;
	MEMSZERO(k0);
	k0.RType = PSRECTYPE_LAST_DAY;
	if(search(0, &k0, spGe) && k0.RType == PSRECTYPE_LAST_DAY)
		ExpandDate(data.Dt, &dt);
	else
		ok = PPDbSearchError();
	ASSIGN_PTR(pDt, dt);
	return ok;
}

int SLAPI PredictSalesCore::SetTblUpdateDt(LDATE dt)
{
	int    ok = 1;
	PredictSalesTbl::Key0 k0;
	MEMSZERO(k0);
	k0.RType = PSRECTYPE_LAST_DAY;
	if(searchForUpdate(0, &k0, spGe) && k0.RType == PSRECTYPE_LAST_DAY) {
		ShrinkDate(dt, &data.Dt);
		THROW_DB(updateRec()); // @sfu
	}
	else
		THROW(AddItem(PSRECTYPE_LAST_DAY, 0, 0, dt, 0, 0, 0));
	CATCHZOK
	return ok;
}

int SLAPI PredictSalesCore::Helper_Enumerate(PPID goodsID, PPID locID,
	const DateRange * pPeriod, int maxItems, EnumPredictSalesProc proc, long extraData)
{
	int16  loc_idx = 0;
	int16  low = 0, upp = MAXSHORT;
	int    dir = 0; // 0 - forward, 1 - backward
	if(maxItems)
		if(maxItems > 0)
			dir = 0; // forward
		else
			dir = 1; // backward
	PredictSalesTbl::Key0 k;
	ShrinkLoc(locID, &loc_idx);
	BExtQuery q(this, 0, maxItems ? abs(maxItems) : 256);
	DBQ * dbq = & (this->RType == (long)PSRECTYPE_DAY && this->Loc == (long)loc_idx && this->GoodsID == goodsID);
	if(pPeriod) {
		ShrinkDate(pPeriod->low, &low);
		if(pPeriod->upp)
			ShrinkDate(pPeriod->upp, &upp);
		else
			upp = MAXSHORT;
		dbq = &(*dbq && this->Dt >= (long)low && this->Dt <= (long)upp);
	}
	q.select(this->Dt, this->Quantity, this->Amount, 0L).where(*dbq);
	SetKey(&k, PSRECTYPE_DAY, locID, goodsID, dir ? MAXDATE : ZERODATE);
	k.Dt = dir ? upp : low;
	long c = 0;
	for(q.initIteration(dir, &k, dir ? spLe : spGe); q.nextIteration() > 0;) {
		PredictSalesItem item;
		MEMSZERO(item);
		ExpandDate(data.Dt, &item.Dt);
		item.Qtty = data.Quantity;
		item.Amount = data.Amount;
		int    r = proc(&item, extraData);
		if(r <= 0)
			return r;
		c++;
		if(maxItems != 0 && c >= abs(maxItems))
			break;
	}
	return 1;
}

int SLAPI PredictSalesCore::Enumerate(PPID goodsID, const ObjIdListFilt & rLocList,
	const DateRange * pPeriod, int maxItems, EnumPredictSalesProc proc, long extraData)
{
	if(rLocList.GetCount() == 1) {
		return Helper_Enumerate(goodsID, rLocList.Get().get(0), pPeriod, maxItems, proc, extraData);
	}
	else {
		PPIDArray loc_list;
		if(rLocList.IsExists())
			rLocList.CopyTo(&loc_list);
		else
			loc_list.add(-1);
		if(loc_list.getSingle() == -1 || loc_list.getCount() > 1) {
			LocTabEntry * p_entry;
			if(P_LocTab)
				if(loc_list.getSingle() == -1) {
					for(uint i = 0; P_LocTab->enumItems(&i, (void **)&p_entry);)
						if(p_entry->LocID >= 0) {
							int r = Helper_Enumerate(goodsID, p_entry->LocID, pPeriod, maxItems, proc, extraData);
							if(r <= 0)
								return r;
						}
				}
				else {
					for(uint i = 0; i < loc_list.getCount(); i++)
						if(loc_list.at(i) > 0) {
							int r = Helper_Enumerate(goodsID, loc_list.at(i), pPeriod, maxItems, proc, extraData);
							if(r <= 0)
								return r;
						}
				}
		}
	}
	return 1;
}

static int FASTCALL EnumProc_GetStat(PredictSalesItem * pItem, long extraData)
{
	((PredictSalesStat *)extraData)->Step(pItem);
	return 1;
}

int SLAPI PredictSalesCore::CalcStat(PPID goodsID, const ObjIdListFilt & rLocList,
	const DateRange * pPeriod, int maxItems, PredictSalesStat * pStat)
{
	int    lsslin = BIN(pStat->Flags & PSSF_USELSSLIN);
	pStat->Init();
	SETFLAG(pStat->Flags, PSSF_USELSSLIN, lsslin);
	pStat->LocID = rLocList.GetSingle();
	pStat->GoodsID = goodsID;
	int    ok = Enumerate(goodsID, rLocList, pPeriod, maxItems, EnumProc_GetStat, (long)pStat);
	if(ok > 0)
		pStat->Finish();
	return ok;
}
//
//
//
SLAPI PsiArray::PsiArray() : TSArray <PredictSalesItem>()
{
	setDelta(16);
}

int SLAPI PsiArray::Add(const PredictSalesItem * pItem)
{
	uint   pos = 0;
	if(bsearch(&pItem->Dt, &pos, CMPF_LONG)) {
		PredictSalesItem & r_item = at(pos);
		r_item.Qtty   += pItem->Qtty;
		r_item.Amount += pItem->Amount;
		return 1;
	}
	else
		return ordInsert(pItem, 0, CMPF_LONG) ? 1 : PPSetErrorSLib();
}

int SLAPI PsiArray::ShrinkByCycleList(const PPCycleArray * pCycleList, PsiArray * pList) const
{
	PredictSalesItem * p_item;
	pList->freeAll();
	for(uint i = 0; enumItems(&i, (void **)&p_item);) {
		uint   pos = 0;
		if(pCycleList->searchDate(p_item->Dt, &pos)) {
			PredictSalesItem new_item = *p_item;
			new_item.Dt = pCycleList->at(pos).low;
			pList->Add(&new_item);
		}
	}
	return 1;
}

static int FASTCALL EnumProc_GetStatByGrp(PredictSalesItem * pItem, long extraData)
{
	return extraData ? ((PsiArray *)extraData)->Add(pItem) : -1;
}

int SLAPI PredictSalesCore::GetSeries(const PPIDArray & rGoodsIDList, const ObjIdListFilt & rLocList,
	const DateRange * pPeriod, PsiArray * pList)
{
	if(pList)
		for(uint i = 0; i < rGoodsIDList.getCount(); i++)
			Enumerate(rGoodsIDList.get(i), rLocList, pPeriod, 0, EnumProc_GetStatByGrp, (long)pList);
	return 1;
}

int SLAPI PredictSalesCore::CalcStat(const PPIDArray & rGoodsIdList, ObjIdListFilt locList,
	const DateRange * pPeriod, PredictSalesStat * pStat)
{
	int    ok = 1;
	pStat->Init();
	pStat->LocID = locList.GetSingle();
	pStat->GoodsID = 0;
	PsiArray list;
	if(GetSeries(rGoodsIdList, locList, pPeriod, &list))
		pStat->Step(&list);
	pStat->Finish();
	return ok;
}

int SLAPI PredictSalesCore::SearchStat(PPID goodsID, PPID locID, GoodsStatTbl::Rec * pRec)
{
	int    ok = -1;
	GoodsStatTbl::Key0 k0;
	k0.GoodsID = goodsID;
	ShrinkLoc(locID, &k0.Loc);
	return SearchByKey(&StT, 0, &k0, pRec);
}

int SLAPI PredictSalesCore::GetLastUpdate(PPID goodsID, const ObjIdListFilt & rLocList, LDATE * pLastDate)
{
	int    ok = -1;
	LDATE  last_date = MAXDATE;
	GoodsStatTbl::Rec rec;
	PPID   single_loc_id = rLocList.GetSingle();
	if(single_loc_id) {
		ok = SearchStat(goodsID, single_loc_id, &rec);
		if(ok > 0)
			ExpandDate(rec.LastDate, &last_date);
		else
			last_date = ZERODATE;
	}
	else {
		GoodsStatTbl::Key1 k1;
		k1.GoodsID = goodsID;
		k1.Loc = 0;
		BExtQuery q(&StT, 1);
		q.select(StT.Loc, StT.LastDate, 0L).where(StT.GoodsID == goodsID);
		for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
			StT.copyBufTo(&rec);
			PPID   loc_id = 0;
			ExpandLoc(rec.Loc, &loc_id);
			if(rLocList.CheckID(loc_id)) {
				//
				// Для агрегирующих записей за дату принимаем минимальное из всех агрегируемых
				// записей значение.
				// Причина: будем считать, что та статистика, которая "убежала" вперед
				//   других менее достоверна (скорее всего был обрыв обработки либо товар
				//   был обсчитан в одиночку).
				//
				LDATE  dt;
				ExpandDate(rec.LastDate, &dt);
				SETMIN(last_date, dt);
				ok = 1;
			}
		}
		if(ok < 0)
			last_date = ZERODATE;
	}
	ASSIGN_PTR(pLastDate, last_date);
	return ok;
}

int SLAPI PredictSalesCore::SearchStat(PPID goodsID, const ObjIdListFilt & rLocList, GoodsStatTbl::Rec * pRec)
{
	int    ok = -1;
	GoodsStatTbl::Rec rec, temp_rec;
	MEMSZERO(rec);
	rec.GoodsID = goodsID;
	if(rLocList.GetSingle()) {
		ok = SearchStat(goodsID, rLocList.GetSingle(), &rec);
	}
	else {
		GoodsStatTbl::Key1 k1;
		k1.GoodsID = goodsID;
		k1.Loc = 0;
		BExtQuery q(&StT, 1);
		q.selectAll().where(StT.GoodsID == goodsID);
		for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
			StT.copyBufTo(&temp_rec);
			PPID   loc_id = 0;
			ExpandLoc(temp_rec.Loc, &loc_id);
			if(rLocList.CheckID(loc_id)) {
				//
				// Если результирующая запись является агрегирующий, то Loc в ней будет нулевой
				//
				rec.Loc = rec.Loc ? 0 : temp_rec.Loc;
				//
				// Для агрегирующих записей за дату принимаем минимальное из всех агрегируемых
				// записей значение.
				// Причина: будем считать, что та статистика, которая "убежала" вперед
				//   других менее достоверна (скорее всего был обрыв обработки либо товар
				//   был обсчитан в одиночку).
				//
				rec.LastDate   = rec.LastDate ? MIN(rec.LastDate, temp_rec.LastDate) : 0;
				rec.QttySum   += temp_rec.QttySum;
				rec.QttySqSum += temp_rec.QttySqSum;
				rec.AmtSum    += temp_rec.AmtSum;
				rec.AmtSqSum  += temp_rec.AmtSqSum;
				rec.Count     += temp_rec.Count;
				ok = 1;
			}
		}
	}
	if(ok > 0)
		ASSIGN_PTR(pRec, rec);
	return ok;
}

int SLAPI PredictSalesCore::GetStat(PPID goodsID, const ObjIdListFilt & rLocList, PredictSalesStat * pStat)
{
	int    ok = 1;
	GoodsStatTbl::Rec rec;
	if((ok = SearchStat(goodsID, rLocList, &rec)) > 0) {
		ExpandLoc(rec.Loc, &pStat->LocID);
		pStat->GoodsID   = rec.GoodsID;
		ExpandDate(rec.LastDate, &pStat->LastDate);
		pStat->Count     = rec.Count;
		pStat->QttySum   = rec.QttySum;
		pStat->QttySqSum = rec.QttySqSum;
		pStat->AmtSum    = rec.AmtSum;
		pStat->AmtSqSum  = rec.AmtSqSum;
	}
	return ok;
}

int SLAPI PredictSalesCore::ClearAll()
{
	int    ok = 1;
	THROW_DB(CurDict->RenewFile(*this, 0, 0));
	THROW_DB(CurDict->RenewFile(StT, 0, 0));
	//
	// Информация о выходных днях и таблица складов остались
	// в памяти. Их необходимо будет сохранить вызовом функции Finish().
	//
	IsHldTabUpdated = 1;
	IsLocTabUpdated = 1;
	CATCHZOK
	return ok;
}

int SLAPI PredictSalesCore::ClearByPeriod(DateRange period, PPID gGrpID, int use_ta)
{
	int    ok = -1;
	if(!period.IsZero()) {
		int16  low_dt = 0, upp_dt = 0;
		PredictSalesTbl::Key0 k;
		PPObjGoods goods_obj;
		BExtQuery q(this, 0, 64);
		DBQ * dbq = 0;
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			MEMSZERO(k);
			ShrinkDate(period.low, &low_dt);
			ShrinkDate(period.upp, &upp_dt);
			q.selectAll().where(Dt >= (long)low_dt && Dt <= (long)upp_dt);
			for(q.initIteration(0, &k, spFirst); q.nextIteration() > 0;) {
				if(data.Dt >= low_dt && data.Dt <= upp_dt) { // Перестраховка
					if(!gGrpID || goods_obj.P_Tbl->BelongToGroup(data.GoodsID, gGrpID)) {
						DBRowId pos;
						THROW_DB(q.getRecPosition(&pos));
						THROW_DB(getDirect(-1, 0, pos));
						THROW_DB(deleteRec());
						ok = 1;
					}
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}
