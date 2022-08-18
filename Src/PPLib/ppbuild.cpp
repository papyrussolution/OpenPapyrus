// PPBUILD.CPP
// Copyright (c) A.Sobolev 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

int SelfbuildStaffForManual_ReservedObjTagList();
int SelfbuildStaffForManual_UserProfileFuncList();

int SelfbuildStaffForManual()
{
	int    ok = 1;
	SString temp_buf;
	PPIniFile ini_file;
	int    do_make_doc = 0;
	if(ini_file.GetInt(PPINISECT_SELFBUILD, PPINIPARAM_RESERVEDOBJECTS, &do_make_doc) && do_make_doc > 0) {
		THROW(SelfbuildStaffForManual_ReservedObjTagList());
		THROW(SelfbuildStaffForManual_UserProfileFuncList());		
	}
	CATCHZOKPPERR
	return ok;
}

PrcssrBuild::BuildVer::BuildVer() : Major(0), Minor(0), Revision(0), Asm(0)
{
}

PrcssrBuild::Param::Param() : Flags(0), ConfigEntryIdx(0), XpConfigEntryIdx(0)
{
	//PrefMsvsVerMajor = 0;
}

PrcssrBuild::Param::Param(const Param & rS)
{
	Copy(rS);
}

PrcssrBuild::Param & FASTCALL PrcssrBuild::Param::operator = (const Param & rS)
{
	return Copy(rS);
}

PrcssrBuild::Param & FASTCALL PrcssrBuild::Param::Copy(const Param & rS)
{
	Ver = rS.Ver;
	Flags = rS.Flags;
	ConfigEntryIdx = rS.ConfigEntryIdx;
	XpConfigEntryIdx = rS.XpConfigEntryIdx; // @v10.6.1
	VerSuffix = rS.VerSuffix;
	TSCollection_Copy(ConfigList, rS.ConfigList);
	return *this;
}

SString & PrcssrBuild::Param::GetVerLabel(SString & rBuf) const
{
	rBuf.Z().Cat(Ver.Major).Dot().Cat(Ver.Minor).Dot().CatLongZ(Ver.Revision, 2).CatChar('(').Cat(Ver.Asm).CatChar(')');
	return rBuf;
}

int	PrcssrBuild::InitConfigEntry(PPIniFile & rIniFile, const char * pSection, Param::ConfigEntry * pEntry)
{
	int    ok = 1;
	SString temp_buf;
	SString full_path_buf;
	rIniFile.Get(pSection, PPINIPARAM_BUILDROOT, temp_buf.Z());
	THROW_PP(temp_buf.NotEmpty(), PPERR_BUILD_UNDEFBUILDROOT);
	THROW_SL(fileExists(temp_buf));
	pEntry->RootPath = temp_buf;
	//
	rIniFile.Get(pSection, PPINIPARAM_BUILDSRC, temp_buf.Z());
	THROW_PP(temp_buf.NotEmpty(), PPERR_BUILD_UNDEFBUILDSRC);
	(full_path_buf = pEntry->RootPath).SetLastSlash().Cat(temp_buf);
	THROW_SL(fileExists(full_path_buf));
	pEntry->SrcPath = full_path_buf;
	//
	rIniFile.Get(pSection, PPINIPARAM_BUILDSOLUTION, temp_buf.Z());
	THROW_PP(temp_buf.NotEmpty(), PPERR_BUILD_UNDEFBUILDSLN);
	(full_path_buf = pEntry->RootPath).SetLastSlash().Cat(temp_buf);
	THROW_SL(fileExists(full_path_buf));
	pEntry->SlnPath = full_path_buf;
	//
	rIniFile.Get(pSection, PPINIPARAM_BUILDTARGET, temp_buf.Z());
	THROW_PP(temp_buf.NotEmpty(), PPERR_BUILD_UNDEFBUILDTARGET);
	THROW_SL(fileExists(temp_buf));
	pEntry->TargetRootPath = temp_buf;
	//
	rIniFile.Get(pSection, PPINIPARAM_BUILDNSIS, temp_buf.Z());
	if(temp_buf.IsEmpty()) {
		(temp_buf = pEntry->RootPath).SetLastSlash().Cat("tools").SetLastSlash().Cat("nsis").SetLastSlash().Cat("makensis.exe");
	}
	THROW_SL(fileExists(temp_buf));
	pEntry->NsisPath = temp_buf;
	//
	rIniFile.Get(pSection, PPINIPARAM_BUILDDISTRIB, temp_buf.Z());
	THROW_PP(temp_buf.NotEmpty(), PPERR_BUILD_UNDEFBUILDDISTRIB);
	THROW_SL(fileExists(temp_buf));
	pEntry->DistribPath = temp_buf;
	//
	rIniFile.Get(pSection, PPINIPARAM_PREFMSVSVER, temp_buf.Z());
	if(temp_buf.NotEmptyS()) {
		pEntry->PrefMsvsVerMajor = temp_buf.ToLong();
	}
	SETIFZ(pEntry->PrefMsvsVerMajor, 7);
	CATCHZOK
	return ok;
}

PrcssrBuild::Param::ConfigEntry * PrcssrBuild::SetupParamByEntryIdx(Param * pParam, int supplementalConfig)
{
	Param::ConfigEntry * p_entry = 0;
	const uint cfg_entry_idx = supplementalConfig ? pParam->XpConfigEntryIdx : pParam->ConfigEntryIdx;
	THROW(pParam && cfg_entry_idx > 0 && cfg_entry_idx <= pParam->ConfigList.getCount());
	p_entry = pParam->ConfigList.at(cfg_entry_idx-1);
	if(p_entry) {
		SString temp_buf;
		SString file_name_buf;
		SString full_path_buf;
		//
		// Извлекаем из файла SRC\RSRC\VERSION\genver.dat номер создаваемой версии
		//
		(temp_buf = p_entry->SrcPath).SetLastSlash().Cat("RSRC").SetLastSlash().Cat("Version").SetLastSlash();
		PPGetFileName(PPFILNAM_GENVER_DAT, file_name_buf);
		(full_path_buf = temp_buf).Cat(file_name_buf);
		if(!fileExists(full_path_buf)) {
			PPGetFileName(PPFILNAM_GENVEROPEN_DAT, file_name_buf);
			(full_path_buf = temp_buf).Cat(file_name_buf);
		}
		temp_buf = full_path_buf;
		THROW(fileExists(temp_buf));
		{
			SIniFile f_genver_file(temp_buf);
			PapyrusPrivateBlock ppb;
			THROW(ppb.ReadFromIni(f_genver_file));
			{
				ppb.Ver.Get(&pParam->Ver.Major, &pParam->Ver.Minor, &pParam->Ver.Revision);
				pParam->Ver.Asm = ppb.AssemblyN;
				SETFLAG(pParam->Flags, Param::fOpenSource, (ppb.Flags & ppb.fOpenSource));
			}
		}
	}
	CATCH
		p_entry = 0;
	ENDCATCH
	return p_entry;
}

int	PrcssrBuild::InitParam(Param * pParam)
{
	int    ok = 1;
	SString temp_buf, full_path_buf;
	SString file_name_buf;
	{
		SString left, right;
		PPIniFile ini_file;
		StringSet sections;
		ini_file.GetSections(&sections);
		for(uint sp = 0; sections.get(&sp, temp_buf);) {
			if(temp_buf.HasPrefixIAscii("selfbuild")) {
				if(temp_buf.Divide('-', left, right) > 0 || temp_buf.Divide(':', left, right) > 0) {
					uint   new_entry_pos = 0;
					Param::ConfigEntry * p_new_entry = pParam->ConfigList.CreateNewItem(&new_entry_pos);
					THROW_SL(p_new_entry);
					if(InitConfigEntry(ini_file, temp_buf, p_new_entry)) {
						p_new_entry->Name = right;
						if(!pParam->ConfigEntryIdx)
							pParam->ConfigEntryIdx = new_entry_pos+1;
					}
					else {
						p_new_entry = 0;
						pParam->ConfigList.atFree(new_entry_pos);
					}
				}
			}
		}
	}
	SetupParamByEntryIdx(pParam, 0/*supplementalConfig*/);
	pParam->Flags |= (Param::fBuildClient|Param::fBuildServer|Param::fBuildMtdll|
		Param::fBuildDrv|Param::fBuildSoap|Param::fBuildDistrib/*|Param::fCopyToUhtt*/);
	CATCHZOK
	return ok;
}

int	PrcssrBuild::EditParam(Param * pParam)
{
	class SelfBuildDialog : public TDialog {
		DECL_DIALOG_DATA(PrcssrBuild::Param);
	public:
		SelfBuildDialog(PrcssrBuild & rPrcssr) : TDialog(DLG_SELFBUILD), R_Prcssr(rPrcssr), PrevTimeoutRest(-1), StartClock(0)
		{
#ifdef NDEBUG
			CloseTimeout = 60;
#else
			CloseTimeout = -1;
#endif
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			RVALUEPTR(Data, pData);
			{
				StrAssocArray config_str_list;
				for(uint i = 0; i < Data.ConfigList.getCount(); i++) {
					Param::ConfigEntry * p_entry = Data.ConfigList.at(i);
					if(p_entry)
						config_str_list.Add(i+1, p_entry->Name);
				}
				SetupStrAssocCombo(this, CTLSEL_SELFBUILD_CONFIG, config_str_list, Data.ConfigEntryIdx, 0, 0, 0);
				SetupStrAssocCombo(this, CTLSEL_SELFBUILD_SCFG,   config_str_list, Data.XpConfigEntryIdx, 0, 0, 0); // @v10.6.1
			}
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 0, PrcssrBuild::Param::fBuildClient);
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 1, PrcssrBuild::Param::fBuildServer);
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 2, PrcssrBuild::Param::fBuildMtdll);
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 3, PrcssrBuild::Param::fBuildDrv);
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 4, PrcssrBuild::Param::fBuildSoap);
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 5, PrcssrBuild::Param::fBuildDistrib);
			AddClusterAssoc(CTL_SELFBUILD_FLAGS, 6, PrcssrBuild::Param::fCopyToUhtt);
			Setup();
			if(CloseTimeout >= 0)
				setStaticText(CTL_SELFBUILD_TIMEOUT, temp_buf.Z().Cat(CloseTimeout));
			StartClock = clock();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			getCtrlData(CTLSEL_SELFBUILD_CONFIG, &Data.ConfigEntryIdx);
			getCtrlData(CTLSEL_SELFBUILD_SCFG,   &Data.XpConfigEntryIdx); // @v10.6.1
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
			if(event.isCbSelected(CTLSEL_SELFBUILD_CONFIG)) {
				getCtrlData(CTLSEL_SELFBUILD_CONFIG, &Data.ConfigEntryIdx);
				Setup();
			}
			else if(TVCMD == cmInputUpdated) {
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
						setStaticText(CTL_SELFBUILD_TIMEOUT, temp_buf.Z().Cat(timeout_rest));
					PrevTimeoutRest = timeout_rest;
					if(diff >= (CloseTimeout * CLOCKS_PER_SEC)) {
						if(IsInState(sfModal)) {
							// @v10.5.9 clearEvent(event);
							endModal(cmOK);
							return; // После endModal не следует обращаться к this
						}
					}
				}
			}
			else
				return;
		}
		void Setup()
		{
			SString temp_buf;
			const Param::ConfigEntry * p_config_entry = R_Prcssr.SetupParamByEntryIdx(&Data, 0/*supplementalConfig*/);
			SetClusterData(CTL_SELFBUILD_FLAGS, Data.Flags);
			setCtrlString(CTL_SELFBUILD_VERSION, Data.GetVerLabel(temp_buf));
			setCtrlString(CTL_SELFBUILD_VERSFX, Data.VerSuffix);
			{
				temp_buf.Z();
				if(p_config_entry) {
					StrAssocArray msvs_ver_list;
					PrcssrBuild::FindMsvs(p_config_entry->PrefMsvsVerMajor, msvs_ver_list, &temp_buf);
					setCtrlString(CTL_SELFBUILD_CMPLRPATH, temp_buf);
				}
				setCtrlString(CTL_SELFBUILD_CMPLRPATH, temp_buf);
			}
			setCtrlString(CTL_SELFBUILD_IMPATH, p_config_entry ? p_config_entry->NsisPath : temp_buf.Z());
			setStaticText(CTL_SELFBUILD_ST_INFO, (Data.Flags & Data.fOpenSource) ? "OPENSOURCE" : 0);
		}
		long   CloseTimeout;
		long   PrevTimeoutRest;
		clock_t StartClock;
		PrcssrBuild & R_Prcssr;
	};
	DIALOG_PROC_BODY_P1(SelfBuildDialog, *this, pParam);
}

int	PrcssrBuild::Init(const Param * pParam)
{
	int    ok = 1;
	RVALUEPTR(P, pParam);
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

/*static*/int PrcssrBuild::FindMsvs(int prefMsvsVerMajor, StrAssocArray & rList, SString * pPrefPath)
{
	int    ok = -1;
	rList.Z();
	SString temp_buf, major, minor, subkey_buf, path_buf;
	StrAssocArray msvs_ver_list;
	{
		const char * p_sub_msvs = "Software\\Microsoft\\VisualStudio";
		WinRegKey reg_key(HKEY_LOCAL_MACHINE, p_sub_msvs, 1/*readonly*/);
		for(uint kidx = 0; reg_key.EnumKeys(&kidx, temp_buf);) {
			if(temp_buf.Divide('.', major, minor) > 0) {
				long msvs_ver = (major.ToLong() << 16) | (minor.ToLong() & 0xffff);
				(subkey_buf = p_sub_msvs).SetLastSlash().Cat(temp_buf); //.Cat("InstallDir");
				WinRegKey reg_key_ver(HKEY_LOCAL_MACHINE, subkey_buf, 1/*readonly*/);
				if(reg_key_ver.GetString("InstallDir", path_buf) > 0 && path_buf.NotEmpty() && fileExists(path_buf)) {
					rList.Add(msvs_ver, path_buf);
					ok = 1;
				}
			}
		}
	}
	{
		const char * p_sub_msvs_2 = "Software\\Microsoft\\VisualStudio\\SxS\\VS7";
		WinRegKey reg_key(HKEY_LOCAL_MACHINE, p_sub_msvs_2, 1/*readonly*/);
		WinRegValue reg_val;
		for(uint vidx = 0; reg_key.EnumValues(&vidx, &temp_buf, &reg_val);) {
			if(temp_buf.Divide('.', major, minor) > 0) {
				long msvs_ver = (major.ToLong() << 16) | (minor.ToLong() & 0xffff);
				if(reg_val.GetString(path_buf) && path_buf.NotEmpty() && fileExists(path_buf)) {
					path_buf.SetLastSlash().Cat("Common7").SetLastSlash().Cat("IDE").SetLastSlash();
					if(fileExists(path_buf)) {
						rList.Add(msvs_ver, path_buf, 0/*replaceDup*/);
						ok = 1;
					}
				}
			}
		}
	}
	if(ok > 0 && pPrefPath) {
		for(uint i = 0; pPrefPath->IsEmpty() && i < rList.getCount(); i++) {
			StrAssocArray::Item item = rList.Get(i);
			const int msvs_ver_major = (item.Id >> 16);
			const int msvs_ver_minor = (item.Id & 0xffff);
			if(prefMsvsVerMajor) {
				if(msvs_ver_major == prefMsvsVerMajor) {
					(*pPrefPath = item.Txt).SetLastSlash().Cat("devenv").DotCat("exe");
					ok = 2;
				}
			}
			else if(msvs_ver_major == 7 && msvs_ver_minor == 1) {
				(*pPrefPath = item.Txt).SetLastSlash().Cat("devenv").DotCat("exe");
				ok = 2;
			}
		}
	}
	return ok;
}

/*static*/int PrcssrBuild::CopyProgressProc(const SDataMoveProgressInfo * scfd)
{
	const long pct = (long)(100L * scfd->SizeDone / scfd->SizeTotal);
	PPWaitPercent(pct, SString(scfd->P_Src).Space().Cat("-->").Space().Cat(scfd->P_Dest));
	return SPRGRS_CONTINUE;
}

int	PrcssrBuild::UploadFileToUhtt(const char * pFileName, const char * pKey, const char * pVerLabel, const char * pMemo)
{
	int    ok = 1;
	PPUhttClient uhtt_cli;
	THROW(uhtt_cli.Auth());
	THROW(uhtt_cli.FileVersionAdd(pFileName, pKey, pVerLabel, pMemo, PrcssrBuild::CopyProgressProc, this));
	CATCHZOK
	return ok;
}

int PrcssrBuild::Helper_Compile(const Param::ConfigEntry * pCfgEntry, int supplementalConfig, PPLogger & rLogger)
{
	int    ok = 1;
	SString temp_buf;
	SString msg_buf;
	SString fmt_buf;
	SString build_log_path;
	//
	// Сборка исполняемых файлов и ресурсов
	//
	struct SolutionEntry {
		const char * P_Name;
		const char * P_Sln;
		const char * P_Config;
		const char * P_Result;
		const char * P_XpCompatResult;
		long   Flag;
	};
	SolutionEntry sln_list[] = {
		// P_Name          P_Sln                P_Config         P_Result           Flag
		{ "client",        "papyrus.sln",       "Release",       
			"ppw.exe;ppdrv-pirit.dll;ppdrv-cd-vikivision.dll;ppdrv-cd-vfd-epson.dll;ppdrv-cd-shtrih-dpd201.dll;ppdrv-cd-posiflex.dll;ppdrv-cd-flytechvfd-epson.dll;ppdrv-ie-korus.dll;"
			"ppdrv-ie-edisoft.dll;ppdrv-ie-kontur.dll;ppdrv-ie-leradata.dll;ppdrv-ie-alcodeclbill.dll;ppdrv-ctrl-reversk2.dll;ppdrv-crdr-emmarine.dll;ppdrv-bnkt-sberbank.dll;"
			"ppdrv-bnkt-emul.dll;ppdrv-bnkt-inpas.dll",
			"ppw.exe;ppdrv-pirit.dll;ppdrv-bnkt-sberbank.dll;ppdrv-bnkt-inpas.dll",
			Param::fBuildClient|Param::fSupplementalBuild },
		{ "mtdll",         "papyrus.sln",       "MtDllRelease",  "ppwmt.dll", "ppwmt.dll", Param::fBuildMtdll|Param::fSupplementalBuild },
		{ "jobsrv",        "papyrus.sln",       "ServerRelease", "ppws.exe", 0, Param::fBuildServer },
		// @v9.6.9 { "equipsolution", "EquipSolution.sln", "Release",       "ppdrv-pirit.dll", Param::fBuildDrv },
		// @v8.3.2 { "ppsoapmodules", "PPSoapModules.sln", "Release",       "PPSoapUhtt.dll",  Param::fBuildSoap }
	};
	StrAssocArray msvs_ver_list;
	SString msvs_path;
	SString name_buf;
	StringSet result_file_list(";");
	const char * p_prc_cur_dir = pCfgEntry->SlnPath.NotEmpty() ? pCfgEntry->SlnPath.cptr() : static_cast<const char *>(0);
	rLogger.Log((msg_buf = "Current dir for child processes").CatDiv(':', 2).Cat(p_prc_cur_dir));
	THROW(FindMsvs(pCfgEntry->PrefMsvsVerMajor, msvs_ver_list, &msvs_path));
	PPLoadText(PPTXT_BUILD_COMPILERNAME_VS71, temp_buf);
	THROW_PP_S(msvs_path.NotEmpty() && fileExists(msvs_path), PPERR_BUILD_COMPILERNFOUND, temp_buf);
	for(uint j = 0; j < SIZEOFARRAY(sln_list); j++) {
		SolutionEntry & r_sln_entry = sln_list[j];
		if(P.Flags & r_sln_entry.Flag && (!supplementalConfig || r_sln_entry.Flag & Param::fSupplementalBuild)) { // @v10.6.1 (!supplementalConfig || r_sln_entry.Flag & Param::fSupplementalBuild)
			PPGetPath(PPPATH_LOG, build_log_path);
			const char * p_log_build_text = supplementalConfig ? "build_xp" : "build";
			build_log_path.SetLastSlash().Cat(p_log_build_text).CatChar('-').Cat(r_sln_entry.P_Name).DotCat("log");
			//D:\msvs70\common7\ide\devenv.exe papyrus.sln /rebuild "Release" /out %BUILDLOG%\client.log
			{
				STARTUPINFO si;
				DWORD exit_code = 0;
				PROCESS_INFORMATION pi;
				MEMSZERO(si);
				si.cb = sizeof(si);
				MEMSZERO(pi);
				temp_buf.Z().CatQStr(msvs_path).Space().Cat(r_sln_entry.P_Sln).Space().Cat("/rebuild").Space().
					Cat(r_sln_entry.P_Config).Space().Cat("/out").Space().Cat(build_log_path);
				STempBuffer cmd_line((temp_buf.Len() + 32) * sizeof(TCHAR));
				strnzcpy(static_cast<TCHAR *>(cmd_line.vptr()), SUcSwitch(temp_buf), cmd_line.GetSize() / sizeof(TCHAR));
				PPLoadText(PPTXT_BUILD_SOLUTION, fmt_buf);
				rLogger.Log(msg_buf.Printf(fmt_buf, temp_buf.cptr()));
				int    r = ::CreateProcess(0, static_cast<TCHAR *>(cmd_line.vptr()), 0, 0, FALSE, 0, 0, SUcSwitch(p_prc_cur_dir), &si, &pi);
				if(!r) {
					SLS.SetOsError(0);
					CALLEXCEPT_PP(PPERR_SLIB);
				}
				WaitForSingleObject(pi.hProcess, INFINITE); // Wait until child process exits.
				{
					GetExitCodeProcess(pi.hProcess, &exit_code);
					if(exit_code == 0) {
                        int    result_files_are_ok = 1;
						int    result_file_no = 0;
						{
							result_file_list.setBuf(r_sln_entry.P_Result, sstrlen(r_sln_entry.P_Result)+1);
							for(uint ssp = 0; result_file_list.get(&ssp, name_buf);) {
								result_file_no++;
								if(name_buf.NotEmptyS()) {
									(temp_buf = pCfgEntry->TargetRootPath).SetLastSlash().Cat("BIN").SetLastSlash().Cat(name_buf);
									if(!fileExists(temp_buf)) {
										result_files_are_ok = 0;
										msg_buf.Printf(PPLoadTextS(PPTXT_BUILD_SLN_TARGETNFOUND, fmt_buf), r_sln_entry.P_Name, temp_buf.cptr());
										rLogger.Log(msg_buf);
									}
								}
							}
						}
						if(supplementalConfig && !isempty(r_sln_entry.P_XpCompatResult)) {
							result_file_list.setBuf(r_sln_entry.P_XpCompatResult, sstrlen(r_sln_entry.P_XpCompatResult)+1);
							SString supplement_file_name;
							for(uint ssp = 0; result_file_list.get(&ssp, name_buf);) {
								(temp_buf = pCfgEntry->TargetRootPath).SetLastSlash().Cat("BIN").SetLastSlash().Cat(name_buf);
								if(fileExists(temp_buf)) {
									SPathStruc ps(temp_buf);
									ps.Nam.Cat("-xp");
									ps.Merge(supplement_file_name);
									if(!SCopyFile(temp_buf, supplement_file_name, 0, FILE_SHARE_READ, 0)) {
										PPSetError(PPERR_SLIB);
										rLogger.LogLastError();
									}
								}
							}
						}
						if(result_files_are_ok) {
							msg_buf.Printf(PPLoadTextS(PPTXT_BUILD_SLN_SUCCESS, fmt_buf), r_sln_entry.P_Name);
							rLogger.Log(msg_buf);
						}
					}
					else {
						msg_buf.Printf(PPLoadTextS(PPTXT_BUILD_SLN_FAIL, fmt_buf), r_sln_entry.P_Name, build_log_path.cptr());
						rLogger.Log(msg_buf);
					}
				}
				// Close process and thread handles.
				::CloseHandle(pi.hProcess);
				::CloseHandle(pi.hThread);
			}
		}
	}
	CATCHZOK
	return ok;
}

int	PrcssrBuild::Run()
{
	int    ok = 1;
	uint   i;
	PPLogger logger;
	SString temp_buf, build_log_path, fmt_buf, msg_buf;

	// @debug {
	//
	/*
	//(temp_buf = P.RootPath).SetLastSlash().Cat("ManWork").SetLastSlash().Cat("LaTex").SetLastSlash().Cat("ppmanual.pdf");
	//temp_buf = P.RootPath;
	temp_buf = "/papyrus";
	temp_buf.SetLastSlash().Cat("ManWork").SetLastSlash().Cat("LaTex").SetLastSlash().Cat("ppmanual.pdf");
	if(fileExists(temp_buf)) {
		PPWaitStart();
		UploadFileToUhtt(temp_buf, "papyrus-manual", "test-ver-3", "");
		PPWaitStop();
	}
	*/
	// } @debug
	const Param::ConfigEntry * p_config_entry = SetupParamByEntryIdx(&P, 0/*supplementalConfig*/);
	const Param::ConfigEntry * p_supplemental_config_entry = SetupParamByEntryIdx(&P, 1/*supplementalConfig*/);
	THROW(p_config_entry);
	if(p_supplemental_config_entry) {
		logger.Log((msg_buf = "Supplemental Configuration").CatDiv(':', 2).Cat(p_supplemental_config_entry->Name));
		THROW(Helper_Compile(p_supplemental_config_entry, 1, logger));
	}
	logger.Log((msg_buf = "Configuration").CatDiv(':', 2).Cat(p_config_entry->Name));
	THROW(Helper_Compile(p_config_entry, 0, logger));
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
			sub_path.Z().CatChar('v').Cat(P.Ver.Major).Cat(P.Ver.Minor).CatLongZ(P.Ver.Revision, 2).CatChar('a' + (P.Ver.Asm + _c) % 26);
			if(P.VerSuffix.NotEmpty())
				sub_path.CatChar('-').Cat(P.VerSuffix);
			(distrib_path = p_config_entry->DistribPath).SetLastSlash().Cat(sub_path);
			_c++;
		} while(fileExists(distrib_path));
		THROW(::createDir(distrib_path));
		(distrib_src_path = p_config_entry->DistribPath).SetLastSlash().Cat("SRC").SetLastSlash().Cat(sub_path);
		THROW(::createDir(distrib_src_path));
		//
		(temp_buf = p_config_entry->RootPath).SetLastSlash().Cat("ManWork").SetLastSlash().Cat("LaTex").SetLastSlash().Cat("ppmanual.pdf");
		STRNSCPY(nsis_list[4].FileName, temp_buf);
		//
		(temp_buf = p_config_entry->RootPath).SetLastSlash().Cat("ManWork").SetLastSlash().Cat("LaTex").SetLastSlash().Cat("features.pdf");
		STRNSCPY(nsis_list[5].FileName, temp_buf);
		//
		(temp_buf = p_config_entry->SrcPath).SetLastSlash().Cat("doc").SetLastSlash().Cat("version.txt");
		STRNSCPY(nsis_list[6].FileName, temp_buf);
		//
		(build_path = p_config_entry->SrcPath).SetLastSlash().Cat("BUILD");
		for(i = 0; i < SIZEOFARRAY(nsis_list); i++) {
			NsisEntry & r_nsis_entry = nsis_list[i];
			if(r_nsis_entry.P_NsisFile) {
				STARTUPINFO si;
				DWORD exit_code = 0;
				PROCESS_INFORMATION pi;
				MEMSZERO(si);
				si.cb = sizeof(si);
				MEMSZERO(pi);
				target_file_name = (P.Flags & Param::fOpenSource) ? "OPpy" : "Ppy";
				target_file_name.Cat(r_nsis_entry.P_Name).CatChar('_').Cat(ver_label).DotCat("exe");
				PPGetPath(PPPATH_LOG, build_log_path);
				build_log_path.SetLastSlash().Cat("build").CatChar('-').Cat("nsis").CatChar('-').Cat(r_nsis_entry.P_Name).DotCat("log");
				temp_buf.Z().CatQStr(p_config_entry->NsisPath).Space().CatEq("/DPRODUCT_VERSION", ver_label).Space().
					CatEq("/DSRC_ROOT", p_config_entry->RootPath).Space().Cat("/NOCD").Space().Cat("/V2").Space().Cat("/P1").Space();
				if(r_nsis_entry.P_Config)
					temp_buf.Cat("/D").Cat(r_nsis_entry.P_Config).Space();
				// @v10.6.1 {
				if(p_supplemental_config_entry)
					temp_buf.Cat("/D").Cat("XPCOMPAT").Space();
				// } @v10.6.1 
				// @v9.4.9 {
				if(P.Flags & Param::fOpenSource)
					temp_buf.Cat("/D").Cat("OPENSOURCE").Space();
				// } @v9.4.9
				temp_buf.Cat("/O").Cat(build_log_path).Space().Cat(r_nsis_entry.P_NsisFile);

				STempBuffer cmd_line((temp_buf.Len() + 32) * sizeof(TCHAR));
				strnzcpy(static_cast<TCHAR *>(cmd_line.vptr()), SUcSwitch(temp_buf), cmd_line.GetSize()/sizeof(TCHAR));
				PPLoadText(PPTXT_BUILD_DISTRIB, fmt_buf);
				logger.Log(msg_buf.Printf(fmt_buf, temp_buf.cptr()));
				int    r = ::CreateProcess(0, static_cast<TCHAR *>(cmd_line.vptr()), 0, 0, FALSE, 0, 0, SUcSwitch(build_path), &si, &pi); // @unicodeproblem
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
							msg_buf.Printf(fmt_buf, target_file_name.cptr());
							logger.Log(msg_buf);
							//
							(dest_path = distrib_path).SetLastSlash().Cat(target_file_name);
							THROW_SL(SCopyFile(temp_buf, dest_path, 0, FILE_SHARE_READ, 0));
							SFile::Remove(temp_buf);
							dest_path.CopyTo(r_nsis_entry.FileName, sizeof(r_nsis_entry.FileName));
						}
						else {
							PPLoadText(PPTXT_BUILD_DISTRIB_TARGETNFOUND, fmt_buf);
							msg_buf.Printf(fmt_buf, target_file_name.cptr(), temp_buf.cptr());
							logger.Log(msg_buf);
						}
					}
					else {
						PPLoadText(PPTXT_BUILD_DISTRIB_FAIL, fmt_buf);
						msg_buf.Printf(fmt_buf, target_file_name.cptr(), build_log_path.cptr());
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
			SString uhtt_symb;
			PPWaitStart();
			for(i = 0; i < SIZEOFARRAY(nsis_list); i++) {
				NsisEntry & r_nsis_entry = nsis_list[i];
				if(r_nsis_entry.P_UhttSymb && r_nsis_entry.FileName[0] && fileExists(r_nsis_entry.FileName)) {
					if(P.Flags & Param::fOpenSource) {
						(uhtt_symb = "open").Cat(r_nsis_entry.P_UhttSymb);
					}
					else
						uhtt_symb = r_nsis_entry.P_UhttSymb;
					{
						//PPTXT_BUILD_UHTTCOPY_INFO        "Копирование на сервер @{brand_uhtt}"
						PPLoadText(PPTXT_BUILD_UHTTCOPY_INFO, msg_buf);
						msg_buf.CatDiv(':', 2).Cat(r_nsis_entry.FileName).CatDiv('-', 1).Cat(uhtt_symb).CatDiv('-', 1).Cat(ver_label);
						logger.Log(msg_buf);
					}
					if(UploadFileToUhtt(r_nsis_entry.FileName, uhtt_symb, ver_label, 0)) {
						PPLoadText(PPTXT_BUILD_UHTTCOPY_SUCCESS, fmt_buf);
						msg_buf.Printf(fmt_buf, r_nsis_entry.FileName);
						logger.Log(msg_buf);
					}
					else {
						PPLoadText(PPTXT_BUILD_UHTTCOPY_FAIL, fmt_buf);
						PPGetLastErrorMessage(1, temp_buf);
						msg_buf.Printf(fmt_buf, r_nsis_entry.FileName, temp_buf.cptr());
						logger.Log(msg_buf);
					}
				}
			}
			PPWaitStop();
		}
	}
	CATCH
		logger.LogLastError();
		ok = 0;
	ENDCATCH
	PPWaitStop();
	logger.Save(PPFILNAM_SELFBUILD_LOG, 0);
	return ok;
}

int PrcssrBuild::BuildLocalDl600(const char * pPath)
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
		cmd_line.Z().CatQStr(temp_buf);
		if(isempty(pPath)) {
			PPGetFilePath(PPPATH_DD, "local.dl6", src_file_name);
		}
		else {
			ps.Split(pPath);
			if(ps.Drv.IsEmpty() && ps.Dir.IsEmpty()) {
				PPGetFilePath(PPPATH_DD, pPath, src_file_name);
			}
		}
		THROW(fileExists(src_file_name));
		{
			ps.Split(src_file_name);
			ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, cur_dir);
		}
		cmd_line.Space().Cat("/ob").Space().CatQStr(src_file_name);
		PPLoadText(PPTXT_BUILD_LOCALDL600, fmt_buf);
		logger.Log(msg_buf.Printf(fmt_buf, cmd_line.cptr()));
		{
			STempBuffer cmd_line_buf((cmd_line.Len() + 32) * sizeof(TCHAR));
			strnzcpy(static_cast<TCHAR *>(cmd_line_buf.vptr()), SUcSwitch(cmd_line), cmd_line_buf.GetSize()/sizeof(TCHAR));
			cpr = ::CreateProcess(0, static_cast<TCHAR *>(cmd_line_buf.vptr()), 0, 0, FALSE, 0, 0, SUcSwitch(cur_dir), &si, &pi); // @unicodeproblem
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
				msg_buf.Printf(PPLoadTextS(PPTXT_BUILD_LOCALDL600_FAIL, fmt_buf), src_file_name.cptr());
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

int SelfBuild()
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

int BuildLocalDL600()
{
	return PrcssrBuild().BuildLocalDl600(0);
}

int ParseWinRcForNativeText()
{
	int    ok = 1;
	SString line_buf;
	SString out_line_buf;
	SString temp_buf;
	SString build_root_path;
	SString src_file_name;
	PPIniFile ini_file;
	ini_file.Get(PPINISECT_SELFBUILD, PPINIPARAM_BUILDROOT, temp_buf.Z());
	THROW_PP(temp_buf.NotEmpty(), PPERR_BUILD_UNDEFBUILDROOT);
	THROW_SL(fileExists(temp_buf));
	build_root_path = temp_buf;
	//
	ini_file.Get(PPINISECT_SELFBUILD, PPINIPARAM_BUILDSRC, temp_buf.Z());
	THROW_PP(temp_buf.NotEmpty(), PPERR_BUILD_UNDEFBUILDSRC);
	(src_file_name = build_root_path).SetLastSlash().Cat(temp_buf);
	THROW_SL(fileExists(src_file_name));
	src_file_name.SetLastSlash().Cat("ppmain\\ppw.rc");
	{
		SFile  f_in(src_file_name, SFile::mRead);
		if(f_in.IsValid()) {
			SPathStruc ps(src_file_name);
			ps.Nam.CatChar('-').Cat("nativetext");
			ps.Ext = "tsv";
			ps.Merge(temp_buf);
			SFile  f_out(temp_buf, SFile::mWrite);
			uint   line_no = 0;
			while(f_in.ReadLine(line_buf, SFile::rlfChomp)) {
				line_no++;
				SStrScan scan(line_buf);
				while(scan.SearchChar('\"')) {
					scan.IncrLen();
					if(scan.GetQuotedString(temp_buf)) {
						temp_buf.Strip();
						int    is_native_text = 0;
						for(uint cidx = 0; !is_native_text && cidx < temp_buf.Len(); cidx++) {
							uchar c = static_cast<uchar>(temp_buf.C(cidx));
							if(c > 127)
								is_native_text = 1;
						}
						if(is_native_text) {
							out_line_buf.Z().Cat(temp_buf).Tab().Cat(line_no).CR().Transf(CTRANSF_OUTER_TO_UTF8);
							f_out.WriteLine(out_line_buf);
						}
					}
					else
						scan.Incr();
				}
			}
		}
	}
	CATCH
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

class GroovyConfig {
public:
	GroovyConfig() : ParamT(2048)
	{
	}
private:
	SymbHashTable ParamT; 
	StrAssocArray NL; // Иерархический список узлов. Идентификаторы соответствуют параметрам (наименования которых хранятся в ParamT), а строки - значениям.
};

// @v11.4.5 @construction
int ProcessStyloQGradle(const char * pPath, int buildSerial, const char * pVersion)
{
	//
	// Нам нужно сделать следующие автоматические изменения в build.gradle:
	// 1. Установить номер сборки (android/defaultConfig/versionCode)
	// 2. Установить строку версии (android/defaultConfig/versionName)
	// 3. Модифицировать параметры подписи приложения (android/signingConfigs)
	//
	/*
		plugins {
			id 'com.android.application'
		}

		android {
			signingConfigs {
				debug {
					storeFile file('d:\\papyrus\\DISTRIB\\StyloQ\\styloq.keystore')
					storePassword 'proton'
					keyAlias 'styloq'
					keyPassword 'proton'
				}
				release {
					storeFile file('d:\\papyrus\\DISTRIB\\StyloQ\\styloq.keystore')
					storePassword 'proton'
					keyAlias 'styloq'
					keyPassword 'proton'
				}
			}
			compileSdkVersion 30

			defaultConfig {
				applicationId "ru.petroglif.styloq"
				minSdkVersion 28
				targetSdkVersion 30
				versionCode 16
				versionName '0.1.03'
				testInstrumentationRunner "androidx.test.runner.AndroidJUnitRunner"
				signingConfig signingConfigs.release
			}

			buildTypes {
				release {
					minifyEnabled false
					proguardFiles getDefaultProguardFile('proguard-android-optimize.txt'), 'proguard-rules.pro'
				}
			}
			compileOptions {
				sourceCompatibility JavaVersion.VERSION_1_8
				targetCompatibility JavaVersion.VERSION_1_8
			}
			buildFeatures {
				viewBinding true
			}
			ndkVersion '21.4.7075529'
		}

		dependencies {
			implementation "androidx.fragment:fragment:1.3.6"
			implementation 'androidx.appcompat:appcompat:1.3.0'
			implementation 'com.google.android.material:material:1.4.0'
			implementation 'androidx.constraintlayout:constraintlayout:2.0.4'
			implementation 'androidx.navigation:navigation-fragment:2.3.5'
			implementation 'androidx.navigation:navigation-ui:2.3.5'
			implementation 'com.journeyapps:zxing-android-embedded:4.3.0'
			implementation 'com.rabbitmq:amqp-client:5.15.0'
			implementation 'com.blankj:utilcodex:1.31.0'
			implementation "com.android.support:support-core-utils:28.0.0"
			implementation 'androidx.room:room-compiler:2.2.6'
			implementation 'androidx.legacy:legacy-support-v4:1.0.0'
			implementation 'com.github.bumptech.glide:glide:4.13.2'
			implementation 'com.google.android.play:core:1.10.3'

			implementation 'com.github.bumptech.glide:annotations:4.13.1'
			annotationProcessor 'com.github.bumptech.glide:compiler:4.13.1'

			testImplementation 'junit:junit:4.+'
			androidTestImplementation 'androidx.test.ext:junit:1.1.2'
			androidTestImplementation 'androidx.test.espresso:espresso-core:3.3.0'
		}

		repositories {
			mavenCentral()
		}
	*/
	int    ok = -1;
	return ok;
}