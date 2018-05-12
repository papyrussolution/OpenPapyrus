// VETIS.CPP
// Copyright (c) A.Sobolev 2017
// @codepage UTF-8
// Модуль для взаимодействия с системой Меркурий (интерфейс ВЕТИС)
//
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>

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
		}
		void   Clear()
		{
			SetBuffer(0);
			IssuerUUID.Z();
		}
		S_GUID IssuerUUID;
	};
	SLAPI  PPVetisInterface();
	SLAPI ~PPVetisInterface();
	int    SLAPI Init(const Param & rP);

	int    SLAPI GetStockEntryList();
	//
	// Операция GetVetDocumentListOperation предназначена для получения всех ветеринарно-сопроводительных документов предприятия. 
	//   При этом список ВСД может быть отфильтрован по следующим критериям:
	//     Тип ВСД: входящий ВСД; исходящий ВСД; производственный ВСД; транспортный ВСД; возвратный ВСД. 
	//     Статус ВСД: оформлен; погашен; аннулирован. 
	//   На вход системы передаются следующие сведения:
	//     информация о пользователе - инициаторе запроса;
	//     информация о предприятии и хозяйствующем субъекте, где осуществляется поиск ВСД;
	//     параметры, по которым будет отфильтрован список ВСД. 
	//   Результатом выполнения данной операции является:
	//     пользователю передаются сведения о запрашиваемых ВСД. 
	//   Запрашиваться пользователем могут только те ВСД, где ветеринарное управление инициатор запроса обслуживает предприятия. 
	//
	int    SLAPI GetVetDocumentList(); // getVetDocumentListRequest
	//
	// Операция IncomingOperation предназначена для оформления в системе Меркурий входящей партии. 
	//   На вход системы, в зависимости от сценария, передаются следующие сведения:
	//     информация об электронном ВСД, по которому продукция поступила на предприятие (для сценария №1);
	//     информация о бумажном ВСД, по которому продукция поступила на предприятие (для сценария №2);
	//     фактические сведения о принимаемой партии;
	//     акт несоответствия, в случае если фактические сведения о продукции отличаются от сведений, указанных в ВСД;
	//     возвратный ВСД, в случае если на весь объем или на его часть оформляется возврат. 
	//   Результатом выполнения данной операции является:
	//     оформление электронного ВСД, в случае, если продукция поступила по бумажному входящему документу (для сценария №2);
	//     гашение электронного ВСД (для сценария №1);
	//     добавление одной записи в журнал входящей продукции;
	//     возвратный ВСД (формируется в случае, если принимается не весь объем продукции);
	//     акт несоответствия (формируется в случае, если фактические сведения о продукции не совпадают с указанными в ВСД). 
	//
	int    SLAPI ProcessIncomingConsignment(); // processIncomingConsignmentRequest
	//
	// Операция PrepareOutgoingConsignmentOperation предназначена для оформления в системе Меркурий транспортной партии. 
	//   На вход системы передаются следующие сведения:
	//     информация об одной или нескольких партиях продукции, из которых будет сформирована транспортная партия;
	//     сведения о получателе транспортной партии;
	//     сведения о транспортном средстве и маршруте его следования;
	//     дополнительные сведения необходимые для оформления ветеринарно-сопроводительного документа (ВСД), например, 
	//     результат ветеринарно-санитарной экспертизы, сведения о ТТН, особые отметки и т.д. 
	//   Результатом выполнения данной операции является:
	//     списание объема с одной или нескольких записей журнала продукции, которые были указаны в заявке;
	//     гашение производственной сертификата, если был указан весь объем по данной записи журнала вырабатываемой продукции;
	//     для каждого наименования продукции указанного в транспортной партии, система Меркурий формирует ветеринарно-сопроводительный документ (ВСД). 
	//
	int    SLAPI PrepareOutgoingConsignment(); // prepareOutgoingConsignmentRequest
	//
	// Операция RegisterProductionOperation предназначена для оформления в системе Меркурий производственной партии, как завершённой, так и незавершённой. 
	//   На вход системы передаются следующие сведения:
	//     информация о сырье, из которого партия или несколько партий были произведены;
	//     информация о произведенной партии или нескольких партиях продукции;
	//     информация о хозяйствующем субъекте - собственнике сырья и выпускаемой продукции и информация о площадке, на которой продукция выпускается;
	//     идентификатор производственной операции (для незавершённого производства);
	//     номер производственной партии;
	//     флаг завершения производственной транзакции. 
	//   Результатом выполнения данной операции является:
	//     списание объема с одной или нескольких записей журнала продукции, указанного в качестве сырья;
	//     добавление одной или нескольких записей в журнал вырабатываемой продукции о партии продукции, которая была произведена или присоединение к 
	//       существующей записи вырабатываемой продукции, если оформляется незаверёшнное производство;
	//     для каждой записи журнала вырабатываемой продукции, которая была добавлена при выполнении операции, система Меркурий формирует 
	//       ветеринарно-сопроводительный документ (ВСД) или происходит увеличение объёма выпущенной продукции в уже оформленном ветеринарном документе 
	//       (для незавершённого производства). 
	//
	int    SLAPI RegisterProductionOperation(); // registerProductionOperationRequest
	//
	// Операция AddBussinessEntityUser предназначена для регистрации новых пользователей в системе Меркурий или 
	//   привязки существующих пользователей к хозяйствующему субъекту.
	//   При выполнении операции на вход системы передаются следующие сведения:
	//     информация о пользователе - инициаторе запроса;
	//     имя пользователя или уникальный идентификатор, если существующий пользователь привязывается к ХС;
	//     данные пользователя (ФИО, паспортные данные, гражданство, адрес электронной почты), если регистрируется новый пользователь;
	//     при регистрации нового пользователя опционально могут быть переданы дополнительные данные пользователя (телефон, рабочий телефон, рабочий адрес электронной почты и т.д.), которые будут сохранены в системе "Ветис.Паспорт";
	//     при регистрации нового пользователя опционально может быть передан список прав пользователя, но назначены эти права будут после активации созданного пользователя. 
	//   Результатом выполнения данной операции является:
	//     регистрация нового пользователя или привязка существующего пользователя к хозяйствующему субъекту. 
	//
	int    SLAPI AddBusinessEntityUser(); // addBusinessEntityUserRequest
	int    SLAPI GetAppliedUserAuthorityList();

	int    SLAPI GetRussianEnterpriseList(TSCollection <VetisEnterprise> & rResult);
	int    SLAPI GetProductItemList(uint offs, uint count, TSCollection <VetisProductItem> & rResult);

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
	if(rBlk.Func == VetisApplicationData::signGetStockEntryListRequest) {
		rBlk.P_GselReq = new VetisGetStockEntryListRequest;
		rBlk.P_GselReq->Initiator.Login = rBlk.User;
		rBlk.P_GselReq->ListOptions.Count = 20;
		ok = 1;
	}
	else if(rBlk.Func == VetisApplicationData::signGetAppliedUserAuthorityListRequest) {
		rBlk.P_LoReq = new VetisListOptionsRequest;
		rBlk.P_LoReq->Initiator.Login = rBlk.User;
		rBlk.P_LoReq->ListOptions.Count = 20;
		ok = 1;
	}
	return ok;
}

//static const char * P_VetisSoapUrl = "https://api.vetrf.ru/platform/services/ApplicationManagementService"; // product
static const char * P_VetisSoapUrl = "https://api2.vetrf.ru:8002/platform/services/ApplicationManagementService"; // test

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
	sess.Setup(P_VetisSoapUrl, user, password);
	{
		VetisApplicationBlock blk;
		blk.User = user;
		blk.Func = appFuncId; //VetisApplicationBlock::detGetStockEntryListReq;
		blk.ServiceId = "mercury-g2b.service:2.0"; // "mercury-g2b.service:2.0";
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

int SLAPI PPVetisInterface::GetProductItemList(uint offs, uint count, TSCollection <VetisProductItem> & rResult)
{
	int    ok = -1;
	SString user, password, api_key;
	TSCollection <VetisProductItem> * p_result = 0; 
	PPSoapClientSession sess;
	VETIS_GETPRODUCTITEMLIST_PROC func = 0;
	THROW(State & stInited);
	THROW(P_Lib);
	THROW_SL(func = (VETIS_GETPRODUCTITEMLIST_PROC)P_Lib->GetProcAddr("Vetis_GetProductItemList"));
	P.GetExtStrData(extssUser, user);
	P.GetExtStrData(extssPassword, password);
	sess.Setup(/*P_VetisSoapUrl*/0, user, password);
	{
		VetisListOptionsRequest lstopt;
		lstopt.ListOptions.Count = count;
		lstopt.ListOptions.Offset = offs;
		p_result = func(sess, &lstopt, 0);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		TSCollection_Copy(rResult, *p_result);
		DestroyResult((void **)&p_result);
		ok = 1;
	}
    CATCHZOK
	return ok;
}

int SLAPI PPVetisInterface::GetRussianEnterpriseList(TSCollection <VetisEnterprise> & rResult)
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

static void Debug_OutputProductItem(const VetisProductItem & rItem, SString & rBuf)
{
	rBuf.Z();
	rBuf.Cat(rItem.Uuid).Tab().Cat(rItem.Guid);
	if(rItem.GlobalID.NotEmpty())
		rBuf.Tab().Cat(rItem.GlobalID);
	if(rItem.Code.NotEmpty())
		rBuf.Tab().Cat(rItem.Code);
	rBuf.Tab().Cat(rItem.Name);
	rBuf.Tab().Cat(rItem.ProductType);
	if(rItem.Gost.NotEmpty())
		rBuf.Tab().Cat(rItem.Gost);
}

int SLAPI TestVetis()
{
	int    ok = 1;
	SString temp_buf;
	TSCollection <VetisEnterprise> ent_list;
	TSCollection <VetisProductItem> productitem_list;
	PPVetisInterface::Param param;
	THROW(PPVetisInterface::SetupParam(param));
	{
		PPVetisInterface ifc;
		THROW(ifc.Init(param));
		//THROW(ifc.GetStockEntryList());
		//THROW(ifc.GetAppliedUserAuthorityList());
		//THROW(ifc.GetRussianEnterpriseList(ent_list));
		{

			PPGetFilePath(PPPATH_LOG, "vetis_product_list.log", temp_buf);
			SFile f_out(temp_buf, SFile::mWrite);
			uint req_count = 50;
			for(uint req_offs = 0; ifc.GetProductItemList(req_offs, req_count, productitem_list);) {
				for(uint i = 0; i < productitem_list.getCount(); i++) {
					const VetisProductItem * p_item = productitem_list.at(i);
					if(p_item) {
						Debug_OutputProductItem(*p_item, temp_buf);
						f_out.WriteLine(temp_buf.CR());
					}
				}
				if(productitem_list.getCount() < req_count)
					break;
				else
					req_offs += productitem_list.getCount();
			}
		}
	}
	CATCHZOK
	return ok;
}


