// SECURDLG.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2011, 2012, 2014, 2015, 2016
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

int SLAPI UpdatePassword()
{
	int    ok = 1;
	char   password[64];
	PPSecurPacket spack;
	PPAccessRestriction accsr;
	THROW(PPRef->LoadSecur(PPOBJ_USR, LConfig.User, &spack));
	if(spack.Rights.IsEmpty())
		ObjRts.GetAccessRestriction(accsr);
	else
		spack.Rights.GetAccessRestriction(accsr);
	if(PasswordDialog(0, password, sizeof(password), accsr.PwMinLen) > 0) {
		memcpy(spack.Secur.Password, password, sizeof(spack.Secur.Password));
		THROW(PPRef->EditSecur(PPOBJ_USR, LConfig.User, &spack, 0));
		PPMessage(mfInfo|mfOK, PPINF_PASSWORDUPDATED, 0);
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}

class SecurDialog : public TDialog {
public:
	SecurDialog(int dlgID, PPID objType, PPID objID) : TDialog(dlgID)
	{
		ObjType = objType;
		ObjID   = objID;
	}
	int    setDTS(const PPSecurPacket * pData)
	{
		Pack = *pData;

		int    ok = 1;
		ushort v  = 0;
		PPSecur * secur = &Pack.Secur;
		setCtrlData(CTL_USRGRP_NAME, secur->Name);
		setCtrlData(CTL_USRGRP_ID, &ObjID);
		disableCtrl(CTL_USRGRP_ID, (!PPMaster || ObjID));
		if(ObjType == PPOBJ_USR) {
			disableCtrl(CTL_USRGRP_NAME, secur->ID == PPUSR_MASTER);
			memzero(Password, sizeof(Password));
			memcpy(Password, secur->Password, sizeof(secur->Password));
			SetupPPObjCombo(this, CTLSEL_USR_GRP,    PPOBJ_USRGRP, secur->ParentID, OLW_CANINSERT);
			SetupPPObjCombo(this, CTLSEL_USR_PERSON, PPOBJ_PERSON, secur->PersonID, OLW_CANINSERT, (void *)PPPRK_EMPL);
			SetupPPObjCombo(this, CTLSEL_USR_UER,    PPOBJ_USREXCLRIGHTS, secur->UerID, 0, 0); // @v8.6.0
			enableCommand(cmCfgConfig, (secur->Flags & USRF_INHCFG) ? ((v |= 1), 0) : 1);
			enableCommand(cmCfgRights, (secur->Flags & USRF_INHRIGHTS) ? ((v |= 2), 0) : 1);
			setCtrlData(CTL_USR_FLAGS, &v);
		}
		else if(ObjType == PPOBJ_USREXCLRIGHTS) {
			AddClusterAssoc(CTL_USR_UERFLAGS, 0, PPEXCLRT_OBJDBDIVACCEPT);
			AddClusterAssoc(CTL_USR_UERFLAGS, 1, PPEXCLRT_CSESSWROFF);
			AddClusterAssoc(CTL_USR_UERFLAGS, 2, PPEXCLRT_CSESSWROFFROLLBACK);
			AddClusterAssoc(CTL_USR_UERFLAGS, 3, PPEXCLRT_INVWROFF);
			AddClusterAssoc(CTL_USR_UERFLAGS, 4, PPEXCLRT_INVWROFFROLLBACK);
			AddClusterAssoc(CTL_USR_UERFLAGS, 5, PPEXCLRT_DRAFTWROFF); // @v8.6.4
			AddClusterAssoc(CTL_USR_UERFLAGS, 6, PPEXCLRT_DRAFTWROFFROLLBACK); // @v8.6.4
			SetClusterData(CTL_USR_UERFLAGS, secur->UerFlags);
		}
		return ok;
	}
	int    getDTS(PPSecurPacket * pData)
	{
		int    ok = 1;
		ushort v;
		PPSecur * secur = &Pack.Secur;
		getCtrlData(CTL_USRGRP_NAME, secur->Name);
		getCtrlData(CTL_USRGRP_ID,   &secur->ID);
		ObjID = secur->ID;
		if(ObjType == PPOBJ_USR) {
			memcpy(secur->Password, Password, sizeof(secur->Password));
			getCtrlData(CTLSEL_USR_GRP, &secur->ParentID);
			getCtrlData(CTLSEL_USR_PERSON, &secur->PersonID);
			getCtrlData(CTLSEL_USR_UER, &secur->UerID); // @v8.6.0
			getCtrlData(CTL_USR_FLAGS, &v);
			SETFLAG(secur->Flags, USRF_INHCFG,    v & 1);
			SETFLAG(secur->Flags, USRF_INHRIGHTS, v & 2);
		}
		else if(ObjType == PPOBJ_USREXCLRIGHTS) {
			GetClusterData(CTL_USR_UERFLAGS, &secur->UerFlags);
		}
		*pData = Pack;
		return ok;
	}
	PPID   ObjType;
	PPID   ObjID;
private:
	DECL_HANDLE_EVENT;
	void   getPassword();
	int    getPaths();
	int    setPathFld(TDialog *, long pathID, uint ctlID, uint labelID);
	int    getPathFld(TDialog *, long pathID, uint ctlID);

	PPSecurPacket Pack;
	char   Password[sizeof(((PPSecur*)0)->Password)];
};

void SecurDialog::getPassword()
{
	PPAccessRestriction accsr;
	size_t minlen = Pack.Rights.IsEmpty() ?
		ObjRts.GetAccessRestriction(accsr).PwMinLen : Pack.Rights.GetAccessRestriction(accsr).PwMinLen;
	PasswordDialog(0, Password, sizeof(Password), minlen);
}

int SecurDialog::setPathFld(TDialog * dlg, long pathID, uint fldID, uint labelID)
{
	short  flags;
	char   buf[128], st[8];
	Pack.Paths.GetPath(pathID, &flags, buf, sizeof(buf));
	dlg->setCtrlData(fldID, strip(buf));
	st[0] = ' ';
	st[1] = (flags & PATHF_INHERITED) ? '.' : ' ';
	st[2] = 0;
	dlg->setStaticText(labelID, st);
	return 1;
}

int SecurDialog::getPathFld(TDialog * dlg, long pathID, uint fldID)
{
	short  flags;
	char   buf[128], pattern[128];
	Pack.Paths.GetPath(pathID, &flags, pattern, sizeof(pattern));
	buf[0] = 0;
	dlg->getCtrlData(fldID, buf);
	if(stricmp(strip(buf), strip(pattern)))
		if(!Pack.Paths.SetPath(pathID, buf, 0, 1))
			return 0;
	return 1;
}

int SecurDialog::getPaths()
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(CheckDialogPtr(&(dlg = new TDialog(DLG_PATH)), 1)) {
		setPathFld(dlg, PPPATH_DRIVE, CTL_PATH_DRV,  CTL_PATH_D_DRV);
		setPathFld(dlg, PPPATH_ROOT,  CTL_PATH_ROOT, CTL_PATH_D_ROOT);
		setPathFld(dlg, PPPATH_DAT,   CTL_PATH_DAT,  CTL_PATH_D_DAT);
		setPathFld(dlg, PPPATH_ARC,   CTL_PATH_ARC,  CTL_PATH_D_ARC);
		setPathFld(dlg, PPPATH_BIN,   CTL_PATH_BIN,  CTL_PATH_D_BIN);
		setPathFld(dlg, PPPATH_IN,    CTL_PATH_IN,   CTL_PATH_D_IN);
		setPathFld(dlg, PPPATH_OUT,   CTL_PATH_OUT,  CTL_PATH_D_OUT);
		setPathFld(dlg, PPPATH_TEMP,  CTL_PATH_TMP,  CTL_PATH_D_TMP);
		if(ExecView(dlg) == cmOK) {
			getPathFld(dlg, PPPATH_DRIVE, CTL_PATH_DRV);
			getPathFld(dlg, PPPATH_ROOT,  CTL_PATH_ROOT);
			getPathFld(dlg, PPPATH_DAT,   CTL_PATH_DAT);
			getPathFld(dlg, PPPATH_ARC,   CTL_PATH_ARC);
			getPathFld(dlg, PPPATH_BIN,   CTL_PATH_BIN);
			getPathFld(dlg, PPPATH_IN,    CTL_PATH_IN);
			getPathFld(dlg, PPPATH_OUT,   CTL_PATH_OUT);
			getPathFld(dlg, PPPATH_TEMP,  CTL_PATH_TMP);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

IMPL_HANDLE_EVENT(SecurDialog)
{
	ushort v;
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmCfgPath:     getPaths();    break;
			case cmCfgConfig:   EditCfgOptionsDialog(&Pack.Config, 0); break;
			case cmCfgRights:   EditRightsDialog(Pack.Rights);         break;
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

static int SLAPI ValidateSecurData(TDialog * dlg, PPID objType, void * pData)
{
	int    ok = 1;
	PPID   temp_id = 0, rec_id = ((PPSecur*)pData)->ID;
	const  char * p_name = strip(((PPSecur*)pData)->Name);
	if(p_name[0] == 0)
		ok = PPErrorByDialog(dlg, CTL_USR_NAME, PPERR_SECURNAMENEEDED);
	else if(objType == PPOBJ_USR && ((PPSecur*)pData)->ParentID == 0)
		ok = PPErrorByDialog(dlg, CTL_USR_GRP, PPERR_USRMUSTBELONGTOGRP);
	else if(PPRef->SearchName(objType, &temp_id, p_name) > 0 && rec_id != temp_id)
		ok = PPErrorByDialog(dlg, CTL_USR_NAME, PPERR_DUPOBJNAME);
	return ok;
}

int SLAPI EditSecurDialog(PPID objType, PPID * pID, void * extraPtr)
{
	int    ok = 1, valid_data = 0, r;
	int    dlg_id = 0;
	PPID   _id = pID ? *pID : 0;
	SecurDialog * dlg = 0;
	PPSecurPacket spack;
	PPObjSecur::ExtraParam param;
	if(extraPtr) {
		if(((PPObjSecur::ExtraParam *)extraPtr)->IsConsistent())
			param = *(PPObjSecur::ExtraParam *)extraPtr;
		/*else
			param.ParentID = (PPID)extraPtr;*/
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
		THROW(PPRef->LoadSecur(obj, _id, &spack));
	}
	else {
		PPID   parent_obj = 0;
		if(param.Flags & PPObjSecur::ExtraParam::fSelectNewType) {
			uint   v = 0;
			if(SelectorDialog(DLG_SELNEWSEC, CTL_SELNEWSEC_SEL, &v) > 0) {
                if(v == 0) {
					obj = PPOBJ_USR;
                }
                else if(v == 1) {
                	obj = PPOBJ_USRGRP;
                }
                else if(v == 2) {
                	obj = PPOBJ_USREXCLRIGHTS;
                }
			}
		}
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
			THROW(spack.Rights.Get(parent_obj, spack.Secur.ParentID));
			spack.Config.Tag = obj;
			spack.Config.ObjID = 0;
		}
	}
	if(obj == PPOBJ_CONFIG)
		dlg_id = DLG_CONFIG;
	else if(obj == PPOBJ_USRGRP)
		dlg_id = DLG_USRGRP;
	else if(obj == PPOBJ_USR)
		dlg_id = DLG_USR;
	else if(obj == PPOBJ_USREXCLRIGHTS)
		dlg_id = DLG_USRER;
	if(dlg_id) {
		THROW(CheckDialogPtr(&(dlg = new SecurDialog(dlg_id, obj, _id))));
		dlg->setDTS(&spack);
		while(!valid_data && (r = ExecView(dlg)) == cmOK) {
			dlg->getDTS(&spack);
			if(ValidateSecurData(dlg, obj, &spack.Secur)) {
				THROW(PPRef->EditSecur(obj, dlg->ObjID, &spack, *pID == 0));
				*pID = dlg->ObjID;
				valid_data = 1;
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

static int SLAPI CfgRoundDialog(PPConfig * pCfg)
{
	int    r;
	ushort v = 0;
	TDialog * dlg = new TDialog(DLG_CFGROUND);
	if(!CheckDialogPtr(&dlg, 1))
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
public:
	CfgOptionsDialog(PPID obj) : TDialog(obj == PPOBJ_CONFIG ? DLG_MAINCFG : DLG_CFGOPTIONS)
	{
	}
	virtual int TransmitData(int dir, void * pData)
	{
		int    s = 0;
		if(dir > 0)
			s = setDTS((PPConfig *)pData);
		else if(dir < 0)
			s = getDTS((PPConfig *)pData);
		else
			s = TDialog::TransmitData(dir, pData);
		return s;
	}
	int    setDTS(const PPConfig *);
	int    getDTS(PPConfig *);
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmCfgRound))
			CfgRoundDialog(&Cfg);
		else if(event.isCmd(cmCfgCurrency))
			editCurConfig();
		else
			return;
		clearEvent(event);
	}
	void   editCurConfig();

	PPConfig Cfg;
};

void CfgOptionsDialog::editCurConfig()
{
	TDialog * dlg = 0;
	if(CheckDialogPtr(&(dlg = new TDialog(DLG_CURCFG)), 1)) {
		SetupPPObjCombo(dlg, CTLSEL_CURCFG_BASECUR, PPOBJ_CURRENCY, Cfg.BaseCurID, OLW_CANINSERT, 0);
		SetupPPObjCombo(dlg, CTLSEL_CURCFG_RATETYPE, PPOBJ_CURRATETYPE, Cfg.BaseRateTypeID, OLW_CANINSERT, 0);
		dlg->AddClusterAssoc(CTL_CURCFG_FLAGS, 0, CFGFLG_MULTICURACCT);
		dlg->SetClusterData(CTL_CURCFG_FLAGS, Cfg.Flags);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			dlg->getCtrlData(CTLSEL_CURCFG_BASECUR,  &Cfg.BaseCurID);
			dlg->getCtrlData(CTLSEL_CURCFG_RATETYPE, &Cfg.BaseRateTypeID);
			dlg->GetClusterData(CTL_CURCFG_FLAGS, &Cfg.Flags);
			valid_data = 1;
		}
	}
	delete dlg;
}

int CfgOptionsDialog::setDTS(const PPConfig * pCfg)
{
	if(pCfg) {
		Cfg = *pCfg;
		setCtrlData(CTL_CFGOPTIONS_ACCESS, &Cfg.AccessLevel);
		setCtrlData(CTL_CFGOPTIONS_MENU,   &Cfg.Menu);
		SetupPPObjCombo(this, CTLSEL_CONFIG_MAINORG, PPOBJ_PERSON, Cfg.MainOrg, 0, (void *)PPPRK_MAIN);
		SetupPPObjCombo(this, CTLSEL_CFGOPTIONS_SHEET, PPOBJ_ACCSHEET, Cfg.LocSheet, 0);
		SetupPPObjCombo(this, CTLSEL_CFGOPTIONS_LOC, PPOBJ_LOCATION, Cfg.Location, 0);
		SetupPPObjCombo(this, CTLSEL_CFGOPTIONS_DBDIV, PPOBJ_DBDIV, Cfg.DBDiv, 0);
		if(Id == DLG_CFGOPTIONS) {
			StrAssocArray list;
			PPCommandFolder::GetMenuList(0, &list, 1);
			SetupStrAssocCombo(this, CTLSEL_CFGOPTIONS_DESK, &list, Cfg.DesktopID, 0);
			PPCommandFolder::GetMenuList(0, &list.Clear(), 0);
			SetupStrAssocCombo(this, CTLSEL_CFGOPTIONS_MENU2, &list, Cfg.MenuID, 0);
		}
		AddClusterAssoc(CTL_CFGOPTIONS_RLZORD, 0, RLZORD_FIFO);
		AddClusterAssoc(CTL_CFGOPTIONS_RLZORD, 1, RLZORD_LIFO);
		SetClusterData(CTL_CFGOPTIONS_RLZORD, Cfg.RealizeOrder);
		AddClusterAssoc(CTL_CFGOPTIONS_FEFO, 0, CFGFLG_FEFO);
		SetClusterData(CTL_CFGOPTIONS_FEFO, Cfg.Flags);
		//_SetFlags(this, Cfg.Flags);
		//static void SLAPI _SetFlags(TDialog * dlg, long flags)
		AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  0, CFGFLG_CHECKTURNREST);
		AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  1, CFGFLG_DISCOUNTBYSUM);
		AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  2, CFGFLG_USEPACKAGE);
		AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  3, CFGFLG_SELGOODSBYPRICE);
		AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  4, CFGFLG_FREEPRICE);
		// @v8.6.6 AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  5, CFGFLG_NEJPBILL);
		AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  5, CFGFLG_ALLOWOVERPAY);
		AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  6, CFGFLG_ENABLEFIXDIS);
		AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  7, CFGFLG_AUTOQUOT);
		AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  8, CFGFLG_SHOWPHQTTY);
		AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS,  9, CFGFLG_CONFGBROWRMV);
		AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS, 10, CFGFLG_USEGOODSMATRIX);
		AddClusterAssoc(CTL_CFGOPTIONS_OPTIONS, 11, CFGFLG_DONTPROCESSDATAONJOBSRV);
		SetClusterData(CTL_CFGOPTIONS_OPTIONS, Cfg.Flags);

		AddClusterAssoc(CTL_CFGOPTIONS_STAFF, 0, CFGFLG_STAFFMGMT);
		SetClusterData(CTL_CFGOPTIONS_STAFF, Cfg.Flags);

		AddClusterAssoc(CTL_CFGOPTIONS_SECFLAGS, 0, CFGFLG_SEC_CASESENSPASSW);
		AddClusterAssoc(CTL_CFGOPTIONS_SECFLAGS, 1, CFGFLG_SEC_DSBLMULTLOGIN);
		AddClusterAssoc(CTL_CFGOPTIONS_SECFLAGS, 2, CFGFLG_SEC_DSBLNSYSDATE);
		SetClusterData(CTL_CFGOPTIONS_SECFLAGS, Cfg.Flags);
	}
	return 1;
}

int CfgOptionsDialog::getDTS(PPConfig * pCfg)
{
	int    ok = -1;
	if(pCfg) {
		getCtrlData(CTL_CFGOPTIONS_ACCESS,   &Cfg.AccessLevel);
		getCtrlData(CTL_CFGOPTIONS_MENU,     &Cfg.Menu);
		getCtrlData(CTLSEL_CONFIG_MAINORG,   &Cfg.MainOrg);
		getCtrlData(CTLSEL_CFGOPTIONS_SHEET, &Cfg.LocSheet);
		getCtrlData(CTLSEL_CFGOPTIONS_LOC,   &Cfg.Location);
		getCtrlData(CTLSEL_CFGOPTIONS_DBDIV, &Cfg.DBDiv);
		if(Id == DLG_CFGOPTIONS) {
			getCtrlData(CTLSEL_CFGOPTIONS_DESK,  &Cfg.DesktopID);
			getCtrlData(CTLSEL_CFGOPTIONS_MENU2, &Cfg.MenuID);
			DS.GetTLA().Lc.DesktopID = Cfg.DesktopID;
		}
		Cfg.RealizeOrder = (short)GetClusterData(CTL_CFGOPTIONS_RLZORD);
		GetClusterData(CTL_CFGOPTIONS_FEFO, &Cfg.Flags);
		GetClusterData(CTL_CFGOPTIONS_OPTIONS, &Cfg.Flags);
		GetClusterData(CTL_CFGOPTIONS_STAFF,   &Cfg.Flags);
		GetClusterData(CTL_CFGOPTIONS_SECFLAGS, &Cfg.Flags);
		ok = 1;
	}
	ASSIGN_PTR(pCfg, Cfg);
	return ok;
}

int SLAPI EditCfgOptionsDialog(PPConfig * pCfg, long, EmbedDialog * pDlg)
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
	ActiveUserListDlg() : PPListDialog(DLG_AUSERSLST, CTL_AUSERSLST_LIST)
	{
		AddClusterAssoc(CTL_AUSERSLST_FLAGS, 0, ACTIVEUSERLIST_SHOWMACHINE);
		SetClusterData(CTL_AUSERSLST_FLAGS, 0);
		Options = 0;
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
			if(pos >= 0 && pos < (long)SyncAry.getCount()) {
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
	LDATETIME login_dtm;
	login_dtm.SetZero();
	rWorkDtm = 0;
	if(DS.CheckExtFlag(ECF_USESJLOGINEVENT)) {
		long   sec = 0, dd = 0;
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		if(p_sj) {
			LDATETIME cur_dtm;
			DateRange srch_prd;
			srch_prd.SetZero();
			getcurdatetime(&cur_dtm);
			plusperiod(&(srch_prd.low = cur_dtm.d), PRD_DAY, -2, 0);
			if((ok = p_sj->GetLastUserEvent(PPACN_LOGIN, userID, sessID, &srch_prd, &login_dtm)) > 0)
				sec = diffdatetime(cur_dtm, login_dtm, 3, &dd);
		}
		{
			int    h = 0, m = 0, s = 0;
			SString buf;
			PPGetWord(PPWORD_DAYS, 0, buf);
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
	PPSyncItem * p_i1 = (PPSyncItem*)i1;
	PPSyncItem * p_i2 = (PPSyncItem*)i2;
	return p_i1->MchnID.Cmp(p_i2->MchnID);
}

int ActiveUserListDlg::setupList()
{
	long   count = 0, uniq_macs = 0;
	SmartListBox * p_list = 0;
	SyncAry.freeAll();
	DS.GetSync().GetItemsList(PPSYNC_DBLOCK, &SyncAry);
	count = SyncAry.getCount();
	if(p_list = (SmartListBox*)getCtrlView(CTL_AUSERSLST_LIST)) {
		SString host, buf, work_dtm_buf;
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; i < SyncAry.getCount(); i++) {
			uint   pos = i + 1;
			LDATETIME login_dtm;
			PPSyncItem * p_item = &SyncAry.at(i);
			ss.clear(1);
			ss.add(p_item->Name, 0);
			ss.add((buf = 0).Cat(p_item->ID), 0);
			ss.add((buf = 0).Cat(p_item->ObjID), 0);
			ss.add(p_item->MchnID.ToStr(buf = 0), 0);
			if(Options & ACTIVEUSERLIST_SHOWMACHINE){
				InetAddr addr;
				GetFirstHostByMACAddr(&p_item->MchnID, &addr);
				addr.ToStr(InetAddr::fmtHost, host);
			}
			ss.add((const char*)host, 0);
			GetDtm(p_item->UserID, p_item->ID, &login_dtm, work_dtm_buf = 0);
			ss.add((buf = 0).Cat(login_dtm), 0);
			ss.add(work_dtm_buf, 0);
			p_list->addItem(i, ss.getBuf());
			if(SyncAry.lsearch(p_item, &pos, PTR_CMPFUNC(PPSyncItem)) <= 0)
				uniq_macs++;
		}
	}
	setCtrlLong(CTL_AUSERSLST_USERSCOUNT, count);
	setCtrlLong(CTL_AUSERSLST_UNIQMACS,   uniq_macs);
	return 1;
}

int SLAPI ActiveUsersListDialog()
{
	int    ok = -1;
	ActiveUserListDlg * dlg = new ActiveUserListDlg();
	if(CheckDialogPtr(&dlg, 1))
		ExecViewAndDestroy(dlg);
	else
		ok = 0;
	return ok;
}
