// VersonSelector.cpp : Defines the entry point for the application.
//
#include "VersionSelector.h"
#include <pp.h>
#include <shlwapi.h>

extern "C" int __declspec(dllexport) SelectVersion(HWND hWndOwner, char* pPath, long flags);
//HINSTANCE hInstance;
static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#undef PPErrCode
int PPErrCode;
int __Semaph;

int FASTCALL PPSetError(int errCode) { return ((PPErrCode = errCode), 0); }
int FASTCALL PPSetError(int errCode, const char * pAddedMsg) { return ((PPErrCode = errCode), 0); }
int PPSetErrorNoMem() { return ((PPErrCode = PPERR_NOMEM), 0); }
int PPSetErrorSLib() { return ((PPErrCode = PPERR_SLIB), 0); }

struct RetVal {
	SString Path;
	int    OK;
};

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			{
				SString product_name;
				(product_name = "Papyrus Version Selector");
				SLS.Init(product_name, (HINSTANCE)hModule);
				SLS.LogMessage(0, "versel.dll -> DLL_PROCESS_ATTACH", 0); // @debug
			}
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

extern "C" int __declspec(dllexport) SelectVersion(HWND hWndOwner, char * pPath, long flags)
{
	SLS.LogMessage(0, "versel.dll -> SelectVersion", 0); // @debug
	RetVal * p_val = new RetVal;
	*pPath = 0;
	if(p_val) {
		p_val->Path = pPath;
		p_val->OK = -1;
		// @debug {
		{
			SString msg_buf;
			(msg_buf = "versel.dll -> SelectVersion(").Space().Cat((uint32)hWndOwner).CatDiv(',', 2).
				Cat(pPath).CatDiv(',', 2).Cat(flags).Cat(")");
			SLS.LogMessage(0, msg_buf, 0);
		}
		// } @debug 
		DialogBoxParam(SLS.GetHInst(), MAKEINTRESOURCE(DLGW_SELVERSION), hWndOwner, DialogProc, reinterpret_cast<LPARAM>(p_val));
		strcpy(pPath, p_val->Path);
		return p_val->OK;
	}
	else
		return -1;
}

struct FindVers : public SFindFile {
public:
	FindVers(HWND list, const char * pPath = 0, const char * pFileName = 0) : SFindFile(pPath, pFileName), List(list)
	{
		Dlg = ::GetParent(List);
		Label = ::GetDlgItem(Dlg, IDC_STATIC);
	}
	virtual int CallbackProc(const char* pPath, SDirEntry* pEntry);
private:
	HWND List;
	HWND Dlg;
	HWND Label;
};

static int GetVersBuf(const char * pPath, char * pBuf, size_t bufSize)
{
	int    ok = -1;
	if(pPath) {
		//char   ver_number[128];
		SString temp_buf;
		SString line_buf;
		(line_buf = pBuf).SetLastSlash().Cat("ppw.exe");
		PPVersionInfo vi(line_buf);
		// @v10.9.6 memzero(ver_number, sizeof(ver_number));
		// @v10.9.6 vi.GetVersionText(ver_number, sizeof(ver_number));
		vi.GetTextAttrib(vi.taiVersionText, temp_buf); // @v10.9.6 
		line_buf.ToLower().CatDiv(':', 2).Cat(/*ver_number*/temp_buf).CopyTo(pBuf, bufSize);
		ok = 1;
	}
	return ok;
}

int FindVers::CallbackProc(const char* pPath, SDirEntry * pEntry)
{
	MSG    msg;
	char   buf[MAX_PATH + 30];
	if(__Semaph) {
		::SetWindowText(Label, SUcSwitch(pPath));
		if(::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		if(pEntry) {
			if(!isempty(pEntry->Name)) {
				strcpy(buf, pPath);
				GetVersBuf(buf, buf, sizeof(buf));
				::SendMessage(List, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(SUcSwitch(buf)));
			}
		}
	}
	return __Semaph;
}

static int FillVersList(HWND list, int byDiscs)
{
	uint i = 0;
	::SendMessage(list, (UINT)LB_RESETCONTENT, 0, 0);
	if(byDiscs) {
		EnableWindow(GetDlgItem(GetParent(list), IDOK), FALSE);
		EnableWindow(GetDlgItem(GetParent(list), cmFindVersions), FALSE);
		::SetWindowText(GetDlgItem(GetParent(list), IDCANCEL), _T("Stop"));
		__Semaph = 1;
		for(int i = 'A'; i <= 'Z'; i++) {
			char path[MAXPATH];
			path[0] = i;
			path[1] = ':';
			path[2] = '\\';
			path[3] = 0;
			uint drive_type = ::GetDriveType(SUcSwitch(path));
			if(oneof2(drive_type, DRIVE_FIXED, DRIVE_REMOTE)) {
				FindVers param(list);
				param.P_Path = path;
				param.P_FileName = "ppw.exe";
				param.Run();
			}
			::SetWindowText(::GetDlgItem(GetParent(list), IDC_STATIC), NULL);
		}
		::SetWindowText(GetDlgItem(GetParent(list), IDCANCEL), _T("Cancel"));
		::EnableWindow(GetDlgItem(GetParent(list), cmFindVersions), TRUE);
	}
	else {
		char   buf[MAXPATH+30];
		SString reg_buf;
		char   param[] = "binpath";
		SString key_param;
		WinRegValue key_val;
		WinRegKey reg_key(HKEY_CURRENT_USER, "Software\\Papyrus\\System", 1);
		while(reg_key.EnumValues(&i, &key_param, &key_val)) {
			key_param.Trim(7);
			if(sstreqi_ascii(key_param, param)) {
				if(key_val.GetString(reg_buf)) {
					STRNSCPY(buf, reg_buf);
					GetVersBuf(buf, buf, sizeof(buf));
					::SendMessage(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(SUcSwitch(buf)));
				}
			}
		}
	}
	i = (uint)::SendMessage(list, LB_GETCOUNT, 0, 0);
	HWND hW = ::GetDlgItem(GetParent(list), IDOK);
	::SendMessage(list, LB_SETCURSEL, 0, 0);
	EnableWindow(hW, i);
	return 1;
}

// Message handler for VersionSelector dialog

static BOOL CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_INITDIALOG:
			::SetWindowLong(hWnd, GWL_USERDATA, lParam);
			FillVersList(GetDlgItem(hWnd, CTL_SELVERSION_LIST), 0);
			__Semaph = 0;
			return TRUE;
		case WM_COMMAND:
			{
				int wmId    = LOWORD(wParam); 
				int wmEvent = HIWORD(wParam); 
				switch(wmId) {
					case cmFindVersions:
						FillVersList(::GetDlgItem(hWnd, CTL_SELVERSION_LIST), 1);
						break;
					case IDOK:
						{
							int    sel_item;
							HWND   list = ::GetDlgItem(hWnd, CTL_SELVERSION_LIST);
							if(list && ::SendMessage(list, LB_GETCOUNT, 0, 0)) {	
								TCHAR buf[1024];
								RetVal * p_val = reinterpret_cast<RetVal *>(::GetWindowLong(hWnd, GWL_USERDATA));
								if(p_val) {
									sel_item = (int)::SendMessage(list, LB_GETCURSEL, 0, 0);
									::SendMessage(list, LB_GETTEXT, static_cast<WPARAM>(sel_item), reinterpret_cast<LPARAM>(buf));
									size_t pos = 0;
									StringSet ss(',', SUcSwitch(buf));
									ss.get(&pos, p_val->Path);
									//p_val->Path.StrStr("\\bin\\ppw.exe");
									if(p_val->Path.Search("\\bin\\ppw.exe", 0, 1, &pos)) {
										p_val->Path.Trim(pos);
									}
									else if(p_val->Path.Search("\\ppw.exe", 0, 1, &pos)) {
										p_val->Path.Trim(pos);
									}
									/*
									TCHAR * p_term = StrStrI(SUcSwitch(p_val->P_Path), _T("\\bin\\ppw.exe"));
									if(p_term)
										*p_term = 0;
									else {
										p_term = StrStrI(SUcSwitch(p_val->P_Path), _T("\\ppw.exe"));
										if(p_term)
											*p_term = 0;
									}
									*/
									p_val->OK = 1;
								}
								else if(p_val)
									p_val->OK = 0;
								::EndDialog(hWnd, LOWORD(wParam));
							}
						}
						break;
					case IDCANCEL:
						if(__Semaph)
							__Semaph = 0;
						else {
							RetVal * p_val = reinterpret_cast<RetVal *>(::GetWindowLong(hWnd, GWL_USERDATA));
							if(p_val)
								p_val->OK = -1;
							::EndDialog(hWnd, LOWORD(wParam));
							return TRUE;
						}
						break;
				}
			}
			break;
	}
	return FALSE;
}
