// V_AMTTYP.CPP
// Copyright (c) A.Starodub 2010, 2012, 2015, 2016, 2017, 2018, 2020, 2021
// @codepage UTF-8
//
// Типы сумм документа
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>

IMPLEMENT_PPFILT_FACTORY(AmountType); AmountTypeFilt::AmountTypeFilt() : PPBaseFilt(PPFILT_AMOUNTTYPE, 0, 0)
{
	SetFlatChunk(offsetof(AmountTypeFilt, ReserveStart), offsetof(AmountTypeFilt, ReserveEnd) - offsetof(AmountTypeFilt, ReserveStart));
	Init(1, 0);
}

AmountTypeFilt & FASTCALL AmountTypeFilt::operator = (const AmountTypeFilt & s)
{
	Copy(&s, 0);
	return *this;
}

int AmountTypeFilt::IsComplementary() const
{
	return BIN(Flags & (PPAmountType::fInAmount | PPAmountType::fOutAmount));
}

PPViewAmountType::PPViewAmountType() : 
	PPView(&ObjAmtT, &Filt, PPVIEW_AMOUNTTYPE, (implBrowseArray|implDontEditNullFilter), 0), Data(sizeof(AmountTypeViewItem))
{
}

PPViewAmountType::~PPViewAmountType()
{
}

int PPViewAmountType::CheckForFilt(const PPAmountTypePacket * pPack) const
{
	if(pPack) {
		long flags = pPack->Rec.Flags;
		if((Filt.Flags & PPAmountType::fTax) && !CheckFiltID(Filt.TaxID, pPack->Rec.Tax))
			return 0;
		if((Filt.Flags & (PPAmountType::fInAmount|PPAmountType::fOutAmount)) && !CheckFiltID(Filt.RefAmtTypeID, pPack->Rec.RefAmtTypeID))
			return 0;
		if((Filt.Flags & flags) != Filt.Flags)
			return 0;
	}
	return 1;
}

/*static*/int FASTCALL PPViewAmountType::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewAmountType * p_v = static_cast<PPViewAmountType *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

int PPViewAmountType::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		SString temp_buf;
		const AmountTypeViewItem * p_item = static_cast<const AmountTypeViewItem *>(pBlk->P_SrcData);
		switch(pBlk->ColumnN) {
			case 0: // ИД
				pBlk->Set(p_item->ID);
				break;
			case 1: // Наименование
				GetObjectName(PPOBJ_AMOUNTTYPE, p_item->ID, temp_buf);
				if(temp_buf.Len() == 0)
					ideqvalstr(p_item->ID, temp_buf);
				pBlk->Set(temp_buf);
				break;
			case 2: // Наименование типа налога
				if(p_item->Tax == GTAX_VAT) {
					// @v9.0.2 PPGetWord(PPWORD_VAT, 0, temp_buf);
					PPLoadString("vat", temp_buf); // @v9.0.2
				}
				else if(p_item->Tax == GTAX_SALES) {
			 		// @v9.2.6 PPGetWord(PPWORD_STAX, 0, temp_buf);
			 		PPLoadString("salestax", temp_buf); // @v9.2.6
				}
				pBlk->Set(temp_buf);
				break;
			case 3: // Налоговая ставка %
				pBlk->Set(p_item->TaxRate);
				break;
			case 4: // Комплементарная сумма
				if(p_item->RefAmtTypeID)
					GetObjectName(PPOBJ_AMOUNTTYPE, p_item->RefAmtTypeID, temp_buf);
				pBlk->Set(temp_buf);
				break;
			case 5: // Формула
				pBlk->Set(p_item->Formula);
				break;
			case 6: // @v10.0.0 Символ
				{
					PPAmountType amtt_rec;
					if(ObjAmtT.Fetch(p_item->ID, &amtt_rec) > 0)
						temp_buf = amtt_rec.Symb;
					pBlk->Set(temp_buf);
				}
				break;
		}
	}
	return ok;
}

int PPViewAmountType::MakeListEntry(const PPAmountTypePacket * pPack, AmountTypeViewItem * pItem)
{
	int    ok = -1;
	if(pPack && pItem) {
		pItem->ID           = pPack->Rec.ID;
		pItem->RefAmtTypeID = pPack->Rec.IsComplementary() ? pPack->Rec.RefAmtTypeID : 0;
		pItem->Flags        = pPack->Rec.Flags;
		pItem->Tax          = (pPack->Rec.Flags & PPAmountType::fTax) ? pPack->Rec.Tax : 0;
		pItem->TaxRate      = (pPack->Rec.Flags & PPAmountType::fTax) ? ((double)pPack->Rec.TaxRate) / 100 : 0;
		pPack->Formula.CopyTo(pItem->Formula, sizeof(pItem->Formula));
		ok = 1;
	}
	return ok;
}
//
//
//
class AmtTypeFiltDialog : public TDialog {
	DECL_DIALOG_DATA(AmountTypeFilt);
public:
	AmtTypeFiltDialog() : TDialog(DLG_FLTAMTTYPE) 
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		ushort v = 0;
		ComboBox * p_cb_tax = static_cast<ComboBox *>(getCtrlView(CTLSEL_AMOUNTTYPE_TAX));
		const long flags = Data.Flags;
		AddClusterAssoc(CTL_AMOUNTTYPE_FLAGS, 0, PPAmountType::fErrOnDefault);
		AddClusterAssoc(CTL_AMOUNTTYPE_FLAGS, 1, PPAmountType::fManual);
		AddClusterAssoc(CTL_AMOUNTTYPE_FLAGS, 2, PPAmountType::fStaffAmount);
		AddClusterAssoc(CTL_AMOUNTTYPE_FLAGS, 3, PPAmountType::fFormula);
		AddClusterAssoc(CTL_AMOUNTTYPE_FLAGS, 4, PPAmountType::fDistribCost);
		SetClusterData(CTL_AMOUNTTYPE_FLAGS, flags);
		if(flags & PPAmountType::fTax)
			v = 1;
		else if(flags & PPAmountType::fInAmount)
			v = 2;
		else if(flags & PPAmountType::fOutAmount)
			v = 3;
		else
			v = 0;
		setCtrlData(CTL_AMOUNTTYPE_KIND, &v);
		ushort replace = 0;
		if(flags & PPAmountType::fReplaceCost)
			replace = 1;
		else if(flags & PPAmountType::fReplacePrice)
			replace = 2;
		else if(flags & PPAmountType::fReplaceDiscount)
			replace = 3;
		setCtrlData(CTL_AMOUNTTYPE_REPLACE, &replace);
		if(p_cb_tax) {
			SString word;
			ListWindow * p_lw = CreateListWindow(16, lbtDblClkNotify|lbtFocNotify|lbtDisposeData);
			// @v9.0.2 PPGetWord(PPWORD_VAT, 0, word);
			PPLoadString("vat", word); // @v9.0.2
			p_lw->listBox()->addItem(GTAX_VAT, word);
			// @v9.2.6 PPGetWord(PPWORD_STAX, 0, word);
			PPLoadString("salestax", word); // @v9.2.6
			p_lw->listBox()->addItem(GTAX_SALES, word);
			p_cb_tax->setListWindow(p_lw);
		}
		SetupPPObjCombo(this, CTLSEL_AMOUNTTYPE_REFAMT, PPOBJ_AMOUNTTYPE, 0, 0, 0);
		setCtrlData(CTLSEL_AMOUNTTYPE_TAX, &Data.TaxID);
		if(!(flags & PPAmountType::fTax) && Data.IsComplementary())
			setCtrlData(CTLSEL_AMOUNTTYPE_REFAMT, &Data.RefAmtTypeID);
		disableCtrls(!(flags & PPAmountType::fTax), CTLSEL_AMOUNTTYPE_TAX, CTL_AMOUNTTYPE_TAXRATE, 0);
		disableCtrl(CTLSEL_AMOUNTTYPE_REFAMT, !Data.IsComplementary());
		disableCtrl(CTL_AMOUNTTYPE_REPLACE, (flags & (PPAmountType::fTax | PPAmountType::fManual)));
		SetupCtrls();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1, sel = 0;
		ushort v, replace;
		GetClusterData(CTL_AMOUNTTYPE_FLAGS, &Data.Flags);
		v = getCtrlUInt16(CTL_AMOUNTTYPE_KIND);
		Data.Flags &= ~(PPAmountType::fTax | PPAmountType::fInAmount | PPAmountType::fOutAmount);
		if(v == 1)
			Data.Flags |= PPAmountType::fTax;
		else if(v == 2)
			Data.Flags |= PPAmountType::fInAmount;
		else if(v == 3)
			Data.Flags |= PPAmountType::fOutAmount;
		if(Data.Flags & PPAmountType::fTax)
			getCtrlData(sel = CTLSEL_AMOUNTTYPE_TAX, &Data.TaxID);
		else {
			Data.TaxID = 0;
			if(Data.Flags & (PPAmountType::fInAmount | PPAmountType::fOutAmount))
				getCtrlData(sel = CTLSEL_AMOUNTTYPE_REFAMT, &Data.RefAmtTypeID);
		}
		replace = getCtrlUInt16(CTL_AMOUNTTYPE_REPLACE);
		Data.Flags &= ~PPAmountType::fReplaces;
		if(!(Data.Flags & (PPAmountType::fTax | PPAmountType::fManual))) {
			if(replace == 1)
				Data.Flags |= PPAmountType::fReplaceCost;
			else if(replace == 2)
				Data.Flags |= PPAmountType::fReplacePrice;
			else if(replace == 3)
				Data.Flags |= PPAmountType::fReplaceDiscount;
		}
		ASSIGN_PTR(pData, Data);
		//CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   SetupCtrls();
};

void AmtTypeFiltDialog::SetupCtrls()
{
	ushort v;
	GetClusterData(CTL_AMOUNTTYPE_FLAGS, &Data.Flags);
	//
	// Запрет на признак замещающей суммы для ручных и распределенных сумм
	//
	if(Data.Flags & (PPAmountType::fManual|PPAmountType::fStaffAmount|PPAmountType::fDistribCost)) {
		setCtrlUInt16(CTL_AMOUNTTYPE_REPLACE, 0);
		disableCtrl(CTL_AMOUNTTYPE_REPLACE, 1);
		DisableClusterItem(CTL_AMOUNTTYPE_FLAGS, 3, 1);
		if(Data.Flags & PPAmountType::fFormula) {
			Data.Flags &= ~PPAmountType::fFormula;
			SetClusterData(CTL_AMOUNTTYPE_FLAGS, Data.Flags);
		}
	}
	else {
		disableCtrl(CTL_AMOUNTTYPE_REPLACE, 0);
		DisableClusterItem(CTL_AMOUNTTYPE_FLAGS, 3, 0);
	}
	DisableClusterItem(CTL_AMOUNTTYPE_FLAGS, 1, BIN(Data.Flags & PPAmountType::fFormula));
	disableCtrl(CTL_AMOUNTTYPE_FORMULA, !BIN(Data.Flags & PPAmountType::fFormula));
	//
	//
	//
	v = getCtrlUInt16(CTL_AMOUNTTYPE_KIND);
	disableCtrls(v != 1, CTLSEL_AMOUNTTYPE_TAX, CTL_AMOUNTTYPE_TAXRATE, 0);
	disableCtrl(CTLSEL_AMOUNTTYPE_REFAMT, !oneof2(v, 2, 3));
	//
	// Запрет дистрибутивности для любых видов сумм, кроме обыкновенных
	//
	DisableClusterItem(CTL_AMOUNTTYPE_FLAGS, 4, (v != 0));
	if(v != 0 && Data.Flags & PPAmountType::fDistribCost) {
		Data.Flags &= ~PPAmountType::fDistribCost;
		SetClusterData(CTL_AMOUNTTYPE_FLAGS, Data.Flags);
	}
	//
	if(v == 1) { // fTax
		//
		// Запрет на признак замещающей суммы для ручных сумм
		//
		setCtrlUInt16(CTL_AMOUNTTYPE_REPLACE, 0);
		disableCtrl(CTL_AMOUNTTYPE_REPLACE, 1);
	}
	else {
		//
		// Установка нулевых значений в параметры налоговой суммы
		//
		setCtrlLong(CTLSEL_AMOUNTTYPE_TAX, 0);
		setCtrlReal(CTL_AMOUNTTYPE_TAXRATE, 0.0);
		disableCtrl(CTL_AMOUNTTYPE_REPLACE, 0);
	}
	if(!oneof2(v, 2, 3)) {
		//
		// Установка нулевого значения в комплементарном типе суммы
		//
		setCtrlLong(CTLSEL_AMOUNTTYPE_REFAMT, 0);
	}
	//
	//
	//
	if(getCtrlUInt16(CTL_AMOUNTTYPE_REPLACE)) {
		if(Data.Flags & (PPAmountType::fManual|PPAmountType::fStaffAmount|PPAmountType::fDistribCost)) {
			Data.Flags &= ~(PPAmountType::fManual|PPAmountType::fStaffAmount|PPAmountType::fDistribCost);
			SetClusterData(CTL_AMOUNTTYPE_FLAGS, Data.Flags);
		}
		disableCtrls(1, CTLSEL_AMOUNTTYPE_TAX, CTL_AMOUNTTYPE_TAXRATE, 0);
	}
	enableCommand(cmDistribCost, Data.Flags & PPAmountType::fDistribCost);
}

IMPL_HANDLE_EVENT(AmtTypeFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_AMOUNTTYPE_FLAGS) || event.isClusterClk(CTL_AMOUNTTYPE_KIND) ||
		event.isClusterClk(CTL_AMOUNTTYPE_REPLACE)) {
		SetupCtrls();
	}
	else
		return;
	clearEvent(event);
}

int PPViewAmountType::EditBaseFilt(PPBaseFilt * pFilt)
{
	int    ok = -1;
	AmountTypeFilt filt;
	AmtTypeFiltDialog * p_dlg = new AmtTypeFiltDialog;
	filt.Copy(pFilt, 0);
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(&filt);
	while(ok < 0 && ExecView(p_dlg) == cmOK) {
		if(p_dlg->getDTS(&filt) > 0) {
			if(pFilt)
				pFilt->Copy(&filt, 0);
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int PPViewAmountType::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	THROW(FetchData(0));
	CATCHZOK
	return ok;
}

int PPViewAmountType::InitIteration()
{
	Counter.Init(Data.getCount());
	return 1;
}

int FASTCALL PPViewAmountType::NextIteration(AmountTypeViewItem * pItem)
{
	int    ok = -1;
	if(pItem && Counter < Data.getCount()) {
		ASSIGN_PTR(pItem, *static_cast<const AmountTypeViewItem *>(Data.at(Counter)));
		Counter.Increment();
		ok = 1;
	}
	return ok;
}

void PPViewAmountType::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, SetDefUserProc(PPViewAmountType::GetDataForBrowser, this));
}

SArray * PPViewAmountType::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	ASSIGN_PTR(pBrwId, BROWSER_AMOUNTTYPE);
	return new SArray(Data);
}

int PPViewAmountType::Transmit(int isCharry)
{
	int    ok = -1;
	PPWaitStart();
	if(isCharry) {
		AmountTypeViewItem item;
		PPIDArray id_list;
		for(InitIteration(); NextIteration(&item) > 0;)
			id_list.add(item.ID);
		THROW(SendCharryObject(PPDS_CRRAMOUNTTYPE, id_list));
		ok = 1;
	}
	else {
		ObjTransmitParam param;
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			AmountTypeViewItem item;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			for(InitIteration(); NextIteration(&item) > 0;)
				objid_ary.Add(PPOBJ_AMOUNTTYPE, item.ID);
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
	}
	PPWaitStop();
	CATCHZOKPPERR
	return ok;
}

int PPViewAmountType::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int        ok = (ppvCmd != PPVCMD_ADDITEM) ? PPView::ProcessCommand(ppvCmd, pHdr, pBrw) : -2;
	PPIDArray  id_list;
	PPID       id = (pHdr) ? *static_cast<const PPID *>(pHdr) : 0;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = (ObjAmtT.Edit(&(id = 0), 0) == cmOK) ? 1 : -1;
				break;
			case PPVCMD_TRANSMIT:
			case PPVCMD_TRANSMITCHARRY:
				ok = Transmit(BIN(ppvCmd == PPVCMD_TRANSMITCHARRY));
				break;
		}
	}
	if(ok > 0 && oneof5(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM, PPVCMD_DELETEITEM, PPVCMD_REFRESHBYPERIOD, PPVCMD_REFRESH)) {
		FetchData(id);
		AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
		CALLPTRMEMB(p_def, setArray(new SArray(Data), 0, 1));
		ok = 1;
	}
	return ok;
}

int PPViewAmountType::FetchData(long id)
{
	int    ok = 1;
	AmountTypeViewItem item;
	if(id == 0) {
		Data.freeAll();
		for(PPID id = 0; ObjAmtT.EnumItems(&id, 0) > 0;) {
			PPAmountTypePacket pack;
			if(ObjAmtT.GetPacket(id, &pack) > 0 && CheckForFilt(&pack) > 0) {
				MEMSZERO(item);
				MakeListEntry(&pack, &item);
				THROW_SL(Data.insert(&item));
			}
		}
	}
	else {
		uint pos = 0;
		int found = BIN(Data.lsearch(&id, &pos, CMPF_LONG) > 0);
		PPAmountTypePacket pack;
		if(ObjAmtT.GetPacket(id, &pack) > 0 && CheckForFilt(&pack) > 0) {
			MEMSZERO(item);
			MakeListEntry(&pack, &item);
			if(found)
				*static_cast<AmountTypeViewItem *>(Data.at(pos)) = item;
			else
				THROW_SL(Data.insert(&item));
		}
		else if(found)
			THROW_SL(Data.atFree(pos));
	}
	CATCHZOK
	return ok;
}
