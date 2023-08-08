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
int GetUserDomain(const wchar_t * pUserIn, SStringU & rUserName, SStringU & rDomainName)
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

/*static*/HANDLE SSystem::GetLocalSystemProcessToken()
{
	HANDLE h_token = 0;
	DWORD pids[1024*10] = {0};
	DWORD cbNeeded = 0;
	if(EnumProcesses(pids, sizeof(pids), &cbNeeded)) {
		SStringU name;
		// Calculate how many process identifiers were returned.
		DWORD cProcesses = cbNeeded / sizeof(DWORD);
		for(DWORD i = 0; !h_token && i < cProcesses; ++i) {
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
						h_token = h_inner_token;
				}
				else
					gle = GetLastError();
				if(!h_token)
					CloseHandle(h_inner_token);
			}
			else
				gle = GetLastError();
			CloseHandle(h_process);
		}
		if(!h_token) {
			; //Log(L"Failed to get token for Local System.", true);
		}
	}
	else {
		; //Log(L"Can't enumProcesses - Failed to get token for Local System.", true);
	}
	return h_token;
}

void Duplicate(HANDLE & h, LPCSTR file, int line)
{
	HANDLE hDupe = NULL;
	if(DuplicateTokenEx(h, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &hDupe)) {
		CloseHandle(h);
		h = hDupe;
		hDupe = NULL;
	}
	else {
		DWORD gle = GetLastError();
		_ASSERT(0);
		//Log(StrFormat(L"Error duplicating a user token (%S, %d)", file, line), GetLastError());
	}
}

bool EnablePrivilege(LPCWSTR privilegeStr, HANDLE hToken /* = NULL */)
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
	if(!LookupPrivilegeValue(NULL, privilegeStr, &luid)) {
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

/*static*/bool SSystem::GetUserHandle(/*Settings*/WinUserBlock & rSettings, uint flags, BOOL & bLoadedProfile, PROFILEINFO & rProfile, HANDLE hCmdPipe)
{
	bool   ok = true;
	DWORD  gle = 0;
	if(flags & guhfUseSystemAccount) {
		if(!rSettings.H_User) { // might already have hUser from a previous call
			EnablePrivilege(SE_DEBUG_NAME, NULL); //helps with OpenProcess, required for GetLocalSystemProcessToken
			rSettings.H_User = GetLocalSystemProcessToken();
			THROW(rSettings.H_User); //Log(L"Not able to get Local System token", true);
			; //Log(L"Got Local System handle", false);
			Duplicate(rSettings.H_User, __FILE__, __LINE__);
		}
	}
	else {
		//not Local System, so either as specified user, or as current user
		if(rSettings.UserName.NotEmpty()) {
			SStringU user, domain;
			GetUserDomain(rSettings.UserName, user, domain);
			BOOL bLoggedIn = LogonUser(user, domain.IsEmpty() ? NULL : domain, rSettings.Password, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_WINNT50, &rSettings.H_User);
			gle = GetLastError();
#ifdef _DEBUG
			//Log(L"DEBUG: LogonUser", gle);
#endif
			THROW(bLoggedIn && SIntHandle::IsValid(rSettings.H_User)); //Log(StrFormat(L"Error logging in as %s.", rSettings.user), gle)
			Duplicate(rSettings.H_User, __FILE__, __LINE__); //gives max rights
			if(!SIntHandle::IsInvalid(rSettings.H_User) && !(flags & guhfDontLoadProfile)) {
				EnablePrivilege(SE_RESTORE_NAME, NULL);
				EnablePrivilege(SE_BACKUP_NAME, NULL);
				wchar_t user_name_buf[256];
				STRNSCPY(user_name_buf, user);
				MEMSZERO(rProfile);
				rProfile.dwSize = sizeof(rProfile);
				rProfile.lpUserName = user_name_buf;
				bLoadedProfile = LoadUserProfile(rSettings.H_User, &rProfile);
#ifdef _DEBUG
				gle = GetLastError();
				//Log(L"DEBUG: LoadUserProfile", gle);
#endif
			}
			return true;
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
			BOOL bOpen = OpenThreadToken(hThread, TOKEN_DUPLICATE | TOKEN_QUERY, TRUE, &rSettings.H_User);
			gle = GetLastError();
			if(gle == 1008) { //no thread token
				bOpen = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &rSettings.H_User);
				gle = GetLastError();
			}
			if(!bOpen) {
				; //Log(L"Failed to open current user token", GetLastError());
			}
			Duplicate(rSettings.H_User, __FILE__, __LINE__); //gives max rights
			RevertToSelf();
			THROW(SIntHandle::IsValid(rSettings.H_User));
		}
	}
	CATCHZOK
	return ok;
} 
#endif // } 0