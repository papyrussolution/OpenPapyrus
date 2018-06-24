// BILLDLG.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//@v9.9.5 #include <process.h>
//#include <stdio.h>
//@v9.9.5 #include <shlwapi.h>
//@v9.9.5 #include <comdisp.h>

// TInputLine setMaxLen

// Prototype
int SLAPI EditCurTransitBill(PPBillPacket *);
int SLAPI EditBillTaxes(AmtList *, double amount);
int SLAPI ViewAdvBillDetails(PPBillPacket * pPack, PPObjBill * pBObj);
int SLAPI EditPaymPlan(const PPBillPacket * pPack, PayPlanArray * pData);
//
//
#define GRP_DBT    1
#define GRP_CRD    2
#define GRP_CURAMT 3
#define GRP_FBG    4
#define GRP_QCERT  5

AccTurnDialog::AccTurnDialog(uint rezID, PPObjBill * pBObj) : TDialog(rezID), P_BObj(pBObj), P_Pack(0)
{
	MEMSZERO(Data);
	setCtrlOption(CTL_ATURN_DTEXT, ofFramed, 1);
	setCtrlOption(CTL_ATURN_CTEXT, ofFramed, 1);
	AcctCtrlGroup * p_ac_grp = new AcctCtrlGroup(CTL_ATURN_DACC, CTL_ATURN_DART, CTLSEL_ATURN_DACCNAME, CTLSEL_ATURN_DARTNAME);
	addGroup(GRP_DBT, p_ac_grp);
	p_ac_grp = new AcctCtrlGroup(CTL_ATURN_CACC, CTL_ATURN_CART, CTLSEL_ATURN_CACCNAME, CTLSEL_ATURN_CARTNAME);
	addGroup(GRP_CRD, p_ac_grp);
	CurAmtCtrlGroup * p_ca_grp = new CurAmtCtrlGroup(CTL_ATURN_AMOUNT, CTLSEL_ATURN_CUR, CTL_ATURN_CRATE, CTL_ATURN_BASEAMT, CTL_ATURN_DATE, 0, 0);
	addGroup(GRP_CURAMT, p_ca_grp);
	SetupCalDate(CTLCAL_ATURN_DATE, CTL_ATURN_DATE);
	setDTS(0, 0);
}

int AccTurnDialog::setDTS(PPAccTurn * pData, PPBillPacket * pPack, long templFlags)
{
	int    ok = 1;
	PPID   temp_acc_id;
	AcctCtrlGroup::Rec rec;
	CurAmtCtrlGroup::Rec ca_rec;
	P_Pack = pPack;
	if(P_Pack) {
		PPOprKind    op_rec;
		PPObjOprKind op_obj;
		if(!P_Pack->Rec.ID)
			P_BObj->SubstMemo(P_Pack);
		if(op_obj.Search(P_Pack->Rec.OpID, &op_rec) > 0)
			setTitle(op_rec.Name);
		setCtrlData(CTL_ATURN_MEMO, P_Pack->Rec.Memo);
		SetupPPObjCombo(this, CTLSEL_ATURN_LOCATION, PPOBJ_LOCATION, P_Pack->Rec.LocID, 0);
	}
	if(pData) {
		Data = *pData;
		if(Data.DbtID.ac)
			if(AccObj.SearchBase(Data.DbtID.ac, &temp_acc_id, 0) > 0)
				Data.DbtID.ac = temp_acc_id;
			else
				PPError();
		if(Data.CrdID.ac)
			if(AccObj.SearchBase(Data.CrdID.ac, &temp_acc_id, 0) > 0)
				Data.CrdID.ac = temp_acc_id;
			else
				PPError();
	}
	else
		MEMSZERO(Data);
	if(Data.Flags & PPAF_OUTBAL && Data.Flags & PPAF_OUTBAL_TRANSFER)
		if(Data.Amount >= 0)
			Data.SwapDbtCrd();
		else
			Data.Amount = -Data.Amount;
	setCtrlData(CTL_ATURN_DATE, &Data.Date);
	setCtrlData(CTL_ATURN_DOC,  Data.BillCode);
	rec.AcctId      = Data.DbtID;
	rec.AccSheetID  = Data.DbtSheet;
	if(Data.Flags & PPAF_REGISTER)
		rec.AccSelParam = ACY_SEL_REGISTER;
	else if(Data.Flags & PPAF_OUTBAL)
		rec.AccSelParam = ACY_SEL_OBAL;
	else
		rec.AccSelParam = ACY_SEL_BAL;
	setGroupData(GRP_DBT, &rec);
	rec.AcctId      = Data.CrdID;
	rec.AccSheetID  = Data.CrdSheet;
	if(Data.Flags & PPAF_REGISTER)
		rec.AccSelParam = ACY_SEL_REGISTER;
	else if(Data.Flags & PPAF_OUTBAL)
		rec.AccSelParam = ACY_SEL_OBAL;
	else
		rec.AccSelParam = ACY_SEL_BAL;
	setGroupData(GRP_CRD, &rec);
	ca_rec.Amount = Data.Amount;
	ca_rec.CurID  = Data.CurID;
	ca_rec.CRate  = Data.CRate;
	ca_rec.CRateDate = Data.Date;
	setGroupData(GRP_CURAMT, &ca_rec);
	setupCurrencyCombo();
	disableCtrl(CTL_ATURN_DACC, (int)(templFlags & ATTF_DACCFIX));
	disableCtrl(CTL_ATURN_DART, (int)(templFlags & ATTF_DARTFIX));
	disableCtrl(CTL_ATURN_CACC, (int)(templFlags & ATTF_CACCFIX));
	disableCtrl(CTL_ATURN_CART, (int)(templFlags & ATTF_CARTFIX));
	disableCtrl(CTL_ATURN_BASEAMT, 1);
	if(Data.Flags & PPAF_OUTBAL) {
		ushort v = 0;
		if(Data.Flags & PPAF_OUTBAL_WITHDRAWAL)
			v = 1;
		else if(Data.Flags & PPAF_OUTBAL_TRANSFER)
			v = 2;
		setCtrlData(CTL_ATURN_OP, &v);
		disableCtrls(v != 2, CTL_ATURN_CACC, CTLSEL_ATURN_CACCNAME, CTL_ATURN_CART, CTLSEL_ATURN_CARTNAME, 0);
	}
	//CATCH
	//	ok = PPErrorZ();
	//ENDCATCH
	return ok;
}

int AccTurnDialog::getDTS(PPAccTurn * pData)
{
	int    ok = 1;
	uint   sel = 0;
	AcctCtrlGroup::Rec rec;
	CurAmtCtrlGroup::Rec ca_rec;
	PPID   dbt_acc_id = 0, crd_acc_id = 0;
	if(Data.Flags & PPAF_OUTBAL) {
		const ushort v = getCtrlUInt16(CTL_ATURN_OP);
		SETFLAG(Data.Flags, PPAF_OUTBAL_WITHDRAWAL, v == 1);
		SETFLAG(Data.Flags, PPAF_OUTBAL_TRANSFER,   v == 2);
	}
	THROW(getGroupData(GRP_DBT, &rec));
	Data.DbtID    = rec.AcctId;
	Data.DbtSheet = rec.AccSheetID;
	THROW(getGroupData(GRP_CRD, &rec));
	Data.CrdID    = rec.AcctId;
	Data.CrdSheet = rec.AccSheetID;
	THROW(getGroupData(GRP_CURAMT, &ca_rec));
	Data.Amount   = ca_rec.Amount;
	Data.CurID    = ca_rec.CurID;
	Data.CRate    = ca_rec.CRate;
	selectCtrl(CTL_ATURN_AMOUNT);
	getCtrlData(sel = CTL_ATURN_DATE, &Data.Date);
	THROW_SL(checkdate(Data.Date));
	sel = 0;
	getCtrlData(CTL_ATURN_DOC,  Data.BillCode);
	THROW(AccObj.SearchCur(Data.DbtID.ac, Data.CurID, &dbt_acc_id, 0) > 0);
	Data.DbtID.ac = dbt_acc_id;
	if(Data.Flags & PPAF_REGISTER || (Data.Flags & PPAF_OUTBAL && !(Data.Flags & PPAF_OUTBAL_TRANSFER))) {
		Data.CrdID.ac = 0;
		Data.CrdID.ar = 0;
		Data.CrdSheet = 0;
		if(Data.Flags & PPAF_OUTBAL_WITHDRAWAL)
			Data.Amount = -Data.Amount;
	}
	else {
		THROW(AccObj.SearchCur(Data.CrdID.ac, Data.CurID, &crd_acc_id, 0) > 0);
		Data.CrdID.ac = crd_acc_id;
		if(Data.Flags & PPAF_OUTBAL_TRANSFER)
			if(Data.Amount >= 0)
				Data.SwapDbtCrd();
			else
				Data.Amount = -Data.Amount;
	}
	if(P_Pack) {
		if(P_Pack->Turns.getCount())
			memcpy(&P_Pack->Turns.at(0), &Data, sizeof(Data));
		else
			THROW_SL(P_Pack->Turns.insert(&Data));
		P_Pack->Rec.Dt = Data.Date;
		memcpy(P_Pack->Rec.Code, Data.BillCode, sizeof(P_Pack->Rec.Code));
		getCtrlData(CTLSEL_ATURN_LOCATION, &P_Pack->Rec.LocID);
		P_Pack->Rec.Amount = BR2(Data.Amount);
		P_Pack->Rec.CurID  = Data.CurID;
		P_Pack->Rec.CRate  = Data.CRate;
		if(Data.CurID)
			P_Pack->Amounts.Put(PPAMT_CRATE, Data.CurID, Data.CRate, 0, 1);
		getCtrlData(CTL_ATURN_MEMO, P_Pack->Rec.Memo);
	}
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

void AccTurnDialog::setupCurrencyCombo()
{
	PPIDArray cur_list;
	AcctCtrlGroup::Rec dbt_rec, crd_rec;
	getGroupData(GRP_DBT, &dbt_rec);
	getGroupData(GRP_CRD, &crd_rec);
	AccObj.GetIntersectCurList(dbt_rec.AcctId.ac, crd_rec.AcctId.ac, &cur_list);
	TView::messageCommand(this, cmCurAmtGrpSetupCurrencyCombo, &cur_list);
}

IMPL_HANDLE_EVENT(AccTurnDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmPPAccSelected))
		setupCurrencyCombo();
	else if(event.isCmd(cmBillTaxes)) {
		if(P_Pack)
			EditBillTaxes(&P_Pack->Amounts, getCtrlReal(CTL_ATURN_AMOUNT));
	}
	else if(event.isClusterClk(CTL_ATURN_OP)) {
		const ushort v = getCtrlUInt16(CTL_ATURN_OP);
		disableCtrls(v != 2, CTL_ATURN_CACC, CTLSEL_ATURN_CACCNAME, CTL_ATURN_CART, CTLSEL_ATURN_CARTNAME, 0);
	}
	else if(event.isKeyDown(kbF2)) {
		if(isCurrCtlID(CTL_ATURN_DOC))
			if(P_Pack)
				P_BObj->UpdateOpCounter(P_Pack);
	}
	else
		return;
	clearEvent(event);
}
//
//
//
int SLAPI EditRentCondition(PPRentCondition * pRc)
{
	class RentConditionDlg : public TDialog {
	public:
		RentConditionDlg() : TDialog(DLG_RENT)
		{
			SetupCalPeriod(CTLCAL_RENT_FROMTO, CTL_RENT_FROMTO);
		}
		int setDTS(const PPRentCondition * pRentCond)
		{
			RVALUEPTR(Data, pRentCond);
			SetPeriodInput(this, CTL_RENT_FROMTO, &Data.Period);
			SetupStringCombo(this, CTLSEL_RENT_PERIOD, PPTXT_CYCLELIST, Data.Cycle);
			ushort v = BIN(Data.Flags & RENTF_PERCENT);
			setCtrlData(CTL_RENT_ISPERCENT, &v);
			setCtrlData(CTL_RENT_PERCENT, &Data.Percent);
			setCtrlData(CTL_RENT_SUMPRD,  &Data.PartAmount);
			disableCtrl(CTL_RENT_PERCENT, (Data.Flags & RENTF_PERCENT) ? 0 : 1);
			disableCtrl(CTL_RENT_SUMPRD,  (Data.Flags & RENTF_PERCENT) ? 1 : 0);
			setCtrlData(CTL_RENT_ACMDATE, &Data.ChargeDayOffs);
			return 1;
		}
		int getDTS(PPRentCondition * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			int16  c = 0;
			ushort v = getCtrlUInt16(CTL_RENT_ISPERCENT);
			THROW(GetPeriodInput(this, sel = CTL_RENT_FROMTO, &Data.Period));
			SETFLAG(Data.Flags, RENTF_PERCENT, v);
			getCtrlData(sel = CTLSEL_RENT_PERIOD, &c);
			THROW_PP(c > 0, PPERR_NOCYCLES);
			Data.Cycle = c;
			getCtrlData(CTL_RENT_PERCENT, &Data.Percent);
			getCtrlData(CTL_RENT_SUMPRD,  &Data.PartAmount);
			getCtrlData(sel = CTL_RENT_ACMDATE, &Data.ChargeDayOffs);
			THROW_PP(Data.ChargeDayOffs >= -30 && Data.ChargeDayOffs <= 30, PPERR_USERINPUT);
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_RENT_ISPERCENT)) {
				ushort v = getCtrlUInt16(CTL_RENT_ISPERCENT);
				disableCtrl(CTL_RENT_PERCENT, v == 0);
				disableCtrl(CTL_RENT_SUMPRD,  v == 1);
				clearEvent(event);
			}
		}
		PPRentCondition Data;
	};
	DIALOG_PROC_BODY(RentConditionDlg, pRc);
}
//
//
//
class BillExtDialog : public TDialog {
public:
	BillExtDialog(uint dlgID, ObjTagList * pTagList) : TDialog(dlgID/*DLG_BILLEXT*/), IsTagList(0)
	{
		if(pTagList) {
			TagL = *pTagList;
			TagL.ObjType = PPOBJ_BILL;
			IsTagList = 1;
		}
		enableCommand(cmTags, IsTagList);
	}
	const ObjTagList * GetTagList() const { return IsTagList ? &TagL : 0; }
	int    SetInlineTags()
	{
		int    ok = 1;
		if(IsTagList) {
			PPObjTag tag_obj;
			PPObjectTag tag_rec;
			SString temp_buf;
			if(getCtrlView(CTL_BILLEXT_OUTERCODE) && tag_obj.Fetch(PPTAG_BILL_OUTERCODE, &tag_rec) > 0 && tag_rec.TagDataType == OTTYP_STRING) {
				const ObjTagItem * p_tag = TagL.GetItem(PPTAG_BILL_OUTERCODE);
				if(p_tag && p_tag->GetStr(temp_buf))
					setCtrlString(CTL_BILLEXT_OUTERCODE, temp_buf);
			}
			if(getCtrlView(CTL_BILLEXT_OUTERDATE) && tag_obj.Fetch(PPTAG_BILL_OUTERDATE, &tag_rec) > 0 && tag_rec.TagDataType == OTTYP_DATE) {
				const ObjTagItem * p_tag = TagL.GetItem(PPTAG_BILL_OUTERDATE);
				LDATE  outerdate;
				if(p_tag && p_tag->GetDate(&outerdate))
					setCtrlData(CTL_BILLEXT_OUTERDATE, &outerdate);
			}
		}
		return ok;
	}
	int    GetInlineTags()
	{
		int    ok = 1;
		if(IsTagList) {
			PPObjTag tag_obj;
			PPObjectTag tag_rec;
			SString temp_buf;
			if(getCtrlView(CTL_BILLEXT_OUTERCODE) && tag_obj.Fetch(PPTAG_BILL_OUTERCODE, &tag_rec) > 0 && tag_rec.TagDataType == OTTYP_STRING) {
				getCtrlString(CTL_BILLEXT_OUTERCODE, temp_buf);
				if(temp_buf.NotEmptyS())
					TagL.PutItemStr(PPTAG_BILL_OUTERCODE, temp_buf);
				else
					TagL.PutItem(PPTAG_BILL_OUTERCODE, 0);
			}
			if(getCtrlView(CTL_BILLEXT_OUTERDATE) && tag_obj.Fetch(PPTAG_BILL_OUTERDATE, &tag_rec) > 0 && tag_rec.TagDataType == OTTYP_DATE) {
				LDATE  outerdate = ZERODATE;
				getCtrlData(CTL_BILLEXT_OUTERDATE, &outerdate);
				if(outerdate) {
					ObjTagItem tag;
					tag.SetDate(PPTAG_BILL_OUTERDATE, outerdate);
					TagL.PutItem(PPTAG_BILL_OUTERDATE, &tag);
				}
				else
					TagL.PutItem(PPTAG_BILL_OUTERDATE, 0);
			}
		}
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmTags) && IsTagList) {
			GetInlineTags();
			EditObjTagValList(&TagL, 0);
			SetInlineTags();
			clearEvent(event);
		}
	}
	int    IsTagList;
	ObjTagList TagL;
};

int SLAPI BillExtraDialog(PPBillExt * pData, ObjTagList * pTagList, int asFilt)
{
	int    ok = -1;
	uint   dlg_id = 0;
	PPObjSCard sc_obj;
	SString temp_buf;
	char   scard_no[48];
	SCardTbl::Rec scard_rec;
	if(asFilt == 0)
		dlg_id = DLG_BILLEXT;
	else if(asFilt == 1)
		dlg_id = DLG_BILLEXTFLT;
	else if(asFilt == 2)
		dlg_id = DLG_BILLEXTFLT2;
	const PPID agent_acs_id = GetAgentAccSheet();
	const PPID payer_acs_id = GetSellAccSheet();
	BillExtDialog * dlg = new BillExtDialog(dlg_id, pTagList);
	if(CheckDialogPtrErr(&dlg)) {
		ushort v;
		SetupArCombo(dlg, CTLSEL_BILLEXT_PAYER, pData->PayerID, OLW_CANINSERT|OLW_LOADDEFONOPEN, payer_acs_id, sacfDisableIfZeroSheet|sacfNonGeneric);
		SetupArCombo(dlg, CTLSEL_BILLEXT_AGENT, pData->AgentID, OLW_CANINSERT|OLW_LOADDEFONOPEN, agent_acs_id, sacfDisableIfZeroSheet|sacfNonGeneric);
		if(asFilt == 0) {
			SetupPPObjCombo(dlg, CTLSEL_BILLEXT_EXTPQUOT, PPOBJ_QUOTKIND, pData->ExtPriceQuotKindID, 0);
			dlg->SetupCalDate(CTLCAL_BILLEXT_INVCDATE, CTL_BILLEXT_INVCDATE);
			dlg->setCtrlData(CTL_BILLEXT_INVCCODE, pData->InvoiceCode);
			dlg->setCtrlData(CTL_BILLEXT_INVCDATE, &pData->InvoiceDate);
			dlg->SetupCalDate(CTLCAL_BILLEXT_PAYMBDATE, CTL_BILLEXT_PAYMBDATE);
			dlg->setCtrlData(CTL_BILLEXT_PAYMBCODE, pData->PaymBillCode);
			dlg->setCtrlData(CTL_BILLEXT_PAYMBDATE, &pData->PaymBillDate);
			dlg->SetupCalDate(CTLCAL_BILLEXT_OUTERDATE, CTL_BILLEXT_OUTERDATE);
			dlg->SetInlineTags();
			dlg->setCtrlUInt16(CTL_BILLEXT_SHIPPED, BIN(pData->IsShipped));
			if(sc_obj.Search(pData->SCardID, &scard_rec) > 0)
				STRNSCPY(scard_no, scard_rec.Code);
			else
				scard_no[0] = 0;
			dlg->setCtrlData(CTL_BILLEXT_SCARDN, scard_no);
			dlg->disableCtrl(CTL_BILLEXT_SCARDN, !sc_obj.CheckRights(SCRDRT_BINDING));
		}
		if(asFilt == 2) {
			PPAccessRestriction accsr;
			const int own_bill_restr = ObjRts.GetAccessRestriction(accsr).GetOwnBillRestrict();
			{
				v = (pData->Ft_STax > 0) ? 1 : ((pData->Ft_STax < 0) ? 2 : 0);
				dlg->setCtrlData(CTL_BILLEXTFLT_STAXTGGL, &v);
			}
			{
				v = (pData->Ft_Declined > 0) ? 1 : ((pData->Ft_Declined < 0) ? 2 : 0);
				dlg->setCtrlData(CTL_BILLEXTFLT_DCLTGGL, &v);
			}
			// @v9.1.6 {
            {
                dlg->AddClusterAssocDef(CTL_BILLEXTFLT_RECADV, 0, PPEDI_RECADV_STATUS_UNDEF);
                dlg->AddClusterAssoc(CTL_BILLEXTFLT_RECADV, 1, PPEDI_RECADV_STATUS_ACCEPT);
                dlg->AddClusterAssoc(CTL_BILLEXTFLT_RECADV, 2, PPEDI_RECADV_STATUS_PARTACCEPT);
                dlg->AddClusterAssoc(CTL_BILLEXTFLT_RECADV, 3, PPEDI_RECADV_STATUS_REJECT);
                dlg->AddClusterAssoc(CTL_BILLEXTFLT_RECADV, 4, -1);
                dlg->SetClusterData(CTL_BILLEXTFLT_RECADV, pData->EdiRecadvStatus);
            }
            {
                dlg->AddClusterAssocDef(CTL_BILLEXTFLT_RECADVCFM, 0, PPEDI_RECADVCONF_STATUS_UNDEF);
                dlg->AddClusterAssoc(CTL_BILLEXTFLT_RECADVCFM, 1, PPEDI_RECADVCONF_STATUS_ACCEPT);
                dlg->AddClusterAssoc(CTL_BILLEXTFLT_RECADVCFM, 2, PPEDI_RECADVCONF_STATUS_REJECT);
                dlg->AddClusterAssoc(CTL_BILLEXTFLT_RECADVCFM, 3, -1);
                dlg->SetClusterData(CTL_BILLEXTFLT_RECADVCFM, pData->EdiRecadvConfStatus);
            }
			// } @v9.1.6
			dlg->disableCtrl(CTLSEL_BILLEXT_CREATOR, own_bill_restr);
			SetupPPObjCombo(dlg, CTLSEL_BILLEXT_CREATOR, PPOBJ_USR, pData->CreatorID, OLW_CANSELUPLEVEL);
			dlg->SetupCalPeriod(CTLCAL_BILLEXT_DUEPERIOD, CTL_BILLEXT_DUEPERIOD);
			SetPeriodInput(dlg, CTL_BILLEXT_DUEPERIOD, &pData->DuePeriod);
		}
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			valid_data = 1;
			pData->PayerID = payer_acs_id ? dlg->getCtrlLong(CTLSEL_BILLEXT_PAYER) : 0;
			pData->AgentID = agent_acs_id ? dlg->getCtrlLong(CTLSEL_BILLEXT_AGENT) : 0;
			if(asFilt == 0) {
				dlg->getCtrlData(CTLSEL_BILLEXT_EXTPQUOT, &pData->ExtPriceQuotKindID);
				dlg->getCtrlData(CTL_BILLEXT_INVCCODE, pData->InvoiceCode);
				strip(pData->InvoiceCode);
				dlg->getCtrlData(CTL_BILLEXT_INVCDATE, &pData->InvoiceDate);
				dlg->getCtrlData(CTL_BILLEXT_PAYMBCODE, pData->PaymBillCode);
				strip(pData->PaymBillCode);
				dlg->getCtrlData(CTL_BILLEXT_PAYMBDATE, &pData->PaymBillDate);
				if(!checkdate(pData->InvoiceDate, 1))
					valid_data = PPErrorByDialog(dlg, CTL_BILLEXT_INVCDATE, PPERR_SLIB);
				else if(!checkdate(pData->PaymBillDate, 1))
					valid_data = PPErrorByDialog(dlg, CTL_BILLEXT_PAYMBDATE, PPERR_SLIB);
				else {
					pData->IsShipped = BIN(dlg->getCtrlUInt16(CTL_BILLEXT_SHIPPED) & 0x01);
					if(sc_obj.CheckRights(SCRDRT_BINDING)) {
						dlg->getCtrlData(CTL_BILLEXT_SCARDN, scard_no);
						if(*strip(scard_no)) {
							if(sc_obj.SearchCode(0, scard_no, &scard_rec) > 0)
								pData->SCardID = scard_rec.ID;
							else {
								PPError(PPERR_SCARDNOTFOUND, scard_no);
								dlg->selectCtrl(CTL_BILLEXT_SCARDN);
								valid_data = 0;
							}
						}
						else
							pData->SCardID = 0;
					}
				}
			}
			if(asFilt == 2) {
				{
					v = dlg->getCtrlUInt16(CTL_BILLEXTFLT_STAXTGGL);
					pData->Ft_STax = (v == 1) ? 1 : ((v == 2) ? -1 : 0);
				}
				{
					v = dlg->getCtrlUInt16(CTL_BILLEXTFLT_DCLTGGL);
					pData->Ft_Declined = (v == 1) ? 1 : ((v == 2) ? -1 : 0);
				}
				// @v9.1.6 {
				pData->EdiRecadvStatus = (int16)dlg->GetClusterData(CTL_BILLEXTFLT_RECADV);
				pData->EdiRecadvConfStatus = (int16)dlg->GetClusterData(CTL_BILLEXTFLT_RECADVCFM);
				// } @v9.1.6
				dlg->getCtrlData(CTLSEL_BILLEXT_CREATOR, &pData->CreatorID);
				if(!GetPeriodInput(dlg, CTL_BILLEXT_DUEPERIOD, &pData->DuePeriod)) {
					PPErrorByDialog(dlg, CTL_BILLEXT_DUEPERIOD);
					valid_data = 0;
				}
			}
			if(pTagList) {
				dlg->GetInlineTags();
				const ObjTagList * p_ret_list = dlg->GetTagList();
				RVALUEPTR(*pTagList, p_ret_list);
			}
			if(valid_data)
				ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SLAPI BillPrelude(const PPIDArray * pOpList, uint opklFlags, PPID linkOpID, PPID * pOpID, PPID * pLocID)
{
	class BillPreludeDialog : public TDialog {
	public:
		BillPreludeDialog() : TDialog(DLG_BILLPRELUDE)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmLBDblClk)) {
				TView::messageCommand(this, cmOK);
				clearEvent(event);
			}
		}
	};
	int    ok = -1;
	PPID   op_id = *pOpID;
	PPID   loc_id = *pLocID;
	TDialog * dlg = new BillPreludeDialog();
	if(CheckDialogPtrErr(&dlg)) {
		PPObjOprKind op_obj;
		SmartListBox * p_listbox = (SmartListBox*)dlg->getCtrlView(CTL_BILLPRELUDE_OPLIST);
		p_listbox->setDef(new StrAssocListBoxDef(op_obj.MakeOprKindList(linkOpID, pOpList, opklFlags), lbtDblClkNotify|lbtFocNotify|lbtDisposeData));
		if(op_id)
			p_listbox->search(&op_id, 0, srchFirst | lbSrchByID);
		SetupPPObjCombo(dlg, CTLSEL_BILLPRELUDE_LOC, PPOBJ_LOCATION, loc_id, 0);
		if(loc_id && opklFlags & OPKLF_FIXEDLOC)
			dlg->disableCtrl(CTLSEL_BILLPRELUDE_LOC, 1);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			dlg->getCtrlData(CTL_BILLPRELUDE_OPLIST, &op_id);
			dlg->getCtrlData(CTLSEL_BILLPRELUDE_LOC, &loc_id);
			if(!op_id)
				PPErrorByDialog(dlg, CTL_BILLPRELUDE_OPLIST, PPERR_OPRKINDNEEDED);
			else if(!loc_id)
				PPErrorByDialog(dlg, CTL_BILLPRELUDE_LOC, PPERR_LOCNEEDED);
			else {
				ok = valid_data = 1;
				*pOpID = op_id;
				*pLocID = loc_id;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

class BillDialog : public PPListDialog {
public:
	friend int SLAPI EditGoodsBill(PPBillPacket * pPack, long egbFlags);

	BillDialog(uint dlgID, PPBillPacket *, int isEdit);
	int    setDTS(PPBillPacket *);
	//
	// Если onCancel != 0, то функция getDTS вызывает checkCreditOverflow //
	//
	int    getDTS(int onCancel);
	int    isModified()
	{
		if(!(Flags & fModified))
			getDTS(1);
		return BIN(Flags & fModified);
	}
	uint   PrnForm;
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int sendItem(long pos, long id);
	double getCurrentDebt(PPID debtDimID) const;
	int    checkCreditOverflow(double amt);
	int    editItems();
	int    editPaymOrder(int forceUpdateRcvr);
	int    EditFreight();
	int    calcAmounts(double * _amt);
	void   setupDebtText();
	void   setupByCntragnt();
	void   ReplyCntragntSelection(int force);
	void   setDiscount(double, int inPersent);
	int    getDiscount(double * d, int * inPersent, int * rmvExcise);
	void   SetupDiscountCtrls();
	int    setCurGroupData();
	int    getCurGroupData();
	int    setAdvanceRepData(PPAdvanceRep * pAR);
	int    getAdvanceRepData(PPAdvanceRep * pAR);
	int    setupAdvanceRepTotal(const PPAdvanceRep *);
	int    calcAdvanceRepRest();
	int    calcDate(uint ctlID); // @<<BillDialog::handleEvent
	void   setupParentOfContragent();
	void   setupHiddenButton(long opflag, uint cm, uint ctlId);
	int    editLinkFiles();
	int    showLinkFilesList();
	int    setupDebt();
	void   SetupAgreementButton();
	int    EditAgreement();
	void   SetupPaymDateCtrls();
	enum {
		fPctDis            = 0x0001, // Признак того, что скидка указана в процентах
		fExtMainCurAmount  = 0x0002, // Признак наличия в диалоге полей валюты
			// и валютного курса (CTL_BILL_CUR, CTLSEL_BILL_CUR, CTL_BILL_CRATE, CTL_BILL_BASEAMT)
		fEditMode          = 0x0004,
		fHasAmtIDList      = 0x0008,
		fModified          = 0x0010,
		fSetupObj2ByCliAgt = 0x0020,
		fCheckAgreement    = 0x0040, // Включается если в таблице статей установлено использование соглашений
		fCheckCreditLim    = 0x0080, // Вид операции, возможно, требует проверки кредитного лимита
		fCheckRetLim       = 0x0100  // Вид операции, возможно, требует проверки лимита возвратов
	};
	long   Flags;
	int    PaymTerm;       // Срок оплаты в днях (по соглашению). Инициализируется в setDTS и в ReplyCntragntSelection
	long   PayDateBase;    // 
	PPObjGoods GObj;
	PPObjArticle ArObj;
	PPObjBill    * P_BObj;
	PPBillPacket * P_Pack;
	BillTbl::Rec   Pattern;
	PPIDArray      ExtAmtIDList;
	PPClientAgreement CliAgt;
	double CurrDebt;
	RAssocArray CDebtList; // Текущий долг с разбивкой по долговым размерностям.
	TRect  DefaultRect;
	enum {
		dummyFirst = 1,
		brushIllPaymDate,  // Кисть для индикации неправильной даты оплаты (не согласующейся с соглашением)
		brushSynced        // Кисть для идникации статуса синхронизации (в поле номера документа)
	};
	SPaintToolBox Ptb;
};

static uint SLAPI GetBillDialogID(PPBillPacket * pack, uint * pPrnForm)
{
	PPOprKind op_rec;
	*pPrnForm = 0;
	switch(pack->OpTypeID) {
		case PPOPT_ACCTURN:
			if(pack->Rec.Flags & BILLF_ADVANCEREP)
				return DLG_ADVANCEREP;
			else if(GetOpSubType(pack->Rec.OpID) == OPSUBT_WARRANT)
				return DLG_WARRANT;
			else if(GetOpSubType(pack->Rec.OpID) == OPSUBT_DEBTINVENT)
				return DLG_BILL_DEBTINVENT;
			else if(GetOpSubType(pack->Rec.OpID) == OPSUBT_ACCWROFF)
				return DLG_ATWROFF;
			else
				return DLG_ATEXT;
		case PPOPT_PAYMENT: return DLG_PAYMENT;
		case PPOPT_CHARGE : return /*DLG_CHARGE*/DLG_PAYMENT;
		case PPOPT_DRAFTRECEIPT:
		case PPOPT_DRAFTTRANSIT:
		case PPOPT_GOODSRECEIPT:
			return (IsIntrOp(pack->Rec.OpID) == INTRRCPT) ? DLG_INTRRCPT : DLG_RCPTBILL;
		case PPOPT_DRAFTEXPEND:
		case PPOPT_GOODSEXPEND:
			if(IsIntrExpndOp(pack->Rec.OpID))
				return DLG_INTREXPD;
			else {
				GetOpData(pack->Rec.OpID, &op_rec);
				return op_rec.AccSheetID ? DLG_SELLBILL : DLG_RETAILBILL;
			}
		case PPOPT_GOODSRETURN:
		case PPOPT_CORRECTION:
			if(GetOpData(pack->Rec.OpID, &op_rec) > 0 && op_rec.LinkOpID && GetOpData(op_rec.LinkOpID, &op_rec))
				if(op_rec.OpTypeID == PPOPT_GOODSRECEIPT)
					return DLG_RCPTRETBILL;
				else
					return op_rec.AccSheetID ? DLG_SELLRETBILL : DLG_RETAILBILL;
			break;
		case PPOPT_GOODSREVAL: return DLG_REVALBILL;
		case PPOPT_GOODSORDER: return DLG_ORDERBILL;
		case PPOPT_GOODSMODIF: return DLG_MODIFBILL;
		case PPOPT_GOODSACK:   return DLG_ACKBILL;
		case PPOPT_POOL:       return DLG_BILLPOOL;
	}
	return (uint)PPSetError(PPERR_INVOPRKIND);
}
//
// Параметр options принимает значение:
//   0 - при добавлении.
//   1 - при редактировании
//   2 - редактирование с форсированным признаком modified.
//       В случае редактирования некоторые поля блокируются.
//   3 - не выводить сообщение о том что документ был модифицирован
//
int SLAPI EditGoodsBill(PPBillPacket * pPack, long egbFlags)
{
	MemLeakTracer mlt;
	int    ok = -1, r;
	const  PPRights & r_rt = ObjRts;
	uint   prn_form = 0;
	BillDialog * dlg = 0;
	uint   dlg_id = 0;
	THROW(r_rt.CheckOpID(pPack->Rec.OpID, PPR_READ));
	if(CheckOpFlags(pPack->Rec.OpID, OPKF_CURTRANSIT)) {
		THROW(r = EditCurTransitBill(pPack));
	}
	else {
		THROW(dlg_id = GetBillDialogID(pPack, &prn_form));
		THROW(CheckDialogPtr(&(dlg = new BillDialog(dlg_id, pPack, (egbFlags & PPObjBill::efEdit)/*options >= 1*/))));
		dlg->PrnForm = prn_form;
		THROW(dlg->setDTS(pPack));
		if(egbFlags & PPObjBill::efEdit && egbFlags & PPObjBill::efForceModify/*options == 2*/)
			dlg->Flags |= BillDialog::fModified;
		if(egbFlags & PPObjBill::efEdit/*options >= 1*/) {
			if(!BillObj->CheckRights(PPR_MOD) || !r_rt.CheckBillDate(pPack->Rec.Dt) || !r_rt.CheckOpID(pPack->Rec.OpID, PPR_MOD)) {
				dlg->enableCommand(cmOK, 0);
				//options = 3;
				egbFlags |= PPObjBill::efNoUpdNotif;
			}
		}
		if(egbFlags & PPObjBill::efCascade)
			dlg->ToCascade();
		while((r = ExecView(dlg)) == cmOK || (/*options != 3*/!(egbFlags & PPObjBill::efNoUpdNotif) && r == cmCancel && dlg->isModified() && !CONFIRM(PPCFM_WARNCANCEL))) {
			if(r == cmOK) {
				if(!dlg->getDTS(0))
					PPError();
				else
					break;
			}
		}
		if(r == cmCancel && pPack && pPack->Rec.ID && pPack->LnkFiles.getCount()) {
			PPLinkFilesArray flnks_ary;
			flnks_ary.ReadFromProp(pPack->Rec.ID);
			pPack->LnkFiles.RemoveByAry(&flnks_ary);
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

BillDialog::BillDialog(uint dlgID, PPBillPacket * pPack, int isEdit) : PPListDialog(dlgID, CTL_BILL_LNKFILELIST), P_BObj(BillObj),
	P_Pack(pPack), CurrDebt(0.0), Flags(0), PaymTerm(-1), PayDateBase(0)
{
	SETFLAG(Flags, fEditMode, isEdit);
	int    is_cash = BIN(P_Pack->Rec.Flags & BILLF_CASH);
	Ptb.SetBrush(brushIllPaymDate, SPaintObj::bsSolid, GetColorRef(SClrCoral), 0);
	Ptb.SetBrush(brushSynced, SPaintObj::bsSolid, GetColorRef(SClrLightsteelblue), 0);
	if(getCtrlView(CTLSEL_BILL_CUR)) {
		Flags |= fExtMainCurAmount;
		CurAmtCtrlGroup * p_ca_grp = new CurAmtCtrlGroup(CTL_BILL_AMOUNT,
			CTLSEL_BILL_CUR, CTL_BILL_CRATE, CTL_BILL_BASEAMT, CTL_BILL_DATE, 0, &P_Pack->Amounts);
		addGroup(GRP_CURAMT, p_ca_grp);
	}
	PPSetupCtrlMenu(this, CTL_BILL_DOC, CTLMNU_BILL_DOC, CTRLMENU_BILLNUMBER);
	PPSetupCtrlMenu(this, CTL_BILL_PAYDATE, CTLMNU_BILL_PAYDATE, CTRLMENU_BILLPAYDATE);
	{
		TInputLine * p_memo_input = (TInputLine *)getCtrlView(CTL_BILL_MEMO);
		if(p_memo_input) {
			PPSetupCtrlMenu(this, CTL_BILL_MEMO, CTLMNU_BILL_MEMO, CTRLMENU_BILLMEMO);
			p_memo_input->setFormat(MKSFMT(sizeof(pPack->Rec.Memo), 0));
			p_memo_input->setType(MKSTYPE(S_ZSTRING, sizeof(pPack->Rec.Memo)));
			p_memo_input->setMaxLen(sizeof(pPack->Rec.Memo));
		}
	}
	SetupCalDate(CTLCAL_BILL_DATE,    CTL_BILL_DATE);
	SetupCalDate(CTLCAL_BILL_DUEDATE, CTL_BILL_DUEDATE);
	SetupCalDate(CTLCAL_BILL_PAYDATE, CTL_BILL_PAYDATE);
	SetupCalPeriod(CTLCAL_BILL_PERIOD, CTL_BILL_PERIOD);
	disableCtrls(is_cash || (Flags & fEditMode && !P_BObj->CheckRights(BILLRT_MODDATE)), CTL_BILL_DATE, CTLCAL_BILL_DATE, 0);
	{
		const int do_disable_object = (Flags & fEditMode && !P_BObj->CheckRights(BILLOPRT_MODOBJ, 1));
		disableCtrl(CTLSEL_BILL_OBJECT, do_disable_object);
		SetupAgreementButton();
	}
	// @v9.4.3 disableCtrl(CTL_BILL_AMOUNT, BIN(P_Pack->Rec.Flags & BILLF_GOODS || P_Pack->OprType == PPOPT_POOL || P_Pack->IsDraft()));
	disableCtrl(CTL_BILL_AMOUNT, BIN(P_Pack->IsGoodsDetail() || P_Pack->OpTypeID == PPOPT_POOL)); // @v9.4.3
	disableCtrl(CTL_BILL_ADV_TOUT, 1);
	disableCtrl(CTL_BILL_DEBTSUM, 1);
	setupPosition();
	DefaultRect = getRect();
	showLinkFilesList();
	{
		PPOprKind  op_rec;
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		GetOpData(P_Pack->Rec.OpID, &op_rec);
		SETFLAG(Flags, fCheckAgreement, BIN(acs_obj.Fetch(op_rec.AccSheetID, &acs_rec) > 0 && (acs_rec.Flags & (ACSHF_USECLIAGT|ACSHF_USESUPPLAGT))));
	}
	setSmartListBoxOption(CTL_BILL_LNKFILELIST, lbtExtMenu);
}

void BillDialog::SetupAgreementButton()
{
	int    do_enable = 0;
	ArticleTbl::Rec ar_rec;
	const PPID ar_id = getCtrlLong(CTLSEL_BILL_OBJECT);
	if(ar_id && ArObj.Fetch(ar_id, &ar_rec) > 0) {
		int agt_kind = PPObjArticle::GetAgreementKind(&ar_rec);
		if(agt_kind == 1) {
			PPClientAgreement agt_cli;
			if(ArObj.GetClientAgreement(ar_id, &agt_cli, 0) > 0) {
				do_enable = 1;
			}
		}
		else if(agt_kind == 2) {
			PPSupplAgreement agt_suppl;
			if(ArObj.GetSupplAgreement(ar_id, &agt_suppl, 0) > 0) {
				do_enable = 2;
			}
		}
	}
	enableCommand(cmAgreement, do_enable);
}

int BillDialog::EditAgreement()
{
	ArticleTbl::Rec ar_rec;
	const PPID ar_id = getCtrlLong(CTLSEL_BILL_OBJECT);
	return (ar_id && ArObj.Fetch(ar_id, &ar_rec) > 0) ? ArObj.EditAgreement(ar_id) : -1;
}

void BillDialog::SetupDiscountCtrls()
{
	if(P_Pack->OpTypeID == PPOPT_GOODSRETURN) {
		disableCtrls(1, CTL_BILL_TTLDISCOUNT, CTL_BILL_DISCOUNT, 0);
	}
	else {
		int rt = P_BObj->CheckRights(BILLOPRT_TOTALDSCNT, 1);
		if(P_Pack->GetSyncStatus() > 0) {
			//
			// Если у пользователя нет прав на изменение синхронизированного документа,
			// то менять скидку на весь документ он не может - это приведет к изменению сумм по строкам.
			//
			if(oneof6(P_Pack->OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSREVAL, PPOPT_GOODSMODIF,
				PPOPT_GOODSRETURN, PPOPT_GOODSORDER)) {
				if(!P_BObj->CheckRights(BILLOPRT_MODTRANSM, 1))
					rt = 0;
			}
		}
		const ushort v = getCtrlUInt16(CTL_BILL_TTLDISCOUNT);
		disableCtrl(CTL_BILL_DISCOUNT, (!v || !rt));
		disableCtrl(CTL_BILL_TTLDISCOUNT, !rt);
	}
}

void BillDialog::setDiscount(double d, int inPercent)
{
	SString buf;
	buf.Cat(d, MKSFMTD(0, 2, NMBF_TRICOMMA|NMBF_NOZERO));
	if(inPercent)
		buf.CatChar('%');
	if(P_Pack->Rec.Flags & BILLF_RMVEXCISE)
		buf.CatChar('A');
	setCtrlString(CTL_BILL_DISCOUNT, buf.Strip());
}

int BillDialog::getDiscount(double * pDiscount, int * pInPercent, int * pRmvExcise)
{
	int    ok = 1, pctdis = 0, rmve = 0;
	double v = 0.0;
	char   buf[32];
	if(getCtrlData(CTL_BILL_DISCOUNT, buf)) {
		strip(buf);
		SString excise_symbols("aAаА"); // латинские 'a' и кириллические 'а'
		excise_symbols.Transf(CTRANSF_UTF8_TO_INNER); // Этот исходный модуль кодируется в UTF-8 (see header of this file above)
		char * p = strpbrk(buf, excise_symbols);
		if(p) {
			rmve = 1;
			strcpy(p, p + 1);
		}
		p = strpbrk(buf, "%/");
		if(p) {
			pctdis = 1;
			strcpy(p, p + 1);
		}
		p = buf + sstrlen(buf);
		if(pctdis)
			*p++ = '%';
		if(rmve)
			*p++ = 'A';
		*p = 0;
		strtodoub(buf, &v);
		setCtrlData(CTL_BILL_DISCOUNT, buf);
	}
	else
		ok = 0;
	ASSIGN_PTR(pDiscount, v);
	ASSIGN_PTR(pInPercent, pctdis);
	ASSIGN_PTR(pRmvExcise, rmve);
	return ok;
}

int BillDialog::editPaymOrder(int forceUpdateRcvr)
{
	int    ok = -1;
	PPBankingOrder order;
	BankingOrderDialog * dlg = 0;
	if(CheckOpFlags(P_Pack->Rec.OpID, OPKF_BANKING)) {
		if(P_Pack->P_PaymOrder)
			order = *P_Pack->P_PaymOrder;
		else
			MEMSZERO(order);
		SETIFZ(order.Dt, P_Pack->Rec.Dt);
		{
			SString temp_buf;
			getCtrlString(CTL_BILL_DOC, temp_buf);
			STRNSCPY(order.Code, temp_buf);
		}
		SETIFZ(order.PayerKindID, PPPRK_MAIN);
		if(!order.RcvrKindID) {
			PPOprKind  op_rec;
			PPObjAccSheet acs_obj;
			PPAccSheet acs_rec;
			GetOpData(P_Pack->Rec.OpID, &op_rec);
			if(acs_obj.IsAssoc(op_rec.AccSheetID, PPOBJ_PERSON, &acs_rec) > 0)
				order.RcvrKindID = acs_rec.ObjGroup;
			if(order.RcvrKindID == 0 && op_rec.LinkOpID) {
				GetOpData(op_rec.LinkOpID, &op_rec);
				if(acs_obj.IsAssoc(op_rec.AccSheetID, PPOBJ_PERSON, &acs_rec) > 0)
					order.RcvrKindID = acs_rec.ObjGroup;
			}
		}
		if(!order.PayerID)
			GetMainOrgID(&order.PayerID);
		if(!order.RcvrID || forceUpdateRcvr) {
			PPID   article_id = getCtrlLong(CTLSEL_BILL_OBJECT);
			order.RcvrID = ObjectToPerson(article_id);
			if(!order.RcvrID && P_Pack->Rec.LinkBillID) {
				BillTbl::Rec link_bill_rec;
				if(P_BObj->Search(P_Pack->Rec.LinkBillID, &link_bill_rec) > 0)
					order.RcvrID = ObjectToPerson(link_bill_rec.Object);
			}
		}
		SETIFZ(order.BnkPaymMethod, BNKPAYMMETHOD_DEFAULT);
		SETIFZ(order.BnkQueueing, BNKQUEUEING_DEFAULT);
		order.Amount = getCtrlReal(CTL_BILL_AMOUNT);
		THROW(CheckDialogPtr(&(dlg = new BankingOrderDialog)));
		dlg->setDTS(&order);
		while(ok <= 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&order)) {
				if(!P_Pack->P_PaymOrder)
					THROW_MEM(P_Pack->P_PaymOrder = new PPBankingOrder);
				*(P_Pack->P_PaymOrder) = order;
				P_Pack->Rec.Flags |= BILLF_BANKING;
				ok = 1;
			}
		SetCtrlBitmap(CTL_BILL_PAYMORD_IND, P_Pack->P_PaymOrder ? BM_GREEN : BM_RED);
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

IMPL_CMPFUNC(PPLinkFile, i1, i2) { return stricmp866(((PPLinkFile*)i1)->Path, ((PPLinkFile*)i2)->Path); }

SLAPI PPLinkFile::PPLinkFile()
{
	Init(0);
}

int SLAPI PPLinkFile::Init(const char * pPath)
{
	int    ok = 0;
	Id = 0;
	Flags = 0;
	Ext.Z();
	Path.Z();
	Description = 0;
	if(pPath && sstrlen(pPath)) {
		pathToUNC(pPath, Path);
		SPathStruc ps(Path);
		Ext = ps.Ext;
		ok = 1;
	}
	return ok;
}

size_t SLAPI PPLinkFile::Size() const
{
	return sizeof(Id) + sizeof(Flags) + sizeof(Ext.Len()) + Ext.Len() + 1 + sizeof(Path.Len()) +
		Path.Len() + 1 + sizeof(Description.Len()) + Description.Len() + 1;
}

int SLAPI PPLinkFile::CopyTo(void ** ppBuf)
{
	size_t ext_len  = Ext.Len()  + 1;
	size_t path_len = Path.Len() + 1;
	size_t descr_len = Description.Len() + 1;
	char * p = (char*)*ppBuf;

	Flags &= ~PPLNKFILE_ISNEW;
	memcpy(p, &Id, sizeof(Id));
	memcpy(p += sizeof(Id), &Flags, sizeof(Flags));

	memcpy(p += sizeof(Flags), &ext_len, sizeof(ext_len));
	memcpy(p += sizeof(ext_len), (const char*)Ext, ext_len);

	memcpy(p += ext_len, &path_len, sizeof(path_len));
	memcpy(p += sizeof(path_len), (const char*)Path, path_len);

	memcpy(p += path_len, &descr_len, sizeof(descr_len));
	memcpy(p += sizeof(descr_len), (const char*)Description, descr_len);
	return 1;
}

int FASTCALL PPLinkFile::CopyFrom(const void * pBuf)
{
	const char * p = (const char*)pBuf;
	size_t ext_len  = 0;
	size_t path_len = 0;
	size_t descr_len = 0;

	memcpy(&Id, p, sizeof(Id));
	memcpy(&Flags, p += sizeof(Id), sizeof(Flags));

	memcpy(&ext_len, p += sizeof(Flags), sizeof(ext_len));
	Ext.CopyFromN(p += sizeof(ext_len), ext_len);

	memcpy(&path_len, p += ext_len, sizeof(path_len));
	Path.CopyFromN(p += sizeof(path_len), path_len);

	memcpy(&descr_len, p += path_len, sizeof(descr_len));
	Description.CopyFromN(p += sizeof(descr_len), descr_len);
	return 1;
}

SLAPI PPLinkFilesArray::PPLinkFilesArray(const char * pStoreDir /*=0*/) : TSCollection<PPLinkFile> ()
{
	Init(pStoreDir);
}

PPLinkFilesArray & FASTCALL PPLinkFilesArray::operator = (const PPLinkFilesArray & s)
{
	freeAll();
	StoreDir = s.StoreDir;
	for(uint i = 0; i < s.getCount(); i++) {
		PPLinkFile * p_flink = new PPLinkFile;
		*p_flink = *s.at(i);
		insert(p_flink);
	}
	return *this;
}

int SLAPI PPLinkFilesArray::Init(const char * pStoreDir)
{
	SString temp_buf;
	StoreDir.Z();
	if(pStoreDir && pStoreDir[0])
		StoreDir = pStoreDir;
	else {
		DBS.GetDbPath(DBS.GetDbPathID(), StoreDir);
		StoreDir.SetLastSlash().Cat(PPLoadTextS(PPTXT_LNKFILESDIR, temp_buf)).SetLastSlash();
	}
	if(StoreDir.NotEmpty()) {
		pathToUNC(temp_buf = StoreDir, StoreDir);
		createDir(StoreDir);
	}
	return 1;
}

int SLAPI PPLinkFilesArray::CreateExcelFile(const char * pPath)
{
	int    ok = 1;
	ComExcelApp * p_app = new ComExcelApp;
	ComExcelWorkbook * p_wkbook = 0;
	THROW_MEM(p_app);
	THROW(p_app->Init() > 0);
	THROW(p_wkbook = p_app->AddWkbook());
	THROW(p_wkbook->_SaveAs(pPath) > 0);
	CATCHZOK
	if(p_wkbook) {
		p_wkbook->_Close();
		ZDELETE(p_wkbook);
	}
	ZDELETE(p_app);
	return ok;
}

int SLAPI PPLinkFilesArray::CreateWordFile(const char * pPath)
{
	int    ok = 1;
	enum {
		Documents = 1L,
		ActiveDocument,
		Quit,
		Add,
		Item,
		SaveAs,
		Close
	};
	ComDispInterface * p_appl = 0, * p_docs = 0, * p_doc = 0;
	THROW_MEM(p_appl = new ComDispInterface);
	THROW(p_appl->Init("Word.Application", 0));

	THROW(ASSIGN_ID_BY_NAME(p_appl, Documents));
	THROW(ASSIGN_ID_BY_NAME(p_appl, ActiveDocument));
	THROW(ASSIGN_ID_BY_NAME(p_appl, Quit));
	THROW_MEM(p_docs = new ComDispInterface);
	THROW(p_appl->GetProperty(Documents, p_docs) > 0);

	THROW(ASSIGN_ID_BY_NAME(p_docs, Add));
	THROW(ASSIGN_ID_BY_NAME(p_docs, Item));
	THROW(p_docs->CallMethod(Add) > 0);
	THROW_MEM(p_doc = new ComDispInterface);
	THROW(p_appl->GetProperty(ActiveDocument, p_doc));

	THROW(ASSIGN_ID_BY_NAME(p_doc, SaveAs));
	THROW(ASSIGN_ID_BY_NAME(p_doc, Close));
	THROW(p_doc->SetParam(pPath) > 0);
	THROW(p_doc->CallMethod(SaveAs) > 0);

	CATCHZOK
	if(p_doc) {
		p_doc->CallMethod(Close);
		ZDELETE(p_doc);
	}
	ZDELETE(p_docs);
	if(p_appl) {
		p_appl->CallMethod(Quit);
		ZDELETE(p_appl);
	}
    return ok;
}

int SLAPI PPLinkFilesArray::AddNewByExt(const char * pExt, const char * pDescr, uint * pPos)
{
	int    ok = 1;
	SString store_path;
	PPLinkFile * p_flink = new PPLinkFile;
	THROW_MEM(p_flink);
	p_flink->Id = GetNewId();
	p_flink->Ext.CopyFrom(pExt);
	p_flink->Flags |= PPLNKFILE_ISNEW;
	p_flink->Description.CopyFrom(pDescr);
	GetFilePath(p_flink->Id, p_flink->Ext, store_path);
	if(p_flink->Ext.Search("xls", 0, 1, 0) > 0) {
		THROW(CreateExcelFile(store_path) > 0);
	}
	else if(p_flink->Ext.Search("doc", 0, 1, 0) > 0) {
		THROW(CreateWordFile(store_path) > 0);
	}
	else if(p_flink->Ext.Search("txt", 0, 1, 0) > 0) {
		FILE * f = fopen(store_path, "w");
		THROW_PP_S(f != NULL, PPERR_CANTOPENFILE, store_path);
		fclose(f);
	}
	else
		ok = -1;
	if(ok > 0) {
		THROW_SL(insert(p_flink));
		ASSIGN_PTR(pPos, getCount() - 1);
	}
	CATCHZOK
	if(ok <= 0)
		ZDELETE(p_flink);
	return ok;
}

int SLAPI PPLinkFilesArray::Add(PPLinkFile * pLink, uint * pPos)
{
	int    ok = 0;
	if(pLink && StoreDir.Len() && pLink->Path.Len() && lsearch(pLink, 0, PTR_CMPFUNC(PPLinkFile)) <= 0 &&
		strnicmp866(StoreDir, pLink->Path, StoreDir.Len()) != 0) {
		PPLinkFile * p_flink = new PPLinkFile;
		SString store_path;
		pLink->Id = GetNewId();
		pLink->Flags |= PPLNKFILE_ISNEW;
		*p_flink = *pLink;
		GetFilePath(p_flink->Id, p_flink->Ext, store_path);
		// @v9.3.9 if(::CopyFile(p_flink->Path, store_path, 0) > 0) // @unicodeproblem
		if(SCopyFile(p_flink->Path, store_path, 0, FILE_SHARE_READ, 0)) // @v9.3.9
			ok = insert(p_flink);
		if(ok <= 0)
			delete p_flink;
		else
			ASSIGN_PTR(pPos, getCount() - 1);
	}
	return ok;
}

int SLAPI PPLinkFilesArray::Remove(uint pos)
{
	int    ok = 0;
	if(pos < getCount()) {
		SString path;
		if(GetFilePath(pos, path) > 0 && SFile::Remove(path)) {
			atFree(pos);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPLinkFilesArray::RemoveByAry(const PPLinkFilesArray * pAry)
{
	for(long i = getCount(); i > 0; i--) {
		uint   pos = 0;
		int   found = 0;
		PPLinkFile * p_flink = at(i - 1);
		if(!pAry || pAry->lsearch(p_flink, &pos, PTR_CMPFUNC(PPLinkFile)) <= 0 || p_flink->Id != pAry->at(pos)->Id)
			Remove(i - 1);
	}
	RVALUEPTR(*this, pAry);
	return 1;
}

#define LOW_APP_VER  1
#define HIGH_APP_VER 20

int SLAPI PPLinkFilesArray::Edit(uint pos)
{
	int    ok = -1;
	if(pos >= 0 && (uint)pos < getCount()) {
		int r = 1;
		SString path;
		GetFilePath(pos, path);
		::ShellExecute(0, _T("open"), path, NULL, NULL, SW_SHOWNORMAL); // @unicodeproblem
		ok = 1;
	}
	return ok;
}

int SLAPI PPLinkFilesArray::EditDescr(uint pos)
{
	int    ok = -1;
	if(pos >= 0 && pos < getCount()) {
		SString title, fname;
		PPLinkFile * p_flink = at(pos);
		SString descr = p_flink->Description;
		SPathStruc ps(p_flink->Path);
		ps.Merge(0, SPathStruc::fDrv|SPathStruc::fDir, fname);
		PPInputStringDialogParam isd_param(PPLoadTextS(PPTXT_INPUTDESCR, title), fname);
		if(InputStringDialog(&isd_param, descr) > 0 && descr.Len()) {
			p_flink->Description = descr;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPLinkFilesArray::GetFilePath(uint pos, SString & aFilePath) const
{
	int    ok = 0;
	if(pos >= 0 && pos < getCount()) {
		const PPLinkFile * p_flink = at(pos);
		GetFilePath(p_flink->Id, p_flink->Ext, aFilePath);
		ok = 1;
	}
	return ok;
}

int SLAPI PPLinkFilesArray::GetFilePath(PPID id, const char * pExt, SString & rFilePath) const
{
	SString data_path, fname;
	fname.CatLongZ(id, 8);
	(data_path = StoreDir).SetLastSlash();
	createDir(data_path);
	data_path.Cat(fname);
	if(pExt && sstrlen(pExt))
		data_path.Dot().Cat(pExt);
	rFilePath = data_path;
	return 1;
}

PPID SLAPI PPLinkFilesArray::GetNewId() const
{
	PPID   id = 0;
	PPBillConfig cfg;
	PPObjOpCounter opc_obj;
	BillObj->ReadConfig(&cfg);
	{
		PPTransaction tra(1);
		THROW(tra);
		if(!cfg.LnkFilesCntrID) {
			PPOpCounterPacket opc_pack;
			opc_pack.Init(0, 0);
			opc_pack.Head.ObjType = PPOBJ_BILL;
			opc_pack.Head.OwnerObjID = -1;
			opc_obj.PutPacket(&cfg.LnkFilesCntrID, &opc_pack, 0);
			THROW(PPObjBill_WriteConfig(&cfg, 0, 0 /*ta*/));
		}
		opc_obj.GetCode(cfg.LnkFilesCntrID, &id, 0, 0, 0, 0 /*ta*/);
		THROW(tra.Commit());
	}
	CATCH
		id = 0;
	ENDCATCH
	return id;
}

int SLAPI PPLinkFilesArray::ReadFromProp(PPID billID)
{
	freeAll();

	int    ok = -1;
	char * p_buf = 0;
	_Entry _e;
	Reference * p_ref = PPRef;
	if(p_ref->GetProperty(PPOBJ_BILL, billID, BILLPRP_LINKFILES, &_e, sizeof(_e)) > 0 && _e.ItemsCount > 0) {
		THROW_MEM(p_buf = new char[_e.Size]);
		if(p_ref->GetProperty(PPOBJ_BILL, billID, BILLPRP_LINKFILES, p_buf, _e.Size) > 0) {
			char * p = p_buf;
			p += sizeof(_e);
			for(uint i = 0; i < (uint)_e.ItemsCount; p += at(i)->Size(), i++) {
				PPLinkFile * p_flink = CreateNewItem();
				CALLPTRMEMB(p_flink, CopyFrom(p));
			}
		}
	}
	if(getCount())
		ok = 1;
	CATCHZOK
	delete [] p_buf;
	return ok;
}

int SLAPI PPLinkFilesArray::WriteToProp(PPID billID, int useTa)
{
	int    ok = 1;
	char * p_buf = 0;
	PPLinkFilesArray prev_ary;
	THROW(prev_ary.ReadFromProp(billID));
	{
		Reference * p_ref = PPRef;
		PPTransaction tra(useTa);
		THROW(tra);
		if(getCount()) {
			char * p = 0;
			size_t sz = 0;
			_Entry _e;
			_e.Tag        = PPOBJ_BILL;
			_e.ID         = billID;
			_e.Prop       = BILLPRP_LINKFILES;
			_e.ItemsCount = getCount();
			_e.Size = sizeof(_e);
			for(uint i = 0; i < (uint)_e.ItemsCount; i++)
				_e.Size += (int16)at(i)->Size();
			sz = MAX(PROPRECFIXSIZE, _e.Size);
			THROW_MEM(p_buf = new char[sz]);
			memzero(p_buf, sz);
			p = p_buf;
			memcpy(p, &_e, sizeof(_e));
			p += sizeof(_e);
			for(uint i = 0; i < getCount(); p += at(i)->Size(), i++)
				at(i)->CopyTo((void**)&p);
			THROW(ok = p_ref->PutProp(PPOBJ_BILL, billID, BILLPRP_LINKFILES, p_buf, sz));
		}
		else {
			THROW(p_ref->RemoveProperty(PPOBJ_BILL, billID, BILLPRP_LINKFILES, 0));
		}
		prev_ary.RemoveByAry(this);
		THROW(tra.Commit());
	}
	CATCHZOK
	delete [] p_buf;
	return ok;
}

class LinkFilesDialog : public PPListDialog {
public:
	LinkFilesDialog(const PPLinkFilesArray * pAry) : PPListDialog(DLG_LINKFILES, CTL_LINKFILES_LIST)
	{
		setDTS(pAry);
		FileBrowseCtrlGroup * p_fbg = 0;
		SString temp_buf;
		PPLoadTextWin(PPTXT_OPENFILET_LINKBILLFILES, temp_buf);
		if(p_fbg = new FileBrowseCtrlGroup(cmLink, 0, temp_buf, 0)) {
			p_fbg->addPattern(PPTXT_FILPAT_ALLLINKFILES);
			p_fbg->addPattern(PPTXT_FILPAT_TEXT);
			p_fbg->addPattern(PPTXT_FILPAT_MSWORD);
			p_fbg->addPattern(PPTXT_FILPAT_MSEXCEL);
			p_fbg->addPattern(PPTXT_FILPAT_ADOBEACROBAT);
			p_fbg->addPattern(PPTXT_FILPAT_PICT);
			addGroup(GRP_FBG, p_fbg);
		}
		{
			PPBillConfig bcfg;
			if(PPObjBill::ReadConfig(&bcfg) > 0)
				AddFilesFolder = bcfg.AddFilesFolder;
			enableCommand(cmWaitFile, BIN(AddFilesFolder.Len()));
		}
		updateList(-1);
	}
	int    setDTS(const PPLinkFilesArray *);
	int    getDTS(PPLinkFilesArray *);
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	int    LinkFile(const char * pPath, uint * pPos);

	SString AddFilesFolder;
	PPLinkFilesArray LinksAry;
};

IMPL_HANDLE_EVENT(LinkFilesDialog)
{
	int    link_cmd = BIN(TVCOMMAND && TVCMD == cmLink);
	PPListDialog::handleEvent(event);
	if(event.isKeyDown(kbF2)) {
		if(LinksAry.getCount() && P_Box && P_Box->def && LinksAry.EditDescr(P_Box->def->_curItem()))
			updateList(-1);
		clearEvent(event);
	}
	else if(link_cmd) {
		FileBrowseCtrlGroup * p_fgb = (FileBrowseCtrlGroup*)getGroup(GRP_FBG);
		if(p_fgb) {
			FileBrowseCtrlGroup::Rec rec;
			p_fgb->getData(this, &rec);
			if(LinkFile(rec.FilePath, 0) > 0)
				updateList(-1);
		}
		clearEvent(event);
	}
	else if(TVCOMMAND) {
		if(TVCMD == cmWaitFile) {
			SString file;
			PPWait(1);
			if(AddFilesFolder.Len() && WaitNewFile(AddFilesFolder, file) > 0) {
				PPWait(0);
				if(LinkFile(file, 0) > 0)
					updateList(-1);
			}
			PPWait(0);
			clearEvent(event);
		}
	}
}

int LinkFilesDialog::LinkFile(const char * pPath, uint * pPos)
{
	int    ok = -1;
	if(pPath && sstrlen(pPath)) {
		SString title, fname;
		PPLinkFile flink;
		SPathStruc ps(pPath);
		ps.Merge(0, SPathStruc::fDrv|SPathStruc::fDir, fname);
		flink.Init(pPath);
		PPInputStringDialogParam isd_param(PPLoadTextS(PPTXT_INPUTDESCR, title), fname.ToOem());
		if(InputStringDialog(&isd_param, flink.Description) > 0)
			if(flink.Description.Len() && LinksAry.Add(&flink, pPos))
				ok = 1;
	}
	return ok;
}

int LinkFilesDialog::setupList()
{
	int    ok = -1;
	if(LinksAry.getCount()) {
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; i < LinksAry.getCount(); i++) {
			PPLinkFile flink = *LinksAry.at(i);
			ss.Z().add(flink.Description);
			THROW(addStringToList(i, ss.getBuf()));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int LinkFilesDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1, selected = 0;
	long   sel_id = 0;
	SString exts, pattern, descr, name, ext;
	StringSet ss_ext;
	StrAssocArray items_list, ext_list;
	StringSet ss(',', PPLoadTextS(PPTXT_LNKFILESEXTS, exts));
	for(uint i = 0, k = 0; ss.get(&i, pattern) > 0; k++) {
		if(k > 0) {
			uint   ext_start = 0, ext_len = 0;
			if(pattern.Divide(':', descr, ext) > 0) {
				while(ext.C(ext_start) && ext.C(ext_start) != '.')
					++ext_start;
				if(ext.C(ext_start)) {
					++ext_start;
					while(ext.C(ext_start+ext_len) && ext.C(ext_start+ext_len) != ';')
						++ext_len;
					ext.Sub(ext_start, ext_len, name);
					if(name.CmpNC("pdf") != 0) {
						items_list.Add((long)k, descr);
						ext_list.Add((long)k, name);
					}
				}
			}
		}
	}
	sel_id = 1;
	while(!selected && ComboBoxSelDialog2(&items_list, PPTXT_CREATEFILETYPE, PPTXT_CREATEFILETYPE, &sel_id, 0) > 0)
		if(sel_id)
			selected = 1;
		else
			PPError(PPERR_FILETYPENOTSEL);
	if(selected) {
		PPInputStringDialogParam isd_param;
		PPLoadText(PPTXT_INPUTDESCR, isd_param.Title);
		selected = BIN(InputStringDialog(&isd_param, descr.Z()) > 0 && descr.Len());
	}
	if(selected) {
		uint   pos = 0;
		ext_list.GetText(sel_id, ext);
		THROW(LinksAry.AddNewByExt(ext, descr, &pos) > 0);
		if((ok = LinksAry.Edit(pos)) > 0) {
			ASSIGN_PTR(pPos, (long)pos);
			ASSIGN_PTR(pID, LinksAry.at(pos)->Id);
		}
	}
	CATCHZOKPPERR
	return ok;
}

int LinkFilesDialog::editItem(long pos, long id)
{
	return LinksAry.Edit(pos);
}

int LinkFilesDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && (uint)pos < LinksAry.getCount()) {
		if(LinksAry.at(pos)->Flags & PPLNKFILE_ISNEW)
			LinksAry.Remove(pos);
		else
			LinksAry.atFree(pos);
		ok = 1;
	}
	return ok;
}

int LinkFilesDialog::setDTS(const PPLinkFilesArray * pAry)
{
	if(!RVALUEPTR(LinksAry, pAry))
		LinksAry.freeAll();
	/*
	if(pAry)
		LinksAry = *pAry;
	else
		LinksAry.freeAll();
	*/
	return 1;
}

int LinkFilesDialog::getDTS(PPLinkFilesArray * pAry)
{
	ASSIGN_PTR(pAry, LinksAry);
	return 1;
}

int BillDialog::editLinkFiles()
{
	int    ok = -1;
	LinkFilesDialog * dlg = 0;
	if(P_Pack) {
		THROW_MEM(dlg = new LinkFilesDialog(&P_Pack->LnkFiles));
		THROW(CheckDialogPtr(&dlg));
		if(ExecView(dlg) == cmOK)
			dlg->getDTS(&P_Pack->LnkFiles);
		else {
			PPLinkFilesArray lnkf_ary;
			dlg->getDTS(&lnkf_ary);
			lnkf_ary.RemoveByAry(&P_Pack->LnkFiles);
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

#define RESIZE_DELTA 50L

int BillDialog::showLinkFilesList()
{
	HWND   list = GetDlgItem(H(), CTL_BILL_LNKFILELIST);
	if(list) {
		int    show_cmd = (!P_Pack || !P_Pack->LnkFiles.getCount()) ? SW_HIDE : SW_NORMAL;
		TRect  rect = DefaultRect;
		if(show_cmd == SW_NORMAL)
			rect.b.y += RESIZE_DELTA;
		MoveWindow(H(), rect.a.x, rect.a.y, rect.width(), rect.height(), 1);
		if(show_cmd == SW_NORMAL) {
			TRect  rect = getClientRect();
			MoveWindow(list, rect.a.x, rect.b.y - RESIZE_DELTA, rect.width(), RESIZE_DELTA, 1);
			updateList(-1);
		}
		ShowWindow(list, show_cmd);
		setupPosition();
	}
	return 1;
}

int BillDialog::setupList()
{
	int    ok = -1;
	if(P_Pack && P_Pack->LnkFiles.getCount()) {
		for(uint i = 0; i < P_Pack->LnkFiles.getCount(); i++) {
			StringSet ss(SLBColumnDelim);
			PPLinkFile flink = *P_Pack->LnkFiles.at(i);
			ss.add(flink.Description);
			THROW(addStringToList(i, ss.getBuf()));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int BillDialog::addItem(long * pPos, long * pID)
{
	editLinkFiles();
	showLinkFilesList();
	return 1;
}

int BillDialog::editItem(long pos, long id)
{
	return P_Pack ? P_Pack->LnkFiles.Edit(pos) : -1;
}

int BillDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(P_Pack && pos >= 0 && (uint)pos < P_Pack->LnkFiles.getCount()) {
		if(P_Pack->LnkFiles.at(pos)->Flags & PPLNKFILE_ISNEW)
			P_Pack->LnkFiles.Remove(pos);
		else
			P_Pack->LnkFiles.atFree(pos);
		ok = 1;
		showLinkFilesList();
	}
	return ok;
}

int BillDialog::sendItem(long pos, long id)
{
	int    ok = -1;
	SendMailDialog * dlg = 0;
	if(P_Pack && pos >= 0 && pos < (long)P_Pack->LnkFiles.getCount()) {
		PPID   ar_id = 0;
		SString path, addr;
		PPLinkFile file_info;
		SendMailDialog::Rec data;
		PPAlbatrosConfig alb_cfg;
		THROW(CheckDialogPtr(&(dlg = new SendMailDialog)));
		getCtrlData(CTLSEL_BILL_OBJECT, &ar_id);
		{
			PPELinkArray addr_list;
			PersonCore::GetELinks(ObjectToPerson(ar_id), &addr_list);
			if(addr_list.GetPhones(1, addr, ELNKRT_EMAIL) > 0)
				data.AddrList.Add(1, addr);
		}
		file_info = *P_Pack->LnkFiles.at(pos);
		P_Pack->LnkFiles.GetFilePath(pos, path);
		THROW_SL(fileExists(path));
		data.Subj = file_info.Description;
		data.FilesList.insert(newStr(path));
		THROW(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0);
		data.MailAccID = alb_cfg.Hdr.MailAccID;
		dlg->setDTS(&data);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			if(dlg->getDTS(&data) > 0) {
				if(data.MailAccID) {
					int    first = 1;
					for(uint i = 0; i < data.AddrList.getCount(); i++) {
						addr = data.AddrList.Get(i).Txt;
						if(addr.NotEmptyS()) {
							if(!first && data.Delay > 0 && data.Delay <= (24 * 3600 * 1000)) {
								SDelay(data.Delay);
							}
							data.Subj.Transf(CTRANSF_INNER_TO_UTF8);
							data.Text.Transf(CTRANSF_INNER_TO_UTF8);
							THROW(SendMailWithAttach(data.Subj, path, data.Text, addr, data.MailAccID));
							first = 0;
							ok = valid_data = 1;
						}
					}
				}
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int BillDialog::calcDate(uint ctlID)
{
	int    ok = -1;
	LDATE  dt = getCtrlDate(CTL_BILL_DATE);
	if(checkdate(dt)) {
		SString input_buf;
		PPInputStringDialogParam isd_param;
		PPLoadText(PPTXT_INPUTNUMDAYS, isd_param.InputTitle);
		if(InputStringDialog(&isd_param, input_buf) > 0) {
			int    num_days = input_buf.ToLong();
			if(num_days > 0) {
				setCtrlDate(ctlID, plusdate(dt, num_days));
				ok = 1;
			}
		}
	}
	return ok;
}

int BillDialog::EditFreight()
{
	int    ok = -1;
	if(P_BObj->EditFreightDialog(P_Pack) > 0) {
		if(oneof2(PayDateBase, PPClientAgreement::pdbFreightIssue, PPClientAgreement::pdbFreightArrival) && !P_Pack->Rec.ID) {
			P_Pack->SetupDefaultPayDate(PaymTerm, PayDateBase);
			SetupPaymDateCtrls();
		}
		ok = 1;
	}
	return ok;
}

IMPL_HANDLE_EVENT(BillDialog)
{
	PPID   id;
	double tmp;
	LDATE  dt;
	if(event.isCmd(cmExecute)) {
		if(P_Pack->Rec.Flags & BILLF_CASH && !(Flags & fEditMode))
			editItems();
		// Далее управление передается базовому классу
	}
	else if(event.isCmd(cmOK))
		calcAmounts(&tmp);
	if(!(TVKEYDOWN && TVKEY == KB_CTRLENTER)) {
		//
		// Функция PPListDialog::handleEvent обрабатывает F11 как cmOK.
		// Здесь этого делать нельзя.
		//
		PPListDialog::handleEvent(event);
	}
	if(event.isClusterClk(CTL_BILL_TTLDISCOUNT)) {
		if(getCtrlUInt16(CTL_BILL_TTLDISCOUNT))
			setCtrlLong(CTL_BILL_DISCOUNT, 0);
		calcAmounts(&tmp);
	}
	else if(event.isCbSelected(CTLSEL_BILL_OBJECT))
		ReplyCntragntSelection(0);
	else if(event.isCbSelected(CTLSEL_BILL_OBJ2)) {
		PPID   ar_id = getCtrlLong(CTLSEL_BILL_OBJ2);
		if(!P_Pack->SetupObject2(ar_id)) {
			PPError();
			setCtrlLong(CTLSEL_BILL_OBJ2, P_Pack->Rec.Object2);
		}
	}
	else if(TVCOMMAND) {
		switch(TVCMD) {
			case cmAgreement:
				EditAgreement();
				break;
			case cmEdiAckBill:
				{
					long   recadv_status = BillCore::GetRecadvStatus(P_Pack->Rec);
					if(recadv_status == PPEDI_RECADV_STATUS_PARTACCEPT) {
						BillTbl::Rec recadv_rec;
						DateIter di;
						if(P_BObj->P_Tbl->EnumLinks(P_Pack->Rec.ID, &di, BLNK_EDIRECADV, &recadv_rec) > 0)
							P_BObj->Edit(&recadv_rec.ID, 0);
					}
				}
				break;
			case cmExAmountList:
				if(Flags & fHasAmtIDList) {
					getCtrlData(CTL_BILL_DATE, &dt);
					getCurGroupData();
					if(AmountListDialog(&P_Pack->Amounts, &ExtAmtIDList, dt, 0, OLW_CANEDIT|OLW_CANDELETE|OLW_CANINSERT) > 0)
						setCurGroupData();
				}
				break;
			case cmLinkedBill:
				{
					id = P_Pack->Rec.LinkBillID;
					if(id != 0) {
                        PPObjBill::EditParam ep;
						ep.Flags |= PPObjBill::efCascade;
						P_BObj->Edit(&id, &ep);
					}
					else if(P_Pack->OpTypeID == PPOPT_PAYMENT) {
						PPOprKind op_rec;
						if(GetOpData(P_Pack->Rec.OpID, &op_rec) > 0 && op_rec.LinkOpID) {
							BillFilt bill_filt;
							bill_filt.OpID = op_rec.LinkOpID;
							bill_filt.ObjectID = P_Pack->Rec.Object;
							bill_filt.AgentID = P_Pack->Ext.AgentID;
							bill_filt.Flags |= (BillFilt::fAsSelector | BillFilt::fShowDebt | BillFilt::fDebtOnly);
							PPViewBill bill_view;
							if(bill_view.Init_(&bill_filt)) {
								if(bill_view.Browse(0) > 0) {
									PPID bill_id = ((BillFilt*)bill_view.GetBaseFilt())->Sel;
									BillTbl::Rec bill_rec;
									if(P_BObj->Search(bill_id, &bill_rec) > 0) {
										P_Pack->Rec.LinkBillID = bill_id;
										if(!P_Pack->Rec.Object) {
											P_Pack->Rec.Object = bill_rec.Object;
											// Приходится использовать SetupArCombo поскольку из-за OLW_LOADDEFONOPEN
											// setCtrlLong не установит значение.
											SetupArCombo(this, CTLSEL_BILL_OBJECT, P_Pack->Rec.Object, OLW_LOADDEFONOPEN|OLW_CANINSERT, P_Pack->AccSheetID, sacfNonGeneric);
											ReplyCntragntSelection(1);
										}
										const double amt = getCtrlReal(CTL_BILL_AMOUNT);
										if(amt == 0.0) {
											double paym = 0.0;
											P_BObj->P_Tbl->GetAmount(bill_rec.ID, PPAMT_PAYMENT, bill_rec.CurID, &paym);
											if(paym < bill_rec.Amount)
												setCtrlReal(CTL_BILL_AMOUNT, bill_rec.Amount - paym);
										}
										getCtrlData(CTL_BILL_MEMO, P_Pack->Rec.Memo);
										P_Pack->Rec.Memo[0] = 0;
										P_BObj->SubstMemo(P_Pack);
										setCtrlData(CTL_BILL_MEMO, P_Pack->Rec.Memo);
										{
											SString text;
											PPLoadString("but_linkbill", text);
											setButtonText(cmLinkedBill, text.Transf(CTRANSF_INNER_TO_OUTER));
											setupDebt();
										}
									}
								}
							}
						}
					}
				}
				break;
			case cmReckonBill:
				if((id = P_Pack->PaymBillID) != 0)
					P_BObj->Edit(&id, 0);
				break;
			case cmBillTaxes:       EditBillTaxes(&P_Pack->Amounts, getCtrlReal(CTL_BILL_AMOUNT)); break;
			case cmDetail:          editItems(); break;
			case cmAdvItems:        editItems(); break;
			/* @v6.2.4 Функция печати из документа блокирована из-за возможности распечатать непроведенный док.
			case cmPrint:           PrintGoodsBill(P_Pack); break;
			*/
			case cmRentCondition:   EditRentCondition(&P_Pack->Rent); break;
			case cmPaymOrder:       editPaymOrder(0);     break;
			case cmBillFreight:
				EditFreight();
				break;
			case cmCalcPrevAdvRest: calcAdvanceRepRest(); break;
			case cmPrevAdvBills:
				{
					PPID   obj_id = getCtrlLong(CTLSEL_BILL_OBJECT);
					if(obj_id) {
						AccAnlzFilt flt;
						if(P_BObj->atobj->ConvertAcct(&CConfig.ImprestAcct, 0 /* @curID */, &flt.AcctId, &flt.AccSheetID) > 0) {
							flt.AcctId.ar = obj_id;
							flt.Aco = ACO_3;
							int    r = P_BObj->atobj->P_Tbl->AcctIDToRel(&flt.AcctId, &flt.AccID);
							if(r > 0)
								ViewAccAnlz(&flt, aakndGeneric);
							else if(r < 0)
								PPError(PPERR_ACCRELABSENCE, 0);
							else
								PPErrorZ();
						}
					}
				}
				break;
			case cmLinkFiles:
				editLinkFiles();
				showLinkFilesList();
				break;
			case cmBillExtra:
				{
					const PPID prev_agent_id = P_Pack->Ext.AgentID;
					P_Pack->Ext.IsShipped = BIN(P_Pack->Rec.Flags & BILLF_SHIPPED);
					P_Pack->Ext.SCardID = P_Pack->Rec.SCardID;
					for(int r = 0; !r && BillExtraDialog(&P_Pack->Ext, &P_Pack->BTagL, 0) > 0;) {
						r = 1;
						if(P_Pack->Ext.AgentID != prev_agent_id) {
							PPID   debt_dim_id = 0;
							if(Flags & fCheckCreditLim && !CliAgt.IsEmpty() && P_Pack->GetDebtDim(&debt_dim_id) > 0) {
								int    is_stopped = CliAgt.IsStopped(debt_dim_id);
								if(is_stopped > 0) {
									SString msg_buf, debt_dim_name;
									GetObjectName(PPOBJ_DEBTDIM, debt_dim_id, debt_dim_name, 0);
									GetArticleName(CliAgt.ClientID, msg_buf);
									msg_buf.CatChar(':').Cat(debt_dim_name);
									PPError(PPERR_DENYSTOPPEDAR, msg_buf);
									P_Pack->Ext.AgentID = prev_agent_id;
									r = 0;
								}
								else {
									double amt = 0.0;
									calcAmounts(&amt);
									if(!checkCreditOverflow(amt)) {
										P_Pack->Ext.AgentID = prev_agent_id;
										r = 0;
									}
								}
							}
						}
						if(r) {
							SETFLAG(P_Pack->Rec.Flags, BILLF_SHIPPED, P_Pack->Ext.IsShipped);
							P_Pack->Rec.SCardID = P_Pack->Ext.SCardID;
							if(PayDateBase == PPClientAgreement::pdbInvoice && !P_Pack->Rec.ID) {
								P_Pack->SetupDefaultPayDate(PaymTerm, PayDateBase);
								SetupPaymDateCtrls();
							}
						}
					}
				}
				break;
			case cmBillPayPlan:
				dt = getCtrlDate(CTL_BILL_PAYDATE);
				if(P_Pack->Pays.getCount() <= 1) {
					P_Pack->Pays.freeAll();
					PayPlanTbl::Rec paym;
					MEMSZERO(paym);
					paym.PayDate = dt;
					paym.Amount  = getCtrlReal(CTL_BILL_AMOUNT);
					P_Pack->Pays.Update(&paym, 0);
				}
				if(EditPaymPlan(P_Pack, &P_Pack->Pays) > 0) {
					SetupPaymDateCtrls();
				}
				break;
			case cmTags:
				P_Pack->BTagL.ObjType = PPOBJ_BILL;
				if(EditObjTagValList(&P_Pack->BTagL, 0) > 0) {
					if(PayDateBase > PPClientAgreement::pdbTagBias && !P_Pack->Rec.ID) {
						P_Pack->SetupDefaultPayDate(PaymTerm, PayDateBase);
						SetupPaymDateCtrls();
					}
				}
				break;
			case cmCtlColor:
				{
					TDrawCtrlData * p_dc = (TDrawCtrlData *)TVINFOPTR;
					if(p_dc) {
						if(getCtrlHandle(CTL_BILL_PAYDATE) == p_dc->H_Ctl) {
							if(PaymTerm >= 0) {
								getCtrlData(CTL_BILL_DATE, &P_Pack->Rec.Dt);
								LDATE paymdate = getCtrlDate(CTL_BILL_PAYDATE);
								LDATE new_paymdate = P_Pack->CalcDefaultPayDate(PaymTerm, PayDateBase);
								if(!paymdate || paymdate != new_paymdate) {
									::SetBkMode(p_dc->H_DC, TRANSPARENT);
									::SetTextColor(p_dc->H_DC, GetColorRef(SClrWhite));
									p_dc->H_Br = (HBRUSH)Ptb.Get(brushIllPaymDate);
									clearEvent(event);
								}
							}
						}
						else if(getCtrlHandle(CTL_BILL_DOC) == p_dc->H_Ctl) {
							int    ss = P_Pack->GetSyncStatus();
							if(ss > 0) {
								::SetBkMode(p_dc->H_DC, TRANSPARENT);
								p_dc->H_Br = (HBRUSH)Ptb.Get(brushSynced);
								clearEvent(event);
							}
						}
					}
				}
				break;
			default:
				return;
		}
	}
	else if(TVBROADCAST) {
		if(TVCMD == cmChangedFocus) {
			if(event.isCtlEvent(CTL_BILL_DISCOUNT))
				calcAmounts(&tmp);
			else if(P_Pack->Rec.Flags & BILLF_ADVANCEREP && P_Pack->P_AdvRep) {
				uint ctl_id = event.getCtlID();
				if(oneof3(ctl_id, CTL_BILL_ADV_RCPAMT1, CTL_BILL_ADV_RCPAMT2, CTL_BILL_ADV_INREST))
					setupAdvanceRepTotal(P_Pack->P_AdvRep);
			}
		}
		else
			return;
	}
	else if(TVKEYDOWN) {
		switch(TVKEY) {
			/* @v6.2.4 Функция печати из документа блокирована из-за возможности распечатаь непроведенный док.
			case kbF7:
				if(getDTS(0))
					PrintGoodsBill(P_Pack);
				else
					PPError();
				break;
			*/
			case kbF2:
				if(LConfig.Cash)
					CalcDiff(getCtrlReal(CTL_BILL_AMOUNT), 0);
				else {
					const uint ctl_id = GetCurrId();
					switch(ctl_id) {
						case CTL_BILL_DOC: P_BObj->UpdateOpCounter(P_Pack); break;
						case CTL_BILL_ADV_INREST: calcAdvanceRepRest(); break;
						case CTL_BILL_DUEDATE:    calcDate(ctl_id); break;
						case CTL_BILL_AMOUNT:
							if(getCtrlData(CTL_BILL_DEBTSUM, &tmp)) {
								TView * p_view = getCtrlView(CTL_BILL_AMOUNT);
								if(p_view && !p_view->IsInState(sfDisabled))
									setCtrlData(CTL_BILL_AMOUNT, &tmp);
							}
							break;
						case CTLSEL_BILL_OBJECT:
						case CTL_BILL_OBJECT:
							{
								SString code;
								PPID   ar_id = 0;
								PPID   reg_type_id = P_BObj->Cfg.ClCodeRegTypeID;
								if(P_Pack->AccSheetID && reg_type_id > 0 && BarcodeInputDialog(0, code) > 0) {
									ArticleTbl::Rec ar_rec;
									if(ArObj.SearchByRegCode(P_Pack->AccSheetID, reg_type_id, code, &ar_id, &ar_rec) > 0) {
										setCtrlLong(CTLSEL_BILL_OBJECT, ar_rec.ID);
										ReplyCntragntSelection(0);
									}
								}
							}
							break;
						case CTL_BILL_PAYDATE:
							if(PaymTerm >= 0) {
								getCtrlData(CTL_BILL_DATE, &P_Pack->Rec.Dt);
                                P_Pack->SetupDefaultPayDate(PaymTerm, PayDateBase);
                                SetupPaymDateCtrls();
							}
							else
								calcDate(ctl_id);
							break;
						case CTL_BILL_MEMO:
							getDTS(0);
							P_BObj->SubstMemo(P_Pack);
							setCtrlData(CTL_BILL_MEMO, P_Pack->Rec.Memo);
							break;
						case CTL_BILL_LNKFILELIST:
							if(P_Pack && P_Pack->LnkFiles.getCount() && P_Box && P_Box->def && P_Pack->LnkFiles.EditDescr(P_Box->def->_curItem()))
								updateList(-1);
							break;
					}
				}
				break;
			case kbF4:
				EditFreight();
				break;
			case KB_CTRLENTER:
				if(P_Pack->OpTypeID != PPOPT_POOL)
					editItems();
				break;
			default:
				return;
		}
	}
	else
		return;
	clearEvent(event);
}

void BillDialog::setupParentOfContragent()
{
	if(getCtrlView(CTL_BILL_ST_PARENTPERSON)) {
		PPID   rel_ar_id = 0;
		SString rel_ar_name;
		PPID   client_id = getCtrlLong(CTLSEL_BILL_OBJECT);
		if(ArObj.GetRelPersonSingle(client_id, PPPSNRELTYP_AFFIL, 0, &rel_ar_id) > 0)
			GetArticleName(rel_ar_id, rel_ar_name);
		setStaticText(CTL_BILL_ST_PARENTPERSON, rel_ar_name);
	}
}

void BillDialog::setupDebtText()
{
	if(getCtrlView(CTL_BILL_ST_DEBT)) {
		SString text;
		PPID   debt_dim_id = 0;
		P_Pack->GetDebtDim(&debt_dim_id);
		if(debt_dim_id) {
			PPObjDebtDim dd_obj;
			PPDebtDim dd_rec;
			if(dd_obj.Fetch(debt_dim_id, &dd_rec) > 0)
				text.Cat(dd_rec.Name);
		}
		double limit = CliAgt.GetCreditLimit(debt_dim_id);
		double curr_debt = getCurrentDebt(debt_dim_id);
		if(curr_debt > 0.0 || limit > 0.0) {
			text.CatDivIfNotEmpty(':', 2);
			text.CatEq("Debt", curr_debt, SFMT_MONEY);
			if(limit > 0.0)
				text.CatDiv(';', 2).CatEq("Limit", limit, SFMT_MONEY);
		}
		setStaticText(CTL_BILL_ST_DEBT, text);
	}
}

void BillDialog::setupByCntragnt()
{
	const PPID ar_id = P_Pack->Rec.Object;
	PaymTerm = -1;
	PayDateBase = 0;
	CurrDebt = 0.0;
	CDebtList.clear();
	CliAgt.Init();
	if(ar_id) {
		ArticleTbl::Rec ar_rec;
		if(ArObj.Fetch(ar_id, &ar_rec) > 0) {
			const  int    agt_kind = PPObjArticle::GetAgreementKind(&ar_rec);
			PPClientAgreement cli_agt;
			if(agt_kind == 1 && ArObj.GetClientAgreement(ar_id, &cli_agt, 1) > 0)
				CliAgt = cli_agt;
			if(P_Pack->Rec.Flags & BILLF_NEEDPAYMENT) {
				if(agt_kind == 2) {
					PPSupplAgreement suppl_agt;
					if(ArObj.GetSupplAgreement(ar_id, &suppl_agt, 1) > 0) {
						PaymTerm = suppl_agt.DefPayPeriod;
						PayDateBase = 0;
					}
				}
				else {
					PaymTerm = CliAgt.DefPayPeriod;
					PayDateBase = CliAgt.PaymDateBase;
				}
			}
			if(Flags & fCheckCreditLim) {
				PPID   acs_id = 0;
				int    has_matured_debt = 0;
				if((CliAgt.MaxCredit > 0.0 || P_BObj->Cfg.Flags & BCF_WARNMATCLIDEBT) && !(CliAgt.Flags & AGTF_DONTCALCDEBTINBILL)) {
					PPObjBill::DebtBlock blk;
					DateRange cdp;
					P_BObj->CalcClientDebt(ar_id, P_BObj->GetDefaultClientDebtPeriod(cdp), 1, blk);
					ASSIGN_PTR(&has_matured_debt, blk.HasMatured);
					CurrDebt = blk.Debt;
					blk.GetDimList(CDebtList);
				}
				if(has_matured_debt) {
					; // PPMessage(mfInfo|mfOK, PPINF_CLIHASMATDEBT, add_msg.Z().Cat(CurrDebt, SFMT_MONEY));
				}
			}
		}
	}
	setupDebtText();
}

void BillDialog::ReplyCntragntSelection(int force)
{
	P_Pack->AgtQuotKindID = 0; //
	PaymTerm = -1;             // Срок оплаты по документу (в днях), взятый из соглашения //
	PayDateBase = 0;
	SString add_msg;
	PPID   client_id = force ? P_Pack->Rec.Object : getCtrlLong(CTLSEL_BILL_OBJECT);
	const  int    to_force_update = BIN((force && client_id) || client_id != P_Pack->Rec.Object);
	if(P_Pack->SampleBillID)
		P_BObj->SetupQuot(P_Pack, client_id);
	PPBillPacket::SetupObjectBlock sob;
	if(P_Pack->SetupObject(client_id, sob)) {
		PPID   agent_id = 0;     // Агент, взятый из соглашения, которого следует установить в документ
		if(sob.State & PPBillPacket::SetupObjectBlock::stHasCliAgreement) {
			if(!(sob.CliAgt.Flags & AGTF_DEFAULT))
				agent_id = sob.CliAgt.DefAgentID;
			if(P_Pack->Rec.Flags & BILLF_NEEDPAYMENT) {
				PaymTerm = sob.CliAgt.DefPayPeriod;
				PayDateBase = sob.CliAgt.PaymDateBase;
			}
		}
		else if(sob.State & PPBillPacket::SetupObjectBlock::stHasSupplAgreement) {
			if(!(sob.SupplAgt.Flags & AGTF_DEFAULT))
				agent_id = sob.SupplAgt.DefAgentID;
			if(P_Pack->Rec.Flags & BILLF_NEEDPAYMENT) {
				PaymTerm = sob.SupplAgt.DefPayPeriod;
				PayDateBase = 0;
			}
		}
		if(agent_id && !P_Pack->Ext.AgentID && GetAgentAccSheet())
			P_Pack->Ext.AgentID = agent_id;
		if(sob.State & PPBillPacket::SetupObjectBlock::stHasCliAgreement)
			CliAgt = sob.CliAgt;
		else
			CliAgt.Init();
		if(Flags & fSetupObj2ByCliAgt) {
			if(P_Pack->SetupObject2(CliAgt.ExtObjectID)) {
				setCtrlLong(CTLSEL_BILL_OBJ2, P_Pack->Rec.Object2);
			}
		}
		if(Flags & fCheckCreditLim && to_force_update) {
			int    has_matured_debt = 0;
			CurrDebt = 0.0;
			CDebtList.clear();
			if(sob.State & PPBillPacket::SetupObjectBlock::stHasCliAgreement) {
				ushort v;
				double dis;
				if(!(P_Pack->Rec.Flags & BILLF_TOTALDISCOUNT)) {
					dis = CliAgt.Dscnt;
					v   = BIN(dis != 0.0);
					setDiscount(dis, v);
					setCtrlUInt16(CTL_BILL_TTLDISCOUNT, v);
					SetupDiscountCtrls();
					calcAmounts(&dis);
				}
				if(client_id && (CliAgt.MaxCredit > 0.0 || P_BObj->Cfg.Flags & BCF_WARNMATCLIDEBT) && !(CliAgt.Flags & AGTF_DONTCALCDEBTINBILL)) {
					int * p_has_matured_debt = (P_BObj->Cfg.Flags & BCF_WARNMATCLIDEBT) ? &has_matured_debt : 0;
					DateRange cdp;
					PPObjBill::DebtBlock blk;
					P_BObj->CalcClientDebt(client_id, P_BObj->GetDefaultClientDebtPeriod(cdp), 1, blk);
					ASSIGN_PTR(p_has_matured_debt, blk.HasMatured);
					CurrDebt = blk.Debt;
					blk.GetDimList(CDebtList);
				}
			}
			if(has_matured_debt)
				PPMessage(mfInfo|mfOK, PPINF_CLIHASMATDEBT, add_msg.Z().Cat(CurrDebt, SFMT_MONEY));
		}
		if(Flags & fCheckAgreement && (P_BObj->Cfg.Flags & BCF_WARNAGREEMENT) && client_id) {
			if(!(sob.State & PPBillPacket::SetupObjectBlock::stHasCliAgreement) || sob.CliAgt.Flags & AGTF_DEFAULT)
				PPMessage(mfInfo|mfOK, PPINF_ARHASNTAGREEMENT, sob.Name);
			else if(sob.CliAgt.Expiry < P_Pack->Rec.Dt) {
				add_msg.Z().Cat(sob.Name).CatDiv('-', 1).Cat(sob.CliAgt.Expiry, DATF_DMY|DATF_CENTURY);
				PPMessage(mfInfo|mfOK, PPINF_ARHASEXPIREDAGREEMENT, add_msg);
			}
		}
		if(to_force_update && PaymTerm >= 0) {
			if(P_Pack->Rec.Flags & BILLF_NEEDPAYMENT || P_Pack->OpTypeID == PPOPT_GOODSORDER) {
				P_Pack->SetupDefaultPayDate(PaymTerm, PayDateBase);
				SetupPaymDateCtrls();
			}
		}
		if(P_Pack->P_PaymOrder && !force && sob.PsnID != P_Pack->P_PaymOrder->RcvrID) {
			if(PPMessage(mfConf|mfYesNo, PPCFM_UPDATEBNKPAYMRCVR) == cmYes)
				editPaymOrder(1);
		}
		if(sob.RegInfoList.getCount()) {
			PPObjRegister reg_obj;
			RegisterTbl::Rec reg_rec;
			StringSet ss(SLBColumnDelim);
			TDialog * dlg = new TDialog(DLG_EXPIRYREG);
			if(CheckDialogPtr(&dlg)) {
				SetupStrListBox(dlg, CTL_EXPIRYREG_LIST);
				SmartListBox * p_list = (SmartListBox *)dlg->getCtrlView(CTL_EXPIRYREG_LIST);
				if(p_list) {
					p_list->freeAll();
					for(uint i = 0; i < sob.RegInfoList.getCount(); i++) {
						const LAssoc & r_item = sob.RegInfoList.at(i);
						GetRegisterTypeName(r_item.Key, add_msg);
						ss.Z().add(add_msg);
						if(r_item.Val && reg_obj.Search(r_item.Val, &reg_rec) > 0) {
							ss.add(reg_rec.Serial);
							ss.add(reg_rec.Num);
						}
						else {
							ss.add(0);
							ss.add(PPGetWord(PPWORD_ABSENCE, 0, add_msg));
						}
						p_list->addItem(i+1, ss.getBuf());
					}
					p_list->Draw_();
					GetPersonName(sob.PsnID, add_msg);
					dlg->setStaticText(CTL_EXPIRYREG_PSNTEXT, add_msg);
				}
				ExecViewAndDestroy(dlg);
			}
		}
		if(client_id && P_Pack->OpTypeID == PPOPT_GOODSRECEIPT && P_Pack->AccSheetID == GetSupplAccSheet() && P_Pack->GetTCount()) {
			int is_there_force_suppl = 0;
			uint i;
			for(i = 0; !is_there_force_suppl && i < P_Pack->GetTCount(); i++) {
				PPTransferItem & r_ti = P_Pack->TI(i);
				if(r_ti.Flags & PPTFR_FORCESUPPL)
					is_there_force_suppl = 1;
			}
			if(is_there_force_suppl && CONFIRM(PPCFM_REPLCTIFORCESUPPL)) {
				for(i = 0; i < P_Pack->GetTCount(); i++) {
					PPTransferItem & r_ti = P_Pack->TI(i);
					if(r_ti.Flags & PPTFR_FORCESUPPL)
						r_ti.Flags &= ~PPTFR_FORCESUPPL;
				}
			}
		}
	}
	else {
		if(PPErrCode == PPERR_DENYSTOPPEDAR)
			P_Pack->Rec.Object = 0;
		PPError();
	}
	if(P_Pack->Rec.Object != client_id)
		setCtrlLong(CTLSEL_BILL_OBJECT, P_Pack->Rec.Object);
	setupParentOfContragent();
	setupDebtText();
	SetupAgreementButton();
}

int BillDialog::setAdvanceRepData(PPAdvanceRep * pAR)
{
	if(pAR) {
		setCtrlData(CTL_BILL_ADV_INREST,      &pAR->InRest);
		setCtrlData(CTL_BILL_ADV_RCPTXT1,     pAR->Rcp[0].Text);
		setCtrlData(CTL_BILL_ADV_RCPDT1,      &pAR->Rcp[0].Dt);
		setCtrlData(CTL_BILL_ADV_RCPAMT1,     &pAR->Rcp[0].Amount);
		setCtrlData(CTL_BILL_ADV_RCPTXT2,     pAR->Rcp[1].Text);
		setCtrlData(CTL_BILL_ADV_RCPDT2,      &pAR->Rcp[1].Dt);
		setCtrlData(CTL_BILL_ADV_RCPAMT2,     &pAR->Rcp[1].Amount);
		setCtrlData(CTL_BILL_ADV_NUMADDBILL,  &pAR->NumAddedBills);
		setCtrlData(CTL_BILL_ADV_NUMADDSHEET, &pAR->NumAddedSheets);
		setupAdvanceRepTotal(pAR);
	}
	return 1;
}

int BillDialog::getAdvanceRepData(PPAdvanceRep * pAR)
{
	if(pAR) {
		getCtrlData(CTL_BILL_ADV_INREST,      &pAR->InRest);
		getCtrlData(CTL_BILL_ADV_TOUT,        &pAR->ExpAmount);
		getCtrlData(CTL_BILL_ADV_RCPTXT1,     pAR->Rcp[0].Text);
		getCtrlData(CTL_BILL_ADV_RCPDT1,      &pAR->Rcp[0].Dt);
		getCtrlData(CTL_BILL_ADV_RCPAMT1,     &pAR->Rcp[0].Amount);
		getCtrlData(CTL_BILL_ADV_RCPTXT2,     pAR->Rcp[1].Text);
		getCtrlData(CTL_BILL_ADV_RCPDT2,      &pAR->Rcp[1].Dt);
		getCtrlData(CTL_BILL_ADV_RCPAMT2,     &pAR->Rcp[1].Amount);
		getCtrlData(CTL_BILL_ADV_NUMADDBILL,  &pAR->NumAddedBills);
		getCtrlData(CTL_BILL_ADV_NUMADDSHEET, &pAR->NumAddedSheets);
		pAR->RcpAmount = pAR->Rcp[0].Amount + pAR->Rcp[1].Amount;
		pAR->OutRest = pAR->InRest + pAR->RcpAmount - pAR->ExpAmount;
	}
	return 1;
}

int BillDialog::setupAdvanceRepTotal(const PPAdvanceRep * pAR)
{
	double in = getCtrlReal(CTL_BILL_ADV_INREST);
	in += (getCtrlReal(CTL_BILL_ADV_RCPAMT1) + getCtrlReal(CTL_BILL_ADV_RCPAMT2));
	setCtrlReal(CTL_BILL_ADV_TIN,   in);
	double expend = pAR ? pAR->ExpAmount : 0.0;
	setCtrlReal(CTL_BILL_ADV_TOUT,  expend);
	setCtrlReal(CTL_BILL_ADV_TREST, in - expend);
	return 1;
}

void BillDialog::setupHiddenButton(long opflag, uint cm, uint ctlId)
{
	int    s = BIN(opflag && CheckOpFlags(P_Pack->Rec.OpID, opflag));
	enableCommand(cm, s);
	showCtrl(ctlId, s);
}

int BillDialog::setupDebt()
{
	int    ok = 1;
	if(P_Pack->Rec.LinkBillID && IsOpPaym(P_Pack->Rec.OpID)) {
		double amt, tmp;
		THROW(P_BObj->P_Tbl->GetAmount(P_Pack->Rec.LinkBillID, PPAMT_MAIN,    P_Pack->Rec.CurID, &amt));
		THROW(P_BObj->P_Tbl->GetAmount(P_Pack->Rec.LinkBillID, PPAMT_PAYMENT, P_Pack->Rec.CurID, &tmp));
		setCtrlReal(CTL_BILL_DEBTSUM, amt - tmp);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

void BillDialog::SetupPaymDateCtrls()
{
	if(P_Pack) {
		LDATE  dt = ZERODATE;
		P_Pack->GetLastPayDate(&dt);
		setCtrlData(CTL_BILL_PAYDATE, &dt);
		PPOprKind op_rec;
		GetOpData(P_Pack->Rec.OpID, &op_rec);
		if(P_Pack->Rec.Flags & BILLF_NEEDPAYMENT || P_Pack->OpTypeID == PPOPT_GOODSORDER || op_rec.SubType == OPSUBT_WARRANT) {
			disableCtrls(P_Pack->Pays.getCount() > 1, CTL_BILL_PAYDATE, CTLCAL_BILL_PAYDATE, 0);
			enableCommand(cmBillPayPlan, 1);
		}
		else {
			disableCtrls(1, CTL_BILL_PAYDATE, CTLCAL_BILL_PAYDATE, 0);
			enableCommand(cmBillPayPlan, 0);
		}
	}
}

int BillDialog::setDTS(PPBillPacket * pPack)
{
	int    ok = 1;
	uint   i, ctl;
	double dis = 0.0;
	ushort v;
	AmtEntry * ae;
	int    dsbl_object = 0, dsbl_object2 = 0;
	PPOprKindPacket op_pack;
	PPObjOprKind    opkobj;
	PPID   id = 0;
	DateIter di;
	BillTbl::Rec z_link_rec;
	int    disabled_cntrag = 0;
	SString temp_buf;
	P_Pack   = pPack;
	Pattern  = pPack->Rec;
	Flags &= ~(fModified|fCheckCreditLim|fCheckRetLim);
	THROW(opkobj.GetPacket(P_Pack->Rec.OpID, &op_pack) > 0);
	{
		temp_buf.Z().Cat(op_pack.Rec.Name).CatDiv(';', 1);
		GetObjectName(PPOBJ_LOCATION, P_Pack->Rec.LocID, temp_buf, 1);
		setTitle(temp_buf);
		GetObjectName(PPOBJ_BILLSTATUS, P_Pack->Rec.StatusID, temp_buf);
		setStaticText(CTL_BILL_STATUS, temp_buf);
	}
	if(op_pack.Amounts.getCount()) {
		ExtAmtIDList.copy(op_pack.Amounts);
		Flags |= fHasAmtIDList;
	}
	else {
		ExtAmtIDList.freeAll();
		Flags &= ~fHasAmtIDList;
	}
	enableCommand(cmExAmountList, BIN(Flags & fHasAmtIDList));
	if(P_Pack->OpTypeID == PPOPT_PAYMENT) {
		if(P_Pack->Rec.LinkBillID == 0) {
			PPLoadString("but_dolinkbill", temp_buf);
			setButtonText(cmLinkedBill, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
		}
		else
			enableCommand(cmLinkedBill, 1);
	}
	else
		enableCommand(cmLinkedBill, BIN(P_Pack->Rec.LinkBillID));
	enableCommand(cmReckonBill, BIN(P_Pack->PaymBillID));
	{
		const int sqs = oneof2(P_Pack->Rec.OpID, PPOPK_EDI_STOCK, PPOPK_EDI_SHOPCHARGEON) ? -1 : 0; // @v9.3.1
		P_Pack->SetQuantitySign(sqs);
	}
	setCtrlData(CTL_BILL_DOC,  P_Pack->Rec.Code);
	setCtrlData(CTL_BILL_DATE, &P_Pack->Rec.Dt);
	setCtrlData(CTL_BILL_DUEDATE, &P_Pack->Rec.DueDate);
	{
		DateRange period;
		period.Set(P_Pack->Rec.PeriodLow, P_Pack->Rec.PeriodUpp);
		SetPeriodInput(this, CTL_BILL_PERIOD, &period);
	}
	if(oneof4(P_Pack->OpTypeID, PPOPT_ACCTURN, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT)) {
		SetupPPObjCombo(this, CTLSEL_BILL_LOCATION, PPOBJ_LOCATION, P_Pack->Rec.LocID, 0);
	}
	else {
		showCtrl(CTL_BILL_LOCATION, 0);
		showCtrl(CTLSEL_BILL_LOCATION, 0);
		disableCtrl(CTLSEL_BILL_LOCATION, 1);
	}
	if(P_Pack->AccSheetID) {
		if(P_Pack->Rec.Flags & BILLF_GEXPEND || oneof2(P_Pack->OpTypeID, PPOPT_DRAFTEXPEND, PPOPT_GOODSORDER))
			Flags |= fCheckCreditLim;
		if((op_pack.Rec.OpTypeID == PPOPT_GOODSRETURN && IsExpendOp(P_Pack->Rec.OpID) == 0) ||
			(op_pack.Rec.OpTypeID == PPOPT_GOODSRECEIPT && op_pack.Rec.ExtFlags & OPKFX_UNLINKRET)) {
			Flags |= fCheckRetLim;
		}
		SetupArCombo(this, CTLSEL_BILL_OBJECT, P_Pack->Rec.Object,  OLW_LOADDEFONOPEN|OLW_CANINSERT, P_Pack->AccSheetID, sacfNonGeneric);
		SetupArCombo(this, CTLSEL_BILL_PAYER,  P_Pack->Ext.PayerID, OLW_LOADDEFONOPEN|OLW_CANINSERT, P_Pack->AccSheetID, sacfNonGeneric);
		if(P_Pack->Rec.LinkBillID || (P_Pack->Rec.ID && !P_BObj->CheckRights(BILLOPRT_MODOBJ, 1)))
			dsbl_object = 1;
	}
	else
		dsbl_object = 1;
	SetupAgreementButton();
	if(op_pack.Rec.AccSheet2ID) {
		PPClientAgreement ca_rec;
		SETFLAG(Flags, fSetupObj2ByCliAgt, ArObj.GetClientAgreement(0, &ca_rec) > 0 && ca_rec.ExtObjectID == op_pack.Rec.AccSheet2ID);
		SetupArCombo(this, CTLSEL_BILL_OBJ2, P_Pack->Rec.Object2, /*OLW_LOADDEFONOPEN|*/OLW_CANINSERT, op_pack.Rec.AccSheet2ID, sacfNonGeneric);
		op_pack.GetExtStrData(OPKEXSTR_OBJ2NAME, temp_buf);
		setStaticText(CTL_BILL_OBJ2NAME, temp_buf);
	}
	else
		dsbl_object2 = 1;
	disableCtrls(dsbl_object2, CTL_BILL_OBJ2, CTLSEL_BILL_OBJ2, CTL_BILL_OBJ2NAME, 0);
	showCtrl(CTL_BILL_OBJ2,     !dsbl_object2);
	showCtrl(CTLSEL_BILL_OBJ2,  !dsbl_object2);
	showCtrl(CTL_BILL_OBJ2NAME, !dsbl_object2);
	showCtrl(CTL_BILL_DUEDATE,  !oneof2(P_Pack->OpTypeID, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT));
	ShowCalCtrl(CTLCAL_BILL_DUEDATE, this, !oneof2(P_Pack->OpTypeID, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT));
	if(P_Pack->Rec.SCardID) {
		PPObjSCard sc_obj;
		SCardTbl::Rec sc_rec;
		if(sc_obj.Search(P_Pack->Rec.SCardID, &sc_rec) > 0) {
			SString info_buf;
			if(sc_obj.IsCreditCard(sc_rec.ID) > 0) {
				double rest = 0.0;
				sc_obj.P_Tbl->GetRest(sc_rec.ID, P_Pack->Rec.Dt, &rest);
				PPLoadString("crdcard", temp_buf);
				info_buf.Cat(temp_buf).CatDiv(':', 2).Cat(sc_rec.Code).CatDiv('.', 2);
				PPLoadString("rest", temp_buf);
				info_buf.Cat(temp_buf).CatDiv('=', 1).Cat(rest);
			}
			else {
				PPLoadString("discard", temp_buf);
				info_buf.Cat(temp_buf).CatDiv(':', 2).Cat(sc_rec.Code).CatDiv('.', 2);
				PPLoadString("discount", temp_buf);
				info_buf.Cat(temp_buf).CatDiv('=', 1).Cat(fdiv100i(sc_rec.PDis)).CatChar('%');
			}
			setStaticText(CTL_BILL_ST_SCARD, info_buf);
		}
	}
	{
		int r = setupDebt();
		THROW(r);
		if(r > 0)
			dsbl_object = false;
	}
	disableCtrl(CTLSEL_BILL_OBJECT, dsbl_object);
	disableCtrl(CTL_BILL_DEBTSUM, 1);
	if(Flags & fExtMainCurAmount)
		setCurGroupData();
	else
		setCtrlReal(CTL_BILL_AMOUNT, P_Pack->GetAmount());
	v = BIN(P_Pack->Rec.Flags & BILLF_TOTALDISCOUNT);
	Flags &= ~fPctDis;
	for(i = 0; P_Pack->Amounts.enumItems(&i, (void**)&ae);) {
		ctl = 0;
		switch(ae->AmtTypeID) {
			case PPAMT_BUYING:  ctl = CTL_BILL_COST;  break;
			case PPAMT_SELLING: ctl = CTL_BILL_PRICE; break;
			case PPAMT_MANDIS:
				if(!(Flags & fPctDis))
					dis = ae->Amt;
				break;
			case PPAMT_PCTDIS:
				if(v) {
					Flags |= fPctDis;
					dis = ae->Amt;
				}
				break;
		}
		setCtrlData(ctl, &ae->Amt);
	}
	setDiscount(dis, BIN(Flags & fPctDis));
	setCtrlData(CTL_BILL_TTLDISCOUNT, &v);
	SetupDiscountCtrls();
	if(P_Pack->OpTypeID == PPOPT_GOODSRECEIPT) {
		setCtrlUInt16(CTL_BILL_TOGGLESTAX, BIN(P_Pack->Rec.Flags & BILLF_RMVEXCISE));
		AddClusterAssoc(CTL_BILL_TOGGLESTAX, 0, BILLF_RMVEXCISE);
		SetClusterData(CTL_BILL_TOGGLESTAX, P_Pack->Rec.Flags);
	}
	SetupPaymDateCtrls();
	if(P_Pack->OpTypeID == PPOPT_GOODSRETURN)
		disableCtrls(1, CTLSEL_BILL_OBJECT, CTL_BILL_AMOUNT, CTL_BILL_PAYDATE, CTLCAL_BILL_PAYDATE, 0);
	if(!(Flags & fEditMode))
		P_BObj->SubstMemo(P_Pack);
	setCtrlData(CTL_BILL_MEMO, P_Pack->Rec.Memo);
	setupHiddenButton(OPKF_RENT,        cmRentCondition, CTL_BILL_RENTBUTTON);
	setupHiddenButton(OPKF_BANKING,     cmPaymOrder,     CTL_BILL_PAYMORDBUTTON);
	if(CheckOpFlags(P_Pack->Rec.OpID, OPKF_BANKING)) {
		showCtrl(CTL_BILL_PAYMORD_IND, 1);
		SetCtrlBitmap(CTL_BILL_PAYMORD_IND, P_Pack->P_PaymOrder ? BM_GREEN : BM_RED);
	}
	else
		showCtrl(CTL_BILL_PAYMORD_IND, 0);
	setupHiddenButton(P_BObj->CheckRights(BILLOPRT_MODFREIGHT, 1) ? OPKF_FREIGHT : 0, cmBillFreight, CTL_BILL_FREIGHTBUTTON);
	setupHiddenButton(OPKF_ATTACHFILES, cmLinkFiles,   CTL_BILL_LINKFILESBUTTON);
	if(P_Pack->OpTypeID == PPOPT_GOODSORDER) {
		AddClusterAssoc(CTL_BILL_CLOSETAG, 0, BILLF_CLOSEDORDER);
		SetClusterData(CTL_BILL_CLOSETAG, P_Pack->Rec.Flags);
	}
	if(P_Pack->Rec.Flags & BILLF_ADVANCEREP && P_Pack->P_AdvRep)
		THROW(setAdvanceRepData(P_Pack->P_AdvRep));
	selectCtrl(CTL_BILL_DOC);
	while(!disabled_cntrag && P_Pack->Rec.ID && P_BObj->P_Tbl->EnumLinks(P_Pack->Rec.ID, &di, BLNK_PAYMENT, &z_link_rec) > 0)
		if(z_link_rec.ID && P_BObj->IsMemberOfPool(z_link_rec.ID, PPASS_PAYMBILLPOOL, &id) && id)
			disabled_cntrag = 1;
	while(!disabled_cntrag && P_Pack->Rec.ID && P_BObj->EnumMembersOfPool(PPASS_PAYMBILLPOOL, P_Pack->Rec.ID, &id) > 0)
		if(P_BObj->Search(id, &z_link_rec) > 0 && GetOpType(z_link_rec.OpID) == PPOPT_PAYMENT)
			if(P_BObj->Search(z_link_rec.LinkBillID) > 0)
				disabled_cntrag = 1;
	disableCtrl(CTLSEL_BILL_OBJECT, disabled_cntrag);
	if(op_pack.Rec.ExtFlags & OPKFX_CANBEDECLINED) {
		AddClusterAssoc(CTL_BILL_DECLINE, 0, BILLF2_DECLINED);
		SetClusterData(CTL_BILL_DECLINE, P_Pack->Rec.Flags2);
		showCtrl(CTL_BILL_DECLINE, 1);
		disableCtrl(CTL_BILL_DECLINE, !P_BObj->CheckRights(BILLOPRT_REJECT, 1)); // @v9.0.1
	}
	else
		showCtrl(CTL_BILL_DECLINE, 0);
	showCtrl(CTL_BILL_EDIACKRESP, 0);
	showCtrl(CTL_BILL_EDIACKSTATUS, 0);
	showButton(cmEdiAckBill, 0);
	if(getCtrlView(CTL_BILL_EDIACKRESP)) {
		const int recadv_status = BillCore::GetRecadvStatus(P_Pack->Rec);
        if(recadv_status) {
			int    recadv_conf_status = BillCore::GetRecadvConfStatus(P_Pack->Rec);
			AddClusterAssocDef(CTL_BILL_EDIACKRESP, 0, PPEDI_RECADVCONF_STATUS_UNDEF);
			AddClusterAssoc(CTL_BILL_EDIACKRESP, 1, PPEDI_RECADVCONF_STATUS_ACCEPT);
			AddClusterAssoc(CTL_BILL_EDIACKRESP, 2, PPEDI_RECADVCONF_STATUS_REJECT);
			SetClusterData(CTL_BILL_EDIACKRESP, recadv_conf_status);
			{
				SString recadv_status_text;
				if(recadv_status == PPEDI_RECADV_STATUS_ACCEPT)
					PPLoadString("accepted", recadv_status_text);
				else if(recadv_status == PPEDI_RECADV_STATUS_PARTACCEPT)
					PPLoadString("disaccord", recadv_status_text);
				else if(recadv_status == PPEDI_RECADV_STATUS_REJECT)
					PPLoadString("rejected", recadv_status_text);
				setStaticText(CTL_BILL_EDIACKSTATUS, recadv_status_text);
			}
			showCtrl(CTL_BILL_EDIACKRESP, 1);
			showCtrl(CTL_BILL_EDIACKSTATUS, 1);
			if(recadv_status == PPEDI_RECADV_STATUS_PARTACCEPT)
				showButton(cmEdiAckBill, 1);
        }
	}
	if(P_Pack->Rec.ID == 0 && P_Pack->Rec.Object)
		ReplyCntragntSelection(1);
	else {
		setupByCntragnt();
		setupParentOfContragent();
	}
	CATCHZOK
	return ok;
}

int BillDialog::calcAmounts(double * pAmount)
{
	int      ok = 1;
	int      rmvexcise = 0;
	double   amt = 0.0;
	TView  * vw  = 0;
	AmtList  al;
	ushort   v = getCtrlUInt16(CTL_BILL_TTLDISCOUNT);
	SetupDiscountCtrls();
	if(v) {
		P_Pack->Rec.Flags |= BILLF_TOTALDISCOUNT;
		int   _pctdis = BIN(Flags & fPctDis);
		getDiscount(&amt, &_pctdis, &rmvexcise);
		SETFLAG(Flags, fPctDis, _pctdis);
		P_Pack->SetTotalDiscount(amt, _pctdis, rmvexcise);
	}
	else {
		Flags &= ~fPctDis;
		if(P_Pack->Rec.Flags & BILLF_TOTALDISCOUNT) {
			P_Pack->SetTotalDiscount(0, 0, 0);
			P_Pack->Rec.Flags &= ~BILLF_TOTALDISCOUNT;
		}
	}
	// @v9.4.3 if(!(P_Pack->Rec.Flags & BILLF_GOODS) && !P_Pack->IsDraft() && !CheckOpFlags(P_Pack->Rec.OpID, OPKF_ADVACC)) {
	if(!P_Pack->IsGoodsDetail() && !CheckOpFlags(P_Pack->Rec.OpID, OPKF_ADVACC)) { // @v9.4.3
		if(getCtrlData(CTL_BILL_AMOUNT, &amt))
			al.Put(PPAMT_MAIN, P_Pack->Rec.CurID, amt, 0, 0);
	}
	THROW(P_Pack->SumAmounts(&al));
	if(!(P_Pack->Rec.Flags & BILLF_TOTALDISCOUNT)) {
		al.Get(PPAMT_DISCOUNT, P_Pack->Rec.CurID, &amt);
		setDiscount(amt, 0);
	}
	THROW(P_Pack->InitAmounts(&al));
	al.Get(PPAMT_MAIN, P_Pack->Rec.CurID, &amt);
	if(P_Pack->Rec.Flags & BILLF_ADVANCEREP && P_Pack->P_AdvRep)
		P_Pack->P_AdvRep->ExpAmount = amt;
	if((vw = getCtrlView(CTL_BILL_AMOUNT)) != 0 && vw->IsInState(sfDisabled))
		setCtrlReal(CTL_BILL_AMOUNT, amt);
	if((vw = getCtrlView(CTL_BILL_ADV_TOUT)) != 0 && vw->IsInState(sfDisabled)) {
		setCtrlReal(CTL_BILL_ADV_TOUT, amt);
		setupAdvanceRepTotal(P_Pack->P_AdvRep);
	}
	CATCH
		amt = 0.0;
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pAmount, amt);
	return ok;
}

double BillDialog::getCurrentDebt(PPID debtDimID) const
{
	//
	// Если задана долговая размерность, то текущий долг береться точно по ней.
	// Таким образом, при debtDimID != 0 и отсутствии в списке CDebtList
	// значения текущего долга по этой размерности считаем, что долг нулевой (а не равный общему долгу CurrDebt).
	//
	return debtDimID ? CDebtList.Get(debtDimID) : CurrDebt;
}

int PPObjBill::CalcReturnPart(PPID arID, const DateRange & rPeriod, double * pShipment, double * pRet)
{
	int    ok = 1;
	double shipment = 0.0;
	double ret = 0.0;
	PPIDArray shpm_op_list;
	PPIDArray ret_op_list;
	PPOprKind op_rec, link_op_rec;
	ArticleTbl::Rec ar_rec;
	if(ArObj.Fetch(arID, &ar_rec) > 0) {
		for(SEnum en = PPRef->Enum(PPOBJ_OPRKIND, 0); ok && en.Next(&op_rec) > 0;) {
			if(op_rec.OpTypeID == PPOPT_GOODSRETURN && op_rec.LinkOpID) {
				if(GetOpData(op_rec.LinkOpID, &link_op_rec) > 0 && link_op_rec.AccSheetID == ar_rec.AccSheetID && link_op_rec.OpTypeID == PPOPT_GOODSEXPEND) {
					ret_op_list.add(op_rec.ID);
				}
			}
			else if(op_rec.AccSheetID == ar_rec.AccSheetID) {
				if(op_rec.OpTypeID == PPOPT_GOODSEXPEND) {
					shpm_op_list.add(op_rec.ID);
				}
				else if(op_rec.OpTypeID == PPOPT_GOODSRECEIPT && op_rec.ExtFlags & OPKFX_UNLINKRET) {
					ret_op_list.add(op_rec.ID);
				}
			}
		}
		if(shpm_op_list.getCount() || ret_op_list.getCount()) {
			shpm_op_list.sort();
			ret_op_list.sort();
			BillTbl * p_tbl = P_Tbl;
			BillTbl::Key3 k3;
			BExtQuery q(p_tbl, 3, 128);
			q.select(p_tbl->ID, p_tbl->OpID, p_tbl->Object, p_tbl->LinkBillID, p_tbl->Amount, 0L).
				where(p_tbl->Object == arID && daterange(p_tbl->Dt, &rPeriod));
			k3.Object = arID;
			k3.Dt   = rPeriod.low;
			k3.BillNo = 0;
			for(q.initIteration(0, &k3, spGt); q.nextIteration() > 0;) {
				if(shpm_op_list.bsearch(p_tbl->data.OpID)) {
					shipment += p_tbl->data.Amount;
				}
				else if(ret_op_list.bsearch(p_tbl->data.OpID)) {
					ret += p_tbl->data.Amount;
				}
			}
		}
	}
	ASSIGN_PTR(pShipment, shipment);
	ASSIGN_PTR(pRet, ret);
	return ok;
}

int BillDialog::checkCreditOverflow(double amt)
{
	int    ok = 1;
	PPID   debt_dim = 0;
	P_Pack->GetDebtDim(&debt_dim);
	double limit = CliAgt.GetCreditLimit(debt_dim);
	if(Flags & fCheckCreditLim && limit > 0.0) {
		double curr_debt = getCurrentDebt(debt_dim);
		if((curr_debt + amt) > limit) {
			SString add_msg;
			add_msg.Cat(curr_debt, SFMT_MONEY).CatChar('+').Cat(amt, SFMT_MONEY).
				Eq().Cat(curr_debt+amt, SFMT_MONEY);
			ok = (PPMessage(mfConf|mfYesNo, PPCFM_CLICRDOVERFLOW, add_msg) == cmYes) ? 1 : PPSetError(PPERR_CLICRDOVERFLOW);
		}
	}
	if(ok && Flags & fCheckRetLim && CliAgt.RetLimPrd && CliAgt.RetLimPart > 0) {
		DateRange period;
		if(period.SetPeriod(P_Pack->Rec.Dt, CliAgt.RetLimPrd)) {
			double shipment = 0.0, ret = 0.0;
			P_BObj->CalcReturnPart(CliAgt.ClientID, period, &shipment, &ret);
			if(shipment == 0.0 || (((ret+amt) / shipment) * 10000.0) > (double)CliAgt.RetLimPart)
				ok = PPSetError(PPERR_CLIRETLIMOVERFLOW);
		}
	}
	return ok;
}

int BillDialog::setCurGroupData()
{
	if(Flags & fExtMainCurAmount) {
		CurAmtCtrlGroup::Rec ca_cg_rec;
		ca_cg_rec.CurID  = P_Pack->Rec.CurID;
		ca_cg_rec.Amount = P_Pack->GetAmount();
		ca_cg_rec.CRate  = P_Pack->Amounts.Get(PPAMT_CRATE, P_Pack->Rec.CurID);
		setGroupData(GRP_CURAMT, &ca_cg_rec);
		disableCtrl(CTLSEL_BILL_CUR, BIN(P_Pack->Rec.ID && P_Pack->GetTCount()));
		return 1;
	}
	else
		return -1;
}

int BillDialog::getCurGroupData()
{
	CurAmtCtrlGroup::Rec ca_cg_rec;
	if(Flags & fExtMainCurAmount) {
		MEMSZERO(ca_cg_rec);
		getGroupData(GRP_CURAMT, &ca_cg_rec);
		if(!(P_Pack->Rec.Flags & BILLF_FIXEDAMOUNTS)) {
			P_Pack->Rec.CurID  = ca_cg_rec.CurID;
			P_Pack->Rec.Amount = BR2(ca_cg_rec.Amount);
			if(P_Pack->Rec.CurID)
				P_Pack->Amounts.Put(PPAMT_CRATE, P_Pack->Rec.CurID, ca_cg_rec.CRate, 0, 1);
		}
		return 1;
	}
	else
		return -1;
}

int BillDialog::getDTS(int onCancel)
{
	int    ok = 1, intr = 0, r;
	double amt = 0.0;
	LDATE  dt;
	PPObjOprKind op_obj;
	getCurGroupData();
	if(P_Pack->OpTypeID == PPOPT_GOODSMODIF)
		P_Pack->CalcModifCost();
	else if(P_Pack->OpTypeID == PPOPT_GOODSRECEIPT) {
		PPTransferItem * p_ti;
		GetClusterData(CTL_BILL_TOGGLESTAX, &P_Pack->Rec.Flags);
		for(uint i = 0; P_Pack->EnumTItems(&i, &p_ti);)
			SETFLAG(p_ti->Flags, PPTFR_COSTWSTAX, BIN(P_Pack->Rec.Flags & BILLF_RMVEXCISE));
	}
	getCtrlData(CTL_BILL_DOC,  P_Pack->Rec.Code);
	getCtrlData(CTL_BILL_DATE, &P_Pack->Rec.Dt);
	P_Pack->Rec.Dt = P_Pack->Rec.Dt.getactual(ZERODATE);
	getCtrlData(CTL_BILL_DUEDATE, &P_Pack->Rec.DueDate);
	P_Pack->Rec.DueDate = P_Pack->Rec.DueDate.getactual(ZERODATE);
	{
		DateRange period;
		THROW(r = GetPeriodInput(this, CTL_BILL_PERIOD, &period));
		P_Pack->Rec.PeriodLow = (r > 0) ? period.low : ZERODATE;
		P_Pack->Rec.PeriodUpp = (r > 0) ? period.upp : ZERODATE;
	}
	if(oneof4(P_Pack->OpTypeID, PPOPT_ACCTURN, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT))
		getCtrlData(CTLSEL_BILL_LOCATION, &P_Pack->Rec.LocID);
	getCtrlData(CTLSEL_BILL_OBJECT, &P_Pack->Rec.Object);
	getCtrlData(CTLSEL_BILL_OBJ2,   &P_Pack->Rec.Object2);
	getCtrlData(CTLSEL_BILL_PAYER,  &P_Pack->Ext.PayerID);
	THROW(calcAmounts(&amt));
	if(!onCancel)
		THROW(checkCreditOverflow(amt));
	if(P_Pack->OpTypeID == PPOPT_GOODSORDER)
		GetClusterData(CTL_BILL_CLOSETAG, &P_Pack->Rec.Flags);
	if(P_Pack->Pays.getCount() <= 1) {
		P_Pack->Pays.freeAll();
		if(getCtrlData(CTL_BILL_PAYDATE, &(dt = ZERODATE)) && dt) {
			THROW_SL(checkdate(dt));
			P_Pack->AddPayDate(dt, amt);
		}
	}
	getCtrlData(CTL_BILL_MEMO, P_Pack->Rec.Memo);
	if(P_Pack->Rec.Flags & BILLF_ADVANCEREP && P_Pack->P_AdvRep)
		THROW(getAdvanceRepData(P_Pack->P_AdvRep));
	intr = IsIntrOp(P_Pack->Rec.OpID);
	if(!intr && P_Pack->IsDraft()) {
		PPDraftOpEx doe;
		if(op_obj.GetDraftExData(P_Pack->Rec.OpID, &doe) > 0)
			intr = IsIntrOp(doe.WrOffOpID);
	}
	if(intr) {
		PPID   prim_sheet_id    = LConfig.LocAccSheetID;
		PPID   foreign_sheet_id = P_Pack->AccSheetID;
		THROW_PP(P_Pack->Rec.Object || intr != INTREXPND, PPERR_INTRDESTNEEDED);
		if(prim_sheet_id && prim_sheet_id == foreign_sheet_id) {
			const PPID prim_obj_id = PPObjLocation::WarehouseToObj(P_Pack->Rec.LocID);
			THROW_PP(!prim_obj_id || prim_obj_id != P_Pack->Rec.Object, PPERR_PRIMEQFOREIN);
		}
	}
	P_Pack->SetQuantitySign(-1);
	// @v9.3.1 /* @v9.2.11 Снимаем запрет на проведение пустых товарных документов
	if(oneof5(P_Pack->OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND,
		PPOPT_GOODSRETURN, PPOPT_GOODSREVAL, PPOPT_GOODSORDER) && !PPMaster) {
		THROW_PP(P_Pack->GetTCount() || P_BObj->CheckRights(BILLOPRT_EMPTY, 1), PPERR_EMPTYGOODSLIST); // @v9.3.1 BILLOPRT_EMPTY
	}
	// @v9.3.1 */
	{
		PPOprKind op_rec;
		GetOpData(P_Pack->Rec.OpID, &op_rec);
		if(op_rec.ExtFlags & OPKFX_CANBEDECLINED) {
			GetClusterData(CTL_BILL_DECLINE, &P_Pack->Rec.Flags2);
		}
	}
	if(getCtrlView(CTL_BILL_EDIACKRESP)) {
        if(P_Pack->Rec.Flags2 & (BILLF2_RECADV_ACCP|BILLF2_RECADV_DECL)) {
        	int recadv_conf_status = GetClusterData(CTL_BILL_EDIACKRESP);
            BillCore::SetRecadvConfStatus(recadv_conf_status, P_Pack->Rec);
        }
	}
	THROW(P_BObj->ValidatePacket(P_Pack, 0));
	CATCHZOK
	if(strcmp(strip(P_Pack->Rec.Code), strip(Pattern.Code)) == 0)
		memcpy(Pattern.Code, P_Pack->Rec.Code, sizeof(Pattern.Code));
	if(P_Pack->GetAmount() == BR2(Pattern.Amount))
		Pattern.Amount = BR2(P_Pack->Rec.Amount);
	if(strcmp(strip(P_Pack->Rec.Memo), strip(Pattern.Memo)) == 0)
		memcpy(Pattern.Memo, P_Pack->Rec.Memo, sizeof(Pattern.Memo));
	if((Flags & fModified) || memcmp(&P_Pack->Rec, &Pattern, sizeof(Pattern)) != 0)
		Flags |= fModified;
	return ok;
}

int BillDialog::editItems()
{
	int    r = -1;
	double tmp = 0.0;
	if(P_Pack->OpTypeID == PPOPT_ACCTURN) {
		if(CheckOpFlags(P_Pack->Rec.OpID, OPKF_ADVACC)) {
			ViewAdvBillDetails(P_Pack, P_BObj);
			calcAmounts(&tmp);
			r = 1;
		}
	}
	else {
		getCurGroupData();
		calcAmounts(&tmp);
		getCtrlData(CTLSEL_BILL_OBJECT, &P_Pack->Rec.Object);
		if(P_Pack->Rec.Object == 0) {
			if(P_Pack->OpTypeID == PPOPT_GOODSRECEIPT && P_Pack->AccSheetID == GetSupplAccSheet() && P_Pack->GetTCount() == 0) {
				if(PPMessage(mfConf|mfYesNo|mfDefaultYes, PPCFM_ZERORCPTOBJ) == cmYes) {
					messageToCtrl(CTLSEL_BILL_OBJECT, cmCBActivate, 0);
					return 0;
				}
			}
			else if(P_Pack->Rec.OpID == _PPOPK_SUPPLRET)
				return (PPError(PPERR_BILLOBJNEEDED, 0), 0);
			else if(IsIntrExpndOp(P_Pack->Rec.OpID))
				return (PPError(PPERR_INTRDESTNEEDED, 0), 0);
			else if(P_Pack->IsDraft()) {
				PPObjOprKind op_obj;
				PPDraftOpEx doe;
				if(op_obj.GetDraftExData(P_Pack->Rec.OpID, &doe) > 0)
					if(IsIntrExpndOp(doe.WrOffOpID))
						return (PPError(PPERR_INTRDESTNEEDED), 0);
			}
		}
		getCtrlData(CTLSEL_BILL_OBJ2, &P_Pack->Rec.Object2);
		getCtrlData(CTL_BILL_DATE, &P_Pack->Rec.Dt);
		PPTransferItem * ti;
		for(uint i = 0; P_Pack->EnumTItems(&i, &ti);)
			ti->Date = P_Pack->Rec.Dt;
		if((r = ViewBillDetails(P_Pack, BIN(Flags & fEditMode), P_BObj)) > 0) {
			Flags |= fModified;
			calcAmounts(&tmp);
		}
	}
	return BIN(r);
}
//
// BillInfoDialog
//
static void FASTCALL SetBillFlagsCtrl(TDialog * dlg, uint ctlID, long f)
{
	dlg->AddClusterAssoc(ctlID,  0, BILLF_TOTALDISCOUNT);
	//dlg->AddClusterAssoc(ctlID,  1, BILLF_TRFRBYPRICE);
	dlg->AddClusterAssoc(ctlID,  2, BILLF_NEEDPAYMENT);
	dlg->AddClusterAssoc(ctlID,  3, BILLF_PAYOUT);
	dlg->AddClusterAssoc(ctlID,  4, BILLF_GRECEIPT);
	dlg->AddClusterAssoc(ctlID,  5, BILLF_GEXPEND);
	dlg->AddClusterAssoc(ctlID,  6, BILLF_GREVAL);
	dlg->AddClusterAssoc(ctlID,  7, BILLF_SHIPPED);
	dlg->AddClusterAssoc(ctlID,  8, BILLF_CLOSEDORDER);
	dlg->AddClusterAssoc(ctlID,  9, BILLF_CASH);
	dlg->AddClusterAssoc(ctlID, 10, BILLF_CHECK);
	dlg->AddClusterAssoc(ctlID, 11, BILLF_NOATURN);
	dlg->AddClusterAssoc(ctlID, 12, BILLF_WHITELABEL);
	dlg->AddClusterAssoc(ctlID, 13, BILLF_RMVEXCISE);
	dlg->AddClusterAssoc(ctlID, 14, BILLF_FIXEDAMOUNTS);
	dlg->SetClusterData(ctlID, f);
}

int SLAPI PPObjBill::ViewBillInfo(PPID billID)
{
	class BillInfoDialog : public AmtListDialog {
	public:
		BillInfoDialog(PPBillPacket * pPack) : AmtListDialog(DLG_BILLINFO, CTL_BILLINFO_AMTLIST, 1,
			&pPack->Amounts, 0, 0, OLW_CANINSERT|OLW_CANEDIT|OLW_CANDELETE), P_Pack(pPack)
		{
		}
		int getDTS(PPBillPacket *)
		{
			getCtrlData(CTL_BILLINFO_OMTPAYM, &P_Pack->Rec.PaymAmount);
			AmtListDialog::getDTS(&P_Pack->Amounts);
			P_Pack->Rec.Amount = BR2(P_Pack->Amounts.Get(PPAMT_MAIN, P_Pack->Rec.CurID));
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			AmtListDialog::handleEvent(event);
			if(event.isCmd(cmPrint)) {
				PView pf(P_Pack);
				PPAlddPrint(REPORT_BILLINFO, &pf);
			}
			else if(event.isCmd(cmObjSyncTab)) {
				PPObjID oid;
				ViewObjSyncTab(oid.Set(PPOBJ_BILL, P_Pack->Rec.ID));
			}
			else
				return;
			clearEvent(event);
		}
		PPBillPacket * P_Pack;
	};
	int    ok = -1;
	PPBillPacket pack;
	BillInfoDialog * dlg = 0;
	if(billID) {
		SString buf;
		S_GUID guid;
		THROW(CheckRights(BILLRT_SYSINFO));
		THROW(ExtractPacket(billID, &pack) > 0);
		{
			const long org_flags = pack.Rec.Flags;
			const long org_flags2 = pack.Rec.Flags2;
			long   recadv_status = 0;
			if(pack.Rec.Flags2 & BILLF2_RECADV_ACCP)
				recadv_status = (pack.Rec.Flags2 & BILLF2_RECADV_DECL) ? 2 : 1;
			else if(pack.Rec.Flags2 & BILLF2_RECADV_DECL)
				recadv_status = 3;
			GetGuid(billID, &guid);
			THROW(CheckDialogPtr(&(dlg = new BillInfoDialog(&pack))));
			dlg->setCtrlLong(CTL_BILLINFO_ID, pack.Rec.ID);
			dlg->setCtrlLong(CTL_BILLINFO_LINKBILLID, pack.Rec.LinkBillID);
			dlg->setCtrlData(CTL_BILLINFO_RBB, &pack.Rec.LastRByBill);
			dlg->AddClusterAssoc(CTL_BILLINFO_FLAGS2, 0, BILLF2_FULLSYNC);
			dlg->AddClusterAssoc(CTL_BILLINFO_FLAGS2, 1, BILLF2_ACKPENDING);
			dlg->SetClusterData(CTL_BILLINFO_FLAGS2, pack.Rec.Flags2);
			dlg->AddClusterAssocDef(CTL_BILLINFO_RECADV, 0, 0);
			dlg->AddClusterAssoc(CTL_BILLINFO_RECADV, 1, 1);
			dlg->AddClusterAssoc(CTL_BILLINFO_RECADV, 2, 2);
			dlg->AddClusterAssoc(CTL_BILLINFO_RECADV, 3, 3);
			dlg->SetClusterData(CTL_BILLINFO_RECADV, recadv_status);
			GetObjectName(PPOBJ_USR, pack.Rec.UserID, buf, 0);
			dlg->setCtrlString(CTL_BILLINFO_USER, buf.Strip());
			guid.ToStr(S_GUID::fmtIDL, buf);
			dlg->setCtrlString(CTL_BILLINFO_UUID, buf.Strip());
			dlg->setCtrlReal(CTL_BILLINFO_OMTPAYM, pack.Rec.PaymAmount);

			PPGetSubStrById(PPTXT_EDIOP, pack.Rec.EdiOp, buf.Z());
			dlg->setCtrlString(CTL_BILLINFO_EDIOP, buf);

			SetBillFlagsCtrl(dlg, CTL_BILLINFO_FLAGS, pack.Rec.Flags);
			if(!PPMaster) {
				dlg->disableCtrls(1, CTL_BILLINFO_FLAGS, 0);
				dlg->setCtrlReadOnly(CTL_BILLINFO_OMTPAYM, 1);
			}
			if(ExecView(dlg) == cmOK) {
				long   f2 = pack.Rec.Flags2;
				dlg->getDTS(&pack);
				dlg->GetClusterData(CTL_BILLINFO_FLAGS, &pack.Rec.Flags);
				dlg->GetClusterData(CTL_BILLINFO_FLAGS2, &f2);
				dlg->GetClusterData(CTL_BILLINFO_RECADV, &recadv_status);
				BillCore::SetRecadvStatus(recadv_status, pack.Rec);
				SETFLAGBYSAMPLE(pack.Rec.Flags2, BILLF2_ACKPENDING, f2);
				THROW(FillTurnList(&pack));
				THROW(UpdatePacket(&pack, 1));
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI ChangeBillFlagsDialog(long * pSetFlags, long * pResetFlags, PPID * pStatusID)
{
	int       ok = -1;
	TDialog * dlg = new TDialog(DLG_BILLF);
	if(CheckDialogPtrErr(&dlg)) {
		SetBillFlagsCtrl(dlg, CTL_BILLF_SET, 0);
		SetBillFlagsCtrl(dlg, CTL_BILLF_RESET, 0);
		SetupPPObjCombo(dlg, CTLSEL_BILLF_STATUS, PPOBJ_BILLSTATUS, *pStatusID, 0);
		if(!BillObj->CheckRights(BILLOPRT_MODSTATUS, 1)) {
			*pStatusID = 0;
			dlg->disableCtrl(CTLSEL_BILLF_STATUS, 1);
		}
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			ushort    v1 = dlg->getCtrlUInt16(CTL_BILLF_SET);
			ushort    v2 = dlg->getCtrlUInt16(CTL_BILLF_RESET);
			valid_data = 1;
			for(uint i = 0; i < 16; i++)
				if((v1 & v2) & (1U << i)) {
					valid_data = 0;
					PPError(PPERR_BILLFCONFLICT, 0);
					break;
				}
			if(valid_data) {
				dlg->GetClusterData(CTL_BILLF_SET, pSetFlags);
				dlg->GetClusterData(CTL_BILLF_RESET, pResetFlags);
				dlg->getCtrlData(CTLSEL_BILLF_STATUS, pStatusID);
				ok = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
//
//
//
int SLAPI PPObjBill::EditFreightDialog(PPBillPacket * pPack)
{
	class FreightDialog : public TDialog {
	public:
		FreightDialog() : TDialog(DLG_FREIGHT), P_Pack(0)
		{
		}
		int setDTS(const PPFreight * pData, PPBillPacket * pPack)
		{
			Data = *pData;
			P_Pack = pPack;

			AddClusterAssocDef(CTL_FREIGHT_TRTYP, 0, PPTRTYP_CAR);
			AddClusterAssoc(CTL_FREIGHT_TRTYP, 1, PPTRTYP_SHIP);
			SetClusterData(CTL_FREIGHT_TRTYP, Data.TrType);
			disableCtrl(CTL_FREIGHT_TRTYP, BIN(Data.ShipID));
			disableCtrl(CTL_FREIGHT_COST, (P_Pack->Rec.ID && !BillObj->CheckRights(PPR_MOD)));
			setCtrlData(CTL_FREIGHT_NAME, Data.Name);
			SetupPPObjCombo(this, CTLSEL_FREIGHT_SHIP,     PPOBJ_TRANSPORT, Data.ShipID, OLW_CANINSERT|OLW_LOADDEFONOPEN, (void *)Data.TrType);
			SetupPPObjCombo(this, CTLSEL_FREIGHT_CAPTAIN,  PPOBJ_PERSON, Data.CaptainID, OLW_CANINSERT/*|OLW_LOADDEFONOPEN*/, (void *)PPPRK_CAPTAIN);
			SetupPPObjCombo(this, CTLSEL_FREIGHT_AGENT,    PPOBJ_PERSON, Data.AgentID, OLW_CANINSERT|OLW_LOADDEFONOPEN, (void *)PPPRK_VESSELSAGENT);
			SetupPPObjCombo(this, CTLSEL_FREIGHT_ISSLOC,   PPOBJ_WORLD, Data.PortOfLoading,   OLW_CANINSERT|OLW_LOADDEFONOPEN|OLW_CANSELUPLEVEL,
				PPObjWorld::MakeExtraParam(WORLDOBJ_CITY|WORLDOBJ_CITYAREA, 0, 0));
			SetupPPObjCombo(this, CTLSEL_FREIGHT_ARRIVLOC, PPOBJ_WORLD, Data.PortOfDischarge, OLW_CANINSERT|OLW_LOADDEFONOPEN|OLW_CANSELUPLEVEL,
				PPObjWorld::MakeExtraParam(WORLDOBJ_CITY|WORLDOBJ_CITYAREA, 0, 0));
			if(P_Pack) {
				// @v9.1.10 if(oneof2(P_Pack->OprType, PPOPT_DRAFTRECEIPT, PPOPT_GOODSRECEIPT) && DS.CheckExtFlag(ECF_RCPTDLVRLOCASWAREHOUSE)) {
				// @v9.1.10 {
				int    dlvr_loc_as_warehouse = 0;
				if(oneof2(P_Pack->OpTypeID, PPOPT_DRAFTRECEIPT, PPOPT_GOODSRECEIPT)) {
					PPOprKind op_rec;
					if(GetOpData(P_Pack->Rec.OpID, &op_rec) > 0 && op_rec.ExtFlags & OPKFX_DLVRLOCASWH)
						dlvr_loc_as_warehouse = 1;
				}
				// } @v9.1.10
				if(dlvr_loc_as_warehouse) {
					SetupPPObjCombo(this, CTLSEL_FREIGHT_DLVRLOC, PPOBJ_LOCATION, Data.DlvrAddrID, 0);
				}
				else {
					PPID   person_id = ObjectToPerson(P_Pack->Rec.Object);
					if(person_id || Data.DlvrAddrID) 
						PersonObj.SetupDlvrLocCombo(this, CTLSEL_FREIGHT_DLVRLOC, person_id, Data.DlvrAddrID);
					//
					// Для внутренней передачи необходимо обеспечить возможность в качестве адреса доставки
					// выбрать склад-получатель.
					// 
					else if(IsIntrOp(P_Pack->Rec.OpID) == INTREXPND) {
						PPID   loc_id = PPObjLocation::ObjToWarehouse(P_Pack->Rec.Object);
						if(loc_id || Data.DlvrAddrID) {
							ComboBox * p_combo = (ComboBox *)getCtrlView(CTLSEL_FREIGHT_DLVRLOC);
							if(p_combo) {
								PPObjLocation loc_obj;
								SString temp_buf;
								StrAssocArray * p_list = new StrAssocArray;
								if(loc_id) {
									GetLocationName(loc_id, temp_buf);
									p_list->Add(loc_id, temp_buf);
								}
								if(Data.DlvrAddrID != loc_id && Data.DlvrAddrID) {
									LocationTbl::Rec loc_rec;
									if(loc_obj.Fetch(Data.DlvrAddrID, &loc_rec) > 0) {
										if(loc_rec.Name[0])
											temp_buf = loc_rec.Name;
										else {
											loc_obj.P_Tbl->GetAddress(loc_rec, 0, temp_buf);
											if(!temp_buf.NotEmptyS())
												ideqvalstr(Data.DlvrAddrID, temp_buf);
										}
										p_list->Add(loc_id, temp_buf);
									}
								}
								ListWindow * p_lw = CreateListWindow(p_list, lbtDisposeData|lbtDblClkNotify);
								if(p_lw)
									p_combo->setListWindow(p_lw, Data.DlvrAddrID);
							}
						}
					}
				}
			}
			SetupCalDate(CTLCAL_FREIGHT_ISSDT, CTL_FREIGHT_ISSDT);
			SetupCalDate(CTLCAL_FREIGHT_ARRIVDT, CTL_FREIGHT_ARRIVDT);
			setCtrlData(CTL_FREIGHT_ISSDT, &Data.IssueDate);
			setCtrlData(CTL_FREIGHT_ARRIVDT, &Data.ArrivalDate);
			setCtrlData(CTL_FREIGHT_NMBORIGSBSL, &Data.NmbOrigsBsL);
			setCtrlReal(CTL_FREIGHT_COST, Data.Cost);
			AddClusterAssoc(CTL_FREIGHT_SHIPPED, 0, BILLF_SHIPPED);
			SetClusterData(CTL_FREIGHT_SHIPPED, P_Pack->Rec.Flags);
			enableCommand(cmFreightEditDlvrLocList, PersonObj.CheckRights(PPR_MOD));
			return 1;
		}

		int getDTS(PPFreight * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			GetClusterData(CTL_FREIGHT_TRTYP, &Data.TrType);
			getCtrlData(CTL_FREIGHT_NAME,        Data.Name);
			strip(Data.Name);
			getCtrlData(CTLSEL_FREIGHT_SHIP,     &Data.ShipID);
			getCtrlData(CTLSEL_FREIGHT_CAPTAIN,  &Data.CaptainID);
			getCtrlData(CTLSEL_FREIGHT_AGENT,    &Data.AgentID);
			getCtrlData(CTLSEL_FREIGHT_ISSLOC,   &Data.PortOfLoading);
			getCtrlData(CTLSEL_FREIGHT_ARRIVLOC, &Data.PortOfDischarge);
			getCtrlData(sel = CTL_FREIGHT_ISSDT, &Data.IssueDate);
			THROW_SL(checkdate(Data.IssueDate, 1));
			getCtrlData(sel = CTL_FREIGHT_ARRIVDT, &Data.ArrivalDate);
			THROW_SL(checkdate(Data.ArrivalDate, 1));
			getCtrlData(CTL_FREIGHT_NMBORIGSBSL, &Data.NmbOrigsBsL);
			if(!P_Pack->Rec.ID || BillObj->CheckRights(PPR_MOD))
				Data.Cost = getCtrlReal(CTL_FREIGHT_COST);
			getCtrlData(CTLSEL_FREIGHT_DLVRLOC,  &Data.DlvrAddrID);
			GetClusterData(CTL_FREIGHT_SHIPPED,  &P_Pack->Rec.Flags);
			*pData = Data;
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_FREIGHT_SHIP)) {
				PPObjTransport tr_obj;
				PPTransport tr_rec;
				PPID   tr_id = getCtrlLong(CTLSEL_FREIGHT_SHIP);
				if(tr_id && tr_obj.Get(tr_id, &tr_rec) > 0) {
					setCtrlData(CTLSEL_FREIGHT_CAPTAIN, &tr_rec.CaptainID);
					disableCtrl(CTL_FREIGHT_TRTYP, 1);
				}
				else
					disableCtrl(CTL_FREIGHT_TRTYP, 0);
			}
			else if(event.isClusterClk(CTL_FREIGHT_TRTYP)) {
				GetClusterData(CTL_FREIGHT_TRTYP, &Data.TrType);
				SetupPPObjCombo(this, CTLSEL_FREIGHT_SHIP, PPOBJ_TRANSPORT, 0, OLW_CANINSERT, (void *)Data.TrType);
			}
			else if(event.isCmd(cmFreightEditDlvrLocList)) {
				if(P_Pack) {
					PPID   loc_id = getCtrlLong(CTLSEL_FREIGHT_DLVRLOC);
					PPID   person_id = ObjectToPerson(P_Pack->Rec.Object);
					if(person_id) {
						if(PersonObj.EditDlvrLocList(person_id) > 0) {
							PersonObj.SetupDlvrLocCombo(this, CTLSEL_FREIGHT_DLVRLOC, person_id, loc_id);
						}
					}
					else if(loc_id) {
						PPID   temp_loc_id = loc_id;
						if(PersonObj.LocObj.Edit(&loc_id, 0) == cmOK) {
							assert(temp_loc_id == loc_id);
							PersonObj.SetupDlvrLocCombo(this, CTLSEL_FREIGHT_DLVRLOC, person_id, loc_id);
						}
					}
				}
			}
			else
				return;
			clearEvent(event);
		}
		PPObjPerson PersonObj;
		PPFreight Data;
		PPBillPacket * P_Pack;
	};
	int    ok = -1;
	FreightDialog * dlg = 0;
	if(!CheckRights(BILLOPRT_MODFREIGHT, 1))
		ok = 0;
	else if(CheckOpFlags(pPack->Rec.OpID, OPKF_FREIGHT)) {
		PPFreight freight;
		RVALUEPTR(freight, pPack->P_Freight);
		SETIFZ(freight.TrType, PPTRTYP_CAR);
		SETIFZ(freight.IssueDate, pPack->Rec.Dt);
		if(CheckDialogPtrErr(&(dlg = new FreightDialog))) {
			dlg->setDTS(&freight, pPack);
			while(ok <= 0 && ExecView(dlg) == cmOK)
	 			if(dlg->getDTS(&freight)) {
					if(!freight.IsEmpty() || (freight.IssueDate && freight.IssueDate != pPack->Rec.Dt)) {
						pPack->SetFreight(&freight);
						ok = 1;
					}
					else {
						pPack->SetFreight(0);
						ok = 100;
					}
					if(!ValidatePacket(pPack, vpfFreightOnly))
						ok = PPErrorZ();
				}
		}
		else
			ok = 0;
	}
	delete dlg;
	return ok;
}

int BillDialog::calcAdvanceRepRest()
{
	int    ok = -1;
	PPID   obj_id = getCtrlLong(CTLSEL_BILL_OBJECT);
	if(obj_id && getCtrlView(CTL_BILL_ADV_INREST)) {
		double rest = 0.0;
		AcctID acctid;
		PPID   acc_sheet_id = 0, ar_sheet_id = 0, acc_rel = 0;
		DateRange period;
		period.low = ZERODATE;
		getCtrlData(CTL_BILL_DATE, &period.upp);
		if(P_BObj->atobj->ConvertAcct(&CConfig.ImprestAcct, 0L /* @curID */, &acctid, &acc_sheet_id) > 0 &&
			acc_sheet_id && GetArticleSheetID(obj_id, &ar_sheet_id) > 0 && ar_sheet_id == acc_sheet_id) {
			acctid.ar = obj_id;
			THROW(P_BObj->atobj->P_Tbl->AcctIDToRel(&acctid, &acc_rel));
			THROW(P_BObj->atobj->P_Tbl->GetAcctRest(period.upp, acc_rel, &rest, 1));
		}
		else {
			THROW(P_BObj->P_Tbl->GetAdvanceBillList(obj_id, P_Pack->Rec.ID, &period, &rest, 0));
		}
		setCtrlData(CTL_BILL_ADV_INREST, &rest);
		setupAdvanceRepTotal(P_Pack->P_AdvRep);
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
IMPL_CMPFUNC(PPBillStatus_Rank_Name, i1, i2)
{
	int    r = 0;
	const PPBillStatus2 * p_s1 = (const PPBillStatus *)i1;
	const PPBillStatus * p_s2 = (const PPBillStatus *)i2;
	if(p_s1->Rank < p_s2->Rank)
		r = -1;
	else if(p_s1->Rank > p_s2->Rank)
		r = 1;
	else
		r = stricmp866(p_s1->Name, p_s2->Name);
	return r;
}

int SLAPI PPObjBill::EditBillStatus(PPID billID)
{
	class SelBillStatusDialog : public PPListDialog {
	public:
		SelBillStatusDialog() : PPListDialog(DLG_SELBILLSTATUS, CTL_SELBILLSTATUS_LIST), IsThereStatus(0)
		{
			//updateList(-1);
		}
		int    setDTS(const BillTbl::Rec * pRec)
		{
			int    ok = 1;
			Rec = *pRec;
			SString temp_buf;
			PPObjBill::MakeCodeString(&Rec, 1, temp_buf);
			setCtrlString(CTL_SELBILLSTATUS_BILL, temp_buf);
			GetObjectName(PPOBJ_BILLSTATUS, Rec.StatusID, temp_buf);
			setCtrlString(CTL_SELBILLSTATUS_STQUO, temp_buf);
			setCtrlData(CTL_SELBILLSTATUS_LIST, &Rec.StatusID); // @v6.5.8
			updateList(-1);
			if(IsThereStatus == 0)
				ok = PPSetError(PPERR_NBILLSTATUS);
			else if(IsThereStatus < 0) {
				GetOpName(Rec.OpID, temp_buf);
				ok = PPSetError(PPERR_NBILLSTATUSFOROP, temp_buf);
			}
			return ok;
		}
		int    getDTS(PPID * pStatusID)
		{
			PPID   id = 0;
			getCurItem(0, &id);
			ASSIGN_PTR(pStatusID, id);
			return (id > 0) ? 1 : PPErrorZ();
		}
	private:
		virtual int  setupList()
		{
			int    ok = 1;
			IsThereStatus = 0;
			PPObjBillStatus bs_obj;
			PPBillStatus bs_rec;
			SArray temp_list(sizeof(PPBillStatus));
			for(SEnum en = bs_obj.ref->Enum(PPOBJ_BILLSTATUS, 0); en.Next(&bs_rec) > 0;) {
				if(!Rec.OpID || !bs_rec.RestrictOpID || IsOpBelongTo(Rec.OpID, bs_rec.RestrictOpID)) {
					THROW_SL(temp_list.insert(&bs_rec));
					IsThereStatus = 1;
				}
				else if(!IsThereStatus)
					IsThereStatus = -1;
			}
			temp_list.sort(PTR_CMPFUNC(PPBillStatus_Rank_Name));
			for(uint i = 0; i < temp_list.getCount(); i++) {
				const PPBillStatus * p_rec = (const PPBillStatus *)temp_list.at(i);
				THROW(addStringToList(p_rec->ID, p_rec->Name));
			}
			CATCHZOK
			return ok;
		}
		int    IsThereStatus;
		BillTbl::Rec Rec;
	};
	int    ok = -1;
	SelBillStatusDialog * dlg = 0;
	if(billID) {
		SString op_name;
		BillTbl::Rec rec;
		THROW(Search(billID, &rec) > 0);
		THROW(CheckDialogPtr(&(dlg = new SelBillStatusDialog())));
		THROW(dlg->setDTS(&rec));
		while(ok <= 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&rec.StatusID))
				if(SetStatus(billID, rec.StatusID, 1))
					ok = 1;
				else
					PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
static int EditPaymPlanItem(const PPBillPacket * pPack, PayPlanTbl::Rec * pData)
{
	int    ok = -1;
	PayPlanTbl::Rec data = *pData;
	TDialog * dlg = new TDialog(DLG_PAYPLANITEM);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->SetupCalDate(CTLCAL_PAYPLANITEM_DT, CTL_PAYPLANITEM_DT);
		dlg->setCtrlData(CTL_PAYPLANITEM_DT, &data.PayDate);
		dlg->setCtrlData(CTL_PAYPLANITEM_AMOUNT,   &data.Amount);
		dlg->setCtrlData(CTL_PAYPLANITEM_INTEREST, &data.Interest);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_PAYPLANITEM_DT, &data.PayDate);
			if(pPack && data.PayDate < pPack->Rec.Dt)
				PPErrorByDialog(dlg, CTL_PAYPLANITEM_AMOUNT, PPERR_PAYDTLTBILLDT);
			else {
				dlg->getCtrlData(CTL_PAYPLANITEM_AMOUNT,   &data.Amount);
				dlg->getCtrlData(CTL_PAYPLANITEM_INTEREST, &data.Interest);
				if(data.Amount < 0 || data.Interest < 0 || (data.Amount == 0 && data.Interest == 0))
					PPErrorByDialog(dlg, CTL_PAYPLANITEM_AMOUNT, PPERR_INVAMOUNT);
				else {
					*pData = data;
					ok = 1;
				}
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

class PaymPlanDialog : public PPListDialog {
public:
	PaymPlanDialog(const PPBillPacket * pPack) : PPListDialog(DLG_PAYPLAN, CTL_PAYPLAN_LIST), P_Pack(pPack)
	{
	}
	int    setDTS(const PayPlanArray *);
	int    getDTS(PayPlanArray *);
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);

	const PPBillPacket * P_Pack;
	PayPlanArray Data;
	PctChargeArray PcList;
};

int PaymPlanDialog::setDTS(const PayPlanArray * pData)
{
	Data = *pData;
	updateList(-1);
	return 1;
}

int PaymPlanDialog::getDTS(PayPlanArray * pData)
{
	ASSIGN_PTR(pData, Data);
	return 1;
}

IMPL_HANDLE_EVENT(PaymPlanDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmPayPlanCalc)) {
		Data.AutoBuild(P_Pack);
		updateList(-1);
		clearEvent(event);
	}
}

int PaymPlanDialog::setupList()
{
	int    ok = 1;
	const  long fmt = SFMT_MONEY;
	double amount = P_Pack->Rec.Amount;
	double rest = amount;
	double total_amount = 0.0;
	double total_interest = 0.0;
	SString sub;
	StringSet ss(SLBColumnDelim);
	PayPlanTbl::Rec * p_item;
	uint    i = 0;
	while(Data.enumItems(&i, (void **)&p_item)) {
		ss.Z().add(sub.Z().Cat(p_item->PayDate));
		ss.add(sub.Z().Cat(p_item->Amount,   fmt));
		ss.add(sub.Z().Cat(p_item->Interest, fmt));
		ss.add(sub.Z().Cat(rest, fmt));
		THROW(addStringToList(i, ss.getBuf()));
		total_amount += p_item->Amount;
		total_interest += p_item->Interest;
		rest -= p_item->Amount;
	}
	if(Data.getCount()) {
		ss.Z().add(PPGetWord(PPWORD_TOTAL, 0, sub));
		ss.add(sub.Z().Cat(total_amount,   fmt));
		ss.add(sub.Z().Cat(total_interest, fmt));
		ss.add(sub.Z().Cat(rest, fmt));
		THROW(addStringToList(Data.getCount()+1, ss.getBuf()));
	}
	CATCHZOK
	return ok;
}

int PaymPlanDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	uint   pos = 0;
	PayPlanTbl::Rec item;
	MEMSZERO(item);
	if(EditPaymPlanItem(P_Pack, &item) > 0)
		if(Data.Update(&item, &pos)) {
			ASSIGN_PTR(pID, (long)item.PayDate);
			ASSIGN_PTR(pPos, (long)pos);
			ok = 1;
		}
		else
			ok = 0;
	return ok;
}

int PaymPlanDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(pos < (long)Data.getCount()) {
		PayPlanTbl::Rec item = Data.at(pos);
		if(EditPaymPlanItem(P_Pack, &item) > 0) {
			uint p = (uint)pos;
			ok = BIN(Data.Update(&item, &p));
		}
	}
	return ok;
}

int PaymPlanDialog::delItem(long pos, long id)
{
	return Data.atFree((uint)pos) ? 1 : -1;
}

int SLAPI EditPaymPlan(const PPBillPacket * pPack, PayPlanArray * pData) { DIALOG_PROC_BODY_P1(PaymPlanDialog, pPack, pData); }
//
//
//
int SLAPI PPObjBill::EditBillFreight(PPID billID)
{
	int    ok = -1;
	BillTbl::Rec bill_rec;
	PPFreight * p_preserve_freight = 0;
	THROW(CheckRights(PPR_READ));
	THROW(CheckRights(BILLOPRT_MODFREIGHT, 1));
	if(Fetch(billID, &bill_rec) > 0 && CheckOpFlags(bill_rec.OpID, OPKF_FREIGHT)) {
		PPBillPacket pack;
		THROW(!pack.Rec.StatusID || CheckStatusFlag(pack.Rec.StatusID, BILSTF_DENY_MOD));
		if(ExtractPacket(billID, &pack) > 0) {
			if(pack.P_Freight) {
				THROW_MEM(p_preserve_freight = new PPFreight(*pack.P_Freight));
			}
			int   do_turn_packet = 0;
			const int    rt_mod = BIN(CheckRights(PPR_MOD));
			const int    prev_freight_state = BIN(pack.Rec.Flags & BILLF_FREIGHT);
			const int    prev_shipped_state = BIN(pack.Rec.Flags & BILLF_SHIPPED);
			const double prev_freight_cost = pack.P_Freight ? pack.P_Freight->Cost : 0.0;
			int r = EditFreightDialog(&pack);
			if(r > 0) {
				const int freight_state = BIN(pack.Rec.Flags & BILLF_FREIGHT);
				const int shipped_state = BIN(pack.Rec.Flags & BILLF_SHIPPED);
				const int is_eq = (p_preserve_freight && pack.P_Freight) ? pack.P_Freight->IsEqual(*p_preserve_freight) : BIN(!p_preserve_freight && !pack.P_Freight);
				if(!is_eq || (prev_freight_state != freight_state) || (prev_shipped_state != shipped_state)) {
					PPTransaction tra(1);
					THROW(tra);
					if(prev_freight_cost != (pack.P_Freight ? pack.P_Freight->Cost : 0.0)) {
						if(rt_mod) {
							do_turn_packet = 1;
						}
						else {
							//
							// Если у пользователя нет прав на изменение документа, то сумму фрахта не меняем
							// так как она влияет на список сумм документа.
							//
							if(pack.P_Freight)
								pack.P_Freight->Cost = prev_freight_cost;
						}
					}
					if(do_turn_packet) {
						THROW(UpdatePacket(&pack, 0));
					}
					else {
						int    bill_updated = 0;
						if(!is_eq) {
							THROW(P_Tbl->SetFreight(billID, pack.P_Freight, 0));
							DS.LogAction(PPACN_UPDBILLFREIGHT, PPOBJ_BILL, billID, 0, 0);
						}
						if(prev_freight_state != freight_state) {
							THROW(P_Tbl->SetRecFlag(billID, BILLF_FREIGHT, BIN(pack.Rec.Flags & BILLF_FREIGHT), 0));
							bill_updated = 1;
						}
						if(prev_shipped_state != shipped_state) {
							THROW(P_Tbl->SetRecFlag(billID, BILLF_SHIPPED, BIN(pack.Rec.Flags & BILLF_SHIPPED), 0));
							bill_updated = 1;
						}
						if(bill_updated)
							DS.LogAction(PPACN_UPDBILL, PPOBJ_BILL, billID, 0, 0);
					}
					THROW(tra.Commit());
					Dirty(billID);
					ok = 1;
				}
			}
		}
	}
	CATCHZOKPPERR
	delete p_preserve_freight;
	return ok;
}

int SLAPI PPObjBill::EditBillExtData(PPID billID)
{
	int    ok = -1;
	BillExtDialog * dlg = 0;
	BillTbl::Rec bill_rec;
	THROW(CheckRights(PPR_READ));
	if(Search(billID, &bill_rec) > 0) {
		const  PPRights & r_rt = ObjRts;
		int    is_need_paym = BIN(CheckOpFlags(bill_rec.OpID, OPKF_NEEDPAYMENT, 0));
		int    valid_data = 0;
		const  PPID agent_acs_id = GetAgentAccSheet();
		const  PPID payer_acs_id = GetSellAccSheet();
		LDATE  last_pay_date;
		PPBillExt ext_data;
		PayPlanArray payplan;
		ObjTagList tag_list;
		THROW(r_rt.CheckBillDate(bill_rec.Dt, 1));
		THROW(P_Tbl->GetExtraData(billID, &ext_data));
		THROW(P_Tbl->GetPayPlan(billID, &payplan));
		THROW(GetTagList(billID, &tag_list));
		last_pay_date = payplan.getCount() ? payplan.at(payplan.getCount()-1).PayDate : ZERODATE;

		THROW(CheckDialogPtr(&(dlg = new BillExtDialog(DLG_BILLEXTMOD, &tag_list))));
		dlg->SetupCalDate(CTLCAL_BILLEXT_PAYDATE, CTL_BILLEXT_PAYDATE);
		if(is_need_paym)
			dlg->setCtrlData(CTL_BILLEXT_PAYDATE, &last_pay_date);
		else
			dlg->disableCtrls(1, CTL_BILLEXT_PAYDATE, CTLCAL_BILLEXT_PAYDATE, 0);
		SetupArCombo(dlg, CTLSEL_BILLEXT_PAYER, ext_data.PayerID, OLW_CANINSERT|OLW_LOADDEFONOPEN, payer_acs_id, sacfDisableIfZeroSheet|sacfNonGeneric);
		SetupArCombo(dlg, CTLSEL_BILLEXT_AGENT, ext_data.AgentID, OLW_CANINSERT|OLW_LOADDEFONOPEN, agent_acs_id, sacfDisableIfZeroSheet|sacfNonGeneric);
		SetupPPObjCombo(dlg, CTLSEL_BILLEXT_EXTPQUOT, PPOBJ_QUOTKIND, ext_data.ExtPriceQuotKindID, 0);
		dlg->SetupCalDate(CTLCAL_BILLEXT_INVCDATE, CTL_BILLEXT_INVCDATE);
		dlg->setCtrlData(CTL_BILLEXT_INVCCODE, ext_data.InvoiceCode);
		dlg->setCtrlData(CTL_BILLEXT_INVCDATE, &ext_data.InvoiceDate);
		if(!CheckRights(PPR_MOD) || !r_rt.CheckBillDate(bill_rec.Dt))
			dlg->enableCommand(cmOK, 0);
		while(!valid_data && ExecView(dlg) == cmOK) {
			THROW(CheckRights(PPR_MOD) && r_rt.CheckBillDate(bill_rec.Dt));
			if(is_need_paym)
				dlg->getCtrlData(CTL_BILLEXT_PAYDATE, &last_pay_date);
			else
				last_pay_date = ZERODATE;
			dlg->getCtrlData(CTL_BILLEXT_INVCDATE, &ext_data.InvoiceDate);
			if(!checkdate(last_pay_date, 1))
				PPErrorByDialog(dlg, CTL_BILLEXT_PAYDATE, PPERR_SLIB);
			else if(!checkdate(ext_data.InvoiceDate, 1))
				PPErrorByDialog(dlg, CTL_BILLEXT_INVCDATE, PPERR_SLIB);
			else {
				ext_data.PayerID = payer_acs_id ? dlg->getCtrlLong(CTLSEL_BILLEXT_PAYER) : 0;
				ext_data.AgentID = agent_acs_id ? dlg->getCtrlLong(CTLSEL_BILLEXT_AGENT) : 0;
				dlg->getCtrlData(CTLSEL_BILLEXT_EXTPQUOT, &ext_data.ExtPriceQuotKindID);
				dlg->getCtrlData(CTL_BILLEXT_INVCCODE, ext_data.InvoiceCode);
				strip(ext_data.InvoiceCode);
				dlg->getCtrlData(CTL_BILLEXT_INVCDATE, &ext_data.InvoiceDate);
				{
					PPTransaction tra(1);
					THROW(tra);
					THROW(P_Tbl->PutExtraData(billID, &ext_data, 0));
					if(is_need_paym) {
						payplan.freeAll();
						if(last_pay_date) {
							PayPlanTbl::Rec rec;
							MEMSZERO(rec);
							rec.BillID  = billID;
							rec.PayDate = last_pay_date;
							rec.Amount  = bill_rec.Amount;
							payplan.insert(&rec);
						}
						THROW(P_Tbl->PutPayPlan(billID, &payplan, 0));
					}
					{
						const ObjTagList * p_tag_list = dlg->GetTagList();
						if(p_tag_list) {
							tag_list = *p_tag_list;
							THROW(SetTagList(billID, p_tag_list, 0));
						}
					}
					DS.LogAction(PPACN_UPDBILLEXT, PPOBJ_BILL, billID, 0, 0);
					THROW(tra.Commit());
				}
				ok = valid_data = 1;
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
LotQCertDialog::LotQCertDialog(PPObjBill * pBObj) : TDialog(DLG_LOTQCERT), P_BObj(pBObj)
{
	MEMSZERO(Data);
	setCtrlOption(CTL_LOTQCERT_FRAME, ofFramed, 1);
	QCertCtrlGroup * p_qc_grp = new QCertCtrlGroup(CTL_LOTQCERT_QCERT);
	addGroup(GRP_QCERT, p_qc_grp);
	SetupCalDate(CTLCAL_LOTQCERT_EXPIRY, CTL_LOTQCERT_EXPIRY);
}

int LotQCertDialog::setDTS(const LotQCertData * pData)
{
	Data = *pData;
	QCertCtrlGroup::Rec qc_rec(Data.QCertID);
	setGroupData(GRP_QCERT, &qc_rec);
	setCtrlData(CTL_LOTQCERT_EXPIRY, &Data.Expiry);
	setCtrlData(CTL_LOTQCERT_CLB, Data.CLB);
	disableCtrl(CTL_LOTQCERT_CLB, Data.IsInheritedClb);
	setCtrlData(CTL_LOTQCERT_SERIAL, Data.Serial);
	return 1;
}

int LotQCertDialog::getDTS(LotQCertData * pData)
{
	int    ok = 1;
	QCertCtrlGroup::Rec qc_rec;
	getCtrlData(CTL_LOTQCERT_EXPIRY,   &Data.Expiry);
	if(!checkdate(Data.Expiry, 1))
		ok = PPErrorByDialog(this, CTL_LOTQCERT_EXPIRY, PPERR_SLIB);
	else {
		getCtrlData(CTL_LOTQCERT_CLB, Data.CLB);
		getGroupData(GRP_QCERT, &qc_rec);
		Data.QCertID = qc_rec.QCertID;
		getCtrlData(CTL_LOTQCERT_SERIAL, Data.Serial);
		ASSIGN_PTR(pData, Data);
	}
	return ok;
}

void LotQCertDialog::generateSerial()
{
	char   templt[48];
	ReceiptTbl::Rec lot_rec;
	if(Data.LotID && P_BObj->trfr->Rcpt.Search(Data.LotID, &lot_rec) > 0) {
		PPObjGoods goods_obj;
		BillTbl::Rec bill_rec;
		if(P_BObj->Search(lot_rec.BillID, &bill_rec) <= 0)
			MEMSZERO(bill_rec);
		STRNSCPY(templt, (goods_obj.IsAsset(lot_rec.GoodsID) > 0) ? P_BObj->Cfg.InvSnTemplt : P_BObj->Cfg.SnTemplt);
		SString serial;
		if(P_BObj->GetSnByTemplate(bill_rec.Code, labs(lot_rec.GoodsID), 0, templt, serial) > 0)
			setCtrlString(CTL_LOTQCERT_SERIAL, serial);
	}
}

IMPL_HANDLE_EVENT(LotQCertDialog)
{
	QCertCtrlGroup::Rec qc_rec;
	TDialog::handleEvent(event);
	if(event.isCmd(cmaEdit)) {
		getGroupData(GRP_QCERT, &qc_rec);
		ViewQCertDialog(qc_rec.QCertID);
	}
	else if(event.isCmd(cmaDelete)) {
		qc_rec.QCertID = 0;
		setGroupData(GRP_QCERT, &qc_rec);
	}
	else if(event.isKeyDown(kbF2) && isCurrCtlID(CTL_LOTQCERT_SERIAL))
		generateSerial();
	else
		return;
	clearEvent(event);
}

int SLAPI PPObjBill::EditLotExtData(PPID lotID)
{
	int    ok = -1;
	ReceiptTbl::Rec lot_rec;
	LotQCertDialog * dlg = 0;
	if(lotID && trfr->Rcpt.Search(lotID, &lot_rec) > 0) {
		int    valid_data = 0, is_inherited_clb = 0;
		LotQCertData lqcd;
		SString org_clb, org_serial;
		GetClbNumberByLot(lotID, &is_inherited_clb, org_clb);
		GetSerialNumberByLot(lotID, org_serial, 0);
		THROW(CheckDialogPtr(&(dlg = new LotQCertDialog(this))));
		lqcd.LotID   = lot_rec.ID;
		lqcd.QCertID = lot_rec.QCertID;
		lqcd.Expiry  = lot_rec.Expiry;
		lqcd.IsInheritedClb = is_inherited_clb;
		org_clb.Strip().CopyTo(lqcd.CLB, sizeof(lqcd.CLB));
		org_serial.Strip().CopyTo(lqcd.Serial, sizeof(lqcd.Serial));
		dlg->setDTS(&lqcd);
		while(!valid_data && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(&lqcd)) {
				valid_data = 1;
				if(lqcd.QCertID != lot_rec.QCertID || lqcd.Expiry != lot_rec.Expiry) {
					THROW(trfr->Rcpt.Search(lotID, &lot_rec) > 0);
					lot_rec.QCertID = lqcd.QCertID;
					lot_rec.Expiry  = lqcd.Expiry;
					THROW_DB(trfr->Rcpt.Update(lotID, &lot_rec, 1));
					ok = 1;
				}
				if(strcmp(org_clb, lqcd.CLB) != 0 && !is_inherited_clb) {
					THROW(SetClbNumberByLot(lotID, lqcd.CLB, 1));
					ok = 1;
				}
				if(strcmp(org_serial, lqcd.Serial) != 0) {
					THROW(SetSerialNumberByLot(lotID, lqcd.Serial, 1));
					ok = 1;
				}
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
// LotInfo
//
class LotInfoDialog : public TDialog {
public:
	LotInfoDialog(ReceiptTbl::Rec * pRec, int _canEdit) : TDialog(DLG_LOTINFO), P_BObj(BillObj), CanEdit(_canEdit)
	{
		SetupPPObjCombo(this, CTLSEL_LOTINFO_LOC, PPOBJ_LOCATION, 0, 0);
		if(P_BObj->CheckRights(BILLOPRT_ACCSSUPPL, 1))
			SetupArCombo(this, CTLSEL_LOTINFO_SUPPL, pRec->SupplID, OLW_LOADDEFONOPEN, GetSupplAccSheet(), sacfDisableIfZeroSheet|sacfNonGeneric);
		SetupPPObjCombo(this, CTLSEL_LOTINFO_INTAXGRP, PPOBJ_GOODSTAX, 0, 0);
		addGroup(GRP_QCERT, new QCertCtrlGroup(CTL_LOTINFO_QCERT));
		Chain.add(pRec->ID);
		setDTS(pRec);
	}
	int    setDTS(const ReceiptTbl::Rec *);
	int    getDTS(ReceiptTbl::Rec *);
private:
	DECL_HANDLE_EVENT;
	void   setupLinkedLot(int prev);

	PPObjBill * P_BObj;
	ReceiptTbl::Rec Data;
	int    CanEdit;
	PPIDArray Chain;
};

void LotInfoDialog::setupLinkedLot(int prev)
{
	PPID   lot_id = 0;
	if(prev) {
		lot_id = Data.PrevLotID;
		if(lot_id && Chain.addUnique(lot_id) < 0) {
			SString msg_buf;
			msg_buf.Cat(Data.ID).CatChar('-').CatChar('>').Cat(lot_id);
			PPError(PPERR_LOTLOOP, msg_buf);
			lot_id = 0;
		}
	}
	else if(Chain.getCount() > 1) {
		const uint pos = Chain.getCount() - 1;
		lot_id = Chain.at(pos-1);
		Chain.atFree(pos);
	}
	if(lot_id) {
		ReceiptTbl::Rec lot_rec;
		if(P_BObj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0)
			setDTS(&lot_rec);
		else
			PPError();
	}
}

int LotInfoDialog::setDTS(const ReceiptTbl::Rec * pRec)
{
	int    s;
	Data = *pRec;
	setCtrlLong(CTLSEL_LOTINFO_LOC, Data.LocID);
	SetupPPObjCombo(this, CTLSEL_LOTINFO_GOODS, PPOBJ_GOODS, Data.GoodsID, OLW_LOADDEFONOPEN);
	if(P_BObj->CheckRights(BILLOPRT_ACCSSUPPL, 1)) {
		const PPID temp_id = getCtrlLong(CTLSEL_LOTINFO_SUPPL);
		if(temp_id != Data.SupplID)
			setCtrlLong(CTLSEL_LOTINFO_SUPPL, Data.SupplID);
	}
	{
		QCertCtrlGroup::Rec qc_rec(Data.QCertID);
		setGroupData(GRP_QCERT, &qc_rec);
	}
	setCtrlLong(CTLSEL_LOTINFO_INTAXGRP, Data.InTaxGrpID);
	setCtrlLong(CTL_LOTINFO_ID,       Data.ID);
	setCtrlReal(CTL_LOTINFO_QTTY,     Data.Quantity);
	setCtrlReal(CTL_LOTINFO_REST,     Data.Rest);
	setCtrlReal(CTL_LOTINFO_PHREST,   Data.WtRest);
	setCtrlReal(CTL_LOTINFO_UPP,      Data.UnitPerPack);
	setCtrlReal(CTL_LOTINFO_COST,     P_BObj->CheckRights(BILLRT_ACCSCOST) ? Data.Cost : 0.0);
	setCtrlData(CTL_LOTINFO_PRICE,    &Data.Price);
	setCtrlDate(CTL_LOTINFO_CLOSEDT, (Data.CloseDate == MAXDATE) ? ZERODATE : Data.CloseDate);
	setCtrlUInt16(CTL_LOTINFO_CLOSED, BIN(Data.Closed));
	disableCtrl(CTL_LOTINFO_ID, 1);
	if(!CanEdit) {
		disableCtrls(1, CTLSEL_LOTINFO_LOC, CTLSEL_LOTINFO_GOODS, CTLSEL_LOTINFO_SUPPL,
			CTLSEL_LOTINFO_QCERT, CTL_LOTINFO_QTTY, CTL_LOTINFO_REST, CTL_LOTINFO_UPP,
			CTL_LOTINFO_COST, CTL_LOTINFO_PRICE, CTL_LOTINFO_CLOSEDT, CTL_LOTINFO_CLOSED, 0);
		enableCommand(cmOK, 0);
	}
	s = BIN(Chain.getCount() > 1);
	showCtrl(STDCTL_BACKBUTTON, s);
	enableCommand(cmOK, !s);
	enableCommand(cmPrevLot, (Data.PrevLotID != 0));
	return 1;
}

int LotInfoDialog::getDTS(ReceiptTbl::Rec * pRec)
{
	ushort v;
	LDATE  dt;
	Data.LocID = getCtrlLong(CTLSEL_LOTINFO_LOC);
	Data.GoodsID = getCtrlLong(CTLSEL_LOTINFO_GOODS);
	if(P_BObj->CheckRights(BILLOPRT_ACCSSUPPL, 1))
		Data.SupplID = getCtrlLong(CTLSEL_LOTINFO_SUPPL);
	{
		QCertCtrlGroup::Rec qc_rec;
		getGroupData(GRP_QCERT, &qc_rec);
		Data.QCertID = qc_rec.QCertID;
	}
	getCtrlData(CTLSEL_LOTINFO_INTAXGRP, &Data.InTaxGrpID);
	getCtrlData(CTL_LOTINFO_ID,    &Data.ID);
	getCtrlData(CTL_LOTINFO_QTTY,  &Data.Quantity);
	getCtrlData(CTL_LOTINFO_REST,  &Data.Rest);
	{
		double ph_rest = Data.WtRest;
		if(getCtrlData(CTL_LOTINFO_REST,  &ph_rest))
			Data.WtRest = (float)ph_rest;
	}
	getCtrlData(CTL_LOTINFO_UPP,   &Data.UnitPerPack);
	if(P_BObj->CheckRights(BILLRT_ACCSCOST))
		getCtrlData(CTL_LOTINFO_COST, &Data.Cost);
	getCtrlData(CTL_LOTINFO_PRICE, &Data.Price);
	getCtrlData(CTL_LOTINFO_CLOSEDT, &dt);
	Data.CloseDate = NZOR(dt, MAXDATE);
	if(getCtrlData(CTL_LOTINFO_CLOSED, &v))
		Data.Closed = BIN(v);
	ASSIGN_PTR(pRec, Data);
	return 1;
}

IMPL_HANDLE_EVENT(LotInfoDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmLotBill)) {
		P_BObj->ViewBillInfo(Data.BillID);
	}
	else if(event.isCmd(cmLotOps))
		::ViewOpersByLot(Data.ID, 0);
	else if(event.isCmd(cmPrevLot))
		setupLinkedLot(1);
	else if(event.isCmd(cmaBack))
		setupLinkedLot(0);
	else if(event.isCmd(cmObjSyncTab)) {
		PPObjID oid;
		ViewObjSyncTab(oid.Set(PPOBJ_LOT, Data.ID));
	}
	else if(event.isCmd(cmTags)) {
		ObjTagList tag_list;
		PPRef->Ot.GetList(PPOBJ_LOT, Data.ID, &tag_list);
		EditObjTagValList(&tag_list, 0);
	}
	else if(event.isKeyDown(kbBack))
		setupLinkedLot(0);
	else
		return;
	clearEvent(event);
}

int SLAPI PPObjBill::EditLotSystemInfo(PPID lotID)
{
	int    ok = -1;
	ReceiptTbl::Rec rec;
	LotInfoDialog * dlg = 0;
	if(lotID && trfr->Rcpt.Search(lotID, &rec) > 0) {
		const int can_edit = PPMaster ? 1 : 0;
		if(CheckDialogPtrErr(&(dlg = new LotInfoDialog(&rec, can_edit)))) {
			if(ExecView(dlg) == cmOK && can_edit) {
				//THROW(P_Tbl->Update(lot_id, &rec, 1));
				//ok = 1;
			}
		}
		else
			ok = 0;
	}
	delete dlg;
	return ok;
}
