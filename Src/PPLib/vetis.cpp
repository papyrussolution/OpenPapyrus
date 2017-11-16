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
	class VetisEntry {
	public:
		SLAPI  VetisEntry()
		{
		}
		virtual SLAPI ~VetisEntry()
		{
		}
	protected:
		virtual int Pack(SXml::WDoc & rDoc)
		{
			return -1;
		}
		virtual int Unpack(xmlParserCtxt * pParseCtx, const SString & rSrc)
		{
			return -1;
		}
		SString TempBuf;
	};
	class VeListOptions : public VetisEntry {
	public:
		VeListOptions() : Count(0), Offset(0)
		{
		}
		// count; offset
		int    Count;
		int    Offset;
	private:
		virtual int Pack(SXml::WDoc & rDoc)
		{
			SXml::WNode n_l(rDoc, "listOptions");
			n_l.PutInner("count", TempBuf.Z().Cat(Count));
			if(Offset > 0)
				n_l.PutInner("offset", TempBuf.Z().Cat(Offset));
			return 1;
		}
		virtual int Unpack(xmlParserCtxt * pParseCtx, const SString & rSrc)
		{
			return -1;
		}
	};


	int    SubmitRequest(const VetisApplicationBlock & rQ, VetisApplicationBlock & rResult);
	int    ReceiveResult(const S_GUID & rAppId, VetisApplicationBlock & rResult);

	long   State;
	SString LogFileName;
	SDynLibrary * P_Lib;
	void * P_DestroyFunc;
	Param  P;
};

SLAPI PPVetisInterface::PPVetisInterface() : State(0), P_Lib(0), P_DestroyFunc(0)
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

int SLAPI PPVetisInterface::TestCall()
{
	int    ok = -1;
	return ok;
}

