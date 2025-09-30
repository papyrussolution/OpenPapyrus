// V_DEBT.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// Implementation of PPViewDebtTrnovr
//
#include <pp.h>
#pragma hdrstop
//
//
//
double PayableBillListItem::MultExtCoef(double val, int zeroIfDivZero /*= 0*/) const
{
	return (Amount != 0.0) ? (val * (ExtAmt / Amount)) : (zeroIfDivZero ? 0.0 : val);
}

PayableBillList::PayableBillList(AmtList * pAmt, AmtList * pPaym) : P_Amt(pAmt), P_Paym(pPaym)
{
}

int PayableBillList::AddPayableItem(const PayableBillListItem * pItem, long tabID, double paym, int useExtCoef)
{
	int    ok = 1;
	CALLPTRMEMB(P_Amt, Add(tabID, pItem->CurID, useExtCoef ? pItem->MultExtCoef(pItem->Amount, 0) : pItem->Amount));
	CALLPTRMEMB(P_Paym, Add(tabID, pItem->CurID, useExtCoef ? pItem->MultExtCoef(paym, 1) : paym));
	THROW_SL(insert(pItem));
	CATCHZOK
	return ok;
}

int FASTCALL PayableBillList::AddBill(const BillTbl::Rec * pBillRec)
{
	if(pBillRec) {
		PayableBillListItem item;
		MEMSZERO(item);
		item.ID     = pBillRec->ID;
		item.Dt     = pBillRec->Dt;
		item.CurID  = pBillRec->CurID;
		item.Amount = BR2(pBillRec->Amount);
		item.PaymAmt = BR2(pBillRec->PaymAmount);
		return insert(&item) ? 1 : PPSetErrorSLib();
	}
	else
		return -1;
}

void FASTCALL PayableBillList::GetIdList(LongArray & rList) const
{
	for(uint i = 0; i < getCount(); i++)
		rList.add(at(i).ID);
}
//
// DebtTrnovrTotal
//
DebtTrnovrTotal::DebtTrnovrTotal()
{
	Init();
}

void DebtTrnovrTotal::Init()
{
	Count = 0;
	Debit.clear();
	Credit.clear();
	Debt.clear();
	RPaym.clear();
	Reckon.clear();
	RDebt.clear();
	TDebt.clear();
	ExpiryDebt.clear();
}

int DebtTrnovrTotal::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, Count, rBuf));
	THROW_SL(pCtx->Serialize(dir, &Debit, rBuf));
	THROW_SL(pCtx->Serialize(dir, &Credit, rBuf));
	THROW_SL(pCtx->Serialize(dir, &Debt, rBuf));
	THROW_SL(pCtx->Serialize(dir, &RPaym, rBuf));
	THROW_SL(pCtx->Serialize(dir, &Reckon, rBuf));
	THROW_SL(pCtx->Serialize(dir, &RDebt, rBuf));
	THROW_SL(pCtx->Serialize(dir, &TDebt, rBuf));
	THROW_SL(pCtx->Serialize(dir, &ExpiryDebt, rBuf));
	CATCHZOK
	return ok;
}
//
// DebtTrnovrFilt
//
IMPLEMENT_PPFILT_FACTORY(DebtTrnovr); DebtTrnovrFilt::DebtTrnovrFilt() : PPBaseFilt(PPFILT_DEBTTRNOVR, 0, 2/*ver*/)
{
#define _S_ DebtTrnovrFilt
	SetFlatChunk(offsetof(_S_, ReserveStart), offsetof(_S_, LocIDList)-offsetof(_S_, ReserveStart));
	SetBranchSVector(offsetof(_S_, LocIDList));
	SetBranchSVector(offsetof(_S_, CliIDList));
	SetBranchObjIdListFilt(offsetof(_S_, BillList));
	SetBranchObjIdListFilt(offsetof(_S_, RcknBillList));
	SetBranchObjIdListFilt(offsetof(_S_, DebtDimList));
#undef _S_
	Init(1, 0);
}

/*virtual*/int DebtTrnovrFilt::ReadPreviousVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == 1) {
		class DebtTrnovrFilt_v1 : public PPBaseFilt {
		public:
			DebtTrnovrFilt_v1() : PPBaseFilt(PPFILT_DEBTTRNOVR, 0, 1)
			{
			#define _S_ DebtTrnovrFilt
				SetFlatChunk(offsetof(_S_, ReserveStart), offsetof(_S_, LocIDList)-offsetof(_S_, ReserveStart));
				SetBranchSVector(offsetof(_S_, LocIDList));
				SetBranchSVector(offsetof(_S_, CliIDList));
				SetBranchObjIdListFilt(offsetof(_S_, BillList));
				SetBranchObjIdListFilt(offsetof(_S_, RcknBillList));
			#undef _S_
				Init(1, 0);
			}
			char   ReserveStart[24];
			SubstGrpBill Sgb;
			DateRange Period;
			DateRange PaymPeriod;
			DateRange ExpiryPeriod;
			PPID   AccSheetID;
			PPID   CurID;
			PPID   AgentID;
			PPID   PayerID;
			long   InitOrder;
			long   Flags;
			PPID   OpID;
			int32  Reserve;
			PPID   CityID;
			PPID   CategoryID;
			long   CycleKind;
			PPCycleFilt Cf;
			long   ExtKind;
			long   ExtExpiryTerm;
			double ExtExpiryMinPart;
			PPID   Article2ID;
			PPID   AccSheet2ID;
			PPIDArray LocIDList;
			PPIDArray CliIDList;
			ObjIdListFilt BillList;
			ObjIdListFilt RcknBillList;
		};
		DebtTrnovrFilt_v1 fv1;
		THROW(fv1.Read(rBuf, 0));
#define CPYFLD(f) f = fv1.f
		CPYFLD(Sgb);
		CPYFLD(Period);
		CPYFLD(PaymPeriod);
		CPYFLD(ExpiryPeriod);
		CPYFLD(AccSheetID);
		CPYFLD(CurID);
		CPYFLD(AgentID);
		CPYFLD(PayerID);
		CPYFLD(InitOrder);
		CPYFLD(Flags);
		CPYFLD(OpID);
		CPYFLD(Reserve);
		CPYFLD(CityID);
		CPYFLD(CategoryID);
		CPYFLD(CycleKind);
		CPYFLD(Cf);
		CPYFLD(ExtKind);
		CPYFLD(ExtExpiryTerm);
		CPYFLD(ExtExpiryMinPart);
		CPYFLD(Article2ID);
		CPYFLD(AccSheet2ID);
		CPYFLD(LocIDList);
		CPYFLD(CliIDList);
		CPYFLD(BillList);
		CPYFLD(RcknBillList);
#undef CPYFLD
		DebtDimList.Set(0);
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
// PPViewDebtTrnovr
//
PPViewDebtTrnovr::DebtEntry::DebtEntry(PPID ident) : ID(ident), _AvgPaym(0.0)
{
}

PPViewDebtTrnovr::PPViewDebtTrnovr() : PPView(0, &Filt, PPVIEW_DEBTTRNOVR, implUseServer, 0), UseOmtPaymAmt(BIN(CConfig.Flags2 & CCFLG2_USEOMTPAYMAMT)),
	P_BObj(BillObj), P_DebtDimAgentList(0), P_TempTbl(0), P_IterBillList(0), IterBillCounter(0), ExpiryDate(ZERODATE), IsDlvrAddrListInited(0)
{
}

PPViewDebtTrnovr::~PPViewDebtTrnovr()
{
	delete P_DebtDimAgentList;
	delete P_TempTbl;
	delete P_IterBillList;
	if(!(BaseState & bsServerInst))
		DBRemoveTempFiles();
}

PPBaseFilt * PPViewDebtTrnovr::CreateFilt(const void * extraPtr) const
{
	DebtTrnovrFilt * p_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_DEBTTRNOVR, reinterpret_cast<PPBaseFilt **>(&p_filt))) {
		p_filt->Sgb.Reset();
		p_filt->Sgb.S = SubstGrpBill::sgbObject;
		if(reinterpret_cast<long>(extraPtr) == 0)
			p_filt->AccSheetID = GetSellAccSheet();
		else if(reinterpret_cast<long>(extraPtr) == 1)
			p_filt->AccSheetID = GetSupplAccSheet();
		p_filt->Flags |= DebtTrnovrFilt::fSkipPassive;
	}
	else
		PPErrCode = PPERR_BASEFILTUNSUPPORTED;
	return static_cast<PPBaseFilt *>(p_filt);
}

int FASTCALL PPViewDebtTrnovr::CheckAddress(PPID locID)
{
	int    ok = 0;
	if(Filt.CityID) {
		PPID   city_id = 0;
		if(locID && PsnObj.GetCityByAddr(locID, &city_id, 0, 1) > 0)
			if(WObj.IsChildOf(city_id, Filt.CityID))
				ok = 1;
	}
	else
		ok = 1;
	return ok;
}

int PPViewDebtTrnovr::GetPayableBillList(PPID arID, PPID curID, PayableBillList * pList)
{
	return GetPayableBillList_(&PayableOpList, arID, curID, pList);
}

int PPViewDebtTrnovr::GetDlvrAddrList()
{
	IsDlvrAddrListInited = 0;
	int    ok = P_BObj->P_Tbl->GetDlvrAddrList(&DlvrAddrList);
	if(!ok) {
		DlvrAddrList.freeAll();
	}
	else {
		DlvrAddrList.Sort();
		IsDlvrAddrListInited = 1;
	}
	return ok;
}

int PPViewDebtTrnovr::GetDlvrAddr(PPID billID, PPID * pAddrID)
{
	PPID   addr_id = 0;
	if(!IsDlvrAddrListInited) {
		GetDlvrAddrList();
	}
	if(IsDlvrAddrListInited) {
		if(!DlvrAddrList.BSearch(billID, &addr_id, 0))
			addr_id = 0;
	}
	else {
		PPFreight freight;
		if(P_BObj->P_Tbl->GetFreight(billID, &freight) > 0)
			addr_id = freight.DlvrAddrID__;
	}
	ASSIGN_PTR(pAddrID, addr_id);
	return addr_id ? 1 : -1;
}

int PPViewDebtTrnovr::CheckBillRec(const BillTbl::Rec * pRec, const PPIDArray * pOpList)
{
	int    ok = 1;
	PROFILE_START
	if((!(Filt.Flags & DebtTrnovrFilt::fLabelOnly) || pRec->Flags & BILLF_WHITELABEL) && (!pOpList || pOpList->lsearch(pRec->OpID))) {
		if(Filt.CityID) {
			PPID   addr_id = 0, dlvr_addr_id = 0;
			if(Filt.Flags & DebtTrnovrFilt::fDeliveryAddr) {
				PROFILE_START
				GetDlvrAddr(pRec->ID, &dlvr_addr_id);
				PPID   psn_id = ObjectToPerson(pRec->Object, 0);
				if(PsnObj.GetAddrID(psn_id, dlvr_addr_id, PSNGETADDRO_DLVRADDR, &addr_id) > 0) {
					if(!CheckAddress(addr_id))
						ok = 0;
				}
				else
					ok = 0;
				PROFILE_END
			}
		}
		if(ok)
			if(Filt.AgentID || Filt.PayerID) {
				PPBillExt ext;
				if(P_BObj->FetchExt(pRec->ID, &ext) > 0) {
					if((Filt.AgentID && ext.AgentID != Filt.AgentID) || (Filt.PayerID && ext.PayerID != Filt.PayerID))
						ok = 0;
				}
				else
					ok = 0;
			}
			if(ok)
				if(Filt.Article2ID && Filt.Article2ID != pRec->Object2)
					ok = 0;
	}
	else
		ok = 0;
	PROFILE_END
	return ok;
}

int PPViewDebtTrnovr::GetPayableBillList_(const PPIDArray * pOpList, PPID arID, PPID curID, PayableBillList * pList)
{
	const  LDATE current_date = NZOR(Filt.PaymPeriod.upp, getcurdate_());
	int    ok = 1;
	int    skip = 0;
	if(Filt.CityID) {
		if(!(Filt.Flags & DebtTrnovrFilt::fDeliveryAddr)) {
			PPID   addr_id = 0;
			PPID   psn_id = ObjectToPerson(arID);
			if(psn_id && PsnObj.GetAddrID(psn_id, 0, PSNGETADDRO_REALADDR, &addr_id) > 0)
				skip = CheckAddress(addr_id) ? 0 : 1;
			else
				skip = 1;
		}
	}
	if(!skip) {
		const  int by_links = BIN(Filt.Flags & DebtTrnovrFilt::fNoForwardPaym || !Filt.PaymPeriod.IsZero());
		const  int by_cost  = BIN(Filt.Flags & DebtTrnovrFilt::fByCost);
		uint   i;
		double paym = 0.0, amt = 0.0;
		PPID   single_op = pOpList ? pOpList->getSingle() : 0L;
		PayableBillListItem * p_item;
		PayableBillList tmp_list;
		BillCore * t = P_BObj->P_Tbl;
		BExtQuery q(t, 3, 64);
		DBQ * dbq = 0;
		q.select(t->ID, t->Dt, t->OpID, t->Flags, t->CurID, t->Amount, t->Object2, t->PaymAmount, 0L);
		dbq = & (t->Object == arID && daterange(t->Dt, &Filt.Period));
		dbq = ppcheckfiltid(dbq, t->OpID, single_op);
		if(curID >= 0)
			dbq = & (*dbq && t->CurID == curID);
		dbq = ppcheckfiltidlist(dbq, t->LocID, &Filt.LocIDList);
		q.where(*dbq);
		BillTbl::Key3 k;
		k.Object = arID;
		k.Dt = Filt.Period.low;
		k.BillNo = 0;
		{
			//
			// Для ускорения проверки отсутствия идентификатора найденного документа в списке pList
			// переносим идентификаторы из pList во временный список ndup_list и сортируем его.
			//
			LongArray ndup_list;
			pList->GetIdList(ndup_list);
			ndup_list.sort();
			for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
				PPID   bill_id = t->data.ID;
				PPID   dlvr_addr_id = 0;
				THROW(PPCheckUserBreak());
				if(!ndup_list.bsearch(bill_id) && CheckBillRec(&t->data, pOpList))
					THROW(tmp_list.AddBill(&t->data));
			}
		}
		//
		int    paydt_needed = 0;
		if(Filt.ExpiryPeriod.IsZero()) {
			if(oneof2(Filt.CycleKind, DebtTrnovrFilt::ckExpiry, DebtTrnovrFilt::ckDelay) && Filt.CycleKind == DebtTrnovrFilt::ckExpiry)
				paydt_needed = 1;
			else if(Filt.ExtKind == DebtTrnovrFilt::ekExpiryPart && Filt.ExtExpiryTerm <= 0)
				paydt_needed = 1;
		}
		else
			paydt_needed = 1;
		for(i = 0; i < tmp_list.getCount(); i++) {
			DateRange * p_paym_period = (Filt.Flags & DebtTrnovrFilt::fNoForwardPaym && Filt.PaymPeriod.IsZero()) ? &Filt.Period : &Filt.PaymPeriod;
			LDATE  paydt = ZERODATE;
			RAssocArray paym_list;
			THROW(PPCheckUserBreak());
			p_item = &tmp_list.at(i);
			if(paydt_needed)
				t->GetLastPayDate(p_item->ID, &paydt);
			if(!paydt_needed || Filt.ExpiryPeriod.CheckDate(paydt)) {
				if(Filt.CycleKind == DebtTrnovrFilt::ckPayments)
					t->CalcPaymentSieve(p_item->ID, p_item->CurID, &CycleSieve, &paym_list, &paym);
				else if(UseOmtPaymAmt && !by_links)
					paym = p_item->PaymAmt;
				else
					t->CalcPayment(p_item->ID, by_links, p_paym_period, p_item->CurID, &paym);
				const double debt = (p_item->Amount - paym);
				if(!(Filt.Flags & DebtTrnovrFilt::fDebtOnly) || debt > 0.0) {
					long   tab_id = 0;
					if(by_cost)
						P_BObj->P_Tbl->GetAmount(p_item->ID, PPAMT_BUYING, p_item->CurID, &p_item->ExtAmt);
					if(oneof2(Filt.CycleKind, DebtTrnovrFilt::ckExpiry, DebtTrnovrFilt::ckDelay)) {
						if(debt > 0.0) {
							LDATE  pd = (Filt.CycleKind == DebtTrnovrFilt::ckExpiry && paydt) ? paydt : p_item->Dt;
							long   nd = diffdate(current_date, pd);
							if(nd >= 0) {
								const uint dsc = DaySieve.getCount();
								long  prev_s = -MAXLONG;
								for(uint j = 0; tab_id == 0 && j < dsc; j++) {
									const long s = DaySieve.at(j);
									if(nd <= s && nd >= prev_s)
										tab_id = j+1;
									prev_s = s+1;
								}
								if(tab_id == 0 && dsc && DaySieve.at(dsc-1) == MAXLONG)
									tab_id = dsc;
								THROW(pList->AddPayableItem(p_item, tab_id, paym, by_cost));
							}
						}
					}
					else if(Filt.CycleKind == DebtTrnovrFilt::ckShipments) {
						uint j = 0;
						if(CycleSieve.searchDate(p_item->Dt, &j))
							THROW(pList->AddPayableItem(p_item, j+1, paym, by_cost));
					}
					else if(Filt.CycleKind == DebtTrnovrFilt::ckPayments) {
						int    first_point = 1;
						double p = 0.0;
						for(uint j = 0; j < CycleSieve.getCount(); j++) {
							uint   cs_pos = 0;
							if(paym_list.Search(j, &p, &cs_pos)) {
								PayableBillListItem item(*p_item);
								if(!first_point)
									item.Amount = item.ExtAmt = 0.0;
								tab_id = j+1;
								THROW(pList->AddPayableItem(&item, tab_id, p, by_cost));
								first_point = 0;
							}
						}
					}
					else if(Filt.ExtKind == DebtTrnovrFilt::ekExpiryPart) {
						if(debt > 0.0) {
							const LDATE pd = (Filt.ExtExpiryTerm > 0) ? plusdate(p_item->Dt, Filt.ExtExpiryTerm) : NZOR(paydt, p_item->Dt);
							double _exp_debt = (pd < current_date) ? debt : 0.0;
							//
							// Для этого (специального) вида отчета в качестве оплаты учитываем
							// общую сумму долга за вычетом просроченного долга.
							// А сумму документа подменяем на полный долг.
							//
							PayableBillListItem item(*p_item);
							item.Amount = debt;
							THROW(pList->AddPayableItem(&item, tab_id, debt - _exp_debt, by_cost));
						}
					}
					/*
					else if(Filt.ExtKind == DebtTrnovrFilt::ekTurnover) {
					}
					*/
					else
						THROW(pList->AddPayableItem(p_item, tab_id, paym, by_cost));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPViewDebtTrnovr::GetReceivableBillList(PPID arID, PayableBillList * pList)
{
	int    ok = 1, m;
	PPObjOprKind op_obj;
	ReckonOpArList op_list;
	PPIDArray tmp_op_list;
	for(uint j = 0; j < PayableOpList.getCount(); j++) {
		op_list.clear();
		THROW(P_BObj->GetPaymentOpListByDebtOp(PayableOpList.get(j), arID, &op_list));
		for(uint i = 0; i < op_list.getCount(); i++) {
			ReckonOpArItem item = op_list.at(i);
			THROW(PPCheckUserBreak());
			tmp_op_list.clear();
			tmp_op_list.add(item.PayableOpID);
			for(m = op_list.getCount()-1; m > (int)i; m--) {
				ReckonOpArItem * p_item = &op_list.at(m);
				if(p_item->PayableArID == item.PayableArID) {
					tmp_op_list.addUnique(p_item->PayableOpID);
					op_list.atFree(m);
				}
			}
			THROW(GetPayableBillList_(&tmp_op_list, item.PayableArID, Filt.CurID, pList));
		}
	}
	CATCHZOK
	return ok;
}

int PPViewDebtTrnovr::SetupRecVals(PPID curID, long tabID, const DebtEntry * pEntry, TempSellTrnovrTbl::Rec * pRec)
{
	int    ok = -1, skip = 0;
	double rdebt = 0.0, rpaym = 0.0, reckon = 0.0, tdebt = 0.0;
	const  double sell = R2(pEntry->DbtList.Get(tabID, curID));
	const  double paym = R2(pEntry->PaymList.Get(tabID, curID));
	const  double expiry_debt = R2(pEntry->ExpiryDebtList.Get(tabID, curID));
	double debt = R2(sell - paym);
	if(Filt.Flags & DebtTrnovrFilt::fCalcTotalDebt) {
		const double total_sell = R2(pEntry->TotalDbtList.Get(tabID, curID));
		const double total_paym = R2(pEntry->TotalPaymList.Get(tabID, curID));
		debt = R2(total_sell - total_paym);
	}
	pRec->CurID   = curID;
	pRec->TabID   = tabID;
	pRec->Sell    += sell;
	pRec->Payment += paym;
	pRec->Debt    += debt;
	pRec->ExpiryDebt += expiry_debt;
	if(Filt.Flags & DebtTrnovrFilt::fExtended) {
		rpaym  = R2(pEntry->RDbtList.Get(tabID, curID));
		reckon = R2(pEntry->RcknList.Get(tabID, curID));
		rdebt  = R2(rpaym - reckon);
		tdebt  = R2(debt - rdebt);
		pRec->RPaym   += rpaym;
		pRec->Reckon  += reckon;
		pRec->RDebt   += rdebt;
		pRec->TDebt   += tdebt;
	}
	if(Filt.ExtKind == DebtTrnovrFilt::ekExpiryPart) {
		const double part = 100.0 * debt / sell;
		if(Filt.ExtExpiryMinPart > 0 && part < Filt.ExtExpiryMinPart)
			skip = 1;
	}
	if(!skip && ((debt || Filt.Flags & DebtTrnovrFilt::fInclZeroDebt) || rdebt)) {
		Total.Debit.Add(0L, curID, sell, 0);
		Total.Credit.Add(0L, curID, paym, 0);
		Total.Debt.Add(0L, curID, debt, 0);
		if(Filt.Flags & DebtTrnovrFilt::fExtended) {
			Total.RPaym.Add(0L,  curID, rpaym,  0);
			Total.Reckon.Add(0L, curID, reckon, 0);
			Total.RDebt.Add(0L,  curID, rdebt,  0);
			Total.TDebt.Add(0L,  curID, tdebt,  0);
			Total.ExpiryDebt.Add(0L, curID, expiry_debt);
		}
		pRec->AvgPaym = pEntry->_AvgPaym;
		ok = 1;
	}
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempSellTrnovr);

class DebtTrnovrCrosstab : public Crosstab {
public:
	DebtTrnovrCrosstab(PPViewDebtTrnovr * pV) : Crosstab(), P_V(pV)
	{
	}
	virtual BrowserWindow * CreateBrowser(uint brwId, int dataOwner)
	{
		PPViewBrowser * p_brw = new PPViewBrowser(brwId, CreateBrowserQuery(), P_V, dataOwner);
		SetupBrowserCtColumns(p_brw);
		return p_brw;
	}
protected:
	virtual void GetTabTitle(const void * pVal, TYPEID typ, SString & rBuf) const
	{
		if(pVal && typ == MKSTYPE(S_INT, 4) && P_V) 
			P_V->GetTabTitle(*static_cast<const long *>(pVal), rBuf);
	}
	PPViewDebtTrnovr * P_V;
};

int PPViewDebtTrnovr::Init_(const PPBaseFilt * pBaseFilt)
{
	// @todo (учесть флаг ROXF_RECKONNEGONLY) const bool reckon_neg_only = LOGIC(reckon_data.Flags & ROXF_RECKONNEGONLY);
	int    ok = 1;
	int    use_ta = 1;
	uint   i;
	BExtQuery * p_q = 0;
	PPObjOprKind op_obj;
	PPIDArray cur_list, tab_list;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	ZDELETE(P_Ct);
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempTbl);
	ZDELETE(P_IterBillList);
	ZDELETE(P_DebtDimAgentList);
	IterBillCounter = 0;
	Total.Init();
	Bsp.Init(Filt.Sgb);
	RcknBsp.Init(Filt.Sgb);
	GoodsList.clear();
	//
	DlvrAddrList.freeAll();
	IsDlvrAddrListInited = 0;
	//
	//
	Filt.Period.Actualize(ZERODATE);
	Filt.PaymPeriod.Actualize(ZERODATE);
	Filt.ExpiryPeriod.Actualize(ZERODATE);
	//
	// Инициализация параметров цикличности отчета
	//
	DaySieve.freeAll();
	CycleSieve.freeAll();
	if(Filt.CycleKind && Filt.Cf.Cycle > 0 && Filt.Cf.NumCycles > 0) {
		if(Filt.CycleKind == DebtTrnovrFilt::ckExpiry || Filt.CycleKind == DebtTrnovrFilt::ckDelay) {
			for(int i = 1; i <= Filt.Cf.NumCycles; i++)
				DaySieve.add(i * Filt.Cf.Cycle);
			DaySieve.add(MAXLONG);
		}
		else if(Filt.CycleKind == DebtTrnovrFilt::ckShipments)
			CycleSieve.init(&Filt.Period, Filt.Cf);
		else if(Filt.CycleKind == DebtTrnovrFilt::ckPayments)
			CycleSieve.init(&Filt.PaymPeriod, Filt.Cf);
		else
			Filt.CycleKind = DebtTrnovrFilt::ckNone;
	}
	else
		Filt.CycleKind = DebtTrnovrFilt::ckNone;
	if(Filt.ExtKind == DebtTrnovrFilt::ekExpiryPart) {
		Filt.Flags &= ~DebtTrnovrFilt::fExtended;
	}
	else if(Filt.ExtKind == DebtTrnovrFilt::ekTurnover) {
		Filt.Flags &= ~DebtTrnovrFilt::fExtended;
		Filt.Flags |= DebtTrnovrFilt::fCalcTotalDebt;
	}
	else
		Filt.ExtKind = DebtTrnovrFilt::ekNone;
	{
		PPUserFuncProfiler ufp(PPUPRF_VIEW_DEBT);
		double ufp_factor = 0.0;
		if(Filt.GoodsGrpID) {
			GoodsIterator::GetListByGroup(Filt.GoodsGrpID, &GoodsList);
			GoodsList.sortAndUndup();
		}
		PayableOpList.clear();
		THROW(op_obj.GetPayableOpList(Filt.AccSheetID, &PayableOpList));
		if(Filt.OpID) {
			PPIDArray op_list;
			if(IsGenericOp(Filt.OpID))
				GetGenericOpList(Filt.OpID, &op_list);
			else
				op_list.add(Filt.OpID);
			uint   c = PayableOpList.getCount();
			if(c) do {
				PPID   op_id = PayableOpList.get(--c);
				if(!op_list.lsearch(op_id))
					PayableOpList.atFree(c);
			} while(c);
		}
		if(!(Filt.Flags & DebtTrnovrFilt::fNoTempTable)) {
			THROW(P_TempTbl = CreateTempFile());
			{
				uint   j;
				SString  msg_buf, name_buf;
				BillTbl::Rec bill_rec;
				UintHashTable finished_bill_tab;
				const int by_obj_subst = (Filt.Sgb.S == SubstGrpBill::sgbObject) ? ((Filt.Sgb.S2.Sgp == sgpNone) ? 1 : 2) : 0;
				PPLoadText(PPTXT_WAIT_DEBTTRNOVR, msg_buf);
				ProcessBlock bblk(Filt);
				if(Filt.BillList.IsExists()) {
					uint   bc = Filt.BillList.GetCount();
					bblk.IterPath = ProcessBlock::ipBillList;
					for(j = 0; j < bc; j++) {
						if(P_BObj->Search(Filt.BillList.Get(j), &bill_rec) > 0) {
							THROW(ProcessBill(bill_rec, bblk));
							ufp_factor += 1.0;
						}
						PPWaitPercent(j+1, bc);
					}
					if(Filt.RcknBillList.IsExists()) {
						uint   rbc = Filt.RcknBillList.GetCount();
						bblk.IterPath = ProcessBlock::ipReckonBillList;
						for(j = 0; j < rbc; j++) {
							if(P_BObj->Search(Filt.RcknBillList.Get(j), &bill_rec) > 0) {
								THROW(ProcessBill(bill_rec, bblk));
								ufp_factor += 1.0;
							}
							PPWaitPercent(j+1, rbc);
						}
					}
				}
				else {
					long   _c = 0;
					PPIDArray processed_ar_list;
					while(NextProcessIteration(0, bblk) > 0) {
						LDATE  last_date = ZERODATE;
						while(NextProcessStep(bill_rec, bblk) > 0) {
							int r;
							if(bblk.IterPath == ProcessBlock::ipOp) {
								if(last_date)
									PPWaitPercent(bblk.Cntr.Add(diffdate(bill_rec.Dt, last_date)), bblk.IterMsgPrefix);
								last_date = bill_rec.Dt;
							}
							THROW(PPCheckUserBreak());
							THROW(r = ProcessBill(bill_rec, bblk));
							ufp_factor += 1.0;
							if(r > 0 && by_obj_subst && Filt.Flags & DebtTrnovrFilt::fExtended)
								processed_ar_list.addUnique(bill_rec.Object);
						}
						if(bblk.IterPath == ProcessBlock::ipArticle)
							PPWaitPercent(bblk.Cntr.Increment(), bblk.IterMsgPrefix);
					}
					BExtQuery::ZDelete(&bblk.P_Q);
					if(by_obj_subst && Filt.Flags & DebtTrnovrFilt::fExtended) {
						ReckonOpArList roa_list;
						PPIDArray tmp_op_list;
						for(j = 0; j < PayableOpList.getCount(); j++) {
							roa_list.clear();
							THROW(P_BObj->GetPaymentOpListByDebtOp(PayableOpList.get(j), 0, &roa_list));
							for(uint k = 0; k < roa_list.getCount(); k++)
								tmp_op_list.addUnique(roa_list.at(k).PayableOpID);
						}
						if(by_obj_subst == 1 && fdivnz(processed_ar_list.getCount(), bblk.FullArListCount) < 0.1) {
							PPIDArray bill_list, temp_list;
							for(j = 0; j < processed_ar_list.getCount(); j++) {
								PayableBillList pbill_list;
								THROW(GetReceivableBillList(processed_ar_list.get(j), &pbill_list));
								temp_list.clear();
								pbill_list.GetIdList(temp_list);
								bill_list.addUnique(&temp_list);
							}
							bblk.IterPath = ProcessBlock::ipReckonOp;
							for(j = 0; j < bill_list.getCount(); j++) {
								if(P_BObj->Search(bill_list.get(j), &bill_rec) > 0) {
									THROW(ProcessBill(bill_rec, bblk));
									ufp_factor += 1.0;
								}
							}
						}
						else {
							bblk.ResetIter();
							for(j = 0; j < tmp_op_list.getCount(); j++) {
								const  PPID op_id = tmp_op_list.get(j);
								if(op_id && NextProcessIteration(op_id, bblk) > 0) {
									while(NextProcessStep(bill_rec, bblk) > 0) {
										THROW(PPCheckUserBreak());
										THROW(ProcessBill(bill_rec, bblk));
										ufp_factor += 1.0;
									}
								}
								BExtQuery::ZDelete(&bblk.P_Q);
							}
						}
					}
				}
				{
					PPTransaction tra(ppDbDependTransaction, use_ta);
					THROW(tra);
					for(i = 0; i < bblk.getCount(); i++) {
						int    count_this_client = 0;
						int    stop = 0;
						long   ar_no = 0;
						cur_list.clear();
						tab_list.clear();
						DebtEntry & r_entry = *bblk.at(i);
						if(by_obj_subst == 1) {
							ArticleTbl::Rec ar_rec;
							if(ArObj.Fetch(r_entry.ID, &ar_rec) > 0) {
								ar_no = ar_rec.Article;
								stop = BIN(ar_rec.Flags & ARTRF_STOPBILL);
							}
						}
						P_BObj->GetSubstText(r_entry.ID, &Bsp, name_buf);
						r_entry.DbtList.GetCurList(-1L, &cur_list);
						r_entry.DbtList.GetAmtTypeList(&tab_list);
						r_entry.PaymList.GetCurList(-1L, &cur_list);
						r_entry.PaymList.GetAmtTypeList(&tab_list);
						r_entry.RDbtList.GetCurList(-1L, &cur_list);
						r_entry.RDbtList.GetAmtTypeList(&tab_list);
						r_entry.ExpiryDebtList.GetCurList(-1L, &cur_list);
						r_entry.ExpiryDebtList.GetAmtTypeList(&tab_list);
						const uint cc = cur_list.getCount();
						const uint tc = tab_list.getCount();
						for(uint ci = 0; ci < cc; ci++) {
							const  PPID cur_id = cur_list.at(ci);
							{
								double paym_sum = 0.0;
								uint   num_val = 0;
								uint   num_nz_val = 0;
								for(j = 0; j < tc; j++) {
									double paym = r_entry.PaymList.Get(tab_list.get(j), cur_id);
									num_val++;
									if(paym != 0.0)
										num_nz_val++;
									paym_sum += paym;
								}
								if(num_nz_val)
									r_entry._AvgPaym = R2(paym_sum / num_nz_val);
							}
							for(j = 0; j < tc; j++) {
								int    r;
								const  long tab_id = tab_list.get(j);
								TempSellTrnovrTbl::Rec rec;
								rec.ID    = r_entry.ID;
								rec.TabID = tab_id;
								rec.NotStop = stop ? 0 : 1;
								rec.Ar = ar_no;
								name_buf.CopyTo(rec.Name, sizeof(rec.Name));
								THROW(r = SetupRecVals(cur_id, tab_id, &r_entry, &rec));
								if(r > 0) {
									THROW_DB(P_TempTbl->insertRecBuf(&rec));
									count_this_client = 1;
								}
							}
						}
						if(count_this_client)
							Total.Count++;
					}
					THROW(tra.Commit());
				}
			}
			ZDELETE(P_Ct);
			if(Filt.CycleKind && P_TempTbl) {
				SString temp_buf;
				DBFieldList total_list;
				THROW_MEM(P_Ct = new DebtTrnovrCrosstab(this));
				P_Ct->SetTable(P_TempTbl, P_TempTbl->TabID);
				P_Ct->AddIdxField(P_TempTbl->ID);
				P_Ct->AddIdxField(P_TempTbl->CurID);
				P_Ct->AddInheritedFixField(P_TempTbl->Name);
				P_Ct->AddInheritedFixField(P_TempTbl->Ar);
				P_Ct->AddAggrField(P_TempTbl->Sell);
				const DBField * p_aggr_fld = 0;
				if(Filt.CycleKind == DebtTrnovrFilt::ckPayments)
					p_aggr_fld = &P_TempTbl->Payment;
				else
					p_aggr_fld = &P_TempTbl->Debt;
				P_Ct->AddAggrField(*p_aggr_fld);
				total_list.Add(*p_aggr_fld);
				P_Ct->AddTotalRow(total_list, 0, PPGetWord(PPWORD_TOTAL, 0, temp_buf));
				P_Ct->AddTotalColumn(*p_aggr_fld, 0, temp_buf);
				THROW(P_Ct->Create(use_ta));
				ufp_factor *= 1.1;
			}
		}
		if(Filt.GoodsGrpID)
			ufp_factor *= 1.2;
		ufp.SetFactor(0, ufp_factor);
		ufp.Commit();
	}
	CATCH
		ZDELETE(P_Ct);
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

PPViewDebtTrnovr::ProcessBlock::ProcessBlock(const DebtTrnovrFilt & rF) : TSCollection <DebtEntry> (), R_F(rF), P_Q(0),
	CurrentDate(NZOR(rF.PaymPeriod.upp, getcurdate_())),
	DoCheckAddr(BIN(rF.CityID)),
	ByLinks(BIN(rF.Flags & DebtTrnovrFilt::fNoForwardPaym || !rF.PaymPeriod.IsZero())),
	ByCost(BIN(rF.Flags & DebtTrnovrFilt::fByCost)),
	P_PaymPeriod((rF.Flags & DebtTrnovrFilt::fNoForwardPaym && rF.PaymPeriod.IsZero()) ? &rF.Period : &rF.PaymPeriod)
{
	ResetIter();
	//
	PPObjArticle ar_obj;
	ArticleTbl::Rec ar_rec;
	PPIDArray full_ar_list;
	ar_obj.P_Tbl->GetListBySheet(rF.AccSheetID, &full_ar_list, 0);
	if(rF.CliIDList.getCount()) {
		PPIDArray temp_list;
		for(uint i = 0; i < rF.CliIDList.getCount(); i++) {
			const  PPID ar_id = rF.CliIDList.at(i);
			if(ar_obj.Fetch(ar_id, &ar_rec) > 0 && ar_rec.AccSheetID == rF.AccSheetID) {
				if(ar_rec.Flags & ARTRF_GROUP)
					ar_obj.P_Tbl->GetListByGroup(ar_id, &ArList);
				else if(!(rF.Flags & DebtTrnovrFilt::fSkipPassive) || !ar_rec.Closed) {
					if(rF.Sgb.S == SubstGrpBill::sgbObject && rF.Sgb.S2.Sgp > sgpFirstRelation) {
						temp_list.clear();
						ar_obj.GetRelPersonList(ar_id, (rF.Sgb.S2.Sgp - sgpFirstRelation), 1, &temp_list);
						ArList.add(&temp_list);
					}
					ArList.add(ar_id);
				}
			}
		}
	}
	else {
		for(uint i = 0; i < full_ar_list.getCount(); i++) {
			const  PPID ar_id = full_ar_list.get(i);
			if(ar_obj.Fetch(ar_id, &ar_rec) > 0) {
				if(!(rF.Flags & DebtTrnovrFilt::fSkipPassive) || !ar_rec.Closed)
					ArList.add(ar_id); 
			}
		}
	}
	FullArListCount = full_ar_list.getCount();
	ArList.sortAndUndup(); 
	IterPath = ProcessBlock::ipUndef;
	IterN = 0;
}

PPViewDebtTrnovr::ProcessBlock::~ProcessBlock()
{
	BExtQuery::ZDelete(&P_Q);
}

PPViewDebtTrnovr::ProcessBlock & PPViewDebtTrnovr::ProcessBlock::ResetIter()
{
	ResetStep();
	IterPath = ProcessBlock::ipUndef;
	IterN = 0;
	IterMsgPrefix.Z();
	ReckonOpID = 0;
	ReckonAccSheetID = 0;
	Cntr.Init();
	ExtBillList.freeAll();
	return *this;
}

PPViewDebtTrnovr::ProcessBlock & PPViewDebtTrnovr::ProcessBlock::ResetStep()
{
	Flags = 0;
	PayDate = ZERODATE;
	Date = ZERODATE;
	CurID = 0;
	Amount = 0.0;
	Paym = 0.0;
	Debt = 0.0;
	ExpiryDebt = 0.0;
	Cost = 0.0;
	P_Entry = 0;
	MEMSZERO(Ext);
	PaymList.clear();
	return *this;
}

double PPViewDebtTrnovr::ProcessBlock::MultCostCoef(double val, int zeroIfDivZero) const
{
	return (Amount != 0.0) ? (val * (Cost / Amount)) : (zeroIfDivZero ? 0.0 : val);
}

int PPViewDebtTrnovr::ProcessBlock::AddStepItem(PPID tabID, double paym, double expiryDebt)
{
	int    ok = 1;
	if(P_Entry) {
		if(oneof2(IterPath, ProcessBlock::ipReckonOp, ProcessBlock::ipReckonBillList)) {
			P_Entry->RDbtList.Add(tabID, CurID, ByCost ? MultCostCoef(Amount, 0) : Amount);
			P_Entry->RcknList.Add(tabID, CurID, ByCost ? MultCostCoef(paym, 1) : paym);
		}
		else {
			P_Entry->DbtList.Add(tabID, CurID, ByCost ? MultCostCoef(Amount, 0) : Amount);
			P_Entry->PaymList.Add(tabID, CurID, ByCost ? MultCostCoef(paym, 1) : paym);
			if(expiryDebt > 0.0) {
				P_Entry->ExpiryDebtList.Add(tabID, CurID, ByCost ? MultCostCoef(expiryDebt, 0) : expiryDebt);
			}
		}
	}
	else
		ok = 0;
	return ok;
}

int PPViewDebtTrnovr::ProcessBillPaymPlanEntry(const BillTbl::Rec & rRec, const PayPlanTbl::Rec & rPayPlanEntry, PPID arID, bool inverseSign, ProcessBlock & rBlk)
{
	int    ok = 1;
	rBlk.PayDate = rPayPlanEntry.PayDate;
	if(Filt.ExpiryPeriod.CheckDate(rBlk.PayDate)) {
		if(Filt.CycleKind == DebtTrnovrFilt::ckPayments)
			P_BObj->P_Tbl->CalcPaymentSieve(rRec.ID, rRec.CurID, &CycleSieve, &rBlk.PaymList, &rBlk.Paym);
		else if(UseOmtPaymAmt && !rBlk.ByLinks)
			rBlk.Paym = rRec.PaymAmount;
		else
			P_BObj->P_Tbl->CalcPayment(rRec.ID, rBlk.ByLinks, rBlk.P_PaymPeriod, rRec.CurID, &rBlk.Paym);
		// @v10.3.6 {
		double effective_amount = rRec.Amount;
		if(effective_amount < 0.0) {
			if(inverseSign) { // @v11.7.11
				effective_amount = -effective_amount;
				rBlk.Paym = -rBlk.Paym;
				rBlk.Debt = (effective_amount - rBlk.Paym);
			}
			else {
				rBlk.Paym = -rBlk.Paym;
				rBlk.Debt = (effective_amount - rBlk.Paym);
			}
		}
		else { // } @v10.3.6
			rBlk.Debt = (effective_amount - rBlk.Paym);
		}
		if(!(Filt.Flags & DebtTrnovrFilt::fDebtOnly) || rBlk.Debt > 0.0) {
			if(Filt.Flags & DebtTrnovrFilt::fByCost)
				P_BObj->P_Tbl->GetAmount(rRec.ID, PPAMT_BUYING, rRec.CurID, &rBlk.Cost);
			rBlk.Amount = effective_amount;
			rBlk.CurID = rRec.CurID;
			rBlk.Date  = rRec.Dt;
			if(rBlk.PayDate < rBlk.CurrentDate)
				rBlk.ExpiryDebt = rBlk.Debt;
			//
			uint   j;
			long   tab_id = 0;
			PPID   ident = 0;
			if(!!Filt.Sgb) {
				PPBill _pack;
				_pack.Rec = rRec;
				_pack.Rec.Object = arID; // Статья могла быть замещена для зачетного документа
				P_BObj->LoadForSubst(&Bsp, &_pack);
				P_BObj->Subst(&_pack, &ident, oneof2(rBlk.IterPath, ProcessBlock::ipReckonOp, ProcessBlock::ipReckonBillList) ? &RcknBsp : &Bsp);
			}
			else
				ident = rRec.Object;
			//
			rBlk.P_Entry = 0;
			if(rBlk.lsearch(&ident, &(j = 0), CMPF_LONG)) {
				rBlk.P_Entry = rBlk.at(j);
			}
			else {
				THROW_MEM(rBlk.P_Entry = new DebtEntry(ident));
				THROW_SL(rBlk.insert(rBlk.P_Entry));
			}
			//
			if(oneof2(Filt.CycleKind, DebtTrnovrFilt::ckExpiry, DebtTrnovrFilt::ckDelay)) {
				if(rBlk.Debt > 0.0) {
					LDATE  pd = (Filt.CycleKind == DebtTrnovrFilt::ckExpiry && rBlk.PayDate) ? rBlk.PayDate : rBlk.Date;
					long   nd = diffdate(rBlk.CurrentDate, pd);
					if(nd >= 0) {
						const uint dsc = DaySieve.getCount();
						long  prev_s = -MAXLONG;
						for(j = 0; tab_id == 0 && j < dsc; j++) {
							const long s = DaySieve.at(j);
							if(nd <= s && nd >= prev_s)
								tab_id = j+1;
							prev_s = s+1;
						}
						if(tab_id == 0 && dsc && DaySieve.at(dsc-1) == MAXLONG)
							tab_id = dsc;
						THROW(rBlk.AddStepItem(tab_id, rBlk.Paym, 0.0/*expiryDebt*/));
					}
				}
			}
			else if(Filt.CycleKind == DebtTrnovrFilt::ckShipments) {
				if(CycleSieve.searchDate(rBlk.Date, &(j = 0)))
					THROW(rBlk.AddStepItem(j+1, rBlk.Paym, rBlk.ExpiryDebt));
			}
			else if(Filt.CycleKind == DebtTrnovrFilt::ckPayments) {
				int    first_point = 1;
				double p = 0.0;
				for(j = 0; j < CycleSieve.getCount(); j++) {
					uint   cs_pos = 0;
					if(rBlk.PaymList.Search(j, &p, &cs_pos)) {
						if(!first_point)
							rBlk.Amount = rBlk.Cost = 0.0;
						tab_id = j+1;
						THROW(rBlk.AddStepItem(tab_id, p, 0.0/*expiryDebt*/));
						first_point = 0;
					}
				}
			}
			else if(Filt.ExtKind == DebtTrnovrFilt::ekExpiryPart) {
				if(rBlk.Debt > 0.0) {
					const LDATE  pd = (Filt.ExtExpiryTerm > 0) ? plusdate(rBlk.Date, Filt.ExtExpiryTerm) : NZOR(rBlk.PayDate, rBlk.Date);
					const double _exp_debt = (pd < rBlk.CurrentDate) ? rBlk.Debt : 0.0;
					//
					// Для этого (специального) вида отчета в качестве оплаты учитываем
					// общую сумму долга за вычетом просроченного долга.
					// А сумму документа подменяем на полный долг.
					//
					rBlk.Amount = rBlk.Debt;
					THROW(rBlk.AddStepItem(tab_id, rBlk.Debt - _exp_debt, 0.0/*expiryDebt*/));
				}
			}
			else {
				THROW(rBlk.AddStepItem(tab_id, rBlk.Paym, rBlk.ExpiryDebt));
			}
		}
		else
			ok = -1;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPViewDebtTrnovr::PreprocessBill(const BillTbl::Rec & rRec, const ProcessBlock & rBlk, PPID * pArID, PPBillExt * pBillExt, double * pPart)
{
	//
	// Проверка документа на соответствие критериям фильтра
	//
	int    ok = 1;
	double __part = 1.0;
	PPID   ar_id = rRec.Object;
	PPBillExt bill_ext;
	THROW((!(Filt.Flags & DebtTrnovrFilt::fLabelOnly) || rRec.Flags & BILLF_WHITELABEL) && (!Filt.Article2ID || rRec.Object2 == Filt.Article2ID));
	if(rBlk.IterPath == ProcessBlock::ipByAgentList) {
		THROW(Filt.Period.CheckDate(rRec.Dt));
		THROW(Filt.CurID < 0 || rRec.CurID == Filt.CurID);
		THROW(!Filt.LocIDList.isList() || Filt.LocIDList.lsearch(rRec.LocID));
		THROW(rBlk.ArList.bsearch(ar_id));
		THROW(PayableOpList.lsearch(rRec.OpID));
	}
	else if(rBlk.IterPath == ProcessBlock::ipOp) { // При переборе по операциям необходимо проверить принадлежность статьи фильтрующему списку
		THROW(rBlk.ArList.bsearch(ar_id));
	}
	else if(rBlk.IterPath == ProcessBlock::ipArticle) { // При переборе по статьям необходимо проверить принадлежность операции списку оплачиваемых
		THROW(PayableOpList.lsearch(rRec.OpID));
	}
	else if(oneof2(rBlk.IterPath, ProcessBlock::ipReckonOp, ProcessBlock::ipReckonBillList)) {
		if(rBlk.ReckonAccSheetID != Filt.AccSheetID) {
			PPID   alt_ar_id = 0;
			P_BObj->GetAlternateArticle(ar_id, Filt.AccSheetID, &alt_ar_id);
			THROW(alt_ar_id && rBlk.ArList.bsearch(alt_ar_id));
			ar_id = alt_ar_id;
		}
		else {
			THROW(rBlk.ArList.bsearch(ar_id));
		}
	}
	if(rBlk.IterPath < 10) {
		if(rBlk.DoCheckAddr || Filt.CategoryID) {
			PPID   psn_id = ObjectToPerson(ar_id, 0);
			PPID   addr_id = 0, dlvr_addr_id = 0;
			PersonTbl::Rec psn_rec;
			if(rBlk.DoCheckAddr) {
				if(Filt.Flags & DebtTrnovrFilt::fDeliveryAddr) {
					GetDlvrAddr(rRec.ID, &dlvr_addr_id);
					PsnObj.GetAddrID(psn_id, dlvr_addr_id, PSNGETADDRO_DLVRADDR, &addr_id);
				}
				else if(psn_id && PsnObj.Fetch(psn_id, &psn_rec) > 0) {
					addr_id = NZOR(psn_rec.RLoc, psn_rec.MainLoc);
				}
				THROW(CheckAddress(addr_id));
			}
			if(Filt.CategoryID) {
				THROW(psn_id && PsnObj.Fetch(psn_id, &psn_rec) > 0 && psn_rec.CatID == Filt.CategoryID);
			}
		}
		if(!oneof2(rBlk.IterPath, ProcessBlock::ipReckonOp, ProcessBlock::ipReckonBillList) && (Filt.AgentID || Filt.PayerID)) {
			THROW(P_BObj->FetchExt(rRec.ID, &bill_ext) > 0);
			THROW((!Filt.AgentID || bill_ext.AgentID == Filt.AgentID) && (!Filt.PayerID || bill_ext.PayerID == Filt.PayerID));
		}
	}
	if(Filt.DebtDimList.GetCount()) {
		THROW(P_BObj->FetchExt(rRec.ID, &bill_ext) > 0);
		const  PPID agent_id = bill_ext.AgentID;
		if(!P_DebtDimAgentList) {
			THROW_MEM(P_DebtDimAgentList = new LAssocArray);
			{
				PPObjDebtDim dd_obj;
				dd_obj.FetchAgentList(P_DebtDimAgentList);
			}
		}
		THROW(P_DebtDimAgentList && P_DebtDimAgentList->getCount());
		{
			LongArray dd_list;
			P_DebtDimAgentList->GetListByVal(agent_id, dd_list);
			const LongArray & r_filter_dd_list = Filt.DebtDimList.Get();
            dd_list.intersect(&r_filter_dd_list, 0);
            THROW(dd_list.getCount());
		}
	}
	if(Filt.GoodsGrpID) {
		if(rRec.Amount != 0.0) {
			PPBillPacket bp;
			if(P_BObj->ExtractPacketWithRestriction(rRec.ID, &bp, 0, &GoodsList) > 0) {
				bp.InitAmounts();
				__part = bp.Rec.Amount / rRec.Amount;
			}
		}
		else
			__part = 0.0;
	}
	CATCHZOK
	ASSIGN_PTR(pArID, ar_id);
	ASSIGN_PTR(pBillExt, bill_ext);
	ASSIGN_PTR(pPart, __part);
	return ok;
}

int PPViewDebtTrnovr::ProcessBill(const BillTbl::Rec & rRec, ProcessBlock & rBlk)
{
	//
	// Проверка документа на соответствие критериям фильтра
	//
	int    ok = 1;
	PPID   ar_id = 0;
	PPBillExt bill_ext;
	double __part = 1.0;
	if(PreprocessBill(rRec, rBlk, &ar_id, &bill_ext, &__part) > 0) {
		rBlk.ResetStep();
        rBlk.Ext = bill_ext;
		LDATE  pay_date = ZERODATE;
		PayPlanTbl::Rec pay_plan_entry;
		// @v11.7.11 (пытаюсь учесть флаг зачетной операции ROXF_RECKONNEGONLY) {
		bool   do_process_positive_amt_only = false;
		bool   inverse_sign = false;
		double effective_rec_amt = 0.0;
		if(oneof2(rBlk.IterPath, ProcessBlock::ipReckonOp, ProcessBlock::ipReckonBillList)) {
			PPObjOprKind op_obj;
			PPReckonOpEx rox;
			const bool reckon_neg_only = (op_obj.FetchReckonExData(rRec.OpID, &rox) > 0) ? LOGIC(rox.Flags & ROXF_RECKONNEGONLY) : false;
			if(rRec.Amount > 0.0 && !reckon_neg_only)
				effective_rec_amt = rRec.Amount;
			else if(rRec.Amount < 0.0 && reckon_neg_only) {
				effective_rec_amt = -rRec.Amount;
				inverse_sign = true;
			}
			do_process_positive_amt_only = true;
		}
		else
			effective_rec_amt = rRec.Amount;
		if(!do_process_positive_amt_only || effective_rec_amt > 0.0) { 
			// } @v11.7.11
			if(Filt.ExpiryPeriod.IsZero()) {
				P_BObj->P_Tbl->GetLastPayDate(rRec.ID, &pay_date);
				MEMSZERO(pay_plan_entry);
				pay_plan_entry.Amount = effective_rec_amt;
				pay_plan_entry.PayDate = pay_date;
				THROW(ok = ProcessBillPaymPlanEntry(rRec, pay_plan_entry, ar_id, inverse_sign, rBlk));
			}
			else {
				PayPlanArray pay_plan;
				P_BObj->P_Tbl->GetPayPlan(rRec.ID, &pay_plan);
				pay_plan.Sort();
				MEMSZERO(pay_plan_entry);
				double pay_plan_total = 0.0;
				for(uint i = 0; i < pay_plan.getCount(); i++) {
					const PayPlanTbl::Rec & r_item = pay_plan.at(i); // @v8.5.11 @fix r_item --> &r_item
					pay_plan_total += r_item.Amount;
					if(Filt.ExpiryPeriod.CheckDate(r_item.PayDate)) {
						pay_plan_entry.BillID = rRec.ID;
						pay_plan_entry.PayDate = r_item.PayDate;
						pay_plan_entry.Amount += r_item.Amount;
					}
				}
				if(pay_plan_entry.BillID) {
					BillTbl::Rec temp_bill_rec;
					temp_bill_rec = rRec;
					temp_bill_rec.Amount = fdivnz(pay_plan_entry.Amount, pay_plan_total) * effective_rec_amt;
					THROW(ok = ProcessBillPaymPlanEntry(temp_bill_rec, pay_plan_entry, ar_id, inverse_sign, rBlk));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPViewDebtTrnovr::NextProcessStep(BillTbl::Rec & rRec, ProcessBlock & rBlk)
{
	int    ok = -1;
	if(rBlk.P_Q) {
		if(rBlk.P_Q->nextIteration() > 0) {
			P_BObj->P_Tbl->CopyBufTo(&rRec);
			ok = 1;
		}
	}
	else if(rBlk.IterPath == ProcessBlock::ipByAgentList) {
		PPIDArray & r_ext_list = rBlk.ExtBillList;
		while(ok < 0 && r_ext_list.testPointer()) {
			rBlk.Cntr.Increment();
			if(P_BObj->Search(r_ext_list.get(r_ext_list.getPointer()), &rRec) > 0)
				ok = 1;
			r_ext_list.incPointer();
		}
	}
	return ok;
}

int PPViewDebtTrnovr::NextProcessIteration(PPID reckonOpID, ProcessBlock & rBlk)
{
	BExtQuery::ZDelete(&rBlk.P_Q);
	rBlk.ExtBillList.clear();

	int    ok = -1;
	BillCore * t = P_BObj->P_Tbl;
	if(reckonOpID) {
		rBlk.IterPath = ProcessBlock::ipReckonOp;
		rBlk.ReckonOpID = reckonOpID;
		PPOprKind op_rec;
		PPLoadText(PPTXT_WAIT_DEBTTRNOVR, rBlk.IterMsgPrefix);
		if(GetOpData(reckonOpID, &op_rec) > 0) {
			rBlk.ReckonAccSheetID = op_rec.AccSheetID;
			DBQ * dbq = 0;
			BillTbl::Key2 k2;
			THROW_MEM(rBlk.P_Q = new BExtQuery(t, 2, 1024)); // @v10.0.02 256-->1024
			rBlk.P_Q->select(t->ID, t->Dt, t->OpID, t->Object, t->Object2, t->StatusID, t->Flags, t->CurID, t->Amount, t->PaymAmount, 0L);
			dbq = & (t->OpID == reckonOpID && daterange(t->Dt, &Filt.Period));
			if(Filt.CurID >= 0)
				dbq = & (*dbq && t->CurID == Filt.CurID);
			dbq = ppcheckfiltidlist(dbq, t->LocID, &Filt.LocIDList);
			rBlk.P_Q->where(*dbq);
			k2.OpID = reckonOpID;
			k2.Dt = Filt.Period.low;
			k2.BillNo = 0;
			rBlk.P_Q->initIteration(false, &k2, spGe);
			ok = 1;
		}
	}
	else {
		if(rBlk.IterN == 0 && rBlk.IterPath == ProcessBlock::ipUndef) {
			const uint ar_c = rBlk.ArList.getCount();
			long   iter_count = 0;
			PPLoadText(PPTXT_WAIT_DEBTTRNOVR, rBlk.IterMsgPrefix);
			if(fdivnz(rBlk.ArList.getCount(), rBlk.FullArListCount) < 0.1) {
				rBlk.IterPath = ProcessBlock::ipArticle;
				iter_count = (long)rBlk.ArList.getCount();
			}
			else if(Filt.AgentID || Filt.PayerID) {
				rBlk.IterPath = ProcessBlock::ipByAgentList;
				P_BObj->P_Tbl->GetBillListByExt(Filt.AgentID, Filt.PayerID, rBlk.ExtBillList);
				rBlk.ExtBillList.setPointer(0);
				iter_count = (long)rBlk.ExtBillList.getCount();
			}
			else {
				rBlk.IterPath = ProcessBlock::ipOp;
				for(uint i = 0; i < PayableOpList.getCount(); i++) {
					const  PPID op_id = PayableOpList.get(i);
					LDATE beg = Filt.Period.low;
					LDATE end = Filt.Period.upp;
					if(!beg) {
						t->GetFirstDate(op_id, &beg);
						if(oneof2(beg, MAXDATE, ZERODATE))
							beg = getcurdate_();
					}
					if(!end) {
						t->GetLastDate(op_id, &end);
						if(oneof2(end, MAXDATE, ZERODATE))
							end = getcurdate_();
					}
					iter_count += diffdate(end, beg); // Единица не прибавляется из-за особенностей учета итераций (см. ниже)
				}
			}
			rBlk.Cntr.Init(iter_count);
			ok = 1;
		}
		if(rBlk.IterPath == ProcessBlock::ipOp) {
			if(rBlk.IterN < PayableOpList.getCount()) {
				const  PPID  op_id = PayableOpList.get(rBlk.IterN++);
				DBQ * dbq = 0;
				BillTbl::Key2 k2;
				THROW_MEM(rBlk.P_Q = new BExtQuery(t, 2, 1024)); // @v10.0.02 256-->1024
				rBlk.P_Q->select(t->ID, t->Dt, t->OpID, t->Object, t->Object2, t->StatusID, t->Flags, t->CurID, t->Amount, t->PaymAmount, 0L);
				dbq = & (t->OpID == op_id && daterange(t->Dt, &Filt.Period));
				if(Filt.CurID >= 0)
					dbq = & (*dbq && t->CurID == Filt.CurID);
				dbq = ppcheckfiltidlist(dbq, t->LocID, &Filt.LocIDList);
				rBlk.P_Q->where(*dbq);
				k2.OpID = op_id;
				k2.Dt = Filt.Period.low;
				k2.BillNo = 0;
				rBlk.P_Q->initIteration(false, &k2, spGe);
				ok = 1;
			}
		}
		else if(rBlk.IterPath == ProcessBlock::ipArticle) {
			if(rBlk.IterN < rBlk.ArList.getCount()) {
				const  PPID  ar_id = rBlk.ArList.get(rBlk.IterN++);
				DBQ * dbq = 0;
				BillTbl::Key3 k3;
				THROW_MEM(rBlk.P_Q = new BExtQuery(t, 3, 256));
				rBlk.P_Q->select(t->ID, t->Dt, t->OpID, t->Object, t->Object2, t->StatusID, t->Flags, t->CurID, t->Amount, t->PaymAmount, 0L);
				dbq = & (t->Object == ar_id && daterange(t->Dt, &Filt.Period));
				if(Filt.CurID >= 0)
					dbq = & (*dbq && t->CurID == Filt.CurID);
				dbq = ppcheckfiltidlist(dbq, t->LocID, &Filt.LocIDList);
				rBlk.P_Q->where(*dbq);
				k3.Object = ar_id;
				k3.Dt = Filt.Period.low;
				k3.BillNo = 0;
				rBlk.P_Q->initIteration(false, &k3, spGe);
				ok = 1;
			}
		}
	}
	CATCH
		ok = 0;
		BExtQuery::ZDelete(&rBlk.P_Q);
	ENDCATCH
	return ok;
}

int PPViewDebtTrnovr::SerializeState(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	SString temp_buf;
	THROW(PPView::SerializeState(dir, rBuf, pCtx));
	THROW_SL(pCtx->Serialize(dir, &PayableOpList, rBuf));
	THROW(Total.Serialize(dir, rBuf, pCtx));
	THROW(Bsp.Serialize(dir, rBuf, pCtx));
	THROW(RcknBsp.Serialize(dir, rBuf, pCtx));
	THROW_SL(pCtx->Serialize(dir, &DlvrAddrList, rBuf));
	THROW_SL(pCtx->Serialize(dir, IsDlvrAddrListInited, rBuf));
	THROW_SL(pCtx->Serialize(dir, &DaySieve, rBuf));
	THROW(CycleSieve.Serialize(dir, rBuf, pCtx));
	THROW(SerializeDbTableByFileName <TempSellTrnovrTbl> (dir, &P_TempTbl, rBuf, pCtx));
	if(dir > 0) {
		uint8 ind = P_Ct ? 0 : 1;
		THROW_SL(rBuf.Write(ind));
		if(P_Ct) {
			THROW(P_Ct->Write(rBuf, pCtx));
		}
	}
	else if(dir < 0) {
		uint8 ind = 0;
		ZDELETE(P_Ct);
		THROW_SL(rBuf.Read(ind));
		if(ind == 0 && Filt.CycleKind && P_TempTbl) {
			THROW_MEM(P_Ct = new DebtTrnovrCrosstab(this));
			THROW(P_Ct->Read(P_TempTbl, rBuf, pCtx));
		}
	}
	CATCHZOK
	return ok;
}


int PPViewDebtTrnovr::InitIteration(IterOrder order)
{
	int    ok = 1, idx = 0;
	// @v10.6.8 char   k[MAXKEYLEN];
	BtrDbKey k_; // @v10.6.8
	switch(order) {
		case OrdByArticleID: idx = 0; break;
		case OrdByArticleName: idx = 1; break;
		case OrdByDebit: idx = 2; break;
		case OrdByDebt: idx = 3; break;
		case OrdByStop: idx = 5; break;
	}
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_IterBillList);
	IterBillCounter = 0;
	if(Filt.Flags & DebtTrnovrFilt::fPrintExt)
		THROW_MEM(P_IterBillList = new PPIDArray /*PayableBillList*/);
	PPInitIterCounter(Counter, P_TempTbl);
	THROW_MEM(P_IterQuery = new BExtQuery(P_TempTbl, idx));
	P_IterQuery->selectAll();
	// @v10.6.8 @ctr memzero(k, sizeof(k));
	THROW(P_IterQuery->initIteration(0, k_, spFirst));
	CATCHZOK
	return ok;
}

int PPViewDebtTrnovr::NextInnerIteration(int initList, DebtTrnovrViewItem * pItem)
{
	int    ok = -1;
	const  int by_links = BIN(Filt.Flags & DebtTrnovrFilt::fNoForwardPaym);
	DebtTrnovrViewItem item;
	if(P_IterBillList) {
		if(initList) {
			IterBillCounter = 0;
			P_IterBillList->clear();
			Bsp.AsscList.GetListByKey(IterCurItem.ID_, *P_IterBillList);
		}
		while(ok < 0 && IterBillCounter < (int)P_IterBillList->getCount()) {
			double paym = 0.0;
			BillTbl::Rec bill_rec, last_paym_rec;
			item = IterCurItem;
			item.BillID = P_IterBillList->get(IterBillCounter++);
			if(P_BObj->Search(item.BillID, &bill_rec) > 0) {
				item.CurID  = bill_rec.CurID;
				item.Debit  = bill_rec.Amount;
				if(UseOmtPaymAmt && !by_links) // @v8.5.8
					paym = bill_rec.PaymAmount;
				else
					P_BObj->P_Tbl->CalcPayment(item.BillID, by_links, &Filt.Period, item.CurID, &paym);
				P_BObj->P_Tbl->GetLastPayDate(item.BillID, &item.PayDate);
				item.LastPaymDate = (P_BObj->P_Tbl->GetLastPayment(item.BillID, &last_paym_rec) > 0) ? last_paym_rec.Dt : ZERODATE;
				item.Credit = paym;
				item.Debt   = bill_rec.Amount - paym;
				ASSIGN_PTR(pItem, item);
				ok = 1;
			}
		}
	}
	return ok;
}

void PPViewDebtTrnovr::InitViewItem(const TempSellTrnovrTbl::Rec * pRec, DebtTrnovrViewItem * pItem)
{
	memzero(pItem, sizeof(*pItem));
	pItem->ID_       = pRec->ID;
	{
		PPObjID oid;
		P_BObj->GetSubstObjType(pRec->ID, &Bsp, &oid);
		pItem->ObjType = oid.Obj;
	}
	pItem->Ar        = pRec->Ar;
	pItem->CurID     = pRec->CurID;
	pItem->TabID     = pRec->TabID;
	SString tab_title;
	GetTabTitle(pItem->TabID, tab_title);
	tab_title.CopyTo(pItem->TabText, sizeof(pItem->TabText));
	pItem->PersonID  = pRec->PersonID;
	STRNSCPY(pItem->ArName, pRec->Name);
	pItem->Debit     = pRec->Sell;
	pItem->Credit    = pRec->Payment;
	pItem->Debt      = pRec->Debt;
	pItem->ExpiryDebt = pRec->ExpiryDebt;
	pItem->RPaym     = pRec->RPaym;
	pItem->Reckon    = pRec->Reckon;
	pItem->RDebt     = pRec->RDebt;
	pItem->TDebt     = pRec->TDebt;
	pItem->_AvgPaym  = pRec->AvgPaym;
	pItem->IsStop    = BIN(!pRec->NotStop);
}

int FASTCALL PPViewDebtTrnovr::NextIteration(DebtTrnovrViewItem * pItem)
{
	int    r;
	if(NextInnerIteration(0, pItem) > 0)
		return 1;
	else {
		while(P_IterQuery->nextIteration() > 0) {
			Counter.Increment();
			InitViewItem(&P_TempTbl->data, &IterCurItem);
			if(P_IterBillList) {
				if((r = NextInnerIteration(1, pItem)) > 0)
					return 1;
				if(!r)
					return 0;
			}
			else {
				ASSIGN_PTR(pItem, IterCurItem);
				return 1;
			}
		}
	}
	return -1;
}

int PPViewDebtTrnovr::GetItem(PPID arID, PPID curID, long tabID, DebtTrnovrViewItem * pItem)
{
	if(P_TempTbl) {
		TempSellTrnovrTbl::Key0 k0;
		k0.ID    = arID;
		k0.CurID = curID;
		k0.TabID = tabID;
		int sp = spEq;
		if(P_Ct)
			sp = spGe;
		if(P_TempTbl->search(0, &k0, sp) && k0.ID == arID && k0.CurID == curID) {
			InitViewItem(&P_TempTbl->data, pItem);
			return 1;
		}
	}
	return -1;
}
//
//
//
class DebtTrnovrFiltDialog : public WLDialog {
public:
	enum {
		ctlgroupLoc = 2
	};
	DebtTrnovrFiltDialog() : WLDialog(DLG_SLLTOFLT, CTL_SLLTOFLT_WL)
	{
		addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_SLLTOFLT_LOC, 0, 0, cmLocList, 0, 0, 0));
		SetupCalPeriod(CTLCAL_SLLTOFLT_PERIOD, CTL_SLLTOFLT_PERIOD);
		SetupCalPeriod(CTLCAL_SLLTOFLT_PAYMPERIOD, CTL_SLLTOFLT_PAYMPERIOD);
		SetupCalPeriod(CTLCAL_SLLTOFLT_EXPIRYPRD, CTL_SLLTOFLT_EXPIRYPRD);
		showCtrl(CTL_SLLTOFLT_EXTBUTTON,   true);
		showCtrl(CTL_SLLTOFLT_CYCLEBUTTON, true);
	}
	void   SetupOp()
	{
		PPIDArray op_list, gen_op_list;
		GetOpList(PPOPT_GENERIC, &gen_op_list);
		if(Data.AccSheetID) {
			PPObjOprKind op_obj;
			op_obj.GetPayableOpList(Data.AccSheetID, &op_list);
		}
		op_list.addUnique(&gen_op_list);
		if(Data.OpID && !op_list.lsearch(Data.OpID))
			Data.OpID = 0;
		SetupOprKindCombo(this, CTLSEL_SLLTOFLT_OP, 0, Data.OpID, &op_list, OPKLF_OPLIST);
	}
	int    setDTS(const DebtTrnovrFilt * pData)
	{
		Data = *pData;

		int    ok = 1;
		SetPeriodInput(this, CTL_SLLTOFLT_PERIOD,     Data.Period);
		SetPeriodInput(this, CTL_SLLTOFLT_EXPIRYPRD,  Data.ExpiryPeriod);
		SetPeriodInput(this, CTL_SLLTOFLT_PAYMPERIOD, Data.PaymPeriod);
		setWL(BIN(Data.Flags & DebtTrnovrFilt::fLabelOnly));
		SetupPPObjCombo(this, CTLSEL_SLLTOFLT_ACCSHEET, PPOBJ_ACCSHEET, Data.AccSheetID, OLW_LOADDEFONOPEN, 0);
		SetupOp();
		SetupArCombo(this, CTLSEL_SLLTOFLT_CLIENT, Data.CliIDList.getSingle(), OLW_LOADDEFONOPEN, Data.AccSheetID, sacfDisableIfZeroSheet);
		LocationCtrlGroup::Rec l_rec;
		l_rec.LocList.Set(&Data.LocIDList);
		setGroupData(ctlgroupLoc, &l_rec);
		SetupCurrencyCombo(this, CTLSEL_SLLTOFLT_CUR, Data.CurID, 0, 1, 0);
		setCtrlUInt16(CTL_SLLTOFLT_ALLCUR, BIN(Data.Flags & DebtTrnovrFilt::fAllCurrencies));
		SetupPPObjCombo(this, CTLSEL_SLLTOFLT_CITY, PPOBJ_WORLD, Data.CityID, OLW_LOADDEFONOPEN, 0);
		SetupPPObjCombo(this, CTLSEL_SLLTOFLT_CATEGORY, PPOBJ_PRSNCATEGORY, Data.CategoryID, OLW_LOADDEFONOPEN|OLW_CANINSERT, 0);
		AddClusterAssoc(CTL_SLLTOFLT_FLAGS, 0, DebtTrnovrFilt::fDebtOnly);
		AddClusterAssoc(CTL_SLLTOFLT_FLAGS, 1, DebtTrnovrFilt::fNoForwardPaym);
		AddClusterAssoc(CTL_SLLTOFLT_FLAGS, 2, DebtTrnovrFilt::fByCost);
		AddClusterAssoc(CTL_SLLTOFLT_FLAGS, 3, DebtTrnovrFilt::fExtended);
		AddClusterAssoc(CTL_SLLTOFLT_FLAGS, 4, DebtTrnovrFilt::fInclZeroDebt);
		AddClusterAssoc(CTL_SLLTOFLT_FLAGS, 5, DebtTrnovrFilt::fDeliveryAddr);
		AddClusterAssoc(CTL_SLLTOFLT_FLAGS, 6, DebtTrnovrFilt::fSkipPassive);
		AddClusterAssoc(CTL_SLLTOFLT_FLAGS, 7, DebtTrnovrFilt::fShowExpiryDebt);
		SetClusterData(CTL_SLLTOFLT_FLAGS, Data.Flags);
		AddClusterAssocDef(CTL_SLLTOFLT_ORDER, 0, PPViewDebtTrnovr::OrdByArticleName);
		AddClusterAssoc(CTL_SLLTOFLT_ORDER, 1, PPViewDebtTrnovr::OrdByDebit);
		AddClusterAssoc(CTL_SLLTOFLT_ORDER, 2, PPViewDebtTrnovr::OrdByDebt);
		AddClusterAssoc(CTL_SLLTOFLT_ORDER, 3, PPViewDebtTrnovr::OrdByStop);
		SetClusterData(CTL_SLLTOFLT_ORDER, Data.InitOrder);
		//enableCommand(cmEditCycles, Data.Flags & DebtTrnovrFilt::fUseCrosstab);
		SetupSubstBillCombo(this, CTLSEL_SLLTOFLT_SGB, Data.Sgb);
		SetupSubst();
		return ok;
	}
	int    getDTS(DebtTrnovrFilt * pData)
	{
		int    ok = 1, sel = 0;
		ushort v = 0;
		PPID   cli_id = 0;
		LocationCtrlGroup::Rec l_rec;
		THROW(GetPeriodInput(this, sel = CTL_SLLTOFLT_PERIOD,     &Data.Period));
		THROW(GetPeriodInput(this, sel = CTL_SLLTOFLT_PAYMPERIOD, &Data.PaymPeriod));
		THROW(GetPeriodInput(this, sel = CTL_SLLTOFLT_EXPIRYPRD,  &Data.ExpiryPeriod));
		/* if(AdjustPeriodToRights(&pFilt->Period)) { */
		getCtrlData(sel = CTLSEL_SLLTOFLT_ACCSHEET, &Data.AccSheetID);
		THROW_PP(Data.AccSheetID, PPERR_ACCSHEETNEEDED);
		getCtrlData(CTLSEL_SLLTOFLT_OP, &Data.OpID);
		getCtrlData(CTLSEL_SLLTOFLT_CLIENT, &cli_id);
		Data.CliIDList.setSingleNZ(cli_id);
		getGroupData(ctlgroupLoc, &l_rec);
		if(l_rec.LocList.GetCount())
			Data.LocIDList = l_rec.LocList.Get();
		else
			Data.LocIDList.freeAll();
		getCtrlData(CTLSEL_SLLTOFLT_CITY, &Data.CityID);
		getCtrlData(CTLSEL_SLLTOFLT_CATEGORY, &Data.CategoryID);
		sel = 0;
		getCtrlData(CTLSEL_SLLTOFLT_CUR, &Data.CurID);
		v = getCtrlUInt16(CTL_SLLTOFLT_ALLCUR);
		SETFLAG(Data.Flags, DebtTrnovrFilt::fAllCurrencies, v & 1);
		if(v & 1)
			Data.CurID = -1;
		GetClusterData(CTL_SLLTOFLT_FLAGS, &Data.Flags);
		SETFLAG(Data.Flags, DebtTrnovrFilt::fLabelOnly, getWL());
		GetClusterData(CTL_SLLTOFLT_ORDER, &Data.InitOrder);
		//
		Data.Sgb.S = (SubstGrpBill::_S)getCtrlLong(CTLSEL_SLLTOFLT_SGB);
		if(Data.Sgb.S == SubstGrpBill::sgbDate)
			Data.Sgb.S2.Sgd = (SubstGrpDate)getCtrlLong(CTLSEL_SLLTOFLT_SGEXT);
		else if(oneof4(Data.Sgb.S, SubstGrpBill::sgbObject, SubstGrpBill::sgbObject2, SubstGrpBill::sgbAgent, SubstGrpBill::sgbPayer))
			Data.Sgb.S2.Sgp = (SubstGrpPerson)getCtrlLong(CTLSEL_SLLTOFLT_SGEXT);
		else
			Data.Sgb.S2.Sgp = sgpNone;
		//
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		WLDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_SLLTOFLT_SGB)) {
			Data.Sgb.S = (SubstGrpBill::_S)getCtrlLong(CTLSEL_SLLTOFLT_SGB);
			SetupSubst();
		}
		else if(event.isCbSelected(CTLSEL_SLLTOFLT_LOC))
			Data.LocIDList.setSingleNZ(getCtrlLong(CTLSEL_SLLTOFLT_LOC));
		else if(event.isCbSelected(CTLSEL_SLLTOFLT_ACCSHEET)) {
			getCtrlData(CTLSEL_SLLTOFLT_ACCSHEET, &Data.AccSheetID);
			SetupArCombo(this, CTLSEL_SLLTOFLT_CLIENT, 0L, OLW_LOADDEFONOPEN, Data.AccSheetID, sacfDisableIfZeroSheet);
			SetupOp();
		}
		else if(event.isCmd(cmaMore))
			editMore();
		else if(event.isCmd(cmEditCycles)) {
			uint   sel = 0;
			if(!GetPeriodInput(this, sel = CTL_SLLTOFLT_PERIOD, &Data.Period) ||
				!GetPeriodInput(this, sel = CTL_SLLTOFLT_PAYMPERIOD, &Data.PaymPeriod) ||
				!GetPeriodInput(this, sel = CTL_SLLTOFLT_EXPIRYPRD,  &Data.ExpiryPeriod)) {
				PPErrorByDialog(this, sel);
			}
			else if(editCycles() > 0) {
				SetPeriodInput(this, CTL_SLLTOFLT_PERIOD,     Data.Period);
				SetPeriodInput(this, CTL_SLLTOFLT_EXPIRYPRD,  Data.ExpiryPeriod);
				SetPeriodInput(this, CTL_SLLTOFLT_PAYMPERIOD, Data.PaymPeriod);
				if(Data.CycleKind == DebtTrnovrFilt::ckPayments)
					if(!(Data.Flags & DebtTrnovrFilt::fInclZeroDebt)) {
						Data.Flags |= DebtTrnovrFilt::fInclZeroDebt;
						SetClusterData(CTL_SLLTOFLT_FLAGS, Data.Flags);
					}
			}
		}
		else if(event.isCmd(cmEditExt))
			editExt();
		else
			return;
		clearEvent(event);
	}
	void SetupSubst()
	{
		int    dsbl_sec_subst = 1;
		const  uint sec_ctl_id = CTLSEL_SLLTOFLT_SGEXT;
		SString label_buf;
		if(!!Data.Sgb) {
			if(Data.Sgb.S == SubstGrpBill::sgbDate) {
				PPLoadString("date", label_buf);
				SetupSubstDateCombo(this, sec_ctl_id, (long)Data.Sgb.S2.Sgd);
				dsbl_sec_subst = 0;
			}
			else if(oneof4(Data.Sgb.S, SubstGrpBill::sgbObject, SubstGrpBill::sgbObject2, SubstGrpBill::sgbAgent, SubstGrpBill::sgbPayer)) {
				PPLoadString("person", label_buf);
				SetupSubstPersonCombo(this, sec_ctl_id, Data.Sgb.S2.Sgp);
				dsbl_sec_subst = 0;
			}
		}
		setLabelText(CTL_SLLTOFLT_SGEXT, label_buf);
		disableCtrl(sec_ctl_id, dsbl_sec_subst);
	}
	int    editCycles();
	int    editExt();
	int    editMore();

	DebtTrnovrFilt Data;
};
//
//
//
int DebtTrnovrFiltDialog::editMore()
{
	class DebtTrnovrMoreFiltDialog : public PPListDialog {
	public:
		DebtTrnovrMoreFiltDialog() : PPListDialog(DLG_DEBTEXF, CTL_DEBTEXF_DBTDIM)
		{
		}
		int    setDTS(const DebtTrnovrFilt * pData)
		{
			if(!RVALUEPTR(Data, pData))
				Data.Init(1, 0);
			SetupArCombo(this, CTLSEL_DEBTEXF_PAYER, pData->PayerID, OLW_LOADDEFONOPEN|OLW_CANINSERT, GetSellAccSheet(), sacfDisableIfZeroSheet);
			SetupArCombo(this, CTLSEL_DEBTEXF_AGENT, pData->AgentID, OLW_LOADDEFONOPEN|OLW_CANINSERT, GetAgentAccSheet(), sacfDisableIfZeroSheet);
			SetupPPObjCombo(this, CTLSEL_DEBTEXF_EOBJSHEET, PPOBJ_ACCSHEET, Data.AccSheet2ID, 0, 0);
			SetupArCombo(this, CTLSEL_DEBTEXF_EXTOBJ, Data.Article2ID, OLW_LOADDEFONOPEN, Data.AccSheet2ID, sacfDisableIfZeroSheet);
			updateList(-1);
			return 1;
		}
		int    getDTS(DebtTrnovrFilt * pData)
		{
			getCtrlData(CTLSEL_DEBTEXF_PAYER, &Data.PayerID);
			getCtrlData(CTLSEL_DEBTEXF_AGENT, &Data.AgentID);
			getCtrlData(CTLSEL_DEBTEXF_EOBJSHEET, &Data.AccSheet2ID);
			getCtrlData(CTLSEL_DEBTEXF_EXTOBJ,    &Data.Article2ID);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			PPListDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_DEBTEXF_EOBJSHEET)) {
				getCtrlData(CTLSEL_DEBTEXF_EOBJSHEET, &Data.AccSheet2ID);
				SetupArCombo(this, CTLSEL_DEBTEXF_EXTOBJ, Data.Article2ID, OLW_LOADDEFONOPEN, Data.AccSheet2ID, sacfDisableIfZeroSheet);
				clearEvent(event);
			}
		}
		virtual int setupList()
		{
			int    ok = 1;
			PPDebtDim dd_rec;
			for(uint i = 0; i < Data.DebtDimList.GetCount(); i++) {
				const  PPID dd_id = Data.DebtDimList.Get(i);
				if(dd_id && DdObj.Search(dd_id, &dd_rec) > 0) {
					THROW(addStringToList(dd_id, dd_rec.Name));
				}
			}
			CATCHZOK
			return ok;
		}
		virtual int addItem(long * pPos, long * pID)
		{
			int    ok = -1;
			PPDebtDim dd_rec;
			PPID   sel_id = PPObjDebtDim::Select();
			if(sel_id > 0 && DdObj.Search(sel_id, &dd_rec) > 0) {
				if(Data.DebtDimList.Search(sel_id, 0, 0)) {
					PPError(PPERR_DUPDEBTDIMINLIST, dd_rec.Name);
				}
				else {
					if(Data.DebtDimList.Add(sel_id)) {
						ASSIGN_PTR(pPos, Data.DebtDimList.GetCount() - 1);
						ASSIGN_PTR(pID, sel_id);
						ok = 1;
					}
				}
			}
			return ok;
		}
		virtual int delItem(long pos, long id)
		{
			return id ? Data.DebtDimList.Remove(id, 0) : -1;
		}
		PPObjDebtDim DdObj;
		DebtTrnovrFilt Data;
	};
	DIALOG_PROC_BODY(DebtTrnovrMoreFiltDialog, &Data);
}
//
//
//
class DebtTrnovrCycleDialog : public EmbedDialog {
public:
	DebtTrnovrCycleDialog() : EmbedDialog(DLG_DEBTTOC)
	{
		SetupCalPeriod(CTLCAL_SLLTOFLT_PERIOD,     CTL_SLLTOFLT_PERIOD);
		SetupCalPeriod(CTLCAL_SLLTOFLT_PAYMPERIOD, CTL_SLLTOFLT_PAYMPERIOD);
		SetupCalPeriod(CTLCAL_SLLTOFLT_EXPIRYPRD,  CTL_SLLTOFLT_EXPIRYPRD);
	}
	int    setDTS(const DebtTrnovrFilt *);
	int    getDTS(DebtTrnovrFilt *);
private:
	class DelayDialog : public TDialog {
	public:
		DelayDialog() : TDialog(DLG_DEBTTOC_DELAY)
		{
		}
		virtual void setData(void * pData)
		{
			PPCycleFilt data = *static_cast<PPCycleFilt *>(pData);
			setCtrlData(CTL_DEBTTOC_CYCLE, &data.Cycle);
			setCtrlData(CTL_DEBTTOC_NUMCYCLES, &data.NumCycles);
		}
		virtual void getData(void * pData)
		{
			PPCycleFilt data;
			getCtrlData(CTL_DEBTTOC_CYCLE, &data.Cycle);
			getCtrlData(CTL_DEBTTOC_NUMCYCLES, &data.NumCycles);
			if(pData)
				*static_cast<PPCycleFilt *>(pData) = data;
		}
	};
	class CycleDialog : public TDialog {
	public:
		CycleDialog(TDialog * pOwner, uint periodCtlId) : TDialog(DLG_DEBTTOC_CYCLE)
		{
			CycleCtrlGroup * p_grp = new CycleCtrlGroup(CTLSEL_DEBTTOC_CYCLE, CTL_DEBTTOC_NUMCYCLES, pOwner, periodCtlId);
			addGroup(1, p_grp);
		}
		virtual void setData(void * pData)
		{
			setGroupData(1, pData);
		}
		virtual void getData(void * pData)
		{
			getGroupData(1, pData);
		}
	};
	DECL_HANDLE_EVENT;
	int    embedChild();
	DebtTrnovrFilt Data;
};

IMPL_HANDLE_EVENT(DebtTrnovrCycleDialog)
{
	EmbedDialog::handleEvent(event);
	if(event.isClusterClk(CTL_DEBTTOC_KIND)) {
		const long   prev_kind = Data.CycleKind;
		GetClusterData(CTL_DEBTTOC_KIND, &Data.CycleKind);
		if(Data.CycleKind != prev_kind) {
			Data.Cf.Cycle = 0;
			Data.Cf.NumCycles = 1;
			if(embedChild() > 0 && P_ChildDlg)
				P_ChildDlg->TransmitData(+1, &Data.Cf);
		}
		clearEvent(event);
	}
}

int DebtTrnovrCycleDialog::embedChild()
{
	int    ok = 1;
	GetClusterData(CTL_DEBTTOC_KIND, &Data.CycleKind);
	if(oneof2(Data.CycleKind, DebtTrnovrFilt::ckExpiry, DebtTrnovrFilt::ckDelay)) {
		DelayDialog * dlg = new DelayDialog;
		if(CheckDialogPtrErr(&dlg)) {
			Embed(dlg);
			dlg->setData(&Data.Cf);
		}
		else
			ok = 0;
	}
	else if(Data.CycleKind == DebtTrnovrFilt::ckShipments || Data.CycleKind == DebtTrnovrFilt::ckPayments) {
		const  uint ctl_id = (Data.CycleKind == DebtTrnovrFilt::ckShipments) ? CTL_SLLTOFLT_PERIOD : CTL_SLLTOFLT_PAYMPERIOD;
		CycleDialog * dlg = new CycleDialog(this, ctl_id);
		if(CheckDialogPtrErr(&dlg)) {
			Embed(dlg);
			dlg->setData(&Data.Cf);
		}
		else
			ok = 0;
	}
	else {
		Embed(0);
		ok = -1;
	}
	if(ok > 0)
		setChildPos(CTL_DEBTTOC_KIND);
	return ok;
}

int DebtTrnovrCycleDialog::setDTS(const DebtTrnovrFilt * pData)
{
	int    ok = 1;
	Data = *pData;
	SetPeriodInput(this, CTL_SLLTOFLT_PERIOD,     Data.Period);
	SetPeriodInput(this, CTL_SLLTOFLT_EXPIRYPRD,  Data.ExpiryPeriod);
	SetPeriodInput(this, CTL_SLLTOFLT_PAYMPERIOD, Data.PaymPeriod);
	AddClusterAssoc(CTL_DEBTTOC_KIND, 0, DebtTrnovrFilt::ckNone);
	AddClusterAssoc(CTL_DEBTTOC_KIND, 1, DebtTrnovrFilt::ckExpiry);
	AddClusterAssoc(CTL_DEBTTOC_KIND, 2, DebtTrnovrFilt::ckDelay);
	AddClusterAssoc(CTL_DEBTTOC_KIND, 3, DebtTrnovrFilt::ckShipments);
	AddClusterAssoc(CTL_DEBTTOC_KIND, 4, DebtTrnovrFilt::ckPayments);
	SetClusterData(CTL_DEBTTOC_KIND, Data.CycleKind);
	if(embedChild() > 0 && P_ChildDlg)
		P_ChildDlg->TransmitData(+1, &Data.Cf);
	return ok;
}

int DebtTrnovrCycleDialog::getDTS(DebtTrnovrFilt * pData)
{
	int    ok = 1;
	uint   sel = 0;
	THROW(GetPeriodInput(this, sel = CTL_SLLTOFLT_PERIOD,     &Data.Period));
	THROW(GetPeriodInput(this, sel = CTL_SLLTOFLT_PAYMPERIOD, &Data.PaymPeriod));
	THROW(GetPeriodInput(this, sel = CTL_SLLTOFLT_EXPIRYPRD,  &Data.ExpiryPeriod));
	GetClusterData(CTL_DEBTTOC_KIND, &Data.CycleKind);
	CALLPTRMEMB(P_ChildDlg, TransmitData(-1, &Data.Cf));
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

int DebtTrnovrFiltDialog::editCycles() { DIALOG_PROC_BODY(DebtTrnovrCycleDialog, &Data); }
//
//
//
int DebtTrnovrFiltDialog::editExt()
{
	class DebtTrnovrExtFiltDialog : public TDialog {
		DECL_DIALOG_DATA(DebtTrnovrFilt);
	public:
		DebtTrnovrExtFiltDialog() : TDialog(DLG_DEBTTEXT)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			AddClusterAssocDef(CTL_DEBTTEXT_KIND, 0, DebtTrnovrFilt::ekNone);
			AddClusterAssoc(CTL_DEBTTEXT_KIND, 1, DebtTrnovrFilt::ekExpiryPart);
			AddClusterAssoc(CTL_DEBTTEXT_KIND, 2, DebtTrnovrFilt::ekTurnover);
			SetClusterData(CTL_DEBTTEXT_KIND, Data.ExtKind);
			setCtrlLong(CTL_DEBTTEXT_EXPIRYTERM, Data.ExtExpiryTerm);
			setCtrlReal(CTL_DEBTTEXT_MINDEBTPART, Data.ExtExpiryMinPart);
			showCtrl(CTL_DEBTTEXT_EXPIRYTERM,  Data.ExtKind == DebtTrnovrFilt::ekExpiryPart);
			showCtrl(CTL_DEBTTEXT_MINDEBTPART, Data.ExtKind == DebtTrnovrFilt::ekExpiryPart);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			GetClusterData(CTL_DEBTTEXT_KIND, &Data.ExtKind);
			if(Data.ExtKind == DebtTrnovrFilt::ekExpiryPart) {
				getCtrlData(CTL_DEBTTEXT_EXPIRYTERM,  &Data.ExtExpiryTerm);
				getCtrlData(CTL_DEBTTEXT_MINDEBTPART, &Data.ExtExpiryMinPart);
			}
			else {
				Data.ExtExpiryTerm = 0;
				Data.ExtExpiryMinPart = 0;
			}
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_DEBTTEXT_KIND)) {
				GetClusterData(CTL_DEBTTEXT_KIND, &Data.ExtKind);
				showCtrl(CTL_DEBTTEXT_EXPIRYTERM,  Data.ExtKind == DebtTrnovrFilt::ekExpiryPart);
				showCtrl(CTL_DEBTTEXT_MINDEBTPART, Data.ExtKind == DebtTrnovrFilt::ekExpiryPart);
				clearEvent(event);
			}
		}
	};
	DIALOG_PROC_BODY(DebtTrnovrExtFiltDialog, &Data);
}
//
//
//
int PPViewDebtTrnovr::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	DebtTrnovrFilt * p_filt = static_cast<DebtTrnovrFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(DebtTrnovrFiltDialog, p_filt);
}

int PPViewDebtTrnovr::GetTotal(DebtTrnovrTotal * pTotal)
{
	ASSIGN_PTR(pTotal, Total);
	return 1;
}

void PPViewDebtTrnovr::ViewTotal()
{
	class ExtDebtTrnovrTotalDialog : public TDialog {
	public:
		ExtDebtTrnovrTotalDialog(int kind, const DebtTrnovrTotal & rTotal) : TDialog((kind == 1) ? DLG_DEBTTOTAL01 : DLG_EXTTDEBT), Kind(kind), R_Total(rTotal)
		{
			SetupCurrencyCombo(this, CTLSEL_EXTTDEBT_CUR, 0L, 0, 1, 0);
			setCtrlLong(CTL_EXTTDEBT_COUNT, R_Total.Count);
			UpdatePage(0);
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_EXTTDEBT_CUR)) {
				UpdatePage(getCtrlLong(CTLSEL_EXTTDEBT_CUR));
				clearEvent(event);
			}
		}
		void   UpdatePage(PPID curID)
		{
			if(Kind == 1) {
				const double debt = R_Total.Debit.Get(0L, curID);
				const double exp_debt = R_Total.Debt.Get(0L, curID);
				const double part = fdivnz(exp_debt, debt) * 100.0;
				setCtrlReal(CTL_EXTTDEBT_DEBT,    debt);
				setCtrlReal(CTL_EXTTDEBT_EXPDEBT, exp_debt);
				setCtrlReal(CTL_EXTTDEBT_EXPDEBTPART, part);
			}
			else {
				setCtrlReal(CTL_EXTTDEBT_SELL,   R_Total.Debit.Get(0L, curID));
				setCtrlReal(CTL_EXTTDEBT_PAYM,   R_Total.Credit.Get(0L, curID));
				setCtrlReal(CTL_EXTTDEBT_DEBT,   R_Total.Debt.Get(0L, curID));
				setCtrlReal(CTL_EXTTDEBT_RPAYM,  R_Total.RPaym.Get(0L, curID));
				setCtrlReal(CTL_EXTTDEBT_RECKON, R_Total.Reckon.Get(0L, curID));
				setCtrlReal(CTL_EXTTDEBT_RDEBT,  R_Total.RDebt.Get(0L, curID));
				setCtrlReal(CTL_EXTTDEBT_TDEBT,  R_Total.TDebt.Get(0L, curID));
			}
		}
		const  DebtTrnovrTotal & R_Total;
		int    Kind;
	};
	const int  kind = (Filt.ExtKind == DebtTrnovrFilt::ekExpiryPart) ? 1 : 0;
	ExtDebtTrnovrTotalDialog * dlg = new ExtDebtTrnovrTotalDialog(kind, Total);
	if(CheckDialogPtrErr(&dlg))
		ExecViewAndDestroy(dlg);
}

int PPViewDebtTrnovr::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	return Detail(static_cast<const BrwHdr *>(pHdr), 0);
}

int PPViewDebtTrnovr::Detail(const BrwHdr * pHdr, int mode)
{
	int    ok = -1;
	if(pHdr /*&& pHdr->ArID*/) {
		PPIDArray bill_list;
		if(0/*Filt.Sgb.S2.Sgp*/) { // Пока непонятно как быть с детализацией подстановки.
			DebtTrnovrViewItem item;
			if(GetItem(pHdr->ArID, pHdr->CurID, pHdr->TabID, &item) > 0) {
				DebtTrnovrFilt temp_filt = Filt;
				temp_filt.Sgb.S2.Sgp = sgpNone;
				Bsp.AsscList.GetListByKey(pHdr->ArID, bill_list);
				if(mode == dmReckon)
					temp_filt.RcknBillList.Set(&bill_list);
				else {
					temp_filt.BillList.Set(&bill_list);
					if(temp_filt.Flags & DebtTrnovrFilt::fExtended) {
						bill_list.clear();
						RcknBsp.AsscList.GetListByKey(pHdr->ArID, bill_list);
						temp_filt.RcknBillList.Set(&bill_list);
					}
				}
				bill_list.freeAll();
				PPView::Execute(PPVIEW_DEBTTRNOVR, &temp_filt, PPView::exefModeless, 0);
			}
		}
		else {
			BillFilt flt;
			flt.Flags |= (BillFilt::fShowDebt | BillFilt::fPaymNeeded);
			if(oneof5(mode, dmDebtCard, dmDebt, dmReckon, dmDebtOnline, dmReckonOnline)) {
				int    done = 0;
				if(Filt.Sgb.S == SubstGrpBill::sgbObject) {
					if(mode == dmDebtCard) {
						flt.ObjectID = pHdr->ArID;
						flt.AgentID  = Filt.AgentID;
						flt.Period = Filt.Period;
						flt.CurID  = pHdr->CurID;
						if(Filt.Flags & DebtTrnovrFilt::fLabelOnly)
							flt.Flags |= BillFilt::fLabelOnly;
						flt.Flags |= BillFilt::fDebtsWithPayments;
						flt.Flags &= ~(BillFilt::fDebtOnly | BillFilt::fPaymNeeded);
					}
					else if(!Filt.Sgb.S2.Sgp) {
						int    _do = 0;
						PayableBillList pbl;
						if(mode == dmDebtOnline) {
							THROW(GetPayableBillList(pHdr->ArID, pHdr->CurID, &pbl));
							_do = 1;
						}
						else if(mode == dmReckonOnline) {
							THROW(GetReceivableBillList(pHdr->ArID, &pbl));
							_do = 1;
						}
						if(_do) {
							for(uint i = 0; i < pbl.getCount(); i++) {
								THROW_SL(bill_list.add(pbl.at(i).ID));
							}
							flt.List.Set(&bill_list);
							flt.LocList.Set(&Filt.LocIDList);
							flt.PaymPeriod = Filt.PaymPeriod;
							flt.Flags |= BillFilt::fBillListOnly;
							ViewGoodsBills(&flt, true/*modeless*/);
							done = 1;
						}
					}
				}
				if(!done) {
					if(mode == dmDebtOnline)
						mode = dmDebt;
					else if(mode == dmReckonOnline)
						mode = dmReckon;
					if(oneof2(mode, dmDebt, dmReckon)) {
						if(mode == dmReckon) {
							if(Filt.Flags & DebtTrnovrFilt::fExtended) {
								RcknBsp.AsscList.GetListByKey(pHdr->ArID, bill_list);
							}
							else if(Filt.Sgb.S == SubstGrpBill::sgbObject && Filt.Sgb.S2.Sgp == sgpNone && pHdr->ArID) {
								PayableBillList pbill_list;
								THROW(GetReceivableBillList(pHdr->ArID, &pbill_list));
								pbill_list.GetIdList(bill_list);
							}
						}
						else {
							Bsp.AsscList.GetListByKey(pHdr->ArID, bill_list);
			   			}
						flt.List.Set(&bill_list);
						flt.LocList.Set(&Filt.LocIDList);
						flt.PaymPeriod = Filt.PaymPeriod;
						flt.Flags |= BillFilt::fBillListOnly;
					}
					if(ok > -2) {
						ViewGoodsBills(&flt, true/*modeless*/);
						ok = -1;
					}
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}
//
// what == 0 - show article
// what == 1 - show person by article_id
// what == 2 - show person by person_id
//
int PPViewDebtTrnovr::ViewArticleInfo(const BrwHdr * pHdr, int what)
{
	int    ok = -1;
	if(pHdr && pHdr->ArID) {
		PPID   person_id = 0;
		if(what == 2) {
			person_id = pHdr->ArID;
			if(PsnObj.Edit(&person_id, 0) == cmOK)
				ok = 1;
		}
		else if(what == 1 && (person_id = ObjectToPerson(pHdr->ArID)) > 0) {
			if(PsnObj.Edit(&person_id, 0) == cmOK)
				ok = 1;
		}
		else {
			PPID ar_id = pHdr->ArID;
			if(ArObj.Edit(&ar_id, 0) == cmOK)
				ok = 1;
		}
		if(ok > 0) {
			ArticleTbl::Rec ar_rec;
			if(ArObj.Search(pHdr->ArID, &ar_rec) > 0) {
				TempSellTrnovrTbl * t = P_TempTbl;
				THROW_DB(updateFor(t, 1, (t->ID == pHdr->ArID),
					set(t->Name, dbconst(ar_rec.Name)).
					set(t->NotStop, dbconst((long)BIN(!(ar_rec.Flags & ARTRF_STOPBILL))))));
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

void PPViewDebtTrnovr::GetTabTitle(long tabID, SString & rBuf) const
{
	rBuf.Z();
	if(oneof2(Filt.CycleKind, DebtTrnovrFilt::ckExpiry, DebtTrnovrFilt::ckDelay)) {
		const long n = DaySieve.getCountI();
		if(tabID && tabID <= n) {
			if(tabID == n && DaySieve.at(n-1) == MAXLONG) {
				if(n > 1)
					rBuf.Cat(DaySieve.at(tabID-2)+1);
				rBuf.CatCharN('.', 2);
			}
			else if(tabID == 1)
				rBuf.CatCharN('.', 2).Cat(DaySieve.at(tabID-1));
			else
				rBuf.Cat(DaySieve.at(tabID-2)+1).CatCharN('.', 2).Cat(DaySieve.at(tabID-1));
		}
	}
	else if(oneof2(Filt.CycleKind, DebtTrnovrFilt::ckShipments, DebtTrnovrFilt::ckPayments)) {
		DateRange period;
		if(CycleSieve.getPeriod(static_cast<uint>(tabID-1), &period)) {
			char temp_buf[256];
			CycleSieve.formatCycle(period.low, temp_buf, sizeof(temp_buf));
			rBuf = temp_buf;
		}
	}
}
//
//
//
int PPViewDebtTrnovr::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	BrwHdr hdr;
	if(pHdr) {
		if(P_Ct) {
			const int32 * p = static_cast<const int32 *>(pHdr);
			hdr.CurID = p[2];
			hdr.TabID = 0;
			hdr.ArID = p[1];
		}
		else
			hdr = *static_cast<const BrwHdr *>(pHdr);
	}
	else
		MEMSZERO(hdr);
	int    ok = PPView::ProcessCommand(ppvCmd, &hdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_VIEWBILLS:   ok = -1; Detail(&hdr, dmDebtOnline); break;
			case PPVCMD_EDITARTICLE:
				if(Filt.Sgb.S2.Sgp == sgpNone)
					ok = hdr.ArID ? ViewArticleInfo(&hdr, 0) : -1;
				else
					ok = -1;
				break;
			case PPVCMD_EDITPERSON:
				if(hdr.ArID) {
					if(Filt.Sgb.S2.Sgp == sgpNone)
						ok = ViewArticleInfo(&hdr, 1);
					else if(Filt.Sgb.S2.Sgp > sgpFirstRelation)
						ok = ViewArticleInfo(&hdr, 2);
				}
				else
					ok = -1;
				break;
			case PPVCMD_DEBTCARD:    ok = -1; Detail(&hdr, dmDebtCard); break;
			case PPVCMD_RECKONBILLS: ok = -1; Detail(&hdr, dmReckon); break;
			case PPVCMD_MOUSEHOVER:
				{
					long   h = 0;
					if(pBrw->ItemByMousePos(&h, 0) && P_Ct == 0 && h == 0) {
						int r = 0;
						SString buf;
						PPELinkArray phones_ary;
						PersonCore::GetELinks(ObjectToPerson(hdr.ArID, 0), phones_ary);
						for(uint i = 0; i < phones_ary.getCount(); i++) {
							CatObjectName(PPOBJ_ELINKKIND, phones_ary.at(i).KindID, buf);
							buf.CatDiv(':', 2).Cat(phones_ary.at(i).Addr).CR();
							r = 1;
						}
						if(r > 0)
							PPTooltipMessage(buf, 0, pBrw->H(), 10000, 0, SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|SMessageWindow::fTextAlignLeft|
								SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow);
					}
				}
				break;
		}
	}
	return ok;
}

void PPViewDebtTrnovr::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw && P_TempTbl && !P_Ct) {
		DBQBrowserDef * p_def = static_cast<DBQBrowserDef *>(pBrw->getDef());
		const DBQuery * p_q = p_def ? p_def->getQuery() : 0;
		if(p_q) {
			if(!(Filt.Flags & DebtTrnovrFilt::fShowExpiryDebt)) {
				const uint target_fld_no = (Filt.Flags & DebtTrnovrFilt::fExtended) ? 16 : 12;
				for(uint i = 0; i < p_def->getCount(); i++) {
					const BroColumn & r_col = p_def->at(i);
					if(r_col.OrgOffs == target_fld_no) {
						pBrw->removeColumn(i);
						break;
					}
				}
			}
			uint  fld_no = 0;
			if(Filt.Sgb.S == SubstGrpBill::sgbDlvrLoc) {
                fld_no = (Filt.Flags & DebtTrnovrFilt::fExtended) ? 15 : 11;
                pBrw->InsColumn(0, "@person", fld_no, 0, 0, 0);
			}
			if(Filt.Flags & DebtTrnovrFilt::fAllCurrencies) {
				fld_no = (Filt.Flags & DebtTrnovrFilt::fExtended) ? 12 : 8;
				pBrw->InsColumn(2, "@currency", fld_no, 0, 0, 0);
			}
			fld_no = (Filt.Flags & DebtTrnovrFilt::fExtended) ? 13 : 9;
			fld_no = (Filt.Flags & DebtTrnovrFilt::fExtended) ? 14 : 10;
			pBrw->InsColumn(-1, "@stop", fld_no, 0, MKSFMTD(0, 0, ALIGN_CENTER), 0);
			if(Filt.CycleKind) {
				fld_no = 2; //(Filt.Flags & DebtTrnovrFilt::fExtended) ? 13 : 9;
				pBrw->InsColumn(2, "@cycle", fld_no, 0, 0, 0);
			}
		}
	}
}

DBQuery * PPViewDebtTrnovr::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	static DbqStringSubst stop_subst(2);  // @global @threadsafe
	uint   brw_id = 0;
	TempSellTrnovrTbl * tbl = 0;
	DBQuery * q = 0;
	DBE  * p_var = 0;
	DBE    dbe_cur;
	DBE    dbe_locowner;
	SString temp_buf;
	if(P_Ct) {
		if(Filt.CycleKind == DebtTrnovrFilt::ckPayments)
			brw_id = BROWSER_SELLTRNOVR_CT_PAYM;
		else
			brw_id = BROWSER_SELLTRNOVR_CT;
		if(pSubTitle)
			PPGetSubStr(PPTXT_DEBTTO_CYCLEKINDTITLE, Filt.CycleKind, *pSubTitle);
		q = PPView::CrosstabDbQueryStub;
	}
	else {
		DBE  * dbe_stop = 0;
		DBFieldList fld_list;
		tbl = new TempSellTrnovrTbl(P_TempTbl->GetName());
		THROW(CheckTblPtr(tbl));
		dbe_stop = & enumtoa(tbl->NotStop, 2, stop_subst.Get(PPTXT_DEBTTRNOVR_NOTSTOP));
		if(Filt.Flags & DebtTrnovrFilt::fExtended)
			brw_id = BROWSER_SELLTRNOVR_E;
		else
			brw_id = BROWSER_SELLTRNOVR;
		fld_list.Add(tbl->ID);      // #0
		fld_list.Add(tbl->CurID);   // #1
		fld_list.Add(tbl->TabID);   // #2
		fld_list.Add(tbl->Ar);      // #3
		fld_list.Add(tbl->Name);    // #4
		fld_list.Add(tbl->Sell);    // #5
		fld_list.Add(tbl->Payment); // #6
		fld_list.Add(tbl->Debt);    // #7
		if(Filt.Flags & DebtTrnovrFilt::fExtended) {
			fld_list.Add(tbl->RPaym);  // #8
			fld_list.Add(tbl->Reckon); // #9
			fld_list.Add(tbl->RDebt);  // #10
			fld_list.Add(tbl->TDebt);  // #11
		}
		q = & select(fld_list).from(tbl, 0L);
		{
			PPDbqFuncPool::InitObjNameFunc(dbe_cur, PPDbqFuncPool::IdObjSymbCurrency, tbl->CurID);
			q->addField(dbe_cur); // #8 : #12
		}
		if(Filt.ExtKind == DebtTrnovrFilt::ekExpiryPart) {
			p_var = &(100L * tbl->Debt / tbl->Sell);
			q->addField(*p_var);       // #9 : #13
			brw_id = BROWSER_SELLTRNOVR_EXT_EXPPART;
		}
		else if(Filt.ExtKind == DebtTrnovrFilt::ekTurnover) {
			p_var = &(tbl->Sell / tbl->Debt);
			q->addField(*p_var);       // #9 : #13
			brw_id = BROWSER_SELLTRNOVR_EXT_TRUNOVER;
		}
		else {
			q->addField(tbl->ID);      // #9 : #13 @stub
		}
		q->addField(*dbe_stop);        // #10 : #14
		if(Filt.Sgb.S == SubstGrpBill::sgbDlvrLoc) {
			PPDbqFuncPool::InitObjNameFunc(dbe_locowner, PPDbqFuncPool::IdLocOwnerName, tbl->ID);
			q->addField(dbe_locowner); // #11 : #15
		}
		else {
			q->addField(tbl->ID);      // #11 : #15 @stub
		}
		q->addField(tbl->ExpiryDebt);  // #12 : #16
		if(Filt.InitOrder == OrdByDebit)
			q->orderBy(tbl->CurID, tbl->Sell, 0L);
		else if(Filt.InitOrder == OrdByDebt)
			q->orderBy(tbl->CurID, tbl->Debt, 0L);
		else if(Filt.InitOrder == OrdByStop)
			q->orderBy(tbl->NotStop, tbl->Name, tbl->CurID, 0L);
		else
			q->orderBy(tbl->Name, tbl->CurID, 0L);
		THROW(CheckQueryPtr(q));
		if(pSubTitle) {
			GetObjectName(PPOBJ_ACCSHEET, Filt.AccSheetID, *pSubTitle);
			GetCurSymbText((Filt.Flags & DebtTrnovrFilt::fAllCurrencies) ? -1L : Filt.CurID, temp_buf);
			if(temp_buf.NotEmpty())
				pSubTitle->Space().Cat(temp_buf);
		}
	}
	CATCH
		if(q) {
			ZDELETE(q);
		}
		else
			delete tbl;
	ENDCATCH
	delete p_var;
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

static int SelectPrintingDebtTrnovrSheet(int * pWhat, LDATE * pExpiry, ushort *pOrder)
{
	class SelWhatPrnDialog : public TDialog {
	public:
		SelWhatPrnDialog() : TDialog(DLG_PRNSELLTO)
		{
			SetupCalDate(CTLCAL_PRNSELLTO_DTIN, CTL_PRNSELLTO_DTIN);
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_PRNSELLTO_FLAGS)) {
				const ushort v = getCtrlUInt16(CTL_PRNSELLTO_FLAGS);
				disableCtrls(v != 2, CTL_PRNSELLTO_DTIN, CTLCAL_PRNSELLTO_DTIN, 0);
				disableCtrl(CTL_PRNSELLTO_ORDER, v == 2);
				clearEvent(event);
			}
		}
	};
	int    ok = -1;
	SelWhatPrnDialog * dlg = new SelWhatPrnDialog();
	if(CheckDialogPtrErr(&dlg)) {
		ushort v = *pWhat;
		dlg->setCtrlData(CTL_PRNSELLTO_FLAGS, &v);
		dlg->setCtrlData(CTL_PRNSELLTO_DTIN,  pExpiry);
		dlg->setCtrlData(CTL_PRNSELLTO_ORDER, pOrder);
		dlg->disableCtrls(v != 2, CTL_PRNSELLTO_DTIN, CTLCAL_PRNSELLTO_DTIN, 0);
		dlg->disableCtrl(CTL_PRNSELLTO_ORDER, v == 2);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_PRNSELLTO_DTIN, pExpiry);
			*pWhat = (int)dlg->getCtrlUInt16(CTL_PRNSELLTO_FLAGS);
			dlg->getCtrlData(CTL_PRNSELLTO_ORDER, pOrder);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPViewDebtTrnovr::Print(const void *)
{
	uint   rpt_id = 0;
	PPReportEnv env(0, 0);
	if(Filt.CycleKind)
		rpt_id = REPORT_DEBTTRNOVR_CT;
	else if(Filt.ExtKind == DebtTrnovrFilt::ekExpiryPart)
		rpt_id = REPORT_DEBTTRNOVR_EXPIRYPART;
	else if(Filt.ExtKind == DebtTrnovrFilt::ekTurnover)
		rpt_id = REPORT_DEBTTRNOVR_TURNOVER;
	if(rpt_id) {
		PPAlddPrint(rpt_id, PView(this), &env);
	}
	else {
		int    what_prn = 0;
		ushort v = 0;
		LDATE  expiry = ExpiryDate;
		if(SelectPrintingDebtTrnovrSheet(&what_prn, &expiry, &v) > 0) {
			if(what_prn < 2) {
				if(v == 0)
					env.Sort = OrdByArticleName;
				else if(v == 1)
					env.Sort = OrdByDebit;
				else
					env.Sort = OrdByDebt;
				if(what_prn == 1) {
					Filt.Flags |= DebtTrnovrFilt::fPrintExt;
					rpt_id = REPORT_SELLTRNOVREXT;
				}
				else if(what_prn == 0) {
					Filt.Flags &= ~DebtTrnovrFilt::fPrintExt;
					rpt_id = (Filt.Flags & DebtTrnovrFilt::fExtended) ? REPORT_SELLTRNOVR2 : REPORT_SELLTRNOVR;
				}
			}
			else {
				Filt.Flags &= ~DebtTrnovrFilt::fPrintExt;
				ExpiryDate = expiry;
				rpt_id = REPORT_DEBTACK1;
			}
			PView pf(this);
			pf.ID = v;
			PPAlddPrint(rpt_id, pf, &env);
		}
	}
	return 1;
}

int ViewDebtTrnovr(const DebtTrnovrFilt * pFilt, int sellOrSuppl) { return PPView::Execute(PPVIEW_DEBTTRNOVR, pFilt, 1, reinterpret_cast<void *>(sellOrSuppl)); }
//
// Implementation of PPALDD_DebtTrnovr
//
PPALDD_CONSTRUCTOR(DebtTrnovr)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(DebtTrnovr) { Destroy(); }

int PPALDD_DebtTrnovr::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(DebtTrnovr, rsrv);
	H.FltLocID       = p_filt->LocIDList.getSingle();
	H.FltAccSheetID  = p_filt->AccSheetID;
	H.FltArID        = p_filt->CliIDList.getSingle();
	H.FltCurID       = p_filt->CurID;
	H.FltCountryID   = 0; // @obsolete
	H.FltRegionID    = 0; // @obsolete
	H.FltCityID      = p_filt->CityID;
	H.FltCatID       = p_filt->CategoryID;
	H.Flags  = p_filt->Flags;
	H.FltBeg = p_filt->Period.low;
	H.FltEnd = p_filt->Period.upp;
	H.FltExpiryBeg   = p_filt->ExpiryPeriod.low;
	H.FltExpiryEnd   = p_filt->ExpiryPeriod.upp;
	H.FltExpiry      = p_v->ExpiryDate;
	H.FltPaymentBeg  = p_filt->PaymPeriod.low;
	H.FltPaymentEnd  = p_filt->PaymPeriod.upp;
	H.fDebtOnly      = BIN(p_filt->Flags & DebtTrnovrFilt::fDebtOnly);
	H.fNoForwardPaym = BIN(p_filt->Flags & DebtTrnovrFilt::fNoForwardPaym);
	H.fLabelOnly     = BIN(p_filt->Flags & DebtTrnovrFilt::fLabelOnly);
	H.fAllCurrencies = BIN(p_filt->Flags & DebtTrnovrFilt::fAllCurrencies);
	H.fDeliveryAddr  = BIN(p_filt->Flags & DebtTrnovrFilt::fDeliveryAddr);
	H.fCalcTotalDebt = BIN(p_filt->Flags & DebtTrnovrFilt::fCalcTotalDebt);
	H.SellOrSuppl    = 0; // p_filt->SellOrSuppl; // @obsolete
	H.CycleKind      = static_cast<int16>(p_filt->CycleKind);
	H.FltCycle       = p_filt->Cf.Cycle;
	H.FltNumCycles   = p_filt->Cf.NumCycles;
	PPGetSubStr(PPTXT_DEBTTO_CYCLEKINDTITLE, p_filt->CycleKind, H.CtTitle, sizeof(H.CtTitle));
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_DebtTrnovr::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	PPViewDebtTrnovr * p_v = static_cast<PPViewDebtTrnovr *>(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	return BIN(p_v->InitIteration(static_cast<PPViewDebtTrnovr::IterOrder>(SortIdx)));
}

int PPALDD_DebtTrnovr::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(DebtTrnovr);
	I.BillID = item.BillID;
	I.ArID   = item.ID_;
	I.CurID  = item.CurID;
	I.TabID  = item.TabID;
	I.ArNo   = item.Ar;
	if(item.ObjType == PPOBJ_ARTICLE)
		I.RelPersonID = ObjectToPerson(item.ID_);
	else if(item.ObjType == PPOBJ_PERSON)
		I.RelPersonID = item.ID_;
	else
		I.RelPersonID = 0;
	STRNSCPY(I.ArName, item.ArName);
	STRNSCPY(I.TabText, item.TabText);
	I.Debit  = item.Debit;
	I.Credit = item.Credit;
	I.Debt   = item.Debt;
	I.RPaym  = item.RPaym;
	I.Reckon = item.Reckon;
	I.RDebt  = item.RDebt;
	I.TDebt  = item.TDebt;
	I._AvgPaym     = item._AvgPaym;
	I.BillPayDate  = item.PayDate;
	I.LastPaymDate = item.LastPaymDate;
	switch(((DebtTrnovrFilt *)p_v->GetBaseFilt())->CycleKind) {
		case DebtTrnovrFilt::ckNone: I.CtVal = 0; break;
		case DebtTrnovrFilt::ckExpiry: I.CtVal = item.Debt; break;
		case DebtTrnovrFilt::ckDelay: I.CtVal = item.Debt; break;
		case DebtTrnovrFilt::ckShipments: I.CtVal = item.Debit; break;
		case DebtTrnovrFilt::ckPayments: I.CtVal = item.Credit; break;
	}
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_DebtTrnovr::Destroy() { DESTROY_PPVIEW_ALDD(DebtTrnovr); }
//
//
//
/*static*/int PPDebtorStatConfig::Read(PPDebtorStatConfig * pCfg)
{
	int    r = -1;
	if(pCfg) {
		r = PPRef->GetPropMainConfig(PPPRP_DEBTORSTATCFG, pCfg, sizeof(*pCfg));
		if(r <= 0)
			memzero(pCfg, sizeof(*pCfg));
		if(pCfg->PaymSigmFactor <= 0.0 || pCfg->PaymSigmFactor > 1000.0)
			pCfg->PaymSigmFactor = 5.0;
		if(pCfg->FinRate < 0.0 || pCfg->FinRate >= 100.0)
			pCfg->FinRate = 0.0;
		if(pCfg->LimitTerm < 0 || pCfg->LimitTerm > 1000)
			pCfg->LimitTerm = 1;
		if(pCfg->LimitFactor <= 0.0f || pCfg->LimitFactor > 10.0f)
			pCfg->LimitFactor = 1.0f;
	}
	return r;
}

/*static*/int PPDebtorStatConfig::Edit()
{
	const  long prop_cfg_id  = PPPRP_DEBTORSTATCFG;
	const  long cfg_obj_type = PPCFGOBJ_DEBTORSTAT;
	int    ok = -1;
	int    is_new = 0;
	SString temp_buf;
	PPDebtorStatConfig cfg;
	MEMSZERO(cfg);
	TDialog * dlg = new TDialog(DLG_DSTATCFG);
	THROW(CheckCfgRights(cfg_obj_type, PPR_READ, 0));
	THROW(is_new = Read(&cfg));
	THROW(CheckDialogPtr(&dlg));
	dlg->SetupCalPeriod(CTLCAL_DSTATCFG_PERIOD, CTL_DSTATCFG_PERIOD);
	SetupPPObjCombo(dlg, CTLSEL_DSTATCFG_RELTYPE, PPOBJ_PERSONRELTYPE, cfg.HoldingRelTypeID, 0, 0);
	dlg->setCtrlReal(CTL_DSTATCFG_SIGMFACTOR, cfg.PaymSigmFactor);
	dlg->setCtrlReal(CTL_DSTATCFG_FINRATE, cfg.FinRate);
	dlg->setCtrlData(CTL_DSTATCFG_PAYMBAR, &cfg.PaymBar);
	dlg->setCtrlData(CTL_DSTATCFG_DELAYBAR, &cfg.DelayBar);
	dlg->setCtrlData(CTL_DSTATCFG_EXPIRYWT, &cfg.ExpiryWeight);
	dlg->setCtrlLong(CTL_DSTATCFG_LIMITTERM, cfg.LimitTerm);
	dlg->setCtrlData(CTL_DSTATCFG_LIMITFACTOR, &cfg.LimitFactor);
	dlg->setCtrlData(CTL_DSTATCFG_ADLIMITTERM, &cfg.LimitAddedTerm); // @v8.2.4
	dlg->setCtrlReal(CTL_DSTATCFG_RPREC, fdiv100i(cfg.LimitRoundPrec));
	dlg->AddClusterAssocDef(CTL_DSTATCFG_RDIR, 0, 0);
	dlg->AddClusterAssoc(CTL_DSTATCFG_RDIR, 1, +1);
	dlg->AddClusterAssoc(CTL_DSTATCFG_RDIR, 2, -1);
	dlg->SetClusterData(CTL_DSTATCFG_RDIR, cfg.LimitRoundDir);
	dlg->AddClusterAssoc(CTL_DSTATCFG_FLAGS, 0, PPDebtorStatConfig::fLimitTermFromAgreement);
	dlg->AddClusterAssoc(CTL_DSTATCFG_FLAGS, 1, PPDebtorStatConfig::fProcessAgents);
	dlg->AddClusterAssoc(CTL_DSTATCFG_FLAGS, 2, PPDebtorStatConfig::fLogRatingVal);
	dlg->AddClusterAssoc(CTL_DSTATCFG_FLAGS, 3, PPDebtorStatConfig::fSimpleLimitAlg);
	dlg->SetClusterData(CTL_DSTATCFG_FLAGS, cfg.Flags);
	SetPeriodInput(dlg, CTL_DSTATCFG_PERIOD, cfg.Period);
	dlg->setCtrlString(CTL_DSTATCFG_LASTTIME, temp_buf.Z().Cat(cfg.LastDtm, DATF_DMY|DATF_CENTURY, TIMF_HMS));
	while(ok < 0 && ExecView(dlg) == cmOK) {
		uint   sel = 0;
		THROW(CheckCfgRights(cfg_obj_type, PPR_MOD, 0));
		cfg.PaymSigmFactor = dlg->getCtrlReal(sel = CTL_DSTATCFG_SIGMFACTOR);
		if(cfg.PaymSigmFactor <= 0.0 || cfg.PaymSigmFactor > 1000.0)
			PPErrorByDialog(dlg, sel, PPERR_USERINPUT);
		else {
			cfg.FinRate = dlg->getCtrlReal(sel = CTL_DSTATCFG_FINRATE);
			if(cfg.FinRate < 0.0 || cfg.FinRate > 99.0)
				PPErrorByDialog(dlg, sel, PPERR_USERINPUT);
			else {
				cfg.LimitTerm = dlg->getCtrlLong(sel = CTL_DSTATCFG_LIMITTERM);
				if(cfg.LimitTerm < 0 || cfg.LimitTerm > 1000)
					PPErrorByDialog(dlg, sel, PPERR_USERINPUT);
				else {
					dlg->getCtrlData(sel = CTL_DSTATCFG_PAYMBAR, &cfg.PaymBar);
					if(cfg.PaymBar <= 0.0 || cfg.PaymBar > 2.0)
						PPErrorByDialog(dlg, sel, PPERR_USERINPUT);
					else {
						dlg->getCtrlData(sel = CTL_DSTATCFG_DELAYBAR, &cfg.DelayBar);
						if(cfg.DelayBar <= 0.0 || cfg.DelayBar > 2.0)
							PPErrorByDialog(dlg, sel, PPERR_USERINPUT);
						else {
							dlg->getCtrlData(sel = CTL_DSTATCFG_EXPIRYWT, &cfg.ExpiryWeight);
							if(cfg.ExpiryWeight < 0.0 || cfg.ExpiryWeight > 1.0)
								PPErrorByDialog(dlg, sel, PPERR_USERINPUT);
							else {
								dlg->getCtrlData(sel = CTL_DSTATCFG_LIMITFACTOR, &cfg.LimitFactor);
								if(cfg.LimitFactor <= 0.0f || cfg.LimitFactor > 10.0f)
									PPErrorByDialog(dlg, sel, PPERR_USERINPUT);
								else {
									if(GetPeriodInput(dlg, sel = CTL_DSTATCFG_PERIOD, &cfg.Period)) {
										dlg->getCtrlData(CTL_DSTATCFG_ADLIMITTERM, &cfg.LimitAddedTerm); // @v8.2.4
										dlg->getCtrlData(CTLSEL_DSTATCFG_RELTYPE, &cfg.HoldingRelTypeID);
										dlg->GetClusterData(CTL_DSTATCFG_FLAGS, &cfg.Flags);
										cfg.LimitRoundPrec = (long)(dlg->getCtrlReal(CTL_DSTATCFG_RPREC) * 100.0);
										dlg->GetClusterData(CTL_DSTATCFG_RDIR, &cfg.LimitRoundDir);
										{
											PPTransaction tra(1);
											THROW(tra);
											THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, prop_cfg_id, &cfg, sizeof(cfg), 0)); // @v8.2.4 0-->sizeof(cfg)
											DS.LogAction((is_new == -1) ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, cfg_obj_type, 0, 0, 0);
											THROW(tra.Commit());
										}
										ok = 1;
									}
									else
										PPErrorByDialog(dlg, sel);
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

PPDebtorStatConfig::PPDebtorStatConfig()
{
	THISZERO();
}
//
//
//
long PPDebtorStat::DebtDimItem::GetPaymPeriod() const
{
	return (FirstPaymDate && LastPaymDate) ? diffdate(LastPaymDate, FirstPaymDate) + 1 : 0;
}

double PPDebtorStat::DebtDimItem::CalcPaymDensity() const
{
	long   paym_period = GetPaymPeriod();
	return (paym_period > 0) ? (PaymAmount / paym_period) : 0.0;
}

PPDebtorStat::PPDebtorStat(PPID arID) : ArID(arID), RelArID(0), DelayMean(0.0), DelayVar(0.0), DelayGammaAlpha(0.0), DelayGammaBeta(0.0), DelayGammaTest(0.0),
	ExpiryMean(0.0), PaymAmount(0.0), PaymDensity(0.0), Limit(0.0), DebtCost(0.0), SigmFactor(0.0), PaymPeriod(0),
	FirstPaymDate(ZERODATE), LastPaymDate(ZERODATE), LimitTerm(0), Flags(0)
{
	memzero(Rating, sizeof(Rating));
}

int PPDebtorStat::IsAggregate() const
{
	return BIN(ArID == 0 || (Flags & (fAgent|fHolding)));
}

int PPDebtorStat::AddPayment(PPID debtDimID, LDATE dt, double amount)
{
	SETIFZ(FirstPaymDate, dt);
	SETMIN(FirstPaymDate, dt);
	SETMAX(LastPaymDate, dt);
	PaymAmount += amount;
	if(debtDimID) {
		uint pos = 0;
		if(DdList.lsearch(&debtDimID, &pos, CMPF_LONG)) {
			DebtDimItem & r_item = DdList.at(pos);
			SETIFZ(r_item.FirstPaymDate, dt);
			SETMIN(r_item.FirstPaymDate, dt);
			SETMAX(r_item.LastPaymDate, dt);
			r_item.PaymAmount += amount;
		}
		else {
			DebtDimItem item;
			MEMSZERO(item);
			item.DebtDimID = debtDimID;
			item.FirstPaymDate = dt;
			item.LastPaymDate = dt;
			item.PaymAmount = amount;
			DdList.insert(&item);
		}
	}
	return 1;
}

int PPDebtorStat::Finish()
{
	{
		StatBase stat(StatBase::fGammaTest);
		for(uint j = 0; j < DelayList.getCount(); j++) {
			stat.Step(static_cast<double>(DelayList.get(j)));
		}
		stat.Finish();
		DelayMean = stat.GetExp();
		DelayVar = stat.GetVariance();
		DelayGammaTest = stat.GetTestGamma();
		stat.GetGammaParams(&DelayGammaAlpha, &DelayGammaBeta);
	}
	{
		StatBase expiry_stat(0);
		for(uint j = 0; j < ExpiryList.getCount(); j++) {
			expiry_stat.Step(static_cast<double>(ExpiryList.get(j)));
		}
		expiry_stat.Finish();
		ExpiryMean = expiry_stat.GetExp();
	}
	assert((FirstPaymDate && LastPaymDate) || (!FirstPaymDate && !LastPaymDate));
	if(FirstPaymDate) {
		PaymPeriod = diffdate(LastPaymDate, FirstPaymDate) + 1;
		assert(PaymPeriod > 0);
		PaymDensity = PaymAmount / PaymPeriod;
	}
	return 1;
}

double PPDebtorStat::GetDelayVarRate() const
{
	return (DelayMean == 0.0) ? 0.0 : (sqrt(DelayVar) / DelayMean);
}

PPDebtorStatArray::PPDebtorStatArray() : TSCollection <PPDebtorStat>(), LogRating(0)
{
}

PPDebtorStat * FASTCALL PPDebtorStatArray::Get(PPID arID)
{
	PPDebtorStat * p_ret = 0;
	uint   pos = 0;
	if(lsearch(&arID, &pos, CMPF_LONG)) {
		p_ret = at(pos);
	}
	else {
		PPDebtorStat * p_item = new PPDebtorStat(arID);
		if(insert(p_item))
			p_ret = at(getCount()-1);
		else
			PPSetErrorSLib();
	}
	return p_ret;
}

double PPDebtorStatArray::GetSigmFactor(double sigmA, long paymPeriod, double paymPeriodMean) const
{
	return (sigmA > 0.0) ? sigmoid(sigmA, ((double)paymPeriod) - paymPeriodMean) : 1.0;
}

int PPDebtorStatArray::CalcDelayIndex(const PPDebtorStat * pItem, const Total * pTotal, double expWeight, double * pResult) const
{
	int    ok = 0;
	double v = 0;
	if(pItem->DelayMean != 0.0 && pTotal->DelayVarRateMean != 0.0) {
		double dvr = pItem->GetDelayVarRate() / pTotal->DelayVarRateMean;
		double e = fdivnz(pItem->ExpiryMean, pTotal->ExpiryMean);
		v = (e != 0.0) ? ((e * expWeight) + (dvr * (1.0 - expWeight))) : dvr;
		ok = (v == 0.0) ? -1 : 1;
	}
	ASSIGN_PTR(pResult, v);
	return ok;
}

static int GetDebtorRatingColor(const char * pRating, COLORREF * pColor)
{
	int    ok = -1;
	COLORREF c = 0;
	if(!isempty(pRating)) {
		char f = pRating[0];
		char s = pRating[1];
		if(f >= 'A' && f <= 'E' && s >= 'A' && s <= 'E') {
			COLORREF cf = GetColorRef(SClrRed);
			COLORREF cs = GetColorRef(SClrGreen);
			float rf = 1.0f - ((f - 'A')+1.0f) * 0.15f;
			float rs = 1.0f - ((s - 'A')+1.0f) * 0.15f;
			cf = LightenColor(cf, rf);
			cs = LightenColor(cs, rs);
			uint8 r = (uint8)(GetRValue(cf) + GetRValue(cs));
			uint8 g = (uint8)(GetGValue(cf) + GetGValue(cs));
			uint8 b = (uint8)(GetBValue(cf) + GetBValue(cs));
			c = RGB(r, g, b);
			ok = 1;
		}
	}
	ASSIGN_PTR(pColor, c);
	return ok;
}

double PPDebtorStatArray::PreprocessRatingVal(double val) const
{
	if(LogRating) {
		if(val == 0.0)
			return 0.0;
		else if(val < 0.0)
			return -log(-val);
		else
			return log(val);
	}
	else
		return val;
}

int PPDebtorStatArray::CalcRating(Total * pTotal, int outMatrixStyle, TSVector <SPoint3R> * pOutMatrix)
{
	int    ok = 1;
	Total  total;
	MEMSZERO(total);
	PPDebtorStatConfig ds_cfg;
	PPDebtorStatConfig::Read(&ds_cfg);
	double sigm_a = ds_cfg.PaymSigmFactor; // Параметр сигмоидальной функции, учитывающей историю платежей дебитора
	double fin_rate = fdiv100r(ds_cfg.FinRate); // Годовая процентная ставка стоимости денег
	double exp_weight = (ds_cfg.ExpiryWeight >= 0.0 && ds_cfg.ExpiryWeight <= 1.0) ? ds_cfg.ExpiryWeight : 0.5;
	long   simple_duration = 0;
	DateRange _period = ds_cfg.Period;
	_period.Actualize(ZERODATE);
	if(ds_cfg.Flags & PPDebtorStatConfig::fSimpleLimitAlg) {
		if(_period.low && _period.upp) {
			simple_duration = diffdate(_period.upp, _period.low);
			if(simple_duration < 0)
				simple_duration = 0;
			else
				simple_duration++;
		}
		else {
		}
	}
	LogRating = BIN(ds_cfg.Flags & PPDebtorStatConfig::fLogRatingVal);
	uint   i;
	SString rate_buf;
	SHistogram sh_delay;
	SHistogram sh_paym;
	StatBase st_period(0);
	StatBase st_expiry(0);
	StatBase st_delayvarrate(0);
	PPObjPerson psn_obj;
	PPObjArticle ar_obj;
	if(pOutMatrix)
		pOutMatrix->clear();
	total.SigmFactor = ds_cfg.PaymSigmFactor;
	sh_delay.SetupDev(0, (ds_cfg.DelayBar > 0.0 && ds_cfg.DelayBar <= 2.0) ? ds_cfg.DelayBar : 0.5, 3);
	sh_paym.SetupDev(0, (ds_cfg.PaymBar > 0.0 && ds_cfg.PaymBar <= 2.0) ? ds_cfg.PaymBar : 1.0, 3);
	for(i = 0; i < getCount(); i++) {
		PPDebtorStat * p_item = at(i);
		if(p_item && !p_item->IsAggregate()) {
			if(p_item->PaymPeriod > 0)
				st_period.Step(p_item->PaymPeriod);
			st_expiry.Step(p_item->ExpiryMean);
			st_delayvarrate.Step(p_item->GetDelayVarRate());
		}
	}
	st_period.Finish();
	st_expiry.Finish();
	st_delayvarrate.Finish();
	total.PaymPeriodMean = st_period.GetExp();
	total.ExpiryMean = st_expiry.GetExp();
	total.DelayVarRateMean = st_delayvarrate.GetExp();
	for(i = 0; i < getCount(); i++) {
		PPDebtorStat * p_item = at(i);
		if(p_item) {
			memzero(p_item->Rating, sizeof(p_item->Rating));
			if(!p_item->IsAggregate()) {
				double v;
				if(CalcDelayIndex(p_item, &total, exp_weight, &v) > 0) {
					THROW(sh_delay.PreparePut(PreprocessRatingVal(v)));
				}
				if(p_item->PaymDensity) {
					double s = GetSigmFactor(sigm_a, p_item->PaymPeriod, total.PaymPeriodMean) * p_item->PaymDensity;
					if(SIEEE754::IsValid(s) && s > 0.0)
						THROW(sh_paym.PreparePut(PreprocessRatingVal(s)));
				}
			}
		}
	}
	{
		const StatBase * p_stat_delay = 0;
		const StatBase * p_stat_paym = 0;
		const double limit_factor = (ds_cfg.LimitFactor > 0.0f && ds_cfg.LimitFactor <= 10.0f) ? ds_cfg.LimitFactor : 1.0;
		for(i = 0; i < getCount(); i++) {
			PPDebtorStat * p_item = at(i);
			if(p_item) {
				if(!p_item->IsAggregate()) {
					long   idx_delay = 0;
					long   idx_paym = 0;
					double delay_val = 0.0;
					double paym_val = 0.0;
					if(CalcDelayIndex(p_item, &total, exp_weight, &delay_val) > 0) {
						THROW(idx_delay = sh_delay.Put(PreprocessRatingVal(delay_val)));
						if(!p_stat_delay && (p_stat_delay = sh_delay.GetDeviationStat()) != 0) {
							double m = 0.0, w = 0.0;
							sh_delay.GetDeviationParams(&m, &w);
							total.DelayRatingMean = m;
							total.DelayRatingBar  = w;
						}
					}
					if(st_period.GetExp() && p_item->PaymDensity && p_item->PaymPeriod > 0) {
						double sigm = GetSigmFactor(sigm_a, p_item->PaymPeriod, total.PaymPeriodMean);
						p_item->SigmFactor = sigm;
						long   term = ds_cfg.LimitTerm;
						if(ds_cfg.Flags & PPDebtorStatConfig::fLimitTermFromAgreement) {
							PPClientAgreement agt_rec;
							if(ar_obj.GetClientAgreement(p_item->ArID, agt_rec, 0) > 0)
								term = agt_rec.DefPayPeriod;
						}
						p_item->LimitTerm = term;
						if(simple_duration > 0)
							p_item->Limit = p_item->LimitTerm * (p_item->PaymDensity * p_item->PaymPeriod) / simple_duration;
						else
							p_item->Limit = (sigm * p_item->PaymDensity) * p_item->LimitTerm;
						p_item->Limit *= limit_factor;
						if(ds_cfg.LimitRoundPrec > 0.0)
							p_item->Limit = PPRound(p_item->Limit, fdiv100i(ds_cfg.LimitRoundPrec), ds_cfg.LimitRoundDir);
						paym_val = sigm * p_item->PaymDensity; // log
						if(SIEEE754::IsValid(paym_val) && paym_val > 0.0) {
							THROW(idx_paym = sh_paym.Put(PreprocessRatingVal(paym_val)));
							if(!p_stat_paym && (p_stat_paym = sh_paym.GetDeviationStat()) != 0) {
								double m = 0.0, w = 0.0;
								sh_paym.GetDeviationParams(&m, &w);
								total.PaymRatingMean = m;
								total.PaymRatingBar  = w;
							}
						}
						else
							paym_val = 0.0;
						if(p_item->Limit > 0.0 && p_item->DdList.getCount()) {
							//
							// Рассчитывает кредитный лимит по долговым размерностям
							//
							for(uint j = 0; j < p_item->DdList.getCount(); j++) {
								PPDebtorStat::DebtDimItem & r_dd_item = p_item->DdList.at(j);
								const long paym_period = r_dd_item.GetPaymPeriod();
								const double dens = r_dd_item.CalcPaymDensity();
								if(dens > 0.0)
									if(simple_duration > 0)
										r_dd_item.Limit = p_item->LimitTerm * (dens * paym_period) / simple_duration;
									else
										r_dd_item.Limit = p_item->LimitTerm * (dens * sigm);
								r_dd_item.Limit *= limit_factor;
								if(ds_cfg.LimitRoundPrec > 0.0)
									r_dd_item.Limit = PPRound(r_dd_item.Limit, fdiv100i(ds_cfg.LimitRoundPrec), ds_cfg.LimitRoundDir);
							}
						}
					}
					if(idx_paym == 0)
						p_item->Rating[0] = 'E';
					else if(idx_paym == MAXLONG)
						p_item->Rating[0] = 'A';
					else if(idx_paym == -MAXLONG)
						p_item->Rating[0] = 'E';
					else if(idx_paym == 1)
						p_item->Rating[0] = 'D';
					else if(idx_paym == 2)
						p_item->Rating[0] = 'C';
					else if(idx_paym == 3)
						p_item->Rating[0] = 'B';
					else
						p_item->Rating[0] = '?';
					//
					if(idx_delay == 0)
						p_item->Rating[1] = 'E';
					else if(idx_delay == -MAXLONG)
						p_item->Rating[1] = 'A';
					else if(idx_delay == MAXLONG)
						p_item->Rating[1] = 'E';
					else if(idx_delay == 1)
						p_item->Rating[1] = 'B';
					else if(idx_delay == 2)
						p_item->Rating[1] = 'C';
					else if(idx_delay == 3)
						p_item->Rating[1] = 'D';
					else
						p_item->Rating[1] = '?';
					//
					// Заполнение исходящей матрицы
					//
					if(pOutMatrix) {
						SPoint3R p;
						if(outMatrixStyle == omLimitByPaymDensity) {
							p.x = p_item->PaymDensity;
							p.y = p_item->PaymPeriod;
							p.z = p_item->Limit;
							pOutMatrix->insert(&p);
						}
						else if(outMatrixStyle == PPDebtorStatArray::omDelayExpiryDots) {
							p.x = p_item->DelayMean;
							p.y = sqrt(p_item->DelayVar);
							p.z = p_item->ExpiryMean;
							pOutMatrix->insert(&p);
						}
						else if(outMatrixStyle == PPDebtorStatArray::omRatingData) {
							p.x = PreprocessRatingVal(paym_val);
							p.y = PreprocessRatingVal(delay_val);
							COLORREF c;
							if(GetDebtorRatingColor(p_item->Rating, &c) > 0)
								p.z = (double)(ulong)c;
							else
								p.z = (double)(ulong)GetColorRef(SClrGrey);
							pOutMatrix->insert(&p);
						}
						else if(outMatrixStyle == PPDebtorStatArray::omSigmFactor) {
							double sigm_factor = GetSigmFactor(sigm_a, p_item->PaymPeriod, total.PaymPeriodMean);
							p.x = p_item->PaymPeriod;
							p.y = sigm_factor;
							p.z = 0.0;
							pOutMatrix->insert(&p);
						}
					}
				}
				else {
					//if(p_item->DelayGammaTest > 10) {
						if(p_item->DelayMean && p_item->DelayVar) {
							double eta = p_item->DelayVar / p_item->DelayMean;
							double K = p_item->DelayMean / eta;
							double sum = 0.0;
							for(uint x = 1; x < 360; x++) { // @v6.0.12 180-->360
								double prob = (GammaIncompleteP(K, x/eta) - GammaIncompleteP(K, (x-1)/eta));
								sum += (fin_rate * x / 360) * prob;
							}
							p_item->DebtCost = sum;
						}
					//}
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pTotal, total);
	return ok;
}
//
//
//
DebtStatCore::DebtStatCore(const char * pFileName) : DebtStatTbl(pFileName)
{
}

int DebtStatCore::GetLastDate(PPID accSheetID, LDATE * pDt)
{
	int    ok = -1;
	LDATE  last_date = ZERODATE;
	DebtStatTbl::Key0 k0;
	MEMSZERO(k0);
	k0.Dt = MAXDATE;
	k0.AccSheetID = accSheetID;
	k0.ArID = MAXLONG;
	while(ok < 0 && search(0, &k0, spLt)) {
		if(data.AccSheetID == accSheetID) {
			last_date = data.Dt;
			ok = 1;
		}
	}
	ASSIGN_PTR(pDt, last_date);
	return ok;
}

int DebtStatCore::GetList(PPID accSheetID, PPDebtorStatArray & rList)
{
	int    ok = -1;
	BExtQuery * q = 0;
	DBQ  * dbq = 0;
	LDATE  last_date;
	rList.freeAll();
	if(GetLastDate(accSheetID, &last_date) > 0) {
		DebtStatTbl::Key0 k0, k0_;
		THROW_MEM(q = new BExtQuery(this, 0));
		q->selectAll();
		dbq = & (this->Dt == last_date && this->AccSheetID == accSheetID);
		q->selectAll().where(*dbq);
		MEMSZERO(k0);
		k0.Dt = last_date;
		k0.AccSheetID = accSheetID;
		k0_ = k0;
		for(q->initIteration(false, &k0, spGe); q->nextIteration() > 0;) {
			DebtStatTbl::Rec rec;
			CopyBufTo(&rec);
			PPDebtorStat * p_item = new PPDebtorStat(rec.ArID);
			THROW_MEM(p_item);
			p_item->RelArID = rec.RelArID;
			p_item->Flags = rec.Flags;
			p_item->DelayMean = rec.DelayMean;
			p_item->DelayVar = fpow2(rec.DelaySd);
			p_item->DelayGammaTest = rec.DelayTestGamma;
			p_item->PaymPeriod  = rec.PaymPeriod;
			p_item->PaymDensity = rec.PaymDensity;
			p_item->ExpiryMean = rec.ExpiryMean;
			p_item->Limit = rec.CreditLimit;
			p_item->LimitTerm = rec.LimitTerm;
			p_item->DebtCost = rec.DebtCost;
			p_item->SigmFactor = rec.SigmFactor;
			STRNSCPY(p_item->Rating, rec.Rating);
			THROW_SL(rList.insert(p_item));
			ok = 1;
		}
	}
	CATCHZOK
	delete q;
	return ok;
}

int DebtStatCore::SetList(PPID accSheetID, LDATE date, const PPDebtorStatArray & rList, int use_ta)
{
	int    ok = 1;
	const LDATETIME now_dtm = getcurdatetime_();
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(uint i = 0; i < rList.getCount(); i++) {
			PPDebtorStat * p_item = rList.at(i);
			DebtStatTbl::Key0 k0;
			DebtStatTbl::Rec rec;
			MEMSZERO(k0);
			k0.Dt = NZOR(date, now_dtm.d);
			k0.AccSheetID = accSheetID;
			k0.ArID = p_item->ArID;
			if(search(0, &k0, spEq)) {
				CopyBufTo(&rec);
				rec.RelArID = p_item->RelArID;
				if(!date)
					rec.Tm = now_dtm.t;
				rec.Flags = p_item->Flags;
				rec.DelayMean = p_item->DelayMean;
				rec.DelaySd = sqrt(p_item->DelayVar);
				rec.DelayTestGamma = p_item->DelayGammaTest;
				rec.DelayTestChSq = 0.0;
				rec.PaymPeriod = p_item->PaymPeriod;
				rec.PaymDensity = p_item->PaymDensity;
				rec.CreditLimit = p_item->Limit;
				rec.LimitTerm = (int16)p_item->LimitTerm;
				rec.DebtCost = p_item->DebtCost;
				rec.ExpiryMean = p_item->ExpiryMean;
				rec.SigmFactor = p_item->SigmFactor;
				STRNSCPY(rec.Rating, p_item->Rating);
				THROW_DB(updateRecBuf(&rec));
			}
			else {
				rec.AccSheetID = accSheetID;
				rec.ArID = p_item->ArID;
				rec.RelArID = p_item->RelArID;
				rec.Dt = NZOR(date, now_dtm.d);
				rec.Tm = now_dtm.t;
				rec.Flags = p_item->Flags;
				rec.DelayMean = p_item->DelayMean;
				rec.DelaySd = sqrt(p_item->DelayVar);
				rec.DelayTestGamma = p_item->DelayGammaTest;
				rec.DelayTestChSq = 0.0;
				rec.PaymPeriod = p_item->PaymPeriod;
				rec.PaymDensity = p_item->PaymDensity;
				rec.CreditLimit = p_item->Limit;
				rec.LimitTerm = static_cast<int16>(p_item->LimitTerm);
				rec.DebtCost = p_item->DebtCost;
				rec.ExpiryMean = p_item->ExpiryMean;
				rec.SigmFactor = p_item->SigmFactor;
				STRNSCPY(rec.Rating, p_item->Rating);
				THROW_DB(insertRecBuf(&rec));
			}
			PPWaitPercent(i+1, rList.getCount());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
PrcssrDebtRate::Param::Param()
{
	THISZERO();
}

int PrcssrDebtRate::Param::Read(SBuffer & rBuf, long)
{
	int    ok = 1;
	if(rBuf.GetAvailableSize()) {
		THROW(rBuf.Read(Ver));
		THROW(rBuf.Read(Reserve, sizeof(Reserve)));
		THROW(rBuf.Read(AccSheetID));
		THROW(rBuf.Read(Gandicap));
		THROW(rBuf.Read(Flags));
		THROW(rBuf.Read(Reserve2));
	}
	else
		ok = -1;
	CATCH
		ok = PPSetErrorSLib();
	ENDCATCH
	return ok;
}

int PrcssrDebtRate::Param::Write(SBuffer & rBuf, long)
{
	int    ok = 1;
	Ver = 0;
	THROW(rBuf.Write(Ver));
	THROW(rBuf.Write(Reserve, sizeof(Reserve)));
	THROW(rBuf.Write(AccSheetID));
	THROW(rBuf.Write(Gandicap));
	THROW(rBuf.Write(Flags));
	THROW(rBuf.Write(Reserve2));
	CATCH
		ok = PPSetErrorSLib();
	ENDCATCH
	return ok;
}

PrcssrDebtRate::PrcssrDebtRate()
{
	IsThereDebtRateLic = 0;
	PPLicData ld;
	if(DS.CheckExtFlag(ECF_OPENSOURCE) || (PPGetLicData(&ld) > 0 && ld.ExtFunc & PPLicData::effDebtorStat))
		IsThereDebtRateLic = 1;
	PPDebtorStatConfig::Read(&Cfg);
}

PrcssrDebtRate::~PrcssrDebtRate()
{
}

int PrcssrDebtRate::EditParam(Param * pParam)
{
	class DebtRateFiltDialog : public TDialog {
		DECL_DIALOG_DATA(PrcssrDebtRate::Param);
	public:
		explicit DebtRateFiltDialog(int isThereDebtRateLic) : TDialog(DLG_DEBTRATE), IsThereDebtRateLic(isThereDebtRateLic)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_DEBTRATE_ACS, PPOBJ_ACCSHEET, Data.AccSheetID, 0, 0);
			setCtrlLong(CTL_DEBTRATE_GANDICAP, Data.Gandicap);
			if(!IsThereDebtRateLic) {
				DisableClusterItem(CTL_DEBTRATE_FLAGS, 3, 1);
				DisableClusterItem(CTL_DEBTRATE_FLAGS, 4, 1);
				DisableClusterItem(CTL_DEBTRATE_FLAGS, 5, 1);
				DisableClusterItem(CTL_DEBTRATE_FLAGS, 6, 1);
				DisableClusterItem(CTL_DEBTRATE_FLAGS, 7, 1);
				DisableClusterItem(CTL_DEBTRATE_FLAGS, 8, 1);
				DisableClusterItem(CTL_DEBTRATE_FLAGS, 9, 1);
				Data.Flags &= ~PrcssrDebtRate::Param::fGatherPaymDelayStat;
			}
			else {
				DisableClusterItem(CTL_DEBTRATE_FLAGS, 4, !(Data.Flags & Data.fGatherPaymDelayStat));
				DisableClusterItem(CTL_DEBTRATE_FLAGS, 5, !(Data.Flags & Data.fGatherPaymDelayStat));
				DisableClusterItem(CTL_DEBTRATE_FLAGS, 6, !(Data.Flags & Data.fGatherPaymDelayStat) || !(Data.Flags & Data.fSetupLimit));
				DisableClusterItem(CTL_DEBTRATE_FLAGS, 7, !(Data.Flags & Data.fGatherPaymDelayStat) || !(Data.Flags & Data.fSetupLimit));
			}
			AddClusterAssoc(CTL_DEBTRATE_FLAGS, 0, Data.fReportOnly);
			AddClusterAssoc(CTL_DEBTRATE_FLAGS, 1, Data.fReportAgtAbsence);
			AddClusterAssoc(CTL_DEBTRATE_FLAGS, 2, Data.fAllowForMaxCredit);
			AddClusterAssoc(CTL_DEBTRATE_FLAGS, 3, Data.fGatherPaymDelayStat);
			AddClusterAssoc(CTL_DEBTRATE_FLAGS, 4, Data.fProcessAgents);
			AddClusterAssoc(CTL_DEBTRATE_FLAGS, 5, Data.fSetupLimit);
			AddClusterAssoc(CTL_DEBTRATE_FLAGS, 6, Data.fUpdateLimitDebtDim);
			AddClusterAssoc(CTL_DEBTRATE_FLAGS, 7, Data.fCreateLimitDebtDim);
			AddClusterAssoc(CTL_DEBTRATE_FLAGS, 8, Data.fUpdateStopDebtDim);
			AddClusterAssoc(CTL_DEBTRATE_FLAGS, 9, Data.fProjCommStopToDebtDim);
			SetClusterData(CTL_DEBTRATE_FLAGS, Data.Flags);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			Data.AccSheetID = getCtrlLong(sel = CTLSEL_DEBTRATE_ACS);
			if(Data.AccSheetID == 0) {
				ok = PPErrorByDialog(this, sel, PPERR_ACCSHEETNEEDED);
			}
			else {
				Data.Gandicap = getCtrlLong(sel = CTL_DEBTRATE_GANDICAP);
				if(Data.Gandicap < 0 || Data.Gandicap > 1000)
					ok = PPErrorByDialog(this, sel, PPERR_USERINPUT);
				else {
					GetClusterData(CTL_DEBTRATE_FLAGS, &Data.Flags);
					ASSIGN_PTR(pData, Data);
					ok = 1;
				}
			}
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_DEBTRATE_FLAGS)) {
				GetClusterData(CTL_DEBTRATE_FLAGS, &Data.Flags);
				if(IsThereDebtRateLic) {
					DisableClusterItem(CTL_DEBTRATE_FLAGS, 4, !(Data.Flags & Data.fGatherPaymDelayStat));
					DisableClusterItem(CTL_DEBTRATE_FLAGS, 5, !(Data.Flags & Data.fGatherPaymDelayStat));
					DisableClusterItem(CTL_DEBTRATE_FLAGS, 6, !(Data.Flags & Data.fGatherPaymDelayStat) || !(Data.Flags & Data.fSetupLimit));
					DisableClusterItem(CTL_DEBTRATE_FLAGS, 7, !(Data.Flags & Data.fGatherPaymDelayStat) || !(Data.Flags & Data.fSetupLimit));
				}
				clearEvent(event);
			}
		}
		int    IsThereDebtRateLic;
	};
	DIALOG_PROC_BODY_P1(DebtRateFiltDialog, IsThereDebtRateLic, pParam);
}

int PrcssrDebtRate::InitParam(Param * pParam)
{
	memzero(pParam, sizeof(*pParam));
	pParam->AccSheetID = GetSellAccSheet();
	pParam->Flags |= Param::fReportOnly;
	if(IsThereDebtRateLic) {
		if(Cfg.Flags & PPDebtorStatConfig::fProcessAgents)
			pParam->Flags |= Param::fProcessAgents;
	}
	return 1;
}

int PrcssrDebtRate::Init(const Param * pParam)
{
	RVALUEPTR(P, pParam);
	return 1;
}

int PrcssrDebtRate::GatherPaymDelayStat(PPLogger * pLogger, int use_ta)
{
	//
	// Коллекция PPDebtorStatEntrySet будет просто хранить указатели на PPDebtorStat,
	// но ни в коем случае не станет разрушать объекты по этим указателям
	// (опции не содержат флага aryEachItem)
	//
	class PPDebtorStatEntrySet : public TSCollection <PPDebtorStat> {
	public:
		PPDebtorStatEntrySet() : TSCollection <PPDebtorStat>()
		{
			setFlag(aryEachItem, 0);
		}
		int    AddEntry(PPDebtorStat * pEntry, long flags)
		{
			if(pEntry) {
				if(flags)
					pEntry->Flags |= flags;
				return insert(pEntry) ? 1 : PPSetErrorSLib();
			}
			else
				return -1;
		}
		void   AddDelay(long delay, long expiry)
		{
			for(uint j = 0; j < getCount(); j++) {
				PPDebtorStat * p = at(j);
				if(p) {
					p->DelayList.add(delay);
					p->ExpiryList.add(expiry);
				}
			}
		}
		void   AddPayment(PPID debtDimID, LDATE dt, double amt)
		{
			for(uint j = 0; j < getCount(); j++) {
				PPDebtorStat * p = at(j);
				CALLPTRMEMB(p, AddPayment(debtDimID, dt, amt));
			}
		}
	};
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	uint   i;
	SString line_buf, op_name, ar_name;
	PPDebtorStatArray list;
	PPIDArray op_list;
	PPIDArray inc_op_list;
	PPIDArray paym_id_list;
	PPIDArray pool_memb_list;
	LAssocArray rel_list; // Список отношений статья-холдинг
	PPObjOprKind op_obj;
	PPObjDebtDim dd_obj;
	LAssocArray dd_agent_list;
	LongArray dd_list; // temp list
	BillTbl::Rec bill_rec, last_paym_rec;
	PPBillExt bill_ext_rec;
	PPDebtorStatEntrySet entry_set;
	//
	// Собираем статистику по задержкам платежей
	//
	op_obj.GetPayableOpList(P.AccSheetID, &op_list);
	// @v6.0.11 { Пока не задействовано
	DateRange period;
	period = Cfg.Period;
	period.Actualize(ZERODATE);
	// } @v6.0.11
	dd_obj.FetchAgentList(&dd_agent_list);
	for(i = 0; i < op_list.getCount(); i++) {
		const  PPID op_id = op_list.get(i);
		GetOpName(op_id, op_name);
		for(SEnum en = p_bobj->P_Tbl->EnumByOp(op_id, &period, 0); en.Next(&bill_rec) > 0;) {
			if(bill_rec.Flags & BILLF_PAYOUT) {
				LDATE last_pay_date = ZERODATE;
				p_bobj->P_Tbl->GetLastPayDate(bill_rec.ID, &last_pay_date);
				if(p_bobj->P_Tbl->GetLastPayment(bill_rec.ID, &last_paym_rec) > 0) {
					long   term  = (last_pay_date ? diffdate(last_pay_date, bill_rec.Dt) : 0);
					long   delay = diffdate(last_paym_rec.Dt, bill_rec.Dt);
					long   expiry = diffdate(last_paym_rec.Dt, NZOR(last_pay_date, bill_rec.Dt));
					if(delay > 0) {
						//
						// Документы с нулевыми задержками платежа не учитываем
						// поскольку фактически они не являются документами с отложенным
						// платежом (предоплата, наличный расчет и т.д.)
						//
						PPDebtorStat * p_entry = list.Get(bill_rec.Object);
						entry_set.clear();
						entry_set.AddEntry(p_entry, 0);
						entry_set.AddEntry(list.Get(0), 0);
						if(Cfg.HoldingRelTypeID) {
							PPID holding_ar_id = 0;
							if(!rel_list.Search(bill_rec.Object, &holding_ar_id, 0)) {
								ArObj.GetRelPersonSingle(bill_rec.Object, Cfg.HoldingRelTypeID, 0, &holding_ar_id);
								rel_list.Add(bill_rec.Object, holding_ar_id, 0);
							}
							if(holding_ar_id) {
								p_entry->RelArID = holding_ar_id;
								entry_set.AddEntry(list.Get(holding_ar_id), PPDebtorStat::fHolding);
							}
						}
						if(P.Flags & Param::fProcessAgents) {
							if(p_bobj->FetchExt(bill_rec.ID, &bill_ext_rec) > 0 && bill_ext_rec.AgentID)
								entry_set.AddEntry(list.Get(bill_ext_rec.AgentID), PPDebtorStat::fAgent);
						}
						entry_set.AddDelay(delay, expiry);
						entry_set.clear();
					}
				}
			}
			PPWaitMsg(line_buf.Z().Cat(op_name).CatDiv('-', 1).Cat(bill_rec.Dt));
		}
	}
	//
	// Собираем статистику по платежам
	//
	op_obj.GetProfitableOpList(P.AccSheetID, &inc_op_list);
	for(i = 0; i < inc_op_list.getCount(); i++) {
		const  PPID op_id = inc_op_list.get(i);
		GetOpName(op_id, op_name);
		const int need_paym = op_list.lsearch(op_id);
		for(SEnum en = p_bobj->P_Tbl->EnumByOp(op_id, 0, 0); en.Next(&bill_rec) > 0;) {
			PPID   agent_id = 0;
			PPID   debt_dim_id = 0;
			PPWaitMsg(line_buf.Z().Cat(op_name).CatDiv('-', 1).Cat(bill_rec.Dt));
			PPDebtorStat * p_entry = list.Get(bill_rec.Object);
			entry_set.clear();
			entry_set.AddEntry(p_entry, 0);
			entry_set.AddEntry(list.Get(0), 0);
			if(Cfg.HoldingRelTypeID) {
				PPID holding_ar_id = 0;
				if(!rel_list.Search(bill_rec.Object, &holding_ar_id, 0)) {
					ArObj.GetRelPersonSingle(bill_rec.Object, Cfg.HoldingRelTypeID, 0, &holding_ar_id);
					rel_list.Add(bill_rec.Object, holding_ar_id, 0);
				}
				if(holding_ar_id) {
					p_entry->RelArID = holding_ar_id;
					entry_set.AddEntry(list.Get(holding_ar_id), PPDebtorStat::fHolding);
				}
			}
			if(P.Flags & Param::fProcessAgents || dd_agent_list.getCount()) {
				if(p_bobj->FetchExt(bill_rec.ID, &bill_ext_rec) > 0 && bill_ext_rec.AgentID) {
					agent_id = bill_ext_rec.AgentID;
					if(P.Flags & Param::fProcessAgents)
						entry_set.AddEntry(list.Get(bill_ext_rec.AgentID), PPDebtorStat::fAgent);
					if(dd_agent_list.getCount()) {
						dd_list.clear();
						dd_agent_list.GetListByVal(agent_id, dd_list);
						if(dd_list.getCount())
							debt_dim_id = dd_list.get(0);
					}
				}
			}
			//
			if(need_paym) {
				paym_id_list.clear();
				for(DateIter di2(&period); p_bobj->P_Tbl->EnumLinks(bill_rec.ID, &di2, BLNK_PAYMRETN, &last_paym_rec) > 0;) {
					//if(last_paym_rec.CurID == curID) {
						paym_id_list.add(last_paym_rec.ID);
						entry_set.AddPayment(debt_dim_id, last_paym_rec.Dt, BR2(last_paym_rec.Amount));
					//}
				}
				{
					pool_memb_list.clear();
					THROW(p_bobj->P_Tbl->GetPoolMembersList(PPASS_PAYMBILLPOOL, bill_rec.ID, &pool_memb_list));
					for(uint j = 0; j < pool_memb_list.getCount(); j++) {
						const  PPID reckon_id = pool_memb_list.get(j);
						if(!paym_id_list.lsearch(reckon_id) && p_bobj->Search(reckon_id, &last_paym_rec) > 0 /*&& last_paym_rec.CurID == curID*/) {
							if(period.CheckDate(last_paym_rec.Dt)) {
								paym_id_list.add(last_paym_rec.ID);
								entry_set.AddPayment(debt_dim_id, last_paym_rec.Dt, BR2(last_paym_rec.Amount));
							}
						}
					}
				}
			}
			else if(period.CheckDate(bill_rec.Dt))
				entry_set.AddPayment(debt_dim_id, bill_rec.Dt, bill_rec.Amount);
			entry_set.clear();
		}
	}
	{
		SString file_name, stat_file_name;
		PPGetFilePath(PPPATH_OUT, "paymdelaystat.txt", file_name);
		PPGetFilePath(PPPATH_OUT, "debtorstat.txt", stat_file_name);
		SFile file(file_name, SFile::mWrite);
		SFile stat_file(stat_file_name, SFile::mWrite);
		list.sort(CMPF_LONG); // Сортируем список по идентификатору статьи
		if(stat_file.IsValid()) {
			line_buf.Z().
				Cat("ArID").Semicol().
				Cat("ArName").Semicol().
				Cat("DelayMean").Semicol().
				Cat("DelayVar").Semicol().
				Cat("DelayGammaAlpha").Semicol().
				Cat("DelayGammaBeta").Semicol().
				Cat("DelayGammaTest").Semicol().
				Cat("ExpiryMean").Semicol().
				Cat("PaymAmount").Semicol().
				Cat("PaymDensity").Semicol().
				Cat("PaymPeriod").CR();
			stat_file.WriteLine(line_buf);
		}
		for(i = 0; i < list.getCount(); i++) {
			PPDebtorStat * p_item = list.at(i);
			GetArticleName(p_item->ArID, ar_name);
			line_buf.Printf(PPLoadTextS(PPTXT_CALCDEBTORSTAT, op_name), ar_name.cptr());
			PPWaitMsg(line_buf);
			p_item->Finish();
			if(stat_file.IsValid()) {
				line_buf.Z().
					Cat(p_item->ArID).Semicol().
					Cat(ar_name).Semicol().
					Cat(p_item->DelayMean, SFMT_QTTY).Semicol().
					Cat(p_item->DelayVar, SFMT_QTTY).Semicol().
					Cat(p_item->DelayGammaAlpha, SFMT_QTTY).Semicol().
					Cat(p_item->DelayGammaBeta, SFMT_QTTY).Semicol().
					Cat(p_item->DelayGammaTest, SFMT_QTTY).Semicol().
					Cat(p_item->ExpiryMean, SFMT_QTTY).Semicol().
					Cat(p_item->PaymAmount, SFMT_QTTY).Semicol().
					Cat(p_item->PaymDensity, SFMT_QTTY).Semicol().
					Cat(p_item->PaymPeriod).CR();
				stat_file.WriteLine(line_buf);
			}
			if(file.IsValid()) {
				for(uint j = 0; j < p_item->DelayList.getCount(); j++) {
					line_buf.Z().Cat(p_item->ArID).Semicol().Cat(p_item->DelayList.get(j)).CR();
					file.WriteLine(line_buf);
				}
			}
		}
		{
			DebtStatCore ds_tbl;
			PPTransaction tra(1);
			THROW(tra);
			THROW(list.CalcRating(0));
			THROW(ds_tbl.SetList(P.AccSheetID, ZERODATE, list, 0));
			{
				//
				// Сохраняем время последнего сбора статистики
				//
				const  long prop_cfg_id = PPPRP_DEBTORSTATCFG;
				PPDebtorStatConfig cfg;
				THROW(PPDebtorStatConfig::Read(&cfg));
				cfg.LastDtm = getcurdatetime_();
				THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, prop_cfg_id, &cfg, 0, 0));
			}
			if(P.Flags & Param::fSetupLimit) {
				const LDATE cur_dt = getcurdate_();
				for(i = 0; i < list.getCount(); i++) {
					const PPDebtorStat * p_item = list.at(i);
					if(p_item->ArID && !(p_item->Flags & (PPDebtorStat::fAgent|PPDebtorStat::fHolding))) {
						PPClientAgreement agt;
						if(ArObj.GetClientAgreement(p_item->ArID, agt, 0) > 0) {
							if(agt.LockPrcBefore && cur_dt <= agt.LockPrcBefore) {
								; // @log "Для клиента '@article' блокировано автоматическое изменение кредитного лимита (=@real) до @date"
								CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_DEBTRATE_LIMITLOCK, &line_buf, p_item->ArID, agt.MaxCredit, agt.LockPrcBefore)));
							}
							else {
								int    dd_lim_upd = 0;
								double prev_limit = agt.MaxCredit;
								if(p_item->DdList.getCount()) {
									for(uint j = 0; j < p_item->DdList.getCount(); j++) {
										const PPDebtorStat::DebtDimItem & r_dd_item = p_item->DdList.at(j);
										const  PPID dd_id = r_dd_item.DebtDimID;
										const double new_lim = MAX(r_dd_item.Limit, 0.0);
										PPClientAgreement::DebtLimit * p_dd_entry = agt.GetDebtDimEntry(dd_id);
										if(p_dd_entry) {
											if(P.Flags & Param::fUpdateLimitDebtDim) {
												if(p_dd_entry->LockPrcBefore && cur_dt <= p_dd_entry->LockPrcBefore) {
													CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_DEBTRATE_DDLIMLOCK, &line_buf, p_item->ArID, dd_id, p_dd_entry->Limit, p_dd_entry->LockPrcBefore)));
												}
												else if(p_dd_entry->Limit != new_lim) {
													if(!(P.Flags & Param::fReportOnly)) {
														p_dd_entry->Limit = new_lim;
														dd_lim_upd = 1;
													}
													if(pLogger) {
														pLogger->Log(PPFormatT(
															(P.Flags & Param::fReportOnly) ? PPTXT_LOG_DEBTRATE_DDLIMSHUPD : PPTXT_LOG_DEBTRATE_DDLIMUPD,
															&line_buf, dd_id, new_lim, p_item->ArID, new_lim));
													}
												}
												else {
													CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_DEBTRATE_DDLIMUNCHG, &line_buf, dd_id, p_item->ArID, new_lim)));
												}
											}
										}
										else if(new_lim > 0.0 && P.Flags & Param::fCreateLimitDebtDim) {
											if(!(P.Flags & Param::fReportOnly)) {
												PPClientAgreement::DebtLimit new_dd_entry;
												MEMSZERO(new_dd_entry);
												new_dd_entry.DebtDimID = dd_id;
												new_dd_entry.Limit = new_lim;
												agt.DebtLimList.insert(&new_dd_entry);
												dd_lim_upd = 1;
											}
											if(pLogger) {
												pLogger->Log(PPFormatT(
													(P.Flags & Param::fReportOnly) ? PPTXT_LOG_DEBTRATE_DDLIMSHCR : PPTXT_LOG_DEBTRATE_DDLIMCR,
													&line_buf, dd_id, new_lim, p_item->ArID));
											}
										}
									}
									{
										uint k = agt.DebtLimList.getCount();
										if(k) do {
											PPClientAgreement::DebtLimit & r_dd_entry = agt.DebtLimList.at(--k);
											PPID agt_dd_id = r_dd_entry.DebtDimID;
											if(!p_item->DdList.lsearch(&agt_dd_id, 0, CMPF_LONG)) {
												if(!r_dd_entry.LockPrcBefore || cur_dt > r_dd_entry.LockPrcBefore) {
													if(pLogger) {
														pLogger->Log(PPFormatT(
															(P.Flags & Param::fReportOnly) ? PPTXT_LOG_DEBTRATE_DDLIMSHDEL : PPTXT_LOG_DEBTRATE_DDLIMDEL,
															&line_buf, r_dd_entry.DebtDimID, r_dd_entry.Limit, p_item->ArID));
													}
													if(!(P.Flags & Param::fReportOnly)) {
														r_dd_entry.Limit = 0.0;
														dd_lim_upd = 1;
													}
												}
											}
										} while(k);
									}
								}
								const int comm_lim_upd = BIN(agt.MaxCredit != p_item->Limit);
								if(comm_lim_upd || dd_lim_upd) {
									agt.MaxCredit = p_item->Limit;
									if((P.Flags & Param::fReportOnly) || ArObj.PutClientAgreement(p_item->ArID, &agt, 0)) {
										; // @log "Установлен кредитный лимит =@real для клиента '@article' (предыдущий лимит =@real)"
										if(comm_lim_upd && pLogger) {
											pLogger->Log(PPFormatT(
												(P.Flags & Param::fReportOnly) ? PPTXT_LOG_DEBTRATE_LIMITSHOULDSET : PPTXT_LOG_DEBTRATE_LIMITSET,
												&line_buf, agt.MaxCredit, p_item->ArID, prev_limit));
										}
									}
									else {
										; // @logerr "Ошибка записи соглашения при установке кредитного лимита =@real для клиента '@article': @lasterr"
										CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_DEBTRATE_LIMITERR, &line_buf, agt.MaxCredit, p_item->ArID, PPErrCode)));
									}
								}
								else {
									; // @log "Кредитный лимит для клиента '@article' не изменился (=@real)"
									CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_DEBTRATE_LIMITUNCHG, &line_buf, p_item->ArID, agt.MaxCredit)));
								}
							}
						}
						else {
							; // @log "Клиент '@article' не имеет клиентского соглашения (рассчитанный лимит =@real)"
						}
					}
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int PrcssrDebtRate::Run()
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	PPLogger logger;
	PPWaitStart();
	if(P.AccSheetID) {
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		if(P.Flags & PrcssrDebtRate::Param::fGatherPaymDelayStat) {
			if(IsThereDebtRateLic) {
				THROW(GatherPaymDelayStat(&logger, 1));
				ok = 1;
			}
			else {
				logger.LogString(PPTXT_NOLIC_DEBTORSTAT, 0);
			}
		}
		else if(P.AccSheetID == GetSellAccSheet() || (acs_obj.Fetch(P.AccSheetID, &acs_rec) > 0 && acs_rec.Flags & ACSHF_USECLIAGT)) {
			struct StopItem { // @flat
				PPID   ArID;
				PPID   DebtDimID;
				double Debt;
				double Rckn;
				long   MaxExpiry;
			};
			const  int update_stop_debtdim = BIN(P.Flags & P.fUpdateStopDebtDim);
			uint   i;
			ArticleTbl::Rec ar_rec;
			PPIDArray ar_list;
			PPIDArray op_list;
			PPIDArray debt_dim_list; // Полный список идентификаторов долговых размерностей
			LAssocArray dim_set_stop_list; // Список признаков STOP, ассоциированных с идентификаторами долговых размерностей
			PPObjOprKind op_obj;
			SString fmt_buf, msg_buf;
			PPObjBill::DebtBlock blk;
			SVector set_stop_list(sizeof(StopItem));       // Список статей, которым следует выставить STOP
			SVector reset_stop_list(sizeof(StopItem));     // Список статей, с которых следует снять STOP
			{
				PPObjDebtDim dd_obj;
				PPDebtDim dd_rec;
				for(SEnum en = dd_obj.P_Ref->Enum(PPOBJ_DEBTDIM, Reference::eoIdName); en.Next(&dd_rec) > 0;)
					debt_dim_list.add(dd_rec.ID);
			}
			DateRange period;
			period = Cfg.Period;
			period.Actualize(ZERODATE);
			THROW(op_obj.GetPayableOpList(P.AccSheetID, &op_list));
			THROW(ArObj.P_Tbl->GetListBySheet(P.AccSheetID, &ar_list, 0));
			for(i = 0; i < ar_list.getCount(); i++) {
				const  PPID ar_id = ar_list.get(i);
				int   do_set_stop = -1;
				PPClientAgreement agt_rec;
				ar_rec.Clear();
				if(ArObj.Fetch(ar_id, &ar_rec) > 0) {
					double rckn = 0.0;
					int    rckn_inited = 0;
					int    r = ArObj.GetClientAgreement(ar_id, agt_rec, 0);
					if(!r)
						logger.LogLastError();
					else {
						do_set_stop = 0;
						dim_set_stop_list.clear();
						if(r < 0 && P.Flags & Param::fReportAgtAbsence)
							logger.LogString(PPTXT_ARHASNTAGREEMENT, ar_rec.Name);
						p_bobj->CalcClientDebt(ar_id, &period, update_stop_debtdim, blk.Z());
						if(blk.Debt > 0.0) {
							if(blk.MaxExpiry > P.Gandicap || P.Flags & Param::fAllowForMaxCredit && agt_rec.MaxCredit > 0.0 || update_stop_debtdim) {
								AmtList amt_list, paym_list;
								PayableBillList list(&amt_list, &paym_list);
								THROW(p_bobj->GetReceivableBillList(ar_id, 0, &list));
								rckn = amt_list.Get(0, 0) - paym_list.Get(0, 0);
								if(rckn < blk.Debt) { // Если сумма незачтенных оплат больше или равна долгу, то считаем долг закрытым
									if(blk.MaxExpiry > P.Gandicap) {
										PPFormatT(PPTXT_LOG_DEBTRATE_DEBTEXCEEDMAXTERM, &msg_buf, ar_id, blk.MaxExpiry, P.Gandicap);
										logger.Log(msg_buf);
										do_set_stop = 1;
									}
									else if(P.Flags & Param::fAllowForMaxCredit && agt_rec.MaxCredit > 0.0 && (blk.Debt - rckn) > agt_rec.MaxCredit) {
										PPFormatT(PPTXT_LOG_DEBTRATE_DEBTEXCEEDMAXCREDIT, &msg_buf, ar_id, (blk.Debt - rckn), agt_rec.MaxCredit);
										logger.Log(msg_buf);
										do_set_stop = 1;
									}
									if(update_stop_debtdim) {
										for(uint j = 0; j < blk.DebtDimList.getCount(); j++) {
											PPObjBill::DebtBlock::DimItem & r_dim_item = blk.DebtDimList.at(j);
											if(r_dim_item.Debt > 0.0) {
												const double lim = agt_rec.GetCreditLimit(r_dim_item.DimID);
												int   stop_dim = 0;
												if(r_dim_item.MaxExpiry > P.Gandicap) {
													PPFormatT(PPTXT_LOG_DEBTRATE_DEBTDIMEXCEEDMAXTERM, &msg_buf, ar_id, r_dim_item.DimID, r_dim_item.MaxExpiry, P.Gandicap);
													logger.Log(msg_buf);
													stop_dim = 1;
													dim_set_stop_list.Add(r_dim_item.DimID, 1, 0);
												}
												else if(P.Flags & Param::fAllowForMaxCredit && lim > 0.0 && r_dim_item.Debt > lim) {
													PPFormatT(PPTXT_LOG_DEBTRATE_DEBTDIMEXCEEDMAXCREDIT, &msg_buf, ar_id, r_dim_item.DimID, r_dim_item.Debt, lim);
													logger.Log(msg_buf);
													dim_set_stop_list.Add(r_dim_item.DimID, 1, 0);
													stop_dim = 1;
												}
												else
													stop_dim = 0;
												if(stop_dim > 0) {
													StopItem si;
													si.ArID = ar_id;
													si.DebtDimID = r_dim_item.DimID;
													si.Debt = r_dim_item.Debt;
													si.Rckn = 0.0;
													si.MaxExpiry = r_dim_item.MaxExpiry;
													THROW_SL(set_stop_list.insert(&si));
												}
											}
										}
									}
								}
							}
						}
						if(do_set_stop >= 0) {
							StopItem si;
							si.ArID = ar_id;
							si.DebtDimID = 0;
							si.Debt = blk.Debt;
							si.Rckn = rckn;
							si.MaxExpiry = blk.MaxExpiry;
							THROW_SL(do_set_stop ? set_stop_list.insert(&si) : reset_stop_list.insert(&si));
						}
					}
				}
				PPWaitPercent(i+1, ar_list.getCount(), ar_rec.Name);
			}
			{
				//
				// Транзакционная часть процесса вынесена в отдельный блок
				// дабы не "забивать" СУБД обширным чтением в рамках транзакции.
				//
				PPArticlePacket pack;
				PPIDArray ar_list;
				set_stop_list.sort(PTR_CMPFUNC(_2long));
				reset_stop_list.sort(PTR_CMPFUNC(_2long));
				for(i = 0; i < set_stop_list.getCount(); i++) {
					const StopItem * p_si = static_cast<const StopItem *>(set_stop_list.at(i));
					ar_list.addUnique(p_si->ArID);
				}
				for(i = 0; i < reset_stop_list.getCount(); i++) {
					const StopItem * p_si = static_cast<const StopItem *>(reset_stop_list.at(i));
					ar_list.addUnique(p_si->ArID);
				}
				{
					PPTransaction tra(BIN(!(P.Flags & Param::fReportOnly)));
					THROW(tra);
					for(i = 0; i < ar_list.getCount(); i++) {
						const  PPID ar_id = ar_list.get(i);
						LAssoc p(ar_id, 0);
						uint   set_pos = 0, reset_pos = 0;
						const bool _set = set_stop_list.bsearch(&p, &set_pos, PTR_CMPFUNC(_2long));
						const bool _reset = reset_stop_list.bsearch(&p, &reset_pos, PTR_CMPFUNC(_2long));
						assert(_set != _reset);
						if(ArObj.GetPacket(ar_id, &pack) > 0) {
							int    do_turn_pack = 0;
							if(_set) {
								const StopItem * p_si = static_cast<const StopItem *>(set_stop_list.at(set_pos));
								if(!(pack.Rec.Flags & ARTRF_STOPBILL)) {
									pack.Rec.Flags |= ARTRF_STOPBILL;
									PPFormatT(PPTXT_LOG_DEBTRATE_SETSTOP, &msg_buf, ar_id, p_si->Debt, p_si->Rckn, p_si->MaxExpiry);
									logger.Log(msg_buf);
									do_turn_pack = 1;
								}
							}
							else {
								const StopItem * p_si = static_cast<const StopItem *>(reset_stop_list.at(reset_pos));
								if(pack.Rec.Flags & ARTRF_STOPBILL) {
									pack.Rec.Flags &= ~ARTRF_STOPBILL;
									PPFormatT(PPTXT_LOG_DEBTRATE_RESETSTOP, &msg_buf, ar_id, p_si->Debt, p_si->Rckn, p_si->MaxExpiry);
									logger.Log(msg_buf);
									do_turn_pack = 1;
								}
							}
							if(update_stop_debtdim && pack.P_CliAgt) {
								for(uint j = 0; j < debt_dim_list.getCount(); j++) {
									const  PPID dd_id = debt_dim_list.get(j);
									PPClientAgreement::DebtLimit * p_dl = pack.P_CliAgt->GetDebtDimEntry(dd_id);
									uint   dset_pos = 0;
									LAssoc p2(ar_id, dd_id);
									const int _cs = _set;
									const int _ds = BIN(set_stop_list.bsearch(&p2, &dset_pos, PTR_CMPFUNC(_2long)));
									const int _dx = BIN(p_dl);
									/*
										Матрица перехода для частного STOP'а
										COMM   (_cs) - рассчитанное значение общего STOP'а
										DIM    (_ds) - рассчитанное значение частного STOP'а
										DIMEXT (_dx) - признак наличия размерности в соглашении
										DIMEFF       - эффективное (устанавливаемое) значение частного STOP'а
										---------------------------------------
										_cs    _ds     _dx | DIMEFF
											0    0       0 |      x    // Ничего не делать
											0    0       1 |      0    // Следует отключить частный STOP в соглашении
											0    1       0 |      .    // Невозможный случай: нельзя вычислить частный STOP не имея параметров размерности
											0    1       1 |      1    // Следует включить частный STOP в соглашении
											1    0       0 |     ?1+   // Если fCreateLimitDebtDim && fProjCommStopToDebtDim, то следует создать размерность с признаком STOP, в противном случае - ничего не делать
											1    0       1 |     ?1    // Если fProjCommStopToDebtDim, то включить частный STOP в соглашении
											1    1       0 |      .    // Невозможный случай: нельзя вычислить частный STOP не имея параметров размерности
											1    1       1 |      1    // Следует включить частный STOP в соглашении
									*/
									if(_cs == 0) {
										if(_ds == 0) {
											if(_dx == 0) {
												; // Ничего не делать
											}
											else {
												// Следует отключить частный STOP в соглашении
												if(p_dl->Flags & p_dl->fStop) {
													p_dl->Flags &= ~p_dl->fStop;
													logger.Log(PPFormatT(PPTXT_LOG_DEBTRATE_DIMRESETSTOP, &msg_buf, ar_id, dd_id));
													do_turn_pack = 1;
												}
											}
										}
										else if(_dx == 0) {
											; // Невозможный случай: нельзя вычислить частный STOP не имея параметров размерности
										}
										else {
											// Следует включить частный STOP в соглашении
											if(!(p_dl->Flags & p_dl->fStop)) {
												p_dl->Flags |= p_dl->fStop;
												logger.Log(PPFormatT(PPTXT_LOG_DEBTRATE_DIMSETSTOP, &msg_buf, ar_id, dd_id));
												do_turn_pack = 1;
											}
										}
									}
									else {
										if(_ds == 0) {
											if(_dx == 0) {
												// Если fCreateLimitDebtDim && fProjCommStopToDebtDim, то следует создать
												// размерность с признаком STOP, в противном случае - ничего не делать
												if(P.Flags & P.fCreateLimitDebtDim && P.Flags & P.fProjCommStopToDebtDim) {
													PPClientAgreement::DebtLimit new_dl;
													MEMSZERO(new_dl);
													new_dl.DebtDimID = dd_id;
													new_dl.Flags |= new_dl.fStop;
													pack.P_CliAgt->DebtLimList.insert(&new_dl);
													logger.Log(PPFormatT(PPTXT_LOG_DEBTRATE_DIMCRSTOP, &msg_buf, ar_id, dd_id));
													do_turn_pack = 1;
												}
											}
											else {
												// Если fProjCommStopToDebtDim, то включить частный STOP в соглашении
												if(P.Flags & P.fProjCommStopToDebtDim) {
													if(!(p_dl->Flags & p_dl->fStop)) {
														p_dl->Flags |= p_dl->fStop;
														logger.Log(PPFormatT(PPTXT_LOG_DEBTRATE_DIMSETSTOP, &msg_buf, ar_id, dd_id));
														do_turn_pack = 1;
													}
												}
											}
										}
										else if(_dx == 0) {
											; // Невозможный случай: нельзя вычислить частный STOP не имея параметров размерности
										}
										else {
											// Следует включить частный STOP в соглашении
											if(!(p_dl->Flags & p_dl->fStop)) {
												p_dl->Flags |= p_dl->fStop;
												logger.Log(PPFormatT(PPTXT_LOG_DEBTRATE_DIMSETSTOP, &msg_buf, ar_id, dd_id));
												do_turn_pack = 1;
											}
										}
									}
								}
							}
							if(do_turn_pack && !(P.Flags & Param::fReportOnly)) {
								PPID   temp_id = ar_id;
								THROW(ArObj.PutPacket(&temp_id, &pack, 0));
							}
						}
						else
							logger.LogLastError();
						PPWaitPercent(i+1, ar_list.getCount());
					}
					/* @v7.1.2
					for(i = 0; set_stop_list.enumItems(&i, (void **)&p_si);) {
						if(ArObj.GetPacket(p_si->ArID, &pack) > 0) {
							if(!(pack.Rec.Flags & ARTRF_STOPBILL)) {
								pack.Rec.Flags |= ARTRF_STOPBILL;
								PPLoadText(PPTXT_LOG_DEBTRATE_SETSTOP, fmt_buf);
								PPFormat(fmt_buf, &msg_buf, p_si->ArID, p_si->Debt, p_si->Rckn, p_si->MaxExpiry);
								logger.Log(msg_buf);
								if(!(P.Flags & Param::fReportOnly)) {
									PPID   temp_id = p_si->ArID;
									THROW(ArObj.PutPacket(&temp_id, &pack, 0));
								}
							}
						}
						else
							logger.LogLastError();
						PPWaitPercent(i, set_stop_list.getCount());
					}
					for(i = 0; reset_stop_list.enumItems(&i, (void **)&p_si);) {
						if(ArObj.GetPacket(p_si->ArID, &pack) > 0) {
							if(pack.Rec.Flags & ARTRF_STOPBILL) {
								pack.Rec.Flags &= ~ARTRF_STOPBILL;
								PPLoadText(PPTXT_LOG_DEBTRATE_RESETSTOP, fmt_buf);
								PPFormat(fmt_buf, &msg_buf, p_si->ArID, p_si->Debt, p_si->Rckn, p_si->MaxExpiry);
								logger.Log(msg_buf);
								if(!(P.Flags & Param::fReportOnly)) {
									PPID   temp_id = p_si->ArID;
									THROW(ArObj.PutPacket(&temp_id, &pack, 0));
								}
							}
						}
						else
							logger.LogLastError();
						PPWaitPercent(i, reset_stop_list.getCount());
					}
					*/
					THROW(tra.Commit());
				}
			}
		}
	}
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	logger.Save(PPFILNAM_INFO_LOG, 0);
	PPWaitStop();
	return ok;
}

int DoDebtRate()
{
	int    ok = -1;
	PrcssrDebtRate::Param p;
	PrcssrDebtRate proc;
	proc.InitParam(&p);
	while(proc.EditParam(&p) > 0) {
		if(!proc.Init(&p))
			PPError();
		if(proc.Run()) {
			ok = 1;
			break;
		}
	}
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(DebtorStat); DebtorStatFilt::DebtorStatFilt() : PPBaseFilt(PPFILT_DEBTORSTAT, 0, 0)
{
#define _S_ DebtorStatFilt
	SetFlatChunk(offsetof(_S_, ReserveStart), offsetof(_S_, ArList)-offsetof(_S_, ReserveStart));
	SetBranchObjIdListFilt(offsetof(_S_, ArList));
#undef _S_
	Init(1, 0);
}
//
//
//
PPViewDebtorStat::PPViewDebtorStat() : PPView(0, &Filt, PPVIEW_DEBTORSTAT, 0, REPORT_DEBTORSTAT), P_TempTbl(0), LastDate(ZERODATE)
{
}

PPViewDebtorStat::~PPViewDebtorStat()
{
	delete P_TempTbl;
}

PPBaseFilt * PPViewDebtorStat::CreateFilt(const void * extraPtr) const
{
	DebtorStatFilt * p_filt = new DebtorStatFilt;
	p_filt->AccSheetID = GetSellAccSheet();
	p_filt->Order = DebtorStatFilt::ordByArName;
	p_filt->Flags |= (DebtorStatFilt::fIncludeTotal|DebtorStatFilt::fIncludeRel|DebtorStatFilt::fIncludeAgent|DebtorStatFilt::fIncludeTerm);
	return p_filt;
}

class DebtorStatFiltDialog : public TDialog {
	DECL_DIALOG_DATA(DebtorStatFilt);
public:
	DebtorStatFiltDialog() : TDialog(DLG_DEBTSTATFILT)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SetupPPObjCombo(this, CTLSEL_DEBTSTATFILT_ACS, PPOBJ_ACCSHEET, Data.AccSheetID, 0, 0);
		AddClusterAssoc(CTL_DEBTSTATFILT_ORDER, 0, DebtorStatFilt::ordByArName);
		AddClusterAssoc(CTL_DEBTSTATFILT_ORDER, -1, DebtorStatFilt::ordByDefault);
		AddClusterAssoc(CTL_DEBTSTATFILT_ORDER, 1, DebtorStatFilt::ordByDelayMean);
		AddClusterAssoc(CTL_DEBTSTATFILT_ORDER, 2, DebtorStatFilt::ordByDelaySd);
		AddClusterAssoc(CTL_DEBTSTATFILT_ORDER, 3, DebtorStatFilt::ordByDelayGammaTest);
		AddClusterAssoc(CTL_DEBTSTATFILT_ORDER, 4, DebtorStatFilt::ordByPaymDensity);
		AddClusterAssoc(CTL_DEBTSTATFILT_ORDER, 5, DebtorStatFilt::ordByPaymPeriod);
		AddClusterAssoc(CTL_DEBTSTATFILT_ORDER, 6, DebtorStatFilt::ordByRating);
		SetClusterData(CTL_DEBTSTATFILT_ORDER, Data.Order);

		AddClusterAssoc(CTL_DEBTSTATFILT_INCL, 0, DebtorStatFilt::fIncludeTotal);
		AddClusterAssoc(CTL_DEBTSTATFILT_INCL, 1, DebtorStatFilt::fIncludeRel);
		AddClusterAssoc(CTL_DEBTSTATFILT_INCL, 2, DebtorStatFilt::fIncludeAgent);
		AddClusterAssoc(CTL_DEBTSTATFILT_INCL, 3, DebtorStatFilt::fIncludeTerm);
		SetClusterData(CTL_DEBTSTATFILT_INCL, Data.Flags);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		getCtrlData(CTLSEL_DEBTSTATFILT_ACS, &Data.AccSheetID);
		GetClusterData(CTL_DEBTSTATFILT_ORDER, &Data.Order);
		GetClusterData(CTL_DEBTSTATFILT_INCL, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		return 1;
	}
};

int PPViewDebtorStat::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	return Filt.IsA(pBaseFilt) ? PPDialogProcBody <DebtorStatFiltDialog, DebtorStatFilt> (static_cast<DebtorStatFilt *>(pBaseFilt)) : 0;
}

void PPViewDebtorStat::MakeTempRec(long order, const DebtorStatViewItem * pItem, TempOrderTbl::Rec * pRec)
{
	SString temp_buf, ar_name;
	GetArticleName(pItem->ArID, ar_name);
	if(pItem->ArID == 0)
		temp_buf.CatChar('0');
	else if(pItem->Flags & PPDebtorStat::fAgent)
		temp_buf.CatChar('A');
	else if(pItem->Flags & PPDebtorStat::fHolding)
		temp_buf.CatChar('H');
	else
		temp_buf.CatChar('Z');
	temp_buf.Space();
	switch(order) {
		case DebtorStatFilt::ordByArName: temp_buf.Cat(ar_name); break;
		case DebtorStatFilt::ordByDelayMean: temp_buf.Cat(pItem->DelayMean, MKSFMTD(16, 5, 0)).Cat(ar_name); break;
		case DebtorStatFilt::ordByDelaySd: temp_buf.Cat(pItem->DelaySd, MKSFMTD(16, 5, 0)).Cat(ar_name); break;
		case DebtorStatFilt::ordByDelayGammaTest: temp_buf.Cat(pItem->DelayTestGamma, MKSFMTD(16, 5, 0)).Cat(ar_name); break;
		case DebtorStatFilt::ordByPaymPeriod: temp_buf.CatLongZ(pItem->PaymPeriod, 6).Cat(ar_name); break;
		case DebtorStatFilt::ordByPaymDensity: temp_buf.Cat(pItem->PaymDensity, MKSFMTD(16, 5, 0)).Cat(ar_name); break;
		case DebtorStatFilt::ordByRating: temp_buf.Cat(pItem->Rating).Cat(ar_name); break;
		default: temp_buf.Cat(ar_name); break;
	}
	pRec->ID = pItem->ArID;
	temp_buf.CopyTo(pRec->Name, sizeof(pRec->Name));
}

int PPViewDebtorStat::UpdateTempTable()
{
	int    ok = 1;
	TempOrderTbl * p_o = P_TempTbl;
	P_TempTbl = 0;
	if(p_o) {
		DebtorStatViewItem item;
		BExtInsert bei(p_o);
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		THROW_DB(deleteFrom(p_o, 0, *reinterpret_cast<DBQ *>(0)));
		for(InitIteration(0); NextIteration(&item) > 0;) {
			TempOrderTbl::Rec temp_rec;
			MakeTempRec(Filt.Order, &item, &temp_rec);
			THROW_DB(bei.insert(&temp_rec));
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
		P_TempTbl = p_o;
	}
	CATCH
		ZDELETE(p_o);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewDebtorStat::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	PPLicData ld;
	THROW_PP(DS.CheckExtFlag(ECF_OPENSOURCE) || (PPGetLicData(&ld) > 0 && ld.ExtFunc & PPLicData::effDebtorStat), PPERR_NOLIC_DEBTORSTAT);
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempTbl);
	Tbl.GetLastDate(Filt.AccSheetID, &LastDate);
	if(Filt.Order) {
		THROW(P_TempTbl = CreateTempOrderFile());
		THROW(UpdateTempTable());
	}
	CATCHZOK
	return ok;
}

int PPViewDebtorStat::InitIteration(long ord)
{
	int    ok = -1;
	BExtQuery * q = 0;
	DBQ  * dbq = 0;
	Counter.Init(0UL);
	BExtQuery::ZDelete(&P_IterQuery);
	if(P_TempTbl) {
		TempOrderTbl::Key1 ord_k1;
		THROW_MEM(q = new BExtQuery(P_TempTbl, 1));
		q->selectAll();
		MEMSZERO(ord_k1);
		Counter.Init(q->countIterations(0, &ord_k1, spFirst));
		MEMSZERO(ord_k1);
		q->initIteration(false, &ord_k1, spFirst);
	}
	else {
		DebtStatTbl::Key0 k0, k0_;
		THROW_MEM(q = new BExtQuery(&Tbl, 0));
		q->selectAll();
		dbq = & (Tbl.Dt == LastDate && Tbl.AccSheetID == Filt.AccSheetID);
		if(!(Filt.Flags & DebtorStatFilt::fIncludeTotal))
			dbq = &(*dbq && Tbl.ArID > 0L);
		q->selectAll().where(*dbq);
		MEMSZERO(k0);
		k0.Dt = LastDate;
		k0.AccSheetID = Filt.AccSheetID;
		k0_ = k0;
		Counter.Init(q->countIterations(0, &k0_, spGe));
		q->initIteration(false, &k0, spGe);
	}
	P_IterQuery = q;
	CATCH
		ZDELETE(q);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewDebtorStat::MakeViewItem(const DebtStatTbl::Rec * pRec, DebtorStatViewItem * pItem)
{
	if(pItem) {
		pItem->ArID = pRec->ArID;
		pItem->RelArID = pRec->RelArID;
		pItem->Dtm.Set(pRec->Dt, pRec->Tm);
		pItem->Flags = pRec->Flags;
		pItem->DelayMean = pRec->DelayMean;
		pItem->DelaySd = pRec->DelaySd;
		pItem->DelayTestGamma = pRec->DelayTestGamma;
		pItem->DelayTestChSq = pRec->DelayTestChSq;
		pItem->ExpiryMean = pRec->ExpiryMean;
		pItem->PaymPeriod = pRec->PaymPeriod;
		pItem->PaymDensity = pRec->PaymDensity;
		pItem->SigmFactor = pRec->SigmFactor;
		pItem->Limit = pRec->CreditLimit;
		pItem->LimitTerm = pRec->LimitTerm;
		pItem->DebtCost = pRec->DebtCost;
		pItem->Rating = pRec->Rating;
		GetArticleName(pItem->ArID, pItem->ArName);
	}
	return 1;
}

int FASTCALL PPViewDebtorStat::CheckForFilt(const DebtStatTbl::Rec & rRec) const
{
	const long ff = Filt.Flags;
	const long f = rRec.Flags;
	if(!(ff & DebtorStatFilt::fIncludeTotal) && rRec.ArID == 0)
		return 0;
	else if(!(ff & DebtorStatFilt::fIncludeRel) && f & PPDebtorStat::fHolding)
		return 0;
	else if(!(ff & DebtorStatFilt::fIncludeAgent) && f & PPDebtorStat::fAgent)
		return 0;
	else if(!(ff & DebtorStatFilt::fIncludeTerm) && !(f & (PPDebtorStat::fHolding|PPDebtorStat::fAgent)) && rRec.ArID != 0)
		return 0;
	else
		return 1;
}

int FASTCALL PPViewDebtorStat::NextIteration(DebtorStatViewItem * pItem)
{
	while(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		Counter.Increment();
		if(P_TempTbl) {
			PPID ar_id = P_TempTbl->data.ID;
			DebtStatTbl::Key0 k0;
			MEMSZERO(k0);
			k0.Dt = LastDate;
			k0.AccSheetID = Filt.AccSheetID;
			k0.ArID = ar_id;
			if(Tbl.search(0, &k0, spEq)) {
				MakeViewItem(&Tbl.data, pItem);
				return 1;
			}
		}
		else {
			if(CheckForFilt(Tbl.data)) {
				MakeViewItem(&Tbl.data, pItem);
				return 1;
			}
		}
	}
	return -1;
}

static IMPL_CMPFUNC(_2double, i1, i2)
{
	struct I {
		double x;
		double y;
	};
	RET_CMPCASCADE2(static_cast<const I *>(i1), static_cast<const I *>(i2), x, y);
}

int PPViewDebtorStat::ViewGraph(const PPViewBrowser * pBrw)
{
	int    ok = -1;
	uint   i;
	const  int col = pBrw ? pBrw->GetCurColumn() : -1;
	SString temp_buf, fmt_buf;
	PPDebtorStatArray list;
	Generator_GnuPlot plot(0);
	Generator_GnuPlot::PlotParam param;
	TSVector <SPoint3R> matrix;
	PPDebtorStatArray::Total total;
	if(col == 8) { // sigm factor
		if(Tbl.GetList(Filt.AccSheetID, list) > 0) {
			THROW(list.CalcRating(&total, PPDebtorStatArray::omSigmFactor, &matrix));
			plot.Preamble();
			plot.SetGrid();
			PPGetSubStr(PPTXT_PLOT_DSTAT_SIGMF, 0, fmt_buf);
			plot.SetTitle(temp_buf.Printf(fmt_buf, total.SigmFactor).Transf(CTRANSF_INNER_TO_OUTER));
			PPGetSubStr(PPTXT_PLOT_DSTAT_SIGMF, 1, temp_buf);
			plot.SetAxisTitle(Generator_GnuPlot::axX, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
			PPGetSubStr(PPTXT_PLOT_DSTAT_SIGMF, 2, temp_buf);
			plot.SetAxisTitle(Generator_GnuPlot::axY, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
			{
				int axis = Generator_GnuPlot::axX;
				plot.UnsetTics(axis);

				Generator_GnuPlot::StyleTics tics;
				tics.Rotate = 0;
				tics.Font.Size = 8;
				PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIALCYR, tics.Font.Face);
				plot.SetTics(axis, &tics);

				PPGpTicsList tics_list(0);
				PPLoadTextWin(PPTXT_PLOT_DSTAT_AVGPAYMPERIOD, temp_buf);
				tics_list.Add(total.PaymPeriodMean, temp_buf);
				plot.SetTicsList(axis, tics_list);
			}
			{
				int axis = Generator_GnuPlot::axY;
				plot.UnsetTics(axis);

				Generator_GnuPlot::StyleTics tics;
				tics.Rotate = 0;
				tics.Font.Size = 8;
				PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIALCYR, tics.Font.Face);
				plot.SetTics(axis, &tics);

				plot.SetAxisRange(axis, -0.1, 1.1);

				PPGpTicsList tics_list(0);
				tics_list.Add(-0.1, 0);
				tics_list.Add(0.0, 0);
				tics_list.Add(0.5, 0);
				tics_list.Add(1.0, 0);
				tics_list.Add(1.1, 0);
				plot.SetTicsList(axis, tics_list);
			}
			{
				PPGpPlotItem item(plot.GetDataFileName(), 0, PPGpPlotItem::sPoints);
				item.Style.SetPoint(GetColorRef(SClrBlueviolet), PPGpStyle::ptCircle, 0.25);
				item.AddDataIndex(1);
				item.AddDataIndex(2);
				plot.AddPlotItem(item);
			}
			plot.Plot(&param);
			plot.StartData(1);
			for(i = 0; i < matrix.getCount(); i++) {
				SPoint3R p = matrix.at(i);
				plot.PutData(p.x);
				plot.PutData(p.y);
				plot.PutEOR();
			}
			plot.PutEndOfData();
			ok = plot.Run();
		}
	}
	else if(col == 9) { // rating
		if(Tbl.GetList(Filt.AccSheetID, list) > 0) {
			THROW(list.CalcRating(&total, PPDebtorStatArray::omRatingData, &matrix));
			param.Flags |= (Generator_GnuPlot::PlotParam::fPoints);
			plot.Preamble();
			plot.SetGrid();
			PPGetSubStr(PPTXT_PLOT_DSTAT_RATING, 0, temp_buf);
			plot.SetTitle(temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
			PPGetSubStr(PPTXT_PLOT_DSTAT_RATING, 2, temp_buf);
			plot.SetAxisTitle(Generator_GnuPlot::axX, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
			PPGetSubStr(PPTXT_PLOT_DSTAT_RATING, 3, temp_buf);
			plot.SetAxisTitle(Generator_GnuPlot::axY, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
			{
				int axis = Generator_GnuPlot::axX;
				plot.UnsetTics(axis);

				Generator_GnuPlot::StyleTics tics;
				tics.Rotate = 0;
				tics.Font.Size = 8;
				PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIALCYR, tics.Font.Face);
				plot.SetTics(axis, &tics);

				double m = total.PaymRatingMean;
				double b = total.PaymRatingBar;
				PPGpTicsList tics_list(0);
				//tics_list.Add(m, "Среднее");
				tics_list.Add(m - b/2 - b, "D");
				tics_list.Add(m - b/2, "C");
				tics_list.Add(m + b/2, "B");
				tics_list.Add(m + b/2 + b, "A");
				plot.SetTicsList(axis, tics_list);
			}
			{
				int axis = Generator_GnuPlot::axY;
				plot.UnsetTics(axis);

				Generator_GnuPlot::StyleTics tics;
				tics.Rotate = 0;
				tics.Font.Size = 8;
				PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIALCYR, tics.Font.Face);
				plot.SetTics(axis, &tics);

				double m = total.DelayRatingMean;
				double b = total.DelayRatingBar;
				PPGpTicsList tics_list(0);
				//tics_list.Add(m, "Среднее");
				tics_list.Add(m - b/2 - b, "B");
				tics_list.Add(m - b/2, "C");
				tics_list.Add(m + b/2, "D");
				tics_list.Add(m + b/2 + b, "E");
				plot.SetTicsList(axis, tics_list);
			}
			{
				PPGpPlotItem item(plot.GetDataFileName(), 0, PPGpPlotItem::sPoints);
				item.Style.SetPoint(GetColorRef(SClrBlue), PPGpStyle::ptCircle, 0.25);
				item.AddDataIndex(1);
				item.AddDataIndex(2);
				// item.AddDataIndex(3); // color
				plot.AddPlotItem(item);
			}
			plot.Plot(&param);
			plot.StartData(1);
			for(i = 0; i < matrix.getCount(); i++) {
				SPoint3R p = matrix.at(i);
				plot.PutData(p.x);
				plot.PutData(p.y);
				COLORREF c = (COLORREF)(ulong)p.z;
				temp_buf.Printf("0x%02x%02x%02x", GetRValue(c), GetGValue(c), GetBValue(c));
				plot.PutData(temp_buf, 1);
				plot.PutEOR();
			}
			plot.PutEndOfData();
			ok = plot.Run();
		}
	}
	else if(oneof3(col, 2, 3, 5)) {
		if(Tbl.GetList(Filt.AccSheetID, list) > 0) {
			THROW(list.CalcRating(0, PPDebtorStatArray::omDelayExpiryDots, &matrix));
			param.Flags |= (Generator_GnuPlot::PlotParam::fPoints | Generator_GnuPlot::PlotParam::fStereo);
			plot.Preamble();
			plot.SetGrid();
			PPGetSubStr(PPTXT_PLOT_DSTAT_DELAY, 0, temp_buf);
			plot.SetTitle(temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
			PPGetSubStr(PPTXT_PLOT_DSTAT_DELAY, 1, temp_buf);
			plot.SetAxisTitle(Generator_GnuPlot::axX, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
			PPGetSubStr(PPTXT_PLOT_DSTAT_DELAY, 2, temp_buf);
			plot.SetAxisTitle(Generator_GnuPlot::axY, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
			PPGetSubStr(PPTXT_PLOT_DSTAT_DELAY, 3, temp_buf);
			plot.SetAxisTitle(Generator_GnuPlot::axZ, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
			{
				PPGpPlotItem item(plot.GetDataFileName(), 0, PPGpPlotItem::sPoints);
				item.Style.SetPoint(GetColorRef(SClrBlue), PPGpStyle::ptCircle, 0.5);
				item.AddDataIndex(1);
				item.AddDataIndex(2);
				item.AddDataIndex(3);
				plot.AddPlotItem(item);
			}
			plot.Plot(&param);
			plot.StartData(1);
			for(i = 0; i < matrix.getCount(); i++) {
				SPoint3R p = matrix.at(i);
				plot.PutData(p.x);
				plot.PutData(p.y);
				plot.PutData(p.z);
				plot.PutEOR();
			}
			plot.PutEndOfData();
			ok = plot.Run();
		}
	}
	else {
		if(Tbl.GetList(Filt.AccSheetID, list) > 0) {
			THROW(list.CalcRating(0, PPDebtorStatArray::omLimitByPaymDensity, &matrix));
			param.Flags |= (Generator_GnuPlot::PlotParam::fDots | Generator_GnuPlot::PlotParam::fPm3D |
				Generator_GnuPlot::PlotParam::fDGrid3D | Generator_GnuPlot::PlotParam::fStereo);
			plot.Preamble();
			plot.SetGrid();
			plot.Plot(&param);
			plot.StartData(1);
			matrix.sort(PTR_CMPFUNC(_2double));
			const uint quant = 3;
			i = 1;
			double x = SMathConst::Max;
			for(; i <= matrix.getCount(); i++) {
				SPoint3R p = matrix.at(i-1);
				if(x == SMathConst::Max)
					x = p.x;
				plot.PutData(x);
				plot.PutData(p.y);
				plot.PutData(p.z);
				plot.PutEOR();
				if(i && i % quant == 0) {
					plot.PutEOR();
					x = SMathConst::Max;
				}
			}
			plot.PutEndOfData();
			ok = plot.Run();
		}
	}
	CATCHZOKPPERR
	return ok;
}

DBQuery * PPViewDebtorStat::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_DEBTORSTAT;
	DBQuery * q = 0;
	DBQ  * dbq = 0;
	DBE    dbe_ar;
	DebtStatTbl * tbl = 0;
	TempOrderTbl * p_ot = 0;
	THROW(CheckTblPtr(tbl = new DebtStatTbl));
	if(P_TempTbl) {
		THROW(CheckTblPtr(p_ot = new TempOrderTbl(P_TempTbl->GetName())));
		dbq = &(*dbq && tbl->ArID == p_ot->ID && tbl->Dt == LastDate && tbl->AccSheetID == Filt.AccSheetID);
	}
	else {
		dbq = &(*dbq && tbl->Dt == LastDate && tbl->AccSheetID == Filt.AccSheetID);
	}
	CALLPTRMEMB(pSubTitle, Z());
	PPDbqFuncPool::InitObjNameFunc(dbe_ar, PPDbqFuncPool::IdObjNameAr, tbl->ArID);
	q = &select(
		tbl->ArID,           // #0
		tbl->Dt,             // #1
		tbl->DelayMean,      // #2
		tbl->DelaySd,        // #3
		tbl->DelayTestGamma, // #4
		tbl->PaymDensity,    // #5
		tbl->PaymPeriod,     // #6
		tbl->Rating,         // #7
		tbl->CreditLimit,    // #8
		tbl->DebtCost,       // #9
		tbl->ExpiryMean,     // #10
		tbl->SigmFactor,     // #11
		tbl->Flags,          // #12
		dbe_ar,              // #13
		0);
	if(p_ot)
		q->from(p_ot, tbl, 0L).where(*dbq).orderBy(p_ot->Name, 0L);
	else
		q->from(tbl, 0L).where(*dbq);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete tbl;
			delete p_ot;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, void * extraPtr)
{
	int    ok = -1;
	if(pData && pCellStyle && paintAction == BrowserWindow::paintNormal) {
		struct tag_Item {
			PPID   ArID;
			LDATE  Dt;
			double DelayMean;
			double DelaySd;
			double DelayTestGamma;
			double PaymDensity;
			long   PaymPeriod;
			char   Rating[12];
			double Limit;
			double DebtCost;
			double ExpiryMean;
			double SigmFactor;
			long   Flags;
		} * p_item = (tag_Item *)pData;
		if(col == 9) {
			COLORREF c;
			if(GetDebtorRatingColor(p_item->Rating, &c) > 0) {
				pCellStyle->Flags = 0;
				pCellStyle->Color = c;
				ok = 1;
			}
		}
		else if(col == 0) {
			if(p_item->Flags & PPDebtorStat::fAgent) {
				pCellStyle->Color = LightenColor(GetColorRef(SClrBlue), 0.7f);
				pCellStyle->Flags = 0;
				ok = 1;
			}
			else if(p_item->Flags & PPDebtorStat::fHolding) {
				pCellStyle->Color = LightenColor(GetColorRef(SClrBlue), 0.9f);
				pCellStyle->Flags = 0;
				ok = 1;
			}
		}
	}
	return ok;
}

void PPViewDebtorStat::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, SetCellStyleFunc(CellStyleFunc, 0));
}

int PPViewDebtorStat::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   ar_id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_EDITARTICLE:
				ok = -1;
				if(ar_id) {
					PPObjArticle ar_obj;
					if(ar_obj.Edit(&ar_id, 0) == cmOK)
						ok = 1;
				}
				break;
			case PPVCMD_EDITPERSON:
				ok = -1;
				if(ar_id) {
					PPObjArticle ar_obj;
					PPID   psn_id = ObjectToPerson(ar_id);
					if(psn_id) {
						PPObjPerson psn_obj;
						if(psn_obj.Edit(&psn_id, 0) == cmOK)
							ok = 1;
					}
				}
				break;
			case PPVCMD_CONFIG:
				ok = -1;
				if(PPDebtorStatConfig::Edit() > 0) {
					PPDebtorStatArray list;
					PPWaitStart();
					if(Tbl.GetList(Filt.AccSheetID, list) > 0) {
						if(list.CalcRating(0) && Tbl.SetList(Filt.AccSheetID, LastDate, list, 1)) {
							UpdateTempTable();
							ok = 1;
						}
						else
							ok = PPErrorZ();
					}
					PPWaitStop();
				}
				break;
			case PPVCMD_GRAPH:
				ok = ViewGraph(pBrw);
				break;
		}
	}
	return ok;
}
//
// Implementation of PPALDD_DebtorStat
//
PPALDD_CONSTRUCTOR(DebtorStat)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(DebtorStat) { Destroy(); }

int PPALDD_DebtorStat::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(DebtorStat, rsrv);
	H.AccSheetID = p_filt->AccSheetID;
	PPDebtorStatConfig cfg;
	PPDebtorStatConfig::Read(&cfg);
	H.LastDate = cfg.LastDtm.d;
	H.LastTime = cfg.LastDtm.t;
	SString temp_buf;
	temp_buf.Cat(cfg.LastDtm).CopyTo(H.LastTimeText, sizeof(H.LastTimeText));
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_DebtorStat::InitIteration(long iterId, int sortId, long rsrv)
{
	PPViewDebtorStat * p_v = static_cast<PPViewDebtorStat *>(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	return BIN(p_v->InitIteration(static_cast<PPViewDebtTrnovr::IterOrder>(SortIdx)));
}

int PPALDD_DebtorStat::NextIteration(long iterId)
{
	START_PPVIEW_ALDD_ITER(DebtorStat);
#define FLD(f) I.f = item.f
	FLD(ArID);
	I.Dt = item.Dtm.d;
	I.Tm = item.Dtm.t;
	FLD(Flags);
	FLD(DelayMean);
	FLD(DelaySd);
	FLD(DelayTestGamma);
	FLD(DelayTestChSq);
	FLD(PaymPeriod);
	FLD(PaymDensity);
	FLD(Limit);
	FLD(DebtCost);
	FLD(ExpiryMean);
	FLD(SigmFactor);
	FLD(LimitTerm);
#undef FLD
	STRNSCPY(I.Rating, item.Rating);
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_DebtorStat::Destroy() { DESTROY_PPVIEW_ALDD(DebtorStat); }
