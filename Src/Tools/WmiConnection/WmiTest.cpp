// WmiTest.cpp : Defines the entry point for the console application.
//
#include "windows.h"
#include "tchar.h"
#include <iostream>
//
using std::cout;
using std::cin;
using std::hex;
using std::endl;
//
int	(*pLoginPasswordBox) (char*, char*, size_t);
void (*pGetLastMsg) (char*, size_t);
int (*pIsUncPath) (char*);
size_t (*pGetServerName) (const char*, char*, size_t);
int (*pGetLocalFromUnc) (const char*, char*, size_t);
int (*pConnect) (const char*, const char*, const char*, const char*);
int (*pWinSvcCreate) (
	const char*, const char*, const char*, int, int, int, const char*, const char*, const char*,
	const char*, const char*, int
);
int (*pWinSvcDelete) (const char*, int);
int (*pWinSvcStart) (const char*, int);
int (*pWinSvcStop) (const char*, int);
int (*pWinSvcExists) (const char*, int);
int (*pWinSvcGetProperty) (const char*, const char*, int, char*, size_t);
int (*pWinRegGetStr) (long, const char*, const char*, int, char*, size_t);
int (*pWinRegAddStr) (long, const char*, const char*, const char*, int);
int (*pWinRegDel) (long, const char*, int);
int (*pWinProcessCreate) (const char*, const char*);
void (*pRelease) ();
//
int _tmain(int argc, _TCHAR* argv[])
{
	if(argc>1) {
		HINSTANCE module = LoadLibrary(argv[1]);
		if(module) {
			cout << "Module \"" << argv[1] << "\" loaded, importing interface:" << endl;
			(FARPROC&) pLoginPasswordBox = GetProcAddress(module, "LoginPasswordBox");
			cout << "\tLoginPasswordBox:\t" << pLoginPasswordBox << endl;
			(FARPROC&) pGetLastMsg = GetProcAddress(module, "GetLastMsg");
			cout << "\tGetLastMsg:\t" << pGetLastMsg << endl;
			(FARPROC&) pIsUncPath = GetProcAddress(module, "IsUncPath");
			cout << "\tIsUncPath:\t" << pIsUncPath << endl;
			(FARPROC&) pGetServerName = GetProcAddress(module, "GetServerName");
			cout << "\tGetServerName:\t" << pGetServerName << endl;
			(FARPROC&) pGetLocalFromUnc = GetProcAddress(module, "GetLocalFromUnc");
			cout << "\tGetLocalFromUnc:\t" << pGetLocalFromUnc << endl;
			(FARPROC&) pConnect= GetProcAddress(module, "Connect");
			cout << "\tConnect:\t" << pConnect << endl;
			(FARPROC&) pWinSvcCreate = GetProcAddress(module, "WinSvcCreate");
			cout << "\tWinSvcCreate:\t" << pWinSvcCreate << endl;
			(FARPROC&) pWinSvcDelete = GetProcAddress(module, "WinSvcDelete");
			cout << "\tWinSvcDelete:\t" << pWinSvcDelete << endl;
			(FARPROC&) pWinSvcStart = GetProcAddress(module, "WinSvcStart");
			cout << "\tWinSvcStart:\t" << pWinSvcStart << endl;
			(FARPROC&) pWinSvcStop = GetProcAddress(module, "WinSvcStop");
			cout << "\tWinSvcStop:\t" << pWinSvcStop << endl;
			(FARPROC&) pWinSvcExists = GetProcAddress(module, "WinSvcExists");
			cout << "\tWinSvcExists:\t" << pWinSvcExists << endl;
			(FARPROC&) pWinSvcGetProperty = GetProcAddress(module, "WinSvcGetProperty");
			cout << "\tWinSvcGetProperty:\t" << pWinSvcGetProperty << endl;
			(FARPROC&) pWinRegGetStr = GetProcAddress(module, "WinRegGetStr");
			cout << "\tWinRegGetStr:\t" << pWinRegGetStr << endl;
			(FARPROC&) pWinRegAddStr = GetProcAddress(module, "WinRegAddStr");
			cout << "\tWinRegAddStr:\t" << pWinRegAddStr << endl;
			(FARPROC&) pWinRegDel = GetProcAddress(module, "WinRegDel");
			cout << "\tWinRegDel:\t" << pWinRegDel << endl;
			(FARPROC&) pWinProcessCreate = GetProcAddress(module, "WinProcessCreate");
			cout << "\tWinProcessCreate:\t" << pWinProcessCreate << endl;
			(FARPROC&) pRelease = GetProcAddress(module, "Release");
			cout << "\tRelease:\t" << pRelease << endl;
			char buff[0x80];
			// connection test
			char wmi_server[0x40], win_user[0x40], win_pwd[0x40];
			cout << "Input WMI server name:\t"; cin >> wmi_server;
			cout << "LoginPasswordBox() returned: " << pLoginPasswordBox(win_user, win_pwd, 0x40) << endl;
			cout << "Connect() returned: " << pConnect(wmi_server, win_user, win_pwd, NULL) << endl;
			char msg[0x100]; pGetLastMsg(msg, 0x100);
			cout << msg << endl;
			// converting from UNC path to local path on remote server test
			char unc_path[0x100], s[0x100];
			cout << "Input some UNC path:\t"; cin >> unc_path;
			cout << pGetLocalFromUnc(unc_path, s, 0x100) << '\t' << s << endl;
			// windows services tests
			char svc_name[0x20], svc_path[0x80], svc_display_name[0x40];
			cout << "Input name for new a windows service:\t"; cin >> svc_name;
			cout << "Input path to executable for a new windows service:\t"; cin >> svc_path;
			cout << "Input display name for a new windows service:\t"; cin >> svc_display_name;
			pLoginPasswordBox(win_user, win_pwd, 0x40);
			cout << "WinSvcCreate:\t" << pWinSvcCreate(svc_name, svc_display_name, svc_path, 0x10, 1, 1, win_user, win_pwd, NULL, NULL, NULL, 1) << endl;
			pGetLastMsg(msg, 0x100); MessageBox(NULL, msg, "message", MB_OK);
			cout << "WinSvcStart:\t" << pWinSvcStart(svc_name, 1) << endl;
			pGetLastMsg(msg, 0x100); MessageBox(NULL, msg, "message", MB_OK);
			cout << "WinSvcStop:\t" << pWinSvcStop(svc_name, 1) << endl;
			pGetLastMsg(msg, 0x100); MessageBox(NULL, msg, "message", MB_OK);
			cout << "WinSvcGetProperty:\t" << pWinSvcGetProperty(svc_name, "PathName", 1, buff, 0x100) << '\t' << buff << endl;
			pGetLastMsg(msg, 0x100); MessageBox(NULL, msg, "message", MB_OK);
			cout << "WinSvcDelete:\t" << pWinSvcDelete(svc_name, 1) << endl;
			pGetLastMsg(msg, 0x100); MessageBox(NULL, msg, "message", MB_OK);
			// windows registry tests
			cout << "WinRegGetStr:\t";
			cout << pWinRegGetStr(0x80000002, "software\\javasoft\\java runtime environment\\1.6", "javahome", 0, buff, 0x80);
			cout << '\t' << buff << endl;
			cout << "WinRegAddStr:\t";
			cout << pWinRegAddStr(0x80000002, "software\\papyrus\\blablabla", "yohoho", "and the bottle of roma", 1) << endl;
			cout << "WinRegGetStr:\t";
			cout << pWinRegGetStr(0x80000002, "software\\papyrus\\blablabla", "yohoho", 1, buff, 0x80);
			cout << '\t' << buff << endl;
			cout << "WinRegDel:\t";
			cout << pWinRegDel(0x80000002, "software\\papyrus\\blablabla", 1) << endl;
			// creating windows processes test
			pWinProcessCreate("regedit", NULL);
			// finish
			pRelease();
			FreeLibrary(module);
		}
		else
			cout << "Cann't load" << argv[1] << endl;
	}
	return 0;
}

