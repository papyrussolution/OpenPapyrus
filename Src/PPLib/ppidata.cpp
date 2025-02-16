// PPYIDATA.CPP
// Copyright (c) A.Starodub, A.Sobolev 2003, 2005, 2006, 2007, 2008, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

#define PPDWNLDATA_MAXTAGVALSIZE   256
#define PPDWNLDATA_MAXTAGSIZE      64
#define PPDWNLDATA_MAXTAGNAMESSIZE 2048
//
// PpyInetDataPrcssr
//
static void SetInetError(HMODULE handle)
{
	const  int os_err_code = GetLastError();
	const  int err_code = PPErrCode;
	if(oneof2(err_code, PPERR_RCVFROMINET, PPERR_INETCONN) && handle != 0) {
		SString msg_buf;
		SSystem::SFormatMessage(os_err_code, msg_buf);
		msg_buf.Chomp().Transf(CTRANSF_OUTER_TO_INNER);
		PPSetAddedMsgString(msg_buf);
	}
}

PpyInetDataPrcssr::PpyInetDataPrcssr() : WinInetDLLHandle(0), InetSession(0)
{
#ifdef __WIN32__
	Uninit();
#endif // __WIN32__
}

PpyInetDataPrcssr::~PpyInetDataPrcssr()
{
#ifdef __WIN32__
	Uninit();
#endif // __WIN32__
}

int PpyInetDataPrcssr::Init()
{
	int    ok = 1;
	ulong  access_type = 0;
	SString proxi;
	Uninit();
	THROW(GetCfg(&IConnCfg));
	//sprintf(proxy, "%s:%s", IConnCfg.ProxyHost, IConnCfg.ProxyPort);
	proxi.Cat(IConnCfg.ProxyHost).Colon().Cat(IConnCfg.ProxyPort);
	access_type = (IConnCfg.AccessType == PPINETCONN_DIRECT) ? INTERNET_OPEN_TYPE_DIRECT :
		((IConnCfg.AccessType == PPINETCONN_PROXY) ? INTERNET_OPEN_TYPE_PROXY : INTERNET_OPEN_TYPE_PRECONFIG);
	THROW_PP(WinInetDLLHandle = ::LoadLibrary(_T("wininet.dll")), 0);
	THROW_PP((InetSession = InternetOpen(SUcSwitch(IConnCfg.Agent), access_type, 
		((access_type == INTERNET_OPEN_TYPE_PROXY) ? SUcSwitch(proxi) : 0), 0, 0)) != NULL, PPERR_RCVFROMINET); // @unicodeproblem
	THROW_PP(InternetSetOption(InetSession, INTERNET_OPTION_CONNECT_RETRIES, &IConnCfg.MaxTries, sizeof(IConnCfg.MaxTries)), PPERR_RCVFROMINET);
	CATCH
		SetInetError();
		ok = 0;
	ENDCATCH
	return ok;
}

void PpyInetDataPrcssr::Uninit()
{
	if(WinInetDLLHandle)
		FreeLibrary((HMODULE)WinInetDLLHandle);
	if(InetSession)
		InternetCloseHandle(InetSession);
	WinInetDLLHandle = 0;
	InetSession = 0;
	MEMSZERO(IConnCfg);
}

void PpyInetDataPrcssr::SetInetError()
{
	::SetInetError((HMODULE)WinInetDLLHandle);
}
//
// WinInetFTP
//
WinInetFTP::WinInetFTP() : InetSession(0), Connection(0), WinInetDLLHandle(0)
{
	UnInit();
}

WinInetFTP::~WinInetFTP()
{
	UnInit();
}

int WinInetFTP::Init(const PPInetConnConfig * pCfg)
{
	int    ok = 1;
	ulong  access_type = 0;
	long   sendrcv_timeout = 3 * 60 * 1000; // 3 min
	long   conn_timeout    = 3 * 60 * 1000; // 3 min
	SString proxy_buf;
	const  char * p_proxy_name = 0;
	THROW_INVARG(pCfg);
	UnInit();
	IConnCfg = *pCfg;
	if(IConnCfg.AccessType == PPINETCONN_DIRECT)
		access_type = INTERNET_OPEN_TYPE_DIRECT;
	else if(IConnCfg.AccessType == PPINETCONN_PROXY) {
		access_type = INTERNET_OPEN_TYPE_PROXY;
		p_proxy_name = proxy_buf.Z().Cat(IConnCfg.ProxyHost).Colon().Cat(IConnCfg.ProxyPort);
	}
	else
		access_type = INTERNET_OPEN_TYPE_PRECONFIG;
	THROW_PP(WinInetDLLHandle = ::LoadLibrary(_T("wininet.dll")), 0);
	THROW_PP((InetSession = InternetOpen(SUcSwitch(IConnCfg.Agent), access_type, SUcSwitch(p_proxy_name), 0, 0)) != NULL, PPERR_RCVFROMINET); // @unicodeproblem
	THROW_PP(InternetSetOption(InetSession, INTERNET_OPTION_CONNECT_RETRIES, &IConnCfg.MaxTries, sizeof(IConnCfg.MaxTries)), PPERR_RCVFROMINET);
	THROW_PP(InternetSetOption(InetSession, INTERNET_OPTION_CONNECT_TIMEOUT, &conn_timeout, sizeof(conn_timeout)), PPERR_RCVFROMINET);
	THROW_PP(InternetSetOption(InetSession, INTERNET_OPTION_RECEIVE_TIMEOUT, &sendrcv_timeout, sizeof(sendrcv_timeout)), PPERR_RCVFROMINET);
	THROW_PP(InternetSetOption(InetSession, INTERNET_OPTION_SEND_TIMEOUT, &sendrcv_timeout, sizeof(sendrcv_timeout)), PPERR_RCVFROMINET);
	CATCH
		SetInetError((HMODULE)WinInetDLLHandle);
		ok = 0;
	ENDCATCH
	return ok;
}

int WinInetFTP::Init()
{
	int    ok = 1;
	PpyInetDataPrcssr prcssr;
	THROW(prcssr.GetCfg(&IConnCfg));
	THROW(Init(&IConnCfg));
	CATCH
		SetInetError((HMODULE)WinInetDLLHandle);
		ok = 0;
	ENDCATCH
	return ok;
}

int WinInetFTP::UnInit()
{
	Disconnect();
	if(InetSession)
		InternetCloseHandle(InetSession);
	if(WinInetDLLHandle)
		FreeLibrary((HMODULE)WinInetDLLHandle);
	WinInetDLLHandle = 0;
	InetSession = 0;
	MEMSZERO(IConnCfg);
	return 1;
}

int WinInetFTP::Connect(PPInternetAccount * pAccount)
{
	int    ok = 1;
	char   pwd[48];
	int    port = 0;
	SString url_buf, user;
	SString host, temp_buf;
	SString scheme;
	Disconnect();
	THROW_INVARG(pAccount);
	Account = *pAccount;
	Account.GetExtField(FTPAEXSTR_HOST, url_buf);
	if(Account.GetExtField(FTPAEXSTR_PORT, temp_buf) > 0)
		port = temp_buf.ToLong();
	Account.GetExtField(FTPAEXSTR_USER, user);
	Account.GetPassword_(pwd, sizeof(pwd), FTPAEXSTR_PASSWORD);
	{
		SString pw_buf;
		Account.GetExtField(MAEXSTR_RCVPASSWORD, temp_buf);
		Reference::Helper_DecodeOtherPw(0, temp_buf, 48, pw_buf);
		STRNSCPY(pwd, pw_buf);
	}
	uint   conn_flags = (Account.Flags & PPInternetAccount::fFtpPassive) ? INTERNET_FLAG_PASSIVE : 0;
	{
		InetUrl url(url_buf);
		url.GetComponent(InetUrl::cHost, 0, host);
		host.SetIfEmpty(url_buf);
		url.GetComponent(InetUrl::cScheme, 0, scheme);
		if(scheme.IsEmpty()) {
			scheme = "ftp";
			url.SetComponent(InetUrl::cScheme, scheme);
		}
		//url.Compose(InetUrl::stScheme|InetUrl::stHost, host);
		if(pwd[0] == 0) {
			if(url.GetComponent(InetUrl::cPassword, 0, temp_buf) > 0)
				STRNSCPY(pwd, temp_buf);
		}
		if(user.IsEmpty()) {
			if(url.GetComponent(InetUrl::cUserName, 0, temp_buf) > 0)
				user = temp_buf;
		}
		if(port == 0) {
			if(url.GetComponent(InetUrl::cPort, 0, temp_buf) > 0) 
				port = temp_buf.ToLong(); // @v11.0.7 @fix
			if(port <= 0)
				port = url.GetDefProtocolPort(InetUrl::protFtp);
		}
		// SETIFZ(port, INTERNET_DEFAULT_FTP_PORT);
	}
	THROW_PP(Connection = InternetConnect(InetSession, SUcSwitch(host), port, 
		SUcSwitch(user), SUcSwitch(pwd), INTERNET_SERVICE_FTP, conn_flags, 0), PPERR_INETCONN); // @unicodeproblem
	CATCH
		SetInetError((HMODULE)WinInetDLLHandle);
		ok = 0;
	ENDCATCH
	return ok;
}

int WinInetFTP::ReInit()
{
	PPInternetAccount account;
	account = Account;
	return BIN(Init(&IConnCfg) && Connect(&account));
}

int WinInetFTP::Disconnect()
{
	if(Connection)
		InternetCloseHandle(Connection);
	return 1;
}

int WinInetFTP::CheckSizeAfterCopy(const char * pLocalPath, const char * pFTPPath)
{
	int    ok = 1;
	WIN32_FIND_DATA ff_info;
	SDirEntry lf_info;
	HINTERNET ftp_dir = NULL;
	SString file_name;
	{
		SString temp_buf(pFTPPath);
		temp_buf.Strip().ReplaceChar('\\', '/');
		if(temp_buf.HasPrefix("//")) {
			temp_buf.ShiftLeft(1);
		}
		else {
			InetUrl url(temp_buf);
			url.GetComponent(InetUrl::cPath, 0, temp_buf);
		}
        SFsPath ps(temp_buf);
		if(ps.Dir.NotEmpty())
			THROW(CD(ps.Dir));
        ps.Merge(SFsPath::fNam|SFsPath::fExt, file_name);
	}
	MEMSZERO(ff_info);
	MEMSZERO(lf_info);
	THROW_PP(ftp_dir = FtpFindFirstFile(Connection, SUcSwitch(file_name), &ff_info, 0, 0), PPERR_FTPSRVREPLYERR); // @unicodeproblem
	THROW_PP_S(GetFileStat(pLocalPath, &lf_info) > 0, PPERR_NOSRCFILE, pLocalPath);
	THROW_PP_S(lf_info.Size == ff_info.nFileSizeLow, PPERR_FTPSIZEINVALID, file_name);
	CATCH
		ok = ReadResponse();
	ENDCATCH
	if(ftp_dir)
		InternetCloseHandle(ftp_dir);
	return ok;
}

static int FtpReInit(WinInetFTP * pFtp, PPLogger * pLog)
{
	int ok = 0;
	if(pFtp) {
		if(pLog) {
			pLog->LogLastError();
			pLog->LogString(PPTXT_FTPCONNECT, 0);
		}
		SDelay(1000 * 5); // 5 sec
		if((ok = pFtp->ReInit()) && pLog)
			pLog->LogString(PPTXT_FTPRECONNECTED, 0);
	}
	return ok;
}

#define FTP_MAXTRIES 3

static SString & GetFileName(const char * pPath, SString & rFile)
{
	SFsPath ps(pPath);
	ps.Merge(SFsPath::fNam|SFsPath::fExt, rFile); 
	return rFile;
}

int WinInetFTP::SafeGet(const char * pLocalPath, const char * pFTPPath, int checkDtTm, PercentFunc pf, PPLogger * pLogger)
{
	int    r = 0;
	int    reinit = 1;
	SString buf;
	if(Exists(pFTPPath)) {
		int tries = IConnCfg.MaxTries > 0 ? IConnCfg.MaxTries : FTP_MAXTRIES;
		for(int i = 0; !r && i < tries; i++) {
			if(reinit) {
				//r = Get(pLocalPath, pFTPPath, checkDtTm, pf);
				//int WinInetFTP::Get(const char * pLocalPath, const char * pFTPPath, int checkDtTm, PercentFunc pf)
				r = (pLocalPath && pFTPPath) ? TransferFile(pLocalPath, pFTPPath, 0, checkDtTm, pf) : PPSetErrorInvParam();
			}
			if(!r)
				reinit = FtpReInit(this, pLogger);
		}
	}
	else
		r = -1;
	if(pLogger) {
		GetFileName(pFTPPath, buf);
		if(r > 0)
			pLogger->LogString(PPTXT_FTPRCVSUCCESS, buf);
		else if(r == -1)
			pLogger->LogMsgCode(mfError, PPERR_NOSRCFILE, buf);
		else if(!r) {
			SString add_buf;
			PPGetLastErrorMessage(1, buf);
			PPLoadString("error", add_buf);
			buf = add_buf.Space().Cat(buf);
			pLogger->Log(buf);
		}
	}
	return r;
}

int WinInetFTP::SafePut(const char * pLocalPath, const char * pFTPPath, int checkDtTm, PercentFunc pf, PPLogger * pLogger)
{
	int    r = 0, reinit = 1;
	SString buf;
	if(fileExists(pLocalPath)) {
		int tries = (IConnCfg.MaxTries > 0) ? IConnCfg.MaxTries : FTP_MAXTRIES;
		for(int i = 0; !r && i < tries; i++) {
			if(reinit) {
				//r = Put(pLocalPath, pFTPPath, checkDtTm, pf);
				//int WinInetFTP::Put(const char * pLocalPath, const char * pFTPPath, int checkDtTm, PercentFunc pf)
				{
					r = (pLocalPath && pFTPPath) ? TransferFile(pLocalPath, pFTPPath, 1, checkDtTm, pf) : PPSetErrorInvParam();
				}
			}
			if(!r)
				reinit = FtpReInit(this, pLogger);
		}
	}
	else
		r = -1;
	if(pLogger) {
		GetFileName(pFTPPath, buf);
		if(r > 0)
			pLogger->LogString(PPTXT_FTPSENDSUCCESS, buf);
		else if(r == -1)
			pLogger->LogMsgCode(mfError, PPERR_NOSRCFILE, buf);
		else if(!r) {
			SString add_buf;
			PPGetLastErrorMessage(1, buf);
			PPLoadString("error", add_buf);
			buf = add_buf.Space().Cat(buf);
			pLogger->Log(buf);
		}
	}
	return r;
}

int WinInetFTP::SafeCD(const char * pPath, int isFullPath, PPLogger * pLogger)
{
	int    r = 0, reinit = 1;
	int    tries = IConnCfg.MaxTries > 0 ? IConnCfg.MaxTries : FTP_MAXTRIES;
	for(int i = 0; !r && i < tries; i++) {
		if(reinit)
			r = CD(pPath, isFullPath);
		if(!r)
			reinit = FtpReInit(this, pLogger);
	}
	return r;
}

int WinInetFTP::SafeDelete(const char * pPath, PPLogger * pLogger)
{
	int    r = 0, reinit = 1;
	int    tries = IConnCfg.MaxTries > 0 ? IConnCfg.MaxTries : FTP_MAXTRIES;
	for(int i = 0; !r && i < tries; i++) {
		if(reinit)
			r = Delete(pPath);
		if(!r)
			reinit = FtpReInit(this, pLogger);
	}
	return r;
}

int WinInetFTP::SafeDeleteWOCD(const char * pPath, PPLogger * pLogger)
{
	int    r = 0, reinit = 1;
	const  int tries = IConnCfg.MaxTries > 0 ? IConnCfg.MaxTries : FTP_MAXTRIES;
	SFsPath sp;
	SString file_name;
	for(int i = 0; !r && i < tries; i++) {
		if(reinit) {
			//r = DeleteWOCD(pPath);
			//int WinInetFTP::DeleteWOCD(const char * pPath)
			{
				sp.Split(pPath);
				sp.Merge(SFsPath::fNam|SFsPath::fExt, file_name);
				if(!FtpDeleteFile(Connection, SUcSwitch(file_name))) { // @unicodeproblem
					PPSetError(PPERR_FTPSRVREPLYERR); 
					r = ReadResponse();
				}
			}
		}
		if(!r)
			reinit = FtpReInit(this, pLogger);
	}
	return r;
}

int WinInetFTP::SafeCreateDir(const char * pDir, PPLogger * pLogger)
{
	int    r = 0, reinit = 1;
	int    tries = IConnCfg.MaxTries > 0 ? IConnCfg.MaxTries : FTP_MAXTRIES;
	for(int i = 0; !r && i < tries; i++) {
		if(reinit)
			r = CreateDir(pDir);
		if(!r)
			reinit = FtpReInit(this, pLogger);
	}
	return r;
}

int WinInetFTP::SafeGetFileList(const char * pDir, StrAssocArray * pFileList, const char * pMask, PPLogger * pLogger)
{
	int    r = 0, reinit = 1;
	int    tries = (IConnCfg.MaxTries > 0) ? IConnCfg.MaxTries : FTP_MAXTRIES;
	for(int i = 0; !r && i < tries; i++) {
		if(reinit)
			r = GetFileList(pDir, pFileList, pMask);
		if(!r)
			reinit = FtpReInit(this, pLogger);
	}
	return r;
}

int WinInetFTP::TransferFile(const char * pLocalPath, const char * pFTPPath, int send, int checkDtTm, PercentFunc pf)
{
	int    ok = 1;
	bool   valid_dttm = false;
	int    lfile_exists = 0;
	BOOL   r = 0;
	SString file_name;
	SString temp_buf;
	SString msg_buf;
	LDATETIME ftpfile_dttm;
	WIN32_FIND_DATA ff_info;
	SYSTEMTIME st_time;
	SDirEntry lf_info;
	FILE * p_file = 0;
	HINTERNET file_conn = NULL;
	HINTERNET ftp_dir = NULL;
	{
		SFsPath::NormalizePath(pFTPPath, SFsPath::npfSlash|SFsPath::npfKeepCase, temp_buf);
		if(temp_buf.HasPrefix("//")) {
			temp_buf.ShiftLeft(1);
		}
		else {
			InetUrl url(temp_buf);
			url.GetComponent(InetUrl::cPath, 0, temp_buf);
		}
        SFsPath ps(temp_buf);
		if(ps.Dir.NotEmpty())
			THROW(CD(ps.Dir));
        ps.Merge(SFsPath::fNam|SFsPath::fExt, file_name);
	}
	MEMSZERO(lf_info);
	MEMSZERO(ff_info);
	THROW_PP((ftp_dir = FtpFindFirstFile(Connection, SUcSwitch(file_name), &ff_info, 0, 0)) || send, PPERR_FTPSRVREPLYERR); // @unicodeproblem
	PPSetAddedMsgString(pLocalPath);
	THROW_PP((lfile_exists = (GetFileStat(pLocalPath, &lf_info) > 0)) || !send, PPERR_NOSRCFILE);
	if(ftp_dir && lfile_exists) {
		FileTimeToSystemTime(&ff_info.ftLastWriteTime, &st_time);
		encodedate(st_time.wDay, st_time.wMonth, st_time.wYear, &ftpfile_dttm.d);
		ftpfile_dttm.t = encodetime(st_time.wHour, st_time.wMinute, st_time.wSecond, st_time.wMilliseconds / 10);
		LDATETIME lf_mod_dtm;
		lf_mod_dtm.SetNs100(lf_info.ModTm_);
		if(send) {
			if(lf_mod_dtm.d == ftpfile_dttm.d)
				valid_dttm = (lf_mod_dtm.t > ftpfile_dttm.t);
			else
				valid_dttm = (lf_mod_dtm.d > ftpfile_dttm.d);
		}
		else {
			if(ftpfile_dttm.d == lf_mod_dtm.d)
				valid_dttm = (ftpfile_dttm.t > lf_mod_dtm.t);
			else
				valid_dttm = (ftpfile_dttm.d > lf_mod_dtm.d);
		}
	}
	else
		valid_dttm = true;
	if(ftp_dir) {
		InternetCloseHandle(ftp_dir);
		ftp_dir = NULL;
	}
	if(!checkDtTm || valid_dttm) {
		char   buf[512];
		DWORD  len = 0, c_len = 0;
		DWORD  t_len = (DWORD)(send ? lf_info.Size : /*(ff_info.nFileSizeHigh * (MAXDWORD + 1)) + */ff_info.nFileSizeLow); // @64
		PPSetAddedMsgString(file_name);
		THROW_PP(file_conn = FtpOpenFile(Connection, SUcSwitch(file_name), send ? GENERIC_WRITE : GENERIC_READ, FTP_TRANSFER_TYPE_BINARY, 0), PPERR_FTPSRVREPLYERR); // @unicodeproblem
		THROW_PP((p_file = fopen(pLocalPath, send ? "rb" : "wb")) != NULL, PPERR_SLIB);
		PPLoadText(send ? PPTXT_PUTFILETOFTP : PPTXT_GETFILEFROMFTP, temp_buf.Z());
		msg_buf.Printf(temp_buf, file_name.cptr());
		if(send) {
			while((len = (DWORD)fread(buf, 1, sizeof(buf), p_file))) {
				DWORD sended = 0;
				c_len += len;
				THROW_PP(InternetWriteFile(file_conn, buf, len, &sended), PPERR_FTPSRVREPLYERR);
				THROW_PP(sended == len, PPERR_SENDTOFTP);
				if(pf)
					pf((long)c_len, (long)t_len, msg_buf, 0);
			}
			//THROW_PP(FtpPutFile(Connection, pLocalPath, fname, FTP_TRANSFER_TYPE_BINARY, 0), PPERR_SLIB);
		}
		else {
			while(InternetReadFile(file_conn, buf, sizeof(buf), &len) && len != 0) {
				c_len += len;
				fwrite(buf, 1, len, p_file);
				if(pf)
					pf((long)c_len, (long)t_len, msg_buf, 0);
			}
			//THROW_PP(FtpGetFile(Connection, fname, pLocalPath, 0, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0), PPERR_SLIB);
		}
	}
	else
		ok = -1;
	CATCH
		ok = ReadResponse();
	ENDCATCH
	SFile::ZClose(&p_file);
	if(file_conn)
		InternetCloseHandle(file_conn);
	if(ftp_dir)
		InternetCloseHandle(ftp_dir);
	if(ok > 0) {
		// @v11.1.7 ok = CheckSizeAfterCopy(pLocalPath, pFTPPath);
	}
	if(!ok) {
		SString add_str = DS.GetTLA().AddedMsgString;
		long err_code = PPErrCode;
		if(send)
			Delete(pFTPPath);
		else
			SFile::Remove(pLocalPath);
		PPSetError(err_code, add_str);
	}
	return ok;
}

int WinInetFTP::ReadResponse()
{
	TCHAR  buf[256];
	DWORD  errcode = 0, buflen = sizeof(buf), last_err = GetLastError();
	memzero(buf, sizeof(buf));
	if(last_err == 0 || GetLastError() == ERROR_INTERNET_EXTENDED_ERROR)
		InternetGetLastResponseInfo(&errcode, buf, &buflen); // @unicodeproblem
	if(buf[0] != '\0') {
		if(buflen > 2)
			buf[buflen - 2] = '\0';
		PPSetAddedMsgString(SUcSwitch(buf));
	}
	else {
		PPSetError(PPERR_RCVFROMINET);
		SetInetError(static_cast<HMODULE>(WinInetDLLHandle));
	}
	return 0;
}

int WinInetFTP::Delete(const char * pPath)
{
	int    ok = 1;
	SString file_name;
	{
		SString temp_buf;
		SFsPath::NormalizePath(pPath, SFsPath::npfSlash|SFsPath::npfKeepCase, temp_buf);
		if(temp_buf.HasPrefix("//")) {
			temp_buf.ShiftLeft(1);
		}
		else {
			InetUrl url(temp_buf);
			url.GetComponent(InetUrl::cPath, 0, temp_buf);
		}
        SFsPath ps(temp_buf);
		if(ps.Dir.NotEmpty())
			THROW(CD(ps.Dir));
        ps.Merge(SFsPath::fNam|SFsPath::fExt, file_name);
	}
	THROW_PP(FtpDeleteFile(Connection, SUcSwitch(file_name)), PPERR_FTPSRVREPLYERR); // @unicodeproblem
	CATCH
		ok = ReadResponse();
	ENDCATCH
	return ok;
}

int WinInetFTP::Exists(const char * pPath)
{
	int    ok = 1;
	SString file_name;
	{
		SString temp_buf;
		SFsPath::NormalizePath(pPath, SFsPath::npfSlash|SFsPath::npfKeepCase, temp_buf);
		if(temp_buf.HasPrefix("//")) {
			temp_buf.ShiftLeft(1);
		}
		else {
			InetUrl url(temp_buf);
			url.GetComponent(InetUrl::cPath, 0, temp_buf);
		}
        SFsPath ps(temp_buf);
		if(ps.Dir.NotEmpty())
			ok = CD(ps.Dir);
        ps.Merge(SFsPath::fNam|SFsPath::fExt, file_name);
	}
	if(ok) {
		WIN32_FIND_DATA ff_info;
		HINTERNET hf = FtpFindFirstFile(Connection, SUcSwitch(file_name), &ff_info, 0, 0); // @unicodeproblem
		if(hf)
			InternetCloseHandle(hf);
		else
			ok = 0;
	}
	if(!ok)
		ReadResponse();
	return ok;
}

int WinInetFTP::GetFileList(const char * pDir, StrAssocArray * pFileList, const char * pMask /*=0*/)
{
	int    ok = 1;
	if(!isempty(pDir)) {
		SString temp_buf;
		SFsPath::NormalizePath(pDir, SFsPath::npfSlash|SFsPath::npfKeepCase, temp_buf);
		if(temp_buf.HasPrefix("//")) {
			temp_buf.ShiftLeft(1);
		}
		else {
			InetUrl url(temp_buf);
			url.GetComponent(InetUrl::cPath, 0, temp_buf);
		}
        SFsPath ps(temp_buf);
		if(ps.Dir.NotEmpty())
			ok = CD(ps.Dir);
	}
	if(ok) {
		// @debug const char * p_mask = isempty(pMask) ? "*.*" : pMask;
		const char * p_mask = "*.*"; // @debug
		WIN32_FIND_DATA ff_info;
		HINTERNET hf = FtpFindFirstFile(Connection, SUcSwitch(p_mask), &ff_info, 0, 0); // @unicodeproblem
		if(hf) { 
			long id = 1;
			if(pFileList && !(ff_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				pFileList->Add(id++, SUcSwitch(ff_info.cFileName)); // @unicodeproblem
			while(InternetFindNextFile(hf, &ff_info) > 0)
				if(pFileList && !(ff_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					pFileList->Add(id++, SUcSwitch(ff_info.cFileName)); // @unicodeproblem
		}
		else {
			const uint last_err = GetLastError();
			ok = (last_err == ERROR_NO_MORE_FILES) ? -1 : 0;
		}
		if(hf)
			InternetCloseHandle(hf);
	}
	if(!ok)
		ReadResponse();
	return ok;
}

int WinInetFTP::CD(const char * pDir, int isFullPath /*=1*/)
{
	int    ok = 1, r;
	if(pDir) {
		SFsPath ps(pDir);
		SString sdir = ps.Dir;
		{
			uint   stop = 0;
			const  char * p_lvl_up = "..";
			sdir.RmvLastSlash();
			sdir.ReplaceChar('\\', '/').ShiftLeftChr('/').ShiftLeftChr('/');
			if(isFullPath) {
				/*
				for(i = 0; !stop && i < 30; i++)
					stop = BIN(!FtpSetCurrentDirectory(Connection, p_lvl_up));
				*/
				r = FtpSetCurrentDirectory(Connection, _T("/"));
			}
		}
		if(sdir.NotEmptyS()) {
			THROW_PP(FtpSetCurrentDirectory(Connection, SUcSwitch(sdir)), PPERR_FTPSRVREPLYERR); // @unicodeproblem
		}
	}
	else {
		FtpSetCurrentDirectory(Connection, _T("/"));
	}
#ifndef NDEBUG // {
	{
		SString test_current_dir;
		TCHAR  _cd[256];
		DWORD  _cd_buf_len = SIZEOFARRAY(_cd);
		FtpGetCurrentDirectory(Connection, _cd, &_cd_buf_len); // @unicodeproblem
		test_current_dir = SUcSwitch(_cd);
	}
#endif // }
	CATCH
		ok = ReadResponse();
	ENDCATCH
	return ok;
}

int WinInetFTP::CreateDir(const char * pDir)
{
	int    ok = 1;
	THROW_PP(FtpCreateDirectory(Connection, SUcSwitch(pDir)), PPERR_FTPSRVREPLYERR); // @unicodeproblem
	CATCH
		ok = ReadResponse();
	ENDCATCH
	return ok;
}

/*static*/int PpyInetDataPrcssr::EditCfg()
{
	class InetConnConfigDialog : public TDialog {
		DECL_DIALOG_DATA(PPInetConnConfig);
	public:
		InetConnConfigDialog() : TDialog(DLG_ICONNCFG)
		{
			PPLoadText(PPTXT_INETCONNAGENTS, Agents);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			int v = 0;
			if(!RVALUEPTR(Data, pData))
				MEMSZERO(Data);
			v = (PPSearchSubStr(Agents, &v, Data.Agent, 0) > 0) ? v : 0;
			setCtrlData(CTL_ICONNCFG_AGENT,      &v);
			setCtrlData(CTL_ICONNCFG_MAXTRIES,   &Data.MaxTries);
			setCtrlData(CTL_ICONNCFG_PROXYHOST,  Data.ProxyHost);
			setCtrlData(CTL_ICONNCFG_PROXYPORT,  &Data.ProxyPort);
			setCtrlData(CTL_ICONNCFG_URLDIR,     Data.URLDir);
			v = (Data.AccessType == PPINETCONN_DIRECT) ? 0 : ((Data.AccessType == PPINETCONN_PROXY) ? 1 : 2);
			setCtrlData(CTL_ICONNCFG_ACCESSTYPE, &v);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = -1;
			uint v = 0;
			getCtrlData(CTL_ICONNCFG_AGENT,      &v);
			PPGetSubStr(PPTXT_INETCONNAGENTS, v, Data.Agent, sizeof(Data.Agent));
			getCtrlData(CTL_ICONNCFG_MAXTRIES,   &Data.MaxTries);
			getCtrlData(CTL_ICONNCFG_PROXYHOST,  Data.ProxyHost);
			getCtrlData(CTL_ICONNCFG_PROXYPORT,  &Data.ProxyPort);
			getCtrlData(CTL_ICONNCFG_URLDIR,     Data.URLDir);
			getCtrlData(CTL_ICONNCFG_ACCESSTYPE, &v);
			Data.AccessType  = (v == 0) ? PPINETCONN_DIRECT : ((v == 1) ? PPINETCONN_PROXY : PPINETCONN_PRECONFIG);
			ASSIGN_PTR(pData, Data);
			ok = 1;
			return ok;
		}
	private:
		SString Agents;
	};
	int    ok = -1;
	int    valid_data = 0;
	InetConnConfigDialog * p_dlg = new InetConnConfigDialog();
	PPInetConnConfig cfg;
	THROW(CheckCfgRights(PPCFGOBJ_INETCONN, PPR_READ, 0));
	{
		MEMSZERO(cfg);
		const bool is_new = (GetCfg(&cfg) <= 0);
		THROW(CheckDialogPtrErr(&p_dlg));
		p_dlg->setDTS(&cfg);
		while(!valid_data && ExecView(p_dlg) == cmOK) {
			THROW(CheckCfgRights(PPCFGOBJ_INETCONN, PPR_MOD, 0));
			if(p_dlg->getDTS(&cfg) > 0) {
				if(PutCfg(&cfg, 1)) {
					valid_data = 1;
					ok = 1;
				}
				else
					PPError();
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

/*static*/int PpyInetDataPrcssr::GetCfg(PPInetConnConfig * pCfg)
{
	PPInetConnConfig cfg;
	int    ok = PPRef->GetPropMainConfig(PPPRP_INETCONNCFG, &cfg, sizeof(cfg));
	if(ok <= 0)
		MEMSZERO(cfg);
	ASSIGN_PTR(pCfg, cfg);
	return ok;
}

/*static*/int PpyInetDataPrcssr::PutCfg(const PPInetConnConfig * pCfg, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	int    is_new = 1;
	PPInetConnConfig prev_cfg;
	PPInetConnConfig cfg = *pCfg;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(p_ref->GetPropMainConfig(PPPRP_INETCONNCFG, &prev_cfg, sizeof(prev_cfg)) > 0)
			is_new = 0;
		THROW(p_ref->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_INETCONNCFG, &cfg, sizeof(cfg), 0));
		DS.LogAction(is_new ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_INETCONN, 0, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
