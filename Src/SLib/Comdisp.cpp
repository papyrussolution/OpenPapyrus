// COMDISP.CPP
// Copyright (c) V.Nasonov, A.Starodub 2003, 2004, 2006, 2007, 2008, 2010, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
// @codepage UTF-8
// Интерфейс IDispatch для работы с COM-приложениями (режим InProcServer) (only WIN32)
//
#include <slib-internal.h>
#pragma hdrstop

ComDispInterface::ComDispInterface() : P_ParamsAry(0), P_Disp(0), HRes(S_OK)
{
}

ComDispInterface::~ComDispInterface()
{
	CALLPTRMEMB(P_Disp, Release());
	ClearParams();
}

int ComDispInterface::Init(const char * pProgID, int inProcServer /*=1*/)
{
	ProgIdent = pProgID;
	int    ok = 1;
	uint   cls_ctx = inProcServer ? CLSCTX_INPROC_SERVER : CLSCTX_LOCAL_SERVER;
	CLSID  cls_id;
	wchar_t w_name[256];
	mbstowcs(w_name, pProgID, SIZEOFARRAY(w_name));
	HRes = CLSIDFromProgID(w_name, &cls_id);
	THROW(SUCCEEDED(HRes));
	HRes = CoCreateInstance(cls_id, NULL, cls_ctx, IID_IDispatch, (void **)&P_Disp);
	THROW(SUCCEEDED(HRes));
	DispIDAry.freeAll();
	CATCH
		ok = (SetErrCode(), 0);
	ENDCATCH
	return ok;
}

int ComDispInterface::Init(const wchar_t * pProgID, int inProcServer)
{
	ProgIdent.Z();

	int    ok = 1;
	CLSID  cls_id;
	HRes = CLSIDFromProgID(pProgID, &cls_id);
	THROW(SUCCEEDED(HRes));
	HRes = CoCreateInstance(cls_id, NULL, inProcServer ? CLSCTX_INPROC_SERVER : CLSCTX_LOCAL_SERVER, IID_IDispatch, (void **)&P_Disp);
	THROW(SUCCEEDED(HRes));
	DispIDAry.freeAll();
	CATCH
		ok = (SetErrCode(), 0);
	ENDCATCH
	return ok;
}

/*virtual*/int ComDispInterface::Init(IDispatch * pIDisp)
{
	ProgIdent.Z();
	DispIDAry.clear();
	P_Disp = pIDisp;
	return 1;
}

int ComDispInterface::AssignIDByName(const char * pName, long nameID)
{
	int    ok = 1;
	size_t wname_len = 0;
	DispIDEntry dispid_entry;
	OLECHAR * p_wname = 0;
	THROW_S(P_Disp, SLERR_INVPARAM);
	wname_len = mbstowcs(NULL, pName, MAX_PATH) + 1;
	THROW(p_wname = new OLECHAR[wname_len]);
	mbstowcs(p_wname, pName, MAX_PATH);
	THROW(SUCCEEDED(HRes = P_Disp->GetIDsOfNames(IID_NULL, &p_wname, 1, LOCALE_USER_DEFAULT, &dispid_entry.DispID)));
	dispid_entry.ID = nameID;
	STRNSCPY(dispid_entry.Name, pName);
	THROW(DispIDAry.insert(&dispid_entry));
	CATCH
		ok = SUCCEEDED(HRes) ? 0 : (SetErrCode(), -1);
		/*{
			SString msg_buf, err_msg;
			PPLoadText(PPTXT_DISPIFASSGNFAULT, msg_buf);
			msg_buf.Space().Cat(ProgIdent).Cat("::").Cat(pName);
			PPGetLastErrorMessage(1, err_msg);
			if(err_msg.NotEmptyS())
				msg_buf.CatDiv(':', 2).Cat(err_msg);
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		}*/
	ENDCATCH
	delete [] p_wname;
	return ok;
}

const DispIDEntry * FASTCALL ComDispInterface::GetDispIDEntry(long entryId) const
{
	long   id = entryId;
	uint   pos = 0;
	return DispIDAry.lsearch(&id, &pos, CMPF_LONG) ? &DispIDAry.at(pos) : 0;
}

SString & ComDispInterface::GetNameByID(int id, SString & rName) const
{
	const  DispIDEntry * p_die = GetDispIDEntry(id);
	if(p_die)
		rName = p_die->Name;
	else
		rName.Z();
	return rName;
}

int ComDispInterface::_GetProperty(int propertyID, VARIANTARG * pVarArg, int sendParams)
{
	int    ok = 1;
	const  DispIDEntry * p_die = 0;
	VARIANTARG var_arg;
	VARTYPE    vt;
	DISPPARAMS null_params = {NULL, NULL, 0, 0};
	DISPPARAMS params = { P_ParamsAry ? static_cast<VARIANTARG *>(P_ParamsAry->dataPtr()) : NULL, NULL, SVectorBase::GetCount(P_ParamsAry), 0 };
	THROW_S(P_Disp, SLERR_INVPARAM);
	THROW(p_die = GetDispIDEntry(propertyID));
	VariantInit(&var_arg);
	THROW(SUCCEEDED(HRes = P_Disp->Invoke(p_die->DispID, IID_NULL, LOCALE_USER_DEFAULT,
		DISPATCH_PROPERTYGET,	 sendParams ? &params : &null_params, &var_arg, NULL, NULL)));
	vt = pVarArg->vt;
	if(var_arg.vt == VT_BOOL && var_arg.boolVal) {
		var_arg.vt     = VT_INT;
		var_arg.intVal = TRUE;
	}
	if(pVarArg && (vt == var_arg.vt && vt == VT_DISPATCH))
		pVarArg->pdispVal = var_arg.pdispVal;
	else {
		THROW(SUCCEEDED(HRes = VariantChangeTypeEx(pVarArg, &var_arg, LOCALE_USER_DEFAULT, 0, vt)));
	}
	CATCH
		ok = SUCCEEDED(HRes) ? 0 : (SetErrCode(), -1);
	ENDCATCH
	if(ok <= 0 || vt != VT_DISPATCH)
		VariantClear(&var_arg);
	if(sendParams)
		ClearParams();
	return ok;
}

int ComDispInterface::GetProperty(int propertyID, bool * pBuf)
{
	int   ok = 1;
	VARIANTARG var_arg;
	VariantInit(&var_arg);
	var_arg.vt = VT_BOOL;
	if((ok = _GetProperty(propertyID, &var_arg)) > 0) {
		ASSIGN_PTR(pBuf, LOGIC(var_arg.boolVal));
	}
	else
		ASSIGN_PTR(pBuf, 0);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::GetProperty(int propertyID, int * pBuf)
{
	VARIANTARG   var_arg;
	VariantInit(&var_arg);
	var_arg.vt = VT_INT;
	int    ok = _GetProperty(propertyID, &var_arg);
	ASSIGN_PTR(pBuf, (ok > 0) ? var_arg.intVal : 0);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::GetProperty(int propertyID, long * pBuf)
{
	VARIANTARG   var_arg;
	VariantInit(&var_arg);
	var_arg.vt = VT_I4;
	int    ok = _GetProperty(propertyID, &var_arg);
	ASSIGN_PTR(pBuf, (ok > 0) ? var_arg.lVal : 0);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::GetProperty(int propertyID, double * pBuf)
{
	VARIANTARG   var_arg;
	VariantInit(&var_arg);
	var_arg.vt = VT_R8;
	int    ok = _GetProperty(propertyID, &var_arg);
	ASSIGN_PTR(pBuf, (ok > 0) ? var_arg.dblVal : 0);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::GetProperty(int propertyID, char * pBuf, size_t bufLen)
{
	int    ok = 1;
	OLECHAR    wstr[MAX_PATH];
	VARIANTARG var_arg;
	VariantInit(&var_arg);
	if(pBuf && bufLen > 0) {
		var_arg.vt = VT_BSTR;
		wstr[0] = 0;
		THROW_S(var_arg.bstrVal = SysAllocString(wstr), SLERR_NOMEM);
		if((ok = _GetProperty(propertyID, &var_arg)) > 0) {
			WideCharToMultiByte(1251, 0, var_arg.bstrVal, -1, pBuf, static_cast<int>(bufLen), NULL, NULL);
		}
		else
			memzero(pBuf, bufLen);
	}
	CATCHZOK
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::GetProperty(int propertyID, ComDispInterface * pDisp)
{
	VARIANTARG var_arg;
	THROW_S(pDisp, SLERR_INVPARAM);
	VariantInit(&var_arg);
	var_arg.vt = VT_DISPATCH;
	int    ok = _GetProperty(propertyID, &var_arg, 1);
	if(ok > 0) {
		THROW(pDisp->Init(var_arg.pdispVal));
	}
	CATCHZOK
	return ok;
}

int ComDispInterface::SetPropertyByParams(int propertyID)
{
	int    ok = 1;
	const  DispIDEntry * p_die = 0;
	VARIANTARG var_arg;
	DISPPARAMS params = { P_ParamsAry ? static_cast<VARIANTARG *>(P_ParamsAry->dataPtr()) : NULL, NULL, SVectorBase::GetCount(P_ParamsAry), 0 };
	VariantInit(&var_arg);
	THROW_S(P_Disp, SLERR_INVPARAM);
	THROW(p_die = GetDispIDEntry(propertyID));
	THROW(SUCCEEDED(HRes = P_Disp->Invoke(p_die->DispID, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &params, &var_arg, NULL, NULL)));
	CATCH
		ok = SUCCEEDED(HRes) ? 0 : (SetErrCode(), -1);
	ENDCATCH
	VariantClear(&var_arg);
	ClearParams();
	return ok;
}

int ComDispInterface::_SetProperty(int propertyID, VARIANTARG * pVarArg)
{
	int    ok = 1;
	const  DispIDEntry * p_die = 0;
	VARIANTARG var_arg;
	DISPID     dispid_put  = DISPID_PROPERTYPUT;
	DISPPARAMS null_params = {NULL, NULL, 0, 0};
	DISPPARAMS dispparams  = {&var_arg, &dispid_put, 1, 1};
	THROW_S(P_Disp, SLERR_INVPARAM);
	THROW(p_die = GetDispIDEntry(propertyID));
	VariantInit(&var_arg);
	THROW(SUCCEEDED(HRes = P_Disp->Invoke(p_die->DispID, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &null_params, &var_arg, NULL, NULL)));
	if(var_arg.vt == VT_NULL || var_arg.vt == VT_EMPTY) {
		THROW(SUCCEEDED(HRes = VariantCopy(&var_arg, pVarArg)));
	}
	else {
		VARTYPE vt = var_arg.vt;
		THROW(SUCCEEDED(HRes = VariantChangeTypeEx(&var_arg, pVarArg, LOCALE_USER_DEFAULT, 0, vt)));
	}
	THROW(SUCCEEDED(HRes = P_Disp->Invoke(p_die->DispID, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL)));
	CATCH
		ok = SUCCEEDED(HRes) ? 0 : (SetErrCode(), -1);
	ENDCATCH
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::_SetPropertyW(int propertyID, VARIANTARG * pVarArg)
{
	int    ok = 1;
	const  DispIDEntry * p_die = 0;
	VARIANTARG var_arg;
	DISPID     dispid_put  = DISPID_PROPERTYPUT;
	DISPPARAMS null_params = {NULL, NULL, 0, 0};
	DISPPARAMS dispparams  = {&var_arg, &dispid_put, 1, 1};
	THROW_S(P_Disp, SLERR_INVPARAM);
	THROW(p_die = GetDispIDEntry(propertyID));
	VariantInit(&var_arg);
	if(var_arg.vt == VT_NULL || var_arg.vt == VT_EMPTY) {
		THROW(SUCCEEDED(HRes = VariantCopy(&var_arg, pVarArg)));
	}
	else {
		VARTYPE vt = var_arg.vt;
		THROW(SUCCEEDED(HRes = VariantChangeTypeEx(&var_arg, pVarArg, LOCALE_USER_DEFAULT, 0, vt)));
	}
	THROW(SUCCEEDED(HRes = P_Disp->Invoke(p_die->DispID, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL)));
	CATCH
		ok = SUCCEEDED(HRes) ? 0 : (SetErrCode(), -1);
	ENDCATCH
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::SetProperty(int propertyID, int iVal, int writeOnly /*=0*/)
{
	int   ok = 1;
	VARIANTARG var_arg;
	VariantInit(&var_arg);
	var_arg.vt = VT_INT;
	var_arg.intVal = iVal;
	ok = writeOnly ? _SetPropertyW(propertyID, &var_arg) : _SetProperty(propertyID, &var_arg);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::SetProperty(int propertyID, long lVal, int writeOnly /*=0*/)
{
	int   ok = 1;
	VARIANTARG   var_arg;
	VariantInit(&var_arg);
	var_arg.vt   = VT_I4;
	var_arg.lVal = lVal;
	ok = writeOnly ? _SetPropertyW(propertyID, &var_arg) : _SetProperty(propertyID, &var_arg);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::SetProperty(int propertyID, double dVal, int writeOnly /*=0*/)
{
	int   ok = 1;
	VARIANTARG   var_arg;
	VariantInit(&var_arg);
	var_arg.vt     = VT_R8;
	var_arg.dblVal = dVal;
	ok = writeOnly ? _SetPropertyW(propertyID, &var_arg) : _SetProperty(propertyID, &var_arg);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::SetProperty(int propertyID, LDATE dtVal, int writeOnly /*=0*/)
{
	int   ok = 1;
	VARIANTARG   var_arg;
	VariantInit(&var_arg);
	var_arg.vt     = VT_DATE;
	var_arg.date = dtVal.GetOleDate();
	ok = writeOnly ? _SetPropertyW(propertyID, &var_arg) : _SetProperty(propertyID, &var_arg);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::SetProperty(int propertyID, bool bVal, int writeOnly /*=0*/)
{
	VARIANTARG   var_arg;
	VariantInit(&var_arg);
	var_arg.vt     = VT_BOOL;
	var_arg.boolVal = bVal;
	int    ok = writeOnly ? _SetPropertyW(propertyID, &var_arg) : _SetProperty(propertyID, &var_arg);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::SetProperty(int propertyID, const char * pStrVal, int writeOnly /*=0*/)
{
	int   ok = 1;
	VARIANTARG var_arg;
	OLECHAR    wstr[512];
	VariantInit(&var_arg);
	if(pStrVal) {
		MultiByteToWideChar(1251, MB_PRECOMPOSED, pStrVal, -1, wstr, SIZEOFARRAY(wstr) - 1);
		var_arg.vt = VT_BSTR;
		THROW_S(var_arg.bstrVal = SysAllocString(wstr), SLERR_NOMEM);
		ok = writeOnly ? _SetPropertyW(propertyID, &var_arg) : _SetProperty(propertyID, &var_arg);
	}
	CATCHZOK
	VariantClear(&var_arg);
	return ok;
}

void ComDispInterface::ClearParams()
{
	if(P_ParamsAry) {
		for(uint i = 0; i < P_ParamsAry->getCount(); i++)
			VariantClear(static_cast<VARIANTARG *>(P_ParamsAry->at(i)));
		ZDELETE(P_ParamsAry);
	}
}

int ComDispInterface::_SetParam(VARIANTARG * pVarArg)
{
	int   ok = 1;
	VARIANTARG  var_arg;
	if(!P_ParamsAry)
		THROW(P_ParamsAry = new SArray(sizeof(VARIANTARG)));
	VariantInit(&var_arg);
	THROW(P_ParamsAry->atInsert(0, &var_arg));
	THROW(SUCCEEDED(HRes = VariantCopy(static_cast<VARIANTARG *>(P_ParamsAry->at(0)), pVarArg)));
	CATCH
		ok = SUCCEEDED(HRes) ? 0 : (SetErrCode(), -1);
	ENDCATCH
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::SetParam(int iVal)
{
	VARIANTARG   var_arg;
	VariantInit(&var_arg);
	var_arg.vt     = VT_INT;
	var_arg.intVal = iVal;
	int    ok = _SetParam(&var_arg);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::SetParam(long lVal)
{
	VARIANTARG   var_arg;
	VariantInit(&var_arg);
	var_arg.vt   = VT_I4;
	var_arg.lVal = lVal;
	int    ok = _SetParam(&var_arg);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::SetParam(double dVal)
{
	VARIANTARG   var_arg;
	VariantInit(&var_arg);
	var_arg.vt     = VT_R8;
	var_arg.dblVal = dVal;
	int    ok = _SetParam(&var_arg);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::SetParam(const char * pStrVal, int codepage/*=1251*/)
{
	int   ok = 1;
	VARIANTARG var_arg;
	OLECHAR    wstr[2048];
	VariantInit(&var_arg);
	if(pStrVal) {
		MultiByteToWideChar(codepage, (codepage == 1251) ? MB_PRECOMPOSED : 0, pStrVal, -1, wstr, SIZEOFARRAY(wstr) - 1);
		var_arg.vt = VT_BSTR;
		THROW_S(var_arg.bstrVal = SysAllocString(wstr), SLERR_NOMEM);
		ok = _SetParam(&var_arg);
	}
	CATCHZOK
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::SetParam(ComDispInterface * pParam)
{
	VARIANTARG   var_arg;
	VariantInit(&var_arg);
	if(pParam) {
		var_arg.vt       = VT_DISPATCH;
		var_arg.pdispVal = pParam->GetDisp();
	}
	else {
		var_arg.vt = VT_ERROR;
		var_arg.scode = DISP_E_PARAMNOTFOUND;
	}
	int    ok = _SetParam(&var_arg);
	VariantClear(&var_arg);
	return ok;
}

int ComDispInterface::CallMethod(int methodID, VARIANTARG * pVarArg)
{
	int    ok = 1;
	const  int rcv_res = BIN(pVarArg);
	const  DispIDEntry * p_die = 0;
	VARTYPE    vt;
	VARIANTARG var_arg;
	DISPPARAMS params;
	params.rgvarg = P_ParamsAry ? static_cast<VARIANTARG *>(P_ParamsAry->dataPtr()) : 0;
	params.rgdispidNamedArgs = 0;
	params.cArgs = SVectorBase::GetCount(P_ParamsAry);
	params.cNamedArgs = 0;
	if(rcv_res)
		VariantInit(&var_arg);
	THROW_S(P_Disp, SLERR_INVPARAM);
	THROW(p_die = GetDispIDEntry(methodID));
	THROW(SUCCEEDED(HRes = P_Disp->Invoke(p_die->DispID, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, rcv_res ? &var_arg : NULL, NULL, NULL)));
	if(rcv_res) {
		vt = pVarArg->vt;
		if(var_arg.vt == VT_BOOL && var_arg.boolVal) {
			var_arg.vt     = VT_INT;
			var_arg.intVal = TRUE;
		}
		if(vt == var_arg.vt && vt == VT_DISPATCH)
			pVarArg->pdispVal = var_arg.pdispVal;
		else {
			THROW(SUCCEEDED(HRes = VariantChangeTypeEx(pVarArg, &var_arg, LOCALE_USER_DEFAULT, 0, vt)));
		}
	}
	CATCH
		ok = SUCCEEDED(HRes) ? 0 : (SetErrCode(), -1);
		/*if(CConfig.Flags & CCFLG_DEBUG) {
			//PPTXT_LOG_DISPINVOKEFAULT         "Ошибка вызова Dispatch-метода '@zstr': @zstr"
			SString msg_buf;
			PPFormatT(PPTXT_LOG_DISPINVOKEFAULT, &msg_buf, (p_die ? p_die->Name : ""), "");
			PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
		}*/
	ENDCATCH
	if(rcv_res && (ok <= 0 || var_arg.vt != VT_DISPATCH))
		VariantClear(&var_arg);
	ClearParams();
	return ok;
}

int  ComDispInterface::CallMethod(int methodID, ComDispInterface * pDisp)
{
	int    ok = 1;
	VARIANTARG var_arg;
	THROW_S(pDisp, SLERR_INVPARAM);
	VariantInit(&var_arg);
	var_arg.vt = VT_DISPATCH;
	ok = CallMethod(methodID, &var_arg);
	if(ok > 0) {
		THROW(pDisp->Init(var_arg.pdispVal));
	}
	CATCHZOK
	return ok;
}

void ComDispInterface::SetErrCode()
{
	SString err_msg;
	SString sys_err_buf;
	{
		HRESULT hr = HRes;
    	if(HRESULT_FACILITY(hr) == FACILITY_WINDOWS)
    	    hr = HRESULT_CODE(hr);
    	//char * p_err_msg = 0;
    	//if(::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&p_err_msg, 0, 0) != 0) {
    	    //sys_err_buf = p_err_msg;
    	    //LocalFree(p_err_msg);
		//}
		SSystem::SFormatMessage(hr, sys_err_buf); // @v10.3.11
	}
	if(sys_err_buf.IsEmpty()) {
		//PPLoadTextWin(PPTXT_RETCODE, temp_buf);
		sys_err_buf.Cat("retcode").Space().CatHex(HRes);
	}
	err_msg.CatQStr(ProgIdent).Space().Cat(sys_err_buf.Transf(CTRANSF_OUTER_TO_INNER));
	//PPSetError(PPERR_COMINTERFACE, err_msg.ToOem());
	//PPSetError(PPERR_DISPIFCCLI, err_msg);
	SLS.SetError(SLERR_DISPIFCCLI, err_msg);
}
//
// Excel disp interface
//
//#define COM_WORD  "Word.Application"
//
// ComExcelFont
//
ComExcelFont::ComExcelFont() : ComDispInterface()
{
}

ComExcelFont::~ComExcelFont()
{
}

int ComExcelFont::Init(IDispatch * pDisp)
{
	int    ok = 1;
	ComDispInterface::Init(pDisp);
	THROW(ASSIGN_ID_BY_NAME(this, Bold) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Color) > 0);
	CATCHZOK
	return ok;
}

int ComExcelFont::SetBold(int bold) { return SetProperty(Bold, bold); }
int ComExcelFont::SetColor(long color) { return SetProperty(Color, color); }
//
// ComExcelShapes
//
ComExcelShapes::ComExcelShapes() : ComDispInterface()
{
}

ComExcelShapes::~ComExcelShapes()
{
}

int ComExcelShapes::Init(IDispatch * pIDisp)
{
	int    ok = 1;
	ComDispInterface::Init(pIDisp);
	THROW(ASSIGN_ID_BY_NAME(this, AddPicture) > 0);
	CATCHZOK
	return ok;
}

int ComExcelShapes::PutPicture(const char * pPath, RECT * pRect)
{
	int    ok = 1;
	ComDispInterface * p_shape = new ComDispInterface;
	THROW_S(pRect, SLERR_INVPARAM);
	THROW(SetParam(pPath));
	THROW(SetParam(1));
	THROW(SetParam(0));
	THROW(SetParam(pRect->left));
	THROW(SetParam(pRect->top));
	THROW(SetParam(pRect->right));
	THROW(SetParam(pRect->bottom));
	THROW(CallMethod(AddPicture, static_cast<ComDispInterface *>(p_shape)) > 0);
	CATCHZOK
	ZDELETE(p_shape);
	return ok;
}
//
// ComExcelInterior
//
ComExcelInterior::ComExcelInterior() : ComDispInterface()
{
}

ComExcelInterior::~ComExcelInterior()
{
}

int ComExcelInterior::Init(IDispatch * pIDisp)
{
	int    ok = 1;
	THROW(ComDispInterface::Init(pIDisp));
	THROW(ASSIGN_ID_BY_NAME(this, Color) > 0);
	CATCHZOK
	return ok;
}

int ComExcelInterior::SetColor(long color)
{
	return SetProperty(Color, color);
}
//
// ComExcelRange
//
ComExcelRange::ComExcelRange() : ComDispInterface()
{
}

ComExcelRange::~ComExcelRange()
{
}

int ComExcelRange::Init(IDispatch * pIDisp)
{
	int    ok = 1;
	THROW(ComDispInterface::Init(pIDisp));
	THROW(ASSIGN_ID_BY_NAME(this, Item) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, NumberFormat) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Value) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Font) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, ColumnWidth) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, RowHeight) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Clear) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Columns) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Interior) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Merge) > 0);
	CATCHZOK
	return ok;
}

int ComExcelRange::SetValue(const char * pValue) { return SetProperty(Value, pValue); }
int ComExcelRange::SetValue(double value) { return SetProperty(Value, value); }
int ComExcelRange::SetFormat(const char * pFormat) { return SetProperty(NumberFormat, pFormat); }

int ComExcelRange::GetValue(SString & rValue)
{
	char   buf[2048];
	int    ok = GetProperty(Value, buf, sizeof(buf));
	if(ok > 0)
		rValue = buf;
	return ok;
}

int ComExcelRange::GetFormat(SString & rFormat)
{
	int    ok = 0;
	char   fmt[128];
	if(GetProperty(NumberFormat, fmt, sizeof(fmt)) > 0) {
		rFormat.CopyFrom(fmt);
		ok = 1;
	}
	return ok;
}

ComExcelFont * ComExcelRange::GetFont()
{
	ComExcelFont * p_font = new ComExcelFont;
	if(GetProperty(Font, p_font) <= 0)
		ZDELETE(p_font);
	return p_font;
}

ComExcelInterior * ComExcelRange::GetInterior()
{
	ComExcelInterior * p_intr = new ComExcelInterior;
	if(GetProperty(Interior, p_intr) <= 0)
		ZDELETE(p_intr);
	return p_intr;
}

int ComExcelRange::SetBold(int bold)
{
	ComExcelFont * p_font = GetFont();
	int    ok = p_font ? p_font->SetBold(bold) : 0;
	ZDELETE(p_font);
	return ok;
}

int ComExcelRange::SetColor(long color)
{
	ComExcelFont * p_font = GetFont();
	int    ok = p_font ? p_font->SetColor(color) : 0;
	ZDELETE(p_font);
	return ok;
}

int ComExcelRange::SetBgColor(long color)
{
	ComExcelInterior * p_intr = GetInterior();
	int    ok = p_intr ? p_intr->SetColor(color) : 0;
	ZDELETE(p_intr);
	return ok;
}

int ComExcelRange::SetWidth(long width) { return BIN(SetParam(width) > 0 && SetProperty(ColumnWidth, width) > 0); }
int ComExcelRange::SetHeight(long height) { return BIN(SetParam(height) > 0 && SetProperty(RowHeight, height) > 0); }
int ComExcelRange::DoClear() { return CallMethod(Clear); }
int ComExcelRange::DoMerge() { return CallMethod(Merge); }

ComExcelRange * ComExcelRange::_Columns()
{
	ComExcelRange * p_range = new ComExcelRange;
	if(GetProperty(Columns, p_range) <= 0)
		ZDELETE(p_range);
	return p_range;
}
//
// ComExcelWorksheet
//
ComExcelWorksheet::ComExcelWorksheet() : ComDispInterface()
{
}

ComExcelWorksheet::~ComExcelWorksheet()
{
}

int ComExcelWorksheet::Init(IDispatch * pIDisp)
{
	int    ok = 1;
	THROW(ComDispInterface::Init(pIDisp));
	THROW(ASSIGN_ID_BY_NAME(this, Activate) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Name) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Cells) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Delete) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Columns) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Rows) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Range) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Shapes) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, PrintOut) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, PrintPreview) > 0);
	CATCHZOK
	return ok;
}

int ComExcelWorksheet::_Activate() { return CallMethod(Activate); }
int ComExcelWorksheet::Print() { return CallMethod(PrintOut); }
int ComExcelWorksheet::Preview() { return CallMethod(PrintPreview); }
int ComExcelWorksheet::SetName(const char * pName) { return pName ? SetProperty(Name, pName) : 1; }

int ComExcelWorksheet::GetName(SString & rName)
{
	char   buf[2048];
	int    ok = GetProperty(Name, buf, sizeof(buf));
	if(ok > 0)
		rName = buf;
	return ok;
}

int ComExcelWorksheet::AddColumn(long before, long after, const char * pColumnName)
{
	return -1;
}

int ComExcelWorksheet::DelColumn(long pos)
{
	return -1;
}

ComExcelRange * ComExcelWorksheet::Cell(long row, long col)
{
	ComExcelRange * p_range = new ComExcelRange;
	THROW(SetParam(row) > 0);
	THROW(SetParam(col) > 0);
	THROW(GetProperty(Cells, static_cast<ComDispInterface *>(p_range)) > 0);
	CATCH
		ZDELETE(p_range);
	ENDCATCH
	return p_range;
}

int GetExcelCellCoordA1(long row, long col, SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
	if(col > 0 && row > 0) {
		if(col <= 26)
			rBuf.CatChar('A' + (char)(col-1));
		else {
			if((col / 26) < 26)
				rBuf.CatChar('A' + (char)(col / 26)).CatChar('A' + (char)(col % 26));
			else
				ok = 0;
		}
		rBuf.Cat(row);
	}
	else
		ok = 0;
	return ok;
}

ComExcelRange * ComExcelWorksheet::GetRange(long luRow, long luCol, long rbRow, long rbCol)
{
	ComExcelRange * p_range = new ComExcelRange;
	SString c1, c2;
	THROW(GetExcelCellCoordA1(luRow, luCol, c1));
	THROW(GetExcelCellCoordA1(rbRow, rbCol, c2));
	THROW(SetParam(c1) > 0);
	THROW(SetParam(c2) > 0);
	THROW(GetProperty(Range, static_cast<ComDispInterface *>(p_range)) > 0);
	CATCH
		ZDELETE(p_range);
	ENDCATCH
	return p_range;
}

int ComExcelWorksheet::PutPicture(const char * pPath, RECT * pRect)
{
	int    ok = 0;
	ComExcelShapes * p_shapes = GetShapes();
	if(p_shapes) {
		ok = p_shapes->PutPicture(pPath, pRect);
		ZDELETE(p_shapes);
	}
	return ok;
}

int ComExcelWorksheet::SetValue(long row, long col, const char * pValue)
{
	ComExcelRange * p_range = Cell(row, col);
	int    ok = p_range ? p_range->SetValue(pValue) : 0;
	ZDELETE(p_range);
	return ok;
}

int ComExcelWorksheet::SetValue(long row, long col, double value)
{
	ComExcelRange * p_range = Cell(row, col);
	int    ok = p_range ? p_range->SetValue(value) : 0;
	ZDELETE(p_range);
	return ok;
}

int ComExcelWorksheet::GetValue(long row, long col, SString & rValue)
{
	ComExcelRange * p_range = Cell(row, col);
	int    ok = p_range ? p_range->GetValue(rValue) : 0;
	ZDELETE(p_range);
	return ok;
}

void ComExcelWorksheet::_Clear(long col1, long colN)
{
	for(long i = col1; i < colN; i++) {
		ComExcelRange * p_range = GetColumn(i);
		if(p_range) {
			p_range->DoClear();
			ZDELETE(p_range);
		}
	}
}

void ComExcelWorksheet::_Clear(long row1, long col1, long row2, long col2)
{
}

ComExcelRange * ComExcelWorksheet::_Select(long row1, long col1, long row2, long col2)
{
	return 0;
}

ComExcelRange * ComExcelWorksheet::GetColumn(long pos)
{
	ComExcelRange * p_range = new ComExcelRange;
	THROW(SetParam(pos) > 0);
	THROW(GetProperty(Columns, p_range) > 0);
	CATCH
		ZDELETE(p_range);
	ENDCATCH
	return p_range;
}

ComExcelRange * ComExcelWorksheet::GetRow(long pos)
{
	ComExcelRange * p_range = new ComExcelRange;
	THROW(SetParam(pos) > 0);
	THROW(GetProperty(Rows, p_range) > 0);
	CATCH
		ZDELETE(p_range);
	ENDCATCH
	return p_range;
}

int ComExcelWorksheet::SetColumnWidth(long pos, long width)
{
	ComExcelRange * p_range = GetColumn(pos);
	int    ok = p_range ? p_range->SetWidth(width) : 0;
	ZDELETE(p_range);
	return ok;
}

int ComExcelWorksheet::_Delete()
{
	return CallMethod(Delete);
}

int ComExcelWorksheet::SetBold(long row, long col, int bold)
{
	ComExcelRange * p_range = Cell(row, col);
	int    ok = p_range ? p_range->SetBold(bold) : 0;
	ZDELETE(p_range);
	return ok;
}

int ComExcelWorksheet::SetColor(long row, long col, COLORREF color)
{
	ComExcelRange * p_range = Cell(row, col);
	int    ok = p_range ? p_range->SetColor((long)color) : 0;
	ZDELETE(p_range);
	return ok;
}

int ComExcelWorksheet::SetBgColor(long row, long col, COLORREF color)
{
	ComExcelRange * p_range = Cell(row, col);
	int    ok = p_range ? p_range->SetBgColor((long)color) : 0;
	ZDELETE(p_range);
	return ok;
}

int ComExcelWorksheet::SetColumnFormat(long pos, const char * pFormat)
{
	ComExcelRange * p_range = GetColumn(pos);
	int    ok = p_range ? p_range->SetFormat(pFormat) : 0;
	ZDELETE(p_range);
	return ok;
}

int ComExcelWorksheet::SetCellFormat(long row, long col, const char * pFormat)
{
	ComExcelRange * p_range = Cell(row, col);
	int    ok = p_range ? p_range->SetFormat(pFormat) : 0;
	ZDELETE(p_range);
	return ok;
}

int ComExcelWorksheet::GetCellFormat(long row, long col, SString & rFormat)
{
	ComExcelRange * p_range = Cell(row, col);
	int    ok = p_range ? p_range->GetFormat(rFormat) : 0;
	ZDELETE(p_range);
	return ok;
}

ComExcelShapes * ComExcelWorksheet::GetShapes()
{
	ComExcelShapes * p_shapes = new ComExcelShapes;
	if(GetProperty(Shapes, static_cast<ComDispInterface *>(p_shapes)) <= 0)
		ZDELETE(p_shapes);
	return p_shapes;
}
//
// ComExcelWorksheets
//
ComExcelWorksheets::ComExcelWorksheets() : ComDispInterface()
{
}

ComExcelWorksheets::~ComExcelWorksheets()
{
}

int ComExcelWorksheets::Init(IDispatch * pDisp)
{
	int    ok = 1;
	THROW(ComDispInterface::Init(pDisp));
	THROW(ASSIGN_ID_BY_NAME(this, Item) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Add) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Move) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Count) > 0);
	CATCHZOK
	return ok;
}

ComExcelWorksheet * ComExcelWorksheets::_Add(long before, long after, const char * pName)
{
	ComExcelWorksheet * p_sheet = new ComExcelWorksheet;
	ComExcelWorksheet * p_before_sheet = Get(before);
	ComExcelWorksheet * p_after_sheet = Get(after);
	THROW(SetParam(p_before_sheet) > 0);
	THROW(SetParam(p_after_sheet) > 0);
	THROW(CallMethod(Add, static_cast<ComDispInterface *>(p_sheet)) > 0);
	THROW(p_sheet->SetName(pName) > 0);
	CATCH
		ZDELETE(p_sheet);
	ENDCATCH
	ZDELETE(p_before_sheet);
	ZDELETE(p_after_sheet);
	return p_sheet;
}

int ComExcelWorksheets::Delete(long pos)
{
	int    ok = 1;
	ComExcelWorksheet * p_sheet = Get(pos);
	if(p_sheet) {
		THROW(p_sheet->_Activate() > 0);
		THROW(p_sheet->_Delete() > 0);
	}
	CATCHZOK
	ZDELETE(p_sheet);
	return ok;
}

int ComExcelWorksheets::_Move(long before, long after)
{
	int    ok = 1;
	ComExcelWorksheet * p_before_sheet = Get(before);
	ComExcelWorksheet * p_after_sheet = Get(after);
	THROW(SetParam(p_before_sheet));
	THROW(SetParam(p_after_sheet));
	THROW(CallMethod(Move));
	CATCHZOK
	ZDELETE(p_before_sheet);
	ZDELETE(p_after_sheet);
	return ok;
}

ComExcelWorksheet * ComExcelWorksheets::Enum(long * pPos)
{
	long   pos = DEREFPTRORZ(pPos);
	ComExcelWorksheet * p_sheet = 0;
	if(pos > 0) {
		p_sheet = Get(pos);
		if(p_sheet)
			(*pPos)++;
	}
	return p_sheet;
}

ComExcelWorksheet * ComExcelWorksheets::Get(long pos)
{
	ComExcelWorksheet * p_sheet = new ComExcelWorksheet;
	THROW(checkirange(static_cast<int>(pos), 1, GetCount()));
	THROW(SetParam(pos) > 0);
	THROW(GetProperty(Item, static_cast<ComDispInterface *>(p_sheet)) > 0);
	CATCH
		ZDELETE(p_sheet);
	ENDCATCH
	return p_sheet;
}

int ComExcelWorksheets::GetCount()
{
	long   count = 0;
	GetProperty(Count, &count);
	return count;
}

int ComExcelWorksheets::Activate(long pos)
{
	int    ok = 0;
	ComExcelWorksheet * p_sheet = Get(pos);
	if(p_sheet) {
		ok = BIN(p_sheet->_Activate() > 0);
		ZDELETE(p_sheet);
	}
	return ok;
}
//
// ComExcelWorkbook
//
ComExcelWorkbook::ComExcelWorkbook() : ComDispInterface()
{
}

ComExcelWorkbook::~ComExcelWorkbook()
{
}

int ComExcelWorkbook::Init(IDispatch * pIDisp)
{
	int    ok = 1;
	THROW(ComDispInterface::Init(pIDisp) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, WorkSheets) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Close) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Save) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, SaveAs) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, ActiveSheet) > 0);
	CATCHZOK
	return ok;
}

ComExcelWorksheets * ComExcelWorkbook::Get()
{
	ComExcelWorksheets * p_sheets = new ComExcelWorksheets;
	if(GetProperty(WorkSheets, static_cast<ComDispInterface *>(p_sheets)) <= 0)
		ZDELETE(p_sheets);
	return p_sheets;
}

int ComExcelWorkbook::_Close()
{
	return CallMethod(Close);
}

int ComExcelWorkbook::_SaveAs(const char * pPath)
{
	return BIN(SetParam(pPath) > 0 && CallMethod(SaveAs) > 0);
}

int ComExcelWorkbook::_Save()
{
	return CallMethod(Save);
}

int ComExcelWorkbook::_Activate()
{
	return CallMethod(Activate);
}

ComExcelWorksheet * ComExcelWorkbook::_ActiveSheet()
{
	ComExcelWorksheet * p_sheet = new ComExcelWorksheet;
	if(GetProperty(ActiveSheet, static_cast<ComDispInterface *>(p_sheet)) <= 0)
		ZDELETE(p_sheet);
	return p_sheet;
}

ComExcelWorksheet * ComExcelWorkbook::GetWorksheet(long pos)
{
	ComExcelWorksheets * p_sheets = Get();
	ComExcelWorksheet * p_sheet = p_sheets ? p_sheets->Get(pos) : 0;
	ZDELETE(p_sheets);
	return p_sheet;
}

ComExcelWorksheet * ComExcelWorkbook::AddWorksheet(long before, long after, const char * pName)
{
	ComExcelWorksheets * p_sheets = Get();
	ComExcelWorksheet  * p_sheet = p_sheets ? p_sheets->_Add(before, after, pName) : 0;
	ZDELETE(p_sheets);
	return p_sheet;
}
//
// ComExcelWorkbooks
//
ComExcelWorkbooks::ComExcelWorkbooks() : ComDispInterface()
{
}

ComExcelWorkbooks::~ComExcelWorkbooks()
{
}

int ComExcelWorkbooks::Init(IDispatch * pIDisp)
{
	int    ok = 1;
	THROW(ComDispInterface::Init(pIDisp) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Item) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Add) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Count) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Open) > 0);
	CATCHZOK
	return ok;
}

ComExcelWorkbook * ComExcelWorkbooks::_Add()
{
	ComExcelWorkbook * p_wkbook = new ComExcelWorkbook;
	if(CallMethod(Add, static_cast<ComDispInterface *>(p_wkbook)) <= 0)
		ZDELETE(p_wkbook);
	return p_wkbook;
}

ComExcelWorkbook * ComExcelWorkbooks::Enum(long * pPos)
{
	long   pos = DEREFPTRORZ(pPos);
	ComExcelWorkbook * p_wkbook = 0;
	if(pos > 0) {
		p_wkbook = Get(pos);
		if(p_wkbook)
			(*pPos)++;
	}
	return p_wkbook;
}

ComExcelWorkbook * ComExcelWorkbooks::Get(long pos)
{
	ComExcelWorkbook * p_wkbook = new ComExcelWorkbook;
	THROW(checkirange(pos, 1L, GetCount()));
	THROW(SetParam(pos) > 0);
	THROW(CallMethod(Item, static_cast<ComDispInterface *>(p_wkbook)) > 0);
	CATCH
		ZDELETE(p_wkbook);
	ENDCATCH
	return p_wkbook;
}

long ComExcelWorkbooks::GetCount()
{
	long   count = 0;
	GetProperty(Count, &count);
	return count;
}

ComExcelWorkbook * ComExcelWorkbooks::_Open(const char * pFileName)
{
	ComExcelWorkbook * p_wkbook = new ComExcelWorkbook;
	THROW(SetParam(pFileName));
	THROW(CallMethod(Open, static_cast<ComDispInterface *>(p_wkbook)) > 0);
	CATCH
		ZDELETE(p_wkbook);
	ENDCATCH
	return p_wkbook;
}

int ComExcelWorkbooks::SaveAs(long pos, const char * pPath)
{
	ComExcelWorkbook * p_wkbook = Get(pos);
	int    ok = p_wkbook ? p_wkbook->_SaveAs(pPath) : 0;
	ZDELETE(p_wkbook);
	return ok;
}

int ComExcelWorkbooks::Close(long pos)
{
	ComExcelWorkbook * p_wkbook = Get(pos);
	int    ok = p_wkbook ? p_wkbook->_Close() : 0;
	ZDELETE(p_wkbook);
	return ok;
}

ComExcelWorksheet * ComExcelWorkbooks::GetWorksheet(long bookPos, long sheetPos)
{
	ComExcelWorkbook * p_wkbook = Get(bookPos);
	ComExcelWorksheet * p_sheet = p_wkbook ? p_wkbook->GetWorksheet(sheetPos) : 0;
	ZDELETE(p_wkbook);
	return p_sheet;
}
//
// ComExcelApp
//
ComExcelApp::ComExcelApp() : ComDispInterface()
{
}

ComExcelApp::~ComExcelApp()
{
	CallMethod(Quit);
}

int ComExcelApp::Init()
{
	int    ok = 1;
	THROW(ComDispInterface::Init("Excel.Application", 0));
	THROW(ASSIGN_ID_BY_NAME(this, Workbooks) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, Quit) > 0);
	THROW(ASSIGN_ID_BY_NAME(this, DisplayAlerts) > 0);
	CATCHZOK
	return ok;
}

ComExcelWorkbooks * ComExcelApp::Get()
{
	ComExcelWorkbooks * p_wkbooks = new ComExcelWorkbooks;
	if(GetProperty(Workbooks, static_cast<ComDispInterface *>(p_wkbooks)) <= 0)
		ZDELETE(p_wkbooks);
	return p_wkbooks;
}

ComExcelWorkbook * ComExcelApp::OpenWkBook(const char * pFileName)
{
	int    ok = 1;
	ComExcelWorkbooks * p_wkbooks = Get();
	ComExcelWorkbook  * p_wkbook = p_wkbooks ? p_wkbooks->_Open(pFileName) : 0;
	ZDELETE(p_wkbooks);
	return p_wkbook;
}

ComExcelWorkbook * ComExcelApp::AddWkbook()
{
	int    ok = 1;
	ComExcelWorkbooks * p_wkbooks = Get();
	ComExcelWorkbook  * p_wkbook = p_wkbooks ? p_wkbooks->_Add() : 0;
	ZDELETE(p_wkbooks);
	return p_wkbook;
}

int ComExcelApp::SaveAsWkbook(long pos, const char * pPath)
{
	int    ok = 1;
	ComExcelWorkbooks * p_wkbooks = Get();
	ok = p_wkbooks ? p_wkbooks->SaveAs(pos, pPath) : 0;
	ZDELETE(p_wkbooks);
	return ok;
}

int ComExcelApp::_DisplayAlerts(int yes)
{
	return SetProperty(DisplayAlerts, (BOOL)BIN(yes));
}

ComExcelShapes * ComExcelApp::GetShapes(long bookPos, long sheetPos)
{
	ComExcelWorkbooks * p_wkbooks = Get();
	ComExcelWorksheet * p_sheet = p_wkbooks ? p_wkbooks->GetWorksheet(bookPos, sheetPos) : 0;
	ComExcelShapes * p_shapes = p_sheet ? p_sheet->GetShapes() : 0;
	ZDELETE(p_sheet);
	ZDELETE(p_wkbooks);
	return p_shapes;
}

ComExcelWorksheet * ComExcelApp::GetWorksheet(long bookPos, long sheetPos)
{
	ComExcelWorkbooks * p_wkbooks = Get();
	ComExcelWorksheet * p_sheet = p_wkbooks ? p_wkbooks->GetWorksheet(bookPos, sheetPos) : 0;
	ZDELETE(p_wkbooks);
	return p_sheet;
}

int ComExcelApp::CloseWkbook(long pos)
{
	ComExcelWorkbooks * p_wkbooks = Get();
	int    ok = p_wkbooks ? p_wkbooks->Close(pos) : 0;
	ZDELETE(p_wkbooks);
	return ok;
}
