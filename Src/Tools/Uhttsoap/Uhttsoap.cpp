//
// Uhttsoap.cpp
//
#include <slib.h>
#include "soapH.h"
#include "soapdefs.h"
#include "soapUhttMainBindingProxy.h"
#include "stdsoap2.h"

int main(int argc, char * argv[])
{
	SString name;
	UhttMainBinding prcssr;
	ns1__Get_USCOREGoodsResponse goods_rec;
	ns1__LoginResponse info;
	info.Status = 0;
	goods_rec.Goods = 0;
	prcssr.endpoint = "http://192.168.0.21/Universe-HTT/soap/universe_htt.php";
	prcssr.soap->imode |= SOAP_IO_KEEPALIVE;
	prcssr.soap->omode |= SOAP_IO_KEEPALIVE;
	if(prcssr.ns1__Login("master", "123", info) == SOAP_OK) {
		soap_set_cookie(prcssr.soap, "SOAPClient", info.Status->Memo, "http://192.168.0.21", "/");
		soap_set_imode(prcssr.soap, SOAP_C_UTFSTRING);
		soap_set_omode(prcssr.soap, SOAP_C_UTFSTRING);

		long goods_id = 0;
		SString buf;
		ns1__UhttObjSelectCriteria query;
		ns1__SelectObjectResponse  sel_rec;
		//
		// Выборка товаров
		//
		query.soap = prcssr.soap;
		query.ObjName = newStr("goods");
		(buf = "parent.id").CatChar('(').Cat(128862).CatChar(')');
		query.ByCriteria = newStr(buf);
		if(prcssr.ns1__SelectObject(&query, sel_rec) == SOAP_OK) {
			for(int i = 0; i < sel_rec.CommObjItem->__sizeCommObjItem; i++) {
				goods_id = sel_rec.CommObjItem->CommObjItem[i]->ID;
				if(prcssr.ns1__Get_USCOREGoods(goods_id, goods_rec) == SOAP_OK) {
					int r = 0;
					SString buf;
					ns1__UhttArCode goods_code_rec;
					ns1__SetGoodsCodeResponse gc_ret;
					(name = goods_rec.Goods->Name).UTF8ToChar();
					goods_code_rec.Code = newStr(buf.Cat(goods_id));
					goods_code_rec.OwnerID = 41171;
					if(prcssr.ns1__SetGoodsCode(&goods_code_rec, gc_ret) == SOAP_OK)
						r = 1;
					ZDELETE(goods_code_rec.Code);
				}
			}
		}
		ZDELETE(query.ByCriteria);
		ZDELETE(query.ObjName);
		//
		// Выборка персоналий
		//
		query.ObjName = newStr("person");
		(buf = "name").CatParStr("Поставщик").ToUtf8();
		query.ByCriteria = newStr(buf);
		if(prcssr.ns1__SelectObject(&query, sel_rec) == SOAP_OK) {
			for(int i = 0; i < sel_rec.CommObjItem->__sizeCommObjItem; i++) {
				long psn_id = 0;
				ns1__Get_USCOREPersonResponse psn_rec;
				psn_id = sel_rec.CommObjItem->CommObjItem[i]->ID;
				if(prcssr.ns1__Get_USCOREPerson(psn_id, psn_rec) == SOAP_OK)
					(name = psn_rec.Person->Name).UTF8ToChar();
			}
		}
		ZDELETE(query.ByCriteria);
		ZDELETE(query.ObjName);

	}
	return 0;
}

