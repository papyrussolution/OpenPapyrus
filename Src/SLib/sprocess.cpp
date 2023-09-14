// SPROCESS.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <UserEnv.h> // SlProcess
#include <sddl.h> // SlProcess
#include <AccCtrl.h>
#include <AclAPI.h>
//
// 
//
SlProcess::SlProcess() : Flags(0)
{
}

SlProcess::~SlProcess()
{
}

bool SlProcess::SetFlags(uint flags)
{
	bool  ok = true;
	// @todo Необходимо проверить переданные с аргументов флаги на непротиворечивость.
	// Например, internal-флаги не должны передаваться (их надо игнорировать).
	Flags |= flags;
	return ok;
}

static void foo()
{
	/*
	const char * p_app_name = 0;
	const char * p_cmd_line = 0;
		BOOL CreateProcessW(
		  [in, optional]      LPCWSTR               lpApplicationName,
		  [in, out, optional] LPWSTR                lpCommandLine,
		  [in, optional]      LPSECURITY_ATTRIBUTES lpProcessAttributes,
		  [in, optional]      LPSECURITY_ATTRIBUTES lpThreadAttributes,
		  [in]                BOOL                  bInheritHandles,
		  [in]                DWORD                 dwCreationFlags,
		  [in, optional]      LPVOID                lpEnvironment,
		  [in, optional]      LPCWSTR               lpCurrentDirectory,
		  [in]                LPSTARTUPINFOW        lpStartupInfo,
		  [out]               LPPROCESS_INFORMATION lpProcessInformation
		);
	*/
	/*

	fBreakawayFromJob           CREATE_BREAKAWAY_FROM_JOB
	fDefaultErrorMode           CREATE_DEFAULT_ERROR_MODE
	fNewConsole                 CREATE_NEW_CONSOLE
	fNewProcessGroup            CREATE_NEW_PROCESS_GROUP
	fNoWindow                   CREATE_NO_WINDOW
	fProtectedProcess           CREATE_PROTECTED_PROCESS
	fPreserveCodeAuthzLevel     CREATE_PRESERVE_CODE_AUTHZ_LEVEL
	fSecureProcess              CREATE_SECURE_PROCESS
	fSeparateWowVdm             CREATE_SEPARATE_WOW_VDM
	fSharedWowVdm               CREATE_SHARED_WOW_VDM
	fSuspended                  CREATE_SUSPENDED
	fUnicodeEnvironment         CREATE_UNICODE_ENVIRONMENT
	fDebugOnlyThisProcess       DEBUG_ONLY_THIS_PROCESS
	fDebugProcess               DEBUG_PROCESS
	fDetachedProcess            DETACHED_PROCESS
	fExtendedStartupinfoPresent EXTENDED_STARTUPINFO_PRESENT
	fInheritParentAffinity      INHERIT_PARENT_AFFINITY

	Creation Flags: 
		Constant/value 	Description
		CREATE_BREAKAWAY_FROM_JOB 0x01000000
			The child processes of a process associated with a job are not associated with the job.
			If the calling process is not associated with a job, this constant has no effect. 
			If the calling process is associated with a job, the job must set the JOB_OBJECT_LIMIT_BREAKAWAY_OK limit.
		CREATE_DEFAULT_ERROR_MODE 0x04000000
			The new process does not inherit the error mode of the calling process. Instead, the new process gets the default error mode.
			This feature is particularly useful for multithreaded shell applications that run with hard errors disabled.
			The default behavior is for the new process to inherit the error mode of the caller. Setting this flag changes that default behavior.
		CREATE_NEW_CONSOLE 0x00000010
			The new process has a new console, instead of inheriting its parent's console (the default). For more information, see Creation of a Console.
			This flag cannot be used with DETACHED_PROCESS.
		CREATE_NEW_PROCESS_GROUP 0x00000200
			The new process is the root process of a new process group. The process group includes all processes that are descendants of this root process. The process identifier of the new process group is the same as the process identifier, which is returned in the lpProcessInformation parameter. Process groups are used by the GenerateConsoleCtrlEvent function to enable sending a CTRL+BREAK signal to a group of console processes.
			If this flag is specified, CTRL+C signals will be disabled for all processes within the new process group.
			This flag is ignored if specified with CREATE_NEW_CONSOLE.
		CREATE_NO_WINDOW 0x08000000
			The process is a console application that is being run without a console window. Therefore, the console handle for the application is not set.
			This flag is ignored if the application is not a console application, or if it is used with either CREATE_NEW_CONSOLE or DETACHED_PROCESS.
		CREATE_PROTECTED_PROCESS 0x00040000
			The process is to be run as a protected process. The system restricts access to protected processes and the threads of protected processes. For more information on how processes can interact with protected processes, see Process Security and Access Rights.
			To activate a protected process, the binary must have a special signature. This signature is provided by Microsoft but not currently available for non-Microsoft binaries. There are currently four protected processes: media foundation, audio engine, Windows error reporting, and system. Components that load into these binaries must also be signed. Multimedia companies can leverage the first two protected processes. For more information, see Overview of the Protected Media Path.
			Windows Server 2003 and Windows XP: This value is not supported.
		CREATE_PRESERVE_CODE_AUTHZ_LEVEL 0x02000000
			Allows the caller to execute a child process that bypasses the process restrictions that would normally be applied automatically to the process.
		CREATE_SECURE_PROCESS 0x00400000
			This flag allows secure processes, that run in the Virtualization-Based Security environment, to launch.
		CREATE_SEPARATE_WOW_VDM 0x00000800
			This flag is valid only when starting a 16-bit Windows-based application. If set, the new process runs in a private Virtual DOS Machine (VDM). By default, all 16-bit Windows-based applications run as threads in a single, shared VDM. The advantage of running separately is that a crash only terminates the single VDM; any other programs running in distinct VDMs continue to function normally. Also, 16-bit Windows-based applications that are run in separate VDMs have separate input queues. That means that if one application stops responding momentarily, applications in separate VDMs continue to receive input. The disadvantage of running separately is that it takes significantly more memory to do so. You should use this flag only if the user requests that 16-bit applications should run in their own VDM.
		CREATE_SHARED_WOW_VDM 0x00001000
			The flag is valid only when starting a 16-bit Windows-based application. If the DefaultSeparateVDM switch in the Windows section of WIN.INI is TRUE, 
			this flag overrides the switch. The new process is run in the shared Virtual DOS Machine.
		CREATE_SUSPENDED 0x00000004
			The primary thread of the new process is created in a suspended state, and does not run until the ResumeThread function is called.
		CREATE_UNICODE_ENVIRONMENT 0x00000400
			If this flag is set, the environment block pointed to by lpEnvironment uses Unicode characters. Otherwise, the environment block uses ANSI characters.
		DEBUG_ONLY_THIS_PROCESS 0x00000002
			The calling thread starts and debugs the new process. It can receive all related debug events using the WaitForDebugEvent function.
		DEBUG_PROCESS 0x00000001
			The calling thread starts and debugs the new process and all child processes created by the new process. 
			It can receive all related debug events using the WaitForDebugEvent function.
			A process that uses DEBUG_PROCESS becomes the root of a debugging chain. 
			This continues until another process in the chain is created with DEBUG_PROCESS.
			If this flag is combined with DEBUG_ONLY_THIS_PROCESS, the caller debugs only the new process, not any child processes.
		DETACHED_PROCESS 0x00000008
			For console processes, the new process does not inherit its parent's console (the default). The new process can call the AllocConsole function at a later time to create a console. For more information, see Creation of a Console.
			This value cannot be used with CREATE_NEW_CONSOLE.
		EXTENDED_STARTUPINFO_PRESENT 0x00080000
			The process is created with extended startup information; the lpStartupInfo parameter specifies a STARTUPINFOEX structure.
			Windows Server 2003 and Windows XP: This value is not supported.
		INHERIT_PARENT_AFFINITY 0x00010000
			The process inherits its parent's affinity. If the parent process has threads in more than one processor group, 
			the new process inherits the group-relative affinity of an arbitrary group in use by the parent.
			Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP: This value is not supported.
	*/
	/*
		typedef struct _STARTUPINFOA {
			DWORD  cb;
			LPSTR  lpReserved;
			LPSTR  lpDesktop;
			LPSTR  lpTitle;
			DWORD  dwX;
			DWORD  dwY;
			DWORD  dwXSize;
			DWORD  dwYSize;
			DWORD  dwXCountChars;
			DWORD  dwYCountChars;
			DWORD  dwFillAttribute;
			DWORD  dwFlags;
			WORD   wShowWindow;
			WORD   cbReserved2;
			LPBYTE lpReserved2;
			HANDLE hStdInput;
			HANDLE hStdOutput;
			HANDLE hStdError;
		} STARTUPINFOA, *LPSTARTUPINFOA;
	*/
	//::CreateProcessW()
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

int SlProcess::Run(SlProcess::Result * pResult)
{
	int    ok = 0;
	THROW(Path.NotEmpty()); // @todo @err
	{
		/*
		wchar_t cmd_line_[512];
		wchar_t working_dir_[512];
		SStringU cmd_line_u;
		SStringU working_dir_u;
		cmd_line_u.CopyFromUtf8(pPe->FullResolvedPath);
		STRNSCPY(cmd_line_, cmd_line_u);
		SPathStruc ps(pPe->FullResolvedPath);
		ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, temp_buf);
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
		MEMSZERO(startup_info);
		startup_info.StartupInfo.cb = sizeof(STARTUPINFOW);
		creation_flags &= ~EXTENDED_STARTUPINFO_PRESENT;
		//
		int r = ::CreateProcessW(p_app_name, p_cmd_line, p_prc_attr_list, p_thread_attr_list, inherit_handles, 
			creation_flags, p_env, p_curr_dir, reinterpret_cast<STARTUPINFOW *>(&startup_info), &prc_info);
		if(r) {
			if(pResult) {
				pResult->HProcess = prc_info.hProcess;
				pResult->HThread = prc_info.hThread;
				pResult->ProcessId = prc_info.dwProcessId;
				pResult->ThreadId = prc_info.dwThreadId;
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

SlProcess::AppContainer::AppContainer()
{
}

SlProcess::AppContainer::~AppContainer()
{
}

bool SlProcess::AppContainer::Create(const char * pName)
{
	bool   ok = false;
	PSID   sid = 0;
	SStringU container_name;
	if(!isempty(pName)) {
		if(container_name.CopyFromUtf8(pName, sstrlen(pName))) {
			HRESULT hr = ::CreateAppContainerProfile(container_name, container_name, container_name, nullptr, 0, &sid);
			if(SUCCEEDED(hr)) {
				ok = true;
			}
			else {
				// see if AppContainer SID already exists
				hr = ::DeriveAppContainerSidFromAppContainerName(container_name, &sid);
				if(SUCCEEDED(hr))
					ok = true;
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
		HRESULT hr = DeleteAppContainerProfile(container_name);
		if(SUCCEEDED(hr))
			ok = true;
	}
	return ok;
}

bool SlProcess::AppContainer::GetFolder()
{
	bool   ok = false;
	if(!!Sid) {
		wchar_t * p_sid_str = 0;
		wchar_t * p_path = 0;
		::ConvertSidToStringSid(Sid, &p_sid_str);
		//((log += L"AppContainer SID:\r\n") += str) += L"\r\n";
		HRESULT hr = ::GetAppContainerFolderPath(p_sid_str, &p_path);
		if(SUCCEEDED(hr)) {
			//((log += L"AppContainer folder: ") += path) += L"\r\n";
			FolderUtf8.CopyUtf8FromUnicode(p_path, sstrlen(p_path), 1);
			::CoTaskMemFree(p_path);
			ok = true;
		}
		::LocalFree(p_sid_str);
	}
	return ok;	
}

bool SlProcess::AppContainer::AllowPath(const char * pPathUtf8, uint accsf)
{
	bool   ok = true;
	SStringU path_u;
	THROW(Sid);
	THROW(!isempty(pPathUtf8));
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
	THROW(!isempty(pKeyUtf8));
	THROW(key_u.CopyFromUtf8Strict(pKeyUtf8, sstrlen(pKeyUtf8)));
	THROW(AllowNamedObjectAccess(key_u, SE_FILE_OBJECT, FILE_ALL_ACCESS));
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
	do {
		EXPLICIT_ACCESS access;
		access.grfAccessMode = GRANT_ACCESS;
		access.grfAccessPermissions = accessMask;
		access.grfInheritance = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
		access.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
		access.Trustee.pMultipleTrustee = nullptr;
		access.Trustee.ptstrName = reinterpret_cast<wchar_t *>(static_cast<void *>(Sid));
		access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		access.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
		status = GetNamedSecurityInfo(pName, static_cast<SE_OBJECT_TYPE>(type), DACL_SECURITY_INFORMATION, nullptr, nullptr, &old_acl, nullptr, nullptr);
		THROW(status == ERROR_SUCCESS); // @todo @err
		status = SetEntriesInAcl(1, &access, old_acl, &new_acl);
		THROW(status == ERROR_SUCCESS); // @todo @err
		{
			STempBuffer name_buf((sstrlen(pName) + 16) * sizeof(*pName));
			THROW(name_buf.IsValid());
			sstrcpy(static_cast<wchar_t *>(name_buf.vptr()), pName);
			status = SetNamedSecurityInfo(static_cast<wchar_t *>(name_buf.vptr()), static_cast<SE_OBJECT_TYPE>(type), 
				DACL_SECURITY_INFORMATION, nullptr, nullptr, new_acl, nullptr);
			if(status != ERROR_SUCCESS)
				break;
		}
	} while(false);
	assert(status == ERROR_SUCCESS);
	CATCHZOK
	if(new_acl)
		::LocalFree(new_acl);
	return (status == ERROR_SUCCESS);
}

