// IMPEXPKORUS.CPP
// Библиотека для импорта/экспорта документов в xml
//
//
// ЭКСПОРТ
// Входные парметры от Papyrus
//		Документ:
//		сумма товарной позиции без ндс рассчитывается
//		цена товарной позиции без ндс рассчитывается
//		номер заказа - Bill.Code
//		дата документа - Bill.Date
//		GLN поставщика - Bill.GLN
//		GLN покупателя - Bill.AgentGLN / Bill.MainGLN
//		GLN места доставки - Bill.DlvrAddrCode / Bill.LocCode
//		дата доставки - Bill.DueDate/Bill.Date
//
//		необязательный параметр:
//			GLN плательщика - Bill.Obj2GLN / Bill.AgentGLN / Bill.MainGLN

//		Товарные строки:
//		штрихкод товара - BRow.Barcode
//		заказанное количество - BRow.Quantity
//		единицы измерения - BRow.UnitName (Штука или Кг)
//		цена товара с ндс - BRow.Cost
//		ставка ндс - BRow.VatRate
//
//		необязательный параметр:
//			артикул товара - BRow.ArCode
//			наименование товара - BRow.GoodsName
//
// Выходные параметры в Papyrus
//		Метод EnumExpReceipt() структуру Sdr_DllImpExpReceipt (ИД заказа в Papyrus и соответствующий ему GUID в системе EDI)
//
//	ИМПОРТ
//  Входные/выходные параметры и порядок их обмена:
//		Из Papyrus - Вид операции (ORDRSP или APERAK)
//					 Период, за который надо проверить докумуенты/статусы. Если его не указать, система EDI
//						просто не включит в параметры отбора это условие
//		Из Dll	   - В случае получения подтверждения - заполненная под максимуму структура Sdr_Bill
//					 В случае получения статуса - заполненные некоторые поля в Sdr_Bill
//		Из Papyrus - Сообщение о наличии документа заказа, на который получено подтверждение/статус. Метод ReplyImportObjStatus, структура Sdr_DllImpObjStatus
//		Если такой заказ в Papyrus есть:
//			Из Dll - В случае получения подтверждения заполненная структура Sdr_BRow
//
//	Выходные параметры в Papyrus
//	ПОЛУЧЕНИЕ ЗАКАЗА
//		Документ:
//			Дата документа - Bill.Date
//			Номер заказа, на который пришло подтверждение  - Bill.OrderBillNo
//			GLN покупателя - Bill.GLN
//			GLN поставщика - Bill.Obj2GLN
//			GLN места доставки - pBill.DlvrAddrCode
//			GLN плательщика - Bill.Obj2GLN
//
//		необязатаельный параметр:
//			Подтвержденная дата доставки - Bill.DueDate
//			Дата создания заказа - Bill.OrderDate
//			Сумма документа с НДС - Bill.Amount
//
//		Товарные строки:
//			Штрихкод товара - BRow.Barcode
//			Подтвержденное количество - BRow.Quantity
//			Единицы измерения - BRow.UnitName
//
//		необязательный параметр:
//			Артикул товара в системе покупатаеля - BRow.ArCode
//			Наименование товара - BRow.GoodsName
//			Количество в упаковке - BRow.UnitPerPack
//			Цена товара с НДС - BRow.Cost
//
//	ПОЛУЧЕНИЕ СТАТУСА
//		Номер заказ, на который пришел статус - Bill.OrderBillNo
//		GLN места доставки - Bill.DlvrAddrCode / Bill.LocCode
//		GLN покупателя - Bill.MainGLN
//		GLN поставщика - Bill.GLN
//		Дата создания заказа - Bill.OrderDate
//
//	Входные параметры из Papyrus
//		Статус заказа в Papyrus - 1 - есть такой заказ, 2 - нет такого заказа. Метод ReplyImportObjStatus, структура Sdr_DllImpObjStatus.
//
//
#include <slib.h>
#include <sxml.h>
#include <ppedi.h>
//#include <ppbrow.h>
//#include <libxml.h>
//#include <libxml\xmlwriter.h>
//#include <libxml\xmlreader.h>
#include <ppsoapclient.h>
#include <Korus\korusEDIWebServiceSoapProxy.h>

#define EXPORT	extern "C" __declspec (dllexport)
#define THROWERR(expr,val) { if(!(expr)){ SetError(val, ""); goto __scatch; } }
#define THROWERR_STR(expr,val,str) { if(!(expr)){ SetError(val, str); goto __scatch; } }

#define UNIT_NAME_KG			"КГ"
#define UNIT_NAME_PIECE			"Штука"
#define WEBSYS_TECH_GLN			"9999000000001"	// Служебный GLN сервиса, с которого отправляются сообщения
#define EMPTY_LISTMB_RESP		"<mailbox-response></mailbox-response>"
//
// Имена элементов xml-ответов web-сервиса
//
#define ELEMENT_NAME_RELATION		"relation"
#define ELEMENT_NAME_PARTNER_ILN	"partner-iln"		// Идентификатор партнера
//
// Значения элементов-идентификатров xml-ответов и запросов web-сервиса
//
#define ELEMENT_CODE_DIRECTION_IN	"IN"		// Входящий документ
#define ELEMENT_CODE_DIRECTION_OUT	"OUT"		// Исходящий документ
#define ELEMENT_CODE_TYPE_ORDRSP	"ORDRSP"	// Подтверждение заказа
#define ELEMENT_CODE_TYPE_APERAK	"APERAK"	// Статус документа
#define ELEMENT_CODE_TYPE_DESADV	"DESADV"	// Уведомление об отправке

// Имена элементов сообщения
#define ELEMENT_NAME_SG3		"SG3"
#define ELEMENT_NAME_CUX		"CUX"
#define ELEMENT_NAME_C504		"C504"
#define ELEMENT_NAME_E6347		"E6347"
#define ELEMENT_NAME_E6345		"E6345"
#define ELEMENT_NAME_E6343		"E6343"
#define ELEMENT_NAME_E7077		"E7077"
#define ELEMENT_NAME_C273		"C273"
#define ELEMENT_NAME_E7008		"E7008"
#define ELEMENT_NAME_E3039		"E3039"
#define ELEMENT_NAME_SG26		"SG26"
#define ELEMENT_NAME_SG17		"SG17"
#define ELEMENT_NAME_E1229		"E1229"
#define ELEMENT_NAME_C186		"C186"
#define ELEMENT_NAME_E6063		"E6063"
#define ELEMENT_NAME_E6060		"E6060"
#define ELEMENT_NAME_E6411		"E6411"
#define ELEMENT_NAME_E5153		"E5153"
#define ELEMENT_NAME_E5278		"E5278"
#define ELEMENT_NAME_E5283		"E5283"
#define ELEMENT_NAME_C243		"C243"
#define ELEMENT_NAME_E5025		"E5025"
#define ELEMENT_NAME_E5004		"E5004"
//
// Значения элементов-идентифиаторов
//
#define ELEMENT_CODE_E2005_17		"17"		// Дата/время доставки (ORDRSP/DESADV)	(возможны оба квалификатора)
#define ELEMENT_CODE_E2005_358		"358"		// Дата/время доставки (DESADV)	(возможны оба квалификатора)
#define ELEMENT_CODE_E2005_137		"137"		// Дата/время документа/сообщения
#define ELEMENT_CODE_E6345_RUB		"RUB"		// Рубли
#define ELEMENT_CODE_E7077_F		"F"			// Код формата описания товара - текст
#define ELEMENT_CODE_E6063_21		"21"		// Идентификатор заказанного количества
#define ELEMENT_CODE_E6063_194		"194"		// Идентификатор принятого количества
#define ELEMENT_CODE_E6063_59		"59"		// Идентификатор количества товара в упаковке
#define ELEMENT_CODE_E6063_113		"113"		// Идентификатор подтвержденного количества (индедификатор может любой из предложенных)
#define ELEMENT_CODE_E6063_170		"170"		// Идентификатор подтвержденного количества (индедификатор может любой из предложенных)
#define ELEMENT_CODE_E6063_12		"12"		// Идентификатор отгруженного количества (DESADV)
#define ELEMENT_CODE_E6411_PCE		"PCE"		// Единицы измерения - Отдельные элементы
#define ELEMENT_CODE_E6411_KGM		"KGM"		// Единицы измерения - Килограммы
#define ELEMENT_CODE_E5025_XB5		"XB5"		// Идентификатор цены товара с НДС (DESADV)
#define ELEMENT_CODE_E4451_AAO		"AAO"		// Идентификатор кода описания статуса
#define ELEMENT_CODE_E2005_171		"171"		// Идентификатор даты документа (в APERAK)
#define ELEMENT_CODE_E1153_CT		"CT"		// Идентификатор номера договара на поставку
//
// Коды ошибок и сообщений
//
#define IEERR_SYMBNOTFOUND			1			// Символ не найден (символ типа объекта импорта/экспорта)
#define IEERR_NODATA				2			// Нет данных
#define IEERR_NOSESS				3			// Сессии с таким номером не существует
#define IEERR_ONLYBILLS				4			// Данная DLL может работать только с документами
#define IEERR_NOOBJID				5			// Объекта с таким идентификатором нет
#define IEERR_IMPEXPCLSNOTINTD		6			// Объект для импорта/экспорта не инициализирован
#define IEERR_WEBSERVСERR			7			// Ошибка веб-сервиса. В этом случае смотрится значение кода WebSrevcErr
#define IEERR_SOAP					8			// Ошибка soap-протокола. В этом случае описание ошибки содержится в SoapError
#define IEERR_IMPFILENOTFOUND		9			// Файл импорта не найден: %s
#define IEERR_INVMESSAGEYTYPE		10			// Неверный тип сообщения. Ожидается %S
#define IEERR_NOCFGFORGLN			11			// При данном типе операции нет настроек конфигурации для пользователя с GLN %s
#define IEERR_NULLWRIEXMLPTR		12			// Нулевой xmlWriter
#define IEERR_NULLREADXMLPTR		13			// Нулевой xmlReader
#define IEERR_XMLWRITE				14			// Ошибка записи в xml-файл
#define IEERR_XMLREAD				15			// Ошибка чтения из xml-файла
#define IEERR_TOOMUCHGOODS			16			// Число товаров, заявленных в файле импорта, меньше реального их количества
#define IEERR_MSGSYMBNOTFOUND		17			// Символ не найден (символ типа операции импорта/экспорта)
#define IEERR_ONLYIMPMSGTYPES		18			// Операция импорта работает только с типами команд ORDRSP, DESADV и APERAK
#define IEERR_NOCONFIG				19			// Не найдена конфигурация для данного типа операции
#define IEERR_NOEXPPATH				20			// Не определена директория для файла экспорта
#define IEERR_ONLYEXPMSGTYPES		21			// Операция экспорта работает только с типами команд ORDER и RECADV
#define IEERR_NOMEM                 22          // Не достаточно памяти
//
// Коды ошибок веб-сервиса
//
#define IEWEBERR_AUTH				1			// Ошибка аутентификации
#define IEWEBERR_CORRELATION		2			// Ошибка во взаимосвязи
#define IEWEBERR_EXTERNAL			3			// Внешняя ошибка
#define IEWEBERR_SERVER				4			// Внутрення ошибка сервера
#define IEWEBERR_TIMELIMIT			5			// Превышен таймаут на выполнение метода
#define IEWEBERR_WEBERR				6			// Ошибка Web
#define IEWEBERR_PARAMS				7			// Некорректные параметры
//
// Коды ошибок сообщений для лог-файла
//
#define SYSLOG_INITEXPORT				"INITEXPORT"
#define SYSLOG_SETEXPORTOBJ				"SETEXPORTOBJ"
#define SYSLOG_INITEXPORTOBJITER		"INITEXPORTOBJITER"
#define SYSLOG_NEXTEXPORTOBJITER		"NEXTEXPORTOBJITER"
#define SYSLOG_ENUMEXPRECEIPT			"ENUMEXPRECEIPT"
#define SYSLOG_INITIMPORT				"INITIMPORT"
#define SYSLOG_GETIMPORTOBJ				"GETIMPORTOBJ"
#define SYSLOG_INITIMPORTOBJITER		"INITIMPORTOBJITER"
#define SYSLOG_NEXTIMPORTOBJITER		"NEXTIMPORTOBJITER"
#define SYSLOG_FINISHIMPEXP				"FINISHIMPEXP"
#define SYSLOG_REPLYIMPORTOBJSTATUS		"REPLYIMPORTOBJSTATUS"
#define SYSLOG_RELATIONSHIPS			"in Relationships()"
#define SYSLOG_PARSERELATIONRESPONSE	"in ParseRlnResponse()"
#define SYSLOG_ORDERHEADER				"in OrderHeader()"
#define SYSLOG_RECADVHEADER				"in RecadvHeader()"
#define SYSLOG_DOCPARTIES				"in DocPartiesAndCurrency()"
//#define SYSLOG_BEGINORDER				"in BeginOrder()"
//#define SYSLOG_GOODSORDER				"in GoodsOrder()"
#define SYSLOG_GOODSLINES				"in GoodsLines()"
//#define SYSLOG_ENDORDER					"in EndOrder()"
#define SYSLOG_ENDDOC					"in EndDoc()"
#define SYSLOG_SENDDOC					"in SendDoc()"
#define SYSLOG_RECEIVEDOC				"in ReceiveDoc()"
#define SYSLOG_PARSELISTMBRESP			"in ParseListMBResp()"
#define SYSLOG_PARSEFORDOCDATA			"in ParseForDocData()"
#define SYSLOG_PARSEFORGOODDATA			"in ParseForGoodData()"
#define SYSLOG_SETNEWSTATUS				"in SetNewStatus()"
#define SYSLOG_LISTMESSAGEBOX			"in ListMessageBox()"
#define SYSLOG_PARSEAPERAKRESP			"in ParseAperakResp()"

#define LOG_NOINCOMDOC					"Нет входящих сообщений"
#define LOG_NEWSTATERR					"Сбой при выставлении статуса NEW для документов:"

class ExportCls;
class ImportCls;

int    ErrorCode = 0;
int    WebServcErrorCode = 0;
SString StrError = "";
SString LogName = "";
SString SysLogName = "";
static ExportCls * P_ExportCls = 0;
static ImportCls * P_ImportCls = 0;

int GetObjTypeBySymb(const char * pSymb, uint & rType);
int Relationships();
void ProcessError(EDIWebServiceSoapProxy & rProxy);
int SetError(int errCode, const char * pStr = "") { ErrorCode = errCode, StrError = pStr; return 1; }
int SetWebServcError(int errCode) { WebServcErrorCode = errCode; return 1; }
void LogMessage(const char * pMsg);
void SysLogMessage(const char * pMsg);
// Удаляет из строки логина лишние символы
// Ибо логин 4607806659997EC_1, а должен быть 4607806659997ЕС
void FormatLoginToLogin(const char * login, SString & rStr);
//
// Структура для настройки обмена с провайдером. Заполняется при вызове Relationships()
//
struct StRlnConfig {
	void Clear()
	{
		EdiDocType = 0;
		SuppGLN = 0;
		Direction = 0;
		//DocType = 0;
		DocVersion = 0;
		DocStandard = 0;
		DocTest = 0;
	}
	int    EdiDocType;
	SString SuppGLN;		// GLN поставщика, к которому относятся данные настройки
	SString Direction;		// Исходящий или входящий документ
	SString DocVersion;		// Версия спецификации
	SString DocStandard;	// Стандарт документа
	SString DocTest;		// Статус документа
};

struct ErrMessage {
	uint   Id;
	const char * P_Msg;
};

struct WebServcErrMessage {
	uint   Id;
	const  char * P_Msg;
};

struct ObjectTypeSymbols {
	char * P_Symb;
	uint Type;
};

struct MessageTypeSymbols {
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

enum ImpObjStatus {
	docStatNoSuchDoc = 0,
	docStatIsSuchDoc
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
	{IEERR_WEBSERVСERR,			"Ошибка Web-сервиса: "},
	{IEERR_SOAP,                "Ошибка SOAP: "},
	{IEERR_IMPFILENOTFOUND,		"Файл импорта не найден: "},
	{IEERR_INVMESSAGEYTYPE,		"Неверный тип сообщения. Ожидается "},
	{IEERR_NOCFGFORGLN,			"При данном типе операции нет настроек конфигурации для пользователя с GLN "},
	{IEERR_NULLWRIEXMLPTR,		"Нулевой xmlWriter"},
	{IEERR_NULLREADXMLPTR,		"Нулевой xmlReader"},
	{IEERR_XMLWRITE,			"Ошибка записи в xml-файл"},
	{IEERR_XMLREAD,				"Ошибка чтения из xml-файла"},
	{IEERR_TOOMUCHGOODS,		"Число товаров, заявленных в файле импорта, меньше реального их количества"},
	{IEERR_MSGSYMBNOTFOUND,		"Символ не найден (символ типа операции импорта/экспорта)"},
	{IEERR_ONLYIMPMSGTYPES,		"Операция импорта работает только с типами команд ORDRSP, DESADV и APERAK"},
	{IEERR_NOCONFIG,			"Не найдена конфигурация для данного типа операции"},
	{IEERR_NOEXPPATH,			"Не определен каталог для файла экспорта"},
	{IEERR_ONLYEXPMSGTYPES,		"Операция экспорта работает только с типами команд ORDER и RECADV"},
	{IEERR_NOMEM,          		"Не достаточно памяти"},
};

WebServcErrMessage WebServcErrMsg[] = {
	{IEWEBERR_AUTH,			"Ошибка аутентификации"},
	{IEWEBERR_CORRELATION,	"Ошибка во взаимосвязи"},
	{IEWEBERR_EXTERNAL,		"Внешняя ошибка"},
	{IEWEBERR_SERVER,		"Внутренняя ошибка сервера"},
	{IEWEBERR_TIMELIMIT,	"Превышен таймаут на выполнение метода"},
	{IEWEBERR_WEBERR,		"Ошибка Web"},
	{IEWEBERR_PARAMS,		"Некорректные параметры"}
};

void LogMessage(const char * pMsg)
{
	SFile file(LogName, SFile::mAppend);
	if(file.IsValid()) {
		SString str;
        file.WriteLine((str = 0).Cat(getcurdatetime_(), DATF_GERMAN|DATF_CENTURY, TIMF_HMS).Tab().Cat(pMsg).CR());
	}
}

void SysLogMessage(const char * pMsg)
{
	SFile  file(SysLogName, SFile::mAppend);
	if(file.IsValid()) {
		SString str;
        file.WriteLine((str = 0).Cat(getcurdatetime_(), DATF_GERMAN|DATF_CENTURY, TIMF_HMS).Tab().Cat(pMsg).CR());
	}
}

void GetErrorMsg(SString & rMsg)
{
	SString str = "";
	for(size_t i = 0; i < SIZEOFARRAY(ErrMsg); i++) {
		if(ErrMsg[i].Id == ErrorCode) {
			str.Cat(ErrMsg[i].P_Msg);
			break;
		}
	}
	if(ErrorCode == IEERR_WEBSERVСERR) {
		for(size_t i = 0; i < SIZEOFARRAY(ErrMsg); i++) {
			if(WebServcErrMsg[i].Id == WebServcErrorCode) {
				str.Cat(WebServcErrMsg[i].P_Msg);
				break;
			}
		}
	}
	else if(oneof4(ErrorCode, IEERR_SOAP, IEERR_IMPFILENOTFOUND, IEERR_INVMESSAGEYTYPE, IEERR_NOCFGFORGLN))
		str.Cat(StrError);
	rMsg = str;
}
//
// Returns:
//		0 - символ не найден
//		1 - символ найден
//
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
//
//
MessageTypeSymbols MsgSymbols[] = {
	{"ORDERS", PPEDIOP_ORDER},
	{"ORDRSP", PPEDIOP_ORDERRSP},
	{"APERAK", PPEDIOP_APERAK},
	{"DESADV", PPEDIOP_DESADV},
	{"RECADV", PPEDIOP_RECADV},

	{"ORDER",  PPEDIOP_ORDER} // Дополнительное значение - должно следовать после {"ORDERS", PPEDIOP_ORDER}
};
//
// Returns:
//		0 - символ не найден
//		1 - символ найден
//
int GetMsgTypeBySymb(const char * pSymb, int & rType)
{
	for(size_t i = 0; i < SIZEOFARRAY(MsgSymbols); i++) {
		if(stricmp(MsgSymbols[i].P_Symb, pSymb) == 0) {
			rType = MsgSymbols[i].Type;
			return 1;
		}
	}
	return 0;
}

int GetMsgTypeSymb(uint msgType, SString & rSymb)
{
	rSymb = 0;
	for(size_t i = 0; i < SIZEOFARRAY(MsgSymbols); i++) {
		if(MsgSymbols[i].Type == msgType) {
			rSymb = MsgSymbols[i].P_Symb;
			return 1;
		}
	}
	return 0;
}
//
// Удаляет из строки логина лишние символы
// Ибо логин 4607806659997EC_1, а должен быть 4607806659997ЕС
//
void FormatLoginToLogin(const char * login, SString & rStr)
{
	uint   exit_while = 0;
	char   low_strip = '_';
	rStr = 0;
	if(login) {
		while(!exit_while) {
			if((*login == 0) || (*login == low_strip))
				exit_while = 1;
			else
				rStr.CatChar(*login);
			login++;
		}
	}
}

class Iterator {
public:
	void Init()
	{
		Count = 0;
	}
	uint GetCount() const
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
//
// Класс, от которого наследуются ImportCls и ExportCls. Содержит общие методы, для этих классов
//
class ImportExportCls {
public:
	ImportExportCls();
	~ImportExportCls();
	void   CleanHeader();
	int    Relationships();
	int    ParseRlnResponse(const char * pResponse);

	Sdr_ImpExpHeader Header;
	TSArray <StRlnConfig> RlnCfgList; // Список конфигураций обмена для участников ЭДО, с которыми работает данный клиент
private:
};

ImportExportCls::ImportExportCls()
{
	RlnCfgList.clear();
}

ImportExportCls::~ImportExportCls()
{
	RlnCfgList.freeAll();
}

void ImportExportCls::CleanHeader()
{
	Header.CurDate = ZERODATE;
	Header.CurTime = ZEROTIME;
	Header.PeriodLow = ZERODATE;
	Header.PeriodUpp = ZEROTIME;
	memzero(Header.SrcSystemName, sizeof(Header.SrcSystemName));
	memzero(Header.SrcSystemVer, sizeof(Header.SrcSystemVer));
	memzero(Header.EdiLogin, sizeof(Header.EdiLogin));
	memzero(Header.EdiPassword, sizeof(Header.EdiPassword));
}

int ImportExportCls::Relationships()
{
	int    ok = 1;
	int    r = 0;
	SString login;
	_ns1__Relationships param;
	_ns1__RelationshipsResponse resp;
	EDIWebServiceSoapProxy proxy(SOAP_XML_INDENT);
	gSoapClientInit(&proxy, 0, 0);
	FormatLoginToLogin(Header.EdiLogin, login = 0); // ИД пользователя
	param.Name = (char *)(const char *)login;
	//param.Name = Header.EdiLogin;			// ИД пользователя
	param.Password = Header.EdiPassword;	// Пароль
	param.Timeout = 5000;					// Таймаут на выполнение вызова метода (мс)
	if((r = proxy.Relationships(&param, &resp)) == SOAP_OK) {
		if(atoi(resp.RelationshipsResult->Res) == 0) {
			//
			// Разбираем полученный ответ и заполняем набор RlnCfgList
			//
			ParseRlnResponse(resp.RelationshipsResult->Cnt);
			ok = 1;
		}
		else {
			SetError(IEERR_WEBSERVСERR);
			SetWebServcError(atoi((const char *)resp.RelationshipsResult->Res));
			ok = 0;
		}
	}
	else {
		ProcessError(proxy);
		ok = 0;
	}
	if(!ok)
		SysLogMessage(SYSLOG_RELATIONSHIPS);
	return ok;
}

int ImportExportCls::ParseRlnResponse(const char * pResp)
{
	int    ok = 1;
	SString str;
	StRlnConfig * p_rln_cfg = 0;
	xmlTextReaderPtr p_xml_ptr;
	xmlParserInputBufferPtr p_input = 0;
	xmlNodePtr p_node;
	RlnCfgList.freeAll();
	if(pResp) {
		p_input = xmlParserInputBufferCreateMem(pResp, sstrlen(pResp), XML_CHAR_ENCODING_NONE);
		THROWERR((p_xml_ptr = xmlNewTextReader(p_input, NULL)), IEERR_NULLREADXMLPTR);
		//
		// xmlTextReaderSetup нужен, потому что через определенное количество элементов (скорее всего) для элемента p_node->children
		// нулевой, хотя это не так. И получается, что содержимое эелемента не считывается.
		//
		THROWERR(xmlTextReaderSetup(p_xml_ptr, p_input, NULL, NULL, XML_PARSE_SAX1) == 0, IEERR_NULLREADXMLPTR);
		while(xmlTextReaderRead(p_xml_ptr)) {
			p_node = xmlTextReaderCurrentNode(p_xml_ptr);
			if(p_node && p_node->children) {
				const char * p_nn = (const char *)p_node->name;
				if(strcmp(p_nn, ELEMENT_NAME_RELATION) == 0) {
					p_rln_cfg = new StRlnConfig;
					p_rln_cfg->Clear();
				}
				else if(p_rln_cfg) {
					if(strcmp(p_nn, ELEMENT_NAME_PARTNER_ILN) == 0) {
						p_rln_cfg->SuppGLN.Set(p_node->children->content);
					}
					else if(strcmp(p_nn, "direction") == 0) { // Входящий или исходящий документ
						p_rln_cfg->Direction.Set(p_node->children->content);
					}
					else if(strcmp(p_nn, "document-type") == 0) { // Тип документа
						GetMsgTypeBySymb((const char *)p_node->children->content, p_rln_cfg->EdiDocType);
					}
					else if(strcmp(p_nn, "document-version") == 0) { // Версия спецификации
						p_rln_cfg->DocVersion.Set(p_node->children->content);
					}
					else if(strcmp(p_nn, "document-standard") == 0) { // Стандарт документа
						p_rln_cfg->DocStandard.Set(p_node->children->content);
					}
					else if(strcmp(p_nn, "document-test") == 0) { // Статус документа
						p_rln_cfg->DocTest.Set(p_node->children->content);
					}
				}
			}
			else if(SXml::IsName(p_node, ELEMENT_NAME_RELATION) && !p_node->children && p_rln_cfg) {
				//
				// Если закрывающий тег <relation>
				// Дополнительно проверяем, чтобы p_node->name != 0, ибо в ответе провайдера есть строки вида
				// <description><bla-bla-bla></description>
				//
				RlnCfgList.insert(p_rln_cfg);
				p_rln_cfg = 0;
			}
		}
	}
	CATCH
		SysLogMessage(SYSLOG_PARSERELATIONRESPONSE);
		GetErrorMsg(str = 0);
		SysLogMessage(str);
		ok = 0;
	ENDCATCH;
	xmlFreeParserInputBuffer(p_input);
	//if(p_xml_ptr)
	//	xmlFreeTextReader(p_xml_ptr);

	// @vmiller для проверки {
	/*if(file.IsValid()) {
		for(int rel_pos = 0; rel_pos < RlnCfgList.getCount(); rel_pos++) {
			(file_str = 0).Cat("Partner ILN = ").Cat(RlnCfgList.at(rel_pos).SuppGLN).CR();
			file.WriteLine(file_str);
			(file_str = 0).Cat("Direction = ").Cat(RlnCfgList.at(rel_pos).Direction).CR();
			file.WriteLine(file_str);
			(file_str = 0).Cat("DocType = ").Cat(RlnCfgList.at(rel_pos).DocType).CR();
			file.WriteLine(file_str);
		}
	}*/
	// }
	return ok;
}
//
//
//
EXPORT int FinishImpExp();

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch(dwReason) {
		case DLL_PROCESS_ATTACH:
			{
				SString product_name;
				(product_name = "Papyrus-ImpExpKorus");
				SLS.Init(product_name/*, (HINSTANCE)hModule*/);
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
			FinishImpExp(); // @vmiller
			break;
	}
	return TRUE;
}
//
// Экспорт
//
class ExportCls : public ImportExportCls {
public:
	ExportCls();
	~ExportCls();
	void   CreateFileName(uint num)
	{
		(ExpFileName = 0).Cat(PathStruct.Drv).CatChar(':').Cat(PathStruct.Dir).Cat(PathStruct.Nam).Cat(num).Dot().Cat(PathStruct.Ext);
	}
	int    OrderHeader();
	int    RecadvHeader();
	int    DocPartiesAndCurrency();
	int    GoodsLines(Sdr_BRow * pBRow);
	int    EndDoc();
	int    SendDoc();

	uint   Id;                  // ИД, который Papyrus будет воспринимать как ИД сеанса экспорта
	uint   ObjId;               // ИД экспортируемого объекта (например, для пакета документов каждому документу соответствует свой ИД)
	uint   ObjType;             // Тип экспортируемых объектов
	uint   Inited;
	uint   SegNum;              // Количество элементов (сегментов) сообщения
	uint   ReadReceiptNum;		// Порядковый номер квитанции, прочитанной из ReceiptList
	int    MessageType;			// Тип сообщеия: ORDER, RECADV, DESADV
	double BillSumWithoutVat;	// Сумма документа без НДС (придется считать самостоятельно, ибо в Sdr_Bill этот праметр не передается)
	SString ExpFileName;
	SString LogFileName;		// Там же, где и ExpFileName
	SString TTN;				// ТТН для RECADV
	xmlTextWriter * P_XmlWriter;
	Iterator Itr;
	SPathStruc PathStruct;
	Sdr_Bill Bill;
	TSArray <Sdr_DllImpExpReceipt> ReceiptList; // Список квитанций об отправленных документах // @todo TSArray-->TSCollection
};

ExportCls::ExportCls()
{
	Id = 0;
	ObjId = 0;
	ObjType = 0;
	Inited = 0;
	SegNum = 0;
	ReadReceiptNum = 0;
	BillSumWithoutVat = 0.0;
	ExpFileName = 0;
	LogName = 0;
	TTN = 0;
	P_XmlWriter = 0;
	ErrorCode = 0;
	WebServcErrorCode = 0;
	MessageType = 0;
}

ExportCls::~ExportCls()
{
	P_XmlWriter = 0;
}
//
// Работа с форматами документов и сообщений
//
// Заголовок заказа
//
int ExportCls::OrderHeader()
{
	int    ok = 1;
	LDATE  date;
	SString str, fmt;
	THROWERR(P_XmlWriter, IEERR_NULLWRIEXMLPTR);
	BillSumWithoutVat = 0.0;
	SegNum = 0;
	xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"ORDERS");
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"UNH");
			SegNum++;
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0062"); // ИД сообщения
				xmlTextWriterWriteString(P_XmlWriter, (str = 0).Cat(ObjId).ucptr());
			xmlTextWriterEndElement(P_XmlWriter); //E0062
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"S009");
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0065"); // Тип сообщения
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"ORDERS");
				xmlTextWriterEndElement(P_XmlWriter); //E0065
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0052"); // Версия сообщения
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"D");
				xmlTextWriterEndElement(P_XmlWriter); //E0052
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0054"); // Версия выпуска
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"01B");
				xmlTextWriterEndElement(P_XmlWriter); //E0054
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0051"); // Код ведущей организации
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"UN");
				xmlTextWriterEndElement(P_XmlWriter); //E0051
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0057"); // Код, присвоенный ведущей организацией
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"EAN010");
				xmlTextWriterEndElement(P_XmlWriter); //E0057
			xmlTextWriterEndElement(P_XmlWriter); //S009
		xmlTextWriterEndElement(P_XmlWriter); //UNH
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"BGM");
			SegNum++;
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C002"); // Имя документа/сообщения
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E1001"); // Код документа
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"220"); // Заказ (есть разновидности заказа)
				xmlTextWriterEndElement(P_XmlWriter); //E1001
			xmlTextWriterEndElement(P_XmlWriter); //С002
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C106"); // Идентификация документа/сообщения
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E1004"); // Номер заказа
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.Code); // Должно быть максимум 17 символов
				xmlTextWriterEndElement(P_XmlWriter); //E1004
			xmlTextWriterEndElement(P_XmlWriter); //С106
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E1225"); // Код функции сообщения
				xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"9"); // Оригинал (есть еще копия, предварительный заказ, замена и т.д.)
			xmlTextWriterEndElement(P_XmlWriter); //E1225
		xmlTextWriterEndElement(P_XmlWriter); //BGM
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"DTM"); // Дата документа
			SegNum++;
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C507");
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E2005"); // Квалификатор функции даты-времени
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_E2005_137); // Дата/время документа/сообщения
				xmlTextWriterEndElement(P_XmlWriter); //E2005
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E2380"); // Дата или время, или период
					xmlTextWriterWriteString(P_XmlWriter, (str = 0).Cat(Bill.Date, DATF_YMD|DATF_CENTURY|DATF_NODIV).ucptr());
				xmlTextWriterEndElement(P_XmlWriter); //E2380
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E2379"); // Формат даты/времени
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"102"); // CCYYMMDD
				xmlTextWriterEndElement(P_XmlWriter); //E2379
			xmlTextWriterEndElement(P_XmlWriter); //C507
		xmlTextWriterEndElement(P_XmlWriter); //DTM
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"DTM"); // Дата доставки
			SegNum++;
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C507");
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E2005"); // Квалификатор функции даты-времени
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"2"); // Дата/время доставки
				xmlTextWriterEndElement(P_XmlWriter); //E2005
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E2380"); // Дата или время, или период
					date = (Bill.DueDate != ZERODATE) ? Bill.DueDate : Bill.Date;
					xmlTextWriterWriteString(P_XmlWriter, (str = 0).Cat(date, DATF_YMD|DATF_CENTURY|DATF_NODIV).ucptr());
				xmlTextWriterEndElement(P_XmlWriter); //E2380
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E2379"); // Формат даты/времени
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"102"); // CCYYMMDD
				xmlTextWriterEndElement(P_XmlWriter); //E2379
			xmlTextWriterEndElement(P_XmlWriter); //C507
		xmlTextWriterEndElement(P_XmlWriter); //DTM
	CATCH
		SysLogMessage(SYSLOG_ORDERHEADER);
		ok = 0;
	ENDCATCH;
	return ok;
}
//
// Заголовок уведомления о приемке
//
int ExportCls::RecadvHeader()
{
	int    ok = 1;
	SString str, fmt;
	THROWERR(P_XmlWriter, IEERR_NULLWRIEXMLPTR);
	BillSumWithoutVat = 0.0;
	SegNum = 0;
	xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"RECADV");
	{
		{
			SXml::WNode n_unh(P_XmlWriter, "UNH");
			{
				SegNum++;
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0062"); // ИД сообщения
					xmlTextWriterWriteString(P_XmlWriter, (str = 0).Cat(ObjId).ucptr());
				xmlTextWriterEndElement(P_XmlWriter); //E0062
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"S009");
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0065"); // Тип сообщения
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"RECADV");
					xmlTextWriterEndElement(P_XmlWriter); //E0065
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0052"); // Версия сообщения
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"D");
					xmlTextWriterEndElement(P_XmlWriter); //E0052
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0054"); // Версия выпуска
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"01B");
					xmlTextWriterEndElement(P_XmlWriter); //E0054
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0051"); // Код ведущей организации
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"UN");
					xmlTextWriterEndElement(P_XmlWriter); //E0051
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0057"); // Код, присвоенный ведущей организацией
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"EAN005");
					xmlTextWriterEndElement(P_XmlWriter); //E0057
				xmlTextWriterEndElement(P_XmlWriter); //S009
			}
		}
		{
			SXml::WNode n_bgm(P_XmlWriter, "BGM");
			{
				SegNum++;
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C002"); // Имя документа/сообщения
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E1001"); // Код документа
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"632"); // RECADV (есть разновидности заказа)
					xmlTextWriterEndElement(P_XmlWriter); //E1001
				xmlTextWriterEndElement(P_XmlWriter); //С002
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C106"); // Идентификация документа/сообщения
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E1004"); // Номер документа
						xmlTextWriterWriteString(P_XmlWriter, (str = Bill.Code).ToUtf8().ucptr()); // Должно быть максимум 17 символов
					xmlTextWriterEndElement(P_XmlWriter); //E1004
				xmlTextWriterEndElement(P_XmlWriter); //С106
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E1225"); // Код функции сообщения
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"9"); // Оригинал (есть еще копия, предварительный заказ, замена и т.д.)
				xmlTextWriterEndElement(P_XmlWriter); //E1225
			}
		}
		{
			SXml::WNode n_dtm(P_XmlWriter, "DTM"); // Дата документа
			{
				SegNum++;
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C507");
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E2005"); // Квалификатор функции даты-времени
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_E2005_137); // Дата/время документа/сообщения
					xmlTextWriterEndElement(P_XmlWriter); //E2005
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E2380"); // Дата или время, или период
						xmlTextWriterWriteString(P_XmlWriter, (str = 0).Cat(Bill.Date, DATF_YMD|DATF_CENTURY|DATF_NODIV).ucptr());
					xmlTextWriterEndElement(P_XmlWriter); //E2380
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E2379"); // Формат даты/времени
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"102"); // CCYYMMDD
					xmlTextWriterEndElement(P_XmlWriter); //E2379
				xmlTextWriterEndElement(P_XmlWriter); //C507
			}
		}
		{
			SXml::WNode n_dtm(P_XmlWriter, "DTM"); // Дата приемки
			{
				SegNum++;
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C507");
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E2005"); // Квалификатор функции даты-времени
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"50"); // Дата/время приемки
					xmlTextWriterEndElement(P_XmlWriter); //E2005
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E2380"); // Дата или время, или период
						xmlTextWriterWriteString(P_XmlWriter, (str = 0).Cat(Bill.Date, DATF_YMD|DATF_CENTURY|DATF_NODIV).ucptr());
					xmlTextWriterEndElement(P_XmlWriter); //E2380
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E2379"); // Формат даты/времени
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"102"); // CCYYMMDD
					xmlTextWriterEndElement(P_XmlWriter); //E2379
				xmlTextWriterEndElement(P_XmlWriter); //C507
			}
		}
		{
			SXml::WNode n_sg1(P_XmlWriter, "SG1"); // Номер уведомления об отгрузке (ТТН)
			{
				SXml::WNode n_ref(P_XmlWriter, "RFF");
				{
					SXml::WNode n_s(P_XmlWriter, "C506");
					n_s.PutInner("E1153", "AAK"); // Идентификатор уведомления об отгрузке
					n_s.PutInner("E1154", (str = Bill.DesadvBillNo).Transf(CTRANSF_INNER_TO_UTF8)); // Номер уведомления об отгрузке
				}
			}
		}
		if(!isempty(Bill.OrderBillNo)) {
			SXml::WNode n_sg1(P_XmlWriter, "SG1"); // Номер заказа
			{
				SXml::WNode n_ref(P_XmlWriter, "RFF");
				{
					SXml::WNode n_s(P_XmlWriter, "C506");
					n_s.PutInner("E1153", "ON"); // Идентификатор документа заказа
					n_s.PutInner("E1154", (str = Bill.OrderBillNo).ToUtf8()); // Номер заказа
				}
			}
		}
	}
	CATCH
		SysLogMessage(SYSLOG_RECADVHEADER);
		ok = 0;
	ENDCATCH;
	return ok;
}
//
// Участники обмена и валюта
//
int ExportCls::DocPartiesAndCurrency()
{
	int    ok = 1;
	SString str, fmt;
	THROWERR(P_XmlWriter, IEERR_NULLWRIEXMLPTR);
	BillSumWithoutVat = 0.0;
	SegNum = 0;
	const char * p_sg = 0;
	if(MessageType == PPEDIOP_ORDER)
		p_sg = "SG2";
	else if(MessageType == PPEDIOP_RECADV)
		p_sg = "SG4";
	if(p_sg) {
		{
			SXml::WNode n_sg2(P_XmlWriter, p_sg); // GLN поставщика
			{
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"NAD"); // Наименование и адрес
					SegNum++;
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E3035"); // Квалификатор стороны
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"SU"); // Поставщик
					xmlTextWriterEndElement(P_XmlWriter); //E3035
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C082"); // Детали стороны
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E3039); // GLN стороны
							xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.GLN);
						xmlTextWriterEndElement(P_XmlWriter); //E3039
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E3055"); // Код ведущей организации
							xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"9"); // EAN (Международная ассоциация товарной нумерации)
						xmlTextWriterEndElement(P_XmlWriter); //E3055
					xmlTextWriterEndElement(P_XmlWriter); //C082
				xmlTextWriterEndElement(P_XmlWriter); //NAD
				if(MessageType == PPEDIOP_ORDER && !isempty(Bill.CntractCode)) {
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_SG3); // Номер договора на поставку
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"RFF");
							xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C506");
								xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E1153"); // Идентификатор
									xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_E1153_CT); // Номер договора
								xmlTextWriterEndElement(P_XmlWriter); //E1153
								xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E1154");
									(str = 0).Cat(Bill.CntractCode).Transf(CTRANSF_INNER_TO_UTF8);
									xmlTextWriterWriteString(P_XmlWriter, str.ucptr()); // Сам номер договора
								xmlTextWriterEndElement(P_XmlWriter); //E1154
							xmlTextWriterEndElement(P_XmlWriter); //C506
						xmlTextWriterEndElement(P_XmlWriter); //RFF
					xmlTextWriterEndElement(P_XmlWriter); //SG3
				}
			}
		}
		{
			SXml::WNode n_sg2(P_XmlWriter, p_sg); // GLN покупателя
			{
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"NAD"); // Наименование и адрес
					SegNum++;
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E3035"); // Квалификатор стороны
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"BY"); // Покупатель
					xmlTextWriterEndElement(P_XmlWriter); //E3035
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C082"); // Детали стороны
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E3039); // GLN стороны
							// Если GLN агента пусто, то пишем GLN главной организации
							if(!isempty(Bill.AgentGLN))
								xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.AgentGLN);
							else
								xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.MainGLN);
						xmlTextWriterEndElement(P_XmlWriter); //E3039
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E3055"); // Код ведущей организации
							xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"9"); // EAN (Международная ассоциация товарной нумерации)
						xmlTextWriterEndElement(P_XmlWriter); //E3055
					xmlTextWriterEndElement(P_XmlWriter); //C082
				xmlTextWriterEndElement(P_XmlWriter); //NAD
			}
		}
		{
			SXml::WNode n_sg2(P_XmlWriter, p_sg); // GLN места доставки
			{
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"NAD"); // Наименование и адрес
					SegNum++;
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E3035"); // Квалификатор стороны
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"DP"); // Конечное место доставки
					xmlTextWriterEndElement(P_XmlWriter); //E3035
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C082"); // Детали стороны
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E3039); // GLN стороны
							if(!isempty(Bill.DlvrAddrCode))
								(str = 0).Cat(Bill.DlvrAddrCode);
							else
								(str = 0).Cat(Bill.LocCode);
							xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
						xmlTextWriterEndElement(P_XmlWriter); //E3039
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E3055"); // Код ведущей организации
							xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"9"); // EAN (Международная ассоциация товарной нумерации)
						xmlTextWriterEndElement(P_XmlWriter); //E3055
					xmlTextWriterEndElement(P_XmlWriter); //C082
				xmlTextWriterEndElement(P_XmlWriter); //NAD
			}
		}
	}
	// Следующих двух секций нет в описании RECADV
	if(MessageType == PPEDIOP_ORDER) {
		// Необязательный параметр
		if(!isempty(Bill.Obj2GLN) || !isempty(Bill.AgentGLN) || !isempty(Bill.MainGLN)) {
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"SG2"); // GLN плательщика
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"NAD"); // Наименование и адрес
					SegNum++;
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E3035"); // Квалификатор стороны
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"IV"); // GLN плательщика
					xmlTextWriterEndElement(P_XmlWriter); //E3035
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C082"); // Детали стороны
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E3039); // GLN стороны
							if(!isempty(Bill.Obj2GLN))
								xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.Obj2GLN);
							else if(!isempty(Bill.AgentGLN))
								xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.AgentGLN);
							else
								xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.MainGLN);
						xmlTextWriterEndElement(P_XmlWriter); //E3039
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E3055"); // Код ведущей организации
							xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"9"); // EAN (Международная ассоциация товарной нумерации)
						xmlTextWriterEndElement(P_XmlWriter); //E3055
					xmlTextWriterEndElement(P_XmlWriter); //C082
				xmlTextWriterEndElement(P_XmlWriter); //NAD
			xmlTextWriterEndElement(P_XmlWriter); //SG2
		}
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"SG7");
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_CUX); // Валюты
				SegNum++;
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_C504); // Детали
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E6347); // Квалификатор валюты
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"2"); // Ссылочная валюта
					xmlTextWriterEndElement(P_XmlWriter); //E6347
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E6345); // Идентификация валюты по ISO 4217
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_E6345_RUB); // Рубли
					xmlTextWriterEndElement(P_XmlWriter); //E6345
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E6343); // Квалификатор типа валюты
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"9"); // Валюта заказа
					xmlTextWriterEndElement(P_XmlWriter); //E6343
				xmlTextWriterEndElement(P_XmlWriter); //C504
			xmlTextWriterEndElement(P_XmlWriter); //CUX
		xmlTextWriterEndElement(P_XmlWriter); //SG7
	}
	CATCH
		SysLogMessage(SYSLOG_DOCPARTIES);
		ok = 0;
	ENDCATCH;
	return ok;
}
//
// Товарные позиции
//
int ExportCls::GoodsLines(Sdr_BRow * pBRow)
{
	int    ok = 1;
	SString str;
	THROWERR(P_XmlWriter, IEERR_NULLWRIEXMLPTR);
	THROWERR(pBRow, IEERR_NODATA);
	//
	// Шапку и участников обмена пишем один раз.
	// ТТН у всех строк одинаковый, ибо пришли от одного DESADV
	//
	if(MessageType == PPEDIOP_RECADV && pBRow->LineNo == 1) {
		TTN = pBRow->TTN;
		THROW(P_ExportCls->RecadvHeader());
		THROW(P_ExportCls->DocPartiesAndCurrency());
	}
	if(MessageType == PPEDIOP_ORDER) {
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"SG28"); // Инфа о товарах
	}
	else if(MessageType == PPEDIOP_RECADV) {
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"SG16"); // Инфа о товарах
	}
	//
	// Только для RECADV
	// Количество уровней упаковки
	//
	if(MessageType == PPEDIOP_RECADV) {
		{
			SXml::WNode n_cps(P_XmlWriter, "CPS");
			{
				SXml::WNode n(P_XmlWriter, "E7164", "1"); // Номер иерархии по умолчанию - 1
			}
		}
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"SG22");
	}
	{
		SXml::WNode n_lin(P_XmlWriter, "LIN");
		SegNum++;
		{
			{
				SXml::WNode n(P_XmlWriter, "E1082", (str = 0).Cat(Itr.GetCount() + 1)); // Номер строки заказа
			}
			{
				SXml::WNode n_212(P_XmlWriter, "C212"); // Номерной идентификатор товара
				{
					SXml::WNode n(P_XmlWriter, "E7140", (str = 0).Cat(pBRow->Barcode)); // Штрих-код товара
				}
				{
					SXml::WNode n(P_XmlWriter, "E7143", "SRV"); // Тип штрихкода EAN.UCC
				}
			}
		}
	}
	//
	// Необязательный параметр
	//
	if(!isempty(pBRow->ArCode)) {
		SXml::WNode n_pia(P_XmlWriter, "PIA"); // Дополнительный идентификатор товара
		{
			SXml::WNode n(P_XmlWriter, "E4347", "1"); // Код типа идентификатора товара (Дополнительный идентификатор)
		}
		{
			SXml::WNode n_212(P_XmlWriter, "C212");
			{
				SXml::WNode n(P_XmlWriter, "E7140", (str = pBRow->ArCode).Transf(CTRANSF_INNER_TO_UTF8)); // Артикул
			}
			{
				SXml::WNode n(P_XmlWriter, "E7143", "SA"); // Идентификатор типа артикула
			}
		}
	}
	// Только для ORDERS
	if(MessageType == PPEDIOP_ORDER) {
		// Необязательный параметр
		if(!isempty(pBRow->GoodsName)) {
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"IMD"); // Описание товара
				SegNum++;
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E7077); // Код формата описания
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_E7077_F); // Текст
				xmlTextWriterEndElement(P_XmlWriter); //E7077
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_C273); // Описание
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E7008); // Описание
					{
						SString str1;
						(str1 = 0).Cat(pBRow->GoodsName).ToUtf8(); // Провайдер потребовал эту кодировку
						(str = 0).Cat("<![CDATA[").Cat(str1).Cat("]]>"); // Делаем конструкцию <![CDATA[какая-то строка]]>, ибо благодаря этому спец символы воспринимаются системой как обычные
						xmlTextWriterWriteString(P_XmlWriter, str.ucptr()); // Наименование товара
					}
					xmlTextWriterEndElement(P_XmlWriter); //E7008
				xmlTextWriterEndElement(P_XmlWriter); //C273
			xmlTextWriterEndElement(P_XmlWriter); //IMD
		}
	}
	xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"QTY"); // Количество товара
		SegNum++;
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_C186); // Подробности
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E6063); // Квалификатор типа количества
				if(MessageType == PPEDIOP_ORDER)
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_E6063_21); // Заказанное количество товара
				else if(MessageType == PPEDIOP_RECADV)
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_E6063_194); // Принятое количество товара
			xmlTextWriterEndElement(P_XmlWriter); //E6063
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E6060); // Количество
				(str = 0).Cat(pBRow->Quantity);
				xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
			xmlTextWriterEndElement(P_XmlWriter); //E6060
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E6411); // Единицы измерения
				(str = pBRow->UnitName).ToUpper1251();
				if(str.CmpNC(UNIT_NAME_KG) == 0)
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_E6411_KGM); // Килограммы
				else
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_E6411_PCE); // Отдельные элементы
			xmlTextWriterEndElement(P_XmlWriter); //E6411
		xmlTextWriterEndElement(P_XmlWriter); //C186
	xmlTextWriterEndElement(P_XmlWriter); //QTY
	// Только для ORDER
	if(MessageType == PPEDIOP_ORDER) {
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"MOA"); // Сумма товарной позиции с НДС
			SegNum++;
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C516");
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E5025); // Квалификатор суммы товарной позиции
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"128"); //	Идентификатор суммы товарной позиции с НДС
				xmlTextWriterEndElement(P_XmlWriter); //E5025
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E5004); // Сумма
					xmlTextWriterWriteString(P_XmlWriter, (str = 0).Cat(pBRow->Cost * pBRow->Quantity).ucptr());
				xmlTextWriterEndElement(P_XmlWriter); //E5004
			xmlTextWriterEndElement(P_XmlWriter); //C516
		xmlTextWriterEndElement(P_XmlWriter); //MOA
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"MOA"); // Сумма товарной позиции без НДС
			SegNum++;
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C516");
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E5025); // Квалификатор суммы товарной позиции
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"203"); //	Идентификатор суммы товарной позиции без НДС
				xmlTextWriterEndElement(P_XmlWriter); //E5025
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E5004); // Сумма
					//(str = 0).Cat((pBRow->Cost - (pBRow->Cost / (pBRow->VatRate + 100) * 100)) * pBRow->Quantity);
					(str = 0).Cat(((pBRow->Cost / (pBRow->VatRate + 100)) * 100) * pBRow->Quantity);
					BillSumWithoutVat += str.ToReal();
					xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
				xmlTextWriterEndElement(P_XmlWriter); //E5004
			xmlTextWriterEndElement(P_XmlWriter); //C516
		xmlTextWriterEndElement(P_XmlWriter); //MOA
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"SG32"); // Цена товара с НДС
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"PRI"); // Ценовая информация
				SegNum++;
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C509");
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E5125"); // Квалификатор цены
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"AAE"); // Цена без сборов и надбавок, но с налогом
					xmlTextWriterEndElement(P_XmlWriter); //E5125
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E5118"); // Цена
						xmlTextWriterWriteString(P_XmlWriter, (str = 0).Cat(pBRow->Cost).ucptr());
					xmlTextWriterEndElement(P_XmlWriter); //E5118
				xmlTextWriterEndElement(P_XmlWriter); //C509
			xmlTextWriterEndElement(P_XmlWriter); //PRI
		xmlTextWriterEndElement(P_XmlWriter); //SG32
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"SG32"); // Цена товара без НДС
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"PRI"); // Ценовая информация
				SegNum++;
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C509");
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E5125"); // Квалификатор цены
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"AAA"); // Чистая цена без налогов
					xmlTextWriterEndElement(P_XmlWriter); //E5125
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E5118"); // Цена
						//(str = 0).Cat(pBRow->Cost - (pBRow->Cost / (pBRow->VatRate + 100) * 100));
						(str = 0).Cat((pBRow->Cost / (pBRow->VatRate + 100)) * 100);
						xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
					xmlTextWriterEndElement(P_XmlWriter); //E5118
				xmlTextWriterEndElement(P_XmlWriter); //C509
			xmlTextWriterEndElement(P_XmlWriter); //PRI
		xmlTextWriterEndElement(P_XmlWriter); //SG32
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"SG38"); // Ставка НДС
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"TAX");
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E5283); // Квалификатор
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"7"); // Идентификатор налогового отчисления
				xmlTextWriterEndElement(P_XmlWriter); //E5283
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C241");
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E5153); // Квалификатор
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"VAT"); // Идентификатор ставки НДС
					xmlTextWriterEndElement(P_XmlWriter); //E5153
				xmlTextWriterEndElement(P_XmlWriter); //C241
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_C243);
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E5278);
						xmlTextWriterWriteString(P_XmlWriter, (str = 0).Cat(pBRow->VatRate).ucptr()); // Ставка НДС
					xmlTextWriterEndElement(P_XmlWriter); //E5278
				xmlTextWriterEndElement(P_XmlWriter); //C243
			xmlTextWriterEndElement(P_XmlWriter); //TAX
		xmlTextWriterEndElement(P_XmlWriter); //SG38
	}
	if(MessageType == PPEDIOP_RECADV)
		xmlTextWriterEndElement(P_XmlWriter); //SG22
	xmlTextWriterEndElement(P_XmlWriter); //SG28/SG16
	CATCH
		SysLogMessage(SYSLOG_GOODSLINES);
		ok = 0;
	ENDCATCH;
	return ok;
}
//
// Завершаем формирование документа
//
int ExportCls::EndDoc()
{
	int    ok = 1;
	SString str;
	THROWERR(P_XmlWriter, IEERR_NULLWRIEXMLPTR);
		// Только для ORDER
		if(MessageType == PPEDIOP_ORDER) {
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"UNS"); // Разделитель зон
				SegNum++;
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0081"); // Идентификатор секции
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"S"); // Зона итоговой информации
				xmlTextWriterEndElement(P_XmlWriter); //E0081
			xmlTextWriterEndElement(P_XmlWriter); //UNS
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"MOA"); // Сумма заказа с НДС
				SegNum++;
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C516");
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E5025); // Квалификатор суммы
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"9"); // Сумма документа с НДС
					xmlTextWriterEndElement(P_XmlWriter); //E5025
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E5004); // Сумма
						(str = 0).Cat(Bill.Amount);
						xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
					xmlTextWriterEndElement(P_XmlWriter); //E5004
				xmlTextWriterEndElement(P_XmlWriter); //С516
			xmlTextWriterEndElement(P_XmlWriter); //MOA
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"MOA"); // Сумма заказа без НДС
				SegNum++;
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C516");
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E5025); // Квалификатор суммы
						xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"98"); // Сумма документа без НДС
					xmlTextWriterEndElement(P_XmlWriter); //E5025
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_E5004); // Сумма
						(str = 0).Cat(BillSumWithoutVat);
						xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
					xmlTextWriterEndElement(P_XmlWriter); //E5004
				xmlTextWriterEndElement(P_XmlWriter); //С516
			xmlTextWriterEndElement(P_XmlWriter); //MOA
		}
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"CNT"); // Итоговая информация
			SegNum++;
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"C270");
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E6069"); // Квалификатор типа итоговой информации
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"2"); // Количество товарных позиций в документе
				xmlTextWriterEndElement(P_XmlWriter); //E6069
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E6066"); // Значение
					(str = 0).Cat(Itr.GetCount());
					xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
				xmlTextWriterEndElement(P_XmlWriter); //E6066
			xmlTextWriterEndElement(P_XmlWriter); //C270
		xmlTextWriterEndElement(P_XmlWriter); //CNT
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"UNT"); // Окончание сообщения
			SegNum++;
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0074"); // Общее число сегментов в сообщении
				(str = 0).Cat(SegNum);
				xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
			xmlTextWriterEndElement(P_XmlWriter); //E0074
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"E0062"); // Номер электронного сообщения (совпадает с указанным в заголовке)
				(str = 0).Cat(ObjId);
				xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
			xmlTextWriterEndElement(P_XmlWriter); //E0062
		xmlTextWriterEndElement(P_XmlWriter); //UNT
	xmlTextWriterEndElement(P_XmlWriter); //ORDERS

	CATCH
		SysLogMessage(SYSLOG_ENDDOC);
		ok = 0;
	ENDCATCH;
	TTN = 0;
	return ok;
}
//
// Работа с soap-протоколом
//
// Отправляем документ
//
int ExportCls::SendDoc()
{
	int    ok = 1;
	int    r = 0;
	int64  file_size = 0;
	size_t pos = 0;
	char * buf = 0;
	SFile  file(ExpFileName, SFile::mRead);
	_ns1__Send param;
	_ns1__SendResponse resp;
	Sdr_DllImpExpReceipt exp_rcpt;
	SString str, login, edi_doc_type_symb;

	EDIWebServiceSoapProxy proxy(SOAP_XML_INDENT);
	gSoapClientInit(&proxy, 0, 0);
	file.CalcSize(&file_size);
	memzero(&exp_rcpt, sizeof(Sdr_DllImpExpReceipt));
	//
	// Ищем конфигурацию обмена для конкретного адресата
	//
	if(MessageType == PPEDIOP_ORDER) { // Заказ
		for(pos = 0; pos < RlnCfgList.getCount(); pos++) {
			const StRlnConfig & r_item = RlnCfgList.at(pos);
			if(!r_item.SuppGLN.CmpNC(Bill.GLN) && !r_item.Direction.CmpNC(ELEMENT_CODE_DIRECTION_OUT) && r_item.EdiDocType == MessageType)
				break;
		}
	}
	else if(MessageType == PPEDIOP_RECADV) { // Уведомление о приемке
		for(pos = 0; pos < RlnCfgList.getCount(); pos++) {
			const StRlnConfig & r_item = RlnCfgList.at(pos);
			if(!r_item.SuppGLN.CmpNC(Bill.GLN) && !r_item.Direction.CmpNC(ELEMENT_CODE_DIRECTION_OUT) && r_item.EdiDocType == MessageType)
				break;
		}
	}
	if(pos < RlnCfgList.getCount()) {
		const StRlnConfig & r_item = RlnCfgList.at(pos);
		FormatLoginToLogin(Header.EdiLogin, login = 0); // ИД пользователя
		param.Name = (char *)(const char *)login;
		//param.Name = Header.EdiLogin;			// ИД пользователя
		param.Password = Header.EdiPassword;	// Пароль
		param.PartnerIln = Bill.GLN;			// ИД партнера, которому посылается документ
		GetMsgTypeSymb(r_item.EdiDocType, edi_doc_type_symb);
		param.DocumentType = (char *)(const char *)edi_doc_type_symb;      // Тип документа
		param.DocumentVersion = (char *)(const char *)r_item.DocVersion;   // Версия спецификации
		param.DocumentStandard = (char *)(const char *)r_item.DocStandard; // Стандарт документа
		param.DocumentTest = (char *)(const char *)r_item.DocTest;         // Статус документа
		param.ControlNumber = (char *)Bill.Code; // Контрольный номер документа (просто номер документа)
		buf = new char[(size_t)file_size + 1];
		memzero(buf, (size_t)file_size + 1);
		file.ReadLine(str = 0); // Пропускаем первую строку <?xml version="1.0" encoding="UTF-8" ?>
		file.Seek((long)str.Len());
		file.ReadV(buf, (size_t)file_size + 1);
		param.DocumentContent = buf; // Содержание документа
		param.Timeout = 5000;		// Таймаут на выполнение вызова метода (мс) (число взято из описания методов web-сервиса)
		if((r = proxy.Send(&param, &resp)) == SOAP_OK) {
			if(atoi(resp.SendResult->Res) == 0) {
				exp_rcpt.ID = atol(Bill.ID);
				STRNSCPY(exp_rcpt.ReceiptNumber, resp.SendResult->Cnt); // ИД документа в системе провайдера. А-ля номер квитанции
				ReceiptList.insert(&exp_rcpt);
			}
			else {
				SetError(IEERR_WEBSERVСERR);
				SetWebServcError(atoi((const char *)resp.SendResult->Res));
				ok = 0;
			}
		}
		else {
			ProcessError(proxy);
			ok = 0;
		}
	}
	else {
		(str = 0).Cat(Bill.GLN).CR().Cat("Документ ").Cat(Bill.Code);
		SetError(IEERR_NOCFGFORGLN, str);

		SString str1;
		(str1 = "Не найден GLN ").Cat(str);
		LogMessage(str1);
		ok = -1;
	}
	if(!ok) {
		SysLogMessage(SYSLOG_SENDDOC);
	}
	ZDELETE(buf);
	return ok;
}
//
// Внешние функции экспорта
//
EXPORT int InitExport(void * pExpHeader, const char * pOutFileName, int * pId)
{
	int    ok = 1;
	SFile log_file;
	SPathStruc log_path;
	SPathStruc rel_path; // @vmiller для проверки
	SString str;
	if(!P_ExportCls) {
		P_ExportCls = new ExportCls;
	}
	if(P_ExportCls && !P_ExportCls->Inited) {
		if(pExpHeader)
			P_ExportCls->Header = *(Sdr_ImpExpHeader*)pExpHeader;
		if(!isempty(pOutFileName)) {
			P_ExportCls->PathStruct.Split(pOutFileName);
			P_ExportCls->PathStruct.Ext = "xml";
			if(P_ExportCls->PathStruct.Nam.Empty())
				P_ExportCls->PathStruct.Nam = "export_";
		}
		else {
			THROWERR(SLS.Init("Papyrus"), IEERR_NOEXPPATH);
			str = SLS.GetExePath();
			P_ExportCls->PathStruct.Split(str);
			P_ExportCls->PathStruct.Dir.ReplaceStr("\\bin", "\\out", 1);
			P_ExportCls->PathStruct.Nam = "export_";
			P_ExportCls->PathStruct.Ext = "xml";
		}
		log_path.Copy(&P_ExportCls->PathStruct, SPathStruc::fDrv | SPathStruc::fDir | SPathStruc::fNam | SPathStruc::fExt);
		log_path.Nam = "export_log";
		log_path.Ext = "txt";
		log_path.Merge(LogName = 0);
		log_path.Nam = "system_log";
		log_path.Merge(SysLogName = 0);
		P_ExportCls->Id = 1;
		*pId = P_ExportCls->Id; // ИД сеанса экспорта
		//
		// Получаем от провайдера конфигурации обмена
		//
		THROW(P_ExportCls->Relationships());
		P_ExportCls->Inited = 1;
	}
	THROWERR(P_ExportCls, IEERR_IMPEXPCLSNOTINTD);
	CATCH
		SysLogMessage(SYSLOG_INITEXPORT);
		GetErrorMsg(str = 0);
		SysLogMessage(str);
		LogMessage(str);
		ok = 0;
	ENDCATCH;
	return ok;
}

EXPORT int SetExportObj(uint idSess, const char * pObjTypeSymb, void * pObjData, int * pObjId, const char * pMsgType = 0)
{
	int    ok = 1;
	SString str;
	THROWERR(P_ExportCls, IEERR_IMPEXPCLSNOTINTD);
	THROWERR(idSess == P_ExportCls->Id, IEERR_NOSESS);
	THROWERR(pObjData, IEERR_NODATA);
	THROWERR(GetObjTypeBySymb(pObjTypeSymb, P_ExportCls->ObjType), IEERR_SYMBNOTFOUND);
	THROWERR(P_ExportCls->ObjType == objBill, IEERR_ONLYBILLS);
	THROWERR_STR(GetMsgTypeBySymb(pMsgType, P_ExportCls->MessageType), IEERR_MSGSYMBNOTFOUND, pMsgType);
	THROWERR(oneof2(P_ExportCls->MessageType, PPEDIOP_ORDER, PPEDIOP_RECADV), IEERR_ONLYEXPMSGTYPES);
	// Завершаем предыдущий документ
	if(P_ExportCls->P_XmlWriter) {
		THROW(P_ExportCls->EndDoc());
		xmlTextWriterEndDocument(P_ExportCls->P_XmlWriter);
		xmlFreeTextWriter(P_ExportCls->P_XmlWriter);
		P_ExportCls->P_XmlWriter = 0;
		// Отправляем файл, сформированный в прошлый раз. Имя файла и номер документа еще не успели принять новые значения
		//THROW(P_ExportCls->SendDoc());
		if(!P_ExportCls->SendDoc()) {
			SysLogMessage(SYSLOG_SETEXPORTOBJ);
			GetErrorMsg(str = 0);
			SysLogMessage(str);
			LogMessage(str);
			ok = 0;
		}
	}
	P_ExportCls->ObjId++;
	*pObjId = P_ExportCls->ObjId;
	P_ExportCls->CreateFileName(P_ExportCls->ObjId);
	P_ExportCls->P_XmlWriter = xmlNewTextWriterFilename(P_ExportCls->ExpFileName, 0);
	THROWERR(P_ExportCls->P_XmlWriter, IEERR_NULLWRIEXMLPTR);
	xmlTextWriterSetIndentString(P_ExportCls->P_XmlWriter, (const xmlChar*)"\t");
	// UTF-8 - по требованию провайдера
	xmlTextWriterStartDocument(P_ExportCls->P_XmlWriter, 0, "UTF-8", 0);
	P_ExportCls->Bill = *(Sdr_Bill *)pObjData;
	P_ExportCls->Bill.GLN[13] = 0; // Ибо иногда, видимо, появляются здесь лишние символы
	if(P_ExportCls->MessageType == PPEDIOP_ORDER) {
		THROW(P_ExportCls->OrderHeader());
		THROW(P_ExportCls->DocPartiesAndCurrency());
	}
	CATCH
		SysLogMessage(SYSLOG_SETEXPORTOBJ);
		GetErrorMsg(str = 0);
		SysLogMessage(str);
		LogMessage(str);
		ok = 0;
	ENDCATCH;
	return ok;
}

EXPORT int InitExportObjIter(uint idSess, uint objId)
{
	int    ok = 1;
	SString str;
	THROWERR(P_ExportCls, IEERR_IMPEXPCLSNOTINTD);
	THROWERR(idSess == P_ExportCls->Id, IEERR_NOSESS);
	THROWERR(objId == P_ExportCls->ObjId, IEERR_NOOBJID);
	P_ExportCls->Itr.Init();
	CATCH
		SysLogMessage(SYSLOG_INITEXPORTOBJITER);
		GetErrorMsg(str = 0);
		SysLogMessage(str);
		LogMessage(str);
		ok = 0;
	ENDCATCH;
	return ok;
}

EXPORT int NextExportObjIter(uint idSess, uint objId, void * pRow)
{
	int    ok = 1;
	SString str;
	THROWERR(P_ExportCls, IEERR_IMPEXPCLSNOTINTD);
	THROWERR(pRow, IEERR_NODATA);
	THROWERR(idSess == P_ExportCls->Id, IEERR_NOSESS);
	THROWERR(objId == P_ExportCls->ObjId, IEERR_NOOBJID);
	THROW(P_ExportCls->GoodsLines((Sdr_BRow *)pRow));
	P_ExportCls->Itr.Next();
	CATCH
		SysLogMessage(SYSLOG_NEXTEXPORTOBJITER);
		GetErrorMsg(str = 0);
		SysLogMessage(str);
		LogMessage(str);
		ok = 0;
	ENDCATCH;
	return ok;
}

EXPORT int EnumExpReceipt(void * pReceipt)
{
	int    ok = -1;
	SString str;
	if(P_ExportCls && pReceipt) {
		// Если последний документ еще не был доформирован и отправлен, сделаем это сейчас
		if(P_ExportCls->P_XmlWriter) {
			THROW(P_ExportCls->EndDoc());
			xmlTextWriterEndDocument(P_ExportCls->P_XmlWriter);
			xmlFreeTextWriter(P_ExportCls->P_XmlWriter);
			P_ExportCls->P_XmlWriter = 0;
			//THROW(P_ExportCls->SendDoc());
			if(!P_ExportCls->SendDoc()) {
				SysLogMessage(SYSLOG_ENUMEXPRECEIPT);
				GetErrorMsg(str = 0);
				SysLogMessage(str);
				LogMessage(str);
				ok = 0;
			}
		}
		// Считываем результаты отправки документов
		if(P_ExportCls->ReadReceiptNum < P_ExportCls->ReceiptList.getCount()) {
			ASSIGN_PTR((Sdr_DllImpExpReceipt *)pReceipt, P_ExportCls->ReceiptList.at(P_ExportCls->ReadReceiptNum++));
			ok = 1;
		}
	}
	CATCH
		SysLogMessage(SYSLOG_ENUMEXPRECEIPT);
		GetErrorMsg(str = 0);
		SysLogMessage(str);
		LogMessage(str);
		ok = 0;
	ENDCATCH;
	return ok;
}
//
// Импорт
//
struct AperakInfoSt {
	AperakInfoSt()
	{
		Clear();
	}
	void Clear()
	{
		DocNum = 0;
		Code = 0;
		Msg = 0;
		AddedMsg = 0;
		SupplGLN = 0;
		BuyerGLN = 0;
		AddrGLN = 0;
		DocDate = ZERODATE;
	}
	SString DocNum;		// Номер документа заказа
	SString Code;		// Код статуса
	SString Msg;		// Описание статуса
	SString AddedMsg;	// Дополнительное сообщение
	SString SupplGLN;	// GLN поставщика
	SString BuyerGLN;	// GLN покупателя
	SString AddrGLN;		// GLN адреса доставки
	LDATE	DocDate;	// Дата документа
};

class ImportCls : public ImportExportCls {
public:
	/*
		<partner-iln>4607180239990</partner-iln>
		<tracking-id>{20fb142f-0a0a-0a0a-0a0a-0a0a0a0a0a0a}</tracking-id>
		<document-type>ORDRSP</document-type>
		<document-version>D01B</document-version>
		<document-standard>XML</document-standard>
		<document-test>P</document-test>
		<document-status>N</document-status>
		<document-number>РЈРђ000002116</document-number>
		<document-date>2015-02-24</document-date>
		<document-control-number>560731568</document-control-number>
		<receive-date>2015-02-24 16:28:20</receive-date>
	*/
	/*
	struct MessageInfoBlock {
		MessageInfoBlock()
		{
			TrackingUUID.SetZero();
			EdiDocType = 0;
			DocDate = ZERODATE;
			ReceiveDate = ZERODATE;
			CheckNumber = 0;
		}
		SString PartnerILN;
		S_GUID TrackingUUID;
		uint   EdiDocType;
        SString DocVer;
        SString DocStd;
        SString DocStatus;
        SString DocNumber;
        LDATE   DocDate;
        LDATE   ReceiveDate;
        uint32  CheckNumber;
	};
	*/
	//TSCollection <MessageInfoBlock> MsgInfoList;
	PPEdiMessageList MsgList;

	ImportCls()
	{
		GoodsCount = 0;
		Id = 0;
		ObjId = 0;
		ObjType = 0;
		MessageType_ = 0;
		Inited = 0;
		IncomMessagesCounter = 0;
		BillSumWithoutVat = 0.0;
		DocNum = 0;
		ImpFileName = 0;
		LogFileName = 0;
		LastTrackId = 0;
		ErrorCode = 0;
		WebServcErrorCode = 0;
		AperakInfo.Clear();
		TrackIds.Clear();
	}
	~ImportCls()
	{
	}
	void   CreateFileName(uint num)
	{
		(ImpFileName = 0).Cat(PathStruct.Drv).CatChar(':').Cat(PathStruct.Dir).Cat(PathStruct.Nam).Cat(num).Dot().Cat(PathStruct.Ext);
	}
	int    ReceiveDoc(uint messageType);
	int    ListMessageBox(uint messageType);
	int    ParseForDocData(uint messageType, Sdr_Bill * pBill);
	int    ParseForGoodsData(uint messageType, Sdr_BRow * pBRow);
	int    ParseAperakResp(const char * pResp);
	int    SetNewStatus(SString & rErrTrackIdList);

	int    GoodsCount;          // Число товаров в документе
	uint   Id;                  // ИД, который Papyrus будет воспринимать как ИД сеанса экспорта
	uint   ObjId;               // ИД импортируемого объекта (например, если это пакет документов, то у каждого документа свой ИД)
	uint   ObjType;             // Тип импортируемых объектов
	uint   MessageType_;        // Тип сообщеия: ORDRESP, APERAK, DESADV
	uint   Inited;
	uint   IncomMessagesCounter;	// Счетчик входящих сообщений. Нужен в случае, если есть входящие сообщения, но они предназначены для других магазинов.
		// В этом случае ReceiveDoc() не выдаст сообщение LOG_NOINCOMDOC
	double BillSumWithoutVat;	// В Sdr_Bill этот праметр не передается, поэтому передача этого параметра Papyrus'у под вопросом
	SString DocNum;				// Номер импортируемого документа, который мы будем писать в качестве ТТН при DESADV
	SString ImpFileName;
	SString LogFileName;		// Там же, где и ImpFileName
	SString LastTrackId;		// GUID последнего прочитанного документа в системе EDI
	Iterator Itr;
	SPathStruc PathStruct;
	AperakInfoSt AperakInfo;
	StrAssocArray TrackIds;		// Массив GUID прочитанных документов в системе EDI, статусы которых надо поменять (то есть предназначены для другого магазина)
};

EXPORT int InitImport(void * pImpHeader, const char * pInputFileName, int * pId)
{
	int    ok = 1;
	SPathStruc log_path;
	SString str;
	ZDELETE(P_ImportCls);
	P_ImportCls = new ImportCls;
	if(P_ImportCls && !P_ImportCls->Inited) {
		if(pImpHeader)
			P_ImportCls->Header = *(Sdr_ImpExpHeader*)pImpHeader;
		if(!isempty(pInputFileName)) {
			P_ImportCls->PathStruct.Split(pInputFileName);
			if(P_ImportCls->PathStruct.Nam.Empty())
				(P_ImportCls->PathStruct.Nam = "korus_import_").Cat(P_ImportCls->ObjId);
			if(P_ImportCls->PathStruct.Ext.Empty())
				P_ImportCls->PathStruct.Ext = "xml";
		}
		else {
			char   fname[256];
			GetModuleFileName(NULL, fname, sizeof(fname));
			P_ImportCls->PathStruct.Split(fname);
			P_ImportCls->PathStruct.Dir.ReplaceStr("\\bin", "\\in", 1);
			(P_ImportCls->PathStruct.Nam = "import_").Cat(P_ImportCls->ObjId);
			P_ImportCls->PathStruct.Ext = "xml";
		}
		log_path.Copy(&P_ImportCls->PathStruct, SPathStruc::fDrv | SPathStruc::fDir | SPathStruc::fNam | SPathStruc::fExt);
		log_path.Nam = "import_log";
		log_path.Ext = "txt";
		log_path.Merge(LogName = 0);
		log_path.Nam = "system_log";
		log_path.Merge(SysLogName = 0);
		P_ImportCls->Id = 1;
		*pId = P_ImportCls->Id; // ИД сеанса импорта
		//
		// Получаем конфигурации обмена
		//
		THROW(P_ImportCls->Relationships());
		if(!isempty(P_ImportCls->Header.EdiDocType)) {
			int    message_type = 0;
			THROWERR(GetMsgTypeBySymb(P_ImportCls->Header.EdiDocType, message_type), IEERR_MSGSYMBNOTFOUND);
			THROW(P_ImportCls->ListMessageBox(message_type));
			P_ImportCls->MessageType_ = message_type;
		}
		else {
			P_ImportCls->MessageType_ = 0;
		}
		P_ImportCls->Inited = 1;
	}
	THROWERR(P_ImportCls, IEERR_IMPEXPCLSNOTINTD);
	CATCH
		SysLogMessage(SYSLOG_INITIMPORT);
		GetErrorMsg(str = 0);
		SysLogMessage(str);
		LogMessage(str);
		ok =0;
	ENDCATCH;
	return ok;
}

EXPORT int GetImportObj(uint idSess, const char * pObjTypeSymb, void * pObjData, int * pObjId, const char * pMsgType_)
{
	int    ok = 1, r = 0;
	SString str;
	THROWERR(P_ImportCls, IEERR_IMPEXPCLSNOTINTD);
	THROWERR(idSess == P_ImportCls->Id, IEERR_NOSESS);
	THROWERR(pObjData, IEERR_NODATA);
	THROWERR(GetObjTypeBySymb(pObjTypeSymb, P_ImportCls->ObjType), IEERR_SYMBNOTFOUND);
	P_ImportCls->ObjId++;
	*pObjId = P_ImportCls->ObjId;
	THROWERR(P_ImportCls->ObjType == objBill, IEERR_ONLYBILLS);
	{
		int    message_type = 0;
		SString message_type_symb = pMsgType_ ? pMsgType_ : P_ImportCls->Header.EdiDocType;
		//
		// Смотрим тип сообщения
		//
		THROWERR(message_type_symb.NotEmptyS(), IEERR_MSGSYMBNOTFOUND);
		THROWERR(GetMsgTypeBySymb(message_type_symb, message_type), IEERR_MSGSYMBNOTFOUND);
		THROWERR(oneof3(/*P_ImportCls->MessageType*/message_type, PPEDIOP_ORDERRSP, PPEDIOP_APERAK, PPEDIOP_DESADV), IEERR_ONLYIMPMSGTYPES);
		// Получаем документ
		P_ImportCls->CreateFileName(P_ImportCls->ObjId);
		THROW(r = P_ImportCls->ReceiveDoc(message_type));
		ok = 1; // @vmiller для проверки
		if(r == -1)
			ok = -1;
		else {
			if(oneof2(message_type, PPEDIOP_ORDERRSP, PPEDIOP_DESADV)) {
				THROW(P_ImportCls->ParseForDocData(message_type, (Sdr_Bill *)pObjData)); // Читаем документ и заполняем Sdr_Bill
			}
			else if(message_type == PPEDIOP_APERAK) {
				//
				// Здесь заполняем pObjData из структуры P_ImportCls->AperakInfo
				// Для пробы запишем номер документа и GLN адреса доставки, дату документа. Этого достаточно для определения принадлежности статуса конкретному магазину
				// GLN покупателя
				//
				P_ImportCls->AperakInfo.DocNum.CopyTo(((Sdr_Bill *)pObjData)->OrderBillNo, sizeof(((Sdr_Bill *)pObjData)->OrderBillNo));
				P_ImportCls->AperakInfo.AddrGLN.CopyTo(((Sdr_Bill *)pObjData)->DlvrAddrCode, sizeof((Sdr_Bill *)pObjData)->DlvrAddrCode);
				P_ImportCls->AperakInfo.AddrGLN.CopyTo(((Sdr_Bill *)pObjData)->LocCode, sizeof((Sdr_Bill *)pObjData)->LocCode);
				P_ImportCls->AperakInfo.BuyerGLN.CopyTo(((Sdr_Bill *)pObjData)->MainGLN, sizeof((Sdr_Bill *)pObjData)->MainGLN);
				P_ImportCls->AperakInfo.SupplGLN.CopyTo(((Sdr_Bill *)pObjData)->GLN, sizeof((Sdr_Bill *)pObjData)->GLN);
				((Sdr_Bill *)pObjData)->OrderDate = P_ImportCls->AperakInfo.DocDate;
			}
		}
	}
	CATCH
		SysLogMessage(SYSLOG_GETIMPORTOBJ);
		GetErrorMsg(str = 0);
		LogMessage(str);
		SysLogMessage(str);
		ok = 0;
	ENDCATCH;
	return ok;
}

EXPORT int InitImportObjIter(uint idSess, uint objId)
{
	int    ok = 1;
	SString str;
	THROWERR(P_ImportCls, IEERR_IMPEXPCLSNOTINTD);
	THROWERR(idSess == P_ImportCls->Id, IEERR_NOSESS);
	THROWERR(objId == P_ImportCls->ObjId, IEERR_NOOBJID);
	P_ImportCls->Itr.Init();
	CATCH
		SysLogMessage(SYSLOG_INITIMPORTOBJITER);
		GetErrorMsg(str = 0);
		LogMessage(str);
		SysLogMessage(str);
		ok = 0;
	ENDCATCH;
	return ok;
}
//
// Returns:
//		-1 - считаны все товарные строки. Однако, если в документе в поле общего количество написано число,
//			меньшее реального числа товарных позиций, то лишние позиции не считаются
//		 0 - ошибка
//		 1 - считана очередная товарная строка
//
EXPORT int NextImportObjIter(uint idSess, uint objId, void * pRow)
{
	int    ok = 1, r = 0;
	SString str;
	THROWERR(P_ImportCls, IEERR_IMPEXPCLSNOTINTD);
	THROWERR(pRow, IEERR_NODATA);
	THROWERR(idSess == P_ImportCls->Id, IEERR_NOSESS);
	THROWERR(objId == P_ImportCls->ObjId, IEERR_NOOBJID);
	// Разбираем записанный ранее документ и заполненяем Sdr_BRow
	THROW(r = P_ImportCls->ParseForGoodsData(P_ImportCls->MessageType_, (Sdr_BRow *)pRow));
	if(r == -1)
		ok = -1;
	P_ImportCls->Itr.Next();
	CATCH
		SysLogMessage(SYSLOG_NEXTIMPORTOBJITER);
		GetErrorMsg(str = 0);
		LogMessage(str);
		SysLogMessage(str);
		ok = 0;
	ENDCATCH;
	return ok;
}
//
// Вызывается Papyrus'ом после каждого GetImportObj. Возможность Papyrus'а сказать свое слово при импорте
//
EXPORT int ReplyImportObjStatus(uint idSess, uint objId, void * pObjStatus)
{
	int    ok = 1;
	if(P_ImportCls) {
		size_t pos = 0;
		SString str;
		const  uint message_type = P_ImportCls->MessageType_;
		Sdr_DllImpObjStatus * p_obj_status = (Sdr_DllImpObjStatus *)pObjStatus;
		//
		// Если в Papyrus есть заказ, на который получено подтверждение или статус
		//
		if(p_obj_status->DocStatus == docStatIsSuchDoc) {
			if(message_type == PPEDIOP_APERAK) {
				// Что-нибудь делаем с этим статусом
				(str = 0).Cat(P_ImportCls->AperakInfo.DocNum).CatChar(':').Space().Cat(P_ImportCls->AperakInfo.Msg).Utf8ToChar();
				LogMessage(str);
			}
			// Иначе ничего не делаем
		}
		//
		// Если в Papyrus такого заказа нет
		//
		else if(p_obj_status->DocStatus == docStatNoSuchDoc) {
			P_ImportCls->IncomMessagesCounter--;
			if(oneof2(message_type, PPEDIOP_ORDERRSP, PPEDIOP_DESADV)) {
				// Удаляем созданный ранее xml-документ
				SFile::Remove(P_ImportCls->ImpFileName);
			}
			//
			// И при подтверждени, и при статусе ставим в ящике сообщений статус у документа/статуса "Новый",
			// чтобы те, кому он предназначен, смогли его получить.
			//
			pos = P_ImportCls->TrackIds.getCount();
			P_ImportCls->TrackIds.Add(pos, P_ImportCls->LastTrackId);
		}
	}
	else
		ok = -1;
	return ok;
}
//
// Descr: Получает документ. В случае успеха, записывает его в файл.
//
// Returns:
//		-1 - нет входящих сообщений
//		 0 - ошибка
//		 1 - сообщение получено
//
int ImportCls::ReceiveDoc(uint messageType)
{
	int    ok = -1, r = 0;
	_ns1__Receive param;
	_ns1__ReceiveResponse resp;
	SString partner_iln, track_id, str, login;
	SString edi_doc_type_symb;
	SFile  file;
	EDIWebServiceSoapProxy proxy(SOAP_XML_INDENT);
	gSoapClientInit(&proxy, 0, 0);
	assert(messageType != 0);
	/*
	const uint _pos = MsgList.getPointer();
	if(_pos < MsgList.getCount()) {
		const PPEdiMessageEntry & r_eme = MsgList.at(_pos);
		MsgList.incPointer();
	*/
	//while(MsgInfoList.getPointer() < MsgInfoList.getCount()) {
	while(MsgList.getPointer() < MsgList.getCount()) {
		//const MessageInfoBlock * p_info_blk = MsgInfoList.at(MsgInfoList.getPointer());
		//MsgInfoList.incPointer();
		const PPEdiMessageEntry & r_eme = MsgList.at(MsgList.getPointer());
		MsgList.incPointer();
		//if(p_info_blk && p_info_blk->EdiDocType == messageType) {
		if(r_eme.EdiOp == messageType) {
			//partner_iln = p_info_blk->PartnerILN;
			//p_info_blk->TrackingUUID.ToStr(S_GUID::fmtIDL, track_id);
			partner_iln = r_eme.SenderCode;
			r_eme.Uuid.ToStr(S_GUID::fmtIDL, track_id);
			//
			// Ищем конфигурацию импорта для конкретного отправителя и типа операции
			//
			const uint rcl_c = RlnCfgList.getCount();
			uint  pos_ = 0;
			for(uint i = 0; !pos_ && i < rcl_c; i++) {
				const StRlnConfig & r_item = RlnCfgList.at(i);
				if(!r_item.SuppGLN.CmpNC(partner_iln) && r_item.EdiDocType == messageType && !r_item.Direction.CmpNC(ELEMENT_CODE_DIRECTION_IN)) {
					pos_ = i+1;
				}
			}
			if(pos_ > 0 && pos_ <= rcl_c) {
				const StRlnConfig & r_item = RlnCfgList.at(pos_-1);
				FormatLoginToLogin(Header.EdiLogin, login = 0); // ИД пользователя
				param.Name = (char *)(const char *)login;
				//param.Name = Header.EdiLogin;			// ИД пользователя в системе
				param.Password = Header.EdiPassword;	// Пароль
				param.PartnerIln = (char *)(const char *)partner_iln; // ИД партнера, которому был послан документ (при ORDRSP это будет GLN поставщика,
					// а при APERAK это будет 9999000000001 - служебный GLN сервиса, с которого отправляются сообщения)
				GetMsgTypeSymb(r_item.EdiDocType, edi_doc_type_symb);
				param.DocumentType = (char *)(const char *)edi_doc_type_symb; // Тип документа
				param.TrackingId = (char *)(const char *)track_id; // ИД документа в системе
				param.DocumentStandard = (char *)(const char *)r_item.DocStandard;	// Стандарт документа
				param.ChangeDocumentStatus = "R"; // Новый статус документа после завершения чтения (read)
				param.Timeout = 10000;		// Таймаут на выполнение вызова метода (мс) (число взято из описания методов web-сервиса)
				LastTrackId = track_id;
				if(proxy.Receive(&param, &resp) == SOAP_OK) {
					if(atoi(resp.ReceiveResult->Res) == 0) {
						if(oneof2(messageType, PPEDIOP_ORDERRSP, PPEDIOP_DESADV)) {
							file.Open(ImpFileName, SFile::mWrite);
							file.WriteLine(resp.ReceiveResult->Cnt);
							IncomMessagesCounter++;
						}
						else if(messageType == PPEDIOP_APERAK) {
							// Сразу разберем ответ
							file.Open(ImpFileName, SFile::mWrite);
							file.WriteLine(resp.ReceiveResult->Cnt);
							THROW(ParseAperakResp(resp.ReceiveResult->Cnt));
							IncomMessagesCounter++;
						}
						ok = 1;
					}
					else {
						SetError(IEERR_WEBSERVСERR);
						SetWebServcError(atoi((const char *)resp.ReceiveResult->Res));
						ok = 0;
					}
				}
				else {
					ProcessError(proxy);
					ok = 0;
				}
			}
			else {
				SetError(IEERR_NOCFGFORGLN, (str = 0).Cat(partner_iln));
				ok = -1;
			}
			break;
		}
	}
	CATCHZOK
	if(!ok)
		SysLogMessage(SYSLOG_RECEIVEDOC);
	return ok;
}
//
// Descr: Ставит статус прочитанного документа в New
//
int ImportCls::SetNewStatus(SString & rErrTrackIdList)
{
	int    ok = 1;
	SString str, login;
	_ns1__ChangeDocumentStatus param;
	_ns1__ChangeDocumentStatusResponse resp;
	EDIWebServiceSoapProxy proxy(SOAP_XML_INDENT);
	gSoapClientInit(&proxy, 0, 0);
	rErrTrackIdList = 0;
	for(size_t pos = 0; pos < TrackIds.getCount(); pos++) {
		FormatLoginToLogin(Header.EdiLogin, login = 0); // ИД пользователя
		param.Name = (char *)(const char *)login;
		//param.Name = Header.EdiLogin;			// ИД пользователя в системе
		param.Password = Header.EdiPassword;	// Пароль
		param.TrackingId = (char *)(const char *)TrackIds.at(pos).Txt; // ИД документа в системе
		param.Status = "N"; // Новый статус документа (new)
		if(proxy.ChangeDocumentStatus(&param, &resp) == SOAP_OK) {
			if(atoi(resp.ChangeDocumentStatusResult->Res) != 0) {
				SetError(IEERR_WEBSERVСERR);
				SetWebServcError(atoi((const char *)resp.ChangeDocumentStatusResult->Res));
				if(rErrTrackIdList.Empty())
					rErrTrackIdList = TrackIds.at(pos).Txt;
				else
					rErrTrackIdList.Comma().Cat(TrackIds.at(pos).Txt);
				ok = 0;
			}
		}
		else {
			ProcessError(proxy);
			if(rErrTrackIdList.Empty())
				rErrTrackIdList = TrackIds.at(pos).Txt;
			else
				rErrTrackIdList.Comma().Cat(TrackIds.at(pos).Txt);
			ok = 0;
		}
	}
	if(!ok)
		SysLogMessage(SYSLOG_SETNEWSTATUS);
	return ok;
}
//
// Returns:
//		-1 - нет сообщений
//		 0 - ошибка
//		 1 - есть сообщение
//
int ImportCls::ListMessageBox(uint messageType)
{
	MsgList.Clear();

	int    ok = -1;
	uint   pos = 0;

	xmlTextReaderPtr p_xml_ptr = 0;
	xmlParserInputBufferPtr p_input = 0;
	//MessageInfoBlock * p_info_blk = 0;

	SString fmt, low, upp, str, login;
	SString edi_doc_type_symb;
	_ns1__ListMBEx param;
	_ns1__ListMBExResponse resp;
	EDIWebServiceSoapProxy proxy(SOAP_XML_INDENT);
	gSoapClientInit(&proxy, 0, 0);
	//
	// Логика получения списка входящих документов/статусов такая.
	// 1. Смотрим первую конфигурацию для входящих документов. Соответественно, это будет первый поставщик.
	// 2. Проверяем, есть ли от него входящие сообщения.
	// 3.1. Если есть, запоминаем нужную инфу о документе и выходим из цикла с положительным результатом
	// 3.2. Если входящих сообщений не оказалось, то смотрим следующую конфигурацию, то есть следующего поставщика
	// 4. Если ящик оказался пуст при переборе всех поставщиков, то выходим из цикла с нулевым результатом
	//
	THROWERR(RlnCfgList.getCount(), IEERR_NOCONFIG);
	for(pos = 0; pos < RlnCfgList.getCount(); pos ++) {
		const StRlnConfig & r_item = RlnCfgList.at(pos);
		if(r_item.Direction.CmpNC(ELEMENT_CODE_DIRECTION_IN) == 0 && r_item.EdiDocType == messageType) {
			FormatLoginToLogin(Header.EdiLogin, login = 0); // ИД пользователя
			param.Name = (char *)(const char *)login;
			//param.Name = Header.EdiLogin;			// ИД пользователя
			param.Password = Header.EdiPassword;	// Пароль пользователя
			param.PartnerIln = (char *)(const char *)r_item.SuppGLN;           // ИД партнера, от которого был получен документ
			GetMsgTypeSymb(r_item.EdiDocType, edi_doc_type_symb);
			param.DocumentType = (char *)(const char *)edi_doc_type_symb;      // Тип документа
			param.DocumentVersion = (char *)(const char *)r_item.DocVersion;   // Версия спецификации
			param.DocumentStandard = (char *)(const char *)r_item.DocStandard; // Стандарт документа
			param.DocumentTest = (char *)(const char *)r_item.DocTest;         // Статус документа
			param.DocumentStatus = "N";	// Статус выбираемых документов (подтверждений, статусов) (только новые) (N - только новые, A - все)
			param.Timeout = 10000;		// Таймаут на выполнение вызова метода (мс) (число взято из описания методов web-сервиса)
			if((Header.PeriodLow != ZERODATE) && (Header.PeriodUpp != ZERODATE)) {
				(low = 0).Cat(Header.PeriodLow.year());
				if((fmt = 0).Cat(Header.PeriodLow.month()).Len() == 1)
					fmt.PadLeft(1, '0');
				low.CatChar('-').Cat(fmt);
				if((fmt = 0).Cat(Header.PeriodLow.day()).Len() == 1)
					fmt.PadLeft(1, '0');
				low.CatChar('-').Cat(fmt);
				(upp = 0).Cat(Header.PeriodUpp.year());
				if((fmt = 0).Cat(Header.PeriodUpp.month()).Len() == 1)
					fmt.PadLeft(1, '0');
				upp.CatChar('-').Cat(fmt);
				if((fmt = 0).Cat(Header.PeriodUpp.day()).Len() == 1)
					fmt.PadLeft(1, '0');
				upp.CatChar('-').Cat(fmt);
				param.DateFrom = (char *)(const char *)low;
				param.DateTo = (char *)(const char *)upp;
			}
			if(proxy.ListMBEx(&param, &resp) == SOAP_OK) {
				if(atoi(resp.ListMBExResult->Res) == 0) {
					if(strcmp(resp.ListMBExResult->Cnt, EMPTY_LISTMB_RESP) != 0) {
						SString xml_input = resp.ListMBExResult->Cnt;
						SString str, cname;
						p_input = xmlParserInputBufferCreateMem(xml_input, xml_input.Len(), XML_CHAR_ENCODING_NONE);
						THROWERR((p_xml_ptr = xmlNewTextReader(p_input, NULL)), IEERR_NULLREADXMLPTR);
						while(xmlTextReaderRead(p_xml_ptr)) {
							xmlNodePtr p_node = xmlTextReaderCurrentNode(p_xml_ptr);
							if(p_node && p_node->children && sstreqi_ascii((const char *)p_node->name, "document-info")) {
								//THROWERR(p_info_blk = new MessageInfoBlock, IEERR_NOMEM);
								PPEdiMessageEntry eme;
								for(xmlNodePtr p_doc_child = p_node->children; p_doc_child != 0; p_doc_child = p_doc_child->next) {
									if(p_doc_child->children) {
										cname.Set(p_doc_child->name);
										if(cname.CmpNC("tracking-id") == 0) { // ИД документа в системе
											//p_info_blk->TrackingUUID.FromStr((const char *)p_doc_child->children->content);
											eme.Uuid.FromStr((const char *)p_doc_child->children->content);
										}
										else if(cname.CmpNC(ELEMENT_NAME_PARTNER_ILN) == 0) {
											//p_info_blk->PartnerILN.Set(p_doc_child->children->content);
											STRNSCPY(eme.SenderCode, (const char *)p_doc_child->children->content);
										}
										else if(cname.CmpNC("document-type") == 0) {
											GetMsgTypeBySymb((const char *)p_doc_child->children->content, eme.EdiOp);
										}
										else if(cname.CmpNC("document-version") == 0) {
											//p_info_blk->DocVer.Set(p_doc_child->children->content);
										}
										else if(cname.CmpNC("document-standard") == 0) {
											//p_info_blk->DocStd.Set(p_doc_child->children->content);
										}
										else if(cname.CmpNC("document-status") == 0) {
											//p_info_blk->DocStatus.Set(p_doc_child->children->content);
										}
										else if(cname.CmpNC("document-number") == 0) {
											//p_info_blk->DocNumber.Set(p_doc_child->children->content);
											STRNSCPY(eme.Code, p_doc_child->children->content);
										}
										else if(cname.CmpNC("document-date") == 0) {
											//strtodate((const char *)p_doc_child->children->content, DATF_YMD|DATF_CENTURY, &p_info_blk->DocDate);
											strtodate((const char *)p_doc_child->children->content, DATF_YMD|DATF_CENTURY, &eme.Dtm.d);
										}
										else if(cname.CmpNC("receive-date") == 0) {
											//strtodate((const char *)p_doc_child->children->content, DATF_YMD|DATF_CENTURY, &p_info_blk->ReceiveDate);
										}
										else if(cname.CmpNC("document-control-number") == 0) {
											//p_info_blk->CheckNumber = strtoul((const char *)p_doc_child->children->content, 0, 10);
										}
									}
								}
								//MsgInfoList.insert(p_info_blk);
								MsgList.Add(eme);
								//p_info_blk = 0;
							}
						}
						ok = 1;
						break;
					}
					else
						ok = -1;
				}
				else {
					SetWebServcError(atoi((const char *)resp.ListMBExResult->Res));
					THROWERR(0, IEERR_WEBSERVСERR);
				}
			}
			else {
				ProcessError(proxy);
				break;
			}
		}
	}
	CATCH
		SysLogMessage(SYSLOG_LISTMESSAGEBOX);
		ok = 0;
	ENDCATCH
	xmlFreeParserInputBuffer(p_input);
	xmlFreeTextReader(p_xml_ptr);
	//delete p_info_blk;
	return ok;
}
//
// Разбираем полученный документ и заполняем Sdr_Bill
//
int ImportCls::ParseForDocData(uint messageType, Sdr_Bill * pBill)
{
	int    ok = 0, exit_while = 0;
	SString str, temp_buf;
	xmlDoc * p_doc = 0;
	THROWERR_STR(fileExists(ImpFileName), IEERR_IMPFILENOTFOUND, ImpFileName);
	THROWERR(pBill, IEERR_NODATA);
	memzero(pBill, sizeof(Sdr_Bill));
	THROWERR((p_doc = xmlReadFile(ImpFileName, NULL, XML_PARSE_NOENT)), IEERR_NULLREADXMLPTR);
	xmlNode * p_node = 0;
	xmlNode * p_root = xmlDocGetRootElement(p_doc);
	THROWERR(p_root, IEERR_XMLREAD);
	// @v8.5.6 {
	STRNSCPY(pBill->EdiOpSymb, p_root->name);
	pBill->EdiOp = messageType;
	// } @v8.5.6
	p_node = p_root->children;
	while(p_node && p_node->type == XML_ELEMENT_NODE) {
		if(p_node->children && (p_node->children->type == XML_READER_TYPE_ELEMENT))
			p_node = p_node->children;
		else if(p_node->next)
			p_node = p_node->next;
		else {
			xmlNode * p_node_2 = 0;
			while(p_node && p_node->parent && !exit_while) {
				p_node_2 = p_node->parent->next;
				if(p_node_2) {
					p_node = p_node_2;
					exit_while = 1;
				}
				else
					p_node = p_node->parent;
			}
		}
		exit_while = 0;
		if(p_node && (p_node->type == XML_ELEMENT_NODE)) {
			if(SXml::IsName(p_node, "E0065") && p_node->children) {
				if(messageType == PPEDIOP_ORDERRSP) {
					THROWERR_STR(SXml::IsContent(p_node->children, "ORDRSP"), IEERR_INVMESSAGEYTYPE, "ORDRSP")
				}
				else if(messageType == PPEDIOP_DESADV) {
					THROWERR_STR(SXml::IsContent(p_node->children, "DESADV"), IEERR_INVMESSAGEYTYPE, "DESADV");
				}
			}
			else if(SXml::IsName(p_node, "E1004") && p_node->children) {
				str.Set(p_node->children->content).Utf8ToChar(); // Будет няшно выглядеть в Papyrus
				str.CopyTo(pBill->Code, sizeof(pBill->Code));
				DocNum = str;
				ok = 1;
				//strcpy(pBill->Code, (const char *)p_node->children->content);
				//DocNum.CopyFrom((const char *)p_node->children->content);
				//ok = 1;
			}
			else if(SXml::IsName(p_node, "C506") && p_node->children && p_node->children->type == XML_READER_TYPE_ELEMENT) {
				p_node = p_node->children; // <E1153>
				if(SXml::IsName(p_node, "E1153") && p_node->children) {
					if(SXml::IsContent(p_node->children, "ON")) {
						if(p_node->next) {
							p_node = p_node->next; // <E1154>
							if(SXml::IsName(p_node, "E1154") && p_node->children) {
								// Номер заказа, на который пришло подтверждение
								STRNSCPY(pBill->OrderBillNo, p_node->children->content);
								ok = 1;
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_node, "C507") && p_node->children && (p_node->children->type == XML_READER_TYPE_ELEMENT)) {
				p_node = p_node->children; // <E2005>
				if(p_node && SXml::IsName(p_node, "E2005") && p_node->children) {
					str.Set(p_node->children->content); // Запомним значение элемента
					if(p_node->next) {
						p_node = p_node->next; // <E2380>
						if(p_node && SXml::IsName(p_node, "E2380") && p_node->children) {
							//
							// Note: Строка с датой имеет формат: YYYYMMDD. При этом возможно наличие хвостовых нулей.
							//   Из-за этого предварительно обрезаем строку даты до 8 символов (функция strtodate корректно извлекает
							//   дату без разделителей только при фиксированном размере строки в 8 символов).
							//
							if(str.CmpNC(ELEMENT_CODE_E2005_137) == 0) {
								temp_buf.Set(p_node->children->content).Trim(8); // Дата документа
								strtodate(temp_buf, DATF_YMD|DATF_CENTURY, &pBill->Date);
								ok = 1;
							}
							else if((str.CmpNC(ELEMENT_CODE_E2005_17) == 0) || (str.CmpNC("2") == 0) || (str.CmpNC(ELEMENT_CODE_E2005_358) == 0)) {
								temp_buf.Set(p_node->children->content).Trim(8); // Дата доставки (дата исполнения документа)
								strtodate(temp_buf, DATF_YMD|DATF_CENTURY, &pBill->DueDate);
								ok = 1;
							}
							else if(str.CmpNC(ELEMENT_CODE_E2005_171) == 0) {
								temp_buf.Set(p_node->children->content).Trim(8); // Дата заказа
								strtodate(temp_buf, DATF_YMD|DATF_CENTURY, &pBill->OrderDate);
								ok = 1;
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_node, "E3035") && p_node->children) {
				str.Set(p_node->children->content); // Запомним значение элемента
				if(p_node->next) {
					p_node = p_node->next; // <C082>
					if(SXml::IsName(p_node, "C082") && p_node->children) {
						p_node = p_node->children; // <E3039>
						if(SXml::IsName(p_node, ELEMENT_NAME_E3039) && p_node->children) {
							if(str.CmpNC("BY") == 0) { // GLN покупателя
								STRNSCPY(pBill->MainGLN, (const char *)p_node->children->content);
								STRNSCPY(pBill->AgentGLN, (const char *)p_node->children->content);
								ok = 1;
							}
							else if(str.CmpNC("SU") == 0) { // GLN поставщика
								STRNSCPY(pBill->GLN, p_node->children->content);
								ok = 1;
							}
							else if(str.CmpNC("DP") == 0) { // GLN адреса доставки
								STRNSCPY(pBill->DlvrAddrCode, p_node->children->content);
								ok = 1;
							}
							else if(str.CmpNC("IV") == 0) { // GLN плательщика
								STRNSCPY(pBill->Obj2GLN, p_node->children->content);
								ok = 1;
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_node, ELEMENT_NAME_E1229) && p_node->children) { // код действия (изменение, принято без изменений, не принято)
				str.Set(p_node->children->content); // @vmiller
				ok = 1;
			}
			else if(SXml::IsName(p_node, "C516") && p_node->children && p_node->children->type == XML_READER_TYPE_ELEMENT) {
				p_node = p_node->children; // <E5025>
				if(p_node && SXml::IsName(p_node, ELEMENT_NAME_E5025) && p_node->children) {
					str.Set(p_node->children->content); // Запомним значение текущего элемента
					if(p_node->next) {
						p_node = p_node->next; // <E5004>
						if(p_node && SXml::IsName(p_node, ELEMENT_NAME_E5004) && p_node->children) {
							if(str == "9") { // сумма документа с НДС
								pBill->Amount = atof((const char *)p_node->children->content);
								ok = 1;
							}
							else if(str == "98") { // сумма документа без НДС
								str.Set(p_node->children->content); // @vmiller
								ok = 1;
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_node, "E6069") && p_node->children) {
				if(SXml::IsContent(p_node->children, "2")) {
					if(p_node->next) {
						p_node = p_node->next; // <E6060>
						if(SXml::IsName(p_node, "E6066") && p_node->children) {
							// Запишем количество товарных позиций в документе
							GoodsCount = atoi((const char *)p_node->children->content);
							ok = 1;
						}
					}
				}
			}
		}
	}
	CATCH
		SysLogMessage(SYSLOG_PARSEFORDOCDATA);
		ok = 0;
	ENDCATCH;
	if(p_doc)
		xmlFreeDoc(p_doc);
	return ok;
}
//
// Разбираем полученный документ и заполняем Sdr_BRow
// Returns:
//		-1 - считаны все товарные строки. Однако, если в документе в поле общего количество написано число,
//			меньшее реального числа товарных позиций, то лишние позиции не считаются
//		 0 - ошибка
//		 1 - считана очередная товарная строка
//
int ImportCls::ParseForGoodsData(uint messageType, Sdr_BRow * pBRow)
{
	int    ok = 1, index = 1, sg26_end = 0, exit_while = 0;
	SString str, goods_segment;
	xmlDoc * p_doc = 0;

	THROWERR_STR(fileExists(ImpFileName), IEERR_IMPFILENOTFOUND, ImpFileName);
	THROWERR(pBRow, IEERR_NODATA);
	memzero(pBRow, sizeof(Sdr_BRow));
	THROWERR((p_doc = xmlReadFile(ImpFileName, NULL, XML_PARSE_NOENT)), IEERR_NULLREADXMLPTR);
	xmlNode * p_node = 0;
	xmlNode * p_root = xmlDocGetRootElement(p_doc);
	THROWERR(p_root, IEERR_XMLREAD);
	if(Itr.GetCount() < (uint)GoodsCount) {
		p_node = p_root->children;
		if(messageType == PPEDIOP_ORDERRSP)
			goods_segment = ELEMENT_NAME_SG26;
		else if(messageType == PPEDIOP_DESADV)
			goods_segment = ELEMENT_NAME_SG17;
		while(p_node && !sg26_end) {
			if(p_node->children && (p_node->children->type == XML_READER_TYPE_ELEMENT))
				p_node = p_node->children;
			else if(p_node->next)
				p_node = p_node->next;
			else {
				xmlNode * p_node_2 = 0;
				while(p_node && p_node->parent && !exit_while) {
					p_node_2 = p_node->parent->next;
					if(p_node_2) {
						p_node = p_node_2;
						exit_while = 1;
					}
					else
						p_node = p_node->parent;
				}
			}
			exit_while = 0;
			// Благодаря индексу считываем разные товарные позиции
			if(p_node && p_node->type == XML_READER_TYPE_ELEMENT) {
				if(SXml::IsName(p_node, goods_segment) && p_node->children) {
					if(index == (Itr.GetCount() + 1)) {
						while(p_node && !sg26_end) {
							exit_while = 0;
							if(p_node->children && (p_node->children->type == XML_READER_TYPE_ELEMENT))
								p_node = p_node->children;
							else if(p_node->next)
								p_node = p_node->next;
							else {
								xmlNode * p_node_2 = 0;
								while(p_node && p_node->parent && !exit_while) {
									p_node_2 = p_node->parent->next;
									if(p_node_2) {
										p_node = p_node_2;
										exit_while = 1;
									}
									else
										p_node = p_node->parent;
								}
							}
							if(p_node) {
								if(p_node->type == XML_DOCUMENT_NODE || SXml::IsName(p_node, goods_segment)) // Первое условие актуально для последней товарной позиции. Если здесь не выйдем, то цикл начнет чиатть документ заново.
									sg26_end = 1;
								else if(SXml::IsName(p_node, "LIN") && p_node->children && p_node->children->type == XML_READER_TYPE_ELEMENT) {
									p_node = p_node->children; // <E1082>
									if(SXml::IsName(p_node, "E1082") && p_node->children) { // номер товарной позиции
										str.Set(p_node->children->content); // @vmiller
										if(p_node->next) {
											p_node = p_node->next; // <E1229>
											if(SXml::IsName(p_node, ELEMENT_NAME_E1229) && p_node->children) { // Статус товарной позиции
												str.Set(p_node->children->content); // @vmiller
											}
										}
									}
								}
								else if(SXml::IsName(p_node, "C212") && p_node->children && p_node->children->type == XML_READER_TYPE_ELEMENT) {
									p_node = p_node->children; // <E7140>
									if(SXml::IsName(p_node, "E7140") && p_node->children) {
										str.Set(p_node->children->content); // Запомним значение текущего элемента (штрихкод)
										if(p_node->next) {
											p_node = p_node->next; // <E7143>
											if(SXml::IsName(p_node, "E7143") && p_node->children) {
												if(SXml::IsContent(p_node->children, "SRV")) // штрихкод товара
													str.CopyTo(pBRow->Barcode, sizeof(pBRow->Barcode));
											}
										}
									}
								}
								else if(SXml::IsName(p_node, "E4347") && p_node->children) {
									if(SXml::IsContent(p_node->children, "1")) {
										//xmlTextReaderRead(p_xml_ptr); // text
										//xmlTextReaderRead(p_xml_ptr); // </E4347>
										//xmlTextReaderRead(p_xml_ptr); // <C212>
										//xmlTextReaderRead(p_xml_ptr); // <E7140>
										//p_node = xmlTextReaderCurrentNode(p_xml_ptr);
										if(p_node->next && p_node->next->children && p_node->next->children->type == XML_READER_TYPE_ELEMENT) {
											p_node = p_node->next->children; // <E7140> (Пропуская <C212>)
											if(SXml::IsName(p_node, "E7140") && p_node->children) {
												str.Set(p_node->children->content); // Запомним значение текущего элемента (артикул товара у поставщика)
												if(p_node->next) {
													p_node = p_node->next; // <E7143>
													if(SXml::IsName(p_node, "E7143") && p_node->children) {
														if(SXml::IsContent(p_node->children, "SA")) // Артикул поставщика
															str.CopyTo(pBRow->ArCode, sizeof(pBRow->ArCode));
													}
												}
											}
										}
									}
								}
								else if(SXml::IsName(p_node, ELEMENT_NAME_E7008) && p_node->children) {
									str.Set(p_node->children->content).Utf8ToOem(); // Наименование товара
									STRNSCPY(pBRow->GoodsName, str);
								}
								else if(SXml::IsName(p_node, ELEMENT_NAME_C186) && p_node->children && p_node->children->type == XML_READER_TYPE_ELEMENT) {
									p_node = p_node->children; // <E6063>
									if(SXml::IsName(p_node, ELEMENT_NAME_E6063) && p_node->children) {
										str.Set(p_node->children->content); // Запомним значение текущего элемента
										if(p_node->next) {
											p_node = p_node->next; // <E6060>
											if(SXml::IsName(p_node, ELEMENT_NAME_E6060) && p_node->children) {
												if(str.CmpNC(ELEMENT_CODE_E6063_21) == 0)
													str.Set(p_node->children->content); // @vmiller Заказанное количество (ORDRSP/DESADV)
												else if(str.CmpNC(ELEMENT_CODE_E6063_113) == 0 || str.CmpNC(ELEMENT_CODE_E6063_170) == 0)
													pBRow->Quantity = atof((const char *)p_node->children->content); // Подтвержденное количество (ORDRSP)
												else if(str.CmpNC(ELEMENT_CODE_E6063_12) == 0)
													pBRow->Quantity = atof((const char *)p_node->children->content); // Отгруженное количество (DESADV)
												else if(str.CmpNC(ELEMENT_CODE_E6063_59) == 0)
													pBRow->UnitPerPack = atof((const char *)p_node->children->content); // Количество товара в упаковке
											}
										}
									}
								}
								else if(SXml::IsName(p_node, ELEMENT_NAME_E6411) && p_node->children) { // Единицы измерения товара
									if(SXml::IsContent(p_node->children, ELEMENT_CODE_E6411_KGM))
										STRNSCPY(pBRow->UnitName, UNIT_NAME_KG);
									else
										STRNSCPY(pBRow->UnitName, UNIT_NAME_PIECE);
								}
								else if(SXml::IsName(p_node, "C516") && p_node->children && p_node->children->type == XML_READER_TYPE_ELEMENT) {
									p_node = p_node->children; // <E5025>
									if(SXml::IsName(p_node, ELEMENT_NAME_E5025) && p_node->children) {
										str.Set(p_node->children->content); // Запомним значение текущего элемента
										if(p_node->next) {
											p_node = p_node->next; // <E5004>
											if(SXml::IsName(p_node, ELEMENT_NAME_E5004) && p_node->children) {
												if(str == "203")
													str.Set(p_node->children->content); // @vmiller // Сумма товарной позиции без НДС
												else if(str == "79")
													str.Set(p_node->children->content); // @vmiller // Сумма товарной позиции с НДС
												else if(messageType == PPEDIOP_DESADV && str.CmpNC(ELEMENT_CODE_E5025_XB5) == 0)
													pBRow->Cost = atof((const char *)p_node->children->content); // Цена товара с НДС для DESADV
											}
										}
									}
								}
								else if(SXml::IsName(p_node, "C509") && p_node->children && p_node->children->type == XML_READER_TYPE_ELEMENT) {
									p_node = p_node->children; // <E5125>
									if(SXml::IsName(p_node, "E5125") && p_node->children) {
										str.Set(p_node->children->content); // Запомним значение текущего элемента
										if(p_node->next) {
											p_node = p_node->next; // <E5118>
											if(SXml::IsName(p_node, "E5118") && p_node->children) {
												if(str.CmpNC("AAA") == 0)
													str.Set(p_node->children->content); // @vmiller // Цена товара без НДС
												else if(str.CmpNC("AAE") == 0)
													pBRow->Cost = atof((const char *)p_node->children->content); // Цена товара с НДС
											}
										}
									}
								}
							}
							DocNum.CopyTo(pBRow->TTN, DocNum.Len() + 1);
						}
					}
					index++;
				}
			}
		}
	}
	else {
		ok = -1;
	}
	CATCH
		SysLogMessage(SYSLOG_PARSEFORGOODDATA);
		ok = 0;
	ENDCATCH;
	if(p_doc)
		xmlFreeDoc(p_doc);
	return ok;
}
//
// Descr: Разбор результата, полученного с помощью Receive() для APERAK
// ARG(pResp		 IN): Данные для разбора, полученные в Receive()
// Retruns:
//		-1 - не было получено входящих сообщений
//		 0 - ошибка
//		 1 - оба параметра найдены
//
int ImportCls::ParseAperakResp(const char * pResp)
{
	int    ok = 1;
	SString str;
	xmlTextReaderPtr p_xml_ptr;
	xmlParserInputBufferPtr p_input = 0;
	xmlNodePtr p_node;
	AperakInfo.Clear();
	if(pResp) {
		p_input = xmlParserInputBufferCreateMem(pResp, sstrlen(pResp), XML_CHAR_ENCODING_NONE);
		THROWERR((p_xml_ptr = xmlNewTextReader(p_input, NULL)), IEERR_NULLREADXMLPTR);
		while(xmlTextReaderRead(p_xml_ptr)) {
			p_node = xmlTextReaderCurrentNode(p_xml_ptr);
			if(p_node) {
				if(SXml::IsName(p_node, "E0065") && p_node->children) {
					THROWERR_STR(SXml::IsContent(p_node->children, "APERAK"), IEERR_INVMESSAGEYTYPE, "APERAK");
				}
				else if(p_node && SXml::IsName(p_node, "E1153") && p_node->children) {
					if(SXml::IsContent(p_node->children, "ON")) {
						// Запомним номер документа заказа
						if(p_node->next)
							if(SXml::IsName(p_node->next, "E1154") && p_node->next->children)
								AperakInfo.DocNum = (const char *)p_node->next->children->content;
					}
				}
				else if(p_node && SXml::IsName(p_node, "E2005") && p_node->children) {
					if(SXml::IsContent(p_node->children, ELEMENT_CODE_E2005_171)) { // Дата документа заказа
						if(p_node->next) {
							if(SXml::IsName(p_node->next, "E2380") && p_node->next->children) {
								strtodate((const char *)p_node->next->children->content, DATF_YMD|DATF_CENTURY, &AperakInfo.DocDate);
							}
						}
					}
				}
				else if(p_node && SXml::IsName(p_node, "E3035") && p_node->children) {
					str.Set(p_node->children->content); // Запомним значение элемента
					if(p_node->next) {
						p_node = p_node->next;
						if(SXml::IsName(p_node, "C082") && p_node->children) {
							if(SXml::IsName(p_node->children, ELEMENT_NAME_E3039)) {
								if(str.CmpNC("BY") == 0)
									AperakInfo.BuyerGLN = (const char *)p_node->children->children->content; // GLN покупателя
								else if(str.CmpNC("SU") == 0)
									AperakInfo.SupplGLN = (const char *)p_node->children->children->content; // GLN поставщика
								else if(str.CmpNC("DP") == 0)
									AperakInfo.AddrGLN = (const char *)p_node->children->children->content; // GLN адреса доставки
							}
						}
					}
				}
				else if(p_node && SXml::IsName(p_node, "E9321") && p_node->children) {
					AperakInfo.Code.Set(p_node->children->content); // Код статуса
				}
				else if(p_node && SXml::IsName(p_node, "E4451") && p_node->children) {
					if(SXml::IsContent(p_node->children, ELEMENT_CODE_E4451_AAO)) {
						if(p_node->next) {
							if(p_node->next->children) {
								if(SXml::IsName(p_node->next->children, "E4440") && p_node->next->children->children)
									AperakInfo.Msg.Set(p_node->next->children->children->content); // Описание статуса
							}
						}
					}
				}
				// Решили не передавать этот параметр
				//else if(p_node && (strcmp((const char *)p_node->name, "E4441") == 0) && p_node->children) {
				//	// Запомним дополнительное сообщение
				//	AperakInfo.AddedMsg = (const char *)p_node->children->content;
				//}
			}
		}
	}
	else
		ok = -1;
	CATCH
		SysLogMessage(SYSLOG_PARSEAPERAKRESP);
		ok = 0;
	ENDCATCH;
	xmlFreeParserInputBuffer(p_input);
	xmlFreeTextReader(p_xml_ptr);
	return ok;
}
//
// Общие функции для импорта/экспорта
//
EXPORT int FinishImpExp()
{
	int    ok = 1;
	SString str, err_track_id_list;
	if(P_ExportCls) {
		// Если EnumExpReceipt() не был запрошен
		if(P_ExportCls->P_XmlWriter) {
			THROW(P_ExportCls->EndDoc());
			xmlTextWriterEndDocument(P_ExportCls->P_XmlWriter);
			xmlFreeTextWriter(P_ExportCls->P_XmlWriter);
			P_ExportCls->P_XmlWriter = 0;
			THROW(P_ExportCls->SendDoc());
		}
	}
	if(P_ImportCls) {
		THROW(P_ImportCls->SetNewStatus(err_track_id_list = 0));
		if(!P_ImportCls->IncomMessagesCounter) {
			SysLogMessage(LOG_NOINCOMDOC);
			LogMessage(LOG_NOINCOMDOC);
		}
	}
	CATCH
		if(err_track_id_list.NotEmpty()) {
			(str = LOG_NEWSTATERR).Cat(err_track_id_list);
			SysLogMessage(str);
			LogMessage(str);
		}
		SysLogMessage(SYSLOG_FINISHIMPEXP);
		GetErrorMsg(str = 0);
		SysLogMessage(str);
		LogMessage(str);
		ok = 0;
	ENDCATCH;
	ZDELETE(P_ExportCls);
	ZDELETE(P_ImportCls);
	return ok;
}

EXPORT int GetErrorMessage(char * pMsg, uint bufLen)
{
	SString str;
	GetErrorMsg(str);
	memzero(pMsg, bufLen);
	if(str.NotEmpty() && pMsg)
		str.CopyTo(pMsg, bufLen < (str.Len() + 1) ? bufLen : (str.Len() + 1));
	ErrorCode = 0;
	WebServcErrorCode = 0;
	StrError = "";
	return 1;
}

void ProcessError(EDIWebServiceSoapProxy & rProxy)
{
	char   temp_err_buf[1024];
	SString temp_buf;
	ErrorCode = IEERR_SOAP;
	rProxy.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
	temp_buf = temp_err_buf;
	temp_buf.ReplaceChar('\xA', ' ');
	temp_buf.ReplaceChar('\xD', ' ');
	temp_buf.ReplaceStr("  ", " ", 0);
	StrError = temp_buf;
}
