// RENT.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2011, 2014, 2015, 2016, 2017, 2018
//
#include <pp.h>
#pragma hdrstop

// Prototype
int SLAPI RentChrgDialog(RentChrgFilt *);
//
//
//
int SLAPI CalcPercent(LDATE beg, LDATE end, double rest, double percent, double * pResult)
{
	long   num_days = _diffdate(&end, &beg, DF_BTRIEVE, 1);
	double result = fdiv100r(rest * num_days * percent) / 360.0;
	ASSIGN_PTR(pResult, result);
	return 1;
}
//
//
//
SLAPI PctChargeArray::PctChargeArray() : SVector(sizeof(PctChargeEntry)) // @v9.8.12 SArray-->SVector
{
}

PctChargeEntry & FASTCALL PctChargeArray::at(uint i) const
{
	return *(PctChargeEntry *)SVector::at(i);
}

LDATE SLAPI PctChargeArray::GetFirstDate() const
{
	return getCount() ? at(0).Dt : ZERODATE;
}

int SLAPI PctChargeArray::EnumItems(uint * pIdx, PctChargeEntry * pEntry) const
{
	if(*pIdx < getCount()) {
		ASSIGN_PTR(pEntry, at(*pIdx));
		(*pIdx)++;
		return 1;
	}
	else
		return 0;
}

int SLAPI PctChargeArray::Add(LDATE dt, double amount)
{
	double rest = GetRest(dt);
	uint   pos = 0;
	if(SVector::bsearch(&dt, &pos, CMPF_LONG)) {
		at(pos).Rest += amount;
		return 1;
	}
	else {
		PctChargeEntry entry;
		entry.Dt = dt;
		entry.Rest = rest + amount;
		return SVector::ordInsert(&entry, 0, CMPF_LONG) ? 1 : PPSetErrorSLib();
	}
}

double SLAPI PctChargeArray::GetRest(LDATE dt)
{
	for(int i = static_cast<int>(getCount())-1; i >= 0; i--)
		if(at(i).Dt <= dt)
			return at(i).Rest;
	return 0;
}
//
// since - дата начала действи€ расчета
// end   - дата окончани€ действи€ расчета (точка начислени€)
// –асчет ведетс€ исход€ из того, что в году 360 дней.
// «а первый день проценты не начисл€ютс€. ƒругими словами, если между since и end
// остаток не мен€лс€, то проценты начисл€ютс€ за (end - since) дней.
//
int SLAPI PctChargeArray::ChargePercent(LDATE since, LDATE end, double percent, double * pResult) const
{
	LDATE  beg = since;
	double rest = 0.0, result = 0.0;
	PctChargeEntry entry;
	for(uint i = 0; EnumItems(&i, &entry) > 0;) {
		if(entry.Dt > since) {
			if(entry.Dt <= end) {
				double part = 0.0;
				CalcPercent(beg, entry.Dt, rest, percent, &part);
				result += part;
			}
			else if(i > 1 && end > at(i-2).Dt) {
				//
				// ≈сли точка начислени€ находитс€ между двум€ операци€ми
				// по кредиту, то начисл€ем за период, прошедший с даты beg
				// до точки начислени€.
				//
				double part = 0.0;
				CalcPercent(beg, end, rest, percent, &part);
				result += part;
			}
			beg = entry.Dt;
		}
		rest = entry.Rest;
	}
	if(end > beg) {
		double part = 0.0;
		CalcPercent(beg, end, rest, percent, &part);
		result += part;
	}
	ASSIGN_PTR(pResult, result);
	return 1;
}
//
//
//
#if 0 // {

static int SLAPI RentPeriodToStr(short prd, short numprds, char * buf, size_t buflen)
{
	char   temp[64];
	char   l = 0;
	char * p = temp;
	if(numprds) {
		itoa(numprds, p, 10);
		p += sstrlen(p);
		switch(prd) {
			case PRD_DAY:    l = 'D'; break;
			case PRD_WEEK:   l = 'W'; break;
			case PRD_MONTH:  l = 'M'; break;
			case PRD_QUART:  l = 'Q'; break;
			case PRD_SEMIAN: l = 'S'; break;
			case PRD_ANNUAL: l = 'A'; break;
			default: l = 0; break;
		}
		*p++ = l;
	}
	*p = 0;
	strnzcpy(buf, temp, buflen);
	return 1;
}

static int SLAPI StrToRentPeriod(const char * pBuf, short * pPeriod, short * pNumPeriods)
{
	char   temp[64], number[32], l = 0;
	short  prd = 0, numprds = 0;
	char * p  = strip(STRNSCPY(temp, pBuf));
	char * np = number;
	if(isdec(*p)) {
		do {
			*np++ = *p++;
		} while(isdec(*p));
		*np = 0;
		numprds = atoi(number);
	}
	else
		numprds = 0;
	l = ToUpper866(*p);
	switch(l) {
		case 'D': case 'Д': prd = PRD_DAY;    break;
		case 'W': case 'Н': prd = PRD_WEEK;   break;
		case 'M': case 'М': prd = PRD_MONTH;  break;
		case 'Q': case 'К': prd = PRD_QUART;  break;
		case 'S': case 'П': prd = PRD_SEMIAN; break;
		case 'A': case 'Г': prd = PRD_ANNUAL; break;
		case 0: prd = numprds ? PRD_MONTH : 0; break;
		default: prd = PRD_UNDEF; break;
	}
	if(prd != 0 && prd != PRD_UNDEF && numprds == 0)
		numprds = 1;
	ASSIGN_PTR(pPeriod, prd);
	ASSIGN_PTR(pNumPeriods, numprds);
	return (prd == PRD_UNDEF) ? (PPSetError(PPERR_UNDEFRENTPERIOD), -1) : 1;
}

#endif // } 0

int SLAPI PPRentCondition::IsEmpty() const
{
	return (!Period.IsZero() || Cycle || Percent || PartAmount) ? 0 : 1;
}

int FASTCALL PPRentCondition::IsEqual(const PPRentCondition & rS) const
{
	if(!Period.IsEqual(rS.Period))
		return 0;
	else if(Cycle != rS.Cycle)
		return 0;
	else if(Percent != rS.Percent)
		return 0;
	else if(PartAmount != rS.PartAmount)
		return 0;
	else if(Flags != rS.Flags)
		return 0;
	else if(ChargeDayOffs != rS.ChargeDayOffs)
		return 0;
	else
		return 1;
}

int SLAPI PPRentCondition::GetChargeDate(const PPCycleArray * pList, uint cycleNo, LDATE * pDt) const
{
	DateRange period;
	if(pList->getPeriod(cycleNo, &period)) {
		*pDt = plusdate(period.upp, ChargeDayOffs);
		return 1;
	}
	else
		return 0;
}

int SLAPI PPRentCondition::CalcRent(LDATE /*chargeDt*/, double * pAmount) const
{
	ASSIGN_PTR(pAmount, PartAmount);
	return 1;
}

int SLAPI PPRentCondition::CalcPercent(LDATE begDt, LDATE chargeDt,
	const PctChargeArray * pCreditList, double * pAmount) const
{
	pCreditList->ChargePercent(begDt, chargeDt, Percent, pAmount);
	return 1;
}
//
//
//
int SLAPI PPObjBill::AutoCharge(PPID billID, PPID opID, const PPRentCondition * pRc, const DateRange * pPeriod, PPLogger * pLogger)
{
	int    ok = 1;
	uint   i, j;
	DateIter di;
	PPCycleArray cycle_list;
	LAssocArray charges;
	THROW_PP(GetOpType(opID) == PPOPT_CHARGE, PPERR_INVCHARGEOP);
	while(P_Tbl->EnumLinks(billID, &di, BLNK_CHARGE) > 0)
		THROW_SL(charges.Add(P_Tbl->data.Dt, P_Tbl->data.ID, 0));
	{
		DateRange enum_period = pRc->Period;
		SETIFZ(enum_period.upp, pPeriod->upp);
		cycle_list.init(&enum_period, pRc->Cycle, 0);
	}
	for(i = 0; i < cycle_list.getCount(); i++) {
		LDATE  charge_dt;
		pRc->GetChargeDate(&cycle_list, i, &charge_dt);
		if(pPeriod->CheckDate(charge_dt) && !charges.Search(charge_dt, 0, 0)) {
			int    r = 0;
			double amount = 0.0;
			if(pRc->Flags & RENTF_PERCENT) {
				LDATE  last_chrg_dt = ZERODATE;
				PctChargeArray credit_list;
				THROW(P_Tbl->GetCreditList(billID, &credit_list));
				//
				// Ќаходим дату последнего начислени€, предшествующую дате текущего начислени€ //
				//
				for(j = 0; j < charges.getCount(); j++) {
					long   ldt = charges.at(j).Key;
					if(ldt < (long)charge_dt)
						last_chrg_dt.v = ldt;
					else
						break;
				}
				//
				// ≈сли до charge_dt начислений не было, то за начальную дату принимает
				// дату первой операции по кредиту
				//
				SETIFZ(last_chrg_dt, credit_list.GetFirstDate());
				r = pRc->CalcPercent(last_chrg_dt, charge_dt, &credit_list, &amount);
			}
			else {
				r = pRc->CalcRent(charge_dt, &amount);
			}
			if(r > 0 && amount != 0.0) {
				PPBillPacket pack;
				THROW(pack.CreateBlank(opID, billID, 0, 1));
				pack.Rec.Dt = charge_dt;
				pack.Rec.Amount = BR2(amount);
				pack.Amounts.Put(PPAMT_MAIN, 0L /* @curID */, amount, 0, 1);
				SubstMemo(&pack);
				THROW(FillTurnList(&pack));
				THROW(TurnPacket(&pack, 1));
				if(pLogger) {
					pLogger->LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
				}
				THROW_SL(charges.Add(pack.Rec.Dt, pack.Rec.ID, 0, 1));
				if(!pPeriod->upp)
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::AutoCharge(PPID id)
{
	int    ok = -1;
	PPLogger logger;
	BillTbl::Rec bill_rec;
	DateRange period;
	PPRentCondition rc;
	period.Z();
	if(P_Tbl->GetRentCondition(id, &rc) > 0 && rc.Cycle && !(rc.Flags & RENTF_CLOSED) && Search(id, &bill_rec) > 0) {
		if(bill_rec.OpID && DateRangeDialog(0, 0, &period) > 0) {
			PPOprKind op_rec;
			for(PPID temp_id = 0; ok < 0 && EnumOperations(PPOPT_CHARGE, &temp_id, &op_rec) > 0;)
				if(op_rec.LinkOpID == bill_rec.OpID) {
					THROW(AutoCharge(id, temp_id, &rc, &period, &logger));
					ok = 1;
				}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjBill::AutoCharge()
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   parent_op = 0;
	PPID   charge_op = 0;
	PPLogger logger;
	PropertyTbl::Key0 k;
	RentChrgFilt filt;
	MEMSZERO(filt);
	if(RentChrgDialog(&filt) <= 0)
		return -1;
	BExtQuery q(&p_ref->Prop, 0);
	q.select(p_ref->Prop.ObjID, p_ref->Prop.Text, 0L).
		where(p_ref->Prop.ObjType == PPOBJ_BILL && p_ref->Prop.Prop == BILLPRP_RENT);
	k.ObjType = PPOBJ_BILL;
	k.ObjID   = 0;
	k.Prop    = 0;
	for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
		PPRentCondition rc;
		memcpy(&rc, p_ref->Prop.data.Text, sizeof(PPRentCondition));
		PPID   id = p_ref->Prop.data.ObjID;
		if(rc.Cycle && !(rc.Flags & RENTF_CLOSED) && Search(id) > 0 && (!filt.CntrgntID || P_Tbl->data.Object == filt.CntrgntID)) {
			const PPID op = P_Tbl->data.OpID;
			if(op) {
				if(parent_op != op) {
					PPOprKind opk;
					parent_op = 0;
					charge_op = 0;
					for(PPID temp_id = 0; EnumOperations(PPOPT_CHARGE, &temp_id, &opk) > 0;)
						if(opk.LinkOpID == op) {
							parent_op = op;
							charge_op = temp_id;
							break;
						}
				}
				if(charge_op)
					THROW(AutoCharge(id, charge_op, &rc, &filt.Period, &logger));
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
class RentChrgDlg : public TDialog {
public:
	RentChrgDlg() : TDialog(DLG_RENTCHARGE)
	{
		SetupCalCtrl(CTLCAL_RENTCHARGE_PERIOD, this, CTL_RENTCHARGE_PERIOD, 1);
	}
	int    setDTS(const RentChrgFilt *);
	int    getDTS(RentChrgFilt *);
private:
	RentChrgFilt Data;
};

int RentChrgDlg::setDTS(const RentChrgFilt * pFilt)
{
	Data = *pFilt;
	SetPeriodInput(this, CTL_RENTCHARGE_PERIOD, &Data.Period);
	SetupArCombo(this, CTLSEL_RENTCHARGE_OBJECT, Data.CntrgntID, OLW_LOADDEFONOPEN, GetSellAccSheet(), sacfNonGeneric);
	return 1;
}

int RentChrgDlg::getDTS(RentChrgFilt * pFilt)
{
	int    ok = 1;
	if(!GetPeriodInput(this, CTL_RENTCHARGE_PERIOD, &Data.Period))
		ok = PPErrorByDialog(this, CTL_RENTCHARGE_PERIOD);
	else {
		getCtrlData(CTLSEL_RENTCHARGE_OBJECT, &Data.CntrgntID);
		*pFilt = Data;
	}
	return ok;
}

int SLAPI RentChrgDialog(RentChrgFilt * pFlt) { DIALOG_PROC_BODY(RentChrgDlg, pFlt); }
