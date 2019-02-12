// V_REPORT.CPP
// Copyright (c) A.Starodub 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2015, 2016, 2017, 2018, 2019
//
#include <pp.h>
#pragma hdrstop
#include <process.h>
#include <crpe.h>

// Prototype (PPREPORT.CPP)
int SLAPI SaveDataStruct(const char *pDataName, const char *pTempPath, const char *pRepFileName);

#if 0 // @v9.8.11 (useless) {
//
// PPInetAccountManager
//
class PPInetAccountManager {
public:
	enum MailClientType {
		mctDefault = 1,
		mctOutlook,
		mctOutlookExpress,
		mctBat,
		mctAll
	};
	int SLAPI GetMailAccounts(MailClientType mct, PPID * pActiveID, PPInetAccntArray * pAccounts);
private:
	int SLAPI GetDefaultMailClient(MailClientType *);
	int SLAPI GetOutlookAccounts(PPID * pActiveID, PPInetAccntArray * pAccounts);
	int SLAPI GetOutlookExpressAccounts(PPID * pActiveID, PPInetAccntArray * pAccounts);
	int SLAPI GetBatAccounts(PPID * pActiveID, PPInetAccntArray * pAccounts);

	int SLAPI Cat(const PPInetAccntArray * pSrc, PPInetAccntArray * pDest);
};

int SLAPI PPInetAccountManager::GetMailAccounts(MailClientType mct, PPID * pActiveID, PPInetAccntArray * pAccounts)
{
	int    ok = 1, all = (mct == mctAll) ? 1 : 0;
	PPInetAccntArray accounts;

	THROW_INVARG(pAccounts);
	pAccounts->freeAll();
	if(mct == mctDefault) {
		THROW(GetDefaultMailClient(&mct));
	}
	if(all || mct == mctOutlook) {
		THROW(GetOutlookAccounts(pActiveID, &accounts));
		Cat(&accounts, pAccounts);
	}
	else if(all || mct == mctOutlookExpress) {
		THROW(GetOutlookExpressAccounts(pActiveID, &accounts));
		Cat(&accounts, pAccounts);
	}
	else if(all || mct == mctBat) {
		THROW(GetBatAccounts(pActiveID, &accounts));
		Cat(&accounts, pAccounts);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPInetAccountManager::GetOutlookAccounts(PPID * pActiveID, PPInetAccntArray * pAccounts)
{
	int ok = 1;
	const char * p_param_name        = "Account Name";
	const char * p_param_addr        = "Email";
	const char * p_param_smtp        = "SMTP Server";
	const char * p_param_port        = "SMTP Port";
	const char * p_param_defmailacct = "{ED475418-B0D6-11D2-8C3B-00104B2A6676}";
	const char * p_root_key          = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows Messaging Subsystem\\Profiles\\Outlook\\9375CFF0413111d3B88A00104B2A6676";
	char  buf[256];
	WCHAR w_buf[512];
	SString   active_id, accnt_id;
	WinRegKey reg_accts;

	THROW_INVARG(pAccounts);
	pAccounts->freeAll();
	THROW_PP(reg_accts.Open(HKEY_CURRENT_USER, p_root_key, 1, 1), PPERR_SLIB);
	THROW_PP(reg_accts.GetBinary(p_param_defmailacct, w_buf, sizeof(w_buf)), PPERR_SLIB)
	WideCharToMultiByte(1251, 0, w_buf, -1, buf, sizeof(buf), 0, 0);
	active_id.CopyFrom(buf);
	THROW_PP(reg_accts.Open(HKEY_CURRENT_USER, p_root_key, 1, 1), PPERR_SLIB);
	for(uint idx = 0; reg_accts.EnumKeys(&idx, accnt_id) > 0;) {
		SString accnt_par;
		WinRegKey reg;
		accnt_par.CopyFrom(p_root_key).SetLastSlash().Cat(accnt_id);
		if(reg.Open(HKEY_CURRENT_USER, (const char*)accnt_par, 1, 1) > 0) {
			SString name, fromaddr, sendserv, port;
			PPInternetAccount * p_account = 0;
			if(reg.GetBinary(p_param_name, w_buf, sizeof(w_buf)) > 0) {
				WideCharToMultiByte(1251, 0, w_buf, -1, buf, sizeof(buf), 0, 0);
				name.CopyFrom(buf);
			}
			else
				continue;
			if(reg.GetBinary(p_param_addr, w_buf, sizeof(w_buf)) > 0) {
				WideCharToMultiByte(1251, 0, w_buf, -1, buf, sizeof(buf), 0, 0);
            	fromaddr.CopyFrom(buf);
			}
			else
				continue;
			if(reg.GetBinary(p_param_smtp, w_buf, sizeof(w_buf)) > 0) {
				THROW_MEM(WideCharToMultiByte(1251, 0, w_buf, -1, buf, sizeof(buf), NULL, NULL));
				sendserv.CopyFrom(buf);
			}
			else
				continue;
			memzero(buf, sizeof(buf));
			reg.GetBinary(p_param_port, buf, sizeof(buf));
			port.Cat((int)buf[0] != 0 ? (int)buf[0] : 0);
			THROW_MEM(p_account = new PPInternetAccount);
			p_account->ID = pAccounts->getCount() + 1;
			name.CopyTo(p_account->Name, sizeof(p_account->Name));
			p_account->SetExtField(MAEXSTR_SENDPORT,    port);
			p_account->SetExtField(MAEXSTR_FROMADDRESS, fromaddr);
			p_account->SetExtField(MAEXSTR_SENDSERVER,  sendserv);
			pAccounts->insert(p_account);
			if(active_id.CmpNC(accnt_id) == 0)
				ASSIGN_PTR(pActiveID, p_account->ID);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPInetAccountManager::GetOutlookExpressAccounts(PPID * pActiveID, PPInetAccntArray * pAccounts)
{
	int ok = 1;
	const char * p_root_key          = "SOFTWARE\\Microsoft\\Internet Account Manager\\Accounts";
	const char * p_acctmngr_key      = "SOFTWARE\\Microsoft\\Internet Account Manager";
	const char * p_param_name        = "Account Name";
	const char * p_param_addr        = "SMTP Email Address";
	const char * p_param_smtp        = "SMTP Server";
	const char * p_param_port        = "SMTP Port";
	const char * p_param_defmailacct = "Default Mail Account";
	char buf[256];
	SString accnt_id, active_id;
	WinRegKey reg_accts;

	THROW_INVARG(pAccounts);
	pAccounts->freeAll();
	THROW_PP(reg_accts.Open(HKEY_CURRENT_USER, p_acctmngr_key, 1, 1), PPERR_SLIB);
	THROW_PP(reg_accts.GetString(p_param_defmailacct, buf, sizeof(buf)), PPERR_SLIB);
	active_id.CopyFrom(buf);

	THROW_PP(reg_accts.Open(HKEY_CURRENT_USER, p_root_key, 1, 1), PPERR_SLIB);
	for(uint idx = 0; reg_accts.EnumKeys(&idx, accnt_id = 0) > 0;) {
		SString accnt_par;
		WinRegKey reg;
		accnt_par.CopyFrom(p_root_key).SetLastSlash().Cat(accnt_id);
		if(reg.Open(HKEY_CURRENT_USER, (const char*)accnt_par, 1, 1) > 0) {
			SString str_port;
			DWORD port = 0;
			SString name, fromaddr, sendserv;
			PPInternetAccount * p_account = 0;
			if(reg.GetString(p_param_name, buf, sizeof(buf)) > 0)
				name.CopyFrom(buf);
			else
				continue;
			if(reg.GetString(p_param_addr, buf, sizeof(buf)) > 0)
				fromaddr.CopyFrom(buf);
			else
				continue;
			if(reg.GetString(p_param_smtp, buf, sizeof(buf)) > 0)
				sendserv.CopyFrom(buf);
			else
				continue;
			reg.GetDWord(p_param_port, &port);
			str_port.Cat(port != 0 ? port : 25);

			THROW_MEM(p_account = new PPInternetAccount);
			p_account->ID = pAccounts->getCount() + 1;
			name.CopyTo(p_account->Name, sizeof(p_account->Name));
			p_account->SetExtField(MAEXSTR_SENDPORT,    str_port);
			p_account->SetExtField(MAEXSTR_FROMADDRESS, fromaddr);
			p_account->SetExtField(MAEXSTR_SENDSERVER,  sendserv);
			THROW_PP(pAccounts->insert(p_account), PPERR_SLIB);
			if(active_id.CmpNC(accnt_id) == 0)
				ASSIGN_PTR(pActiveID, p_account->ID);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPInetAccountManager::GetBatAccounts(PPID * pActiveAcctID, PPInetAccntArray * pAccounts)
{
	int ok = 1;
	THROW_INVARG(pAccounts);
	pAccounts->freeAll();
	CATCHZOK
	return ok;
}

int SLAPI PPInetAccountManager::Cat(const PPInetAccntArray * pSrc, PPInetAccntArray * pDest)
{
	if(pSrc && pDest) {
		for(uint i = 0; i < pSrc->getCount(); i++) {
			PPInternetAccount * p_account = new PPInternetAccount;
			*p_account = *pSrc->at(i);
			pDest->insert(p_account);
		}
	}
	return 1;
}

int SLAPI PPInetAccountManager::GetDefaultMailClient(MailClientType * pMCT)
{
	int ok = 1;
	const char * p_outlook       = "Microsoft Outlook";
	const char * p_outlooke      = "Outlook Express";
	const char * p_bat           = "The Bat";
	const char * p_defmailcl_key = "SOFTWARE\\Clients\\Mail";
	char def_mailcl[256];
	WinRegKey reg;

	memzero(def_mailcl, sizeof(def_mailcl));
	THROW_PP(reg.Open(HKEY_LOCAL_MACHINE, p_defmailcl_key, 1, 1), PPERR_SLIB);
	THROW_PP(reg.GetString(0, def_mailcl, sizeof(def_mailcl)), PPERR_SLIB);
	if(stricmp(def_mailcl, p_outlook) == 0) {
		ASSIGN_PTR(pMCT, mctOutlook);
	}
	else if(stricmp(def_mailcl, p_outlooke) == 0) {
		ASSIGN_PTR(pMCT, mctOutlookExpress);
	}
	else if(stricmp(def_mailcl, p_bat)) {
		ASSIGN_PTR(pMCT, mctBat);
	}
	CATCHZOK
	return ok;
}
#endif // } 0 @v9.8.11 (useless)
//
// PPViewReport
//
IMPLEMENT_PPFILT_FACTORY(Report); SLAPI ReportFilt::ReportFilt() : PPBaseFilt(PPFILT_REPORT, 0, 0)
{
	SetFlatChunk(offsetof(ReportFilt, ReserveStart), offsetof(ReportFilt, StdName) - offsetof(ReportFilt, ReserveStart));
	SetBranchSString(offsetof(ReportFilt, StdName));
	SetBranchSString(offsetof(ReportFilt, StrucName));
	Init(1, 0);
}

SLAPI PPViewReport::PPViewReport() : PPView(0, &Filt, PPVIEW_REPORT), P_StdRptFile(0), P_RptFile(0), P_TempTbl(0), LocalRptCodepage(866)
{
	DefReportId = REPORT_RPTINFO;
	PPIniFile::GetSectSymb(PPINISECT_SYSTEM, SystemSect);
	PPIniFile::GetParamSymb(  PPINIPARAM_CODEPAGE,           CodepageParam);
	PPIniFile::GetParamSymb(  PPINIPARAM_REPORT_FORMAT,      FmtParam);
	PPIniFile::GetParamSymb(  PPINIPARAM_REPORT_DESTINATION, DestParam);
	PPIniFile::GetParamSymb(  PPINIPARAM_REPORT_SILENT,      SilentParam);
	PPIniFile::GetParamSymb(  PPINIPARAM_REPORT_DATA,        DataParam);
	PPIniFile::GetParamSymb(  PPINIPARAM_REPORT_DESCRIPTION, DescrParam);
	PPIniFile::GetParamSymb(  PPINIPARAM_REPORT_MODIFDATE,   ModifDtParam);
}

SLAPI PPViewReport::~PPViewReport()
{
	SaveChanges(1);
}

PPBaseFilt * SLAPI PPViewReport::CreateFilt(void * extraPtr) const
{
	ReportFilt * p_filt = new ReportFilt;
	return p_filt;
}

int SLAPI PPViewReport::SaveChanges(int remove)
{
	SString fname;
	/*
	if(P_StdRptFile) {
		P_StdRptFile->FlashIniBuf();
		PPGetFilePath(PPPATH_BIN, PPFILNAM_STDRPT_INI, fname);
		SCopyFile(P_StdRptFile->GetFileName(), fname, 0, 0, 0);
	}
	*/
	if(P_RptFile) {
		P_RptFile->FlashIniBuf();
		PPGetFilePath(PPPATH_BIN, (uint)PPFILNAM_REPORT_INI, fname);
		SCopyFile(P_RptFile->GetFileName(), fname, 0, 0, 0);
	}
	if(remove) {
		SString temp_fname;
		if(P_StdRptFile) {
			temp_fname = P_StdRptFile->GetFileName();
			ZDELETE(P_StdRptFile); // release object before file removing
			SFile::Remove(temp_fname);
		}
		if(P_RptFile) {
			temp_fname = P_RptFile->GetFileName();
			ZDELETE(P_RptFile); // release object before file removing
			SFile::Remove(temp_fname);
		}
	}
	return 1;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempReport);

// virtual
int SLAPI PPViewReport::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	SString fname, temp_fname;
	//SString temp_dir;
	PPViewBrowser * p_prev_win = static_cast<PPViewBrowser *>(PPFindLastBrowser());
	THROW(Helper_InitBaseFilt(pFilt));
	if(p_prev_win && p_prev_win->P_View)
		((PPViewReport*)p_prev_win->P_View)->SaveChanges(0);
	else
		SaveChanges(1);
	PPGetFilePath(PPPATH_BIN, PPFILNAM_STDRPT_INI, fname);
	SCopyFile(fname, PPMakeTempFileName("stdrpt", "ini", 0, temp_fname), 0, 0, 0);
	THROW_MEM(P_StdRptFile = new PPIniFile(temp_fname, 0, 0, 1));
	PPGetFilePath(PPPATH_BIN, (uint)PPFILNAM_REPORT_INI, fname);
	SCopyFile(fname, PPMakeTempFileName("rpt", "ini", 0, temp_fname), 0, 0, 0);
	THROW_MEM(P_RptFile = new PPIniFile(temp_fname, 0, 0, 1));
	THROW(P_StdRptFile->IsValid() && P_RptFile->IsValid());
	{
		//int    codepage = 866;
		//SCodepage _cp = cp866;
		ReportViewItemArray list;
		ZDELETE(P_TempTbl);
		THROW(P_TempTbl = CreateTempFile());
		{
			BExtInsert bei(P_TempTbl);
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			THROW(CreateStdRptList(&list));
			THROW(CreateRptList(&list));
			for(uint i = 0; i < list.getCount(); i++) {
				if(CheckForFilt(&list.at(i))) {
					TempReportTbl::Rec temp_rec;
					MEMSZERO(temp_rec);
					MakeTempRec(&list.at(i), &temp_rec);
					temp_rec.ID = 0;
					THROW_DB(bei.insert(&temp_rec));
				}
			}
			THROW(bei.flash());
			THROW(tra.Commit());
		}
	}
	CATCH
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

IMPL_CMPFUNC(REPORTNAME, i1, i2)
{
	const ReportViewItem * p_i1 = (const ReportViewItem*)i1;
	const ReportViewItem * p_i2 = (const ReportViewItem*)i2;
	return stricmp866(p_i1->StdName, p_i2->StdName);
}

class ReportFiltDlg : public TDialog {
public:
	explicit ReportFiltDlg(PPViewReport * pView) : TDialog(DLG_REPORTFLT)
	{
		PPWait(1);
		ReportViewItemArray _list;
		SetupCalCtrl(CTLCAL_REPORTFLT_PERIOD, this, CTL_REPORTFLT_PERIOD, 1);
		if(pView) {
			if(pView->CreateStdRptList(&_list) > 0) {
				_list.sort(PTR_CMPFUNC(REPORTNAME));
				uint   i = 0;
				for(; i < _list.getCount(); i++) {
					ReportViewItem & r_item = _list.at(i);
					StdReportList.Add(i + 1, r_item.StdName);
					if(sstrlen(r_item.StrucName) && StrucList.SearchByText(r_item.StrucName, 1, 0) <= 0)
						StrucList.Add(i + 1, r_item.StrucName);
				}
				pView->CreateRptList(&_list);
				for(; i < _list.getCount(); i++) {
					ReportViewItem & r_item = _list.at(i);
					if(sstrlen(r_item.StrucName) && StrucList.SearchByText(r_item.StrucName, 1, 0) <= 0)
						StrucList.Add(i + 1, r_item.StrucName);
				}
			}
		}
		PPWait(0);
	}
	int	setDTS(const ReportFilt *);
	int	getDTS(ReportFilt *);
private:
	ReportFilt    Data;
	StrAssocArray StdReportList;
	StrAssocArray StrucList;
};

int ReportFiltDlg::setDTS(const ReportFilt * pData)
{
	int    ok = 1;
	uint   pos = 0;
	long   _id = 0;
	Data.Copy(pData, 1);
	if(StdReportList.SearchByText(Data.StdName, 1, &pos) > 0)
		_id = StdReportList.Get(pos).Id;
	SetupStrAssocCombo(this, CTLSEL_REPORTFLT_STDNAME, &StdReportList, _id, 0);

	if(StrucList.SearchByText(Data.StrucName, 1, &pos) > 0)
		_id = StrucList.Get(pos).Id;
	else
		_id = 0;
	SetupStrAssocCombo(this, CTLSEL_REPORTFLT_STRUC, &StrucList, _id, 0);

	SetPeriodInput(this, CTL_REPORTFLT_PERIOD, &Data.Period);
	AddClusterAssocDef(CTL_REPORTFLT_TYPE,  0, ReportFilt::rpttAll);
	AddClusterAssoc(CTL_REPORTFLT_TYPE,  1, ReportFilt::rpttStandart);
	AddClusterAssoc(CTL_REPORTFLT_TYPE,  2, ReportFilt::rpttLocal);
	SetClusterData(CTL_REPORTFLT_TYPE, Data.Type);
	AddClusterAssocDef(CTL_REPORTFLT_ORDER,  0, ReportFilt::ordByName);
	AddClusterAssoc(CTL_REPORTFLT_ORDER,  1, ReportFilt::ordByType);
	AddClusterAssoc(CTL_REPORTFLT_ORDER,  2, ReportFilt::ordByStruc);
	SetClusterData(CTL_REPORTFLT_ORDER, Data.Order);
	return ok;
}

int ReportFiltDlg::getDTS(ReportFilt * pData)
{
	int    ok = 1;
	uint   pos = 0;
	StdReportList.GetText(getCtrlLong(CTLSEL_REPORTFLT_STDNAME), Data.StdName);
	StrucList.GetText(getCtrlLong(CTLSEL_REPORTFLT_STRUC), Data.StrucName);
	SetPeriodInput(this, CTL_REPORTFLT_PERIOD, &Data.Period);
	GetClusterData(CTL_REPORTFLT_TYPE,  &Data.Type);
	GetClusterData(CTL_REPORTFLT_ORDER, &Data.Order);
	CALLPTRMEMB(pData, Copy(&Data, 1));
	return ok;
}

// virtual
int SLAPI PPViewReport::EditBaseFilt(PPBaseFilt * pFilt)
{
	DIALOG_PROC_BODY_P1ERR(ReportFiltDlg, this, (ReportFilt*)pFilt);
}

int SLAPI PPViewReport::InitIteration()
{
	int    ok  = 1, idx = 0;
	void * k = 0, * k_ = 0;
	union {
		TempReportTbl::Key1 k1;
		TempReportTbl::Key2 k2;
		TempReportTbl::Key3 k3;
	} _k, __k;

	BExtQuery::ZDelete(&P_IterQuery);
	THROW(P_TempTbl);
	Counter.Init();
	if(Filt.Order == ReportFilt::ordByStruc) {
		MEMSZERO(_k.k2);
		k  = &_k.k2;
		k_ = &__k.k2;
		idx = 2;
	}
	else if(Filt.Order == ReportFilt::ordByType) {
		MEMSZERO(_k.k3);
		k = &_k.k3;
		k_ = &__k.k3;
		idx = 3;
	}
	else {
		MEMSZERO(_k.k1);
		k  = &_k.k1;
		k_ = &__k.k1;
		idx = 1;
	}
	PPInitIterCounter(Counter, P_TempTbl);
	THROW_MEM(P_IterQuery = new BExtQuery(P_TempTbl, idx));
	P_IterQuery->selectAll();
	Counter.Init(P_IterQuery->countIterations(0, k_, spGe));
	P_IterQuery->initIteration(0, k, spFirst);
	CATCHZOK
	return ok;
}

int FASTCALL PPViewReport::NextIteration(ReportViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		ASSIGN_PTR(pItem, *((ReportViewItem*)&P_TempTbl->data));
		Counter.Increment();
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewReport::SendMail(long id)
{
	class ReportMailDialog : public TDialog {
	public:
		struct Rec {
			Rec() : AccountID(0), Dtm(ZERODATETIME)
			{
			}
			PPID    AccountID;
			SString SupportMail;
			SString MainOrg;
			SString Licence;
			SString User;
			SString DB;
			SString RptPath;
			SString Struc;
			LDATETIME Dtm;
		};
		ReportMailDialog() : TDialog(DLG_RPTMAIL)
		{
		}
		int    setDTS(const ReportMailDialog::Rec * pData)
		{
			if(!RVALUEPTR(Data, pData)) {
				Data.AccountID = 0;
				Data.SupportMail = 0;
				Data.MainOrg = 0;
				Data.Licence = 0;
				Data.User = 0;
				Data.DB = 0;
				Data.RptPath = 0;
				Data.Struc = 0;
				Data.Dtm.Z();
			}
			SString buf;
			SetupPPObjCombo(this, CTLSEL_RPTMAIL_ACCNT, PPOBJ_INTERNETACCOUNT, Data.AccountID, 0, (void *)PPObjInternetAccount::filtfMail/*INETACCT_ONLYMAIL*/);
			setCtrlString(CTL_RPTMAIL_SUPPMAIL,  Data.SupportMail);
			setCtrlString(CTL_RPTMAIL_ORG,       Data.MainOrg);
			setCtrlString(CTL_RPTMAIL_LIC,       Data.Licence);
			setCtrlString(CTL_RPTMAIL_USER,      Data.User);
			setCtrlString(CTL_RPTMAIL_DB,        Data.DB);
			setCtrlString(CTL_RPTMAIL_RPT,       Data.RptPath);
			setCtrlString(CTL_RPTMAIL_DATASTRUC, Data.Struc);
			setCtrlString(CTL_RPTMAIL_DATE,      buf.Z().Cat(Data.Dtm.d));
			setCtrlString(CTL_RPTMAIL_TIME,      buf.Z().Cat(Data.Dtm.t));
			disableCtrls(1, CTL_RPTMAIL_ORG, CTL_RPTMAIL_LIC, CTL_RPTMAIL_USER, CTL_RPTMAIL_DB, CTL_RPTMAIL_RPT, CTL_RPTMAIL_DATASTRUC, CTL_RPTMAIL_DATE, CTL_RPTMAIL_TIME, 0L);
			return 1;
		}
		int    getDTS(ReportMailDialog::Rec * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlData((sel = CTLSEL_RPTMAIL_ACCNT), &Data.AccountID);
			THROW_PP(Data.AccountID != 0, PPERR_INETACCOUNTNOTDEF);
			getCtrlString(CTL_RPTMAIL_SUPPMAIL, Data.SupportMail);
			ASSIGN_PTR(pData, Data);
			CATCH
				selectCtrl(sel);
				ok = 0;
			ENDCATCH
			return ok;
		}
	private:
		Rec    Data;
	};
	int    ok = -1, valid_data = 0;
	ReportMailDialog * p_dlg = 0;
	if(id && P_TempTbl && P_TempTbl->search(0, &id, spEq) > 0) {
		ReportMailDialog::Rec data;
		PPIniFile ini_file;
		PPAlbatrosConfig alb_cfg;
		TempReportTbl::Rec & r_rec = P_TempTbl->data;
		THROW(ini_file.IsValid());
		THROW_PP(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_SUPPORTMAIL, data.SupportMail) > 0, PPERR_SUPPORTMAILNOTDEF);
		{
			DbProvider * p_dict = CurDict;
			PPLicData lic;
			GetMainOrgName(data.MainOrg);
			PPGetLicData(&lic);

			THROW(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0);
			data.AccountID = alb_cfg.Hdr.MailAccID;
			data.Licence.CopyFrom(lic.RegName);
			GetCurUserName(data.User);
			if(p_dict)
				p_dict->GetDataPath(data.DB);
			else
				data.DB = 0;
			data.Struc.CopyFrom(r_rec.StrucName);
			data.Struc.ReplaceStr("(!)", "\0", 0);
			data.Dtm = getcurdatetime_();
			THROW(GetAltPath(r_rec.Type, r_rec.Path, r_rec.StdName, data.RptPath));
			THROW(CheckDialogPtr(&(p_dlg = new ReportMailDialog)));
			p_dlg->setDTS(&data);
			while(!valid_data && ExecView(p_dlg) == cmOK) {
				if(p_dlg->getDTS(&data)) {
					uint   pos = 0;
					SString fmt, msg;
					p_dlg->getDTS(&data);
					data.MainOrg.Transf(CTRANSF_INNER_TO_OUTER);
					data.Licence.Transf(CTRANSF_INNER_TO_OUTER);
					PPLoadText(PPTXT_RPTSUPPMAIL_HEADER, fmt);
					msg.Printf(fmt, data.MainOrg.cptr(), data.Licence.cptr(), data.User.cptr(),
						data.DB.cptr(), data.RptPath.cptr(), data.Struc.cptr());
					PPLoadText(PPTXT_RPTSUPPMAIL_TIMESTAMP, fmt);
					msg.Cat(fmt).Space().Cat(data.Dtm);
					PPWait(1);
					THROW(SendMailWithAttach("Report info", data.RptPath, msg, data.SupportMail, data.AccountID));
					PPWait(0);
					valid_data = ok = 1;
				}
				else
					PPError();
			}
		}
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

int SLAPI PPViewReport::CheckForFilt(const ReportViewItem * pItem)
{
	if(pItem) {
		uint pos = 0;
		if(!Filt.Period.IsZero() && Filt.Period.CheckDate(pItem->ModifDt) == 0)
			return 0;
		if(Filt.Type != ReportFilt::rpttAll && Filt.Type != pItem->Type)
			return 0;
		if(Filt.StrucName.NotEmptyS() && Filt.StrucName.CmpNC(pItem->StrucName) != 0)
			return 0;
		if(Filt.StdName.NotEmptyS() && Filt.StdName.CmpNC(pItem->StdName) != 0)
			return 0;
	}
	return 1;
}

void SLAPI PPViewReport::MakeTempRec(const ReportViewItem * pItem, TempReportTbl::Rec * pTempRec)
{
	if(pItem && pTempRec)
		*pTempRec = *((TempReportTbl::Rec*)pItem);
}

int SLAPI PPViewReport::GetAltPath(long type, const char * pPath, const char * pStdName, SString & rPath)
{
	int    ok = 1;
	rPath.Z();
	if(type == ReportFilt::rpttLocal && sstrlen(pPath)) {
		SString path;
		SPathStruc sp(pPath);
		if(!sp.Drv.NotEmptyS()) {
			PPGetPath(PPPATH_BIN, path);
			path.SetLastSlash().Cat(pPath);
		}
		else
			path.CopyFrom(pPath);
		rPath = path;
	}
	else {
		PPGetPath(PPPATH_BIN, rPath);
		rPath.SetLastSlash().Cat("rpt").SetLastSlash().Cat(pStdName).Cat(".rpt");
	}
	return ok;
}

int SLAPI PPViewReport::Verify(long id)
{
	int    ok = -1;
	DlRtm * p_rtm = 0;
	if(id && P_TempTbl && P_TempTbl->search(0, &id, spEq) > 0) {
		TempReportTbl::Rec & r_rec = P_TempTbl->data;
		SString alt_path, data_name;

		CrwError = PE_ERR_ERRORINDATABASEDLL;
		THROW(GetAltPath(r_rec.Type, r_rec.Path, r_rec.StdName, alt_path));
		(data_name = r_rec.StrucName).ReplaceStr("(!)", "\0", 0);
		{
			DlContext ctx;
			DlRtm::ExportParam ep;
			THROW(ctx.InitSpecial(DlContext::ispcExpData));
			THROW(ctx.CreateDlRtmInstance(data_name, &p_rtm));
			ep.Flags |= DlRtm::ExportParam::fIsView;
			THROW(p_rtm->Export(ep));
			THROW(SaveDataStruct(data_name, ep.Path, alt_path));
			ok = 1;
		}
	}
	CATCHZOK
	delete p_rtm;
	return ok;
}

int SLAPI PPViewReport::CallCR(long id)
{
	int    ok = -1;
	HKEY   crr_key = 0;
	if(id && P_TempTbl && P_TempTbl->search(0, &id, spEq) > 0) {
		char   crr_path[MAXPATH];
		DWORD  path_size = MAXPATH;
		char   crr_name[30];
		DWORD  crr_name_size = sizeof(crr_name);
		TempReportTbl::Rec & r_rec = P_TempTbl->data;

		memzero(crr_path, sizeof(crr_path));
		memzero(crr_name, sizeof(crr_name));
		// Для CRR 7.0
		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,	_T("SOFTWARE\\Seagate Software\\Crystal Reports"), 0, KEY_QUERY_VALUE, &crr_key) == ERROR_SUCCESS &&
			RegQueryValueEx(crr_key, _T("Path"), NULL, NULL, (LPBYTE)crr_path, &path_size) == ERROR_SUCCESS)
			ok = 1;
		// Или для  CRR 10
		else if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Crystal Decisions\\10.0\\Crystal Reports"), 0, KEY_QUERY_VALUE, &crr_key) == ERROR_SUCCESS &&
			RegQueryValueEx(crr_key, _T("Path"), NULL, NULL, (LPBYTE)crr_path, &(path_size = MAXPATH)) == ERROR_SUCCESS)
			ok = 1;
		// Если ничего не помогло - общий альтернативный способ
		else {
			ok = 0;
			if(RegOpenKeyEx(HKEY_CLASSES_ROOT,	_T(".rpt"), 0, KEY_QUERY_VALUE, &crr_key) == ERROR_SUCCESS &&
				RegQueryValueEx(crr_key, NULL, NULL, NULL, (LPBYTE)crr_name, &(crr_name_size = sizeof(crr_name))) == ERROR_SUCCESS) { // имя кристала для *.btr
				strcat(crr_name, "\\shell\\Open\\command");
				ok = 1;
			}
			if(ok && RegOpenKeyEx(HKEY_CLASSES_ROOT, crr_name, 0, KEY_QUERY_VALUE, &crr_key) == ERROR_SUCCESS &&
				RegQueryValueEx(crr_key, NULL, NULL, NULL, (LPBYTE)crr_path, &(path_size = MAXPATH)) == ERROR_SUCCESS) // @unicodeproblem
				ok = 1;
		}
		if(ok && fileExists(crr_path)) {
			SString alt_path;
			setLastSlash(crr_path);
			strcat(crr_path, "crw32.exe");
			THROW(GetAltPath(r_rec.Type, r_rec.Path, r_rec.StdName, alt_path));
			ok = spawnl(_P_NOWAIT, crr_path, (const char*)alt_path, (const char*)alt_path, 0);
		}
	}
	CATCHZOK
	if(crr_key)
		RegCloseKey(crr_key);
	return ok;
}

int SLAPI PPViewReport::CreateStdRptList(ReportViewItemArray * pList)
{
	int    ok = 1;
	int    close_file = 0;
	SCodepage _cp = cp866;
	uint   i  = 0;
	long   id = 0;
	SString sect;
	StringSet sections(";");
	PPIniFile * p_file = P_StdRptFile;
	THROW_INVARG(pList);
	if(!p_file) {
		SString filename;
		PPGetFilePath(PPPATH_BIN, PPFILNAM_STDRPT_INI, filename);
		p_file = new PPIniFile(filename);
	}
	THROW(p_file->GetSections(&sections));
	for(i = 0, id = 0; sections.get(&i, sect.Z()) > 0; id++) {
		if(sect.CmpNC(SystemSect) == 0) {
			int    icp = 0;
			p_file->GetIntParam(sect, CodepageParam, &icp);
			if(icp == 1251)
				_cp = cp1251;
			else
				_cp = cp866;
		}
		else {
			SString data, dt, descr;
			p_file->GetParam(sect, DataParam, data);
			if(data.C(0) == ';')
				data = 0;
			if(data.NotEmptyS()) {
				ReportViewItem item;
				MEMSZERO(item);
				p_file->GetParam(sect, ModifDtParam, dt);
				p_file->GetParam(sect, DescrParam,   descr);
				if(dt.C(0) == ';')
					dt = 0;
				if(descr.C(0) == ';')
					descr = 0;
				item.ID = id + 1;
				sect.CopyTo(item.StdName, sizeof(item.StdName));
				data.CopyTo(item.StrucName, sizeof(item.StrucName));
				strtodate((const char*)dt, DATF_DMY, &item.ModifDt);
				if(_cp == cp866)
					descr.Transf(CTRANSF_OUTER_TO_INNER);
				descr.CopyTo(item.Descr, sizeof(item.Descr));
				item.Type = ReportFilt::rpttStandart;
				THROW_SL(pList->insert(&item));
			}
		}
	}
	CATCHZOK
	if(close_file)
		ZDELETE(p_file);
	return ok;
}

int SLAPI PPViewReport::SplitLocalRptStr(PPIniFile * pFile, int codepage, SString & rSect, SString & rBuf, ReportViewItem * pItem)
{
	int    ok = -1;
	long   id = 0;
	ReportViewItem item;
	uint   k = 0;
	SString par, val;
	StringSet ss("=");
	MEMSZERO(item);
	THROW_INVARG(pFile);
	ss.setBuf(rBuf, rBuf.Len() + 1);
	ss.get(&k, par);
	ss.get(&k, val);
	if(par.C(0) != ';' && par.CmpNC(FmtParam) != 0 && par.CmpNC(DestParam) != 0 && par.CmpNC(SilentParam) != 0) {
		SString descr, struc;
		StringSet ss1(";");
		ss1.setBuf(val, val.Len() + 1);
		ss1.get(&(k = 0), descr);
		ss1.get(&k, struc);
		if(descr.Len()) {
			item.ID = id + 1;
			rSect.CopyTo(item.StdName, sizeof(item.StdName));
			if(codepage == 866)
				descr.Transf(CTRANSF_OUTER_TO_INNER);
			descr.CopyTo(item.Descr, sizeof(item.Descr));
			if(!par.IsEqiAscii("std")) {
				par.CopyTo(item.Path, sizeof(item.Path));
				item.Type = ReportFilt::rpttLocal;
			}
			else
				item.Type = ReportFilt::rpttStandart;
			if(!struc.Len())
				pFile->GetParam(item.StdName, DataParam, struc);
			struc.CopyTo(item.StrucName, sizeof(item.StrucName));
			ok = 1;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pItem, item);
	return ok;
}

int SLAPI PPViewReport::CreateRptList(ReportViewItemArray * pList)
{
	int    ok = 1, close_file = 0, codepage = 0;
	uint   i  = 0;
	long   id = 0;
	PPIniFile * p_file = P_RptFile;
	SString sect;
	StringSet sections(";");
	THROW_INVARG(pList);
	if(!p_file) {
		SString filename;
		PPGetFilePath(PPPATH_BIN, (uint)PPFILNAM_REPORT_INI, filename);
		p_file = new PPIniFile(filename);
		close_file = 1;
	}

	THROW(p_file->GetSections(&sections));
	for(i = 0, id = 0; sections.get(&i, sect.Z()) > 0; id++) {
		if(sect.CmpNC(SystemSect) == 0) {
			p_file->GetIntParam(sect, CodepageParam, &codepage);
			LocalRptCodepage = (LocalRptCodepage == 0) ? 866 : LocalRptCodepage;
		}
		else {
			ReportViewItem item;
			SString   entry;
			StringSet entries("\n");
			p_file->GetEntries(sect, &entries, 1);
			for(uint j = 0; entries.get(&j, entry) > 0;) {
				if(SplitLocalRptStr(p_file, LocalRptCodepage, sect, entry, &item) > 0)
					THROW_SL(pList->insert(&item));
			}
		}
	}
	CATCHZOK
	if(close_file)
		ZDELETE(p_file);
	return ok;
}

// virtual
DBQuery * SLAPI PPViewReport::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	int ok = 1;
	uint brw_id = BROWSER_REPORT;
	DBE dbe_type;
	DBQ * dbq = 0;
	DBQuery * q = 0;
	TempReportTbl * p_tbl = 0;
	THROW(P_TempTbl);
	THROW(CheckTblPtr(p_tbl = new TempReportTbl(P_TempTbl->GetName())));
	PPDbqFuncPool::InitLongFunc(dbe_type,   PPDbqFuncPool::IdReportTypeName, p_tbl->Type);
	q = & select(p_tbl->ID, 0L);                           // #00
	q->addField(p_tbl->StdName);                           // #01
	q->addField(p_tbl->ModifDt);                           // #02
	q->addField(p_tbl->Descr);                             // #03
	q->addField(p_tbl->StrucName);                         // #04
	q->addField(dbe_type);                                 // #05
	q->addField(p_tbl->Path);                              // #06
	q->from(p_tbl, 0L);
	if(Filt.Order == ReportFilt::ordByStruc)
		q->orderBy(p_tbl->StrucName, 0L);
	else if(Filt.Order == ReportFilt::ordByType)
		q->orderBy(p_tbl->Type, 0L);
	else
		q->orderBy(p_tbl->StdName, 0L);
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete p_tbl;
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

class ReportDlg : public TDialog {
public:
	explicit ReportDlg(PPViewReport * pView) : TDialog(DLG_REPORT)
	{
		SetupCalDate(CTLCAL_REPORT_MODIFDATE, CTL_REPORT_MODIFDATE);
		PPWait(1);
		if(pView) {
			ReportViewItemArray _list;
			if(pView->CreateStdRptList(&_list) > 0 && pView->CreateRptList(&_list)) {
				_list.sort(PTR_CMPFUNC(REPORTNAME));
				for(uint i = 0; i < _list.getCount(); i++) {
					ReportViewItem & r_item = _list.at(i);
					RptList.Add(i + 1, r_item.StdName);
					if(sstrlen(r_item.StrucName) && StrucList.SearchByText(r_item.StrucName, 1, 0) <= 0)
						StrucList.Add(i + 1, r_item.StrucName);
				}
			}
		}
		PPWait(0);
	}
	int setDTS(const ReportViewItem * pData);
	int getDTS(ReportViewItem * pData);
private:
	ReportViewItem  Data;
	StrAssocArray   RptList;
	StrAssocArray   StrucList;
};

int ReportDlg::setDTS(const ReportViewItem * pData)
{
	uint   pos = 0;
	long   _id = 0;
	if(!RVALUEPTR(Data, pData))
		MEMSZERO(Data);
	setCtrlData(CTL_REPORT_MODIFDATE, &Data.ModifDt);
	if(RptList.SearchByText(Data.StdName, 1, &pos) > 0)
		_id = RptList.Get(pos).Id;
	SetupStrAssocCombo(this, CTLSEL_REPORT_STDNAME, &RptList, _id,   0);
	if(StrucList.SearchByText(Data.StrucName, 1, &pos) > 0)
		_id = StrucList.Get(pos).Id;
	else
		_id = 0;
	SetupStrAssocCombo(this, CTLSEL_REPORT_STRUCNAME, &StrucList,   _id, 0);
	setCtrlData(CTL_REPORT_PATH,  Data.Path);
	setCtrlData(CTL_REPORT_DESCR, Data.Descr);
	AddClusterAssocDef(CTL_REPORT_TYPE, 0, ReportFilt::rpttStandart);
	AddClusterAssoc(CTL_REPORT_TYPE, 1, ReportFilt::rpttLocal);
	SetClusterData(CTL_REPORT_TYPE, Data.Type);
	disableCtrls(Data.Type == ReportFilt::rpttStandart, CTL_REPORT_MODIFDATE, CTLSEL_REPORT_STDNAME, CTLSEL_REPORT_STRUCNAME, CTL_REPORT_PATH, CTL_REPORT_DESCR, 0L);
	disableCtrls(1, CTL_REPORT_MODIFDATE, CTL_REPORT_TYPE, 0L);
	return 1;
}

int ReportDlg::getDTS(ReportViewItem * pData)
{
	int    ok = -1;
	uint   sel = 0;
	long   _id = 0;
	SString buf;
	GetClusterData(CTL_REPORT_TYPE, &Data.Type);
	THROW_PP(_id = getCtrlLong(sel = CTLSEL_REPORT_STDNAME), PPERR_INVRPTSTDNAME);
	RptList.GetText(_id, buf);
	buf.CopyTo(Data.StdName, sizeof(Data.StdName));
	THROW_PP(_id = getCtrlLong(sel = CTLSEL_REPORT_STRUCNAME), PPERR_INVRPTSTRUCNAME);
	StrucList.GetText(_id, buf);
	buf.CopyTo(Data.StrucName, sizeof(Data.StrucName));
	getCtrlData(CTL_REPORT_MODIFDATE, &Data.ModifDt);
	if(Data.Type == ReportFilt::rpttLocal) {
		SString path;
		SPathStruc sp;
		getCtrlData(sel = CTL_REPORT_PATH,  Data.Path);
		THROW_PP(sstrlen(Data.Path), PPERR_USERINPUT);
		sp.Split(Data.Path);
		if(!sp.Drv.NotEmptyS()) {
			PPGetPath(PPPATH_BIN, path);
			path.SetLastSlash().Cat(Data.Path);
		}
		else
			path.CopyFrom(Data.Path);
		THROW_SL(fileExists(path));
	}
	getCtrlData(CTL_REPORT_DESCR, Data.Descr);
	ASSIGN_PTR(pData, Data);
	ok = 1;
	CATCH
		ok = (selectCtrl(sel), 0);
	ENDCATCH
	return ok;
}

int SLAPI PPViewReport::EditItem(long * pID)
{
	int ok = -1, valid_data = 0;
	long id = (pID) ? *pID : 0;
	ReportViewItem item, prev_item;
	ReportDlg * p_dlg = 0;
	if(id && P_TempTbl && P_TempTbl->search(0, &id, spEq) > 0) {
		item = *((ReportViewItem*)&P_TempTbl->data);
		prev_item = item;
	}
	else {
		id = 0;
		MEMSZERO(item);
		MEMSZERO(item);
		item.Type = ReportFilt::rpttLocal;
	}
	THROW(CheckDialogPtr(&(p_dlg = new ReportDlg(this))));
	p_dlg->setDTS(&item);
	while(!valid_data && ExecView(p_dlg) == cmOK) {
		if(p_dlg->getDTS(&item) > 0)
			valid_data = ok = 1;
		else
			PPError();
	}
	if(ok > 0 && item.Type == ReportFilt::rpttLocal && P_RptFile) {
		SString val;
		TempReportTbl::Rec temp_rec;
		MEMSZERO(temp_rec);
		(val = item.Descr).Semicol().Cat(item.StrucName);
		if(LocalRptCodepage != 866)
			val.Transf(CTRANSF_INNER_TO_OUTER);
		P_RptFile->RemoveParam(prev_item.StdName, prev_item.Path);
		P_RptFile->AppendParam(item.StdName, item.Path, val, 1);
		if(CheckForFilt(&item) > 0) {
			MakeTempRec(&item, &temp_rec);
			if(id) {
				THROW_DB(UpdateByID(P_TempTbl, 0, id, &temp_rec, 0));
			}
			else
				THROW_DB(AddByID(P_TempTbl, &id, &temp_rec, 0));
		}
	}
	else
		ok = -1;
	CATCH
		ok = (PPError(), 0);
	ENDCATCH
	delete p_dlg;
	ASSIGN_PTR(pID, id);
	return ok;
}

int SLAPI PPViewReport::DelItem(long id)
{
	int    ok = -1;
	if(P_TempTbl && P_TempTbl->search(0, &id, spEq) > 0) {
		TempReportTbl::Rec & r_rec = P_TempTbl->data;
		if(P_RptFile && r_rec.Type == ReportFilt::rpttLocal && CONFIRM(PPCFM_DELREPORTSECTION) && P_RptFile->RemoveSection(r_rec.StdName) > 0) {
			P_TempTbl->deleteRec();
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPViewReport::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		uint pos = 0;
		BrwHdr hdr;
		if(pHdr)
			hdr = *(PPViewReport::BrwHdr *)pHdr;
		else
			MEMSZERO(hdr);
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				hdr.ID = 0;
				// @fallthrough
			case PPVCMD_EDITITEM: ok = EditItem(&hdr.ID); break;
			case PPVCMD_DELETEITEM: ok = DelItem(hdr.ID); break;
			case PPVCMD_CALLCRR: ok = CallCR(hdr.ID); break;
			case PPVCMD_VERIFY: ok = Verify(hdr.ID); break;
			case PPVCMD_MAILRPT: ok = SendMail(hdr.ID); break;
		}
	}
	if(!ok)
		PPError();
	return ok;
}
//
// Implementation of PPALDD_ReportInfo
//
PPALDD_CONSTRUCTOR(RptInfo)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(RptInfo) { Destroy(); }

int PPALDD_RptInfo::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Report, rsrv);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_RptInfo::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(Report);
}

int PPALDD_RptInfo::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Report);
	I.ModifDt = item.ModifDt;
	STRNSCPY(I.StdName, item.StdName);
	STRNSCPY(I.AltName, item.Path);
	STRNSCPY(I.Descr, item.Descr);
	STRNSCPY(I.DataStruc, item.StrucName);
	I.Type = item.Type;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_RptInfo::Destroy()
{
	DESTROY_PPVIEW_ALDD(Report);
}
