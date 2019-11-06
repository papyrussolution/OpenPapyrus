// PPSOAPCALLER-UHTT.CPP
// Copyright (c) A.Sobolev 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
//
#include <ppsoapclient.h>
#include "uhtt2\uhttSoapWSInterfaceImplServiceSoapBindingProxy.h"

// Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	return Implement_SoapModule_DllMain(hModule, dwReason, lpReserved, "Papyrus SoapModule");
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

static SString & FASTCALL GetSoapStr(const char * pSoapStr, SString & rBuf)
{
	return (rBuf = pSoapStr).Transf(CTRANSF_UTF8_TO_INNER);
}

static UhttStatus * FASTCALL CreateResultStatus(const ns1__status * pStatus)
{
	UhttStatus * p_result = new UhttStatus;
	if(p_result) {
		PPSoapRegisterResultPtr(p_result);
		p_result->Code = pStatus->Code;
		p_result->Msg = pStatus->Message;
	}
	return p_result;
}

static UhttStatus * FASTCALL CreateResultStatus(const ns1__statusExt * pStatus)
{
	UhttStatus * p_result = new UhttStatus;
	if(p_result) {
		PPSoapRegisterResultPtr(p_result);
		p_result->Code = pStatus->Code;
		p_result->Index = pStatus->Index;
		p_result->Id = pStatus->Id;
		p_result->Msg = pStatus->Message;
	}
	return p_result;
}

static void FASTCALL ProcessError(WSInterfaceImplServiceSoapBindingProxy & rProxi, PPSoapClientSession & rSess)
{
	char   temp_err_buf[1024];
	rProxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
	rSess.SetMsg(temp_err_buf);
}

static int FASTCALL PreprocessCall(WSInterfaceImplServiceSoapBindingProxy & rProxy, PPSoapClientSession & rSess, int result)
{
	if(result == SOAP_OK) {
		return 1;
	}
	else {
		ProcessError(rProxy, rSess);
		return 0;
	}
}
//
//
//
extern "C" __declspec(dllexport) int UhttDestroyResult(void * pResult)
{
	return pResult ? PPSoapDestroyResultPtr(pResult) : -1;
}

extern "C" __declspec(dllexport) SString * UhttAuth(PPSoapClientSession & rSess, const char * pName, const char * pPassword)
{
	char * p_token = 0;
	SString * p_result = 0;
	SString temp_buf;
	InParamString arg_name(pName);
	InParamString arg_pw(pPassword);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__auth param;
	ns1__authResponse resp;
	param.email = arg_name;
	param.password = arg_pw;
	if(PreprocessCall(proxi, rSess, proxi.auth(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new SString;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			*p_result = resp.token;
		}
	}
	return p_result;
}

static int Implement_GetPersonPacket(const ns1__person * pSp, UhttPersonPacket & rP)
{
	int    ok = 1;
	if(pSp) {
		int    i;
		SString temp_buf;
		rP.ID = pSp->ID;
		rP.StatusID = pSp->StatusID;
		rP.CategoryID = pSp->CategoryID;
		GetSoapStr(pSp->Name, rP.Name);
		GetSoapStr(pSp->INN, rP.INN);
		rP.PhoneList.clear();
		for(i = 0; i < pSp->__sizePhoneList; i++) {
			rP.PhoneList.add(GetSoapStr(pSp->PhoneList[i], temp_buf));
		}
		rP.EMailList.clear();
		for(i = 0; i < pSp->__sizeEMailList; i++) {
			rP.EMailList.add(GetSoapStr(pSp->EMailList[i], temp_buf));
		}
		rP.UrlList.clear();
		for(i = 0; i < pSp->__sizeUrlList; i++) {
			rP.UrlList.add(GetSoapStr(pSp->UrlList[i], temp_buf));
		}
		rP.KindList.freeAll();
		for(i = 0; i < pSp->__sizeKindList; i++) {
			if(pSp->KindList[i]) {
				UhttPersonPacket::Kind * p_item = new UhttPersonPacket::Kind;
				GetSoapStr(pSp->KindList[i]->Code, p_item->Code);
				GetSoapStr(pSp->KindList[i]->Name, p_item->Name);
				rP.KindList.insert(p_item);
			}
		}
		rP.RegList.freeAll();
		for(i = 0; i < pSp->__sizeRegisterList; i++) {
			if(pSp->RegisterList[i]) {
				UhttPersonPacket::Register * p_item = new UhttPersonPacket::Register;
				p_item->ID = pSp->RegisterList[i]->RegID;
				p_item->TypeID = pSp->RegisterList[i]->RegTypeID;
				p_item->OrganID = pSp->RegisterList[i]->RegOrgID;
				p_item->Dt = pSp->RegisterList[i]->RegDt;
				p_item->Expiry = pSp->RegisterList[i]->RegExpiry;
				GetSoapStr(pSp->RegisterList[i]->RegTypeName, p_item->TypeName);
				GetSoapStr(pSp->RegisterList[i]->RegSerial, p_item->Serial);
				GetSoapStr(pSp->RegisterList[i]->RegNumber, p_item->Number);
				rP.RegList.insert(p_item);
			}
		}
		rP.AddrList.freeAll();
		for(i = 0; i < pSp->__sizeAddrList; i++) {
			if(pSp->AddrList[i]) {
				UhttPersonPacket::AddressP * p_item = new UhttPersonPacket::AddressP;
				p_item->ID = pSp->AddrList[i]->LocID;
				p_item->Type = 3; // LOCTYP_ADDRESS
				p_item->CityID = pSp->AddrList[i]->CityID;
				GetSoapStr(pSp->AddrList[i]->Country, p_item->CountryName);
				GetSoapStr(pSp->AddrList[i]->City, p_item->CityName);
				p_item->OwnerPersonID = pSp->AddrList[i]->OwnerID;
				p_item->Latitude = pSp->AddrList[i]->Latitude;
				p_item->Longitude = pSp->AddrList[i]->Longitude;
				GetSoapStr(pSp->AddrList[i]->LocCode, p_item->Code);
				GetSoapStr(pSp->AddrList[i]->LocName, p_item->Name);
				GetSoapStr(pSp->AddrList[i]->ZIP, p_item->PostalCode);
				GetSoapStr(pSp->AddrList[i]->Address, p_item->Address);
				rP.AddrList.insert(p_item);
			}
		}
	}
	return ok;
}

extern "C" __declspec(dllexport) UhttPersonPacket * UhttGetPersonByID(PPSoapClientSession & rSess, const char * pToken, int id)
{
	UhttPersonPacket * p_result = 0;
	InParamString arg_token(pToken);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getPersonByID param;
	ns1__getPersonByIDResponse resp;
	param.token = arg_token;
	param.id = id;
	if(PreprocessCall(proxi, rSess, proxi.getPersonByID(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		THROW(p_result = new UhttPersonPacket);
		PPSoapRegisterResultPtr(p_result);
		if(resp.person) {
			Implement_GetPersonPacket(resp.person, *p_result);
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <UhttPersonPacket> * UhttGetPersonByName(PPSoapClientSession & rSess, const char * pToken, const char * pName)
{
	TSCollection <UhttPersonPacket> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_name(pName);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getPersonByName param;
	ns1__getPersonByNameResponse resp;
	param.token = arg_token;
	param.name = arg_name;
	if(PreprocessCall(proxi, rSess, proxi.getPersonByName(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new TSCollection <UhttPersonPacket>;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			if(resp.personArray) {
				for(int i = 0; i < resp.__sizepersonArray; i++) {
					ns1__person * p_item = resp.personArray[i];
					if(p_item) {
						UhttPersonPacket * p_pack = new UhttPersonPacket;
						THROW(p_pack);
						Implement_GetPersonPacket(p_item, *p_pack);
						p_result->insert(p_pack);
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

extern "C" __declspec(dllexport) SString * UhttGetGoodsArCode(PPSoapClientSession & rSess, const char * pToken, const char * pGoodsCode, const char * pPersonInn)
{
	char * p_code = 0;
	SString * p_result = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString arg_goods_code(pGoodsCode);
	InParamString arg_person_inn(pPersonInn);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getGoodsArCode param;
	ns1__getGoodsArCodeResponse resp;
	param.token = arg_token;
	param.barcode = arg_goods_code;
	param.inn = arg_person_inn;
	if(PreprocessCall(proxi, rSess, proxi.getGoodsArCode(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new SString;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			*p_result = resp.code;
		}
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttStatus * UhttSetImageByID(PPSoapClientSession & rSess, const char * pToken, UhttDocumentPacket & rPack)
{
	UhttStatus * p_result = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString arg_objtype(rPack.ObjTypeSymb);
	InParamString content_type(rPack.ContentType);
	InParamString content(rPack.ContentMime);
	InParamString encoding(rPack.Encoding);
	InParamString name(rPack.Name);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__dataChunk chunk;
	ns1__setImageByID param;
	ns1__setImageByIDResponse resp;
	param.token = arg_token;
	param.objectType = arg_objtype;
	param.objectID = rPack.UhttObjID;

	chunk.Number = 1;
	chunk.RawChunkSize = rPack.Size;
	chunk.DataBase64 = content;
	param.image = &chunk;
	if(PreprocessCall(proxi, rSess, proxi.setImageByID(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = CreateResultStatus(resp.status);
	}
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <UhttBrandPacket> * UhttGetBrandByName(PPSoapClientSession & rSess, const char * pToken, const char * pName)
{
	TSCollection <UhttBrandPacket> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_name(pName);
	SString temp_buf;
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getBrandByName param;
	ns1__getBrandByNameResponse resp;
	param.token = arg_token;
	param.name = arg_name;
	if(PreprocessCall(proxi, rSess, proxi.getBrandByName(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new TSCollection <UhttBrandPacket>;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			if(resp.brandArray) {
				for(int i = 0; i < resp.__sizebrandArray; i++) {
					ns1__brand * p_item = resp.brandArray[i];
					if(p_item) {
						UhttBrandPacket * p_pack = new UhttBrandPacket;
						THROW(p_pack);
						p_pack->ID = p_item->ID;
						(temp_buf = p_item->Name).Transf(CTRANSF_UTF8_TO_INNER).CopyTo(p_pack->Name, sizeof(p_pack->Name));
						p_pack->OwnerID = p_item->OwnerID;
						p_result->insert(p_pack);
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

extern "C" __declspec(dllexport) int UhttGetGoodsRefList(PPSoapClientSession & rSess, const char * pToken, TSArray <UhttCodeRefItem> & rList)
{
	int    ok = 0;
	ns1__getGoodsObjRefList param;
	if(rList.getCount()) {
		uint   i;
		InParamString arg_token(pToken);
		SString temp_buf;
		WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
		gSoapClientInit(&proxi, 0, 0);
		ns1__getGoodsObjRefListResponse resp;
		TSCollection <ns1__objRefItem> ref_list;
		TSCollection <InParamString> arglist_code;
		param.token = arg_token;
		param.__sizerefList = 0;
		param.refList = 0;
		{
			for(i = 0; i < rList.getCount(); i++) {
				const UhttCodeRefItem & r_item = rList.at(i);
				ns1__objRefItem * p_new_item = new ns1__objRefItem;
				THROW(p_new_item);

				InParamString * p_arg_code = new InParamString(r_item.Code);
				THROW(p_arg_code);
				arglist_code.insert(p_arg_code);

				p_new_item->ID = r_item.PrivateID;
				p_new_item->Code = *p_arg_code;
				p_new_item->UhttObjID = 0;
				ref_list.insert(p_new_item);
			}
			param.refList = static_cast<ns1__objRefItem **>(SAlloc::C(ref_list.getCount(), sizeof(ns1__objRefItem*)));
			THROW(param.refList);
			for(i = 0; i < ref_list.getCount(); i++) {
				param.refList[i] = ref_list.at(i);
				param.__sizerefList++;
			}
		}
		if(PreprocessCall(proxi, rSess, proxi.getGoodsObjRefList(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
			ok = -1;
			for(i = 0; i < (uint)resp.__sizerefList; i++) {
				const ns1__objRefItem * p_ret_item = resp.refList[i];
				if(p_ret_item && p_ret_item->UhttObjID && p_ret_item->Code[0]) {
					for(uint sp = 0; rList.lsearch(p_ret_item->Code, &sp, PTR_CMPFUNC(Pchar), offsetof(UhttCodeRefItem, Code)); sp++) {
						rList.at(sp).UhttID = p_ret_item->UhttObjID;
						if(ok < 0)
							ok = 1;
						else
							ok++;
					}
				}
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	ZFREE(param.refList);
	return ok;
}

static UhttGoodsPacket * UhttGoodsOuterToInner(const ns1__goods * pOuter)
{
	UhttGoodsPacket * p_result = new UhttGoodsPacket;
	if(pOuter && p_result) {
		SString temp_buf;
		p_result->ID = pOuter->ID;
		p_result->BrandID = pOuter->BrandID; // @v8.2.10
		p_result->ManufID = pOuter->ManufactorID; // @v8.3.5
		(temp_buf = pOuter->Name).Transf(CTRANSF_UTF8_TO_INNER).CopyTo(p_result->Name, sizeof(p_result->Name));
		(temp_buf = pOuter->SingleBarcode).Transf(CTRANSF_UTF8_TO_INNER).CopyTo(p_result->SingleBarcode, sizeof(p_result->SingleBarcode));
		p_result->Package = pOuter->Package;
		if(pOuter->Barcodes) {
			for(int j = 0; j < pOuter->__sizeBarcodes; j++) {
				ns1__barcode * p_barcode = pOuter->Barcodes[j];
				if(p_barcode) {
					UhttGoodsPacket::Barcode bc;
					(temp_buf = p_barcode->Code).Transf(CTRANSF_UTF8_TO_INNER).CopyTo(bc.Code, sizeof(bc.Code));
					if(temp_buf.NotEmptyS()) {
						bc.GoodsID = p_result->ID;
						bc.Qtty = p_barcode->Package;
						THROW(p_result->BarcodeList.insert(&bc));
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

extern "C" __declspec(dllexport) UhttGoodsPacket * UhttGetGoodsByID(PPSoapClientSession & rSess, const char * pToken, int id)
{
	UhttGoodsPacket * p_result = 0;
	InParamString arg_token(pToken);
	SString temp_buf;
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getGoodsByID param;
	ns1__getGoodsByIDResponse resp;
	param.token = arg_token;
	param.id = id;
	if(PreprocessCall(proxi, rSess, proxi.getGoodsByID(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = UhttGoodsOuterToInner(resp.goods);
		PPSoapRegisterResultPtr(p_result);
	}
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <UhttGoodsPacket> * UhttGetGoodsByCode(PPSoapClientSession & rSess, const char * pToken, const char * pCode)
{
	TSCollection <UhttGoodsPacket> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_code(pCode);
	SString temp_buf;
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getGoodsByCode param;
	ns1__getGoodsByCodeResponse resp;
	param.token = arg_token;
	param.code = arg_code;
	if(PreprocessCall(proxi, rSess, proxi.getGoodsByCode(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new TSCollection <UhttGoodsPacket>;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			if(resp.goodsArray) {
				for(int i = 0; i < resp.__sizegoodsArray; i++) {
					UhttGoodsPacket * p_pack = UhttGoodsOuterToInner(resp.goodsArray[i]);
					if(p_pack)
						p_result->insert(p_pack);
				}
			}
		}
	}
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <UhttGoodsPacket> * UhttGetGoodsByName(PPSoapClientSession & rSess, const char * pToken, const char * pName)
{
	SString temp_buf;
	TSCollection <UhttGoodsPacket> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_name(pName);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getGoodsByName param;
	ns1__getGoodsByNameResponse resp;
	param.token = arg_token;
	param.name = arg_name;
	if(PreprocessCall(proxi, rSess, proxi.getGoodsByName(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new TSCollection <UhttGoodsPacket>;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			if(resp.goodsArray) {
				for(int i = 0; i < resp.__sizegoodsArray; i++) {
					UhttGoodsPacket * p_pack = UhttGoodsOuterToInner(resp.goodsArray[i]);
					if(p_pack)
						p_result->insert(p_pack);
				}
			}
		}
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttStatus * UhttCreateGoods(PPSoapClientSession & rSess, const char * pToken, const UhttGoodsPacket & rPack)
{
	UhttStatus * p_result = 0;
	uint   i;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString arg_name(rPack.Name);
	InParamString arg_single_barcode(rPack.SingleBarcode);

	InParamString arg_ext_a(rPack.ExtA);
	InParamString arg_ext_b(rPack.ExtB);
	InParamString arg_ext_c(rPack.ExtC);
	InParamString arg_ext_d(rPack.ExtD);
	InParamString arg_ext_e(rPack.ExtE);

	TSCollection <ns1__tagValue> tag_list;
	TSCollection <InParamString> arglist_tag;

	ns1__goods goods_param;
	goods_param.ID = rPack.ID;
	goods_param.OKOF = 0;
	goods_param.Barcodes = 0;
	goods_param.BrandID = rPack.BrandID; // @v8.2.10 0-->rPack.BrandID
	goods_param.ManufactorID = rPack.ManufID; // @v8.3.5
	goods_param.Brutto = 0.0;
	goods_param.ClassID = 0;
	goods_param.Energy = arg_ext_d;
	goods_param.ExpiryPeriod = 0;
	goods_param.GroupID = 0;
	goods_param.Height = 0;
	goods_param.Length = 0;
	goods_param.Width = 0;
	goods_param.Usages = arg_ext_e;
	goods_param.Ingredients = arg_ext_c;
	goods_param.Name = arg_name;
	goods_param.PhUnitPerUnit = 0.0;
	goods_param.PhUnitID = 0;
	goods_param.Standard = arg_ext_b;
	goods_param.Storage = arg_ext_a;
	goods_param.TaxGroupID = 0;
	goods_param.TypeID = 0;
	goods_param.UnitID = 0;
	goods_param.SingleBarcode = arg_single_barcode;
	goods_param.Package = rPack.Package;

	for(i = 0; i < rPack.TagList.getCount(); i++) {
		const UhttTagItem * p_item = rPack.TagList.at(i);
		ns1__tagValue * p_tag = new ns1__tagValue;
		InParamString * p_arg_symb = new InParamString(p_item->Symb);
		InParamString * p_arg_val = new InParamString(p_item->Value);
		arglist_tag.insert(p_arg_symb);
		arglist_tag.insert(p_arg_val);

		p_tag->Symb = *p_arg_symb;
		p_tag->Value = *p_arg_val;
		tag_list.insert(p_tag);
	}
	goods_param.__sizeTags = 0;
	goods_param.Tags = static_cast<ns1__tagValue **>(SAlloc::C(tag_list.getCount(), sizeof(ns1__tagValue*)));
	for(i = 0; i < tag_list.getCount(); i++) {
		goods_param.Tags[i] = tag_list.at(i);
		goods_param.__sizeTags++;
	}

	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__createGoods param;
	ns1__createGoodsResponse resp;
	param.token = arg_token;
	param.goods = &goods_param;
	if(PreprocessCall(proxi, rSess, proxi.createGoods(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = CreateResultStatus(resp.status);
	}
	SAlloc::F(goods_param.Tags);
	return p_result;
}

static UhttLocationPacket * UhttLocationOuterToInner(const ns1__location * pOuter)
{
	UhttLocationPacket * p_result = 0;
	if(pOuter) {
		p_result = new UhttLocationPacket;
		if(p_result) {
			p_result->ID = pOuter->ID;
			p_result->ParentID = pOuter->ParentID;
			p_result->Type = pOuter->Type;
			p_result->CityID = pOuter->CityID;
			p_result->OwnerPersonID = pOuter->OwnerID;
			p_result->Latitude = pOuter->Latitude;
			p_result->Longitude = pOuter->Longitude;
			p_result->Flags = pOuter->Flags;
			(p_result->Name = pOuter->Name).Transf(CTRANSF_UTF8_TO_INNER);
			(p_result->Code = pOuter->Code).Transf(CTRANSF_UTF8_TO_INNER);
			(p_result->PostalCode = pOuter->ZIP).Transf(CTRANSF_UTF8_TO_INNER);
			(p_result->Address = pOuter->Address).Transf(CTRANSF_UTF8_TO_INNER);
			(p_result->Phone = pOuter->Phone).Transf(CTRANSF_UTF8_TO_INNER);
			(p_result->Contact = pOuter->Contact).Transf(CTRANSF_UTF8_TO_INNER);
		}
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttLocationPacket * UhttGetLocationByID(PPSoapClientSession & rSess, const char * pToken, int ID)
{
	UhttLocationPacket * p_result = 0;
	InParamString arg_token(pToken);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getLocationByID param;
	ns1__getLocationByIDResponse resp;
	param.token = arg_token;
	param.id = ID;
	if(PreprocessCall(proxi, rSess, proxi.getLocationByID(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = UhttLocationOuterToInner(resp.location);
		PPSoapRegisterResultPtr(p_result);
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttLocationPacket * UhttGetLocationByCode(PPSoapClientSession & rSess, const char * pToken, const char * pCode)
{
	UhttLocationPacket * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_code(pCode);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getLocationByCode param;
	ns1__getLocationByCodeResponse resp;
	param.token = arg_token;
	param.code = arg_code;
	if(PreprocessCall(proxi, rSess, proxi.getLocationByCode(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = UhttLocationOuterToInner(resp.location);
		PPSoapRegisterResultPtr(p_result);
	}
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <UhttLocationPacket> * UhttGetLocationListByPhone(PPSoapClientSession & rSess, const char * pToken, const char * pPhone)
{
	SString temp_buf;
	TSCollection <UhttLocationPacket> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_phone(pPhone);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getLocationByPhone param;
	ns1__getLocationByPhoneResponse resp;
	param.token = arg_token;
	param.phone = arg_phone;
	if(PreprocessCall(proxi, rSess, proxi.getLocationByPhone(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new TSCollection <UhttLocationPacket>;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			if(resp.locationArray) {
				for(int i = 0; i < resp.__sizelocationArray; i++) {
					UhttLocationPacket * p_pack = UhttLocationOuterToInner(resp.locationArray[i]);
					if(p_pack)
						p_result->insert(p_pack);
				}
			}
		}
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttStatus * UhttCreateStandaloneLocation(PPSoapClientSession & rSess, const char * pToken, const UhttLocationPacket & rPack)
{
	UhttStatus * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_name(rPack.Name);
	InParamString arg_code(rPack.Code);
	InParamString arg_zip(rPack.PostalCode);
	InParamString arg_address(rPack.Address);
	InParamString arg_phone(rPack.Phone);
	InParamString arg_email(rPack.EMail);
	InParamString arg_contact(rPack.Contact);

	ns1__location loc_param;
	loc_param.ID = rPack.ID;
	loc_param.ParentID = rPack.ParentID;
	loc_param.CityID = rPack.CityID;
	loc_param.Type = 3; // LOCTYP_ADDRESS
	loc_param.Flags = rPack.Flags | 0x0200; // LOCF_STANDALONE
	loc_param.OwnerID = rPack.OwnerPersonID;
	loc_param.Name = arg_name;
	loc_param.Code = arg_code;
	loc_param.ZIP = arg_zip;
	loc_param.Address = arg_address;
	loc_param.Phone = arg_phone;
	loc_param.EMail = arg_email;
	loc_param.Contact = arg_contact;
	loc_param.Latitude = rPack.Latitude;
	loc_param.Longitude = rPack.Longitude;

	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__createStandaloneLocation param;
	ns1__createStandaloneLocationResponse resp;
	param.token = arg_token;
	param.location = &loc_param;
	if(PreprocessCall(proxi, rSess, proxi.createStandaloneLocation(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = CreateResultStatus(resp.status);
	}
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <UhttSpecSeriesPacket> * UhttGetSpecSeriesByPeriod(PPSoapClientSession & rSess, const char * pToken, const char * pPeriod)
{
	SString temp_buf;
	TSCollection <UhttSpecSeriesPacket> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_period(pPeriod);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getSpecSeriesByPeriod param;
	ns1__getSpecSeriesByPeriodResponse resp;
	param.token = arg_token;
	param.period = arg_period;
	if(PreprocessCall(proxi, rSess, proxi.getSpecSeriesByPeriod(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new TSCollection <UhttSpecSeriesPacket>;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			if(resp.specSeriesArray) {
				for(int i = 0; i < resp.__sizespecSeriesArray; i++) {
					ns1__specSeries * p_specseries_item = resp.specSeriesArray[i];
					if(p_specseries_item) {
						UhttSpecSeriesPacket * p_pack = new UhttSpecSeriesPacket;
						THROW(p_pack);
						p_pack->ID = p_specseries_item->ID;
						p_pack->GoodsID = p_specseries_item->GoodsID;
						p_pack->ManufID = p_specseries_item->ManufID;
						p_pack->ManufCountryID = p_specseries_item->ManufCountryID;
						p_pack->LabID = p_specseries_item->LabID;
						(p_pack->Serial = p_specseries_item->Serial).Transf(CTRANSF_UTF8_TO_INNER);
						(p_pack->Barcode = p_specseries_item->Barcode).Transf(CTRANSF_UTF8_TO_INNER);
						(p_pack->GoodsName = p_specseries_item->GoodsName).Transf(CTRANSF_UTF8_TO_INNER);
						(p_pack->ManufName = p_specseries_item->ManufName).Transf(CTRANSF_UTF8_TO_INNER);
						p_pack->InfoDate = p_specseries_item->InfoDate;
						p_pack->InfoKind = p_specseries_item->InfoKind;
						(p_pack->InfoIdent = p_specseries_item->InfoIdent).Transf(CTRANSF_UTF8_TO_INNER);
						p_pack->AllowDate = p_specseries_item->AllowDate;
						(p_pack->AllowNumber = p_specseries_item->AllowNumber).Transf(CTRANSF_UTF8_TO_INNER);
						(p_pack->LetterType = p_specseries_item->LetterType).Transf(CTRANSF_UTF8_TO_INNER);
						p_pack->Flags = p_specseries_item->Flags;
						p_result->insert(p_pack);
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

extern "C" __declspec(dllexport) UhttStatus * UhttCreateSpecSeries(PPSoapClientSession & rSess, const char * pToken, const UhttSpecSeriesPacket & rPack)
{
	UhttStatus * p_result = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString serial(rPack.Serial);
	InParamString barcode(rPack.Barcode);
	InParamString goods_name(rPack.GoodsName);
	InParamString manuf_name(rPack.ManufName);
	InParamString info_date(rPack.InfoDate);
	InParamString info_ident(rPack.InfoIdent);
	InParamString allow_date(rPack.AllowDate);
	InParamString allow_number(rPack.AllowNumber);
	InParamString letter_type(rPack.LetterType);
	ns1__specSeries specSeries_param;
	specSeries_param.ID = rPack.ID;
	specSeries_param.GoodsID = rPack.GoodsID;
	specSeries_param.ManufID = rPack.ManufID;
	specSeries_param.ManufCountryID = 0;
	specSeries_param.LabID = rPack.LabID;
	specSeries_param.Serial = serial;
	specSeries_param.Barcode = barcode;
	specSeries_param.GoodsName = goods_name;
	specSeries_param.ManufName = manuf_name;
	specSeries_param.InfoDate = info_date;
	specSeries_param.InfoKind = rPack.InfoKind;
	specSeries_param.InfoIdent = info_ident;
	specSeries_param.AllowDate = allow_date;
	specSeries_param.AllowNumber = allow_number;
	specSeries_param.LetterType = letter_type;
	specSeries_param.Flags = rPack.Flags;
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__createSpecSeries param;
	ns1__createSpecSeriesResponse resp;
	param.token = arg_token;
	param.series = &specSeries_param;
	if(PreprocessCall(proxi, rSess, proxi.createSpecSeries(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = CreateResultStatus(resp.status);
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttStatus * UhttCreateSCard(PPSoapClientSession & rSess, const char * pToken, UhttSCardPacket & rPack)
{
	UhttStatus * p_result = 0;
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;
	ns1__dateTime arg_issue_date;
	ns1__dateTime arg_expiry;
	ns1__sCard instance;
	instance.Code = GetDynamicParamString(rPack.Code, arg_str_pool);
	instance.Hash = GetDynamicParamString(rPack.Hash, arg_str_pool);
	instance.Phone = GetDynamicParamString(rPack.Phone, arg_str_pool);
	instance.Memo = GetDynamicParamString(rPack.Memo, arg_str_pool);
	instance.ID = rPack.ID;
	instance.Flags = rPack.Flags;
	if(rPack.IssueDate.Date.NotEmpty()) {
		arg_issue_date.Date = GetDynamicParamString(rPack.IssueDate.Date, arg_str_pool);
		arg_issue_date.Time = GetDynamicParamString(rPack.IssueDate.Time, arg_str_pool);
		instance.IssueDate = &arg_issue_date;
	}
	if(rPack.Expiry.Date.NotEmpty()) {
		arg_expiry.Date = GetDynamicParamString(rPack.Expiry.Date, arg_str_pool);
		arg_expiry.Time = GetDynamicParamString(rPack.Expiry.Time, arg_str_pool);
		instance.Expiry = &arg_expiry;
	}
	instance.OwnerID = rPack.OwnerID;
	instance.PDis = rPack.PDis;
	instance.SeriesID = rPack.SeriesID;
	instance.Overdraft = rPack.Overdraft;
	instance.Credit = rPack.Credit;
	instance.Debit = rPack.Debit;
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__createSCard param;
	ns1__createSCardResponse resp;
	param.token = GetDynamicParamString(pToken, arg_str_pool);
	param.instance = &instance;
	if(PreprocessCall(proxi, rSess, proxi.createSCard(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = CreateResultStatus(resp.status);
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttSCardPacket * UhttGetSCardByNumber(PPSoapClientSession & rSess, const char * pToken, const char * pNumber)
{
	SString temp_buf;
	UhttSCardPacket * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_number(pNumber);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getSCardByNumber param;
	ns1__getSCardByNumberResponse resp;
	param.token = arg_token;
	param.number = arg_number;
	if(PreprocessCall(proxi, rSess, proxi.getSCardByNumber(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new UhttSCardPacket;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			THROW(resp.scard);
			p_result->ID = resp.scard->ID;
			p_result->SeriesID = resp.scard->SeriesID;
			p_result->OwnerID = resp.scard->OwnerID;
			(p_result->Code = resp.scard->Code).Transf(CTRANSF_UTF8_TO_INNER);
			(p_result->Hash = resp.scard->Hash).Transf(CTRANSF_UTF8_TO_INNER);
			if(resp.scard->IssueDate) {
				p_result->IssueDate.Date = resp.scard->IssueDate->Date;
				p_result->IssueDate.Time = resp.scard->IssueDate->Time;
			}
			if(resp.scard->Expiry) {
				p_result->IssueDate.Date = resp.scard->Expiry->Date;
				p_result->IssueDate.Time = resp.scard->Expiry->Time;
			}
			p_result->PDis = resp.scard->PDis;
			p_result->Overdraft = resp.scard->Overdraft;
			p_result->Debit = resp.scard->Debit;
			p_result->Credit = resp.scard->Credit;
			p_result->Rest = resp.scard->Rest;
			p_result->Flags = resp.scard->Flags;
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) UhttStatus * UhttCreateSCardCheck
	(PPSoapClientSession & rSess, const char * pToken, const char * pLocSymb, const char * pSCardNumber, const UhttCheckPacket & rPack)
{
	UhttStatus * p_result = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString arg_loc_symb(pLocSymb);
	InParamString arg_scard_number(pSCardNumber);
	InParamString arg_date(rPack.Dtm.Date);
	InParamString arg_time(rPack.Dtm.Time);
	ns1__check check_param;
	ns1__dateTime arg_dtm;
	check_param.PosNodeID = rPack.PosNodeID;
	check_param.LocID = rPack.LocID;
	{
		arg_dtm.Date = arg_date;
		arg_dtm.Time = arg_time;
		check_param.Dtm = &arg_dtm;
	}
	check_param.SCardID = rPack.SCardID;
	check_param.Amount = rPack.Amount;
	check_param.Discount = rPack.Discount;
	check_param.Flags = rPack.Flags;
	// @todo Process items
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__createSCardCheck param;
	ns1__createSCardCheckResponse resp;
	param.token = arg_token;
	param.locSymb = arg_loc_symb;
	param.scardNumber = arg_scard_number;
	param.check = &check_param;
	if(PreprocessCall(proxi, rSess, proxi.createSCardCheck(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = CreateResultStatus(resp.status);
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttStatus * UhttDepositSCardAmount
	(PPSoapClientSession & rSess, const char * pToken, const char * pNumber, const double amount)
{
	UhttStatus * p_result = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString arg_number(pNumber);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__depositSCardAmount param;
	ns1__depositSCardAmountResponse resp;
	param.token = arg_token;
	param.number = arg_number;
	param.amount = amount;
	if(PreprocessCall(proxi, rSess, proxi.depositSCardAmount(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = CreateResultStatus(resp.status);
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttStatus * UhttWithdrawSCardAmount
	(PPSoapClientSession & rSess, const char * pToken, const char * pNumber, const double amount)
{
	UhttStatus * p_result = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString arg_number(pNumber);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__withdrawSCardAmount param;
	ns1__withdrawSCardAmountResponse resp;
	param.token = arg_token;
	param.number = arg_number;
	param.amount = amount;
	if(PreprocessCall(proxi, rSess, proxi.withdrawSCardAmount(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = CreateResultStatus(resp.status);
	}
	return p_result;
}

extern "C" __declspec(dllexport) int UhttGetSCardRest
	(PPSoapClientSession & rSess, const char * pToken, const char * pNumber, const char * pDate, double & rRest)
{
	rRest = 0.0;

	int     ok = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString arg_number(pNumber);
	InParamString arg_date(pDate);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getSCardRest param;
	ns1__getSCardRestResponse resp;
	param.token = arg_token;
	param.number = arg_number;
	param.date = arg_date;
	if(PreprocessCall(proxi, rSess, proxi.getSCardRest(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		rRest = resp.rest;
		ok = 1;
	}
	return ok;
}

extern "C" __declspec(dllexport) int UhttStartDataTransfer(PPSoapClientSession & rSess,
	const char * pToken, const char * pName, int64 totalRawSize, int32 chunkCount)
{
	int     transfer_id = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString arg_name(pName);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__dataTransferInfo info;
	ns1__startDataTransfer param;
	ns1__startDataTransferResponse resp;
	info.Name = arg_name;
	info.ChunksNumber = chunkCount;
	info.Size = totalRawSize;
	param.token = arg_token;
	param.info = &info;
	if(PreprocessCall(proxi, rSess, proxi.startDataTransfer(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		transfer_id = resp.transferID;
	}
	return transfer_id;
}

extern "C" __declspec(dllexport) int UhttTransferData(PPSoapClientSession & rSess,
	const char * pToken, int transferID, int chunkNumber, int64 rawChunkSize, char * pMime64Data)
{
	int    ok = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__dataChunk chunk;
	ns1__putDataChunk param;
	ns1__putDataChunkResponse resp;
	chunk.Number = chunkNumber;
	chunk.RawChunkSize = rawChunkSize;
	chunk.DataBase64 = pMime64Data;
	param.token = arg_token;
	param.transferID = transferID;
	param.chunk = &chunk;
	if(PreprocessCall(proxi, rSess, proxi.putDataChunk(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		ok = 1;
	}
	return ok;
}

extern "C" __declspec(dllexport) int UhttFinishTransferData(PPSoapClientSession & rSess, const char * pToken, int transferID)
{
	int    ok = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__dataTransferInfo info;
	ns1__finishDataTransfer param;
	ns1__finishDataTransferResponse resp;
	param.token = arg_token;
	param.transferID = transferID;
	param.checkSum = 0;
	if(PreprocessCall(proxi, rSess, proxi.finishDataTransfer(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		ok = 1;
	}
	return ok;
}

extern "C" __declspec(dllexport) int UhttAddFileVersion(PPSoapClientSession & rSess, const char * pToken,
	int transferID, const char * pKey, const char * pLabel, const char * pMemo)
{
	int    ok = 0;
	InParamString arg_token(pToken);
	InParamString arg_key(pKey);
	InParamString arg_label(pLabel);
	InParamString arg_memo(pMemo);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__DCAddFileVersion param;
	ns1__DCAddFileVersionResponse resp;
	param.token = arg_token;
	param.transferID = transferID;
	param.key = arg_key;
	param.label = arg_label;
	param.memo = arg_memo;
	if(PreprocessCall(proxi, rSess, proxi.DCAddFileVersion(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		ok = 1;
	}
	return ok;
}

static void FASTCALL UhttQuotPacketTo_ns1__quote(const UhttQuotPacket * pSrc, ns1__quote * pDest)
{
	pDest->GoodsID = pSrc->GoodsID;
	pDest->SellerID = pSrc->SellerID;
	pDest->LocationID = pSrc->LocID;
	pDest->BuyerID = pSrc->BuyerID;
	pDest->CurrencyID = pSrc->CurrID;
	pDest->MinQtty = R0(pSrc->MinQtty);
	pDest->Value = pSrc->Value;
	pDest->Flags = pSrc->Flags;
}

extern "C" __declspec(dllexport) int UhttSetQuot(PPSoapClientSession & rSess, const char * pToken, UhttQuotPacket & rPack)
{
	int    ok = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	ns1__quote quote_param;

	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	UhttQuotPacketTo_ns1__quote(&rPack, &quote_param);

	ns1__setGoodsQuote param;
	ns1__setGoodsQuoteResponse resp;
	param.token = arg_token;
	param.quote = &quote_param;
	if(PreprocessCall(proxi, rSess, proxi.setGoodsQuote(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		ok = 1;
	}
	return ok;
}

extern "C" __declspec(dllexport) TSCollection <UhttStatus> * UhttSetQuotList(PPSoapClientSession & rSess, const char * pToken, const TSCollection <UhttQuotPacket> & rList)
{
	TSCollection <UhttStatus> * p_result = 0;
	uint   i;
	SString temp_buf;
	InParamString arg_token(pToken);
	TSCollection <ns1__quote> quot_list;

	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__setGoodsQuoteList param;
	ns1__setGoodsQuoteListResponse resp;
	{
		for(i = 0; i < rList.getCount(); i++) {
			const UhttQuotPacket * p_src_item = rList.at(i);
			ns1__quote * p_new_item = new ns1__quote;
			THROW(p_new_item);
			UhttQuotPacketTo_ns1__quote(p_src_item, p_new_item);
			quot_list.insert(p_new_item);
		}
		param.quote = static_cast<ns1__quote **>(SAlloc::C(quot_list.getCount(), sizeof(ns1__quote*)));
		THROW(param.quote);
		for(i = 0; i < quot_list.getCount(); i++) {
			param.quote[i] = quot_list.at(i);
			param.__sizequote++;
		}
	}
	param.token = arg_token;
	if(PreprocessCall(proxi, rSess, proxi.setGoodsQuoteList(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		if(resp.__sizestatus > 0) {
			THROW(p_result = new TSCollection <UhttStatus>);
			PPSoapRegisterResultPtr(p_result);
			for(i = 0; i < (uint)resp.__sizestatus; i++) {
				const ns1__statusExt * p_status_item = resp.status[i];
				if(p_status_item) {
					UhttStatus * p_new_item = CreateResultStatus(p_status_item);
					THROW(p_new_item);
					p_result->insert(p_new_item);
				}
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	ZFREE(param.quote);
	return p_result;
}

extern "C" __declspec(dllexport) int UhttCreateBill(PPSoapClientSession & rSess, const char * pToken, UhttBillPacket & rPack)
{
	int    ok = 0;
	int    result_bill_id = 0;
	uint   i;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString arg_bill_code(rPack.Code);
	InParamString arg_bill_memo(rPack.Memo);
	InParamString arg_dt(rPack.Dtm.Date);
	InParamString arg_op_symb(rPack.OpSymb);
	ns1__bill bill_param;
	TSCollection <ns1__billLine> bill_line_list;
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);

	bill_param.ID = 0/*rPack.ID*/;
	bill_param.AgentID = rPack.AgentID;
	bill_param.ArticleID = rPack.ArID;
	bill_param.CurrencyID = rPack.CurrID;
	bill_param.DlvrLocID = rPack.DlvrLocID;
	bill_param.Flags = rPack.Flags;
	bill_param.LocationID = rPack.LocID;
	bill_param.StatusID = rPack.StatusID;
	bill_param.Code = arg_bill_code;
	bill_param.Dt = arg_dt;
	bill_param.OpSymb = arg_op_symb;
	bill_param.Memo = arg_bill_memo;

	bill_param.__sizeItems = 0;
	bill_param.Items = static_cast<ns1__billLine **>(SAlloc::C(rPack.Items.getCount(), sizeof(ns1__billLine*)));
	for(i = 0; i < rPack.Items.getCount(); i++) {
		const UhttBillPacket::BillItem & r_item = rPack.Items.at(i);
		ns1__billLine * p_line = new ns1__billLine;
		p_line->ID = r_item.BillID;
		p_line->GoodsID = r_item.GoodsID;
		p_line->Flags = r_item.Flags;
		p_line->Quantity = r_item.Quantity;
		p_line->Cost   = r_item.Cost;
		p_line->Price = r_item.Price;
		p_line->Discount = r_item.Discount;
		p_line->Amount = r_item.Amount;
		bill_line_list.insert(p_line);
	}
	for(i = 0; i < bill_line_list.getCount(); i++) {
		bill_param.Items[i] = bill_line_list.at(i);
		bill_param.__sizeItems++;
	}
	ns1__createBill param;
	ns1__createBillResponse resp;
	param.token = arg_token;
	param.bill = &bill_param;
	if(PreprocessCall(proxi, rSess, proxi.createBill(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		result_bill_id = resp.status->Code;
		ok = 1;
	}
	ZFREE(bill_param.Items);
	bill_param.__sizeItems = 0;
	rPack.ID = result_bill_id;
	return ok;
}

extern "C" __declspec(dllexport) TSCollection <UhttBillPacket> * UhttGetBill(PPSoapClientSession & rSess, const char * pToken, UhttBillFilter & rFilt)
{
	SString temp_buf;
	TSCollection <UhttBillPacket> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_filt_prd_low(rFilt.Period.Low);
	InParamString arg_filt_prd_upp(rFilt.Period.Upp);
	InParamString arg_filt_dt(rFilt.Dt);
	InParamString arg_filt_opsymb(rFilt.OpSymb);
	InParamString arg_filt_sinced(rFilt.Since.Date);
	InParamString arg_filt_sincet(rFilt.Since.Time);
	ns1__dateTime arg_filt_since;

	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getBill param;
	ns1__getBillResponse resp;
	ns1__billFilter filt_param;
	ns1__datePeriod arg_period;

	filt_param.LocationID = rFilt.LocID;
	filt_param.ArticleID = rFilt.ArID;
	filt_param.CurrencyID = rFilt.CurrID;
	filt_param.AgentID = rFilt.AgentID;
	filt_param.Count = rFilt.Count;
	filt_param.Last = rFilt.Last;
	if(rFilt.Since.Date.NotEmpty()) {
		arg_filt_since.Date = arg_filt_sinced;
		arg_filt_since.Time = arg_filt_sincet;
		filt_param.Since = &arg_filt_since;
	}
	{
		arg_period.LowerDate = arg_filt_prd_low;
		arg_period.UpperDate = arg_filt_prd_upp;
		filt_param.Period = &arg_period;
	}
	filt_param.Dt = arg_filt_dt;
	filt_param.OpSymb = arg_filt_opsymb;

	param.token = arg_token;
	param.filter = &filt_param;
	if(PreprocessCall(proxi, rSess, proxi.getBill(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new TSCollection <UhttBillPacket>;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			if(resp.billArray) {
				for(int i = 0; i < resp.__sizebillArray; i++) {
					ns1__bill * p_bill = resp.billArray[i];
					if(p_bill) {
						UhttBillPacket * p_pack = p_result->CreateNewItem();
						THROW(p_pack);
						p_pack->ID = p_bill->ID;
						p_pack->LocID = p_bill->LocationID;
						p_pack->ArID = p_bill->ArticleID;
						p_pack->DlvrLocID = p_bill->DlvrLocID;
						p_pack->CurrID = p_bill->CurrencyID;
						p_pack->AgentID = p_bill->AgentID;
						p_pack->StatusID = p_bill->StatusID;
						p_pack->Flags = p_bill->Flags;
						p_pack->Uuid.FromStr(GetSoapStr(p_bill->GUID, temp_buf));
						if(p_bill->Dt)
							p_pack->Dtm.Date = p_bill->Dt;
						(p_pack->Code = p_bill->Code).Transf(CTRANSF_UTF8_TO_INNER);
						(p_pack->OpSymb = p_bill->OpSymb).Transf(CTRANSF_UTF8_TO_INNER);
						(p_pack->Memo = p_bill->Memo).Transf(CTRANSF_UTF8_TO_INNER);
						Implement_GetPersonPacket(p_bill->Contractor, p_pack->Contractor);
						if(p_bill->Items) {
							for(int j = 0; j < p_bill->__sizeItems; j++) {
								ns1__billLine * p_bill_line = p_bill->Items[j];
								if(p_bill_line) {
									UhttBillPacket::BillItem item;
									MEMSZERO(item);
									item.BillID = p_bill->ID;
									item.GoodsID = p_bill_line->GoodsID;
									(temp_buf = p_bill_line->Serial).Transf(CTRANSF_UTF8_TO_INNER);
									temp_buf.CopyTo(item.Serial, sizeof(item.Serial));
									item.Quantity = p_bill_line->Quantity;
									item.Cost = p_bill_line->Cost;
									item.Price = p_bill_line->Price;
									item.Discount = p_bill_line->Discount;
									item.Amount = p_bill_line->Amount;
									item.Flags = p_bill_line->Flags;
									p_pack->Items.insert(&item);
								}
							}
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

extern "C" __declspec(dllexport) TSCollection <UhttQuotPacket> * UhttGetQuot(PPSoapClientSession & rSess, const char * pToken, UhttQuotFilter & rFilt)
{
	SString temp_buf;
	TSCollection <UhttQuotPacket> * p_result = 0;
	InParamString arg_token(pToken);

	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getGoodsQuote param;
	ns1__getGoodsQuoteResponse resp;
	ns1__quoteFilter filt_param;

	filt_param.GoodsID = rFilt.GoodsID;
	filt_param.GroupID = rFilt.GroupID;
	filt_param.BrandID = rFilt.BrandID;
	filt_param.SellerID = rFilt.SellerID;
	filt_param.LocationID = rFilt.LocationID;
	filt_param.BuyerID = rFilt.BuyerID;
	filt_param.AndFlags = rFilt.AndFlags;
	filt_param.NotFlags = rFilt.NotFlags;

	param.token = arg_token;
	param.filter = &filt_param;
	if(PreprocessCall(proxi, rSess, proxi.getGoodsQuote(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new TSCollection <UhttQuotPacket>;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			if(resp.quoteArray) {
				for(int i = 0; i < resp.__sizequoteArray; i++) {
					ns1__quote * p_item = resp.quoteArray[i];
					if(p_item) {
						UhttQuotPacket * p_pack = p_result->CreateNewItem();
						THROW(p_pack);
						p_pack->GoodsID = p_item->GoodsID;
						p_pack->SellerID = p_item->SellerID;
						p_pack->LocID = p_item->LocationID;
						p_pack->BuyerID = p_item->BuyerID;
						p_pack->CurrID = p_item->CurrencyID;
						p_pack->Value = p_item->Value;
						p_pack->MinQtty = p_item->MinQtty;
						//p_pack->ActualPeriod = p_item->ActialPeriod;
						//p_pack->ChangesDate = p_item->ChangesDate;
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

extern "C" __declspec(dllexport) TSCollection <UhttGoodsRestListItem> * UhttGetGoodsRestList(PPSoapClientSession & rSess, const char * pToken, int uhttGoodsID)
{
	SString temp_buf;
	TSCollection <UhttGoodsRestListItem> * p_result = 0;
	InParamString arg_token(pToken);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getGoodsRestList param;
	ns1__getGoodsRestListResponse resp;
	param.token = arg_token;
	param.goodsID = uhttGoodsID;
	if(PreprocessCall(proxi, rSess, proxi.getGoodsRestList(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new TSCollection <UhttGoodsRestListItem>;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			if(resp.goodsRestList) {
				for(int i = 0; i < resp.__sizegoodsRestList; i++) {
					ns1__goodsRestValue * p_item = resp.goodsRestList[i];
					if(p_item) {
						UhttGoodsRestListItem * p_pack = p_result->CreateNewItem();
						THROW(p_pack);
						p_pack->GoodsID = p_item->GoodsID;
						p_pack->LocID = p_item->LocationID;
						p_pack->LocCode = p_item->LocationCode;
						(p_pack->LocAddr = p_item->LocationAddress).Transf(CTRANSF_UTF8_TO_INNER);
						(p_pack->Name    = p_item->OrgName).Transf(CTRANSF_UTF8_TO_INNER);
						p_pack->Rest = p_item->Rest;
						p_pack->Price = p_item->QuoteVal;
						if(p_item->RestBillDt) {
							p_pack->RestDtm.Date = p_item->RestBillDt->Date;
							p_pack->RestDtm.Time = p_item->RestBillDt->Time;
						}
						if(p_item->QuoteDt) {
							p_pack->PriceDtm.Date = p_item->QuoteDt->Date;
							p_pack->PriceDtm.Time = p_item->QuoteDt->Time;
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

extern "C" __declspec(dllexport) TSCollection <UhttDCFileVersionInfo> * UhttDCGetFileVersionList(PPSoapClientSession & rSess, const char * pToken, const char * pKey)
{
	SString temp_buf;
	TSCollection <UhttDCFileVersionInfo> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_key(pKey);

	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__DCGetFileVersionList          param;
	ns1__DCGetFileVersionListResponse  resp;
	param.token   = arg_token;
	param.key     = arg_key;
	if(PreprocessCall(proxi, rSess, proxi.DCGetFileVersionList(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new TSCollection <UhttDCFileVersionInfo>;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			if(resp.verArray) {
				for(int i = 0; i < resp.__sizeverArray; i++) {
					ns1__dcFileVersionInfo * p_item = resp.verArray[i];
					if(p_item) {
						UhttDCFileVersionInfo * p_pack = p_result->CreateNewItem();
						THROW(p_pack);
						p_pack->ID        = p_item->ID;
						(p_pack->Key      = p_item->Key).Transf(CTRANSF_UTF8_TO_INNER);
						(p_pack->Title    = p_item->Title).Transf(CTRANSF_UTF8_TO_INNER);
						(p_pack->Name     = p_item->Name).Transf(CTRANSF_UTF8_TO_INNER);
						(p_pack->Label    = p_item->Label).Transf(CTRANSF_UTF8_TO_INNER);
						p_pack->Revision  = p_item->Revision;
						p_pack->Ts        = p_item->Ts->nanos; // наносекунд
						p_pack->Size      = static_cast<long>(p_item->Size);
						p_pack->Flags     = p_item->Flags;
						(p_pack->Memo     = p_item->Memo).Transf(CTRANSF_UTF8_TO_INNER);
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

static int Implement_GetWorkbookPacket(const ns1__workbook * pSp, UhttWorkbookItemPacket & rP)
{
	int    ok = 1;
	if(pSp) {
#define CPYFLD(f) rP.f = pSp->f
		CPYFLD(ID);
		CPYFLD(Type);
		CPYFLD(ParentID);
		CPYFLD(LinkID);
		CPYFLD(CssID);
		CPYFLD(Flags);
		CPYFLD(KeywordCount);
		CPYFLD(KeywordDilute);
#undef CPYFLD
		(rP.Name = pSp->Name).Transf(CTRANSF_UTF8_TO_INNER);
		(rP.Symb = pSp->Symb).Transf(CTRANSF_UTF8_TO_INNER);
		rP.Dtm.Date = pSp->Dt;
		rP.Dtm.Time = pSp->Tm;
		rP.ModifDtm.Date = pSp->ModifDt; // @v9.3.8
		rP.ModifDtm.Time = pSp->ModifTm; // @v9.3.8
		rP.ContentModifDtm.Date = pSp->ContentModifDt; // @v9.3.7
		rP.ContentModifDtm.Time = pSp->ContentModifTm; // @v9.3.7
		(rP.Version = pSp->Version).Transf(CTRANSF_UTF8_TO_INNER);
		(rP.Descr = pSp->Descr).Transf(CTRANSF_UTF8_TO_INNER);
		if(pSp->Tags) {
			for(int j = 0; j < pSp->__sizeTags; j++) {
				ns1__tagValue * p_tag = pSp->Tags[j];
				if(p_tag) {
					UhttTagItem * p_new_item = rP.TagList.CreateNewItem();
					THROW(p_new_item);
					(p_new_item->Symb = p_tag->Symb).Transf(CTRANSF_UTF8_TO_INNER);
					(p_new_item->Value = p_tag->Value).Transf(CTRANSF_UTF8_TO_INNER);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

extern "C" __declspec(dllexport) UhttWorkbookItemPacket * UhttGetWorkbookItemByID(PPSoapClientSession & rSess, const char * pToken, int id)
{
	UhttWorkbookItemPacket * p_result = 0;
	InParamString arg_token(pToken);
	SString temp_buf;
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getWorkbookItemByID param;
	ns1__getWorkbookItemByIDResponse resp;
	param.token = arg_token;
	param.id = id;
	if(PreprocessCall(proxi, rSess, proxi.getWorkbookItemByID(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		if(resp.workbookItem) {
			THROW(p_result = new UhttWorkbookItemPacket);
			PPSoapRegisterResultPtr(p_result);
			THROW(Implement_GetWorkbookPacket(resp.workbookItem, *p_result));
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) UhttWorkbookItemPacket * UhttGetWorkbookItemByCode(PPSoapClientSession & rSess, const char * pToken, const char * pCode)
{
	UhttWorkbookItemPacket * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_code(pCode);
	SString temp_buf;
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getWorkbookItemByCode param;
	ns1__getWorkbookItemByCodeResponse resp;
	param.token = arg_token;
	param.code = arg_code;
	if(PreprocessCall(proxi, rSess, proxi.getWorkbookItemByCode(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		if(resp.workbookItem) {
			THROW(p_result = new UhttWorkbookItemPacket);
			PPSoapRegisterResultPtr(p_result);
			THROW(Implement_GetWorkbookPacket(resp.workbookItem, *p_result));
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <UhttWorkbookItemPacket> * UhttGetWorkbookListByParentCode(PPSoapClientSession & rSess, const char * pToken, const char * pParentCode)
{
	TSCollection <UhttWorkbookItemPacket> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_code(pParentCode);
	SString temp_buf;
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getOwnWorkbookItems param;
	ns1__getOwnWorkbookItemsResponse resp;
	param.token = arg_token;
	param.parentCode = arg_code;
	if(PreprocessCall(proxi, rSess, proxi.getOwnWorkbookItems(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new TSCollection <UhttWorkbookItemPacket>;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			if(resp.workbookItemArray) {
				for(int i = 0; i < resp.__sizeworkbookItemArray; i++) {
					ns1__workbook * p_item = resp.workbookItemArray[i];
					if(p_item) {
						UhttWorkbookItemPacket * p_pack = p_result->CreateNewItem();
						THROW(p_pack);
						THROW(Implement_GetWorkbookPacket(p_item, *p_pack));
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

extern "C" __declspec(dllexport) UhttDocumentPacket * UhttGetWorkbookContentByID(PPSoapClientSession & rSess, const char * pToken, int id)
{
	UhttDocumentPacket * p_result = 0;
	InParamString arg_token(pToken);

	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);

	ns1__getWorkbookItemContent param;
	ns1__getWorkbookItemContentResponse resp;
	param.id = id;
	param.token = arg_token;
	if(PreprocessCall(proxi, rSess, proxi.getWorkbookItemContent(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		if(resp.content) {
			p_result = new UhttDocumentPacket;
			if(p_result) {
				PPSoapRegisterResultPtr(p_result);
				p_result->UhttObjID = id;
				p_result->Size = resp.content->RawChunkSize;
				p_result->ObjTypeSymb = "WORKBOOK";
				(p_result->Name = resp.content->Name).Transf(CTRANSF_UTF8_TO_INNER);
				p_result->Encoding = resp.content->Encoding;
				p_result->ContentType = resp.content->ContentType;
				p_result->ContentMime = resp.content->DataBase64;
			}
		}
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttStatus * UhttSetWorkbookContentByID(PPSoapClientSession & rSess, const char * pToken, UhttDocumentPacket & rPack)
{
	UhttStatus * p_result = 0;
	InParamString arg_token(pToken);
	InParamString content(rPack.ContentMime);

	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);

	ns1__dataChunk chunk;
	ns1__setWorkbookItemContent param;
	ns1__setWorkbookItemContentResponse resp;
	param.token = arg_token;
	param.id = rPack.UhttObjID;
	chunk.Number = 1;
	chunk.RawChunkSize = rPack.Size;
	chunk.DataBase64 = content;
	param.content = &chunk;
	if(PreprocessCall(proxi, rSess, proxi.setWorkbookItemContent(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = CreateResultStatus(resp.status);
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttStatus * UhttCreateWorkbookItem(PPSoapClientSession & rSess, const char * pToken, const UhttWorkbookItemPacket & rPack)
{
	UhttStatus * p_result = 0;
	InParamString arg_token(pToken);
	TSCollection <InParamString> arg_str_pool;
	TSCollection <ns1__tagValue> tag_list;
	ns1__workbook instance;
	instance.ID = rPack.ID;
	instance.ParentID = rPack.ParentID; // @?
	instance.CssID = rPack.CssID; // @?
	instance.LinkID = rPack.LinkID; // @?
	instance.Name = GetDynamicParamString(rPack.Name, arg_str_pool);
	instance.Symb = GetDynamicParamString(rPack.Symb, arg_str_pool);
	instance.Type = rPack.Type;
	instance.Rank = rPack.Rank;
	instance.Flags = rPack.Flags;
	instance.KeywordCount = rPack.KeywordCount;
	instance.KeywordDilute = rPack.KeywordDilute;
	instance.Dt = GetDynamicParamString(rPack.Dtm.Date, arg_str_pool);
	instance.Tm = GetDynamicParamString(rPack.Dtm.Time, arg_str_pool);
	instance.ModifDt = GetDynamicParamString(rPack.ModifDtm.Date, arg_str_pool); // @v9.3.8
	instance.ModifTm = GetDynamicParamString(rPack.ModifDtm.Time, arg_str_pool); // @v9.3.8
	instance.Version = GetDynamicParamString(rPack.Version, arg_str_pool);
	instance.Descr = GetDynamicParamString(rPack.Descr, arg_str_pool);
	instance.ContentModifDt = GetDynamicParamString(rPack.ContentModifDtm.Date, arg_str_pool);
	instance.ContentModifTm = GetDynamicParamString(rPack.ContentModifDtm.Time, arg_str_pool);
	instance.Tags = 0;
	{
		uint   i;
		for(i = 0; i < rPack.TagList.getCount(); i++) {
			const UhttTagItem * p_item = rPack.TagList.at(i);
			ns1__tagValue * p_tag = tag_list.CreateNewItem(0);
			THROW(p_tag);
			p_tag->Symb = GetDynamicParamString(p_item->Symb, arg_str_pool);
			p_tag->Value = GetDynamicParamString(p_item->Value, arg_str_pool);
		}
		instance.__sizeTags = 0;
		instance.Tags = static_cast<ns1__tagValue **>(SAlloc::C(tag_list.getCount(), sizeof(ns1__tagValue*)));
		for(i = 0; i < tag_list.getCount(); i++) {
			instance.Tags[i] = tag_list.at(i);
			instance.__sizeTags++;
		}
	}
	{
		WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
		ns1__createWorkbookItem param;
		ns1__createWorkbookItemResponse resp;
		gSoapClientInit(&proxi, 0, 0);
		param.token = arg_token;
		param.workbookItem = &instance;
		if(PreprocessCall(proxi, rSess, proxi.createWorkbookItem(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
			p_result = CreateResultStatus(resp.status);
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	SAlloc::F(instance.Tags);
	return p_result;
}
//
//
//
extern "C" __declspec(dllexport) UhttStatus * UhttCreateStyloDevice(PPSoapClientSession & rSess, const char * pToken, const UhttStyloDevicePacket & rPack)
{
	UhttStatus * p_result = 0;
	TSCollection <InParamString> arg_str_pool;
	InParamString arg_token(pToken);

	ns1__styloDevice instance;
	instance.ID = rPack.ID;
	instance.ParentID = rPack.ParentID;
	instance.Flags = rPack.Flags;
	instance.DeviceVer = rPack.DeviceVer;
	instance.Name = GetDynamicParamString(rPack.Name, arg_str_pool);
	instance.Symb = GetDynamicParamString(rPack.Symb, arg_str_pool);
	instance.RegisterTime = GetDynamicParamString(rPack.RegisterTime.T, arg_str_pool);

	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__createStyloDevice param;
	ns1__createStyloDeviceResponse resp;
	param.token = arg_token;
	param.instance = &instance;
	if(PreprocessCall(proxi, rSess, proxi.createStyloDevice(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = CreateResultStatus(resp.status);
	}
	return p_result;
}

static UhttStyloDevicePacket * UhttStyloDeviceOuterToInner(const ns1__styloDevice * pOuter)
{
	UhttStyloDevicePacket * p_result = 0;
	if(pOuter) {
		p_result = new UhttStyloDevicePacket;
		if(p_result) {
			#define CPYFLD(f) p_result->f = pOuter->f
			CPYFLD(ID);
			CPYFLD(ParentID);
			CPYFLD(Flags);
			CPYFLD(DeviceVer);
			#undef CPYFLD
			(p_result->Name = pOuter->Name).Transf(CTRANSF_UTF8_TO_INNER);
			(p_result->Symb = pOuter->Symb).Transf(CTRANSF_UTF8_TO_INNER);
			p_result->RegisterTime.T = pOuter->RegisterTime;
		}
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttStyloDevicePacket * UhttGetStyloDeviceByID(PPSoapClientSession & rSess, const char * pToken, int id)
{
	UhttStyloDevicePacket * p_result = 0;
	InParamString arg_token(pToken);
	SString temp_buf;
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getStyloDeviceByID param;
	ns1__getStyloDeviceByIDResponse resp;
	param.token = arg_token;
	param.id = id;
	if(PreprocessCall(proxi, rSess, proxi.getStyloDeviceByID(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = UhttStyloDeviceOuterToInner(resp.styloDeviceItem);
		PPSoapRegisterResultPtr(p_result);
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttStyloDevicePacket * UhttGetStyloDeviceByCode(PPSoapClientSession & rSess, const char * pToken, const char * pCode)
{
	UhttStyloDevicePacket * p_result = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString arg_code(pCode);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getStyloDeviceByCode param;
	ns1__getStyloDeviceByCodeResponse resp;
	param.token = arg_token;
	param.code = arg_code;
	if(PreprocessCall(proxi, rSess, proxi.getStyloDeviceByCode(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = UhttStyloDeviceOuterToInner(resp.styloDeviceItem);
		PPSoapRegisterResultPtr(p_result);
	}
	return p_result;
}
//
//
//
static UhttProcessorPacket * UhttProcessorOuterToInner(const ns1__processor * pOuter)
{
	UhttProcessorPacket * p_result = 0;
	if(pOuter) {
		p_result = new UhttProcessorPacket;
		if(p_result) {
			SString temp_buf;
			#define CPYFLD(f) p_result->f = pOuter->f
			CPYFLD(ID);
			CPYFLD(ParentID);
			CPYFLD(Kind);
			CPYFLD(Flags);
			CPYFLD(LocID);
			CPYFLD(LinkObjType);
			CPYFLD(LinkObjID);
			CPYFLD(CipPersonKindID);
			CPYFLD(CipMax);
			#undef CPYFLD
			(p_result->Name = pOuter->Name).Transf(CTRANSF_UTF8_TO_INNER);
			(p_result->Symb = pOuter->Symb).Transf(CTRANSF_UTF8_TO_INNER);
			for(int i = 0; i < pOuter->__sizePlaces; i++) {
				const ns1__prcPlace * p_src_place = pOuter->Places[i];
				if(p_src_place) {
					UhttPrcPlaceDescription * p_new_place = p_result->Places.CreateNewItem();
					THROW(p_new_place);
					p_new_place->GoodsID = p_src_place->GoodsID;
					(p_new_place->Range = p_src_place->Range).Transf(CTRANSF_UTF8_TO_INNER);
					(p_new_place->Descr = p_src_place->Descr).Transf(CTRANSF_UTF8_TO_INNER);
				}
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) UhttProcessorPacket * UhttGetProcessorByID(PPSoapClientSession & rSess, const char * pToken, int id)
{
	UhttProcessorPacket * p_result = 0;
	InParamString arg_token(pToken);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getProcessorByID param;
	ns1__getProcessorByIDResponse resp;
	param.token = arg_token;
	param.id = id;
	if(PreprocessCall(proxi, rSess, proxi.getProcessorByID(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = UhttProcessorOuterToInner(resp.processorItem);
		PPSoapRegisterResultPtr(p_result);
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttProcessorPacket * UhttGetProcessorByCode(PPSoapClientSession & rSess, const char * pToken, const char * pCode)
{
	UhttProcessorPacket * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_code(pCode);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getProcessorByCode param;
	ns1__getProcessorByCodeResponse resp;
	param.token = arg_token;
	param.code = arg_code;
	if(PreprocessCall(proxi, rSess, proxi.getProcessorByCode(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = UhttProcessorOuterToInner(resp.processorItem);
		PPSoapRegisterResultPtr(p_result);
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttStatus * UhttCreateProcessor(PPSoapClientSession & rSess, const char * pToken, const UhttProcessorPacket & rPack)
{
	UhttStatus * p_result = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString arg_name(rPack.Name);
	InParamString arg_symb(rPack.Symb);
	TSCollection <InParamString> arglist;
	ns1__processor instance;
	TSCollection <ns1__prcPlace> place_list;
	{
		#define CPYFLD(f) instance.f = rPack.f
		CPYFLD(ID);
		CPYFLD(ParentID);
		CPYFLD(Kind);
		CPYFLD(Flags);
		CPYFLD(LocID);
		CPYFLD(LinkObjType);
		CPYFLD(LinkObjID);
		CPYFLD(CipPersonKindID);
		CPYFLD(CipMax);
		#undef CPYFLD
		instance.Name = arg_name;
		instance.Symb = arg_symb;
		uint   i;
		for(i = 0; i < rPack.Places.getCount(); i++) {
			const UhttPrcPlaceDescription & r_item = *rPack.Places.at(i);
			ns1__prcPlace * p_place = new ns1__prcPlace;

			InParamString * p_arg_range = new InParamString(r_item.Range);
			InParamString * p_arg_descr = new InParamString(r_item.Descr);
			arglist.insert(p_arg_range);
			arglist.insert(p_arg_descr);

			p_place->GoodsID = r_item.GoodsID;
			p_place->Range = *p_arg_range;
			p_place->Descr = *p_arg_descr;
			place_list.insert(p_place);
		}
		instance.__sizePlaces = 0;
		instance.Places = static_cast<ns1__prcPlace **>(SAlloc::C(place_list.getCount(), sizeof(ns1__prcPlace*)));
		for(i = 0; i < place_list.getCount(); i++) {
			instance.Places[i] = place_list.at(i);
			instance.__sizePlaces++;
		}
	}
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__createProcessor param;
	ns1__createProcessorResponse resp;
	param.token = arg_token;
	param.instance = &instance;
	if(PreprocessCall(proxi, rSess, proxi.createProcessor(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = CreateResultStatus(resp.settled_USCOREcode);
	}
	return p_result;
}
//
//
//
static UhttTSessionPacket * UhttTSessionOuterToInner(const ns1__tSession * pOuter)
{
	UhttTSessionPacket * p_result = 0;
	if(pOuter) {
		p_result = new UhttTSessionPacket;
		if(p_result) {
			SString temp_buf;
			#define CPYFLD(f) p_result->f = pOuter->f
			CPYFLD(ID);
			CPYFLD(Num);
			CPYFLD(PrcID);
			CPYFLD(TechID);
			CPYFLD(ParentID);
			CPYFLD(Status);
			CPYFLD(Flags);
			#undef CPYFLD
			p_result->StTime.T = pOuter->StTime;
			p_result->FinTime.T = pOuter->FinTime;
			(p_result->Memo = pOuter->Memo).Transf(CTRANSF_UTF8_TO_INNER);
			(p_result->Detail = pOuter->Detail).Transf(CTRANSF_UTF8_TO_INNER); // @v8.8.0
			{
				if(pOuter->Tags) {
					for(int j = 0; j < pOuter->__sizeTags; j++) {
						ns1__tagValue * p_tag = pOuter->Tags[j];
						if(p_tag) {
							UhttTagItem * p_new_item = p_result->TagList.CreateNewItem();
							THROW(p_new_item);
							(p_new_item->Symb = p_tag->Symb).Transf(CTRANSF_UTF8_TO_INNER);
							(p_new_item->Value = p_tag->Value).Transf(CTRANSF_UTF8_TO_INNER);
						}
					}
				}
				for(int i = 0; i < pOuter->__sizeLines; i++) {
					const ns1__tSessionLine * p_src_line = pOuter->Lines[i];
					if(p_src_line) {
						UhttTSessLine * p_new_line = p_result->Lines.CreateNewItem();
						THROW(p_new_line);
						#define CPYFLD(f) p_new_line->f = p_src_line->f
						CPYFLD(TSessID);
						CPYFLD(OprNo);
						CPYFLD(GoodsID);
						CPYFLD(LotID);
						CPYFLD(UserID);
						CPYFLD(Sign);
						CPYFLD(Flags);
						CPYFLD(Qtty);
						CPYFLD(WtQtty);
						CPYFLD(Price);
						CPYFLD(Discount);

						p_new_line->Tm.T = p_src_line->Tm;
						p_new_line->Expiry.T = p_src_line->Expiry;
						(p_new_line->Serial = p_src_line->Serial).Transf(CTRANSF_UTF8_TO_INNER);
						#undef CPYFLD
					}
				}
			}
			{
				for(int i = 0; i < pOuter->__sizeCips; i++) {
					const ns1__checkInPerson * p_src_cip = pOuter->Cips[i];
					if(p_src_cip) {
						UhttCipPacket * p_new_cip = p_result->Cips.CreateNewItem();
						THROW(p_new_cip);
						#define CPYFLD(f) p_new_cip->f = p_src_cip->f
						CPYFLD(ID);
						CPYFLD(Kind);
						CPYFLD(PrmrID);
						CPYFLD(PersonID);
						CPYFLD(Num);
						CPYFLD(RegCount);
						CPYFLD(CiCount);
						CPYFLD(Flags);
						CPYFLD(Amount);
						CPYFLD(CCheckID);
						CPYFLD(SCardID);

						p_new_cip->RegTm.T = p_src_cip->RegTm;
						p_new_cip->CiTm.T = p_src_cip->CiTm;
						(p_new_cip->PlaceCode = p_src_cip->PlaceCode).Transf(CTRANSF_UTF8_TO_INNER);
						(p_new_cip->Memo = p_src_cip->Memo).Transf(CTRANSF_UTF8_TO_INNER);
						#undef CPYFLD
					}
				}
			}
			{
#if 0 // { «арезервировано (сесси€ может содержать опциональный список диапазонов мест)
				for(int i = 0; i < pOuter->__sizePlaces; i++) {
					const ns1__prcPlace * p_src_place = pOuter->Places[i];
					if(p_src_place) {
						UhttPrcPlaceDescription * p_new_place = new UhttPrcPlaceDescription;
						THROW(p_new_place);
						p_new_place->GoodsID = p_src_place->GoodsID;
						(p_new_place->Range = p_src_place->Range).Transf(CTRANSF_UTF8_TO_INNER);
						(p_new_place->Descr = p_src_place->Descr).Transf(CTRANSF_UTF8_TO_INNER);
						THROW(p_result->Places.insert(p_new_place));
					}
				}
#endif // } 0
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) UhttTSessionPacket * UhttGetTSessionByID(PPSoapClientSession & rSess, const char * pToken, int id)
{
	UhttTSessionPacket * p_result = 0;
	InParamString arg_token(pToken);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getTSessionByID param;
	ns1__getTSessionByIDResponse resp;
	param.token = arg_token;
	param.id = id;
	if(PreprocessCall(proxi, rSess, proxi.getTSessionByID(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = UhttTSessionOuterToInner(resp.tsessItem);
		PPSoapRegisterResultPtr(p_result);
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttTSessionPacket * UhttGetTSessionByUUID(PPSoapClientSession & rSess, const char * pToken, const S_GUID & rUuid)
{
	UhttTSessionPacket * p_result = 0;
	SString temp_buf;
	rUuid.ToStr(S_GUID::fmtIDL, temp_buf);
	InParamString arg_token(pToken);
	InParamString arg_uuid(temp_buf);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getTSessionByUUID param;
	ns1__getTSessionByUUIDResponse resp;
	param.token = arg_token;
	param.uuid = arg_uuid;
	if(PreprocessCall(proxi, rSess, proxi.getTSessionByUUID(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = UhttTSessionOuterToInner(resp.tsessItem);
		PPSoapRegisterResultPtr(p_result);
	}
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <UhttTSessionPacket> * UhttGetTSessionByPrc(PPSoapClientSession & rSess, const char * pToken, int prcID, const UhttTimestamp * pSince)
{
	TSCollection <UhttTSessionPacket> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_since(pSince ? pSince->T.cptr() : 0);
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getTSessionByPrc param;
	ns1__getTSessionByPrcResponse resp;
	param.token = arg_token;
	param.prcID = prcID;
	param.since = arg_since;
	if(PreprocessCall(proxi, rSess, proxi.getTSessionByPrc(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		THROW(p_result = new TSCollection <UhttTSessionPacket>);
		PPSoapRegisterResultPtr(p_result);
		if(resp.tsessItem) {
			for(int i = 0; i < resp.__sizetsessItem; i++) {
				UhttTSessionPacket * p_pack = 0;
				THROW(p_pack = UhttTSessionOuterToInner(resp.tsessItem[i]));
				p_result->insert(p_pack);
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) UhttStatus * UhttCreateTSession(PPSoapClientSession & rSess, const char * pToken, const UhttTSessionPacket & rPack)
{
	UhttStatus * p_result = 0;
	SString temp_buf;
	uint   i;
	InParamString arg_token(pToken);
	InParamString arg_memo(rPack.Memo);
	InParamString arg_detail(rPack.Detail);
	InParamString arg_sttime(rPack.StTime.T);
	InParamString arg_fntime(rPack.FinTime.T);
	TSCollection <InParamString> arglist;
	ns1__tSession instance;
	TSCollection <ns1__tSessionLine> lines_list;
	TSCollection <ns1__checkInPerson> cip_list;
	TSCollection <ns1__prcPlace> place_list;
	TSCollection <ns1__tagValue> tag_list;
	{
		#define CPYFLD(f) instance.f = rPack.f
		CPYFLD(ID);
		CPYFLD(Num);
		CPYFLD(PrcID);
		CPYFLD(TechID);
		CPYFLD(ParentID);
		CPYFLD(Status);
		CPYFLD(Flags);
		#undef CPYFLD
		instance.StTime = arg_sttime;
		instance.FinTime = arg_fntime;
		instance.Memo = arg_memo;
		instance.Detail = arg_detail;
		{
			for(i = 0; i < rPack.Lines.getCount(); i++) {
				const UhttTSessLine & r_item = *rPack.Lines.at(i);
				ns1__tSessionLine * p_line = new ns1__tSessionLine;
				InParamString * p_arg_serial = new InParamString(r_item.Serial);
				InParamString * p_arg_tm = new InParamString(r_item.Tm.T);
				InParamString * p_arg_expiry = new InParamString(r_item.Expiry.T);
				arglist.insert(p_arg_serial);
				arglist.insert(p_arg_tm);
				arglist.insert(p_arg_expiry);

				#define CPYFLD(f) p_line->f = r_item.f
				CPYFLD(TSessID);
				CPYFLD(OprNo);
				CPYFLD(GoodsID);
				CPYFLD(LotID);
				CPYFLD(UserID);
				CPYFLD(Sign);
				CPYFLD(Flags);
				CPYFLD(Qtty);
				CPYFLD(WtQtty);
				CPYFLD(Price);
				CPYFLD(Discount);
				#undef CPYFLD
				p_line->Tm = *p_arg_tm;
				p_line->Expiry = *p_arg_expiry;
				p_line->Serial = *p_arg_serial;

				lines_list.insert(p_line);
			}
			instance.__sizeLines = 0;
			instance.Lines = static_cast<ns1__tSessionLine **>(SAlloc::C(lines_list.getCount(), sizeof(ns1__tSessionLine*)));
			for(i = 0; i < lines_list.getCount(); i++) {
				instance.Lines[i] = lines_list.at(i);
				instance.__sizeLines++;
			}
		}
		{
			for(i = 0; i < rPack.Cips.getCount(); i++) {
				const UhttCipPacket & r_item = *rPack.Cips.at(i);
				ns1__checkInPerson * p_cip = new ns1__checkInPerson;
				InParamString * p_arg_regtm = new InParamString(r_item.RegTm.T);
				InParamString * p_arg_citm = new InParamString(r_item.CiTm.T);
				InParamString * p_arg_placecode = new InParamString(r_item.PlaceCode);
				InParamString * p_arg_memo = new InParamString(r_item.Memo);
				arglist.insert(p_arg_regtm);
				arglist.insert(p_arg_citm);
				arglist.insert(p_arg_placecode);
				arglist.insert(p_arg_memo);

				#define CPYFLD(f) p_cip->f = r_item.f

				CPYFLD(ID);
				CPYFLD(Kind);
				CPYFLD(PrmrID);
				CPYFLD(PersonID);
				CPYFLD(Num);
				CPYFLD(RegCount);
				CPYFLD(CiCount);
				CPYFLD(Flags);
				CPYFLD(Amount);
				CPYFLD(CCheckID);
				CPYFLD(SCardID);

				#undef CPYFLD
				p_cip->RegTm = *p_arg_regtm;
				p_cip->CiTm = *p_arg_citm;
				p_cip->PlaceCode = *p_arg_placecode;
				p_cip->Memo = *p_arg_memo;
				cip_list.insert(p_cip);
			}
			instance.__sizeCips = 0;
			instance.Cips = static_cast<ns1__checkInPerson **>(SAlloc::C(cip_list.getCount(), sizeof(ns1__checkInPerson*)));
			for(i = 0; i < cip_list.getCount(); i++) {
				instance.Cips[i] = cip_list.at(i);
				instance.__sizeCips++;
			}
		}
		{
			for(i = 0; i < rPack.TagList.getCount(); i++) {
				const UhttTagItem * p_item = rPack.TagList.at(i);
				ns1__tagValue * p_tag = new ns1__tagValue;
				InParamString * p_arg_symb = new InParamString(p_item->Symb);
				InParamString * p_arg_val = new InParamString(p_item->Value);
				arglist.insert(p_arg_symb);
				arglist.insert(p_arg_val);
				p_tag->Symb = *p_arg_symb;
				p_tag->Value = *p_arg_val;
				tag_list.insert(p_tag);
			}
			instance.__sizeTags = 0;
			instance.Tags = static_cast<ns1__tagValue **>(SAlloc::C(tag_list.getCount(), sizeof(ns1__tagValue*)));
			for(i = 0; i < tag_list.getCount(); i++) {
				instance.Tags[i] = tag_list.at(i);
				instance.__sizeTags++;
			}
		}
#if 0 // { «арезервировано (сесси€ может содержать опциональный список диапазонов мест)
		{
			for(i = 0; i < rPack.Places.getCount(); i++) {
				const UhttPrcPlaceDescription & r_item = *rPack.Places.at(i);
				ns1__prcPlace * p_place = new ns1__prcPlace;
				InParamString * p_arg_range = new InParamString(r_item.Range);
				InParamString * p_arg_descr = new InParamString(r_item.Descr);
				arglist.insert(p_arg_range);
				arglist.insert(p_arg_descr);

				p_place->GoodsID = r_item.GoodsID;
				p_place->Range = *p_arg_range;
				p_place->Descr = *p_arg_descr;
				place_list.insert(p_place);
			}
			instance.__sizePlaces = 0;
			instance.Places = static_cast<ns1__prcPlace **>(SAlloc::C(place_list.getCount(), sizeof(ns1__prcPlace*)));
			for(i = 0; i < place_list.getCount(); i++) {
				instance.Places[i] = place_list.at(i);
				instance.__sizePlaces++;
			}
		}
#endif // } 0
	}
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__createTSession param;
	ns1__createTSessionResponse resp;
	param.token = arg_token;
	param.instance = &instance;
	if(PreprocessCall(proxi, rSess, proxi.createTSession(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = CreateResultStatus(resp.settled_USCOREcode);
	}
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <UhttStatus> * UhttSendSms(PPSoapClientSession & rSess, const char * pToken, const TSCollection <UhttSmsPacket> & rPack)
{
	TSCollection <UhttStatus> * p_result = 0;
	TSCollection <InParamString> arg_str_pool;
	SString temp_buf;
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__sendSMS param;
	ns1__sendSMSResponse resp;
	TSCollection <ns1__smsMessage> msg_list;
	{
		{
			for(uint i = 0; i < rPack.getCount(); i++) {
				const UhttSmsPacket * p_src_item = rPack.at(i);
				if(p_src_item) {
					ns1__smsMessage * p_new_item = msg_list.CreateNewItem(0);
					THROW(p_new_item);
					p_new_item->From = GetDynamicParamString((temp_buf = p_src_item->From).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					p_new_item->To = GetDynamicParamString((temp_buf = p_src_item->To).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
					p_new_item->Text = GetDynamicParamString((temp_buf = p_src_item->Text).Transf(CTRANSF_INNER_TO_UTF8), arg_str_pool);
				}
			}
		}
		{
			param.message = reinterpret_cast<ns1__smsMessage **>(PPSoapCreateArray(msg_list.getCount(), param.__sizemessage));
			THROW(param.message);
			for(uint i = 0; i < msg_list.getCount(); i++) {
				param.message[i] = msg_list.at(i);
			}
		}
		param.token = GetDynamicParamString(pToken, arg_str_pool);
		if(PreprocessCall(proxi, rSess, proxi.sendSMS(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
			if(resp.__sizestatus > 0) {
				THROW(p_result = new TSCollection <UhttStatus>);
				PPSoapRegisterResultPtr(p_result);
				for(uint i = 0; i < (uint)resp.__sizestatus; i++) {
					const ns1__statusExt * p_status_item = resp.status[i];
					if(p_status_item) {
						UhttStatus * p_new_item = CreateResultStatus(p_status_item);
						THROW(p_new_item);
						p_result->insert(p_new_item);
					}
				}
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	SAlloc::F(param.message);
	return p_result;
}

extern "C" __declspec(dllexport) SString * UhttGetCommonMqsConfig(PPSoapClientSession & rSess)
{
	SString * p_result = 0;
	WSInterfaceImplServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxi, 0, 0);
	ns1__getCommonMqsConfig param;
	ns1__getCommonMqsConfigResponse resp;
	if(PreprocessCall(proxi, rSess, proxi.getCommonMqsConfig(rSess.GetUrl(), 0 /* soap_action */, &param, &resp))) {
		p_result = new SString;
		if(p_result) {
			PPSoapRegisterResultPtr(p_result);
			*p_result = resp.result;
		}
	}
	return p_result;
}
