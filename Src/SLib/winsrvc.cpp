// WINSRVC.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2010, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <winsvc.h>
//
// Descr: Менеджер сервисов для Windows
// @used{class WinService}
//
class WinServiceMngr {
public:
	SLAPI  WinServiceMngr()
	{
		H = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	}
	SLAPI ~WinServiceMngr()
	{
		if(H)
			CloseServiceHandle(H);
	}
	SLAPI  operator SC_HANDLE () const { return H; }
	int    SLAPI IsValid() const { return H ? 1 : 0; }
private:
	SC_HANDLE H;
};
//
//
//
//static
int SLAPI WinService::Install(const char * pServiceName, const char * pDisplayName,
	const char * pModuleName, const char * pLogin, const char * pPassword)
{
	WinServiceMngr sm;
	WinService s(sm, pServiceName);
	return s.Create(pDisplayName, pModuleName, pLogin, pPassword);
}

//static
int SLAPI WinService::Uninstall(const char * pServiceName)
{
	WinServiceMngr sm;
	WinService s(sm, pServiceName, DELETE);
	return s.Delete();
}

//static
int SLAPI WinService::Start(const char * pServiceName, int stop)
{
	WinServiceMngr sm;
	WinService s(sm, pServiceName, stop ? SERVICE_STOP : SERVICE_START);
	return stop ? s.Stop() : s.Start();
}

SLAPI WinService::WinService(const WinServiceMngr & rMngr, const char * pServiceName, long desiredAccess)
{
	P_ScMngr = &rMngr;
	H = 0;
	Name = pServiceName;
	if(P_ScMngr->IsValid()) {
		H = OpenService(*P_ScMngr, pServiceName, desiredAccess);
		if(!H) {
			DWORD last_err = GetLastError();
			if(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
				SLS.SetError(SLERR_WINSVC_SVCNEXISTS, pServiceName);
			else
				SLS.SetOsError();
			SetLastError(last_err);
		}
	}
}

SLAPI WinService::~WinService()
{
	if(H)
		CloseServiceHandle(H);
}

int SLAPI WinService::IsValid() const
{
	return H ? 1 : 0;
}

int SLAPI WinService::Create(const char * pDisplayName, const char * pModuleName,
	const char * pLogin, const char * pPw)
{
	int    ok = 0;
	if(P_ScMngr->IsValid()) {
		const char * p_login = (pLogin && *pLogin) ? pLogin : 0;
		const char * p_pw = (p_login && pPw) ? pPw : 0;
		char   path[MAXPATH];
		if(pModuleName)
			STRNSCPY(path, pModuleName);
		else
			GetModuleFileName(NULL, path, sizeof(path));
		if(!H) {
			H = CreateService(*P_ScMngr, (const char *)Name, pDisplayName ? pDisplayName : (const char *)Name,
    	    	SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        		path, NULL, NULL, NULL, p_login, p_pw);
			ok = H ? 1 : 0;
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
			QUERY_SERVICE_CONFIG * p_cfg = (QUERY_SERVICE_CONFIG *)malloc(buf_sz);
			QUERY_SERVICE_CONFIG & cfg = *p_cfg;
			DWORD  bytes_needed = buf_sz;
			int    to_upd = 0;
			if(QueryServiceConfig(H, &cfg, buf_sz, &bytes_needed)) {
				SString _path = cfg.lpBinaryPathName, _path2 = path;
				if(_path.CmpNC(_path2) != 0)
					to_upd = 1;
				else if(p_login && stricmp(p_login, cfg.lpServiceStartName) != 0)
					to_upd = 1;
				if(to_upd) {
					if(ChangeServiceConfig(H, cfg.dwServiceType, cfg.dwStartType, cfg.dwErrorControl,
						path, cfg.lpLoadOrderGroup, 0, cfg.lpDependencies,
						p_login ? p_login : cfg.lpServiceStartName, p_pw, cfg.lpDisplayName))
						ok = 1;
				}
			}
			else
				ok = 0;
			free(p_cfg);
		}
	}
	return ok;
}

int SLAPI WinService::Delete()
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

int SLAPI WinService::Start()
{
	int    ok = (H && StartService(H, 0, 0)) ? 1 : 0;
	return ok;
}

int SLAPI WinService::Stop()
{
	int    ok = 0;
	SERVICE_STATUS r;
	if(H)
		ok = (ControlService(H, SERVICE_CONTROL_STOP, &r) || GetLastError() == ERROR_SERVICE_NOT_ACTIVE) ? 1 : 0;
	return ok;
}
