// WINPROFILE.CPP
// Copyright (c) A.Sobolev 2023
//
#include <slib-internal.h>
#pragma hdrstop
#include <ProfInfo.h>
#include <sddl.h>
#include <psapi.h>
#include <userenv.h>
//
// Returns:
//   0 - error (empty input string)
//   1 - user name only
//   2 - user name and domain
//
int __ParseWindowsUserForDomain(const wchar_t * pUserIn, SStringU & rUserName, SStringU & rDomainName)
{
	rUserName.Z();
	rDomainName.Z();
	int    ok = 0;
	if(!isempty(pUserIn)) {
		//run as specified user
		if(wcschr(pUserIn, L'@')) {
			rUserName = pUserIn; //leave domain as NULL
			ok = 1;
		}
		else {
			SStringU tmp(pUserIn);
			if(tmp.Divide(L'\\', rDomainName, rUserName) > 0) {
				ok = 2;
			}
			else {
				//no domain given
				rUserName = pUserIn;
				rDomainName = L".";
				ok = 1;
			}
		}
	}
	return ok;
}

bool Helper_GetTokenInformation(HANDLE hToken, TOKEN_INFORMATION_CLASS tokenInformationClass, STempBuffer & rRetBuf, size_t * pActualSize)
{
	bool   ok = true;
	DWORD  actual_size = 0;
	rRetBuf.Alloc(4096);
	THROW(GetTokenInformation(hToken, tokenInformationClass, rRetBuf, rRetBuf.GetSize(), &actual_size));
	ASSIGN_PTR(pActualSize, actual_size);
	CATCHZOK
	return ok;
}

#if 1 // {

int GetTokenUserSID(HANDLE hToken, SStringU & rUserName)
{
	int   ok = 1;
	rUserName.Z();
	DWORD tmp = 0;
	DWORD sid_name_size = 64;
	//std::vector<WCHAR> sidName;
	//sidName.resize(sidNameSize);
	//DWORD sidDomainSize = 64;
	//std::vector<WCHAR> sidDomain;
	//sidDomain.resize(sidNameSize);
	//DWORD userTokenSize = 1024;
	//std::vector<WCHAR> tokenUserBuf;
	//tokenUserBuf.resize(userTokenSize);
	//TOKEN_USER * userToken = (TOKEN_USER*)&tokenUserBuf.front();
	size_t actual_size = 0;
	STempBuffer token_data(4096);
	THROW(Helper_GetTokenInformation(hToken, TokenUser, token_data, &actual_size));
	{
		const TOKEN_USER * p_user_token = static_cast<const TOKEN_USER *>(token_data.vcptr());
		WCHAR * p_sid_string = NULL;
		if(ConvertSidToStringSid(p_user_token->User.Sid, &p_sid_string))
			rUserName = p_sid_string;
		if(p_sid_string)
			LocalFree(p_sid_string);
	}
	CATCHZOK
	return ok;
}

struct SystemLogonParamBlock {
	SystemLogonParamBlock() : Type(0), Provider(0), State(0)
	{
	}
	SystemLogonParamBlock(const wchar_t * pDomain, const wchar_t * pUserName, const wchar_t * pPw, uint logontype, uint logonprv) : 
		Type(logontype), Provider(logonprv), State(0)
	{
		SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
		r_temp_buf_u = pUserName;
		THROW(r_temp_buf_u.NotEmpty());
		if(isempty(pDomain)) {
			__ParseWindowsUserForDomain(r_temp_buf_u, User, Domain);
		}
		else {
			Domain = pDomain;
		}		
		CATCH
			State |= stError;
		ENDCATCH
		r_temp_buf_u.Obfuscate();
	}
	SystemLogonParamBlock(const char * pDomainUtf8, const char * pUserNameUtf8, const char * pPwUtf8, uint logontype, uint logonprv) : 
		Type(logontype), Provider(logonprv), State(0)
	{
		SString & r_temp_buf = SLS.AcquireRvlStr();
		SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
		r_temp_buf = pDomainUtf8;
		THROW(r_temp_buf.IsEmpty() || r_temp_buf.IsLegalUtf8());
		r_temp_buf = pPwUtf8;
		THROW(r_temp_buf.IsEmpty() || r_temp_buf.IsLegalUtf8());
		Pw.CopyFromUtf8(r_temp_buf);
		//
		r_temp_buf = pUserNameUtf8;
		THROW(r_temp_buf.NotEmpty() && r_temp_buf.IsLegalUtf8());
		if(isempty(pDomainUtf8)) {
			r_temp_buf_u.CopyFromUtf8(r_temp_buf);
			__ParseWindowsUserForDomain(r_temp_buf_u, User, Domain);
		}
		else {
			THROW(Domain.CopyFromUtf8R(pDomainUtf8, sstrlen(pDomainUtf8), 0));
		}
		CATCH
			State |= stError;
		ENDCATCH
		r_temp_buf_u.Obfuscate();
		r_temp_buf.Obfuscate();
	}
	~SystemLogonParamBlock()
	{
		Domain.Obfuscate();
		User.Obfuscate();
		Pw.Obfuscate();
		Type = 0;
		Provider = 0;
		State = 0;
	}
	bool   IsValid() const { return !(State & stError); }
	/*
		typedef struct _PROFILEINFOW {
			DWORD       dwSize;                 // Set to sizeof(PROFILEINFO) before calling
			DWORD       dwFlags;                // See PI_ flags defined in userenv.h
			MIDL_STRING LPWSTR      lpUserName;             // User name (required)
			MIDL_STRING LPWSTR      lpProfilePath;          // Roaming profile path (optional, can be NULL)
			MIDL_STRING LPWSTR      lpDefaultPath;          // Default user profile path (optional, can be NULL)
			MIDL_STRING LPWSTR      lpServerName;           // Validating domain controller name in netbios format (optional, can be NULL but group NT4 style policy won't be applied)
			MIDL_STRING LPWSTR      lpPolicyPath;           // Path to the NT4 style policy file (optional, can be NULL)
			#ifdef __midl
				ULONG_PTR   hProfile;               // Filled in by the function.  Registry key handle open to the root.
			#else
				HANDLE      hProfile;               // Filled in by the function.  Registry key handle open to the root.
			#endif
		} PROFILEINFOW, FAR * LPPROFILEINFOW;
	*/
	SPtrHandle Implement_Logon(SSystem::UserProfileInfo * pProfileInfo)
	{
		SPtrHandle result;
		if(IsValid()) {
			HANDLE h_token = 0;			
			BOOL r = LogonUserW(User, Domain, Pw, /*LOGON32_LOGON_INTERACTIVE*/Type, /*LOGON32_PROVIDER_DEFAULT*/Provider, &h_token);
			if(r) {
				{
					HANDLE h_dup = 0;
					if(::DuplicateTokenEx(h_token, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &h_dup)) {
						::CloseHandle(h_token);
						h_token = h_dup;
						h_dup = NULL;
					}
					else {
						DWORD gle = ::GetLastError();
						_ASSERT(0);
						//Log(StrFormat(L"Error duplicating a user token (%S, %d)", file, line), GetLastError());
					}
				}
				if(pProfileInfo) {
					STempBuffer user_name_noncost(2 * (User.Len() + 1) * sizeof(wchar_t)); // 2* - insurance
					PROFILEINFOW profile_info;
					MEMSZERO(profile_info);
					profile_info.dwSize = sizeof(profile_info);
					sstrcpy(static_cast<wchar_t *>(user_name_noncost.vptr()), User);
					profile_info.lpUserName = static_cast<wchar_t *>(user_name_noncost.vptr());
					profile_info.dwFlags = PI_NOUI;
					//profile_info.lpProfilePath = pUserInfo->usri4_profile;
					BOOL lpr = LoadUserProfileW(h_token, &profile_info);
					if(lpr) {
						pProfileInfo->DefaultPath = profile_info.lpDefaultPath;
						pProfileInfo->PolicyPath = profile_info.lpPolicyPath;
						pProfileInfo->ProfilePath = profile_info.lpProfilePath;
						pProfileInfo->ServerName = profile_info.lpServerName;
						pProfileInfo->UserName = profile_info.lpUserName;
						pProfileInfo->ProfileRegKey = profile_info.hProfile;
					}
					else { // @error
						::CloseHandle(h_token);
						h_token = 0;
					}
				}
				result = h_token;
			}
		}
		return result;
	}
	enum {
		stError = 0x0001
	};
	uint   Type;     // SSystem::logontypeXXX
	uint   Provider; // SSystem::logonprvXXX
	uint   State;
	SStringU Domain;
	SStringU User;
	SStringU Pw;
};

/*static*/SPtrHandle SSystem::Logon(const wchar_t * pDomain, const wchar_t * pUserName, const wchar_t * pPw, uint logontype, UserProfileInfo * pProfileInfo)
{
	SystemLogonParamBlock blk(pDomain, pUserName, pPw, logontype, SSystem::logonprvDEFAULT);
	return blk.Implement_Logon(pProfileInfo);
}

/*static*/SPtrHandle SSystem::Logon(const char * pDomainUtf8, const char * pUserNameUtf8, const char * pPwUtf8, uint logontype, UserProfileInfo * pProfileInfo)
{
	SystemLogonParamBlock blk(pDomainUtf8, pUserNameUtf8, pPwUtf8, logontype, SSystem::logonprvDEFAULT);
	return blk.Implement_Logon(pProfileInfo);
}

/*static*/SPtrHandle SSystem::GetLocalSystemProcessToken()
{
	//HANDLE h_token = 0;
	SPtrHandle result;
	DWORD pids[1024*10] = {0};
	DWORD cbNeeded = 0;
	if(EnumProcesses(pids, sizeof(pids), &cbNeeded)) {
		SStringU name;
		// Calculate how many process identifiers were returned.
		DWORD cProcesses = cbNeeded / sizeof(DWORD);
		for(DWORD i = 0; !result && i < cProcesses; ++i) {
			DWORD gle = 0;
			DWORD dwPid = pids[i];
			HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPid);
			if(h_process) {
				HANDLE h_inner_token = 0;
				if(OpenProcessToken(h_process, TOKEN_QUERY|TOKEN_READ|TOKEN_IMPERSONATE|TOKEN_QUERY_SOURCE|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_EXECUTE, &h_inner_token)) {
					GetTokenUserSID(h_inner_token, name);
					//const wchar_t arg[] = L"NT AUTHORITY\\";
					//if(0 == _wcsnicmp(name, arg, sizeof(arg)/sizeof(arg[0])-1))
					if(name.IsEq(L"S-1-5-18")) // Well known SID for Local System
						result = h_inner_token;
				}
				else
					gle = GetLastError();
				if(!result)
					CloseHandle(h_inner_token);
			}
			else
				gle = GetLastError();
			CloseHandle(h_process);
		}
		if(!result) {
			; //Log(L"Failed to get token for Local System.", true);
		}
	}
	else {
		; //Log(L"Can't enumProcesses - Failed to get token for Local System.", true);
	}
	return result;
}

static SPtrHandle DuplicateAccessToken(SPtrHandle & rH, LPCSTR file, int line)
{
	SPtrHandle result;
	HANDLE h_dup = 0;
	if(::DuplicateTokenEx(rH, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &h_dup)) {
		::CloseHandle(rH);
		result = h_dup;
		h_dup = NULL;
	}
	else {
		DWORD gle = ::GetLastError();
		_ASSERT(0);
		//Log(StrFormat(L"Error duplicating a user token (%S, %d)", file, line), GetLastError());
	}
	return result;
}

bool EnablePrivilege(HANDLE hToken, const wchar_t * pPrivilegeStr/* = NULL */)
{
	TOKEN_PRIVILEGES tp; // token privileges
	LUID luid;
	bool do_close_token = false;
	if(!hToken) {
		if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
			//Log(StrFormat(L"Failed to open process to enable privilege %s", privilegeStr), false);
			return false;
		}
		do_close_token = true;
	}
	if(!LookupPrivilegeValueW(NULL, pPrivilegeStr, &luid)) {
		if(do_close_token)
			CloseHandle(hToken);
		_ASSERT(0);
		//Log(StrFormat(L"Could not find privilege %s", privilegeStr), false);
		return false;
	}
	ZeroMemory(&tp, sizeof (tp));
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	// Adjust Token privileges
	if(!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
		DWORD gle = GetLastError();
		//Log(StrFormat(L"Failed to adjust token for privilege %s", privilegeStr), gle);
		if(do_close_token)
			CloseHandle(hToken);
		_ASSERT(0);
		return false;
	}
	if(do_close_token)
		CloseHandle(hToken);
	return true;
}

/*static*/bool SSystem::GetUserName_(SString & rBuf)
{
	bool    ok = false;
	wchar_t _buf[256];
	DWORD buf_len = SIZEOFARRAY(_buf);
	if(::GetUserNameW(_buf, &buf_len)) {
		ok = rBuf.CopyUtf8FromUnicode(_buf, sstrlen(_buf), 1);
	}
	return ok;
}

/*static*/bool SSystem::GetUserHandle(/*Settings*/WinUserBlock & rSettings, uint flags, BOOL & bLoadedProfile, PROFILEINFO & rProfile, HANDLE hCmdPipe)
{
	bool   ok = true;
	DWORD  gle = 0;
	if(flags & guhfUseSystemAccount) {
		if(!rSettings.H_User_) { // might already have hUser from a previous call
			EnablePrivilege(SE_DEBUG_NAME, NULL); //helps with OpenProcess, required for GetLocalSystemProcessToken
			rSettings.H_User_ = GetLocalSystemProcessToken();
			THROW(rSettings.H_User_); //Log(L"Not able to get Local System token", true);
			; //Log(L"Got Local System handle", false);
			SPtrHandle h_dup = DuplicateAccessToken(rSettings.H_User_, __FILE__, __LINE__);
			rSettings.H_User_ = h_dup;
		}
	}
	else {
		//not Local System, so either as specified user, or as current user
		if(rSettings.UserName.NotEmpty()) {
			SStringU user, domain;
			__ParseWindowsUserForDomain(rSettings.UserName, user, domain);
			HANDLE h_logon_user = 0;
			BOOL is_logged_id = ::LogonUser(user, domain.IsEmpty() ? NULL : domain, rSettings.Password, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_WINNT50, &h_logon_user);
			if(is_logged_id)
				rSettings.H_User_ = h_logon_user;
			else
				rSettings.H_User_ = 0;
			gle = GetLastError();
#ifdef _DEBUG
			//Log(L"DEBUG: LogonUser", gle);
#endif
			THROW(is_logged_id && rSettings.H_User_); //Log(StrFormat(L"Error logging in as %s.", rSettings.user), gle)
			{
				SPtrHandle h_dup = DuplicateAccessToken(rSettings.H_User_, __FILE__, __LINE__); //gives max rights
				rSettings.H_User_ = h_dup;
				if(!!rSettings.H_User_ && !(flags & guhfDontLoadProfile)) {
					EnablePrivilege(NULL, SE_RESTORE_NAME);
					EnablePrivilege(NULL, SE_BACKUP_NAME);
					EnablePrivilege(NULL, SE_ASSIGNPRIMARYTOKEN_NAME); //
					wchar_t user_name_buf[256];
					STRNSCPY(user_name_buf, user);
					MEMSZERO(rProfile);
					rProfile.dwSize = sizeof(rProfile);
					rProfile.lpUserName = user_name_buf;
					bLoadedProfile = LoadUserProfile(rSettings.H_User_, &rProfile);
	#ifdef _DEBUG
					gle = GetLastError();
					//Log(L"DEBUG: LoadUserProfile", gle);
	#endif
				}
				return true;
			}
		}
		else {
			//run as current user
			if(hCmdPipe) {
				BOOL b = ImpersonateNamedPipeClient(hCmdPipe);
				DWORD gle = GetLastError();
				if(!b) {
					; // Log(L"Failed to impersonate client user", gle);
				}
				else {
					; // Log(L"Impersonated caller", false);
				}
			}
			HANDLE hThread = GetCurrentThread();
			BOOL bDupe = DuplicateHandle(GetCurrentProcess(), hThread, GetCurrentProcess(), &hThread, 0, TRUE, DUPLICATE_SAME_ACCESS);
			DWORD gle = GetLastError();
			HANDLE h_token = 0;
			BOOL bOpen = OpenThreadToken(hThread, TOKEN_DUPLICATE|TOKEN_QUERY, TRUE, &h_token);
			rSettings.H_User_ = bOpen ? h_token : 0;
			gle = GetLastError();
			if(gle == 1008) { //no thread token
				rSettings.H_User_ = SlProcess::OpenCurrentAccessToken(TOKEN_DUPLICATE | TOKEN_QUERY);
				//bOpen = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &rSettings.H_User);
				gle = GetLastError();
			}
			if(/*!bOpen*/!rSettings.H_User_) {
				; //Log(L"Failed to open current user token", GetLastError());
			}
			SPtrHandle h_dup = DuplicateAccessToken(rSettings.H_User_, __FILE__, __LINE__); //gives max rights
			rSettings.H_User_ = h_dup;
			RevertToSelf();
			//THROW(SIntHandle::IsValid(rSettings.H_User_));
			THROW(rSettings.H_User_);
		}
	}
	CATCHZOK
	return ok;
} 
#endif // } 0