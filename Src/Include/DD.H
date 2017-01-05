
//
// This file was compiled from d:\Papyrus\Src\Ppaldd\dd.ald by PPALDDC.EXE
//

class PPALDD_DataObject : public PPALDD {
public:
	PPALDD_DataObject(long resId = 1, PPALDD ** ppA = 0);
	~PPALDD_DataObject();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[48];
	} Head_data;
};

class PPALDD_Global : public PPALDD {
public:
	PPALDD_Global(long resId = 2, PPALDD ** ppA = 0);
	~PPALDD_Global();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char MainOrgName[48];
		char ShortOrgName[48];
		char MainOrgMemo[128];
		char MainOrgExtName[128];
		char MainOrgBankAcc[128];
		char MainOrgBnkName[128];
		char MainOrgBnkAccN[32];
		char Director[48];
		char Cashier[48];
		char Accountant[48];
		char Address[64];
		char Phone[48];
		char INN[32];
		char KPP[12];
		char OKPO[16];
		int16 IsPrivateEnt;
		LDATE CurDate;
	} Head_data;
};

class PPALDD_User : public PPALDD {
public:
	PPALDD_User(long resId = 3, PPALDD ** ppA = 0);
	~PPALDD_User();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[32];
	} Head_data;
};

class PPALDD_DBDiv : public PPALDD {
public:
	PPALDD_DBDiv(long resId = 4, PPALDD ** ppA = 0);
	~PPALDD_DBDiv();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[30];
	} Head_data;
};

class PPALDD_Currency : public PPALDD {
public:
	PPALDD_Currency(long resId = 5, PPALDD ** ppA = 0);
	~PPALDD_Currency();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[32];
		char Symb[16];
	} Head_data;
};

class PPALDD_CurRateType : public PPALDD {
public:
	PPALDD_CurRateType(long resId = 6, PPALDD ** ppA = 0);
	~PPALDD_CurRateType();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[32];
	} Head_data;
};

class PPALDD_AccSheet : public PPALDD {
public:
	PPALDD_AccSheet(long resId = 7, PPALDD ** ppA = 0);
	~PPALDD_AccSheet();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[48];
		int32 LinkObjType;
	} Head_data;
};

class PPALDD_OprKind : public PPALDD {
public:
	PPALDD_OprKind(long resId = 8, PPALDD ** ppA = 0);
	~PPALDD_OprKind();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[48];
		int32 AccSheetID;
		int32 AccSheet2ID;
		int32 Flags;
		int16 fNeedPayment;
		int16 fProfitable;
		int16 fReckon;
	} Head_data;
};

#ifdef __WIN32__
class PPALDD_OprKindList : public PPALDD {
public:
	PPALDD_OprKindList(long resId = 9, PPALDD ** ppA = 0);
	~PPALDD_OprKindList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int16 FltReckon;
		int16 FltNeedPayment;
		int16 FltProfitable;
		int16 FltPassive;
		char FltOpTypeName[64];
		int32 FltOpType;
		int32 FltAccSheet;
	} Head_data;
	struct IT_default {
		int32 OpID;
		char OpType[64];
		char FlagsMnemonics[32];
		char Template[84];
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_CurRateView : public PPALDD {
public:
	PPALDD_CurRateView(long resId = 10, PPALDD ** ppA = 0);
	~PPALDD_CurRateView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltCurID;
		int32 FltBaseCurID;
		int32 FltRateTypeID;
		LDATE FltBeg;
		LDATE FltEnd;
	} Head_data;
	struct IT_default {
		LDATE Dt;
		int32 CurID;
		int32 BaseCurID;
		int32 RateTypeID;
		double Rate;
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_Country : public PPALDD {
public:
	PPALDD_Country(long resId = 11, PPALDD ** ppA = 0);
	~PPALDD_Country();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[30];
		char Abbr[20];
		char Phone[20];
		int16 fNativeLand;
	} Head_data;
};

class PPALDD_Region : public PPALDD {
public:
	PPALDD_Region(long resId = 12, PPALDD ** ppA = 0);
	~PPALDD_Region();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[48];
		char Abbr[20];
		char Phone[20];
	} Head_data;
};

class PPALDD_City : public PPALDD {
public:
	PPALDD_City(long resId = 13, PPALDD ** ppA = 0);
	~PPALDD_City();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 CountryID;
		int32 RegionID;
		char Name[30];
		char Abbr[20];
		char Phone[20];
	} Head_data;
};

class PPALDD_Location : public PPALDD {
public:
	PPALDD_Location(long resId = 14, PPALDD ** ppA = 0);
	~PPALDD_Location();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Code[10];
		int32 CityID;
		char Name[48];
		char Addr[128];
	} Head_data;
};

class PPALDD_PersonKind : public PPALDD {
public:
	PPALDD_PersonKind(long resId = 15, PPALDD ** ppA = 0);
	~PPALDD_PersonKind();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[48];
	} Head_data;
};

class PPALDD_RegisterType : public PPALDD {
public:
	PPALDD_RegisterType(long resId = 16, PPALDD ** ppA = 0);
	~PPALDD_RegisterType();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[48];
		char Symb[20];
		int32 KindID;
		int32 RegOrgKindID;
		int32 Flags;
	} Head_data;
};

class PPALDD_TagType : public PPALDD {
public:
	PPALDD_TagType(long resId = 17, PPALDD ** ppA = 0);
	~PPALDD_TagType();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[30];
	} Head_data;
};

class PPALDD_PersonStatus : public PPALDD {
public:
	PPALDD_PersonStatus(long resId = 18, PPALDD ** ppA = 0);
	~PPALDD_PersonStatus();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[48];
	} Head_data;
};

class PPALDD_PersonCat : public PPALDD {
public:
	PPALDD_PersonCat(long resId = 19, PPALDD ** ppA = 0);
	~PPALDD_PersonCat();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[48];
	} Head_data;
};

class PPALDD_Person : public PPALDD {
public:
	PPALDD_Person(long resId = 20, PPALDD ** ppA = 0);
	~PPALDD_Person();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 LocID;
		int32 RLocID;
		int32 Status;
		int32 Category;
		char Name[48];
		char Memo[128];
		char Passport[48];
		int16 fNativeLand;
	} Head_data;
};

class PPALDD_PersonReq : public PPALDD {
public:
	PPALDD_PersonReq(long resId = 21, PPALDD ** ppA = 0);
	~PPALDD_PersonReq();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[48];
		char ExtName[128];
		char Memo[128];
		char Passport[64];
		char Address[64];
		char RAddr[64];
		char BankAccount[128];
		char BankCity[48];
		char OKPO[32];
		char OKONF[32];
		char INN[32];
		char KPP[12];
		char Phone[32];
		int32 SrchRegTypeID;
		char SrchCode[32];
		int32 Flags;
	} Head_data;
};

class PPALDD_Warehouse : public PPALDD {
public:
	PPALDD_Warehouse(long resId = 22, PPALDD ** ppA = 0);
	~PPALDD_Warehouse();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Code[10];
		int32 RspnsPersonID;
		int32 CityID;
		char Name[48];
		char Addr[128];
	} Head_data;
};

class PPALDD_Article : public PPALDD {
public:
	PPALDD_Article(long resId = 23, PPALDD ** ppA = 0);
	~PPALDD_Article();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 SheetID;
		int32 LinkObjType;
		int32 PersonID;
		int32 WhID;
		int32 Number;
		char Name[48];
		int16 Group;
		int16 Closed;
		int16 Stop;
	} Head_data;
};

#ifdef __WIN32__
class PPALDD_ArticleView : public PPALDD {
public:
	PPALDD_ArticleView(long resId = 24, PPALDD ** ppA = 0);
	~PPALDD_ArticleView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		char FltAccSheetName[48];
	} Head_data;
	struct IT_default {
		int32 ArticleID;
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_SCardSeries : public PPALDD {
public:
	PPALDD_SCardSeries(long resId = 25, PPALDD ** ppA = 0);
	~PPALDD_SCardSeries();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[30];
		double PDis;
		double Overdraft;
		LDATE IssueDate;
		LDATE Expiry;
		int32 Flags;
		int16 fCredit;
	} Head_data;
};

class PPALDD_SCard : public PPALDD {
public:
	PPALDD_SCard(long resId = 26, PPALDD ** ppA = 0);
	~PPALDD_SCard();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 SeriesID;
		int32 OwnerReqID;
		char Code[24];
		LDATE IssueDate;
		LDATE Expiry;
		double PDis;
		double Overdraft;
		double Debit;
		double Credit;
		double Rest;
		int32 Flags;
		int16 fCredit;
		int16 fClosed;
		int16 fInherited;
	} Head_data;
};

class PPALDD_QuotKind : public PPALDD {
public:
	PPALDD_QuotKind(long resId = 27, PPALDD ** ppA = 0);
	~PPALDD_QuotKind();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 OpID;
		char Name[30];
		double Discount;
		int16 fAbsDiscount;
	} Head_data;
};

class PPALDD_Transport : public PPALDD {
public:
	PPALDD_Transport(long resId = 28, PPALDD ** ppA = 0);
	~PPALDD_Transport();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 TrType;
		char Name[64];
		char Code[16];
		char TrailerCode[16];
		char ModelName[30];
		int32 OwnerID;
		int32 CountryID;
		int32 CaptainID;
	} Head_data;
};

class PPALDD_Brand : public PPALDD {
public:
	PPALDD_Brand(long resId = 29, PPALDD ** ppA = 0);
	~PPALDD_Brand();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[64];
		int32 OwnerID;
	} Head_data;
};

class PPALDD_BillStatus : public PPALDD {
public:
	PPALDD_BillStatus(long resId = 30, PPALDD ** ppA = 0);
	~PPALDD_BillStatus();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[20];
		char Symb[16];
		int32 Flags;
		int32 Rank;
	} Head_data;
};

class PPALDD_Bill : public PPALDD {
public:
	PPALDD_Bill(long resId = 31, PPALDD ** ppA = 0);
	~PPALDD_Bill();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 ArticleID;
		int32 Object2ID;
		int32 LocID;
		int32 OprKindID;
		int32 CurID;
		int32 StatusID;
		LDATE Dt;
		char Code[24];
		char Memo[128];
		int32 Flags;
		int16 fNeedPayment;
		int16 fPayout;
		int16 fWL;
		int16 Pad1;
		double Amount;
		double CostSum;
		double PriceSum;
		double Discount;
		double GenCostSum;
		double GenPriceSum;
		double GenDiscount;
		double ManPctDscnt;
		double ManAbsDscnt;
		double Payment;
		double Debt;
		double CVat;
		double PVat;
		double SalesTax;
		double Excise;
		double Vat1Rate;
		double Vat1Sum;
		double Vat2Rate;
		double Vat2Sum;
		LDATE LastPayDate;
		int32 PayerID;
		int32 AgentID;
		int32 ExtPriceQkID;
		char InvoiceCode[24];
		LDATE InvoiceDate;
		char PaymBillCode[10];
		int16 Pad2;
		LDATE PaymBillDate;
		char FreightCode[20];
		int32 TranspID;
		int32 CaptainID;
		int32 VesselsAgentID;
		int32 PortOfLoading;
		int32 PortOfDschrg;
		int32 DlvrLocID;
		LDATE IssueDate;
		LDATE ArrivalDate;
		int16 NmbOrigsBsL;
		int16 Pad3;
		LDATE RcStart;
		LDATE RcFinish;
		int32 RcFlags;
		int16 fRcPctCharge;
		int16 fRcClosed;
		int16 RcCycle;
		int16 RcDayOffs;
		double RcPercent;
		double RcPartAmount;
		char RcCycleText[16];
	} Head_data;
};

class PPALDD_AmountType : public PPALDD {
public:
	PPALDD_AmountType(long resId = 32, PPALDD ** ppA = 0);
	~PPALDD_AmountType();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[48];
		char Symb[20];
		int32 Flags;
		int32 Tax;
		int32 TaxRate;
	} Head_data;
};

class PPALDD_Unit : public PPALDD {
public:
	PPALDD_Unit(long resId = 33, PPALDD ** ppA = 0);
	~PPALDD_Unit();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[24];
	} Head_data;
};

class PPALDD_GoodsType : public PPALDD {
public:
	PPALDD_GoodsType(long resId = 34, PPALDD ** ppA = 0);
	~PPALDD_GoodsType();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[30];
		int32 CVatAmtID;
		int32 CostAmtID;
		int32 PriceAmtID;
		int32 DscntAmtID;
		int32 Flags;
		int16 fUnlimited;
		int16 fRplcCost;
		int16 fRplcPrice;
		int16 fRplcDscnt;
		int16 fPriceInclDis;
		int16 fExclAmount;
		int16 fAutoCompl;
		int16 fAllowZeroPrice;
		int16 fAsset;
	} Head_data;
};

class PPALDD_GoodsTaxGrp : public PPALDD {
public:
	PPALDD_GoodsTaxGrp(long resId = 35, PPALDD ** ppA = 0);
	~PPALDD_GoodsTaxGrp();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[30];
		double VAT;
		double Excise;
		double SalesTax;
		char OrderStr[16];
		int16 fAbsExcise;
	} Head_data;
};

class PPALDD_GoodsClass : public PPALDD {
public:
	PPALDD_GoodsClass(long resId = 36, PPALDD ** ppA = 0);
	~PPALDD_GoodsClass();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[32];
		int32 DefGrpID;
		char DefGrpName[64];
		int32 DefUnitID;
		int32 DefPhUnitID;
		int32 DefTaxGrpID;
		int32 DefGoodsTypeID;
		int32 Flags;
		int32 DynGenMask;
		char PropKindNam[32];
		int32 PropKindObj;
		char PropKindObjN[32];
		char PropGradNam[32];
		int32 PropGradObj;
		char PropGradObjN[32];
		char PropAddNam[32];
		int32 PropAddObj;
		char PropAddObjN[32];
		char DimXName[32];
		int16 DimXScale;
		char DimXValues[48];
		char DimYName[32];
		int16 DimYScale;
		char DimYValues[48];
		char DimZName[32];
		int16 DimZScale;
		char DimZValues[48];
		char NameConv[64];
		char PhUPerU_Form[48];
		char TaxMult_Form[48];
		char Package_Form[48];
	} Head_data;
};

#ifdef __WIN32__
class PPALDD_GoodsClassView : public PPALDD {
public:
	PPALDD_GoodsClassView(long resId = 37, PPALDD ** ppA = 0);
	~PPALDD_GoodsClassView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
	} Head_data;
	struct IT_default {
		int32 GcID;
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_AssetWrOffGrp : public PPALDD {
public:
	PPALDD_AssetWrOffGrp(long resId = 38, PPALDD ** ppA = 0);
	~PPALDD_AssetWrOffGrp();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[30];
		char Code[14];
		int32 WrOffType;
		int32 WrOffTerm;
		int32 Flags;
	} Head_data;
};

class PPALDD_GoodsGroup : public PPALDD {
public:
	PPALDD_GoodsGroup(long resId = 39, PPALDD ** ppA = 0);
	~PPALDD_GoodsGroup();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 TypeID;
		int32 TaxGrpID;
		char Name[64];
	} Head_data;
};

class PPALDD_Goods : public PPALDD {
public:
	PPALDD_Goods(long resId = 40, PPALDD ** ppA = 0);
	~PPALDD_Goods();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 UnitID;
		int32 PhUnitID;
		int32 GroupID;
		int32 TypeID;
		int32 ManufID;
		int32 TaxGrpID;
		int32 WrOffGrpID;
		int32 BrandID;
		int32 Flags;
		double PhPerUnit;
		double Brutto;
		int32 Length;
		int32 Width;
		int32 Height;
		double MinStock;
		double Package;
		int16 ExpiryPeriod;
		int16 Pad1;
		char Name[64];
		char SingleBarCode[32];
		char Storage[128];
		char Standard[48];
		char Ingred[128];
		char Energy[128];
		char Usage[128];
		char LabelName[64];
		char OKOF[24];
		double DimX;
		double DimY;
		double DimZ;
		int32 KindID;
		char KindText[48];
		int32 GradeID;
		char GradeText[48];
		int32 AddObjID;
		char AddObjText[48];
	} Head_data;
};

class PPALDD_QualityCert : public PPALDD {
public:
	PPALDD_QualityCert(long resId = 41, PPALDD ** ppA = 0);
	~PPALDD_QualityCert();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 PersonID;
		char Code[26];
		int16 Pad1;
		char Blank[16];
		char InnerCode[10];
		int16 Pad2;
		char GoodsName[48];
		char ManufName[30];
		int16 Pad3;
		char ProduceDate[20];
		int16 Passive;
		int16 Pad4;
		LDATE InitDate;
		LDATE Expiry;
		char Etc[32];
	} Head_data;
};

class PPALDD_Lot : public PPALDD {
public:
	PPALDD_Lot(long resId = 42, PPALDD ** ppA = 0);
	~PPALDD_Lot();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 GoodsID;
		int32 LocID;
		int32 SupplID;
		int32 BillID;
		int32 QCertID;
		int32 InTaxGrpID;
		LDATE Dt;
		LDATE CloseDt;
		LDATE Expiry;
		int16 Closed;
		int16 Pad1;
		int32 Flags;
		double UnitPerPack;
		double Qtty;
		double Rest;
		double Cost;
		double Price;
		char Serial[28];
		char CLB[28];
	} Head_data;
};

#ifdef __WIN32__
class PPALDD_GoodsView : public PPALDD {
public:
	PPALDD_GoodsView(long resId = 43, PPALDD ** ppA = 0);
	~PPALDD_GoodsView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltGrpID;
		int32 FltManufID;
		int32 FltUnitID;
		int32 FltPhUnitID;
		int32 FltGoodsTypeID;
		int32 FltBrandID;
		int32 FltTaxGrpID;
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		char BarCode[32];
		char StrucType[14];
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_GoodsRest : public PPALDD {
public:
	PPALDD_GoodsRest(long resId = 44, PPALDD ** ppA = 0);
	~PPALDD_GoodsRest();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltLocID;
		int32 FltSupplID;
		int32 FltGoodsGrpID;
		LDATE Dt;
		LDATE FltPrgnBeg;
		LDATE FltPrgnEnd;
		int32 PriceType;
		int32 InPacks;
		int32 Flags;
		int16 fNullRest;
		int16 fCalcOrder;
		int16 fGroupByGenGoods;
		int16 fNoZeroOrderOnly;
		int16 fEachLocation;
		int16 fUnderMinStock;
	} Head_data;
	struct IT_default {
		enum {
			OrdGoodsName = 1,
			OrdGoodsGroup,
		};
		int32 GoodsID;
		int32 LocID;
		int32 AltNo;
		double Rest;
		double PhRest;
		double MinStock;
		double Cost;
		double Price;
		double UnitsPerPack;
		double Order;
		char GoodsGrpName[128];
		double SCost;
		double SPrice;
		double PctAddedVal;
		char CRest[16];
		double Prognosis;
		double RestInDays;
		double Deficit;
		double DraftRcpt;
		char GoodsName[64];
		int32 SubstAsscCount;
	} IT_default_data;
};

#ifdef __WIN32__
class PPALDD_GoodsTaxAnlz : public PPALDD {
public:
	PPALDD_GoodsTaxAnlz(long resId = 46, PPALDD ** ppA = 0);
	~PPALDD_GoodsTaxAnlz();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltLocID;
		int32 FltSupplID;
		int32 FltGoodsGrpID;
		int32 FltOpID;
		LDATE FltBeg;
		LDATE FltEnd;
		LDATE FltLotsBeg;
		LDATE FltLotsEnd;
		char Prd[32];
		char LotsPrd[32];
		int16 FltCycle;
		int16 FltNumCycles;
		int32 FltFlags;
		int16 fNozeroExciseOnly;
		int16 fLabelOnly;
		int16 fByGroups;
		int16 fDayly;
		double VATRate1;
		double VATRate2;
		double VATRate3;
		double VATRate4;
		double VATRate5;
		double VATSumIn1;
		double VATSumIn2;
		double VATSumIn3;
		double VATSumIn4;
		double VATSumIn5;
		double VATSumOut1;
		double VATSumOut2;
		double VATSumOut3;
		double VATSumOut4;
		double VATSumOut5;
		int16 IncomeMethod;
	} Head_data;
	struct IT_default {
		LDATE Dt;
		char CycleText[32];
		int32 GoodsID;
		int32 GoodsGrpID;
		int32 GTaxGrpID;
		int32 LTaxGrpID;
		char Name[48];
		char GrpName[128];
		char TaxStr[48];
		double Qtty;
		double PhQtty;
		double TrnovrCost;
		double TrnovrPrice;
		double Income;
		int16 fIsVatFree;
		int16 fToggleSTax;
		double ExciseSum;
		double C_VATSum;
		double C_STaxSum;
		double IncVATSum;
		double VATSum;
		double STaxSum;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_GoodsOpAnlz : public PPALDD {
public:
	PPALDD_GoodsOpAnlz(long resId = 47, PPALDD ** ppA = 0);
	~PPALDD_GoodsOpAnlz();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltLocID;
		int32 FltOpID;
		int32 FltObjectID;
		int32 FltSupplID;
		int32 FltAgentID;
		int32 FltSupplAgentID;
		int32 FltGoodsGrpID;
		LDATE FltBeg;
		LDATE FltEnd;
		char FltOpTxt[64];
		int32 FltOpGrpID;
		int32 FltFlags;
		int16 fDiffByPrice;
		int16 fDiffByNetPrice;
		int16 fIntrReval;
		int16 fLabelOnly;
		int16 fPriceWithoutExcise;
		int16 fCalcRest;
		int16 fEachLocation;
		int16 Sgg;
		char SggTxt[32];
		LDATE RestCalcDate;
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		int32 GoodsGrpID;
		int32 Contragnt;
		int32 LocID;
		int16 InOutTag;
		char GoodsName[64];
		char GrpName[128];
		double UnitPerPack;
		double Qtty;
		double PhQtty;
		double OldCost;
		double OldPrice;
		double Cost;
		double Price;
		double SumCost;
		double SumPrice;
		double Income;
		double PctVal;
		double Rest;
		char CQtty[16];
		char CRest[16];
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_GoodsOpAnlzCmp : public PPALDD {
public:
	PPALDD_GoodsOpAnlzCmp(long resId = 48, PPALDD ** ppA = 0);
	~PPALDD_GoodsOpAnlzCmp();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltLocID;
		int32 FltOpID;
		int32 FltObjectID;
		int32 FltSupplID;
		int32 FltAgentID;
		int32 FltSupplAgentID;
		int32 FltGoodsGrpID;
		LDATE FltBeg;
		LDATE FltEnd;
		LDATE CmpFltBeg;
		LDATE CmpFltEnd;
		char FltOpTxt[64];
		int32 FltOpGrpID;
		int32 FltFlags;
		int16 fDiffByPrice;
		int16 fDiffByNetPrice;
		int16 fIntrReval;
		int16 fLabelOnly;
		int16 fPriceWithoutExcise;
		int16 fCalcRest;
		int16 fEachLocation;
		int16 Sgg;
		char SggTxt[32];
		LDATE RestCalcDate;
		LDATE CmpRestCalcDate;
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		int32 GoodsGrpID;
		int32 SubstArID;
		int32 SubstPsnID;
		int32 SubstLocID;
		int32 LocID;
		char GoodsName[64];
		double Qtty;
		double SumCost;
		double SumPrice;
		double Income;
		double Rest;
		double RestCostSum;
		double RestPriceSum;
		double CmpQtty;
		double CmpSumCost;
		double CmpSumPrice;
		double CmpIncome;
		double CmpRest;
		double CmpRestCostSum;
		double CmpRestPriceSum;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_PriceListData : public PPALDD {
public:
	PPALDD_PriceListData(long resId = 50, PPALDD ** ppA = 0);
	~PPALDD_PriceListData();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 LocID;
		int32 SupplID;
		int32 FltQuotKindID;
		int32 FltManufID;
		int32 FltGoodsGrpID;
		int32 CfgPriceQuot1;
		int32 CfgPriceQuot2;
		int32 CfgPriceQuot3;
		char PLMemo[128];
		int32 PListID;
		LDATE Date;
		double FltAddPct;
		LDATE FltLotBeg;
		LDATE FltLotEnd;
		int16 fPresentOnly;
		int16 fByQuot;
		int16 fExcludeGoodsGrp;
		int16 fPlistNew;
	} Head_data;
	struct IT_default {
		int32 __HLID;
		int32 QCertID;
		int32 GoodsGrpID;
		int32 GoodsID;
		int32 UnitID;
		int32 ManufID;
		int32 QuotKindID;
		int32 LN;
		int32 LineNo;
		int32 GoodsCode;
		LDATE Expiry;
		double UnitPerPack;
		double Price;
		double AddPrice1;
		double AddPrice2;
		double AddPrice3;
		double Rest;
		char GoodsName[64];
		char ExtGroupName[128];
		char Memo[128];
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_GoodsLabel : public PPALDD {
public:
	PPALDD_GoodsLabel(long resId = 52, PPALDD ** ppA = 0);
	~PPALDD_GoodsLabel();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);

	struct Head {
		int32 Glb;
		int32 HdrGoodsID;
		int32 LocID;
		int32 NumCopies;
		int32 nn;
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		LDATE Expiry;
		double Price;
		double UnitPerPack;
		char PrnBarcode[32];
	} IT_default_data;
};

class PPALDD_GoodsBillBase : public PPALDD {
public:
	PPALDD_GoodsBillBase(long resId = 53, PPALDD ** ppA = 0);
	~PPALDD_GoodsBillBase();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);

	struct Head {
		int32 Glb;
		int32 OprKindID;
		int32 ExpendFlag;
		int32 BillID;
		int32 LinkBillID;
		int32 CurID;
		int32 ObjectID;
		int32 PayerID;
		int32 AgentID;
		int32 LocID;
		int32 DlvrID;
		int32 DlvrReq;
		int32 DlvrLocID;
		int32 RcvrID;
		int32 RcvrReq;
		int32 RcvrLocID;
		int32 ConsignorReq;
		int32 ConsigneeReq;
		LDATE Dt;
		LDATE PayDt;
		int32 Flags;
		int16 fSupplIsVatExempt;
		int16 fShortMainOrg;
		int16 fMergeSameGoods;
		int16 RowOrder;
		char Code[24];
		char Memo[128];
		double TotalSum;
		double TotalSalesTax;
		double TotalVAT;
		double TotalQtty;
		double TotalPhQtty;
		double TotalPacks;
		double TotalBrutto;
		int32 TotalGoodsLines;
		int32 TotalGoodsNames;
		char TxtManualDscnt[64];
		double VATRate1;
		double VATRate2;
		double VATSum1;
		double VATSum2;
	} Head_data;
	struct IT_default {
		int32 nn;
		int32 GoodsID;
		int32 LotID;
		char CLB[28];
		char GoodsGrpName[64];
		double Cost;
		double Price;
		double Discount;
		double MainPrice;
		double CurPrice;
		double CurSum;
		double Qtty;
		double UnitsPerPack;
		double FullPack;
		char CQtty[32];
		double VATRate;
		double VATSum;
		double ExcRate;
		double ExcSum;
		double STRate;
		double STSum;
		double ExtPrice;
	} IT_default_data;
};

class PPALDD_GoodsBillQCert : public PPALDD {
public:
	PPALDD_GoodsBillQCert(long resId = 54, PPALDD ** ppA = 0);
	~PPALDD_GoodsBillQCert();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);

	struct Head {
		int32 Glb;
		int32 BillID;
		int32 LocID;
		int32 DlvrReq;
		int32 DlvrLocID;
		int32 RcvrReq;
		int32 RcvrLocID;
		int32 ExpendFlag;
		LDATE Dt;
		char Code[10];
		int16 fShortMainOrg;
	} Head_data;
	struct IT_default {
		int32 nn;
		int32 GoodsID;
		int32 LotID;
		int32 QualityCertID;
		double Qtty;
		char LExpiry[20];
	} IT_default_data;
};

class PPALDD_GoodsReval : public PPALDD {
public:
	PPALDD_GoodsReval(long resId = 55, PPALDD ** ppA = 0);
	~PPALDD_GoodsReval();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);

	struct Head {
		int32 Glb;
		int32 ArticleID;
		int32 LocID;
		char Code[24];
		LDATE Dt;
		char Memo[128];
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		int32 nn;
		double OldCost;
		double NewCost;
		double OldPrice;
		double NewPrice;
		double Quantity;
	} IT_default_data;
};

class PPALDD_GoodsBillModif : public PPALDD {
public:
	PPALDD_GoodsBillModif(long resId = 56, PPALDD ** ppA = 0);
	~PPALDD_GoodsBillModif();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);

	struct Head {
		int32 Glb;
		int32 OprKindID;
		int32 BillID;
		int32 CurID;
		int32 ObjectID;
		int32 LocID;
		int32 CntragntID;
		int32 CntragntReq;
		LDATE Dt;
		int32 Flags;
		int16 fShortMainOrg;
		char Code[24];
		char Memo[128];
		double ExpendQtty;
		double ExpendPacks;
		double ExpendCost;
		double ExpendPrice;
		double ExpendSum;
		double ExpendVAT;
		double ExpendExcise;
		double ReceiptQtty;
		double ReceiptPacks;
		double ReceiptCost;
		double ReceiptPrice;
		double ReceiptSum;
		double ReceiptVAT;
		double ReceiptExcise;
	} Head_data;
	struct IT_default {
		int32 Iter_NN;
		int32 Sign_NN;
		int32 GoodsID;
		int16 Sign;
		double Cost;
		double Price;
		double MainPrice;
		double CurPrice;
		double CurSum;
		double Qtty;
		double UnitsPerPack;
		double FullPack;
		char CQtty[32];
		double VATRate;
		double VATSum;
		double ExcRate;
		double ExcSum;
	} IT_default_data;
};

class PPALDD_CashOrder : public PPALDD {
public:
	PPALDD_CashOrder(long resId = 57, PPALDD ** ppA = 0);
	~PPALDD_CashOrder();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 Glb;
		int32 ArticleID;
		int32 Article2ID;
		int32 PersonReqID;
		int32 Person2ReqID;
		double Amount;
		char Code[24];
		LDATE Dt;
		char Memo[128];
		int32 CashAc;
		int32 CashSb;
		int32 Ac;
		int32 Sb;
		int32 Ar;
		double VAT;
		double VATRate1;
		double VATSum1;
		double VATRate2;
		double VATSum2;
		double STaxRate;
		double STaxSum;
	} Head_data;
};

#ifdef __WIN32__
class PPALDD_BnkPaymOrder : public PPALDD {
public:
	PPALDD_BnkPaymOrder(long resId = 60, PPALDD ** ppA = 0);
	~PPALDD_BnkPaymOrder();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 Glb;
		int32 PayerID;
		int32 RcvrID;
		int32 PayerBnkID;
		int32 RcvrBnkID;
		int32 BillID;
		LDATE LclDt;
		char LclCode[10];
		char LclMemo[160];
		int32 PayerBnkAccID;
		char PayerBnkCity[48];
		char PayerBIC[24];
		char PayerBnkCorAcc[24];
		char PayerAcc[24];
		int32 RcvrBnkAccID;
		char RcvrBnkCity[48];
		char RcvrBIC[24];
		char RcvrBnkCorAcc[24];
		char RcvrAcc[24];
		int16 BnkPaymMethod;
		int16 BnkQueueing;
		int16 PayerStatus;
		char TxtPayerStatus[6];
		double Amount;
		double VATRate;
		double VATSum;
		int16 fNoAccept;
		char TxmClass[24];
		char TxmOKATO[16];
		char TxmReason[8];
		char TxmPeriod[12];
		char TxmDocNumber[12];
		LDATE TxmDocDate;
		char TxmPaymType[8];
	} Head_data;
};

#endif // __WIN32__
class PPALDD_BillPayPlan : public PPALDD {
public:
	PPALDD_BillPayPlan(long resId = 61, PPALDD ** ppA = 0);
	~PPALDD_BillPayPlan();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);

	struct Head {
		int32 Glb;
		int32 OprKindID;
		int32 BillID;
		int16 fProfitable;
	} Head_data;
	struct IT_default {
		int32 nn;
		LDATE PayDate;
		double Amount;
		double Interest;
		double Rest;
	} IT_default_data;
};

#ifdef __WIN32__
class PPALDD_Lots : public PPALDD {
public:
	PPALDD_Lots(long resId = 62, PPALDD ** ppA = 0);
	~PPALDD_Lots();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltLocID;
		int32 FltSupplID;
		int32 FltGoodsGrpID;
		int32 FltGoodsID;
		LDATE FltBeg;
		LDATE FltEnd;
		LDATE FltExpiryBeg;
		LDATE FltExpiryEnd;
		int32 IsOper;
		LDATE OperLow;
		LDATE OperUpp;
		int16 FltClosedTag;
		int32 FltFlags;
		int16 fWithoutQCert;
		int16 fOrders;
		int16 fCostAbovePrice;
	} Head_data;
	struct IT_default {
		int32 LotID;
		char CQtty[16];
		char CRest[16];
		double BegRest;
		double EndRest;
		double Sales;
		char CBegRest[16];
		char CEndRest[16];
		char CSales[16];
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_LotOps : public PPALDD {
public:
	PPALDD_LotOps(long resId = 63, PPALDD ** ppA = 0);
	~PPALDD_LotOps();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 LotID;
	} Head_data;
	struct IT_default {
		int32 BillID;
		int32 CurID;
		LDATE Dt;
		int16 OprNo;
		double Qtty;
		double Rest;
		double Cost;
		double Price;
		double Discount;
		double CurPrice;
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_BillList : public PPALDD {
public:
	PPALDD_BillList(long resId = 64, PPALDD ** ppA = 0);
	~PPALDD_BillList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltOprKindID;
		int32 FltLocID;
		int32 FltArtID;
		int32 FltPayerID;
		int32 FltAgentID;
		int32 FltCurID;
		LDATE FltBeg;
		LDATE FltEnd;
		double FltMinAmt;
		double FltMaxAmt;
		int32 Flags;
		int16 fAllCurrencies;
		double InDebt;
		double OutDebt;
	} Head_data;
	struct IT_default {
		int32 BillID;
		LDATE LastPaymDate;
		double Debit;
		double Credit;
		double Saldo;
	} IT_default_data;
};

class PPALDD_ContentBList : public PPALDD {
public:
	PPALDD_ContentBList(long resId = 65, PPALDD ** ppA = 0);
	~PPALDD_ContentBList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltOprKindID;
		int32 FltLocID;
		int32 FltPayerID;
		int32 FltCurID;
		LDATE FltBeg;
		LDATE FltEnd;
		int16 fAllCurrencies;
		double TotalVatRate1;
		double TotalVatRate2;
		double TotalVatRate3;
		double TotalVatRate4;
		double TotalVat1;
		double TotalVat2;
		double TotalVat3;
		double TotalVat4;
	} Head_data;
	struct IT_default {
		int32 BillID;
		int32 GoodsID;
		int32 LotID;
		int32 recNo;
		int32 grpNo;
		double Quantity;
		double Cost;
		double Price;
		double CurPrice;
		double VaTax;
		double ExTax;
		double StTax;
		double VatSum;
		double ExtSum;
		double StSum;
	} IT_default_data;
};

class PPALDD_CashBook : public PPALDD {
public:
	PPALDD_CashBook(long resId = 66, PPALDD ** ppA = 0);
	~PPALDD_CashBook();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);

	struct Head {
		int32 Glb;
		double InRest;
		double OutRest;
		LDATE Dt;
		int32 PayCashCnt;
		int32 RcvCashCnt;
	} Head_data;
	struct IT_default {
		char Code[16];
		char CorrAcc[16];
		char CorrAccName[32];
		double Debit;
		double Credit;
		char Memo[128];
	} IT_default_data;
};

class PPALDD_SysJournal : public PPALDD {
public:
	PPALDD_SysJournal(long resId = 67, PPALDD ** ppA = 0);
	~PPALDD_SysJournal();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltUserID;
		int32 FltObjType;
		int32 FltAction;
		LDATE FltBeg;
		LDATE FltEnd;
	} Head_data;
	struct IT_default {
		int32 UserID;
		int32 ObjType;
		int32 ObjID;
		int32 ActionID;
		LDATE Dt;
		LTIME Tm;
		char ActionName[48];
		char ObjName[64];
	} IT_default_data;
};

class PPALDD_DebtTrnovr : public PPALDD {
public:
	PPALDD_DebtTrnovr(long resId = 79, PPALDD ** ppA = 0);
	~PPALDD_DebtTrnovr();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltLocID;
		int32 FltAccSheetID;
		int32 FltArID;
		int32 FltCurID;
		int32 FltCountryID;
		int32 FltRegionID;
		int32 FltCityID;
		int32 FltCatID;
		LDATE FltBeg;
		LDATE FltEnd;
		LDATE FltPaymentBeg;
		LDATE FltPaymentEnd;
		LDATE FltExpiryBeg;
		LDATE FltExpiryEnd;
		LDATE FltExpiry;
		int32 Flags;
		int16 fDebtOnly;
		int16 fNoForwardPaym;
		int16 fLabelOnly;
		int16 fAllCurrencies;
		int16 fExtended;
		int16 fDeliveryAddr;
		int16 fCalcTotalDebt;
		int32 SellOrSuppl;
		int16 CycleKind;
		int16 FltCycle;
		int16 FltNumCycles;
		char CtTitle[48];
	} Head_data;
	struct IT_default {
		int32 BillID;
		int32 ArID;
		int32 CurID;
		int32 RelPersonID;
		int32 ArNo;
		char ArName[80];
		int32 TabID;
		char TabText[32];
		double Debit;
		double Credit;
		double Debt;
		double RPaym;
		double Reckon;
		double RDebt;
		double TDebt;
		double CtVal;
		double _AvgPaym;
		LDATE BillPayDate;
	} IT_default_data;
};

#ifdef __WIN32__
class PPALDD_PaymBillList : public PPALDD {
public:
	PPALDD_PaymBillList(long resId = 81, PPALDD ** ppA = 0);
	~PPALDD_PaymBillList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 LinkBillID;
		int32 Kind;
	} Head_data;
	struct IT_default {
		int32 CurrencyID;
		int32 BillID;
		int32 ItLinkBillID;
		int32 ItRcknBillID;
		char BillNo[24];
		char OpName[42];
		LDATE PaymDt;
		double Amount;
		double Rest;
		char Memo[128];
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_TransferBase : public PPALDD {
public:
	PPALDD_TransferBase(long resId = 82, PPALDD ** ppA = 0);
	~PPALDD_TransferBase();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);

	struct Head {
		int32 Glb;
		LDATE StartDate;
		LDATE EndDate;
		int32 FltLocID;
		int32 FltGoodsID;
		int32 FltOprKindID;
		int32 FltContrAgentID;
	} Head_data;
	struct IT_default {
		int32 BillID;
		int32 LocID;
		int32 GoodsID;
		int32 OprKindID;
		int32 ContrAgentID;
		double Quantity;
		double Rest;
		double Price;
		double Cost;
		double Discount;
	} IT_default_data;
};

class PPALDD_Account : public PPALDD {
public:
	PPALDD_Account(long resId = 83, PPALDD ** ppA = 0);
	~PPALDD_Account();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 Ac;
		int32 Sb;
		int32 Kind;
		int32 Type;
		char Name[48];
		char Code[32];
		int32 Flags;
		int32 CurID;
		int32 AccSheetID;
	} Head_data;
};

class PPALDD_AccRel : public PPALDD {
public:
	PPALDD_AccRel(long resId = 84, PPALDD ** ppA = 0);
	~PPALDD_AccRel();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 AccID;
		int32 ArID;
		int16 Ac;
		int16 Sb;
		int32 Ar;
		int32 CurID;
		char AccName[64];
	} Head_data;
};

#ifdef __WIN32__
class PPALDD_AccountView : public PPALDD {
public:
	PPALDD_AccountView(long resId = 85, PPALDD ** ppA = 0);
	~PPALDD_AccountView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltType;
		int32 FltFlags;
	} Head_data;
	struct IT_default {
		int32 AccID;
		char CurListTxt[48];
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_AccturnList : public PPALDD {
public:
	PPALDD_AccturnList(long resId = 86, PPALDD ** ppA = 0);
	~PPALDD_AccturnList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		LDATE FltBeg;
		LDATE FltEnd;
		int32 FltOprKindID;
		int32 FltCurID;
		double FltMinAmount;
		double FltMaxAmount;
		int16 fAllCurrencies;
		int16 fWlOnly;
		int16 GrpAco;
		int16 FltCycle;
		int16 FltNumCycles;
		int16 Aco;
		int16 FltDbtAc;
		int16 FltDbtSb;
		int32 FltDbtAr;
		int16 FltCrdAc;
		int16 FltCrdSb;
		int32 FltCrdAr;
	} Head_data;
	struct IT_default {
		LDATE Dt;
		char CycleText[32];
		int32 OprNo;
		int32 BillID;
		int16 RByBill;
		int32 DbtAccID;
		int32 DbtRelID;
		int32 CrdAccID;
		int32 CrdRelID;
		int32 CurID;
		double CRate;
		double Amount;
	} IT_default_data;
};

class PPALDD_AccAnlz : public PPALDD {
public:
	PPALDD_AccAnlz(long resId = 87, PPALDD ** ppA = 0);
	~PPALDD_AccAnlz();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 LeafNo;
		LDATE FltBeg;
		LDATE FltEnd;
		LDATE ExpiryDate;
		int16 Aco;
		int32 FltAccID;
		int32 FltArID;
		int32 FltAccRelID;
		int32 FltCurID;
		int16 fAllCurrencies;
		int16 fWlOnly;
		int16 fGroupByCorAcc;
		int16 fTrnovrBySheet;
		int16 fTrnovrBySuppl;
		int16 fSpprZTrnovr;
		int16 fSpprZSaldo;
		int16 CorAco;
		int16 FltCycle;
		int16 FltNumCycles;
		int16 FltOrder;
		int32 FltFlags;
		char FltPeriod[30];
		char FltAccText[96];
		int32 TotalCount;
		double TotalInRest;
		double TotalOutRest;
		int32 TotalDbtCount;
		int32 TotalCrdCount;
	} Head_data;
	struct IT_default {
		LDATE Dt;
		char CycleText[32];
		int32 OprNo;
		int32 BillID;
		int16 RByBill;
		int16 Reverse;
		int32 CorAccID;
		int32 CorAccRelID;
		int32 CurID;
		int32 RelPersonID;
		char CorAccName[48];
		int32 Count;
		double InRest;
		double DbtAmt;
		double CrdAmt;
		double OutRest;
		double GoodsRestAmt;
		char OrderText[32];
	} IT_default_data;
};

#ifdef __WIN32__
class PPALDD_Balance : public PPALDD {
public:
	PPALDD_Balance(long resId = 91, PPALDD ** ppA = 0);
	~PPALDD_Balance();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltCurID;
		LDATE FltBeg;
		LDATE FltEnd;
		int32 FltAccID;
		int32 FltFlags;
		int16 fAllCurrencies;
	} Head_data;
	struct IT_default {
		LDATE Dt;
		int32 AccID;
		int32 CurID;
		double InDbtRest;
		double InCrdRest;
		double DbtTrnovr;
		double CrdTrnovr;
		double OutDbtRest;
		double OutCrdRest;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_VatBook : public PPALDD {
public:
	PPALDD_VatBook(long resId = 92, PPALDD ** ppA = 0);
	~PPALDD_VatBook();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltKind;
		LDATE FltBeg;
		LDATE FltEnd;
		double HVat1Rate;
		double HVat2Rate;
		double HVat3Rate;
		int16 fByPayment;
		int16 fShowLinkedOnly;
		int16 fShowUnlinkedOnly;
		int16 fShowExcluded;
		int32 Counter;
	} Head_data;
	struct IT_default {
		int32 ID;
		int32 DtLineNo;
		int32 LineNo;
		int16 fExcluded;
		int16 fVatFree;
		int16 fFixed;
		int16 fPartPayment;
		int32 Reserve;
		char Code[24];
		char ManufCountry[30];
		char CLB[28];
		LDATE Dt;
		LDATE InvcDate;
		LDATE PaymDate;
		LDATE RcptDate;
		int32 ArticleID;
		int32 PersonID;
		int32 LinkBillID;
		int32 OpID;
		double Amount;
		double Excise;
		double Export;
		double Vat0Amount;
		double Vat1Amount;
		double Vat2Amount;
		double Vat3Amount;
		double Vat1Rate;
		double Vat2Rate;
		double Vat3Rate;
		double Vat1Sum;
		double Vat2Sum;
		double Vat3Sum;
		double VatFreeAmount;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_OpGrouping : public PPALDD {
public:
	PPALDD_OpGrouping(long resId = 93, PPALDD ** ppA = 0);
	~PPALDD_OpGrouping();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltOpID;
		int32 FltLocID;
		int32 FltSupplID;
		int32 FltObjectID;
		int32 FltGoodsGrpID;
		int32 FltGoodsID;
		int32 ExtGoodsTypeID;
		LDATE FltBeg;
		LDATE FltEnd;
		LDATE FltLotBeg;
		LDATE FltLotEnd;
		int16 FltCycle;
		int16 FltNumCycles;
		int32 Flags;
		int16 fWlOnly;
		double TotalQtty;
		double TotalPhQtty;
		double TotalCost;
		double TotalPrice;
		double TotalAmount;
		double TotalExtCost;
		double TotalExtPrice;
		int32 TotalCount;
	} Head_data;
	struct IT_default {
		int32 RecNo;
		int32 GrpNo;
		LDATE Dt;
		char CycleText[32];
		int32 BillID;
		int32 ObjectID;
		int32 OpID;
		int32 GoodsID;
		int32 GoodsTaxGrpID;
		int32 LotTaxGrpID;
		char OpName[48];
		int16 Sign;
		double Qtty;
		double PhQtty;
		double SumCost;
		double SumPrice;
		double SumDiscount;
		double Income;
		double Amount;
		double ExtCost;
		double ExtPrice;
		int32 Count;
		double SumVat;
		double SumExcise;
		double SumSalesTax;
		LDATE BillDt;
		double BillQtty;
		double BillPhQtty;
		double BillAmount;
		double BillCostSum;
		double BillPriceSum;
		double BillDscntSum;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_GoodsStruc : public PPALDD {
public:
	PPALDD_GoodsStruc(long resId = 94, PPALDD ** ppA = 0);
	~PPALDD_GoodsStruc();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 ID;
		int32 Glb;
		int32 GoodsID;
		int32 LocID;
		char Name[30];
		int32 Flags;
		int16 fAllowCompl;
		int16 fAllowDecompl;
		int16 fPartitial;
		int16 fNamed;
		int16 fUncertainPrice;
		double CommDenom;
		int32 ItemsCount;
		double EstPrice;
		int32 VariedPropObjType;
	} Head_data;
	struct IT_default {
		int32 LineNo;
		int32 ItemID;
		int32 LotID;
		int32 Flags;
		int16 fPercentVal;
		int16 fRoundDown;
		int16 fPhUnitVal;
		int16 fUncertainPrice;
		double Netto;
		double Median;
		double PerUnit;
		char CMedian[32];
		double Price;
		double SumPerUnit;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_GoodsStrucList : public PPALDD {
public:
	PPALDD_GoodsStrucList(long resId = 95, PPALDD ** ppA = 0);
	~PPALDD_GoodsStrucList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 ID;
		int32 Glb;
		int32 LocID;
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		int32 ItemID;
		char GoodsName[128];
		char Qtty[30];
		char Type[14];
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_GoodsMov : public PPALDD {
public:
	PPALDD_GoodsMov(long resId = 96, PPALDD ** ppA = 0);
	~PPALDD_GoodsMov();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltLocID;
		int32 FltSupplID;
		int32 FltGoodsGrpID;
		LDATE FltBeg;
		LDATE FltEnd;
		char Period[32];
		int32 Flags;
		int16 fLabelOnly;
		int16 fCostWoVat;
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		char GoodsGrpName[128];
		double PhPerU;
		double InRest_Qtty;
		double InRest_Cost;
		double InRest_Price;
		char InRest_CQtty[32];
		double Rcpt_Qtty;
		double Rcpt_Cost;
		double Rcpt_Price;
		char Rcpt_CQtty[32];
		double _Rcpt_Qtty;
		double _Rcpt_Cost;
		double _Rcpt_Price;
		char _Rcpt_CQtty[32];
		double Rlz_Qtty;
		double Rlz_Cost;
		double Rlz_Price;
		char Rlz_CQtty[32];
		double SRlz_Qtty;
		double SRlz_Cost;
		double SRlz_Price;
		char SRlz_CQtty[32];
		double Expnd_Qtty;
		double Expnd_Cost;
		double Expnd_Price;
		char Expnd_CQtty[32];
		double TRcpt_Qtty;
		double TRcpt_Cost;
		double TRcpt_Price;
		char TRcpt_CQtty[32];
		double TExpnd_Qtty;
		double TExpnd_Cost;
		double TExpnd_Price;
		char TExpnd_CQtty[32];
		double OutRest_Qtty;
		double OutRest_Cost;
		double OutRest_Price;
		char OutRest_CQtty[32];
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_CashNode : public PPALDD {
public:
	PPALDD_CashNode(long resId = 97, PPALDD ** ppA = 0);
	~PPALDD_CashNode();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[32];
		int32 CashType;
		int32 CurSessID;
		LDATE CurDate;
		int32 LocID;
	} Head_data;
};

class PPALDD_CSession : public PPALDD {
public:
	PPALDD_CSession(long resId = 98, PPALDD ** ppA = 0);
	~PPALDD_CSession();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 CashNodeID;
		int32 SuperSessID;
		int32 SessNumber;
		int32 CashNumber;
		LDATE Dt;
		LTIME Tm;
		double Amount;
		double Discount;
		double AggrAmount;
		double AggrRest;
		double WrOffAmount;
		double Banking;
	} Head_data;
};

class PPALDD_CCheck : public PPALDD {
public:
	PPALDD_CCheck(long resId = 99, PPALDD ** ppA = 0);
	~PPALDD_CCheck();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 Code;
		int32 CashID;
		int32 UserID;
		int32 SessID;
		LDATE Dt;
		LTIME Tm;
		double Amount;
		double Discount;
		int32 SCardID;
		char SCardCode[24];
	} Head_data;
};

class PPALDD_CCheckView : public PPALDD {
public:
	PPALDD_CCheckView(long resId = 100, PPALDD ** ppA = 0);
	~PPALDD_CCheckView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltCashNodeID;
		int32 FltCashNumber;
		int32 FltGoodsID;
		int32 FltSCardID;
		double AmountQuant;
		LDATE FltBeg;
		LDATE FltEnd;
		int16 FltGrp;
	} Head_data;
	struct IT_default {
		int32 CheckID;
		int32 GSCardID;
		char GrpngItemText[48];
		double Amount;
		double Discount;
		int32 GCount;
		LDATE GDate;
		double GPctPart;
		double GAmount;
		double GDiscount;
		double GQtty;
	} IT_default_data;
};

#ifdef __WIN32__
class PPALDD_CCheckDetail : public PPALDD {
public:
	PPALDD_CCheckDetail(long resId = 101, PPALDD ** ppA = 0);
	~PPALDD_CCheckDetail();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 CashNodeID;
		int32 AgentID;
		int32 CheckID;
		int32 CheckNumber;
		LDATE CheckDt;
		LTIME CheckTm;
		double CheckAmount;
		double CheckDiscount;
		char CheckSCardCode[24];
		int32 TableNo;
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		double Price;
		double Discount;
		double Quantity;
		double Amount;
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_CSessionView : public PPALDD {
public:
	PPALDD_CSessionView(long resId = 102, PPALDD ** ppA = 0);
	~PPALDD_CSessionView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltSuperSessID;
		int32 FltCashNodeID;
		int32 FltCashNumber;
		LDATE FltBeg;
		LDATE FltEnd;
	} Head_data;
	struct IT_default {
		int32 SessID;
		double WrOffRcptAmount;
		double Income;
		int16 SessIncomplete;
		int32 CheckCount;
		double WORetAmount;
		double RetAmount;
	} IT_default_data;
};

class PPALDD_CSessExc : public PPALDD {
public:
	PPALDD_CSessExc(long resId = 103, PPALDD ** ppA = 0);
	~PPALDD_CSessExc();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltSessID;
		int32 FltCashNodeID;
		int32 FltGoodsGrpID;
		int32 FltGoodsID;
		LDATE FltBeg;
		LDATE FltEnd;
		int16 FltSessNum;
		int16 FltSessIncompl;
		LDATE FltSessBegDt;
		LTIME FltSessBegTm;
		LDATE FltSessEndDt;
		LTIME FltSessEndTm;
		int32 FltFlags;
		int16 fNoZeroAltGoods;
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		int32 AltGoodsID;
		double Qtty;
		double Rest;
		double Price;
		double Sum;
		double RestSum;
		double AltGoodsPrice;
		double AltGoodsQtty;
		double AltGoodsSum;
		double AltDiff;
	} IT_default_data;
};

#ifdef __WIN32__
class PPALDD_TrfrAnlzBase : public PPALDD {
public:
	PPALDD_TrfrAnlzBase(long resId = 104, PPALDD ** ppA = 0);
	~PPALDD_TrfrAnlzBase();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		LDATE FltBeg;
		LDATE FltEnd;
		int32 FltOpID;
		int32 FltLocID;
		int32 FltSupplID;
		int32 FltObjectID;
		int32 FltGoodsGrpID;
		int32 FltGoodsID;
		int32 Flags;
		int16 fLabelOnly;
		int16 Ord;
		int16 Grp;
		int16 Sgg;
		int16 Sgp;
		int16 Sgd;
		int16 CtValKind;
	} Head_data;
	struct IT_default {
		int32 BillID;
		int32 GoodsID;
		int32 OpID;
		int32 LotID;
		int32 LocID;
		int32 DlvrLocID;
		int32 ObjectID;
		int32 _AgentID;
		int32 ContragentID;
		LDATE Dt;
		char GoodsText[64];
		char PersonText[80];
		char DtText[32];
		double Qtty;
		double Rest;
		double PhQtty;
		double Cost;
		double Price;
		double Discount;
		double Amount;
		double SalQtty;
		double SalAmt;
		int32 RptGrpngVal;
		int32 LocCount;
		double CtVal;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_Invent : public PPALDD {
public:
	PPALDD_Invent(long resId = 110, PPALDD ** ppA = 0);
	~PPALDD_Invent();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 BillID;
		int32 Glb;
		int32 Flags;
		int16 fLack;
		int16 fSurplus;
		int16 fAmtVal;
		int16 fPctVal;
		int16 fWrOff;
		int16 fUnwrOff;
		int16 fZeroByDefault;
		int32 FltGrpID;
	} Head_data;
	struct IT_default {
		int32 LineNo;
		int32 ItemID;
		int16 WritedOff;
		int16 AutoLine;
		double UnitPerPack;
		double Quantity;
		double StockRest;
		double Price;
		double StockPrice;
		int16 DiffSign;
		double DiffQtty;
		double DiffPctQtty;
		double UnwritedDiff;
		char ExtGrpName[128];
		double WrOffPrice;
		double CSesDfctQtty;
		double CSesDfctPrice;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_PUGL : public PPALDD {
public:
	PPALDD_PUGL(long resId = 111, PPALDD ** ppA = 0);
	~PPALDD_PUGL();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);

	struct Head {
	} Head_data;
	struct IT_default {
		int32 nn;
		int32 GoodsID;
		int32 LocID;
		double NeedQtty;
		double Deficit;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_GoodsBasket : public PPALDD {
public:
	PPALDD_GoodsBasket(long resId = 112, PPALDD ** ppA = 0);
	~PPALDD_GoodsBasket();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 ID;
		int32 Glb;
		int32 FltSupplID;
		char Name[30];
		char Phones[32];
		int32 Flags;
		int32 ItemsCount;
		double EstPrice;
	} Head_data;
	struct IT_default {
		int32 LineNo;
		int32 ItemID;
		int32 Flags;
		double Price;
		double Quantity;
		double SumPerUnit;
		char CQtty[32];
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_ShipmAnlz : public PPALDD {
public:
	PPALDD_ShipmAnlz(long resId = 113, PPALDD ** ppA = 0);
	~PPALDD_ShipmAnlz();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltLocID;
		int32 FltOpID;
		int32 FltAccSheetID;
		int32 FltArID;
		LDATE FltBeg;
		LDATE FltEnd;
		int32 Flags;
		int16 fDebtOnly;
		int16 fLabelOnly;
		int16 fDiffByBill;
	} Head_data;
	struct IT_default {
		int32 BillID;
		int32 GoodsID;
		double OrderQtty;
		double OrderAmount;
		double ShipmentQtty;
		double ShipmentAmount;
		double AckQtty;
		double AckAmount;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_PredictSales : public PPALDD {
public:
	PPALDD_PredictSales(long resId = 114, PPALDD ** ppA = 0);
	~PPALDD_PredictSales();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltGoodsID;
		int32 FltLocID;
		LDATE FltBeg;
		LDATE FltEnd;
		int16 FltCycle;
		int16 FltNumCycles;
		LDATE Watershed;
		int32 Flags;
	} Head_data;
	struct IT_default {
		LDATE Dt;
		char CycleText[32];
		double Qtty;
		double QttyPredict;
		double QttyAbsErr;
		double QttyPctErr;
		double Amt;
		double AmtPredict;
		double AmtAbsErr;
		double AmtPctErr;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_GoodsTurnovr : public PPALDD {
public:
	PPALDD_GoodsTurnovr(long resId = 115, PPALDD ** ppA = 0);
	~PPALDD_GoodsTurnovr();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		LDATE FltBeg;
		LDATE FltEnd;
		int32 FltGoodsGrpID;
		char FltLocName[48];
		char FltSupplName[64];
	} Head_data;
	struct IT_default {
		LDATE Dt;
		double RcptSuppl;
		double RcptIntr;
		double RetRetail;
		double RetClient;
		double RetSuppl;
		double ExpndRetail;
		double ExpndClient;
		double ExpndIntr;
		double PayClient;
		double Income;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_PaymPlan : public PPALDD {
public:
	PPALDD_PaymPlan(long resId = 116, PPALDD ** ppA = 0);
	~PPALDD_PaymPlan();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		LDATE FltBeg;
		LDATE FltEnd;
		int32 FltAccSheetID;
		int32 FltArID;
		int32 FltLocID;
		int16 FltCycle;
		int16 FltNumCycles;
		int32 FltFlags;
	} Head_data;
	struct IT_default {
		int32 BillID;
		int32 ArID;
		LDATE Dt;
		char CycleText[32];
		char Name[48];
		double Amount;
		double Paym;
		double Debt;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_AdvanceRep : public PPALDD {
public:
	PPALDD_AdvanceRep(long resId = 117, PPALDD ** ppA = 0);
	~PPALDD_AdvanceRep();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);

	struct Head {
		int32 Glb;
		int32 BillID;
		char Rcp1Text[28];
		LDATE Rcp1Dt;
		double Rcp1Amt;
		char Rcp2Text[28];
		LDATE Rcp2Dt;
		double Rcp2Amt;
		double InRest;
		double RcpAmount;
		double ExpAmount;
		double OutRest;
		int16 NumAddedBills;
		int16 NumAddedSheets;
		int16 nn;
	} Head_data;
	struct IT_default {
		char AdvCode[10];
		LDATE AdvDt;
		int32 AdvBillKindID;
		char AdvBillKindName[48];
		int32 AdvBillID;
		int32 AccID;
		int32 ArID;
		int32 Flags;
		double Amt;
		double ExtAmt;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_SCardList : public PPALDD {
public:
	PPALDD_SCardList(long resId = 118, PPALDD ** ppA = 0);
	~PPALDD_SCardList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		LDATE FltTrnovrBeg;
		LDATE FltTrnovrEnd;
		int32 FltSeriesID;
		int32 FltOwnerReqID;
		int32 FltEmplrReqID;
		int16 Ft_Inherited;
		int16 Ft_Closed;
		int32 FltFlags;
		double FltMinPDis;
		double FltMaxPDis;
		double FltMinTurnover;
		double FltMaxTurnover;
	} Head_data;
	struct IT_default {
		int32 SCardID;
		int32 EmployerID;
		double Debit;
		double Credit;
		double Rest;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_SCardOpList : public PPALDD {
public:
	PPALDD_SCardOpList(long resId = 119, PPALDD ** ppA = 0);
	~PPALDD_SCardOpList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 SCardID;
		int32 EmployerID;
		LDATE Beg;
		LDATE End;
		double InRest;
		double OutRest;
	} Head_data;
	struct IT_default {
		LDATE Dt;
		LTIME Tm;
		int32 CheckID;
		int32 UserID;
		int32 BillID;
		int32 Flags;
		double Amount;
		double Rest;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_BillTotal : public PPALDD {
public:
	PPALDD_BillTotal(long resId = 120, PPALDD ** ppA = 0);
	~PPALDD_BillTotal();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltLocID;
		int32 FltOpID;
		int32 FltObjID;
		LDATE FltBeg;
		LDATE FltEnd;
		int32 Count;
		double Nominal;
		double Debt;
		int32 Flags;
	} Head_data;
	struct IT_default {
		int32 LineNo;
		int32 AmtTypeID;
		int32 CurID;
		double Amt;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_GoodsRestTotal : public PPALDD {
public:
	PPALDD_GoodsRestTotal(long resId = 121, PPALDD ** ppA = 0);
	~PPALDD_GoodsRestTotal();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltLocID;
		int32 FltSupplID;
		int32 FltGoodsGrpID;
		int32 FltQuotKindID;
		LDATE FltDate;
		int32 Count;
		double SumCost;
		double SumPrice;
		double Quantity;
		double PhQtty;
		int32 Flags;
	} Head_data;
	struct IT_default {
		int32 LineNo;
		int32 AmtTypeID;
		double Amt;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_GTaxAnlzTotal : public PPALDD {
public:
	PPALDD_GTaxAnlzTotal(long resId = 122, PPALDD ** ppA = 0);
	~PPALDD_GTaxAnlzTotal();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltLocID;
		int32 FltOpID;
		int32 FltGoodsGrpID;
		int32 FltSupplID;
		int32 FltSupplAgentID;
		LDATE FltBeg;
		LDATE FltEnd;
		LDATE FltLotsBeg;
		LDATE FltLotsEnd;
		int16 IncomeMethod;
		int32 Count;
		double TrnovrCost;
		double TrnovrPrice;
		double Income;
		double ExciseSum;
		double C_VATSum;
		double VATSum;
		double IncVATSum;
		double STaxSum;
	} Head_data;
	struct IT_default {
		int32 LineNo;
		double VatRate;
		double InVat;
		double OutVat;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_PersonList : public PPALDD {
public:
	PPALDD_PersonList(long resId = 123, PPALDD ** ppA = 0);
	~PPALDD_PersonList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 PsnKindID;
		int32 StatusID;
		int32 CountryID;
		int32 CityID;
		int32 RegTypeID;
		int32 TagTypeID;
		int32 AttrType;
	} Head_data;
	struct IT_default {
		int32 PersonID;
		char Phone[64];
		char Address[64];
		char RAddress[64];
		char BnkName[48];
		char BnkAcct[24];
		char Serial[12];
		char Number[64];
		LDATE InitDate;
		LDATE Expiry;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_AssetReceipt : public PPALDD {
public:
	PPALDD_AssetReceipt(long resId = 124, PPALDD ** ppA = 0);
	~PPALDD_AssetReceipt();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 LotID;
		int32 OrgLotID;
		int32 AssetAccID;
		int32 AssetArID;
		int32 ExplBillID;
		double StartCost;
		double StartPrice;
		int32 MovLineNo;
	} Head_data;
	struct IT_default {
		int32 MovBillID;
		int32 MovLotID;
		int32 DestLocID;
		double MovPrice;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_ObjSyncCmp : public PPALDD {
public:
	PPALDD_ObjSyncCmp(long resId = 125, PPALDD ** ppA = 0);
	~PPALDD_ObjSyncCmp();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltObjType;
		int32 DestDbDiv;
		int32 FltErrCode;
		int32 fErrOnly;
		int32 Order;
	} Head_data;
	struct IT_default {
		int32 ObjType;
		int32 CommID;
		int32 SrcID;
		int32 DestID;
		char SrcName[64];
		char DestName[64];
		int32 ErrCode;
		double CmpScore;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_AssetView : public PPALDD {
public:
	PPALDD_AssetView(long resId = 126, PPALDD ** ppA = 0);
	~PPALDD_AssetView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltLocID;
		int32 FltGrpID;
		LDATE FltBeg;
		LDATE FltEnd;
		LDATE FltOperBeg;
		LDATE FltOperEnd;
		int16 Ft_InExpl;
	} Head_data;
	struct IT_default {
		int32 LotID;
		int32 GoodsID;
		LDATE Dt;
		LDATE ExplDt;
		char GoodsGrpName[128];
		double Cost;
		double Price;
		double Deprec;
		double Cost2;
		double Price2;
		double Deprec2;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_SStatView : public PPALDD {
public:
	PPALDD_SStatView(long resId = 127, PPALDD ** ppA = 0);
	~PPALDD_SStatView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		LDATE FltBeg;
		LDATE FltEnd;
		int16 FltCycle;
		int16 FltNumCycles;
		int32 LocID;
		int32 GoodsGrpID;
		int32 SupplID;
		LDATE RestDate;
		int16 OrdTerm;
		int16 DlvrTerm;
		int32 Flags;
		int16 fSkipZeroNhCount;
		int16 fSupplOrderForm;
		int16 fRoundOrderToPack;
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		char GoodsName[64];
		int32 Count;
		LDATE Dt;
		char CycleText[32];
		double QttySum;
		double QttyAvg;
		double QttySigma;
		double QttyVar;
		double QttyTrnovr;
		double AmtSum;
		double AmtAvg;
		double AmtSigma;
		double AmtVar;
		double AmtTrnovr;
		double Rest;
		double Predict;
		double SupplOrder;
		double PriceAvg;
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_LogList : public PPALDD {
public:
	PPALDD_LogList(long resId = 128, PPALDD ** ppA = 0);
	~PPALDD_LogList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
	} Head_data;
	struct IT_default {
		char LogListStr[255];
	} IT_default_data;
};

#ifdef __WIN32__
class PPALDD_Warrant : public PPALDD {
public:
	PPALDD_Warrant(long resId = 129, PPALDD ** ppA = 0);
	~PPALDD_Warrant();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 PersonReqID;
		int32 SupplID;
		char WarrantNo[12];
		LDATE WarrantDt;
		LDATE ExpiryDt;
		char Memo[128];
	} Head_data;
	struct IT_default {
		int32 LineNo;
		char GdsName[48];
		char Unit[6];
		char Qtty[32];
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_FreightList : public PPALDD {
public:
	PPALDD_FreightList(long resId = 130, PPALDD ** ppA = 0);
	~PPALDD_FreightList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		LDATE FltBillBeg;
		LDATE FltBillEnd;
		LDATE FltShipmBeg;
		LDATE FltShipmEnd;
		LDATE FltArrvlBeg;
		LDATE FltArrvlEnd;
		int32 FltLocID;
		int32 FltOpID;
		int32 FltObjectID;
		int32 FltPortID;
		int32 FltFlags;
		int16 fUnshippedOnly;
		int16 fFillLaggageFields;
	} Head_data;
	struct IT_default {
		int32 BillID;
		int32 ObjectID;
		LDATE BillDate;
		LDATE ArrvlDate;
		LDATE ShipmDate;
		int16 IsShipped;
		char BillCode[24];
		char ShipName[64];
		int32 PortID;
		char PortName[30];
		int32 DlvrAddrID;
		char DlvrAddr[64];
		double Amount;
		double Brutto;
		double Volume;
		double PackCount;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_QuotView : public PPALDD {
public:
	PPALDD_QuotView(long resId = 131, PPALDD ** ppA = 0);
	~PPALDD_QuotView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltQuotKindID;
		int32 FltLocID;
		int32 FltClientID;
		int32 FltCurID;
		int32 FltGoodsGrpID;
		int32 FltGoodsID;
		int32 Flags;
		int32 NumQuotKinds;
		char QuotName1[30];
		char QuotName2[30];
		char QuotName3[30];
		char QuotName4[30];
		char QuotName5[30];
		char QuotName6[30];
		char QuotName7[30];
		char QuotName8[30];
		char QuotName9[30];
		char QuotName10[30];
		char QuotName11[30];
		char QuotName12[30];
		char QuotName13[30];
		char QuotName14[30];
		char QuotName15[30];
		char QuotName16[30];
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		int32 LocID;
		int32 ClientID;
		char Quot1[16];
		char Quot2[16];
		char Quot3[16];
		char Quot4[16];
		char Quot5[16];
		char Quot6[16];
		char Quot7[16];
		char Quot8[16];
		char Quot9[16];
		char Quot10[16];
		char Quot11[16];
		char Quot12[16];
		char Quot13[16];
		char Quot14[16];
		char Quot15[16];
		char Quot16[16];
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_GoodsBillCmp : public PPALDD {
public:
	PPALDD_GoodsBillCmp(long resId = 132, PPALDD ** ppA = 0);
	~PPALDD_GoodsBillCmp();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 LhBillID;
		int32 RhBillID;
		int16 fOnlyDiffQtty;
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		double LhQtty;
		double RhQtty;
		double DiffQtty;
		double LhPrice;
		double RhPrice;
		double DiffPrice;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_Complete : public PPALDD {
public:
	PPALDD_Complete(long resId = 133, PPALDD ** ppA = 0);
	~PPALDD_Complete();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 ParentLotID;
		int32 BillID;
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		int32 LotID;
		char Serial[24];
		double Qtty;
		double Cost;
		double Price;
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_MrpTab : public PPALDD {
public:
	PPALDD_MrpTab(long resId = 134, PPALDD ** ppA = 0);
	~PPALDD_MrpTab();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Name[48];
		int32 LinkObjType;
		int32 LinkObjID;
		int32 LocID;
		LDATE Dt;
		int32 Flags;
		char LinkObjName[64];
	} Head_data;
};

#ifdef __WIN32__
class PPALDD_MrpLines : public PPALDD {
public:
	PPALDD_MrpLines(long resId = 135, PPALDD ** ppA = 0);
	~PPALDD_MrpLines();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 TabID;
		int16 FltRowKind;
		int16 FltShowDfctOnly;
		int16 FltShowTermOnly;
	} Head_data;
	struct IT_default {
		int32 ID;
		int32 GoodsID;
		double ReqQtty;
		double Rest;
		double Deficit;
		double Cost;
		double Price;
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_Processor : public PPALDD {
public:
	PPALDD_Processor(long resId = 136, PPALDD ** ppA = 0);
	~PPALDD_Processor();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 ParentID;
		int32 Kind;
		char Name[48];
		char Code[10];
		int32 LocID;
		int32 TimeUnitID;
		int32 Flags;
		int32 LinkObjType;
		int32 LinkObjID;
		int32 WrOffOpID;
		int32 WrOffArID;
	} Head_data;
};

#ifdef __WIN32__
class PPALDD_ProcessorView : public PPALDD {
public:
	PPALDD_ProcessorView(long resId = 137, PPALDD ** ppA = 0);
	~PPALDD_ProcessorView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltGrpID;
		int32 FltLocID;
		int32 FltKind;
		int32 Flags;
	} Head_data;
	struct IT_default {
		int32 PrcID;
		int32 ParentID;
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_Tech : public PPALDD {
public:
	PPALDD_Tech(long resId = 138, PPALDD ** ppA = 0);
	~PPALDD_Tech();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		char Code[24];
		int32 PrcID;
		int32 GoodsID;
		int32 GStrucID;
		char GStrucName[30];
		int32 Flags;
		int16 Sign;
		double Timing;
		double Cost;
		double Capacity;
		char Memo[160];
	} Head_data;
};

class PPALDD_TSession : public PPALDD {
public:
	PPALDD_TSession(long resId = 139, PPALDD ** ppA = 0);
	~PPALDD_TSession();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 Glb;
		int32 ParentID;
		int32 Number;
		int32 TechID;
		int32 PrcID;
		char Name[48];
		char SuperName[48];
		LDATE StDt;
		LTIME StTm;
		char StTmTxt[12];
		LDATE FinDt;
		LTIME FinTm;
		char FinTmTxt[12];
		int16 Incomplete;
		int16 Status;
		int16 Idle;
		int16 Reserve;
		char StatusText[12];
		int32 Flags;
		int32 ArID;
		int32 Ar2ID;
		int32 PlannedTiming;
		char PlnTimingText[24];
		double PlannedQtty;
		int32 ActTiming;
		char ActTimingText[24];
		double ActQtty;
		double Amount;
		char Memo[160];
	} Head_data;
};

#ifdef __WIN32__
class PPALDD_TechView : public PPALDD {
public:
	PPALDD_TechView(long resId = 141, PPALDD ** ppA = 0);
	~PPALDD_TechView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltPrcID;
		int32 FltGoodsID;
		int32 FltGsID;
		int32 SignFlags;
		int32 Flags;
	} Head_data;
	struct IT_default {
		int32 TechID;
		char TimingTxt[16];
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_TSessionView : public PPALDD {
public:
	PPALDD_TSessionView(long resId = 142, PPALDD ** ppA = 0);
	~PPALDD_TSessionView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltSuperID;
		int32 FltPrcID;
		int32 FltTechID;
		int32 FltArID;
		int32 FltAr2ID;
		LDATE FltStBeg;
		LDATE FltStEnd;
		LDATE FltFnBeg;
		LDATE FltFnEnd;
		int32 Ft_Idle;
	} Head_data;
	struct IT_default {
		int32 SessID;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_TSessLineView : public PPALDD {
public:
	PPALDD_TSessLineView(long resId = 143, PPALDD ** ppA = 0);
	~PPALDD_TSessLineView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltSessID;
		int32 FltGoodsID;
		int32 FltFlags;
	} Head_data;
	struct IT_default {
		int32 SessID;
		int32 GoodsID;
		int32 UserID;
		int32 OprNo;
		int16 Sign;
		LDATE Dt;
		LTIME Tm;
		int32 Flags;
		double Qtty;
		double Price;
		char Serial[24];
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_TSessAnlz : public PPALDD {
public:
	PPALDD_TSessAnlz(long resId = 144, PPALDD ** ppA = 0);
	~PPALDD_TSessAnlz();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 PlanSessID;
		int32 SingleSessID;
		int32 FltPrcID;
		int32 FltGoodsGrpID;
		int32 FltArID;
		int32 FltAr2ID;
		LDATE FltStBeg;
		LDATE FltStEnd;
		LTIME FltStTime;
		LDATE FltFnBeg;
		LDATE FltFnEnd;
		LTIME FltFnTime;
		char StPeriod[32];
		char FnPeriod[32];
		int32 FltFlags;
		int16 fAll;
		int16 fPrmrGoodsOnly;
		int16 fShowRest;
		int16 fExtrapolToPeriod;
		int16 fAddTotalRows;
		int16 fNmgRestrictOnly;
		int16 DiffPrc;
		int16 DiffMg;
		int16 DiffDt;
		int16 Reserve;
	} Head_data;
	struct IT_default {
		int32 PrcID;
		int32 PrmrGoodsID;
		int32 GoodsID;
		LDATE Dt;
		int16 NotPrmrLine;
		int16 PlanPhUnit;
		double InQtty;
		double OutQtty;
		double InQttyPh;
		double OutQttyPh;
		double PlanInQtty;
		double PlanOutQtty;
		double OutRest;
		double Dev;
		double DevPct;
		double InCompPart;
		double OutCompPart;
		char PrmrGoodsName[64];
		char GoodsName[64];
		char PrcName[48];
		char DtText[48];
		char Intrn_PrcName[48];
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_Project : public PPALDD {
public:
	PPALDD_Project(long resId = 145, PPALDD ** ppA = 0);
	~PPALDD_Project();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 Kind;
		int32 ParentID;
		char Name[48];
		char Code[16];
		LDATE Dt;
		LDATE BeginDt;
		LDATE EstFinishDt;
		LDATE FinishDt;
		int32 MngrID;
		int32 ClientID;
		int32 TemplateID;
		int32 Status;
		int32 Flags;
		char FullName[96];
		char Descr[224];
		char Memo[128];
	} Head_data;
};

class PPALDD_PrjTask : public PPALDD {
public:
	PPALDD_PrjTask(long resId = 146, PPALDD ** ppA = 0);
	~PPALDD_PrjTask();
	virtual int InitData(PPFilt *, long rsrv = 0);

	struct Head {
		int32 ID;
		int32 ProjectID;
		int32 Kind;
		char Code[16];
		int32 CreatorID;
		int32 GroupID;
		int32 EmployerID;
		int32 ClientID;
		int32 DlvrAddrID;
		int32 TemplateID;
		int32 LinkTaskID;
		LDATE Dt;
		LTIME Tm;
		LDATE StartDt;
		LTIME StartTm;
		LDATE EstFinishDt;
		LTIME EstFinishTm;
		LDATE FinishDt;
		LTIME FinishTm;
		int16 Priority;
		int16 Status;
		char PriorText[16];
		char StatusText[16];
		int16 DrPrd;
		int16 DrKind;
		int32 DrDetail;
		int32 Flags;
		char Descr[224];
		char Memo[128];
	} Head_data;
};

#ifdef __WIN32__
class PPALDD_ProjectView : public PPALDD {
public:
	PPALDD_ProjectView(long resId = 147, PPALDD ** ppA = 0);
	~PPALDD_ProjectView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltParentID;
		int32 FltClientID;
		int32 FltManagerID;
		int32 FltFlags;
		LDATE FltStart_beg;
		LDATE FltStart_end;
		LDATE FltEstFn_beg;
		LDATE FltEstFn_end;
	} Head_data;
	struct IT_default {
		int32 ProjectID;
		int32 PrjTaskID;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_PrjTaskView : public PPALDD {
public:
	PPALDD_PrjTaskView(long resId = 148, PPALDD ** ppA = 0);
	~PPALDD_PrjTaskView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltKind;
		int32 FltProjectID;
		int32 FltClientID;
		int32 FltEmployerID;
		int32 FltTemplateID;
		int32 FltCreatorID;
		int32 FltFlags;
		char FltPriorList[48];
		char FltStatusList[48];
		LDATE FltPeriod_beg;
		LDATE FltPeriod_end;
		LDATE FltStart_beg;
		LDATE FltStart_end;
		LDATE FltEstFn_beg;
		LDATE FltEstFn_end;
		LDATE FltFn_beg;
		LDATE FltFn_end;
	} Head_data;
	struct IT_default {
		int32 TodoID;
		int32 TemplID;
		int32 LinkID;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_PrjTaskViewCt : public PPALDD {
public:
	PPALDD_PrjTaskViewCt(long resId = 149, PPALDD ** ppA = 0);
	~PPALDD_PrjTaskViewCt();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltKind;
		int32 FltProjectID;
		int32 FltClientID;
		int32 FltEmployerID;
		int32 FltTemplateID;
		int32 FltCreatorID;
		int32 FltFlags;
		char FltPriorList[48];
		char FltStatusList[48];
		LDATE FltPeriod_beg;
		LDATE FltPeriod_end;
		LDATE FltStart_beg;
		LDATE FltStart_end;
		LDATE FltEstFn_beg;
		LDATE FltEstFn_end;
		LDATE FltFn_beg;
		LDATE FltFn_end;
		char IdxFld[48];
		char TabFld[48];
	} Head_data;
	struct IT_default {
		char IdxName[48];
		char TabName[48];
		int32 Value;
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_BillInfo : public PPALDD {
public:
	PPALDD_BillInfo(long resId = 150, PPALDD ** ppA = 0);
	~PPALDD_BillInfo();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);

	struct Head {
		int32 Glb;
		int32 BillID;
		int16 fTotalDiscount;
		int16 fGReceipt;
		int16 fGExpend;
		int16 fGReval;
		int16 fGModif;
		int16 fClosedOrder;
		int16 fCash;
		int16 fCCheck;
		int16 fNoAturn;
		int16 fRmvExcise;
		int16 fFixedAmounts;
		int16 fFreight;
		int16 fRent;
		int16 fRecon;
		int16 fBanking;
		int16 fShipped;
		int16 fWritedOff;
		int16 fCSessWrOff;
		int16 fAdvanceRep;
		int16 fTGGLexCsNPrice;
		int16 fTSessWrOff;
	} Head_data;
	struct IT_default {
		int32 nn;
		int32 AmountID;
		int32 CurrencyID;
		double Amount;
	} IT_default_data;
};

class PPALDD_BillInfoList : public PPALDD {
public:
	PPALDD_BillInfoList(long resId = 151, PPALDD ** ppA = 0);
	~PPALDD_BillInfoList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);

	struct Head {
		int32 Glb;
	} Head_data;
	struct IT_default {
		int32 BillID;
		int16 fTotalDiscount;
		int16 fGReceipt;
		int16 fGExpend;
		int16 fGReval;
		int16 fGModif;
		int16 fClosedOrder;
		int16 fCash;
		int16 fCCheck;
		int16 fNoAturn;
		int16 fRmvExcise;
		int16 fFixedAmounts;
		int16 fFreight;
		int16 fRent;
		int16 fRecon;
		int16 fBanking;
		int16 fShipped;
		int16 fWritedOff;
		int16 fCSessWrOff;
		int16 fAdvanceRep;
		int16 fTGGLexCsNPrice;
		int16 fTSessWrOff;
		int32 nn;
		int32 AmountID;
		int32 CurrencyID;
		double Amount;
	} IT_default_data;
};

#ifdef __WIN32__
class PPALDD_PriceAnlz : public PPALDD {
public:
	PPALDD_PriceAnlz(long resId = 152, PPALDD ** ppA = 0);
	~PPALDD_PriceAnlz();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltSupplID;
		LDATE FltPeriod_beg;
		LDATE FltPeriod_end;
	} Head_data;
	struct IT_default {
		int32 GoodsID;
		int32 LocID;
		double ContractCost;
		double Cost;
		double CostDiff;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_PrcBusyView : public PPALDD {
public:
	PPALDD_PrcBusyView(long resId = 153, PPALDD ** ppA = 0);
	~PPALDD_PrcBusyView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 FltPrcID;
		LDATE FltStDt;
		LTIME FltStTm;
		LDATE FltFnDt;
		LTIME FltFnTm;
		LTIME FltMinDuration;
		int32 FltFlags;
		int16 fFree;
	} Head_data;
	struct IT_default {
		int32 PrcID;
		int32 TSessID;
		LDATE StDt;
		LTIME StTm;
		LDATE FnDt;
		LTIME FnTm;
		int32 Duration;
		char TxtPeriod[48];
		char TxtDuration[16];
	} IT_default_data;
};

#endif // __WIN32__
class PPALDD_RptInfo : public PPALDD {
public:
	PPALDD_RptInfo(long resId = 154, PPALDD ** ppA = 0);
	~PPALDD_RptInfo();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
	} Head_data;
	struct IT_default {
		char StdName[30];
		LDATE ModifDt;
		char Descr[128];
		char DataStruc[30];
		char RPTType[12];
		char AltName[70];
	} IT_default_data;
};

#ifdef __WIN32__
class PPALDD_SCardSerView : public PPALDD {
public:
	PPALDD_SCardSerView(long resId = 155, PPALDD ** ppA = 0);
	~PPALDD_SCardSerView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
	} Head_data;
	struct IT_default {
		int32 QuotKindID;
		int32 PsnKindID;
		char Name[30];
		char CodeTempl[18];
		double PDis;
		double MaxCredit;
		LDATE IssueDate;
		LDATE Expiry;
		int16 fCredit;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_GoodsGroupView : public PPALDD {
public:
	PPALDD_GoodsGroupView(long resId = 156, PPALDD ** ppA = 0);
	~PPALDD_GoodsGroupView();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int16 GrpType;
	} Head_data;
	struct IT_default {
		int32 UnitID;
		int32 PhUnitID;
		int32 GdsTypeID;
		int32 TaxGrpID;
		int32 GdsClsID;
		int32 GrpID;
		char Name[64];
		char BfrNmVLines[64];
		char Links[64];
		char Code[16];
		int16 fAlt;
		int16 fTempAlt;
		int16 fDynAlt;
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_PersonEvent : public PPALDD {
public:
	PPALDD_PersonEvent(long resId = 157, PPALDD ** ppA = 0);
	~PPALDD_PersonEvent();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
	} Head_data;
	struct IT_default {
		int32 LocID;
		int32 PersonID;
		int32 SecondID;
		LDATE Dt;
		int32 OprNo;
		char OpName[48];
		char Memo[128];
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_PsnEventItem : public PPALDD {
public:
	PPALDD_PsnEventItem(long resId = 158, PPALDD ** ppA = 0);
	~PPALDD_PsnEventItem();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
		int32 LocID;
		int32 PersonID;
		int32 SecondID;
		LDATE Dt;
		int32 OprNo;
		char OpName[48];
		char Memo[128];
	} Head_data;
	struct IT_default {
		int32 TagID;
		int32 nn;
		char TagValue[128];
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_PsnRelList : public PPALDD {
public:
	PPALDD_PsnRelList(long resId = 159, PPALDD ** ppA = 0);
	~PPALDD_PsnRelList();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
	} Head_data;
	struct IT_default {
		int32 PersonID;
		int32 SecondID;
		char RelationName[30];
	} IT_default_data;
};

#endif // __WIN32__
#ifdef __WIN32__
class PPALDD_JobPool : public PPALDD {
public:
	PPALDD_JobPool(long resId = 160, PPALDD ** ppA = 0);
	~PPALDD_JobPool();
	virtual int InitData(PPFilt *, long rsrv = 0);
	virtual int InitIteration(PPIterID, int sortId, long rsrv = 0);
	virtual int NextIteration(PPIterID, long rsrv = 0);
	virtual int Destroy();

	struct Head {
		int32 Glb;
	} Head_data;
	struct IT_default {
		int32 nn;
		int32 ID;
		char Name[48];
		char Symb[32];
		char DateTimeRepeating[48];
	} IT_default_data;
};

#endif // __WIN32__
