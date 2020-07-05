// SEXTPRC.CPP
// Copyright (c) A.Sobolev 2016, 2017, 2018, 2019, 2020
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <process.h>

int SLAPI PKZip(const char * pSrcPath, const char * pDestPath, const char * pZipDir)
{
	int    ok = 1;
	SString cmd;
	SString src_path, dest_path, zip_path, file_name;
	file_name = "7z.exe";
	(zip_path = pZipDir).SetLastSlash().Cat(file_name);
	THROW(fileExists(pSrcPath));
	THROW(fileExists(pZipDir));
	src_path.CatQStr(pSrcPath);
	dest_path.CatQStr(pDestPath);
	cmd = zip_path;
	THROW(_spawnl(_P_WAIT, cmd.cptr(), file_name.cptr(), "a", dest_path.cptr(), src_path.cptr(), NULL) != -1);
	CATCHZOK
	return ok;
}

SLAPI ExecVDosParam::ExecVDosParam() : Flags(0)
{
}

int SLAPI ExecVDos(const ExecVDosParam & rParam)
{
	int   ok = 1;
	int   r = 0;
	SString vdos_path;
	SString startup_path = rParam.StartUpPath;
	SString cmd_line;
	SString temp_buf;
	SString exe_filename;
	SString autoexec_filename;
	SString config_filename;
	SString curdir;
	{
		wchar_t curdir_u[1024];
		::GetCurrentDirectoryW(sizeof(curdir_u), curdir_u);
		curdir.CopyUtf8FromUnicode(curdir_u, sstrlen(curdir_u), 0);
		curdir.Transf(CTRANSF_UTF8_TO_OUTER);
	}
	if(startup_path.NotEmptyS()) {
		startup_path.RmvLastSlash();
		if(::fileExists(startup_path)) {
			SPathStruc ps(startup_path.SetLastSlash());
			if(ps.Drv.Empty()) {
				SPathStruc ps_cd(curdir);
				ps.Drv = ps_cd.Drv;
				ps.Merge(startup_path);
			}
		}
	}
	{
		vdos_path = rParam.ExePath;
		THROW(vdos_path.NotEmptyS());
		vdos_path.RmvLastSlash();
		THROW(::fileExists(vdos_path));
		{
			SPathStruc ps(vdos_path.SetLastSlash());
			if(ps.Drv.Empty()) {
				SPathStruc ps_cd(curdir);
				ps.Drv = ps_cd.Drv;
				ps.Merge(vdos_path);
			}
		}
		vdos_path.SetLastSlash();
		(exe_filename = vdos_path).Cat("vdos.exe");
		(autoexec_filename = vdos_path).Cat("autoexec.txt");
		(config_filename = vdos_path).Cat("config.txt");
		{
			/* config.txt
			font=cour
			mouse=on
			*/
			SFile f_cfg(config_filename, SFile::mWrite);
			THROW(f_cfg.IsValid());
			temp_buf.Z().CatEq("font", "cour").CR();
			THROW(f_cfg.WriteLine(temp_buf));
			temp_buf.Z().CatEq("mouse", "on").CR();
			THROW(f_cfg.WriteLine(temp_buf));
		}
		{
			SFile f_ae(autoexec_filename, SFile::mWrite);
			THROW(f_ae.IsValid());
			if(startup_path.NotEmptyS()) {
				temp_buf.Z().Cat("use").Space().Cat("c:").Space().Cat(startup_path).CR();
				THROW(f_ae.WriteLine(temp_buf));
			}
			//
			for(uint sp = 0; rParam.Batch.get(&sp, temp_buf);) {
				temp_buf.Strip().CR();
				THROW(f_ae.WriteLine(temp_buf));
			}
			//
			if(rParam.Flags & rParam.fExitAfter) {
				(temp_buf = "exit").CR();
				THROW(f_ae.WriteLine(temp_buf));
			}
		}
	}
	{
		STARTUPINFO si;
		DWORD exit_code = 0;
		PROCESS_INFORMATION pi;
		MEMSZERO(si);
		si.cb = sizeof(si);
		MEMSZERO(pi);
		{
			STempBuffer cmd_line((exe_filename.Len()+16) * sizeof(TCHAR));
			strnzcpy(static_cast<TCHAR *>(cmd_line.vptr()), SUcSwitch(exe_filename), cmd_line.GetSize());
			r = ::CreateProcess(0, static_cast<TCHAR *>(cmd_line.vptr()), 0, 0, FALSE, 0, 0, SUcSwitch(vdos_path.cptr()), &si, &pi); // @unicodeproblem
		}
		if(!r) {
			SLS.SetOsError(0);
			CALLEXCEPT();
		}
		if(rParam.Flags & rParam.fWait)
			WaitForSingleObject(pi.hProcess, INFINITE); // Wait until child process exits.
	}
	CATCHZOK
	return ok;
}
