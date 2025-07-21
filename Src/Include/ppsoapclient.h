// PPSOAPCLIENT.H
// Copyright (c) A.Sobolev 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2025
// @codepage UTF-8
//
#include <slib.h>

BOOL Implement_SoapModule_DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved, const char * pProductName);

class PPSoapResultPtrBase {
public:
	static SCollection ResultPtrList;

	virtual void Destroy() = 0;
	virtual const void * GetPtr() const = 0;
	int operator == (const void * ptr) const { return (ptr == GetPtr()); }
};

template <class T> class PPSoapResultPtr : public PPSoapResultPtrBase {
public:
	PPSoapResultPtr(T * p) : Ptr(p)
	{
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

int FASTCALL PPSoapDestroyResultPtr(void * p);
void ** FASTCALL PPSoapCreateArray(uint count, int & rArrayCount);

template <class T> static void PPSoapRegisterResultPtr(T * ptr)
{
	if(ptr) {
		PPSoapResultPtr <T> * p_item = new PPSoapResultPtr <T> (ptr);
		if(p_item)
			PPSoapResultPtrBase::ResultPtrList.insert(p_item);
	}
}


class PPSoapClientSession {
public:
	PPSoapClientSession();
	void   Setup(const char * pUrl);
	void   Setup(const char * pUrl, const char * pUser, const char * pPassword);
	const  char * GetUrl() const;
	void   FASTCALL SetMsg(const char * pUtf8Text);
	const  char * GetMsg() const;
	const  char * GetUser() const;
	const  char * GetPassword() const;
private:
	int    ErrCode;
	char   ErrMsg[1024];
	char   Url[256];
	SString User;
	SString Password;
};

class InParamString : public STempBuffer {
public:
	InParamString(const char * pStr);
};

char * FASTCALL GetDynamicParamString(const char * pSrc, TSCollection <InParamString> & rPool);
char * FASTCALL GetDynamicParamString(long ival, TSCollection <InParamString> & rPool);
char * FASTCALL GetDynamicParamString(int ival, TSCollection <InParamString> & rPool);
char * FASTCALL GetDynamicParamString_(double rval, long fmt, TSCollection <InParamString> & rPool);
char * FASTCALL GetDynamicParamString(LDATE dval, long fmt, TSCollection <InParamString> & rPool);
char * FASTCALL GetDynamicParamString(LDATETIME dtval, long dfmt, long tfmt, TSCollection <InParamString> & rPool);

class UhttStatus {
public:
	UhttStatus();
	int    Code;    // Код результата исполнения операции
	int    Index;   // Для списка - индекс в исходном списке (переденном в виде параметра), к которому относится статус
	int    Id;      // Для списка - идентификатор объекта в исходном списке (переденном в виде параметра), к которому относится статус
	SString Msg;    // Текст сообщения //
};

class UhttDate {
public:
	UhttDate & Z();
	UhttDate & FASTCALL operator = (const char * pStr);
	UhttDate & FASTCALL operator = (LDATE dt);
	operator LDATE () const;

	SString Date;
};
//
// Descr: Дата/время, передаваемые одной строкой в формате ISO-8601 (yyyy-mm-ddThh:mm:ss)
//
class UhttTimestamp {
public:
	UhttTimestamp & Z();
	UhttTimestamp & FASTCALL operator = (LDATE dt);
	UhttTimestamp & FASTCALL operator = (LTIME tm);
	UhttTimestamp & FASTCALL operator = (const LDATETIME & rDtm);
	operator LDATE () const;
	operator LTIME () const;
	operator LDATETIME () const;

	SString T;
};

class UhttDateTime {
public:
	UhttDateTime & Z();
	UhttDateTime & FASTCALL operator = (LDATE dt);
	UhttDateTime & FASTCALL operator = (LTIME tm);
	UhttDateTime & FASTCALL operator = (const LDATETIME & rDtm);
	operator LDATE () const;
	operator LTIME () const;
	operator LDATETIME () const;
	SString Date;
	SString Time;
};

class UhttDatePeriod {
public:
	UhttDatePeriod & Z();
	UhttDatePeriod & FASTCALL operator = (const DateRange & rRng);
	SString Low;
	SString Upp;
};

class UhttTagItem {
public:
	UhttTagItem();
	UhttTagItem(const char * pSymb, const SString & rText);
	void FASTCALL SetSymb(const char * pText);
	void FASTCALL SetValue(const char * pText);
	SString Symb;
	SString Value;
};

class UhttBrandPacket {
public:
	UhttBrandPacket();

	int32  ID;
	char   Name[128];
	int32  OwnerID;
	int32  Flags;
};

class UhttStyloDevicePacket {
public:
	UhttStyloDevicePacket();
	void FASTCALL SetName(const char * pText);
	void FASTCALL SetSymb(const char * pText);
	int    ID;
	SString Name;
	SString Symb;
	int    ParentID;
	int    Flags;
	int    DeviceVer;
	UhttTimestamp RegisterTime;
};

class UhttGoodsPacket {
public:
	UhttGoodsPacket();
	UhttGoodsPacket & FASTCALL operator = (const UhttGoodsPacket & rS);
	UhttGoodsPacket & FASTCALL Copy(const UhttGoodsPacket & rS);
	//
	// Descr: Устанавливает наименование товара (с перекодировкой и корректировкой не верных символов);
	// Note: Доступно только из проекта Papyrus
	//
	void    FASTCALL SetName(const char * pName);
	//
	// Descr: Устанавливает текст расширения товара (с перекодировкой и корректировкой не верных символов);
	// Note: Доступно только из проекта Papyrus
	//
	void    FASTCALL SetExt(int extFldId, const char * pText);

	struct Barcode {
		int32  GoodsID;
		char   Code[32];
		double Qtty;
	};
	int32  ID;
	int32  BrandID;
	int32  ManufID; // @v8.3.5
	char   Name[256];
	char   SingleBarcode[32];
	double Package;

	SString ExtA; // STORAGE
	SString ExtB; // STANDARD
	SString ExtC; // INGRED
	SString ExtD; // ENERGY
	SString ExtE; // USAGE
	TSArray <UhttGoodsPacket::Barcode> BarcodeList;
	TSCollection <UhttTagItem> TagList;
};
//
// Descr: Представление элемента сопоставления приватного идентификатора объекта с идентификатором
//   соответствующего объекта в Universe-HTT по коду.
//
struct UhttCodeRefItem {
	UhttCodeRefItem();
	UhttCodeRefItem & FASTCALL Set(int privateID, const char * pCode);

	int32  PrivateID;
	int32  UhttID;
	uint   InnerPos; // Внутреннее поле. Используется при вызове WSDL-метода для сопоставления списков.
	char   Code[32];
};

class UhttDocumentPacket {
public:
	UhttDocumentPacket();
	int    SetFile(const char * pFileName);

	int    UhttObjID;
	int64  Size;
	SString ObjTypeSymb;
	SString Name;
	SString Encoding;
	SString ContentType;
	SString ContentMime;
};

class UhttSpecSeriesPacket {
public:
	UhttSpecSeriesPacket();
	void FASTCALL SetSerial(const char * pText);
	void FASTCALL SetGoodsName(const char * pText);
	void FASTCALL SetManufName(const char * pText);
	void FASTCALL SetInfoIdent(const char * pText);
	void FASTCALL SetAllowNumber(const char * pText);
	void FASTCALL SetLetterType(const char * pText);

	int     ID;
	int     GoodsID;
	int     ManufID;
	int     ManufCountryID;
	int     LabID;
	SString Serial;
	SString Barcode;
	SString GoodsName;
	SString ManufName;
	SString InfoDate;
	int     InfoKind;
	SString InfoIdent;
	SString AllowDate;
	SString AllowNumber;
	SString LetterType;
	int     Flags;
};

class UhttLocationPacket {
public:
	UhttLocationPacket();
	void FASTCALL SetName(const char * pName);
	void FASTCALL SetCode(const char * pCode);
	void FASTCALL SetPostalCode(const char * pPostalCode);
	void FASTCALL SetAddress(const char * pAddress);
	void FASTCALL SetPhone(const char * pPhone);
	void FASTCALL SetEMail(const char * pEMail);
	void FASTCALL SetContact(const char * pContact);
	SString & FASTCALL GetCode(SString & rBuf) const;

	int    ID;
	int    ParentID;
	int    Type;
	int    CityID;
	int    OwnerPersonID;
	double Latitude;
	double Longitude;
	int    Flags;
	SString Name;
	SString Code;
	SString PostalCode;
	SString Address;
	SString Phone;
	SString EMail;
	SString Contact;
};

class UhttSCardPacket {
public:
	UhttSCardPacket();
	void FASTCALL SetCode(const char * pText);
	void FASTCALL SetPhone(const char * pText);
	void FASTCALL SetMemo(const char * pText);

	int    ID;
	int    SeriesID;
	int    OwnerID;
	SString Code;
	SString Hash;
	SString Phone;
	SString Memo;
	UhttDateTime IssueDate;
	UhttDateTime Expiry;
	double PDis;
	double Overdraft;
	double Debit;
	double Credit;
	double Rest;
	int    Flags;
};

class UhttCheckPacket {
public:
	UhttCheckPacket();

	struct CheckItem {
		int    ID;
		int    GoodsID;
		int    Qtty;
		double Price;
		double Discount;
	};
	int    PosNodeID;
	int    LocID;
	int    SCardID;
	double Amount;
	double Discount;
	int    Flags;
	UhttDateTime Dtm;
	TSArray <UhttCheckPacket::CheckItem> Items;
};

class UhttQuotFilter {
public:
	UhttQuotFilter();

	int    GoodsID;
	int    GroupID;
	int    BrandID;
	int    SellerID;
	int    LocationID;
	int    BuyerID;
	int    AndFlags;
	int    NotFlags;
};

class UhttQuotPacket {
public:
	UhttQuotPacket();

	int    GoodsID;
	int    SellerID;
	int    LocID;
	int    BuyerID;
	int    CurrID;
	double MinQtty;
	UhttDateTime ChangesDate;
	UhttDatePeriod ActualPeriod;
	double Value;
	int    Flags;
};

class UhttPersonPacket {
public:
	UhttPersonPacket();
	UhttPersonPacket & operator = (const UhttPersonPacket & rS);
	SString & GetUhttContragentCode(SString & rBuf) const;

	struct Kind {
		SString Code;
		SString Name;
	};
	struct Register {
		int    ID;
		int    TypeID;
		int    OrganID;
		UhttDate Dt;
		UhttDate Expiry;
		SString TypeName;
		SString Serial;
		SString Number;
	};
	struct AddressP : public UhttLocationPacket {
		AddressP();

		int    Kind; // 1 - юр адрес, 2 - фактический адрес, 3 - адрес доставки
		SString CountryName;
		SString CityName;
	};
	int    ID;
	int    StatusID;
	int    CategoryID;
	SString Name;
	SString INN;
	StringSet PhoneList;
	StringSet EMailList;
	StringSet UrlList;
	TSCollection <Kind> KindList;
	TSCollection <Register> RegList;
	TSCollection <AddressP> AddrList;
};

class UhttBillPacket {
public:
	UhttBillPacket();

	struct BillItem {
		int     BillID;
		int     GoodsID;
		int     Flags;
		double  Quantity;
		double  Cost;
		double  Price;
		double  Discount;
		double  Amount;
		char    Serial[32];
	};
	int    ID;
	int    LocID;
	int    ArID;
	int    DlvrLocID;
	int    CurrID;
	int    AgentID;
	int    StatusID;
	int    Flags;
	S_GUID Uuid;
	UhttDateTime Dtm;
	SString Code;
	SString OpSymb;
	SString Memo;
	UhttPersonPacket Contractor;
	TSArray <UhttBillPacket::BillItem> Items;
};

class UhttBillFilter {
public:
	UhttBillFilter();
	void FASTCALL SetDate(LDATE dt);
	void FASTCALL SetOpSymb(const char * pOpSymb);

	int     LocID;
	int     ArID;
	int     CurrID;
	int     AgentID;
	int     Count;
	int     Last;
	UhttDateTime Since; // @v8.3.2
	UhttDatePeriod Period;
	SString Dt;
	SString OpSymb;
};

class UhttGoodsRestListItem {
public:
	UhttGoodsRestListItem();

	int    GoodsID;
	int    LocID;
	SString Name;     // Наименование организации
	SString LocCode;  // Код локации
	SString LocAddr;  // Адрес локации
	double Rest;
	double Price;
	UhttDateTime RestDtm;
	UhttDateTime PriceDtm;
};

class UhttDCFileVersionInfo {
public:
	UhttDCFileVersionInfo();

	int     ID;
	SString Key;
	SString Title;
	SString Name;
	SString Label;
	int     Revision;
	int64   Ts;
	long    Size;
	int     Flags;
	SString Memo;
};

class UhttWorkbookItemPacket {
public:
	UhttWorkbookItemPacket();
	UhttWorkbookItemPacket & FASTCALL operator = (const UhttWorkbookItemPacket & rS);
	UhttWorkbookItemPacket & FASTCALL Copy(const UhttWorkbookItemPacket & rS);
    void   FASTCALL SetName(const char * pText);
    void   FASTCALL SetSymb(const char * pText);
    void   FASTCALL SetVersion(const char * pText);
    void   FASTCALL SetDescr(const char * pText);

	int    ID;
	int    Type;
	int    ParentID;
	int    LinkID;
	int    CssID;
	int    Rank;
	int    Flags;
	int    KeywordCount;
	int    KeywordDilute;
	SString Name;
	SString Symb;
	UhttDateTime Dtm;
	UhttDateTime ModifDtm;        // @v9.3.8
	UhttDateTime ContentModifDtm; // @v9.3.7
	SString Version;
	SString Descr;
	TSCollection <UhttTagItem> TagList;
};
//
//
//
class UhttPrcPlaceDescription {
public:
	UhttPrcPlaceDescription();
	void FASTCALL SetRange(const char * pText);
	void FASTCALL SetDescr(const char * pText);

	int    GoodsID;    // Товар, ассоциированный с диапазоном мест
	SString Range;
	SString Descr;
};

class UhttProcessorPacket {
public:
	UhttProcessorPacket();
	UhttProcessorPacket & FASTCALL operator = (const UhttProcessorPacket & rS);
	UhttProcessorPacket & FASTCALL Copy(const UhttProcessorPacket & rS);
	void FASTCALL SetName(const char * pName);
	void FASTCALL SetSymb(const char * pSymb);

	int    ID;
	SString Name;
	SString Symb;
	int    ParentID;
	int    Kind;
	int    Flags;
	int    LocID;
	int    LinkObjType;
	int    LinkObjID;
	int    CipPersonKindID;
	int    CipMax;
	TSCollection <UhttPrcPlaceDescription> Places;
};

class UhttTSessLine {
public:
	UhttTSessLine();

	int    TSessID;
	int    OprNo;
	int    GoodsID;
	int    LotID;
	int    UserID;
	int    Sign;
	UhttTimestamp Tm;
	int    Flags;
	double Qtty;
	double WtQtty;
	double Price;
	double Discount;
	UhttTimestamp Expiry;
	SString Serial;
};

class UhttCipPacket {
public:
	UhttCipPacket();
	void FASTCALL SetMemo(const char * pText);
	void FASTCALL SetPlaceCode(const char * pText);

	int    ID;     // Идентификатор ассоциации. В пределах одного раздела БД строка идентифицируется этим полем.
	int    Kind;   // Вид ассоциации.
	int    PrmrID; // Идентификатор первичного объекта, для которого фиксируется регистрация (например, PPObjTSession)
	int    PersonID; // ->Person.ID
	int    Num;      // Номер записи в однородном списке относительно заданного первичного объекта
	int    RegCount; // Количество персоналий, включенное в данную регистрацию. Как правило =1,
		// но в некоторых случаях может быть >1, например, для анонимной или групповой регистрации
	int    CiCount;  // Количество персоналий, включенное в данную регистрацию со статусом CheckedIn. Как правило =(0|1),
		// но в некоторых случаях может быть >1, например, для анонимной или групповой регистрации
	int    Flags;   // @flags
	UhttTimestamp RegTm;  // Время регистрации //
	UhttTimestamp CiTm;   // Время подтверждения регистрации (CheckID)
	double Amount;   // Сумма, уплаченная или которая должна быть уплачена за регистрацию (подтверждение)
	int    CCheckID; // Кассовый чек, которым оплачено подтверждение регистрации
	int    SCardID;  // Карта, с которой ассоциирована зерегистрированная персоналия //
	SString Memo;      // Примечание
	SString PlaceCode; // Номер места (для регистрации, ассоциированной с посадочным местом)
};

class UhttTSessionPacket {
public:
	UhttTSessionPacket();
	UhttTSessionPacket & FASTCALL operator = (const UhttTSessionPacket & rS);
	UhttTSessionPacket & FASTCALL Copy(const UhttTSessionPacket & rS);
	void FASTCALL SetMemo(const char * pText);
	void FASTCALL SetDetail(const char * pText);

	int    ID;
	int    Num; // Номер сессии (уникальный по процессору)
	int    PrcID;
	int    TechID;
	int    ParentID;
	int    Status;
	int    Flags;
	UhttTimestamp StTime;
	UhttTimestamp FinTime;
	SString Memo;
	SString Detail;
	TSCollection <UhttTSessLine> Lines;
	TSCollection <UhttCipPacket> Cips;
	TSCollection <UhttPrcPlaceDescription> Places;
	TSCollection <UhttTagItem> TagList;
};

class UhttSmsPacket {
public:
	SString From;
	SString To;
	SString Text;
};
//
// Descr: Инициализирует параметры openssl
//
int gSoapSslClientContextInit(struct soap * pSoap, const char * pKeyfile);
//
// Descr: Инициализирует вызов методов gSoap
//
int gSoapClientInit(struct soap * pSoap, long flags, const char * pKeyfile);

typedef int (*UHTT_DESTROYRESULT)(void *);
typedef SString * (*UHTTAUTH_PROC)(PPSoapClientSession & rSess, const char * pName, const char * pPassword);
typedef UhttPersonPacket * (*UHTTGETPERSONBYID_PROC)(PPSoapClientSession & rSess, const char * pToken, int id);
typedef TSCollection <UhttPersonPacket> * (*UHTTGETPERSONBYNAME_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pName);
typedef TSCollection <UhttBrandPacket> * (*UHTTGETBRANDBYNAME_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pName);
typedef int (*UHTTGETGOODSREFLIST_PROC)(PPSoapClientSession & rSess, const char * pToken, TSArray <UhttCodeRefItem> & rList);
typedef TSCollection <UhttGoodsPacket> * (*UHTTGETGOODSBYCODE_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pCode);
typedef TSCollection <UhttGoodsPacket> * (*UHTTGETGOODSBYNAME_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pName);
typedef UhttGoodsPacket * (*UHTTGETGOODSBYID_PROC)(PPSoapClientSession & rSess, const char * pToken, int id);
typedef SString *  (*UHTTGETGOODSARCODE_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pGoodsCode, const char * pPersonINN);
typedef UhttStatus * (*UHTTCREATEGOODS_PROC)(PPSoapClientSession & rSess, const char * pToken, const UhttGoodsPacket & rPack);
typedef UhttStatus * (*UHTTSETIMAGEBYID_PROC)(PPSoapClientSession & rSess, const char * pToken, UhttDocumentPacket & rPack);
typedef TSCollection <UhttSpecSeriesPacket> * (*UHTTGETSPECSERIESBYPERIOD_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pPeriod);
typedef UhttStatus * (*UHTTCREATESPECSERIES_PROC)(PPSoapClientSession & rSess, const char * pToken, const UhttSpecSeriesPacket & rPack);
typedef UhttSCardPacket * (*UHTTGETSCARDBYNUMBER_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pNumber);
typedef UhttStatus * (*UHTTCREATESCARD_PROC)(PPSoapClientSession & rSess, const char * pToken, UhttSCardPacket & rPack);
typedef UhttStatus * (*UHTTCREATESCARDCHECK_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pLocSymb, const char * pSCardNumber, const UhttCheckPacket & rPack);
typedef UhttStatus * (*UHTTDEPOSITSCARDAMOUNT_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pNumber, const double amount);
typedef UhttStatus * (*UHTTWITHDRAWSCARDAMOUNT_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pNumber, const double amount);
typedef int (*UHTTGETSCARDREST_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pNumber, const char * pDate, double & rRest);
typedef int (*UHTTSTARTDATATRANSFER_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pName, int64 totalRawSize, int32 chunkCount);
typedef int (*UHTTTRANSFERDATA_PROC)(PPSoapClientSession & rSess, const char * pToken, int transferID, int chunkNumber, int64 rawChunkSize, char * pMime64Data);
typedef int (*UHTTFINISHTRANSFERDATA_PROC)(PPSoapClientSession & rSess, const char * pToken, int transferID);
typedef int (*UHTTADDFILEVERSION_PROC)(PPSoapClientSession & rSess, const char * pToken, int transferID, const char * pKey, const char * pLabel, const char * pMemo);
typedef UhttLocationPacket * (*UHTTGETLOCATIONBYID_PROC)(PPSoapClientSession & rSess, const char * pToken, int ID);
typedef UhttLocationPacket * (*UHTTGETLOCATIONBYCODE_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pCode);
typedef TSCollection <UhttLocationPacket> * (*UHTTGETLOCATIONLISTBYPHONE_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pPhone);
typedef UhttStatus * (*UHTTCREATESTANDALONELOCATION_PROC)(PPSoapClientSession & rSess, const char * pToken, const UhttLocationPacket & rPack);
typedef int (*UHTTCREATEBILL_PROC)(PPSoapClientSession & rSess, const char * pToken, UhttBillPacket & rPack);
typedef TSCollection <UhttBillPacket> * (*UHTTGETBILL_PROC)(PPSoapClientSession & rSess, const char * pToken, const UhttBillFilter & rFilt);
typedef int (*UHTTSETQUOT_PROC)(PPSoapClientSession & rSess, const char * pToken, const UhttQuotPacket & rPack);
typedef TSCollection <UhttStatus> * (*UHTTSETQUOTLIST_PROC)(PPSoapClientSession & rSess, const char * pToken, const TSCollection <UhttQuotPacket> & rList);
typedef TSCollection <UhttQuotPacket> * (*UHTTGETQUOT_PROC)(PPSoapClientSession & rSess, const char * pToken, const UhttQuotFilter & rFilt);
typedef TSCollection <UhttGoodsRestListItem> * (*UHTTGETGOODSRESTLIST_PROC)(PPSoapClientSession & rSess, const char * pToken, int uhttGoodsID);
typedef TSCollection <UhttDCFileVersionInfo> * (*UHTTDCGETFILEVERSIONLIST_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pKey);
typedef UhttWorkbookItemPacket * (*UHTTGETWORKBOOKITEMBYID_PROC)(PPSoapClientSession & rSess, const char * pToken, int ID);
typedef UhttWorkbookItemPacket * (*UHTTGETWORKBOOKITEMBYCODE_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pCode);
typedef TSCollection <UhttWorkbookItemPacket> * (*UHTTGETWORKBOOKLISTBYPARENTCODE_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pCode);
typedef UhttDocumentPacket * (*UHTTGETWORKBOOKCONTENTBYID_PROC)(PPSoapClientSession & rSess, const char * pToken, int id);
typedef UhttStatus * (*UHTTCREATEWORKBOOKITEM_PROC)(PPSoapClientSession & rSess, const char * pToken, const UhttWorkbookItemPacket & rPack);
typedef UhttStatus * (*UHTTSETWORKBOOKCONTENTBYID_PROC)(PPSoapClientSession & rSess, const char * pToken, UhttDocumentPacket & rPack);
typedef UhttStyloDevicePacket * (*UHTTGETSTYLODEVICEBYID_PROC)(PPSoapClientSession & rSess, const char * pToken, int ID);
typedef UhttStyloDevicePacket * (*UHTTGETSTYLODEVICEBYCODE_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pCode);
typedef UhttStatus * (*UHTTCREATESTYLODEVICE_PROC)(PPSoapClientSession & rSess, const char * pToken, const UhttStyloDevicePacket & rPack);
typedef UhttProcessorPacket * (*UHTTGETPROCESSORBYID_PROC)(PPSoapClientSession & rSess, const char * pToken, int id);
typedef UhttProcessorPacket * (*UHTTGETPROCESSORBYCODE_PROC)(PPSoapClientSession & rSess, const char * pToken, const char * pCode);
typedef UhttStatus * (*UHTTCREATEPROCESSOR_PROC)(PPSoapClientSession & rSess, const char * pToken, const UhttProcessorPacket & rPack);
typedef UhttTSessionPacket * (*UHTTGETTSESSIONBYID_PROC)(PPSoapClientSession & rSess, const char * pToken, int id);
typedef UhttTSessionPacket * (*UHTTGETTSESSIONBYUUID_PROC)(PPSoapClientSession & rSess, const char * pToken, const S_GUID & id);
typedef TSCollection <UhttTSessionPacket> * (*UHTTGETTSESSIONBYPRC_PROC)(PPSoapClientSession & rSess, const char * pToken, int prcID, const UhttTimestamp * pSince);
typedef UhttStatus * (*UHTTCREATETSESSION_PROC)(PPSoapClientSession & rSess, const char * pToken, const UhttTSessionPacket & rPack);
typedef TSCollection <UhttStatus> * (*UHTTSENDSMS_PROC)(PPSoapClientSession & rSess, const char * pToken, const TSCollection <UhttSmsPacket> & rPack);
typedef SString * (*UHTTGETCOMMONMQSCONFIG_PROC)(PPSoapClientSession & rSess);
//
// iSales PEPSI SOAP CLIENT
//
struct iSalesUOM {
	iSalesUOM();
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);

	int    Code;     // 0 - штука, 1 - коробка, 2 - условная коробка
    double Width;    // Ширана, см
    double Height;   // Высота, см
    double Length;   // Длина, см
    double Netto;    // Масса нетто, кг
    double Brutto;   // Масса брутто, кг
    SString Barcode; // Штрихкод
};

struct iSalesUOMCvt {
	iSalesUOMCvt();
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);

	SString UomFrom;
	SString UomTo;
	double Rate;
};

struct iSalesGoodsPacket {
	iSalesGoodsPacket();
	iSalesGoodsPacket(const iSalesGoodsPacket & rS);
	iSalesGoodsPacket & FASTCALL Copy(const iSalesGoodsPacket & rS);
	iSalesGoodsPacket & FASTCALL operator = (const iSalesGoodsPacket & rS);
	double RecalcUnits(long uomFrom, long uomTo, double qtty) const;
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);

    int    TypeOfProduct;
    SString OuterCode;
    SString NativeCode;
    SString Name;
    SString Abbr;
    double VatRate;
    int    UnitCode; // 0 - штука, 1 - коробка, 2 - условная коробка
    int    Valid;    // 0 - active, 1 - passive
    SString CountryName;
    SString CLB;     // ГТД
    TSCollection <iSalesUOM> UomList;
    TSCollection <iSalesUOMCvt> CvtList;
    SString ErrMsg;
};

struct iSalesVisit {
    SString Ident;
    int    Valid;	   // 0 - active, 1 - passive
    int    Freq;   // Частота (2 - раз в 2 недели)
    int    DayOfWeek; // День недели [1..7]
    LDATE  InitDate;
    SString Order; // Порядок посещения
    SString OuterClientCode;
    SString InnerClientCode;
};

struct iSalesRoutePacket {
	iSalesRoutePacket();
	iSalesRoutePacket(const iSalesRoutePacket & rS);
	iSalesRoutePacket & FASTCALL Copy(const iSalesRoutePacket & rS);
	iSalesRoutePacket & FASTCALL operator = (const iSalesRoutePacket & rS);

    int    TypeOfRoute; // Pre sell - 0, Van sell - 1
    int    Valid;	    // 0 - active, 1 - passive
    SString Ident;
    SString Code;
    SString NativeAgentCode;
    TSCollection <iSalesVisit> VisitList;
    //
    SString ErrMsg;
};

struct iSalesCustomerPacket {
	SString Name;
	SString Title;
	SString ShortName;
	SString OuterCode;       // Код клиента или адреса доставки у контрагента
	SString NativeCode;      // Код клиента или адреса доставки у нас
	SString OuterOwnerCode;  // Код головного подразделения у контрагента
	SString NativeOwnerCode; // Код головного подразделения у нас
	SString INN;
	SString KPP;
	int    TaxScheme; // 0 - плательщик НДС, 1 - УСНО
    //
    SString ErrMsg;
};

struct iSalesStockCountingItem {
	SString OuterCode;
	int   Type; // Тип остатков: 0 - годные, 1 - брак, 2 - резерв
	int   UnitCode; // 0 - штука, 1 - коробка, 2 - условная коробка
	double Qtty; // Количество на остатке
};

struct iSalesStockCountingWhPacket {
	int   WhID;
	SString WhCode; // Символ склада
	TSCollection <iSalesStockCountingItem> Items;
};

struct iSalesPriceItem {
	SString OuterCode; // Код товара у контрагента
	double Price;
};

struct iSalesPriceListPacket {
    int    PriceListID;
    StringSet OuterCliCodeList;
    TSCollection <iSalesPriceItem> Prices;
};

struct iSalesBillAmountEntry {
	iSalesBillAmountEntry();

	int    SetType;       // 0 - Значения, рассчитанные на основе базовых цен, 1 - Значения, которые д.б. в реальности, 2 - Значения, которые будут печататься в документах
	double NetPrice;
	double GrossPrice;
	double NetSum;
	double DiscAmount;
	double DiscNetSum;
	double VatSum;
	double GrossSum;
	double DiscGrossSum;
};

struct iSalesBillItem {
	iSalesBillItem();

	int    LineN; // Номер строки
	SString OuterGoodsCode;  // Outer-код товара
	SString NativeGoodsCode; // Native-код товара
	SString Country; // Страна происхождения
	SString CLB;     // Номер ГТД

	int    UnitCode;
	double Qtty;
	SString Memo; // DESC
	TSCollection <iSalesBillAmountEntry> Amounts;
};
//
// Descr: Атрибуты связанного документа
//
struct iSalesBillRef {
	int    DocType; // Тип связанного документа
	SString Code;   // Номер связанного документа
	LDATETIME Dtm;  // Дата/время связанного документа
};
//
// Descr: Дополнительный атрибут передачи данных от iSales
//
struct iSalesExtAttr {
	SString Name;
	SString Value;
};

struct iSalesBillPacket {
	iSalesBillPacket();

	long   NativeID;
	SString iSalesId;
	int    DocType; // 0 - отгрузка, 1 - документ реализации, 2 - счет на оплату, 3 - возврат от покупателя,
		// 4 - возврат обратной реализации, 5 - возврат по акту, 6 - накладная доставки/приход, 7 - документ поступления,
		// 8 - оплата дистибьютору, 9 - взаимозачет, 10 - движение долга между клиентами, 11 - списание/начисление долга,
		// 12 - аванс от клиента, 13 - заказ от клиента, 14 - выплата (дистрибьютора контрагенту)
	int    ExtDocType; // (должно быть пусто)
	int    Status; // 0 - проведен, 1 - отменен
	SString Code;    // Номер документа
	SString ExtCode; // Номер внешнего документа
	LDATETIME Dtm;    // Дата/время документа (передается строкой dd.mm.yyyy hh:mm:ss)
	LDATETIME IncDtm; // Дата входящего документа (должно быть пусто), для заказа - дата исполнения
	LDATETIME ExtDtm; // Дата/время внешнего документа (передается строкой dd.mm.yyyy hh:mm:ss)
	LDATETIME CreationDtm; // Дата/время создания документа (передается строкой dd.mm.yyyy hh:mm:ss)
	LDATETIME LastUpdDtm;  // Дата/время последнего изменения документа (передается строкой dd.mm.yyyy hh:mm:ss)
	LDATE   DueDate;  // Срок оплаты
	SString ShipFrom; // Код грузоотправителя (iSales код дистрибьютора при отгрузке на клиента, Native-код клиента при возврате)
	SString ShipTo;   // Код грузополучателя (Native-код клиента (точки доставки) при документе реализации, iSales код дистрибьютора при возврате)
    SString SellerCode; // Код продавца (iSales код дистрибьютора при отгрузке на клиента, Native-код клиента при возврате)
	SString PayerCode;  // Код плательщика (Native-код клиента при отгрузке на клиента, iSales код дистрибьютора при возврате)
	SString Memo;      // Примечание
	SString SrcLocCode;  // Код склада-отправителя
	SString DestLocCode; // Код склада-получателя
	SString AgentCode;   // Native-код агента
	SString AuthId;      // Ключевой пользователь (Native-код), кто зарегистрировал документ в iSales
	SString EditId;      // Ключевой пользователь (Native-код), кто изменил документ в iSales
	SString ErrMsg;      // Сообщение об ошибке, приходящее от сервера
	TSCollection <iSalesBillItem> Items;
	TSCollection <iSalesBillAmountEntry> Amounts;
	TSCollection <iSalesBillRef> Refs;
	TSCollection <iSalesExtAttr> Attrs; // @v9.3.1
};

struct iSalesBillDebt {
	SString iSalesId;
	int   DocType;
	SString Code;
	LDATETIME Dtm;
	SString PayerCode;  // Код плательщика (Native-код клиента при отгрузке на клиента, iSales код дистрибьютора при возврате)
	double Amount;
	double Debt;
	//
	SString ErrMsg;
};

struct iSalesTransferStatus {
	enum {
		ifcUnkn = 0,
		ifcOrder,      // ORDS
		ifcInvoice,    // DOCL
		ifcReceipt,    // DOCS
		ifcReturnReq   // VDOC 
	};
	int    Ifc;
	SString Ident; // Идентификатор созданного на принимающей стороне объекта (принимающая стороная отправляет iSalesTransferStatus)
};

typedef TSCollection <iSalesGoodsPacket> * (*ISALESGETGOODSLIST_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const DateRange * pPeriod);
typedef TSCollection <iSalesRoutePacket> * (*ISALESGETROUTELIST_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const DateRange * pPeriod);
typedef TSCollection <iSalesBillPacket>  * (*ISALESGETORDERLIST_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const DateRange * pPeriod, int inclProcessedItems);
typedef TSCollection <iSalesBillPacket>  * (*ISALESGETRECEIPTLIST_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const DateRange * pPeriod, int inclProcessedItems);
typedef TSCollection <iSalesBillPacket>  * (*ISALESGETVDOCLIST_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const DateRange * pPeriod, int inclProcessedItems);
typedef TSCollection <iSalesBillDebt>    * (*ISALESGETUNCLOSEDBILLLIST_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const DateRange * pPeriod);
typedef SString * (*ISALESPUTSTOCKCOUNTING_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const TSCollection <iSalesStockCountingWhPacket> * pItems);
typedef SString * (*ISALESPUTPRICES_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const TSCollection <iSalesPriceListPacket> * pItems);
typedef SString * (*ISALESPUTBILLS_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const TSCollection <iSalesBillPacket> * pItems, uint maxItems);
typedef SString * (*ISALESPUTTRANSFERSTATUS_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const TSCollection <iSalesTransferStatus> * pItems);
typedef SString * (*ISALESPUTDEBTSETTLEMENT_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const TSCollection <iSalesBillDebt> * pItems);
//
//
//
struct SapEfesCallHeader {
	SapEfesCallHeader();

	const char * P_SalesOrg;
	const char * P_Wareh;
	long   SessionID;
};

enum SapEfesUnitType {
    sapefesUnitItem = 1,  // "ST"
    sapefesUnitPack,      // "PCG"
    sapefesUnitMultipack, // "MPK"
    spaefesUnitLiter      // "L"
};

struct SapEfesBillItem {
    int    PosN;   // Номер позиции
    int    UnitType;
    int    BaseUnitType;
    double Qtty;   // Количество в выбранной единице измерения
    double QtyN;   // Числитель коэффициента выбранной единицы по отношению к базовой
    double QtyD;   // Знаменатель коэффициента выбранной единицы по отношению к базовой
	double Amount; // Сумма без НДС по строке
    SString PosType;
    SString GoodsCode;
    SString Currency;
};

struct SapEfesContractor {
    SString Code;
    SString Name;
    SString Addr;
    SString City;
    SString Region;
    SString ZIP;
};

struct SapEfesOrder {
	SapEfesOrder();

	LDATETIME Date; // Время создания документа
	LDATE  DueDate; // Дата поставки
	double Amount;  // Сумма заказа без НДС
    SString Code;
    SString DocType;
    SString Status;
    SString PaymTerm;
    SString ShipCond;
    SString ShipCondDescr;
    SString SalesOrg;
    SString DistrChannel;
    SString Division;
    SString Warehouse;
    SString TerrIdent;
    SString Currency;
    SString Memo;
    SapEfesContractor Buyer;
    SapEfesContractor DlvrLoc;
    TSCollection <SapEfesBillItem> Items;
};

struct SapEfesLogMsg {
	enum {
		tS = 1,
		tW = 2,
		tE = 3
	};
	int    MsgType;
	SString Msg;
};

struct SapEfesBillStatus {
    SString NativeCode;
    SString Code;
    SString Status;
	TSCollection <SapEfesLogMsg> MsgList;
};

struct SapEfesBillPacket {
	SapEfesBillPacket();

	enum {
		fHasOrderRef = 0x0001,
		fReturn      = 0x0002
	};
	enum {
		tRetail = 1, // ZDSD
		tReturn,     // ZRET
		tBonus,      // ZDS1
		tBonusEfes   // ZDS2
	};
	long   NativeID; // Идентификатор документа в собственной БД (поставщику не передается)
	int    Flags;
    int    DocType;
    LDATE  Date;
    LDATE  DueDate;
    SString NativeCode;
    SString OrderCode;
    SString BuyerCode;
    SString DlvrLocCode;
    SString Memo;
    TSCollection <SapEfesBillItem> Items;
};

struct SapEfesGoodsReportEntry {
	LDATE  Dt;
	int    UnitType;
	double Qtty;
	SString DlvrLocCode;
    SString GoodsCode;
};

struct SapEfesDebtReportEntry {
	SapEfesDebtReportEntry();

	long   NativeArID; // @anchor(strictly the first member) @v9.5.2 Ид статьи в нашей базе данных
	double Debt;
	double CreditLimit;
	long   PayPeriod;
	long   DebtDelayDays; // Задолженность в днях
	SString BuyerCode;
	SString Memo;
};

struct SapEfesDebtDetailReportEntry {
	SapEfesDebtDetailReportEntry();

	long   NativeArID; // @anchor(strictly the first member) @v9.5.2 Ид статьи в нашей базе данных
	LDATE  BillDate;
	LDATE  PaymDate;
	double Amount;
	double Debt;
	SString BuyerCode;
	SString NativeBillCode;
};

typedef TSCollection <SapEfesOrder> * (*EFESGETSALESORDERSYNCLIST_PROC)(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const DateRange * pPeriod, int repeat, const char * pDocNumberList);
typedef int (*EFESSETSALESORDERSTATUSSYNC_PROC)(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesBillStatus> * pItems);
typedef TSCollection <SapEfesBillStatus> * (*EFESSETDELIVERYNOTESYNC_PROC)(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesBillPacket> * pItems);
typedef TSCollection <SapEfesBillStatus> * (*EFESCANCELDELIVERYNOTESYNC_PROC)(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SString> * pItems);
typedef TSCollection <SapEfesLogMsg> * (*EFESSETDAILYSTOCKREPORTSYNC_PROC)(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesGoodsReportEntry> * pItems);
typedef TSCollection <SapEfesLogMsg> * (*EFESSETMTDPRODUCTREPORTSYNC_PROC)(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesGoodsReportEntry> * pItems);
typedef TSCollection <SapEfesLogMsg> * (*EFESSETMTDOUTLETSREPORTSYNC_PROC)(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesGoodsReportEntry> * pItems);
typedef TSCollection <SapEfesLogMsg> * (*EFESSETDEBTSYNC_PROC)(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesDebtReportEntry> * pItems);
typedef TSCollection <SapEfesLogMsg> * (*EFESSETDEBTDETAILSYNC_PROC)(PPSoapClientSession & rSess, const SapEfesCallHeader & rH, const TSCollection <SapEfesDebtDetailReportEntry> * pItems);
//
//
//
struct SfaHeinekenWarehouseBalanceEntry {
	int    ForeignGoodsID;
	int    ForeignLocID;
	int    Rest;
};

struct SfaHeinekenOrderStatusEntry {
	S_GUID OrderUUID;
	/*enum ns1__Statuses {
		ns1__Statuses__formed = 0,
		ns1__Statuses__accepted = 1,
		ns1__Statuses__shipped = 2,
		ns1__Statuses__comments = 3
	};*/
	int    Status;
	SString Comments;
};

/*struct SfaHeinekenDeliveryPosition {
	int    SkuID;
	double Volume;
	double Count;
	double Amount;
	SString Comments;
};

struct SfaHeinekenOrderDelivery {
	S_GUID OrderUuid;
	TSCollection <SfaHeinekenDeliveryPosition> DeliveryList;
};

struct SfaHeinekenDistributorDelivery {
	SString InnerOrderCode;
	int    SalePointID;
	TSCollection <SfaHeinekenDeliveryPosition> DeliveryList;
};

struct SfaHeinekenSalePointDelivery {
	SString InnerOrderCode;
	SString DistributorOrderID; // Для отправки приходных документов
	int    InnerDlvrLocID;
	int    ForeignLocID; // Склад
	SString DlvrLocName;
	SString DlvrLocAddr;
	TSCollection <SfaHeinekenDeliveryPosition> DeliveryList;
};*/

struct SfaHeinekenInvoice {
	struct DeliveryPosition {
		int    SkuID;
		double Volume;
		double Count;
		double Amount;
		SString Comments;
	};
	struct OrderDelivery {
		S_GUID OrderUuid;
		TSCollection <DeliveryPosition> DeliveryList;
	};
	struct DistributorDelivery {
		SString InnerOrderCode;
		int    SalePointID;
		TSCollection <DeliveryPosition> DeliveryList;
	};
	struct SalePointDelivery {
		SString InnerOrderCode;
		SString DistributorOrderID; // Для отправки приходных документов
		int    InnerDlvrLocID;
		int    ForeignLocID; // Склад
		SString DlvrLocName;
		SString DlvrLocAddr;
		TSCollection <DeliveryPosition> DeliveryList;
	};
	SString Code;
	LDATE  Dt;
	TSCollection <OrderDelivery> OrderList; // Если доставка по заказу из системы Jeans
	TSCollection <DistributorDelivery> DistributorDeliveryList; // Если доставка вне заказа из системы Jeans, по ТТ из системы Jeans
	TSCollection <SalePointDelivery> DistributorSalePointDeliveryList; // Если доставка вне заказа из системы Jeans, по торговой точке, остсутствующей в системе Jeans
};

struct SfaHeinekenDebetEntry {
	int    ContragentID;
	double DebetSum;
	double DebetLimit;
};

typedef SString * (*SFAHEINEKENGETSKUASSORTIMENT_PROC)(PPSoapClientSession & rSess);
typedef SString * (*SFAHEINEKENGETORDERS_PROC)(PPSoapClientSession & rSess, LDATE dt, int demo);
typedef SString * (*SFAHEINEKENSENDORDERSSTATUSES_PROC)(PPSoapClientSession & rSess, const TSCollection <SfaHeinekenOrderStatusEntry> & rList);
typedef SString * (*SFAHEINEKENSENDWAREHOUSEBALANCE_PROC)(PPSoapClientSession & rSess, const TSVector <SfaHeinekenWarehouseBalanceEntry> & rList);
typedef SString * (*SFAHEINEKENSENDSELLOUT_PROC)(PPSoapClientSession & rSess, const TSCollection <SfaHeinekenInvoice> & rList);
typedef SString * (*SFAHEINEKENSENDSELLIN_PROC)(PPSoapClientSession & rSess, const TSCollection <SfaHeinekenInvoice> & rList);
typedef SString * (*SFAHEINEKENDELETESELLOUT_PROC)(PPSoapClientSession & rSess, const SString & rCode, LDATE date);
typedef SString * (*SFAHEINEKENDELETESELLIN_PROC)(PPSoapClientSession & rSess, const SString & rCode, LDATE date);
typedef SString * (*SFAHEINEKENSENDALLCONTRAGENTDEBET_PROC)(PPSoapClientSession & rSess, const TSCollection <SfaHeinekenDebetEntry> & rList);
//extern "C" __declspec(dllexport) SString * SfaHeineken_SendAllContragentDebet(PPSoapClientSession & rSess, const TSCollection <SfaHeinekenDebetEntry> & rList) // DRP_SendAllContragentDebet
//extern "C" __declspec(dllexport) SString * SfaHeineken_GetOrders(PPSoapClientSession & rSess, LDATE dt, int demo) // DRP_GetOrders
//
//
//
