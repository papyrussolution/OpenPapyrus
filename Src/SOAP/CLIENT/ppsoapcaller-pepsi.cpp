// PPSOAPCALLER-PEPSI.CPP
// Copyright (c) A.Sobolev 2016
//
#include <ppsoapclient.h>
#include "pepsi\pepsiSoapAccountingTransferSoapProxy.h"

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch(dwReason) {
		case DLL_PROCESS_ATTACH:
			{
				SString product_name;
				(product_name = "Papyrus iSalesPepsiSoapModule");
				SLS.Init(product_name, (HINSTANCE)hModule);
			}
			break;
#ifdef _MT
		case DLL_THREAD_ATTACH:
			SLS.InitThread();
			break;
		case DLL_THREAD_DETACH:
			SLS.ReleaseThread();
			break;
#endif
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

/*
static SCollection __ResultPtrList;

static int DestroyResultPtr(void * p)
{
	int    ok = 0;
	uint c = __ResultPtrList.getCount();
	if(c) do {
		PPSoapResultPtrBase * p_item = (PPSoapResultPtrBase *)__ResultPtrList.at(--c);
		if(p_item && *p_item == p) {
			p_item->Destroy();
			__ResultPtrList.atFree(c);
			ok = 1;
		}
	} while(!ok && c);
	return ok;
}

template <class T> static void RegisterResultPtr(T * ptr)
{
	if(ptr) {
		PPSoapResultPtr <T> * p_item = new PPSoapResultPtr <T> (ptr);
		if(p_item)
			__ResultPtrList.insert(p_item);
	}
}
*/

extern "C" __declspec(dllexport) int iSalesDestroyResult(void * pResult)
{
	return pResult ? PPSoapDestroyResultPtr(pResult) : -1;
}

//typedef TSCollection <iSalesGoodsPacket> * (*ISALESGETGOODSLIST_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw);
//typedef int (*ISALESPUTSTOCKCOUNTING_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, TSCollection <iSalesStockCountingWhPacket> * pItems);

static void FASTCALL ProcessError(AccountingTransferSoapProxy & rProxi, PPSoapClientSession & rSess)
{
	char   temp_err_buf[1024];
	rProxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
	rSess.SetMsg(temp_err_buf);
}

static int PreprocessCall(AccountingTransferSoapProxy & rProxy, PPSoapClientSession & rSess, int result)
{
	if(result == SOAP_OK) {
		return 1;
	}
	else {
		ProcessError(rProxy, rSess);
		return 0;
	}
}

#if 0 // {
static char * FASTCALL GetDynamicParamString(const char * pSrc, TSCollection <InParamString> & rPool)
{
	InParamString * p_new_item = new InParamString(pSrc);
	if(p_new_item) {
		if(!rPool.insert(p_new_item)) {
			ZDELETE(p_new_item);
		}
	}
	return p_new_item ? (char *)*p_new_item : 0;
}

static char * FASTCALL GetDynamicParamString(long ival, TSCollection <InParamString> & rPool)
{
	SString temp_buf;
	return GetDynamicParamString(temp_buf.Cat(ival), rPool);
}

static char * FASTCALL GetDynamicParamString(int ival, TSCollection <InParamString> & rPool)
{
	SString temp_buf;
	return GetDynamicParamString(temp_buf.Cat((long)ival), rPool);
}

static char * FASTCALL GetDynamicParamString(double rval, long fmt, TSCollection <InParamString> & rPool)
{
	SString temp_buf;
	temp_buf.Cat(rval, fmt);
	temp_buf.ReplaceChar('.', ',');
	return GetDynamicParamString(temp_buf, rPool);
}

static char * FASTCALL GetDynamicParamString(LDATE dval, long fmt, TSCollection <InParamString> & rPool)
{
	SString temp_buf;
	if(checkdate(dval, 0))
		temp_buf.Cat(dval, fmt);
	else
		temp_buf = 0;
	return GetDynamicParamString(temp_buf, rPool);
}

static char * FASTCALL GetDynamicParamString(LDATETIME dtval, long dfmt, long tfmt, TSCollection <InParamString> & rPool)
{
	SString temp_buf;
	if(checkdate(dtval.d, 0))
		temp_buf.Cat(dtval, dfmt, tfmt);
	else
		temp_buf = 0;
	return GetDynamicParamString(temp_buf, rPool);
}
#endif // } 0

extern "C" __declspec(dllexport) TSCollection <iSalesBillDebt> * iSalesGetUnclosedBillList(PPSoapClientSession & rSess,
	const char * pUser, const char * pPassw, const DateRange * pPeriod)
{
	TSCollection <iSalesBillDebt> * p_result = 0;
	AccountingTransferSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__UnclosedDocumentsTransfer param;
	_ns1__UnclosedDocumentsTransferResponse resp;
	SString temp_buf;
	param.username = GetDynamicParamString(pUser, arg_str_pool);
	param.password = GetDynamicParamString(pPassw, arg_str_pool);
	DateRange period;
	if(!RVALUEPTR(period, pPeriod))
		period.SetZero();
	SETIFZ(period.low, encodedate(1, 1, 2016));
	SETIFZ(period.upp, encodedate(31, 12, 2030));
	param.dtFrom = GetDynamicParamString(period.low, DATF_YMD|DATF_CENTURY|DATF_NODIV, arg_str_pool);
	param.dtTo = GetDynamicParamString(period.upp, DATF_YMD|DATF_CENTURY|DATF_NODIV, arg_str_pool);
	THROW(PreprocessCall(proxi, rSess, proxi.UnclosedDocumentsTransfer(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	if(resp.UnclosedDocumentsTransferResult && resp.UnclosedDocumentsTransferResult->__sizeDEBT > 0) {
		THROW(p_result = new TSCollection <iSalesBillDebt>);
		PPSoapRegisterResultPtr(p_result);
		for(int i = 0; i < resp.UnclosedDocumentsTransferResult->__sizeDEBT; i++) {
			const ns1__DEBT * p_doc = resp.UnclosedDocumentsTransferResult->DEBT[i];
			if(p_doc) {
				iSalesBillDebt * p_new_item = p_result->CreateNewItem(0);
				THROW(p_new_item);
				p_new_item->iSalesId = (temp_buf = p_doc->ISALES_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->DocType = (temp_buf = p_doc->DOC_USCORETP).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
				p_new_item->Code = (temp_buf = p_doc->DOC_USCORENO).Transf(CTRANSF_UTF8_TO_INNER);
				(temp_buf = p_doc->DOC_USCOREDT).Transf(CTRANSF_UTF8_TO_INNER);
				strtodatetime(temp_buf, &p_new_item->Dtm, DATF_DMY, TIMF_HMS);
				p_new_item->PayerCode = (temp_buf = p_doc->CUST_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->Amount = (temp_buf = p_doc->GROSS_USCORESUM).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
				p_new_item->Debt = (temp_buf = p_doc->DEBT).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <iSalesRoutePacket> * iSalesGetRouteList(PPSoapClientSession & rSess,
	const char * pUser, const char * pPassw, const DateRange * pPeriod)
{
	TSCollection <iSalesRoutePacket> * p_result = 0;
	AccountingTransferSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__RoutesTransfer param;
	_ns1__RoutesTransferResponse resp;
	SString temp_buf;
	param.username = GetDynamicParamString(pUser, arg_str_pool);
	param.password = GetDynamicParamString(pPassw, arg_str_pool);
	DateRange period;
	if(!RVALUEPTR(period, pPeriod))
		period.SetZero();
	SETIFZ(period.low, encodedate(1, 1, 2016));
	SETIFZ(period.upp, encodedate(31, 12, 2030));
	param.dtFrom = GetDynamicParamString(period.low, DATF_YMD|DATF_CENTURY|DATF_NODIV, arg_str_pool);
	param.dtTo = GetDynamicParamString(period.upp, DATF_YMD|DATF_CENTURY|DATF_NODIV, arg_str_pool);
	param.OnlyActive = GetDynamicParamString(0L, arg_str_pool);
	THROW(PreprocessCall(proxi, rSess, proxi.RoutesTransfer(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	if(resp.RoutesTransferResult && resp.RoutesTransferResult->__sizeROUTE > 0) {
		THROW(p_result = new TSCollection <iSalesRoutePacket>);
		PPSoapRegisterResultPtr(p_result);
		for(int i = 0; i < resp.RoutesTransferResult->__sizeROUTE; i++) {
			const ns1__ROUTE * p_route = resp.RoutesTransferResult->ROUTE[i];
			if(p_route) {
				iSalesRoutePacket * p_new_item = p_result->CreateNewItem(0);
				THROW(p_new_item);
				p_new_item->Ident = (temp_buf = p_route->ISALES_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->Code = (temp_buf = p_route->ROUTE_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->NativeAgentCode = (temp_buf = p_route->SR_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->TypeOfRoute = (temp_buf = p_route->ROUTE_USCORETP).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
				p_new_item->Valid = (temp_buf = p_route->VALID).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
				p_new_item->ErrMsg = (temp_buf = p_route->ErrorMessage).Transf(CTRANSF_UTF8_TO_INNER);
				if(p_route->VISITS && p_route->VISITS->__sizeVISIT > 0) {
					for(int j = 0; j < p_route->VISITS->__sizeVISIT; j++) {
						const ns1__VISIT * p_visit = p_route->VISITS->VISIT[j];
						if(p_visit) {
							iSalesVisit * p_new_visit = p_new_item->VisitList.CreateNewItem(0);
							THROW(p_new_visit);
							p_new_visit->Ident = (temp_buf = p_visit->VISIT_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_visit->OuterClientCode = (temp_buf = p_visit->SFA_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_visit->InnerClientCode = (temp_buf = p_visit->ACC_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_visit->Valid = (temp_buf = p_visit->VALID).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
							p_new_visit->InitDate = strtodate_((temp_buf = p_visit->INIT_USCOREDT).Transf(CTRANSF_UTF8_TO_INNER), DATF_DMY);
							p_new_visit->Freq = (temp_buf = p_visit->WEEK_USCORENO).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
							p_new_visit->DayOfWeek = (temp_buf = p_visit->DAY_USCORENO).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
							p_new_visit->Order = (temp_buf = p_visit->ORDER).Transf(CTRANSF_UTF8_TO_INNER);
						}
					}
				}
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <iSalesGoodsPacket> * iSalesGetGoodsList(PPSoapClientSession & rSess, const char * pUser, const char * pPassw)
{
	TSCollection <iSalesGoodsPacket> * p_result = 0;
	AccountingTransferSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	InParamString arg_name(pUser);
	InParamString arg_pw(pPassw);
	gSoapClientInit(&proxi, 0, 0);
	_ns1__ProductsTransfer param;
	_ns1__ProductsTransferResponse resp;
	SString temp_buf;
	param.username = arg_name;
	param.password = arg_pw;
	DateRange period;
	//period.Set(encodedate(1, 1, 2001), encodedate(31, 12, 2030));
	//period.Set(encodedate(1, 7, 2016), encodedate(20, 7, 2016));
	period.Set(encodedate(1, 1, 2016), encodedate(20, 7, 2030));
	InParamString arg_dtstart((temp_buf = 0).Cat(period.low, DATF_YMD|DATF_CENTURY|DATF_NODIV));
	InParamString arg_dtend((temp_buf = 0).Cat(period.upp, DATF_YMD|DATF_CENTURY|DATF_NODIV));
	param.dtFrom = arg_dtstart;
	param.dtTo = arg_dtend;
	THROW(PreprocessCall(proxi, rSess, proxi.ProductsTransfer(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	if(resp.ProductsTransferResult && resp.ProductsTransferResult->__sizePRODUCT > 0) {
		THROW(p_result = new TSCollection <iSalesGoodsPacket>);
		PPSoapRegisterResultPtr(p_result);
		for(int i = 0; i < resp.ProductsTransferResult->__sizePRODUCT; i++) {
			const ns1__PRODUCT * p_product = resp.ProductsTransferResult->PRODUCT[i];
			if(p_product) {
				iSalesGoodsPacket * p_new_item = p_result->CreateNewItem(0);
				THROW(p_new_item);
				p_new_item->TypeOfProduct = (temp_buf = p_product->PROD_USCORETYPE).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
				p_new_item->OuterCode = (temp_buf = p_product->SFA_USCOREPRD_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->NativeCode = (temp_buf = p_product->ACC_USCOREPRD_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->Name = (temp_buf = p_product->FL_USCORENM).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->Abbr = (temp_buf = p_product->BF_USCORENM).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->VatRate = (temp_buf = p_product->VAT).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
				p_new_item->Valid = (temp_buf = p_product->VALID).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
				p_new_item->UnitCode = (temp_buf = p_product->MIN_USCOREUOM).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
				p_new_item->CountryName = (temp_buf = p_product->COUNTRY_USCOREPRODUCER).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->CLB = (temp_buf = p_product->CUSTOMS_USCOREDECLARATION).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->ErrMsg = (temp_buf = p_product->ErrorMessage).Transf(CTRANSF_UTF8_TO_INNER);
				if(p_product->UOMS && p_product->UOMS->__sizeUOM > 0) {
					for(int j = 0; j < p_product->UOMS->__sizeUOM; j++) {
						const ns1__UOM * p_uom = p_product->UOMS->UOM[j];
						if(p_uom) {
							iSalesUOM * p_new_uom = p_new_item->UomList.CreateNewItem(0);
							THROW(p_new_uom);
							p_new_uom->Code = (temp_buf = p_uom->UOM_USCOREID).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
							p_new_uom->Width = (temp_buf = p_uom->WI).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_uom->Height = (temp_buf = p_uom->HE).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_uom->Length = (temp_buf = p_uom->LE).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_uom->Netto = (temp_buf = p_uom->WE).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_uom->Brutto = (temp_buf = p_uom->WEB).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_uom->Barcode = (temp_buf = p_uom->BAR).Transf(CTRANSF_UTF8_TO_INNER);
						}
					}
				}
				if(p_product->CONVS && p_product->CONVS->__sizeCONV > 0) {
					for(int j = 0; j < p_product->CONVS->__sizeCONV; j++) {
						const ns1__CONV * p_cvt = p_product->CONVS->CONV[j];
						if(p_cvt) {
							iSalesUOMCvt * p_new_cvt = p_new_item->CvtList.CreateNewItem(0);
							THROW(p_new_cvt);
							p_new_cvt->UomFrom = (temp_buf = p_cvt->FROM).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_cvt->UomTo = (temp_buf = p_cvt->TO_USCORE).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_cvt->Rate = (temp_buf = p_cvt->FACTOR).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
						}
					}
				}
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

static TSCollection <iSalesBillPacket> * Helper_AcceptDocList(const ns1__ArrayOfDOC * pSrcList)
{
	TSCollection <iSalesBillPacket> * p_result = 0;
	SString temp_buf;
	THROW(p_result = new TSCollection <iSalesBillPacket>);
	PPSoapRegisterResultPtr(p_result);
	for(int i = 0; i < pSrcList->__sizeDOC; i++) {
		const ns1__DOC * p_ord = pSrcList->DOC[i];
		if(p_ord) {
			const int doc_type = (temp_buf = p_ord->DOC_USCORETP).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
			if(oneof2(doc_type, 6, 13)) { // Заказ от клиента или приход от поставщика
				iSalesBillPacket * p_new_order = p_result->CreateNewItem(0);
				THROW(p_new_order);
				p_new_order->DocType = doc_type;
				p_new_order->iSalesId = (temp_buf = p_ord->ISALES_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_order->Code = (temp_buf = p_ord->DOC_USCORENO).Transf(CTRANSF_UTF8_TO_INNER);
				(temp_buf = p_ord->DOC_USCOREDT).Transf(CTRANSF_UTF8_TO_INNER);
				strtodatetime(temp_buf, &p_new_order->Dtm, DATF_DMY, TIMF_HMS);
				p_new_order->Status = (temp_buf = p_ord->STATUS).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
				p_new_order->SellerCode = (temp_buf = p_ord->SELLER).Transf(CTRANSF_UTF8_TO_INNER); // Код дистрибьютора
				p_new_order->PayerCode = (temp_buf = p_ord->PAYER).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_order->AgentCode = (temp_buf = p_ord->SR_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_order->Memo = (temp_buf = p_ord->COMMS).Transf(CTRANSF_UTF8_TO_INNER);
				(temp_buf = p_ord->DUE_USCOREDATE).Transf(CTRANSF_UTF8_TO_INNER);
				strtodate(temp_buf, DATF_DMY, &p_new_order->DueDate);
				p_new_order->ShipFrom = (temp_buf = p_ord->SHIP_USCOREFROM).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_order->ShipTo = (temp_buf = p_ord->SHIP_USCORETO).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_order->SrcLocCode = (temp_buf = p_ord->WHS_USCORESRC).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_order->DestLocCode = (temp_buf = p_ord->WHS_USCOREDST).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_order->AuthId = (temp_buf = p_ord->AUTH_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_order->EditId = (temp_buf = p_ord->EDIT_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
				(temp_buf = p_ord->CREATE_USCOREDT).Transf(CTRANSF_UTF8_TO_INNER);
				strtodatetime(temp_buf, &p_new_order->CreationDtm, DATF_DMY, TIMF_HMS);
				(temp_buf = p_ord->EDIT_USCOREDT).Transf(CTRANSF_UTF8_TO_INNER);
				strtodatetime(temp_buf, &p_new_order->LastUpdDtm, DATF_DMY, TIMF_HMS);
				// @v9.2.6 {
				(temp_buf = p_ord->INC_USCOREDT).Transf(CTRANSF_UTF8_TO_INNER);
				strtodatetime(temp_buf, &p_new_order->IncDtm, DATF_DMY, TIMF_HMS);
				// } @v9.2.6
				p_new_order->ErrMsg = (temp_buf = p_ord->ErrorMessage).Transf(CTRANSF_UTF8_TO_INNER);
				if(p_ord->DOC_USCOREITEMS && p_ord->DOC_USCOREITEMS->__sizeDOC_USCOREITEM > 0) {
					for(int j = 0; j < p_ord->DOC_USCOREITEMS->__sizeDOC_USCOREITEM; j++) {
						const ns1__DOC_USCOREITEM * p_item = p_ord->DOC_USCOREITEMS->DOC_USCOREITEM[j];
						if(p_item) {
							iSalesBillItem * p_new_item = p_new_order->Items.CreateNewItem(0);
							THROW(p_new_item);
							p_new_item->LineN = (temp_buf = p_item->LINE_USCORENO).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
							p_new_item->OuterGoodsCode = (temp_buf = p_item->SFA_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_item->NativeGoodsCode = (temp_buf = p_item->ACC_USCOREID).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_item->Qtty = (temp_buf = p_item->QUANTITY).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_item->UnitCode = (temp_buf = p_item->UOM_USCOREID).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
							p_new_item->Memo = (temp_buf = p_item->DESC).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_item->Country = (temp_buf = p_item->COUNTRY).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_item->CLB = (temp_buf = p_item->CUSTOMS).Transf(CTRANSF_UTF8_TO_INNER);
							if(p_item->SETS && p_item->SETS->__sizeDSET > 0) {
								for(int k = 0; k < p_item->SETS->__sizeDSET; k++) {
									const ns1__DSET * p_set = p_item->SETS->DSET[k];
									if(p_set) {
										iSalesBillAmountEntry * p_new_amt = p_new_item->Amounts.CreateNewItem(0);
										THROW(p_new_amt);
										p_new_amt->SetType = (temp_buf = p_set->SET_USCORETYPE).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
										p_new_amt->NetPrice = (temp_buf = p_set->NET_USCOREPR).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
										p_new_amt->NetSum = (temp_buf = p_set->NET_USCORESUM).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
										p_new_amt->GrossPrice = (temp_buf = p_set->GROSS_USCOREPR).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
										p_new_amt->GrossSum = (temp_buf = p_set->GROSS_USCORESUM).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
										p_new_amt->VatSum = (temp_buf = p_set->VAT_USCORESUM).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
										p_new_amt->DiscNetSum = (temp_buf = p_set->DISC_USCORENET_USCORESUM).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
										p_new_amt->DiscGrossSum = (temp_buf = p_set->DISC_USCOREGR_USCOREAMOUNT).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
										p_new_amt->DiscAmount = (temp_buf = p_set->DISCOUNT_USCOREAMOUNT).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
									}
								}
							}
						}
					}
				}
				if(p_ord->SUM_USCORESETS && p_ord->SUM_USCORESETS->__sizeHSET > 0) {
					for(int j = 0; j < p_ord->SUM_USCORESETS->__sizeHSET; j++) {
						const ns1__HSET * p_set = p_ord->SUM_USCORESETS->HSET[j];
						if(p_set) {
							iSalesBillAmountEntry * p_new_amt = p_new_order->Amounts.CreateNewItem(0);
							THROW(p_new_amt);
							p_new_amt->SetType = (temp_buf = p_set->SET_USCORETYPE).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
							p_new_amt->NetSum = (temp_buf = p_set->NET).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_amt->GrossSum = (temp_buf = p_set->GROSS).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_amt->VatSum = (temp_buf = p_set->VAT).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_amt->DiscGrossSum = (temp_buf = p_set->DISC_USCOREGR_USCORESUM).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_amt->DiscAmount = (temp_buf = p_set->DISC_USCORESUM).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
						}
					}
				}
				// @v9.3.1 {
				if(p_ord->ATTRS && p_ord->ATTRS->__sizeATTR > 0) {
					for(int j = 0; j < p_ord->ATTRS->__sizeATTR; j++) {
						const ns1__ATTR * p_attr = p_ord->ATTRS->ATTR[j];
						if(p_attr) {
							iSalesExtAttr * p_new_attr = p_new_order->Attrs.CreateNewItem(0);
							THROW(p_new_attr);
							p_new_attr->Name = (temp_buf = p_attr->ATTR_USCORENAME).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_attr->Value = (temp_buf = p_attr->ATTR_USCOREVALUE).Transf(CTRANSF_UTF8_TO_INNER);
						}
					}
				}
				// } @v9.3.1 
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <iSalesBillPacket> * iSalesGetReceiptList(PPSoapClientSession & rSess,
	const char * pUser, const char * pPassw, const DateRange * pPeriod, int inclProcessedItems)
{
	TSCollection <iSalesBillPacket> * p_result = 0;
	AccountingTransferSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__DocumentsTransfer  param;
	_ns1__DocumentsTransferResponse resp;
	SString temp_buf;
	param.username = GetDynamicParamString(pUser, arg_str_pool);
	param.password = GetDynamicParamString(pPassw, arg_str_pool);
	DateRange period;
	if(!RVALUEPTR(period, pPeriod))
		period.SetZero();
	SETIFZ(period.low, encodedate(1, 1, 2001));
	SETIFZ(period.upp, encodedate(31, 12, 2030));
	param.dtFrom = GetDynamicParamString(period.low, DATF_YMD|DATF_CENTURY|DATF_NODIV, arg_str_pool);
	param.dtTo = GetDynamicParamString(period.upp, DATF_YMD|DATF_CENTURY|DATF_NODIV, arg_str_pool);
	param.includeAlreadyProcessedItems = GetDynamicParamString(inclProcessedItems, arg_str_pool);
	param.docTypes = GetDynamicParamString("6", arg_str_pool);
	param.docStatuses = GetDynamicParamString("2", arg_str_pool); // 0 - выписан; 1 - отменен; 2 - черновик
	THROW(PreprocessCall(proxi, rSess, proxi.DocumentsTransfer(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	if(resp.DocumentsTransferResult && resp.DocumentsTransferResult->__sizeDOC > 0) {
		p_result = Helper_AcceptDocList(resp.DocumentsTransferResult);
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <iSalesBillPacket> * iSalesGetOrderList(PPSoapClientSession & rSess,
	const char * pUser, const char * pPassw, const DateRange * pPeriod, int inclProcessedItems)
{
	TSCollection <iSalesBillPacket> * p_result = 0;
	AccountingTransferSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__OrdersTransfer param;
	_ns1__OrdersTransferResponse resp;
	SString temp_buf;
	param.username = GetDynamicParamString(pUser, arg_str_pool);
	param.password = GetDynamicParamString(pPassw, arg_str_pool);
	DateRange period;
	if(!RVALUEPTR(period, pPeriod))
		period.SetZero();
	SETIFZ(period.low, encodedate(1, 1, 2001));
	SETIFZ(period.upp, encodedate(31, 12, 2030));
	param.dtFrom = GetDynamicParamString(period.low, DATF_YMD|DATF_CENTURY|DATF_NODIV, arg_str_pool);
	param.dtTo = GetDynamicParamString(period.upp, DATF_YMD|DATF_CENTURY|DATF_NODIV, arg_str_pool);
	param.includeAlreadyProcessedItems = GetDynamicParamString(inclProcessedItems, arg_str_pool);
	THROW(PreprocessCall(proxi, rSess, proxi.OrdersTransfer(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	if(resp.OrdersTransferResult && resp.OrdersTransferResult->__sizeDOC > 0) {
		p_result = Helper_AcceptDocList(resp.OrdersTransferResult);
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

#if 0 // {
static void ** FASTCALL /*_CreateSoapArray*/PPSoapCreateArray(uint count, int & rArrayCount)
{
	void ** pp_list = count ? (void **)calloc(count, sizeof(void *)) : 0;
	rArrayCount = (int)count;
	return pp_list;
}
#endif // } 0

extern "C" __declspec(dllexport) SString * iSalesPutTransferStatus(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const TSCollection <iSalesTransferStatus> * pItems)
{
	SString * p_result = 0;
	SString reply_msg;
	SString temp_buf;
	AccountingTransferSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	InParamString arg_name(pUser);
	InParamString arg_pw(pPassw);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__UpdateTransferStatus param;
	_ns1__UpdateTransferStatusResponse resp;

	TSCollection <ns1__pTransferStatus> arg_status_list;
	ns1__ArrayOfPTransferStatus scl;

	if(pItems && pItems->getCount()) {
		param.username = arg_name;
		param.password = arg_pw;
		param.documents = &scl;
		for(uint i = 0; i < pItems->getCount(); i++) {
			const iSalesTransferStatus * p_src_pack = pItems->at(i);
			if(p_src_pack) {
				switch(p_src_pack->Ifc) {
					case iSalesTransferStatus::ifcOrder: temp_buf = "ORDS"; break;
					case iSalesTransferStatus::ifcInvoice: temp_buf = "DOCL"; break;
					case iSalesTransferStatus::ifcReceipt: temp_buf = "DOCS"; break;
					default: temp_buf = 0; break;
				}
				if(temp_buf.NotEmpty()) {
					ns1__pTransferStatus * p_new_item = arg_status_list.CreateNewItem(0);
					THROW(p_new_item);
					p_new_item->DOC_USCORETYPE = GetDynamicParamString(temp_buf, arg_str_pool);
					p_new_item->DOC_USCOREID = GetDynamicParamString(p_src_pack->Ident, arg_str_pool);
				}
			}
		}
		{
			param.documents->pTransferStatus = (ns1__pTransferStatus **)PPSoapCreateArray(arg_status_list.getCount(), param.documents->__sizepTransferStatus);
			for(uint j = 0; j < arg_status_list.getCount(); j++) {
				param.documents->pTransferStatus[j] = arg_status_list.at(j);
			}
		}
		THROW(PreprocessCall(proxi, rSess, proxi.UpdateTransferStatus(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		{
			THROW(p_result = new SString);
			PPSoapRegisterResultPtr(p_result);
			(*p_result = resp.UpdateTransferStatusResult).Transf(CTRANSF_UTF8_TO_INNER);
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	ZFREE(param.documents->pTransferStatus);
	return p_result;
}

extern "C" __declspec(dllexport) SString * iSalesPutDebtSettlement(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const TSCollection <iSalesBillDebt> * pItems)
{
	SString * p_result = 0;
	SString reply_msg;
	SString temp_buf;
	AccountingTransferSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	InParamString arg_name(pUser);
	InParamString arg_pw(pPassw);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__DebtSettlementUpdatesTransfer param;
	_ns1__DebtSettlementUpdatesTransferResponse resp;

	TSCollection <ns1__DEBT> arg_item_list;
	ns1__ArrayOfDEBT scl;

	if(pItems && pItems->getCount()) {
		param.username = arg_name;
		param.password = arg_pw;
		param.debtSettlements = &scl;
		for(uint i = 0; i < pItems->getCount(); i++) {
			const iSalesBillDebt * p_src_pack = pItems->at(i);
			if(p_src_pack) {
				ns1__DEBT * p_new_item = arg_item_list.CreateNewItem(0);
				THROW(p_new_item);
				p_new_item->ISALES_USCOREID = GetDynamicParamString(p_src_pack->iSalesId, arg_str_pool);
				p_new_item->DOC_USCORETP = GetDynamicParamString(p_src_pack->DocType, arg_str_pool);
				p_new_item->DOC_USCORENO = GetDynamicParamString(p_src_pack->Code, arg_str_pool);
				p_new_item->DOC_USCOREDT = GetDynamicParamString(p_src_pack->Dtm, DATF_GERMAN|DATF_CENTURY, TIMF_HMS, arg_str_pool);
				p_new_item->CUST_USCOREID = GetDynamicParamString(p_src_pack->PayerCode, arg_str_pool);
				p_new_item->GROSS_USCORESUM = GetDynamicParamString_(p_src_pack->Amount, MKSFMTD(0, 2, NMBF_DECCOMMA), arg_str_pool);
				p_new_item->DEBT = GetDynamicParamString_(p_src_pack->Debt, MKSFMTD(0, 2, NMBF_DECCOMMA), arg_str_pool);
			}
		}
		{
			param.debtSettlements->DEBT = (ns1__DEBT **)PPSoapCreateArray(arg_item_list.getCount(), param.debtSettlements->__sizeDEBT);
			for(uint j = 0; j < arg_item_list.getCount(); j++) {
				param.debtSettlements->DEBT[j] = arg_item_list.at(j);
			}
		}
		THROW(PreprocessCall(proxi, rSess, proxi.DebtSettlementUpdatesTransfer(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		{
			THROW(p_result = new SString);
			PPSoapRegisterResultPtr(p_result);
			(*p_result = resp.DebtSettlementUpdatesTransferResult).Transf(CTRANSF_UTF8_TO_INNER);
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	ZFREE(param.debtSettlements->DEBT);
	return p_result;
}

extern "C" __declspec(dllexport) SString * iSalesPutStockCounting(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const TSCollection <iSalesStockCountingWhPacket> * pItems)
{
	//int StockCountingTransfer(const char *endpoint, const char *soap_action, _ns1__StockCountingTransfer *ns1__StockCountingTransfer, _ns1__StockCountingTransferResponse *ns1__StockCountingTransferResponse);
	SString * p_result = 0;
	SString reply_msg;
	SString temp_buf;
	AccountingTransferSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	InParamString arg_name(pUser);
	InParamString arg_pw(pPassw);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__StockCountingTransfer param;
	_ns1__StockCountingTransferResponse resp;

	TSCollection <ns1__WAR> arg_war_list;
	TSCollection <ns1__ArrayOfPROD> arg_prodlh_list;
	TSCollection <ns1__PROD> arg_prod_list;
	ns1__ArrayOfWAR scl;

	if(pItems && pItems->getCount()) {
		param.username = arg_name;
		param.password = arg_pw;
		param.stockCountingList = &scl;
		for(uint i = 0; i < pItems->getCount(); i++) {
			const iSalesStockCountingWhPacket * p_src_pack = pItems->at(i);
			if(p_src_pack) {
				ns1__WAR * p_new_war_item = arg_war_list.CreateNewItem(0);
				ns1__ArrayOfPROD * p_new_prodlh = arg_prodlh_list.CreateNewItem(0);
				THROW(p_new_war_item);
				THROW(p_new_prodlh);
				{
					uint   j;
					p_new_war_item->WAR_USCOREID = GetDynamicParamString(p_src_pack->WhID, arg_str_pool);
					p_new_war_item->PRODS = p_new_prodlh;
					const uint start_prod_list_pos = arg_prod_list.getCount();
					for(j = 0; j < p_src_pack->Items.getCount(); j++) {
						const iSalesStockCountingItem * p_src_stock_item = p_src_pack->Items.at(j);
						if(p_src_stock_item) {
							ns1__PROD * p_new_prod_item = arg_prod_list.CreateNewItem(0);
							THROW(p_new_prod_item);
							p_new_prod_item->PROD_USCOREID = GetDynamicParamString(p_src_stock_item->OuterCode, arg_str_pool);
							p_new_prod_item->STOCK_USCORETP = GetDynamicParamString(p_src_stock_item->Type, arg_str_pool);
							p_new_prod_item->UOM = GetDynamicParamString(p_src_stock_item->UnitCode, arg_str_pool);
							p_new_prod_item->AMOUNT = GetDynamicParamString_(p_src_stock_item->Qtty, MKSFMTD(0, 0, NMBF_DECCOMMA), arg_str_pool);
						}
					}
					p_new_war_item->PRODS->PROD = (ns1__PROD **)PPSoapCreateArray(arg_prod_list.getCount()-start_prod_list_pos, p_new_war_item->PRODS->__sizePROD);
					for(j = start_prod_list_pos; j < arg_prod_list.getCount(); j++)
						p_new_war_item->PRODS->PROD[j-start_prod_list_pos] = arg_prod_list.at(j);
				}
			}
		}
		{
			param.stockCountingList->WAR = (ns1__WAR **)PPSoapCreateArray(arg_war_list.getCount(), param.stockCountingList->__sizeWAR);
			for(uint j = 0; j < arg_war_list.getCount(); j++) {
				param.stockCountingList->WAR[j] = arg_war_list.at(j);
			}
		}
		THROW(PreprocessCall(proxi, rSess, proxi.StockCountingTransfer(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		{
			THROW(p_result = new SString);
			PPSoapRegisterResultPtr(p_result);
			(*p_result = resp.StockCountingTransferResult).Transf(CTRANSF_UTF8_TO_INNER);
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	for(uint i = 0; i < arg_war_list.getCount(); i++) {
		ns1__WAR * p_item = arg_war_list.at(i);
		if(p_item && p_item->PRODS) {
			ZFREE(p_item->PRODS->PROD);
		}
	}
	ZFREE(param.stockCountingList->WAR);
	return p_result;
}

extern "C" __declspec(dllexport) SString * iSalesPutPrices(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const TSCollection <iSalesPriceListPacket> * pItems)
{
	//int StockCountingTransfer(const char *endpoint, const char *soap_action, _ns1__StockCountingTransfer *ns1__StockCountingTransfer, _ns1__StockCountingTransferResponse *ns1__StockCountingTransferResponse);
	SString * p_result = 0;
	SString temp_buf;
	AccountingTransferSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	InParamString arg_name(pUser);
	InParamString arg_pw(pPassw);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__CustomerPricesTransfer param;
	_ns1__CustomerPricesTransferResponse resp;
	ns1__ArrayOfPL param_list_hdr;

	TSCollection <ns1__PL> arg_pl_list;
	TSCollection <ns1__PRC> arg_prc_list;
	TSCollection <ns1__CUST> arg_cust_list;
	TSCollection <ns1__ArrayOfPRC> arg_prclh_list;
	TSCollection <ns1__ArrayOfCUST> arg_custlh_list;
	uint   i, j;
	if(pItems && pItems->getCount()) {
		param.username = arg_name;
		param.password = arg_pw;
		param.customerPrices = &param_list_hdr;
        for(i = 0; i < pItems->getCount(); i++) {
			const iSalesPriceListPacket * p_src_pack = pItems->at(i);
			if(p_src_pack) {
				ns1__PL * p_new_pl = arg_pl_list.CreateNewItem(0);
				THROW(p_new_pl);
				p_new_pl->PL_USCOREID = GetDynamicParamString(p_src_pack->PriceListID, arg_str_pool);
				p_new_pl->ISDEF = GetDynamicParamString((i == 0) ? 0L : 1L, arg_str_pool);
				p_new_pl->VALID = GetDynamicParamString(0L, arg_str_pool);
				const uint start_prc_list_pos = arg_prc_list.getCount();
				const uint start_cust_list_pos = arg_cust_list.getCount();
                for(j = 0; j < p_src_pack->Prices.getCount(); j++) {
					const iSalesPriceItem * p_src_item = p_src_pack->Prices.at(j);
					if(p_src_item) {
						ns1__PRC * p_new_prc = arg_prc_list.CreateNewItem(0);
						THROW(p_new_prc);
                        p_new_prc->PRD_USCOREID = GetDynamicParamString(p_src_item->OuterCode, arg_str_pool);
                        p_new_prc->NET = GetDynamicParamString_(p_src_item->Price, MKSFMTD(0, 2, NMBF_DECCOMMA), arg_str_pool);
					}
                }
				{
                    ns1__ArrayOfPRC * p_new_lh = arg_prclh_list.CreateNewItem(0);
                    THROW(p_new_lh);
                    p_new_pl->PRICES = p_new_lh;
					p_new_pl->PRICES->PRC = (ns1__PRC **)PPSoapCreateArray(arg_prc_list.getCount()-start_prc_list_pos, p_new_pl->PRICES->__sizePRC);
					for(j = start_prc_list_pos; j < arg_prc_list.getCount(); j++)
						p_new_pl->PRICES->PRC[j-start_prc_list_pos] = arg_prc_list.at(j);
				}
				{
					const uint cc = p_src_pack->OuterCliCodeList.getCount();
					uint   cn = 0;
                    for(j = 0; p_src_pack->OuterCliCodeList.get(&j, temp_buf);) {
						ns1__CUST * p_new_cust = arg_cust_list.CreateNewItem(0);
						THROW(p_new_cust);
						p_new_cust->CUST_USCOREID = GetDynamicParamString(temp_buf, arg_str_pool);
					}
					{
						ns1__ArrayOfCUST * p_new_lh = arg_custlh_list.CreateNewItem(0);
						THROW(p_new_lh);
						p_new_pl->CUSTS = p_new_lh;
						p_new_pl->CUSTS->CUST = (ns1__CUST **)PPSoapCreateArray(arg_cust_list.getCount()-start_cust_list_pos, p_new_pl->CUSTS->__sizeCUST);
						for(j = start_cust_list_pos; j < arg_cust_list.getCount(); j++)
							p_new_pl->CUSTS->CUST[j-start_cust_list_pos] = arg_cust_list.at(j);
					}
				}
			}
        }
        param.customerPrices->PL = (ns1__PL **)PPSoapCreateArray(arg_pl_list.getCount(), param.customerPrices->__sizePL);
        for(i = 0; i < arg_pl_list.getCount(); i++) {
			param.customerPrices->PL[i] = arg_pl_list.at(i);
        }
		THROW(PreprocessCall(proxi, rSess, proxi.CustomerPricesTransfer(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		{
			THROW(p_result = new SString);
			PPSoapRegisterResultPtr(p_result);
			(*p_result = resp.CustomerPricesTransferResult).Transf(CTRANSF_UTF8_TO_INNER);
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	for(i = 0; i < arg_pl_list.getCount(); i++) {
		ns1__PL * p_item = arg_pl_list.at(i);
		if(p_item) {
			if(p_item->PRICES) {
				ZFREE(p_item->PRICES->PRC);
			}
			if(p_item->CUSTS) {
				ZFREE(p_item->CUSTS->CUST);
			}
		}
	}
	ZFREE(param.customerPrices->PL);
	return p_result;
}

extern "C" __declspec(dllexport) SString * iSalesPutBills(PPSoapClientSession & rSess, const char * pUser, const char * pPassw,
	const TSCollection <iSalesBillPacket> * pItems, uint maxItems)
{
	//int StockCountingTransfer(const char *endpoint, const char *soap_action, _ns1__StockCountingTransfer *ns1__StockCountingTransfer, _ns1__StockCountingTransferResponse *ns1__StockCountingTransferResponse);
	//InvoiceWaybillTransfer
	SString * p_result = 0;
	SString temp_buf;
	AccountingTransferSoapProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	InParamString arg_name(pUser);
	InParamString arg_pw(pPassw);
	TSCollection <InParamString> arg_str_pool;
	gSoapClientInit(&proxi, 0, 0);
	_ns1__InvoiceWaybillTransfer param;
	_ns1__InvoiceWaybillTransferResponse resp;
	ns1__ArrayOfDOC param_list_hdr;

	TSCollection <ns1__DOC> arg_bill_list;
	TSCollection <ns1__HSET> arg_billamt_list;
	TSCollection <ns1__ArrayOfHSET> arg_hsetlh_list;
	TSCollection <ns1__DOC_USCOREITEM> arg_item_list;
	TSCollection <ns1__ArrayOfDOC_USCOREITEM> arg_itemlh_list;
	TSCollection <ns1__DSET> arg_itemamt_list;
	TSCollection <ns1__ArrayOfDSET> arg_dsetlh_list;

	TSCollection <ns1__ArrayOfREF> arg_reflh_list;
	TSCollection <ns1__REF> arg_ref_list;

	uint   i, j, k;
	if(pItems && pItems->getCount()) {
		param.username = arg_name;
		param.password = arg_pw;
		param.documents = &param_list_hdr;
		const uint first_pos = maxItems ? pItems->getPointer() : 0;
		const uint after_last_pos = maxItems ? MIN((first_pos + maxItems), pItems->getCount()) : pItems->getCount();
        for(i = first_pos; i < after_last_pos; i++) {
			const iSalesBillPacket * p_src_pack = pItems->at(i);
			if(p_src_pack) {
				ns1__DOC * p_new_bill = arg_bill_list.CreateNewItem(0);
				THROW(p_new_bill);
				p_new_bill->ISALES_USCOREID = GetDynamicParamString(p_src_pack->iSalesId, arg_str_pool);
				p_new_bill->STATUS = GetDynamicParamString(p_src_pack->Status, arg_str_pool);
				p_new_bill->DOC_USCORETP = GetDynamicParamString(p_src_pack->DocType, arg_str_pool);
				p_new_bill->DOC_USCORENO = GetDynamicParamString(p_src_pack->Code, arg_str_pool);
				p_new_bill->DOC_USCOREDT = GetDynamicParamString(p_src_pack->Dtm, DATF_GERMAN|DATF_CENTURY, TIMF_HMS, arg_str_pool);
				p_new_bill->INC_USCOREDT = 0;
				p_new_bill->EXT_USCORETP = 0;
				p_new_bill->EXT_USCORENO = 0;
				p_new_bill->EXT_USCOREDT = 0;
				p_new_bill->SHIP_USCOREFROM = GetDynamicParamString(p_src_pack->ShipFrom, arg_str_pool);
				p_new_bill->SHIP_USCORETO   = GetDynamicParamString(p_src_pack->ShipTo, arg_str_pool);
				p_new_bill->SELLER = GetDynamicParamString(p_src_pack->SellerCode, arg_str_pool);
				p_new_bill->PAYER  = GetDynamicParamString(p_src_pack->PayerCode, arg_str_pool);
				p_new_bill->ITEMS_USCOREAMOUNT = GetDynamicParamString((long)p_src_pack->Items.getCount(), arg_str_pool);
				p_new_bill->DUE_USCOREDATE = GetDynamicParamString(p_src_pack->DueDate, DATF_GERMAN|DATF_CENTURY, arg_str_pool);
				p_new_bill->COMMS = GetDynamicParamString(p_src_pack->Memo, arg_str_pool);
				p_new_bill->WHS_USCORESRC = GetDynamicParamString(p_src_pack->SrcLocCode, arg_str_pool);
				p_new_bill->WHS_USCOREDST = GetDynamicParamString(p_src_pack->DestLocCode, arg_str_pool);
				p_new_bill->SR_USCOREID = GetDynamicParamString(p_src_pack->AgentCode, arg_str_pool);
				p_new_bill->AUTH_USCOREID = GetDynamicParamString(p_src_pack->AuthId, arg_str_pool);
				p_new_bill->EDIT_USCOREID = GetDynamicParamString(p_src_pack->EditId, arg_str_pool);
				p_new_bill->CREATE_USCOREDT = GetDynamicParamString(p_src_pack->CreationDtm, DATF_GERMAN|DATF_CENTURY, TIMF_HMS, arg_str_pool);
				p_new_bill->EDIT_USCOREDT = GetDynamicParamString(p_src_pack->LastUpdDtm, DATF_GERMAN|DATF_CENTURY, TIMF_HMS, arg_str_pool);
				p_new_bill->ErrorMessage = 0;
				p_new_bill->ATTRS = 0; // list
				p_new_bill->REFS = 0; // list
				const uint start_item_list_pos = arg_item_list.getCount();
				for(j = 0; j < p_src_pack->Items.getCount(); j++) {
					const iSalesBillItem * p_item = p_src_pack->Items.at(j);
					if(p_item) {
						ns1__DOC_USCOREITEM * p_new_item = arg_item_list.CreateNewItem(0);
						THROW(p_new_item);
						p_new_item->LINE_USCORENO = GetDynamicParamString(p_item->LineN, arg_str_pool);
						p_new_item->SFA_USCOREID = GetDynamicParamString(p_item->OuterGoodsCode, arg_str_pool);
						p_new_item->ACC_USCOREID = GetDynamicParamString(p_item->NativeGoodsCode, arg_str_pool);
						p_new_item->QUANTITY = GetDynamicParamString_(p_item->Qtty, MKSFMTD(0, 8, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
						p_new_item->UOM_USCOREID = GetDynamicParamString(p_item->UnitCode, arg_str_pool);
						p_new_item->DESC = GetDynamicParamString(p_item->Memo, arg_str_pool);
						p_new_item->COUNTRY = GetDynamicParamString(p_item->Country, arg_str_pool);
						p_new_item->CUSTOMS = GetDynamicParamString(p_item->CLB, arg_str_pool);
						p_new_item->RET_USCORERSN = 0;
						const uint start_itemamt_list_pos = arg_itemamt_list.getCount();
						for(k = 0; k < p_item->Amounts.getCount(); k++) {
							const iSalesBillAmountEntry * p_src_amt = p_item->Amounts.at(k);
							if(p_src_amt) {
								ns1__DSET * p_new_amt = arg_itemamt_list.CreateNewItem(0);
								THROW(p_new_amt);
								p_new_amt->SET_USCORETYPE = GetDynamicParamString(p_src_amt->SetType, arg_str_pool);
								p_new_amt->NET_USCOREPR = GetDynamicParamString_(p_src_amt->NetPrice, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
								p_new_amt->GROSS_USCOREPR = GetDynamicParamString_(p_src_amt->GrossPrice, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
								p_new_amt->NET_USCORESUM = GetDynamicParamString_(p_src_amt->NetSum, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
								p_new_amt->DISCOUNT_USCOREAMOUNT = GetDynamicParamString_(p_src_amt->DiscAmount, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
								p_new_amt->DISC_USCORENET_USCORESUM = GetDynamicParamString_(p_src_amt->DiscNetSum, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
								p_new_amt->VAT_USCORESUM = GetDynamicParamString_(p_src_amt->VatSum, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
								p_new_amt->GROSS_USCORESUM = GetDynamicParamString_(p_src_amt->GrossSum, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
								p_new_amt->DISC_USCOREGR_USCOREAMOUNT = GetDynamicParamString_(p_src_amt->DiscGrossSum, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
							}
						}
						{
							ns1__ArrayOfDSET * p_new_lh = arg_dsetlh_list.CreateNewItem(0);
							THROW(p_new_lh);
							p_new_item->SETS = p_new_lh;
							p_new_item->SETS->DSET = (ns1__DSET **)PPSoapCreateArray(arg_itemamt_list.getCount()-start_itemamt_list_pos, p_new_item->SETS->__sizeDSET);
							for(k = start_itemamt_list_pos; k < arg_itemamt_list.getCount(); k++)
								p_new_item->SETS->DSET[k-start_itemamt_list_pos] = arg_itemamt_list.at(k);
						}
					}
				}
				{
					ns1__ArrayOfDOC_USCOREITEM * p_new_lh = arg_itemlh_list.CreateNewItem(0);
					THROW(p_new_lh);
					p_new_bill->DOC_USCOREITEMS = p_new_lh;
					p_new_bill->DOC_USCOREITEMS->DOC_USCOREITEM = (ns1__DOC_USCOREITEM **)PPSoapCreateArray(arg_item_list.getCount()-start_item_list_pos, p_new_bill->DOC_USCOREITEMS->__sizeDOC_USCOREITEM);
					for(j = start_item_list_pos; j < arg_item_list.getCount(); j++)
						p_new_bill->DOC_USCOREITEMS->DOC_USCOREITEM[j-start_item_list_pos] = arg_item_list.at(j);
				}
				//
				const uint start_billamt_list_pos = arg_billamt_list.getCount();
				for(j = 0; j < p_src_pack->Amounts.getCount(); j++) {
					const iSalesBillAmountEntry * p_src_amt = p_src_pack->Amounts.at(j);
					if(p_src_amt) {
						ns1__HSET * p_new_amt = arg_billamt_list.CreateNewItem(0);
						THROW(p_new_amt);
						p_new_amt->SET_USCORETYPE = GetDynamicParamString(p_src_amt->SetType, arg_str_pool);
						p_new_amt->NET = GetDynamicParamString_(p_src_amt->NetSum, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
						p_new_amt->DISC_USCORESUM = GetDynamicParamString_(p_src_amt->DiscNetSum, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
						p_new_amt->VAT = GetDynamicParamString_(p_src_amt->VatSum, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
						p_new_amt->GROSS = GetDynamicParamString_(p_src_amt->GrossSum, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
						p_new_amt->DISC_USCOREGR_USCORESUM = GetDynamicParamString_(p_src_amt->DiscGrossSum, MKSFMTD(0, 7, NMBF_DECCOMMA|NMBF_NOTRAILZ), arg_str_pool);
					}
				}
				{
					ns1__ArrayOfHSET * p_new_lh = arg_hsetlh_list.CreateNewItem(0);
					THROW(p_new_lh);
					p_new_bill->SUM_USCORESETS = p_new_lh;
					p_new_bill->SUM_USCORESETS->HSET = (ns1__HSET **)PPSoapCreateArray(arg_billamt_list.getCount()-start_billamt_list_pos, p_new_bill->SUM_USCORESETS->__sizeHSET);
					for(j = start_billamt_list_pos; j < arg_billamt_list.getCount(); j++)
						p_new_bill->SUM_USCORESETS->HSET[j-start_billamt_list_pos] = arg_billamt_list.at(j);
				}
				//
				const uint start_ref_list_pos = arg_ref_list.getCount();
				for(j = 0; j < p_src_pack->Refs.getCount(); j++) {
					const iSalesBillRef * p_src_ref = p_src_pack->Refs.at(j);
					if(p_src_ref) {
						ns1__REF * p_new_ref = arg_ref_list.CreateNewItem(0);
						THROW(p_new_ref);
						p_new_ref->REF_USCORETP = GetDynamicParamString(p_src_ref->DocType, arg_str_pool);
						p_new_ref->REF_USCORENO = GetDynamicParamString(p_src_ref->Code, arg_str_pool);
						p_new_ref->REF_USCOREDT = GetDynamicParamString(p_src_ref->Dtm, DATF_GERMAN|DATF_CENTURY, TIMF_HMS, arg_str_pool);
					}
				}
				{
					ns1__ArrayOfREF * p_new_lh = arg_reflh_list.CreateNewItem(0);
					THROW(p_new_lh);
					p_new_bill->REFS = p_new_lh;
					p_new_bill->REFS->REF = (ns1__REF **)PPSoapCreateArray(arg_ref_list.getCount()-start_ref_list_pos, p_new_bill->REFS->__sizeREF);
					for(j = start_ref_list_pos; j < arg_ref_list.getCount(); j++)
						p_new_bill->REFS->REF[j-start_ref_list_pos] = arg_ref_list.at(j);
				}
			}
        }
		param.documents->DOC = (ns1__DOC **)PPSoapCreateArray(arg_bill_list.getCount(), param.documents->__sizeDOC);
		for(i = 0; i < arg_bill_list.getCount(); i++) {
			param.documents->DOC[i] = arg_bill_list.at(i);
		}
		THROW(PreprocessCall(proxi, rSess, proxi.InvoiceWaybillTransfer(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		{
			THROW(p_result = new SString);
			PPSoapRegisterResultPtr(p_result);
			(*p_result = resp.InvoiceWaybillTransferResult).Transf(CTRANSF_UTF8_TO_INNER);
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	for(i = 0; i < arg_bill_list.getCount(); i++) {
		ns1__DOC * p_bill = arg_bill_list.at(i);
		if(p_bill) {
			if(p_bill->SUM_USCORESETS)
				ZFREE(p_bill->SUM_USCORESETS->HSET);
			if(p_bill->REFS)
				ZFREE(p_bill->REFS->REF);
			if(p_bill->DOC_USCOREITEMS) {
				for(j = 0; j < (uint)p_bill->DOC_USCOREITEMS->__sizeDOC_USCOREITEM; j++) {
					ns1__DOC_USCOREITEM * p_item = p_bill->DOC_USCOREITEMS->DOC_USCOREITEM[j];
					if(p_item && p_item->SETS) {
						ZFREE(p_item->SETS->DSET);
					}
				}
			}
		}
	}
	ZFREE(param.documents->DOC);
	return p_result;
}
