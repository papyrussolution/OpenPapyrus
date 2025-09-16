// WINSRVC.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2010, 2016, 2019, 2020, 2021, 2023, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <winsvc.h>
//
// Descr: Менеджер сервисов для Windows
// @used{class WinService}
//
class WinServiceMngr {
public:
	WinServiceMngr()
	{
		H = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	}
	~WinServiceMngr()
	{
		if(H)
			CloseServiceHandle(H);
	}
	operator SC_HANDLE () const { return H; }
	bool   IsValid() const { return LOGIC(H); }
private:
	SC_HANDLE H;
};
//
//
//
/*static*/int WinService::Install(const char * pServiceName, const char * pDisplayName, const char * pModuleName, const char * pLogin, const char * pPassword)
{
	WinServiceMngr sm;
	WinService s(sm, pServiceName);
	return s.Create(pDisplayName, pModuleName, pLogin, pPassword);
}

/*static*/int WinService::Uninstall(const char * pServiceName)
{
	WinServiceMngr sm;
	WinService s(sm, pServiceName, DELETE);
	return s.Delete();
}

/*static*/int WinService::Start(const char * pServiceName, int stop)
{
	WinServiceMngr sm;
	WinService s(sm, pServiceName, stop ? SERVICE_STOP : SERVICE_START);
	return stop ? s.Stop() : s.Start();
}

WinService::WinService(const WinServiceMngr & rMngr, const char * pServiceName, long desiredAccess) : P_ScMngr(&rMngr), H(0), Name(pServiceName)
{
	if(P_ScMngr->IsValid()) {
		H = ::OpenService(*P_ScMngr, SUcSwitch(pServiceName), desiredAccess); // @unicodeproblem
		if(!H) {
			DWORD last_err = GetLastError();
			if(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
				SLS.SetError(SLERR_WINSVC_SVCNEXISTS, pServiceName);
			else
				SLS.SetOsError(0, 0);
			SetLastError(last_err);
		}
	}
}

WinService::~WinService()
{
	if(H)
		CloseServiceHandle(H);
}

bool WinService::IsValid() const { return LOGIC(H); }

int WinService::Create(const char * pDisplayName, const char * pModuleName, const char * pLogin, const char * pPw)
{
	int    ok = 0;
	if(P_ScMngr->IsValid()) {
		const TCHAR * p_login = (pLogin && *pLogin) ? SUcSwitch(pLogin) : 0;
		const TCHAR * p_pw = (p_login && pPw) ? SUcSwitch(pPw) : 0;
		const TCHAR * p_disp_name = pDisplayName ? SUcSwitch(pDisplayName) : SUcSwitch(Name.cptr());
		SString path;
		if(pModuleName) {
			path = pModuleName;
		}
		else
			SSystem::SGetModuleFileName(0, path);
		if(!H) {
			H = ::CreateService(*P_ScMngr, SUcSwitch(Name), p_disp_name,
    	    	SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        		SUcSwitch(path), NULL, NULL, NULL, p_login, p_pw);
			ok = BIN(H);
		}
		else {
			/*
			BOOL ChangeServiceConfig(
  			SC_HANDLE hService,
  			DWORD dwServiceType,
  			DWORD dwStartType,
  			DWORD dwErrorControl,
  			LPCTSTR lpBinaryPathName,
  			LPCTSTR lpLoadOrderGroup,
  			LPDWORD lpdwTagId,
  			LPCTSTR lpDependencies,
  			LPCTSTR lpServiceStartName,
  			LPCTSTR lpPassword,
  			LPCTSTR lpDisplayName
			);
			*/
			const size_t buf_sz = 4096;
			QUERY_SERVICE_CONFIG * p_cfg = static_cast<QUERY_SERVICE_CONFIG *>(SAlloc::M(buf_sz));
			QUERY_SERVICE_CONFIG & cfg = *p_cfg;
			DWORD  bytes_needed = buf_sz;
			int    to_upd = 0;
			if(::QueryServiceConfig(H, &cfg, buf_sz, &bytes_needed)) {
				SString _path(SUcSwitch(cfg.lpBinaryPathName));
				SString _path2(path);
				if(_path.CmpNC(_path2) != 0)
					to_upd = 1;
				else if(p_login) {
					if(!sstreq(p_login, cfg.lpServiceStartName))
						to_upd = 1;
				}
				if(to_upd) {
					if(::ChangeServiceConfig(H, cfg.dwServiceType, cfg.dwStartType, cfg.dwErrorControl,
						SUcSwitch(path), cfg.lpLoadOrderGroup, 0, cfg.lpDependencies,
						p_login ? p_login : cfg.lpServiceStartName, p_pw, cfg.lpDisplayName)) // @unicodeproblem
						ok = 1;
				}
			}
			else
				ok = 0;
			SAlloc::F(p_cfg);
		}
	}
	return ok;
}

int WinService::Delete()
{
	int    ok = 0;
	if(H && P_ScMngr->IsValid()) {
		if(DeleteService(H)) {
			CloseServiceHandle(H);
			H = 0;
			ok = 1;
		}
	}
	return ok;
}

int WinService::Start()
{
	return BIN(H && StartService(H, 0, 0));
}

int WinService::Stop()
{
	int    ok = 0;
	SERVICE_STATUS r;
	if(H)
		ok = BIN(ControlService(H, SERVICE_CONTROL_STOP, &r) || GetLastError() == ERROR_SERVICE_NOT_ACTIVE);
	return ok;
}
