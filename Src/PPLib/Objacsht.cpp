// OBJACSHT.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPObjAccSheet)
//
PPAccSheet2::PPAccSheet2()
{
	THISZERO();
	Tag = PPOBJ_ACCSHEET;
}

PPAccSheet2 & PPAccSheet2::Z()
{
	THISZERO();
	Tag = PPOBJ_ACCSHEET;
	return *this;
}

class AccSheetDialog : public TDialog {
	DECL_DIALOG_DATA(PPAccSheet);
public:
	AccSheetDialog() : TDialog(DLG_ACCSHEET)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		setCtrlLong(CTL_ACCSHEET_ID, Data.ID);
		setCtrlData(CTL_ACCSHEET_NAME, Data.Name);
		setCtrlData(CTL_ACCSHEET_SYMB, Data.Symb);
		setupAssoc();
		checkLink();
		SetupPPObjCombo(this, CTLSEL_ACCSHEET_REGTYPE, PPOBJ_REGISTERTYPE, Data.CodeRegTypeID, 0, 0);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTL_ACCSHEET_NAME, Data.Name);
		if(*strip(Data.Name) == 0)
			ok = PPErrorByDialog(this, CTL_ACCSHEET_NAME, PPERR_NAMENEEDED);
		else {
			getCtrlData(CTL_ACCSHEET_SYMB, Data.Symb);
			// @v11.3.12 GetClusterData(CTL_ACCSHEET_ASSOC, &Data.Assoc);
			Data.Assoc = getCtrlLong(CTLSEL_ACCSHEET_ASSCOBJ); // @v11.3.12
			if(Data.Assoc == 0) {
				GetClusterData(CTL_ACCSHEET_FLAGS, &Data.Flags);
				Data.Flags &= ~ACSHF_AUTOCREATART;
			}
			else {
				if(Data.Assoc == PPOBJ_PERSON) {
					Data.ObjGroup = getCtrlLong(CTLSEL_ACCSHEET_GROUP);
					Data.WarehouseGroup = 0;
				}
				else if(Data.Assoc == PPOBJ_LOCATION) {
					Data.ObjGroup = LOCTYP_WAREHOUSE;
					Data.WarehouseGroup = getCtrlLong(CTLSEL_ACCSHEET_GROUP); // @v12.1.5
				}
				else {
					Data.ObjGroup = 0;
					Data.WarehouseGroup = 0;
				}
				GetClusterData(CTL_ACCSHEET_FLAGS, &Data.Flags);
				ushort v = getCtrlUInt16(CTL_ACCSHEET_AGTKIND);
				Data.Flags &= ~(ACSHF_USECLIAGT | ACSHF_USESUPPLAGT);
				if(v == 1)
					Data.Flags |= ACSHF_USECLIAGT;
				else if(v == 2)
					Data.Flags |= ACSHF_USESUPPLAGT;
			}
			getCtrlData(CTLSEL_ACCSHEET_REGTYPE, &Data.CodeRegTypeID);
			*pData = Data;
		}
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	int    setupAssoc();
	PPID   groupObjType() const { return (Data.Assoc == PPOBJ_PERSON) ? PPOBJ_PERSONKIND : 0; }
	void   getAssocData();
	void   checkLink();
};

IMPL_HANDLE_EVENT(AccSheetDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_ACCSHEET_ASSCOBJ)) {
		const  PPID preserve_assoc = Data.Assoc;
		Data.Assoc = getCtrlLong(CTLSEL_ACCSHEET_ASSCOBJ);
		if(Data.Assoc != preserve_assoc) {
			SETFLAG(Data.Flags, ACSHF_AUTOCREATART, oneof2(Data.Assoc, PPOBJ_PERSON, PPOBJ_LOCATION));
			Data.ObjGroup = 0;
			setupAssoc();
		}
		clearEvent(event);
	}
}

void AccSheetDialog::checkLink()
{
	if(!DS.CheckExtFlag(ECF_AVERAGE) || !PPMaster) {
		PPID   tmp_id = 0;
		ArticleFilt ar_filt;
		ar_filt.AccSheetID = Data.ID;
		ar_filt.Ft_Closed = 0;
		PPObjArticle arobj(&ar_filt/*(void *)Data.ID*/);
		const int r = arobj.GetFreeArticle(&tmp_id, Data.ID);
		if(!r)
			PPError();
		else {
			const bool has_link = (r >= 0 && tmp_id != 1);
			disableCtrl(CTLSEL_ACCSHEET_GROUP, has_link); // @verify
		}
	}
}

int AccSheetDialog::setupAssoc()
{
	PPIDArray assc_objtype_list;
	if(Data.Assoc == PPOBJ_ACCOUNT_PRE9004)
		Data.Assoc = PPOBJ_ACCOUNT2;
	assc_objtype_list.addzlist(PPOBJ_PERSON, PPOBJ_LOCATION, PPOBJ_ACCOUNT2, PPOBJ_GLOBALUSERACC, PPOBJ_PROCESSOR, 0L);
	SetupObjListCombo(this, CTLSEL_ACCSHEET_ASSCOBJ, Data.Assoc, &assc_objtype_list);
	{
		bool   disable_combo = true;
		if(Data.Assoc == PPOBJ_PERSON) {
			SetupPPObjCombo(this, CTLSEL_ACCSHEET_GROUP, groupObjType(), Data.ObjGroup, OLW_CANINSERT, 0);
			disable_combo = false;
		}
		else if(Data.Assoc == PPOBJ_LOCATION) { // @v12.1.5
			LocationFilt filt;
			filt.LocType = LOCTYP_WAREHOUSEGROUP;
			SetupLocationCombo(this, CTLSEL_ACCSHEET_GROUP, Data.WarehouseGroup, 0, &filt);
			disable_combo = false;
		}
		else
			setCtrlLong(CTL_ACCSHEET_GROUP, 0);
		disableCtrl(CTLSEL_ACCSHEET_GROUP, disable_combo);
	}
	AddClusterAssoc(CTL_ACCSHEET_FLAGS, 0, ACSHF_AUTOCREATART);
	AddClusterAssoc(CTL_ACCSHEET_FLAGS, 1, ACSHF_USEALIASSUBST);
	SetClusterData(CTL_ACCSHEET_FLAGS, Data.Flags);
	ushort v = (Data.Flags & ACSHF_USECLIAGT) ? 1 : ((Data.Flags & ACSHF_USESUPPLAGT) ? 2 : 0);
	setCtrlData(CTL_ACCSHEET_AGTKIND, &v);
	return 1;
}
//
//
//
PPObjAccSheet::PPObjAccSheet(void * extraPtr) : PPObjReference(PPOBJ_ACCSHEET, extraPtr)
{
	// @v10.4.10 (ответственность ctr PPObjReference) FiltProc = 0;
}

/*virtual*/int PPObjAccSheet::MakeReserved(long flags)
{
	int    ok = -1;
    if(flags & mrfInitializeDb) {
		long    _count = 0;
		PPAccSheet acs_rec;
		{
			for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&acs_rec) > 0;) {
				_count++;
			}
		}
        if(_count == 0) {
			SString temp_buf;
			const  PPID zero_id = 0;
			{
				acs_rec.Z();
				PPLoadString("warehouse_pl", temp_buf);
				STRNSCPY(acs_rec.Name, temp_buf);
				STRNSCPY(acs_rec.Symb, "ACS-WH");
				acs_rec.Assoc = PPOBJ_LOCATION;
				acs_rec.Flags |= ACSHF_AUTOCREATART;
				THROW(StoreItem(Obj, zero_id, &acs_rec, 1));
				ok = 1;
			}
			{
				PPPersonKind pk_rec;
				for(SEnum en = P_Ref->Enum(PPOBJ_PERSONKIND, 0); en.Next(&pk_rec) > 0;) {
					if(oneof6(pk_rec.ID, PPPRK_MAIN, PPPRK_EMPL, PPPRK_CLIENT, PPPRK_SUPPL, PPPRK_BANK, PPPRK_AGENT)) {
						acs_rec.Z();
						STRNSCPY(acs_rec.Name, pk_rec.Name);
						if(pk_rec.Symb[0]) {
							(temp_buf = "ACS").CatChar('-').Cat(pk_rec.Symb);
							STRNSCPY(acs_rec.Symb, temp_buf);
						}
						acs_rec.Assoc = PPOBJ_PERSON;
						acs_rec.ObjGroup = pk_rec.ID;
						if(pk_rec.ID != PPPRK_BANK)
							acs_rec.Flags |= ACSHF_AUTOCREATART;
						if(pk_rec.ID == PPPRK_CLIENT)
							acs_rec.Flags |= ACSHF_USECLIAGT;
						else if(pk_rec.ID == PPPRK_SUPPL)
							acs_rec.Flags |= ACSHF_USESUPPLAGT;
						THROW(StoreItem(Obj, zero_id, &acs_rec, 1));
						ok = 1;
					}
				}
			}
        }
    }
    CATCHZOK
    return ok;
}

int PPObjAccSheet::IsAssoc(PPID acsID, PPID objType, PPAccSheet * pRec)
{
	int    ok = -1;
	PPAccSheet acs_rec;
	if(Fetch(acsID, &acs_rec) > 0) {
		ASSIGN_PTR(pRec, acs_rec);
		if(acs_rec.Assoc == objType)
			ok = 1;
	}
	return ok;
}

int PPObjAccSheet::IsLinkedToMainOrg(PPID acsID)
{
	PPAccSheet acs_rec;
	return BIN(acsID && Fetch(acsID, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup == PPPRK_MAIN);
}

int PPObjAccSheet::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel;
	int    valid_data = 0;
	PPAccSheet rec;
	AccSheetDialog * dlg = 0;
	THROW(CheckRightsModByID(pID));
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	THROW(CheckDialogPtr(&(dlg = new AccSheetDialog)));
	dlg->setDTS(&rec);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		if(dlg->getDTS(&rec)) {
			if(*pID)
				*pID = rec.ID;
			if(!CheckName(*pID, rec.Name, 0))
				dlg->selectCtrl(CTL_ACCSHEET_NAME);
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

/*virtual*/void * PPObjAccSheet::CreateObjListWin(uint flags, void * extraPtr)
{
	class PPObjAccSheetListWindow : public PPObjListWindow {
	public:
		PPObjAccSheetListWindow(PPObject * pObj, uint flags, void * extraPtr) : PPObjListWindow(pObj, flags, extraPtr)
		{
			DefaultCmd = cmaEdit;
			SetToolbar(TOOLBAR_LIST_ACCSHEET);
		}
	private:
		DECL_HANDLE_EVENT
		{
			int    update = 0;
			PPID   id = 0;
			PPObjListWindow::handleEvent(event);
			if(P_Obj) {
				getResult(&id);
				if(TVCOMMAND) {
					switch(TVCMD) {
						case cmaInsert:
							id = 0;
							if(Flags & OLW_CANINSERT && P_Obj->Edit(&id, ExtraPtr) == cmOK)
								update = 2;
							else
								::SetFocus(H());
							break;
						case cmaMore:
							if(id) {
								ArticleFilt filt;
								filt.AccSheetID = id;
								ViewArticle(&filt);
							}
							break;
					}
				}
				PostProcessHandleEvent(update, id);
			}
		}
		virtual int Transmit(PPID id)
		{
			int    ok = -1;
			ObjTransmitParam param;
			if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
				PPObjAccSheet acs_obj;
				const PPIDArray & rary = param.DestDBDivList.Get();
				PPObjIDArray objid_ary;
				StrAssocArray * p_list = acs_obj.MakeStrAssocList(0);
				THROW(p_list);
				PPWaitStart();
				for(uint i = 0; i < p_list->getCount(); i++) {
					THROW(objid_ary.Add(PPOBJ_ACCSHEET, p_list->Get(i).Id));
				}
				THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
				ok = 1;
			}
			CATCH
				ok = PPErrorZ();
			ENDCATCH
			PPWaitStop();
			return ok;
		}
	};
	return /*0; */ new PPObjAccSheetListWindow(this, flags, extraPtr);
}

int PPObjAccSheet::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE && _obj == PPOBJ_PERSONKIND) {
		int    r;
		for(PPID acs_id = 0; (r = EnumItems(&acs_id)) > 0;)
			if(P_Ref->data.Val1 == PPOBJ_PERSON && P_Ref->data.Val2 == _id) {
				ok = RetRefsExistsErr(Obj, acs_id);
				break;
			}
		if(!r)
			ok = DBRPL_ERROR;
	}
	return ok;
}

int PPObjAccSheet::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data)
		if(stream == 0) {
			PPAccSheet * p_rec = static_cast<PPAccSheet *>(p->Data);
			if(*pID == 0) {
				PPID   same_id = 0;
				if(P_Ref->SearchSymb(Obj, &same_id, p_rec->Name, offsetof(PPAccSheet, Name)) > 0) {
					PPAccSheet same_rec;
					if(Search(same_id, &same_rec) > 0 && same_rec.Assoc == p_rec->Assoc) {
						if(same_rec.ObjGroup == p_rec->ObjGroup) {
							ASSIGN_PTR(pID, same_id);
						}
						else if(p_rec->Assoc == PPOBJ_LOCATION && (same_rec.ObjGroup == LOCTYP_WAREHOUSE || !same_rec.ObjGroup) &&
							(p_rec->ObjGroup == LOCTYP_WAREHOUSE || !p_rec->ObjGroup)) {
							ASSIGN_PTR(pID, same_id);
						}
						else
							same_id = 0;
					}
					else
						same_id = 0;
				}
				if(same_id == 0) {
					p_rec->ID = 0;
					if(StoreItem(Obj, *pID, p_rec, 1)) {
						ASSIGN_PTR(pID, P_Ref->data.ObjID);
					}
					else {
						pCtx->OutputAcceptObjErrMsg(Obj, p_rec->ID, p_rec->Name);
						ok = -1;
					}
				}
			}
			else {
				;
			}
		}
		else {
			THROW(Serialize_(+1, static_cast<ReferenceTbl::Rec *>(p->Data), stream, pCtx));
		}
	CATCHZOK
	return ok;
}

int PPObjAccSheet::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPAccSheet * p_rec = static_cast<PPAccSheet *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_REGISTERTYPE, &p_rec->CodeRegTypeID, ary, replace));
		if(p_rec->Assoc == PPOBJ_PERSON) {
			THROW(ProcessObjRefInArray(PPOBJ_PERSONKIND, &p_rec->ObjGroup, ary, replace));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
class AccSheetCache : public ObjCache {
public:
	struct AccSheetData : public ObjCacheEntry {
		PPID   BinArID;       // Статья для сброса остатков по закрываемым статьям
		PPID   CodeRegTypeID; // ИД типа регистрационного документа, идентифицирующего персоналию, соответствующую статье.
		long   Flags;         // ACSHF_XXX
		long   Assoc;         // @#{0L, PPOBJ_PERSON, PPOBJ_LOCATION, PPOBJ_ACCOUNT} Ассоциированный объект
		long   ObjGroup;      // Подгруппа ассоциированных объектов
	};
	AccSheetCache() : ObjCache(PPOBJ_ACCSHEET, sizeof(AccSheetData))
	{
	}
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
};

int AccSheetCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	AccSheetData * p_cache_rec = static_cast<AccSheetData *>(pEntry);
	PPObjAccSheet as_obj;
	PPAccSheet rec;
	if(as_obj.Search(id, &rec) > 0) {
		p_cache_rec->BinArID  = rec.BinArID;
		p_cache_rec->CodeRegTypeID = rec.CodeRegTypeID;
		p_cache_rec->Flags    = rec.Flags;
		p_cache_rec->Assoc    = rec.Assoc;
		p_cache_rec->ObjGroup = rec.ObjGroup;
		ok = PutName(rec.Name, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void AccSheetCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPAccSheet * p_data_rec = static_cast<PPAccSheet *>(pDataRec);
	const AccSheetData * p_cache_rec = static_cast<const AccSheetData *>(pEntry);
	p_data_rec->Z();
	p_data_rec->Tag   = PPOBJ_ACCSHEET;
	p_data_rec->ID    = p_cache_rec->ID;
	p_data_rec->BinArID = p_cache_rec->BinArID;
	p_data_rec->CodeRegTypeID = p_cache_rec->CodeRegTypeID;
	p_data_rec->Flags = p_cache_rec->Flags;
	p_data_rec->Assoc = p_cache_rec->Assoc;
	p_data_rec->ObjGroup = p_cache_rec->ObjGroup;
	GetName(pEntry, p_data_rec->Name, sizeof(p_data_rec->Name));
}
// }

IMPL_OBJ_FETCH(PPObjAccSheet, PPAccSheet, AccSheetCache);
