// PPSOAPCALLER-SFA-HEINEKEN.CPP
// Copyright (c) A.Sobolev 2018
//
#include <ppsoapclient.h>
#include "heineken\heinekenSoapDRPServiceSoapProxy.h"

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	return Implement_SoapModule_DllMain(hModule, dwReason, lpReserved, "Papyrus iSalesPepsiSoapModule");
}

extern "C" __declspec(dllexport) int SfaHeinekenDestroyResult(void * pResult)
{
	return pResult ? PPSoapDestroyResultPtr(pResult) : -1;
}

static void FASTCALL ProcessError(DRPServiceSoapProxy & rProxi, PPSoapClientSession & rSess)
{
	char   temp_err_buf[1024];
	rProxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
	rSess.SetMsg(temp_err_buf);
}

static int PreprocessCall(DRPServiceSoapProxy & rProxy, PPSoapClientSession & rSess, int result)
{
	if(result == SOAP_OK) {
		return 1;
	}
	else {
		ProcessError(rProxy, rSess);
		return 0;
	}
}

int SfaHeineken_SendOrdersStatuses() // DRP_SendOrdersStatuses
{
	return -1;
}

int SfaHeineken_GetSalePoints() // DRP_GetSalePoints
{
	return -1;
}

int SfaHeineken_GetWarehouses() // DRP_GetWarehouses
{
	return -1;
}

int SfaHeineken_SendSellout() // DRP_SendSellout
{
	return -1;
}

int SfaHeineken_SendReturn() // DRP_SendReturn
{
	return -1;
}

int SfaHeineken_SendSellin() // DRP_SendSellin
{
	return -1;
}

int SfaHeineken_DeleteSellout() // DRP_DeleteSellout
{
	return -1;
}

int SfaHeineken_DeleteReturn() // DRP_DeleteReturn
{
	return -1;
}

int SfaHeineken_SendAllContragentDebet() // DRP_SendAllContragentDebet
{
	return -1;
}

int SfaHeineken_SendWarehousesBalance() // DRP_SendWarehousesBalance
{
	return -1;
}

int SfaHeineken_GetEncashments() // DRP_GetEncashments
{
	return -1;
}

int SfaHeineken_GetEncashmentsByDate() // DRP_GetEncashmentsByDate
{
	return -1;
}

static SString * FASTCALL PreprocessAnyResult(char * pAny)
{
	SString * p_result = 0;
	if(pAny) {
		p_result = new SString(pAny);
		PPSoapRegisterResultPtr(p_result);
	}
	return p_result;
}

extern "C" __declspec(dllexport) SString * SfaHeineken_GetSkuAssortiment(PPSoapClientSession & rSess) // DRP_GetSkuAssortment
{
	SString * p_result = 0;
	DRPServiceSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__DRP_USCOREGetSkuAssortment param;
	_ns1__DRP_USCOREGetSkuAssortmentResponse resp;
	param._USCORElogin = GetDynamicParamString(rSess.GetUser(), arg_str_pool);
	param._USCOREpass = GetDynamicParamString(rSess.GetPassword(), arg_str_pool);
	THROW(PreprocessCall(proxi, rSess, proxi.DRP_USCOREGetSkuAssortment(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	p_result = PreprocessAnyResult(resp.DRP_USCOREGetSkuAssortmentResult->__any);
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) SString * SfaHeineken_GetOrders(PPSoapClientSession & rSess, LDATE dt, int demo) // DRP_GetOrders
{
	SString * p_result = 0;
	DRPServiceSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	if(demo) {
		_ns1__DRP_USCOREGetOrdersDemo param;
		_ns1__DRP_USCOREGetOrdersDemoResponse resp;
		param._USCORElogin = GetDynamicParamString(rSess.GetUser(), arg_str_pool);
		param._USCOREpass = GetDynamicParamString(rSess.GetPassword(), arg_str_pool);
		THROW(PreprocessCall(proxi, rSess, proxi.DRP_USCOREGetOrdersDemo(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		p_result = PreprocessAnyResult(resp.DRP_USCOREGetOrdersDemoResult->__any);
	}
	else if(checkdate(dt, 0)) {
		_ns1__DRP_USCOREGetOrdersByDate param;
		_ns1__DRP_USCOREGetOrdersByDateResponse resp;
		param._USCORElogin = GetDynamicParamString(rSess.GetUser(), arg_str_pool);
		param._USCOREpass = GetDynamicParamString(rSess.GetPassword(), arg_str_pool);
		param._USCOREdate = dt.GetTimeT();
		THROW(PreprocessCall(proxi, rSess, proxi.DRP_USCOREGetOrdersByDate(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		p_result = PreprocessAnyResult(resp.DRP_USCOREGetOrdersByDateResult->__any);
	}
	else {
		_ns1__DRP_USCOREGetOrders param;
		_ns1__DRP_USCOREGetOrdersResponse resp;
		param._USCORElogin = GetDynamicParamString(rSess.GetUser(), arg_str_pool);
		param._USCOREpass = GetDynamicParamString(rSess.GetPassword(), arg_str_pool);
		THROW(PreprocessCall(proxi, rSess, proxi.DRP_USCOREGetOrders(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		p_result = PreprocessAnyResult(resp.DRP_USCOREGetOrdersResult->__any);
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}
