// PPSMS.H
//
#ifndef __PPSMS_H
#define __PPSMS_H

// Kernel Parameters
#define MAX_BUFFER_SIZE			1048576 // 1MB Размер буфера для приема данных
#define MAX_PDU_SIZE			 131072 // 128 KB Максимальный размер пакета SMPP
#define RECONNECT_TIMEOUT		   /*5000*/ 1000 // miliseconds
// @v8.5.4 #define WAIT_PACKET_RESPONSE	  15000 // milliseconds Время ожидания ответа на запрос от СМСЦ
#define ENQUIRE_LINK_TIMEOUT	  60000 // miliseconds (обычно 1 минута)
#define RESEND_QUEUE_MSG_TIMEOUT  /*10000*/ 5000 // milliseconds Таймаут повторной отправки смс при переполнении очереди сообщений
#define BIND_RECEIVE_TIMEOUT      15000 // Таймаут для получения ответа на команды Bind и Unbind
#define ENQLINQ_RECEIVE_TIMEOUT    3000 // Таймаут для получения ответа на команд SendEnquireLink
#define GENNAC_RECEIVE_TIMEOUT     3000 // Таймаут для получения ответа на команд SendGenericNack
#define SUBMIT_RECEIVE_TIMEOUT     1000 // Таймаут для получения ответа на команд SubmitSM
#define RESEND_MSG_COUNT			  2 // Число попыток повторно отправить смс
#define RECONNECTION_COUNT			  2 // Число попыток переподключения
#define REST_OF_RECEIVES_COUNT		  2 // Число попыток прослушивания сокета перед отключением
//#define UNDELIVERABLE_MESSAGES		 10 // Число недостовляемых сообщений (в протоколе рекомендовано, чтобы было не более 10-ти таких сообщений)
#define INTERFACE_VERSION		   0x34 // Версия интерфейса SMPP
#define SPLIT_LONG_TEXT			  true // Разбить длинное сообщение на несколько пакетов
#define USE_ENQUIRELINK			  true // Посылать команду проверки связи через промежуток времени ENQUIRE_LINK_TIMEOUT
#define REPLACE_IF_PRESENT		 false // Заменить существующее сообщение для данного адресата
#define ADDRESS_RANGE				"" // Формат адресов отправителя
// Параметры VALIDITY_PERIOD и SCHEDULE_DELIVERY_TIME либо пустые строки, либо строки
// длиной 17 символов(с символом окончания строки) формата "YYMMDDhhmmsstnnp", где
//	'YY' - две последние цифры года (00-99)
//	'MM' - месяц(01-12)
//	'DD' - день(01-31)
//	'hh' - час(00-23)
//	'mm' - минута(00-59)
//	'ss' - секунда(00-59)
//	't'  - десятые секунды (0-9)
//	'nn' - разница во времени в четверти часа между местным	временем (как выражено в первых 13 октетах)
//		   и UTC временем (Всеобщим Скоординированным Временем - Universal Time Constant) (00-48)
//	'p'  - '+' местное время впереди на четверть часа поотношению ко времени UTC
//		   '-' местное время отстает на четверть часа по отношению ко времени UTC
//		   'R' местное время является относительным к текущему времени SMSC
#define VALIDITY_PERIOD			"000000240000000R" // Период допустимости сообщения (24 часа)
#define SCHEDULE_DELIVERY_TIME					"" // Немеделнная доставка сообщения
//
// Максимальные размеры полей запросов
//
#define MAX_PASSWORD_LEN				  9 // Вместе с символом конца строки
#define MAX_SYSTEM_TYPE_LEN				 13 // Вместе с символом конца строки
#define MAX_SYSTEM_ID_LEN				 16 // Вместе с символом конца строки
#define MAX_SERVICE_TYPE_LEN			  6 // Вместе с символом конца строки
#define MAX_ADDR_LEN					 12 // Вместе с символом конца строки
#define MAX_DATE_LEN					 17 // Вместе с символом конца строки (для параметров SCHEDULE_DELIVERY_TIME, VALIDITY_PERIOD)
#define MAX_ADDR_RANGE_LEN				 41 // Вместе с символом конца строки
#define MAX_SUBMIT_MESSAGE_LEN			254 // (byte) Максимальный размер, который может поместиться  в поле short_message (дальше используется поле message_payload)
#define MAX_MESSAGE_7BIT_LONG		  38760 // (byte) Максимальный Размер длинного сообщения, если символы кодируются 7-ю битами
#define MAX_MESSAGE_8BIT_LONG		  34170 // (byte) Максимальный размер длинного сообщения, если символы кодируются 8-ю битами
#define MAX_MESSAGE_UCS2_LONG		  17085 // (byte) Максимальный размер длинного сообщения при кодировке UTF-16
#define MAX_MESSAGE_7BIT_LEN			160 // Максимальный размер короткого сообщения при 7-битной кодировке
#define MAX_MESSAGE_8BIT_LEN			140 // Максимальный размер короткого сообщения при 8-битной кодировке
#define MAX_MESSAGE_UCS2_LEN			 70 // Максимальный размер короткого сообщения при UTF-16 кодировке
#define MAX_MESSAGE_7BIT_NOTSPLIT_LEN	153 // Максимальный размер части длинного сообщения	при 7-битной кодировке
#define MAX_MESSAGE_UCS2_NOTSPLIT_LEN	 67 // Максимальный размер части длинного сообщения	при UTF-16 кодировке
//
// ConnectionStates
//
#define SMPP_SOCKET_CONNECTED		1
#define SMPP_BIND_SENT				2
#define SMPP_BINDED					3
#define SMPP_UNBIND_SENT			4
#define SMPP_UNBINDED				5
#define SMPP_SOCKET_DISCONNECTED	6
//
// StatusCodes
//
#define ESME_ROK            0x00000000 // No Error
#define ESME_RINVMSGLEN     0x00000001 // Message Length is invalid
#define ESME_RINVCMDLEN		0x00000002 // Command Length is invalid
#define ESME_RINVCMDID		0x00000003 // Invalid Command ID
#define ESME_RINVBNDSTS		0x00000004 // Incorrect BIND Status for given command
#define ESME_RALYBND		0x00000005 // ESME Already in Bound State
#define ESME_RINVPRTFLG		0x00000006 // Invalid Priority Flag
#define ESME_RINVREGDLVFLG	0x00000007 // Invalid Registered Delivery Flag
#define ESME_RSYSERR		0x00000008 // System Error
#define ESME_RINVSRCADR		0x0000000A // Invalid Source Address
#define ESME_RINVDSTADR		0x0000000B // Invalid Dest Addr
#define ESME_RINVMSGID		0x0000000C // Message ID is invalid
#define ESME_RBINDFAIL		0x0000000D // Bind Failed
#define ESME_RINVPASWD		0x0000000E // Invalid Password
#define ESME_RINVSYSID		0x0000000F // Invalid System ID
#define ESME_RCANCELFAIL	0x00000011 // Cancel SM Failed
#define ESME_RREPLACEFAIL	0x00000013 // Replace SM Failed
#define ESME_RMSGQFULL		0x00000014 // Message Queue Full
#define ESME_RINVSERTYP		0x00000015 // Invalid Service Type
#define ESME_RINVNUMDESTS	0x00000033 // Invalid number of destinations
#define ESME_RINVDLNAME		0x00000034 // Invalid Distribution List name
#define ESME_RINVDESTFLAG	0x00000040 // Destination flag is invalid(submit_multi)
#define ESME_RINVSUBREP		0x00000042 // Invalid 'submit with replace' request(i.e. submit_sm with replace_if_present_flag set)
#define ESME_RINVESMCLASS	0x00000043 // Invalid esm_class field data
#define ESME_RCNTSUBDL		0x00000044 // Cannot Submit to Distribution List
#define ESME_RSUBMITFAIL	0x00000045 // submit_sm or submit_multi failed
#define ESME_RINVSRCTON		0x00000048 // Invalid Source address TON
#define ESME_RINVSRCNPI		0x00000049 // Invalid Source address NPI
#define ESME_RINVDSTTON		0x00000050 // Invalid Destination address TON
#define ESME_RINVDSTNPI		0x00000051 // Invalid Destination address NPI
#define ESME_RINVSYSTYP		0x00000053 // Invalid system_type field
#define ESME_RINVREPFLAG	0x00000054 // Invalid replace_if_present flag
#define ESME_RINVNUMMSGS	0x00000055 // Invalid number of messages
#define ESME_RTHROTTLED		0x00000058 // Throttling error (ESME has exceeded allowed message limits)
#define ESME_RINVSCHED		0x00000061 // Invalid Scheduled Delivery Time
#define ESME_RINVEXPIRY         0x00000062 // Invalid message validity period (Expiry time)
#define ESME_RINVDFTMSGID       0x00000063 // Predefined Message Invalid or Not Found
#define ESME_RX_T_APPN          0x00000064 // ESME Receiver Temporary App Error Code
#define ESME_RX_P_APPN          0x00000065 // ESME Receiver Permanent App Error Code
#define ESME_RX_R_APPN          0x00000066 // ESME Receiver Reject Message Error Code
#define ESME_RQUERYFAIL         0x00000067 // Query_sm request failed
#define ESME_RINVOPTPARSTREAM	0x000000C0 // Error in the optional part of the PDU Body.
#define ESME_ROPTPARNOTALLWD	0x000000C1 // Optional Parameter not allowed
#define ESME_RINVPARLEN         0x000000C2 // Invalid Parameter Length.
#define ESME_RMISSINGOPTPARAM   0x000000C3 // Expected Optional Parameter missing
#define ESME_RINVOPTPARAMVAL    0x000000C4 // Invalid Optional Parameter Value
#define ESME_RDELIVERYFAILURE   0x000000FE // Delivery Failure (used for data_sm_resp)
#define ESME_RUNKNOWNERR        0x000000FF // Unknown Error
#define NO_STATUS               -1 // Статус не получен
//
// PriorityFlags
//
#define BULK		0
#define NORMAL		1
#define URGENT		2
#define VERY_URGENT	3

// AddressTons
/*
#define UNKNOWN_TON			0
#define INTERNATIONAL		1
#define NATIONAL			2
#define NETWORK_SPECIFIC	3
#define SUBSCRIBER_NUMBER	4
#define ALPHANUMERIC		5
#define ABBREVIATED			6
*/
#define TON_UNKNOWN           0
#define TON_INTERNATIONAL     1
#define TON_NATIONAL          2
#define TON_NETWORK_SPECIFIC  3
#define TON_SUBSCRIBER_NUMBER 4
#define TON_ALPHANUMERIC      5
#define TON_ABBREVIATED       6
//
// AddressNpis
//
#define NPI_UNKNOWN           0
#define NPI_ISDN              1
//
// Data coding
//
#define SMSC_DEFAULT          0 // Часто это кодирока GSM, где символ кодируется 7-ю битами
#define UCS2                  8 // UCS2(ISO/IEC-10646)
//
// Service types
//
#define DEFAULT_SERVICE_TYPE    0 // Default
#define CMT_SERVISE_TYPE        1 // Cellular Messaging
#define CPT_SRVICE_TYPE         2 // Cellular Paging
#define VMN_SERVICE_TYPE        3 // Voice Mail Notification
#define VMA_SERVICE_TYPE        4 // Voice Mail Alerting
#define WAP_SERVICE_TYPE        5 // WirelessApplication Protocol
#define USSD_SERVICETYPE        6 // Unstructured SupplementaryServices Data
//
// Коды команд запроса
//
#define SUBMIT_SM			0x00000004
#define DELIVER_SM			0x00000005
#define UNBIND				0x00000006
#define BIND_TRANSCEIVER	0x00000009
#define ENQUIRE_LINK		0x00000015
//
// Коды команд ответов
//
#define GENERIC_NACK			0x80000000
#define SUBMIT_SM_RESP			0x80000004
#define DELIVER_SM_RESP			0x80000005
#define UNBIND_RESP				0x80000006
#define BIND_TRANSCEIVER_RESP	0x80000009
#define ENQUIRE_LINK_RESP		0x80000015
//
// Статусы сообщений
//
#define SMS_DELIVERED		1	// Сообщение доставлено адресату.
#define SMS_EXPIRED			2	// Истек период допустимости сообщения.
#define SMS_DELETED			3	// Сообщение было удалено.
#define SMS_UNDELIVERABLE	4	// Сообщение является недоставляемым.
#define SMS_ACCEPTED		5	// Сообщение находится в принятом состоянии(т.е. читалось вручную от имени абонента	клиентской службой).
#define SMS_UNKNOWN			6	// Сообщение находится в недопустимом состоянии.
#define SMS_REJECTED		7	// Сообщение находится в отклоненном состоянии.

// Строковые статусы сообщений (в ответе deliver_sm)
#define SMS_DELIVERED_STR		"DELIVRD" // Сообщение доставлено адресату
#define SMS_EXPIRED_STR			"EXPIRED" // Период допустимости сообщения истек.
#define SMS_DELETED_STR 		"DELETED" // Сообщение было удалено.
#define SMS_UNDELIVERABLE_STR	"UNDELIV" // Сообщение является недоставляемым.
#define SMS_ACCEPTED_STR		"ACCEPTD" // Сообщение находится в принятом состоянии (то есть, прочитано вручную от имени абонента клиентской службы).
#define SMS_UNKNOWN_STR			"UNKNOWN" // Сообщение находится в ошибочном состоянии.
#define SMS_REJECTED_STR		"REJECTD" // Сообщение находится в отклоненном состоянии.

class SmsProtocolBuf;

struct StSmscErrorText {
	int    Id;
	SString ErrorMsg;
};

struct StStatusText {
	int    Id;
	SString Status;
};

StSmscErrorText SmscErrorMsgs[];
StStatusText StatusTexts[];

SString & GetSmscErrorText(int error, SString & rErrorText);
SString & GetStatusText(int status, SString & rStatusText);
// @v9.3.7 @unused int IsLatinFull(const SString & rStr);

struct StConfig {
	StConfig();
	void   Clear();
	int    SetConfig(const char * pHost, uint port, const char * pSystemId, const char * pLogin,
		const char * pPassword, const char * pSystemType, uint sourceAddressTon, uint sourceAddressNpi,
		uint destAddressTon, uint destAddressNpi, uint dataCoding, const char * pFrom, uint splitLongMsg);

	SString Host;
	uint   Port;
	SString SystemId; // Идентификатор клиента в системе
	SString Login;
	SString Password;
	SString SystemType; // Тип системы
	SString AddressRange; // Фильтр адресов отправителя
	uint   SourceAddressTon; // Тип номера отправителя
	uint   SourceAddressNpi; //Цифровой индикатор плана отправителя
	uint   DestAddressTon; // Тип номера получателя
	uint   DestAddressNpi; // Цифровой индикатор плана получателя
	uint   DataCoding; // Схема кодирования сообщения
	uint   EsmClass; // Тип сообщения и режим отправки сообщения
	SString From; // Имя или номер отправителя
	uint   SplitLongMsg; // Разбивать или нет длинные сообщения
	uint16 ResponseTimeout;
	uint16 ResendMsgQueueTimeout;
	uint16 ResendTriesNum;
	uint16 ReconnectTimeout;
	uint16 ReconnectTriesNum;
};

struct StSMResults {
	StSMResults();
	int   GetResult(int kindOfResult);

	int    BindResult;
	int    UnbindResult;
	int    SubmitResult;
	int    DataResult;
	int    EnquireLinkResult;
	int    GenericNackResult;
};

class SmsClient {
public:
	struct StSubmitSMParam { // Структура параметров запроса submit_sm
		StSubmitSMParam();
		void   Clear();

		uchar  SourceAddressTon;
		uchar  SourceAddressNpi;
		uchar  DestinationAddressTon;
		uchar  DestinationAddressNpi;
		uchar  EsmClass;
		uchar  ProtocolId;
		uchar  PriorityFlag;
		uchar  ReplaceIfPresentFlag;
		uchar  DataCoding;
		uchar  SmDefaultMsgId;
		uint8  Reserve[2]; // @alignment
		SString SourceAddress;
		SString DestinationAddress;
		SString SheduleDeliveryTime;
		SString ValidityPeriod;
	};
	SmsClient();
	~SmsClient();
	int    CanSend() const;
	int    IsConnected() const;
	int    Connect(const StConfig & config);
    int    Disconnect();
	//
	// Descr: Считывает номер абонента и описание состояния сообщения из массива состояний StatusCodesArr
	//
	int    GetStatusCode(SString & rDestNum, SString & rStatus, size_t pos) const;
	//
	// Descr: Считывает из массива ErrorSubmitArr номер абонента и описание ошибки отправленного сообщения
	//
	int    GetErrorSubmit(SString & rDestNum, SString & rErrText, size_t pos) const;
	//
	// Descr: Получает оставшиеся ответы от СМСЦ, если они есть
	//
	int    GetRestOfReceives();
	int    SendSms(const char * pTo, const char * pText, SString & rStatus);
	//
	// @v8.5.4 void   SetRecvTimeout(int timeout) { RecvTimeout = timeout; }

	PPLogger Logger;
private:
	void   SetConfig(const StConfig & rConfig) { Config = rConfig; }
	//
	// Descr: Добавляет информацию об ошибочных командах посылки смс в массив ErrorSubmitArr
	//
	void   AddErrorSubmit(const char * pDestNum, int errCode);
	void   DecodeDeliverSm(int sequenceNumber, void * pPduBody, size_t bodyLength);
	void   DisconnectSocket();
	//
	// Descr: Добавляет статус смс в массив StatusCodesArr
	//
	void   AddStatusCode(const char * pDestNum, int statusCode, const char * pError);
	int    SendSms_(const char * pFrom, const char * pTo, const char * pText);
	int    Send(const void * data, size_t bufLen);
	int    Send(const SmsProtocolBuf & rBuf, int tryReconnect);
	int    Bind();
	int    Unbind();
	int    SubmitSM(const StSubmitSMParam & rParam, const char * pMessage, bool payloadMessage);
	int    ConnectToSMSC();
	int    TryToReconnect(uint & rRecconectionCount);
	int    SendEnquireLink(int sequenceNumber);
	int    SendEnquireLinkResp(int sequenceNnumber);
	int    SendGenericNack(int sequenceNumber, int commandStatus);
	int    SendDeliverSmResp(int sequenceNumber);
	int    Receive(uint timeout);
	//
	// Descr: Если USE_ENQUIRELINK = true, то посылает команду на проверку связи через каждый промежуток времени ENQUIRE_LINK_TIMEOUT
	//
	int    ReceiveToTimer();

	TcpSocket ClientSocket;
	LDATETIME StartTime;
	StConfig Config;
	StSMResults SMResults;
	StSubmitSMParam SMParams;
	//
	// Массив с информацией об ошибочных запросах посылки смс. Сруктура записи: номер_получателя;описание_ошибки
	//
	StrAssocArray ErrorSubmitArr;
	//
	// Массив структур состояний сообщений, в том числе и ошибочных. Сруктура записи: номер_получателя;состояние_сообщения
	//
	StrAssocArray StatusCodesArr;
	int    ConnectionState;
	uint   ResendErrLenMsg; // Счетчик попыток переотправить сообщение при неверной его длине
	uint   ReSendQueueMsgTryNums; // Счетчик попыток переотправить смс, которые не были отправлены из-за переполнения очереди
	uint   MessageCount; // Счетчик успешных запросов отправки сообщения (submit_sm)
	uint   SequenceNumber; // Номер пакета
	uint   AddStatusCodeNum; // Счетчик элементов массива StatusCodesArr
	uint   AddErrorSubmitNum; // Счетчик эелементов массива ErrorSubmitArr
	uint   UndeliverableMessages; // Счетчик недоставляемых сообщений
	// @v8.5.4 int    RecvTimeout;  // Таймаут получение данных
};
#endif // __PPSMS_H
