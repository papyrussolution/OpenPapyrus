// RENT.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2011, 2014, 2015, 2016, 2017, 2018, 2020, 2021, 2022
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

int RentChrgDialog(RentChrgFilt *); // Prototype
//
//
//
double CalcPercent(LDATE beg, LDATE end, double rest, double percent)
{
	long   num_days = _diffdate(&end, &beg, DF_BTRIEVE, 1);
	double result = fdiv100r(rest * num_days * percent) / 360.0;
	return result;
}
//
//
//
RentChrgFilt::RentChrgFilt() : CntrgntID(0)
{
	Period.Z();
}
//
//
//
PctChargeArray::PctChargeArray() : SVector(sizeof(PctChargeEntry)) // @v9.8.12 SArray-->SVector
{
}

PctChargeEntry & FASTCALL PctChargeArray::at(uint i) const
{
	return *static_cast<PctChargeEntry *>(SVector::at(i));
}

LDATE PctChargeArray::GetFirstDate() const
{
	return getCount() ? at(0).Dt : ZERODATE;
}

int PctChargeArray::EnumItems(uint * pIdx, PctChargeEntry * pEntry) const
{
	if(*pIdx < getCount()) {
		ASSIGN_PTR(pEntry, at(*pIdx));
		(*pIdx)++;
		return 1;
	}
	else
		return 0;
}

int PctChargeArray::Add(LDATE dt, double amount)
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

double PctChargeArray::GetRest(LDATE dt)
{
	for(int i = static_cast<int>(getCount())-1; i >= 0; i--)
		if(at(i).Dt <= dt)
			return at(i).Rest;
	return 0;
}
//
// since - дата начала действия расчета
// end   - дата окончания действия расчета (точка начисления)
// Расчет ведется исходя из того, что в году 360 дней.
// За первый день проценты не начисляются. Другими словами, если между since и end
// остаток не менялся, то проценты начисляются за (end - since) дней.
//
int PctChargeArray::ChargePercent(LDATE since, LDATE end, double percent, double * pResult) const
{
	LDATE  beg = since;
	double rest = 0.0, result = 0.0;
	PctChargeEntry entry;
	for(uint i = 0; EnumItems(&i, &entry) > 0;) {
		if(entry.Dt > since) {
			if(entry.Dt <= end) {
				double part = CalcPercent(beg, entry.Dt, rest, percent);
				result += part;
			}
			else if(i > 1 && end > at(i-2).Dt) {
				//
				// Если точка начисления находится между двумя операциями
				// по кредиту, то начисляем за период, прошедший с даты beg до точки начисления.
				//
				double part = CalcPercent(beg, end, rest, percent);
				result += part;
			}
			beg = entry.Dt;
		}
		rest = entry.Rest;
	}
	if(end > beg) {
		double part = CalcPercent(beg, end, rest, percent);
		result += part;
	}
	ASSIGN_PTR(pResult, result);
	return 1;
}
//
//
//
#if 0 // {
static int RentPeriodToStr(short prd, short numprds, char * buf, size_t buflen)
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

static int StrToRentPeriod(const char * pBuf, short * pPeriod, short * pNumPeriods)
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
		numprds = satoi(number);
	}
	else
		numprds = 0;
	l = ToUpper866(*p);
	switch(l) {
		case 'D': case '„': prd = PRD_DAY;    break;
		case 'W': case 'Ќ': prd = PRD_WEEK;   break;
		case 'M': case 'Њ': prd = PRD_MONTH;  break;
		case 'Q': case 'Љ': prd = PRD_QUART;  break;
		case 'S': case 'Џ': prd = PRD_SEMIAN; break;
		case 'A': case 'ѓ': prd = PRD_ANNUAL; break;
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

int PPRentCondition::IsEmpty() const
{
	return (!Period.IsZero() || Cycle || Percent || PartAmount) ? 0 : 1;
}

int FASTCALL PPRentCondition::IsEq(const PPRentCondition & rS) const
{
	if(!Period.IsEq(rS.Period))
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

int PPRentCondition::GetChargeDate(const PPCycleArray * pList, uint cycleNo, LDATE * pDt) const
{
	DateRange period;
	if(pList->getPeriod(cycleNo, &period)) {
		*pDt = plusdate(period.upp, ChargeDayOffs);
		return 1;
	}
	else
		return 0;
}

int PPRentCondition::CalcRent(LDATE /*chargeDt*/, double * pAmount) const
{
	ASSIGN_PTR(pAmount, PartAmount);
	return 1;
}

double PPRentCondition::CalcPercent(LDATE begDt, LDATE chargeDt, const PctChargeArray * pCreditList) const
{
	double result = 0.0;
	pCreditList->ChargePercent(begDt, chargeDt, Percent, &result);
	return result;
}
//
//
//
int PPObjBill::AutoCharge(PPID billID, PPID opID, const PPRentCondition * pRc, const DateRange * pPeriod, PPLogger * pLogger)
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
			if(pRc->Flags & PPRentCondition::fPercent) {
				LDATE  last_chrg_dt = ZERODATE;
				PctChargeArray credit_list;
				THROW(P_Tbl->GetCreditList(billID, &credit_list));
				//
				// Находим дату последнего начисления, предшествующую дате текущего начисления //
				//
				for(j = 0; j < charges.getCount(); j++) {
					long   ldt = charges.at(j).Key;
					if(ldt < (long)charge_dt)
						last_chrg_dt.v = ldt;
					else
						break;
				}
				//
				// Если до charge_dt начислений не было, то за начальную дату принимает
				// дату первой операции по кредиту
				//
				SETIFZ(last_chrg_dt, credit_list.GetFirstDate());
				amount = pRc->CalcPercent(last_chrg_dt, charge_dt, &credit_list);
				r = 1;
			}
			else {
				r = pRc->CalcRent(charge_dt, &amount);
			}
			if(r > 0 && amount != 0.0) {
				PPBillPacket pack;
				THROW(pack.CreateBlank(opID, billID, 0, 1));
				pack.Rec.Dt = charge_dt;
				pack.Rec.Amount = BR2(amount);
				pack.Amounts.Put(PPAMT_MAIN, 0L/*@curID*/, amount, 0, 1);
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

int PPObjBill::AutoCharge(PPID id)
{
	int    ok = -1;
	PPLogger logger;
	BillTbl::Rec bill_rec;
	DateRange period;
	PPRentCondition rc;
	period.Z();
	if(P_Tbl->GetRentCondition(id, &rc) > 0 && rc.Cycle && !(rc.Flags & PPRentCondition::fClosed) && Search(id, &bill_rec) > 0) {
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

int PPObjBill::AutoCharge()
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   parent_op = 0;
	PPID   charge_op = 0;
	PPLogger logger;
	PropertyTbl::Key0 k;
	RentChrgFilt filt;
	// @v10.8.5 @ctr MEMSZERO(filt);
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
		if(rc.Cycle && !(rc.Flags & PPRentCondition::fClosed) && Search(id) > 0 && (!filt.CntrgntID || P_Tbl->data.Object == filt.CntrgntID)) {
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
int RentChrgDialog(RentChrgFilt * pFlt) 
{ 
	class RentChrgDlg : public TDialog {
		DECL_DIALOG_DATA(RentChrgFilt);
	public:
		RentChrgDlg() : TDialog(DLG_RENTCHARGE)
		{
			SetupCalPeriod(CTLCAL_RENTCHARGE_PERIOD, CTL_RENTCHARGE_PERIOD);
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			SetPeriodInput(this, CTL_RENTCHARGE_PERIOD, &Data.Period);
			SetupArCombo(this, CTLSEL_RENTCHARGE_OBJECT, Data.CntrgntID, OLW_LOADDEFONOPEN, GetSellAccSheet(), sacfNonGeneric);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			if(!GetPeriodInput(this, CTL_RENTCHARGE_PERIOD, &Data.Period))
				ok = PPErrorByDialog(this, CTL_RENTCHARGE_PERIOD);
			else {
				getCtrlData(CTLSEL_RENTCHARGE_OBJECT, &Data.CntrgntID);
				ASSIGN_PTR(pData, Data);
			}
			return ok;
		}
	};
	DIALOG_PROC_BODY(RentChrgDlg, pFlt); 
}
