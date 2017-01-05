// PKZIP.CPP
// Copyright (c) A.Starodub 2003, 2010, 2012, 2015
//
#include <slib.h>
#pragma hdrstop
#include <process.h>

int SLAPI PKUnzip(const char * pSrc, const char * pDest, const char * pZipFile)
{
	int    ok = 0;
	if(pSrc && pDest && pZipFile) {
		char cmd[MAXPATH];
		char zip_file_path[MAXPATH];

		strnzcpy(zip_file_path, pSrc, MAXPATH);
		setLastSlash(zip_file_path);
		STRNSCPY(cmd, zip_file_path);
		strcat(zip_file_path, pZipFile);
		strcat(cmd, "pkunzip.exe");
		THROW(fileExists(zip_file_path));
#ifdef __WIN32__
		THROW(_spawnl(_P_WAIT, cmd, "-e", zip_file_path, pDest, NULL) != -1);
#else
		THROW(spawnl(P_WAIT, cmd, "-e", zip_file_path, pDest, NULL) != -1);
#endif
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PKZip(const char * pSrcPath, const char * pDestPath, const char * pZipDir)
{
	int ok = 1;
	SString cmd;
	SString src_path, dest_path, zip_path, file_name;
	file_name = "7z.exe";
	(zip_path = pZipDir).SetLastSlash().Cat(file_name);
	THROW(fileExists(pSrcPath));
	THROW(fileExists(pZipDir));
	src_path.CatQStr(pSrcPath);
	dest_path.CatQStr(pDestPath);
	cmd = zip_path;
	THROW(_spawnl(_P_WAIT, (const char*)cmd, (const char*)file_name, "a", (const char*)dest_path, (const char*)src_path, NULL) != -1);
	CATCHZOK
	return ok;
}
