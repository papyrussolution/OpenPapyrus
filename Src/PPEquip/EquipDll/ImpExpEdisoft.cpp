// IMPEXPEDISOFT.CPP
// Библиотека для импорта/экспорта документов в xml через web-сервис EDISoft
//
//
//
// ЭКСПОРТ
// (Order, Recadv)
// Входные парметры от Papyrus
//		Документ:
//		Сумма всего документа без НДС. Рассчитывается. (Order/Recadv)
//		Общее количество заказанного/принятого товара. Рассчитывается. (Order/Recadv)
//		Желаемая дата доставки - Bill.DueDate (Order)
//		Дата документа - Bill.Date (Order, Recadv)
//		Фактическая дата приема товара - Bill.Date (Recadv)
//		Номер заказа - Bill.OrderBillNo (Recadv) (правда, все равно не заполняется)
//		GLN отправителя и GLN покупателя - Bill.AgentGLN / Bill.MainGLN (Order, Recadv)
//		GLN получателя и GLN продавца - Bill.GLN (Order, Recadv)
//		GLN точки доставки - Bill.DlvrAddrCode / Bill.LocCode (Order, Recadv)
//		Адрес точки доставки - Bill.DlvrAddr (Order, Recadv) (опционально)
//		Сумма всего заказа с НДС - Bill.Amount (Order, Recadv)

//		Товарные строки:
//		Номер ТТН - BRow.TTN (Recadv) (Записывается в заголовке документа)
//		Штрихкод товара - BRow.Barcode (Order, Recadv)
//		Артикул товара - BRow.ArCode (Order)
//		Описание товара - BRow.GoodsName (Order, Recadv)
//		Количество заказанного/пришедшего товара - BRow.Quantity (Order/Recadv)
//		Количество в упаковке - BRow.UnitPerPack (Order, Recadv)
//		Цена с НДС - BRow.Cost (Order, Recadv)
//		Ставка НДС - BRow.VatRate (Order, Recadv)
//		Единицы измерения - BRow.UnitName (Order, Recadv)
//
// Выходные параметры в Papyrus
//		Метод EnumExpReceipt() структуру Sdr_DllImpExpReceipt (ИД заказа в Papyrus и соответствующий ему GUID в системе EDI)
//
//
//	ИМПОРТ
//  Входные/выходные параметры и порядок их обмена:
//		Из Papyrus - Вид операции (ORDRSP, DESADV или APERAK)
//					 Период, за который надо проверить докумуенты/статусы. Если его не указать, система EDI
//						просто не включит в параметры отбора это условие
//		Из Dll	   - В случае получения ORDRSP или DESADV - заполненная под максимуму структура Sdr_Bill
//					 В случае получения статуса - заполненные некоторые поля в Sdr_Bill
//		Из Papyrus - Сообщение о наличии документа заказа, на который получено подтверждение/уведомление/статус. Метод ReplyImportObjStatus, структура Sdr_DllImpObjStatus
//		Если такой заказ в Papyrus есть:
//			Из Dll - В случае получения ORDRSP или DESADV заполненная структура Sdr_BRow
//
//	Выходные параметры в Papyrus
//	ПОЛУЧЕНИЕ ORDRSP, DESADV (подтверждения, уведомления)
//		Документ:
//			Номер документа - Bill.Code (Ordrsp, Desadv)
//			Дата документа - Bill.Date (Ordrsp, Desadv)
//			Ожидаемая дата доставки - Bill.DueDate (Ordrsp, Desadv)
//			Дата заказа, на который пришел данный ответ - Bill.OrderDate (Ordrsp, Desadv)
//			Номер заказа, на который пришел данный ответ - Bill.OrderBillNo (Ordrsp, Desadv)
//			GLN получателя - Bill.MainGLN (Ordrsp, Desadv)
//			GLN получателя - Bill.MainGLN (Ordrsp, Desadv)
//			GLN отправителя - Bill.GLN (Ordrsp, Desadv)
//			GLN точки доставки - Bill.DlvrAddrCode (Ordrsp, Desadv)
//			Сумма документа с НДС - Bill.Amount (Ordrsp, Desadv)
//
//		Товарные строки:
//			Утвержденное/Отгруженное количество товара - BRow.Quantity (Ordrsp/Desadv)
//			Количество товара в упаковке - BRow.UnitPerPack (Ordrsp, Desadv)
//			Цена с НДС - BRow.Cost (Ordrsp, Desadv)
//			ИНН производителя. Для алкоголя - BRow.ManufINN (Desadv)
//			Тип алкогольной продукции (код) - BRow.GoodKindCode (Desadv)
//			Штрихкод товара - BRow.Barcode (Ordrsp, Desadv)
//			Описание товара - BRow.GoodsName (Ordrsp, Desadv)
//			Единицы измерения - BRow.UnitName (Ordrsp, Desadv)
//			Ставка НДС в % - BRow.VatRate (Ordrsp, Desadv)
//			Номер ТТН - BRow.TTN (Desadv)
//
//	ПОЛУЧЕНИЕ СТАТУСА
//		Номер документа заказа - Bill.OrderBillNo
//		Дата документа заказа - Bill.OrderDate
//		GLN покупателя - Bill.MainGLN
//		GLN поставщика - Bill.GLN
//		GLN точки поставки - Bill.DlvrAddrCode
//		GLN точки поставки - Bill.LocCode
//
//	Входные параметры из Papyrus
//		Статус заказа в Papyrus - 0 - нет такого заказа, 1 - есть такой заказ. Метод ReplyImportObjStatus, структура Sdr_DllImpObjStatus.
//
//
#include <slib.h>
#include <sxml.h>
#include <ppedi.h>
#include <ppsoapclient.h>
#include <Edisoft\edisoftEDIServiceSoapProxy.h>

#define EXPORT	extern "C" __declspec (dllexport)
#define THROWERR(expr,val) { if(!(expr)){ SetError(val, ""); goto __scatch; } }
#define THROWERR_STR(expr,val,str) { if(!(expr)){ SetError(val, str); goto __scatch; } }

#define UNIT_NAME_KG			"КГ"
#define UNIT_NAME_PIECE			"Штука"
#define EMPTY_LISTMB_RESP		"<mailbox-response></mailbox-response>"

// Имена элементов xml-ответов web-сервиса
#define WEB_ELEMENT_NAME_TRACKID		"tracking-id"		// ИД документа в системе
#define WEB_ELEMENT_NAME_PARTNER_ILN	"partner-iln"		// Идентификатор партнера
#define WEB_ELEMENT_NAME_DIRECTION		"direction"			// Входящий или исходящий документ
#define WEB_ELEMENT_NAME_DOCTYPE		"document-type"		// Тип документа
#define WEB_ELEMENT_NAME_DOCVERSION		"document-version"	// Версия спецификации
#define WEB_ELEMENT_NAME_DOCSTANDART	"document-standard"	// Стандарт документа
#define WEB_ELEMENT_NAME_DOCTEST		"document-test"		// Статус документа

// Значения элементов-идентификатров xml-ответов и запросов web-сервиса
#define WEB_ELEMENT_CODE_DIRECTION_IN	"IN"		// Входящий документ
#define WEB_ELEMENT_CODE_DIRECTION_OUT	"OUT"		// Исходящий документ
#define WEB_ELEMENT_CODE_TYPE_ORDERS	"ORDER" 	// Заказ
#define WEB_ELEMENT_CODE_TYPE_ORDRSP	"ORDRSP"	// Подтверждение заказа
#define WEB_ELEMENT_CODE_TYPE_APERAK	"APERAK"	// Статус документа
#define WEB_ELEMENT_CODE_TYPE_DESADV    "DESADV"    // Уведомление об отгрузки товара поставщиком (накладная)
#define WEB_ELEMENT_CODE_TYPE_RECADV    "RECADV"    // Уведомление о прибытии товара
#define WEB_ELEMENT_CODE_STATUS_NEW		"N"			// Статус докуемнта "новый"
#define WEB_ELEMENT_CODE_STATUS_READ	"R"			// Статус документа "прочитан"

// Названия элементов идентификаторов
#define ELEMENT_NAME_DELIVERDATE        "ExpectedDeliveryDate"
#define ELEMENT_NAME_DOCNAMECODE        "DocumentNameCode"
#define ELEMENT_NAME_DOCPARTIES         "Document-Parties"
#define ELEMENT_NAME_ORDERPARTIES       "Order-Parties"
#define ELEMENT_NAME_BUYER              "Buyer"
#define ELEMENT_NAME_SELLER             "Seller"
#define ELEMENT_NAME_ORDERLINES         "Order-Lines"
#define ELEMENT_NAME_NAME               "Name"
#define ELEMENT_NAME_CITYNAME           "CityName"
#define ELEMENT_NAME_COUNTRY            "Country"
#define ELEMENT_NAME_TAXRATE            "TaxRate"
#define ELEMENT_NAME_ORDERSUMMARY       "Order-Summary"
#define ELEMENT_NAME_TOTALLINES         "TotalLines"
#define ELEMENT_NAME_TOTALNETAMOUNT     "TotalNetAmount"
#define ELEMENT_NAME_TOTALGROSSAMOUNT   "TotalGrossAmount"
#define ELEMENT_NAME_ORDRSPNUMBER       "OrderResponseNumber"
#define ELEMENT_NAME_ORDRSPDATE         "OrderResponseDate"
#define ELEMENT_NAME_BUYERORDERNUMBER   "BuyerOrderNumber"
#define ELEMENT_NAME_BUYERORDERDATE     "BuyerOrderDate"
#define ELEMENT_NAME_ITEMDISCR          "ItemDescription"
#define ELEMENT_NAME_ITEMSTATUS         "ItemStatus"
#define ELEMENT_NAME_ALLOCDELIVRD       "AllocatedDelivered"
#define ELEMENT_NAME_DESPATCHADVICEDATE "DespatchAdviceDate"
#define ELEMENT_NAME_QTTYDISPATCHED     "QuantityDespatched"
#define ELEMENT_NAME_ALCOCONTENT        "AlcoholContent"
#define ELEMENT_NAME_LICENSE            "License"
#define ELEMENT_NAME_SERIES             "Series"
#define ELEMENT_NAME_NUMBER             "Number"
#define ELEMENT_NAME_IISSUINAUTH        "IssuingAuthority"
#define ELEMENT_NAME_DATEOFISSUE        "DateOfIssue"
#define ELEMENT_NAME_EXPARITIONDATE     "ExpirationDate"
#define ELEMENT_NAME_PARTYTYPE          "Party-Type"
#define ELEMENT_NAME_TAXID              "TaxID"
#define ELEMENT_NAME_TAXRECREASONCODE   "TaxRegistrationReasonCode"
#define ELEMENT_NAME_CERTIFICATE        "Certificate"
#define ELEMENT_NAME_TYPE               "Type"
#define ELEMENT_NAME_DOCRECADV          "Document-ReceivingAdvice"
#define ELEMENT_NAME_DOCNUMBER          "DocumentNumber"
#define ELEMENT_NAME_DOCDATE            "DocumentDate"
#define ELEMENT_NAME_SYSTEMSGTEXT       "SystemMessageText"
#define ELEMENT_NAME_DOCUMENTID         "DocumentID"
#define ELEMENT_NAME_UNITPACKSZ         "UnitPacksize"
#define ELEMENT_NAME_UNITGROSSPRICE     "UnitGrossPrice"
#define ELEMENT_NAME_UNITNETPRICE		"UnitNetPrice"
#define ELEMENT_NAME_NETAMOUNT			"NetAmount"
#define ELEMENT_NAME_GROSSAMOUNT		"GrossAmount"

// Значения элементов идентификаторов
#define ELEMENT_CODE_COUNTRY_RUS       "RU"     // Код страны - Россия
#define ELEMENT_CODE_UNITOFMEASURE_PCE "PCE"    // Единицы измерения - штуки
#define ELEMENT_CODE_UNITOFMEASURE_KGM "KGM"    // Единицы измерения - килограмы
#define ELEMENT_CODE_UNITOFMEASURE_DPA "DPA"    // Единицы измерения - ящики
#define ELEMENT_CODE_PARTYTYPE_MF      "MF"     // Тип стороны - производитель
#define ELEMENT_CODE_REFTYPE_YA1       "YA1"    // Тип поля - код вида продукции
#define ELEMENT_CODE_REFTYPE_ABT       "ABT"    // Тип поля - ГТД
#define ELEMENT_CODE_REFTYPE_SER       "SER"    // Тип поля - серия (ГТД с дополнительной цифрой после слеша)

// Коды ошибок и сообщений
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
#define IEERR_ONLYEXPMSGTYPES		19			// Операция экспорта работает только с типами команд ORDER и RECADV
#define IEERR_NOCONFIG				20			// Не найдена конфигурация для данного типа операции
#define IEERR_NOEXPPATH				21			// Не определена директория для файла экспорта
#define IEERR_PARSERESPONSEFAILD    22          // Ошибка разбора ответа от системы провайдера
#define IEERR_TTNNEEDED				23			// Документ не создан. Необходимо указать ТТН. Документ: %s// Причем ТТН из DESADV, а не из бумажного документа прихода

// Коды ошибок веб-сервиса
#define IEWEBERR_AUTH				1			// Ошибка аутентификации
#define IEWEBERR_CORRELATION		2			// Ошибка во взаимосвязи
#define IEWEBERR_EXTERNAL			3			// Внешняя ошибка
#define IEWEBERR_SERVER				4			// Внутрення ошибка сервера
#define IEWEBERR_TIMELIMIT			5			// Превышен таймаут на выполнение метода
#define IEWEBERR_WEBERR				6			// Ошибка Web
#define IEWEBERR_PARAMS				7			// Некорректные параметры

// Коды ошибок сообщений для лог-файла
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
#define SYSLOG_ORDERHEADER				"in OrderHeader()"
#define SYSLOG_RECADVHEADER				"in RecadvHeader()"
#define SYSLOG_DOCPARTIES				"in DocParties"
#define SYSLOG_GOODSLINES				"GoodsLines"
#define SYSLOG_ENDDOC					"EndDoc"
#define SYSLOG_SENDDOC					"in SendDoc()"
#define SYSLOG_RECEIVEDOC				"in ReceiveDoc()"
#define SYSLOG_PARSELISTMBRESP			"in ParseListMBResp()"
#define SYSLOG_PARSEFORDOCDATA			"in ParseForDocData()"
#define SYSLOG_PARSEFORGOODDATA			"in ParseForGoodData()"
#define SYSLOG_SETREADSTATUS			"in SetReadStatus()"
#define SYSLOG_SETNEWSTATUS				"in SetNewStatus()"
#define SYSLOG_LISTMESSAGEBOX			"in ListMessageBox()"
#define SYSLOG_PARSEAPERAKRESP			"in ParseAperakResp()"

#define LOG_NOINCOMDOC					"Нет входящих сообщений"
#define LOG_READSTATERR					"Сбой при выставлении статуса READ для документа "

class ExportCls;
class ImportCls;

int ErrorCode = 0;
int WebServcErrorCode = 0;
SString StrError = "";
SString LogName = "";
SString SysLogName = "";
static ExportCls * P_ExportCls = 0;
static ImportCls * P_ImportCls = 0;

int GetObjTypeBySymb(const char * pSymb, uint & rType);
void ProcessError(EDIServiceSoapProxy & rProxy);

int SetError(int errCode, const char * pStr = "")
{
	ErrorCode = errCode;
	StrError = pStr;
	return 0;
}

int SetWebServcError(int errCode, const char * pStrErr = 0)
{
	WebServcErrorCode = errCode;
	StrError = pStrErr;
	return 0;
}

void LogMessage(const char * pMsg);
void SysLogMessage(const char * pMsg);
void FormatLoginToGLN(const char * login, SString & rStr);
void FormatLoginToLogin(const char * login, SString & rStr);

struct ErrMessage {
	uint Id;
	const char * P_Msg;
};

struct WebServcErrMessage {
	uint Id;
	const char * P_Msg;
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

MessageTypeSymbols MsgSymbols[] = {
	{"ORDER",       PPEDIOP_ORDER},
	{"ORDRSP",      PPEDIOP_ORDERRSP},
	{"APERAK",      PPEDIOP_APERAK},
	{"DESADV",      PPEDIOP_DESADV},
	{"RECADV",      PPEDIOP_RECADV}
};

ErrMessage ErrMsg[] = {
	{IEERR_SYMBNOTFOUND,		"Символ не найден"},
	{IEERR_NODATA,				"Данные не переданы"},
	{IEERR_NOSESS,				"Сеанса с таким номером нет"},
	{IEERR_ONLYBILLS,			"Dll может работать только с документами"},
	{IEERR_NOOBJID,				"Объекта с таким идентификатором нет"},
	{IEERR_IMPEXPCLSNOTINTD,	"Объект для импорта/экспорта не инициализирован"},
	{IEERR_WEBSERVСERR,			"Ошибка Web-сервиса: "},
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
	{IEERR_ONLYEXPMSGTYPES,		"Операция экспорта работает только с типами команд ORDER и RECADV"},
	{IEERR_NOCONFIG,			"Не найдена конфигурация для данного типа операции"},
	{IEERR_NOEXPPATH,			"Не опредеена директория для файла экспорта"},
	{IEERR_PARSERESPONSEFAILD,	"Ошибка разбора ответа от системы провайдера"},
	{IEERR_TTNNEEDED,			"Документ не создан. Необходимо указать ТТН. Документ: "}
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
	SString str;
	int dd = 0, mm = 0, yy = 0, hh = 0, min = 0, ss = 0;
	LDATE date;
	LTIME time;
	if(file.IsValid()) {
		 getcurdate(&date);
		 getcurtime(&time);
		str.Z().Cat(date.day()).Dot().Cat(date.month()).Dot().Cat(date.year()).Space().Cat(time.hour()).
			CatChar(':').Cat(time.minut()).CatChar(':').Cat(time.sec()).Tab().Cat(pMsg).CR();
        file.WriteLine(str);
	}
}

void SysLogMessage(const char * pMsg)
{
	SFile file(SysLogName, SFile::mAppend);
	SString str;
	int dd = 0, mm = 0, yy = 0, hh = 0, min = 0, ss = 0;
	LDATE date;
	LTIME time;
	if(file.IsValid()) {
		 getcurdate(&date);
		 getcurtime(&time);
		str.Z().Cat(date.day()).Dot().Cat(date.month()).Dot().Cat(date.year()).Space().Cat(time.hour()).
			CatChar(':').Cat(time.minut()).CatChar(':').Cat(time.sec()).Tab().Cat(pMsg).CR();
        file.WriteLine(str);
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
	if((ErrorCode == IEERR_WEBSERVСERR)) {
		for(size_t i = 0; i < SIZEOFARRAY(ErrMsg); i++) {
			if(WebServcErrMsg[i].Id == WebServcErrorCode) {
				(str).Cat(WebServcErrMsg[i].P_Msg).Space().Cat(StrError.Utf8ToChar());
				StrError.ToUtf8(); // Обратно переводим, ибо если запросится второй раз описание ошибки, то StrError уже будет переведен в Char и в Папиросе будет шляпа, а не сообщение
				break;
			}
		}
	}
	if((ErrorCode == IEERR_SOAP) || (ErrorCode == IEERR_IMPFILENOTFOUND) || (ErrorCode == IEERR_INVMESSAGEYTYPE) || (ErrorCode  == IEERR_NOCFGFORGLN))
		str.Cat(StrError);
	rMsg.Z().CopyFrom(str);
}

// Переделывает строку логина в строку GLN (то есть без EC на конце)
// Ибо логин 4607806659997EC, а GLN Отправителя должен быть 4607806659997
void FormatLoginToGLN(const char * login, SString & rStr)
{
	uint exit_while = 0;
	rStr.Z();
	if(login) {
		while(!exit_while) {
			if((*login == 0) || !isdec(*login))
				exit_while = 1;
			else {
				rStr.CatChar(*login);
				login ++;
			}
		}
	}
}

// Удаляет из строки логина лишние символы
// Ибо логин 4607806659997EC_1, а должен быть 4607806659997ЕС
void FormatLoginToLogin(const char * login, SString & rStr)
{
	uint exit_while = 0;
	char low_strip = '_';
	rStr.Z();
	if(login) {
		while(!exit_while) {
			if((*login == 0) || (*login == low_strip))
				exit_while = 1;
			else {
				rStr.CatChar(*login);
				login ++;
			}
		}
	}
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
// Returns:
//		0 - символ не найден
//		1 - символ найден
int GetMsgTypeBySymb(const char * pSymb, uint & rType)
{
	for(size_t i = 0; i < SIZEOFARRAY(MsgSymbols); i++) {
		if(strcmp(MsgSymbols[i].P_Symb, pSymb) == 0) {
			rType = MsgSymbols[i].Type;
			return 1;
		}
	}
	return 0;
}

class Iterator {
public:
	void Init() { Count = 0; }
	const uint GetCount() { return Count; }
	void Next() { Count++; }
private:
	uint Count;
};
//
// Класс, от которого наследуются ImportCls и ExportCls. Содержит общие методы, для этих классов
//
class ImportExportCls {
public:
	ImportExportCls()
	{
	}
	~ImportExportCls()
	{
	}
	void CleanHeader()
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
	Sdr_ImpExpHeader Header;
	SString TTN;  // Для RECADV. Получаем из первой полученной строки документа подтверждения прихода товара
					 // Для DESADV. Получаем из заголовка документа и пихаем во все товарные строки
};

EXPORT int FinishImpExp();

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch(dwReason) {
		case DLL_PROCESS_ATTACH:
			{
				SString product_name;
				(product_name = "Papyrus-ImpExpEdisoft");
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
	void CreateFileName(uint num);
	int OrderHeader();
	int RecadvHeader();
	int DocParties();
	int GoodsLines(Sdr_BRow * pBRow);
	int EndDoc();
	int SendDoc();
	uint Id;					// ИД, который Papyrus будет воспринимать как ИД сеанса экспорта
	uint ObjId;					// ИД экспортируемого объекта (например, для пакета документов каждому документу соответствует свой ИД)
	uint ObjType;				// Тип экспортируемых объектов
	uint MessageType;			// Тип сообщения: ORDER, RECADV
	uint Inited;
	uint ReadReceiptNum;		// Порядковый номер квитанции, прочитанной из ReceiptList
	double BillSumWithoutVat;	// Сумма документа без НДС (придется считать самостоятельно, ибо в Sdr_Bill этот праметр не передается)
	SString ExpFileName;
	xmlTextWriter * P_XmlWriter;
	Iterator Itr;
	SPathStruc PathStruct;
	Sdr_Bill Bill;
	// Состоит из типов Sdr_DllImpExpReceipt
	TSCollection <Sdr_DllImpExpReceipt> ReceiptList; // Список квитанций об отправленных документах
private:
	double TotalGoodsCount;     // В документах надо указывать общее количество товара. Придется считать самостоятельно
};

ExportCls::ExportCls() : Id(0), ObjId(0), ObjType(0), MessageType(0), Inited(0), ReadReceiptNum(0), BillSumWithoutVat(0.0), P_XmlWriter(0)
{
	LogName.Z();
	ErrorCode = 0;
	WebServcErrorCode = 0;
	TotalGoodsCount = 0;
	TTN.Z();
}

ExportCls::~ExportCls()
{
	P_XmlWriter = 0;
}

void ExportCls::CreateFileName(uint num)
{
	ExpFileName.Z().Cat(PathStruct.Drv).CatChar(':').Cat(PathStruct.Dir).Cat(PathStruct.Nam).Cat(num).Dot().Cat(PathStruct.Ext);
}
//
// Работа с форматами документов и сообщений
//
// Начало и заголовок заказа
//
int ExportCls::OrderHeader()
{
	int ok = 1;
	SString str, fmt;
	THROWERR(P_XmlWriter, IEERR_NULLWRIEXMLPTR);
	BillSumWithoutVat = 0.0;
	xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Document-Order");
	{
		SXml::WNode n_hdr(P_XmlWriter, "Order-Header");
		{
			n_hdr.PutInner("OrderNumber", (str = Bill.Code).ToUtf8()); // Номер заказа
			{
				fmt.Z().Cat(Bill.Date.month());
				if(fmt.Len() == 1)
					fmt.PadLeft(1, '0');
				str.Z().Cat(Bill.Date.year()).CatChar('-').Cat(fmt);
				fmt.Z().Cat(Bill.Date.day());
				if(fmt.Len() == 1)
					fmt.PadLeft(1, '0');
                str.CatChar('-').Cat(fmt);
				n_hdr.PutInner("OrderDate", str); // Дата заказа
			}
			n_hdr.PutInner("OrderCurrency", "RUR"); // Код валюты (Рубли)
			{
				LDATE date;
				if(Bill.DueDate != ZERODATE)
					date = Bill.DueDate;
				else
					date = Bill.Date;
				fmt.Z().Cat(date.month());
				if(fmt.Len() == 1)
					fmt.PadLeft(1, '0');
				str.Z().Cat(date.year()).CatChar('-').Cat(fmt);
				fmt.Z().Cat(date.day());
				if(fmt.Len() == 1)
					fmt.PadLeft(1, '0');
				str.CatChar('-').Cat(fmt);
				n_hdr.PutInner(ELEMENT_NAME_DELIVERDATE, str); // Желаемая дата доставки (опционально)
			}
			n_hdr.PutInner("DocumentFunctionCode", "9"); // Код функции документа (Оригинал)
			n_hdr.PutInner(ELEMENT_NAME_DOCNAMECODE, "220"); // Код типа документа (Заказ)
		}
	}
	CATCH
		SysLogMessage(SYSLOG_ORDERHEADER);
		ok = 0;
	ENDCATCH;
	return ok;
}
//
// Начало и заголовок документа о приемке товара
//
int ExportCls::RecadvHeader()
{
	int ok = 1;
	SString str, fmt;
	THROWERR(P_XmlWriter, IEERR_NULLWRIEXMLPTR);
	BillSumWithoutVat = 0.0;
	xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_DOCRECADV);
		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"ReceivingAdvice-Header");
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"ReceivingAdviceNumber"); // Номер документа уведомления
			//{
			//	SString str1;
			//	str1.Z().Cat(Bill.Code).ToUtf8();
			//	str.Z().Cat("<![CDATA[").Cat(str1).Cat("]]>"); // Делаем конструкцию <![CDATA[какая-то строка]]>, ибо благодаря этому спец символы воспринимаются системой как обычные
			//	xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
			//}
				xmlTextWriterWriteString(P_XmlWriter, (str = Bill.Code).ToUtf8().ucptr());
			xmlTextWriterEndElement(P_XmlWriter); // RECADVNUMBER
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"ReceivingAdviceDate"); // Дата создания уведомления
				fmt.Z().Cat(Bill.Date.month());
				if(fmt.Len() == 1)
					fmt.PadLeft(1, '0');
				str.Z().Cat(Bill.Date.year()).CatChar('-').Cat(fmt);
				fmt.Z().Cat(Bill.Date.day());
				if(fmt.Len() == 1)
					fmt.PadLeft(1, '0');
                str.CatChar('-').Cat(fmt);
				xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
			xmlTextWriterEndElement(P_XmlWriter); // RECADVDATE
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"GoodsReceiptDate"); // Фактическая дата приема товара
				fmt.Z().Cat(Bill.Date.month());
				if(fmt.Len() == 1)
					fmt.PadLeft(1, '0');
				str.Z().Cat(Bill.Date.year()).CatChar('-').Cat(fmt);
				fmt.Z().Cat(Bill.Date.day());
				if(fmt.Len() == 1)
					fmt.PadLeft(1, '0');
                str.CatChar('-').Cat(fmt);
				xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
			xmlTextWriterEndElement(P_XmlWriter); // GOODSRECDATE
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_BUYERORDERNUMBER); // Номер заказа
				xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.OrderBillNo); // Все равно пустое значение
			xmlTextWriterEndElement(P_XmlWriter); // BUYERORDERNUMBER
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"DespatchNumber"); // Номер ТТН
				xmlTextWriterWriteString(P_XmlWriter, TTN.Transf(CTRANSF_INNER_TO_UTF8).ucptr());
			xmlTextWriterEndElement(P_XmlWriter); // DESPATCHNUMBER
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"DocumentFunctionCode"); // Код функции документа
				xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"9"); // Оригинал
			xmlTextWriterEndElement(P_XmlWriter); // DOCFUNCTYPE
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_DOCNAMECODE); // Код типа документа
				xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"351"); // Уведомление о поставке
			xmlTextWriterEndElement(P_XmlWriter); // DOCNAMECODE
		xmlTextWriterEndElement(P_XmlWriter); // DOCORDERHDR

	CATCH
		SysLogMessage(SYSLOG_RECADVHEADER);
		ok = 0;
	ENDCATCH;
	return ok;
}

// Описание участников обмена
// Примечание: Sender = Buyer (покупатель)
//             Reciever = Seller (поставщик)
//
int ExportCls::DocParties()
{
	int    ok = 1;
	SString str, sender_gln, login;
	THROWERR(P_XmlWriter, IEERR_NULLWRIEXMLPTR);
		{
			SXml::WNode n_dp(P_XmlWriter, ELEMENT_NAME_DOCPARTIES);
			{
				{
					SXml::WNode n_sender(P_XmlWriter, "Sender"); // Отправитель
					// В EDISoft GLN отправителя совпадает с логином за некотрой разницей
					// Пошла таким путем, чтобы в настройках в Папирусе лишний раз ничего не менять, ибо в Папирусе
					// в GLN организации указан GLN склада, то есть точки доставки
					FormatLoginToGLN(Header.EdiLogin, sender_gln.Z());
					n_sender.PutInner("ILN", sender_gln); // GLN отправителя
					//// Если GLN агента пусто, то пишем GLN главной организации (оставила по-старому, ибо иначе будут проблемы при получении входящих документов)
					//if(!isempty(Bill.AgentGLN))
					//	xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.AgentGLN);
					//else
					//	xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.MainGLN);
				}
				{
					SXml::WNode n_rcvr(P_XmlWriter, "Receiver"); // Получатель
					n_rcvr.PutInner("ILN", Bill.GLN); // GLN получателя
				}
			}
		}
		if(MessageType == PPEDIOP_ORDER)
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_ORDERPARTIES);
		else if(MessageType == PPEDIOP_RECADV)
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"ReceivingAdvice-Parties");
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_BUYER); // Покупатель
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"ILN"); // GLN покупателя
					FormatLoginToGLN(Header.EdiLogin, login.Z());
					xmlTextWriterWriteString(P_XmlWriter, login.ucptr());
					//// Если GLN агента пусто, то пишем GLN главной организации
					//if(!isempty(Bill.AgentGLN))
					//	xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.AgentGLN);
					//else
					//	xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.MainGLN);
				xmlTextWriterEndElement(P_XmlWriter); // ILN
				//xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_NAME); // Название покупателя (опционально)
				//xmlTextWriterEndElement(P_XmlWriter); // NAME
			xmlTextWriterEndElement(P_XmlWriter); // BUYER
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_SELLER); // Продавец
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"ILN"); // GLN продавца
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.GLN);
				xmlTextWriterEndElement(P_XmlWriter); // ILN
				//xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_NAME); // Название продавца (опционально)
				//	xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)Bill.ContragName); // Может быть, это поле
				//xmlTextWriterEndElement(P_XmlWriter); // NAME
			xmlTextWriterEndElement(P_XmlWriter); // SELLER
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"DeliveryPoint"); // Точка доставки
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"ILN"); // GLN точки доставки
					if(!isempty(Bill.DlvrAddrCode))
						str.Z().Cat(Bill.DlvrAddrCode);
					else
						str.Z().Cat(Bill.LocCode);
					xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
				xmlTextWriterEndElement(P_XmlWriter); // ILN
				//xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_NAME); // Название точки доставки (опционально)
				//xmlTextWriterEndElement(P_XmlWriter); // NAME
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"StreetAndNumber"); // Адрес точки доставки (улица и дом) (опционально)
					(str = Bill.DlvrAddr).ToUtf8();
					xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
				xmlTextWriterEndElement(P_XmlWriter); // ADDRESS
				//xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_CITYNAME); // Город точки доставки (опционально)
				//xmlTextWriterEndElement(P_XmlWriter); // CITYNAME
				//xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"PostalCode"); // Почтовый индекс точки доставки (опционально)
				//xmlTextWriterEndElement(P_XmlWriter); // POSTALCODE
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_COUNTRY); // Страна точки доставки (опционально)
					xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_COUNTRY_RUS); // Россия
				xmlTextWriterEndElement(P_XmlWriter); // COUNTRY
			xmlTextWriterEndElement(P_XmlWriter); // DELIVERYPOINT
		xmlTextWriterEndElement(P_XmlWriter); // ORDERPARTIES/RECADVPARTIES
		if(MessageType == PPEDIOP_ORDER)
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_ORDERLINES);
		else if(MessageType == PPEDIOP_RECADV)
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"ReceivingAdvice-Lines");

	CATCH
		SysLogMessage(SYSLOG_DOCPARTIES);
		ok = 0;
	ENDCATCH;
	return ok;
}

int ExportCls::GoodsLines(Sdr_BRow * pBRow)
{
	int    ok = 1;
	SString str;
	THROWERR(P_XmlWriter, IEERR_NULLWRIEXMLPTR);
	THROWERR(pBRow, IEERR_NODATA);
	//
	// Для RECADV
	// Если ТТН не указан уже в первой строке, то документ не создаем. Ибо строки запишутся только после записи шапки документа,
	// которая в свою очередь записывается только при наличии ТТН
	//
	if(MessageType == PPEDIOP_RECADV)
		THROWERR_STR(!isempty(pBRow->TTN), IEERR_TTNNEEDED, Bill.Code);
	// Шапку и участников обмена пишем один раз.
	// На наличие pBRow->TTN можно заложиться, ибо его надо обязательно указать, так как в системе провайдера
	// идет привязка RECADV к DESADV именно по этому параметру.
	// ТТН у всех строк одинаковый, ибо пришли от одного DESADV
	if(MessageType == PPEDIOP_RECADV && TTN.Empty()) {
		TTN = pBRow->TTN;
		THROW(P_ExportCls->RecadvHeader());
		THROW(P_ExportCls->DocParties());
	}
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Line");
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Line-Item");
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"LineNumber"); // Порядковый номер товара в заказе
						xmlTextWriterWriteString(P_XmlWriter, str.Z().Cat(Itr.GetCount() + 1).ucptr());
					xmlTextWriterEndElement(P_XmlWriter); // LINENUMBER
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"EAN"); // Штрихкод товара
						xmlTextWriterWriteString(P_XmlWriter, str.Z().Cat(pBRow->Barcode).ucptr());
					xmlTextWriterEndElement(P_XmlWriter); // EAN
					if(!isempty(pBRow->ArCode)) {
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"SupplierItemCode"); // Артикул товара поставщика
							xmlTextWriterWriteString(P_XmlWriter, (str = pBRow->ArCode).Transf(CTRANSF_INNER_TO_UTF8).ucptr());
						xmlTextWriterEndElement(P_XmlWriter); // SUPPITEMCODE
					}
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_ITEMDISCR); // Описание товара (имя) (опционально)
						{
							str.Z().Cat(pBRow->GoodsName).ToUtf8(); // Провайдер требует эту кодировку
							xmlTextWriterWriteString(P_XmlWriter, SXml::WNode::CDATA(str).ucptr()); // Наименование товара
						}
					xmlTextWriterEndElement(P_XmlWriter); // ITEMDISCR
					if(MessageType == PPEDIOP_ORDER)
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"OrderedQuantity"); // Количество заказанного товара
					else if(MessageType == PPEDIOP_RECADV)
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"QuantityReceived"); // Количество пришедшего товара
							// В этом документообороте нужно учитывать количество упаковок, ибо есть тут такие единицы измерения
							//if(pBRow->UnitPerPack) {
							//	double qtty_pack = pBRow->Quantity / pBRow->UnitPerPack;
							//	str.Z().Cat(qtty_pack);
							//	str.Printf("%.3f", qtty_pack); // Число знаков после запятой должно быть не больше 3-х
							//	TotalGoodsCount += qtty_pack;
							//}
							//else {
								str.Z().Cat(pBRow->Quantity);
								TotalGoodsCount += pBRow->Quantity;
							//}
							xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
						xmlTextWriterEndElement(P_XmlWriter); // ORDEREDQTTY/RECEIVEDQTTY
					if(pBRow->UnitPerPack) {
						if(MessageType == PPEDIOP_ORDER)
							xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"OrderedUnitPacksize"); // Количество в упаковке (заказанное)
						else if(MessageType == PPEDIOP_RECADV)
							xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_UNITPACKSZ); // Количество в упаковке (пришедшее)
							//str.Z().Cat(pBRow->UnitPerPack);
							//xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
							xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)"1");
						xmlTextWriterEndElement(P_XmlWriter); // ORDEREDPACKSZ/UNITPACKSZ
					}

					// У ORDER и RECADV отличается порядок следования последних трех полей
					// Плюс в ORDER есть taxrate и суммы по строке, а в RECADV - нет
					if(MessageType == PPEDIOP_ORDER) {
						{
							SXml::WNode n_uom(P_XmlWriter, "UnitOfMeasure"); // Единицы измерения
							{
								(str = pBRow->UnitName).ToUpper1251();
								if(str.CmpNC(UNIT_NAME_KG) == 0)
									xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_UNITOFMEASURE_KGM); // Килограммы
								else {
									// Если товар в упаковках, то пишем упаковки
									//if(pBRow->UnitPerPack)
									//	xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_UNITOFMEASURE_DPA); // Упаковки
									// Иначе штуки
									//else
										xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_UNITOFMEASURE_PCE); // Отдельные элементы
								}
							}
						}
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"OrderedUnitNetPrice"); // Цена без НДС (заказ)
							//double cost = pBRow->Cost - (pBRow->Cost / (pBRow->VatRate + 100) * 100);
							double cost = (pBRow->Cost / (pBRow->VatRate + 100) * 100);
							str.Z().Printf("%.3f", cost); // Число знаков после запятой должно быть не больше 3-х
							xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
						xmlTextWriterEndElement(P_XmlWriter); // ORDEREDNETPRICE
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"OrderedUnitGrossPrice"); // Цена с НДС (заказ)
							str.Z().Printf("%.3f", pBRow->Cost); // Число знаков после запятой должно быть не больше 3-х
							xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
						xmlTextWriterEndElement(P_XmlWriter); // ORDEREDGROSSPRICE
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"OrderedNetAmount"); // Сумма без НДС
							//double amount = (pBRow->Cost - (pBRow->Cost / (pBRow->VatRate + 100) * 100)) * pBRow->Quantity;
							double amount = ((pBRow->Cost / (pBRow->VatRate + 100) * 100)) * pBRow->Quantity;
							str.Z().Printf("%.3f", amount); // Число знаков после запятой должно быть не больше 3-х
							BillSumWithoutVat += str.ToReal();
							xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
						xmlTextWriterEndElement(P_XmlWriter); // ORDEREDNETAMOUNT
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"OrderedGrossAmount"); // Сумма с НДС
							str.Z().Printf("%.3f", pBRow->Cost * pBRow->Quantity); // Число знаков после запятой должно быть не больше 3-х
							xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
						xmlTextWriterEndElement(P_XmlWriter); // ORDEREDGROSSAMOUNT
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_TAXRATE); // Налоговая ставка % (опционально)
							str.Z().Printf("%.3f", pBRow->VatRate); // Число знаков после запятой должно быть не больше 3-х
							xmlTextWriterWriteString(P_XmlWriter, str.ucptr()); // Ставка НДС
						xmlTextWriterEndElement(P_XmlWriter); // TAXRATE
					}
					else if(MessageType == PPEDIOP_RECADV) {
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_UNITGROSSPRICE); // Цена с НДС (приход)
							str.Z().Printf("%.3f", pBRow->Cost); // Число знаков после запятой должно быть не больше 3-х
							xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
						xmlTextWriterEndElement(P_XmlWriter); // UNITGROSSPRICE
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_UNITNETPRICE); // Цена без НДС (приход)
							//double price = pBRow->Cost - (pBRow->Cost / (pBRow->VatRate + 100) * 100);
							double price = (pBRow->Cost / (pBRow->VatRate + 100) * 100);
							str.Z().Printf("%.3f", price); // Число знаков после запятой должно быть не больше 3-х
							xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
						xmlTextWriterEndElement(P_XmlWriter); // UNITNETPRICE
						{
							SXml::WNode n_uom(P_XmlWriter, "UnitOfMeasure"); // Единицы измерения
							{
								(str = pBRow->UnitName).Utf8ToChar().ToUpper().Transf(CTRANSF_INNER_TO_OUTER);
								if(str.CmpNC(UNIT_NAME_KG) == 0)
									xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_UNITOFMEASURE_KGM); // Килограммы
								else {
									// Если товар в упаковках, то пишем упаковки
									//if(pBRow->UnitPerPack)
									//	xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_UNITOFMEASURE_DPA); // Упаковки
									// Иначе штуки
									//else
									xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_UNITOFMEASURE_PCE); // Отдельные элементы
								}
							}
						}

						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_NETAMOUNT); // Сумма без НДС
							//double amount = (pBRow->Cost - (pBRow->Cost / (pBRow->VatRate + 100) * 100)) * pBRow->Quantity;
							double amount = ((pBRow->Cost / (pBRow->VatRate + 100) * 100)) * pBRow->Quantity;
							str.Z().Printf("%.3f", amount); // Число знаков после запятой должно быть не больше 3-х
							BillSumWithoutVat += str.ToReal();
							xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
						xmlTextWriterEndElement(P_XmlWriter); // NETAMOUNT
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_GROSSAMOUNT); // Сумма с НДС
							str.Z().Printf("%.3f", pBRow->Cost * pBRow->Quantity); // Число знаков после запятой должно быть не больше 3-х
							xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
						xmlTextWriterEndElement(P_XmlWriter); // GROSSAMOUNT
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_TAXRATE); // Налоговая ставка % (опционально)
							str.Z().Printf("%.3f", pBRow->VatRate); // Число знаков после запятой должно быть не больше 3-х
							xmlTextWriterWriteString(P_XmlWriter, str.ucptr()); // Ставка НДС
						xmlTextWriterEndElement(P_XmlWriter); // TAXRATE
					}
				xmlTextWriterEndElement(P_XmlWriter); // LINEITEM
				// @vmiller {
				// Добавлена дополнительная информация
				//if(!isempty(pBRow->LotManuf)) {
				//	xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Line-Parties");
				//		xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Line-Party");
				//			// Производитель
				//			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_PARTYTYPE);
				//				xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_PARTYTYPE_MF); // Тип поля - производитель
				//			xmlTextWriterEndElement(P_XmlWriter); // PARTYTYPE
				//			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_NAME);
				//				SString str1;
				//				str1.Z().Cat(pBRow->LotManuf).ToUtf8();
				//				str.Z().Cat("<![CDATA[").Cat(str1).Cat("]]>");
				//				xmlTextWriterWriteString(P_XmlWriter, str.ucptr()); // Наименование производителя
				//			xmlTextWriterEndElement(P_XmlWriter); // NAME
				//		xmlTextWriterEndElement(P_XmlWriter); // LINEPARTY
				//	xmlTextWriterEndElement(P_XmlWriter); // LINEPARTIES
				//}
				if(pBRow->GoodKindCode || !isempty(pBRow->CLB) || !isempty(pBRow->Serial)) {
					xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Line-Reference");
						xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Reference-Elements");
							// Код вида продукции
							if(pBRow->GoodKindCode) {
								SXml::WNode n_ref(P_XmlWriter, "Reference-Element");
								{
									xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Reference-Type");
										xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_REFTYPE_YA1); // Тип поля - код вида продукции
									xmlTextWriterEndElement(P_XmlWriter); // REFTYPE
									xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Reference-Id");
										xmlTextWriterWriteString(P_XmlWriter, str.Z().Cat(pBRow->GoodKindCode).ucptr()); // Код вида продукции
									xmlTextWriterEndElement(P_XmlWriter); // REFID
								}
							}
							// ГТД
							if(!isempty(pBRow->CLB)) {
								SXml::WNode n_ref(P_XmlWriter, "Reference-Element");
								{
									xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Reference-Type");
										xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_REFTYPE_ABT); // Тип поля - ГТД
									xmlTextWriterEndElement(P_XmlWriter); // REFTYPE
									xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Reference-Id");
										xmlTextWriterWriteString(P_XmlWriter, str.Z().Cat(pBRow->CLB).ucptr()); // ГТД
									xmlTextWriterEndElement(P_XmlWriter); // REFID
								}
							}
							// Серия
							if(!isempty(pBRow->Serial)) {
								SXml::WNode n_ref(P_XmlWriter, "Reference-Element");
								{
									xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Reference-Type");
										xmlTextWriterWriteString(P_XmlWriter, (const xmlChar*)ELEMENT_CODE_REFTYPE_SER); // Тип поля - серия
									xmlTextWriterEndElement(P_XmlWriter); // REFTYPE
									xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"Reference-Id");
										xmlTextWriterWriteString(P_XmlWriter, str.Z().Cat(pBRow->Serial).ucptr()); // Серия
									xmlTextWriterEndElement(P_XmlWriter); // REFID
								}
							}
						xmlTextWriterEndElement(P_XmlWriter); // REFELEMENTS
					xmlTextWriterEndElement(P_XmlWriter); // LINEREFERENCE
				}
				// } @vmiller
			xmlTextWriterEndElement(P_XmlWriter); // LINE


	CATCH
		SysLogMessage(SYSLOG_GOODSLINES);
		ok = 0;
		if((MessageType == PPEDIOP_RECADV) && (!pBRow || isempty(pBRow->TTN))) // Иначе запишется окончание документа и программа попытается его отправить
			P_XmlWriter = 0;
	ENDCATCH;
	return ok;
}

// Завершаем формирование документа (заказа или документа о приемке товара)
int ExportCls::EndDoc()
{
	int    ok = 1;
	SString str;
	THROWERR(P_XmlWriter, IEERR_NULLWRIEXMLPTR);
		xmlTextWriterEndElement(P_XmlWriter); // ORDERLINES/RECADVPARTIES
		if(MessageType == PPEDIOP_ORDER)
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_ORDERSUMMARY);
		else if(MessageType == PPEDIOP_RECADV)
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"ReceivingAdvice-Summary");
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_TOTALLINES); // Общее количество товарных линий
				xmlTextWriterWriteString(P_XmlWriter, str.Z().Cat(Itr.GetCount()).ucptr());
			xmlTextWriterEndElement(P_XmlWriter); // TOTALLINES
			if(MessageType == PPEDIOP_ORDER)
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"TotalOrderedAmount"); // Общее количество заказанного товара
			else if(MessageType == PPEDIOP_RECADV)
				xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)"TotalGoodsReceiptAmount"); // Общее количество принятого товара
			xmlTextWriterWriteString(P_XmlWriter, str.Z().Printf("%.3f", TotalGoodsCount).ucptr()); // Число знаков после запятой должно быть не больше 3-х
			xmlTextWriterEndElement(P_XmlWriter); // TOTALORDEREDAMT/TOTALGOODSRECAMT

			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_TOTALNETAMOUNT); // Сумма всего заказа без НДС
				xmlTextWriterWriteString(P_XmlWriter, str.Z().Printf("%.3f", BillSumWithoutVat).ucptr()); // Число знаков после запятой должно быть не больше 3-х
			xmlTextWriterEndElement(P_XmlWriter); // TOTALNETAMOUNT
			xmlTextWriterStartElement(P_XmlWriter, (const xmlChar*)ELEMENT_NAME_TOTALGROSSAMOUNT); // Сумма всего заказа с НДС
				str.Z().Printf("%.3f", Bill.Amount); // Число знаков после запятой должно быть не больше 3-х
				xmlTextWriterWriteString(P_XmlWriter, str.ucptr());
			xmlTextWriterEndElement(P_XmlWriter); // TOTALGROSSAMOUNT
		xmlTextWriterEndElement(P_XmlWriter); // ORDERSUMMARY/RECADVSUMMARY
	xmlTextWriterEndElement(P_XmlWriter); // DOCORDERS/DOCRECADV

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
	Sdr_DllImpExpReceipt * p_exp_rcpt = 0;
	SString str, login;
	EDIServiceSoapProxy proxy(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxy, 0, 0);
	file.CalcSize(&file_size);

	// Ищем конфигурацию обмена для конкретного адресата и типа документа
	/*if(MessageType == PPEDIOP_ORDER) {
		for(pos = 0; pos < RlnCfgList.getCount(); pos++) {
			if(!RlnCfgList.at(pos).SuppGLN.CmpNC(Bill.GLN) &&
				!RlnCfgList.at(pos).Direction.CmpNC(WEB_ELEMENT_CODE_DIRECTION_OUT) &&
				!RlnCfgList.at(pos).DocType.CmpNC(WEB_ELEMENT_CODE_TYPE_ORDERS))
				break;
		}
	}
	else if(MessageType == PPEDIOP_RECADV) {
		for(pos = 0; pos < RlnCfgList.getCount(); pos++) {
			if(!RlnCfgList.at(pos).SuppGLN.CmpNC(Bill.GLN) &&
				!RlnCfgList.at(pos).Direction.CmpNC(WEB_ELEMENT_CODE_DIRECTION_OUT) &&
				!RlnCfgList.at(pos).DocType.CmpNC(WEB_ELEMENT_CODE_TYPE_RECADV))
				break;
		}
	}*/
	//if(pos < RlnCfgList.getCount()) { // @vmiller comment
		FormatLoginToLogin(Header.EdiLogin, login.Z()); // ИД пользователя
		param.Name = (char *)(const char *)login;
		//param.Name = Header.EdiLogin;			// ИД пользователя
		param.Password = Header.EdiPassword;	// Пароль
		param.PartnerIln = Bill.GLN;			// ИД партнера, которому посылается документ
		//param.DocumentType = (char *)(const char *)RlnCfgList.at(pos).DocType;	// Тип документа
		if(MessageType == PPEDIOP_ORDER) // Тип документа
			param.DocumentType = WEB_ELEMENT_CODE_TYPE_ORDERS;
		else if(MessageType == PPEDIOP_RECADV)
			param.DocumentType = WEB_ELEMENT_CODE_TYPE_RECADV;
		//param.DocumentVersion = (char *)(const char *)RlnCfgList.at(pos).DocVersion;	// Версия спецификации
		//param.DocumentStandard = (char *)(const char *)RlnCfgList.at(pos).DocStandard;	// Стандарт документа
		//param.DocumentTest = (char *)(const char *)RlnCfgList.at(pos).DocTest;			// Статус документа
		//param.ControlNumber = (char *)Bill.Code; // Контрольный номер документа (просто номер документа)
		buf = new char[(size_t)file_size + 1];
		memzero(buf, (size_t)file_size + 1);
		file.ReadLine(str.Z()); // Пропускаем первую строку <?xml version="1.0" encoding="UTF-8" ?>
		file.Seek((long)str.Len());
		file.ReadV(buf, (size_t)file_size + 1);
		param.DocumentContent = buf; // Содержание документа
		param.Timeout = 10000;		// Таймаут на выполнение вызова метода (мс) (значение рекомендовано)
		if((r = proxy.Send(&param, &resp)) == SOAP_OK) {
			if(atoi(resp.SendResult->Res) == 0) {
				p_exp_rcpt = new Sdr_DllImpExpReceipt;
				memzero(p_exp_rcpt, sizeof(Sdr_DllImpExpReceipt));
				p_exp_rcpt->ID = atol(Bill.ID);
				STRNSCPY(p_exp_rcpt->ReceiptNumber, resp.SendResult->Cnt); // ИД документа в системе провайдера
				ReceiptList.insert(p_exp_rcpt);
			}
			else {
				SetError(IEERR_WEBSERVСERR);
				SetWebServcError(atoi((const char *)resp.SendResult->Res), (const char *)resp.SendResult->Cnt);
				ok = 0;
			}
		}
		else {
			ProcessError(proxy);
			ok = 0;
		}
	// @vmiller comment
	/*}
	else {
		str.Z().Cat(Bill.GLN).CR().Cat("Документ ").Cat(Bill.Code);
		SetError(IEERR_NOCFGFORGLN, str);
		ok = -1;
	}*/
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
	SFile  log_file;
	SPathStruc log_path;
	SString str;
	SETIFZ(P_ExportCls, new ExportCls);
	if(P_ExportCls && !P_ExportCls->Inited) {
		if(pExpHeader)
			P_ExportCls->Header = *(Sdr_ImpExpHeader*)pExpHeader;
		if(!isempty(pOutFileName)) {
			P_ExportCls->PathStruct.Split(pOutFileName);
			P_ExportCls->PathStruct.Nam.SetIfEmpty("edisoft_export_");
			P_ExportCls->PathStruct.Ext.SetIfEmpty("xml");
		}
		else {
			SLS.Init("Papyrus");
			str = SLS.GetExePath();
			P_ExportCls->PathStruct.Split(str);
			P_ExportCls->PathStruct.Dir.ReplaceStr("\\bin", "\\out", 1);
			P_ExportCls->PathStruct.Nam = "edisoft_export_";
			P_ExportCls->PathStruct.Ext = "xml";
		}
		log_path.Copy(&P_ExportCls->PathStruct, SPathStruc::fDrv | SPathStruc::fDir | SPathStruc::fNam | SPathStruc::fExt);
		log_path.Nam = "export_log";
		log_path.Ext = "txt";
		log_path.Merge(LogName);
		log_path.Nam = "system_log";
		log_path.Merge(SysLogName);
		P_ExportCls->Id = 1;
		*pId = P_ExportCls->Id; // ИД сеанса экспорта
		P_ExportCls->Inited = 1;
	}
	THROWERR(P_ExportCls, IEERR_IMPEXPCLSNOTINTD);
	CATCH
		SysLogMessage(SYSLOG_INITEXPORT);
		GetErrorMsg(str.Z());
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
	//THROWERR(GetMsgTypeBySymb(pMsgType, P_ExportCls->MessageType), IEERR_MSGSYMBNOTFOUND);
	THROWERR_STR(GetMsgTypeBySymb(pMsgType, P_ExportCls->MessageType), IEERR_MSGSYMBNOTFOUND, pMsgType);
	THROWERR(oneof2(P_ExportCls->MessageType, PPEDIOP_ORDER, PPEDIOP_RECADV), IEERR_ONLYEXPMSGTYPES);
	THROWERR(GetObjTypeBySymb(pObjTypeSymb, P_ExportCls->ObjType), IEERR_SYMBNOTFOUND);
	THROWERR(P_ExportCls->ObjType == objBill, IEERR_ONLYBILLS);
	// Завершаем предыдущий документ
	if(P_ExportCls->P_XmlWriter) {
		THROW(P_ExportCls->EndDoc());
		xmlTextWriterEndDocument(P_ExportCls->P_XmlWriter);
		xmlFreeTextWriter(P_ExportCls->P_XmlWriter);
		P_ExportCls->P_XmlWriter = 0;
		// Отправляем файл, сформированный в прошлый раз. Имя файла и номер документа еще не успели принять новые значения
		//THROW(P_ExportCls->SendDoc());
		// Делаем без THROW, чтобы отправка остальных документов не прерывалась, но ошибку все равно запомним
		if(!P_ExportCls->SendDoc()) {
			SysLogMessage(SYSLOG_SETEXPORTOBJ);
			GetErrorMsg(str.Z());
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
	if(P_ExportCls->MessageType == PPEDIOP_ORDER) {
		THROW(P_ExportCls->OrderHeader())
		THROW(P_ExportCls->DocParties());
	}
	// @vmiller comment {
	// Перенесу эту часть в NextExportObjIter() из-за того, что поле ТТН содержится в структуре строк документов, а не самих
	// документов, а в RECADV этот параметр указывается в заголовке.
	//else if(P_ExportCls->MessageType == PPEDIOP_RECADV) {
	//	THROW(P_ExportCls->RecadvHeader());
	//	THROW(P_ExportCls->DocParties());
	// } @vmiller comment
	CATCH
		SysLogMessage(SYSLOG_SETEXPORTOBJ);
		GetErrorMsg(str.Z());
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
		GetErrorMsg(str.Z());
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
		GetErrorMsg(str.Z());
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
			// Делаем без THROW, чтобы у отправленных документов проставились тэги, но ошибку запомним
			if(!P_ExportCls->SendDoc()) {
				SysLogMessage(SYSLOG_ENUMEXPRECEIPT);
				GetErrorMsg(str.Z());
				SysLogMessage(str);
				LogMessage(str);
				ok = 0;
			}
		}
		// Считываем результаты отправки документов
		if(P_ExportCls->ReadReceiptNum < P_ExportCls->ReceiptList.getCount()) {
			ASSIGN_PTR((Sdr_DllImpExpReceipt *)pReceipt, *P_ExportCls->ReceiptList.at(P_ExportCls->ReadReceiptNum++));
			ok = 1;
		}
	}
	CATCH
		SysLogMessage(SYSLOG_ENUMEXPRECEIPT);
		GetErrorMsg(str.Z());
		SysLogMessage(str);
		LogMessage(str);
		ok = 0;
	ENDCATCH;
	return ok;
}

//
//
// Импорт
//
//
struct AperakInfoSt {
	AperakInfoSt()
	{
		Clear();
	}
	void Clear()
	{
		DocID.Z();
		OrderNum.Z();
		Code.Z();
		Msg.Z();
		SupplGLN.Z();
		BuyerGLN.Z();
		AddrGLN.Z();
		OrderDate = ZERODATE;
	}
	SString DocID;      // ИД документа статуса (чтобы поставить ему стоатус read)
	SString OrderNum;     // Номер документа заказа
	SString Code;		// Код статуса (в случае ошибки - 27)
	SString Msg;        // Описание ошибки или статуса
	SString SupplGLN;   // GLN поставщика
	SString BuyerGLN;   // GLN покупателя
	SString AddrGLN;    // GLN адреса доставки
	LDATE	OrderDate;    // Дата документа
};

class ImportCls : public ImportExportCls {
public:
	ImportCls();
	~ImportCls();
	void CreateFileName(uint num);
	int ReceiveDoc();
	int ListMessageBox(SString & rResp);
	int ParseForDocData(Sdr_Bill * pBill);
	int ParseForGoodsData(Sdr_BRow * pBRow);
	int ParseAperakResp(const char * pResp);
	int SetReadStatus(SString & trackID);
	int SetNewStatus(SString & rErrTrackIdList);
	int GoodsCount;				// Число товаров в документе
	uint Id;					// ИД, который Papyrus будет воспринимать как ИД сеанса экспорта
	uint ObjId;					// ИД импортируемого объекта (например, емли это пакет документов, то у каждого документа свой ИД)
	uint ObjType;				// Тип импортируемых объектов
	uint MessageType;			// Тип сообщеия: ORDRESP, APERAK
	uint Inited;
	uint IncomMessagesCounter;	// Счетчик входящих сообщений. Нужен в случае, если есть входящие сообщения, но они предназначены для других магазинов.
								// В этом случае ReceiveDoc() не выдаст сообщение LOG_NOINCOMDOC
	double BillSumWithoutVat;	// В Sdr_Bill этот праметр не передается, поэтому передача этого параметра Papyrus'у под вопросом
	SString ImpFileName;
	SString LastTrackId;		// GUID последнего прочитанного документа в системе EDI
	Iterator Itr;
	SPathStruc PathStruct;
	AperakInfoSt AperakInfo;
	StrAssocArray TrackIds;		// Массив GUID прочитанных документов в системе EDI, статусы которых надо поменять (то есть предназначены для другого магазина)
private:
	int ParseListMBResp(const char * pResp, SString & rPartnerIln, SString & rDocId);

	int HasSellerCertificate;  // Чтобы при чтении товарных строк не считывать каждый раз инфу о лицензии на продажу алкоголя для поставщика.
							   // Заполняется при чтении товарных строк, потому что структура, которую надо заполнять, находится в Sdr_BRow
};

ImportCls::ImportCls() : GoodsCount(0), Id(0), ObjId(0), ObjType(0), MessageType(0), Inited(0), IncomMessagesCounter(0), BillSumWithoutVat(0.0)
{
	ErrorCode = 0;
	WebServcErrorCode = 0;
	HasSellerCertificate = 0;
}

ImportCls::~ImportCls()
{
}

void ImportCls::CreateFileName(uint num)
{
	ImpFileName.Z().Cat(PathStruct.Drv).CatChar(':').Cat(PathStruct.Dir).Cat(PathStruct.Nam).Cat(num).Dot().Cat(PathStruct.Ext);
}

EXPORT int InitImport(void * pImpHeader, const char * pInputFileName, int * pId)
{
	int    ok = 1;
	SPathStruc log_path;
	SString str;
	SETIFZ(P_ImportCls, new ImportCls);
	if(P_ImportCls && !P_ImportCls->Inited) {
		if(pImpHeader)
			P_ImportCls->Header = *(Sdr_ImpExpHeader*)pImpHeader;
		if(!isempty(pInputFileName)) {
			P_ImportCls->PathStruct.Split(pInputFileName);
			if(P_ImportCls->PathStruct.Nam.Empty())
				(P_ImportCls->PathStruct.Nam = "edisoft_import_").Cat(P_ImportCls->ObjId);
			P_ImportCls->PathStruct.Ext.SetIfEmpty("xml");
		}
		else {
			char fname[256];
			GetModuleFileName(NULL, fname, sizeof(fname));
			P_ImportCls->PathStruct.Split(fname);
			P_ImportCls->PathStruct.Dir.ReplaceStr("\\bin", "\\in", 1);
			(P_ImportCls->PathStruct.Nam = "edisoft_import_").Cat(P_ImportCls->ObjId);
			P_ImportCls->PathStruct.Ext = "xml";
		}
		log_path.Copy(&P_ImportCls->PathStruct, SPathStruc::fDrv | SPathStruc::fDir | SPathStruc::fNam | SPathStruc::fExt);
		log_path.Nam = "import_log";
		log_path.Ext = "txt";
		log_path.Merge(LogName);
		log_path.Nam = "system_log";
		log_path.Merge(SysLogName);
		P_ImportCls->Id = 1;
		*pId = P_ImportCls->Id; // ИД сеанса импорта
		P_ImportCls->Inited = 1;
	}
	THROWERR(P_ImportCls, IEERR_IMPEXPCLSNOTINTD);
	CATCH
		SysLogMessage(SYSLOG_INITIMPORT);
		GetErrorMsg(str.Z());
		SysLogMessage(str);
		LogMessage(str);
		ok =0;
	ENDCATCH;
	return ok;
}

EXPORT int GetImportObj(uint idSess, const char * pObjTypeSymb, void * pObjData, int * pObjId, const char * pMsgType = 0)
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
	// Смотрим тип сообщения
	THROWERR(pMsgType, IEERR_MSGSYMBNOTFOUND);
	THROWERR(GetMsgTypeBySymb(pMsgType, P_ImportCls->MessageType), IEERR_MSGSYMBNOTFOUND);
	THROWERR(oneof3(P_ImportCls->MessageType, PPEDIOP_ORDERRSP, PPEDIOP_APERAK, PPEDIOP_DESADV), IEERR_ONLYIMPMSGTYPES);
	//
	// Получаем документ
	//
	P_ImportCls->CreateFileName(P_ImportCls->ObjId);
	THROW(r = P_ImportCls->ReceiveDoc());
	if(r == -1)
		ok = -1;
	else {
		if(((P_ImportCls->MessageType == PPEDIOP_ORDERRSP) || (P_ImportCls->MessageType == PPEDIOP_DESADV)) && (r == 1)) // Ибо если r == -2, то нам не надо пытаться разбирать файл, ибо его нет
			// Читаем документ и заполняем Sdr_Bill
			THROW(P_ImportCls->ParseForDocData((Sdr_Bill *)pObjData))
		else if(P_ImportCls->MessageType == PPEDIOP_APERAK) {
			// Здесь заполняем pObjData из структуры P_ImportCls->AperakInfo
			// Для пробы запишем номер документа и GLN адреса доставки, дату документа. Этого достаточно для определения принадлежности статуса конкретному магазину
			// GLN покупателя
			P_ImportCls->AperakInfo.OrderNum.CopyTo(((Sdr_Bill *)pObjData)->OrderBillNo, sizeof(((Sdr_Bill *)pObjData)->OrderBillNo));
			P_ImportCls->AperakInfo.AddrGLN.CopyTo(((Sdr_Bill *)pObjData)->DlvrAddrCode, sizeof((Sdr_Bill *)pObjData)->DlvrAddrCode);
			P_ImportCls->AperakInfo.AddrGLN.CopyTo(((Sdr_Bill *)pObjData)->LocCode, sizeof((Sdr_Bill *)pObjData)->LocCode);
			P_ImportCls->AperakInfo.BuyerGLN.CopyTo(((Sdr_Bill *)pObjData)->MainGLN, sizeof((Sdr_Bill *)pObjData)->MainGLN);
			P_ImportCls->AperakInfo.SupplGLN.CopyTo(((Sdr_Bill *)pObjData)->GLN, sizeof((Sdr_Bill *)pObjData)->GLN);
			((Sdr_Bill *)pObjData)->OrderDate = P_ImportCls->AperakInfo.OrderDate;
		}
	}
	CATCH
		SysLogMessage(SYSLOG_GETIMPORTOBJ);
		GetErrorMsg(str.Z());
		LogMessage(str);
		SysLogMessage(str);
		ok = 0;
	ENDCATCH;
	return ok;
}

EXPORT int InitImportObjIter(uint idSess, uint objId)
{
	int ok = 1;
	SString str;

	THROWERR(P_ImportCls, IEERR_IMPEXPCLSNOTINTD);
	THROWERR(idSess == P_ImportCls->Id, IEERR_NOSESS);
	THROWERR(objId == P_ImportCls->ObjId, IEERR_NOOBJID);
	P_ImportCls->Itr.Init();

	CATCH
		SysLogMessage(SYSLOG_INITIMPORTOBJITER);
		GetErrorMsg(str.Z());
		LogMessage(str);
		SysLogMessage(str);
		ok = 0;
	ENDCATCH;
	return ok;
}

// Returns:
//		-1 - считаны все товарные строки. Однако, если в документе в поле общего количество написано число,
//			меньшее реального числа товарных позиций, то лишние позиции не считаются
//		 0 - ошибка
//		 1 - считана очередная товарная строка
EXPORT int NextImportObjIter(uint idSess, uint objId, void * pRow)
{
	int ok = 1, r = 0;
	SString str;
	THROWERR(P_ImportCls, IEERR_IMPEXPCLSNOTINTD);
	THROWERR(pRow, IEERR_NODATA);
	THROWERR(idSess == P_ImportCls->Id, IEERR_NOSESS);
	THROWERR(objId == P_ImportCls->ObjId, IEERR_NOOBJID);
	// Разбираем записанный ранее документ и заполненяем Sdr_BRow
	THROW(r = P_ImportCls->ParseForGoodsData((Sdr_BRow *)pRow));
	if(r == -1)
		ok = -1;
	P_ImportCls->Itr.Next();
	CATCH
		SysLogMessage(SYSLOG_NEXTIMPORTOBJITER);
		GetErrorMsg(str.Z());
		LogMessage(str);
		SysLogMessage(str);
		ok = 0;
	ENDCATCH;
	return ok;
}

// Вызывается Papyrus'ом после каждого GetImportObj. Возможность Papyrus'а сказать свое слово при импорте
EXPORT int ReplyImportObjStatus(uint idSess, uint objId, void * pObjStatus)
{
	int ok = 1;
	size_t pos = 0;
	SString str;
	// Если в Papyrus есть заказ, на который получено подтверждение или статус
	if(((Sdr_DllImpObjStatus *)pObjStatus)->DocStatus == docStatIsSuchDoc) {
		if(P_ImportCls) {
			if(P_ImportCls->MessageType == PPEDIOP_APERAK) {
				// Что-нибудь делаем с этим статусом
				str.Z().Cat(P_ImportCls->AperakInfo.OrderNum.Transf(CTRANSF_INNER_TO_OUTER)).CatDiv(':', 2).Cat(P_ImportCls->AperakInfo.Msg.Utf8ToChar());
				LogMessage(str);
			}
			// Иначе ничего не делаем
		}
	}
	// Если в Papyrus такого заказа нет
	else if(((Sdr_DllImpObjStatus *)pObjStatus)->DocStatus == docStatNoSuchDoc) {
		if(P_ImportCls) {
			P_ImportCls->IncomMessagesCounter--;
			if((P_ImportCls->MessageType == PPEDIOP_ORDERRSP) || (P_ImportCls->MessageType == PPEDIOP_DESADV)) {
				// Удаляем созданный ранее xml-документ
				SFile file;
				file.Remove(P_ImportCls->ImpFileName);
			}
			// И при подтверждени, и при статусе ставим в ящике сообщений статус у документа/статуса "Новый", чтобы те, кому он предназначен, смогли его получить.
			pos = P_ImportCls->TrackIds.getCount();
			P_ImportCls->TrackIds.Add(pos, P_ImportCls->LastTrackId);
		}
	}
	return ok;
}

// Descr: Получает документ. В случае успеха, записывает его в файл.
//
// Returns:
//		-1 - нет входящих сообщений
//		 0 - ошибка
//		 1 - сообщение получено
int ImportCls::ReceiveDoc()
{
	int ok = 1, r = 0;
	uint pos = 0;
	_ns1__Receive param;
	_ns1__ReceiveResponse resp;
	SString listmb, partner_iln, track_id, str, login;
	SFile file;

	EDIServiceSoapProxy proxy(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxy, 0, 0);

	AperakInfo.Clear(); // Это необходимо здесь из-за "косячного" сообщения (см. ниже), чтобы при его получении
						// в Папирус не паредавалась инфа от предыдущего полученного нормального сообщения

	// Проверяем наличие входящих сообщений
	THROW(r = ListMessageBox(listmb));
	if(r == 1) // Если есть, что разбирать, то разбираем
		THROW(r = ParseListMBResp(listmb, partner_iln, track_id));
	if(r == 1) {
		// Ищем конфигурацию импорта для конкретного отправителя и типа операции
		//if(MessageType == PPEDIOP_ORDERRSP) {
		//	for(pos = 0; pos < RlnCfgList.getCount(); pos++) {
		//		if(!RlnCfgList.at(pos).SuppGLN.CmpNC(partner_iln) &&
		//			!RlnCfgList.at(pos).DocType.CmpNC(WEB_ELEMENT_CODE_TYPE_ORDRSP) &&
		//			!RlnCfgList.at(pos).Direction.CmpNC(WEB_ELEMENT_CODE_DIRECTION_IN))
		//			break;
		//	}
		//}
		//else if(MessageType == PPEDIOP_DESADV) {
		//	for(pos = 0; pos < RlnCfgList.getCount(); pos++) {
		//		if(!RlnCfgList.at(pos).SuppGLN.CmpNC(partner_iln) &&
		//			!RlnCfgList.at(pos).DocType.CmpNC(WEB_ELEMENT_CODE_TYPE_DESADV) &&
		//			!RlnCfgList.at(pos).Direction.CmpNC(WEB_ELEMENT_CODE_DIRECTION_IN))
		//			break;
		//	}
		//}
		//else if(MessageType == PPEDIOP_APERAK) {
		//	for(pos = 0; pos < RlnCfgList.getCount(); pos++) {
		//		if(!RlnCfgList.at(pos).SuppGLN.CmpNC(partner_iln) &&
		//			!RlnCfgList.at(pos).DocType.CmpNC(WEB_ELEMENT_CODE_TYPE_APERAK)/* &&
		//			!RlnCfgList.at(pos).Direction.CmpNC(WEB_ELEMENT_CODE_DIRECTION_IN)*/)
		//			break;
		//	}
		//}
		//if(pos < RlnCfgList.getCount()) { // @vmiller comment
			FormatLoginToLogin(Header.EdiLogin, login.Z()); // ИД пользователя
			param.Name = (char *)(const char *)login;
			//param.Name = Header.EdiLogin;			// ИД пользователя в системе
			param.Password = Header.EdiPassword;	// Пароль
			param.PartnerIln = (char *)(const char *)/*RlnCfgList.at(pos).SuppGLN*/partner_iln;	// ИД партнера, которому был послан документ (при ORDRSP и DESADV это будет GLN поставщика,
												// а при APERAK это будет ??? - служебный GLN сервиса, с которого отправляются сообщения)
			//param.DocumentType = (char *)(const char *)RlnCfgList.at(pos).DocType;	// Тип документа
			if(MessageType == PPEDIOP_ORDERRSP)
				param.DocumentType = WEB_ELEMENT_CODE_TYPE_ORDRSP;
			else if(MessageType == PPEDIOP_DESADV)
				param.DocumentType = WEB_ELEMENT_CODE_TYPE_DESADV;
			else if(MessageType == PPEDIOP_APERAK)
				param.DocumentType = WEB_ELEMENT_CODE_TYPE_APERAK;
			param.TrackingId = (char *)(const char *)track_id; // ИД документа в системе
			//param.DocumentStandard = (char *)(const char *)RlnCfgList.at(pos).DocStandard;	// Стандарт документа
			param.Timeout = 10000;		// Таймаут на выполнение вызова метода (мс) (значение рекомендовано)
			LastTrackId = track_id;

			if(proxy.Receive(&param, &resp) == SOAP_OK) {
				// При получении Aperak иногда resp.ReceiveResult не заполняется и программа умирает.
				// Обычно это происходит, если при получении сообщения сервер был недоступен.
				// Тогда при повторном получении сообщение вроде как дублируется. Вот дубляж и убивает все.
				// Дабы этого не было, сделаем проверку на ноль и поставим этому сообщению статус Read
				if(resp.ReceiveResult) {
					if(atoi(resp.ReceiveResult->Res) == 0) {
						if((MessageType == PPEDIOP_ORDERRSP) || (MessageType == PPEDIOP_DESADV)) {
							// Для простоты обработки полученного документа удалим все переносы строк
							(str = resp.ReceiveResult->Cnt).ReplaceCR();
							file.Open(ImpFileName, SFile::mWrite);
							file.WriteLine(/*resp.ReceiveResult->Cnt*/str);
							IncomMessagesCounter++;
						}
						else if(MessageType == PPEDIOP_APERAK) {
							// Для простоты обработки полученного документа удалим все переносы строк
							(str = resp.ReceiveResult->Cnt).ReplaceCR().ReplaceStr("\n", "", 0);
							// Сразу разберем ответ
							file.Open(ImpFileName, SFile::mWrite);
							//file.Open("D:\\Papyrus\\ppy\\OUT\\Edisoft\\aperak.xml", SFile::mWrite);
							file.WriteLine(/*resp.ReceiveResult->Cnt*/str);
							file.Close();
							THROW(ParseAperakResp(resp.ReceiveResult->Cnt));
							IncomMessagesCounter++;
						}
						// Ставим статус документа - прочитан
						SetReadStatus(track_id);
					}
					else {
						SetError(IEERR_WEBSERVСERR);
						SetWebServcError(atoi((const char *)resp.ReceiveResult->Res));
						ok = 0;
					}
				}
				else {
					// Избавимся от "косячного" сообщения
					SetReadStatus(track_id);
					r = -2; // Надо же как-то отметить сие событие
				}
			} else {
				ProcessError(proxy);
				ok = 0;
			}
		// @vmiller comment
		/*}
		else {
			str.Z().Cat(partner_iln);
			SetError(IEERR_NOCFGFORGLN, str);
			ok = -1;
		}*/
	}
	else if(r == -1) {
		// За сообщение "нет входящих сообщений" отвечает параметр IncomMessagesCounter и функция FinishImpExp
		ok = -1;
	}

	CATCHZOK;
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
	SString login;
	char   track_id_buf[256];
	_ns1__ChangeDocumentStatus param;
	_ns1__ChangeDocumentStatusResponse resp;
	EDIServiceSoapProxy proxy(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxy, 0, 0);
	rErrTrackIdList.Z();
	for(size_t pos = 0; pos < TrackIds.getCount(); pos++) {
		STRNSCPY(track_id_buf, TrackIds.Get(pos).Txt);
		FormatLoginToLogin(Header.EdiLogin, login.Z()); // ИД пользователя
		param.Name = (char *)(const char *)login; // @badcast
		//param.Name = Header.EdiLogin;			// ИД пользователя в системе
		param.Password = Header.EdiPassword;	// Пароль
		param.TrackingId = track_id_buf; // ИД документа в системе
		param.Status = "N"; // Новый статус документа (new)
		if(proxy.ChangeDocumentStatus(&param, &resp) == SOAP_OK) {
			if(atoi(resp.ChangeDocumentStatusResult->Res) != 0) {
				SetError(IEERR_WEBSERVСERR);
				SetWebServcError(atoi(resp.ChangeDocumentStatusResult->Res));
				if(rErrTrackIdList.Empty())
					rErrTrackIdList = track_id_buf;
				else
					rErrTrackIdList.Comma().Cat(track_id_buf);
				ok = 0;
			}
		}
		else {
			ProcessError(proxy);
			if(rErrTrackIdList.Empty())
				rErrTrackIdList = track_id_buf;
			else
				rErrTrackIdList.Comma().Cat(track_id_buf);
			ok = 0;
		}
	}
	if(!ok)
		SysLogMessage(SYSLOG_SETNEWSTATUS);
	return ok;
}
//
// Descr: Ставит статус прочитанного документа в Read
//
int ImportCls::SetReadStatus(SString & trackID)
{
	int    ok = 1;
	SString str, login;
	_ns1__ChangeDocumentStatus param;
	_ns1__ChangeDocumentStatusResponse resp;
	EDIServiceSoapProxy proxy(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxy, 0, 0);
	FormatLoginToLogin(Header.EdiLogin, login.Z()); // ИД пользователя
	param.Name = (char *)(const char *)login;
	//param.Name = Header.EdiLogin;			// ИД пользователя в системе
	param.Password = Header.EdiPassword;	// Пароль
	param.TrackingId = (char *)(const char *)trackID; // ИД документа в системе
	param.Status = "R"; // Новый статус документа (read)
	if(proxy.ChangeDocumentStatus(&param, &resp) == SOAP_OK) {
		if(atoi(resp.ChangeDocumentStatusResult->Res) != 0) {
			SetError(IEERR_WEBSERVСERR);
			SetWebServcError(atoi((const char *)resp.ChangeDocumentStatusResult->Res));
			ok = 0;
		}
	}
	else
		ok = 0;
	if(!ok) {
		SysLogMessage(SYSLOG_SETREADSTATUS);
		(str = LOG_READSTATERR).Cat(trackID);
		SysLogMessage(str);
		LogMessage(str);
	}
	return ok;
}
//
// Returns:
//		-1 - нет сообщений
//		 0 - ошибка
//		 1 - есть сообщение
//
int ImportCls::ListMessageBox(SString & rResp)
{
	int    ok = 0;
	uint   pos = 0;
	SString fmt, low, upp, str, login;
	_ns1__ListMBEx param;
	_ns1__ListMBExResponse resp;
	EDIServiceSoapProxy proxy(SOAP_XML_INDENT|SOAP_XML_IGNORENS);
	gSoapClientInit(&proxy, 0, 0);
	//
	// Логика получения списка входящих документов/статусов такая.
	// 1. Смотрим первую конфигурацию для входящих документов. Соответественно, это будет первый постащик.
	// 2. Проверяем, есть ли от него входящие сообщения.
	// 3.1. Если есть, запоминаем нужную инфу о документе и выходим из цикла с положительным результатом
	// 3.2. Если входящих сообщений не оказалось, то смотрим следующую конфигурацию, то есть следующего поставщика
	// 4. Если ящик оказался пуст при переборе всех поставщиков, то выходим из цикла с нулевым результатом
	//
	//for(pos = 0; pos < RlnCfgList.getCount(); pos ++) {

	//
	// Другая логика.
	// 1. Смотрим входящие сообщения для текущего типа
	// 2. Если ящик оказался пуст при переборе всех типов сообщений, то выходим с -1

		//if((((!RlnCfgList.at(pos).DocType.CmpNC(WEB_ELEMENT_CODE_TYPE_ORDRSP) && (MessageType == PPEDIOP_ORDERRSP)) || // Если ORDRSP
		//	(!RlnCfgList.at(pos).DocType.CmpNC(WEB_ELEMENT_CODE_TYPE_DESADV) && (MessageType == PPEDIOP_DESADV))) && // Если DESADV
		//	!RlnCfgList.at(pos).Direction.CmpNC(WEB_ELEMENT_CODE_DIRECTION_IN)) ||
		//	(!RlnCfgList.at(pos).DocType.CmpNC(WEB_ELEMENT_CODE_TYPE_APERAK) && (MessageType == PPEDIOP_APERAK))) { // Если APERAK
			FormatLoginToLogin(Header.EdiLogin, login.Z()); // ИД пользователя
			param.Name = (char *)(const char *)login;
			//param.Name = Header.EdiLogin;			// ИД пользователя
			param.Password = Header.EdiPassword;	// Пароль пользователя
		//	param.PartnerIln = (char *)(const char *)RlnCfgList.at(pos).SuppGLN;	// ИД партнера, от которого был получен документ
		//	param.DocumentType = (char *)(const char *)RlnCfgList.at(pos).DocType;	// Тип документа
			if(MessageType == PPEDIOP_ORDERRSP)
				param.DocumentType = WEB_ELEMENT_CODE_TYPE_ORDRSP;
			else if(MessageType == PPEDIOP_DESADV)
				param.DocumentType = WEB_ELEMENT_CODE_TYPE_DESADV;
			else if(MessageType == PPEDIOP_APERAK)
				param.DocumentType = WEB_ELEMENT_CODE_TYPE_APERAK;
		//	param.DocumentVersion = (char *)(const char *)RlnCfgList.at(pos).DocVersion;	// Версия спецификации
		//	param.DocumentStandard = (char *)(const char *)RlnCfgList.at(pos).DocStandard;	// Стандарт документа
		//	param.DocumentTest = (char *)(const char *)RlnCfgList.at(pos).DocTest;		// Статус документа
			param.DocumentStatus = "N";	// Статус выбираемых документов (подтверждений, статусов) (только новые)
			param.Timeout = 10000;		// Таймаут на выполнение вызова метода (мс) (значение рекомендовано)
			if((Header.PeriodLow != ZERODATE) && (Header.PeriodUpp != ZERODATE)) {
				low.Z().Cat(Header.PeriodLow.year());
				if(fmt.Z().Cat(Header.PeriodLow.month()).Len() == 1)
					fmt.PadLeft(1, '0');
				low.CatChar('-').Cat(fmt);
				if(fmt.Z().Cat(Header.PeriodLow.day()).Len() == 1)
					fmt.PadLeft(1, '0');
				low.CatChar('-').Cat(fmt);
				upp.Z().Cat(Header.PeriodUpp.year());
				if(fmt.Z().Cat(Header.PeriodUpp.month()).Len() == 1)
					fmt.PadLeft(1, '0');
				upp.CatChar('-').Cat(fmt);
				if(fmt.Z().Cat(Header.PeriodUpp.day()).Len() == 1)
					fmt.PadLeft(1, '0');
				upp.CatChar('-').Cat(fmt);
				param.DateFrom = (char *)(const char *)low;
				param.DateTo = (char *)(const char *)upp;
			}
			if(proxy.ListMBEx(&param, &resp) == SOAP_OK) {
				if(atoi(resp.ListMBExResult->Res) == 0) {
					int len = strlen(resp.ListMBExResult->Cnt);
					if(strncmp(resp.ListMBExResult->Cnt, EMPTY_LISTMB_RESP, strlen(resp.ListMBExResult->Cnt)) != 0) {
						rResp.CopyFrom(resp.ListMBExResult->Cnt);
						ok = 1;
					}
					else ok = -1;
				}
				else {
					SetError(IEERR_WEBSERVСERR);
					SetWebServcError(atoi((const char *)resp.ListMBExResult->Res));
				}
			}
			else {
				ProcessError(proxy);
			}
		//}

	//if(!ok) {
	//	SetError(IEERR_NOCONFIG);
	//	SysLogMessage(SYSLOG_LISTMESSAGEBOX);
	//}
	return ok;
}
//
// Разбор результата, полученного с помощью ListMB()
// Собственно, нужно вытащить "ИД документа в системе" и "ID партнера, от которого был получен документ"
// Retruns:
//		-1 - не было получено входящих сообщений
//		 0 - ошибка
//		 1 - оба параметра найдены
//
int ImportCls::ParseListMBResp(const char * pResp, SString & rPartnerIln, SString & rDocId)
{
	int    ok = -1, found = 0;
	SString str;
	xmlTextReader * p_xml_ptr;
	xmlNode * p_node;
	xmlParserInputBuffer * p_input = xmlParserInputBufferCreateMem(pResp, sstrlen(pResp), XML_CHAR_ENCODING_NONE);
	THROWERR((p_xml_ptr = xmlNewTextReader(p_input, NULL)), IEERR_NULLREADXMLPTR);
	while(xmlTextReaderRead(p_xml_ptr) && (found != 2)) {
		p_node = xmlTextReaderCurrentNode(p_xml_ptr);
		if(p_node && sstreq(p_node->name, WEB_ELEMENT_NAME_TRACKID) && p_node->children) {
			rDocId = (const char *)p_node->children->content;
			found++;
		}
		else if(p_node && sstreq(p_node->name, WEB_ELEMENT_NAME_PARTNER_ILN) && p_node->children) {
			rPartnerIln = (const char *)p_node->children->content;
			found++;
		}
	}
	if(found == 2)
		ok = 1;
	CATCH
		SysLogMessage(SYSLOG_PARSELISTMBRESP);
		ok = 0;
	ENDCATCH;
	xmlFreeParserInputBuffer(p_input);
	xmlFreeTextReader(p_xml_ptr);
	return ok;
}
//
// Для ORDRESP и DESADV
// Разбираем полученный документ и заполняем Sdr_Bill
//
// Примечание: Sender = Seller
//             Reciever = Buyer
//
int ImportCls::ParseForDocData(Sdr_Bill * pBill)
{
	int    ok = 0, exit_while = 0, is_correct = 0;
	SString str;
	xmlDoc * p_doc = 0;
    HasSellerCertificate = 0;
	THROWERR_STR(fileExists(ImpFileName), IEERR_IMPFILENOTFOUND, ImpFileName);
	THROWERR(pBill, IEERR_NODATA);
	memzero(pBill, sizeof(Sdr_Bill));
	THROWERR((p_doc = xmlReadFile(ImpFileName, NULL, XML_PARSE_NOENT)), IEERR_NULLREADXMLPTR);
	xmlNode * p_node = xmlDocGetRootElement(p_doc);
	THROWERR(p_node, IEERR_XMLREAD);
	// @v8.5.6 {
	STRNSCPY(pBill->EdiOpSymb, p_node->name);
	pBill->EdiOp = MessageType;
	// } @v8.5.6
	if(((MessageType == PPEDIOP_ORDERRSP && SXml::IsName(p_node, "Document-OrderResponse")) || // По первому тэгу можно понять, что это за тип
		(MessageType == PPEDIOP_DESADV && SXml::IsName(p_node, "Document-DespatchAdvice"))) && p_node->children)
		is_correct = 1;
	while(p_node && (p_node->type == XML_ELEMENT_NODE)) {
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
		if(p_node && p_node->type == XML_ELEMENT_NODE) {
			if(((MessageType == PPEDIOP_ORDER && SXml::IsName(p_node, "Document-OrderResponse")) ||
				(MessageType == PPEDIOP_DESADV && SXml::IsName(p_node, "Document-DespatchAdvice") == 0)) && p_node->children) {
				is_correct = 1; // Проверили, тип документа, указанный в конфигурации импорта, соответствует читаемому документу
				p_node = p_node->children;
			}
			if(is_correct) {
				//
				// Эти параметры только для ORDRSP
				//
				if(MessageType == PPEDIOP_ORDERRSP) {
					if(SXml::IsName(p_node, ELEMENT_NAME_ORDRSPNUMBER) && p_node->children) {
						// Номер документа ответа
						str.Set(p_node->children->content).Utf8ToOem(); // @vmiller @TODO tooem -> tochar
						STRNSCPY(pBill->Code, str);
						ok = 1;
					}
					else if(SXml::IsName(p_node, ELEMENT_NAME_ORDRSPDATE) && p_node->children) {
						// Дата документа ответа
						// YYYY-MM-DD
						str.Set(p_node->children->content);
						SString sub_str;
						str.Sub(0, 4, sub_str);
						pBill->Date.setyear((uint)sub_str.ToLong());
						str.Sub(5, 2, sub_str);
						pBill->Date.setmonth((uint)sub_str.ToLong());
						str.Sub(8, 2, sub_str);
						pBill->Date.setday((uint)sub_str.ToLong());
						ok = 1;
					}
					else if(SXml::IsName(p_node, ELEMENT_NAME_DELIVERDATE) && p_node->children) {
						// Ожидаемая дата доставки
						// YYYY-MM-DD
						str = (const char *)p_node->children->content;
						SString sub_str;
						str.Sub(0, 4, sub_str);
						pBill->DueDate.setyear((uint)sub_str.ToLong());
						str.Sub(5, 2, sub_str);
						pBill->DueDate.setmonth((uint)sub_str.ToLong());
						str.Sub(8, 2, sub_str);
						pBill->DueDate.setday((uint)sub_str.ToLong());
						ok = 1;
					}
				}
				//
				// Эти параметры только в DESADV
				//
				if(MessageType == PPEDIOP_DESADV) {
					if(SXml::IsName(p_node, "DespatchAdviceNumber") && p_node->children) {
						// Номер документа уведомления
						//strcpy(pBill->Code, (const char *)p_node->children->content);
						str.Set(p_node->children->content).Utf8ToChar(); // Будет няшно выглядеть в Papyrus
						STRNSCPY(pBill->Code, str);
						(TTN = (const char *)p_node->children->content).Utf8ToOem();
						ok = 1;
					}
					else if(SXml::IsName(p_node, "EstimatedDeliveryDate") && p_node->children) {
						// Ожидаемая дата поставки
						// YYYY-MM-DD
						str.Set(p_node->children->content);
						SString sub_str;
						str.Sub(0, 4, sub_str);
						pBill->DueDate.setyear((uint)sub_str.ToLong());
						str.Sub(5, 2, sub_str);
						pBill->DueDate.setmonth((uint)sub_str.ToLong());
						str.Sub(8, 2, sub_str);
						pBill->DueDate.setday((uint)sub_str.ToLong());
						ok = 1;
					}
					else if(SXml::IsName(p_node, ELEMENT_NAME_DESPATCHADVICEDATE) && p_node->children) {
						// Дата документа уведомления
						// YYYY-MM-DD
						str.Set(p_node->children->content);
						SString sub_str;
						str.Sub(0, 4, sub_str);
						pBill->Date.setyear((uint)sub_str.ToLong());
						str.Sub(5, 2, sub_str);
						pBill->Date.setmonth((uint)sub_str.ToLong());
						str.Sub(8, 2, sub_str);
						pBill->Date.setday((uint)sub_str.ToLong());
						ok = 1;
					}
				}
				//
				// Общие параметры
				//
				if(SXml::IsName(p_node, "DocumentFunctionCode") && p_node->children) {
					// Код функции документа
				}
				else if(SXml::IsName(p_node, ELEMENT_NAME_BUYERORDERNUMBER) && p_node->children) {
					// Номер заказа, на который пришел данный ответ
					strcpy(pBill->OrderBillNo, (const char *)p_node->children->content);
					ok = 1;
				}
				else if(SXml::IsName(p_node, ELEMENT_NAME_BUYERORDERDATE) && p_node->children) {
					// Дата заказа, на который пришел данный ответ
					// YYYY-MM-DD
					str = (const char *)p_node->children->content;
					SString sub_str;
					str.Sub(0, 4, sub_str);
					pBill->OrderDate.setyear((uint)sub_str.ToLong());
					str.Sub(5, 2, sub_str);
					pBill->OrderDate.setmonth((uint)sub_str.ToLong());
					str.Sub(8, 2, sub_str);
					pBill->OrderDate.setday((uint)sub_str.ToLong());
					ok = 1;
				}
				else if(SXml::IsName(p_node, "Receiver") && p_node->children) {
					p_node = p_node->children;
					if(SXml::IsName(p_node, "ILN") && p_node->children) {
						// GLN получателя
						// Записываем GLN покупателя
						strcpy(pBill->MainGLN, (const char *)p_node->children->content);
						ok = 1;
					}
				}
				else if(SXml::IsName(p_node, "Sender") && p_node->children) {
					p_node = p_node->children;
					if(SXml::IsName(p_node, "ILN") && p_node->children) { // GLN отправителя
						strcpy(pBill->GLN, (const char *)p_node->children->content);
						ok = 1;
					}
				}
				else if(SXml::IsName(p_node, "DeliveryPoint") && p_node->children) {
					p_node = p_node->children;
					if(SXml::IsName(p_node, "ILN") && p_node->children) { // GLN точки доставки
						STRNSCPY(pBill->DlvrAddrCode, p_node->children->content);
						ok = 1;
					}
				}
				else if(SXml::IsName(p_node, ELEMENT_NAME_TOTALLINES) && p_node->children) { // Количество товарных строк
					GoodsCount = atoi((const char *)p_node->children->content);
					ok = 1;
				}
				else if(SXml::IsName(p_node, ELEMENT_NAME_TOTALGROSSAMOUNT) && p_node->children) { // Сумма документа с НДС
					pBill->Amount = atof((const char *)p_node->children->content);
					ok = 1;
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
// Для ORDRESP и DESADV
// Разбираем полученный документ и заполняем Sdr_BRow
// Returns:
//		-1 - считаны все товарные строки. Однако, если в документе в поле общего количество написано число,
//			меньшее реального числа товарных позиций, то лишние позиции не считаются
//		 0 - ошибка
//		 1 - считана очередная товарная строка
//
int ImportCls::ParseForGoodsData(Sdr_BRow * pBRow)
{
	int    ok = 1, index = 1, line_end = 0, exit_while = 0;
	SString str;
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
		while(p_node && !line_end) {
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
			if(p_node && (p_node->type == XML_READER_TYPE_ELEMENT)) {
				if(SXml::IsName(p_node, "Line") && p_node->children) {
					if(index == (Itr.GetCount() + 1)) {
						while(p_node && !line_end) {
							exit_while = 0;
							if(p_node->children && (p_node->children->type == XML_READER_TYPE_ELEMENT))
								p_node = p_node->children;
							else if(p_node->next)
								p_node = p_node->next;
							else {
								while(p_node && p_node->parent && !exit_while) {
									xmlNode * p_node_2 = p_node->parent->next;
									if(p_node_2) {
										p_node = p_node_2;
										exit_while = 1;
									}
									else
										p_node = p_node->parent;
								}
							}
							if(p_node) {
								//
								// Первое условие актуально для последней товарной позиции. Если здесь не выйдем,
								// то цикл начнет читать документ заново.
								//
								if(p_node->type == XML_DOCUMENT_NODE || SXml::IsName(p_node, "Line"))
									line_end = 1;
								else {
									if(MessageType == PPEDIOP_ORDERRSP) {
										if(p_node->children) {
											if(sstreq(p_node->name, ELEMENT_NAME_ITEMSTATUS)) {
												// Статус товарной позиции (принят, изменен, не принят)
											}
											else if(sstreq(p_node->name, ELEMENT_NAME_ALLOCDELIVRD)) {
												// Утвержденное количество товара
												pBRow->Quantity = atof((const char *)p_node->children->content);
											}
											else if(sstreq(p_node->name, "OrderedUnitPacksize")) {
												// Количество товара в упаковке
												pBRow->UnitPerPack = atof((const char *)p_node->children->content);
											}
											else if(sstreq(p_node->name, "OrderedUnitGrossPrice")) {
												// Цена с НДС
												pBRow->Cost = atof((const char *)p_node->children->content);
											}
										}
									}
									else if(MessageType == PPEDIOP_DESADV) {
										if(p_node->children) {
											if(sstreq(p_node->name, ELEMENT_NAME_QTTYDISPATCHED)) {
												// Отгруженное количество
												pBRow->Quantity = atof((const char *)p_node->children->content);
											}
											else if(sstreq(p_node->name, ELEMENT_NAME_UNITPACKSZ)) {
												// Количество товара в упаковке
												pBRow->UnitPerPack = atof((const char *)p_node->children->content);
											}
											else if(sstreq(p_node->name, ELEMENT_NAME_UNITGROSSPRICE)) {
												// Цена с НДС
												pBRow->Cost = atof((const char *)p_node->children->content);
											}
											else if(sstreq(p_node->name, ELEMENT_NAME_ALCOCONTENT)) {
												// Содержание алкоголя в %
											}
											else if(sstreq(p_node->name, "Line-Party")) {
												// Читаем инфу о производителе
												p_node = p_node->children;
												if(SXml::IsName(p_node, ELEMENT_NAME_PARTYTYPE) && p_node->children) {
													// Тип инфы - производитель?
													if(SXml::IsContent(p_node->children, ELEMENT_CODE_PARTYTYPE_MF)) {
														while(p_node->next) {
															p_node = p_node->next;
															if(p_node->children) {
																//if(sstreq(p_node->name, "ILN") && p_node->children) {
																//	// GLN производителя
																//	str = (const char *)p_node->children->content;
																//}
																if(SXml::IsName(p_node, ELEMENT_NAME_TAXID)) { // ИНН производителя
																	STRNSCPY(pBRow->ManufINN, p_node->children->content);
																}
																else if(SXml::IsName(p_node, ELEMENT_NAME_TAXRECREASONCODE)) { // КПП производителя
																	STRNSCPY(pBRow->ManufKPP, p_node->children->content);
																}
																else if(SXml::IsName(p_node, ELEMENT_NAME_NAME)) { // Наименование производителя
																	str.Set(p_node->children->content).Utf8ToOem().CopyTo(pBRow->LotManuf, sizeof(pBRow->LotManuf));
																}
															}
														}
													}
												}
											}
											else if(SXml::IsName(p_node, "Reference-Elements")) {
												// Читаем дополнительную инфу о товаре
												int is_next = 1;
												p_node = p_node->children;
												while(is_next) {
													if(SXml::IsName(p_node, "Reference-Element") && p_node->children) {
														p_node = p_node->children;
														if(SXml::IsName(p_node, "Reference-Type") && p_node->children) {
															// Если это ГТД
															if(SXml::IsContent(p_node->children, ELEMENT_CODE_REFTYPE_ABT)) {
																p_node = p_node->next;
																if(SXml::IsName(p_node, "Reference-Id") && p_node->children) {
																	str.Set(p_node->children->content).CopyTo(pBRow->CLB, sizeof(pBRow->CLB));
																}
															}
															// Если это серия
															else if(SXml::IsContent(p_node->children, ELEMENT_CODE_REFTYPE_SER)) {
																p_node = p_node->next;
																if(SXml::IsName(p_node, "Reference-Id") && p_node->children) {
																	str.Set(p_node->children->content).CopyTo(pBRow->Serial, sizeof(pBRow->Serial));
																}
															}
															// Если это код вида продукции
															else if(SXml::IsContent(p_node->children, ELEMENT_CODE_REFTYPE_YA1)) {
																p_node = p_node->next;
																if(SXml::IsName(p_node, "Reference-Id") && p_node->children) {
																	pBRow->GoodKindCode = str.Set(p_node->children->content).ToLong();
																}
															}
															p_node = p_node->parent;
															if(p_node->next)
																p_node = p_node->next;
															else
																is_next = 0;
														}
													}
												}
											}
										}
									}
									// Общие параметры
									if(SXml::IsName(p_node, "LineNumber") && p_node->children) {
										// Номер строки товарной позиции
									}
									else if(SXml::IsName(p_node, "EAN") && p_node->children) {
										// Штрихкод товара
										strcpy(pBRow->Barcode, (const char *)p_node->children->content);
									}
									else if(SXml::IsName(p_node, "SupplierItemCode") && p_node->children) {
										// Артикул товара у поставщика
										strcpy(pBRow->ArCode, (const char *)p_node->children->content);
									}
									else if(SXml::IsName(p_node, ELEMENT_NAME_ITEMDISCR) && p_node->children) {
										// Описание товара (название)
										(str = (const char *)p_node->children->content).Utf8ToOem();
										STRNSCPY(pBRow->GoodsName, str);
									}
									else if(SXml::IsName(p_node, "UnitOfMeasure") && p_node->children) {
										// Единицы измерения
										if(SXml::IsContent(p_node->children, ELEMENT_CODE_UNITOFMEASURE_KGM))
											strcpy(pBRow->UnitName, UNIT_NAME_KG);
										else // Если в штуках
											strcpy(pBRow->UnitName, UNIT_NAME_PIECE);
									}
									else if(SXml::IsName(p_node, ELEMENT_NAME_TAXRATE) && p_node->children) {
										// Ставка НДС в %
										pBRow->VatRate = atof((const char *)p_node->children->content);
									}
								}
							}
						}
					}
					// Номер ТТН. Прочитан из заголовка документа
					strcpy(pBRow->TTN, TTN);
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

// Descr: Разбор результата, полученного с помощью Receive() для APERAK
// ARG(pResp		 IN): Данные для разбора, полученные в Receive()
// Retruns:
//		-1 - не было получено входящих сообщений
//		 0 - ошибка
//		 1 - оба параметра найдены
int ImportCls::ParseAperakResp(const char * pResp)
{
	int ok = 1, is_correct = 0, exit_while = 0;
	SString str;
	xmlDoc * p_doc = 0;

	AperakInfo.Clear();
	if(pResp) {
		THROWERR_STR(fileExists(ImpFileName), IEERR_IMPFILENOTFOUND, ImpFileName);
		THROWERR((p_doc = xmlReadFile(ImpFileName, NULL, XML_PARSE_NOENT)), IEERR_NULLREADXMLPTR);
		xmlNode * p_node = xmlDocGetRootElement(p_doc);
		THROWERR(p_node, IEERR_XMLREAD);
		if(SXml::IsName(p_node, "Document-ApplicationMessage") && p_node->children) // По первому тэгу можно понять, что это Aperak
			is_correct = 1;
		while(is_correct && p_node && (p_node->type == XML_ELEMENT_NODE)) {
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
				if(is_correct) {
					if(SXml::IsName(p_node, ELEMENT_NAME_DOCNUMBER) && p_node->children) {
						// Запомним номер документа заказа
						(str = (const char *)p_node->children->content).Utf8ToOem();
						AperakInfo.OrderNum = str;
					}
					else if(SXml::IsName(p_node, ELEMENT_NAME_DOCUMENTID) && p_node->children) {
						// Читаем ID документа статуса
						AperakInfo.DocID = (const char *)p_node->children->content;
					}
					else if(SXml::IsName(p_node, ELEMENT_NAME_DOCDATE) && p_node->children) {
						// Запишем дату документа заказа
						SString sub;
						str = (const char *)p_node->children->content;
						str.Sub(0, 4, sub);
						AperakInfo.OrderDate.setyear((uint)sub.ToLong());
						str.Sub(5, 2, sub);
						AperakInfo.OrderDate.setmonth((uint)sub.ToLong());
						str.Sub(8, 2, sub);
						AperakInfo.OrderDate.setday((uint)sub.ToLong());
					}
					else if(SXml::IsName(p_node, ELEMENT_NAME_BUYER) && p_node->children) {
						p_node = p_node->children;
						if(SXml::IsName(p_node, "ILN") && p_node->children)
							// Запишем GLN покупателя
							AperakInfo.BuyerGLN = (const char *)p_node->children->content;
					}
					else if(p_node && sstreq(p_node->name, ELEMENT_NAME_SELLER) && p_node->children) {
						p_node = p_node->children;
						if(SXml::IsName(p_node, "ILN") && p_node->children)
							// Запишем GLN поставщика
							AperakInfo.SupplGLN = (const char *)p_node->children->content;
					}
					else if(p_node && sstreq(p_node->name, "DeliveryPoint") && p_node->children) {
						p_node = p_node->children;
						if(SXml::IsName(p_node, "ILN") && p_node->children)
							// Запишем GLN точки поставки
							AperakInfo.AddrGLN = (const char *)p_node->children->content;
					}
					else if(p_node && sstreq(p_node->name, "LineMessageCode") && p_node->children) {
						// Запомним код статуса
						AperakInfo.Code = (const char *)p_node->children->content;
					}
					else if(p_node && sstreq(p_node->name, ELEMENT_NAME_SYSTEMSGTEXT) && p_node->children) {
						// Запомним описание статуса
						AperakInfo.Msg = (const char *)p_node->children->content;
					}
				}
			}
		}
	}
	else
		ok = -1;
	THROWERR_STR(is_correct, IEERR_INVMESSAGEYTYPE, "APERAK");
	CATCH
		SysLogMessage(SYSLOG_PARSEAPERAKRESP);
		ok = 0;
	ENDCATCH;
	if(p_doc)
		xmlFreeDoc(p_doc);
	return ok;
}
//
//
// Общие функции для импорта/экспорта
//
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
		THROW(P_ImportCls->SetNewStatus(err_track_id_list.Z()));
		if(!P_ImportCls->IncomMessagesCounter && ((P_ImportCls->MessageType == PPEDIOP_ORDERRSP) || (P_ImportCls->MessageType == PPEDIOP_DESADV))) {
			SysLogMessage(LOG_NOINCOMDOC);
			LogMessage(LOG_NOINCOMDOC);
		}
	}
	CATCH
		SysLogMessage(SYSLOG_FINISHIMPEXP);
		GetErrorMsg(str.Z());
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

void ProcessError(EDIServiceSoapProxy & rProxy)
{
	char   temp_err_buf[1024];
	SString temp_buf;
	ErrorCode = IEERR_SOAP;
	rProxy.soap_sprint_fault(temp_err_buf, sizeof(temp_err_buf));
	(temp_buf = temp_err_buf).Utf8ToChar();
	StrError = temp_err_buf;
}

