// V_LOG.CPP
// Copyright (c) Starodub A. 2006, 2007, 2008, 2009, 2012, 2013, 2014, 2016, 2017, 2020
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
// @v10.9.3 #include <process.h>

LogFileEntry::LogFileEntry() : ID(0)
{
	PTR32(LogName)[0] = 0;
	PTR32(FileName)[0] = 0;
}

class LogsDialog : public PPListDialog {
public:
	LogsDialog() : PPListDialog(DLG_LOGS, CTL_LOGS_LIST)
	{
		SString buf;
		PPLoadText(PPTXT_LOGSNAMES, buf);
		StringSet ss(';', buf);
		for(uint i = 0; ss.get(&i, buf);) {
			uint j = 0;
			LogFileEntry e;
			// @v10.7.8 @ctr MEMSZERO(e);
			StringSet ss1(',', buf);
			ss1.get(&j, buf.Z());
			e.ID = buf.ToLong();
			ss1.get(&j, buf.Z());
			buf.CopyTo(e.LogName, sizeof(e.LogName));
			ss1.get(&j, buf.Z());
			buf.CopyTo(e.FileName, sizeof(e.FileName));
			LogsAry.insert(&e);
		}
		updateList(-1);
	}
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int editItem(long pos, long id);
	int    SendByEmail();
	int    CreateVerHistLog(long id);

	LogsArray LogsAry;
};

IMPL_HANDLE_EVENT(LogsDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmSendByEmail)) {
		SendByEmail();
		clearEvent(event);
	}
}

/*virtual*/int LogsDialog::setupList()
{
	int    ok = 1;
	SString path;
	PPGetPath(PPPATH_LOG, path);
	path.SetLastSlash();
	for(uint i = 0; ok && i < LogsAry.getCount(); i++) {
		int64  fsize = 0;
		LogFileEntry & r_e = LogsAry.at(i);
		SString buf;
		SFile f;
		LDATETIME dtm;
		StringSet ss(SLBColumnDelim);
		ss.add(r_e.LogName);
		ss.add(r_e.FileName);
		(buf = path).Cat(r_e.FileName);
		CreateVerHistLog(r_e.ID - 1);
		f.Open(buf, SFile::mRead);
		f.CalcSize(&fsize);
		buf.Z().Cat((double)fsize / 1024, MKSFMTD(10, 1, 0));
		ss.add(buf);
		f.GetDateTime(0, 0, &dtm);
		buf.Z().Cat(dtm);
		ss.add(buf);
		if(!addStringToList(LogsAry.at(i).ID, ss.getBuf()))
			ok = 0;
	}
	return ok;
}

/*virtual*/int LogsDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < static_cast<long>(LogsAry.getCount())) {
		SString path;
		PPGetPath(PPPATH_LOG, path);
		path.SetLastSlash().Cat(LogsAry.at(pos).FileName);
		THROW(CreateVerHistLog(id - 1));
		SLibError = SLERR_FILENOTFOUND;
		THROW_PP_S(fileExists(path), PPERR_SLIB, path);
		SFile::WaitForWriteSharingRelease(path, 2000);
		ok = (int)::ShellExecute(0, _T("open"), SUcSwitch(path), NULL, NULL, SW_SHOWNORMAL);
	}
	CATCHZOKPPERR
	return ok;
}

int LogsDialog::SendByEmail()
{
	int    ok = -1;
	long   pos = 0, id = 0;
	if(getCurItem(&pos, &id) > 0) {
		PPID   acct_id = 0;
		SString path, support, subj, temp_buf, fmt_buf;
		UserInterfaceSettings uis;
		LogFileEntry & r_e = LogsAry.at(pos);
		PPAlbatrossConfig alb_cfg;
		PPIniFile ini_file;
		THROW(CreateVerHistLog(id - 1));
		THROW(ini_file.IsValid());
		ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_SUPPORTMAIL, support);
		uis.Restore();
		if(!support.Len())
			support.CopyFrom(uis.SupportMail);
		PPLoadText(PPTXT_INPUTEMAIL, temp_buf);
		PPInputStringDialogParam isd_param(temp_buf, temp_buf);
		if(InputStringDialog(&isd_param, support) > 0) {
			THROW_PP(support.Len() > 0, PPERR_SUPPORTMAILNOTDEF);
			uis.SupportMail = support;
			THROW(uis.Save());
			PPGetFilePath(PPPATH_LOG, r_e.FileName, path);
			THROW(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0);
			acct_id = alb_cfg.Hdr.MailAccID;
			if(ListBoxSelDialog(PPOBJ_INTERNETACCOUNT, &acct_id, reinterpret_cast<void *>(PPObjInternetAccount::filtfMail)) > 0) {
				GetMainOrgName(temp_buf);
				PPLoadText(PPTXT_LOGFILEMAILSUBJ, fmt_buf);
				subj.Printf(fmt_buf, r_e.FileName, temp_buf.cptr()).Transf(CTRANSF_INNER_TO_OUTER);
				PPWait(1);
				THROW(SendMailWithAttach(subj, path, 0, support, acct_id));
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int LogsDialog::CreateVerHistLog(long id)
{
	int    ok = 1;
	if(id == PPLOGNAM_VERHIST && CurDict) {
		SString db_path, log_path;
		PPGetPath(PPPATH_LOG, log_path);
		DBS.GetDbPath(DBS.GetDbPathID(), db_path);
		THROW(PPVerHistory::Log(db_path, log_path));
	}
	CATCHZOK
	return ok;
}

int ViewLogs()
{
	LogsDialog * p_dlg = 0;
	return CheckDialogPtrErr(&(p_dlg = new LogsDialog)) ? ExecViewAndDestroy(p_dlg) : 0;
}
