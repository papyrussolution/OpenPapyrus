// OBJSALAR.CPP
// Copyright (c) A.Starodub, A.Sobolev 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>

SLAPI PPSalChargePacket::PPSalChargePacket()
{
	MEMSZERO(Rec);
}

int SLAPI PPSalChargePacket::Init()
{
	MEMSZERO(Rec);
	Formula = 0;
	GrpList.freeAll();
	return 1;
}

static int SalChargeFilt(void * pRec, void * extraPtr)
{
	const long extra_param = (long)extraPtr;
	if(((PPSalCharge *)pRec)->Flags & PPSalCharge::fGroup)
		return (extra_param == -10000) ? 0 : 1;
	else
		return (extra_param == -1000) ? 0 : 1;
}

SLAPI PPObjSalCharge::PPObjSalCharge(void * extraPtr) : PPObjReference(PPOBJ_SALCHARGE, extraPtr)
{
	filt = SalChargeFilt;
}

class SalChargeDialog : public PPListDialog {
public:
	SalChargeDialog(int grp) : PPListDialog(grp ? DLG_SALCHGRP : DLG_SALCHARGE, CTL_SALCHARGE_LIST)
	{
	}
	int    setDTS(const PPSalChargePacket * pData);
	int    getDTS(PPSalChargePacket * pData);
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int delItem(long pos, long id);
	virtual int moveItem(long pos, long id, int up);
	int    SetupEnumExt();

	PPSalChargePacket Data;
};

IMPL_HANDLE_EVENT(SalChargeDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmSalChargeSelSymb) || (event.isKeyDown(kbF2) && isCurrCtlID(CTL_SALCHARGE_FORMULA))) {
		int    kind = 0;
		PPID   symb_id = 0;
		long   o = selSymbAmount|selSymbFormula|selSymbStaffCal|selSymbSalPeriod;
		SString symb;
		if(SelectAmountSymb(&symb_id, o, &kind, symb) > 0) {
			TInputLine * p_il = (TInputLine *)getCtrlView(CTL_SALCHARGE_FORMULA);
			if(p_il) {
				SString input;
				p_il->getText(input);
				size_t pos = p_il->getCaret();
				input.Insert(pos, symb.Quot(' ', ' '));
				p_il->setText(input);
				p_il->Draw_();
				p_il->selectAll(0);
				p_il->setCaret(pos + symb.Len());
			}
		}
		clearEvent(event);
	}
	else if(event.isClusterClk(CTL_SALCHARGE_ENUM)) {
		PPID prev_enum = Data.Rec.EnumObjType;
		GetClusterData(CTL_SALCHARGE_ENUM, &Data.Rec.EnumObjType);
		if(Data.Rec.EnumObjType != prev_enum) {
			SetupEnumExt();
		}
		clearEvent(event);
	}
}

int SalChargeDialog::SetupEnumExt()
{
	if(Data.Rec.EnumObjType == PPOBJ_PERSONEVENT) {
		disableCtrl(CTLSEL_SALCHARGE_ENUMEXT, 0);
		SetupPPObjCombo(this, CTLSEL_SALCHARGE_ENUMEXT, PPOBJ_PERSONOPKIND, Data.Rec.EnumExtVal, 0, 0);
	}
	else {
		setCtrlLong(CTLSEL_SALCHARGE_ENUMEXT, Data.Rec.EnumExtVal = 0);
		disableCtrl(CTLSEL_SALCHARGE_ENUMEXT, 1);
	}
	return 1;
}

int SalChargeDialog::setDTS(const PPSalChargePacket * pData)
{
	if(!RVALUEPTR(Data, pData))
		Data.Init();
	setCtrlData(CTL_SALCHARGE_ID,        &Data.Rec.ID);
	setCtrlData(CTL_SALCHARGE_NAME,      Data.Rec.Name);
	setCtrlData(CTL_SALCHARGE_SYMB,      Data.Rec.Symb);

	AddClusterAssocDef(CTL_SALCHARGE_ENUM, 0, 0);
	AddClusterAssoc(CTL_SALCHARGE_ENUM, 1, PPOBJ_PERSONEVENT);
	SetClusterData(CTL_SALCHARGE_ENUM, Data.Rec.EnumObjType);
	SetupEnumExt();

	setCtrlString(CTL_SALCHARGE_FORMULA, Data.Formula);
	disableCtrl(CTL_SALCHARGE_ID,        Data.Rec.ID);
	//SetupPPObjCombo(this, CTLSEL_SALCHARGE_STAFCAL, PPOBJ_STAFFCAL, Data.Rec.CalID, 0, 0);
	SetupAmtTypeCombo(this, CTLSEL_SALCHARGE_AMTTYPE, Data.Rec.AmtID, OLW_CANINSERT, PPObjAmountType::selStaffOnly, 0);
	PPIDArray op_type_list;
	op_type_list.add(PPOPT_ACCTURN);
	SetupOprKindCombo(this, CTLSEL_SALCHARGE_WROFFOP, Data.Rec.WrOffOpID, OLW_CANINSERT, &op_type_list, 0);
	updateList(-1);
	return 1;
}

int SalChargeDialog::getDTS(PPSalChargePacket * pData)
{
	getCtrlData(CTL_SALCHARGE_ID,          &Data.Rec.ID);
	getCtrlData(CTL_SALCHARGE_NAME,        Data.Rec.Name);
	getCtrlData(CTL_SALCHARGE_SYMB,        Data.Rec.Symb);
	GetClusterData(CTL_SALCHARGE_ENUM, &Data.Rec.EnumObjType);
	Data.Rec.EnumExtVal = getCtrlLong(CTLSEL_SALCHARGE_ENUMEXT);
	getCtrlString(CTL_SALCHARGE_FORMULA,   Data.Formula);
	//getCtrlData(CTLSEL_SALCHARGE_STAFCAL,  &Data.Rec.CalID);
	Data.Rec.AmtID     = getCtrlLong(CTLSEL_SALCHARGE_AMTTYPE);
	Data.Rec.WrOffOpID = getCtrlLong(CTLSEL_SALCHARGE_WROFFOP);
	ASSIGN_PTR(pData, Data);
	return 1;
}

int SalChargeDialog::setupList()
{
	int    ok = 1;
	SString objname;
	PPObjSalCharge schg_obj;
	PPSalChargePacket schg_pack;
	for(uint i = 0; ok && i < Data.GrpList.getCount(); i++) {
		PPID   id = Data.GrpList.at(i);
		if(schg_obj.Fetch(id, &schg_pack) > 0)
			objname = schg_pack.Rec.Name;
		else
			ideqvalstr(id, objname);
		if(!addStringToList(id, objname))
			ok = 0;
	}
	return ok;
}

int SalChargeDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	PPID   id = 0;
	if(ListBoxSelDialog(PPOBJ_SALCHARGE, &id, (void *)-10000) > 0 && Data.GrpList.lsearch(id) <= 0) {
		Data.GrpList.addUnique(id);
		ASSIGN_PTR(pPos, Data.GrpList.getCount()-1);
		ASSIGN_PTR(pID, id);
		ok = 1;
	}
	return ok;
}

int SalChargeDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < (long)Data.GrpList.getCount() && CONFIRM(PPCFM_DELSALCHRGGRPITEM)) {
		Data.GrpList.atFree(pos);
		ok = 1;
	}
	return ok;
}

int SalChargeDialog::moveItem(long pos, long id, int up)
{
	return Data.GrpList.swap(pos, up ? pos-1 : pos+1) ? 1 : -1;
}

int SLAPI PPObjSalCharge::Edit(PPID * pID, void * extraPtr)
{
	const  long extra_param = (long)extraPtr;
	int    ok = -1, r = cmCancel, is_new = 0;
	PPSalChargePacket pack;
	SalChargeDialog * p_dlg = 0;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack));
	}
	else if(extra_param == -1000)
		pack.Rec.Flags |= PPSalCharge::fGroup;
	THROW(CheckDialogPtr(&(p_dlg = new SalChargeDialog((pack.Rec.Flags & PPSalCharge::fGroup) ? 1 : 0))));
	THROW(EditPrereq(pID, p_dlg, 0));
	p_dlg->setDTS(&pack);
	while(ok < 0 && (r = ExecView(p_dlg)) == cmOK) {
		THROW(is_new || CheckRights(PPR_MOD));
		if(p_dlg->getDTS(&pack))
			if(PutPacket(pID, &pack, 1))
				ok = 1;
			else
				PPError();
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok ? r : 0;
}

int SLAPI PPObjSalCharge::Browse(void * extraPtr)
{
	class SalChargeView : public ObjViewDialog {
	public:
		SalChargeView(PPObjSalCharge * _ppobj) : ObjViewDialog(DLG_SALCHARGEVIEW, _ppobj, 0)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			ObjViewDialog::handleEvent(event);
			if(event.isCmd(cmTransmitCharry)) {
				PPIDArray id_list, grp_id_list;
				PPSalCharge rec;
				for(PPID id = 0; ((PPObjReference *)P_Obj)->EnumItems(&id, &rec) > 0;) {
					if(rec.Flags & PPSalCharge::fGroup)
						grp_id_list.add(id);
					else
						id_list.add(id);
				}
				if(!SendCharryObject(PPDS_CRRSALCHARGE, id_list) || !SendCharryObject(PPDS_CRRSALCHARGEGROUP, grp_id_list))
					PPError();
				clearEvent(event);
			}
		}
		virtual void extraProc(long id)
		{
			PPObjSalCharge * p_obj = (PPObjSalCharge *)P_Obj;
			if(p_obj) {
				PPID   new_id = 0;
				if(p_obj->Edit(&new_id, (void *)-1000) == cmOK)
					updateList(new_id);
			}
		}
	};
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		TDialog * dlg = new SalChargeView(/*DLG_SALCHGRPV*/this);
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}

int SLAPI PPObjSalCharge::GetPacket(PPID id, PPSalChargePacket * pPack)
{
	int    ok = -1, r;
	if(pPack) {
		pPack->Init();
		THROW(r = Search(id, &pPack->Rec));
		if(r > 0) {
			THROW(ref->GetPropVlrString(Obj, id, SCPRP_FORMULA, pPack->Formula));
			THROW(ref->GetPropArray(Obj, id, SCPPRP_GRPLIST, &pPack->GrpList));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjSalCharge::PutPacket(PPID * pID, PPSalChargePacket * pPack, int useTa)
{
	int    ok = 1;
	{
		PPTransaction tra(useTa);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				THROW(CheckDupName(*pID, pPack->Rec.Name));
				THROW(ref->CheckUniqueSymb(Obj, *pID, pPack->Rec.Symb, offsetof(PPSalCharge, Symb)));
				THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
				THROW(ref->PutPropVlrString(Obj, *pID, SCPRP_FORMULA, pPack->Formula));
				THROW(ref->PutPropArray(Obj, *pID, SCPPRP_GRPLIST, &pPack->GrpList, 0));
			}
			else {
				THROW(ref->RemoveItem(Obj, *pID, 0));
				THROW(ref->PutPropVlrString(Obj, *pID, SCPRP_FORMULA, 0));
				THROW(ref->PutPropArray(Obj, *pID, SCPPRP_GRPLIST, 0, 0));
			}
			Dirty(*pID);
		}
		else {
			*pID = pPack->Rec.ID;
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			THROW(ref->CheckUniqueSymb(Obj, *pID, pPack->Rec.Symb, offsetof(PPSalCharge, Symb)));
			THROW(ref->AddItem(Obj, pID, &pPack->Rec, 0));
			THROW(ref->PutPropVlrString(Obj, *pID, SCPRP_FORMULA, pPack->Formula));
			THROW(ref->PutPropArray(Obj, *pID, SCPPRP_GRPLIST, &pPack->GrpList, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int  SLAPI PPObjSalCharge::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		PPSalCharge rec;
		if(oneof3(_obj, PPOBJ_AMOUNTTYPE, PPOBJ_STAFFCAL, PPOBJ_OPRKIND)) {
			for(PPID id = 0; ok == DBRPL_OK && EnumItems(&id, &rec) > 0;) {
				if(_obj == PPOBJ_AMOUNTTYPE && rec.AmtID == _id)
					ok = RetRefsExistsErr(Obj, id);
				else if(_obj == PPOBJ_STAFFCAL && rec.CalID == _id)
					ok = RetRefsExistsErr(Obj, id);
				else if(_obj == PPOBJ_OPRKIND && rec.WrOffOpID == _id)
					ok = RetRefsExistsErr(Obj, id);
			}
		}
		else if(_obj == PPOBJ_SALCHARGE) {
			PPIDArray grp_list;
			for(PPID id = 0; ok == DBRPL_OK && EnumItems(&id, &rec) > 0;) {
				ref->GetPropArray(Obj, id, SCPPRP_GRPLIST, &grp_list);
				if(grp_list.lsearch(_id))
					ok = RetRefsExistsErr(Obj, id);
			}
		}
	}
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjSalCharge, PPSalChargePacket);

int SLAPI PPObjSalCharge::SerializePacket(int dir, PPSalChargePacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->Formula, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &pPack->GrpList, rBuf));
	CATCHZOK
	return ok;
}

int SLAPI PPObjSalCharge::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	PPSalChargePacket * p_pack = new PPSalChargePacket;
	p->Data = p_pack;
	THROW_MEM(p_pack);
	if(stream == 0) {
		THROW(GetPacket(id, p_pack) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile((FILE*)stream, 0))
		THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjSalCharge::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data) {
		PPSalChargePacket * p_pack = (PPSalChargePacket *)p->Data;
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				if(SearchBySymb(p_pack->Rec.Symb, &same_id) > 0) {
					; // Не изменяем объект
				}
				if(same_id == 0) {
					p_pack->Rec.ID = 0;
					if(!PutPacket(pID, p_pack, 1)) {
						pCtx->OutputAcceptObjErrMsg(Obj, p_pack->Rec.ID, p_pack->Rec.Name);
						ok = -1;
					}
					else
						ok = 101;
				}
			}
			else {
				; // Не изменяем объект
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile((FILE*)stream, 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjSalCharge::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPSalChargePacket * p_pack = (PPSalChargePacket *)p->Data;
		PPIDArray temp_list;
		THROW(ProcessObjRefInArray(PPOBJ_AMOUNTTYPE, &p_pack->Rec.AmtID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_STAFFCAL,   &p_pack->Rec.CalID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_OPRKIND,    &p_pack->Rec.WrOffOpID, ary, replace));
		for(uint i = 0; i < p_pack->GrpList.getCount(); i++) {
			PPID   temp_id = p_pack->GrpList.get(i);
			THROW(ProcessObjRefInArray(PPOBJ_SALCHARGE, &temp_id, ary, replace));
			if(replace && temp_id)
				temp_list.addUnique(temp_id);
		}
		if(replace)
			p_pack->GrpList = temp_list;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
class SalChargeCache : public ObjCache {
public:
	SLAPI  SalChargeCache() : ObjCache(PPOBJ_SALCHARGE, sizeof(Data))
	{
	}
private:
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		PPID   EnumObjType;
		PPID   EnumExtVal;
		PPID   AmtID;
		PPID   WrOffOpID;
		PPID   CalID;
		long   Flags;
	};
};

int SLAPI SalChargeCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = (Data *)pEntry;
	PPObjSalCharge sl_obj;
	PPSalChargePacket pack;
	if(sl_obj.GetPacket(id, &pack) > 0) {
		#define FLD(f) p_cache_rec->f = pack.Rec.f
		FLD(EnumObjType);
		FLD(EnumExtVal);
		FLD(AmtID);
		FLD(WrOffOpID);
		FLD(CalID);
		FLD(Flags);
		#undef FLD
		MultTextBlock b;
		b.Add(pack.Rec.Name);
		b.Add(pack.Rec.Symb);
		b.Add(pack.Formula);
		PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void SLAPI SalChargeCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPSalChargePacket * p_data_rec = (PPSalChargePacket *)pDataRec;
	const Data * p_cache_rec = (const Data *)pEntry;
	memzero(p_data_rec, sizeof(*p_data_rec));
	#define FLD(f) p_data_rec->Rec.f = p_cache_rec->f
	p_data_rec->Rec.Tag = PPOBJ_SALCHARGE;
	FLD(ID);
	FLD(EnumObjType);
	FLD(EnumExtVal);
	FLD(AmtID);
	FLD(WrOffOpID);
	FLD(CalID);
	FLD(Flags);
	#undef FLD
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Rec.Name, sizeof(p_data_rec->Rec.Name));
	b.Get(p_data_rec->Rec.Symb, sizeof(p_data_rec->Rec.Symb));
	b.Get(p_data_rec->Formula);
}

int SLAPI PPObjSalCharge::Fetch(PPID id, PPSalChargePacket * pRec)
{
	SalChargeCache * p_cache = GetDbLocalCachePtr <SalChargeCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : GetPacket(id, pRec);
}
//
// Implementation of PPALDD_SalCharge
//
PPALDD_CONSTRUCTOR(SalCharge)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	Extra[0].Ptr = new PPObjSalCharge;
}

PPALDD_DESTRUCTOR(SalCharge)
{
	Destroy();
	delete (PPObjSalCharge *)Extra[0].Ptr;
}

int PPALDD_SalCharge::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		PPSalChargePacket pack;
		PPObjSalCharge * p_obj = (PPObjSalCharge *)Extra[0].Ptr;
		if(p_obj->Fetch(rFilt.ID, &pack) > 0) {
			H.ID = pack.Rec.ID;
			STRNSCPY(H.Name, pack.Rec.Name);
			STRNSCPY(H.Symb, pack.Rec.Symb);
			H.AmtID  = pack.Rec.AmtID;
			H.CalID  = pack.Rec.CalID;
			H.Flags  = pack.Rec.Flags;
			H.fGroup = (pack.Rec.Flags & PPSalCharge::fGroup) ? 1 : 0;
			STRNSCPY(H.Formula, pack.Formula);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// coclass(PPObjSalCharge)
//
#define USE_IMPL_DL6ICLS_PPObjSalCharge
#include "..\rsrc\dl600\ppifc_auto.cpp"

DL6_IC_CONSTRUCTION_EXTRA(PPObjSalCharge, DL6ICLS_PPObjSalCharge_VTab, PPObjSalCharge)
//
// Interface IPapyrusObject implementation
//
static void FillSalChargeRec(const PPSalChargePacket * pInner, SPpyO_SalCharge * pOuter)
{
	SString temp_buf;
	pOuter->RecTag = ppoSalCharge;
	#define FLD(f) pOuter->f = pInner->Rec.f
	FLD(ID);
	FLD(AmtID);
	FLD(CalID);
	FLD(Flags);
	#undef FLD
	(temp_buf = pInner->Rec.Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Rec.Symb).CopyToOleStr(&pOuter->Symb);
	(temp_buf = pInner->Formula).CopyToOleStr(&pOuter->Formula);
}

int32 DL6ICLS_PPObjSalCharge::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjSalCharge * p_obj = (PPObjSalCharge *)ExtraPtr;
	if(p_obj) {
		PPSalChargePacket pack;
		ok = p_obj->GetPacket(id, &pack);
		FillSalChargeRec(&pack, (SPpyO_SalCharge *)rec);
	}
	if(!ok)
		AppError = 1;
	return ok;
}

int32 DL6ICLS_PPObjSalCharge::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjSalCharge * p_obj = (PPObjSalCharge *)ExtraPtr;
	if(p_obj) {
		PPSalChargePacket pack;
		PPID   id = 0;
		if(kind == 0)
			ok = p_obj->SearchByName(text, &id, 0);
		else if(kind == 1)
			ok = p_obj->ref->SearchSymb(p_obj->Obj, &id, text, offsetof(PPSalCharge, Symb));
		else
			ok = PPSetErrorInvParam();
		if(ok > 0)
			if(p_obj->GetPacket(id, &pack) > 0)
				FillSalChargeRec(&pack, (SPpyO_SalCharge *)rec);
			else
				ok = -1;
	}
	if(!ok)
		AppError = 1;
	return ok;
}

SString & DL6ICLS_PPObjSalCharge::GetName(int32 id)
{
	PPObjSalCharge * p_obj = (PPObjSalCharge *)ExtraPtr;
	if(p_obj) {
		PPSalChargePacket pack;
		if(p_obj->Fetch(id, &pack) > 0)
			RetStrBuf = pack.Rec.Name;
		else
			ideqvalstr(id, RetStrBuf);
	}
	else {
		RetStrBuf = 0;
		AppError = 1;
	}
	return RetStrBuf;
}

IStrAssocList * DL6ICLS_PPObjSalCharge::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	if(!p)
		AppError = 1;
	return p;
}

int32 DL6ICLS_PPObjSalCharge::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	int    ok = 1;
	PPID   id = 0;
	PPSalChargePacket pack;
	SString temp_buf;
	SPpyO_SalCharge * p_outer_rec = (SPpyO_SalCharge *)pRec;
	PPObjSalCharge  * p_obj = (PPObjSalCharge *)ExtraPtr;
	THROW(p_obj);
	THROW_PP_S(p_outer_rec->RecTag == ppoSalCharge, PPERR_INVSTRUCTAG, "ppoSalCharge");
	pack.Rec.AmtID = p_outer_rec->AmtID;
	pack.Rec.CalID = p_outer_rec->CalID;
	pack.Rec.Flags = p_outer_rec->Flags;
	temp_buf.CopyFromOleStr(p_outer_rec->Name).CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
	temp_buf.CopyFromOleStr(p_outer_rec->Symb).CopyTo(pack.Rec.Symb, sizeof(pack.Rec.Symb));
	pack.Formula.CopyFromOleStr(p_outer_rec->Formula);
	THROW(p_obj->PutPacket(&id, &pack, (flags & 0x0001) ? 0 : 1));
	ASSIGN_PTR(pID, id);
	CATCH
		AppError = 1;
		ok = 0;
	ENDCATCH
	return ok;
}

int32 DL6ICLS_PPObjSalCharge::Update(int32 id, long flags, PPYOBJREC rec)
{
	int    ok = 1;
	PPSalChargePacket pack;
	SString temp_buf;
	SPpyO_SalCharge * p_outer_rec = (SPpyO_SalCharge *)rec;
	PPObjSalCharge  * p_obj = (PPObjSalCharge *)ExtraPtr;
	THROW(p_obj);
	THROW_PP_S(p_outer_rec->RecTag == ppoSalCharge, PPERR_INVSTRUCTAG, "ppoSalCharge");
	THROW(p_obj->GetPacket(id, &pack) > 0);
	pack.Rec.AmtID = p_outer_rec->AmtID;
	pack.Rec.CalID = p_outer_rec->CalID;
	pack.Rec.Flags = p_outer_rec->Flags;
	temp_buf.CopyFromOleStr(p_outer_rec->Name).CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
	temp_buf.CopyFromOleStr(p_outer_rec->Symb).CopyTo(pack.Rec.Symb, sizeof(pack.Rec.Symb));
	pack.Formula.CopyFromOleStr(p_outer_rec->Formula);
	THROW(p_obj->PutPacket(&id, &pack, (flags & 0x0001) ? 0 : 1));
	CATCH
		AppError = 1;
		ok = 0;
	ENDCATCH
	return ok;
}
//
// @ModuleDef(PPObjStaffCal)
//
IMPL_INVARIANT_C(PPStaffCal)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(sizeof(*this) == sizeof(ReferenceTbl::Rec), pInvP);
	S_ASSERT_P(Tag == PPOBJ_STAFFCAL, pInvP);
	S_ASSERT_P(strlen(Name) < sizeof(Name), pInvP);
	S_ASSERT_P(strlen(Symb) < sizeof(Symb), pInvP);
	S_ASSERT_P(LinkObjType  || !LinkObjID, pInvP);
	S_ASSERT_P(!LinkObjType || LinkCalID, pInvP);
	S_ASSERT_P(!(Flags & fInherited) || LinkCalID, pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

//static
int SLAPI PPStaffCalPacket::InvariantEntry(const StaffCalendarTbl::Rec * pRec)
{
	int    ok = 1;
	CALDATE cd;
	cd.v = pRec->DtVal;
	SInvariantParam invp;
	THROW(pRec->CalID >= 0);
	THROW(pRec->ObjID >= 0);
	THROW((pRec->Flags & ~STCALEF_ALLFALGS) == 0);
	THROW(cd.InvariantC(&invp));
	THROW(pRec->TmStart.InvariantC(&invp));
	THROW(pRec->TmEnd.InvariantC(&invp));
	THROW(pRec->TmVal >= 0 && pRec->TmVal <= 24*60*60);
	THROW_PP(!(pRec->Flags & STCALEF_CONTINUOUS) || cd.GetKind() == CALDATE::kDate, PPERR_CALENTRYCONTNDATE);
	CATCHZOK
	return ok;
}

//static
int SLAPI PPStaffCalPacket::SetupEntry(StaffCalendarTbl::Rec * pEntry)
{
	int    ok = -1;
	LTIME  tm = ZEROTIME;
	SInvariantParam invp;
	if(pEntry->TmEnd.InvariantC(&invp)) {
		if(pEntry->TmStart.InvariantC(&invp)) {
			if(pEntry->TmEnd > pEntry->TmStart)
				tm.settotalsec(DiffTime(pEntry->TmEnd, pEntry->TmStart, 3));
		}
		else {
			pEntry->TmStart = ZEROTIME;
			ok = 1;
		}
	}
	else {
		pEntry->TmEnd = ZEROTIME;
		ok = 1;
	}
	if(tm) {
		long   temp = tm.totalsec();
		if(temp != pEntry->TmVal) {
			pEntry->TmVal = temp;
			ok = 1;
		}
	}
	return InvariantEntry(pEntry) ? ok : 0;
}

SLAPI PPStaffCalPacket::PPStaffCalPacket()
{
	MEMSZERO(Rec);
	Rec.Tag = PPOBJ_STAFFCAL;
}

void SLAPI PPStaffCalPacket::Init(const StaffCalFilt * pFilt)
{
	MEMSZERO(Rec);
	Items.freeAll();
	Rec.Tag = PPOBJ_STAFFCAL;
	if(pFilt) {
		Rec.LinkCalID   = pFilt->CalList.GetSingle();
		Rec.LinkObjType = pFilt->LinkObjType;
		Rec.LinkObjID   = pFilt->LinkObjList.GetSingle();
	}
}

PPStaffCalPacket & FASTCALL PPStaffCalPacket::operator = (const PPStaffCalPacket & rSrc)
{
	Rec = rSrc.Rec;
	Items.copy(rSrc.Items);
	return *this;
}

int SLAPI PPStaffCalPacket::Helper_Get(LDATE dt, CALDATE * pCdt, uint * pPos) const
{
	int    ok = 0;
	CALDATE cdt;
	cdt = dt;
	uint   pos = 0;
	const  size_t offs = offsetof(StaffCalendarTbl::Rec, DtVal);
	if(Items.lsearch(&cdt, &pos, CMPF_LONG, offs))
		ok = 1;
	else {
		cdt.SetCalDate(dt.day(), dt.month());
		if(Items.lsearch(&cdt, &(pos = 0), CMPF_LONG, offs))
			ok = 1;
		else {
			cdt.SetDayOfWeek(dayofweek(&dt, 1));
			if(Items.lsearch(&cdt, &(pos = 0), CMPF_LONG, offs))
				ok = 1;
		}
	}
	ASSIGN_PTR(pCdt, cdt);
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int SLAPI PPStaffCalPacket::Get(LDATE dt, double * pHours, uint * pPos) const
{
	int    ok = 0;
	CALDATE cdt;
	double hours = 0.0;
	uint   pos = 0;
	if(Helper_Get(dt, &cdt, &pos) > 0) {
		do {
			const StaffCalendarTbl::Rec & r_item = Items.at(pos);
			if(!(r_item.Flags & STCALEF_SKIP)) {
				LTIME tm;
				tm.v = r_item.TmVal;
				hours += tm.totalsec() / 3600;
				if(ok <= 0) // Присваиваем первую найденную позицию
					ASSIGN_PTR(pPos, pos);
				ok = 1;
			}
			else if(ok == 0)
				ok = -1;
		} while((++pos < Items.getCount()) && Items.at(pos).DtVal == (long)(cdt.v));
	}
	ASSIGN_PTR(pHours, hours);
	return ok;
}

int SLAPI PPStaffCalPacket::GetTimeChunkList(const DateRange & rPeriod, STimeChunkArray * pList) const
{
	int    ok = -1;
	LDATE  dt;
	SString msg_buf;
	THROW_PP_S(rPeriod.low && rPeriod.upp, PPERR_PERIODMUSTBECLOSED, msg_buf.Cat(rPeriod, 0));
	for(dt = rPeriod.low; dt <= rPeriod.upp; dt = plusdate(dt, 1)) {
		CALDATE cdt;
		uint   pos;
		if(Helper_Get(dt, &cdt, &pos) > 0) {
			do {
				const StaffCalendarTbl::Rec & r_item = Items.at(pos);
				if(!(r_item.Flags & STCALEF_SKIP)) {
					STimeChunk chunk;
					chunk.Start.Set(dt, r_item.TmStart);
					chunk.Finish.Set(dt, r_item.TmEnd);
					pList->Add(&chunk, 1);
					ok = 1;
				}
				else if(ok == 0) {
					STimeChunk chunk;
					chunk.Start.Set(dt, r_item.TmStart);
					chunk.Finish.Set(dt, r_item.TmEnd);
					if(pList->Excise(&chunk) > 0)
						ok = 1;
				}
			} while((++pos < Items.getCount()) && Items.at(pos).DtVal == (long)(cdt.v));
		}
	}
	CATCHZOK
	return ok;
}

IMPL_CMPFUNC(STAFFCALREC, i1, i2)
{
	int    r;
	// @v6.3.1 CMPCASCADE4(r, (StaffCalendarTbl::Rec*)i1, (StaffCalendarTbl::Rec*)i2, CalID, Kind, DtVal, TmStart);
	CMPCASCADE3(r, (StaffCalendarTbl::Rec*)i1, (StaffCalendarTbl::Rec*)i2, CalID, DtVal, TmStart); // @v6.3.1
	return r;
}

IMPL_CMPFUNC(STAFFCALREC_WO_TM, i1, i2)
{
	int    r;
	// @v6.3.1 CMPCASCADE3(r, (StaffCalendarTbl::Rec*)i1, (StaffCalendarTbl::Rec*)i2, CalID, Kind, DtVal);
	CMPCASCADE2(r, (StaffCalendarTbl::Rec*)i1, (StaffCalendarTbl::Rec*)i2, CalID, DtVal); // @v6.3.1
	return r;
}


int SLAPI PPStaffCalPacket::SearchContinuousEntry(long dtVal, StaffCalendarTbl::Rec * pRec) const
{
	//
	// Предполагаем, что массив Items отсортирован по критерию CMPFUNC(STAFFCALREC)
	//
	uint   c = Items.getCount();
	if(c)
		do {
			const StaffCalendarTbl::Rec & r_rec = Items.at(--c);
			if(r_rec.DtVal <= dtVal && r_rec.Flags & STCALEF_CONTINUOUS) {
				ASSIGN_PTR(pRec, r_rec);
				return 1;
			}
		} while(c);
	return 0;
}

int SLAPI PPStaffCalPacket::CheckContinuousEntry(const StaffCalendarTbl::Rec * pRec) const
{
	int    ok = 1;
	if(pRec) {
		StaffCalendarTbl::Rec test_rec;
		if(pRec->Flags & STCALEF_CONTINUOUS) {
			if(SearchContinuousEntry(pRec->DtVal, &test_rec)) {
				THROW_PP(pRec->DtVal == test_rec.DtVal && pRec->TmStart == test_rec.TmStart, PPERR_CALINCONTMODE);
			}
			else {
				for(uint i = 0; i < Items.getCount(); i++) {
					const StaffCalendarTbl::Rec & r_rec = Items.at(i);
					if(CMPFUNC(STAFFCALREC, &r_rec, pRec) > 0) {
						CALDATE cd;
						cd.v = r_rec.DtVal;
						THROW_PP(cd.GetKind() != CALDATE::kDate && !(r_rec.Flags & STCALEF_CONTINUOUS), PPERR_CALSETBACKCONT);
					}
				}
			}
		}
		else {
			CALDATE cd;
			cd.v = pRec->DtVal;
			if(cd.GetKind() == CALDATE::kDate) {
				THROW_PP(!SearchContinuousEntry(pRec->DtVal, &test_rec), PPERR_CALINCONTMODE);
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPStaffCalPacket::AddItem(StaffCalendarTbl::Rec * pItem, uint * pPos)
{
	int    ok = 1;
	int    f_day_gap = 0;
	THROW_INVARG(pItem);
	THROW(SetupEntry(pItem));
	// @v8.4.11 {
	if(Rec.LinkCalID) {
		PPObjStaffCal stc_obj;
		PPStaffCal link_rec;
		if(stc_obj.Search(Rec.LinkCalID, &link_rec) > 0)
			f_day_gap = BIN(link_rec.Flags & PPStaffCal::fDayGap);
	}
	else
		f_day_gap = BIN(Rec.Flags & PPStaffCal::fDayGap);
	// } @v8.4.11
	if(f_day_gap) {
		THROW(CheckContinuousEntry(pItem));
		THROW_SL(Items.ordInsert(pItem, pPos, PTR_CMPFUNC(STAFFCALREC)));
	}
	else {
		THROW_PP(Items.lsearch(pItem, 0, PTR_CMPFUNC(STAFFCALREC_WO_TM)) == 0, PPERR_DUPSTAFFCALDATE);
		THROW(CheckContinuousEntry(pItem));
		THROW_SL(Items.ordInsert(pItem, pPos, PTR_CMPFUNC(STAFFCALREC_WO_TM)));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPStaffCalPacket::RemoveItem(uint pos)
{
	int    ok = 1;
	if(pos < Items.getCount())
		Items.atFree(pos);
	else
		ok = -1;
	return ok;
}

//static
int FASTCALL PPObjStaffCal::HasValidTimeRange(const StaffCalendarTbl::Rec & rRec)
{
	return BIN(rRec.TmStart || (rRec.TmEnd && rRec.TmEnd >= rRec.TmStart));
}


TLP_IMPL(PPObjStaffCal, StaffCalendarTbl, P_ScT);

PPObjStaffCal::PPObjStaffCal(void * extraPtr) : PPObjReference(PPOBJ_STAFFCAL, extraPtr)
{
	TLP_OPEN(P_ScT);
	if(ExtraPtr)
		CurrFilt = *(StaffCalFilt *)ExtraPtr;
}

PPObjStaffCal::~PPObjStaffCal()
{
	TLP_CLOSE(P_ScT);
}

int SLAPI PPObjStaffCal::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	if(msg == DBMSG_OBJDELETE)
		if(_obj == PPOBJ_STAFFCAL) {
			PPIDArray child_list;
			if(GetChildList(_id, &child_list) > 0)
				return RetRefsExistsErr(Obj, child_list.at(0));
		}
		else if(oneof3(_obj, PPOBJ_PERSON, PPOBJ_STAFFLIST2, PPOBJ_PERSONPOST)) {
			PPStaffCal rec;
			PPObjID oi;
			if(SearchByObj(0, oi.Set(_obj, _id), &rec) > 0)
				return RetRefsExistsErr(Obj, rec.ID);
		}
	return DBRPL_OK;
}

IMPL_DESTROY_OBJ_PACK(PPObjStaffCal, PPStaffCalPacket);

int SLAPI PPObjStaffCal::SerializePacket(int dir, PPStaffCalPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(P_ScT->SerializeArrayOfRecords(dir, &pPack->Items, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int SLAPI PPObjStaffCal::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	PPStaffCalPacket * p_pack = new PPStaffCalPacket;
	THROW_MEM(p_pack);
	p->Data = p_pack;
	if(stream == 0) {
		THROW(GetPacket(id, p_pack) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile((FILE*)stream, 0))
		THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int  SLAPI PPObjStaffCal::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	PPStaffCalPacket * p_pack = 0;
	THROW(p && p->Data);
	p_pack = (PPStaffCalPacket*)p->Data;
	if(stream == 0) {
		if(*pID == 0) {
			PPID   same_id = 0;
			if(p_pack->Rec.Symb[0] && SearchBySymb(p_pack->Rec.Symb, &same_id, 0) > 0) {
				*pID = same_id;
			}
			else {
				if(!PutPacket(pID, p_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTSTAFFCAL, p_pack->Rec.ID, p_pack->Rec.Name);
					ok = -1;
				}
			}
		}
		else if(!PutPacket(pID, p_pack, 1)) {
			pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTSTAFFCAL, p_pack->Rec.ID, p_pack->Rec.Name);
			ok = -1;
		}
	}
	else {
		SBuffer buffer;
		THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
		THROW_SL(buffer.WriteToFile((FILE*)stream, 0, 0))
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjStaffCal::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPStaffCalPacket * p_pack = (PPStaffCalPacket *)p->Data;
		THROW(ProcessObjRefInArray(PPOBJ_STAFFCAL, &p_pack->Rec.LinkCalID, ary, replace));
		if(p_pack->Rec.LinkObjType)
			THROW(ProcessObjRefInArray(p_pack->Rec.LinkObjType, &p_pack->Rec.LinkObjID, ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjStaffCal::CheckForFilt(const StaffCalFilt * pFilt, const PPStaffCal * pRec) const
{
	if(pFilt) {
		if(pFilt->LinkObjType >= 0 && pRec->LinkObjType != pFilt->LinkObjType)
			return 0;
		if(!pFilt->LinkObjList.CheckID(pRec->LinkObjID))
			return 0;
		if(!pFilt->CalList.CheckID(pRec->LinkCalID))
			return 0;
	}
	return 1;
}

StrAssocArray * SLAPI PPObjStaffCal::MakeStrAssocList(void * extraPtr)
{
	StaffCalFilt filt;
	if(extraPtr)
		filt = *(StaffCalFilt *)extraPtr;
	StrAssocArray * p_list = new StrAssocArray;
	if(p_list) {
		PPStaffCal rec, parent_rec;
		SString name_buf;
		SEnum en;
		if(filt.LinkObjType && filt.LinkObjList.GetSingle())
			en = ref->EnumByIdxVal(Obj, 2, filt.LinkObjList.GetSingle());
		else if(filt.CalList.GetSingle())
			en = ref->EnumByIdxVal(Obj, 1, filt.CalList.GetSingle());
		else
			en = ref->Enum(Obj, 0);
		while(en.Next(&rec) > 0)
			if(CheckForFilt(&filt, &rec)) {
				if(rec.Name[0] == 0)
					if(rec.LinkCalID)
						if(Search(rec.LinkCalID, &parent_rec) > 0)
							name_buf = parent_rec.Name;
						else
							ideqvalstr(rec.LinkCalID, name_buf);
					else
						ideqvalstr(rec.ID, name_buf);
				else
					name_buf = rec.Name;
				p_list->Add(rec.ID, name_buf);
			}
	}
	else
		PPSetErrorNoMem();
	return p_list;
}

#define GRP_COLOR 1

class StaffCalDialog : public PPListDialog {
public:
	StaffCalDialog() : PPListDialog(DLG_STAFFCAL, CTL_STAFFCAL_LIST)
	{
		PPLoadText(PPTXT_WEEKDAYS, WeekDays);
		addGroup(GRP_COLOR, new ColorCtrlGroup(CTL_STAFFCAL_COLOR, CTLSEL_STAFFCAL_COLOR, cmSelColor, CTL_STAFFCAL_SELCOLOR));
	}
	int    setDTS(const PPStaffCalPacket * pData);
	int    getDTS(PPStaffCalPacket * pData);
	void   setupPositionByDate(LDATE);
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int addItem(long *, long *);
	virtual int delItem(long, long);
	virtual int editItem(long , long);
	int    setupSymbols();

	SString WeekDays;
	PPStaffCalPacket Data;
	PPObjStaffCal StcObj;
};

IMPL_HANDLE_EVENT(StaffCalDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_STAFFCAL_PARENT)) {
		getCtrlData(CTLSEL_STAFFCAL_PARENT, &Data.Rec.LinkCalID);
		if(!Data.Rec.LinkCalID)
			Data.Rec.Flags &= PPStaffCal::fInherited;
		DisableClusterItem(CTL_STAFFCAL_FLAGS, 0, !Data.Rec.LinkCalID);
		DisableClusterItem(CTL_STAFFCAL_FLAGS, 1, Data.Rec.LinkCalID);
		DisableClusterItem(CTL_STAFFCAL_FLAGS, 2, Data.Rec.LinkCalID); // @v7.7.12
		setupSymbols();
		clearEvent(event);
	}
}

void StaffCalDialog::setupPositionByDate(LDATE dt)
{
	if(P_Box && dt) {
		uint pos = 0;
		if(Data.Get(dt, 0, &pos) > 0)
			P_Box->focusItem(pos);
	}
}

int StaffCalDialog::setupSymbols()
{
	int    ok = -1;
	PPStaffCal rec;
	if(Data.Rec.LinkCalID && StcObj.Search(Data.Rec.LinkCalID, &rec) > 0) {
		setCtrlData(CTL_STAFFCAL_NAME, rec.Name);
		setCtrlData(CTL_STAFFCAL_SYMB, rec.Symb);
		ok = 1;
	}
	disableCtrls(ok > 0, CTL_STAFFCAL_NAME, CTL_STAFFCAL_SYMB, 0);
	return ok;
}

int StaffCalDialog::setDTS(const PPStaffCalPacket * pData)
{
	if(pData)
		Data = *pData;
	SString temp_buf, obj_name;
	SetupPPObjCombo(this, CTLSEL_STAFFCAL_PARENT, PPOBJ_STAFFCAL, Data.Rec.LinkCalID, 0, 0);
	disableCtrl(CTLSEL_STAFFCAL_PARENT, !(Data.Rec.LinkObjType && Data.Rec.LinkObjID));
	SetupPPObjCombo(this, CTLSEL_STAFFCAL_SUBST, PPOBJ_STAFFCAL, Data.Rec.SubstCalID, 0, 0);
	if(Data.Rec.LinkObjType) {
		GetObjectTitle(Data.Rec.LinkObjType, temp_buf);
		if(Data.Rec.LinkObjID && GetObjectName(Data.Rec.LinkObjType, Data.Rec.LinkObjID, obj_name) > 0)
			temp_buf.CatDiv(':', 1).Cat(obj_name);
	}
	else
		temp_buf = 0;
	setCtrlString(CTL_STAFFCAL_LINKOBJ, temp_buf);
	//
	setCtrlData(CTL_STAFFCAL_ID,  &Data.Rec.ID);
	if(setupSymbols() < 0) {
		setCtrlData(CTL_STAFFCAL_NAME, Data.Rec.Name);
		setCtrlData(CTL_STAFFCAL_SYMB, Data.Rec.Symb);
	}
	AddClusterAssoc(CTL_STAFFCAL_FLAGS, 0, PPStaffCal::fInherited);
	AddClusterAssoc(CTL_STAFFCAL_FLAGS, 1, PPStaffCal::fUseNominalPeriod); // @v6.2.4
	AddClusterAssoc(CTL_STAFFCAL_FLAGS, 2, PPStaffCal::fDayGap); // @v7.7.12
	SetClusterData(CTL_STAFFCAL_FLAGS, Data.Rec.Flags);
	DisableClusterItem(CTL_STAFFCAL_FLAGS, 0, !Data.Rec.LinkCalID);
	DisableClusterItem(CTL_STAFFCAL_FLAGS, 1, Data.Rec.LinkCalID);
	DisableClusterItem(CTL_STAFFCAL_FLAGS, 2, Data.Rec.LinkCalID); // @v7.7.12
	disableCtrl(CTL_STAFFCAL_ID, Data.Rec.ID);
	{
		ColorCtrlGroup::Rec color_rec;
		color_rec.SetupStdColorList();
		color_rec.C = NZOR(Data.Rec.Color, GetColorRef(SClrCoral));
		setGroupData(GRP_COLOR, &color_rec);
	}
	updateList(-1);
    return 1;
}

int StaffCalDialog::getDTS(PPStaffCalPacket * pData)
{
	int    ok = 1;
	SInvariantParam invp;
	getCtrlData(CTL_STAFFCAL_ID, &Data.Rec.ID);
	if(Data.Rec.LinkObjType && Data.Rec.LinkObjID) {
		getCtrlData(CTLSEL_STAFFCAL_PARENT, &Data.Rec.LinkCalID);
		getCtrlData(CTLSEL_STAFFCAL_SUBST,  &Data.Rec.SubstCalID);
	}
	PPStaffCal link_rec;
	if(Data.Rec.LinkCalID && StcObj.Search(Data.Rec.LinkCalID, &link_rec) > 0) {
		memzero(Data.Rec.Name, sizeof(Data.Rec.Name));
		memzero(Data.Rec.Symb, sizeof(Data.Rec.Symb));
	}
	else {
		getCtrlData(CTL_STAFFCAL_NAME, Data.Rec.Name);
		getCtrlData(CTL_STAFFCAL_SYMB, Data.Rec.Symb);
	}
	GetClusterData(CTL_STAFFCAL_FLAGS, &Data.Rec.Flags);
	{
		ColorCtrlGroup::Rec color_rec;
		getGroupData(GRP_COLOR, &color_rec);
		Data.Rec.Color = color_rec.C;
	}
	THROW_PP(Data.Rec.InvariantC(&invp), PPERR_PARENTCALNOTFOUND);
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERR
	return ok;
}

int StaffCalDialog::setupList()
{
	int    ok = 1;
	LDATE  dt;
	SString sub, buf1, id_buf;
	for(uint i = 0; i < Data.GetList().getCount(); i++) {
		StringSet ss(SLBColumnDelim);
		const StaffCalendarTbl::Rec & rrec = Data.GetList().at(i);
		CALDATE cdt;
		cdt.v = rrec.DtVal;
		if(cdt.GetKind() == CALDATE::kDayOfWeek) {
			PPGetSubStr(WeekDays, rrec.DtVal - 1, buf1);
			buf1.Divide(',', id_buf, sub);
		}
		else if(cdt.GetKind() == CALDATE::kCalDate) {
			dt.v = rrec.DtVal;
			(sub = 0).CatLongZ(dt.day(), 2).CatChar('/').CatLongZ(dt.month(), 2);
		}
		else {
			dt.v = rrec.DtVal;
			sub.Z().Cat(dt);
		}
		ss.add(sub);
		LTIME tm;
		tm.settotalsec(rrec.TmVal);
		ss.add(sub.Z().Cat(tm, MKSFMT(0, TIMF_HMS)));
		sub = 0;
		if(rrec.Flags & STCALEF_SKIP)
			sub.CatChar('X');
		ss.add(sub);
		if(!addStringToList(i + 1, ss.getBuf()))
			PPError();
	}
	return ok;
}

class StaffCalDayDialog : public TDialog {
public:
	StaffCalDayDialog(const PPStaffCalPacket * pPack) : TDialog(DLG_STAFFCALD)
	{
		P_Pack = pPack;
		SetupCalDate(CTLCAL_STAFFCALD_DATE, CTL_STAFFCALD_DATE);
		SetupTimePicker(this, CTL_STAFFCALD_TMSTART, CTLTM_STAFFCALD_TMSTART);
		SetupTimePicker(this, CTL_STAFFCALD_TMEND, CTLTM_STAFFCALD_TMEND);
		LockUpdByInput = 0;
	}
	int    setDTS(const StaffCalendarTbl::Rec * pData);
	int    getDTS(StaffCalendarTbl::Rec * pData);
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(TVCOMMAND) {
			if(event.isClusterClk(CTL_STAFFCALD_KIND))
				setupDate();
			// @v9.7.9 {
			else if(event.isClusterClk(CTL_STAFFCALD_FLAGS)) {
				const long preserve_flags = Data.Flags;
				GetClusterData(CTL_STAFFCALD_FLAGS, &Data.Flags);
				if((Data.Flags & STCALEF_SKIP) && !(preserve_flags & STCALEF_SKIP)) {
					LockUpdByInput = 1;
					Data.TmStart = ZEROTIME;
					Data.TmEnd = encodetime(24, 0, 0, 0);
					Data.TmVal = Data.TmEnd.totalsec();
					setCtrlData(CTL_STAFFCALD_TMSTART, &Data.TmStart);
					setCtrlData(CTL_STAFFCALD_TMEND,   &Data.TmEnd);
					setupWorkTime();
					LockUpdByInput = 0;
				}
			}
			// } @v9.7.9 
			else if(cmInputUpdated) {
				if(event.isCtlEvent(CTL_STAFFCALD_TMSTART) || event.isCtlEvent(CTL_STAFFCALD_TMEND)) {
					if(!LockUpdByInput) {
						LockUpdByInput = 1;
						getCtrlData(CTL_STAFFCALD_TMSTART, &Data.TmStart);
						getCtrlData(CTL_STAFFCALD_TMEND, &Data.TmEnd);
						if(PPStaffCalPacket::SetupEntry(&Data) > 0)
							setupWorkTime();
						LockUpdByInput = 0;
					}
				}
				else if(event.isCtlEvent(CTL_STAFFCALD_WORKTIME)) {
					if(!LockUpdByInput) {
						LockUpdByInput = 1;
						LTIME tm;
						getCtrlData(CTL_STAFFCALD_WORKTIME, &tm);
						Data.TmVal = tm.totalsec();
						getCtrlData(CTL_STAFFCALD_TMSTART, &Data.TmStart);
						Data.TmEnd.settotalsec(Data.TmStart.totalsec() + Data.TmVal);
						PPStaffCalPacket::SetupEntry(&Data);
						// На случай, если SetupEntry изменит введенное пользователем значение,
						// возвращаем его на место
						//setCtrlData(CTL_STAFFCALD_WORKTIME, &tm);
						setCtrlData(CTL_STAFFCALD_TMEND, &Data.TmEnd);
						LockUpdByInput = 0;
					}
				}
				else
					return;
			}
			else
				return;
		}
		else
			return;
		clearEvent(event);
	}
	void   setupWorkTime()
	{
		LTIME tm;
		tm.settotalsec(Data.TmVal);
		setCtrlTime(CTL_STAFFCALD_WORKTIME, tm);
	}
	int    setupDate();
	int    PrevKind;
	int    LockUpdByInput; // Блокировка пересчета времени для избежания зацикливания //
	const PPStaffCalPacket * P_Pack;
	StaffCalendarTbl::Rec Data;
};

int StaffCalDayDialog::setupDate()
{
	long   kind = 0;
	long   dw = dayofweek(&Data.DtVal, 1);
	GetClusterData(CTL_STAFFCALD_KIND, &kind);
	if(kind != PrevKind) {
		LDATE dt = ZERODATE;
		setCtrlData(CTL_STAFFCALD_DATE, &dt);
		setCtrlData(CTLSEL_STAFFCALD_DAYOFW, &dt);
	}
	else if(kind == CALDATE::kDate) {
		setCtrlData(CTL_STAFFCALD_DATE, &Data.DtVal);
		setCtrlData(CTLSEL_STAFFCALD_DAYOFW, &dw);
	}
	else if(kind == CALDATE::kCalDate) {
		int    d, m, y;
		decodedate(&d, &m, &y, &Data.DtVal);
		SString temp_buf;
		temp_buf.Cat(d).CatChar('/').Cat(m);
		TInputLine * p_il = (TInputLine *)getCtrlView(CTL_STAFFCALD_DATE);
		CALLPTRMEMB(p_il, setText(temp_buf));
		setCtrlData(CTLSEL_STAFFCALD_DAYOFW, &(dw = 0));
	}
	else if(kind == CALDATE::kDayOfWeek) {
		LDATE dt = ZERODATE;
		setCtrlData(CTL_STAFFCALD_DATE, &dt);
		dw = (dw == -1) ? Data.DtVal : dw;
		setCtrlData(CTLSEL_STAFFCALD_DAYOFW, &dw);
	}
	DisableClusterItem(CTL_STAFFCALD_FLAGS, 1, kind != CALDATE::kDate);
	disableCtrl(CTLSEL_STAFFCALD_DAYOFW, kind != CALDATE::kDayOfWeek);
	disableCtrls(kind == CALDATE::kDayOfWeek, CTL_STAFFCALD_DATE, CTLCAL_STAFFCALD_DATE, 0);
	PrevKind = kind;
	return 1;
}

int StaffCalDayDialog::setDTS(const StaffCalendarTbl::Rec * pData)
{
	if(!RVALUEPTR(Data, pData))
		MEMSZERO(Data);
	if(P_Pack) {
		const PPID cal_id = P_Pack->Rec.ID;
		const PPID main_cal_id = NZOR(P_Pack->Rec.LinkCalID, cal_id);
		SString temp_buf;
		PPObjStaffCal sc_obj;
		PPStaffCal sc_rec;
		if(sc_obj.Fetch(main_cal_id, &sc_rec) > 0) {
			setStaticText(CTL_STAFFCALD_ST_CAL, sc_rec.Name);
		}
		const PPID link_obj_type = P_Pack->Rec.LinkObjType;
		if(link_obj_type) {
			GetObjectTitle(link_obj_type, temp_buf);
			if(P_Pack->Rec.LinkObjID) {
				temp_buf.CatDiv(':', 2);
				GetObjectName(link_obj_type, P_Pack->Rec.LinkObjID, temp_buf, 1);
			}
			setStaticText(CTL_STAFFCALD_ST_OBJ, temp_buf);
		}
	}
	PPStaffCalPacket::SetupEntry(&Data);
	CALDATE cdt;
	cdt.v = Data.DtVal;
	PrevKind = cdt.GetKind();
	AddClusterAssocDef(CTL_STAFFCALD_KIND,  0, CALDATE::kDate);
	AddClusterAssoc(CTL_STAFFCALD_KIND,  1, CALDATE::kCalDate);
	AddClusterAssoc(CTL_STAFFCALD_KIND,  2, CALDATE::kDayOfWeek);
	SetClusterData(CTL_STAFFCALD_KIND,   cdt.GetKind());
	AddClusterAssoc(CTL_STAFFCALD_FLAGS, 0, STCALEF_SKIP);
	AddClusterAssoc(CTL_STAFFCALD_FLAGS, 1, STCALEF_CONTINUOUS);
	SetClusterData(CTL_STAFFCALD_FLAGS, Data.Flags);

	setCtrlData(CTL_STAFFCALD_TMSTART, &Data.TmStart);
	setCtrlData(CTL_STAFFCALD_TMEND,   &Data.TmEnd);
	setupWorkTime();
	SetupStringCombo(this, CTLSEL_STAFFCALD_DAYOFW, PPTXT_WEEKDAYS, 0);
	setupDate();
	return 1;
}

int StaffCalDayDialog::getDTS(StaffCalendarTbl::Rec * pData)
{
	int    ok = 1;
	long   kind = 0;
	uint   sel = 0;
	CALDATE cdt;
	LTIME  tm;
	getCtrlData(CTL_STAFFCALD_TMSTART, &Data.TmStart);
	getCtrlData(CTL_STAFFCALD_TMEND,   &Data.TmEnd);
	// @v9.7.9 {
	if(Data.TmStart.totalsec() < 0 || Data.TmStart.totalsec() > (24*36000))
		Data.TmStart = ZEROTIME;
	if(!Data.TmEnd || Data.TmEnd.totalsec() < 0 || Data.TmEnd.totalsec() > (24*36000))
		Data.TmEnd = encodetime(24, 0, 0, 0);
	// } @v9.7.9 
	getCtrlData(sel = CTL_STAFFCALD_WORKTIME, &tm);
	Data.TmVal = tm.totalsec();
	GetClusterData(CTL_STAFFCALD_KIND, &kind);
	if(kind == CALDATE::kDate) {
		getCtrlData(sel = CTL_STAFFCALD_DATE, &cdt);
		THROW_SL(checkdate(cdt, 0));
	}
	else if(kind == CALDATE::kCalDate) {
		LDATE  dt;
		getCtrlData(sel = CTL_STAFFCALD_DATE, &dt);
		THROW_SL(cdt.SetCalDate(dt.day(), dt.month()));
	}
	else if(kind == CALDATE::kDayOfWeek)
		THROW_PP(cdt.SetDayOfWeek(getCtrlLong(sel = CTL_HOLIDAY_DAYOFWEEK)), PPERR_INVDAYOFWEEK);
	Data.DtVal = cdt;
	Data.Kind  = 0; // @! На этапе отладки это поле - 0. В дальнейшем может понадобиться убрать этот оператор
	GetClusterData(CTL_STAFFCALD_FLAGS, &Data.Flags);
	THROW(PPStaffCalPacket::SetupEntry(&Data));
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

int SLAPI PPObjStaffCal::EditEntry(const PPStaffCalPacket * pPack, StaffCalendarTbl::Rec * pRec)
{
	DIALOG_PROC_BODY_P1(StaffCalDayDialog, pPack, pRec);
}

int StaffCalDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	StaffCalendarTbl::Rec rec;
	MEMSZERO(rec);
	rec.CalID = Data.Rec.ID;
	while(StcObj.EditEntry(&Data, &rec) > 0) {
		uint   new_pos = 0;
		if(Data.AddItem(&rec, &new_pos)) {
			updateList(new_pos);
			MEMSZERO(rec);
			rec.CalID = Data.Rec.ID;
            ok = 1;
		}
		else
			PPError();
	}
	return ok;
}

int StaffCalDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < (long)Data.GetList().getCount()) {
		StaffCalendarTbl::Rec rec, sav_rec;
		MEMSZERO(rec);
		rec = Data.GetList().at(pos);
		sav_rec = rec;
		if(StcObj.EditEntry(&Data, &rec) > 0) {
			uint   new_pos = 0;
			Data.RemoveItem(pos);
			if(Data.AddItem(&rec, &new_pos)) {
				updateList(new_pos);
				ok = 1;
			}
			else
				PPError();
		}
	}
	return ok;
}

int StaffCalDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(id /*&& CONFIRM(PPCFM_DELCALDAY)*/) {
		ok = Data.RemoveItem(pos);
	}
	return ok;
}

int SLAPI PPObjStaffCal::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1, r = cmCancel, valid_data = 0, is_new = 0;
	StaffCalFilt filt;
	if(extraPtr)
		filt = *(StaffCalFilt *)extraPtr;
	PPStaffCalPacket pack;
	StaffCalDialog * p_dlg = 0;
	THROW(CheckDialogPtr(&(p_dlg = new StaffCalDialog)));
	THROW(EditPrereq(pID, p_dlg, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack));
	}
	else {
		pack.Init(&filt);
	}
	p_dlg->setDTS(&pack);
	if(*pID && filt.Period.low)
		p_dlg->setupPositionByDate(filt.Period.low);
	while(!valid_data && (r = ExecView(p_dlg)) == cmOK)
		if(p_dlg->getDTS(&pack) > 0)
			if(PutPacket(pID, &pack, 1))
				ok = valid_data = 1;
			else
				PPError();
	CATCHZOKPPERR
	delete p_dlg;
	return ok ? r : 0;
}

int SLAPI PPObjStaffCal::PutItems(PPID id, PPStaffCalPacket * pPack, int logAction)
{
	int    ok = 1;
	THROW_DB(deleteFrom(P_ScT, 0, P_ScT->CalID == id));
	if(pPack) {
		BExtInsert bei(P_ScT);
		for(uint i = 0; i < pPack->GetList().getCount(); i++) {
			StaffCalendarTbl::Rec & r_rec = pPack->GetList().at(i);
			r_rec.CalID = id;
			THROW_DB(bei.insert(&r_rec));
		}
		THROW_DB(bei.flash());
		if(logAction)
			DS.LogAction(PPACN_OBJUPD, Obj, id, 0, 0);
	}
	else if(logAction)
		DS.LogAction(PPACN_OBJRMV, Obj, id, 0, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPObjStaffCal::PutPacket(PPID * pID, PPStaffCalPacket * pPack, int useTa)
{
	int    ok = 1;
	{
		PPTransaction tra(useTa);
		THROW(tra);
		if(pPack) {
			THROW(ref->CheckUniqueSymb(Obj, *pID, pPack->Rec.Name, offsetof(PPStaffCal, Name)));
			THROW(ref->CheckUniqueSymb(Obj, *pID, pPack->Rec.Symb, offsetof(PPStaffCal, Symb)));
		}
		if(*pID) {
			if(pPack) {
				int    r;
				THROW(r = ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
				THROW(PutItems(*pID, (pPack->Rec.Flags & PPStaffCal::fInherited) ? 0 : pPack, r < 0));
			}
			else {
				THROW(ref->RemoveItem(Obj, *pID, 0));
				THROW(PutItems(*pID, 0, 1));
			}
		}
		else {
			*pID = pPack->Rec.ID;
			THROW(ref->AddItem(Obj, pID, &pPack->Rec, 0));
			THROW(PutItems(*pID, (pPack->Rec.Flags & PPStaffCal::fInherited) ? 0 : pPack, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjStaffCal::GetPacket(PPID id, PPStaffCalPacket * pPack)
{
	int    ok = -1;
	if(id && pPack && Search(id, &pPack->Rec) > 0) {
		PPID   row_id = id;
		StaffCalendarTbl::Key0 k;
		BExtQuery q(P_ScT, 0);
		pPack->Items.freeAll();
		if(pPack->Rec.Flags & PPStaffCal::fInherited)
			if(pPack->Rec.LinkCalID)
				row_id = pPack->Rec.LinkCalID;
			else
				pPack->Rec.Flags &= ~PPStaffCal::fInherited;
		MEMSZERO(k);
		k.CalID = row_id;
		q.selectAll().where(P_ScT->CalID == row_id);
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;)
			pPack->Items.insert(&P_ScT->data);
		ok = 1;
	}
	return ok;
}

int SLAPI PPObjStaffCal::SearchByObj(PPID parentID, PPObjID linkObj, PPStaffCal * pRec)
{
	int    ok = -1;
	PPStaffCal rec;
	for(SEnum  en = ref->EnumByIdxVal(Obj, 2, linkObj.Id); ok < 0 && en.Next(&rec) > 0;)
		if(rec.LinkObjType == linkObj.Obj && (!parentID || rec.LinkCalID == parentID)) {
			ASSIGN_PTR(pRec, rec);
			ok = 1;
		}
	return ok;
}

int SLAPI PPObjStaffCal::CreateChild(PPID * pID, PPID parentID, PPObjID linkObj, int use_ta)
{
	int    ok = 1;
	int    r;
	PPID   id = 0;
	PPStaffCal new_entry;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = SearchByObj(parentID, linkObj, &new_entry));
		if(r > 0) {
			ASSIGN_PTR(pID, new_entry.ID);
			ok = 2;
		}
		else {
			MEMSZERO(new_entry);
			new_entry.LinkObjType = linkObj.Obj;
			new_entry.LinkObjID = linkObj.Id;
			new_entry.LinkCalID = parentID;
			THROW(ref->AddItem(Obj, pID, &new_entry, 0));
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjStaffCal::RemoveEntriesByPsnEvent(PPID psnEvID, int use_ta)
{
	int    ok = 1;
	StaffCalendarTbl::Key2 k2;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		MEMSZERO(k2);
		k2.ObjID = psnEvID;
		if(P_ScT->searchForUpdate(2, &k2, spGe) && k2.ObjID == psnEvID) do {
			if(P_ScT->data.Flags & STCALEF_BYPEVENT) {
				THROW_DB(P_ScT->deleteRec()); // @sfu
			}
		} while(P_ScT->searchForUpdate(2, &k2, spNext) && k2.ObjID == psnEvID);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjStaffCal::SetEntriesByDutySched(PPID baseCalID, PPDutySchedPacket * pDsPack, const DateRange & rPeriod, int use_ta)
{
	int    ok = -1;
	THROW_INVARG(pDsPack);
	if(pDsPack->Rec.ObjType == PPOBJ_PERSON) {
		PPDutySchedPacket::EnumParam ep;
		STimeChunk bounds;
		bounds.Start.Set(rPeriod.low, ZEROTIME);
		bounds.Finish.Set(rPeriod.upp, ZEROTIME);
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(pDsPack->InitIteration(&ep, bounds));
			while(pDsPack->NextIteration(&ep) > 0) {
				StaffCalendarTbl::Rec entry;
				PPID   cal_id = 0;
				CALDATE cdt;
				LTIME   tm;
				PPObjID oi;
				THROW(CreateChild(&cal_id, baseCalID, oi.Set(PPOBJ_PERSON, ep.ObjID), 0) > 0);
				MEMSZERO(entry);
				entry.CalID  = cal_id;
				entry.ObjID  = pDsPack->Rec.ID;
				entry.Flags |= STCALEF_BYDUTYSCHED;
				THROW_SL(cdt.SetDate(ep.Dtm.d));
				entry.DtVal = cdt;
				tm = ep.Dtm.t;
				entry.TmStart = tm;
				entry.TmEnd   = tm.addhs(ep.Duration * 100);
				entry.TmVal   = ep.Duration;
				THROW(SetEntry(entry, 0));
				ok = 1;
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjStaffCal::CheckContinuousEntry(const StaffCalendarTbl::Rec * pRec)
{
	int    ok = 1, r;
	if(pRec) {
		StaffCalendarTbl::Rec test_rec;
		if(pRec->Flags & STCALEF_CONTINUOUS) {
			THROW(r = SearchContinuousEntry(pRec->CalID, pRec->DtVal, &test_rec));
			if(r > 0) {
				THROW_PP(pRec->DtVal == test_rec.DtVal && pRec->TmStart == test_rec.TmStart, PPERR_CALINCONTMODE);
			}
			else {
				StaffCalendarTbl::Key0 k0;
				k0.CalID = pRec->CalID;
				k0.DtVal = pRec->DtVal;
				k0.TmStart = pRec->TmStart+1;
				while(P_ScT->search(0, &k0, spGt) && k0.CalID == pRec->CalID) {
					CALDATE cd;
					cd.v = P_ScT->data.DtVal;
					THROW_PP(cd.GetKind() != CALDATE::kDate && !(P_ScT->data.Flags & STCALEF_CONTINUOUS), PPERR_CALSETBACKCONT);
				}
			}
		}
		else {
			CALDATE cd;
			cd.v = pRec->DtVal;
			if(cd.GetKind() == CALDATE::kDate) {
				THROW(r = SearchContinuousEntry(pRec->CalID, pRec->DtVal, &test_rec));
				THROW_PP(r < 0, PPERR_CALINCONTMODE);
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjStaffCal::SearchContinuousEntry(PPID calID, long dtVal, StaffCalendarTbl::Rec * pRec)
{
	int    ok = -1;
	StaffCalendarTbl::Key0 k0;
	k0.CalID = calID;
	k0.DtVal = dtVal;
	k0.TmStart = ZEROTIME;
	if(P_ScT->search(0, &k0, spLe) && k0.CalID == calID && P_ScT->data.Flags & STCALEF_CONTINUOUS) {
		P_ScT->copyBufTo(pRec);
		ok = 1;
	}
	else
		ok = BTROKORNFOUND ? -1 : PPSetErrorDB();
	return ok;
}

int SLAPI PPObjStaffCal::SearchEntry(PPID calID, long dtVal, LTIME tmStart, StaffCalendarTbl::Rec * pRec)
{
	int    ok = -1;
	StaffCalendarTbl::Key0 k0;
	k0.CalID = calID;
	k0.DtVal = dtVal;
	k0.TmStart = tmStart;
	if(P_ScT->search(0, &k0, spEq)) {
		P_ScT->copyBufTo(pRec);
		ok = 1;
	}
	else
		ok = BTROKORNFOUND ? -1 : PPSetErrorDB();
	return ok;
}

int SLAPI PPObjStaffCal::SearchEntriesByDtVal(PPID calID, long dtVal, TSArray <StaffCalendarTbl::Rec> & rList)
{
	int    ok = -1;
	rList.clear();
	StaffCalendarTbl::Key0 k0;
	k0.CalID = calID;
	k0.DtVal = dtVal;
	k0.TmStart = ZEROTIME;
	if(P_ScT->search(0, &k0, spGe) && P_ScT->data.CalID == calID && P_ScT->data.DtVal == dtVal) {
		do {
			StaffCalendarTbl::Rec rec;
			P_ScT->copyBufTo(&rec);
			THROW_SL(rList.insert(&rec));
			ok = 1;
		} while(P_ScT->search(0, &k0, spNext) && P_ScT->data.CalID == calID && P_ScT->data.DtVal == dtVal);
	}
	THROW_DB(BTROKORNFOUND);
	CATCHZOK
	return ok;
}

int SLAPI PPObjStaffCal::SearchDate(PPID calID, LDATE dt, TSArray <StaffCalendarTbl::Rec> & rList)
{
	int    ok = -1;
	StaffCalendarTbl::Rec rec;
	CALDATE cdt;
	cdt = dt;
	rList.clear();
	if(calID == PPSTCAL_STANDARD) {
		if(checkdate(dt, 0)) {
			MEMSZERO(rec);
			rec.CalID = PPSTCAL_STANDARD;
			rec.DtVal = cdt.v;
			rec.TmStart = ZEROTIME;
			rec.TmEnd = encodetime(24, 0, 0, 0);
			rec.TmVal = rec.TmEnd.totalsec();
			rList.insert(&rec);
			ok = 100;
		}
	}
	else {
		if(SearchEntriesByDtVal(calID, cdt, rList) > 0) {
			assert(rList.getCount());
			ok = 100;
		}
		else if(SearchContinuousEntry(calID, cdt, &rec) > 0) {
			rec.DtVal = cdt.v;
			rec.TmStart = ZEROTIME;
			rec.TmEnd = encodetime(24, 0, 0, 0);
			rec.TmVal = rec.TmEnd.totalsec();
			rList.insert(&rec);
			ok = 100;
		}
		else {
			cdt.SetCalDate(dt.day(), dt.month());
			if(SearchEntriesByDtVal(calID, cdt, rList) > 0) {
				assert(rList.getCount());
				ok = 100;
			}
			else {
				cdt.SetDayOfWeek(dayofweek(&dt, 1));
				if(SearchEntriesByDtVal(calID, cdt, rList) > 0) {
					assert(rList.getCount());
					ok = 100;
				}
			}
		}
	}
	if(ok == 100) {
		ok = 2;
		for(uint i = 0; i < rList.getCount(); i++) {
			const StaffCalendarTbl::Rec & r_rec = rList.at(i);
			if(!(r_rec.Flags & STCALEF_SKIP)) {
				ok = 1;
				break;
			}
		}
	}
	return ok;
}

int SLAPI PPObjStaffCal::RemoveEntry(const StaffCalendarTbl::Rec & rEntry, int use_ta)
{
	int    ok = -1;
	StaffCalendarTbl::Key0 k0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		k0.CalID = rEntry.CalID;
		k0.DtVal = rEntry.DtVal;
		k0.TmStart = rEntry.TmStart;
		if(P_ScT->searchForUpdate(0, &k0, spEq)) {
			THROW_DB(P_ScT->deleteRec()); // @sfu
			ok = 1;
		}
		else {
			THROW_DB(BTROKORNFOUND);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjStaffCal::SetEntry(StaffCalendarTbl::Rec & rEntry, int use_ta)
{
	int    ok = 1;
	int    allow_day_gap = 0;
	int    r;
	PPStaffCal sc_rec, sc_parent_rec;
	StaffCalendarTbl::Rec entry, prev_rec;
 	entry = rEntry;
	THROW(PPStaffCalPacket::SetupEntry(&entry));
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		{
			//
			// Для правильной установки значения необходимо выяснить, допускает ли календарь множественные
			// элементы за день (PPStaffCal::fDayGap)
			//
			THROW(Fetch(rEntry.CalID, &sc_rec) > 0);
			{
				PPIDArray recur_stack;
				PPID   parent_id = sc_rec.LinkCalID;
				if(parent_id) {
					do {
						THROW_PP_S(recur_stack.addUnique(parent_id) > 0, PPERR_STCALRECUR, sc_rec.Name);
						THROW(Fetch(parent_id, &sc_parent_rec) > 0);
						parent_id = sc_parent_rec.LinkCalID;
					} while(parent_id);
				}
				else
					sc_parent_rec = sc_rec;
			}
			allow_day_gap = BIN(sc_parent_rec.Flags & PPStaffCal::fDayGap);
		}
		THROW(CheckContinuousEntry(&entry));
		if(allow_day_gap) {
			THROW(r = SearchEntry(rEntry.CalID, rEntry.DtVal, rEntry.TmStart, &prev_rec));
			if(r > 0) {
				THROW_DB(P_ScT->updateRecBuf(&entry));
			}
			else {
				entry.CalID = rEntry.CalID;
				THROW_DB(P_ScT->insertRecBuf(&entry));
			}
		}
		else {
			int    done = 0;
			TSArray <StaffCalendarTbl::Rec> dlist;
			THROW(r = SearchEntriesByDtVal(rEntry.CalID, rEntry.DtVal, dlist));
			if(r > 0) {
				assert(dlist.getCount()); // Параноидальная проверка правильности работы SearchEntriesByDtVal
				for(uint i = 0; i < dlist.getCount(); i++) {
					const StaffCalendarTbl::Rec & r_ditem = dlist.at(i);
					if(r_ditem.DtVal != entry.DtVal || r_ditem.TmStart != entry.TmStart) { // @v8.2.6 @fix
						THROW(RemoveEntry(r_ditem, 0));
					}
					else {
						StaffCalendarTbl::Key0 k0;
						k0.CalID = entry.CalID;
						k0.DtVal = entry.DtVal;
						k0.TmStart = entry.TmStart;
						//
						// Не может быть в нормальной ситуации, что мы не нашли запись, которую только что видели
						// функцией SearchEntriesByDtVal. По этому, любая ошибка поиска считается критической.
						//
						THROW_DB(P_ScT->searchForUpdate(0, &k0, spEq));
						assert(entry.CalID == rEntry.CalID);
						THROW_DB(P_ScT->updateRecBuf(&entry)); // @sfu
						done = 1;
					}
				}
			}
			else {
				assert(dlist.getCount() == 0); // Параноидальная проверка правильности работы SearchEntriesByDtVal
			}
			if(!done) {
				assert(entry.CalID == rEntry.CalID);
				THROW_DB(P_ScT->insertRecBuf(&entry));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjStaffCal::GetChildList(PPID calID, PPIDArray * pList)
{
	int    ok = -1;
	PPStaffCal rec;
	for(SEnum en = ref->EnumByIdxVal(Obj, 1, calID); en.Next(&rec) > 0;) {
		ok = 1;
		if(pList)
			pList->addUnique(rec.ID);
		else {
			//
			// Если указатель пустой, то просто определяем факт
			// наличия дочерних календарей (ok > 0) и выходим из цикла.
			//
			break;
		}
	}
	return ok;
}

int SLAPI PPObjStaffCal::InitScObjAssoc(PPID calID, PPID prjCalID, const PersonPostTbl::Rec * pPostRec, ScObjAssoc * pAssc)
{
	int    ok = 1;
	PPID   cal_id = calID;        // Может быть замещен подстановочным календарем
	PPID   prj_cal_id = prjCalID; // Может быть замещен подстановочным календарем
	PPObjStaffList sl_obj;
	PPStaffEntry sl_rec;
	memzero(pAssc, sizeof(*pAssc));
	pAssc->List[ScObjAssoc::scPerson   ].Oi.Set(PPOBJ_PERSON,     pPostRec->PersonID);
	pAssc->List[ScObjAssoc::scPost     ].Oi.Set(PPOBJ_PERSONPOST, pPostRec->ID);
	pAssc->List[ScObjAssoc::scStaffList].Oi.Set(PPOBJ_STAFFLIST2, pPostRec->StaffID);
	if(sl_obj.Fetch(pPostRec->StaffID, &sl_rec) > 0)
		pAssc->List[ScObjAssoc::scMainOrg].Oi.Set(PPOBJ_PERSON, sl_rec.OrgID);
	for(uint i = 0; i < ScObjAssoc::scCount; i++) {
		ScObjAssoc::H & r_entry = pAssc->List[i];
		if(i != ScObjAssoc::scHeader) {
			if(r_entry.Oi.Obj && r_entry.Oi.Id) {
				PPStaffCal sc_rec;
				if(SearchByObj(cal_id, r_entry.Oi, &sc_rec) > 0) {
					r_entry.CalID = sc_rec.ID;
					if(sc_rec.SubstCalID)
						cal_id = sc_rec.SubstCalID; // Наследование переходит на подстановочный календарь
				}
				if(prj_cal_id && SearchByObj(prj_cal_id, r_entry.Oi, &sc_rec) > 0) {
					r_entry.ProjCalID = sc_rec.ID;
					if(sc_rec.SubstCalID)
						prj_cal_id = sc_rec.SubstCalID; // Наследование переходит на подстановочный календарь
				}
			}
		}
		else {
			r_entry.CalID = cal_id;
			r_entry.ProjCalID = prj_cal_id;
		}
	}
	return ok;
}

int SLAPI PPObjStaffCal::InitScObjAssoc(PPID calID, PPID prjCalID, PPID personID, ScObjAssoc * pAssc)
{
	int    ok = 1;
	PPID   cal_id = calID;        // Может быть замещен подстановочным календарем
	PPID   prj_cal_id = prjCalID; // Может быть замещен подстановочным календарем
	PPObjStaffList sl_obj;
	memzero(pAssc, sizeof(*pAssc));
	pAssc->List[ScObjAssoc::scPerson].Oi.Set(PPOBJ_PERSON, personID);
	pAssc->List[ScObjAssoc::scHeader].Oi.Set(0, 0);
	for(uint i = 0; i < ScObjAssoc::scCount; i++) {
		ScObjAssoc::H & r_entry = pAssc->List[i];
		if(i != ScObjAssoc::scHeader) {
			if(r_entry.Oi.Obj && r_entry.Oi.Id) {
				PPStaffCal sc_rec;
				if(cal_id && SearchByObj(cal_id, r_entry.Oi, &sc_rec) > 0) {
					r_entry.CalID = sc_rec.ID;
					if(sc_rec.SubstCalID)
						cal_id = sc_rec.SubstCalID; // Наследование переходит на подстановочный календарь
				}
				if(prj_cal_id && SearchByObj(prj_cal_id, r_entry.Oi, &sc_rec) > 0) {
					r_entry.ProjCalID = sc_rec.ID;
					if(sc_rec.SubstCalID)
						prj_cal_id = sc_rec.SubstCalID; // Наследование переходит на подстановочный календарь
				}
			}
		}
		else {
			r_entry.CalID = cal_id;
			r_entry.ProjCalID = prj_cal_id;
		}
	}
	return ok;
}

int SLAPI PPObjStaffCal::Helper_CheckInEntry(LDATE dt, int proj_r, int inverse,
	const TSArray <StaffCalendarTbl::Rec> & rCalList, const TSArray <StaffCalendarTbl::Rec> & rProjCalList,
	STimeChunkArray * pList, long & rDays, double & rHour)
{
	uint   i;
	STimeChunkArray list;

	STimeChunkArray cal_chunk_list;
	STimeChunkArray proj_chunk_list;
	STimeChunkArray skip_chunk_list;
	for(i = 0; i < rCalList.getCount(); i++) {
		const StaffCalendarTbl::Rec & r_rec = rCalList.at(i);
		STimeChunk chunk;
		chunk.Start.d = chunk.Finish.d = dt;
		if(HasValidTimeRange(r_rec)) {
			chunk.Start.t  = r_rec.TmStart;
			chunk.Finish.t = r_rec.TmEnd;
		}
		else {
			const long tmval = r_rec.TmVal;
			chunk.Finish.t.settotalsec((tmval > 0 && tmval < (24*60*60)) ? tmval : (24*60*60)-1);
		}
		if(r_rec.Flags & STCALEF_SKIP)
			skip_chunk_list.Add(&chunk, 0);
		else
			cal_chunk_list.Add(&chunk, 0);
	}
	// @v8.4.11 {
	for(i = 0; i < skip_chunk_list.getCount(); i++) {
		if(cal_chunk_list.getCount() == 0) {
			STimeChunk chunk;
			chunk.Start.d = chunk.Finish.d = dt;
			chunk.Start.t = ZEROTIME;
			chunk.Finish.t = encodetime(24, 0, 0, 0);
			cal_chunk_list.insert(&chunk);
		}
		cal_chunk_list.Excise((const STimeChunk *)skip_chunk_list.at(i));
	}
	skip_chunk_list.clear();
	// } @v8.4.11
	for(uint j = 0; j < rProjCalList.getCount(); j++) {
		const StaffCalendarTbl::Rec & r_proj_rec = rProjCalList.at(j);
		STimeChunk chunk;
		chunk.Start.d = chunk.Finish.d = dt;
		if(HasValidTimeRange(r_proj_rec)) {
			chunk.Start.t  = r_proj_rec.TmStart;
			chunk.Finish.t = r_proj_rec.TmEnd;
		}
		else
			chunk.Finish.t.settotalsec(r_proj_rec.TmVal);
		if(r_proj_rec.Flags & STCALEF_SKIP)
			skip_chunk_list.Add(&chunk, 0);
		else
			proj_chunk_list.Add(&chunk, 0);
	}
	// @v8.4.11 {
	for(i = 0; i < skip_chunk_list.getCount(); i++) {
		proj_chunk_list.Excise((const STimeChunk *)skip_chunk_list.at(i));
	}
	skip_chunk_list.clear();
	// } @v8.4.11
	if(inverse && proj_r > 0) { // @v8.4.11 (proj_r==1)-->(proj_r>0)
		STimeChunkArray inverse_chunk_list;
		proj_chunk_list.Sort();
		proj_chunk_list.GetFreeList(&inverse_chunk_list);
		const uint ic = inverse_chunk_list.getCount();
		if(ic) {
			STimeChunk * p_first = (STimeChunk *)inverse_chunk_list.at(0);
			if(!p_first->Start) {
				p_first->Start.Set(dt, ZEROTIME);
			}
			STimeChunk * p_last = (STimeChunk *)inverse_chunk_list.at(ic-1);
			if(p_last->Finish.IsFar()) {
				p_last->Finish.Set(dt, encodetime(23, 59, 59, 99));
			}
		}
		proj_chunk_list = inverse_chunk_list;
	}
	{
		if(proj_chunk_list.getCount()) {
			proj_chunk_list.Intersect(&cal_chunk_list, &list);
		}
		else
			list = cal_chunk_list;
	}
	if(list.getCount()) {
		long   sec = 0;
		for(uint k = 0; k < list.getCount(); k++) {
			const STimeChunk & r_chunk = *(const STimeChunk *)list.at(k);
			sec += r_chunk.GetDuration();
			if(pList)
				pList->Add(&r_chunk, 0);
		}
		rDays += 1;
		rHour += ((double)sec) / 3600.0;
	}
	return 1;
}

int SLAPI PPObjStaffCal::CalcPeriodByPersonEvent(const ScObjAssoc & rAssc, const PPIDArray & rEvList, int inverse,
	long * pDays, double * pHours, STimeChunkArray * pList)
{
	int    ok = 1;
	long   numdays = 0;
	double numhours = 0.0;
	TSArray <StaffCalendarTbl::Rec> cal_list;
	TSArray <StaffCalendarTbl::Rec> proj_cal_list;
	for(uint i = 0; i < rEvList.getCount(); i++) {
		const PPID ev_id = rEvList.get(i);
		StaffCalendarTbl::Key2 k2;
		MEMSZERO(k2);
		k2.ObjID = ev_id;
		BExtQuery q(P_ScT, 2);
		q.selectAll().where(P_ScT->ObjID == ev_id);
		for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
			if(P_ScT->data.Flags & STCALEF_BYPEVENT /* @v8.4.11 && !(P_ScT->data.Flags & STCALEF_SKIP) */) {
				CALDATE cd;
				cd.v = P_ScT->data.DtVal;
				if(cd.GetKind() == CALDATE::kDate) {
					const LDATE dt = (LDATE)cd;
					const PPID  cal_id = P_ScT->data.CalID;
					int    proj_r = -1;
					cal_list.clear();
					proj_cal_list.clear();
					THROW_SL(cal_list.insert(&P_ScT->data));
					if(!rAssc.List[ScObjAssoc::scHeader].ProjCalID)
						proj_r = 0;
					else {
						for(i = 0; proj_r < 0 && i < ScObjAssoc::scCount; i++) {
							const PPID proj_cal_id = rAssc.List[i].ProjCalID;
							if(proj_cal_id)
								THROW(proj_r = SearchDate(proj_cal_id, dt, proj_cal_list));
						}
					}
					Helper_CheckInEntry(dt, proj_r, inverse, cal_list, proj_cal_list, pList, numdays, numhours);
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pDays, numdays);
	ASSIGN_PTR(pHours, numhours);
	return ok;
}

int SLAPI PPObjStaffCal::CalcPeriod(const ScObjAssoc & rAssc, const DateRange & rPeriod, int inverse,
	long * pDays, double * pHours, STimeChunkArray * pList)
{
	int    ok = 1;
	long   numdays = 0;
	double numhours = 0.0;
	TSArray <StaffCalendarTbl::Rec> cal_list;
	TSArray <StaffCalendarTbl::Rec> proj_cal_list;
	for(LDATE dt = rPeriod.low; dt <= rPeriod.upp; dt = plusdate(dt, 1)) {
		int    r = -1, proj_r = -1;
		uint   i;
		proj_cal_list.clear();
		if(!rAssc.List[ScObjAssoc::scHeader].ProjCalID)
			proj_r = 0;
		else {
			for(i = 0; proj_r < 0 && i < ScObjAssoc::scCount; i++) {
				const PPID cal_id = rAssc.List[i].ProjCalID;
				if(cal_id)
					THROW(proj_r = SearchDate(cal_id, dt, proj_cal_list));
			}
		}
		for(i = 0; r < 0 && i < ScObjAssoc::scCount; i++) {
			const PPID cal_id = rAssc.List[i].CalID;
			if(cal_id) {
				cal_list.clear();
				THROW(r = SearchDate(cal_id, dt, cal_list));
				if(r > 0) { // @v8.4.11 (r==1)-->(r>0)
					Helper_CheckInEntry(dt, proj_r, inverse, cal_list, proj_cal_list, pList, numdays, numhours);
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pDays, numdays);
	ASSIGN_PTR(pHours, numhours);
	return ok;
}
//
//
//
int SLAPI PPObjStaffCal::Browse(void * extraPtr)
{
	class StaffCalView : public ObjViewDialog {
	public:
		StaffCalView(PPObjStaffCal * _ppobj, void * extraPtr) : ObjViewDialog(DLG_STAFFCALVIEW, _ppobj, extraPtr)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			ObjViewDialog::handleEvent(event);
			if(event.isCmd(cmTransmitCharry)) {
				PPIDArray id_list;
				PPStaffCal rec;
				for(PPID id = 0; ((PPObjReference *)P_Obj)->EnumItems(&id, &rec) > 0;) {
					if(rec.LinkCalID == 0)
						id_list.add(rec.ID);
				}
				if(!SendCharryObject(PPDS_CRRSTAFFCAL, id_list))
					PPError();
				clearEvent(event);
			}
		}
	};
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		StaffCalFilt filt;
		if(extraPtr)
			filt = *(StaffCalFilt *)extraPtr;
		TDialog * dlg = new StaffCalView(this, &filt);
		if(CheckDialogPtrErr(&dlg)) {
			SString temp_buf, obj_name;
			if(filt.LinkObjType) {
				GetObjectTitle(filt.LinkObjType, temp_buf);
				const PPID obj_id = filt.LinkObjList.GetSingle();
				if(obj_id && GetObjectName(filt.LinkObjType, obj_id, obj_name) > 0)
					temp_buf.CatDiv('-', 1).Cat(obj_name);
			}
			dlg->setStaticText(CTL_STAFFCALVIEW_ST_TTL, temp_buf);
			ExecViewAndDestroy(dlg);
		}
	}
	else
		ok = PPErrorZ();
	return ok;
}
//
//
//
class StaffCalCache : public ObjCache {
public:
	SLAPI  StaffCalCache() : ObjCache(PPOBJ_STAFFCAL, sizeof(Data))
	{
	}
private:
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		PPID   LinkObjType;
		PPID   LinkCalID;
		PPID   LinkObjID;
		PPID   SubstCalID;
		long   Flags;
		long   Color;
	};
};

int SLAPI StaffCalCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = (Data *)pEntry;
	PPObjStaffCal sc_obj;
	PPStaffCal rec;
	if(sc_obj.Search(id, &rec) > 0) {
#define FLD(f) p_cache_rec->f = rec.f
		FLD(LinkObjType);
		FLD(LinkCalID);
		FLD(LinkObjID);
		FLD(SubstCalID);
		FLD(Flags);
		FLD(Color);
#undef FLD
		StringSet ss("/&");
		ss.add(rec.Name, 0);
		ss.add(rec.Symb, 0);
		PutName(ss.getBuf(), p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void SLAPI StaffCalCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPStaffCal * p_data_rec = (PPStaffCal *)pDataRec;
	const Data * p_cache_rec = (const Data *)pEntry;
	memzero(p_data_rec, sizeof(*p_data_rec));
#define FLD(f) p_data_rec->f = p_cache_rec->f
	p_data_rec->Tag = PPOBJ_STAFFCAL;
	FLD(ID);
	FLD(LinkObjType);
	FLD(LinkCalID);
	FLD(LinkObjID);
	FLD(SubstCalID);
	FLD(Flags);
	FLD(Color);
#undef FLD
	char   temp_buf[2048];
	GetName(pEntry, temp_buf, sizeof(temp_buf));
	StringSet ss("/&");
	ss.setBuf(temp_buf, strlen(temp_buf)+1);
	uint   p = 0;
	ss.get(&p, p_data_rec->Name, sizeof(p_data_rec->Name));
	ss.get(&p, p_data_rec->Symb, sizeof(p_data_rec->Symb));
}

IMPL_OBJ_FETCH(PPObjStaffCal, PPStaffCal, StaffCalCache);
//
// Implementation of PPALDD_StaffCal
//
PPALDD_CONSTRUCTOR(StaffCal)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
}

PPALDD_DESTRUCTOR(StaffCal)
{
	Destroy();
}

int PPALDD_StaffCal::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		PPStaffCal rec;
		if(SearchObject(PPOBJ_STAFFCAL, rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Symb, rec.Symb);
			H.LinkObjType = rec.LinkObjType;
			H.Flags       = rec.Flags;
			H.fInherited  = BIN(rec.Flags & PPStaffCal::fInherited);
			H.LinkCalID   = rec.LinkCalID;
			H.LinkObjID   = rec.LinkObjID;
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

