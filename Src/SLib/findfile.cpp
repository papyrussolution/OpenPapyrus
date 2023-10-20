// FINDFILE.CPP
// Copyright (c) A.Sobolev 2005, 2010, 2016, 2017, 2020, 2023
//
#include <slib-internal.h>
#pragma hdrstop

SFindFileParam::SFindFileParam() : Flags(0)
{
}

SFindFileParam::SFindFileParam(const char * pInitPath, const char * pFileNamePattern) : Flags(0)
{
	InitPath = pInitPath;
	FileNamePattern = pFileNamePattern;
}

static int Helper_SFindFile2(const SFindFileParam & rP, const SString & rBasePath, SFileEntryPool & rResult)
{
	assert(rBasePath.NotEmpty());
	int    ok = -1;
	SString path;
	SString inner_path;
	SDirEntry dir_entry;
	(path = rBasePath).Strip().SetLastSlash();
	if(rP.FileNamePattern.NotEmpty()) {
		(inner_path = path).Cat(rP.FileNamePattern);
		for(SDirec dir(inner_path, 0); dir.Next(&dir_entry) > 0;) {
			THROW(rResult.Add(path, dir_entry, 0));
		}
	}
	{
		(inner_path = path).CatChar('*').Dot().CatChar('*');
		for(SDirec dir(inner_path, 1); dir.Next(&dir_entry) > 0;) {
			if(!dir_entry.IsSelf() && !dir_entry.IsUpFolder()) {
				dir_entry.GetNameA(path, inner_path);
				MEMSZERO(dir_entry);
				THROW(Helper_SFindFile2(rP, inner_path, rResult)); // @recursion
			}
		}
	}
	CATCHZOK
	return ok;
}

int SFindFile2(const SFindFileParam & rP, SFileEntryPool & rResult)
{
	rResult.Z();
	int    ok = 1;
	SString base_path;
	if(rP.InitPath.NotEmpty()) {
		base_path = rP.InitPath;
		THROW(Helper_SFindFile2(rP, base_path, rResult));
	}
	else {
		for(int i = 'A'; i <= 'Z'; i++) {
			char path[MAX_PATH];
			path[0] = i;
			path[1] = ':';
			path[2] = '\\';
			path[3] = 0;
			uint drive_type = ::GetDriveType(SUcSwitch(path));
			if(oneof2(drive_type, DRIVE_FIXED, DRIVE_REMOTE)) {
				base_path.Z().CatChar(i).Colon().BSlash();
				THROW(Helper_SFindFile2(rP, base_path, rResult));
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SFindFile::SFindFile(const char * pPath /* =0 */, const char * pFileName /* =0 */) :
	P_Path(pPath), Flags(0), State(0), DirCount(0), FileCount(0)
{
	FileNamePattern = pFileName;
	//SubStr
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
	(inner_path = path).CatChar('*').Dot().CatChar('*');
	SDirec dir(inner_path, 1);
	SDirEntry dir_entry;
	while(ok && dir.Next(&dir_entry) > 0) {
		if(!dir_entry.IsSelf() && !dir_entry.IsUpFolder()) {
			dir_entry.GetNameA(path, inner_path);
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
	if(pParam->FileNamePattern.NotEmpty()) {
		(inner_path = path).Cat(pParam->FileNamePattern);
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
//
//
SFindFile_ToPool::SFindFile_ToPool(const char * pPath, const char * pFileName) : SFindFile(pPath, pFileName), P_UserContainer(0)
{
}

/*virtual*/int SFindFile_ToPool::CallbackProc(const char * pPath, SDirEntry * pEntry)
{
	if(P_UserContainer && pEntry) {
		//if(sstreqi_ascii(FileNamePattern.cptr(), pEntry->Name))
			P_UserContainer->Add(pPath, *pEntry, 0);
	}
	return 1;
}

int SFindFile_ToPool::Run(SFileEntryPool & rResult)
{
	int    ok = -1;
	rResult.Z();
	P_UserContainer = &rResult;
	ok = SFindFile::Run();
	return ok;
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
		char path[MAX_PATH];
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
