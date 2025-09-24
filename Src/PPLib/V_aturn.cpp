// V_ATURN.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2011, 2012, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2024, 2025
//
#include <pp.h>
#pragma hdrstop
//
//
//
class TempAssoc {
public:
	static TempAssoc * CreateInstance();
	TempAssoc();
	bool   IsValid() const { return LOGIC(P_Tbl); }
	int    Add(PPID prmrID, PPID scndID);
	int    EnumPrmr(PPID * pPrmrID);
	int    GetList(PPID prmrID, PPIDArray *);
private:
	TempAssocTbl * P_Tbl;
};

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempAssoc);

/*static*/TempAssoc * TempAssoc::CreateInstance()
{
	TempAssoc * p_assc = new TempAssoc;
	if(!p_assc)
		return (PPSetErrorNoMem(), (TempAssoc *)0);
	else
		return p_assc->IsValid() ? p_assc : 0;
}

TempAssoc::TempAssoc() : P_Tbl(CreateTempFile())
{
}

int TempAssoc::Add(PPID prmrID, PPID scndID)
{
	if(P_Tbl) {
		P_Tbl->data.PrmrID = prmrID;
		P_Tbl->data.ScndID = scndID;
		return P_Tbl->insertRec() ? 1 : PPSetErrorDB();
	}
	return 0;
}

int TempAssoc::EnumPrmr(PPID * pPrmrID)
{
	if(P_Tbl) {
		TempAssocTbl::Key0 k0;
		k0.PrmrID = *pPrmrID;
		k0.ScndID = MAXLONG;
		if(P_Tbl->search(0, &k0, spGt)) {
			*pPrmrID = P_Tbl->data.PrmrID;
			return 1;
		}
	}
	return -1;
}

int TempAssoc::GetList(PPID prmrID, PPIDArray * pList)
{
	int    ok = 1;
	if(P_Tbl) {
		TempAssocTbl::Key0 k0;
		BExtQuery q(P_Tbl, 0, 128);
		q.select(P_Tbl->ScndID, 0L).where(P_Tbl->PrmrID == prmrID);
		k0.PrmrID = prmrID;
		k0.ScndID = -MAXLONG;
		for(q.initIteration(false, &k0, spGe); ok && q.nextIteration() > 0;)
			if(!pList->add(P_Tbl->data.ScndID))
				ok = 0;
	}
	else
		ok = 0;
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(Accturn); AccturnFilt::AccturnFilt() : PPBaseFilt(PPFILT_ACCTURN, 0, 0)
{
	SetFlatChunk(offsetof(AccturnFilt, ReserveStart),
		offsetof(AccturnFilt, Reserve)-offsetof(AccturnFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

AccturnFilt & FASTCALL AccturnFilt::operator = (const AccturnFilt & s)
{
	Copy(&s, 0);
	return *this;
}

PPViewAccturn::PPViewAccturn() : PPView(0, &Filt, PPVIEW_ACCTURN, 0, 0),
	P_BObj(BillObj), P_ATC(P_BObj->atobj->P_Tbl), P_TmpAGTbl(0), P_TmpBillTbl(0)
{
}

PPViewAccturn::~PPViewAccturn()
{
	delete P_TmpAGTbl;
	delete P_TmpBillTbl;
	DBRemoveTempFiles();
}

PPBaseFilt * PPViewAccturn::CreateFilt(const void * extraPtr) const
{
	AccturnFilt * p_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_ACCTURN, reinterpret_cast<PPBaseFilt **>(&p_filt)))
		p_filt->Period.SetDate(LConfig.OperDate);
	return static_cast<PPBaseFilt *>(p_filt);
}

int PPViewAccturn::DeleteItem(PPID billID)
{
	int    ok = -1;
	if(!Filt.GrpAco && billID && !Filt.BillID) {
		THROW(P_BObj->P_Tbl->Search(billID) > 0);
		if(GetOpType(P_BObj->P_Tbl->data.OpID) == PPOPT_ACCTURN) {
			THROW(ok = P_BObj->RemoveObjV(billID, 0, PPObject::rmv_default, 0));
			if(ok > 0)
				THROW(ok = RemoveBillFromList(billID));
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewAccturn::EditItem(PPID billID)
{
	int    ok = -1;
	PPID   id = billID;
	if(!Filt.GrpAco && id && !Filt.BillID)
		if(P_BObj->Edit(&id, 0) == cmOK) {
			RemoveBillFromList(id);
			AddBillToList(id);
			ok = 1;
		}
	return ok;
}

/*virtual*/int PPViewAccturn::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	if(Filt.GrpAco) {
		AccturnFilt temp_flt = Filt;
		struct __AGBI {
			LDATE Dt;
			PPID  DbtAccID;
			PPID  CrdAccID;
			PPID  CurID;
			Acct  DbtAcc;
			Acct  CrdAcc;
		} agbi;
		if(pHdr)
			agbi = *static_cast<const __AGBI *>(pHdr);
		else
			MEMSZERO(agbi);
		if(Filt.Cycl.Cycle && CycleList.searchPeriodByDate(agbi.Dt, &temp_flt.Period) <= 0)
			return -1;
		temp_flt.Aco = Filt.GrpAco;
		temp_flt.GrpAco = 0;
		temp_flt.Cycl.Cycle = 0;
		temp_flt.DbtAcct = agbi.DbtAcc;
		temp_flt.CrdAcct = agbi.CrdAcc;
		temp_flt.Flags &= ~AccturnFilt::fAllCurrencies;
		temp_flt.CurID = agbi.CurID;
		return PPView::Execute(PPVIEW_ACCTURN, &temp_flt, 1, 0);
	}
	else
		return -1;
}

/*virtual*/void PPViewAccturn::ViewTotal()
{
	AccturnTotal total;
	if(CalcTotal(&total)) {
		AmtListDialog * dlg = new AmtListDialog(DLG_ATURNTOTAL, CTL_ATURNTOTAL_AMTLIST, 1, &total.Amounts, 0, 0, 0);
		if(CheckDialogPtrErr(&dlg)) {
			dlg->setCtrlLong(CTL_ATURNTOTAL_COUNT, total.Count);
			ExecViewAndDestroy(dlg);
		}
	}
}

int PPViewAccturn::CalcTotal(AccturnTotal * pTotal)
{
	AccturnViewItem item;
	pTotal->Count = 0;
	pTotal->Amounts.freeAll();
	PPWaitStart();
	if(InitIteration())
		while(NextIteration(&item) > 0) {
			pTotal->Count++;
			pTotal->Amounts.Add(0, item.CurID, item.Amount);
		}
	PPWaitStop();
	return 1;
}

int PPViewAccturn::PrintItem(PPID billID)
{
	int    ok = -1;
	ushort v = 0;
	PPBillPacket pack;
	TDialog    * dlg = 0;
	if(Filt.GrpAco)
		ok = Print(0);
	else {
		THROW(P_BObj->ExtractPacket(billID, &pack) > 0);
		THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PRNCASH))));
		dlg->setCtrlData(CTL_PRNCASH_FORM, &v);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_PRNCASH_FORM, &v);
			THROW(PrintCashOrder(&pack, BIN(v == 0)));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

/*virtual*/int PPViewAccturn::Print(const void *)
{
	int    ok = 1;
	uint   rpt_id = Filt.GrpAco ? (Filt.Cycl.Cycle ? REPORT_ATURNCYCLEGRPNG : REPORT_ATURNGRPNG) : REPORT_ATURNLIST;
	PPAlddPrint(rpt_id, PView(this), 0);
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempAGFile, TempAccturnGrpng);

struct ACGREC { // @flat
	LDATE  Dt;
	PPID   DbtAccID;
	PPID   CrdAccID;
	PPID   CurID;
	Acct   Da;
	Acct   Ca;
	long   DbtNameIdx;
	long   CrdNameIdx;
	long   Count;
	double Amount;
};

IMPL_CMPFUNC(ACGREC, i1, i2) 
	{ RET_CMPCASCADE4(static_cast<const ACGREC *>(i1), static_cast<const ACGREC *>(i2), Dt, DbtAccID, CrdAccID, CurID); }

int PPViewAccturn::CreateGrouping()
{
	class AccturnGroupingCache {
	public:
		AccturnGroupingCache() : LastNameIdx(0)
		{
		}
		int Add(LDATE dt, PPID dbtAccID, const Acct & rDa, PPID crdAccID, const Acct & rCa, PPID curID, double amt)
		{
			int    ok = 1;
			ACGREC item;
			MEMSZERO(item);
			item.Dt = dt;
			item.DbtAccID = dbtAccID;
			item.CrdAccID = crdAccID;
			item.CurID = curID;
			item.Da = rDa;
			item.Ca = rCa;
			uint pos = 0;
			if(List.lsearch(&item, &pos, PTR_CMPFUNC(ACGREC))) {
				List.at(pos).Count++;
				List.at(pos).Amount += amt;
			}
			else {
				SString name_buf;
				GetAcctName(&item.Da, curID, 0, name_buf);
				item.DbtNameIdx = ++LastNameIdx;
				NameList.Add(item.DbtNameIdx, name_buf);
				GetAcctName(&item.Ca, curID, 0, name_buf);
				item.CrdNameIdx = ++LastNameIdx;
				NameList.Add(item.CrdNameIdx, name_buf);
				item.Count = 1;
				item.Amount = amt;
				List.insert(&item);
			}
			return ok;
		}
		uint GetCount() const
		{
			return List.getCount();
		}
		int Get(uint pos, TempAccturnGrpngTbl::Rec * pRec)
		{
			int    ok = 1;
			memzero(pRec, sizeof(*pRec));
			if(pos < List.getCount()) {
				ACGREC & r_item = List.at(pos);
				pRec->Dt = r_item.Dt;
				pRec->DbtAccID = r_item.DbtAccID;
				pRec->DbtAc = r_item.Da.ac;
				pRec->DbtSb = r_item.Da.sb;
				pRec->DbtAr = r_item.Da.ar;
				pRec->CrdAccID = r_item.CrdAccID;
				pRec->CrdAc = r_item.Ca.ac;
				pRec->CrdSb = r_item.Ca.sb;
				pRec->CrdAr = r_item.Ca.ar;
				pRec->CurID = r_item.CurID;
				pRec->Count = r_item.Count;
				pRec->Amount = r_item.Amount;
				if(r_item.DbtNameIdx) {
					NameList.GetText(r_item.DbtNameIdx, TempBuf);
					TempBuf.CopyTo(pRec->DbtAccName, sizeof(pRec->DbtAccName));
				}
				if(r_item.CrdNameIdx) {
					NameList.GetText(r_item.CrdNameIdx, TempBuf);
					TempBuf.CopyTo(pRec->CrdAccName, sizeof(pRec->CrdAccName));
				}
				ok = 1;
			}
			else
				ok = 0;
			return ok;
		}
	private:
		TSVector <ACGREC> List;
		StrAssocArray NameList;
		long   LastNameIdx;
		SString TempBuf; // @allocreuse
	};
	int    ok = 1;
	AccturnViewItem item;
	PPViewAccturn * p_temp_view = 0;
	TempAccturnGrpngTbl * p_tbl = 0;
	AccturnFilt temp_flt;
	AccturnGroupingCache cache;
	THROW(p_tbl = CreateTempAGFile());
	THROW_MEM(p_temp_view = new PPViewAccturn);
	if(Filt.Cycl.Cycle) {
		CycleList.init(&Filt.Period, Filt.Cycl);
		CycleList.getCycleParams(&Filt.Period, &Filt.Cycl);
	}
	else
		CycleList.freeAll();
	temp_flt = Filt;
	temp_flt.GrpAco = 0;
	temp_flt.Cycl.Cycle = 0;
	THROW(p_temp_view->Init_(&temp_flt));
	for(p_temp_view->InitIteration(); p_temp_view->NextIteration(&item) > 0;) {
		uint   cycle_pos = 0;
		Acct   dbt_acct, crd_acct;
		PPID   cur_id = item.CurID, temp_cur_id = 0;
		TempAccturnGrpngTbl::Rec rec;
		if(Filt.Cycl.Cycle)
			if(CycleList.searchDate(item.Date, &cycle_pos))
				rec.Dt = CycleList.at(cycle_pos).low;
			else
				continue;
		else
			rec.Dt = Filt.Period.low;
		P_ATC->ConvertAcctID(item.DbtID, &dbt_acct, &temp_cur_id, 1 /* useCache */);
		P_ATC->ConvertAcctID(item.CrdID, &crd_acct, &temp_cur_id, 1 /* useCache */);
		rec.CurID = cur_id;
		if(Filt.GrpAco == ACO_1) {
			int    r;
			PPAccount acc_rec;
			dbt_acct.sb = crd_acct.sb = 0;
			dbt_acct.ar = crd_acct.ar = 0;
			THROW(r = AccObj.FetchNum(dbt_acct.ac, 0, 0, &acc_rec));
			if(r > 0)
				rec.DbtAccID = acc_rec.ID;
			THROW(r = AccObj.FetchNum(crd_acct.ac, 0, 0, &acc_rec));
			if(r > 0)
				rec.CrdAccID = acc_rec.ID;
		}
		else if(Filt.GrpAco == ACO_2) {
			dbt_acct.ar = 0;
			crd_acct.ar = 0;
			rec.DbtAccID = item.DbtID.ac;
			rec.CrdAccID = item.CrdID.ac;
		}
		else {
			rec.DbtAccID = item.DbtAccRelID;
			rec.CrdAccID = item.CrdAccRelID;
		}
		THROW(cache.Add(rec.Dt, rec.DbtAccID, dbt_acct, rec.CrdAccID, crd_acct, cur_id, item.Amount));
		PPWaitDate(item.Date);
	}
	{
		BExtInsert bei(p_tbl);
		for(uint i = 0; i < cache.GetCount(); i++) {
			TempAccturnGrpngTbl::Rec rec;
			if(cache.Get(i, &rec) > 0) {
				THROW_DB(bei.insert(&rec));
			}
		}
		THROW_DB(bei.flash());
	}
	P_TmpAGTbl = p_tbl;
	CATCH
		ok = 0;
		ZDELETE(p_tbl);
	ENDCATCH
	delete p_temp_view;
	return ok;
}

/*virtual*/int PPViewAccturn::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	if(P_TmpBillTbl)
		deleteFrom(P_TmpBillTbl, 0, P_TmpBillTbl->PrmrID > 0L);
	ZDELETE(P_TmpBillTbl);
	THROW(Helper_InitBaseFilt(pFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TmpAGTbl);
	IterBillPos = 0;
	IterRByBill = 0;
	OpList.freeAll();
	Filt.Period.Actualize(ZERODATE);
	if(Filt.Flags & AccturnFilt::fLastOnly && !Filt.BillID) {
		THROW_MEM(P_TmpBillTbl = CreateTempFile());
		//THROW(AddBillToList(Filt.BillID));
	}
	else if(AdjustPeriodToRights(Filt.Period, 0)) {
		if(Filt.OpID)
			if(IsGenericOp(Filt.OpID) > 0)
				GetGenericOpList(Filt.OpID, &OpList);
			else
				OpList.add(Filt.OpID);
		if(Filt.GrpAco) {
			if(!CreateGrouping())
				ok = 0;
		}
	}
	else
		ok = 0;
	CATCHZOK
	return ok;
}

int PPViewAccturn::InitIteration()
{
	int    ok = 1;
	DBQ  * dbq = 0;
	BExtQuery::ZDelete(&P_IterQuery);
	if(Filt.Flags & AccturnFilt::fLastOnly && Filt.BillID) {
		AccTurnTbl::Key0 k0;
		MEMSZERO(k0);
		k0.BillID = Filt.BillID;
		THROW_MEM(P_IterQuery = new BExtQuery(P_ATC, 0));
		P_IterQuery->selectAll().where(P_ATC->BillID == Filt.BillID && P_ATC->Reverse == 0L);
		P_IterQuery->initIteration(false, &k0, spGe);
	}
	else if(Filt.GrpAco) {
		if(P_TmpAGTbl) {
			TempAccturnGrpngTbl::Key1 agk;
			THROW_MEM(P_IterQuery = new BExtQuery(P_TmpAGTbl, 1, 16 /* very large record */));
			P_IterQuery->selectAll();
			MEMSZERO(agk);
			P_IterQuery->initIteration(false, &agk, spFirst);
		}
		else
			ok = -1;
	}
	else {
		double ip;
		double min_amt = Filt.AmtR.low;
		double max_amt = Filt.AmtR.upp;
		int    idx = 2;
		AccTurnTbl::Key2 k2;
		THROW_MEM(P_IterQuery = new BExtQuery(P_ATC, idx));
		dbq = & daterange(P_ATC->Dt, &Filt.Period);
		if(min_amt == max_amt && min_amt != 0L) {
			min_amt = (modf(min_amt, &ip) == 0) ? (ip - 1L) : ip;
			max_amt = ip + 1L;
		}
		dbq = & (*dbq && realrange(P_ATC->Amount, min_amt, max_amt) && P_ATC->Reverse == 0L);
		if(P_TmpBillTbl)
			dbq = & (*dbq && P_TmpBillTbl->PrmrID == P_ATC->BillID);
		P_IterQuery->where(*dbq);
		k2.Dt    = Filt.Period.low;
		k2.OprNo = 0;
		P_IterQuery->initIteration(false, &k2, spGt);
	}
	CATCH
		ok = 0;
		BExtQuery::ZDelete(&P_IterQuery);
	ENDCATCH
	return ok;
}

int PPViewAccturn::InitViewItem(const AccTurnTbl::Rec * pAtRec, AccturnViewItem * pItem)
{
	int    ok = -1;
	PPAccTurn aturn;
	if(P_BObj->GetAccturn(pAtRec, &aturn, 1) > 0) {
		if((Filt.Flags & AccturnFilt::fAllCurrencies) || aturn.CurID == Filt.CurID) {
			if(P_TmpBillTbl || !OpList.getCount() || OpList.lsearch(aturn.Opr)) {
				int    rt = 0;
				const PPRights & r_orts = ObjRts;
				if(!r_orts.CheckAccID(aturn.DbtID.ac, PPR_READ)) {
					aturn.DbtID.ac = 0;
					aturn.DbtID.ar = 0;
					aturn.DbtSheet = 0;
				}
				else
					rt++;
				if(!r_orts.CheckAccID(aturn.CrdID.ac, PPR_READ)) {
					aturn.CrdID.ac = 0;
					aturn.CrdID.ar = 0;
					aturn.CrdSheet = 0;
				}
				else
					rt++;
				if(rt) {
					if(pItem) {
						if(aturn.Flags & PPAF_OUTBAL_WITHDRAWAL)
							aturn.Amount = -aturn.Amount;
						*static_cast<PPAccTurn *>(pItem) = aturn;
						pItem->OprNo = pAtRec->OprNo;
						P_ATC->GetAccRelIDs(pAtRec, &pItem->DbtAccRelID, &pItem->CrdAccRelID);
					}
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int PPViewAccturn::InitViewItem(const TempAccturnGrpngTbl::Rec * pATGRec, AccturnViewItem * pItem)
{
	int    ok = 1;
	if(pItem && pATGRec) {
		memzero(pItem, sizeof(AccturnViewItem));
		pItem->Date   = pATGRec->Dt;
		pItem->CurID  = pATGRec->CurID;
		pItem->Amount = pATGRec->Amount;
		if(oneof2(Filt.GrpAco, ACO_1, ACO_2)) {
			pItem->DbtID.ac = pATGRec->DbtAccID;
			pItem->CrdID.ac = pATGRec->CrdAccID;
		}
		else {
			AcctRelTbl::Rec rec;
			if(P_ATC->AccRel.Fetch(pATGRec->DbtAccID, &rec) > 0) {
				pItem->DbtAccRelID = rec.ID;
				pItem->DbtID.ac = rec.AccID;
				pItem->DbtID.ar = rec.ArticleID;
			}
			if(P_ATC->AccRel.Fetch(pATGRec->CrdAccID, &rec) > 0) {
				pItem->CrdAccRelID = rec.ID;
				pItem->CrdID.ac = rec.AccID;
				pItem->CrdID.ar = rec.ArticleID;
			}
		}
	}
	else
		ok = -1;
	return ok;
}

int FASTCALL PPViewAccturn::NextIteration(AccturnViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery)
		while(ok < 0 && P_IterQuery->nextIteration() > 0) {
			if(Filt.GrpAco) {
				if(InitViewItem(&P_TmpAGTbl->data, pItem) > 0)
					ok = 1;
			}
			else {
				AccTurnTbl::Rec at_rec;
				P_ATC->copyBufTo(&at_rec);
				if(Filt.Aco) {
					PPID   dbt_rel_id = 0;
					PPID   crd_rel_id = 0;
					P_ATC->GetAccRelIDs(&at_rec, &dbt_rel_id, &crd_rel_id);
					if(P_ATC->AccBelongToOrd(dbt_rel_id, Filt.Aco, &Filt.DbtAcct, Filt.CurID, 1) <= 0)
						continue;
					if(P_ATC->AccBelongToOrd(crd_rel_id, Filt.Aco, &Filt.CrdAcct, Filt.CurID, 1) <= 0)
						continue;
				}
				if(InitViewItem(&at_rec, pItem) > 0)
					ok = 1;
			}
		}
	return ok;
}

void PPViewAccturn::FormatCycle(LDATE dt, char * pBuf, size_t bufLen)
{
	if(Filt.GrpAco)
		Helper_FormatCycle(Filt.Cycl, CycleList, dt, pBuf, bufLen);
	else
		ASSIGN_PTR(pBuf, 0);
}
//
//
//
/*virtual*/int PPViewAccturn::EditBaseFilt(PPBaseFilt * pFilt)
{
	class AccturnFiltDialog : public WLDialog {
		DECL_DIALOG_DATA(AccturnFilt);
		enum {
			ctrgroupCycle = 1
		};
	public:
		AccturnFiltDialog() : WLDialog(DLG_ATFLT, CTL_ATFLT_WL)
		{
			CycleCtrlGroup * grp = new CycleCtrlGroup(CTLSEL_ATFLT_CYCLE, CTL_ATFLT_NUMCYCLES, CTL_ATFLT_PERIOD);
			addGroup(ctrgroupCycle, grp);
			SetupCalPeriod(CTLCAL_ATFLT_PERIOD, CTL_ATFLT_PERIOD);
		}
		DECL_DIALOG_SETDTS()
		{
			PPIDArray types;
			ushort v;
			CycleCtrlGroup::Rec cycle_rec;
			if(!RVALUEPTR(Data, pData))
				Data.Init(1, 0);
			SetPeriodInput(this, CTL_ATFLT_PERIOD, Data.Period);
			types.addzlist(PPOPT_ACCTURN, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN,
				PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, PPOPT_PAYMENT, PPOPT_CHARGE, PPOPT_GOODSACK, PPOPT_POOL, PPOPT_GENERIC, 0L);
			SetupOprKindCombo(this, CTLSEL_ATFLT_OPRKIND, Data.OpID, 0, &types, 0);
			SetupCurrencyCombo(this, CTLSEL_ATFLT_CUR, Data.CurID, 0, 1, 0);
			SetRealRangeInput(this, CTL_ATFLT_AMOUNT, &Data.AmtR);
			v = 0;
			SETFLAG(v, 0x01, Data.Flags & AccturnFilt::fLastOnly);
			setCtrlData(CTL_ATFLT_LASTONLY, &v);
			v = 0;
			SETFLAG(v, 1, Data.Flags & AccturnFilt::fAllCurrencies);
			setCtrlData(CTL_ATFLT_ALLCUR, &v);
			setWL(BIN(Data.Flags & AccturnFilt::fLabelOnly));
			v = 0;
			if(Data.GrpAco == ACO_1)
				v = 1;
			else if(Data.GrpAco == ACO_2)
				v = 2;
			else if(Data.GrpAco == ACO_3)
				v = 3;
			else
				v = 0;
			setCtrlData(CTL_ATFLT_GRPACO, &v);
			cycle_rec.C = Data.Cycl;
			setGroupData(ctrgroupCycle, &cycle_rec);
			disableCtrls(((Data.Flags & AccturnFilt::fLastOnly) || Data.GrpAco == 0), CTLSEL_ATFLT_CYCLE, CTL_ATFLT_NUMCYCLES, 0);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			CycleCtrlGroup::Rec cycle_rec;
			THROW(GetPeriodInput(this, sel = CTL_ATFLT_PERIOD, &Data.Period));
			ushort v = getCtrlUInt16(CTL_ATFLT_LASTONLY);
			SETFLAG(Data.Flags, AccturnFilt::fLastOnly,   v & 0x01);
			SETFLAG(Data.Flags, AccturnFilt::fLabelOnly, getWL());
			THROW(Data.Flags & AccturnFilt::fLastOnly || AdjustPeriodToRights(Data.Period, 1));
			getCtrlData(CTL_ATFLT_ALLCUR, &v);
			SETFLAG(Data.Flags, AccturnFilt::fAllCurrencies, v & 1);
			getCtrlData(CTLSEL_ATFLT_OPRKIND, &Data.OpID);
			getCtrlData(CTLSEL_ATFLT_CUR,     &Data.CurID);
			GetRealRangeInput(this, CTL_ATFLT_AMOUNT, &Data.AmtR);
			getCtrlData(CTL_ATFLT_GRPACO, &v);
			if(v == 1)
				Data.GrpAco = ACO_1;
			else if(v == 2)
				Data.GrpAco = ACO_2;
			else if(v == 3)
				Data.GrpAco = ACO_3;
			else
				Data.GrpAco = 0;
			getGroupData(ctrgroupCycle, &cycle_rec);
			Data.Cycl = cycle_rec.C;
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			WLDialog::handleEvent(event);
			if(event.isCmd(cmClusterClk)) {
				if(event.isCtlEvent(CTL_ATFLT_GRPACO) || event.isCtlEvent(CTL_ATFLT_LASTONLY)) {
					const ushort v_grpaco   = getCtrlUInt16(CTL_ATFLT_GRPACO);
					const ushort v_lastonly = getCtrlUInt16(CTL_ATFLT_LASTONLY);
					disableCtrls((v_grpaco == 0 || (v_lastonly & 0x01)), CTLSEL_ATFLT_CYCLE, CTL_ATFLT_NUMCYCLES, 0);
				}
				clearEvent(event);
			}
		}
	};
	DIALOG_PROC_BODY(AccturnFiltDialog, static_cast<AccturnFilt *>(pFilt));
}

/*virtual*/void PPViewAccturn::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		if(Filt.BillID) {
			pBrw->ViewOrigin.y = 15;
			pBrw->ViewSize.y = 8;
		}
		if((Filt.Flags & (AccturnFilt::fAllCurrencies|AccturnFilt::fLastOnly)) && !Filt.GrpAco) {
			BrowserDef * p_def = pBrw->getDef();
			const int col = p_def ? (p_def->getCountI()-2) : -1;
			if(col > 0)
				pBrw->InsColumn(col, "@currency", /*12*/8, 0, MKSFMT(6, 0), 0);
		}
	}
}

// dbt_acc_id, crd_acc_id, (long)(AccTurnFilt *)

static IMPL_DBE_PROC(dbqf_accturn_checkrelrestriction_iii)
{
	long   ok = 1;
	PPID   dbt_acc_id = params[0].lval;
	PPID   crd_acc_id = params[1].lval;
	const  AccturnFilt * p_filt = reinterpret_cast<const AccturnFilt *>(params[2].lval); // @longtoptr
	PPID   bill_id = params[0].lval;
	if(p_filt && p_filt->GetSignature() == PPFILT_ACCTURN) {
		AcctRelTbl::Rec dbt_rec, crd_rec;
		AcctRel & r_tbl = BillObj->atobj->P_Tbl->AccRel;
		if(r_tbl.Fetch(dbt_acc_id, &dbt_rec) <= 0)
			MEMSZERO(dbt_rec);
		if(r_tbl.Fetch(crd_acc_id, &crd_rec) <= 0)
			MEMSZERO(crd_rec);
		if(p_filt->Aco) {
			if(dbt_rec.Ac != (long)p_filt->DbtAcct.ac || crd_rec.Ac != (long)p_filt->CrdAcct.ac)
				ok = 0;
			else if(p_filt->Aco != ACO_1) {
				if(dbt_rec.Sb != (long)p_filt->DbtAcct.sb || crd_rec.Sb != (long)p_filt->CrdAcct.sb)
					ok = 0;
			}
			if(ok && p_filt->Aco != ACO_1 && p_filt->Aco != ACO_2) {
				if(dbt_rec.Ar != p_filt->DbtAcct.ar || crd_rec.Ar != p_filt->CrdAcct.ar)
					ok = 0;
			}
		}
		if(ok) {
			if(!(p_filt->Flags & AccturnFilt::fAllCurrencies) && dbt_rec.CurID != p_filt->CurID)
				ok = 0;
		}
		if(ok) {
			const PPRights & r_orts = ObjRts;
			if(!r_orts.CheckAccID(dbt_rec.AccID, PPR_READ) && !r_orts.CheckAccID(crd_rec.AccID, PPR_READ))
				ok = 0;
		}
	}
	else
		ok = 0;
	result->init(ok);
}

static IMPL_DBE_PROC(dbqf_objname_cursymbbyacctrel_i)
{
	char   name_buf[48];
	PPCurrency rec;
	if(option == CALC_SIZE)
		result->init(sizeof(name_buf));
	else {
		const PPID id = PPDbqFuncPool::helper_dbq_name(params, name_buf);
		if(id) {
			AcctRel & r_tbl = BillObj->atobj->P_Tbl->AccRel;
			AcctRelTbl::Rec acr_rec;
			if(r_tbl.Fetch(id, &acr_rec) > 0) {
				PPObjCurrency cur_obj;
				if(acr_rec.CurID || cur_obj.Fetch(acr_rec.CurID, &rec) <= 0)
					rec.Symb[0] = 0;
				STRNSCPY(name_buf, rec.Symb);
			}
		}
		result->init(name_buf);
	}
}

/*static*/int PPViewAccturn::DynFuncCheckRelRestrictions = 0;
/*static*/int PPViewAccturn::DynFuncCurSymbByAccRelID = 0;

/*virtual*/DBQuery * PPViewAccturn::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DbqFuncTab::RegisterDyn(&DynFuncCheckRelRestrictions, BTS_INT,    dbqf_accturn_checkrelrestriction_iii, 3, BTS_INT, BTS_INT, BTS_INT);
	DbqFuncTab::RegisterDyn(&DynFuncCurSymbByAccRelID,    BTS_STRING, dbqf_objname_cursymbbyacctrel_i, 1, BTS_INT);

	uint   brw_id = 0;
	SString sub_title;
	DBQuery    * q   = 0;
	BillTbl    * bll = 0;
	AccTurnTbl * at  = 0;
	TempAssocTbl * p_tmp_bill_t = 0;
	TempAccturnGrpngTbl * p_grp_tbl = 0;
	DBE    dbe_cur;
	DBE    dbe_oprkind;
	DBE    dbe_rel_restrict;
	DBE    dbe_acc_dbt;
	DBE    dbe_acc_crd;
	DBE    dbe_memo; // @v11.1.12
	if(Filt.GrpAco) {
		THROW_PP(P_TmpAGTbl, PPERR_PPVIEWNOTINITED);
		THROW(CheckTblPtr(p_grp_tbl = new TempAccturnGrpngTbl(P_TmpAGTbl->GetName())));
		PPDbqFuncPool::InitObjNameFunc(dbe_cur, PPDbqFuncPool::IdObjSymbCurrency, p_grp_tbl->CurID);
		q = & select(
			p_grp_tbl->Dt,          // #00
			p_grp_tbl->DbtAccID,    // #01
			p_grp_tbl->CrdAccID,    // #02
			p_grp_tbl->CurID,       // #03
			p_grp_tbl->DbtAc,       // #04
			p_grp_tbl->DbtSb,       // #05
			p_grp_tbl->DbtAr,       // #06
			p_grp_tbl->CrdAc,       // #07
			p_grp_tbl->CrdSb,       // #08
			p_grp_tbl->CrdAr,       // #09
			dbe_cur,                // #10
			p_grp_tbl->DbtAccName,  // #11
			p_grp_tbl->CrdAccName,  // #12
			p_grp_tbl->Count,       // #13
			p_grp_tbl->Amount,      // #14
			0L).from(p_grp_tbl, 0L).orderBy(p_grp_tbl->Dt, p_grp_tbl->DbtAc, p_grp_tbl->DbtSb, p_grp_tbl->DbtAr, 0L);
	}
	else {
		double ip;
		double min_amt = Filt.AmtR.low;
		double max_amt = Filt.AmtR.upp;
		DBFieldList fld_list;
		DBQ  * dbq = 0;
		bll = new BillTbl;
		at  = new AccTurnTbl;
		if(P_TmpBillTbl)
			p_tmp_bill_t = new TempAssocTbl(P_TmpBillTbl->GetName());
		THROW(CheckTblPtr(at) && CheckTblPtr(bll));
		PPDbqFuncPool::InitObjNameFunc(dbe_cur, PPViewAccturn::DynFuncCurSymbByAccRelID, at->Acc);
		PPDbqFuncPool::InitObjNameFunc(dbe_oprkind, PPDbqFuncPool::IdObjNameOprKind, bll->OpID);
		PPDbqFuncPool::InitObjNameFunc(dbe_acc_dbt, PPDbqFuncPool::IdObjNameAcctRel, at->Acc);
		PPDbqFuncPool::InitObjNameFunc(dbe_acc_crd, PPDbqFuncPool::IdObjNameAcctRel, at->CorrAcc);
		PPDbqFuncPool::InitObjNameFunc(dbe_memo, PPDbqFuncPool::IdObjMemoBill, at->BillID); // @v11.1.12
		{
			dbe_rel_restrict.init();
			dbe_rel_restrict.push(at->Acc);
			dbe_rel_restrict.push(at->CorrAcc);
			DBConst c_temp;
			c_temp.init((long)&Filt); // @x64crit
			dbe_rel_restrict.push(c_temp);
			dbe_rel_restrict.push(static_cast<DBFunc>(PPViewAccturn::DynFuncCheckRelRestrictions));
		}
		q = & select(
			at->BillID,             // #00
			at->Dt,                 // #01
			at->RByBill,            // #02
			bll->Code,              // #03
			dbe_acc_dbt,            // #04
			dbe_acc_crd,            // #05
			at->Amount,             // #06
			// @v11.1.12 bll->Memo,              // #07
			dbe_memo,               // #07 @v11.1.12
			dbe_cur,                // #08
			dbe_oprkind,            // #09
			0L);
		if(p_tmp_bill_t) {
			q->from(p_tmp_bill_t, at, bll, 0L);
			dbq = & (at->BillID == p_tmp_bill_t->PrmrID);
		}
		else {
			q->from(at, bll, 0L);
			if(Filt.Flags & AccturnFilt::fLastOnly && Filt.BillID)
				dbq = & (*dbq && at->BillID == Filt.BillID && at->Reverse == 0L);
			else
				dbq = & daterange(at->Dt, &Filt.Period);
		}
		dbq = & (*dbq && dbe_rel_restrict > 0L);
		if(min_amt == max_amt && min_amt != 0.0) {
			min_amt = (modf(min_amt, &ip) == 0.0) ? (ip - 1.0) : ip;
			max_amt = ip + 1.0;
		}
		dbq = & (*dbq && realrange(at->Amount, min_amt, max_amt) && (at->Reverse == 0L));
		if(OpList.getCount())
			dbq = &(*dbq && bll->ID == at->BillID && ppidlist(bll->OpID, &OpList));
		else
			dbq = &(*dbq && (bll->ID += at->BillID));
		q->where(*dbq);
		if(Filt.Flags & AccturnFilt::fLastOnly && Filt.BillID)
			q->orderBy(at->BillID, 0L);
		else
			q->orderBy(at->Dt, at->OprNo, 0L);
	}
	THROW(CheckQueryPtr(q));
	if(Filt.GrpAco) {
		SString cursymb;
		brw_id = !Filt.Cycl ? BROWSER_ATURNGRPNG : BROWSER_ATURNGRPNG_CYCLE;
		PPGetSubStr(PPTXT_GRPACO, Filt.GrpAco - 1, sub_title);
		::GetCurSymbText(((Filt.Flags & AccturnFilt::fAllCurrencies) ? -1L : Filt.CurID), cursymb);
		sub_title.Space().Cat(cursymb);
	}
	else {
		::GetCurSymbText(((Filt.Flags & AccturnFilt::fAllCurrencies) ? -1L : Filt.CurID), sub_title);
		brw_id = BROWSER_ATURNLIST;
	}
	ASSIGN_PTR(pBrwId, brw_id);
	ASSIGN_PTR(pSubTitle, sub_title);
	CATCH
		if(q) {
			ZDELETE(q);
		}
		else {
			delete at;
			delete bll;
			delete p_tmp_bill_t;
			delete p_grp_tbl;
		}
	ENDCATCH
	return q;
}

int PPViewAccturn::AddBillToList(PPID billID)
{
	int    ok = 1;
	if(P_TmpBillTbl && billID) {
		TempAssocTbl::Rec tmp_rec;
		tmp_rec.PrmrID = billID;
		tmp_rec.ScndID = 0;
		THROW_DB(P_TmpBillTbl->insertRecBuf(&tmp_rec));
	}
	CATCHZOK
	return ok;
}

int PPViewAccturn::RemoveBillFromList(PPID billID)
{
	if(P_TmpBillTbl && billID)
		deleteFrom(P_TmpBillTbl, 1, P_TmpBillTbl->PrmrID == billID);
	return 1;
}

/*virtual*/int PPViewAccturn::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = (ppvCmd == PPVCMD_PRINT) ? -2 : PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	int    update = 0;
	if(ok == -2) {
		Hdr    hdr;
		if(pHdr)
			hdr = *static_cast<const Hdr *>(pHdr);
		else
			MEMSZERO(hdr);
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = -1;
				if(!Filt.GrpAco && P_BObj->atobj->CheckRights(PPR_INS) && !Filt.BillID) {
					PPID   id = 0;
					if(P_BObj->AddAccturn(&id, 0) == cmOK) {
						AddBillToList(id);
						ok = update = 1;
					}
				}
				break;
			case PPVCMD_EDITITEM:
				if(Filt.GrpAco)
					ok = Detail(pHdr, pBrw);
				else if((ok = EditItem(hdr.Id)) > 0)
					update = 1;
				break;
			case PPVCMD_DELETEITEM:
				if((ok = DeleteItem(hdr.Id)) > 0)
					update = 1;
				break;
			case PPVCMD_ADDBYSAMPLE:
				ok = -1;
				if(hdr.Id && !Filt.GrpAco && P_BObj->atobj->CheckRights(PPR_INS) && !Filt.BillID) {
					PPID id = 0;
					if(P_BObj->AddAccturnBySample(&id, hdr.Id) == cmOK) {
						AddBillToList(id);
						ok = update = 1;
					}
				}
				break;
			case PPVCMD_EDITBILLINFO:
				ok = P_BObj->ViewBillInfo(hdr.Id);
				break;
			case PPVCMD_PRINT:
				ok = Filt.GrpAco ? Print(0) : PrintItem(hdr.Id);
				break;
			case PPVCMD_ALTPRINT:
				ok = Print(0);
				break;
			case PPVCMD_CVTGENATURNTOEXTACCBILL:
				ok = ConvertGenAccturnToExtAccBill();
				break;
		}
	}
	if(ok > 0 && update > 0) {
		CALLPTRMEMB(pBrw, Update());
	}
	return (update > 0) ? ok : ((ok <= 0) ? ok : -1);
}

/*virtual*/int PPViewAccturn::Browse(int modeless)
{
	return (P_BObj->atobj->CheckRights(PPR_READ)) ? PPView::Browse(modeless) : 0;
}

CvtAt2Ab_Param::CvtAt2Ab_Param() : LocID(0), OpID(0), ObjID(0), ExtObjID(0), Flags(0), P_OpList(0)
{
}

int PPViewAccturn::ConvertGenAccturnToExtAccBill()
{
	class CvtAt2Ab_Dialog : public TDialog {
		DECL_DIALOG_DATA(CvtAt2Ab_Param);
	public:
		CvtAt2Ab_Dialog() : TDialog(DLG_CVTAT2AB), AccSheetID(0), AccSheet2ID(0)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			ushort v;
			PPOprKind op_rec;
			SetupOprKindCombo(this, CTLSEL_CVTAT2AB_OP, 0, 0, Data.P_OpList, OPKLF_OPLIST);
			if(GetOpData(Data.OpID, &op_rec) > 0) {
				AccSheetID  = (op_rec.AccSheetID != 0)  ? op_rec.AccSheetID  : (Data.ObjID = 0);
				AccSheet2ID = (op_rec.AccSheet2ID != 0) ? op_rec.AccSheet2ID : (Data.ExtObjID = 0);
			}
			else
				AccSheet2ID = AccSheetID = Data.ExtObjID = Data.ObjID = 0;
			SetupArCombo(this, CTLSEL_CVTAT2AB_OBJ,    Data.ObjID,    OLW_CANINSERT, AccSheetID, 0);
			SetupArCombo(this, CTLSEL_CVTAT2AB_EXTOBJ, Data.ExtObjID, OLW_CANINSERT, AccSheet2ID, 0);
			SetupPPObjCombo(this, CTLSEL_CVTAT2AB_LOC, PPOBJ_LOCATION, Data.LocID, 0);
			v = 0;
			SETFLAG(v, 0x01, Data.Flags & CvtAt2Ab_Param::fNegAmount);
			setCtrlData(CTL_CVTAT2AB_FLAGS, &v);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			ushort v = 0;
			getCtrlData(CTLSEL_CVTAT2AB_OP,     &Data.OpID);
			getCtrlData(CTLSEL_CVTAT2AB_OBJ,    &Data.ObjID);
			getCtrlData(CTLSEL_CVTAT2AB_EXTOBJ, &Data.ExtObjID);
			getCtrlData(CTLSEL_CVTAT2AB_LOC,    &Data.LocID);
			getCtrlData(CTL_CVTAT2AB_FLAGS,     &(v = 0));
			SETFLAG(Data.Flags, CvtAt2Ab_Param::fNegAmount, v & 0x01);
			if(Data.OpID == 0)
				return (PPError(PPERR_OPRKINDNEEDED, 0), 0);
			else {
				ASSIGN_PTR(pData, Data);
				return 1;
			}
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_CVTAT2AB_OP)) {
				PPOprKind op_rec;
				PPID   op_id = getCtrlLong(CTLSEL_CVTAT2AB_OP);
				if(GetOpData(op_id, &op_rec) > 0) {
					if(op_rec.AccSheet2ID != 0) {
						if(op_rec.AccSheet2ID != AccSheet2ID)
							AccSheet2ID = op_rec.AccSheet2ID;
					}
					else
						AccSheet2ID = Data.ExtObjID = 0;
					if(op_rec.AccSheetID != 0) {
						if(op_rec.AccSheetID != AccSheetID)
							AccSheetID = op_rec.AccSheetID;
					}
					else
						AccSheetID = Data.ObjID = 0;
				}
				else
					AccSheetID = AccSheet2ID = Data.ObjID = Data.ExtObjID = 0;
				SetupArCombo(this, CTLSEL_CVTAT2AB_OBJ,    0, OLW_CANINSERT, AccSheetID, 0);
				SetupArCombo(this, CTLSEL_CVTAT2AB_EXTOBJ, 0, OLW_CANINSERT, AccSheet2ID, 0);
				clearEvent(event);
			}
		}
		PPID   AccSheetID;
		PPID   AccSheet2ID;
	};
	int    ok = -1;
	int    frrl_tag = 0;
	uint   i;
	PPID   op_id;
	CvtAt2Ab_Dialog * dlg = 0;
	AccturnViewItem item;
	if(oneof2(Filt.Aco, ACO_2, ACO_3) && Filt.DbtAcct.ac && Filt.CrdAcct.ac) {
		PPOprKind op_rec;
		PPIDArray op_list;
		int    skip = 0;
		if(IsGenericOp(Filt.OpID) > 0) {
			GetGenericOpList(Filt.OpID, &op_list);
			for(i = 0; !skip && i < op_list.getCount(); i++)
				if(GetOpType(op_list.at(i), &op_rec) != PPOPT_ACCTURN)
					skip = 1;
		}
		else if(GetOpType(Filt.OpID, &op_rec) != PPOPT_ACCTURN)
			skip = 1;
		if(!skip) {
			op_list.freeAll();
			for(op_id = 0; EnumOperations(PPOPT_ACCTURN, &op_id, &op_rec) > 0;) {
				if(op_rec.Flags & OPKF_EXTACCTURN)
					op_list.add(op_id);
			}
			{
				int    valid_data = 0;
				CvtAt2Ab_Param param;
				param.P_OpList = &op_list;
				THROW(CheckDialogPtr(&(dlg = new CvtAt2Ab_Dialog())));
				dlg->setDTS(&param);
				while(!valid_data && ExecView(dlg) == cmOK) {
					if(dlg->getDTS(&param)) {
						PPIDArray bill_id_list;
						valid_data = 1;
						ZDELETE(dlg);
						PPWaitStart();
						{
							PPTransaction tra(1);
							THROW(tra);
							THROW(P_ATC->LockingFRR(1, &frrl_tag, 0));
							for(InitIteration(); NextIteration(&item) > 0;)
								if(P_BObj->Search(item.BillID) > 0)
									bill_id_list.addUnique(item.BillID);
							for(i = 0; i < bill_id_list.getCount(); i++) {
								THROW(P_BObj->ConvertGenAccturnToExtAccBill(bill_id_list.at(i), 0, &param, 0));
								PPWaitPercent(i+1, bill_id_list.getCount());
							}
							THROW(P_ATC->LockingFRR(0, &frrl_tag, 0));
							THROW(tra.Commit());
						}
						PPWaitStop();
						ok = 1;
					}
				}
			}
		}
	}
	CATCH
		P_ATC->LockingFRR(-1, &frrl_tag, 0);
		ok = PPErrorZ();
	ENDCATCH
	delete dlg;
	return ok;
}
