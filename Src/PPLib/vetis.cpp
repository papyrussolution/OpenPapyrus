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
		void   Clear()
		{
			SetBuffer(0);
			IssuerUUID.SetZero();
		}
		S_GUID IssuerUUID;
	};
	SLAPI  PPVetisInterface();
	SLAPI ~PPVetisInterface();
	int    SLAPI Init(const Param & rP);

	int    SLAPI GetStockEntryList();

	static int SLAPI SetupParam(Param & rP);
private:
	int    SubmitRequest(int appFuncId, const void * pAppData, VetisApplicationBlock & rResult);
	int    ReceiveResult(const S_GUID & rAppId, VetisApplicationBlock & rResult);
	int    SLAPI PrepareAppReqData(VetisApplicationBlock & rBlk, const void * pAppData);
	int    SLAPI PreprocessResult(const void * pResult, const PPSoapClientSession & rSess);
	void   FASTCALL DestroyResult(void ** ppResult);

	long   State;
	SString LogFileName;
	SString LastMsg;
	SDynLibrary * P_Lib;
	void * P_DestroyFunc;
	Param   P;
	int64   LastLocalTransactionId;
};

//static 
int SLAPI PPVetisInterface::SetupParam(Param & rP)
{
	rP.Clear();

	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   main_org_id = 0;
	SString temp_buf;
	PPAlbatrosConfig acfg;
	THROW(PPAlbatrosCfgMngr::Get(&acfg) > 0);
	GetMainOrgID(&main_org_id);
	acfg.GetExtStrData(ALBATROSEXSTR_VETISUSER, temp_buf);
	THROW(temp_buf.NotEmptyS()); // @error
	rP.PutExtStrData(extssUser, temp_buf);
	acfg.GetPassword(ALBATROSEXSTR_VETISPASSW, temp_buf);
	THROW(temp_buf.NotEmptyS()); // @error
	rP.PutExtStrData(extssPassword, temp_buf);
	acfg.GetExtStrData(ALBATROSEXSTR_VETISAPIKEY, temp_buf);
	THROW(temp_buf.NotEmptyS()); // @error
	rP.PutExtStrData(extssApiKey, temp_buf);
	{
		ObjTagItem tag_item;
		if(p_ref->Ot.GetTag(PPOBJ_PERSON, main_org_id, PPTAG_PERSON_VETISUUID, &tag_item) > 0) {
			tag_item.GetGuid(&rP.IssuerUUID);
		}
		THROW(!rP.IssuerUUID.IsZero()); // @error
	}
	CATCHZOK
	return ok;
}

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
	State |= stInited;
	return ok;
}

int SLAPI PPVetisInterface::PreprocessResult(const void * pResult, const PPSoapClientSession & rSess)
{
	LastMsg = rSess.GetMsg();
    return BIN(pResult);
}

void FASTCALL PPVetisInterface::DestroyResult(void ** ppResult)
{
	if(P_DestroyFunc) {
		((UHTT_DESTROYRESULT)P_DestroyFunc)(*ppResult);
		*ppResult = 0;
	}
}

int SLAPI PPVetisInterface::PrepareAppReqData(VetisApplicationBlock & rBlk, const void * pAppData)
{
	int    ok = 0;
	if(rBlk.Func == VetisApplicationBlock::detGetStockEntryListReq) {
		rBlk.P_GselReq = new VetisGetStockEntryListRequest;
		rBlk.P_GselReq->Initiator.Login = rBlk.User;
		rBlk.P_GselReq->ListOptions.Count = 50;
		ok = 1;
	}
	return ok;
}

int SLAPI PPVetisInterface::SubmitRequest(int appFuncId, const void * pAppData, VetisApplicationBlock & rResult)
{
	int    ok = -1;
	SString temp_buf;
	SString user, password, api_key;
	VetisApplicationBlock * p_result = 0; 
	PPSoapClientSession sess;
	VETIS_SUBMITAPPLICATIONREQUEST_PROC func = 0;
	THROW(State & stInited);
	THROW(P_Lib);
	THROW_SL(func = (VETIS_SUBMITAPPLICATIONREQUEST_PROC)P_Lib->GetProcAddr("Vetis_SubmitApplicationRequest"));
	P.GetExtStrData(extssUser, user);
	P.GetExtStrData(extssPassword, password);
	P.GetExtStrData(extssApiKey, api_key);
	sess.Setup(0/*url*/, user, password);
	{
		VetisApplicationBlock blk;
		blk.User = user;
		blk.Func = appFuncId; //VetisApplicationBlock::detGetStockEntryListReq;
		blk.ServiceId = "mercury-g2b.service:2.0";
		blk.IssuerId = P.IssuerUUID;
		blk.IssueDate = getcurdatetime_();
		blk.ApplicationId.Generate();
		blk.LocalTransactionId = ++LastLocalTransactionId;
		THROW(PrepareAppReqData(blk, pAppData));
		p_result = func(sess, api_key, blk);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		rResult = *p_result;
		//LogResultMsgList(p_result);
		DestroyResult((void **)&p_result);
		ok = 1;
	}
    CATCHZOK
	return ok;
}

int SLAPI PPVetisInterface::ReceiveResult(const S_GUID & rAppId, VetisApplicationBlock & rResult)
{
	int    ok = -1;
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
	sess.Setup(0/*url*/, user, password);
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

int SLAPI PPVetisInterface::GetStockEntryList()
{
	int    ok = 1;
	VetisApplicationBlock submit_result;
	VetisApplicationBlock receive_result;
	THROW(SubmitRequest(VetisApplicationBlock::detGetStockEntryListReq, 0, submit_result));
	if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
		SDelay(200);
		THROW(ReceiveResult(submit_result.ApplicationId, receive_result));
	}
	CATCHZOK
	return ok;
}

int SLAPI TestVetis()
{
	int    ok = 1;
	PPVetisInterface::Param param;
	THROW(PPVetisInterface::SetupParam(param));
	{
		PPVetisInterface ifc;
		THROW(ifc.Init(param));
		THROW(ifc.GetStockEntryList());
	}
	CATCHZOK
	return ok;
}


