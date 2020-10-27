// SETUPSUP.CPP
//
#include <errno.h>
#include <pp.h>

extern "C" __declspec(dllexport)
	void MyGetVersion(uint FAR* pfValue, uint FAR* psValue, uint FAR* ptValue, char * pFileName);
// переименовывает директорию из pOldname в pNewName
extern "C" __declspec(dllexport) int RenameDir(char *pOldname, char *pNewname);

#undef PPErrCode

int PPErrCode;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport)
	void MyGetVersion(uint FAR *pfValue, uint FAR* psValue, uint FAR* ptValue, char *pFileName)
{
	PPVersionInfo * p_version_info;
	p_version_info = new PPVersionInfo(pFileName);
	p_version_info->GetVersion(pfValue, psValue, ptValue, NULL);
	delete p_version_info;
}

extern "C" __declspec(dllexport) int RenameDir(char * pOldname, char * pNewname)
{
	int ok = 0;
	int result = rename(pOldname, pNewname);
	if(result == ENOENT)
		ok = -1;
	return ok;
}

/*
extern "C" __declspec(dllexport) int FindExeFiles()
{
	for(int i = 'A'; i <= 'Z'; i++) {
		char path[MAXPATH];
		path[0] = i;
		path[1] = ':';
		path[2] = '\\';
		path[3] = 0;
		uint drive_type = GetDriveType(path);
		if(drive_type == DRIVE_FIXED || drive_type == DRIVE_REMOTE) {
			pList[count] = i;
			count++;
		}
	}
}
*/
