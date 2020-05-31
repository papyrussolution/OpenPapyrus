// OBJWORLD.CPP
// Copyright (c) A.Sobolev, A.Starodub 2003, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPObjWorldObjStatus)
//
SLAPI PPObjWorldObjStatus::PPObjWorldObjStatus(void * extraPtr) : PPObjReference(PPOBJ_CITYSTATUS, extraPtr)
{
}

int SLAPI PPObjWorldObjStatus::SearchByCode(long code, long kind, PPID * pID, PPWorldObjStatus * pRec)
{
	int    ok = -1;
	PPID   id = 0;
	if(code) {
		ReferenceTbl::Key3 k3;
		k3.ObjType = Obj;
		k3.Val2 = code;
		if(ref->search(3, &k3, spEq)) {
			const PPWorldObjStatus * p_wos = reinterpret_cast<const PPWorldObjStatus *>(&ref->data);
			do {
				if((!kind || p_wos->Kind == kind) && !(pID && *pID && p_wos->ID == *pID)) {
					ASSIGN_PTR(pID, p_wos->ID);
					ref->copyBufTo(pRec);
					ok = 1;
				}
			} while(ok < 0 && ref->search(3, &k3, spNext) && p_wos->Tag == Obj && p_wos->Code == code);
		}
	}
	return ok;
}

int SLAPI PPObjWorldObjStatus::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel, valid_data = 0;
	PPWorldObjStatus rec;
	TDialog * dlg = new TDialog(DLG_WOBJSTATUS);
	THROW(CheckDialogPtr(&dlg));
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	else {
		MEMSZERO(rec);
		rec.Kind = WORLDOBJ_CITY;
	}
	dlg->setCtrlData(CTL_WOBJSTATUS_NAME, rec.Name);
	dlg->setCtrlData(CTL_WOBJSTATUS_ABBR, rec.Abbr);
	dlg->setCtrlData(CTL_WOBJSTATUS_CODE, &rec.Code);
	dlg->setCtrlData(CTL_WOBJSTATUS_ID,   &rec.ID); // read-only
	dlg->AddClusterAssoc(CTL_WOBJSTATUS_KIND, 0, WORLDOBJ_CONTINENT);
	dlg->AddClusterAssoc(CTL_WOBJSTATUS_KIND, 1, WORLDOBJ_COUNTRY);
	dlg->AddClusterAssoc(CTL_WOBJSTATUS_KIND, 2, WORLDOBJ_REGION);
	dlg->AddClusterAssoc(CTL_WOBJSTATUS_KIND, 3, WORLDOBJ_CITY);
	dlg->AddClusterAssoc(CTL_WOBJSTATUS_KIND, 4, WORLDOBJ_STREET);
	dlg->AddClusterAssoc(CTL_WOBJSTATUS_KIND, 5, WORLDOBJ_CITYAREA);
	dlg->SetClusterData(CTL_WOBJSTATUS_KIND, rec.Kind);
	while(!valid_data && ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTL_WOBJSTATUS_NAME, rec.Name);
		if(strip(rec.Name)[0] == 0)
			PPErrorByDialog(dlg, CTL_WOBJSTATUS_NAME, PPERR_NAMENEEDED);
		else if(!CheckName(*pID, rec.Name, 1))
			PPErrorByDialog(dlg, CTL_WOBJSTATUS_NAME);
		else {
			dlg->getCtrlData(CTL_WOBJSTATUS_ABBR, rec.Abbr);
			dlg->getCtrlData(CTL_WOBJSTATUS_CODE, &rec.Code);
			dlg->GetClusterData(CTL_WOBJSTATUS_KIND, &rec.Kind);
			PPID temp_id = rec.ID;
			if(SearchByCode(rec.Code, 0, &temp_id, 0) > 0)
				PPErrorByDialog(dlg, CTL_WOBJSTATUS_CODE, PPERR_DUPWORLDOBJSTATUSCODE);
			else {
				if(EditItem(Obj, *pID, &rec, 1)) {
					SETIFZ(*pID, rec.ID);
					valid_data = 1;
					ok = cmOK;
				}
				else
					PPError();
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPObjWorldObjStatus::AddSimple(PPID * pID, const char * pName, const char * pAbbr, long kind, long code, int use_ta)
{
	int    ok = -1;
	long   h = -1;
	PPID   id = 0;
	PPWorldObjStatus rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchByCode(code, kind, &(id = 0), &rec) > 0)
			ok = 1;
		else if(ref->SearchSymb(Obj, &(id = 0), pName, offsetof(PPWorldObjStatus, Name)) > 0)
			ok = 1;
		else {
			MEMSZERO(rec);
			rec.Tag = Obj;
			STRNSCPY(rec.Name, pName);
			STRNSCPY(rec.Abbr, pAbbr);
			rec.Kind = kind;
			rec.Code = code;
			THROW(ref->AddItem(Obj, &(id = 0), &rec, 0));
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pID, id);
	return ok;
}
//
//
//
SLAPI PPWorldPacket::PPWorldPacket()
{
	Init();
}

void SLAPI PPWorldPacket::Init()
{
	THISZERO();
}
//
//
//
PPID PPObjWorld::SelFilt::GetSingleKind() const
{
	PPID   k = 0;
	if(KindFlags) {
		for(uint i = 0; i < 32; i++) {
			if(KindFlags & (1U << i)) {
				if(k) {
					k = 0;
					break;
				}
				else
					k = (i+1);
			}
		}
	}
	return k;
}
//
//
//
//static
void * FASTCALL PPObjWorld::MakeExtraParam(int kind, PPID parentID, PPID countryID)
{
	uint   mask = 0;
	if(oneof6(kind, WORLDOBJ_CONTINENT, WORLDOBJ_GENREGION, WORLDOBJ_COUNTRY, WORLDOBJ_REGION, WORLDOBJ_CITY, WORLDOBJ_STREET))
		mask |= (1U << (kind-1));
	long   v = (mask << 24);
	if(countryID)
		v |= (0x80000000 | countryID);
	else
		v |= parentID;
	return reinterpret_cast<void *>(v);
}

//static
void * FASTCALL PPObjWorld::MakeExtraParam(const PPIDArray & rKindList, PPID parentID, PPID countryID)
{
	uint   mask = 0;
	for(uint i = 0; i < rKindList.getCount(); i++) {
		uint k = rKindList.get(i);
		if(oneof7(k, WORLDOBJ_CONTINENT, WORLDOBJ_GENREGION, WORLDOBJ_COUNTRY, WORLDOBJ_REGION, WORLDOBJ_CITY, WORLDOBJ_STREET, WORLDOBJ_CITYAREA)) {
			mask |= (1U << (k-1));
		}
	}
	long   v = (mask << 24);
	if(countryID)
		v |= (0x80000000 | countryID);
	else
		v |= parentID;
	return reinterpret_cast<void *>(v);
}

//static
int SLAPI PPObjWorld::ConvertExtraParam(void * extraPtr, SelFilt * pFilt)
{
	const long extra_param = reinterpret_cast<long>(extraPtr);
	pFilt->KindFlags = (extra_param & ~0x80000000) >> 24;
	if(extra_param & 0x80000000) {
		pFilt->CountryID = (extra_param & 0x00ffffff);
		pFilt->ParentID = 0;
	}
	else {
		pFilt->CountryID = 0;
		pFilt->ParentID = (extra_param & 0x00ffffff);
	}
	return 1;
}

//static
SString & SLAPI PPObjWorld::GetNativeCountryName(SString & rBuf)
{
	PPLoadString("nativecountry", rBuf);
	return rBuf;
}
//
//
//
TLP_IMPL(PPObjWorld, WorldTbl, P_Tbl);

SLAPI PPObjWorld::PPObjWorld(void * extraPtr) : PPObject(PPOBJ_WORLD), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

SLAPI PPObjWorld::~PPObjWorld()
{
	TLP_CLOSE(P_Tbl);
}

int SLAPI PPObjWorld::GetParentCountry(PPID id, PPID * pParentCountryID)
{
	PPID   temp_id = id;
	WorldTbl::Rec rec;
	while(temp_id && Search(temp_id, &rec) > 0)
		if(rec.Kind == WORLDOBJ_COUNTRY) {
			ASSIGN_PTR(pParentCountryID, temp_id);
			return 1;
		}
		else {
			temp_id = NZOR(rec.ParentID, rec.CountryID);
		}
	ASSIGN_PTR(pParentCountryID, 0);
	return -1;
}

int SLAPI PPObjWorld::GetNativeCountry(PPID * pID)
{
	int    ok = -1;
	PPID   main_org_id = 0;
	PPID   country_id = 0;
	if(GetMainOrgID(&main_org_id) > 0) {
		PPObjPerson psn_obj;
		if(psn_obj.GetCountry(main_org_id, &country_id, 0) > 0)
			ok = 1;
	}
	else {
		WorldTbl::Rec rec;
		SString temp_buf;
		PPObjWorld::GetNativeCountryName(temp_buf);
		if(SearchByName(WORLDOBJ_COUNTRY, temp_buf, &rec) > 0) {
			country_id = rec.ID;
			ok = 1;
		}
	}
	if(ok < 0) {
		PPSetError(PPERR_NONATIVECOUNTRY);
	}
	ASSIGN_PTR(pID, country_id);
	return ok;
}

PPObjWorld::SelFilt::SelFilt() : KindFlags(0), ParentID(0), CountryID(0)
{
}

class PPObjWorldListWindow : public PPObjListWindow {
public:
	static void * FASTCALL MakeExtraParam(PPID curId, void * extraPtr)
	{
		void * extra_ptr = extraPtr;
	  	PPObjWorld::SelFilt filt;
	  	PPObjWorld::ConvertExtraParam(extraPtr, &filt);
	  	if(!filt.GetSingleKind()) {
	  		PPObjWorld w_obj;
	  		PPIDArray kind_list;
	  		WorldTbl::Rec w_rec;
			if(w_obj.Fetch(curId, &w_rec) > 0) {
				filt.ParentID = curId;
				if(w_rec.Kind == WORLDOBJ_CITYAREA) {
					kind_list.addzlist(WORLDOBJ_CITY, WORLDOBJ_CITYAREA, 0L);
					extra_ptr = PPObjWorld::MakeExtraParam(kind_list, filt.ParentID, 0);
				}
				else if(w_rec.Kind == WORLDOBJ_CITY) {
					kind_list.addzlist(WORLDOBJ_CITY, WORLDOBJ_CITYAREA, 0L);
					extra_ptr = PPObjWorld::MakeExtraParam(kind_list, filt.ParentID, 0);
				}
				else if(w_rec.Kind == WORLDOBJ_COUNTRY) {
					kind_list.addzlist(WORLDOBJ_COUNTRY, WORLDOBJ_CITY, WORLDOBJ_REGION, WORLDOBJ_CITYAREA, 0L);
					extra_ptr = PPObjWorld::MakeExtraParam(kind_list, filt.ParentID, 0);
				}
			}
		}
		return extra_ptr;
	}
	PPObjWorldListWindow(PPObject * pObj, uint flags, void * extraPtr) : PPObjListWindow(pObj, flags, extraPtr)
	{
		DefaultCmd = cmaEdit;
		SetToolbar(TOOLBAR_LIST_WORLD);
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
						if(Flags & OLW_CANINSERT && P_Obj->Edit(&id, MakeExtraParam(id, ExtraPtr)) == cmOK)
							update = 2;
						else
							::SetFocus(H());
						break;
					case cmaMore:
						if(id) {
							/*
							long extra_param = PPObjWorld::MakeExtraParam(WORLDOBJ_STREET, parentID, 0);
							CheckExecAndDestroyDialog(new ObjWorldDialog((PPObjWorld *)ppobj, extra_param), 1, 1);
							*/
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
			PPObjWorld w_obj;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			StrAssocArray * p_list = w_obj.MakeStrAssocList(0);
			THROW(p_list);
			PPWait(1);
			for(uint i = 0; i < p_list->getCount(); i++) {
				THROW(objid_ary.Add(PPOBJ_WORLD, p_list->Get(i).Id));
			}
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
		CATCHZOKPPERR
		PPWait(0);
		return ok;
	}
};

class ObjWorldDialog : public ObjViewDialog {
public:
	ObjWorldDialog(PPObjWorld * pObj, void * extraPtr) : ObjViewDialog(DLG_WORLDVIEW, pObj, extraPtr)
	{
	}
private:
	virtual void addItem()
	{
		PPID   id = 0;
		if((Rt & PPR_INS) && P_List) {
			PPID cur_id = 0;
			P_List->getCurID(&cur_id);
			if(P_Obj->Edit(&id, PPObjWorldListWindow::MakeExtraParam(cur_id, ExtraPtr)) == cmOK)
				updateList(id);
		}
	}
	virtual int transmit(PPID id)
	{
		int    ok = -1;
		ObjTransmitParam param;
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			PPObjWorld w_obj;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			StrAssocArray * p_list = w_obj.MakeStrAssocList(0);
			THROW(p_list);
			PPWait(1);
			for(uint i = 0; i < p_list->getCount(); i++) {
				THROW(objid_ary.Add(PPOBJ_WORLD, p_list->Get(i).Id));
			}
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
		CATCHZOKPPERR
		PPWait(0);
		return ok;
	}
	virtual void extraProc(long parentID)
	{
		if(parentID) {
			void * extra_ptr = PPObjWorld::MakeExtraParam(WORLDOBJ_STREET, parentID, 0);
			CheckExecAndDestroyDialog(new ObjWorldDialog(static_cast<PPObjWorld *>(P_Obj), extra_ptr), 1, 1);
		}
	}
};

//virtual
void * SLAPI PPObjWorld::CreateObjListWin(uint flags, void * extraPtr)
{
	return /*0; */ new PPObjWorldListWindow(this, flags, extraPtr);
}

// static
int SLAPI PPObjWorld::UniteMaxLike()
{
	int    ok = 1;
	StrAssocArray * p_list = 0;
	if(CONFIRMCRIT(PPCFM_UNITELIKEWORLDS)) {
		PPObjWorld obj_world;
		PPIDArray kind_list;
		PPWait(1);
		kind_list.add(WORLDOBJ_STREET);
		kind_list.add(WORLDOBJ_CITY);
		p_list = obj_world.MakeStrAssocList(0);
		if(p_list) {
			SString msg;
			p_list->SortByID();
			PPLoadText(PPTXT_UNITELIKEWORLDS, msg);
			{
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < p_list->getCount(); i++) {
					long   like_flags = PPObjWorld::smlCode|PPObjWorld::smlName|smlCheckCountryOrParent;
					PPID   id = p_list->Get(i).Id;
					PPID   like_id = 0;
					PPWorldPacket pack;
					THROW(PPCheckUserBreak());
					if(obj_world.GetPacket(id, &pack) > 0) {
						while(obj_world.SearchMaxLike(&pack, like_flags, &like_id) > 0) {
							THROW(PPObject::ReplaceObj(PPOBJ_WORLD, like_id, id, 0));
							THROW_SL(p_list->Remove(like_id));
						}
					}
					PPWaitPercent(i, p_list->getCount() - 1, msg);
				}
				THROW(tra.Commit());
			}
		}
		PPWait(0);
	}
	CATCHZOKPPERR
	ZDELETE(p_list);
	return ok;
}

int SLAPI PPObjWorld::Browse(void * extraPtr)
{
	int    ok = cmCancel;
	if(extraPtr == 0) {
		PPIDArray kind_list;
		kind_list.addzlist(WORLDOBJ_CONTINENT, WORLDOBJ_COUNTRY, WORLDOBJ_REGION, WORLDOBJ_CITY, WORLDOBJ_CITYAREA, 0L);
		extraPtr = PPObjWorld::MakeExtraParam(kind_list, 0, 0);
	}
	THROW(CheckRights(PPR_READ));
	ok = CheckExecAndDestroyDialog(new ObjWorldDialog(this, extraPtr), 0, 0);
	CATCHZOK
	return ok;
}

class EditWorldDialog : public TDialog {
public:
	EditWorldDialog(uint resID) : TDialog(resID), DupID(0)
	{
		PPLoadText(PPTXT_WORLDKINDNAMES, KindNames);
	}
	int    setDTS(const PPWorldPacket *);
	int    getDTS(PPWorldPacket *);
	PPID   GetDupID() const
	{
		return DupID;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(TVBROADCAST && TVCMD == cmReleasedFocus) {
			if(TVINFOVIEW->TestId(CTL_WORLD_NAME)) {
				PPID   same_id = 0;
				if(CheckDuplicateName(&same_id) == 2) {
					DupID = same_id;
					endModal(cmOK);
					return; // После endModal не следует обращаться к this
				}
				clearEvent(event);
			}
		}
	}
	int CheckDuplicateName(PPID * pSelID);
	int SetGeoCoord();
	int GetGeoCoord();

	SString Name;
	SString KindNames;
	PPObjWorld ObjWorld;
	PPWorldPacket Data;
	//
	// Если при вводе нового объекта обнаружен дубликат и пользователь
	// подтвердил его использование, то DupID получает значение идентификатора
	// этого дубликата и диалог завершается с командой cmOK.
	//
	int    DupID;
};

int EditWorldDialog::CheckDuplicateName(PPID * pSelID)
{
	int    ok = -1;
	if(!Data.Rec.ID) {
		size_t name_len = 0;
		char   name[64];
		getCtrlData(CTL_WORLD_NAME, name);
		if(Name.CmpNC(name) != 0 && (name_len = sstrlen(name)) > 0) {
			long   i = 0;
			SString buf, parent_name;
			WorldTbl * p_tbl = ObjWorld.P_Tbl;
			StrAssocArray items_list;
			WorldTbl::Rec w_rec;
			WorldTbl::Key3 k3;
			BExtQuery q(p_tbl, 3);
			q.select(p_tbl->ID, p_tbl->Name, p_tbl->Kind, p_tbl->ParentID, p_tbl->CountryID, 0L).
				where(p_tbl->Kind == Data.Rec.Kind);
			MEMSZERO(k3);
			k3.Kind = Data.Rec.Kind;
			STRNSCPY(k3.Name, name);
			for(q.initIteration(0, &k3); q.nextIteration() > 0;) {
				p_tbl->copyBufTo(&w_rec);
				if(name_len == sstrlen(w_rec.Name) && stricmp866(name, w_rec.Name) == 0) {
					PPID   item_id = w_rec.ID;
					PPID   parent_id = NZOR(w_rec.ParentID, w_rec.CountryID);
					PPGetSubStr(KindNames, w_rec.Kind - 1, buf);
					buf.CatDiv('-', 1).Cat(w_rec.Name);
					while(parent_id && ObjWorld.Fetch(parent_id, &w_rec) > 0) {
						PPGetSubStr(KindNames, w_rec.Kind - 1, parent_name);
						buf.Comma().Space().Cat(parent_name).CatDiv(':', 2).Cat(w_rec.Name);
						parent_id = NZOR(w_rec.ParentID, w_rec.CountryID);
					}
					items_list.Add(item_id, buf);
				}
			}
			if(items_list.getCount())
				if(ListBoxSelDialog(DLG_DUPNAMES, &items_list, pSelID, 0) > 0)
					ok = 2;
				else
					ok = 1;
			Name = name;
		}
	}
	return ok;
}

int EditWorldDialog::SetGeoCoord()
{
	SString temp_buf;
	SGeoPosLL pos(Data.Rec.Latitude, Data.Rec.Longitude);
	pos.ToStr(temp_buf);
	return setCtrlString(CTL_WORLD_COORD, pos.ToStr(temp_buf));
}

int EditWorldDialog::GetGeoCoord()
{
	int    ok = -1;
	uint   sel;
	SString temp_buf, nmb;
	if(getCtrlString(sel = CTL_WORLD_COORD, temp_buf)) {
		SGeoPosLL pos;
		if(pos.FromStr(temp_buf)) {
			Data.Rec.Latitude = pos.Lat;
			Data.Rec.Longitude = pos.Lon;
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

int EditWorldDialog::setDTS(const PPWorldPacket * pData)
{
	long   parent_flags = OLW_CANINSERT|OLW_CANSELUPLEVEL;
	void * extra_ptr = 0;
	SString temp_buf;
	PPIDArray kind_list;
	if(pData)
		Data = *pData;
	else
		Data.Init();
	setCtrlData(CTL_WORLD_NAME, Data.Rec.Name);
	setCtrlData(CTL_WORLD_ABBR, Data.Rec.Abbr);
	setCtrlData(CTL_WORLD_CODE, Data.Rec.Code);
	setCtrlData(CTL_WORLD_ZIP,  Data.Rec.ZIP);
	SetGeoCoord();
	setStaticText(CTL_WORLD_ID, temp_buf.Z().Cat(Data.Rec.ID));
	SetupPPObjCombo(this, CTLSEL_WORLD_STATUS, PPOBJ_CITYSTATUS, Data.Rec.Status, OLW_CANINSERT, 0);
	if(oneof2(Id, DLG_STREET, DLG_CITYAREA)) {
		extra_ptr = PPObjWorld::MakeExtraParam(WORLDOBJ_CITY, 0, 0);
		parent_flags = OLW_CANINSERT;
	}
	else {
		setCtrlData(CTL_WORLD_PHONE, Data.Rec.Phone);
		if(Id == DLG_COUNTRY)
			extra_ptr = PPObjWorld::MakeExtraParam(WORLDOBJ_CONTINENT, 0, 0);
		else if(oneof2(Id, DLG_CITY, DLG_REGION)) {
			SetupPPObjCombo(this, CTLSEL_WORLD_STATUS,  PPOBJ_CITYSTATUS, Data.Rec.Status, OLW_CANINSERT); // @v10.5.4 @fix PPOBJ_STATUS-->PPOBJ_CITYSTATUS
			SetupPPObjCombo(this, CTLSEL_WORLD_COUNTRY, PPOBJ_WORLD,  Data.Rec.CountryID, OLW_CANINSERT|OLW_CANSELUPLEVEL|OLW_WORDSELECTOR,
				PPObjWorld::MakeExtraParam(WORLDOBJ_COUNTRY, 0, 0)); // @v10.7.8 OLW_WORDSELECTOR
			kind_list.addzlist(WORLDOBJ_REGION, WORLDOBJ_COUNTRY, 0);
			extra_ptr = PPObjWorld::MakeExtraParam(kind_list, 0, 0);
		}
	}
	if(extra_ptr)
		SetupPPObjCombo(this, CTLSEL_WORLD_PARENT, PPOBJ_WORLD, Data.Rec.ParentID, parent_flags|OLW_WORDSELECTOR, extra_ptr); // @v10.7.8 OLW_WORDSELECTOR
	if(oneof2(Data.Rec.Kind, WORLDOBJ_STREET, WORLDOBJ_CITYAREA)) {
		if(Data.Rec.ParentID)
			selectCtrl(CTL_WORLD_NAME);
	}
	return 1;
}

int EditWorldDialog::getDTS(PPWorldPacket * pData)
{
	int    ok = 1;
	uint   sel = 0;
	getCtrlData(sel = CTL_WORLD_NAME, Data.Rec.Name);
	THROW_PP(sstrlen(Data.Rec.Name), PPERR_NAMENEEDED);
	getCtrlData(CTL_WORLD_ABBR, Data.Rec.Abbr);
	getCtrlData(CTL_WORLD_CODE, Data.Rec.Code);
	getCtrlData(CTL_WORLD_ZIP,  Data.Rec.ZIP);
	sel = CTL_WORLD_COORD;
	THROW(GetGeoCoord());
	getCtrlData(CTLSEL_WORLD_STATUS, &Data.Rec.Status);
	getCtrlData(sel = CTLSEL_WORLD_PARENT,  &Data.Rec.ParentID);
	if(oneof3(Id, DLG_CITY, DLG_REGION, DLG_COUNTRY)) {
		getCtrlData(CTL_WORLD_PHONE, Data.Rec.Phone);
		getCtrlData(sel = CTLSEL_WORLD_COUNTRY, &Data.Rec.CountryID);
		if(Id == DLG_CITY || Id == DLG_REGION) {
			getCtrlData(CTLSEL_WORLD_STATUS, &Data.Rec.Status);
			THROW_PP(Data.Rec.ParentID || Data.Rec.CountryID, PPERR_REGIONORCNTRYINV);
			if(Data.Rec.ParentID && Data.Rec.CountryID) {
				PPID   parent_country_id = 0;
				THROW(ObjWorld.GetParentCountry(Data.Rec.ParentID, &parent_country_id) > 0);
				THROW_PP(Data.Rec.CountryID == parent_country_id, PPERR_CITYREGIONINV);
			}
		}
	}
	else if(oneof2(Id, DLG_STREET, DLG_CITYAREA))
		THROW_PP(Data.Rec.ParentID, PPERR_CITYNEEDED);
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

int SLAPI PPObjWorld::Edit(PPID * pID, void * extraPtr /*parent*/)
{
	int    ok = 0, edit = 0;
	int    is_new = BIN(!pID || !(*pID));
	PPID   id = is_new ? 0 : *pID;
	PPID   world_kind = 0;
	PPWorldPacket pack;
	EditWorldDialog * p_dlg = 0;
	if(is_new) {
		uint   val = 0;
		SelFilt filt;
		PPObjWorld::ConvertExtraParam(extraPtr, &filt);
		world_kind = filt.GetSingleKind();
		if(world_kind) {
			edit = 1;
		}
		else if(SelectorDialog(DLG_SELWORLD, CTL_SELWORLD_TYPE, &val) > 0) {
			world_kind = (val == 0) ? (val+1) : (val+2);
			edit = 1;
		}
		if(world_kind) {
			int    is_parent = 0;
			WorldTbl::Rec parent_rec;
			pack.Rec.Kind = world_kind;
			// @v10.7.9 @ctr MEMSZERO(parent_rec);
			if(Fetch(filt.ParentID, &parent_rec) > 0)
				is_parent = 1;
			if(world_kind == WORLDOBJ_CITYAREA) {
				if(is_parent) {
					if(parent_rec.Kind == WORLDOBJ_CITYAREA) {
						pack.Rec.CountryID = parent_rec.CountryID;
						pack.Rec.ParentID = parent_rec.ParentID;
					}
					else if(parent_rec.Kind == WORLDOBJ_CITY) {
						pack.Rec.CountryID = parent_rec.CountryID;
						pack.Rec.ParentID = parent_rec.ID;
					}
				}
				else
					pack.Rec.CountryID = filt.CountryID;
			}
			else if(world_kind == WORLDOBJ_CITY) {
				if(is_parent) {
					if(parent_rec.Kind == WORLDOBJ_CITY) {
						pack.Rec.CountryID = parent_rec.CountryID;
						pack.Rec.ParentID = parent_rec.ParentID;
					}
					else if(parent_rec.Kind == WORLDOBJ_REGION) {
						pack.Rec.CountryID = parent_rec.CountryID;
						pack.Rec.ParentID = parent_rec.ID;
					}
				}
				else
					pack.Rec.CountryID = filt.CountryID;
			}
			else if(world_kind == WORLDOBJ_REGION) {
				if(is_parent) {
					if(parent_rec.Kind == WORLDOBJ_REGION) {
						pack.Rec.CountryID = parent_rec.CountryID;
						pack.Rec.ParentID = parent_rec.ParentID;
					}
					else if(parent_rec.Kind == WORLDOBJ_COUNTRY)
						pack.Rec.CountryID = parent_rec.ID;
				}
				else
					pack.Rec.CountryID = filt.CountryID;
			}
			else if(world_kind == WORLDOBJ_COUNTRY) {
				if(is_parent) {
					if(parent_rec.Kind == WORLDOBJ_COUNTRY)
						pack.Rec.ParentID = parent_rec.ParentID;
					else if(parent_rec.Kind == WORLDOBJ_CONTINENT)
						pack.Rec.ParentID = parent_rec.ID;
				}
			}
		}
	}
	else {
		THROW(Search(id, &pack) > 0);
		world_kind = pack.Rec.Kind;
		edit = 1;
	}
	if(edit) {
		PPID   dup_id = 0;
		uint   dlg_id = 0;
		switch(world_kind) {
			case WORLDOBJ_CONTINENT: dlg_id = DLG_CONTINENT; break;
			case WORLDOBJ_COUNTRY:   dlg_id = DLG_COUNTRY;   break;
			case WORLDOBJ_REGION:    dlg_id = DLG_REGION;    break;
			case WORLDOBJ_CITY:      dlg_id = DLG_CITY;      break;
			case WORLDOBJ_STREET:    dlg_id = DLG_STREET;    break;
			case WORLDOBJ_CITYAREA:  dlg_id = DLG_CITYAREA;  break;
			default: dlg_id = DLG_CITY; break;
		}
		THROW(CheckDialogPtr(&(p_dlg = new EditWorldDialog(dlg_id))));
		p_dlg->setDTS(&pack);
		while(ok <= 0 && ExecView(p_dlg) == cmOK) {
			dup_id = p_dlg->GetDupID();
			if(dup_id || p_dlg->getDTS(&pack))
				ok = 1;
		}
		if(ok == 1) {
			if(!dup_id)
				THROW(PutPacket(&id, &pack, 1));
			ASSIGN_PTR(pID, NZOR(dup_id, id));
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return (ok > 0) ? cmOK : cmCancel;
}

int SLAPI PPObjWorld::Search(PPID id, void * b)
{
	return SearchByID(P_Tbl, Obj, id, b);
}

int SLAPI PPObjWorld::SearchByCode(const char * pCode, WorldTbl::Rec * pRec)
{
	int    ok = -1;
	if(pCode) {
		const size_t len = sstrlen(pCode);
		if(len > 0 && len < sizeof(P_Tbl->data.Code)) {
			WorldTbl::Key4 k4;
			MEMSZERO(k4);
			STRNSCPY(k4.Code, pCode);
			if(SearchByKey(P_Tbl, 4, &k4, 0) > 0) {
				ASSIGN_PTR(pRec, P_Tbl->data);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPObjWorld::SearchCountry(const char * pName, const char * pCode, const char * pAlpha2, WorldTbl::Rec * pRec)
{
	int    ok = -1;
	PPID   id = 0;
	if(!isempty(pName)) {
		SearchByName(WORLDOBJ_COUNTRY, pName, pRec);
		SVector list(sizeof(WorldTbl::Rec)); // @v10.6.7 SArray-->SVector
		if(GetListByName(WORLDOBJ_COUNTRY, pName, &list) > 0) {
			uint pos = 0;
			if(list.getCount() > 1) {
				for(uint i = 0; i < list.getCount(); i++) {
					const WorldTbl::Rec * p_rec = static_cast<const WorldTbl::Rec *>(list.at(i));
					if(!isempty(pCode) && stricmp(p_rec->Code, pCode) == 0) {
						pos = i;
						break;
					}
					else if(!isempty(pAlpha2) && stricmp(p_rec->Abbr, pAlpha2) == 0) {
						pos = i;
						break;
					}
				}
			}
			if(pos < list.getCount()) {
				const WorldTbl::Rec * p_rec = static_cast<const WorldTbl::Rec *>(list.at(pos));
				id = p_rec->ID;
				ASSIGN_PTR(pRec, *p_rec);
				ok = 1;
			}
		}
	}
	if(ok < 0) {
		if(!isempty(pCode) && SearchByCode(pCode, pRec) > 0) {
			ok = 2;
		}
		else if(!isempty(pAlpha2)) {
			BExtQuery q(P_Tbl, 3);
			WorldTbl::Key3 k3;
			MEMSZERO(k3);
			k3.Kind = WORLDOBJ_COUNTRY;
			q.selectAll().where(P_Tbl->Kind == (long)WORLDOBJ_COUNTRY);
			for(q.initIteration(0, &k3, spGe); q.nextIteration() > 0;) {
				if(stricmp(P_Tbl->data.Abbr, pAlpha2) == 0) {
					ASSIGN_PTR(pRec, P_Tbl->data);
					ok = 3;
					break;
				}
			}
		}
	}
	return ok;
}

int SLAPI PPObjWorld::SearchByName(int kind, const char * pName, WorldTbl::Rec * pRec)
{
	SVector list(sizeof(WorldTbl::Rec)); // @v10.6.7 SArray-->SVector
	int    ok = GetListByName(kind, pName, &list);
	if(ok > 0) {
		if(list.getCount()) {
			ASSIGN_PTR(pRec, *static_cast<const WorldTbl::Rec *>(list.at(0)));
		}
		else
			ok = -1;
	}
	return ok;
}

int SLAPI PPObjWorld::GetListByName(int kind, const char * pName, SVector * pList) // @v10.6.7 SArray-->SVector
{
	int    ok = -1;
	WorldTbl::Key3 k3;
	MEMSZERO(k3);
	k3.Kind = kind;
	STRNSCPY(k3.Name, pName);
	if(P_Tbl->search(3, &k3, spEq))
		do {
			THROW_SL(pList->insert(&P_Tbl->data));
		} while(P_Tbl->search(3, &k3, spNext) && k3.Kind == kind && stricmp866(k3.Name, pName) == 0);
	ok = (pList->getCount()) ? 1 : -1;
	CATCHZOK
	return ok;
}

// @Muxa {
int SLAPI PPObjWorld::GetListByFilt(const SelFilt & rFilt, SVector * pList) // @v10.6.7 SArray-->SVector
{
	int    ok = -1;
	WorldTbl::Rec * p_rec;
	WorldTbl::Key3 k3;
	MEMSZERO(k3);
	k3.Kind = rFilt.KindFlags;
	if(P_Tbl->search(3, &k3, spGe)) {
		do {
			p_rec = &P_Tbl->data;
			if(rFilt.ParentID == 0 || rFilt.ParentID == p_rec->ParentID) {
				if(rFilt.CountryID == 0 || rFilt.CountryID == p_rec->CountryID) {
					if(rFilt.SubName.Empty() || ExtStrSrch(p_rec->Name, rFilt.SubName.cptr(), 0) > 0)
						THROW_SL(pList->insert(&P_Tbl->data));
				}
			}
		} while(P_Tbl->search(3, &k3, spNext) && k3.Kind == rFilt.KindFlags);
	}
	ok = (pList->getCount()) ? 1 : -1;
	CATCHZOK
	return ok;
}
// } @Muxa

int SLAPI PPObjWorld::GetListByCode(int kind, const char * pCode, SVector * pList) // @v10.6.7 SArray-->SVector
{
	int    ok = -1;
	size_t len = sstrlen(pCode);
	if(len > 0 && len < sizeof(P_Tbl->data.Code)) {
		const WorldTbl::Rec & r_rec = P_Tbl->data;
		WorldTbl::Key4 k4;
		STRNSCPY(k4.Code, pCode);
		if(P_Tbl->search(4, &k4, spEq))
			do {
				if(!kind || kind == r_rec.Kind)
					THROW_SL(pList->insert(&P_Tbl->data));
			} while(P_Tbl->search(4, &k4, spNext) && (!kind || r_rec.Kind == kind) && len == sstrlen(k4.Code) && stricmp866(k4.Code, pCode) == 0);
	}
	ok = (pList->getCount()) ? 1 : -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjWorld::GetCountryByChild(PPID id, WorldTbl::Rec * pCountryRec)
{
	WorldTbl::Rec rec;
	while(id && Fetch(id, &rec) > 0)
		if(rec.Kind == WORLDOBJ_COUNTRY) {
			ASSIGN_PTR(pCountryRec, rec);
			return 1;
		}
		else
			id = NZOR(rec.CountryID, rec.ParentID);
	return -1;
}

int SLAPI PPObjWorld::Helper_IsChildOf(PPID id, PPID parentID, PPIDArray * pRecurTrace)
{
	int    ok = 0;
	if(id == parentID)
		ok = 1;
	else {
		WorldTbl::Rec rec;
		if(pRecurTrace && pRecurTrace->lsearch(id)) {
			SString added_buf;
			if(Fetch(id, &rec) > 0)
				added_buf.Cat(rec.Name);
			else
				ideqvalstr(id, added_buf);
			PPSetError(PPERR_CYCLEWORLDOBJ, added_buf);
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
			ok = 0;
		}
		else {
			pRecurTrace->add(id);
			if(Fetch(id, &rec) > 0) {
				if(oneof2(parentID, rec.CountryID, rec.ParentID))
					ok = 1;
				else if(rec.CountryID && !pRecurTrace->lsearch(rec.CountryID) && Helper_IsChildOf(rec.CountryID, parentID, pRecurTrace)) // @recursion
					ok = 1;
				else if(rec.ParentID && !pRecurTrace->lsearch(rec.ParentID) && Helper_IsChildOf(rec.ParentID, parentID, pRecurTrace)) // @recursion
					ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI PPObjWorld::IsChildOf(PPID id, PPID parentID)
{
	PPIDArray recur_trace;
	return Helper_IsChildOf(id, parentID, &recur_trace);
}

int SLAPI PPObjWorld::DeleteObj(PPID id)
{
	return PutPacket(&id, 0, 0);
}

int SLAPI PPObjWorld::PutPacket(PPID * pID, PPWorldPacket * pPack, int useTa)
{
	int    ok = 1;
	if(pID) {
		PPID   action = 0;
		{
			PPTransaction tra(useTa);
			THROW(tra);
			if(*pID) {
				WorldTbl::Rec rec;
				THROW(Search(*pID, &rec) > 0);
				if(!pPack) {
					PPIDArray list;
					THROW(CheckRights(PPR_DEL));
					THROW(GetChildList(*pID, &list, 0));
					for(uint i = 0; i < list.getCount(); i++) {
						const PPID id = list.at(i);
						if(Search(id, 0) > 0) {
							THROW(RemoveObjV(id, 0, PPObject::not_addtolog | PPObject::not_checkrights, 0));
						}
					}
					/*
					if(list.getCount()) {
						PPObject::SetLastErrObj(PPOBJ_WORLD, list.at(0));
						CALLEXCEPT_PP(PPERR_REFSEXISTS);
					}
					*/
					THROW(RemoveByID(P_Tbl, *pID, 0));
					action = PPACN_OBJRMV;
				}
				else if(memcmp(&pPack->Rec, &rec, sizeof(rec)) != 0) {
					THROW(CheckRights(PPR_MOD)); // @v8.1.2
					THROW(UpdateByID(P_Tbl, Obj, *pID, &pPack->Rec, 0));
					action = PPACN_OBJUPD;
				}
				else
					ok = -1;
			}
			else if(pPack) {
				THROW(CheckRights(PPR_INS)); // @v8.1.2
				THROW(AddObjRecByID(P_Tbl, Obj, pID, &pPack->Rec, 0));
				action = PPACN_OBJADD;
			}
			DS.LogAction(action, PPOBJ_WORLD, *pID, 0, 0);
			THROW(tra.Commit());
		}
		if(action)
			Dirty(*pID);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjWorld::AddSimple(PPID * pID, int kind, const char * pName, const char * pCountry, int use_ta)
{
	int    ok = 1;
	PPID   id = 0, country_id = 0;
	SString country_name;
	SVector list(sizeof(WorldTbl::Rec)); // @v10.6.7 SArray-->SVector
	THROW(GetListByName(kind, pName, &list));
	if(pCountry) {
		country_name = pCountry;
	}
	else {
		PPObjWorld::GetNativeCountryName(country_name);
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(kind == WORLDOBJ_COUNTRY) {
			SETIFZ(pCountry, country_name); // @redundant
		}
		else if(oneof2(kind, WORLDOBJ_CITY, WORLDOBJ_REGION)) {
			THROW(AddSimple(&country_id, WORLDOBJ_COUNTRY, country_name, 0, 0)); // @recursion
		}
		if(list.getCount()) {
			if(country_id) {
				WorldTbl::Rec * p_item;
				for(uint i = 0; !id && list.enumItems(&i, (void **)&p_item);) {
					if(oneof2(country_id, p_item->CountryID, p_item->ParentID))
						id = p_item->ID;
				}
			}
			else
				id = static_cast<const WorldTbl::Rec *>(list.at(0))->ID;
		}
		if(id == 0) {
			PPWorldPacket pack;
			pack.Rec.Kind = kind;
			pack.Rec.CountryID = country_id;
			STRNSCPY(pack.Rec.Name, pName);
			if(kind == WORLDOBJ_CITY)
				pack.Rec.Status = PPCTS_CITY;
			THROW(PutPacket(&id, &pack, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pID, id);
	return ok;
}

int SLAPI PPObjWorld::GetPacket(PPID id, PPWorldPacket * pPack)
{
	int    ok = -1;
	if(id) {
		PPWorldPacket pack;
		if(Search(id, &pack.Rec) > 0) {
			ok = 1;
			ASSIGN_PTR(pPack, pack);
		}
	}
	return ok;
}

int SLAPI PPObjWorld::GetChildList(PPID id, PPIDArray * pChildList, PPIDArray * pStack)
{
	int    ok = -1;
	PPIDArray inner_stack;
	{
		WorldTbl::Key1 k1;
		BExtQuery q(P_Tbl, 1);
		q.select(P_Tbl->ID, 0).where(P_Tbl->ParentID == id);
		MEMSZERO(k1);
		k1.ParentID = id;
		for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
			pChildList->add(P_Tbl->data.ID);
			ok = 1;
		}
	}
	{
		WorldTbl::Key2 k2;
		BExtQuery q(P_Tbl, 2);
		q.select(P_Tbl->ID, 0).where(P_Tbl->CountryID == id);
		MEMSZERO(k2);
		k2.CountryID = id;
		for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
			pChildList->add(P_Tbl->data.ID);
			ok = 1;
		}
	}
	pChildList->sortAndUndup();
	SETIFZ(pStack, &inner_stack);
	if(pStack->lsearch(id)) {
		SString added_buf;
		WorldTbl::Rec rec;
		if(Fetch(id, &rec) > 0)
			added_buf.Cat(rec.Name);
		else
			ideqvalstr(id, added_buf);
		PPSetError(PPERR_CYCLEWORLDOBJ, added_buf);
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
		CALLEXCEPT();
	}
	else {
		PPIDArray child_list;
		THROW_SL(pStack->add(id));
		for(uint i = 0; i < pChildList->getCount(); i++) {
			THROW(GetChildList(pChildList->get(i), &child_list, pStack)); // @recursion
		}
		THROW_SL(pChildList->add(&child_list));
		pChildList->sortAndUndup();
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjWorld::ValidateSelection(PPID id, uint olwFlags, void * extraPtr)
{
	int    ok = 0;
	WorldTbl::Rec rec;
	if(Search(id, &rec) > 0) {
		SelFilt sf;
		ConvertExtraParam(extraPtr, &sf);
		if(sf.KindFlags) {
			if(sf.KindFlags & (1L << (rec.Kind-1)))
				ok = 1;
			else if(olwFlags & OLW_CANSELUPLEVEL)
				ok = 1;
		}
		else
			ok = 1;
	}
	return ok;
}

int SLAPI PPObjWorld::AddItemToSelectorList(const WorldTbl::Rec & rRec, AislBlock & rBlk)
{
	int    ok = -1;
	if(!rBlk.El.Has(rRec.ID)) {
		PPID   parent_id = rBlk.UseHierarchy ? NZOR(rRec.ParentID, rRec.CountryID) : 0;
		if(parent_id) {
			WorldTbl::Rec parent_rec;
			if(Fetch(parent_id, &parent_rec) > 0) {
				if(rBlk.Stack.lsearch(parent_id)) {
					SString added_buf;
					(added_buf = rRec.Name).CatChar('-').CatChar('>');
					added_buf.Cat(parent_rec.Name);
					PPSetError(PPERR_CYCLEWORLDOBJ, added_buf);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
					parent_id = 0;
				}
				else {
					rBlk.Stack.add(parent_id);
					THROW(AddItemToSelectorList(parent_rec, rBlk)); // @recursion
				}
			}
			else
				parent_id = 0;
		}
		THROW_SL(rBlk.P_List->AddFast(rRec.ID, parent_id, rRec.Name));
		rBlk.El.Add(rRec.ID);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjWorld::AddItemToSelectorList(PPID id, StrAssocArray * pList, int useHierarchy, PPIDArray * pStack)
{
	int    ok = -1;
	if(!pList->Search(id)) {
		WorldTbl::Rec rec;
		if(Fetch(id, &rec) > 0) {
			PPID   parent_id = useHierarchy ? NZOR(rec.ParentID, rec.CountryID) : 0;
			if(parent_id) {
				PPIDArray inner_stack;
				if(pStack == 0) {
					inner_stack.add(id);
					pStack = &inner_stack;
				}
				if(pStack->lsearch(parent_id)) {
					SString added_buf;
					WorldTbl::Rec parent_rec;
					(added_buf = rec.Name).CatChar('-').CatChar('>');
					if(Fetch(parent_id, &parent_rec) > 0)
						added_buf.Cat(parent_rec.Name);
					else
						ideqvalstr(parent_id, added_buf);
					PPSetError(PPERR_CYCLEWORLDOBJ, added_buf);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
					parent_id = 0;
				}
				else {
					pStack->add(parent_id);
					THROW(AddItemToSelectorList(parent_id, pList, useHierarchy, pStack)); // @recursion
				}
			}
			THROW_SL(pList->AddFast(id, parent_id, rec.Name)); // @v7.9.0 Add-->AddFast
		}
	}
	CATCHZOK
	return ok;
}

StrAssocArray * SLAPI PPObjWorld::MakeStrAssocList(void * extraPtr)
{
	SelFilt sf;
	ConvertExtraParam(extraPtr, &sf);
	StrAssocArray * p_list = new StrAssocArray;
	WorldTbl::Key1 k1;
	BExtQuery q(P_Tbl, 1);
	DBQ * dbq = 0;
	THROW_MEM(p_list);
	MEMSZERO(k1);
	q.select(P_Tbl->ID, P_Tbl->Name, P_Tbl->ParentID, P_Tbl->CountryID, 0L);
	dbq = &(*dbq && P_Tbl->Kind > 0L);
	dbq = ppcheckfiltid(dbq, P_Tbl->CountryID, sf.CountryID);
	dbq = ppcheckfiltid(dbq, P_Tbl->ParentID, sf.ParentID);
	if(sf.KindFlags) {
		PPIDArray kind_list;
		for(uint i = 0; i < 8; i++)
			if(sf.KindFlags & (1L << i))
				kind_list.add(i+1);
		dbq = ppcheckfiltidlist(dbq, P_Tbl->Kind, &kind_list);
	}
	q.where(*dbq);
	MEMSZERO(k1);
	/*
	{
		const int use_hierarchy = BIN(sf.ParentID == 0 && sf.CountryID == 0);
		for(q.initIteration(0, &k1, spGt); q.nextIteration() > 0;) {
			THROW(AddItemToSelectorList(P_Tbl->data.ID, p_list, use_hierarchy, 0));
		}
	}
	*/
	//
	{
		WorldTbl::Rec rec;
		AislBlock blk;
		blk.P_List = p_list;
		blk.UseHierarchy = BIN(sf.ParentID == 0 && sf.CountryID == 0);
		for(q.initIteration(0, &k1, spGt); q.nextIteration() > 0;) {
			blk.Stack.clear();
			P_Tbl->copyBufTo(&rec);
			THROW(AddItemToSelectorList(rec, blk));
		}
	}
	//
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int SLAPI PPObjWorld::SearchMaxLike(const PPWorldPacket * pPack, long flags, PPID * pID)
{
	int    ok = -1;
	PPIDArray excl_list;
	WorldTbl::Rec rec;

#define CHECK_PARAM \
	if(flags & smlCheckCountryOrParent) \
		ok = (NZOR(pPack->Rec.ParentID, pPack->Rec.CountryID) == NZOR(rec.ParentID, rec.CountryID)) ? 1 : -1; \
	else if(flags & smlCheckParent) \
		ok = (pPack->Rec.ParentID == rec.ParentID) ? 1 : -1; \
	else if(flags & smlCheckCountry) \
		ok = (pPack->Rec.CountryID == rec.CountryID) ? 1 : -1;

	SETIFZ(flags, smlCode|smlName|smlCheckCountry);
	excl_list.add(pPack->Rec.ID);
	if(flags & smlCode) {
		SVector list(sizeof(WorldTbl::Rec)); // @v10.6.7 SArray-->SVector
		if(GetListByCode(pPack->Rec.Kind, pPack->Rec.Code, &list) > 0) {
			uint count = list.getCount();
			for(uint i = 0; ok < 0 && i < count; i++) {
				rec = *static_cast<const WorldTbl::Rec *>(list.at(i));
				if(rec.ID != pPack->Rec.ID)
					CHECK_PARAM;
			}
		}
	}
	if(ok < 0 && flags & smlName) {
		SVector list(sizeof(WorldTbl::Rec)); // @v10.6.7 SArray-->SVector
		if(GetListByName(pPack->Rec.Kind, pPack->Rec.Name, &list) > 0) {
			uint count = list.getCount();
			for(uint i = 0; ok < 0 && i < count; i++) {
				rec = *static_cast<const WorldTbl::Rec *>(list.at(i));
				if(rec.ID != pPack->Rec.ID)
					CHECK_PARAM;
			}
		}
	}
	if(ok > 0)
		ASSIGN_PTR(pID, rec.ID);
	return ok;
}

int SLAPI PPObjWorld::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new PPWorldPacket);
	{
		PPWorldPacket * p_pack = static_cast<PPWorldPacket *>(p->Data);
		if(stream == 0) {
			THROW(GetPacket(id, p_pack) > 0);
		}
		else {
			SBuffer buffer;
			THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0));
			THROW_SL(P_Tbl->SerializeRecord(-1, &p_pack->Rec, buffer, &pCtx->SCtx));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjWorld::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data) {
		PPWorldPacket * p_pack = static_cast<PPWorldPacket *>(p->Data);
		if(stream == 0) {
			if(*pID || SearchMaxLike(p_pack, 0, pID) > 0) {
				p_pack->Rec.ID = *pID;
				int r = PutPacket(pID, p_pack, 1);
				if(!r) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTWORLD, p_pack->Rec.ID, p_pack->Rec.Name);
					THROW(*pID);
					ok = -1;
				}
				else if(r > 0)
					ok = 102; // @ObjectUpdated Пакет действительно был изменен
			}
			else {
				p_pack->Rec.ID = *pID = 0;
				if(!PutPacket(pID, p_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTWORLD, p_pack->Rec.ID, p_pack->Rec.Name);
					ok = -1;
				}
				else
					ok = 101;
			}
		}
		else {
			SBuffer buffer;
			THROW_SL(P_Tbl->SerializeRecord(+1, &p_pack->Rec, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjWorld::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		PPWorldPacket * gp = static_cast<PPWorldPacket *>(p->Data);
		ProcessObjRefInArray(PPOBJ_WORLD, &gp->Rec.ParentID,  ary, replace);
		ProcessObjRefInArray(PPOBJ_WORLD, &gp->Rec.CountryID, ary, replace);
		ProcessObjRefInArray(PPOBJ_CURRENCY, &gp->Rec.CurrencyID, ary, replace);
		return 1;
	}
	return -1;
}

int SLAPI PPObjWorld::Unite(PPID destID, PPID srcID)
{
	int    ok = 1;
	if(Obj == PPOBJ_WORLD) {
		PPID k = 0;
		while(ok && P_Tbl->search(0, &k, spGt)) {
			int update = 0;
			if(P_Tbl->data.CountryID == destID) {
				P_Tbl->data.CountryID = srcID;
				update = 1;
			}
			if(P_Tbl->data.ParentID == destID) {
				P_Tbl->data.ParentID = srcID;
				update = 1;
			}
			if(update && !P_Tbl->updateRec())
				ok = PPSetErrorDB();
		}
		if(ok > 0)
			ok = BroadcastObjMessage(DBMSG_OBJREPLACE, Obj, destID, reinterpret_cast<void *>(srcID));
	}
	return ok;
}

int SLAPI PPObjWorld::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	if(msg == DBMSG_OBJDELETE) {
		union {
			WorldTbl::Key0 k0;
			WorldTbl::Key1 k1;
			WorldTbl::Key2 k2;
		} k;
		if(_obj == PPOBJ_WORLD) {
			MEMSZERO(k);
			k.k1.ParentID = _id;
			if(P_Tbl->search(1, &k, spGe) && k.k1.ParentID == _id) {
				return RetRefsExistsErr(Obj, P_Tbl->data.ID);
			}
			else {
				MEMSZERO(k);
				k.k2.CountryID = _id;
				if(P_Tbl->search(2, &k, spGe) && k.k2.CountryID == _id) {
					return RetRefsExistsErr(Obj, P_Tbl->data.ID);
				}
			}
		}
		else if(_obj == PPOBJ_CITYSTATUS) {
			MEMSZERO(k);
			BExtQuery q(P_Tbl, 0);
			q.selectAll().where(P_Tbl->Status == _id);
			q.initIteration(0, &k, spFirst);
			if(q.nextIteration() > 0) {
				return RetRefsExistsErr(Obj, P_Tbl->data.ID);
			}
		}
		return DBRPL_OK;
	}
	if(msg == DBMSG_OBJREPLACE)
		switch(_obj) {
			case PPOBJ_WORLD:
				return Unite(_id, reinterpret_cast<long>(extraPtr)) ? DBRPL_OK : DBRPL_ERROR;
			default:
				break;
		}
	return DBRPL_OK;
}

IMPL_DESTROY_OBJ_PACK(PPObjWorld, PPWorldPacket);

const char * SLAPI PPObjWorld::GetNamePtr()
{
	return P_Tbl->data.Name;
}
//
// @todo Срочно!!!
//
int SLAPI PPObjWorld::Recover(PPLogger * pLogger)
{
    int    ok = -1;
    return ok;
}
//
//
//
class WorldCache : public ObjCache {
public:
	SLAPI  WorldCache() : ObjCache(PPOBJ_WORLD, sizeof(WorldData))
	{
	}
private:
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct WorldData : public ObjCacheEntry {
		long   Kind;
		long   ParentID;
		long   CountryID;
		long   Status;
		long   Flags;
		long   CurrencyID;
		double Latitude;
		double Longitude;
	};
};

int SLAPI WorldCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	WorldData * p_cache_rec = static_cast<WorldData *>(pEntry);
	PPObjWorld w_obj;
	WorldTbl::Rec rec;
	if(w_obj.Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
		CPY_FLD(Kind);
		CPY_FLD(ParentID);
		CPY_FLD(CountryID);
		CPY_FLD(Status);
		CPY_FLD(Flags);
		CPY_FLD(CurrencyID);
		CPY_FLD(Latitude);
		CPY_FLD(Longitude);
#undef CPY_FLD
		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Abbr);
		b.Add(rec.Phone);
		b.Add(rec.Code);
		b.Add(rec.ZIP);
		PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void SLAPI WorldCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	WorldTbl::Rec * p_data_rec = static_cast<WorldTbl::Rec *>(pDataRec);
	const WorldData * p_cache_rec = static_cast<const WorldData *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
#define CPY_FLD(Fld) p_data_rec->Fld=p_cache_rec->Fld
	CPY_FLD(ID);
	CPY_FLD(Kind);
	CPY_FLD(ParentID);
	CPY_FLD(CountryID);
	CPY_FLD(Status);
	CPY_FLD(Flags);
	CPY_FLD(CurrencyID);
	CPY_FLD(Latitude);
	CPY_FLD(Longitude);
#undef CPY_FLD
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name,  sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Abbr,  sizeof(p_data_rec->Abbr));
	b.Get(p_data_rec->Phone, sizeof(p_data_rec->Phone));
	b.Get(p_data_rec->Code,  sizeof(p_data_rec->Code));
	b.Get(p_data_rec->ZIP,   sizeof(p_data_rec->ZIP));
}

int SLAPI PPObjWorld::Fetch(PPID id, WorldTbl::Rec * pRec)
{
	WorldCache * p_cache = GetDbLocalCachePtr <WorldCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}
//
//
//
//static
SString & FiasObjCore::HouseCode::NormalizeItem(SString & rBuf)
{
	// @todo
	return rBuf;
}

SLAPI FiasObjCore::HouseCode::HouseCode(const char * pCode)
{
	if(pCode)
		Decode(pCode);
}

FiasObjCore::HouseCode & SLAPI FiasObjCore::HouseCode::Z()
{
	HseNum = 0;
	BldNum = 0;
	StrNum = 0;
	return *this;
}

SString & FASTCALL FiasObjCore::HouseCode::Encode(SString & rBuf) const
{
	SString temp_buf;
	rBuf.Z();
	rBuf.Cat((temp_buf = HseNum).Strip());
	rBuf.CatChar(':').Cat((temp_buf = BldNum).Strip());
	rBuf.CatChar(':').Cat((temp_buf = StrNum).Strip());
	return rBuf;
}

int FASTCALL FiasObjCore::HouseCode::Decode(const char * pCode)
{
	int    ok = 0;
	return ok;
}

SLAPI FiasObjCore::FiasObjCore()
{
}

int SLAPI FiasObjCore::SearchAddr(long recID, FiasAddrObjTbl::Rec * pRec)
{
    return SearchByID(&AdrT, PPOBJ_FIAS, recID, pRec);
}

int SLAPI FiasObjCore::SearchAddrByID(long addrID, FiasAddrObjTbl::Rec * pRec)
{
	int    ok = -1;
	FiasAddrObjTbl::Key1 k1;
	MEMSZERO(k1);
	k1.IdUuRef = addrID;
	if(AdrT.search(1, &k1, spGe) && AdrT.data.IdUuRef == addrID) {
		AdrT.copyBufTo(pRec);
		ok = 1;
	}
	else
		ok = PPDbSearchError();
	return ok;
}

int SLAPI FiasObjCore::SearchHouse(long hseID, FiasHouseObjTbl::Rec * pRec)
{
	return SearchByID(&HseT, PPOBJ_FIAS, hseID, pRec);
}

int SLAPI FiasObjCore::SearchAddrByUUID(const S_GUID & rUuid, FiasAddrObjTbl::Rec * pRec)
{
	long   ui = 0;
	int    ok = UrT.SearchUuid(rUuid, 0, &ui);
	if(ok > 0) {
		ok = SearchAddrByID(ui, pRec);
	}
	return ok;
}

int SLAPI FiasObjCore::SearchHouseByUUID(const S_GUID & rUuid, FiasHouseObjTbl::Rec * pRec)
{
	long   ui = 0;
	int    ok = UrT.SearchUuid(rUuid, 0, &ui);
	if(ok > 0) {
		ok = SearchHouse(ui, pRec);
	}
	return ok;
}

int SLAPI FiasObjCore::GetChildList(PPID parentID, int level, PPIDArray & rList)
{
    int    ok = -1;
    rList.clear();
    FiasAddrObjTbl::Rec rec;
    if(SearchAddrByID(parentID, &rec) > 0) {
		if(!level || rec.LevelStatus < level) {
			ok = Helper_GetChildList(parentID, level, rList);
		}
    }
    rList.sortAndUndup();
    if(ok && rList.getCount())
		ok = 1;
    return ok;

}

int SLAPI FiasObjCore::Helper_GetChildList(PPID parentID, int level, PPIDArray & rList)
{
    int    ok = 1;
	PPIDArray this_level_list;
	PPIDArray other_level_list;
	FiasAddrObjTbl::Key2 k2;
	k2.ParentUuRef = parentID;
	DBQ * dbq = &(AdrT.ParentUuRef == parentID);
	BExtQuery q(&AdrT, 2);
	q.selectAll().where(*dbq);
	for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
		const PPID _id = AdrT.data.IdUuRef;
		if(level) {
			const int _lvl = AdrT.data.LevelStatus;
			if(_lvl == level) {
				THROW_SL(this_level_list.add(_id));
			}
			else if(_lvl < level) {
				THROW_SL(other_level_list.add(_id));
			}
		}
		else {
			THROW_SL(rList.add(_id));
		}
	}
	if(level) {
		if(other_level_list.getCount()) {
			for(uint i = 0; i < other_level_list.getCount(); i++) {
				const PPID _parent_id = other_level_list.get(i);
				THROW(ok = Helper_GetChildList(_parent_id, level, rList)); // @recursion
			}
		}
		rList.add(&this_level_list);
	}
    CATCHZOK
    return ok;
}

int SLAPI FiasObjCore::GetHouseListByZIP(const char * pZip, PPIDArray & rList)
{
	int    ok = -1;
	long   zip_val = pZip ? atol(pZip) : 0;
	if(zip_val) {
		FiasHouseObjTbl::Key4 k4;
		MEMSZERO(k4);
		k4.PostalCode = zip_val;
		BExtQuery q(&HseT, 4);
		q.selectAll().where(HseT.PostalCode == zip_val);
		for(q.initIteration(0, &k4, spGe); q.nextIteration() > 0;) {
			THROW_SL(rList.add(HseT.data.IdUuRef));
			ok = 1;
		}
    }
    CATCHZOK
	return ok;
}

int SLAPI GetAddrListByZIP(const char * pZip, PPIDArray & rList)
{
	int    ok = -1;
	return ok;
}
//
//
// static
int SLAPI PPObjWorld::Convert()
{
	const char * p_spec_name = "special";

	int    ok = -1;
	BExtQuery * p_q = 0;
	int    is_need_conversion = 0;
	WorldTbl * p_tbl = 0;
	WorldTbl::Rec wrec;
	{
		WorldTbl::Key3 k3;
		int16  num_keys;
		THROW_MEM(p_tbl = new WorldTbl);
		p_tbl->getNumKeys(&num_keys);
		if(num_keys < 4) {
			SString file_name = p_tbl->GetFileName();
			ZDELETE(p_tbl);
			SFile::Remove(file_name);
			THROW_MEM(p_tbl = new WorldTbl);
		}
		MEMSZERO(k3);
		k3.Kind = WORLDOBJ_SPECIAL;
		STRNSCPY(k3.Name, p_spec_name);
		if(p_tbl->search(3, &k3, spEq))
			is_need_conversion = 0;
		else
			is_need_conversion = 1;
	}
	if(is_need_conversion) {
		PPID   id = 1, temp_id = 0;
		CityTbl::Key0 k0;
		LAssocArray cntry_list, reg_list;
		CityTbl    city_tbl;
		RegionTbl  reg_tbl;
		CountryTbl cntry_tbl;
		{
			PPTransaction tra(1);
			THROW(tra);
			k0.ID = MAXLONG;
			id = (city_tbl.search(0, &k0, spLt) > 0) ? (city_tbl.data.ID + 1) : 1;
			{
				CountryTbl::Key0 _k0;
				THROW_MEM(p_q = new BExtQuery(&cntry_tbl, 0));
				p_q->selectAll();
				MEMSZERO(_k0);
				for(p_q->initIteration(0, &_k0, spGe); p_q->nextIteration() > 0; id++) {
					CountryTbl::Rec cntry_rec = cntry_tbl.data;
					MEMSZERO(wrec);
					wrec.ID = id;
					wrec.Kind = WORLDOBJ_COUNTRY;
					STRNSCPY(wrec.Name,  cntry_rec.Name);
					STRNSCPY(wrec.Abbr,  cntry_rec.Abbr);
					STRNSCPY(wrec.Phone, cntry_rec.Phone);
					THROW(AddByID(p_tbl, &id, &wrec, 0));
					THROW_SL(cntry_list.AddUnique(cntry_rec.ID, id, 0));
				}
			}
			cntry_list.Sort();
			BExtQuery::ZDelete(&p_q);
			{
				RegionTbl::Key0 reg_k0;
				MEMSZERO(reg_k0);
				THROW_MEM(p_q = new BExtQuery(&reg_tbl, 0));
				p_q->selectAll();
				for(p_q->initIteration(0, &reg_k0, spGe); p_q->nextIteration() > 0; id++) {
					RegionTbl::Rec reg_rec = reg_tbl.data;
					MEMSZERO(wrec);
					wrec.ID   = id;
					wrec.Kind = WORLDOBJ_REGION;
					STRNSCPY(wrec.Name,  reg_rec.Name);
					STRNSCPY(wrec.Abbr,  reg_rec.Abbr);
					STRNSCPY(wrec.Phone, reg_rec.Phone);
					if(reg_rec.Country)
						cntry_list.BSearch(reg_rec.Country, &wrec.ParentID, 0);
					THROW(AddByID(p_tbl, &(temp_id = 0), &wrec, 0));
					THROW_SL(reg_list.AddUnique(reg_rec.ID, id, 0));
				}
			}
			reg_list.Sort();
			BExtQuery::ZDelete(&p_q);
			{
				CityTbl::Key0 city_k0;
				MEMSZERO(city_k0);
				THROW_MEM(p_q = new BExtQuery(&city_tbl, 0));
				p_q->selectAll();
				for(p_q->initIteration(0, &city_k0, spGe); p_q->nextIteration() > 0; id++) {
					CityTbl::Rec city_rec = city_tbl.data;
					MEMSZERO(wrec);
					wrec.ID   = city_rec.ID;
					wrec.Kind = WORLDOBJ_CITY;
					STRNSCPY(wrec.Name,  city_rec.Name);
					STRNSCPY(wrec.Abbr,  city_rec.Abbr);
					STRNSCPY(wrec.Phone, city_rec.Phone);
					wrec.Status = city_rec.Status;
					if(city_rec.Country)
						cntry_list.BSearch(city_rec.Country, &wrec.CountryID, 0);
					if(city_rec.Region)
						reg_list.BSearch(city_rec.Region, &wrec.ParentID, 0);
					THROW(AddByID(p_tbl, &(temp_id = 0), &wrec, 0));
				}
			}
			{
				//
				// Фиксируем факт конвертации
				//
				MEMSZERO(wrec);
				wrec.ID = id;
				wrec.Kind = WORLDOBJ_SPECIAL;
				STRNSCPY(wrec.Name, p_spec_name);
				STRNSCPY(wrec.Abbr, p_spec_name);
				THROW(AddByID(p_tbl, &(temp_id = 0), &wrec, 0));
			}
			THROW(tra.Commit());
		}
		ok = 1;
	}
	CATCHZOK
	BExtQuery::ZDelete(&p_q);
	delete p_tbl;
	return ok;
}
//
//
//
class GeoCityImportBlock {
public:
	GeoCityImportBlock()
	{
		StrPool.add("$"); // zero index - is empty string
	}
	int AddString(const char * pStr, uint * pPos)
	{
		if(!isempty(pStr))
			return StrPool.add(pStr, pPos);
		else {
			ASSIGN_PTR(pPos, 0);
			return -1;
		}
	}
	int GetString(uint pos, SString & rBuf) const
	{
		return StrPool.getnz(pos, rBuf);
	}
	int Import(const char * pPath);

	struct Country {
		uint32 Id;
		PPID   PapyrusID;
		uint   NameRuPos;
		uint   NameEnPos;
		char   Code2[4];
	};
	struct City {
		uint32 Id;
		PPID   PapyrusID;
		uint32 CountryId;
		uint   NameRuPos;
		uint   NameEnPos;
		char   Region[12];
		char   ZIP[12];
		double Latitude;
		double Longitude;
	};
	struct IpRange {
		uint32 ObjId;
		uint32 V4Low;
		uint32 V4Upp;
	};
	StringSet StrPool;
	TSArray <GeoCityImportBlock::Country> CountryList;
	TSArray <GeoCityImportBlock::City> CityList;

	EAddrCore EaT;
	PPObjWorld WObj;
};

int GeoCityImportBlock::Import(const char * pPath)
{
	int    ok = 1;
	SString file_name, line_buf, fld_buf, name_buf;
	StringSet ss("\t");
	{
		//
		// Импорт государств
		//
		(file_name = pPath).SetLastSlash().Cat("net_country.txt");
		SFile f_in(file_name, SFile::mRead);
		THROW_SL(f_in.IsValid());
		while(f_in.ReadLine(line_buf)) {
			GeoCityImportBlock::Country rec;
			line_buf.Chomp();
			ss.setBuf(line_buf, line_buf.Len()+1);
			uint ss_pos = 0;
			MEMSZERO(rec);
			for(uint ss_pos = 0, fld_no = 0; ss.get(&ss_pos, fld_buf); fld_no++) {
				switch(fld_no) {
					case 0:
						rec.Id = fld_buf.ToLong();
						break;
					case 1:
						AddString(fld_buf.Strip(), &rec.NameRuPos);
						break;
					case 2:
						AddString(fld_buf.Strip(), &rec.NameEnPos);
						break;
					case 3:
						fld_buf.CopyTo(rec.Code2, sizeof(rec.Code2));
						break;
				}
			}
			if(rec.Id && (rec.NameRuPos || rec.NameEnPos))
				THROW_SL(CountryList.insert(&rec));
		}
		CountryList.sort(CMPF_LONG);
	}
	{
		//
		// Импорт городов
		//
		(file_name = pPath).SetLastSlash().Cat("net_city.txt");
		SFile f_in(file_name, SFile::mRead);
		THROW_SL(f_in.IsValid());
		while(f_in.ReadLine(line_buf)) {
			GeoCityImportBlock::City rec;
			line_buf.Chomp();
			ss.setBuf(line_buf, line_buf.Len()+1);
			uint ss_pos = 0;
			MEMSZERO(rec);
			for(uint ss_pos = 0, fld_no = 0; ss.get(&ss_pos, fld_buf); fld_no++) {
				switch(fld_no) {
					case 0: rec.Id = fld_buf.ToLong(); break;
					case 1: rec.CountryId = fld_buf.ToLong(); break;
					case 2: AddString(fld_buf.Strip(), &rec.NameRuPos); break;
					case 3: AddString(fld_buf.Strip(), &rec.NameEnPos); break;
					case 4: fld_buf.CopyTo(rec.Region, sizeof(rec.Region)); break;
					case 5: fld_buf.CopyTo(rec.ZIP, sizeof(rec.ZIP)); break;
					case 6: rec.Latitude = fld_buf.ToReal(); break;
					case 7: rec.Longitude = fld_buf.ToReal(); break;
				}
			}
			if(rec.Id && rec.CountryId && (rec.NameRuPos || rec.NameEnPos))
				THROW_SL(CityList.insert(&rec));
		}
		CityList.sort(CMPF_LONG);
	}
	{
		uint i;
		PPTransaction tra(1);
		THROW(tra);
		for(i = 0; i < CountryList.getCount(); i++) {
			GeoCityImportBlock::Country & r_rec = CountryList.at(i);
			name_buf.Z();
			if(GetString(r_rec.NameRuPos, name_buf) > 0) {
				;
			}
			else if(GetString(r_rec.NameEnPos, name_buf) > 0) {
				;
			}
			if(name_buf.NotEmpty()) {
				name_buf.Transf(CTRANSF_UTF8_TO_INNER);
				PPWorldPacket w_pack;
				PPID   id = 0;
				if(WObj.SearchCountry(name_buf, 0, r_rec.Code2, &w_pack.Rec) > 0) {
					r_rec.PapyrusID = w_pack.Rec.ID;
					if(w_pack.Rec.Abbr[0] == 0 && r_rec.Code2[0] != 0) {
						STRNSCPY(w_pack.Rec.Abbr, r_rec.Code2);
						id = w_pack.Rec.ID;
						THROW(WObj.PutPacket(&id, &w_pack, 0));
					}
				}
				else {
					w_pack.Init();
					w_pack.Rec.Kind = WORLDOBJ_COUNTRY;
					name_buf.CopyTo(w_pack.Rec.Name, sizeof(w_pack.Rec.Name));
					STRNSCPY(w_pack.Rec.Abbr, r_rec.Code2);
					THROW(WObj.PutPacket(&id, &w_pack, 0));
					r_rec.PapyrusID = id;
				}
			}
		}
		for(i = 0; i < CityList.getCount(); i++) {
			GeoCityImportBlock::City & r_rec = CityList.at(i);
			uint   country_pos = 0;
			name_buf.Z();
			if(GetString(r_rec.NameRuPos, name_buf) > 0) {
				;
			}
			else if(GetString(r_rec.NameEnPos, name_buf) > 0) {
				;
			}
			if(name_buf.NotEmpty() && r_rec.CountryId && CountryList.bsearch(&r_rec.CountryId, &country_pos, CMPF_LONG) && CountryList.at(country_pos).PapyrusID) {
				name_buf.Transf(CTRANSF_UTF8_TO_INNER);
				PPWorldPacket w_pack;
				PPID   id = 0;
				w_pack.Init();
				w_pack.Rec.Kind = WORLDOBJ_CITY;
				name_buf.CopyTo(w_pack.Rec.Name, sizeof(w_pack.Rec.Name));
				w_pack.Rec.CountryID = CountryList.at(country_pos).PapyrusID;
				w_pack.Rec.Latitude = r_rec.Latitude;
				w_pack.Rec.Longitude = r_rec.Longitude;
				if(WObj.SearchMaxLike(&w_pack, PPObjWorld::smlName|PPObjWorld::smlCheckCountry, &id) > 0) {
					WorldTbl::Rec w_rec;
					int do_update = 0;
					THROW(WObj.Search(id, &w_rec) > 0);
					r_rec.PapyrusID = id;
					if(!w_rec.Latitude && r_rec.Latitude)
						do_update = 1;
					else if(!w_rec.Longitude && r_rec.Longitude)
						do_update = 1;
					if(do_update) {
						w_pack.Rec.ID = id;
						THROW(WObj.PutPacket(&id, &w_pack, 0));
					}
				}
				else {
					THROW(WObj.PutPacket(&id, &w_pack, 0));
					r_rec.PapyrusID = id;
				}
			}
		}
		{
			(file_name = pPath).SetLastSlash().Cat("net_city_ip.txt");
			SFile f_in(file_name, SFile::mRead);
			THROW_SL(f_in.IsValid());
			while(f_in.ReadLine(line_buf)) {
				GeoCityImportBlock::IpRange rec;
				line_buf.Chomp();
				ss.setBuf(line_buf, line_buf.Len()+1);
				uint ss_pos = 0;
				MEMSZERO(rec);
				for(uint ss_pos = 0, fld_no = 0; ss.get(&ss_pos, fld_buf); fld_no++) {
					struct IpRange {
						uint32 ObjId;
						uint32 V4Low;
						uint32 V4Upp;
					};
					switch(fld_no) {
						case 0:
							rec.ObjId = fld_buf.ToLong();
							break;
						case 1:
							rec.V4Low = fld_buf.ToULong();
							break;
						case 2:
							rec.V4Upp = fld_buf.ToULong();
							break;
					}
				}
				uint city_pos = 0;
				if(rec.ObjId && CityList.bsearch(&rec.ObjId, &city_pos, CMPF_LONG) && rec.V4Low && rec.V4Upp && rec.V4Low <= rec.V4Upp) {
					GeoCityImportBlock::City & r_city_rec = CityList.at(city_pos);
					if(r_city_rec.PapyrusID) {
						PPEAddr eadr;
						if(rec.V4Low == rec.V4Upp)
							eadr.Set(rec.V4Low);
						else
							eadr.Set(rec.V4Low, rec.V4Upp);
						PPID   eat_id = 0;
						PPObjID objid;
						objid.Set(PPOBJ_WORLD, r_city_rec.PapyrusID);
						THROW(EaT.Put(&eat_id, &eadr, &objid, 0));
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI ImportGeoCity(const char * pPath)
{
	int    ok = 1;
	GeoCityImportBlock blk;
	if(!blk.Import(pPath))
		ok = PPErrorZ();
	return ok;
}
//
//
//
#if SLTEST_RUNNING // {

SLTEST_R(ObjWorld)
{
	int    ok = 1, r;
	SString temp_buf;
	PPObjWorld w_obj;
	{
		PPID   city_id1 = 0;
		PPID   city_id2 = 0;
		const  char * p_city_name1 = "Акрополь Великий";
		const  char * p_city_name2 = "АКРОПОЛЬ ВЕЛИКИЙ";
		PPTransaction tra(1);
		THROW(SLTEST_CHECK_NZ(tra));
		//
		// Проверка функции AddSimple.
		//
		THROW(SLTEST_CHECK_NZ(r = w_obj.AddSimple(&city_id1, WORLDOBJ_CITY, (temp_buf = p_city_name1).Transf(CTRANSF_UTF8_TO_INNER), 0, 0)));
		THROW(SLTEST_CHECK_NZ(r = w_obj.AddSimple(&city_id2, WORLDOBJ_CITY, (temp_buf = p_city_name2).Transf(CTRANSF_UTF8_TO_INNER), 0, 0)));
		THROW(SLTEST_CHECK_EQ(city_id1, city_id2));
		THROW(SLTEST_CHECK_NZ(w_obj.PutPacket(&city_id1, 0, 0)));
		THROW(SLTEST_CHECK_NZ(tra.Commit()));
	}
	CATCH
		ok = 0;
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
