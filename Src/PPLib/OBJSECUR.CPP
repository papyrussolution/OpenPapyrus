// OBJSECUR.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998-2001, 2005, 2007, 2008, 2009, 2010, 2012, 2013, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
// @v9.6.3 #include <idea.h>

#define SECUREXTRAPARAMSIGN 0xABCDEF07

SLAPI PPConfig::PPConfig()
{
	THISZERO();
}

PPObjSecur::Exclusion::Exclusion(int exclrt) : State(0)
{
	if(!PPMaster) {
		PPThreadLocalArea & r_tla = DS.GetTLA();
		PPObjSecur sec_obj(PPOBJ_USR, 0);
		PPSecur secur;
		if(sec_obj.Fetch(r_tla.Lc.User, &secur) > 0 && secur.UerID) {
			PPObjSecur uer_obj(PPOBJ_USREXCLRIGHTS, 0);
			PPSecur uer;
			if(uer_obj.Fetch(secur.UerID, &uer) > 0 && (uer.UerFlags & exclrt)) {
				PPRights uer_rights;
				PPAccessRestriction accsr;
				if(uer_rights.Get(PPOBJ_USREXCLRIGHTS, uer.ID)) {
					uer_rights.ExtentOpRights();
                    Preserve = r_tla.Rights;
                    r_tla.Rights.Merge(uer_rights, 0);
                    r_tla.RightsUpdateCounter.Incr();
                    State |= stMerged;
				}
			}
		}
	}
}

PPObjSecur::Exclusion::~Exclusion()
{
	if(State & stMerged) {
		PPThreadLocalArea & r_tla = DS.GetTLA();
        r_tla.Rights = Preserve;
        r_tla.RightsUpdateCounter.Incr();
	}
}

PPObjSecur::ExtraParam::ExtraParam() : Signature(SECUREXTRAPARAMSIGN), Type(0), ParentID(0), SampleID(0), Flags(0)
{
}

PPObjSecur::ExtraParam & PPObjSecur::ExtraParam::operator = (const PPObjSecur::ExtraParam & rS)
{
    Type = rS.Type;
    ParentID = rS.ParentID;
	SampleID = rS.SampleID; // @v9.8.4
    Flags = rS.Flags;
    return *this;
}

int PPObjSecur::ExtraParam::IsConsistent() const
{
	return BIN(Signature == SECUREXTRAPARAMSIGN);
}

static int SLAPI UsrListFilt(void * rec, void * extraPtr/*grp*/)
{
	const PPID parent_id = (PPID)extraPtr;
	return (!parent_id || (rec && (((PPSecur*)rec)->ParentID == parent_id)));
}

SLAPI PPObjSecur::PPObjSecur(PPID aObj, void * extraPtr) : PPObjReference(aObj, extraPtr)
{
	if(Obj == PPOBJ_USR)
		filt = UsrListFilt;
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

int SLAPI PPObjSecur::AssignImages(ListBoxDef * pDef)
{
	if(pDef && pDef->valid() && (ImplementFlags & implTreeSelector)) {
		LongArray list;
		StdTreeListBoxDef * p_def = (StdTreeListBoxDef*)pDef;
		p_def->ClearImageAssocList();
		if(p_def->getIdList(list) > 0) {
			PPSecur rec;
			MEMSZERO(rec);
			for(uint i = 0; i < list.getCount(); i++) {
				const  PPID id = list.get(i);
				long   img_id = 0;
				if((id & PPObjSecur::maskUserGroup) || Obj == PPOBJ_USRGRP)
					img_id = ICON_USERGROUP16;
				else if((id & PPObjSecur::maskConfig) || Obj == PPOBJ_CONFIG)
					img_id = ICON_GEAR16;
				else if((id & PPObjSecur::maskUER) || Obj == PPOBJ_USREXCLRIGHTS)
					img_id = ICON_STARYELLOW16;
				else
					img_id = ICON_USER16;
				p_def->AddImageAssoc(id, img_id);
			}
		}
	}
	return 1;
}

// virtual
ListBoxDef * SLAPI PPObjSecur::Selector(void * extraPtr)
{
	if(extraPtr && ((ExtraParam *)extraPtr)->IsConsistent())
		SelectorP = *(ExtraParam *)extraPtr;
	ListBoxDef * p_def = PPObject::Selector(&SelectorP);
	AssignImages(p_def);
	return p_def;
}

// virtual
int SLAPI PPObjSecur::UpdateSelector(ListBoxDef * pDef, void * extraPtr)
{
	int    ok = PPObject::UpdateSelector(pDef, &SelectorP);
	if(ok > 0)
		AssignImages(pDef);
	return ok;
}

StrAssocArray * SLAPI PPObjSecur::MakeStrAssocList(void * extraPtr)
{
	ExtraParam param;
	if(extraPtr && ((ExtraParam *)extraPtr)->IsConsistent())
		param = *(ExtraParam *)extraPtr;
	StrAssocArray * p_list = new StrAssocArray();
	THROW_MEM(p_list);
	{
		PPSecur rec;
		SString temp_buf;
		PPIDArray parent_list;
		if(param.Flags & ExtraParam::fShowAll) {
			if(Obj != PPOBJ_USREXCLRIGHTS) {
				for(SEnum en = ref->Enum(PPOBJ_USREXCLRIGHTS, 0); en.Next(&rec) > 0;) {
					if(*strip(rec.Name) == 0)
						ideqvalstr(rec.ID, rec.Name, sizeof(rec.Name));
					THROW_SL(p_list->Add((rec.ID | PPObjSecur::maskUER), 0, rec.Name));
				}
			}
			if(Obj != PPOBJ_USRGRP) {
				for(SEnum en = ref->Enum(PPOBJ_USRGRP, 0); en.Next(&rec) > 0;) {
					if(*strip(rec.Name) == 0)
						ideqvalstr(rec.ID, rec.Name, sizeof(rec.Name));
					PPID   par_id = 0;
					THROW_SL(p_list->Add((rec.ID | PPObjSecur::maskUserGroup), par_id, rec.Name));
				}
			}
			if(Obj != PPOBJ_CONFIG) {
				if(ref->GetItem(PPOBJ_CONFIG, PPCFG_MAIN, &rec) > 0) {
					THROW_SL(p_list->Add((rec.ID | PPObjSecur::maskConfig), 0, rec.Name));
				}
			}
		}
		{
			for(SEnum en = ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
				if(!param.ParentID || rec.ParentID == param.ParentID) {
					if(*strip(rec.Name) == 0)
						ideqvalstr(rec.ID, rec.Name, sizeof(rec.Name));
					PPID   par_id = (Obj == PPOBJ_USR) ? (rec.ParentID | PPObjSecur::maskUserGroup) : 0;
					THROW_SL(p_list->Add(rec.ID, par_id, rec.Name));
					if(par_id)
						parent_list.add(par_id);
				}
			}
			parent_list.sortAndUndup();
		}
		if(!(param.Flags & ExtraParam::fShowAll)) {
			PPObjSecur grp_obj(PPOBJ_USRGRP, 0);
			for(uint i = 0; i < parent_list.getCount(); i++) {
				const PPID id = (parent_list.get(i) & ~PPObjSecur::maskUserGroup);
				if(grp_obj.Search(id, &rec) > 0) {
					temp_buf = rec.Name;
				}
				else
					ideqvalstr(id, temp_buf);
				THROW_SL(p_list->Add((id | PPObjSecur::maskUserGroup), 0, temp_buf));
			}
		}
	}
	p_list->SortByText();
	for(uint i = 0; i < p_list->getCount(); i++) {
		StrAssocArray::Item item = p_list->at_WithoutParent(i);
		if(item.Id == (PPCFG_MAIN | PPObjSecur::maskConfig)) {
			if(i != 0)
				p_list->Swap(i, 0);
			break;
		}
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int SLAPI PPObjSecur::GetPrivateDesktop(PPID userID, PPID * pDesktopID)
{
	int    ok = -1;
	PPID   dt_id = 0;
	PPConfig cfg_rec;
	PPConfigPrivate cfgp_rec;
	if(ref->GetConfig(PPOBJ_USR, userID, PPPRP_CFG, &cfg_rec, sizeof(cfg_rec)) > 0) {
		dt_id = cfg_rec.DesktopID;
		if(cfg_rec.Tag == PPOBJ_USR && cfg_rec.ObjID == userID) // desktop for user
			ok = 1;
	}
	if(ok < 0 && ref->GetConfig(PPOBJ_USR, userID, PPPRP_CFGPRIVATE, &cfgp_rec, sizeof(cfgp_rec)) > 0) {
		SString desk_name;
		Reference::GetExField(&cfgp_rec, PCFGEXSTR_DESKTOPNAME, desk_name);
		dt_id = 0;
		if(desk_name.Len()) {
			SString def_desk_name, db_symb;
			const PPCommandItem * p_item = 0;
			PPCommandGroup desk_list;
			PPCommandMngr * p_mgr = PPDesktop::LoadDeskList(1, &desk_list);
			if(p_mgr) {
				CurDict->GetDbSymb(db_symb);
				if((p_item = desk_list.SearchByName(desk_name, db_symb, 0)) && p_item->Kind == PPCommandItem::kGroup)
					dt_id = p_item->ID;
				ZDELETE(p_mgr);
			}
		}
		SETIFZ(dt_id, cfgp_rec.DesktopID);
		ok = 2;
	}
	else if(dt_id) // desktop for users group
		ok = 3;
	ASSIGN_PTR(pDesktopID, dt_id);
	return ok;
}

int SLAPI PPObjSecur::AssignPrivateDesktop(PPID userID, PPID desktopID, const char * pDeskName, int use_ta)
{
	int    ok = -1;
	PPConfig cfg_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(ref->GetConfig(PPOBJ_USR, userID, PPPRP_CFG, &cfg_rec, sizeof(cfg_rec)) > 0 && cfg_rec.Tag == PPOBJ_USR && cfg_rec.ObjID == userID) {
			if(cfg_rec.DesktopID != desktopID) {
				cfg_rec.DesktopID = desktopID;
				THROW(ref->SetConfig(PPOBJ_USR, userID, PPPRP_CFG, &cfg_rec, sizeof(cfg_rec)));
			}
			ok = 1;
		}
		if(ok < 0) {
			//
			// Если пользователь не имеет собственной конфигурации, то записываем частную конфигурацию
			//
			int    r;
			SString desk_name;
			PPConfigPrivate cfgp_rec;
			MEMSZERO(cfgp_rec);
			THROW(r = ref->GetConfig(PPOBJ_USR, userID, PPPRP_CFGPRIVATE, &cfgp_rec, sizeof(cfgp_rec)));
			Reference::GetExField(&cfgp_rec, PCFGEXSTR_DESKTOPNAME, desk_name);
			if(r < 0 || cfgp_rec.DesktopID != desktopID || desk_name.Len() == 0) {
				cfgp_rec.DesktopID = desktopID;
				Reference::SetExField(&cfgp_rec, PCFGEXSTR_DESKTOPNAME, pDeskName);
				THROW(ref->SetConfig(PPOBJ_USR, userID, PPPRP_CFGPRIVATE, &cfgp_rec, sizeof(cfgp_rec)));
			}
			ok = 2;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjSecur::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE)
		if(_obj == PPOBJ_CONFIG && Obj == PPOBJ_USRGRP) {
			ReferenceTbl::Key2 k;
			k.ObjType = Obj;
			k.Val1    = _id;
			int    r = SearchByKey(ref, 2, &k, 0);
			if(r > 0)
				ok = RetRefsExistsErr(Obj, ref->data.ObjID);
			else if(r == 0)
				ok = DBRPL_ERROR;
		}
		else if(_obj == PPOBJ_USRGRP && Obj == PPOBJ_USR) {
			ok = DBRPL_OK; // @todo Обработать событие
		}
	return ok;
}

int SLAPI PPObjSecur::Edit(PPID * pID, void * extraPtr)
{
	int    ok = CheckRights(*pID ? PPR_MOD : PPR_INS) ? EditSecurDialog(Obj, pID, extraPtr) : PPErrorZ();
	return ok;
}

//virtual
int  SLAPI PPObjSecur::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options, void * pExtraParam)
{
	int    ok = -1;
	THROW(CheckRights(PPR_DEL));
	if(oneof2(Obj, PPOBJ_USRGRP, PPOBJ_USR)) {
		int    msg = 0;
		if(Obj == PPOBJ_USRGRP) {
			PPSecur usr_rec;
			for(SEnum en = ref->EnumByIdxVal(PPOBJ_USR, 1, id); en.Next(&usr_rec) > 0;) {
				THROW_PP(usr_rec.ParentID != id, PPERR_CANTDELNEMPTYUSRGRP);
			}
			msg = PPCFM_DELUSRGRP;
		}
		else if(Obj == PPOBJ_USR) {
			THROW_PP(id != PPUSR_MASTER, PPERR_CANTDELMASTER);
			msg = PPCFM_DELUSR;
		}
		if(msg && PPMessage(mfConf|mfYes|mfNo, msg) == cmYes) {
			PPTransaction tra(1);
			THROW(tra);
			THROW(ref->RemoveSecur(Obj, id, 0));
			THROW(RemoveSync(id));
			THROW(tra.Commit());
			ok = 1;
		}
	}
	else if(Obj == PPOBJ_CONFIG) {
		PPMessage(mfInfo|mfCancel, PPINF_CANNOTDELCONFIG);
	}
	CATCHZOKPPERR
	return ok;
}

class Grp_n_UsrDialog : public ObjViewDialog {
public:
	Grp_n_UsrDialog(PPObjSecur * pUsrObj) : ObjViewDialog(DLG_GRPNUSR, pUsrObj, 0), SecurGrpObj(PPOBJ_USRGRP, 0)
	{
		P_UsrObj = pUsrObj;
	}
private:
	virtual void addItem();
	virtual void removeItem();
	virtual void editItem();

	PPObjSecur * P_UsrObj;
	PPObjSecur SecurGrpObj;
};

void Grp_n_UsrDialog::addItem()
{
	PPID   new_id = 0;
	//
	PPObjSecur::ExtraParam param;
	param.Flags |= param.fSelectNewType;
	PPID   cur_id = getCurrID();
	PPSecur cur_rec;
	if(cur_id & PPObjSecur::maskUserGroup)
		param.ParentID = (cur_id & ~PPObjSecur::maskUserGroup);
	else if(P_UsrObj->Fetch(cur_id, &cur_rec) > 0)
		param.ParentID = cur_rec.ParentID;
	if(EditSecurDialog(PPOBJ_USR, &new_id, &param) == cmOK) {
		updateList(new_id);
	}
#if 0 // {
	uint   v = 0;
	if(SelectorDialog(DLG_SELNEWSEC, CTL_SELNEWSEC_SEL, &v) > 0) {
		PPID   new_id = 0;
		PPSecur new_rec;
		MEMSZERO(new_rec);
		if(v == 0) { // USER
			PPID   cur_id = getCurrID();
			PPID   parent_id = 0;
			PPSecur cur_rec;
			if(cur_id & PPObjSecur::maskUserGroup)
				parent_id = (cur_id & ~PPObjSecur::maskUserGroup);
			else if(P_UsrObj->Fetch(cur_id, &cur_rec) > 0)
				parent_id = cur_rec.ParentID;
			if(P_UsrObj->Edit(&new_id, (void *)parent_id) == cmOK) {
				ok = 1;
			}
		}
		else if(v == 1) { // GROUP
			if(SecurGrpObj.Edit(&new_id, 0) == cmOK) {
				new_id |= PPObjSecur::maskUserGroup;
				ok = 1;
			}
		}
		if(ok > 0)
			updateList(new_id);
	}
#endif // } 0
}

void Grp_n_UsrDialog::editItem()
{
	int    ok = -1;
	PPID   cur_id = getCurrID();
	if(cur_id) {
		if(cur_id & PPObjSecur::maskUserGroup) {
			cur_id &= ~PPObjSecur::maskUserGroup;
			if(SecurGrpObj.Edit(&cur_id, 0) == cmOK) {
				cur_id |= PPObjSecur::maskUserGroup;
				ok = 1;
			}
		}
		else {
			if(P_UsrObj->Edit(&cur_id, 0) == cmOK)
				ok = 1;
		}
		if(ok > 0)
			updateList(cur_id);
	}
}

void Grp_n_UsrDialog::removeItem()
{
	int    ok = -1;
	PPID   cur_id = getCurrID();
	if(cur_id) {
		if(cur_id & PPObjSecur::maskUserGroup) {
			cur_id &= ~PPObjSecur::maskUserGroup;
			if(SecurGrpObj.RemoveObjV(cur_id, 0, PPObject::rmv_default, 0) > 0)
				ok = 1;
		}
		else if(P_UsrObj->RemoveObjV(cur_id, 0, PPObject::rmv_default, 0) > 0)
			ok = 1;
		if(ok > 0)
			updateList(cur_id);
	}
}

int SLAPI PPObjSecur::Browse(void * extraPtr)
{
	int    ok = 1;
	if(!CheckRights(PPR_READ))
		ok = PPErrorZ();
	else if(Obj == PPOBJ_USR)
		ok = BIN(CheckExecAndDestroyDialog(new Grp_n_UsrDialog(this), 1, 0));
	return ok;
}

// virtual
void * SLAPI PPObjSecur::CreateObjListWin(uint flags, void * extraPtr)
{
	class PPObjSecurListWindow : public PPObjListWindow {
	public:
		PPObjSecurListWindow(PPObject * pObj, uint flags, void * extraPtr) : PPObjListWindow(pObj, flags, extraPtr)
		{
			if(extraPtr && ((PPObjSecur::ExtraParam *)extraPtr)->IsConsistent()) {
				Param = *(PPObjSecur::ExtraParam *)extraPtr;
			}
			else
				Param.Flags |= PPObjSecur::ExtraParam::fShowAll;
			DefaultCmd = cmaEdit;
			SetToolbar(TOOLBAR_LIST_GRPNUSR);
		}
	private:
		DECL_HANDLE_EVENT
		{
			int    update = 0;
			PPID   cur_id = 0;
			PPID   preserve_focus_id = 0;
			if(P_Obj) {
				if(event.isCmd(cmaInsert)) {
					getResult(&cur_id);
					PPID   id = 0;
					PPObjSecur::ExtraParam param;
					param.Flags |= param.fSelectNewType;
					if(cur_id & PPObjSecur::maskUserGroup) {
						param.ParentID = (cur_id & ~PPObjSecur::mask);
					}
					else if(!(cur_id & PPObjSecur::mask)) {
						PPObjSecur usr_obj(PPOBJ_USR, 0);
						PPSecur sec_rec;
						if(usr_obj.Fetch(cur_id, &sec_rec) > 0 && sec_rec.Tag == PPOBJ_USR) {
							param.ParentID = sec_rec.ParentID;
						}
					}
					if(Flags & OLW_CANINSERT && P_Obj->Edit(&id, &param) == cmOK) {
						cur_id = id;
						update = 2;
					}
					else
						::SetFocus(H());
				}
				if(event.isKeyDown(kbAltF2) || event.isCmd(cmaAltInsert)) {
					if(Flags & OLW_CANINSERT) {
						PPID   id = 0;
						getResult(&cur_id);
						PPObjSecur::ExtraParam param;
						if(cur_id & PPObjSecur::maskUserGroup) {
							param.SampleID = (cur_id & ~PPObjSecur::mask);
							param.Type = PPOBJ_USRGRP;
						}
						else if(cur_id & PPObjSecur::maskUER) {
							param.SampleID = (cur_id & ~PPObjSecur::mask);
							param.Type = PPOBJ_USREXCLRIGHTS;
						}
						else if(!(cur_id & PPObjSecur::mask)) {
							param.SampleID = (cur_id & ~PPObjSecur::mask);
							param.Type = PPOBJ_USR;
						}
						if(param.SampleID && param.Type) {
							if(P_Obj->Edit(&id, &param) == cmOK) {
								if(param.Type == PPOBJ_USRGRP)
									cur_id = (id | PPObjSecur::maskUserGroup);
								if(param.Type == PPOBJ_USREXCLRIGHTS)
									cur_id = (id | PPObjSecur::maskUER);
								else if(param.Type == PPOBJ_USR)
									cur_id = id;
								update = 2;
							}
							else
								::SetFocus(H());
						}
					}
					clearEvent(event);
				}
				else if(event.isCmd(cmaDelete)) {
					if(Flags & OLW_CANDELETE) {
						getResult(&cur_id);
						preserve_focus_id = cur_id;
						PPID   id = 0;
						if(cur_id & PPObjSecur::maskUserGroup) {
							//param.ParentID = (cur_id & ~PPObjSecur::mask);
							PPObjSecur ug_obj(PPOBJ_USRGRP, 0);
							if(ug_obj.RemoveObjV((cur_id & ~PPObjSecur::mask), 0, PPObject::rmv_default, ExtraPtr) > 0) {
								update = 2;
							}
						}
						else if(!(cur_id & PPObjSecur::mask)) {
							PPObjSecur usr_obj(PPOBJ_USR, 0);
							if(usr_obj.RemoveObjV(cur_id, 0, PPObject::rmv_default, ExtraPtr) > 0) {
								update = 2;
							}
						}
						else
							::SetFocus(H());
					}
				}
				else
					PPObjListWindow::handleEvent(event);
			}
			else
				PPObjListWindow::handleEvent(event);
			if(update) {
				P_Obj->UpdateSelector(P_Lb->def, &Param);
				P_Lb->Draw_();
				P_Lb->setRange(P_Lb->def->getRecsCount());
				if(update == 2) {
					if(preserve_focus_id)
						P_Lb->search(&preserve_focus_id, 0, srchFirst | lbSrchByID);
					else if(cur_id)
						P_Lb->search(&cur_id, 0, srchFirst | lbSrchByID);
				}
				::SetFocus(H());
			}
		}
		ExtraParam Param;
	};
	return new PPObjSecurListWindow(this, flags, extraPtr);
}

int SLAPI PPObjSecur::ProcessReservedItem(TVRez & rez)
{
	int    ok = 1;
	int    r = 0;
	SString name;
	PPID   id = (PPID)rez.getUINT();
	rez.getString(name, 2);
	PPExpandString(name, CTRANSF_UTF8_TO_INNER); // @v9.4.4
	if(stricmp(name, PPSession::P_JobLogin) == 0) {
		PPID   temp_id = 0;
		PPSecur temp_rec;
		THROW(r = SearchByName(PPSession::P_JobLogin, &temp_id, &temp_rec));
		if(r < 0) {
			id = 0;
		}
	}
	else {
		THROW(r = Search(id));
	}
	if(r < 0) {
		PPSecur secur;
		MEMSZERO(secur);
		secur.Tag = Obj;
		secur.ID  = id;
		STRNSCPY(secur.Name, name);
		if(Obj == PPOBJ_USR) {
			secur.ParentID = PPGRP_EVERYONE;
			char   pw[64];
			pw[0] = 0;
			Reference::Encrypt(Reference::crymRef2, pw, secur.Password, sizeof(secur.Password));
		}
		else if(Obj == PPOBJ_USRGRP)
			secur.ParentID = PPCFG_MAIN;
		Reference::VerifySecur(&secur, 1);
		THROW(EditItem(Obj, 0, &secur, 1));
	}
	CATCHZOK
	return ok;
}

int SLAPI EditMainConfig()
{
	PPID   id = PPCFG_MAIN;
	PPObjSecur cfg_obj(PPOBJ_CONFIG, 0);
	cfg_obj.Edit(&id, 0);
	return 1;
}
//
//
//
class UserCache : public ObjCache {
public:
	SLAPI  UserCache() : ObjCache(PPOBJ_USR, sizeof(Data)) {}
private:
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		PPID   PersonID;
		PPID   ParentID;
		PPID   UerID;
		long   Flags;
		long   UerFlags;
	};
};

int SLAPI UserCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = (Data *)pEntry;
	PPObjSecur sec_obj(PPOBJ_USR, 0);
	PPSecur rec;
	if(sec_obj.Search(id, &rec) > 0) {
		p_cache_rec->PersonID = rec.PersonID;
		p_cache_rec->ParentID = rec.ParentID;
		p_cache_rec->UerID = rec.UerID; // @v8.6.1
		p_cache_rec->Flags = rec.Flags;
		p_cache_rec->UerFlags = rec.UerFlags; // @v8.6.1
		ok = PutName(rec.Name, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void SLAPI UserCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPSecur * p_data_rec = (PPSecur *)pDataRec;
	const Data * p_cache_rec = (const Data *)pEntry;
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->Tag   = ObjType;
	p_data_rec->ID    = p_cache_rec->ID;
	p_data_rec->PersonID = p_cache_rec->PersonID;
	p_data_rec->ParentID = p_cache_rec->ParentID;
	p_data_rec->UerID = p_cache_rec->UerID; // @v8.6.1
	p_data_rec->Flags = p_cache_rec->Flags;
	p_data_rec->UerFlags = p_cache_rec->UerFlags; // @v8.6.1
	GetName(pEntry, p_data_rec->Name, sizeof(p_data_rec->Name));
}
// }

int FASTCALL PPObjSecur::Fetch(PPID id, PPSecur * pRec)
{
	UserCache * p_cache = GetDbLocalCachePtr <UserCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}
//
// Implementation of PPALDD_User
//
PPALDD_CONSTRUCTOR(User)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(User)
{
	Destroy();
}

int PPALDD_User::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		H.ID = rFilt.ID;
		PPObjSecur sec_obj(PPOBJ_USR, 0);
		PPSecur rec;
		if(sec_obj.Fetch(rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Symb, rec.Symb); // @v9.5.2
			H.PersonID = rec.PersonID; // @v9.5.2
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

