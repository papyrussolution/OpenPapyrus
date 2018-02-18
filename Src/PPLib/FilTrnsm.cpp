// FILTRNSM.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2005, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage windows-1251
// Передача объектов между разделами БД
//
#include <pp.h>
#pragma hdrstop
// @v9.6.2 (moved to pp.h) #include <ppidata.h>
//
// Descr: Удаляет или добавляет (в зависимости от remove) заголовок из файла pFile
//
int SLAPI RemovePPSHeader(const char * pFile, const PPObjectTransmit::Header * pHdr, int remove);
//
// Descr: Упаковывает или распаковывает файлы из pFileList в дириктории pDir
//   pack == 1 - упаковать, pack == 0 - распаковать. Упакованные и распакованные файлы
//   будут иметь одинаковые имена
//
static int SLAPI PackTransmitFiles(const /*PPFileNameArray*/SFileEntryPool * pFileList, int pack);
//
// ObjReceiveParamDialog
//
class ObjReceiveParamDialog : public TDialog {
public:
	ObjReceiveParamDialog(int editOptions) : TDialog(DLG_DBTRANSM_IN), EditOptions(editOptions)
	{
		if(!SetupStrListBox(this, CTL_DBTRANSM_IN_LIST))
			PPError();
	}
	int  setDTS(const ObjReceiveParam *);
	int  getDTS(ObjReceiveParam *);
private:
	DECL_HANDLE_EVENT;
	void   updateList();
	void   addItem();
	void   delItem();
	const  int EditOptions;
	ObjReceiveParam Data;
};

void ObjReceiveParamDialog::addItem()
{
	PPID   id = 0;
	ListToListData  ll_data(PPOBJ_DBDIV, 0, &Data.SenderDbDivList);
	ll_data.TitleStrID = PPTXT_SELECTDBDIV;
	if(!ListToListDialog(&ll_data))
		PPError();
	else {
		Data.SenderDbDivList.removeByID(LConfig.DBDiv);
		updateList();
	}
}

void ObjReceiveParamDialog::delItem()
{
	uint   p = 0;
	PPID   id;
	SmartListBox * p_list = (SmartListBox*)getCtrlView(CTL_DBTRANSM_IN_LIST);
	if(Data.SenderDbDivList.getCount() > 0 && p_list && p_list->getCurID(&id) &&
		Data.SenderDbDivList.lsearch(id, &p)) {
		Data.SenderDbDivList.atFree(p);
		updateList();
	}
}

void ObjReceiveParamDialog::updateList()
{
	SmartListBox * p_list = (SmartListBox*)getCtrlView(CTL_DBTRANSM_IN_LIST);
	if(p_list) {
		SString text;
		p_list->freeAll();
		if(Data.SenderDbDivList.getCount())
			for(uint i = 0; i < Data.SenderDbDivList.getCount(); i++) {
				PPID id = Data.SenderDbDivList.at(i);
				GetObjectName(PPOBJ_DBDIV, id, text, 0);
				THROW_SL(p_list->addItem(id, text));
			}
		else {
			PPLoadText(PPTXT_ALLDBDIV, text);
			THROW_SL(p_list->addItem(1, text));
		}
		p_list->Draw_();
	}
	CATCH
		PPError();
	ENDCATCH
}

IMPL_HANDLE_EVENT(ObjReceiveParamDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmaInsert))
		addItem();
	else if(event.isCmd(cmaDelete))
		delItem();
	else
		return;
	clearEvent(event);
}

int ObjReceiveParamDialog::setDTS(const ObjReceiveParam * pParam)
{
	ushort v = 0;
	if(!RVALUEPTR(Data, pParam))
		Data.Init();
	if(EditOptions) {
		if(Data.Flags & ObjReceiveParam::fGetFromOutSrcr)
			if(Data.Flags & ObjReceiveParam::fClearInpBefore)
				v = 0;
			else
				v = 1;
		else
			v = 2;
	}
	else {
		SString ss;
		setStaticText(CTL_DBTRANSM_IN_INFO1, 0);
		getStaticText(CTL_DBTRANSM_IN_INFO3, ss);
		setStaticText(CTL_DBTRANSM_IN_INFO2, ss);
		setStaticText(CTL_DBTRANSM_IN_INFO3, 0);
		v = 3;
	}
	setCtrlData(CTL_DBTRANSM_IN_OPTIONS, &v);
	disableCtrl(CTL_DBTRANSM_IN_OPTIONS, !EditOptions);
	AddClusterAssoc(CTL_DBTRANSM_FLAGS, 0, ObjReceiveParam::fClearInpAfter);
	AddClusterAssoc(CTL_DBTRANSM_FLAGS, 1, ObjReceiveParam::fCommitQueue);
	AddClusterAssoc(CTL_DBTRANSM_FLAGS, 2, ObjReceiveParam::fForceDestroyQueue);
	SetClusterData(CTL_DBTRANSM_FLAGS, Data.Flags);
	updateList();
	return 1;
}

int ObjReceiveParamDialog::getDTS(ObjReceiveParam * pParam)
{
	ushort v = 2;
	GetClusterData(CTL_DBTRANSM_FLAGS, &Data.Flags);
	if(EditOptions)
		getCtrlData(CTL_DBTRANSM_IN_OPTIONS, &v);
	if(v == 0)
		Data.Flags |= (ObjReceiveParam::fGetFromOutSrcr | ObjReceiveParam::fClearInpBefore);
	else if(v == 1) {
		Data.Flags |= ObjReceiveParam::fGetFromOutSrcr;
		Data.Flags &= ~ObjReceiveParam::fClearInpBefore;
	}
	else
		Data.Flags &= ~(ObjReceiveParam::fGetFromOutSrcr | ObjReceiveParam::fClearInpBefore);
	ASSIGN_PTR(pParam, Data);
	return 1;
}
//
//
//
int SLAPI EditObjReceiveParam(ObjReceiveParam * pParam, int editOptions) { DIALOG_PROC_BODY_P1(ObjReceiveParamDialog, editOptions, pParam); }

static int SLAPI IsEmailAddr(const char * pPath)
{
	return BIN(pPath && strchr(pPath, '@'));
}

static int SLAPI IsFtpAddr(const char * pPath)
{
	const char * p_ftp_prefx = "ftp:";
	return BIN(pPath && strnicmp(pPath, p_ftp_prefx, sstrlen(p_ftp_prefx)) == 0);
}

static int SLAPI IsRemovableDrive(const char drive)
{
	int    ok = 0;
	uint   drive_type = 0;
	char   str_drive[16];
	memzero(str_drive, sizeof(str_drive));
	sprintf(str_drive, "%c%c", drive, ':');
	setLastSlash(str_drive);
	drive_type = GetDriveType(str_drive); // @unicodeproblem
	if(oneof3(drive_type, DRIVE_REMOVABLE, DRIVE_CDROM, DRIVE_RAMDISK))
		ok = 1;
	else if(drive_type == DRIVE_NO_ROOT_DIR)
		ok = 0;
	else
		ok = -1;
	return ok;
}

static void RcvMailCallback(const IterCounter & bytesCounter, const IterCounter & msgCounter)
{
	SString msg;
	PPLoadText(PPTXT_RCVMAILWAITMSG, msg);
	if(msgCounter.GetTotal() > 1)
		msg.Space().Cat(msgCounter).CatChar('/').Cat(msgCounter.GetTotal());
	PPWaitPercent(bytesCounter, msg);
}

static void SendMailCallback(const IterCounter & bytesCounter, const IterCounter & msgCounter)
{
	SString msg;
	PPLoadText(PPTXT_SENDMAILWAITMSG, msg);
	if(msgCounter.GetTotal() > 1)
		msg.Space().Cat(msgCounter).CatChar('/').Cat(msgCounter.GetTotal());
	PPWaitPercent(bytesCounter, msg);
}

//typedef int (* SDataMoveProgressProc)(const SDataMoveProgressInfo *);

static int GetFilesFromMailServerProgressProc(const SDataMoveProgressInfo * pInfo)
{
	SString msg_buf;
	msg_buf.Cat(pInfo->OverallItemsDone).CatChar(':').Cat(pInfo->OverallSizeDone).CatChar('/').
		Cat(pInfo->OverallItemsCount).CatChar(':').Cat(pInfo->OverallSizeTotal);
	PPWaitMsg(msg_buf);
	return 0;
}

int SLAPI GetFilesFromMailServer2(PPID mailAccID, const char * pDestPath, long filtFlags, int clean, int deleMsg)
{
	int    ok = 1;
	SString temp_buf;
	SString enc_buf;
	PPID   mail_acc_id = mailAccID;
	PPObjInternetAccount mac_obj;
	PPInternetAccount mac_rec;
	if(mail_acc_id == 0) {
		PPAlbatrosConfig cfg;
		THROW(PPAlbatrosCfgMngr::Get(&cfg) > 0);
		mail_acc_id = cfg.Hdr.MailAccID;
	}
	THROW_PP(mail_acc_id, PPERR_UNDEFMAILACC);
	THROW_PP(mac_obj.Get(mail_acc_id, &mac_rec) > 0, PPERR_UNDEFMAILACC);
	PPWait(1);
	{
		InetUrl url;
		SUniformFileTransmParam uftp;
		uftp.DestPath = pDestPath;
		{
			mac_rec.GetExtField(MAEXSTR_RCVSERVER, temp_buf);
			url.SetComponent(url.cHost, temp_buf);
			url.SetProtocol((mac_rec.Flags & mac_rec.fUseSSL) ? InetUrl::protPOP3S : InetUrl::protPOP3);
			int    port = mac_rec.GetRcvPort();
			if(port)
				url.SetPort(port);
			mac_rec.GetExtField(MAEXSTR_RCVNAME, temp_buf);
			enc_buf.EncodeUrl(temp_buf, 0); // @v9.8.12
			url.SetComponent(url.cUserName, enc_buf);
			{
				char pw[128];
				mac_rec.GetPassword(pw, sizeof(pw), MAEXSTR_RCVPASSWORD);
				enc_buf.EncodeUrl(temp_buf = pw, 0); // @v9.8.12
				url.SetComponent(url.cPassword, enc_buf);
				memzero(pw, sizeof(pw));
				enc_buf.Obfuscate(); // @v9.8.12
				temp_buf.Obfuscate(); // @v9.8.12
			}
		}
		if(filtFlags & SMailMessage::fPpyObject) {
			temp_buf.EncodeUrl("/*.pps", 0);
			url.SetComponent(InetUrl::cPath, temp_buf);
			//
			temp_buf.EncodeUrl("subject=" SUBJECTDBDIV, 0);
			url.SetComponent(InetUrl::cQuery, temp_buf);
		}
		else if(filtFlags & SMailMessage::fPpyCharry) {
			temp_buf.EncodeUrl("/*.chy", 0);
			url.SetComponent(InetUrl::cPath, temp_buf);
			temp_buf.EncodeUrl("subject=" SUBJECTCHARRY, 0);
			url.SetComponent(InetUrl::cQuery, temp_buf);
		}
		else if(filtFlags & SMailMessage::fPpyOrder) {
			temp_buf.EncodeUrl("subject=" SUBJECTORDER, 0);
			url.SetComponent(InetUrl::cQuery, temp_buf);
		}
		url.Composite(0, temp_buf);
		uftp.SrcPath = temp_buf;
		if(deleMsg)
			uftp.Flags |= uftp.fDeleteAfter;
		//uftp.Format = SFileFormat::Jpeg;
		THROW_SL(uftp.Run(GetFilesFromMailServerProgressProc, 0));
		temp_buf = uftp.Reply;
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

/* @v9.9.0 (replaced with GetFilesFromMailServer2) {
int SLAPI GetFilesFromMailServer(PPID mailAccID, const char * pDestPath, long filtFlags, int clean, int deleMsg)
{
	int    ok = 1;
	uint   i;
	PPID   mail_acc_id = mailAccID;
	PPObjInternetAccount mac_obj;
	PPInternetAccount mac_rec;
	PPMailPop3 mail(0);
	PPIDArray msg_list;
	long   mailbox_count = 0, mailbox_size = 0;
	int    msg_n = 0;
	IterCounter msg_counter;
	SString wait_msg;
	SString temp_fname;
	SString dest_fname;
	SString temp_path;
	PPWait(1);
	PPGetPath(PPPATH_TEMP, temp_path);
	if(mail_acc_id == 0) {
		PPAlbatrosConfig cfg;
		THROW(PPAlbatrosCfgMngr::Get(&cfg) > 0);
		mail_acc_id = cfg.Hdr.MailAccID;
	}
	THROW_PP(mail_acc_id, PPERR_UNDEFMAILACC);
	THROW_PP(mac_obj.Get(mail_acc_id, &mac_rec) > 0, PPERR_UNDEFMAILACC);
	THROW(mail.Init(&mac_rec));
	mac_rec.GetExtField(MAEXSTR_RCVSERVER, wait_msg);
	PPWaitMsg(PPSTR_TEXT, PPTXT_WTMAILCONNECTION, wait_msg);
	THROW(mail.Connect());
	THROW(mail.Login());
	THROW(mail.GetStat(&mailbox_count, &mailbox_size));
	PPLoadText(PPTXT_CHECKINMAILFORPPY, wait_msg);
	for(msg_n = 1; msg_n <= mailbox_count; msg_n++) {
		if(filtFlags) {
			SMailMessage msg;
			const int gmi_r = mail.GetMsgInfo(msg_n, &msg);
			//THROW(mail.GetMsgInfo(msg_n, &msg));
			//if(mail.GetMsgInfo(msg_n, &msg) > 0)
			if(gmi_r > 0) {
				if((filtFlags & SMailMessage::fPpyOrder && msg.Flags & SMailMessage::fPpyOrder) ||
					(filtFlags & SMailMessage::fPpyObject && msg.Flags & SMailMessage::fPpyObject) ||
					(filtFlags & SMailMessage::fPpyCharry && msg.Flags & SMailMessage::fPpyCharry))
					msg_list.add(msg_n);
			}
			else {
				THROW(gmi_r);
			}
			PPWaitPercent(msg_n, mailbox_count, wait_msg);
		}
		else
			msg_list.add(msg_n);
		assert(MemHeapTracer::Check()); // @debug
	}
	if(clean) {
		if(filtFlags & SMailMessage::fPpyObject)
			PPRemoveFilesByExt(pDestPath, PPSEXT, 0, 0);
		if(filtFlags & SMailMessage::fPpyOrder)
			PPRemoveFilesByExt(pDestPath, ORDEXT, 0, 0);
		if(filtFlags & SMailMessage::fPpyCharry)
			PPRemoveFilesByExt(pDestPath, CHARRYEXT, 0, 0);
	}
	msg_counter.Init(msg_list.getCount());
	for(i = 0; i < msg_list.getCount(); i++) {
		SMailMessage msg;
		MakeTempFileName(temp_path, 0, "msg", 0, temp_fname.Z());
		msg_counter.Increment();
		THROW(mail.GetMsg(msg_list.at(i), &msg, temp_fname, RcvMailCallback, msg_counter));
		if(filtFlags & SMailMessage::fPpyObject && msg.Flags & SMailMessage::fPpyObject)
			THROW(mail.SaveAttachment(temp_fname, 0, pDestPath));
		if(filtFlags & SMailMessage::fPpyCharry && msg.Flags & SMailMessage::fPpyCharry)
			THROW(mail.SaveAttachment(temp_fname, 0, pDestPath));
		if(filtFlags & SMailMessage::fPpyOrder && msg.Flags & SMailMessage::fPpyOrder) {
			SPathStruc::ReplacePath(dest_fname = temp_fname, pDestPath, 1);
			SPathStruc::ReplaceExt(dest_fname, ORDEXT, 1);
			THROW(mail.SaveOrder(temp_fname, dest_fname));
		}
		if(deleMsg)
			THROW(mail.DeleteMsg(msg_list.at(i)));
	}
	PPWait(0);
	CATCHZOK
	return ok;
}
*/

/* @v9.8.11 int SLAPI Debug_GetFilesFromMessage(const char * pMsgFile)
{
	int    ok = 0;
	SString dest_path;
	PPGetPath(PPPATH_IN, dest_path);
	PPMailPop3 mail(0); //
	ok = mail.SaveAttachment(pMsgFile, 0, dest_path);
	return ok;
}*/

static int CallbackFTPTransfer(long count, long total, const char * pMsg, int)
{
	PPWaitPercent(count, total, pMsg);
	return 1;
}

int SLAPI GetFilesFromFtp(PPID ftpAccID, const char * pSrcDir, const char * pDestDir, long filtFlags, int clean, int delFromFtp, PPIDArray * pDbDivList)
{
	int    ok = 1;
	PPID   ftp_acc_id = ftpAccID;
	SString src_dir, dest_dir;
	StrAssocArray file_list;
	PPObjInternetAccount obj_acct;
	PPInternetAccount acct;
	WinInetFTP ftp;

	PPWait(1);
	if(ftp_acc_id == 0) {
		PPEquipConfig eq_cfg;
		THROW(ReadEquipConfig(&eq_cfg) > 0);
		ftp_acc_id = eq_cfg.FtpAcctID;
	}
	THROW_PP(ftp_acc_id, PPERR_FTPACCTUNDEF);
	THROW(obj_acct.Get(ftp_acc_id, &acct));

	(src_dir = pSrcDir).ReplaceStr("ftp:", "", 1).SetLastSlash();
	(dest_dir = pDestDir).SetLastSlash();

	THROW(ftp.Init());
	THROW(ftp.Connect(&acct));
	THROW(ftp.SafeGetFileList(src_dir, &file_list, 0, 0));
	if(clean) {
		if(filtFlags & SMailMessage::fPpyObject)
			PPRemoveFilesByExt(pDestDir, PPSEXT, 0, 0);
		if(filtFlags & SMailMessage::fPpyOrder)
			PPRemoveFilesByExt(pDestDir, ORDEXT, 0, 0);
		if(filtFlags & SMailMessage::fPpyCharry)
			PPRemoveFilesByExt(pDestDir, CHARRYEXT, 0, 0);
	}
	for(uint i = 0; i < file_list.getCount(); i++) {
		SString src_path, dest_path;
		SString    ext;
		StrAssocArray::Item file_item = file_list.Get(i);
		SPathStruc sp(file_item.Txt);
		ext.Dot().Cat(sp.Ext);
		(src_path = src_dir).Cat(file_item.Txt);
		(dest_path = dest_dir).Cat(file_item.Txt);
		if(((filtFlags & SMailMessage::fPpyObject) && ext.CmpNC(PPSEXT) == 0) ||
			((filtFlags & SMailMessage::fPpyCharry) && ext.CmpNC(ORDEXT) == 0) ||
			((filtFlags & SMailMessage::fPpyOrder) && ext.CmpNC(CHARRYEXT) == 0)) {
			THROW(ftp.SafeGet(dest_path, src_path, 0, CallbackFTPTransfer, 0));
			{
				int accept_file = 1;
				PPObjectTransmit::Header hdr;
				if(PPObjectTransmit::GetHeader(dest_path, &hdr)) {
					if(hdr.DestDBID != LConfig.DBDiv || pDbDivList && pDbDivList->getCount() && pDbDivList->lsearch(hdr.DBID, 0) <= 0)
						accept_file = 0;
				}
				if(accept_file && delFromFtp)
					THROW(ftp.SafeDelete(src_path, 0));
			}
		}
	}
	PPWait(0);
	CATCHZOK
	return ok;
}

int SLAPI PutFilesToEmail(const /*PPFileNameArray*/SFileEntryPool * pFileList, PPID mailAccID, const char * pDestAddr, const char * pSubj, long trnsmFlags)
{
	StringSet ss_file_list;
	SString file_path;
	if(pFileList) {
		//for(uint i = 0; pFileList->Enum(&i, 0, &file_path);) {
		for(uint i = 0; i < pFileList->GetCount(); i++) {
			if(pFileList->Get(i, 0, &file_path))
				ss_file_list.add(file_path);
		}
	}
	//return PutFilesToEmail((pFileList ? &ss_file_list : (const StringSet *)0), mailAccID, pDestAddr, pSubj, trnsmFlags);
	return PutFilesToEmail2((pFileList ? &ss_file_list : (const StringSet *)0), mailAccID, pDestAddr, pSubj, trnsmFlags);
}

#if 0 // @v9.8.11 {
int SLAPI PutFilesToEmail(const StringSet * pFileList, PPID mailAccID, const char * pDestAddr, const char * pSubj, long trnsmFlags)
{
	int    ok = 1;
	SString file_path, temp_buf;
	PPObjInternetAccount mac_obj;
	PPInternetAccount mac_rec;
	IterCounter msg_counter;
	PPID   mail_acc_id = mailAccID;
	SMailMessage msg;
	msg_counter.Init(1);
	PPWait(1);
	if(mail_acc_id == 0) {
		PPAlbatrosConfig cfg;
		THROW(PPAlbatrosCfgMngr::Get(&cfg) > 0);
		mail_acc_id = cfg.Hdr.MailAccID;
	}
	THROW_PP(mail_acc_id, PPERR_UNDEFMAILACC);
	THROW_PP(mac_obj.Get(mail_acc_id, &mac_rec) > 0, PPERR_UNDEFMAILACC);
	msg.SetField(SMailMessage::fldSubj, pSubj);
	mac_rec.GetExtField(MAEXSTR_FROMADDRESS, temp_buf);
	msg.SetField(SMailMessage::fldFrom, temp_buf.Strip());
	msg.SetField(SMailMessage::fldTo,   pDestAddr);
	if(pFileList) {
        for(uint i = 0; pFileList->get(&i, file_path);) {
			THROW(!::fileExists(file_path) || msg.AttachFile(file_path));
        }
	}
	mac_rec.GetExtField(MAEXSTR_SENDSERVER, temp_buf);
	PPWaitMsg(PPSTR_TEXT, PPTXT_WTMAILCONNECTION, temp_buf);
	THROW(PPMailSmtp::Send(mac_rec, msg, SendMailCallback, msg_counter));
	PPWait(0);
	if(trnsmFlags & TRNSMF_DELINFILES && pFileList) {
	    for(uint i = 0; pFileList->get(&i, file_path);)
			SFile::Remove(file_path);
	}
	CATCHZOK
	return ok;
}
#endif // } 0 @v9.8.11

//int SLAPI PPMailSmtp::Send(const PPInternetAccount & rAcc, SMailMessage & rMsg, MailCallbackProc cbProc, const IterCounter & rMsgCounter)
int SLAPI PPSendEmail(const PPInternetAccount & rAcc, SMailMessage & rMsg, MailCallbackProc cbProc, const IterCounter & rMsgCounter)
{
	int    ok = 1;
	SString temp_buf;
	SString enc_buf;
	InetUrl url;
	ScURL curl;
	{
		rAcc.GetExtField(MAEXSTR_SENDSERVER, temp_buf);
		url.SetComponent(url.cHost, temp_buf);
		url.SetProtocol((rAcc.Flags & rAcc.fUseSSL) ? InetUrl::protSMTPS : InetUrl::protSMTP);
		int    port = rAcc.GetSendPort();
		if(port)
			url.SetPort(port);
		rAcc.GetExtField(MAEXSTR_RCVNAME, temp_buf);
		enc_buf.EncodeUrl(temp_buf, 0); // @v9.8.12
		url.SetComponent(url.cUserName, enc_buf);
		{
			char pw[128];
			rAcc.GetPassword(pw, sizeof(pw), MAEXSTR_RCVPASSWORD);
			enc_buf.EncodeUrl(temp_buf = pw, 0); // @v9.8.12
			url.SetComponent(url.cPassword, enc_buf);
			memzero(pw, sizeof(pw));
			enc_buf.Obfuscate(); // @v9.8.12
			temp_buf.Obfuscate(); // @v9.8.12
		}
		THROW_SL(curl.SmtpSend(url, curl.mfVerbose|ScURL::mfDontVerifySslPeer, rMsg));
	}
	CATCHZOK
	return ok;
}

int SLAPI PutFilesToEmail2(const StringSet * pFileList, PPID mailAccID, const char * pDestAddr, const char * pSubj, long trnsmFlags)
{
	int    ok = 1;
	PPID   mail_acc_id = mailAccID;
	SString file_path;
	SString temp_buf;
	SString enc_buf;
	PPObjInternetAccount mac_obj;
	PPInternetAccount mac_rec;
	{
		if(mail_acc_id == 0) {
			PPAlbatrosConfig cfg;
			THROW(PPAlbatrosCfgMngr::Get(&cfg) > 0);
			mail_acc_id = cfg.Hdr.MailAccID;
		}
		THROW_PP(mail_acc_id, PPERR_UNDEFMAILACC);
		THROW_PP(mac_obj.Get(mail_acc_id, &mac_rec) > 0, PPERR_UNDEFMAILACC);
		{
			SMailMessage msg;
			msg.SetField(SMailMessage::fldSubj, pSubj);
			mac_rec.GetExtField(MAEXSTR_FROMADDRESS, temp_buf);
			msg.SetField(SMailMessage::fldFrom, temp_buf.Strip());
			msg.SetField(SMailMessage::fldTo,   pDestAddr);
			if(pFileList) {
				for(uint i = 0; pFileList->get(&i, file_path);) {
					THROW_SL(!::fileExists(file_path) || msg.AttachFile(0, SFileFormat::Unkn, file_path));
				}
			}
			mac_rec.GetExtField(MAEXSTR_SENDSERVER, temp_buf);
			{
				InetUrl url;
				ScURL curl;
				{
					mac_rec.GetExtField(MAEXSTR_SENDSERVER, temp_buf);
					url.SetComponent(url.cHost, temp_buf);
					url.SetProtocol((mac_rec.Flags & mac_rec.fUseSSL) ? InetUrl::protSMTPS : InetUrl::protSMTP);
					int    port = mac_rec.GetSendPort();
					if(port)
						url.SetPort(port);
					mac_rec.GetExtField(MAEXSTR_RCVNAME, temp_buf);
					enc_buf.EncodeUrl(temp_buf, 0); // @v9.8.12
					url.SetComponent(url.cUserName, enc_buf);
					{
						char pw[128];
						mac_rec.GetPassword(pw, sizeof(pw), MAEXSTR_RCVPASSWORD);
						enc_buf.EncodeUrl(temp_buf = pw, 0); // @v9.8.12
						url.SetComponent(url.cPassword, enc_buf);
						memzero(pw, sizeof(pw));
						enc_buf.Obfuscate(); // @v9.8.12
						temp_buf.Obfuscate(); // @v9.8.12
					}
					THROW_SL(curl.SmtpSend(url, curl.mfVerbose|ScURL::mfDontVerifySslPeer, msg));
					if(trnsmFlags & TRNSMF_DELINFILES && pFileList) {
						for(uint i = 0; pFileList->get(&i, file_path);)
							SFile::Remove(file_path);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

static int SLAPI PutFilesToFtp(const /*PPFileNameArray*/SFileEntryPool * pFileList, PPID ftpAccID, const char * pDestAddr, long trnsmFlags)
{
	int    ok = 1;
	uint   i;
	SString dest_dir;
	SString dest_path;
	SString file_path;
	SString file_name;
	SPathStruc ps;
	WinInetFTP ftp;
	PPObjInternetAccount obj_acct;
	PPInternetAccount acct;
	PPID   ftp_acc_id = ftpAccID;

	PPWait(1);
	if(ftp_acc_id == 0) {
		PPEquipConfig eq_cfg;
		THROW(ReadEquipConfig(&eq_cfg) > 0);
		ftp_acc_id = eq_cfg.FtpAcctID;
	}
	THROW_PP(ftp_acc_id, PPERR_FTPACCTUNDEF);
	THROW(obj_acct.Get(ftp_acc_id, &acct));
	(dest_dir = pDestAddr).ReplaceStr("ftp:", "", 1).SetLastSlash();
	THROW(ftp.Init());
	THROW(ftp.Connect(&acct));
	//for(i = 0; pFileList->Enum(&i, 0, &file_path);) {
	for(i = 0; i < pFileList->GetCount(); i++) {
		if(pFileList->Get(i, 0, &file_path)) {
			ps.Split(file_path);
			ps.Merge(SPathStruc::fNam|SPathStruc::fExt, file_name);
			(dest_path = dest_dir).Cat(file_name);
			THROW(ftp.SafePut(file_path, dest_path, 0, CallbackFTPTransfer, 0));
		}
	}
	PPWait(0);
	if(trnsmFlags & TRNSMF_DELINFILES)
		PPRemoveFiles(pFileList, 0, 0);
	CATCHZOK
	return ok;
}

int SLAPI GetTransmitFiles(ObjReceiveParam * pParam)
{
	int    ok = 1, user_accept = 1;
	const  PPConfig & r_cfg = LConfig;
	int    check_email = 0, check_ftp = 0;
	uint   i;
	SString dest, src, msg_buf, file_path;
	SString dest_dir;
	//PPFileNameArray fary;
	//SDirEntry fb;
	SFileEntryPool fep;
	SFileEntryPool::Entry fe;
	DBDivPack dbdiv_pack;
	PPObjDBDiv obj_dbdiv;
	PPDBDiv db_div_rec;
	PPID   db_div_id = 0;

	PPGetPath(PPPATH_IN, dest);
	THROW(obj_dbdiv.Get(r_cfg.DBDiv, &dbdiv_pack) > 0);
	while((!check_email || !check_ftp) && obj_dbdiv.EnumItems(&db_div_id, &db_div_rec) > 0) {
		if(db_div_id != r_cfg.DBDiv)
			check_email = BIN(check_email || IsEmailAddr(strip(db_div_rec.Addr)));
		check_ftp   = BIN(check_ftp || IsFtpAddr(db_div_rec.Addr));
	}
	(src = dbdiv_pack.Rec.Addr).Strip();
	if(pParam->Flags & ObjReceiveParam::fNonInteractive)
		user_accept = 1;
	else {
		pParam->Flags |= (ObjReceiveParam::fGetFromOutSrcr | ObjReceiveParam::fClearInpBefore);
		THROW(user_accept = EditObjReceiveParam(pParam, src.NotEmpty() || check_email || check_ftp));
	}
	if(user_accept < 0)
		ok = -1;
	else if(pParam->Flags & ObjReceiveParam::fGetFromOutSrcr) {
		int    use_src = 0, use_email = 0, use_ftp = 0;
		if(pParam->Flags & ObjReceiveParam::fClearInpBefore) {
			PPRemoveFilesByExt(dest, PPSEXT, 0, 0);
		}
		if(pParam->SenderDbDivList.getCount()) {
			for(i = 0; (!use_src || !use_email) && i < pParam->SenderDbDivList.getCount(); i++) {
				THROW(obj_dbdiv.Get(pParam->SenderDbDivList.at(i), &dbdiv_pack) > 0);
				if(IsEmailAddr(strip(dbdiv_pack.Rec.Addr)))
					use_email = 1;
				else
					use_src = 1;
			}
		}
		else
			use_src = use_email = use_ftp = 1;
		if(use_src && src.NotEmpty() && IsEmailAddr(src) == 0 && IsFtpAddr(src) == 0) {
			int    removable_drive = 0;
			char   drive = 'C', ext[10];
			fnsplit(src, &drive, 0, 0, ext);
			PPSetAddedMsgString(src);
			THROW_PP(driveValid(src), PPERR_NEXISTPATH);
			removable_drive = IsRemovableDrive(toupper(drive));
			if(removable_drive > 0) {
				file_path.Z().CatChar(drive).CatChar(':').CatChar('\\');
				PPSetAddedMsgString(msg_buf.Z().CatChar(toupper(drive)));
				while(::access(file_path, 0) != 0)
					THROW_PP(CONFIRM(PPCFM_INSERTDISK), PPERR_DRIVEUNAVELAIBLE);
			}
			PPWait(1);
			dest.RmvLastSlash();
			if(removable_drive != 1)
				src.RmvLastSlash();
			THROW_PP_S(::access(src, 0) == 0, PPERR_NEXISTPATH, src);
			if(::access(dest, 0) != 0) {
				PPSetAddedMsgString(dest);
				THROW_SL(createDir(dest));
			}
			//
			// copy files to PPPATH_IN
			//
			//THROW(fary.Scan(src, "*" PPSEXT));
			THROW(fep.Scan(src, "*" PPSEXT, 0));
			//for(i = 0; fary.Enum(&i, &fb, &file_path);) {
			for(i = 0; i < fep.GetCount(); i++) {
				if(fep.Get(i, &fe, &file_path)) {
					//char   dest_dir[MAXPATH];
					PPWaitMsg(fe.Name);
					//STRNSCPY(dest_dir, file_path);
					//::replacePath(dest_dir, dest, 1);
					SPathStruc::ReplacePath(dest_dir = file_path, dest, 1);
					if(SFile::WaitForWriteSharingRelease(file_path, 20000)) {
						THROW_SL(copyFileByName(file_path, dest_dir));
					}
				}
			}
		}
		assert(MemHeapTracer::Check()); // @debug
		if(use_email && check_email) {
			THROW(GetFilesFromMailServer2(0, dest, SMailMessage::fPpyObject, 0 /* don't clean */, 1 /* dele msg */));
		}
		if(IsFtpAddr(src) && check_ftp) {  // else if -> if Так как в обратном случае, если хоть один раздел получает данные с эл. почты, то в эту ветку никогда не попадем.
			THROW(GetFilesFromFtp(0, src, dest, SMailMessage::fPpyObject, 0 /* don't clean */, 1 /* dele from ftp */, &pParam->SenderDbDivList));
		}
		assert(MemHeapTracer::Check()); // @debug
	}
	// AHTOXA {
	// Распаковка принятых файлов
	//THROW(fary.Scan(dest, "*" PPSEXT));
	THROW(fep.Scan(dest, "*" PPSEXT, 0));
	{
		// Пропускаем файлы, заголовок которых не читается {
		//SDirEntry fb;
		LongArray to_remove_pos_list;
		//for(i = 0; fary.Enum(&i, &fb, &file_path);) {
		for(i = 0; i < fep.GetCount(); i++) {
			if(fep.Get(i, 0, &file_path)) {
				PPObjectTransmit::Header hdr;
				if(!PPObjectTransmit::GetHeader(file_path, &hdr)) {
					to_remove_pos_list.add(/*i-1*/i);
				}
			}
		}
		if(to_remove_pos_list.getCount()) {
			for(i = 0; i < to_remove_pos_list.getCount(); i++) {
				const uint fa_pos = (uint)to_remove_pos_list.get(i);
				//fary.atFree(fa_pos);
				fep.Remove(fa_pos);
			}
		}
		// }
	}
	THROW(PackTransmitFiles(/*&fary*/&fep, 0));
	// } AHTOXA
	CATCHZOK
	PPWait(0);
	return ok;
}

static int SLAPI PutFilesToDiskPath(const /*PPFileNameArray*/SFileEntryPool * pFileList, const char * pDestPath, long trnsmFlags)
{
	int    ok = 1;
	int    removable_drive = 0;
	//char   src_dir[MAXPATH];
	//char   dest_dir[MAXPATH];
	SString src_dir;
	SString dest_dir;
	char   msg_buf[128], ext[10], drive;
	SString file_path;
	uint   i;
	//SDirEntry fb;
	SFileEntryPool::Entry fe;

	fnsplit(pDestPath, &drive, 0, 0, ext);
	PPSetAddedMsgString(pDestPath);
	THROW_PP_S(driveValid(pDestPath), PPERR_NEXISTPATH, pDestPath);
	removable_drive = IsRemovableDrive(toupper(drive));
	if(removable_drive > 0) {
		//sprintf(dest_dir, "%c:\\", drive);
		dest_dir.Z().CatChar(drive).CatChar(':').CatChar('\\');
		msg_buf[0] = toupper(drive);
		msg_buf[1] = 0;
		PPSetAddedMsgString(msg_buf);
		while(::access(dest_dir, 0))
			THROW_PP(CONFIRM(PPCFM_INSERTDISK), PPERR_DRIVEUNAVELAIBLE);
	}
	//
	// Проверяем существование заданных каталогов
	//
	//rmvLastSlash(STRNSCPY(src_dir, pFileList->GetPath()));
	pFileList->GetInitPath(src_dir);
	src_dir.RmvLastSlash();
	if(removable_drive != 1) {
		//rmvLastSlash(STRNSCPY(dest_dir, pDestPath));
		(dest_dir = pDestPath).RmvLastSlash();
	}
	PPSetAddedMsgString(src_dir);
	THROW_PP(access(src_dir, 0) == 0, PPERR_NEXISTPATH);
	PPSetAddedMsgString(dest_dir);
	if(access(dest_dir, 0) != 0)
		THROW_SL(createDir(dest_dir));
	THROW_PP(::access(dest_dir, 2) == 0, PPERR_DIRACCESSDENIED_W);
	if(trnsmFlags & TRNSMF_DELOUTFILES)
		PPRemoveFilesByExt(pDestPath, PPSEXT, 0, 0);
	//
	// copy files to dest
	//
	PPWait(1);
	//for(i = 0; pFileList->Enum(&i, &fb, &file_path);) {
	for(i = 0; i < pFileList->GetCount(); i++) {
		if(pFileList->Get(i, &fe, &file_path)) {
			PPWaitMsg(fe.Name);
			//STRNSCPY(dest_dir, file_path);
			//replacePath(dest_dir, pDestPath, 1);
			SPathStruc::ReplacePath(dest_dir = file_path, pDestPath, 1);
			THROW_SL(copyFileByName(file_path, dest_dir));
		}
	}
	PPWait(0);
	if(trnsmFlags & TRNSMF_DELINFILES)
		PPRemoveFiles(pFileList, 0, 0);
	CATCHZOK
	return ok;
}

int SLAPI PutTransmitFiles(PPID dbDivID, long trnsmFlags)
{
	int    ok = 1;
	SString dest, src;
	DBDivPack dbdiv_pack;
	PPObjDBDiv obj_dbdiv;
	PPGetPath(PPPATH_OUT, src);
	THROW(obj_dbdiv.Get(dbDivID, &dbdiv_pack) > 0);
	if((dest = dbdiv_pack.Rec.Addr).NotEmptyS()) {
		//PPFileNameArray fary;
		SFileEntryPool fep;
		//THROW(fary.Scan(src, "*" PPSEXT));
		THROW(fep.Scan(src, "*" PPSEXT, 0));
		{
			//SDirEntry fb;
			SFileEntryPool::Entry fe;
			SString file_name;
			PPObjectTransmit ot(PPObjectTransmit::tmReading, 0, 0);
			uint i = fep.GetCount();
			if(i) do {
				if(fep.Get(--i, &fe, &file_name)) {
					PPObjectTransmit::Header hdr;
					if(!ot.GetHeader(file_name, &hdr) || hdr.DestDBID != dbDivID) {
						//
						// Если заголовок файла не удалось считать, то полагаем, что файл
						// либо инвалидный, либо не наш. То есть, его передавать не следует.
						// Не следует так же передавать файлы, не предназначенные для раздела dbDivID.
						//
						fep.Remove(i);
					}
				}
			} while(i);
		}
		// AHTOXA {
		// Упаковка файлов перед отправкой
		PPDBXchgConfig cfg;
		THROW(PPObjectTransmit::ReadConfig(&cfg));
		if(cfg.Flags & DBDXF_PACKFILES) {
			THROW(PackTransmitFiles(/*&fary*/&fep, 1));
		}
		// } AHTOXA
		if(IsEmailAddr(dest)) {
			THROW(PutFilesToEmail(/*&fary*/&fep, 0, dest, SUBJECTDBDIV, trnsmFlags));
		}
		else if(IsFtpAddr(dest)) {
			THROW(PutFilesToFtp(/*&fary*/&fep, 0, dest, trnsmFlags));
		}
		else {
			THROW(PutFilesToDiskPath(/*&fary*/&fep, dest, trnsmFlags));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

// AHTOXA {
int CallbackCompress(long, long, const char *, int);

int SLAPI RemovePPSHeader(const char * pFile, const PPObjectTransmit::Header * pHdr, int remove)
{
	int    ok = -1;
	//char   dest[MAXPATH];
	SString dest_file_name;
	FILE * p_in_stream = 0, *p_out_stream = 0;
	size_t read_bytes = 0, wr_bytes = 0, total_rd_bytes = 0, total_wr_bytes = 0;
	if(pFile && (pHdr || remove)) {
		char   buf[4096];
		//replaceExt(STRNSCPY(dest, pFile), "$$$", 1);
		dest_file_name = pFile;
		SPathStruc::ReplaceExt(dest_file_name, "$$$", 1);
		THROW_PP_S(p_in_stream = fopen(pFile, "rb"), PPERR_PPOSOPENFAULT, pFile);
		THROW_PP_S(p_out_stream = fopen(dest_file_name, "wb"), PPERR_PPOSOPENFAULT, dest_file_name);
		if(!remove) {
			wr_bytes = fwrite(pHdr, 1, sizeof(*pHdr), p_out_stream);
			total_wr_bytes += wr_bytes;
		}
		else {
			PPObjectTransmit::Header hdr;
			read_bytes = fread(&hdr, 1, sizeof(hdr), p_in_stream);
			total_rd_bytes += read_bytes;
		}
		while((read_bytes = fread(buf, 1, sizeof(buf), p_in_stream)) != 0) {
			wr_bytes = fwrite(buf, 1, read_bytes, p_out_stream);
			total_rd_bytes += read_bytes;
			total_wr_bytes += wr_bytes;
		}
		ok = 1;
	}
	CATCHZOK
	SFile::ZClose(&p_in_stream);
	SFile::ZClose(&p_out_stream);
	if(ok > 0) {
		SFile::Remove(pFile);
		SFile::Rename(dest_file_name, pFile);
	}
	else
		SFile::Remove(dest_file_name);
	return ok;
}

int SLAPI PackTransmitFile(const char * pFileName, int pack, PercentFunc callbackProc)
{
	int    ok = -1, rmv_temp_file = 0;
	SString dest_file;
	PPObjectTransmit::Header hdr;
	THROW(PPObjectTransmit::GetHeader(pFileName, &hdr));
	if((pack && !(hdr.Flags & PPOTF_ARC)) || (!pack && hdr.Flags & PPOTF_ARC)) {
		if(!pack) {
			// @dbd_exchange @v5.6.12 При следующем изменении формата убрать коммент со следующей строки
			// THROW_PP_S(hdr.CryptMethod == 0, PPERR_UNKNCRYPTMETHOD, pFileName);
			THROW_PP_S(hdr.PackMethod  == 1, PPERR_UNKNPACKMETHOD,  pFileName);
		}
		THROW(RemovePPSHeader(pFileName, 0, 1));
		dest_file = pFileName;
		SPathStruc::ReplaceExt(dest_file, "__", 1);
		rmv_temp_file = 1;
		THROW_SL(DoCompress(pFileName, dest_file, 0, pack, callbackProc));
		THROW_SL(SFile::Remove(pFileName));
		THROW_SL(SFile::Rename(dest_file, pFileName));
		rmv_temp_file = 0;
		if(pack) {
			hdr.Flags |= PPOTF_ARC;
			hdr.PackMethod = 1;
			hdr.CryptMethod = 0;
		}
		else {
			hdr.Flags &= ~PPOTF_ARC;
			hdr.PackMethod = 0;
			hdr.CryptMethod = 0;
		}
		THROW(RemovePPSHeader(pFileName, &hdr, 0));
		ok = 1;
	}
	CATCH
		ok = 0;
		if(rmv_temp_file && dest_file.NotEmptyS())
			SFile::Remove(dest_file);
	ENDCATCH
	return ok;
}

static int SLAPI PackTransmitFiles(const /*PPFileNameArray*/SFileEntryPool * pFileList, int pack)
{
#ifndef __CONFIG__
	int (* p_callback_proc)(long, long, const char *, int) = CallbackCompress;
#else
	int (* p_callback_proc)(long, long, const char *, int) = 0;
#endif //__CONFIG__
	int    ok = 1;
	SString file_path;
	//SDirEntry fb;
	SFileEntryPool::Entry fe;
	PPWait(1);
	//for(uint i = 0; pFileList->Enum(&i, &fb, &file_path);) {
	for(uint i = 0; i < pFileList->GetCount(); i++) {
		if(pFileList->Get(i, &fe, &file_path)) {
			THROW(PackTransmitFile(file_path, pack, p_callback_proc));
		}
	}
	CATCHZOK
	PPWait(0);
	return ok;
}
// } AHTOXA
