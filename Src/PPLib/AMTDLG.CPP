// AMTDLG.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2010, 2014, 2015, 2016, 2017, 2018, 2020, 2024
//
#include <pp.h>
#pragma hdrstop

struct AmtDialogEntry {
	PPID   AmtTypeID;
	PPID   CurID;
	double Amt;
	double CRate;
	double BaseAmt;
	LDATE  CRateDate;
};

class AmountDialog : public TDialog {
	DECL_DIALOG_DATA(AmtDialogEntry);
public:
	enum {
		ctlgroupCurAmt = 1
	};
	AmountDialog(int isCanEdit, AmtList * pList, PPIDArray * pRestrictList) :
		TDialog(DLG_BILLAMTCUR), CanEdit(isCanEdit), P_RestrictList(pRestrictList)
	{
		CurAmtCtrlGroup * p_ca_grp = new CurAmtCtrlGroup(CTL_BILLAMT_AMOUNT,
			CTLSEL_BILLAMT_CUR, CTL_BILLAMT_CRATE, CTL_BILLAMT_BASEAMT, 0, cmBillAmtSelRate, pList);
		addGroup(ctlgroupCurAmt, p_ca_grp);
	}
	DECL_DIALOG_SETDTS()
	{
		CurAmtCtrlGroup::Rec ca_rec;
		RVALUEPTR(Data, pData);
		if(Data.AmtTypeID == 0 && P_RestrictList && P_RestrictList->getCount() == 1)
			Data.AmtTypeID = P_RestrictList->at(0);
		SetupAmtTypeCombo(this, CTLSEL_BILLAMT_AMTTYPE, Data.AmtTypeID, OLW_CANINSERT, 0, P_RestrictList);
		disableCtrl(CTL_BILLAMT_BASEAMT, 1);
		disableCtrls(!CanEdit, CTLSEL_BILLAMT_AMTTYPE, CTLSEL_BILLAMT_CUR, 0);
		disableCtrls(Data.AmtTypeID == PPAMT_CRATE, CTL_BILLAMT_CRATE, 0);
		if(!CanEdit)
			selectCtrl(CTL_BILLAMT_AMOUNT);
		ca_rec.Amount = Data.Amt;
		ca_rec.CurID  = Data.CurID;
		ca_rec.CRate  = Data.CRate;
		ca_rec.BaseAmount = Data.BaseAmt;
		ca_rec.CRateDate = Data.CRateDate;
		setGroupData(ctlgroupCurAmt, &ca_rec);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		CurAmtCtrlGroup::Rec ca_rec;
		getCtrlData(CTLSEL_BILLAMT_AMTTYPE, &Data.AmtTypeID);
		getGroupData(ctlgroupCurAmt, &ca_rec);
		Data.CurID   = ca_rec.CurID;
		Data.Amt     = ca_rec.Amount;
		Data.CRate   = ca_rec.CRate;
		Data.BaseAmt = ca_rec.BaseAmount;
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	int    CanEdit;
	PPIDArray * P_RestrictList; // Not owned by this
};
//
//
//
int AmountListDialog(AmtList * pData, PPIDArray * pRestrictList, LDATE crateDate, const char * pTitle, uint options)
{
	int    r = -1;
	AmtListDialog * dlg = 0;
	if(pData) {
		dlg = new AmtListDialog(DLG_EXAMTLIST, CTL_EXAMTLIST_LIST, 1, pData, pRestrictList, pTitle, options);
		if(CheckDialogPtrErr(&dlg)) {
			dlg->setCRateDate(crateDate);
			if(ExecView(dlg) == cmOK) {
				dlg->getDTS(pData);
				r = 1;
			}
		}
		else
			r = 0;
	}
	delete dlg;
	return r;
}
//
// AmtListDialog implementation
//
AmtListDialog::AmtListDialog(uint resID, uint listCtlID, int useCurCol, AmtList * pList,
	PPIDArray * pRestrictList, const char * pTitle, uint o) : PPListDialog(resID, listCtlID), CRateDate(ZERODATE), UseCurColumn(useCurCol), Options(o)
{
	setTitle(pTitle);
	Data.copy(*pList);
	if(pRestrictList) {
		HasRestrict = 1;
		RestrictList.copy(*pRestrictList);
	}
	else
		HasRestrict = 0;
	updateList(-1);
	enableCommand(cmaInsert, BIN(Options & OLW_CANINSERT));
	enableCommand(cmaDelete, BIN(Options & OLW_CANDELETE));
	enableCommand(cmaEdit,   BIN(Options & OLW_CANEDIT));
}

void AmtListDialog::setCRateDate(LDATE dt)
{
	CRateDate = dt;
}

int AmtListDialog::getDTS(AmtList * pList)
{
	pList->copy(Data);
	return 1;
}

int AmtListDialog::setupListEntry(PPID idx, const AmtEntry * pEntry)
{
	SString sub;
	StringSet ss(SLBColumnDelim);
	PPAmountType amt_rec;
	if(AmtObj.Fetch(pEntry->AmtTypeID, &amt_rec) > 0)
		sub = amt_rec.Name;
	else
		ideqvalstr(pEntry->AmtTypeID, sub);
	ss.add(sub);
	if(UseCurColumn) {
		PPObjCurrency cur_obj;
		PPCurrency cur_rec;
		if(pEntry->CurID == 0)
			sub.Z();
		else if(cur_obj.Fetch(pEntry->CurID, &cur_rec) > 0)
			sub = amt_rec.Symb;
		else
			ideqvalstr(pEntry->CurID, sub.Z());
		ss.add(sub);
	}
	ss.add(sub.Z().Cat(pEntry->Amt, SFMT_MONEY));
	if(addStringToList(pEntry->AmtTypeID, ss.getBuf())) {
		PosList.add(idx);
		return 1;
	}
	else
		return 0;
}

int AmtListDialog::setupList()
{
	PosList.clear();
	if(HasRestrict) {
		for(uint i = 0; i < RestrictList.getCount(); i++)
			if(!Data.HasAmtTypeID(RestrictList.at(i))) {
				AmtEntry entry(RestrictList.at(i));
				Data.Put(&entry, 0, 0);
			}
	}
	{
		AmtEntry * ae;
		for(uint i = 0; Data.enumItems(&i, (void **)&ae);)
			if(!HasRestrict || RestrictList.lsearch(ae->AmtTypeID))
				if(!setupListEntry(i-1, ae))
					return 0;
	}
	return 1;
}

int AmtListDialog::editAmount(int pos)
{
	int    ok = -1;
	int    valid_data = 0;
	int    can_edit = 0;
	const  int p = (pos >= 0) ? (int)PosList.at(static_cast<uint>(pos)) : -1;
	AmountDialog * dlg = 0;
	AmtEntry ae;
	AmtDialogEntry dlg_data;
	MEMSZERO(dlg_data);
	if(p >= 0) {
		ae = Data.at(p);
		dlg_data.AmtTypeID = ae.AmtTypeID;
		dlg_data.CurID = ae.CurID;
		dlg_data.Amt = ae.Amt;
		can_edit = 0;
	}
	else
		can_edit = 1;
	dlg_data.CRateDate = CRateDate;
	if(CheckDialogPtrErr(&(dlg = new AmountDialog(can_edit, &Data, HasRestrict ? &RestrictList : 0)))) {
		dlg->setDTS(&dlg_data);
		while(!valid_data && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&dlg_data)) {
				ae.AmtTypeID = dlg_data.AmtTypeID;
				ae.CurID = dlg_data.CurID;
				ae.Amt   = dlg_data.Amt;
				Data.Put(&ae, 0, 1);
				if(ae.CurID) {
					AmtEntry cae;
					cae.AmtTypeID = PPAMT_CRATE;
					cae.CurID = ae.CurID;
					cae.Amt   = dlg_data.CRate;
					Data.Put(&cae, 0, 1);
				}
				ok = valid_data = 1;
			}
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}

int AmtListDialog::editItem(long pos, long /*id*/) { return (Options & OLW_CANEDIT && checkupper((uint)pos, PosList.getCount())) ? editAmount((int)pos) : -1; }
int AmtListDialog::addItem(long *, long * /*pID*/) { return (Options & OLW_CANINSERT) ? editAmount(-1) : -1; }

int AmtListDialog::delItem(long pos, long /*id*/)
{
	int    ok = -1;
	if((Options & OLW_CANDELETE) && checkupper((uint)pos, PosList.getCount())) {
		uint   p = (uint)PosList.at(static_cast<uint>(pos));
		AmtEntry & ae = Data.at(p);
		if(ae.AmtTypeID == PPAMT_CRATE && ae.CurID) {
			AmtEntry * p_ae;
			for(uint i = 0; Data.enumItems(&i, (void **)&p_ae);)
				THROW_PP((i-1) == p || p_ae->CurID != ae.CurID, PPERR_CANTRMVCRATE);
		}
		else
			THROW_PP(ae.AmtTypeID != PPAMT_TRANSITCRATE, PPERR_CANTRMVCRATE);
		if(Data.atFree(p))
			ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
class StaffAmtListDialog : public PPListDialog {
	DECL_DIALOG_DATA(StaffAmtList);
public:
	StaffAmtListDialog(const char * pTitle, const StaffAmtList * pList) : PPListDialog(DLG_STAFFAMTLIST, CTL_STAFFAMTLIST_LIST)
	{
		setTitle(pTitle);
		Data = *pList;
		updateList(-1);
	}
	int    getDTS(StaffAmtList * pData)
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
};

int StaffAmtListDialog::setupList()
{
	int    ok = 1;
	PPObjAmountType at_obj;
	PPAmountType at_rec;
	PPObjCurrency cur_obj;
	PPCurrency cur_rec;
	SString sub;
	StringSet ss(SLBColumnDelim);
	StaffAmtEntry * p_entry;
	for(uint i = 0; Data.enumItems(&i, (void **)&p_entry);) {
		ss.Z();
		if(at_obj.Search(p_entry->AmtTypeID, &at_rec) > 0)
			sub = at_rec.Name;
		else
			ideqvalstr(p_entry->AmtTypeID, sub.Z());
		ss.add(sub);
		if(/*UseCurColumn*/1) {
			if(cur_obj.Search(p_entry->CurID, &cur_rec) > 0)
				sub = cur_rec.Symb;
			else if(p_entry->CurID > 0)
				ideqvalstr(p_entry->CurID, sub.Z());
			else
				sub.Z();
			ss.add(sub);
		}
		ss.add(sub.Z().Cat(p_entry->Period, 1));
		ss.add(sub.Z().Cat(p_entry->Amt, SFMT_MONEY));
		THROW(addStringToList(i, ss.getBuf()));
	}
	CATCHZOK
	return ok;
}

class StaffAmtEntryDlg : public TDialog {
	DECL_DIALOG_DATA(StaffAmtEntry);
public:
	StaffAmtEntryDlg() : TDialog(DLG_STAFFAMT)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		SetupCalPeriod(CTLCAL_BILLAMT_PERIOD, CTL_BILLAMT_PERIOD);
		SetupAmtTypeCombo(this, CTLSEL_BILLAMT_AMTTYPE, Data.AmtTypeID, OLW_CANINSERT, PPObjAmountType::selStaffOnly, 0);
		SetupCurrencyCombo(this, CTLSEL_BILLAMT_CUR,  Data.CurID, 0, 1, 0);
		setCtrlReal(CTL_BILLAMT_AMOUNT, Data.Amt);
		SetPeriodInput(this, CTL_BILLAMT_PERIOD, &Data.Period);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		ushort sel = 0;
		THROW_PP(Data.AmtTypeID = getCtrlLong(sel = CTLSEL_BILLAMT_AMTTYPE), PPERR_AMTTYPENEEDED);
		Data.CurID = getCtrlLong(CTLSEL_BILLAMT_CUR);
		getCtrlData(CTL_BILLAMT_AMOUNT, &Data.Amt);
		THROW(GetPeriodInput(this, sel = CTL_BILLAMT_PERIOD, &Data.Period));
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = (selectCtrl(sel), 0);
		ENDCATCH
		return ok;
	}
};

int EditStaffAmtEntry(long idx, StaffAmtList * pAmtList)
{
	int    ok = -1;
	StaffAmtEntryDlg * dlg = 0;
	THROW_INVARG(pAmtList);
	if(CheckDialogPtrErr(&(dlg = new StaffAmtEntryDlg))) {
		StaffAmtEntry entry;
		if(idx >= 0)
			entry = pAmtList->at(idx);
		dlg->setDTS(&entry);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(&entry) > 0) {
				if(!pAmtList->CheckDup(idx, &entry))
					PPErrorByDialog(dlg, CTL_BILLAMT_PERIOD);
				else {
					THROW((idx >= 0) ? pAmtList->Put(idx, &entry) : pAmtList->Add(&entry));
					ok = 1;
				}
			}
			else
				PPError();
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int StaffAmtListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	if(EditStaffAmtEntry(-1, &Data) > 0)
		ok = 1;
	else
		PPError();
	return ok;
}

int StaffAmtListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(checkupper((uint)pos, Data.getCount())) {
		if(EditStaffAmtEntry(pos, &Data) > 0)
			ok = 1;
		else
			PPError();
	}
	return ok;
}

int StaffAmtListDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(checkupper((uint)pos, Data.getCount()))
		if(Data.Put(pos, 0))
			ok = 1;
		else
			PPError();
	return ok;
}

int EditStaffAmtList(StaffAmtList * pData, const char * pTitle, int enableMod)
{
	int    ok = -1;
	StaffAmtListDialog * dlg = new StaffAmtListDialog(pTitle, pData);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->enableCommand(cmOK,      enableMod);
		dlg->enableCommand(cmaEdit,   enableMod);
		dlg->enableCommand(cmaInsert, enableMod);
		dlg->enableCommand(cmaDelete, enableMod);
		if(!enableMod) {
			SString text;
			PPLoadString("viewonly", text);
			dlg->setStaticText(CTL_STAFFAMTLIST_ST_INFO, text);
		}
		while(ok < 0 && ExecView(dlg) == cmOK) {
			if(!enableMod)
				break;
			else if(dlg->getDTS(pData))
				ok = 1;
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
class BillTaxesDialog : public TDialog {
	DECL_DIALOG_DATA(AmtList);
public:
	BillTaxesDialog() : TDialog(DLG_BILLTAX)
	{
		disableCtrl(CTL_BILLTAX_AMOUNT, 1);
	}
	int    setDTS(const AmtList * pData, const TaxAmountIDs * pTais);
	int    getDTS(AmtList * pData);
private:
	DECL_HANDLE_EVENT;
	void   calcTotal();
	void   calcTax(int taxN);
	void   setBillTaxCtrl(PPID amtTypeID, uint ctlText, uint ctlInput);
	void   getBillTaxCtrl(PPID amtTypeID, uint ctlInput);
	PPObjAmountType AmtObj;
	TaxAmountIDs Tais;
};

IMPL_HANDLE_EVENT(BillTaxesDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmaMore))
		calcTotal();
	else if(TVBROADCAST && TVCMD == cmChangedFocus) {
		TView * p_v = TVINFOVIEW;
		const uint ctl_id = p_v->GetId();
		if(oneof4(ctl_id, CTL_BILLTAX_VAT1, CTL_BILLTAX_VAT2, CTL_BILLTAX_VAT3, CTL_BILLTAX_STAX))
			calcTotal();
		else
			return;
	}
	else if(event.isKeyDown(kbF2) && P_Current) {
		int    tax_n = -1;
		switch(GetCurrId()) {
			case CTL_BILLTAX_STAX: tax_n = 0; break;
			case CTL_BILLTAX_VAT1: tax_n = 1; break;
			case CTL_BILLTAX_VAT2: tax_n = 2; break;
			case CTL_BILLTAX_VAT3: tax_n = 3; break;
			default: return;
		}
		calcTax(tax_n);
	}
	else
		return;
	clearEvent(event);
}

void BillTaxesDialog::calcTax(int taxN)
{
	uint   ctl_id = 0;
	double bill_amt = getCtrlReal(CTL_BILLTAX_BILLAMT);
	double amt = 0.0;
	double rate = 0.0;
	if(taxN == 0) { // Налог с продаж
		amt = bill_amt * SalesTaxMult(fdiv100i(Tais.STaxRate));
		ctl_id = CTL_BILLTAX_STAX;
	}
	else {
		if(taxN == 1) {
			rate = fdiv100i(Tais.VatRate[0]);
			ctl_id = CTL_BILLTAX_VAT1;
		}
		else if(taxN == 2) {
			rate = fdiv100i(Tais.VatRate[1]);
			ctl_id = CTL_BILLTAX_VAT2;
		}
		else if(taxN == 3) {
			rate = fdiv100i(Tais.VatRate[2]);
			ctl_id = CTL_BILLTAX_VAT3;
		}
		double stax = getCtrlReal(CTL_BILLTAX_STAX);
		amt = (bill_amt - stax) * SalesTaxMult(rate);
	}
	if(ctl_id) {
		setCtrlReal(ctl_id, amt);
		calcTotal();
	}
}

void BillTaxesDialog::calcTotal()
{
	double amt = 0.0;
	getDTS(0);
	int    tag_stax = 0;
	for(uint i = 0; i < Data.getCount(); i++) {
		const AmtEntry & r_ae = Data.at(i);
		PPAmountType amtt_rec;
		if(r_ae.Amt != 0.0 && AmtObj.Fetch(r_ae.AmtTypeID, &amtt_rec) > 0)
			if(amtt_rec.IsTax(GTAX_VAT))
				amt += r_ae.Amt / SalesTaxMult(fdiv100i(amtt_rec.TaxRate));
			else if(!tag_stax && amtt_rec.IsTax(GTAX_SALES)) {
				amt += r_ae.Amt;
				tag_stax = 1;
			}
	}
	setCtrlReal(CTL_BILLTAX_AMOUNT, amt);
}

void BillTaxesDialog::setBillTaxCtrl(PPID amtTypeID, uint ctlText, uint ctlInput)
{
	PPAmountType amtt_rec;
	if(amtTypeID) {
		AmtObj.Fetch(amtTypeID, &amtt_rec);
		setStaticText(ctlText, amtt_rec.Name);
		setCtrlReal(ctlInput, Data.Get(amtTypeID, 0L/*@curID*/));
	}
	else
		disableCtrl(ctlInput, 1);
}

void BillTaxesDialog::getBillTaxCtrl(PPID amtTypeID, uint ctlInput)
{
	if(amtTypeID)
		Data.Put(amtTypeID, 0L/*@curID*/, getCtrlReal(ctlInput), 0, 1);
}

int BillTaxesDialog::setDTS(const AmtList * pData, const TaxAmountIDs * pTais)
{
	Data.copy(*pData);
	Tais = *pTais;
	setBillTaxCtrl(Tais.VatAmtID[0], CTL_BILLTAX_T_VAT1, CTL_BILLTAX_VAT1);
	setBillTaxCtrl(Tais.VatAmtID[1], CTL_BILLTAX_T_VAT2, CTL_BILLTAX_VAT2);
	setBillTaxCtrl(Tais.VatAmtID[2], CTL_BILLTAX_T_VAT3, CTL_BILLTAX_VAT3);
	setBillTaxCtrl(Tais.STaxAmtID, CTL_BILLTAX_T_STAX, CTL_BILLTAX_STAX);
	calcTotal();
	return 1;
}

int BillTaxesDialog::getDTS(AmtList * pData)
{
	getBillTaxCtrl(Tais.VatAmtID[0], CTL_BILLTAX_VAT1);
	getBillTaxCtrl(Tais.VatAmtID[1], CTL_BILLTAX_VAT2);
	getBillTaxCtrl(Tais.VatAmtID[2], CTL_BILLTAX_VAT3);
	getBillTaxCtrl(Tais.STaxAmtID, CTL_BILLTAX_STAX);
	CALLPTRMEMB(pData, copy(Data));
	return 1;
}

int EditBillTaxes(AmtList * pAmounts, double amount)
{
	int    ok = -1;
	PPObjAmountType amtt_obj;
	TaxAmountIDs tais;
	amtt_obj.GetTaxAmountIDs(&tais, 0);
	if(tais.VatAmtID[0] || tais.VatAmtID[1] || tais.VatAmtID[2] || tais.STaxAmtID) {
		BillTaxesDialog * dlg = new BillTaxesDialog();
		if(CheckDialogPtrErr(&dlg)) {
			dlg->setDTS(pAmounts, &tais);
			dlg->setCtrlData(CTL_BILLTAX_BILLAMT, &amount);
			while(ok < 0 && ExecView(dlg) == cmOK)
				if(dlg->getDTS(pAmounts))
					ok = 1;
			delete dlg;
		}
		else
			ok = 0;
	}
	return ok;
}
