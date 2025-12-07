// V_PAYPLN.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2015, 2016, 2017, 2018, 2019, 2020, 2022, 2024, 2025
// @codepage UTF-8
// Plan of Payments View
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(PaymPlan); PaymPlanFilt::PaymPlanFilt() : PPBaseFilt(PPFILT_PAYMPLAN, 0, 1)
{
	SetFlatChunk(offsetof(PaymPlanFilt, ReserveStart),
		offsetof(PaymPlanFilt, Reserve)-offsetof(PaymPlanFilt, ReserveStart)+sizeof(Reserve));
	SetBranchObjIdListFilt(offsetof(PaymPlanFilt, LocList));
	Init(1, 0);
}


PPViewPaymPlan::PPViewPaymPlan() : PPView(0, &Filt, PPVIEW_PAYMPLAN, 0, 0), P_BObj(BillObj), P_TempTbl(0)
{
}

PPViewPaymPlan::~PPViewPaymPlan()
{
	ZDELETE(P_TempTbl);
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempPaymPlan);

static int IterProc_Count(PayPlanTbl::Rec * pRec, void * extraPtr)
{
	long * p_count = static_cast<long *>(extraPtr);
	(*p_count)++;
	return 1;
}

struct IterProcParam_ProcessPeriod {
	IterProcParam_ProcessPeriod(PPViewPaymPlan * pView, DateRange & rPeriod, IterCounter * pCntr) :
		P_V(pView), P_BillList(0), P_PaymList(0), Period(rPeriod), FiltObjID(0), P_Cntr(pCntr)
	{
	}
	IterProcParam_ProcessPeriod(PPViewPaymPlan * pView, ObjIdListFilt * pBillList, RPairArray * pPaymList, DateRange & rPeriod, PPID filtObjID) :
		P_V(pView), P_BillList(pBillList), P_PaymList(pPaymList), Period(rPeriod), FiltObjID(filtObjID), P_Cntr(0)
	{
	}
	struct BillPaymItem {
		PPID   BillID;
		double Paym;
		LDATE  LastDate;
	};
	PPViewPaymPlan * P_V;
	DateRange Period;
	IterCounter * P_Cntr;
	//
	// Если P_BillList != 0, то функция перечисляет документы и складывает те,
	// что подходят под фильтрацию по Period и FiltObjID в этот список.
	//
	ObjIdListFilt * P_BillList; // @notowned
	RPairArray    * P_PaymList; // @notowned
	TSVector <BillPaymItem> BillPaymList; // Список учтенных платежей по документам
	PPID   FiltObjID;
};

int IterProc_PPViewPaymPlan_ProcessPeriod(PayPlanTbl::Rec * pRec, void * extraPtr)
{
	int    ok = 1;
	IterProcParam_ProcessPeriod * p = static_cast<IterProcParam_ProcessPeriod *>(extraPtr);
	PPViewPaymPlan * p_v = p->P_V;
	BillTbl::Rec bill_rec;
	if(p->P_Cntr)
		PPWaitPercent(p->P_Cntr->Increment());
	if(p_v->P_BObj->Search(pRec->BillID, &bill_rec) > 0) {
		if(!p_v->Filt.LocList.CheckID(bill_rec.LocID))
			return 1;
		else if(!p_v->Filt.ObjectID) {
			ArticleTbl::Rec ar_rec;
			if(p_v->ArObj.Fetch(bill_rec.Object, &ar_rec) > 0)
				if(ar_rec.AccSheetID != p_v->Filt.AccSheetID)
					return 1;
		}
		else if(bill_rec.Object != p_v->Filt.ObjectID)
			return 1;
		if(p_v->Filt.AgentID) {
			PPBillExt bext;
			if(p_v->P_BObj->FetchExt(bill_rec.ID, &bext) < 0 || bext.AgentID != p_v->Filt.AgentID)
				return 1;
		}
		double amt = BR2(bill_rec.Amount);
		double paym = 0.0;
		double debt = 0.0;
		{
			uint pos = 0;
			const LDATE last_date = (p_v->Filt.Flags & PaymPlanFilt::fCurrentDebt) ? ZERODATE : plusdate(p->Period.low, -1);
			if(!p->BillPaymList.lsearch(&bill_rec.ID, &pos, CMPF_LONG)) {
				DateRange period;
				period.Set(ZERODATE, last_date);
				p_v->P_BObj->P_Tbl->CalcPayment(bill_rec.ID, 1, &period, bill_rec.CurID, &paym);
				pos = p->BillPaymList.getCount();
				IterProcParam_ProcessPeriod::BillPaymItem bpi;
				bpi.BillID = bill_rec.ID;
				bpi.Paym = paym;
				bpi.LastDate = last_date;
				p->BillPaymList.insert(&bpi);
			}
			else {
				IterProcParam_ProcessPeriod::BillPaymItem & r_bpi = p->BillPaymList.at(pos);
				paym = r_bpi.Paym;
				if(r_bpi.LastDate) {
					DateRange period = p->Period;
					period.Set(plusdate(r_bpi.LastDate, 1), last_date);
					double added_paym = 0.0;
					p_v->P_BObj->P_Tbl->CalcPayment(bill_rec.ID, 1, &period, bill_rec.CurID, &added_paym);
					r_bpi.Paym += added_paym;
					r_bpi.LastDate = last_date;
				}
			}
			if(paym >= pRec->Amount) {
				debt = 0.0;
				paym -= pRec->Amount;
				p->BillPaymList.at(pos).Paym = paym;
			}
			else {
				debt = pRec->Amount - paym;
				p->BillPaymList.at(pos).Paym = paym;
			}
		}
		if(debt > 0.0) {
			TempPaymPlanTbl::Rec rec;
			TempPaymPlanTbl::Key0 pk0;
			rec.Dt     = p->Period.low;
			rec.PayPlanDt = pRec->PayDate;
			rec.BillID = 0;
			PPID   psn_id = 0;
			if(p_v->Filt.Flags & PaymPlanFilt::fDetail) {
				if(p_v->Filt.Sgp) {
					PPID   _psn_id = 0;
					PPID   dlvr_loc_id = 0;
					PPFreight freight;
					if(p_v->P_BObj->P_Tbl->GetFreight(bill_rec.ID, &freight) > 0)
						dlvr_loc_id = freight.DlvrAddrID__;
					else
						MEMSZERO(freight);
					if(p_v->Filt.Sgp == sgpBillAgent) {
						PPBillExt billext_rec;
						if(p_v->P_BObj->FetchExt(bill_rec.ID, &billext_rec) > 0 && billext_rec.AgentID)
							_psn_id = ObjectToPerson(billext_rec.AgentID);
					}
					else if(p_v->Filt.Sgp == sgpVesselAgent) {
						_psn_id = ObjectToPerson(freight.AgentID);
					}
					else {
						p_v->PsnObj.Subst((bill_rec.Object | sgpArticleMask), dlvr_loc_id, &p_v->Psp, PSNSUBSTF_LASTRELINHIERARH, &_psn_id);
					}
					rec.ObjID = _psn_id;
				}
				else {
					psn_id = ObjectToPerson(bill_rec.Object);
					if(psn_id)
						rec.ObjID = psn_id;
					else
						rec.ObjID = (bill_rec.Object | sgpArticleMask);
				}
			}
			if(p->P_PaymList && (!p->FiltObjID || rec.ObjID == p->FiltObjID)) {
				RPair paym_item;
				paym_item.X = amt;
				paym_item.Y = paym;
				p->P_PaymList->insert(&paym_item);
			}
			if(p->P_BillList) {
				if(!p->FiltObjID || rec.ObjID == p->FiltObjID) {
					p->P_BillList->Add(bill_rec.ID);
				}
			}
			else {
				rec.Amount = amt;
				//rec.Interest = 0;
				rec.Paym   = paym;
				rec.Debt   = debt;
				pk0.Dt     = rec.Dt;
				pk0.ObjID  = rec.ObjID;
				pk0.BillID = rec.BillID;
				if(p_v->P_TempTbl->searchForUpdate(0, &pk0, spEq)) {
					p_v->P_TempTbl->data.Amount   += rec.Amount;
					//p->P_TempTbl->data.Interest += rec.Interest;
					p_v->P_TempTbl->data.Paym     += rec.Paym;
					p_v->P_TempTbl->data.Debt     += rec.Debt;
					THROW_DB(p_v->P_TempTbl->updateRec()); // @sfu
				}
				else {
					if(rec.ObjID) {
						SString temp_buf;
						p_v->PsnObj.GetSubstText(rec.ObjID, 0, &p_v->Psp, temp_buf);
						STRNSCPY(rec.Name, temp_buf);
					}
					THROW_DB(p_v->P_TempTbl->insertRecBuf(&rec));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPViewPaymPlan::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt) > 0);
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_Ct);
	MEMSZERO(Total);
	CycleList.init2(&Filt.Period, &Filt.Cycl);
	if(Filt.LocList.GetCount()) {
		PPIDArray loc_list;
		PPObjLocation loc_obj;
		THROW(loc_obj.ResolveWarehouseList(&Filt.LocList.Get(), loc_list));
		Filt.LocList.Set(&loc_list);
	}
	else
		Filt.LocList.Set(0);
	{
		ZDELETE(P_TempTbl);
		Psp.Init(Filt.Sgp);
		IterCounter cntr;
		long   count = 0;
		TSVector <DateRange> period_list;
		THROW(P_BObj->P_Tbl->PayPlanEnumerator(&Filt.Period, IterProc_Count, &count));
		cntr.Init(count);
		THROW(P_TempTbl = CreateTempFile());
		if(!Filt.Cycl)
			period_list.insert(&Filt.Period);
		else
			for(uint cycle_no = 0; cycle_no < CycleList.getCount(); cycle_no++) {
				period_list.insert(&CycleList.at(cycle_no));
			}
		if(period_list.getCount()) {
			PPTransaction tra(ppDbDependTransaction, 1);
			IterProcParam_ProcessPeriod ipp(this, Filt.Period, &cntr);
			THROW(tra);
			for(uint i = 0; i < period_list.getCount(); i++) {
				ipp.Period = period_list.at(i);
				THROW(P_BObj->P_Tbl->PayPlanEnumerator(&period_list.at(i), IterProc_PPViewPaymPlan_ProcessPeriod, &ipp));
			}
			THROW(tra.Commit());
		}
		{
			MEMSZERO(Total);
			BExtQuery qt(P_TempTbl, 0);
			TempPaymPlanTbl::Key0 tk0;
			qt.select(P_TempTbl->ObjID, P_TempTbl->Amount, P_TempTbl->Debt, 0L);
			MEMSZERO(tk0);
			for(qt.initIteration(false, &tk0, spFirst); qt.nextIteration() > 0;) {
				Total.Count++;
				Total.Amount += P_TempTbl->data.Amount;
				Total.Paym   += P_TempTbl->data.Paym;
				Total.Debt   += P_TempTbl->data.Debt;
			}
			Counter.Init(Total.Count);
		}
	}
	{
		class PaymPlanCrosstab : public Crosstab {
		public:
			PaymPlanCrosstab(PPViewPaymPlan * pV) : Crosstab(), P_V(pV)
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
				if(pVal && P_V) 
					P_V->GetTabTitle(*static_cast<const long *>(pVal), rBuf);
			}
			PPViewPaymPlan * P_V;
		};
		if((Filt.Flags & PaymPlanFilt::fCrosstab) && P_TempTbl) {
			THROW_MEM(P_Ct = new PaymPlanCrosstab(this));
			P_Ct->SetTable(P_TempTbl, P_TempTbl->Dt);
			P_Ct->AddIdxField(P_TempTbl->ObjID);
			P_Ct->AddInheritedFixField(P_TempTbl->Name);
			P_Ct->AddAggrField(P_TempTbl->Amount);
			P_Ct->AddAggrField(P_TempTbl->Paym);
			P_Ct->AddAggrField(P_TempTbl->Debt);
			{
				SString word;
				DBFieldList total_row_list, total_col_list;
				total_row_list.Add(P_TempTbl->Amount);
				total_row_list.Add(P_TempTbl->Paym);
				total_row_list.Add(P_TempTbl->Debt);
				PPGetWord(PPWORD_TOTAL, 0, word);
				P_Ct->AddTotalRow(total_row_list, 0, word);
				PPLoadString("shipped", word);
				P_Ct->AddTotalColumn(P_TempTbl->Amount, 0, word);
				PPLoadString("paid", word);
				P_Ct->AddTotalColumn(P_TempTbl->Paym, 0, word);
				// @v9.2.1 PPGetWord(PPWORD_DEBT, 0, word);
				PPLoadString("debt", word); // @v9.2.1
				P_Ct->AddTotalColumn(P_TempTbl->Debt, 0, word);
			}
			THROW(P_Ct->Create(1));
		}
	}
	CATCH
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewPaymPlan::InitIteration()
{
	BExtQuery::ZDelete(&P_IterQuery);
	int    ok = 1;
	if(!P_TempTbl)
		ok = PPSetError(PPERR_PPVIEWNOTINITED);
	else {
		TempPaymPlanTbl::Key1 k1;
		P_IterQuery = new BExtQuery(P_TempTbl, 1);
		P_IterQuery->selectAll();
		MEMSZERO(k1);
		P_IterQuery->initIteration(false, &k1, spFirst);
		Counter.Init(Total.Count);
		InnerCounter.Init(0UL);
		BillList.freeAll();
		PaymList.freeAll();
	}
	return ok;
}

void PPViewPaymPlan::NextInnerIteration(PaymPlanViewItem * pItem)
{
	if(Filt.Flags & PaymPlanFilt::fDetailByBill) {
		if(InnerCounter >= InnerCounter.GetTotal()) {
			ObjIdListFilt bill_list;
			BillList.freeAll();
			PaymList.freeAll();
			GetBillList(pItem->ObjID, pItem->Dt, &bill_list, &PaymList, 0);
			bill_list.CopyTo(&BillList);
			InnerCounter.Init(BillList.getCount());
		}
		const uint idx = InnerCounter;
		if(idx < InnerCounter.GetTotal()) {
			if(pItem) {
				pItem->BillID = BillList.at(idx);
				pItem->Amount = PaymList.at(idx).X;
				pItem->Paym   = PaymList.at(idx).Y;
				pItem->Debt   = R2(pItem->Amount - pItem->Paym);
			}
			InnerCounter.Increment();
		}
	}
}

int FASTCALL PPViewPaymPlan::NextIteration(PaymPlanViewItem * pItem)
{
	int    ok = 1;
	if(P_IterQuery == 0)
		ok = PPSetError(PPERR_ITERNOTINITED);
	else {
		int    r = 1;
		if(Counter == 0 || InnerCounter >= InnerCounter.GetTotal()) {
			r = P_IterQuery->nextIteration();
			Counter.Increment();
		}
		if(r > 0) {
			PaymPlanViewItem item;
			const TempPaymPlanTbl::Rec & r_src_rec = P_TempTbl->data;
			item.Dt     = r_src_rec.Dt;
			item.BillID = r_src_rec.BillID;
			item.ObjID  = r_src_rec.ObjID;
			STRNSCPY(item.Name, r_src_rec.Name);
			item.Amount = r_src_rec.Amount;
			item.Paym   = r_src_rec.Paym;
			item.Debt   = r_src_rec.Debt;
			NextInnerIteration(&item);
			ASSIGN_PTR(pItem, item);
		}
		else
			ok = -1;
	}
	return ok;
}

void PPViewPaymPlan::FormatCycle(LDATE dt, char * pBuf, size_t bufLen)
{
	Helper_FormatCycle(Filt.Cycl, CycleList, dt, pBuf, bufLen);
}

void PPViewPaymPlan::GetTabTitle(long tabID, SString & rBuf) const
{
	if(tabID && P_TempTbl) {
		char   buf[64];
		LDATE  dt;
		dt.v = tabID;
		memzero(buf, sizeof(buf));
		if(Filt.Cycl.Cycle) {
			CycleList.formatCycle(dt, buf, sizeof(buf));
			rBuf = buf;
		}
		else
			rBuf.Z().Cat(dt);
	}
}

void PPViewPaymPlan::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_PAYPLANTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlLong(CTL_PAYPLANTOTAL_COUNT,  Total.Count);
		dlg->setCtrlReal(CTL_PAYPLANTOTAL_AMOUNT, Total.Amount);
		dlg->setCtrlReal(CTL_PAYPLANTOTAL_PAYM,   Total.Paym);
		dlg->setCtrlReal(CTL_PAYPLANTOTAL_DEBT,   Total.Debt);
		ExecViewAndDestroy(dlg);
	}
}

int PPViewPaymPlan::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class PaymPlanFiltDialog : public WLDialog {
		DECL_DIALOG_DATA(PaymPlanFilt);
		enum {
			ctlgroupCycle = 1,
			ctlgroupLoc   = 2
		};
	public:
		PaymPlanFiltDialog() : WLDialog(DLG_PAYPLANFLT, CTL_PAYPLANFLT_LABEL)
		{
			CycleCtrlGroup * grp = new CycleCtrlGroup(CTLSEL_PAYPLANFLT_CYCLE, CTL_PAYPLANFLT_NUMCYCLES, CTL_PAYPLANFLT_PERIOD);
			addGroup(ctlgroupCycle, grp);
			addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_PAYPLANFLT_LOC, 0, 0, cmLocList, 0, LocationCtrlGroup::fEnableSelUpLevel, 0));
			SetupCalPeriod(CTLCAL_PAYPLANFLT_PERIOD, CTL_PAYPLANFLT_PERIOD);
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			CycleCtrlGroup::Rec cycle_rec;
			SETIFZ(Data.AccSheetID, GetSellAccSheet());
			SetPeriodInput(this, CTL_PAYPLANFLT_PERIOD, Data.Period);
			LocationCtrlGroup::Rec loc_rec(&Data.LocList);
			setGroupData(ctlgroupLoc, &loc_rec);
			SetupPPObjCombo(this, CTLSEL_PAYPLANFLT_SHEET, PPOBJ_ACCSHEET, Data.AccSheetID, 0, 0);
			SetupArCombo(this, CTLSEL_PAYPLANFLT_OBJECT, Data.ObjectID, OLW_LOADDEFONOPEN, Data.AccSheetID, sacfDisableIfZeroSheet);
			SetupArCombo(this, CTLSEL_PAYPLANFLT_AGENT,  Data.AgentID,  OLW_LOADDEFONOPEN, GetAgentAccSheet(), sacfDisableIfZeroSheet);
			cycle_rec.C = Data.Cycl;
			setGroupData(ctlgroupCycle, &cycle_rec);
			AddClusterAssoc(CTL_PAYPLANFLT_FLAGS, 0, PaymPlanFilt::fCurrentDebt);
			AddClusterAssoc(CTL_PAYPLANFLT_FLAGS, 1, PaymPlanFilt::fDetail);
			AddClusterAssoc(CTL_PAYPLANFLT_FLAGS, 2, PaymPlanFilt::fCrosstab);
			SetClusterData(CTL_PAYPLANFLT_FLAGS, Data.Flags);
			SetupSubstPersonCombo(this, CTLSEL_PAYPLANFLT_SGP, Data.Sgp);
			disableCtrl(CTLSEL_PAYPLANFLT_SGP, !(Data.Flags & PaymPlanFilt::fDetail));
			setWL(BIN(Data.Flags & PaymPlanFilt::fLabelOnly));
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			CycleCtrlGroup::Rec cycle_rec;
			LocationCtrlGroup::Rec loc_rec;
			getGroupData(ctlgroupLoc, &loc_rec);
			Data.LocList = loc_rec.LocList;
			THROW(GetPeriodInput(this, sel = CTL_PAYPLANFLT_PERIOD, &Data.Period));
			getCtrlData(sel = CTLSEL_PAYPLANFLT_SHEET, &Data.AccSheetID);
			THROW_PP(Data.AccSheetID, PPERR_ACCSHEETNEEDED);
			getCtrlData(CTLSEL_PAYPLANFLT_OBJECT, &Data.ObjectID);
			getCtrlData(CTLSEL_PAYPLANFLT_AGENT,  &Data.AgentID);
			getGroupData(ctlgroupCycle, &cycle_rec);
			Data.Cycl = cycle_rec.C;
			GetClusterData(CTL_PAYPLANFLT_FLAGS, &Data.Flags);
			if(Data.Flags & PaymPlanFilt::fDetail) {
				getCtrlData(CTLSEL_PAYPLANFLT_SGP, &Data.Sgp);
			}
			else
				Data.Sgp = sgpNone;
			SETFLAG(Data.Flags, PaymPlanFilt::fLabelOnly, getWL());
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			WLDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_PAYPLANFLT_SHEET)) {
				Data.AccSheetID = getCtrlLong(CTLSEL_PAYPLANFLT_SHEET);
				SetupArCombo(this, CTLSEL_PAYPLANFLT_OBJECT, 0, OLW_LOADDEFONOPEN, Data.AccSheetID, sacfDisableIfZeroSheet);
				clearEvent(event);
			}
			else if(event.isClusterClk(CTL_PAYPLANFLT_FLAGS)) {
				GetClusterData(CTL_PAYPLANFLT_FLAGS, &Data.Flags);
				disableCtrl(CTLSEL_PAYPLANFLT_SGP, !(Data.Flags & PaymPlanFilt::fDetail));
			}
		}
	};
	if(!Filt.IsA(pBaseFilt))
		return 0;
	PaymPlanFilt * p_filt = static_cast<PaymPlanFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(PaymPlanFiltDialog, p_filt);
}

DBQuery * PPViewPaymPlan::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	int    ok = 1;
	uint   brw_id = BROWSER_PAYMPLAN;
	DBE    dbe_cycle;
	DBQuery * q = 0;
	TempPaymPlanTbl * t = 0;
	if(P_Ct) {
		q = PPView::CrosstabDbQueryStub;
		brw_id = BROWSER_PAYMPLAN_CROSSTAB;
	}
	else {
		t = new TempPaymPlanTbl(P_TempTbl->GetName());
		THROW(CheckTblPtr(t));
		{
			dbe_cycle.init();
			dbe_cycle.push(t->Dt);
			dbe_cycle.push(dbconst((long)&CycleList)); // @x64crit
			dbe_cycle.push(static_cast<DBFunc>(PPDbqFuncPool::IdFormatCycle));
		}
		q = & Select_(
			t->BillID, // #00
			t->ObjID,  // #01
			t->Dt,     // #02
			t->Name,   // #03
			t->Amount, // #04
			t->Paym,   // #05
			t->Debt,   // #06
			dbe_cycle, // #07
			0L).from(t, 0L);
		THROW(CheckQueryPtr(q));
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete t;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewPaymPlan::ViewArticleInfo(const BrwHdr * pHdr, int what)
{
	int    ok = -1;
	if(pHdr && pHdr->ObjID) {
		PPID   person_id = 0;
		if(what == 1 && (person_id = ObjectToPerson(pHdr->ObjID)) > 0) {
			PPObjPerson psn_obj;
			if(psn_obj.Edit(&person_id, 0) == cmOK)
				ok = 1;
		}
		else {
			PPID ar_id = pHdr->ObjID;
			if(ArObj.Edit(&ar_id, 0) == cmOK)
				ok = 1;
		}
		if(ok > 0) {
			ArticleTbl::Rec ar_rec;
			if(ArObj.Search(pHdr->ObjID, &ar_rec) > 0) {
				TempPaymPlanTbl * t = P_TempTbl;
				THROW_DB(updateFor(t, 1, (t->ObjID == pHdr->ObjID), set(t->Name, dbconst(ar_rec.Name))));
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

void PPViewPaymPlan::GetEditIds(const void * pRow, LDATE * pDt, PPID * pObjID, long col)
{
	LDATE dt = ZERODATE;
	PPID  obj_id = 0;
	if(pRow) {
		if(P_Ct) {
			uint   tab_idx = (col > 0) ? (col - 1) / 3 + 1 : col;
			int    r = (tab_idx > 0) ? P_Ct->GetTab(tab_idx - 1, &dt.v) : 1;
			P_Ct->GetIdxFieldVal(0, pRow, &obj_id, sizeof(obj_id));
			obj_id = (obj_id == MAXLONG) ? 0 : obj_id;
		}
		else {
			BrwHdr hdr = *(BrwHdr*)pRow;
			dt       = hdr.Dt;
			obj_id   = hdr.ObjID;
		}
	}
	ASSIGN_PTR(pDt, dt);
	ASSIGN_PTR(pObjID, obj_id);
}

int PPViewPaymPlan::GetBillList(PPID objID, LDATE dt, ObjIdListFilt * pBillList, RPairArray * pPaymList, int allowZeroPeriod)
{
	int    ok = 1;
	DateRange period;
	period.Z();
	if(!Filt.Cycl)
		period = Filt.Period;
	else if(CycleList.searchPeriodByDate(dt, &period) <= 0) {
		if(allowZeroPeriod)
			period = Filt.Period;
		else
			ok = -1;
	}
	if(ok > 0) {
		RPairArray paym_list;
		ObjIdListFilt bill_list;
		IterProcParam_ProcessPeriod ipp(this, &bill_list, &paym_list, period, objID);
		THROW(P_BObj->P_Tbl->PayPlanEnumerator(&period, IterProc_PPViewPaymPlan_ProcessPeriod, &ipp));
		ASSIGN_PTR(pBillList, bill_list);
		ASSIGN_PTR(pPaymList, paym_list);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPViewPaymPlan::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	LDATE  dt = ZERODATE;
	PPID   obj_id = 0L;
	const  BrwHdr * p_hdr = static_cast<const BrwHdr *>(pHdr);
	BillFilt flt;
	GetEditIds(p_hdr, &dt, &obj_id, (pBrw) ? pBrw->GetCurColumn() : 0);
	flt.List.InitEmpty();
	THROW(GetBillList(obj_id, dt, &flt.List, 0, BIN(P_Ct != 0)));
	flt.Flags |= (BillFilt::fShowDebt|BillFilt::fPaymNeeded);
	ViewGoodsBills(&flt, true/*modeless*/);
	CATCHZOKPPERR
	return ok;
}

int PPViewPaymPlan::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr hdr;
		if(pHdr)
			hdr = *static_cast<const BrwHdr *>(pHdr);
		else
			MEMSZERO(hdr);
		GetEditIds(&hdr, &hdr.Dt, &hdr.ObjID, (pBrw) ? pBrw->GetCurColumn() : 0);
		switch(ppvCmd) {
			case PPVCMD_EDITARTICLE: ok = hdr.ObjID ? ViewArticleInfo(&hdr, 0) : -1; break;
			case PPVCMD_EDITPERSON:  ok = hdr.ObjID ? ViewArticleInfo(&hdr, 1) : -1; break;
			case PPVCMD_PRINTDETAIL:
				SETFLAG(Filt.Flags, PaymPlanFilt::fDetailByBill, 1);
				Print(0);
				SETFLAG(Filt.Flags, PaymPlanFilt::fDetailByBill, 0);
				break;
		}
	}
	return ok;
}

int PPViewPaymPlan::Print(const void *)
{
	uint rpt_id = (Filt.Flags & PaymPlanFilt::fDetailByBill) ? REPORT_PAYMPLANDETAIL : REPORT_PAYMPLAN;
	return Helper_Print(rpt_id, 0);
}
//
// Implementation of PPALDD_PaymPlan
//
PPALDD_CONSTRUCTOR(PaymPlan)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PaymPlan) { Destroy(); }

int PPALDD_PaymPlan::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(PaymPlan, rsrv);
	H.FltBeg   = p_filt->Period.low;
	H.FltEnd   = p_filt->Period.upp;
	H.FltAccSheetID = p_filt->AccSheetID;
	H.FltArID  = p_filt->ObjectID;
	H.FltLocID = p_filt->LocList.GetSingle();
	H.FltCycle = p_filt->Cycl.Cycle;
	H.FltNumCycles = p_filt->Cycl.NumCycles;
	H.FltFlags     = p_filt->Flags;
	return DlRtm::InitData(rFilt, rsrv);
}

void PPALDD_PaymPlan::Destroy() { DESTROY_PPVIEW_ALDD(PaymPlan); }
int  PPALDD_PaymPlan::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/) { INIT_PPVIEW_ALDD_ITER(PaymPlan); }

int PPALDD_PaymPlan::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(PaymPlan);
	I.Dt     = item.Dt;
	I.BillID = item.BillID;
	I.ArID   = item.ObjID;
	I.Amount = item.Amount;
	I.Paym   = item.Paym;
	I.Debt   = item.Debt;
	STRNSCPY(I.Name, item.Name);
	p_v->FormatCycle(item.Dt, I.CycleText, sizeof(I.CycleText));
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}
