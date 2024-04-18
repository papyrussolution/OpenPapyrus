// OBJREF.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>

static void FASTCALL set_rt_flag(PPObject * pObj, int * pRt, int flag)
{
	if(pObj->CheckRights(flag))
		(*pRt) |= flag;
}

ObjViewDialog::ObjViewDialog(uint dlgRez, PPObject * aObj, void * extraPtr) : TDialog(/*DLG_OBJVIEW*/dlgRez), P_Obj(aObj), ExtraPtr(extraPtr), Rt(0)
{
	set_rt_flag(P_Obj, &Rt, PPR_INS);
	set_rt_flag(P_Obj, &Rt, PPR_MOD);
	set_rt_flag(P_Obj, &Rt, PPR_DEL);
	P_List = static_cast<SmartListBox *>(getCtrlView(CTL_OBJVIEW_LIST));
	if(ViewSize.x && ViewSize.y && P_List) {
		SString obj_title;
		setTitle(GetObjectTitle(P_Obj->Obj, obj_title));
		// @v11.3.2 @obsolete P_List->ViewOptions |= ofFramed;
		P_List->setDef(P_Obj->Selector(0, 0, ExtraPtr));
		P_List->Draw_();
		enableCommand(cmaInsert, Rt & PPR_INS);
		enableCommand(cmaEdit,   Rt & PPR_MOD);
		enableCommand(cmaDelete, Rt & PPR_DEL);
	}
}

void ObjViewDialog::updateList(PPID id)
{
	if(SmartListBox::IsValidS(P_List)) {
		const long cur = (id < 0) ? P_List->P_Def->_curItem() : -1;
		P_List->setDef(P_Obj->Selector(0, 0, ExtraPtr));
		if(id < 0)
			P_List->focusItem(cur);
		else if(id > 0)
			P_List->TransmitData(+1, &id);
		P_List->Draw_();
	}
}

PPID ObjViewDialog::getCurrID()
{
	PPID   id = 0;
	return (P_List && P_List->getCurID(&id)) ? id : 0;
}

void ObjViewDialog::addItem()
{
	PPID   id = 0;
	if((Rt & PPR_INS) && P_List && P_Obj->Edit(&id, ExtraPtr) == cmOK)
		updateList(id);
}

void ObjViewDialog::editItem()
{
	MemLeakTracer mlt;
	PPID   id = getCurrID();
	if((Rt & PPR_MOD) && id && P_Obj->Edit(&id, ExtraPtr) == cmOK)
		updateList(-1);
}

void ObjViewDialog::removeItem()
{
	PPID   id = getCurrID();
	if((Rt & PPR_DEL) && id) {
		P_List->P_Def->step(+1);
		PPID   next_id = getCurrID();
		if(next_id == 0 || next_id == id) {
			P_List->P_Def->step(-2);
			next_id = getCurrID();
		}
		if(P_Obj->RemoveObjV(id, 0, PPObject::rmv_default, ExtraPtr) > 0) {
			updateList(next_id);
		}
	}
}

void   ObjViewDialog::extraProc(long) {}
int    ObjViewDialog::Print() { return -1; }

int ObjViewDialog::transmit(PPID)
{
	int    ok = -1;
	if(P_Obj && IS_REF_OBJTYPE(P_Obj->Obj)) {
		ObjTransmitParam param;
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			PPID   id = 0;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			PPWaitStart();
			for(id = 0; static_cast<PPObjReference *>(P_Obj)->EnumItems(&id, 0) > 0;)
				objid_ary.Add(P_Obj->Obj, id);
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

int ObjViewDialog::RemoveAll()
{
	int    ok = -1;
	if(P_Obj && IS_DYN_OBJTYPE(P_Obj->Obj)) {
		if(P_Obj->CheckRights(0, DYNOBJRT_RMVALL)) {
			if(CONFIRMCRIT(PPCFM_RMVALLREFITEMS)) {
				PPID   id = 0;
				PPIDArray id_list;
				PPWaitStart();
				for(id = 0; static_cast<PPObjReference *>(P_Obj)->EnumItems(&id, 0) > 0;)
					id_list.addnz(id);
				if(id_list.getCount()) {
					PPLogger logger;
					PPTransaction tra(1);
					THROW(tra);
					for(uint i = 0; i < id_list.getCount(); i++) {
						id = id_list.get(i);
						if(!P_Obj->RemoveObjV(id, 0, PPObject::no_wait_indicator|PPObject::not_checkrights, ExtraPtr)) {
							logger.LogLastError();
						}
					}
					THROW(tra.Commit());
					updateList(-1);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

IMPL_HANDLE_EVENT(ObjViewDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND)
		switch(TVCMD) {
			case cmLBDblClk:
				if(TVINFOVIEW && TVINFOVIEW == P_List) {
					int edit = 1;
					if(P_List->IsTreeList()) {
						PPID cur_id = 0;
						P_List->P_Def->getCurID(&cur_id);
						if(static_cast<const StdTreeListBoxDef *>(P_List->P_Def)->HasChildren(cur_id))
							edit = 0;
					}
					if(edit)
						editItem();
					else
						return;
				}
				break;
			case cmaInsert:    addItem();      break;
			case cmaEdit:      editItem();     break;
			case cmaDelete:    removeItem();   break;
			case cmaMore:      extraProc(getCurrID()); break;
			case cmPrint:      Print();        break;
			case cmTransmit:   transmit(getCurrID());  break;
			case cmSysJournalByObj:
				if(getCurrID())
					ViewSysJournal(P_Obj->Obj, getCurrID(), 0);
				break;
			default: return;
		}
	else if(TVKEYDOWN) {
		switch(TVKEY) {
			case kbF11:
			case kbCtrlEnter: extraProc(getCurrID()); break;
			case kbCtrlF8: RemoveAll(); break;
			default: return;
		}
	}
	else
		return;
	clearEvent(event);
}

RefObjViewDialog::RefObjViewDialog(uint dlgRez, PPObject * aObj, long charryID, void * extraPtr) : ObjViewDialog(dlgRez, aObj, extraPtr), CharryID(charryID)
{
}

IMPL_HANDLE_EVENT(RefObjViewDialog)
{
	ObjViewDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(TVCMD == cmTransmitCharry || TVCMD == cmTransmit) {
			PPIDArray id_list;
			ReferenceTbl::Rec rec;
			for(PPID id = 0; static_cast<PPObjReference *>(P_Obj)->EnumItems(&id, &rec) > 0;)
				id_list.add(rec.ObjID);
			if(id_list.getCount()) {
				if(TVCMD == cmTransmitCharry) {
					if(!SendCharryObject(CharryID, id_list))
						PPError();
				}
				else {
					ObjTransmitParam param;
					if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
						const PPIDArray & rary = param.DestDBDivList.Get();
						PPObjIDArray objid_ary;
						PPWaitStart();
						if(!objid_ary.Add(P_Obj->Obj, id_list) || (!PPObjectTransmit::Transmit(&rary, &objid_ary, &param))) {
							PPWaitStop();
							PPError();
						}
						PPWaitStop();
					}
				}
			}
			clearEvent(event);
		}
	}
}

int RefObjView(PPObject * pObj, long charryID, void * extraPtr)
{
	int    ok = 1;
	TDialog * dlg = new RefObjViewDialog(DLG_REFOBJVIEW, pObj, charryID, extraPtr);
	if(CheckDialogPtrErr(&dlg))
		ExecViewAndDestroy(dlg);
	else
		ok = 0;
	return ok;
}
//
//
//
int SimpleObjView(PPObject * pObj, void * extraPtr)
{
	int    ok = 1;
	TDialog * dlg = new ObjViewDialog(((pObj->GetImplementFlags() & PPObject::implTreeSelector) ?
		DLG_OBJVIEWT : DLG_OBJVIEW), pObj, extraPtr);
	if(CheckDialogPtrErr(&dlg))
		ExecViewAndDestroy(dlg);
	else
		ok = 0;
	return ok;
}
//
// @ModuleDef(PPObjReference)
//
PPObjReference::PPObjReference(PPID aObj, void * extraPtr) : PPObject(aObj), ExtraPtr(extraPtr), P_Ref(PPRef)
{
	ImplementFlags |= implStrAssocMakeList;
	if(IS_DYN_OBJTYPE(aObj)) {
		ReferenceTbl::Rec rec;
		if(P_Ref->GetItem(PPOBJ_DYNAMICOBJS, aObj, &rec) > 0 && rec.Val1 & PPCommObjEntry::fHierarchical)
			ImplementFlags |= implTreeSelector;
	}
}

int    PPObjReference::Search(PPID id, void * b) { return P_Ref->GetItem(Obj, id, b); }
const  char * PPObjReference::GetNamePtr() { return P_Ref->data.ObjName; }
int    PPObjReference::SearchByName(const char * pName, PPID * pID, void * pRec) { return P_Ref->SearchName(Obj, pID, pName, pRec); }

int PPObjReference::SearchBySymb(const char * pSymb, PPID * pID, void * pRec)
{
	PPID   id = 0;
	int    r = P_Ref->SearchSymb(Obj, &id, pSymb, offsetof(ReferenceTbl::Rec, Symb));
	if(r > 0) {
		if(pRec)
			r = P_Ref->GetItem(Obj, id, pRec);
	}
	else {
		if(Obj == PPOBJ_ACCSHEET) {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			(r_temp_buf = pSymb).Strip();
			if(r_temp_buf.IsEqiAscii("$buyer") || r_temp_buf.IsEqiAscii("$client"))
				id = GetSellAccSheet();
			else if(r_temp_buf.IsEqiAscii("$suppl") || r_temp_buf.IsEqiAscii("$supplier"))
				id = GetSupplAccSheet();
			else if(r_temp_buf.IsEqiAscii("$agent"))
				id = GetAgentAccSheet();
			if(id && P_Ref->GetItem(Obj, id, pRec) > 0)
				r = 1;
			else {
				id = 0;
				PPSetError(PPERR_ACCSHEETSYMBNFOUND, pSymb);
			}
		}
		else if(Obj == PPOBJ_PERSONKIND) {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			(r_temp_buf = pSymb).Strip();
			if(r_temp_buf.IsEqiAscii("$buyer") || r_temp_buf.IsEqiAscii("$client"))
				id = PPPRK_CLIENT;
			else if(r_temp_buf.IsEqiAscii("$suppl") || r_temp_buf.IsEqiAscii("$supplier"))
				id = PPPRK_SUPPL;
			else if(r_temp_buf.IsEqiAscii("$agent"))
				id = PPPRK_AGENT;
			if(id && P_Ref->GetItem(Obj, id, pRec) > 0)
				r = 1;
			else {
				id = 0;
				PPSetError(PPERR_PERSONKINDSYMBNFOUND, pSymb);
			}
		}
		else if(Obj == PPOBJ_BIZSCORE)
			PPSetError(PPERR_BIZSCORESYMBNFOUND, pSymb);
		else if(Obj == PPOBJ_STAFFCAL)
			PPSetError(PPERR_STAFFCALSYMBNFOUND, pSymb);
	}
	ASSIGN_PTR(pID, id);
	return r;
}

int PPObjReference::EnumItems(PPID * pID, void * b) { return P_Ref->EnumItems(Obj, pID, b); }
SEnum::Imp * FASTCALL PPObjReference::Enum(int options) { return P_Ref->Enum(Obj, options); }
SEnum::Imp * PPObjReference::EnumByIdxVal(int valN, long val) { return P_Ref->EnumByIdxVal(Obj, valN, val); }

int PPObjReference::CheckDupName(PPID objID, const char * pName)
{
	PPID   temp_id = 0;
	return (SearchByName(pName, &temp_id) > 0 && objID != temp_id) ? PPSetError(PPERR_DUPOBJNAME) : 1;
}

int PPObjReference::CheckDupSymb(PPID objID, const char * pSymb)
{
	return P_Ref->CheckUniqueSymb(Obj, objID, pSymb, offsetof(ReferenceTbl::Rec, Symb));
}

PPID PPObjReference::GetSingle()
{
	PPID   id = 0, k = 0;
	long   count = 0;
	while(EnumItems(&k) > 0) {
		id = k;
		if(++count > 1)
			return 0;
	}
	return (count == 1) ? id : 0;
}

int PPObjReference::AddItem(PPID * pID, void * pBuf, int use_ta)
{
	int    ok = -1;
	if((ok = P_Ref->AddItem(Obj, &static_cast<ReferenceTbl::Rec *>(pBuf)->ObjID, pBuf, 0)) > 0)
		ASSIGN_PTR(pID, static_cast<ReferenceTbl::Rec *>(pBuf)->ObjID);
	return ok;
}

int PPObjReference::UpdateItem(PPID id, const void * pRec, int use_ta)
{
	return P_Ref->UpdateItem(Obj, id, pRec, 1, use_ta);
}

int PPObjReference::Browse(void * extraPtr)
{
	return CheckRights(PPR_READ) ? SimpleObjView(this, extraPtr) : PPErrorZ();
}

int PPObjReference::StoreItem(PPID obj, PPID id, void * pRec, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(id) {
			int r = P_Ref->UpdateItem(obj, id, pRec, 1, 0);
			THROW(r);
			if(r < 0)
				ok = -1;
		}
		else {
			THROW(P_Ref->AddItem(obj, &static_cast<ReferenceTbl::Rec *>(pRec)->ObjID, pRec, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

PPCommObjEntry::PPCommObjEntry(PPID objType, PPID id, const char * pName, const char * pSymb, long flags) :
	ObjType(objType), ID(id), Flags(flags), ParentID(0)
{
	STRNSCPY(Name, pName);
	STRNSCPY(Symb, pSymb);
}

/*static*/int PPObjReference::EditCommObjItem(PPCommObjEntry * pEntry)
{
	int    r = -1;
	int    valid_data = 0;
	TDialog * p_dlg = 0;
	PPObjReference * p_obj = 0;
	if(pEntry) {
		uint   dlg_id = DLG_COMMOBJ;
		if(IS_DYN_OBJTYPE(pEntry->ObjType))
			dlg_id = DLG_DYNOBJ;
		else if(pEntry->ObjType == PPOBJ_DYNAMICOBJS)
			dlg_id = DLG_DYNOBJHDR;
		else
			dlg_id = DLG_COMMOBJ;
		PPID   _id = pEntry->ID;
		SString obj_title;
		SString name(pEntry->Name);
		SString symb(pEntry->Symb);
		// @v10.2.4 @fix p_dlg = new TDialog(DLG_COMMOBJ);
		THROW(CheckDialogPtr(&(p_dlg = new TDialog(dlg_id))));
		THROW(p_obj = static_cast<PPObjReference *>(GetPPObject(pEntry->ObjType, 0)));
		THROW(p_obj->EditPrereq(&pEntry->ID, p_dlg, 0));
		p_dlg->setTitle(GetObjectTitle(pEntry->ObjType, obj_title));
		p_dlg->setCtrlLong(CTL_COMMOBJ_ID, _id);
		p_dlg->setCtrlString(CTL_COMMOBJ_NAME, name);
		p_dlg->setCtrlString(CTL_COMMOBJ_SYMB, symb);
		p_dlg->AddClusterAssoc(CTL_COMMOBJ_FLAGS, 0, PPCommObjEntry::fPassive);
		if(pEntry->ObjType == PPOBJ_DYNAMICOBJS) {
			p_dlg->AddClusterAssoc(CTL_COMMOBJ_FLAGS, 1, PPCommObjEntry::fHierarchical);
		}
		p_dlg->SetClusterData(CTL_COMMOBJ_FLAGS, pEntry->Flags);
		if(p_dlg->getCtrlView(CTLSEL_COMMOBJ_PARENT)) {
			SetupPPObjCombo(p_dlg, CTLSEL_COMMOBJ_PARENT, pEntry->ObjType, pEntry->ParentID, OLW_CANINSERT|OLW_CANSELUPLEVEL, 0);
		}
		p_dlg->disableCtrl(CTL_COMMOBJ_ID, (!PPMaster || _id));
		while(!valid_data && ExecView(p_dlg) == cmOK) {
			long flags = 0L;
			p_dlg->getCtrlData(CTL_COMMOBJ_ID, &_id);
			p_dlg->getCtrlString(CTL_COMMOBJ_NAME, name);
			p_dlg->getCtrlString(CTL_COMMOBJ_SYMB, symb);
			p_dlg->GetClusterData(CTL_COMMOBJ_FLAGS, &flags);
			if(p_obj && !p_obj->CheckName(_id, name, 1))
				PPErrorByDialog(p_dlg, CTL_COMMOBJ_NAME);
			else if(p_obj && !p_obj->P_Ref->CheckUniqueSymb(pEntry->ObjType, _id, symb, offsetof(ReferenceTbl::Rec, Symb))) {
				PPErrorByDialog(p_dlg, CTL_COMMOBJ_SYMB);
			}
			else if(pEntry->ObjType == PPOBJ_DYNAMICOBJS && GetObjectTypeBySymb(symb, 0)) {
				PPSetError(PPERR_ILLEGALDYNOBJSYMB, symb);
				PPErrorByDialog(p_dlg, CTL_COMMOBJ_SYMB);
			}
			else {
				if(p_dlg->getCtrlView(CTLSEL_COMMOBJ_PARENT)) {
					p_dlg->getCtrlData(CTLSEL_COMMOBJ_PARENT, &pEntry->ParentID);
				}
				pEntry->ID = _id;
				name.CopyTo(pEntry->Name, sizeof(pEntry->Name));
				symb.CopyTo(pEntry->Symb, sizeof(pEntry->Symb));
				pEntry->Flags = flags;
				valid_data = 1;
				r = 1;
			}
		}
	}
	CATCH
		r = 0;
	ENDCATCH
	delete p_obj;
	delete p_dlg;
	return r;
}

int PPObjReference::CheckName(PPID id, const char * pName, int silent)
{
	int    ok = 1;
	SString temp_buf(pName);
	THROW_PP(temp_buf.NotEmptyS(), PPERR_NAMENEEDED);
	THROW(CheckDupName(id, temp_buf));
	CATCH
		ok = 0;
		if(!silent)
			PPError();
	ENDCATCH
	return ok;
}

int PPObjReference::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	bool   is_new = false;
	ReferenceTbl::Rec rec;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(P_Ref->GetItem(Obj, *pID, &rec) > 0);
	}
	{
		PPCommObjEntry param(Obj, rec.ObjID, rec.ObjName, rec.Symb);
		if(IS_DYN_OBJTYPE(Obj)) {
			param.Flags = rec.Val1;
			param.ParentID = rec.Val2;
		}
		THROW(ok = PPObjReference::EditCommObjItem(&param));
		if(ok > 0) {
			rec.ObjID = param.ID;
			STRNSCPY(rec.Symb, param.Symb);
			STRNSCPY(rec.ObjName, param.Name);
			if(IS_DYN_OBJTYPE(Obj)) {
				rec.Val1 = param.Flags;
				rec.Val2 = param.ParentID;
			}
			THROW(is_new || CheckRights(PPR_MOD));
			if(*pID)
				*pID = rec.ObjID;
			THROW(StoreItem(Obj, *pID, &rec, 1));
			*pID = rec.ObjID;
			ok = cmOK;
		}
	}
	CATCHZOKPPERR
	return ok;
}

/*virtual*/int PPObjReference::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options, void * pExtraParam)
{
	int    r = -1;
	if(!CheckRights(PPR_DEL))
		r = 0;
	else if(!(options & PPObject::user_request) || PPMessage(mfConf|mfYesNo, PPCFM_DELETE) == cmYes) {
		PPWaitStart();
		r = BIN(P_Ref->RemoveItem(Obj, id, BIN(options & PPObject::use_transaction)) && RemoveSync(id));
		PPWaitStop();
	}
	if(r == 0 && (options & PPObject::user_request))
		PPError();
	return r;
}

StrAssocArray * PPObjReference::MakeStrAssocList(void * extraPtr)
{
	StrAssocArray * p_list = new StrAssocArray();
	THROW_MEM(p_list);
	{
		const int is_hier = BIN(IS_DYN_OBJTYPE(Obj) && ImplementFlags & implTreeSelector);
		ReferenceTbl::Rec rec;
		for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
			if(!FiltProc || FiltProc(&rec, extraPtr)) {
				if(*strip(rec.ObjName) == 0)
					ideqvalstr(rec.ObjID, rec.ObjName, sizeof(rec.ObjName));
				if(is_hier) {
					const  PPID parent_id = (rec.Val2 != rec.ObjID) ? rec.Val2 : 0;
					THROW_SL(p_list->Add(rec.ObjID, parent_id, rec.ObjName));
				}
				else {
					THROW_SL(p_list->Add(rec.ObjID, rec.ObjName));
				}
			}
		}
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjReference::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	if(IS_DYN_OBJTYPE(Obj)) {
		if(msg == DBMSG_OBJDELETE) {
			if(_obj == PPOBJ_DYNAMICOBJS) {
				if(_id == Obj)
					return RetRefsExistsErr(Obj, 0);
			}
		}
	}
	return DBRPL_OK;
}

int PPObjReference::IsRecEq(const ReferenceTbl::Rec & rR1, const ReferenceTbl::Rec & rR2)
{
	return P_Ref->GetFields().IsEqualRecords(&rR1, &rR2);
}

int PPObjReference::Serialize_(int dir, ReferenceTbl::Rec * pPack, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	SBuffer buffer;
	if(dir > 0) {
		THROW_SL(P_Ref->SerializeRecord(dir, pPack, buffer, &pCtx->SCtx));
		THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0));
	}
	else if(dir < 0) {
		THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0));
		THROW_SL(P_Ref->SerializeRecord(dir, pPack, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int PPObjReference::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new ReferenceTbl::Rec);
	if(stream == 0) {
		if(Obj == PPOBJ_DYNAMICOBJS)
			p->Priority = 100;
		THROW(Search(id, p->Data) > 0);
	}
	else {
		THROW(Serialize_(-1, static_cast<ReferenceTbl::Rec *>(p->Data), stream, pCtx));
	}
	CATCHZOK
	return ok;
}

int PPObjReference::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	THROW(p->Data);
	if(stream == 0) {
		PPID   same_id = 0;
		ReferenceTbl::Rec * p_rec = static_cast<ReferenceTbl::Rec *>(p->Data);
		ok = 0; // !
		if(*pID == 0) {
			if(p_rec->ObjID < PP_FIRSTUSRREF) {
				if(Search(p_rec->ObjID) > 0) {
					*pID = p_rec->ObjID;
					ok = 1;
				}
			}
			else if(IS_DYN_OBJTYPE(Obj) && p_rec->ObjName[0] && SearchByName(p_rec->ObjName, &same_id, 0) > 0) {
				//
				// Контекстная синхронизация по имени для динамических объектов
				//
				*pID = same_id;
				ok = 1;
			}
			else
				p_rec->ObjID = 0;
		}
		else {
			p_rec->ObjID = *pID;
			//
			// Не следует модифицировать:
			//   - описание раздела базы даных
			//   - структуры штрихкодов
			//   - счетчики операций
			//
			if(oneof3(Obj, PPOBJ_DBDIV, PPOBJ_BCODESTRUC, PPOBJ_OPCOUNTER))
				ok = 1;
		}
		if(!ok) {
			int    r = StoreItem(Obj, *pID, p_rec, 1);
			if(r > 0) {
				ok = *pID ? 102 : 101;
				*pID = P_Ref->data.ObjID;
			}
			else if(r < 0) // Ничего не изменилось
				ok = 1;
			else {
				pCtx->OutputAcceptObjErrMsg(Obj, p_rec->ObjID, p_rec->ObjName);
   	            ok = -1;
			}
		}
	}
	else {
		THROW(Serialize_(+1, static_cast<ReferenceTbl::Rec *>(p->Data), stream, pCtx));
	}
	CATCHZOK
	return ok;
}

int PPObjReference::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(IS_DYN_OBJTYPE(Obj)) {
		if(p && p->Data) {
			PPDynanicObjItem * p_rec = static_cast<PPDynanicObjItem *>(p->Data);
			//
			// Очень необычный случай: при разрешении ссылки может измениться тип объекта
			//
			THROW(ProcessObjRefInArray(PPOBJ_DYNAMICOBJS, &Obj, ary, replace));
			THROW(ProcessObjRefInArray(Obj, &p_rec->ParentID, ary, replace));
		}
		else
			ok = -1;
	}
	CATCHZOK
	return ok;
}

int PPObjReference::ProcessReservedItem(TVRez & rez)
{
	int    ok = 1;
	int    r;
	SString name;
	PPID   id = static_cast<PPID>(rez.getUINT());
	rez.getString(name, 2);
	PPExpandString(name, CTRANSF_UTF8_TO_INNER);
	THROW(r = Search(id));
	if(r < 0) {
		ReferenceTbl::Rec rec;
		rec.ObjType = Obj;
		rec.ObjID   = id;
		STRNSCPY(rec.ObjName, name);
		THROW(StoreItem(Obj, 0, &rec, 1));
	}
	CATCHZOK
	return ok;
}

int PPObjReference::LoadReservedItems(uint rezID)
{
	int    ok = 1;
	uint   num_recs, i;
	TVRez * p_rez = P_SlRez;
	THROW_PP(p_rez, PPERR_RESFAULT);
	THROW_PP(p_rez->findResource(rezID, PP_RCDATA), PPERR_RESFAULT);
	THROW_PP(num_recs = p_rez->getUINT(), PPERR_RESFAULT);
	for(i = 0; i < num_recs; i++) {
		THROW(ProcessReservedItem(*p_rez));
	}
	CATCHZOK
	return ok;
}

int PPObjReference::MakeReserved(long flags)
{
	uint   rez_id = 0;
	switch(Obj) {
		case PPOBJ_CONFIG:        rez_id = ROD_CONFIG;        break;
		case PPOBJ_USRGRP:        rez_id = ROD_USRGRP;        break;
		case PPOBJ_USR:           rez_id = ROD_USR;           break;
		case PPOBJ_UNIT:          rez_id = ROD_UNIT;          break;
		case PPOBJ_CITYSTATUS:    rez_id = ROD_CITYSTATUS;    break;
		case PPOBJ_PERSONKIND:    rez_id = ROD_PRSNKIND;      break;
		case PPOBJ_PRSNSTATUS:    rez_id = ROD_PRSNSTATUS;    break;
		case PPOBJ_OPRTYPE:       rez_id = ROD_OPRTYPE;       break;
		case PPOBJ_BNKACCTYPE:    rez_id = ROD_BNKACCTYPE;    break;
		case PPOBJ_AMOUNTTYPE:    rez_id = ROD_AMOUNTTYPE;    break;
		case PPOBJ_ACTION:        rez_id = ROD_ACTION;        break;
		case PPOBJ_REGISTERTYPE:  rez_id = ROD_REGISTERTYPE;  break;
		case PPOBJ_ELINKKIND:     rez_id = ROD_ELINKKIND;     break;
		case PPOBJ_PERSONRELTYPE: rez_id = ROD_PERSONRELTYPE; break;
		case PPOBJ_TAXSYSTEMKIND: rez_id = ROD_TAXSYSTEMKIND; break; // @v10.6.0
		case PPOBJ_GOODSBASKET:   rez_id = ROD_GOODSBASKET;   break; // @v10.6.8
	}
	return rez_id ? LoadReservedItems(rez_id) : 1;
}
//
// @ModuleDef(PPObjUnit)
//
static int UnitListFilt(void * rec, void * extraPtr)
{
	const long extra_flags = reinterpret_cast<long>(extraPtr);
	return extra_flags ? ((static_cast<const PPUnit *>(rec)->Flags & extra_flags) == extra_flags) : 1;
}

PPUnit2::PPUnit2()
{
	THISZERO();
}

int PPUnit2::ValidateQuantityFraction(double qtty) const
{
	int    ok = 1;
	const  double frac_tolerance = 1.1E-5; // @v11.5.2 1E-5-->1.1E-5
	if(Fragmentation > 0 && Fragmentation <= 100000) {
		if(!fsplitintofractions(qtty, Fragmentation, frac_tolerance, 0, 0, 0))
			ok = PPSetError(PPERR_QTTYMUSTMATCHUFRAG, SLS.AcquireRvlStr().Cat(Fragmentation));
	}
	else if(Rounding_ > 0.0) {
		const double _r = round(qtty, Rounding_, 0);
		if(!feqeps(_r, qtty, frac_tolerance))
			ok = PPSetError(PPERR_QTTYMUSTMATCHUFRAG, SLS.AcquireRvlStr().Cat(Rounding_, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
	}
	else if(Flags & PPUnit::IntVal) {
		const double _f = ffrac(qtty);
		if(!feqeps(_f, 0.0, frac_tolerance) && !feqeps(_f, 1.0, frac_tolerance))
			ok = PPSetError(PPERR_QTTYMUSTBEINT);
	}
	return ok;
}

PPObjUnit::PPObjUnit(void * extraPtr) : PPObjReference(PPOBJ_UNIT, extraPtr)
{
	FiltProc = UnitListFilt;
}

int PPObjUnit::Browse(void * extraPtr)
{
	class ObjUnitDialog : public ObjViewDialog {
	public:
		ObjUnitDialog(PPObjUnit * pObj, void * extraPtr) : ObjViewDialog(DLG_UNITVIEW, pObj, extraPtr)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			ObjViewDialog::handleEvent(event);
			if(event.isCmd(cmUniteMaxLike) && P_Obj) {
				if(static_cast<PPObjUnit *>(P_Obj)->UniteMaxLike() > 0)
					updateList(-1);
				clearEvent(event);
			}
		}
	};
	int    ok = cmCancel;
	THROW(CheckRights(PPR_READ));
	{
		ObjUnitDialog * p_dlg = new ObjUnitDialog(this, extraPtr);
		THROW(CheckDialogPtr(&p_dlg));
		ok = ExecViewAndDestroy(p_dlg);
	}
	CATCHZOK
	return ok;
}

int PPObjUnit::SearchMaxLike(const PPUnit * pRec, PPID * pID)
{
	int    ok = -1;
	if(pRec) {
		ReferenceTbl::Key1 k1;
		MEMSZERO(k1);
		k1.ObjType = Obj;
		STRNSCPY(k1.ObjName, pRec->Name);
		if(SearchByKey(P_Ref, 1, &k1, 0) > 0) {
			const PPUnit & rec = *reinterpret_cast<const PPUnit *>(&P_Ref->data);
			do {
				if(pRec->ID != rec.ID) {
					ASSIGN_PTR(pID, P_Ref->data.ObjID);
					ok = 1;
				}
			} while(ok < 0 && P_Ref->search(1, &k1, spNext) && k1.ObjType == Obj && sstrlen(pRec->Name) == sstrlen(k1.ObjName) && stricmp866(pRec->Name, k1.ObjName) == 0);
		}
	}
	return ok;
}

int PPObjUnit::UniteMaxLike()
{
	int    ok = 1;
	StrAssocArray * p_list = 0;
	if(CONFIRMCRIT(PPCFM_UNITELIKEUNITS)) {
		PPWaitStart();
		p_list = MakeStrAssocList(0);
		if(p_list) {
			SString msg;
			p_list->SortByID();
			PPLoadText(PPTXT_UNITELIKEUNITS, msg);
			{
				PPTransaction tra(1); // @v9.8.6
				THROW(tra); // @v9.8.6
				for(uint i = 0; i < p_list->getCount(); i++) {
					PPID   id = p_list->Get(i).Id;
					PPID   like_id = 0;
					PPUnit rec;
					if(Search(id, &rec) > 0) {
						while(SearchMaxLike(&rec, &like_id) > 0) {
							THROW(PPObject::ReplaceObj(Obj, like_id, id, 0));
							THROW_SL(p_list->Remove(like_id));
						}
					}
					PPWaitPercent(i, p_list->getCount() - 1, msg);
				}
				THROW(tra.Commit()); // @v9.8.6
			}
		}
	}
	CATCH
		PPWaitStop();
		ok = PPErrorZ();
	ENDCATCH
	ZDELETE(p_list);
	PPWaitStop();
	return ok;
}

int PPObjUnit::IsPhisical(PPID id)
{
	PPUnit rec;
	return BIN(Fetch(id, &rec) > 0 && rec.Flags & PPUnit::Physical);
}

int PPObjUnit::IsInteger(PPID id)
{
	PPUnit rec;
	return BIN(Fetch(id, &rec) > 0 && rec.Flags & PPUnit::IntVal);
}

int PPObjUnit::TranslateToBase(PPID unitID, PPID baseUnitID, double * pRatio)
{
	int    ok = -1;
	double ratio = 0.0;
	if(unitID) {
		if(unitID == baseUnitID) {
			ratio = 1.0;
			ok = 1;
		}
		else {
			PPUnit u_rec;
			if(Fetch(unitID, &u_rec) > 0 && u_rec.BaseUnitID == baseUnitID && u_rec.BaseRatio > 0.0) {
				ratio = u_rec.BaseRatio;
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pRatio, ratio);
	return ok;
}

int PPObjUnit::AddSimple(PPID * pID, const char * pName, long flags, int use_ta)
{
	int    ok = -1, r;
	PPID   id = 0;
	if(pName && pName[0]) {
		THROW(r = SearchByName(pName, &id));
		if(r > 0)
			ok = 1;
		else {
			PPUnit unit_rec;
			STRNSCPY(unit_rec.Name, pName);
			if(flags)
				unit_rec.Flags = flags;
			else
				unit_rec.Flags |= PPUnit::Trade;
			THROW(P_Ref->AddItem(PPOBJ_UNIT, &(id = 0), &unit_rec, use_ta));
			ok = 2;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pID, id);
	return ok;
}

int PPObjUnit::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	THROW(p->Data);
	if(stream == 0) {
		PPUnit * p_rec = static_cast<PPUnit *>(p->Data);
		if(*pID || SearchByName(p_rec->Name, pID, 0) > 0) {
			p_rec->ID = *pID;
			int    r = StoreItem(Obj, *pID, p_rec, 1);
			if(r > 0)
   			    ok = ((*pID = P_Ref->data.ObjID), 102);
			else if(r < 0)
				ok = 1;
			else {
				pCtx->OutputAcceptObjErrMsg(Obj, p_rec->ID, p_rec->Name);
				THROW(*pID);
				ok = -1;
			}
		}
		else {
			p_rec->ID = *pID = 0;
			if(StoreItem(Obj, *pID, p_rec, 1))
			    ok = ((*pID = P_Ref->data.ObjID), 101);
			else {
				pCtx->OutputAcceptObjErrMsg(Obj, p_rec->ID, p_rec->Name);
				ok = -1;
			}
		}
	}
	else {
		THROW(Serialize_(+1, static_cast<ReferenceTbl::Rec *>(p->Data), stream, pCtx));
	}
	CATCHZOK
	return ok;
}

int PPObjUnit::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == PPOBJ_UNIT) {
			PPUnit unit_rec;
			for(PPID id = 0; ok == DBRPL_OK && EnumItems(&id, &unit_rec) > 0;)
				if(unit_rec.BaseUnitID == _id)
					ok = RetRefsExistsErr(Obj, id);
		}
	}
	else if(msg == DBMSG_OBJREPLACE) {
		if(_obj == PPOBJ_UNIT) {
			PPUnit unit_rec;
			for(PPID id = 0; ok == DBRPL_OK && EnumItems(&id, &unit_rec) > 0;) {
				if(unit_rec.BaseUnitID == _id) {
					unit_rec.BaseUnitID = reinterpret_cast<long>(extraPtr);
					if(!UpdateItem(unit_rec.ID, &unit_rec, 0))
						ok = DBRPL_ERROR;
					else
						Dirty(unit_rec.ID);
				}
			}
			if(ok && !BroadcastObjMessage(DBMSG_OBJREPLACE, Obj, _id, extraPtr))
				ok = DBRPL_ERROR;
		}
	}
	return ok;
}
//
//
//
class UnitCache : public ObjCache {
public:
	UnitCache() : ObjCache(PPOBJ_UNIT, sizeof(UnitData))
	{
	}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct UnitData : public ObjCacheEntry {
		PPID   BaseUnitID;
		double BaseRatio;
		double Rounding_;
		long   Flags;
		uint16 Fragmentation; // @v11.2.5
		uint16 Reserve;       // @v11.2.5 @alignment
	};
};

int UnitCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	UnitData * p_cache_rec = static_cast<UnitData *>(pEntry);
	PPObjUnit u_obj;
	PPUnit rec;
	if(u_obj.Search(id, &rec) > 0) {
		p_cache_rec->BaseUnitID = rec.BaseUnitID;
		p_cache_rec->BaseRatio  = rec.BaseRatio;
		p_cache_rec->Rounding_  = rec.Rounding_;
		p_cache_rec->Flags = rec.Flags;
		p_cache_rec->Fragmentation = rec.Fragmentation; // @v11.2.5
		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Abbr);
		b.Add(rec.Code);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void UnitCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPUnit * p_data_rec = static_cast<PPUnit *>(pDataRec);
	const UnitData * p_cache_rec = static_cast<const UnitData *>(pEntry);
	if(p_data_rec) { // @v11.3.2
		memzero(p_data_rec, sizeof(*p_data_rec));
		p_data_rec->Tag        = PPOBJ_UNIT;
		p_data_rec->ID = p_cache_rec->ID;
		p_data_rec->BaseUnitID = p_cache_rec->BaseUnitID;
		p_data_rec->BaseRatio  = p_cache_rec->BaseRatio;
		p_data_rec->Rounding_  = p_cache_rec->Rounding_;
		p_data_rec->Flags      = p_cache_rec->Flags;
		p_data_rec->Fragmentation = p_cache_rec->Fragmentation; // @v11.2.5
		MultTextBlock b(this, pEntry);
		b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
		b.Get(p_data_rec->Abbr, sizeof(p_data_rec->Abbr));
		b.Get(p_data_rec->Code, sizeof(p_data_rec->Code));
	}
}
// }

int PPObjUnit::Fetch(PPID id, PPUnit * pRec)
{
	UnitCache * p_cache = GetDbLocalCachePtr <UnitCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}

int PPObjUnit::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel;
	int    valid_data = 0;
	PPUnit unit;
	TDialog * dlg = 0;
	THROW(CheckRightsModByID(pID));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_UNIT))));
	if(*pID) {
		THROW(Search(*pID, &unit) > 0);
	}
	else {
		const long _extra = reinterpret_cast<long>(ExtraPtr);
		unit.Flags |= (_extra ? (_extra & 0x0000ffffL) : PPUnit::Trade);
	}
	dlg->setCtrlData(CTL_UNIT_NAME, unit.Name);
	dlg->setCtrlData(CTL_UNIT_ABBR, unit.Abbr);
	dlg->setCtrlData(CTL_UNIT_CODE, unit.Code);
	dlg->setCtrlData(CTL_UNIT_ID,   &unit.ID);
	dlg->disableCtrl(CTL_UNIT_ID, (!PPMaster || unit.ID));
	dlg->AddClusterAssoc(CTL_UNIT_FLAGS, 0, PPUnit::Trade);
	dlg->AddClusterAssoc(CTL_UNIT_FLAGS, 1, PPUnit::Physical);
	dlg->AddClusterAssoc(CTL_UNIT_FLAGS, 2, PPUnit::SI);
	dlg->AddClusterAssoc(CTL_UNIT_FLAGS, 3, PPUnit::IntVal);
	dlg->SetClusterData(CTL_UNIT_FLAGS, unit.Flags);
	SetupPPObjCombo(dlg, CTLSEL_UNIT_BASE, PPOBJ_UNIT, unit.BaseUnitID, 0, reinterpret_cast<void *>(PPUnit::SI));
	dlg->setCtrlReal(CTL_UNIT_BASERATIO, unit.BaseRatio);
	dlg->setCtrlReal(CTL_UNIT_ROUNDING,  unit.Rounding_);
	dlg->setCtrlUInt16(CTL_UNIT_FRAGM, unit.Fragmentation); // @v11.2.5
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		dlg->getCtrlData(CTL_UNIT_NAME, unit.Name);
		if(!CheckName(*pID, strip(unit.Name), 0))
			dlg->selectCtrl(CTL_UNIT_NAME);
		else {
			valid_data = 1;
			dlg->getCtrlData(CTL_UNIT_ABBR,  unit.Abbr);
			dlg->getCtrlData(CTL_UNIT_CODE,  unit.Code);
			dlg->getCtrlData(CTL_UNIT_ID,    &unit.ID);
			dlg->GetClusterData(CTL_UNIT_FLAGS, &unit.Flags);
			if(unit.Flags & PPUnit::SI)
				unit.Flags |= PPUnit::Physical;
			dlg->getCtrlData(CTLSEL_UNIT_BASE, &unit.BaseUnitID);
			if(unit.BaseUnitID && unit.BaseUnitID == *pID)
				PPErrorByDialog(dlg, CTL_UNIT_BASE, PPERR_BASEUNITEQTHIS);
			else {
				dlg->getCtrlData(CTL_UNIT_BASERATIO, &unit.BaseRatio);
				dlg->getCtrlData(CTL_UNIT_ROUNDING, &unit.Rounding_);
				dlg->getCtrlData(CTL_UNIT_FRAGM, &unit.Fragmentation); // @v11.2.5
				if(*pID)
					*pID = unit.ID;
				THROW(StoreItem(PPOBJ_UNIT, *pID, &unit, 1));
				Dirty(*pID);
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int PPObjUnit::ProcessReservedItem(TVRez & rez)
{
	int    ok = 1, r;
	SString name;
	SString abbr;
	SString flags;
	PPID   id = static_cast<PPID>(rez.getUINT());
	rez.getString(name, 2);
	PPExpandString(name, CTRANSF_UTF8_TO_INNER); // @v9.4.4
	rez.getString(abbr, 2);
	rez.getString(flags, 2);
	THROW(r = Search(id));
	if(r < 0) {
		PPUnit rec;
		rec.ID = id;
		STRNSCPY(rec.Name, name);
		STRNSCPY(rec.Abbr, abbr);
		for(size_t i = 0; i < flags.Len(); i++) {
			switch(toupper(flags.C(i))) {
				case 'S': rec.Flags |= PPUnit::SI;       break;
				case 'P': rec.Flags |= PPUnit::Physical; break;
				case 'T': rec.Flags |= PPUnit::Trade;    break;
				case 'H': rec.Flags |= PPUnit::Hide;     break;
				case 'I': rec.Flags |= PPUnit::IntVal;   break;
			}
		}
		THROW(StoreItem(PPOBJ_UNIT, 0, &rec, 1));
	}
	CATCHZOK
	return ok;
}
//
// Implementation of PPALDD_Unit
//
PPALDD_CONSTRUCTOR(Unit)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(Unit) { Destroy(); }

int PPALDD_Unit::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjUnit unit_obj;
		PPUnit unit_rec;
		if(unit_obj.Fetch(rFilt.ID, &unit_rec) > 0) {
			H.ID = unit_rec.ID;
			STRNSCPY(H.Name, unit_rec.Name);
			STRNSCPY(H.Abbr, unit_rec.Abbr);
			STRNSCPY(H.Code, unit_rec.Code);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// @ModuleDef(PPObjPersonStatus}
//
PPObjPersonStatus::PPObjPersonStatus(void * extraPtr) : PPObjReference(PPOBJ_PRSNSTATUS, extraPtr)
{
}

int PPObjPersonStatus::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel, valid_data = 0;
	ushort v = 0;
	PPPersonStatus rec;
	TDialog * dlg = 0;
	THROW(CheckRightsModByID(pID));
	dlg = new TDialog(DLG_PSNST);
	THROW(CheckDialogPtr(&dlg));
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	else
		MEMSZERO(rec);
	dlg->setCtrlData(CTL_PSNST_NAME, rec.Name);
	dlg->setCtrlData(CTL_PSNST_SYMB, rec.Symb);
	dlg->setCtrlData(CTL_PSNST_ID,   &rec.ID);
	if(rec.ID || !PPMaster)
		dlg->disableCtrl(CTL_PSNST_ID, 1);
	v = (rec.Flags & PSNSTF_PRIVATE) ? 0 : 1;
	dlg->setCtrlData(CTL_PSNST_TYPE, &v);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		dlg->getCtrlData(CTL_PSNST_NAME, rec.Name);
		dlg->getCtrlData(CTL_PSNST_SYMB, rec.Symb);
		if(!P_Ref->CheckUniqueSymb(Obj, *pID, rec.Name, offsetof(PPPersonStatus, Name))) {
			PPErrorByDialog(dlg, CTL_PSNST_NAME);
		}
		else if(!P_Ref->CheckUniqueSymb(Obj, *pID, rec.Symb, offsetof(PPPersonStatus, Symb))) {
			PPErrorByDialog(dlg, CTL_PSNST_SYMB);
		}
		else {
			valid_data = 1;
			dlg->getCtrlData(CTL_PSNST_TYPE, &v);
			SETFLAG(rec.Flags, PSNSTF_PRIVATE, v == 0);
			dlg->getCtrlData(CTL_PSNST_ID, &rec.ID);
			if(*pID)
			   	*pID = rec.ID;
			THROW(StoreItem(PPOBJ_PRSNSTATUS, *pID, &rec, 1));
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

class PersonStatusCache : public ObjCache {
public:
	PersonStatusCache() : ObjCache(PPOBJ_PRSNSTATUS, sizeof(Data))
	{
	}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		long   Flags;
	};
};

int PersonStatusCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjPersonStatus ps_obj;
	PPPersonStatus rec;
	if(ps_obj.Search(id, &rec) > 0) {
		p_cache_rec->Flags = rec.Flags;
		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Symb);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void PersonStatusCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPPersonStatus * p_data_rec = static_cast<PPPersonStatus *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->Tag   = ObjType;
	p_data_rec->ID    = p_cache_rec->ID;
	p_data_rec->Flags = p_cache_rec->Flags;
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
}

IMPL_OBJ_FETCH(PPObjPersonStatus, PPPersonStatus, PersonStatusCache);
//
// @ModuleDef(PPObjPersonCat)
//
PPObjPersonCat::PPObjPersonCat(void * extraPtr) : PPObjReference(PPOBJ_PRSNCATEGORY, extraPtr)
{
}

int PPObjPersonCat::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJREPLACE) {
		if(_obj == Obj) {
			if(!BroadcastObjMessage(DBMSG_OBJREPLACE, Obj, _id, extraPtr))
				ok = DBRPL_ERROR;
		}
	}
	return ok;
}

int PPObjPersonCat::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	THROW(p->Data);
	if(stream == 0) {
		PPPersonCat * p_rec = static_cast<PPPersonCat *>(p->Data);
		if(*pID || SearchByName(p_rec->Name, pID, 0) > 0) {
			p_rec->ID = *pID;
			int    r = StoreItem(Obj, *pID, p_rec, 1);
			if(r > 0)
   			    ok = ((*pID = P_Ref->data.ObjID), 102);
			else if(r < 0)
				ok = 1;
			else {
				pCtx->OutputAcceptObjErrMsg(Obj, p_rec->ID, p_rec->Name);
				THROW(*pID);
				ok = -1;
			}
		}
		else {
			p_rec->ID = *pID = 0;
			if(StoreItem(Obj, *pID, p_rec, 1))
			    ok = ((*pID = P_Ref->data.ObjID), 101);
			else {
				pCtx->OutputAcceptObjErrMsg(Obj, p_rec->ID, p_rec->Name);
				ok = -1;
			}
		}
	}
	else {
		THROW(Serialize_(+1, static_cast<ReferenceTbl::Rec *>(p->Data), stream, pCtx));
	}
	CATCHZOK
	return ok;
}

class PersonCatCache : public ObjCache {
public:
	PersonCatCache() : ObjCache(PPOBJ_PRSNCATEGORY, sizeof(ObjCacheEntry))
	{
	}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
};

int PersonCatCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	ObjCacheEntry * p_cache_rec = pEntry;
	PPObjPersonCat ps_obj;
	PPPersonCat rec;
	if(ps_obj.Search(id, &rec) > 0) {
		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Symb);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void PersonCatCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPPersonCat * p_data_rec = static_cast<PPPersonCat *>(pDataRec);
	const ObjCacheEntry * p_cache_rec = pEntry;
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->Tag   = ObjType;
	p_data_rec->ID    = p_cache_rec->ID;
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
}

IMPL_OBJ_FETCH(PPObjPersonCat, PPPersonCat, PersonCatCache);
//
// @ModuleDef(PPObjELinkKind)
//
PPObjELinkKind::PPObjELinkKind(void * extraPtr) : PPObjReference(PPOBJ_ELINKKIND, extraPtr)
{
}

int PPObjELinkKind::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel, valid_data = 0;
	ushort v = 0;
	PPELinkKind rec;
	TDialog * dlg = 0;
	THROW(CheckRightsModByID(pID));
	dlg = new TDialog(DLG_ELNKK);
	THROW(CheckDialogPtr(&dlg));
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	else
		MEMSZERO(rec);
	dlg->setCtrlData(CTL_ELNKK_NAME, rec.Name);
	dlg->setCtrlData(CTL_ELNKK_SYMB, rec.Symb); // @v9.9.11
	dlg->setCtrlData(CTL_ELNKK_ID,   &rec.ID);
	if(rec.ID || !PPMaster)
		dlg->disableCtrl(CTL_ELNKK_ID, 1);
	dlg->AddClusterAssoc(CTL_ELNKK_TYPE, 0, ELNKRT_PHONE);
	dlg->AddClusterAssoc(CTL_ELNKK_TYPE, 1, ELNKRT_FAX);
	dlg->AddClusterAssoc(CTL_ELNKK_TYPE, 2, ELNKRT_PAGER);
	dlg->AddClusterAssoc(CTL_ELNKK_TYPE, 3, ELNKRT_EMAIL);
	dlg->AddClusterAssoc(CTL_ELNKK_TYPE, 4, ELNKRT_WEBADDR);
	dlg->AddClusterAssoc(CTL_ELNKK_TYPE, 5, ELNKRT_INTERNALEXTEN); // @v9.9.11
	dlg->AddClusterAssocDef(CTL_ELNKK_TYPE, 6, ELNKRT_UNKNOWN); // @v9.9.11 5-->6
	dlg->SetClusterData(CTL_ELNKK_TYPE, rec.Type);
	dlg->setCtrlUInt16(CTL_ELNKK_FLAGS, BIN(rec.Flags & ELNKF_PREF));
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		dlg->getCtrlData(CTL_ELNKK_NAME, rec.Name);
		dlg->getCtrlData(CTL_ELNKK_SYMB, rec.Symb); // @v9.9.11
		if(!CheckName(rec.ID, rec.Name, 1))
			PPErrorByDialog(dlg, CTL_ELNKK_NAME);
		else if(!P_Ref->CheckUniqueSymb(Obj, rec.ID, rec.Symb, offsetof(ReferenceTbl::Rec, Symb))) {
			PPErrorByDialog(dlg, CTL_ELNKK_SYMB);
		}
		else {
			valid_data = 1;
			dlg->GetClusterData(CTL_ELNKK_TYPE, &rec.Type);
			v = dlg->getCtrlUInt16(CTL_ELNKK_FLAGS);
			SETFLAG(rec.Flags, ELNKF_PREF, v & 1);
			dlg->getCtrlData(CTL_ELNKK_ID, &rec.ID);
			if(*pID)
			   	*pID = rec.ID;
			THROW(StoreItem(PPOBJ_ELINKKIND, *pID, &rec, 1));
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int PPObjELinkKind::AddSimple(PPID * pID, const char * pName, int use_ta)
{
	int    ok = 1, r;
	PPELinkKind elk_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID < PP_FIRSTUSROBJ) {
			THROW(r = SearchObject(PPOBJ_ELINKKIND, *pID, &elk_rec));
			if(r < 0) {
				MEMSZERO(elk_rec);
				STRNSCPY(elk_rec.Name, pName);
				elk_rec.Type = ELNKRT_UNKNOWN;
				THROW(P_Ref->AddItem(Obj, pID, &elk_rec, 0));
			}
		}
		else {
			THROW(r = SearchByName(pName, pID));
			if(r < 0) {
				*pID = 0;
				MEMSZERO(elk_rec);
				STRNSCPY(elk_rec.Name, pName);
				elk_rec.Type = ELNKRT_UNKNOWN;
				THROW(P_Ref->AddItem(Obj, pID, &elk_rec, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjELinkKind::ProcessReservedItem(TVRez & rez)
{
	int    ok = 1;
	int    r;
	SString name;
	SString type;
	uint   pref;
	PPID   id = static_cast<PPID>(rez.getUINT());
	rez.getString(name, 2);
	PPExpandString(name, CTRANSF_UTF8_TO_INNER);
	rez.getString(type, 2);
	pref = rez.getUINT();
	THROW(r = Search(id));
	if(r < 0) {
		PPELinkKind rec;
		MEMSZERO(rec);
		rec.ID = id;
		STRNSCPY(rec.Name, name);
		switch(toupper(type.C(0))) {
			case 'P': rec.Type = ELNKRT_PHONE;   break;
			case 'F': rec.Type = ELNKRT_FAX;     break;
			case 'G': rec.Type = ELNKRT_PAGER;   break;
			case 'E': rec.Type = ELNKRT_EMAIL;   break;
			case 'W': rec.Type = ELNKRT_WEBADDR; break;
			default:  rec.Type = ELNKRT_UNKNOWN; break;
		}
		SETFLAG(rec.Flags, ELNKF_PREF, pref);
		THROW(StoreItem(PPOBJ_ELINKKIND, 0, &rec, 1));
	}
	CATCHZOK
	return ok;
}

class ELinkKindCache : public ObjCache {
public:
	ELinkKindCache() : ObjCache(PPOBJ_ELINKKIND, sizeof(Data))
	{
	}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		long   Type;
		long   Flags;
	};
};

int ELinkKindCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjELinkKind el_obj;
	PPELinkKind rec;
	if(el_obj.Search(id, &rec) > 0) {
		p_cache_rec->Type  = rec.Type;
		p_cache_rec->Flags = rec.Flags;
		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Symb);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void ELinkKindCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPELinkKind * p_data_rec = static_cast<PPELinkKind *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->Tag   = PPOBJ_ELINKKIND;
	p_data_rec->ID    = p_cache_rec->ID;
	p_data_rec->Type  = p_cache_rec->Type;
	p_data_rec->Flags = p_cache_rec->Flags;
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
}

IMPL_OBJ_FETCH(PPObjELinkKind, PPELinkKind, ELinkKindCache);
//
// @ModuleDef(PPObjOprType)
//
PPObjOprType::PPObjOprType(void * extraPtr) : PPObjReference(PPOBJ_OPRTYPE, extraPtr)
{
}
//
// @ModuleDef(PPObjFormula)
//
PPObjFormula::PPObjFormula(void * extraPtr) : PPObjReference(PPOBJ_FORMULA, extraPtr)
{
}

// prototype
int DirtyBillSymbCache(const char * pSymb);

int PPObjFormula::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1, r = cmCancel, valid_data = 0;
	SString frm_name, frm_expr, prev_name;
	PPObjAmountType amtobj;
	TDialog * dlg = 0;
	if(*pID) {
		THROW(Get(*pID, frm_name, frm_expr) > 0);
		prev_name = frm_name;
	}
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_FORMULA))));
	dlg->setCtrlString(CTL_FORMULA_SYMB, frm_name);
	dlg->setCtrlString(CTL_FORMULA_EXPR, frm_expr);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		PPID tmpid = 0;
		dlg->getCtrlString(CTL_FORMULA_SYMB, frm_name);
		dlg->getCtrlString(CTL_FORMULA_EXPR, frm_expr);
		if(frm_name.Strip().IsEmpty())
			PPError(PPERR_NAMENEEDED, 0);
		else if(SearchByName(frm_name, &tmpid) > 0 && tmpid != *pID)
			PPError(PPERR_DUPFORMSYMB, 0);
		else if(!amtobj.CheckDupSymb(0, frm_name))
			PPError();
		else {
			valid_data = 1;
			THROW(Put(pID, frm_name, frm_expr, 1));
			DirtyBillSymbCache(prev_name);
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int PPObjFormula::Put(PPID * pID, const char * pName, const char * pExpr, int use_ta)
{
	int    ok = 1;
	ReferenceTbl::Rec rec;
	THROW_PP(pExpr[0], PPERR_INVFORMULA);
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	else {
		PPID   tmpid = 0;
		THROW_PP(SearchByName(pName, &tmpid) < 0, PPERR_DUPFORMSYMB);
	}
   	STRNSCPY(rec.ObjName, pName);
   	{
   		PPTransaction tra(use_ta);
   		THROW(tra);
		if(*pID) {
			THROW(P_Ref->UpdateItem(Obj, *pID, &rec, 1, 0));
		}
		else {
			THROW(P_Ref->AddItem(Obj, pID, &rec, 0));
		}
		THROW(P_Ref->PutPropVlrString(Obj, *pID, 1, pExpr));
		THROW(tra.Commit());
   	}
	CATCHZOK
	return ok;
}

int PPObjFormula::Get(PPID id, SString & rName, SString & rFormula)
{
	int    ok = 1;
	ReferenceTbl::Rec rec;
	if(id == 0) {
		THROW(SearchByName(rName, &id, &rec) > 0);
	}
	else {
		THROW(Search(id, &rec) > 0);
	}
	THROW(P_Ref->GetPropVlrString(Obj, id, 1, rFormula));
	rName = strip(rec.ObjName);
	CATCHZOK
	return ok;
}

int PPObjFormula::Browse(void * extraPtr) { return RefObjView(this, PPDS_CRRFORMULA, 0); }
//
// @ModuleDef(PPObjCurrency)
//
PPObjCurrency::PPObjCurrency(void * extraPtr) : PPObjReference(PPOBJ_CURRENCY, extraPtr)
{
}

int PPObjCurrency::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel, valid_data = 0;
	PPCurrency rec;
	TDialog * dlg = 0;
	THROW(CheckRightsModByID(pID));
	dlg = new TDialog(DLG_CURRENCY);
	THROW(CheckDialogPtr(&dlg));
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	else
		MEMSZERO(rec);
	dlg->setCtrlData(CTL_CURRENCY_NAME, rec.Name);
	dlg->setCtrlData(CTL_CURRENCY_SYMB, rec.Symb);
	dlg->setCtrlData(CTL_CURRENCY_CODE, &rec.Code);
	dlg->setCtrlData(CTL_CURRENCY_ID,   &rec.ID);
	dlg->disableCtrl(CTL_CURRENCY_ID, (!PPMaster || rec.ID));
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		int    err = 0, sel = 0;
		dlg->getCtrlData(CTL_CURRENCY_NAME, rec.Name);
		dlg->getCtrlData(CTL_CURRENCY_SYMB, rec.Symb);
		dlg->getCtrlData(CTL_CURRENCY_CODE, &rec.Code);
		dlg->getCtrlData(CTL_CURRENCY_ID,   &rec.ID);
		if(*strip(rec.Name) == 0) {
			err = PPERR_NAMENEEDED; sel = CTL_CURRENCY_NAME;
		}
		/*
		else if(!CheckDupSymb(*pID, strip(rec.Symb))) {
			err = PPErrCode; sel = CTL_CURRENCY_SYMB;
		}
		*/
		if(err) {
			PPError(err, 0);
			dlg->selectCtrl(sel);
		}
		else {
			valid_data = 1;
			if(*pID)
				*pID = rec.ID;
			THROW(StoreItem(PPOBJ_CURRENCY, *pID, &rec, 1));
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int PPObjCurrency::Browse(void * extraPtr) { return RefObjView(this, PPDS_CRRCURRENCY, 0); }
int PPObjCurrency::AddItem(PPID * pID, PPCurrency * pCurrency, int use_ta) { return StoreItem(PPOBJ_CURRENCY, *pID, pCurrency, use_ta); }

StrAssocArray * PPObjCurrency::CreateSelectorList(int asSymb, const PPIDArray * pIncludeList, const PPIDArray * pExcludeList)
{
	PPID   id = 0;
	PPCurrency rec;
	StrAssocArray * p_list = new StrAssocArray();
	THROW_MEM(p_list);
	while(EnumItems(&id, &rec) > 0) {
		if((!pIncludeList || pIncludeList->lsearch(id)) && (!pExcludeList || !pExcludeList->lsearch(id)))
			THROW_SL(p_list->Add(id, (asSymb && *strip(rec.Symb)) ? rec.Symb : rec.Name));
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjCurrency::Select(int asSymb, PPIDArray * pIncludeList, const PPIDArray * pExcludeList, PPID * pID)
{
	int    ok = -1;
	PPObjListWindow * p_lw = 0;
	ASSIGN_PTR(pID, 0);
	StrAssocArray * p_list = CreateSelectorList(asSymb, pIncludeList, pExcludeList);
	THROW(p_list);
	THROW_MEM(p_lw = new PPObjListWindow(Obj, p_list, 0/*flags*/, 0));
	if(ExecView(p_lw) == cmOK) {
		PPID   result = 0;
		if(p_lw->getResult(&result) && result) {
			ASSIGN_PTR(pID, result);
			ok = 1;
		}
	}
	CATCHZOK
	delete p_lw;
	return ok;
}

int PPObjCurrency::SearchSymb(PPID * pID, const char * pSymb)
{
	return P_Ref->SearchSymb(Obj, pID, pSymb, offsetof(PPCurrency, Symb));
}

int PPObjCurrency::SearchCode(PPID * pID, long code)
{
	int    ok = -1;
	PPID   id = 0;
	if(code) {
		Reference2Tbl::Key2 k2;
		k2.ObjType = Obj;
		k2.Val1 = code;
		if(P_Ref->search(2, &k2, spEq)) {
			id = P_Ref->data.ObjID;
            if(P_Ref->search(2, &k2, spNext) && P_Ref->data.ObjType == Obj && P_Ref->data.Val1 == code) {
				ok = 2;
            }
            else
				ok = 1;
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int SetupCurrencyCombo(TDialog * dlg, uint ctl, PPID id, uint /*flags*/, int asSymb, PPIDArray * pInclList)
{
	int    ok = 0;
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
	if(p_combo) {
		PPObjCurrency cur_obj;
		StrAssocArray * p_list = cur_obj.CreateSelectorList(asSymb, pInclList, 0);
		if(p_list) {
			PPObjListWindow * p_lw = new PPObjListWindow(PPOBJ_CURRENCY, p_list, 0/*flags*/, 0);
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

class CurrencyCache : public ObjCache {
public:
	CurrencyCache() : ObjCache(PPOBJ_CURRENCY, sizeof(Data))
	{
	}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		long   Code;
		long   Flags;
	};
};

int CurrencyCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjCurrency cur_obj;
	PPCurrency rec;
	if(cur_obj.Search(id, &rec) > 0) {
		p_cache_rec->Code  = rec.Code;
		p_cache_rec->Flags = rec.Flags;
		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Symb);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void CurrencyCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPCurrency * p_data_rec = static_cast<PPCurrency *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->Tag   = PPOBJ_CURRENCY;
	p_data_rec->ID    = p_cache_rec->ID;
	p_data_rec->Code  = p_cache_rec->Code;
	p_data_rec->Flags = p_cache_rec->Flags;
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
}

IMPL_OBJ_FETCH(PPObjCurrency, PPCurrency, CurrencyCache);
//
//
//
PPObjCurRateType::PPObjCurRateType(void * extraPtr) : PPObjReference(PPOBJ_CURRATETYPE, extraPtr)
{
}

int PPObjCurRateType::Browse(void * extraPtr) { return RefObjView(this, PPDS_CRRCURRATETYPE, 0); }
//
// @ModuleDef(PPObjDynamicObjs)
//
PPObjDynamicObjs::PPObjDynamicObjs(void * extraPtr) : PPObjReference(PPOBJ_DYNAMICOBJS, extraPtr)
{
}

int PPObjDynamicObjs::Browse(void * extraPtr)
{
	class DynObjsView : public ObjViewDialog {
	public:
		DynObjsView(PPObjDynamicObjs * _ppobj) : ObjViewDialog(DLG_DYNOBJSVIEW, _ppobj, 0)
		{
		}
	private:
		virtual void extraProc(long id)
		{
			if(id)
				ShowObjects(id, 0);
		}
	};
	return CheckRights(PPR_READ) ? CheckExecAndDestroyDialog(new DynObjsView(this), 1, 0) : PPErrorZ();
}

int PPObjDynamicObjs::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	bool   is_new = false;
	ReferenceTbl::Rec rec;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(P_Ref->GetItem(Obj, *pID, &rec));
	}
	{
		PPCommObjEntry param(Obj, rec.ObjID, rec.ObjName, rec.Symb, rec.Val1);
		THROW(ok = PPObjReference::EditCommObjItem(&param));
		if(ok > 0) {
			THROW(is_new || CheckRights(PPR_MOD));
			*pID = rec.ObjID = param.ID;
			STRNSCPY(rec.Symb, param.Symb);
			STRNSCPY(rec.ObjName, param.Name);
			rec.Val1 = param.Flags;
			if(is_new) {
				THROW(P_Ref->AllocDynamicObj(pID, rec.ObjName, rec.Val1, 1));
			}
			else {
				THROW(StoreItem(Obj, *pID, &rec, 1));
			}
			ok = cmOK;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjDynamicObjs::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	THROW(p->Data);
	if(stream == 0) {
		ReferenceTbl::Rec * p_rec = static_cast<ReferenceTbl::Rec *>(p->Data);
		PPID   same_id = 0;
		int    done = 0;
		if(*pID == 0) {
			if(p_rec->ObjID < PP_FIRSTUSRREF) {
				if(Search(p_rec->ObjID) > 0) {
					*pID = p_rec->ObjID;
					done = 1;
				}
				else
					p_rec->ObjID = 0;
			}
			//
			// Контекстная синхронизация по имени
			//
			else if(p_rec->ObjName[0] && SearchByName(p_rec->ObjName, &same_id, 0) > 0) {
				*pID = same_id;
				done = 1;
			}
			else
				p_rec->ObjID = 0;
		}
		else
			p_rec->ObjID = *pID;
		if(!done) {
			if(*pID == 0) {
				THROW(P_Ref->AllocDynamicObj(pID, p_rec->ObjName, p_rec->Val1, 1));
			}
			else {
				THROW(StoreItem(Obj, *pID, p_rec, 1));
			}
		}
	}
	else {
		THROW(Serialize_(+1, static_cast<ReferenceTbl::Rec *>(p->Data), stream, pCtx));
	}
	CATCHZOK
	return ok;
}

/*
//static
int PPObjDynamicObjs::ReplaceObjs(PPID srcID /*=0*//*)
{
	class ReplObjDialog : public TDialog {
	public:
		ReplObjDialog() : TDialog(DLG_REPLOBJ)
		{
			SString buf;
			setSubTitle(GetObjectTitle(PPOBJ_DYNAMICOBJS, buf));
		}
		int    setDTS(PPID destID, PPID srcID)
		{
			SetupPPObjCombo(this, CTLSEL_REPLOBJ_SRC,  PPOBJ_DYNAMICOBJS,   srcID,  OLW_CANSELUPLEVEL, 0);
			SetupPPObjCombo(this, CTLSEL_REPLOBJ_DEST, PPOBJ_DYNAMICOBJS,  destID, OLW_CANSELUPLEVEL, 0);
			return 1;
		}
		int    getDTS(PPID * pDestID, PPID * pSrcID)
		{
			int    ok = 1;
			PPID   src_id  = getCtrlLong(CTLSEL_REPLOBJ_SRC);
			PPID   dest_id = getCtrlLong(CTLSEL_REPLOBJ_DEST);
			THROW_PP(dest_id != 0 && src_id != 0, PPERR_REPLZEROOBJ);
			THROW_PP(dest_id != src_id, PPERR_REPLSAMEOBJ);
			ASSIGN_PTR(pSrcID, src_id);
			ASSIGN_PTR(pDestID, dest_id);
			CATCH
				selectCtrl(src_id == 0 ? CTL_REPLOBJ_DEST : CTL_REPLOBJ_SRC);
				ok = 0;
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVCOMMAND && TVCMD == cmExchange) {
				PPID   src_id = 0, dest_id = 0;
				getDTS(&dest_id, &src_id);
				setDTS(src_id, dest_id);
				clearEvent(event);
			}
		}
		PPID   ObjTypeID;
	};
	int    ok = -1;
	PPID   dest_id = 0, src_id = 0;
	ReplObjDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new ReplObjDialog())));
	dlg->setDTS(dest_id, src_id);
	while(ExecView(dlg) == cmOK)
		if(!dlg->getDTS(&dest_id, &src_id) || !PPObject::ReplaceObj(PPOBJ_DYNAMICOBJS, dest_id, src_id))
			PPError();
		else {
			dlg->setDTS((dest_id = 0), src_id);
			ok = 1;
		}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
*/
//
// @ModuleDecl(PPObjGlobalUserAcc)
//
PPGlobalUserAcc::PPGlobalUserAcc()
{
	THISZERO();
}

PPGlobalUserAccPacket::PPGlobalUserAccPacket() : PPExtStrContainer()
{
}

PPGlobalUserAccPacket & PPGlobalUserAccPacket::Z()
{
	MEMSZERO(Rec);
	TagL.Destroy();
	PPExtStrContainer::Z(); // @v11.9.9
	return *this;
}

int FASTCALL PPGlobalUserAccPacket::IsEq(const PPGlobalUserAccPacket & rS) const
{
	const  int exfl[] = { extssAccessKey };
#define NRECFLD(f) Rec.f != rS.Rec.f
	if(NRECFLD(Tag))
		return 0;
	else if(NRECFLD(ID))
		return 0;
	else if(NRECFLD(LocalDbUuid))
		return 0;
	else if(NRECFLD(Flags))
		return 0;
	else if(NRECFLD(LocalUserID))
		return 0;
	else if(NRECFLD(PersonID))
		return 0;
	else if(NRECFLD(ServiceIdent)) // @v10.5.5
		return 0;
	else if(stricmp(Rec.Name, rS.Rec.Name) != 0)
		return 0;
	else if(stricmp(Rec.Symb, rS.Rec.Symb) != 0)
		return 0;
	else if(stricmp(Rec.Password, rS.Rec.Password) != 0)
		return 0;
	else if(!TagL.IsEq(rS.TagL))
		return 0;
	else if(!PPExtStrContainer::IsEq(rS, SIZEOFARRAY(exfl), exfl)) // @v11.9.9
		return 0;
	else
		return 1;
#undef NRECFLD
}

int PPGlobalUserAccPacket::SetAccessKey(const char * pValue)
{
	return PutExtStrData(extssAccessKey, pValue);
}

int PPGlobalUserAccPacket::GetAccessKey(SString & rValue) const
{
	rValue.Z();
	int    ok = 0;
	if(GetExtStrData(extssAccessKey, rValue) > 0) {
		ok = 1;
	}
	else if(TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, rValue) > 0) {
		ok = 1;
	}
	return ok;
}

/*static*/int FASTCALL PPObjGlobalUserAcc::ReadConfig(PPGlobalUserAccConfig * pCfg)
{
	int    r = PPRef->GetPropMainConfig(PPPRP_GLOBALUSERACCCFG, pCfg, sizeof(*pCfg));
	if(r > 0) {
		if(pCfg)
			pCfg->Flags |= PPGlobalUserAccConfig::fValid;
	}
	else
		memzero(pCfg, sizeof(*pCfg));
	return r;
}

/*static*/int PPObjGlobalUserAcc::EditConfig()
{
	int    ok = -1;
	int    is_new = 0;
	PPGlobalUserAccConfig cfg;
	TDialog * dlg = new TDialog(DLG_GUACFG);
	THROW(CheckCfgRights(PPCFGOBJ_GLOBALUSERACC, PPR_READ, 0));
	THROW(is_new = ReadConfig(&cfg));
	THROW(CheckDialogPtr(&dlg));
	SetupPPObjCombo(dlg, CTLSEL_GUACFG_OWNERKIND, PPOBJ_PERSONKIND, cfg.PersonKindID, OLW_CANINSERT, 0);
	SetupPPObjCombo(dlg, CTLSEL_GUACFG_SCARDSER,  PPOBJ_SCARDSERIES, cfg.SCardSerID, OLW_CANINSERT, 0);
	// @v10.6.6 {
	dlg->AddClusterAssoc(CTL_GUACFG_FLAGS, 0, PPGlobalUserAccConfig::fAutoCreateGUID);
	dlg->SetClusterData(CTL_GUACFG_FLAGS, cfg.Flags);
	// } @v10.6.6 
	while(ok < 0 && ExecView(dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_GLOBALUSERACC, PPR_MOD, 0));
		dlg->getCtrlData(CTLSEL_GUACFG_OWNERKIND, &cfg.PersonKindID);
		dlg->getCtrlData(CTLSEL_GUACFG_SCARDSER,  &cfg.SCardSerID);
		dlg->GetClusterData(CTL_GUACFG_FLAGS, &cfg.Flags);
		{
			PPTransaction tra(1);
			THROW(tra);
			cfg.Flags &= ~PPGlobalUserAccConfig::fValid; // transient-flag
			THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_GLOBALUSERACCCFG, &cfg, sizeof(cfg), 0));
			DS.LogAction(is_new == -1 ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_GLOBALUSERACC, 0, 0, 0);
			THROW(tra.Commit());
			{
				PPObjGlobalUserAcc gua_obj;
				gua_obj.DirtyConfig();
			}
		}
		ok = 1;
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

PPObjGlobalUserAcc::PPObjGlobalUserAcc(void * extraPtr) : PPObjReference(PPOBJ_GLOBALUSERACC, extraPtr)
{
}

IMPL_DESTROY_OBJ_PACK(PPObjGlobalUserAcc, PPGlobalUserAccPacket);

int PPObjGlobalUserAcc::SerializePacket(int dir, PPGlobalUserAccPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW(pPack->TagL.Serialize(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int PPObjGlobalUserAcc::PutPacket(PPID * pID, PPGlobalUserAccPacket * pPack, int use_ta)
{
	int    ok = 1;
	PPID   _id = DEREFPTRORZ(pID);
	const  int is_new = (_id == 0);
	const  int is_removing = BIN(_id != 0 && pPack == 0);
	PPID   hid = 0;
	SString ext_buffer;
	PPGlobalUserAccPacket org_pack;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(_id) {
			THROW(GetPacket(_id, &org_pack) > 0);
		}
		if(pPack == 0) {
			if(*pID) {
				THROW(CheckRights(PPR_DEL));
				THROW(P_Ref->RemoveItem(Obj, _id, 0));
				// @v11.9.9 @fix (Reference::RemoveItem does it) THROW(P_Ref->RemoveProperty(Obj, _id, 0, 0));
				THROW(P_Ref->Ot.PutList(Obj, _id, 0, 0));
				THROW(P_Ref->UtrC.SetText(TextRefIdent(Obj, _id, PPTRPROP_GUAEXT), static_cast<const wchar_t *>(0), 0)); // @v11.9.9
				THROW(RemoveSync(_id));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, hid, 0);
			}
		}
		else {
			THROW(CheckRightsModByID(pID));
			// @v10.6.6 {
			{
				const ObjTagItem * p_guid_tag = pPack->TagL.GetItem(PPTAG_GUA_GUID);
				if(!p_guid_tag) {
					PPGlobalUserAccConfig cfg;
					if(PPObjGlobalUserAcc::ReadConfig(&cfg) > 0 && cfg.Flags & PPGlobalUserAccConfig::fAutoCreateGUID) {
						ObjTagItem guid_tag;
						if(guid_tag.SetGuid(PPTAG_GUA_GUID, &S_GUID(SCtrGenerate())))
							pPack->TagL.PutItem(PPTAG_GUA_GUID, &guid_tag);
					}
				}
			}
			// } @v10.6.6 
			if(_id) {
				if(pPack->IsEq(org_pack))
					ok = -1;
				else {
					if(!sstreq(pPack->Rec.Name, org_pack.Rec.Name))
						THROW(SendObjMessage(DBMSG_OBJNAMEUPDATE, PPOBJ_ARTICLE, Obj, _id, pPack->Rec.Name, 0));
					THROW(P_Ref->UpdateItem(Obj, _id, &pPack->Rec, 0/*logAction*/, 0)); // @v10.6.6 @fix logAction 1-->0
					THROW(P_Ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
					// @v11.9.9 {
					(ext_buffer = pPack->GetBuffer()).Strip();
					THROW(P_Ref->UtrC.SetText(TextRefIdent(Obj, _id, PPTRPROP_GUAEXT), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
					// } @v11.9.9 
					DS.LogAction(PPACN_OBJUPD, Obj, _id, 0, 0);
				}
			}
			else {
				THROW(P_Ref->AddItem(Obj, &_id, &pPack->Rec, 0));
				pPack->Rec.ID = _id;
				THROW(P_Ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
				// @v11.9.9 {
				(ext_buffer = pPack->GetBuffer()).Strip();
				THROW(P_Ref->UtrC.SetText(TextRefIdent(Obj, _id, PPTRPROP_GUAEXT), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0)); // @v11.9.9
				// } @v11.9.9 
				THROW(SendObjMessage(DBMSG_GLOBALACCADDED, PPOBJ_ARTICLE, Obj, _id) == DBRPL_OK);
				DS.LogAction(PPACN_OBJADD, Obj, _id, 0, 0);
				ASSIGN_PTR(pID, _id);
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		if(is_new) {
			*pID = 0;
			if(pPack)
				pPack->Rec.ID = 0;
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int PPObjGlobalUserAcc::GetPacket(PPID id, PPGlobalUserAccPacket * pPack)
{
	int    ok = Search(id, &pPack->Rec);
	if(ok > 0) {
		pPack->TagL.Destroy();
		THROW(P_Ref->Ot.GetList(Obj, id, &pPack->TagL));
		// @v11.9.9 {
		{
			SString text_buf;
			THROW(P_Ref->UtrC.GetText(TextRefIdent(Obj, id, PPTRPROP_GUAEXT), text_buf));
			text_buf.Transf(CTRANSF_UTF8_TO_INNER);
			pPack->SetBuffer(text_buf.Strip());
		}
		// } @v11.9.9 
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjGlobalUserAcc::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjGlobalUserAcc, PPGlobalUserAccPacket>(this, p, id, stream, pCtx); }

int PPObjGlobalUserAcc::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1, ta = 0;
	if(p && p->Data) {
		PPGlobalUserAccPacket * p_pack = static_cast<PPGlobalUserAccPacket *>(p->Data);
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				if(P_Ref->SearchSymb(Obj, &same_id, p_pack->Rec.Name, offsetof(PPGlobalUserAcc, Name)) > 0) {
					PPGlobalUserAcc same_rec;
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

int PPObjGlobalUserAcc::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPGlobalUserAccPacket * p_pack = static_cast<PPGlobalUserAccPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_PERSON, &p_pack->Rec.PersonID, ary, replace));
		THROW(p_pack->TagL.ProcessObjRefs(ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

class VkGuaCfgDialog : public TDialog {
	//DECL_DIALOG_DATA(VkStruct);
	DECL_DIALOG_DATA(VkInterface::InitBlock);
public:
	VkGuaCfgDialog() : TDialog(DLG_VKGUACFG)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		PPGlobalUserAccConfig cfg;
		GuaObj.FetchConfig(&cfg);
		RVALUEPTR(Data, pData);
		// нужно установить поле группа
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		SString temp_buf;
		//нужно вернуть данные о groupid, ownerid and token
		ASSIGN_PTR(pData, Data);
		//CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		SString temp_buf;
		if(event.isCmd(cmQueryToken)) {
			VkInterface ifc;
			ifc.GetVKAccessToken();
		}
		else if(event.isCmd(cmInputUpdated)&&event.isCtlEvent(CTL_VKGUACFG_LOGIN_URL)) {
			UI_LOCAL_LOCK_ENTER
				getCtrlString(CTL_VKGUACFG_LOGIN_URL, temp_buf.Z());
				{
					InetUrl url(temp_buf);
					url.GetComponent(InetUrl::cRef, 0, temp_buf);
					SString left;
					SString right;
					StringSet ss('&', temp_buf);
					for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
						temp_buf.Divide('=', left, right);
						if(left.IsEq("access_token"))
							Data.CliAccsKey.Z().Cat(right);
						else if(left.IsEq("user_id"))
							Data.PageId.Z().Cat(right);
					}
				}
			UI_LOCAL_LOCK_LEAVE
		}
		else if(event.isCmd(cmInputUpdated)&&event.isCtlEvent(CTL_VKGUACFG_GROUP_ID)) {
			UI_LOCAL_LOCK_ENTER
				getCtrlString(CTL_VKGUACFG_GROUP_ID, temp_buf.Z());
				Data.GroupId.Z().Cat(temp_buf);
			UI_LOCAL_LOCK_LEAVE
		}
		else
			return;
		clearEvent(event);
	}
	PPObjGlobalUserAcc GuaObj;
};

class GlobalUserAccDialog : public TDialog{
	DECL_DIALOG_DATA(PPGlobalUserAccPacket);
public:
	GlobalUserAccDialog() : TDialog(DLG_GLOBUSERACC)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		PPGlobalUserAccConfig cfg;
		GuaObj.FetchConfig(&cfg);
		RVALUEPTR(Data, pData);
		SString temp_buf;
		setCtrlData(CTL_GLOBUSERACC_NAME, Data.Rec.Name);
		setCtrlData(CTL_GLOBUSERACC_SYMB, Data.Rec.Symb); // @v10.1.6
		setCtrlLong(CTL_GLOBUSERACC_ID, Data.Rec.ID);
		disableCtrl(CTL_GLOBUSERACC_ID, 1);
		Data.Rec.LocalDbUuid.ToStr(S_GUID::fmtIDL, temp_buf);
		setCtrlString(CTL_GLOBUSERACC_DBUUID, temp_buf);
		setCtrlLong(CTL_GLOBUSERACC_LOCALID, Data.Rec.LocalUserID);
		enableCommand(cmOK, GuaObj.CheckRights(PPR_MOD));
		SetupPPObjCombo(this, CTLSEL_GLOBUSERACC_PSN, PPOBJ_PERSON, Data.Rec.PersonID, 0, reinterpret_cast<void *>(cfg.PersonKindID));
		SetupStringCombo(this, CTLSEL_GLOBUSERACC_SVC, PPTXT_GLOBALSERVICELIST, Data.Rec.ServiceIdent); // @v10.5.5
		//@erik {
		if(!Data.Rec.ServiceIdent) {
			disableCtrl(CTL_GS_SETTING, 1);
		}
		// } @erik
		AddClusterAssoc(CTL_GLOBUSERACC_FLAGS, 0, PPGlobalUserAcc::fSandBox); // @v10.6.3
		SetClusterData(CTL_GLOBUSERACC_FLAGS, Data.Rec.Flags); // @v10.6.3
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		SString temp_buf;
		getCtrlString(sel = CTL_GLOBUSERACC_NAME, temp_buf);
		THROW_PP(temp_buf.NotEmptyS(), PPERR_NAMENEEDED);
		temp_buf.CopyTo(Data.Rec.Name, sizeof(Data.Rec.Name));
		getCtrlString(sel = CTL_GLOBUSERACC_SYMB, temp_buf); // @v10.1.6
		temp_buf.CopyTo(Data.Rec.Symb, sizeof(Data.Rec.Symb)); // @v10.1.6
		getCtrlString(sel = CTL_GLOBUSERACC_DBUUID, temp_buf);
		THROW_SL(Data.Rec.LocalDbUuid.FromStr(temp_buf));
		Data.Rec.LocalUserID = getCtrlLong(CTL_GLOBUSERACC_LOCALID);
		THROW(GuaObj.CheckName(Data.Rec.ID, Data.Rec.Name, 1));
		getCtrlData(CTLSEL_GLOBUSERACC_PSN, &Data.Rec.PersonID);
		//getCtrlData(CTLSEL_GLOBUSERACC_SVC, &Data.Rec.ServiceIdent); // @v10.5.5 =>  To comment @erik @v10.7.13
		GetClusterData(CTL_GLOBUSERACC_FLAGS, &Data.Rec.Flags); // @v10.6.3
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		SString temp_buf;
		PPLogger logger;
		PPObjTag obj_tag;
		Reference ref;
		if(event.isCmd(cmCfgPassword)) {
			//
				//SString pwd;
				//Reference::Decrypt(Reference::crymRef2, Data.Rec.Password, sstrlen(Data.Rec.Password), pwd);
			//
			PasswordDialog(0, Data.Rec.Password, sizeof(Data.Rec.Password), 3, 0);
		}
		else if(event.isCmd(cmTags)) {
			Data.TagL.ObjType = PPOBJ_GLOBALUSERACC;
			EditObjTagValList(&Data.TagL, 0);
		}
		else if(event.isCmd(cmGuaExtList)) {
			EditExtList();
		}
		else if(event.isCmd(cmGlobalServiceSetting)) {
			PPID service_ident = Data.Rec.ServiceIdent;
			switch(service_ident) {
				case PPGLS_TWITTER:
					break;
				case PPGLS_FACEBOOK:
					break;
				case PPGLS_VK:
					{			
						uint change_flag = 0;
						SString temp_buf;
						PPLogger logger;
						VkInterface::InitBlock pack;
						VkGuaCfgDialog * dlg = new VkGuaCfgDialog();
						dlg->setDTS(&pack);
						if(ExecView(dlg) == cmOK) {
							if(dlg->getDTS(&pack)) {
								if(pack.GroupId.Len() > 0) {
									Data.TagL.PutItemStr(PPTAG_GUA_SOCIALGROUPCODE, pack.GroupId.Unescape()); // @v11.9.11
									/* @v11.9.11
									ObjTagItem new_obj_tag_item;
									new_obj_tag_item.SetStr(PPTAG_GUA_SOCIALGROUPCODE, pack.GroupId.Unescape());
									if(ref.Ot.PutTag(PPOBJ_GLOBALUSERACC, Data.Rec.ID, &new_obj_tag_item, 1) > 0)
										change_flag++;
									*/
								}
								if(pack.PageId.Len() > 0) {
									Data.TagL.PutItemStr(PPTAG_GUA_SOCIALPAGECODE, pack.PageId.Unescape()); // @v11.9.11
									/* @v11.9.11
									ObjTagItem new_obj_tag_item;
									new_obj_tag_item.SetStr(PPTAG_GUA_SOCIALPAGECODE, pack.PageId.Unescape());
									if(ref.Ot.PutTag(PPOBJ_GLOBALUSERACC, Data.Rec.ID, &new_obj_tag_item, 1) > 0) // @v10.8.3 @fix (remove ';')
										change_flag++;
									*/
								}
								if(pack.CliAccsKey.Len()) {
									Data.SetAccessKey(pack.CliAccsKey.Unescape()); // @v11.9.11
									/* @v11.9.11
									ObjTagItem new_obj_tag_item;
									new_obj_tag_item.SetStr(PPTAG_GUA_ACCESSKEY, pack.CliAccsKey.Unescape());
									if(ref.Ot.PutTag(PPOBJ_GLOBALUSERACC, Data.Rec.ID, &new_obj_tag_item, 1) > 0)
										change_flag++;
									*/
								}
								/* @v11.9.11
								if(change_flag > 0)
									ref.Ot.GetList(PPOBJ_GLOBALUSERACC, Data.Rec.ID, &Data.TagL);
								*/
							}
						}
						delete dlg;
					}
					break;
				case PPGLS_VETIS:
					break;
				case PPGLS_CHZN:
					break;
				default:
					break;
			}
		}
		//@erik @v10.7.13{
		else if(event.isCbSelected(CTLSEL_GLOBUSERACC_SVC)) {
			getCtrlData(CTLSEL_GLOBUSERACC_SVC, &Data.Rec.ServiceIdent);
			disableCtrl(CTL_GS_SETTING, !Data.Rec.ServiceIdent);
		}
		// } @erik @v10.7.13
		else
			return;
		clearEvent(event);
	}
	int    EditExtList()
	{
		int    ok = -1;
		static const SIntToSymbTabEntry ext_list[] = {
			// Функционально поле PPGlobalUserAccPacket::extssAccessKey замещает тег PPTAG_GUA_ACCESSKEY, по этому и наименование будет использовать то же самое
			{ PPGlobalUserAccPacket::extssAccessKey, "rtag_guaaccesskey" }, 
		};
		ExtStrContainerListDialog * dlg = new ExtStrContainerListDialog(DLG_CCEXTLIST, CTL_CCEXTLIST_LIST, "@extfldlist", false, ext_list, SIZEOFARRAY(ext_list));
		if(CheckDialogPtrErr(&dlg)) {
			dlg->setDTS(&Data);
			if(ExecView(dlg) == cmOK) {
				;
			}
		}
		delete dlg;
		return ok;
	}
	PPObjGlobalUserAcc GuaObj;
};

int PPObjGlobalUserAcc::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	bool   is_new = false;
	PPGlobalUserAccPacket pack;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	ok = PPDialogProcBody <GlobalUserAccDialog, PPGlobalUserAccPacket> (&pack);
	THROW(ok);
	if(ok > 0) {
		THROW(PutPacket(pID, &pack, 1));
		ok = cmOK;
	}
	CATCHZOKPPERR
	return ok;
}

/*virtual*/StrAssocArray * PPObjGlobalUserAcc::MakeStrAssocList(void * extraPtr /*ServiceIdent*/)
{
	const  long service_ident = reinterpret_cast<long>(extraPtr);
	PPID   id = 0;
	StrAssocArray * p_list = new StrAssocArray();
	THROW_MEM(p_list);
	{
		PPGlobalUserAcc rec;
		for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
			if(!service_ident || rec.ServiceIdent == service_ident) {
				if(*strip(rec.Name) == 0)
					ideqvalstr(rec.ID, rec.Name, sizeof(rec.Name));
				THROW_SL(p_list->Add(rec.ID, 0, rec.Name));
			}
		}
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjGlobalUserAcc::SearchByLocalID(const S_GUID & rDbUuid, PPID localUserID, PPID * pID, PPGlobalUserAcc * pRec)
{
	int    ok = -1;
	PPID   id = 0;
	PPGlobalUserAcc rec;
	for(SEnum en = P_Ref->EnumByIdxVal(Obj, 1, localUserID); ok < 0 && en.Next(&rec) > 0;) {
		if(rec.LocalUserID == localUserID && rec.LocalDbUuid == rDbUuid) {
			id = rec.ID;
			ok = 1;
		}
	}
	ASSIGN_PTR(pID, id);
	ASSIGN_PTR(pRec, rec);
	return ok;
}

int PPObjGlobalUserAcc::GetListByServiceIdent(PPID serviceIdent, PPIDArray * pList)
{
	int    ok = -1;
	PPGlobalUserAcc gua_rec;
	if(serviceIdent) {
		for(SEnum en = Enum(0); en.Next(&gua_rec) > 0;) {
			if(gua_rec.ServiceIdent == serviceIdent) {
				CALLPTRMEMB(pList, add(gua_rec.ID));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPObjGlobalUserAcc::SearchByAssociatedPersonID(PPID personID, PPIDArray & rList)
{
	int    ok = -1;
	rList.clear();
	if(personID > 0) {
		PPGlobalUserAcc rec;
		for(SEnum en = P_Ref->EnumByIdxVal(Obj, 2, personID); en.Next(&rec) > 0;) {
			if(rec.PersonID == personID) {
				rList.add(rec.ID);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPObjGlobalUserAcc::CheckPassword(const char * pName, const char * pPassword, PPGlobalUserAcc * pUserAcc /* =0 */)
{
	int    ok = 0;
	PPID   gua_id = 0;
	PPGlobalUserAcc gua_rec;
	SString pwd;
	THROW_INVARG(pName && pPassword);
	if(SearchByName(pName, &gua_id, &gua_rec) > 0) {
		pwd.Z();
		Reference::Decrypt(Reference::crymRef2, gua_rec.Password, sstrlen(gua_rec.Password), pwd);
		if(pwd.CmpNC(pPassword) == 0) {
			ASSIGN_PTR(pUserAcc, gua_rec);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

static SString & MakeMsgStr(int fmtId, const char * pName, const char * pUuid, PPID localId, SString & rBuf)
{
	rBuf.Z();
	SString fmt_buf, temp_buf, info_buf;
	if(fmtId)
		PPLoadText(fmtId, fmt_buf);
	PPLoadString("name", temp_buf);
	info_buf.CatEq(temp_buf, pName);
	info_buf.CatDiv(',', 2).CatEq("GUID", pUuid);
	PPLoadString("id", temp_buf);
	info_buf.CatDiv(',', 2).CatEq(temp_buf, localId);
	if(fmtId)
		rBuf.Printf(fmt_buf, (const char *)info_buf);
	else
		rBuf = info_buf;
	return rBuf;
}

int PPObjGlobalUserAcc::Register(PPID & ID, const char * pName, const char * pPassword, const S_GUID & rDbUuid, PPID localUserID, PPID personID)
{
	int    ok = 0;
	SString s_guid, msg;
	PPGlobalUserAccConfig cfg;
	PPObjSCard * p_sc_obj = 0;
	PPObjSCardSeries * p_scs_obj = 0;
	rDbUuid.ToStr(S_GUID::fmtIDL, s_guid);
	THROW_PP_S(!isempty(pName) && !isempty(pPassword) && s_guid.Len(),
		PPERR_GLBUSERACCT_REGFAILED, MakeMsgStr(0, pName, s_guid, localUserID, msg));
	{
		int    found = 0;
		PPGlobalUserAcc glb_acc;
		if(SearchByLocalID(rDbUuid, localUserID, 0, &glb_acc) <= 0) {
			for(PPID id = 0; !found && EnumItems(&id, &glb_acc) > 0;)
				if(stricmp866(glb_acc.Name, pName) == 0)
					found = 1;
		}
		else
			found = 1;
		THROW_PP_S(!found, PPERR_GLBUSERACCT_ALREADYEXISTS, MakeMsgStr(0, pName, s_guid, localUserID, msg));
		if(FetchConfig(&cfg) > 0 && cfg.SCardSerID) {
			THROW_MEM(p_sc_obj = new PPObjSCard);
			THROW_MEM(p_scs_obj = new PPObjSCardSeries);
		}
		if(p_sc_obj) {
			PPTransaction tra(1);
			THROW(tra);
			MEMSZERO(glb_acc);
			glb_acc.Tag  = PPOBJ_GLOBALUSERACC;
			STRNSCPY(glb_acc.Name, pName);
			glb_acc.LocalDbUuid = rDbUuid;
			Reference::Encrypt(Reference::crymRef2, pPassword, glb_acc.Password, sizeof(glb_acc.Password));
			glb_acc.LocalUserID = localUserID;
			glb_acc.PersonID = personID;
			THROW(StoreItem(Obj, 0, &glb_acc, 0));
			ID = reinterpret_cast<const ReferenceTbl::Rec *>(&glb_acc)->ObjID;
			if(p_scs_obj) {
				PPSCardSerPacket scs_pack;
				if(p_scs_obj->GetPacket(cfg.SCardSerID, &scs_pack) > 0 && scs_pack.Rec.GetType() == scstCredit && scs_pack.Eb.CodeTempl[0]) {
					PPID   sc_id = 0;
					SString sc_code;
					THROW(p_sc_obj->Create_(&sc_id, scs_pack.Rec.ID, personID, 0, sc_code, 0, PPObjSCard::cdfCreditCard, 0));
				}
			}
			THROW(tra.Commit());
			PPLogMessage((const char *)0, MakeMsgStr(PPTXT_GLBUSERACCT_REGSUCCESS, pName, s_guid, localUserID, msg), LOGMSGF_TIME|LOGMSGF_USER);
			ok = 1;
		}
	}
	CATCHZOK
	delete p_sc_obj;
	delete p_scs_obj;
	return ok;
}

/*virtual*/int  PPObjGlobalUserAcc::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	PPGlobalUserAcc rec;
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == Obj) {
			GlobalBizScoreTbl bizsc_tbl;
			deleteFrom(&bizsc_tbl, 0, bizsc_tbl.GlobalUserID == _id);
		}
		else if(_obj == PPOBJ_PERSON) {
			for(SEnum en = P_Ref->Enum(Obj, 0); ok && en.Next(&rec) > 0;) {
				if(rec.PersonID == _id) {
					ok = RetRefsExistsErr(Obj, 0);
				}
			}
		}
	}
	else if(msg == DBMSG_OBJREPLACE) { // @v11.0.10
		const  PPID new_id = reinterpret_cast<long>(extraPtr);
		if(_obj == PPOBJ_PERSON) {
			PPIDArray id_to_upd_list;
			for(SEnum en = P_Ref->Enum(Obj, 0); ok && en.Next(&rec) > 0;) {
				if(rec.PersonID == _id) {
					id_to_upd_list.add(rec.ID);
				}
			}
			id_to_upd_list.sortAndUndup();
			for(uint i = 0; i < id_to_upd_list.getCount(); i++) {
				const  PPID gua_id = id_to_upd_list.get(i);
				if(Search(gua_id, &rec) > 0 && rec.PersonID == _id) {
					rec.PersonID = new_id;
					THROW(P_Ref->UpdateItem(Obj, gua_id, &rec, 1, 0));				
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

class GlobalUserAccCache : public ObjCache {
public:
	GlobalUserAccCache() : ObjCache(PPOBJ_GLOBALUSERACC, sizeof(GlobalUserAccData))
	{
		MEMSZERO(Cfg);
		AlbCfg.Hdr.Prop = 0; // @v10.6.3 @paranoic
	}
	int    GetConfig(PPGlobalUserAccConfig * pCfg, int enforce);
	int    GetAlbatrossConfig(PPAlbatrossConfig * pCfg, int enforce);
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;

	PPGlobalUserAccConfig Cfg;
	PPAlbatrossConfig AlbCfg; // @v10.6.3
	ReadWriteLock CfgLock;
	ReadWriteLock AlbCfgLock; // @v10.6.3
public:
	struct GlobalUserAccData : public ObjCacheEntry {
		S_GUID LocalDbUuid;
		long   Flags;
		long   LocalUserID;
		PPID   PersonID;
		PPID   ServiceIdent; // @v10.5.5
	};
};

int GlobalUserAccCache::GetConfig(PPGlobalUserAccConfig * pCfg, int enforce)
{
	{
		SRWLOCKER(CfgLock, SReadWriteLocker::Read);
		if(!(Cfg.Flags & PPGlobalUserAccConfig::fValid) || enforce) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!(Cfg.Flags & PPGlobalUserAccConfig::fValid) || enforce) {
				PPObjGlobalUserAcc::ReadConfig(&Cfg);
				Cfg.Flags |= PPGlobalUserAccConfig::fValid;
			}
		}
		ASSIGN_PTR(pCfg, Cfg);
	}
	return 1;
}

int GlobalUserAccCache::GetAlbatrossConfig(PPAlbatrossConfig * pCfg, int enforce)
{
	{
		SRWLOCKER(AlbCfgLock, SReadWriteLocker::Read);
		if(AlbCfg.Hdr.Tag != PPPRP_ALBATROSCFG2 || enforce) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(AlbCfg.Hdr.Tag != PPPRP_ALBATROSCFG2 || enforce) {
				PPAlbatrosCfgMngr::Get(&AlbCfg);
				AlbCfg.Hdr.Tag = PPPRP_ALBATROSCFG2;
			}
		}
		ASSIGN_PTR(pCfg, AlbCfg);
	}
	return 1;
}

int GlobalUserAccCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	GlobalUserAccData * p_cache_rec = static_cast<GlobalUserAccData *>(pEntry);
	PPObjGlobalUserAcc gua_obj;
	PPGlobalUserAcc rec;
	if(gua_obj.Search(id, &rec) > 0) {
		p_cache_rec->LocalDbUuid = rec.LocalDbUuid;
		p_cache_rec->Flags = rec.Flags;
		p_cache_rec->LocalUserID = rec.LocalUserID;
		p_cache_rec->PersonID = rec.PersonID;
		p_cache_rec->ServiceIdent = rec.ServiceIdent; // @v10.5.5

		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Symb);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void GlobalUserAccCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPGlobalUserAcc * p_data_rec = static_cast<PPGlobalUserAcc *>(pDataRec);
	const GlobalUserAccData * p_cache_rec = static_cast<const GlobalUserAccData *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->Tag = PPOBJ_GLOBALUSERACC;
	p_data_rec->ID  = p_cache_rec->ID;
	p_data_rec->LocalDbUuid = p_cache_rec->LocalDbUuid;
	p_data_rec->Flags       = p_cache_rec->Flags;
	p_data_rec->LocalUserID = p_cache_rec->LocalUserID;
	p_data_rec->PersonID    = p_cache_rec->PersonID;
	p_data_rec->ServiceIdent = p_cache_rec->ServiceIdent; // @v10.5.5

	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
}
// }

int PPObjGlobalUserAcc::Fetch(PPID id, PPGlobalUserAcc * pRec)
{
	GlobalUserAccCache * p_cache = GetDbLocalCachePtr <GlobalUserAccCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}

int FASTCALL PPObjGlobalUserAcc::FetchConfig(PPGlobalUserAccConfig * pCfg)
{
	GlobalUserAccCache * p_cache = GetDbLocalCachePtr <GlobalUserAccCache> (Obj);
	if(p_cache) {
		return p_cache->GetConfig(pCfg, 0);
	}
	else {
		memzero(pCfg, sizeof(*pCfg));
		return 0;
	}
}

int FASTCALL PPObjGlobalUserAcc::FetchAlbatossConfig(PPAlbatrossConfig * pCfg)
{
	GlobalUserAccCache * p_cache = GetDbLocalCachePtr <GlobalUserAccCache> (Obj);
	return p_cache ? p_cache->GetAlbatrossConfig(pCfg, 0) : PPAlbatrosCfgMngr::Get(pCfg);
}

int PPObjGlobalUserAcc::DirtyConfig()
{
	GlobalUserAccCache * p_cache = GetDbLocalCachePtr <GlobalUserAccCache> (Obj);
	return p_cache ? p_cache->GetConfig(0, 1) : 0;
}

#if 0 // {
int PPObjGlobalUserAcc::Unregister(const char * pName, const char * pPassword)
{
}

int PPObjGlobalUserAcc::ChangePassword(const char * pName, const char * pOldPassword, const char * pNewPassword)
{
}
#endif // } 0
//
//
//
PPObjSysAction::PPObjSysAction(void * extraPtr) : PPObjReference(PPOBJ_ACTION, extraPtr)
{
}

int PPObjSysAction::Edit(PPID * pID, void * extraPtr)
{
	return -1;
}

int PPObjSysAction::Remove(PPID id, long, uint options = PPObject::rmv_default)
{
	return -1;
}

int PPObjSysAction::Search(PPID id, void * pRec)
{
	int    ok = -1;
	SString temp_buf;
	if(PPLoadString(PPSTR_ACTION, id, temp_buf)) {
		NameBuf = temp_buf;
		if(pRec) {
			ReferenceTbl::Rec rec;
			rec.ObjType = Obj;
			rec.ObjID = id;
			temp_buf.CopyTo(rec.ObjName, sizeof(rec.ObjName));
			*static_cast<ReferenceTbl::Rec *>(pRec) = rec;
		}
		ok = 1;
	}
	else
		ideqvalstr(id, NameBuf.Z());
	return ok;
}

const char * PPObjSysAction::GetNamePtr()
{
	return NameBuf.cptr();
}

StrAssocArray * PPObjSysAction::MakeStrAssocList(void * extraPtr)
{
	PPID   id = 0;
	SString temp_buf;
	StrAssocArray * p_list = new StrAssocArray();
	THROW_MEM(p_list);
	for(long i = 1; i <= PPACN_LAST; i++) {
		if(PPLoadString(PPSTR_ACTION, i, temp_buf))
			THROW_SL(p_list->Add(i, temp_buf));
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;

}
//
//
//
PPObjGtaAction::PPObjGtaAction(void * extraPtr) : PPObjReference(PPOBJ_GTACTION, extraPtr)
{
}

int PPObjGtaAction::Edit(PPID * pID, void * extraPtr)
{
	return -1;
}

int PPObjGtaAction::Remove(PPID id, long, uint options = PPObject::rmv_default)
{
	return -1;
}

int PPObjGtaAction::Search(PPID id, void * pRec)
{
	int    ok = -1;
	SString temp_buf;
	if(PPLoadString(PPSTR_GTA, id, temp_buf)) {
		NameBuf = temp_buf;
		if(pRec) {
			ReferenceTbl::Rec rec;
			rec.ObjType = Obj;
			rec.ObjID = id;
			temp_buf.CopyTo(rec.ObjName, sizeof(rec.ObjName));
			*static_cast<ReferenceTbl::Rec *>(pRec) = rec;
		}
		ok = 1;
	}
	else
		ideqvalstr(id, NameBuf.Z());
	return ok;
}

const char * PPObjGtaAction::GetNamePtr()
{
	return NameBuf.cptr();
}

StrAssocArray * PPObjGtaAction::MakeStrAssocList(void * extraPtr)
{
	PPID   id = 0;
	SString temp_buf;
	StrAssocArray * p_list = new StrAssocArray();
	THROW_MEM(p_list);
	for(long i = 1; i <= GTAOP_LAST; i++) {
		if(PPLoadString(PPSTR_GTA, i, temp_buf))
			THROW_SL(p_list->Add(i, temp_buf));
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}
//
// @ModuleDecl(PPObjUhttStore)
//
PPUhttStorePacket::PPUhttStorePacket()
{
	destroy();
}

PPUhttStorePacket::~PPUhttStorePacket()
{
	destroy();
}

int FASTCALL PPUhttStorePacket::IsEq(const PPUhttStorePacket & rS) const
{
#define NRECFLD(f) if(Rec.f != rS.Rec.f) return 0
	NRECFLD(ID);
	NRECFLD(Tag);
	NRECFLD(LocID);
	NRECFLD(Flags);
	NRECFLD(PersonID);
	NRECFLD(UpRestShowThreshold);
	if(!sstreq(Rec.Name, rS.Rec.Name))
		return 0;
	if(!sstreq(Rec.Symb, rS.Rec.Symb))
		return 0;
	if(!TagL.IsEq(rS.TagL))
		return 0;
	if(!Sd.IsEq(rS.Sd))
		return 0;
	return 1;
#undef NRECFLD
}

void PPUhttStorePacket::destroy()
{
	MEMSZERO(Rec);
	TagL.Destroy();
	Sd.Destroy();
}

int PPUhttStorePacket::GetSelectorListInfo(PPID clsID, StrAssocArray & rList) const
{
	int    ok = -1;
	rList.Z();
	SString title_buf;
	PPUhttStoreSelDescr::Entry entry;
	for(uint i = 0; i < Sd.GetCount(); i++) {
		if(Sd.GetEntry(i, entry)) {
			long   key = i+1;
			if(entry.Attr == PPUhttStoreSelDescr::attrClass && entry.ClsID != clsID) {
				key = 0;
			}
			if(key > 0) {
				title_buf = entry.Title;
				if(!title_buf.NotEmptyS())
					Sd.GetDefaultEntryTitle(i, title_buf);
				rList.Add(key, title_buf);
				ok = 1;
			}
		}
	}
	return ok;
}

PPObjUhttStore::PPObjUhttStore(void * extraPtr) : PPObjReference(PPOBJ_UHTTSTORE, extraPtr)
{
}

IMPL_DESTROY_OBJ_PACK(PPObjUhttStore, PPUhttStorePacket);

int PPObjUhttStore::Browse(void * extraPtr)
{
	int    ok = -1;
	ObjViewDialog * p_dlg = 0;
	if(CheckRights(PPR_READ) && CheckDialogPtr(&(p_dlg = new ObjViewDialog(DLG_OBJVIEW, this, extraPtr)))) {
		ExecViewAndDestroy(p_dlg);
		ok = 1;
	}
	else
		ok = PPErrorZ();
	return ok;
}

class UhttStoreDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPUhttStorePacket);
public:
	UhttStoreDialog() : PPListDialog(DLG_UHTTSTORE, CTL_UHTTSTORE_SLIST)
	{
		updateList(-1);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString temp_buf;
		setCtrlLong(CTL_UHTTSTORE_ID, Data.Rec.ID);
		disableCtrl(CTL_GLOBUSERACC_ID, 1);
		setCtrlData(CTL_UHTTSTORE_NAME, Data.Rec.Name);
		setCtrlData(CTL_UHTTSTORE_SYMB, Data.Rec.Symb);
		SetupPersonCombo(this, CTLSEL_UHTTSTORE_PSN, Data.Rec.PersonID, OLW_CANINSERT, (PPID)PPPRK_SUPPL, 0);
		PsnObj.SetupDlvrLocCombo(this, CTLSEL_UHTTSTORE_LOC, Data.Rec.PersonID, Data.Rec.LocID);
		AddClusterAssocDef(CTL_UHTTSTORE_KIND,  0, PPUhttStore::kGenericStore);
		AddClusterAssoc(CTL_UHTTSTORE_KIND,  1, PPUhttStore::kTSessCip);
		SetClusterData(CTL_UHTTSTORE_KIND, Data.Rec.Kind);
		AddClusterAssoc(CTL_UHTTSTORE_FLAGS, 0, PPUhttStore::fUseGoodsPkg);
		AddClusterAssoc(CTL_UHTTSTORE_FLAGS, 1, PPUhttStore::fShowRest);
		AddClusterAssoc(CTL_UHTTSTORE_FLAGS, 2, PPUhttStore::fDontShowZeroRestItems);
		AddClusterAssoc(CTL_UHTTSTORE_FLAGS, 3, PPUhttStore::fCreateBuyerPerson);
		SetClusterData(CTL_UHTTSTORE_FLAGS, Data.Rec.Flags);
		setCtrlReal(CTL_UHTTSTORE_URST, Data.Rec.UpRestShowThreshold);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		SString temp_buf;
		getCtrlString(sel = CTL_UHTTSTORE_NAME, temp_buf);
		THROW_PP(temp_buf.NotEmptyS(), PPERR_NAMENEEDED);
		temp_buf.CopyTo(Data.Rec.Name, sizeof(Data.Rec.Name));
		{
			PPObjUhttStore uhs_obj;
			THROW(uhs_obj.CheckName(Data.Rec.ID, Data.Rec.Name, 1));
		}
		getCtrlString(sel = CTL_UHTTSTORE_SYMB, temp_buf);
		if(temp_buf.NotEmptyS())
			temp_buf.CopyTo(Data.Rec.Symb, sizeof(Data.Rec.Symb));
		Data.Rec.ID = getCtrlLong(CTL_UHTTSTORE_ID);
		getCtrlData(CTLSEL_UHTTSTORE_PSN, &Data.Rec.PersonID);
		getCtrlData(CTLSEL_UHTTSTORE_LOC, &Data.Rec.LocID);
		GetClusterData(CTL_UHTTSTORE_KIND, &Data.Rec.Kind);
		GetClusterData(CTL_UHTTSTORE_FLAGS, &Data.Rec.Flags);
		Data.Rec.UpRestShowThreshold = getCtrlReal(CTL_UHTTSTORE_URST);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmTags)) {
			Data.TagL.ObjType = PPOBJ_UHTTSTORE;
			EditObjTagValList(&Data.TagL, 0);
		}
		else if(event.isCbSelected(CTLSEL_UHTTSTORE_PSN)) {
		 	PPID   new_psn_id = getCtrlLong(CTLSEL_UHTTSTORE_PSN);
			if(new_psn_id != Data.Rec.PersonID) {
				Data.Rec.PersonID = new_psn_id;
				PsnObj.SetupDlvrLocCombo(this, CTLSEL_UHTTSTORE_LOC, Data.Rec.PersonID, 0);
			}
		}
		else
			return;
		clearEvent(event);
	}
	virtual int setupList()
	{
		SString temp_buf, name_buf;
		StringSet ss(SLBColumnDelim);
		PPUhttStoreSelDescr::Entry entry;
		for(uint i = 0; i < Data.Sd.GetCount(); i++) {
			if(Data.Sd.GetEntry(i, entry)) {
				ss.Z();
				switch(entry.Attr) {
					case PPUhttStoreSelDescr::attrGroup: PPLoadString("goodsgroup", temp_buf); break;
					case PPUhttStoreSelDescr::attrBrand: PPLoadString("brand", temp_buf); break;
					case PPUhttStoreSelDescr::attrName:  PPLoadString("appellation", temp_buf); break;
					case PPUhttStoreSelDescr::attrCity: PPLoadString("city", temp_buf); break;
					case PPUhttStoreSelDescr::attrProcessor: PPLoadString("processor", temp_buf); break;
					case PPUhttStoreSelDescr::attrPeriod: PPLoadString("daterange", temp_buf); break;
					case PPUhttStoreSelDescr::attrClass:
						PPLoadString("class", temp_buf);
						temp_buf.Colon();
						{
							PPGdsClsPacket gc_pack;
							if(GcObj.GetPacket(entry.ClsID, &gc_pack) > 0) {
								temp_buf.Cat(gc_pack.Rec.Name);
								if(gc_pack.GetPropName(entry.GcClsf, name_buf))
									temp_buf.Colon().Cat(name_buf);
							}
							else
								gc_pack.Rec.ID = 0;
						}
						break;
					case PPUhttStoreSelDescr::attrTag:
						PPLoadString("tag", temp_buf);
						temp_buf.Colon();
						{
							PPObjectTag tag_rec;
							if(TagObj.Fetch(entry.TagID, &tag_rec) > 0)
								temp_buf.Cat(tag_rec.Name);
							else
								temp_buf.Cat(ideqvalstr(entry.TagID, name_buf));
						}
						break;
				}
				ss.add(temp_buf);
				ss.add(entry.Title);
				addStringToList(i+1, ss.getBuf());
			}
		}
		return 1;
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		uint   pos = 0;
		PPUhttStoreSelDescr::Entry entry;
		while(ok < 0 && EditSelectorItem(&entry) > 0) {
			pos = 0;
			if(Data.Sd.AddEntry(&pos, &entry)) {
				ASSIGN_PTR(pPos, pos);
				ASSIGN_PTR(pID, pos+1);
				ok = 1;
			}
			else
				PPError();
		}
		return ok;
	}
	virtual int editItem(long pos, long id)
	{
		int    ok = -1;
		PPUhttStoreSelDescr::Entry entry;
		if(Data.Sd.GetEntry(pos, entry)) {
			while(ok < 0 && EditSelectorItem(&entry) > 0) {
				if(Data.Sd.UpdateEntry(pos, &entry))
					ok = 1;
				else
					PPError();
			}
		}
		return ok;
	}
	virtual int delItem(long pos, long id)
	{
		int    ok = -1;
		PPUhttStoreSelDescr::Entry entry;
		if(Data.Sd.GetEntry(pos, entry)) {
			if(Data.Sd.UpdateEntry(pos, 0))
				ok = 1;
			else
				PPError();
		}
		return ok;
	}
	int    EditSelectorItem(PPUhttStoreSelDescr::Entry * pData)
	{
		class StSelDialog : public TDialog {
			DECL_DIALOG_DATA(PPUhttStoreSelDescr::Entry);
		public:
			StSelDialog() : TDialog(DLG_UHTTSTSELITEM)
			{
			}
			DECL_DIALOG_SETDTS()
			{
				RVALUEPTR(Data, pData);
				int    ok = 1;
				AddClusterAssocDef(CTL_UHTTSTSELITEM_ATTR,  0, PPUhttStoreSelDescr::attrName);
				AddClusterAssoc(CTL_UHTTSTSELITEM_ATTR,  1, PPUhttStoreSelDescr::attrGroup);
				AddClusterAssoc(CTL_UHTTSTSELITEM_ATTR,  2, PPUhttStoreSelDescr::attrBrand);
				AddClusterAssoc(CTL_UHTTSTSELITEM_ATTR,  3, PPUhttStoreSelDescr::attrTag);
				AddClusterAssoc(CTL_UHTTSTSELITEM_ATTR,  4, PPUhttStoreSelDescr::attrClass);
				AddClusterAssoc(CTL_UHTTSTSELITEM_ATTR,  5, PPUhttStoreSelDescr::attrPeriod);
				AddClusterAssoc(CTL_UHTTSTSELITEM_ATTR,  6, PPUhttStoreSelDescr::attrProcessor);
				AddClusterAssoc(CTL_UHTTSTSELITEM_ATTR,  7, PPUhttStoreSelDescr::attrCity);
				SetClusterData(CTL_UHTTSTSELITEM_ATTR, Data.Attr);
				AddClusterAssoc(CTL_UHTTSTSELITEM_GCCLSF,  0, PPGdsCls::eX);
				AddClusterAssoc(CTL_UHTTSTSELITEM_GCCLSF,  1, PPGdsCls::eY);
				AddClusterAssoc(CTL_UHTTSTSELITEM_GCCLSF,  2, PPGdsCls::eZ);
				AddClusterAssoc(CTL_UHTTSTSELITEM_GCCLSF,  3, PPGdsCls::eW);
				AddClusterAssoc(CTL_UHTTSTSELITEM_GCCLSF,  4, PPGdsCls::eKind);
				AddClusterAssoc(CTL_UHTTSTSELITEM_GCCLSF,  5, PPGdsCls::eGrade);
				AddClusterAssoc(CTL_UHTTSTSELITEM_GCCLSF,  6, PPGdsCls::eAdd);
				AddClusterAssoc(CTL_UHTTSTSELITEM_GCCLSF,  7, PPGdsCls::eAdd2);
				SetupAttr();
				setCtrlString(CTL_UHTTSTSELITEM_TITLE, Data.Title);
				return ok;
			}
			DECL_DIALOG_GETDTS()
			{
				int    ok = 1;
				Data.Attr = GetClusterData(CTL_UHTTSTSELITEM_ATTR);
				if(Data.Attr == PPUhttStoreSelDescr::attrClass) {
					Data.ClsID = getCtrlLong(CTLSEL_UHTTSTSELITEM_EXT);
					Data.GcClsf = GetClusterData(CTL_UHTTSTSELITEM_GCCLSF);
				}
				else {
					Data.GcClsf = 0;
					if(Data.Attr == PPUhttStoreSelDescr::attrTag)
						Data.TagID = getCtrlLong(CTLSEL_UHTTSTSELITEM_EXT);
					else
						Data.TagID = 0;
				}
				getCtrlString(CTL_UHTTSTSELITEM_TITLE, Data.Title);
				ASSIGN_PTR(pData, Data);
				return ok;
			}
		private:
			DECL_HANDLE_EVENT
			{
				TDialog::handleEvent(event);
				if(event.isClusterClk(CTL_UHTTSTSELITEM_ATTR)) {
					int    preserve_attr = Data.Attr;
					Data.Attr = GetClusterData(CTL_UHTTSTSELITEM_ATTR);
					if(Data.Attr != preserve_attr)
						SetupAttr();
				}
				else if(event.isCbSelected(CTLSEL_UHTTSTSELITEM_EXT)) {
					PPID   preserve_ext_id = Data.TagID;
					Data.TagID = getCtrlLong(CTLSEL_UHTTSTSELITEM_EXT);
					if(Data.TagID != preserve_ext_id)
						SetupClass();
				}
				else
					return;
				clearEvent(event);
			}
			void   SetupClsPropClusterItem(const PPGdsClsPacket & rGcPack, int prop, int itemN)
			{
				SString name_buf;
				if(rGcPack.GetPropName(prop, name_buf)) {
					DisableClusterItem(CTL_UHTTSTSELITEM_GCCLSF, itemN, 0);
					SetClusterItemText(CTL_UHTTSTSELITEM_GCCLSF, itemN, name_buf);
				}
				else
					DisableClusterItem(CTL_UHTTSTSELITEM_GCCLSF, itemN, 1);
			}
			void   SetupClass()
			{
				if(Data.Attr == PPUhttStoreSelDescr::attrClass && Data.ClsID) {
					PPObjGoodsClass gc_obj;
					PPGdsClsPacket gc_pack;
					if(gc_obj.GetPacket(Data.ClsID, &gc_pack) > 0) {
						disableCtrl(CTL_UHTTSTSELITEM_GCCLSF, 0);
						SetupClsPropClusterItem(gc_pack, PPGdsCls::eX, 0);
						SetupClsPropClusterItem(gc_pack, PPGdsCls::eY, 1);
						SetupClsPropClusterItem(gc_pack, PPGdsCls::eZ, 2);
						SetupClsPropClusterItem(gc_pack, PPGdsCls::eW, 3);
						SetupClsPropClusterItem(gc_pack, PPGdsCls::eKind,  4);
						SetupClsPropClusterItem(gc_pack, PPGdsCls::eGrade, 5);
						SetupClsPropClusterItem(gc_pack, PPGdsCls::eAdd,   6);
						SetupClsPropClusterItem(gc_pack, PPGdsCls::eAdd2,  7);
						SetClusterData(CTL_UHTTSTSELITEM_GCCLSF, Data.GcClsf);
					}
					else
						disableCtrl(CTL_UHTTSTSELITEM_GCCLSF, 1);
				}
				else
					disableCtrl(CTL_UHTTSTSELITEM_GCCLSF, 1);
			}
			void   SetupAttr()
			{
				if(Data.Attr == PPUhttStoreSelDescr::attrTag) {
					disableCtrl(CTLSEL_UHTTSTSELITEM_EXT, 0);
					ObjTagFilt ot_filt(PPOBJ_GOODS);
					ot_filt.Flags |= ObjTagFilt::fOnlyTags;
					SetupObjTagCombo(this, CTLSEL_UHTTSTSELITEM_EXT, Data.TagID, 0, &ot_filt);
				}
				else if(Data.Attr == PPUhttStoreSelDescr::attrClass) {
					disableCtrl(CTLSEL_UHTTSTSELITEM_EXT, 0);
					SetupPPObjCombo(this, CTLSEL_UHTTSTSELITEM_EXT, PPOBJ_GOODSCLASS, Data.ClsID, 0, 0);
				}
				else
					disableCtrl(CTLSEL_UHTTSTSELITEM_EXT, 1);
				SetupClass();
			}
		};
		DIALOG_PROC_BODY(StSelDialog, pData);
	}

	PPObjPerson PsnObj;
	PPObjTag TagObj;
	PPObjGoodsClass GcObj;
};

int PPObjUhttStore::Edit(PPID * pID, void * extraPtr)
{
	int    r = cmCancel, ok = 1, valid_data = 0;
	PPUhttStorePacket pack;
	UhttStoreDialog * dlg = 0;
	THROW(CheckRightsModByID(pID));
	THROW(CheckDialogPtr(&(dlg = new UhttStoreDialog)));
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	dlg->setDTS(&pack);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		if(dlg->getDTS(&pack)) {
			if(*pID)
				*pID = pack.Rec.ID;
			if(PutPacket(pID, &pack, 1))
				valid_data = 1;
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int PPObjUhttStore::ValidatePacket(const PPUhttStorePacket * pPack, long flags)
{
	int    ok = 1;
	SString temp_buf;
	THROW_PP(pPack, PPERR_NULLPTR);
	THROW_PP(pPack->Rec.Name[0], PPERR_NAMENEEDED);
	THROW(CheckName(pPack->Rec.ID, pPack->Rec.Name, 1));
	CATCHZOK
	return ok;
}

int PPObjUhttStore::PutPacket(PPID * pID, PPUhttStorePacket * pPack, int use_ta)
{
	int    ok = 1;
	PPID   _id = DEREFPTRORZ(pID);
	const  int is_new = (_id == 0);
	const  int is_removing = BIN(_id != 0 && pPack == 0);
	PPID   hid = 0;
	PPUhttStorePacket org_pack;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(_id) {
			THROW(GetPacket(_id, &org_pack) > 0);
		}
		if(pPack == 0) {
			if(*pID) {
				THROW(CheckRights(PPR_DEL));
				THROW(P_Ref->RemoveItem(Obj, _id, 0));
				THROW(P_Ref->RemoveProperty(Obj, _id, 0, 0));
				THROW(P_Ref->Ot.PutList(Obj, _id, 0, 0));
				THROW(RemoveSync(_id));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, hid, 0);
			}
		}
		else {
			SSerializeContext sctx;
			SBuffer sd_buf;
			if(pPack->Sd.GetCount()) {
				THROW(pPack->Sd.Serialize(+1, sd_buf, &sctx));
			}
			THROW(CheckRightsModByID(pID));
			if(_id) {
				if(pPack->IsEq(org_pack))
					ok = -1;
				else {
					THROW(P_Ref->UpdateItem(Obj, _id, &pPack->Rec, 0, 0));
					THROW(P_Ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
					THROW(P_Ref->PutPropSBuffer(Obj, _id, UHTTSPRP_SELECTORS, sd_buf, 0));
					DS.LogAction(PPACN_OBJUPD, Obj, _id, 0, 0);
				}
			}
			else {
				THROW(P_Ref->AddItem(Obj, &_id, &pPack->Rec, 0));
				pPack->Rec.ID = _id;
				THROW(P_Ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
				THROW(P_Ref->PutPropSBuffer(Obj, _id, UHTTSPRP_SELECTORS, sd_buf, 0));
				DS.LogAction(PPACN_OBJADD, Obj, _id, 0, 0);
				ASSIGN_PTR(pID, _id);
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		if(is_new) {
			*pID = 0;
			if(pPack)
				pPack->Rec.ID = 0;
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int PPObjUhttStore::GetPacket(PPID id, PPUhttStorePacket * pPack)
{
	int    ok = Search(id, &pPack->Rec);
	if(ok > 0) {
		SBuffer sd_buf;
		pPack->TagL.Destroy();
		pPack->Sd.Destroy();
		THROW(P_Ref->Ot.GetList(Obj, id, &pPack->TagL));
		THROW(P_Ref->GetPropSBuffer(Obj, id, UHTTSPRP_SELECTORS, sd_buf));
		{
			size_t sd_size = sd_buf.GetAvailableSize();
			if(sd_size) {
				// @v10.2.11 STempBuffer temp_buf(sd_size);
				SSerializeContext sctx;
				// @v10.2.11 THROW_SL(temp_buf.IsValid());
				//THROW(pPack->Sd.Serialize(-1, sd_buf, &sctx));
				pPack->Sd.Serialize(-1, sd_buf, &sctx);
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjUhttStore::SerializePacket(int dir, PPUhttStorePacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW(pPack->TagL.Serialize(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int PPObjUhttStore::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjUhttStore, PPUhttStorePacket>(this, p, id, stream, pCtx); }

int PPObjUhttStore::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1, ta = 0;
	if(p && p->Data) {
		PPUhttStorePacket * p_pack = static_cast<PPUhttStorePacket *>(p->Data);
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				if(P_Ref->SearchSymb(Obj, &same_id, p_pack->Rec.Name, offsetof(PPUhttStore, Name)) > 0) {
					PPUhttStore same_rec;
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

int PPObjUhttStore::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPUhttStorePacket * p_pack = static_cast<PPUhttStorePacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_LOCATION, &p_pack->Rec.LocID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSON, &p_pack->Rec.PersonID, ary, replace));
		THROW(p_pack->TagL.ProcessObjRefs(ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjUhttStore::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		PPUhttStore us_rec;
		if(_obj == PPOBJ_PERSON) {
			SEnum en = P_Ref->EnumByIdxVal(Obj, 1, _id);
			if(en.Next(&us_rec) > 0) {
				ok = RetRefsExistsErr(Obj, us_rec.ID);
			}
		}
		else if(_obj == PPOBJ_LOCATION) {
			SEnum en = P_Ref->EnumByIdxVal(Obj, 2, _id);
			if(en.Next(&us_rec) > 0) {
				ok = RetRefsExistsErr(Obj, us_rec.ID);
			}
		}
	}
	else if(msg == DBMSG_OBJREPLACE) {
		if(_obj == PPOBJ_PERSON) {
			PPUhttStore us_rec;
			PPIDArray id_list;
			for(SEnum en = P_Ref->EnumByIdxVal(Obj, 1, _id); en.Next(&us_rec) > 0;) {
				id_list.addUnique(us_rec.ID);
			}
			for(uint i = 0; i < id_list.getCount(); i++) {
				if(Search(id_list.get(i), &us_rec) > 0) {
					if(us_rec.PersonID == _id) { // @paranoic
						us_rec.PersonID = reinterpret_cast<long>(extraPtr);
						if(!UpdateItem(us_rec.ID, &us_rec, 0)) {
							ok = DBRPL_ERROR;
							break;
						}
					}
				}
			}
		}
	}
	return ok;
}
//
//
//
PPUhttStoreSelDescr::Entry::Entry() : Attr(0), GcClsf(0), ClsID(0)
{
}

PPUhttStoreSelDescr::PPUhttStoreSelDescr() : SStrGroup()
{
	Ver = DS.GetVersion();
}

PPUhttStoreSelDescr & PPUhttStoreSelDescr::Destroy()
{
	L.clear();
	ClearS();
	return *this;
}

uint PPUhttStoreSelDescr::GetCount() const
{
	return L.getCount();
}

int FASTCALL PPUhttStoreSelDescr::IsEq(const PPUhttStoreSelDescr & rS) const
{
	int    eq = 1;
	const  uint c = L.getCount();
	if(c != rS.L.getCount())
		eq = 0;
	else {
		SString title_t, title_s;
		for(uint i = 0; eq && i < c; i++) {
			const InnerEntry & r_t = L.at(i);
			const InnerEntry & r_s = rS.L.at(i);
			if(r_t.Attr != r_s.Attr)
				eq = 0;
			else if(r_t.ExtID != r_s.ExtID)
				eq = 0;
			else if(r_t.GcClsf != r_s.GcClsf)
				eq = 0;
			else {
				GetS(r_t.TitleP, title_t);
				rS.GetS(r_s.TitleP, title_s);
				if(title_t != title_s)
					eq = 0;
			}
		}
	}
	return eq;
}

int PPUhttStoreSelDescr::Pack()
{
	int    ok = -1;
	if(Pool.getDataLen()) {
		void * p_pack_handle = Pack_Start();
		if(p_pack_handle) {
			const uint c = L.getCount();
			for(uint i = 0; i < c; i++) {
				InnerEntry & r_item = L.at(i);
				Pack_Replace(p_pack_handle, r_item.TitleP);
			}
			Pack_Finish(p_pack_handle);
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

int PPUhttStoreSelDescr::ValidateEntry(const Entry * pEntry) const
{
	int    ok = 1;
	THROW(pEntry);
	THROW(oneof8(pEntry->Attr, attrGroup, attrBrand, attrName, attrClass, attrTag, attrCity, attrPeriod, attrProcessor));
	if(pEntry->Attr == attrTag) {
		PPObjTag tag_obj;
		PPObjectTag tag_rec;
		THROW(pEntry->TagID != 0);
		THROW(tag_obj.Fetch(pEntry->TagID, &tag_rec) > 0);
		THROW(tag_rec.ObjTypeID == PPOBJ_GOODS);
	}
	else if(pEntry->Attr == attrClass) {
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		THROW(pEntry->ClsID != 0);
		THROW(gc_obj.Fetch(pEntry->ClsID, &gc_pack) > 0);
		THROW(oneof8(pEntry->GcClsf, PPGdsCls::eKind, PPGdsCls::eGrade, PPGdsCls::eAdd, PPGdsCls::eX, PPGdsCls::eY, PPGdsCls::eZ, PPGdsCls::eW, PPGdsCls::eAdd2));

		THROW(gc_pack.Rec.Flags & PPGdsCls::fUsePropKind || pEntry->GcClsf != PPGdsCls::eKind);
		THROW(gc_pack.Rec.Flags & PPGdsCls::fUsePropGrade || pEntry->GcClsf != PPGdsCls::eGrade);
		THROW(gc_pack.Rec.Flags & PPGdsCls::fUsePropAdd  || pEntry->GcClsf != PPGdsCls::eAdd);
		THROW(gc_pack.Rec.Flags & PPGdsCls::fUsePropAdd2 || pEntry->GcClsf != PPGdsCls::eAdd2);
		THROW(gc_pack.Rec.Flags & PPGdsCls::fUseDimX || pEntry->GcClsf != PPGdsCls::eX);
		THROW(gc_pack.Rec.Flags & PPGdsCls::fUseDimY || pEntry->GcClsf != PPGdsCls::eY);
		THROW(gc_pack.Rec.Flags & PPGdsCls::fUseDimZ || pEntry->GcClsf != PPGdsCls::eZ);
		THROW(gc_pack.Rec.Flags & PPGdsCls::fUseDimW || pEntry->GcClsf != PPGdsCls::eW);
	}
	CATCHZOK
	return ok;
}

int PPUhttStoreSelDescr::AddEntry(uint * pPos, const Entry * pEntry)
{
	int    ok = 1;
	uint   pos = 0;
	THROW(ValidateEntry(pEntry));
	for(uint i = 0; i < L.getCount(); i++) {
		const InnerEntry & r_item = L.at(i);
		THROW(pEntry->Attr != r_item.Attr || pEntry->TagID != r_item.ExtID || pEntry->GcClsf != r_item.GcClsf);
	}
	{
		InnerEntry item;
		item.Attr = pEntry->Attr;
		item.ExtID = pEntry->TagID;
		item.GcClsf = pEntry->GcClsf;
		THROW_SL(AddS(pEntry->Title, &item.TitleP));
		pos = L.getCount();
		THROW_SL(L.insert(&item));
		ASSIGN_PTR(pPos, pos);
	}
	CATCHZOK
	return ok;
}

int PPUhttStoreSelDescr::UpdateEntry(uint pos, const Entry * pEntry)
{
	int    ok = 1;
	THROW(pos < L.getCount());
	if(pEntry) {
		THROW(ValidateEntry(pEntry));
		for(uint i = 0; i < L.getCount(); i++) {
			if(i != pos) {
				const InnerEntry & r_item = L.at(i);
				THROW(pEntry->Attr != r_item.Attr || pEntry->TagID != r_item.ExtID || pEntry->GcClsf != r_item.GcClsf);
			}
		}
		{
			InnerEntry & r_item = L.at(pos);
			r_item.Attr = pEntry->Attr;
			r_item.ExtID = pEntry->TagID;
			r_item.GcClsf = pEntry->GcClsf;
			SString org_title;
			GetS(r_item.TitleP, org_title);

			if(org_title != pEntry->Title)
				THROW_SL(AddS(pEntry->Title, &r_item.TitleP));
		}
	}
	else
		L.atFree(pos);
	CATCHZOK
	return ok;
}

int PPUhttStoreSelDescr::GetEntry(uint pos, Entry & rEntry) const
{
	int    ok = 1;
	if(pos < L.getCount()) {
		const InnerEntry & r_item = L.at(pos);
		rEntry.Attr  = r_item.Attr;
		rEntry.TagID = r_item.ExtID;
		rEntry.GcClsf = r_item.GcClsf;
		GetS(r_item.TitleP, rEntry.Title);
	}
	else
		ok = 0;
	return ok;
}

int PPUhttStoreSelDescr::GetDefaultEntryTitle(uint pos, SString & rBuf) const
{
	int    ok = 1;
	rBuf.Z();
	if(pos < L.getCount()) {
		const InnerEntry & r_item = L.at(pos);
		switch(r_item.Attr) {
			case attrGroup: PPLoadString("goodsgroup", rBuf); break;
			case attrBrand: PPLoadString("brand", rBuf); break;
			case attrName:  PPLoadString("appellation", rBuf); break;
			case attrTag:
				if(r_item.ExtID) {
					PPObjTag tag_obj;
					PPObjectTag tag_rec;
					if(tag_obj.Fetch(r_item.ExtID, &tag_rec) > 0)
						rBuf = tag_rec.Name;
				}
				if(!rBuf) {
					PPLoadString("tag", rBuf);
				}
				break;
			case attrClass:
				if(r_item.ExtID) {
					PPObjGoodsClass gc_obj;
					PPGdsClsPacket gc_pack;
					if(gc_obj.Fetch(r_item.ExtID, &gc_pack) > 0) {
						gc_pack.GetPropName(r_item.GcClsf, rBuf);
					}
				}
				if(!rBuf) {
					PPLoadString("class", rBuf);
				}
				break;
		}
	}
	else
		ok = 0;
	return ok;
}

int PPUhttStoreSelDescr::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		Ver = DS.GetVersion(); // Будем уверены в том, что версия при записи окажется актуальной
		THROW(Pack());
	}
	THROW_SL(Ver.Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, &L, rBuf));
	THROW(SStrGroup::SerializeS(dir, rBuf, pCtx));
	CATCHZOK
	return ok;
}
//
//
//
PPEdiProvider::PPEdiProvider()
{
	THISZERO();
}

int FASTCALL PPEdiProvider::IsEq(const PPEdiProvider & rS) const
{
	int    eq = 1;
	if(!sstreq(Name, rS.Name))
		eq = 0;
	else if(!sstreq(Symb, rS.Symb))
		eq = 0;
	else if(Flags != rS.Flags)
		eq = 0;
	else if(SuppOpFlags != rS.SuppOpFlags)
		eq = 0;
	else if(AddrPort != rS.AddrPort)
		eq = 0;
	else if(GuaID != rS.GuaID)
		eq = 0;
	else if(DtoPersonID != rS.DtoPersonID) // @v11.9.12
		eq = 0;
	return eq;
}

PPEdiProviderPacket::PPEdiProviderPacket()
{
}

PPEdiProviderPacket::~PPEdiProviderPacket()
{
}

int PPEdiProviderPacket::MakeUrl(int reserved, InetUrl & rUrl)
{
	rUrl.Z();
	int    ok = 0;
	SString temp_buf;
	GetExtStrData(reserved ? extssAddr2 : extssAddr, temp_buf);
	THROW_PP_S(temp_buf.NotEmptyS(), PPERR_EDI_SVCADDRUNDEF, Rec.Name);
	rUrl.Parse(temp_buf);
	ok |= 0x01;
	if(Rec.GuaID) {
		PPObjGlobalUserAcc gua_obj;
		PPGlobalUserAcc gua_rec;
		if(gua_obj.Search(Rec.GuaID, &gua_rec) > 0) { // Fetch использовать нельзя - пароль не извлечется!
			SString pwd;
			SString login;
			Reference::Decrypt(Reference::crymRef2, gua_rec.Password, sstrlen(gua_rec.Password), pwd);
			if(PPRef->Ot.GetTagStr(PPOBJ_GLOBALUSERACC, Rec.GuaID, PPTAG_GUA_LOGIN, login) > 0) {
				;
			}
			else {
				login = gua_rec.Name;
			}
			if(login.NotEmptyS()) {
				rUrl.SetComponent(InetUrl::cUserName, login.Transf(CTRANSF_INNER_TO_OUTER));
				ok |= 0x02;
				if(pwd.NotEmptyS()) {
					rUrl.SetComponent(InetUrl::cPassword, pwd.Transf(CTRANSF_INNER_TO_OUTER));
					pwd.Obfuscate();
					ok |= 0x04;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPEdiProviderPacket::GetExtStrData(int fldID, SString & rBuf) const { return PPGetExtStrData(fldID, ExtString, rBuf); }
int PPEdiProviderPacket::PutExtStrData(int fldID, const char * pBuf) { return PPPutExtStrData(fldID, ExtString, pBuf); }

bool FASTCALL PPEdiProviderPacket::IsEq(const PPEdiProviderPacket & rS) const
{
	static const int extss_list[] = {
		extssDllModuleName, extssTempPath, extssAddr, extssAddr2, /*extssLogin, extssPassword,*/extssFormatSymb, extssSubIn, extssSubOut
	};
	int    eq = 1;
	if(!Rec.IsEq(rS.Rec))
		eq = 0;
	else {
		for(uint i = 0; eq && i < SIZEOFARRAY(extss_list); i++) {
			if(PPCmpExtStrData(extss_list[i], ExtString, rS.ExtString, 0) != 0)
				eq = 0;
		}
	}
	return eq;
}
//
//
//
PPObjEdiProvider::PPObjEdiProvider(void * extraPtr) : PPObjReference(PPOBJ_EDIPROVIDER, extraPtr)
{
}

PPObjEdiProvider::~PPObjEdiProvider()
{
}

int PPObjEdiProvider::GetPacket(PPID id, PPEdiProviderPacket * pPack)
{
	int    ok = -1;
	PPEdiProviderPacket pack;
	int    r = Search(id, &pack.Rec);
	if(r > 0) {
		P_Ref->GetPropVlrString(Obj, id, EDIPRVPRP_EXTSTRING, pack.ExtString);
		ASSIGN_PTR(pPack, pack);
		ok = 1;
	}
	else
		ok = r;
	return ok;
}

int PPObjEdiProvider::PutPacket(PPID * pID, PPEdiProviderPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				PPEdiProviderPacket org_pack;
				THROW(GetPacket(*pID, &org_pack) > 0);
				if(pPack->IsEq(org_pack)) {
					ok = -1;
				}
				else {
					THROW(CheckRights(PPR_MOD));
					THROW(P_Ref->UpdateItem(Obj, *pID, &pPack->Rec, 0, 0));
					THROW(P_Ref->PutPropVlrString(Obj, *pID, EDIPRVPRP_EXTSTRING, pPack->ExtString));
					DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
				}
			}
			else {
				THROW(CheckRights(PPR_DEL));
				THROW(P_Ref->RemoveItem(Obj, *pID, 0));
				THROW(P_Ref->PutPropVlrString(Obj, *pID, EDIPRVPRP_EXTSTRING, 0));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
			}
			if(ok > 0)
				Dirty(*pID);
		}
		else if(pPack) {
			THROW(CheckRights(PPR_INS));
			*pID = pPack->Rec.ID;
			THROW(P_Ref->AddItem(Obj, pID, &pPack->Rec, 0));
			THROW(P_Ref->PutPropVlrString(Obj, *pID, EDIPRVPRP_EXTSTRING, pPack->ExtString));
			pPack->Rec.ID = *pID;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjEdiProvider::Edit(PPID * pID, void * extraPtr)
{
	class EdiProviderDialog : public TDialog {
		DECL_DIALOG_DATA(PPEdiProviderPacket);
		PPObjEdiProvider * P_Obj;
	public:
		EdiProviderDialog(PPObjEdiProvider * pObj) : TDialog(DLG_EDIPRV), P_Obj(pObj)
		{
			assert(pObj);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			RVALUEPTR(Data, pData);
            setCtrlData(CTL_EDIPRV_NAME, Data.Rec.Name);
            setCtrlData(CTL_EDIPRV_SYMB, Data.Rec.Symb);
            setCtrlLong(CTL_EDIPRV_ID, Data.Rec.ID);

            AddClusterAssoc(CTL_EDIPRV_SUPPOPS, 0, (1 << PPEDIOP_ORDER));
            AddClusterAssoc(CTL_EDIPRV_SUPPOPS, 1, (1 << PPEDIOP_ORDERRSP));
            AddClusterAssoc(CTL_EDIPRV_SUPPOPS, 2, (1 << PPEDIOP_DESADV));
            AddClusterAssoc(CTL_EDIPRV_SUPPOPS, 3, (1 << PPEDIOP_RECADV));
            AddClusterAssoc(CTL_EDIPRV_SUPPOPS, 4, (1 << PPEDIOP_ALCODESADV));
			AddClusterAssoc(CTL_EDIPRV_SUPPOPS, 5, (1 << PPEDIOP_INVOIC)); // @v10.0.06
            SetClusterData(CTL_EDIPRV_SUPPOPS, Data.Rec.SuppOpFlags);

			AddClusterAssoc(CTL_EDIPRV_FLAGS, 0, PPEdiProvider::fPassive);
			SetClusterData(CTL_EDIPRV_FLAGS, Data.Rec.Flags);

			Data.GetExtStrData(PPEdiProviderPacket::extssFormatSymb, temp_buf); // @v10.0.02
			setCtrlString(CTL_EDIPRV_FORMAT, temp_buf); // @v10.0.02
			Data.GetExtStrData(PPEdiProviderPacket::extssDllModuleName, temp_buf);
			setCtrlString(CTL_EDIPRV_MODULE, temp_buf);
			Data.GetExtStrData(PPEdiProviderPacket::extssAddr, temp_buf);
			setCtrlString(CTL_EDIPRV_ADDR1, temp_buf);
			setCtrlLong(CTL_EDIPRV_PORT1, Data.Rec.AddrPort);
			Data.GetExtStrData(PPEdiProviderPacket::extssAddr2, temp_buf);
			setCtrlString(CTL_EDIPRV_ADDR2, temp_buf);
			setCtrlLong(CTL_EDIPRV_PORT2, Data.Rec.AddrPort2);
			// @v11.9.4 {
			Data.GetExtStrData(PPEdiProviderPacket::extssSubIn, temp_buf);
			setCtrlString(CTL_EDIPRV_SUBIN, temp_buf);
			Data.GetExtStrData(PPEdiProviderPacket::extssSubOut, temp_buf);
			setCtrlString(CTL_EDIPRV_SUBOUT, temp_buf);
			// } @v11.9.4
			SetupPPObjCombo(this, CTLSEL_EDIPRV_GUA, PPOBJ_GLOBALUSERACC, Data.Rec.GuaID, OLW_CANINSERT);
			SetupPersonCombo(this, CTLSEL_EDIPRV_DTO, Data.Rec.DtoPersonID, OLW_CANINSERT, PPPRK_DTO, true); // @v11.9.12
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			SString temp_buf;
			getCtrlData(sel = CTL_EDIPRV_NAME, Data.Rec.Name);
			THROW_PP(*strip(Data.Rec.Name) != 0, PPERR_NAMENEEDED);
			THROW(P_Obj->CheckDupName(Data.Rec.ID, Data.Rec.Name));
			getCtrlData(sel = CTL_EDIPRV_SYMB, Data.Rec.Symb);
			THROW_PP(*strip(Data.Rec.Symb) != 0, PPERR_SYMBNEEDED);
			THROW(P_Obj->CheckDupSymb(Data.Rec.ID, strip(Data.Rec.Symb)));
			GetClusterData(CTL_EDIPRV_SUPPOPS, &Data.Rec.SuppOpFlags);
			GetClusterData(CTL_EDIPRV_FLAGS, &Data.Rec.Flags);
			getCtrlString(CTL_EDIPRV_FORMAT, temp_buf.Z()); // @v10.0.02
			Data.PutExtStrData(PPEdiProviderPacket::extssFormatSymb, temp_buf); // @v10.0.02
			getCtrlString(CTL_EDIPRV_MODULE, temp_buf.Z());
			Data.PutExtStrData(PPEdiProviderPacket::extssDllModuleName, temp_buf);
			getCtrlString(CTL_EDIPRV_ADDR1, temp_buf.Z());
			Data.PutExtStrData(PPEdiProviderPacket::extssAddr, temp_buf);
			Data.Rec.AddrPort  = getCtrlLong(CTL_EDIPRV_PORT1);
			getCtrlString(CTL_EDIPRV_ADDR2, temp_buf.Z());
			Data.PutExtStrData(PPEdiProviderPacket::extssAddr2, temp_buf);
			Data.Rec.AddrPort2 = getCtrlLong(CTL_EDIPRV_PORT2);
			// @v11.9.4 {
			getCtrlString(CTL_EDIPRV_SUBIN, temp_buf);
			Data.PutExtStrData(PPEdiProviderPacket::extssSubIn, temp_buf);
			getCtrlString(CTL_EDIPRV_SUBOUT, temp_buf);
			Data.PutExtStrData(PPEdiProviderPacket::extssSubOut, temp_buf);
			// } @v11.9.4
			Data.Rec.GuaID = getCtrlLong(CTLSEL_EDIPRV_GUA);
			Data.Rec.DtoPersonID = getCtrlLong(CTLSEL_EDIPRV_DTO); // @v11.9.12
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	};

	int    ok = cmCancel;
	int    r = cmCancel;
	int    valid_data = 0;
	bool   is_new = false;
	PPEdiProviderPacket pack;
	EdiProviderDialog * dlg = new EdiProviderDialog(this);
	THROW(CheckDialogPtr(&dlg));
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

IMPL_DESTROY_OBJ_PACK(PPObjEdiProvider, PPEdiProviderPacket);

int PPObjEdiProvider::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjEdiProvider, PPEdiProviderPacket>(this, p, id, stream, pCtx); }

int PPObjEdiProvider::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1, r;
	if(p && p->Data) {
		PPEdiProviderPacket * p_pack = static_cast<PPEdiProviderPacket *>(p->Data);
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				PPEdiProvider same_rec;
				if(p_pack->Rec.Symb[0] && SearchBySymb(p_pack->Rec.Symb, &same_id, &same_rec) > 0) {
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
					if(!r) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTEDIPRV, p_pack->Rec.ID, p_pack->Rec.Name);
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
				if(!r) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTEDIPRV, p_pack->Rec.ID, p_pack->Rec.Name);
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
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjEdiProvider::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPEdiProviderPacket * p_pack = static_cast<PPEdiProviderPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_GLOBALUSERACC, &p_pack->Rec.GuaID, ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjEdiProvider::SerializePacket(int dir, PPEdiProviderPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->ExtString, rBuf));
	CATCHZOK
	return ok;
}
//
//
//
PPObjFreightPackageType::PPObjFreightPackageType(void * extraPtr) : PPObjReference(PPOBJ_FREIGHTPACKAGETYPE, extraPtr)
{
}

int PPObjFreightPackageType::Edit(PPID * pID, void * extraPtr)
{
	class FreightPackageTypeDialog : public TDialog {
		DECL_DIALOG_DATA(PPFreightPackageType);
	public:
		FreightPackageTypeDialog() : TDialog(DLG_FPACKTYPE)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			setCtrlLong(CTL_FPACKTYPE_ID, Data.ID);
			setCtrlData(CTL_FPACKTYPE_NAME, Data.Name);
			setCtrlData(CTL_FPACKTYPE_SYMB, Data.Symb);
			AddClusterAssoc(CTL_FPACKTYPE_FLAGS, 0, PPFreightPackageType::fPassive);
			SetClusterData(CTL_FPACKTYPE_FLAGS, Data.Flags);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTL_FPACKTYPE_NAME, Data.Name);
			getCtrlData(CTL_FPACKTYPE_SYMB, Data.Symb);
			GetClusterData(CTL_FPACKTYPE_FLAGS, &Data.Flags);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	};
	int    ok = 1;
	int    r = cmCancel, valid_data = 0;
	PPFreightPackageType rec;
	FreightPackageTypeDialog * dlg = 0;
	THROW(CheckRightsModByID(pID));
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	else {
		MEMSZERO(rec);
	}
	THROW(CheckDialogPtr(&(dlg = new FreightPackageTypeDialog)));
	dlg->setDTS(&rec);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		if(dlg->getDTS(&rec)) {
			if(*pID)
				*pID = rec.ID;
			if(!CheckName(*pID, rec.Name, 0))
				dlg->selectCtrl(CTL_FPACKTYPE_NAME);
			else if(!CheckDupSymb(*pID, rec.Symb))
				PPErrorByDialog(dlg, CTL_FPACKTYPE_SYMB);
			else if(StoreItem(Obj, *pID, &rec, 1)) {
				Dirty(*pID);
				valid_data = 1;
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}
//
//
//
PPObjTaxSystemKind::PPObjTaxSystemKind(void * extraPtr) : PPObjReference(PPOBJ_TAXSYSTEMKIND, extraPtr)
{
}

int PPObjTaxSystemKind::ProcessReservedItem(TVRez & rRez)
{
	int    ok = 1;
	int    r;
	SString name;
	SString symb;
	PPID   id = static_cast<PPID>(rRez.getUINT());
	rRez.getString(name, 2);
	PPExpandString(name, CTRANSF_UTF8_TO_INNER);
	rRez.getString(symb, 2);
	THROW(r = Search(id));
	if(r < 0) {
		ReferenceTbl::Rec rec;
		rec.ObjType = Obj;
		rec.ObjID   = id;
		STRNSCPY(rec.ObjName, name);
		STRNSCPY(rec.Symb, symb);
		THROW(StoreItem(Obj, 0, &rec, 1));
	}
	CATCHZOK
	return ok;
}
