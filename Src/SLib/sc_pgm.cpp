// SC_PGM.CPP
// Copyright (c) A.Starodub 2009, 2010, 2015, 2016, 2018, 2019, 2020
// Part of StyloConduit project
// // Ёкспорт файлов дл€ обновлени€ программы
//
#include <slib-internal.h>
#pragma hdrstop
#include "StyloConduit.h"
#include <stddef.h>
#include <winver.h>

SCDBObjProgram::SCDBObjProgram(SpiiExchgContext * pCtx) : SCDBObject(pCtx)
{
}

SCDBObjProgram::~SCDBObjProgram()
{
}

int SCDBObjProgram::Init()
{
	int    ok = 0;
	int64  size = 0;
	ProgramFile.Close();
	DllFile.Close();
	if(P_Ctx->P_Pte) {
		SString path;
		P_Ctx->P_Pte->GetInstallPath(path);
		path.SetLastSlash().Cat(P_PalmProgramFileName);
		ProgramFile.Open(path, SFile::mRead|SFile::mBinary);
		P_Ctx->P_Pte->GetInstallPath(path);
		path.SetLastSlash().Cat(P_PalmDllFileName);
		DllFile.Open(path, SFile::mRead|SFile::mBinary);
		ProgramFile.CalcSize(&size);
	}
	return (ProgramFile.IsValid() && size > 0);
}

const SCDBTblEntry * SCDBObjProgram::GetDefinition(uint * pEntryCount) const
{
	static const SCDBTblEntry def[] = {
		{P_PalmProgramFileName, 0, -1, 0, 0},
		{P_PalmDllFileName,     0, -1, 0, 0}
	};
	ASSIGN_PTR(pEntryCount, 2);
	return def;
}

int SCDBObjProgram::GetHostProgramVer(uint32 * pVer)
{
	int    ok = -1;
	uint32 ver = 0;
	uint   info_size = 0;
	SString path;
	DWORD set_to_zero = 0;
	P_Ctx->P_Pte->GetInstallPath(path);
	path.SetLastSlash().Cat(P_PalmProgramFileName);
	info_size = GetFileVersionInfoSize(SUcSwitch(path), &set_to_zero); // @unicodeproblem
	if(info_size) {
		char * p_buf = new char[info_size];
		if(GetFileVersionInfo(SUcSwitch(path), 0, info_size, p_buf)) { // @unicodeproblem
			uint   value_size = 0;
			char * p_ver_buf = 0;
			if(VerQueryValue(p_buf, _T("\\"), (LPVOID *)&p_ver_buf, &value_size)) {
				const VS_FIXEDFILEINFO * p_file_info = reinterpret_cast<const VS_FIXEDFILEINFO *>(p_ver_buf);
				ver = HIWORD(p_file_info->dwProductVersionMS) << 16;
				ver = ver | (LOWORD(p_file_info->dwProductVersionMS) << 8);
				ver = ver | HIWORD(p_file_info->dwProductVersionLS);
				ok = 1;
			}
		}
		ZDELETEARRAY(p_buf);
	}
	ASSIGN_PTR(pVer, ver);
	return ok;
}

int SCDBObjProgram::ExportFile(SFile * pFile, PROGRESSFN pFn)
{
	int    ok = -1;
	char * p_out_buf = 0;
	if(pFile) {
		int64  size = 0;
		size_t buf_len = 1024, read_bytes = 0;
		uint32 numrecs = 0, recno = 0;
		ProgramFile.CalcSize(&size);
		THROW(size > 0);
		{
			const SString & r_path = pFile->GetName();
			SString log_msg, fname;
			SFsPath sp(r_path);
			sp.Merge(0, SFsPath::fDrv|SFsPath::fDir, fname);
			numrecs = (long)(size / buf_len); // @32-64
			p_out_buf = new char[buf_len];

			SyncTable stbl(P_Ctx->PalmCfg.CompressData(), 0, P_Ctx);
			THROW(stbl.DeleteTable(fname));
			THROW(stbl.Open(fname, SyncTable::oCreate));
			while(pFile->Read(p_out_buf, buf_len, &read_bytes) > 0 && read_bytes > 0) {
				THROW(stbl.AddRec(0, p_out_buf, read_bytes));
				recno++;
				THROW(stbl.Reopen(-1, recno));
				log_msg.Printf("Ёкспорт Program (%s)", fname.cptr());
				WaitPercent(pFn, recno, numrecs, log_msg.cptr());
			}
			stbl.Close();
			P_Ctx->TransmitComprFile = 1;
			{
				log_msg.Printf("SPII OK: Program (%s) exported", fname.cptr());
				SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
			}
		}
	}
	CATCHZOK
	ZDELETE(p_out_buf);
	return ok;
}

int SCDBObjProgram::Export(PROGRESSFN pFn, CSyncProperties * pProps)
{
	int    ok = 1;
	long   numrecs = 0, recno = 0;
	char   log_msg[128];
	uint32 ver = 0;
	char * p_out_buf = 0;
	if(P_Ctx->P_Pte && P_Ctx->PalmCfg.CompressData() && P_Ctx->PalmCfg.Ver >= 400 && P_Ctx->P_Pte->GetProgramVer(&ver) > 0) {
		uint32 dev_ver = 0;
		THROW(GetHostProgramVer(&dev_ver));
		if(P_Ctx->PalmCfg.Flags & CFGF_DENYUPDATEPROGRAM) {
			SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: PROGRAM declined by palm");
			ok = -1;
		}
		else if(!ForceExportObsoleteData && ver >= dev_ver) {
			SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: Program base on palm is recently that host");
			ok = -1;
		}
		else {
			THROW(ExportFile(&ProgramFile, pFn));
			if(DllFile.IsValid())
				THROW(ExportFile(&DllFile, pFn));
		}
	}
	else
		ok = -1;
	CATCH
		ok = 0;
		{
			sprintf(log_msg, "SPII ERR: Program export failed");
			SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
		}
	ENDCATCH
	delete p_out_buf;
	return ok;
}
