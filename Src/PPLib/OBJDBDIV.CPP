// OBJDBDIV.CPP
// Copyright (c) A.Sobolev 1997-2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2010, 2011, 2013, 2014, 2015, 2016, 2017
// @codepage windows-1251
// Объект, управляющий разделами БД
//
#include <pp.h>
#pragma hdrstop

static StrAssocArray * SLAPI MakeDBDivList(int skipCurrent)
{
	int    r;
	PPID   id;
	PPDBDiv div;
	PPObjDBDiv div_obj;
	StrAssocArray * p_list = new StrAssocArray;
	THROW_MEM(p_list);
	for(id = 0; (r = div_obj.EnumItems(&id, &div)) > 0;)
		if(!skipCurrent || div.ID != LConfig.DBDiv)
			THROW_SL(p_list->Add(id, div.Name));
	THROW(r);
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

// static
PPID SLAPI PPObjDBDiv::Select(int skipCurrent, int * pSingle)
{
	PPID   id = 0;
	ListWindow * p_lw = 0;
	StrAssocArray * p_ary = 0;
	THROW(p_ary = MakeDBDivList(skipCurrent));
	if(p_ary->getCount() == 1) {
		id = p_ary->at(0).Id;
		ASSIGN_PTR(pSingle, 1)
		ZDELETE(p_ary);
	}
	else {
		SString sub_title;
		ASSIGN_PTR(pSingle, 0);
		THROW(p_lw = CreateListWindow(p_ary, lbtDisposeData|lbtDblClkNotify));
		THROW(PPLoadText(PPTXT_SELECTDBDIV, sub_title));
		p_lw->setTitle(sub_title);
		p_lw->options |= (ofCenterX | ofCenterY);
		if(ExecView(p_lw) == cmOK)
			p_lw->getResult(&id);
		else
			id = -1;
	}
	CATCH
		id = 0;
		PPError();
	ENDCATCH
	delete p_lw;
	return id;
}
//
//
//
SLAPI DBDivPack::DBDivPack()
{
	MEMSZERO(Rec);
}

DBDivPack & FASTCALL DBDivPack::operator = (const DBDivPack & rS)
{
	Rec = rS.Rec;
	LocList = rS.LocList;
	ExtString = rS.ExtString;
	return *this;
}

void SLAPI DBDivPack::Init()
{
	MEMSZERO(Rec);
	LocList.freeAll();
	ExtString = 0;
}

int SLAPI DBDivPack::PutExtStrData(int fldID, const char * pBuf)
{
	return PPPutExtStrData(fldID, ExtString, pBuf);
}

int SLAPI DBDivPack::GetExtStrData(int fldID, SString & rBuf) const
{
	return PPGetExtStrData(fldID, DBDIVEXSTR_ACCLIST, ExtString, rBuf);
}

int SLAPI DBDivPack::ResponsibleForLoc(PPID locID, int forQuot) const
{
	int    yes = 0;
	if(forQuot)
		yes = PPQuot::IsQuotAcceptableForLoc(locID, LocList);
	else if(locID && (!LocList.getCount() || LocList.lsearch(locID)))
		yes = 1;
	return yes;
}

SLAPI PPObjDBDiv::PPObjDBDiv(void * extraPtr) : PPObjReference(PPOBJ_DBDIV, extraPtr)
{
}

class DBDivAddrListDialog : public PPListDialog {
public:
	DBDivAddrListDialog() : PPListDialog(DLG_DBDIVADDRLIST, CTL_DBDIVADDRLIST_LIST)
	{
	}
	int    setDTS(const SString * pData)
	{
		Data = *pData;
		updateList(-1);
		return 1;
	}
	int    getDTS(SString * pData)
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	virtual int setupList()
	{
		int    ok = 1;
		StringSet ss(';', Data);
		SString temp_buf;
		uint   i = 0;
		for(uint p = 0; ss.get(&p, temp_buf);) {
			if(temp_buf.NotEmptyS()) {
				THROW(addStringToList(++i, temp_buf));
			}
		}
		CATCHZOK
		return ok;
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		SString addr_buf;
		PPInputStringDialogParam isd_param;
		PPLoadString("addrdbdiv", isd_param.Title);
		isd_param.InputTitle = isd_param.Title;
		if(InputStringDialog(&isd_param, addr_buf) > 0) {
			if(addr_buf.NotEmptyS()) {
				SStrCollection sc;
				ToCollection(sc);
				sc.insert(newStr(addr_buf));
				FromCollection(sc);
				ok = 1;
			}
		}
		return ok;
	}
	virtual int editItem(long pos, long id)
	{
		int    ok = -1;
		SString addr_buf;
		SStrCollection sc;
		ToCollection(sc);
		if(id > 0 && id <= (long)sc.getCount()) {
			addr_buf = sc.at(id-1);
			PPInputStringDialogParam isd_param;
			PPLoadString("addrdbdiv", isd_param.Title);
			isd_param.InputTitle = isd_param.Title;
			if(InputStringDialog(&isd_param, addr_buf) > 0) {
				sc.atFree(id-1);
				if(addr_buf.NotEmptyS()) {
					sc.atInsert(id-1, addr_buf);
				}
				FromCollection(sc);
				ok = 1;
			}
		}
		return ok;
	}
	virtual int delItem(long pos, long id)
	{
		int    ok = -1;
		SStrCollection sc;
		ToCollection(sc);
		if(id > 0 && id <= (long)sc.getCount()) {
			sc.atFree(id-1);
			FromCollection(sc);
			ok = 1;
		}
		return ok;
	}
	virtual int moveItem(long pos, long id, int up)
	{
		int    ok = -1;
		SStrCollection sc;
		ToCollection(sc);
		if(up && pos > 0) {
			sc.swap(pos, pos-1);
			ok = 1;
		}
		else if(!up && pos < (long)(sc.getCount()-1)) {
			sc.swap(pos, pos+1);
			ok = 1;
		}
		if(ok > 0) {
			FromCollection(sc);
		}
		return ok;
	}
	int    ToCollection(SStrCollection & rC) const
	{
		rC.freeAll();
		StringSet ss(';', Data);
		SString temp_buf;
		for(uint p = 0; ss.get(&p, temp_buf);) {
			if(temp_buf.NotEmptyS())
				rC.insert(newStr(temp_buf));
		}
		return 1;
	}
	int    FromCollection(const SStrCollection & rC)
	{
		StringSet ss(';', 0);
		for(uint i = 0; i < rC.getCount(); i++) {
			const char * p_item = rC.at(i);
			if(!isempty(p_item)) {
				ss.add(p_item);
			}
		}
		Data = ss.getBuf();
		return 1;
	}

	SString Data;
};

static int EditDBDivAddrList(SString * pData)
{
	DIALOG_PROC_BODY(DBDivAddrListDialog, pData);
}

class DBDivDialog : public TDialog {
public:
	DBDivDialog() : TDialog(DLG_DBDIV)
	{
		if(!SetupStrListBox(this, CTL_DBDIV_LIST))
			PPError();
	}
	int  setDTS(const DBDivPack *);
	int  getDTS(DBDivPack *);
private:
	DECL_HANDLE_EVENT;
	void   updateList();
	void   addItem();
	void   delItem();
	void   SetupAddr(SString & rAddrList);
	DBDivPack data;
};

void DBDivDialog::addItem()
{
	PPID   id = 0;
	int    r = PPSelectObject(PPOBJ_LOCATION, &id, PPTXT_SELECTLOCATION, 0);
	if(r > 0 && !data.LocList.lsearch(id)) {
		data.LocList.add(id);
		updateList();
	}
}

inline void DBDivDialog::delItem()
{
	uint   p = 0;
	PPID   id;
	SmartListBox * p_list = (SmartListBox*)getCtrlView(CTL_DBDIV_LIST);
	if(p_list && p_list->getCurID(&id) && data.LocList.lsearch(id, &p)) {
		data.LocList.atFree(p);
		updateList();
	}
}

void DBDivDialog::updateList()
{
	SmartListBox * p_list = (SmartListBox*)getCtrlView(CTL_DBDIV_LIST);
	if(p_list) {
		p_list->freeAll();
		SString text;
		for(uint i = 0; i < data.LocList.getCount(); i++) {
			PPID   id = data.LocList.at(i);
			GetLocationName(id, text);
			if(!p_list->addItem(id, text)) {
				PPError(PPERR_SLIB, 0);
				break;
			}
		}
		p_list->drawView();
	}
}

void DBDivDialog::SetupAddr(SString & rAddrList)
{
	SString temp_buf;
	StringSet ss(';', rAddrList);
	const uint c = ss.getCount();
	if(c > 1 || (c == 1 && rAddrList.Len() >= sizeof(data.Rec.Addr))) {
		PPLoadString("list", temp_buf);
		setCtrlString(CTL_DBDIV_ADDR, temp_buf);
		disableCtrl(CTL_DBDIV_ADDR, 1);
		data.PutExtStrData(DBDIVEXSTR_GATEWAYLIST, rAddrList);
	}
	else {
		setCtrlString(CTL_DBDIV_ADDR, temp_buf = rAddrList);
		disableCtrl(CTL_DBDIV_ADDR, 0);
		data.PutExtStrData(DBDIVEXSTR_GATEWAYLIST, temp_buf = 0);
	}
}

IMPL_HANDLE_EVENT(DBDivDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmaInsert))
		addItem();
	else if(event.isCmd(cmaDelete))
		delItem();
	else if(event.isCmd(cmAddrList)) {
		SString addr_list, temp_buf;
		data.GetExtStrData(DBDIVEXSTR_GATEWAYLIST, addr_list);
		if(!addr_list.NotEmptyS()) {
			getCtrlString(CTL_DBDIV_ADDR, addr_list);
		}
		if(EditDBDivAddrList(&addr_list) > 0) {
			SetupAddr(addr_list);
		}
	}
	else
		return;
	clearEvent(event);
}

int DBDivDialog::setDTS(const DBDivPack * p)
{
	data = *p;
	SString temp_buf;
	updateList();
	setCtrlData(CTL_DBDIV_ID,   &data.Rec.ID);
	disableCtrl(CTL_DBDIV_ID,   (int)data.Rec.ID);
	// @v8.0.11 {
	data.Rec.Uuid.ToStr(S_GUID::fmtIDL, temp_buf);
	setCtrlString(CTL_DBDIV_UUID, temp_buf.Strip());
	// } @v8.0.11
	setCtrlData(CTL_DBDIV_CNT,  &data.Rec.OutCounter);
	setCtrlData(CTL_DBDIV_NAME, data.Rec.Name);
	{
		data.GetExtStrData(DBDIVEXSTR_GATEWAYLIST, temp_buf); // @v8.0.11
		if(temp_buf.Empty())
			temp_buf = data.Rec.Addr;
		SetupAddr(temp_buf); // @v8.0.11
		// @v8.0.11 setCtrlData(CTL_DBDIV_ADDR, data.Rec.Addr);
	}
	data.GetExtStrData(DBDIVEXSTR_ACCLIST, temp_buf);
	setCtrlString(CTL_DBDIV_ACCLIST, temp_buf);
	SetupPPObjCombo(this, CTLSEL_DBDIV_INTRRCPT, PPOBJ_OPRKIND, data.Rec.IntrRcptOpr, OLW_CANINSERT, (void *)PPOPT_GOODSRECEIPT);
	AddClusterAssoc(CTL_DBDIV_FLAGS, 0, DBDIVF_DISPATCH);
	AddClusterAssoc(CTL_DBDIV_FLAGS, 1, DBDIVF_SCARDSONLY);
	AddClusterAssoc(CTL_DBDIV_FLAGS, 2, DBDIVF_RCVCSESSANDWROFFBILLS);
	AddClusterAssoc(CTL_DBDIV_FLAGS, 3, DBDIVF_CONSOLID);
	AddClusterAssoc(CTL_DBDIV_FLAGS, 4, DBDIVF_PASSIVE);
	SetClusterData(CTL_DBDIV_FLAGS, data.Rec.Flags);
	return 1;
}

int DBDivDialog::getDTS(DBDivPack * p)
{
	int    ok = 1;
	uint   sel = 0;
	SString temp_buf;
	getCtrlData(sel = CTL_DBDIV_ID, &data.Rec.ID);
	THROW_PP(data.Rec.ID > 0 && data.Rec.ID < 256, PPERR_INVDBDIVID);
	getCtrlData(CTL_DBDIV_CNT, &data.Rec.OutCounter);
	getCtrlData(sel = CTL_DBDIV_NAME, data.Rec.Name);
	THROW_PP(*strip(data.Rec.Name), PPERR_NAMENEEDED);
	//
	{
		data.GetExtStrData(DBDIVEXSTR_GATEWAYLIST, temp_buf);
		StringSet ss(';', temp_buf);
		const uint c = ss.getCount();
		if(c > 1 || (c == 1 && temp_buf.Len() >= sizeof(data.Rec.Addr))) {
			data.Rec.Addr[0] = 0;
		}
		else {
			getCtrlData(CTL_DBDIV_ADDR, data.Rec.Addr);
		}
	}
	//
	getCtrlData(sel = CTLSEL_DBDIV_INTRRCPT, &data.Rec.IntrRcptOpr);
	THROW_PP(IsIntrOp(data.Rec.IntrRcptOpr) == INTRRCPT, PPERR_NOTINTRRCPTOP);
	getCtrlString(CTL_DBDIV_ACCLIST, temp_buf);
	data.PutExtStrData(DBDIVEXSTR_ACCLIST, temp_buf);
	GetClusterData(CTL_DBDIV_FLAGS, &data.Rec.Flags);
	ASSIGN_PTR(p, data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

int SLAPI PPObjDBDiv::Get(PPID id, DBDivPack * pack)
{
	int    ok = 1;
	THROW(ref->GetItem(Obj, id, &pack->Rec) > 0);
	THROW(ref->GetPropArray(Obj, id, DBDPRP_LOCLIST, &pack->LocList));
	THROW(ref->GetPropVlrString(Obj, id, DBDPRP_EXTSTRING, pack->ExtString));
	pack->ExtString.Strip();
	CATCH
		ok = 0;
		pack->Init();
	ENDCATCH
	return ok;
}

StrAssocArray * SLAPI PPObjDBDiv::MakeStrAssocList(void * extraPtr)
{
	const long extra_param = (long)extraPtr;
	PPDBDiv rec;
	StrAssocArray * p_list = new StrAssocArray();
	THROW_MEM(p_list);
	for(SEnum en = ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
		if((extra_param & mklefShowPassive) || !(rec.Flags & DBDIVF_PASSIVE)) {
			if(*strip(rec.Name) == 0)
				ideqvalstr(rec.ID, rec.Name, sizeof(rec.Name));
			THROW_SL(p_list->Add(rec.ID, rec.Name));
		}
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int SLAPI PPObjDBDiv::Put(PPID * pID, DBDivPack * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
		}
		else {
			THROW(ref->AddItem(Obj, &pPack->Rec.ID, &pPack->Rec, 0));
			*pID = pPack->Rec.ID;
		}
		THROW(ref->PutPropArray(Obj, *pID, DBDPRP_LOCLIST, &pPack->LocList, 0));
		THROW(ref->PutPropVlrString(Obj, *pID, DBDPRP_EXTSTRING, pPack->ExtString));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjDBDiv::AcceptUuid(PPID dbid, const S_GUID & rUuid, int use_ta)
{
	int    ok = -1;
	PPDBDiv rec;
	THROW(ref->GetItem(PPOBJ_DBDIV, dbid, &rec) > 0);
	if(rec.Uuid.IsZero() && !rUuid.IsZero()) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ref->GetItem(PPOBJ_DBDIV, dbid, &rec) > 0);
		rec.Uuid = rUuid;
		THROW(ref->UpdateItem(PPOBJ_DBDIV, dbid, &rec, 1, 0));
		THROW(tra.Commit());
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjDBDiv::GetUuid(PPID dbid, S_GUID * pUuid, int use_ta)
{
	int    ok = 1;
	PPDBDiv rec;
	dbid = NZOR(dbid, LConfig.DBDiv);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ref->GetItem(PPOBJ_DBDIV, dbid, &rec) > 0);
		if(rec.Uuid.IsZero()) {
			rec.Uuid.Generate();
			THROW(ref->UpdateItem(PPOBJ_DBDIV, dbid, &rec, 1, 0));
		}
		THROW(tra.Commit());
		ASSIGN_PTR(pUuid, rec.Uuid);
		ok = rec.Uuid.IsZero() ? 0 : 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjDBDiv::GetCounter(PPID dbid, long * pCounter, int use_ta)
{
	int    ok = 1;
	PPDBDiv rec;
	dbid = NZOR(dbid, LConfig.DBDiv);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ref->GetItem(PPOBJ_DBDIV, dbid, &rec) > 0);
		*pCounter = ++rec.OutCounter;
		if(rec.OutCounter >= 1000000L) // @v7.4.7 10000L-->1000000L
			rec.OutCounter = 0;
		THROW(ref->UpdateItem(PPOBJ_DBDIV, dbid, &rec, 1, 0));
		THROW(tra.Commit());
	}
	CATCH
		*pCounter = 0;
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPObjDBDiv::Edit(PPID * pID, void * extraPtr)
{
	int    r = cmCancel, valid_data = 0, is_new = 0;
	DBDivPack pack;
	DBDivDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new DBDivDialog)));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new)
		THROW(Get(*pID, &pack) > 0);
	dlg->setDTS(&pack);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		if(dlg->getDTS(&pack)) {
			valid_data = 1;
			THROW(Put(pID, &pack, 1));
		}
	}
	CATCH
		r = PPErrorZ();
	ENDCATCH
	delete dlg;
	return r;
}

//virtual
int  SLAPI PPObjDBDiv::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options, void * pExtraParam)
{
	int    r = -1;
	THROW(CheckRights(PPR_DEL));
	if(id != LConfig.DBDiv && (!(options & PPObject::user_request) || PPMessage(mfConf|mfYesNo, PPCFM_DELDBDIV, 0) == cmYes)) {
		int    use_ta = BIN(options & PPObject::use_transaction);
		ObjSyncTbl sync_tbl;
		PPWait(1);
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(deleteFrom(&sync_tbl, 0, (sync_tbl.DBID == id)));
		THROW(ref->RemoveItem(Obj, id, 0) && RemoveSync(id));
		THROW(tra.Commit());
		r = 1;
	}
	CATCH
		r = 0;
		if(options & PPObject::user_request)
			PPError();
	ENDCATCH
	PPWait(0);
	return r;
}

int SLAPI PPObjDBDiv::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	if(msg == DBMSG_OBJDELETE) {
		PPDBDiv rec;
		if(_obj == PPOBJ_OPRKIND) {
			for(SEnum en = ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
				if(rec.IntrRcptOpr == _id)
					return RetRefsExistsErr(Obj, rec.ID);
			}
		}
		// @v7.0.9 {
		else if(_obj == PPOBJ_LOCATION) {
			for(SEnum en = ref->Enum(Obj, Reference::eoIdName); en.Next(&rec) > 0;) {
				DBDivPack pack;
				if(Get(rec.ID, &pack) > 0 && pack.LocList.lsearch(_id))
					return RetRefsExistsErr(Obj, rec.ID);
			}
		}
		// } @v7.0.9
	}
	return DBRPL_OK;
}
//
// Implementation of PPALDD_DBDiv
//
PPALDD_CONSTRUCTOR(DBDiv)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(DBDiv)
{
	Destroy();
}

int PPALDD_DBDiv::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		H.ID = rFilt.ID;
		PPDBDiv rec;
		if(SearchObject(PPOBJ_DBDIV, rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			STRNSCPY(H.Name, rec.Name);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
