// PPSOAPCALLER-UHTT.CPP
// Copyright (c) A.Sobolev 2012
//
#include <ppsoapclient.h>
#include "uhtt\uhttSoapPlugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy.h"

// Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy

class PPSoapResultPtrBase {
public:
	virtual void Destroy() = 0;
	virtual const void * GetPtr() const = 0;
	int operator == (const void * ptr) const
	{
		return (ptr == GetPtr());
	}
};

template <class T> class PPSoapResultPtr : public PPSoapResultPtrBase {
public:
	PPSoapResultPtr(T * p)
	{
		Ptr = p;
	}
	virtual void Destroy()
	{
		if(Ptr) {
			delete Ptr;
			Ptr = 0;
		}
	}
	virtual const void * GetPtr() const
	{
		return Ptr;
	}
	T * Ptr;
};

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
//
//
//
extern "C" __declspec(dllexport) int UhttDestroyResult(void * pResult)
{
	return pResult ? DestroyResultPtr(pResult) : -1;
}

extern "C" __declspec(dllexport) SString * UhttAuth(PPSoapClientSession & rSess, const char * pName, const char * pPassword)
{
	char * p_token = 0;
	SString * p_result = 0;
	SString temp_buf;
	InParamString arg_name(pName);
	InParamString arg_pw(pPassword);
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	soap_set_omode(&proxi, SOAP_C_UTFSTRING);
	int    r = proxi.auth(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_name, arg_pw, p_token);
	if(r == SOAP_OK) {
		p_result = new SString;
		if(p_result) {
			RegisterResultPtr(p_result);
			*p_result = p_token;
		}
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
	}
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
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	soap_set_omode(&proxi, SOAP_C_UTFSTRING);
	int    r = proxi.getGoodsArCode(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_token, arg_goods_code, arg_person_inn, p_code);
	if(r == SOAP_OK) {
		p_result = new SString;
		if(p_result) {
			RegisterResultPtr(p_result);
			*p_result = p_code;
		}
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
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

	ns9__Document image;
	image.contentType = content_type;
	image.data = content;
	image.encoding = encoding;
	image.name = name;
	image.size = rPack.Size;
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	soap_set_omode(&proxi, SOAP_C_UTFSTRING);
	struct ns1__setImageByIDResponse resp;
	int    r = proxi.setImageByID(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_token, arg_objtype, rPack.UhttObjID, &image, resp);
	if(r == SOAP_OK) {
		p_result = new UhttStatus;
		if(p_result) {
			RegisterResultPtr(p_result);
			p_result->Code = resp._setImageByIDReturn->code;
			p_result->Msg = resp._setImageByIDReturn->message;
		}
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
	}
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <UhttGoodsPacket> * UhttGetGoodsByCode(PPSoapClientSession & rSess, const char * pToken, const char * pCode)
{
	TSCollection <UhttGoodsPacket> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_code(pCode);
	SString temp_buf;
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	struct ns1__getGoodsByCodeResponse resp;
	int    r = proxi.getGoodsByCode(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_token, arg_code, resp);
	if(r == SOAP_OK) {
		p_result = new TSCollection <UhttGoodsPacket>;
		if(p_result) {
			RegisterResultPtr(p_result);
			if(resp._getGoodsByCodeReturn) {
				for(int i = 0; i < resp._getGoodsByCodeReturn->__size; i++) {
					ns3__Goods * p_goods_item = resp._getGoodsByCodeReturn->__ptr[i];
					if(p_goods_item) {
						UhttGoodsPacket * p_pack = new UhttGoodsPacket;
						THROW(p_pack);
						p_pack->ID = p_goods_item->ID;
						(temp_buf = p_goods_item->Name).UTF8ToOem().CopyTo(p_pack->Name, sizeof(p_pack->Name));
						(temp_buf = p_goods_item->SingleBarCode).UTF8ToOem().CopyTo(p_pack->SingleBarcode, sizeof(p_pack->SingleBarcode));
						p_pack->Package = p_goods_item->Package;
						p_result->insert(p_pack);
					}
				}
			}
		}
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <UhttGoodsPacket> * UhttGetGoodsByName(PPSoapClientSession & rSess, const char * pToken, const char * pName)
{
	SString temp_buf;
	TSCollection <UhttGoodsPacket> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_name(pName);
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	soap_set_omode(&proxi, SOAP_C_UTFSTRING);
	struct ns1__getGoodsByNameResponse resp;
	int    r = proxi.getGoodsByName(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_token, arg_name, resp);
	if(r == SOAP_OK) {
		p_result = new TSCollection <UhttGoodsPacket>;
		if(p_result) {
			RegisterResultPtr(p_result);
			if(resp._getGoodsByNameReturn) {
				for(int i = 0; i < resp._getGoodsByNameReturn->__size; i++) {
					ns3__Goods * p_goods_item = resp._getGoodsByNameReturn->__ptr[i];
					if(p_goods_item) {
						UhttGoodsPacket * p_pack = new UhttGoodsPacket;
						THROW(p_pack);
						p_pack->ID = p_goods_item->ID;
						(temp_buf = p_goods_item->Name).UTF8ToOem().CopyTo(p_pack->Name, sizeof(p_pack->Name));
						(temp_buf = p_goods_item->SingleBarCode).UTF8ToOem().CopyTo(p_pack->SingleBarcode, sizeof(p_pack->SingleBarcode));
						p_pack->Package = p_goods_item->Package;
						p_result->insert(p_pack);
					}
				}
			}
		}
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

extern "C" __declspec(dllexport) UhttStatus * UhttCreateGoods(PPSoapClientSession & rSess, const char * pToken, const UhttGoodsPacket & rPack)
{
	UhttStatus * p_result = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString name(rPack.Name);
	InParamString single_barcode(rPack.SingleBarcode);
	ns3__Goods goods_param;
	goods_param.ID = 0;
	goods_param.OKOF = 0;
	goods_param.Barcodes = 0;
	goods_param.BrandID = 0;
	goods_param.Brutto = 0.0;
	goods_param.ClsID = 0;
	goods_param.Energy = 0;
	goods_param.ExpiryPeriod = 0;
	goods_param.GroupID = 0;
	goods_param.Height = 0;
	goods_param.Length = 0;
	goods_param.Width = 0;
	goods_param.HowToUse = 0;
	goods_param.Ingredients = 0;
	goods_param.ManufactorID = 0;
	goods_param.Name = name;
	goods_param.PhPerUnit = 0.0;
	goods_param.PhUnitID = 0;
	goods_param.Standard = 0;
	goods_param.Storage = 0;
	goods_param.TaxGrpID = 0;
	goods_param.TypeID = 0;
	goods_param.UnitID = 0;
	goods_param.SingleBarCode = single_barcode;
	goods_param.Package = rPack.Package;
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	soap_set_omode(&proxi, SOAP_C_UTFSTRING);
	struct ns1__createGoodsResponse resp;
	int    r = proxi.createGoods(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_token, &goods_param, resp);
	if(r == SOAP_OK) {
		p_result = new UhttStatus;
		if(p_result) {
			RegisterResultPtr(p_result);
			p_result->Code = resp._createGoodsReturn->code;
			p_result->Msg = resp._createGoodsReturn->message;
		}
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
	}
	return p_result;
}

extern "C" __declspec(dllexport) TSCollection <UhttSpecSeriesPacket> * UhttGetSpecSeriesByPeriod(PPSoapClientSession & rSess, const char * pToken, const char * pPeriod)
{
	SString temp_buf;
	TSCollection <UhttSpecSeriesPacket> * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_period(pPeriod);
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	soap_set_omode(&proxi, SOAP_C_UTFSTRING);
	struct ns1__getSpecSeriesByPeriodResponse resp;
	int    r = proxi.getSpecSeriesByPeriod(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_token, arg_period, resp);
	if(r == SOAP_OK) {
		p_result = new TSCollection <UhttSpecSeriesPacket>;
		if(p_result) {
			RegisterResultPtr(p_result);
			if(resp._getSpecSeriesByPeriodReturn) {
				for(int i = 0; i < resp._getSpecSeriesByPeriodReturn->__size; i++) {
					ns7__SpecSeries * p_specseries_item = resp._getSpecSeriesByPeriodReturn->__ptr[i];
					if(p_specseries_item) {
						UhttSpecSeriesPacket * p_pack = new UhttSpecSeriesPacket;
						THROW(p_pack);
						p_pack->ID = p_specseries_item->ID;
						p_pack->GoodsID = p_specseries_item->goodsID;
						p_pack->ManufID = p_specseries_item->manufID;
						p_pack->ManufCountryID = p_specseries_item->manufCountryID;
						p_pack->LabID = p_specseries_item->labID;
						(p_pack->Serial = p_specseries_item->serial).UTF8ToOem();
						(p_pack->Barcode = p_specseries_item->barcode).UTF8ToOem();
						(p_pack->GoodsName = p_specseries_item->goodsName).UTF8ToOem();
						(p_pack->ManufName = p_specseries_item->manufName).UTF8ToOem();
						p_pack->InfoDate = p_specseries_item->infoDate;
						p_pack->InfoKind = p_specseries_item->infoKind;
						(p_pack->InfoIdent = p_specseries_item->infoIdent).UTF8ToOem();
						p_pack->AllowDate = p_specseries_item->allowDate;
						(p_pack->AllowNumber = p_specseries_item->allowNumber).UTF8ToOem();
						(p_pack->LetterType = p_specseries_item->letterType).UTF8ToOem();
						p_pack->Flags = p_specseries_item->flags;
						p_result->insert(p_pack);
					}
				}
			}
		}
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
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
	ns7__SpecSeries specSeries_param;
	specSeries_param.ID = rPack.ID;
	specSeries_param.goodsID = rPack.GoodsID;
	specSeries_param.manufID = rPack.ManufID;
	specSeries_param.manufCountryID = 0;
	specSeries_param.labID = rPack.LabID;
	specSeries_param.serial = serial;
	specSeries_param.barcode = barcode;
	specSeries_param.goodsName = goods_name;
	specSeries_param.manufName = manuf_name;
	specSeries_param.infoDate = info_date;
	specSeries_param.infoKind = rPack.InfoKind;
	specSeries_param.infoIdent = info_ident;
	specSeries_param.allowDate = allow_date;
	specSeries_param.allowNumber = allow_number;
	specSeries_param.letterType = letter_type;
	specSeries_param.flags = rPack.Flags;
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	soap_set_omode(&proxi, SOAP_C_UTFSTRING);
	struct ns1__createSpecSeriesResponse resp;
	int    r = proxi.createSpecSeries(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_token, &specSeries_param, resp);
	if(r == SOAP_OK) {
		p_result = new UhttStatus;
		if(p_result) {
			RegisterResultPtr(p_result);
			p_result->Code = resp._createSpecSeriesReturn->code;
			p_result->Msg = resp._createSpecSeriesReturn->message;
		}
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
	}
	return p_result;
}

extern "C" __declspec(dllexport) UhttSCardPacket * UhttGetSCardByNumber(PPSoapClientSession & rSess, const char * pToken, const char * pNumber)
{
	SString temp_buf;
	UhttSCardPacket * p_result = 0;
	InParamString arg_token(pToken);
	InParamString arg_number(pNumber);
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	soap_set_omode(&proxi, SOAP_C_UTFSTRING);
	struct ns1__getSCardByNumberResponse resp;
	int    r = proxi.getSCardByNumber(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_token, arg_number, resp);
	if(r == SOAP_OK) {
		p_result = new UhttSCardPacket;
		if(p_result) {
			RegisterResultPtr(p_result);
			THROW(resp._getSCardByNumberReturn);
			p_result->ID = resp._getSCardByNumberReturn->ID;
			p_result->SeriesID = resp._getSCardByNumberReturn->SeriesID;
			p_result->OwnerID = resp._getSCardByNumberReturn->OwnerID;
			(p_result->Code = resp._getSCardByNumberReturn->Code).UTF8ToOem();
			(p_result->Hash = resp._getSCardByNumberReturn->Hash).UTF8ToOem();
			if(resp._getSCardByNumberReturn->IssueDate) {
				p_result->IssueDate.Date = resp._getSCardByNumberReturn->IssueDate->date;
				p_result->IssueDate.Time = resp._getSCardByNumberReturn->IssueDate->time;
			}
			if(resp._getSCardByNumberReturn->Expiry) {
				p_result->IssueDate.Date = resp._getSCardByNumberReturn->Expiry->date;
				p_result->IssueDate.Time = resp._getSCardByNumberReturn->Expiry->time;
			}
			p_result->PDis = resp._getSCardByNumberReturn->PDis;
			p_result->Overdraft = resp._getSCardByNumberReturn->Overdraft;
			p_result->Debit = resp._getSCardByNumberReturn->Debit;
			p_result->Credit = resp._getSCardByNumberReturn->Credit;
			p_result->Rest = resp._getSCardByNumberReturn->Rest;
			p_result->Flags = resp._getSCardByNumberReturn->Flags;
		}
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
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
	ns6__Check check_param;
	check_param.posNodeID = rPack.PosNodeID;
	check_param.locID = rPack.LocID;
	check_param.dtm = new ns2__DateTime;
	check_param.dtm->date = 0;
	check_param.dtm->time = 0;
	{
		const size_t tm_buf_len = 128;
		if(!rPack.Dtm.Date.Empty()) {
			check_param.dtm->date = new char[tm_buf_len];
			rPack.Dtm.Date.CopyTo(check_param.dtm->date, tm_buf_len);
		}
		if(!rPack.Dtm.Time.Empty()) {
			check_param.dtm->time = new char[tm_buf_len];
			rPack.Dtm.Time.CopyTo(check_param.dtm->time, tm_buf_len);
		}
	}
	check_param.SCardID = rPack.SCardID;
	check_param.amount = rPack.Amount;
	check_param.discount = rPack.Discount;
	check_param.flags = rPack.Flags;
	// TODO: Process items
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	soap_set_omode(&proxi, SOAP_C_UTFSTRING);
	struct ns1__createSCardCheckResponse resp;
	int    r = proxi.createSCardCheck(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_token, arg_loc_symb, arg_scard_number, &check_param, resp);
	if(r == SOAP_OK) {
		p_result = new UhttStatus;
		if(p_result) {
			RegisterResultPtr(p_result);
			p_result->Code = resp._createSCardCheckReturn->code;
			p_result->Msg = resp._createSCardCheckReturn->message;
		}
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
	}
	{
		delete [] check_param.dtm->date;
		delete [] check_param.dtm->time;
		delete check_param.dtm;
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
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	soap_set_omode(&proxi, SOAP_C_UTFSTRING);
	struct ns1__depositSCardAmountResponse resp;
	int    r = proxi.depositSCardAmount(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_token, arg_number, amount, resp);
	if(r == SOAP_OK) {
		p_result = new UhttStatus;
		if(p_result) {
			RegisterResultPtr(p_result);
			p_result->Code = resp._depositSCardAmountReturn->code;
			p_result->Msg = resp._depositSCardAmountReturn->message;
		}
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
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
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	soap_set_omode(&proxi, SOAP_C_UTFSTRING);
	struct ns1__withdrawSCardAmountResponse resp;
	int    r = proxi.withdrawSCardAmount(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_token, arg_number, amount, resp);
	if(r == SOAP_OK) {
		p_result = new UhttStatus;
		if(p_result) {
			RegisterResultPtr(p_result);
			p_result->Code = resp._withdrawSCardAmountReturn->code;
			p_result->Msg = resp._withdrawSCardAmountReturn->message;
		}
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
	}
	return p_result;
}

extern "C" __declspec(dllexport) int UhttGetSCardRest
	(PPSoapClientSession & rSess, const char * pToken, const char * pNumber, const char * pDate, double & rRest)
{
	int     ok = 0;
	double  rest = 0;
	SString temp_buf;
	InParamString arg_token(pToken);
	InParamString arg_number(pNumber);
	InParamString arg_date(pDate);
	Plugin_USCOREUHTT_USCORESOAPServiceSoapBindingProxy proxi(SOAP_XML_INDENT|SOAP_XML_IGNORENS); 
	soap_set_imode(&proxi, SOAP_C_UTFSTRING);
	soap_set_omode(&proxi, SOAP_C_UTFSTRING);
	int    r = proxi.getSCardRest(isempty(rSess.Url) ? 0 : rSess.Url, 0 /* soap_action */, arg_token, arg_number, arg_date, rest);
	if(r == SOAP_OK) {
		rRest = rest;
		ok = 1;
	}
	else {
		char   temp_err_buf[1024];
		proxi.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
		(temp_buf = temp_err_buf).UTF8ToChar().CopyTo(rSess.ErrMsg, sizeof(rSess.ErrMsg));
		rRest = 0.0;
	}
	return ok;
}