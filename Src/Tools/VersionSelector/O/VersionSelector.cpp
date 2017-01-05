// VersonSelector.cpp : Defines the entry point for the application.
//
#include "VersionSelector.h"
#include <errno.h>
#include <pp.h>
#include <shlwapi.h>

extern "C" int __declspec(dllexport) SelectVersion(HWND hWndOwner, char* pPath, long flags);

HINSTANCE hInstance;

BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int GetVersBuf(const char* pPath, char* pBuf, size_t bufSize);
int FillVersList(HWND list, int byDiscs);

#undef PPErrCode
int PPErrCode;
int semaph;

struct RetVal {
	char* P_Path;
	int OK;
};

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	hInstance = (HINSTANCE)hModule;
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

extern "C" int __declspec(dllexport) SelectVersion(HWND hWndOwner, char* pPath, long flags)
{
	RetVal* val = new RetVal;
	*pPath = 0;
	if (val) {
		val->P_Path = pPath;
		val->OK = -1;
		DialogBoxParam(hInstance, MAKEINTRESOURCE(DLG_VERF), hWndOwner, DialogProc, (LPARAM)val);
		return val->OK;
	}
	else
		return -1;
}

struct FindVers : public SFindFile {
public:
	FindVers(HWND list, const char* pPath = 0, const char* pFileName = 0);
	virtual int SLAPI CallbackProc(const char* pPath, SDirEntry* pEntry);
private:
	HWND List;
	HWND Dlg;
	HWND Label;
};

FindVers::FindVers(HWND list, const char* pPath, const char* pFileName) : SFindFile(pPath, pFileName)
{
	List = list;
	Dlg = GetParent(List);
	Label = GetDlgItem(Dlg, IDC_STATIC);
}

int SLAPI FindVers::CallbackProc(const char* pPath, SDirEntry* pEntry)
{
	MSG msg;
	char buf[MAX_PATH + 30];
	if (semaph) {
		SetWindowText(Label, pPath);
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))	{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (*(pEntry->FileName)) {
			strcpy(buf, pPath);
			GetVersBuf(buf, buf, sizeof(buf));
			SendMessage(List, LB_ADDSTRING, (WPARAM)0L, (LPARAM)buf);
		}
	}
	return semaph;
}

int GetVersBuf(const char* pPath, char* pBuf, size_t bufSize)
{
	int ok = -1;
	PPVersionInfo* p_ver_inf = 0;
	if (pPath) {
		char buf[MAX_PATH + 30];
		setLastSlash(STRNSCPY(buf, pPath));
		strcat(buf, "ppw.exe");
		uint f = 0, s = 0, t = 0;
		p_ver_inf = new PPVersionInfo(buf);
		p_ver_inf->GetVersion(&f, &s, &t, NULL);
		sprintf(buf, "%s, %u, %u, %u", buf, f, s, t);
		if (pBuf)
			strnzcpy(pBuf, buf, (int) bufSize);
		ok = 1;
	}
	delete p_ver_inf;
	return ok;
}

int FillVersList(HWND list, int byDiscs)
{
	uint i = 0;
	SendMessage(list, (UINT)LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
	if (byDiscs) {
		EnableWindow(GetDlgItem(GetParent(list), IDOK), FALSE);
		EnableWindow(GetDlgItem(GetParent(list), cmFindVersions), FALSE);
		SetWindowText(GetDlgItem(GetParent(list), IDCANCEL), "Стоп");
		semaph = 1;
		for(int i = 'A'; i <= 'Z'; i++) {
			char path[MAXPATH];
			path[0] = i;
			path[1] = ':';
			path[2] = '\\';
			path[3] = 0;
			uint drive_type = GetDriveType(path);
			if (drive_type == DRIVE_FIXED || drive_type == DRIVE_REMOTE) {
				FindVers param(list);
				param.P_Path = path;
				param.P_FileName = "ppw.exe";
				param.Run();
			}
			SetWindowText(GetDlgItem(GetParent(list), IDC_STATIC), NULL);
		}
		SetWindowText(GetDlgItem(GetParent(list), IDCANCEL), "Отмена");
		EnableWindow(GetDlgItem(GetParent(list), cmFindVersions), TRUE);
	}
	else {
		char buf[MAXPATH+30];
		char param[] = "binpath";
		SString key_param;
		WinRegValue key_val;
		WinRegKey reg_key(HKEY_CURRENT_USER, "Software\\Papyrus\\System", 1);
		while(reg_key.EnumValues(&i, &key_param, &key_val)) {
			key_param.Trim(7);
			if (!stricmp(key_param, param)) {
				strcpy(buf, key_val.GetString());
				GetVersBuf(buf, buf, sizeof(buf));
				SendMessage(list, LB_ADDSTRING, (WPARAM)0L, (LPARAM)buf);
			}
		}
	}
	i = (uint)SendMessage(list, LB_GETCOUNT, 0, 0);
	HWND hW = GetDlgItem(GetParent(list), IDOK);
	SendMessage(list, LB_SETCURSEL, 0, 0);
	EnableWindow(hW, i);
	return 1;
}

// Message handler for VersionSelector dialog

BOOL CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG:
			{
				SetWindowLong(hWnd, GWL_USERDATA, (long)lParam);
				FillVersList(GetDlgItem(hWnd, CTL_VERF_LIST), 0);
				semaph = 0;
			}
			return TRUE;
		case WM_COMMAND:
			int wmId = 0, wmEvent = 0;
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			switch (wmId) {
				case cmFindVersions:
					FillVersList(GetDlgItem(hWnd, CTL_VERF_LIST), 1);
					break;
				case IDOK:
					{
						int sel_item;
						HWND list = GetDlgItem(hWnd, CTL_VERF_LIST);
						if (list && SendMessage(list, LB_GETCOUNT, 0, 0)) {	
							char buf[MAXPATH];
							RetVal* p_val = (RetVal*)GetWindowLong(hWnd, GWL_USERDATA);
							if (p_val && p_val->P_Path) {
								sel_item = (int)SendMessage(list, LB_GETCURSEL, 0, 0);
								SendMessage(list, LB_GETTEXT, (WPARAM)sel_item, (LPARAM)(LPCTSTR)buf);
								uint pos = 0;
								StringSet ss(",");
								ss.setBuf(buf, strlen(buf) + 1);
								ss.get(&pos, p_val->P_Path, MAXPATH);
								pos = (uint)StrStrI(p_val->P_Path, "\\bin\\ppw.exe");
								if (pos)
									*(char*)pos = 0;
								else {
									pos = (uint)StrStrI(p_val->P_Path, "\\ppw.exe");
									*(char*)pos = 0;
								}
								p_val->OK = 1;
							}
							else if (p_val)
								p_val->OK = 0;
							EndDialog(hWnd, LOWORD(wParam));
						}
					}
					break;
				case IDCANCEL:
					if (semaph)
						semaph = 0;
					else {
						RetVal* p_val = (RetVal*)GetWindowLong(hWnd, GWL_USERDATA);
						if (p_val)
							p_val->OK = -1;
						EndDialog(hWnd, LOWORD(wParam));
						return TRUE;
					}
					break;
			}
			break;
	}
	return FALSE;
}
