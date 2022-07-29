// PPRIGHTS.CPP
// Copyright (c) A.Sobolev, A.Starodub 1996, 1997, 1998, 1999, 2000-2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
// Права доступа
//
#include <pp.h>
#pragma hdrstop

ObjRights::ObjRights(PPID objType) : ObjType(objType), Size(sizeof(ObjRights)), Flags(PPRights::GetDefaultFlags()), OprFlags(PPRights::GetDefaultOprFlags())
{
}

/*static*/ObjRights * ObjRights::Create(PPID objType, size_t totalSize)
{
	const size_t total_size = MAX(totalSize, sizeof(ObjRights));
	ObjRights * ptr = reinterpret_cast<ObjRights *>(::new uint8[total_size]);
	if(ptr) {
		ptr->ObjType = objType;
		ptr->Size = static_cast<uint16>(total_size);
		ptr->Flags = PPRights::GetDefaultFlags();
		ptr->OprFlags = PPRights::GetDefaultOprFlags();
	}
	else
		PPSetErrorNoMem();
	return ptr;
}

/*static*/void FASTCALL ObjRights::Destroy(ObjRights * pObj)
{
    if(pObj)
		::delete [] reinterpret_cast<uint8 *>(pObj);
}
//
//
//
int FASTCALL AdjustPeriodToRights(DateRange & rPeriod, int checkOnly)
{
	return ObjRts.AdjustBillPeriod(rPeriod, checkOnly);
}

class RightsDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPRights);
public:
	RightsDialog() : PPListDialog(DLG_RTCOMM, CTL_RTCOMM_OBJLIST)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		PPAccessRestriction accsr;
		Data.GetAccessRestriction(accsr);
		setCtrlData(CTL_RTCOMM_PWMIN,    &accsr.PwMinLen);
		setCtrlData(CTL_RTCOMM_PWPERIOD, &accsr.PwPeriod);
		setCtrlData(CTL_RTCOMM_ACCESS,   &accsr.AccessLevel);
		accsr.SetPeriodInputExt(this, CTL_RTCOMM_RBILLPRD, PPAccessRestriction::pparR);
		accsr.SetPeriodInputExt(this, CTL_RTCOMM_WBILLPRD, PPAccessRestriction::pparW);
		AddClusterAssoc(CTL_RTCOMM_PRDFORCSESS, 0, PPAccessRestriction::cfApplyBillPeriodsToCSess);
		SetClusterData(CTL_RTCOMM_PRDFORCSESS, accsr.CFlags);
		/*
		AddClusterAssoc(CTL_RTCOMM_RTDESKTOP, 0, PPR_INS);
		AddClusterAssoc(CTL_RTCOMM_RTDESKTOP, 1, PPR_MOD);
		SetClusterData(CTL_RTCOMM_RTDESKTOP, (long)accsr.RtDesktop);
		*/
		AddClusterAssoc(CTL_RTCOMM_RTDESKTOP, 0, PPAccessRestriction::cfDesktopCr);
		AddClusterAssoc(CTL_RTCOMM_RTDESKTOP, 1, PPAccessRestriction::cfDesktopMod);
		SetClusterData(CTL_RTCOMM_RTDESKTOP, accsr.CFlags);

		AddClusterAssocDef(CTL_RTCOMM_OWNBILLRESTR, 0, 0);
		AddClusterAssoc(CTL_RTCOMM_OWNBILLRESTR, 1, PPAccessRestriction::cfOwnBillRestr);
		AddClusterAssoc(CTL_RTCOMM_OWNBILLRESTR, 2, PPAccessRestriction::cfOwnBillRestr2);
		SetClusterData(CTL_RTCOMM_OWNBILLRESTR, (accsr.CFlags & (PPAccessRestriction::cfOwnBillRestr | PPAccessRestriction::cfOwnBillRestr2)));

		SetupPPObjCombo(this, CTLSEL_RTCOMM_ONLYGGRP, PPOBJ_GOODSGROUP, accsr.OnlyGoodsGrpID, OLW_CANSELUPLEVEL, 0);
		AddClusterAssoc(CTL_RTCOMM_ONLYGGRPSTRIC, 0, PPAccessRestriction::cfStrictOnlyGoodsGrp);
		SetClusterData(CTL_RTCOMM_ONLYGGRPSTRIC, accsr.CFlags);
		// @v10.5.7 {
		AddClusterAssoc(CTL_RTCOMM_DBXRCV, 0, PPAccessRestriction::cfAllowDbxReceive);
		SetClusterData(CTL_RTCOMM_DBXRCV, accsr.CFlags);
		// } @v10.5.7
		DisableClusterItem(CTL_RTCOMM_ONLYGGRPSTRIC, 0, accsr.OnlyGoodsGrpID == 0);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		// @v10.3.0 (never used) long   rt_desk = 0;
		PPAccessRestriction accsr;
		// @v10.7.8 @ctr MEMSZERO(accsr);
		getCtrlData(CTL_RTCOMM_PWMIN,    &accsr.PwMinLen);
		getCtrlData(CTL_RTCOMM_PWPERIOD, &accsr.PwPeriod);
		getCtrlData(CTL_RTCOMM_ACCESS,   &accsr.AccessLevel);
		accsr.GetPeriodInputExt(this, CTL_RTCOMM_RBILLPRD, PPAccessRestriction::pparR);
		accsr.GetPeriodInputExt(this, CTL_RTCOMM_WBILLPRD, PPAccessRestriction::pparW);
		GetClusterData(CTL_RTCOMM_PRDFORCSESS, &accsr.CFlags); // @v9.2.11
		getCtrlData(CTLSEL_RTCOMM_ONLYGGRP, &accsr.OnlyGoodsGrpID);
		//GetClusterData(CTL_RTCOMM_RTDESKTOP, &rt_desk);
		//accsr.RtDesktop = (uint8)rt_desk;
		GetClusterData(CTL_RTCOMM_RTDESKTOP, &accsr.CFlags);
		{
			long temp = 0;
			GetClusterData(CTL_RTCOMM_OWNBILLRESTR, &temp);
			accsr.CFlags &= ~(PPAccessRestriction::cfOwnBillRestr | PPAccessRestriction::cfOwnBillRestr2);
			accsr.CFlags |= temp;
		}
		GetClusterData(CTL_RTCOMM_ONLYGGRPSTRIC, &accsr.CFlags);
		GetClusterData(CTL_RTCOMM_DBXRCV, &accsr.CFlags); // @v10.5.7
		accsr.SetSaveMode(1);
		Data.SetAccessRestriction(&accsr);
		accsr.SetSaveMode(0);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int editItem(long pos, long id);
	void   editObjRights();
	void   editOpList();
	void   editLocList();
	void   editCfgList();
	void   editAccList();
	void   editPosNodeList();
	void   editQuotKindList();
};

IMPL_HANDLE_EVENT(RightsDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmRtOpList))
		editOpList();
	else if(event.isCmd(cmRtLocList))
		editLocList();
	else if(event.isCmd(cmRtConfigList))
		editCfgList();
	else if(event.isCmd(cmRtAccList))
		editAccList();
	else if(event.isCmd(cmRtPosNodeList))
		editPosNodeList();
	else if(event.isCmd(cmRtQuotKindList))
		editQuotKindList();
	else if(event.isCbSelected(CTLSEL_RTCOMM_ONLYGGRP)) {
		PPID grp_id = getCtrlLong(CTLSEL_RTCOMM_ONLYGGRP);
		DisableClusterItem(CTL_RTCOMM_ONLYGGRPSTRIC, 0, grp_id == 0);
	}
	else
		return;
	clearEvent(event);
}

int RightsDialog::setupList()
{
	int    ok = 1;
	int    defrt;
	uint   i;
	char   rtstr[32];
	ushort rtfld;
	StrAssocArray temp_list;
	StringSet ss(SLBColumnDelim);
	SString obj_title;
	PPIDArray obj_type_list;
	PPGetObjTypeList(&obj_type_list, 0);
	const  int inh = Data.IsInherited();
	for(i = 0; i < obj_type_list.getCount(); i++) {
		const PPID obj_type = obj_type_list.get(i);
		if(!IS_DYN_OBJTYPE(obj_type))
			temp_list.Add(obj_type, GetObjectTitle(obj_type, obj_title));
	}
	temp_list.SortByText();
	for(i = 0; i < temp_list.getCount(); i++) {
		ss.clear();
		StrAssocArray::Item entry = temp_list.Get(i);
		ObjRights * p_obj_rt = Data.GetObjRights(entry.Id, 1);
		THROW(p_obj_rt);
		rtfld = p_obj_rt->Flags;
		defrt = BIN(p_obj_rt->OprFlags & PPORF_DEFAULT);
		ObjRights::Destroy(p_obj_rt);
		p_obj_rt = 0;
		rtstr[0] = (rtfld & PPR_READ) ? 'R' : ' ';
		rtstr[1] = (rtfld & PPR_INS)  ? 'C' : ' ';
		rtstr[2] = (rtfld & PPR_MOD)  ? 'M' : ' ';
		rtstr[3] = (rtfld & PPR_DEL)  ? 'D' : ' ';
		rtstr[4] = (rtfld & PPR_ADM)  ? 'A' : ' ';
		rtstr[5] = defrt ? '*' : (inh ? '+' : ' ');
		rtstr[6] = 0;
		ss.add(entry.Txt);
		ss.add(rtstr);
		THROW(addStringToList(entry.Id, ss.getBuf()));
		if(entry.Id == PPOBJ_CONFIG) {
			int    not_adm_list = (PPMaster || (rtfld & PPR_ADM)) ? 0 : 1;
			disableCtrls(not_adm_list, CTL_RTCOMM_OPLIST, CTL_RTCOMM_LOCLIST, CTL_RTCOMM_CONFLIST, 0);
		}
	}
	CATCHZOK
	return ok;
}

int RightsDialog::editItem(long pos, long id)
{
	int    ok = -1;
	PPObject  * p_obj   = 0;
	ObjRights * p_obj_rt = 0;
	THROW(p_obj_rt = Data.GetObjRights(id, 1));
	THROW(p_obj = GetPPObject(id, 0));
	if(p_obj->CheckRights(PPR_ADM) && p_obj->EditRights(p_obj_rt->Size, p_obj_rt) > 0) {
		p_obj_rt->ObjType = id;
		p_obj_rt->OprFlags &= ~(PPORF_DEFAULT /*@v8.3.3 | PPORF_INHERITED*/);
		THROW(Data.SetObjRights(id, p_obj_rt, 1));
		ok = 1;
	}
	CATCHZOKPPERR
	delete p_obj;
	ObjRights::Destroy(p_obj_rt);
	return ok;
}
//
//
//
class RtOpListDialog : public ObjRestrictListDialog {
	DECL_DIALOG_DATA(ObjRestrictArray);
public:
	RtOpListDialog() : ObjRestrictListDialog(DLG_RTOPLIST, CTL_RTOPLIST_LIST)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.freeAll();
		setParams(PPOBJ_OPRKIND, &Data);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
	virtual int TransmitData(int dir, void * pData)
	{
		int    s = 0;
		if(dir > 0)
			s = setDTS(static_cast<const ObjRestrictArray *>(pData));
		else if(dir < 0)
			s = getDTS(static_cast<ObjRestrictArray *>(pData));
		else
			s = ObjRestrictListDialog::TransmitData(dir, pData);
		return s;
	}
private:
	virtual void   getObjName(PPID objID, long, SString & rBuf) { GetOpName(objID, rBuf); }
	virtual void   getExtText(PPID objID, long objFlags, SString & rBuf)
	{
		const long comm = (objFlags & 0x80000000) ? 0 : 1;
		if(comm) {
			PPLoadString("rt_inherited", rBuf);
		}
		else {
			rBuf.Z();
			rBuf.CatChar((objFlags & PPR_READ) ? 'R' : ' ');
			rBuf.CatChar((objFlags & PPR_INS)  ? 'C' : ' ');
			rBuf.CatChar((objFlags & PPR_MOD)  ? 'M' : ' ');
			rBuf.CatChar((objFlags & PPR_DEL)  ? 'D' : ' ');
			rBuf.CatChar((objFlags & PPR_ADM)  ? 'A' : ' ');
			if(rBuf.IsEmpty())
				PPLoadString("none", rBuf);
		}
	}
	virtual int    editItemDialog(ObjRestrictItem *);
};

int RtOpListDialog::editItemDialog(ObjRestrictItem * pItem)
{
	class RtOpItemDialog : public TDialog {
		DECL_DIALOG_DATA(ObjRestrictItem);
	public:
		explicit RtOpItemDialog(const ObjRestrictArray * pList) : TDialog(DLG_RTOPLI), P_List(pList)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			PPID   op_id = 0;
			PPIDArray op_list;
			while(EnumOperations(0L, &op_id) > 0) {
				if(op_id == Data.ObjID || !P_List || !P_List->SearchItemByID(op_id, 0))
					op_list.add(op_id);
			}
			SetupOprKindCombo(this, CTLSEL_RTOPLI_OPRKIND, Data.ObjID, 0, &op_list, OPKLF_OPLIST|OPKLF_SHOWPASSIVE); // @v10.8.1 OPKLF_SHOWPASSIVE
			{
				const long comm = (Data.Flags & 0x80000000) ? 0 : 1;
				{
					AddClusterAssoc(CTL_RTOPLI_COMMRT, 0, 1);
					SetClusterData(CTL_RTOPLI_COMMRT, comm);
				}
				AddClusterAssoc(CTL_RTOPLI_FLAGS, 0, PPR_READ);
				AddClusterAssoc(CTL_RTOPLI_FLAGS, 1, PPR_INS);
				AddClusterAssoc(CTL_RTOPLI_FLAGS, 2, PPR_MOD);
				AddClusterAssoc(CTL_RTOPLI_FLAGS, 3, PPR_DEL);
				SetClusterData(CTL_RTOPLI_FLAGS, Data.Flags);
				disableCtrl(CTL_RTOPLI_FLAGS, comm);
			}
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlData(sel = CTLSEL_RTOPLI_OPRKIND, &Data.ObjID);
			THROW_PP(Data.ObjID, PPERR_OPRKINDNEEDED);
			{
				const long comm = GetClusterData(CTL_RTOPLI_COMMRT);
				if(!comm) {
					Data.Flags |= 0x80000000;
					GetClusterData(CTL_RTOPLI_FLAGS, &Data.Flags);
				}
				else
					Data.Flags = 0;
				ASSIGN_PTR(pData, Data);
			}
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_RTOPLI_COMMRT)) {
				const long comm = GetClusterData(CTL_RTOPLI_COMMRT);
				disableCtrl(CTL_RTOPLI_FLAGS, comm);
			}
			else
				return;
			clearEvent(event);
		}
		const ObjRestrictArray * P_List;
	};
	int    ok = -1, valid_data = 0;
	RtOpItemDialog * dlg = new RtOpItemDialog(&Data);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(pItem);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(pItem)) {
				ok = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

void RightsDialog::editOpList()
{
	RtOpListDialog * dlg = new RtOpListDialog;
	if(CheckDialogPtrErr(&dlg)) {
		ObjRestrictArray temp_list;
		ObjRestrictArray * p_list = NZOR(Data.P_OpList, &temp_list);
		dlg->setDTS(p_list);
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(p_list);
			if(p_list->getCount()) {
				SETIFZ(Data.P_OpList, new ObjRestrictArray(temp_list));
			}
			else
				ZDELETE(Data.P_OpList);
		}
		delete dlg;
	}
}
//
//
//
class RtLocListDialog : public ObjRestrictListDialog {
	DECL_DIALOG_DATA(ObjRestrictArray);
public:
	RtLocListDialog() : ObjRestrictListDialog(DLG_RTLOCLIST, CTL_RTLOCLIST_LIST)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.freeAll();
		setParams(PPOBJ_LOCATION, &Data);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
	virtual int TransmitData(int dir, void * pData)
	{
		int    s = 0;
		if(dir > 0)
			s = setDTS(static_cast<const ObjRestrictArray *>(pData));
		else if(dir < 0)
			s = getDTS(static_cast<ObjRestrictArray *>(pData));
		else
			s = ObjRestrictListDialog::TransmitData(dir, pData);
		return s;
	}
private:
	//virtual void getObjName(PPID objID, long objFlags, SString &);
	//virtual void getExtText(long objFlags, SString &);
	virtual int editItemDialog(ObjRestrictItem * pItem)
	{
		int    ok = -1, valid_data = 0;
		TDialog * p_dlg = new TDialog(DLG_RTLOCLI);
		if(CheckDialogPtrErr(&p_dlg)) {
			StrAssocArray loc_list;
			{
				SString loc_name;
				PPIDArray _list;
				LocObj.GetWarehouseList(&_list, 0);
				for(uint i = 0; i < _list.getCount(); i++) {
					PPID loc_id = _list.at(i);
					if(!Data.SearchItemByID(loc_id, 0) || loc_id == pItem->ObjID) {
						GetObjectName(PPOBJ_LOCATION, loc_id, loc_name, 0);
						loc_list.Add(loc_id, loc_name);
					}
				}
			}
			SetupStrAssocCombo(p_dlg, CTLSEL_RTLOCLI_LOC, loc_list, pItem->ObjID, 0, 0, 0);
			// SetupPPObjCombo(dlg, CTLSEL_RTLOCLI_LOC, PPOBJ_LOCATION, pItem->ObjID, 0, 0);
			while(!valid_data && ExecView(p_dlg) == cmOK) {
				p_dlg->getCtrlData(CTLSEL_RTLOCLI_LOC, &pItem->ObjID);
				pItem->Flags = 0;
				if(pItem->ObjID)
					ok = valid_data = 1;
			}
		}
		else
			ok = 0;
		delete p_dlg;
		return ok;
	}
	PPObjLocation LocObj;
};

void RightsDialog::editLocList()
{
	RtLocListDialog * dlg = new RtLocListDialog;
	if(CheckDialogPtrErr(&dlg)) {
		ObjRestrictArray temp_list;
		ObjRestrictArray * p_list = Data.P_LocList ? Data.P_LocList : &temp_list;
		dlg->setDTS(p_list);
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(p_list);
			if(p_list->getCount()) {
				SETIFZ(Data.P_LocList, new ObjRestrictArray(temp_list));
			}
			else
				ZDELETE(Data.P_LocList);
		}
		delete dlg;
	}
}

class RtCfgListDialog : public PPListDialog {
	DECL_DIALOG_DATA(ObjRestrictArray);
public:
	RtCfgListDialog() : PPListDialog(DLG_RTCFGLIST, CTL_RTCFGLIST_LIST)
	{
		PPLoadText(PPTXT_CFGNAMES, CfgNames);
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.freeAll();
		StrAssocArray t_ary;
		PPGetConfigList(&t_ary);
		for(uint i = Data.getCount(); i < t_ary.getCount(); i++)
			Data.Add(t_ary.Get(i).Id, 0, 0);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int delItem(long pos, long id);

	SString CfgNames;
};

int RtCfgListDialog::setupList()
{
	ObjRestrictItem * p_item;
	SString sub;
	for(uint i = 0; Data.enumItems(&i, (void **)&p_item);) {
		if(p_item->Flags) {
			long rtfld = p_item->Flags;
			StringSet ss(SLBColumnDelim);
			PPGetSubStr(CfgNames, i - 1, sub);
			ss.add(sub);
			sub.Z();
			sub.CatChar((rtfld & PPR_READ) ? 'R' : ' ');
			sub.CatChar((rtfld & PPR_INS)  ? 'C' : ' ');
			sub.CatChar((rtfld & PPR_MOD)  ? 'M' : ' ');
			sub.CatChar((rtfld & PPR_DEL)  ? 'D' : ' ');
			sub.CatChar((rtfld & PPR_ADM)  ? 'A' : ' ');
			ss.add(sub);
			if(!addStringToList(p_item->ObjID, ss.getBuf()))
				return 0;
		}
	}
	return 1;
}

int RtCfgListDialog::delItem(long pos, long id)
{
	uint upos = 0;
	if(Data.SearchItemByID(id, &upos))
		Data.at(upos).Flags = 0;
	return 1;
}

int RtCfgListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1, valid_data = 0;
	TDialog * p_dlg = new TDialog(DLG_CBXSEL);
	if(CheckDialogPtrErr(&p_dlg)) {
		StrAssocArray t_ary;
		SString buf, title;
		PPGetConfigList(&t_ary);
		PPLoadString("config", title); // @v11.2.4 @fix "@config"-->"config"
		p_dlg->setSubTitle(title);
		p_dlg->setLabelText(CTL_CBXSEL_COMBO, title);
		SetupStrAssocCombo(p_dlg, CTLSEL_CBXSEL_COMBO, t_ary, 0, 0);
		while(!valid_data && ExecView(p_dlg) == cmOK) {
			uint   pos = 0;
			PPID   id = p_dlg->getCtrlLong(CTLSEL_CBXSEL_COMBO);
			if(Data.SearchItemByID(id, &pos))
				Data.at(pos).Flags = PPR_READ|PPR_INS|PPR_MOD|PPR_DEL;
			if(id) {
				ASSIGN_PTR(pPos, pos);
				ASSIGN_PTR(pID, id);
				ok = valid_data = 1;
			}
		}
	}
	else
		ok = 0;
	delete p_dlg;
	return ok;
}

void RightsDialog::editCfgList()
{
	RtCfgListDialog * dlg = new RtCfgListDialog;
	if(CheckDialogPtrErr(&dlg)) {
		ObjRestrictArray temp_list;
		ObjRestrictArray * p_list = Data.P_CfgList ? Data.P_CfgList : &temp_list;
		dlg->setDTS(p_list);
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(p_list);
			if(p_list->getCount()) {
				SETIFZ(Data.P_CfgList, new ObjRestrictArray(temp_list));
			}
			else
				ZDELETE(Data.P_CfgList);
		}
		delete dlg;
	}
}
//
//
//
class RtAccListDialog : public ObjRestrictListDialog {
	DECL_DIALOG_DATA(ObjRestrictArray);
public:
	RtAccListDialog() : ObjRestrictListDialog(DLG_RTACCLIST, CTL_RTACCLIST_LIST)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.freeAll();
		if(!Data.SearchItemByID(0, 0))
			Data.Add(0, PPR_READ, 0);
		setParams(PPOBJ_ACCOUNT2, &Data); // @v9.0.4 PPOBJ_ACCOUNT-->PPOBJ_ACCOUNT2
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
	virtual int TransmitData(int dir, void * pData)
	{
		int    s = 0;
		if(dir > 0)
			s = setDTS(static_cast<const ObjRestrictArray *>(pData));
		else if(dir < 0)
			s = getDTS(static_cast<ObjRestrictArray *>(pData));
		else
			s = ObjRestrictListDialog::TransmitData(dir, pData);
		return s;
	}
private:
	virtual int addItem(long * pPos, long * pID)
	{
		if(P_ORList) {
			uint   pos = 0;
			ObjRestrictItem item;
			// @v10.7.8 @ctr MEMSZERO(item);
			if(editItemDialog(&item, 1) > 0)
				if(Data.Add(item.ObjID, item.Flags, &pos)) {
					ASSIGN_PTR(pID, item.ObjID);
					ASSIGN_PTR(pPos, pos);
					return 1;
				}
				else
					return 0;
		}
		return -1;
	}
	virtual int editItem(long pos, long id)
	{
		uint   p = (uint)pos;
		if(p < Data.getCount()) {
			ObjRestrictItem item = Data.at(p);
			if(editItemDialog(&item, 0) > 0) {
				Data.at(p) = item;
				return 1;
			}
		}
		return -1;
	}
	virtual int delItem(long pos, long id)
	{
		return (pos >= 0 && pos < (long)Data.getCount()) ? ObjRestrictListDialog::delItem(pos, id) : -1;
	}
	int editItemDialog(ObjRestrictItem * pItem, int isNew)
	{
		int    ok = -1, valid_data = 0;
		StrAssocArray * p_acc_list = 0;
		TDialog * p_dlg = new TDialog(DLG_RTACCLI);
		if(CheckDialogPtrErr(&p_dlg)) {
			int    others_acc = (pItem->ObjID == 0 && isNew == 0) ? 1 : 0;
			p_acc_list = AcctObj.MakeStrAssocList(reinterpret_cast<void *>(ACY_SEL_BAL));
			p_dlg->AddClusterAssoc(CTL_RTACCLI_FLAGS, 0, PPR_READ);
			p_dlg->AddClusterAssoc(CTL_RTACCLI_FLAGS, 1, 0);
			p_dlg->SetClusterData(CTL_RTACCLI_FLAGS, pItem->Flags);
			if(p_acc_list) {
				for(uint i = 0; i < Data.getCount(); i++)
					if(Data.at(i).ObjID != pItem->ObjID)
						p_acc_list->Remove(Data.at(i).ObjID);
				SetupStrAssocCombo(p_dlg, CTLSEL_RTACCLI_ACC, *p_acc_list, pItem->ObjID, 0, 0, 0);
			}
			p_dlg->disableCtrl(CTLSEL_RTACCLI_ACC, others_acc);
			while(!valid_data && ExecView(p_dlg) == cmOK) {
				p_dlg->getCtrlData(CTLSEL_RTACCLI_ACC, &pItem->ObjID);
				p_dlg->GetClusterData(CTL_RTACCLI_FLAGS, &pItem->Flags);
				if(pItem->ObjID || others_acc)
					ok = valid_data = 1;
			}
		}
		else
			ok = 0;
		delete p_dlg;
		ZDELETE(p_acc_list);
		return ok;
	}
	virtual void getObjName(PPID objID, long objFlags, SString & rBuf)
	{
		if(objID == 0)
			PPGetWord(PPWORD_OTHERS, 0, rBuf);
		else {
			PPAccount acc_rec;
			if(AcctObj.Search(objID, &acc_rec) > 0)
				(rBuf = acc_rec.Code).Strip().CatDiv('-', 1).Cat(acc_rec.Name);
			else
				ideqvalstr(objID, rBuf);
		}
	}
	virtual void getExtText(PPID objID, long objFlags, SString & rBuf)
	{
		PPLoadString((objFlags == 0) ? "no" : "yes", rBuf);
	}
	PPObjAccount AcctObj;
};

void RightsDialog::editAccList()
{
	RtAccListDialog * dlg = new RtAccListDialog;
	if(CheckDialogPtrErr(&dlg)) {
		ObjRestrictArray temp_list;
		ObjRestrictArray * p_list = Data.P_AccList ? Data.P_AccList : &temp_list;
		dlg->setDTS(p_list);
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(p_list);
			if(p_list->getCount()) {
				SETIFZ(Data.P_AccList, new ObjRestrictArray(temp_list));
			}
			else
				ZDELETE(Data.P_AccList);
		}
		delete dlg;
	}
}
//
//
//
class RtPosNodeListDialog : public ObjRestrictListDialog {
public:
	RtPosNodeListDialog() : ObjRestrictListDialog(DLG_RTPOSLIST, CTL_RTPOSLIST_LIST)
	{
	}
	int    setDTS(const ObjRestrictArray * pData)
	{
		if(!RVALUEPTR(Data, pData))
			Data.freeAll();
		setParams(PPOBJ_CASHNODE, &Data);
		updateList(-1);
		return 1;
	}
	int    getDTS(ObjRestrictArray * pData)
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
	virtual int TransmitData(int dir, void * pData)
	{
		int    s = 0;
		if(dir > 0)
			s = setDTS(static_cast<const ObjRestrictArray *>(pData));
		else if(dir < 0)
			s = getDTS(static_cast<ObjRestrictArray *>(pData));
		else
			s = ObjRestrictListDialog::TransmitData(dir, pData);
		return s;
	}
private:
	virtual int addItem(long * pPos, long * pID)
	{
		if(P_ORList) {
			uint   pos = 0;
			ObjRestrictItem item;
			// @v10.7.8 @ctr MEMSZERO(item);
			if(editItemDialog(&item, 1) > 0)
				if(Data.Add(item.ObjID, item.Flags, &pos)) {
					ASSIGN_PTR(pID, item.ObjID);
					ASSIGN_PTR(pPos, pos);
					return 1;
				}
				else
					return 0;
		}
		return -1;
	}
	virtual int editItem(long pos, long id)
	{
		uint   p = (uint)pos;
		if(p < Data.getCount()) {
			ObjRestrictItem item = Data.at(p);
			if(editItemDialog(&item, 0) > 0) {
				Data.at(p) = item;
				return 1;
			}
		}
		return -1;
	}
	virtual int delItem(long pos, long id)
	{
		return (pos >= 0 && pos < (long)Data.getCount()) ? ObjRestrictListDialog::delItem(pos, id) : -1;
	}
	int editItemDialog(ObjRestrictItem * pItem, int isNew)
	{
		int    ok = -1;
		int    valid_data = 0;
		StrAssocArray * p_list = 0;
		TDialog * p_dlg = new TDialog(DLG_RTPOSLI);
		if(CheckDialogPtrErr(&p_dlg)) {
			int    others = BIN(pItem->ObjID == 0 && isNew == 0);
			p_list = CnObj.MakeStrAssocList(0);
			if(p_list) {
				for(uint i = 0; i < Data.getCount(); i++)
					if(Data.at(i).ObjID != pItem->ObjID)
						p_list->Remove(Data.at(i).ObjID);
				SetupStrAssocCombo(p_dlg, CTLSEL_RTPOSLI_ITEM, *p_list, pItem->ObjID, 0, 0, 0);
			}
			p_dlg->disableCtrl(CTLSEL_RTPOSLI_ITEM, others);
			while(!valid_data && ExecView(p_dlg) == cmOK) {
				p_dlg->getCtrlData(CTLSEL_RTPOSLI_ITEM, &pItem->ObjID);
				if(pItem->ObjID || others)
					ok = valid_data = 1;
			}
		}
		else
			ok = 0;
		delete p_dlg;
		ZDELETE(p_list);
		return ok;
	}
	virtual void getObjName(PPID objID, long objFlags, SString & rBuf)
	{
		if(objID == 0)
			PPGetWord(PPWORD_OTHERS, 0, rBuf);
		else {
			PPCashNode rec;
			if(CnObj.Search(objID, &rec) > 0)
				rBuf = rec.Name;
			else
				ideqvalstr(objID, rBuf);
		}
	}
	virtual void getExtText(PPID objID, long objFlags, SString & rBuf)
	{
		// @v10.4.10 PPLoadString((objFlags == 0) ? "no" : "yes", rBuf);
		PPLoadString("yes", rBuf); // @v10.4.10
	}
	ObjRestrictArray Data;
	PPObjCashNode CnObj;
};

void RightsDialog::editPosNodeList()
{
	RtPosNodeListDialog * dlg = new RtPosNodeListDialog;
	if(CheckDialogPtrErr(&dlg)) {
		ObjRestrictArray temp_list;
		ObjRestrictArray * p_list = NZOR(Data.P_PosList, &temp_list);
		dlg->setDTS(p_list);
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(p_list);
			if(p_list->getCount()) {
				SETIFZ(Data.P_PosList, new ObjRestrictArray(temp_list));
			}
			else
				ZDELETE(Data.P_PosList);
		}
		delete dlg;
	}
}
//
//
//
class RtQuotKindListDialog : public ObjRestrictListDialog {
public:
	RtQuotKindListDialog() : ObjRestrictListDialog(DLG_RTQKLIST, CTL_RTQKLIST_LIST)
	{
	}
	int    setDTS(const ObjRestrictArray * pData)
	{
		if(!RVALUEPTR(Data, pData))
			Data.freeAll();
		setParams(PPOBJ_QUOTKIND, &Data);
		updateList(-1);
		return 1;
	}
	int    getDTS(ObjRestrictArray * pData)
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
	virtual int TransmitData(int dir, void * pData)
	{
		int    s = 0;
		if(dir > 0)
			s = setDTS(static_cast<const ObjRestrictArray *>(pData));
		else if(dir < 0)
			s = getDTS(static_cast<ObjRestrictArray *>(pData));
		else
			s = ObjRestrictListDialog::TransmitData(dir, pData);
		return s;
	}
private:
	virtual int addItem(long * pPos, long * pID)
	{
		if(P_ORList) {
			uint   pos = 0;
			ObjRestrictItem item;
			// @v10.7.8 @ctr MEMSZERO(item);
			if(editItemDialog(&item, 1) > 0)
				if(Data.Add(item.ObjID, item.Flags, &pos)) {
					ASSIGN_PTR(pID, item.ObjID);
					ASSIGN_PTR(pPos, pos);
					return 1;
				}
				else
					return 0;
		}
		return -1;
	}
	virtual int editItem(long pos, long id)
	{
		uint   p = static_cast<uint>(pos);
		if(p < Data.getCount()) {
			ObjRestrictItem item = Data.at(p);
			if(editItemDialog(&item, 0) > 0) {
				Data.at(p) = item;
				return 1;
			}
		}
		return -1;
	}
	virtual int delItem(long pos, long id)
	{
		return (pos >= 0 && pos < static_cast<long>(Data.getCount())) ? ObjRestrictListDialog::delItem(pos, id) : -1;
	}
	int editItemDialog(ObjRestrictItem * pItem, int isNew)
	{
		int    ok = -1;
		int    valid_data = 0;
		StrAssocArray * p_list = 0;
		TDialog * p_dlg = new TDialog(DLG_RTQKLI);
		if(CheckDialogPtrErr(&p_dlg)) {
			int    others = (pItem->ObjID == 0 && isNew == 0) ? 1 : 0;
			p_list = QkObj.MakeStrAssocList(0);
			if(p_list) {
				for(uint i = 0; i < Data.getCount(); i++)
					if(Data.at(i).ObjID != pItem->ObjID)
						p_list->Remove(Data.at(i).ObjID);
				SetupStrAssocCombo(p_dlg, CTLSEL_RTQKLI_ITEM, *p_list, pItem->ObjID, 0, 0, 0);
			}
			p_dlg->disableCtrl(CTLSEL_RTQKLI_ITEM, others);
			while(!valid_data && ExecView(p_dlg) == cmOK) {
				p_dlg->getCtrlData(CTLSEL_RTQKLI_ITEM, &pItem->ObjID);
				if(pItem->ObjID || others)
					ok = valid_data = 1;
			}
		}
		else
			ok = 0;
		delete p_dlg;
		ZDELETE(p_list);
		return ok;
	}
	virtual void getObjName(PPID objID, long objFlags, SString & rBuf)
	{
		if(objID == 0)
			PPGetWord(PPWORD_OTHERS, 0, rBuf);
		else {
			PPCashNode rec;
			if(QkObj.Search(objID, &rec) > 0)
				rBuf = rec.Name;
			else
				ideqvalstr(objID, rBuf);
		}
	}
	virtual void getExtText(PPID objID, long objFlags, SString & rBuf)
	{
		PPLoadString((objFlags == 0) ? "no" : "yes", rBuf);
	}
	ObjRestrictArray Data;
	PPObjQuotKind QkObj;
};

void RightsDialog::editQuotKindList()
{
	RtQuotKindListDialog * dlg = new RtQuotKindListDialog;
	if(CheckDialogPtrErr(&dlg)) {
		ObjRestrictArray temp_list;
		ObjRestrictArray * p_list = NZOR(Data.P_QkList, &temp_list);
		dlg->setDTS(p_list);
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(p_list);
			if(p_list->getCount()) {
				SETIFZ(Data.P_QkList, new ObjRestrictArray(temp_list));
			}
			else
				ZDELETE(Data.P_QkList);
		}
		delete dlg;
	}
}
//
//
//
int EditRightsDialog(PPRights & rights)
{
	int    r = cmCancel;
	RightsDialog * dlg = new RightsDialog;
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(&rights);
		if((r = ExecView(dlg)) == cmOK)
			dlg->getDTS(&rights);
	}
	delete dlg;
	return r;
}
//
// AdvEditRights
//
class SecurCollection : public SCollection {
public:
	explicit SecurCollection(/*uint aDelta = DEFCOLLECTDELTA,*/uint o = O_COLLECTION) : SCollection(/*aDelta,*/o)
	{
	}
	SecurCollection(const SecurCollection & src) : SCollection(src)
	{
	}
	~SecurCollection()
	{
		while(getCount())
			atFree(0);
	}
	int    copy(const SecurCollection & aSrc);
	PPSecurPacket * at(uint pos) const { return static_cast<PPSecurPacket *>(SCollection::at(pos)); }
	virtual void FASTCALL freeItem(void * item) { if(item) delete static_cast<PPSecurPacket *>(item); }
};

int SecurCollection::copy(const SecurCollection & aSrc)
{
	int    ok = 1;
	PPSecurPacket * p_pack = 0;
	if(aSrc.VFlags & aryDataOwner) {
		freeAll();
		isize = aSrc.isize;
		Limit = 0;
		count = 0;
		VFlags = aSrc.VFlags;
		for(uint i = 0; i < aSrc.getCount(); i++) {
			p_pack = new PPSecurPacket;
			*p_pack = *aSrc.at(i);
			THROW_SL(insert(p_pack));
		}
	}
	else
		SArray::copy(aSrc);
	CATCHZOK
	return ok;
}
//
//   PPAccessRestriction
//
PPAccessRestriction::PPAccessRestriction() : TimeBeg(ZEROTIME), TimeEnd(ZEROTIME), WeekDays(0), PwMinLen(0), PwPeriod(0), AccessLevel(0), CFlags(0), OnlyGoodsGrpID(0),
	ShowInnerDates(0)
{
	RBillPeriod.Z();
	WBillPeriod.Z();
}

void PPAccessRestriction::SetSaveMode(int saveData)
{
	ShowInnerDates = saveData;
}

int PPAccessRestriction::GetRBillPeriod(DateRange * pPeriod) const
{
	DateRange  period = RBillPeriod;
	if(!ShowInnerDates) {
		period.Actualize(ZERODATE);
	}
	ASSIGN_PTR(pPeriod, period);
	return pPeriod ? 1 : -1;
}

int PPAccessRestriction::GetWBillPeriod(DateRange * pPeriod) const
{
	DateRange  period = WBillPeriod;
	if(!ShowInnerDates) {
        period.Actualize(ZERODATE);
	}
	ASSIGN_PTR(pPeriod, period);
	return pPeriod ? 1 : -1;
}

int PPAccessRestriction::SetBillPeriod(const DateRange * pPeriod, int setROrW)
{
	int    ok = -1;
	if(pPeriod && oneof2(setROrW, PPAccessRestriction::pparR, PPAccessRestriction::pparW)) {
		if(setROrW == PPAccessRestriction::pparR)
			RBillPeriod = *pPeriod;
		else
			WBillPeriod = *pPeriod;
		ok = 1;
	}
	return ok;
}

int ParseBound(const char * pBuf, long * pVal)
{
	int    ok = 1;
	char   buf[64];
	long   val = 0;
	STRNSCPY(buf, pBuf);
	strip(buf);
	char * p = buf;
	if(*p == '@') {
		p++;
		while(*p == ' ' || *p == '\t')
			p++;
		if(*p == '+' || *p == '-') {
			int sign = (*p == '+') ? 1 : -1;
			p++;
			while(*p == ' ' || *p == '\t')
				p++;
			char   nmb[32];
			char * n = nmb;
			while(*p >= '0' && *p <= '9')
				*n++ = *p++;
			*n = 0;
			const int shift = satoi(nmb);
			val = MakeLong(shift * sign, 0x8000);
		}
		else if(*p == 0)
			val = MakeLong(0, 0x8000);
		else
			ok = PPSetError(PPERR_USERINPUT);
	}
	else {
		LDATE temp = ZERODATE;
		strtodate(buf, DATF_DMY, &temp);
		if(checkdate(temp, 1))
			val = temp.v;
		else
			ok = PPSetError(PPERR_USERINPUT);
	}
	ASSIGN_PTR(pVal, val);
	return ok;
}

int PPAccessRestriction::GetPeriodInputExt(TDialog * pDlg, uint ctrlID, int setROrW)
{
	int    ok = -1;
	assert(oneof2(setROrW, PPAccessRestriction::pparR, PPAccessRestriction::pparW));
	if(oneof2(setROrW, PPAccessRestriction::pparR, PPAccessRestriction::pparW)) {
		DateRange period;
		period.Z();
		ok = GetPeriodInput(pDlg, ctrlID, &period);
		if(ok > 0) {
			if(setROrW == PPAccessRestriction::pparR)
				RBillPeriod = period;
			else
				WBillPeriod = period;
		}
	}
	return ok;
}

int PPAccessRestriction::SetPeriodInputExt(TDialog * pDlg, uint ctrlID, int getROrW) const
{
	int    ok = -1;
	assert(oneof2(getROrW, PPAccessRestriction::pparR, PPAccessRestriction::pparW));
	if(oneof2(getROrW, PPAccessRestriction::pparR, PPAccessRestriction::pparW)) {
		const DateRange period = (getROrW == PPAccessRestriction::pparR) ? RBillPeriod : WBillPeriod;
		SetPeriodInput(pDlg, ctrlID, &period);
		ok = 1;
	}
	return ok;
}

int PPAccessRestriction::GetOwnBillRestrict() const
{
	const long mask = (cfOwnBillRestr | cfOwnBillRestr2);
	if((CFlags & mask) == cfOwnBillRestr)
		return 1;
	else if((CFlags & mask) == cfOwnBillRestr2)
		return 2;
	else
		return 0;
}
//
//
//
class RightsPeriodDialog : public TDialog {
public:
	RightsPeriodDialog() : TDialog(DLG_RHTSPRD)
	{
	}
	virtual int TransmitData(int dir, void * pData)
	{
		int    s = 0;
		if(dir > 0)
			s = setDTS(static_cast<const PPAccessRestriction *>(pData));
		else if(dir < 0)
			s = getDTS(static_cast<PPAccessRestriction *>(pData));
		else
			s = TDialog::TransmitData(dir, pData);
		return s;
	}
	int    setDTS(const PPAccessRestriction *);
	int    getDTS(PPAccessRestriction *);
};

int RightsPeriodDialog::setDTS(const PPAccessRestriction * pFilt)
{
	if(pFilt) {
		pFilt->SetPeriodInputExt(this, CTL_RHTSPRD_RBILL, PPAccessRestriction::pparR);
		pFilt->SetPeriodInputExt(this, CTL_RHTSPRD_WBILL, PPAccessRestriction::pparW);
	}
	return 1;
}

int RightsPeriodDialog::getDTS(PPAccessRestriction * pFilt)
{
	int    ok = 1;
	if(pFilt) {
		THROW(pFilt->GetPeriodInputExt(this, CTL_RHTSPRD_RBILL, PPAccessRestriction::pparR));
		THROW(pFilt->GetPeriodInputExt(this, CTL_RHTSPRD_WBILL, PPAccessRestriction::pparW));
	}
	CATCHZOK
	return ok;
}

#define USERID_OFFSET  100000L
#define OBJTYPE_OFFSET 100L

class FastEditRightsDlg : public EmbedDialog {
public:
	enum {
		cAccessPeriod = 1,
		cAccessibleOpr,
		cAccessibleLoc,
		cConfig,
		cAccessibleAcc
	};
	explicit FastEditRightsDlg(int readOnly);
	int    setDTS(const SecurCollection *);
	int    getDTS(SecurCollection *);
private:
	DECL_HANDLE_EVENT;
	void   disableChild(int disable);
	int    loadChild(uint editWhat);
	int    setupGrpUsrList();
	int    updateChildsRights(const PPSecurPacket * pParent);
	int    loadPtr(void ** ppPtr, uint pos, int load);
	int    loadData(PPID grpUserID, int load);
	PPID   getCurrID();

	int    ReadOnly;
	uint   EditWhat;
	PPID   GrpUserID;
	SecurCollection Data;
};

FastEditRightsDlg::FastEditRightsDlg(int readOnly) : EmbedDialog(DLG_EDITRHTS), ReadOnly(readOnly), GrpUserID(0), EditWhat(cAccessPeriod)
{
	loadChild(EditWhat);
}

IMPL_HANDLE_EVENT(FastEditRightsDlg)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(event.isCbSelected(CTLSEL_EDITRHTS_EDITWHAT)) {
			uint edit_what = 0;
			getCtrlData(CTL_EDITRHTS_EDITWHAT, &edit_what);
			if(edit_what != EditWhat) {
				PPID grp_user_id = getCurrID();
				if(loadData(grp_user_id, 0) > 0) {
					GrpUserID = grp_user_id;
					loadChild(edit_what);
					EditWhat = edit_what;
					loadData(grp_user_id, 1);
				}
				else {
					setCtrlData(CTLSEL_EDITRHTS_EDITWHAT, &EditWhat);
					PPError();
				}
			}
		}
		else if(TVCMD == cmLBItemFocused) {
			PPID grp_user_id = getCurrID();
			if(grp_user_id != GrpUserID && GrpUserID != 0) {
				if(loadData(GrpUserID, 0) > 0) {
					loadData(grp_user_id, 1);
					GrpUserID = grp_user_id;
				}
				else {
					SmartListBox * p_lbx = static_cast<SmartListBox *>(getCtrlView(CTL_EDITRHTS_GRPUSRLIST));
					if(p_lbx) {
						p_lbx->Search_(&GrpUserID, CMPF_LONG, srchFirst | lbSrchByID);
						p_lbx->Draw_();
						PPError();
					}
				}
			}
		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}


void FastEditRightsDlg::disableChild(int disable)
{
	SString msg_buf;
	if(P_ChildDlg) {
		LONG   style = TView::GetWindowStyle(P_ChildDlg->H());
		if(disable) {
			style |= WS_DISABLED;
			// @v10.0.0 PPGetWord(PPWORD_VIEWONLY, 0, msg_buf);
			PPLoadString("viewonly", msg_buf); // @v10.0.0
		}
		else
			style &= ~WS_DISABLED;
		TView::SetWindowProp(P_ChildDlg->H(), GWL_STYLE, style);
	}
	setStaticText(CTL_EDITRHTS_MSG, msg_buf);
}

PPID FastEditRightsDlg::getCurrID()
{
	PPID   cur_id = 0;
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_EDITRHTS_GRPUSRLIST));
	if(SmartListBox::IsValidS(p_list))
		p_list->P_Def->getCurID(&cur_id);
	return cur_id;
}

IMPL_CMPFUNC(PPSecurPacket, i1, i2)
{
	const PPSecurPacket * p1 = static_cast<const PPSecurPacket *>(i1);
	const PPSecurPacket * p2 = static_cast<const PPSecurPacket *>(i2);
	return (p1->Secur.ID > p2->Secur.ID) ? 1 : ((p1->Secur.ID < p2->Secur.ID) ? -1 : 0);
}

int FastEditRightsDlg::updateChildsRights(const PPSecurPacket * pParent)
{
	if(pParent && pParent->Secur.Tag == PPOBJ_USRGRP) {
		PPSecurPacket * p_pack = 0;
		for(uint i = 0; Data.enumItems(&i, (void **)&p_pack) > 0;) {
			if(p_pack->Secur.ParentID == pParent->Secur.ID && p_pack->Secur.Tag == PPOBJ_USR
				&& ((EditWhat != cConfig && p_pack->Secur.Flags & USRF_INHRIGHTS) || (EditWhat == cConfig && (p_pack->Secur.Flags & USRF_INHCFG)))) {
				switch(EditWhat) {
					case cAccessPeriod:
						{
							PPAccessRestriction a_r;
							pParent->Rights.GetAccessRestriction(a_r);
							a_r.SetSaveMode(1);
							p_pack->Rights.SetAccessRestriction(&a_r);
							a_r.SetSaveMode(0);
						}
						break;
					case cAccessibleOpr:
						if(pParent->Rights.P_OpList) {
							SETIFZ(p_pack->Rights.P_OpList, new ObjRestrictArray());
							p_pack->Rights.P_OpList->copy(*(pParent->Rights.P_OpList));
						}
						else
							ZDELETE(p_pack->Rights.P_OpList);
						break;
					case cAccessibleLoc:
						if(pParent->Rights.P_LocList) {
							SETIFZ(p_pack->Rights.P_LocList, new ObjRestrictArray());
							p_pack->Rights.P_LocList->copy(*(pParent->Rights.P_LocList));
						}
						else
							ZDELETE(p_pack->Rights.P_LocList);
						break;
					case cAccessibleAcc:
						if(pParent->Rights.P_AccList) {
							SETIFZ(p_pack->Rights.P_AccList, new ObjRestrictArray());
							p_pack->Rights.P_AccList->copy(*(pParent->Rights.P_AccList));
						}
						else
							ZDELETE(p_pack->Rights.P_AccList);
						break;
					case cConfig:
						p_pack->Config = pParent->Config;
						break;
					default:
						{
							ObjRights * p_r = pParent->Rights.GetObjRights(EditWhat - OBJTYPE_OFFSET, 1);
							p_pack->Rights.SetObjRights(EditWhat - OBJTYPE_OFFSET, p_r, 1);
							ObjRights::Destroy(p_r);
							p_r = 0;
						}
					break;
				}
			}
		}
	}
	return 1;
}

int FastEditRightsDlg::loadPtr(void ** ppPtr, uint pos, int load)
{
	PPSecurPacket * p_item = (pos < Data.getCount()) ? Data.at(pos) : 0;
	switch(EditWhat) {
		case cAccessPeriod:
			if(load == 1) {
				*ppPtr = new PPAccessRestriction;
				p_item->Rights.GetAccessRestriction(*static_cast<PPAccessRestriction *>(*ppPtr));
			}
			else if(load == 2) {
				delete static_cast<PPAccessRestriction *>(*ppPtr);
				*ppPtr = 0;
			}
			else if(*ppPtr) {
				static_cast<PPAccessRestriction *>(*ppPtr)->SetSaveMode(1);
				p_item->Rights.SetAccessRestriction(static_cast<PPAccessRestriction *>(*ppPtr));
				static_cast<PPAccessRestriction *>(*ppPtr)->SetSaveMode(0);
			}
			break;
		case cAccessibleOpr:
			if(load == 1) {
				SETIFZ(p_item->Rights.P_OpList, new ObjRestrictArray);
				*ppPtr = p_item->Rights.P_OpList;
			}
			else if(!load && (*ppPtr))	{
				p_item->Rights.P_OpList = static_cast<ObjRestrictArray *>(*ppPtr);
				if(p_item->Rights.P_OpList->getCount() <= 0) {
					ZDELETE(p_item->Rights.P_OpList);
					*ppPtr = 0;
				}
			}
			break;
		case cAccessibleLoc:
			if(load == 1) {
				SETIFZ(p_item->Rights.P_LocList, new ObjRestrictArray);
				*ppPtr = p_item->Rights.P_LocList;
			}
			else if(!load && (*ppPtr)) {
				p_item->Rights.P_LocList = static_cast<ObjRestrictArray *>(*ppPtr);
				if(p_item->Rights.P_LocList->getCount() <= 0) {
					ZDELETE(p_item->Rights.P_LocList);
					*ppPtr = 0;
				}
			}
			break;
		case cAccessibleAcc:
			if(load == 1) {
				SETIFZ(p_item->Rights.P_AccList, new ObjRestrictArray);
				*ppPtr = p_item->Rights.P_AccList;
			}
			else if(!load && (*ppPtr)) {
				p_item->Rights.P_AccList = static_cast<ObjRestrictArray *>(*ppPtr);
				if(p_item->Rights.P_AccList->getCount() <= 0) {
					ZDELETE(p_item->Rights.P_AccList);
					*ppPtr = 0;
				}
			}
			break;
		case cConfig:
			if(load == 1)
				*ppPtr = &p_item->Config;
			else if(!load && (*ppPtr))
				p_item->Config = *static_cast<const PPConfig *>(*ppPtr);
			break;
		default:
			if(load == 1)
				*ppPtr = p_item->Rights.GetObjRights(EditWhat - OBJTYPE_OFFSET, 1);
			else if(load == 2) {
				ObjRights * p_r = static_cast<ObjRights *>(*ppPtr);
				ObjRights::Destroy(p_r);
				*ppPtr = 0;
			}
			else if(!load && (*ppPtr)) {
				ObjRights * p_r = static_cast<ObjRights *>(*ppPtr);
				p_r->OprFlags &= ~(PPORF_DEFAULT /* @v8.3.3 | PPORF_INHERITED*/);
				p_item->Rights.SetObjRights(EditWhat - OBJTYPE_OFFSET, p_r, 1);
			}
			break;
	}
	if(!load)
		updateChildsRights(Data.at(pos));
	return 1;
}

int FastEditRightsDlg::loadData(PPID grpUserID, int load)
{
	void * ptr = 0;
	int    ok = 1;
	if(grpUserID != 0) {
		uint   pos = 0;
		PPSecurPacket spack;
		spack.Secur.ID = grpUserID;
		if(Data.lsearch(&spack, &pos, PTR_CMPFUNC(PPSecurPacket))) {
			loadPtr(&ptr, pos, 1); // load data
			if(!load) {
				if(ptr && P_ChildDlg) {
					P_ChildDlg->TransmitData(-1, ptr); // ok = P_ChildDlg->getData(ptr);
					if(ok > 0)
						loadPtr(&ptr, pos, 0); // save data
				}
			}
			else {
				const PPSecurPacket * p_sp = Data.at(pos);
				int    disable = (EditWhat == cConfig) ? BIN(p_sp->Secur.Flags & USRF_INHCFG) : BIN(p_sp->Secur.Flags & USRF_INHRIGHTS);
				PPObject * p_obj = GetPPObject(EditWhat - OBJTYPE_OFFSET, 0);
				if(P_ChildDlg)
					P_ChildDlg->TransmitData(+1, ptr);
				if(p_obj) {
					disable = !p_obj->CheckRights(PPR_ADM) || disable;
					ZDELETE(p_obj);
				}
				disableChild(disable || ReadOnly);
			}
			loadPtr(&ptr, pos, 2); // delete ptr
		}
	}
	return ok;
}

int FastEditRightsDlg::setupGrpUsrList()
{
	int    ok = -1;
	uint   i = 0;
	SString temp_buf, inh_buf;
	StrAssocArray * p_list = 0;
	StdTreeListBoxDef * p_grp_usr_def = 0;
	PPSecurPacket * p_pack = 0;
	SmartListBox * p_lbx = 0;
	THROW_MEM(p_list = new StrAssocArray);
	for(i = 0; Data.enumItems(&i, (void **)&p_pack) > 0; ) {
		temp_buf = p_pack->Secur.Name;
		if(p_pack->Secur.Tag == PPOBJ_USR) {
			inh_buf.Z();
			if(p_pack->Secur.Flags & USRF_INHRIGHTS)
				inh_buf.CatChar('R');
			if(p_pack->Secur.Flags & USRF_INHCFG)
				inh_buf.CatChar('C');
			if(inh_buf.NotEmpty())
				temp_buf.Space().Cat(inh_buf.Quot('(', ')'));
		}
		THROW_SL(p_list->Add(p_pack->Secur.ID, (p_pack->Secur.Tag == PPOBJ_USR) ? p_pack->Secur.ParentID : 0, temp_buf));
	}
	p_lbx = static_cast<SmartListBox *>(getCtrlView(CTL_EDITRHTS_GRPUSRLIST));
	p_grp_usr_def = new StdTreeListBoxDef(p_list, lbtDisposeData | lbtDblClkNotify | lbtFocNotify | lbtSelNotify, 0);
	THROW_MEM(p_grp_usr_def);
	p_lbx->setDef(p_grp_usr_def);
	p_lbx->P_Def->go(0);
	p_lbx->Draw_();
	ok = 1;
	CATCH
		ok = 0;
		ZDELETE(p_grp_usr_def);
		ZDELETE(p_list);
	ENDCATCH
	return ok;
}

int FastEditRightsDlg::loadChild(uint editWhat)
{
	int    ok = -1;
	TDialog * p_dlg = 0;
	switch(editWhat) {
		case cAccessPeriod:  p_dlg = new RightsPeriodDialog(); break;
		case cAccessibleOpr: p_dlg = new RtOpListDialog(); break;
		case cAccessibleLoc: p_dlg = new RtLocListDialog(); break;
		case cAccessibleAcc: p_dlg = new RtAccListDialog(); break;
		case cConfig:
			EditCfgOptionsDialog(0, 0, this);
			setChildPos(CTL_EDITRHTS_GRPUSRLIST);
			ok = 1;
			break;
		default:
			{
				PPObject * p_obj = GetPPObject(editWhat - OBJTYPE_OFFSET, 0);
				if(p_obj) {
					p_obj->EditRights(0, 0, this);
					setChildPos(CTL_EDITRHTS_GRPUSRLIST);
					ZDELETE(p_obj);
					ok = 1;
				}
			}
			break;
	}
	if(ok < 0) {
		ok = Embed(p_dlg);
		setChildPos(CTL_EDITRHTS_GRPUSRLIST);
	}
	return ok;
}

int FastEditRightsDlg::setDTS(const SecurCollection * pData)
{
	int    ok = 1;
	uint   i = 0;
	StrAssocArray temp_list;
	SCollection rights_params;
	if(pData)
		Data.copy(*pData);
	else
		Data.freeAll();
	PPIDArray obj_type_list;
	SString obj_title, buf, sub, rbyo_fmt_buf, rbyo_buf;
	PPLoadText(PPTXT_EDITRIGHTSPARAMS, buf);
	PPLoadText(PPTXT_RIGHTSBYOBJ, rbyo_fmt_buf);

	StringSet ss(';', buf);
	for(uint pos = 0; ss.get(&pos, sub);)
		THROW_SL(rights_params.insert(newStr(sub)));
	PPGetObjTypeList(&obj_type_list, 0);
	for(i = 0; i < obj_type_list.getCount(); i++) {
		PPID   obj_type = obj_type_list.get(i);
		if(!IS_DYN_OBJTYPE(obj_type))
			temp_list.Add(obj_type, GetObjectTitle(obj_type, obj_title));
	}
	temp_list.SortByText();
	for(i = 0; i < temp_list.getCount(); i++) {
		StrAssocArray::Item entry = temp_list.Get(i);
		rbyo_buf.Printf(rbyo_fmt_buf, entry.Id + OBJTYPE_OFFSET, entry.Txt);
		THROW_SL(rights_params.insert(newStr(rbyo_buf)));
	}
	SetupSCollectionComboBox(this, CTLSEL_EDITRHTS_EDITWHAT, &rights_params, (long)EditWhat);
	THROW(setupGrpUsrList());
	GrpUserID = getCurrID();
	loadData(GrpUserID, 1);
	CATCHZOK
	return ok;
}

int FastEditRightsDlg::getDTS(SecurCollection * pData)
{
	int    ok = -1;
	if(loadData(GrpUserID, 0) > 0) {
		CALLPTRMEMB(pData, copy(Data));
		ok = 1;
	}
	return ok;
}

int LoadGrpUsrRights(SecurCollection * pRights)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	PPObjSecur grp_obj(PPOBJ_USRGRP, 0);
	PPObjSecur usr_obj(PPOBJ_USR, 0);
	StrAssocArray * p_usr_list = 0;
	THROW(grp_obj.CheckRights(PPR_READ) && usr_obj.CheckRights(PPR_READ));
	THROW_MEM(p_usr_list = usr_obj.MakeStrAssocList(0));
	for(uint i = 0; i < p_usr_list->getCount(); i++) {
		PPSecurPacket * p_pack = new PPSecurPacket;
		PPID   id = p_usr_list->Get(i).Id;
		if(id & PPObjSecur::maskUserGroup) {
			THROW(p_ref->LoadSecur(PPOBJ_USRGRP, (id & ~PPObjSecur::maskUserGroup), p_pack));
		}
		else {
			THROW(p_ref->LoadSecur(PPOBJ_USR, id, p_pack));
			p_pack->Secur.ID += USERID_OFFSET;
		}
		pRights->insert(p_pack);
	}
	if(grp_obj.CheckRights(PPR_MOD) && usr_obj.CheckRights(PPR_MOD))
		ok = 1;
	CATCHZOK
	delete p_usr_list;
	return ok;
}

int SaveGrpUsrRights(SecurCollection * pRights)
{
	int    ok = -1;
	if(pRights) {
		Reference * p_ref = PPRef;
		PPSecurPacket * p_pack = 0;
		PPTransaction tra(1);
		THROW(tra);
		for(uint i = 0; pRights->enumItems(&i, (void **)&p_pack) > 0;) {
			p_pack->Secur.ID = (p_pack->Secur.Tag == PPOBJ_USR) ? (p_pack->Secur.ID - USERID_OFFSET) : p_pack->Secur.ID;
			THROW(p_ref->EditSecur(p_pack->Secur.Tag, p_pack->Secur.ID, p_pack, 0, 0));
		}
		THROW(tra.Commit());
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int FastEditRightsDialog()
{
	int    ok = -1;
	int    valid_data = 0, read_only = 0;
	SecurCollection grp_usr_rights;
	FastEditRightsDlg * p_dlg = 0;
	THROW(read_only = LoadGrpUsrRights(&grp_usr_rights));
	read_only = (read_only == -1);
	p_dlg = new FastEditRightsDlg(read_only);
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(&grp_usr_rights);
	while(!valid_data && ExecView(p_dlg) == cmOK) {
		if(!read_only) {
			if(p_dlg->getDTS(&grp_usr_rights) > 0) {
				THROW(SaveGrpUsrRights(&grp_usr_rights));
				ok = valid_data = 1;
			}
			else
				PPError();
		}
		else
			ok = valid_data = 1;
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}
