// GDSCLS.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPObjGoodsClass)
//
#define GDSCLS_EXTSTR_FLD_NAMECONV    1
#define GDSCLS_EXTSTR_FLD_ABBRCONV    2
#define GDSCLS_EXTSTR_FLD_PROPKIND    3
#define GDSCLS_EXTSTR_FLD_PROPGRADE   4
#define GDSCLS_EXTSTR_FLD_PROPADD     5
#define GDSCLS_EXTSTR_FLD_DIM_NAME    6
#define GDSCLS_EXTSTR_FLD_DIM_SCALE   7
#define GDSCLS_EXTSTR_FLD_DIM_VALS    8
#define GDSCLS_EXTSTR_FLD_PHPERUFORM  9
#define GDSCLS_EXTSTR_FLD_TAXFACTOR  10
#define GDSCLS_EXTSTR_FLD_PACKAGE    11
#define GDSCLS_EXTSTR_FLD_PROPADD2   12
#define GDSCLS_EXTSTR_FLD_LOTDIMQTTY 13

#define GDSCLS_EXTSTR_FROM_NAME      21
#define GDSCLS_EXTSTR_FROM_FORM      22
#define GDSCLS_EXTSTR_FROM_PAR1      23
#define GDSCLS_EXTSTR_FROM_PAR2      24
#define GDSCLS_EXTSTR_FROM_PAR3      25
#define GDSCLS_EXTSTR_FROM_FLAGS     40
//
//
//
PPObjGoodsClass::PPObjGoodsClass(void * extraPtr) : PPObjReference(PPOBJ_GOODSCLASS, extraPtr)
{
}

IMPL_DESTROY_OBJ_PACK(PPObjGoodsClass, PPGdsClsPacket);

/*static*/int PPObjGoodsClass::SerializeDim(int dir, PPGdsClsDim * p, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, MKSTYPE(S_ZSTRING, sizeof(p->Name)), p->Name, 0, rBuf));
	THROW_SL(pSCtx->Serialize(dir, MKSTYPE(S_INT, sizeof(p->Scale)), &p->Scale, 0, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &p->ValList, rBuf));
	CATCHZOK
	return ok;
}

/*static*/int PPObjGoodsClass::PutProp(SString & rBuf, uint fldID, PPGdsClsProp * pProp)
{
	int    ok = 1;
	char   temp_str[128];
	PPPutExtStrData(10*fldID+1, rBuf, pProp->Name);
	PPPutExtStrData(10*fldID+2, rBuf, ltoa(pProp->ItemsListID, temp_str, 10));
	return ok;
}

/*static*/int PPObjGoodsClass::GetProp(const SString & rBuf, uint fldID, PPGdsClsProp * pProp)
{
	SString temp_buf;
	PPGetExtStrData(10*fldID+1, rBuf, temp_buf);
	temp_buf.CopyTo(pProp->Name, sizeof(pProp->Name));
	PPGetExtStrData(10*fldID+2, rBuf, temp_buf);
	pProp->ItemsListID = temp_buf.ToLong();
	return 1;
}

/*static*/int PPObjGoodsClass::PutPropsToLine(SString & rLine, PPGdsClsPacket * pPack)
{
	int    ok = 1;
	PPPutExtStrData(GDSCLS_EXTSTR_FLD_NAMECONV,   rLine, pPack->NameConv);
	PPPutExtStrData(GDSCLS_EXTSTR_FLD_ABBRCONV,   rLine, pPack->AbbrConv);
	PPPutExtStrData(GDSCLS_EXTSTR_FLD_PHPERUFORM, rLine, pPack->PhUPerU_Formula);
	PPPutExtStrData(GDSCLS_EXTSTR_FLD_TAXFACTOR,  rLine, pPack->TaxMult_Formula);
	PPPutExtStrData(GDSCLS_EXTSTR_FLD_PACKAGE,    rLine, pPack->Package_Formula);
	PPPutExtStrData(GDSCLS_EXTSTR_FLD_LOTDIMQTTY, rLine, pPack->LotDimQtty_Formula);

	THROW(PutProp(rLine, GDSCLS_EXTSTR_FLD_PROPKIND,  &pPack->PropKind));
	THROW(PutProp(rLine, GDSCLS_EXTSTR_FLD_PROPGRADE, &pPack->PropGrade));
	THROW(PutProp(rLine, GDSCLS_EXTSTR_FLD_PROPADD,   &pPack->PropAdd));
	THROW(PutProp(rLine, GDSCLS_EXTSTR_FLD_PROPADD2,  &pPack->PropAdd2));
	CATCHZOK
	return ok;
}

/*static*/int PPObjGoodsClass::GetPropsFromLine(const SString & rLine, PPGdsClsPacket * pPack)
{
	int    ok = 1;
	PPGetExtStrData(GDSCLS_EXTSTR_FLD_NAMECONV,   rLine, pPack->NameConv);
	PPGetExtStrData(GDSCLS_EXTSTR_FLD_ABBRCONV,   rLine, pPack->AbbrConv);
	PPGetExtStrData(GDSCLS_EXTSTR_FLD_PHPERUFORM, rLine, pPack->PhUPerU_Formula);
	PPGetExtStrData(GDSCLS_EXTSTR_FLD_TAXFACTOR,  rLine, pPack->TaxMult_Formula);
	PPGetExtStrData(GDSCLS_EXTSTR_FLD_PACKAGE,    rLine, pPack->Package_Formula);
	PPGetExtStrData(GDSCLS_EXTSTR_FLD_LOTDIMQTTY, rLine, pPack->LotDimQtty_Formula);
	GetProp(rLine, GDSCLS_EXTSTR_FLD_PROPKIND,  &pPack->PropKind);
	GetProp(rLine, GDSCLS_EXTSTR_FLD_PROPGRADE, &pPack->PropGrade);
	GetProp(rLine, GDSCLS_EXTSTR_FLD_PROPADD,   &pPack->PropAdd);
	GetProp(rLine, GDSCLS_EXTSTR_FLD_PROPADD2,  &pPack->PropAdd2);
	return ok;
}

int PPObjGoodsClass::SerializePacket(int dir, PPGdsClsPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString line_buf;
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	if(dir > 0) {
		THROW(PutPropsToLine(line_buf, pPack));
		THROW_SL(rBuf.Write(line_buf));
	}
	else if(dir < 0) {
		THROW_SL(rBuf.Read(line_buf));
		THROW(GetPropsFromLine(line_buf, pPack));
	}
	THROW(SerializeDim(dir, &pPack->DimX, rBuf, pSCtx));
	THROW(SerializeDim(dir, &pPack->DimY, rBuf, pSCtx));
	THROW(SerializeDim(dir, &pPack->DimZ, rBuf, pSCtx));
	THROW(SerializeDim(dir, &pPack->DimW, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int PPObjGoodsClass::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjGoodsClass, PPGdsClsPacket>(this, p, id, stream, pCtx); }

int PPObjGoodsClass::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data) {
		PPGdsClsPacket * p_pack = static_cast<PPGdsClsPacket *>(p->Data);
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				if(P_Ref->SearchSymb(Obj, &same_id, p_pack->Rec.Name, offsetof(PPGdsCls, Name)) > 0) {
					PPGdsCls same_rec;
					if(Search(same_id, &same_rec) > 0) {
						ASSIGN_PTR(pID, same_id);
					}
					else
						same_id = 0;
				}
				if(same_id == 0) {
					p_pack->Rec.ID = 0;
					THROW(PutPacket(pID, p_pack, 1));
				}
			}
			else {
				p_pack->Rec.ID = *pID;
				THROW(PutPacket(pID, p_pack, 1));
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjGoodsClass::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPGdsClsPacket * p_pack = static_cast<PPGdsClsPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_UNIT, &p_pack->Rec.DefUnitID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_UNIT, &p_pack->Rec.DefPhUnitID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODSTAX, &p_pack->Rec.DefTaxGrpID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODSTYPE, &p_pack->Rec.DefGoodsTypeID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODSGROUP, &p_pack->Rec.DefGrpID, ary, replace));
		if(IS_DYN_OBJTYPE(p_pack->PropKind.ItemsListID))
			THROW(ProcessObjRefInArray(PPOBJ_DYNAMICOBJS, &p_pack->PropKind.ItemsListID, ary, replace));
		if(IS_DYN_OBJTYPE(p_pack->PropGrade.ItemsListID))
			THROW(ProcessObjRefInArray(PPOBJ_DYNAMICOBJS, &p_pack->PropGrade.ItemsListID, ary, replace));
		if(IS_DYN_OBJTYPE(p_pack->PropAdd.ItemsListID))
			THROW(ProcessObjRefInArray(PPOBJ_DYNAMICOBJS, &p_pack->PropAdd.ItemsListID, ary, replace));
		if(IS_DYN_OBJTYPE(p_pack->PropAdd2.ItemsListID))
			THROW(ProcessObjRefInArray(PPOBJ_DYNAMICOBJS, &p_pack->PropAdd2.ItemsListID, ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjGoodsClass::GetPacket(PPID id, PPGdsClsPacket * pPack)
{
	int    ok = -1;
	if(Search(id, &pPack->Rec) > 0) {
		SString line_buf, form_buf;
		if(P_Ref->GetPropVlrString(Obj, id, GDCPRP_PROPS, line_buf) > 0) {
			GetPropsFromLine(line_buf, pPack);
		}
		GetDim(id, GDCPRP_DIMX, &pPack->DimX);
		GetDim(id, GDCPRP_DIMY, &pPack->DimY);
		GetDim(id, GDCPRP_DIMZ, &pPack->DimZ);
		GetDim(id, GDCPRP_DIMW, &pPack->DimW);
		{
			pPack->FormulaList.freeAll();
			if(P_Ref->GetPropVlrString(Obj, id, GDCPRP_FORMLIST, line_buf) > 0) {
				PPStringSetSCD form_list;
				form_list.setBuf(line_buf, line_buf.Len()+1);
				for(uint pos = 0; form_list.get(&pos, form_buf);)
					pPack->FormulaList.insert(newStr(form_buf));
			}
		}
		ok = 1;
	}
	return ok;
}

int PPObjGoodsClass::PutPacket(PPID * pID, PPGdsClsPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				THROW(P_Ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
			}
			else {
				THROW(P_Ref->RemoveItem(Obj, *pID, 0));
			}
			THROW(Dirty(*pID));
		}
		else {
			*pID = pPack->Rec.ID;
			THROW(P_Ref->AddItem(Obj, pID, &pPack->Rec, 0));
		}
		{
			const PPID id = *pID;
			SString line_buf, form_buf;
			if(pPack) {
				THROW(PutPropsToLine(line_buf, pPack));
				THROW(P_Ref->PutPropVlrString(Obj, id, GDCPRP_PROPS, line_buf));
				THROW(PutDim(id, GDCPRP_DIMX, &pPack->DimX));
				THROW(PutDim(id, GDCPRP_DIMY, &pPack->DimY));
				THROW(PutDim(id, GDCPRP_DIMZ, &pPack->DimZ));
				THROW(PutDim(id, GDCPRP_DIMW, &pPack->DimW));
				{
					PPStringSetSCD form_list;
					for(uint i = 1; i <= pPack->FormulaList.getCount(); i++) {
						PPGdsClsFormula form;
						pPack->GetFormula(i, &form);
						form.PutToBuffer(form_buf);
						form_list.add(form_buf);
					}
					THROW(P_Ref->PutPropVlrString(Obj, id, GDCPRP_FORMLIST, form_list.getBuf()));
				}
			}
			else {
				THROW(P_Ref->PutPropVlrString(Obj, id, GDCPRP_PROPS, 0));
				THROW(PutDim(id, GDCPRP_DIMX, 0));
				THROW(PutDim(id, GDCPRP_DIMY, 0));
				THROW(PutDim(id, GDCPRP_DIMZ, 0));
				THROW(PutDim(id, GDCPRP_DIMW, 0));
				THROW(P_Ref->PutPropVlrString(Obj, id, GDCPRP_FORMLIST, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsClass::PutDim(PPID gdsClsID, PPID propID, PPGdsClsDim * pDim)
{
	int    ok = 1;
	SString line_buf;
	if(pDim) {
		char   temp_str[128];
		PPPutExtStrData(GDSCLS_EXTSTR_FLD_DIM_NAME,  line_buf, pDim->Name);
		PPPutExtStrData(GDSCLS_EXTSTR_FLD_DIM_SCALE, line_buf, ltoa(pDim->Scale, temp_str, 10));
		pDim->ToStr(0, temp_str, sizeof(temp_str));
		PPPutExtStrData(GDSCLS_EXTSTR_FLD_DIM_VALS, line_buf, temp_str);
	}
	ok = P_Ref->PutPropVlrString(Obj, gdsClsID, propID, line_buf);
	return ok;
}

int PPObjGoodsClass::GetDim(PPID gdsClsID, PPID propID, PPGdsClsDim * pDim)
{
	int    ok = 1;
	SString line_buf, temp_buf;
	if(P_Ref->GetPropVlrString(PPOBJ_GOODSCLASS, gdsClsID, propID, line_buf) > 0) {
		PPGetExtStrData(GDSCLS_EXTSTR_FLD_DIM_NAME,  line_buf, temp_buf);
		temp_buf.CopyTo(pDim->Name, sizeof(pDim->Name));
		PPGetExtStrData(GDSCLS_EXTSTR_FLD_DIM_SCALE, line_buf, temp_buf);
		pDim->Scale = temp_buf.ToLong();
		PPGetExtStrData(GDSCLS_EXTSTR_FLD_DIM_VALS, line_buf, temp_buf);
		pDim->FromStr(0, temp_buf);
	}
	return ok;
}
//
//
//
class GoodsClassDialog : public TDialog {
	DECL_DIALOG_DATA(PPGdsClsPacket);
public:
	GoodsClassDialog() : TDialog(DLG_GDSCLS)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_GDSCLS_NAME, Data.Rec.Name);
		setCtrlData(CTL_GDSCLS_SYMB, Data.Rec.Symb);
		setCtrlData(CTL_GDSCLS_ID, &Data.Rec.ID);
		setCtrlString(CTL_GDSCLS_NAMECONV,  Data.NameConv);
		setCtrlString(CTL_GDSCLS_ABBRCONV,  Data.AbbrConv);
		setCtrlString(CTL_GDSCLS_PHUPERU,   Data.PhUPerU_Formula);
		setCtrlString(CTL_GDSCLS_TAXFACTOR, Data.TaxMult_Formula);
		setCtrlString(CTL_GDSCLS_PACKAGE,   Data.Package_Formula);
		SetupPPObjCombo(this, CTLSEL_GDSCLS_UNIT, PPOBJ_UNIT, Data.Rec.DefUnitID, OLW_CANINSERT, reinterpret_cast<void *>(PPUnit::Trade));
		SetupPPObjCombo(this, CTLSEL_GDSCLS_PHUNIT, PPOBJ_UNIT, Data.Rec.DefPhUnitID, OLW_CANINSERT, reinterpret_cast<void *>(PPUnit::Trade|PPUnit::Physical));
		SetupPPObjCombo(this, CTLSEL_GDSCLS_GDSTYPE, PPOBJ_GOODSTYPE, Data.Rec.DefGoodsTypeID, OLW_CANINSERT);
		SetupPPObjCombo(this, CTLSEL_GDSCLS_TAXGRP, PPOBJ_GOODSTAX, Data.Rec.DefTaxGrpID, OLW_CANINSERT);
		SetupPPObjCombo(this, CTLSEL_GDSCLS_GRP, PPOBJ_GOODSGROUP, Data.Rec.DefGrpID, OLW_CANINSERT|OLW_LOADDEFONOPEN);
		setupCmdFlag(PPGdsCls::fUsePropKind,  CTL_GDSCLS_E_KIND,  cmGdsClsKind);
		setupCmdFlag(PPGdsCls::fUsePropGrade, CTL_GDSCLS_E_GRADE, cmGdsClsGrade);
		setupCmdFlag(PPGdsCls::fUsePropAdd,   CTL_GDSCLS_E_ADD,   cmGdsClsAdd);
		setupCmdFlag(PPGdsCls::fUsePropAdd2,  CTL_GDSCLS_E_ADD2,  cmGdsClsAdd2);
		setupCmdFlag(PPGdsCls::fUseDimX, CTL_GDSCLS_E_X, cmGdsClsX);
		setupCmdFlag(PPGdsCls::fUseDimY, CTL_GDSCLS_E_Y, cmGdsClsY);
		setupCmdFlag(PPGdsCls::fUseDimZ, CTL_GDSCLS_E_Z, cmGdsClsZ);
		setupCmdFlag(PPGdsCls::fUseDimW, CTL_GDSCLS_E_W, cmGdsClsW);
		AddClusterAssoc(CTL_GDSCLS_FLAGS, 0, PPGdsCls::fStdEditDlg);
		AddClusterAssoc(CTL_GDSCLS_FLAGS, 1, PPGdsCls::fDupCombine);
		AddClusterAssoc(CTL_GDSCLS_FLAGS, 2, PPGdsCls::fDisableFreeDim);
		AddClusterAssoc(CTL_GDSCLS_FLAGS, 3, PPGdsCls::fPrintDiffLots);
		SetClusterData(CTL_GDSCLS_FLAGS, Data.Rec.Flags);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTL_GDSCLS_NAME, Data.Rec.Name);
		getCtrlData(CTL_GDSCLS_SYMB, Data.Rec.Symb);
		if(Data.Rec.ID == 0)
			getCtrlData(CTL_GDSCLS_ID, &Data.Rec.ID);
		getCtrlString(CTL_GDSCLS_NAMECONV, Data.NameConv);
		getCtrlString(CTL_GDSCLS_ABBRCONV, Data.AbbrConv);
		getCtrlString(CTL_GDSCLS_PHUPERU,  Data.PhUPerU_Formula);
		getCtrlString(CTL_GDSCLS_TAXFACTOR,  Data.TaxMult_Formula);
		getCtrlString(CTL_GDSCLS_PACKAGE,    Data.Package_Formula);
		getCtrlData(CTLSEL_GDSCLS_UNIT,    &Data.Rec.DefUnitID);
		getCtrlData(CTLSEL_GDSCLS_PHUNIT,  &Data.Rec.DefPhUnitID);
		getCtrlData(CTLSEL_GDSCLS_GDSTYPE, &Data.Rec.DefGoodsTypeID);
		getCtrlData(CTLSEL_GDSCLS_TAXGRP,  &Data.Rec.DefTaxGrpID);
		getCtrlData(CTLSEL_GDSCLS_GRP,     &Data.Rec.DefGrpID);
		getCmdFlag(PPGdsCls::fUsePropKind,  CTL_GDSCLS_E_KIND);
		getCmdFlag(PPGdsCls::fUsePropGrade, CTL_GDSCLS_E_GRADE);
		getCmdFlag(PPGdsCls::fUsePropAdd,   CTL_GDSCLS_E_ADD);
		getCmdFlag(PPGdsCls::fUsePropAdd2,   CTL_GDSCLS_E_ADD2);
		getCmdFlag(PPGdsCls::fUseDimX, CTL_GDSCLS_E_X);
		getCmdFlag(PPGdsCls::fUseDimY, CTL_GDSCLS_E_Y);
		getCmdFlag(PPGdsCls::fUseDimZ, CTL_GDSCLS_E_Z);
		getCmdFlag(PPGdsCls::fUseDimW, CTL_GDSCLS_E_W);
		GetClusterData(CTL_GDSCLS_FLAGS, &Data.Rec.Flags);
		if(!(Data.Rec.Flags & (PPGdsCls::fDupCombine|PPGdsCls::fStdEditDlg)))
			THROW_PP(Data.NameConv.NotEmptyS(), PPERR_GDSCLSNMTMPLNEEDED);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERR
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   setupCmdFlag(uint struF, uint flgCtlID, uint cmdID);
	int    getCmdFlag(uint struF, uint flgCtlID);
	int    replyPropCmd(PPGdsClsProp *, int gcProp);
	int    replyDimCmd(PPGdsClsDim *);
	int    editDynGenMask();
	int    EditLotDimParam();
};

class GoodsClassProp2Dialog : public TDialog {
public:
	GoodsClassProp2Dialog() : TDialog(DLG_GDSCLSPROP2), OrgObjType(0), P_GcPack(0), GcProp(0)
	{
	}
	int    setDTS(const PPGdsClsProp *, int gcProp, const PPGdsClsPacket * pGcPack);
	int    getDTS(PPGdsClsProp *);
private:
	DECL_HANDLE_EVENT;
	void   createObject();
	void   replaceObject();
	PPGdsClsProp Data;
	int    GcProp;
	const  PPGdsClsPacket * P_GcPack;

	PPID   OrgObjType;
};

void GoodsClassProp2Dialog::createObject()
{
	getCtrlData(CTLSEL_GDSCLSPROP_OBJ, &Data.ItemsListID);
	if(!Data.ItemsListID) {
		PPID   dyn_obj_type = 0;
		PPObjDynamicObjs dyn_obj;
		if(dyn_obj.Edit(&dyn_obj_type, 0) == cmOK) {
			Data.ItemsListID = dyn_obj_type;
			SetupObjListCombo(this, CTLSEL_GDSCLSPROP_OBJ, Data.ItemsListID);
			enableCommand(cmGcPropViewObj,    Data.ItemsListID);
			enableCommand(cmGcPropReplaceObj, Data.ItemsListID);
			enableCommand(cmGcPropCreateObj, !Data.ItemsListID);
		}
	}
}

void GoodsClassProp2Dialog::replaceObject()
{
	getCtrlData(CTLSEL_GDSCLSPROP_OBJ, &Data.ItemsListID);
	if(Data.ItemsListID != OrgObjType && P_GcPack && P_GcPack->Rec.ID) {
		PPObjGoods goods_obj;
		LAssocArray goods_list;
		if(goods_obj.P_Tbl->GetExtPropRefList(P_GcPack->Rec.ID, GcProp, 0, &goods_list) > 0) {
			if(CONFIRMCRIT(PPCFM_REPLACEGCPROP)) {
				;
			}
			else {
				Data.ItemsListID = OrgObjType;
				setCtrlData(CTLSEL_GDSCLSPROP_OBJ, &Data.ItemsListID);
			}
		}
	}
	enableCommand(cmGcPropViewObj, Data.ItemsListID);
}

IMPL_HANDLE_EVENT(GoodsClassProp2Dialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND)
		switch(TVCMD) {
			case cmCBSelected:
				if(event.isCtlEvent(CTLSEL_GDSCLSPROP_OBJ))
					replaceObject();
				break;
			case cmGcPropViewObj:
				getCtrlData(CTLSEL_GDSCLSPROP_OBJ, &Data.ItemsListID);
				if(Data.ItemsListID)
					ShowObjects(Data.ItemsListID, 0);
				break;
			case cmGcPropCreateObj:
				createObject();
				break;
			default:
				return;
		}
	else
		return;
	clearEvent(event);
}

int GoodsClassProp2Dialog::setDTS(const PPGdsClsProp * pData, int gcProp, const PPGdsClsPacket * pGcPack)
{
	P_GcPack = pGcPack;
	GcProp = gcProp;
	Data = *pData;
	OrgObjType = Data.ItemsListID;
	setCtrlData(CTL_GDSCLSPROP_NAME, Data.Name);
	SetupObjListCombo(this, CTLSEL_GDSCLSPROP_OBJ, Data.ItemsListID);
	enableCommand(cmGcPropViewObj,    Data.ItemsListID);
	enableCommand(cmGcPropReplaceObj, Data.ItemsListID);
	enableCommand(cmGcPropCreateObj, !Data.ItemsListID);
	return 1;
}

int GoodsClassProp2Dialog::getDTS(PPGdsClsProp * pData)
{
	getCtrlData(CTL_GDSCLSPROP_NAME, Data.Name);
	getCtrlData(CTLSEL_GDSCLSPROP_OBJ, &Data.ItemsListID);
	ASSIGN_PTR(pData, Data);
	return 1;
}
//
//
//
// prototype
int EditGcFormulaList(PPGdsClsPacket * pData);

IMPL_HANDLE_EVENT(GoodsClassDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmGdsClsKind:  replyPropCmd(&Data.PropKind,  PPGdsCls::eKind);  break;
			case cmGdsClsGrade: replyPropCmd(&Data.PropGrade, PPGdsCls::eGrade); break;
			case cmGdsClsAdd:   replyPropCmd(&Data.PropAdd,   PPGdsCls::eAdd);   break;
			case cmGdsClsAdd2:  replyPropCmd(&Data.PropAdd2,  PPGdsCls::eAdd2);  break;
			case cmGdsClsX:     replyDimCmd(&Data.DimX);       break;
			case cmGdsClsY:     replyDimCmd(&Data.DimY);       break;
			case cmGdsClsZ:     replyDimCmd(&Data.DimZ);       break;
			case cmGdsClsW:     replyDimCmd(&Data.DimW);       break;
			case cmGdsClsFormList:   EditGcFormulaList(&Data); break;
			case cmGdsClsDynGenMask: editDynGenMask();         break;
			case cmGdsClsLotDim:     EditLotDimParam();        break;
			case cmClusterClk: 
				{
					uint   f_ctl_id = event.getCtlID();
					uint   stru_f = 0;
					uint   cmd_id = 0;
					switch(f_ctl_id) {
						case CTL_GDSCLS_E_KIND: stru_f = PPGdsCls::fUsePropKind; cmd_id = cmGdsClsKind; break;
						case CTL_GDSCLS_E_GRADE: stru_f = PPGdsCls::fUsePropGrade; cmd_id = cmGdsClsGrade; break;
						case CTL_GDSCLS_E_ADD: stru_f = PPGdsCls::fUsePropAdd; cmd_id = cmGdsClsAdd; break;
						case CTL_GDSCLS_E_ADD2: stru_f = PPGdsCls::fUsePropAdd2; cmd_id = cmGdsClsAdd2; break;
						case CTL_GDSCLS_E_X: stru_f = PPGdsCls::fUseDimX; cmd_id = cmGdsClsX; break;
						case CTL_GDSCLS_E_Y: stru_f = PPGdsCls::fUseDimY; cmd_id = cmGdsClsY; break;
						case CTL_GDSCLS_E_Z: stru_f = PPGdsCls::fUseDimZ; cmd_id = cmGdsClsZ; break;
						case CTL_GDSCLS_E_W: stru_f = PPGdsCls::fUseDimW; cmd_id = cmGdsClsW; break;
						case CTL_GDSCLS_FLAGS:
							{
								ushort v = 0;
								getCtrlData(CTL_GDSCLS_FLAGS, &v);
								SETFLAG(Data.Rec.Flags, PPGdsCls::fDupCombine, v & 0x02);
								/* @v5.1.9 Для поддержки уникальности записей GoodsExt
								 теперь используется специализированное поле UniqCntr
								if(v & 0x02) {
									Data.Rec.Flags &= ~PPGdsCls::fUsePropAdd;
									setupCmdFlag(PPGdsCls::fUsePropAdd, CTL_GDSCLS_E_ADD, cmGdsClsAdd);
									disableCtrl(CTL_GDSCLS_E_ADD, 1);
								}
								else
									disableCtrl(CTL_GDSCLS_E_ADD, 0);
								*/
							}
							break;
						default:
							return;
					}
					if(cmd_id) {
						getCmdFlag(stru_f, f_ctl_id);
						setupCmdFlag(stru_f, f_ctl_id, cmd_id);
					}
				}
				break;
			default:
				return;
		}
	}
	else
		return;
	clearEvent(event);
}

void GoodsClassDialog::setupCmdFlag(uint struF, uint flgCtlID, uint cmdID)
{
	ushort v = BIN(Data.Rec.Flags & struF);
	setCtrlData(flgCtlID, &v);
	enableCommand(cmdID, v);
}

int GoodsClassDialog::getCmdFlag(uint struF, uint flgCtlID)
{
	ushort v;
	getCtrlData(flgCtlID, &v);
	SETFLAG(Data.Rec.Flags, struF, v);
	return BIN(v);
}

int GdsClsParamMask::Edit(long options)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_GCDGM);
	if(CheckDialogPtrErr(&dlg)) {
		SetupPPObjCombo(dlg, CTLSEL_GCDGM_GDSCLS, PPOBJ_GOODSCLASS, ClsID, 0, 0);
		if(options & 0x0001)
			dlg->disableCtrl(CTLSEL_GCDGM_GDSCLS, 1);
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 0, 1<<(PPGdsCls::eKind-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 1, 1<<(PPGdsCls::eGrade-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 2, 1<<(PPGdsCls::eAdd-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 3, 1<<(PPGdsCls::eAdd2-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 4, 1<<(PPGdsCls::eX-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 5, 1<<(PPGdsCls::eY-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 6, 1<<(PPGdsCls::eZ-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 7, 1<<(PPGdsCls::eW-1));
		dlg->SetClusterData(CTL_GCDGM_MASK, Mask);
		if(ExecView(dlg) == cmOK) {
			GdsClsParamMask temp = *this;
			dlg->getCtrlData(CTLSEL_GCDGM_GDSCLS, &temp.ClsID);
			dlg->GetClusterData(CTL_GCDGM_MASK, &temp.Mask);
			*this = temp;
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int GoodsClassDialog::editDynGenMask()
{
	GdsClsParamMask param;
	param.ClsID = Data.Rec.ID;
	param.Mask = Data.Rec.DynGenMask;
	int    ok = param.Edit(1);
	if(ok > 0)
		Data.Rec.DynGenMask = param.Mask;
	return ok;
}

int GoodsClassDialog::EditLotDimParam()
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_GDSCLSLD);
	THROW(CheckDialogPtr(&dlg));
    SetupPPObjCombo(dlg, CTLSEL_GDSCLSLD_UNIT, PPOBJ_UNIT, Data.Rec.LotDimUnitID, 0, reinterpret_cast<void *>(PPUnit::Physical));
    dlg->setCtrlData(CTL_GDSCLSLD_NUMDIM, &Data.Rec.LotDimCount);
	dlg->setCtrlString(CTL_GDSCLSLD_QTTYFORM, Data.LotDimQtty_Formula);
    while(ok < 0 && ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTLSEL_GDSCLSLD_UNIT, &Data.Rec.LotDimUnitID);
		dlg->getCtrlData(CTL_GDSCLSLD_NUMDIM, &Data.Rec.LotDimCount);
		dlg->getCtrlString(CTL_GDSCLSLD_QTTYFORM, Data.LotDimQtty_Formula);
		ok = 1;
    }
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

/*
int GoodsClassDialog::editDynGenMask()
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_GCDGM);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 0, 1<<(PPGdsCls::eKind-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 1, 1<<(PPGdsCls::eGrade-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 2, 1<<(PPGdsCls::eAdd-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 3, 1<<(PPGdsCls::eAdd2-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 4, 1<<(PPGdsCls::eX-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 5, 1<<(PPGdsCls::eY-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 6, 1<<(PPGdsCls::eZ-1));
		dlg->AddClusterAssoc(CTL_GCDGM_MASK, 7, 1<<(PPGdsCls::eW-1));
		dlg->SetClusterData(CTL_GCDGM_MASK, Data.Rec.DynGenMask);
		if(ExecView(dlg) == cmOK)
			dlg->GetClusterData(CTL_GCDGM_MASK, &Data.Rec.DynGenMask);
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
*/

int GoodsClassDialog::replyPropCmd(PPGdsClsProp * pData, int gcProp)
{
	int    ok = -1;
	GoodsClassProp2Dialog * dlg = new GoodsClassProp2Dialog;
	if(CheckDialogPtrErr(&dlg) && dlg->setDTS(pData, gcProp, &Data)) {
		while(ok <= 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(pData))
				ok = 1;
	}
	else
		ok = 0;
	delete dlg;
	return ok;
	//DIALOG_PROC_BODY(GoodsClassPropDialog, pData);
}

int GoodsClassDialog::replyDimCmd(PPGdsClsDim * pData)
{
	class GoodsClassDimDialog : public TDialog {
		DECL_DIALOG_DATA(PPGdsClsDim);
	public:
		GoodsClassDimDialog() : TDialog(DLG_GDSCLSDIM)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			char   val_str[128];
			setCtrlData(CTL_GDSCLSDIM_NAME, Data.Name);
			setCtrlData(CTL_GDSCLSDIM_SCALE, &Data.Scale);
			memzero(val_str, sizeof(val_str));
			Data.ToStr(0, val_str, sizeof(val_str));
			setCtrlData(CTL_GDSCLSDIM_VALS, val_str);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			SString val_str;
			getCtrlData(CTL_GDSCLSDIM_NAME,  Data.Name);
			getCtrlData(CTL_GDSCLSDIM_SCALE, &Data.Scale);
			getCtrlString(CTL_GDSCLSDIM_VALS, val_str);
			Data.FromStr(0, val_str);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	};
	DIALOG_PROC_BODY(GoodsClassDimDialog, pData);
}
//
//
//
int PPObjGoodsClass::ReplacePropRefList(PPID prevObjType, PPID newObjType, LAssocArray * pAssc, LAssocArray * pBadRefList)
{
	int    ok = 1;
	LAssocArray new_assc, bad_ref_list;
	SString name_buf;
	THROW_PP(IS_REF_OBJTYPE(newObjType), PPERR_CANTUPDGDSCLSPROP);
	for(uint i = 0; i < pAssc->getCount(); i++) {
		PPID   new_val_id = 0;
		if(pAssc->at(i).Val) {
			if(GetObjectName(prevObjType, pAssc->at(i).Val, name_buf) > 0) {
				int r = P_Ref->SearchName(newObjType, &new_val_id, name_buf, 0);
				THROW(r);
				if(r < 0)
					new_val_id = 0;
			}
		}
		if(new_val_id) {
			THROW_SL(new_assc.Add(pAssc->at(i).Key, new_val_id, 0));
		}
		else {
			THROW_SL(bad_ref_list.Add(pAssc->at(i).Key, pAssc->at(i).Val, 0));
		}
	}
	*pAssc = new_assc;
	ASSIGN_PTR(pBadRefList, bad_ref_list);
	CATCHZOK
	return ok;
}

int PPObjGoodsClass::UpdatePropObjType(PPID clsID, int gcProp, PPID prevObjType, PPID newObjType, LAssocArray * pBadRefList)
{
	int    ok = -1;
	CALLPTRMEMB(pBadRefList, clear());
	if(prevObjType && prevObjType != newObjType) {
		LAssocArray assc_list;
		PPObjGoods goods_obj;
		goods_obj.P_Tbl->GetExtPropRefList(clsID, gcProp, 0, &assc_list);
		if(assc_list.getCount()) {
			THROW(ReplacePropRefList(prevObjType, newObjType, &assc_list, pBadRefList));
			THROW(goods_obj.P_Tbl->ReplaceExtObjRefs(clsID, gcProp, &assc_list, 0));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsClass::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel, valid_data = 0, is_new = 0;
	const  PPID org_id = DEREFPTRORZ(pID);
	PPObjGoods goods_obj;
	PPGdsClsPacket pack;
	GoodsClassDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new GoodsClassDialog)));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	dlg->setDTS(&pack);
	while(!valid_data && ExecView(dlg) == cmOK) {
		PPID prev_gc_kind  = pack.PropKind.ItemsListID;
		PPID prev_gc_grade = pack.PropGrade.ItemsListID;
		PPID prev_gc_add   = pack.PropAdd.ItemsListID;
		PPID prev_gc_add2  = pack.PropAdd2.ItemsListID;
		long prev_scale_x = pack.DimX.Scale;
		long prev_scale_y = pack.DimY.Scale;
		long prev_scale_z = pack.DimZ.Scale;
		long prev_scale_w = pack.DimW.Scale;
		THROW(is_new || CheckRights(PPR_MOD));
		if(dlg->getDTS(&pack)) {
			if(!CheckName(*pID, strip(pack.Rec.Name), 0))
				dlg->selectCtrl(CTL_PALLET_NAME);
			else if(*strip(pack.Rec.Symb) && !P_Ref->CheckUniqueSymb(Obj, pack.Rec.ID, pack.Rec.Symb, offsetof(PPGdsCls, Symb)))
				PPErrorByDialog(dlg, CTL_PALLET_SYMB);
			else {
				valid_data = 1;
				LAssocArray bad_ref_list;
				PPWaitStart();
				{
					PPTransaction tra(1);
					THROW(tra);
					THROW(PutPacket(pID, &pack, 0));
					THROW(UpdatePropObjType(*pID, PPGdsCls::eKind,  prev_gc_kind,  pack.PropKind.ItemsListID,  &bad_ref_list));
					THROW(UpdatePropObjType(*pID, PPGdsCls::eGrade, prev_gc_grade, pack.PropGrade.ItemsListID, &bad_ref_list));
					THROW(UpdatePropObjType(*pID, PPGdsCls::eAdd,   prev_gc_add,   pack.PropAdd.ItemsListID,   &bad_ref_list));
					THROW(UpdatePropObjType(*pID, PPGdsCls::eAdd2,  prev_gc_add2,  pack.PropAdd2.ItemsListID,  &bad_ref_list));
					if(org_id) {
						THROW(goods_obj.P_Tbl->ReplaceExtDimScale(*pID, PPGdsCls::eX, prev_scale_x, pack.DimX.Scale, 0));
						THROW(goods_obj.P_Tbl->ReplaceExtDimScale(*pID, PPGdsCls::eY, prev_scale_y, pack.DimY.Scale, 0));
						THROW(goods_obj.P_Tbl->ReplaceExtDimScale(*pID, PPGdsCls::eZ, prev_scale_z, pack.DimZ.Scale, 0));
						THROW(goods_obj.P_Tbl->ReplaceExtDimScale(*pID, PPGdsCls::eW, prev_scale_w, pack.DimW.Scale, 0));
					}
					THROW(tra.Commit());
				}
				PPWaitStop();
				ok = cmOK;
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPObjGoodsClass::Print(PPID id)
{
	return PPAlddPrint(REPORT_GOODSCLASSVIEW, PView((void *)0), 0);
}

int PPObjGoodsClass::Browse(void * extraPtr)
{
	class GdsClsView : public ObjViewDialog {
	public:
		GdsClsView(PPObjGoodsClass * _ppobj) : ObjViewDialog(DLG_GDSCLSVIEW, _ppobj, 0)
		{
		}
	private:
		virtual void extraProc(long id)
		{
			if(id) {
				GoodsFilt flt;
				flt.Ep.GdsClsID = id;
				PPView::Execute(PPVIEW_GOODS, &flt, 1, 0);
			}
		}
		virtual int Print()
		{
			return P_Obj ? static_cast<PPObjGoodsClass *>(P_Obj)->Print(0) : 0;
		}
	};
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		TDialog * dlg = new GdsClsView(this);
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}
//
//
//
class GoodsClassCache : public ObjCache {
public:
	GoodsClassCache() : ObjCache(PPOBJ_GOODSCLASS, sizeof(GoodsClassData)) {}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct GoodsClassData : public ObjCacheEntry {
		PPID   DefUnitID;
		PPID   DefPhUnitID;
		long   DefPhUPerU;
		PPID   DefTaxGrpID;
		PPID   DefGoodsTypeID;
		PPID   DefGrpID;
		uint16 EditDlgID;
		uint16 FiltDlgID;
		long   Flags;
		int16  DynGenMask;
		uint8  LotDimCount;    // 
		uint8  Reserve[1];     // @alignment
		PPID   LotDimUnitID;   // 
		PPID   KindID;
		PPID   GradeID;
		PPID   AddPropID;
		PPID   AddProp2ID;
		int16  ScaleX;
		int16  ScaleY;
		int16  ScaleZ;
		int16  ScaleW;
	};
};

int GoodsClassCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	GoodsClassData * p_cache_rec = static_cast<GoodsClassData *>(pEntry);
	PPObjGoodsClass gc_obj;
	PPGdsClsPacket gc_pack;
	if(gc_obj.GetPacket(id, &gc_pack) > 0) {
		p_cache_rec->ID             = gc_pack.Rec.ID;
		p_cache_rec->DefUnitID      = gc_pack.Rec.DefUnitID;
		p_cache_rec->DefPhUnitID    = gc_pack.Rec.DefPhUnitID;
		p_cache_rec->DefPhUPerU     = gc_pack.Rec.DefPhUPerU;
		p_cache_rec->DefTaxGrpID    = gc_pack.Rec.DefTaxGrpID;
		p_cache_rec->DefGoodsTypeID = gc_pack.Rec.DefGoodsTypeID;
		p_cache_rec->DefGrpID       = gc_pack.Rec.DefGrpID;
		p_cache_rec->EditDlgID      = gc_pack.Rec.EditDlgID;
		p_cache_rec->FiltDlgID      = gc_pack.Rec.FiltDlgID;
		p_cache_rec->Flags          = gc_pack.Rec.Flags;
		p_cache_rec->DynGenMask     = static_cast<int16>(gc_pack.Rec.DynGenMask);
		p_cache_rec->LotDimCount    = gc_pack.Rec.LotDimCount;
		p_cache_rec->LotDimUnitID   = gc_pack.Rec.LotDimUnitID;
		p_cache_rec->KindID     = gc_pack.PropKind.ItemsListID;
		p_cache_rec->GradeID    = gc_pack.PropGrade.ItemsListID;
		p_cache_rec->AddPropID  = gc_pack.PropAdd.ItemsListID;
		p_cache_rec->AddProp2ID = gc_pack.PropAdd2.ItemsListID;
		p_cache_rec->ScaleX  = static_cast<int16>(gc_pack.DimX.Scale);
		p_cache_rec->ScaleY  = static_cast<int16>(gc_pack.DimY.Scale);
		p_cache_rec->ScaleZ  = static_cast<int16>(gc_pack.DimZ.Scale);
		p_cache_rec->ScaleW  = static_cast<int16>(gc_pack.DimW.Scale);

		PPStringSetSCD ss;
		ss.add(gc_pack.Rec.Name);
		ss.add(gc_pack.Rec.Symb);
		ss.add(gc_pack.NameConv);
		ss.add(gc_pack.AbbrConv);
		ss.add(gc_pack.PhUPerU_Formula);
		ss.add(gc_pack.TaxMult_Formula);
		ss.add(gc_pack.LotDimQtty_Formula);
		PutName(ss.getBuf(), p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void GoodsClassCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPGdsClsPacket * p_data_pack = static_cast<PPGdsClsPacket *>(pDataRec);
	const GoodsClassData * p_cache_rec = static_cast<const GoodsClassData *>(pEntry);
	p_data_pack->Init();
	p_data_pack->Rec.Tag         = PPOBJ_GOODSCLASS;
	p_data_pack->Rec.ID          = p_cache_rec->ID;
	p_data_pack->Rec.DefUnitID   = p_cache_rec->DefUnitID;
	p_data_pack->Rec.DefPhUnitID = p_cache_rec->DefPhUnitID;
	p_data_pack->Rec.DefPhUPerU  = p_cache_rec->DefPhUPerU;
	p_data_pack->Rec.DefTaxGrpID = p_cache_rec->DefTaxGrpID;
	p_data_pack->Rec.DefGoodsTypeID = p_cache_rec->DefGoodsTypeID;
	p_data_pack->Rec.DefGrpID   = p_cache_rec->DefGrpID;
	p_data_pack->Rec.EditDlgID  = p_cache_rec->EditDlgID;
	p_data_pack->Rec.FiltDlgID  = p_cache_rec->FiltDlgID;
	p_data_pack->Rec.Flags      = p_cache_rec->Flags;
	p_data_pack->Rec.DynGenMask = static_cast<long>(p_cache_rec->DynGenMask);
	p_data_pack->Rec.LotDimCount  = p_cache_rec->LotDimCount;
	p_data_pack->Rec.LotDimUnitID = p_cache_rec->LotDimUnitID;
	p_data_pack->PropKind.ItemsListID  = p_cache_rec->KindID;
	p_data_pack->PropGrade.ItemsListID = p_cache_rec->GradeID;
	p_data_pack->PropAdd.ItemsListID   = p_cache_rec->AddPropID;
	p_data_pack->PropAdd2.ItemsListID  = p_cache_rec->AddProp2ID;
	p_data_pack->DimX.Scale = p_cache_rec->ScaleX;
	p_data_pack->DimY.Scale = p_cache_rec->ScaleY;
	p_data_pack->DimZ.Scale = p_cache_rec->ScaleZ;
	p_data_pack->DimW.Scale = p_cache_rec->ScaleW;

	char   temp_buf[1024];
	GetName(pEntry, temp_buf, sizeof(temp_buf));
	PPStringSetSCD ss;
	ss.setBuf(temp_buf, sstrlen(temp_buf)+1);
	uint   p = 0;
	ss.get(&p, p_data_pack->Rec.Name, sizeof(p_data_pack->Rec.Name));
	ss.get(&p, p_data_pack->Rec.Symb, sizeof(p_data_pack->Rec.Symb));
	ss.get(&p, p_data_pack->NameConv);
	ss.get(&p, p_data_pack->AbbrConv);
	ss.get(&p, p_data_pack->PhUPerU_Formula);
	ss.get(&p, p_data_pack->TaxMult_Formula);
	ss.get(&p, p_data_pack->LotDimQtty_Formula);
}

int PPObjGoodsClass::Fetch(PPID id, PPGdsClsPacket * pRec)
{
	GoodsClassCache * p_cache = GetDbLocalCachePtr <GoodsClassCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : GetPacket(id, pRec);
}
//
//
//
PPGdsClsFormula::PPGdsClsFormula() : Flags(0)
{
}

void PPGdsClsFormula::Clear()
{
	Flags = 0;
	Name.Z();
	Formula.Z();
	Var1Name.Z();
	Var2Name.Z();
	Var3Name.Z();
}

int FASTCALL PPGdsClsFormula::PutToBuffer(SString & rBuf) const
{
	rBuf.Z();
	PPPutExtStrData(GDSCLS_EXTSTR_FROM_NAME, rBuf, Name);
	PPPutExtStrData(GDSCLS_EXTSTR_FROM_FORM, rBuf, Formula);
	PPPutExtStrData(GDSCLS_EXTSTR_FROM_PAR1, rBuf, Var1Name);
	PPPutExtStrData(GDSCLS_EXTSTR_FROM_PAR2, rBuf, Var2Name);
	PPPutExtStrData(GDSCLS_EXTSTR_FROM_PAR3, rBuf, Var3Name);
	SString temp_buf;
	temp_buf.Cat(Flags);
	PPPutExtStrData(GDSCLS_EXTSTR_FROM_FLAGS, rBuf, temp_buf);
	return 1;
}

int FASTCALL PPGdsClsFormula::GetFromBuffer(const char * pBuf)
{
	SString temp_buf;
	const SString line_buf(pBuf);
	Clear();
	PPGetExtStrData(GDSCLS_EXTSTR_FROM_NAME, line_buf, Name);
	PPGetExtStrData(GDSCLS_EXTSTR_FROM_FORM, line_buf, Formula);
	PPGetExtStrData(GDSCLS_EXTSTR_FROM_PAR1, line_buf, Var1Name);
	PPGetExtStrData(GDSCLS_EXTSTR_FROM_PAR2, line_buf, Var2Name);
	PPGetExtStrData(GDSCLS_EXTSTR_FROM_PAR3, line_buf, Var3Name);
	PPGetExtStrData(GDSCLS_EXTSTR_FROM_FLAGS, line_buf, temp_buf);
	Flags = temp_buf.ToLong();
	return 1;
}

int PPGdsClsPacket::PutFormula(int * pIdx, const PPGdsClsFormula * pForm)
{
	int    ok = 1;
	int    idx = *pIdx;
	SString buf;
	if(idx && idx <= FormulaList.getCountI()) {
		if(pForm) {
			FormulaList.atFree(idx-1);
			pForm->PutToBuffer(buf);
			FormulaList.atInsert(idx-1, newStr(buf));
		}
		else
			FormulaList.atFree(idx-1);
	}
	else if(pForm) {
		pForm->PutToBuffer(buf);
		FormulaList.insert(newStr(buf));
		*pIdx = FormulaList.getCount();
	}
	return ok;
}

int PPGdsClsPacket::GetFormula(int idx, PPGdsClsFormula * pForm) const
{
	if(idx > 0 && idx <= FormulaList.getCountI()) {
		pForm->GetFromBuffer(FormulaList.at(idx-1));
		return 1;
	}
	else {
		pForm->Clear();
		return 0;
	}
}

int PPGdsClsPacket::SetDefaultFormula(int idx)
{
	const int cnt = FormulaList.getCountI();
	if(idx > 0 && idx <= cnt) {
		for(int i = cnt; i > 0; i--) {
			PPGdsClsFormula form;
			if(GetFormula(i, &form)) {
				SETFLAG(form.Flags, PPGdsClsFormula::fDefault, i == idx);
				int temp_idx = i;
				PutFormula(&temp_idx, &form);
			}
		}
		return 1;
	}
	else
		return -1;
}
//
//
//
static int EditGcFormula(PPGdsClsFormula * pForm)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_GDSCLSFORM);
	if(CheckDialogPtrErr(&dlg)) {
		PPGdsClsFormula form = *pForm;
		dlg->setCtrlString(CTL_GDSCLSFORM_NAME, form.Name);
		dlg->setCtrlString(CTL_GDSCLSFORM_FORM, form.Formula);
		dlg->setCtrlString(CTL_GDSCLSFORM_PAR1, form.Var1Name);
		dlg->setCtrlString(CTL_GDSCLSFORM_PAR2, form.Var2Name);
		dlg->setCtrlString(CTL_GDSCLSFORM_PAR3, form.Var3Name);
		dlg->AddClusterAssoc(CTL_GDSCLSFORM_FLAGS, 0, PPGdsClsFormula::fDefault);
		dlg->SetClusterData(CTL_GDSCLSFORM_FLAGS, form.Flags);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_GDSCLSFORM_NAME, form.Name);
			if(form.Name.NotEmptyS()) {
				dlg->getCtrlString(CTL_GDSCLSFORM_FORM, form.Formula);
				dlg->getCtrlString(CTL_GDSCLSFORM_PAR1, form.Var1Name);
				dlg->getCtrlString(CTL_GDSCLSFORM_PAR2, form.Var2Name);
				dlg->getCtrlString(CTL_GDSCLSFORM_PAR3, form.Var3Name);
				dlg->GetClusterData(CTL_GDSCLSFORM_FLAGS, &form.Flags);
				*pForm = form;
				ok = 1;
			}
			else
				PPErrorByDialog(dlg, CTL_GDSCLSFORM_NAME, PPERR_NAMENEEDED);
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

class GdsClsFormListDialog : public PPListDialog {
public:
	GdsClsFormListDialog(PPGdsClsPacket * pData) : PPListDialog(DLG_GDSCLSFORMLIST, CTL_GDSCLSFORMLIST_LIST), P_Data(pData)
	{
		updateList(-1);
	}
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	PPGdsClsPacket * P_Data;
};

int GdsClsFormListDialog::setupList()
{
	SString temp_buf;
	for(uint i = 1; i <= P_Data->FormulaList.getCount(); i++) {
		PPGdsClsFormula form;
		P_Data->GetFormula(i, &form);
		temp_buf.Z();
		if(form.Flags & PPGdsClsFormula::fDefault)
			temp_buf.CatChar('!');
		temp_buf.Cat(form.Name);
		if(!addStringToList(i, temp_buf))
			return 0;
	}
	return 1;
}

int GdsClsFormListDialog::addItem(long *, long * pID)
{
	int    ok = -1;
	PPGdsClsFormula form;
	if(EditGcFormula(&form) > 0) {
		int    idx = 0;
		if(!P_Data->PutFormula(&idx, &form)) {
			ok = PPErrorZ();
		}
		else {
			if(form.Flags & PPGdsClsFormula::fDefault)
				P_Data->SetDefaultFormula(idx);
			ASSIGN_PTR(pID, idx);
			ok = 1;
		}
	}
	return ok;
}

int GdsClsFormListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	int    idx = id;
	PPGdsClsFormula form;
	if(P_Data->GetFormula(idx, &form)) {
		if(EditGcFormula(&form) > 0) {
			if(!P_Data->PutFormula(&idx, &form))
				ok = PPErrorZ();
			else {
				if(form.Flags & PPGdsClsFormula::fDefault)
					P_Data->SetDefaultFormula(idx);
				ok = 1;
			}
		}
	}
	return ok;
}

int GdsClsFormListDialog::delItem(long pos, long id)
{
	int    ok = -1;
	int    idx = id;
	PPGdsClsFormula form;
	if(P_Data->GetFormula(idx, &form))
		if(!P_Data->PutFormula(&idx, 0))
			ok = PPErrorZ();
		else
			ok = 1;
	return ok;
}

int EditGcFormulaList(PPGdsClsPacket * pData)
{
	int    ok = -1;
	PPGdsClsPacket temp_data;
	temp_data = *pData;
	GdsClsFormListDialog * dlg = new GdsClsFormListDialog(&temp_data);
	if(CheckDialogPtrErr(&dlg)) {
		if(ExecView(dlg) == cmOK) {
			*pData = temp_data;
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
class GcCalcDialog : public TDialog {
public:
	GcCalcDialog() : TDialog(DLG_GDSCLSCALC), Result(0.0), IsThereResult(0), TSessID(0)
	{
	}
	int    setDTS(PPID goodsID, PPID tsessID);
	int    getDTS(double * pResult);
	void   setArg(int argIdx /* 1..3 */, double arg);
private:
	DECL_HANDLE_EVENT;
	int    setupFormulaCombo();
	void   setupFormula();
	int    calc();

	PPObjGoods GObj;
	PPObjGoodsClass GcObj;
	PPGoodsPacket Pack;
	PPGdsClsPacket GcPack;
	PPID   TSessID;
	int    IsThereResult;
	double Result;
};

void GcCalcDialog::setupFormula()
{
	PPGdsClsFormula form;
	long   idx = getCtrlLong(CTLSEL_GDSCLSCALC_FORM);
	if(idx && GcPack.GetFormula(idx, &form) > 0)
		selectCtrl(CTL_GDSCLSCALC_PAR1);
	setLabelText(CTL_GDSCLSCALC_PAR1, form.Var1Name);
	disableCtrl(CTL_GDSCLSCALC_PAR1,  form.Var1Name.IsEmpty());
	setLabelText(CTL_GDSCLSCALC_PAR2, form.Var2Name);
	disableCtrl(CTL_GDSCLSCALC_PAR2,  form.Var2Name.IsEmpty());
	setLabelText(CTL_GDSCLSCALC_PAR3, form.Var3Name);
	disableCtrl(CTL_GDSCLSCALC_PAR3,  form.Var3Name.IsEmpty());
}

IMPL_HANDLE_EVENT(GcCalcDialog)
{
	if(event.isCmd(cmOK)) {
		calc();
		clearEvent(event);
	}
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_GDSCLSCALC_FORM)) {
		setupFormula();
		clearEvent(event);
	}
}

int GcCalcDialog::calc()
{
	int    ok  = 0;
	long   idx = 0;
	PPGdsClsFormula form;
	getCtrlData(CTLSEL_GDSCLSCALC_FORM, &idx);
	if(idx && GcPack.GetFormula(idx, &form) > 0 && form.Formula.NotEmpty()) {
		SString par_buf;
		GdsClsCalcExprContext ctx(&GcPack, &Pack);
		ctx.TSessID = TSessID;
		getCtrlString(CTL_GDSCLSCALC_PAR1, par_buf);
		ctx.Par1 = par_buf.ToReal();
		getCtrlString(CTL_GDSCLSCALC_PAR2, par_buf);
		ctx.Par2 = par_buf.ToReal();
		getCtrlString(CTL_GDSCLSCALC_PAR3, par_buf);
		ctx.Par3 = par_buf.ToReal();
		if(PPCalcExpression(form.Formula, &Result, &ctx))
			ok = 1;
		par_buf.Z().Cat(Result, MKSFMTD(0, 6, NMBF_NOTRAILZ));
		setCtrlString(CTL_GDSCLSCALC_VAL, par_buf);
		IsThereResult = 1;
	}
	else
		IsThereResult = 0;
	return ok;
}

int GcCalcDialog::setupFormulaCombo()
{
	int    ok = 1;
	ComboBox   * p_cb = 0;
	ListWindow * p_lw = 0;
	if((p_cb = static_cast<ComboBox *>(getCtrlView(CTLSEL_GDSCLSCALC_FORM))) != 0) {
		long   single_idx = 0, default_idx = 0;
		uint   idx = 0;
		PPGdsClsFormula form;
		THROW(p_lw = CreateListWindow_Simple(lbtDblClkNotify));
		for(int i = 1; i <= GcPack.FormulaList.getCountI(); i++)
			if(GcPack.GetFormula(i, &form) > 0) {
				p_lw->listBox()->addItem(i, form.Name);
				if(form.Flags & PPGdsClsFormula::fDefault)
					default_idx = i;
				single_idx = (single_idx == 0) ? i : 0;
			}
		p_cb->setListWindow(p_lw, NZOR(default_idx, single_idx));
		if(single_idx)
			setupFormula();
	}
	CATCHZOKPPERR
	return ok;
}

void GcCalcDialog::setArg(int argIdx, double arg)
{
	if(argIdx >= 1 && argIdx <= 3) {
		long   idx = 0;
		PPGdsClsFormula form;
		getCtrlData(CTLSEL_GDSCLSCALC_FORM, &idx);
		if(idx && GcPack.GetFormula(idx, &form) > 0 && form.Formula.NotEmpty()) {
			SString par_buf;
			uint ctl_id = CTL_GDSCLSCALC_PAR1 + argIdx - 1;
			par_buf.Cat(arg, MKSFMTD(0, 6, NMBF_NOTRAILZ));
			setCtrlString(ctl_id, par_buf);
			calc();
		}
	}
}

int GcCalcDialog::setDTS(PPID goodsID, PPID tsessID)
{
	int    ok = 0;
	Result = 0.0;
	TSessID = tsessID;
	if(GObj.GetPacket(goodsID, &Pack, PPObjGoods::gpoSkipQuot) > 0 && Pack.Rec.GdsClsID && GcObj.GetPacket(Pack.Rec.GdsClsID, &GcPack) > 0) {
		if(GcPack.FormulaList.getCount()) {
			setupFormulaCombo();
			ok = 1;
		}
	}
	return ok;
}

int GcCalcDialog::getDTS(double * pResult)
{
	ASSIGN_PTR(pResult, Result);
	return IsThereResult ? 1 : -1;
}

int PPGoodsCalculator(PPID goodsID, PPID tsesID, int argIdx, double arg, double * pVal)
{
	int    ok = -1;
	GcCalcDialog * dlg = new GcCalcDialog;
	if(CheckDialogPtrErr(&dlg)) {
		if(dlg->setDTS(goodsID, tsesID)) {
			dlg->setArg(argIdx, arg);
			ExecView(dlg);
			ok = dlg->getDTS(pVal);
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
