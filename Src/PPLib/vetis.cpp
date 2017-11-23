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
	return ok;
}

int SLAPI PPVetisInterface::GetStockEntryList()
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
	sess.Setup(0/*url*/, user, password);
	{
		VetisApplicationBlock blk;
		blk.Func = VetisApplicationBlock::detGetStockEntryListReq;
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
	return ok;
}

