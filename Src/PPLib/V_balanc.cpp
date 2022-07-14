// V_BALANCE.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2006, 2007, 2009, 2010, 2011, 2015, 2016, 2018, 2019, 2020, 2021
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(Balance); BalanceFilt::BalanceFilt() : PPBaseFilt(PPFILT_BALANCE, 0, 0)
{
	SetFlatChunk(offsetof(BalanceFilt, ReserveStart),
		offsetof(BalanceFilt, ReserveEnd) - offsetof(BalanceFilt, ReserveStart) + sizeof(ReserveEnd));
	Init(1, 0);
}

PPViewBalance::PPViewBalance() : PPView(0, &Filt, PPVIEW_BALANCE, implBrowseArray, REPORT_BALANCE), P_ATC(BillObj->atobj->P_Tbl)
{
}

PPViewBalance::~PPViewBalance()
{
}

struct Balance_AccItem {
	PPID   ID;
	int16  Ac;
	int16  Sb;
	PPID   CurID;
	int16  Kind;
};

IMPL_CMPCFUNC(Balance_AccItem_AcSb, p1, p2) { RET_CMPCASCADE2(static_cast<const Balance_AccItem *>(p1), static_cast<const Balance_AccItem *>(p2), Ac, Sb); }

int PPViewBalance::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	Filt.Period.Actualize(ZERODATE);
	THROW(AdjustPeriodToRights(Filt.Period, 0));
	List.freeAll();
	MEMSZERO(Total);
	{
		uint   i;
		int    zero_trnovr;
		int    skip;
		int    th = BIN(Filt.Flags & BALFORM_THOUSAND);
		Balance_AccItem * p_aci;
		SArray aca(sizeof(Balance_AccItem));
		PPAccount acc_rec;
		Balance_AccItem item;
		int16  ac = 0;
		if(Filt.AccID) {
			THROW(AccObj.Search(Filt.AccID, &acc_rec) > 0);
			ac = acc_rec.A.Ac;
		}
		for(SEnum en = AccObj.Enum(0); en.Next(&acc_rec) > 0;) {
			if((!ac || acc_rec.A.Ac == ac) && (uint)acc_rec.Type == Filt.AccType &&
				((Filt.Flags & BALFORM_ALLCUR) || Filt.CurID == acc_rec.CurID) &&
				(!(Filt.Flags & BALFORM_ACO1GROUPING) || acc_rec.A.Sb == 0)) {
				item.ID    = acc_rec.ID;
				item.Ac    = acc_rec.A.Ac;
				item.Sb    = acc_rec.A.Sb;
				item.CurID = acc_rec.CurID;
				item.Kind  = acc_rec.Kind;
				THROW_SL(aca.insert(&item));
			}
		}
		aca.sort(PTR_CMPCFUNC(Balance_AccItem_AcSb));
		//
		for(i = 0; aca.enumItems(&i, (void **)&p_aci);) {
			uint   brf = 0;
			if(Filt.Flags & BALFORM_ACO1GROUPING)
				brf |= BALRESTF_ACO1GROUPING;
			if(Filt.Flags & BALFORM_SPREADBYSUBACC)
				brf |= BALRESTF_SPREADBYSUBACC;
			else if(Filt.Flags & BALFORM_SPREADBYARTICLE)
				brf |= BALRESTF_SPREADBYARTICLE;
			skip = 0;
			zero_trnovr = 0;
			BalanceViewItem entry;
			MEMSZERO(entry);
			entry.AccID = p_aci->ID;
			entry.Ac    = p_aci->Ac;
			entry.Sb    = p_aci->Sb;
			entry.CurID = p_aci->CurID;

			P_ATC->GetBalRest(Filt.Period.low, p_aci->ID, &entry.InDbtRest, &entry.InCrdRest, brf | BALRESTF_INCOMING);
			P_ATC->GetBalRest(Filt.Period.upp, p_aci->ID, &entry.OutDbtRest, &entry.OutCrdRest, brf);
			if(Filt.Flags & BALFORM_SPREAD) {
				brf &= ~BALRESTF_SPREAD;
				double d, c;
				P_ATC->GetBalRest(Filt.Period.upp, p_aci->ID, &entry.DbtTrnovr, &entry.CrdTrnovr, brf);
				P_ATC->GetBalRest(Filt.Period.low, p_aci->ID, &d, &c, brf | BALRESTF_INCOMING);
				entry.DbtTrnovr -= d;
				entry.CrdTrnovr -= c;
			}
			else {
				entry.DbtTrnovr  = entry.OutDbtRest - entry.InDbtRest;
				entry.CrdTrnovr  = entry.OutCrdRest - entry.InCrdRest;
				if(p_aci->Kind == ACT_ACTIVE) {
					entry.OutDbtRest -= entry.OutCrdRest;
					entry.InDbtRest  -= entry.InCrdRest;
					entry.OutCrdRest  = entry.InCrdRest = 0.0;
				}
				else if(p_aci->Kind == ACT_PASSIVE) {
					entry.OutCrdRest -= entry.OutDbtRest;
					entry.InCrdRest  -= entry.InDbtRest;
					entry.OutDbtRest  = entry.InDbtRest = 0.0;
				}
				else if(p_aci->Kind == ACT_AP) {
					//
					// Свертка остатков по активно-пассивным счетам
					//
					if(entry.OutDbtRest >= entry.OutCrdRest) {
						entry.OutDbtRest -= entry.OutCrdRest;
						entry.OutCrdRest  = 0.0;
					}
					else {
						entry.OutCrdRest -= entry.OutDbtRest;
						entry.OutDbtRest  = 0.0;
					}
					if(entry.InDbtRest >= entry.InCrdRest) {
						entry.InDbtRest -= entry.InCrdRest;
						entry.InCrdRest  = 0.0;
					}
					else {
						entry.InCrdRest -= entry.InDbtRest;
						entry.InDbtRest  = 0.0;
					}
				}
			}
			if(entry.DbtTrnovr == 0.0 && entry.CrdTrnovr == 0.0) {
				zero_trnovr = 1;
				if(Filt.Flags & BALFORM_IGNOREZEROTURNOVER)
					skip = 1;
			}
			else {
				if(th) {
					entry.DbtTrnovr = R0(entry.DbtTrnovr / 1000.0);
					entry.CrdTrnovr = R0(entry.CrdTrnovr / 1000.0);
				}
				Total.DbtTrnovr += entry.DbtTrnovr;
				Total.CrdTrnovr += entry.CrdTrnovr;
			}
			if(entry.OutDbtRest == 0 && entry.OutCrdRest == 0) {
				if(Filt.Flags & BALFORM_IGNOREZEROREST)
					skip = 1;
				else if(zero_trnovr && (Filt.Flags & BALFORM_IGNOREZERO))
					skip = 1;
			}
			else {
				if(th) {
					entry.OutDbtRest = R0(entry.OutDbtRest / 1000.0);
					entry.OutCrdRest = R0(entry.OutCrdRest / 1000.0);
				}
				Total.OutDbtRest += entry.OutDbtRest;
				Total.OutCrdRest += entry.OutCrdRest;
			}
			if(th) {
				entry.InDbtRest = R0(entry.InDbtRest / 1000.0);
				entry.InCrdRest = R0(entry.InCrdRest / 1000.0);
			}
			Total.InDbtRest += entry.InDbtRest;
			Total.InCrdRest += entry.InCrdRest;
			if(!skip) {
				if(entry.CurID > 0) {
					PPCurrency cur_rec;
					if(CurObj.Fetch(entry.CurID, &cur_rec) > 0)
						STRNSCPY(entry.CurSymb, cur_rec.Symb);
					else
						ltoa(entry.CurID, entry.CurSymb, 10);
				}
				THROW_SL(List.insert(&entry));
			}
		}
	}
	CATCH
		ok = 0;
		List.freeAll();
	ENDCATCH
	return ok;
}

int PPViewBalance::InitIteration()
{
	IterPos = 0;
	return 1;
}

int FASTCALL PPViewBalance::NextIteration(BalanceViewItem * pItem)
{
	if(IterPos < List.getCount()) {
		ASSIGN_PTR(pItem, List.at(IterPos));
		IterPos++;
		return 1;
	}
	return -1;
}

PPBaseFilt * PPViewBalance::CreateFilt(const void * extraPtr) const
{
	BalanceFilt * p_filt = new BalanceFilt;
	if(p_filt) {
		p_filt->Period.SetDate(getcurdate_());
		p_filt->Flags = BALFORM_IGNOREZERO | BALFORM_ACO1GROUPING;
		p_filt->AccType = ACY_BAL;
		p_filt->AccID = 0;
	}
	return p_filt;
}

class BalanceFiltDialog : public TDialog {
	DECL_DIALOG_DATA(BalanceFilt);
public:
	BalanceFiltDialog() : TDialog(DLG_BALFORM)
	{
		SetupCalPeriod(CTLCAL_BALFORM_PERIOD, CTL_BALFORM_PERIOD);
		setDTS(0);
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData)) {
			Data.Init(1, 0);
			Data.AccType = ACY_BAL;
		}
		PPID   cur_id = Data.CurID;
		SetPeriodInput(this, CTL_BALFORM_PERIOD, &Data.Period);
		::SetupCurrencyCombo(this, CTLSEL_BALFORM_CUR, cur_id, 0, 1, 0);
		ushort v = 0;
		AddClusterAssoc(CTL_BALFORM_OPTIONS, 0, BALFORM_IGNOREZERO);
		AddClusterAssoc(CTL_BALFORM_OPTIONS, 1, BALFORM_THOUSAND);
		AddClusterAssoc(CTL_BALFORM_OPTIONS, 2, BALFORM_ACO1GROUPING);
		SetClusterData(CTL_BALFORM_OPTIONS, Data.Flags);
		if(Data.Flags & BALFORM_SPREADBYSUBACC)
			v = 1;
		else if(Data.Flags & BALFORM_SPREADBYARTICLE)
			v = 2;
		else
			v = 0;
		setCtrlData(CTL_BALFORM_SPREAD, &v);
		AddClusterAssoc(CTL_BALFORM_ALLCUR, 0, BALFORM_ALLCUR);
		SetClusterData(CTL_BALFORM_ALLCUR, Data.Flags);
		if(Data.AccType == ACY_BAL)
			v = 0;
		else if(Data.AccType == ACY_OBAL)
			v = 1;
		else if(Data.AccType == ACY_REGISTER)
			v = 2;
		else
			v = 0;
		setCtrlData(CTL_BALFORM_ACCTYPE, &v);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ushort v = 0;
		PPID   cur_id = 0;
		if(!GetPeriodInput(this, CTL_BALFORM_PERIOD, &Data.Period) || !AdjustPeriodToRights(Data.Period, 1))
			return PPErrorZ();
		getCtrlData(CTLSEL_BALFORM_CUR, &cur_id);
		GetClusterData(CTL_BALFORM_OPTIONS, &Data.Flags);
		getCtrlData(CTL_BALFORM_SPREAD, &(v = 0));
		Data.Flags &= ~(BALFORM_SPREADBYSUBACC|BALFORM_SPREADBYARTICLE);
		switch(v) {
			case 1: Data.Flags |= BALFORM_SPREADBYSUBACC;  break;
			case 2: Data.Flags |= BALFORM_SPREADBYARTICLE; break;
		}
		GetClusterData(CTL_BALFORM_ALLCUR, &Data.Flags);
		getCtrlData(CTL_BALFORM_ACCTYPE, &(v = 0));
		if(v == 0)
			Data.AccType = ACY_BAL;
		else if(v == 1)
			Data.AccType = ACY_OBAL;
		else if(v == 2)
			Data.AccType = ACY_REGISTER;
		else
			Data.AccType = ACY_BAL;
		Data.CurID = (Data.Flags & BALFORM_ALLCUR) ? -1 : cur_id;
		ASSIGN_PTR(pData, Data);
		return 1;
	}
};


int PPViewBalance::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	BalanceFilt * p_filt = static_cast<BalanceFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(BalanceFiltDialog, p_filt);
}

void PPViewBalance::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_BALTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		char   diff_buf[32];
		const  double diff = Total.InDbtRest - Total.InCrdRest + Total.DbtTrnovr - Total.CrdTrnovr - (Total.OutDbtRest - Total.OutCrdRest);
		SetPeriodInput(dlg, CTL_BALTOTAL_PERIOD, &Filt.Period);
		dlg->setCtrlData(CTL_BALTOTAL_INREST_D, &Total.InDbtRest);
		dlg->setCtrlData(CTL_BALTOTAL_INREST_C, &Total.InCrdRest);
		dlg->setCtrlData(CTL_BALTOTAL_TRNOVR_D, &Total.DbtTrnovr);
		dlg->setCtrlData(CTL_BALTOTAL_TRNOVR_C, &Total.CrdTrnovr);
		dlg->setCtrlData(CTL_BALTOTAL_OUTREST_D, &Total.OutDbtRest);
		dlg->setCtrlData(CTL_BALTOTAL_OUTREST_C, &Total.OutCrdRest);
		realfmt(diff, MKSFMTD(0, 2, NMBF_NOZERO), diff_buf);
		dlg->setStaticText(CTL_BALTOTAL_DIFF, diff_buf);
		ExecViewAndDestroy(dlg);
	}
}

// static
int FASTCALL PPViewBalance::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewBalance * p_v = static_cast<PPViewBalance *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

int FASTCALL PPViewBalance::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		SString temp_buf;
		const BalanceViewItem * p_item = static_cast<const BalanceViewItem *>(pBlk->P_SrcData);
		switch(pBlk->ColumnN) {
			case 3: *static_cast<Acct *>(pBlk->P_DestData) = *reinterpret_cast<const Acct *>(&p_item->Ac); break; // Номер счета
			case 4: // Символ валюты
				if(p_item->CurID > 0) {
					PPCurrency cur_rec;
					if(CurObj.Fetch(p_item->CurID, &cur_rec) > 0)
						pBlk->Set(cur_rec.Symb);
					else
						pBlk->Set(ideqvalstr(p_item->CurID, temp_buf));
				}
				else
					pBlk->SetZero();
				break;
			case 5: pBlk->Set(p_item->InDbtRest); break; // Входящий дебет
			case 6: pBlk->Set(p_item->InCrdRest); break; // Входящий кредит
			case 7: pBlk->Set(p_item->DbtTrnovr); break; // Обороты дебет
			case 8: pBlk->Set(p_item->CrdTrnovr); break; // Обороты кредит
			case 9: pBlk->Set(p_item->OutDbtRest); break; // Исходящий дебет
			case 10: pBlk->Set(p_item->OutCrdRest); break; // Исходящий кредит
			case 11: // Наименование счета
				if(p_item->AccID > 0) {
					PPAccount acc_rec;
					if(AccObj.Fetch(p_item->AccID, &acc_rec) > 0)
						pBlk->Set(acc_rec.Name);
					else
						pBlk->Set(ideqvalstr(p_item->AccID, temp_buf));
				}
				else
					pBlk->SetZero();
				break;
		}
	}
	return ok;
}

void PPViewBalance::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, SetDefUserProc(PPViewBalance::GetDataForBrowser, this));
}

SArray * PPViewBalance::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	TSArray <BalanceViewItem> * p_array = new TSArray <BalanceViewItem>;
	ASSIGN_PTR(p_array, List);
	CALLPTRMEMB(pSubTitle, Cat(Filt.Period, 1));
	ASSIGN_PTR(pBrwId, BROWSER_BALANCE);
	return p_array;
}

int PPViewBalance::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		if(ppvCmd == PPVCMD_VIEWACCOUNT) {
			ok = -1;
			const BalanceViewItem * p_item = static_cast<const BalanceViewItem *>(pHdr);
			if(p_item && p_item->AccID) {
				PPObjAccount acc_obj;
				PPID acc_id = p_item->AccID;
				if(acc_obj.Edit(&acc_id, 0) == cmOK)
					ok = 1;
			}
		}
	}
	return ok;
}

int PPViewBalance::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = 1;
	const BalanceViewItem * p_item = static_cast<const BalanceViewItem *>(pHdr);
	if(p_item) {
		if(p_item->AccID) {
			int    is_aco1_grp = BIN(Filt.Flags & BALFORM_ACO1GROUPING);
			PPAccount acc_rec;
			if(AccObj.Search(p_item->AccID, &acc_rec) > 0) {
				if((is_aco1_grp || !acc_rec.A.Sb) && AccObj.HasAnySubacct(acc_rec.A.Ac) > 0 && (!acc_rec.AccSheetID || is_aco1_grp)) {
					if(acc_rec.ID != Filt.AccID && (Filt.Flags & BALFORM_ACO1GROUPING)) {
						BalanceFilt f;
						f.Period = Filt.Period;
						f.AccID = acc_rec.ID;
						f.AccType = Filt.AccType;
						f.CurID = p_item->CurID;
						f.Flags = (Filt.Flags & ~(BALFORM_ACO1GROUPING | BALFORM_ALLCUR));
						PPView::Execute(PPVIEW_BALANCE, &f, 1, 0);
					}
				}
				else {
					AccAnlzFilt aaflt;
					aaflt.Period = Filt.Period;
					aaflt.Aco = ACO_2;
					aaflt.AccID = acc_rec.ID;
					aaflt.CurID = p_item->CurID;
					if(acc_rec.AccSheetID)
						aaflt.Flags |= AccAnlzFilt::fTrnovrBySheet;
					ViewAccAnlz(&aaflt, aakndGeneric);
				}
			}
		}
	}
	else
		ok = -1;
	return ok;
}
//
// Implementation of PPALDD_Balance
//
PPALDD_CONSTRUCTOR(Balance)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(Balance) { Destroy(); }

int PPALDD_Balance::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Balance, rsrv);
	H.FltBeg   = p_filt->Period.low;
	H.FltEnd   = p_filt->Period.upp;
	H.FltCurID = p_filt->CurID;
	H.FltAccID = p_filt->AccID;
	H.FltFlags = p_filt->Flags;
	H.fAllCurrencies = BIN(p_filt->Flags & BALFORM_ALLCUR);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_Balance::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(Balance);
}

int PPALDD_Balance::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Balance);
	I.Dt     = item.Dt;
	I.AccID  = item.AccID;
	I.CurID  = item.CurID;
	I.InDbtRest  = item.InDbtRest;
	I.InCrdRest  = item.InCrdRest;
	I.DbtTrnovr  = item.DbtTrnovr;
	I.CrdTrnovr  = item.CrdTrnovr;
	I.OutDbtRest = item.OutDbtRest;
	I.OutCrdRest = item.OutCrdRest;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_Balance::Destroy() { DESTROY_PPVIEW_ALDD(Balance); }
