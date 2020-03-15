// V_ACANLZ.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <graph.h>
//
// Utility
//
static SString & FASTCALL GetAccAnlzTitle(int aco, PPID accID, PPID curID, SString & rStr)
{
	char   acc_buf[64];
	PPID   cur_id = 0;
	PPCurrency cur_rec;
	Acct   acct;
	AccIDToAcct(accID, aco, &acct);
	acct.ToStr(ACCF_DEFAULT, acc_buf);
	rStr = acc_buf;
	rStr.Space();
	if(curID < 0)
		rStr.CatCharN('*', 3);
	else if(curID == 0) {
		cur_id = LConfig.BaseCurID;
		rStr.CatChar('*');
	}
	else
		cur_id = curID;
	if(SearchObject(PPOBJ_CURRENCY, cur_id, &cur_rec) > 0)
		rStr.Cat(cur_rec.Symb);
	return rStr;
}

IMPLEMENT_PPFILT_FACTORY(AccAnlz); SLAPI AccAnlzFilt::AccAnlzFilt() : PPBaseFilt(PPFILT_ACCANLZ, 0, 1)
{
	SetFlatChunk(offsetof(AccAnlzFilt, ReserveStart), offsetof(AccAnlzFilt, ReserveEnd) - offsetof(AccAnlzFilt, ReserveStart));
	Init(1, 0);
}

AccAnlzFilt & FASTCALL AccAnlzFilt::operator = (const AccAnlzFilt & s)
{
	Copy(&s, 1);
	return *this;
}

char * SLAPI AccAnlzFilt::GetAccText(char * pBuf, size_t bufLen) const
{
	SString buf, name;
	PPObjAccount acc_obj;
	PPAccount acc_rec;
	if(acc_obj.Fetch(AcctId.ac, &acc_rec) > 0) {
		ArticleTbl::Rec ar_rec;
		Acct acc;
		acc.ac = acc_rec.A.Ac;
		acc.sb = acc_rec.A.Sb;
		acc.ar = 0;
		(name = acc_rec.Name).Strip();
		PPID   ar_id = (acc_rec.Type == ACY_AGGR) ? SingleArID : AcctId.ar;
		if(ar_id) {
			PPObjArticle ar_obj;
			if(ar_obj.Fetch(ar_id, &ar_rec) > 0) {
				acc.ar = ar_rec.Article;
				name.Dot().Cat(ar_rec.Name);
			}
			else
				ar_id = 0;
		}
		acc.ToStr(ACCF_DEFAULT, buf).Space().Cat(name.Quot('(', ')'));
	}
	return strnzcpy(pBuf, buf, bufLen);
}
//
//
//
SLAPI PPViewAccAnlz::PPViewAccAnlz() : PPView(0, &Filt, PPVIEW_ACCANLZ), P_BObj(BillObj), P_TmpAATbl(0), P_TmpATTbl(0), EffDlvrLocID(0)
{
	P_ATC = P_BObj->atobj->P_Tbl;
}

SLAPI PPViewAccAnlz::~PPViewAccAnlz()
{
	delete P_TmpAATbl;
	delete P_TmpATTbl;
	DBRemoveTempFiles();
}

// virtual
PPBaseFilt * SLAPI PPViewAccAnlz::CreateFilt(void * extraPtr) const
{
	const LDATE oper_date = LConfig.OperDate;
	const Acct & r_cash_acct = CConfig.CashAcct;
	const Acct & r_suppl_acct = CConfig.SupplAcct;
	AccAnlzKind kind = (AccAnlzKind)reinterpret_cast<long>(extraPtr);
	AccAnlzFilt * p_filt = new AccAnlzFilt;
	switch(kind) {
		case aakndCashBook:
			p_filt->Period.SetDate(oper_date);
			p_filt->Flags |= AccAnlzFilt::fAsCashBook;
			p_filt->LeafNo = 1;
			P_ATC->ConvertAcct(&r_cash_acct, 0L /* @curID */, &p_filt->AcctId, &p_filt->AccSheetID);
			break;
		case aakndAccTrnovr:
			p_filt->Flags |= AccAnlzFilt::fTrnovrBySheet;
			break;
		case aakndSupplTrnovr:
			p_filt->Flags |= (AccAnlzFilt::fTrnovrBySheet | AccAnlzFilt::fTrnovrBySuppl);
			P_ATC->ConvertAcct(&r_suppl_acct, 0L /* @curID */, &p_filt->AcctId, &p_filt->AccSheetID);
			break;
		case aakndGeneric:
		default:
			p_filt->Period.SetDate(oper_date);
			break;
	}
	return p_filt;
}
//
//
//
#define GRP_ACC   1
#define GRP_CYCLE 2
#define GRP_LOC   3

class AccAnlzFiltDialog : public WLDialog {
	DECL_DIALOG_DATA(AccAnlzFilt);
public:
	AccAnlzFiltDialog(uint dlgID, PPObjAccTurn * _ppobj) : WLDialog(dlgID, CTL_ACCANLZ_LABEL), ATObj(_ppobj), RelComboInited(0)
	{
		AcctCtrlGroup * p_acc_grp = new AcctCtrlGroup(CTL_ACCANLZ_ACC, CTL_ACCANLZ_ART, CTLSEL_ACCANLZ_ACCNAME, CTLSEL_ACCANLZ_ARTNAME);
		addGroup(GRP_ACC, p_acc_grp);
		CycleCtrlGroup * p_cycle_grp = new CycleCtrlGroup(CTLSEL_ACCANLZ_CYCLE, CTL_ACCANLZ_NUMCYCLES, CTL_ACCANLZ_PERIOD);
		addGroup(GRP_CYCLE, p_cycle_grp);
		SetupCalPeriod(CTLCAL_ACCANLZ_PERIOD, CTL_ACCANLZ_PERIOD);
	}
	DECL_DIALOG_SETDTS()
	{
		ushort v;
		AcctCtrlGroup::Rec  acc_rec;
		CycleCtrlGroup::Rec cycle_rec;
		Data.Init(1, 0);
		RVALUEPTR(Data, pData);
		if(Data.Flags & AccAnlzFilt::fAsCashBook) {
			if(Data.Period.IsZero())
				Data.Period.SetDate(LConfig.OperDate);
			disableCtrls(1, CTL_ACCANLZ_ACCGRP, CTLSEL_ACCANLZ_SUBST, CTL_ACCANLZ_CORACCGRP, CTL_ACCANLZ_ACC,
				CTLSEL_ACCANLZ_ACCNAME, CTL_ACCANLZ_ART, CTLSEL_ACCANLZ_ARTNAME, 0);
		}
		else if(Data.AcctId.ac) {
			PPID   temp_acc_id = 0;
			PPAccount acc_rec;
			if(ATObj->P_Tbl->AccObj.SearchBase(Data.AcctId.ac, &temp_acc_id, &acc_rec) > 0)
				Data.AcctId.ac = temp_acc_id;
			else
				PPError();
		}
		SetPeriodInput(this, CTL_ACCANLZ_PERIOD, &Data.Period);
		setCtrlData(CTL_ACCANLZ_LEAF, &Data.LeafNo);
		setCtrlUInt16(CTL_ACCANLZ_ACCGRP, (Data.Aco == ACO_1) ? 1 : ((Data.Aco == ACO_2) ? 2 : 0));
		SetupSubstCombo();
		setWL(BIN(Data.Flags & AccAnlzFilt::fLabelOnly));
		acc_rec.AcctId      = Data.AcctId;
		if(Data.Aco == ACO_2 && Data.Flags & AccAnlzFilt::fTrnovrBySheet)
			acc_rec.AcctId.ar = Data.SingleArID;
		acc_rec.AccSheetID  = Data.AccSheetID;
		acc_rec.AccSelParam = -100;
		setGroupData(GRP_ACC, &acc_rec);
		SetupSubstRelCombo();
		cycle_rec.C = Data.Cycl;
		setGroupData(GRP_CYCLE, &cycle_rec);

		AddClusterAssoc(CTL_ACCANLZ_TRNOVR, 0, AccAnlzFilt::fTrnovrBySheet);
		AddClusterAssoc(CTL_ACCANLZ_TRNOVR, 1, AccAnlzFilt::fSpprZTrnovr);
		SetClusterData(CTL_ACCANLZ_TRNOVR, Data.Flags);

		AddClusterAssoc(CTL_ACCANLZ_EXCLINNRT, 0, AccAnlzFilt::fExclInnerTrnovr);
		SetClusterData(CTL_ACCANLZ_EXCLINNRT, Data.Flags);
		v = BIN(Data.Flags & AccAnlzFilt::fExclInnerTrnovr);
		disableCtrls(v, CTL_ACCANLZ_ACCGRP, CTLSEL_ACCANLZ_SUBST, CTL_ACCANLZ_CORACCGRP, 0);
		disableCtrls(v, CTLSEL_ACCANLZ_CYCLE, CTL_ACCANLZ_NUMCYCLES, 0);
		ReplyAccSelected();
		AddClusterAssoc(CTL_ACCANLZ_ALLCUR, 0, AccAnlzFilt::fAllCurrencies);
		SetClusterData(CTL_ACCANLZ_ALLCUR, Data.Flags);
		ReplyTrnovrSelected(Data.Flags & AccAnlzFilt::fTrnovrBySheet);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1, r;
		ushort v = 0;
		AcctCtrlGroup::Rec  acc_rec;
		CycleCtrlGroup::Rec cycle_rec;
		ArticleTbl::Rec ar_rec;
		int    is_ar_grouping = 0;
		PPID   rel = 0;
		// @v10.6.4 MEMSZERO(ar_rec);
		THROW(GetPeriodInput(this, CTL_ACCANLZ_PERIOD, &Data.Period));
		getCtrlData(CTL_ACCANLZ_LEAF, &Data.LeafNo);
		v = getCtrlUInt16(CTL_ACCANLZ_ACCGRP);
		Data.Aco = (v == 1) ? ACO_1 : ((v == 2) ? ACO_2 : ACO_3);
		Data.CorAco = getCtrlLong(CTLSEL_ACCANLZ_SUBST);
		THROW(getGroupData(GRP_ACC, &acc_rec));
		THROW_PP(acc_rec.AcctId.ac, PPERR_ACCNOTVALID);
		if(acc_rec.AccType == ACY_AGGR) {
			if(acc_rec.AcctId.ar) {
				rel = acc_rec.AcctId.ac;
				Data.SingleArID = acc_rec.AcctId.ar;
			}
		}
		// @v9.5.9 {
		if(acc_rec.AcctId.ar) {
			if(ATObj->P_Tbl->Art.Search(acc_rec.AcctId.ar, &ar_rec) > 0) {
				if(ar_rec.Flags & ARTRF_GROUP)
					is_ar_grouping = 1;
			}
			else
				ar_rec.ID = 0;
		}
		// } @v9.5.9
		Data.SubstRelTypeID = 0;
		if(acc_rec.AccSheetID) {
			GetClusterData(CTL_ACCANLZ_TRNOVR, &Data.Flags);
			PPObjAccSheet acs_obj;
			PPAccSheet acs_rec;
			if(acs_obj.Fetch(acc_rec.AccSheetID, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON) {
				GetClusterData(CTL_ACCANLZ_TRNOVR, &Data.Flags);
				if(Data.Flags & AccAnlzFilt::fTrnovrBySheet || acc_rec.AcctId.ar)
					Data.SubstRelTypeID = getCtrlLong(CTLSEL_ACCANLZ_RELSUBST);
			}
		}
		else
			Data.Flags &= ~AccAnlzFilt::fTrnovrBySheet;
		GetClusterData(CTL_ACCANLZ_EXCLINNRT, &Data.Flags);
		if(Data.Flags & AccAnlzFilt::fTrnovrBySheet) {
			Data.Flags &= ~AccAnlzFilt::fGroupByCorAcc;
			Data.Aco = ACO_2;
			Data.CorAco = 0;
		}
		if(Data.Aco == ACO_3 && acc_rec.AccType != ACY_AGGR && !is_ar_grouping) { // @v9.5.9 (&& !is_ar_grouping)
			THROW(r = ATObj->P_Tbl->AcctIDToRel(&acc_rec.AcctId, &rel));
			if(r < 0) {
				rel = 0;
				THROW_PP(acc_rec.AcctId.ar == 0, PPERR_ACCRELABSENCE);
				Data.Aco = ACO_2;
			}
		}
		if(Data.Aco == ACO_2) {
			if(Data.Flags & AccAnlzFilt::fTrnovrBySheet)
				Data.SingleArID = acc_rec.AcctId.ar;
			else {
				PPAccount acr;
				THROW(ATObj->P_Tbl->AccObj.Search(acc_rec.AcctId.ac, &acr) > 0);
				if(acr.Flags & ACF_HASBRANCH && rel == 0 && !acr.AccSheetID)
					Data.Aco = ACO_1;
			}
		}
		if(Data.Aco != ACO_3) {
			rel = acc_rec.AcctId.ac;
			acc_rec.AcctId.ar = 0;
		}
		Data.AccID      = rel;
		Data.AcctId     = acc_rec.AcctId;
		Data.AccSheetID = acc_rec.AccSheetID;
		getGroupData(GRP_CYCLE, &cycle_rec);
		Data.Cycl = cycle_rec.C;
		getCtrlData(CTLSEL_ACCANLZ_CUR, &Data.CurID);
		GetClusterData(CTL_ACCANLZ_ALLCUR, &Data.Flags);
		SETFLAG(Data.Flags, AccAnlzFilt::fLabelOnly, getWL());
		if(Data.Flags & AccAnlzFilt::fAllCurrencies)
			Data.CurID = -1;
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERR
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		WLDialog::handleEvent(event);
		if(event.isCmd(cmPPAccSelected))
			ReplyAccSelected();
		else if(event.isCmd(cmPPArSelected)) {
			if(getCtrlLong(CTLSEL_ACCANLZ_ARTNAME))
				setCtrlUInt16(CTL_ACCANLZ_ACCGRP, 0);
			SetupSubstRelCombo();
		}
		else if(event.isClusterClk(CTL_ACCANLZ_TRNOVR)) {
			ReplyTrnovrSelected(getCtrlUInt16(CTL_ACCANLZ_TRNOVR) & 1);
		}
		else if(event.isCbSelected(CTLSEL_ACCANLZ_SUBST)) {
			Data.CorAco = getCtrlLong(CTLSEL_ACCANLZ_SUBST);
			if(Data.CorAco)
				setCtrlUInt16(CTL_ACCANLZ_ORDER, 0);
			disableCtrl(CTL_ACCANLZ_ORDER, BIN(Data.CorAco));
		}
		else if(event.isCmd(cmaMore)) {
			TDialog * dlg = new TDialog(DLG_ACCANLZ2);
			if(CheckDialogPtrErr(&dlg)) {
				getDTS(0); // @v10.5.0
				SetupPPObjCombo(dlg, CTLSEL_ACCANLZ_LOC, PPOBJ_LOCATION, Data.LocID, OLW_CANSELUPLEVEL, 0);
				SetupArCombo(dlg, CTLSEL_ACCANLZ_AGENT, Data.AgentID, OLW_LOADDEFONOPEN, GetAgentAccSheet(), sacfDisableIfZeroSheet);
				// @v10.5.0 {
				PPID  psn_id = 0;
				if(Data.Aco == ACO_3 && Data.AcctId.ar) {
					psn_id = ObjectToPerson(Data.AcctId.ar, 0);
					// SetupLocationCombo(dlg, CTLSEL_ACCANLZ_DLVRLOC, Filt.DlvrLocID, 0, LOCTYP_ADDRESS, psn_id);
					PsnObj.SetupDlvrLocCombo(dlg, CTLSEL_ACCANLZ_DLVRLOC, psn_id, Data.DlvrLocID);
				}
				// } @v10.5.0
				dlg->AddClusterAssocDef(CTL_ACCANLZ_ORDER,  0, PPViewAccAnlz::OrdByDefault);
				dlg->AddClusterAssoc(CTL_ACCANLZ_ORDER,  1, PPViewAccAnlz::OrdByBillCode_Date);
				dlg->AddClusterAssoc(CTL_ACCANLZ_ORDER,  2, PPViewAccAnlz::OrdByCorrAcc_Date);
				dlg->SetClusterData(CTL_ACCANLZ_ORDER, Data.InitOrder);
				for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
					dlg->getCtrlData(CTLSEL_ACCANLZ_LOC, &Data.LocID);
					dlg->getCtrlData(CTLSEL_ACCANLZ_AGENT, &Data.AgentID);
					dlg->GetClusterData(CTL_ACCANLZ_ORDER, &Data.InitOrder);
					Data.DlvrLocID = psn_id ? dlg->getCtrlLong(CTLSEL_ACCANLZ_DLVRLOC) : 0;
					valid_data = 1;
				}
			}
			delete dlg;
		}
		else
			return;
		clearEvent(event);
	}
	void   ReplyAccSelected()
	{
		PPIDArray cur_list;
		AcctCtrlGroup::Rec acg_rec;
		PPID   cur_id = getCtrlLong(CTLSEL_ACCANLZ_CUR);
		SETIFZ(cur_id, Data.CurID);
		getGroupData(GRP_ACC, &acg_rec);
		if(acg_rec.AccSheetID)
			disableCtrls(0, CTL_ACCANLZ_ACCGRP, CTL_ACCANLZ_TRNOVR, 0);
		else {
			disableCtrl(CTL_ACCANLZ_TRNOVR, 1);
			setCtrlUInt16(CTL_ACCANLZ_TRNOVR, 0);
			disableCtrls(0, CTL_ACCANLZ_ACCGRP, CTLSEL_ACCANLZ_SUBST, CTL_ACCANLZ_CORACCGRP, 0);
			disableCtrls(0, CTLSEL_ACCANLZ_CYCLE, CTL_ACCANLZ_NUMCYCLES, 0);
		}
		if(acg_rec.AccType == ACY_AGGR) {
			ObjRestrictArray ext_gen_acc_list;
			PPAccount acc_rec;
			ATObj->P_Tbl->GetExtentAccListByGen(acg_rec.AcctId.ac, &ext_gen_acc_list, &cur_list);
			setCtrlUInt16(CTL_ACCANLZ_ACCGRP, 2);
			disableCtrl(CTL_ACCANLZ_ACCGRP, 1);
			if(ATObj->P_Tbl->AccObj.Search(acg_rec.AcctId.ac, &acc_rec) > 0)
				setCtrlUInt16(CTL_ACCANLZ_EXCLINNRT, BIN(acc_rec.Flags & ACF_EXCLINNERTRNOVR));
		}
		else
			ATObj->P_Tbl->AccObj.GetCurList(acg_rec.AcctId.ac, 0, &cur_list);
		if(!cur_list.lsearch(cur_id))
			cur_id = 0;
		::SetupCurrencyCombo(this, CTLSEL_ACCANLZ_CUR, cur_id, 0, 1, &cur_list);
		SetupSubstRelCombo();
	}
	void   ReplyTrnovrSelected(int trnovr)
	{
		if(trnovr) {
			setCtrlUInt16(CTL_ACCANLZ_ACCGRP,    2); // By ACO_2
			setCtrlLong(CTLSEL_ACCANLZ_SUBST,    0); // No grouping by CorrAcc
			setCtrlUInt16(CTL_ACCANLZ_ORDER,     0);
		}
		disableCtrls(trnovr, CTL_ACCANLZ_ACCGRP, CTLSEL_ACCANLZ_SUBST, CTL_ACCANLZ_CORACCGRP, 0);
		disableCtrls(trnovr, CTLSEL_ACCANLZ_CYCLE, CTL_ACCANLZ_NUMCYCLES, 0);
		disableCtrl(CTL_ACCANLZ_ORDER, trnovr);
		SetupSubstRelCombo();
	}
	void   SetupSubstRelCombo()
	{
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		AcctCtrlGroup::Rec acg_rec;
		getGroupData(GRP_ACC, &acg_rec);
		if(acg_rec.AccSheetID && acs_obj.Fetch(acg_rec.AccSheetID, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON) {
			GetClusterData(CTL_ACCANLZ_TRNOVR, &Data.Flags);
			if(Data.Flags & AccAnlzFilt::fTrnovrBySheet || acg_rec.AcctId.ar) {
				if(!RelComboInited) {
					PPObjPersonRelType rel_obj;
					PPPersonRelType rel_item;
					StrAssocArray list;
					for(PPID rel_id = 0; rel_obj.EnumItems(&rel_id, &rel_item) > 0;)
						if(rel_item.Cardinality & (PPPersonRelType::cOneToOne | PPPersonRelType::cManyToOne)) {
							list.Add(rel_item.ID, rel_item.Name);
						}
					SetupStrAssocCombo(this, CTLSEL_ACCANLZ_RELSUBST, &list, Data.SubstRelTypeID, 0);
					RelComboInited = 1;
				}
				disableCtrl(CTLSEL_ACCANLZ_RELSUBST, 0);
			}
			else
				disableCtrl(CTLSEL_ACCANLZ_RELSUBST, 1);
		}
		else
			disableCtrl(CTLSEL_ACCANLZ_RELSUBST, 1);
	}
	int    SetupSubstCombo()
	{
		SString buf, id_buf, txt_buf, word_rel;
		StrAssocArray list;
		PPLoadText(PPTXT_SUBSTACCANLZ, buf);
		StringSet ss(';', buf);
		for(uint i = 0; ss.get(&i, buf) > 0;)
			if(buf.Divide(',', id_buf, txt_buf) > 0)
				list.Add(id_buf.ToLong(), txt_buf);
		{
			PPObjPersonRelType rel_obj;
			PPPersonRelType rel_item;
			// @v10.5.9 PPGetWord(PPWORD_RELATION, 0, word_rel);
			PPLoadString("relation", word_rel); // @v10.5.9
			for(PPID rel_id = 0; rel_obj.EnumItems(&rel_id, &rel_item) > 0;)
				if(rel_item.Cardinality & (PPPersonRelType::cOneToOne | PPPersonRelType::cManyToOne)) {
					(buf = word_rel).CatChar(':').Cat(rel_item.Name);
					list.Add(rel_id + (long)AccAnlzFilt::aafgFirstRelation, buf);
				}
		}
		return SetupStrAssocCombo(this, CTLSEL_ACCANLZ_SUBST, &list, Data.CorAco, 0);
	}

	PPObjAccTurn * ATObj;
	PPObjPerson PsnObj;
	int    RelComboInited;
};

int SLAPI PPViewAccAnlz::EditSupplTrnovrFilt(AccAnlzFilt * pFilt)
{
	int    ok = -1, valid_data = 0;
	int    search;
	PPAccSheet acc_sheet_rec;
	TDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_SPLTOFLT))));
	THROW(P_BObj->atobj->ConvertAcct(&CConfig.SupplAcct, 0 /* @curID */, &pFilt->AcctId, &pFilt->AccSheetID));
	THROW(search = SearchObject(PPOBJ_ACCSHEET, pFilt->AccSheetID, &acc_sheet_rec));
	THROW_PP(search > 0, PPERR_INVACCSUPPL);
	SetupCalCtrl(CTLCAL_SPLTOFLT_PERIOD, dlg, CTL_SPLTOFLT_PERIOD, 1);
	SetPeriodInput(dlg, CTL_SPLTOFLT_PERIOD, &pFilt->Period);
	SetupArCombo(dlg, CTLSEL_SPLTOFLT_SUPPL, pFilt->SingleArID, OLW_LOADDEFONOPEN, pFilt->AccSheetID, 0);
	dlg->AddClusterAssoc(CTL_SPLTOFLT_FLAGS, 0, AccAnlzFilt::fSpprZTrnovr);
	dlg->AddClusterAssoc(CTL_SPLTOFLT_FLAGS, 1, AccAnlzFilt::fSpprZSaldo);
	dlg->SetClusterData(CTL_SPLTOFLT_FLAGS, pFilt->Flags);
	SetupPPObjCombo(dlg, CTLSEL_SPLTOFLT_LOC, PPOBJ_LOCATION, pFilt->LocID, 0, 0);
	while(!valid_data && ExecView(dlg) == cmOK)
		if(!GetPeriodInput(dlg, CTL_SPLTOFLT_PERIOD, &pFilt->Period))
			PPErrorByDialog(dlg, CTL_SPLTOFLT_PERIOD);
		else {
			dlg->getCtrlData(CTLSEL_SPLTOFLT_LOC, &pFilt->LocID);
			dlg->getCtrlData(CTLSEL_SPLTOFLT_SUPPL, &pFilt->SingleArID);
			dlg->GetClusterData(CTL_SPLTOFLT_FLAGS, &pFilt->Flags);
			pFilt->Flags &= ~AccAnlzFilt::fAsCashBook;
			pFilt->Flags |= AccAnlzFilt::fTrnovrBySheet;
			pFilt->Aco   = ACO_2;
			pFilt->AccID = pFilt->AcctId.ac;
			ok = valid_data = 1;
		}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

// virtual
int SLAPI PPViewAccAnlz::EditBaseFilt(PPBaseFilt * pFilt)
{
	int    ok = -1, valid_data = 0;
	AccAnlzFilt * p_filt = static_cast<AccAnlzFilt *>(pFilt);
	THROW_INVARG(p_filt);
	if(p_filt->Flags & AccAnlzFilt::fTrnovrBySuppl)
		ok = EditSupplTrnovrFilt(p_filt);
	else {
		uint   dlg_id = (p_filt->Flags & AccAnlzFilt::fAsCashBook) ? DLG_CASHBOOK : DLG_ACCANLZ;
		AccAnlzFiltDialog * dlg = new AccAnlzFiltDialog(dlg_id, P_BObj->atobj);
		if(CheckDialogPtrErr(&dlg)) {
			dlg->setDTS(p_filt);
			while(!valid_data && ExecView(dlg) == cmOK)
				if(dlg->getDTS(p_filt)) {
					SETFLAG(p_filt->Flags, AccAnlzFilt::fGroupByCorAcc, p_filt->CorAco);
					ok = valid_data = 1;
				}
		}
		else
			ok = 0;
		delete dlg;
	}
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI PPViewAccAnlz::GetTotal(AccAnlzTotal * pTotal) const
{
	ASSIGN_PTR(pTotal, Total);
	return 1;
}

int SLAPI PPViewAccAnlz::ViewGraph(const PPViewBrowser * pBrw)
{
	struct I {
		LDATE  Dt;
		long   OprNo;
		long   N;      // Номер операции за день строго по отчету [1..di(Dt).Count]
		double IR;
		double D;
		double C;
		double OR;
	};
	struct DI {
		LDATE  Dt;
		long   Count;
	};
	int    ok = -1;
	const  int col = pBrw ? pBrw->GetCurColumn() : -1;
	SString temp_buf;
	Generator_GnuPlot plot(0);
	Generator_GnuPlot::PlotParam param;
	if(Filt.Flags & AccAnlzFilt::fTrnovrBySheet) {
	}
	else if(Filt.Flags & AccAnlzFilt::fGroupByCorAcc) {
	}
	/*
	else  if(Filt.Cycl.Cycle) {
	}
	*/
	else {
		plot.Preamble();
		if((!Filt.Cycl.Cycle && col == 6) || (Filt.Cycl.Cycle && oneof2(col, 2, 5))) {
			{
				PPGpPlotItem item(plot.GetDataFileName(), 0, PPGpPlotItem::sFilledCurves);
				item.Style.SetLine(GetColorRef(SClrSnow), 1);
				item.AddDataIndex(1);
				item.AddDataIndex(3);
				item.AddDataIndex(6);
				plot.AddPlotItem(item);
			}
			{
				PPLoadString("inrest", temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				PPGpPlotItem item(0, temp_buf, PPGpPlotItem::sLines);
				item.Style.SetLine(GetColorRef(SClrGreen), 1);
				item.AddDataIndex(1);
				item.AddDataIndex(3);
				plot.AddPlotItem(item);
			}
			{
				PPLoadString("outrest", temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				PPGpPlotItem item(0, temp_buf, PPGpPlotItem::sLines);
				item.Style.SetLine(GetColorRef(SClrBlue), 2);
				item.AddDataIndex(1);
				item.AddDataIndex(6);
				plot.AddPlotItem(item);
			}
		}
		else {
			{
				PPLoadString("debit", temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				PPGpPlotItem item(plot.GetDataFileName(), temp_buf, PPGpPlotItem::sLines);
				item.Style.SetLine(LightenColor(GetColorRef(SClrRed), 0.5f), 2);
				item.AddDataIndex(1);
				item.AddDataIndex(4);
				plot.AddPlotItem(item);
			}
			{
				PPLoadString("credit", temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				PPGpPlotItem item(0, temp_buf, PPGpPlotItem::sLines);
				item.Style.SetLine(LightenColor(GetColorRef(SClrBlue), 0.5f), 2);
				item.AddDataIndex(1);
				item.AddDataIndex(5);
				plot.AddPlotItem(item);
			}
		}
		plot.SetTitle((temp_buf = pBrw->getTitle()).Transf(CTRANSF_INNER_TO_OUTER));
		plot.SetDateTimeFormat(Generator_GnuPlot::axX, 1);
		plot.SetGrid();
		plot.Plot(&param);
		AccAnlzViewItem item;
		SArray list(sizeof(I));
		SArray di_list(sizeof(DI));
		for(InitIteration(); NextIteration(&item) > 0;) {
			I it;
			MEMSZERO(it);
			it.Dt = item.Dt;
			it.OprNo = item.OprNo;
			it.IR = item.InRest;
			SETIFZ(it.IR, item.OutRest - item.DbtAmt + item.CrdAmt);
			it.D = item.DbtAmt;
			it.C = item.CrdAmt;
			it.OR = item.OutRest;
			list.insert(&it);
		}
		list.sort(PTR_CMPFUNC(_2long));
		{
			uint   i;
			DI     di;
			MEMSZERO(di);
			for(i = 0; i < list.getCount(); i++) {
				I * p_it = (I *)list.at(i);
				p_it->N = ++di.Count;
				if(p_it->Dt != di.Dt) {
					if(di.Dt)
						di_list.insert(&di);
					di.Dt = p_it->Dt;
					p_it->N = di.Count = 1;
				}
			}
			if(di.Dt)
				di_list.insert(&di);
			plot.StartData(1);
			for(i = 0; i < list.getCount(); i++) {
				I * p_it = (I *)list.at(i);
				uint di_pos = 0;
				LDATETIME dtm;
				dtm.d = p_it->Dt;
				if(di_list.bsearch(&p_it->Dt, &di_pos, CMPF_LONG)) {
					const DI * p_di = (DI *)di_list.at(di_pos);
					dtm.t.settotalsec((long)((86400.0 * p_it->N) / p_di->Count));
				}
				else
					dtm.t.settotalsec(0);
				plot.PutData(dtm);           // #1 #2
				plot.PutData(p_it->IR);      // #3
				if(p_it->D != 0.0)
					plot.PutData(p_it->D);   // #4
				else
					plot.PutData("-", 1);    // #4
				if(p_it->C != 0.0)
					plot.PutData(p_it->C);   // #5
				else
					plot.PutData("-", 1);    // #5
				plot.PutData(p_it->OR);      // #6
				plot.PutEOR();
			}
			plot.PutEndOfData();
		}
		ok = plot.Run();
	}
	return ok;
}

int SLAPI PPViewAccAnlz::FetchBill(PPID billID, BillEntry * pEntry)
{
	int    ok = -1;
	BillTbl::Rec rec;
	if(P_BObj->Fetch(billID, &rec) > 0) {
		if(pEntry) {
			pEntry->ID = rec.ID;
			pEntry->LocID = rec.LocID;
			pEntry->OpID = rec.OpID;
			pEntry->Object2ID = rec.Object2;
			pEntry->Flags = rec.Flags;
			pEntry->LinkBillID = rec.LinkBillID; // @v10.5.2
			pEntry->AgentID = 0;
			if(Filt.AgentID || Filt.CorAco == AccAnlzFilt::aafgByAgent) {
				PPBillExt bext_rec;
				if(P_BObj->FetchExt(billID, &bext_rec) > 0)
					pEntry->AgentID = bext_rec.AgentID;
			}
		}
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewAccAnlz::GetAcctRel(PPID accID, PPID arID, AcctRelTbl::Rec * pRec, int use_ta)
{
	int    ok = -1;
	int    cr = 0;
	if(Filt.SubstRelTypeID && arID) {
		PPIDArray temp_list;
		if(ArObj.GetRelPersonList(arID, Filt.SubstRelTypeID, 1, &temp_list) > 0)
			cr = 1;
	}
	return P_ATC->GetAcctRel(accID, arID, pRec, cr, use_ta);
}

int SLAPI PPViewAccAnlz::EnumerateByIdentifiedAcc(long aco, PPID accID, AccAnlzViewEnumProc proc, void * extraPtr)
{
	int    ok = 1, r;
	PROFILE_START
	int    aco2;
	const  int mult = (aco < 0) ? -1 : 1;
	uint   i;
	Acct   acct;
	SArray * p_acct_list = 0;
	ObjRestrictItem * p_item = 0;
	PPObjLocation loc_obj;
	union {
		AccTurnTbl::Key1 k1;
		AccTurnTbl::Key3 k3;
	} k;
	PPAccount acc_rec;
	AcctRelTbl::Rec acrel_rec;
	int    idx;
	BExtQuery * q   = 0;
	DBQ  * dbq = 0;
	if(Filt.Flags & AccAnlzFilt::fExclInnerTrnovr) {
		if(IsGenAcc) {
			THROW_MEM(p_acct_list = new SArray(sizeof(Acct)));
			for(i = 0; ExtGenAccList.enumItems(&i, (void **)&p_item);) {
				aco2 = abs(GetAcoByGenFlags(p_item->Flags));
				MEMSZERO(acct);
				if(aco2 == ACO_2) {
					if(Filt.SingleArID) {
						if(GetAcctRel(p_item->ObjID, Filt.SingleArID, &acrel_rec, 1) > 0)
							acct = acrel_rec;
					}
					else if(AccObj.Fetch(p_item->ObjID, &acc_rec) > 0)
						acct = acc_rec;
				}
				else if(aco2 == ACO_1) {
					if(AccObj.Fetch(p_item->ObjID, &acc_rec) > 0)
						acct.ac = acc_rec.A.Ac;
				}
				else if(P_ATC->AccRel.Fetch(p_item->ObjID, &acrel_rec) > 0)
					acct = acrel_rec;
				THROW_SL(p_acct_list->insert(&acct));
			}
		}
		else if(Filt.Aco == ACO_3) {
			THROW(P_ATC->AccRel.Fetch(Filt.AccID, &acrel_rec) > 0);
			acct = acrel_rec;
		}
		else {
			THROW(AccObj.Fetch(Filt.AccID, &acc_rec) > 0);
			acct.ac = acc_rec.A.Ac;
			acct.sb = (Filt.Aco == ACO_2) ? acc_rec.A.Sb : 0;
			acct.ar = 0;
		}
	}
	aco = abs(aco);
	MEMSZERO(k);
	if(aco == ACO_3) {
		idx = 1;
		k.k1.Acc   = accID;
		k.k1.Dt    = Filt.Period.low;
		k.k1.OprNo = 0;
		dbq = &(P_ATC->Acc == accID);
	}
	else {
		idx = 3;
		k.k3.Bal   = accID;
		k.k3.Dt    = Filt.Period.low;
		k.k3.OprNo = 0;
		dbq = &(P_ATC->Bal == accID);
	}
	dbq = & (*dbq && daterange(P_ATC->Dt, &Filt.Period));
	THROW_MEM(q = new BExtQuery(P_ATC, idx, 64));
	q->selectAll().where(*dbq);
	for(q->initIteration(0, &k, spGe); q->nextIteration() > 0;) {
		int    ibf = 0; // Признак того, что найден документ, соответствующий текущей записи
		BillEntry bill_entry;
		AccTurnTbl::Rec rec;
		P_ATC->copyBufTo(&rec);
		THROW(PPCheckUserBreak());
		if(Filt.Flags & AccAnlzFilt::fExclInnerTrnovr) {
			if(IsGenAcc) {
				if(p_acct_list) {
					int    to_continue = 0;
					for(i = 0; ExtGenAccList.enumItems(&i, (void **)&p_item);) {
						aco2 = abs(GetAcoByGenFlags(p_item->Flags));
						if(P_ATC->AccBelongToOrd(rec.CorrAcc, aco2, static_cast<const Acct *>(p_acct_list->at(i-1)), Filt.CurID, 1) > 0) {
							to_continue = 1;
							break;
						}
					}
					if(to_continue)
						continue;
				}
			}
			else if(P_ATC->AccBelongToOrd(rec.CorrAcc, Filt.Aco, &acct, Filt.CurID, 1) > 0)
				continue;
		}
		if(!(Filt.Flags & AccAnlzFilt::fGroupByCorAcc) && Filt.CorAco) {
			switch(Filt.CorAco) {
				case AccAnlzFilt::aafgByOp:
					if(Filt.CorAcc.ar && (FetchBill(rec.Bill, &bill_entry) <= 0 || Filt.CorAcc.ar != bill_entry.OpID))
						continue;
					break;
				case AccAnlzFilt::aafgByLoc:
					if(Filt.CorAcc.ar && (FetchBill(rec.Bill, &bill_entry) <= 0 || Filt.CorAcc.ar != bill_entry.LocID))
						continue;
					break;
				case AccAnlzFilt::aafgByExtObj:
					if(Filt.CorAcc.ar && (FetchBill(rec.Bill, &bill_entry) <= 0 || Filt.CorAcc.ar != bill_entry.Object2ID))
						continue;
					break;
				case AccAnlzFilt::aafgByAgent:
					if(Filt.CorAcc.ar && (FetchBill(rec.Bill, &bill_entry) <= 0 || Filt.CorAcc.ar != bill_entry.AgentID))
						continue;
					break;
				default:
					if(P_ATC->AccBelongToOrd(rec.CorrAcc, Filt.CorAco, &Filt.CorAcc, Filt.CurID, 1) <= 0)
						continue;
					break;
			}
		}
		if(Filt.Flags & AccAnlzFilt::fLabelOnly || Filt.LocID || Filt.AgentID || Filt.Object2ID || EffDlvrLocID)
			if(FetchBill(rec.Bill, &bill_entry) > 0) {
				if(Filt.AgentID && bill_entry.AgentID != Filt.AgentID)
					continue;
				if(Filt.Object2ID && bill_entry.Object2ID != Filt.Object2ID)
					continue;
				if(Filt.Flags & AccAnlzFilt::fLabelOnly && !(bill_entry.Flags & BILLF_WHITELABEL))
					continue;
				if(Filt.LocID && bill_entry.LocID != Filt.LocID && loc_obj.IsMemberOfGroup(bill_entry.LocID, Filt.LocID) <= 0)
					continue;
				// @v10.5.0 {
				if(EffDlvrLocID) {
					PPFreight freight;
					if(!(P_BObj->FetchFreight(rec.Bill, &freight) > 0 && freight.DlvrAddrID == EffDlvrLocID)) {
						if(!(bill_entry.LinkBillID && P_BObj->FetchFreight(bill_entry.LinkBillID, &freight) > 0 && freight.DlvrAddrID == EffDlvrLocID)) // @v10.5.2
							continue;
					}
				}
				// } @v10.5.0
			}
			else
				continue;
		//
		if(mult < 0)
			LDBLTOMONEY(-MONEYTOLDBL(rec.Amount), rec.Amount);
		THROW(r = proc(&rec, extraPtr));
		if(r < 0) {
			ok = -1;
			break;
		}
	}
	CATCHZOK
	delete q;
	delete p_acct_list;
	PROFILE_END
	return ok;
}

int FASTCALL GetAcoByGenFlags(long f)
{
	const long t = CHKXORFLAGS(f, ACGF_ACO1GRP, ACGF_ACO2GRP);
	if(t == ACGF_ACO1GRP)
		return (f & ACGF_NEGATIVE) ? -ACO_1 : ACO_1;
	else if(t == ACGF_ACO2GRP)
		return (f & ACGF_NEGATIVE) ? -ACO_2 : ACO_2;
	else
		return (f & ACGF_NEGATIVE) ? -ACO_3 : ACO_3;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempAAFile, TempAccAnlz);
PP_CREATE_TEMP_FILE_PROC(CreateTempATFile, TempAccTrnovr);

struct IterProcParam_CrtTmpTbl {
	PPObjBill    * P_BObj;
	AccTurnCore  * P_ATC;
	BExtInsert   * Bei;
	AccAnlzTotal * Total;
	AccAnlzFilt  * Filt;
	PPCycleArray * CycleList;
	TempAccTrnovrTbl * P_TmpATTbl;
	int    IsRegister;
	AmtList InRest;
	AmtList * P_CycleOutRests;
};

int IterProc_CrtTmpAATbl(AccTurnTbl::Rec * pRec, void * extraPtr)
{
	int    ok = 1;
	PPID   cur_id = 0;
	IterProcParam_CrtTmpTbl & p = *static_cast<IterProcParam_CrtTmpTbl *>(extraPtr);
	TempAccAnlzTbl::Rec trec;
	AcctRelTbl::Rec arel_rec;
	// @v10.6.4 MEMSZERO(trec);
	trec.Dt      = pRec->Dt;
	trec.OprNo   = pRec->OprNo;
	trec.BillID  = pRec->Bill;
	trec.Reverse = pRec->Reverse;
	trec.ThisAccRelID = pRec->Acc;
	trec.Acc     = pRec->CorrAcc;
	if(p.P_ATC->AccRel.Fetch(pRec->CorrAcc, &arel_rec) > 0) {
		if(ObjRts.CheckAccID(arel_rec.AccID, PPR_READ)) {
			trec.Ac  = arel_rec.Ac;
			trec.Sb  = arel_rec.Sb;
			trec.Ar  = arel_rec.Ar;
		}
		else {
			trec.Ac  = 0;
			trec.Sb  = 0;
			trec.Ar  = 0;
		}
		trec.CurID = cur_id = arel_rec.CurID;
	}
	else
		trec.Ar  = pRec->CorrAcc;
	double tmp = MONEYTOLDBL(pRec->Amount);
	if(p.IsRegister) {
		if(tmp > 0.0)
			p.Total->AddTrnovr(1, cur_id, tmp);
		else
			p.Total->AddTrnovr(0, cur_id, -tmp);
		p.Total->Count++;
	}
	else if(pRec->Reverse) {
		p.Total->CrdTrnovr.Add(1L, cur_id, 1L);
		p.Total->AddTrnovr(0, cur_id, tmp);
		trec.Crd = tmp;
		tmp = -tmp;
	}
	else {
		p.Total->DbtTrnovr.Add(1L, cur_id, 1L);
		p.Total->AddTrnovr(1, cur_id, tmp);
		trec.Dbt = tmp;
	}
	p.Total->Count++;
	p.InRest.Add(0, cur_id, tmp);
	trec.Rest = p.InRest.Get(0, cur_id);
	if(p.Filt->InitOrder == PPViewAccAnlz::OrdByBillCode_Date) {
		BillTbl::Rec bill_rec;
		if(p.P_BObj->Fetch(trec.BillID, &bill_rec) > 0)
			STRNSCPY(trec.OrdData, p.P_BObj->P_Tbl->GetCode(bill_rec.Code));
	}
	else if(p.Filt->InitOrder == PPViewAccAnlz::OrdByCorrAcc_Date) {
		SString temp_buf;
		temp_buf.CatLongZ(trec.Ac, 4).CatLongZ(trec.Sb, 4).CatLongZ(trec.Ar, 6).CatLongZ(trec.CurID, 6).
			CopyTo(trec.OrdData, sizeof(trec.OrdData));
	}
	if(p.Bei)
		THROW_DB(p.Bei->insert(&trec));
	CATCHZOK
	return ok;
}

int IterProc_CrtTmpATTbl(AccTurnTbl::Rec * pRec, void * extraPtr)
{
	int    ok = 1;
	const PPRights & r_orts = ObjRts;
	uint   cycle_pos = 0;
	SString temp_buf;
	IterProcParam_CrtTmpTbl & p = *static_cast<IterProcParam_CrtTmpTbl *>(extraPtr);
	AcctRelTbl::Rec arel_rec;
	BillTbl::Rec bill_rec;
	TempAccTrnovrTbl::Rec  trec;
	TempAccTrnovrTbl::Key0 k;
	// @v10.6.4 MEMSZERO(trec);
	if(!p.Filt->Cycl)
		trec.Dt = p.Filt->Period.low;
	else if(p.CycleList->searchDate(pRec->Dt, &(cycle_pos = 0)))
		trec.Dt = p.CycleList->at(cycle_pos).low;
	else
		return 1;
	if(p.Filt->CorAco == AccAnlzFilt::aafgByOp) {
		if(p.P_BObj->Fetch(pRec->Bill, &bill_rec) > 0 && r_orts.CheckOpID(bill_rec.OpID, PPR_READ)) {
			trec.Ar       = bill_rec.OpID;
			trec.AccRelID = bill_rec.OpID;
			GetOpName(bill_rec.OpID, temp_buf);
			temp_buf.CopyTo(trec.Name, sizeof(trec.Name));
		}
	}
	else if(p.Filt->CorAco == AccAnlzFilt::aafgByLoc) {
		if(p.P_BObj->Fetch(pRec->Bill, &bill_rec) > 0 && r_orts.CheckLocID(bill_rec.LocID, 0)) {
			trec.Ar       = bill_rec.LocID;
			trec.AccRelID = bill_rec.LocID;
			GetLocationName(bill_rec.LocID, temp_buf);
			temp_buf.CopyTo(trec.Name, sizeof(trec.Name));
		}
	}
	else if(p.Filt->CorAco == AccAnlzFilt::aafgByExtObj) {
		if(p.P_BObj->Fetch(pRec->Bill, &bill_rec) > 0) {
			trec.Ar       = bill_rec.Object2;
			trec.AccRelID = bill_rec.Object2;
			GetArticleName(bill_rec.Object2, temp_buf);
			temp_buf.CopyTo(trec.Name, sizeof(trec.Name));
		}
	}
	else if(p.Filt->CorAco == AccAnlzFilt::aafgByAgent) {
		PPBillExt bext_rec;
		PPID   agent_id = 0;
		if(p.P_BObj->FetchExt(pRec->Bill, &bext_rec) > 0)
			agent_id = bext_rec.AgentID;
		trec.Ar       = agent_id;
		trec.AccRelID = agent_id;
		GetArticleName(agent_id, temp_buf);
		temp_buf.CopyTo(trec.Name, sizeof(trec.Name));
	}
	else {
		trec.AccRelID = pRec->CorrAcc;
		if(p.P_ATC->AccRel.Fetch(pRec->CorrAcc, &arel_rec) > 0) {
			trec.CurID = arel_rec.CurID;
			if(r_orts.CheckAccID(arel_rec.AccID, PPR_READ)) {
				if(p.Filt->CorAco) {
					trec.Ac = arel_rec.Ac;
					trec.Sb = arel_rec.Sb;
					trec.Ar = arel_rec.Ar;
					if(p.Filt->CorAco == ACO_1) {
						PPAccount acc_rec;
						THROW(p.P_ATC->AccObj.SearchNum(trec.Ac, 0, 0L, &acc_rec) > 0);
						trec.AccRelID = acc_rec.ID;
						trec.Sb = 0;
						trec.Ar = 0;
					}
					else if(p.Filt->CorAco == ACO_2) {
						trec.AccRelID = arel_rec.AccID;
						trec.Ar = 0;
					}
					GetAcctName((Acct*)&trec.Ac, trec.CurID, 0, temp_buf);
					temp_buf.CopyTo(trec.Name, sizeof(trec.Name));
				}
			}
		}
		else {
			trec.Ac = 999;
			trec.Ar = pRec->CorrAcc % 1000L;
			if(PPLoadText(PPTXT_UNKNOWNACC, temp_buf)) {
				//sprintf(trec.Name, temp_buf, pRec->CorrAcc);
				temp_buf.Space();
				ideqvalstr(pRec->CorrAcc, temp_buf);
			}
			else
				ltoa(pRec->CorrAcc, trec.Name, 10);
		}
	}
	const double amount = MONEYTOLDBL(pRec->Amount);
	if((p.IsRegister && amount < 0.0) || pRec->Reverse) {
		const double temp_amt = p.IsRegister ? fabs(amount) : amount;
		p.Total->CrdTrnovr.Add(1L, trec.CurID, 1L);
		p.Total->AddTrnovr(0, trec.CurID, temp_amt);
		trec.Dbt = 0.0;
		trec.Crd = temp_amt;
	}
	else {
		p.Total->DbtTrnovr.Add(1L, trec.CurID, 1L);
		p.Total->AddTrnovr(1, trec.CurID, amount);
		trec.Dbt = amount;
		trec.Crd = 0.0;
	}
	p.Total->Count++;
	if(p.P_TmpATTbl) {
		trec.Count = 1;
		k.Dt = trec.Dt;
		k.Ac = trec.Ac;
		k.Sb = trec.Sb;
		k.Ar = trec.Ar;
		k.CurID = trec.CurID;
		if(p.P_TmpATTbl->searchForUpdate(0, &k, spEq)) {
			p.P_TmpATTbl->data.Count++;
			p.P_TmpATTbl->data.Dbt += trec.Dbt;
			p.P_TmpATTbl->data.Crd += trec.Crd;
			THROW_DB(p.P_TmpATTbl->updateRec()); // @sfu
		}
		else
			THROW_DB(p.P_TmpATTbl->insertRecBuf(&trec));
	}
	if(p.P_CycleOutRests && p.Filt->Cycl.Cycle) {
		for(uint j = cycle_pos; j < p.CycleList->getCount(); j++)
			p.P_CycleOutRests->Add(j, trec.CurID, trec.Dbt-trec.Crd);
	}
	CATCHZOK
	return ok;
}

// virtual
int SLAPI PPViewAccAnlz::Init_(const PPBaseFilt * pFilt)
{
	ExpiryDate = ZERODATE;
	IterFlags = 0;
	IsGenAcc = 0;
	IsGenAr = 0; // @v9.5.9
	IsRegister = 0;
	EffDlvrLocID = 0; // @v10.5.0
	ExtGenAccList.freeAll();

	int    ok = 1;
	PPAccount acc_rec;
	ArticleTbl::Rec ar_rec;
	AcctRelTbl::Rec acr_rec;
	PPIDArray acc_list;
	PPIDArray cur_list;
	PPIDArray gen_ar_list;
	AccAnlzTotal::Cut cut;
	IterProcParam_CrtTmpTbl param;
	AccAnlzViewEnumProc enum_proc = 0;
	param.Bei = 0;
	param.P_CycleOutRests = 0;
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	ZDELETE(P_TmpAATbl);
	ZDELETE(P_TmpATTbl);
	Total.Init();
	CycleList.init2(&Filt.Period, &Filt.Cycl);
	// @v10.5.0 {
	if(Filt.DlvrLocID && Filt.Aco == ACO_3 && Filt.AcctId.ar) {
		EffDlvrLocID = Filt.DlvrLocID;
	}
	// } @v10.5.0
	if(!(Filt.Flags & AccAnlzFilt::fTotalOnly && (Filt.Flags & AccAnlzFilt::fLabelOnly || Filt.LocID || EffDlvrLocID))) {
		THROW(AdjustPeriodToRights(Filt.Period, 0));
	}
	if(Filt.AccID && (!Filt.AcctId.ac || !Filt.AcctId.ar)) {
		if(Filt.Aco == ACO_3)
			P_ATC->AcctRelToID(Filt.AccID, &Filt.AcctId, &Filt.AccSheetID);
		else {
			Filt.AcctId.ac = Filt.AccID;
			Filt.AcctId.ar = Filt.SingleArID;
		}
	}
	if(AccObj.Fetch(Filt.AcctId.ac, &acc_rec) > 0) {
		THROW(ObjRts.CheckAccID(acc_rec.ID, PPR_READ));
		if(acc_rec.Type == ACY_AGGR) {
			IsGenAcc = 1;
			P_ATC->GetExtentAccListByGen(Filt.AcctId.ac, &ExtGenAccList, 0);
		}
		else if(acc_rec.Type == ACY_REGISTER)
			IsRegister = 1;
	}
	// @v9.5.9 {
	if(ArObj.Fetch(Filt.AcctId.ar, &ar_rec) > 0 && ar_rec.Flags & ARTRF_GROUP) {
		IsGenAr = 1;
		ArObj.P_Tbl->GetListByGroup(Filt.AcctId.ar, &gen_ar_list);
	}
	// } @v9.5.9
	Filt.CurID = (Filt.Flags & AccAnlzFilt::fAllCurrencies) ? -1 : Filt.CurID;
	if(Filt.Flags & AccAnlzFilt::fLabelOnly || Filt.LocID || EffDlvrLocID) {
		Total.InRest.freeAll();
		Total.OutRest.freeAll();
		if(Filt.Period.low) {
			PPViewAccAnlz temp_view;
			AccAnlzFilt temp_filt = Filt;
			temp_filt.Cycl.Init();
			temp_filt.Period.Set(ZERODATE, plusdate(Filt.Period.low, -1));
			temp_filt.Flags |= AccAnlzFilt::fTotalOnly;
			THROW(temp_view.Init_(&temp_filt));
			Total.InRest.Add(&temp_view.Total.DbtTrnovr);
			Total.InRest.Sub(&temp_view.Total.CrdTrnovr);
		}
	}
	else if(IsGenAcc) {
		ObjRestrictItem * p_item;
		for(uint i = 0; ExtGenAccList.enumItems(&i, (void **)&p_item);) {
			const int aco = GetAcoByGenFlags(p_item->Flags);
			if(Filt.SingleArID && abs(aco) == ACO_2) {
				if(GetAcctRel(p_item->ObjID, Filt.SingleArID, &acr_rec, 1) > 0)
					THROW(P_ATC->CalcComplexRest((aco > 0) ? ACO_3 : -ACO_3, acr_rec.ID, Filt.CurID, Filt.SubstRelTypeID, &Filt.Period, &Total.InRest, &Total.OutRest));
			}
			else {
				THROW(P_ATC->CalcComplexRest(aco, p_item->ObjID, Filt.CurID, Filt.SubstRelTypeID, &Filt.Period, &Total.InRest, &Total.OutRest));
			}
		}
	}
	else if(IsGenAr) {
		for(uint i = 0; i < gen_ar_list.getCount(); i++) {
			AcctID temp_acct_id;
			temp_acct_id.ac = Filt.AcctId.ac;
			temp_acct_id.ar = gen_ar_list.get(i);
			PPID   temp_acrel = 0;
			if(P_ATC->AcctIDToRel(&temp_acct_id, &temp_acrel) > 0) {
				THROW(P_ATC->CalcComplexRest(ACO_3, temp_acrel, Filt.CurID, Filt.SubstRelTypeID, &Filt.Period, &Total.InRest, &Total.OutRest));
			}
		}
	}
	else {
		THROW(P_ATC->CalcComplexRest(Filt.Aco, Filt.AccID, Filt.CurID, Filt.SubstRelTypeID, &Filt.Period, &Total.InRest, &Total.OutRest));
	}
	if(Filt.Flags & AccAnlzFilt::fTrnovrBySheet) {
		//
		// Обороты по статьям
		//
		int    is_person_rel = 0;
		ArticleTbl::Rec ar_rec;
		PPAccSheet acs_rec;
		TSVector <AcctRelTbl::Rec> acr_list; // @v9.8.4 TSArray-->TSVector
		THROW(P_TmpATTbl = CreateTempATFile());
		THROW(AccObj.Fetch(Filt.AccID, &acc_rec) > 0);
		THROW_PP(acc_rec.AccSheetID, PPERR_ACCHASNTSHEET);
		THROW(SearchObject(PPOBJ_ACCSHEET, acc_rec.AccSheetID, &acs_rec) > 0);
		if(acs_rec.Assoc == PPOBJ_PERSON)
			is_person_rel = 1;
		{
			PPViewAccAnlz temp_view;
			BExtInsert bei(P_TmpATTbl);
			if(IsGenAcc) {
				if(Filt.SingleArID) {
					if(ArObj.Fetch(Filt.SingleArID, &ar_rec) > 0) {
						MEMSZERO(acr_rec);
						acr_rec.ID = Filt.SingleArID;
						acr_rec.Ac = acc_rec.A.Ac;
						acr_rec.Sb = acc_rec.A.Sb;
						acr_rec.Ar = ar_rec.Article;
						acr_rec.AccID = Filt.AccID;
						acr_rec.ArticleID = Filt.SingleArID;
						THROW_SL(acr_list.insert(&acr_rec));
					}
				}
				else {
					for(long ar = 0; P_ATC->Art.EnumBySheet(acc_rec.AccSheetID, &ar, &ar_rec) > 0;) {
						MEMSZERO(acr_rec);
						acr_rec.ID = ar_rec.ID;
						acr_rec.Ac = acc_rec.A.Ac;
						acr_rec.Sb = acc_rec.A.Sb;
						acr_rec.Ar = ar_rec.Article;
						acr_rec.AccID = Filt.AccID;
						acr_rec.ArticleID = ar_rec.ID;
						THROW_SL(acr_list.insert(&acr_rec));
					}
				}
			}
			// @v9.5.9 {
			else if(IsGenAr) {
				for(uint i = 0; i < gen_ar_list.getCount(); i++) {
					const PPID ar_id = gen_ar_list.get(i);
					if(ArObj.Fetch(ar_id, &ar_rec) > 0 && P_ATC->GetAcctRel(acc_rec.ID, ar_id, &acr_rec, 0, 0) > 0) {
						THROW_SL(acr_list.insert(&acr_rec));
					}
				}
			}
			// } @v9.5.9
			else if(Filt.SingleArID) {
				if(GetAcctRel(Filt.AccID, Filt.SingleArID, &acr_rec, 1) > 0)
					THROW_SL(acr_list.insert(&acr_rec));
			}
			else {
				AcctRelTbl::Key1 k;
				BExtQuery q(&P_ATC->AccRel, 1);
				q.selectAll().where(P_ATC->AccRel.AccID == Filt.AccID && P_ATC->AccRel.Closed == 0L);
				k.AccID = Filt.AccID;
				k.ArticleID = 0;
				for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
					P_ATC->AccRel.copyBufTo(&acr_rec);
					THROW_SL(acr_list.insert(&acr_rec));
				}
			}
			for(uint i2 = 0; i2 < acr_list.getCount(); i2++) {
				const AcctRelTbl::Rec & r_acr_rec = acr_list.at(i2);
				PPID   rel_person_id = 0;
				AccAnlzTotal  total;
				TempAccTrnovrTbl::Rec rec;
				AccAnlzFilt   temp_flt = Filt;
				temp_flt.Flags |= AccAnlzFilt::fTotalOnly;
				temp_flt.Flags &= ~AccAnlzFilt::fTrnovrBySheet;
				temp_flt.Aco    = ACO_3;
				temp_flt.Cycl.Init();
				if(IsGenAcc) {
					temp_flt.AccID = Filt.AccID;
					temp_flt.SingleArID = r_acr_rec.ArticleID;
					temp_flt.AcctId.ac = r_acr_rec.AccID;
					temp_flt.AcctId.ar = r_acr_rec.ArticleID;
				}
				else if(IsGenAr) {
					temp_flt.AccID = r_acr_rec.ID;
					temp_flt.AcctId.ar = r_acr_rec.ArticleID;
				}
				else
					temp_flt.AccID = r_acr_rec.ID;
				THROW(temp_view.Init_(&temp_flt));
				temp_view.GetTotal(&total);
				Total.DbtTrnovr.Add(&total.DbtTrnovr);
				Total.CrdTrnovr.Add(&total.CrdTrnovr);
				// @v10.7.3 @ctr MEMSZERO(rec);
				if(r_acr_rec.ArticleID && ArObj.Fetch(r_acr_rec.ArticleID, &ar_rec) > 0) {
					STRNSCPY(rec.Name, ar_rec.Name);
					if(is_person_rel)
						rel_person_id = ar_rec.ObjID;
				}
				else {
					rec.Name[0] = static_cast<char>(250);
					ideqvalstr(r_acr_rec.ArticleID, rec.Name+1, sizeof(rec.Name)-1);
				}
				if(Filt.Flags & AccAnlzFilt::fTrnovrBySuppl) {
					GoodsRestParam gp;
					gp.CalcMethod  = GoodsRestParam::pcmSum;
					gp.Date        = Filt.Period.upp;
					gp.SupplID     = ar_rec.ID;
					gp.LocID       = Filt.LocID;
					THROW(P_BObj->trfr->GetRest(gp));
					rec.GoodsRest  = gp.Total.Cost;
				}
				cur_list.clear();
				total.GetCurList(&cur_list);
				for(uint i = 0; i < cur_list.getCount(); i++) {
					if(total.GetCut(cur_list.get(i), &cut) > 0) {
						rec.AccRelID = r_acr_rec.ID;
						rec.Ac       = r_acr_rec.Ac;
						rec.Sb       = r_acr_rec.Sb;
						rec.Ar       = r_acr_rec.Ar;
						rec.CurID    = cur_list.get(i);
						rec.RelPersonID = rel_person_id;
						rec.Count    = cut.CDbtCount + cut.CCrdCount;
						rec.InRest   = cut.CInRest;
						rec.OutRest  = cut.COutRest;
						rec.Dbt      = cut.CDbtTrnovr;
						rec.Crd      = cut.CCrdTrnovr;
						if(!Filt.SingleArID)
							if(Filt.Flags & AccAnlzFilt::fSpprZTrnovr && rec.Count == 0)
								continue;
							else if(Filt.Flags & AccAnlzFilt::fSpprZSaldo && rec.OutRest == 0.0)
								continue;
						THROW_DB(bei.insert(&rec));
						Total.Count++; // @v10.1.9
					}
				}
				PPWaitPercent(i2+1, acr_list.getCount());
			}
			THROW_DB(bei.flash());
		}
		// @v10.1.9 Total.Count = acr_list.getCount();
		//
		// Без вызова этой функции печать оборотной ведомости выдает
		// непонятно откуда взявшуюся ошибку "Счет не найден"
		// Только в release-версии.
		//
		CalcTotalAccTrnovr(&Total);
	}
	else if(Filt.Cycl.Cycle && !(Filt.Flags & AccAnlzFilt::fGroupByCorAcc)) {
		AccAnlzTotal  total;
		PPViewAccAnlz temp_view;
		AccAnlzFilt   temp_flt = Filt;
		temp_flt.Flags |= AccAnlzFilt::fTotalOnly;
		temp_flt.Cycl.Init();
		THROW(P_TmpATTbl = CreateTempATFile());
		{
			BExtInsert bei(P_TmpATTbl);
			for(uint cycle_no = 0; cycle_no < CycleList.getCount(); cycle_no++) {
				TempAccTrnovrTbl::Rec rec;
				CycleList.getPeriod(cycle_no, &temp_flt.Period);
				THROW(temp_view.Init_(&temp_flt));
				temp_view.GetTotal(&total);
				Total.DbtTrnovr.Add(&total.DbtTrnovr);
				Total.CrdTrnovr.Add(&total.CrdTrnovr);
				Total.Count += total.Count;
				// @v10.6.4 MEMSZERO(rec);
				cur_list.clear();
				total.GetCurList(&cur_list);
				for(uint i = 0; i < cur_list.getCount(); i++) {
					if(total.GetCut(cur_list.get(i), &cut) > 0 && (cut.CDbtTrnovr != 0.0 || cut.CCrdTrnovr != 0.0)) {
						rec.Dt      = CycleList.at(cycle_no).low;
						rec.CurID   = cur_list.get(i);
						rec.Count   = total.Count;
						rec.InRest  = cut.CInRest;
						rec.OutRest = cut.COutRest;
						rec.Dbt     = cut.CDbtTrnovr;
						rec.Crd     = cut.CCrdTrnovr;
						THROW_DB(bei.insert(&rec));
					}
				}
				PPWaitPercent(cycle_no, CycleList.getCount());
			}
			THROW_DB(bei.flash());
		}
	}
	else {
		Filt.Flags &= ~AccAnlzFilt::fTrnovrBySuppl;
		if(!IsGenAcc && !IsGenAr) // @v9.5.9 (&& !IsGenAr)
			THROW(P_ATC->IdentifyAcc(&Filt.Aco, &Filt.AccID, Filt.CurID, Filt.SubstRelTypeID, &acc_list));
		param.IsRegister = IsRegister;
		param.P_BObj = P_BObj;
		param.P_TmpATTbl = 0;
		param.P_ATC = P_ATC;
		param.Filt  = &Filt;
		param.CycleList = &CycleList;
		param.Total = &Total;
		param.InRest.copy(Total.InRest);
		param.P_CycleOutRests = 0;
		if(Filt.Flags & AccAnlzFilt::fGroupByCorAcc) {
			if(!(Filt.Flags & AccAnlzFilt::fTotalOnly)) {
				THROW(P_TmpATTbl = CreateTempATFile());
				if(Filt.Cycl.Cycle)
					THROW_MEM(param.P_CycleOutRests = new AmtList);
			}
			param.P_TmpATTbl = P_TmpATTbl;
			enum_proc = IterProc_CrtTmpATTbl;
		}
		else {
			if(!(Filt.Flags & AccAnlzFilt::fTotalOnly)) {
				THROW(P_TmpAATbl = CreateTempAAFile());
				THROW_MEM(param.Bei = new BExtInsert(P_TmpAATbl));
			}
			enum_proc = IterProc_CrtTmpAATbl;
		}
		{
			LAssocArray aco_list;
			if(IsGenAcc) {
				for(uint i = 0; ok > 0 && i < ExtGenAccList.getCount(); i++) {
					const PPID acc_id = ExtGenAccList.at(i).ObjID;
					const int  aco    = GetAcoByGenFlags(ExtGenAccList.at(i).Flags);
					if(Filt.SingleArID && abs(aco) == ACO_2) {
						if(GetAcctRel(acc_id, Filt.SingleArID, &acr_rec, 1) > 0) {
							aco_list.Add(acr_rec.ID, (aco > 0) ? ACO_3 : -ACO_3, 0);
						}
					}
					else {
						aco_list.Add(acc_id, aco, 0);
					}
				}
			}
			// @v9.5.9 {
			else if(IsGenAr) {
				for(uint i = 0; i < gen_ar_list.getCount(); i++) {
					AcctID temp_acct_id;
					temp_acct_id.ac = Filt.AcctId.ac;
					temp_acct_id.ar = gen_ar_list.get(i);
					PPID   temp_acrel = 0;
					if(P_ATC->AcctIDToRel(&temp_acct_id, &temp_acrel) > 0) {
						aco_list.Add(temp_acrel, ACO_3, 0);
					}
				}
			}
			// } @v9.5.9
			else {
				aco_list.Add(Filt.AccID, Filt.Aco, 0);
			}
			for(uint i = 0; ok > 0 && i < aco_list.getCount(); i++) {
				long   aco = aco_list.at(i).Val;
				PPID   acc_id = aco_list.at(i).Key;
				int    sign = (aco < 0) ? -1 : 1;
				PPIDArray __acc_list;
				aco = abs(aco);
				THROW(P_ATC->IdentifyAcc(&aco, &acc_id, Filt.CurID, Filt.SubstRelTypeID, &__acc_list));
				if(!__acc_list.isList()) {
					THROW(ok = EnumerateByIdentifiedAcc(aco * sign, acc_id, enum_proc, &param));
				}
				else {
					for(uint j = 0; ok > 0 && j < __acc_list.getCount(); j++)
						THROW(ok = EnumerateByIdentifiedAcc(aco * sign, __acc_list.get(j), enum_proc, &param));
				}
			}
		}
		if(param.Bei) {
			THROW(param.Bei->flash());
			ZDELETE(param.Bei);
		}
		if(Filt.Flags & AccAnlzFilt::fGroupByCorAcc && Filt.CorAco == AccAnlzFilt::aafgByLoc) {
			//
			// Установка исходящих остатков в группировке по складам
			//
			if(P_TmpATTbl) {
				TempAccTrnovrTbl::Key0 k;
				if(Filt.Period.low) {
					PPIDArray loc_list;
					AccAnlzFilt temp_filt = Filt;
					temp_filt.Cycl.Init();
					temp_filt.Period.Set(ZERODATE, plusdate(Filt.Period.low, -1));
					temp_filt.Flags &= ~AccAnlzFilt::fGroupByCorAcc;
					temp_filt.Flags |= AccAnlzFilt::fTotalOnly;
					for(MEMSZERO(k); P_TmpATTbl->search(0, &k, spGt);)
 						loc_list.addUnique(P_TmpATTbl->data.Ar);
					for(uint i = 0; i < loc_list.getCount(); i++) {
						temp_filt.LocID = loc_list.at(i);
						PPViewAccAnlz temp_view;
						THROW(temp_view.Init_(&temp_filt));
						for(MEMSZERO(k); P_TmpATTbl->search(0, &k, spGt);)
							if(P_TmpATTbl->data.Ar == temp_filt.LocID) {
								P_TmpATTbl->data.OutRest = temp_view.Total.OutRest.Get(0, P_TmpATTbl->data.CurID) +
									P_TmpATTbl->data.Dbt - P_TmpATTbl->data.Crd;
								THROW_DB(P_TmpATTbl->updateRec());
							}
					}
				}
				else {
					for(MEMSZERO(k); P_TmpATTbl->search(0, &k, spGt);) {
						P_TmpATTbl->data.OutRest = P_TmpATTbl->data.Dbt - P_TmpATTbl->data.Crd;
						THROW_DB(P_TmpATTbl->updateRec());
					}
				}
			}
		}
		else if(param.P_CycleOutRests) {
			uint   i;
			TempAccTrnovrTbl::Key0 k;
			cur_list.clear();
			param.InRest.GetCurList(0L, &cur_list);
			for(i = 0; i < CycleList.getCount(); i++)
				for(uint j = 0; j < cur_list.getCount(); j++) {
					const PPID c = cur_list.get(j);
					param.P_CycleOutRests->Add(i, c, param.InRest.Get(0L, c));
				}
			for(MEMSZERO(k); P_TmpATTbl->search(0, &k, spGt);)
				if(CycleList.searchDate(k.Dt, &(i = 0))) {
					P_TmpATTbl->data.OutRest = param.P_CycleOutRests->Get(i, k.CurID);
					THROW_DB(P_TmpATTbl->updateRec());
				}
		}
		else if(IsGenAcc || acc_list.getCount() > 1) {
			if(P_TmpAATbl) {
				TempAccAnlzTbl::Key1 k1;
				MEMSZERO(k1);
				AmtList rest = Total.InRest;
				while(P_TmpAATbl->searchForUpdate(1, &k1, spGt)) {
					rest.Add(0, P_TmpAATbl->data.CurID, P_TmpAATbl->data.Dbt - P_TmpAATbl->data.Crd);
					P_TmpAATbl->data.Rest = rest.Get(0, P_TmpAATbl->data.CurID);
					THROW_DB(P_TmpAATbl->updateRec()); // @sfu
				}
			}
		}
	}
	if(Filt.Flags & AccAnlzFilt::fLabelOnly || Filt.LocID || EffDlvrLocID) {
		Total.OutRest.Add(&Total.InRest);
		Total.OutRest.Add(&Total.DbtTrnovr);
		Total.OutRest.Sub(&Total.CrdTrnovr);
	}
	CATCH
		ok = 0;
		ZDELETE(param.Bei);
		ZDELETE(P_TmpATTbl);
		ZDELETE(P_TmpAATbl);
	ENDCATCH
	ZDELETE(param.P_CycleOutRests);
	return ok;
}

int SLAPI PPViewAccAnlz::InitIteration()
{
	int    ok = 1;
	BExtQuery::ZDelete(&P_IterQuery);
	if(Filt.Flags & AccAnlzFilt::fTotalOnly)
		ok = -1;
	else {
		if(IsRegister && !(Filt.Flags & AccAnlzFilt::fTrnovrBySheet)) {
			int    idx = 0;
			DBQ  * dbq = 0;
			void * p_key = 0;
			AccTurnTbl::Key1 k1;
			AccTurnTbl::Key3 k3;
			AccTurnTbl * t = P_ATC;
			if(Filt.Aco == ACO_3) {
				dbq = & (t->Acc == Filt.AccID && daterange(t->Dt, &Filt.Period));
				idx = 1;
				k1.Acc = Filt.AccID;
				k1.Dt = Filt.Period.low;
				k1.OprNo = 0;
				p_key = &k1;
				//.orderBy(rat->Acc, rat->Dt, rat->OprNo, 0L);
			}
			else { // if(Filt.Aco == ACO_2)
				dbq = & (t->Bal == Filt.AccID && daterange(t->Dt, &Filt.Period));
				idx = 3;
				k3.Bal = Filt.AccID;
				k3.Dt = Filt.Period.low;
				k3.OprNo = 0;
				p_key = &k3;
				//.orderBy(rat->Bal, rat->Dt, rat->OprNo, 0L);
			}
			THROW_MEM(P_IterQuery = new BExtQuery(t, idx));
			P_IterQuery->selectAll().where(*dbq);
			P_IterQuery->initIteration(0, p_key, spGe);
		}
		else {
			// @v10.6.8 char   k[MAXKEYLEN];
			BtrDbKey k_; // @v10.6.8 
			if(Filt.Flags & (AccAnlzFilt::fGroupByCorAcc|AccAnlzFilt::fTrnovrBySheet) || Filt.Cycl.Cycle) {
				THROW_MEM(P_IterQuery = new BExtQuery(P_TmpATTbl, 1));
			}
			else {
				THROW_MEM(P_IterQuery = new BExtQuery(P_TmpAATbl, 0));
			}
			P_IterQuery->selectAll();
			// @v10.6.8 memzero(k, sizeof(k));
			P_IterQuery->initIteration(0, k_, spFirst);
		}
	}
	CATCHZOK
	return ok;
}

int FASTCALL PPViewAccAnlz::NextIteration(AccAnlzViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery) {
		if(P_IterQuery->nextIteration() > 0) {
			AccAnlzViewItem item;
			MEMSZERO(item);
			if(IsRegister && !(Filt.Flags & AccAnlzFilt::fTrnovrBySheet)) {
				AccTurnTbl::Rec rec;
				P_ATC->copyBufTo(&rec);
				item.Dt      = rec.Dt;
				item.OprNo   = rec.OprNo;
				item.BillID  = rec.Bill;
				item.RByBill = rec.RByBill;
				item.Reverse = rec.Reverse;
				item.AccID   = Filt.AccID;
				item.Count   = 1;
				double amt   = MONEYTOLDBL(rec.Amount);
				if(amt > 0.0)
					item.DbtAmt = amt;
				else
					item.CrdAmt = -amt;
				item.OutRest = MONEYTOLDBL(rec.Rest);
				item.InRest  = R2(item.OutRest - amt);
			}
			else if((Filt.Flags & (AccAnlzFilt::fGroupByCorAcc | AccAnlzFilt::fTrnovrBySheet)) || Filt.Cycl.Cycle) {
				TempAccTrnovrTbl::Rec & atrec = P_TmpATTbl->data;
				item.Dt = atrec.Dt;
				if(Filt.Flags & AccAnlzFilt::fTrnovrBySheet) {
					item.AccRelID = IsGenAcc ? 0 : atrec.AccRelID;
					item.AccID = 0;
				}
				else if(oneof2(Filt.CorAco, ACO_1, ACO_2))
					item.AccID = atrec.AccRelID;
				else
					item.AccRelID = atrec.AccRelID;
				item.CurID   = atrec.CurID;
				item.RelPersonID = atrec.RelPersonID;
				item.Count   = atrec.Count;
				item.DbtAmt  = atrec.Dbt;
				item.CrdAmt  = atrec.Crd;
				if(IterFlags & PPViewAccAnlz::fIterNegRest) {
					item.InRest  = -atrec.InRest;
					item.OutRest = -atrec.OutRest;
				}
				else {
					item.InRest  = atrec.InRest;
					item.OutRest = atrec.OutRest;
				}
				item.SupplGoodsRestAmt = atrec.GoodsRest;
				STRNSCPY(item.AccName, atrec.Name);
			}
			else {
				SString temp_buf;
				TempAccAnlzTbl::Rec & aarec = P_TmpAATbl->data;
				item.Dt      = aarec.Dt;
				item.OprNo   = aarec.OprNo;
				STRNSCPY(item.OrderText, aarec.OrdData);
				item.BillID  = aarec.BillID;
				//item.RByBill = aarec.RByBill;
				item.Reverse = aarec.Reverse;
				item.ThisAccRelID = aarec.ThisAccRelID;
				item.AccRelID = aarec.Acc;
				item.CurID   = aarec.CurID;
				item.Count   = 1;
				item.DbtAmt  = aarec.Dbt;
				item.CrdAmt  = aarec.Crd;
				item.OutRest = (IterFlags & PPViewAccAnlz::fIterNegRest) ? -aarec.Rest : aarec.Rest;
				GetAcctName((Acct*)&aarec.Ac, item.CurID, 0, temp_buf);
				temp_buf.CopyTo(item.AccName, sizeof(item.AccName));
			}
			ASSIGN_PTR(pItem, item);
			ok = 1;
		}
	}
	return ok;
}

void SLAPI PPViewAccAnlz::FormatCycle(LDATE dt, char * pBuf, size_t bufLen)
{
	Helper_FormatCycle(Filt.Cycl, CycleList, dt, pBuf, bufLen);
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	const PPViewAccAnlz * p_view = static_cast<const PPViewAccAnlz *>(extraPtr);
	PPViewAccAnlz::BrwHdr hdr;
	if(p_view && p_view->GetBrwHdr(pData, &hdr) && pStyle && paintAction == BrowserWindow::paintNormal) {
		if(hdr.CorAccID && !ObjRts.CheckAccID(hdr.CorAccID, PPR_READ)) {
			pStyle->Color = GetGrayColorRef(0.9f);
			pStyle->Flags = 0;
			ok = 1;
		}
	}
	return ok;
}

void SLAPI PPViewAccAnlz::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, SetCellStyleFunc(CellStyleFunc, this));
}

// virtual
DBQuery * SLAPI PPViewAccAnlz::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = 0;
	SString sub_title;
	DBQuery * q  = 0;
	DBQ  * dbq = 0;
	DBE    dbe_cur;
	DBE    dbe_ar;
	DBE    dbe_bill_memo;
	DBE    dbe_bill_code;
	TempAccAnlzTbl   * att = 0;
	TempAccTrnovrTbl * ttt = 0;
	AccTurnTbl * rat = 0;
	AcctRelTbl * rt = 0;
	GetAccAnlzTitle(Filt.Aco, Filt.AccID, Filt.CurID, sub_title).Space().Cat(Filt.Period);
	if(Filt.Flags & AccAnlzFilt::fTrnovrBySheet) {
		DBE  * p_dbe1 = 0, * p_dbe2 = 0;
		THROW(CheckTblPtr(ttt = new TempAccTrnovrTbl(P_TmpATTbl->GetName())));
		PPDbqFuncPool::InitObjNameFunc(dbe_cur, PPDbqFuncPool::IdObjSymbCurrency, ttt->CurID);
		p_dbe1 = &(0 - ttt->InRest);  // @warn unary '-' not defined in class DBField
		p_dbe2 = &(0 - ttt->OutRest); // @warn unary '-' not defined in class DBField
		q = & select(
			ttt->AccRelID,  // #00
			ttt->Dt,        // #01
			ttt->Ac,        // #02
			ttt->Sb,        // #03
			ttt->Ar,        // #04
			ttt->CurID,     // #05
			dbe_cur,        // #06
			ttt->InRest,    // #07
			ttt->Dbt,       // #08
			ttt->Crd,       // #09
			ttt->OutRest,   // #10
			ttt->Name,      // #11
			ttt->GoodsRest, // #12
			*p_dbe1,        // #13
			*p_dbe2,        // #14
			//ttt->DispFlags, // #15 @v7.1.2
			0L).from(ttt, 0L).orderBy(ttt->Dt, ttt->Name, 0L);
		delete p_dbe1;
		delete p_dbe2;
		if(Filt.Flags & AccAnlzFilt::fTrnovrBySuppl)
			brw_id = BROWSER_SUPPLTRNOVR;
		else
			brw_id = BROWSER_ACCTRNOVR;
	}
	else if(Filt.Flags & AccAnlzFilt::fGroupByCorAcc || Filt.Cycl.Cycle) {
		THROW(CheckTblPtr(ttt = new TempAccTrnovrTbl(P_TmpATTbl->GetName())));
		PPDbqFuncPool::InitObjNameFunc(dbe_cur, PPDbqFuncPool::IdObjSymbCurrency, ttt->CurID);
		q = & select(
			ttt->AccRelID,  // #00
			ttt->Dt,        // #01
			ttt->Ac,        // #02
			ttt->Sb,        // #03
			ttt->Ar,        // #04
			ttt->CurID,     // #05
			dbe_cur,        // #06
			ttt->InRest,    // #07
			ttt->Dbt,       // #08
			ttt->Crd,       // #09
			ttt->OutRest,   // #10
			ttt->Name,      // #11
			ttt->GoodsRest, // #12
			0L).from(ttt, 0L).orderBy(ttt->Dt, ttt->Name, 0L);
		if(Filt.Flags & AccAnlzFilt::fGroupByCorAcc) {
			if(Filt.CorAco == AccAnlzFilt::aafgByOp)
				brw_id = !Filt.Cycl ? BROWSER_ACCTOOP : BROWSER_ACCTOOP_CYCLE;
			else if(Filt.CorAco == AccAnlzFilt::aafgByLoc)
				brw_id = !Filt.Cycl ? BROWSER_ACCTOLOC : BROWSER_ACCTOLOC_CYCLE;
			else
				brw_id = !Filt.Cycl ? BROWSER_ACCTOBAL : BROWSER_ACCTOBAL_CYCLE;
		}
		else
			brw_id = BROWSER_ACCTOTAL_CYCLE;
	}
	else {
		if(IsRegister) {
			DBE * dbe_crd = 0;
			THROW(CheckTblPtr(rat = new AccTurnTbl));
			THROW(CheckTblPtr(rt = new AcctRelTbl));
			{
				/* @v9.6.1
				dbe_ar.init();
				dbe_ar.push(rt->ArticleID);
				dbe_ar.push(rt->AccID);
				dbe_ar.push(static_cast<DBFunc>(PPDbqFuncPool::IdObjNameArByAcc));
				*/
				PPDbqFuncPool::InitFunc2Arg(dbe_ar, PPDbqFuncPool::IdObjNameArByAcc, rt->ArticleID, rt->AccID); // @v9.6.1
			}
			dbe_crd = & (rat->Amount * -1);
			PPDbqFuncPool::InitObjNameFunc(dbe_bill_code, PPDbqFuncPool::IdObjCodeBill, rat->Bill);
			PPDbqFuncPool::InitObjNameFunc(dbe_bill_memo, PPDbqFuncPool::IdObjMemoBill, rat->Bill);
			q = & select(
				rat->Dt,       // #00
				rat->OprNo,    // #01
				rat->Bill,     // #02
				rat->Acc,      // #03
				rt->AccID,     // #04
				dbe_bill_code, // #05
				dbe_ar,        // #06
				rat->Amount,   // #07
				*dbe_crd,      // #08
				rat->Rest,     // #09
				dbe_bill_memo, // #10
				0L).from(rat, rt, 0L);
			delete dbe_crd;
			dbq = & (daterange(rat->Dt, &Filt.Period) && rt->ID == rat->Acc);
			brw_id = BROWSER_ACCREGISTEROPS;
			if(Filt.Aco == ACO_3)
				q->where(rat->Acc == Filt.AccID && *dbq).orderBy(rat->Acc, rat->Dt, rat->OprNo, 0L);
			else // if(Filt.Aco == ACO_2)
				q->where(rat->Bal == Filt.AccID && *dbq).orderBy(rat->Bal, rat->Dt, rat->OprNo, 0L);
		}
		else {
			THROW(CheckTblPtr(att = new TempAccAnlzTbl(P_TmpAATbl->GetName())));
			THROW(CheckTblPtr(rt = new AcctRelTbl));
			{
				/* @v9.6.1
				dbe_ar.init();
				dbe_ar.push(rt->ArticleID);
				dbe_ar.push(rt->AccID);
				dbe_ar.push(static_cast<DBFunc>(PPDbqFuncPool::IdObjNameArByAcc));
				*/
				PPDbqFuncPool::InitFunc2Arg(dbe_ar, PPDbqFuncPool::IdObjNameArByAcc, rt->ArticleID, rt->AccID); // @v9.6.1
			}
			PPDbqFuncPool::InitObjNameFunc(dbe_cur, PPDbqFuncPool::IdObjSymbCurrency, att->CurID);
			PPDbqFuncPool::InitObjNameFunc(dbe_bill_code, PPDbqFuncPool::IdObjCodeBill, att->BillID);
			PPDbqFuncPool::InitObjNameFunc(dbe_bill_memo, PPDbqFuncPool::IdObjMemoBill, att->BillID);
			q = & select(
				att->Dt,       // #00
				att->OprNo,    // #01
				att->BillID,   // #02
				att->Acc,      // #03
				rt->AccID,     // #04
				dbe_bill_code, // #05
				att->Ac,       // #06
				att->Sb,       // #07
				att->Ar,       // #08
				att->CurID,    // #09
				dbe_cur,       // #10
				att->Dbt,      // #11
				att->Crd,      // #12
				att->Rest,     // #13
				dbe_bill_memo, // #14
				dbe_ar,        // #15
				0L).from(att, rt, 0L).where((rt->ID += att->Acc));
			brw_id = BROWSER_ACCTOACC;
		}
	}
	THROW(CheckQueryPtr(q));
	ASSIGN_PTR(pSubTitle, sub_title);
	CATCH
		ZDELETE(q);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewAccAnlz::GetBrwHdr(const void * pRow, BrwHdr * pHdr) const
{
	int    ok = 0;
	BrwHdr hdr;
	MEMSZERO(hdr);
	if(pRow) {
		const  char * p_hdr = static_cast<const char *>(pRow);
		if((Filt.Flags & (AccAnlzFilt::fGroupByCorAcc |	AccAnlzFilt::fTrnovrBySheet)) || Filt.Cycl.Cycle) {
			hdr.AccRelID = *reinterpret_cast<const PPID *>(p_hdr);
			hdr.Dt = *reinterpret_cast<const LDATE *>(p_hdr + sizeof(PPID));
			hdr.A = *reinterpret_cast<const Acct *>(p_hdr + sizeof(PPID) + sizeof(LDATE));
			hdr.CurID = *reinterpret_cast<const PPID *>(p_hdr + sizeof(PPID) + sizeof(LDATE) + sizeof(Acct));
		}
		else {
			hdr.Dt = *reinterpret_cast<const LDATE *>(p_hdr);
			hdr.OprNo = *reinterpret_cast<const long *>(p_hdr+sizeof(LDATE));
			hdr.BillID = *reinterpret_cast<const PPID *>(p_hdr+sizeof(long)+sizeof(LDATE));
			hdr.AccRelID = *reinterpret_cast<const PPID *>(p_hdr+sizeof(long)+sizeof(LDATE)+sizeof(PPID));
			hdr.CorAccID = *reinterpret_cast<const PPID *>(p_hdr+sizeof(long)+sizeof(LDATE)+sizeof(PPID)*2);
		}
		ok = 1;
	}
	ASSIGN_PTR(pHdr, hdr);
	return ok;
}

// virtual
int SLAPI PPViewAccAnlz::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -2;
	if(ppvCmd != PPVCMD_DETAIL)
		ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr hdr;
		int    r = 0;
		switch(ppvCmd) {
			case PPVCMD_DETAIL:
			case PPVCMD_EDITITEM:
				ok = -1;
				if(GetBrwHdr(pHdr, &hdr)) {
					if((Filt.Flags & (AccAnlzFilt::fGroupByCorAcc |	AccAnlzFilt::fTrnovrBySheet)) || Filt.Cycl.Cycle) {
						AccAnlzFilt flt = Filt;
						flt.Flags &= ~AccAnlzFilt::fAllCurrencies;
						flt.CurID = hdr.CurID;
						if(flt.Flags & AccAnlzFilt::fTrnovrBySheet) {
							if(IsGenAcc) {
								if(P_ATC->Art.SearchNum(flt.AccSheetID, hdr.A.ar) > 0)
									flt.SingleArID = P_ATC->Art.data.ID;
								else
									ok = 0;
							}
							/*
							else if(IsGenAr) {
								flt.Aco = ACO_3;
							}
							*/
							else {
								flt.Aco = ACO_3;
								AcctRelTbl::Rec acr_rec;
								if(P_ATC->AccRel.SearchNum(0, &hdr.A, hdr.CurID, &acr_rec) > 0) {
									flt.AccID = acr_rec.ID;
									// @v9.5.11 (Если Filt.AcctId.ar группирующая статья, то это - необходимо) {
									flt.AcctId.ac = acr_rec.AccID;
									flt.AcctId.ar = acr_rec.ArticleID;
									flt.SingleArID = acr_rec.ArticleID;
									// } @v9.5.11
								}
								else
									ok = 0;
							}
						}
						else if(Filt.Cycl.Cycle && CycleList.searchPeriodByDate(hdr.Dt, &flt.Period) <= 0)
							ok = 0;
						else if(Filt.CorAco == AccAnlzFilt::aafgByLoc) {
							flt.CorAco = 0;
							flt.LocID = hdr.A.ar;
						}
						else if(Filt.CorAco == AccAnlzFilt::aafgByExtObj) {
							flt.CorAco = 0;
							flt.Object2ID = hdr.A.ar;
						}
						else if(Filt.CorAco == AccAnlzFilt::aafgByAgent) {
							flt.CorAco = 0;
							flt.AgentID = hdr.A.ar;
						}
						else {
							flt.CorAco = Filt.CorAco;
							flt.CorAcc = hdr.A;
						}
						if(ok != 0) {
							flt.Flags &= ~(AccAnlzFilt::fGroupByCorAcc | AccAnlzFilt::fTrnovrBySheet);
							flt.Cycl.Init();
							ok = ViewAccAnlz(&flt, aakndGeneric) ? -1 : 0;
						}
						else
							ok = -1;
					}
					else if(hdr.BillID) {
						ok = P_BObj->Edit(&hdr.BillID, 0);
						if(ok == cmOK)
							ok = 1;
						else if(ok != 0)
							ok = -1;
					}
				}
				break;
			case PPVCMD_ADDITEM:
				ok = -1;
				if(IsRegister) {
					PPID   bill_id = 0;
					PPObjBill::AddBlock ab;
					ab.RegisterID = Filt.AccID;
					ok = P_BObj->AddAccturn(&bill_id, &ab);
				}
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				break;
			case PPVCMD_VIEWLOTS:
				ok = -1;
				if(Filt.Flags & AccAnlzFilt::fTrnovrBySuppl && GetBrwHdr(pHdr, &hdr)) {
					if(P_ATC->AccRel.SearchNum(0, &hdr.A, hdr.CurID) > 0) {
						const PPID suppl_id = P_ATC->AccRel.data.ArticleID;
						if(suppl_id)
							ViewLots(0, 0, suppl_id, 0, 1);
					}
				}
				break;
			case PPVCMD_VIEWBILLS:
				ok = -1;
				if(Filt.Flags & AccAnlzFilt::fTrnovrBySuppl && GetBrwHdr(pHdr, &hdr)) {
					if(P_ATC->AccRel.SearchNum(0, &hdr.A, hdr.CurID) > 0) {
						const PPID suppl_id = P_ATC->AccRel.data.ArticleID;
						if(suppl_id) {
							BillFilt flt;
							flt.Period = Filt.Period;
							flt.ObjectID = suppl_id;
							ViewGoodsBills(&flt, 1);
						}
					}
				}
				break;
			case PPVCMD_GOODSREST:
				ok = -1;
				if(Filt.Flags & AccAnlzFilt::fTrnovrBySuppl && GetBrwHdr(pHdr, &hdr)) {
   					if(P_ATC->AccRel.SearchNum(0, &hdr.A, hdr.CurID) > 0) {
						const PPID suppl_id = P_ATC->AccRel.data.ArticleID;
						if(suppl_id) {
							GoodsRestFilt flt;
							flt.Init(1, 0);
							flt.Date = Filt.Period.upp;
							flt.SupplID = suppl_id;
							flt.LocList.Add(Filt.LocID);
							ViewGoodsRest(&flt, 0);
						}
					}
				}
				break;
			case PPVCMD_GRAPH:
				ok = ViewGraph(pBrw);
				break;
			case PPVCMD_REFRESH:
				ok = ChangeFilt(1, pBrw);
		}
	}
	if(ok > 0 && oneof3(ppvCmd, PPVCMD_DETAIL, PPVCMD_ADDITEM, PPVCMD_DELETEITEM)) {
		CALLPTRMEMB(pBrw, Update());
	}
	return ok;
}

int SLAPI PPViewAccAnlz::Browse(int modeless)
{
	if(Filt.Flags & AccAnlzFilt::fTotalOnly)
		return ViewTotal();
	else
		return PPView::Browse(modeless);
}
//
// AccAnlzTotal
//
SLAPI AccAnlzTotal::AccAnlzTotal()
{
	Init();
}

void SLAPI AccAnlzTotal::Init()
{
	Count = DbtCount = CrdCount = 0;
	InRest.freeAll();
	OutRest.freeAll();
	DbtTrnovr.freeAll();
	CrdTrnovr.freeAll();
	InRestDbt.freeAll();
	InRestCrd.freeAll();
	OutRestDbt.freeAll();
	OutRestCrd.freeAll();
}

int SLAPI AccAnlzTotal::GetCurList(PPIDArray * pCurList) const
{
	int    ok = 1;
	if(pCurList) {
		PPIDArray list;
		InRest.GetCurList(0, &list);
		DbtTrnovr.GetCurList(0, &list);
		CrdTrnovr.GetCurList(0, &list);
		OutRest.GetCurList(0, &list);
		*pCurList = list;
	}
	else
		ok = -1;
	return ok;
}

int SLAPI AccAnlzTotal::GetCut(PPID curID, AccAnlzTotal::Cut * pCut) const
{
	int    ok = 1;
	AccAnlzTotal::Cut cut;
	cut.CDbtCount  = (long)DbtTrnovr.Get(1L, curID);
	cut.CCrdCount  = (long)CrdTrnovr.Get(1L, curID);
	cut.CInRest    = InRest.Get(0, curID);
	cut.COutRest   = OutRest.Get(0, curID);
	cut.CDbtTrnovr = DbtTrnovr.Get(0, curID);
	cut.CCrdTrnovr = CrdTrnovr.Get(0, curID);
	if(cut.CInRest == 0 && cut.COutRest == 0 && cut.CDbtCount == 0 && cut.CCrdCount == 0)
		ok = -1;
	ASSIGN_PTR(pCut, cut);
	return ok;
}

void SLAPI AccAnlzTotal::AddTrnovr(int dbt, PPID curID, double amt)
{
	if(dbt) {
		DbtTrnovr.Add(0, curID, amt);
		DbtCount++;
	}
	else {
		CrdTrnovr.Add(0, curID, amt);
		CrdCount++;
	}
}
//
//
//
class AccAnlzTotalDialog : public TDialog {
public:
	AccAnlzTotalDialog(AccAnlzTotal * pTotal, const AccAnlzFilt * pFilt) : TDialog(DLG_AANLZTOTAL)
	{
		Filt = *pFilt;
		Total = *pTotal;
		PPID cur_id = (Filt.Flags & AccAnlzFilt::fAllCurrencies) ? -1 : Filt.CurID;
		::SetupCurrencyCombo(this, CTLSEL_AANLZTOTAL_CUR, (cur_id > 0) ? cur_id : 0, 0, 1, 0);
		setupCur((cur_id >= 0) ? cur_id : 0L);
		if(cur_id >= 0)
			disableCtrl(CTLSEL_AANLZTOTAL_CUR, 1);
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_AANLZTOTAL_CUR)) {
			setupCur(getCtrlLong(CTLSEL_AANLZTOTAL_CUR));
			clearEvent(event);
		}
	}
	void   setupCur(PPID curID)
	{
		char   diff_buf[32];
		SString acc_buf;
		SetPeriodInput(this, CTL_AANLZTOTAL_PERIOD, &Filt.Period);
		setCtrlString(CTL_AANLZTOTAL_ACC, GetAccAnlzTitle(Filt.Aco, Filt.AccID, curID, acc_buf));
		setCtrlLong(CTL_AANLZTOTAL_COUNT, Total.Count);
		double val  = Total.InRest.Get(0, curID);
		double diff = val;
		setCtrlReal(CTL_AANLZTOTAL_INREST,  val);
		setCtrlReal(CTL_AANLZTOTAL_INRESTD, Total.InRestDbt.Get(0, curID));
		setCtrlReal(CTL_AANLZTOTAL_INRESTC, Total.InRestCrd.Get(0, curID));
		val = Total.DbtTrnovr.Get(0, curID);
		diff += val;
		setCtrlReal(CTL_AANLZTOTAL_DBT, val);
		val = Total.CrdTrnovr.Get(0, curID);
		diff -= val;
		setCtrlReal(CTL_AANLZTOTAL_CRD, val);
		val = Total.OutRest.Get(0, curID);
		diff -= val;
		setCtrlReal(CTL_AANLZTOTAL_OUTREST,  val);
		setCtrlReal(CTL_AANLZTOTAL_OUTRESTD, Total.OutRestDbt.Get(0, curID));
		setCtrlReal(CTL_AANLZTOTAL_OUTRESTC, Total.OutRestCrd.Get(0, curID));
		setStaticText(CTL_AANLZTOTAL_DIFF, realfmt(diff, MKSFMTD(0, 2, NMBF_NOZERO), diff_buf));
	}
	AccAnlzFilt Filt;
	AccAnlzTotal Total;
};

int SLAPI PPViewAccAnlz::CalcTotalAccTrnovr(AccAnlzTotal * pTotal)
{
	int    ok = -1;
	if(pTotal) {
		AccAnlzViewItem item;
		pTotal->InRestDbt.freeAll();
		pTotal->InRestCrd.freeAll();
		pTotal->OutRestDbt.freeAll();
		pTotal->OutRestCrd.freeAll();
		for(InitIteration(); NextIteration(&item) > 0; ) {
			if(item.InRest > 0)
				pTotal->InRestDbt.Add(0, item.CurID, item.InRest);
			else
				pTotal->InRestCrd.Add(0, item.CurID, -item.InRest);
			if(item.OutRest > 0)
				pTotal->OutRestDbt.Add(0, item.CurID, item.OutRest);
			else
				pTotal->OutRestCrd.Add(0, item.CurID, -item.OutRest);
		}
		ok = 1;
	}
	return ok;
}

// virtual
int SLAPI PPViewAccAnlz::ViewTotal()
{
	int    ok = 1;
	if(Filt.Flags & AccAnlzFilt::fTrnovrBySheet)
		CalcTotalAccTrnovr(&Total);
	AccAnlzTotalDialog * dlg = new AccAnlzTotalDialog(&Total, &Filt);
	if(CheckDialogPtrErr(&dlg))
		ExecViewAndDestroy(dlg);
	else
		ok = 0;
	return ok;
}
//
//
//
static int SLAPI SelectPrintingAccSheetTrnovr(int * pWhat, LDATE * pExpiry, uint * pFlags)
{
	class SelPrnAtDialog : public TDialog {
	public:
		SelPrnAtDialog() : TDialog(DLG_SELPRNAT)
		{
			SetupCalDate(CTLCAL_SELPRNAT_EXPIRY, CTL_SELPRNAT_EXPIRY);
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_SELPRNAT_SEL)) {
				disableCtrls(getCtrlUInt16(CTL_SELPRNAT_SEL) != 3,
					CTL_SELPRNAT_FLAGS, CTL_SELPRNAT_EXPIRY, CTLCAL_SELPRNAT_EXPIRY, 0);
				clearEvent(event);
			}
		}
	};
	int    ok = -1;
	SelPrnAtDialog * dlg = new SelPrnAtDialog();
	if(CheckDialogPtrErr(&dlg)) {
		ushort v = *pWhat;
		dlg->setCtrlData(CTL_SELPRNAT_SEL, &v);
		dlg->setCtrlData(CTL_SELPRNAT_EXPIRY, pExpiry);
		dlg->disableCtrls(v != 3, CTL_SELPRNAT_FLAGS, CTL_SELPRNAT_EXPIRY, CTLCAL_SELPRNAT_EXPIRY, 0);
		v = 0;
		SETFLAG(v, 0x01, *pFlags & PPViewAccAnlz::fIterNegRest);
		dlg->setCtrlData(CTL_SELPRNAT_FLAGS, &v);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_SELPRNAT_SEL, &v);
			*pWhat = v;
			dlg->getCtrlData(CTL_SELPRNAT_EXPIRY, pExpiry);
			dlg->getCtrlData(CTL_SELPRNAT_FLAGS,  &(v = 0));
			SETFLAG(*pFlags, PPViewAccAnlz::fIterNegRest, v & 0x01);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

// virtual
int SLAPI PPViewAccAnlz::Print(const void *)
{
	int    ok = 1, done = 0;
	int    disable_grpng = 0;
	uint   rpt_id = 0;
	TDialog * dlg = 0;
	if(Filt.Flags & AccAnlzFilt::fTotalOnly)
		ok = -1;
	else if(Filt.Flags & AccAnlzFilt::fGroupByCorAcc) {
		if(Filt.CorAco == AccAnlzFilt::aafgByOp)
			rpt_id = REPORT_ACCGRPNGBYOP;
		else if(Filt.CorAco == AccAnlzFilt::aafgByLoc)
			rpt_id = REPORT_ACCGRPNGBYLOC;
		else
			rpt_id = REPORT_ACCGRPNG;
	}
	else if(Filt.Flags & AccAnlzFilt::fTrnovrBySheet) {
		int    prn_sel = 0;
		uint   iter_flags = IterFlags;
		LDATE  expiry = ExpiryDate;
		if(SelectPrintingAccSheetTrnovr(&prn_sel, &expiry, &iter_flags) > 0) {
			if(prn_sel == 0)
				rpt_id = (Filt.Flags & AccAnlzFilt::fTrnovrBySuppl) ? REPORT_SUPPLTRNOVR : REPORT_ACCTRNOVR;
			else if(prn_sel == 1)
				rpt_id = REPORT_DBTRSSTMNT;
			else if(prn_sel == 2)
				rpt_id = REPORT_CRDTRSSTMNT;
			else {
				ExpiryDate = expiry;
				IterFlags  = iter_flags;
				rpt_id = REPORT_DEBTACK;
			}
		}
		else
			ok = -1;
	}
	else if(Filt.Flags & AccAnlzFilt::fAsCashBook) {
		ushort v = 0;
		THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PRNCASHBOOK))));
		dlg->setCtrlData(CTL_PRNCASHBOOK_WHAT, &v);
		if(ExecView(dlg) == cmOK)
			rpt_id = dlg->getCtrlUInt16(CTL_PRNCASHBOOK_WHAT) ? REPORT_CASHBOOK2 : REPORT_CASHBOOK;
		else
			ok = -1;
		delete dlg;
		dlg = 0;
		if(ok > 0 && Filt.Period.low != Filt.Period.upp) {
			int    leaf_no = Filt.LeafNo;
			PPViewAccAnlz temp_view;
			for(LDATE dt = Filt.Period.low; dt <= Filt.Period.upp; plusdate(&dt, 1, 0)) {
				int r;
				AccAnlzFilt temp_flt = Filt;
				temp_flt.Period.SetDate(dt);
				temp_flt.LeafNo = leaf_no;
				THROW(temp_view.Init_(&temp_flt));
				THROW(r = temp_view.Print(0));
				leaf_no = temp_flt.LeafNo+1;
				if(r < 0)
					break;
			}
			done = 1;
		}
	}
	else if(!Filt.Cycl) {
		rpt_id = REPORT_ACCANLZ;
	}
	else
		rpt_id = REPORT_ACCGRPNGCYCLE;
	if(!done && ok > 0) {
		PView  pv(this);
		PPReportEnv env;
		env.PrnFlags = disable_grpng ? SReport::DisableGrouping : 0;
		PPAlddPrint(rpt_id, &pv, &env);
	}
	CATCHZOKPPERR
	IterFlags = 0;
	return ok;
}
//
//
//
int SLAPI ViewAccAnlz(const AccAnlzFilt * pFilt, AccAnlzKind aak) { return PPView::Execute(PPVIEW_ACCANLZ, pFilt, GetModelessStatus(), reinterpret_cast<void *>(aak)); }
//
// Implementation of PPALDD_Account
//
PPALDD_CONSTRUCTOR(Account)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjAccount(0);
	}
}

PPALDD_DESTRUCTOR(Account)
{
	Destroy();
	delete static_cast<PPObjAccount *>(Extra[0].Ptr);
	Extra[0].Ptr = 0;
}

int PPALDD_Account::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPAccount rec;
		if(static_cast<PPObjAccount *>(Extra[0].Ptr)->Search(rFilt.ID, &rec) > 0) {
			H.ID    = rec.ID;
			H.Ac    = rec.A.Ac;
			H.Sb    = rec.A.Sb;
			STRNSCPY(H.Code, rec.Code);
			H.CurID = rec.CurID;
			H.Kind  = rec.Kind;
			H.Type  = rec.Type;
			H.Flags = rec.Flags;
			H.AccSheetID = rec.AccSheetID;
			STRNSCPY(H.Name, rec.Name);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_Article
//
PPALDD_CONSTRUCTOR(Article)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjArticle;
	}
}

PPALDD_DESTRUCTOR(Article)
{
	Destroy();
	delete static_cast<PPObjArticle *>(Extra[0].Ptr);
	Extra[0].Ptr = 0;
}

int PPALDD_Article::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		ArticleTbl::Rec rec;
		if(static_cast<PPObjArticle *>(Extra[0].Ptr)->Search(rFilt.ID, &rec) > 0) {
			H.ID      = rec.ID;
			H.Number  = rec.Article;
			H.SheetID = rec.AccSheetID;
			H.AgrID   = rec.ID;
			H.Closed  = rec.Closed;
			H.Group   = BIN(rec.Flags & ARTRF_GROUP);
			H.Stop    = BIN(rec.Flags & ARTRF_STOPBILL);
			STRNSCPY(H.Name, rec.Name);
			PPObjAccSheet acc_sheet_obj;
			PPAccSheet acs_rec;
			if(acc_sheet_obj.Fetch(rec.AccSheetID, &acs_rec) > 0) {
				H.LinkObjType = acs_rec.Assoc;
				if(acs_rec.Assoc == PPOBJ_PERSON)
					H.PersonID = rec.ObjID;
				else if(acs_rec.Assoc == PPOBJ_LOCATION)
					H.WhID = rec.ObjID;
			}
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_AccAnlz
//
PPALDD_CONSTRUCTOR(AccAnlz)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(AccAnlz) { Destroy(); }

int PPALDD_AccAnlz::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(AccAnlz, rsrv);
	AccAnlzTotal total;
	SString temp_buf;
	H.LeafNo       = p_filt->LeafNo;
	H.FltBeg       = p_filt->Period.low;
	H.FltEnd       = p_filt->Period.upp;
	PPFormatPeriod(&p_filt->Period, temp_buf).CopyTo(H.FltPeriod, sizeof(H.FltPeriod));
	H.ExpiryDate   = p_v->ExpiryDate;
	H.Aco          = (int16)p_filt->Aco;
	H.CorAco       = (int16)p_filt->CorAco;
	H.FltCycle     = p_filt->Cycl.Cycle;
	H.FltNumCycles = p_filt->Cycl.NumCycles;
	H.FltOrder     = (int16)p_filt->InitOrder;
	if(p_filt->Aco == ACO_3) {
		H.FltAccID    = p_filt->AcctId.ac;
		H.FltArID     = p_filt->AcctId.ar;
		H.FltAccRelID = p_filt->AccID;
	}
	else {
		H.FltAccID    = p_filt->AccID;
		H.FltArID     = 0;
		H.FltAccRelID = 0;
	}
	{
		PPAccount acc_rec;
		if(SearchObject(PPOBJ_ACCOUNT2, H.FltAccID, &acc_rec) > 0)
			if(acc_rec.Type == ACY_AGGR)
				H.FltArID = p_filt->SingleArID;
	}
	p_filt->GetAccText(H.FltAccText, sizeof(H.FltAccText));
	H.FltCurID       = p_filt->CurID;
	H.FltFlags       = p_filt->Flags;
	H.fAllCurrencies = BIN(p_filt->Flags & AccAnlzFilt::fAllCurrencies);
	H.fWlOnly        = BIN(p_filt->Flags & AccAnlzFilt::fLabelOnly);
	H.fGroupByCorAcc = BIN(p_filt->Flags & AccAnlzFilt::fGroupByCorAcc);
	H.fTrnovrBySheet = BIN(p_filt->Flags & AccAnlzFilt::fTrnovrBySheet);
	H.fTrnovrBySuppl = BIN(p_filt->Flags & AccAnlzFilt::fTrnovrBySuppl);
	p_v->GetTotal(&total);
	if(H.fAllCurrencies) {
		H.TotalInRest  = total.InRest.Get(0, 0);
		H.TotalOutRest = total.OutRest.Get(0, 0);
	}
	else {
		H.TotalInRest  = total.InRest.Get(0, H.FltCurID);
		H.TotalOutRest = total.OutRest.Get(0, H.FltCurID);
	}
	H.TotalCount    = total.Count;
	H.TotalDbtCount = total.DbtCount;
	H.TotalCrdCount = total.CrdCount;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_AccAnlz::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(AccAnlz);
}

int PPALDD_AccAnlz::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(AccAnlz);
	p_v->FormatCycle(item.Dt, I.CycleText, sizeof(I.CycleText));
	I.Dt       = item.Dt;
	I.OprNo    = item.OprNo;
	STRNSCPY(I.OrderText, item.OrderText);
	I.BillID   = item.BillID;
	I.RByBill  = item.RByBill;
	I.Reverse  = item.Reverse;
	I.ThisAccRelID = item.ThisAccRelID;
	I.CorAccID = item.AccID;
	I.CorAccRelID = item.AccRelID;
	STRNSCPY(I.CorAccName, item.AccName);
	I.CurID    = item.CurID;
	I.RelPersonID = item.RelPersonID;
	I.Count    = item.Count;
	I.InRest   = item.InRest;
	I.DbtAmt   = item.DbtAmt;
	I.CrdAmt   = item.CrdAmt;
	I.OutRest  = item.OutRest;
	I.GoodsRestAmt = item.SupplGoodsRestAmt;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_AccAnlz::Destroy()
{
	DESTROY_PPVIEW_ALDD(AccAnlz);
}
//
// Implementation of PPALDD_AccturnList
//
PPALDD_CONSTRUCTOR(AccturnList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(AccturnList) { Destroy(); }

int PPALDD_AccturnList::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Accturn, rsrv);
	H.FltBeg       = p_filt->Period.low;
	H.FltEnd       = p_filt->Period.upp;
	H.FltOprKindID = p_filt->OpID;
	H.FltCurID     = p_filt->CurID;
	H.GrpAco       = p_filt->GrpAco;
	H.FltCycle     = p_filt->Cycl.Cycle;
	H.FltNumCycles = p_filt->Cycl.NumCycles;
	H.Aco          = p_filt->Aco;
	H.FltDbtAc     = p_filt->DbtAcct.ac;
	H.FltDbtSb     = p_filt->DbtAcct.sb;
	H.FltDbtAr     = p_filt->DbtAcct.ar;
	H.FltCrdAc     = p_filt->CrdAcct.ac;
	H.FltCrdSb     = p_filt->CrdAcct.sb;
	H.FltCrdAr     = p_filt->CrdAcct.ar;
	H.FltMinAmount = p_filt->AmtR.low;
	H.FltMaxAmount = p_filt->AmtR.upp;
	H.fAllCurrencies = BIN(p_filt->Flags & AccturnFilt::fAllCurrencies);
	H.fWlOnly        = BIN(p_filt->Flags & AccturnFilt::fLabelOnly);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_AccturnList::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(Accturn);
}

int PPALDD_AccturnList::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Accturn);
	p_v->FormatCycle(item.Date, I.CycleText, sizeof(I.CycleText));
	I.Dt       = item.Date;
	I.OprNo    = item.OprNo;
	I.BillID   = item.BillID;
	I.RByBill  = item.RByBill;
	I.DbtAccID = item.DbtID.ac;
	I.DbtRelID = item.DbtAccRelID;
	I.CrdAccID = item.CrdID.ac;
	I.CrdRelID = item.CrdAccRelID;
	I.CurID    = item.CurID;
	I.CRate    = item.CRate;
	I.Amount   = item.Amount;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_AccturnList::Destroy()
{
	DESTROY_PPVIEW_ALDD(Accturn);
}
//
// Implementation of PPALDD_AccountView
//
PPALDD_CONSTRUCTOR(AccountView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(AccountView) { Destroy(); }

int PPALDD_AccountView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Account, rsrv);
	H.FltType  = p_filt->Type;
	H.FltFlags = p_filt->Flags;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_AccountView::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(Account);
}

int PPALDD_AccountView::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Account);
	I.AccID = item.ID;
	STRNSCPY(I.CurListTxt, item.CurList);
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_AccountView::Destroy()
{
	DESTROY_PPVIEW_ALDD(Account);
}
//
// Implementation of PPALDD_Currency
//
PPALDD_CONSTRUCTOR(Currency)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(Currency) { Destroy(); }

int PPALDD_Currency::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		H.ID = rFilt.ID;
		PPObjCurrency cur_obj;
		PPCurrency cur_rec;
		if(cur_obj.Fetch(rFilt.ID, &cur_rec) > 0) {
			H.ID = cur_rec.ID;
			H.DigitCode = cur_rec.Code;
			STRNSCPY(H.Name, cur_rec.Name);
			STRNSCPY(H.Symb, cur_rec.Symb);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

// @Muxa {
int PPALDD_Currency::Set(long iterId, int commit)
{
	int    ok = 1;
	SETIFZ(Extra[3].Ptr, new PPCurrency);
	PPCurrency * p_cur_rec = static_cast<PPCurrency *>(Extra[3].Ptr);
	if(commit == 0) {
		if(iterId == 0) {
			p_cur_rec->ID = 0;
			p_cur_rec->Code = H.DigitCode;
			STRNSCPY(p_cur_rec->Name, strip(H.Name));
			STRNSCPY(p_cur_rec->Symb, strip(H.Symb));
		}
	}
	else {
		PPObjCurrency cur_obj;
		PPID  id = 0;
		THROW(cur_obj.AddItem(&id, p_cur_rec, 1));
		Extra[4].Ptr = reinterpret_cast<void *>(p_cur_rec->ID);
	}
	CATCHZOK
	if(commit) {
		delete p_cur_rec;
		Extra[3].Ptr = 0;
	}
	return ok;
}
// } @Muxa
//
// Implementation of PPALDD_CurRateType
//
PPALDD_CONSTRUCTOR(CurRateType)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(CurRateType) { Destroy(); }

int PPALDD_CurRateType::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		H.ID = rFilt.ID;
		PPCurRateType rec;
		if(SearchObject(PPOBJ_CURRATETYPE, rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Symb, rec.Symb);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
// @Muxa {
int PPALDD_CurRateType::Set(long iterId, int commit)
{
	int    ok = 1;
	SETIFZ(Extra[3].Ptr, new PPCurRateType);
	PPCurRateType * p_crt_rec = static_cast<PPCurRateType *>(Extra[3].Ptr);
	if(commit == 0) {
		if(iterId == 0) {
			p_crt_rec->ID = 0;
			STRNSCPY(p_crt_rec->Name, strip(H.Name));
			STRNSCPY(p_crt_rec->Symb, strip(H.Symb));
		}
	}
	else {
		PPObjCurRateType crt_obj;
		PPID  id = 0;
		THROW(crt_obj.AddItem(&id, p_crt_rec, 1));
		Extra[4].Ptr = reinterpret_cast<void *>(id);
	}
	CATCHZOK
	if(commit) {
		delete p_crt_rec;
		Extra[3].Ptr = 0;
	}
	return ok;
} // } @Muxa
//
// Implementation of PPALDD_CurRateView
//
PPALDD_CONSTRUCTOR(CurRateView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(CurRateView) { Destroy(); }

int PPALDD_CurRateView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(CurRate, rsrv);
	H.FltCurID       = p_filt->CurID;
	H.FltBaseCurID   = p_filt->BaseCurID;
	H.FltRateTypeID  = p_filt->RateTypeID;
	H.FltBeg         = p_filt->Period.low;
	H.FltEnd         = p_filt->Period.upp;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_CurRateView::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(CurRate);
}

int PPALDD_CurRateView::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(CurRate);
	I.CurID      = item.CurID;
	I.BaseCurID  = item.RelCurID;
	I.RateTypeID = item.RateTypeID;
	I.Dt         = item.Dt;
	I.Rate       = item.Rate;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_CurRateView::Destroy()
{
	DESTROY_PPVIEW_ALDD(CurRate);
}

// @Muxa {
//
// Implementation of PPALDD_UhttCurRateIdent
//
struct UhttCurRateIdentBlock {
	UhttCurRateIdentBlock()
	{
		Clear();
	}
	void Clear()
	{
		MEMSZERO(Rec);
	}
	CurRateCore   CrCore;
	CurRateIdent  Rec;
};

PPALDD_CONSTRUCTOR(UhttCurRateIdent)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new UhttCurRateIdentBlock;
	}
}

PPALDD_DESTRUCTOR(UhttCurRateIdent)
{
	Destroy();
	delete static_cast<UhttCurRateIdentBlock *>(Extra[0].Ptr);
}

int PPALDD_UhttCurRateIdent::InitData(PPFilt & rFilt, long rsrv)
{
	return -1;
}

int PPALDD_UhttCurRateIdent::Set(long iterId, int commit)
{
	int    ok = 1;
	UhttCurRateIdentBlock & r_blk = *static_cast<UhttCurRateIdentBlock *>(Extra[0].Ptr);
	if(commit == 0) {
		if(iterId == 0) {
			r_blk.Rec.CurID = H.CurID;
			r_blk.Rec.BaseCurID = H.BaseCurID;
			r_blk.Rec.RateTypeID = H.RateTypeID;
			r_blk.Rec.Dt = H.Dt;
		}
	}
	else
		THROW(r_blk.CrCore.UpdateRate(&r_blk.Rec, H.Rate, 1));
	CATCHZOK
	if(commit || !ok)
		r_blk.Clear();
	return ok;
}
// } @Muxa
//
// Implementation of PPALDD_AccSheet
//
PPALDD_CONSTRUCTOR(AccSheet)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(AccSheet) { Destroy(); }

int PPALDD_AccSheet::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		if(acs_obj.Fetch(rFilt.ID, &acs_rec) > 0) {
		   	H.ID = acs_rec.ID;
			STRNSCPY(H.Name, acs_rec.Name);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_AccRel
//
PPALDD_CONSTRUCTOR(AccRel)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = BillObj->atobj;
	}
}

PPALDD_DESTRUCTOR(AccRel)
{
	Destroy();
	Extra[0].Ptr = 0;
}

int PPALDD_AccRel::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		AcctRel & r_acr_tbl = BillObj->atobj->P_Tbl->AccRel;
		AcctRelTbl::Rec rec;
		if(r_acr_tbl.Fetch(rFilt.ID, &rec) > 0) {
			SString temp_buf;
			H.ID    = rec.ID;
			H.AccID = rec.AccID;
			H.ArID  = rec.ArticleID;
			H.Ac    = rec.Ac;
			H.Sb    = rec.Sb;
			H.Ar    = rec.Ar;
			H.CurID = rec.CurID;
			GetAcctName((Acct*)&H.Ac, H.CurID, 0, temp_buf);
			temp_buf.CopyTo(H.AccName, sizeof(H.AccName));
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
