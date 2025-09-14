// SPROCESS.CPP
// Copyright (c) A.Sobolev 2023, 2024, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <UserEnv.h> // SlProcess
#include <sddl.h> // SlProcess
#include <AccCtrl.h>
#include <AclAPI.h>
#include <NTSecAPI.h>
#include <strsafe.h>

#include <iostream>
#include <dsgetdc.h>
#include <Lm.h>
#include <tlhelp32.h> // CreateToolhelp32Snapshot

// #include <..\SLib\subprocess\subprocess.h> // @v11.9.3 @experimental

typedef HRESULT (WINAPI * FN_CREATEAPPCONTAINERPROFILE)(PCWSTR, PCWSTR, PCWSTR, PSID_AND_ATTRIBUTES, DWORD, PSID *); // Userenv.dll:CreateAppContainerProfile
typedef HRESULT (WINAPI * FN_DERIVEAPPCONTAINERSIDFROMAPPCONTAINERNAME)(PCWSTR, PSID *); // Userenv.dll:DeriveAppContainerSidFromAppContainerName
typedef HRESULT (WINAPI * FN_DELETEAPPCONTAINERPROFILE)(PCWSTR); // Userenv.dll:DeleteAppContainerProfile
typedef HRESULT (WINAPI * FN_GETAPPCONTAINERFOLDERPATH)(PCWSTR, PWSTR *); // Userenv.dll:GetAppContainerFolderPath
typedef HRESULT (WINAPI * FN_GETAPPCONTAINERREGISTRYLOCATION)(REGSAM, PHKEY); // Userenv.dll:GetAppContainerRegistryLocation

class WinApi_UserEnv_ProcPool {
public:
	static const WinApi_UserEnv_ProcPool * GetInstance()
	{
		WinApi_UserEnv_ProcPool * p_result = 0;
		const char * p_symb = "WinApi_UserEnv_ProcPool";
		long   symbol_id = SLS.GetGlobalSymbol(p_symb, -1, 0);
		if(symbol_id < 0) {
			TSClassWrapper <WinApi_UserEnv_ProcPool> cls;
			symbol_id = SLS.CreateGlobalObject(cls);
			const long s = SLS.GetGlobalSymbol(p_symb, symbol_id, 0);
			assert(symbol_id == s);
			p_result = static_cast<WinApi_UserEnv_ProcPool *>(SLS.GetGlobalObject(symbol_id));
		}
		else {
			p_result = static_cast<WinApi_UserEnv_ProcPool *>(SLS.GetGlobalObject(symbol_id));
		}
		return p_result;
	}
	WinApi_UserEnv_ProcPool() : Lib("Userenv.dll")
	{
		if(Lib.IsValid()) {
			CreateAppContainerProfile_Func = (FN_CREATEAPPCONTAINERPROFILE)Lib.GetProcAddr("CreateAppContainerProfile");
			DeriveAppContainerSidFromAppContainerName_Func = (FN_DERIVEAPPCONTAINERSIDFROMAPPCONTAINERNAME)Lib.GetProcAddr("DeriveAppContainerSidFromAppContainerName");
			DeleteAppContainerProfile_Func = (FN_DELETEAPPCONTAINERPROFILE)Lib.GetProcAddr("DeleteAppContainerProfile");
			GetAppContainerFolderPath_Func = (FN_GETAPPCONTAINERFOLDERPATH)Lib.GetProcAddr("GetAppContainerFolderPath");
			GetAppContainerRegistryLocation_Func = (FN_GETAPPCONTAINERREGISTRYLOCATION)Lib.GetProcAddr("GetAppContainerRegistryLocation");
		}
	}
	FN_CREATEAPPCONTAINERPROFILE CreateAppContainerProfile_Func;
	FN_DERIVEAPPCONTAINERSIDFROMAPPCONTAINERNAME DeriveAppContainerSidFromAppContainerName_Func;
	FN_DELETEAPPCONTAINERPROFILE DeleteAppContainerProfile_Func;
	FN_GETAPPCONTAINERFOLDERPATH GetAppContainerFolderPath_Func;
	FN_GETAPPCONTAINERREGISTRYLOCATION GetAppContainerRegistryLocation_Func;
private:
	SDynLibrary Lib;
};
//
// 
//
/*static*/SPtrHandle SlProcess::OpenAccessToken(SIntHandle hProcess, uint desiredAccess)
{
	SPtrHandle result;
	HANDLE h = 0;
	if(::OpenProcessToken(hProcess, desiredAccess, &h)) {
		assert(h != 0);
		result = h;
	}
	return result;
}

/*static*/SPtrHandle SlProcess::OpenCurrentAccessToken(uint desiredAccess)
{
	SPtrHandle result;
	HANDLE h = 0;
	if(::OpenProcessToken(::GetCurrentProcess(), desiredAccess, &h)) {
		assert(h != 0);
		result = h;
	}
	return result;
}
//
//
//
#define MAX_PRIVNAME 32
#define MAX_PRIVSCAN 256

struct PRIVILAGENAME_MAPPING {
	WCHAR SymbolName[MAX_PRIVNAME];
	WCHAR PrivilegeName[MAX_PRIVNAME];
};

static const PRIVILAGENAME_MAPPING PrivilegeNameMapping[] = {
	{ L"SE_CREATE_TOKEN_NAME", SE_CREATE_TOKEN_NAME },
	{ L"SE_ASSIGNPRIMARYTOKEN_NAME", SE_ASSIGNPRIMARYTOKEN_NAME },
	{ L"SE_LOCK_MEMORY_NAME", SE_LOCK_MEMORY_NAME },
	{ L"SE_INCREASE_QUOTA_NAME", SE_INCREASE_QUOTA_NAME },
	{ L"SE_UNSOLICITED_INPUT_NAME", SE_UNSOLICITED_INPUT_NAME }, // no LUID?
	{ L"SE_MACHINE_ACCOUNT_NAME", SE_MACHINE_ACCOUNT_NAME },
	{ L"SE_TCB_NAME", SE_TCB_NAME },
	{ L"SE_SECURITY_NAME", SE_SECURITY_NAME },
	{ L"SE_TAKE_OWNERSHIP_NAME", SE_TAKE_OWNERSHIP_NAME },
	{ L"SE_LOAD_DRIVER_NAME", SE_LOAD_DRIVER_NAME },
	{ L"SE_SYSTEM_PROFILE_NAME", SE_SYSTEM_PROFILE_NAME },
	{ L"SE_SYSTEMTIME_NAME", SE_SYSTEMTIME_NAME },
	{ L"SE_PROF_SINGLE_PROCESS_NAME", SE_PROF_SINGLE_PROCESS_NAME },
	{ L"SE_INC_BASE_PRIORITY_NAME", SE_INC_BASE_PRIORITY_NAME },
	{ L"SE_CREATE_PAGEFILE_NAME", SE_CREATE_PAGEFILE_NAME },
	{ L"SE_CREATE_PERMANENT_NAME", SE_CREATE_PERMANENT_NAME },
	{ L"SE_BACKUP_NAME", SE_BACKUP_NAME },
	{ L"SE_RESTORE_NAME", SE_RESTORE_NAME },
	{ L"SE_SHUTDOWN_NAME", SE_SHUTDOWN_NAME },
	{ L"SE_DEBUG_NAME", SE_DEBUG_NAME },
	{ L"SE_AUDIT_NAME", SE_AUDIT_NAME },
	{ L"SE_SYSTEM_ENVIRONMENT_NAME", SE_SYSTEM_ENVIRONMENT_NAME },
	{ L"SE_CHANGE_NOTIFY_NAME", SE_CHANGE_NOTIFY_NAME },
	{ L"SE_REMOTE_SHUTDOWN_NAME", SE_REMOTE_SHUTDOWN_NAME },
	{ L"SE_UNDOCK_NAME", SE_UNDOCK_NAME },
	{ L"SE_SYNC_AGENT_NAME", SE_SYNC_AGENT_NAME },
	{ L"SE_ENABLE_DELEGATION_NAME", SE_ENABLE_DELEGATION_NAME },
	{ L"SE_MANAGE_VOLUME_NAME", SE_MANAGE_VOLUME_NAME },
	{ L"SE_IMPERSONATE_NAME", SE_IMPERSONATE_NAME },
	{ L"SE_CREATE_GLOBAL_NAME", SE_CREATE_GLOBAL_NAME },
	{ L"SE_TRUSTED_CREDMAN_ACCESS_NAME", SE_TRUSTED_CREDMAN_ACCESS_NAME },
	{ L"SE_RELABEL_NAME", SE_RELABEL_NAME },
	{ L"SE_INC_WORKING_SET_NAME", SE_INC_WORKING_SET_NAME },
	{ L"SE_TIME_ZONE_NAME", SE_TIME_ZONE_NAME },
	{ L"SE_CREATE_SYMBOLIC_LINK_NAME", SE_CREATE_SYMBOLIC_LINK_NAME },
	{ L"", L"" }
};

static bool _LookupPrivilegeName(const wchar_t * pSystemName, const PLUID Luid, const wchar_t ** ppSymbolName,
    wchar_t * pPrivilegeName, DWORD * pPrivilegeNameLength, wchar_t * pDisplayName, DWORD * pDisplayNameLength, BOOL noErrMsg) 
{
	DWORD language_id;
	int Index = -1;
	BOOL ret = LookupPrivilegeNameW(NULL, Luid, pPrivilegeName, pPrivilegeNameLength);
	if(!ret) {
		if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER && !noErrMsg)
			wprintf(L"LookupPrivilegeName failed - 0x%08x\n", GetLastError());
		goto cleanup;
	}
	ret = LookupPrivilegeDisplayName(NULL, pPrivilegeName, pDisplayName, pDisplayNameLength, &language_id);
	if(!ret) {
		if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER && !noErrMsg)
			wprintf(L"LookupPrivilegeDisplayName failed - 0x%08x\n", GetLastError());
		goto cleanup;
	}
	ret = FALSE;
	{
		const PRIVILAGENAME_MAPPING * p = PrivilegeNameMapping;
		for(Index = 0; p->SymbolName[0]!=0; ++p, ++Index) {
			if(wcscmp(pPrivilegeName, p->PrivilegeName) == 0) {
				ret = TRUE;
				break;
			}
		}
	}
	if(ret) {
		if(ppSymbolName)
			*ppSymbolName = PrivilegeNameMapping[Index].SymbolName;
	}
	else if(noErrMsg) {
		wprintf(L"%s not found\n", pPrivilegeName);
	}
cleanup:
	return LOGIC(ret);
}

static BOOL LookupPrivilegeValueEx(const wchar_t * pSystemName, const wchar_t * pName, PLUID Luid) 
{
	BOOL result = LookupPrivilegeValueW(pSystemName, pName, Luid);
	if(!result && GetLastError() == ERROR_NO_SUCH_PRIVILEGE) {
		const PRIVILAGENAME_MAPPING * p;
		for(p = PrivilegeNameMapping; p->SymbolName[0]!=0; ++p) {
			if(wcscmp(pName, p->SymbolName) == 0)
				return LookupPrivilegeValue(pSystemName, p->PrivilegeName, Luid);
		}
		SetLastError(ERROR_NO_SUCH_PRIVILEGE);
		result = FALSE;
	}
	return result;
}

/*static*/int SlProcess::CheckAccessTokenPrivilege(SPtrHandle token, const wchar_t * pPrivilegeName)
{
	int    result = privrError;
	LUID   luid;
	if(!LookupPrivilegeValueEx(NULL, pPrivilegeName, &luid) ) {
		//wprintf(L"LookupPrivilegeValue failed - 0x%08x\n", GetLastError());
		result = privrError;
		//return FALSE;
	}
	else {
		PRIVILEGE_SET privilege_set;
		privilege_set.Control = 0;
		privilege_set.PrivilegeCount = 1;
		privilege_set.Privilege[0].Luid = luid;
		privilege_set.Privilege[0].Attributes = 0; // not used
		BOOL _check;
		if(!PrivilegeCheck(token, &privilege_set, &_check) ) {
			//wprintf(L"PrivilegeCheck failed - 0x%08x\n", GetLastError());
			result = privrError;
			//return FALSE;
		}
		else if(_check) {
			result = privrEnabled;
			//*Privileged = 1;
		}
		else {
			TOKEN_PRIVILEGES tp;
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			tp.Privileges[0].Attributes = 0;
			if(!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL) ) {
				//wprintf(L"AdjustTokenPrivileges failed - 0x%08x\n", GetLastError());
				result = privrError;
				//return FALSE;
			}
			else {
				//*Privileged = (GetLastError()==ERROR_NOT_ALL_ASSIGNED) ? -1 : 0;
				result = (GetLastError()==ERROR_NOT_ALL_ASSIGNED) ? privrNotAssigned : privrDisabled;
			}
		}
	}
	//return TRUE;
	return result;
}

/*static*/bool SlProcess::EnableAccesTokenPrivilege(SPtrHandle token, const wchar_t * pName, bool enable)
{
	bool   ok = true;
	LUID   luid;
	if(!LookupPrivilegeValueEx(NULL, pName, &luid)) {
		//wprintf(L"LookupPrivilegeValue failed - 0x%08x\n", GetLastError());
		ok = false;
	}
	else {
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0; // not use SE_PRIVILEGE_REMOVED, just disable
		if(!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL) ) {
			//wprintf(L"AdjustTokenPrivileges failed - 0x%08x\n", GetLastError());
			ok = false;
		}
		else if(GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
			//wprintf(L"The process token does not have %s (%I64d).\n", pName, luid);
			ok = false;
		}
		//wprintf(L"%s (%I64d) is temporarily %s.\n", pName, luid, enable ? L"enabled" : L"disabled");
	}
	return ok;
}

//
// http://msdn.microsoft.com/en-us/library/ms722492(v=VS.85) InitLsaString
// http://msdn.microsoft.com/en-us/library/ms721874(v=vs.85).aspx
// http://msdn.microsoft.com/en-us/library/ms721863(v=vs.85).aspx
//
bool SlProcess::AddPrivilegeToAccessToken(SPtrHandle token, const wchar_t * pPrivilegeName) 
{
	NTSTATUS ret = 0;
	LSA_OBJECT_ATTRIBUTES obj_attr;//ObjectAttributes;
	LSA_HANDLE h_policy = NULL;
	//PSID Sid = NULL;
	LSA_UNICODE_STRING privilege[1];
	size_t priv_name_len = 0;
	TOKEN_USER * p_cur_user_sid = NULL;
	DWORD cur_user_sid_len = 0;
	// get current user SID from the token
	if(!GetTokenInformation(token, TokenUser, NULL, 0, &cur_user_sid_len) && GetLastError()!=ERROR_INSUFFICIENT_BUFFER) {
		wprintf(L"GetTokenInformation (size check) failed - 0x%08x\n", GetLastError());
		goto cleanup;
	}
	p_cur_user_sid = (TOKEN_USER *)HeapAlloc(GetProcessHeap(), 0, cur_user_sid_len);
	if(!p_cur_user_sid) {
		wprintf(L"HeapAlloc failed - 0x%08x\n", GetLastError());
		goto cleanup;
	}
	if(!GetTokenInformation(token, TokenUser, p_cur_user_sid, cur_user_sid_len, &cur_user_sid_len)) {
		wprintf(L"GetTokenInformation failed - 0x%08x\n", GetLastError());
		goto cleanup;
	}
	priv_name_len = StringCchLength(pPrivilegeName, MAX_PRIVNAME, &priv_name_len); // ??? StringCchLength возвращает HRESULT
	privilege[0].Buffer = (PWCHAR)pPrivilegeName;
	privilege[0].Length = static_cast<ushort>(priv_name_len * sizeof(WCHAR));
	privilege[0].MaximumLength = static_cast<ushort>((priv_name_len+1)*sizeof(WCHAR));
	ZeroMemory(&obj_attr, sizeof(obj_attr));
	ret = LsaOpenPolicy(NULL, &obj_attr, POLICY_ALL_ACCESS, &h_policy);
	/*
		SCESTATUS_SUCCESS 	The function succeeded.
		SCESTATUS_INVALID_PARAMETER 	One of the parameters passed to the function was not valid.
		SCESTATUS_RECORD_NOT_FOUND 	The specified record was not found in the security database.
		SCESTATUS_INVALID_DATA 	The function failed because some data was not valid.
		SCESTATUS_OBJECT_EXISTS 	The object already exists.
		SCESTATUS_BUFFER_TOO_SMALL 	The buffer passed into the function to receive data is not large enough to receive all the data.
		SCESTATUS_PROFILE_NOT_FOUND 	The specified profile was not found.
		SCESTATUS_BAD_FORMAT 	The format is not valid.
		SCESTATUS_NOT_ENOUGH_RESOURCE 	There is insufficient memory.
		SCESTATUS_ACCESS_DENIED 	The caller does not have sufficient privileges to complete this action.
		SCESTATUS_CANT_DELETE 	The function cannot delete the specified item.
		SCESTATUS_PREFIX_OVERFLOW 	A prefix overflow occurred.
		SCESTATUS_OTHER_ERROR 	An unspecified error has occurred.
		SCESTATUS_ALREADY_RUNNING 	The service is already running.
		SCESTATUS_SERVICE_NOT_SUPPORT 	The specified service is not supported.
		SCESTATUS_MOD_NOT_FOUND 	An attachment engine DLL listed in the registry either cannot be found or cannot be loaded.
		SCESTATUS_EXCEPTION_IN_SERVER 	An exception occurred in the server.
	*/
	if(ret != 0/*STATUS_SUCCESS*/) {
		ulong wr = LsaNtStatusToWinError(ret);
		wprintf(L"LsaOpenPolicy failed - 0x%08x\n", LsaNtStatusToWinError(ret));
		goto cleanup;
	}
	StringCchLength(pPrivilegeName, MAX_PRIVNAME, &priv_name_len);
	privilege[0].Buffer = (PWCHAR)pPrivilegeName;
	privilege[0].Length = static_cast<ushort>(priv_name_len * sizeof(WCHAR));
	privilege[0].MaximumLength = static_cast<ushort>((priv_name_len + 1)*sizeof(WCHAR));
	ret = LsaAddAccountRights(h_policy, p_cur_user_sid->User.Sid, privilege, 1);
	if(ret != 0/*STATUS_SUCCESS*/) {
		wprintf(L"LsaAddAccountRights failed - 0x%08x\n", LsaNtStatusToWinError(ret));
		goto cleanup;
	}
	wprintf(L"Privilege '%s' was assigned successfully.\n", pPrivilegeName);
	wprintf(L"To apply it to the token, re-log on the system.\n");
cleanup:
	if(h_policy) 
		LsaClose(h_policy);
	if(p_cur_user_sid) 
		HeapFree(GetProcessHeap(), 0, p_cur_user_sid);
	return (ret == 0/*STATUS_SUCCESS*/);
}

/*static*/bool SlProcess::CheckAndEnableAccesTokenPrivilege(SPtrHandle token, const wchar_t * pPrivilegeName)
{
	//const wchar_t * p_priv_symb = SE_ASSIGNPRIMARYTOKEN_NAME;
	int privr = 0;
	if(!isempty(pPrivilegeName)) {
		privr = SlProcess::CheckAccessTokenPrivilege(token, pPrivilegeName);
		if(privr == privrNotAssigned) {
			if(SlProcess::AddPrivilegeToAccessToken(token, pPrivilegeName)) {
				privr = SlProcess::CheckAccessTokenPrivilege(token, pPrivilegeName);
			}
		}
		if(privr == privrDisabled) {
			if(EnableAccesTokenPrivilege(token, pPrivilegeName, true)) {
				privr = SlProcess::CheckAccessTokenPrivilege(token, pPrivilegeName);
			}
		}
	}
	return (privr == privrEnabled);
}

SlProcess::ProcessPool::Entry::Entry() 
{
	THISZERO();
}

SlProcess::ProcessPool::Entry & SlProcess::ProcessPool::Entry::Z()
{
	THISZERO();
	return *this;
}

SlProcess::ProcessPool::ProcessPool() : SStrGroup()
{
}

bool SlProcess::ProcessPool::GetEntry(uint idx, SlProcess::ProcessPool::Entry & rEntry, SString * pExeFileNameUtf8) const
{
	bool    ok = true;
	CALLPTRMEMB(pExeFileNameUtf8, Z());
	if(idx < L.getCount()) {
		rEntry = L.at(idx);
		if(pExeFileNameUtf8) {
			GetS(rEntry.ExeFileNameUtf8P, *pExeFileNameUtf8);
		}
	}
	else
		ok = false;
	return ok;
}

const SlProcess::ProcessPool::Entry * SlProcess::ProcessPool::SearchByExeFileName(const char * pPatternUtf8, uint * pIdx)
{
	const SlProcess::ProcessPool::Entry * p_result = 0;
	uint   idx = 0;
	if(!isempty(pPatternUtf8)) {
		SString temp_buf;
		for(uint i = 0; !p_result && i < L.getCount(); i++) {
			const Entry & r_entry = L.at(i);
			GetS(r_entry.ExeFileNameUtf8P, temp_buf);
			if(temp_buf.IsEqiUtf8(pPatternUtf8)) {
				p_result = &r_entry;
				idx = i;
			}
		}
	}
	ASSIGN_PTR(pIdx, idx);
	return p_result;
}

/*static*/uint SlProcess::SearchProcessByName(const char * pPatternUtf8)
{
	uint prc_id = 0;
	if(!isempty(pPatternUtf8)) {
		ProcessPool pp;
		const uint plc = GetProcessList(pp);
		if(plc) {
			const ProcessPool::Entry * p_entry = pp.SearchByExeFileName(pPatternUtf8, 0);
			if(p_entry) {
				prc_id = p_entry->ProcessId;
			}
		}
	}
	return prc_id;
}

/*static*/uint SlProcess::Helper_GetProcessList(const char * pFileNameUtf8, ProcessPool & rList)
{
	uint   result = 0;
	HANDLE h = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(h) {
		SString name;
		PROCESSENTRY32W pe;
		pe.dwSize = sizeof(pe);
		if(::Process32FirstW(h, &pe)) do {
			name.CopyUtf8FromUnicode(pe.szExeFile, sstrlen(pe.szExeFile), 0);
			if(isempty(pFileNameUtf8) || name.IsEqiUtf8(pFileNameUtf8)) {
				ProcessPool::Entry new_entry;
				new_entry.ProcessId = pe.th32ProcessID;
				new_entry.ParentId = pe.th32ParentProcessID;
				new_entry.ThreadCount = pe.cntThreads;
				new_entry.PriClassBase = pe.pcPriClassBase;
				rList.AddS(name, &new_entry.ExeFileNameUtf8P);
				rList.L.insert(&new_entry);
				result++;
			}
		} while(::Process32NextW(h, &pe));
		::CloseHandle(h);
		h = 0;
	}
	return result;
}

/*static*/uint SlProcess::GetProcessList(ProcessPool & rList) // @v12.3.11 @construction
{
	return Helper_GetProcessList(0, rList);
}

/*static*/uint SlProcess::GetProcessListByName(const char * pPatternUtf8, ProcessPool & rList) // @v12.4.0 @construction
{
	return Helper_GetProcessList(pPatternUtf8, rList);
}

// example function
static void EnumPrivileges(SPtrHandle token, bool all) 
{
	bool  ret = false;
	DWORD token_len = 0;
	TOKEN_PRIVILEGES * p_token_priv = NULL;
	DWORD privilege_name_len = 256;
	DWORD display_name_len = 256;
	wchar_t * p_privilege_name = NULL;
	wchar_t * p_display_name = NULL;
	const wchar_t * p_symbol_name = NULL;
	// LUID = Locally Unique Identifier
	wprintf(L"-------------------------------------------------------------------------------------------------------\n");
	wprintf(L"   LUID                Symbol                           PrivilegeName                    DisplayName\n");
	wprintf(L"-------------------------------------------------------------------------------------------------------\n");
	if(!all) {
		if(!GetTokenInformation(token, TokenPrivileges, NULL, 0, &token_len) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			wprintf(L"GetTokenInformation (size check) failed - 0x%08x\n", GetLastError());
			goto cleanup;
		}
		p_token_priv = (TOKEN_PRIVILEGES *)HeapAlloc(GetProcessHeap(), 0, token_len);
		if(!p_token_priv) {
			wprintf(L"HeapAlloc failed - 0x%08x\n", GetLastError());
			goto cleanup;
		}
		if(!GetTokenInformation(token, TokenPrivileges, p_token_priv, token_len, &token_len) ) {
			wprintf(L"GetTokenInformation failed - 0x%08x\n", GetLastError());
			goto cleanup;
		}
	}
	else {
		p_token_priv = (TOKEN_PRIVILEGES *)HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD)+sizeof(LUID_AND_ATTRIBUTES)*MAX_PRIVSCAN);
		if(!p_token_priv) {
			wprintf(L"HeapAlloc failed - 0x%08x\n", GetLastError());
			goto cleanup;
		}
		p_token_priv->PrivilegeCount = MAX_PRIVSCAN;
		for(LONGLONG i = 0; i < MAX_PRIVSCAN; ++i) {
			p_token_priv->Privileges[i].Luid = *(PLUID)&i;
			p_token_priv->Privileges[i].Attributes = 0;
		}
	}
	for(DWORD i = 0; i < p_token_priv->PrivilegeCount; ++i) {
		do {
			delete [] p_privilege_name;
			delete [] p_display_name;
			p_privilege_name = new wchar_t[privilege_name_len];
			p_display_name = new wchar_t[display_name_len];
			ret = _LookupPrivilegeName(NULL, &p_token_priv->Privileges[i].Luid, &p_symbol_name, p_privilege_name, &privilege_name_len, p_display_name, &display_name_len, all);
		} while(!ret && GetLastError()==ERROR_INSUFFICIENT_BUFFER);
		if(ret) {
			wchar_t mark = 0;
			if(all) {
				/*
					// >0 Enabled
					// =0 Disabled
					// <0 Not assigned
				*/
				//LONG l = 0;
				int r = SlProcess::CheckAccessTokenPrivilege(token, p_privilege_name);
				mark = (r == SlProcess::privrDisabled) ? (mark = 'X') : ((r == SlProcess::privrEnabled) ? (mark = 'O') : '-');
			}
			else {
				mark = p_token_priv->Privileges[i].Attributes&SE_PRIVILEGE_ENABLED ? L'O' : L'X';
			}
			wprintf(L" %c 0x%08x`%08x %-32s %-32s %s\n", mark, p_token_priv->Privileges[i].Luid.HighPart,
			    p_token_priv->Privileges[i].Luid.LowPart, p_symbol_name, p_privilege_name, p_display_name);
		}
	}
cleanup:
	delete [] p_privilege_name;
	delete [] p_display_name;
	if(p_token_priv) 
		HeapFree(GetProcessHeap(), 0, p_token_priv);
}

SlProcess::SlProcess() : Flags(0), P_AppC(0)
{
}

SlProcess::~SlProcess()
{
	P_AppC = 0; // @notowned
}

bool SlProcess::SetAppContainer(AppContainer * pAppC)
{
	bool   ok = true;
	if(pAppC) {
		if(pAppC->IsValid())
			P_AppC = pAppC;
		else
			ok = false;
	}
	else
		P_AppC = 0;
	return ok;
}

bool SlProcess::SetFlags(uint flags)
{
	bool  ok = true;
	// @todo Необходимо проверить переданные с аргументом флаги на непротиворечивость.
	// Например, internal-флаги не должны передаваться (их надо игнорировать).
	Flags |= flags;
	return ok;
}

bool SlProcess::SetImpersUser(const char * pUserUtf8, const char * pPasswordUtf8)
{
	bool   ok = true;
	SString temp_buf;
	(temp_buf = pUserUtf8).Strip();
	THROW(temp_buf.IsLegalUtf8()); // @todo @err
	THROW(UserName.CopyFromUtf8(temp_buf));
	(temp_buf = pPasswordUtf8).Strip();
	THROW(temp_buf.IsLegalUtf8()); // @todo @err
	THROW(UserPw.CopyFromUtf8(temp_buf));
	CATCH
		UserName.Z();
		UserPw.Z();
		ok = false;
	ENDCATCH
	return ok;
}

bool SlProcess::SetImpersUserToken(SPtrHandle userToken)
{
	bool   ok = true;
	UserToken = userToken;
	return ok;
}

SlProcess::StartUpBlock::StartUpBlock() : Flags(0), ShowWindowFlags(0), P_AttributeList(0)
{
	ConsoleCharCount.Z();
}

SlProcess::StartUpBlock::~StartUpBlock()
{
	if(P_AttributeList) {
		DeleteProcThreadAttributeList(static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(P_AttributeList));
	}
}

bool SlProcess::Helper_SetParam(SStringU & rInner, const char * pParamUtf8)
{
	bool   ok = true;
	if(isempty(pParamUtf8)) {
		rInner.Z();
	}
	else {
		SString temp_buf(pParamUtf8);
		ok = temp_buf.IsLegalUtf8() ? rInner.CopyFromUtf8(temp_buf) : false;
	}
	return ok;
}

bool SlProcess::Helper_SetParam(SStringU & rInner, const wchar_t * pParam)
{
	bool   ok = true;
	if(isempty(pParam)) {
		rInner.Z();
	}
	else {
		rInner = pParam;
	}
	return ok;
}

bool SlProcess::Helper_SsAdd(StringSet & rInner, const char * pAddendumUtf8)
{
	bool   ok = true;
	if(isempty(pAddendumUtf8)) {
		;
	}
	else {
		SString temp_buf(pAddendumUtf8);
		if(temp_buf.IsLegalUtf8()) {
			// @todo Здесь надо как-то проверить добавляемую строку на предмет того, что 
			// ничего подобного в контейнере rInner нет, но в виду различной природы
			// контейнеров не понятно как это сделать унифицированным образом.
			rInner.add(temp_buf);
		}
		else
			ok = false;
	}
	return ok;
}

bool SlProcess::Helper_SsAdd(StringSet & rInner, const wchar_t * pAddendum)
{
	bool   ok = true;
	if(isempty(pAddendum)) {
		;
	}
	else {
		SString temp_buf;
		if(temp_buf.CopyUtf8FromUnicode(pAddendum, sstrlen(pAddendum), 1) && temp_buf.IsLegalUtf8()) {
			// @todo Здесь надо как-то проверить добавляемую строку на предмет того, что 
			// ничего подобного в контейнере rInner нет, но в виду различной природы
			// контейнеров не понятно как это сделать унифицированным образом.
			rInner.add(temp_buf);
		}
		else
			ok = false;
	}
	return ok;	
}

bool SlProcess::SetAppName(const char * pAppNameUtf8) { return Helper_SetParam(AppName, pAppNameUtf8); }
bool SlProcess::SetAppName(const wchar_t * pAppName) { return Helper_SetParam(AppName, pAppName); }
bool SlProcess::SetPath(const char * pPathUtf8) { return Helper_SetParam(Path, pPathUtf8); }
bool SlProcess::SetPath(const wchar_t * pPath) { return Helper_SetParam(Path, pPath); }
bool SlProcess::SetWorkingDir(const char * pWorkingDirUtf8) { return Helper_SetParam(WorkingDir, pWorkingDirUtf8); }
bool SlProcess::SetWorkingDir(const wchar_t * pWorkingDir) { return Helper_SetParam(WorkingDir, pWorkingDir); }

bool SlProcess::AddArg(const char * pArgUtf8) { return Helper_SsAdd(SsArgUtf8, pArgUtf8); }
bool SlProcess::AddArg(const wchar_t * pArg) { return Helper_SsAdd(SsArgUtf8, pArg); }
bool SlProcess::AddEnv(const char * pKeyUtf8, const char * pValUtf8)
{
	bool   ok = true;
	if(sstrchr(pKeyUtf8, '=') || sstrchr(pValUtf8, '=')) {
		ok = false; // @todo @err
	}
	else {
		SString temp_buf;
		temp_buf.CatEq(pKeyUtf8, pValUtf8);
		ok = Helper_SsAdd(SsEnvUtf8, temp_buf);
	}
	return ok;
}

bool   SlProcess::AddEnv(const wchar_t * pKey, const wchar_t * pVal)
{
	bool   ok = true;
	if(sstrchr(pKey, L'=') || sstrchr(pVal, L'=')) {
		ok = false; // @todo @err
	}
	else {
		SStringU temp_buf;
		temp_buf.CatEq(pKey, pVal);
		ok = Helper_SsAdd(SsEnvUtf8, temp_buf);
	}
	return ok;
}

static const LAssoc CreateProcessFlagsAssoc[] = {
	{ SlProcess::fBreakawayFromJob,           CREATE_BREAKAWAY_FROM_JOB },
	{ SlProcess::fDefaultErrorMode,           CREATE_DEFAULT_ERROR_MODE },
	{ SlProcess::fNewConsole,                 CREATE_NEW_CONSOLE },
	{ SlProcess::fNewProcessGroup,            CREATE_NEW_PROCESS_GROUP },
	{ SlProcess::fNoWindow,                   CREATE_NO_WINDOW },
	{ SlProcess::fProtectedProcess,           CREATE_PROTECTED_PROCESS },
	{ SlProcess::fPreserveCodeAuthzLevel,     CREATE_PRESERVE_CODE_AUTHZ_LEVEL },
	{ SlProcess::fSecureProcess,              CREATE_SECURE_PROCESS },
	{ SlProcess::fSeparateWowVdm,             CREATE_SEPARATE_WOW_VDM },
	{ SlProcess::fSharedWowVdm,               CREATE_SHARED_WOW_VDM },
	{ SlProcess::fSuspended,                  CREATE_SUSPENDED },
	{ SlProcess::fUnicodeEnvironment,         CREATE_UNICODE_ENVIRONMENT },
	{ SlProcess::fDebugOnlyThisProcess,       DEBUG_ONLY_THIS_PROCESS },
	{ SlProcess::fDebugProcess,               DEBUG_PROCESS },
	{ SlProcess::fDetachedProcess,            DETACHED_PROCESS },
	{ SlProcess::fExtendedStartupinfoPresent, EXTENDED_STARTUPINFO_PRESENT },
	{ SlProcess::fInheritParentAffinity,      INHERIT_PARENT_AFFINITY },
};

int __ParseWindowsUserForDomain(const wchar_t * pUserIn, SStringU & rUserName, SStringU & rDomainName); // @prototype(winprofile.cpp)

bool subprocess_create_named_pipe_helper(void ** ppRd, void ** ppWr) 
{
	bool   ok = true;
	SIntHandle h_rd;
	SIntHandle h_wr;
	const ulong pipeAccessInbound = 0x00000001;
	const ulong fileFlagOverlapped = 0x40000000;
	const ulong pipeTypeByte = 0x00000000;
	const ulong pipeWait = 0x00000000;
	const ulong genericWrite = 0x40000000;
	const ulong openExisting = 3;
	const ulong fileAttributeNormal = 0x00000080;
	//struct subprocess_security_attributes_s saAttr = {sizeof(saAttr), nullptr, 1};
	SECURITY_ATTRIBUTES sa_pipe;
	MEMSZERO(sa_pipe);
	sa_pipe.nLength = sizeof(sa_pipe);
	sa_pipe.bInheritHandle = TRUE;
	//char name[256] = {0};
	//static subprocess_tls long index = 0;
	//const long unique = index++;
	SString temp_buf;
	SString uniq_pipe_name;
	temp_buf.Z().Cat("slib_subprocess").CatChar('-').Cat(S_GUID(SCtrGenerate_), S_GUID::fmtPlain);
	SFile::MakeNamedPipeName(temp_buf, uniq_pipe_name);
/*
#if defined(_MSC_VER) && _MSC_VER < 1900
	#pragma warning(push, 1)
	#pragma warning(disable : 4996)
	_snprintf(name, sizeof(name) - 1, "\\\\.\\pipe\\sheredom_subprocess_h.%08lx.%08lx.%ld", GetCurrentProcessId(), GetCurrentThreadId(), unique);
	#pragma warning(pop)
#else
	snprintf(name, sizeof(name) - 1, "\\\\.\\pipe\\sheredom_subprocess_h.%08lx.%08lx.%ld", GetCurrentProcessId(), GetCurrentThreadId(), unique);
#endif
*/
	h_rd = CreateNamedPipeA(uniq_pipe_name, pipeAccessInbound|fileFlagOverlapped, pipeTypeByte | pipeWait, 1, 4096, 4096, 0, &sa_pipe);
	THROW(h_rd); // @todo @err
	h_wr = CreateFileA(uniq_pipe_name, genericWrite, 0, &sa_pipe, openExisting, fileAttributeNormal, nullptr);
	THROW(h_wr);
	ASSIGN_PTR(ppRd, h_rd);
	ASSIGN_PTR(ppWr, h_wr);
	CATCH
		if(h_rd.IsValid())
			::CloseHandle(h_rd);
		if(h_wr.IsValid())
			::CloseHandle(h_wr);
		ok = false;
	ENDCATCH
	return ok;
}

int SlProcess::Run(SlProcess::Result * pResult)
{
	int    ok = 0;
	FILE * f_captured_stdin  = 0; // @v11.9.3
	FILE * f_captured_stdout = 0; // @v11.9.3
	FILE * f_captured_stderr = 0; // @v11.9.3
	THROW(Path.NotEmpty()); // @todo @err
	{
		/*
		wchar_t cmd_line_[512];
		wchar_t working_dir_[512];
		SStringU cmd_line_u;
		SStringU working_dir_u;
		cmd_line_u.CopyFromUtf8(pPe->FullResolvedPath);
		STRNSCPY(cmd_line_, cmd_line_u);
		SFsPath ps(pPe->FullResolvedPath);
		ps.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
		working_dir_u.CopyFromUtf8(temp_buf.SetLastSlash());
		STRNSCPY(working_dir_, working_dir_u);
		SECURITY_ATTRIBUTES process_attr;
		SECURITY_ATTRIBUTES thread_attr;
		BOOL   inherit_handles = FALSE;
		DWORD  creation_flags = 0;
		void * p_env = 0;
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		MEMSZERO(si);
		si.cb = sizeof(si);
		MEMSZERO(pi);
		r = ::CreateProcessW(0, cmd_line_, 0, 0, inherit_handles, creation_flags, p_env, working_dir_, &si, &pi);
		*/
		SString temp_buf;
		SStringU temp_buf_u;
		const wchar_t * p_app_name = 0;
		wchar_t * p_cmd_line = 0;
		STempBuffer cmd_line_raw(1024 * sizeof(wchar_t));
		SECURITY_ATTRIBUTES * p_prc_attr_list = 0;
		SECURITY_ATTRIBUTES * p_thread_attr_list = 0;
		const BOOL inherit_handles = BIN(Flags & fInheritHandles);
		DWORD creation_flags = 0;
		void * p_env = 0;
		const wchar_t * p_curr_dir = 0;
		STARTUPINFOEXW startup_info;
		/*_Out_*/PROCESS_INFORMATION prc_info;
		//
		if(AppName.NotEmpty()) {
			p_app_name = AppName.ucptr();
		}
		{
			SStringU cmd_line_buf;
			cmd_line_buf.Cat(Path).Strip();
			if(SsArgUtf8.getCount()) {
				for(uint ssp = 0; SsArgUtf8.get(&ssp, temp_buf);) {
					THROW(temp_buf_u.CopyFromUtf8(temp_buf.Strip()));
					cmd_line_buf.Space().Cat(temp_buf_u);
				}
			}
			const size_t cmd_line_buf_size = (cmd_line_buf.Len() + 1) * sizeof(wchar_t);
			cmd_line_raw.Alloc(cmd_line_buf_size);
			memcpy(cmd_line_raw.vptr(), cmd_line_buf.ucptr(), cmd_line_buf_size);
			p_cmd_line = static_cast<wchar_t *>(cmd_line_raw.vptr());
		}
		if(SsEnvUtf8.getCount()) {
			
		}
		if(!WorkingDir.IsEmpty()) {
			p_curr_dir = WorkingDir.ucptr();
		}
		if(Flags) {
			for(uint i = 0; i < SIZEOFARRAY(CreateProcessFlagsAssoc); i++) {
				if(Flags & CreateProcessFlagsAssoc[i].Key) {
					if(!oneof2(Flags, fUnicodeEnvironment, fExtendedStartupinfoPresent)) {
						creation_flags |= CreateProcessFlagsAssoc[i].Val;
					}
				}
			}
		}
		//
		int create_proc_result = 0;
		{
			STempBuffer ptal_buf(256);
			MEMSZERO(startup_info);
			creation_flags &= ~EXTENDED_STARTUPINFO_PRESENT;
			if(P_AppC && P_AppC->IsValid()) {
				SIZE_T ptal_size = 0;
				SECURITY_CAPABILITIES sc;
				MEMSZERO(sc);
				sc.AppContainerSid = P_AppC->GetSid();
				::InitializeProcThreadAttributeList(nullptr, 1, 0, &ptal_size);
				THROW(ptal_buf.Alloc(ptal_size+32)); // 32 - insurance
				startup_info.lpAttributeList = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(ptal_buf.vptr());
				THROW(::InitializeProcThreadAttributeList(startup_info.lpAttributeList, 1, 0, &ptal_size));
				THROW(::UpdateProcThreadAttribute(startup_info.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES, &sc, sizeof(sc), nullptr, nullptr));
				startup_info.StartupInfo.cb = sizeof(STARTUPINFOEXW);
				creation_flags |= EXTENDED_STARTUPINFO_PRESENT;
			}
			else {
				startup_info.StartupInfo.cb = sizeof(STARTUPINFOW);
			}
			// @v11.9.3 @construction {
			{
				SECURITY_ATTRIBUTES sa_pipe;
				MEMSZERO(sa_pipe);
				sa_pipe.nLength = sizeof(sa_pipe);
				sa_pipe.bInheritHandle = TRUE;
				if(Flags & fCaptureStdIn) {
					void * rd;
					void * wr;
					THROW(::CreatePipe(&rd, &wr, &sa_pipe, 0)); // @todo @err
					THROW(::SetHandleInformation(wr, HANDLE_FLAG_INHERIT, 0)); // @todo @err
					{
						int fd = _open_osfhandle(reinterpret_cast<intptr_t>(wr), 0);
						if(fd != -1) {
							f_captured_stdin = _fdopen(fd, "wb");
							THROW(f_captured_stdin); // @todo @err
						}
						startup_info.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
						startup_info.StartupInfo.hStdInput = rd;
					}
				}
				if(Flags & fCaptureStdOut) {
					void * rd;
					void * wr;
					if(Flags & fCaptureReadAsync) {
						THROW(subprocess_create_named_pipe_helper(&rd, &wr));
					}
					else {
						THROW(::CreatePipe(&rd, &wr, &sa_pipe, 0)); // @todo @err
					}
					THROW(::SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0)); // @todo @err
					{
						int fd = _open_osfhandle(reinterpret_cast<intptr_t>(rd), 0);
						if(fd != -1) {
							f_captured_stdout = _fdopen(fd, "rb");
							THROW(f_captured_stdout); // @todo @err
						}
						startup_info.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
						startup_info.StartupInfo.hStdOutput = wr;
					}
				}
				if(Flags & fCaptureStdErr) {
					void * rd;
					void * wr;
					if(Flags & fCombinedStdErrStdOut) {
						f_captured_stderr = f_captured_stdout;
						startup_info.StartupInfo.hStdError = startup_info.StartupInfo.hStdOutput;
					}
					else {
						if(Flags & fCaptureReadAsync) {
							THROW(subprocess_create_named_pipe_helper(&rd, &wr));
						}
						else {
							THROW(::CreatePipe(&rd, &wr, &sa_pipe, 0)); // @todo @err
						}
						THROW(::SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0)); // @todo @err
						{
							int fd = _open_osfhandle(reinterpret_cast<intptr_t>(rd), 0);
							if(fd != -1) {
								f_captured_stderr = _fdopen(fd, "rb");
								THROW(f_captured_stderr); // @todo @err
							}
							startup_info.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
							startup_info.StartupInfo.hStdError = wr;
						}
					}
				}
			}
			// } @v11.9.3
			enum {
				runprocessmAsUser = 1,
				runprocessmWithToken,
				runprocessmWithLogon,
			};
			int method = runprocessmWithLogon;
			if(UserToken) {
				method = runprocessmWithToken;
				uint   logon_flags = 0;
				if(Flags & fLogonWithProfile)
					logon_flags |= LOGON_WITH_PROFILE;
				SSystem::WinUserBlock wub;
				wub.UserName = UserName;
				wub.Password = UserPw;
				uint   guhf = 0;
				BOOL   loaded_profile = FALSE;
				HANDLE h_cmd_pipe = 0;
				//PROFILEINFO profile_info;
				SPtrHandle caller_token = SlProcess::OpenCurrentAccessToken(/*TOKEN_ALL_ACCESS*/TOKEN_READ|TOKEN_WRITE|TOKEN_EXECUTE);
				//bool guhr = SSystem::GetUserHandle(wub, guhf, loaded_profile, profile_info, h_cmd_pipe);
				SlProcess::CheckAndEnableAccesTokenPrivilege(caller_token, SE_IMPERSONATE_NAME);
				//SPtrHandle callee_token = SSystem::Logon(0, UserName, UserPw, SSystem::logontypeInteractive, 0);
				//BOOL iplour = ImpersonateLoggedOnUser(UserToken);
				//if(iplour) {
					create_proc_result = ::CreateProcessWithTokenW(UserToken, logon_flags, p_app_name, p_cmd_line, 
						/*p_prc_attr_list, p_thread_attr_list, inherit_handles,*/
						creation_flags, p_env, p_curr_dir, reinterpret_cast<STARTUPINFOW *>(&startup_info), &prc_info);
				//}
			}
			else if(UserName.NotEmpty()) {
				if(method == runprocessmWithLogon) {
					uint   logon_flags = 0;
					if(Flags & fLogonWithProfile)
						logon_flags |= LOGON_WITH_PROFILE;
					SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
					SStringU & r_domain = SLS.AcquireRvlStrU();
					SStringU & r_user = SLS.AcquireRvlStrU();
					SStringU & r_pw = SLS.AcquireRvlStrU();
					r_pw = UserPw;
					//
					r_temp_buf_u = UserName;
					THROW(r_temp_buf_u.NotEmpty());
					__ParseWindowsUserForDomain(r_temp_buf_u, r_user, r_domain);
					create_proc_result = ::CreateProcessWithLogonW(r_user, r_domain, r_pw, logon_flags, p_app_name, p_cmd_line, 
						/*p_prc_attr_list, p_thread_attr_list, inherit_handles,*/
						creation_flags, p_env, p_curr_dir, reinterpret_cast<STARTUPINFOW *>(&startup_info), &prc_info);
				}
				else if(method == runprocessmWithToken) {
					;
				}
				else if(method == runprocessmAsUser) {
					SSystem::WinUserBlock wub;
					wub.UserName = UserName;
					wub.Password = UserPw;
					uint   guhf = 0;
					BOOL   loaded_profile = FALSE;
					HANDLE h_cmd_pipe = 0;
					//PROFILEINFO profile_info;
					SPtrHandle caller_token = SlProcess::OpenCurrentAccessToken(TOKEN_ALL_ACCESS);
					//bool guhr = SSystem::GetUserHandle(wub, guhf, loaded_profile, profile_info, h_cmd_pipe);
					//int privr = 0;
					SlProcess::CheckAndEnableAccesTokenPrivilege(caller_token, SE_INCREASE_QUOTA_NAME);
					SlProcess::CheckAndEnableAccesTokenPrivilege(caller_token, SE_ASSIGNPRIMARYTOKEN_NAME);
					SPtrHandle callee_token = SSystem::Logon(0, UserName, UserPw, SSystem::logontypeInteractive, 0);
					if(callee_token) {
						//callee_token = wub.H_User_;
						BOOL iplour = ImpersonateLoggedOnUser(callee_token);
						if(iplour) {
							create_proc_result = ::CreateProcessAsUserW(callee_token, p_app_name, p_cmd_line, p_prc_attr_list, p_thread_attr_list, inherit_handles, 
								creation_flags, p_env, p_curr_dir, reinterpret_cast<STARTUPINFOW *>(&startup_info), &prc_info);
						}
					}
				}
			}
			else {
				create_proc_result = ::CreateProcessW(p_app_name, p_cmd_line, p_prc_attr_list, p_thread_attr_list, inherit_handles, 
					creation_flags, p_env, p_curr_dir, reinterpret_cast<STARTUPINFOW *>(&startup_info), &prc_info);
			}
			::DeleteProcThreadAttributeList(startup_info.lpAttributeList);
		}
		if(create_proc_result) {
			if(pResult) {
				pResult->HProcess = prc_info.hProcess;
				pResult->HThread = prc_info.hThread;
				pResult->ProcessId = prc_info.dwProcessId;
				pResult->ThreadId = prc_info.dwThreadId;
				pResult->F_StdIn = f_captured_stdin;
				f_captured_stdin = 0;
				pResult->F_StdOut = f_captured_stdout;
				f_captured_stdout = 0;
				pResult->F_StdErr = f_captured_stderr;
				f_captured_stderr = 0;
			}
			ok = 1;
		}
	}
	CATCHZOK
	SFile::ZClose(&f_captured_stdin);
	SFile::ZClose(&f_captured_stdout);
	SFile::ZClose(&f_captured_stderr);
	return ok;
}

SlProcess::Result::Result() : ProcessId(0), ThreadId(0), F_StdIn(0), F_StdOut(0), F_StdErr(0)
{
}
		
SlProcess::Result::~Result()
{
	SFile::ZClose(&F_StdIn);
	SFile::ZClose(&F_StdOut);
	SFile::ZClose(&F_StdErr);
}

SlProcess::AppContainer::AppContainer()
{
}

SlProcess::AppContainer::~AppContainer()
{
}

bool SlProcess::AppContainer::IsValid() const
{
	return !!Sid;
}

bool SlProcess::AppContainer::Create(const char * pName)
{
	const WinApi_UserEnv_ProcPool * p_proc_pool = WinApi_UserEnv_ProcPool::GetInstance();
	assert(p_proc_pool);
	bool   ok = false;
	PSID   sid = 0;
	SStringU container_name;
	if(!isempty(pName)) {
		if(p_proc_pool) {
			if(container_name.CopyFromUtf8(pName, sstrlen(pName))) {
				HRESULT hr = -1;
				if(p_proc_pool->CreateAppContainerProfile_Func) {
					hr = p_proc_pool->CreateAppContainerProfile_Func(container_name, container_name, container_name, nullptr, 0, &sid);
					if(SUCCEEDED(hr)) {
						ok = true;
					}
					else {
						// see if AppContainer SID already exists
						if(p_proc_pool->DeriveAppContainerSidFromAppContainerName_Func) {
							hr = p_proc_pool->DeriveAppContainerSidFromAppContainerName_Func(container_name, &sid);
							if(SUCCEEDED(hr))
								ok = true;
						}
					}
				}
			}
		}
	}
	if(ok) {
		NameUtf8.CopyUtf8FromUnicode(container_name, container_name.Len(), 1);
		Sid = sid;
		GetFolder();
	}
	return ok;
}

bool SlProcess::AppContainer::Delete()
{
	bool   ok = false;
	if(NameUtf8.NotEmpty()) {
		SStringU container_name;
		container_name.CopyFromUtf8(NameUtf8);
		const WinApi_UserEnv_ProcPool * p_proc_pool = WinApi_UserEnv_ProcPool::GetInstance();
		if(p_proc_pool && p_proc_pool->DeleteAppContainerProfile_Func) {
			HRESULT hr = p_proc_pool->DeleteAppContainerProfile_Func(container_name);
			if(SUCCEEDED(hr))
				ok = true;
		}
	}
	return ok;
}

bool SlProcess::AppContainer::GetFolder()
{
	bool   ok = false;
	HRESULT hr = -1;
	if(!!Sid) {
		const WinApi_UserEnv_ProcPool * p_proc_pool = WinApi_UserEnv_ProcPool::GetInstance();
		if(p_proc_pool) {
			wchar_t * p_sid_str = 0;
			wchar_t * p_path = 0;
			::ConvertSidToStringSid(Sid, &p_sid_str);
			//((log += L"AppContainer SID:\r\n") += str) += L"\r\n";
			if(p_proc_pool->GetAppContainerFolderPath_Func) {
				hr = p_proc_pool->GetAppContainerFolderPath_Func(p_sid_str, &p_path);
				if(SUCCEEDED(hr)) {
					//((log += L"AppContainer folder: ") += path) += L"\r\n";
					FolderUtf8.CopyUtf8FromUnicode(p_path, sstrlen(p_path), 1);
					::CoTaskMemFree(p_path);
					ok = true;
				}
			}
			::LocalFree(p_sid_str);
			//
			{
				REGSAM da = 0;
				HKEY h_key = 0;
				if(p_proc_pool && p_proc_pool->GetAppContainerRegistryLocation_Func) {
					hr = p_proc_pool->GetAppContainerRegistryLocation_Func(da, &h_key);
					if(SUCCEEDED(hr)) {
						ok = true;
					}
				}
			}
		}
	}
	return ok;	
}

bool SlProcess::AppContainer::AllowPath(const char * pPathUtf8, uint accsf)
{
	bool   ok = true;
	SStringU path_u;
	THROW(Sid);
	THROW_S_S(!isempty(pPathUtf8), SLERR_INVPARAM, __FUNCTION__"/pPathUtf8");
	THROW(path_u.CopyFromUtf8Strict(pPathUtf8, sstrlen(pPathUtf8)));
	THROW(AllowNamedObjectAccess(path_u, SE_FILE_OBJECT, FILE_ALL_ACCESS));
	CATCHZOK
	return ok;
}

bool SlProcess::AppContainer::AllowRegistry(const char * pKeyUtf8, int keyType, uint accsf)
{
	bool   ok = true;
	SStringU key_u;
	THROW(Sid);
	THROW_S_S(!isempty(pKeyUtf8), SLERR_INVPARAM, __FUNCTION__"/pKeyUtf8");
	THROW(key_u.CopyFromUtf8Strict(pKeyUtf8, sstrlen(pKeyUtf8)));
	{
		int   _type = 0;
		if(keyType == WinRegKey::regkeytypWow64_64)
			_type = SE_REGISTRY_WOW64_64KEY;
		else if(keyType == WinRegKey::regkeytypWow64_32)
			_type = SE_REGISTRY_WOW64_32KEY;
		else 
			_type = SE_REGISTRY_KEY;
		THROW(AllowNamedObjectAccess(key_u, _type, FILE_ALL_ACCESS));
	}
	CATCHZOK
	return ok;
}

bool SlProcess::AppContainer::AllowNamedObjectAccess(const wchar_t * pName, /*SE_OBJECT_TYPE*/int type, /*ACCESS_MASK*/uint accessMask) 
{
	bool   ok = true;
	PACL   old_acl;
	PACL   new_acl = nullptr;
	DWORD  status = ERROR_SUCCESS;
	THROW(Sid); // @todo @err
	{
		EXPLICIT_ACCESSW access;
		access.grfAccessMode = GRANT_ACCESS;
		access.grfAccessPermissions = accessMask;
		access.grfInheritance = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
		access.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
		access.Trustee.pMultipleTrustee = nullptr;
		access.Trustee.ptstrName = reinterpret_cast<wchar_t *>(static_cast<void *>(Sid));
		access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		access.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
		status = GetNamedSecurityInfoW(pName, static_cast<SE_OBJECT_TYPE>(type), DACL_SECURITY_INFORMATION, nullptr, nullptr, &old_acl, nullptr, nullptr);
		THROW_S(status == ERROR_SUCCESS, SLERR_WINDOWS);
		status = SetEntriesInAclW(1, &access, old_acl, &new_acl);
		THROW_S(status == ERROR_SUCCESS, SLERR_WINDOWS);
		{
			STempBuffer name_buf((sstrlen(pName) + 16) * sizeof(*pName));
			THROW(name_buf.IsValid());
			sstrcpy(static_cast<wchar_t *>(name_buf.vptr()), pName);
			status = SetNamedSecurityInfoW(static_cast<wchar_t *>(name_buf.vptr()), static_cast<SE_OBJECT_TYPE>(type), DACL_SECURITY_INFORMATION, nullptr, nullptr, new_acl, nullptr);
			THROW_S(status == ERROR_SUCCESS, SLERR_WINDOWS);
		}
	}
	//assert(status == ERROR_SUCCESS);
	CATCHZOK
	if(new_acl)
		::LocalFree(new_acl);
	return (status == ERROR_SUCCESS);
}
//
//
//
//#pragma comment(lib, "netapi32.lib")
//#pragma comment(lib, "userenv.lib")

static HANDLE g_hStopEvent = NULL; // @global
static HANDLE g_hProcess = NULL; // @global

static void PrintWin32ErrorToString(LPCWSTR szMessage, DWORD dwErr)
{
	const int maxSite = 512;
	const LPCWSTR szFormat = L"%s hex: 0x%x dec: %d message: %s\n";
	LPCWSTR szDefaultMessage = L"<< unknown message for this error code >>";
	WCHAR wszMsgBuff[maxSite];
	DWORD dwChars;
	HINSTANCE hInst = NULL;
	dwChars = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dwErr, NULL, wszMsgBuff, maxSite, nullptr);
	if(!dwChars) {
		hInst = LoadLibraryW(L"Ntdsbmsg.dll");
		if(!hInst) {
			wprintf(szFormat, szMessage, dwErr, dwErr, szDefaultMessage);
		}
		dwChars = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, hInst, dwErr, NULL, wszMsgBuff, maxSite, nullptr);
		if(hInst) {
			FreeLibrary(hInst);
			hInst = NULL;
		}
	}
	wprintf(szFormat, szMessage, dwErr, dwErr, (dwChars ? wszMsgBuff : szDefaultMessage));
}

static bool WINAPI ConsoleHandler(DWORD signal)
{
	if(oneof5(signal, CTRL_C_EVENT, CTRL_CLOSE_EVENT, CTRL_BREAK_EVENT, CTRL_LOGOFF_EVENT, CTRL_SHUTDOWN_EVENT)) {
		if(!TerminateProcess(g_hProcess, S_OK)) {
			PrintWin32ErrorToString(L"ERROR: Could not terminate child process (needs to be terminated/closed manually) with error:", GetLastError());
		}
		SetEvent(g_hStopEvent);
	}
	return true;
}

int _DontRun__wmain(int argc, PWCHAR argv[])
{
	if(argc < 4) {
		//std::wcout << L"ERROR: Invalid number of arguments passed." << std::endl;
		//std::wcout << L"\tArg1: User Account (ex. Domain\\UserName)" << std::endl;
		//std::wcout << L"\tArg2: User Account Password" << std::endl;
		//std::wcout << L"\tArg3: Process to start" << std::endl;
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}
	DWORD status = S_OK;
	std::wstring userAccount = argv[1];
	std::wstring userPassword = argv[2];
	std::wstring cmdLine = argv[3];

	std::wstring userName = userAccount.substr(userAccount.find_first_of(L"\\") + 1, userAccount.length());
	std::wstring domainName = userAccount.substr(0, userAccount.find_first_of(L"\\"));

	PDOMAIN_CONTROLLER_INFOW pDomainControllerInfo = nullptr;
	LPUSER_INFO_4 pUserInfo = nullptr;
	PROFILEINFOW pProfileInfo;
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES tokenPrivileges;
	LPVOID pEnvironmentBlock = nullptr;
	STARTUPINFOW startupInfo;
	PROCESS_INFORMATION processInfo;
	PRIVILEGE_SET privileges;
	BOOL privCheckStatus;
	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		status = GetLastError();
		PrintWin32ErrorToString(L"ERROR: Cannot open current user token with error:", status);
		goto cleanup;
	}
	ZeroMemory(&tokenPrivileges, sizeof(tokenPrivileges));
	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if(!LookupPrivilegeValueW(nullptr, SE_ASSIGNPRIMARYTOKEN_NAME, &tokenPrivileges.Privileges[0].Luid)) {
		status = GetLastError();
		PrintWin32ErrorToString(L"ERROR: Cannot lookup privilege value (LUID) with error:", status);
		goto cleanup;
	}
	ZeroMemory(&privileges, sizeof(privileges));
	privileges.PrivilegeCount = 1;
	privileges.Control = PRIVILEGE_SET_ALL_NECESSARY;
	privileges.Privilege[0].Luid = tokenPrivileges.Privileges[0].Luid;
	privileges.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
	if(!PrivilegeCheck(hToken, &privileges, &privCheckStatus)) {
		status = GetLastError();
		PrintWin32ErrorToString(L"ERROR: Cannot check user privileges with error:", status);
		goto cleanup;
	}
	else {
		if(!privCheckStatus) {
			if(!AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, NULL, nullptr, nullptr)) {
				status = GetLastError();
				PrintWin32ErrorToString(L"ERROR: Cannot adjust privileges with error:", status);
				goto cleanup;
			}
			else if(GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
				status = GetLastError();
				PrintWin32ErrorToString(L"ERROR: Cannot adjust privileges with error:", status);
				//std::wcout << L"Open secpol.msc and got to \"Security Settings\" > \"Local Policies\" > \"User Rights Assignment\"" << std::endl;
				//std::wcout << L"From there, add the user under which this process is running to the \"Replace a process level token\" policy and log off and back on again (with that user)." << std::endl;
				goto cleanup;
			}
			else {
				CloseHandle(hToken);
				hToken = NULL;
			}
		}
	}
	if(!LogonUserW(userName.c_str(), domainName.c_str(), userPassword.c_str(), LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken)) {
		status = GetLastError();
		PrintWin32ErrorToString(L"ERROR: Cannot logon user with error:", status);
		goto cleanup;
	}
	status = DsGetDcNameW(nullptr, domainName.c_str(), nullptr, nullptr, NULL, &pDomainControllerInfo);
	if(status != ERROR_SUCCESS) {
		status = DsGetDcNameW(nullptr, domainName.c_str(), nullptr, nullptr, DS_FORCE_REDISCOVERY, &pDomainControllerInfo);
	if(status != ERROR_SUCCESS) {
		status = HRESULT_FROM_WIN32(status);
		PrintWin32ErrorToString(L"ERROR: Cannot find domain controller with error:", status);
		goto cleanup;
	}
	}
	status = NetUserGetInfo(pDomainControllerInfo->DomainControllerName, userName.c_str(), 4, reinterpret_cast<LPBYTE*>(&pUserInfo));
	if(status != ERROR_SUCCESS) {
		status = HRESULT_FROM_WIN32(status);
		PrintWin32ErrorToString(L"ERROR: Cannot get user info with error:", status);
		goto cleanup;
	}
	ZeroMemory(&pProfileInfo, sizeof(pProfileInfo));
	pProfileInfo.dwSize = sizeof(pProfileInfo);
	pProfileInfo.lpUserName = const_cast<LPWSTR>(userAccount.c_str());
	pProfileInfo.dwFlags = PI_NOUI;
	pProfileInfo.lpProfilePath = pUserInfo->usri4_profile;
	if(!LoadUserProfileW(hToken, &pProfileInfo)) {
		status = GetLastError();
		PrintWin32ErrorToString(L"ERROR: Cannot user profile with error:", status);
		goto cleanup;
	}
	if(!CreateEnvironmentBlock(&pEnvironmentBlock, hToken, false)) {
		status = GetLastError();
		PrintWin32ErrorToString(L"ERROR: Cannot create environment block with error:", status);
		goto cleanup;
	}
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.lpDesktop = const_cast<LPWSTR>(L"");
	ZeroMemory(&processInfo, sizeof(processInfo));
	if(!CreateProcessAsUserW(hToken, nullptr, const_cast<LPWSTR>(cmdLine.c_str()), nullptr,
		nullptr, false, CREATE_UNICODE_ENVIRONMENT, pEnvironmentBlock, nullptr, &startupInfo, &processInfo)){
		status = HRESULT_FROM_WIN32(GetLastError());
		PrintWin32ErrorToString(L"ERROR: Cannot create process with error:", status);
		goto cleanup;
	}
	else {
		//std::wcout << L"!!! SUCCESS !!! => Waiting (forever) for child porcess to exit ..." << std::endl;
		g_hProcess = processInfo.hProcess;
		g_hStopEvent = CreateEventW(NULL, true, false, nullptr);
		if(!g_hStopEvent) {
			status = GetLastError();
			PrintWin32ErrorToString(L"ERROR: Failed to create event with error:", status);
			goto cleanup;
		}
		if(!SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(ConsoleHandler), true)) {
			status = GetLastError();
			PrintWin32ErrorToString(L"ERROR: Could not set control handler with error:", status);
			goto cleanup;
		}
		//std::wcout << L"\tAlso waiting for \"CTRL+C\" to (force) terminate the child process and finish the program (just in case it has no UI or you can't see it for some reason) ..." << std::endl;
		//std::wcout << L"\tIf you are using a tool like PsExec.exe (or similar) to start this RunAs tool (program/exe), then, if you press \"CTRL+C\"," << std::endl;
		//std::wcout << L"it will terminate without being able to close the child process in some situations (non-interactive - running under gMSA for example) and so, " << std::endl;
		//std::wcout << L"it might be that you need to kill the child process manually - PID of the child process is: " << processInfo.dwProcessId << std::endl;
		const int waitHandleCount = 2;
		HANDLE hWaitForHandles[waitHandleCount];
		hWaitForHandles[0] = processInfo.hProcess;
		hWaitForHandles[1] = g_hStopEvent;
		status = WaitForMultipleObjects(waitHandleCount, hWaitForHandles, false, INFINITE);
		if(status == WAIT_OBJECT_0) {
			if(!GetExitCodeProcess(processInfo.hProcess, &status)) {
				status = GetLastError();
				PrintWin32ErrorToString(L"ERROR: Failed to get exit status of child process with error:", status);
			}
			else {
				PrintWin32ErrorToString(L"Child process succesfully existed with exit code:", status);
			}
		}
		else if(status == (WAIT_OBJECT_0 + 1)) {
			//std::wcout << L"Cancel event (Ctrl+C) was pressed, so it \"killed\" the child process and thus the exit status is irrelevant." << std::endl;
		}
		else {
			//std::wcout << L"Something went wrong while waiting on the child process to finish. This can be ignored in this case though ..." << std::endl;
		}
		if(startupInfo.hStdError) {
			::CloseHandle(startupInfo.hStdError);
			startupInfo.hStdError = NULL;
		}
		if(startupInfo.hStdInput) {
			CloseHandle(startupInfo.hStdInput);
			startupInfo.hStdInput = NULL;
		}
		if(startupInfo.hStdOutput) {
			CloseHandle(startupInfo.hStdOutput);
			startupInfo.hStdOutput = NULL;
		}
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		UnloadUserProfile(hToken, pProfileInfo.hProfile);
	}
cleanup:
	if(hToken) {
		CloseHandle(hToken);
		hToken = NULL;
	}
	if(pDomainControllerInfo) {
		NetApiBufferFree(pDomainControllerInfo);
		pDomainControllerInfo = nullptr;
	}
	if(pUserInfo) {
		NetApiBufferFree(pUserInfo);
		pUserInfo = nullptr;
	}
	if(pEnvironmentBlock) {
		DestroyEnvironmentBlock(pEnvironmentBlock);
		pEnvironmentBlock = nullptr;
	}
	if(g_hStopEvent) {
		CloseHandle(g_hStopEvent);
		g_hStopEvent = NULL;
	}
	return status;
}
