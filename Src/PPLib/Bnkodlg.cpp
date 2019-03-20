// BNKODLG.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2005, 2007, 2008, 2010, 2011, 2014, 2016, 2017
// @codepage windows-1251
// Диалог редактирования данных для банковских платежных документов
//
#include <pp.h>
#pragma hdrstop

class TaxPeriodDialog : public TDialog {
public:
	TaxPeriodDialog() : TDialog(DLG_TXMPERIOD)
	{
	}
	int    setDTS(const PPTaxPeriod * pData);
	int    getDTS(PPTaxPeriod *);
private:
	DECL_HANDLE_EVENT;
	PPTaxPeriod Data;
};

IMPL_HANDLE_EVENT(TaxPeriodDialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_TXMPERIOD_PERIOD)) {
		ushort v = getCtrlUInt16(CTL_TXMPERIOD_PERIOD);
		disableCtrl(CTLSEL_TXMPERIOD_MONTH, (v < PPTaxPeriod::eMonth));
		disableCtrl(CTL_TXMPERIOD_DAY, (v != PPTaxPeriod::eDate));
		clearEvent(event);
	}
}

int TaxPeriodDialog::setDTS(const PPTaxPeriod * pData)
{
	Data = *pData;
	ushort v = (ushort)Data.P;
	setCtrlData(CTL_TXMPERIOD_PERIOD, &v);
	setCtrlData(CTL_TXMPERIOD_YEAR, &Data.Year);
	setCtrlData(CTL_TXMPERIOD_DAY, &Data.Day);
	SetupStringCombo(this, CTLSEL_TXMPERIOD_MONTH, PPTXT_MONTHES, Data.Month);
	disableCtrl(CTLSEL_TXMPERIOD_MONTH, (v < PPTaxPeriod::eMonth));
	disableCtrl(CTL_TXMPERIOD_DAY, (v != PPTaxPeriod::eDate));
	return 1;
}

int TaxPeriodDialog::getDTS(PPTaxPeriod * pData)
{
	int    ok = 1;
	long   monthid = 0;
	ushort v = getCtrlUInt16(CTL_TXMPERIOD_PERIOD);
	Data.P = (int8)v;
	getCtrlData(CTL_TXMPERIOD_YEAR, &Data.Year);
	if(Data.Year == 0)
		ok = PPErrorByDialog(this, CTL_TXMPERIOD_YEAR, PPERR_INVPERIODINPUT);
	else {
		if(Data.Year < 100)
			if(Data.Year >= 50)
				Data.Year += 1900;
			else
				Data.Year += 2000;
		else if(Data.Year >= 200 && Data.Year <= 299)
			Data.Year = 2000 + (Data.Year - 200);
		getCtrlData(CTL_TXMPERIOD_MONTH, &monthid);
		Data.Month = (monthid >= 0 && monthid <= 12) ? (int8)monthid : 0;
		getCtrlData(CTL_TXMPERIOD_DAY, &Data.Day);
		if(Data.Year < 1990 || Data.Year > 2100)
			ok = PPErrorByDialog(this, CTL_TXMPERIOD_YEAR, PPERR_INVPERIODINPUT);
		else if(Data.P >= PPTaxPeriod::eMonth && Data.Month == 0)
			ok = PPErrorByDialog(this, CTLSEL_TXMPERIOD_MONTH, PPERR_INVPERIODINPUT);
		else if(Data.P == PPTaxPeriod::eDate && Data.Day < 1 || Data.Day > 31)
			ok = PPErrorByDialog(this, CTL_TXMPERIOD_DAY, PPERR_INVPERIODINPUT);
		else
			ASSIGN_PTR(pData, Data);
	}
	return ok;
}

class BnkOrdTaxMarkersDialog : public TDialog {
public:
	BnkOrdTaxMarkersDialog() : TDialog(DLG_TXM)
	{
		SetupCalCtrl(CTLCAL_TXM_DOCDATE, this, CTL_TXM_DOCDATE, 4);
	}
	int    setDTS(const PPBankingOrder *);
	//
	// Функция getDTS может изменить только структуру по полю
	// PPBankingOrder::Txm
	//
	int    getDTS(PPBankingOrder *);
private:
	DECL_HANDLE_EVENT;
	int    selectPeriod();
	int    translateCode(int dir /* 0 - code to id, 1 - id to code */, uint strID, char * pCode, long * pID);
	PPBankingOrder Data;
};

IMPL_HANDLE_EVENT(BnkOrdTaxMarkersDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmSelectPeriod) && selectPeriod() > 0) {
		SString buf;
		setCtrlString(CTL_TXM_TAXPERIOD, Data.Txm.Period.Format(buf));
		clearEvent(event);
	}
}

int BnkOrdTaxMarkersDialog::selectPeriod() { DIALOG_PROC_BODY(TaxPeriodDialog, &Data.Txm.Period); }

int BnkOrdTaxMarkersDialog::translateCode(int dir, uint strID, char * pCode, long * pID)
{
	char   item_buf[64];
	for(int idx = 0; PPGetSubStr(strID, idx, item_buf, sizeof(item_buf)) > 0; idx++) {
		char * p = sstrchr(item_buf, ',');
		if(p) {
			char   code[64];
			code[0] = 0;
			*p++ = 0;
			const long id = atol(item_buf);
			char * q = sstrchr(p, '-');
			if(q) {
				char * c = code;
				for(char * r = p; r < q;)
					*c++ = *r++;
				*c = 0;
				if(dir == 0) {
					if(stricmp866(pCode, code) == 0) {
						ASSIGN_PTR(pID, id);
						return 1;
					}
				}
				else if(dir == 1) {
					if(id == *pID) {
						*q = 0;
						strcpy(pCode, p);
						return 1;
					}
				}
			}
		}
	}
	if(dir == 0) {
		ASSIGN_PTR(pID, 0);
	}
	else if(dir == 1)
		pCode[0] = 0;
	return 0;
}

int BnkOrdTaxMarkersDialog::setDTS(const PPBankingOrder * pData)
{
	Data = *pData;

	long   id = 0;
	SString buf;
	setCtrlData(CTL_TXM_TAXCLASS, Data.Txm.TaxClass2);
	if(*strip(Data.Txm.OKATO) == 0 && Data.RcvrID) {
		PPObjPerson psn_obj;
		psn_obj.GetRegNumber(Data.RcvrID, PPREGT_OKATO, buf);
		buf.CopyTo(Data.Txm.OKATO, sizeof(Data.Txm.OKATO));
	}
	setCtrlData(CTL_TXM_OKATO, Data.Txm.OKATO);
	setCtrlString(CTL_TXM_TAXPERIOD, Data.Txm.Period.Format(buf.Z()));
	disableCtrl(CTL_TXM_TAXPERIOD, 1);
	translateCode(0, PPTXT_TAXPAYMREASON, Data.Txm.Reason, &(id = 0));
	SetupStringCombo(this, CTLSEL_TXM_REASON, PPTXT_TAXPAYMREASON, id);
	setCtrlData(CTL_TXM_DOCNUMBER, Data.Txm.DocNumber);
	setCtrlData(CTL_TXM_DOCDATE, &Data.Txm.DocDate);
	translateCode(0, PPTXT_TAXPAYMTYPE, Data.Txm.PaymType, &(id = 0));
	setCtrlData(CTL_TXM_UIN, Data.Txm.UIN); // @v8.0.9
	SetupStringCombo(this, CTLSEL_TXM_PAYMTYPE, PPTXT_TAXPAYMTYPE, id);
	return 1;
}

int BnkOrdTaxMarkersDialog::getDTS(PPBankingOrder * pData)
{
	getCtrlData(CTL_TXM_TAXCLASS, Data.Txm.TaxClass2);
	getCtrlData(CTL_TXM_OKATO, Data.Txm.OKATO);
	long   id = getCtrlLong(CTLSEL_TXM_REASON);
	translateCode(1, PPTXT_TAXPAYMREASON, Data.Txm.Reason, &id);
	id = getCtrlLong(CTLSEL_TXM_PAYMTYPE);
	translateCode(1, PPTXT_TAXPAYMTYPE, Data.Txm.PaymType, &id);
	getCtrlData(CTL_TXM_UIN, Data.Txm.UIN); // @v8.0.9
	getCtrlData(CTL_TXM_DOCNUMBER, Data.Txm.DocNumber);
	getCtrlData(CTL_TXM_DOCDATE, &Data.Txm.DocDate);
	if(pData)
		pData->Txm = Data.Txm;
	return 1;
}
//
// BankingOrder (declared in PPDLGS.H)
//
BankingOrderDialog::BankingOrderDialog() : TDialog(DLG_BNKPAYM), PayerValidCode(-1), RcvrValidCode(-1)
{
	SetupCalCtrl(CTLCAL_BNKPAYM_DT, this, CTL_BNKPAYM_DT, 4);
	Ptb.SetBrush(brushValidNumber,   SPaintObj::bsSolid, GetColorRef(SClrAqua),  0);
	Ptb.SetBrush(brushInvalidNumber, SPaintObj::bsSolid, GetColorRef(SClrCoral), 0);
}

void BankingOrderDialog::setupVAT()
{
	long   vat_rate = getCtrlLong(CTL_BNKPAYM_VATRATE);
	if(vat_rate > 0) {
		double amount  = getCtrlReal(CTL_BNKPAYM_AMOUNT);
		setCtrlReal(CTL_BNKPAYM_VATSUM, amount * vat_rate / (vat_rate + 100L));
	}
}

void BankingOrderDialog::swapPersons()
{
	PPID   temp_id;
	getCtrlData(CTLSEL_BNKPAYM_PAYERKIND, &Data.PayerKindID);
	getCtrlData(CTLSEL_BNKPAYM_RCVRKIND,  &Data.RcvrKindID);
	getCtrlData(CTLSEL_BNKPAYM_PAYER,     &Data.PayerID);
	getCtrlData(CTLSEL_BNKPAYM_RCVR,      &Data.RcvrID);
	getCtrlData(CTLSEL_BNKPAYM_PAYERACC,  &Data.PayerBnkAccID);
	getCtrlData(CTLSEL_BNKPAYM_RCVRACC,   &Data.RcvrBnkAccID);

	temp_id = Data.PayerKindID;
	Data.PayerKindID = Data.RcvrKindID;
	Data.RcvrKindID = temp_id;

	temp_id = Data.PayerID;
	Data.PayerID = Data.RcvrID;
	Data.RcvrID = temp_id;

	temp_id = Data.PayerBnkAccID;
	Data.PayerBnkAccID = Data.RcvrBnkAccID;
	Data.RcvrBnkAccID = temp_id;

	setCtrlData(CTLSEL_BNKPAYM_PAYERKIND, &Data.PayerKindID);
	setCtrlData(CTLSEL_BNKPAYM_RCVRKIND,  &Data.RcvrKindID);
	SetupPersonCombo(this, CTLSEL_BNKPAYM_PAYER, Data.PayerID, OLW_CANINSERT, Data.PayerKindID, 0);
	SetupPersonCombo(this, CTLSEL_BNKPAYM_RCVR, Data.RcvrID, OLW_CANINSERT, Data.RcvrKindID, 0);
	setupPerson(0, Data.PayerID, &Data.PayerBnkAccID);
	setupPerson(1, Data.RcvrID,  &Data.RcvrBnkAccID);
}

int BankingOrderDialog::editTaxMarkers()
{
	getCtrlData(CTLSEL_BNKPAYM_RCVR, &Data.RcvrID);
	DIALOG_PROC_BODY(BnkOrdTaxMarkersDialog, &Data);
}

IMPL_HANDLE_EVENT(BankingOrderDialog)
{
	PPID   person_kind_id;
	TDialog::handleEvent(event);
	if(event.isCmd(cmSwap))
		swapPersons();
	else if(event.isCmd(cmTaxMarkers))
		editTaxMarkers();
	else if(event.isCbSelected(CTLSEL_BNKPAYM_PAYERKIND)) {
		person_kind_id = getCtrlLong(CTLSEL_BNKPAYM_PAYERKIND);
		SetupPersonCombo(this, CTLSEL_BNKPAYM_PAYER, 0, OLW_CANINSERT, person_kind_id, 0);
		setupPerson(0, 0, 0);
	}
	else if(event.isCbSelected(CTLSEL_BNKPAYM_RCVRKIND)) {
		person_kind_id = getCtrlLong(CTLSEL_BNKPAYM_RCVRKIND);
		SetupPersonCombo(this, CTLSEL_BNKPAYM_RCVR, 0, OLW_CANINSERT, person_kind_id, 0);
		setupPerson(1, 0, 0);
	}
	else if(event.isCbSelected(CTLSEL_BNKPAYM_PAYER))
		setupPerson(0, getCtrlLong(CTLSEL_BNKPAYM_PAYER), 0);
	else if(event.isCbSelected(CTLSEL_BNKPAYM_RCVR))
		setupPerson(1, getCtrlLong(CTLSEL_BNKPAYM_RCVR), 0);
	else if(event.isCbSelected(CTLSEL_BNKPAYM_PAYERACC))
		setupBnkAcc(0, getCtrlLong(CTLSEL_BNKPAYM_PAYERACC));
	else if(event.isCbSelected(CTLSEL_BNKPAYM_RCVRACC))
		setupBnkAcc(1, getCtrlLong(CTLSEL_BNKPAYM_RCVRACC));
	else if(TVCOMMAND && TVCMD == cmCtlColor) {
		TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
		if(p_dc && oneof2(p_dc->H_Ctl, getCtrlHandle(CTL_BNKPAYM_PAYERACC), getCtrlHandle(CTL_BNKPAYM_RCVRACC))) {
			int valid_code = (p_dc->H_Ctl == getCtrlHandle(CTL_BNKPAYM_PAYERACC)) ? PayerValidCode : RcvrValidCode;
			if(valid_code > 0) {
				::SetBkMode(p_dc->H_DC, TRANSPARENT);
				p_dc->H_Br = (HBRUSH)Ptb.Get(brushValidNumber);
			}
			else {
				::SetBkMode(p_dc->H_DC, TRANSPARENT);
				p_dc->H_Br = (HBRUSH)Ptb.Get(brushInvalidNumber);
			}
		}
		else
			return;
	}
	else if(TVBROADCAST && TVCMD == cmChangedFocus && TVINFOVIEW) {
		if(event.isCtlEvent(CTL_BNKPAYM_VATRATE) || event.isCtlEvent(CTL_BNKPAYM_AMOUNT))
			setupVAT();
		else if(event.isCtlEvent(CTL_BNKPAYM_VATSUM)) {
			long   vat_rate = getCtrlLong(CTL_BNKPAYM_VATRATE);
			double amount  = getCtrlReal(CTL_BNKPAYM_AMOUNT);
			double vat_sum = getCtrlReal(CTL_BNKPAYM_VATSUM);
			double calc_rate = R2(100.0 * vat_sum / (amount - vat_sum));
			if(calc_rate != (double)vat_rate)
				setCtrlLong(CTL_BNKPAYM_VATRATE, 0);
		}
	}
	else
		return;
	clearEvent(event);
}

int BankingOrderDialog::setupBnkAcc(int payerOrRcvr, PPID bnkAccID)
{
	int    ok = 1;
	uint   bnk_ctl = (payerOrRcvr == 0) ? CTL_BNKPAYM_PAYERBNK : CTL_BNKPAYM_RCVRBNK;
	int * p_valid_code = (payerOrRcvr == 0) ? &PayerValidCode : &RcvrValidCode;
	SString bnk_name;
	ASSIGN_PTR(p_valid_code, 0);
	if(bnkAccID) {
		BnkAcctData bnk_data(BADIF_INITBNAME);
		PsnObj.GetBnkAcctData(bnkAccID, (const PPBankAccount *)0, &bnk_data);
		bnk_name = bnk_data.Bnk.Name;
		ASSIGN_PTR(p_valid_code, CheckBnkAcc(bnk_data.Acct, bnk_data.Bnk.BIC));
	}
	setCtrlString(bnk_ctl, bnk_name);
	return ok;
}

int BankingOrderDialog::setupPerson(int payerOrRcvr, PPID personID, PPID * pBnkAcctID)
{
	int    ok = 1;
	uint   person_ctl, bacc_ctl;
	if(payerOrRcvr == 0) {
		person_ctl = CTLSEL_BNKPAYM_PAYER;
		bacc_ctl   = CTLSEL_BNKPAYM_PAYERACC;
	}
	else {
		person_ctl = CTLSEL_BNKPAYM_RCVR;
		bacc_ctl   = CTLSEL_BNKPAYM_RCVRACC;
	}
	PPID   bacc_id = DEREFPTRORZ(pBnkAcctID);
	PsnObj.GetSingleBnkAcct(personID, 0, &bacc_id, 0);
	setCtrlData(person_ctl, &personID);
	{
		BnkAccFilt.Oid.Set(PPOBJ_PERSON, personID);
		BnkAccFilt.RegTypeID = PPREGT_BANKACCOUNT;
		SetupPPObjCombo(this, bacc_ctl, PPOBJ_REGISTER, bacc_id, OLW_CANINSERT, &BnkAccFilt);
	}
	setupBnkAcc(payerOrRcvr, bacc_id);
	ASSIGN_PTR(pBnkAcctID, bacc_id);
	return ok;
}

int BankingOrderDialog::setDTS(PPBankingOrder * pData)
{
	int    ok = 1;
	ushort v = 0;
	Data = *pData;
	setCtrlUInt16(CTL_BNKPAYM_WHAT, BIN(Data.Flags & BNKPAYMF_REQ));
	setCtrlUInt16(CTL_BNKPAYM_ACCEPT, BIN(Data.Flags & BNKPAYMF_WOACCEPT));
	setCtrlData(CTL_BNKPAYM_DT, &Data.Dt);
	setCtrlData(CTL_BNKPAYM_BILLNO, Data.Code);
	SetupPPObjCombo(this,  CTLSEL_BNKPAYM_PAYERKIND, PPOBJ_PRSNKIND, Data.PayerKindID, 0);
	SetupPersonCombo(this, CTLSEL_BNKPAYM_PAYER,     Data.PayerID, OLW_CANINSERT, Data.PayerKindID, 0);
	SetupPPObjCombo(this,  CTLSEL_BNKPAYM_RCVRKIND,  PPOBJ_PRSNKIND, Data.RcvrKindID, 0);
	SetupPersonCombo(this, CTLSEL_BNKPAYM_RCVR,      Data.RcvrID, OLW_CANINSERT, Data.RcvrKindID, 0);
	setupPerson(0, Data.PayerID, &Data.PayerBnkAccID);
	setupPerson(1, Data.RcvrID,  &Data.RcvrBnkAccID);
	switch(Data.BnkPaymMethod) {
		case BNKPAYMMETHOD_UNDEF:     v = 0; break;
		case BNKPAYMMETHOD_MAIL:      v = 1; break;
		case BNKPAYMMETHOD_TELEGRAPH: v = 2; break;
		case BNKPAYMMETHOD_EMAIL:     v = 3; break;
		case BNKPAYMMETHOD_URGENTLY:  v = 4; break;
		default: v = 3; break;
	}
	setCtrlData(CTL_BNKPAYM_METHOD, &v);
	setCtrlData(CTL_BNKPAYM_PAYERSTATUS, &Data.PayerStatus);
	setCtrlData(CTL_BNKPAYM_QUEUEING, &Data.BnkQueueing);
	setCtrlData(CTL_BNKPAYM_AMOUNT,   &Data.Amount);
	setCtrlData(CTL_BNKPAYM_VATRATE,  &Data.VATRate);
	setCtrlData(CTL_BNKPAYM_VATSUM,   &Data.VATSum);
	disableCtrls(1, CTL_BNKPAYM_PAYERBNK, CTL_BNKPAYM_RCVRBNK, 0);
	return ok;
}

int BankingOrderDialog::getDTS(PPBankingOrder * pData)
{
	int    ok = 1;
	ushort v = 0;
	getCtrlData(CTL_BNKPAYM_WHAT, &v);
	SETFLAG(Data.Flags, BNKPAYMF_REQ, v == 1);
	getCtrlData(CTL_BNKPAYM_ACCEPT, &(v = 0));
	SETFLAG(Data.Flags, BNKPAYMF_WOACCEPT, v == 1);
	getCtrlData(CTL_BNKPAYM_DT,     &Data.Dt);
	getCtrlData(CTL_BNKPAYM_BILLNO, Data.Code);
	getCtrlData(CTLSEL_BNKPAYM_PAYERKIND, &Data.PayerKindID);
	getCtrlData(CTLSEL_BNKPAYM_RCVRKIND,  &Data.RcvrKindID);
	getCtrlData(CTLSEL_BNKPAYM_PAYER,     &Data.PayerID);
	getCtrlData(CTLSEL_BNKPAYM_RCVR,      &Data.RcvrID);
	getCtrlData(CTLSEL_BNKPAYM_PAYERACC,  &Data.PayerBnkAccID);
	getCtrlData(CTLSEL_BNKPAYM_RCVRACC,   &Data.RcvrBnkAccID);
	getCtrlData(CTL_BNKPAYM_METHOD, &(v = 0));
	switch(v) {
		case 0:  Data.BnkPaymMethod  = BNKPAYMMETHOD_UNDEF;     break;
		case 1:  Data.BnkPaymMethod  = BNKPAYMMETHOD_MAIL;      break;
		case 2:  Data.BnkPaymMethod  = BNKPAYMMETHOD_TELEGRAPH; break;
		case 3:  Data.BnkPaymMethod  = BNKPAYMMETHOD_EMAIL;     break;
		case 4:  Data.BnkPaymMethod  = BNKPAYMMETHOD_URGENTLY;  break;
		default: Data.BnkPaymMethod  = BNKPAYMMETHOD_DEFAULT;   break;
	}
	getCtrlData(CTL_BNKPAYM_PAYERSTATUS, &Data.PayerStatus);
	getCtrlData(CTL_BNKPAYM_QUEUEING, &Data.BnkQueueing);
	getCtrlData(CTL_BNKPAYM_AMOUNT,   &Data.Amount);
	Data.Amount = R2(Data.Amount);
	getCtrlData(CTL_BNKPAYM_VATRATE,  &Data.VATRate);
	getCtrlData(CTL_BNKPAYM_VATSUM,   &Data.VATSum);
	ASSIGN_PTR(pData, Data);
	return ok;
}

