// SECURDLG.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2011, 2012, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
// @codepage UTF-8
// Диалоги редактирования пользователей, групп, прав доступа.
//
#include <pp.h>
#pragma hdrstop

int UpdatePassword()
{
	int    ok = 1;
	char   password[64];
	PPSecurPacket spack;
	PPAccessRestriction accsr;
	Reference * p_ref = PPRef;
	const PPID user_id = LConfig.UserID;
	THROW(p_ref->LoadSecur(PPOBJ_USR, user_id, &spack));
	if(spack.Rights.IsEmpty())
		ObjRts.GetAccessRestriction(accsr);
	else
		spack.Rights.GetAccessRestriction(accsr);
	if(PasswordDialog(0, password, sizeof(password), accsr.PwMinLen) > 0) {
		memcpy(spack.Secur.Password, password, sizeof(spack.Secur.Password));
		THROW(p_ref->EditSecur(PPOBJ_USR, user_id, &spack, 0, 1));
		PPMessage(mfInfo|mfOK, PPINF_PASSWORDUPDATED, 0);
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}

class SecurDialog : public TDialog {
	DECL_DIALOG_DATA(PPSecurPacket);
public:
	SecurDialog(int dlgID, PPID objType, PPID objID) : TDialog(dlgID), ObjType(objType), ObjID(objID)
	{
		PTR32(Password)[0] = 0; // @v10.3.0
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		int    ok = 1;
		ushort v  = 0;
		PPSecur * p_secur = &Data.Secur;
		setCtrlData(CTL_USRGRP_NAME, p_secur->Name);
		setCtrlData(CTL_USRGRP_ID, &ObjID);
		disableCtrl(CTL_USRGRP_ID, (!PPMaster || ObjID));
		if(ObjType == PPOBJ_USR) {
			disableCtrl(CTL_USRGRP_NAME, p_secur->ID == PPUSR_MASTER);
			setCtrlData(CTL_USR_SYMB, p_secur->Symb); // @v10.3.8
			memzero(Password, sizeof(Password));
			memcpy(Password, p_secur->Password, sizeof(p_secur->Password));
			SetupPPObjCombo(this, CTLSEL_USR_GRP,    PPOBJ_USRGRP, p_secur->ParentID, OLW_CANINSERT);
			SetupPPObjCombo(this, CTLSEL_USR_PERSON, PPOBJ_PERSON, p_secur->PersonID, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_EMPL));
			SetupPPObjCombo(this, CTLSEL_USR_UER,    PPOBJ_USREXCLRIGHTS, p_secur->UerID, 0, 0);
			enableCommand(cmCfgConfig, (p_secur->Flags & USRF_INHCFG) ? ((v |= 1), 0) : 1);
			enableCommand(cmCfgRights, (p_secur->Flags & USRF_INHRIGHTS) ? ((v |= 2), 0) : 1);
			setCtrlData(CTL_USR_FLAGS, &v);
			setCtrlData(CTL_USR_EXPIRY, &p_secur->ExpiryDate); // @v10.1.10
		}
		// @v10.3.8 {
		else if(ObjType == PPOBJ_USRGRP) {
			setCtrlData(CTL_USRGRP_SYMB, p_secur->Symb); 
		}
		// } @v10.3.8 
		else if(ObjType == PPOBJ_USREXCLRIGHTS) {
			AddClusterAssoc(CTL_USR_UERFLAGS, 0, PPEXCLRT_OBJDBDIVACCEPT);
			AddClusterAssoc(CTL_USR_UERFLAGS, 1, PPEXCLRT_CSESSWROFF);
			AddClusterAssoc(CTL_USR_UERFLAGS, 2, PPEXCLRT_CSESSWROFFROLLBACK);
			AddClusterAssoc(CTL_USR_UERFLAGS, 3, PPEXCLRT_INVWROFF);
			AddClusterAssoc(CTL_USR_UERFLAGS, 4, PPEXCLRT_INVWROFFROLLBACK);
			AddClusterAssoc(CTL_USR_UERFLAGS, 5, PPEXCLRT_DRAFTWROFF);
			AddClusterAssoc(CTL_USR_UERFLAGS, 6, PPEXCLRT_DRAFTWROFFROLLBACK);
			SetClusterData(CTL_USR_UERFLAGS, p_secur->UerFlags);
		}
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		ushort v;
		PPSecur * p_secur = &Data.Secur;
		getCtrlData(CTL_USRGRP_NAME, p_secur->Name);
		getCtrlData(CTL_USRGRP_ID,   &p_secur->ID);
		ObjID = p_secur->ID;
		if(ObjType == PPOBJ_USR) {
			getCtrlData(CTL_USR_SYMB, p_secur->Symb); // @v10.3.8
			memcpy(p_secur->Password, Password, sizeof(p_secur->Password));
			getCtrlData(CTLSEL_USR_GRP, &p_secur->ParentID);
			getCtrlData(CTLSEL_USR_PERSON, &p_secur->PersonID);
			getCtrlData(CTLSEL_USR_UER, &p_secur->UerID);
			getCtrlData(CTL_USR_FLAGS, &v);
			SETFLAG(p_secur->Flags, USRF_INHCFG,    v & 1);
			SETFLAG(p_secur->Flags, USRF_INHRIGHTS, v & 2);
			getCtrlData(CTL_USR_EXPIRY, &p_secur->ExpiryDate); // @v10.1.10
		}
		// @v10.3.8 {
		else if(ObjType == PPOBJ_USRGRP) {
			getCtrlData(CTL_USRGRP_SYMB, p_secur->Symb); 
		}
		// } @v10.3.8 
		else if(ObjType == PPOBJ_USREXCLRIGHTS) {
			GetClusterData(CTL_USR_UERFLAGS, &p_secur->UerFlags);
		}
		ASSIGN_PTR(pData, Data);
		return ok;
	}
	const  PPID   ObjType;
	PPID   ObjID;
private:
	DECL_HANDLE_EVENT;
	void   getPassword();
	void   getPaths();
	
	char   Password[sizeof(reinterpret_cast<const PPSecur *>(0)->Password)];
};

void SecurDialog::getPassword()
{
	PPAccessRestriction accsr;
	size_t minlen = Data.Rights.IsEmpty() ? ObjRts.GetAccessRestriction(accsr).PwMinLen : Data.Rights.GetAccessRestriction(accsr).PwMinLen;
	PasswordDialog(0, Password, sizeof(Password), minlen);
}

void SecurDialog::getPaths()
{
	TDialog * dlg = 0;
	if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_PATH)))) {
		static const struct {
			long   PathID;
			uint   FldId;
			uint   IndFldId;
		} path_fld_list[] = {
			{ PPPATH_DRIVE, CTL_PATH_DRV,  CTL_PATH_D_DRV },
			{ PPPATH_ROOT,  CTL_PATH_ROOT, CTL_PATH_D_ROOT },
			{ PPPATH_DAT,   CTL_PATH_DAT,  CTL_PATH_D_DAT },
			{ PPPATH_ARC,   CTL_PATH_ARC,  CTL_PATH_D_ARC },
			{ PPPATH_BIN,   CTL_PATH_BIN,  CTL_PATH_D_BIN },
			{ PPPATH_IN,    CTL_PATH_IN,   CTL_PATH_D_IN },
			{ PPPATH_OUT,   CTL_PATH_OUT,  CTL_PATH_D_OUT },
			{ PPPATH_TEMP,  CTL_PATH_TMP,  CTL_PATH_D_TMP }
		};
		uint    i;
		short  flags = 0;
		SString temp_buf;
		SString pattern;
		for(i = 0; i < SIZEOFARRAY(path_fld_list); i++) {
			//setPathFld(dlg, path_fld_list[i].PathID, path_fld_list[i].FldId, path_fld_list[i].IndFldId);
			//void SecurDialog::setPathFld(TDialog * dlg, long pathID, uint fldID, uint labelID)
			char   st[8];
			Data.Paths.GetPath(path_fld_list[i].PathID, &flags, temp_buf);
			dlg->setCtrlString(path_fld_list[i].FldId, temp_buf.Strip());
			st[0] = ' ';
			st[1] = (flags & PATHF_INHERITED) ? '.' : ' ';
			st[2] = 0;
			dlg->setStaticText(path_fld_list[i].IndFldId, st);
		}
		dlg->disableCtrl(CTL_PATH_DAT, 1); // @v10.7.8
		dlg->disableCtrl(CTL_PATH_ARC, 1); // @v10.7.8
		dlg->disableCtrl(CTL_PATH_BIN, 1); // @v10.7.8
		dlg->disableCtrl(CTL_PATH_TMP, 1); // @v10.7.8
		if(ExecView(dlg) == cmOK) {
			for(i = 0; i < SIZEOFARRAY(path_fld_list); i++) {
				//getPathFld(dlg, path_fld_list[i].PathID, path_fld_list[i].FldId);
				//void SecurDialog::getPathFld(TDialog * dlg, long pathID, uint fldID)
				Data.Paths.GetPath(path_fld_list[i].PathID, &flags, pattern);
				dlg->getCtrlString(path_fld_list[i].FldId, temp_buf);
				temp_buf.Strip();
				pattern.Strip();
				if(temp_buf.CmpNC(pattern) != 0)
					Data.Paths.SetPath(path_fld_list[i].PathID, temp_buf, 0, 1);
			}
		}
	}
	delete dlg;
}

IMPL_HANDLE_EVENT(SecurDialog)
{
	ushort v;
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmCfgPath:     getPaths();    break;
			case cmCfgConfig:   EditCfgOptionsDialog(&Data.Config, 0); break;
			case cmCfgRights:   EditRightsDialog(Data.Rights);         break;
			case cmCfgPassword: getPassword(); break;
			case cmClusterClk:
				if(event.isCtlEvent(CTL_USR_FLAGS)) {
					TVINFOVIEW->TransmitData(-1, &v);
					enableCommand(cmCfgConfig, !(v & 1));
					enableCommand(cmCfgRights, !(v & 2));
				}
				else
					return;
				break;
			default:
				return;
		}
		clearEvent(event);
	}
}

static int ValidateSecurData(TDialog * dlg, PPID objType, const void * pData)
{
	int    ok = 1;
	PPID   temp_id = 0;
	PPID   rec_id = reinterpret_cast<const PPSecur *>(pData)->ID;
	SString name_buf(reinterpret_cast<const PPSecur *>(pData)->Name);
	SString symb_buf(reinterpret_cast<const PPSecur *>(pData)->Symb); // @v10.9.12
	if(!name_buf.NotEmptyS())
		ok = PPErrorByDialog(dlg, CTL_USR_NAME, PPERR_SECURNAMENEEDED);
	else if(objType == PPOBJ_USR && reinterpret_cast<const PPSecur *>(pData)->ParentID == 0)
		ok = PPErrorByDialog(dlg, CTL_USR_GRP, PPERR_USRMUSTBELONGTOGRP);
	else if(PPRef->SearchName(objType, &temp_id, name_buf) > 0 && rec_id != temp_id)
		ok = PPErrorByDialog(dlg, CTL_USR_NAME, PPERR_DUPOBJNAME);
	else if(symb_buf.NotEmptyS() && PPRef->SearchSymb(objType, &temp_id, symb_buf, offsetof(PPSecur, Symb)) > 0 && rec_id != temp_id) { // @v10.9.12
		PPSetObjError(PPERR_DUPSYMB, objType, temp_id);
		ok = PPErrorByDialog(dlg, CTL_USR_SYMB, -1);
	}
	else {
		if(objType == PPOBJ_USR && reinterpret_cast<const PPSecur *>(pData)->ExpiryDate) {
			if(!checkdate(reinterpret_cast<const PPSecur *>(pData)->ExpiryDate)) {
				ok = PPErrorByDialog(dlg, CTL_USR_EXPIRY, PPERR_SLIB);
			}
		}
	}
	return ok;
}

int EditSecurDialog(PPID objType, PPID * pID, void * extraPtr)
{
	int    ok = 1, valid_data = 0, r;
	Reference * p_ref = PPRef;
	int    dlg_id = 0;
	PPID   _id = DEREFPTRORZ(pID);
	SecurDialog * dlg = 0;
	PPSecurPacket spack;
	PPSecurPacket sample_spack;
	PPObjSecur::ExtraParam param;
	if(extraPtr) {
		if(static_cast<const PPObjSecur::ExtraParam *>(extraPtr)->IsConsistent())
			param = *static_cast<const PPObjSecur::ExtraParam *>(extraPtr);
	}
	PPID   obj = objType;
	if(_id) {
		if(_id & PPObjSecur::maskUserGroup)
			obj = PPOBJ_USRGRP;
		else if(_id & PPObjSecur::maskConfig)
			obj = PPOBJ_CONFIG;
		else if(_id & PPObjSecur::maskUER)
			obj = PPOBJ_USREXCLRIGHTS;
		_id &= ~PPObjSecur::mask;
		THROW(p_ref->LoadSecur(obj, _id, &spack));
	}
	else {
		if(param.SampleID) {
			SString temp_buf;
			obj = param.Type;
			THROW(p_ref->LoadSecur(obj, param.SampleID, &sample_spack) > 0);
			spack = sample_spack;
			spack.Secur.ID = 0;
			spack.Secur.Symb[0] = 0;
			memzero(spack.Secur.Password, sizeof(spack.Secur.Password));
			temp_buf = spack.Secur.Name;
			long   _n = 1;
			PPID   temp_id = 0;
			PPSecur temp_rec;
			do {
				(temp_buf = spack.Secur.Name).Strip().Space().CatChar('#').Cat(++_n);
			} while(p_ref->SearchName(obj, &temp_id, temp_buf, &temp_rec) > 0);
			if(temp_buf.Len() < sizeof(spack.Secur.Name))
				STRNSCPY(spack.Secur.Name, temp_buf);
			else
				spack.Secur.Name[0] = 0;
		}
		else {
			if(param.Flags & PPObjSecur::ExtraParam::fSelectNewType) {
				uint   v = 0;
				obj = 0; // @v10.4.0
				if(SelectorDialog(DLG_SELNEWSEC, CTL_SELNEWSEC_SEL, &v) > 0) {
					switch(v) {
						case 0: obj = PPOBJ_USR; break;
						case 1: obj = PPOBJ_USRGRP; break;
						case 2: obj = PPOBJ_USREXCLRIGHTS; break;
					}
				}
			}
			if(obj) {
				PPID   parent_obj = 0;
				spack.Secur.Tag = obj;
				if(obj == PPOBJ_USRGRP) {
					parent_obj = PPOBJ_CONFIG;
					spack.Secur.ParentID = PPCFG_MAIN;
				}
				else if(obj == PPOBJ_USREXCLRIGHTS) {
					parent_obj = PPOBJ_CONFIG;
					spack.Secur.ParentID = PPCFG_MAIN;
				}
				else if(obj == PPOBJ_USR && param.ParentID) {
					parent_obj = PPOBJ_USRGRP;
					spack.Secur.ParentID = param.ParentID;
				}
				if(parent_obj && spack.Secur.ParentID) {
					THROW(DS.FetchConfig(parent_obj, spack.Secur.ParentID, &spack.Config));
					THROW(spack.Rights.Get(parent_obj, spack.Secur.ParentID, 0/*ignoreCheckSum*/));
					spack.Config.Tag = obj;
					spack.Config.ObjID = 0;
				}
			}
		}
	}
	switch(obj) {
		case PPOBJ_CONFIG: dlg_id = DLG_CONFIG; break;
		case PPOBJ_USRGRP: dlg_id = DLG_USRGRP; break;
		case PPOBJ_USR: dlg_id = DLG_USR; break;
		case PPOBJ_USREXCLRIGHTS: dlg_id = DLG_USRER; break;
	}
	if(dlg_id) {
		THROW(CheckDialogPtr(&(dlg = new SecurDialog(dlg_id, obj, _id))));
		dlg->setDTS(&spack);
		while(!valid_data && (r = ExecView(dlg)) == cmOK) {
			dlg->getDTS(&spack);
			if(ValidateSecurData(dlg, obj, &spack.Secur)) {
				THROW(p_ref->EditSecur(obj, dlg->ObjID, &spack, *pID == 0, 1));
				ASSIGN_PTR(pID, spack.Secur.ID);
				valid_data = 1;
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

static int CfgRoundDialog(PPConfig * pCfg)
{
	int    r;
	ushort v = 0;
	TDialog * dlg = new TDialog(DLG_CFGROUND);
	if(!CheckDialogPtrErr(&dlg))
		return 0;
	dlg->setCtrlData(CTL_CFGROUND_PREC, &pCfg->RoundPrec);
	long   rd = (pCfg->Flags & (CFGFLG_ROUNDUP | CFGFLG_ROUNDDOWN));
	if(rd == 0 || rd == (CFGFLG_ROUNDUP | CFGFLG_ROUNDDOWN))
		v = 0;
	else if(rd & CFGFLG_ROUNDUP)
		v = 1;
	else if(rd & CFGFLG_ROUNDDOWN)
		v = 2;
	dlg->setCtrlData(CTL_CFGROUND_DIR, &v);
	if((r = ExecView(dlg)) == cmOK) {
		dlg->getCtrlData(CTL_CFGROUND_PREC, &pCfg->RoundPrec);
		dlg->getCtrlData(CTL_CFGROUND_DIR, &v);
		pCfg->Flags &= ~(CFGFLG_ROUNDUP | CFGFLG_ROUNDDOWN);
		if(v == 1)
			pCfg->Flags |= CFGFLG_ROUNDUP;
		else if(v == 2)
			pCfg->Flags |= CFGFLG_ROUNDDOWN;
	}
	delete dlg;
	return r;
}

class CfgOptionsDialog : public TDialog {
	DECL_DIALOG_DATA(PPConfig);
public:
	explicit CfgOptionsDialog(PPID obj) : TDialog(obj == PPOBJ_CONFIG ? DLG_MAINCFG : DLG_CFGOPTIONS)
	{
	}
	virtual int TransmitData(int dir, void * pData)
	{
		int    s = 0;
		if(dir > 0)
			s = setDTS(static_cast<const PPConfig *>(pData));
		else if(dir < 0)
			s = getDTS(static_cast<PPConfig *>(pData));
		else
			s = TDialog::TransmitData(dir, pData);
		return s;
	}
	DECL_DIALOG_SETDTS()
	{
		if(pData) {
			Data = *pData;
			setCtrlData(CTL_CFGOPTIONS_ACCESS, &Data.AccessLevel);
			setCtrlData(CTL_CFGOPTIONS_MENU,   &Data.Menu);
			SetupPPObjCombo(this, CTLSEL_CONFIG_MAINORG, PPOBJ_PERSON, Data.MainOrg, 0, reinterpret_cast<void *>(PPPRK_MAIN));
			SetupPPObjCombo(this, CTLSEL_CFGOPTIONS_SHEET, PPOBJ_ACCSHEET, Data.LocAccSheetID, 0);
			SetupPPObjCombo(this, CTLSEL_CFGOPTIONS_LOC, PPOBJ_LOCATION, Data.Location, 0);
			SetupPPObjCombo(this, CTLSEL_CFGOPTIONS_DBDIV, PPOBJ_DBDIV, Data.DBDiv, 0);
			if(Id == DLG_CFGOPTIONS) {
				StrAssocArray list;
				long   desktop_surr_id = 0;
				long   menu_surr_id = 0;
				PPCommandFolder::CommandGroupList::Entry entry;
				{
					PPCommandFolder::GetCommandGroupList(0, cmdgrpcDesktop, DesktopList);
					if(!!Data.DesktopUuid) {
						uint idx = 0;
						if(DesktopList.SearchByUuid(Data.DesktopUuid, &idx)) {
							if(DesktopList.Get(idx, entry))
								desktop_surr_id = entry.SurrID;
						}
						else {
							LongArray idx_list;
							if(DesktopList.SearchByNativeID(Data.DesktopID_Obsolete, idx_list)) {
								assert(idx_list.getCount());
								if(idx_list.getCount() && DesktopList.Get(idx_list.get(0), entry))
									desktop_surr_id = entry.SurrID;
							}
						}
					}
					DesktopList.GetStrAssocList(list);
					SetupStrAssocCombo(this, CTLSEL_CFGOPTIONS_DESK, &list, /*Data.DesktopID*/desktop_surr_id, 0);
				}
				{
					PPCommandFolder::GetCommandGroupList(0, cmdgrpcMenu, MenuList);
					if(!!Data.MenuUuid) {
						uint idx = 0;
						if(MenuList.SearchByUuid(Data.MenuUuid, &idx)) {
							if(MenuList.Get(idx, entry))
								menu_surr_id = entry.SurrID;
						}
						else {
							LongArray idx_list;
							if(MenuList.SearchByNativeID(Data.MenuID_Obsolete, idx_list)) {
								assert(idx_list.getCount());
								if(idx_list.getCount() && MenuList.Get(idx_list.get(0), entry))
									menu_surr_id = entry.SurrID;
							}
						}						
					}
					MenuList.GetStrAssocList(list);
					SetupStrAssocCombo(this, CTLSEL_CFGOPTIONS_MENU2, &list, /*Data.MenuID*/menu_surr_id, 0);
				}
			}
			AddClusterAssoc(CTL_CFGOPTIONS_RLZORD, 0, RLZORD_FIFO);
			AddClusterAssoc(CTL_CFGOPTIONS_RLZORD, 1, RLZORD_LIFO);
			SetClusterData(CTL_CFGOPTIONS_RLZORD, Data.RealizeOrder);
			AddClusterAssoc(CTL_CFGOPTIONS_FEFO, 0, CFGFLG_FEFO);
			SetClusterData(CTL_CFGOPTIONS_FEFO, Data.Flags);
			AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  0, CFGFLG_CHECKTURNREST);
			AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  1, CFGFLG_DISCOUNTBYSUM);
			AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  2, CFGFLG_USEPACKAGE);
			AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  3, CFGFLG_SELGOODSBYPRICE);
			AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  4, CFGFLG_FREEPRICE);
			AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  5, CFGFLG_ALLOWOVERPAY);
			AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  6, CFGFLG_ENABLEFIXDIS);
			AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  7, CFGFLG_AUTOQUOT);
			AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  8, CFGFLG_SHOWPHQTTY);
			AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  9, CFGFLG_CONFGBROWRMV);
			AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS, 10, CFGFLG_USEGOODSMATRIX);
			if(Id == DLG_CFGOPTIONS) {
				AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS, 11, CFGFLG_DONTPROCESSDATAONJOBSRV);
				AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS, 12, CFGFLG_MULTICURBILL_DISABLE); // @v10.7.8
			}
			SetClusterData(CTL_CFGOPTIONS_OPTIONS, Data.Flags);

			AddClusterAssoc(CTL_CFGOPTIONS_STAFF, 0, CFGFLG_STAFFMGMT);
			SetClusterData(CTL_CFGOPTIONS_STAFF, Data.Flags);

			AddClusterAssoc(CTL_CFGOPTIONS_SECFLAGS, 0, CFGFLG_SEC_CASESENSPASSW);
			AddClusterAssoc(CTL_CFGOPTIONS_SECFLAGS, 1, CFGFLG_SEC_DSBLMULTLOGIN);
			AddClusterAssoc(CTL_CFGOPTIONS_SECFLAGS, 2, CFGFLG_SEC_DSBLNSYSDATE);
			SetClusterData(CTL_CFGOPTIONS_SECFLAGS, Data.Flags);
		}
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTL_CFGOPTIONS_ACCESS,   &Data.AccessLevel);
		getCtrlData(CTL_CFGOPTIONS_MENU,     &Data.Menu);
		getCtrlData(CTLSEL_CONFIG_MAINORG,   &Data.MainOrg);
		getCtrlData(CTLSEL_CFGOPTIONS_SHEET, &Data.LocAccSheetID);
		getCtrlData(CTLSEL_CFGOPTIONS_LOC,   &Data.Location);
		getCtrlData(CTLSEL_CFGOPTIONS_DBDIV, &Data.DBDiv);
		if(Id == DLG_CFGOPTIONS) {
			PPCommandFolder::CommandGroupList::Entry entry;
			{
				long   surr_id = 0;
				uint   idx = 0;
				getCtrlData(CTLSEL_CFGOPTIONS_DESK,  /*&Data.DesktopID*/&surr_id);
				if(DesktopList.SearchBySurrID(surr_id, &idx) && DesktopList.Get(idx, entry)) {
					Data.DesktopUuid = entry.Uuid;
					Data.DesktopID_Obsolete = entry.NativeID;
					DS.GetTLA().Lc.DesktopUuid = entry.Uuid;
					DS.GetTLA().Lc.DesktopID_Obsolete = entry.NativeID;
				}
				//DS.GetTLA().Lc.DesktopID = Data.DesktopID;
			}
			{
				long   surr_id = 0;
				uint   idx = 0;
				getCtrlData(CTLSEL_CFGOPTIONS_MENU2, /*&Data.MenuID*/&surr_id);
				if(MenuList.SearchBySurrID(surr_id, &idx) && MenuList.Get(idx, entry)) {
					Data.MenuUuid = entry.Uuid;
					Data.MenuID_Obsolete = entry.NativeID;
				}
			}
		}
		Data.RealizeOrder = static_cast<short>(GetClusterData(CTL_CFGOPTIONS_RLZORD));
		GetClusterData(CTL_CFGOPTIONS_FEFO, &Data.Flags);
		GetClusterData(CTL_CFGOPTIONS_OPTIONS, &Data.Flags);
		GetClusterData(CTL_CFGOPTIONS_STAFF,   &Data.Flags);
		GetClusterData(CTL_CFGOPTIONS_SECFLAGS, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmCfgRound))
			CfgRoundDialog(&Data);
		else if(event.isCmd(cmCfgCurrency))
			editCurConfig();
		else
			return;
		clearEvent(event);
	}
	void editCurConfig()
	{
		TDialog * dlg = 0;
		if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_CURCFG)))) {
			SetupPPObjCombo(dlg, CTLSEL_CURCFG_BASECUR, PPOBJ_CURRENCY, Data.BaseCurID, OLW_CANINSERT, 0);
			SetupPPObjCombo(dlg, CTLSEL_CURCFG_RATETYPE, PPOBJ_CURRATETYPE, Data.BaseRateTypeID, OLW_CANINSERT, 0);
			dlg->AddClusterAssoc(CTL_CURCFG_FLAGS, 0, CFGFLG_MULTICURACCT);
			dlg->AddClusterAssoc(CTL_CURCFG_FLAGS, 1, CFGFLG_MULTICURBILL_DISABLE); // @v10.7.8
			dlg->SetClusterData(CTL_CURCFG_FLAGS, Data.Flags);
			for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
				dlg->getCtrlData(CTLSEL_CURCFG_BASECUR,  &Data.BaseCurID);
				dlg->getCtrlData(CTLSEL_CURCFG_RATETYPE, &Data.BaseRateTypeID);
				dlg->GetClusterData(CTL_CURCFG_FLAGS, &Data.Flags);
				valid_data = 1;
			}
		}
		delete dlg;
	}
	PPCommandFolder::CommandGroupList MenuList;
	PPCommandFolder::CommandGroupList DesktopList;
};

int EditCfgOptionsDialog(PPConfig * pCfg, long, EmbedDialog * pDlg)
{
	int    r = 0, valid_data = 0;
	CfgOptionsDialog * dlg = 0;
	if(pDlg) {
		dlg = new CfgOptionsDialog(0);
		pDlg->Embed(dlg);
		r = cmOK;
	}
	else {
		PPConfig cfg = *pCfg;
		dlg = new CfgOptionsDialog(cfg.Tag);
		THROW(CheckDialogPtr(&dlg));
		dlg->setDTS(&cfg);
		while(!valid_data && (r = ExecView(dlg)) == cmOK) {
			dlg->getDTS(&cfg);
			valid_data = 1;
			*pCfg = cfg;
		}
	}
	CATCH
		r = PPErrorZ();
	ENDCATCH
	if(!pDlg)
		delete dlg;
	return r;
}
//
//
//
#define ACTIVEUSERLIST_SHOWMACHINE 0x00000001L

class ActiveUserListDlg : public PPListDialog {
public:
	ActiveUserListDlg() : PPListDialog(DLG_AUSERSLST, CTL_AUSERSLST_LIST), Options(0)
	{
		AddClusterAssoc(CTL_AUSERSLST_FLAGS, 0, ACTIVEUSERLIST_SHOWMACHINE);
		SetClusterData(CTL_AUSERSLST_FLAGS, 0);
		updateList(-1);
	}
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	int  GetDtm(PPID userID, PPID sessID, LDATETIME * pLoginDtm, SString & rWorkDtm);

	long Options;
	PPSyncArray SyncAry;
};

IMPL_HANDLE_EVENT(ActiveUserListDlg)
{
	PPListDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(TVCMD == cmClusterClk) {
			GetClusterData(CTL_AUSERSLST_FLAGS, &Options);
			updateList(-1);
		}
		else if(TVCMD == cmUpdate)
			updateList(-1);
		else if(TVCMD == cmSysJournalByObj) {
			long pos = 0, user_id = 0, sess_id;
			getSelection(&pos);
			if(pos >= 0 && pos < static_cast<long>(SyncAry.getCount())) {
				sess_id = SyncAry.at(pos).ID;
				user_id = SyncAry.at(pos).UserID;
			}
			if(user_id > 0) {
				SString buf;
				LDATETIME login_dtm;
				SysJournalFilt sj_filt;
				if(GetDtm(user_id, sess_id, &login_dtm, buf) > 0) {
					sj_filt.BegTm      = login_dtm.t;
					sj_filt.Period.low = login_dtm.d;
				}
				else {
					sj_filt.BegTm      = ZEROTIME;
					sj_filt.Period.low = getcurdate_();
				}
				sj_filt.UserID     = user_id;
				ViewSysJournal(&sj_filt, 0);
			}
		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

int ActiveUserListDlg::GetDtm(PPID userID, PPID sessID, LDATETIME * pLoginDtm, SString & rWorkDtm)
{
	int    ok = -1;
	LDATETIME login_dtm = ZERODATETIME;
	rWorkDtm.Z();
	if(DS.CheckExtFlag(ECF_USESJLOGINEVENT)) {
		long   sec = 0, dd = 0;
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		if(p_sj) {
			const LDATETIME cur_dtm = getcurdatetime_();
			DateRange srch_prd;
			srch_prd.Z();
			plusperiod(&(srch_prd.low = cur_dtm.d), PRD_DAY, -2, 0);
			if((ok = p_sj->GetLastUserEvent(PPACN_LOGIN, userID, sessID, &srch_prd, &login_dtm)) > 0)
				sec = diffdatetime(cur_dtm, login_dtm, 3, &dd);
		}
		{
			int    h = 0, m = 0, s = 0;
			SString buf;
			// @v9.8.12 PPGetWord(PPWORD_DAYS, 0, buf);
			PPLoadString("days", buf); // @v9.8.12
			h = sec / 3600;
			sec %= 3600;
			m = sec / 60;
			s = sec % 60;
			rWorkDtm.Printf("%ld %s %02d:%02d:%02d", dd, buf.cptr(), h, m, s);
		}
	}
	ASSIGN_PTR(pLoginDtm, login_dtm);
	return ok;
}

IMPL_CMPFUNC(PPSyncItem, i1, i2)
{
	const PPSyncItem * p_i1 = static_cast<const PPSyncItem *>(i1);
	const PPSyncItem * p_i2 = static_cast<const PPSyncItem *>(i2);
	return p_i1->MchnID.Cmp(p_i2->MchnID);
}

int ActiveUserListDlg::setupList()
{
	long   count = 0, uniq_macs = 0;
	SyncAry.freeAll();
	DS.GetSync().GetItemsList(PPSYNC_DBLOCK, &SyncAry);
	count = SyncAry.getCount();
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_AUSERSLST_LIST));
	if(p_list) {
		SString host, buf, work_dtm_buf;
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; i < SyncAry.getCount(); i++) {
			uint   pos = i + 1;
			LDATETIME login_dtm;
			PPSyncItem * p_item = &SyncAry.at(i);
			ss.clear();
			ss.add(p_item->Name);
			ss.add(buf.Z().Cat(p_item->ID));
			ss.add(buf.Z().Cat(p_item->ObjID));
			ss.add(p_item->MchnID.ToStr(buf.Z()));
			if(Options & ACTIVEUSERLIST_SHOWMACHINE){
				InetAddr addr;
				GetFirstHostByMACAddr(&p_item->MchnID, &addr);
				addr.ToStr(InetAddr::fmtHost, host);
			}
			ss.add(host);
			GetDtm(p_item->UserID, p_item->ID, &login_dtm, work_dtm_buf.Z());
			ss.add(buf.Z().Cat(login_dtm));
			ss.add(work_dtm_buf);
			p_list->addItem(i, ss.getBuf());
			if(SyncAry.lsearch(p_item, &pos, PTR_CMPFUNC(PPSyncItem)) <= 0)
				uniq_macs++;
		}
	}
	setCtrlLong(CTL_AUSERSLST_USERSCOUNT, count);
	setCtrlLong(CTL_AUSERSLST_UNIQMACS,   uniq_macs);
	return 1;
}

int ActiveUsersListDialog()
{
	int    ok = -1;
	ActiveUserListDlg * dlg = new ActiveUserListDlg();
	if(CheckDialogPtrErr(&dlg))
		ExecViewAndDestroy(dlg);
	else
		ok = 0;
	return ok;
}
