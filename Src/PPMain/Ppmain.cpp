// PPMAIN.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2017, 2020, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <crpe.h>
#include <crtdbg.h> // _CrtDumpMemoryLeaks
//
//
//
#if defined(_PPSERVER) // {

int run_server();
int run_client();
int run_service();
int install_service(int inst, const char * pLoginName, const char * pLoginPassword);
int start_service(int start);
int RFIDPrcssr();

enum SrvCmd {
	srvcmdRun = 1,
	srvcmdClient,
	srvcmdInstall,
	srvcmdUninstall,
	srvcmdStart,
	srvcmdStop,
	srvcmdDemon,
	srvcmdRFIDPrcssr,
	srvcmdHelp
};

static int TranslateCmd(int cmd, SString & rCmdBuf)
{
	static const SIntToSymbTabEntry SrvCmdList[] = {
		{ srvcmdRun,        "service"      },
		{ srvcmdClient,     "client"       },
		{ srvcmdInstall,    "install"      },
		{ srvcmdUninstall,  "uninstall"    },
		{ srvcmdStart,      "servicestart" },
		{ srvcmdStop,       "servicestop"  },
		{ srvcmdDemon,      "demon"        },
		{ srvcmdDemon,      "daemon"       },
		{ srvcmdRFIDPrcssr, "rfidprcssr"   },
		{ srvcmdHelp,       "/?"           }
	};
	return cmd ? SIntToSymbTab_GetSymb(SrvCmdList, SIZEOFARRAY(SrvCmdList), cmd, rCmdBuf) : SIntToSymbTab_GetId(SrvCmdList, SIZEOFARRAY(SrvCmdList), rCmdBuf);
}

static SString & FormatCmdHelp(SrvCmd cmd, SString & rBuf, int addTabs)
{
	SString cmd_buf;
	TranslateCmd(cmd, cmd_buf);
	rBuf.Z().Tab().Cat("ppws").Space().Cat(cmd_buf).Space();
	if(addTabs)
		rBuf.Tab_(addTabs);
	return rBuf;
}

static void OutHelp(const char * pInvCmd)
{
	SString line_buf;
	if(pInvCmd)
		printf("Invalid command '%s'\n", pInvCmd);
	printf("Papyrus JobServer. Copyright (c) A.Sobolev 2005-2025\n");
	printf("Usage:\n");
	printf(FormatCmdHelp(srvcmdInstall,   line_buf, 0).
		Cat("[login_name] [login_password]\tservice installation").CR());
	printf(FormatCmdHelp(srvcmdUninstall, line_buf, 3).Cat("service uninstallation").CR());
	printf(FormatCmdHelp(srvcmdStart,     line_buf, 2).Cat("service starting").CR());
	printf(FormatCmdHelp(srvcmdStop,      line_buf, 2).Cat("service stopping").CR());
	printf(FormatCmdHelp(srvcmdRun,       line_buf, 3).Cat("service running").CR());
	printf(FormatCmdHelp(srvcmdHelp,      line_buf, 3).Cat("this help").CR());
}

static void OutPressAnyKey()
{
	//printf("Press any key to continue...");
}

#if SLTEST_RUNNING // {
int dummy_ppserver(); // Насильственная линковка модуля ppserver.cpp в случае сборки для автотестирования //
#endif // } SLTEST_RUNNING

int main(int argc, char ** argv)
{
	int    ret = 0;
#if SLTEST_RUNNING // {
	dummy_ppserver(); // Насильственная линковка модуля ppserver.cpp в случае сборки для автотестирования //
#endif // } SLTEST_RUNNING
	if(!PPSession::CheckExecutionLocking())
		ret = -1;
	else if(DS.Init(PPSession::internalappPapyrusServer, PPSession::fInitPaths, 0, /*pUiDescriptionFileName*/0)) {
		DS.SetMenu(0);
		SString cmd_buf;
		SString line_buf;
		SString login;
		SString pw;
		if(argc == 1) {
			OutHelp(0);
		}
		else {
			if(argc > 1) {
				cmd_buf = argv[1];
				SrvCmd cmd_id = (SrvCmd)TranslateCmd(0, cmd_buf);
				printf("Cmd arg = %s\n", cmd_buf.cptr()); // @debug
				switch(cmd_id) {
					case srvcmdInstall:
						if(argc > 2)
							login = argv[2];
						if(argc > 3)
							pw = argv[3];
						if(install_service(1, login, pw))
							printf("OK: Service installed successfully\n");
						else {
							printf("Error: Service does not installed\n");
							OutPressAnyKey();
							ret = -2;
						}
						break;
					case srvcmdUninstall:
						install_service(0, 0, 0);
						break;
					case srvcmdStart:
						if(start_service(1))
							printf("OK: Service started successfully\n");
						else {
							// @err,hr
							PPGetMessage(mfError, PPERR_SLIB, 0, 1, line_buf);
							printf("Error: Service does not started (%s)\n", line_buf.cptr());
							OutPressAnyKey();
							ret = -2;
						}
						break;
					case srvcmdStop:
						if(start_service(0))
							printf("OK: Service stopped\n");
						else {
							printf("Error: Service does not stopped\n");
							OutPressAnyKey();
							ret = -2;
						}
						break;
					case srvcmdRun:
						run_service();
						break;
					case srvcmdClient:
						run_client();
						break;
					case srvcmdDemon:
						run_server();
						break;
					case srvcmdRFIDPrcssr:
						RFIDPrcssr();
					case srvcmdHelp:
						OutHelp(0);
						break;
					default:
						OutHelp(cmd_buf);
						ret = -1;
						break;
				}
			}
		}
	}
	PPReleaseStrings();
	return ret;
}
#elif defined(_PPDLL) // } {
//
// DLL entry points {
//
#include <dl600.h>

STDAPI DllCanUnloadNow()
{
	TRACE_FUNC();
	return SCoClass::Helper_DllCanUnloadNow();
}

STDAPI DllGetClassObject(const CLSID & clsid, const IID & iID, void ** ppV) { return SCoClass::Helper_DllGetClassObject(clsid, iID, ppV); }
STDAPI DllRegisterServer() { return SCoClass::Helper_DllRegisterServer(0) ? S_OK : E_UNEXPECTED; }
STDAPI DllUnregisterServer() { return SCoClass::Helper_DllRegisterServer(1) ? S_OK : E_UNEXPECTED; }

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	static PPThread * p_last_thread = 0;
	switch(dwReason) {
		case DLL_PROCESS_ATTACH:
			if(!PPSession::CheckExecutionLocking())
				return FALSE;
			else {
				DS.Init(PPSession::internalappPapyrusIfc, PPSession::fInitPaths|PPSession::fDenyLogQueue, (HINSTANCE)hModule, /*pUiDescriptionFileName*/0);
				DS.SetExtFlag(ECF_SYSSERVICE, 1);
				DS.SetExtFlag(ECF_DLLMODULE, 1);
			}
			TRACE_FUNC_S("DLL_PROCESS_ATTACH");
			break;
		case DLL_THREAD_ATTACH:
			//p_last_thread = new PPThread(PPThread::kDllSession, "DLL session", 0);
			//p_last_thread->Start();
			//TRACE_FUNC_S("DLL_THREAD_ATTACH");
			break;
		case DLL_THREAD_DETACH:
			//TRACE_FUNC_S("DLL_THREAD_DETACH");
			//if(p_last_thread) {
			//	p_last_thread->Stop();
			//	ZDELETE(p_last_thread);
			//}
			break;
		case DLL_PROCESS_DETACH:
			TRACE_FUNC_S("DLL_PROCESS_DETACH");
			if(p_last_thread) {
				p_last_thread->Shutdown();
				ZDELETE(p_last_thread);
			}
			PPReleaseStrings();
			break;
	}
	return TRUE;
}
//
// } DLL entry points
//
#else // } !_PPSERVER && !_PPDLL {

int SGetAudioVolume(int decibels, double * pVolume); // @debug
int SSetAudioVolume(int decibels, double volume); // @debug
int ReformatIceCat(const char * pFileName);
// @experimental void ExploreIEEE754();
extern "C" int __declspec(dllexport) SelectVersion(HWND hWndOwner, char * pPath, long flags); // @debug

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int    ret = 0;
	if(!PPSession::CheckExecutionLocking())
		ret = -1;
	else {
		INITCOMMONCONTROLSEX iccex;
		iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		iccex.dwICC = ICC_COOL_CLASSES|ICC_BAR_CLASSES|ICC_LISTVIEW_CLASSES|ICC_USEREX_CLASSES|ICC_WIN95_CLASSES;
		InitCommonControlsEx(&iccex);
		if(DS.Init(PPSession::internalappPapyrusUi, PPSession::fInitPaths, 0, /*pUiDescriptionFileName*/"uid-papyrus.json")) {
//#if SLTEST_RUNNING
			TestNoLogin();
			// @experimental ExploreIEEE754();
			/*
			double curr_volume = 0.0;
			SGetAudioVolume(0, &curr_volume);
			SSetAudioVolume(0, 1.0);
			*/
			// @debug ReformatIceCat("D:/DEV/Resource/Data/Goods/icecat/prodid_d.txt"); // @once
//#endif
			if(!PEOpenEngine()) {
				PPSetError(PPERR_CRYSTAL_REPORT);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP);
			}
			// @debug {
#if 0 // {
			{
				//extern "C" int __declspec(dllexport) SelectVersion(HWND hWndOwner, char * pPath, long flags)
				SDynLibrary sv_lib("d:\\papyrus\\src\\build\\versel.dll");
				if(sv_lib.IsValid()) {
					char sel_ver_result[1024];
					int (* SelectVersionFunc)(HWND hWndOwner, char * pPath, long flags) = 0;
					SelectVersionFunc = reinterpret_cast<int (*)(HWND, char *, long)>(sv_lib.GetProcAddr("SelectVersion"));
					if(SelectVersionFunc) {
						SelectVersionFunc(0, sel_ver_result, 0);
					}
				}
			}
#endif // } 0
			// } @debug 
			DS.Register();
			{
				bool  font_smoothing_settled = false;
				// @v12.2.1 {
				const UiDescription * p_uid = SLS.GetUiDescription();
				if(p_uid) {
					SString fst;
					p_uid->VList.Get(UiValueList::vFontSmoothingType, fst);
					font_smoothing_settled = true;
					if(fst.IsEqiAscii("none")) {
						::SystemParametersInfo(SPI_SETFONTSMOOTHING, FALSE, 0, 0); 
					}
					else if(fst.IsEqiAscii("standard") || fst.IsEqiAscii("std")) {
						::SystemParametersInfo(SPI_SETFONTSMOOTHING, TRUE, 0, 0); 
						::SystemParametersInfo(SPI_SETFONTSMOOTHINGTYPE, 0, (PVOID)FE_FONTSMOOTHINGSTANDARD, 0); 
					}
					else if(fst.IsEqiAscii("cleartype")) {
						::SystemParametersInfo(SPI_SETFONTSMOOTHING, TRUE, 0, 0); 
						::SystemParametersInfo(SPI_SETFONTSMOOTHINGTYPE, 0, (PVOID)FE_FONTSMOOTHINGCLEARTYPE, 0); 
						int   fsc = 0;
						p_uid->VList.Get(UiValueList::vFontSmoothingContrast, fsc);
						if(fsc >= 1000 && fsc <= 2200)
							::SystemParametersInfo(SPI_SETFONTSMOOTHINGCONTRAST, 0, (PVOID)fsc, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
						else {
							::SystemParametersInfo(SPI_SETFONTSMOOTHINGCONTRAST, 0, (PVOID)1400, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
						}
					}
					else 
						font_smoothing_settled = false;
				}
				// } @v12.2.1 
				if(!font_smoothing_settled) {
					::SystemParametersInfo(SPI_SETFONTSMOOTHING, TRUE, 0, 0); 
					::SystemParametersInfo(SPI_SETFONTSMOOTHINGTYPE, 0, (PVOID)FE_FONTSMOOTHINGCLEARTYPE, 0); 
					::SystemParametersInfo(SPI_SETFONTSMOOTHINGCONTRAST, 0, (PVOID)1600, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
				}
			}
			DS.SetMenu(MENU_DEFAULT);
			{
				uint   app_ctrflags = 0;
				// @v11.6.7 {
				{
					SString arg_buf;
					if(PPSession::GetStartUpOption(PPSession::cmdlWsControl, arg_buf)) { 
						// С этим флагом интерактивная (и неинтерактивная тоже) часть системы значительно меняет свою работу!
						DS.SetExtFlag(ECF_WSCONTROL, 1);
						app_ctrflags |= TProgram::ctrfBorderless;
					}
				}
				// } @v11.6.7 
				PPApp app(hInst, SLS.GetAppName(), SLS.GetAppName(), app_ctrflags);
				app.run();
				DS.Unregister();
				SLS.Stop();
			}
		}
		else
			ret = -1;
	}
	PPReleaseStrings();
	return ret;
}

#endif // } !_PPSERVER && !_PPDLL
