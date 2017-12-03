// PPSOAPCALLER-VETIS.CPP
// Copyright (c) A.Sobolev 2017
//
#include <ppsoapclient.h>
#include <sstream>
#include "vetis\vetisamsAMSMercuryG2BBindingProxy.h"
#include "vetis\vetisamsApplicationManagementServiceBindingProxy.h"
#include "vetis\vetisamsEnterpriseServiceBindingProxy.h"

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

static void FASTCALL ProcessError(EnterpriseServiceBindingProxy & rProxi, PPSoapClientSession & rSess)
{
	char   temp_err_buf[1024];
	rProxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
	rSess.SetMsg(temp_err_buf);
}

static int PreprocessCall(EnterpriseServiceBindingProxy & rProxy, PPSoapClientSession & rSess, int result)
{
	if(result == SOAP_OK) {
		return 1;
	}
	else {
		ProcessError(rProxy, rSess);
		return 0;
	}
}

#define VETIS_STRUC_APPLICATION ns4__Application

//typedef VetisApplicationBlock * (* VETIS_SUBMITAPPLICATIONREQUEST_PROC)(PPSoapClientSession & rSess, const char * pApiKey, const VetisApplicationBlock & rBlk);
//typedef VetisApplicationBlock * (* VETIS_RECEIVEAPPLICATIONRESULT_PROC)(PPSoapClientSession & rSess, const char * pApiKey, const S_GUID & rIssuerId, const S_GUID & rApplicationId);

static int GetResult(const VETIS_STRUC_APPLICATION * pSrc, VetisApplicationBlock * pResult)
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
		/*
		if(pSrc->result)
			pResult->Result = pSrc->result->__any;
		if(pSrc->data) {
			pResult->Data = pSrc->data->__any;
		}
		*/
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

static void * CreateAppData(const VetisApplicationBlock & rBlk, TSCollection <InParamString> & rPool)
{
	void * p_result = 0;
	SString temp_buf;
	switch(rBlk.Func) {
		case VetisApplicationData::signGetAppliedUserAuthorityListRequest:
			if(rBlk.P_LoReq) {
				ns5__GetAppliedUserAuthorityListRequest * p = new ns5__GetAppliedUserAuthorityListRequest; // (pilot) 
				// (product) ns6__GetApplicableUserAuthorityListRequest * p = new ns6__GetApplicableUserAuthorityListRequest;
				if(p) {
					p->initiator = new ns7__User;
					p->initiator->login = GetDynamicParamString((temp_buf = rBlk.P_LoReq->Initiator.Login).Transf(CTRANSF_INNER_TO_UTF8), rPool);
					p->localTransactionId = GetDynamicParamString(temp_buf.Z().Cat(rBlk.LocalTransactionId), rPool);
					p->ns3__listOptions = new ns3__ListOptions;
					p->ns3__listOptions->count = GetDynamicParamString(temp_buf.Z().Cat(rBlk.P_LoReq->ListOptions.Count), rPool);
					p->ns3__listOptions->offset = GetDynamicParamString(temp_buf.Z().Cat(rBlk.P_LoReq->ListOptions.Offset), rPool);
					{
						{
							struct soap * p_app_soap = soap_new1(SOAP_XML_CANONICAL|SOAP_XML_INDENT|SOAP_XML_IGNORENS);
							std::stringstream ss;
							p_app_soap->os = &ss;
							soap_write_ns5__GetAppliedUserAuthorityListRequest(p_app_soap, p); // (pilot) 
							// (product) soap_write_ns6__GetApplicableUserAuthorityListRequest(p_app_soap, p);
							p_app_soap->os = NULL; // no longer writing to the string
							soap_end(p_app_soap); // warning: this deletes str with XML too!
							soap_free(p_app_soap);
							temp_buf = ss.str().c_str();
						}
						p_result = GetDynamicParamString(temp_buf, rPool);
					}
					delete p;
				}
			}
			break;
		case VetisApplicationData::signGetRussianEnterpriseListRequest:
			if(rBlk.P_LoReq) {
				_ns8__getRussianEnterpriseListRequest * p = new _ns8__getRussianEnterpriseListRequest; // (pilot) 
				if(p) {
					p->ns3__listOptions = new ns3__ListOptions;
					p->ns3__listOptions->count = GetDynamicParamString(temp_buf.Z().Cat(rBlk.P_LoReq->ListOptions.Count), rPool);
					p->ns3__listOptions->offset = GetDynamicParamString(temp_buf.Z().Cat(rBlk.P_LoReq->ListOptions.Offset), rPool);
					{
						{
							struct soap * p_app_soap = soap_new1(SOAP_XML_CANONICAL|SOAP_XML_INDENT|SOAP_XML_IGNORENS);
							std::stringstream ss;
							p_app_soap->os = &ss;
							soap_write__ns8__getRussianEnterpriseListRequest(p_app_soap, p); // (pilot) 
							// (product) soap_write_ns6__GetApplicableUserAuthorityListRequest(p_app_soap, p);
							p_app_soap->os = NULL; // no longer writing to the string
							soap_end(p_app_soap); // warning: this deletes str with XML too!
							soap_free(p_app_soap);
							temp_buf = ss.str().c_str();
						}
						p_result = GetDynamicParamString(temp_buf, rPool);
					}
					delete p;
				}
			}
			break;
		case VetisApplicationData::signGetStockEntryListRequest:
			if(rBlk.P_GselReq) {
				ns5__GetStockEntryListRequest * p = new ns5__GetStockEntryListRequest;
				if(p) {
					p->initiator = new ns7__User;
					p->initiator->login = GetDynamicParamString((temp_buf = rBlk.P_GselReq->Initiator.Login).Transf(CTRANSF_INNER_TO_UTF8), rPool);
					p->localTransactionId = GetDynamicParamString(temp_buf.Z().Cat(rBlk.LocalTransactionId), rPool);
					p->ns6__enterpriseGuid = GetDynamicParamString(temp_buf.Z().Cat(rBlk.EnterpriseId, S_GUID::fmtIDL), rPool);
					p->ns3__listOptions = new ns3__ListOptions;
					p->ns3__listOptions->count = GetDynamicParamString(temp_buf.Z().Cat(rBlk.P_GselReq->ListOptions.Count), rPool);
					p->ns3__listOptions->offset = GetDynamicParamString(temp_buf.Z().Cat(rBlk.P_GselReq->ListOptions.Offset), rPool);
					{
						{
							struct soap * p_app_soap = soap_new1(SOAP_XML_CANONICAL|SOAP_XML_INDENT|SOAP_XML_IGNORENS);
							std::stringstream ss;
							p_app_soap->os = &ss;
							soap_write_ns5__GetStockEntryListRequest(p_app_soap, p);
							p_app_soap->os = NULL; // no longer writing to the string
							soap_end(p_app_soap); // warning: this deletes str with XML too!
							soap_free(p_app_soap);
							temp_buf = ss.str().c_str();
						}
						p_result = GetDynamicParamString(temp_buf, rPool);
					}
					delete p;
				}
			}
			break;
	}
	return p_result;
}

static void DestroyAppData(const VetisApplicationBlock & rBlk, void * pData)
{
}

static void _GetAddressObjectView(VetisAddressObjectView & rDest, const ns6__AddressObjectView * pSrc)
{
	if(pSrc) {
		rDest.CountryGUID.FromStr(pSrc->countryGuid);
		(rDest.Name = pSrc->name).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.EnglishName = pSrc->englishName).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.RegionCode = pSrc->regionCode).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.View = pSrc->view).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.Type = pSrc->type).Transf(CTRANSF_UTF8_TO_INNER);
		SETFLAG(rDest.Flags, rDest.fHasStreets, pSrc->hasStreets && *pSrc->hasStreets);
	}
}

static void _GetDistrict(VetisDistrict & rDest, const ns6__District * pSrc)
{
	if(pSrc) {
		_GetAddressObjectView(rDest, pSrc);
		rDest.RegionGUID.FromStr(pSrc->regionGuid);
	}
}

static void _GetLocality(VetisLocality & rDest, const ns6__Locality * pSrc)
{
	if(pSrc) {
		_GetAddressObjectView(rDest, pSrc);
		rDest.RegionGUID.FromStr(pSrc->regionGuid);
		rDest.DistrictGUID.FromStr(pSrc->districtGuid);
	}
}

static void _GetStreet(VetisStreet & rDest, const ns6__Street * pSrc)
{
	if(pSrc) {
		_GetAddressObjectView(rDest, pSrc);
		rDest.LocalityGUID.FromStr(pSrc->localityGuid);
	}
}

static void _GetFederalDistrict(VetisFederalDistrict & rDest, const ns6__FederalDistrict * pSrc)
{
	if(pSrc) {
		(rDest.FullName = pSrc->fullName).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.ShortName = pSrc->shortName).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.Abbreviation = pSrc->abbreviation).Transf(CTRANSF_UTF8_TO_INNER);
	}
}

static void _GetCountry(VetisCountry & rDest, const ns6__Country * pSrc)
{
	if(pSrc) {
		(rDest.Name = pSrc->name).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.FullName = pSrc->fullName).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.EnglishName = pSrc->englishName).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.Code = pSrc->code).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.Code3 = pSrc->code3).Transf(CTRANSF_UTF8_TO_INNER);
	}
}

static void _GetAddress(VetisAddress & rDest, const ns6__Address * pSrc)
{
	if(pSrc) {
		(rDest.AddressView = pSrc->addressView ? *pSrc->addressView : 0).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.EnAddressView = pSrc->enAddressView ? *pSrc->enAddressView : 0).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.House = pSrc->house).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.Building = pSrc->building).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.Room = pSrc->room).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.PostBox = pSrc->postBox).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.PostIndex = pSrc->postIndex).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.AdditionalInfo = pSrc->additionalInfo ? *pSrc->additionalInfo : 0).Transf(CTRANSF_UTF8_TO_INNER);
		_GetLocality(rDest.Locality, pSrc->locality);
		_GetLocality(rDest.SubLocality, pSrc->subLocality);
		_GetAddressObjectView(rDest.Region, pSrc->region);
		_GetDistrict(rDest.District, pSrc->district);
		_GetFederalDistrict(rDest.FederalDistrict, pSrc->federalDistrict);
		_GetStreet(rDest.Street, pSrc->street);
		_GetCountry(rDest.Country, pSrc->country);
	}
}

static void _GetGenericEntity(VetisGenericEntity & rDest, const ns3__GenericEntity * pSrc)
{
	if(pSrc) {
		rDest.Uuid.FromStr(pSrc->uuid);
	}
}

static void _GetGenericVersioningEntity(VetisGenericVersioningEntity & rDest, const ns3__GenericVersioningEntity * pSrc)
{
	if(pSrc) {
		_GetGenericEntity(rDest, pSrc);
		rDest.Guid.FromStr(pSrc->guid);
		SETFLAG(rDest.Flags, rDest.fActive, pSrc->active && *pSrc->active);
		SETFLAG(rDest.Flags, rDest.fLast, pSrc->last && *pSrc->last);
		rDest.Status = pSrc->status ? atoi(*pSrc->status) : 0;
		rDest.Next.FromStr(pSrc->next);
		rDest.Previous.FromStr(pSrc->previous);
		if(pSrc->createDate)
			rDest.CreateDate.SetTimeT(*pSrc->createDate);
		else
			rDest.CreateDate.SetZero();
		if(pSrc->updateDate)
			rDest.UpdateDate.SetTimeT(*pSrc->updateDate);
		else
			rDest.UpdateDate.SetZero();
	}
}

static void _GetEnterprise(VetisEnterprise & rDest, const ns6__Enterprise * pSrc); // @prototype

static void _GetBusinessEntity_activityLocation(VetisBusinessEntity_activityLocation & rDest, const _ns6__BusinessEntity_activityLocation * pSrc)
{
	if(pSrc) {
		rDest.GlobalID = pSrc->globalID ? *pSrc->globalID : 0;
		_GetEnterprise(rDest.Enterprise, pSrc->enterprise);
	}
}

static void _GetIncorporationForm(VetisIncorporationForm & rDest, const ns6__IncorporationForm * pSrc)
{
	if(pSrc) {
		_GetGenericEntity(rDest, pSrc);
		(rDest.Name = pSrc->name).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.ShortName = pSrc->shortName).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.Code = pSrc->code).Transf(CTRANSF_UTF8_TO_INNER);
	}
}

static void _GetBusinessEntity(VetisBusinessEntity & rDest, const ns6__BusinessEntity * pSrc)
{
	if(pSrc) {
		_GetGenericVersioningEntity(rDest, pSrc);
		rDest.Type = pSrc->type ? *pSrc->type : 0;
		_GetIncorporationForm(rDest.IncForm, pSrc->incorporationForm);
		(rDest.Name = pSrc->name).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.FullName = pSrc->fullName).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.Fio = pSrc->fio).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.Passport = pSrc->passport).Transf(CTRANSF_UTF8_TO_INNER);
		rDest.Inn = pSrc->inn;
		rDest.Kpp = pSrc->kpp;
		rDest.Ogrn = pSrc->ogrn;
		_GetAddress(rDest.JuridicalAddress, pSrc->juridicalAddress);
		rDest.ActivityLocationList.freeAll();
		if(pSrc->__sizeactivityLocation > 0 && pSrc->activityLocation) {
			for(int i = 0; i < pSrc->__sizeactivityLocation; i++) {
				VetisBusinessEntity_activityLocation * p_new_loc = rDest.ActivityLocationList.CreateNewItem();
				if(p_new_loc) {
					_GetBusinessEntity_activityLocation(*p_new_loc, &pSrc->activityLocation[i]);
				}
			}
		}
	}
}

static void _GetEnterpriseOfficialRegistration(VetisEnterpriseOfficialRegistration & rDest, const ns6__EnterpriseOfficialRegistration * pSrc)
{
	if(pSrc) {
		rDest.ID = pSrc->ID;
		rDest.Kpp = pSrc->kpp;
		ZDELETE(rDest.P_BusinessEntity);
		if(pSrc->businessEntity) {
			rDest.P_BusinessEntity = new VetisBusinessEntity;
			if(rDest.P_BusinessEntity) {
				_GetBusinessEntity(*rDest.P_BusinessEntity, pSrc->businessEntity);
			}
		}
	}
}

static void _GetEntityList(VetisEntityList & rDest, const ns3__EntityList * pSrc)
{
	if(pSrc) {
		rDest.Count = pSrc->count ? *pSrc->count : 0;
		rDest.Offset = pSrc->offset ? *pSrc->offset : 0;
		rDest.Total = pSrc->total ? *pSrc->total : 0;
		SETFLAG(rDest.Flags, rDest.fHasMore, pSrc->hasMore && *pSrc->hasMore);
	}
}

static void _GetEnterpriseActivityList(VetisEnterpriseActivityList & rDest, const ns6__EnterpriseActivityList * pSrc)
{
	if(pSrc) {
		_GetEntityList(rDest, pSrc);
		rDest.Activity.freeAll();
		if(pSrc->activity && pSrc->__sizeactivity > 0) {
			for(int i = 0; i < pSrc->__sizeactivity; i++) {
				VetisEnterpriseActivity * p_new_item = rDest.Activity.CreateNewItem();
				if(p_new_item) {
					_GetGenericEntity(*p_new_item, pSrc->activity[i]);
					(p_new_item->Name = pSrc->activity[i]->name ? *pSrc->activity[i]->name : 0).Transf(CTRANSF_UTF8_TO_INNER);
				}
			}
		}
	}
}

static void _GetEnterprise(VetisEnterprise & rDest, const ns6__Enterprise * pSrc)
{
	if(pSrc) {
		_GetGenericVersioningEntity(rDest, pSrc);
		(rDest.Name = pSrc->name).Transf(CTRANSF_UTF8_TO_INNER);
		(rDest.EnglishName = pSrc->englishName).Transf(CTRANSF_UTF8_TO_INNER);
		_GetAddress(rDest.Address, pSrc->address);
		ZDELETE(rDest.P_Owner);
		rDest.Type = pSrc->type ? atoi(*pSrc->type) : 0;
		if(pSrc->owner) {
			rDest.P_Owner = new VetisBusinessEntity;
			if(rDest.P_Owner)
				_GetBusinessEntity(*rDest.P_Owner, pSrc->owner);
		}
		rDest.OfficialRegistration.freeAll();
		if(pSrc->__sizeofficialRegistration > 0 && pSrc->officialRegistration) {
			for(int i = 0; i < pSrc->__sizeofficialRegistration; i++) {
				VetisEnterpriseOfficialRegistration * p_new_item = rDest.OfficialRegistration.CreateNewItem();
				if(p_new_item) {
					_GetEnterpriseOfficialRegistration(*p_new_item, pSrc->officialRegistration[i]);
				}
			}
		}
		_GetEnterpriseActivityList(rDest.ActivityList, pSrc->activityList);
		rDest.NumberList.clear();
		if(pSrc->numberList && pSrc->numberList->__sizeenterpriseNumber > 0 && pSrc->numberList->enterpriseNumber) {
			SString temp_buf;
			for(int i = 0; i < pSrc->numberList->__sizeenterpriseNumber; i++) {
				(temp_buf = pSrc->numberList->enterpriseNumber[i]).Transf(CTRANSF_UTF8_TO_INNER);
				if(temp_buf.NotEmptyS())
					rDest.NumberList.add(temp_buf);
			}
		}
	}
}

extern "C" __declspec(dllexport) TSCollection <VetisEnterprise> * Vetis_GetRussianEnterpriseList(PPSoapClientSession & rSess, VetisListOptionsRequest * pListReq, VetisEnterprise * pEntFilt)
{
	void * p_app_req = 0;
	int    embed_id = 0;
	TSCollection <VetisEnterprise> * p_result = 0;
	EnterpriseServiceBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;

	_ns8__getRussianEnterpriseListRequest param;
	_ns8__getRussianEnterpriseListResponse resp;
	gSoapClientInit(&proxi, 0, 0);
	proxi.userid = GetDynamicParamString((temp_buf = rSess.GetUser()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	proxi.passwd = GetDynamicParamString((temp_buf = rSess.GetPassword()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	param.ns3__listOptions = new ns3__ListOptions;
	{
		int   list_count = pListReq ? pListReq->ListOptions.Count : 50;
		int   list_offs = pListReq ? pListReq->ListOptions.Offset : 0;
		param.ns3__listOptions->count = GetDynamicParamString(temp_buf.Z().Cat(list_count), arg_str_pool);
		param.ns3__listOptions->offset = GetDynamicParamString(temp_buf.Z().Cat(list_offs), arg_str_pool);
	}
	if(pEntFilt) {

	}
	THROW(PreprocessCall(proxi, rSess, proxi.GetRussianEnterpriseList(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	if(resp.ns6__enterpriseList) {
		THROW(p_result = new TSCollection <VetisEnterprise>);
		PPSoapRegisterResultPtr(p_result);
		if(resp.ns6__enterpriseList && resp.ns6__enterpriseList->count) {
			const int ec = *resp.ns6__enterpriseList->count;
			for(int i = 0; i < ec; i++) {
				ns6__Enterprise * p_ent = resp.ns6__enterpriseList->enterprise[i];
				if(p_ent) {
					VetisEnterprise * p_new_item = p_result->CreateNewItem();
					THROW(p_new_item);
					_GetEnterprise(*p_new_item, p_ent);
				}
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	ZDELETE(param.ns3__listOptions);
	ZDELETE(param.ns6__enterprise);
	return p_result;
}

extern "C" __declspec(dllexport) VetisApplicationBlock * Vetis_SubmitApplicationRequest(PPSoapClientSession & rSess, const char * pApiKey, const VetisApplicationBlock & rBlk)
{
	void * p_app_req = 0;
	int    embed_id = 0;
	VetisApplicationBlock * p_result = 0;
	ApplicationManagementServiceBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;
	time_t issue_date = rBlk.IssueDate.GetTimeT();
	time_t rcv_date = rBlk.RcvDate.GetTimeT();
	time_t prdc_date = rBlk.PrdcRsltDate.GetTimeT();
	_ns1__submitApplicationRequest param;
	_ns1__submitApplicationResponse resp;
	enum ns4__ContentEncoding app_encoding = ns4__ContentEncoding__plain;
	gSoapClientInit(&proxi, 0, 0);
	proxi.userid = GetDynamicParamString((temp_buf = rSess.GetUser()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	proxi.passwd = GetDynamicParamString((temp_buf = rSess.GetPassword()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	THROW(param.ns4__application = new ns4__Application);
	//THROW(param.ns4__application->data = new ns4__ApplicationDataWrapper);
	param.apiKey = GetDynamicParamString(pApiKey, arg_str_pool);
	param.ns4__application->applicationId = GetDynamicParamString(rBlk.ApplicationId.ToStr(S_GUID::fmtIDL, temp_buf), arg_str_pool);
	param.ns4__application->serviceId = GetDynamicParamString(rBlk.ServiceId, arg_str_pool);
	param.ns4__application->issuerId = GetDynamicParamString(rBlk.IssuerId.ToStr(S_GUID::fmtIDL, temp_buf), arg_str_pool);
	param.ns4__application->issueDate = &issue_date;
	param.ns4__application->rcvDate = &rcv_date;
	param.ns4__application->prdcRsltDate = &prdc_date;
	THROW(p_app_req = CreateAppData(rBlk, arg_str_pool));
	{
		//
		//param.ns5__application->data = (ns5__ApplicationDataWrapper *)p_app_req;
		param.ns4__application->data = new ns4__ApplicationDataWrapper;
		param.ns4__application->data->encoding = &app_encoding;
		param.ns4__application->data->__any = (char *)p_app_req;
		//
	}
	THROW(PreprocessCall(proxi, rSess, proxi.submitApplicationRequest(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	if(resp.ns4__application) {
		THROW(p_result = new VetisApplicationBlock);
		PPSoapRegisterResultPtr(p_result);
		GetResult(resp.ns4__application, p_result);
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	DestroyAppData(rBlk, param.ns4__application->data);
	//delete param.ns4__application->data;
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
