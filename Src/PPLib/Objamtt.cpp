// OBJAMTT.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2010, 2013, 2014, 2015, 2016
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
// @ModuleDef(PPObjAmountType)
//
int SLAPI PPAmountType::IsTax(PPID taxID  /* GTAX_XXX */) const
{
	return BIN(Flags & PPAmountType::fTax && Tax == taxID);
}

int SLAPI PPAmountType::IsComplementary() const
{
	return BIN(Flags & (PPAmountType::fInAmount | PPAmountType::fOutAmount));
}

SLAPI PPAmountTypePacket::PPAmountTypePacket()
{
	MEMSZERO(Rec);
}

int FASTCALL PPAmountTypePacket::IsEqual(const PPAmountTypePacket & rS) const
{
#define CMPF(f) if(Rec.f!=rS.Rec.f) return 0;
	CMPF(Tag);
	CMPF(ID);
	CMPF(EcAlg);
	CMPF(Flags);
	CMPF(Tax);
	CMPF(TaxRate);
	CMPF(RefAmtTypeID);
#undef CMPF
	if(strcmp(Rec.Name, rS.Rec.Name) != 0)
		return 0;
	else if(strcmp(Rec.Symb, rS.Rec.Symb) != 0)
		return 0;
	else if(Formula != rS.Formula)
		return 0;
	return 1;
}

int SLAPI SetupAmtTypeCombo(TDialog * dlg, uint ctl, PPID id, uint flags, long options, PPIDArray * pInclList)
{
	int    ok = 0;
	PPObjAmountType amttobj;
	ComboBox * p_combo = (ComboBox *)dlg->getCtrlView(ctl);
	if(p_combo) {
		StrAssocArray * p_list = amttobj.CreateSelectorList(options, pInclList);
		if(p_list) {
			PPObjListWindow * p_lw = new PPObjListWindow(PPOBJ_AMOUNTTYPE, p_list, flags, 0);
			if(p_lw) {
				p_combo->setListWindow(p_lw, id);
				ok = 1;
			}
		}
	}
	else
		ok = -1;
	return ok;
}

SLAPI PPObjAmountType::PPObjAmountType(void * extraPtr) : PPObjReference(PPOBJ_AMOUNTTYPE, extraPtr)
{
}

int SLAPI PPObjAmountType::SearchSymb(PPID * pID, const char * pSymb)
{
	return ref->SearchSymb(Obj, pID, pSymb, offsetof(PPAmountType, Symb));
}

int SLAPI PPObjAmountType::CheckDupTax(PPID id, PPID tax, long taxRate)
{
	int    ok = 1;
	PPAmountType rec;
	for(PPID i = 0; ok && EnumItems(&i, &rec) > 0;)
		if(i != id && rec.IsTax(tax) && rec.TaxRate == taxRate)
			ok = PPSetError(PPERR_DUPAMTTYPETAX);
	return ok;
}

int SLAPI PPObjAmountType::GetFormulaList(StrAssocArray * pList)
{
	int    ok = -1;
	PPAmountType rec;
	SString formula;
	if(pList)
		pList->Clear();
	for(SEnum en = ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
		if(rec.Flags & PPAmountType::fFormula) {
			GetFormula(rec.ID, formula);
			if(formula.NotEmptyS()) {
				if(pList)
					pList->Add(rec.ID, formula);
				ok = 1;
			}
		}
	}
	return ok;
}

StrAssocArray * SLAPI PPObjAmountType::CreateSelectorList(long options, const PPIDArray * pIncludeList)
{
	PPAmountType rec;
	StrAssocArray * p_list = new StrAssocArray;
	THROW_MEM(p_list);
	for(PPID id = 0; EnumItems(&id, &rec) > 0;) {
		if(!pIncludeList || pIncludeList->lsearch(rec.ID)) {
			if(!(options & selStaffOnly) || rec.Flags & PPAmountType::fStaffAmount)
				THROW_SL(p_list->Add(rec.ID, rec.Name));
		}
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

class AmtTypeDialog : public TDialog {
public:
	AmtTypeDialog(PPObjAmountType * pObj) : TDialog(DLG_AMOUNTTYPE)
	{
		P_Obj = pObj;
	}
	int    setDTS(const PPAmountTypePacket *);
	int    getDTS(PPAmountTypePacket *);
private:
	DECL_HANDLE_EVENT;
	void   SetupCtrls();
	int    EditDistribParam();

	PPObjAmountType * P_Obj;
	PPAmountTypePacket Data;
};

int AmtTypeDialog::EditDistribParam()
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_COSTDISTR);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->AddClusterAssocDef(CTL_COSTDISTR_ALG, 0, ecalgCost);
		dlg->AddClusterAssoc(CTL_COSTDISTR_ALG, 1, ecalgPrice);
		dlg->AddClusterAssoc(CTL_COSTDISTR_ALG, 2, ecalgQtty);
		dlg->AddClusterAssoc(CTL_COSTDISTR_ALG, 3, ecalgPhQtty);
		dlg->AddClusterAssoc(CTL_COSTDISTR_ALG, 4, ecalgBrutto);
		dlg->AddClusterAssoc(CTL_COSTDISTR_ALG, 5, ecalgVolume);
		dlg->SetClusterData(CTL_COSTDISTR_ALG, Data.Rec.EcAlg);
		dlg->AddClusterAssoc(CTL_COSTDISTR_FLAGS, 0, PPAmountType::fDcNeg);
		dlg->SetClusterData(CTL_COSTDISTR_FLAGS, Data.Rec.Flags);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			dlg->GetClusterData(CTL_COSTDISTR_ALG, &Data.Rec.EcAlg);
			dlg->GetClusterData(CTL_COSTDISTR_FLAGS, &Data.Rec.Flags);
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}

void AmtTypeDialog::SetupCtrls()
{
	ushort v;
	GetClusterData(CTL_AMOUNTTYPE_FLAGS, &Data.Rec.Flags);
	//
	// Запрет на признак замещающей суммы для ручных и распределенных сумм
	//
	if(Data.Rec.Flags & (PPAmountType::fManual|PPAmountType::fStaffAmount|PPAmountType::fDistribCost)) {
		setCtrlUInt16(CTL_AMOUNTTYPE_REPLACE, 0);
		disableCtrl(CTL_AMOUNTTYPE_REPLACE, 1);
		DisableClusterItem(CTL_AMOUNTTYPE_FLAGS, 3, 1);
		if(Data.Rec.Flags & PPAmountType::fFormula) {
			Data.Rec.Flags &= ~PPAmountType::fFormula;
			SetClusterData(CTL_AMOUNTTYPE_FLAGS, Data.Rec.Flags);
		}
	}
	else {
		disableCtrl(CTL_AMOUNTTYPE_REPLACE, 0);
		DisableClusterItem(CTL_AMOUNTTYPE_FLAGS, 3, 0);
	}
	DisableClusterItem(CTL_AMOUNTTYPE_FLAGS, 1, BIN(Data.Rec.Flags & PPAmountType::fFormula));
	disableCtrl(CTL_AMOUNTTYPE_FORMULA, !BIN(Data.Rec.Flags & PPAmountType::fFormula));
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
	if(v != 0 && Data.Rec.Flags & PPAmountType::fDistribCost) {
		Data.Rec.Flags &= ~PPAmountType::fDistribCost;
		SetClusterData(CTL_AMOUNTTYPE_FLAGS, Data.Rec.Flags);
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
		if(Data.Rec.Flags & (PPAmountType::fManual|PPAmountType::fStaffAmount|PPAmountType::fDistribCost)) {
			Data.Rec.Flags &= ~(PPAmountType::fManual|PPAmountType::fStaffAmount|PPAmountType::fDistribCost);
			SetClusterData(CTL_AMOUNTTYPE_FLAGS, Data.Rec.Flags);
		}
		disableCtrls(1, CTLSEL_AMOUNTTYPE_TAX, CTL_AMOUNTTYPE_TAXRATE, 0);
	}
	enableCommand(cmDistribCost, Data.Rec.Flags & PPAmountType::fDistribCost);
}

IMPL_HANDLE_EVENT(AmtTypeDialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_AMOUNTTYPE_FLAGS) || event.isClusterClk(CTL_AMOUNTTYPE_KIND) ||
		event.isClusterClk(CTL_AMOUNTTYPE_REPLACE)) {
		SetupCtrls();
	}
	else if(event.isCmd(cmDistribCost))
		EditDistribParam();
	else
		return;
	clearEvent(event);
}

int AmtTypeDialog::setDTS(const PPAmountTypePacket * pData)
{
	Data = *pData;

	ushort v = 0;
	ComboBox * p_cb_tax = (ComboBox*)getCtrlView(CTLSEL_AMOUNTTYPE_TAX);
	const long flags = Data.Rec.Flags;
	setCtrlData(CTL_AMOUNTTYPE_NAME, Data.Rec.Name);
	setCtrlData(CTL_AMOUNTTYPE_SYMB, Data.Rec.Symb);
	setCtrlData(CTL_AMOUNTTYPE_ID,   &Data.Rec.ID);
	disableCtrl(CTL_AMOUNTTYPE_ID, (!PPMaster || Data.Rec.ID));
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
	setCtrlData(CTLSEL_AMOUNTTYPE_TAX, &Data.Rec.Tax);
	setCtrlString(CTL_AMOUNTTYPE_FORMULA, Data.Formula);
	disableCtrl(CTL_AMOUNTTYPE_FORMULA, !BIN(flags & PPAmountType::fFormula));
	if(flags & PPAmountType::fTax)
		setCtrlReal(CTL_AMOUNTTYPE_TAXRATE, fdiv100i(Data.Rec.TaxRate)); // @divtax
	else if(Data.Rec.IsComplementary())
		setCtrlData(CTLSEL_AMOUNTTYPE_REFAMT, &Data.Rec.RefAmtTypeID);
	disableCtrls(!(flags & PPAmountType::fTax), CTLSEL_AMOUNTTYPE_TAX, CTL_AMOUNTTYPE_TAXRATE, 0);
	disableCtrl(CTLSEL_AMOUNTTYPE_REFAMT, !Data.Rec.IsComplementary());
	disableCtrl(CTL_AMOUNTTYPE_REPLACE, (flags & (PPAmountType::fTax | PPAmountType::fManual)) ||
		(Data.Rec.ID > 0 && Data.Rec.ID < 1000));  // Reserved amount types can't replace another
	SetupCtrls();
	return 1;
}

int AmtTypeDialog::getDTS(PPAmountTypePacket * pData)
{
	int    ok = 1, sel = 0;
	ushort v, replace;
	getCtrlData(sel = CTL_AMOUNTTYPE_NAME, Data.Rec.Name);
	THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
	getCtrlData(CTL_AMOUNTTYPE_ID, &Data.Rec.ID);
	THROW(P_Obj->CheckDupName(Data.Rec.ID, Data.Rec.Name));
	getCtrlData(sel = CTL_AMOUNTTYPE_SYMB, Data.Rec.Symb);
	THROW(P_Obj->CheckDupSymb(Data.Rec.ID, strip(Data.Rec.Symb)));
	GetClusterData(CTL_AMOUNTTYPE_FLAGS, &Data.Rec.Flags);
	v = getCtrlUInt16(CTL_AMOUNTTYPE_KIND);
	Data.Rec.Flags &= ~(PPAmountType::fTax | PPAmountType::fInAmount | PPAmountType::fOutAmount);
	if(v == 1)
		Data.Rec.Flags |= PPAmountType::fTax;
	else if(v == 2)
		Data.Rec.Flags |= PPAmountType::fInAmount;
	else if(v == 3)
		Data.Rec.Flags |= PPAmountType::fOutAmount;
	if(Data.Rec.Flags & PPAmountType::fTax) {
		getCtrlData(sel = CTLSEL_AMOUNTTYPE_TAX, &Data.Rec.Tax);
		double tax_rate = getCtrlReal(CTL_AMOUNTTYPE_TAXRATE);
		Data.Rec.TaxRate = R0i(tax_rate * 100.0);
		if(!PPMaster)
			THROW(P_Obj->CheckDupTax(Data.Rec.ID, Data.Rec.Tax, Data.Rec.TaxRate));
	}
	else {
		Data.Rec.Tax = 0;
		Data.Rec.TaxRate = 0L;
		if(Data.Rec.Flags & (PPAmountType::fInAmount | PPAmountType::fOutAmount)) {
			getCtrlData(sel = CTLSEL_AMOUNTTYPE_REFAMT, &Data.Rec.RefAmtTypeID);
			THROW_PP(Data.Rec.RefAmtTypeID, PPERR_REFAMTTYPENEEDED);
		}
	}
	replace = getCtrlUInt16(CTL_AMOUNTTYPE_REPLACE);
	Data.Rec.Flags &= ~PPAmountType::fReplaces;
	if(!(Data.Rec.Flags & (PPAmountType::fTax | PPAmountType::fManual))) {
		if(replace == 1)
			Data.Rec.Flags |= PPAmountType::fReplaceCost;
		else if(replace == 2)
			Data.Rec.Flags |= PPAmountType::fReplacePrice;
		else if(replace == 3)
			Data.Rec.Flags |= PPAmountType::fReplaceDiscount;
	}
	if(Data.Rec.Flags & PPAmountType::fFormula)
		getCtrlString(CTL_AMOUNTTYPE_FORMULA, Data.Formula);
	else
		Data.Formula = 0;
	*pData = Data;
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

int SLAPI PPObjAmountType::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	int    r = cmCancel, valid_data = 0, is_new = 0;
	PPAmountTypePacket pack;
	AmtTypeDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new AmtTypeDialog(this))));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	dlg->setDTS(&pack);
	while(!valid_data && ExecView(dlg) == cmOK) {
		THROW(is_new || CheckRights(PPR_MOD));
		if(dlg->getDTS(&pack)) {
			if(*pID)
				*pID = pack.Rec.ID;
			if(PutPacket(pID, &pack, 1)) {
				*pID = pack.Rec.ID;
				ok = cmOK;
				valid_data = 1;
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPObjAmountType::GetFormula(PPID id, SString & rBuf)
{
	return ref->GetPropVlrString(Obj, id, AMTTPRP_FORMULA, rBuf);
}

int SLAPI PPObjAmountType::GetPacket(PPID id, PPAmountTypePacket * pPack)
{
	int    ok = -1;
	PPAmountTypePacket pack;
	int    r = Search(id, &pack.Rec);
	if(r > 0) {
		if(pack.Rec.Flags & PPAmountType::fFormula)
			GetFormula(id, pack.Formula);
		ASSIGN_PTR(pPack, pack);
		ok = 1;
	}
	else
		ok = r;
	return ok;
}

int SLAPI PPObjAmountType::PutPacket(PPID * pID, PPAmountTypePacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				PPAmountTypePacket org_pack;
				THROW(GetPacket(*pID, &org_pack) > 0);
				if(pPack->IsEqual(org_pack)) {
					ok = -1;
				}
				else {
					THROW(CheckRights(PPR_MOD));
					THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 0, 0));
					if(pPack->Rec.Flags & PPAmountType::fFormula) {
						THROW(ref->PutPropVlrString(Obj, *pID, AMTTPRP_FORMULA, pPack->Formula));
					}
					DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
					// @v8.5.5 @fix ok = -1;
				}
			}
			else {
				THROW(CheckRights(PPR_DEL));
				THROW(ref->RemoveItem(Obj, *pID, 0));
				THROW(ref->PutPropVlrString(Obj, *pID, AMTTPRP_FORMULA, 0));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
			}
			if(ok > 0) // @v8.5.5 @fix (ok < 0)-->(ok > 0)
				THROW(Dirty(*pID));
		}
		else if(pPack) {
			THROW(CheckRights(PPR_INS));
			*pID = pPack->Rec.ID;
			THROW(ref->AddItem(Obj, pID, &pPack->Rec, 0));
			if(pPack->Rec.Flags & PPAmountType::fFormula)
				THROW(ref->PutPropVlrString(Obj, *pID, AMTTPRP_FORMULA, pPack->Formula));
			pPack->Rec.ID = *pID;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjAmountType::ProcessReservedItem(TVRez & rez)
{
	int    ok = 1, r;
	SString name;
	SString symb;
	PPID   id = (PPID)rez.getUINT();
	rez.getString(name, 2);
	PPExpandString(name, CTRANSF_UTF8_TO_INNER); // @v9.4.4
	rez.getString(symb, 2);
	uint   errOnDefault = rez.getUINT();
	THROW(r = Search(id));
	if(r < 0) {
		PPAmountType rec;
		MEMSZERO(rec);
		rec.ID = id;
		STRNSCPY(rec.Name, name);
		STRNSCPY(rec.Symb, symb);
		if(errOnDefault)
			rec.Flags |= PPAmountType::fErrOnDefault;
		THROW(EditItem(PPOBJ_AMOUNTTYPE, 0, &rec, 1));
	}
	CATCHZOK
	return ok;
}

int  SLAPI PPObjAmountType::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPAmountTypePacket * p_pack = (PPAmountTypePacket *)p->Data;
		if(p_pack->Rec.Flags & (PPAmountType::fInAmount | PPAmountType::fOutAmount))
			THROW(ProcessObjRefInArray(PPOBJ_AMOUNTTYPE, &p_pack->Rec.RefAmtTypeID, ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjAmountType::SerializePacket(int dir, PPAmountTypePacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString line_buf;
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->Formula, rBuf));
	CATCHZOK
	return ok;
}

int  SLAPI PPObjAmountType::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new PPAmountTypePacket);
	if(stream == 0) {
		THROW(GetPacket(id, (PPAmountTypePacket *)p->Data) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile((FILE*)stream, 0))
		THROW(SerializePacket(-1, (PPAmountTypePacket *)p->Data, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int  SLAPI PPObjAmountType::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1, r;
	if(p && p->Data) {
		PPAmountTypePacket * p_pack = (PPAmountTypePacket *)p->Data;
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				PPAmountType same_rec;
				if(p_pack->Rec.ID < PP_FIRSTUSRREF) {
					if(Search(p_pack->Rec.ID, &same_rec) > 0) {
						*pID = same_id = p_pack->Rec.ID;
						ok = 1;
					}
				}
				else if(p_pack->Rec.Symb[0] && SearchBySymb(p_pack->Rec.Symb, &same_id, &same_rec) > 0) {
					*pID = same_id;
					ok = 1;
				}
				else if(p_pack->Rec.Name[0] && SearchByName(p_pack->Rec.Name, &same_id, &same_rec) > 0) {
					*pID = same_id;
					ok = 1;
				}
				else {
					same_id = p_pack->Rec.ID = 0;
				}
				if(same_id == 0) {
					p_pack->Rec.ID = 0;
					r = PutPacket(pID, p_pack, 1);
					if(r == 0) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTAMTTYPE, p_pack->Rec.ID, p_pack->Rec.Name);
						ok = -1;
					}
					else if(r > 0)
						ok = 1; // 101; // @ObjectCreated
					else
						ok = 1;
				}
			}
			else {
				p_pack->Rec.ID = *pID;
				r = PutPacket(pID, p_pack, 1);
				if(r == 0) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTAMTTYPE, p_pack->Rec.ID, p_pack->Rec.Name);
					ok = -1;
				}
				else if(r > 0)
					ok = 1; // 102; // @ObjectUpdated
				else
					ok = 1;
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile((FILE*)stream, 0, 0))
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjAmountType, PPAmountTypePacket);
//
//
//
class AmountTypeCache : public ObjCache {
public:
	SLAPI  AmountTypeCache();
	virtual int SLAPI Dirty(PPID id); // @sync_w
	int    SLAPI FetchByTax(PPID * pID, PPID tax, double taxRate); // @sync_r
	int    SLAPI FetchCompl(PPID srcAmtID, PPID * pInAmtID, PPID * pOutAmtID); // @sync_r
	int    SLAPI FetchTaxIDs(TaxAmountIDs * pBlk);
	int    SLAPI IsThereDistribCost()
	{
		int    yes = 0;
		RwL.ReadLock();
		yes = IsThereDistribCostAmounts;
		RwL.Unlock();
		return yes;
	}
private:
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
	int    SLAPI InitTaxBlock();
	TaxAmountIDs TaxBlock;
	int    IsThereDistribCostAmounts;
	ReadWriteLock LocRwL;
public:
	struct AmountTypeData : public ObjCacheEntry {
		long   Tax;
		union {
			long   TaxRate;
			PPID   RefAmtTypeID;
		};
		long   Flags;
		int16  EcAlg;
		int16  Reserve; // @alignment
	};
};

SLAPI AmountTypeCache::AmountTypeCache() : ObjCache(PPOBJ_AMOUNTTYPE, sizeof(AmountTypeCache::AmountTypeData))
{
	PPObjAmountType amt_obj;
	PPAmountType temp_rec;
	LocRwL.WriteLock();
	IsThereDistribCostAmounts = 0;
	for(PPID i = 0; amt_obj.EnumItems(&i, &temp_rec) > 0;)
		Get(i, &temp_rec);
	InitTaxBlock();
	LocRwL.Unlock();
}

static void SLAPI SwapVat(TaxAmountIDs * pData, uint i1, uint i2)
{
	if(pData->VatAmtID[i1] && pData->VatAmtID[i2] && pData->VatRate[i1] > pData->VatRate[i2]) {
		Exchange(&pData->VatAmtID[i1], &pData->VatAmtID[i2]);
		Exchange(&pData->VatRate[i1], &pData->VatRate[i2]);
	}
}

int SLAPI AmountTypeCache::InitTaxBlock()
{
	MEMSZERO(TaxBlock);
	IsThereDistribCostAmounts = 0;
	for(uint j = 0; j < P_Ary->getCount(); j++) {
		const AmountTypeData * p_entry = (AmountTypeData *)P_Ary->at(j);
		if(p_entry->Flags & PPAmountType::fDistribCost)
			IsThereDistribCostAmounts = 1;
		if(p_entry->Flags & PPAmountType::fTax) {
			if(p_entry->Tax == GTAX_SALES) {
				TaxBlock.STaxAmtID = p_entry->ID;
				TaxBlock.STaxRate  = p_entry->TaxRate;
			}
			else if(p_entry->Tax == GTAX_VAT) {
				if(TaxBlock.VatAmtID[0] == 0) {
					TaxBlock.VatAmtID[0] = p_entry->ID;
					TaxBlock.VatRate[0]  = p_entry->TaxRate;
				}
				else if(TaxBlock.VatAmtID[1] == 0) {
					TaxBlock.VatAmtID[1] = p_entry->ID;
					TaxBlock.VatRate[1]  = p_entry->TaxRate;
				}
				else if(TaxBlock.VatAmtID[2] == 0) {
					TaxBlock.VatAmtID[2] = p_entry->ID;
					TaxBlock.VatRate[2]  = p_entry->TaxRate;
				}
			}
		}
	}
	SwapVat(&TaxBlock, 0, 1);
	SwapVat(&TaxBlock, 1, 2);
	SwapVat(&TaxBlock, 0, 1);
	return 1;
}

int SLAPI AmountTypeCache::Dirty(PPID id)
{
	int    ok = 1;
	PPAmountType temp_rec;
	{
		RwL.WriteLock();
		ok = Helper_Dirty(id);
		RwL.Unlock();
	}
	//
	// Функция Get вызывает блокировку чтения. По этому, мы вынуждены разорвать
	// блокировку записи на две пары {RwL.WriteLock() - RwL.Unlock()}
	//
	Get(id, &temp_rec);
	{
		RwL.WriteLock();
		InitTaxBlock();
		RwL.Unlock();
	}
	return ok;
}

int SLAPI AmountTypeCache::FetchTaxIDs(TaxAmountIDs * pBlk)
{
	RwL.ReadLock();
	ASSIGN_PTR(pBlk, TaxBlock);
	RwL.Unlock();
	return 1;
}

int SLAPI AmountTypeCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	AmountTypeData * p_cache_rec = (AmountTypeData *)pEntry;
	PPAmountTypePacket pack;
	PPObjAmountType at_obj;
	if((ok = at_obj.GetPacket(id, &pack)) > 0) {
		#define FLD(f) p_cache_rec->f = pack.Rec.f
	   	FLD(Tax);
		FLD(TaxRate);
		FLD(Flags);
		FLD(EcAlg);
		#undef FLD

		StringSet ss("/&");
		ss.add(pack.Rec.Name);
		ss.add(pack.Rec.Symb);
		ss.add(pack.Formula);
		ok = PutName(ss.getBuf(), p_cache_rec);
	}
	return ok;
}

void SLAPI AmountTypeCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPAmountType * p_data_rec = (PPAmountType*)pDataRec;
	const AmountTypeData * p_cache_rec = (const AmountTypeData *)pEntry;
	memzero(p_data_rec, sizeof(PPAmountType));
	p_data_rec->Tag = PPOBJ_AMOUNTTYPE;
	#define FLD(f) p_data_rec->f = p_cache_rec->f
	FLD(ID);
	FLD(Tax);
	FLD(TaxRate);
	FLD(Flags);
	FLD(EcAlg);
	#undef FLD

	char   temp_buf[1024];
	GetName(pEntry, temp_buf, sizeof(temp_buf));
	StringSet ss("/&");
	ss.setBuf(temp_buf, strlen(temp_buf)+1);
	uint   p = 0;
	ss.get(&p, p_data_rec->Name, sizeof(p_data_rec->Name));
	ss.get(&p, p_data_rec->Symb, sizeof(p_data_rec->Symb));
}

int SLAPI AmountTypeCache::FetchByTax(PPID * pID, PPID tax, double taxRate)
{
	int    ok = -1;
	RwL.ReadLock();
	const uint c = GetCount();
	for(uint i = 0; ok < 0 && i < c; i++) {
		const AmountTypeData * p_entry = (const AmountTypeData *)SearchByPos(i, 0);
		if(p_entry->Tax == tax && p_entry->TaxRate == R0i(taxRate * 100L)) {
			ASSIGN_PTR(pID, p_entry->ID);
			ok = 1;
		}
	}
	RwL.Unlock();
	return ok;
}

int SLAPI AmountTypeCache::FetchCompl(PPID srcAmtID, PPID * pInAmtID, PPID * pOutAmtID)
{
	PPID   in_id = 0, out_id = 0;
	RwL.ReadLock();
	const uint c = GetCount();
	for(uint i = 0; i < c; i++) {
		const AmountTypeData * p_entry = (const AmountTypeData *)SearchByPos(i, 0);
		if(p_entry->RefAmtTypeID == srcAmtID)
			if(p_entry->Flags & PPAmountType::fInAmount && !in_id)
				in_id = p_entry->ID;
			else if(p_entry->Flags & PPAmountType::fOutAmount && !out_id)
				out_id = p_entry->ID;
	}
	RwL.Unlock();
	ASSIGN_PTR(pInAmtID, in_id);
	ASSIGN_PTR(pOutAmtID, out_id);
	return (in_id || out_id) ? 1 : -1;
}

IMPL_OBJ_FETCH(PPObjAmountType, PPAmountType, AmountTypeCache);
IMPL_OBJ_DIRTY(PPObjAmountType, AmountTypeCache);

int SLAPI PPObjAmountType::FetchByTax(PPID * pID, PPID tax, double taxRate)
{
	AmountTypeCache * p_cache = GetDbLocalCachePtr <AmountTypeCache> (Obj);
	return p_cache ? p_cache->FetchByTax(pID, tax, taxRate) : -1;
}

int SLAPI PPObjAmountType::FetchCompl(PPID srcAmtID, PPID * pInAmtID, PPID * pOutAmtID)
{
	AmountTypeCache * p_cache = GetDbLocalCachePtr <AmountTypeCache> (Obj);
	return p_cache ? p_cache->FetchCompl(srcAmtID, pInAmtID, pOutAmtID) : -1;
}

int SLAPI PPObjAmountType::IsThereDistribCostAmounts()
{
	AmountTypeCache * p_cache = GetDbLocalCachePtr <AmountTypeCache> (Obj);
	return p_cache ? p_cache->IsThereDistribCost() : 0;
}

int SLAPI PPObjAmountType::GetTaxAmountIDs(TaxAmountIDs * pData, int useCache)
{
	AmountTypeCache * p_cache = 0;
	if(useCache && (p_cache = GetDbLocalCachePtr <AmountTypeCache> (Obj, 0)) != 0) {
		PROFILE(p_cache->FetchTaxIDs(pData));
	}
	else {
		PROFILE_START
		PPAmountType amtt_rec;
		memzero(pData, sizeof(TaxAmountIDs));
		for(PPID amt_type_id = 0; EnumItems(&amt_type_id, &amtt_rec) > 0;)
			if(amtt_rec.IsTax(GTAX_SALES)) {
				pData->STaxAmtID = amt_type_id;
				pData->STaxRate  = amtt_rec.TaxRate;
			}
			else if(amtt_rec.IsTax(GTAX_VAT))
				for(uint i = 0; i < 3; i++)
					if(pData->VatAmtID[i] == 0) {
						pData->VatAmtID[i] = amt_type_id;
						pData->VatRate[i]  = amtt_rec.TaxRate;
						break;
					}
		SwapVat(pData, 0, 1);
		SwapVat(pData, 1, 2);
		SwapVat(pData, 0, 1);
		PROFILE_END
	}
	return 1;
}
//
//
//
int SLAPI PPObjAmountType::Browse(void * extraPtr)
{
	class AmountTypeView : public ObjViewDialog {
	public:
		AmountTypeView(PPObjAmountType * pObj, void * extraPtr) : ObjViewDialog(DLG_AMTTVIEW, pObj, extraPtr)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			ObjViewDialog::handleEvent(event);
			if(event.isCmd(cmTransmitCharry)) {
				PPIDArray id_list;
				for(PPID id = 0; ((PPObjReference *)P_Obj)->EnumItems(&id, 0) > 0;)
					id_list.add(id);
				if(!SendCharryObject(PPDS_CRRAMOUNTTYPE, id_list))
					PPError();
				clearEvent(event);
			}
		}
	};
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		TDialog * dlg = new AmountTypeView(this, extraPtr);
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}

