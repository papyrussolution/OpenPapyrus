// DTI.CPP
// Copyright (c) V.Antonov 2004, 2005, 2008, 2010, 2015, 2017
//
// DTI
//
#include <db.h>
#pragma hdrstop
#define BTI_WIN_32
#include <psql\dticonst.h>
/*
#include <psql\btitypes.h>
#include <psql\catalog.h>
#include <psql\ddf.h>
#include <psql\connect.h>
#include <psql\config.h>
#include <psql\dtilicense.h>
//#include <psql\monitor.h>
*/
// ..\lib\w3dbav80.lib

PT_PvConnectServer       PervasiveDBCatalog::PvConnectServer = 0;
PT_PvDisconnect          PervasiveDBCatalog::PvDisconnect = 0;
PT_PvStart               PervasiveDBCatalog::PvStart = 0;
PT_PvStop                PervasiveDBCatalog::PvStop = 0;
PT_PvCreateDatabase      PervasiveDBCatalog::PvCreateDatabase = 0;
PT_PvCreateDSN           PervasiveDBCatalog::PvCreateDSN = 0;
PT_PvGetOpenFilesData    PervasiveDBCatalog::PvGetOpenFilesData = 0;
PT_PvGetFileHandlesData  PervasiveDBCatalog::PvGetFileHandlesData = 0;
PT_PvFreeOpenFilesData   PervasiveDBCatalog::PvFreeOpenFilesData = 0;
PT_PvGetOpenFileName     PervasiveDBCatalog::PvGetOpenFileName = 0;
PT_PvGetFileInfo         PervasiveDBCatalog::PvGetFileInfo = 0;
PT_PvGetFileHandleInfo   PervasiveDBCatalog::PvGetFileHandleInfo = 0;
PT_PvGetServerName       PervasiveDBCatalog::PvGetServerName = 0;

SDynLibrary * PervasiveDBCatalog::P_Lib = 0;

//static
int PervasiveDBCatalog::PtLoad()
{
	int    ok = -1;
	SETIFZ(P_Lib, new SDynLibrary("w3dbav80.dll"));
	if(P_Lib->IsValid()) {
		if(PvStart == 0) {
			#define LOAD_PROC(proc) proc = (PT_##proc)P_Lib->GetProcAddr(#proc)
			LOAD_PROC(PvConnectServer);
			LOAD_PROC(PvDisconnect);
			LOAD_PROC(PvStart);
			LOAD_PROC(PvStop);
			LOAD_PROC(PvCreateDatabase);
			LOAD_PROC(PvCreateDSN);
			LOAD_PROC(PvGetOpenFilesData);
			LOAD_PROC(PvGetFileHandlesData);
			LOAD_PROC(PvFreeOpenFilesData);
			LOAD_PROC(PvGetOpenFileName);
			LOAD_PROC(PvGetFileInfo);
			LOAD_PROC(PvGetFileHandleInfo);
			LOAD_PROC(PvGetServerName);
			#undef LOAD_PROC
			ok = 1;
		}
		else
			ok = -1;
	}
	else
		ok = 0;
	return ok;
}

//static
int PervasiveDBCatalog::PtRelease()
{
	int    ok = -1;
	if(P_Lib) {
		#define UNLOAD_PROC(proc) proc = 0
		UNLOAD_PROC(PvConnectServer);
		UNLOAD_PROC(PvDisconnect);
		UNLOAD_PROC(PvStart);
		UNLOAD_PROC(PvStop);
		UNLOAD_PROC(PvCreateDatabase);
		UNLOAD_PROC(PvCreateDSN);
		UNLOAD_PROC(PvGetOpenFilesData);
		UNLOAD_PROC(PvGetFileHandlesData);
		UNLOAD_PROC(PvFreeOpenFilesData);
		UNLOAD_PROC(PvGetOpenFileName);
		UNLOAD_PROC(PvGetFileInfo);
		UNLOAD_PROC(PvGetFileHandleInfo);
		UNLOAD_PROC(PvGetServerName);
		#undef UNLOAD_PROC
		ZDELETE(P_Lib);
		ok = 1;
	}
	return ok;
}

PervasiveDBCatalog::PervasiveDBCatalog() : State(0), H_Connection(0xFFFFFFFF) 
{
	P_Lib = 0;
	if(PtLoad()) {
		State |= sValid;
		PvStart(0);
	}
}

PervasiveDBCatalog::~PervasiveDBCatalog()
{
	if(IsValid()) {
		Disconnect();
		PvStop(0);
		PtRelease();
	}
}

int PervasiveDBCatalog::IsValid() const
{
	return BIN(State & sValid);
}

static int SLAPI GetServerNameFromUncPath(const char * pUncPath, SString & rServerName)
{
	int    start = 0;
	rServerName = 0;
	for(const char * p = pUncPath; *p && start >= 0; p++) {
		if(p[0] == '\\' && p[1] == '\\') {
			p += 2;
			start = 1;
		}
		else if(oneof2(p[0], '\\', '/'))
			start = -1;
		if(start > 0)
			rServerName.CatChar(p[0]);
	}
	return rServerName.NotEmpty();
}

int PervasiveDBCatalog::ServernameFromFilename(const char * pFilename, SString & rServerName)
{
	SString unc_path;
	if(!pathToUNC(pFilename, unc_path))
		return (DBErrCode = SDBERR_SLIB, 0);
	if(!GetServerNameFromUncPath(unc_path, rServerName)) {
		SLibError = SLERR_INVPATH;
		DBErrCode = SDBERR_SLIB;
		return 0;
	}
	else
		return 1;
}

int PervasiveDBCatalog::Connect(const char * pServerName, char * pUserName, char * pPassword)
{
	int    ok = 0;
	if(IsValid()) {
		char * p_temp_srv_name = newStr(pServerName);
		int    status = PvConnectServer(p_temp_srv_name, pUserName, pPassword, &H_Connection);
		switch(status) {
			case P_E_FAIL:
			case P_E_NULL_PTR:
				DBErrCode = SDBERR_CONNECTFAULT;
				break;
			case P_OK:
				State |= sConnected;
				ok = 1;
		}
		delete p_temp_srv_name;
	}
	return ok;
}

int PervasiveDBCatalog::Disconnect()
{
	if(IsValid()) {
		if(State & sConnected) {
			PvDisconnect(H_Connection);
			State &= ~sConnected;
		}
		return 1;
	}
	else
		return 0;
}

int PervasiveDBCatalog::CreateDB(const char * pEntryName, const char * pDict, const char * pData)
{
	EXCEPTVAR(DBErrCode);
	int    ok = 1;
	int    r;
	SString dict_path, data_path;
	char * p_dict = 0, * p_data = 0, * p_entry_name = 0;
	THROW(IsValid());
	THROW(State & sConnected);
	THROW_V(pathToUNC(pDict, dict_path), SDBERR_SLIB);
	THROW_V(pathToUNC(pData, data_path), SDBERR_SLIB);
	p_entry_name = (char *)pEntryName;
	p_dict = (char *)dict_path.cptr(); // @badcast
	p_data = (char *)data_path.cptr(); // @badcast
	r = PvCreateDatabase(H_Connection, p_entry_name, p_dict, p_dict, 0);
	if(r ==  P_OK)
		r = PvCreateDSN(H_Connection, p_entry_name, p_entry_name, p_entry_name, NORMAL_MODE);
	if(r != P_OK)
		if(r == P_E_DUPLICATE_NAME) {
			CALLEXCEPTV(SDBERR_ALREADYEXIST);
		}
		else
			CALLEXCEPTV(SDBERR_FCRFAULT);
	CATCHZOK
	return ok;
}

int PervasiveDBCatalog::GetOpenFilesData(TSArray <DBFileInfo> * pInfoList)
{
	int    ok = 0;
	if(IsValid()) {
		int   files_opened = 0;
		ulong count = 0;
		int   status = PvGetOpenFilesData(H_Connection, &count);
		switch(status) {
			case P_E_FAIL:
			case P_E_NULL_PTR:
			case P_E_INVALID_HANDLE:
				DBErrCode = SDBERR_CONNECTFAULT;
				break;
			case P_OK:
				files_opened = 1;
				ok = 1;
		}
		if(ok > 0) {
			for(ulong i = 0; i < count; i++) {
				char   path[P_MAX_PATH_SIZE];
				ulong  buf_size = sizeof(path);
				memzero(path, sizeof(path));
				status = PvGetOpenFileName(H_Connection, i, &buf_size, path);
				if(status == P_OK) {
					DBFileInfo file_info;
					memzero(&file_info, sizeof(file_info));
					status = PvGetFileHandleInfo(H_Connection, path, (PVFILEHDLINFO*)&file_info);
					if(status == P_OK && pInfoList) {
						STRNSCPY(file_info.FileName, path);
						pInfoList->insert(&file_info);
					}
				}
			}
		}
		if(files_opened)
			PvFreeOpenFilesData(H_Connection);
	}
	return ok;
}
