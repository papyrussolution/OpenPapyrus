// ATURNDLG.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000-2002, 2003, 2004, 2005, 2007, 2016
//
#include <pp.h>
#pragma hdrstop

// Prototype
int SLAPI EditBillTaxes(AmtList *, double amount);

SLAPI CurAmtCtrlGroup::Rec::Rec()
{
	THISZERO();
}

CurAmtCtrlGroup::CurAmtCtrlGroup(uint amtCID, uint curSelCID, uint crateCID,
	uint baseAmtCID, uint dateCID, uint selCRateCmd, AmtList * pAmtList) : CtrlGroup()
{
	AmtCID = amtCID;
	CurSelCID = curSelCID;
	CRateCID = crateCID;
	BaseAmtCID = baseAmtCID;
	DateCID = dateCID;
	SelCRateCmd = selCRateCmd;
	MEMSZERO(Data);
	P_AL = pAmtList;
}

CurAmtCtrlGroup::~CurAmtCtrlGroup()
{
}

void CurAmtCtrlGroup::setupCurrencyCombo(TDialog * pDlg, PPIDArray * pCurList)
{
	PPID   cur_id = pDlg->getCtrlLong(CurSelCID);
	SETIFZ(cur_id, Data.CurID);
	if(pCurList && !pCurList->lsearch(cur_id))
		cur_id = 0;
	::SetupCurrencyCombo(pDlg, CurSelCID, cur_id, 0, 1, pCurList);
	if(cur_id == 0)
		setupCurRate(pDlg, 0);
}

void CurAmtCtrlGroup::setupCurRate(TDialog * pDlg, int fromBase)
{
	double crate = 0.0, base_amount = 0.0;
	PPID   cur_id = pDlg->getCtrlLong(CurSelCID);
	double amount = pDlg->getCtrlReal(AmtCID);
	if(DateCID)
		pDlg->getCtrlData(DateCID, &Data.CRateDate);
	if(cur_id) {
		pDlg->disableCtrl(CRateCID, 0);
		if(fromBase) {
			if(!P_AL || P_AL->Get(PPAMT_CRATE, cur_id, &crate) <= 0)
				BillObj->GetCurRate(cur_id, &Data.CRateDate, &crate);
			pDlg->setCtrlReal(CRateCID, crate);
		}
		else
			crate = pDlg->getCtrlReal(CRateCID);
		base_amount = R2(amount * crate);
	}
	else {
		pDlg->setCtrlReal(CRateCID, crate);
		pDlg->disableCtrl(CRateCID, 1);
	}
	if(SelCRateCmd)
		pDlg->enableCommand(SelCRateCmd, BIN(cur_id));
	pDlg->setCtrlReal(BaseAmtCID, base_amount);
}

int CurAmtCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	int    use_same_crate = 0;
	Data = *(Rec *)pData;
	pDlg->disableCtrl(BaseAmtCID, 1);
	SetupCurrencyCombo(pDlg, CurSelCID, Data.CurID, 0, 1, 0);
	pDlg->setCtrlData(AmtCID, &Data.Amount);
	if(Data.CRate > 0.0) {
		pDlg->setCtrlReal(CRateCID, Data.CRate);
		use_same_crate = 1;
	}
	setupCurRate(pDlg, use_same_crate ? 0 : 1);
	return 1;
}

int CurAmtCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	int    ok = 1;
	pDlg->getCtrlData(CurSelCID,  &Data.CurID);
	pDlg->getCtrlData(AmtCID,     &Data.Amount);
	pDlg->getCtrlData(CRateCID,   &Data.CRate);
	pDlg->getCtrlData(BaseAmtCID, &Data.BaseAmount);
	if(pData)
		*(Rec*)pData = Data;
	return ok;
}

void CurAmtCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(event.isCbSelected(CurSelCID))
		setupCurRate(pDlg, 1);
	else if(event.isCmd(cmCurAmtGrpSetupCurrencyCombo)) {
		PPIDArray * p_cur_list = (PPIDArray *)event.message.infoPtr;
		setupCurrencyCombo(pDlg, p_cur_list);
	}
	else if(SelCRateCmd && event.isCmd(SelCRateCmd)) {
		double rate = 0.0;
		PPID   cur_id = pDlg->getCtrlLong(CurSelCID);
		if(cur_id && SelectCurRate(cur_id, LConfig.BaseRateTypeID, &rate) > 0) {
			pDlg->setCtrlData(CRateCID, &rate);
			setupCurRate(pDlg, 0);
		}
	}
	else if(TVBROADCAST && TVCMD == cmChangedFocus && TVINFOVIEW) {
		if(event.isCtlEvent(AmtCID) || event.isCtlEvent(CRateCID))
			setupCurRate(pDlg, 0);
		else if(event.isCtlEvent(DateCID))
			setupCurRate(pDlg, 1);
		return;
	}
	else if(event.isKeyDown(kbF2) && pDlg->isCurrCtlID(CRateCID)) {
		double rate = 0.0;
		PPID   cur_id = pDlg->getCtrlLong(CurSelCID);
		if(SelectCurRate(cur_id, LConfig.BaseRateTypeID, &rate) > 0) {
			pDlg->setCtrlData(CRateCID, &rate);
			setupCurRate(pDlg, 0);
		}
	}
	else
		return;
	pDlg->clearEvent(event);
}
//
//
//
#define GRP_DBT    1
#define GRP_CRD    2
#define GRP_CURAMT 3

AccTurnDialog::AccTurnDialog(uint rezID, PPObjBill * pBObj) : TDialog(rezID), P_BObj(pBObj), P_Pack(0)
{
	MEMSZERO(Data);
	setCtrlOption(CTL_ATURN_DTEXT, ofFramed, 1);
	setCtrlOption(CTL_ATURN_CTEXT, ofFramed, 1);
	AcctCtrlGroup * p_ac_grp = new AcctCtrlGroup(CTL_ATURN_DACC,
		CTL_ATURN_DART, CTLSEL_ATURN_DACCNAME, CTLSEL_ATURN_DARTNAME);
	addGroup(GRP_DBT, p_ac_grp);
	p_ac_grp = new AcctCtrlGroup(CTL_ATURN_CACC, CTL_ATURN_CART, CTLSEL_ATURN_CACCNAME, CTLSEL_ATURN_CARTNAME);
	addGroup(GRP_CRD, p_ac_grp);
	CurAmtCtrlGroup * p_ca_grp = new CurAmtCtrlGroup(CTL_ATURN_AMOUNT,
		CTLSEL_ATURN_CUR, CTL_ATURN_CRATE, CTL_ATURN_BASEAMT, CTL_ATURN_DATE, 0, 0);
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
	THROW_SL(checkdate(Data.Date, 0));
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
