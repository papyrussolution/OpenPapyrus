// PPMAIL.CPP
// Copyright (c) A. Starodub, A.Sobolev 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
//
//
int InternetAccountFilter(void * pData, void * extraPtr)
{
	const  long extra_param = reinterpret_cast<const long>(extraPtr);
	int    r = 0;
	PPInternetAccount * p_acct = static_cast<PPInternetAccount *>(pData);
	if(pData) {
		if(extra_param == 0)
			r = 1;
		else if(p_acct->Flags & PPInternetAccount::fFtpAccount)
			r = BIN(extra_param & PPObjInternetAccount::filtfFtp);
		else if(extra_param & PPObjInternetAccount::filtfMail)
			r = 1;
	}
	return r;
}

PPObjInternetAccount::PPObjInternetAccount(void * extraPtr) : PPObjReference(PPOBJ_INTERNETACCOUNT, extraPtr)
{
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
	FiltProc = InternetAccountFilter;
}

/*virtual*/ListBoxDef * PPObjInternetAccount::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr)
{
	ListBoxDef * p_def = PPObject::Selector(pOrgDef, flags, extraPtr);
	AssignImages(p_def);
	return p_def;
}

/*virtual*//*int PPObjInternetAccount::UpdateSelector_Obsolete(ListBoxDef * pDef, long flags, void * extraPtr)
{
	int    ok = PPObject::UpdateSelector(pDef, flags, extraPtr);
	if(ok > 0)
		AssignImages(pDef);
	return ok;
}*/

int PPObjInternetAccount::AssignImages(ListBoxDef * pDef)
{
	if(pDef && pDef->IsValid() && (ImplementFlags & implTreeSelector)) {
		LongArray list;
		StdTreeListBoxDef * p_def = static_cast<StdTreeListBoxDef *>(pDef);
		p_def->ClearImageAssocList();
		if(p_def->getIdList(list) > 0) {
			PPInternetAccount rec;
			for(uint i = 0; i < list.getCount(); i++) {
				PPID id = list.at(i);
				if(Get(id, &rec) > 0) {
					long   img_id = 0;
					if(rec.Flags & PPInternetAccount::fFtpAccount)
						img_id = PPDV_FTP01;
					else 
						img_id = PPDV_MAIL01;
					if(img_id)
						p_def->AddVecImageAssoc(id, img_id);
				}
			}
		}
	}
	return 1;
}

int PPObjInternetAccount::Browse(void * extraPtr)
{
	class MailAcctsView : public ObjViewDialog {
	public:
		MailAcctsView(PPObjInternetAccount * pObj, void * extraPtr) : ObjViewDialog(DLG_MAILACCVIEW, pObj, extraPtr)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			ObjViewDialog::handleEvent(event);
			if(event.isCmd(cmExport)) {
				PPIDArray mailacct_list;
				ListToListData data(PPOBJ_INTERNETACCOUNT, 0, &mailacct_list);
				data.TitleStrID = PPTXT_SELMAILACCTS;
				int    r = ListToListDialog(&data);
				if(r > 0)
					ExportEmailAccts(data.P_List);
				else if(!r)
					PPError();
			}
			else if(event.isCmd(cmTransmitCharry)) {
				PPIDArray id_list;
				ReferenceTbl::Rec rec;
				for(PPID id = 0; static_cast<PPObjReference *>(P_Obj)->EnumItems(&id, &rec) > 0;)
					id_list.add(rec.ObjID);
				if(!SendCharryObject(PPDS_CRRMAILACCOUNT, id_list))
					PPError();
			}
			else
				return;
			clearEvent(event);
		}
	};
	int    ok = -1;
	const  long extra_param = reinterpret_cast<const long>(extraPtr);
	// @v10.7.11 if(extra_param == PPObjInternetAccount::filtfMail/*INETACCT_ONLYMAIL*/) {
		MailAcctsView * p_dlg = 0;
		if(CheckRights(PPR_READ) && CheckDialogPtr(&(p_dlg = new MailAcctsView(this, extraPtr)))) {
			ExecView(p_dlg);
			delete p_dlg;
			ok = 1;
		}
		else
			ok = PPErrorZ();
	// @v10.7.11 } else ok = PPObjReference::Browse(extraPtr);
	return ok;
}

MailAccCtrlGroup::Rec::Rec() : MailAccID(0), ExtraPtr(0)
{
}

MailAccCtrlGroup::MailAccCtrlGroup(uint ctlSelInput, uint editCmd) : CtlSelInput(ctlSelInput), EditCmd(editCmd)
{
}

void MailAccCtrlGroup::handleEvent(TDialog * dlg, TEvent & event)
{
	if(event.isCmd(cmEditMailAcc)) {
		PPObjInternetAccount mac_obj;
		PPID   mac_id = 0;
		dlg->getCtrlData(CtlSelInput, &mac_id);
		if(mac_obj.Edit(&mac_id, 0) == cmOK)
			SetupPPObjCombo(dlg, CtlSelInput, PPOBJ_INTERNETACCOUNT, mac_id, OLW_CANINSERT, Data.ExtraPtr);
	}
}

int MailAccCtrlGroup::setData(TDialog * dlg, void * pData)
{
	Data = *static_cast<Rec *>(pData);
	SetupPPObjCombo(dlg, CtlSelInput, PPOBJ_INTERNETACCOUNT, Data.MailAccID, OLW_CANINSERT, Data.ExtraPtr);
	return 1;
}

int MailAccCtrlGroup::getData(TDialog * dlg, void * pData)
{
	dlg->getCtrlData(CtlSelInput, &Data.MailAccID);
	if(pData)
		*static_cast<Rec *>(pData) = Data;
	return 1;
}
//
//
//
PPInternetAccount2::PPInternetAccount2()
{
	memzero(this, offsetof(PPInternetAccount, ExtStr)-0);
}

void PPInternetAccount::Init()
{
	ExtStr.Z();
	memzero(this, offsetof(PPInternetAccount, ExtStr)-0);
}

int PPInternetAccount::Cmp(const PPInternetAccount * pAccount) const
{
	int    ok = 1;
	if(!(Flags & fFtpAccount) && pAccount) {
		SString smtp1, smtp2, pop31, pop32, rcv_name1, rcv_name2, from1, from2;
		GetExtField(MAEXSTR_SENDSERVER,  smtp1);
		GetExtField(MAEXSTR_RCVSERVER,   pop31);
		GetExtField(MAEXSTR_RCVNAME,     rcv_name1);
		GetExtField(MAEXSTR_FROMADDRESS, from1);
		pAccount->GetExtField(MAEXSTR_SENDSERVER,  smtp2);
		pAccount->GetExtField(MAEXSTR_RCVSERVER,   pop32);
		pAccount->GetExtField(MAEXSTR_RCVNAME,     rcv_name2);
		pAccount->GetExtField(MAEXSTR_FROMADDRESS, from2);
		if(smtp1.CmpNC(smtp2) == 0 && pop31.CmpNC(pop32) == 0 && (rcv_name1.CmpNC(rcv_name2) == 0 || from1.CmpNC(from2) == 0))
			ok = 0;
	}
	return ok;
}

int PPInternetAccount::NotEmpty()
{
	int    ok = 1;
	if(!(Flags & fFtpAccount)) {
		SString msg;
		SString smtp, smtp_port, pop3, pop3_port, user, from;
		GetExtField(MAEXSTR_SENDSERVER,  smtp);
		GetExtField(MAEXSTR_SENDPORT,    smtp_port);
		GetExtField(MAEXSTR_RCVSERVER,   pop3);
		GetExtField(MAEXSTR_RCVPORT,     pop3_port);
		GetExtField(MAEXSTR_RCVNAME,     user);
		GetExtField(MAEXSTR_FROMADDRESS, from);
		msg.CatEq("Name", Name).CatDiv(',', 2).CatEq("SMTP", smtp).CatDiv(',', 2).
			CatEq("POP3", pop3).CatDiv(',', 2).CatEq("From", from).CatDiv(',', 2).CatEq("User", user);
		PPSetAddedMsgString(msg);
		THROW_PP(strip(Name)[0] != 0, PPERR_NAMENEEDED);
		THROW_PP(smtp.NotEmpty(),     PPERR_MAIL_INVSMTPSERVER);
		THROW_PP(smtp_port.ToLong(),  PPERR_MAIL_INVSMTPSERVERPORT);
		THROW_PP(pop3.NotEmpty(),     PPERR_MAIL_INVPOP3SERVER);
		THROW_PP(pop3_port.ToLong(),  PPERR_MAIL_INVPOP3SERVERPORT);
		THROW_PP(user.NotEmpty(),     PPERR_MAIL_RCVNAME);
		THROW_PP(from.NotEmpty(),     PPERR_MAIL_INVFROMADDRESS);
	}
	CATCHZOK
	return ok;
}

int PPInternetAccount::GetExtField(int fldID, SString & rBuf) const { return PPGetExtStrData(fldID, ExtStr, rBuf); }
int PPInternetAccount::SetExtField(int fldID, const char * pBuf) { return PPPutExtStrData(fldID, ExtStr, pBuf); }

// @v11.8.11 (moved to PPConst) #define POP3_PW_SIZE_BEFORE11509 20 // @attention изменение значения требует конвертации хранимого пароля
// @v11.8.11 (moved to PPConst) #define POP3_PW_SIZE_2           32 // @attention изменение значения требует конвертации хранимого пароля

int PPInternetAccount::SetPassword_(const char * pPassword, int fldID /* = MAEXSTR_RCVPASSWORD */)
{
	SString temp_buf;
	// @v11.5.9 {
	if(fldID == MAEXSTR_RCVPASSWORD)
		fldID = MAEXSTR_RCVPASSWORD2;
	// } @v11.5.9 
	Reference::Helper_EncodeOtherPw(0, pPassword, PPConst::PwSize_POP3_2, temp_buf);
	return SetExtField(fldID, temp_buf);
}

int PPInternetAccount::GetPassword_(char * pBuf, size_t bufLen, int fldID /* = MAEXSTR_RCVPASSWORD */) const
{
	SString temp_buf, pw_buf;
	// @v11.5.9 {
	size_t pw_buf_size = 0;
	if(oneof2(fldID, MAEXSTR_RCVPASSWORD, MAEXSTR_RCVPASSWORD2)) {
		GetExtField(MAEXSTR_RCVPASSWORD2, temp_buf);
		if(temp_buf.NotEmpty()) {
			pw_buf_size = PPConst::PwSize_POP3_2;
		}
		else {
			GetExtField(MAEXSTR_RCVPASSWORD, temp_buf);
			pw_buf_size = PPConst::PwSize_POP3_BEFORE11509;
		}
	}
	else {
		GetExtField(fldID, temp_buf);
		pw_buf_size = PPConst::PwSize_POP3_BEFORE11509;
	}
	// } @v11.5.9 
	//GetExtField(fldID, temp_buf);
	Reference::Helper_DecodeOtherPw(0, temp_buf, pw_buf_size, pw_buf);
	pw_buf.CopyTo(pBuf, bufLen);
	return 1;
}

int PPInternetAccount::SetMimedPassword_(const char * pPassword, int fldID /* = MAEXSTR_RCVPASSWORD */)
{
	int    ok = -1;
	// @v11.5.9 {
	if(fldID == MAEXSTR_RCVPASSWORD)
		fldID = MAEXSTR_RCVPASSWORD2;
	// } @v11.5.9 
	const  size_t pwd_len = sstrlen(pPassword);
	if(pwd_len) {
		size_t len = 0;
		char   out_buf[512];
		MIME64 m64;
		memzero(out_buf, sizeof(out_buf));
		m64.Decode(pPassword, pwd_len, out_buf, &len);
		ok = SetExtField(fldID, out_buf);
	}
	return ok;
}

int PPInternetAccount::GetMimedPassword_(char * pBuf, size_t bufLen, int fldID /* = MAEXSTR_RCVPASSWORD */)
{
	if(pBuf && bufLen) {
		char   out_buf[512];
		SString temp_buf;
		size_t len = 0;
		memzero(out_buf, sizeof(out_buf));
		// @v11.5.9 GetExtField(fldID, buf);
		// @v11.5.9 {
		size_t pw_buf_size = 0;
		if(oneof2(fldID, MAEXSTR_RCVPASSWORD, MAEXSTR_RCVPASSWORD2)) {
			GetExtField(MAEXSTR_RCVPASSWORD2, temp_buf);
			if(temp_buf.NotEmpty()) {
				pw_buf_size = PPConst::PwSize_POP3_2;
			}
			else {
				GetExtField(MAEXSTR_RCVPASSWORD, temp_buf);
				pw_buf_size = PPConst::PwSize_POP3_BEFORE11509;
			}
		}
		else {
			GetExtField(fldID, temp_buf);
			pw_buf_size = PPConst::PwSize_POP3_BEFORE11509;
		}
		// } @v11.5.9 
		if(temp_buf.Len() == (pw_buf_size * 3)) {
			size_t out_len = 0;
			MIME64 m64;
			m64.Encode(temp_buf, temp_buf.Len(), out_buf, sizeof(out_buf), &out_len);
		}
		strnzcpy(pBuf, out_buf, bufLen);
	}
	return 1;
}

int PPInternetAccount::GetSendPort() const
{
	SString temp_buf;
	GetExtField(MAEXSTR_SENDPORT, temp_buf);
	return temp_buf.ToLong();
}

int PPInternetAccount::GetRcvPort()
{
	SString temp_buf;
	GetExtField(MAEXSTR_RCVPORT, temp_buf);
	return temp_buf.ToLong();
}
//
//
//
static void FASTCALL SetExtStrData(TDialog * dlg, const PPInternetAccount & rData, uint ctlID, uint strID)
{
	SString temp_buf;
	rData.GetExtField(strID, temp_buf);
	dlg->setCtrlString(ctlID, temp_buf);
}

static void FASTCALL GetExtStrData(TDialog * dlg, PPInternetAccount * pData, uint ctlID, uint strID)
{
	SString temp_buf;
	dlg->getCtrlString(ctlID, temp_buf);
	if(temp_buf.NotEmptyS())
		pData->SetExtField(strID, temp_buf);
}

static void FASTCALL SetExtIntData(TDialog * dlg, const PPInternetAccount & rData, uint ctlID, uint strID)
{
	SString temp_buf;
	rData.GetExtField(strID, temp_buf);
	dlg->setCtrlLong(ctlID, temp_buf.ToLong());
}

static void FASTCALL GetExtIntData(TDialog * dlg, PPInternetAccount * pData, uint ctlID, uint strID)
{
	SString temp_buf;
	long   val = 0;
	dlg->getCtrlData(ctlID, &val);
	temp_buf.Cat(val);
	pData->SetExtField(strID, temp_buf);
}

class InetAcctDialog : public TDialog {
public:
	InetAcctDialog(uint dlgID) : TDialog(dlgID)
	{
	}
	virtual int setDTS(const PPInternetAccount * pData) = 0;
	virtual int getDTS(PPInternetAccount * pData) = 0;
protected:
	PPInternetAccount Data;
};

class MailAcctDialog : public InetAcctDialog {
public:
	MailAcctDialog() : InetAcctDialog(DLG_MAILACC)
	{
	}
	virtual int setDTS(const PPInternetAccount * pData)
	{
		char   temp_buf[256];
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_MAILACC_NAME, Data.Name);
		setCtrlData(CTL_MAILACC_SYMB, Data.Symb); // @v10.7.12
		setCtrlData(CTL_MAILACC_ID,   &Data.ID);
		disableCtrl(CTL_MAILACC_ID, (!PPMaster || Data.ID));
		SetExtStrData(this, Data, CTL_MAILACC_FROMADDR, MAEXSTR_FROMADDRESS);
		SetExtStrData(this, Data, CTL_MAILACC_SENDSRV,  MAEXSTR_SENDSERVER);
		SetExtIntData(this, Data, CTL_MAILACC_SENDPORT, MAEXSTR_SENDPORT);
		SetExtStrData(this, Data, CTL_MAILACC_RCVSRV,   MAEXSTR_RCVSERVER);
		SetExtIntData(this, Data, CTL_MAILACC_RCVPORT,  MAEXSTR_RCVPORT);
		SetExtStrData(this, Data, CTL_MAILACC_RCVNAME,  MAEXSTR_RCVNAME);
		Data.GetPassword_(temp_buf, sizeof(temp_buf), MAEXSTR_RCVPASSWORD);
		setCtrlData(CTL_MAILACC_RCVPASSWORD, temp_buf);
		// @v11.1.1 IdeaRandMem(temp_buf, sizeof(temp_buf));
		SObfuscateBuffer(temp_buf, sizeof(temp_buf)); // @v11.1.1 
		setCtrlData(CTL_MAILACC_TIMEOUT, &Data.Timeout);
		SetupStringCombo(this, CTLSEL_MAILACC_AUTHTYPE, PPTXT_SMTPAUTHTYPES, Data.SmtpAuthType);
		AddClusterAssoc(CTL_MAILACC_FLAGS, 0, Data.fUseSSL);
		SetClusterData(CTL_MAILACC_FLAGS, Data.Flags);
		return 1;
	}
	virtual int getDTS(PPInternetAccount * pData)
	{
		int    ok = 1;
		uint   selctl = 0;
		SString temp_buf;
		getCtrlData(selctl = CTL_MAILACC_NAME, Data.Name);
		THROW_PP(*strip(Data.Name) != 0, PPERR_NAMENEEDED);
		getCtrlData(CTL_MAILACC_SYMB, Data.Symb); // @v10.7.12
		GetExtStrData(this, &Data, CTL_MAILACC_FROMADDR, MAEXSTR_FROMADDRESS);
		GetExtStrData(this, &Data, CTL_MAILACC_SENDSRV,  MAEXSTR_SENDSERVER);
		GetExtIntData(this, &Data, CTL_MAILACC_SENDPORT, MAEXSTR_SENDPORT);
		GetExtStrData(this, &Data, CTL_MAILACC_RCVSRV,   MAEXSTR_RCVSERVER);
		GetExtIntData(this, &Data, CTL_MAILACC_RCVPORT,  MAEXSTR_RCVPORT);
		GetExtStrData(this, &Data, CTL_MAILACC_RCVNAME,  MAEXSTR_RCVNAME);
		getCtrlString(CTL_MAILACC_RCVPASSWORD, temp_buf);
		getCtrlData(CTLSEL_MAILACC_AUTHTYPE, &Data.SmtpAuthType);
		Data.SetPassword_(temp_buf, MAEXSTR_RCVPASSWORD);
		temp_buf.Obfuscate();
		getCtrlData(selctl = CTL_MAILACC_TIMEOUT, &Data.Timeout);
		THROW_PP(Data.Timeout >= 0 && Data.Timeout <= 600, PPERR_USERINPUT);
		GetClusterData(CTL_MAILACC_FLAGS, &Data.Flags); // @v8.3.5
		getCtrlData(CTL_MAILACC_ID, &Data.ID);
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = (selectCtrl(selctl), 0);
		ENDCATCH
		return ok;
	}
};

class FTPAcctDialog : public InetAcctDialog {
public:
	FTPAcctDialog() : InetAcctDialog(DLG_FTPACCT)
	{
	}
	virtual int setDTS(const PPInternetAccount * pData)
	{
		char temp_buf[64];
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_FTPACCT_NAME, Data.Name);
		setCtrlData(CTL_FTPACCT_SYMB, Data.Symb); // @v10.7.12
		setCtrlData(CTL_FTPACCT_ID,   &Data.ID);
		disableCtrl(CTL_FTPACCT_ID, (!PPMaster || Data.ID));
		SetExtStrData(this, Data, CTL_FTPACCT_HOST,      FTPAEXSTR_HOST);
		SetExtIntData(this, Data, CTL_FTPACCT_PORT,      FTPAEXSTR_PORT);
		SetExtStrData(this, Data, CTL_FTPACCT_USER,      FTPAEXSTR_USER);
		SetExtStrData(this, Data, CTL_FTPACCT_PROXY,     FTPAEXSTR_PROXY);
		SetExtIntData(this, Data, CTL_FTPACCT_PROXYPORT, FTPAEXSTR_PROXYPORT);
		SetExtStrData(this, Data, CTL_FTPACCT_PROXYUSER, FTPAEXSTR_PROXYUSER);
		Data.GetPassword_(temp_buf, sizeof(temp_buf), FTPAEXSTR_PASSWORD);
		setCtrlData(CTL_FTPACCT_PWD, temp_buf);
		// @v11.1.1 IdeaRandMem(temp_buf, sizeof(temp_buf));
		SObfuscateBuffer(temp_buf, sizeof(temp_buf)); // @v11.1.1 
		Data.GetPassword_(temp_buf, sizeof(temp_buf), FTPAEXSTR_PROXYPASSWORD);
		setCtrlData(CTL_FTPACCT_PROXYPWD, temp_buf);
		// @v11.1.1 IdeaRandMem(temp_buf, sizeof(temp_buf));
		SObfuscateBuffer(temp_buf, sizeof(temp_buf)); // @v11.1.1 
		setCtrlData(CTL_FTPACCT_TIMEOUT, &Data.Timeout);
		AddClusterAssoc(CTL_FTPACCT_FLAGS, 0, PPInternetAccount::fFtpPassive);
		SetClusterData(CTL_FTPACCT_FLAGS, Data.Flags);
		return 1;
	}
	virtual int getDTS(PPInternetAccount * pData)
	{
		int    ok = 1;
		uint   selctl = 0;
		char   temp_buf[64];
		getCtrlData(selctl = CTL_FTPACCT_NAME, Data.Name);
		THROW_PP(*strip(Data.Name) != 0, PPERR_NAMENEEDED);
		getCtrlData(CTL_FTPACCT_SYMB, Data.Symb); // @v10.7.12
		GetExtStrData(this, &Data, CTL_FTPACCT_HOST,      FTPAEXSTR_HOST);
		GetExtIntData(this, &Data, CTL_FTPACCT_PORT,      FTPAEXSTR_PORT);
		GetExtStrData(this, &Data, CTL_FTPACCT_USER,      FTPAEXSTR_USER);
		GetExtStrData(this, &Data, CTL_FTPACCT_PROXY,     FTPAEXSTR_PROXY);
		GetExtIntData(this, &Data, CTL_FTPACCT_PROXYPORT, FTPAEXSTR_PROXYPORT);
		GetExtStrData(this, &Data, CTL_FTPACCT_PROXYUSER, FTPAEXSTR_PROXYUSER);
		getCtrlData(CTL_FTPACCT_PWD, temp_buf);
		Data.SetPassword_(temp_buf, FTPAEXSTR_PASSWORD);
		getCtrlData(CTL_FTPACCT_PROXYPWD, temp_buf);
		Data.SetPassword_(temp_buf, FTPAEXSTR_PROXYPASSWORD);
		// @v11.1.1 IdeaRandMem(temp_buf, sizeof(temp_buf));
		SObfuscateBuffer(temp_buf, sizeof(temp_buf)); // @v11.1.1 
		getCtrlData(selctl = CTL_FTPACCT_TIMEOUT, &Data.Timeout);
		THROW_PP(Data.Timeout >= 0 && Data.Timeout <= 600, PPERR_USERINPUT);
		getCtrlData(CTL_FTPACCT_ID, &Data.ID);
		GetClusterData(CTL_FTPACCT_FLAGS, &Data.Flags);
		Data.Flags |= PPInternetAccount::fFtpAccount;
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = (selectCtrl(selctl), 0);
		ENDCATCH
		return ok;
	}
};

int PPObjInternetAccount::Edit(PPID * pID, void * extraPtr)
{
	long   extra_param = reinterpret_cast<long>(extraPtr);
	int    ok = 1;
	int    r = cmCancel;
	int    valid_data = 0;
	PPInternetAccount rec;
	InetAcctDialog * dlg = 0;
	THROW(CheckRightsModByID(pID));
	if(*pID) {
		THROW(Get(*pID, &rec) > 0);
		if(rec.Flags & rec.fFtpAccount)
			extra_param = PPObjInternetAccount::filtfFtp;
		else
			extra_param = PPObjInternetAccount::filtfMail;
	}
	if(!oneof2(extra_param, PPObjInternetAccount::filtfMail, PPObjInternetAccount::filtfFtp)) {
		uint   t = 0;
		SelectorDialog(DLG_SELIACC, CTL_SELIACC_WHAT, &t, 0);
		if(t == 0)
			extra_param = PPObjInternetAccount::filtfMail;
		else if(t == 1)
			extra_param = PPObjInternetAccount::filtfFtp;
	}
	if(extra_param == PPObjInternetAccount::filtfMail/*INETACCT_ONLYMAIL*/)
		dlg = new MailAcctDialog();
	else if(extra_param == PPObjInternetAccount::filtfFtp/*INETACCT_ONLYFTP*/)
		dlg = new FTPAcctDialog();
	THROW(CheckDialogPtr(&dlg));
	dlg->setDTS(&rec);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		if(dlg->getDTS(&rec)) {
			if(*pID)
				*pID = rec.ID;
			if(Put(pID, &rec, 1))
				valid_data = 1;
			else
				PPError();
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int PPObjInternetAccount::Put(PPID * pID, const PPInternetAccount * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				THROW(CheckDupName(*pID, pPack->Name));
				THROW(P_Ref->UpdateItem(Obj, *pID, pPack, 1, 0));
			}
			else {
				THROW(P_Ref->RemoveItem(Obj, *pID, 0));
			}
		}
		else {
			*pID = pPack->ID;
			THROW(P_Ref->AddItem(Obj, pID, pPack, 0));
		}
		if(*pID) {
			THROW(P_Ref->PutPropVlrString(Obj, *pID, MACPRP_EXTRA, pPack->ExtStr));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjInternetAccount::Get(PPID id, PPInternetAccount * pPack)
{
	int    ok = 1;
	PPInternetAccount pack;
	THROW(Search(id, &pack) > 0);
	THROW(PPRef->GetPropVlrString(Obj, id, MACPRP_EXTRA, pack.ExtStr));
	ASSIGN_PTR(pPack, pack);
	CATCHZOK
	return ok;
}
//
//
//
class AddrBookDialog : public PPListDialog {
public:
	AddrBookDialog(int asSelector);
	int    getSelection(PPID * pPersonID, SString & rMailAddr);
private:
	DECL_HANDLE_EVENT;
	virtual int  setupList();
	virtual int  addItem(long * pPos, long * pID);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);

	int    AsSelector;
	PPID   Selection;
	SelPersonIdent LastIdent;
	PPObjPerson PsnObj;
};

AddrBookDialog::AddrBookDialog(int asSelector) : PPListDialog(DLG_ADDRBOOK, CTL_ADDRBOOK_LIST), Selection(0), AsSelector(asSelector)
{
	if(AsSelector)
		setButtonText(cmOK, "Select");
	MEMSZERO(LastIdent);
	updateList(0);
}

int AddrBookDialog::getSelection(PPID * pPersonID, SString & rMailAddr)
{
	int    ok = -1;
	rMailAddr.Z();
	if(Selection) {
		PPELinkArray el_list;
		StringSet ss(SLBColumnDelim);
		PsnObj.P_Tbl->GetELinks(Selection, el_list);
		el_list.GetPhones(1, rMailAddr, ELNKRT_EMAIL);
		ok = 1;
	}
	ASSIGN_PTR(pPersonID, Selection);
	return ok;
}

IMPL_HANDLE_EVENT(AddrBookDialog)
{
	if(AsSelector)
		if(TVCOMMAND) {
			if(TVCMD == cmLBDblClk) {
				if(event.isCtlEvent(CTL_ADDRBOOK_LIST)) {
					if(SmartListBox::IsValidS(P_Box)) {
						P_Box->getCurID(&Selection);
					}
					if(IsInState(sfModal))
						endModal(cmOK);
					clearEvent(event);
				}
			}
			else if(TVCMD == cmOK) {
				if(SmartListBox::IsValidS(P_Box)) {
					P_Box->getCurID(&Selection);
				}
			}
		}
	PPListDialog::handleEvent(event);
}

int AddrBookDialog::addItem(long * /*pPos*/, long * pID)
{
	int    ok = -1;
	SelPersonIdent ident = LastIdent;
	ident.PersonID = 0;
	if(SelectPerson(&ident) > 0 && ident.PersonID) {
		LastIdent = ident;
		if(PsnObj.AddToAddrBook(ident.PersonID, -1, 1)) {
			ASSIGN_PTR(pID, ident.PersonID);
			ok = 1;
		}
		else
			ok = PPErrorZ();
	}
	return ok;
}

int AddrBookDialog::editItem(long /*pos*/, long id)
{
	return (id && PsnObj.Edit(&id, 0) == cmOK) ? 1 : -1;
}

int AddrBookDialog::delItem(long /*pos*/, long id)
{
	int    ok = -1;
	if(id && CONFIRM(PPCFM_DELETE))
		if(PsnObj.RemoveFromAddrBook(id, -1, 1))
			ok = 1;
		else
			ok = PPErrorZ();
	return ok;
}

int AddrBookDialog::setupList()
{
	int    ok = 1;
	SString sub;
	PPIDArray id_list;
	PsnObj.GetAddrBookIDList(-1, &id_list);
	for(uint i = 0; i < id_list.getCount(); i++) {
		PPID   psn_id = id_list.at(i);
		PersonTbl::Rec psn_rec;
		if(PsnObj.Fetch(psn_id, &psn_rec) > 0) {
			PPELinkArray el_list;
			StringSet ss(SLBColumnDelim);
			ss.add(psn_rec.Name);
			PsnObj.P_Tbl->GetELinks(psn_id, el_list);
			el_list.GetPhones(1, sub, ELNKRT_EMAIL);
			ss.add(sub);
			el_list.GetPhones(1, sub, ELNKRT_PHONE);
			ss.add(sub);
			if(!addStringToList(psn_id, ss.getBuf()))
				ok = 0;
		}
	}
	return ok;
}

int ViewAddressBook()
{
	AddrBookDialog * dlg = new AddrBookDialog(0);
	if(CheckDialogPtrErr(&dlg))
		ExecView(dlg);
	delete dlg;
	return 1;
}

int SelectAddressFromBook(PPID * pSelPersonID, SString & rAddr)
{
	int    ok = -1;
	AddrBookDialog * dlg = new AddrBookDialog(1);
	if(CheckDialogPtrErr(&dlg)) {
		if(ExecView(dlg) == cmOK && dlg->getSelection(pSelPersonID, rAddr) > 0)
			ok = 1;
	}
	delete dlg;
	return ok;
}
//
//
//
//
// PPMailFile
//
PPMailFile::PPMailFile(const char * pFileName)
{
	SString temp_buf;
	P_FieldStrBuf = (PPLoadText(PPTXT_MAILFILEDS, temp_buf) > 0) ? newStr(temp_buf) : 0;
	Stream = 0;
	LineBufSize = 0;
	P_LineBuf = 0;
	Open(pFileName);
}

PPMailFile::~PPMailFile()
{
	Close();
	delete P_LineBuf;
	delete P_FieldStrBuf;
}

int PPMailFile::Open(const char * pFileName)
{
	int    ok = 1;
	if(pFileName) {
		Close();
		LineBufSize = 4096;
		THROW_MEM(P_LineBuf = new char[LineBufSize]);
		PPSetAddedMsgString(pFileName);
		THROW_SL(fileExists(pFileName));
		SLibError = SLERR_OPENFAULT;
		THROW_SL(Stream = fopen(pFileName, "rt"));
		THROW(ReadHeader());
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPMailFile::Close()
{
	if(Stream) {
		SFile::ZClose(&Stream);
		LineBufSize = 0;
		ZDELETE(P_LineBuf);
		return 1;
	}
	else
		return -1;
}

const char * PPMailFile::ReadLine()
{
	if(Stream) {
		char * r;
		size_t p = 0;
		while((r = fgets(P_LineBuf+p, LineBufSize-p, Stream)) != 0) {
			p += sstrlen(chomp(P_LineBuf+p));
			int c = fgetc(Stream);
			ungetc(c, Stream);
			if(c == '\t')
				P_LineBuf[p++] = ' ';
			else
				break;
		}
		return r ? P_LineBuf : 0;
	}
	return 0;
}

int PPMailFile::IsBoundaryLine(int start) const
{
	if(P_LineBuf) {
		SString boundary;
		Msg.GetBoundary(start, boundary);
		if(boundary.NotEmpty() && strncmp(P_LineBuf, boundary, boundary.Len()) == 0)
			return 1;
	}
	return 0;
}

int PPMailFile::GetFieldTitle(uint id, SString & rBuf) const
{
	return P_FieldStrBuf ? PPGetSubStr(P_FieldStrBuf, id, rBuf) : 0;
}

int PPMailFile::GetField(const char * pLine, uint fldID, SString & rBuf) const
{
	rBuf.Z();
	SString fld_name;
	if(GetFieldTitle(fldID, fld_name))
		if(fld_name.CmpL(pLine, 1) == 0) {
			(rBuf = pLine+fld_name.Len()).Strip();
			return 1;
		}
		else
			return -1;
	else
		return 0;
}

int PPMailFile::ProcessMsgHeaderLine(const char * pLine)
{
	int    ok = 1;
	SString buf;
	if(GetField(pLine, PPMAILFLD_SUBJ, buf) > 0) {
		//SString temp_buf;
		//SCodepageIdent cpi;
		//buf.Decode_EncodedWordRFC2047(temp_buf, &cpi, 0);
		Msg.SetField(SMailMessage::fldSubj, buf);
		if(Msg.CmpField(SMailMessage::fldSubj, PPConst::P_SubjectDbDiv) == 0)
			Msg.Flags |= SMailMessage::fPpyObject;
		else if(Msg.CmpField(SMailMessage::fldSubj, PPConst::P_SubjectOrder) == 0)
			Msg.Flags |= SMailMessage::fPpyOrder;
		else if(Msg.CmpField(SMailMessage::fldSubj, PPConst::P_SubjectCharry) == 0)
			Msg.Flags |= SMailMessage::fPpyCharry;
		else if(Msg.CmpField(SMailMessage::fldSubj, SUBJECTFRONTOL, sstrlen(SUBJECTFRONTOL)) == 0)
			Msg.Flags |= SMailMessage::fFrontol;
	}
	else if(GetField(pLine, PPMAILFLD_FROM, buf) > 0) {
		Msg.SetField(SMailMessage::fldFrom, buf);
	}
	else if(GetField(pLine, PPMAILFLD_TO, buf) > 0) {
		Msg.SetField(SMailMessage::fldTo, buf);
	}
	else if(GetField(pLine, PPMAILFLD_CONTENTTYPE, buf) > 0) {
		if(strnicmp(buf, "multipart", sstrlen("multipart")) == 0) {
			Msg.Flags |= SMailMessage::fMultipart;
			const char * p = sstrchr(buf, ';');
			if(p) {
				p++;
				while(*p == ' ')
					p++;
				if(GetField(p, PPMAILFLD_BOUNDARY, buf) > 0)
					Msg.SetField(SMailMessage::fldBoundary, buf.StripQuotes());
			}
		}
	}
	return ok;
}

int PPMailFile::ReadHeader()
{
	Msg.Init();
	while(ReadLine()) {
		if(!ProcessMsgHeaderLine(P_LineBuf))
			return 0;
	}
	return 1;
}

int PPMailFile::SkipHeader()
{
	if(Stream) {
		rewind(Stream);
		while(ReadLine())
			if(strip(P_LineBuf)[0] == 0)
				return 1;
	}
	return 0;
}

int PPMailFile::ReadDisposition(SMailMessage & rMsg, SMailMessage::ContentDispositionBlock * pD)
{
	pD->Type = pD->tUnkn;
	int    ok = 1;
	SString disp_buf, temp_buf;
	if(GetField(P_LineBuf, PPMAILFLD_CONTENTDISP, disp_buf) > 0) {
		while(ReadLine()) {
			const int ws = BIN(oneof2(P_LineBuf[0], ' ', '\t'));
			if(*strip(P_LineBuf) == 0) {
				break;
			}
			else if(ws) {
				disp_buf.Space().Cat(P_LineBuf);
			}
			else {
				break;
			}
		}
		if(GetField((temp_buf = disp_buf).Strip(), PPMAILFLD_ATTACHMENT, disp_buf) > 0) {
			pD->Type = pD->tAttachment;
		}
		else if(GetField((temp_buf = disp_buf).Strip(), PPMAILFLD_INLINE, disp_buf) > 0) {
			pD->Type = pD->tInline;
		}
		if(pD->Type && disp_buf.C(0) == ';') {
			disp_buf.ShiftLeft(1).Strip();
			SStrScan scan(disp_buf);
			SString key, val, fld_name;
			while(scan.GetEqQ(key, val)) {
				if(GetFieldTitle(PPMAILFLD_FILENAME, fld_name) && key.CmpNC(fld_name) == 0) {
					Msg.AddS(val.Strip(), &pD->FileNameP);
				}
				else if(GetFieldTitle(PPMAILFLD_MODIFICATIONDATE, fld_name) && key.CmpNC(fld_name) == 0) {
					; // @todo
				}
				else if(GetFieldTitle(PPMAILFLD_CREATIONDATE, fld_name) && key.CmpNC(fld_name) == 0) {
					; // @todo
				}
				else if(GetFieldTitle(PPMAILFLD_READDATE, fld_name) && key.CmpNC(fld_name) == 0) {
					; // @todo
				}
				else if(GetFieldTitle(PPMAILFLD_SIZE, fld_name) && key.CmpNC(fld_name) == 0) {
					; // @todo
				}
			}
		}
	}
	else
		ok = 0;
	return ok;
}

int PPMailFile::SaveAttachment(const char * pAttachName, const char * pDestPath)
{
	int    ok = -1;
	SString temp_buf, file_name;
	FILE * p_out = 0;
	THROW_INVARG(pDestPath);
	THROW(SkipHeader());
	if(Msg.Flags & SMailMessage::fMultipart) {
		int    boundary = 0, this_attachment = 0;
		int    start_file = 0;
		int    line_readed = 0;
		SMailMessage::ContentDispositionBlock disposition;
		while(line_readed || ReadLine()) {
			line_readed = 0;
			int    bound_kind = -1;
			if(IsBoundaryLine(1))
				bound_kind = 1;
			else if(IsBoundaryLine(2))
				bound_kind = 2;
			else if(IsBoundaryLine(0))
				bound_kind = 0;
			if(oneof3(bound_kind, 0, 1, 2)) {
				this_attachment = 0;
				start_file = 0;
				file_name = 0;
				if(p_out) {
					//
					// До этого сохранялся предыдущий присоединенный файл.
					// Закрываем его handler.
					//
					if(boundary)
						ok = 1;
					SFile::ZClose(&p_out);
				}
				if(bound_kind == 1) // Дальше (опять) следует присоединенный файл
					boundary = 1;
				else                // Все присоединенные файлы закончились
					boundary = 0;
			}
			else if(boundary) {
				SString disp;
				size_t out_buf_len;
				if(GetField(P_LineBuf, PPMAILFLD_CONTENTCOD, disp) > 0) {
					if(disp.CmpNC("base64") != 0)
						boundary = 0; // Если тип кодировки отличается от base64, то не принимаем эту полосу.
				}
				else {
					int rd = ReadDisposition(Msg, &disposition);
					if(rd > 0) {
						line_readed = 1;
						if(disposition.FileNameP) {
							Msg.GetS(disposition.FileNameP, temp_buf);
							if(temp_buf.NotEmptyS() && (pAttachName == 0 || temp_buf.CmpNC(pAttachName) == 0)) {
								this_attachment = 1;
								SFile::ZClose(&p_out);
								(file_name = pDestPath).Strip().SetLastSlash().Cat(temp_buf);
								THROW_PP_S(p_out = fopen(file_name, "wb"), PPERR_CANTOPENFILE, file_name);
							}
						}
					}
					else if(*strip(P_LineBuf) == 0) {
						start_file = 1;
					}
					else if(this_attachment && start_file) {
						if(*strip(P_LineBuf)) {
							char   mime_buf[1024];
							THROW_PP_S(Base64_Decode(P_LineBuf, sstrlen(P_LineBuf), mime_buf, &out_buf_len), PPERR_GETATTACHS, file_name);
							SLibError = SLERR_WRITEFAULT;
							THROW_SL(fwrite(mime_buf, out_buf_len, 1, p_out) == 1);
						}
					}
				}
			}
		}
	}
	CATCHZOK
	SFile::ZClose(&p_out);
	return ok;
}

/* @v9.9.0 int PPMailFile::SaveOrder(const char * pDestFileName)
{
	int    ok = -1;
	FILE * p_out = 0;
	THROW_INVARG(pDestFileName);
	THROW(SkipHeader());
	if(Msg.Flags & SMailMessage::fPpyOrder) {
		int    store = 0;
		char   main_tag[64], start_str[64], end_str[64];
		PPGetSubStr(PPTXT_ALBATROSTAGNAMES, PPALTAGNAM_ALBORD, main_tag, sizeof(main_tag));
		sprintf(start_str, "<%s>", main_tag);
		sprintf(end_str, "</%s>", main_tag);
		PPSetAddedMsgString(pDestFileName);
		THROW_PP(p_out = fopen(pDestFileName, "wt"), PPERR_CANTOPENFILE);
		while(ReadLine()) {
			char * p = P_LineBuf;
			while(*p == ' ' || *p == '\t')
				p++;
			if(strnicmp(P_LineBuf, start_str, sstrlen(start_str)) == 0)
				store = 1;
			if(store) {
				fputs(P_LineBuf, p_out);
				fputc('\n', p_out);
			}
			if(strnicmp(P_LineBuf, end_str, sstrlen(end_str)) == 0) {
				store = 0;
				ok = 1;
			}
		}
	}
	CATCHZOK
	SFile::ZClose(&p_out);
	return ok;
}*/
//
// PPMail
//
PPMail::PPMail(const PPInternetAccount * pMailAcc)
{
	SString temp_buf;
	Init(pMailAcc);
	P_FieldStrBuf = (PPLoadText(PPTXT_MAILFILEDS, temp_buf) > 0) ? newStr(temp_buf) : 0;
}

PPMail::~PPMail()
{
	delete P_FieldStrBuf;
}

void PPMail::Init(const PPInternetAccount * pMailAcc)
{
	RVALUEPTR(MailAcc, pMailAcc);
}

int PPMail::Connect()
{
	int    ok = 1;
	InetUrl url;
	FinalizeServerUrl(url);
	THROW_SL(Sess.Connect(url));
	CATCHZOK
	return ok;
}

int PPMail::Disconnect() { return Sess.Disconnect() ? 1 : PPSetErrorSLib(); }
int PPMail::PutLine(const char * pLine) { return Sess.WriteLine(pLine, 0) ? 1 : PPSetErrorSLib(); }
int PPMail::GetLine(SString & rBuf) { return Sess.ReadLine(rBuf) ? 1 : PPSetErrorSLib(); }
int PPMail::GetFieldTitle(uint id, SString & rBuf) const { return P_FieldStrBuf ? PPGetSubStr(P_FieldStrBuf, id, rBuf) : 0; }

static void PreprocessEncodedField(const char * pLine, SString & rResult)
{
	rResult.Z();
	SString temp_buf;
	const char * p_org = pLine;
	while(*p_org) {
		const char * p = 0;
		if(p_org[0] == '=' && p_org[1] == '?' && (p = strstr(p_org+2, "?=")) != 0) {
			temp_buf = p_org + 2;
			temp_buf.Trim(p - p_org - 2);
			const char * p_utf_prefix = "UTF-8?B?";
			if(temp_buf.HasPrefixIAscii(p_utf_prefix)) {
				temp_buf.ShiftLeft(sstrlen(p_utf_prefix));
				char   mime_buf[1024];
				size_t mime_len = 0;
				temp_buf.DecodeMime64(mime_buf, sizeof(mime_buf), &mime_len);
				temp_buf.Z().CatN(mime_buf, mime_len);
				temp_buf.Utf8ToChar();
			}
			else {
				size_t sp = 0;
				if(temp_buf.Search("?B?", 0, 0, &sp)) {
					temp_buf.ShiftLeft(sp+3);
				}
			}
			rResult.Cat(temp_buf);
			p_org = p+2;
		}
		else
			rResult.CatChar(*p_org++);
	}
}

int PPMail::GetField(const char * pLine, uint fldID, SString & rBuf) const
{
	SString fld_name;
	if(GetFieldTitle(fldID, fld_name))
		if(fld_name.CmpL(pLine, 1) == 0) {
			SString temp_buf;
			PPLogMessage(PPFILNAM_DEBUG_LOG, pLine, LOGMSGF_TIME); // @debug
			//PreprocessEncodedField(pLine+fld_name.Len(), rBuf);
			(temp_buf = pLine+fld_name.Len()).Strip();
			SCodepageIdent cpi;
			temp_buf.Decode_EncodedWordRFC2047(rBuf, &cpi, 0);
			rBuf.Strip();
			return 1;
		}
		else
			return -1;
	else
		return 0;
}

int PPMail::PutField(uint fldId, const char * pVal, SString & rResult) const
{
	int    ok = 1;
	SString temp_buf;
	if(GetFieldTitle(fldId, temp_buf)) {
		if(pVal) {
			temp_buf.Space().Cat(pVal);
		}
	}
	else
		ok = 0;
	rResult = temp_buf;
	return ok;
}

static void FASTCALL mkmailcmd(SString & rBuf, const char * pCmd, int type, const void * pAddInfo)
{
	rBuf = pCmd;
	if(type == 1)
		rBuf.Space().Cat(static_cast<const char *>(pAddInfo));
	else if(type == 2)
		rBuf.Space().Cat(reinterpret_cast<long>(pAddInfo));
	else if(type == 3)
		rBuf.Cat(static_cast<const char *>(pAddInfo));
}
//
//
//
PPMailPop3::PPMailPop3(const PPInternetAccount * pMailAcc) : PPMail(pMailAcc)
{
}

int PPMailPop3::FinalizeServerUrl(InetUrl & rUrl)
{
	SString server_addr;
	MailAcc.GetExtField(MAEXSTR_RCVSERVER, server_addr);
	rUrl.Set(server_addr, 0);
	const int protocol = (MailAcc.Flags & MailAcc.fUseSSL) ? InetUrl::protPOP3S : InetUrl::protPOP3;
	rUrl.SetProtocol(protocol);
	int    port = MailAcc.GetRcvPort();
	SETIFZ(port, InetUrl::GetDefProtocolPort(protocol));
	rUrl.SetPort_(port);
	return 1;
}

int PPMailPop3::SendCmd(long cmd, const char * pAddedInfo, long addedInfo, SString & rReplyBuf)
{
	int    ok = 1;
	SString buf;
	switch(cmd) {
		case POP3CMD_RETR:
			if(addedInfo)
				mkmailcmd(buf, "RETR", 2, reinterpret_cast<void *>(addedInfo));
			else
				ok = -1;
			break;
		case POP3CMD_TOP:
			(buf = "TOP").Space().Cat(addedInfo).Space().Cat("30");
			break;
		default: ok = -1; break;
	}
	if(ok > 0) {
		THROW(PutLine(buf));
		THROW(GetLine(buf));
		THROW_SL(Sess.CheckReply(buf));
	}
	else {
		buf.Z();
		ok = 0;
	}
	CATCHZOK
	rReplyBuf = buf;
	return ok;
}

int PPMailPop3::Login()
{
	int    ok = 1;
	SString reply_buf, user_name;
	char   psw[64];
	MailAcc.GetExtField(MAEXSTR_RCVNAME, user_name);
	MailAcc.GetPassword_(psw, sizeof(psw), MAEXSTR_RCVPASSWORD);
	THROW_SL(Sess.Auth(0, user_name, psw));
	CATCHZOK
	// @v11.1.1 IdeaRandMem(psw, sizeof(psw));
	SObfuscateBuffer(psw, sizeof(psw)); // @v11.1.1 
	return ok;
}

int PPMailPop3::GetStat(long * pCount, long * pSize)
{
	return Sess.Pop3_GetStat(pCount, pSize) ? 1 : PPSetErrorSLib();
}

int PPMailPop3::ProcessMsgHeaderLine(const char * pLine, SMailMessage * pMsg)
{
	int    ok = 1;
	SString buf, temp_buf;
	if(GetField(pLine, PPMAILFLD_SUBJ, buf) > 0) {
		pMsg->SetField(SMailMessage::fldSubj, buf);
		if(pMsg->CmpField(SMailMessage::fldSubj, PPConst::P_SubjectDbDiv) == 0)
			pMsg->Flags |= SMailMessage::fPpyObject;
		else if(pMsg->CmpField(SMailMessage::fldSubj, PPConst::P_SubjectOrder) == 0)
			pMsg->Flags |= SMailMessage::fPpyOrder;
		else if(pMsg->CmpField(SMailMessage::fldSubj, PPConst::P_SubjectCharry) == 0)
			pMsg->Flags |= SMailMessage::fPpyCharry;
		else if(pMsg->CmpField(SMailMessage::fldSubj, SUBJECTFRONTOL, sstrlen(SUBJECTFRONTOL)) == 0)
			pMsg->Flags |= SMailMessage::fFrontol;
	}
	else if(GetField(pLine, PPMAILFLD_FROM, buf) > 0) {
		pMsg->SetField(SMailMessage::fldFrom, buf);
	}
	else if(GetField(pLine, PPMAILFLD_TO, buf) > 0) {
		pMsg->SetField(SMailMessage::fldTo, buf);
	}
	else if(GetField(pLine, PPMAILFLD_CONTENTTYPE, buf) > 0) {
		if(strnicmp(buf, "multipart", sstrlen("multipart")) == 0) {
			pMsg->Flags |= SMailMessage::fMultipart;
			const char * p = sstrchr(buf, ';');
			if(p && GetField((temp_buf = p+1), PPMAILFLD_BOUNDARY, buf) > 0)
				pMsg->SetField(SMailMessage::fldBoundary, buf.StripQuotes());
		}
	}
	else if(GetField(pLine, PPMAILFLD_DATE, buf) > 0) { // @debug
	}
	return ok;
}

int PPMailPop3::GetMsgInfo(long msgN, SMailMessage * pMsg)
{
	int    ok = 1;
	SString line_buf;
	long   msg_size = 0, symb_num = 0;
	pMsg->Init();
	THROW_SL(Sess.Pop3_GetMsgSize(msgN, &msg_size) > 0);
	pMsg->Size = msg_size;
	THROW(SendCmd(POP3CMD_TOP, 0, msgN, line_buf) > 0);
	do {
		THROW(GetLine(line_buf));
		THROW(ProcessMsgHeaderLine(line_buf, pMsg));
		if(line_buf[0] == '.')
			if(line_buf[1] == '.')
				line_buf.ShiftLeft(1);
			else if(line_buf.Len() == 1)
				break;   // end while
	} while(1);
	CATCHZOK
	return ok;
}

int PPMailPop3::GetMsg(long msgN, SMailMessage * pMsg, const char * pFileName, MailCallbackProc pf, const IterCounter & msgCounter)
{
	int    ok = 1;
	long   msg_size = 0, symb_num = 0;
	FILE * p_out = 0;
	SString line_buf, temp_buf;
	IterCounter bytes_counter;
	pMsg->Init();
	PPSetAddedMsgString(pFileName);
	THROW_PP((p_out = fopen(pFileName, "wt")) != NULL, PPERR_CANTOPENFILE);
	THROW_SL(Sess.Pop3_GetMsgSize(msgN, &msg_size) > 0);
	pMsg->Size = msg_size;
	bytes_counter.Init(msg_size);
	THROW(SendCmd(POP3CMD_RETR, 0, msgN, line_buf) > 0);
	do {
		THROW(GetLine(line_buf));
		bytes_counter.Add(line_buf.Len() + 2);
		if(pf)
			pf(bytes_counter, msgCounter);
		THROW(ProcessMsgHeaderLine(line_buf, pMsg));
		if(line_buf[0] == '.')
			if(line_buf[1] == '.')
				line_buf.ShiftLeft(1);
			else if(line_buf.Len() == 1)
				break;   // end while
		{
			temp_buf.Z();
			const size_t len = line_buf.Len();
			for(size_t i = 0; i < len; i++) {
				temp_buf.CatChar(_koi8_to_866(line_buf.C(i)));
			}
			line_buf = temp_buf;
		}
		fputs(line_buf.cptr(), p_out);
		fputc('\n', p_out);
	} while(1);
	CATCHZOK
	SFile::ZClose(&p_out);
	return ok;
}

int PPMailPop3::DeleteMsg(long msgN)
{
	return Sess.Pop3_DeleteMsg(msgN) ? 1 : PPSetErrorSLib();
}

int PPMailPop3::SaveAttachment(const char * pMsgFileName, const char * pAttachName, const char * pDestPath)
{
	int    ok = -1;
	PPMailFile mail_file;
	THROW_INVARG(pMsgFileName);
	THROW_INVARG(pDestPath);
	THROW(mail_file.Open(pMsgFileName));
	THROW(ok = mail_file.SaveAttachment(pAttachName, pDestPath));
	CATCHZOK
	return ok;
}

/* @v9.9.0 int PPMailPop3::SaveOrder(const char * pMsgFileName, const char * pDestPath)
{
	int    ok = -1;
	PPMailFile mail_file;
	THROW_INVARG(pMsgFileName);
	THROW_INVARG(pDestPath);
	THROW(mail_file.Open(pMsgFileName));
	THROW(ok = mail_file.SaveOrder(pDestPath));
	CATCHZOK
	return ok;
} */
//
//
//
#if 0 // @v9.8.11 (obsolete) {
PPMailSmtp::PPMailSmtp(const PPInternetAccount * pMailAcc) : PPMail(pMailAcc)
{
}

PPMailSmtp::~PPMailSmtp()
{
}

int PPMailSmtp::Auth()
{
	int    ok = 1;
	char   pwd[64];
	SString user_name;
	memzero(pwd, sizeof(pwd));
	MailAcc.GetExtField(MAEXSTR_RCVNAME, user_name);
	MailAcc.GetPassword(pwd, sizeof(pwd));
	THROW_SL(Sess.Auth(MailAcc.SmtpAuthType, user_name, pwd));
	CATCHZOK
	return ok;
}

int PPMailSmtp::FinalizeServerUrl(InetUrl & rUrl)
{
	SString server_addr;
	MailAcc.GetExtField(MAEXSTR_SENDSERVER, server_addr);
	rUrl.Set(server_addr, 0);

	int    protocol = (MailAcc.Flags & MailAcc.fUseSSL) ? InetUrl::protSMTPS : InetUrl::protSMTP;
	rUrl.SetProtocol(protocol);
	int    port = MailAcc.GetSendPort();
	SETIFZ(port, InetUrl::GetDefProtocolPort(protocol));
	rUrl.SetPort(port);
	return 1;
}

int PPMailSmtp::SendCmd(long cmd, const char * pAddStr, SString & rReplyBuf)
{
	int    ok = 1;
	SString buf;
	switch(cmd) {
		case SMTPCMD_MAIL:
			if(pAddStr)
				mkmailcmd(buf, "MAIL FROM:", 1, pAddStr);
			else
				ok = -1;
			break;
		case SMTPCMD_RCPT:
			if(pAddStr)
				mkmailcmd(buf, "RCPT TO:", 1, pAddStr);
			else
				ok = -1;
			break;
		case SMTPCMD_DATA: mkmailcmd(buf, "DATA", 0, 0); break;
		case SMTPCMD_RSET: mkmailcmd(buf, "RSET", 0, 0); break;
		case SMTPCMD_AUTH: mkmailcmd(buf, "AUTH", 1, pAddStr); break;
		case SMTPCMD_STRING: mkmailcmd(buf, "", 3, pAddStr); break;
		default: ok = 0; break;
	}
	if(ok > 0) {
		THROW(PutLine(buf));
		THROW(GetLine(buf));
		THROW_SL(Sess.CheckReply(buf));
	}
	else {
		buf = 0;
		ok = 0;
	}
	CATCHZOK
	rReplyBuf = buf;
	return ok;
}

static void FASTCALL _PUTS(const char * pLine, FILE * out)
{
	if(pLine)
		fputs(pLine, out);
	fputc('\n', out);
}

int PPMailSmtp::MakeMessageID(SString & rBuf)
{
	rBuf.Z().CatChar('<');
	SString temp_buf;
	MailAcc.GetExtField(MAEXSTR_FROMADDRESS, temp_buf);
	const char * p = temp_buf.SearchChar('@', 0);
	if(p == 0) {
		S_GUID uuid;
		temp_buf.Z();
		if(uuid.Generate())
			uuid.ToStr(S_GUID::fmtIDL, temp_buf);
		else
			temp_buf.Cat("sl").DotCat("msg").DotCat("id").DotCat("stb");
		p = temp_buf;
	}
	else
		p++;
	LDATETIME dtm;
	getcurdatetime(&dtm);
	{
		/*SRng * p_rng = SRng::CreateInstance(SRng::algMT, 0);
		p_rng->Set(dtm.t);
		rBuf.Cat(p_rng->Get()).Dot();
		delete p_rng;*/
		rBuf.Cat(SLS.GetTLA().Rg.Get()).Dot();
	}
	/*rBuf.Cat(dtm.d.year()).Cat(dtm.d.month()).Cat(dtm.d.day()).
		Cat(dtm.t.hour()).Cat(dtm.t.minut()).Cat(dtm.t.sec()).Cat(dtm.t.hs());*/
	rBuf.Cat(dtm.d, DATF_YMD|DATF_CENTURY|DATF_NODIV).Cat(dtm.t, TIMF_HMS|TIMF_MSEC|TIMF_NODIV);
	rBuf.CatChar('@').Cat(p).CatChar('>');
	return 1;
}

int PPMailSmtp::SendMsgToFile(SMailMessage * pMsg, SString & rFileName)
{
	int    ok = -1, is_attach = 0;
	SString buf, boundary;
	SString fname, temp_buf;
	uint   i = 0;
	SDirEntry * p_fb = 0;
	FILE * f2 = 0;
	FILE * out = 0;

	THROW_INVARG(pMsg);
	/*
	if(!PPGetPath(PPPATH_TEMP, temp_buf))
		PPGetPath(PPPATH_OUT, temp_buf);
	*/
	PPMakeTempFileName("ppm", "msg", 0, fname);
	PPSetAddedMsgString(fname);
	THROW_PP_S(out = fopen(fname, "wt"), PPERR_CANTOPENFILE, fname);
	pMsg->SetField(SMailMessage::fldBoundary, pMsg->MakeBoundaryCode(temp_buf));
	is_attach = (pMsg->Flags & SMailMessage::fMultipart) ? 1 : 0;
	{
		char   sd_buf[256];
		datetimefmt(getcurdatetime_(), DATF_INTERNET, TIMF_HMS|TIMF_TIMEZONE, sd_buf, sizeof(sd_buf));
		temp_buf.Z().Cat("Date").CatDiv(':', 2).Cat(sd_buf);
		_PUTS(temp_buf, out);
	}
	PutField(PPMAILFLD_FROM, pMsg->GetField(SMailMessage::fldFrom, temp_buf), buf);
	_PUTS(buf, out);
	{
		char   ver_text[64];
		PPVersionInfo vi = DS.GetVersionInfo();
		vi.GetVersionText(ver_text, sizeof(ver_text));
		temp_buf.Z().Cat("X-Mailer").CatDiv(':', 2).Cat("Papyrus").Space().CatChar('v').Cat(ver_text);
		_PUTS(temp_buf, out);
	}
	{
		MakeMessageID(temp_buf);
		PutField(PPMAILFLD_MESSAGEID, temp_buf, buf);
		_PUTS(buf, out);
	}
	PutField(PPMAILFLD_TO, pMsg->GetField(SMailMessage::fldTo, temp_buf), buf);
	_PUTS(buf, out);
	{
		if(pMsg->GetField(SMailMessage::fldSubj, temp_buf) == PPConst::P_SubjectDbDiv) {
			//
			// Специальный случай: так как кодировка в UTF8 введена с версии 7.6.3, которая не предполагает
			// обновлений во всех разделах, то дабы более старые версии могли принять почту из 7.6.3 и выше,
			// не будем кодировать SUBJECTDBDIV ($PpyDbDivTransmission$)
			//
		}
		else {
			SString subj;
			subj.EncodeMime64(temp_buf, temp_buf.Len());
			(temp_buf = "=?UTF-8?B?").Cat(subj).Cat("?=");
		}
		PutField(PPMAILFLD_SUBJ, temp_buf, buf);
		_PUTS(buf, out);
	}
	_PUTS("MIME-Version: 1.0", out);
	if(is_attach) {
		(temp_buf = "multipart/mixed; boundary=").CatQStr(pMsg->GetBoundary(0, boundary));
		PutField(PPMAILFLD_CONTENTTYPE, temp_buf, buf);
	}
	else {
		(temp_buf = "multipart/alternative; boundary=").CatQStr(pMsg->GetBoundary(0, boundary));
		PutField(PPMAILFLD_CONTENTTYPE, temp_buf, buf);
	}
	_PUTS(buf, out);
	_PUTS(0, out); // empty line

	if(pMsg->IsField(SMailMessage::fldText)) {
		if(pMsg->IsField(SMailMessage::fldBoundary)) {
			_PUTS(pMsg->GetBoundary(1, boundary), out);
			PutField(PPMAILFLD_CONTENTTYPE, "text/plain; charset=UTF-8", buf);
			_PUTS(buf, out);
			PutField(PPMAILFLD_CONTENTCOD, "8bit", buf);
			_PUTS(buf, out);
			_PUTS(0, out); // Empty line
		}
		_PUTS(pMsg->GetField(SMailMessage::fldText, temp_buf), out);
		_PUTS(0, out); // Empty line
	}
	if(is_attach) {
		SString fn, path;
		SString img_exts;
		PPLoadText(PPTXT_PICFILESEXTS, img_exts);
		for(i = 0; pMsg->EnumAttach(&i, fn, path);) { // send attaches
			int    in_len, is_img = 0;
			SString ext;
			fn.Quot('\"', '\"');
			_PUTS(pMsg->GetBoundary(1, boundary), out);
			{
				SFsPath sp(path);
				if(img_exts.Search(sp.Ext, 0, 1, 0) > 0) {
					is_img = 1;
					ext = sp.Ext;
				}
			}
			if(is_img)
				(temp_buf = "image").Slash().Cat((const char *)ext).CatDiv(';', 2).CatEq("name", fn);
			else
				(temp_buf = "application/X-Papyrus").CatDiv(';', 2).CatEq("name", fn);
			PutField(PPMAILFLD_CONTENTTYPE, temp_buf, buf);
			_PUTS(buf, out);
			PutField(PPMAILFLD_CONTENTCOD, "base64", buf);
			_PUTS(buf, out);
			if(is_img)
				(temp_buf = "inline").CatDiv(';', 2).CatEq("filename", fn);
			else
				(temp_buf = "attachment").CatDiv(';', 2).CatEq("filename", fn);
			PutField(PPMAILFLD_CONTENTDISP, temp_buf, buf);
			_PUTS(buf, out);
			_PUTS(0, out); // Empty line

			THROW_PP_S(f2 = fopen(path, "rb"), PPERR_CANTOPENFILE, path);
			{
				char   mime_buf[256];
				while((in_len = fread(mime_buf, 1, 57, f2)) > 0) {
					char enc_buf[256];
					Base64_Encode(mime_buf, in_len, enc_buf, sizeof(enc_buf), 0);
					_PUTS(enc_buf, out);
				}
			}
			_PUTS(0, out); // Empty line
			SFile::ZClose(&f2);
		}
		_PUTS(pMsg->GetBoundary(2, boundary), out);
	}
	fputs("\n.\n", out);
	CATCH
		ok = 0;
		fname = 0;
	ENDCATCH
	SFile::ZClose(&f2);
	SFile::ZClose(&out);
	rFileName = fname;
	return ok;
}

int PPMailSmtp::SendMsgFromFile(SMailMessage * pMsg, const char * pFileName, MailCallbackProc pf, const IterCounter & msgCounter)
{
	int    ok = -1, is_attach = 0;
	SString buf, temp_buf;
	SDirEntry * p_fb = 0;
	FILE * f2 = 0;
	FILE * in = 0;

	THROW_INVARG(pMsg);
	THROW_PP(in = fopen(pFileName, "rt"), PPERR_CANTOPENFILE);
	is_attach = (pMsg->Flags & SMailMessage::fMultipart) ? 1 : 0;

	temp_buf.Z().CatChar('<').Cat(pMsg->GetField(SMailMessage::fldFrom, buf)).CatChar('>');
	THROW(SendCmd(SMTPCMD_MAIL, temp_buf, buf));
	{
		SString addr_buf;
		StringSet ss(',', pMsg->GetField(SMailMessage::fldTo, temp_buf));
		for(uint i = 0; ss.get(&i, temp_buf);) {
			addr_buf.Z().CatChar('<').Cat(temp_buf.Strip()).CatChar('>');
			THROW(SendCmd(SMTPCMD_RCPT, addr_buf, buf));
		}
	}
	THROW(SendCmd(SMTPCMD_DATA, 0, buf));
	THROW(TransmitFile(pFileName, pf, msgCounter));
	THROW(GetLine(buf));
	THROW_SL(Sess.CheckReply(buf));
	CATCHZOK
	SFile::ZClose(&f2);
	SFile::ZClose(&in);
	return ok;
}

int PPMailSmtp::TransmitFile(const char * pFileName, MailCallbackProc pf, const IterCounter & msgCounter)
{
	const  size_t KB = 1024;
	int    ok = 1;
	char * buf = 0;
	size_t buflen = 0;
	ulong  flen = 0;
	int    bytes_read = 0;
	int    fh = -1;
	IterCounter bytes_counter;

	PPSetAddedMsgString(pFileName);
	THROW_SL(fileExists(pFileName));
	fh = open(pFileName, O_RDONLY | O_BINARY, S_IWRITE | S_IREAD);
	THROW_PP(fh >= 0, PPERR_CANTOPENFILE);
	lseek(fh, 0L, SEEK_END);
	flen = tell(fh);
	lseek(fh, 0L, SEEK_SET);
	if(flen < buflen)
		buflen = KB * (1 + (uint)flen / KB);
	else
		buflen = 8 * KB;
	THROW_MEM(buf = static_cast<char *>(SAlloc::M(buflen)));
	bytes_counter.Init(flen);
	do {
		THROW(PPCheckUserBreak());
		bytes_read = read(fh, buf, (flen < buflen) ? (uint)flen : buflen);
		SLibError = SLERR_READFAULT;
		THROW_SL(bytes_read > 0);
		THROW_SL(Sess.WriteBlock(buf, bytes_read));
		bytes_counter.Add(bytes_read);
		if(pf)
			pf(bytes_counter, msgCounter);
		flen -= bytes_read;
	} while(flen > 0);
	CATCHZOK
	if(fh >= 0)
		_close(fh);
	SAlloc::F(buf);
	return ok;
}

/*static*/int PPMailSmtp::Send(const PPInternetAccount & rAcc, SMailMessage & rMsg, MailCallbackProc cbProc, const IterCounter & rMsgCounter)
{
	int    ok = 1;
	PPMailSmtp cli(&rAcc);
	THROW(cli.Connect());
	THROW(cli.Auth());
	{
		SString file_name;
		THROW(cli.SendMsgToFile(&rMsg, file_name));
		THROW(cli.SendMsgFromFile(&rMsg, file_name, cbProc, rMsgCounter));
		if(!(CConfig.Flags & CCFLG_DEBUG))
			SFile::Remove(file_name);
	}
	CATCHZOK
	return ok;
}
#endif // } 0 @v9.8.11 (obsolete)
//
//
//
static void SendMailCallback(const IterCounter & bytesCounter, const IterCounter & msgCounter)
{
	SString msg;
	PPLoadText(PPTXT_SENDMAILWAITMSG, msg);
	if(msgCounter.GetTotal() > 1)
		msg.Space().Cat(msgCounter).Slash().Cat(msgCounter.GetTotal());
	PPWaitPercent(bytesCounter, msg);
}

int SendMailWithAttach(const char * pSubj, const char * pPath, const char * pLetter, const char * pMail, PPID accountID)
{
	int    ok = -1;
	THROW_INVARG(pPath);
	{
		SStrCollection files_list;
		THROW_SL(files_list.insert(newStr(pPath)));
		THROW(ok = SendMail(pSubj, pLetter, pMail, accountID, &files_list, 0));
	}
	CATCHZOK
	return ok;
}

int SendMail(const char * pSubj, const char * pLetter, const char * pMail, PPID accountID, SStrCollection * pFilesList, PPLogger * pLogger)
{
	int    ok = 1;
	StrAssocArray  mail_list;
	PPInternetAccount account;
	PPObjInternetAccount ia_obj;
	if(ia_obj.Get(accountID, &account) > 0) {
		mail_list.Add(1, 0, pMail);
		ok = SendMail(pSubj, pLetter, &mail_list, &account, pFilesList, pLogger);
	}
	else
		ok = 0;
	return ok;
}

int SendMail(const char * pSubj, const char * pLetter, StrAssocArray * pMailList, PPInternetAccount * pAccount, SStrCollection * pFilesList, PPLogger * pLogger)
{
	int    ok = 1, conn = 0;
	SString ok_msg, buf, from_addr, mail_addr;
	IterCounter msg_counter;
	THROW_INVARG(pSubj && pMailList && pMailList->getCount());
	PPLoadText(PPTXT_MAIL_SENDOK, ok_msg);
	msg_counter.Init(1);
	for(uint i = 0; i < pMailList->getCount(); i++) {
		SMailMessage mail_msg;
		mail_addr = pMailList->Get(i).Txt;
		pAccount->GetExtField(MAEXSTR_FROMADDRESS, from_addr);
		mail_msg.SetField(SMailMessage::fldSubj,     pSubj);
		mail_msg.SetField(SMailMessage::fldFrom,     from_addr.cptr());
		mail_msg.SetField(SMailMessage::fldTo,       mail_addr.cptr());
		THROW_SL(mail_msg.AttachContent(0, SFileFormat::Txt, cpUTF8, pLetter, sstrlen(pLetter)));
		if(pFilesList) {
			char * p_path = 0;
			for(uint i = 0; pFilesList->enumItems(&i, (void **)&p_path);)
				mail_msg.AttachFile(0, SFileFormat::Unkn, p_path);
		}
		THROW(PPSendEmail(*pAccount, mail_msg, SendMailCallback, msg_counter));
		buf.Printf(ok_msg.cptr(), mail_addr.cptr());
		CALLPTRMEMB(pLogger, Log(buf));
	}
	ok = 1;
	CATCH
		CALLPTRMEMB(pLogger, LogLastError());
		ok = 0;
	ENDCATCH
	return ok;
}
//
// PPEmailAcctsImporter
//
int InitImpExpParam(PPImpExpParam * pParam, const char * pFileName, int forExport)
{
	int    ok = 1;
	SStrCollection fields;
	THROW_INVARG(pParam && pFileName);
	pParam->Init();
	THROW(LoadSdRecord(PPREC_EMAILACCOUNT, &pParam->InrRec));
	pParam->Direction  = forExport ? 0 : 1;
	pParam->DataFormat = PPImpExpParam::dfText;
	pParam->TdfParam.Flags |= TextDbFile::fCpOem/*|TextDbFile::fVerticalRec*/;
	pParam->TdfParam.FldDiv.CopyFrom(";");
	pParam->FileName.CopyFrom(pFileName);
	pParam->OtrRec = pParam->InrRec;
	CATCHZOK
	return ok;
}
//
//
//
class PPEmailAcctsImporter {
public:
	PPEmailAcctsImporter()
	{
	}
	int Init(const PPImpExpParam * pImpExpParam);
    int Import(PPLogger * pLogger, int useTa);
private:
	int Check(PPInternetAccount * pAccount, PPLogger * pLogger);
	int ResolveAuthType(const char * pAuthType, uint16 * pOutAuthType);
	PPImpExpParam ImpExpParam;
	PPObjInternetAccount MAcctObj;
};

int PPEmailAcctsImporter::Init(const PPImpExpParam * pImpExpParam)
{
	int    ok = 1;
	if(!RVALUEPTR(ImpExpParam, pImpExpParam))
		ok = PPSetErrorInvParam();
	return ok;
}

int PPEmailAcctsImporter::Import(PPLogger * pLogger, int useTa)
{
	int    ok = 1;
	long   imported = 0, count = 0;
	PPImpExp ie(&ImpExpParam, 0);
	{
		PPTransaction tra(useTa);
		THROW(tra);
		THROW(ie.OpenFileForReading(0));
		ie.GetNumRecs(&count);
		for(long i = 0; i < count; i++) {
			SString buf;
			Sdr_EmailAccount  account_rec;
			PPInternetAccount account;
			THROW(ie.ReadRecord(&account_rec, sizeof(account_rec)));
			STRNSCPY(account.Name, account_rec.Name);
			account.Timeout = account_rec.Timeout;
			account.SetExtField(MAEXSTR_SENDSERVER, account_rec.SmtpServer);
			ResolveAuthType(account_rec.AuthType, &account.SmtpAuthType);
			buf.Z().Cat((long)account_rec.SmtpPort);
			account.SetExtField(MAEXSTR_SENDPORT, buf);
			account.SetExtField(MAEXSTR_RCVSERVER, account_rec.Pop3Server);
			buf.Z().Cat((long)account_rec.Pop3Port);
			account.SetExtField(MAEXSTR_RCVPORT, buf);
			account.SetExtField(MAEXSTR_RCVNAME, account_rec.RcvName);
			account.SetPassword_(account_rec.RcvPassword, MAEXSTR_RCVPASSWORD);
			account.SetExtField(MAEXSTR_FROMADDRESS, account_rec.FromAddress);
			account.SetMimedPassword_(account_rec.RcvPassword, MAEXSTR_RCVPASSWORD);
			if(Check(&account, pLogger) > 0) {
				PPID id = 0;
				THROW(MAcctObj.Put(&id, &account, 0));
				if(pLogger) {
					SString msg, buf2;
					PPLoadText(PPTXT_MAILACCOUNTIMPOTED, buf2);
					buf.Z().CatEq("Name", account_rec.Name).
						CatDiv(',', 2).CatEq("SMTP", account_rec.SmtpServer).
						CatDiv(',', 2).CatEq("POP3", account_rec.Pop3Server).
						CatDiv(',', 2).CatEq("From", account_rec.FromAddress).
						CatDiv(',', 2).CatEq("User", account_rec.RcvName);
					msg.Printf(buf2, buf.cptr());
					CALLPTRMEMB(pLogger, Log(msg)); // @v10.4.10 @fix Log(buf)-->Log(msg)
				}
				imported++;
			}
			else {
				CALLPTRMEMB(pLogger, LogLastError());
				ok = -1;
			}
			PPWaitPercent(i + 1, count);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	if(pLogger) {
		SString msg, buf;
		PPLoadText(PPTXT_IMPORTEDRECS, buf);
		msg.Printf(buf, imported, count);
		pLogger->Log(msg);
	}
	return ok;
}

int PPEmailAcctsImporter::Check(PPInternetAccount * pAccount, PPLogger * pLogger)
{
	int    ok = -1;
	if(pAccount) {
		PPInternetAccount account;
		SString smtp, smtp_port, pop3, pop3_port, user, from;
		pAccount->GetExtField(MAEXSTR_SENDSERVER,  smtp);
		pAccount->GetExtField(MAEXSTR_RCVSERVER,   pop3);
		pAccount->GetExtField(MAEXSTR_RCVNAME,     user);
		pAccount->GetExtField(MAEXSTR_FROMADDRESS, from);
		THROW(pAccount->NotEmpty());
		for(PPID id = 0; MAcctObj.EnumItems(&id, &account) > 0;) {
			if(!(account.Flags & PPInternetAccount::fFtpAccount)) {
				SString msg;
				THROW(MAcctObj.Get(id, &account));
				msg.CatEq("Name", pAccount->Name).CatDiv(',', 2);
				msg.CatEq("SMTP", smtp).CatDiv(',', 2);
				msg.CatEq("POP3", pop3).CatDiv(',', 2);
				msg.CatEq("From", from).CatDiv(',', 2);
				msg.CatEq("User", user);
				THROW_PP_S(pAccount->Cmp(&account) != 0, PPERR_MAIL_DUPINETACCOUNT, msg);
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPEmailAcctsImporter::ResolveAuthType(const char * pAuthType, uint16 * pOutAuthType)
{
	int    ok = -1;
	uint16 auth_type = 0;
	if(pAuthType && sstrlen(pAuthType)) {
		SString auths, auth, buf;
		auth.CopyFrom(pAuthType);
		auth.Strip();
		PPLoadText(PPTXT_SMTPAUTHTYPES, auths);
		for(uint i = 0; ok < 0 && PPGetSubStr(auths, i, buf) > 0; i++) {
			uint j = 0;
			long id = 0;
			StringSet ss(',', buf);
			ss.get(&j, buf);
			id = buf.ToLong();
			ss.get(&j, buf);
			if(buf.CmpNC(auth) == 0) {
				auth_type = (uint16)id;
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pOutAuthType, auth_type);
	return ok;
}

int ImportEmailAccts()
{
	int    ok = -1;
	SString path;
	if(PPOpenFile(PPTXT_FILPAT_MAILACC, path, 0, 0) > 0) {
		PPLogger logger;
		PPImpExpParam param;
		PPEmailAcctsImporter importer;
		THROW(InitImpExpParam(&param, path, 0));
		THROW(importer.Init(&param));
		THROW((ok = importer.Import(&logger, 1)));
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
int ExportEmailAccts(const PPIDArray * pMailAcctsList)
{
	int    ok = 1;
	uint   exported = 0;
	SString path;
	PPInternetAccount account;
	PPImpExp * p_ie = 0;
	PPImpExpParam param;
	PPObjInternetAccount mobj;
	PPWaitStart();
	PPGetFilePath(PPPATH_OUT, PPFILNAM_MAILACCTS, path);
	THROW(InitImpExpParam(&param, path, 1));
	THROW_MEM(p_ie = new PPImpExp(&param, 0));
	THROW(p_ie->OpenFileForWriting(0, 1));
	for(PPID id = 0; mobj.EnumItems(&id, &account) > 0;) {
		if(!(account.Flags & PPInternetAccount::fFtpAccount) && (!pMailAcctsList || pMailAcctsList->lsearch(id))) {
			Sdr_EmailAccount account_rec;
			SString smtp, smtp_port, pop3, pop3_port, user, from;
			THROW(mobj.Get(id, &account));
			account.GetExtField(MAEXSTR_SENDSERVER,  smtp);
			account.GetExtField(MAEXSTR_SENDPORT,    smtp_port);
			account.GetExtField(MAEXSTR_RCVSERVER,   pop3);
			account.GetExtField(MAEXSTR_RCVPORT,     pop3_port);
			account.GetExtField(MAEXSTR_RCVNAME,     user);
			account.GetExtField(MAEXSTR_FROMADDRESS, from);
			STRNSCPY(account_rec.Name, account.Name);
			account_rec.Timeout = account.Timeout;
			if(account.SmtpAuthType) {
				SString auth;
				PPGetSubStr(PPTXT_SMTPAUTHTYPES, account.SmtpAuthType - 1, auth);
				STRNSCPY(account_rec.AuthType, auth.SearchChar(',', 0) + 1);
			}
			smtp.CopyTo(account_rec.SmtpServer, sizeof(account_rec.SmtpServer));
			account_rec.SmtpPort = (int16)smtp_port.ToLong();
			pop3.CopyTo(account_rec.Pop3Server, sizeof(account_rec.Pop3Server));
			account_rec.Pop3Port = (int16)pop3_port.ToLong();
			user.CopyTo(account_rec.RcvName, sizeof(account_rec.RcvName));
			from.CopyTo(account_rec.FromAddress, sizeof(account_rec.FromAddress));
			account.GetMimedPassword_(account_rec.RcvPassword, sizeof(account_rec.RcvPassword), MAEXSTR_RCVPASSWORD);
			THROW(p_ie->AppendRecord(&account_rec, sizeof(account_rec)));
			exported++;
		}
	}
	PPWaitStop();
	CATCHZOKPPERR
	ZDELETE(p_ie);
	{
		SString buf, msg;
		PPLoadText(PPTXT_EXPORTEDRECS, buf);
		msg.Printf(buf, exported);
		messageBox(msg, mfInfo|mfOK);
	}
	return ok;
}
