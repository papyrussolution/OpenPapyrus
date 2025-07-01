// GTAXDLG.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2005, 2007, 2016, 2017, 2018, 2019, 2020, 2021, 2024, 2025
//
#include <pp.h>
#pragma hdrstop

class GoodsTaxDialog : public TDialog {
	DECL_DIALOG_DATA(PPGoodsTaxPacket);
	enum VatRateType {
		vrtSimple  = 1,
		vrtSpecial = 2,
		vrtGeneral = 3
	};
	long FlagsToVatRateType(long flags)
	{
		long vrt = vrtSimple;
		const long _f = flags & (GTAXF_SPCVAT|GTAXF_GENERALVAT);
		if(_f == (GTAXF_SPCVAT|GTAXF_GENERALVAT) || _f == 0)
			vrt = vrtSimple;
		else if(_f == GTAXF_SPCVAT)
			vrt = vrtSpecial;
		else if(_f == GTAXF_GENERALVAT)
			vrt = vrtGeneral;
		else {
			assert(0); // @unreachable
		}
		assert(oneof3(vrt, vrtSimple, vrtSpecial, vrtGeneral));
		return vrt;
	}
	void VatRateTypeToFlags(long vrt, long & rFlags)
	{
		rFlags &= ~(GTAXF_SPCVAT|GTAXF_GENERALVAT); 
		switch(vrt)	{
			case vrtSimple: 
				break;
			case vrtSpecial: 
				rFlags |= GTAXF_SPCVAT; 
				break;
			case vrtGeneral: 
				rFlags |= GTAXF_GENERALVAT; 
				break;
		}
	}
public:
	GoodsTaxDialog(uint dlgID) : TDialog(dlgID)
	{
		SetupCalPeriod(CTLCAL_GDSTAX_PERIOD, CTL_GDSTAX_PERIOD);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		PPGoodsTaxEntry gtx;
		setCtrlData(CTL_GDSTAX_NAME, Data.Rec.Name);
		setCtrlData(CTL_GDSTAX_SYMB, Data.Rec.Symb);
		setCtrlData(CTL_GDSTAX_ID,  &Data.Rec.ID);
		AddClusterAssoc(CTL_GDSTAX_FLAGS, 0, GTAXF_USELIST);
		SetClusterData(CTL_GDSTAX_FLAGS, Data.Rec.Flags);
		enableCommand(cmGoodsTaxList, BIN(Data.Rec.Flags & GTAXF_USELIST));
		AddClusterAssoc(CTL_GDSTAX_NOLOTEXCISE, 0, GTAXF_NOLOTEXCISE);
		SetClusterData(CTL_GDSTAX_NOLOTEXCISE, Data.Rec.Flags);
		Data.Rec.ToEntry(&gtx);
		return SetEntry(&gtx);
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		PPGoodsTaxEntry gtx;
		getCtrlData(CTL_GDSTAX_NAME, Data.Rec.Name);
		getCtrlData(CTL_GDSTAX_SYMB, Data.Rec.Symb);
		if(*strip(Data.Rec.Name) == 0)
			GTxObj.GetDefaultName(&Data.Rec, Data.Rec.Name, sizeof(Data.Rec.Name));
		if(Data.Rec.ID == 0)
			getCtrlData(CTL_GDSTAX_ID, &Data.Rec.ID);
		if(GetEntry(&gtx)) {
			Data.Rec.FromEntry(&gtx);
			GetClusterData(CTL_GDSTAX_FLAGS, &Data.Rec.Flags);
			GetClusterData(CTL_GDSTAX_NOLOTEXCISE, &Data.Rec.Flags);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
		else
			ok = 0;
		return ok;
	}
	int    SetEntry(const PPGoodsTaxEntry *);
	int    GetEntry(PPGoodsTaxEntry *);
private:
	DECL_HANDLE_EVENT
	{
		ushort v;
		TDialog::handleEvent(event);
		if(TVCOMMAND) {
			if(TVCMD == cmGoodsTaxList) {
				getCtrlData(CTL_GDSTAX_FLAGS, &(v = 0));
				if(v & 0x01)
					EditList();
				clearEvent(event);
			}
			else if(event.isClusterClk(CTL_GDSTAX_FLAGS)) {
				getCtrlData(CTL_GDSTAX_FLAGS, &(v = 0));
				enableCommand(cmGoodsTaxList, BIN(v & 0x01));
				clearEvent(event);
			}
			else if(event.isClusterClk(CTL_GDSTAX_SPCVAT)) {
				const long vrt = GetClusterData(CTL_GDSTAX_SPCVAT);
				disableCtrl(CTL_GDSTAX_VAT, vrt == vrtGeneral);
			}
		}
	}
	void   EditList();
	PPGoodsTaxEntry Entry;
	PPObjGoodsTax GTxObj;
};

int GoodsTaxDialog::SetEntry(const PPGoodsTaxEntry * pEntry)
{
	Entry = *pEntry;
	SString temp_buf;
	if(Entry.Flags & GTAXF_ENTRY) {
		SetPeriodInput(this, CTL_GDSTAX_PERIOD, Entry.Period);
		SetupOprKindCombo(this, CTLSEL_GDSTAX_OP, Entry.OpID, 0, 0, 0);
	}
	setCtrlString(CTL_GDSTAX_VAT, Entry.FormatVAT(temp_buf));
	// @v12.2.3 {
	{
		//GTAXF_SPCVAT
		AddClusterAssocDef(CTL_GDSTAX_SPCVAT, 0, vrtSimple);
		AddClusterAssoc(CTL_GDSTAX_SPCVAT, 1, vrtSpecial);
		AddClusterAssoc(CTL_GDSTAX_SPCVAT, 2, vrtGeneral);
		const long vrt = FlagsToVatRateType(Entry.Flags);
		SetClusterData(CTL_GDSTAX_SPCVAT, vrt);
		disableCtrl(CTL_GDSTAX_VAT, vrt == vrtGeneral);
	}
	// } @v12.2.3 
	setCtrlString(CTL_GDSTAX_EXCISE, Entry.FormatExcise(temp_buf));
	setCtrlString(CTL_GDSTAX_STAX,   Entry.FormatSTax(temp_buf));
	GTxObj.FormatOrder(Entry.Order, Entry.UnionVect, temp_buf);
	setCtrlString(CTL_GDSTAX_ORDER, temp_buf);
	return 1;
}

int GoodsTaxDialog::GetEntry(PPGoodsTaxEntry * pEntry)
{
	int    ok = 1;
	SString temp_buf;
	double rv;
	if(Entry.Flags & GTAXF_ENTRY) {
		GetPeriodInput(this, CTL_GDSTAX_PERIOD, &Entry.Period);
		getCtrlData(CTLSEL_GDSTAX_OP, &Entry.OpID);
	}
	getCtrlString(CTL_GDSTAX_VAT, temp_buf);
	rv = temp_buf.ToReal();
	Entry.VAT = R0i(rv * 100L);
	// @v12.2.5 {
	{
		const long vrt = GetClusterData(CTL_GDSTAX_SPCVAT);
		VatRateTypeToFlags(vrt, Entry.Flags);
	}
	// } @v12.2.5 
	Entry.Flags &= ~GTAXF_ABSEXCISE;
	getCtrlString(CTL_GDSTAX_EXCISE, temp_buf);
	if(temp_buf.NotEmptyS()) {
		size_t dollar_pos = 0;
		if(temp_buf.SearchChar('$', &dollar_pos)) {
			temp_buf.Trim(dollar_pos);
			Entry.Flags |= GTAXF_ABSEXCISE;
		}
		rv = temp_buf.ToReal();
		Entry.Excise = R0i(rv * 100L);
	}
	else
		Entry.Excise = 0;
	if(Entry.Excise == 0)
		Entry.Flags &= ~GTAXF_ABSEXCISE;
	getCtrlString(CTL_GDSTAX_STAX, temp_buf);
	rv = temp_buf.ToReal();
	Entry.SalesTax = R0i(rv * 100L);
	getCtrlString(CTL_GDSTAX_ORDER, temp_buf);
	if(GTxObj.StrToOrder(temp_buf, &Entry.Order, &Entry.UnionVect)) {
		*pEntry = Entry;
	}
	else {
		selectCtrl(CTL_GDSTAX_ORDER);
		ok = (PPError(PPERR_INVEXPR, temp_buf), 0); // @todo (err code)
	}
	return ok;
}

class GoodsTaxListDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPGoodsTaxPacket);
public:
	GoodsTaxListDialog() : PPListDialog(DLG_GDSTAXLST, CTL_GDSTAXLST_LIST)
	{
		disableCtrl(CTL_GDSTAXLST_NAME, true);
		updateList(-1);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_GDSTAXLST_NAME, Data.Rec.Name);
		setCtrlData(CTL_GDSTAXLST_ID, &Data.Rec.ID);
		disableCtrl(CTL_GDSTAXLST_ID, true);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTL_GDSTAXLST_NAME, Data.Rec.Name);
		*pData = Data;
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	virtual int  setupList();
	virtual int  addItem(long * pPos, long * pID);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);
	int    editItemDialog(int, PPGoodsTaxEntry *);
	void   addBySample();
};

IMPL_HANDLE_EVENT(GoodsTaxListDialog)
{
	PPListDialog::handleEvent(event);
	if(TVKEYDOWN && TVKEY == kbAltF2) {
		addBySample();
		clearEvent(event);
	}
}

int GoodsTaxListDialog::setupList()
{
	SString temp_buf;
	StringSet ss(SLBColumnDelim);
	for(uint i = 0; i < Data.GetCount(); i++) {
		const PPGoodsTaxEntry & r_item = Data.Get(i);
		// @v12.3.7 char   sub[64];
		ss.Z();
		// @v12.3.7 ss.add(periodfmt(r_item.Period, sub));
		ss.add(r_item.Period.ToStr(0, temp_buf)); // @v12.3.7
		GetOpName(r_item.OpID, temp_buf);
		ss.add(temp_buf);
		ss.add(r_item.FormatExcise(temp_buf));
		ss.add(r_item.FormatVAT(temp_buf));
		ss.add(r_item.FormatSTax(temp_buf));
		if(!addStringToList(i, ss.getBuf()))
			return 0;
	}
	return 1;
}

int GoodsTaxListDialog::delItem(long pos, long)
{
	int    ok = -1;
	if(pos >= 0) {
		Data.PutEntry(pos, 0);
		ok = 1;
	}
	return ok;
}

int GoodsTaxListDialog::editItemDialog(int pos, PPGoodsTaxEntry * pEntry)
{
	int    ok = -1;
	GoodsTaxDialog * dlg = new GoodsTaxDialog(DLG_GDSTAXENTRY);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->SetEntry(pEntry);
		dlg->setCtrlData(CTL_GDSTAX_NAME, Data.Rec.Name);
		dlg->disableCtrl(CTL_GDSTAX_NAME, true);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
			if(dlg->GetEntry(pEntry) && Data.PutEntry(pos, pEntry))
				ok = valid_data = 1;
			else
				PPError();
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}

void GoodsTaxListDialog::addBySample()
{
	long   p, i;
	if(getCurItem(&p, &i)) {
		PPGoodsTaxEntry item = Data.Get((uint)p);
		if(editItemDialog(-1, &item) > 0)
			updateList(-1);
	}
}

int GoodsTaxListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	PPGoodsTaxEntry item;
	Data.Rec.ToEntry(&item);
	item.Flags |= GTAXF_ENTRY;
	item.Flags &= ~GTAXF_USELIST;
	if(editItemDialog(-1, &item) > 0) {
		const long _c = (long)Data.GetCount();
		ASSIGN_PTR(pPos, _c-1);
		ASSIGN_PTR(pID, _c);
		ok = 1;
	}
	return ok;
}

int GoodsTaxListDialog::editItem(long pos, long)
{
	int    ok = -1;
	if(pos >= 0 && pos < (long)Data.GetCount()) {
		PPGoodsTaxEntry item = Data.Get(static_cast<uint>(pos));
		if(editItemDialog((int)pos, &item) > 0)
			ok = 1;
	}
	return ok;
}

void GoodsTaxDialog::EditList()
{
	GoodsTaxListDialog * dlg = new GoodsTaxListDialog();
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(&Data);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
			if(dlg->getDTS(&Data))
				valid_data = 1;
	}
	delete dlg;
}

int PPObjGoodsTax::AddBySample(PPID * pID, long sampleID)
{
	int    r = cmCancel;
	PPGoodsTaxPacket pack;
	GoodsTaxDialog * dlg = 0;
	if(GetPacket(sampleID, &pack) > 0) {
		pack.Rec.ID = 0;
		for(uint i = 0; i < pack.GetCount(); i++) {
			pack.Get(i).TaxGrpID = 0;
		}
		if(CheckDialogPtrErr(&(dlg = new GoodsTaxDialog(DLG_GDSTAX)))) {
			dlg->setDTS(&pack);
			for(int valid_data = 0; !valid_data && (r = ExecView(dlg)) == cmOK;)
				if(dlg->getDTS(&pack))
					if(!CheckName(*pID, pack.Rec.Name, 0))
						dlg->selectCtrl(CTL_GDSTAX_NAME);
					else if(PutPacket(pID, &pack, 1)) {
						Dirty(*pID);
						valid_data = 1;
					}
		}
	}
	delete dlg;
	return r;
}

//#define TEST_GTAX

int PPObjGoodsTax::Edit(PPID * pID, void * extraPtr)
{
	int    r = cmCancel;
	int    valid_data = 0;
	bool   is_new = false;
	PPGoodsTaxPacket pack;
	GoodsTaxDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new GoodsTaxDialog(DLG_GDSTAX))));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	dlg->setDTS(&pack);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		THROW(is_new || CheckRights(PPR_MOD));
		if(dlg->getDTS(&pack)) {
			if(CheckDupName(*pID, pack.Rec.Name) && P_Ref->CheckUniqueSymb(Obj, *pID, pack.Rec.Symb, offsetof(PPGoodsTax, Symb))) {
				if(PutPacket(pID, &pack, 1)) {
					Dirty(*pID);
					valid_data = 1;
				}
			}
			else
				PPErrorByDialog(dlg, CTL_GDSTAX_NAME);
		}
	}
	CATCH
		r = PPErrorZ();
	ENDCATCH
	delete dlg;
#ifdef TEST_GTAX
	Test(*pID);
#endif
	return r;
}

