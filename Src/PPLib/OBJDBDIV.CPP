// OBJDBDIV.CPP
// Copyright (c) A.Sobolev 1997-2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2024, 2025
// @codepage UTF-8
// Объект, управляющий разделами БД
//
#include <pp.h>
#pragma hdrstop

static StrAssocArray * MakeDBDivList(int skipCurrent)
{
	int    r;
	PPID   id;
	PPDBDiv div;
	PPObjDBDiv div_obj;
	StrAssocArray * p_list = new StrAssocArray;
	THROW_MEM(p_list);
	for(id = 0; (r = div_obj.EnumItems(&id, &div)) > 0;) {
		if(!skipCurrent || div.ID != LConfig.DBDiv)
			THROW_SL(p_list->Add(id, div.Name));
	}
	THROW(r);
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

/*static*/PPID PPObjDBDiv::Select(int skipCurrent, int * pSingle)
{
	PPID   id = 0;
	ListWindow * p_lw = 0;
	StrAssocArray * p_ary = 0;
	THROW(p_ary = MakeDBDivList(skipCurrent));
	if(p_ary->getCount() == 1) {
		id = p_ary->Get(0).Id;
		ASSIGN_PTR(pSingle, 1)
		ZDELETE(p_ary);
	}
	else {
		SString sub_title;
		ASSIGN_PTR(pSingle, 0);
		THROW(p_lw = CreateListWindow(p_ary, lbtDisposeData|lbtDblClkNotify));
		THROW(PPLoadText(PPTXT_SELECTDBDIV, sub_title));
		p_lw->setTitle(sub_title);
		p_lw->ViewOptions |= (ofCenterX | ofCenterY);
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
DBDivPack::DBDivPack()
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

void DBDivPack::Init()
{
	MEMSZERO(Rec);
	LocList.freeAll();
	ExtString.Z();
}

int DBDivPack::PutExtStrData(int fldID, const char * pBuf) { return PPPutExtStrData(fldID, ExtString, pBuf); }
int DBDivPack::GetExtStrData(int fldID, SString & rBuf) const { return PPGetExtStrData_def(fldID, DBDIVEXSTR_ACCLIST, ExtString, rBuf); }

int DBDivPack::ResponsibleForLoc(PPID locID, int forQuot) const
{
	int    yes = 0;
	if(forQuot)
		yes = PPQuot::IsQuotAcceptableForLoc(locID, LocList);
	else if(locID && (!LocList.getCount() || LocList.lsearch(locID)))
		yes = 1;
	return yes;
}

PPObjDBDiv::PPObjDBDiv(void * extraPtr) : PPObjReference(PPOBJ_DBDIV, extraPtr)
{
}

class DBDivAddrListDialog : public PPListDialog {
	DECL_DIALOG_DATA(SString);
public:
	DBDivAddrListDialog() : PPListDialog(DLG_DBDIVADDRLIST, CTL_DBDIVADDRLIST_LIST)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
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
		if(InputStringDialog(isd_param, addr_buf) > 0) {
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
		if(id > 0 && id <= sc.getCountI()) {
			addr_buf = sc.at(id-1);
			PPInputStringDialogParam isd_param;
			PPLoadString("addrdbdiv", isd_param.Title);
			isd_param.InputTitle = isd_param.Title;
			if(InputStringDialog(isd_param, addr_buf) > 0) {
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
		if(id > 0 && id <= sc.getCountI()) {
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
		else if(!up && pos < (sc.getCountI()-1)) {
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
};

static int EditDBDivAddrList(SString * pData) { DIALOG_PROC_BODY(DBDivAddrListDialog, pData); }

class DBDivDialog : public TDialog {
	DECL_DIALOG_DATA(DBDivPack);
public:
	DBDivDialog() : TDialog(DLG_DBDIV)
	{
		if(!SetupStrListBox(this, CTL_DBDIV_LIST))
			PPError();
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString temp_buf;
		updateList();
		setCtrlData(CTL_DBDIV_ID,   &Data.Rec.ID);
		disableCtrl(CTL_DBDIV_ID,   LOGIC(Data.Rec.ID));
		Data.Rec.Uuid.ToStr(S_GUID::fmtIDL, temp_buf);
		setCtrlString(CTL_DBDIV_UUID, temp_buf.Strip());
		setCtrlData(CTL_DBDIV_CNT,  &Data.Rec.OutCounter);
		setCtrlData(CTL_DBDIV_NAME, Data.Rec.Name);
		{
			Data.GetExtStrData(DBDIVEXSTR_GATEWAYLIST, temp_buf);
			temp_buf.SetIfEmpty(Data.Rec.Addr);
			SetupAddr(temp_buf);
		}
		Data.GetExtStrData(DBDIVEXSTR_ACCLIST, temp_buf);
		setCtrlString(CTL_DBDIV_ACCLIST, temp_buf);
		SetupPPObjCombo(this, CTLSEL_DBDIV_INTRRCPT, PPOBJ_OPRKIND, Data.Rec.IntrRcptOpr, OLW_CANINSERT, reinterpret_cast<void *>(PPOPT_GOODSRECEIPT));
		AddClusterAssoc(CTL_DBDIV_MQBEXCHG, 0, DBDIVF_MQBEXCHANGE);
		SetClusterData(CTL_DBDIV_MQBEXCHG, Data.Rec.Flags); 
		AddClusterAssoc(CTL_DBDIV_FLAGS, 0, DBDIVF_DISPATCH);
		AddClusterAssoc(CTL_DBDIV_FLAGS, 1, DBDIVF_SCARDSONLY);
		AddClusterAssoc(CTL_DBDIV_FLAGS, 2, DBDIVF_RCVCSESSANDWROFFBILLS);
		AddClusterAssoc(CTL_DBDIV_FLAGS, 3, DBDIVF_CONSOLID);
		AddClusterAssoc(CTL_DBDIV_FLAGS, 4, DBDIVF_PASSIVE);
		SetClusterData(CTL_DBDIV_FLAGS, Data.Rec.Flags);
		{
			disableCtrl(CTL_DBDIV_ADDR, LOGIC(Data.Rec.Flags & DBDIVF_MQBEXCHANGE));
			enableCommand(cmAddrList, !(Data.Rec.Flags & DBDIVF_MQBEXCHANGE));
		}
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		SString temp_buf;
		getCtrlData(sel = CTL_DBDIV_ID, &Data.Rec.ID);
		THROW_PP(Data.Rec.ID > 0 && Data.Rec.ID < 256, PPERR_INVDBDIVID);
		getCtrlData(CTL_DBDIV_CNT, &Data.Rec.OutCounter);
		getCtrlData(sel = CTL_DBDIV_NAME, Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
		{
			Data.GetExtStrData(DBDIVEXSTR_GATEWAYLIST, temp_buf);
			StringSet ss(';', temp_buf);
			const uint c = ss.getCount();
			if(c > 1 || (c == 1 && temp_buf.Len() >= sizeof(Data.Rec.Addr)))
				Data.Rec.Addr[0] = 0;
			else
				getCtrlData(CTL_DBDIV_ADDR, Data.Rec.Addr);
		}
		getCtrlData(sel = CTLSEL_DBDIV_INTRRCPT, &Data.Rec.IntrRcptOpr);
		THROW_PP(IsIntrOp(Data.Rec.IntrRcptOpr) == INTRRCPT, PPERR_NOTINTRRCPTOP);
		getCtrlString(CTL_DBDIV_ACCLIST, temp_buf);
		Data.PutExtStrData(DBDIVEXSTR_ACCLIST, temp_buf);
		GetClusterData(CTL_DBDIV_MQBEXCHG, &Data.Rec.Flags);
		GetClusterData(CTL_DBDIV_FLAGS, &Data.Rec.Flags);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   updateList();
	void   addItem();
	void   delItem();
	void   SetupAddr(const SString & rAddrList);
};

void DBDivDialog::addItem()
{
	PPID   id = 0;
	const  int r = PPSelectObject(PPOBJ_LOCATION, &id, PPTXT_SELECTLOCATION, 0);
	if(r > 0 && !Data.LocList.lsearch(id)) {
		Data.LocList.add(id);
		updateList();
	}
}

inline void DBDivDialog::delItem()
{
	uint   p = 0;
	PPID   id;
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_DBDIV_LIST));
	if(p_list && p_list->getCurID(&id) && Data.LocList.lsearch(id, &p)) {
		Data.LocList.atFree(p);
		updateList();
	}
}

void DBDivDialog::updateList()
{
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_DBDIV_LIST));
	if(p_list) {
		p_list->freeAll();
		SString text;
		for(uint i = 0; i < Data.LocList.getCount(); i++) {
			const  PPID id = Data.LocList.at(i);
			GetLocationName(id, text);
			if(!p_list->addItem(id, text)) {
				PPError(PPERR_SLIB, 0);
				break;
			}
		}
		p_list->Draw_();
	}
}

void DBDivDialog::SetupAddr(const SString & rAddrList)
{
	SString temp_buf;
	StringSet ss(';', rAddrList);
	const uint c = ss.getCount();
	if(c > 1 || (c == 1 && rAddrList.Len() >= sizeof(Data.Rec.Addr))) {
		PPLoadString("list", temp_buf);
		setCtrlString(CTL_DBDIV_ADDR, temp_buf);
		disableCtrl(CTL_DBDIV_ADDR, true);
		Data.PutExtStrData(DBDIVEXSTR_GATEWAYLIST, rAddrList);
	}
	else {
		setCtrlString(CTL_DBDIV_ADDR, temp_buf = rAddrList);
		disableCtrl(CTL_DBDIV_ADDR, false);
		Data.PutExtStrData(DBDIVEXSTR_GATEWAYLIST, temp_buf.Z());
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
		SString temp_buf;
		SString addr_list;
		Data.GetExtStrData(DBDIVEXSTR_GATEWAYLIST, addr_list);
		if(!addr_list.NotEmptyS()) {
			getCtrlString(CTL_DBDIV_ADDR, addr_list);
		}
		if(EditDBDivAddrList(&addr_list) > 0) {
			SetupAddr(addr_list);
		}
	}
	else if(event.isClusterClk(CTL_DBDIV_MQBEXCHG)) {
		GetClusterData(CTL_DBDIV_MQBEXCHG, &Data.Rec.Flags); 
		disableCtrl(CTL_DBDIV_ADDR, LOGIC(Data.Rec.Flags & DBDIVF_MQBEXCHANGE));
		enableCommand(cmAddrList, !(Data.Rec.Flags & DBDIVF_MQBEXCHANGE));
	}
	else
		return;
	clearEvent(event);
}

int PPObjDBDiv::Get(PPID id, DBDivPack * pPack)
{
	int    ok = 1;
	if(pPack) {
		THROW(P_Ref->GetItem(Obj, id, &pPack->Rec) > 0);
		THROW(P_Ref->GetPropArray(Obj, id, DBDPRP_LOCLIST, &pPack->LocList));
		THROW(P_Ref->GetPropVlrString(Obj, id, DBDPRP_EXTSTRING, pPack->ExtString));
		pPack->ExtString.Strip();
	}
	CATCH
		ok = 0;
		CALLPTRMEMB(pPack, Init());
	ENDCATCH
	return ok;
}

StrAssocArray * PPObjDBDiv::MakeStrAssocList(void * extraPtr)
{
	const long extra_param = reinterpret_cast<long>(extraPtr);
	PPDBDiv rec;
	StrAssocArray * p_list = new StrAssocArray();
	THROW_MEM(p_list);
	for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
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

int PPObjDBDiv::Put(PPID * pID, DBDivPack * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(P_Ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
		}
		else {
			THROW(P_Ref->AddItem(Obj, &pPack->Rec.ID, &pPack->Rec, 0));
			*pID = pPack->Rec.ID;
		}
		THROW(P_Ref->PutPropArray(Obj, *pID, DBDPRP_LOCLIST, &pPack->LocList, 0));
		THROW(P_Ref->PutPropVlrString(Obj, *pID, DBDPRP_EXTSTRING, pPack->ExtString));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjDBDiv::AcceptUuid(PPID dbid, const S_GUID & rUuid, int use_ta)
{
	int    ok = -1;
	PPDBDiv rec;
	THROW(P_Ref->GetItem(PPOBJ_DBDIV, dbid, &rec) > 0);
	if(rec.Uuid.IsZero() && !rUuid.IsZero()) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(P_Ref->GetItem(PPOBJ_DBDIV, dbid, &rec) > 0);
		rec.Uuid = rUuid;
		THROW(P_Ref->UpdateItem(PPOBJ_DBDIV, dbid, &rec, 1, 0));
		THROW(tra.Commit());
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjDBDiv::GetUuid(PPID dbid, S_GUID * pUuid, int use_ta)
{
	int    ok = 1;
	PPDBDiv rec;
	dbid = NZOR(dbid, LConfig.DBDiv);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(P_Ref->GetItem(PPOBJ_DBDIV, dbid, &rec) > 0);
		if(rec.Uuid.IsZero()) {
			rec.Uuid.Generate();
			THROW(P_Ref->UpdateItem(PPOBJ_DBDIV, dbid, &rec, 1, 0));
		}
		THROW(tra.Commit());
		ASSIGN_PTR(pUuid, rec.Uuid);
		ok = rec.Uuid.IsZero() ? 0 : 1;
	}
	CATCHZOK
	return ok;
}

int PPObjDBDiv::GetCounter(PPID dbid, long * pCounter, int use_ta)
{
	int    ok = 1;
	PPDBDiv rec;
	dbid = NZOR(dbid, LConfig.DBDiv);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(P_Ref->GetItem(PPOBJ_DBDIV, dbid, &rec) > 0);
		*pCounter = ++rec.OutCounter;
		if(rec.OutCounter >= 1000000L)
			rec.OutCounter = 0;
		THROW(P_Ref->UpdateItem(PPOBJ_DBDIV, dbid, &rec, 1, 0));
		THROW(tra.Commit());
	}
	CATCH
		*pCounter = 0;
		ok = 0;
	ENDCATCH
	return ok;
}

int PPObjDBDiv::Edit(PPID * pID, void * extraPtr)
{
	int    r = cmCancel;
	int    valid_data = 0;
	bool   is_new = false;
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

/*virtual*/int PPObjDBDiv::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options, void * pExtraParam)
{
	int    r = -1;
	THROW(CheckRights(PPR_DEL));
	if(id != LConfig.DBDiv && (!(options & PPObject::user_request) || PPMessage(mfConf|mfYesNo, PPCFM_DELDBDIV) == cmYes)) {
		int    use_ta = BIN(options & PPObject::use_transaction);
		ObjSyncTbl sync_tbl;
		PPWaitStart();
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(deleteFrom(&sync_tbl, 0, (sync_tbl.DBID == id)));
		THROW(P_Ref->RemoveItem(Obj, id, 0) && RemoveSync(id));
		THROW(tra.Commit());
		r = 1;
	}
	CATCH
		r = 0;
		if(options & PPObject::user_request)
			PPError();
	ENDCATCH
	PPWaitStop();
	return r;
}

int PPObjDBDiv::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	if(msg == DBMSG_OBJDELETE) {
		PPDBDiv rec;
		if(_obj == PPOBJ_OPRKIND) {
			for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
				if(rec.IntrRcptOpr == _id)
					return RetRefsExistsErr(Obj, rec.ID);
			}
		}
		else if(_obj == PPOBJ_LOCATION) {
			for(SEnum en = P_Ref->Enum(Obj, Reference::eoIdName); en.Next(&rec) > 0;) {
				DBDivPack pack;
				if(Get(rec.ID, &pack) > 0 && pack.LocList.lsearch(_id))
					return RetRefsExistsErr(Obj, rec.ID);
			}
		}
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

PPALDD_DESTRUCTOR(DBDiv) { Destroy(); }

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
