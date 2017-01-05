// ALCODECLBILL.CPP
// Библиотека для экспорта алкогольных накладных в xml
//

// В результате выгрузки создается группа из трех файлов: список контрагентов (производителей-импортеров),
// список алкоголя и список пива.
// Файлы записываются в подкаталоги с названиями - именами контрагентов.
// В каждом подкаталоге файлы с алкоголем, пивом, контрагентами (производителями). Количество пар файлов с инфой
// о продукции равно количеству магазинов этого контрагента. Файл с контрагентами в подкаталоге один.
//

#include <slib.h>
#include <ppbrow.h>
#include <libxml.h>
#include <libxml\xmlwriter.h>
#include <libxml\xmlreader.h>

#define EXPORT	extern "C" __declspec (dllexport)
#define THROWERR(expr,val) { if(!(expr)){ SetError(val, ""); goto __scatch; } }
#define THROWERR_STR(expr,val,str) { if(!(expr)){ SetError(val, str); goto __scatch; } }

// Имена элементов xml-файла
#define ELEMENT_NAME_FILE			"Файл"
#define ELEMENT_NAME_DOCUMENT		"Документ"
#define ELEMENT_NAME_TURN			"Оборот"
#define ELEMENT_NAME_MANUFDATA		"СведПроизвИмпорт"
#define ELEMENT_NAME_PRODUCT		"Продукция"
#define ELEMENT_NAME_MOVEMENT		"Движение"
#define ELEMENT_NAME_CATALOG		"Справочники"
#define ELEMENT_NAME_REQUISITES		"Реквизиты"
#define ELEMENT_NAME_RESPPERS		"ОтветЛицо"	// Ответственное лицо
#define ELEMENT_NAME_CONTRAGS		"Контрагенты"
#define ELEMENT_NAME_RESIDENT		"Резидент"
#define ELEMENT_NAME_LICENSES		"Лицензии"
#define ELEMENT_NAME_LICENSE		"Лицензия"
#define ELEMENT_NAME_P000000000008	"П000000000008"	// Адрес организации
#define ELEMENT_NAME_COUNTRYCODE	"КодСтраны"
#define ELEMENT_NAME_INDEX			"Индекс"
#define ELEMENT_NAME_REGIONCODE		"КодРегион"
#define ELEMENT_NAME_DISTRICT		"Район"
#define ELEMENT_NAME_TOWN			"Город"
#define ELEMENT_NAME_COMMUNITY		"НаселПункт"
#define ELEMENT_NAME_STREET			"Улица"
#define ELEMENT_NAME_HOUSE			"Дом"
#define ELEMENT_NAME_HOUSING		"Корпус"
#define ELEMENT_NAME_LETTER			"Литера"
#define ELEMENT_NAME_SQUARE			"Кварт"
#define ELEMENT_NAME_LPERSON		"ЮЛ"
#define ELEMENT_NAME_PRODUCER		"Производитель"
#define ELEMENT_NAME_TRANSPORTER	"Перевозчик"

// Имена атрибутов xml-файла
#define ATRIBUTE_NAME_DATEDOC		"ДатаДок"		// Дата выгрузки
#define ATRIBUTE_NAME_VERSFORM		"ВерсФорм"
#define ATRIBUTE_NAME_PROGNAME		"НаимПрог"
#define ATRIBUTE_NAME_PN			"ПN"			// Порядковый номер
#define ATRIBUTE_NAME_P000000000003	"П000000000003"	// Код вида продукиции
#define ATRIBUTE_NAME_P000000000007	"П000000000007"	// Наименование организации
#define ATRIBUTE_NAME_P000000000009	"П000000000009"	// ИНН Организации
#define ATRIBUTE_NAME_P000000000010	"П000000000010"	// КПП организации
#define ATRIBUTE_NAME_P000000000011	"П000000000011"	// Серия,номер лицензии
#define ATRIBUTE_NAME_P000000000012	"П000000000012"	// Дата начала лицензии
#define ATRIBUTE_NAME_P000000000013	"П000000000013"	// Дата окончания лицензии
#define ATRIBUTE_NAME_P000000000014	"П000000000014"	// Регистрирующий орган
#define ATRIBUTE_NAME_NAMEORG		"NameOrg"
#define ATRIBUTE_NAME_INN			"INN"
#define ATRIBUTE_NAME_KPP			"KPP"
#define ATRIBUTE_NAME_P200000000013	"П200000000013"	// Дата накладной
#define ATRIBUTE_NAME_P200000000014	"П200000000014"	// Номер ТТН
#define ATRIBUTE_NAME_P200000000015	"П200000000015"	// ГТД
#define ATRIBUTE_NAME_P200000000016	"П200000000016"	// Количество в декалитрах
#define ATRIBUTE_NAME_P100000000006	"П100000000006"
#define ATRIBUTE_NAME_P100000000007	"П100000000007"
#define ATRIBUTE_NAME_P100000000008	"П100000000008"	// Общее количество товара в декалитрах
#define ATRIBUTE_NAME_P100000000009	"П100000000009"
#define ATRIBUTE_NAME_P100000000010	"П100000000010"	// Общее количество товара в декалитрах
#define ATRIBUTE_NAME_P100000000011	"П100000000011"
#define ATRIBUTE_NAME_P100000000012	"П100000000012"
#define ATRIBUTE_NAME_P100000000013	"П100000000013"
#define ATRIBUTE_NAME_P100000000014	"П100000000014"	// Общее количество товара в декалитрах
#define ATRIBUTE_NAME_P100000000015	"П100000000015"
#define ATRIBUTE_NAME_P100000000016	"П100000000016"
#define ATRIBUTE_NAME_P100000000017	"П100000000017"
#define ATRIBUTE_NAME_P100000000018	"П100000000018"
#define ATRIBUTE_NAME_P100000000019	"П100000000019"
#define ATRIBUTE_NAME_P100000000020	"П100000000020"
#define ATRIBUTE_NAME_IDCONTR		"ИдКонтр"
#define ATRIBUTE_NAME_IDLICENSE		"ИдЛицензии"
#define ATRIBUTE_NAME_VALUE			"value"

// Другие константы
//#define KVARTAL_1		3	// 1 квартал
//#define KVARTAL_2		6	// 2 квартал
//#define KVARTAL_3		9	// 3 квартал
//#define KVARTAL_4		0	// 4 квартал

// Коды ошибок и сообщений
#define IEERR_SYMBNOTFOUND			1			// Символ не найден (символ типа объекта импорта/экспорта)
#define IEERR_NODATA				2			// Нет данных
#define IEERR_NOSESS				3			// Сессии с таким номером не существует
#define IEERR_ONLYBILLS				4			// Данная DLL может работать только с документами
#define IEERR_NOOBJID				5			// Объекта с таким идентификатором нет
#define IEERR_IMPEXPCLSNOTINTD		6			// Объект для импорта/экспорта не инициализирован
#define IERR_IMPFILENOTFOUND		7			// Файл импорта не найден: %s
#define IERR_INVMESSAGEYTYPE		8			// Неверный тип сообщения. Ожидается %S
#define IERR_ARRAYNOTINITED			9			// Ошибка инициализации массива

class ExportCls;

int ErrorCode = 0;
SString StrError = "";
static ExportCls * P_ExportCls = 0;

int GetObjTypeBySymb(const char * pSymb, uint & rType);
int SetError(int errCode, const char * pStr = "") { ErrorCode = errCode, StrError = pStr; return 1; }

struct ErrMessage {
	uint Id;
	const char * P_Msg;
};

struct ObjectTypeSymbols {
	char * P_Symb;
	uint Type;
};

enum ObjectType {
	objGood = 1,
	objBill,
	objCheck,
	objCashSess,
	objPriceList,
	objLot,
	objPhoneList,
	objCliBnkData
	// и т.д.
};

ObjectTypeSymbols Symbols[] = {
	{"GOODS",       objGood},
	{"BILLS",		objBill},
	{"CHECKS",		objCheck},
	{"CSESS",		objCashSess},
	{"PRICELIST",	objPriceList},
	{"LOTS",		objLot},
	{"PHONELIST",	objPhoneList},
	{"CLIBANKDATA",	objCliBnkData}
};

ErrMessage ErrMsg[] = {
	{IEERR_SYMBNOTFOUND,		"Символ не найден"},
	{IEERR_NODATA,				"Данные не переданы"},
	{IEERR_NOSESS,				"Сеанса с таким номером нет"},
	{IEERR_ONLYBILLS,			"Dll может работать только с документами"},
	{IEERR_NOOBJID,				"Объекта с таким идентификатором нет"},
	{IEERR_IMPEXPCLSNOTINTD,	"Объект для импорта/экспорта не инициализирован"},
	{IERR_IMPFILENOTFOUND,		"Файл импорта не найден: "},
	{IERR_INVMESSAGEYTYPE,		"Неверный тип сообщения. Ожидается "},
	{IERR_ARRAYNOTINITED,		"Ошибка инициализации массива"}
};

enum FileTypes {
	fContrag = 0x0001,
	fBeer = 0x0002,
	fAlco = 0x0004,
	fReturn = 0x0008
};

class Iterator {
public:
	void Init()
	{
		Count = 0;
	}
	const uint GetCount()
	{
		return Count;
	}
	void Next()
	{
		Count++;
	}
private:
	uint   Count;
};

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return true;
}

struct GoodInfoSt {
	GoodInfoSt()
	{
		Clear();
	}
	void Clear()
	{
		Quantity = 0.0;
		ReceiptDate = ZERODATE;
		GoodKind = 0;
		OrgName = 0;
		INN = 0;
		KPP = 0;
		TTN = 0;
		GTD = 0;
	};
	double Quantity;
	LDATE  ReceiptDate;
	long   GoodKind;
	SString OrgName;
	SString INN;
	SString KPP;
	SString	TTN;
	SString GTD;
};

struct ContragInfoSt {
	ContragInfoSt()
	{
		Clear();
	}
	void Clear()
	{
		House = 0;
		CountryCode = 0;
		RegionCode = 0;
		Date = ZERODATE;
		Expiry = ZERODATE;
		Index = 0;
		ContragName = 0;
		LicOrgName = 0;
		LicID = 0;
		LicSerial = 0;
		LicNum = 0;
		District = 0;
		Town = 0;
		Community = 0;
		Street = 0;
		Housing = 0;
		Letter = 0;
		INN = 0;
		KPP = 0;
		Producer = 0;
		Transporter = 0;
		IsManuf = 0;
	}
	int		House;
	int		CountryCode;
	int		RegionCode;
	int		IsManuf;
	long	LicID;
	LDATE	Date;
	LDATE	Expiry;
	SString	Index;
	SString ContragName;
	SString LicOrgName;
	SString LicSerial;
	SString LicNum;
	SString	District;
	SString Town;
	SString Community;
	SString Street;
	SString	Housing;
	SString	Letter;
	SString INN;
	SString KPP;
	SString Producer;
	SString Transporter;
};

struct ShopInfoSt {
	ShopInfoSt()
	{
		Init();
	}
	~ShopInfoSt()
	{
		Clear();
	}
	void Init()
	{
		ShopId = 0;
		ContragName = 0;
		ShopAddr = 0;
		ShopAlco.freeAll();
		ShopBeer.freeAll();
		//ShopContrag.freeAll();
	}
	void Clear()
	{
		ShopAlco.freeAll();
		ShopBeer.freeAll();
		//ShopContrag.freeAll();
	}
	long   ShopId;
	SString ContragName;
	SString ShopAddr;
	TSCollection <GoodInfoSt> ShopAlco;
	TSCollection <GoodInfoSt> ShopBeer;
	TSCollection <GoodInfoSt> ShopAlcoRet; // @vmiller new
	TSCollection <GoodInfoSt> ShopBeerRet; // @vmiller new
	//TSCollection <ContragInfoSt> ShopContrag;
};
//
// Заметка:
// Имя выходного файла передаваться не будет. Только если путь.
// Более того, будет три имени файла как минимум.
// Один общий, где список контрагентов
// Два других для разных контрагентов, а значит, с разными именами. Надо запоминать, что у нас уже есть.
// При выгрузке каждой накладной, выходные файлы сохраняем. Если контрагент повторяется, то читаем и дополняем уже сознаные файлы,
// которые к нему относятся.
//
class ExportCls {
public:
	ExportCls();
	~ExportCls();
	void   CreateFileName(long DocType, const char * pContragName, const char * pShopAddr, uint grpNum, SString & rFileName);
	//
	// Descr: Заполняем массивы с инфой об алкоголе и пиве
	//
	int    GetInfo(Sdr_BRow * pBRow, TSCollection <GoodInfoSt> * pArr);
	int    GetContragInfo(Sdr_BRow * pBRow, TSCollection <ContragInfoSt> * pArr); // Заполняем массив с инфой о контрагентах
	int    SaveInfo(TSCollection <GoodInfoSt> * pArr, xmlTextWriterPtr pXmlPtr, int isBeer = 0); // Сохраняем данные об алкоголе и пиве в соответствующие файлы
	int    SaveContragInfo(TSCollection <ContragInfoSt> * pArr); // Сохраняем данные о контрагентах в файл

	uint   Id;					// ИД, который Papyrus будет воспринимать как ИД сеанса экспорта
	uint   ObjId;				// ИД экспортируемого объекта (например, для пакета документов каждому документу соответствует свой ИД)
	uint   ObjType;				// Тип экспортируемых объектов
	uint   Inited;
	//uint   Kvart;				// Номер квартала отчетности. Для имени выходног файла
	//uint   Year;				// Год отчетности. Для имени выходного файла
	//long BeerTypeId;			// ИД группы товара "Пиво"
	long   ShopPos;				// Позиция инфы в массиве ShopInfo для текущего магазина
	LDATE  BillDate;
	xmlTextWriterPtr P_WXmlAlco;
	xmlTextWriterPtr P_WXmlBeer;
	xmlTextWriterPtr P_WXmlAlcoRet; // @vmiller new
	xmlTextWriterPtr P_WXmlBeerRet; // @vmiller new
	xmlTextWriterPtr P_WXmlContrag;
	Iterator Itr;
	SPathStruc PathStruct;
	SString Prefix;				// Префикс имени документа. Раз уж стали указывать, пусть будет
	Sdr_ImpExpHeader Header;
	TSCollection <ShopInfoSt> ShopInfo;	// Массив групп массивов для каждого магазина
	TSCollection <ContragInfoSt> ContragInfo; // Массив с инфой о контрагентах (производеителей/импортеров)
private:
	void   GetPeriod(LDATE billDate, int quarterNum, int year);
	SString & DeleteExtSymbols(const char * pIn, SString & rOut);
};

ExportCls::ExportCls()
{
	Id = 0;
	ObjId = 0;
	ObjType = 0;
	ShopPos = 0;
	Inited = 0;
//	BeerTypeId = 0; // @vmiller
	BillDate = ZERODATE;
	P_WXmlAlco = 0;
	P_WXmlBeer = 0;
	P_WXmlAlcoRet = 0; // @vmiller new
	P_WXmlBeerRet = 0; // @vmiller new
	P_WXmlContrag = 0;
	ErrorCode = 0;
	ShopInfo.freeAll();
	ContragInfo.freeAll();
}

ExportCls::~ExportCls()
{
	P_WXmlAlco = 0;
	P_WXmlBeer = 0;
	P_WXmlAlcoRet = 0; // @vmiller new
	P_WXmlBeerRet = 0; // @vmiller new
	P_WXmlContrag = 0;
	ShopInfo.freeAll();
	ContragInfo.freeAll();
}

static SString & PreprocessFnText(SString & rT)
{
	rT.ReplaceChar(',', ' ').ReplaceChar('\\', ' ').ReplaceChar('/', ' ').ReplaceChar('.', ' ').
		ReplaceChar('\"', ' ').ReplaceChar('*', ' ');
	rT.ReplaceStr("  ", " ", 0).Strip();
	return rT;
}

void ExportCls::CreateFileName(long DocType, const char * pContragName, const char * pShopAddr, uint grpNum, SString & rFileName)
{
	int doc_num = 1, exit_while = 0;
	SString fmt, dir;
	SFile file;
	while(!exit_while) {
		(rFileName = 0).Cat(PathStruct.Drv).CatChar(':').Cat(PathStruct.Dir);
		if(DocType & fContrag) {
			// Папка с именем конграгента
			if(pContragName) {
				PreprocessFnText(fmt = pContragName);
				rFileName.Cat(fmt).CatChar('\\');
				dir = rFileName;
			}
			//rFileName.Cat("Контрагенты_GRP").Cat(grpNum).CatChar('(').Cat(doc_num).CatChar(')').Dot().Cat(PathStruct.Ext);
			rFileName.Cat("Контрагенты").CatChar('(').Cat(/*doc_num*/1).CatChar(')').Dot().Cat(PathStruct.Ext);
			// Если такой файл уже существует, то создаем файл с номером, большим на единицу

			// уже не создаем
			exit_while = 1;
		}
		else {
			// Папка с именем контрагента
			if(pContragName) {
				PreprocessFnText(fmt = pContragName);
				rFileName.Cat(fmt).CatChar('\\');
				dir = rFileName;
			}
			// Возврат
			if(DocType & fReturn)
				rFileName.Cat("Return_");
			// Префикс
			rFileName.Cat(Prefix);
			// Тип товаров, которые будут перечислены в файле (крепкий алкоголь или пиво)
			if(DocType & fAlco)
				rFileName.Cat("11_");
			else if(DocType & fBeer)
				rFileName.Cat("12_");
			// Номер квартала (две цифры)
			//Year = BillDate.year();
			//Kvart = (uint)ceil((double)BillDate.month() / 3);
			//if(Kvart <= 3)
			//	Kvart = KVARTAL_1;
			//else if((Kvart <= 6) && (Kvart >= 4))
			//	Kvart = KVARTAL_2;
			//else if((Kvart <= 9) && (Kvart >=5))
			//	Kvart = KVARTAL_3;
			//else if(Kvart >= 10)
			//	Kvart = KVARTAL_4;
			//rFileName.CatChar('0').Cat(Kvart);
			//// Первая цифра года
			//(fmt = 0).Cat(BillDate.year());
			//rFileName.CatChar(fmt.C(0)).CatChar('_');
			// Дата отгрузки
			/*LDATE date;
			getcurdate(&date);
			(fmt = 0).Cat(date.day());
			if(fmt.Len() == 1)
				fmt.PadLeft(1, '0');
			rFileName.Cat(fmt);
			(fmt = 0).Cat(date.month());
			if(fmt.Len() == 1)
				fmt.PadLeft(1, '0');
			rFileName.Cat(fmt).Cat(date.year());*/
			// От себя добавим ИД магазина
			//rFileName.CatChar('_').Cat(shopId).CatChar('_');
			// Адрес магазина
			if(pShopAddr) {
				PreprocessFnText(fmt = pShopAddr);
				rFileName.Cat(fmt);
			}
			rFileName.CatChar('(').Cat(doc_num).CatChar(')').Dot().Cat(PathStruct.Ext);
			// Номер файла
			//rFileName.Cat("GRP").Cat(grpNum).CatChar('(').Cat(doc_num).CatChar(')').Dot().Cat(PathStruct.Ext);
		}
		if(pContragName && !pathValid(dir, 1))
			createDir(dir);
		// Если файл с таким именем существует, увеличиваем номер файла
		if(!file.Open(rFileName, SFile::mRead))
			exit_while = 1;
		else
			doc_num++;
	}
}

int ExportCls::GetInfo(Sdr_BRow * pBRow, TSCollection <GoodInfoSt> * pList)
{
	int    ok = 1;
	THROWERR(pBRow, IEERR_NODATA);
	THROWERR(pList, IERR_ARRAYNOTINITED);
	{
		GoodInfoSt * p_new_item = new GoodInfoSt;
		p_new_item->GoodKind = pBRow->GoodKindCode;
		p_new_item->OrgName = pBRow->LotManuf;
		p_new_item->INN = pBRow->ManufINN;
		p_new_item->KPP = pBRow->ManufKPP;
		p_new_item->ReceiptDate = pBRow->BillDate/*LotDocDate*/;
		p_new_item->TTN = /*pBRow->TTN*/pBRow->BillCode; // У заказчика номер докумена совпадает с номером ТТН
		p_new_item->GTD = pBRow->GTD;
		p_new_item->Quantity = fabs(pBRow->PhQtty);
		pList->insert(p_new_item);
	}
	CATCHZOK
	return ok;
}

int ExportCls::GetContragInfo(Sdr_BRow * pBRow, TSCollection <ContragInfoSt> * pArr)
{
	int    ok = 1, same_found = 0;
	THROWERR(pBRow, IEERR_NODATA);
	for(size_t i = 0; (i < pArr->getCount()) && !same_found; i++) {
		const ContragInfoSt * p_item = pArr->at(i);
		if(p_item) {
			if(!p_item->ContragName.CmpNC(pBRow->LotManuf) && !p_item->INN.CmpNC(pBRow->ManufINN) &&
				!p_item->KPP.CmpNC(pBRow->ManufKPP) && !p_item->LicSerial.CmpNC(pBRow->LicenseSerial) &&
				!p_item->LicNum.CmpNC(pBRow->LicenseNum) && (p_item->LicID == pBRow->LicenseID))
				same_found = 1;
		}
	}
	if(!same_found) {
		ContragInfoSt * p_new_item = new ContragInfoSt;
		p_new_item->ContragName = pBRow->LotManuf;
		p_new_item->LicOrgName = pBRow->RegAuthority;
		p_new_item->Expiry = pBRow->LicenseExpiry;
		p_new_item->Date = pBRow->LicenseDate;
		p_new_item->LicSerial = pBRow->LicenseSerial;
		p_new_item->LicNum = pBRow->LicenseNum;
		p_new_item->LicID = pBRow->LicenseID;
		p_new_item->CountryCode = 643; // @vmiller
		p_new_item->Index = pBRow->ManufIndex;
		p_new_item->RegionCode = pBRow->ManufRegionCode;
		p_new_item->District = pBRow->ManufDistrict;
		p_new_item->Town = pBRow->ManufCityName;
		p_new_item->Community = 0;
		p_new_item->Street = pBRow->ManufStreet;
		p_new_item->House = pBRow->ManufHouse;
		p_new_item->Housing = pBRow->ManufHousing;
		p_new_item->Letter = 0;
		p_new_item->INN = pBRow->ManufINN;
		p_new_item->KPP = pBRow->ManufKPP;
		p_new_item->IsManuf = pBRow->IsManuf;
		pArr->insert(p_new_item);
	}
	CATCHZOK
	return ok;
}

int ExportCls::SaveInfo(TSCollection <GoodInfoSt> * pArr, xmlTextWriterPtr pXmlPtr, int isBeer/* = 0*/)
{
	// index - порядковый номер записи оборота
	int    ok = 1, index = 1;
	long   good_kind_code = 0;
	double total_qtt = 0.0;
	SString str, fmt;
	LDATE  date = ZERODATE;
	TSCollection <GoodInfoSt> one_grp;
	SString org_name = "", inn = "", kpp = "";

	THROW(pXmlPtr);

	xmlTextWriterStartElement(pXmlPtr, (const xmlChar*)ELEMENT_NAME_FILE);
		xmlTextWriterStartAttribute(pXmlPtr, (const xmlChar*)ATRIBUTE_NAME_DATEDOC);
			getcurdate(&date);
			(fmt = 0).Cat(date.day());
			if(fmt.Len() == 1)
				fmt.PadLeft(1, '0');
			(str = 0).Cat(fmt);
			(fmt = 0).Cat(date.month());
			if(fmt.Len() == 1)
				fmt.PadLeft(1, '0');
			str.Dot().Cat(fmt).Dot().Cat(date.year());
			xmlTextWriterWriteString(pXmlPtr, (const xmlChar*)(const char *)str);
		xmlTextWriterEndAttribute(pXmlPtr); //ДатаДок
		xmlTextWriterStartAttribute(pXmlPtr, (const xmlChar*)ATRIBUTE_NAME_VERSFORM);
			xmlTextWriterWriteString(pXmlPtr, (const xmlChar*)"4.20"); // Версия ДекларантАлко @v8.6.12 "4.20"-->"4.30"; @v8.7.11 "4.30"-->"4.20"
		xmlTextWriterEndAttribute(pXmlPtr); //ВерсФорм
		xmlTextWriterStartAttribute(pXmlPtr, (const xmlChar*)ATRIBUTE_NAME_PROGNAME);
			xmlTextWriterWriteString(pXmlPtr, (const xmlChar*)Header.SrcSystemName);
		xmlTextWriterEndAttribute(pXmlPtr); //НаимПрог

		xmlTextWriterStartElement(pXmlPtr, (const xmlChar*)ELEMENT_NAME_DOCUMENT);
		// GoodsKind
		// OrgName
		// ReceiptDate
			while(pArr->getCount()) {
				// Сортируем по виду товара
				GoodInfoSt good_info;
				const GoodInfoSt * p_item = pArr->at(0);
				good_kind_code = p_item->GoodKind;
				//
				// Запишем данные о виде товара
				//
				xmlTextWriterStartElement(pXmlPtr, (const xmlChar*)ELEMENT_NAME_TURN);
					xmlTextWriterStartAttribute(pXmlPtr, (const xmlChar*)ATRIBUTE_NAME_PN);
						xmlTextWriterWriteString(pXmlPtr, (str = 0).Cat(index).ucptr());
					xmlTextWriterEndAttribute(pXmlPtr); //ПN
					xmlTextWriterStartAttribute(pXmlPtr, (const xmlChar*)ATRIBUTE_NAME_P000000000003);
						xmlTextWriterWriteString(pXmlPtr, (str = 0).Cat(good_kind_code).ucptr());
					xmlTextWriterEndAttribute(pXmlPtr); //П0000000003
				for(uint j = 0; j < pArr->getCount();) {
					one_grp.freeAll();
					total_qtt = 0.0;
					const GoodInfoSt * p_item_j = pArr->at(j);
					org_name = p_item_j->OrgName;
					inn = p_item_j->INN;
					kpp = p_item_j->KPP;
					for(uint i = 0; i < pArr->getCount();) {
						//
						// Сортируем по контрагенту
						//
						const GoodInfoSt * p_item_i = pArr->at(i);
						if((p_item_i->GoodKind == good_kind_code) && p_item_i->OrgName.CmpNC(org_name) == 0 && p_item_i->INN.CmpNC(inn) == 0 && p_item_i->KPP.CmpNC(kpp) == 0) {
							GoodInfoSt * p_new_item = new GoodInfoSt;
							*p_new_item = *p_item_i;
							one_grp.insert(p_new_item);
							pArr->atFree(i);
						}
						else
							i++;
					}
					if(one_grp.getCount()) {
						// Расположим записи в порядке возрастания даты
						for(size_t k = 0; k < one_grp.getCount(); k++) {
							const  GoodInfoSt * p_item_k = one_grp.at(k);
							LDATE  min_date = p_item_k->ReceiptDate;
							size_t min_index = k;
							for(size_t d = 0; d < one_grp.getCount(); d++) {
								const  GoodInfoSt * p_item_d = one_grp.at(d);
								if(p_item_d->ReceiptDate < min_date) {
									min_date = p_item_d->ReceiptDate;
									min_index = d;
								}
								one_grp.swap(d, min_index);
							}
						}
						// Вот теперь можно записывать данные
							xmlTextWriterStartElement(pXmlPtr, (const xmlChar*)ELEMENT_NAME_MANUFDATA);
								xmlTextWriterStartAttribute(pXmlPtr, (const xmlChar*)ATRIBUTE_NAME_NAMEORG);
									xmlTextWriterWriteString(pXmlPtr, org_name.ucptr());
								xmlTextWriterEndAttribute(pXmlPtr); //NameOrg
								xmlTextWriterStartAttribute(pXmlPtr, (const xmlChar*)ATRIBUTE_NAME_INN);
									xmlTextWriterWriteString(pXmlPtr, inn.ucptr());
								xmlTextWriterEndAttribute(pXmlPtr); //INN
								xmlTextWriterStartAttribute(pXmlPtr, (const xmlChar*)ATRIBUTE_NAME_KPP);
									xmlTextWriterWriteString(pXmlPtr, kpp.ucptr());
								xmlTextWriterEndAttribute(pXmlPtr); //KPP
								// Пишем инфу о каждом товаре
								for(size_t k = 0; k < one_grp.getCount(); k++) {
									const  GoodInfoSt * p_item_k = one_grp.at(k);
									xmlTextWriterStartElement(pXmlPtr, (const xmlChar*)ELEMENT_NAME_PRODUCT);
										xmlTextWriterStartAttribute(pXmlPtr, (const xmlChar*)ATRIBUTE_NAME_P200000000013);
											(fmt = 0).Cat(p_item_k->ReceiptDate.day());
											if(fmt.Len() == 1)
												fmt.PadLeft(1, '0');
											(str = 0).Cat(fmt);
											(fmt = 0).Cat(p_item_k->ReceiptDate.month());
											if(fmt.Len() == 1)
												fmt.PadLeft(1, '0');
											str.Dot().Cat(fmt).Dot().Cat(p_item_k->ReceiptDate.year());
											xmlTextWriterWriteString(pXmlPtr, (const xmlChar*)(const char *)str);
										xmlTextWriterEndAttribute(pXmlPtr); //П20000000013
										xmlTextWriterStartAttribute(pXmlPtr, (const xmlChar*)ATRIBUTE_NAME_P200000000014);
											xmlTextWriterWriteString(pXmlPtr, (const xmlChar*)(const char *)p_item_k->TTN);
										xmlTextWriterEndAttribute(pXmlPtr); //П20000000014
										xmlTextWriterStartAttribute(pXmlPtr, (const xmlChar*)ATRIBUTE_NAME_P200000000015);
											(str = p_item_k->GTD).Transf(CTRANSF_INNER_TO_OUTER); // Почему-то здесь без преобразования выводится шляпа, а не кириллица
											xmlTextWriterWriteString(pXmlPtr, (const xmlChar*)(const char *)/*p_item_k->GTD*/str);
										xmlTextWriterEndAttribute(pXmlPtr); //П20000000015
										xmlTextWriterStartAttribute(pXmlPtr, (const xmlChar*)ATRIBUTE_NAME_P200000000016);
											(str = 0).Cat(p_item_k->Quantity / 10); // В декалитрах
											total_qtt += p_item_k->Quantity / 10;
											xmlTextWriterWriteString(pXmlPtr, (const xmlChar*)(const char *)str);
										xmlTextWriterEndAttribute(pXmlPtr); //П20000000016
									xmlTextWriterEndElement(pXmlPtr); //Продукция
								}

							xmlTextWriterEndElement(pXmlPtr); //СведПроизвИмпорт
					}
					else
						j++;
				}
				xmlTextWriterEndElement(pXmlPtr); //Оборот
				index++;
			}
		xmlTextWriterEndElement(pXmlPtr); //Документ
	xmlTextWriterEndElement(pXmlPtr); //Файл
	CATCHZOK
	one_grp.freeAll();
	return ok;
}

int ExportCls::SaveContragInfo(TSCollection <ContragInfoSt> * pArr)
{
	int    ok = 1;
	SString str, fmt;
	xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_CATALOG);
	for(size_t i = 0; i < pArr->getCount(); i++) {
		const ContragInfoSt * p_item = pArr->at(i);
		xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_CONTRAGS);
			xmlTextWriterStartAttribute(P_WXmlContrag, (const xmlChar*)ATRIBUTE_NAME_IDCONTR);
				xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)(str = 0).Cat(i + 1));
			xmlTextWriterEndAttribute(P_WXmlContrag); //ИдКонтр
			xmlTextWriterStartAttribute(P_WXmlContrag, (const xmlChar*)ATRIBUTE_NAME_P000000000007);
				xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)p_item->ContragName);
			xmlTextWriterEndAttribute(P_WXmlContrag); //П000000000007
			xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_RESIDENT);
				if((p_item->Expiry != ZERODATE) && (p_item->Date != ZERODATE) && (p_item->LicOrgName) && p_item->LicSerial.NotEmpty() && p_item->LicNum) {
					xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_LICENSES);
						xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_LICENSE);
							xmlTextWriterStartAttribute(P_WXmlContrag, (const xmlChar*)ATRIBUTE_NAME_P000000000014);
								xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)p_item->LicOrgName);
							xmlTextWriterEndAttribute(P_WXmlContrag); //П000000000014
							xmlTextWriterStartAttribute(P_WXmlContrag, (const xmlChar*)ATRIBUTE_NAME_P000000000013);
								(fmt = 0).Cat(p_item->Expiry.day());
								if(fmt.Len() == 1)
									fmt.PadLeft(1, '0');
								(str = 0).Cat(fmt);
								(fmt = 0).Cat(p_item->Expiry.month());
								if(fmt.Len() == 1)
									fmt.PadLeft(1, '0');
								str.Dot().Cat(fmt).Dot().Cat(p_item->Expiry.year());
								xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)str);
							xmlTextWriterEndAttribute(P_WXmlContrag); //П000000000013
							xmlTextWriterStartAttribute(P_WXmlContrag, (const xmlChar*)ATRIBUTE_NAME_P000000000012);
								(fmt = 0).Cat(p_item->Date.day());
								if(fmt.Len() == 1)
									fmt.PadLeft(1, '0');
								(str = 0).Cat(fmt);
								(fmt = 0).Cat(p_item->Date.month());
								if(fmt.Len() == 1)
									fmt.PadLeft(1, '0');
								str.Dot().Cat(fmt).Dot().Cat(p_item->Date.year());
								xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)str);
							xmlTextWriterEndAttribute(P_WXmlContrag); //П000000000012
							xmlTextWriterStartAttribute(P_WXmlContrag, (const xmlChar*)ATRIBUTE_NAME_P000000000011);
								(str = p_item->LicSerial).Comma().Cat(p_item->LicNum);
								xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)str);
							xmlTextWriterEndAttribute(P_WXmlContrag); //П000000000011
							xmlTextWriterStartAttribute(P_WXmlContrag, (const xmlChar*)ATRIBUTE_NAME_IDLICENSE);
								xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)"1"); // 1 - Производство, хранение и поставки произведенного этилового
																							// спирта, в том числе денатурированного
							xmlTextWriterEndAttribute(P_WXmlContrag); //ИдЛицензии
						xmlTextWriterEndElement(P_WXmlContrag); //Лицензия
					xmlTextWriterEndElement(P_WXmlContrag); //Лицензии
				}
				xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_P000000000008);
					xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_COUNTRYCODE);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)(str = 0).Cat(p_item->CountryCode));
					xmlTextWriterEndElement(P_WXmlContrag); //КодСтраны
					xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_INDEX);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)(str = 0).Cat(p_item->Index));
					xmlTextWriterEndElement(P_WXmlContrag); //Индекс
					xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_REGIONCODE);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)(str = 0).Cat(p_item->RegionCode));
					xmlTextWriterEndElement(P_WXmlContrag); //КодРегион
					xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_DISTRICT);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)p_item->District);
					xmlTextWriterEndElement(P_WXmlContrag); //Район
					xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_TOWN);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)p_item->Town);
					xmlTextWriterEndElement(P_WXmlContrag); //Город
					xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_COMMUNITY);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)"");
					xmlTextWriterEndElement(P_WXmlContrag); //НаселПункт
					xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_STREET);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)p_item->Street);
					xmlTextWriterEndElement(P_WXmlContrag); //Улица
					xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_HOUSE);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)(str = 0).Cat(p_item->House));
					xmlTextWriterEndElement(P_WXmlContrag); //Дом
					xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_HOUSING);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)p_item->Housing);
					xmlTextWriterEndElement(P_WXmlContrag); //Корпус
					xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_LETTER);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)p_item->Letter);
					xmlTextWriterEndElement(P_WXmlContrag); //Литера
					xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_SQUARE);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)"");
					xmlTextWriterEndElement(P_WXmlContrag); //Кварт
				xmlTextWriterEndElement(P_WXmlContrag); //П000000000008
				xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_LPERSON);
					xmlTextWriterStartAttribute(P_WXmlContrag, (const xmlChar*)ATRIBUTE_NAME_P000000000009);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)p_item->INN);
					xmlTextWriterEndAttribute(P_WXmlContrag); //П000000000009
					xmlTextWriterStartAttribute(P_WXmlContrag, (const xmlChar*)ATRIBUTE_NAME_P000000000010);
						xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)(const char *)p_item->KPP);
					xmlTextWriterEndAttribute(P_WXmlContrag); //П000000000010
				xmlTextWriterEndElement(P_WXmlContrag); //ЮЛ
				xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_PRODUCER);
					xmlTextWriterStartAttribute(P_WXmlContrag, (const xmlChar*)ATRIBUTE_NAME_VALUE);
						if(p_item->IsManuf)
							xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)"True");
						else
							xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)"False");
					xmlTextWriterEndAttribute(P_WXmlContrag); //value
				xmlTextWriterEndElement(P_WXmlContrag); //Производитель
				xmlTextWriterStartElement(P_WXmlContrag, (const xmlChar*)ELEMENT_NAME_TRANSPORTER);
					xmlTextWriterStartAttribute(P_WXmlContrag, (const xmlChar*)ATRIBUTE_NAME_VALUE);
							xmlTextWriterWriteString(P_WXmlContrag, (const xmlChar*)"False");
					xmlTextWriterEndAttribute(P_WXmlContrag); //value
				xmlTextWriterEndElement(P_WXmlContrag); //Перевозчик
			xmlTextWriterEndElement(P_WXmlContrag); //Резидент
		xmlTextWriterEndElement(P_WXmlContrag); //Контрагенты
	}
	xmlTextWriterEndElement(P_WXmlContrag); //Справочники
	//pArr->freeAll(); // @vmiller
	return ok;
}

void ExportCls::GetPeriod(LDATE billDate, int quarterNum, int year)
{
	int month = billDate.month();
	quarterNum = (int)ceil((double)month / 3);
	year = billDate.year();
}

SString & ExportCls::DeleteExtSymbols(const char * pIn, SString & rOut)
{
	rOut.CopyFrom(pIn);
	for(size_t i = 0; i < rOut.Len();) {
		if((rOut.C(i) == '/') || (rOut.C(i) == '\\') || (rOut.C(i) == '.') || (rOut.C(i) == ',') ||
			(rOut.C(i) == '?'))
			rOut.Excise(i, 1);
		else
			i++;
	}
	return rOut;
}

// Returns:
//		0 - символ не найден
//		1 - символ найден
int GetObjTypeBySymb(const char * pSymb, uint & rType)
{
	for(size_t i = 0; i < SIZEOFARRAY(Symbols); i++) {
		if(strcmp(Symbols[i].P_Symb, pSymb) == 0) {
			rType = Symbols[i].Type;
			return 1;
		}
	}
	return 0;
}

//
// Внешние функции экспорта
//
EXPORT int InitExport(void * pExpHeader, const char * pOutFileName, int * pId)
{
	int ok = 1;

	if(!P_ExportCls) {
		P_ExportCls = new ExportCls;
	}
	if(P_ExportCls && !P_ExportCls->Inited) {
		if(pExpHeader)
			P_ExportCls->Header = *(Sdr_ImpExpHeader*)pExpHeader;
		if(!isempty(pOutFileName)) {
			P_ExportCls->PathStruct.Split(pOutFileName);
			P_ExportCls->PathStruct.Ext = "xml";
		}
		else {
			char fname[256];
			GetModuleFileName(NULL, fname, sizeof(fname));
			P_ExportCls->PathStruct.Split(fname);
			P_ExportCls->PathStruct.Dir.ReplaceStr("\\bin", "\\out", 1);
			P_ExportCls->PathStruct.Ext = "xml";
		}
		P_ExportCls->Id = 1;
		*pId = P_ExportCls->Id; // ИД сеанса экспорта
		// Получаем от провайдера конфигурации обмена
		P_ExportCls->Inited = 1;
	}
	THROWERR(P_ExportCls, IEERR_IMPEXPCLSNOTINTD);
	CATCHZOK
	return ok;
}

EXPORT int SetExportObj(uint idSess, const char * pObjTypeSymb, void * pObjData, int * pObjId)
{
	int    ok = 1;
	size_t pos = 0;
	THROWERR(P_ExportCls, IEERR_IMPEXPCLSNOTINTD);
	THROWERR(idSess == P_ExportCls->Id, IEERR_NOSESS);
	THROWERR(pObjData, IEERR_NODATA);
	THROWERR(GetObjTypeBySymb(pObjTypeSymb, P_ExportCls->ObjType), IEERR_SYMBNOTFOUND);
	THROWERR(P_ExportCls->ObjType == objBill, IEERR_ONLYBILLS);
	//P_ExportCls->BeerTypeId = ((Sdr_Bill *)pObjData)->BeerGrpID; // @vmiller
	P_ExportCls->BillDate = ((Sdr_Bill *)pObjData)->Date;
	P_ExportCls->Prefix = ((Sdr_Bill *)pObjData)->XmlPrefix;
	P_ExportCls->ObjId++;
	*pObjId = P_ExportCls->ObjId;

	if(((Sdr_Bill *)pObjData)->DlvrAddrID) {
		for(pos = 0; pos < P_ExportCls->ShopInfo.getCount(); pos++) {
			if(P_ExportCls->ShopInfo.at(pos)->ShopId == ((Sdr_Bill *)pObjData)->DlvrAddrID) {
				P_ExportCls->ShopPos = pos;
				break;
			}
		}
	}
	else {
		for(pos = 0; pos < P_ExportCls->ShopInfo.getCount(); pos++) {
			const ShopInfoSt * p_item = P_ExportCls->ShopInfo.at(pos);
			if(!p_item->ContragName.CmpNC(((Sdr_Bill *)pObjData)->CntragName) && !p_item->ShopId) {
				P_ExportCls->ShopPos = pos;
				break;
			}
		}
	}
	if((pos == P_ExportCls->ShopInfo.getCount()) || (P_ExportCls->ShopInfo.getCount() == 0)) {
		ShopInfoSt * p_new_item = new ShopInfoSt;
		P_ExportCls->ShopPos = pos;
		p_new_item->ShopId = ((Sdr_Bill *)pObjData)->DlvrAddrID;
		p_new_item->ShopAddr = ((Sdr_Bill *)pObjData)->DlvrAddr;
		p_new_item->ContragName = ((Sdr_Bill *)pObjData)->CntragName;
		P_ExportCls->ShopInfo.insert(p_new_item);
	}
	CATCHZOK
	return ok;
}

EXPORT int InitExportObjIter(uint idSess, uint objId)
{
	int    ok = 1;
	THROWERR(P_ExportCls, IEERR_IMPEXPCLSNOTINTD);
	THROWERR(idSess == P_ExportCls->Id, IEERR_NOSESS);
	THROWERR(objId == P_ExportCls->ObjId, IEERR_NOOBJID);
	P_ExportCls->Itr.Init();
	CATCHZOK
	return ok;
}

EXPORT int NextExportObjIter(uint idSess, uint objId, void * pBRow)
{
	int    ok = 1;
	THROWERR(P_ExportCls, IEERR_IMPEXPCLSNOTINTD);
	THROWERR(pBRow, IEERR_NODATA);
	THROWERR(idSess == P_ExportCls->Id, IEERR_NOSESS);
	THROWERR(objId == P_ExportCls->ObjId, IEERR_NOOBJID);
	{
		ShopInfoSt * p_item = P_ExportCls->ShopInfo.at(P_ExportCls->ShopPos);
		//if(P_ExportCls->BeerTypeId != ((Sdr_BRow *)pBRow)->GoodGrpID) // @vmiller
		if(((Sdr_BRow *)pBRow)->IsAlco) { // @vmiller
			if(((Sdr_BRow *)pBRow)->Quantity < 0) { // @vmiller new
				// Продажа
				THROW(P_ExportCls->GetInfo((Sdr_BRow *)pBRow, &p_item->ShopAlco))
			// @vmiller new {
			}
			else {
				// Возврат
				THROW(P_ExportCls->GetInfo((Sdr_BRow *)pBRow, &p_item->ShopAlcoRet))
			}
			// } @vmiller new
		}
		//else if(P_ExportCls->BeerTypeId == ((Sdr_BRow *)pBRow)->GoodGrpID) // @vmiller
		else if(((Sdr_BRow *)pBRow)->IsBeer) // @vmiller
			if(((Sdr_BRow *)pBRow)->Quantity < 0) { // @vmiller new
				// Продажа
				THROW(P_ExportCls->GetInfo((Sdr_BRow *)pBRow, &p_item->ShopBeer));
			// @vmiller new {
			}
			else {
				// Возврат
				THROW(P_ExportCls->GetInfo((Sdr_BRow *)pBRow, &p_item->ShopBeerRet));
			}
			// } @vmiller new
		if(((Sdr_BRow *)pBRow)->IsAlco || ((Sdr_BRow *)pBRow)->IsBeer)
			THROW(P_ExportCls->GetContragInfo((Sdr_BRow *)pBRow, &P_ExportCls->ContragInfo)); // Файл с контрагентами один
			//THROW(P_ExportCls->GetContragInfo((Sdr_BRow *)pBRow, &p_item->ShopContrag)); // Число файлов с контрагентами равно количеству адресов доставки
		P_ExportCls->Itr.Next();
	}
	CATCHZOK
	return ok;
}

EXPORT int EnumExpReceipt(void * pReceipt)
{
	return -1;
}

EXPORT int FinishImpExp()
{
	int    ok = 1;
	if(P_ExportCls) {
		SString alco_name, beer_name, contrag_name;
		for(size_t pos = 0; pos < P_ExportCls->ShopInfo.getCount(); pos++) {
			ShopInfoSt * p_item = P_ExportCls->ShopInfo.at(pos);
			if(p_item->ShopAlco.getCount()) {
				P_ExportCls->CreateFileName(fAlco, p_item->ContragName, p_item->ShopAddr, pos + 1, alco_name);
				P_ExportCls->P_WXmlAlco = xmlNewTextWriterFilename(alco_name, 0);
				if(P_ExportCls->P_WXmlAlco) {
					xmlTextWriterSetIndentString(P_ExportCls->P_WXmlAlco, (const xmlChar*)"\t");
					xmlTextWriterStartDocument(P_ExportCls->P_WXmlAlco, 0, "windows-1251", 0);
					P_ExportCls->SaveInfo(&p_item->ShopAlco, P_ExportCls->P_WXmlAlco);
					xmlTextWriterEndDocument(P_ExportCls->P_WXmlAlco);
					xmlFreeTextWriter(P_ExportCls->P_WXmlAlco);
					P_ExportCls->P_WXmlAlco = 0;
				}
			}
			if(p_item->ShopBeer.getCount()) {
				P_ExportCls->CreateFileName(fBeer, p_item->ContragName, p_item->ShopAddr, pos + 1, beer_name);
				P_ExportCls->P_WXmlBeer = xmlNewTextWriterFilename(beer_name, 0);
				if(P_ExportCls->P_WXmlBeer) {
					xmlTextWriterSetIndentString(P_ExportCls->P_WXmlBeer, (const xmlChar*)"\t");
					xmlTextWriterStartDocument(P_ExportCls->P_WXmlBeer, 0, "windows-1251", 0);
					P_ExportCls->SaveInfo(&p_item->ShopBeer, P_ExportCls->P_WXmlBeer, 1);
					xmlTextWriterEndDocument(P_ExportCls->P_WXmlBeer);
					xmlFreeTextWriter(P_ExportCls->P_WXmlBeer);
					P_ExportCls->P_WXmlBeer = 0;
				}
			}
			// @vmiller new {
			// Возвраты
			if(p_item->ShopAlcoRet.getCount()) {
				P_ExportCls->CreateFileName(fAlco | fReturn, p_item->ContragName, p_item->ShopAddr, pos + 1, alco_name);
				P_ExportCls->P_WXmlAlcoRet = xmlNewTextWriterFilename(alco_name, 0);
				if(P_ExportCls->P_WXmlAlcoRet) {
					xmlTextWriterSetIndentString(P_ExportCls->P_WXmlAlcoRet, (const xmlChar*)"\t");
					xmlTextWriterStartDocument(P_ExportCls->P_WXmlAlcoRet, 0, "windows-1251", 0);
					P_ExportCls->SaveInfo(&p_item->ShopAlcoRet, P_ExportCls->P_WXmlAlcoRet);
					xmlTextWriterEndDocument(P_ExportCls->P_WXmlAlcoRet);
					xmlFreeTextWriter(P_ExportCls->P_WXmlAlcoRet);
					P_ExportCls->P_WXmlAlcoRet = 0;
				}
			}
			if(p_item->ShopBeerRet.getCount()) {
				P_ExportCls->CreateFileName(fBeer | fReturn, p_item->ContragName, p_item->ShopAddr, pos + 1, beer_name);
				P_ExportCls->P_WXmlBeerRet = xmlNewTextWriterFilename(beer_name, 0);
				if(P_ExportCls->P_WXmlBeerRet) {
					xmlTextWriterSetIndentString(P_ExportCls->P_WXmlBeerRet, (const xmlChar*)"\t");
					xmlTextWriterStartDocument(P_ExportCls->P_WXmlBeerRet, 0, "windows-1251", 0);
					P_ExportCls->SaveInfo(&p_item->ShopBeerRet, P_ExportCls->P_WXmlBeerRet, 1);
					xmlTextWriterEndDocument(P_ExportCls->P_WXmlBeerRet);
					xmlFreeTextWriter(P_ExportCls->P_WXmlBeerRet);
					P_ExportCls->P_WXmlBeerRet = 0;
				}
			}
			// } @vmiller new
			// Контрагенты
			P_ExportCls->CreateFileName(fContrag, p_item->ContragName, 0, pos + 1, contrag_name);
			if(!fileExists(contrag_name)) {
				P_ExportCls->P_WXmlContrag = xmlNewTextWriterFilename(contrag_name, 0);
				if(P_ExportCls->P_WXmlContrag) {
					xmlTextWriterSetIndentString(P_ExportCls->P_WXmlContrag, (const xmlChar*)"\t");
					xmlTextWriterStartDocument(P_ExportCls->P_WXmlContrag, 0, "windows-1251", 0);
					P_ExportCls->SaveContragInfo(&P_ExportCls->ContragInfo);
					xmlTextWriterEndDocument(P_ExportCls->P_WXmlContrag);
					xmlFreeTextWriter(P_ExportCls->P_WXmlContrag);
					P_ExportCls->P_WXmlContrag = 0;
				}
			}
			// Число файлов с контрагентами равно количеству адресов доставки
			/*if(p_item->ShopContrag.getCount()) {
				P_ExportCls->CreateFileName(fContrag, 0, pos + 1, contrag_name);
				P_ExportCls->P_WXmlContrag = xmlNewTextWriterFilename(contrag_name, 0);
				if(P_ExportCls->P_WXmlContrag) {
					xmlTextWriterSetIndentString(P_ExportCls->P_WXmlContrag, (const xmlChar*)"\t");
					xmlTextWriterStartDocument(P_ExportCls->P_WXmlContrag, 0, "windows-1251", 0);
					P_ExportCls->SaveContragInfo(&p_item->ShopContrag);
					xmlTextWriterEndDocument(P_ExportCls->P_WXmlContrag);
					xmlFreeTextWriter(P_ExportCls->P_WXmlContrag);
					P_ExportCls->P_WXmlContrag = 0;
				}
			}*/
		}
		//// Файл с контрагентами один {
		//P_ExportCls->CreateFileName(fContrag, 0, 0, pos + 1, contrag_name);
		//P_ExportCls->P_WXmlContrag = xmlNewTextWriterFilename(contrag_name, 0);
		//if(P_ExportCls->P_WXmlContrag) {
		//	xmlTextWriterSetIndentString(P_ExportCls->P_WXmlContrag, (const xmlChar*)"\t");
		//	xmlTextWriterStartDocument(P_ExportCls->P_WXmlContrag, 0, "windows-1251", 0);
		//	P_ExportCls->SaveContragInfo(&P_ExportCls->ContragInfo);
		//	xmlTextWriterEndDocument(P_ExportCls->P_WXmlContrag);
		//	xmlFreeTextWriter(P_ExportCls->P_WXmlContrag);
		//	P_ExportCls->P_WXmlContrag = 0;
		//}
		//// }
	}
	ZDELETE(P_ExportCls);
	return ok;
}

EXPORT int GetErrorMessage(char * pMsg, uint bufLen)
{
	SString str = "";
	for(size_t i = 0; i < SIZEOFARRAY(ErrMsg); i++) {
		if(ErrMsg[i].Id == ErrorCode) {
			str.Cat(ErrMsg[i].P_Msg);
			break;
		}
	}
	if(ErrorCode == IERR_IMPFILENOTFOUND || IERR_INVMESSAGEYTYPE)
		str.Cat(StrError);
	memzero(pMsg, bufLen);
	if(str.NotEmpty() && pMsg)
		str.CopyTo(pMsg, bufLen < (str.Len() + 1) ? bufLen : (str.Len() + 1));
	ErrorCode = 0;
	StrError = "";
	return 1;
}
