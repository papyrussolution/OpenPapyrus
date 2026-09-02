// WINSRVC.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2010, 2016, 2019, 2020, 2021, 2023, 2025, 2026
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
	WinServiceMngr(uint desiredAccess = SC_MANAGER_ALL_ACCESS)
	{
		H = OpenSCManagerW(NULL, NULL, desiredAccess/*SC_MANAGER_ALL_ACCESS*/);
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

#if 0 // {
#include <windows.h>
#include <winsvc.h>
#include <iostream>

bool IsServiceRunning(const std::wstring& serviceName) 
{
	SC_HANDLE hSCManager = OpenSCManager(
		nullptr,                 // локальный компьютер
		nullptr,                 // база данных активных служб
		SC_MANAGER_CONNECT); // запрашиваемые права доступа
	if(hSCManager == nullptr) {
		std::cerr << "OpenSCManager failed. Error: " << GetLastError() << std::endl;
		return false;
	}
	// Открываем службу с правом на запрос её статуса[reference:0]
	SC_HANDLE hService = OpenService(hSCManager, serviceName.c_str(), SERVICE_QUERY_STATUS/*достаточно для получения статуса[reference:1]*/);
	if(hService == nullptr) {
		std::cerr << "OpenService failed. Error: " << GetLastError() << std::endl;
		CloseServiceHandle(hSCManager);
		return false;
	}
	SERVICE_STATUS status;
	// Запрашиваем текущий статус службы[reference:2]
	if(!QueryServiceStatus(hService, &status)) {
		std::cerr << "QueryServiceStatus failed. Error: " << GetLastError() << std::endl;
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		return false;
	}
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	// Сравниваем полученный статус с состоянием "остановлена"[reference:3]
	return status.dwCurrentState != SERVICE_STOPPED;
}

int main() 
{
	if(IsServiceRunning(L"Spooler")) {
		std::wcout << L"Служба запущена." << std::endl;
	} 
	else {
		std::wcout << L"Служба НЕ запущена." << std::endl;
	}
	return 0;
}
#endif // } 0

/*static*/int WinService::GetStatus(const char * pServiceName)
{
	int   result = 0;
	if(!isempty(pServiceName)) {
		WinServiceMngr sm(SC_MANAGER_CONNECT);
		WinService s(sm, pServiceName, SERVICE_QUERY_STATUS);
		if(s.IsValid()) {
			result = s.QueryStatus();
		}
	}
	return result;
}

/*static*/int WinService::Start(const char * pServiceName)
{
	int    ok = 0;
	if(!isempty(pServiceName)) {
		WinServiceMngr sm;
		WinService s(sm, pServiceName, SERVICE_START);
		ok = s.Start();
	}
	return ok;
}

/*static*/int WinService::Stop(const char * pServiceName)
{
	int    ok = 0;
	if(!isempty(pServiceName)) {
		WinServiceMngr sm;
		WinService s(sm, pServiceName, SERVICE_STOP);
		ok = s.Stop();
	}
	return ok;
}

WinService::WinService(const WinServiceMngr & rMngr, const char * pServiceName, long desiredAccess) : P_ScMngr(&rMngr), H(0), Name(pServiceName)
{
	if(P_ScMngr->IsValid()) {
		H = ::OpenServiceW(*P_ScMngr, SUcSwitchW(pServiceName), desiredAccess);
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

int WinService::QueryStatus() // @v12.7.6 @construction
{
	int    result = 0;
	SERVICE_STATUS status;
	if(QueryServiceStatus(H, &status)) {
		result = status.dwCurrentState;
	}
	return result;
}

int WinService::Create(const char * pDisplayName, const char * pModuleName, const char * pLogin, const char * pPw)
{
	int    ok = 0;
	if(P_ScMngr->IsValid()) {
		const  wchar_t * p_login = (pLogin && *pLogin) ? SUcSwitchW(pLogin) : 0;
		const  wchar_t * p_pw = (p_login && pPw) ? SUcSwitchW(pPw) : 0;
		const  wchar_t * p_disp_name = pDisplayName ? SUcSwitchW(pDisplayName) : SUcSwitchW(Name.cptr());
		SString path;
		if(pModuleName) {
			path = pModuleName;
		}
		else
			SSystem::SGetModuleFileName(0, path);
		if(!H) {
			H = ::CreateServiceW(*P_ScMngr, SUcSwitchW(Name), p_disp_name,
    	    	SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        		SUcSwitchW(path), NULL, NULL, NULL, p_login, p_pw);
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
			if(::QueryServiceConfigW(H, &cfg, buf_sz, &bytes_needed)) {
				SString _path(SUcSwitchW(cfg.lpBinaryPathName));
				SString _path2(path);
				if(_path.CmpNC(_path2) != 0)
					to_upd = 1;
				else if(p_login) {
					if(!sstreq(p_login, cfg.lpServiceStartName))
						to_upd = 1;
				}
				if(to_upd) {
					if(::ChangeServiceConfigW(H, cfg.dwServiceType, cfg.dwStartType, cfg.dwErrorControl,
						SUcSwitchW(path), cfg.lpLoadOrderGroup, 0, cfg.lpDependencies,
						p_login ? p_login : cfg.lpServiceStartName, p_pw, cfg.lpDisplayName))
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
	return BIN(H && StartServiceW(H, 0, 0));
}

int WinService::Stop()
{
	int    ok = 0;
	SERVICE_STATUS r;
	if(H) {
		ok = BIN(ControlService(H, SERVICE_CONTROL_STOP, &r) || GetLastError() == ERROR_SERVICE_NOT_ACTIVE);
	}
	return ok;
}
