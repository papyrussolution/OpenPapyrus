// V_REPORT.CPP
// Copyright (c) A.Starodub 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <crpe.h>

int SaveDataStruct(const char *pDataName, const char *pTempPath, const char *pRepFileName); // Prototype (PPREPORT.CPP)
//
// PPViewReport
//
IMPLEMENT_PPFILT_FACTORY(Report); ReportFilt::ReportFilt() : PPBaseFilt(PPFILT_REPORT, 0, 0)
{
	SetFlatChunk(offsetof(ReportFilt, ReserveStart), offsetof(ReportFilt, StdName) - offsetof(ReportFilt, ReserveStart));
	SetBranchSString(offsetof(ReportFilt, StdName));
	SetBranchSString(offsetof(ReportFilt, StrucName));
	Init(1, 0);
}

PPViewReport::PPViewReport() : PPView(0, &Filt, PPVIEW_REPORT, 0, REPORT_RPTINFO), P_StdRptFile(0), P_RptFile(0), P_TempTbl(0), LocalRptCodepage(866)
{
	PPIniFile::GetSectSymb(PPINISECT_SYSTEM, SystemSect);
	PPIniFile::GetParamSymb(  PPINIPARAM_CODEPAGE,           CodepageParam);
	PPIniFile::GetParamSymb(  PPINIPARAM_REPORT_FORMAT,      FmtParam);
	PPIniFile::GetParamSymb(  PPINIPARAM_REPORT_DESTINATION, DestParam);
	PPIniFile::GetParamSymb(  PPINIPARAM_REPORT_SILENT,      SilentParam);
	PPIniFile::GetParamSymb(  PPINIPARAM_REPORT_DATA,        DataParam);
	PPIniFile::GetParamSymb(  PPINIPARAM_REPORT_DESCRIPTION, DescrParam);
	PPIniFile::GetParamSymb(  PPINIPARAM_REPORT_MODIFDATE,   ModifDtParam);
}

PPViewReport::~PPViewReport()
{
	SaveChanges(1);
}

PPBaseFilt * PPViewReport::CreateFilt(void * extraPtr) const
{
	ReportFilt * p_filt = new ReportFilt;
	return p_filt;
}

int PPViewReport::SaveChanges(int remove)
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
		SCopyFile(P_RptFile->GetFileName(), PPGetFilePathS(PPPATH_BIN, PPFILNAM_REPORT_INI, fname), 0, 0, 0);
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

/*virtual*/int PPViewReport::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	SString fname, temp_fname;
	PPViewBrowser * p_prev_win = static_cast<PPViewBrowser *>(PPFindLastBrowser());
	THROW(Helper_InitBaseFilt(pFilt));
	if(p_prev_win && p_prev_win->P_View)
		static_cast<PPViewReport *>(p_prev_win->P_View)->SaveChanges(0);
	else
		SaveChanges(1);
	SCopyFile(PPGetFilePathS(PPPATH_BIN, PPFILNAM_STDRPT_INI, fname), PPMakeTempFileName("stdrpt", "ini", 0, temp_fname), 0, 0, 0);
	THROW_MEM(P_StdRptFile = new PPIniFile(temp_fname, 0, 0, 1));
	SCopyFile(PPGetFilePathS(PPPATH_BIN, PPFILNAM_REPORT_INI, fname), PPMakeTempFileName("rpt", "ini", 0, temp_fname), 0, 0, 0);
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
					// @v10.7.3 @ctr MEMSZERO(temp_rec);
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
	const ReportViewItem * p_i1 = static_cast<const ReportViewItem *>(i1);
	const ReportViewItem * p_i2 = static_cast<const ReportViewItem *>(i2);
	return stricmp866(p_i1->StdName, p_i2->StdName);
}

class ReportFiltDlg : public TDialog {
public:
	explicit ReportFiltDlg(PPViewReport * pView) : TDialog(DLG_REPORTFLT)
	{
		PPWaitStart();
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
		PPWaitStop();
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

/*virtual*/int PPViewReport::EditBaseFilt(PPBaseFilt * pFilt)
{
	DIALOG_PROC_BODY_P1ERR(ReportFiltDlg, this, static_cast<ReportFilt *>(pFilt));
}

int PPViewReport::InitIteration()
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
		ASSIGN_PTR(pItem, *static_cast<const ReportViewItem *>(&P_TempTbl->data));
		Counter.Increment();
		ok = 1;
	}
	return ok;
}

int PPViewReport::SendMail(long id)
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
		DECL_DIALOG_DATA(ReportMailDialog::Rec);

		ReportMailDialog() : TDialog(DLG_RPTMAIL)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			if(!RVALUEPTR(Data, pData)) {
				Data.AccountID = 0;
				Data.SupportMail.Z();
				Data.MainOrg.Z();
				Data.Licence.Z();
				Data.User.Z();
				Data.DB.Z();
				Data.RptPath.Z();
				Data.Struc.Z();
				Data.Dtm.Z();
			}
			SString buf;
			SetupPPObjCombo(this, CTLSEL_RPTMAIL_ACCNT, PPOBJ_INTERNETACCOUNT, Data.AccountID, 0, 
				reinterpret_cast<void *>(PPObjInternetAccount::filtfMail)/*INETACCT_ONLYMAIL*/);
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
		DECL_DIALOG_GETDTS()
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
	};
	int    ok = -1, valid_data = 0;
	ReportMailDialog * p_dlg = 0;
	if(id && P_TempTbl && P_TempTbl->search(0, &id, spEq) > 0) {
		ReportMailDialog::Rec data;
		PPIniFile ini_file;
		PPAlbatrossConfig alb_cfg;
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
				data.DB.Z();
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
					PPWaitStart();
					THROW(SendMailWithAttach("Report info", data.RptPath, msg, data.SupportMail, data.AccountID));
					PPWaitStop();
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

int PPViewReport::CheckForFilt(const ReportViewItem * pItem)
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

void PPViewReport::MakeTempRec(const ReportViewItem * pItem, TempReportTbl::Rec * pTempRec)
{
	if(pItem && pTempRec)
		*pTempRec = *static_cast<const TempReportTbl::Rec *>(pItem);
}

int PPViewReport::GetAltPath(long type, const char * pPath, const char * pStdName, SString & rPath)
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

int PPViewReport::Verify(long id)
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

int PPViewReport::CallCR(long id)
{
	int    ok = -1;
	HKEY   crr_key = 0;
	if(id && P_TempTbl && P_TempTbl->search(0, &id, spEq) > 0) {
		/*
		{
			char   crr_path[MAXPATH];
			DWORD  path_size = MAXPATH;
			memzero(crr_path, sizeof(crr_path));
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
				TCHAR   crr_name[64];
				DWORD   crr_name_size = SIZEOFARRAY(crr_name);
				PTR32(crr_name)[0] = 0;
				if(RegOpenKeyEx(HKEY_CLASSES_ROOT,	_T(".rpt"), 0, KEY_QUERY_VALUE, &crr_key) == ERROR_SUCCESS &&
					RegQueryValueEx(crr_key, NULL, NULL, NULL, (LPBYTE)crr_name, &(crr_name_size = sizeof(crr_name))) == ERROR_SUCCESS) { // имя кристала для *.btr
					strcat(crr_name, "\\shell\\Open\\command");
					ok = 1;
				}
				if(ok && RegOpenKeyEx(HKEY_CLASSES_ROOT, crr_name, 0, KEY_QUERY_VALUE, &crr_key) == ERROR_SUCCESS &&
					RegQueryValueEx(crr_key, NULL, NULL, NULL, (LPBYTE)crr_path, &(path_size = MAXPATH)) == ERROR_SUCCESS) // @unicodeproblem
					ok = 1;
			}
		}
		*/
		{
			SString crr_path;
			{
				WinRegKey reg_key(HKEY_LOCAL_MACHINE, "SOFTWARE\\Seagate Software\\Crystal Reports", 1);
				if(reg_key.GetString("Path", crr_path) > 0)
					ok = 1;
			}
			if(ok < 0) {
				WinRegKey reg_key(HKEY_LOCAL_MACHINE, "SOFTWARE\\Crystal Decisions\\10.0\\Crystal Reports", 1);
				if(reg_key.GetString("Path", crr_path) > 0)
					ok = 1;
			}
			if(ok < 0) {
				SString crr_name;
				WinRegKey reg_key(HKEY_CLASSES_ROOT, ".rpt", 1);
				if(reg_key.GetString(0, crr_name) > 0) {
					crr_name.Cat("\\shell\\Open\\command");
					WinRegKey reg_key_path(HKEY_CLASSES_ROOT, crr_name, 1);
					if(reg_key_path.GetString(0, crr_path) > 0) {
						ok = 1;
					}
				}
			}
			if(ok > 0 && fileExists(crr_path)) {
				SString alt_path;
				const TempReportTbl::Rec & r_rec = P_TempTbl->data;
				crr_path.SetLastSlash().Cat("crw32.exe");
				//strcat(crr_path, "crw32.exe");
				THROW(GetAltPath(r_rec.Type, r_rec.Path, r_rec.StdName, alt_path));
				ok = spawnl(_P_NOWAIT, crr_path, alt_path.cptr(), alt_path.cptr(), 0);
			}
		}
	}
	CATCHZOK
	if(crr_key)
		RegCloseKey(crr_key);
	return ok;
}

int PPViewReport::CreateStdRptList(ReportViewItemArray * pList)
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
		p_file = new PPIniFile(PPGetFilePathS(PPPATH_BIN, PPFILNAM_STDRPT_INI, filename));
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
				// @v10.7.3 @ctr MEMSZERO(item);
				p_file->GetParam(sect, ModifDtParam, dt);
				p_file->GetParam(sect, DescrParam,   descr);
				if(dt.C(0) == ';')
					dt = 0;
				if(descr.C(0) == ';')
					descr = 0;
				item.ID = id + 1;
				sect.CopyTo(item.StdName, sizeof(item.StdName));
				data.CopyTo(item.StrucName, sizeof(item.StrucName));
				strtodate(dt.cptr(), DATF_DMY, &item.ModifDt);
				if(oneof2(_cp, cp1251, cpANSI)) // @v10.3.11 @fix cp866-->(cp1251, cpANSI)
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

int PPViewReport::SplitLocalRptStr(PPIniFile * pFile, int codepage, const SString & rSect, SString & rBuf, ReportViewItem * pItem)
{
	int    ok = -1;
	long   id = 0;
	ReportViewItem item;
	uint   k = 0;
	SString par, val;
	StringSet ss("=");
	// @v10.7.3 @ctr MEMSZERO(item);
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

int PPViewReport::CreateRptList(ReportViewItemArray * pList)
{
	int    ok = 1;
	int    do_close_file = 0;
	int    codepage = 0;
	uint   i  = 0;
	long   id = 0;
	PPIniFile * p_file = P_RptFile;
	SString sect;
	StringSet sections(";");
	THROW_INVARG(pList);
	if(!p_file) {
		SString filename;
		p_file = new PPIniFile(PPGetFilePathS(PPPATH_BIN, PPFILNAM_REPORT_INI, filename));
		do_close_file = 1;
	}
	THROW(p_file->GetSections(&sections));
	for(i = 0, id = 0; sections.get(&i, sect.Z()) > 0; id++) {
		if(sect.CmpNC(SystemSect) == 0) {
			p_file->GetIntParam(sect, CodepageParam, &codepage);
			SETIFZ(LocalRptCodepage, 866);
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
	if(do_close_file)
		ZDELETE(p_file);
	return ok;
}

/*virtual*/DBQuery * PPViewReport::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	int    ok = 1;
	uint brw_id = BROWSER_REPORT;
	DBE dbe_type;
	DBQ * dbq = 0;
	DBQuery * q = 0;
	TempReportTbl * p_tbl = 0;
	THROW(P_TempTbl);
	THROW(CheckTblPtr(p_tbl = new TempReportTbl(P_TempTbl->GetName())));
	PPDbqFuncPool::InitLongFunc(dbe_type, PPDbqFuncPool::IdReportTypeName, p_tbl->Type);
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
	DECL_DIALOG_DATA(ReportViewItem);
public:
	explicit ReportDlg(PPViewReport * pView) : TDialog(DLG_REPORT)
	{
		SetupCalDate(CTLCAL_REPORT_MODIFDATE, CTL_REPORT_MODIFDATE);
		PPWaitStart();
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
		PPWaitStop();
	}
	DECL_DIALOG_SETDTS()
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
	DECL_DIALOG_GETDTS()
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
private:
	StrAssocArray   RptList;
	StrAssocArray   StrucList;
};

int PPViewReport::EditItem(long * pID)
{
	int    ok = -1;
	int    valid_data = 0;
	long   id = DEREFPTRORZ(pID);
	ReportViewItem item;
	ReportViewItem prev_item;
	ReportDlg * p_dlg = 0;
	if(id && P_TempTbl && P_TempTbl->search(0, &id, spEq) > 0) {
		item = *static_cast<const ReportViewItem *>(&P_TempTbl->data);
		prev_item = item;
	}
	else {
		id = 0;
		// @v10.7.3 @ctr MEMSZERO(item);
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
		// @v10.7.3 @ctr MEMSZERO(temp_rec);
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

int PPViewReport::DelItem(long id)
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

int PPViewReport::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		uint pos = 0;
		BrwHdr hdr;
		if(pHdr)
			hdr = *static_cast<const PPViewReport::BrwHdr *>(pHdr);
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

void PPALDD_RptInfo::Destroy() { DESTROY_PPVIEW_ALDD(Report); }
