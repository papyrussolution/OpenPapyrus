// FINDFILE.CPP
// Copyright (c) A.Sobolev 2005, 2010, 2016, 2017, 2020
//
#include <slib-internal.h>
#pragma hdrstop

SFindFile::SFindFile(const char * pPath /* =0 */, const char * pFileName /* =0 */) :
	P_Path(pPath), P_FileName(pFileName), P_SubStr(0), Flags(0), DirCount(0), FileCount(0)
{
}

int SFindFile::CallbackProc(const char * pPath, SDirEntry * pEntry)
{
	return -1;
}

static int Helper_SFindFile(const SString * pPath, SFindFile * pParam)
{
	int    ok = 1;
	SString path, inner_path;
	if(!RVALUEPTR(path, pPath))
		path = pParam->P_Path;
	path.Strip().SetLastSlash();
	inner_path = path;
	inner_path.CatChar('*').Dot().CatChar('*');
	SDirec dir(inner_path, 1);
	SDirEntry dir_entry;
	while(ok && dir.Next(&dir_entry) > 0) {
		if(!dir_entry.IsSelf() && !dir_entry.IsUpFolder()) {
			(inner_path = path).Cat(dir_entry.FileName);
			pParam->DirCount++;
			//
			MEMSZERO(dir_entry);
			if(!pParam->CallbackProc(inner_path, &dir_entry))
				ok = 0;
			//
			else if(!Helper_SFindFile(&inner_path, pParam)) // @recursion
				ok = 0;
		}
	}
	if(pParam->P_FileName) {
		(inner_path = path).Cat(pParam->P_FileName);
		for(dir.Init(inner_path, 0); ok && dir.Next(&dir_entry) > 0;) {
			pParam->FileCount++;
			if(pParam->CallbackProc(path, &dir_entry) == 0)
				ok = 0;
		}
	}
	return ok;
}

int SFindFile::Run()
{
	return Helper_SFindFile(0, this);
}
//
// TEST
//
#ifdef TEST_SFINDFILE // {

#include <pp.h>

struct Test_SFindFile_ : public SFindFile {
	virtual int CallbackProc(const char * pPath, SDirEntry * pEntry)
	{
		SString path = pPath;
		if(pEntry->FileName[0] == 0)
			/*PPWaitMsg(path.ToOem())*/;
		else
			PPLogMessage(PPFILNAM_DEBUG_LOG, path.Cat(pEntry->FileName).Transf(CTRANSF_OUTER_TO_INNER), 0);
		return 1;
	}
};

int Test_SFindFile()
{
	MemLeakTracer mlt;
	PROFILE_START
	PPWaitStart();
	for(int i = 'A'; i <= 'Z'; i++) {
		char path[MAXPATH];
		path[0] = i;
		path[1] = ':';
		path[2] = '\\';
		path[3] = 0;
		uint drive_type = GetDriveType(path);
		if(drive_type == DRIVE_FIXED || drive_type == DRIVE_REMOTE) {
			Test_SFindFile_ param;
			param.P_Path = path;
			param.P_FileName = "ppw.exe";
			param.Run();
		}
	}
	PPWaitStop();
	PROFILE_END
	return 1;
}

#endif // } TEST_SFINDFILE
