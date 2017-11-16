// PPSOAPCALLER-VETIS.CPP
// Copyright (c) A.Sobolev 2017
//
#include <ppsoapclient.h>
#include "vetis\ams\mams14ApplicationManagementServiceBindingProxy.h"
#include "VETIS\AMS\mams14AMSMercuryG2BBindingProxy.h"
//#include "vetis\g2b\mg2bAMSMercuryG2BBindingProxy.h"

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	return Implement_SoapModule_DllMain(hModule, dwReason, lpReserved, "Papyrus VetisSoapModule");
}

extern "C" __declspec(dllexport) int VetisDestroyResult(void * pResult)
{
	return pResult ? PPSoapDestroyResultPtr(pResult) : -1;
}

static void FASTCALL ProcessError(ApplicationManagementServiceBindingProxy & rProxi, PPSoapClientSession & rSess)
{
	char   temp_err_buf[1024];
	rProxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
	rSess.SetMsg(temp_err_buf);
}

static int PreprocessCall(ApplicationManagementServiceBindingProxy & rProxy, PPSoapClientSession & rSess, int result)
{
	if(result == SOAP_OK) {
		return 1;
	}
	else {
		ProcessError(rProxy, rSess);
		return 0;
	}
}

//typedef VetisApplicationBlock * (* VETIS_SUBMITAPPLICATIONREQUEST_PROC)(PPSoapClientSession & rSess, const char * pApiKey, const VetisApplicationBlock & rBlk);
//typedef VetisApplicationBlock * (* VETIS_RECEIVEAPPLICATIONRESULT_PROC)(PPSoapClientSession & rSess, const char * pApiKey, const S_GUID & rIssuerId, const S_GUID & rApplicationId);

static int GetResult(const ns4__Application * pSrc, VetisApplicationBlock * pResult)
{
	int    ok = 1;
	if(pSrc && pResult) {
		pResult->ApplicationId.FromStr(pSrc->applicationId);
		pResult->ServiceId = pSrc->serviceId;
		pResult->IssuerId.FromStr(pSrc->issuerId);
		if(pSrc->status)
			pResult->ApplicationStatus = (int)*pSrc->status;
		if(pSrc->issueDate)
			pResult->IssueDate.SetTimeT(*pSrc->issueDate);
		if(pSrc->rcvDate)
			pResult->RcvDate.SetTimeT(*pSrc->rcvDate);
		if(pSrc->prdcRsltDate)
			pResult->PrdcRsltDate.SetTimeT(*pSrc->prdcRsltDate);
		if(pSrc->result)
			pResult->Result = pSrc->result->__any;
		if(pSrc->data) {
			pResult->Data = pSrc->data->__any;
		}
		if(pSrc->errors && pSrc->errors->__sizeerror) {
			/*
			class SOAP_CMAC ns5__BusinessErrorList {
			public:
				int __sizeerror;
				class ns5__BusinessError **error;
			*/
			for(int i = 0; i < pSrc->errors->__sizeerror; i++) {
				/*
				class SOAP_CMAC ns4__Error {
				public:
					char *__item;
					char *code;
					char **qualifier;
				*/
				const ns4__BusinessError * p_src_err = pSrc->errors->error[i];
				if(p_src_err) {
					VetisErrorEntry * p_err_entry = pResult->ErrList.CreateNewItem();
					if(p_err_entry) {
						p_err_entry->Code = p_src_err->code;
						p_err_entry->Item = p_src_err->__item;
						//p_err_entry->Qualifier
					}
				}
			}
		}
	}
	else
		ok = 0;
	return ok;
}

extern "C" __declspec(dllexport) VetisApplicationBlock * Vetis_SubmitApplicationRequest(PPSoapClientSession & rSess, const char * pApiKey, const VetisApplicationBlock & rBlk)
{
	VetisApplicationBlock * p_result = 0;
	ApplicationManagementServiceBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;
	time_t issue_date = rBlk.IssueDate.GetTimeT();
	time_t rcv_date = rBlk.RcvDate.GetTimeT();
	time_t prdc_date = rBlk.PrdcRsltDate.GetTimeT();
	_ns1__submitApplicationRequest param;
	_ns1__submitApplicationResponse resp;
	THROW(param.ns4__application = new ns4__Application);
	THROW(param.ns4__application->data = new ns4__ApplicationDataWrapper);
	gSoapClientInit(&proxi, 0, 0);
	proxi.userid = GetDynamicParamString((temp_buf = rSess.GetUser()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	proxi.passwd = GetDynamicParamString((temp_buf = rSess.GetPassword()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	param.apiKey = GetDynamicParamString(pApiKey, arg_str_pool);
	param.ns4__application->applicationId = GetDynamicParamString(rBlk.ApplicationId.ToStr(S_GUID::fmtIDL, temp_buf), arg_str_pool);
	param.ns4__application->serviceId = GetDynamicParamString(rBlk.ServiceId, arg_str_pool);
	param.ns4__application->issuerId = GetDynamicParamString(rBlk.IssuerId.ToStr(S_GUID::fmtIDL, temp_buf), arg_str_pool);
	param.ns4__application->issueDate = &issue_date;
	param.ns4__application->rcvDate = &rcv_date;
	param.ns4__application->prdcRsltDate = &prdc_date;
	param.ns4__application->data->__any = GetDynamicParamString(rBlk.Data, arg_str_pool);
	THROW(PreprocessCall(proxi, rSess, proxi.submitApplicationRequest(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	if(resp.ns4__application) {
		THROW(p_result = new VetisApplicationBlock);
		PPSoapRegisterResultPtr(p_result);
		GetResult(resp.ns4__application, p_result);
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	delete param.ns4__application->data;
	delete param.ns4__application;
	return p_result;
}

extern "C" __declspec(dllexport) VetisApplicationBlock * Vetsi_ReceiveApplicationResult(PPSoapClientSession & rSess, const char * pApiKey, const S_GUID & rIssuerId, const S_GUID & rApplicationId)
{
	VetisApplicationBlock * p_result = 0;

	ApplicationManagementServiceBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;
	_ns1__receiveApplicationResultRequest param;
	_ns1__receiveApplicationResultResponse resp;

	gSoapClientInit(&proxi, 0, 0);
	proxi.userid = GetDynamicParamString((temp_buf = rSess.GetUser()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	proxi.passwd = GetDynamicParamString((temp_buf = rSess.GetPassword()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	param.apiKey = GetDynamicParamString(pApiKey, arg_str_pool);
	param.issuerId = GetDynamicParamString(rIssuerId.ToStr(S_GUID::fmtIDL, temp_buf), arg_str_pool);
	param.applicationId = GetDynamicParamString(rApplicationId.ToStr(S_GUID::fmtIDL, temp_buf), arg_str_pool);
	THROW(PreprocessCall(proxi, rSess, proxi.receiveApplicationResult(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	if(resp.ns4__application) {
		THROW(p_result = new VetisApplicationBlock);
		PPSoapRegisterResultPtr(p_result);
		GetResult(resp.ns4__application, p_result);
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}
