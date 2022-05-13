// WMI.CPP
// Copyright (c) A.Sobolev 2008, 2010, 2015, 2018, 2020
//
#include <slib-internal.h>
#pragma hdrstop
#include <comdef.h>
#include <wbemidl.h>
//
//
//
SWmi::SWmi() : State(0), P_Loc(0), P_Svc(0)
{
	CoInitializeEx(0, COINIT_APARTMENTTHREADED);
}

SWmi::~SWmi()
{
	Release();
	CoUninitialize();
	//OleUninitialize();
}
//
// соединение с сервисом WMI
//
//const char * const SWmi::P_NS_DEFAULT = "\\root\\default";
//const char * const SWmi::P_NS_CIMV2	  = "\\root\\cimv2";
//const char * const SWmi::P_UNC_LOCAL  = ".";
//static const char *p_name_space = "root\\cimv2";

int SWmi::Connect(const char * pServer, const char * pUserName, const char * pPassword)
{
	EXCEPTVAR(SLibError);
	Release(); // если соединения не было, то никаких побочных действий не выполнится //
	int    ok = 1, r;
	BSTR   bs_namespace = 0;
	BSTR   bs_user = 0;
	BSTR   bs_pw = 0;
	r = CoInitializeSecurity(0, -1, 0, 0, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_NONE, 0);
	THROW(r == 0 || r == RPC_E_TOO_LATE);
	THROW_V(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void **)&P_Loc) == S_OK,
		SLERR_WMI_CREATELOCATOR);
	{
		SString t_buf, ns;
		//const  char * p_name_space = "root\\cimv2";
		if(!isempty(pUserName) && !isempty(pServer)) {
			(t_buf = pUserName).CopyToOleStr(&bs_user);
			if(!isempty(pPassword))
				(t_buf = pPassword).CopyToOleStr(&bs_pw);
		}
		ns.CatCharN('\\', 2);
		if(isempty(pServer))
			ns.Dot();
		else
			ns.Cat(pServer);
		ns.SetLastSlash().Cat("root").SetLastSlash().Cat("cimv2").CopyToOleStr(&bs_namespace);
		SLS.SetAddedMsgString(ns);
		THROW_V(P_Loc->ConnectServer(bs_namespace, bs_user, bs_pw, 0, 0, 0, 0, &P_Svc) == 0, SLERR_WMI_CONNECTSRV);
	}
	THROW_V(CoSetProxyBlanket(P_Svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, 0,
		RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_NONE) == 0, SLERR_WMI_SETPROXIBLANKET);
	State |= sConnected;
	CATCHZOK
	SysFreeString(bs_pw);
	SysFreeString(bs_user);
	SysFreeString(bs_namespace);
	return ok;
}

int SWmi::GetSvcError(SString & rBuf)
{
	int    ok = 0;
	rBuf.Z();
	IErrorInfo * p_err_info = 0;
	GetErrorInfo(0, &p_err_info);
	if(p_err_info) {
		BSTR bs_err = 0;
		int r = p_err_info->GetDescription(&bs_err);
		rBuf.CopyFromOleStr(bs_err);
		ok = 1;
		SysFreeString(bs_err);
		p_err_info->Release();
	}
	return ok;
}

int SWmi::GetMethodList(IWbemClassObject * pCls, StrAssocArray * pList)
{
	int    ok = 0;
	if(pCls) {
		int r = pCls->BeginMethodEnumeration(0/*WBEM_FLAG_LOCAL_ONLY|WBEM_FLAG_PROPAGATED_ONLY*/);
		if(r == WBEM_S_NO_ERROR) {
			long i = 0;
			SString name;
			BSTR bs_method_name;
			while((r = pCls->NextMethod(0, &bs_method_name, 0, 0)) == S_OK) {
				pList->Add(++i, name.CopyFromOleStr(bs_method_name));
				ok = 1;
			}
			pCls->EndMethodEnumeration();
		}
	}
	return ok;
}
//
// запуск процесса
//
int SWmi::Method_CreateProcess(const char * pCmdLine)
{
	enum {
		sWin32Process = 0,
		sCreate,
		sCommandLine,
		sReturnValue
	};
	const char * str_const_list[] = {
		"Win32_Process",
		"Create",
		"CommandLine",
		"ReturnValue"
	};
	EXCEPTVAR(SLibError);
	int    ok = 1, r;
	SString temp_buf, temp_buf2, err_buf;
	BSTR   bs_buf = 0, bs_buf2 = 0;
	IWbemClassObject * p_out = 0;
	IWbemClassObject * p_inparams = 0;
	IWbemClassObject * p_incls = 0;
	IWbemClassObject * p_process = 0;
	/*
	if(NameSpace != _bstr_t(P_NS_CIMV2))
		Connect(Server, User, Password, P_NS_CIMV2);
	*/
	if(State & sConnected) {
		VARIANT v_param;
		VARIANT v_retval;
		r = P_Svc->GetObject(_bstr_t(L"Win32_Process"), 0, 0, &p_process, 0);
		if(r != S_OK || !p_process) {
			temp_buf = str_const_list[sWin32Process];
			SLS.SetAddedMsgString(temp_buf);
			CALLEXCEPTV(SLERR_WMI_GETOBJECT);
		}
		/*
#ifdef _DEBUG
		{
			StrAssocArray method_list;
			GetMethodList(p_process, &method_list);
		}
#endif
		*/
		r = p_process->GetMethod(_bstr_t(L"Create"), 0, &p_incls, 0);
		if(r != S_OK || !p_incls) {
			temp_buf = str_const_list[sCreate];
			SLS.SetAddedMsgString(temp_buf);
			CALLEXCEPTV(SLERR_WMI_GETMETHOD);
		}
		r = p_incls->SpawnInstance(0, &p_inparams);
		THROW_V(p_inparams, SLERR_WMI_SPAWNINSTANCE);

		v_param.vt = VT_BSTR;
		(temp_buf = pCmdLine).CopyToOleStr(&(v_param.bstrVal = 0));
		p_inparams->Put(L"CommandLine", 0, &v_param, 0);
		VariantClear(&v_param);
		r = P_Svc->ExecMethod(_bstr_t(L"Win32_Process"), _bstr_t(L"Create"), 0, 0, p_inparams, &p_out, 0);
		if(r != S_OK || !p_out) {
			temp_buf  = str_const_list[sWin32Process];
			temp_buf2 = str_const_list[sCreate];
			SLS.SetAddedMsgString(temp_buf.CatCharN(':', 2).Cat(temp_buf2).CatParStr(pCmdLine));
			CALLEXCEPTV(SLERR_WMI_EXECMETHOD);
		}
		r = p_out->Get(L"ReturnValue", 0, &v_retval, 0, 0);
		r = (v_retval.lVal == 0);
		VariantClear(&v_retval);
		THROW_V(r, SLERR_WMI_EXECMETHODRETVAL);
	}
	CATCHZOK
	CALLPTRMEMB(p_out, Release());
	CALLPTRMEMB(p_inparams, Release());
	CALLPTRMEMB(p_incls, Release());
	CALLPTRMEMB(p_process, Release());
	return ok;
}

void SWmi::Release()
{
	if(P_Svc) {
		P_Svc->Release();
		P_Svc = 0;
	}
	if(P_Loc) {
		P_Loc->Release();
		P_Loc = 0;
	}
}

//
#if SLTEST_RUNNING // {

SLTEST_R(SWmi)
{
	int    ok = 1, r = 0;
	SFile out(MakeOutputFilePath("WmiCreateProcess.txt"), SFile::mWrite);
	SString server, user, pw, cmdline;
	uint   arg_no = 0;
	SString arg;
	/*
	;
	; Аргументы:
	;   0 - имя удаленного компьютера
	;   1 - имя входа на удаленный комп
	;   2 - пароль для входа на удаленный комп
	;   3 - командная строка для запуска
	;
	*/
	if(EnumArg(&arg_no, arg)) {
		server = arg;
		if(EnumArg(&arg_no, arg)) {
			user = arg;
			if(EnumArg(&arg_no, arg)) {
				pw = arg;
				if(EnumArg(&arg_no, arg)) {
					cmdline = arg;
					r = 1;
				}
			}
		}
	}
	if(!r) {
		out.WriteLine("Invalid arg list\n");
		CurrentStatus = 0;
	}
	else {
		SWmi wmi;
		THROW(SLTEST_CHECK_NZ(wmi.Connect(server, user, pw)));
		THROW(SLTEST_CHECK_NZ(wmi.Method_CreateProcess(cmdline)));
	}
	CATCH
		CurrentStatus = ok = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
