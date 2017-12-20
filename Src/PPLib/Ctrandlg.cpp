// CTRANDLG.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2003, 2005, 2006, 2007, 2008, 2016, 2017
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

#define GRP_INCURAMT  1
#define GRP_OUTCURAMT 2

class CurTransBillDialog : public TDialog {
public:
	CurTransBillDialog() : TDialog(DLG_C_TRANS), P_Pack(0), HasAmtIDList(0)
	{
		MEMSZERO(Data);
		setCtrlOption(CTL_BILL_FRAME1, ofFramed, 1);
		setCtrlOption(CTL_BILL_FRAME2, ofFramed, 1);
		CurAmtCtrlGroup * p_ca_grp = new CurAmtCtrlGroup(CTL_BILL_AMOUNT, CTLSEL_BILL_CUR,
			CTL_BILL_CRATE, CTL_BILL_BASEAMT, CTL_BILL_DATE, cmSelInRate, 0);
		addGroup(GRP_INCURAMT, p_ca_grp);
		p_ca_grp = new CurAmtCtrlGroup(CTL_BILL_OUTCTAMOUNT, CTLSEL_BILL_OUTCTCUR,
			CTL_BILL_OUTCTRATE, CTL_BILL_OUTCTBASE, CTL_BILL_DATE, cmSelOutRate, 0);
		addGroup(GRP_OUTCURAMT, p_ca_grp);
		SetupCalDate(CTLCAL_BILL_DATE, CTL_BILL_DATE);
	}
	int    setDTS(PPBillPacket *);
	int    getDTS();
private:
	DECL_HANDLE_EVENT;
	int    getCurGroupData(int ctlGrpID);
	int    setCurGroupData(int ctlGrpID);
	int    recalcAmounts(uint masterCtlID);
	void   editExtAmounts();
	void   swapCurrencies();
	void   swapTransitRate();

	PPBillPacket * P_Pack;
	PPCurTransit Data;
	int          HasAmtIDList;
	PPIDArray    ExtAmtIDList;
};

int CurTransBillDialog::recalcAmounts(uint masterCtlID)
{
	double in_amt  = getCtrlReal(CTL_BILL_AMOUNT);
	double out_amt = getCtrlReal(CTL_BILL_OUTCTAMOUNT);
	double ct_rate = getCtrlReal(CTL_BILL_CTRATE);
	if(masterCtlID == CTL_BILL_AMOUNT) {
		if(in_amt > 0.0)
			if(out_amt != 0.0)
				ct_rate = R6(out_amt / in_amt);
			else if(ct_rate != 0.0)
				out_amt = R2(in_amt * ct_rate);
	}
	else if(masterCtlID == CTL_BILL_OUTCTAMOUNT) {
		if(out_amt > 0)
			if(in_amt != 0.0)
				ct_rate = R6(out_amt / in_amt);
			else if(ct_rate != 0.0)
				in_amt = R2(out_amt / ct_rate);
	}
	else if(masterCtlID == CTL_BILL_CTRATE) {
		if(ct_rate > 0.0) {
			if(in_amt == 0.0)
				in_amt = R2(out_amt / ct_rate);
			else //if(out_amt == 0)
				out_amt = R2(in_amt * ct_rate);
		}
	}
	setCtrlReal(CTL_BILL_AMOUNT,      in_amt);
	setCtrlReal(CTL_BILL_OUTCTAMOUNT, out_amt);
	setCtrlReal(CTL_BILL_CTRATE,      ct_rate);
	return 1;
}

void CurTransBillDialog::swapTransitRate()
{
	double tmp_val = getCtrlReal(CTL_BILL_CTRATE);
	if(tmp_val != 0.0) {
		setCtrlReal(CTL_BILL_CTRATE, 1.0 / tmp_val);
		recalcAmounts(CTL_BILL_CTRATE);
	}
}

void CurTransBillDialog::swapCurrencies()
{
	PPID   tmp_id = 0;
	double tmp_val;
	getCurGroupData(GRP_INCURAMT);
	getCurGroupData(GRP_OUTCURAMT);
	getCtrlData(CTL_BILL_CTRATE, &Data.TransitCRate);
	tmp_id            = Data.InCurID;
	Data.InCurID      = Data.OutCurID;
	Data.OutCurID     = tmp_id;
	tmp_val           = Data.InCurAmount;
	Data.InCurAmount  = Data.OutCurAmount;
	Data.OutCurAmount = tmp_val;
	tmp_val           = Data.InCRate;
	Data.InCRate      = Data.OutCRate;
	Data.OutCRate     = tmp_val;

	CurAmtCtrlGroup::Rec ca_cg_rec;
	ca_cg_rec.CurID  = Data.InCurID;
	ca_cg_rec.Amount = Data.InCurAmount;
	ca_cg_rec.CRate  = Data.InCRate;
	setGroupData(GRP_INCURAMT, &ca_cg_rec);

	MEMSZERO(ca_cg_rec);
	ca_cg_rec.CurID  = Data.OutCurID;
	ca_cg_rec.Amount = Data.OutCurAmount;
	ca_cg_rec.CRate  = Data.OutCRate;
	setGroupData(GRP_OUTCURAMT, &ca_cg_rec);
	if(Data.InCurAmount > 0.0 && Data.OutCurAmount > 0.0)
		Data.TransitCRate = R6(Data.OutCurAmount / Data.InCurAmount);
	else if(Data.TransitCRate != 0.0)
		Data.TransitCRate = R6(1.0 / Data.TransitCRate);
	setCtrlData(CTL_BILL_CTRATE, &Data.TransitCRate);
}

IMPL_HANDLE_EVENT(CurTransBillDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmSwap))
		swapCurrencies();
	else if(event.isCmd(cmExAmountList))
		editExtAmounts();
	else if(TVBROADCAST && TVCMD == cmChangedFocus) {
		uint   ctl_id = event.getCtlID();
		if(oneof3(ctl_id, CTL_BILL_AMOUNT, CTL_BILL_OUTCTAMOUNT, CTL_BILL_CTRATE))
			recalcAmounts(ctl_id);
		else
			return;
	}
	else if(event.isKeyDown(kbF2)) {
		if(isCurrCtlID(CTL_BILL_DOC))
			BillObj->UpdateOpCounter(P_Pack);
		else if(isCurrCtlID(CTL_BILL_CTRATE))
			swapTransitRate();
	}
	else
		return;
	clearEvent(event);
}

int CurTransBillDialog::setDTS(PPBillPacket * pPack)
{
	P_Pack = pPack;

	int    ok = 1;
	PPObjOprKind opkobj;
	PPOprKindPacket op_pack;

	MEMSZERO(Data);
	THROW(P_Pack->GetCurTransit(&Data));
	THROW(opkobj.GetPacket(Data.OpID, &op_pack) > 0);
	setTitle(op_pack.Rec.Name);
	if(op_pack.Amounts.getCount()) {
		ExtAmtIDList.copy(op_pack.Amounts);
		HasAmtIDList = 1;
	}
	else {
		ExtAmtIDList.freeAll();
		HasAmtIDList = 0;
	}
	enableCommand(cmExAmountList, HasAmtIDList);
	setCtrlData(CTL_BILL_DOC,  Data.BillCode);
	setCtrlData(CTL_BILL_DATE, &Data.Date);
	SetupArCombo(this, CTLSEL_BILL_OBJECT, Data.ObjectID, OLW_LOADDEFONOPEN|OLW_CANINSERT, Data.AccSheetID, sacfDisableIfZeroSheet|sacfNonGeneric);
	setCurGroupData(GRP_INCURAMT);
	setCurGroupData(GRP_OUTCURAMT);
	setCtrlData(CTL_BILL_CTRATE, &Data.TransitCRate);
	setCtrlData(CTL_BILL_MEMO,   Data.Memo);
	disableCtrls(Data.BillID ? 1 : 0, CTLSEL_BILL_CUR, CTLSEL_BILL_OUTCTCUR, 0);
	CATCHZOK
	return ok;
}

int CurTransBillDialog::getCurGroupData(int ctlGrpID)
{
	int    ok = 1;
	CurAmtCtrlGroup::Rec ca_cg_rec;
	getGroupData(ctlGrpID, &ca_cg_rec);
	if(ctlGrpID == GRP_INCURAMT) {
		Data.InCurID      = ca_cg_rec.CurID;
		Data.InCurAmount  = ca_cg_rec.Amount;
		Data.InCRate      = ca_cg_rec.CRate;
	}
	else if(ctlGrpID == GRP_OUTCURAMT) {
		Data.OutCurID     = ca_cg_rec.CurID;
		Data.OutCurAmount = ca_cg_rec.Amount;
		Data.OutCRate     = ca_cg_rec.CRate;
	}
	else
		ok = 0;
	return ok;
}

int CurTransBillDialog::setCurGroupData(int ctlGrpID)
{
	CurAmtCtrlGroup::Rec ca_cg_rec;
	if(ctlGrpID == GRP_INCURAMT) {
		ca_cg_rec.CurID  = Data.InCurID;
		ca_cg_rec.Amount = Data.InCurAmount;
		ca_cg_rec.CRate  = Data.InCRate;
	}
	else if(ctlGrpID == GRP_OUTCURAMT) {
		ca_cg_rec.CurID  = Data.OutCurID;
		ca_cg_rec.Amount = Data.OutCurAmount;
		ca_cg_rec.CRate  = Data.OutCRate;
	}
	else
		return 0;
	setGroupData(ctlGrpID, &ca_cg_rec);
	return 1;
}

void CurTransBillDialog::editExtAmounts()
{
	if(HasAmtIDList) {
		//
		// В случае, если в списке дополнительных сумм будут изменены
		// валютные курсы, предварительно извлечем, а затем установим
		// назад уже возможно измененные курсы.
		//
		LDATE bill_date;
		getCtrlData(CTL_BILL_DATE, &bill_date);
		getCurGroupData(GRP_INCURAMT);
		getCurGroupData(GRP_OUTCURAMT);
		P_Pack->Amounts.Remove(PPAMT_CRATE, Data.InCurID);
		P_Pack->Amounts.Remove(PPAMT_CRATE, Data.OutCurID);
		P_Pack->Amounts.Put(PPAMT_CRATE, Data.InCurID,  Data.InCRate,  0, 1);
		P_Pack->Amounts.Put(PPAMT_CRATE, Data.OutCurID, Data.OutCRate, 0, 1);
		if(AmountListDialog(&P_Pack->Amounts, &ExtAmtIDList,
			bill_date, 0, OLW_CANEDIT | OLW_CANDELETE | OLW_CANINSERT) > 0) {
			Data.InCRate  = P_Pack->Amounts.Get(PPAMT_CRATE, Data.InCurID);
			Data.OutCRate = P_Pack->Amounts.Get(PPAMT_CRATE, Data.OutCurID);
			setCurGroupData(GRP_INCURAMT);
			setCurGroupData(GRP_OUTCURAMT);
		}
	}
}

int CurTransBillDialog::getDTS()
{
	int    ok = 1;
	getCurGroupData(GRP_INCURAMT);
	getCurGroupData(GRP_OUTCURAMT);
	THROW_PP(Data.InCurID != Data.OutCurID, PPERR_EQCTCUR);
	THROW_PP(Data.InCurID != 0,  PPERR_UNDEFCTINCUR);
	THROW_PP(Data.OutCurID != 0, PPERR_UNDEFCTOUTCUR);
	THROW_PP(Data.InCurAmount  > 0, PPERR_INVCTINAMT);
	THROW_PP(Data.OutCurAmount > 0, PPERR_INVCTOUTAMT);
	getCtrlData(CTL_BILL_CTRATE, &Data.TransitCRate);
	getCtrlData(CTL_BILL_DOC,  Data.BillCode);
	getCtrlData(CTL_BILL_DATE, &Data.Date);
	THROW_SL(checkdate(&Data.Date));
	THROW(ObjRts.CheckBillDate(Data.Date, 0));
	getCtrlData(CTLSEL_BILL_OBJECT, &Data.ObjectID);
	getCtrlData(CTL_BILL_MEMO, Data.Memo);
	THROW(P_Pack->SetCurTransit(&Data));
	CATCHZOK
	return ok;
}

int SLAPI EditCurTransitBill(PPBillPacket * pPack)
{
	int    r = cmCancel, valid_data = 0;
	CurTransBillDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new CurTransBillDialog())));
	THROW(dlg->setDTS(pPack));
	while(!valid_data && (r = ExecView(dlg)) == cmOK)
		if(dlg->getDTS())
			valid_data = 1;
		else
			PPError();
	CATCH
		r = (/*PPError(),*/ 0);
	ENDCATCH
	delete dlg;
	return r;
}
