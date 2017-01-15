// PPBUILD.CPP
// Copyright (c) A.Sobolev 2012, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop

class PrcssrBuild {
public:
	struct BuildVer {
		BuildVer()
		{
			THISZERO();
		}
		int    Major; // MajorVer
		int    Minor; // MinorVer
		int    Revision; // Revision
		int    Asm; // AssemblyVer
	};
	struct Param {
		enum {
			fBuildClient     = 0x0001,
			fBuildServer     = 0x0002,
			fBuildMtdll      = 0x0004,
			fBuildDrv        = 0x0008,
			fBuildSoap       = 0x0010,
			fBuildDistrib    = 0x0020,
			fCopyToUhtt      = 0x0040,
			fOpenSource      = 0x0080  // OpenSource-вариант сборки
		};
		Param()
		{
			Flags = 0;
			PrefMsvsVerMajor = 0;
		}
		SString & GetVerLabel(SString & rBuf) const
		{
			(rBuf = 0).Cat(Ver.Major).Dot().Cat(Ver.Minor).Dot().CatLongZ(Ver.Revision, 2).
				CatChar('(').Cat(Ver.Asm).CatChar(')');
			return rBuf;
		}
		BuildVer Ver;           // Собираемая версия //
		long   Flags;
		int    PrefMsvsVerMajor;

		SString VerSuffix;      // Опциональный суффикс версии дистрибутива (например, PRE)
		SString RootPath;       // Корневой каталог проекта
		SString SrcPath;        // Каталог исходных кодов
		SString SlnPath;        // Каталог, содержащий файлы проектов
		SString TargetRootPath; // Корневой каталог, в котором должна собираться версия (C:\PPY)
		SString NsisPath;       // Путь к исполняемому файлу NSIS (сборщик дистрибутива)
		SString DistribPath;    // Корневой каталог, хранящий дистрибутивы
	};

	static int SLAPI FindMsvs(int prefMajor, StrAssocArray & rList, SString * pPrefPath);
	int	   SLAPI InitParam(Param *);
	int	   SLAPI EditParam(Param *);
	int	   SLAPI Init(const Param *);
	int	   SLAPI Run();
	int    SLAPI Build();
	int    SLAPI BuildLocalDl600(const char * pPath);
private:
	static int CopyProgressProc(const SCopyFileData * scfd); // SCopyFileProgressProc
	int	   SLAPI UploadFileToUhtt(const char * pFileName, const char * pKey, const char * pVerLabel, const char * pMemo);

	Param  P;
};

int	SLAPI PrcssrBuild::InitParam(Param * pParam)
{
	int    ok = 1;
	SString temp_buf, full_path_buf;
	SString file_name_buf;
	PPIniFile ini_file;
	//
	ini_file.Get(PPINISECT_PATH, PPINIPARAM_BUILDROOT, temp_buf = 0);
	THROW_PP(temp_buf.NotEmpty(), PPERR_BUILD_UNDEFBUILDROOT);
	THROW_SL(fileExists(temp_buf));
	pParam->RootPath = temp_buf;
	//
	ini_file.Get(PPINISECT_PATH, PPINIPARAM_BUILDSRC, temp_buf = 0);
	THROW_PP(temp_buf.NotEmpty(), PPERR_BUILD_UNDEFBUILDSRC);
	(full_path_buf = pParam->RootPath).SetLastSlash().Cat(temp_buf);
	THROW_SL(fileExists(full_path_buf));
	pParam->SrcPath = full_path_buf;
	//
	ini_file.Get(PPINISECT_PATH, PPINIPARAM_BUILDSOLUTION, temp_buf = 0);
	THROW_PP(temp_buf.NotEmpty(), PPERR_BUILD_UNDEFBUILDSLN);
	(full_path_buf = pParam->RootPath).SetLastSlash().Cat(temp_buf);
	THROW_SL(fileExists(full_path_buf));
	pParam->SlnPath = full_path_buf;
	//
	ini_file.Get(PPINISECT_PATH, PPINIPARAM_BUILDTARGET, temp_buf = 0);
	THROW_PP(temp_buf.NotEmpty(), PPERR_BUILD_UNDEFBUILDTARGET);
	THROW_SL(fileExists(temp_buf));
	pParam->TargetRootPath = temp_buf;
	//
	ini_file.Get(PPINISECT_PATH, PPINIPARAM_BUILDNSIS, temp_buf = 0);
	if(temp_buf.Empty()) {
		(temp_buf = pParam->RootPath).SetLastSlash().Cat("tools").SetLastSlash().Cat("nsis").SetLastSlash().Cat("makensis.exe");
	}
	THROW_SL(fileExists(temp_buf));
	pParam->NsisPath = temp_buf;
	//
	ini_file.Get(PPINISECT_PATH, PPINIPARAM_PREFMSVSVER, temp_buf = 0);
	if(temp_buf.NotEmptyS()) {
		pParam->PrefMsvsVerMajor = temp_buf.ToLong();
	}
	SETIFZ(pParam->PrefMsvsVerMajor, 7);
	//
	{
		//
		// Извлекаем из файла SRC\RSRC\VERSION\genver.dat номер создаваемой версии
		//
		(temp_buf = pParam->SrcPath).SetLastSlash().Cat("RSRC").SetLastSlash().Cat("Version").SetLastSlash();
		//(temp_buf = pParam->SrcPath).SetLastSlash().Cat("RSRC").SetLastSlash().Cat("Version").SetLastSlash().Cat("genver.dat");
		PPGetFileName(PPFILNAM_GENVER_DAT, file_name_buf);
		(full_path_buf = temp_buf).Cat(file_name_buf);
		if(!fileExists(full_path_buf)) {
			PPGetFileName(PPFILNAM_GENVEROPEN_DAT, file_name_buf);
			(full_path_buf = temp_buf).Cat(file_name_buf);
		}
		temp_buf = full_path_buf;
		THROW(fileExists(temp_buf));
		{
			// @v9.4.9 {
			SIniFile f_genver_file(temp_buf);
			PapyrusPrivateBlock ppb;
			THROW(ppb.ReadFromIni(f_genver_file));
			{
				ppb.Ver.Get(&pParam->Ver.Major, &pParam->Ver.Minor, &pParam->Ver.Revision);
				pParam->Ver.Asm = ppb.AssemblyN;
				if(ppb.Flags & ppb.fOpenSource)
				SETFLAG(pParam->Flags, Param::fOpenSource, (ppb.Flags & ppb.fOpenSource));
			}
			// } @v9.4.9
			/* @v9.4.9
			SString line_buf, key_buf, val_buf;
			int    mj_difined = 0;
			int    mn_difined = 0;
			int    r_difined = 0;
			int    a_difined = 0;
			SFile f(temp_buf, SFile::mRead);
			THROW_SL(f.IsValid());
			while(f.ReadLine(line_buf)) {
				line_buf.Chomp().Strip();
				if(line_buf.Divide('=', key_buf, val_buf) > 0) {
					key_buf.Strip();
					if(key_buf.CmpNC("MajorVer") == 0) {
						pParam->Ver.Major = val_buf.Strip().ToLong();
						mj_difined = 1;
					}
					else if(key_buf.CmpNC("MinorVer") == 0) {
						pParam->Ver.Minor = val_buf.Strip().ToLong();
						mn_difined = 1;
					}
					else if(key_buf.CmpNC("Revision") == 0) {
						pParam->Ver.Revision = val_buf.Strip().ToLong();
						r_difined = 1;
					}
					else if(key_buf.CmpNC("AssemblyVer") == 0) {
						pParam->Ver.Asm = val_buf.Strip().ToLong();
						a_difined = 1;
					}
					else if(key_buf.CmpNC("OpenSource") == 0) {
						if(val_buf.Strip().ToLong() > 0) {
							pParam->Flags |= Param::fOpenSource;
						}
					}
				}
			}
			*/
		}
	}
	//
	ini_file.Get(PPINISECT_PATH, PPINIPARAM_BUILDDISTRIB, temp_buf = 0);
	THROW_PP(temp_buf.NotEmpty(), PPERR_BUILD_UNDEFBUILDDISTRIB);
	THROW_SL(fileExists(temp_buf));
	pParam->DistribPath = temp_buf;
	//
	pParam->Flags = (Param::fBuildClient|Param::fBuildServer|Param::fBuildMtdll|Param::fBuildDrv|Param::fBuildSoap|Param::fBuildDistrib/*|Param::fCopyToUhtt*/);
	CATCHZOK
	return ok;
}

int	SLAPI PrcssrBuild::EditParam(Param * pParam)
{
	class SelfBuildDialog : public TDialog {
	public:
		SelfBuildDialog() : TDialog(DLG_SELFBUILD)
		{
			CloseTimeout = 60;
			PrevTimeoutRest = -1;
			StartClock = 0;
		}
		int setDTS(const PrcssrBuild::Param * pData)
		{
			int    ok = 1;
			SString temp_buf;
			Data = *pData;
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 0, PrcssrBuild::Param::fBuildClient);
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 1, PrcssrBuild::Param::fBuildServer);
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 2, PrcssrBuild::Param::fBuildMtdll);
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 3, PrcssrBuild::Param::fBuildDrv);
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 4, PrcssrBuild::Param::fBuildSoap);
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 5, PrcssrBuild::Param::fBuildDistrib);
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 6, PrcssrBuild::Param::fCopyToUhtt);
			SetClusterData(CTL_SELFBUILD_FLAGS, Data.Flags);

			setCtrlString(CTL_SELFBUILD_VERSION, Data.GetVerLabel(temp_buf));
			setCtrlString(CTL_SELFBUILD_VERSFX, Data.VerSuffix);
			{
				StrAssocArray msvs_ver_list;
				PrcssrBuild::FindMsvs(Data.PrefMsvsVerMajor, msvs_ver_list, &(temp_buf = 0));
				setCtrlString(CTL_SELFBUILD_CMPLRPATH, temp_buf);
			}
			setCtrlString(CTL_SELFBUILD_IMPATH, Data.NsisPath);
			if(CloseTimeout >= 0)
				setStaticText(CTL_SELFBUILD_TIMEOUT, (temp_buf = 0).Cat(CloseTimeout));
			StartClock = clock();
			return ok;
		}
		int getDTS(PrcssrBuild::Param * pData)
		{
			GetClusterData(CTL_SELFBUILD_FLAGS, &Data.Flags);
			getCtrlString(CTL_SELFBUILD_VERSFX, Data.VerSuffix);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			SString temp_buf;
			TDialog::handleEvent(event);
			if(TVCMD == cmInputUpdated) {
				if(event.isCtlEvent(CTL_SELFBUILD_VERSFX)) {
					getCtrlString(CTL_SELFBUILD_VERSFX, Data.VerSuffix);
					setCtrlString(CTL_SELFBUILD_VERSION, Data.GetVerLabel(temp_buf));
					clearEvent(event);
				}
			}
			else if(TVBROADCAST && TVCMD == cmIdle) {
				if(CloseTimeout >= 0) {
					clock_t diff = clock() - StartClock;
					long   timeout_rest = CloseTimeout - diff / CLOCKS_PER_SEC;
					if(PrevTimeoutRest != timeout_rest)
						setStaticText(CTL_SELFBUILD_TIMEOUT, (temp_buf = 0).Cat(timeout_rest));
					PrevTimeoutRest = timeout_rest;
					if(diff >= (CloseTimeout * CLOCKS_PER_SEC)) {
						if(IsInState(sfModal)) {
							clearEvent(event);
							endModal(cmOK);
							return; // После endModal не следует обращаться к this
						}
					}
				}
			}
			else
				return;
		}
		PrcssrBuild::Param Data;
		long   CloseTimeout;
		long   PrevTimeoutRest;
		clock_t StartClock;
	};
	DIALOG_PROC_BODY(SelfBuildDialog, pParam);
}

int	SLAPI PrcssrBuild::Init(const Param * pParam)
{
	int    ok = 1;
	P = *pParam;
	return ok;
}

#if 0 // {

SET BUILDLOG=%PPYSRC%\BUILD\LOG\vc70build
mkdir %BUILDLOG%
set BUILD_STATUS=build.ok
set FULLFLAG=/rebuild
del %PPYSRC%\build\log\build.ok
echo status > %PPYSRC%\build\log\build.failed
del %BUILDLOG%\*.log
SET MSVS_PATH="D:\msvs70\common7\ide\devenv.exe"
%MSVS_PATH% papyrus.sln %FULLFLAG% "Release" /out %BUILDLOG%\client.log
if exist ..\..\ppy\bin\ppw.exe (echo ppw=OK) else (set BUILD_STATUS=build.failed)
%MSVS_PATH% papyrus.sln %FULLFLAG% "MtDllRelease" /out %BUILDLOG%\mtdll.log
if exist ..\..\ppy\bin\ppwmt.dll (echo ppwmt=OK) else (set BUILD_STATUS=build.failed)
%MSVS_PATH% papyrus.sln %FULLFLAG% "ServerRelease" /out %BUILDLOG%\jobsrv.log
if exist ..\..\ppy\bin\ppws.exe (echo ppws=OK) else (set BUILD_STATUS=build.failed)

%MSVS_PATH% EquipSolution.sln %FULLFLAG% "Release" /out %BUILDLOG%\EquipSolution.log
if exist ..\..\ppy\bin\ppdrv-pirit.dll (echo ppdrv-pirit=OK) else (set BUILD_STATUS=build.failed)

%MSVS_PATH% PPSoapModules.sln %FULLFLAG% "Release" /out %BUILDLOG%\PPSoapModules.log
if exist ..\..\ppy\bin\PPSoapUhtt.dll     (echo PPSoapUhtt=OK)     else (set BUILD_STATUS=build.failed)


del %PPYSRC%\build\log\build.failed
echo status > %PPYSRC%\build\log\%BUILD_STATUS%

#endif // } 0

// static
int SLAPI PrcssrBuild::FindMsvs(int prefMsvsVerMajor, StrAssocArray & rList, SString * pPrefPath)
{
	int    ok = -1;
	rList.Clear();
	const char * p_sub_msvs = "Software\\Microsoft\\VisualStudio";
	WinRegKey reg_key(HKEY_LOCAL_MACHINE, p_sub_msvs, 1); // @v9.2.0 readonly 0-->1
	SString temp_buf, major, minor, subkey_buf, path_buf;
	StrAssocArray msvs_ver_list;
	for(uint kidx = 0; reg_key.EnumKeys(&kidx, temp_buf);) {
		if(temp_buf.Divide('.', major, minor) > 0) {
			long msvs_ver = (major.ToLong() << 16) | (minor.ToLong() & 0xffff);
			(subkey_buf = p_sub_msvs).SetLastSlash().Cat(temp_buf); //.Cat("InstallDir");
			WinRegKey reg_key_ver(HKEY_LOCAL_MACHINE, subkey_buf, 1); // @v9.2.0 readonly 0-->1
			if(reg_key_ver.GetString("InstallDir", path_buf) > 0 && path_buf.NotEmpty() && fileExists(path_buf)) {
				rList.Add(msvs_ver, path_buf);
				ok = 1;
			}
		}
	}
	if(ok > 0 && pPrefPath) {
		for(uint i = 0; pPrefPath->Empty() && i < rList.getCount(); i++) {
			StrAssocArray::Item item = rList.at(i);
			int    msvs_ver_major = (item.Id >> 16);
			int    msvs_ver_minor = (item.Id & 0xffff);
			if(prefMsvsVerMajor) {
				if(msvs_ver_major == prefMsvsVerMajor) {
					(*pPrefPath = item.Txt).SetLastSlash().Cat("devenv.exe");
					ok = 2;
				}
			}
			else if(msvs_ver_major == 7 && msvs_ver_minor == 1) {
				(*pPrefPath = item.Txt).SetLastSlash().Cat("devenv.exe");
				ok = 2;
			}
		}
	}
	return ok;
}

//static
int PrcssrBuild::CopyProgressProc(const SCopyFileData * scfd)
{
	SString msg_buf;
	(msg_buf = scfd->SrcFileName).Space().Cat("-->").Space().Cat(scfd->DestFileName);
	long   pct = (long)(100L * scfd->TransferredBytes / scfd->TotalFileSize);
	PPWaitPercent(pct, msg_buf);
	return SPRGRS_CONTINUE;
}

int	SLAPI PrcssrBuild::UploadFileToUhtt(const char * pFileName, const char * pKey, const char * pVerLabel, const char * pMemo)
{
	int    ok = 1;
	PPUhttClient uhtt_cli;
	THROW(uhtt_cli.Auth());
	THROW(uhtt_cli.FileVersionAdd(pFileName, pKey, pVerLabel, pMemo, PrcssrBuild::CopyProgressProc, this));
	CATCHZOK
	return ok;
}

int	SLAPI PrcssrBuild::Run()
{
	int    ok = 1;
	uint   i;
	PPLogger logger;
	SString temp_buf, build_log_path, fmt_buf, msg_buf;

	// @debug {
	//
	/*
	(temp_buf = P.RootPath).SetLastSlash().Cat("ManWork").SetLastSlash().Cat("LaTex").SetLastSlash().Cat("ppmanual.pdf");
	if(fileExists(temp_buf)) {
		PPWait(1);
		UploadFileToUhtt(temp_buf, "papyrus-manual", "test-ver-3", "");
		PPWait(0);
	}
	*/
	// } @debug
	{
		//
		// Сборка исполняемых файлов и ресурсов
		//
		struct SolutionEntry {
			const char * P_Name;
			const char * P_Sln;
			const char * P_Config;
			const char * P_Result;
			long   Flag;
		};
		SolutionEntry sln_list[] = {
			// P_Name          P_Sln                P_Config         P_Result           Flag
			{ "client",        "papyrus.sln",       "Release",       "ppw.exe",         Param::fBuildClient },
			{ "mtdll",         "papyrus.sln",       "MtDllRelease",  "ppwmt.dll",       Param::fBuildMtdll },
			{ "jobsrv",        "papyrus.sln",       "ServerRelease", "ppws.exe",        Param::fBuildServer },
			{ "equipsolution", "EquipSolution.sln", "Release",       "ppdrv-pirit.dll", Param::fBuildDrv },
			// @v8.3.2 { "ppsoapmodules", "PPSoapModules.sln", "Release",       "PPSoapUhtt.dll",  Param::fBuildSoap }
		};
		StrAssocArray msvs_ver_list;
		SString msvs_path;
		const char * p_prc_cur_dir = P.SlnPath.NotEmpty() ? (const char *)P.SlnPath : (const char *)0;
		logger.Log((msg_buf = "Current dir for child processes").CatDiv(':', 2).Cat(p_prc_cur_dir));
		THROW(FindMsvs(P.PrefMsvsVerMajor, msvs_ver_list, &msvs_path));
		PPLoadText(PPTXT_BUILD_COMPILERNAME_VS71, temp_buf);
		THROW_PP_S(msvs_path.NotEmpty() && fileExists(msvs_path), PPERR_BUILD_COMPILERNFOUND, temp_buf);
		for(uint j = 0; j < SIZEOFARRAY(sln_list); j++) {
			SolutionEntry & r_sln_entry = sln_list[j];
			if(P.Flags & r_sln_entry.Flag) {
				PPGetPath(PPPATH_LOG, build_log_path);
				build_log_path.SetLastSlash().Cat("build").CatChar('-').Cat(r_sln_entry.P_Name).Dot().Cat("log");
				//D:\msvs70\common7\ide\devenv.exe papyrus.sln /rebuild "Release" /out %BUILDLOG%\client.log
				{
					STARTUPINFO si;
					DWORD exit_code = 0;
					PROCESS_INFORMATION pi;
					MEMSZERO(si);
					si.cb = sizeof(si);
					MEMSZERO(pi);
					(temp_buf = 0).CatQStr(msvs_path).Space().Cat(r_sln_entry.P_Sln).Space().Cat("/rebuild").Space().
						Cat(r_sln_entry.P_Config).Space().Cat("/out").Space().Cat(build_log_path);
					STempBuffer cmd_line(temp_buf.Len()*2);
					strnzcpy(cmd_line, temp_buf, cmd_line.GetSize());

					PPLoadText(PPTXT_BUILD_SOLUTION, fmt_buf);
					logger.Log(msg_buf.Printf(fmt_buf, (const char *)temp_buf));
					int    r = ::CreateProcess(0, cmd_line, 0, 0, FALSE, 0, 0, p_prc_cur_dir, &si, &pi); // @unicodeproblem
					if(!r) {
						SLS.SetOsError(0);
						CALLEXCEPT_PP(PPERR_SLIB);
					}
					WaitForSingleObject(pi.hProcess, INFINITE); // Wait until child process exits.
					{
						GetExitCodeProcess(pi.hProcess, &exit_code);
						if(exit_code == 0) {
							(temp_buf = P.TargetRootPath).SetLastSlash().Cat("BIN").SetLastSlash().Cat(r_sln_entry.P_Result);
							if(fileExists(temp_buf)) {
								msg_buf.Printf(PPLoadTextS(PPTXT_BUILD_SLN_SUCCESS, fmt_buf), r_sln_entry.P_Name);
								logger.Log(msg_buf);
							}
							else {
								msg_buf.Printf(PPLoadTextS(PPTXT_BUILD_SLN_TARGETNFOUND, fmt_buf), r_sln_entry.P_Name, (const char *)temp_buf);
								logger.Log(msg_buf);
							}
						}
						else {
							msg_buf.Printf(PPLoadTextS(PPTXT_BUILD_SLN_FAIL, fmt_buf), r_sln_entry.P_Name, (const char *)build_log_path);
							logger.Log(msg_buf);
						}
					}
					// Close process and thread handles.
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}
			}
		}
	}
	if(P.Flags & Param::fBuildDistrib) {
		//
		// Сборка дистрибутива
		//
		struct NsisEntry {
			const char * P_Name;
			const char * P_Config;
			const char * P_NsisFile;
			const char * P_UhttSymb;

			char   FileName[512];
		};
		NsisEntry nsis_list[] = {
			{ "Server",      "INSTALL_SERVER", "papyrus.nsi", "papyrus-setup-server",    "" }, // #0
			{ "Client",      "INSTALL_CLIENT", "papyrus.nsi", "papyrus-setup-client",    "" }, // #1
			{ "Update",      "INSTALL_UPDATE", "papyrus.nsi", "papyrus-setup-update",    "" }, // #2
			{ "JobSrvr",     0,                "ppws.nsi",    "papyrus-setup-jobserver", "" }, // #3

			{ "Manual",      0,                0,             "papyrus-manual",          "" }, // #4
			{ "Features",    0,                0,             "papyrus-features",        "" }, // #5
			{ "VersionInfo", 0,                0,             "papyrus-version-info",    "" }  // #6
		};
		//
		SString ver_label, sub_path, distrib_path, distrib_src_path, build_path, target_file_name, dest_path;
		P.GetVerLabel(ver_label);
		char _c = 0;
		do {
			(sub_path = 0).CatChar('v').Cat(P.Ver.Major).Cat(P.Ver.Minor).CatLongZ(P.Ver.Revision, 2).CatChar('a' + (P.Ver.Asm + _c) % 26);
			if(P.VerSuffix.NotEmpty())
				sub_path.CatChar('-').Cat(P.VerSuffix);
			(distrib_path = P.DistribPath).SetLastSlash().Cat(sub_path);
			_c++;
		} while(fileExists(distrib_path));
		THROW(::createDir(distrib_path));
		(distrib_src_path = P.DistribPath).SetLastSlash().Cat("SRC").SetLastSlash().Cat(sub_path);
		THROW(::createDir(distrib_src_path));
		//
		(temp_buf = P.RootPath).SetLastSlash().Cat("ManWork").SetLastSlash().Cat("LaTex").SetLastSlash().Cat("ppmanual.pdf");
		temp_buf.CopyTo(nsis_list[4].FileName, sizeof(nsis_list[4].FileName));
		//
		(temp_buf = P.RootPath).SetLastSlash().Cat("ManWork").SetLastSlash().Cat("LaTex").SetLastSlash().Cat("features.pdf");
		temp_buf.CopyTo(nsis_list[5].FileName, sizeof(nsis_list[5].FileName));
		//
		(temp_buf = P.SrcPath).SetLastSlash().Cat("doc").SetLastSlash().Cat("version.txt");
		temp_buf.CopyTo(nsis_list[6].FileName, sizeof(nsis_list[6].FileName));
		//
		(build_path = P.SrcPath).SetLastSlash().Cat("BUILD");
		for(i = 0; i < SIZEOFARRAY(nsis_list); i++) {
			NsisEntry & r_nsis_entry = nsis_list[i];
			if(r_nsis_entry.P_NsisFile) {
				STARTUPINFO si;
				DWORD exit_code = 0;
				PROCESS_INFORMATION pi;
				MEMSZERO(si);
				si.cb = sizeof(si);
				MEMSZERO(pi);

				(target_file_name = "Ppy").Cat(r_nsis_entry.P_Name).CatChar('_').Cat(ver_label).Dot().Cat("exe");
				PPGetPath(PPPATH_LOG, build_log_path);
				build_log_path.SetLastSlash().Cat("build").CatChar('-').Cat("nsis").CatChar('-').Cat(r_nsis_entry.P_Name).Dot().Cat("log");
				(temp_buf = 0).CatQStr(P.NsisPath).Space().CatEq("/DPRODUCT_VERSION", ver_label).Space().
					CatEq("/DSRC_ROOT", P.RootPath).Space().Cat("/NOCD").Space().Cat("/V2").Space().Cat("/P1").Space();
				if(r_nsis_entry.P_Config) {
					temp_buf.Cat("/D").Cat(r_nsis_entry.P_Config).Space();
				}
				// @v9.4.9 {
				if(P.Flags & Param::fOpenSource) {
					temp_buf.Cat("/D").Cat("OPENSOURCE").Space();
				}
				// } @v9.4.9
				temp_buf.Cat("/O").Cat(build_log_path).Space().Cat(r_nsis_entry.P_NsisFile);

				STempBuffer cmd_line(temp_buf.Len()*2);
				strnzcpy(cmd_line, temp_buf, cmd_line.GetSize());

				PPLoadText(PPTXT_BUILD_DISTRIB, fmt_buf);
				logger.Log(msg_buf.Printf(fmt_buf, (const char *)temp_buf));

				int    r = ::CreateProcess(0, cmd_line, 0, 0, FALSE, 0, 0, build_path, &si, &pi); // @unicodeproblem
				if(!r) {
					SLS.SetOsError(0);
					CALLEXCEPT_PP(PPERR_SLIB);
				}
				WaitForSingleObject(pi.hProcess, INFINITE); // Wait until child process exits.
				{
					GetExitCodeProcess(pi.hProcess, &exit_code);
					if(exit_code == 0) {
						(temp_buf = build_path).SetLastSlash().Cat(target_file_name);
						if(fileExists(temp_buf)) {
							PPLoadText(PPTXT_BUILD_DISTRIB_SUCCESS, fmt_buf);
							msg_buf.Printf(fmt_buf, (const char *)target_file_name);
							logger.Log(msg_buf);
							//
							(dest_path = distrib_path).SetLastSlash().Cat(target_file_name);
							THROW_SL(SCopyFile(temp_buf, dest_path, 0, FILE_SHARE_READ, 0));
							SFile::Remove(temp_buf);
							dest_path.CopyTo(r_nsis_entry.FileName, sizeof(r_nsis_entry.FileName));
						}
						else {
							PPLoadText(PPTXT_BUILD_DISTRIB_TARGETNFOUND, fmt_buf);
							msg_buf.Printf(fmt_buf, (const char *)target_file_name, (const char *)temp_buf);
							logger.Log(msg_buf);
						}
					}
					else {
						PPLoadText(PPTXT_BUILD_DISTRIB_FAIL, fmt_buf);
						msg_buf.Printf(fmt_buf, (const char *)target_file_name, (const char *)build_log_path);
						logger.Log(msg_buf);
					}
				}
				// Close process and thread handles.
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
		}
		if(P.Flags & Param::fCopyToUhtt) {
			//
			// Копирование файлов дистрибутива в хранилище Universe-HTT
			//
			PPWait(1);
			for(i = 0; i < SIZEOFARRAY(nsis_list); i++) {
				NsisEntry & r_nsis_entry = nsis_list[i];
				if(r_nsis_entry.P_UhttSymb && r_nsis_entry.FileName[0] && fileExists(r_nsis_entry.FileName)) {
					if(UploadFileToUhtt(r_nsis_entry.FileName, r_nsis_entry.P_UhttSymb, ver_label, 0)) {
						PPLoadText(PPTXT_BUILD_UHTTCOPY_SUCCESS, fmt_buf);
						msg_buf.Printf(fmt_buf, r_nsis_entry.FileName);
						logger.Log(msg_buf);
					}
					else {
						PPLoadText(PPTXT_BUILD_UHTTCOPY_FAIL, fmt_buf);
						PPGetMessage(mfError, PPErrCode, 0, 1, temp_buf);
						msg_buf.Printf(fmt_buf, r_nsis_entry.FileName, (const char *)temp_buf);
						logger.Log(msg_buf);
					}
				}
			}
			PPWait(0);
		}
	}
	CATCH
		logger.LogLastError();
		ok = 0;
	ENDCATCH
	PPWait(0);
	logger.Save(PPFILNAM_SELFBUILD_LOG, 0);
	return ok;
}

int SLAPI PrcssrBuild::BuildLocalDl600(const char * pPath)
{
	int    ok = 1;
	SString temp_buf;
	PPLogger logger;
	{
		int    cpr = 0; // CreateProcess result
		SString src_file_name;
		SString cmd_line;
		SString cur_dir;
		SString fmt_buf, msg_buf;
		SPathStruc ps;
		STARTUPINFO si;
		DWORD exit_code = 0;
		PROCESS_INFORMATION pi;
		MEMSZERO(si);
		si.cb = sizeof(si);
		MEMSZERO(pi);

		PPGetFilePath(PPPATH_BIN, "dl600c.exe", temp_buf);
		THROW_SL(fileExists(temp_buf));
		(cmd_line = 0).CatQStr(temp_buf);
		if(isempty(pPath)) {
			PPGetFilePath(PPPATH_DD, "local.dl6", src_file_name);
		}
		else {
			ps.Split(pPath);
			if(ps.Drv.Empty() && ps.Dir.Empty()) {
				PPGetFilePath(PPPATH_DD, pPath, src_file_name);
			}
		}
		THROW(fileExists(src_file_name));
		{
			ps.Split(src_file_name);
			ps.Nam = 0;
			ps.Ext = 0;
			ps.Merge(cur_dir);
		}
		cmd_line.Space().Cat("/ob").Space().CatQStr(src_file_name);
		PPLoadText(PPTXT_BUILD_LOCALDL600, fmt_buf);
		logger.Log(msg_buf.Printf(fmt_buf, (const char *)cmd_line));
		{
			STempBuffer cmd_line_buf(cmd_line.Len()*2);
			strnzcpy(cmd_line_buf, cmd_line, cmd_line_buf.GetSize());
			cpr = ::CreateProcess(0, cmd_line_buf, 0, 0, FALSE, 0, 0, cur_dir, &si, &pi); // @unicodeproblem
			if(!cpr) {
				SLS.SetOsError(0);
				CALLEXCEPT_PP(PPERR_SLIB);
			}
			WaitForSingleObject(pi.hProcess, INFINITE); // Wait until child process exits.
		}
		{
			GetExitCodeProcess(pi.hProcess, &exit_code);
			if(exit_code == 0) {
				SString result_file_name;
                SString dest_file_name;
				result_file_name = src_file_name;
				SPathStruc::ReplaceExt(result_file_name, "bin", 1);
				THROW_SL(fileExists(result_file_name));
                PPGetFilePath(PPPATH_BIN, "ppexp.bin", dest_file_name);
                if(fileExists(dest_file_name)) {
					SString backup_file_name = dest_file_name;
					SPathStruc::ReplaceExt(backup_file_name, "bin-backup", 1);
					if(fileExists(backup_file_name))
						SFile::Remove(backup_file_name);
					THROW_SL(SFile::Rename(dest_file_name, backup_file_name));
                }
                THROW_SL(SCopyFile(result_file_name, dest_file_name, 0, FILE_SHARE_READ, 0));
			}
			else {
				msg_buf.Printf(PPLoadTextS(PPTXT_BUILD_LOCALDL600_FAIL, fmt_buf), (const char *)src_file_name);
				logger.Log(msg_buf);
			}
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	CATCH
		logger.LogLastError();
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI SelfBuild()
{
	int    ok = 1;
	PrcssrBuild prc;
	PrcssrBuild::Param param;
	THROW(prc.InitParam(&param));
	if(prc.EditParam(&param) > 0) {
		THROW(prc.Init(&param));
		THROW(prc.Run());
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI BuildLocalDL600()
{
	PrcssrBuild prc;
	return prc.BuildLocalDl600(0);
}
