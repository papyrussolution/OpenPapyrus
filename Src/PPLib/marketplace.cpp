// MARKETPLACE.CPP
// Copyright (c) A.Sobolev 2024
// @codepage UTF-8
// @construction 
//
#include <pp.h>
#pragma hdrstop

class PPMarketplaceInterface {
public:
	virtual ~PPMarketplaceInterface()
	{
	}
	virtual int Init(PPID guaID);
	//
	int    GetMarketplacePerson(PPID * pID, int use_ta);
	//
	// Descr: Возвращает идентификатор таблицы аналитического учета, соответствующей маркетплейсам (вид персоналий PPPRK_MARKETPLACE).
	// Returns:
	//   0 - искомая таблица не найдена
	//  >0 - идентификатор искомой таблицы аналитических статей
	//
	PPID   GetMarketplaceAccSheetID();
	const  char * GetSymbol() const { return P_Symbol; }
protected:
	PPMarketplaceInterface(const char * pSymbol) : P_Symbol(pSymbol), State(0)
	{
	}
	uint    State;
	PPGlobalUserAccPacket GuaPack;
	//
	PPObjPerson PsnObj;
	PPObjGoods GObj;
	PPObjArticle ArObj;
private:
	const   char * P_Symbol;
};

PPID PPMarketplaceInterface::GetMarketplaceAccSheetID()
{
	PPID   acs_id = 0;
	Reference * p_ref = PPRef;
	PPObjAccSheet acs_obj;
	PPAccSheet2 acs_rec;
	for(SEnum en = p_ref->EnumByIdxVal(PPOBJ_ACCSHEET, 1, PPOBJ_PERSON); !acs_id && en.Next(&acs_rec) > 0;) {
		if(acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup == PPPRK_MARKETPLACE) {
			acs_id = acs_rec.ID;
		}
	}
	return acs_id;
}

int PPMarketplaceInterface::GetMarketplacePerson(PPID * pID, int use_ta)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	const  PPID person_kind = PPPRK_MARKETPLACE;
	SString temp_buf;
	PPObjPersonKind pk_obj;
	PPPersonKind pk_rec;
	THROW(pk_obj.Fetch(person_kind, &pk_rec) > 0); // @todo @err (этот вид персоналий должен быть создан вызовом функции созднания зарезервированных объектов)
	{
		PPID   acs_id = GetMarketplaceAccSheetID();
		PPTransaction tra(use_ta);
		THROW(tra);
		{
			// Кроме прочего, нам необходимо убедиться, что существует таблица аналитических статей, ассоциированная с видом персоналий PPPRK_MARKETPLACE
			PPObjAccSheet acs_obj;
			PPAccSheet2 acs_rec;
			
			if(!acs_id) {
				MEMSZERO(acs_rec);
				STRNSCPY(acs_rec.Name, pk_rec.Name);
				acs_rec.Assoc = PPOBJ_PERSON;
				acs_rec.ObjGroup = person_kind;
				acs_rec.Flags = ACSHF_AUTOCREATART;
				THROW(p_ref->AddItem(PPOBJ_ACCSHEET, &acs_id, &acs_rec, 0));
			}
		}
		if(!isempty(P_Symbol)) {
			PPIDArray found_list;
			PPObjTag tag_obj;
			PPObjectTag tag_rec;
			THROW(tag_obj.Fetch(PPTAG_PERSON_MARKETPLACESYMB, &tag_rec) > 0); // @todo @err
			if(p_ref->Ot.SearchObjectsByStr(PPOBJ_PERSON, PPTAG_PERSON_MARKETPLACESYMB, P_Symbol, &found_list) > 0) {
				for(uint i = 0; ok < 0 && i < found_list.getCount(); i++) {
					const PPID psn_id = found_list.get(i);
					PersonTbl::Rec psn_rec;
					if(PsnObj.Search(psn_id, &psn_rec) > 0) {
						if(PsnObj.P_Tbl->IsBelongsToKind(psn_id, person_kind)) {
							ASSIGN_PTR(pID, psn_id);
							ok = 1;
						}
					}
				}
			}
			if(ok < 0) {
				PPPersonPacket psn_pack;
				(temp_buf = "Marketplace").CatDiv('-', 1).Cat(P_Symbol);
				psn_pack.Rec.Status = PPPRS_LEGAL;
				STRNSCPY(psn_pack.Rec.Name, temp_buf);
				psn_pack.Kinds.add(person_kind);
				THROW(psn_pack.TagL.PutItemStr(PPTAG_PERSON_MARKETPLACESYMB, P_Symbol));
				THROW(PsnObj.PutPacket(pID, &psn_pack, 0));
				ok = 2;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

class PPMarketplaceInterface_Wildberries : public PPMarketplaceInterface {
public:
	struct Warehouse {
		Warehouse();
		Warehouse & Z();
		bool FromJsonObj(const SJson * pJs);

		enum {
			fAcceptsQR = 0x0001
		};
		long   ID; // Идентификатор склада на маркетплейсе (не в нашей базе данных!)
		uint   Flags;
		SString Name;
		SString Address;
	};
	struct WareBase {
		WareBase();
		WareBase & Z();
		bool FromJsonObj(const SJson * pJs);

		int64 ID;
		SString Name;
		SString TechSize;
		SString SupplArticle;
		SString Barcode;
		SString Category;
		SString Brand;
	};
	struct Stock {
		Stock();
		Stock & Z();
		bool FromJsonObj(const SJson * pJs);

		enum {
			fIsSupply      = 0x0001,
			fIsRealization = 0x0002,
		};
		LDATETIME DtmLastChange;
		WareBase Ware;
		SString WarehouseName;
		double Qtty;
		double QttyFull;
		double InWayToClient;
		double InWayFromClient;
		double Price;
		double Discount;
		uint   Flags;
	};
	struct Income  {
		Income();
		Income & Z();
		bool FromJsonObj(const SJson * pJs);

		int64   IncomeID;
		LDATETIME Dtm;
		LDATETIME DtmLastChange;
		SString Number; // Номер УПД
		WareBase Ware;
		double Qtty;
		double TotalPrice;
		LDATETIME DtmClose;
		SString WarehouseName;
		SString Status;
		/*
		"incomeId": 22650325,
		"number": "",
		"date": "2024-08-31T00:00:00",
		"lastChangeDate": "2024-09-03T16:12:09",
		"supplierArticle": "Lampasunone",
		"techSize": "0",
		"barcode": "2040705818355",
		"quantity": 30,
		"totalPrice": 0,
		"dateClose": "2024-09-03T00:00:00",
		"warehouseName": "Электросталь",
		"nmId": 245313051,
		"status": "Принято"
		*/
	};
	//
	// Descr: Структура описания элемента продажи или заказа
	//
	struct Sale {
		Sale();
		Sale & Z();
		bool FromJsonObj(const SJson * pJs);

		enum {
			fIsSupply      = 0x0001,
			fIsRealization = 0x0002,
			fIsCancel      = 0x0004  // Только для заказа 
		};
		LDATETIME Dtm;
		LDATETIME DtmLastChange;
		LDATETIME DtmCancel;      // order
		WareBase Ware;
		SString WarehouseName;
		SString CountryName;
		SString DistrictName;
		SString RegionName;
		int64  IncomeID;
		uint   Flags;
		double TotalPrice;
		double DiscountPct;
		double Spp;               // Скидка WB
		double PaymentSaleAmount; // sale Оплачено с WB Кошелька
		double ForPay;            // sale К перечислению продавцу
		double FinishedPrice;     // Фактическая цена с учетом всех скидок (к взиманию с покупателя)
		double PriceWithDiscount; // Цена со скидкой продавца, от которой считается сумма к перечислению продавцу forPay (= totalPrice * (1 - discountPercent/100))
		SString SaleId;           // Уникальный идентификатор продажи/возврата // S********** — продажа, R********** — возврат (на склад WB)
		SString OrderType;        // Тип заказа:
			// Клиентский — заказ, поступивший от покупателя
			// Возврат Брака — возврат товара продавцу
			// Принудительный возврат — возврат товара продавцу
			// Возврат обезлички — возврат товара продавцу
			// Возврат Неверного Вложения — возврат товара продавцу
			// Возврат Продавца — возврат товара продавцу
			// Возврат из Отзыва — возврат товара продавцу
			// АвтоВозврат МП — возврат товара продавцу
			// Недокомплект (Вина продавца) — возврат товара продавцу
			// Возврат КГТ — возврат товара продавцу
		SString Sticker; // Идентификатор стикера
		SString GNumber; // Номер заказа
		SString SrID;    // Уникальный идентификатор заказа. Примечание для использующих API Маркетплейс: srid равен rid в ответах методов сборочных заданий.
	};

	PPMarketplaceInterface_Wildberries() : PPMarketplaceInterface("WILDBERRIES"), Lth(PPFILNAM_MRKTPLCWBTALK_LOG)
	{
	}
	virtual ~PPMarketplaceInterface_Wildberries()
	{
	}
	virtual int Init(PPID guaID)
	{
		int    ok = PPMarketplaceInterface::Init(guaID);
		if(ok > 0) {
			RequestWarehouseList(WhList);
		}
		return ok;
	}
	//
	// Methods
	//
	int   RequestCommission();
	int   RequestWarehouseList(TSCollection <Warehouse> & rList);
	int   RequestGoodsPrices();
	int   RequestIncomes(TSCollection <Income> & rList);
	int   RequestStocks(TSCollection <Stock> & rList);
	int   RequestOrders(TSCollection <Sale> & rList);
	int   RequestSales(TSCollection <Sale> & rList);
	int   RequestSupplies();
	int   RequestAcceptanceReport(const DateRange & rPeriod);
	int   UploadWare();
	int   RequestWareList();

	int   CreateWarehouse(PPID * pID, int64 outerId, const char * pOuterName, const char * pAddress, int use_ta);
	const Warehouse * SearchWarehouseByName(const TSCollection <Warehouse> & rWhList, const char * pWhName) const;
	int   CreateWare(PPID * pID, const WareBase & rWare, int use_ta);
	int   ImportOrders();
	int   ImportReceipts();
	int   ImportSales();
private:
	//
	// Descr: 
	// Returns:
	//   0 - error
	//   >0 - ид лота
	//
	PPID  CreateReceipt(int64 incomeId, LDATE dt, PPID locID, PPID goodsID, double qtty, int use_ta);
	//
	// Descr: 
	// Returns:
	//   0 - error
	//   >0 - ид лота, к которому должна быть привязана продажа
	//
	PPID  AdjustReceiptOnExpend(int64 incomeId, LDATE dt, PPID locID, PPID goodsID, double neededQtty, int use_ta);
	SString & MakeHeaderFields(const char * pToken, StrStrAssocArray * pHdrFlds, SString & rBuf)
	{
		StrStrAssocArray hdr_flds;
		SETIFZ(pHdrFlds, &hdr_flds);
		{
			SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
			SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrCacheControl, "no-cache");
			//SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAcceptLang, "ru");
			SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAccept, "application/json");
		}
		if(!isempty(pToken)) {
			SString temp_buf;
			//(temp_buf = "Bearer").Space().Cat(pToken);
			temp_buf = pToken;
			SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAuthorization, temp_buf);
		}
		SHttpProtocol::PutHeaderFieldsIntoString(*pHdrFlds, rBuf);
		return rBuf;
	}
	//
	//
	//
	enum {
		apiUndef = 0,
		apiCommon = 1,
		apiStatistics,
		apiSellerAnalytics,
		apiAdvert,
		apiRecommend,
		apiSupplies,
		apiDiscountsPrices,
		apiContent,
		apiMarketplace,
		apiAnalytics,
	};
	enum {
		methCommission = 1,   // apiCommon
		methTariffBox,        // apiCommon 
		methTariffPallet,     // apiCommon
		methTariffReturn,     // apiCommon
		methWarehouses,       // apiSupplies
		methIncomes,          // apiStatistics
		methStocks,           // apiStatistics
		methOrders,           // apiStatistics
		methSales,            // apiStatistics
		methSupples,          // apiMarketplace https://marketplace-api.wildberries.ru/api/v3/supplies Получить список поставок
		methSupply,           // apiMarketplace https://marketplace-api.wildberries.ru/api/v3/supplies/{supplyId} Получить поставку
		methSupplyOrders,     // apiMarketplace https://marketplace-api.wildberries.ru/api/v3/supplies/{supplyId}/orders Получить сборочные задания в поставке
		methAcceptanceReport, // apiAnalytics   https://seller-analytics-api.wildberries.ru/api/v1/analytics/acceptance-report
		methGoodsPrices,      // apiDiscountsPrices https://discounts-prices-api.wildberries.ru/api/v2/list/goods/filter
		methContentCardsList, // apiContent https://content-api.wildberries.ru/content/v2/get/cards/list
	};
	bool MakeTargetUrl_(int meth, int * pReq/*SHttpProtocol::reqXXX*/, SString & rResult) const
	{
		static const SIntToSymbTabEntry api_list[] = {
			{ apiCommon, "common-api" },
			{ apiStatistics, "statistics-api" },
			{ apiSellerAnalytics, "seller-analytics-api" },
			{ apiAdvert, "advert-api" },
			{ apiRecommend, "recommend-api" },
			{ apiSupplies, "supplies-api" },
			{ apiDiscountsPrices, "discounts-prices-api" },
			{ apiContent, "content-api" },
			{ apiMarketplace, "marketplace-api" },
			{ apiAnalytics, "seller-analytics-api" }
		};
		struct MethEntry {
			int    Meth;
			int    Api;
			int    Req;
			const char * P_UrlSuffix;
		};
		static const MethEntry meth_list[] = {
			{ methCommission, apiCommon, SHttpProtocol::reqGet, "api/v1/tariffs/commission" },
			{ methTariffBox, apiCommon, 0, "" },
			{ methTariffPallet, apiCommon, 0, "" },
			{ methTariffReturn, apiCommon, 0, "" },
			{ methWarehouses, apiSupplies, SHttpProtocol::reqGet, "api/v1/warehouses" },
			{ methIncomes, apiStatistics, SHttpProtocol::reqGet, "api/v1/supplier/incomes" },
			{ methStocks, apiStatistics, SHttpProtocol::reqGet, "api/v1/supplier/stocks" },
			{ methOrders, apiStatistics, SHttpProtocol::reqGet, "api/v1/supplier/orders" },
			{ methSales, apiStatistics, SHttpProtocol::reqGet, "api/v1/supplier/sales" },
			{ methSupples, apiMarketplace, SHttpProtocol::reqGet, "api/v3/supplies" },
			{ methSupply, apiMarketplace, SHttpProtocol::reqGet, "api/v3/supplies" }, // /supplies/{supplyId}
			{ methSupplyOrders, apiMarketplace, SHttpProtocol::reqGet, "api/v3/supplies" }, // /{supplyId}/orders
			{ methAcceptanceReport, apiAnalytics, SHttpProtocol::reqGet, "api/v1/analytics/acceptance-report" },
			{ methGoodsPrices, apiDiscountsPrices, SHttpProtocol::reqGet, "api/v2/list/goods/filter" },
			{ methContentCardsList, apiContent, SHttpProtocol::reqPost, "content/v2/get/cards/list" },
		};
		//https://content-api.wildberries.ru/content/v2/cards/upload
		//https://discounts-prices-api.wildberries.ru/api/v2/upload/task
		//https://supplies-api.wildberries.ru/api/v1/acceptance/coefficients
		//https://recommend-api.wildberries.ru/api/v1/ins
		//https://advert-api.wildberries.ru/adv/v1/save-ad
		// https://seller-analytics-api.wildberries.ru/api/v1/analytics/excise-report
		//https://statistics-api.wildberries.ru/api/v1/supplier/incomes
		// https://common-api.wildberries.ru/api/v1/tariffs/commission
		bool ok = false;
		rResult.Z();
		int  req = 0;
		SString temp_buf;
		{
			bool   local_ok = false;
			for(uint midx = 0; midx < SIZEOFARRAY(meth_list); midx++) {
				if(meth_list[midx].Meth == meth) {
					const int api = meth_list[midx].Api;
					if(SIntToSymbTab_GetSymb(api_list, SIZEOFARRAY(api_list), api, temp_buf)) {
						rResult.Cat("https").Cat("://").Cat(temp_buf).DotCat("wildberries").DotCat("ru");
						if(!isempty(meth_list[midx].P_UrlSuffix)) {
							req = meth_list[midx].Req;
							rResult.SetLastDSlash().Cat(meth_list[midx].P_UrlSuffix);
							local_ok = true;
						}
					}
					break;
				}
			}
			if(local_ok) {
				ok = true;
			}
		}
		/*switch(query) {
			case qAuthLogin: entry.Set(SHttpProtocol::reqPost, "login-api/api/v1/Auth/login"); break;
			case qAuthExtTok: entry.Set(SHttpProtocol::reqPost, "login-api/api/v1/Auth/extended-token"); break;
			case qGetWarehouses: entry.Set(SHttpProtocol::reqGet, "distribution-api/api/v1/Distributions/warehouses"); break;
			case qGetProducts: entry.Set(SHttpProtocol::reqGet, "product-api/api/v1/Products/integration"); break;
			case qGetClients: entry.Set(SHttpProtocol::reqPost, "client-api/api/v1/Clients/GetFilteredList"); break;
			case qSendSellout: entry.Set(SHttpProtocol::reqPost, "sales-api/api/v2/Sales/sellout"); break;
			case qSendSellin: entry.Set(SHttpProtocol::reqPost, "sales-api/api/v2/Sales/sellin"); break;
			case qSendRest: entry.Set(SHttpProtocol::reqPost, "sales-api/api/v1/Sales/warehousebalances"); break;
		}*/
		/*if(!isempty(entry.P_Path)) {
			rResult.SetLastDSlash().Cat(entry.P_Path);
		}*/
		ASSIGN_PTR(pReq, req);
		return ok;
	}
	int    Helper_InitRequest(int meth, SString & rUrlBuf, StrStrAssocArray & rHdrFlds)
	{
		int    ok = 0;
		rUrlBuf.Z();
		rHdrFlds.Z();
		if(GuaPack.Rec.ServiceIdent == PPGLS_WILDBERRIES) {
			SString token;
			if(GuaPack.GetAccessKey(token) > 0) {
				//InetUrl url(MakeTargetUrl_(qAuthLogin, &req, url_buf));
				int   req = 0;
				if(MakeTargetUrl_(meth, &req/*SHttpProtocol::reqXXX*/, rUrlBuf)) {
					SString hdr_buf;
					MakeHeaderFields(token, &rHdrFlds, hdr_buf);
					ok = 1;
				}
			}
		}
		return ok;
	}
	SString Token;
	PPGlobalServiceLogTalkingHelper Lth;
	TSCollection <Warehouse> WhList;
};

int PPMarketplaceInterface::Init(PPID guaID)
{
	int    ok = 1;
	PPObjGlobalUserAcc gua_obj;
	if(gua_obj.GetPacket(guaID, &GuaPack) > 0) {
		; // ok
	}
	else
		ok = 0;
	return ok;
}
//
//
//
PPMarketplaceInterface_Wildberries::Warehouse::Warehouse() : ID(0), Flags(0)
{
}
		
PPMarketplaceInterface_Wildberries::Warehouse & PPMarketplaceInterface_Wildberries::Warehouse::Z()
{
	ID = 0;
	Flags = 0;
	Name.Z();
	Address.Z();
	return *this;
}

bool PPMarketplaceInterface_Wildberries::Warehouse::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("ID")) {
				ID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("name")) {
				Name = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("address")) {
				Address = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("workTime")) {
				;
			}
			else if(p_cur->Text.IsEqiAscii("acceptsQR")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1)
					Flags |= fAcceptsQR;
			}
		}
		ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::WareBase::WareBase() : ID(0)
{
}
		
PPMarketplaceInterface_Wildberries::WareBase & PPMarketplaceInterface_Wildberries::WareBase::Z()
{
	ID = 0;
	Name.Z();
	TechSize.Z();
	SupplArticle.Z();
	Barcode.Z();
	Category.Z();
	Brand.Z();
	return *this;
}

bool PPMarketplaceInterface_Wildberries::WareBase::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("supplierArticle")) {
				SupplArticle = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("nmId")) {
				ID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("barcode")) {
				Barcode = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("category")) {
				Category = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("subject")) {
				Name = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("brand")) {
				Brand = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("techSize")) {
				TechSize = p_cur->P_Child->Text;
			}
		}
		ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::Stock::Stock() : Ware(), DtmLastChange(ZERODATETIME), Qtty(0.0), QttyFull(0.0), InWayToClient(0.0), InWayFromClient(0.0),
	Price(0.0), Discount(0.0), Flags(0)
{
}
		
PPMarketplaceInterface_Wildberries::Stock & PPMarketplaceInterface_Wildberries::Stock::Z()
{
	Ware.Z();
	DtmLastChange.Z();
	WarehouseName.Z();
	Qtty = 0.0;
	QttyFull = 0.0;
	InWayToClient = 0.0;
	InWayFromClient = 0.0;
	Price = 0.0;
	Discount = 0.0;
	Flags = 0;
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::Stock::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		Ware.FromJsonObj(pJs);
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("lastChangeDate")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					DtmLastChange = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("warehouseName")) {
				WarehouseName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("quantity")) {
				Qtty = p_cur->P_Child->Text.ToReal_Plain();
			}
			else if(p_cur->Text.IsEqiAscii("inWayToClient")) {
				InWayToClient = p_cur->P_Child->Text.ToReal_Plain();
			}
			else if(p_cur->Text.IsEqiAscii("inWayFromClient")) {
				InWayFromClient = p_cur->P_Child->Text.ToReal_Plain();
			}
			else if(p_cur->Text.IsEqiAscii("quantityFull")) {
				QttyFull = p_cur->P_Child->Text.ToReal_Plain();
			}
			else if(p_cur->Text.IsEqiAscii("brand")) {
				;
			}
			else if(p_cur->Text.IsEqiAscii("Price")) {
				Price = p_cur->P_Child->Text.ToReal_Plain();
			}
			else if(p_cur->Text.IsEqiAscii("Discount")) {
				Discount = p_cur->P_Child->Text.ToReal_Plain();
			}
			else if(p_cur->Text.IsEqiAscii("isSupply")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1) {
					Flags |= fIsSupply;
				}
			}
			else if(p_cur->Text.IsEqiAscii("isRealization")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1) {
					Flags |= fIsRealization;
				}
			}
			else if(p_cur->Text.IsEqiAscii("SCCode")) {
				;
			}
		}
		ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::Income::Income() : Ware(), Dtm(ZERODATETIME), DtmLastChange(ZERODATETIME), DtmClose(ZERODATETIME), IncomeID(0),
	Qtty(0.0), TotalPrice(0.0)
{
}
		
PPMarketplaceInterface_Wildberries::Income & PPMarketplaceInterface_Wildberries::Income::Z()
{
	Ware.Z();
	IncomeID = 0;
	Dtm = ZERODATETIME;
	DtmLastChange = ZERODATETIME;
	DtmClose = ZERODATETIME;
	Number.Z();
	Qtty = 0.0;
	TotalPrice = 0.0;
	WarehouseName.Z();
	Status.Z();
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::Income::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		LDATETIME dtm;
		Ware.FromJsonObj(pJs);
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("lastChangeDate")) {
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					DtmLastChange = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("date")) {
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					Dtm = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("dateClose")) {
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					DtmClose = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("incomeId")) {
				IncomeID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("number")) {
				Number = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("warehouseName")) {
				WarehouseName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("status")) {
				Status = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("quantity")) {
				Qtty = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("totalPrice")) {
				TotalPrice = p_cur->P_Child->Text.ToReal();
			}
		}
		ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::Sale::Sale() : Ware(), Dtm(ZERODATETIME), DtmLastChange(ZERODATETIME), DtmCancel(ZERODATETIME), IncomeID(0), Flags(0),
	TotalPrice(0.0), DiscountPct(0.0), Spp(0.0), PaymentSaleAmount(0.0), ForPay(0.0), FinishedPrice(0.0), PriceWithDiscount(0.0) 
{
}
		
PPMarketplaceInterface_Wildberries::Sale & PPMarketplaceInterface_Wildberries::Sale::Z()
{
	Dtm.Z();
	DtmLastChange.Z();
	Ware.Z();
	WarehouseName.Z();
	CountryName.Z();
	DistrictName.Z();
	RegionName.Z();
	IncomeID = 0;
	Flags = 0;
	TotalPrice = 0.0;
	DiscountPct = 0.0;
	Spp = 0.0;
	PaymentSaleAmount = 0.0;
	ForPay = 0.0;
	FinishedPrice = 0.0;
	PriceWithDiscount = 0.0;
	SaleId.Z();
	OrderType.Z();
	Sticker.Z();
	GNumber.Z();
	SrID.Z();
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::Sale::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		LDATETIME dtm;
		Ware.FromJsonObj(pJs);
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("lastChangeDate")) {
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					DtmLastChange = dtm;
				}
			}
			if(p_cur->Text.IsEqiAscii("date")) {
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					Dtm = dtm;
				}
			}
			if(p_cur->Text.IsEqiAscii("cancelDate")) {
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					DtmCancel = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("warehouseName")) {
				WarehouseName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("countryName")) {
				CountryName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("oblastOkrugName")) {
				DistrictName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("regionName")) {
				RegionName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("incomeID")) {
				IncomeID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("totalPrice")) {
				TotalPrice = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("discountPercent")) {
				DiscountPct = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("spp")) {
				Spp = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("paymentSaleAmount")) { // sale only
				PaymentSaleAmount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("forPay")) { // sale only
				ForPay = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("finishedPrice")) {
				FinishedPrice = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("priceWithDisc")) {
				PriceWithDiscount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("orderType")) {
				OrderType = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("sticker")) {
				Sticker = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("gNumber")) {
				GNumber = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("srid")) {
				SrID = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("saleID")) {
				SaleId = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("isSupply")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1) {
					Flags |= fIsSupply;
				}
			}
			else if(p_cur->Text.IsEqiAscii("isRealization")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1) {
					Flags |= fIsRealization;
				}
			}
			else if(p_cur->Text.IsEqiAscii("isCancel")) { // Order only
				if(SJson::GetBoolean(p_cur->P_Child) == 1) {
					Flags |= fIsCancel;
				}
			}
		}
		ok = true;
	}
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestWarehouseList(TSCollection <Warehouse> & rList)
{
	rList.freeAll();
	int    ok = 1;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methWarehouses, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					SJson * p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								uint   new_item_pos = 0;
								Warehouse * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestWareList()
{
	int    ok = 1;
	SString temp_buf;
	SString url_buf;
	SString req_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methContentCardsList, url_buf, hdr_flds));
	{
		/*
			{
				"settings": {
					"sort": {
						"ascending": false
					},
					"filter": {
						"textSearch": "",
						"allowedCategoriesOnly": true,
						"tagIDs": [],
						"objectIDs": [],
						"brands": [],
						"imtID": 0,
						"withPhoto": -1
					},
					"cursor": {
						"updatedAt": "",
						"nmID": 0,
						"limit": 11
					}
				}
			}
		*/
		SJson json_req(SJson::tOBJECT);
		/*{
			SJson * p_js_sort = new SJson(SJson::tOBJECT);
			p_js_sort->InsertBool("ascending", "true");
			json_req.Insert("sort", p_js_sort);
		}*/
		{
			SJson * p_js_filt = new SJson(SJson::tOBJECT);
			p_js_filt->InsertInt("withPhoto", -1);
			//p_js_filt->InsertString("textSearch", "");
			//p_js_filt->InsertBool("allowedCategoriesOnly", true);
			//p_js_filt->InsertInt("imtID", 0);
			json_req.Insert("filter", p_js_filt);
		}
		{
			SJson * p_js_cursor = new SJson(SJson::tOBJECT);
			//p_js_cursor->InsertString("updatedAt", "");
			//p_js_cursor->InsertInt("nmID", 0);
			p_js_cursor->InsertInt("limit", 100);
			json_req.Insert("cursor", p_js_cursor);
		}
		THROW_SL(json_req.ToStr(req_buf));
	}
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		url_buf.CatChar('?').CatEq("locale", "ru");
		Lth.Log("req", url_buf, temp_buf.Z());
		InetUrl url(url_buf);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					SJson * p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								/*
								uint   new_item_pos = 0;
								Warehouse * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
								*/
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestGoodsPrices()
{
	//rList.freeAll();
	int    ok = 1;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methGoodsPrices, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("limit", 1000).CatChar('&').CatEq("offset", 0);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					SJson * p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								/*
								uint   new_item_pos = 0;
								Warehouse * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
								*/
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestCommission()
{
	// https://common-api.wildberries.ru/api/v1/tariffs/commission
	int    ok = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methCommission, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("locale", "ru");
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestIncomes(TSCollection <Income> & rList)
{
	rList.freeAll();
	int    ok = 1;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methIncomes, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("dateFrom", encodedate(1, 1, 2024), DATF_ISO8601CENT);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					SJson * p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								uint   new_item_pos = 0;
								Income * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestStocks(TSCollection <Stock> & rList)
{
	rList.freeAll();
	int    ok = 1;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methStocks, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("dateFrom", encodedate(1, 1, 2024), DATF_ISO8601CENT);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					SJson * p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								uint   new_item_pos = 0;
								Stock * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
							}
						}
					}
				}
				{
					//int PPObjOprKind::GetEdiStockOp(PPID * pID, int use_ta)
					/*
					PPObjOprKind op_obj;
					PPID   op_id = 0;
					if(op_obj.GetEdiStockOp(&op_id, 1)) {
						
					}
					*/
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestSupplies()
{
	int    ok = 1;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methSupples, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("limit", 1000L).CatChar('&').CatEq("next", 0L);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
			}
		}
	}
	CATCHZOK
	return ok;	
}

int PPMarketplaceInterface_Wildberries::RequestAcceptanceReport(const DateRange & rPeriod)
{
	int    ok = 1;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	if(checkdate(rPeriod.low) && checkdate(rPeriod.upp) && rPeriod.upp >= rPeriod.low) {
		THROW(Helper_InitRequest(methAcceptanceReport, url_buf, hdr_flds));
		{
			ScURL c;
			SString reply_buf;
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
			temp_buf.Z().CatEq("dateFrom", rPeriod.low, DATF_ISO8601CENT).CatChar('&').CatEq("dateTo", rPeriod.upp, DATF_ISO8601CENT);
			url_buf.CatChar('?').Cat(temp_buf);
			Lth.Log("req", url_buf, temp_buf.Z());
			THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, reply_buf);
				}
			}
		}
	}
	CATCHZOK
	return ok;	
}

int PPMarketplaceInterface_Wildberries::RequestOrders(TSCollection <Sale> & rList)
{
	rList.freeAll();
	int    ok = 1;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methOrders, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("dateFrom", encodedate(1, 1, 2024), DATF_ISO8601CENT);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					SJson * p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								uint   new_item_pos = 0;
								Sale * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestSales(TSCollection <Sale> & rList)
{
	rList.freeAll();
	int    ok = 1;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methSales, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("dateFrom", encodedate(1, 1, 2024), DATF_ISO8601CENT);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					SJson * p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								uint   new_item_pos = 0;
								Sale * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::CreateWarehouse(PPID * pID, int64 outerId, const char * pOuterName, const char * pAddress, int use_ta) // @construction
{
	int    ok = -1;
	PPID   result_id = 0;
	PPID   psn_id = 0;
	SString temp_buf;
	PPObjLocation & r_loc_obj = PsnObj.LocObj;
	LocationTbl::Rec loc_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		const int  gmppr = GetMarketplacePerson(&psn_id, 0);
		THROW(gmppr);
		{
			PPID   local_id = 0;
			SString code_buf;
			code_buf.Z().Cat(outerId);
			if(r_loc_obj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, code_buf, &local_id, &loc_rec) > 0 && loc_rec.OwnerID == psn_id) {
				result_id = loc_rec.ID;
				ok = 1;
			}
			else {
				PPID   parent_id = 0; // Идентификатор папки для складов маркетплейса.
				PPLocationPacket loc_pack;
				THROW(!isempty(pOuterName));
				{
					PPID   local_id = 0;
					temp_buf.Z().Cat("MP").CatChar('.').Cat(GetSymbol()).ToUpper();
					if(r_loc_obj.P_Tbl->SearchCode(LOCTYP_WAREHOUSEGROUP, temp_buf, &local_id, &loc_rec) > 0) {
						parent_id = local_id;
					}
					else {
						PPLocationPacket loc_pack;
						loc_pack.Type = LOCTYP_WAREHOUSEGROUP;
						STRNSCPY(loc_pack.Code, temp_buf);
						STRNSCPY(loc_pack.Name, temp_buf);
						THROW(r_loc_obj.PutPacket(&parent_id, &loc_pack, 0));
					}
				}
				loc_pack.Type = LOCTYP_WAREHOUSE;
				loc_pack.OwnerID = psn_id;
				loc_pack.ParentID = parent_id;
				STRNSCPY(loc_pack.Code, code_buf);
				temp_buf = pOuterName;
				if(temp_buf.IsLegalUtf8()) {
					temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				}
				STRNSCPY(loc_pack.Name, temp_buf);
				temp_buf = pAddress;
				if(temp_buf.NotEmptyS()) {
					if(temp_buf.IsLegalUtf8())
						temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					LocationCore::SetExField(&loc_pack, LOCEXSTR_FULLADDR, temp_buf);
					loc_pack.Flags |= LOCF_MANUALADDR;
				}
				THROW(r_loc_obj.PutPacket(&result_id, &loc_pack, 0));
				ok = 2;
			}
		}
		THROW(tra.Commit());
	}
	ASSIGN_PTR(pID, result_id);
	CATCHZOK
	return ok;
}

const PPMarketplaceInterface_Wildberries::Warehouse * PPMarketplaceInterface_Wildberries::SearchWarehouseByName(const TSCollection <Warehouse> & rWhList, const char * pWhName) const
{
	const Warehouse * p_result = 0;
	for(uint i = 0; !p_result && i < rWhList.getCount(); i++) {
		const Warehouse * p_iter = rWhList.at(i);
		if(p_iter && p_iter->Name.IsEqiUtf8(pWhName)) {
			p_result = p_iter;
		}
	}
	return p_result;
}

int PPMarketplaceInterface_Wildberries::CreateWare(PPID * pID, const WareBase & rWare, int use_ta) // @construction
{
	int    ok = -1;
	PPID   result_id = 0;
	SString temp_buf;
	BarcodeTbl::Rec bc_rec;
	Goods2Tbl::Rec goods_rec;
	if(GObj.SearchByBarcode(rWare.Barcode, &bc_rec, &goods_rec, 0) > 0) {
		assert(goods_rec.ID > 0 && bc_rec.GoodsID > 0 && goods_rec.ID == bc_rec.GoodsID); // @paranoic
		result_id = goods_rec.ID;
		ok = 1;
	}
	else if(rWare.Name.NotEmpty()) {
		const PPGoodsConfig & r_cfg = GObj.GetConfig();
		const PPID acs_id = GetMarketplaceAccSheetID();
		SString goods_name(rWare.Name);
		SString ar_code;
		PPID   mp_ar_id = 0;
		PPID   temp_id = 0;
		if(rWare.ID)
			ar_code.Cat(rWare.ID);
		if(goods_name.IsLegalUtf8()) {
			goods_name.Transf(CTRANSF_UTF8_TO_INNER);
		}
		if(rWare.Barcode.IsEmpty() && GObj.SearchByName(goods_name, &temp_id, &goods_rec) > 0) {
			result_id = goods_rec.ID;
			ok = 1;
		}
		else {
			PPID   mp_psn_id = 0;
			PPObjBrand brand_obj;
			{
				PPTransaction tra(use_ta);
				THROW(tra);
				GetMarketplacePerson(&mp_psn_id, 0/*use_ta*/);
				if(ar_code.NotEmpty() && acs_id) {
					if(mp_psn_id) {
						ArObj.P_Tbl->PersonToArticle(mp_psn_id, acs_id, &mp_ar_id);
						if(mp_ar_id) {
							ArGoodsCodeTbl::Rec ar_code_rec;
							if(GObj.P_Tbl->SearchByArCode(mp_ar_id, ar_code, &ar_code_rec, &goods_rec) > 0) {
								result_id = goods_rec.ID;
								ok = 1;
							}
						}
					}
				}
				if(ok < 0) {
					PPGoodsPacket goods_pack;
					assert(result_id == 0);
					goods_pack.Rec.Kind = PPGDSK_GOODS;
					STRNSCPY(goods_pack.Rec.Name, goods_name);
					STRNSCPY(goods_pack.Rec.Abbr, goods_name);
					if(rWare.Barcode.NotEmpty()) {
						goods_pack.AddCode(rWare.Barcode, 0, 1);
					}
					if(ar_code.NotEmpty()) {
						if(acs_id) {
							if(!mp_ar_id) {
								ArObj.P_Tbl->PersonToArticle(mp_psn_id, acs_id, &mp_ar_id);
							}
							if(mp_ar_id) {
								ArGoodsCodeTbl::Rec ar_code_rec;
								STRNSCPY(ar_code_rec.Code, ar_code);
								ar_code_rec.ArID = mp_ar_id;
								ar_code_rec.Pack = 1000; // 1.0
								ar_code.CopyTo(ar_code_rec.Code, sizeof(ar_code_rec.Code));
								goods_pack.ArCodes.insert(&ar_code_rec);
							}
						}
					}
					if(rWare.Brand.NotEmpty()) {
						temp_buf = rWare.Brand;
						if(temp_buf.IsLegalUtf8()) {
							temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						}
						PPID   brand_id = 0;
						THROW(brand_obj.AddSimple(&brand_id, temp_buf, 0, 0/*use_ta*/));
						goods_pack.Rec.BrandID = brand_id;
					}
					goods_pack.Rec.UnitID = r_cfg.DefUnitID;
					goods_pack.Rec.ParentID = r_cfg.DefGroupID;
					THROW(GObj.PutPacket(&result_id, &goods_pack, 0/*use_ta*/));
					ok = 2;
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pID, result_id);
	return ok;
}

PPID PPMarketplaceInterface_Wildberries::CreateReceipt(int64 incomeId, LDATE dt, PPID locID, PPID goodsID, double qtty, int use_ta)
{
	PPID   result_lot_id = 0;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	const  PPID rcpt_op_id = CConfig.ReceiptOp;
	PPID   suppl_id = 0;
	SString bill_code;
	PPBillPacket::SetupObjectBlock sob;
	PPBillPacket pack;
	THROW(incomeId); // @todo @err
	THROW(rcpt_op_id); // @todo @err
	THROW(checkdate(dt)); 
	bill_code.Z().Cat(incomeId);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ArObj.GetMainOrgAsSuppl(&suppl_id, 1/*processAbsense*/, 0/*use_ta*/));
		THROW(pack.CreateBlank_WithoutCode(rcpt_op_id, 0, locID, 0));
		pack.Rec.Object = suppl_id;
		pack.Rec.Dt = dt;
		STRNSCPY(pack.Rec.Code, bill_code);
		THROW(pack.SetupObject(suppl_id, sob));
		{
			//
			//pack.Rec.DueDate = checkdate(p_src_pack->DlvrDtm.d) ? p_src_pack->DlvrDtm.d : ZERODATE;
			//sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop;
			//
			PPTransferItem ti;
			LongArray row_idx_list;
			ti.GoodsID = goodsID;
			ti.Cost = 0.0;
			ti.Price = 0.0;
			ti.Quantity_ = fabs(qtty);
			THROW(pack.InsertRow(&ti, &row_idx_list));
			//pack.Se
			assert(row_idx_list.getCount() == 1);
			const long new_row_idx = row_idx_list.get(0);
			pack.LTagL.AddNumber(PPTAG_LOT_SN, new_row_idx, temp_buf.Z().Cat(incomeId));
			pack.InitAmounts();
			p_bobj->FillTurnList(&pack);
			THROW(p_bobj->TurnPacket(&pack, 0));
			assert(pack.GetTCount() == 1);
			THROW(pack.GetTCount() == 1);
			result_lot_id = pack.ConstTI(0).LotID;
		}
		THROW(tra.Commit());
	}
	CATCH
		result_lot_id = 0;
	ENDCATCH
	return result_lot_id;
}

int PPMarketplaceInterface_Wildberries::ImportReceipts()
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	const  PPID rcpt_op_id = CConfig.ReceiptOp;
	if(rcpt_op_id) {
		PPID   suppl_id = 0;
		SString bill_code;
		TSCollection <PPMarketplaceInterface_Wildberries::Income> income_list;
		THROW(ArObj.GetMainOrgAsSuppl(&suppl_id, 1/*processAbsense*/, 1/*use_ta*/));
		RequestIncomes(income_list);
		if(income_list.getCount()) {
			for(uint i = 0; i < income_list.getCount(); i++) {
				const Income * p_wb_item = income_list.at(i);
				if(p_wb_item) {
					bill_code.Z().Cat(p_wb_item->IncomeID);
					if(p_bobj->P_Tbl->SearchByCode(bill_code, rcpt_op_id, ZERODATE, 0) > 0) {
						;
					}
					else {
						PPID   wh_id = 0;
						PPBillPacket pack;
						PPID   ex_bill_id = 0;
						Goods2Tbl::Rec goods_rec;
						PPBillPacket::SetupObjectBlock sob;
						if(p_wb_item->WarehouseName.NotEmpty()) {
							const Warehouse * p_wh = SearchWarehouseByName(WhList, p_wb_item->WarehouseName);
							if(p_wh) {
								int r = CreateWarehouse(&wh_id, p_wh->ID, p_wh->Name, p_wh->Address, 1);
							}
						}
						if(!wh_id)
							wh_id = LConfig.Location;
						PPID   goods_id = 0;
						if(CreateWare(&goods_id, p_wb_item->Ware, 1/*use_ta*/)) {
							const LDATE dt = checkdate(p_wb_item->Dtm.d) ? p_wb_item->Dtm.d : getcurdate_();
							const PPID lot_id = CreateReceipt(p_wb_item->IncomeID, dt, wh_id, goods_id, fabs(p_wb_item->Qtty), 1);
							if(lot_id) {
								ok = 1;
							}
						}
						else {
							// @todo @err
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

PPID PPMarketplaceInterface_Wildberries::AdjustReceiptOnExpend(int64 incomeId, LDATE dt, PPID locID, PPID goodsID, double neededQtty, int use_ta)
{
	PPID   result_lot_id = 0;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	if(incomeId && locID) {
		temp_buf.Z().Cat(incomeId);
		PPIDArray lot_id_list;
		bool   lot_found = false;
		if(p_bobj->SearchLotsBySerialExactly(temp_buf, &lot_id_list) > 0) {
			ReceiptTbl::Rec lot_rec;
			PPID   lot_id = 0;
			for(uint i = 0; !lot_id && i < lot_id_list.getCount(); i++) {
				const PPID iter_lot_id = lot_id_list.get(i);
				if(p_bobj->trfr->Rcpt.Search(iter_lot_id, &lot_rec) > 0 && lot_rec.GoodsID == goodsID && lot_rec.LocID == locID) {
					lot_id = iter_lot_id;
				}
			}
			if(lot_id) {
				bool   do_update = false;
				double adj_qtty = lot_rec.Quantity;
				LDATE  adj_date = lot_rec.Dt;

				assert(lot_rec.ID == lot_id);
				assert(lot_rec.LocID == locID);
				PPID   rcpt_bill_id = lot_rec.BillID;
				double down_lim = 0.0;
				double up_lim = 0.0;
				if(lot_rec.Dt > dt) {
					adj_date = dt;
					do_update = true;
					//
					down_lim = lot_rec.Rest; // Если дата лота превышала дату создаваемой расходной операции, то остаток, 
						// доступный для расхода равен тепкущему остатку лота (с учетом того факта, что мы переместим лот назад по времени)
				}
				else {
					p_bobj->trfr->GetBounds(lot_id, dt, -1, &down_lim, &up_lim);
				}
				if(down_lim < neededQtty) {
					adj_qtty = lot_rec.Quantity + (neededQtty - down_lim);
					do_update = true;
				}
				if(do_update) {
					PPBillPacket rcpt_bill_pack;
					THROW(p_bobj->ExtractPacket(rcpt_bill_id, &rcpt_bill_pack) > 0);
					{
						uint   ti_pos = 0;
						THROW(rcpt_bill_pack.SearchLot(lot_id, &ti_pos));
						if(checkdate(adj_date)) {
							rcpt_bill_pack.Rec.Dt = adj_date;
						}
						if(adj_qtty) {
							rcpt_bill_pack.TI(ti_pos).Quantity_ = adj_qtty;
						}
						THROW(p_bobj->UpdatePacket(&rcpt_bill_pack, 1/*use_ta*/));
					}
				}
				result_lot_id = lot_id;
				lot_found = true;
			}
		}
		if(!lot_found) {
			result_lot_id = CreateReceipt(incomeId, dt, locID, goodsID, fabs(neededQtty), 1);
		}
	}
	CATCH
		result_lot_id = 0;
	ENDCATCH
	return result_lot_id;
}

int PPMarketplaceInterface_Wildberries::ImportSales()
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	PPID   op_id = 0;
	TSCollection <PPMarketplaceInterface_Wildberries::Sale> sale_list;
	PPAlbatrossConfig  albtr_cfg;
	const PPID order_op_id = (PPAlbatrosCfgMngr::Get(&albtr_cfg) > 0) ? albtr_cfg.Hdr.OpID : 0;
	PPOprKind order_op_rec;
	THROW(order_op_id); // @todo @err
	THROW(GetOpData(order_op_id, &order_op_rec) > 0);
	{
		PPOprKind op_rec;
		for(PPID iter_op_id = 0; !op_id && EnumOperations(PPOPT_GOODSEXPEND, &iter_op_id, &op_rec) > 0;) {
			if(op_rec.Flags & OPKF_ONORDER && op_rec.AccSheetID == order_op_rec.AccSheetID) {
				op_id = op_rec.ID;
			}
		}
	}
	THROW(op_id); // @todo @err
	RequestSales(sale_list);
	if(sale_list.getCount()) {
		SString bill_code;
		SString ord_bill_code;
		SString serial_buf;
		SString ord_serial_buf;
		for(uint i = 0; i < sale_list.getCount(); i++) {
			const PPMarketplaceInterface_Wildberries::Sale * p_wb_item = sale_list.at(i);
			if(p_wb_item) {
				BillTbl::Rec ord_bill_rec;
				bill_code.Z().Cat(p_wb_item->SaleId);
				ord_bill_code.Z().Cat(p_wb_item->GNumber);
				if(p_bobj->P_Tbl->SearchByCode(bill_code, op_id, ZERODATE, 0) > 0) {
					// Документ уже акцептирован
				}
				else if(p_bobj->P_Tbl->SearchByCode(ord_bill_code, order_op_id, ZERODATE, &ord_bill_rec) > 0) {
					PPBillPacket ord_pack;
					THROW(p_bobj->ExtractPacket(ord_bill_rec.ID, &ord_pack) > 0);
					{
						PPID   wh_id = 0;
						PPBillPacket pack;
						PPID   ex_bill_id = 0;
						Goods2Tbl::Rec goods_rec;
						PPBillPacket::SetupObjectBlock sob;
						if(p_wb_item->WarehouseName.NotEmpty()) {
							const Warehouse * p_wh = SearchWarehouseByName(WhList, p_wb_item->WarehouseName);
							if(p_wh) {
								int r = CreateWarehouse(&wh_id, p_wh->ID, p_wh->Name, p_wh->Address, 1);
							}
						}
						if(!wh_id)
							wh_id = LConfig.Location;
						if(pack.CreateBlank_WithoutCode(op_id, 0, wh_id, 1)) {
							const double sold_quantity = 1.0; // 
							pack.Rec.Dt = checkdate(p_wb_item->Dtm.d) ? p_wb_item->Dtm.d : getcurdate_();
							STRNSCPY(pack.Rec.Code, bill_code);
							//
							//pack.Rec.DueDate = checkdate(p_src_pack->DlvrDtm.d) ? p_src_pack->DlvrDtm.d : ZERODATE;
							//sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop;
							//
							PPID   goods_id = 0;
							if(CreateWare(&goods_id, p_wb_item->Ware, 1/*use_ta*/)) {
								//ord_pack.S
								PPID   lot_id = AdjustReceiptOnExpend(p_wb_item->IncomeID, pack.Rec.Dt, wh_id, goods_id, sold_quantity, 1/*use_ta*/);
								if(lot_id) {
									uint   ord_ti_idx = 0; // [1..]
									for(uint oti = 0; !ord_ti_idx && oti < ord_pack.GetTCount(); oti++) {
										const PPTransferItem & r_ord_ti = ord_pack.ConstTI(oti);
										if(labs(r_ord_ti.GoodsID) == goods_id) {
											if(p_wb_item->IncomeID != 0) {
												ord_pack.LTagL.GetNumber(PPTAG_LOT_SN, oti, ord_serial_buf);
												if(ord_serial_buf.ToInt64() == p_wb_item->IncomeID) {
													ord_ti_idx = oti+1;
												}
											}
										}
									}
									if(ord_ti_idx) {
										if(p_bobj->InsertShipmentItemByOrder(&pack, &ord_pack, static_cast<int>(ord_ti_idx)-1, lot_id/*srcLotID*/, 0)) {
											//assert(row_idx_list.getCount() == 1);
											const long new_row_idx = 0;//row_idx_list.get(0);
											pack.LTagL.AddNumber(PPTAG_LOT_SN, new_row_idx, temp_buf.Z().Cat(p_wb_item->IncomeID));
											pack.InitAmounts();
											p_bobj->FillTurnList(&pack);
											if(p_bobj->TurnPacket(&pack, 1)) {
												ok = 1;
											}
											else {
												// @todo @err
											}
										}
										/*
										PPTransferItem ti;
										LongArray row_idx_list;
										ti.GoodsID = goods_id;
										ti.Price = p_wb_item->FinishedPrice;
										ti.Quantity_ = sold_quantity;
										ti.SetupLot(lot_id, 0, 0);
										if(pack.InsertRow(&ti, &row_idx_list)) {
											//pack.Se
											assert(row_idx_list.getCount() == 1);
											const long new_row_idx = row_idx_list.get(0);
											pack.LTagL.AddNumber(PPTAG_LOT_SN, new_row_idx, temp_buf.Z().Cat(p_wb_item->IncomeID));
											pack.InitAmounts();
											p_bobj->FillTurnList(&pack);
											if(p_bobj->TurnPacket(&pack, 1)) {
												ok = 1;
											}
											else {
												// @todo @err
											}
										}*/
									}
								}
								else {
									// @todo @err
								}
							}
							else {
								// @todo @err
							}
						}
					}
				}
				else {
					// Документ заказа не найден
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::ImportOrders()
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	TSCollection <PPMarketplaceInterface_Wildberries::Sale> order_list;
	RequestOrders(order_list);
	if(order_list.getCount()) {
		PPAlbatrossConfig  albtr_cfg;
		const PPID order_op_id = (PPAlbatrosCfgMngr::Get(&albtr_cfg) > 0) ? albtr_cfg.Hdr.OpID : 0;
		THROW(order_op_id); // @todo @err
		{
			SString bill_code;
			LongArray seen_idx_list;
			for(uint i = 0; i < order_list.getCount(); i++) {
				if(!seen_idx_list.lsearch(static_cast<long>(i))) {
					seen_idx_list.add(static_cast<long>(i));
					const PPMarketplaceInterface_Wildberries::Sale * p_wb_item = order_list.at(i);
					if(p_wb_item) {
						bill_code.Z().Cat(p_wb_item->GNumber);
						if(p_bobj->P_Tbl->SearchByCode(bill_code, order_op_id, ZERODATE, 0) > 0) {
							;
						}
						else {
							PPID   wh_id = 0;
							PPBillPacket pack;
							PPID   ex_bill_id = 0;
							Goods2Tbl::Rec goods_rec;
							PPBillPacket::SetupObjectBlock sob;
							if(p_wb_item->WarehouseName.NotEmpty()) {
								const Warehouse * p_wh = SearchWarehouseByName(WhList, p_wb_item->WarehouseName);
								if(p_wh) {
									int r = CreateWarehouse(&wh_id, p_wh->ID, p_wh->Name, p_wh->Address, 1);
								}
							}
							if(!wh_id)
								wh_id = LConfig.Location;
							if(pack.CreateBlank_WithoutCode(order_op_id, 0, wh_id, 1)) {
								pack.Rec.Dt = checkdate(p_wb_item->Dtm.d) ? p_wb_item->Dtm.d : getcurdate_();
								STRNSCPY(pack.Rec.Code, bill_code);
								//
								//pack.Rec.DueDate = checkdate(p_src_pack->DlvrDtm.d) ? p_src_pack->DlvrDtm.d : ZERODATE;
								//sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop;
								//
								PPID   goods_id = 0;
								if(CreateWare(&goods_id, p_wb_item->Ware, 1/*use_ta*/)) {
									PPTransferItem ti;
									LongArray row_idx_list;
									ti.GoodsID = goods_id;
									ti.Price = p_wb_item->FinishedPrice;
									ti.Quantity_ = 1.0;
									{
										//
										// Если покупатель заказал несколько штук одного и того же, то каждая штука
										// будет задана отдельным документом заказа, но все эти документы будут содержать одно и то же значение GNumber
										//
										double sum_price = p_wb_item->FinishedPrice * 1.0;
										double sum_qtty = 1.0;
										bool   is_there_other_items = false;
										for(uint j = i+1; j < order_list.getCount(); j++) {
											const PPMarketplaceInterface_Wildberries::Sale * p_wb_item_inner = order_list.at(j);
											if(p_wb_item_inner && p_wb_item_inner->GNumber.IsEqiUtf8(p_wb_item->GNumber)) {
												if(!seen_idx_list.lsearch(static_cast<long>(j))) {
													seen_idx_list.add(static_cast<long>(j));
													ti.Quantity_ += 1.0;
													sum_price += (p_wb_item_inner->FinishedPrice * 1.0);
													sum_qtty += 1.0;
													is_there_other_items = true;
												}
											}
										}
										if(is_there_other_items) {
											assert(sum_qtty == ti.Quantity_);
											ti.Price = sum_price / sum_qtty;
										}
									}
									if(pack.InsertRow(&ti, &row_idx_list)) {
										//pack.Se
										assert(row_idx_list.getCount() == 1);
										const long new_row_idx = row_idx_list.get(0);
										pack.LTagL.AddNumber(PPTAG_LOT_SN, new_row_idx, temp_buf.Z().Cat(p_wb_item->IncomeID));
										pack.InitAmounts();
										p_bobj->FillTurnList(&pack);
										if(p_bobj->TurnPacket(&pack, 1)) {
											ok = 1;
										}
										else {
											// @todo @err
										}
									}
								}
								else {
									// @todo @err
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int TestMarketplace()
{
	class TestMarketplaceDialog : public TDialog {
	public:
		TestMarketplaceDialog() : TDialog(DLG_TESTMRKTPLC)
		{
		}
	};
	int    ok = -1;
	bool   do_test = false;
	PPID   gua_id = 0;
	SString param_buf;
	TestMarketplaceDialog * dlg = new TestMarketplaceDialog();
	if(CheckDialogPtr(&dlg)) {
		SetupPPObjCombo(dlg, CTLSEL_TESTMRKTPLC_GUA, PPOBJ_GLOBALUSERACC, gua_id, 0);
		dlg->setCtrlString(CTL_TESTMRKTPLC_TEXT, param_buf);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTLSEL_TESTMRKTPLC_GUA, &gua_id);
			dlg->getCtrlString(CTL_TESTMRKTPLC_TEXT, param_buf);
			do_test = true;
		}
	}
	if(do_test) {
		PPID   mp_psn_id = 0;
		PPMarketplaceInterface_Wildberries ifc;
		const int gmppr = ifc.GetMarketplacePerson(&mp_psn_id, 1);
		if(ifc.Init(gua_id)) {
			//TSCollection <PPMarketplaceInterface_Wildberries::Warehouse> wh_list;
			TSCollection <PPMarketplaceInterface_Wildberries::Stock> stock_list;
			//TSCollection <PPMarketplaceInterface_Wildberries::Sale> sale_list;
			//TSCollection <PPMarketplaceInterface_Wildberries::Sale> order_list;
			//TSCollection <PPMarketplaceInterface_Wildberries::Income> income_list;

			int r = 0;
			r = ifc.RequestWareList();
			//
			DateRange period;
			period.SetPredefined(PREDEFPRD_LASTMONTH, ZERODATE);
			r = ifc.ImportReceipts();
			r = ifc.ImportOrders();
			r = ifc.ImportSales();
			//r = ifc.RequestWarehouseList(wh_list);
			r = ifc.RequestAcceptanceReport(period);
			r = ifc.RequestSupplies();
			//r = ifc.RequestIncomes(income_list);
			r = ifc.RequestCommission();
			r = ifc.RequestStocks(stock_list);
			//r = ifc.RequestOrders(order_list);
			//r = ifc.RequestSales(sale_list);
			r = ifc.RequestGoodsPrices();
		}
	}
	delete dlg;
	return ok;
}
