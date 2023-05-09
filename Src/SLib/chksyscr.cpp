// CHKSYSCR.CPP
// 2005, 2007, 2008, 2009, 2010, 2016, 2018, 2020, 2023
// Слегка видоизмененный метод проверки имени пользователя и пароля в Windows
//
//  SSPI Authentication Sample
//  This program demonstrates how to use SSPI to authenticate user credentials.
//  Copyright (C) 2001.  Microsoft Corporation.  All rights reserved.
//
#include <slib-internal.h>
#pragma hdrstop
//@v11.7.1 #include <tchar.h>
#include <sspi.h>
#include <lm.h>

// Older versions of WinError.h does not have SEC_I_COMPLETE_NEEDED #define.
// So, in such SDK environment setup, we will include issperr.h which has the
// definition for SEC_I_COMPLETE_NEEDED. Include issperr.h only if
// SEC_I_COMPLETE_NEEDED is not defined.
#ifndef SEC_I_COMPLETE_NEEDED
	#include <issperr.h>
#endif

typedef struct _AUTH_SEQ {
	BOOL   fInitialized;
	BOOL   fHaveCredHandle;
	BOOL   fHaveCtxtHandle;
	CredHandle hcred;
	struct _SecHandle hctxt;
} AUTH_SEQ, *PAUTH_SEQ;
//
// Function pointers
//
struct _SysCrProcTable {
	_SysCrProcTable()
	{
		THISZERO();
	}
	int    Load();
	void   Unload()
	{
		if(H)
			FreeLibrary(H);
	}
	HMODULE H;
	ACCEPT_SECURITY_CONTEXT_FN       _AcceptSecurityContext;
	ACQUIRE_CREDENTIALS_HANDLE_FN    _AcquireCredentialsHandle;
	COMPLETE_AUTH_TOKEN_FN           _CompleteAuthToken;
	DELETE_SECURITY_CONTEXT_FN       _DeleteSecurityContext;
	FREE_CONTEXT_BUFFER_FN           _FreeContextBuffer;
	FREE_CREDENTIALS_HANDLE_FN       _FreeCredentialsHandle;
	INITIALIZE_SECURITY_CONTEXT_FN   _InitializeSecurityContext;
	QUERY_SECURITY_PACKAGE_INFO_FN   _QuerySecurityPackageInfo;
};

int _SysCrProcTable::Load()
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	TCHAR   lpszDLL[MAX_PATH];
	OSVERSIONINFO VerInfo;
	SString proc_name;
#ifdef UNICODE
	const int is_unicode = 1;
#else
	const int is_unicode = 0;
#endif
	//
	//  Find out which security DLL to use, depending on
	//  whether we are on NT or Win95 or 2000 or XP or Windows Server 2003
	//  We have to use security.dll on Windows NT 4.0.
	//  All other operating systems, we have to use Secur32.dll
	//
	VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	// If this fails, something has gone wrong
	THROW(GetVersionEx (&VerInfo));
	if(VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT && VerInfo.dwMajorVersion == 4 && VerInfo.dwMinorVersion == 0)
		lstrcpy(lpszDLL, _T("security.dll"));
	else
		lstrcpy(lpszDLL, _T("secur32.dll"));
	THROW(H = LoadLibrary(lpszDLL));
	THROW(_AcceptSecurityContext = (ACCEPT_SECURITY_CONTEXT_FN)GetProcAddress(H, "AcceptSecurityContext"));
	(proc_name = "AcquireCredentialsHandle").CatChar(is_unicode ? 'W' : 'A');
	THROW(_AcquireCredentialsHandle = (ACQUIRE_CREDENTIALS_HANDLE_FN)GetProcAddress(H, proc_name));
	//
	// CompleteAuthToken is not present on Windows 9x Secur32.dll
	// Do not check for the availablity of the function if it is NULL;
	//
	_CompleteAuthToken = (COMPLETE_AUTH_TOKEN_FN)GetProcAddress(H, "CompleteAuthToken");
	THROW(_DeleteSecurityContext = (DELETE_SECURITY_CONTEXT_FN)GetProcAddress(H, "DeleteSecurityContext"));
	THROW(_FreeContextBuffer = (FREE_CONTEXT_BUFFER_FN)GetProcAddress(H, "FreeContextBuffer"));
	THROW(_FreeCredentialsHandle = (FREE_CREDENTIALS_HANDLE_FN)GetProcAddress(H, "FreeCredentialsHandle"));
	(proc_name = "InitializeSecurityContext").CatChar(is_unicode ? 'W' : 'A');
	THROW(_InitializeSecurityContext = (INITIALIZE_SECURITY_CONTEXT_FN)GetProcAddress(H, proc_name));
	(proc_name = "QuerySecurityPackageInfo").CatChar(is_unicode ? 'W' : 'A');
	THROW(_QuerySecurityPackageInfo = (QUERY_SECURITY_PACKAGE_INFO_FN)GetProcAddress(H, proc_name));
	CATCH
		ok = SLS.SetOsError();
		Unload();
	ENDCATCH
	return ok;
}

//SLERR_WINSEC_ACQCREDHDL        202 // "AcquireCredentialsHandle failed with %s"
//SLERR_WINSEC_INITSECCTX        203 // "InitializeSecurityContext failed with %s"
//SLERR_WINSEC_COMPLAUTHTOK      204 // "CompleteAuthToken failed with %s"
//SLERR_WINSEC_COMPLAUTHTOKNSUPP 205 // "CompleteAuthToken not supported"
//SLERR_WINSEC_ACCPTSECCTX       206 // "AcceptSecurityContext failed with %s"

static int GenClientContext(_SysCrProcTable & rVt, PAUTH_SEQ pAS, PSEC_WINNT_AUTH_IDENTITY pAuthIdentity,
	PVOID pIn, DWORD cbIn, PVOID pOut, PDWORD pcbOut, PBOOL pfDone)
{
	/*++
	Routine Description:
		Optionally takes an input buffer coming from the server and returns
		a buffer of information to send back to the server.  Also returns
		an indication of whether or not the context is complete.
	Return Value:
		Returns TRUE if successful; otherwise FALSE.
	--*/
	EXCEPTVAR(SLibError);
	int    ok = 1;
	SECURITY_STATUS ss;
	TimeStamp       tsExpiry;
	SecBufferDesc   sbdOut;
	SecBuffer       sbOut;
	SecBufferDesc   sbdIn;
	SecBuffer       sbIn;
	ULONG           fContextAttr;
	SString msg_buf;
	if(!pAS->fInitialized) {
		ss = rVt._AcquireCredentialsHandle(NULL, const_cast<wchar_t *>(_T("NTLM")), SECPKG_CRED_OUTBOUND, NULL, pAuthIdentity, NULL, NULL, &pAS->hcred, &tsExpiry);
		SLS.SetAddedMsgString(msg_buf.Z().Cat(ss));
		THROW_V(ss >= 0, SLERR_WINSEC_ACQCREDHDL);
		pAS->fHaveCredHandle = TRUE;
	}
	//
	// Prepare output buffer
	//
	sbdOut.ulVersion = 0;
	sbdOut.cBuffers = 1;
	sbdOut.pBuffers = &sbOut;
	sbOut.cbBuffer = *pcbOut;
	sbOut.BufferType = SECBUFFER_TOKEN;
	sbOut.pvBuffer = pOut;
	//
	// Prepare input buffer
	//
	if(pAS->fInitialized) {
		sbdIn.ulVersion = 0;
		sbdIn.cBuffers = 1;
		sbdIn.pBuffers = &sbIn;
		sbIn.cbBuffer = cbIn;
		sbIn.BufferType = SECBUFFER_TOKEN;
		sbIn.pvBuffer = pIn;
	}
	ss = rVt._InitializeSecurityContext(&pAS->hcred, pAS->fInitialized ? &pAS->hctxt : NULL, NULL, 0, 0,
		SECURITY_NATIVE_DREP, pAS->fInitialized ? &sbdIn : NULL, 0, &pAS->hctxt, &sbdOut, &fContextAttr, &tsExpiry);
	SLS.SetAddedMsgString(msg_buf.Z().Cat(ss));
	THROW_V(ss >= 0, SLERR_WINSEC_INITSECCTX);
	pAS->fHaveCtxtHandle = TRUE;
	// If necessary, complete token
	if(ss == SEC_I_COMPLETE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE) {
		THROW_V(rVt._CompleteAuthToken, SLERR_WINSEC_COMPLAUTHTOKNSUPP);
		ss = rVt._CompleteAuthToken(&pAS->hctxt, &sbdOut);
		SLS.SetAddedMsgString(msg_buf.Z().Cat(ss));
		THROW_V(ss >= 0, SLERR_WINSEC_COMPLAUTHTOK);
	}
	*pcbOut = sbOut.cbBuffer;
	if(!pAS->fInitialized)
		pAS->fInitialized = TRUE;
	*pfDone = !(ss == SEC_I_CONTINUE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE );
	CATCHZOK
	return ok;
}

static int GenServerContext(_SysCrProcTable & rVt, PAUTH_SEQ pAS, PVOID pIn,
	DWORD cbIn, PVOID pOut, PDWORD pcbOut, PBOOL pfDone)
{
	/*++
	Routine Description:
		Takes an input buffer coming from the client and returns a buffer
		to be sent to the client.  Also returns an indication of whether or
		not the context is complete.
	Return Value:
		Returns TRUE if successful; otherwise FALSE.
	--*/
	EXCEPTVAR(SLibError);
	int    ok = 1;
	SECURITY_STATUS ss;
	TimeStamp       tsExpiry;
	SecBufferDesc   sbdOut;
	SecBuffer       sbOut;
	SecBufferDesc   sbdIn;
	SecBuffer       sbIn;
	ULONG           fContextAttr;
	SString msg_buf;
	if(!pAS->fInitialized) {
		ss = rVt._AcquireCredentialsHandle(NULL, const_cast<wchar_t *>(_T("NTLM")), SECPKG_CRED_INBOUND, 0, 0, 0, 0, &pAS->hcred, &tsExpiry);
		SLS.SetAddedMsgString(msg_buf.Z().Cat(ss));
		THROW_V(ss >= 0, SLERR_WINSEC_ACQCREDHDL);
		pAS->fHaveCredHandle = TRUE;
	}
	//
	// Prepare output buffer
	//
	sbdOut.ulVersion = 0;
	sbdOut.cBuffers = 1;
	sbdOut.pBuffers = &sbOut;
	sbOut.cbBuffer = *pcbOut;
	sbOut.BufferType = SECBUFFER_TOKEN;
	sbOut.pvBuffer = pOut;
	//
	// Prepare input buffer
	//
	sbdIn.ulVersion = 0;
	sbdIn.cBuffers = 1;
	sbdIn.pBuffers = &sbIn;
	sbIn.cbBuffer = cbIn;
	sbIn.BufferType = SECBUFFER_TOKEN;
	sbIn.pvBuffer = pIn;
	ss = rVt._AcceptSecurityContext(&pAS->hcred, pAS->fInitialized ? &pAS->hctxt : 0, &sbdIn, 0,
		SECURITY_NATIVE_DREP, &pAS->hctxt, &sbdOut, &fContextAttr, &tsExpiry);
	SLS.SetAddedMsgString(msg_buf.Z().Cat(ss));
	THROW_V(ss >= 0, SLERR_WINSEC_ACCPTSECCTX);
	pAS->fHaveCtxtHandle = TRUE;
	// If necessary, complete token
	if(oneof2(ss, SEC_I_COMPLETE_NEEDED, SEC_I_COMPLETE_AND_CONTINUE)) {
		THROW_V(rVt._CompleteAuthToken, SLERR_WINSEC_COMPLAUTHTOKNSUPP);
		ss = rVt._CompleteAuthToken(&pAS->hctxt, &sbdOut);
		SLS.SetAddedMsgString(msg_buf.Z().Cat(ss));
		THROW_V(ss >= 0, SLERR_WINSEC_COMPLAUTHTOK);
	}
	*pcbOut = sbOut.cbBuffer;
	if(!pAS->fInitialized)
		pAS->fInitialized = TRUE;
	*pfDone = !oneof2(ss, SEC_I_CONTINUE_NEEDED, SEC_I_COMPLETE_AND_CONTINUE);
	CATCHZOK
	return ok;
}

static BOOL WINAPI SSPLogonUser(LPTSTR szDomain, LPTSTR szUser, LPTSTR szPassword)
{
	int    ok = 1;
	AUTH_SEQ asServer = {0};
	AUTH_SEQ asClient = {0};
	BOOL   fDone      = FALSE;
	DWORD  cbOut      = 0;
	DWORD  cbIn       = 0;
	DWORD  cbMaxToken = 0;
	PVOID  pClientBuf = NULL;
	PVOID  pServerBuf = NULL;
	PSecPkgInfo pSPI  = NULL;
	SEC_WINNT_AUTH_IDENTITY ai;
	_SysCrProcTable vt;
	THROW(vt.Load());
	// Get max token size
	vt._QuerySecurityPackageInfo(const_cast<wchar_t *>(_T("NTLM")), &pSPI);
	cbMaxToken = pSPI->cbMaxToken;
	vt._FreeContextBuffer(pSPI);
	// Allocate buffers for client and server messages
	pClientBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbMaxToken);
	pServerBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbMaxToken);
	// Initialize auth identity structure
	MEMSZERO(ai);
#if defined(UNICODE) || defined(_UNICODE)
	ai.Domain = reinterpret_cast<ushort *>(szDomain);
	ai.DomainLength = lstrlen(szDomain);
	ai.User = reinterpret_cast<ushort *>(szUser);
	ai.UserLength = lstrlen(szUser);
	ai.Password = reinterpret_cast<ushort *>(szPassword);
	ai.PasswordLength = lstrlen(szPassword);
	ai.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
#else
	ai.Domain = reinterpret_cast<uchar *>(szDomain);
	ai.DomainLength = lstrlen(szDomain);
	ai.User = reinterpret_cast<uchar *>(szUser);
	ai.UserLength = lstrlen(szUser);
	ai.Password = reinterpret_cast<uchar *>(szPassword);
	ai.PasswordLength = lstrlen(szPassword);
	ai.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
#endif
	// Prepare client message (negotiate) .
	cbOut = cbMaxToken;
	THROW(GenClientContext(vt, &asClient, &ai, NULL, 0, pClientBuf, &cbOut, &fDone));
	// Prepare server message (challenge) .
	cbIn = cbOut;
	cbOut = cbMaxToken;
	THROW(GenServerContext(vt, &asServer, pClientBuf, cbIn, pServerBuf, &cbOut, &fDone));
		// Most likely failure: AcceptServerContext fails with SEC_E_LOGON_DENIED
		// in the case of bad szUser or szPassword.
		// Unexpected Result: Logon will succeed if you pass in a bad szUser and
		// the guest account is enabled in the specified domain.
		// Prepare client message (authenticate) .
	cbIn = cbOut;
	cbOut = cbMaxToken;
	THROW(GenClientContext(vt, &asClient, &ai, pServerBuf, cbIn, pClientBuf, &cbOut, &fDone));
	// Prepare server message (authentication) .
	cbIn = cbOut;
	cbOut = cbMaxToken;
	THROW(GenServerContext(vt, &asServer, pClientBuf, cbIn, pServerBuf, &cbOut, &fDone));
	CATCHZOK
	if(asClient.fHaveCtxtHandle)
    	vt._DeleteSecurityContext(&asClient.hctxt);
	if(asClient.fHaveCredHandle)
    	vt._FreeCredentialsHandle(&asClient.hcred);
	if(asServer.fHaveCtxtHandle)
    	vt._DeleteSecurityContext(&asServer.hctxt);
	if(asServer.fHaveCredHandle)
    	vt._FreeCredentialsHandle(&asServer.hcred);
	vt.Unload();
	HeapFree(GetProcessHeap(), 0, pClientBuf);
	HeapFree(GetProcessHeap(), 0, pServerBuf);
	return ok;
}

int SCheckSystemCredentials(const char * pDomain, const char * pUserName, const char * pPw)
{
	TCHAR  domain[128];
	TCHAR  user[128];
	TCHAR  pw[128];
	STRNSCPY(domain, SUcSwitch(pDomain));
	STRNSCPY(user, SUcSwitch(pUserName));
	STRNSCPY(pw, SUcSwitch(pPw));
	return BIN(SSPLogonUser(domain, user, pw));
}
//
//
//
int SGetTimeFromRemoteServer(const char * pServerName, LDATETIME * pDtm)
{
	int    ok = 0;
	LDATETIME dtm = ZERODATETIME;
	size_t sn_len = sstrlen(pServerName);
	if(sn_len) {
		typedef NET_API_STATUS (WINAPI * NETREMOTETOD)(LPCWSTR UncServerName, LPBYTE* BufferPtr);
		typedef NET_API_STATUS (WINAPI * NETAPIBUFFERFREE)(LPVOID Buffer);
		SDynLibrary lib_netapi("NETAPI32.DLL");
		if(lib_netapi.IsValid()) {
			NETREMOTETOD proc_NetRemoteTOD = (NETREMOTETOD)lib_netapi.GetProcAddr("NetRemoteTOD");
			NETAPIBUFFERFREE proc_NetApiBufferFree = (NETAPIBUFFERFREE)lib_netapi.GetProcAddr("NetApiBufferFree");
			if(proc_NetRemoteTOD && proc_NetApiBufferFree) {
				LPTIME_OF_DAY_INFO p_server_dtm = NULL;
				SStringU tm_serv_name; // @v10.8.2 
				// @v10.8.2 WCHAR  tm_serv[MAXPATH];
				// @v10.8.2 memzero(tm_serv, sizeof(tm_serv));
				// @v10.8.2 MultiByteToWideChar(1251, 0, pServerName, (int)sn_len, tm_serv, (int)SIZEOFARRAY(tm_serv));
				tm_serv_name.CopyFromMb_OUTER(pServerName, sn_len); // @v10.8.2
				int    oserr = proc_NetRemoteTOD(tm_serv_name, reinterpret_cast<LPBYTE *>(&p_server_dtm));
				if(oserr == NERR_Success && p_server_dtm) {
					dtm.d.encode(p_server_dtm->tod_day, p_server_dtm->tod_month, p_server_dtm->tod_year);
					dtm.t = encodetime(p_server_dtm->tod_hours, p_server_dtm->tod_mins, p_server_dtm->tod_secs, 0);
					if(p_server_dtm->tod_timezone != -1)
						dtm = plusdatetime(dtm, -p_server_dtm->tod_timezone, 2 /* minuts */);
					proc_NetApiBufferFree(p_server_dtm);
					ok = 1;
				}
				else {
					SlThreadLocalArea & r_tla = SLS.GetTLA();
					r_tla.LastErr = SLERR_WINDOWS;
					r_tla.LastOsErr = oserr;
				}
			}
		}
	}
	else
		ok = -1;
	ASSIGN_PTR(pDtm, dtm);
	return ok;
}

int SGetComputerName(SString & rName)
{
	int    ok = 1;
	rName = 0;
#ifdef UNICODE
	wchar_t buf[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD   sz = SIZEOFARRAY(buf);
	if(::GetComputerName(buf, &sz)) {
		rName.CopyUtf8FromUnicode(buf, sz, 0);
		rName.Transf(CTRANSF_UTF8_TO_OUTER);
	}
	else {
		ok = SLS.SetOsError();
	}
#else
	char   buf[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD  sz = SIZEOFARRAY(buf);
	if(::GetComputerName(buf, &sz)) {
		rName = buf;
	}
	else {
		ok = SLS.SetOsError();
	}
#endif
	return ok;
}
