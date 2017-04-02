// PPSMS.H
//
#ifndef __PPSMS_H
#define __PPSMS_H

#if 0 // {

class SmsProtocolBuf;

struct StConfig {
	StConfig();
	void   Clear();
	int    SetConfig_(const char * pHost, uint port, const char * pSystemId, const char * pLogin,
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
	struct StSMResults {
		StSMResults();
		int    GetResult(int kindOfResult);

		int    BindResult;
		int    UnbindResult;
		int    SubmitResult;
		int    DataResult;
		int    EnquireLinkResult;
		int    GenericNackResult;
	};
	void   SetConfig(const StConfig & rConfig)
	{
		Config = rConfig;
	}
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

#endif // } 0

#endif // __PPSMS_H
