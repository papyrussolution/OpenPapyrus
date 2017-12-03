// VETIS.CPP
// Copyright (c) A.Sobolev 2017
// @codepage UTF-8
// Модуль для взаимодействия с системой Меркурий (интерфейс ВЕТИС)
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>

#if 0
class SOAP_CMAC ns5__Application {
public:
	char *applicationId;	/* optional element of type ns4:UUID */
	enum ns5__ApplicationStatus *status;	/* optional element of type ns5:ApplicationStatus */
	char *serviceId;	/* optional element of type xsd:NCName */
	char *issuerId;	/* optional element of type ns4:UUID */
	time_t *issueDate;	/* optional element of type xsd:dateTime */
	time_t *rcvDate;	/* optional element of type xsd:dateTime */
	time_t *prdcRsltDate;	/* optional element of type xsd:dateTime */
	class ns5__ApplicationDataWrapper *data;	/* optional element of type ns5:ApplicationDataWrapper */
	class ns5__ApplicationResultWrapper *result;	/* optional element of type ns5:ApplicationResultWrapper */
	class ns5__BusinessErrorList *errors;	/* optional element of type ns5:BusinessErrorList */
	struct soap *soap;	/* transient */
};
#endif

class PPVetisInterface {
public:
	enum {
		extssApplicationId = 1,
		extssServiceId,
		extssApiKey,
		extssUser,
		extssPassword
	};
	enum {
		stInited = 0x0001
	};
	struct Param : public PPExtStrContainer {
		SLAPI  Param() 
		{
			IssuerUUID.SetZero();
		}
		S_GUID IssuerUUID;
	};
	SLAPI  PPVetisInterface();
	SLAPI ~PPVetisInterface();
	int    SLAPI Init(const Param & rP);
	int    SLAPI TestCall();

	int    SLAPI GetStockEntryList();
<<<<<<< HEAD
	int    SLAPI GetAppliedUserAuthorityList();
	int    SLAPI GetRussianEnterpriseListRequest(TSCollection <VetisEnterprise> & rResult);

	static int SLAPI SetupParam(Param & rP);
=======
>>>>>>> parent of f010b32... Version 9.8.9
private:
	int    SubmitRequest(const VetisApplicationBlock & rQ, VetisApplicationBlock & rResult);
	int    ReceiveResult(const S_GUID & rAppId, VetisApplicationBlock & rResult);

	long   State;
	SString LogFileName;
	SDynLibrary * P_Lib;
	void * P_DestroyFunc;
	Param   P;
	int64   LastLocalTransactionId;
};

SLAPI PPVetisInterface::PPVetisInterface() : State(0), P_Lib(0), P_DestroyFunc(0), LastLocalTransactionId(0)
{
	PPGetFilePath(PPPATH_LOG, "vetis.log", LogFileName);
 	{
		SString lib_path;
		PPGetFilePath(PPPATH_BIN, "PPSoapMercury_AMS.dll", lib_path);
		P_Lib = new SDynLibrary(lib_path);
		if(P_Lib && !P_Lib->IsValid()) {
			ZDELETE(P_Lib);
		}
		if(P_Lib)
			P_DestroyFunc = (void *)P_Lib->GetProcAddr("VetisDestroyResult");
	}
}

SLAPI PPVetisInterface::~PPVetisInterface()
{
	P_DestroyFunc = 0;
	delete P_Lib;
}

int SLAPI PPVetisInterface::Init(const Param & rP)
{
	int    ok = 1;
	P = rP;
	return ok;
}

int SLAPI PPVetisInterface::SubmitRequest(const VetisApplicationBlock & rQ, VetisApplicationBlock & rResult)
{
	int    ok = 0;
	return ok;
}

int SLAPI PPVetisInterface::ReceiveResult(const S_GUID & rAppId, VetisApplicationBlock & rResult)
{
	int    ok = 0;
<<<<<<< HEAD
	if(rBlk.Func == VetisApplicationData::signGetStockEntryListRequest) {
		rBlk.P_GselReq = new VetisGetStockEntryListRequest;
		rBlk.P_GselReq->Initiator.Login = rBlk.User;
		rBlk.P_GselReq->ListOptions.Count = 20;
		ok = 1;
	}
	else if(rBlk.Func == VetisApplicationData::signGetAppliedUserAuthorityListRequest) {
		rBlk.P_LoReq = new VetisListOptionsRequest(rBlk.Func);
		rBlk.P_LoReq->Initiator.Login = rBlk.User;
		rBlk.P_LoReq->ListOptions.Count = 20;
		ok = 1;
	}
	else if(rBlk.Func == VetisApplicationData::signGetRussianEnterpriseListRequest) {
		rBlk.P_LoReq = new VetisListOptionsRequest(rBlk.Func);
		rBlk.P_LoReq->Initiator.Login = rBlk.User;
		rBlk.P_LoReq->ListOptions.Count = 20;
		ok = 1;
	}
	return ok;
}

//static const char * P_VetisSoapUrl = "https://api.vetrf.ru/platform/services/ApplicationManagementService"; // product
static const char * P_VetisSoapUrl = "https://api2.vetrf.ru:8002/platform/services/ApplicationManagementService"; // test

int SLAPI PPVetisInterface::SubmitRequest(int appFuncId, const void * pAppData, VetisApplicationBlock & rResult)
=======
	return ok;
}

int SLAPI PPVetisInterface::GetStockEntryList()
>>>>>>> parent of f010b32... Version 9.8.9
{
	int    ok = -1;
	SString temp_buf;
	SString user, password, api_key;
	VetisApplicationBlock * p_result = 0;
	PPSoapClientSession sess;
	SapEfesCallHeader sech;
	VETIS_SUBMITAPPLICATIONREQUEST_PROC func = 0;
	THROW(State & stInited);
	THROW(P_Lib);
	THROW_SL(func = (VETIS_SUBMITAPPLICATIONREQUEST_PROC)P_Lib->GetProcAddr("Vetis_SubmitApplicationRequest"));
	P.GetExtStrData(extssUser, user);
	P.GetExtStrData(extssPassword, password);
	P.GetExtStrData(extssApiKey, api_key);
	sess.Setup(P_VetisSoapUrl, user, password);
	{
		VetisApplicationBlock blk;
<<<<<<< HEAD
		blk.User = user;
		blk.Func = appFuncId; //VetisApplicationBlock::detGetStockEntryListReq;
		if(appFuncId == VetisApplicationData::signGetRussianEnterpriseListRequest)
			blk.ServiceId = "EnterpriseService";
		else
			blk.ServiceId = "mercury-g2b.service"; // "mercury-g2b.service:2.0";
		blk.IssuerId = P.IssuerUUID;
		blk.IssueDate = getcurdatetime_();
		blk.ApplicationId.Generate();
=======
		blk.Func = VetisApplicationBlock::detGetStockEntryListReq;
>>>>>>> parent of f010b32... Version 9.8.9
		blk.LocalTransactionId = ++LastLocalTransactionId;
		blk.P_GselReq = new VetisGetStockEntryListRequest;
		p_result = func(sess, api_key, blk);
		//THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		//LogResultMsgList(p_result);
		//DestroyResult((void **)&p_result);
	}
    CATCHZOK
	return ok;
}

int SLAPI PPVetisInterface::TestCall()
{
	int    ok = -1;
<<<<<<< HEAD
	SString user, password, api_key;
	VetisApplicationBlock * p_result = 0; 
	PPSoapClientSession sess;
	VETIS_RECEIVEAPPLICATIONRESULT_PROC func = 0;
	THROW(State & stInited);
	THROW(P_Lib);
	THROW_SL(func = (VETIS_RECEIVEAPPLICATIONRESULT_PROC)P_Lib->GetProcAddr("Vetsi_ReceiveApplicationResult"));
	P.GetExtStrData(extssUser, user);
	P.GetExtStrData(extssPassword, password);
	P.GetExtStrData(extssApiKey, api_key);
	sess.Setup(P_VetisSoapUrl, user, password);
	{
		//VetisApplicationBlock * Vetsi_ReceiveApplicationResult(PPSoapClientSession & rSess, const char * pApiKey, const S_GUID & rIssuerId, const S_GUID & rApplicationId)
		p_result = func(sess, api_key, P.IssuerUUID, rAppId);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		rResult = *p_result;
		//LogResultMsgList(p_result);
		DestroyResult((void **)&p_result);
		ok = 1;
	}
    CATCHZOK
	return ok;
}

int SLAPI PPVetisInterface::GetRussianEnterpriseListRequest(TSCollection <VetisEnterprise> & rResult)
{
	int    ok = -1;
	SString user, password, api_key;
	TSCollection <VetisEnterprise> * p_result = 0; 
	PPSoapClientSession sess;
	VETIS_GETRUSSIANENTERPRISELIST_PROC func = 0;
	THROW(State & stInited);
	THROW(P_Lib);
	THROW_SL(func = (VETIS_GETRUSSIANENTERPRISELIST_PROC)P_Lib->GetProcAddr("Vetis_GetRussianEnterpriseList"));
	P.GetExtStrData(extssUser, user);
	P.GetExtStrData(extssPassword, password);
	sess.Setup(/*P_VetisSoapUrl*/0, user, password);
	{
		p_result = func(sess, 0, 0);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		TSCollection_Copy(rResult, *p_result);
		DestroyResult((void **)&p_result);
		ok = 1;
	}
    CATCHZOK
	return ok;
}

int SLAPI PPVetisInterface::GetAppliedUserAuthorityList()
{
	int    ok = 1;
	VetisApplicationBlock submit_result;
	VetisApplicationBlock receive_result;
	THROW(SubmitRequest(VetisApplicationData::signGetAppliedUserAuthorityListRequest, 0, submit_result));
	if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
		SDelay(1000);
		THROW(ReceiveResult(submit_result.ApplicationId, receive_result));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPVetisInterface::GetStockEntryList()
{
	int    ok = 1;
	VetisApplicationBlock submit_result;
	VetisApplicationBlock receive_result;
	THROW(SubmitRequest(VetisApplicationData::signGetStockEntryListRequest, 0, submit_result));
	if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
		SDelay(1000);
		THROW(ReceiveResult(submit_result.ApplicationId, receive_result));
	}
	CATCHZOK
	return ok;
}

int SLAPI TestVetis()
{
	int    ok = 1;

	TSCollection <VetisEnterprise> ent_list;

	PPVetisInterface::Param param;
	THROW(PPVetisInterface::SetupParam(param));
	{
		PPVetisInterface ifc;
		THROW(ifc.Init(param));
		//THROW(ifc.GetStockEntryList());
		//THROW(ifc.GetAppliedUserAuthorityList());
		THROW(ifc.GetRussianEnterpriseListRequest(ent_list));
	}
	CATCHZOK
	return ok;
}


=======
	return ok;
}

>>>>>>> parent of f010b32... Version 9.8.9
