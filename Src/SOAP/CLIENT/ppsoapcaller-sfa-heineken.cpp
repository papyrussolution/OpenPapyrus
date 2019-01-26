// PPSOAPCALLER-SFA-HEINEKEN.CPP
// Copyright (c) A.Sobolev 2018, 2019
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

static int FASTCALL PreprocessCall(DRPServiceSoapProxy & rProxy, PPSoapClientSession & rSess, int result)
{
	if(result == SOAP_OK) {
		return 1;
	}
	else {
		ProcessError(rProxy, rSess);
		return 0;
	}
}

int SfaHeineken_GetSalePoints() // DRP_GetSalePoints
{
	return -1;
}

int SfaHeineken_GetWarehouses() // DRP_GetWarehouses
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

static SString * FASTCALL PreprocessAnyResult(char * pAny)
{
	SString * p_result = 0;
	if(pAny) {
		p_result = new SString(pAny);
		PPSoapRegisterResultPtr(p_result);
	}
	return p_result;
}

extern "C" __declspec(dllexport) SString * SfaHeineken_SendWarehousesBalance(PPSoapClientSession & rSess, const TSVector <SfaHeinekenWarehouseBalanceEntry> & rList) // DRP_SendWarehousesBalance
{
	//int DRP_USCORESendWarehousesBalance(_ns1__DRP_USCORESendWarehousesBalance *ns1__DRP_USCORESendWarehousesBalance, _ns1__DRP_USCORESendWarehousesBalanceResponse *ns1__DRP_USCORESendWarehousesBalanceResponse) { return DRP_USCORESendWarehousesBalance(NULL, NULL, ns1__DRP_USCORESendWarehousesBalance, ns1__DRP_USCORESendWarehousesBalanceResponse); }
	SString * p_result = 0;
	DRPServiceSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__DRP_USCORESendWarehousesBalance param;
	_ns1__DRP_USCORESendWarehousesBalanceResponse resp;
	param._USCORElogin = GetDynamicParamString(rSess.GetUser(), arg_str_pool);
	param._USCOREpass = GetDynamicParamString(rSess.GetPassword(), arg_str_pool);
	THROW(param._USCOREwarehouseBalanceData = new ns1__ArrayOfBalance);
	param._USCOREwarehouseBalanceData->__sizeBalance = (int)rList.getCount();
	THROW(param._USCOREwarehouseBalanceData->Balance = (ns1__Balance **)SAlloc::C(rList.getCount(), sizeof(ns1__Balance *)));
	for(uint i = 0; i < rList.getCount(); i++) {
		const SfaHeinekenWarehouseBalanceEntry & r_entry = rList.at(i);
		THROW(param._USCOREwarehouseBalanceData->Balance[i] = new ns1__Balance);
		param._USCOREwarehouseBalanceData->Balance[i]->SkuID = r_entry.ForeignGoodsID;
		param._USCOREwarehouseBalanceData->Balance[i]->WarehouseID = r_entry.ForeignLocID;
		param._USCOREwarehouseBalanceData->Balance[i]->Amount = r_entry.Rest;
	}
	THROW(PreprocessCall(proxi, rSess, proxi.DRP_USCORESendWarehousesBalance(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	p_result = PreprocessAnyResult(resp.DRP_USCORESendWarehousesBalanceResult->__any);
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	for(uint i = 0; i < rList.getCount(); i++) {
		delete param._USCOREwarehouseBalanceData->Balance[i];
	}	
	ZFREE(param._USCOREwarehouseBalanceData->Balance);
	ZDELETE(param._USCOREwarehouseBalanceData);
	return p_result;
}

static ns1__ArrayOfDeliveryPosition * FASTCALL CreateDeliveryPositions(const TSCollection <SfaHeinekenDeliveryPosition> & rSrcList, TSCollection <InParamString> & rArgStrPool)
{
	ns1__ArrayOfDeliveryPosition * p_result = 0;
	SString temp_buf;
	THROW(p_result = new ns1__ArrayOfDeliveryPosition);
	THROW(p_result->DeliveryPosition = (ns1__DeliveryPosition **)PPSoapCreateArray(rSrcList.getCount(), p_result->__sizeDeliveryPosition));
	for(uint i = 0; i < rSrcList.getCount(); i++) {
		const SfaHeinekenDeliveryPosition * p_src_item = rSrcList.at(i);
		ns1__DeliveryPosition * p_item = new ns1__DeliveryPosition;
		THROW(p_item);
		p_result->DeliveryPosition[i] = p_item;
		temp_buf.Z().Cat(p_src_item->SkuID);
		p_item->SkuID = GetDynamicParamString(temp_buf, rArgStrPool);
		temp_buf.Z().Cat(p_src_item->Count, MKSFMTD(0, 3, NMBF_NOTRAILZ));
		p_item->Count = GetDynamicParamString(temp_buf, rArgStrPool);
		temp_buf.Z().Cat(p_src_item->Volume, MKSFMTD(0, 6, NMBF_NOTRAILZ));
		p_item->Volume = GetDynamicParamString(temp_buf, rArgStrPool);
		temp_buf.Z().Cat(p_src_item->Amount, MKSFMTD(0, 2, 0));
		p_item->Rubles = GetDynamicParamString(temp_buf, rArgStrPool);
		p_item->Comments = GetDynamicParamString(p_src_item->Comments, rArgStrPool);
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

static void FASTCALL DestroyDeliveryPositions(ns1__ArrayOfDeliveryPosition * pList)
{
	if(pList) {
		for(int i = 0; i < pList->__sizeDeliveryPosition; i++) {
			ZDELETE(pList->DeliveryPosition);
		}
		ZFREE(pList->DeliveryPosition);
		ZDELETE(pList);
	}
}

extern "C" __declspec(dllexport) SString * SfaHeineken_DeleteSellout(PPSoapClientSession & rSess, const SString & rCode, LDATE date) // DRP_DeleteSellout
{
	SString * p_result = 0;
	SString temp_buf;
	DRPServiceSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	_ns1__DRP_USCOREDeleteSellOut param;
	_ns1__DRP_USCOREDeleteSellOutResponse resp;
	param._USCORElogin = GetDynamicParamString(rSess.GetUser(), arg_str_pool);
	param._USCOREpass = GetDynamicParamString(rSess.GetPassword(), arg_str_pool);
	param._USCOREinvoiceNum = GetDynamicParamString(rCode, arg_str_pool);
	param._USCOREinvoiceDate = GetDynamicParamString(temp_buf.Z().Cat(date, DATF_ISO8601|DATF_CENTURY), arg_str_pool);
	THROW(PreprocessCall(proxi, rSess, proxi.DRP_USCOREDeleteSellOut(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	p_result = PreprocessAnyResult(resp.DRP_USCOREDeleteSellOutResult->__any);
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) SString * SfaHeineken_SendSellout(PPSoapClientSession & rSess, const TSCollection <SfaHeinekenInvoice> & rList) // DRP_SendSellout
{
	//int DRP_USCORESendSellOut(_ns1__DRP_USCORESendSellOut *ns1__DRP_USCORESendSellOut, _ns1__DRP_USCORESendSellOutResponse *ns1__DRP_USCORESendSellOutResponse) { return DRP_USCORESendSellOut(NULL, NULL, ns1__DRP_USCORESendSellOut, ns1__DRP_USCORESendSellOutResponse); }
	SString * p_result = 0;
	SString temp_buf;
	DRPServiceSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__DRP_USCORESendSellOut param;
	_ns1__DRP_USCORESendSellOutResponse resp;
	param._USCORElogin = GetDynamicParamString(rSess.GetUser(), arg_str_pool);
	param._USCOREpass = GetDynamicParamString(rSess.GetPassword(), arg_str_pool);
	THROW(param._USCOREinvoices = new ns1__ArrayOfInvoice);
	THROW(param._USCOREinvoices->Invoice = (ns1__Invoice **)PPSoapCreateArray(rList.getCount(), param._USCOREinvoices->__sizeInvoice));
	for(uint i = 0; i < rList.getCount(); i++) {
		const SfaHeinekenInvoice * p_entry = rList.at(i);
		ns1__Invoice * p_inv = new ns1__Invoice;
		THROW(p_inv);
		param._USCOREinvoices->Invoice[i] = p_inv;
		{
			p_inv->InvoiceNum = GetDynamicParamString(p_entry->Code, arg_str_pool);
			temp_buf.Z().Cat(p_entry->Dt, DATF_ISO8601|DATF_CENTURY);
			p_inv->InvoiceDate = GetDynamicParamString(temp_buf, arg_str_pool);
			if(p_entry->OrderList.getCount()) {
				THROW(p_inv->OrderDeliveries = new ns1__ArrayOfOrderDelivery);
				THROW(p_inv->OrderDeliveries->OrderDelivery = (ns1__OrderDelivery **)PPSoapCreateArray(p_entry->OrderList.getCount(), p_inv->OrderDeliveries->__sizeOrderDelivery));
				for(uint j = 0; j < p_entry->OrderList.getCount(); j++) {
					const SfaHeinekenOrderDelivery * p_src_item = p_entry->OrderList.at(j);
					ns1__OrderDelivery * p_item = new ns1__OrderDelivery;
					THROW(p_item);
					p_inv->OrderDeliveries->OrderDelivery[j] = p_item;
					p_src_item->OrderUuid.ToStr(S_GUID::fmtIDL, temp_buf);
					p_item->OrderID = GetDynamicParamString(temp_buf, arg_str_pool);
					THROW(p_item->DeliveryPositions = CreateDeliveryPositions(p_src_item->DeliveryList, arg_str_pool));
				}
			}
			if(p_entry->DistributorDeliveryList.getCount()) {
				THROW(p_inv->DistributorDeliveries = new ns1__ArrayOfDistributorDelivery);
				THROW(p_inv->DistributorDeliveries->DistributorDelivery = (ns1__DistributorDelivery **)PPSoapCreateArray(p_entry->DistributorDeliveryList.getCount(), p_inv->DistributorDeliveries->__sizeDistributorDelivery));
				for(uint j = 0; j < p_entry->DistributorDeliveryList.getCount(); j++) {
					const SfaHeinekenDistributorDelivery * p_src_item = p_entry->DistributorDeliveryList.at(j);
					ns1__DistributorDelivery * p_item = new ns1__DistributorDelivery;
					THROW(p_item);
					p_inv->DistributorDeliveries->DistributorDelivery[j] = p_item;
					p_item->DistributorOrderID = GetDynamicParamString(p_src_item->InnerOrderCode, arg_str_pool);
					p_item->SalePointID = GetDynamicParamString(temp_buf.Z().Cat(p_src_item->SalePointID), arg_str_pool);
					THROW(p_item->DeliveryPositions = CreateDeliveryPositions(p_src_item->DeliveryList, arg_str_pool));
				}
			}
			if(p_entry->DistributorSalePointDeliveryList.getCount()) {
				THROW(p_inv->DistributorSalePointDeliveries = new ns1__ArrayOfDistributorSalePointDelivery);
				THROW(p_inv->DistributorSalePointDeliveries->DistributorSalePointDelivery = (ns1__DistributorSalePointDelivery **)PPSoapCreateArray(
					p_entry->DistributorSalePointDeliveryList.getCount(), p_inv->DistributorSalePointDeliveries->__sizeDistributorSalePointDelivery));
				for(uint j = 0; j < p_entry->DistributorSalePointDeliveryList.getCount(); j++) {
					const SfaHeinekenSalePointDelivery * p_src_item = p_entry->DistributorSalePointDeliveryList.at(j);
					ns1__DistributorSalePointDelivery * p_item = new ns1__DistributorSalePointDelivery;
					THROW(p_item);
					p_inv->DistributorSalePointDeliveries->DistributorSalePointDelivery[j] = p_item;
					p_item->DistributorOrderID = GetDynamicParamString(p_src_item->InnerOrderCode, arg_str_pool);
					p_item->DistributorSalePointID = GetDynamicParamString(temp_buf.Z().Cat(p_src_item->InnerDlvrLocID), arg_str_pool); 
					p_item->SalePointName = GetDynamicParamString(p_src_item->DlvrLocName, arg_str_pool); 
					p_item->SalePointAddress = GetDynamicParamString(p_src_item->DlvrLocAddr, arg_str_pool); 
					p_item->WarehouseID = GetDynamicParamString(temp_buf.Z().Cat(p_src_item->ForeignLocID), arg_str_pool); // @v10.0.08
					THROW(p_item->DeliveryPositions = CreateDeliveryPositions(p_src_item->DeliveryList, arg_str_pool));
				}
			}
		}
	}
	THROW(PreprocessCall(proxi, rSess, proxi.DRP_USCORESendSellOut(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	p_result = PreprocessAnyResult(resp.DRP_USCORESendSellOutResult->__any);
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	for(uint i = 0; i < rList.getCount(); i++) {
		ns1__Invoice * p_inv = param._USCOREinvoices->Invoice[i];
		//
		if(p_inv->OrderDeliveries) {
			for(int j = 0; j < p_inv->OrderDeliveries->__sizeOrderDelivery; j++) {
				ns1__OrderDelivery * p_item = p_inv->OrderDeliveries->OrderDelivery[j];
				DestroyDeliveryPositions(p_item->DeliveryPositions);
				p_item->DeliveryPositions = 0;
				delete p_item;
			}
			ZFREE(p_inv->OrderDeliveries->OrderDelivery);
			ZDELETE(p_inv->OrderDeliveries);
		}
		if(p_inv->DistributorDeliveries) {
			for(int j = 0; j < p_inv->DistributorDeliveries->__sizeDistributorDelivery; j++) {
				ns1__DistributorDelivery * p_item = p_inv->DistributorDeliveries->DistributorDelivery[j];
				DestroyDeliveryPositions(p_item->DeliveryPositions);
				p_item->DeliveryPositions = 0;
				delete p_item;
			}
			ZFREE(p_inv->DistributorDeliveries->DistributorDelivery);
			ZDELETE(p_inv->DistributorDeliveries);
		}
		if(p_inv->DistributorSalePointDeliveries) {
			for(int j = 0; j < p_inv->DistributorSalePointDeliveries->__sizeDistributorSalePointDelivery; j++) {
				ns1__DistributorSalePointDelivery * p_item = p_inv->DistributorSalePointDeliveries->DistributorSalePointDelivery[j];
				DestroyDeliveryPositions(p_item->DeliveryPositions);
				p_item->DeliveryPositions = 0;
				delete p_item;
			}
			ZFREE(p_inv->DistributorSalePointDeliveries->DistributorSalePointDelivery);
			ZDELETE(p_inv->DistributorSalePointDeliveries);
		}
		//
		delete p_inv;
		param._USCOREinvoices->Invoice[i] = 0;
	}	
	ZFREE(param._USCOREinvoices->Invoice);
	ZDELETE(param._USCOREinvoices);
	return p_result;
}

extern "C" __declspec(dllexport) SString * SfaHeineken_SendOrdersStatuses(PPSoapClientSession & rSess, const TSCollection <SfaHeinekenOrderStatusEntry> & rList)
{
	//int DRP_USCORESendOrdersStatuses(_ns1__DRP_USCORESendOrdersStatuses *ns1__DRP_USCORESendOrdersStatuses, _ns1__DRP_USCORESendOrdersStatusesResponse *ns1__DRP_USCORESendOrdersStatusesResponse) { return DRP_USCORESendOrdersStatuses(NULL, NULL, ns1__DRP_USCORESendOrdersStatuses, ns1__DRP_USCORESendOrdersStatusesResponse); }
	SString * p_result = 0;
	SString temp_buf;
	DRPServiceSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__DRP_USCORESendOrdersStatuses param;
	_ns1__DRP_USCORESendOrdersStatusesResponse resp;
	param._USCORElogin = GetDynamicParamString(rSess.GetUser(), arg_str_pool);
	param._USCOREpass = GetDynamicParamString(rSess.GetPassword(), arg_str_pool);
	THROW(param._USCOREstatuses = new ns1__ArrayOfSendStatuses);
	param._USCOREstatuses->__sizeSendStatuses = (int)rList.getCount();
	THROW(param._USCOREstatuses->SendStatuses = (ns1__SendStatuses **)SAlloc::C(rList.getCount(), sizeof(ns1__SendStatuses *)));
	for(uint i = 0; i < rList.getCount(); i++) {
		const SfaHeinekenOrderStatusEntry * p_entry = rList.at(i);
		THROW(param._USCOREstatuses->SendStatuses[i] = new ns1__SendStatuses);
		p_entry->OrderUUID.ToStr(S_GUID::fmtIDL, temp_buf);
		param._USCOREstatuses->SendStatuses[i]->Number = GetDynamicParamString(temp_buf, arg_str_pool);
		param._USCOREstatuses->SendStatuses[i]->Status = (ns1__Statuses)p_entry->Status;
		param._USCOREstatuses->SendStatuses[i]->Comments = GetDynamicParamString(p_entry->Comments, arg_str_pool);
	}
	THROW(PreprocessCall(proxi, rSess, proxi.DRP_USCORESendOrdersStatuses(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	p_result = PreprocessAnyResult(resp.DRP_USCORESendOrdersStatusesResult->__any);
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	for(uint i = 0; i < rList.getCount(); i++) {
		delete param._USCOREstatuses->SendStatuses[i];
	}	
	ZFREE(param._USCOREstatuses->SendStatuses);
	ZDELETE(param._USCOREstatuses);
	return p_result;
}

int SfaHeineken_GetEncashments() // DRP_GetEncashments
{
	return -1;
}

int SfaHeineken_GetEncashmentsByDate() // DRP_GetEncashmentsByDate
{
	return -1;
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
	else if(checkdate(dt)) {
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

extern "C" __declspec(dllexport) SString * SfaHeineken_SendAllContragentDebet(PPSoapClientSession & rSess, const TSCollection <SfaHeinekenDebetEntry> & rList) // DRP_SendAllContragentDebet
{
	SString * p_result = 0;
	SString temp_buf;
	DRPServiceSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__DRP_USCORESendAllContragentDebet param;
	_ns1__DRP_USCORESendAllContragentDebetResponse resp;
	param._USCORElogin = GetDynamicParamString(rSess.GetUser(), arg_str_pool);
	param._USCOREpass = GetDynamicParamString(rSess.GetPassword(), arg_str_pool);
	THROW(param._USCOREcontragentDebets = new ns1__ArrayOfDistributorDebet);
	param._USCOREcontragentDebets->__sizeDistributorDebet = (int)rList.getCount();
	THROW(param._USCOREcontragentDebets->DistributorDebet = (ns1__DistributorDebet **)SAlloc::C(rList.getCount(), sizeof(ns1__DistributorDebet *)));
	for(uint i = 0; i < rList.getCount(); i++) {
		const SfaHeinekenDebetEntry * p_entry = rList.at(i);
		THROW(param._USCOREcontragentDebets->DistributorDebet[i] = new ns1__DistributorDebet);
		param._USCOREcontragentDebets->DistributorDebet[i]->ContragentID = p_entry->ContragentID;
		param._USCOREcontragentDebets->DistributorDebet[i]->DebetSum = p_entry->DebetSum;
		param._USCOREcontragentDebets->DistributorDebet[i]->DebetLimit = p_entry->DebetLimit;
	}
	THROW(PreprocessCall(proxi, rSess, proxi.DRP_USCORESendAllContragentDebet(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	p_result = PreprocessAnyResult(resp.DRP_USCORESendAllContragentDebetResult->__any);
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	for(uint i = 0; i < rList.getCount(); i++) {
		delete param._USCOREcontragentDebets->DistributorDebet[i];
	}	
	ZFREE(param._USCOREcontragentDebets->DistributorDebet);
	ZDELETE(param._USCOREcontragentDebets);
	return p_result;
}
