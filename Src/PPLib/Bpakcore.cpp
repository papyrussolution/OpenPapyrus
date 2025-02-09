// BPAKCORE.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop

AmtEntry::AmtEntry() : AmtTypeID(0), CurID(0), Amt(0.0)
{
}

AmtEntry::AmtEntry(PPID amtTypeID) : AmtTypeID(amtTypeID), CurID(0), Amt(0.0)
{
}

AmtEntry::AmtEntry(PPID amtTypeID, PPID curID) : AmtTypeID(amtTypeID), CurID(curID), Amt(0.0)
{
}

AmtEntry::AmtEntry(PPID amtTypeID, PPID curID, double amt) : AmtTypeID(amtTypeID), CurID(curID), Amt(amt)
{
}

bool FASTCALL AmtEntry::IsEq(const AmtEntry & rS) const { return (AmtTypeID == rS.AmtTypeID && CurID == rS.CurID && Amt == rS.Amt); }

AmtList::AmtList() : TSVector <AmtEntry> ()
{
}

AmtList & FASTCALL AmtList::operator = (const AmtList & s)
{
	SVector::copy(s);
	return *this;
}

bool FASTCALL AmtList::HasAmtTypeID(PPID amtTypeID) const { return lsearch(&amtTypeID, 0, CMPF_LONG); }

bool FASTCALL AmtList::HasVatSum(const TaxAmountIDs * pTai) const
{
	bool   yes = false;
	if(pTai) {
		AmtEntry * p_ae;
		for(uint i = 0; !yes && enumItems(&i, (void **)&p_ae);)
			if(p_ae->AmtTypeID && p_ae->Amt != 0.0 && oneof3(p_ae->AmtTypeID, pTai->VatAmtID[0], pTai->VatAmtID[1], pTai->VatAmtID[2]))
				yes = true;
	}
	return yes;
}

bool FASTCALL AmtList::IsEq(const AmtList * pS) const
{
	bool   ok = (getCount() == pS->getCount());
	if(ok) {
		AmtEntry * p_e1, * p_e2;
		LongArray saw_list;
		for(uint i = 0; ok && enumItems(&i, (void **)&p_e1);) {
			int found = 0;
			for(uint j = 0; !found && pS->enumItems(&j, (void **)&p_e2);) {
				const long p = static_cast<long>(j);
				if(!saw_list.lsearch(p) && p_e1->IsEq(*p_e2)) {
					saw_list.add(p);
					found = 1;
				}
			}
			if(!found)
				ok = false;
		}
	}
	return ok;
}

bool AmtList::Search(PPID amtTypeID, PPID curID, uint * pPos) const
{
	for(uint i = 0; i < count; i++) {
		const AmtEntry & r_entry = at(i);
		if(r_entry.AmtTypeID == amtTypeID && r_entry.CurID == curID) {
			ASSIGN_PTR(pPos, i);
			return true;
		}
	}
	ASSIGN_PTR(pPos, 0);
	return false;
}

int AmtList::GetCurList(PPID amtTypeID, PPIDArray * pCurList) const
{
	int    ok = -1;
	if(pCurList)
		for(uint i = 0; ok && i < getCount(); i++)
			if(amtTypeID < 0 || at(i).AmtTypeID == amtTypeID)
				ok = pCurList->addUnique(at(i).CurID) ? 1 : 0;
	return ok;
}

int AmtList::GetAmtTypeList(PPIDArray * pList) const
{
	int    ok = -1;
	if(pList)
		for(uint i = 0; ok && i < getCount(); i++)
			ok = pList->addUnique(at(i).AmtTypeID) ? 1 : 0;
	return ok;
}

int AmtList::Get(PPID amtTypeID, PPID curID, double * pAmt) const
{
	int    ok = -1;
	uint   pos = 0;
	if(Search(amtTypeID, curID, &pos)) {
		ASSIGN_PTR(pAmt, at(pos).Amt);
		ok = 1;
	}
	else
		ASSIGN_PTR(pAmt, 0);
	return ok;
}

double AmtList::Get(PPID amtTypeID, PPID curID) const
{
	double v;
	Get(amtTypeID, curID, &v);
	return v;
}

int AmtList::Put(PPID amtTypeID, PPID curID, double amt, int ignoreZero, int replace)
{
	int    ok = 1;
	uint   pos = 0;
	amt = oneof2(amtTypeID, PPAMT_CRATE, PPAMT_TRANSITCRATE) ? R6(amt) : R2(amt);
	int    zero = BIN(ignoreZero && amt == 0.0);
	if(Search(amtTypeID, curID, &pos)) {
		if(replace)
			if(zero)
				atFree(pos);
			else
				at(pos).Amt = amt;
	}
	else if(!zero) {
		AmtEntry e(amtTypeID, curID, amt);
		ok = insert(&e) ? 1 : PPSetErrorSLib();
	}
	return ok;
}

int AmtList::Put(const AmtEntry * pEntry, int ignoreZero, int replace)
{
	return Put(pEntry->AmtTypeID, pEntry->CurID, pEntry->Amt, ignoreZero, replace);
}

int AmtList::Put(const AmtList * pList, int ignoreZero, int replace)
{
	for(uint i = 0; i < pList->getCount(); i++) {
		AmtEntry & e = pList->at(i);
		if(!Put(e.AmtTypeID, e.CurID, e.Amt, ignoreZero, replace))
			return 0;
	}
	return 1;
}

int AmtList::Add(PPID amtTypeID, PPID curID, double amt, int ignoreZero)
{
	AmtEntry ae(amtTypeID, curID, amt);
	return Add(&ae, ignoreZero);
}

int AmtList::Add(const AmtEntry * pEntry, int ignoreZero)
{
	uint pos = 0;
	if(Search(pEntry->AmtTypeID, pEntry->CurID, &pos)) {
		AmtEntry & r_entry = at(pos);
		r_entry.Amt += pEntry->Amt;
		return 1;
	}
	else
		return Put(pEntry, ignoreZero, 1);
}

int AmtList::Sub(const AmtEntry * pEntry, int ignoreZero)
{
	AmtEntry temp_entry = *pEntry;
	temp_entry.Amt = -temp_entry.Amt;
	return Add(&temp_entry, ignoreZero);
}

int AmtList::Add(const AmtList * pList, int ignoreZero)
{
	if(pList)
		for(uint i = 0; i < pList->getCount(); i++)
			if(!Add(&pList->at(i), ignoreZero))
				return 0;
	return 1;
}

int AmtList::Sub(const AmtList * pList, int ignoreZero)
{
	if(pList)
		for(uint i = 0; i < pList->getCount(); i++)
			if(!Sub(&pList->at(i), ignoreZero))
				return 0;
	return 1;
}

int AmtList::Remove(PPID amtTypeID, PPID curID)
{
	int    ok = -1;
	uint   i = count;
	if(i) do {
		const AmtEntry & r_entry = at(--i);
		if(r_entry.AmtTypeID == amtTypeID && (curID < 0 || r_entry.CurID == curID)) {
			atFree(i);
			ok = 1;
		}
	} while(i);
	return ok;
}
//
//
//
IMPL_CMPFUNC(PayPlanTblRec, i1, i2) { return CMPSIGN(static_cast<const PayPlanTbl::Rec *>(i1)->PayDate, static_cast<const PayPlanTbl::Rec *>(i2)->PayDate); }

PayPlanArray::PayPlanArray() : TSVector <PayPlanTbl::Rec>()
{
}

bool FASTCALL PayPlanArray::IsEq(const PayPlanArray & rS) const
{
	bool   ok = (getCount() == rS.getCount());
	if(ok) {
		PayPlanTbl::Rec * p_e1, * p_e2;
		LongArray saw_list;
		for(uint i = 0; ok && enumItems(&i, reinterpret_cast<void**>(&p_e1));) {
			int    found = 0;
			for(uint j = 0; !found && rS.enumItems(&j, reinterpret_cast<void**>(&p_e2));) {
				long   p = static_cast<long>(j);
				if(!saw_list.lsearch(p) && memcmp(p_e1, p_e2, sizeof(*p_e1)) == 0) {
					saw_list.add(p);
					found = 1;
				}
			}
			if(!found)
				ok = false;
		}
	}
	return ok;
}

int PayPlanArray::SearchDate(LDATE dt, uint * pPos, PayPlanTbl::Rec * pRec) const
{
	for(uint i = 0; i < getCount(); i++)
		if(at(i).PayDate == dt) {
			ASSIGN_PTR(pPos, i);
			ASSIGN_PTR(pRec, at(i));
			return 1;
		}
	return 0;
}

int PayPlanArray::GetLast(LDATE * pDt, double * pAmount, double * pInterest) const
{
	int    ok = 1;
	PayPlanTbl::Rec item;
	if(getCount())
		item = at(getCount()-1);
	else
		ok = -1;
	ASSIGN_PTR(pDt, item.PayDate);
	ASSIGN_PTR(pAmount, item.Amount);
	ASSIGN_PTR(pInterest, item.Interest);
	return ok;
}

void PayPlanArray::SetBillID(PPID billID)
{
	for(uint i = 0; i < getCount(); i++)
		at(i).BillID = billID;
}

int PayPlanArray::Update(const PayPlanTbl::Rec * pItem, uint * pPos)
{
	int    ok = 1;
	uint   pos = 0;
	LDATE  test_dt = pItem->PayDate;
	THROW_INVARG(pItem);
	THROW_SL(checkdate(&test_dt));
	THROW_PP(pItem->Amount > 0.0 || pItem->Interest > 0.0, PPERR_INVAMOUNT);
	if(SearchDate(pItem->PayDate, &pos, 0)) {
		at(pos).Amount   = pItem->Amount;
		at(pos).Interest = pItem->Interest;
		ok = 2;
	}
	else {
		THROW_SL(ordInsert(pItem, &pos, PTR_CMPFUNC(PayPlanTblRec)));
		ok = 1;
	}
	ASSIGN_PTR(pPos, pos);
	CATCHZOK
	return ok;
}

void PayPlanArray::Sort()
{
	SVector::sort(PTR_CMPFUNC(PayPlanTblRec));
}

static int GetDefaultPaymPeriod(const PPBillPacket * pPack, int * pDays)
{
	int    ok = -1;
	PPID   client_id = 0;
	int    paym_period = 1; // Срок оплаты по документу (в днях), взятый из соглашения //
	PPObjArticle arobj;
	ArticleTbl::Rec ar_rec;
	if(arobj.Fetch(pPack->Rec.Object, &ar_rec) > 0)
		client_id = ar_rec.ID;
	else
		MEMSZERO(ar_rec);
	const  int    agt_kind = PPObjArticle::GetAgreementKind(&ar_rec);
	if(agt_kind == 1) {
		int    is_agreement = 0;
		PPClientAgreement ca_rec;
		if(arobj.GetClientAgreement(client_id, ca_rec, 1) > 0)
			paym_period = ca_rec.DefPayPeriod;
	}
	else if(agt_kind == 2) {
		PPSupplAgreement sa_rec;
		if(arobj.GetSupplAgreement(client_id, &sa_rec, 1) > 0)
			paym_period = sa_rec.DefPayPeriod;
	}
	if(paym_period > 0)
		ok = 1;
	ASSIGN_PTR(pDays, paym_period);
	return ok;
}

int PayPlanArray::AutoBuild(const PPBillPacket * pPack)
{
	int    ok = -1;
	uint   i = 0;
	int    is_there_amount = 0;
	for(i = 1; i <= getCount(); i++) {
		if(at(i-1).Amount > 0) {
			is_there_amount = 1;
			at(i-1).Interest = 0;
		}
		else if(at(i-1).Interest > 0)
			atFree(--i);
	}
	if(!is_there_amount) {
		if(pPack->Rent.Cycle && pPack->Rent.Period.upp) {
			PPCycleArray cycle_list;
			cycle_list.init(&pPack->Rent.Period, pPack->Rent.Cycle, 0);
			if(cycle_list.getCount()) {
				double part  = R2(pPack->Rec.Amount / cycle_list.getCount());
				double total = 0.0;
				for(i = 0; i < cycle_list.getCount(); i++) {
					PayPlanTbl::Rec item;
					pPack->Rent.GetChargeDate(&cycle_list, i, &item.PayDate);
					if(i == cycle_list.getCount()-1) {
						//
						// Для того, чтобы общая сумма была равна номинальной сумме
						// документа, в последний платеж занесем остаток между номинальной
						// суммой и суммой всех предыдущих начислений.
						//
						item.Amount = pPack->Rec.Amount - total;
					}
					else
						item.Amount = part;
					total += item.Amount;
					THROW(Update(&item, 0));
					ok = 1;
				}
			}
		}
		else {
			int    paym_period = 0;
			if(GetDefaultPaymPeriod(pPack, &paym_period) > 0) {
				PayPlanTbl::Rec item;
				item.PayDate = plusdate(pPack->Rec.Dt, paym_period);
				item.Amount  = pPack->Rec.Amount;
				THROW(Update(&item, 0));
				ok = 1;
			}
		}
	}
	if(pPack->Rent.Flags & PPRentCondition::fPercent && pPack->Rent.Percent > 0 && pPack->Rent.Period.upp) {
		uint   j;
		DateIter di;
		PPCycleArray cycle_list;
		LAssocArray charges;
		cycle_list.init(&pPack->Rent.Period, pPack->Rent.Cycle, 0);
		for(i = 0; i < cycle_list.getCount(); i++) {
			LDATE  charge_dt;
			pPack->Rent.GetChargeDate(&cycle_list, i, &charge_dt);
			if(!charges.Search(charge_dt, 0)) {
				int    r = 0;
				double amount = 0.0;
				if(pPack->Rent.Flags & PPRentCondition::fPercent) {
					LDATE  last_chrg_dt = ZERODATE;
					PctChargeArray credit_list;
					THROW(credit_list.Add(pPack->Rec.Dt, pPack->Rec.Amount));
					for(j = 0; j < getCount(); j++) {
						PayPlanTbl::Rec & r_item = at(j);
						if(r_item.Amount > 0.0)
							THROW(credit_list.Add(r_item.PayDate, -r_item.Amount));
					}
					//
					// Находим дату последнего начисления, предшествующую дате текущего начисления //
					//
					for(j = 0; j < charges.getCount(); j++)
						if(charges.at(j).Key < (long)charge_dt)
							last_chrg_dt.v = charges.at(j).Key;
						else
							break;
					//
					// Если до charge_dt начислений не было, то за начальную дату принимает
					// дату первой операции по кредиту
					//
					SETIFZ(last_chrg_dt, credit_list.GetFirstDate());
					amount = pPack->Rent.CalcPercent(last_chrg_dt, charge_dt, &credit_list);
					r = 1;
				}
				else {
					r = pPack->Rent.CalcRent(charge_dt, &amount);
				}
				if(r > 0 && amount != 0) {
					uint pos = 0;
					PayPlanTbl::Rec item;
					SearchDate(charge_dt, 0, &item);
					item.PayDate  = charge_dt;
					item.Interest = BR2(amount);
					THROW(Update(&item, &pos));
					THROW_SL(charges.Add(item.PayDate, pos, 0, 1));
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
PPFreight::PPFreight()
{
	Z();
}

PPFreight & PPFreight::Z()
{
	THISZERO();
	return *this;
}

int FASTCALL PPFreight::CheckForFilt(const FreightFilt & rFilt) const
{
	int    ok = 1;
	if(!rFilt.ShipmPeriod.CheckDate(IssueDate))
		ok = 0;
	else if(!rFilt.ArrvlPeriod.CheckDate(ArrivalDate))
		ok = 0;
	else if(!CheckFiltID(rFilt.ShipID, ShipID))
		ok = 0;
	else if(!CheckFiltID(rFilt.CaptainID, CaptainID))
		ok = 0;
	else if(!CheckFiltID(rFilt.DlvrLocID, DlvrAddrID__))
		ok = 0;
	else if(rFilt.PortID || rFilt.PortOfLoading) {
		const int strict = BIN(rFilt.Flags & FreightFilt::fStrictPort);
		if(rFilt.PortID && !PortOfDischarge && (!DlvrAddrID__ || strict))
			ok = 0;
		else if(rFilt.PortOfLoading && !PortOfLoading)
			ok = 0;
		else {
			PPObjWorld w_obj;
			if(rFilt.PortID) {
				if(strict) {
					if(PortOfDischarge != rFilt.PortID)
						ok = 0;
				}
				else if(PortOfDischarge) {
					if(!w_obj.IsChildOf(PortOfDischarge, rFilt.PortID))
						ok = 0;
				}
				else {
					PPObjLocation loc_obj;
					LocationTbl::Rec loc_rec;
					if(loc_obj.Fetch(DlvrAddrID__, &loc_rec) > 0 && loc_rec.CityID) {
						if(!w_obj.IsChildOf(loc_rec.CityID, rFilt.PortID))
							ok = 0;
					}
					else
						ok = 0;
				}
			}
			if(ok) {
				if(strict) {
					if(PortOfLoading != rFilt.PortOfLoading)
						ok = 0;
				}
				else if(!w_obj.IsChildOf(PortOfLoading, rFilt.PortOfLoading))
					ok = 0;
				else if(PPObjTag::CheckForTagFilt(PPOBJ_BILL, ID, rFilt.P_TagF) <= 0)
					ok = 0;
			}
		}
	}
	return ok;
}

bool PPFreight::IsEmpty() const
{
	return (!ShipID && !AgentID && Name[0] == 0 && !PortOfDischarge &&
		!PortOfLoading && !CaptainID && !DlvrAddrID__ && !ArrivalDate && Cost == 0.0 && !StorageLocID && !Captain2ID);
}

bool FASTCALL PPFreight::IsEq(const PPFreight & s) const
{
	if(DlvrAddrID__ != s.DlvrAddrID__)
		return false;
	else if(NmbOrigsBsL != s.NmbOrigsBsL)
		return false;
	else if(TrType != s.TrType)
		return false;
	else if(PortOfLoading != s.PortOfLoading)
		return false;
	else if(PortOfDischarge != s.PortOfDischarge)
		return false;
	else if(IssueDate != s.IssueDate)
		return false;
	else if(ArrivalDate != s.ArrivalDate)
		return false;
	else if(CaptainID != s.CaptainID)
		return false;
	else if(Captain2ID != s.Captain2ID)
		return false;
	else if(R6(Cost - s.Cost) != 0)
		return false;
	else if(AgentID != s.AgentID)
		return false;
	else if(ShipID != s.ShipID)
		return false;
	else if(StorageLocID != s.StorageLocID)
		return false;
	else if(stricmp866(Name, s.Name) != 0)
		return false;
	else
		return true;
}

int PPFreight::SetupDlvrAddr(PPID dlvrAddrID)
{
	int    ok = -1;
	if(dlvrAddrID) {
		PPObjLocation loc_obj;
		LocationTbl::Rec loc_rec;
		THROW(loc_obj.Fetch(dlvrAddrID, &loc_rec) > 0);
		THROW_PP(oneof2(loc_rec.Type, LOCTYP_ADDRESS, LOCTYP_WAREHOUSE), PPERR_LOCMUSTBEADDRORWAREHOUSE);
		if(DlvrAddrID__ != dlvrAddrID) {
			DlvrAddrID__ = dlvrAddrID;
			ok = 1;
		}
		if(!PortOfDischarge) {
			const  PPID trunk_point_id = loc_obj.GetTrunkPointByDlvrAddr(dlvrAddrID);
			if(trunk_point_id) {
				PortOfDischarge = trunk_point_id;
				ok = 2;
			}
		}
	}
	else if(!DlvrAddrID__) {
		DlvrAddrID__ = 0;
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
//
//
PPBill::Agreement::Agreement()
{
	THISZERO();
}

PPBill::Agreement::Agreement(const Agreement & rS)
{
	Copy(rS);
}

PPBill::Agreement & FASTCALL PPBill::Agreement::operator = (const Agreement & rS)
{
	Copy(rS);
	return *this;
}

bool PPBill::Agreement::IsEmpty() const
{
	return !(Flags || Expiry || MaxCredit > 0.0 || MaxDscnt != 0.0 || Dscnt != 0.0 || DefAgentID ||
		DefQuotKindID || DefPayPeriod > 0 || RetLimPrd || RetLimPart || DefDlvrTerm > 0 || PctRet);
}

int FASTCALL PPBill::Agreement::Copy(const Agreement & rS)
{
	memcpy(this, &rS, sizeof(*this));
	return 1;
}

bool FASTCALL PPBill::Agreement::IsEq(const Agreement & rS) const
{
	#define CF(f) if(f != rS.f) return false
	CF(Flags);
	CF(Expiry);
	CF(MaxCredit);
	CF(MaxDscnt);
	CF(Dscnt);
	CF(DefAgentID);
	CF(DefQuotKindID);
	CF(PaymDateBase);
	CF(DefPayPeriod);
	CF(RetLimPrd);
	CF(RetLimPart);
	CF(DefDlvrTerm);
	CF(PctRet);
	#undef CF
	return true;
}

PPBill::PPBill() : P_PaymOrder(0), P_Freight(0), P_AdvRep(0), P_Agt(0), Ver(DS.GetVersion()), ObjTagContainerHelper(BTagL, PPOBJ_BILL, PPTAG_BILL_UUID)
{
}

PPBill::PPBill(const PPBill & rS) : P_PaymOrder(0), P_Freight(0), P_AdvRep(0), P_Agt(0), Ver(DS.GetVersion()), ObjTagContainerHelper(BTagL, PPOBJ_BILL, PPTAG_BILL_UUID) 
{
	Copy(rS);
}

void PPBill::BaseDestroy()
{
	Pays.clear();
	Amounts.clear();
	SMemo.Z(); // @v11.1.12
	MEMSZERO(Rec);
	MEMSZERO(Rent);
	MEMSZERO(Ext);
	ZDELETE(P_PaymOrder);
	ZDELETE(P_Freight);
	ZDELETE(P_AdvRep);
	ZDELETE(P_Agt);
	//
	Turns.clear();
	AdvList.Clear();
	LTagL.Release();
	XcL.Release();
	_VXcL.Release();
	BTagL.Z();
	Ver = DS.GetVersion();
}

int FASTCALL PPBill::IsEq(const PPBill & rS) const
{
	int    yes = 1;
	PPObjBill * p_bobj = BillObj;
	if(p_bobj) {
		if(!p_bobj->P_Tbl->GetFields().IsEqualRecords(&Rec, &rS.Rec))
			yes = 0;
	}
	else if(memcmp(&Rec, &rS.Rec, sizeof(Rec)) != 0)
		yes = 0;
	if(yes) {
		if(SMemo != rS.SMemo) // @v11.1.12
			yes = 0;
		else if(!Amounts.IsEq(&rS.Amounts))
			yes = 0;
		else if(!Pays.IsEq(rS.Pays))
			yes = 0;
		else if(!Ext.IsEq(rS.Ext))
			yes = 0;
		else if(!Rent.IsEq(rS.Rent))
			yes = 0;
		else if((P_PaymOrder && !rS.P_PaymOrder) || (!P_PaymOrder && rS.P_PaymOrder))
			yes = 0;
		else if(P_PaymOrder && rS.P_PaymOrder && memcmp(P_PaymOrder, rS.P_PaymOrder, sizeof(*P_PaymOrder)) != 0)
			yes = 0;
		else if((P_Freight && !rS.P_Freight) || (!P_Freight && rS.P_Freight))
			yes = 0;
		else if(P_Freight && rS.P_Freight && !P_Freight->IsEq(*rS.P_Freight))
			yes = 0;
		else if((P_Agt && !rS.P_Agt) || (!P_Agt && rS.P_Agt))
			yes = 0;
		else if(P_Agt && rS.P_Agt && !P_Agt->IsEq(*rS.P_Agt))
			yes = 0;
		else if((P_AdvRep && !rS.P_AdvRep) || (!P_AdvRep && rS.P_AdvRep))
			yes = 0;
		else if(P_AdvRep && rS.P_AdvRep && memcmp(P_AdvRep, rS.P_AdvRep, sizeof(*P_AdvRep)) != 0)
			yes = 0;
		else if(!AdvList.IsEq(rS.AdvList))
			yes = 0;
		else if(!BTagL.IsEq(rS.BTagL))
			yes = 0;
		else if(!XcL.IsEq(rS.XcL))
			yes = 0;
		else if(!_VXcL.IsEq(rS._VXcL))
			yes = 0;
		else {
			const uint c1 = Turns.getCount();
			const uint c2 = rS.Turns.getCount();
			if(c1 != c2)
				yes = 0;
			else {
				LongArray s2_fpos_list;
				for(uint i = 0; yes && i < c1; i++) {
					const  PPAccTurn & r_at1 = Turns.at(i);
					int    found = 0;
					for(uint j = 0; !found && j < c2; j++) {
						if(!s2_fpos_list.lsearch(j)) {
							const PPAccTurn & r_at2 = rS.Turns.at(j);
							if(r_at1.IsEq(r_at2)) {
								s2_fpos_list.add(j);
								found = 1;
							}
						}
					}
					if(!found)
						yes = 0;
				}
			}
		}
	}
	return yes;
}

int FASTCALL PPBill::Copy(const PPBill & rS)
{
	int    ok = 1;
	BaseDestroy();
	Rec = rS.Rec;
	Amounts = rS.Amounts;
	Pays = rS.Pays;
	Ext = rS.Ext;
	Rent = rS.Rent;
	if(rS.P_PaymOrder)
		P_PaymOrder = new PPBankingOrder(*rS.P_PaymOrder);
	else
		ZDELETE(P_PaymOrder);
	if(rS.P_Freight)
		P_Freight = new PPFreight(*rS.P_Freight);
	else
		ZDELETE(P_Freight);
	if(rS.P_AdvRep)
		P_AdvRep = new PPAdvanceRep(*rS.P_AdvRep);
	else
		ZDELETE(P_AdvRep);
	if(rS.P_Agt)
		P_Agt = new Agreement(*rS.P_Agt);
	else
		ZDELETE(P_Agt);
	Turns = rS.Turns;
	AdvList = rS.AdvList;
	LTagL = rS.LTagL;
	BTagL = rS.BTagL;
	XcL = rS.XcL;
	Ver = rS.Ver;
	SMemo = rS.SMemo; // @v11.2.4 @fix (это - серьезная недоработка - должно было быть сделано в @v11.1.12)!
	return ok;
}

PPBill & FASTCALL PPBill::operator = (const PPBill & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL PPBill::GetLastPayDate(LDATE * pDt) const
{
	return Pays.GetLast(pDt, 0, 0);
}

int PPBill::AddPayDate(LDATE dt, double amount)
{
	int    ok = 1;
	if(checkdate(dt)) {
		PayPlanTbl::Rec rec;
		rec.BillID  = Rec.ID;
		rec.PayDate = dt;
		rec.Amount  = BR2(amount);
		Pays.insert(&rec);
	}
	else
		ok = -1;
	return ok;
}

int PPBill::SetPayDate(LDATE dt, double amount)
{
	Pays.clear();
	return AddPayDate(dt, amount);
}

#if 0 // @v11.4.0 {
int PPBill::SetGuid(const S_GUID & rGuid)
{
	int    ok = 1;
	if(rGuid.IsZero()) {
		THROW(BTagL.PutItem(PPTAG_BILL_UUID, 0));
	}
	else {
		ObjTagItem tag;
		PPObjTag tagobj;
		PPObjectTag tag_rec;
		THROW_PP(tagobj.Fetch(PPTAG_BILL_UUID, &tag_rec) > 0, PPERR_BILLTAGUUIDABS);
		THROW(tag.SetGuid(PPTAG_BILL_UUID, &rGuid));
		THROW(BTagL.PutItem(PPTAG_BILL_UUID, &tag));
	}
	CATCH
		ok = 0;
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
	ENDCATCH
	return ok;
}

int PPBill::GenerateGuid(S_GUID * pGuid)
{
	int    ok = 1;
	S_GUID guid;
	THROW_SL(guid.Generate());
	THROW(SetGuid(guid));
	CATCHZOK
	ASSIGN_PTR(pGuid, guid);
	return ok;
}

int PPBill::GetGuid(S_GUID & rGuid)
{
	int    ok = 0;
	const ObjTagItem * p_tag_item = BTagL.GetItem(PPTAG_BILL_UUID);
	if(p_tag_item && p_tag_item->GetGuid(&rGuid) > 0) {
		ok = 1;
	}
	else {
		rGuid.Z();
		ok = 0;
	}
	return ok;
}
#endif // } 0 @v11.4.0

PPID PPBill::GetDlvrAddrID() const
{
	// @v12.1.11 {
	PPID   result = 0;
	PPObjBill * p_bobj = BillObj;
	return (p_bobj && p_bobj->GetDlvrAddrID(Rec, P_Freight, &result) > 0) ? result : 0;
	// } @v12.1.11 
	// @v12.1.11 return P_Freight ? P_Freight->DlvrAddrID__ : 0; 
}

int FASTCALL PPBill::GetFreight(PPFreight * pFreight) const
{
	int    ok = -1;
	if(P_Freight) {
		ASSIGN_PTR(pFreight, *P_Freight);
		ok = 1;
	}
	else
		CALLPTRMEMB(pFreight, Z());
	return ok;
}

int FASTCALL PPBill::SetFreight(const PPFreight * pFreight)
{
	int    ok = 1;
	if(pFreight == 0) {
		ZDELETE(P_Freight);
		Rec.Flags &= ~BILLF_FREIGHT;
	}
	else if(!SETIFZ(P_Freight, new PPFreight)) {
		ok = PPSetErrorNoMem();
	}
	else {
		memcpy(P_Freight, pFreight, sizeof(*P_Freight));
		Rec.Flags |= BILLF_FREIGHT;
	}
	return ok;
}

int FASTCALL PPBill::SetFreight_DlvrAddrOnly(PPID dlvrAddrID)
{
	int    ok = -1;
	if(dlvrAddrID) {
		PPFreight freight;
		ok = freight.SetupDlvrAddr(dlvrAddrID) ? SetFreight(&freight) : 0;
	}
	return ok;
}
//
//
//
PPLotTagContainer::PPLotTagContainer() : SArray(sizeof(PPLotTagContainer::Item), aryDataOwner|aryEachItem)
{
}

PPLotTagContainer::PPLotTagContainer(const PPLotTagContainer & rS) : SArray(sizeof(PPLotTagContainer::Item), aryDataOwner|aryEachItem)
{
	Copy(rS);
}

PPLotTagContainer & FASTCALL PPLotTagContainer::operator = (const PPLotTagContainer & rS)
{
    Copy(rS);
    return *this;
}

int FASTCALL PPLotTagContainer::Copy(const PPLotTagContainer & rS)
{
	int    ok = 1;
    Release();
    for(uint i = 0; i < rS.GetCount(); i++) {
		const Item * p_item = static_cast<const Item *>(rS.at(i));
		THROW(Set(p_item->RowIdx, &p_item->List));
    }
    CATCHZOK
    return ok;
}

void PPLotTagContainer::Release() { clear(); }
uint PPLotTagContainer::GetCount() const { return getCount(); }
int  PPLotTagContainer::GetString(PPID tagID, int rowIdx, SString & rBuf) const { return GetTagStr(rowIdx, tagID, rBuf); }

/*virtual*/void FASTCALL PPLotTagContainer::freeItem(void * pItem)
{
	if(pItem)
		static_cast<Item *>(pItem)->List.Z();
}

ObjTagList * FASTCALL PPLotTagContainer::Get(int rowIdx) const
{
	uint   pos = 0;
	int32  key = rowIdx;
	return lsearch(&key, &pos, CMPF_LONG) ? &(static_cast<Item *>(at(pos))->List) : 0;
}

const ObjTagItem * PPLotTagContainer::GetTag(int rowIdx, PPID tagID) const
{
	uint   pos = 0;
	int32  key = rowIdx;
	return (lsearch(&key, &pos, CMPF_LONG)) ? static_cast<const Item *>(at(pos))->List.GetItem(tagID) : 0;
}

bool PPLotTagContainer::SearchString(const char * pPattern, PPID tagID, long flags, LongArray & rRowIdxList) const
{
	bool ok = false;
	rRowIdxList.clear();
	if(!isempty(pPattern)) {
		SString temp_buf;
		for(uint i = 0; i < getCount(); i++) {
			const Item * p_item = static_cast<const Item *>(at(i));
			const  ObjTagItem * p_tag = p_item->List.GetItem(tagID);
			if(p_tag) {
				p_tag->GetStr(temp_buf);
				if(temp_buf.CmpNC(pPattern) == 0) {
					rRowIdxList.add(p_item->RowIdx);
					ok = true;
				}
			}
		}
	}
	return ok;
}

int PPLotTagContainer::GetTagStr(int rowIdx, PPID tagID, SString & rBuf) const
{
	rBuf.Z();
	uint   pos = 0;
	int32  key = rowIdx;
	const ObjTagItem * p_item = (lsearch(&key, &pos, CMPF_LONG)) ? static_cast<const Item *>(at(pos))->List.GetItem(tagID) : 0;
	return (p_item && p_item->GetStr(rBuf) && rBuf.NotEmptyS()) ? 1 : -1;
}

int PPLotTagContainer::Set(int rowIdx, const ObjTagList * pItem)
{
	int    ok = 1;
	uint   pos = 0;
	int32  key = rowIdx;
	if(lsearch(&key, &pos, CMPF_LONG)) {
		if(pItem) {
			Item * p_item = static_cast<Item *>(at(pos));
			p_item->List = *pItem;
			p_item->List.Oid.Obj = PPOBJ_LOT;
			p_item->RowIdx = rowIdx;
		}
		else {
			atFree(pos);
		}
	}
	else if(pItem) {
		//
		// Заботимся о том, чтобы автоматический деструктор не разрушил
		// содержимое контейнера (используем временный пустой объект new_item).
		//
		Item new_item;
		insert(&new_item);
		Item * p_item = static_cast<Item *>(at(getCount()-1));
		p_item->List = *pItem;
		p_item->List.Oid.Obj = PPOBJ_LOT;
		p_item->RowIdx = rowIdx;
	}
	else
		ok = -1;
	return ok;
}

void PPLotTagContainer::RemovePosition(int rowIdx)
{
	int    removed_item_idx = -1;
	for(uint i = 0; i < getCount(); i++) {
		Item & r_item = *static_cast<Item *>(at(i));
		if(r_item.RowIdx > rowIdx)
			r_item.RowIdx--;
		else if(r_item.RowIdx == rowIdx)
			removed_item_idx = i;
	}
	if(removed_item_idx >= 0)
		atFree(removed_item_idx);
}

int PPLotTagContainer::ReplacePosition(int rowIdx, int newRowIdx)
{
	uint   pos = 0;
	int32  key = rowIdx;
	if(lsearch(&key, &pos, CMPF_LONG)) {
		static_cast<Item *>(at(pos))->RowIdx = newRowIdx;
		return 1;
	}
	else
		return -1;
}

int PPLotTagContainer::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir < 0)
		freeAll();
	int32  c = static_cast<int32>(getCount());
	THROW_SL(pSCtx->Serialize(dir, c, rBuf));
	for(int i = 0; i < c; i++) {
		if(dir > 0) {
			Item * p_item = static_cast<Item *>(at(i));
			THROW_SL(pSCtx->Serialize(dir, p_item->RowIdx, rBuf));
			THROW(p_item->List.Serialize(dir, rBuf, pSCtx));
		}
		else if(dir < 0) {
			int32  row_idx = 0;
			ObjTagList temp_list;
			THROW_SL(pSCtx->Serialize(dir, row_idx, rBuf));
			THROW(temp_list.Serialize(dir, rBuf, pSCtx));
			THROW(Set(row_idx, &temp_list));
		}
	}
	CATCHZOK
	return ok;
}

int PPLotTagContainer::ProcessObjRefs(PPObjIDArray * ary, int replace)
{
	int    ok = 1;
	for(uint i = 0; i < getCount(); i++) {
		Item * p_item = static_cast<Item *>(at(i));
		THROW(p_item->List.ProcessObjRefs(ary, replace));
	}
	CATCHZOK
	return ok;
}

int PPLotTagContainer::SetString(PPID tagID, const LongArray * pRows, const char * pClbNumber)
{
	const uint _c = SVector::GetCount(pRows);
	if(_c && !isempty(pClbNumber)) {
		for(uint i = 0; i < _c; i++)
			SetString(tagID, pRows->at(i), pClbNumber);
	}
	return 1;
}

int PPLotTagContainer::SetString(PPID tagID, int rowIdx, const char * pNumber)
{
	int    ok = 1;
	SString temp_buf(pNumber);
	ObjTagList * p_tag_list = Get(rowIdx);
	if(temp_buf.NotEmptyS()) {
		if(p_tag_list) {
			THROW(p_tag_list->PutItemStr(tagID, temp_buf));
		}
		else {
			ObjTagList new_tag_list;
			THROW(new_tag_list.PutItemStr(tagID, temp_buf));
			THROW(Set(rowIdx, &new_tag_list));
		}
	}
	else if(p_tag_list) {
		THROW(p_tag_list->PutItem(tagID, 0));
	}
	CATCHZOK
	return ok;
}

int PPLotTagContainer::GetReal(PPID tagID, int rowIdx, double * pValue) const
{
	int    ok = 0;
	double value = 0.0;
	uint   pos = 0;
	int32  key = rowIdx;
	const ObjTagItem * p_item = (lsearch(&key, &pos, CMPF_LONG)) ? static_cast<const Item *>(at(pos))->List.GetItem(tagID) : 0;
	if(p_item && p_item->GetReal(&value)) {
		ok = 1;
	}
	ASSIGN_PTR(pValue, value);
	return ok;
}

int PPLotTagContainer::SetReal(PPID tagID, int rowIdx, const double * pValue)
{
	int    ok = 1;
	ObjTagList * p_tag_list = Get(rowIdx);
	if(pValue) {
		if(p_tag_list) {
			THROW(p_tag_list->PutItemReal(tagID, *pValue));
		}
		else {
			ObjTagList new_tag_list;
			THROW(new_tag_list.PutItemReal(tagID, *pValue));
			THROW(Set(rowIdx, &new_tag_list));
		}
	}
	else if(p_tag_list) {
		THROW(p_tag_list->PutItem(tagID, 0));
	}
	CATCHZOK
	return ok;
}
//
//
//
PPLotExtCodeContainer::MarkSet::MarkSet() : SStrGroup()
{
}

PPLotExtCodeContainer::MarkSet & PPLotExtCodeContainer::MarkSet::Z()
{
	ClearS();
	L.clear();
	return *this;
}

long PPLotExtCodeContainer::MarkSet::AddBox(long id, const char * pNum, int doVerify)
{
	int    real_id = 0;
	long   max_id = 0;
	THROW_PP_S(!isempty(pNum), PPERR_INVPARAM_EXT, __FUNCTION__"/pNum");
	for(uint i = 0; i < L.getCount(); i++) {
		const InnerEntry & r_entry = L.at(i);
		if(r_entry.Flags & fBox) {
			SETMAX(max_id, r_entry.BoxID);
			if(doVerify)
				THROW(!id || r_entry.BoxID != id);
		}
	}
	{
		real_id = NZOR(id, max_id+1);
		InnerEntry new_entry;
		MEMSZERO(new_entry);
		new_entry.BoxID = real_id;
		new_entry.Flags = fBox;
		THROW_SL(AddS(pNum, &new_entry.NumP));
		THROW_SL(L.insert(&new_entry));
	}
	CATCH
		real_id = 0;
	ENDCATCH
	return real_id;
}

int PPLotExtCodeContainer::MarkSet::GetBoxNum(long boxId, SString & rNum) const
{
	rNum.Z();
	int    ok = 0;
	if(boxId) {
		for(uint i = 0; !ok && i < L.getCount(); i++) {
			const InnerEntry & r_entry = L.at(i);
			if(r_entry.Flags & fBox && r_entry.BoxID == boxId) {
				GetS(r_entry.NumP, rNum);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPLotExtCodeContainer::MarkSet::AddNum(long boxId, const char * pNum, int doVerify)
{
	int    ok = 1;
	SString temp_buf;
	THROW_PP_S(!isempty(pNum), PPERR_INVPARAM_EXT, __FUNCTION__"/pNum");
	THROW(!boxId || GetBoxNum(boxId, temp_buf));
	if(doVerify) {
		for(uint i = 0; i < L.getCount(); i++) {
			const InnerEntry & r_entry = L.at(i);
			if(!(r_entry.Flags & fBox)) {
				GetS(r_entry.NumP, temp_buf);
				THROW(temp_buf != pNum);
			}
		}
	}
	{
		InnerEntry new_entry;
		MEMSZERO(new_entry);
		new_entry.BoxID = boxId;
		THROW_SL(AddS(pNum, &new_entry.NumP));
		THROW_SL(L.insert(&new_entry));
	}
	CATCHZOK
	return ok;
}

uint PPLotExtCodeContainer::MarkSet::GetCount() const
{
	return L.getCount();
}

bool PPLotExtCodeContainer::MarkSet::GetByIdx(uint idx, Entry & rEntry) const
{
	bool   ok = true;
	if(idx < L.getCount()) {
		const InnerEntry & r_entry = L.at(idx);
		rEntry.BoxID = r_entry.BoxID;
		rEntry.Flags = r_entry.Flags;
		GetS(r_entry.NumP, rEntry.Num);
	}
	else
		ok = false;
	return ok;
}

int PPLotExtCodeContainer::MarkSet::GetByBoxID(long boxId, StringSet & rSs) const
{
	int    ok = 1;
	SString temp_buf;
	rSs.Z();
	for(uint i = 0; i < L.getCount(); i++) {
		const InnerEntry & r_entry = L.at(i);
		if(!(r_entry.Flags & fBox) && r_entry.BoxID == boxId) {
			GetS(r_entry.NumP, temp_buf);
			THROW_SL(rSs.add(temp_buf));
		}
	}
	CATCHZOK
	return ok;
}

int PPLotExtCodeContainer::MarkSet::SearchCode(const char * pNum, uint * pIdx) const
{
	int    ok = 0;
	uint   idx = 0;
	SString temp_buf;
	for(uint i = 0; !ok && i < L.getCount(); i++) {
		const InnerEntry & r_entry = L.at(i);
		GetS(r_entry.NumP, temp_buf);
		if(temp_buf == pNum) {
			idx = i;
			ok = 1;
		}
	}
	ASSIGN_PTR(pIdx, idx);
	return ok;
}

long PPLotExtCodeContainer::MarkSet::SearchLastBox(int currentPos) const
{
	long    result_box_id = 0;
	if(L.getCount()) {
		uint    cur_pos = (currentPos < 0 || currentPos >= L.getCountI()) ? L.getCount() : static_cast<uint>(currentPos+1);
		do {
			const InnerEntry & r_entry = L.at(--cur_pos);
			if(r_entry.Flags & fBox) {
				result_box_id = r_entry.BoxID;
				break;
			}
		} while(cur_pos);
	}
	return result_box_id;
}
//
//
//
PPLotExtCodeContainer::PPLotExtCodeContainer() : SVector(sizeof(InnerItem))
{
}

int FASTCALL PPLotExtCodeContainer::Copy(const PPLotExtCodeContainer & rS)
{
	int    ok = 1;
	THROW_SL(SVector::copy(rS));
	SStrGroup::CopyS(rS);
	CATCHZOK
	return ok;
}

PPLotExtCodeContainer::PPLotExtCodeContainer(const PPLotExtCodeContainer & rS) : SVector(rS), SStrGroup(rS)
{
}

PPLotExtCodeContainer & FASTCALL PPLotExtCodeContainer::operator = (const PPLotExtCodeContainer & rS)
{
	Copy(rS);
	return *this;
}

uint PPLotExtCodeContainer::GetCount() const
{
	return SVector::getCount();
}

int FASTCALL PPLotExtCodeContainer::IsEq(const PPLotExtCodeContainer & rS) const
{
	int    eq = 1;
	const  uint _c = getCount();
	if(_c != rS.getCount())
		eq = 0;
	else if(_c) {
		SString code_buf;
		SString code_buf2;
		UintHashTable found_idx_list__;
		for(uint i = 0; eq && i < _c; i++) {
			const InnerItem & r_item = *static_cast<const InnerItem *>(at(i));
			GetS(r_item.CodeP, code_buf);
			int    is_found = 0;
			for(uint j = 0; !is_found && j < _c; j++) {
				if(!found_idx_list__.Has(j+1)) {
					const InnerItem & r_item2 = *static_cast<const InnerItem *>(rS.at(i));
					if(r_item2.RowIdx == r_item.RowIdx && r_item2.Flags == r_item.Flags && r_item2.BoxId == r_item.BoxId) {
						rS.GetS(r_item2.CodeP, code_buf2);
						if(code_buf == code_buf2) {
							found_idx_list__.Add(j+1);
							is_found = 1;
						}
					}
				}
			}
			if(!is_found)
				eq = 0;
		}
	}
	return eq;
}

void PPLotExtCodeContainer::Release()
{
	SVector::clear();
	SStrGroup::ClearS();
}

int PPLotExtCodeContainer::Helper_Add(int rowIdx, long boxId, int16 flags, const char * pCode, int doVerifyUniq, uint * pIdx)
{
	int    ok = 1;
    THROW_PP(!isempty(pCode), PPERR_INVPARAM);
	THROW_PP_S(!doVerifyUniq || !Search(pCode, 0, 0), PPERR_DUPLOTEXTCODE, pCode);
	{
        InnerItem new_item;
        MEMSZERO(new_item);
        new_item.RowIdx = rowIdx;
        new_item.Flags = flags;
		new_item.BoxId = boxId;
        AddS(pCode, &new_item.CodeP);
		THROW_SL(insert(&new_item));
		ASSIGN_PTR(pIdx, getCount()-1);
		ok = 1;
    }
	CATCHZOK
	return ok;
}

int PPLotExtCodeContainer::Add(int rowIdx, long boxId, int16 flags, const char * pCode, uint * pIdx)
{
	int    ok = 1;
    THROW_PP(rowIdx >= 0, PPERR_INVPARAM);
	THROW(Helper_Add(rowIdx, boxId, flags, pCode, 1, pIdx));
	CATCHZOK
	return ok;
}

int PPLotExtCodeContainer::AddValidation(long boxId, int16 flags, const char * pCode, uint * pIdx)
{
	int    ok = 1;
	THROW(Helper_Add(-1, boxId, flags, pCode, 1, pIdx));
	CATCHZOK
	return ok;
}

int PPLotExtCodeContainer::Delete(int rowIdx, uint itemIdx)
{
	int    ok = 1;
    //THROW(rowIdx >= 0);
	THROW(itemIdx < SVector::getCount());
	{
		Item2 item;
		if(GetByIdx(itemIdx, item)) {
			if(item.Flags & fBox) {
				uint i = SVector::getCount();
				if(i) do {
					InnerItem * p_ii = static_cast<InnerItem *>(SVector::at(--i));
					if(p_ii->BoxId == item.BoxId)
						SVector::atFree(i);
				} while(i);
			}
			else {
				SVector::atFree(itemIdx);
				if(item.BoxId) {
					uint  i;
					int   is_other_by_box = 0;
					int   box_pos = -1;
					for(i = 0; i < SVector::getCount(); i++) {
						const InnerItem * p_ii = static_cast<const InnerItem *>(SVector::at(i));
						if(p_ii->BoxId == item.BoxId) {
							if(p_ii->Flags & fBox)
								box_pos = static_cast<int>(i);
							else
								is_other_by_box = 1;
						}
					}
					if(!is_other_by_box && box_pos >= 0) {
						SVector::atFree(box_pos);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPLotExtCodeContainer::Add(int rowIdx, const MarkSet & rS)
{
	int    ok = -1;
	MarkSet::Entry entry;
	THROW_PP(rowIdx >= 0, PPERR_INVPARAM);
	for(uint i = 0; i < rS.GetCount(); i++) {
		if(rS.GetByIdx(i, entry))
			THROW(Add(rowIdx, entry.BoxID, static_cast<int16>(entry.Flags), entry.Num, 0));
	}
	CATCHZOK
	return ok;
}

int PPLotExtCodeContainer::AddValidation(const MarkSet & rS)
{
	int    ok = -1;
	MarkSet::Entry entry;
	for(uint i = 0; i < rS.GetCount(); i++) {
		if(rS.GetByIdx(i, entry)) {
			THROW(AddValidation(entry.BoxID, static_cast<int16>(entry.Flags), entry.Num, 0));
		}
	}
	CATCHZOK
	return ok;
}

int PPLotExtCodeContainer::Set_2(int rowIdx, const MarkSet * pS)
{
	int    ok = -1;
	THROW_PP(rowIdx >= 0, PPERR_INVPARAM);
	if(!pS) {
		uint i = getCount();
		if(i) do {
			InnerItem & r_entry = *static_cast<InnerItem *>(at(--i));
			if(r_entry.RowIdx == static_cast<int16>(rowIdx)) {
				atFree(i);
				ok = 1;
			}
		} while(i);
	}
	else {
		SString temp_buf;
		THROW(Set_2(rowIdx, 0)); // @recursion Вычищаем все коды для строки rowIdx
		MarkSet::Entry entry;
		for(uint i = 0; i < pS->GetCount(); i++) {
			if(pS->GetByIdx(i, entry)) {
				if(entry.Flags & fBox) {
					long suffix_code = 0;
					temp_buf = entry.Num;
					while(Search(temp_buf, 0, 0)) {
						(temp_buf = entry.Num).CatChar('-').Cat(++suffix_code);
					}
					THROW(Add(rowIdx, entry.BoxID, static_cast<int16>(entry.Flags), temp_buf, 0));
				}
				else {
					THROW(Add(rowIdx, entry.BoxID, static_cast<int16>(entry.Flags), entry.Num, 0));
				}
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPLotExtCodeContainer::Get(int rowIdx, LongArray * pIdxList, MarkSet & rS) const
{
	int    ok = -1;
	CALLPTRMEMB(pIdxList, clear());
	rS.Z();
	SString temp_buf;
	SString box_num;
	LongArray seen_idx_list;
	for(uint i = 0; i < getCount(); i++) {
        const InnerItem & r_item = *static_cast<const InnerItem *>(at(i));
        if(r_item.RowIdx == rowIdx && !seen_idx_list.lsearch(static_cast<long>(i)) && GetS(r_item.CodeP, temp_buf)) {
			if(r_item.Flags & fBox) {
				THROW(rS.AddBox(r_item.BoxId, temp_buf, 0));
			}
			else {
				if(r_item.BoxId && !rS.GetBoxNum(r_item.BoxId, box_num)) {
					for(uint j = i+1; j < getCount(); j++) {
						const InnerItem & r_item2 = *static_cast<const InnerItem *>(at(j));
						if(r_item2.RowIdx == rowIdx && !seen_idx_list.lsearch(static_cast<long>(j)) && r_item2.Flags & fBox && GetS(r_item2.CodeP, box_num)) {
							THROW(rS.AddBox(r_item2.BoxId, box_num, 0));
							seen_idx_list.add(j);
							break;
						}
					}
				}
				THROW(rS.AddNum(r_item.BoxId, temp_buf, 0));
			}
			CALLPTRMEMB(pIdxList, add(static_cast<long>(i)));
			ok = 1;
        }
	}
	CATCHZOK
	return ok;
}

int PPLotExtCodeContainer::GetByIdx(uint idx, PPLotExtCodeContainer::Item2 & rItem) const
{
	int    ok = 1;
	if(idx < getCount()) {
		const InnerItem & r_item = *static_cast<const InnerItem *>(at(idx));
		rItem.RowIdx = r_item.RowIdx;
		rItem.Flags = r_item.Flags;
		rItem.BoxId = r_item.BoxId;
		GetS(r_item.CodeP, rItem.Num);
	}
	else {
		rItem.BoxId = 0;
		rItem.Flags = 0;
		rItem.RowIdx = 0;
		rItem.Num.Z();
		ok = 0;
	}
	return ok;
}

int PPLotExtCodeContainer::GetByBoxID(long boxId, StringSet & rSs) const
{
	int    ok = -1;
	SString temp_buf;
	for(uint i = 0; i < getCount(); i++) {
		const InnerItem & r_item = *static_cast<const InnerItem *>(at(i));
		if(r_item.BoxId == boxId && !(r_item.Flags & fBox)) {
			GetS(r_item.CodeP, temp_buf);
			if(temp_buf.NotEmptyS()) {
				rSs.add(temp_buf);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPLotExtCodeContainer::Search(const char * pCode, int * pRowIdx, uint * pInnerIdx) const
{
	int    ok = 0;
	for(uint pos = 0; !ok && SStrGroup::Pool.search(pCode, &pos, 1); pos++) {
		uint vpos = 0;
        if(SVector::lsearch(&pos, &vpos, CMPF_LONG, offsetof(InnerItem, CodeP))) {
			const InnerItem * p_item = static_cast<const InnerItem *>(SVector::at(vpos));
			ASSIGN_PTR(pRowIdx, p_item->RowIdx);
			ASSIGN_PTR(pInnerIdx, vpos);
            ok = 1;
        }
	}
	return ok;
}

int PPLotExtCodeContainer::ValidateCode(const char * pCode, const char * pBox, int * pErr, int * pRowId, SString * pBoxCode) const
{
	int    ok = 1;
	int    err = 0;
	int    row_idx = -1;
	const SString src_box_code(pBox);
	SString box_code;
	if(isempty(pCode)) {
		err = 1;
		ok = 0;
	}
	else {
		const  bool iemr = PrcssrAlcReport::IsEgaisMark(pCode, 0);
		uint   inner_idx = 0;
		if(Search(pCode, &row_idx, &inner_idx)) {
			const InnerItem * p_item = static_cast<const InnerItem *>(SVector::at(inner_idx));
			assert(p_item);
			if(p_item->Flags & fBox) {
				GetS(p_item->CodeP, box_code);
				ok = 2;
			}
			else if(p_item->BoxId) {
				for(uint i = 0; i < getCount(); i++) {
					const InnerItem * p_item2 = static_cast<const InnerItem *>(SVector::at(i)); // @v11.1.0 @fix inner_idx-->i
					if(p_item2->RowIdx == p_item->RowIdx && p_item2->BoxId == p_item->BoxId && p_item2->Flags & fBox) { // @v11.1.0 @fix (p_item2->RowIdx == p_item->RowIdx &&)
						GetS(p_item2->CodeP, box_code);
						if(src_box_code.NotEmpty() && box_code != src_box_code) {
							err = 3; // Не та коробка
							ok = 0;
						}
						break;
					}
				}
			}
		}
		else {
			err = 2; // марка не найдена
			ok = 0;
		}
	}
	ASSIGN_PTR(pErr, err);
	ASSIGN_PTR(pRowId, row_idx);
	ASSIGN_PTR(pBoxCode, box_code);
	return ok;
}

void PPLotExtCodeContainer::RemovePosition(int rowIdx)
{
	uint i = getCount();
	if(i) do {
		InnerItem & r_item = *static_cast<InnerItem *>(at(--i));
		if(r_item.RowIdx > rowIdx)
			r_item.RowIdx--;
		else if(r_item.RowIdx == rowIdx)
			atFree(i);
	} while(i);
}

int PPLotExtCodeContainer::ReplacePosition(int rowIdx, int newRowIdx)
{
	int    ok = -1;
	for(uint i = 0; i < getCount(); i++) {
		InnerItem & r_item = *static_cast<InnerItem *>(at(i));
		if(r_item.RowIdx == rowIdx) {
			r_item.RowIdx = newRowIdx;
			ok = 1;
		}
	}
	return ok;
}

struct PPLotExtCodeContainer_Item_Before10209 { // @persistent
	int16  RowIdx;
	int16  Sign;
	uint   CodeP;
};

int PPLotExtCodeContainer::Serialize_Before10209(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir < 0) {
		SVector::clear();
		TSVector <PPLotExtCodeContainer_Item_Before10209> temp_vect;
		THROW_SL(pSCtx->Serialize(dir, &temp_vect, rBuf));
		for(uint i = 0; i < temp_vect.getCount(); i++) {
			const PPLotExtCodeContainer_Item_Before10209 & r_src_item = temp_vect.at(i);
			InnerItem new_item;
			MEMSZERO(new_item);
			new_item.RowIdx = r_src_item.RowIdx;
			new_item.CodeP = r_src_item.CodeP;
			THROW_SL(SVector::insert(&new_item));
		}
		THROW_SL(SStrGroup::SerializeS(dir, rBuf, pSCtx));
	}
	else {
		THROW(Serialize(dir, rBuf, pSCtx));
	}
	CATCHZOK
	return ok;
}

int PPLotExtCodeContainer::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, (SVector *)this, rBuf));
	THROW_SL(SStrGroup::SerializeS(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}
//
//
//
SString & PPTaxPeriod::Format(SString & rBuf) const
{
	rBuf.Z();
	//char   buf[32];
	size_t p = 0;
	if(P != eEmpty) {
		switch(P) {
			case eYear:
				// ГД
				//rBuf.Cat("ГД").DotCat("00");
				PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_PERIOD_GD, rBuf).Transf(CTRANSF_INNER_TO_OUTER).DotCat("00");
				//buf[p++] = 'ѓ'; buf[p++] = '„'; buf[p++] = '.'; buf[p++] = '0'; buf[p++] = '0';
				break;
			case eSemiyear1:
				// ПЛ
				//rBuf.Cat("ПЛ").DotCat("01");
				PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_PERIOD_PL, rBuf).Transf(CTRANSF_INNER_TO_OUTER).DotCat("01");
				//buf[p++] = 'Џ'; buf[p++] = '‹'; buf[p++] = '.'; buf[p++] = '0'; buf[p++] = '1';
				break;
			case eSemiyear2:
				// ПЛ
				//rBuf.Cat("ПЛ").DotCat("02");
				PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_PERIOD_PL, rBuf).Transf(CTRANSF_INNER_TO_OUTER).DotCat("02");
				//buf[p++] = 'Џ'; buf[p++] = '‹'; buf[p++] = '.'; buf[p++] = '0'; buf[p++] = '2';
				break;
			case eQuart1:
				// КВ
				//rBuf.Cat("КВ").DotCat("01");
				PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_PERIOD_KV, rBuf).Transf(CTRANSF_INNER_TO_OUTER).DotCat("01");
				//buf[p++] = 'Љ'; buf[p++] = '‚'; buf[p++] = '.'; buf[p++] = '0'; buf[p++] = '1';
				break;
			case eQuart2:
				// КВ
				//rBuf.Cat("КВ").DotCat("02");
				PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_PERIOD_KV, rBuf).Transf(CTRANSF_INNER_TO_OUTER).DotCat("02");
				//buf[p++] = 'Љ'; buf[p++] = '‚'; buf[p++] = '.'; buf[p++] = '0'; buf[p++] = '2';
				break;
			case eQuart3:
				// КВ
				//rBuf.Cat("КВ").DotCat("03");
				PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_PERIOD_KV, rBuf).Transf(CTRANSF_INNER_TO_OUTER).DotCat("03");
				//buf[p++] = 'Љ'; buf[p++] = '‚'; buf[p++] = '.'; buf[p++] = '0'; buf[p++] = '3';
				break;
			case eQuart4:
				// КВ
				//rBuf.Cat("КВ").DotCat("04");
				PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_PERIOD_KV, rBuf).Transf(CTRANSF_INNER_TO_OUTER).DotCat("04");
				//buf[p++] = 'Љ'; buf[p++] = '‚'; buf[p++] = '.'; buf[p++] = '0'; buf[p++] = '4';
				break;
			case eMonth:
				// МС
				//rBuf.Cat("МС").Dot().CatLongZ(Month, 2);
				PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_PERIOD_MS, rBuf).Transf(CTRANSF_INNER_TO_OUTER).Dot().CatLongZ(Month, 2);
				//buf[p++] = 'Њ'; buf[p++] = '‘'; buf[p++] = '.';
				//buf[p++] = '0' + (Month / 10); buf[p++] = '0' + (Month % 10);
				break;
			case eDec1:
				// Д1
				//rBuf.Cat("Д1").Dot().CatLongZ(Month, 2);
				PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_PERIOD_D1, rBuf).Transf(CTRANSF_INNER_TO_OUTER).Dot().CatLongZ(Month, 2);
				//buf[p++] = '„'; buf[p++] = '1'; buf[p++] = '.';
				//buf[p++] = '0' + (Month / 10); buf[p++] = '0' + (Month % 10);
				break;
			case eDec2:
				// Д2
				//rBuf.Cat("Д2").Dot().CatLongZ(Month, 2);
				PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_PERIOD_D2, rBuf).Transf(CTRANSF_INNER_TO_OUTER).Dot().CatLongZ(Month, 2);
				//buf[p++] = '„'; buf[p++] = '2'; buf[p++] = '.';
				//buf[p++] = '0' + (Month / 10); buf[p++] = '0' + (Month % 10);
				break;
			case eDec3:
				// Д3
				//rBuf.Cat("Д3").Dot().CatLongZ(Month, 2);
				PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_PERIOD_D3, rBuf).Transf(CTRANSF_INNER_TO_OUTER).Dot().CatLongZ(Month, 2);
				//buf[p++] = '„'; buf[p++] = '3'; buf[p++] = '.';
				//buf[p++] = '0' + (Month / 10); buf[p++] = '0' + (Month % 10);
				break;
			case eDate:
				rBuf.CatLongZ(Day, 2).Dot().CatLongZ(Month, 2);
				//buf[p++] = '0' + (Day / 10); buf[p++] = '0' + (Day % 10); buf[p++] = '.';
				//buf[p++] = '0' + (Month / 10); buf[p++] = '0' + (Month % 10);
				break;
		}
		if(rBuf.NotEmpty())
			rBuf.Dot().Cat(Year);
		/*if(p) {
			buf[p++] = '.';
			ltoa(Year, buf+p, 10);
		}*/
	}
	rBuf.Transf(CTRANSF_OUTER_TO_INNER);
	//rBuf = buf;
	return rBuf;
}

PPBankingOrder::PPBankingOrder()
{
	THISZERO();
	Tag = PPOBJ_BILL;
	PropID = BILLPRP_PAYMORDER;
}

bool PPBankingOrder::TaxMarkers::IsEmpty() const
{
	return !(TaxClass[0] || OKATO[0] || Reason[0] || Period.Year || DocNumber[0] || DocDate || PaymType[0] || TaxClass2[0]);
}
//
//
//
PPBillPacket::SetupObjectBlock::SetupObjectBlock() : State(0), PsnID(0)
{
    //Clear_();
	Flags = 0;
}

PPBillPacket::SetupObjectBlock & PPBillPacket::SetupObjectBlock::Z()
{
	// Flags = 0;
	State = 0;
	PsnID = 0;
	Name.Z();
	RegInfoList.clear();
	CliAgt.Z();
	SupplAgt.Z();
	return *this;
}

PPBillPacket::CipBlock::CipBlock() : P_CipList(0), P_TSesObj(0)
{
}

PPBillPacket::CipBlock::CipBlock(const PPBillPacket::CipBlock & rS) : P_CipList(0), P_TSesObj(0)
{
	Copy(rS);
}

PPBillPacket::CipBlock::~CipBlock()
{
	destroy();
}

void PPBillPacket::CipBlock::destroy()
{
	ZDELETE(P_CipList);
	ZDELETE(P_TSesObj);
}

PPBillPacket::CipBlock & FASTCALL PPBillPacket::CipBlock::operator = (const PPBillPacket::CipBlock & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL PPBillPacket::CipBlock::Copy(const PPBillPacket::CipBlock & rS)
{
	int    ok = 1;
	if(rS.P_CipList) {
		THROW_MEM(SETIFZ(P_CipList, new TSVector <PPCheckInPersonItem>));
		*P_CipList = *rS.P_CipList;
	}
	else
		ZDELETE(P_CipList);
	if(rS.P_TSesObj) {
		THROW_MEM(SETIFZ(P_TSesObj, new PPObjTSession));
	}
	else
		ZDELETE(P_TSesObj);
	CATCHZOK
	return ok;
}

int PPBillPacket::AddTSessCip(PPID tsessID, const char * pPlaceCode, PPID personID)
{
	int    ok = 1;
	if(tsessID) {
		TSessionTbl::Rec tses_rec;
        THROW_MEM(SETIFZ(CipB.P_TSesObj, new PPObjTSession));
        THROW(CipB.P_TSesObj->Search(tsessID, &tses_rec) > 0);
		THROW_MEM(SETIFZ(CipB.P_CipList, new TSVector <PPCheckInPersonItem>));
        {
        	PPCheckInPersonItem cip_item;
        	cip_item.PrmrID = tsessID;
        	if(personID)
				cip_item.SetPerson(personID);
			else
				cip_item.SetAnonym();
        	STRNSCPY(cip_item.PlaceCode, pPlaceCode);
			THROW_SL(CipB.P_CipList->insert(&cip_item));
        }
	}
	CATCHZOK
	return ok;
}

LDATE PPBillPacket::CalcDefaultPayDate(int paymTerm, long paymDateBase) const
{
	LDATE  paym_date = ZERODATE;
	if(paymTerm >= 0) {
		LDATE   base_date = ZERODATE;
		switch(paymDateBase) {
			case PPClientAgreement::pdbMain:
				base_date = Rec.Dt;
				break;
			case PPClientAgreement::pdbInvoice:
				base_date = Ext.InvoiceDate;
				break;
			case PPClientAgreement::pdbFreightIssue:
				if(P_Freight)
					base_date = P_Freight->IssueDate;
				break;
			case PPClientAgreement::pdbFreightArrival:
				if(P_Freight)
					base_date = P_Freight->ArrivalDate;
				break;
			default:
				if(paymDateBase > PPClientAgreement::pdbTagBias) {
					PPObjTag tag_obj;
					PPObjectTag tag_rec;
					if(tag_obj.Fetch(paymDateBase, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_BILL &&
						oneof2(tag_rec.TagDataType, OTTYP_DATE, OTTYP_TIMESTAMP)) {
						const ObjTagItem * p_tag_item = BTagL.GetItem(paymDateBase);
						CALLPTRMEMB(p_tag_item, GetDate(&base_date));
					}
				}
				break;
		}
		if(!checkdate(base_date))
			base_date = Rec.Dt;
		if(checkdate(base_date))
			paym_date = plusdate(base_date, paymTerm);
	}
	return paym_date;
}

int PPBillPacket::SetupDefaultPayDate(int paymTerm, long paymDateBase)
{
	const LDATE paym_date = CalcDefaultPayDate(paymTerm, paymDateBase);
	return paym_date ? SetPayDate(paym_date, 0) : -1;
}

void PPBillPacket::SetupEdiAttributes(int ediOp, const char * pEdiChannel, const char * pEdiIdent)
{
	Rec.EdiOp = ediOp;
	BTagL.PutItemStrNE(PPTAG_BILL_EDICHANNEL, pEdiChannel);
	BTagL.PutItemStrNE(PPTAG_BILL_EDIIDENT, pEdiIdent);
}

int PPBillPacket::SetupObject2(PPID arID)
{
	int    ok = -1;
	const  PPID preserve_ar2 = Rec.Object2;
	if(arID != Rec.Object2) {
		if(arID) {
			PPOprKind op_rec;
			PPObjArticle ar_obj;
			ArticleTbl::Rec ar_rec;
			THROW(GetOpData(Rec.OpID, &op_rec) > 0);
			THROW_PP_S(op_rec.AccSheet2ID, PPERR_OPHASNTACS2, op_rec.Name);
			THROW(ar_obj.Fetch(arID, &ar_rec) > 0);
			THROW_PP_S(ar_rec.AccSheetID == op_rec.AccSheet2ID, PPERR_ARDONTBELONGOPACS2, ar_rec.Name);
		}
		Rec.Object2 = arID;
		for(uint i = 0; i < GetTCount(); i++) {
			const PPTransferItem & r_ti = ConstTI(i);
			THROW(CheckGoodsForRestrictions((int)i, r_ti.GoodsID, TISIGN_UNDEF, r_ti.Qtty(), cgrfObject2, 0));
		}
		ok = 1;
	}
	CATCH
		Rec.Object2 = preserve_ar2;
		ok = 0;
	ENDCATCH
	return ok;
}

int PPBillPacket::SetupObject(PPID arID, SetupObjectBlock & rRet)
{
	int    ok = 1;
	const  PPID preserve_ar_id = Rec.Object;
	SString temp_buf;
	PPOprKind op_rec;
	rRet.Z();
	ProcessFlags &= ~(pfRestrictByArCodes | pfSubCostOnSubPartStr);
	LAssocArray rglist;
	const bool getopdata_result = (GetOpData(Rec.OpID, &op_rec) > 0); // @v12.0.8
	THROW(!arID || getopdata_result);
	if(arID) {
		PPObjArticle ar_obj;
		ArticleTbl::Rec ar_rec;
		PPID   acs_id = 0;
		THROW(ar_obj.Search(arID, &ar_rec) > 0);
		// @v12.2.5 THROW_PP_S(ar_rec.AccSheetID == op_rec.AccSheetID, PPERR_ARDONTBELONGOPACS, ar_rec.Name); // @v12.0.8
		// @v12.2.5 {
		if(ar_rec.AccSheetID != op_rec.AccSheetID) {
			bool local_err = true;
			if(!op_rec.AccSheetID && op_rec.LinkOpID) {
				PPOprKind link_op_rec;
				if(GetOpData(op_rec.LinkOpID, &link_op_rec) > 0 && link_op_rec.AccSheetID == ar_rec.AccSheetID)
					local_err = false;
			}
			THROW_PP_S(!local_err, PPERR_ARDONTBELONGOPACS, ar_rec.Name);
		}
		// } @v12.2.5 
		const  int    agt_kind = PPObjArticle::GetAgreementKind(&ar_rec);
		rRet.PsnID = ObjectToPerson(arID, &acs_id);
		rRet.Name = ar_rec.Name;
		if(agt_kind == 1) {
			if(ar_obj.GetClientAgreement(arID, rRet.CliAgt, 1) > 0) {
				rRet.State |= rRet.stHasCliAgreement;
				AgtQuotKindID = rRet.CliAgt.DefQuotKindID;
				if(rRet.CliAgt.Flags & AGTF_USEMARKEDGOODSONLY)
					ProcessFlags |= pfRestrictByArCodes;
			}
		}
		else if(agt_kind == 2) {
			PPSupplAgreement sa_rec;
			if(ar_obj.GetSupplAgreement(arID, &rRet.SupplAgt, 1) > 0) {
				rRet.State |= rRet.stHasSupplAgreement;
				if(rRet.SupplAgt.Flags & AGTF_USEMARKEDGOODSONLY)
					ProcessFlags |= pfRestrictByArCodes;
				if(rRet.SupplAgt.Flags & AGTF_SUBCOSTONSUBPARTSTR)
					ProcessFlags |= pfSubCostOnSubPartStr;
			}
		}
		if(OpTypeID == PPOPT_AGREEMENT) {
			if(!P_Agt || P_Agt->IsEmpty()) {
				SETIFZ(P_Agt, new Agreement);
				if(rRet.State & rRet.stHasCliAgreement) {
					P_Agt->DefAgentID = rRet.CliAgt.DefAgentID;
					P_Agt->DefPayPeriod = rRet.CliAgt.DefPayPeriod;
					P_Agt->DefQuotKindID = rRet.CliAgt.DefQuotKindID;
					P_Agt->MaxCredit = rRet.CliAgt.MaxCredit;
					P_Agt->MaxDscnt = rRet.CliAgt.MaxDscnt;
					P_Agt->Dscnt = rRet.CliAgt.Dscnt;
				}
				else if(rRet.State & rRet.stHasSupplAgreement) {
					P_Agt->DefAgentID = rRet.SupplAgt.DefAgentID;
					P_Agt->DefPayPeriod = rRet.SupplAgt.DefPayPeriod;
					P_Agt->PctRet = rRet.SupplAgt.PctRet;
					P_Agt->DefDlvrTerm = rRet.SupplAgt.DefDlvrTerm;
					P_Agt->DefQuotKindID = rRet.SupplAgt.CostQuotKindID;
				}
			}
		}
		else {
			if(Rec.Flags & BILLF_GEXPEND || oneof2(OpTypeID, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND)) {
				const  bool ignore_stop = ((rRet.Flags & SetupObjectBlock::fEnableStop) || (getopdata_result && op_rec.ExtFlags & OPKFX_IGNORECLISTOP));
				if(!ignore_stop) {
					int    is_stopped = -1;
					SString stop_err_addedmsg;
					PPObjBill::MakeCodeString(&Rec, 0, stop_err_addedmsg); // @v11.4.3
					stop_err_addedmsg.CatDiv('-', 1).Cat(ar_rec.Name);
					if(rRet.State & rRet.stHasCliAgreement) {
						PPID   debt_dim_id = 0;
						GetDebtDim(&debt_dim_id);
						is_stopped = rRet.CliAgt.IsStopped(debt_dim_id);
						if(is_stopped > 0) {
							GetObjectName(PPOBJ_DEBTDIM, debt_dim_id, temp_buf);
							stop_err_addedmsg.Colon().Cat(temp_buf);
						}
					}
					if(is_stopped < 0)
						is_stopped = BIN(ar_rec.Flags & ARTRF_STOPBILL);
					THROW_PP_S(!is_stopped, PPERR_DENYSTOPPEDAR, stop_err_addedmsg);
				}
			}
			if(rRet.PsnID) {
				PPID   restrict_psn_kind = 0;
				PPAccSheet acs_rec;
				PPObjAccSheet acs_obj;
				if(acs_obj.Fetch(acs_id, &acs_rec) > 0) {
					restrict_psn_kind = acs_rec.ObjGroup;
				}
				{
					PPObjPerson psn_obj;
					PPObjRegisterType rt_obj;
					PPRegisterType rt_rec;
					RegisterArray reg_list;
					psn_obj.GetRegList(rRet.PsnID, &reg_list, 1);
					for(uint i = 0; i < reg_list.getCount(); i++) {
						RegisterTbl::Rec & r_reg_rec = reg_list.at(i);
						if(rt_obj.Fetch(r_reg_rec.RegTypeID, &rt_rec) > 0) {
							if(rt_rec.Flags & REGTF_WARNEXPIRY && r_reg_rec.Expiry && Rec.Dt > r_reg_rec.Expiry) {
								rRet.RegInfoList.Add(rt_rec.ID, r_reg_rec.ID, 0);
							}
							if(rt_rec.RestrictGoodsGrpID) {
								if(rt_rec.RestrictGoodsKind == PPRegisterType::ggrpaOnlyGroup)
									rglist.Add(rt_rec.RestrictGoodsGrpID, 1, 0);
								else if(rt_rec.RestrictGoodsKind == PPRegisterType::ggrpaDenyGroup)
									rglist.Add(rt_rec.RestrictGoodsGrpID, -1, 0);
							}
						}
					}
					for(SEnum en = PPRef->Enum(PPOBJ_REGISTERTYPE, 0); en.Next(&rt_rec) > 0;) {
						if(reg_list.GetRegister(rt_rec.ID, 0, 0) < 0) {
							if(rt_rec.Flags & REGTF_WARNABSENCE) {
								rRet.RegInfoList.Add(rt_rec.ID, 0, 0);
							}
							if(rt_rec.RestrictGoodsGrpID && rt_rec.RestrictGoodsKind == PPRegisterType::ggrpaAllowByReg) {
								if(!rt_rec.PersonKindID || rt_rec.PersonKindID == acs_rec.ObjGroup) {
									rglist.Add(rt_rec.RestrictGoodsGrpID, -1, 0);
								}
							}
						}
					}
				}
			}
		}
	}
	if(rglist.getCount()) {
		THROW_MEM(SETIFZ(P_GoodsGrpRestrict, new LAssocArray));
		*P_GoodsGrpRestrict = rglist;
	}
	else {
		ZDELETE(P_GoodsGrpRestrict);
	}
	if(arID && Rec.Object != arID) {
		Rec.Object = arID;
		if(GetTCount()) {
			PPObjGoods goods_obj;
			const int invp_act = DS.GetTLA().InvalidSupplDealQuotAction;
			const PPCommConfig & r_ccfg = CConfig;
			for(uint i = 0; i < GetTCount(); i++) {
				const PPTransferItem & r_ti = ConstTI(i);
				THROW(CheckGoodsForRestrictions(static_cast<int>(i), r_ti.GoodsID, TISIGN_UNDEF, r_ti.Qtty(), cgrfObject, 0));
				if(oneof2(OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT)) {
					PPSupplDeal sd;
					const QuotIdent qi(r_ti.LocID, 0, r_ti.CurID, arID);
					goods_obj.GetSupplDeal(r_ti.GoodsID, qi, &sd, 1);
					THROW_PP(!sd.IsDisabled, PPERR_GOODSRCPTDISABLED);
					if(invp_act == PPSupplAgreement::invpaRestrict && (OpTypeID == PPOPT_GOODSRECEIPT || (Rec.OpID == r_ccfg.DraftRcptOp && r_ccfg.Flags2 & CCFLG2_USESDONPURCHOP))) {
						THROW_PP_S(sd.CheckCost(r_ti.Cost), PPERR_SUPPLDEALVIOLATION, sd.Format(temp_buf));
					}
				}
			}
		}
	}
	Rec.Object = arID;
	CATCH
		Rec.Object = preserve_ar_id;
		ok = 0;
	ENDCATCH
	return ok;
}

int PPBillPacket::SetupDlvrAddr(PPID dlvrAddrID)
{
	int    ok = 1;
	if(P_Freight || dlvrAddrID) {
		THROW_SL(SETIFZ(P_Freight, new PPFreight));
		THROW(P_Freight->SetupDlvrAddr(dlvrAddrID));
	}
	CATCHZOK
	return ok;
}

int PPBillPacket::GetContextEmailAddr(SString & rBuf) const
{
	rBuf.Z();
	int    ok = -1;
	if(Rec.Object) {
		PPID   psn_id = ObjectToPerson(Rec.Object);
		if(psn_id) {
			StringSet ss;
			PPObjPerson psn_obj;
			PPELinkArray elink_list;
			psn_obj.P_Tbl->GetELinks(psn_id, elink_list);
			if(elink_list.GetListByType(ELNKRT_EMAIL, ss) > 0) {
				assert(ss.getCount()); // @v11.3.5
				ss.get(0U, rBuf);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPBillPacket::AttachRowToOrder(int itemNo, const PPBillPacket * pOrdPack)
{
	int    ok = -1;
	if(itemNo >= 0 && pOrdPack && CheckOpFlags(Rec.OpID, OPKF_ONORDER)) {
		PPTransferItem & r_ti = TI(itemNo);
		if(!(r_ti.Flags & PPTFR_ONORDER)) {
			uint   ord_pos = 0, j;
			for(double qtty = fabs(r_ti.Quantity_); qtty > 0.0 && pOrdPack->SearchGoods(r_ti.GoodsID, &ord_pos); ord_pos++) {
				const  PPTransferItem & r_ord_ti = pOrdPack->ConstTI(ord_pos);
				PPTransferItem * p_tmp_sti;
				double ord_qtty = 0.0;
				THROW(RestByOrderLot(r_ord_ti.LotID, 0, -1, &ord_qtty));
				ord_qtty = MIN(ord_qtty, qtty);
				if(ord_qtty > 0.0) {
					r_ti.OrdLotID = r_ord_ti.LotID; // @ordlotid
					r_ti.Flags   |= PPTFR_ONORDER;
					if(!SearchShLot(r_ti.OrdLotID, &(j = 0))) // @ordlotid
						THROW(AddShadowItem(&r_ord_ti, &j));
					p_tmp_sti = &P_ShLots->at(j);
					THROW(CalcShadowQuantity(p_tmp_sti->LotID, &p_tmp_sti->Quantity_));
					qtty -= ord_qtty;
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPBillPacket::AttachToOrder(const PPBillPacket * pOrdPack)
{
	int    ok = -1;
	for(uint i = 0; ok && i < GetTCount(); i++) {
		const int r = AttachRowToOrder((int)i, pOrdPack);
		if(r > 0)
			ok = 1;
		else if(!r)
			ok = 0;
	}
	return ok;
}

int PPBillPacket::SetupRow(int itemNo, PPTransferItem * pItem, const PPTransferItem * pOrdItem, double extraQtty)
{
	int    ok = 1, j = -2;
	uint   p;
	if(pItem->Flags & PPTFR_ONORDER) {
		uint    i = 0;
		j = SearchShLot(pItem->OrdLotID, &i) ? i : -1; // @ordlotid
	}
	if(itemNo == -1) {
		if(j == -1) {
			THROW(AddShadowItem(pOrdItem, &p));
			j = (int)p;
		}
		THROW(InsertRow(pItem, 0, PCUG_USERCHOICE));
		LTagL.ReplacePosition(-1, GetTCount() - 1);
		CheckLargeBill(1);
	}
	else if(pItem->Flags & PPTFR_AUTOCOMPL) {
		THROW(UpdateAutoComplRow(itemNo));
	}
	if(j >= 0) {
		PPTransferItem * ti = &P_ShLots->at(j);
		THROW(CalcShadowQuantity(ti->LotID, &ti->Quantity_));
	}
	else if(j == -1)
		pItem->Flags &= ~PPTFR_ONORDER;
	if(R6(extraQtty) > 0.0 && pItem->Flags & PPTFR_MINUS) {
		ILTI   ilti;
		LongArray row_pos_list;
		uint   fl = CILTIF_ABSQTTY;
		ilti.GoodsID  = pItem->GoodsID;
	   	ilti.Price    = pItem->NetPrice();
		if(pItem->CurID)
			ilti.CurPrice = pItem->CurPrice;
		ilti.SetQtty(-extraQtty);
		if(pItem->Flags & PPTFR_QUOT)
			fl |= CILTIF_QUOT;
		// @v11.7.4 {
		if(ilti.Price == 0.0) {
			PPObjGoods goods_obj;
			if(goods_obj.IsZeroPriceAllowed(ilti.GoodsID))
				fl |= CILTIF_ALLOWZPRICE;
		}
		// } @v11.7.4 
		THROW(P_BObj->ConvertILTI(&ilti, this, &row_pos_list, fl, 0));
		if(IsIntrExpndOp(Rec.OpID)) {
			for(uint i = 0; i < row_pos_list.getCount(); i++) {
				const uint row_pos = row_pos_list.at(i);
				const PPTransferItem & r_item = TI(row_pos);
				ObjTagList tag_list;
				P_BObj->GetTagListByLot(r_item.LotID, 0/*skipReserveTags*/, &tag_list);
				LTagL.Set(row_pos, tag_list.GetCount() ? &tag_list : 0);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPBillPacket::RemoveRow(uint rowIdx)
{
	return (rowIdx < GetTCount()) ? RemoveRow(&rowIdx) : -1;
}

int PPBillPacket::RemoveRow(uint * pRowIdx)
{
	const uint r = *pRowIdx;
	LongArray positions;
	if(ConstTI(r).IsRecomplete()) {
		//
		// Вместе со строкой рекомплектации удаляются все входящие строки,
		// поскольку они по определению являются зависимыми от строки рекомплектации.
		//
		PPTransferItem * p_ti;
		for(uint i = 0; EnumTItems(&i, &p_ti);)
			if(p_ti->Flags & PPTFR_PLUS)
				RemoveRow(--i);
		Rec.Flags &= ~BILLF_RECOMPLETE;
	}
	if(P_PckgList) {
		if(ConstTI(r).Flags & PPTFR_PCKG) {
			LPackage * p_pckg = P_PckgList->GetByIdx(r);
			if(p_pckg) {
				int    pii;
				for(uint i = 0; p_pckg->EnumItems(&i, &pii, 0);)
					if(ChkTIdx(pii) && ConstTI(pii).Flags & PPTFR_PCKGGEN)
						positions.insert(&pii);
				P_PckgList->RemoveByIdx(r);
				positions.insert(&r);
				positions.sort();
				for(int j = positions.getCount()-1; j >= 0; j--)
					RemoveRow(positions.at(j));
				*pRowIdx = (r > 0) ? (r-1) : 0;
				return 1;
			}
		}
		P_PckgList->ShiftIdx(r);
	}
	Lots.atFree(r);
	LTagL.RemovePosition(r);
	XcL.RemovePosition(r+1); // В XcL индексы [1..]
	if(P_QuotSetupInfoList) {
		uint i = P_QuotSetupInfoList->getCount();
		if(i) do {
			if(P_QuotSetupInfoList->at(--i).TiPos == r)
				P_QuotSetupInfoList->atFree(i);
		} while(i);
		if(P_QuotSetupInfoList->getCount() == 0)
			ZDELETE(P_QuotSetupInfoList);
	}
	*pRowIdx = (r > 0) ? (r-1) : 0;
	return 1;
}

int PPBillPacket::RemoveRows(LongArray * pPositions, int lowStop)
{
	if(pPositions) {
		for(int p = pPositions->getCount() - 1; p >= lowStop; p--) {
			if(!RemoveRow(pPositions->at(p)))
				return 0;
			pPositions->atFree(p);
		}
	}
	else {
		Rec.Flags &= ~BILLF_RECOMPLETE;
		Lots.clear();
		LTagL.Release();
		XcL.Release();
		ZDELETE(P_PckgList);
	}
	return 1;
}

int PPBillPacket::SetupItemQuotInfo(int itemNo, PPID quotKindID, double quotValue, long flags)
{
	int    ok = 1;
	if(itemNo >= 0 && itemNo < GetTCountI() && quotKindID) {
		if(SETIFZ(P_QuotSetupInfoList, new TSVector <QuotSetupInfoItem>)) {
			uint   p = 0;
			if(P_QuotSetupInfoList->lsearch(&itemNo, &p, CMPF_LONG)) {
				QuotSetupInfoItem & r_item = P_QuotSetupInfoList->at(p);
				r_item.QkID = quotKindID;
				r_item.Value = quotValue;
				r_item.Flags = flags;
			}
			else {
				QuotSetupInfoItem item;
				item.TiPos = (uint)itemNo;
				item.QkID = quotKindID;
				item.Value = quotValue;
				item.Flags = flags;
				P_QuotSetupInfoList->insert(&item);
			}
		}
		else
			ok = PPSetErrorNoMem();
	}
	else
		ok = 0;
	return ok;
}
//
//
//
PPBillExt::PPBillExt()
{
	THISZERO();
	OrderFulfillmentStatus = -1; // @useless
}

bool PPBillExt::IsEmpty() const
{ 
	return (AgentID || PayerID || InvoiceCode[0] || InvoiceDate || PaymBillCode[0] || PaymBillDate || 
		ExtPriceQuotKindID || CcID || GoodsGroupID || CliPsnCategoryID || TradePlanLocID) ? false : true; // @v12.1.12 TradePlanLocID
}

bool FASTCALL PPBillExt::IsEq(const PPBillExt & rS) const
{
	if(AgentID != rS.AgentID)
		return false;
	else if(PayerID != rS.PayerID)
		return false;
	else if(CcID != rS.CcID)
		return false;
	else if(GoodsGroupID != rS.GoodsGroupID) // @v11.0.11
		return false;
	else if(CliPsnCategoryID != rS.CliPsnCategoryID) // @v11.1.9
		return false;
	else if(stricmp866(InvoiceCode, rS.InvoiceCode) != 0)
		return false;
	else if(InvoiceDate != rS.InvoiceDate)
		return false;
	else if(stricmp866(PaymBillCode, rS.PaymBillCode) != 0)
		return false;
	else if(PaymBillDate != rS.PaymBillDate)
		return false;
	else if(ExtPriceQuotKindID != rS.ExtPriceQuotKindID)
		return false;
	else if(TradePlanLocID != rS.TradePlanLocID) // @v12.1.12
		return false;
	else
		return true;
}

int PPBillExt::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, PayerID, rBuf));
	THROW_SL(pCtx->Serialize(dir, AgentID, rBuf));
	THROW_SL(pCtx->Serialize(dir, InvoiceCode, sizeof(InvoiceCode), rBuf));
	THROW_SL(pCtx->Serialize(dir, InvoiceDate, rBuf));
	THROW_SL(pCtx->Serialize(dir, PaymBillCode, sizeof(PaymBillCode), rBuf));
	THROW_SL(pCtx->Serialize(dir, PaymBillDate, rBuf));
	THROW_SL(pCtx->Serialize(dir, ExtPriceQuotKindID, rBuf));
	// @v10.9.7 @todo CcID
	CATCHZOK
	return ok;
}

static double FASTCALL setsign(double v, int minus)
{
	v = fabs(v);
	return minus ? -v : v;
}
//
// PPBillPacketCollection
//
PPBillPacketCollection::PPBillPacketCollection() : TSCollection <PPBillPacket>()
{
}

PPBillPacketCollection::PPBillPacketCollection(const PPBillPacketCollection & rS) : TSCollection <PPBillPacket>()
{
	Copy(rS);
}

PPBillPacketCollection & FASTCALL PPBillPacketCollection::operator = (const PPBillPacketCollection & rS)
{
	Copy(rS);
	return *this;
}

bool FASTCALL PPBillPacketCollection::Copy(const PPBillPacketCollection & rS)
{
	return TSCollection_Copy(*this, rS);
}

PPBillPacket * PPBillPacketCollection::SearchByObject(PPID arID) const
{
	for(uint i = 0; i < getCount(); i++) {
		PPBillPacket * p_item = at(i);
		if(p_item && p_item->Rec.Object == arID)
			return p_item;
	}
	return 0;
}

const PPBillPacket * PPBillPacketCollection::Search_SrcIltiPos(uint srcIltiPos, uint * pTiIdx) const
{
	const PPBillPacket * p_result = 0;
	for(uint i = 0; !p_result && i < getCount(); i++) {
		const PPBillPacket * p_pack = at(i);
		if(p_pack) {
			for(uint tiidx = 0; !p_result && tiidx < p_pack->GetTCount(); tiidx++) {
				const PPTransferItem & r_ti = p_pack->ConstTI(tiidx);
				if(r_ti.SrcIltiPos == srcIltiPos) {
					p_result = p_pack;
					ASSIGN_PTR(pTiIdx, tiidx);
				}
			}
		}
	}
	return p_result;
}
// 
// PPBillPacket
//
PPBillPacket::TiItemExt::TiItemExt()
{
}

void PPBillPacket::TiItemExt::Clear()
{
	Clb.Z();
	Pckg.Z();
	MEMSZERO(LctRec);
	MergePosList.clear();
}

PPBillPacket::TiDifferenceItem::TiDifferenceItem(long flags, const LongArray * pThisPList, const LongArray * pOtherPList) :
	Flags(flags), ThisQtty(0.0), OtherQtty(0.0), ThisCost(0.0), OtherCost(0.0), ThisPrice(0.0), OtherPrice(0.0), ThisNetPrice(0.0), OtherNetPrice(0.0)
{
	RVALUEPTR(ThisPList, pThisPList);
	RVALUEPTR(OtherPList, pOtherPList);
}
//
//
//
void PPBillPacket::Helper_Init()
{
	ErrCause = 0; // @v11.3.0
	ErrLine = 0; // @v11.3.0
	OpTypeID = 0; // @v11.3.0
	AccSheetID = 0; // @v11.3.0
	Counter = 0; // @v11.3.0
	P_BObj = BillObj;
	P_ShLots = 0;
	P_ACPack = 0;
	P_LinkPack = 0;
	P_Outer  = 0;
	PaymBillID = 0;
	OrderPoolBillID = 0; // @v12.0.11
	OutAmtType = 0;
	CSessID = 0;
	SampleBillID = 0;
	P_PaymOrder = 0;
	P_Freight = 0;
	P_PckgList = 0;
	P_Iter = 0;
	P_GoodsGrpRestrict = 0;
	P_QuotSetupInfoList = 0;
	P_MirrorLTagL = 0;
	QuotKindID = AgtQuotKindID = 0;
	ProcessFlags = 0;
	SyncStatus = -2;
	Reserve = 0;
	LoadMoment = ZERODATETIME;
}

PPBillPacket::PPBillPacket() : PPBill()
{
	Helper_Init();
}

PPBillPacket::PPBillPacket(const PPBillPacket & rS) : PPBill()
{
	Helper_Init();
	Copy(rS);
}

PPBillPacket::~PPBillPacket()
{
	destroy();
}

void PPBillPacket::destroy()
{
	PPBill::BaseDestroy();
	RemoveRows(0);
	ZDELETE(P_ShLots);
	ZDELETE(P_ACPack);
	ZDELETE(P_LinkPack);
	ZDELETE(P_GoodsGrpRestrict);
	ZDELETE(P_QuotSetupInfoList);
	CipB.destroy();
	QuotKindID = 0;
	P_Outer = 0;
	ZDELETE(P_MirrorLTagL);
	PaymBillID = 0;
	OrderPoolBillID = 0; // @v12.0.11
	CSessID = 0;
	SampleBillID = 0;
	ZDELETE(P_PckgList);
	ZDELETE(P_Iter);
	LnkFiles.freeAll();
	ErrCause = ErrLine = 0;
	TiErrList.freeAll();
	ProcessFlags = 0;
	SyncStatus = -2;
	LoadMoment = ZERODATETIME;
	OpTypeID = 0;
	AccSheetID = 0;
	InvList.freeAll();
}

PPBillPacket & FASTCALL PPBillPacket::operator = (const PPBillPacket & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL PPBillPacket::Copy(const PPBillPacket & rS)
{
	int    ok = 1;
	destroy();
	PPBill::Copy((PPBill &)rS);
#define _FLD(f) f = rS.f
	_FLD(ErrCause);
	_FLD(ErrLine);
	_FLD(TiErrList);
	_FLD(OutAmtType);
	_FLD(QuotKindID);
	_FLD(AgtQuotKindID);
	_FLD(OpTypeID);
	_FLD(AccSheetID);
	_FLD(Counter);
	_FLD(PaymBillID);
	_FLD(OrderPoolBillID); // @v12.0.11
	_FLD(CSessID);
	_FLD(SampleBillID);
	_FLD(ProcessFlags);
	_FLD(LnkFiles);
	_FLD(Lots);
	_FLD(SyncStatus);
	_FLD(Reserve);
#undef _FLD
	if(rS.P_ShLots) {
		THROW_MEM(P_ShLots = new PPTrfrArray(*rS.P_ShLots));
	}
	if(rS.P_ACPack) {
		THROW_MEM(P_ACPack = new PPBillPacket(*rS.P_ACPack));
	}
	if(rS.P_LinkPack) {
		THROW_MEM(P_LinkPack = new PPBillPacket(*rS.P_LinkPack));
	}
	if(rS.P_PckgList) {
		THROW_MEM(P_PckgList = new LPackageList);
		for(uint i = 0; i < rS.P_PckgList->getCount(); i++) {
			P_PckgList->insert(rS.P_PckgList->at(i));
		}
	}
	if(rS.P_GoodsGrpRestrict) {
		THROW_MEM(P_GoodsGrpRestrict = new LAssocArray(*rS.P_GoodsGrpRestrict));
	}
	if(rS.P_QuotSetupInfoList) {
		THROW_MEM(P_QuotSetupInfoList = new TSVector <QuotSetupInfoItem> (*rS.P_QuotSetupInfoList));
	}
	CipB = rS.CipB;
	if(rS.P_MirrorLTagL) {
		P_MirrorLTagL = new PPLotTagContainer;
		*P_MirrorLTagL = *rS.P_MirrorLTagL;
	}
	InvList = rS.InvList;
	CATCHZOK
	return ok;
}

int PPBillPacket::SerializeLots(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir < 0)
		Lots.freeAll();
	int32  c = Lots.getCount(); // @persistent
	THROW_SL(pSCtx->Serialize(dir, c, rBuf));
	for(int32 i = 0; i < c; i++) {
		PPTransferItem item;
		if(dir > 0)
			item = Lots.at(i);
		THROW_SL(pSCtx->Serialize(dir, item.Date, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.BillID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.RByBill, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.SrcIltiPos, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.CurID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.LocID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.GoodsID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.LotID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.CorrLoc, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.OrdLotID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.UnitPerPack, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Quantity_, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.WtQtty, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.RevalCost, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Rest_, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Cost, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.ExtCost, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Price, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Discount, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.CurPrice, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.QuotPrice, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.LotTaxGrpID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.QCert, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Suppl, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Flags, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.TFlags, rBuf));
		THROW_SL(pSCtx->Serialize(dir, item.Expiry, rBuf));
		if(dir < 0)
			THROW_SL(Lots.insert(&item));
	}
	CATCHZOK
	return ok;
}

bool PPBillPacket::IsDraft() const
{
	const  PPID op_type_id = OpTypeID;
	return oneof4(op_type_id, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ);
}

bool PPBillPacket::IsGoodsDetail() const
{
	const  PPID op_type_id = OpTypeID;
	return oneof12(op_type_id, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ, PPOPT_GOODSRECEIPT, 
		PPOPT_GOODSEXPEND, PPOPT_GOODSREVAL, PPOPT_CORRECTION, PPOPT_GOODSACK, PPOPT_GOODSMODIF, PPOPT_GOODSRETURN, PPOPT_GOODSORDER);
}

int PPBillPacket::UngetCounter()
{
	int    ok = -1;
	if(!(CConfig.Flags & CCFLG_DONTUNDOOPCNTRONESC) && Counter) {
		PPOprKind op_rec;
		if(GetOpData(Rec.OpID, &op_rec) > 0) {
			PPObjOpCounter opc_obj;
			ok = opc_obj.UngetCounter(op_rec.OpCounterID, Counter, Rec.LocID, 1);
		}
	}
	return ok;
}

int PPBillPacket::AddShadowItem(const PPTransferItem * pOrdItem, uint * pPos)
{
	int    ok = 0;
	uint   p = 0;
	PPTransferItem sti;
	sti.InitShadow(&Rec, pOrdItem);
	if(AddShadowItem(&sti) && P_ShLots->getCount()) {
		p = P_ShLots->getCount()-1;
		ok = 1;
	}
	ASSIGN_PTR(pPos, p);
	return ok;
}

int FASTCALL PPBillPacket::AddShadowItem(const PPTransferItem * pTI)
	{ return SETIFZ(P_ShLots, new PPTrfrArray) ? (P_ShLots->insert(pTI) ? 1 : PPSetErrorSLib()) : PPSetErrorNoMem(); }
int PPBillPacket::CreateBlank(PPID opID, PPID linkBillID, PPID locID, int use_ta)
	{ return _CreateBlank(opID, linkBillID, locID, 0, use_ta); }
int PPBillPacket::CreateBlank_WithoutCode(PPID opID, PPID linkBillID, PPID locID, int use_ta)
	{ return _CreateBlank(opID, linkBillID, locID, 1, use_ta); }

int PPBillPacket::CreateBlank2(PPID opID, LDATE dt, PPID locID, int use_ta)
{
	int    r = _CreateBlank(opID, 0, locID, 0, use_ta);
	if(r) {
		if(dt)
			Rec.Dt = dt;
		if(locID)
			Rec.LocID = locID;
	}
	return r;
}

int PPBillPacket::CreateBlankByFilt(PPID opID, const BillFilt * pFilt, int use_ta)
{
	int    ok = 1;
	PPID   single_loc_id = pFilt->LocList.GetSingle();
	PPOprKind op_rec;
	THROW(_CreateBlank(NZOR(opID, pFilt->OpID), 0L, single_loc_id, 0, use_ta));
	if(pFilt->Period.upp)
		Rec.Dt = pFilt->Period.upp;
	if(Rec.OpID && GetOpData(Rec.OpID, &op_rec) > 0) {
		PPID acc_sheet_id = 0;
		if(pFilt->ObjectID && GetArticleSheetID(pFilt->ObjectID, &acc_sheet_id) > 0)
			if(acc_sheet_id == op_rec.AccSheetID)
				Rec.Object = pFilt->ObjectID;
		if(pFilt->Object2ID && GetArticleSheetID(pFilt->Object2ID, &acc_sheet_id) > 0)
			if(acc_sheet_id == op_rec.AccSheet2ID)
				Rec.Object2 = pFilt->Object2ID;
	}
	if(single_loc_id)
		Rec.LocID = single_loc_id;
	if(!(pFilt->Flags & BillFilt::fAllCurrencies))
		Rec.CurID = pFilt->CurID;
	Ext.PayerID = pFilt->PayerID;
	Ext.AgentID = pFilt->AgentID;
	if(oneof2(OpTypeID, PPOPT_ACCTURN, PPOPT_PAYMENT))
		if(pFilt->AmtRange.low == pFilt->AmtRange.upp && pFilt->AmtRange.low > 0)
			Rec.Amount = pFilt->AmtRange.low;
	CATCHZOK
	return ok;
}

int PPBillPacket::CreateBlankBySample(PPID sampleBillID, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   op_type_id = 0;
	BillTbl::Rec rec;
	THROW(P_BObj->Search(sampleBillID, &rec) > 0);
	THROW(_CreateBlank(rec.OpID, rec.LinkBillID, rec.LocID, 0, use_ta));
	Rec.Dt       = rec.Dt;
	// @todo use SetupObject(rec.Object) insteed {Rec.Object=rec.Object}
	Rec.Object   = rec.Object;
	Rec.Object2  = rec.Object2;
	Rec.LocID    = rec.LocID;
	Rec.CurID    = rec.CurID;
	op_type_id = GetOpType(Rec.OpID);
	switch(op_type_id) {
		case PPOPT_ACCTURN:
		case PPOPT_PAYMENT:
			Rec.Amount = BR2(rec.Amount);
			if(rec.Flags & BILLF_BANKING) {
				PPBankingOrder paym_order;
				if(p_ref->GetProperty(PPOBJ_BILL, sampleBillID, BILLPRP_PAYMORDER, &paym_order, sizeof(paym_order)) > 0) {
					THROW_MEM(P_PaymOrder = new PPBankingOrder);
					*P_PaymOrder = paym_order;
					Rec.Flags |= BILLF_BANKING;
				}
			}
			break;
		case PPOPT_AGREEMENT:
			{
				ZDELETE(P_Agt);
				PPBill::Agreement agt;
				if(p_ref->GetProperty(PPOBJ_BILL, sampleBillID, BILLPRP_AGREEMENT, &agt, sizeof(agt)) > 0 && !agt.IsEmpty()) {
					THROW_MEM(P_Agt = new PPBill::Agreement(agt));
				}
			}
			break;
		case PPOPT_DRAFTRECEIPT:
		case PPOPT_DRAFTEXPEND:
		case PPOPT_DRAFTTRANSIT:
			P_BObj->P_CpTrfr->LoadItems(rec.ID, this, 0);
			{
				//
				// Проверка на выполнение ограничений по товарам для документа
				//
				uint i = GetTCount();
				if(i) {
					PPObjGoods goods_obj;
					do {
						const PPTransferItem & r_ti = ConstTI(--i);
						if(!CheckGoodsForRestrictions((int)i, r_ti.GoodsID, TISIGN_UNDEF, r_ti.Qtty(), PPBillPacket::cgrfAll, 0))
							RemoveRow(i);
					} while(i && GetTCount()); // Теоретически, RemoveRow может удалить несколько строк
						// из таблицы Lots. Поэтому, для страховки проверяем GetTCount().
				}
			}
			break;
		case PPOPT_GOODSRECEIPT:
		case PPOPT_GOODSEXPEND:
			{
				PPFreight freight;
				PPBillExt extra_data;
				if(P_BObj->P_Tbl->GetFreight(sampleBillID, &freight) > 0)
					SetFreight(&freight);
				if(P_BObj->P_Tbl->GetExtraData(sampleBillID, &extra_data) > 0)
					Ext.AgentID = extra_data.AgentID;
			}
			break;
	}
	// @v11.1.12 STRNSCPY(Rec.Memo, rec.Memo);
	P_BObj->P_Tbl->GetItemMemo(sampleBillID, SMemo); // @v11.1.12
	CATCHZOK
	return ok;
}

int PPBillPacket::_CreateBlank(PPID opID, PPID linkBillID, PPID locID, int dontInitCode, int use_ta /*=1*/)
{
	int    ok = 1;
	int    r;
	const PPConfig & r_cfg = LConfig;
	PPOprKind op_rec;
	destroy();
	if(opID) {
		PPID   op_counter_id = 0;
		// @v10.2.3 THROW(ObjRts.CheckOpID(opID, PPR_INS));
		THROW(BillObj->CheckRightsWithOp(opID, PPR_INS)); // @v10.2.3
		if(opID == PPOPK_CASHSESS)
			op_rec.OpTypeID = PPOPT_CASHSESS;
		else {
			THROW(GetOpData(opID, &op_rec) > 0);
			if(!dontInitCode)
				op_counter_id = op_rec.OpCounterID;
			if(op_rec.Flags & OPKF_AUTOWL)
				Rec.Flags |= BILLF_WHITELABEL;
		}
		Rec.StatusID = NZOR(op_rec.InitStatusID, P_BObj->Cfg.InitStatusID);
		if(Rec.StatusID) {
			PPObjBillStatus bs_obj;
			PPBillStatus bs_rec;
			if(bs_obj.Fetch(Rec.StatusID, &bs_rec) > 0) {
				if(bs_rec.RestrictOpID)
					THROW_PP_S(IsOpBelongTo(opID, bs_rec.RestrictOpID), PPERR_INITBILLSTATUSRESTRICTOP, bs_rec.Name);
				if(bs_rec.CounterID && op_counter_id) // Если op_counter_id == 0, то не следует присваивать код документу (см. выше)
					op_counter_id = bs_rec.CounterID;
				if(bs_rec.Flags & BILSTF_LOCK_ACCTURN)
					Rec.Flags |= BILLF_NOATURN;
			}
		}
		if(op_counter_id) {
			//
			// @v11.7.8 Введена проверка на дублирование сформированного кода документа.
			//
			const bool global_check_dup_code = LOGIC(CConfig.Flags & CCFLG_CHECKUNIQBILLCODE);
			PPObjOpCounter opc_obj;
			PPOpCounter opc_rec;
			const  PPID loc_id = NZOR(locID, r_cfg.Location);
			// @v11.9.2 Добавлено условие для проверки уникальности номера: в счетчике операций должен стоять флаг OPCNTF_VERIFYUNIQ
			const bool _check_dup_code = (global_check_dup_code && opc_obj.Search(op_counter_id, &opc_rec) > 0 && opc_rec.Flags & OPCNTF_VERIFYUNIQ);
			if(_check_dup_code) {
				PPObjBill * p_bobj = BillObj;
				//
				// Так как код документа формируется по заданному человеком шаблону, то
				// не исключена возможность того, что при каждой новой итерации счетчика
				// номер будет оставаться один и тот же. Для защиты от этого будем сохранять
				// предыдущий сгенерированный код и, если следующая итерация сгенерирует
				// такой же код, то остановим цикл.
				// Другой потенциальной причиной зацикливания может быть сбой в базе данных.
				//
				SString prev_code;
				PPTransaction tra(use_ta);
				THROW(tra);
				for(bool done = false; !done;) {
					THROW(opc_obj.GetCode(op_counter_id, &Counter, Rec.Code, sizeof(Rec.Code), loc_id, 0));
					if(prev_code == Rec.Code) {
						done = true; // We are in the potentially infinite loop: exit!
					}
					else {
						prev_code = Rec.Code;
						BillTbl::Rec dup_bill_rec;
						const int sbcr = p_bobj ? p_bobj->P_Tbl->SearchByCode(Rec.Code, opID, ZERODATE, &dup_bill_rec) : -1;
						THROW(sbcr);
						if(sbcr > 0) {
							; // continue loop. @todo @logmessage
						}
						else
							done = true;
					}
				}
				THROW(tra.Commit());
			}
			else {
				THROW(opc_obj.GetCode(op_counter_id, &Counter, Rec.Code, sizeof(Rec.Code), loc_id, use_ta));
			}
		}
	}
	else // Теневой документ
		op_rec.LinkOpID = 1;
	OpTypeID  = op_rec.OpTypeID;
	AccSheetID = op_rec.AccSheetID;
	ErrCause = 0;
	ErrLine  = 0;
	TiErrList.clear();
	Rec.OpID = opID;
	Rec.Dt   = getcurdate_(); // @v10.8.10 LConfig.OperDate-->getcurdate_()
	if(r_cfg.Cash && oneof2(opID, GetCashOp(), GetCashRetOp())) {
		Rec.UserID = r_cfg.Cash;
		Rec.Flags |= (BILLF_CASH | BILLF_NOATURN);
	}
	else
		Rec.UserID = r_cfg.UserID;
	Rec.LocID = NZOR(locID, r_cfg.Location);
	Rec.CurID = 0L;
	Rec.LinkBillID = linkBillID;
	if(op_rec.Flags & OPKF_NEEDPAYMENT)
		Rec.Flags |= BILLF_NEEDPAYMENT;
	else if(!(op_rec.Flags & OPKF_RECKON)) {
		// Зачетные платежи изначально не должны иметь признак "Оплачен", даже если не требуют явной оплаты
		Rec.Flags |= BILLF_PAYOUT;
	}
	if(opID) {
		if((r = IsExpendOp(opID)) > 0)
			Rec.Flags |= BILLF_GEXPEND;
		else if(!r)
			Rec.Flags |= BILLF_GRECEIPT;
		if(OpTypeID == PPOPT_GOODSRECEIPT) {
			if(AccSheetID == 0) {
				PPObjArticle arobj;
				THROW(arobj.GetMainOrgAsSuppl(&Rec.Object, 0, use_ta));
			}
		}
		if(op_rec.ExtFlags & OPKFX_AUTOGENUUID)
			GenerateGuid(0);
	}
	else // Теневой документ
		Rec.Flags |= BILLF_GEXPEND;
	if(oneof2(OpTypeID, PPOPT_GOODSREVAL, PPOPT_CORRECTION))
		Rec.Flags |= BILLF_GREVAL;
	else if(OpTypeID == PPOPT_GOODSMODIF)
		Rec.Flags |= BILLF_GMODIF;
	else if(OpTypeID == PPOPT_ACCTURN) {
		if(op_rec.SubType == OPSUBT_ADVANCEREP) {
			THROW_MEM(P_AdvRep = new PPAdvanceRep);
			memzero(P_AdvRep, sizeof(*P_AdvRep));
			Rec.Flags |= BILLF_ADVANCEREP;
		}
	}
	if(OpTypeID == PPOPT_AGREEMENT) {
		P_Agt = new Agreement;
	}
	if(Rec.LinkBillID) {
		SString msg_buf;
		BillTbl::Rec link_rec;
		PPFreight freight;
		THROW_PP_S(P_BObj->Search(Rec.LinkBillID, &link_rec) > 0, PPERR_LINKBILLNFOUND, msg_buf.Z().Cat(Rec.LinkBillID));
		if(OpTypeID == PPOPT_CORRECTION /* && GetOpType(link_rec.OpID) == PPOPT_GOODSEXPEND */) {
			THROW_MEM(SETIFZ(P_LinkPack, new PPBillPacket));
			P_LinkPack->destroy();
			THROW(P_BObj->ExtractPacket(Rec.LinkBillID, P_LinkPack) > 0);
		}
		//
		// Строго говоря, для инвентаризации (PPOPT_INVENTORY) и драфт-документом необходимо
		// проверять, чтобы вид операции создаваемого документа соответствовал операции списания.
		// Однако, пока это не делаем во избежании конфликтов в эксплуатируемых базах данных.
		//
		if(op_rec.LinkOpID) {
			THROW_PP(!opID || op_rec.LinkOpID == link_rec.OpID || IsDraftOp(link_rec.OpID) || GetOpType(link_rec.OpID) == PPOPT_INVENTORY, PPERR_LINKBILLOPR);
		}
		//
		// Если операция, имеющая связанную операцию, не определяет
		// собственной аналитической таблицы, то она наследует ее из
		// связанной операции.
		// Начиная с v1.9.2 операция может иметь таблицу, отличную
		// от таблицы, определенной для связанной операции.
		//
		THROW(GetOpData(link_rec.OpID, &op_rec));
		SETIFZ(AccSheetID, op_rec.AccSheetID);
		//
		// Если таблица связанного документа совпадает с таблицей
		// этого документа (AccSheet == 0), тип операции является оплатой
		// и связанный документ имеет плательщика, то по умолчанию
		// объект этого документа равен плательщику связанного документа.
		//
		// Если имеет место совпадение таблиц связанного и этого документа,
		// то по умолчанию объект этого документа равен объекту связанного.
		//
		// В прочих случаях объект этого документа нулевой.
		//
		if(opID && AccSheetID == op_rec.AccSheetID) {
			PPID   payer_id = 0;
			if(link_rec.Flags & BILLF_EXTRA) {
				PPBillExt ext;
				THROW(r = P_BObj->P_Tbl->GetExtraData(link_rec.ID, &ext));
				if(r > 0)
					payer_id = ext.PayerID;
			}
			Rec.Object = (OpTypeID == PPOPT_PAYMENT && payer_id) ? payer_id : link_rec.Object;
		}
		else
			Rec.Object = 0;
		Rec.CurID = link_rec.CurID;
		if(OpTypeID == PPOPT_GOODSRETURN && op_rec.Flags & OPKF_FREIGHT && P_BObj->GetConfig().Flags & BCF_RETINHERITFREIGHT) {
			if(P_BObj->P_Tbl->GetFreight(link_rec.ID, &freight) > 0) {
				freight.IssueDate = ZERODATE;
				freight.ArrivalDate = ZERODATE;
				freight.Cost = 0.0;
				freight.ID = 0;
				freight.ShipID = 0;
				freight.CaptainID = 0;
				if(freight.PortOfDischarge || freight.PortOfLoading)
					SExchange(&freight.PortOfDischarge, &freight.PortOfLoading);
				SetFreight(&freight);
			}
		}
		//
		// Для теневого документа поле Object должно содержать ссылку на
		// документ заказа. Это придется сделать после вызова CreateBlank
		//
		if(OpTypeID != PPOPT_GOODSRETURN || Rec.LocID == 0)
			Rec.LocID = link_rec.LocID;
	}
	CATCHZOK
	return ok;
}

void PPBillPacket::CreateAccTurn(PPAccTurn & rAt) const
{
	memzero(&rAt, sizeof(rAt));
	memcpy(rAt.BillCode, Rec.Code, sizeof(rAt.BillCode));
	rAt.Date   = Rec.Dt;
	rAt.BillID = Rec.ID;
	rAt.Opr    = Rec.OpID;
	PPOprKind op_rec;
	if(GetOpData(Rec.OpID, &op_rec) > 0) {
		if(op_rec.SubType == OPSUBT_REGISTER)
			rAt.Flags |= PPAF_REGISTER;
		else
			SETFLAG(rAt.Flags, PPAF_OUTBAL, CheckOpFlags(Rec.OpID, OPKF_OUTBALACCTURN));
	}
}

int PPBillPacket::SetCurTransit(const PPCurTransit * pTrans)
{
	int    ok = 1;
	PPOprKind op_rec;
	double scale;
	double in_tran_crate;
	double out_tran_crate;
	THROW_PP(pTrans->InCurID != pTrans->OutCurID, PPERR_EQCTCUR);
	THROW_PP(pTrans->InCurID != 0,  PPERR_UNDEFCTINCUR);
	THROW_PP(pTrans->OutCurID != 0, PPERR_UNDEFCTOUTCUR);
	THROW_PP(pTrans->InCurAmount  > 0, PPERR_INVCTINAMT);
	THROW_PP(pTrans->OutCurAmount > 0, PPERR_INVCTOUTAMT);
	THROW_PP(pTrans->InCRate  > 0, PPERR_INVCRATE);
	THROW_PP(pTrans->OutCRate > 0, PPERR_INVCRATE);
	THROW_PP(pTrans->TransitCRate > 0, PPERR_INVCRATE);
	GetOpData(pTrans->OpID, &op_rec);
	Rec.ID      = pTrans->BillID;
	STRNSCPY(Rec.Code, pTrans->BillCode);
	Rec.Dt      = pTrans->Date;
	Rec.OpID    = pTrans->OpID;
	Rec.Object  = pTrans->ObjectID;
	Rec.Flags   = pTrans->Flags;
	// @v11.1.12 STRNSCPY(Rec.Memo, pTrans->Memo);
	SMemo = pTrans->Memo; // @v11.1.12
	Rec.CRate   = pTrans->TransitCRate;
	if(op_rec.Flags & OPKF_SELLING) {
		Rec.CurID  = pTrans->OutCurID;
		Rec.Amount = BR2(pTrans->OutCurAmount);
	}
	else {
		Rec.CurID = pTrans->InCurID;
		Rec.Amount = BR2(pTrans->InCurAmount);
	}
	Amounts.Remove(PPAMT_MAIN, -1L);
	Amounts.Remove(PPAMT_BUYING, -1L);
	Amounts.Remove(PPAMT_SELLING, -1L);
	Amounts.Remove(PPAMT_TRANSITCRATE, -1L);
	Amounts.Remove(PPAMT_CRATE, pTrans->InCurID);
	Amounts.Remove(PPAMT_CRATE, pTrans->OutCurID);

	Amounts.Put(PPAMT_MAIN,    Rec.CurID, Rec.Amount, 0, 1);
	Amounts.Put(PPAMT_BUYING,  pTrans->InCurID,  pTrans->InCurAmount, 0, 1);
	Amounts.Put(PPAMT_SELLING, pTrans->OutCurID, pTrans->OutCurAmount, 0, 1);
	Amounts.Put(PPAMT_TRANSITCRATE, 0L, pTrans->TransitCRate, 0, 1);
	Amounts.Put(PPAMT_CRATE, pTrans->InCurID,  pTrans->InCRate,  0, 1);
	Amounts.Put(PPAMT_CRATE, pTrans->OutCurID, pTrans->OutCRate, 0, 1);

	scale = pTrans->TransitCRate / (pTrans->InCRate / pTrans->OutCRate);
	in_tran_crate  = (pTrans->InCRate  != 1) ? R6(pTrans->InCRate  * scale) : 1;
	out_tran_crate = (pTrans->OutCRate != 1) ? R6(pTrans->OutCRate * scale) : 1;
	Amounts.Put(PPAMT_TRANSITCRATE, pTrans->InCurID,  in_tran_crate,  0, 1);
	Amounts.Put(PPAMT_TRANSITCRATE, pTrans->OutCurID, out_tran_crate, 0, 1);
	CATCHZOK
	return ok;
}

int PPBillPacket::GetCurTransit(PPCurTransit * pTrans) const
{
	int    ok = 1;
	PPIDArray cur_list;
	memzero(pTrans, sizeof(*pTrans));
	pTrans->BillID     = Rec.ID;
	pTrans->Date       = Rec.Dt;
	pTrans->OpID       = Rec.OpID;
	pTrans->ObjectID   = Rec.Object;
	pTrans->Flags      = Rec.Flags;
	pTrans->AccSheetID = AccSheetID;
	STRNSCPY(pTrans->BillCode, Rec.Code);
	// @v11.1.12 STRNSCPY(pTrans->Memo, Rec.Memo);
	STRNSCPY(pTrans->Memo, SMemo); // @v11.1.12
	pTrans->TransitCRate = Rec.CRate;
	Amounts.GetCurList(PPAMT_BUYING, &cur_list);
	if(Rec.ID) {
		THROW_PP(cur_list.getCount(), PPERR_UNDEFCTINCUR);
		THROW_PP(cur_list.getCount() == 1, PPERR_DUPCTINCUR);
		THROW_PP((pTrans->InCurID = cur_list.at(0)) != 0, PPERR_UNDEFCTINCUR);
	}
	else if(cur_list.getCount() == 1)
		pTrans->InCurID = cur_list.at(0);
	pTrans->InCurAmount = Amounts.Get(PPAMT_BUYING, pTrans->InCurID);
	pTrans->InCRate     = Amounts.Get(PPAMT_CRATE,  pTrans->InCurID);

	cur_list.freeAll();
	Amounts.GetCurList(PPAMT_SELLING, &cur_list);
	if(Rec.ID) {
		THROW_PP(cur_list.getCount(), PPERR_UNDEFCTOUTCUR);
		THROW_PP(cur_list.getCount() == 1, PPERR_DUPCTOUTCUR);
		THROW_PP((pTrans->OutCurID = cur_list.at(0)) != 0, PPERR_UNDEFCTOUTCUR);
	}
	else if(cur_list.getCount() == 1)
		pTrans->OutCurID = cur_list.at(0);
	pTrans->OutCurAmount = Amounts.Get(PPAMT_SELLING, pTrans->OutCurID);
	pTrans->OutCRate     = Amounts.Get(PPAMT_CRATE, pTrans->OutCurID);
	CATCHZOK
	return ok;
}

int FASTCALL PPBillPacket::GetOrderList(PPIDArray & rList) const
{
	rList.clear();
	int    ok = -1;
	if(P_ShLots) {
		for(uint i = 0; i < P_ShLots->getCount(); i++) {
			rList.add(P_ShLots->at(i).BillID);
			ok = 1;
		}
		rList.sortAndUndup();
	}
	return ok;
}

int PPBillPacket::GetSyncStatus()
{
	if(SyncStatus == -2 && Rec.ID) {
		ObjSyncCore * p_osc = DS.GetTLA().P_ObjSync;
		if(p_osc)
			SyncStatus = p_osc->GetSyncStatus(PPOBJ_BILL, Rec.ID, 0, 0);
	}
	return SyncStatus;
}

int FASTCALL PPBillPacket::SetTPointer(int pos)
{
	if((pos >= 0 && pos < (int)Lots.getCount()) || pos == -1) {
		Lots.setPointer(static_cast<uint>(pos));
		return 1;
	}
	else
		return 0;
}

static IMPL_CMPFUNC(PPTransferItem_RByBill, p1, p2)
{
	const PPTransferItem * p_ti1 = static_cast<const PPTransferItem *>(p1);
	const PPTransferItem * p_ti2 = static_cast<const PPTransferItem *>(p2);
	return CMPSIGN(p_ti1->RByBill, p_ti2->RByBill);
}

void  PPBillPacket::SortTI() { Lots.sort(PTR_CMPFUNC(PPTransferItem_RByBill)); }
int   FASTCALL PPBillPacket::EnumTItems(uint * pI, PPTransferItem ** ppTI) const { return Lots.enumItems(pI, (void **)ppTI); }
uint  PPBillPacket::GetTCount() const { return Lots.getCount(); }
int   PPBillPacket::GetTCountI() const { return Lots.getCountI(); }
int   PPBillPacket::GetTPointer() const { return static_cast<int>(Lots.getPointer()); }
PPTransferItem & FASTCALL PPBillPacket::TI(uint p) const { return Lots.at(p); }
const PPTransferItem & FASTCALL PPBillPacket::ConstTI(uint p) const { return Lots.at(p); }
const PPTrfrArray & PPBillPacket::GetLots() const { return Lots; }
void  PPBillPacket::SetLots(const PPTrfrArray & rS) { Lots = rS; }
int   FASTCALL PPBillPacket::ChkTIdx(int idx) const { return (idx >= 0 && idx < (int)Lots.getCount()) ? 1 : PPSetError(PPERR_INVTIIDX); }

bool FASTCALL PPBillPacket::SearchTI(int rByBill, uint * pPos) const
{
	bool   ok = false;
	uint   pos = 0;
	if(rByBill > 0) {
		for(uint i = 0; !ok && i < Lots.getCount(); i++) {
			if(Lots.at(i).RByBill == rByBill) {
				pos = i;
				ok = true;
			}
		}
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

PPID PPBillPacket::GetPrefSupplForTi(uint tiIdx/*0..*/) const
{
	// int    FetchTag(PPID objID, PPID tagID, ObjTagItem * pItem);
	const  ObjTagItem * p_tag = LTagL.GetTag(tiIdx, PPTAG_LOT_PREFSUPPL);
	int    tag_int_val = 0;
	return (p_tag && p_tag->GetInt(&tag_int_val)) ? tag_int_val : 0;
}

int PPBillPacket::SetupPrefSupplForTi(uint tiIdx/*0..*/, PPID supplID)
{
	int    ok = -1;
	if(tiIdx < GetTCount()) {
		PPTransferItem & r_ti = TI(tiIdx);
		if(supplID) {
			ObjTagItem tag_item;
			if(tag_item.Init(PPTAG_LOT_PREFSUPPL)) {
				PPID   org_suppl_id = 0;
				ObjTagList * p_org_tag_list = LTagL.Get(tiIdx);
				ObjTagList tag_list;
				if(p_org_tag_list)
					tag_list = *p_org_tag_list;
				{
					const ObjTagItem * p_tag_item = tag_list.GetItem(PPTAG_LOT_PREFSUPPL);
					if(p_tag_item && p_tag_item->GetInt(&org_suppl_id) > 0) {
						;
					}
				}
				if(supplID != org_suppl_id) {
					tag_item.SetInt(PPTAG_LOT_PREFSUPPL, supplID);
					tag_list.PutItem(PPTAG_LOT_PREFSUPPL, &tag_item);
					LTagL.Set(tiIdx, &tag_list);
					{ // @v12.1.2 
						const QuotIdent suppl_deal_qi(Rec.Dt, Rec.LocID, 0, 0, supplID);
						PPSupplDeal sd;
						PPObjGoods goods_obj;
						goods_obj.GetSupplDeal(labs(r_ti.GoodsID), suppl_deal_qi, &sd, 1);
						r_ti.Cost = sd.Cost;
					}
					ok = 1;
				}
				else
					ok = -1;
			}
		}
		else {
			r_ti.Cost = 0.0;
		}
	}
	else
		ok = 0;
	return ok;
}

int PPBillPacket::LoadTItem(const PPTransferItem * pItem, const char * pClb, const char * pSerial)
{
	int    ok = 1;
	uint   pos = Lots.getCount();
	THROW_SL(Lots.insert(pItem));
	if(pClb)
		LTagL.SetString(PPTAG_LOT_CLB, pos, pClb);
	if(pSerial)
		LTagL.SetString(PPTAG_LOT_SN, pos, pSerial);
	CATCHZOK
	return ok;
}

void FASTCALL PPBillPacket::SetQuantitySign(int minus)
{
	PPTransferItem * p_ti;
	for(uint i = 0; EnumTItems(&i, &p_ti);) {
		if(minus == -1)
			p_ti->SetupSign(Rec.OpID);
		else {
			p_ti->Quantity_ = setsign(p_ti->Quantity_, minus);
			if(p_ti->Flags & PPTFR_INDEPPHQTTY)
				p_ti->WtQtty = setsign(p_ti->WtQtty, minus);
		}
	}
}

/*static*/PPBillPacket::PoolKind PPBillPacket::ObjAssocToPoolKind(PPID assocID)
{
	switch(assocID) {
		case PPASS_PAYMBILLPOOL: return bpkReckon;
		case PPASS_OPBILLPOOL: return bpkOpBill;
		case PPASS_CSESSBILLPOOL: return bpkCSess;
		case PPASS_TSESSBILLPOOL: return bpkTSess;
		case PPASS_CSDBILLPOOL: return bpkCSessDfct;
		case PPASS_TODOBILLPOOL: return bpkTodo;
		case PPASS_PRJBILLPOOL: return bpkPrj;
		case PPASS_PRJPHASEBILLPOOL: return bpkPrjPhase;
		case PPASS_TSDBILLPOOL: return bpkTSessDfct;
		case PPASS_ORDACCOMPBILLPOOL: return bpkOrdAccomplish; // @v12.0.11
	}
	return bpkNone;
}

/*static*/PPID PPBillPacket::PoolKindToObjAssoc(PPBillPacket::PoolKind kind)
{
	switch(kind) {
		case bpkReckon: return PPASS_PAYMBILLPOOL;
		case bpkOpBill: return PPASS_OPBILLPOOL;
		case bpkCSess: return PPASS_CSESSBILLPOOL;
		case bpkTSess: return PPASS_TSESSBILLPOOL;
		case bpkCSessDfct: return PPASS_CSDBILLPOOL;
		case bpkTodo: return PPASS_TODOBILLPOOL;
		case bpkPrj: return PPASS_PRJBILLPOOL;
		case bpkPrjPhase: return PPASS_PRJPHASEBILLPOOL;
		case bpkTSessDfct: return PPASS_TSDBILLPOOL;
		case bpkOrdAccomplish: return PPASS_ORDACCOMPBILLPOOL; // @v12.0.11
	}
	return 0;
}

void PPBillPacket::SetPoolMembership(PoolKind poolKind, PPID poolID)
{
	switch(poolKind) {
		case bpkReckon:
			PaymBillID = poolID;
			break;
		case bpkCSess:
			SETFLAG(Rec.Flags, BILLF_CSESSWROFF, poolID);
			CSessID = poolID;
			break;
		case bpkCSessDfct:
			SETFLAG(Rec.Flags, BILLF_CDFCTWROFF, poolID);
			CSessID = poolID;
			break;
		case bpkTSess:
			SETFLAG(Rec.Flags, BILLF_TSESSWROFF, poolID);
			CSessID = poolID;
			break;
		case bpkTSessDfct:
			SETFLAG(Rec.Flags, BILLF_TDFCTWROFF, poolID);
			CSessID = poolID;
			break;
		case bpkTSessPaym:
			SETFLAG(Rec.Flags2, BILLF2_TSESSPAYM, poolID);
			CSessID = poolID;
			break;
		case bpkOrdAccomplish: // @v12.0.11 @construction
			OrderPoolBillID = poolID;
			break;
	}
}

bool PPBillPacket::HasIndepModifPlus() const
{
	if(Rec.Flags & BILLF_GMODIF) {
		PPTransferItem * p_ti;
		for(uint i = 0; EnumTItems(&i, &p_ti);)
			if(p_ti->Flags & PPTFR_PLUS && p_ti->IsReceipt())
				return true;
	}
	return false;
}

PPBillPacket::CgrRetBlock::CgrRetBlock()
{
	THISZERO();
}

PPBillPacket::QuotSetupInfoItem::QuotSetupInfoItem()
{
	THISZERO();
}

struct ModifGoodsItem {
	PPID   GoodsID;
	int    Sign;
	double Qtty;
	LongArray PosList;
};

static int AddModifGoodsListItem(TSCollection <ModifGoodsItem> & rList, int pos, PPID goodsID, int sign, double qtty)
{
	int   ok = 1;
	int   found = 0;
	for(uint j = 0; !found && j < rList.getCount(); j++) {
		ModifGoodsItem * p_mg_item = rList.at(j);
		if(p_mg_item->GoodsID == goodsID && p_mg_item->Sign == sign) {
			p_mg_item->Qtty += qtty;
			p_mg_item->PosList.add(pos);
			found = 1;
		}
	}
	if(!found) {
		ModifGoodsItem * p_new_mg_item = rList.CreateNewItem();
		THROW_SL(p_new_mg_item);
		p_new_mg_item->GoodsID = goodsID;
		p_new_mg_item->Sign = sign;
		p_new_mg_item->Qtty = qtty;
		p_new_mg_item->PosList.add(pos);
	}
	CATCHZOK
	return ok;
}

int PPBillPacket::CheckGoodsForRestrictions(int rowIdx, PPID goodsID, int sign, double qtty, long flags, CgrRetBlock * pRetBlk)
{
	int    ok = 1;
	PPObjGoods goods_obj;
	SString msg_buf, temp_buf;
	CgrRetBlock ret_blk;
	Goods2Tbl::Rec goods_rec;
	THROW(goods_obj.Fetch(goodsID, &goods_rec) > 0);
	if((flags & cgrfMatrix) && !goods_obj.CheckMatrix(goodsID, Rec.LocID, Rec.OpID, Rec.Object))
		ok = 0;
	else if((flags & cgrfObject) && (ProcessFlags & pfRestrictByArCodes) && goods_obj.P_Tbl->GetArCode(Rec.Object, labs(goodsID), temp_buf, 0) <= 0) {
		GetArticleName(Rec.Object, temp_buf);
		msg_buf.Z().Cat(goods_rec.Name).CatDiv('-', 1).Cat(temp_buf);
		ok = PPSetError(PPERR_BILLGOODSRESTRICTBYARCODE, msg_buf);
	}
	else if((flags & cgrfGoodsGrpRestrict) && P_GoodsGrpRestrict) {
		for(uint i = 0; ok && i < P_GoodsGrpRestrict->getCount(); i++) {
			const LAssoc & r_item = P_GoodsGrpRestrict->at(i);
			const  PPID grp_id = r_item.Key;
			if(r_item.Val > 0) {
				if(!goods_obj.BelongToGroup(goodsID, grp_id)) {
					msg_buf.Z().Cat(goods_rec.Name).Space().Cat(">>").Space();
					goods_obj.FetchNameR(grp_id, temp_buf);
					msg_buf.Cat(temp_buf);
					ok = PPSetError(PPERR_BILLGOODSRESTRICT, msg_buf);
				}
			}
			else if(r_item.Val < 0) {
				if(goods_obj.BelongToGroup(goodsID, grp_id)) {
					msg_buf.Z().Cat(goods_rec.Name).Space().Cat(">>").Space();
					goods_obj.FetchNameR(grp_id, temp_buf);
					msg_buf.Cat(temp_buf);
					ok = PPSetError(PPERR_BILLGOODSRESTRICTNEG, msg_buf);
				}
			}
		}
	}
	if(ok) {
		if(OpTypeID == PPOPT_GOODSMODIF) {
            PPOprKind op_rec;
            if(GetOpData(Rec.OpID, &op_rec) > 0 && (op_rec.ExtFlags & (OPKFX_MCR_GROUP|OPKFX_MCR_SUBSTSTRUC|OPKFX_MCR_EQQTTY))) {
				TSCollection <ModifGoodsItem> mg_list;
				uint   i;
				for(i = 0; i < GetTCount(); i++) {
					const PPTransferItem & r_ti = ConstTI(i);
					const  PPID goods_id = labs((rowIdx == (int)i) ? goodsID : r_ti.GoodsID);
					const double _q = (rowIdx == (int)i) ? qtty : r_ti.Quantity_;
					THROW(AddModifGoodsListItem(mg_list, (int)i, goods_id, r_ti.GetSign(Rec.OpID), _q));
				}
				if(rowIdx < 0) {
					THROW(AddModifGoodsListItem(mg_list, (int)i, labs(goodsID), sign, qtty));
				}
				if(mg_list.getCount() > 2) {
					ok = PPSetError(PPERR_BILLGOODSRESTR_MCR_COUNT);
				}
				else if(mg_list.getCount() == 2) {
                    const  int  s1 = mg_list.at(0)->Sign;
                    const  int  s2 = mg_list.at(1)->Sign;
                    const  PPID _id1 = mg_list.at(0)->GoodsID;
                    const  PPID _id2 = mg_list.at(1)->GoodsID;
                    if((s1 == TISIGN_PLUS && s2 == TISIGN_MINUS) || (s1 == TISIGN_MINUS && s2 == TISIGN_PLUS)) {
						if(ok && op_rec.ExtFlags & OPKFX_MCR_EQQTTY && flags & cgrfQtty) {
							const  double _qtty1 = mg_list.at(0)->Qtty;
							const  double _qtty2 = mg_list.at(1)->Qtty;
							if(!feqeps(_qtty1, _qtty2, 1e-3))
								ok = PPSetError(PPERR_BILLGOODSRESTR_MCR_EQQTTY);
						}
						if(ok && op_rec.ExtFlags & OPKFX_MCR_GROUP) {
							PPID parent_id1 = 0;
							PPID parent_id2 = 0;
                            goods_obj.GetParentID(_id1, &parent_id1);
                            goods_obj.GetParentID(_id2, &parent_id2);
                            if(parent_id1 != parent_id2)
								ok = PPSetError(PPERR_BILLGOODSRESTR_MCR_GRP);
						}
						if(ok && op_rec.ExtFlags & OPKFX_MCR_SUBSTSTRUC) {
							PPIDArray aggr_list1;
							PPIDArray aggr_list2;
							PPObjGoodsStruc gs_obj;
							PPGoodsStrucHeader gs_hdr;
							ObjAssocTbl::Rec assc_rec;
							int    local_ok = 0;
							Reference * p_ref = PPRef;
							{
								for(SEnum en = p_ref->Assc.Enum(PPASS_GOODSSTRUC, _id1, 1); !local_ok && en.Next(&assc_rec) > 0;) {
									const  PPID gs_id = assc_rec.PrmrObjID;
									if(gs_obj.Fetch(gs_id, &gs_hdr) > 0 && gs_hdr.Flags & GSF_SUBST) {
										aggr_list1.add(gs_id);
										PPID   owner_goods_id = 0;
										if(goods_obj.P_Tbl->SearchAnyRef(PPOBJ_GOODSSTRUC, gs_id, &owner_goods_id) > 0 && owner_goods_id == _id2)
											local_ok = 1;
									}
								}
								aggr_list1.sortAndUndup();
							}
							if(!local_ok) {
								for(SEnum en = p_ref->Assc.Enum(PPASS_GOODSSTRUC, _id2, 1); !local_ok && en.Next(&assc_rec) > 0;) {
									const  PPID gs_id = assc_rec.PrmrObjID;
									if(gs_obj.Fetch(gs_id, &gs_hdr) > 0 && gs_hdr.Flags & GSF_SUBST) {
										aggr_list2.add(gs_id);
										PPID   owner_goods_id = 0;
										if(goods_obj.P_Tbl->SearchAnyRef(PPOBJ_GOODSSTRUC, gs_id, &owner_goods_id) > 0 && owner_goods_id == _id1)
											local_ok = 1;
									}
								}
								aggr_list2.sortAndUndup();
							}
							if(!local_ok) {
								PPIDArray is;
								is = aggr_list1;
								is.intersect(&aggr_list2, 1);
								if(!is.getCount())
									ok = PPSetError(PPERR_BILLGOODSRESTR_MCR_SUBST);
							}
						}
					}
					else
						ok = PPSetError(PPERR_BILLGOODSRESTR_MCR_SIGN);
				}
				else {
					// ok, but bill isn't completed
				}
            }
		}
	}
	if(ok) {
		PPObjGoodsType gt_obj;
		PPGoodsType2 gt_rec;
		TransferTbl::Rec trfr_rec; // Используется при вызове методов GCTIterator
		BillTbl::Rec bill_rec;     // Используется при вызове методов GCTIterator
		if(goods_rec.GoodsTypeID && gt_obj.Fetch(goods_rec.GoodsTypeID, &gt_rec) > 0 && gt_rec.PriceRestrID) {
			PPObjGoodsValRestr gvr_obj;
			PPObjGoodsValRestr::GvrArray gvr_list;
			if(gvr_obj.FetchBarList(gvr_list) > 0) {
				if((flags & cgrfObject) && !gvr_list.TestGvrBillArPair(gt_rec.PriceRestrID, Rec.Object)) {
					GetArticleName(Rec.Object, temp_buf);
					msg_buf.Z().Cat(goods_rec.Name).CatDiv('-', 1).Cat(temp_buf);
					ok = PPSetError(PPERR_BILLGOODSRESTRBARMAIN, msg_buf);
				}
				if((flags & cgrfObject2) && !gvr_list.TestGvrBillExtArPair(gt_rec.PriceRestrID, Rec.Object2)) {
					GetArticleName(Rec.Object2, temp_buf);
					msg_buf.Z().Cat(goods_rec.Name).CatDiv('-', 1).Cat(temp_buf);
					ok = PPSetError(PPERR_BILLGOODSRESTRBAREXT, msg_buf);
				}
			}
			if(flags & cgrfShipmControl && qtty != 0.0) {
				PPGoodsValRestrPacket gvr_pack;
				if(Rec.Dt && Rec.Object && gvr_obj.Fetch(gt_rec.PriceRestrID, &gvr_pack) > 0 && gvr_pack.Rec.ScpShipmOpID) {
					const int is_shipm = IsOpBelongTo(Rec.OpID, gvr_pack.Rec.ScpShipmOpID);
					if(gvr_pack.Rec.ScpShipmLimitOpID && is_shipm) {
                        BillCore & r_billc = *P_BObj->P_Tbl;
                        RAssocArray limits; // Ассоциации документов с лимитом количества товара goodsID
                        PPIDArray zero_obj_bill_list; // Список документов плана, имеющих нулевого контрагента
                        PPIDArray obj_bill_list; // Список документов плана, имеющих того же контрагента, что и this
                        PPIDArray dlvr_bill_list; // Список документов плана, имеющих тот же адрес доставки, что и this
                        BillTbl::Rec plan_bill_rec;
                        PPIDArray plan_bill_list;
                        DateRange plan_bill_period;
						PPIDArray _goods_list;
						_goods_list.add(goodsID);
                        plan_bill_period.Set(plusdate(Rec.Dt, -180), Rec.Dt);
                        for(SEnum en = r_billc.EnumByOp(gvr_pack.Rec.ScpShipmLimitOpID, &plan_bill_period, 0); en.Next(&plan_bill_rec) > 0;) {
                            if(Rec.Dt >= plan_bill_rec.Dt && (!plan_bill_rec.DueDate || Rec.Dt < plan_bill_rec.DueDate)) {
								if(!plan_bill_rec.Object || plan_bill_rec.Object == Rec.Object) {
									const  PPID plan_bill_id = plan_bill_rec.ID;
									PPBillPacket plan_bill_pack;
									P_BObj->P_CpTrfr->LoadItems(plan_bill_id, &plan_bill_pack, &_goods_list);
									int   has_goods = 0;
									for(uint tiidx = 0; tiidx < plan_bill_pack.GetTCount(); tiidx++) {
										if(plan_bill_pack.ConstTI(tiidx).GoodsID == goodsID) {
											limits.Add(plan_bill_id, fabs(plan_bill_pack.ConstTI(tiidx).Quantity_), 1, 0);
											has_goods = 1;
										}
									}
									if(has_goods) {
										if(plan_bill_rec.Object == 0)
											zero_obj_bill_list.add(plan_bill_id);
										else {
											obj_bill_list.add(plan_bill_id);
											if(P_Freight && P_Freight->DlvrAddrID__) { // @1
												PPFreight freight;
                                                if(r_billc.GetFreight(plan_bill_id, &freight) > 0 && freight.DlvrAddrID__ == P_Freight->DlvrAddrID__)
													dlvr_bill_list.add(plan_bill_id);
											}
										}
									}
								}
                            }
                        }
                        {
							const PPIDArray * p_target_bill_list = 0;
							GCTFilt gct_filt;
							gct_filt.GoodsID = goodsID;
							gct_filt.ArList.Add(Rec.Object);
							gct_filt.Flags |= OPG_FORCEBILLCACHE;
							if(dlvr_bill_list.getCount()) {
								p_target_bill_list = &dlvr_bill_list;
								assert(P_Freight && P_Freight->DlvrAddrID__); // Не может такого быть, чтобы условие не выполнилось (see @1)
								gct_filt.DlvrAddrID = P_Freight->DlvrAddrID__;
							}
							else if(obj_bill_list.getCount())
								p_target_bill_list = &obj_bill_list;
							else if(zero_obj_bill_list.getCount())
								p_target_bill_list = &zero_obj_bill_list;
							if(p_target_bill_list) {
								double lim = 0.0;
								for(uint limidx = 0; limidx < limits.getCount(); limidx++)
									if(p_target_bill_list->lsearch(limits.at(limidx).Key))
										lim += limits.at(limidx).Val;
								if(lim > 0.0) {
									for(uint bidx = 0; ok && bidx < p_target_bill_list->getCount(); bidx++) {
										const  PPID target_bill_id = p_target_bill_list->get(bidx);
										const double lim_by_plan = limits.Get(target_bill_id);
										if(lim_by_plan > 0.0 && r_billc.Search(target_bill_id, &plan_bill_rec) > 0) {
											DateRange observe_period;
											observe_period.Set(plan_bill_rec.Dt, plan_bill_rec.DueDate);
											GCTIterator gctiter(&gct_filt, &observe_period);
											double shipm_qtty = 0.0;
											double this_bill_ex_qtty = 0.0;
											if(gctiter.First(&trfr_rec, &bill_rec) > 0) do {
												if(IsOpBelongTo(bill_rec.OpID, gvr_pack.Rec.ScpShipmOpID)) {
													shipm_qtty += fabs(trfr_rec.Quantity);
													if(bill_rec.ID == Rec.ID) // Учет записей в базе данных по обрабатываемому документу
														this_bill_ex_qtty += fabs(trfr_rec.Quantity);
												}
												else if(gvr_pack.Rec.ScpRetOpID && IsOpBelongTo(bill_rec.OpID, gvr_pack.Rec.ScpRetOpID)) {
													shipm_qtty -= fabs(trfr_rec.Quantity);
												}
											} while(gctiter.Next(&trfr_rec, &bill_rec) > 0);
											if(((shipm_qtty - this_bill_ex_qtty) + qtty) > lim_by_plan) {
												GetArticleName(Rec.Object, temp_buf);
												msg_buf.Z().Cat(goods_rec.Name).CatDiv('-', 1).Cat(temp_buf).CatDiv('-', 1).Cat(plan_bill_rec.Code);
												ok = PPSetError(PPERR_BILLGOODSRESTRLIMSHPM, msg_buf);
											}
										}
									}
								}
							}
                        }
					}
					if(ok && gvr_pack.Rec.ScpDurationDays > 0 && (is_shipm || GetOpType(Rec.OpID) == PPOPT_GOODSORDER)) {
						ret_blk.ScpDuration = gvr_pack.Rec.ScpDurationDays;
						PPIDArray bill_id_list;
						GCTFilt gct_filt;
						gct_filt.GoodsID = labs(goodsID);
						gct_filt.Period.low = plusdate(Rec.Dt, -(gvr_pack.Rec.ScpDurationDays+1));
						gct_filt.Period.upp = plusdate(Rec.Dt, -1);
						gct_filt.ArList.Add(Rec.Object);
						/*
						Вероятно, отбор по адресу доставки надо будет включать по специальному флагу.
						Возвраты часто вводятся без адреса доставки, потому безусловное ограничение таким адресом
						может исказить общую статистику.
						if(P_Freight && P_Freight->DlvrAddrID) {
							gct_filt.DlvrAddrID = P_Freight->DlvrAddrID;
						}
						*/
						gct_filt.Flags |= OPG_FORCEBILLCACHE;
						GCTIterator gctiter(&gct_filt, &gct_filt.Period);
						if(gctiter.First(&trfr_rec, &bill_rec) > 0) {
							do {
								if(GetOpType(bill_rec.OpID) != PPOPT_GOODSREVAL) {
									if(IsOpBelongTo(bill_rec.OpID, gvr_pack.Rec.ScpShipmOpID)) {
										if(trfr_rec.Quantity < 0.0) {
											if(bill_id_list.addUnique(bill_rec.ID) > 0)
												ret_blk.ScpShipmBillCount++;
											ret_blk.ScpShipmQtty += fabs(trfr_rec.Quantity);
										}
									}
									else if(gvr_pack.Rec.ScpRetOpID && IsOpBelongTo(bill_rec.OpID, gvr_pack.Rec.ScpRetOpID)) {
										if(trfr_rec.Quantity > 0.0) {
											if(bill_id_list.addUnique(bill_rec.ID) > 0)
												ret_blk.ScpRetBillCount++;
											ret_blk.ScpRetQtty += fabs(trfr_rec.Quantity);
										}
									}
								}
							} while(gctiter.Next(&trfr_rec, &bill_rec) > 0);
							if(ret_blk.ScpShipmBillCount > 0) {
								ret_blk.ScpResult = (ret_blk.ScpShipmQtty - ret_blk.ScpRetQtty) / ret_blk.ScpShipmBillCount;
								ret_blk.ScpRange.low = (gvr_pack.Rec.ScpDnDev > 0) ? ((1.0 - fdiv1000i(gvr_pack.Rec.ScpDnDev)) * ret_blk.ScpResult) : 0.0;
								ret_blk.ScpRange.upp = (gvr_pack.Rec.ScpUpDev > 0) ? ((1.0 + fdiv1000i(gvr_pack.Rec.ScpUpDev)) * ret_blk.ScpResult) : round(ret_blk.ScpResult, 1.0, +1);
								if(fabs(qtty) < ret_blk.ScpRange.low || fabs(qtty) > ret_blk.ScpRange.upp) {
									ok = 100;
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pRetBlk, ret_blk);
	return ok;
}

int PPBillPacket::InsertRow(const PPTransferItem * pTI, LongArray * pRows, int pcug)
{
	int    ok = 1;
	uint   pos = Lots.getCount();
	if(pTI->IsRecomplete()) {
		//
		// Нельзя добавляеть строку рекомплектации в документ, который уже содержит независимый выход
		//
		THROW_PP(!HasIndepModifPlus(), PPERR_LOTGENONRECOMPL);
		THROW_PP(!(Rec.Flags & BILLF_RECOMPLETE), PPERR_DUPRECOMPLETE);
		Rec.Flags |= BILLF_RECOMPLETE;
	}
	THROW_SL(Lots.insert(pTI) && (!pRows || pRows->insert(&pos)));
	if(pTI->Flags & PPTFR_AUTOCOMPL) {
		THROW(InsertAutoComplRow(pos, pcug));
	}
	if(Rec.Flags & BILLF_RECOMPLETE) {
		// Нельзя добавлять строку независимого выхода в документ рекомплектации
		THROW_PP(!(pTI->Flags & PPTFR_PLUS) || !(pTI->Flags & PPTFR_RECEIPT), PPERR_LOTGENONRECOMPL);
	}
	CATCHZOK
	return ok;
}

int PPBillPacket::ShrinkTRows(long fl /* = ETIEF_DIFFBYLOT | ETIEF_UNITEBYGOODS */)
{
	int    ok = 1;
	LongArray saw;
	PPTransferItem * p_ti;
	for(uint i = 0; EnumTItems(&i, &p_ti);) {
		const long idx = static_cast<long>(i-1);
		if(!saw.lsearch(idx))
			MergeTI(p_ti, idx, fl, saw, 0);
	}
	saw.sort();
	RemoveRows(&saw);
	return ok;
}

bool FASTCALL IsLotVATFree(const ReceiptTbl::Rec & rLotRec)
	{ return (IsSupplVATFree(rLotRec.SupplID) > 0 || PPObjLocation::CheckWarehouseFlags(rLotRec.LocID, LOCF_VATFREE)); }
double FASTCALL PPBillPacket::GetAmount(int minus /*= 0*/) const
	{ return BR2(minus ? -Rec.Amount : Rec.Amount); }

double FASTCALL PPBillPacket::GetBaseAmount(int minus /*= 0*/) const
{
	double result = BR2(minus ? -Rec.Amount : Rec.Amount);
	if(Rec.CurID)
		result = R2(result * Amounts.Get(PPAMT_CRATE, Rec.CurID));
	return result;
}

int PPBillPacket::GetQuotKindList(PPIDArray * pList)
{
	PPObjQuotKind qk_obj;
	return qk_obj.GetListByOp(Rec.OpID, Rec.Dt, pList);
}

int PPBillPacket::GetQuotExt(const PPTransferItem & rTi, double * pPrice)
{
	int    ok = -1, r;
	double result = 0.0;
	PPIDArray qk_list;
	GetQuotKindList(&qk_list);
	if(qk_list.getCount()) {
		PPObjGoods goods_obj;
		for(uint i = 0; i < qk_list.getCount(); i++) {
			const QuotIdent qi(QIDATE(rTi.Date), rTi.LocID, qk_list.get(i), 0 /* curID */, 0 /* arID */);
			if((r = goods_obj.GetQuotExt(labs(rTi.GoodsID), qi, rTi.Cost, *pPrice, &result, 1)) > 0) {
				ok = r;
				break;
			}
		}
	}
	ASSIGN_PTR(pPrice, result);
	return ok;
}

static void AddSalesTax(PPTransferItem * pTI, double rate, int plus)
{
	const PPCommConfig & r_ccfg = CConfig;
	if(!(r_ccfg.Flags & CCFLG_TGGLEXCSNPRICE) && !(pTI->Flags & PPTFR_PRICEWOTAXES)) {
		const double net_price_rate = ((pTI->Flags & PPTFR_ORDER) ? pTI->Price : pTI->NetPrice()) * rate;
		double add_dis = 0.0;
		if(!(r_ccfg.Flags & CCFLG_PRICEWOEXCISE))
			plus = plus ? 0 : 1;
		if(plus)
			add_dis = net_price_rate / (100.0 + rate);
		else
			add_dis = -net_price_rate / 100.0;
		pTI->Discount += R2(add_dis);
	}
}

struct TiDisItem {
	double Price;
	double Qtty;
	uint   Idx;
};

IMPL_CMPFUNC(TiDisItem, i1, i2)
{
	const TiDisItem * p1 = static_cast<const TiDisItem *>(i1);
	const TiDisItem * p2 = static_cast<const TiDisItem *>(i2);
	if(p1->Price < p2->Price)
		return -1;
	else if(p1->Price > p2->Price)
		return 1;
	else if(p1->Qtty < p2->Qtty)
		return 1;
	else if(p1->Qtty > p2->Qtty)
		return -1;
	else
		return 0;
}

static double FASTCALL ti_price(const PPTransferItem * pTi) { return R2((pTi->Flags & PPTFR_ORDER) ? pTi->Price : pTi->NetPrice()); }
static double FASTCALL ti_price2(const PPTransferItem * pTi) { return TR5((pTi->Flags & PPTFR_ORDER) ? pTi->Price : pTi->NetPrice()); }

static void FASTCALL set_ti_dis(PPTransferItem * pTi, double d)
{
	if(pTi->Flags & PPTFR_ORDER)
		pTi->Discount = d;
	else
		pTi->Discount += d;
}

void PPBillPacket::SetTotalDiscount(double dis, int pctdis, int rmvexcise)
{
	if(!oneof2(OpTypeID, PPOPT_GOODSREVAL, PPOPT_CORRECTION)) {
		const  PPCommConfig & r_ccfg = CConfig;
		const  PPBillConfig & cfg = P_BObj->GetConfig();
		const  int zero = BIN(dis == 0.0 && rmvexcise == 0);
		const  int empty_toggle = BIN((r_ccfg.Flags & CCFLG_TGGLEXCSNPRICE) || (Rec.Flags & BILLF_TGGLEXCSNPRICE));
		uint   i;
		PPTransferItem * ti;
		uint   last_index = 0;
		double min_qtty  = SMathConst::Max;
		double max_price = 0.0;
		double amount    = 0.0;
		PPObjGoods     gobj;
		Goods2Tbl::Rec grec;
		PPGoodsTaxEntry gtx;
		SArray list(sizeof(TiDisItem));
		if(cfg.SwitchedTDisCalcMethodDate && cfg.SwitchedTDisCalcMethodDate <= Rec.Dt) {
			for(i = 0; EnumTItems(&i, &ti);) {
				if(!(ti->Flags & PPTFR_PCKG)) {
					const  int skip_dis = 0;
					gobj.Fetch(ti->GoodsID, &grec);
					SETFLAG(ti->Flags, PPTFR_NODISCOUNT, grec.Flags & GF_NODISCOUNT);
					if(!skip_dis) {
						if(ti->Flags & PPTFR_QUOT) {
							if(ti->Flags & PPTFR_ORDER) {
								ti->Discount = 0.0;
								ti->Price = ti->QuotPrice;
							}
							else
								ti->Discount = ti->Price - ti->QuotPrice;
						}
						else if(!(empty_toggle && dis == 0.0))
							ti->Discount = 0.0;
					}
					if(cfg.TDisCalcMethod != PPBillConfig::tdcmSimple) {
						const int qr = BIN(ti->Flags & PPTFR_QUOT && ti->Flags & PPTFR_RMVEXCISE);
						if(rmvexcise || qr) {
							double rate = 0.0;
							if(!empty_toggle) {
								gobj.FetchTaxEntry2(ti->GoodsID, 0/*lotID*/, 0/*taxPayerID*/, ti->Date, Rec.OpID, &gtx); // SALES_TAX
								rate = fdiv100i(gtx.SalesTax); // @divtax
								if(!rmvexcise)
									AddSalesTax(ti, rate, 1);
								else if(!qr)
									AddSalesTax(ti, rate, 0);
							}
						}
						SETFLAG(ti->Flags, PPTFR_RMVEXCISE, rmvexcise);
					}
					if(!skip_dis && !(ti->Flags & PPTFR_NODISCOUNT) && !zero) {
						const double qtty = fabs(ti->Quantity_);
						const double p    = ti_price2(ti);
						amount += R2(p * qtty);
						if(cfg.TDisCalcMethod == PPBillConfig::tdcmRegress2) {
							TiDisItem tdi;
							tdi.Qtty  = qtty;
							tdi.Price = p;
							tdi.Idx   = i-1;
							list.insert(&tdi);
						}
						else if(qtty > 0.0 && (qtty < min_qtty || (qtty == min_qtty && p > max_price))) {
							last_index = i;
							min_qtty   = qtty;
							max_price  = p;
						}
					}
				}
			}
			if(dis != 0 && amount != 0) {
				const double discount = pctdis ? (dis * fdiv100r(amount)) : dis;
				double d;
				double part_dis = 0.0;
				double part_amount = 0.0;
				if(cfg.TDisCalcMethod == PPBillConfig::tdcmRegress2) {
					TiDisItem * p_tdi;
					list.sort(PTR_CMPFUNC(TiDisItem));
					for(i = 0; list.enumItems(&i, (void **)&p_tdi);) {
						const double qtty = p_tdi->Qtty;
						const double ths  = p_tdi->Price;
						const double prev = (i > 1) ? static_cast<const TiDisItem *>(list.at(i-2))->Price : SMathConst::Max;
						const double next = (i < list.getCount()) ? static_cast<const TiDisItem *>(list.at(i))->Price : SMathConst::Max;
						if(qtty > 0 && (qtty < min_qtty || (qtty == min_qtty && ths > max_price)) && (ths != prev && ths != next)) {
							last_index = p_tdi->Idx+1;
							min_qtty   = qtty;
							max_price  = ths;
						}
					}
					double prev_price = -SMathConst::Max, prev_dis = 0.0;
					for(i = 0; list.enumItems(&i, (void **)&p_tdi);) {
						if((p_tdi->Idx+1) != (int)last_index) {
							ti = & TI(p_tdi->Idx);
							double p    = ti_price2(ti);
							double qtty = fabs(ti->Quantity_);
							if(i > 1 && p == prev_price)
								d = prev_dis;
							else
								d = round(p * (discount - part_dis) / (amount - part_amount), cfg.TDisCalcPrec);
							part_dis    += (d * qtty);
							part_amount += (p * qtty);
							set_ti_dis(ti, d);
							prev_price = p_tdi->Price;
							prev_dis = d;
						}
					}
					if(last_index) {
						ti = &TI(last_index-1);
						set_ti_dis(ti, round((discount - part_dis) / fabs(ti->Quantity_), cfg.TDisCalcPrec));
					}
				}
				else if(cfg.TDisCalcMethod == PPBillConfig::tdcmRegress) {
					for(i = 0; EnumTItems(&i, &ti);)
						if(!ti->CurID && !(ti->Flags & (PPTFR_NODISCOUNT | PPTFR_PCKG)) && i != last_index) {
							const double p    = ti_price2(ti);
							const double qtty = fabs(ti->Quantity_);
							d = round(p * (discount - part_dis) / (amount - part_amount), cfg.TDisCalcPrec);
							part_dis += (d * qtty);
							part_amount += (p * qtty);
							set_ti_dis(ti, d);
						}
					if(last_index) {
						ti = &TI(last_index-1);
						set_ti_dis(ti, round((discount - part_dis) / fabs(ti->Quantity_), cfg.TDisCalcPrec));
					}
				}
				else { // PPBillconfig::tdcmSimple
					const double rel = pctdis ? fdiv100r(dis) : (dis / amount);
					for(i = 0; EnumTItems(&i, &ti);)
						if(!ti->CurID && !(ti->Flags & (PPTFR_NODISCOUNT | PPTFR_PCKG)))
							set_ti_dis(ti, round(ti_price2(ti) * rel, cfg.TDisCalcPrec));
				}
			}
			if(cfg.TDisCalcMethod == PPBillConfig::tdcmSimple) {
				for(i = 0; EnumTItems(&i, &ti);) {
					if(!(ti->Flags & PPTFR_PCKG)) {
						if(rmvexcise && !empty_toggle) {
							gobj.FetchTaxEntry2(ti->GoodsID, 0/*lotID*/, 0/*taxPayerID*/, ti->Date, Rec.OpID, &gtx); // SALES_TAX
							AddSalesTax(ti, fdiv100i(gtx.SalesTax), 0);
						}
						SETFLAG(ti->Flags, PPTFR_RMVEXCISE, rmvexcise);
					}
				}
			}
		}
		else {
			//
			// Этот участок кода, хотя почти повторяет участок, находящийся выше, НЕ ПОДЛЕЖИТ МОДИФИКАЦИИ,
			// так как является хорошо отлаженным и ошибки в нем могут привести к модификации
			// сумм документов в базах данных задним числом.
			//
			const int  v3918_calc_method = (Rec.Dt <= r_ccfg._3918_TDisCalcMethodLockDate) ? 0 : 1;
			const int  v405_calc_method  = (Rec.Dt <= r_ccfg._405_TDisCalcMethodLockDate) ? 0 : 1;
			const int  v418_calc_method  = (Rec.Dt <= r_ccfg._418_TDisCalcMethodLockDate) ? 0 : 1;
			for(i = 0; EnumTItems(&i, &ti);) {
				if(!(ti->Flags & PPTFR_PCKG)) {
					const  int skip_dis = 0;
					gobj.Fetch(ti->GoodsID, &grec);
					SETFLAG(ti->Flags, PPTFR_NODISCOUNT, grec.Flags & GF_NODISCOUNT);
					if(!skip_dis)
						if(ti->Flags & PPTFR_QUOT)
							if(ti->Flags & PPTFR_ORDER) {
								ti->Discount = 0.0;
								ti->Price = ti->QuotPrice;
							}
							else
								ti->Discount = ti->Price - ti->QuotPrice;
						else if(!(empty_toggle && dis == 0.0))
							ti->Discount = 0;
					if(!v3918_calc_method) {
						int    qr = BIN(ti->Flags & PPTFR_QUOT && ti->Flags & PPTFR_RMVEXCISE);
						if(rmvexcise || qr) {
							double rate = 0.0;
							if(!empty_toggle) {
								gobj.FetchTaxEntry2(ti->GoodsID, 0/*lotID*/, 0/*taxPayerID*/, ti->Date, Rec.OpID, &gtx); // SALES_TAX
								rate = fdiv100i(gtx.SalesTax); // @divtax
							}
							if(!rmvexcise) {
								if(!empty_toggle)
									AddSalesTax(ti, rate, 1);
							}
							else {
								if(!qr && !empty_toggle)
									AddSalesTax(ti, rate, 0);
								ti->Flags |= PPTFR_RMVEXCISE;
							}
						}
						if(!rmvexcise)
							ti->Flags &= ~PPTFR_RMVEXCISE;
					}
					if(!skip_dis && !(ti->Flags & PPTFR_NODISCOUNT) && !zero) {
						const double qtty = fabs(ti->Quantity_);
						const double p    = R2((ti->Flags & PPTFR_ORDER) ? ti->Price : ti->NetPrice());
						amount += R2(p * qtty);
						if(v418_calc_method) {
							TiDisItem tdi;
							tdi.Qtty = qtty;
							tdi.Price = p;
							tdi.Idx = i-1;
							list.insert(&tdi);
						}
						else if(qtty > 0 && (qtty < min_qtty || (qtty == min_qtty && p > max_price))) {
							last_index = i;
							min_qtty = qtty;
							max_price = p;
						}
					}
				}
			}
			if(dis != 0.0 && amount != 0.0) {
				const double discount = pctdis ? (dis * fdiv100r(amount)) : dis;
				double d;
				double part_dis = 0.0;
				double part_amount = 0.0;
				if(v418_calc_method) {
					TiDisItem * p_tdi;
					list.sort(PTR_CMPFUNC(TiDisItem));
					for(i = 0; list.enumItems(&i, (void **)&p_tdi);) {
						const double qtty = p_tdi->Qtty;
						const double ths  = p_tdi->Price;
						const double prev = (i > 1) ? ((TiDisItem *)list.at(i-2))->Price : SMathConst::Max;
						const double next = (i < list.getCount()) ? ((TiDisItem *)list.at(i))->Price : SMathConst::Max;
						if(qtty > 0.0 && (qtty < min_qtty || (qtty == min_qtty && ths > max_price)) && (ths != prev && ths != next)) {
							last_index = p_tdi->Idx+1;
							min_qtty   = qtty;
							max_price  = ths;
						}
					}
					double prev_price = -SMathConst::Max;
					double prev_dis = 0.0;
					for(i = 0; list.enumItems(&i, (void **)&p_tdi);) {
						if((p_tdi->Idx+1) != (int)last_index) {
							ti = & TI(p_tdi->Idx);
							const double qtty = fabs(ti->Quantity_);
							double p = ti_price(ti);
							if(i > 1 && p == prev_price)
								d = prev_dis;
							else
								d = R2(p * (discount - part_dis) / (amount - part_amount));
							part_dis    += (d * qtty);
							part_amount += (p * qtty);
							set_ti_dis(ti, d);
							prev_price = p_tdi->Price;
							prev_dis = d;
						}
					}
					if(last_index) {
						ti = &TI(last_index-1);
						set_ti_dis(ti, R2((discount - part_dis) / fabs(ti->Quantity_)));
					}
				}
				else if(v405_calc_method) {
					for(i = 0; EnumTItems(&i, &ti);) {
						if(!ti->CurID && !(ti->Flags & (PPTFR_NODISCOUNT | PPTFR_PCKG)) && i != last_index) {
							const double p    = ti_price(ti);
							const double qtty = fabs(ti->Quantity_);
							d = R2(p * (discount - part_dis) / (amount - part_amount));
							part_dis += (d * qtty);
							part_amount += (p * qtty);
							set_ti_dis(ti, d);
						}
					}
					if(last_index) {
						ti = &TI(last_index-1);
						set_ti_dis(ti, R2((discount - part_dis) / fabs(ti->Quantity_)));
					}
				}
				else {
					const double rel = pctdis ? fdiv100r(dis) : (dis / amount);
					for(i = 0; EnumTItems(&i, &ti);)
						if(!ti->CurID && !(ti->Flags & (PPTFR_NODISCOUNT | PPTFR_PCKG)))
							set_ti_dis(ti, R2(ti_price(ti) * rel));
				}
			}
			if(v3918_calc_method)
				for(i = 0; EnumTItems(&i, &ti);)
					if(!(ti->Flags & PPTFR_PCKG)) {
						if(rmvexcise && !empty_toggle) {
							gobj.FetchTaxEntry2(ti->GoodsID, 0/*lotID*/, 0/*taxPayerID*/, ti->Date, Rec.OpID, &gtx); // SALES_TAX
							AddSalesTax(ti, fdiv100i(gtx.SalesTax), 0);
						}
						SETFLAG(ti->Flags, PPTFR_RMVEXCISE, rmvexcise);
					}
		}
		if(pctdis) {
			Amounts.Put(PPAMT_PCTDIS, 0L/*@curID*/, dis, 1, 1);
			Amounts.Remove(PPAMT_MANDIS, -1);
		}
		else {
			Amounts.Put(PPAMT_MANDIS, Rec.CurID, dis, 1, 1);
			Amounts.Remove(PPAMT_PCTDIS, -1);
		}
		SETFLAG(Rec.Flags, BILLF_RMVEXCISE, rmvexcise);
	}
}

bool PPBillPacket::UsesDistribCost() const { return (OpTypeID == PPOPT_GOODSRECEIPT && CConfig.Flags & CCFLG_USEDISTRIBCOST); }

int PPBillPacket::DistributeExtCost()
{
	int    ok = -1;
	if(UsesDistribCost()) {
		PPObjAmountType at_obj;
		if(at_obj.IsThereDistribCostAmounts()) {
			ok = Helper_DistributeExtCost(0.0, 0);
			PPAmountType at_rec;
			for(uint i = 0; i < Amounts.getCount(); i++) {
				const AmtEntry & r_ae = Amounts.at(i);
				if(at_obj.Fetch(r_ae.AmtTypeID, &at_rec) > 0 && at_rec.Flags & PPAmountType::fDistribCost) {
					if(Helper_DistributeExtCost((at_rec.Flags & PPAmountType::fDcNeg) ? -fabs(r_ae.Amt) : fabs(r_ae.Amt), at_rec.EcAlg) > 0)
						ok = 1;
				}
			}
		}
	}
	return ok;
}

int PPBillPacket::Helper_DistributeExtCost(double extCostSum, int alg)
{
	int    ok = -1;
	uint   i;
	PPTransferItem * p_ti;
	if(extCostSum == 0.0) {
		for(i = 0; EnumTItems(&i, &p_ti);) {
			if(p_ti->ExtCost != 0.0) {
				p_ti->Cost = R5(p_ti->Cost - p_ti->ExtCost);
				p_ti->ExtCost = 0.0;
				ok = 1;
			}
		}
	}
	else {
		uint   last_index = 0;
		double min_qtty  = SMathConst::Max;
		double max_price = 0.0;
		double part_sum  = 0.0;
		double rest      = extCostSum;
		RAssocArray distr_list; // Список значений, пропорционально которым
			// распределяется стоимость между строками документа.
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		for(i = 0; EnumTItems(&i, &p_ti);) {
			const  PPID goods_id = labs(p_ti->GoodsID);
			if(goods_obj.Fetch(goods_id, &goods_rec) > 0 && !(goods_rec.Flags & GF_UNLIM)) {
				const double qtty = fabs(p_ti->Quantity_);
				const double p    = (p_ti->Cost + p_ti->ExtCost);
				double part = 0.0;
				GoodsStockExt gse;
				switch(alg) {
					case ecalgCost:
						part = (p * qtty);
						break;
					case ecalgPrice:
						part = (p_ti->NetPrice() * qtty);
						break;
					case ecalgQtty:
						part = qtty;
						break;
					case ecalgPhQtty:
						part = qtty * goods_rec.PhUPerU;
						break;
					case ecalgBrutto:
						if(goods_obj.GetStockExt(goods_id, &gse, 1) > 0)
							part = gse.CalcBrutto(qtty);
						break;
					case ecalgVolume:
						if(goods_obj.GetStockExt(goods_id, &gse, 1) > 0)
							part = gse.CalcVolume(qtty);
						break;
					default:
						part = (p * qtty);
						break;
				}
				if(part > 0.0) {
					distr_list.Add(i, part);
					part_sum += part;
					if(qtty > 0.0 && (qtty < min_qtty || (qtty == min_qtty && p > max_price))) {
						last_index = i;
						min_qtty   = qtty;
						max_price  = p;
					}
				}
			}
		}
		if(distr_list.getCount()) {
			for(i = 0; EnumTItems(&i, &p_ti);) {
				const double part = distr_list.Get(i);
				if(part > 0.0 && i != last_index) {
					const double qtty = fabs(p_ti->Quantity_);
					const double ext_cost = R5((part * extCostSum) / (part_sum * qtty));
					p_ti->Cost += ext_cost;
					p_ti->ExtCost += ext_cost;
					rest -= (ext_cost * qtty);
					ok = 1;
				}
			}
			if(last_index) {
				p_ti = &Lots.at(last_index-1);
				double part = distr_list.Get(last_index);
				if(part > 0.0) {
					const double qtty = fabs(p_ti->Quantity_);
					const double ext_cost = R5(rest / qtty);
					p_ti->Cost += ext_cost;
					p_ti->ExtCost += ext_cost;
					rest -= (ext_cost * qtty);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int FASTCALL PPBillPacket::InitAmounts(const AmtList * pList)
{
	if(!(Rec.Flags & BILLF_FIXEDAMOUNTS)) {
		PPIDArray op_type_list; // Список типов операций, для которых следует
		// удалять налоговые суммы даже если они имеют признак "Ручная" (PPAmountType::fManual)
		op_type_list.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSREVAL, PPOPT_GOODSMODIF,
			PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0L);
		const int subop = GetOpSubType(Rec.OpID);
		PPObjAmountType amtt_obj;
		for(int i = Amounts.getCount() - 1; i >= 0; i--) {
			const  PPID atyp = Amounts.at(i).AmtTypeID;
			// Вычищаем все рассчитываемые суммы
			int   skip_amt = 0;
			if(oneof4(atyp, PPAMT_PAYMENT, PPAMT_PCTDIS, PPAMT_MANDIS, PPAMT_CRATE))
				skip_amt = 1;
			else if(OpTypeID == PPOPT_ACCTURN && subop == OPSUBT_POSCORRECTION && oneof3(atyp, PPAMT_CS_CASH, PPAMT_CS_BANK, PPAMT_CS_CSCARD))
				skip_amt = 1;
			if(!skip_amt) {
				PPAmountType2 amtt_rec;
				if(amtt_obj.Fetch(atyp, &amtt_rec) > 0) {
					if(amtt_rec.Flags & PPAmountType::fManual)
						if(!(amtt_rec.Flags & PPAmountType::fTax) || !op_type_list.lsearch(OpTypeID))
							continue;
				}
				Amounts.atFree(i);
			}
		}
		uint   pos = 0;
		if(pList->Search(PPAMT_MAIN, Rec.CurID, &pos))
			Rec.Amount = BR2(pList->at(pos).Amt);
		Amounts.Put(pList, 1, 1);
		DistributeExtCost();
	}
	{
		//
		// Этот участок кода перенесен из функции PPBillPacket::SumAmounts
		// из-за того, что номинальная сумма документа должна рассчитываться исходя //
		// из уже сформированного списка сумм документа.
		//
		int    do_calc_amount = (Rec.Flags & BILLF_FIXEDAMOUNTS) ? 0 : 1;
		SString amt_formula;
		PPObjOprKind op_obj;
		int    r = op_obj.GetExtStrData(Rec.OpID, OPKEXSTR_AMTFORMULA, amt_formula);
		if(r > 0 && amt_formula.NotEmptyS()) {
			const char * p_ignfix = "ignfix";
			if(amt_formula.HasPrefixIAscii(p_ignfix)) {
				amt_formula.ShiftLeft(sstrlen(p_ignfix));
				do_calc_amount = 1;
			}
			if(do_calc_amount && amt_formula.NotEmptyS()) {
				double main_amt = 0.0;
				if((r = PPCalcExpression(amt_formula, &main_amt, this, Rec.CurID, 0)) > 0) {
					Amounts.Put(PPAMT_MAIN, Rec.CurID, main_amt, 0, 1);
					Rec.Amount = BR2(main_amt);
				}
			}
		}
	}
	return 1;
}

int FASTCALL PPBillPacket::InitAmounts(int fromDB)
{
	AmtList al;
	if(!(Rec.Flags & BILLF_FIXEDAMOUNTS))
		if(!SumAmounts(&al, fromDB))
			return 0;
	return InitAmounts(&al);
}

bool PPBillPacket::SearchGoods(PPID goodsID, uint * pPos) const
{
	bool r = Lots.lsearch(&goodsID, pPos, CMPF_LONG, offsetof(PPTransferItem, GoodsID));
	if(!r) {
		goodsID = -goodsID;
		r = Lots.lsearch(&goodsID, pPos, CMPF_LONG, offsetof(PPTransferItem, GoodsID));
	}
	return r;
}

int PPBillPacket::HasOneOfGoods(const ObjIdListFilt & rList) const
{
	int    yes = 0;
	if(rList.GetCount()) {
		for(uint i = 0; !yes && i < rList.GetCount(); i++) {
			const  PPID goods_id = rList.Get(i);
			if(goods_id && Lots.lsearch(&goods_id, 0, CMPF_LONG, offsetof(PPTransferItem, GoodsID)))
				yes = 1;
		}
	}
	return yes;
}

bool PPBillPacket::SearchLot(PPID lotID, uint * pPos) const
	{ return Lots.lsearch(&lotID, pPos, CMPF_LONG, offsetof(PPTransferItem, LotID)); }
bool PPBillPacket::SearchShLot(PPID lotID, uint * pPos) const
	{ return P_ShLots ? P_ShLots->lsearch(&lotID, pPos, CMPF_LONG, offsetof(PPTransferItem, LotID)) : false; }

void PPBillPacket::AdjustLotQtty(PPID lotID, const PPTransferItem * pItem, int pos, double * pQtty) const
{
	// @fixme Есть сомнения насчет корректности работы блока при pItem != 0 в части проверок !pItem->IsRecomplete() и pItem->IsCorrectionExp()
	if(pQtty) {
		if(pItem) {
			if((Rec.Flags & (BILLF_GEXPEND|BILLF_GRECEIPT)) || (OpTypeID == PPOPT_GOODSMODIF && !pItem->IsRecomplete()) || pItem->IsCorrectionExp()) {
				P_BObj->trfr->SubtractBillQtty(Rec.ID, lotID, pQtty);
				for(uint i = 0; SearchLot(lotID, &i); i++) {
					if(static_cast<int>(i) != pos) {
						*pQtty += ConstTI(i).SQtty(Rec.OpID);
					}
				}
				if(pItem->LotID == lotID)
					*pQtty += pItem->SQtty(Rec.OpID);
			}
		}
		else {
			if((Rec.Flags & (BILLF_GEXPEND|BILLF_GRECEIPT)) || OpTypeID == PPOPT_GOODSMODIF) {
				P_BObj->trfr->SubtractBillQtty(Rec.ID, lotID, pQtty);
				for(uint i = 0; SearchLot(lotID, &i); i++) {
					if(static_cast<int>(i) != pos) {
						*pQtty += ConstTI(i).SQtty(Rec.OpID);
					}
				}
			}
		}
	}
}

int PPBillPacket::RestByLot(PPID lotID, const PPTransferItem * pTi, int pos, double * pRest) const
{
	int    ok = 1;
	double rest = 0.0;
	if(lotID)
		if(P_BObj->trfr->GetRest(lotID, Rec.Dt, &rest))
			AdjustLotQtty(lotID, pTi, pos, &rest);
		else
			ok = 0;
	ASSIGN_PTR(pRest, R6(rest));
	return ok;
}

int PPBillPacket::BoundsByLot(PPID lotID, const PPTransferItem * pItem, int pos, double * pMinusDelta, double * pPlusDelta) const
{
	if(pMinusDelta || pPlusDelta) {
		ASSIGN_PTR(pMinusDelta, 0.0);
		ASSIGN_PTR(pPlusDelta,  0.0);
		if(lotID) {
			if(P_BObj->trfr->GetBounds(lotID, Rec.Dt, -1, pMinusDelta, pPlusDelta)) {
				double zero = 0.0;
				AdjustLotQtty(lotID, pItem, pos, &zero);
				ASSIGN_PTR(pMinusDelta, R6(*pMinusDelta + zero));
				ASSIGN_PTR(pPlusDelta, R6(*pPlusDelta - zero));
			}
			else
				return 0;
		}
	}
	return 1;
}

int PPBillPacket::RestByOrderLot(PPID lotID, const PPTransferItem * pItem, int pos, double * pRest) const
{
	int    ok = 1;
	double rest = 0.0;
	PPTransferItem * p_ti;
	if(lotID) {
		if(P_BObj->trfr->GetRest(lotID, Rec.Dt, &rest)) {
			if(P_ShLots && Rec.Flags & (BILLF_GEXPEND | BILLF_GRECEIPT)) {
				PPIDArray exclude;
				for(uint i = 0; P_ShLots->enumItems(&i, (void **)&p_ti);) {
					if(p_ti->OrdLotID && !exclude.lsearch(p_ti->OrdLotID)) { // @ordlotid
						P_BObj->trfr->SubtractBillQtty(p_ti->OrdLotID, lotID, &rest); // @ordlotid
						exclude.add(p_ti->OrdLotID); // @ordlotid
					}
					if(p_ti->LotID == lotID && (int)(i-1) != pos)
						rest += p_ti->SQtty(Rec.OpID);
				}
				if(pItem && pItem->LotID == lotID)
					rest += pItem->SQtty(Rec.OpID);
			}
		}
		else
			ok = 0;
	}
	ASSIGN_PTR(pRest, rest);
	return ok;
}

int PPBillPacket::GoodsRest(PPID goodsID, const PPTransferItem * pItem, int pos, double * pRest, double * pReserve)
{
	int    ok = 1, r = 1;
	double lot_rest, total_rest = 0.0, reserve = 0.0;
	if(goodsID) {
		if(goodsID > 0 && pItem && pItem->Flags & PPTFR_ORDER) {
			pItem = 0;
			pos = -1;
		}
		LotArray lot_list;
		THROW(P_BObj->trfr->Rcpt.GetListOfOpenedLots(1, goodsID, Rec.LocID, Rec.Dt, &lot_list));
		for(uint i = 0; i < lot_list.getCount(); i++) {
			const ReceiptTbl::Rec & r_lot_rec = lot_list.at(i);
			if(goodsID > 0) {
				THROW(BoundsByLot(r_lot_rec.ID, pItem, pos, &lot_rest, 0));
			}
			else {
				THROW(RestByOrderLot(r_lot_rec.ID, pItem, pos, &lot_rest));
				if(r_lot_rec.Flags & LOTF_ORDRESERVE)
					reserve += lot_rest;
			}
			total_rest += lot_rest;
		}
	}
	THROW(r);
	CATCH
		ok = 0;
		total_rest = reserve = 0.0;
	ENDCATCH
	ASSIGN_PTR(pRest, total_rest);
	ASSIGN_PTR(pReserve, reserve);
	return ok;
}

int PPBillPacket::CalcShadowQuantity(PPID lotID, double * pQtty) const
{
	int    ok = 1;
	uint   i  = 0;
	double rest, q = 0.0;
	//
	// Вычисляем остаток по лоту заказа без данного пакета (pos).
	//
	if(SearchShLot(lotID, &i)) {
		if(!RestByOrderLot(lotID, 0, i, &rest))
			ok = 0;
		else {
			PPTransferItem * p_ti;
			for(i = 0; q < rest && Lots.enumItems(&i, (void **)&p_ti);)
				if(p_ti->Flags & PPTFR_ONORDER && p_ti->OrdLotID == lotID) // @ordlotid
					if((q += fabs(p_ti->Quantity_)) >= rest)
						q = rest;
		}
	}
	ASSIGN_PTR(pQtty, q);
	return ok;
}
//
// Helper. Called only from PPBillPacket::CalcModifCost
//
static void CalcTiModifCost(PPTransferItem & rTi, uint outCount, double sumCost, double sumPrice)
{
	if(!(rTi.Flags & PPTFR_FIXMODIFCOST)) {
		if(sumPrice == 0.0 && outCount == 1)
			rTi.Cost = TR5(sumCost / fabs(rTi.Qtty()));
		else
			rTi.Cost = TR5(sumCost * rTi.Price / sumPrice);
	}
}

static double CalcTiInputCost(const PPBillPacket & rBp, const PPTransferItem & rTi, double sumCost, int minus)
{
	double cost = (rTi.Flags & PPTFR_REVAL) ? rTi.RevalCost : rTi.Cost;
	if(rTi.Flags & PPTFR_UNLIM && cost == 0.0)
		cost = rTi.NetPrice();
	double c = R2(cost * fabs(rTi.Qtty()));
	if(rTi.Flags & PPTFR_COSTWOVAT) {
		GTaxVect gtv;
		gtv.CalcBPTI(rBp, rTi, TIAMT_COST);
		c += gtv.GetValue(GTAXVF_VAT);
	}
	return minus ? (sumCost-c) : (sumCost+c);
}

void PPBillPacket::CalcModifCost()
{
	uint   out_count = 0;
	double sum_cost = 0.0;
	double sum_price = 0.0;
	{
		for(uint tii = 0; tii < GetTCount(); tii++) {
			const PPTransferItem & r_ti = ConstTI(tii);
			if(r_ti.IsRecomplete()) {
				if(!(r_ti.Flags & PPTFR_FIXMODIFCOST)) {
					out_count++;
					sum_price += R2(r_ti.Price * fabs(r_ti.Qtty()));
				}
				sum_cost = CalcTiInputCost(*this, r_ti, sum_cost, 0);
			}
			else if(r_ti.Flags & PPTFR_MINUS)
				sum_cost = CalcTiInputCost(*this, r_ti, sum_cost, 0);
			else if(r_ti.Flags & PPTFR_PLUS) {
				if(Rec.Flags & BILLF_RECOMPLETE || r_ti.Flags & PPTFR_FIXMODIFCOST)
					sum_cost = CalcTiInputCost(*this, r_ti, sum_cost, 1);
				else {
					out_count++;
					sum_price += R2(r_ti.Price * fabs(r_ti.Qtty()));
				}
			}
		}
	}
	if(sum_price || out_count == 1) {
		for(uint tii = 0; tii < GetTCount(); tii++) {
			PPTransferItem & r_ti = TI(tii);
			if((r_ti.Flags & PPTFR_PLUS && !(Rec.Flags & BILLF_RECOMPLETE)) || r_ti.IsRecomplete()) {
				double cost = 0.0;
				if(r_ti.Flags & PPTFR_COSTWOVAT && !(r_ti.Flags & PPTFR_FIXMODIFCOST)) {
					CalcTiModifCost(r_ti, out_count, sum_cost, sum_price);
					cost = r_ti.Cost;
					GTaxVect gtv;
					r_ti.Flags &= ~PPTFR_COSTWOVAT;
					gtv.CalcBPTI(*this, r_ti, TIAMT_COST);
					r_ti.Flags |= PPTFR_COSTWOVAT;
					r_ti.Cost   = R2(r_ti.Cost - gtv.GetValue(GTAXVF_VAT) / fabs(r_ti.Qtty()));
				}
				else {
					CalcTiModifCost(r_ti, out_count, sum_cost, sum_price);
					cost = r_ti.Cost;
				}
				if(r_ti.Price == 0)
					r_ti.Price = cost;
				if(P_Outer && r_ti.LotID < 0)
					for(uint pos = 0; P_Outer->SearchLot(r_ti.LotID, &pos); pos++)
						P_Outer->TI(pos).Cost = r_ti.Cost;
			}
		}
	}
}

int PPBillPacket::CheckLargeBill(int genWarn) const
{
	int    is_max_items = 0;
	if(GetOpSubType(Rec.OpID) == OPSUBT_WARRANT) {
		if(AdvList.GetCount() > 9)
			is_max_items = 1;
	}
	else {
		const PPCommConfig & r_ccfg = CConfig;
		const uint max_items = (r_ccfg.MaxGoodsBillLines > 0) ? r_ccfg.MaxGoodsBillLines : 300;
		if(GetTCount() > max_items || (P_ACPack && P_ACPack->GetTCount() > max_items))
			is_max_items = 1;
	}
	if(is_max_items && genWarn)
		PPMessage(mfInfo|mfOK, PPINF_BILLTOOLARGE);
	return is_max_items;
}

int PPBillPacket::SetupVirtualTItems()
{
	int    ok = -1;
	if(!(ProcessFlags & pfHasVirtualTI) && GetTCount() == 0) {
		if(OpTypeID == PPOPT_CHARGE) {
			uint   i = 0;
			PPBillPacket link_pack;
			PPTransferItem * p_link_ti;
			double rel;
			SString msg_buf;
			msg_buf.Cat(Rec.LinkBillID);
			THROW_PP_S(P_BObj->ExtractPacket(Rec.LinkBillID, &link_pack) > 0, PPERR_LINKBILLNFOUND, msg_buf);
			rel = Rec.Amount / link_pack.Rec.Amount;
			while(link_pack.EnumTItems(&i, &p_link_ti) > 0) {
				PPTransferItem temp_ti(&Rec, 0);
				temp_ti.SetupGoods(p_link_ti->GoodsID);
				SETFLAG(temp_ti.Flags, PPTFR_RMVEXCISE, Rec.Flags & BILLF_RMVEXCISE);
				double qtty = fabs(p_link_ti->Quantity_);
				temp_ti.Quantity_ = qtty;
				temp_ti.Cost  = R2(rel * (p_link_ti->Cost * qtty) / qtty);
				temp_ti.Price = R2(rel * (p_link_ti->NetPrice() * qtty) / qtty);
				THROW(InsertRow(&temp_ti, 0));
				ok = 1;
			}
		}
		else {
			PPTransferItem temp_ti;
			if(temp_ti.InitAccturnInvoice(this) > 0) {
				THROW(InsertRow(&temp_ti, 0));
				ok = 1;
			}
		}
	}
	CATCH
		if(ok > 0)
			while(GetTCount())
				RemoveRow(0U);
		ok = 0;
	ENDCATCH
	if(ok > 0)
		ProcessFlags |= pfHasVirtualTI;
	return ok;
}

int PPBillPacket::RemoveVirtualTItems()
{
	if(ProcessFlags & pfHasVirtualTI) {
		while(GetTCount())
			RemoveRow(0U);
		return 1;
	}
	else
		return -1;
}

int PPBillPacket::GetMainOrgID_(PPID * pID) const
{
	PPID   main_org_id = 0;
	if(Rec.Object2 && P_BObj && P_BObj->GetConfig().Flags & BCF_EXTOBJASMAINORG) {
		PPObjArticle ar_obj;
		ArticleTbl::Rec ar_rec;
		if(ar_obj.Fetch(Rec.Object2, &ar_rec) > 0 && ar_rec.ObjID) {
			PPObjAccSheet acs_obj;
			if(acs_obj.IsLinkedToMainOrg(ar_rec.AccSheetID))
				main_org_id = ar_rec.ObjID;
		}
	}
	SETIFZ(main_org_id, GetMainOrgID());
	ASSIGN_PTR(pID, main_org_id);
	return BIN(main_org_id);
}
//
//
//
CompleteItem::CompleteItem()
{
	THISZERO();
}

CompleteArray::CompleteArray() : TSVector <CompleteItem> (), LotID(0), BillID(0)
{
}

CompleteArray::CompleteArray(const CompleteArray & s) : TSVector <CompleteItem> ()
{
	LotID  = s.LotID;
	BillID = s.BillID;
	SVector::copy(s);
}

CompleteArray & FASTCALL CompleteArray::operator = (const CompleteArray & s)
{
	LotID  = s.LotID;
	BillID = s.BillID;
	SVector::copy(s);
	return *this;
}

int CompleteArray::SearchLotID(PPID lotID, uint * pPos, CompleteItem * pItem) const
{
	CompleteItem * p_item;
	for(uint i = 0; enumItems(&i, reinterpret_cast<void**>(&p_item));)
		if(p_item->LotID == lotID && !(p_item->Flags & CompleteItem::fExclude)) {
			ASSIGN_PTR(pPos, i-1);
			ASSIGN_PTR(pItem, *p_item);
			return 1;
		}
	return 0;
}

int CompleteArray::SearchGoodsID(PPID goodsID, uint * pPos, CompleteItem * pItem) const
{
	CompleteItem * p_item;
	for(uint i = 0; enumItems(&i, reinterpret_cast<void**>(&p_item));)
		if(p_item->LotID == goodsID && !(p_item->Flags & CompleteItem::fExclude)) {
			ASSIGN_PTR(pPos, i-1);
			ASSIGN_PTR(pItem, *p_item);
			return 1;
		}
	return 0;
}

int CompleteArray::IsExcludedLot(PPID lotID) const
{
	CompleteItem * p_item;
	for(uint i = 0; enumItems(&i, reinterpret_cast<void**>(&p_item));)
		if(p_item->LotID == lotID && p_item->Flags & CompleteItem::fExclude)
			return 1;
	return 0;
}

int CompleteArray::RemoveExludedItems(PPID exclLotID)
{
	int    ok = -1;
	CompleteItem * p_item;
	for(uint i = 0; enumItems(&i, reinterpret_cast<void**>(&p_item));)
		if(p_item->LotID == exclLotID) {
			atFree(--i);
			ok = 1;
		}
	return ok;
}

int PPBillPacket::GetComplete(PPID lotID, CompleteArray * pList)
{
	int    ok = -1;
	uint   pos = 0;
	if(SearchLot(lotID, &pos)) {
		const PPTransferItem & r_lead_ti = ConstTI(pos);
		PPTransferItem * p_ti = 0;
		if(r_lead_ti.Flags & PPTFR_PLUS || r_lead_ti.IsRecomplete()) {
			SString serial;
			for(uint p = 0; EnumTItems(&p, &p_ti);) {
				if(p_ti->Flags & PPTFR_MINUS) {
					if(!pList->IsExcludedLot(p_ti->LotID)) {
						CompleteItem item;
						item.LotID   = p_ti->LotID;
						item.BillID  = p_ti->BillID;
						item.GoodsID = p_ti->GoodsID;
						item.Dt      = p_ti->Date;
						item.Expiry  = p_ti->Expiry;
						item.ArID    = Rec.Object;
						item.Qtty    = fabs(p_ti->Quantity_);
						item.Cost    = p_ti->Cost;
						item.Price   = p_ti->Price;
						item.Flags  |= CompleteItem::fSource;
						LTagL.GetTagStr(p-1, PPTAG_LOT_SN, serial);
						serial.CopyTo(item.Serial, sizeof(item.Serial));
						THROW_SL(pList->insert(&item));
						ok = 1;
					}
				}
				else if(p_ti->Flags & PPTFR_PLUS && !p_ti->IsReceipt() && p_ti->LotID) {
					CompleteItem item;
					item.LotID   = p_ti->LotID;
					item.GoodsID = p_ti->GoodsID;
					item.Dt      = p_ti->Date;
					item.Expiry  = p_ti->Expiry;
					item.ArID    = Rec.Object;
					item.Qtty    = fabs(p_ti->Quantity_);
					item.Cost    = p_ti->Cost;
					item.Price   = p_ti->Price;
					LTagL.GetTagStr(p-1, PPTAG_LOT_SN, serial);
					serial.CopyTo(item.Serial, sizeof(item.Serial));
					item.Flags  |= CompleteItem::fExclude;
					THROW_SL(pList->insert(&item));
					pList->RemoveExludedItems(p_ti->LotID);
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
TiIter::IndexItem::IndexItem(long tiPos, long ext, long disposePos) : TiPos(tiPos), Ext(ext), DisposePos(disposePos)
{
}

TiIter::TiIter(PPBillPacket * pPack, long flags, long filtGrpID, Order o) : AccsCost(BillObj->CheckRights(BILLRT_ACCSCOST))
{
	Init(pPack, flags, filtGrpID, o);
}

int FASTCALL TiIter::IsPassedIdx(int idx) const
{
	return BIN(Seen.lsearch(idx));
}

int TiIter::OrderRows_Mem(const PPBillPacket * pPack, Order o)
{
	int    ok = 1;
	PPObjArticle * p_ar_obj = 0;
	PPObjLocation * p_loc_obj = 0;
	GoodsToObjAssoc * p_gto_assc = 0;
	PPObjQCert * p_qcert_obj = 0;
	if(Flags & ETIEF_DISPOSE || FiltGrpID || oneof8(o, ordByGoods, ordByGrpGoods, ordByBarcode, ordBySuppl,
		ordByLocation, ordByPLU, ordByQCert, ordByStorePlaceGrpGoods)) {
		SString grp_name, goods_name, temp_buf;
		uint   i = 0;
		long   uniq_counter = 0;
		struct Ext { // @flat
			long   Uc;
			long   Pos;
			long   Extra;
			long   DispPos;
		};
		Ext    ext;
		SVector ext_list(sizeof(Ext));
		PPTransferItem * p_ti;
		PPIDArray scale_alt_grp_list;
		PPIDArray skip_list;
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		StrAssocArray ord_list;
		if(Flags & ETIEF_DISPOSE) {
			LocTransfCore ltt;
			ltt.GetDisposition(pPack->Rec.ID, DispList);
		}
		if(o == ordBySuppl) {
			THROW_MEM(p_ar_obj = new PPObjArticle);
		}
		else if(o == ordByLocation) {
			THROW_MEM(p_loc_obj = new PPObjLocation);
			THROW_MEM(p_gto_assc = new GoodsToObjAssoc(PPASS_GOODS2LOC, PPOBJ_LOCATION));
			THROW(p_gto_assc->Load());
		}
		else if(o == ordByStorePlaceGrpGoods) {
			THROW_MEM(p_loc_obj = new PPObjLocation);
			THROW_MEM(p_gto_assc = new GoodsToObjAssoc(PPASS_GOODS2WAREPLACE, PPOBJ_LOCATION, 1));
			THROW(p_gto_assc->Load());
		}
		else if(o == ordByPLU) {
			PPScale scl_rec;
			PPObjScale scl_obj;
			for(PPID scl_id = 0; scl_obj.EnumItems(&scl_id, &scl_rec) > 0;)
				if(scl_rec.AltGoodsGrp)
					scale_alt_grp_list.addUnique(scl_rec.AltGoodsGrp);
		}
		else if(o == ordByQCert) {
			THROW_MEM(p_qcert_obj = new PPObjQCert);
		}
		uint   saldo_list_pos = 0;
		int    rf = 0;
		while((rf = pPack->EnumTItems(&i, &p_ti)) > 0 || saldo_list_pos < SaldoList.getCount()) {
			const  PPID goods_id = (rf > 0) ? p_ti->GoodsID : SaldoList.at(saldo_list_pos).Key;
			if(goods_obj.BelongToGroup(goods_id, FiltGrpID, 0) > 0 && (skip_list.addUnique(goods_id) > 0 || rf > 0)) {
				const  PPID suppl_id = (rf > 0) ? p_ti->Suppl : 0;
				int    to_add_entry = 1;
				size_t nl = 0, gl = 0;
				grp_name.Z();
				goods_name.Z();
				if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
					goods_name = goods_rec.Name;
					if(Flags & ETIEF_DISPOSE) {
						for(uint j = 0; j < DispList.getCount(); j++) {
							if(DispList.at(j).RByBill == p_ti->RByBill) {
								ord_list.Add(++uniq_counter, temp_buf.Z().Cat(goods_name), 1);
								ext.Uc = uniq_counter;
								ext.Pos = i-1;
								ext.Extra = 0;
								ext.DispPos = j;
								ext_list.insert(&ext);
								to_add_entry = 0;
							}
						}
					}
					{
						if(o == ordDefault) {
							grp_name.Z().Cat(i);
							goods_name.Z();
						}
						else if(o == ordByGrpGoods)
							goods_obj.P_Tbl->MakeFullName(goods_rec.ParentID, 0, grp_name);
						else if(o == ordByBarcode)
							goods_obj.GetSingleBarcode(goods_rec.ID, 0, goods_name);
						else if(o == ordBySuppl)
							GetArticleName(suppl_id, grp_name);
						else if(o == ordByLocation) {
							PPID   loc_id = 0;
							if(p_gto_assc && p_gto_assc->Get(goods_rec.ID, &loc_id) > 0)
								GetLocationName(loc_id, grp_name);
						}
						else if(o == ordByPLU) {
							Goods2Tbl::Rec grp_rec;
							ObjAssocTbl::Rec assc_rec;
							for(SEnum en = PPRef->Assc.Enum(PPASS_ALTGOODSGRP, goods_rec.ID, 1); en.Next(&assc_rec) > 0;) {
								const  PPID grp_id = assc_rec.PrmrObjID;
								if(scale_alt_grp_list.lsearch(grp_id) && goods_obj.Fetch(grp_id, &grp_rec) > 0) {
									ord_list.Add(++uniq_counter, temp_buf.Z().Cat(grp_rec.Name).Cat(goods_name), 1);
									ext.Uc = uniq_counter;
									ext.Pos = i-1;
									ext.Extra = assc_rec.ID;
									ext.DispPos = -1;
									ext_list.insert(&ext);
									to_add_entry = 0;
								}
							}
						}
						else if(o == ordByQCert) {
							if(p_qcert_obj) {
								PPObjBill * p_bobj = BillObj;
								ReceiptTbl::Rec lot_rec;
								PPIDArray qcert_list;
								QualityCertTbl::Rec qcert_rec;
								for(DateIter di; p_bobj->trfr->Rcpt.EnumByGoods(goods_rec.ID, &di, &lot_rec) > 0;) {
									if(lot_rec.QCertID && p_qcert_obj->Search(lot_rec.QCertID, &qcert_rec) > 0) {
										if(!qcert_rec.Passive && !qcert_list.lsearch(qcert_rec.ID)) {
											ord_list.Add(++uniq_counter, temp_buf.Z().Cat(qcert_rec.Code).Cat(goods_name), 1);
											ext.Uc = uniq_counter;
											ext.Pos = i-1;
											ext.Extra = qcert_rec.ID;
											ext.DispPos = -1;
											ext_list.insert(&ext);
											qcert_list.addUnique(qcert_rec.ID);
											to_add_entry = 0;
										}
									}
								}
							}
						}
						else if(o == ordByStorePlaceGrpGoods) {
							PPID   loc_id = 0;
							PPIDArray loc_list;
							LocationTbl::Rec wp_rec;
							if(p_loc_obj && p_gto_assc && p_gto_assc->GetListByGoods(goods_rec.ID, loc_list) > 0) {
								for(uint j = 0; to_add_entry && j < loc_list.getCount(); j++) {
									const  PPID _loc_id = loc_list.get(j);
									PPID  _wh_id = 0;
									if(p_loc_obj->Fetch(_loc_id, &wp_rec) > 0 && p_loc_obj->GetParentWarehouse(_loc_id, &_wh_id) > 0 && _wh_id == pPack->Rec.LocID) {
										goods_obj.P_Tbl->MakeFullName(goods_rec.ParentID, 0, grp_name);
										temp_buf.Z().Cat(wp_rec.Name).Cat(grp_name).Cat(goods_name);
										ord_list.Add(++uniq_counter, temp_buf, 1);
										ext.Uc = uniq_counter;
										ext.Pos = i-1;
										ext.Extra = loc_list.get(j);
										ext.DispPos = -1;
										ext_list.insert(&ext);
										to_add_entry = 0;
									}
								}
							}
							if(to_add_entry) {
								goods_obj.P_Tbl->MakeFullName(goods_rec.ParentID, 0, grp_name);
								temp_buf.Z().CatCharN('0', 20).Cat(grp_name);
								grp_name = temp_buf;
							}
						}
					}
				}
				if(to_add_entry) {
					ord_list.Add(++uniq_counter, (temp_buf = grp_name).Cat(goods_name), -1);
					ext.Uc = uniq_counter;
					ext.Pos = (rf > 0) ? (i-1) : (saldo_list_pos + SALDOLIST_POS_BIAS);
					ext.Extra = 0;
					ext.DispPos = -1;
					ext_list.insert(&ext);
				}
			}
			if(rf <= 0)
				saldo_list_pos++;
		}
		{
			ord_list.SortByText();
			for(i = 0; i < ord_list.getCount(); i++) {
				StrAssocArray::Item oi = ord_list.Get(i);
				uint   elp = 0;
				int    r = ext_list.lsearch(&oi.Id, &elp, CMPF_LONG);
				assert(r);
				Ext * p_ext = static_cast<Ext *>(ext_list.at(elp));
				IndexItem ii(p_ext->Pos, p_ext->Extra, p_ext->DispPos);
				THROW_SL(Index.insert(&ii));
			}
			{
				PPEquipConfig eq_cfg;
				ReadEquipConfig(&eq_cfg);
				if(eq_cfg.Flags & PPEquipConfig::fUseQuotAsPrice)
					Flags |= ETIEF_LABELQUOTPRICE;
			}
			UseIndex = 1;
		}
	}
	CATCH
		ok = 0;
		Index.freeAll();
		UseIndex = 0;
	ENDCATCH
	delete p_ar_obj;
	delete p_loc_obj;
	delete p_gto_assc;
	delete p_qcert_obj;
	return ok;
}

int TiIter::Init(const PPBillPacket * pPack, long flags, long filtGrpID, Order o)
{
	Flags = flags;
	Flags &= ~ETIEF_LABELQUOTPRICE; // internal flag
	I = PckgI = PckgItemI = 0;
	FiltGrpID = filtGrpID;
	Seen.freeAll();
	Index.freeAll();
	SaldoList.freeAll();
	DispList.freeAll();
	UseIndex = 0;
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	if(FiltGrpID && Flags & ETIEF_SALDOFILTGRP) {
		const  PPID dlvr_loc_id = pPack->GetDlvrAddrID();
		PPIDArray temp_list;
		GoodsIterator::GetListByGroup(FiltGrpID, &temp_list);
		for(uint i = 0; i < temp_list.getCount(); i++) {
			const  PPID goods_id = temp_list.get(i);
			double saldo = 0.0;
			int    is_present = 0;
			double qtty = 0.0;
			long   oprno = 0;
			TransferTbl::Rec rec;
			for(uint pos = 0; !oprno && pPack->SearchGoods(goods_id, &pos); pos++) {
				is_present = 1;
				const PPTransferItem & r_ti = pPack->ConstTI(pos);
				if(r_ti.RByBill && p_bobj->trfr->SearchByBill(r_ti.BillID, 0, r_ti.RByBill, &rec) > 0)
					oprno = rec.OprNo;
			}
			p_bobj->GetGoodsSaldo(goods_id, pPack->Rec.Object, dlvr_loc_id, pPack->Rec.Dt, oprno, &saldo, 0);
			if(is_present || saldo != 0.0)
				SaldoList.Add(goods_id, saldo);
		}
	}
	if(FiltGrpID || Flags & ETIEF_DISPOSE)
		ok = OrderRows_Mem(pPack, o);
	else if(oneof8(o, ordByGoods, ordByGrpGoods, ordByBarcode, ordBySuppl, ordByLocation, ordByPLU, ordByQCert, ordByStorePlaceGrpGoods)) {
		if(pPack->GetTCount() > 1 || oneof2(o, ordByPLU, ordByQCert))
			ok = OrderRows_Mem(pPack, o);
	}
	return ok;
}

static int CanMerge(const PPTransferItem * pTI1, const PPTransferItem * pTI2, long f)
{
	int    yes = 1;
	if(f & ETIEF_DONTUNITE)
		yes = 0;
	else if(!(f & ETIEF_FORCEUNITEGOODS && pTI1->GoodsID == pTI2->GoodsID)) {
		if(f & ETIEF_DISPOSE)
			yes = 0;
		else if(!(f & ETIEF_UNITEBYGOODS) || (pTI1->GoodsID != pTI2->GoodsID))
			yes = 0;
		else if((pTI1->Flags & (PPTFR_PLUS|PPTFR_MINUS)) != (pTI2->Flags & (PPTFR_PLUS|PPTFR_MINUS)))
			yes = 0;
		else if(f & ETIEF_DIFFBYLOT && pTI1->LotID != pTI2->LotID)
			yes = 0;
		else if(f & ETIEF_DIFFBYCOST && pTI1->Cost != pTI2->Cost)
			yes = 0;
		else if(f & ETIEF_DIFFBYPRICE && pTI1->Price != pTI2->Price)
			yes = 0;
		else if(f & ETIEF_DIFFBYNETPRICE && R6(pTI1->NetPrice() - pTI2->NetPrice()) != 0L)
			yes = 0;
		else if(f & ETIEF_DIFFBYPACK && pTI1->UnitPerPack != pTI2->UnitPerPack)
			yes = 0;
		else if(f & ETIEF_DIFFBYQCERT && pTI1->QCert != pTI2->QCert)
			yes = 0;
		else if(f & ETIEF_DIFFBYINTAXGRP && pTI1->LotTaxGrpID != pTI2->LotTaxGrpID)
			yes = 0;
		else if((pTI1->Flags & PPTFR_PRICEWOTAXES) != (pTI2->Flags & PPTFR_PRICEWOTAXES))
			yes = 0;
		else if(pTI1->GoodsID == pTI2->GoodsID && pTI1->LotID != pTI2->LotID) {
			PPGdsClsPacket gc_pack;
			PPObjGoods goods_obj;
			if(goods_obj.FetchCls(pTI1->GoodsID, 0, &gc_pack) > 0 && gc_pack.Rec.Flags & PPGdsCls::fPrintDiffLots)
				yes = 0;
		}
	}
	return yes;
}

void PPBillPacket::InitExtTIter(long f, long filtGrpID, TiIter::Order o)
{
	delete P_Iter;
	P_Iter = new TiIter(this, f, filtGrpID, o);
}

int PPBillPacket::MergeTI(PPTransferItem * pTI, int idx, long flags, LongArray & rSaw, LongArray * pMergePosList)
{
	int    ok = -1;
	PPTransferItem * p_ti;
	if(pMergePosList) {
		pMergePosList->clear();
		pMergePosList->add(idx);
	}
	for(uint i = 0; EnumTItems(&i, &p_ti);) {
		int    ii = static_cast<int>(i-1);
		if(ii != idx && !rSaw.lsearch(ii) && CanMerge(pTI, p_ti, flags)) {
			const double q1 = pTI->Quantity_;
			const double q2 = p_ti->Quantity_;
			const double q = q1 + q2;
			const double s_np = (pTI->NetPrice() * q1 + p_ti->NetPrice() * q2);
			const double s_p  = (pTI->Price * q1 + p_ti->Price * q2);
			const double s_d  = s_p - s_np;
			const double p    = s_p / q;
			const double d    = s_d / q;
			pTI->Quantity_ = q;
			pTI->Price    = p;
			pTI->Discount = d;
			pTI->Cost     = (pTI->Cost * q1 + p_ti->Cost * q2) / q;
			pTI->CurPrice = (pTI->CurPrice * q1 + p_ti->CurPrice * q2) / q;
			rSaw.add(ii);
			CALLPTRMEMB(pMergePosList, add(ii));
			ok = 1;
		}
	}
	return ok;
}

int PPBillPacket::GetNextPLU(TiIter * pI, long * pPLU, SString & rObjAsscName)
{
	int    ok = -1;
	long   plu = 0;
	TiIter * p_iter = NZOR(pI, P_Iter);
	if(p_iter) {
		uint   pos = p_iter->I-1;
		TiIter::IndexItem * p_p = 0;
		if(p_iter->Index.enumItems(&pos, reinterpret_cast<void **>(&p_p))) {
			ObjAssocTbl::Rec oa_rec;
			if(PPRef->Assc.Search(p_p->Ext, &oa_rec) > 0) {
				plu = oa_rec.InnerNum;
				GetGoodsName(oa_rec.PrmrObjID, rObjAsscName);
			}
			ok = 1;
		}
	}
	ASSIGN_PTR(pPLU, plu);
	return ok;
}

int PPBillPacket::EnumTItemsExt(TiIter * pI, PPTransferItem * pTI, TiItemExt * pExt)
{
	PPTransferItem ti, * p_ti;
	TiIter * p_i = NZOR(pI, P_Iter);
	CALLPTRMEMB(pExt, Clear());
	if(p_i) {
		if(P_PckgList) {
			uint   temp_pckg_i = p_i->PckgI;
			LPackage * p_pckg = 0;
			int    idx;
			while(P_PckgList->EnumItems(&temp_pckg_i, &p_pckg) > 0) {
				while(p_pckg->EnumItems(&p_i->PckgItemI, &idx, 0) > 0) {
					if(!p_i->IsPassedIdx(idx)) {
						*pTI = ConstTI(idx);
						pTI->RByBill = idx;
						p_i->Seen.add(idx);
						if(pExt) {
							LTagL.GetTagStr(idx, PPTAG_LOT_CLB, pExt->Clb);
							pExt->Pckg = p_pckg->Code;
						}
						return 1;
					}
				}
				p_i->PckgI = temp_pckg_i;
				p_i->PckgItemI = 0;
			}
		}
		if(p_i->UseIndex) {
			TiIter::IndexItem * p_p = 0;
			PPObjGoods g_obj;
			while(p_i->Index.enumItems(&p_i->I, (void **)&p_p)) {
				const  int _idx = p_p->TiPos;
				uint   temp_idx = _idx;
				int    rf = 0; // 2 - tare saldo
				if(temp_idx < SALDOLIST_POS_BIAS) {
					rf = EnumTItems(&temp_idx, &p_ti);
					if(rf > 0) {
						if(!(p_ti->Flags & PPTFR_PCKG) && !p_i->IsPassedIdx(_idx))
							ti = *p_ti;
						else
							rf = 0;
					}
				}
				else if((temp_idx-SALDOLIST_POS_BIAS) < p_i->SaldoList.getCount()) {
					ti.GoodsID = p_i->SaldoList.at(temp_idx-SALDOLIST_POS_BIAS).Key;
					ti.BillID = Rec.ID;
					ti.Date = Rec.Dt;
					ti.LocID = Rec.LocID;
					rf = 2;
				}
				if(rf > 0) {
					ti.RByBill = (temp_idx < SALDOLIST_POS_BIAS) ? _idx : 0;
					if(rf != 2) {
						if(p_i->Flags & ETIEF_UNITEBYGOODS)
							MergeTI(&ti, _idx, p_i->Flags, p_i->Seen, (pExt ? &pExt->MergePosList : 0));
						else if(pExt)
							pExt->MergePosList.add(_idx);
					}
					if(ProcessFlags & pfPrintPLabel) {
						if(p_i->Flags & ETIEF_LABELQUOTPRICE) {
							double quot = 0.0;
							const QuotIdent qi(QIDATE(Rec.Dt), Rec.LocID, PPQUOTK_BASE);
							if(g_obj.GetQuotExt(ti.GoodsID, qi, ti.Cost, ti.Price, &quot, 1) > 0)
								ti.Price = quot;
						}
					}
					else if(ProcessFlags & pfPrintQCertList)
						ti.QCert = p_p->Ext;
					else
						p_i->Seen.add(_idx);
					*pTI = ti;
					if(!p_i->IsAccsCost())
						pTI->Cost = 0.0;
					if(pExt) {
						LTagL.GetTagStr(_idx, PPTAG_LOT_CLB, pExt->Clb);
						if(p_i->Flags & ETIEF_DISPOSE && p_p->DisposePos >= 0 && p_p->DisposePos < p_i->DispList.getCountI())
							pExt->LctRec = p_i->DispList.at(p_p->DisposePos);
					}
					return 1;
				}
			}
		}
		else {
			while(EnumTItems(&p_i->I, &p_ti) > 0) {
				const  int _idx = p_i->I-1;
				if(!(p_ti->Flags & PPTFR_PCKG) && !p_i->IsPassedIdx(_idx)) {
					ti = *p_ti;
					if(!(p_i->Flags & ETIEF_DONTUNITE))
						ti.RByBill = _idx;
					if(p_i->Flags & ETIEF_UNITEBYGOODS)
						MergeTI(&ti, _idx, p_i->Flags, p_i->Seen, (pExt ? &pExt->MergePosList : 0));
					else if(pExt)
						pExt->MergePosList.add(_idx);
					*pTI = ti;
					p_i->Seen.add(_idx);
					if(!p_i->IsAccsCost())
						pTI->Cost = 0;
					if(pExt)
						LTagL.GetTagStr(_idx, PPTAG_LOT_CLB, pExt->Clb);
					return 1;
				}
			}
		}
	}
	return 0;
}

int PPBillPacket::CompareTIByCorrection(long tidFlags, long filtGrpID, TSCollection <TiDifferenceItem> & rDiffList)
{
	rDiffList.freeAll();
	int    ok = 1;
	PPIDArray cor_bill_list;
	BillTbl::Rec cor_bill_rec;
	for(DateIter diter; P_BObj->P_Tbl->EnumLinks(Rec.ID, &diter, BLNK_CORRECTION, &cor_bill_rec) > 0;) {
		cor_bill_list.add(cor_bill_rec.ID);
	}
	if(cor_bill_list.getCount()) {
		PPBillPacket cor_bp;
		for(uint i = 0; i < cor_bill_list.getCount(); i++) {
			const  PPID cor_bill_id = cor_bill_list.get(i);
			THROW(P_BObj->ExtractPacket(cor_bill_id, &cor_bp) > 0);
			for(uint j = 0; j < cor_bp.GetTCount(); j++) {
				const PPTransferItem & r_other = cor_bp.ConstTI(j);
				uint lp = 0;
				if(SearchLot(r_other.LotID, &lp)) {
					const PPTransferItem & r_ti = ConstTI(lp);
					long   dif = 0;
					if(tidFlags & tidfQtty && !feqeps(r_ti.Quantity_, r_other.Quantity_, 1.0E-6))
						dif |= tidfQtty;
					if(tidFlags & tidfCost && !feqeps(r_ti.Cost, r_other.Cost, 1.0E-6))
						dif |= tidfCost;
					if(tidFlags & tidfPrice && !feqeps(r_ti.Price, r_other.Price, 1.0E-6))
						dif |= tidfPrice;
					if(tidFlags & tidfNetPrice && !feqeps(r_ti.NetPrice(), r_other.NetPrice(), 1.0E-6))
						dif |= tidfNetPrice;
					if(dif) {
						LongArray tpl, opl;
						tpl.add(lp);
						opl.add(j);
						TiDifferenceItem * p_new_item = new TiDifferenceItem(dif, &tpl, &opl);
						p_new_item->ThisQtty = r_ti.Quantity_;
						p_new_item->OtherQtty = r_other.Quantity_;
						p_new_item->ThisCost = r_ti.Cost;
						p_new_item->OtherCost = r_other.Cost;
						p_new_item->ThisPrice = r_ti.Price;
						p_new_item->OtherPrice = r_other.Price;
						p_new_item->ThisNetPrice = r_ti.NetPrice();
						p_new_item->OtherNetPrice = r_other.NetPrice();
						{
							uint   k = rDiffList.getCount();
							if(k) do {
								TiDifferenceItem * p_di = rDiffList.at(--k);
								if(p_di->ThisPList.lsearch(lp)) {
									//
									// Если есть более одной корректировки на один лот исходного
									// документа, то считаем актуальной наиболее позднюю (существующую удаляем).
									//
									p_di = 0;
                                    rDiffList.atFree(k);
								}
							} while(k);
						}
						rDiffList.insert(p_new_item);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPBillPacket::CompareTI(PPBillPacket & rS, long tidFlags, long filtGrpID, TSCollection <TiDifferenceItem> & rDiffList)
{
	rDiffList.freeAll();
	int    ok = 1;
	TiItemExt et, eo;
	PPTransferItem ti, ti_other;
	const long etief = (tidFlags & tidfRByBillPrec) ? ETIEF_DONTUNITE : ETIEF_UNITEBYGOODS;
	{
		for(TiIter iter(this, etief, filtGrpID); EnumTItemsExt(&iter, &ti, &et) > 0;) {
			int    found = 0;
			for(TiIter iter_other(&rS, etief, filtGrpID); !found && rS.EnumTItemsExt(&iter_other, &ti_other, &eo) > 0;) {
				if(!(tidFlags & tidfRByBillPrec) || ti_other.RByBill == ti.RByBill) {
					if(ti_other.GoodsID == ti.GoodsID || (tidFlags & tidfIgnoreGoods)) {
						if((tidFlags & tidfIgnoreSign) || (ti.Flags & (PPTFR_PLUS|PPTFR_MINUS)) == (ti_other.Flags & (PPTFR_PLUS|PPTFR_MINUS)))
							found = 1;
					}
				}
			}
			if(found) {
				long   dif = 0;
				const  double qtty = (tidFlags & tidfIgnoreSign) ? fabs(ti.Quantity_) : ti.Quantity_;
				const  double other_qtty = (tidFlags & tidfIgnoreSign) ? fabs(ti_other.Quantity_) : ti_other.Quantity_;
				if(tidFlags & tidfQtty && !feqeps(qtty, other_qtty, 1.0E-6))
					dif |= tidfQtty;
				if(tidFlags & tidfCost && !feqeps(ti.Cost, ti_other.Cost, 1.0E-6))
					dif |= tidfCost;
				if(tidFlags & tidfPrice && !feqeps(ti.Price, ti_other.Price, 1.0E-6))
					dif |= tidfPrice;
				if(tidFlags & tidfNetPrice && !feqeps(ti.NetPrice(), ti_other.NetPrice(), 1.0E-6))
					dif |= tidfNetPrice;
				if(dif) {
					TiDifferenceItem * p_new_item = new TiDifferenceItem(dif, &et.MergePosList, &eo.MergePosList);
					p_new_item->ThisQtty = qtty;
					p_new_item->OtherQtty = other_qtty;
					p_new_item->ThisCost = ti.Cost;
					p_new_item->OtherCost = ti_other.Cost;
					p_new_item->ThisPrice = ti.Price;
					p_new_item->OtherPrice = ti_other.Price;
					p_new_item->ThisNetPrice = ti.NetPrice();
					p_new_item->OtherNetPrice = ti_other.NetPrice();
					rDiffList.insert(p_new_item);
				}
			}
			else {
				TiDifferenceItem * p_new_item = new TiDifferenceItem(tidfOtherAbsent, &et.MergePosList, 0);
				p_new_item->ThisQtty = ti.Quantity_;
				p_new_item->ThisCost = ti.Cost;
				p_new_item->ThisPrice = ti.Price;
				p_new_item->ThisNetPrice = ti.NetPrice();
				rDiffList.insert(p_new_item);
			}
		}
	}
	{
		//
		// Идентифицируем позиции, которые есть в other-пакете, но отсутствуют в this
		//
		for(TiIter iter_other(&rS, etief, filtGrpID); rS.EnumTItemsExt(&iter_other, &ti_other, &eo) > 0;) {
			int    found = 0;
			for(TiIter iter(this, etief, filtGrpID); !found && EnumTItemsExt(&iter, &ti, &et) > 0;) {
				if(!(tidFlags & tidfRByBillPrec) || ti_other.RByBill == ti.RByBill) {
					if(ti_other.GoodsID == ti.GoodsID && (ti.Flags & (PPTFR_PLUS|PPTFR_MINUS)) == (ti_other.Flags & (PPTFR_PLUS|PPTFR_MINUS)))
						found = 1;
				}
			}
			if(!found) {
				TiDifferenceItem * p_new_item = new TiDifferenceItem(tidfThisAbsent, 0, &eo.MergePosList);
				p_new_item->OtherQtty = ti_other.Quantity_;
				p_new_item->OtherCost = ti_other.Cost;
				p_new_item->OtherPrice = ti_other.Price;
				p_new_item->OtherNetPrice = ti_other.NetPrice();
				rDiffList.insert(p_new_item);
			}
		}
	}
	return ok;
}
//
//
//
BillViewItem::BillViewItem() : Debit(0.0), Credit(0.0), Saldo(0.0), LastPaymDate(ZERODATE)
{
}

BillTotal::BillTotal() : Count(0), Sum(0.0), Debt(0.0), InSaldo(0.0), Debit(0.0), Credit(0.0), OutSaldo(0.0)
{
}

BillTotal & BillTotal::Z()
{
	Count = 0;
	Sum = 0.0;
	Debt = 0.0;
	Amounts.clear();
	InSaldo = 0.0;
	Debit = 0.0;
	OutSaldo = 0.0;
	return *this;
}
//
//
//
BillTotalData::BillTotalData()
{
	Z();
}

BillTotalData & BillTotalData::Z()
{
	memzero(&LinesCount, offsetof(BillTotalData, Amounts));
	Amounts.clear();
	VatList.clear();
	CostVatList.clear();
	PriceVatList.clear();
	return *this;
}

BillVatArray::BillVatArray() : TSVector <BillVatEntry>()
{
}

const BillVatEntry * BillVatArray::GetByRate(double rate) const
{
	for(uint i = 0; i < count; i++) {
		const BillVatEntry & r_entry = at(i);
		if(feqeps(r_entry.Rate, rate, 1E-5))
			return &r_entry;
	}
	return 0;
}

int BillVatArray::Add(double rate, double sum, double base, double amtByVat)
{
	int    ok = 0;
	for(uint i = 0; !ok && i < count; i++) {
		BillVatEntry & r_entry = at(i);
		if(feqeps(r_entry.Rate, rate, 1E-5)) {
			r_entry.VatSum += sum;
			r_entry.BaseAmount += base;
			r_entry.AmountByVat += amtByVat;
			ok = 1;
		}
	}
	if(!ok) {
		BillVatEntry entry;
		entry.Rate = rate;
		entry.VatSum = sum;
		entry.BaseAmount = base;
		entry.AmountByVat = amtByVat;
		ok = ordInsert(&entry, 0, PTR_CMPFUNC(double)) ? 1 : PPSetErrorSLib();
	}
	return ok;
}

BillTotalBlock::BillTotalBlock(BillTotalData & rData, /*PPID opID*/const PPBillPacket & rBPack, PPID goodsTypeID, int outAmtType, long flags) :
	State(0), R_Data(rData), /*OpID(opID)*/R_BPack(rBPack), GoodsTypeID(goodsTypeID), OutAmtType(outAmtType), Flags(flags)
{
	SETFLAG(State, stSelling, BIN(IsSellingOp(R_BPack.Rec.OpID) > 0));
	State |= stAllGoodsUnlimUndef;
	DynGoodsTypeForSupplAgent = CConfig.DynGoodsTypeForSupplAgent;
}

void FASTCALL BillTotalBlock::Add(const PPAdvBillItemList::Item & rItem)
{
	R_Data.Amt += rItem.Amount;
}
//
// This is temporary and wrong function. Must be corrected !!!
//
void BillTotalBlock::AddPckg(const PPTransferItem * /*pTI*/)
{
	R_Data.PackCount++;
}

void BillTotalBlock::SetupStdAmount(PPID stdAmtID, PPID altAmtID, double stdAmount, double altAmount, long replaceStdAmount, int in_out)
{
	PPID   in_id = 0, out_id = 0;
	if(altAmtID && altAmtID != stdAmtID) {
		R_Data.Amounts.Add(altAmtID, 0L/*@curID*/, altAmount, 1);
		if(in_out && ATObj.FetchCompl(altAmtID, &in_id, &out_id) > 0) {
			if(in_out < 0 && in_id) {
				R_Data.Amounts.Add(in_id, 0L/*@curID*/, -altAmount, 1); // Для входящих позиций сумма имеет инвертированный знак
			}
			if(in_out > 0 && out_id)
				R_Data.Amounts.Add(out_id, 0L/*@curID*/, altAmount, 1);
		}
	}
	if(!replaceStdAmount || altAmtID == stdAmtID) {
		R_Data.Amounts.Add(stdAmtID, 0L/*@curID*/, stdAmount, 1);
		if(in_out && ATObj.FetchCompl(stdAmtID, &in_id, &out_id) > 0) {
			if(in_out < 0 && in_id) {
				R_Data.Amounts.Add(in_id, 0L/*@curID*/, -stdAmount, 1); // Для входящих позиций сумма имеет инвертированный знак
			}
			if(in_out > 0 && out_id)
				R_Data.Amounts.Add(out_id, 0L/*@curID*/, stdAmount, 1);
		}
	}
}

void BillTotalBlock::SetupStdAmount(PPID stdAmtID, double stdAmount, int in_out)
{
	PPID   in_id = 0, out_id = 0;
	R_Data.Amounts.Add(stdAmtID, 0L/*@curID*/, stdAmount, 1);
	if(in_out && ATObj.FetchCompl(stdAmtID, &in_id, &out_id) > 0) {
		if(in_out < 0 && in_id) {
			R_Data.Amounts.Add(in_id, 0L/*@curID*/, -stdAmount, 1); // Для входящих позиций сумма имеет инвертированный знак
		}
		if(in_out > 0 && out_id)
			R_Data.Amounts.Add(out_id, 0L/*@curID*/, stdAmount, 1);
	}
}

void FASTCALL BillTotalBlock::Add(PPTransferItem & rTi)
{
	if(State & stAllGoodsUnlimUndef) {
		State |= stAllGoodsUnlim;
		State &= ~stAllGoodsUnlimUndef;
		R_Data.Flags |= BillTotalData::fAllGoodsUnlim;
	}
	if(!(rTi.Flags & PPTFR_UNLIM)) {
		State &= ~stAllGoodsUnlim;
		State &= ~stAllGoodsUnlimUndef;
		R_Data.Flags &= ~BillTotalData::fAllGoodsUnlim;
	}
	const  PPID goods_id = labs(rTi.GoodsID);
	Goods2Tbl::Rec goods_rec;
	//
	// Условие (goods_rec.ID == goods_id) в дальнейшем будет означать,
	// что goods_rec успешно извлечена из кэша вызовом GObj.Fetch.
	//
	goods_rec.ID = 0;
	//
	int    r = 1;
	if(GoodsTypeID || Flags & BTC_ONLYUNLIMGOODS) {
		//
		// Если расчет необходим только по отдельному товарному типу,
		// то проверяем товар на принадлежность этому типу.
		//
		if(GObj.Fetch(goods_id, &goods_rec) > 0) {
			if(!goods_rec.GoodsTypeID)
				r = -1;
			else if(GoodsTypeID && goods_rec.GoodsTypeID != GoodsTypeID)
				r = -1;
			else if(Flags & BTC_ONLYUNLIMGOODS) {
				PPObjGoodsType gt_obj;
				PPGoodsType gt_rec;
				if(gt_obj.Fetch(goods_rec.GoodsTypeID, &gt_rec) <= 0 || !(gt_rec.Flags & (GTF_UNLIMITED|GTF_QUASIUNLIM)))
					r = -1;
			}
		}
		else
			r = -1;
	}
	if(r > 0) {
		int    oddgoods = 0;
		int    exclamount = 0;
		int    is_asset = 0;
		int    sign = 1;
		int    in_out = 0; // < 0 - input on modif; > 0 - output on modif; 0 - no modif
		PPID   cvat_amt_id = 0;
		double qtty;
		double cq;
		double pq;
		double dq;
		double curamtq;
		double tax_rate;
		double tax_sum;
		double asset_expl = 0.0;
		GTaxVect gtv;
		SETFLAG(rTi.Flags, PPTFR_SELLING, (State & stSelling));
		if(rTi.Flags & PPTFR_RECEIPT && rTi.ExtCost != 0.0) {
			State |= stExtCost;
		}
		R_Data.LinesCount++;
		//
		// Count goods IDs
		//
		if(GoodsList.addUnique(goods_id) > 0)
			R_Data.GoodsCount++;
		//
		// Count quantity and phisical units quantity
		//
		R_Data.UnitsCount += rTi.Qtty();
		if(goods_rec.ID == goods_id || GObj.Fetch(goods_id, &goods_rec) > 0) {
			is_asset = BIN(goods_rec.Flags & GF_ASSETS);
			GoodsStockExt gse;
			R_Data.PhUnitsCount += rTi.Qtty() * goods_rec.PhUPerU;
			if(GObj.GetStockExt(goods_id, &gse, 1) > 0) {
				R_Data.Brutto += gse.CalcBrutto(rTi.Qtty());
				R_Data.Volume += gse.CalcVolume(rTi.Qtty());
			}
		}
		else
			goods_rec.ID = 0;
		//
		// Count number of packs
		//
		if(rTi.UnitPerPack > 0.0)
			R_Data.PackCount += fabs(rTi.Qtty()) / rTi.UnitPerPack;
		//
		// Рассчитываем суммы вводимых в эксплуатацию основных средств
		//
		if(rTi.Flags & PPTFR_ASSETEXPL) {
			gtv.CalcBPTI(R_BPack, rTi, TIAMT_ASSETEXPL);
			asset_expl = gtv.GetValue(GTAXVF_AFTERTAXES | GTAXVF_EXCISE);
			if(rTi.Flags & PPTFR_MODIF)
				asset_expl = -asset_expl; // Вывод из эксплуатации
		}
		//
		// Count generic amounts: cost*qtty, price*qtty, nominal amount*qtty
		//
		if(rTi.Flags & PPTFR_REVAL) {
			if(rTi.Flags & PPTFR_CORRECTION) {
				qtty = fabs(rTi.Quantity_);
				const double q = fabs(rTi.Quantity_);
				const double q_pre = fabs(rTi.QuotPrice);
				cq = R2(rTi.Cost * q - rTi.RevalCost * q_pre);
				pq = R2(rTi.Price * q - rTi.Discount * q_pre);
				dq = 0.0;
			}
			else {
				qtty = fabs(rTi.Rest_);
				cq = R2((rTi.Cost  - rTi.RevalCost) * qtty);
				pq = R2((rTi.Price - rTi.Discount) * qtty);
				dq = 0.0;
			}
		}
		else {
			if(rTi.Flags & PPTFR_CORRECTION) {
				qtty = fabs(rTi.Quantity_);
				const double q = fabs(rTi.Quantity_);
				const double q_pre = fabs(rTi.QuotPrice);
				cq = R2(rTi.Cost * (q - q_pre));
				pq = R2(rTi.NetPrice() * q - rTi.RevalCost * q_pre);
				dq = 0.0;
			}
			else {
				if(GetOpType(R_BPack.Rec.OpID) == PPOPT_GOODSMODIF) {
					qtty = rTi.SQtty(R_BPack.Rec.OpID);
					if(rTi.Flags & PPTFR_MINUS) {
						sign = -1;
						in_out = -1;
					}
					else if(rTi.Flags & PPTFR_PLUS)
						in_out = 1;
				}
				else
					qtty = fabs(rTi.Quantity_);
				cq = R2(rTi.Cost  * qtty);
				pq = R2(rTi.Price * qtty);
				if(rTi.Date > CConfig._390_DisCalcMethodLockDate)
					dq = pq - R2((rTi.Price-rTi.Discount) * qtty);
				else
					dq = R2(rTi.Discount * qtty);
			}
		}
		//
		// If cost defined without VAT, then adjust (cost*qtty) value to VAT
		//
		if(rTi.Flags & PPTFR_COSTWOVAT) {
			gtv.CalcBPTI(R_BPack, rTi, TIAMT_COST);
			cq += gtv.GetValue(GTAXVF_VAT) * sign;
			//
			// Для основных фондов, если балансовая стоимость задана без НДС, то
			// и остаточная стоимость учитывается без НДС
			//
			if(is_asset) {
				gtv.CalcBPTI(R_BPack, rTi, TIAMT_PRICE);
				pq += gtv.GetValue(GTAXVF_VAT) * sign;
			}
		}
		if(rTi.Flags & PPTFR_PRICEWOTAXES) {
			gtv.CalcBPTI(R_BPack, rTi, TIAMT_PRICE);
			pq = gtv.GetValue(GTAXVF_BEFORETAXES);
			dq = 0;
		}
		curamtq = R2(rTi.CurPrice * qtty);
		//
		// Calculation values for printing and another output
		//
		if(Flags & BTC_CALCOUTAMOUNTS) {
			int    tiamt = 0;
			if(OutAmtType == TIAMT_COST) {
				tiamt = TIAMT_COST;
				R_Data.Amt += cq;
			}
			else if(OutAmtType == TIAMT_PRICE) {
				tiamt = TIAMT_PRICE;
				R_Data.Amt += (pq - dq);
			}
			else {
				// @v12.2.4 tiamt = TIAMT_AMOUNT;
				// @v12.2.4 R_Data.Amt += (rTi.Flags & (PPTFR_SELLING|PPTFR_REVAL)) ? (pq - dq) : cq;
				// @v12.2.4 {
				tiamt = GTaxVect::GetTaxNominalAmountType(R_BPack.Rec);
				if(tiamt == TIAMT_PRICE) {
					R_Data.Amt += (rTi.Flags & (PPTFR_SELLING|PPTFR_REVAL)) ? (pq - dq) : cq;
				}
				else { // @fixme Возможно, здесь нужно уточнение
					R_Data.Amt += cq;
				}
				// } @v12.2.4 
			}
			gtv.CalcBPTI(R_BPack, rTi, tiamt, (Flags & BTC_EXCLUDEVAT) ? GTAXVF_VAT : 0);
			tax_sum = gtv.GetValue(GTAXVF_VAT);
			if(tax_sum != 0.0) {
				tax_rate = gtv.GetTaxRate(GTAX_VAT, 0);
				R_Data.VAT  += gtv.GetValue(GTAXVF_VAT);
				R_Data.VatList.Add(tax_rate, tax_sum, gtv.GetValue(GTAXVF_AFTERTAXES), gtv.GetValue(GTAXVF_BEFORETAXES));
			}
			R_Data.STax += gtv.GetValue(GTAXVF_SALESTAX);
		}
		else {
			PPID   gt_id = 0;
			if(DynGoodsTypeForSupplAgent && rTi.LotID) {
				if(BillObj->GetSupplAgent(rTi.LotID) > 0) // @slow
					gt_id = DynGoodsTypeForSupplAgent;
			}
			//
			// Full calculation amounts and taxes
			//
 			if(gt_id || rTi.Flags & PPTFR_ODDGOODS) {
				PPGoodsType gt_rec;
				if(SETIFZ(gt_id, goods_rec.GoodsTypeID) && gt_id != PPGT_DEFAULT) {
					oddgoods = 1;
					if(GtObj.Fetch(gt_id, &gt_rec) > 0) {
						if(gt_rec.Flags & GTF_EXCLAMOUNT)
							exclamount = 1;
						SetupStdAmount(PPAMT_BUYING,  gt_rec.AmtCost, cq, cq, (gt_rec.Flags & GTF_RPLC_COST), in_out);
						SetupStdAmount(PPAMT_SELLING, gt_rec.AmtPrice, pq,
							(gt_rec.Flags & GTF_PRICEINCLDIS) ? (pq - dq) : pq, (gt_rec.Flags & GTF_RPLC_PRICE), in_out);
						SetupStdAmount(PPAMT_DISCOUNT, gt_rec.AmtDscnt, dq, dq, (gt_rec.Flags & GTF_RPLC_DSCNT), 0);
						cvat_amt_id = gt_rec.AmtCVat;
					}
				}
			}
			if(!oddgoods) {
				SetupStdAmount(PPAMT_BUYING,   cq, in_out);
				SetupStdAmount(PPAMT_SELLING,  pq, in_out);
				SetupStdAmount(PPAMT_DISCOUNT, dq, 0);
			}
			R_Data.Amounts.Add(PPAMT_ASSETEXPL, 0L  /*@curID*/, asset_expl, 1);
			if(!exclamount) {
				R_Data.Amt = R2(R_Data.Amt + ((State & stSelling) ? (pq - dq) : cq));
				if(rTi.CurID)
					R_Data.CurAmt = R2(R_Data.CurAmt + curamtq);
			}
			if(Flags & BTC_CALCSALESTAXES) {
				PPID   tax_amt_id = 0;
				double vat = 0.0;
				double cvat = 0.0;
				double pvat = 0.0;
				double vat_base = 0.0;
				double amt_by_vat = 0.0;
				double excise = 0.0;
				double cexcise = 0.0;
				double stax = 0.0;
				double cstax = 0.0;
				{
					//
					// Расчет налоговых сумм в номинальных ценах //
					//
					gtv.CalcBPTI(R_BPack, rTi, /*TIAMT_AMOUNT*/GTaxVect::GetTaxNominalAmountType(R_BPack.Rec), (Flags & BTC_CALCNOMINALGVAT) ? GTAXVF_NOMINAL : 0);
					vat      = gtv.GetValue(GTAXVF_VAT) * sign;
					vat_base = gtv.GetValue(GTAXVF_AFTERTAXES) * sign;
					amt_by_vat = gtv.GetValue(GTAXVF_BEFORETAXES) * sign;
					tax_rate = gtv.GetTaxRate(GTAX_VAT, 0);
					excise   = gtv.GetValue(GTAXVF_EXCISE) * sign;
					stax     = gtv.GetValue(GTAXVF_SALESTAX) * sign;
					R_Data.VatList.Add(tax_rate, vat, vat_base, amt_by_vat);
				}
				{
					//
					// Расчет налоговых сумм в ценах поступления //
					//
					gtv.CalcBPTI(R_BPack, rTi, TIAMT_COST);
					cvat    = gtv.GetValue(GTAXVF_VAT) * sign;
					cexcise = gtv.GetValue(GTAXVF_EXCISE) * sign;
					cstax   = gtv.GetValue(GTAXVF_SALESTAX) * sign;
					if(cvat_amt_id)
						R_Data.Amounts.Add(cvat_amt_id, 0L/*@curID*/, cvat, 1);
					R_Data.CostVatList.Add(gtv.GetTaxRate(GTAX_VAT, 0), cvat, gtv.GetValue(GTAXVF_AFTERTAXES) * sign, gtv.GetValue(GTAXVF_BEFORETAXES) * sign);
				}
				{
					//
					// Расчет налоговых сумм в ценах реализации //
					//
					gtv.CalcBPTI(R_BPack, rTi, TIAMT_PRICE);
					pvat = gtv.GetValue(GTAXVF_VAT) * sign;
					R_Data.PriceVatList.Add(gtv.GetTaxRate(GTAX_VAT, 0), pvat, gtv.GetValue(GTAXVF_AFTERTAXES) * sign, gtv.GetValue(GTAXVF_BEFORETAXES) * sign);
				}
				R_Data.VAT     += vat;
				R_Data.CVAT    += cvat;
				R_Data.PVAT    += pvat;
				R_Data.Excise  += excise;
				R_Data.CExcise += cexcise;
				R_Data.STax    += stax;
				R_Data.CSTax   += cstax;
				if(vat != 0.0) {
					if(ATObj.FetchByTax(&tax_amt_id, GTAX_VAT, tax_rate) > 0)
						R_Data.Amounts.Add(tax_amt_id, 0L/*@curID*/, vat, 1);
				}
				if(stax != 0.0 && ATObj.FetchByTax(&tax_amt_id, GTAX_SALES, gtv.GetTaxRate(GTAX_SALES, 0)) > 0)
					R_Data.Amounts.Add(tax_amt_id, 0L/*@curID*/, stax, 1);
				SetupStdAmount(PPAMT_VATAX,    vat,     in_out);
				SetupStdAmount(PPAMT_CVAT,     cvat,    in_out);
				SetupStdAmount(PPAMT_PVAT,     pvat,    in_out);
				SetupStdAmount(PPAMT_EXCISE,   excise,  in_out);
				SetupStdAmount(PPAMT_CEXCISE,  cexcise, in_out);
				SetupStdAmount(PPAMT_SALESTAX, stax,    in_out);
				SetupStdAmount(PPAMT_CSTAX,    cstax,   in_out);
			}
		}
	}
}

void BillTotalBlock::Finish(/*const PPBillPacket * pPack*/)
{
	if(!(State & stExtCost) && R_BPack.UsesDistribCost()) {
		PPObjAmountType at_obj;
		if(at_obj.IsThereDistribCostAmounts()) {
			PPAmountType at_rec;
			for(uint i = 0; i < R_BPack.Amounts.getCount(); i++) {
				const AmtEntry & r_ae = R_BPack.Amounts.at(i);
				if(at_obj.Fetch(r_ae.AmtTypeID, &at_rec) > 0 && at_rec.Flags & PPAmountType::fDistribCost) {
					State |= stExtCost;
					break;
				}
			}
		}
	}
	SETFLAG(R_Data.Flags, BillTotalData::fExtCost, (State & stExtCost));
	R_Data.Flags |= BillTotalData::fInitialized;
}

int PPBillPacket::CalcTotal(BillTotalData & rTotal, PPID goodsTypeID, long btcFlags)
{
	BillTotalBlock btb(rTotal, *this, goodsTypeID, OutAmtType, btcFlags);
	if(GetOpType(Rec.OpID) == PPOPT_ACCTURN) {
		for(uint i = 0; i < AdvList.GetCount(); i++)
			btb.Add(AdvList.Get(i));
	}
	{
		for(uint tii = 0; tii < GetTCount(); tii++) {
			PPTransferItem & r_ti = TI(tii);
			if(!(r_ti.Flags & PPTFR_PCKG))
				btb.Add(r_ti);
			else
				btb.AddPckg(&r_ti);
		}
	}
	btb.Finish();
	SETFLAG(ProcessFlags, pfAllGoodsUnlim, (rTotal.Flags & BillTotalData::fAllGoodsUnlim));
	SETFLAG(ProcessFlags, pfHasExtCost, (rTotal.Flags & BillTotalData::fExtCost));
	return 1;
}

int PPBillPacket::CalcTotal(BillTotalData & rTotal, long btcFlags)
{
	return CalcTotal(rTotal, 0, btcFlags);
}

int PPBillPacket::SumAmounts(AmtList * pList, int fromDB)
{
	uint   i;
	long   btb_flags = 0;
	BillTotalData total_data;
	// @v11.6.6 {
	PPOprKind op_rec;
	if(Rec.OpID && GetOpData(Rec.OpID, &op_rec)) {
		if(op_rec.Flags & OPKF_CALCSTAXES)
			btb_flags |= BTC_CALCSALESTAXES;
	}
	// } @v11.6.6 
	/* @v11.6.6 if(CheckOpFlags(Rec.OpID, OPKF_CALCSTAXES))
		btb_flags |= BTC_CALCSALESTAXES;*/
	{
		BillTotalBlock btb(total_data, *this, 0, 0, btb_flags);
		if(GetOpType(Rec.OpID) == PPOPT_ACCTURN) {
			for(i = 0; i < AdvList.GetCount(); i++)
				btb.Add(AdvList.Get(i));
		}
		if(fromDB) {
			PPTransferItem ti;
			int    r;
			int    r_by_bill = 0;
			while((r = P_BObj->trfr->EnumItems(Rec.ID, &r_by_bill, &ti)) > 0) {
				if(!(ti.Flags & PPTFR_PCKG))
					btb.Add(ti);
			}
			if(!r)
				return 0;
		}
		else {
			for(uint tii = 0; tii < GetTCount(); tii++) {
				PPTransferItem & r_ti = TI(tii);
				if(!(r_ti.Flags & PPTFR_PCKG))
					btb.Add(r_ti);
			}
		}
		btb.Finish();
		SETFLAG(ProcessFlags, pfHasExtCost, (total_data.Flags & BillTotalData::fExtCost));
		pList->Put(&total_data.Amounts, 1, 1);
		if(btb_flags & BTC_CALCSALESTAXES) {
			if(total_data.CVAT != 0.0 && OpTypeID == PPOPT_GOODSRECEIPT && Rec.Object) {
				if(IsSupplVATFree(Rec.Object) > 0 || PPObjLocation::CheckWarehouseFlags(Rec.LocID, LOCF_VATFREE)) {
					total_data.CVAT = 0.0;
					pList->Put(PPAMT_CVAT, 0L/*@curID*/, 0, 1, 1);
				}
			}
		}
		// @v11.6.6 {
		if(op_rec.OpTypeID == PPOPT_ACCTURN && op_rec.ExtFlags & OPKFX_ACCAUTOVAT) {
			const double nominal_amount = pList->Get(PPAMT_MAIN, 0L/*@curID*/);
			if(nominal_amount != 0.0) {
				PPID  virtual_goods_id = CConfig.PrepayInvoiceGoodsID;
				if(virtual_goods_id) {
					PPObjGoods gobj;
					PPGoodsTaxEntry te;
					if(gobj.FetchTaxEntry2(virtual_goods_id, 0/*lotID*/, 0/*taxPayerID*/, Rec.Dt, Rec.OpID, &te) > 0) {
						const double def_vat_rate = te.GetVatRate();
						PPObjAmountType amtt_obj;
						TaxAmountIDs tais;
						amtt_obj.GetTaxAmountIDs(&tais, 0);
						uint   vat_rate_idx = 0;
						for(uint i = 0; !vat_rate_idx && i < SIZEOFARRAY(tais.VatRate); i++) {
							if(feqeps(def_vat_rate, fdiv100i(tais.VatRate[i]), 1E-6))
								vat_rate_idx = i+1;
						}
						if(vat_rate_idx > 0) {
							if(def_vat_rate > 0.0 && def_vat_rate <= 40.0) {
								double vat_amount = nominal_amount * SalesTaxMult(def_vat_rate);
								pList->Put(tais.VatAmtID[vat_rate_idx-1], 0L/*@curID*/, vat_amount, 1, 1);
							}
						}
					}
				}
			}
		}
		// } @v11.6.6 
	}
	if(P_Freight)
		pList->Add(PPAMT_FREIGHT, 0, P_Freight->Cost);
	pList->Put(PPAMT_MAIN, Rec.CurID, (Rec.CurID ? total_data.CurAmt : total_data.Amt), 0, 0);
	return 1;
}

int PPBillPacket::GetDebtDim(PPID * pDebtDimID) const
{
	int    ok = -1;
	PPID   dd_id = 0;
	if(Ext.AgentID) {
		LAssocArray debt_dim_agent_list;
		PPIDArray dim_list;
		PPObjDebtDim dd_obj;
		dd_obj.FetchAgentList(&debt_dim_agent_list);
		debt_dim_agent_list.GetListByVal(Ext.AgentID, dim_list);
		if(dim_list.getCount()) {
			dd_id = dim_list.get(0);
			ok = (dim_list.getCount() > 1) ? 2 : 1;
		}
	}
	ASSIGN_PTR(pDebtDimID, dd_id);
	return ok;
}
//
// Package methods {
//
int PPBillPacket::CalcPckgTotals()
{
	if(P_PckgList) {
		PPObjGoods goods_obj;
		LPackage * p_pckg;
		for(uint i = 0; P_PckgList->EnumItems(&i, &p_pckg);) {
			int    idx;
			PPID   id;
			p_pckg->Cost = p_pckg->Price = 0;
			p_pckg->Qtty = p_pckg->PhQtty = 0;
			for(uint j = 0; p_pckg->EnumItems(&j, &idx, &id);) {
				if(ChkTIdx(idx)) {
					const PPTransferItem * p_ti = &ConstTI(idx);
					double phuperu;
					const double qtty = p_ti->Quantity_;
					p_pckg->Cost  += p_ti->Cost  * fabs(qtty);
					p_pckg->Price += p_ti->Price * fabs(qtty);
					p_pckg->Qtty  += qtty;
					if(goods_obj.GetPhUPerU(p_ti->GoodsID, 0, &phuperu) > 0)
						p_pckg->PhQtty += qtty * phuperu;
				}
			}
			if(p_pckg->PckgIdx >= 0) {
				PPTransferItem * p_ti = &TI(p_pckg->PckgIdx);
				if(p_ti->IsReceipt() && p_ti->Flags & PPTFR_PCKG) {
					p_ti->Cost  = p_pckg->Cost;
					p_ti->Price = p_pckg->Price;
				}
			}
		}
	}
	return 1;
}

int PPBillPacket::InitPckg()
{
	int    ok = 1;
	int    is_mounting_op = BIN(CheckOpFlags(Rec.OpID, OPKF_PCKGMOUNTING));
	LPackage * p_pckg;
	for(uint i = 0; P_PckgList && P_PckgList->EnumItems(&i, &p_pckg);) {
		int  idx;
		PPID id;
		PPTransferItem * p_ti;
		THROW(ChkTIdx(p_pckg->PckgIdx));
		p_ti = &TI(p_pckg->PckgIdx);
		p_pckg->ID = p_ti->LotID;
		p_ti->GoodsID = p_pckg->PckgTypeID;
		for(uint j = 0; p_pckg->EnumItems(&j, &idx, &id);) {
			THROW(ChkTIdx(idx));
			p_ti = &TI(idx);
			if(p_ti->Flags & PPTFR_PCKGGEN) {
				PPID lot_id = 0;
				if(is_mounting_op) {
					THROW(P_BObj->trfr->SearchByBill(Rec.ID, 1, p_ti->RByBill, 0) > 0);
					lot_id = P_BObj->trfr->data.LotID;
				}
				else
					lot_id = p_ti->LotID;
				if(lot_id)
					p_pckg->UpdateItem(j-1, idx, lot_id);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPBillPacket::AddPckg(LPackage * pPckg)
{
	int    ok = 1;
	THROW_MEM(SETIFZ(P_PckgList, new LPackageList));
	if(pPckg->PckgIdx < 0) {
		PPTransferItem ti;
		ti.GoodsID = pPckg->PckgTypeID;
		ti.Quantity_ = 1.0;
		ti.Cost = pPckg->Cost;
		ti.Price = pPckg->Price;
		ti.Flags |= PPTFR_PCKG;
		if(pPckg->Flags & PCKGF_MOUNTED)
			ti.Flags |= PPTFR_MODIF;
		ti.Init(&Rec, 0);
		SETFLAG(ti.Flags, PPTFR_RECEIPT, pPckg->ID == 0);
		SETFLAG(ti.Flags, PPTFR_PLUS,    pPckg->ID == 0);
		SETFLAG(ti.Flags, PPTFR_MINUS,   pPckg->ID);
		THROW_SL(Lots.insert(&ti));
		pPckg->PckgIdx = Lots.getCount()-1;
	}
	THROW(P_PckgList->Add(pPckg));
	CATCHZOK
	return ok;
}
//
// } Package methods
//
