// MARKETPLACE.CPP
// Copyright (c) A.Sobolev 2024
// @codepage UTF-8
// @construction 
//
#include <pp.h>
#pragma hdrstop

class PPMarketplaceInterface {
public:
	virtual ~PPMarketplaceInterface();
	virtual int Init(PPID guaID);
	//
	int    GetMarketplacePerson(PPID * pID, int use_ta);
	const  char * GetSymbol() const { return P_Symbol; }
protected:
	PPMarketplaceInterface(const char * pSymbol, PrcssrMarketplaceInterchange & rPrc);

	uint    State;
	PPObjPerson PsnObj;
	PPObjGoods GObj;
	PPObjArticle ArObj;
	PrcssrMarketplaceInterchange & R_Prc;
private:
	const  char * P_Symbol;
};

PPMarketplaceInterface::PPMarketplaceInterface(const char * pSymbol, PrcssrMarketplaceInterchange & rPrc) : 
	P_Symbol(pSymbol), State(0), R_Prc(rPrc)
{
}

/*virtual*/PPMarketplaceInterface::~PPMarketplaceInterface()
{
}
	
int PPMarketplaceInterface::GetMarketplacePerson(PPID * pID, int use_ta)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	const  PPID person_kind = PPPRK_MARKETPLACE;
	SString temp_buf;
	PPObjPersonKind pk_obj;
	PPPersonKind pk_rec;
	THROW(pk_obj.Fetch(person_kind, &pk_rec) > 0); // @todo @err (этот вид персоналий должен быть создан вызовом функции создания зарезервированных объектов)
	{
		PPID   acs_id = R_Prc.GetMarketplaceAccSheetID();
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
	// кВВ - коэффициент вознаграждения Вайлдберриз
public:
	struct Warehouse {
		Warehouse();
		Warehouse & Z();
		bool FromJsonObj(const SJson * pJs);

		enum {
			fAcceptsQR = 0x0001,
			fSelected  = 0x0002  // Признак того, что склад уже выбран продавцом
		};
		long   ID; // Идентификатор склада на маркетплейсе (не в нашей базе данных!)
		uint   Flags;
		int    CargoType; // Тип товара, который принимает склад:
			// 1 - обычный; 2 - СГТ (Сверхгабаритный товар); 3 - КГТ (Крупногабаритный товар). Не используется на данный момент.
		int    DeliveryType; // Тип доставки, который принимает склад:
			// 1 - доставка на склад Wildberries; 2 - доставка силами продавца; 3 - доставка курьером WB
		uint64 UedGeoLoc;
		SString Name;
		SString City;
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

	static SString & MakeSerialIdent(int64 incomeId, const WareBase & rWare, SString & rBuf)
	{
		return rBuf.Z().Cat(incomeId).CatChar('-').Cat(rWare.ID);
	}

	struct Income  {
		Income();
		Income & Z();
		bool FromJsonObj(const SJson * pJs);

		int64  IncomeID;
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
			// Проецируется на тег PPTAG_LOT_ORGORDERIDENT
	};
	
	struct SalesRepDbpEntry { // SalesReportDetailedByPeriod
		SalesRepDbpEntry();
		SalesRepDbpEntry & Z();
		bool FromJsonObj(const SJson * pJs);

		int64  RepId;                 // realizationreport_id Номер отчёта
		int    RepType;               // report_type integer Тип отчёта: 1 — стандартный, 2 — для уведомления о выкупе
		DateRange Period;             // date_from, date_to
		LDATE  CrDate;                // create_dt
		char   CurrencySymb[8];       // currency_name
		SString SupplierContractCode; // suppliercontract_code : object // Договор
		int64  RrdId;                 // rrd_id Номер строки
		int64  IncomeID;              // gi_id Номер поставки
		SString SrID;                 // srid string Уникальный идентификатор заказа. Примечание для использующих API Marketplace: srid равен rid в ответах методов сборочных заданий.
			// Проецируется на тег PPTAG_LOT_ORGORDERIDENT
		WareBase Ware;                // Наименования полей отличаются от таких же в других методах!
			/*
				subject_name string Предмет
				nm_id integer Артикул WB
				brand_name string Бренд
				sa_name string Артикул продавца
				ts_name string Размер
				barcode string Баркод
			*/
		SString DocTypeName;            // Тип документа
		SString Warehouse;              // office_name Склад
		SString SupplOpName;            // supplier_oper_name Обоснование для оплаты
			// Возмещение издержек по перевозке/по складским операциям с товаром
			// Логистика
			// Пересчет платной приемки
			// Продажа
			// Удержание
			// Хранение
		LDATETIME OrderDtm;             // order_dt <date-time> Дата заказа. Присылается с явным указанием часового пояса
		LDATETIME SaleDtm;              // sale_dt <date-time> Дата продажи. Присылается с явным указанием часового пояса
		LDATETIME RrDtm;                // rr_dt <date-time> Дата операции. Присылается с явным указанием часового пояса
		int64  ShkId;                   // shk_id Штрих-код. Это - Sticker из заказа и продажи. 
		SString BoxTypeName;            // gi_box_type_name Тип коробов
		SString SupplPromo;             // supplier_promo number
		int64  RId;                     // Уникальный идентификатор заказа 
		int    Ppvz_OfficeId;           // ppvz_office_id integer Номер офиса
		int    Ppvz_SupplierId;         // ppvz_supplier_id integer Номер партнера
		SString AcquiringBank;          // acquiring_bank string Наименование банка-эквайера
		SString Ppvz_OfficeName;        // ppvz_office_name string Наименование офиса доставки
		SString Ppvz_SupplierName;      // ppvz_supplier_name string Партнер
		SString Ppvz_Inn;               // ppvz_inn string ИНН партнера
		SString Clb;                    // declaration_number string Номер таможенной декларации
		SString BonusTypeName;          // bonus_type_name string Обоснование штрафов и доплат. Поле будет в ответе при наличии значения
		SString Sticker;                // sticker_id string Цифровое значение стикера, который клеится на товар в процессе сборки заказа по схеме "Маркетплейс"
		SString Country;                // site_country string Страна продажи
		SString RebillLogisticOrg;      // rebill_logistic_org string Организатор перевозки. Поле будет в ответе при наличии значения
		SString Kiz;                    // Марка чзн 
		int    DeliveryCount;           // delivery_amount Количество доставок
		int    ReturnCount;             // return_amount   Количество возвратов
		double Qtty;                    // Количество
		double RetailPrice;             // Цена розничная
		double RetailAmount;            // Сумма продаж (возвратов)
		double SalePct;                 // Согласованная скидка
		double CommissionPct;           // Процент комиссии
		double RetailPriceWithDiscount; // retail_price_withdisc_rub Цена розничная с учетом согласованной скидки
		double DeliveryAmount;          // delivery_rub    Стоимость логистики
		double ProductDiscount;         // Согласованный продуктовый дисконт
		double Ppvz_Spp_Prc;          // ppvz_spp_prc Скидка постоянного покупателя   
		double Ppvz_Kvw_Prc_Base;     // ppvz_kvw_prc_base number Размер кВВ без НДС, % базовый
		double Ppvz_Kvw_Prc;          // ppvz_kvw_prc number Итоговый кВВ без НДС, %
		double Sup_Rating_Prc_Up;     // sup_rating_prc_up number Размер снижения кВВ из-за рейтинга
		double IS_Kgvp_V2;            // is_kgvp_v2 number Размер снижения кВВ из-за акции
		double Ppvz_Sales_Commission; // ppvz_sales_commission number Вознаграждение с продаж до вычета услуг поверенного, без НДС
		double Ppvz_For_Pay;          // ppvz_for_pay number К перечислению продавцу за реализованный товар
		double Ppvz_Reward;           // ppvz_reward	number Возмещение за выдачу и возврат товаров на ПВЗ
		double AcquiringFee;          // acquiring_fee number Возмещение издержек по эквайрингу. Издержки WB за услуги эквайринга: вычитаются из вознаграждения WB и не влияют на доход продавца.
		double AcquiringPct;          // acquiring_percent number Размер комиссии за эквайринг без НДС, %
		double Ppvz_Vw;               // ppvz_vw number Вознаграждение WB без НДС
		double Ppvz_Vw_Vat;           // ppvz_vw_nds number НДС с вознаграждения WB
		double Penalty;               // penalty number Штрафы
		double AdditionalPayment;     // additional_payment number Доплаты
		double RebillLogisticCost;    // rebill_logistic_cost number Возмещение издержек по перевозке. Поле будет в ответе при наличии значения
		double StorageFee;            // storage_fee number Стоимость хранения
		double Deduction;             // deduction number Прочие удержания/выплаты
		double Acceptance;            // acceptance number Стоимость платной приёмки
	};

	PPMarketplaceInterface_Wildberries(PrcssrMarketplaceInterchange & rPrc) : PPMarketplaceInterface("WILDBERRIES", rPrc), Lth(PPFILNAM_MRKTPLCWBTALK_LOG)
	{
	}
	virtual ~PPMarketplaceInterface_Wildberries()
	{
	}
	virtual int Init(PPID guaID)
	{
		int    ok = PPMarketplaceInterface::Init(guaID);
		if(ok > 0) {
			FetchWarehouseList(WhList);
		}
		return ok;
	}
	//
	// Methods
	//
	int   RequestCommission();
	//
	// Замечание по поводу методов RequestWarehouseList и RequestWarehouseList2.
	// Первый использует метод WB https://supplies-api.wildberries.ru/api/v1/warehouses,
	// второй - https://marketplace-api.wildberries.ru/api/v3/offices.
	// Я не понимаю кто все это там делал, но оба метода возвращают разные количества складов, с
	// несовместимыми идентификаторами и разными наименованиями. При этом все методы WB предоставляют только
	// наименованования скадов и далеко не всегда наименование в виде ссылки может быть сопоставлено
	// с каким-либо складом, возвращенным хоть первым, хоть вторым методом.
	// 
	// Короче говоря, сейчас будем закладываться на первый вариант (methWarehouses aka https://supplies-api.wildberries.ru/api/v1/warehouses)
	// а дальше посмотрим.
	//
	int   RequestWarehouseList(TSCollection <Warehouse> & rList);
	int   RequestWarehouseList2(TSCollection <Warehouse> & rList);
	int   RequestGoodsPrices();
	int   RequestIncomes(TSCollection <Income> & rList);
	int   RequestStocks(TSCollection <Stock> & rList);
	int   RequestOrders(TSCollection <Sale> & rList);
	int   RequestSales(TSCollection <Sale> & rList);
	int   RequestSupplies();
	int   RequestReturns();
	int   RequestAcceptanceReport(const DateRange & rPeriod);
	int   RequestSalesReportDetailedByPeriod(const DateRange & rPeriod, TSCollection <SalesRepDbpEntry> & rList);
	int   RequestBalance();
	int   RequestDocumentsList();
	int   UploadWare();
	int   RequestWareList();

	int   CreateWarehouse(PPID * pID, int64 outerId, const char * pOuterName, const char * pAddress, int use_ta);
	const Warehouse * SearchWarehouseByName(const TSCollection <Warehouse> & rWhList, const char * pWhName, bool adoptive) const;
	int   ResolveWarehouseByName(const TSCollection <Warehouse> & rWhList, const char * pWhName, PPID defaultID, PPID * pResultID);
	int   ImportOrders();
	int   ImportReceipts();
	int   ImportSales();
	int   ImportFinancialTransactions();
private:
	int   GetLocalCachePath(SString & rBuf);
	int   FetchWarehouseList(TSCollection <Warehouse> & rList);
	int   ParseJson_WarehouseList(const SJson * pJs, TSCollection <Warehouse> & rList);
	int   Helper_RequestWarehouseList(int meth/*methWarehouses||methWarehouses2*/, TSCollection <Warehouse> & rList, const char * pFileNameToStoreJson);
	int   InsertReceiptItem(PPBillPacket & rPack, const char * pSerial, PPID goodsID, double qtty);
	//
	// Descr: 
	// ARG(surrogateAutoLot IN): Признак того, что создается суррогатный приход, не имеющий соответствия реальному документу на маркетплейсе,
	//   поскольку маркетплейс по той или иной причине не предоставил соответствующей информации.
	// Returns:
	//   0 - error
	//   >0 - ид лота
	//
	PPID  CreateReceipt(int64 incomeId, const WareBase & rWare, LDATE dt, PPID locID, PPID goodsID, double qtty, bool surrogateAutoLot, int use_ta);
	PPID  CreateBuyer(const Sale * pSaleEntry, int use_ta);
	PPID  CreateWare(const WareBase & rWare, int use_ta);
	int   FindShipmentBillByOrderIdent(const char * pOrgOrdIdent, PPIDArray & rShipmBillIdList);
	//
	// Descr: 
	// Returns:
	//   0 - error
	//   >0 - ид лота, к которому должна быть привязана продажа
	//
	PPID  AdjustReceiptOnExpend(const Sale & rWbItem, LDATE dt, PPID locID, PPID goodsID, double neededQtty, double nominalPrice, int use_ta);
	int   SearchOriginalLotForMp(const char * pSerial, LDATE dt, PPID locID, PPID goodsID, PPID * pResultLotID);
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
		apiDocuments,
	};
	enum {
		methCommission = 1,   // apiCommon
		methTariffBox,        // apiCommon 
		methTariffPallet,     // apiCommon
		methTariffReturn,     // apiCommon
		methWarehouses,       // apiSupplies
		methWarehouses2,      // apiMarketplace https://marketplace-api.wildberries.ru/api/v3/offices
		methIncomes,          // apiStatistics
		methStocks,           // apiStatistics
		methOrders,           // apiStatistics
		methSales,            // apiStatistics
		methSalesReportDetailedByPeriod, // apiStatistics https://statistics-api.wildberries.ru/api/v5/supplier/reportDetailByPeriod
		methSupples,          // apiMarketplace https://marketplace-api.wildberries.ru/api/v3/supplies Получить список поставок
		methSupply,           // apiMarketplace https://marketplace-api.wildberries.ru/api/v3/supplies/{supplyId} Получить поставку
		methSupplyOrders,     // apiMarketplace https://marketplace-api.wildberries.ru/api/v3/supplies/{supplyId}/orders Получить сборочные задания в поставке
		methAcceptanceReport, // apiAnalytics   https://seller-analytics-api.wildberries.ru/api/v1/analytics/acceptance-report
		methGoodsPrices,      // apiDiscountsPrices https://discounts-prices-api.wildberries.ru/api/v2/list/goods/filter
		methContentCardsList, // apiContent https://content-api.wildberries.ru/content/v2/get/cards/list
		methBalance,          // apiAdvert https://advert-api.wildberries.ru/adv/v1/balance
		methDocumentsList,    // apiDocuments https://documents-api.wildberries.ru/api/v1/documents/list 
		methReturns,          // apiAnalytics   https://seller-analytics-api.wildberries.ru/api/v1/analytics/goods-return
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
			{ apiAnalytics, "seller-analytics-api" },
			{ apiDocuments, "documents-api" },
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
			{ methWarehouses2, apiMarketplace, SHttpProtocol::reqGet, "api/v3/offices" },
			{ methIncomes, apiStatistics, SHttpProtocol::reqGet, "api/v1/supplier/incomes" },
			{ methStocks, apiStatistics, SHttpProtocol::reqGet, "api/v1/supplier/stocks" },
			{ methOrders, apiStatistics, SHttpProtocol::reqGet, "api/v1/supplier/orders" },
			{ methSales, apiStatistics, SHttpProtocol::reqGet, "api/v1/supplier/sales" },
			{ methSalesReportDetailedByPeriod, apiStatistics,  SHttpProtocol::reqGet, "api/v5/supplier/reportDetailByPeriod" },
			{ methSupples, apiMarketplace, SHttpProtocol::reqGet, "api/v3/supplies" },
			{ methSupply, apiMarketplace, SHttpProtocol::reqGet, "api/v3/supplies" }, // /supplies/{supplyId}
			{ methSupplyOrders, apiMarketplace, SHttpProtocol::reqGet, "api/v3/supplies" }, // /{supplyId}/orders
			{ methAcceptanceReport, apiAnalytics, SHttpProtocol::reqGet, "api/v1/analytics/acceptance-report" },
			{ methGoodsPrices, apiDiscountsPrices, SHttpProtocol::reqGet, "api/v2/list/goods/filter" },
			{ methContentCardsList, apiContent, SHttpProtocol::reqPost, "content/v2/get/cards/list" },
			{ methBalance, apiAdvert, SHttpProtocol::reqGet, "adv/v1/balance" },
			{ methDocumentsList, apiDocuments, SHttpProtocol::reqGet, "api/v1/documents/list" },
			{ methReturns, apiAnalytics, SHttpProtocol::reqGet, "api/v1/analytics/goods-return" },
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
		if(R_Prc.GetGuaPack().Rec.ServiceIdent == PPGLS_WILDBERRIES) {
			SString token;
			if(R_Prc.GetGuaPack().GetAccessKey(token) > 0) {
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
	PPIDArray MpLocList; // Список идентификаторов складов маркетплейса, применяемых при обработке документов
	LAssocArray MpLotToOwnLotAssocList; // Специализированный кэш, хранящий ассоциации между лотами на складах маркетплейса и лотами,
		// из которых они образовались на собственных складах. Фактически, этот массив кэширует результат работы функции SearchOriginalLotForMp
};

int PPMarketplaceInterface::Init(PPID guaID)
{
	int    ok = 1;
	/*
	PPObjGlobalUserAcc gua_obj;
	if(gua_obj.GetPacket(guaID, &GuaPack) > 0) {
		; // ok
	}
	else
		ok = 0;
	*/
	return ok;
}
//
//
//
PPMarketplaceInterface_Wildberries::Warehouse::Warehouse() : ID(0), Flags(0), UedGeoLoc(0ULL), CargoType(0), DeliveryType(0)
{
}
		
PPMarketplaceInterface_Wildberries::Warehouse & PPMarketplaceInterface_Wildberries::Warehouse::Z()
{
	ID = 0;
	Flags = 0;
	UedGeoLoc = 0ULL;
	CargoType = 0;
	DeliveryType = 0;
	Name.Z();
	City.Z();
	Address.Z();
	return *this;
}

bool PPMarketplaceInterface_Wildberries::Warehouse::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		SGeoPosLL geoloc;
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("ID")) {
				ID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("name")) {
				Name = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("city")) {
				City = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("address")) {
				Address = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("longitude")) {
				geoloc.Lon = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("latitude")) {
				geoloc.Lat = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("cargoType")) {
				CargoType = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("deliveryType")) {
				DeliveryType = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("workTime")) {
				;
			}
			else if(p_cur->Text.IsEqiAscii("acceptsQR")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1)
					Flags |= fAcceptsQR;
			}
			else if(p_cur->Text.IsEqiAscii("selected")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1)
					Flags |= fSelected;
			}
		}
		UedGeoLoc = UED::SetRaw_GeoLoc(geoloc);
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
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("supplierArticle"))
				SupplArticle = p_cur->P_Child->Text;
			else if(p_cur->Text.IsEqiAscii("nmId"))
				ID = p_cur->P_Child->Text.ToInt64();
			else if(p_cur->Text.IsEqiAscii("barcode"))
				Barcode = p_cur->P_Child->Text;
			else if(p_cur->Text.IsEqiAscii("category"))
				Category = p_cur->P_Child->Text;
			else if(p_cur->Text.IsEqiAscii("subject"))
				Name = p_cur->P_Child->Text;
			else if(p_cur->Text.IsEqiAscii("brand"))
				Brand = p_cur->P_Child->Text;
			else if(p_cur->Text.IsEqiAscii("techSize"))
				TechSize = p_cur->P_Child->Text;
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
			else if(p_cur->Text.IsEqiAscii("warehouseName"))
				WarehouseName = p_cur->P_Child->Text;
			else if(p_cur->Text.IsEqiAscii("quantity"))
				Qtty = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("inWayToClient"))
				InWayToClient = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("inWayFromClient"))
				InWayFromClient = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("quantityFull"))
				QttyFull = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("brand")) {
				;
			}
			else if(p_cur->Text.IsEqiAscii("Price"))
				Price = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("Discount"))
				Discount = p_cur->P_Child->Text.ToReal_Plain();
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

PPMarketplaceInterface_Wildberries::SalesRepDbpEntry::SalesRepDbpEntry() : RepId(0), RepType(0), CrDate(ZERODATE), RrdId(0), IncomeID(0), OrderDtm(ZERODATETIME), SaleDtm(ZERODATETIME), RrDtm(ZERODATETIME),
	ShkId(0), RId(0), Ppvz_OfficeId(0), Ppvz_SupplierId(0), DeliveryCount(0), ReturnCount(0),
	Qtty(0.0), RetailPrice(0.0), RetailAmount(0.0), SalePct(0.0), CommissionPct(0.0), RetailPriceWithDiscount(0.0), DeliveryAmount(0.0),
	ProductDiscount(0.0), Ppvz_Spp_Prc(0.0), Ppvz_Kvw_Prc_Base(0.0), Ppvz_Kvw_Prc(0.0), Sup_Rating_Prc_Up(0.0), IS_Kgvp_V2(0.0), Ppvz_Sales_Commission(0.0),
	Ppvz_For_Pay(0.0), Ppvz_Reward(0.0), AcquiringFee(0.0), AcquiringPct(0.0), Ppvz_Vw(0.0), Ppvz_Vw_Vat(0.0), Penalty(0.0), AdditionalPayment(0.0), RebillLogisticCost(0.0),
	StorageFee(0.0), Deduction(0.0), Acceptance(0.0)
{
	Period.Z();
	CurrencySymb[0] = 0;
}
		
PPMarketplaceInterface_Wildberries::SalesRepDbpEntry & PPMarketplaceInterface_Wildberries::SalesRepDbpEntry::Z()
{
	Period.Z();
	CurrencySymb[0] = 0;
	RepId = 0;
	RepType = 0;
	CrDate.Z();
	RrdId = 0;
	IncomeID = 0;
	OrderDtm.Z();
	SaleDtm.Z();
	RrDtm.Z();
	ShkId = 0;
	RId = 0;
	Ppvz_OfficeId = 0;
	Ppvz_SupplierId = 0;
	DeliveryCount = 0;
	ReturnCount = 0;
	Qtty = 0.0;
	RetailPrice = 0.0;
	RetailAmount = 0.0;
	SalePct = 0.0;
	CommissionPct = 0.0;
	RetailPriceWithDiscount = 0.0;
	DeliveryAmount = 0.0;
	ProductDiscount = 0.0;
	Ppvz_Spp_Prc = 0.0;
	Ppvz_Kvw_Prc_Base = 0.0;
	Ppvz_Kvw_Prc = 0.0;
	Sup_Rating_Prc_Up = 0.0;
	IS_Kgvp_V2 = 0.0;
	Ppvz_Sales_Commission = 0.0;
	Ppvz_For_Pay = 0.0;
	Ppvz_Reward = 0.0;
	AcquiringFee = 0.0;
	AcquiringPct = 0.0;
	Ppvz_Vw = 0.0;
	Ppvz_Vw_Vat = 0.0;
	Penalty = 0.0;
	AdditionalPayment = 0.0;
	RebillLogisticCost = 0.0;
	StorageFee = 0.0;
	Deduction = 0.0;
	Acceptance = 0.0;
	SupplierContractCode.Z();
	SrID.Z();
	Ware.Z();
	DocTypeName.Z();
	Warehouse.Z();
	SupplOpName.Z();
	BoxTypeName.Z();
	SupplPromo.Z();
	AcquiringBank.Z();
	Ppvz_OfficeName.Z();
	Ppvz_SupplierName.Z();
	Ppvz_Inn.Z();
	Clb.Z();
	BonusTypeName.Z();
	Sticker.Z();
	Country.Z();
	RebillLogisticOrg.Z();
	Kiz.Z();
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::SalesRepDbpEntry::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("realizationreport_id")) {
				RepId = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("date_from")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					Period.low = dtm.d;
				}
			}
			else if(p_cur->Text.IsEqiAscii("date_to")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					Period.upp = dtm.d;
				}
			}
			else if(p_cur->Text.IsEqiAscii("create_dt")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					CrDate = dtm.d;
				}
			}
			else if(p_cur->Text.IsEqiAscii("currency_name")) {
				STRNSCPY(CurrencySymb, p_cur->P_Child->Text);
			}
			else if(p_cur->Text.IsEqiAscii("suppliercontract_code")) {
				SupplierContractCode = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("rrd_id")) {
				RrdId = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("gi_id")) {
				IncomeID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("subject_name")) {
				Ware.Name = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("nm_id")) {
				Ware.ID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("brand_name")) {
				Ware.Brand = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("sa_name")) {
				Ware.SupplArticle = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("ts_name")) {
				Ware.TechSize = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("barcode")) {
				Ware.Barcode = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("doc_type_name")) {
				DocTypeName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("quantity")) {
				Qtty = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("retail_price")) {
				RetailPrice = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("retail_amount")) {
				RetailAmount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("sale_percent")) {
				SalePct = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("commission_percent")) {
				CommissionPct = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("office_name")) {
				Warehouse = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("supplier_oper_name")) {
				SupplOpName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("order_dt")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					OrderDtm = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("sale_dt")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					SaleDtm = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("rr_dt")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					RrDtm = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("shk_id")) {
				ShkId = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("retail_price_withdisc_rub")) {
				RetailPriceWithDiscount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("delivery_amount")) {
				DeliveryCount = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("return_amount")) {
				ReturnCount = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("delivery_rub")) {
				DeliveryAmount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("gi_box_type_name")) {
				BoxTypeName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("product_discount_for_report")) {
				ProductDiscount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("supplier_promo")) {
				SupplPromo = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("rid")) {
				RId = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_spp_prc")) {
				Ppvz_Spp_Prc = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_kvw_prc_base")) {
				Ppvz_Kvw_Prc_Base = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_kvw_prc")) {
				Ppvz_Kvw_Prc = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("sup_rating_prc_up")) {
				Sup_Rating_Prc_Up = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("is_kgvp_v2")) {
				IS_Kgvp_V2 = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_sales_commission")) {
				Ppvz_Sales_Commission = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_for_pay")) {
				Ppvz_For_Pay = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_reward")) {
				Ppvz_Reward = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("acquiring_fee")) {
				AcquiringFee = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("acquiring_percent")) {
				AcquiringPct = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("acquiring_bank")) {
				AcquiringBank = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_vw")) {
				Ppvz_Vw = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_vw_nds")) {
				Ppvz_Vw_Vat = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_office_id")) {
				Ppvz_OfficeId = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_office_name")) {
				Ppvz_OfficeName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_supplier_id")) {
				Ppvz_SupplierId = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_supplier_name")) {
				Ppvz_SupplierName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_inn")) {
				Ppvz_Inn = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("declaration_number")) {
				Clb = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("bonus_type_name")) {
				BonusTypeName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("sticker_id")) {
				Sticker = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("site_country")) {
				Country = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("penalty")) {
				Penalty = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("additional_payment")) {
				AdditionalPayment = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("rebill_logistic_cost")) {
				RebillLogisticCost = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("rebill_logistic_org")) {
				RebillLogisticOrg = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("kiz")) {
				Kiz = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("storage_fee")) {
				StorageFee = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("deduction")) {
				Deduction = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("acceptance")) {
				Acceptance = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("srid")) {
				SrID = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("report_type")) {
				RepType = p_cur->P_Child->Text.ToLong();
			}
		}
		ok = true;
	}
	return ok;
}

int PPMarketplaceInterface_Wildberries::GetLocalCachePath(SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	SString cache_path;
	PPGetPath(PPPATH_WORKSPACE, cache_path);	
	cache_path.SetLastSlash().Cat("cache").SetLastSlash().Cat("data").SetLastSlash().Cat("wildberries");
	if(SFile::CreateDir(cache_path))
		rBuf = cache_path;
	else
		ok = 0;
	return ok;
}

int PPMarketplaceInterface_Wildberries::ParseJson_WarehouseList(const SJson * pJs, TSCollection <Warehouse> & rList)
{
	rList.freeAll();
	int    ok = 0;
	if(SJson::IsArray(pJs)) {
		for(const SJson * p_js_item = pJs->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
			if(SJson::IsObject(p_js_item)) {
				uint   new_item_pos = 0;
				Warehouse * p_new_item = rList.CreateNewItem(&new_item_pos);
				if(p_new_item) {
					if(p_new_item->FromJsonObj(p_js_item)) {
						ok = 1;
					}
					else {
						rList.atFree(new_item_pos);
					}
				}
			}
		}
	}
	return ok;
}

int PPMarketplaceInterface_Wildberries::FetchWarehouseList(TSCollection <Warehouse> & rList)
{
	int    ok = 1;
	bool   do_request = true;
	const  LDATETIME now_dtm = getcurdatetime_();
	SString local_cache_path;
	if(GetLocalCachePath(local_cache_path)) {
		local_cache_path.SetLastSlash().Cat("warehouselist.json");
		SFile::Stat st;
		if(SFile::GetStat(local_cache_path, 0, &st, 0)) {
			LDATETIME file_dtm;
			file_dtm.SetNs100(st.ModTm_);
			long s = diffdatetimesec(now_dtm, file_dtm);
			if(s <= (3 * 24 * 3600)) {
				SJson * p_js = SJson::ParseFile(local_cache_path);
				if(ParseJson_WarehouseList(p_js, rList)) {
					do_request = false;
				}
				delete p_js;
			}
		}
	}
	if(do_request) {
		ok = Helper_RequestWarehouseList(methWarehouses, rList, local_cache_path);
	}
	return ok;
}

int PPMarketplaceInterface_Wildberries::Helper_RequestWarehouseList(int meth/*methWarehouses||methWarehouses2*/, TSCollection <Warehouse> & rList, const char * pFileNameToStoreJson)
{
	rList.freeAll();
	int    ok = 0;
	SJson * p_js_reply = 0;
	assert(oneof2(meth, methWarehouses, methWarehouses2));
	if(oneof2(meth, methWarehouses, methWarehouses2)) {
		SString temp_buf;
		SString url_buf;
		StrStrAssocArray hdr_flds;
		THROW(Helper_InitRequest(meth, url_buf, hdr_flds));
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
						p_js_reply = SJson::Parse(reply_buf);
						if(ParseJson_WarehouseList(p_js_reply, rList)) {
							if(!isempty(pFileNameToStoreJson)) {
								SFile f_cache(pFileNameToStoreJson, SFile::mWrite);
								if(f_cache.IsValid()) {
									f_cache.Write(reply_buf.cptr(), reply_buf.Len());
								}
							}
							ok = 1;
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestWarehouseList(TSCollection <Warehouse> & rList)
{
	return Helper_RequestWarehouseList(methWarehouses, rList, 0);
}

int PPMarketplaceInterface_Wildberries::RequestWarehouseList2(TSCollection <Warehouse> & rList)
{
	return Helper_RequestWarehouseList(methWarehouses2, rList, 0);
}

int PPMarketplaceInterface_Wildberries::RequestWareList()
{
	int    ok = 1;
	SJson * p_js_reply = 0;
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
					p_js_reply = SJson::Parse(reply_buf);
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
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestGoodsPrices()
{
	//rList.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
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
					p_js_reply = SJson::Parse(reply_buf);
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
	delete p_js_reply;
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
	SJson * p_js_reply = 0;
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
					p_js_reply = SJson::Parse(reply_buf);
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
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestStocks(TSCollection <Stock> & rList)
{
	rList.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
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
					p_js_reply = SJson::Parse(reply_buf);
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
	delete p_js_reply;
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

int PPMarketplaceInterface_Wildberries::RequestReturns()
{
	int    ok = 1;
	const  LDATETIME now_dtm = getcurdatetime_();
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	DateRange period;
	period.upp = plusdate(now_dtm.d, -30);
	period.low = plusdate(now_dtm.d, -60);
	THROW(Helper_InitRequest(methReturns, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		temp_buf.Z().CatEq("dateFrom", period.low, DATF_ISO8601CENT).CatChar('&').CatEq("dateTo", period.upp, DATF_ISO8601CENT);
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
	SJson * p_js_reply = 0;
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
					p_js_reply = SJson::Parse(reply_buf);
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
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestSales(TSCollection <Sale> & rList)
{
	rList.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
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
		url_buf.CatChar('?').CatEq("dateFrom", encodedate(1, 1, 2024), DATF_ISO8601CENT).CatChar('&').CatEq("flag", 0);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
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
					if(rList.getCount()) {
						PPGetFilePath(PPPATH_OUT, "mpwb-sales_order_type.txt", temp_buf);
						SFile f_out(temp_buf, SFile::mWrite);
						for(uint i = 0; i < rList.getCount(); i++) {
							const Sale * p_item = rList.at(i);
							if(p_item) {
								if(p_item->OrderType.NotEmpty()) {
									f_out.WriteLine((temp_buf = p_item->OrderType).CR());
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestDocumentsList()
{
	//rList.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methDocumentsList, url_buf, hdr_flds));
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
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								/*
								uint   new_item_pos = 0;
								Sale * p_new_item = rList.CreateNewItem(&new_item_pos);
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
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestBalance()
{
	//rList.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methBalance, url_buf, hdr_flds));
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
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								/*
								uint   new_item_pos = 0;
								Sale * p_new_item = rList.CreateNewItem(&new_item_pos);
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
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestSalesReportDetailedByPeriod(const DateRange & rPeriod, TSCollection <SalesRepDbpEntry> & rList)
{
	rList.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	/*
		Query parameters:

		dateFrom required string <RFC3339>
			Начальная дата отчета.
			Дата в формате RFC3339. Можно передать дату или дату со временем. Время можно указывать с точностью до секунд или миллисекунд.
			Время передаётся в часовом поясе Мск (UTC+3).
			Примеры:

				2019-06-20
				2019-06-20T23:59:59
				2019-06-20T00:00:00.12345
				2017-03-25T00:00:00 

		limit integer Default: 100000
			Максимальное количество строк отчета, возвращаемых методом. Не может быть более 100000.

		dateTo required string <date>
			Конечная дата отчета

		rrdid integer
			Уникальный идентификатор строки отчета. Необходим для получения отчета частями.
			Загрузку отчета нужно начинать с rrdid = 0 и при последующих вызовах API передавать в запросе значение rrd_id из последней строки, полученной в результате предыдущего вызова.
			Таким образом для загрузки одного отчета может понадобиться вызывать API до тех пор, пока количество возвращаемых строк не станет равным нулю.
	*/
	THROW(Helper_InitRequest(methSalesReportDetailedByPeriod, url_buf, hdr_flds));
	{
		DateRange period(rPeriod);
		period.Actualize(ZERODATE);
		if(!checkdate(period.low)) {
			if(checkdate(period.upp))
				period.low = period.upp;
			else
				period.SetPredefined(PREDEFPRD_LASTMONTH, ZERODATE);
		}
		else if(!checkdate(period.upp))
			period.upp = period.low;

		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		int   rep_limit = 10000;
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("dateFrom", period.low, DATF_ISO8601CENT).CatChar('&').CatEq("dateTo", period.upp, DATF_ISO8601CENT).
			CatChar('&').CatEq("limit", rep_limit);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								uint   new_item_pos = 0;
								SalesRepDbpEntry * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
							}
						}
						if(rList.getCount()) {
							PPGetFilePath(PPPATH_OUT, "mpwb-supplier_oper_name.txt", temp_buf);
							SFile f_out(temp_buf, SFile::mWrite);
							for(uint i = 0; i < rList.getCount(); i++) {
								const SalesRepDbpEntry * p_item = rList.at(i);
								if(p_item) {
									if(p_item->SupplOpName.NotEmpty()) {
										f_out.WriteLine((temp_buf = p_item->SupplOpName).CR());
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
	delete p_js_reply;
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
				R_Prc.GetLogger().LogAcceptMsg(PPOBJ_LOCATION, result_id, 0);
				ok = 2;
			}
		}
		THROW(tra.Commit());
	}
	ASSIGN_PTR(pID, result_id);
	CATCHZOK
	return ok;
}

const PPMarketplaceInterface_Wildberries::Warehouse * PPMarketplaceInterface_Wildberries::SearchWarehouseByName(const TSCollection <Warehouse> & rWhList, const char * pWhName, bool adoptive) const
{
	const Warehouse * p_result = 0;
	SString key_buf(pWhName);
	key_buf.Strip();
	{
		for(uint i = 0; !p_result && i < rWhList.getCount(); i++) {
			const Warehouse * p_iter = rWhList.at(i);
			if(p_iter && p_iter->Name.IsEqiUtf8(key_buf))
				p_result = p_iter;
		}
	}
	if(!p_result && adoptive) {
		SString temp_buf;
		key_buf.Utf8ToLower().ReplaceStr("(", " ", 0).ReplaceStr(")", " ", 0).ReplaceStr("  ", " ", 0).Strip();
		{
			LongArray eqpfx_list;
			for(uint i = 0; !p_result && i < rWhList.getCount(); i++) {
				const Warehouse * p_iter = rWhList.at(i);
				if(p_iter) {
					(temp_buf = p_iter->Name).Utf8ToLower().ReplaceStr("(", " ", 0).ReplaceStr(")", " ", 0).ReplaceStr("  ", " ", 0).Strip();
					if(temp_buf.IsEq(key_buf))
						p_result = p_iter;
					else if(temp_buf.HasPrefix(key_buf)) {
						eqpfx_list.add(i);
					}
				}
			}
			if(!p_result) {
				if(eqpfx_list.getCount() == 1) {
					p_result = rWhList.at(eqpfx_list.get(0));
				}
			}
		}
	}
	return p_result;
}

int PPMarketplaceInterface_Wildberries::ResolveWarehouseByName(const TSCollection <Warehouse> & rWhList, const char * pWhName, PPID defaultID, PPID * pResultID)
{
	int    ok = -1;
	PPID   wh_id = 0;
	if(!isempty(pWhName)) {
		const Warehouse * p_wh = SearchWarehouseByName(WhList, pWhName, true);
		if(p_wh) {
			if(CreateWarehouse(&wh_id, p_wh->ID, p_wh->Name, p_wh->Address, 1) > 0) {
				assert(wh_id > 0);
				MpLocList.addUnique(wh_id);
				ok = 1;
			}
		}
	}
	if(!wh_id) {
		if(defaultID) {
			wh_id = defaultID;
			ok = 2;
		}
	}
	ASSIGN_PTR(pResultID, wh_id);
	return ok;
}

PPID PPMarketplaceInterface_Wildberries::CreateWare(const WareBase & rWare, int use_ta) 
{
	PPID   result_id = 0;
	SString temp_buf;
	BarcodeTbl::Rec bc_rec;
	Goods2Tbl::Rec goods_rec;
	if(GObj.SearchByBarcode(rWare.Barcode, &bc_rec, &goods_rec, 0) > 0) {
		assert(goods_rec.ID > 0 && bc_rec.GoodsID > 0 && goods_rec.ID == bc_rec.GoodsID); // @paranoic
		result_id = goods_rec.ID;
	}
	else {
		SString name_buf;
		if(rWare.Name.NotEmpty())
			name_buf = rWare.Name;
		if(rWare.SupplArticle.NotEmpty()) {
			name_buf.CatDivIfNotEmpty('-', 1).Cat(rWare.SupplArticle);
		}
		if(name_buf.IsEmpty())
			name_buf = rWare.Barcode;
		if(name_buf.NotEmpty()) {
			const PPGoodsConfig & r_cfg = GObj.GetConfig();
			const PPID acs_id = R_Prc.GetMarketplaceAccSheetID();
			SString ar_code;
			PPID   mp_ar_id = 0;
			PPID   temp_id = 0;
			if(rWare.ID)
				ar_code.Cat(rWare.ID);
			if(name_buf.IsLegalUtf8()) {
				name_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
			if(rWare.Barcode.IsEmpty() && GObj.SearchByName(name_buf, &temp_id, &goods_rec) > 0) {
				result_id = goods_rec.ID;
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
								}
							}
						}
					}
					if(!result_id) {
						PPGoodsPacket goods_pack;
						assert(result_id == 0);
						goods_pack.Rec.Kind = PPGDSK_GOODS;
						STRNSCPY(goods_pack.Rec.Name, name_buf);
						STRNSCPY(goods_pack.Rec.Abbr, name_buf);
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
						R_Prc.GetLogger().LogAcceptMsg(PPOBJ_GOODS, result_id, 0);
					}
					THROW(tra.Commit());
				}
			}
		}
	}
	CATCH
		result_id = 0;
	ENDCATCH
	return result_id;
}

PPID PPMarketplaceInterface_Wildberries::CreateBuyer(const Sale * pSaleEntry, int use_ta)
{
	PPID  result_ar_id = 0;
	if(pSaleEntry) {
		const  PPID sale_op_id = R_Prc.GetSaleOpID();
		if(sale_op_id) {
			PPOprKind op_rec;
			if(GetOpData(sale_op_id, &op_rec) > 0) {
				const PPID acs_id = op_rec.AccSheetID;
				if(acs_id) {
					PPObjAccSheet acs_obj;
   					PPAccSheet acs_rec;
					PPID   ar_id = 0;
					PPID   psn_id = 0;
					if(acs_obj.Fetch(acs_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON) {
						const PPID  psn_kind_id = acs_rec.ObjGroup;
						if(psn_kind_id) {
							SString name;
							PPID   status_id = PPPRS_REGION;
							if(pSaleEntry->CountryName.NotEmpty()) {
								name.Cat(pSaleEntry->CountryName);
								status_id = PPPRS_COUNTRY;
							}
							if(pSaleEntry->RegionName.NotEmpty()) {
								name.CatDivIfNotEmpty('-', 1).Cat(pSaleEntry->RegionName);
								status_id = PPPRS_REGION;
							}
							if(name.NotEmpty()) {
								if(name.IsLegalUtf8()) {
									name.Transf(CTRANSF_UTF8_TO_INNER);
								}
								PPObjPerson::ResolverParam rslvp;
								rslvp.Flags = rslvp.fCreateIfNFound;
								rslvp.KindID = psn_kind_id;
								rslvp.StatusID = status_id;
								rslvp.CommonName = name;
								PPIDArray psn_candidate_list;
								PsnObj.Resolve(rslvp, psn_candidate_list, use_ta);
								if(psn_candidate_list.getCount()) {
									PPID psn_id = psn_candidate_list.get(0);
									PPID ar_id = 0;
									if(ArObj.P_Tbl->PersonToArticle(psn_id, acs_id, &ar_id)) {
										result_ar_id = ar_id;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return result_ar_id;
}

int PPMarketplaceInterface_Wildberries::InsertReceiptItem(PPBillPacket & rPack, const char * pSerial, PPID goodsID, double qtty)
{
	int    ok = 1;
	PPTransferItem ti;
	LongArray row_idx_list;
	double cost = 0.0;
	double price = 0.0;
	if(!isempty(pSerial)) {
		ReceiptTbl::Rec own_lot_rec;
		PPID   own_lot_id = 0;
		if(SearchOriginalLotForMp(pSerial, rPack.Rec.Dt, rPack.Rec.LocID, goodsID, &own_lot_id) > 0) {
			if(BillObj->trfr->Rcpt.Search(own_lot_id, &own_lot_rec) > 0) {
				cost = own_lot_rec.Cost;
				price = own_lot_rec.Price;
			}							
		}
	}
	ti.GoodsID = goodsID;
	ti.Cost = cost;
	ti.Price = price;
	ti.Quantity_ = fabs(qtty);
	THROW(rPack.InsertRow(&ti, &row_idx_list));
	assert(row_idx_list.getCount() == 1);
	if(!isempty(pSerial)) {
		const long new_row_idx = row_idx_list.get(0);
		rPack.LTagL.SetString(PPTAG_LOT_SN, new_row_idx, pSerial);
	}
	CATCHZOK
	return ok;
}

PPID PPMarketplaceInterface_Wildberries::CreateReceipt(int64 incomeId, const WareBase & rWare, LDATE dt, PPID locID, PPID goodsID, double qtty, bool surrogateAutoLot, int use_ta)
{
	PPID   result_lot_id = 0;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	const  PPID rcpt_op_id = CConfig.ReceiptOp;
	PPID   suppl_id = 0;
	PPID   own_lot_id = 0;
	double adj_cost = 0.0;
	double adj_price = 0.0;
	SString bill_code;
	SString serial_buf;
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
			THROW(InsertReceiptItem(pack, MakeSerialIdent(incomeId, rWare, serial_buf), goodsID, qtty));
			if(surrogateAutoLot)
				PPGetWord(PPWORD_AT_AUTO, 0, pack.SMemo);
			pack.InitAmounts();
			p_bobj->FillTurnList(&pack);
			if(!surrogateAutoLot)
				pack.SetupEdiAttributes(PPEDIOP_MRKTPLC_RECEIPT, "MP-WILDBERRIES", 0);
			THROW(p_bobj->TurnPacket(&pack, 0));
			assert(pack.GetTCount() == 1);
			THROW(pack.GetTCount() == 1);
			result_lot_id = pack.ConstTI(0).LotID;
			if(own_lot_id)
				MpLotToOwnLotAssocList.Add(result_lot_id, own_lot_id);
			R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
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
	PPID   suppl_id = 0;
	SString bill_code;
	TSCollection <PPMarketplaceInterface_Wildberries::Income> income_list;
	const  PPID rcpt_op_id = CConfig.ReceiptOp;
	THROW_PP(rcpt_op_id, PPERR_UNDEFRECEIPTOP);
	THROW(ArObj.GetMainOrgAsSuppl(&suppl_id, 1/*processAbsense*/, 1/*use_ta*/));
	RequestIncomes(income_list);
	if(income_list.getCount()) {
		for(uint i = 0; i < income_list.getCount(); i++) {
			const Income * p_wb_item = income_list.at(i);
			if(p_wb_item) {
				PPID   wh_id = 0;
				BillTbl::Rec ex_bill_rec;
				ResolveWarehouseByName(WhList, p_wb_item->WarehouseName, LConfig.Location, &wh_id);
				const  PPID  goods_id = CreateWare(p_wb_item->Ware, 1/*use_ta*/);
				if(goods_id) {
					bill_code.Z().Cat(p_wb_item->IncomeID);
					if(p_bobj->P_Tbl->SearchByCode(bill_code, rcpt_op_id, ZERODATE, &ex_bill_rec) > 0) {
						// Поступление на склад '%s' уже акцептировано ранее
						PPBillPacket ex_pack;
						if(p_bobj->ExtractPacket(ex_bill_rec.ID, &ex_pack) > 0) {
							uint   goods_idx = 0;
							if(ex_pack.SearchGoods(goods_id, &goods_idx)) {
								;
							}
							else {
								if(InsertReceiptItem(ex_pack, MakeSerialIdent(p_wb_item->IncomeID, p_wb_item->Ware, temp_buf), goods_id, fabs(p_wb_item->Qtty))) {
									ex_pack.InitAmounts();
									p_bobj->FillTurnList(&ex_pack);
									const int upr = p_bobj->UpdatePacket(&ex_pack, 1);
									if(upr > 0) {
										R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, ex_pack.Rec.ID, 1/*upd*/);
										ok = 1;
									}
									else if(!upr) {
										R_Prc.GetLogger().LogLastError();
									}
								}
							}
						}
					}
					else {
						PPBillPacket pack;
						PPID   ex_bill_id = 0;
						Goods2Tbl::Rec goods_rec;
						PPBillPacket::SetupObjectBlock sob;
						const LDATE dt = checkdate(p_wb_item->Dtm.d) ? p_wb_item->Dtm.d : getcurdate_();
						const PPID lot_id = CreateReceipt(p_wb_item->IncomeID, p_wb_item->Ware, dt, wh_id, goods_id, fabs(p_wb_item->Qtty), false, 1);
						if(lot_id) {
							ok = 1;
						}
					}
				}
				else {
					// @todo @err
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::SearchOriginalLotForMp(const char * pSerial, LDATE dt, PPID locID, PPID goodsID, PPID * pResultLotID)
{
	int    ok = -1;
	PPID   target_lot_id = 0;
	Reference * p_ref = PPRef;
	SString temp_buf;
	if(goodsID) {
		PPID   transf_op_id = R_Prc.GetConfig().TransferToMpOpID;
		if(transf_op_id) {
			PPObjBill * p_bobj = BillObj;
			BillCore * p_billc = p_bobj->P_Tbl;
			Transfer * p_trfr = p_bobj->trfr;
			LDATE recent_date = ZERODATE;
			BillTbl::Rec bill_rec;
			BillTbl::Key2 k2;
			k2.OpID = transf_op_id;
			k2.Dt   = dt;
			k2.BillNo = MAXLONG;
			const long max_days_for_survey = 30;
			PPID   last_suitable_lot_id_without_serial = 0;
			PPIDArray candidate_lot_list_by_serial;
			SString serial_buf(pSerial);
			if(serial_buf.NotEmpty()) {
				p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_LOT, PPTAG_LOT_SN, serial_buf, &candidate_lot_list_by_serial);
			}
			if(p_billc->search(2, &k2, spLe) && k2.OpID == transf_op_id) do {
				const PPID bill_id = p_billc->data.ID;
				PPTransferItem ti;
				for(int rbb = 0; p_trfr->EnumItems(bill_id, &rbb, &ti) > 0;) {
					if(ti.GoodsID == goodsID) {
						SETIFZ(last_suitable_lot_id_without_serial, ti.LotID);
						if(serial_buf.NotEmpty() && candidate_lot_list_by_serial.getCount()) {
							if(candidate_lot_list_by_serial.lsearch(ti.LotID)) {
								target_lot_id = ti.LotID;
							}
						}
					}
				}
			} while(!target_lot_id && p_billc->search(2, &k2, spPrev) && k2.OpID == transf_op_id);
			ReceiptTbl::Rec target_lot_rec;
			SETIFZ(target_lot_id, last_suitable_lot_id_without_serial);
			if(target_lot_id && p_trfr->Rcpt.Search(target_lot_id, &target_lot_rec) > 0) {
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pResultLotID, target_lot_id);
	return ok;
}

PPID PPMarketplaceInterface_Wildberries::AdjustReceiptOnExpend(const Sale & rWbItem, LDATE dt, PPID locID, PPID goodsID, double neededQtty, double nominalPrice, int use_ta)
{
	PPID   result_lot_id = 0;
	PPObjBill * p_bobj = BillObj;
	SString serial_buf;
	const  PPID rcpt_op_id = CConfig.ReceiptOp;
	THROW_SL(checkdate(dt)); 
	if(rWbItem.IncomeID && rWbItem.Ware.ID && locID) {
		MakeSerialIdent(rWbItem.IncomeID, rWbItem.Ware, serial_buf);
		PPIDArray lot_id_list;
		bool   lot_found = false;
		PPID   intrexp_op_id = 0;
		ReceiptTbl::Rec lot_rec;
		{
			PPObjOprKind op_obj;
			PPOprKind op_rec;
			for(SEnum en = op_obj.Enum(0); !intrexp_op_id && en.Next(&op_rec) > 0;) {
				if(IsIntrOp(op_rec.ID) == INTREXPND)
					intrexp_op_id = op_rec.ID;
			}
		}
		if(p_bobj->SearchLotsBySerialExactly(serial_buf, &lot_id_list) > 0) {
			PPID   lot_id = 0;
			PPID   lot_candidate_for_intrexpend = 0; // Лот - кандидат на передачу товара межскладом
			for(uint i = 0; !lot_id && i < lot_id_list.getCount(); i++) {
				const PPID iter_lot_id = lot_id_list.get(i);
				if(p_bobj->trfr->Rcpt.Search(iter_lot_id, &lot_rec) > 0 && lot_rec.GoodsID == goodsID) {
					if(lot_rec.LocID == locID) {
						lot_id = iter_lot_id;
					}
					else if(intrexp_op_id) {
						BillTbl::Rec bill_rec;
						if(p_bobj->Search(lot_rec.BillID, &bill_rec) > 0) {
							if(GetOpType(bill_rec.OpID) == rcpt_op_id && bill_rec.EdiOp == PPEDIOP_MRKTPLC_RECEIPT) {
								lot_candidate_for_intrexpend = iter_lot_id;
							}
							else if(!lot_candidate_for_intrexpend) {
								double down_lim = 0.0;
								double up_lim = 0.0;
								p_bobj->trfr->GetBounds(iter_lot_id, dt, -1, &down_lim, &up_lim);
								if(down_lim >= neededQtty) {
									lot_candidate_for_intrexpend = iter_lot_id;
								}
							}
						}
					}
				}
			}
			if(!lot_id) {
				// @v12.1.10 {
				if(lot_candidate_for_intrexpend) {
					assert(intrexp_op_id);
					{
						const  PPID ar_id = PPObjLocation::WarehouseToObj(locID);
						SString bill_code;
						PPBillPacket::SetupObjectBlock sob;
						PPBillPacket pack;
						bill_code.Z().Cat("TRANSF").CatChar('-').Cat(rWbItem.IncomeID);
						THROW(p_bobj->trfr->Rcpt.Search(lot_candidate_for_intrexpend, &lot_rec) > 0 && lot_rec.GoodsID == goodsID);
						{
							PPTransaction tra(use_ta);
							THROW(tra);
							THROW(pack.CreateBlank(intrexp_op_id, 0, lot_rec.LocID, 0));
							pack.Rec.Dt = dt;
							STRNSCPY(pack.Rec.Code, bill_code);
							THROW(pack.SetupObject(ar_id, sob));
							{
								{
									PPTransferItem ti;
									LongArray row_idx_list;
									ti.GoodsID = goodsID;
									ti.LotID = lot_candidate_for_intrexpend;
									ti.Cost = lot_rec.Cost;
									ti.Price = lot_rec.Price;
									ti.Quantity_ = -fabs(neededQtty);
									THROW(pack.InsertRow(&ti, &row_idx_list));
									assert(row_idx_list.getCount() == 1);
								}
								PPGetWord(PPWORD_AT_AUTO, 0, pack.SMemo);
								pack.InitAmounts();
								p_bobj->FillTurnList(&pack);
								THROW(p_bobj->TurnPacket(&pack, 0));
								assert(pack.GetTCount() == 1);
								THROW(pack.GetTCount() == 1);
							}
							THROW(tra.Commit());
							R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
							{
								const PPTransferItem & r_ti = pack.ConstTI(0);
								TransferTbl::Rec trfr_rec;
								if(p_bobj->trfr->SearchByBill(r_ti.BillID, 1, r_ti.RByBill, &trfr_rec) > 0) {
									result_lot_id = trfr_rec.LotID;
									lot_found = true;
								}
							}
						}
					}
				}
				// } @v12.1.10 
			}
			else {
				double adj_cost = 0.0;
				{
					uint   _pos = 0;
					PPID   own_lot_id = 0;
					ReceiptTbl::Rec own_lot_rec;
					if(MpLotToOwnLotAssocList.Search(lot_id, &own_lot_id, &_pos)) {
						if(p_bobj->trfr->Rcpt.Search(own_lot_id, &own_lot_rec) > 0) {
							adj_cost = own_lot_rec.Cost;
						}
					}
					else if(SearchOriginalLotForMp(serial_buf, dt, locID, goodsID, &own_lot_id) > 0) {
						if(p_bobj->trfr->Rcpt.Search(own_lot_id, &own_lot_rec) > 0) {
							MpLotToOwnLotAssocList.Add(lot_id, own_lot_id);
							adj_cost = own_lot_rec.Cost;
						}							
					}
				}
				bool   do_update = false;
				double adj_price = lot_rec.Price;
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
				if(nominalPrice > 0.0 && nominalPrice != lot_rec.Price) {
					adj_price = nominalPrice;
					do_update = true;
				}
				if(adj_cost > 0.0 && adj_cost != lot_rec.Cost) {
					do_update = true;
				}
				if(do_update) {
					PPBillPacket rcpt_bill_pack;
					THROW(p_bobj->ExtractPacketWithFlags(rcpt_bill_id, &rcpt_bill_pack, BPLD_FORCESERIALS) > 0);
					{
						uint   ti_pos = 0;
						THROW(rcpt_bill_pack.SearchLot(lot_id, &ti_pos));
						if(checkdate(adj_date)) {
							rcpt_bill_pack.Rec.Dt = adj_date;
						}
						if(adj_qtty) {
							rcpt_bill_pack.TI(ti_pos).Quantity_ = adj_qtty;
						}
						if(adj_price > 0.0) {
							rcpt_bill_pack.TI(ti_pos).Price = adj_price;
						}
						if(adj_cost > 0.0) {
							rcpt_bill_pack.TI(ti_pos).Cost = adj_cost;
						}
						const int upr = p_bobj->UpdatePacket(&rcpt_bill_pack, 1/*use_ta*/);
						THROW(upr);
						if(upr > 0)
							R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, rcpt_bill_pack.Rec.ID, 1/*upd*/);
					}
				}
				result_lot_id = lot_id;
				lot_found = true;
			}
		}
		if(!lot_found) {
			result_lot_id = CreateReceipt(rWbItem.IncomeID, rWbItem.Ware, dt, locID, goodsID, fabs(neededQtty), true, 1);
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
	bool   debug_mark = false; // @debug
	Reference * p_ref = PPRef;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	SString fmt_buf;
	SString msg_buf;
	TSCollection <PPMarketplaceInterface_Wildberries::Sale> sale_list;
	const PPID order_op_id = R_Prc.GetOrderOpID();
	const PPID sale_op_id = R_Prc.GetSaleOpID();
	THROW(sale_op_id); // @todo @err
	RequestSales(sale_list);
	if(sale_list.getCount()) {
		SString bill_code;
		SString ord_bill_code;
		SString serial_buf;
		SString ord_serial_buf;
		SString org_order_ident; // Оригинальный идентификатор строки заказа
		PPIDArray mp_lot_list; // Список идентификаторов лотов на маркетплейсе, "увиденных" при обработке документов
		for(uint i = 0; i < sale_list.getCount(); i++) {
			const PPMarketplaceInterface_Wildberries::Sale * p_wb_item = sale_list.at(i);
			if(p_wb_item) {
				if(p_wb_item->SrID.IsEqiAscii("9047889103344706579.0.4")) {
					debug_mark = true;
				}
				PPID   wh_id = 0;
				const  double seller_part_amount = p_wb_item->ForPay;
				BillTbl::Rec ord_bill_rec;
				BillTbl::Rec ex_bill_rec;
				ResolveWarehouseByName(WhList, p_wb_item->WarehouseName, LConfig.Location, &wh_id);
				bill_code.Z().Cat(p_wb_item->SaleId);
				ord_bill_code.Z().Cat(p_wb_item->GNumber);
				if(p_bobj->P_Tbl->SearchByCode(bill_code, sale_op_id, ZERODATE, &ex_bill_rec) > 0) {
					// Документ уже акцептирован.
					// Увы, все равно придется его открыть - нужен список лотов, которые он использует и, возможно, надо установить какие-то суммы.
					PPBillPacket ex_pack;
					THROW(p_bobj->ExtractPacketWithFlags(ex_bill_rec.ID, &ex_pack, BPLD_FORCESERIALS) > 0);
					{
						for(uint tiidx = 0; tiidx < ex_pack.GetTCount(); tiidx++) {
							const PPID lot_id = ex_pack.ConstTI(tiidx).LotID;
							if(lot_id > 0)
								mp_lot_list.add(lot_id);
						}
					}
					if(seller_part_amount > 0.0) {
						double ex_seller_part_amount = ex_pack.Amounts.Get(PPAMT_MP_SELLERPART, 0);
						if(ex_seller_part_amount != seller_part_amount) {
							ex_pack.Amounts.Put(PPAMT_MP_SELLERPART, 0, seller_part_amount, 1, 1);
							p_bobj->FillTurnList(&ex_pack);
							const int upr = p_bobj->UpdatePacket(&ex_pack, 1);
							if(upr > 0) {
								R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, ex_pack.Rec.ID, 1/*upd*/);
							}
							else if(!upr) {
								R_Prc.GetLogger().LogLastError();
							}
						}
					}
				}
				else if(p_bobj->P_Tbl->SearchByCode(ord_bill_code, order_op_id, ZERODATE, &ord_bill_rec) > 0) {
					PPBillPacket ord_pack;
					THROW(p_bobj->ExtractPacketWithFlags(ord_bill_rec.ID, &ord_pack, BPLD_FORCESERIALS) > 0);
					{
						PPBillPacket pack;
						PPID   ex_bill_id = 0;
						Goods2Tbl::Rec goods_rec;
						PPBillPacket::SetupObjectBlock sob;
						PPID   ar_id = CreateBuyer(p_wb_item, 1/*use_ta*/);
						if(!pack.CreateBlank_WithoutCode(sale_op_id, 0, wh_id, 1)) {
							R_Prc.GetLogger().LogLastError();
						}
						else {
							const double sold_quantity = 1.0; // 
							pack.Rec.Dt = checkdate(p_wb_item->Dtm.d) ? p_wb_item->Dtm.d : getcurdate_();
							STRNSCPY(pack.Rec.Code, bill_code);
							if(ar_id) {
								PPBillPacket::SetupObjectBlock sob;
								pack.SetupObject(ar_id, sob);
							}
							//
							//pack.Rec.DueDate = checkdate(p_src_pack->DlvrDtm.d) ? p_src_pack->DlvrDtm.d : ZERODATE;
							//sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop;
							//
							PPID   goods_id = CreateWare(p_wb_item->Ware, 1/*use_ta*/);
							if(goods_id) {
								double nominal_price = 0.0;
								if(p_wb_item->PriceWithDiscount > 0.0)
									nominal_price = p_wb_item->PriceWithDiscount;
								else if(p_wb_item->TotalPrice > 0.0)
									nominal_price = p_wb_item->TotalPrice;
								else
									nominal_price = p_wb_item->FinishedPrice;
								// Предполагаем, что на маркетплейсе заказ может прийти только тогда, когда товар уже есть на складе.
								// Таким образом, дата прихода должна быть не больше даты заказа.
								const  LDATE lot_date = checkdate(ord_pack.Rec.Dt) ? ord_pack.Rec.Dt : pack.Rec.Dt;
								PPID   lot_id = AdjustReceiptOnExpend(*p_wb_item, lot_date, wh_id, goods_id, sold_quantity, nominal_price, 1/*use_ta*/);
								if(lot_id) {
									mp_lot_list.add(lot_id);
									uint   ord_ti_idx = 0; // [1..]
									org_order_ident.Z(); // Оригинальный идентификатор строки заказа
									for(uint oti = 0; !ord_ti_idx && oti < ord_pack.GetTCount(); oti++) {
										const PPTransferItem & r_ord_ti = ord_pack.ConstTI(oti);
										if(labs(r_ord_ti.GoodsID) == goods_id) {
											ord_pack.LTagL.GetString(PPTAG_LOT_SN, oti, ord_serial_buf);
											ord_pack.LTagL.GetString(PPTAG_LOT_ORGORDERIDENT, oti, org_order_ident);
											if(p_wb_item->SrID.NotEmpty()) {
												if(org_order_ident.IsEqiUtf8(p_wb_item->SrID)) {
													ord_ti_idx = oti+1;
												}
											}
											else if(p_wb_item->IncomeID != 0) {
												if(ord_serial_buf.ToInt64() == p_wb_item->IncomeID) {
													ord_ti_idx = oti+1;
												}
											}
										}
									}
									if(!ord_ti_idx) {
										// @todo @err
										// Не удалось найти заказ, который закрывается документом продажи %s
										PPLoadText(PPTXT_MP_ORDERFORSALESITEMNFOUND, fmt_buf);
										(temp_buf = bill_code).CatDiv('-', 1).Cat(p_wb_item->Dtm.d, DATF_DMY);
										msg_buf.Printf(fmt_buf, temp_buf.cptr());
										R_Prc.GetLogger().Log(msg_buf);
									}
									else if(p_bobj->InsertShipmentItemByOrder(&pack, &ord_pack, static_cast<int>(ord_ti_idx)-1, lot_id/*srcLotID*/, 1.0, 0)) {
										const long new_row_idx = 0;//row_idx_list.get(0);
										ReceiptTbl::Rec lot_rec;
										if(p_bobj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
											PPTransferItem & r_ti = pack.TI(new_row_idx);
											r_ti.Cost = lot_rec.Cost;
											r_ti.Price = lot_rec.Price;
											r_ti.Discount = (r_ti.Price - p_wb_item->FinishedPrice);
										}
										// @v12.1.9 {
										if(p_wb_item->PriceWithDiscount > 0.0)
											pack.LTagL.SetReal(PPTAG_LOT_MP_SELLERSPRICE, new_row_idx, &p_wb_item->PriceWithDiscount);
										// } @v12.1.9 
										pack.LTagL.SetString(PPTAG_LOT_SN, new_row_idx, MakeSerialIdent(p_wb_item->IncomeID, p_wb_item->Ware, temp_buf));
										pack.LTagL.SetString(PPTAG_LOT_ORGORDERIDENT, new_row_idx, temp_buf.Z().Cat(p_wb_item->SrID));
										if(p_wb_item->Spp != 0.0) {
											pack.LTagL.SetReal(PPTAG_LOT_MP_DISCOUNTPCT, new_row_idx, &p_wb_item->Spp);
										}
										pack.InitAmounts();
										if(p_wb_item->ForPay > 0.0) {
											pack.Amounts.Put(PPAMT_MP_SELLERPART, 0, p_wb_item->ForPay, 1, 1);
										}
										p_bobj->FillTurnList(&pack);
										pack.SetupEdiAttributes(PPEDIOP_MRKTPLC_SALE, "MP-WILDBERRIES", 0);
										if(p_bobj->TurnPacket(&pack, 1)) {
											R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
											ok = 1;
										}
										else {
											R_Prc.GetLogger().LogLastError();
										}
									}
									else {
										R_Prc.GetLogger().LogLastError();
									}
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
					PPLoadText(PPTXT_MP_ORDERFORSALESITEMNFOUND, fmt_buf);
					(temp_buf = bill_code).CatDiv('-', 1).Cat(p_wb_item->Dtm.d, DATF_DMY);
					msg_buf.Printf(fmt_buf, temp_buf.cptr());
					R_Prc.GetLogger().Log(msg_buf);
				}
			}
		}
		if(mp_lot_list.getCount()) {
			mp_lot_list.sortAndUndup();
			for(uint i = 0; i < mp_lot_list.getCount(); i++) {
				const PPID lot_id = mp_lot_list.get(i);
				ReceiptTbl::Rec lot_rec;
				if(p_bobj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
					if(lot_rec.Cost == 0.0) {
						double adj_cost = 0.0;
						{
							uint   _pos = 0;
							PPID   own_lot_id = 0;
							ReceiptTbl::Rec own_lot_rec;
							if(MpLotToOwnLotAssocList.Search(lot_id, &own_lot_id, &_pos)) {
								if(p_bobj->trfr->Rcpt.Search(own_lot_id, &own_lot_rec) > 0) {
									adj_cost = own_lot_rec.Cost;
								}
							}
							else {
								PPID   own_lot_id = 0;
								p_ref->Ot.GetTagStr(PPOBJ_LOT, lot_id, PPTAG_LOT_SN, serial_buf);
								if(SearchOriginalLotForMp(serial_buf, lot_rec.Dt, lot_rec.LocID, lot_rec.GoodsID, &own_lot_id) > 0) {
									if(p_bobj->trfr->Rcpt.Search(own_lot_id, &own_lot_rec) > 0) {
										MpLotToOwnLotAssocList.Add(lot_id, own_lot_id);
										adj_cost = own_lot_rec.Cost;
									}							
								}
							}
						}
						if(adj_cost > 0.0) {
							PPBillPacket rcpt_bpack;
							if(p_bobj->ExtractPacketWithFlags(lot_rec.BillID, &rcpt_bpack, BPLD_FORCESERIALS) > 0) {
								uint   tiidx = 0;
								if(rcpt_bpack.SearchLot(lot_id, &tiidx)) {
									rcpt_bpack.TI(tiidx).Cost = adj_cost;
									rcpt_bpack.InitAmounts();
									p_bobj->FillTurnList(&rcpt_bpack);
									const int upr = p_bobj->UpdatePacket(&rcpt_bpack, 1);
									THROW(upr);
									if(upr > 0)
										R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, rcpt_bpack.Rec.ID, 1/*upd*/);
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

int PPMarketplaceInterface_Wildberries::ImportOrders()
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	TSCollection <PPMarketplaceInterface_Wildberries::Sale> order_list;
	RequestOrders(order_list);
	if(order_list.getCount()) {
		const PPID order_op_id = R_Prc.GetOrderOpID();
		THROW(order_op_id); // @todo @err
		{
			SString bill_code;
			LongArray seen_idx_list;
			for(uint ord_list_idx = 0; ord_list_idx < order_list.getCount(); ord_list_idx++) {
				if(!seen_idx_list.lsearch(static_cast<long>(ord_list_idx))) {
					seen_idx_list.add(static_cast<long>(ord_list_idx));
					const PPMarketplaceInterface_Wildberries::Sale * p_wb_item = order_list.at(ord_list_idx);
					if(p_wb_item) {
						PPID   wh_id = 0;
						ResolveWarehouseByName(WhList, p_wb_item->WarehouseName, LConfig.Location, &wh_id);
						bill_code.Z().Cat(p_wb_item->GNumber);
						if(p_bobj->P_Tbl->SearchByCode(bill_code, order_op_id, ZERODATE, 0) > 0) {
							;
						}
						else {
							PPBillPacket pack;
							PPID   ex_bill_id = 0;
							Goods2Tbl::Rec goods_rec;
							PPBillPacket::SetupObjectBlock sob;
							PPID   ar_id = CreateBuyer(p_wb_item, 1/*use_ta*/);
							if(pack.CreateBlank_WithoutCode(order_op_id, 0, wh_id, 1)) {
								pack.Rec.Dt = checkdate(p_wb_item->Dtm.d) ? p_wb_item->Dtm.d : getcurdate_();
								STRNSCPY(pack.Rec.Code, bill_code);
								if(ar_id) {
									PPBillPacket::SetupObjectBlock sob;
									pack.SetupObject(ar_id, sob);
								}
								//
								//pack.Rec.DueDate = checkdate(p_src_pack->DlvrDtm.d) ? p_src_pack->DlvrDtm.d : ZERODATE;
								//sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop;
								//
								PPID   goods_id = CreateWare(p_wb_item->Ware, 1/*use_ta*/);
								if(!goods_id) {
									// @todo @err
								}
								else {
									PPTransferItem ti;
									LongArray row_idx_list;
									ti.GoodsID = goods_id;
									// @v12.1.9 ti.Price = p_wb_item->FinishedPrice;
									// @v12.1.9 {
									ti.Price = p_wb_item->PriceWithDiscount;
									ti.Discount = p_wb_item->PriceWithDiscount - p_wb_item->FinishedPrice;
									// } @v12.1.9
									ti.Quantity_ = 1.0;
									if(pack.InsertRow(&ti, &row_idx_list)) {
										{
											assert(row_idx_list.getCount() == 1);
											const long new_row_idx = row_idx_list.get(0);
											pack.LTagL.SetString(PPTAG_LOT_SN, new_row_idx, MakeSerialIdent(p_wb_item->IncomeID, p_wb_item->Ware, temp_buf));
											pack.LTagL.SetString(PPTAG_LOT_ORGORDERIDENT, new_row_idx, p_wb_item->SrID);
										}
										{
											//
											// Если покупатель заказал несколько штук одного и того же, то каждая штука
											// будет задана отдельным документом заказа, но все эти документы будут содержать одно и то же значение GNumber
											//
											for(uint j = ord_list_idx+1; j < order_list.getCount(); j++) {
												const PPMarketplaceInterface_Wildberries::Sale * p_wb_item_inner = order_list.at(j);
												if(p_wb_item_inner && p_wb_item_inner->GNumber.IsEqiUtf8(p_wb_item->GNumber)) {
													if(!seen_idx_list.lsearch(static_cast<long>(j))) {
														seen_idx_list.add(static_cast<long>(j));
														PPTransferItem ti_inner;
														const PPID inner_goods_id = CreateWare(p_wb_item_inner->Ware, 1/*use_ta*/);
														if(!inner_goods_id) {
															// @todo @err
														}
														else {
															ti_inner.GoodsID = inner_goods_id;
															// @v12.1.9 ti_inner.Price = p_wb_item_inner->FinishedPrice;
															// @v12.1.9 {
															ti_inner.Price = p_wb_item_inner->PriceWithDiscount;
															ti_inner.Discount = p_wb_item_inner->PriceWithDiscount - p_wb_item_inner->FinishedPrice;
															// } @v12.1.9 
															ti_inner.Quantity_ = 1.0;
															row_idx_list.Z();
															if(pack.InsertRow(&ti_inner, &row_idx_list)) {
																assert(row_idx_list.getCount() == 1);
																const long new_row_idx = row_idx_list.get(0);
																pack.LTagL.SetString(PPTAG_LOT_SN, new_row_idx, MakeSerialIdent(p_wb_item_inner->IncomeID, p_wb_item_inner->Ware, temp_buf));
																pack.LTagL.SetString(PPTAG_LOT_ORGORDERIDENT, new_row_idx, p_wb_item_inner->SrID);
																if(p_wb_item_inner->Spp != 0.0) {
																	pack.LTagL.SetReal(PPTAG_LOT_MP_DISCOUNTPCT, new_row_idx, &p_wb_item_inner->Spp);
																}
															}
														}
													}
												}
											}
										}
										//
										if(p_wb_item->Flags & Sale::fIsCancel) {
											pack.Rec.Flags2 |= BILLF2_DECLINED;
										}
										pack.InitAmounts();
										p_bobj->FillTurnList(&pack);
										pack.SetupEdiAttributes(PPEDIOP_MRKTPLC_ORDER, "MP-WILDBERRIES", 0);
										if(p_bobj->TurnPacket(&pack, 1)) {
											R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
											ok = 1;
										}
										else {
											R_Prc.GetLogger().LogLastError();
										}
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

int PPMarketplaceInterface_Wildberries::FindShipmentBillByOrderIdent(const char * pOrgOrdIdent, PPIDArray & rShipmBillIdList)
{
	int    ok = -1;
	rShipmBillIdList.Z();
	if(!isempty(pOrgOrdIdent)) {
		Reference * p_ref = PPRef;
		PPObjBill * p_bobj = BillObj;
		Transfer * p_trfr = p_bobj->trfr;
		PPIDArray lot_id_list;
		PPIDArray temp_list;
		p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_LOT, PPTAG_LOT_ORGORDERIDENT, pOrgOrdIdent, &lot_id_list);
		if(lot_id_list.getCount()) {
			PPIDArray bill_id_list;
			for(uint i = 0; i < lot_id_list.getCount(); i++) {
				const PPID ord_lot_id = lot_id_list.get(i);
				ReceiptTbl::Rec ord_lot_rec;
				if(p_trfr->Rcpt.Search(ord_lot_id, &ord_lot_rec) > 0) {
					p_bobj->GetShipmByOrder(ord_lot_rec.BillID, 0/*DateRange*/, temp_list);
					bill_id_list.add(&temp_list);
				}
			}
			if(bill_id_list.getCount()) {
				bill_id_list.sortAndUndup();
				for(uint i = 0; i < bill_id_list.getCount(); i++) {
					const PPID bill_id = bill_id_list.get(i);
					PPLotTagContainer ltc;
					p_bobj->LoadRowTagListForDraft(bill_id, ltc);
					LongArray row_idx_list;
					if(ltc.SearchString(pOrgOrdIdent, PPTAG_LOT_ORGORDERIDENT, 0, row_idx_list)) {
						rShipmBillIdList.add(bill_id);
					}
					//ltc.GetTagStr(ln-1, PPTAG_LOT_FSRARINFB, ref_b);
					//ltc.GetTagStr(ln-1, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code_by_mark);					
				}
				if(rShipmBillIdList.getCount() == 1) {
					ok = 1;
				}
				else if(rShipmBillIdList.getCount() > 1) {
					ok = -2;
				}
			}
		}
	}
	return ok;
}

int PPMarketplaceInterface_Wildberries::ImportFinancialTransactions()
{
	int    ok = -1;
	bool   debug_mark = false;
	Reference * p_ref = PPRef;
	const  LDATETIME now_dtm = getcurdatetime_();
	SString temp_buf;
	SString fmt_buf;
	SString msg_buf;
	TSCollection <PPMarketplaceInterface_Wildberries::SalesRepDbpEntry> sales_rep_dbp_list;
	DateRange period;
	PPID   acc_id = 0;
	PPID   acs_id = 0;
	PPID   op_id = 0;
	PPObjBill * p_bobj = BillObj;
	Transfer * p_trfr = p_bobj->trfr;
	PPObjOprKind op_obj;
	PPObjAccSheet acs_obj;
	PPObjAccount acc_obj;
	PPObjTag tag_obj;
	PPObjGlobalUserAcc gua_obj;
	{
		PPTransaction tra(1);
		THROW(tra);
		acs_id = R_Prc.Helper_GetMarketplaceOpsAccSheetID(true/*createIfNExists*/, true/*createArticles*/, 0);
		THROW(acs_id);
		{
			acc_id = R_Prc.Helper_GetMarketplaceOpsAccount(true, 0);
			THROW(acc_id);
		}
		op_obj.GetGenericAccTurnForRegisterOp(&op_id, 0/*use_ta*/);
		THROW(op_id);
		THROW(tra.Commit());
	}
	period.low.encode(1, 5, 2024);
	period.upp = now_dtm.d;
	//PPTXT_MP_FINTRA_WB_REQ               "Запрос отчета о продажах по реализации за период %s"
	PPLoadText(PPTXT_MP_FINTRA_WB_REQ, fmt_buf);
	PPFormatPeriod(&period, temp_buf);
	msg_buf.Printf(fmt_buf, temp_buf.cptr());
	R_Prc.GetLogger().Log(msg_buf);
	int r = RequestSalesReportDetailedByPeriod(period, sales_rep_dbp_list);
	if(!sales_rep_dbp_list.getCount()) {
		R_Prc.GetLogger().Log(PPLoadTextS(PPTXT_MP_FINTRA_WB_REPEMPTY, msg_buf));
	}
	else {
		// PPTXT_MPWB_NATIVEOPS                 "1,Возмещение издержек по перевозке/по складским операциям с товаром;2,Логистика;3,Пересчет платной приемки;4,Продажа;5,Удержание;6,Хранение"
		enum {
			nativeopCargo       = 1, // Возмещение издержек по перевозке/по складским операциям с товаром
			nativeopLogistics   = 2, // Логистика
			nativeopAcceptance  = 3, // Пересчет платной приемки
			nativeopSales       = 4, // Продажа
			nativeopDeduction   = 5, // Удержание
			nativeopStorage     = 6, // Хранение
		};
		PPLoadTextUtf8(PPTXT_MPWB_NATIVEOPS, temp_buf);
		const StringSet ss_native_ops(';', temp_buf);
		SString id_buf;
		SString text_buf;
		PPIDArray shipm_bill_id_list;
		LongArray seen_pos_list; // список обработанных позиций массива sales_rep_dbp_list. 
		StringSet ss_order_ident;
		const char * p_empty_sr_ident_symb = "$empty-sr-id";
		{
			for(uint i = 0; i < sales_rep_dbp_list.getCount(); i++) {
				const SalesRepDbpEntry * p_entry = sales_rep_dbp_list.at(i);
				if(p_entry) {
					temp_buf.Z();
					if(p_entry->SrID.NotEmpty()) {
						temp_buf = p_entry->SrID;
					}
					else {
						temp_buf = p_empty_sr_ident_symb;
					}
					ss_order_ident.add(temp_buf);
				}
			}
			ss_order_ident.sortAndUndup();
		}
		SString sr_ident;
		for(uint ssp = 0; ss_order_ident.get(&ssp, sr_ident);) {
			if(sr_ident.IsEqiAscii("9047889103344706579.0.4")) {
				debug_mark = true;
			}
			const bool is_empty_sr_ident = (sr_ident == p_empty_sr_ident_symb);
			const int fsr = is_empty_sr_ident ? -1 : FindShipmentBillByOrderIdent(sr_ident, shipm_bill_id_list);
			PPID  shipm_bill_id = 0;
			PPBillPacket bpack;
			int   row_idx = -1;
			PPTransferItem * p_ti = 0;
			bool is_bpack_updated = false;
			double freight_amount = 0.0;
			if(fsr > 0) {
				assert(shipm_bill_id_list.getCount() == 1);
				shipm_bill_id = shipm_bill_id_list.get(0);
				if(p_bobj->ExtractPacketWithFlags(shipm_bill_id, &bpack, BPLD_FORCESERIALS) > 0) {
					LongArray row_list;
					bpack.LTagL.SearchString(sr_ident, PPTAG_LOT_ORGORDERIDENT, 0, row_list);
					if(row_list.getCount() == 1) {
						row_idx = row_list.get(0);
						p_ti = &bpack.TI(row_idx);
					}
				}
				else {
					;
				}
			}
			for(uint i = 0; i < sales_rep_dbp_list.getCount(); i++) {
				const SalesRepDbpEntry * p_entry = sales_rep_dbp_list.at(i);
				if(p_entry && ((is_empty_sr_ident && p_entry->SrID.IsEmpty()) || p_entry->SrID == sr_ident)) {
					int   native_op_id = 0;
					for(uint ssp = 0; !native_op_id && ss_native_ops.get(&ssp, temp_buf);) {
						if(temp_buf.Divide(',', id_buf, text_buf) > 0 && text_buf.IsEqiUtf8(p_entry->SupplOpName)) {
							native_op_id = id_buf.ToLong();
						}
					}
					if(!native_op_id) {
						// @todo Написать что-то в лог
					}
					else {
						PPID  loc_id = 0;
						ArticleTbl::Rec ar_rec;
						ResolveWarehouseByName(WhList, p_entry->Warehouse, LConfig.Location, &loc_id);
						switch(native_op_id) {
							case nativeopCargo:
								// ppvz_vw, ppvz_vw_nds, rebill_logistic_cost
								// Насколько я понял из изучения материала, rebill_logistic_cost == ppvz_vw + ppvz_vw_nds
								if(bpack.Rec.ID) {
									double delivery_amount = p_entry->RebillLogisticCost;
									if(delivery_amount > 0.0) {
										freight_amount += delivery_amount;
									}
								}
								break;
							case nativeopLogistics:
								// delivery_amount, delivery_rub, 
								if(bpack.Rec.ID) {
									double delivery_amount = p_entry->DeliveryAmount;
									if(delivery_amount > 0.0) {
										freight_amount += delivery_amount;
									}
								}
								break;
							case nativeopAcceptance:
								// acceptance
								if(p_entry->Acceptance != 0.0) {
									temp_buf.Z().Cat(p_entry->RrdId);
									BillTbl::Rec ex_bill_rec;
									if(p_bobj->P_Tbl->SearchByCode(temp_buf, op_id, ZERODATE, &ex_bill_rec) > 0) {
										;
									}
									else {
										PPBillPacket bpack_at;
										{
											PPTransaction tra(1);
											THROW(tra);
											bpack_at.CreateBlank_WithoutCode(op_id, 0, loc_id, 0);
											bpack_at.Rec.Dt = checkdate(p_entry->RrDtm.d) ? p_entry->RrDtm.d : (checkdate(p_entry->CrDate) ? p_entry->CrDate : now_dtm.d);
											STRNSCPY(bpack_at.Rec.Code, temp_buf);
											if(ArObj.P_Tbl->SearchNum(acs_id, ARTN_MRKTPLCACC_ACCEPTANCE, &ar_rec) > 0) {
												PPAccTurn at;
												bpack_at.CreateAccTurn(at);
												at.DbtID.ac = acc_id;
												at.DbtID.ar = ar_rec.ID;
												at.DbtSheet = acs_id;
												at.Amount = -fabs(p_entry->Acceptance);
												bpack_at.Turns.insert(&at);
												bpack_at.Rec.Amount = at.Amount;
												bpack_at.SetupEdiAttributes(0, "MP-WILDBERRIES", 0);
												THROW(p_bobj->TurnPacket(&bpack_at, 0));
											}
											THROW(tra.Commit());
										}
									}
								}
								break;
							case nativeopSales:
								// quantity, retail_price, retail_price_withdisc_rub, retail_amount, commission_percent, ppvz_spp_prc, ppvz_kvw_prc_base, ppvz_kvw_prc, 
								// ppvz_sales_commission, ppvz_for_pay, acquiring_fee, acquiring_percent, ppvz_vw, ppvz_vw_nds	
								if(bpack.Rec.ID) {
									if(p_entry->CommissionPct != 0.0) {
										if(row_idx >= 0) {
											bpack.LTagL.SetReal(PPTAG_LOT_MP_COMMISSIONPCT, row_idx, &p_entry->CommissionPct);
											if(p_entry->RetailPrice > 0.0) {
												double commission_amount = (p_entry->RetailPrice * p_entry->Qtty) * (p_entry->CommissionPct / 100.0);
												bpack.Amounts.Put(PPAMT_MP_COMMISSION, 0, commission_amount, 1, 1);
											}
										}
									}
									if(p_entry->Ppvz_For_Pay != 0.0) {
										bpack.Amounts.Put(PPAMT_MP_SELLERPART, 0, p_entry->Ppvz_For_Pay, 1, 1);
										is_bpack_updated = true;
									}
									/*if(p_entry->Ppvz_Vw != 0.0) {
										bpack.Amounts.Put(PPAMT_MP_COMMISSION, 0, fabs(p_entry->Ppvz_Vw + p_entry->Ppvz_Vw_Vat), 1, 1);
										is_bpack_updated = true;
									}*/
									if(p_entry->AcquiringFee != 0.0) {
										bpack.Amounts.Put(PPAMT_MP_ACQUIRING, 0, fabs(p_entry->AcquiringFee), 1, 1);
										is_bpack_updated = true;
									}
								}
								break;
							case nativeopDeduction:
								// deduction
								if(p_entry->Deduction != 0.0) {
									temp_buf.Z().Cat(p_entry->RrdId);
									BillTbl::Rec ex_bill_rec;
									if(p_bobj->P_Tbl->SearchByCode(temp_buf, op_id, ZERODATE, &ex_bill_rec) > 0) {
										;
									}
									else {
										PPBillPacket bpack_at;
										{
											PPTransaction tra(1);
											THROW(tra);
											bpack_at.CreateBlank_WithoutCode(op_id, 0, loc_id, 0);
											bpack_at.Rec.Dt = checkdate(p_entry->RrDtm.d) ? p_entry->RrDtm.d : (checkdate(p_entry->CrDate) ? p_entry->CrDate : now_dtm.d);
											STRNSCPY(bpack_at.Rec.Code, temp_buf);
											if(ArObj.P_Tbl->SearchNum(acs_id, ARTN_MRKTPLCACC_DEDUCTION, &ar_rec) > 0) {
												PPAccTurn at;
												bpack_at.CreateAccTurn(at);
												at.DbtID.ac = acc_id;
												at.DbtID.ar = ar_rec.ID;
												at.DbtSheet = acs_id;
												at.Amount = -fabs(p_entry->Deduction);
												bpack_at.Turns.insert(&at);
												bpack_at.Rec.Amount = at.Amount;
												if(p_entry->BonusTypeName.NotEmpty()) {
													(bpack_at.SMemo = p_entry->BonusTypeName).Transf(CTRANSF_UTF8_TO_INNER);
												}
												bpack_at.SetupEdiAttributes(0, "MP-WILDBERRIES", 0);
												THROW(p_bobj->TurnPacket(&bpack_at, 0));
											}
											THROW(tra.Commit());
										}
									}
								}
								break;
							case nativeopStorage:
								// storage_fee
								if(p_entry->StorageFee != 0.0) {
									temp_buf.Z().Cat(p_entry->RrdId);
									BillTbl::Rec ex_bill_rec;
									if(p_bobj->P_Tbl->SearchByCode(temp_buf, op_id, ZERODATE, &ex_bill_rec) > 0) {
										;
									}
									else {
										PPBillPacket bpack_at;
										{
											PPTransaction tra(1);
											THROW(tra);
											bpack_at.CreateBlank_WithoutCode(op_id, 0, loc_id, 0);
											bpack_at.Rec.Dt = checkdate(p_entry->RrDtm.d) ? p_entry->RrDtm.d : (checkdate(p_entry->CrDate) ? p_entry->CrDate : now_dtm.d);
											STRNSCPY(bpack_at.Rec.Code, temp_buf);
											if(ArObj.P_Tbl->SearchNum(acs_id, ARTN_MRKTPLCACC_STORAGE, &ar_rec) > 0) {
												PPAccTurn at;
												bpack_at.CreateAccTurn(at);
												at.DbtID.ac = acc_id;
												at.DbtID.ar = ar_rec.ID;
												at.DbtSheet = acs_id;
												at.Amount = -fabs(p_entry->StorageFee);
												bpack_at.Turns.insert(&at);
												bpack_at.Rec.Amount = at.Amount;
												bpack_at.SetupEdiAttributes(0, "MP-WILDBERRIES", 0);
												THROW(p_bobj->TurnPacket(&bpack_at, 0));
											}
											THROW(tra.Commit());
										}
									}
								}
								break;
						}
					}
				}
			}
			if(freight_amount != 0.0) {
				bpack.Amounts.Put(PPAMT_FREIGHT, 0L, freight_amount, 1/*ignoreZero*/, 1/*replace*/);
				is_bpack_updated = true;
			}
			if(is_bpack_updated) {
				//bpack.InitAmounts();
				p_bobj->FillTurnList(&bpack);
				const int upr = p_bobj->UpdatePacket(&bpack, 1);
				if(upr > 0) {
					R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, bpack.Rec.ID, 1/*upd*/);
				}
				else if(!upr) {
					R_Prc.GetLogger().LogLastError();
				}
			}
		}
	}
	CATCH
		R_Prc.GetLogger().LogLastError();
		ok = 0;
	ENDCATCH
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
		//PPLogger logger;
		PrcssrMarketplaceInterchange prc;
		MarketplaceInterchangeFilt filt;
		filt.GuaID = gua_id;
		if(prc.Init(&filt)) {
			PPMarketplaceInterface * p_ifc = prc.GetIfc();
			if(p_ifc) {
				PPMarketplaceInterface_Wildberries * p_ifc_wb = static_cast<PPMarketplaceInterface_Wildberries *>(p_ifc);
				const int gmppr = p_ifc_wb->GetMarketplacePerson(&mp_psn_id, 1);
				TSCollection <PPMarketplaceInterface_Wildberries::Warehouse> wh_list2;
				TSCollection <PPMarketplaceInterface_Wildberries::Stock> stock_list;
				//TSCollection <PPMarketplaceInterface_Wildberries::Sale> sale_list;
				//TSCollection <PPMarketplaceInterface_Wildberries::Sale> order_list;
				//TSCollection <PPMarketplaceInterface_Wildberries::Income> income_list;
				TSCollection <PPMarketplaceInterface_Wildberries::SalesRepDbpEntry> sales_rep_dbp_list;

				int r = 0;
				r = p_ifc_wb->RequestReturns();
				r = p_ifc_wb->RequestWarehouseList2(wh_list2);
				r = p_ifc_wb->RequestDocumentsList();
				r = p_ifc_wb->RequestBalance();
				r = p_ifc_wb->RequestWareList();
				//
				DateRange period;
				period.SetPredefined(PREDEFPRD_LASTMONTH, ZERODATE);
				r = p_ifc_wb->ImportReceipts();
				r = p_ifc_wb->ImportOrders();
				r = p_ifc_wb->ImportSales();
				//r = p_ifc_wb->RequestWarehouseList(wh_list);
				r = p_ifc_wb->RequestAcceptanceReport(period);
				r = p_ifc_wb->RequestSupplies();
				//r = p_ifc_wb->RequestIncomes(income_list);
				r = p_ifc_wb->RequestCommission();
				r = p_ifc_wb->RequestStocks(stock_list);
				//r = p_ifc_wb->RequestOrders(order_list);
				//r = p_ifc_wb->RequestSales(sale_list);
				r = p_ifc_wb->RequestGoodsPrices();
				{
					//period.SetPredefined(PREDEFPRD_LASTMONTH, ZERODATE);
					//period.low.encode(1, 5, 2024);
					//period.upp = getcurdate_();
					//r = p_ifc_wb->RequestSalesReportDetailedByPeriod(period, sales_rep_dbp_list);
					r = p_ifc_wb->ImportFinancialTransactions();
				}
			}
		}
	}
	delete dlg;
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(MarketplaceInterchange); MarketplaceInterchangeFilt::MarketplaceInterchangeFilt() : PPBaseFilt(PPFILT_MARKETPLACEINTERCHANGE, 0, 0)
{
	SetFlatChunk(offsetof(MarketplaceInterchangeFilt, ReserveStart),
		offsetof(MarketplaceInterchangeFilt, Reserve) - offsetof(MarketplaceInterchangeFilt, ReserveStart) + sizeof(Reserve));
	Init(1, 0);
}

MarketplaceInterchangeFilt & FASTCALL MarketplaceInterchangeFilt::operator = (const MarketplaceInterchangeFilt & rS)
{
	PPBaseFilt::Copy(&rS, 0);
	return *this;
}

PPMarketplaceConfig::PPMarketplaceConfig() : Tag(PPOBJ_CONFIG), ID(PPCFG_MAIN), Prop(PPPRP_MRKTPLCCFG), OrderOpID(0), SalesOpID(0), TransferToMpOpID(0), Flags(0)
{
	memzero(Reserve, sizeof(Reserve));
	memzero(Reserve2, sizeof(Reserve2));
}

PPMarketplaceConfig & PPMarketplaceConfig::Z()
{
	Tag = PPOBJ_CONFIG;
	ID = PPCFG_MAIN;
	Prop = PPPRP_MRKTPLCCFG;
	OrderOpID = 0;
	SalesOpID = 0;
	TransferToMpOpID = 0;
	Flags = 0;
	return *this;
}

bool PPMarketplaceConfig::IsEq_ForStorage(const PPMarketplaceConfig & rS) const
{
	return (OrderOpID == rS.OrderOpID && SalesOpID == rS.SalesOpID && TransferToMpOpID == rS.TransferToMpOpID);
}

/*static*/int FASTCALL PrcssrMarketplaceInterchange::ReadConfig(PPMarketplaceConfig * pCfg)
{
	static_assert(sizeof(PPMarketplaceConfig) == PROPRECFIXSIZE);
	int    r = PPRef->GetPropMainConfig(PPPRP_MRKTPLCCFG, pCfg, sizeof(*pCfg));
	if(r > 0) {
		if(pCfg)
			pCfg->Flags |= PPMarketplaceConfig::fValid;
	}
	else {
		memzero(pCfg, sizeof(*pCfg));
	}
	return r;
}

/*static*/int FASTCALL PrcssrMarketplaceInterchange::WriteConfig(PPMarketplaceConfig * pCfg, int use_ta)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	PPMarketplaceConfig org_cfg;
	const  int  rcr = ReadConfig(&org_cfg);
	const  bool is_exists = (rcr > 0);
	PPTransaction tra(1);
	THROW(tra);
	if(pCfg) {
		if(pCfg->IsEq_ForStorage(org_cfg)) {
			;
		}
		else {
			PPMarketplaceConfig cfg(*pCfg);
			cfg.Flags &= ~PPMarketplaceConfig::fValid; // transient-flag
			THROW(p_ref->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_MRKTPLCCFG, &cfg, sizeof(cfg), 0));
			DS.LogAction(is_exists ? PPACN_CONFIGUPDATED : PPACN_CONFIGCREATED, PPCFGOBJ_MARKETPLACE, 0, 0, 0);
		}
	}
	else {
		THROW(p_ref->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_MRKTPLCCFG, 0, 0, 0));
	}
	THROW(tra.Commit());
	CATCHZOK
	return ok;
}

/*static*/int PrcssrMarketplaceInterchange::EditConfig()
{
	class MpConfigDialog : public TDialog {
		DECL_DIALOG_DATA(PPMarketplaceConfig);
	public:
		MpConfigDialog() : TDialog(DLG_MPCFG)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			PPIDArray op_type_list;
			RVALUEPTR(Data, pData);
			op_type_list.Z().add(PPOPT_GOODSORDER);
			SetupOprKindCombo(this, CTLSEL_MPCFG_ORDEROP, Data.OrderOpID, 0, &op_type_list, 0);
			op_type_list.Z().add(PPOPT_GOODSEXPEND);
			SetupOprKindCombo(this, CTLSEL_MPCFG_SALEOP, Data.SalesOpID, 0, &op_type_list, 0);
			op_type_list.Z().add(PPOPT_GOODSEXPEND);
			SetupOprKindCombo(this, CTLSEL_MPCFG_TRANSFOP, Data.TransferToMpOpID, 0, &op_type_list, 0);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTLSEL_MPCFG_ORDEROP, &Data.OrderOpID);
			getCtrlData(CTLSEL_MPCFG_SALEOP, &Data.SalesOpID);
			getCtrlData(CTLSEL_MPCFG_TRANSFOP, &Data.TransferToMpOpID);
			//
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	};

	int    ok = -1;
	int    is_new = 0;
	PPMarketplaceConfig cfg;
	MpConfigDialog * dlg = new MpConfigDialog();
	THROW(CheckCfgRights(PPCFGOBJ_MARKETPLACE, PPR_READ, 0));
	THROW(is_new = ReadConfig(&cfg));
	THROW(CheckDialogPtr(&dlg));
	dlg->setDTS(&cfg);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&cfg)) {
			THROW(CheckCfgRights(PPCFGOBJ_MARKETPLACE, PPR_MOD, 0));
			THROW(WriteConfig(&cfg, 1));
			ok = 1;
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

PrcssrMarketplaceInterchange::PrcssrMarketplaceInterchange() : P_Ifc(0), State(0)
{
}
	
PrcssrMarketplaceInterchange::~PrcssrMarketplaceInterchange()
{
	delete P_Ifc;
}

class MarketplaceInterchangeFiltDialog : public TDialog {
	DECL_DIALOG_DATA(MarketplaceInterchangeFilt);
public:
	MarketplaceInterchangeFiltDialog() : TDialog(DLG_MRKTPLCIX)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int   ok = 1;
		RVALUEPTR(Data, pData);
		SetupPPObjCombo(this, CTLSEL_MRKTPLCIX_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, 0, reinterpret_cast<void *>(PPGLS_WILDBERRIES));
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int   ok = 1;
		Data.GuaID = getCtrlLong(CTLSEL_MRKTPLCIX_GUA);
		//
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};

int PrcssrMarketplaceInterchange::EditParam(PPBaseFilt * pBaseFilt)
{
	MarketplaceInterchangeFilt temp_filt;
	if(!temp_filt.IsA(pBaseFilt))
		return 0;
	MarketplaceInterchangeFilt * p_filt = static_cast<MarketplaceInterchangeFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(MarketplaceInterchangeFiltDialog, p_filt);
}

int PrcssrMarketplaceInterchange::InitParam(PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	// @nothingtodo
	return ok;
}

PPID PrcssrMarketplaceInterchange::GetOrderOpID()
{
	PPID   result_id = 0;
	if(Cfg.OrderOpID > 0) {
		result_id = Cfg.OrderOpID;
	}
	else {
		if(Cfg.OrderOpID == 0) {
			PPAlbatrossConfig albtr_cfg;
			result_id = (PPAlbatrosCfgMngr::Get(&albtr_cfg) > 0) ? albtr_cfg.Hdr.OpID : 0;
			if(result_id == 0)
				Cfg.OrderOpID = -1; // Индицирует факт того, что вид операции заказа получить не удается.
			else
				Cfg.OrderOpID = result_id;
		}
	}
	return result_id;
}

PPID PrcssrMarketplaceInterchange::GetSaleOpID()
{
	PPID   result_id = 0;
	if(Cfg.SalesOpID > 0)
		result_id = Cfg.SalesOpID;
	else {
		if(Cfg.SalesOpID == 0) {
			const PPID order_op_id = GetOrderOpID();
			PPOprKind order_op_rec;
			if(order_op_id) {
				if(GetOpData(order_op_id, &order_op_rec) > 0) {
					PPOprKind op_rec;
					for(PPID iter_op_id = 0; !result_id && EnumOperations(PPOPT_GOODSEXPEND, &iter_op_id, &op_rec) > 0;) {
						if(op_rec.Flags & OPKF_ONORDER && op_rec.AccSheetID == order_op_rec.AccSheetID) {
							result_id = op_rec.ID;
						}
					}
				}
			}
			if(result_id == 0)
				Cfg.SalesOpID = -1; // Индицирует факт того, что вид операции продажи получить не удается.
			else
				Cfg.SalesOpID = result_id;
		}
	}
	return result_id;
}

PPID PrcssrMarketplaceInterchange::GetMarketplaceAccSheetID()
{
	PPID   acs_id = 0;
	Reference * p_ref = PPRef;
	PPAccSheet2 acs_rec;
	for(SEnum en = p_ref->EnumByIdxVal(PPOBJ_ACCSHEET, 1, PPOBJ_PERSON); !acs_id && en.Next(&acs_rec) > 0;) {
		if(acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup == PPPRK_MARKETPLACE) {
			acs_id = acs_rec.ID;
		}
	}
	return acs_id;
}

PPID PrcssrMarketplaceInterchange::Helper_GetMarketplaceOpsAccSheetID(bool createIfNExists, bool createArticles, int use_ta)
{
//PPTXT_ACCSHEET_MP_NAME               "Операции на маркетплейсах"
//PPTXT_ACCSHEET_MP_AR_NAMES           "1,Комиссионное вознаграждение;2,Стоимость эквайринга;3,Стоимость хранения;4,Стоимость приемки на склад хранения"
	const char * p_symb = "MARKETPLACE-OPS";
	PPID   acs_id = 0;
	Reference * p_ref = PPRef;
	SString temp_buf;
	PPObjAccSheet acs_obj;
	PPAccSheet2 acs_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(acs_obj.SearchBySymb(p_symb, &acs_id, &acs_rec) > 0) {
			;
		}
		else if(createIfNExists) {
			MEMSZERO(acs_rec);
			PPLoadText(PPTXT_ACCSHEET_MP_NAME, temp_buf);
			STRNSCPY(acs_rec.Name, temp_buf);
			STRNSCPY(acs_rec.Symb, p_symb);
			acs_rec.Assoc = 0;
			acs_rec.ObjGroup = 0;
			acs_rec.Flags = 0;
			THROW(p_ref->AddItem(PPOBJ_ACCSHEET, &acs_id, &acs_rec, 0));		
		}
		if(acs_id && createArticles) {
			SString id_buf;
			SString nm_buf;
			PPLoadText(PPTXT_ACCSHEET_MP_AR_NAMES, temp_buf);
			StringSet ss(';', temp_buf);
			for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				if(temp_buf.Divide(',', id_buf, nm_buf) > 0) {
					long ar_n = id_buf.ToLong();
					if(ar_n > 0 && nm_buf.NotEmptyS()) {
						ArticleTbl::Rec ar_rec;
						const int snr = ArObj.P_Tbl->SearchNum(acs_id, ar_n, &ar_rec);
						if(snr < 0) {
							PPID new_id = 0;
							MEMSZERO(ar_rec);
							ar_rec.Article = ar_n;
							ar_rec.ObjID = ar_n;
							ar_rec.AccSheetID = acs_id;
							STRNSCPY(ar_rec.Name, nm_buf);
							THROW(ArObj.P_Tbl->Add(&new_id, &ar_rec, 0));
						}
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		acs_id = 0;
	ENDCATCH
	return acs_id;
}

PPID PrcssrMarketplaceInterchange::Helper_GetMarketplaceOpsAccount(bool createIfNExists, int use_ta)
{
	PPID   acc_id = 0;
	SString temp_buf;
	PPObjAccount acc_obj;
	PPAccount acc_rec;
	PPID acs_id = 0;
	PPObjTag tag_obj;
	PPObjectTag tag_rec;
	const PPID gua_id = GetGuaPack().Rec.ID;
	THROW(tag_obj.Fetch(PPTAG_GUA_ACCOUNT, &tag_rec) > 0); // @todo @err (создание зарезервированных объектов)
	if(gua_id) {
		PPTransaction tra(use_ta);
		THROW(tra);
		acs_id = Helper_GetMarketplaceOpsAccSheetID(createIfNExists, true/*createArticles*/, 0);
		THROW(acs_id);
		{
			const ObjTagItem * p_tag_item = GetGuaPack().TagL.GetItem(PPTAG_GUA_ACCOUNT);
			int    intval = 0;
			if(p_tag_item && p_tag_item->GetInt(&intval) && acc_obj.Search(intval, &acc_rec) > 0) {
				acc_id = acc_rec.ID;
			}
			else if(createIfNExists) {
				PPAccountPacket acc_pack;
				(temp_buf = "Marketplace account").CatDiv('-', 1).Cat(GetGuaPack().Rec.Name);
				STRNSCPY(acc_pack.Rec.Name, temp_buf);
				acc_pack.Rec.Type = ACY_REGISTER;
				acc_pack.Rec.Kind = ACT_ACTIVE;
				acc_pack.Rec.Flags |= ACF_SYSNUMBER;
				acc_pack.Rec.AccSheetID = acs_id;
				THROW(acc_obj.GenerateNumber(&acc_pack.Rec));
				assert(acc_id == 0);
				THROW(acc_obj.PutPacket(&acc_id, &acc_pack, 0));
				{
					assert(acc_id);
					ObjTagItem tag_item;
					tag_item.Init(PPTAG_GUA_ACCOUNT);
					tag_item.SetInt(PPTAG_GUA_ACCOUNT, acc_id);
					THROW(PPRef->Ot.PutTag(PPOBJ_GLOBALUSERACC, GetGuaPack().Rec.ID, &tag_item, 0));
					THROW(ReloadGuaPack()); // Повторно извлекаем пакет глобальной учетной записи ибо там теперь новый тег
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		acc_id = 0;
	ENDCATCH
	return acc_id;
}

PPID PrcssrMarketplaceInterchange::GetMarketplaceOpsAccSheetID()
{
	return Helper_GetMarketplaceOpsAccSheetID(false, false, 0);
}

int PrcssrMarketplaceInterchange::ReloadGuaPack()
{
	int    ok = 1;
	PPObjGlobalUserAcc gua_obj;
	THROW(gua_obj.GetPacket(GuaPack.Rec.ID, &GuaPack) > 0);
	CATCHZOK
	return ok;
}

int PrcssrMarketplaceInterchange::Init(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	PPObjGlobalUserAcc gua_obj;
	MarketplaceInterchangeFilt temp_filt;
	State &= ~stInited;
	ZDELETE(P_Ifc);
	THROW(temp_filt.IsA(pBaseFilt));
	temp_filt = *static_cast<const MarketplaceInterchangeFilt *>(pBaseFilt);
	THROW(temp_filt.GuaID); // @todo @err
	THROW(gua_obj.GetPacket(temp_filt.GuaID, &GuaPack) > 0);
	{
		int rcr = ReadConfig(&Cfg);
		THROW(rcr);
		if(rcr < 0) {
			Cfg.OrderOpID = GetOrderOpID();
			Cfg.SalesOpID = GetSaleOpID();
		}
	}
	if(GuaPack.Rec.ServiceIdent == PPGLS_WILDBERRIES) {
		PPID   mp_psn_id = 0;
		P_Ifc = new PPMarketplaceInterface_Wildberries(*this);
		THROW_SL(P_Ifc);
		const int gmppr = P_Ifc->GetMarketplacePerson(&mp_psn_id, 1);
		THROW(P_Ifc->Init(temp_filt.GuaID));
	}
	else {
		CALLEXCEPT(); // @todo @err
	}
	State |= stInited;
	CATCHZOK
	return ok;
}

int PrcssrMarketplaceInterchange::Run()
{
	int    ok = -1;
	THROW(State & stInited); // @todo @err
	THROW(P_Ifc); // @todo @err
	PPWait(1);
	{
		PPMarketplaceInterface_Wildberries * p_ifc_wb = static_cast<PPMarketplaceInterface_Wildberries *>(P_Ifc);
		Logger.LogString(PPTXT_MP_PROCESSING_RECEIPTS, 0);
		p_ifc_wb->ImportReceipts();
		Logger.LogString(PPTXT_MP_PROCESSING_ORDERS, 0);
		p_ifc_wb->ImportOrders();
		Logger.LogString(PPTXT_MP_PROCESSING_SALES, 0);
		p_ifc_wb->ImportSales();
		Logger.LogString(PPTXT_MP_PROCESSING_FINTRANSACTIONS, 0);
		p_ifc_wb->ImportFinancialTransactions();
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

int DoMarketplaceInterchange()
{
	MarketplaceInterchangeFilt * pFilt = 0; // @stub
	int    ok = -1;
	PrcssrMarketplaceInterchange prcssr;
	if(pFilt) {
		if(prcssr.Init(pFilt) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
	}
	else {
		MarketplaceInterchangeFilt param;
		prcssr.InitParam(&param);
		if(prcssr.EditParam(&param) > 0)
			if(prcssr.Init(&param) && prcssr.Run())
				ok = 1;
			else
				ok = PPErrorZ();
	}
	return ok;
}

int CreateMarketplaceAccSheet()
{
	/*
			// Возмещение издержек по перевозке/по складским операциям с товаром
			// Логистика
			// Пересчет платной приемки
			// Продажа
			// Удержание
			// Хранение
	*/
	// MARKETPLACE-OPS символ таблицы статей
	int    ok = 1;
	PPID   acs_id = 0;
	PPObjAccSheet acs_obj;
	/*{
		PPAccSheet2 acs_rec;
		if(!acs_id) {
			MEMSZERO(acs_rec);
			STRNSCPY(acs_rec.Name, pk_rec.Name);
			acs_rec.Assoc = PPOBJ_PERSON;
			acs_rec.ObjGroup = person_kind;
			acs_rec.Flags = ACSHF_AUTOCREATART;
			THROW(p_ref->AddItem(PPOBJ_ACCSHEET, &acs_id, &acs_rec, 0));
		}
	}*/
	return ok;
}
//
//
//
/*
Показатели продаж маркетплейса, необходимые для анализа:

% брака кабинета
% брака, шт
% Выкупа
% выполнения плана по МАРЖЕ
% компенсации брака от себеса
??? Индекс локализации, %
??? КТР (коэффициент)
xxx Контент всего, руб
xxx Контент, руб
xxx Прочие расходы, %
xxx Прочие расходы, руб
xxx Расход всего, руб
xxx Расходы на закуп товара, руб
xxx Реклама и маркетинг, %
xxx Реклама и маркетинг, руб
xxx Специалисты, руб
xxx Фотосессии, руб
Брак компенсация, руб
Внешний трафик, в т.ч. блогеры, руб
Внешний трафик, руб
Возвраты (не выкуп), шт
Всего к перечислению, руб - проверка
Всего начислено, в т.ч. комиссия, руб
Выручка к перечислению ФАКТ, руб
Выручка ПЛАН без комиссии, руб
Выручка, руб
Доплата за разгрузку, руб
Кол-во брака, шт
Комиссия проверка (ручная)
Комиссия, %
Комиссия, руб
Компенсация брака, руб
Маржа за минусом всех расх., %
Маржа за минусом всех расх., руб
Маржа, руб
Маржа, руб/ед
Маржа,%
Переменные расходы, руб
Платная приемка, руб
Премиум-пакет, руб
Продажи, шт
Продвижение, руб
Прочие + хранение + реклама, %
Прочие + хранение + реклама, руб
Реклама WB, руб
Самовыкупы, руб
Самовыкупы, шт
Себестоимость брака, руб
Себестоимость закупа товара, руб
СПП, %
СПП, руб
Ср.маржа на 1 ед.товара, руб
Ср.себес на 1 ед. товара, руб
Ср.чек на 1 ед. товара, руб
Транзит, руб
Утилизация ед, шт
Утилизация, руб
Хранение WB, руб
Хранение ФФ, руб
Хранение, %
Хранение, руб
Штрафы, %
Штрафы, руб
Логистика, руб
Логистика, руб в т.ч.:
	Повышенная логистика, руб
	Обратная логистика, руб
	Обратная логистика, %
	Ср.логистика на 1 ед. товара, руб
*/