// PPSOAPCALLER-EFES.CPP
// Copyright (c) A.Sobolev 2016, 2017
//
#include <ppsoapclient.h>
#include "efes\efesSoapWS_USCOREEFES_USCOREDDEBindingProxy.h"

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	return Implement_SoapModule_DllMain(hModule, dwReason, lpReserved, "Papyrus EfesSoapModule");
}

extern "C" __declspec(dllexport) int EfesDestroyResult(void * pResult)
{
	return pResult ? PPSoapDestroyResultPtr(pResult) : -1;
}

static void FASTCALL ProcessError(WS_USCOREEFES_USCOREDDEBindingProxy & rProxi, PPSoapClientSession & rSess)
{
	char   temp_err_buf[1024];
	rProxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
	rSess.SetMsg(temp_err_buf);
}

static int PreprocessCall(WS_USCOREEFES_USCOREDDEBindingProxy & rProxy, PPSoapClientSession & rSess, int result)
{
	if(result == SOAP_OK) {
		return 1;
	}
	else {
		ProcessError(rProxy, rSess);
		return 0;
	}
}

static void InitEfecCallParam(ns2__DistributorRequestType & rParam, const SapEfesCallHeader & rH, TSCollection <InParamString> & rStrPool)
{
	rParam.SalesOrg = GetDynamicParamString(rH.P_SalesOrg, rStrPool);
	rParam.Wareh = GetDynamicParamString(rH.P_Wareh, rStrPool);
	SString temp_buf;
	temp_buf.CatLongZ(rH.SessionID, 12);
	rParam.SessionID = GetDynamicParamString(temp_buf, rStrPool);
}

static int DecodeEfesUnitType(const char * pUnitTypeStr)
{
	SString temp_buf;
	(temp_buf = pUnitTypeStr).Transf(CTRANSF_UTF8_TO_INNER);
	if(temp_buf.CmpNC("PCG") == 0)
		return sapefesUnitPack;
	else if(temp_buf.CmpNC("MPK") == 0)
		return sapefesUnitMultipack;
	else if(temp_buf.CmpNC("ST") == 0)
		return sapefesUnitItem;
	else
		return 0;
}

static char * EncodeEfesUnitType(int unitType, TSCollection <InParamString> & rArgStrPool)
{
	SString temp_buf;
	if(unitType == sapefesUnitItem)
		temp_buf = "ST";
	else if(unitType == sapefesUnitPack)
		temp_buf = "PCG";
	else if(unitType == sapefesUnitMultipack)
		temp_buf = "MPK";
	else if(unitType == spaefesUnitLiter)
		temp_buf = "L";
	else
		temp_buf = 0;
	return GetDynamicParamString(temp_buf.Transf(CTRANSF_INNER_TO_UTF8), rArgStrPool);
}

extern "C" __declspec(dllexport) TSCollection <SapEfesOrder> * EfesGetSalesOrderSyncList(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, 
	const DateRange * pPeriod, int repeat, const char * pDocNumberList)
{
	TSCollection <SapEfesOrder> * p_result = 0;
	WS_USCOREEFES_USCOREDDEBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS|/*SOAP_IO_CHUNK|*/SOAP_IO_KEEPALIVE); // @v9.5.0 SOAP_IO_CHUNK|SOAP_IO_KEEPALIVE
	//WS_USCOREEFES_USCOREDDEBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	//proxi.recv_timeout = 60; // @v9.5.0
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;
	gSoapClientInit(&proxi, 0, 0); 
	ns2__GetSalesOrderRequestType param;
	ns2__GetSalesOrderResponseType resp;
	char * p_empty_doc_num[] = { 0 };

	InitEfecCallParam(param, rH, arg_str_pool);
	proxi.userid = GetDynamicParamString((temp_buf = rSess.GetUser()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	proxi.passwd = GetDynamicParamString((temp_buf = rSess.GetPassword()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	{
		DateRange period;
		if(!RVALUEPTR(period, pPeriod))
			period.SetZero();
		SETIFZ(period.low, encodedate(1, 12, 2016));
		SETIFZ(period.upp, encodedate(31, 12, 2017));
		param.DateFrom =  GetDynamicParamString(period.low, DATF_ISO8601|DATF_CENTURY, arg_str_pool);
		param.DateTo   =  GetDynamicParamString(period.upp, DATF_ISO8601|DATF_CENTURY, arg_str_pool);
	}
	param.RepeatFlag = GetDynamicParamString(repeat, arg_str_pool);
	{
		if(!isempty(pDocNumberList)) {
			StringSet ss_doc_num(';', pDocNumberList);
			const uint doc_num_count = ss_doc_num.getCount();
			uint  ssi = 0;
			THROW(param.EFRDocNum = (char **)calloc(doc_num_count, sizeof(char *)));
			for(uint ssp = 0; ss_doc_num.get(&ssp, temp_buf);) {
				param.EFRDocNum[ssi++] = GetDynamicParamString(temp_buf.Strip().Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
			}
			assert(ssi == doc_num_count);
			param.__sizeEFRDocNum = (int)doc_num_count;
		}
		else {
			param.EFRDocNum = p_empty_doc_num;
			param.__sizeEFRDocNum = 0;
		}
	}
	THROW(PreprocessCall(proxi, rSess, proxi.GetSalesOrderSync(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	if(resp.SalesOrder && resp.__sizeSalesOrder > 0) {
		THROW(p_result = new TSCollection <SapEfesOrder>);
		PPSoapRegisterResultPtr(p_result);
		for(int i = 0; i < resp.__sizeSalesOrder; i++) {
			const ns2__DDESalesOrderType * p_doc = resp.SalesOrder[i];
			if(p_doc) {
				SapEfesOrder * p_new_item = p_result->CreateNewItem(0);
				THROW(p_new_item);
				//(temp_buf = p_doc->DocDate).Transf(CTRANSF_UTF8_TO_INNER);
				//strtodatetime(temp_buf, &p_new_item->Date, DATF_ISO8601|DATF_CENTURY, TIMF_HMS);
				p_new_item->Date.SetTimeT(p_doc->DocDate);
				p_new_item->DueDate = strtodate_(temp_buf = p_doc->RDelvDate, DATF_ISO8601|DATF_CENTURY);
				p_new_item->Amount = (temp_buf = p_doc->DocAmnt).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
				p_new_item->Code = (temp_buf = p_doc->EFRDocNum).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->DocType = (temp_buf = p_doc->DocType).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->Status = (temp_buf = p_doc->PRTStatus).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->PaymTerm = (temp_buf = p_doc->PaymTerm).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->ShipCond = (temp_buf = p_doc->ShipCond).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->ShipCondDescr = (temp_buf = p_doc->ShipCondDesc).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->SalesOrg = (temp_buf = p_doc->SalesOrg).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->DistrChannel = (temp_buf = p_doc->DistrChan).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->Division = (temp_buf = p_doc->Division).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->Warehouse = (temp_buf = p_doc->Wareh).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->TerrIdent = (temp_buf = p_doc->TerrID).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->Currency = (temp_buf = p_doc->Currency).Transf(CTRANSF_UTF8_TO_INNER);
				p_new_item->Memo = (temp_buf = p_doc->Comment).Transf(CTRANSF_UTF8_TO_INNER);
				{
					p_new_item->Buyer.Code = (temp_buf = p_doc->EFRSoldTo).Transf(CTRANSF_UTF8_TO_INNER);
					p_new_item->Buyer.Name = (temp_buf = p_doc->SoldToName1).Transf(CTRANSF_UTF8_TO_INNER);
					(temp_buf = p_doc->SoldToName2).Transf(CTRANSF_UTF8_TO_INNER);
					if(temp_buf.NotEmptyS())
						p_new_item->Buyer.Name.Space().Cat(temp_buf);
					p_new_item->Buyer.ZIP = (temp_buf = p_doc->SoldToPost).Transf(CTRANSF_UTF8_TO_INNER);
					p_new_item->Buyer.City = (temp_buf = p_doc->SoldToCity).Transf(CTRANSF_UTF8_TO_INNER);
					p_new_item->Buyer.Region = (temp_buf = p_doc->SoldToReg).Transf(CTRANSF_UTF8_TO_INNER);
					p_new_item->Buyer.Addr = (temp_buf = p_doc->SoldToAddr).Transf(CTRANSF_UTF8_TO_INNER);
					(temp_buf = p_doc->ShipToAddr2).Transf(CTRANSF_UTF8_TO_INNER);
					if(temp_buf.NotEmptyS())
						p_new_item->Buyer.Addr.Space().Cat(temp_buf);
				}
				{
					p_new_item->DlvrLoc.Code = (temp_buf = p_doc->EFRShipTo).Transf(CTRANSF_UTF8_TO_INNER);
					p_new_item->DlvrLoc.Name = (temp_buf = p_doc->ShipToName1).Transf(CTRANSF_UTF8_TO_INNER);
					(temp_buf = p_doc->ShipToName2).Transf(CTRANSF_UTF8_TO_INNER);
					if(temp_buf.NotEmptyS())
						p_new_item->DlvrLoc.Name.Space().Cat(temp_buf);
					p_new_item->DlvrLoc.ZIP = (temp_buf = p_doc->ShipToPost).Transf(CTRANSF_UTF8_TO_INNER);
					p_new_item->DlvrLoc.City = (temp_buf = p_doc->ShipToCity).Transf(CTRANSF_UTF8_TO_INNER);
					p_new_item->DlvrLoc.Region = (temp_buf = p_doc->ShipToReg).Transf(CTRANSF_UTF8_TO_INNER);
					p_new_item->DlvrLoc.Addr = (temp_buf = p_doc->ShipToAddr).Transf(CTRANSF_UTF8_TO_INNER);
					(temp_buf = p_doc->ShipToAddr2).Transf(CTRANSF_UTF8_TO_INNER);
					if(temp_buf.NotEmptyS())
						p_new_item->DlvrLoc.Addr.Space().Cat(temp_buf);
				}
				if(p_doc->Item && p_doc->__sizeItem > 0) {
					for(int j = 0; j < p_doc->__sizeItem; j++) {
						const ns2__SalesOrderItemExtType * p_src_item = p_doc->Item[j];
						if(p_src_item) {
							SapEfesBillItem * p_new_row = p_new_item->Items.CreateNewItem(0);
							THROW(p_new_row);
							p_new_row->PosN = (temp_buf = p_src_item->PosNum).Transf(CTRANSF_UTF8_TO_INNER).ToLong();
							p_new_row->UnitType = DecodeEfesUnitType(p_src_item->Unit);
							p_new_row->BaseUnitType = DecodeEfesUnitType(p_src_item->BaseUnit);
							p_new_row->Qtty = (temp_buf = p_src_item->Qty).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_row->QtyN = (temp_buf = p_src_item->QtyN).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_row->QtyD = (temp_buf = p_src_item->QtyD).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
							p_new_row->PosType = (temp_buf = p_src_item->PosType).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_row->GoodsCode = (temp_buf = p_src_item->EFRProd).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_row->Currency = (temp_buf = p_src_item->Currency).Transf(CTRANSF_UTF8_TO_INNER);
							p_new_row->Amount = (temp_buf = p_src_item->Amnt).Transf(CTRANSF_UTF8_TO_INNER).ToReal();
						}
					}
				}
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	if(param.__sizeEFRDocNum)
		free(param.EFRDocNum);
	return p_result;
}

extern "C" __declspec(dllexport) int EfesSetSalesOrderStatusSync(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesBillStatus> * pItems)
{
	int    result = 0;
	uint   j;
	WS_USCOREEFES_USCOREDDEBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;
	gSoapClientInit(&proxi, 0, 0);
	ns2__SetSalesOrderStatusRequestType param;
	ns2__LogMsgType resp;
	TSCollection <ns2__SalesOrderStatusType> arg_status_list;
	TSCollection <ns2__LogMsgType> arg_msg_list;
	InitEfecCallParam(param, rH, arg_str_pool);
	proxi.userid = GetDynamicParamString((temp_buf = rSess.GetUser()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	proxi.passwd = GetDynamicParamString((temp_buf = rSess.GetPassword()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	if(pItems && pItems->getCount()) {
		for(uint i = 0; i < pItems->getCount(); i++) {
			const SapEfesBillStatus * p_src_pack = pItems->at(i);
			if(p_src_pack) {
				ns2__SalesOrderStatusType * p_new_item = arg_status_list.CreateNewItem(0);
				THROW(p_new_item);
				p_new_item->PRTDocNum = GetDynamicParamString((temp_buf = p_src_pack->NativeCode).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
				p_new_item->ERPDocNum = GetDynamicParamString((temp_buf = p_src_pack->Code).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
				p_new_item->OrderStatus = GetDynamicParamString((temp_buf = p_src_pack->Status).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
				const uint start_msg_list_pos = arg_msg_list.getCount();
				if(p_src_pack->MsgList.getCount()) {
					for(uint j = 0; j < p_src_pack->MsgList.getCount(); j++) {
						const SapEfesLogMsg * p_src_msg = p_src_pack->MsgList.at(j);
						if(p_src_msg) {
							ns2__LogMsgType * p_new_msg = arg_msg_list.CreateNewItem(0);
							THROW(p_new_msg);
							if(p_src_msg->MsgType == SapEfesLogMsg::tE)
								temp_buf = "E";
							else if(p_src_msg->MsgType == SapEfesLogMsg::tW)
								temp_buf = "W";
							else if(p_src_msg->MsgType == SapEfesLogMsg::tS)
								temp_buf = "S";
							else
								temp_buf = "";
							p_new_msg->MsgType = GetDynamicParamString(temp_buf.Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
							p_new_msg->MsgText = GetDynamicParamString((temp_buf = p_src_msg->Msg).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
						}
					}
				}
				else {
					ns2__LogMsgType * p_new_msg = arg_msg_list.CreateNewItem(0);
					THROW(p_new_msg);
					temp_buf = "";
					p_new_msg->MsgType = GetDynamicParamString(temp_buf.Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					temp_buf = "";
					p_new_msg->MsgText = GetDynamicParamString(temp_buf.Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
				}
				p_new_item->LogMsg = (ns2__LogMsgType **)PPSoapCreateArray(arg_msg_list.getCount()-start_msg_list_pos, p_new_item->__sizeLogMsg);
				for(j = start_msg_list_pos; j < arg_msg_list.getCount(); j++)
					p_new_item->LogMsg[j-start_msg_list_pos] = arg_msg_list.at(j);
			}
		}
		THROW(PreprocessCall(proxi, rSess, proxi.SetSalesOrderStatusSync(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
	}
	CATCH
		result = 0;
	ENDCATCH
	for(uint i = 0; i < arg_status_list.getCount(); i++) {
		ns2__SalesOrderStatusType * p_item = arg_status_list.at(i);
		if(p_item) {
			ZFREE(p_item->LogMsg);
		}
	}
	return result;
}

static int CreateLogMsgItem(TSCollection <SapEfesLogMsg> & rList, const ns2__LogMsgType * pSrcMsg)
{
	int   ok = 1;
	if(pSrcMsg) {
		SapEfesLogMsg * p_new_msg = rList.CreateNewItem(0);
		THROW(p_new_msg);
		if(_stricmp(pSrcMsg->MsgType, "S") == 0)
			p_new_msg->MsgType = SapEfesLogMsg::tS;
		else if(_stricmp(pSrcMsg->MsgType, "W") == 0)
			p_new_msg->MsgType = SapEfesLogMsg::tW;
		else if(_stricmp(pSrcMsg->MsgType, "E") == 0)
			p_new_msg->MsgType = SapEfesLogMsg::tE;
		(p_new_msg->Msg = pSrcMsg->MsgText).Transf(CTRANSF_UTF8_TO_INNER);
	}
	CATCHZOK
	return ok;
}

extern "C" __declspec(dllexport) TSCollection <SapEfesBillStatus> * EfesSetDeliveryNoteSync(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesBillPacket> * pItems)
{
	TSCollection <SapEfesBillStatus> * p_result = 0;
	WS_USCOREEFES_USCOREDDEBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;
	gSoapClientInit(&proxi, 0, 0);
	ns2__SetDeliveryNoteRequestType param;
	ns2__SetDeliveryNoteResponseType resp;
	TSCollection <ns2__DeliveryNoteType> arg_note_list;
	TSCollection <ns2__SalesOrderItemType> arg_items_list;
	//TSCollection <ns2__LogMsgType> arg_msg_list;
	InitEfecCallParam(param, rH, arg_str_pool);
	proxi.userid = GetDynamicParamString((temp_buf = rSess.GetUser()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	proxi.passwd = GetDynamicParamString((temp_buf = rSess.GetPassword()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);

	if(pItems && pItems->getCount()) {
		{
			for(uint i = 0; i < pItems->getCount(); i++) {
				const SapEfesBillPacket * p_src_pack = pItems->at(i);
				if(p_src_pack) {
					ns2__DeliveryNoteType * p_new_item = arg_note_list.CreateNewItem(0);
					THROW(p_new_item);
					p_new_item->PRTDocNum = GetDynamicParamString((temp_buf = p_src_pack->NativeCode).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					p_new_item->EFRDocNum = GetDynamicParamString((temp_buf = p_src_pack->OrderCode).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					p_new_item->DelvDate = GetDynamicParamString(p_src_pack->Date, DATF_ISO8601|DATF_CENTURY, arg_str_pool);
					p_new_item->RDelvDate = GetDynamicParamString(p_src_pack->DueDate, DATF_ISO8601|DATF_CENTURY, arg_str_pool);
					p_new_item->EFRSoldTo = GetDynamicParamString((temp_buf = p_src_pack->BuyerCode).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					p_new_item->EFRShipTo = GetDynamicParamString((temp_buf = p_src_pack->DlvrLocCode).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					{
						(temp_buf = p_src_pack->Memo).Trim(40);
						p_new_item->Comment = GetDynamicParamString(temp_buf.Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					}
					p_new_item->ReturnSign = GetDynamicParamString((p_src_pack->Flags & p_src_pack->fReturn) ? "1" : "", arg_str_pool);
					switch(p_src_pack->DocType) {
						case SapEfesBillPacket::tRetail:    temp_buf = "ZDSD"; break;
						case SapEfesBillPacket::tReturn:    temp_buf = "ZRET"; break;
						case SapEfesBillPacket::tBonus:     temp_buf = "ZDS1"; break;
						case SapEfesBillPacket::tBonusEfes: temp_buf = "ZDS2"; break;
						default: temp_buf = 0; break;
					}
					p_new_item->DocType = GetDynamicParamString(temp_buf.Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					if(p_src_pack->Flags & SapEfesBillPacket::fHasOrderRef)
						p_new_item->DelvType = GetDynamicParamString("1", arg_str_pool);
					else
						p_new_item->DelvType = GetDynamicParamString("2", arg_str_pool);
					if(p_src_pack->Items.getCount()) {
						const uint start_items_list_pos = arg_items_list.getCount();
						for(uint rowidx = 0; rowidx < p_src_pack->Items.getCount(); rowidx++) {
							const SapEfesBillItem * p_src_row = p_src_pack->Items.at(rowidx);
							if(p_src_row) {
								ns2__SalesOrderItemType * p_new_row = arg_items_list.CreateNewItem(0);
								THROW(p_new_row);
								p_new_row->EFRProd = GetDynamicParamString((temp_buf = p_src_row->GoodsCode).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
								p_new_row->PosNum = GetDynamicParamString(p_src_row->PosN, arg_str_pool);
								p_new_row->PosType = GetDynamicParamString((temp_buf = "").Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
								p_new_row->Currency = GetDynamicParamString((temp_buf = p_src_row->Currency).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
								p_new_row->Qty = GetDynamicParamString_(p_src_row->Qtty, MKSFMTD(0, 2, 0), arg_str_pool);
								//p_new_row->Unit = GetDynamicParamString((temp_buf = p_src_row->UnitType).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
								p_new_row->Unit = EncodeEfesUnitType(p_src_row->UnitType, arg_str_pool);
								p_new_row->Amnt = GetDynamicParamString_(p_src_row->Amount, MKSFMTD(0, 2, 0), arg_str_pool);
							}
						}
						p_new_item->Item = (ns2__SalesOrderItemType **)PPSoapCreateArray(arg_items_list.getCount() - start_items_list_pos, p_new_item->__sizeItem);
						for(uint j = start_items_list_pos; j < arg_items_list.getCount(); j++) {
							p_new_item->Item[j-start_items_list_pos] = arg_items_list.at(j);
						}
					}
				}
			}
		}
		{
			param.DeliveryNote = (ns2__DeliveryNoteType **)PPSoapCreateArray(arg_note_list.getCount(), param.__sizeDeliveryNote);
			THROW(param.DeliveryNote);
			for(uint i = 0; i < arg_note_list.getCount(); i++) {
				param.DeliveryNote[i] = arg_note_list.at(i);
			}
		}
		THROW(PreprocessCall(proxi, rSess, proxi.SetDeliveryNoteSync(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		if(resp.DeliveryNoteStatus && resp.__sizeDeliveryNoteStatus > 0) {
			THROW(p_result = new TSCollection <SapEfesBillStatus>);
			PPSoapRegisterResultPtr(p_result);
			for(int i = 0; i < resp.__sizeDeliveryNoteStatus; i++) {
				const ns2__DeliveryNoteStatusType * p_src_status = resp.DeliveryNoteStatus[i];
				SapEfesBillStatus * p_new_status = p_result->CreateNewItem(0);
				THROW(p_new_status);
				p_new_status->NativeCode = p_src_status->PRTDocNum;
				p_new_status->Code = p_src_status->EFRDelvNum;
				p_new_status->Status = p_src_status->EFRStatus;
				if(p_src_status->LogMsg && p_src_status->__sizeLogMsg) {
					for(int j = 0; j < p_src_status->__sizeLogMsg; j++) {
						THROW(CreateLogMsgItem(p_new_status->MsgList, p_src_status->LogMsg[j]));
					}
				}
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	{
		for(uint i = 0; i < arg_note_list.getCount(); i++) {
			ns2__DeliveryNoteType * p_item = arg_note_list.at(i);
			if(p_item) {
				ZFREE(p_item->Item);
			}
		}
		ZFREE(param.DeliveryNote);
	}
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <SapEfesLogMsg> * EfesSetDailyStockReportSync(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesGoodsReportEntry> * pItems)
{
	TSCollection <SapEfesLogMsg> * p_result = 0;
	WS_USCOREEFES_USCOREDDEBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;
	gSoapClientInit(&proxi, 0, 0);
	ns1__SetDailyStockReportRequestType param;
	ns1__SetDailyStockReportResponseType resp;
	TSCollection <_ns1__ProductReportRequestType_Row> arg_row_list;
	TSCollection <ns1__ProductReportRequestType> arg_entry2_list;
	TSCollection <ns2__LogMsgType> arg_msg_list;
	{
		ns2__DistributorRequestType temp_h;
		InitEfecCallParam(temp_h, rH, arg_str_pool);
		param.SalesOrg = temp_h.SalesOrg;
		param.SessionID = temp_h.SessionID;
		param.Wareh = temp_h.Wareh;
	}
	proxi.userid = GetDynamicParamString((temp_buf = rSess.GetUser()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	proxi.passwd = GetDynamicParamString((temp_buf = rSess.GetPassword()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);

	if(pItems && pItems->getCount()) {
		param.DATA = (ns1__ProductReportRequestType **)PPSoapCreateArray(1, param.__sizeDATA);
		{
			for(uint i = 0; i < pItems->getCount(); i++) {
				const SapEfesGoodsReportEntry * p_src_pack = pItems->at(i);
				if(p_src_pack) {
					_ns1__ProductReportRequestType_Row * p_new_item = arg_row_list.CreateNewItem(0);
					THROW(p_new_item);
					p_new_item->EFRProd = GetDynamicParamString((temp_buf = p_src_pack->GoodsCode).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					p_new_item->ReportDate = GetDynamicParamString(p_src_pack->Dt, DATF_ISO8601|DATF_CENTURY, arg_str_pool);
					p_new_item->Qty = GetDynamicParamString_(p_src_pack->Qtty, MKSFMTD(0, 3, 0), arg_str_pool);
					p_new_item->Unit = EncodeEfesUnitType(p_src_pack->UnitType, arg_str_pool);
				}
			}
		}
		{
			ns1__ProductReportRequestType * p_new_entry = arg_entry2_list.CreateNewItem(0);
			THROW(p_new_entry);
			p_new_entry->__sizeRow = (int)arg_row_list.getCount();
			//p_new_entry->Row = (_ns1__ProductReportRequestType_Row *)calloc(arg_row_list.getCount(), sizeof(*p_new_entry->Row));
			p_new_entry->Row = new _ns1__ProductReportRequestType_Row[arg_row_list.getCount()];
			THROW(p_new_entry->Row);
			for(uint i = 0; i < arg_row_list.getCount(); i++) {
				p_new_entry->Row[i] = *arg_row_list.at(i);
			}
			param.DATA[0] = p_new_entry;
			/*
			param.DATA[0]->__sizeRow = (int)arg_row_list.getCount();
			param.DATA[0]->Row = (_ns1__ProductReportRequestType_Row *)calloc(arg_row_list.getCount(), sizeof(*(param.DATA[0]->Row)));
			THROW(param.DATA[0]->Row);
			for(uint i = 0; i < arg_row_list.getCount(); i++) {
				param.DATA[0]->Row[i] = *arg_row_list.at(i);
			}
			*/
		}
		THROW(PreprocessCall(proxi, rSess, proxi.SetDailyStockReportSync(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		if(resp.ReportStatus.LogMsg && resp.ReportStatus.__sizeLogMsg) {
			p_result = new TSCollection <SapEfesLogMsg>();
			THROW(p_result);
			PPSoapRegisterResultPtr(p_result);
			for(int i = 0; i < resp.ReportStatus.__sizeLogMsg; i++) {
				ns2__LogMsgType temp_item;
				temp_item.MsgType = resp.ReportStatus.LogMsg[i].MsgType;
				temp_item.MsgText = resp.ReportStatus.LogMsg[i].MsgText;
				THROW(CreateLogMsgItem(*p_result, &temp_item));
			}
		}
	}
	CATCH
		ZFREE(p_result);
	ENDCATCH
	{
		for(uint i = 0; i < arg_entry2_list.getCount(); i++) {
			ns1__ProductReportRequestType * p_entry = arg_entry2_list.at(i);
			if(p_entry) {
				delete [] p_entry->Row;
			}
		}
	}
	ZFREE(param.DATA);
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <SapEfesLogMsg> * EfesSetMTDProductReportSync(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesGoodsReportEntry> * pItems)
{
	TSCollection <SapEfesLogMsg> * p_result = 0;
	WS_USCOREEFES_USCOREDDEBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;
	gSoapClientInit(&proxi, 0, 0);
	ns1__SetMTDProductReportRequestType param;
	ns1__SetMTDProductReportResponseType resp;
	TSCollection <_ns1__ProductReportRequestType_Row> arg_entry_list;
	TSCollection <ns2__LogMsgType> arg_msg_list;
	{
		ns2__DistributorRequestType temp_h;
		temp_h.SalesOrg = param.SalesOrg;
		temp_h.SessionID = param.SessionID;
		temp_h.Wareh = param.Wareh;
		InitEfecCallParam(temp_h, rH, arg_str_pool);
	}
	proxi.userid = GetDynamicParamString((temp_buf = rSess.GetUser()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	proxi.passwd = GetDynamicParamString((temp_buf = rSess.GetPassword()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	if(pItems && pItems->getCount()) {
		param.DATA = (ns1__ProductReportRequestType **)PPSoapCreateArray(1, param.__sizeDATA);
		{
			for(uint i = 0; i < pItems->getCount(); i++) {
				const SapEfesGoodsReportEntry * p_src_pack = pItems->at(i);
				if(p_src_pack) {
					_ns1__ProductReportRequestType_Row * p_new_item = arg_entry_list.CreateNewItem(0);
					THROW(p_new_item);
					p_new_item->EFRProd = GetDynamicParamString((temp_buf = p_src_pack->GoodsCode).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					p_new_item->ReportDate = GetDynamicParamString(p_src_pack->Dt, DATF_ISO8601|DATF_CENTURY, arg_str_pool);
					p_new_item->Qty = GetDynamicParamString_(p_src_pack->Qtty, MKSFMTD(0, 3, 0), arg_str_pool);
					p_new_item->Unit = EncodeEfesUnitType(p_src_pack->UnitType, arg_str_pool);
				}
			}
		}
		{
			THROW(param.DATA[0] = new ns1__ProductReportRequestType);
			param.DATA[0]->__sizeRow = (int)arg_entry_list.getCount();
			THROW(param.DATA[0]->Row = new _ns1__ProductReportRequestType_Row[arg_entry_list.getCount()]);
			for(uint i = 0; i < arg_entry_list.getCount(); i++) {
				param.DATA[0]->Row[i] = *arg_entry_list.at(i);
			}
		}
		THROW(PreprocessCall(proxi, rSess, proxi.SetMTDProductReportSync(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		if(resp.ReportStatus.LogMsg && resp.ReportStatus.__sizeLogMsg) {
			p_result = new TSCollection <SapEfesLogMsg>();
			THROW(p_result);
			PPSoapRegisterResultPtr(p_result);
			for(int i = 0; i < resp.ReportStatus.__sizeLogMsg; i++) {
				ns2__LogMsgType temp_item;
				temp_item.MsgType = resp.ReportStatus.LogMsg[i].MsgType;
				temp_item.MsgText = resp.ReportStatus.LogMsg[i].MsgText;
				THROW(CreateLogMsgItem(*p_result, &temp_item));
			}
		}
	}
	CATCH
		ZFREE(p_result);
	ENDCATCH
	delete [] param.DATA[0]->Row;
	delete param.DATA[0];
	ZFREE(param.DATA);
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <SapEfesLogMsg> * EfesSetMTDOutletsReportSync(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesGoodsReportEntry> * pItems)
{
	TSCollection <SapEfesLogMsg> * p_result = 0;
	WS_USCOREEFES_USCOREDDEBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;
	gSoapClientInit(&proxi, 0, 0);
	ns1__SetMTDOutletsReportRequestType param;
	ns1__SetMTDOutletsReportResponseType resp;
	TSCollection <_ns1__OutletReportRequestType_Row> arg_entry_list;
	TSCollection <ns2__LogMsgType> arg_msg_list;
	{
		ns2__DistributorRequestType temp_h;
		temp_h.SalesOrg = param.SalesOrg;
		temp_h.SessionID = param.SessionID;
		temp_h.Wareh = param.Wareh;
		InitEfecCallParam(temp_h, rH, arg_str_pool);
	}
	proxi.userid = GetDynamicParamString((temp_buf = rSess.GetUser()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	proxi.passwd = GetDynamicParamString((temp_buf = rSess.GetPassword()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	if(pItems && pItems->getCount()) {
		param.DATA = (ns1__OutletReportRequestType **)PPSoapCreateArray(1, param.__sizeDATA);
		{
			for(uint i = 0; i < pItems->getCount(); i++) {
				const SapEfesGoodsReportEntry * p_src_pack = pItems->at(i);
				if(p_src_pack) {
					_ns1__OutletReportRequestType_Row * p_new_item = arg_entry_list.CreateNewItem(0);
					THROW(p_new_item);
					p_new_item->EFRShipTo = GetDynamicParamString((temp_buf = p_src_pack->DlvrLocCode).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					p_new_item->ReportDate = GetDynamicParamString(p_src_pack->Dt, DATF_ISO8601|DATF_CENTURY, arg_str_pool);
					p_new_item->Qty = GetDynamicParamString_(p_src_pack->Qtty, MKSFMTD(0, 3, 0), arg_str_pool);
					p_new_item->Unit = EncodeEfesUnitType(p_src_pack->UnitType, arg_str_pool);
				}
			}
		}
		{
			THROW(param.DATA[0] = new ns1__OutletReportRequestType);
			param.DATA[0]->__sizeRow = (int)arg_entry_list.getCount();
			THROW(param.DATA[0]->Row = new _ns1__OutletReportRequestType_Row[arg_entry_list.getCount()]);
			for(uint i = 0; i < arg_entry_list.getCount(); i++) {
				param.DATA[0]->Row[i] = *arg_entry_list.at(i);
			}
		}
		THROW(PreprocessCall(proxi, rSess, proxi.SetMTDOutletsReportSync(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		if(resp.ReportStatus.LogMsg && resp.ReportStatus.__sizeLogMsg) {
			p_result = new TSCollection <SapEfesLogMsg>();
			THROW(p_result);
			PPSoapRegisterResultPtr(p_result);
			for(int i = 0; i < resp.ReportStatus.__sizeLogMsg; i++) {
				ns2__LogMsgType temp_item;
				temp_item.MsgType = resp.ReportStatus.LogMsg[i].MsgType;
				temp_item.MsgText = resp.ReportStatus.LogMsg[i].MsgText;
				THROW(CreateLogMsgItem(*p_result, &temp_item));
			}
		}
	}
	CATCH
		ZFREE(p_result);
	ENDCATCH
	delete [] param.DATA[0]->Row;
	delete param.DATA[0];
	ZFREE(param.DATA);
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <SapEfesLogMsg> * EfesSetDebtSync(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesDebtReportEntry> * pItems)
{
	TSCollection <SapEfesLogMsg> * p_result = 0;
	WS_USCOREEFES_USCOREDDEBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;
	gSoapClientInit(&proxi, 0, 0);
	ns2__SetDebtsRequestType param;
	ns2__LogMsgType resp;
	TSCollection <_ns2__SetDebtsRequestType_OutletDebts> arg_row_list;
	//
	TSCollection <ns2__LogMsgType> arg_msg_list;
	{
		ns2__DistributorRequestType temp_h;
		InitEfecCallParam(temp_h, rH, arg_str_pool);
		param.SalesOrg = temp_h.SalesOrg;
		param.SessionID = temp_h.SessionID;
		param.Wareh = temp_h.Wareh;
	}
	proxi.userid = GetDynamicParamString((temp_buf = rSess.GetUser()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
	proxi.passwd = GetDynamicParamString((temp_buf = rSess.GetPassword()).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);

	if(pItems && pItems->getCount()) {
		THROW(param.OutletDebts = new _ns2__SetDebtsRequestType_OutletDebts[pItems->getCount()]);
		param.__sizeOutletDebts = 0;
		{
			for(uint i = 0; i < pItems->getCount(); i++) {
				const SapEfesDebtReportEntry * p_src_pack = pItems->at(i);
				_ns2__SetDebtsRequestType_OutletDebts & r_new_item = param.OutletDebts[i];
				if(p_src_pack) {
					r_new_item.EFRSoldTo = GetDynamicParamString((temp_buf = p_src_pack->BuyerCode).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					r_new_item.DebtAmnt = GetDynamicParamString_(p_src_pack->Debt, MKSFMTD(0, 2, 0), arg_str_pool);
					r_new_item.DebtLimit = GetDynamicParamString_(p_src_pack->CreditLimit, MKSFMTD(0, 2, 0), arg_str_pool);
					r_new_item.DebtLmtDays = GetDynamicParamString(p_src_pack->DebtDelayDays, arg_str_pool);
					r_new_item.DebtDelayDays = GetDynamicParamString(0L, arg_str_pool);
					r_new_item.Comment = GetDynamicParamString("", arg_str_pool);
					param.__sizeOutletDebts++;
				}
			}
		}
		THROW(PreprocessCall(proxi, rSess, proxi.SetDebtsSync(rSess.GetUrl(), 0 /* soap_action */, &param, &resp)));
		{
			ns2__LogMsgType temp_item;
			p_result = new TSCollection <SapEfesLogMsg>();
			THROW(p_result);
			PPSoapRegisterResultPtr(p_result);
			temp_item.MsgType = resp.MsgType;
			temp_item.MsgText = resp.MsgText;
			THROW(CreateLogMsgItem(*p_result, &temp_item));
		}
	}
	CATCH
		ZFREE(p_result);
	ENDCATCH
	delete [] param.OutletDebts;
	return p_result;
}