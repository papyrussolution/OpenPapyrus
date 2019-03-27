// BNKINPAS.CPP
// Библиотека для работы с терминалами INPAS, обслуживающими карты

#include <ppdrvapi.h>
#include <slib.h>
#include <comdisp.h>

extern PPDrvSession DRVS;

#define THROWERR(expr,val)     { if(!(expr)) { DRVS.SetErrCode(val); goto __scatch; } }

#define TIMEOUT                  180000  // Время на операцию в тысячных долях секунды
#define RUBLE                    643     // Код рубля

// Номера операций
#define INPAS_FUNC_INIT          26      // Инициализация устройства
#define INPAS_FUNC_PAY           1       // Оплата
#define INPAS_FUNC_REFUND        4       // Возврат/отмена оплаты
#define INPAS_FUNC_CLOSEDAY      59      // Закрытие дня

class PPDrvINPASTrmnl : public PPBaseDriver {
public:
	PPDrvINPASTrmnl()
	{
		SString file_name;
		getExecPath(file_name);
		(SlipLogFileName = file_name).SetLastSlash().Cat("INPAStrmnl_Slip.log"); // @v10.2.0
		DRVS.SetLogFileName(file_name.SetLastSlash().Cat("INPAStrmnl.log"));
	}
	int    ProcessCommand(const SString & rCmd, const char * pInputData, SString & rOutput);
	int	   Init(SString & rCheck);
	int    Pay(double amount, SString & rSlip);
	int	   Refund(double amount, SString & rSlip);
	int	   GetSessReport(SString & rCheck);
private:
	enum {
		InitResources = 1,          // Метод класса DualConnectorInterface
		Exchange,                   // Метод класса DualConnectorInterface
		FreeResources,              // Метод класса DualConnectorInterface
		SetChannelTerminalParam,	// Метод класса DualConnectorInterface
		Release,                    // Метод класса ISAPacket
		ErrorCode,                  // Св-во объекта класса DualConnectorInterface
		ErrorDescription,           // Св-во объекта класса DualConnectorInterface
		Amount,                     // Св-во объекта класса ISAPacket
		AdditionalAmount,           // Св-во объекта класса ISAPacket
		CurrencyCode,               // Св-во объекта класса ISAPacket
		DateTimeHost,               // Св-во объекта класса ISAPacket
		CardEntryMode,              // Св-во объекта класса ISAPacket
		PINCodingMode,              // Св-во объекта класса ISAPacket
		PAN,                        // Св-во объекта класса ISAPacket
		CardExpiryDate,             // Св-во объекта класса ISAPacket
		TRACK2,                     // Св-во объекта класса ISAPacket
		AuthorizationCode,          // Св-во объекта класса ISAPacket
		ReferenceNumber,            // Св-во объекта класса ISAPacket
		ResponseCodeHost,           // Св-во объекта класса ISAPacket
		PinBlock,                   // Св-во объекта класса ISAPacket
		PinKey,                     // Св-во объекта класса ISAPacket
		WorkKey,                    // Св-во объекта класса ISAPacket
		TextResponse,               // Св-во объекта класса ISAPacket
		TerminalDateTime,           // Св-во объекта класса ISAPacket
		TrxID,                      // Св-во объекта класса ISAPacket
		OperationCode,              // Св-во объекта класса ISAPacket
		TerminalTrxID,              // Св-во объекта класса ISAPacket
		TerminalID,                 // Св-во объекта класса ISAPacket
		MerchantID,                 // Св-во объекта класса ISAPacket
		DebitAmount,                // Св-во объекта класса ISAPacket
		DebitCount,                 // Св-во объекта класса ISAPacket
		CreditAmount,               // Св-во объекта класса ISAPacket
		CreditCount,                // Св-во объекта класса ISAPacket
		OrigOperation,              // Св-во объекта класса ISAPacket
		MAC,                        // Св-во объекта класса ISAPacket
		Status,                     // Св-во объекта класса ISAPacket
		AdminTrack2,                // Св-во объекта класса ISAPacket
		AdminPinBlock,              // Св-во объекта класса ISAPacket
		AdminPAN,                   // Св-во объекта класса ISAPacket
		AdminCardExpiryDate,        // Св-во объекта класса ISAPacket
		AdminCardEntryMode,         // Св-во объекта класса ISAPacket
		VoidDebitAmount,            // Св-во объекта класса ISAPacket
		VoidDebitCount,             // Св-во объекта класса ISAPacket
		VoidCreditAmount,           // Св-во объекта класса ISAPacket
		VoidCreditCount,            // Св-во объекта класса ISAPacket
		ProcessingFlag,             // Св-во объекта класса ISAPacket
		HostTrxID,                  // Св-во объекта класса ISAPacket
		RecipientAddress,           // Св-во объекта класса ISAPacket
		CardWaitTimeout,            // Св-во объекта класса ISAPacket
		DeviceSerNumber,            // Св-во объекта класса ISAPacket
		CommandMode,                // Св-во объекта класса ISAPacket
		CommandMode2,               // Св-во объекта класса ISAPacket
		CommandResult,              // Св-во объекта класса ISAPacket
		FileData,                   // Св-во объекта класса ISAPacket
		MessageED,                  // Св-во объекта класса ISAPacket
		CashierRequest,             // Св-во объекта класса ISAPacket
		CashierResponse,            // Св-во объекта класса ISAPacket
		AccountType,                // Св-во объекта класса ISAPacket
		CommodityCode,              // Св-во объекта класса ISAPacket
		PaymentDetails,             // Св-во объекта класса ISAPacket
		ProviderCode,               // Св-во объекта класса ISAPacket
		Acquirer,                   // Св-во объекта класса ISAPacket
		AdditionalData,             // Св-во объекта класса ISAPacket
		ModelNo,                    // Св-во объекта класса ISAPacket
		ReceiptData,                // Св-во объекта класса ISAPacket
		TermResponseCode,           // Св-во объекта класса ISAPacket
		SlipNumber,                 // Св-во объекта класса ISAPacket
	};
	void AsseptDC(ComDispInterface * pNameDCObj) 
	{
		if(pNameDCObj) {
			ASSIGN_ID_BY_NAME(pNameDCObj, InitResources);
			ASSIGN_ID_BY_NAME(pNameDCObj, Exchange);
			ASSIGN_ID_BY_NAME(pNameDCObj, FreeResources);
			ASSIGN_ID_BY_NAME(pNameDCObj, SetChannelTerminalParam);
			ASSIGN_ID_BY_NAME(pNameDCObj, ErrorCode);
			ASSIGN_ID_BY_NAME(pNameDCObj, ErrorDescription);
		}
	}
	void AsseptSAP(ComDispInterface * pNameSAPObj) 
	{
		if(pNameSAPObj) {
			ASSIGN_ID_BY_NAME(pNameSAPObj, Amount);
			ASSIGN_ID_BY_NAME(pNameSAPObj, AdditionalAmount);
			ASSIGN_ID_BY_NAME(pNameSAPObj, CurrencyCode);
			ASSIGN_ID_BY_NAME(pNameSAPObj, DateTimeHost);
			ASSIGN_ID_BY_NAME(pNameSAPObj, CardEntryMode);
			ASSIGN_ID_BY_NAME(pNameSAPObj, PINCodingMode);
			ASSIGN_ID_BY_NAME(pNameSAPObj, PAN);
			ASSIGN_ID_BY_NAME(pNameSAPObj, CardExpiryDate);
			ASSIGN_ID_BY_NAME(pNameSAPObj, TRACK2);
			ASSIGN_ID_BY_NAME(pNameSAPObj, AuthorizationCode);
			ASSIGN_ID_BY_NAME(pNameSAPObj, ReferenceNumber);
			ASSIGN_ID_BY_NAME(pNameSAPObj, ResponseCodeHost);
			ASSIGN_ID_BY_NAME(pNameSAPObj, PinBlock);
			ASSIGN_ID_BY_NAME(pNameSAPObj, PinKey);
			ASSIGN_ID_BY_NAME(pNameSAPObj, WorkKey);
			ASSIGN_ID_BY_NAME(pNameSAPObj, TextResponse);
			ASSIGN_ID_BY_NAME(pNameSAPObj, TerminalDateTime);
			ASSIGN_ID_BY_NAME(pNameSAPObj, TrxID);
			ASSIGN_ID_BY_NAME(pNameSAPObj, OperationCode);
			ASSIGN_ID_BY_NAME(pNameSAPObj, TerminalTrxID);
			ASSIGN_ID_BY_NAME(pNameSAPObj, TerminalID);
			ASSIGN_ID_BY_NAME(pNameSAPObj, MerchantID);
			ASSIGN_ID_BY_NAME(pNameSAPObj, DebitAmount);
			ASSIGN_ID_BY_NAME(pNameSAPObj, DebitCount);
			ASSIGN_ID_BY_NAME(pNameSAPObj, CreditAmount);
			ASSIGN_ID_BY_NAME(pNameSAPObj, CreditCount);
			ASSIGN_ID_BY_NAME(pNameSAPObj, OrigOperation);
			ASSIGN_ID_BY_NAME(pNameSAPObj, MAC);
			ASSIGN_ID_BY_NAME(pNameSAPObj, Status);
			ASSIGN_ID_BY_NAME(pNameSAPObj, AdminTrack2);
			ASSIGN_ID_BY_NAME(pNameSAPObj, AdminPinBlock);
			ASSIGN_ID_BY_NAME(pNameSAPObj, AdminPAN);
			ASSIGN_ID_BY_NAME(pNameSAPObj, AdminCardExpiryDate);
			ASSIGN_ID_BY_NAME(pNameSAPObj, AdminCardEntryMode);
			ASSIGN_ID_BY_NAME(pNameSAPObj, VoidDebitAmount);
			ASSIGN_ID_BY_NAME(pNameSAPObj, VoidDebitCount);
			ASSIGN_ID_BY_NAME(pNameSAPObj, VoidCreditAmount);
			ASSIGN_ID_BY_NAME(pNameSAPObj, VoidCreditCount);
			ASSIGN_ID_BY_NAME(pNameSAPObj, ProcessingFlag);
			ASSIGN_ID_BY_NAME(pNameSAPObj, HostTrxID);
			ASSIGN_ID_BY_NAME(pNameSAPObj, RecipientAddress);
			ASSIGN_ID_BY_NAME(pNameSAPObj, CardWaitTimeout);
			ASSIGN_ID_BY_NAME(pNameSAPObj, DeviceSerNumber);
			ASSIGN_ID_BY_NAME(pNameSAPObj, CommandMode);
			ASSIGN_ID_BY_NAME(pNameSAPObj, CommandMode2);
			ASSIGN_ID_BY_NAME(pNameSAPObj, CommandResult);
			ASSIGN_ID_BY_NAME(pNameSAPObj, FileData);
			ASSIGN_ID_BY_NAME(pNameSAPObj, MessageED);
			ASSIGN_ID_BY_NAME(pNameSAPObj, CashierRequest);
			ASSIGN_ID_BY_NAME(pNameSAPObj, CashierResponse);
			ASSIGN_ID_BY_NAME(pNameSAPObj, AccountType);
			ASSIGN_ID_BY_NAME(pNameSAPObj, CommodityCode);
			ASSIGN_ID_BY_NAME(pNameSAPObj, PaymentDetails);
			ASSIGN_ID_BY_NAME(pNameSAPObj, ProviderCode);
			ASSIGN_ID_BY_NAME(pNameSAPObj, Acquirer);
			ASSIGN_ID_BY_NAME(pNameSAPObj, AdditionalData);
			ASSIGN_ID_BY_NAME(pNameSAPObj, ModelNo);
			ASSIGN_ID_BY_NAME(pNameSAPObj, ReceiptData);
			ASSIGN_ID_BY_NAME(pNameSAPObj, TermResponseCode);
			ASSIGN_ID_BY_NAME(pNameSAPObj, SlipNumber);
			ASSIGN_ID_BY_NAME(pNameSAPObj, Release);
		}
	}
	SString SlipLogFileName;
};

// перечисление статусов
static SIntToSymbTabEntry _StatusMsgTab_SAP[] = {
	{0,		"Неопределённый статус"},
	{1,		"Одобрено"},
	{16,	"Отказано"},
	{17,	"Выполнено в OFFLINE"},
	{34,	"Нет соединения"},
	{53,	"Операция прервана"}
};

// перечисление ошибок
static PPDrvSession::TextTableEntry _ErrMsgTab_DC[] = { 
	{0,		"ошибок устройства не найдено"},
	{1,		"истёк таймаут операции"},
	{2,		"ошибка создания LOG файла"},
	{3,		"общая ошибка"},
	{4,		"ошибка данных запроса"},
	{6,		"не найден файл конфигурации"},
	{7,		"ошибка формата файла конфигурации"},
	{8,		"ошибка параметров логирования"},
	{9,		"ошибка в параметрах терминала"},
	{10,	"ошибка настройки устройства на COM порт"},
	{11,	"ошибка в выходных параметрах"},
	{12,	"ошибка при передаче образа чека"},
	{13,	"ошибка установки связи с устройством"},
	{14,	"ошибка в параметрах настройки интерфейса взаимодействия с пользователем"}
};

PPDRV_INSTANCE_ERRTAB(InpasTrmnl, 1, 0, PPDrvINPASTrmnl, _ErrMsgTab_DC);

int PPDrvINPASTrmnl::Init(SString & rCheck)
{
	int    ok = 1;
	SString msg_ok, buf_ok;    // переменные, необходимые для вывода инф-ии в логи в случае успеха
	int result_dc = 0;         // принимает код ошибки из p_dclink
	int result_sar = 1;        // принимает значение свойства Status из p_res

	ComDispInterface * p_req = 0;
	ComDispInterface * p_res = 0;
	ComDispInterface * p_dclink = new ComDispInterface;
	THROW(p_dclink);
	p_dclink->Init("DualConnector.DCLink");
	AsseptDC(p_dclink);
	p_dclink->CallMethod(InitResources);

	// определение указателей на in-объект и out-объект SAPacket
	p_req = new ComDispInterface;
	p_res = new ComDispInterface;
	THROW(p_dclink);
	p_req->Init("DualConnector.SAPacket");
	p_res->Init("DualConnector.SAPacket");
	AsseptSAP(p_req);
	AsseptSAP(p_res);

	// заданине Необходимых свойств in-объекта SAPacket
	p_req->SetProperty(OperationCode, INPAS_FUNC_INIT);

	// Передача параметров метода Exchange
	p_dclink->SetParam(p_req);
	p_dclink->SetParam(p_res);
	p_dclink->SetParam(TIMEOUT);
	// вызов метода Exchange класса DCLink
	p_dclink->CallMethod(Exchange);

	p_dclink->GetProperty(ErrorCode, &result_dc);
	p_res->GetProperty(Status, &result_sar);

	THROWERR(result_sar == 1, result_dc);									  // Если не 1, значит ошибка. Отправляемся в обработку исключений
	{																		  //   Надо доработать обработку исключений. 
		char slip_ch[1024];                // массив для чека
		p_res->GetProperty(ReceiptData, slip_ch, sizeof(slip_ch));
		rCheck = slip_ch;
	}
	// Если нет ошибок, в логи идет отчет об успешном выполнении операции
	msg_ok.Cat("operation Init completed");
	DRVS.Log(msg_ok, 0xffff);

	// выдает информацию о ошибках в логи
	CATCH
		ok = 0;
		{
			SString msg, buf, rcode;
			int i = SIntToSymbTab_GetSymb(_StatusMsgTab_SAP, SIZEOFARRAY(_StatusMsgTab_SAP), result_sar, rcode);
			if(!i)
				rcode = _StatusMsgTab_SAP[0].P_Symb;
			DRVS.GetErrText(-1, msg);
			DRVS.GetErrText(result_dc, buf);
			msg.Space().Cat("Error Code").Space().Cat(result_dc).CatDiv(':', 2).Cat(buf).Space().Cat(rcode);
			DRVS.Log(msg, 0xffff);
		}
	ENDCATCH;
	// Освобождение ресурсов
	p_req->CallMethod(Release);
	p_res->CallMethod(Release);
	p_dclink->CallMethod(FreeResources);
	if(p_res) {
		p_res->CallMethod(Release);
		delete p_res;
	}
	if(p_req) {
		p_req->CallMethod(Release);
		delete p_req;
	}
	if(p_dclink) {
		p_dclink->CallMethod(FreeResources);
		delete p_dclink;
	}
	return ok;
}

// оплата
int PPDrvINPASTrmnl::Pay(double amount, SString & rSlip)
{
	int    ok = 1;
	SString msg_ok, buf_ok;            // переменные, необходимые для вывода инф-ии в логи в случае успеха
	int result_dc = 0;                 // принимает код ошибки из p_dclink
	int result_sar = 1;	               // принимает значение свойства Status из p_res
	SString temp_buf;
	ComDispInterface * p_req = 0;
	ComDispInterface * p_res = 0;
	ComDispInterface * p_dclink = new ComDispInterface;
	THROW(p_dclink);
	p_dclink->Init("DualConnector.DCLink");
	AsseptDC(p_dclink);
	p_dclink->CallMethod(InitResources);

	// определение указателей на in-объект и out-объект SAPacket
	p_req = new ComDispInterface;
	p_res = new ComDispInterface;
	THROW(p_dclink);
	p_req->Init("DualConnector.SAPacket");
	p_res->Init("DualConnector.SAPacket");
	AsseptSAP(p_req);
	AsseptSAP(p_res);

	// заданине Необходимых свойств in-объекта SAPacket
	temp_buf.Z().Cat((long)R0(amount));
	p_req->SetProperty(Amount, temp_buf);
	p_req->SetProperty(CurrencyCode, RUBLE);
	p_req->SetProperty(OperationCode, INPAS_FUNC_PAY);
	
	// Передача параметров метода Exchange
	p_dclink->SetParam(p_req);
	p_dclink->SetParam(p_res);
	p_dclink->SetParam(TIMEOUT);
	// вызов метода Exchange класса DCLink
	p_dclink->CallMethod(Exchange);
	
	p_dclink->GetProperty(ErrorCode, &result_dc);
	p_res->GetProperty(Status, &result_sar);
																		 // Если не 1 значит ошибка. Отправляемся в обработку исключений
	THROWERR(result_sar == 1, result_dc); 								 //   Надо доработать обработку исключений. 
	{
		char slip_ch[1024]; // массив для чека
		p_res->GetProperty(ReceiptData, slip_ch, sizeof(slip_ch));
		rSlip = slip_ch;
	}
	// @v10.2.0 {
	{
		temp_buf.Z().Cat(getcurdatetime_(), DATF_DMY|DATF_CENTURY, TIMF_HMS);
		SLS.LogMessage(SlipLogFileName, temp_buf, 8192);
		SLS.LogMessage(SlipLogFileName, rSlip, 8192);
	}
	// } @v10.2.0 
	// Если нет ошибок, в логи идет отчет об успешном выполнении операции
	msg_ok.Cat("operation Pay completed");
	DRVS.Log(msg_ok, 0xffff);

	// выдает информацию о ошибках в логи
	CATCH
		{
			SString msg, buf, rcode;
			int i = SIntToSymbTab_GetSymb(_StatusMsgTab_SAP, SIZEOFARRAY(_StatusMsgTab_SAP), result_sar, rcode);
			if(!i)
				rcode = _StatusMsgTab_SAP[0].P_Symb;
			DRVS.GetErrText(-1, msg);
			DRVS.GetErrText(result_dc, buf);
			msg.Space().Cat("Error Code").Space().Cat(result_dc).CatDiv(':', 2).Cat(buf).Space().Cat(rcode);
			DRVS.Log(msg, 0xffff);
		}
		ok = 0;
	ENDCATCH;
	// Освобождение ресурсов
	if(p_res) {
		p_res->CallMethod(Release);
		delete p_res;
	}
	if(p_req) {
		p_req->CallMethod(Release);
		delete p_req;
	}
	if(p_dclink) {
		p_dclink->CallMethod(FreeResources);
		delete p_dclink;
	}
	return ok;
}

// Возврат
int PPDrvINPASTrmnl::Refund(double amount, SString & rSlip)
{
	int    ok = 1;
	SString msg_ok, buf_ok;     // переменные, необходимые для вывода инф-ии в логи в случае успеха
	int    result_dc = 0;          // принимает код ошибки из p_dclink
	int    result_sar = 1;         // принимает значение свойства Status из p_res
	SString temp_buf;
	ComDispInterface * p_req = 0;
	ComDispInterface * p_res = 0;
	ComDispInterface * p_dclink = new ComDispInterface;
	THROW(p_dclink);
	p_dclink->Init("DualConnector.DCLink");
	AsseptDC(p_dclink);
	p_dclink->CallMethod(InitResources);

	// определение указателей на in-объект и out-объект SAPacket
	p_req = new ComDispInterface;
	p_res = new ComDispInterface;
	THROW(p_dclink);
	p_req->Init("DualConnector.SAPacket");
	p_res->Init("DualConnector.SAPacket");
	AsseptSAP(p_req);
	AsseptSAP(p_res);

	// заданине Необходимых свойств in-объекта SAPacket
	temp_buf.Z().Cat((long)R0(amount));
	p_req->SetProperty(Amount, temp_buf);
	p_req->SetProperty(CurrencyCode, RUBLE);
	p_req->SetProperty(OperationCode, INPAS_FUNC_REFUND);

	// Передача параметров метода Exchange
	p_dclink->SetParam(p_req);
	p_dclink->SetParam(p_res);
	p_dclink->SetParam(TIMEOUT);
	// вызов метода Exchange класса DCLink
	p_dclink->CallMethod(Exchange);

	p_dclink->GetProperty(ErrorCode, &result_dc);
	p_res->GetProperty(Status, &result_sar);
	// Если не 1, значит ошибка. Отправляемся в обработку исключений
	THROWERR(result_sar == 1, result_dc); //   Надо доработать обработку исключений. 
	{
		char slip_ch[1024];                // массив для чека
		p_res->GetProperty(ReceiptData, slip_ch, sizeof(slip_ch));
		rSlip = slip_ch;
	}
	// @v10.2.0 {
	{
		temp_buf.Z().Cat(getcurdatetime_(), DATF_DMY|DATF_CENTURY, TIMF_HMS);
		SLS.LogMessage(SlipLogFileName, temp_buf, 8192);
		SLS.LogMessage(SlipLogFileName, rSlip, 8192);
	}
	// } @v10.2.0 
	// Если нет ошибок, в логи идет отчет об успешном выполнении операции
	msg_ok.Cat("operation Refund completed");
	DRVS.Log(msg_ok, 0xffff);
	// выдает информацию о ошибках в логи
	CATCH
		{
			SString msg, buf, rcode;
			int i = SIntToSymbTab_GetSymb(_StatusMsgTab_SAP, SIZEOFARRAY(_StatusMsgTab_SAP), result_sar, rcode);
			if(!i)
				rcode = _StatusMsgTab_SAP[0].P_Symb;
			DRVS.GetErrText(-1, msg);
			DRVS.GetErrText(result_dc, buf);
			msg.Space().Cat("Error Code").Space().Cat(result_dc).CatDiv(':', 2).Cat(buf).Space().Cat(rcode);
			DRVS.Log(msg, 0xffff);
		}
		ok = 0;
	ENDCATCH;
	// Освобождение ресурсов
	if(p_res) {
		p_res->CallMethod(Release);	
		delete p_res;
	}
	if(p_req) {
		p_req->CallMethod(Release);
		delete p_req;
	}
	if(p_dclink) {
		p_dclink->CallMethod(FreeResources);
		delete p_dclink;
	}
	return ok;
}

int PPDrvINPASTrmnl::GetSessReport(SString & rCheck)
{
	int    ok = 1;
	SString msg_ok, buf_ok;    // переменные, необходимые для вывода инф-ии в логи в случае успеха
	int result_dc = 0;         // принимает код ошибки из p_dclink
	int result_sar = 1001;     // принимает значение свойства Status из p_res

	ComDispInterface * p_req = 0;
	ComDispInterface * p_res = 0;
	ComDispInterface * p_dclink = new ComDispInterface;
	THROW(p_dclink);
	p_dclink->Init("DualConnector.DCLink");
	AsseptDC(p_dclink);
	p_dclink->CallMethod(InitResources);
	// определение указателей на in-объект и out-объект SAPacket
	p_req = new ComDispInterface;
	p_res = new ComDispInterface;
	THROW(p_dclink);
	p_req->Init("DualConnector.SAPacket");
	p_res->Init("DualConnector.SAPacket");
	AsseptSAP(p_req);
	AsseptSAP(p_res);
	
	// заданине Необходимых свойств in-объекта SAPacket
	p_req->SetProperty(OperationCode, INPAS_FUNC_CLOSEDAY);

	// Передача параметров метода Exchange
	p_dclink->SetParam(p_req);
	p_dclink->SetParam(p_res);
	p_dclink->SetParam(TIMEOUT);
	// Вызов метода Exchange класса DCLink
	p_dclink->CallMethod(Exchange);
	p_dclink->GetProperty(ErrorCode, &result_dc);
	p_res->GetProperty(Status, &result_sar);
	THROWERR(result_sar == 1, result_dc); // Если не 1, значит ошибка. Отправляемся в обработку исключений  
	{																 //   Надо доработать обработку исключений. 
		char slip_ch[1024];				   // Массив для чека					   
		p_res->GetProperty(ReceiptData, slip_ch, sizeof(slip_ch));
		rCheck = slip_ch;
	}
	// Если нет ошибок, в логи идет отчет об успешном выполнении операции
	msg_ok.Cat("operation GetSessReport completed");
	DRVS.Log(msg_ok, 0xffff);

	// Выдает информацию о ошибках в логи
	CATCH
		{
			SString msg, buf, rcode;
			int i = SIntToSymbTab_GetSymb(_StatusMsgTab_SAP, SIZEOFARRAY(_StatusMsgTab_SAP), result_sar, rcode);
			if(!i)
				rcode = _StatusMsgTab_SAP[0].P_Symb;
			DRVS.GetErrText(-1, msg);
			DRVS.GetErrText(result_dc, buf);
			msg.Space().Cat("Error Code").Space().Cat(result_dc).CatDiv(':', 2).Cat(buf).Space().Cat(rcode);
			DRVS.Log(msg, 0xffff);
		}
		ok = 0;
	ENDCATCH;
	// Освобождение ресурсов
	if(p_res) {
		p_res->CallMethod(Release);
		delete p_res;
	}
	if(p_req) {
		p_req->CallMethod(Release);
		delete p_req;
	}
	if(p_dclink) {
		p_dclink->CallMethod(FreeResources);
		delete p_dclink;
	}
	return ok;
}

// virtual
int PPDrvINPASTrmnl::ProcessCommand(const SString & rCmd, const char * pInputData, SString & rOutput)
{
	int    err = 0;
	SString value;
	PPDrvInputParamBlock pb(pInputData);
	if(rCmd == "INIT") {
		THROW(Init(rOutput));
	}
	else if(rCmd == "PAY") {
		double amount = (pb.Get("AMOUNT", value) > 0) ? value.ToReal() : 0;
		THROW(Pay(amount, rOutput)); // @v10.3.3 @fix THROW
	}
	else if(rCmd == "REFUND") {
		double amount = (pb.Get("AMOUNT", value) > 0) ? value.ToReal() : 0;
		THROW(Refund(amount, rOutput)); // @v10.3.3 @fix THROW
	}
	else if(rCmd == "GETBANKREPORT") {
		// Получаем отчет по операциям за день (грубо говоря, закрытие сессии)
		GetSessReport(rOutput);
	}
	CATCH
		err = 1;
		{
			SString msg_buf;
			DRVS.Log((msg_buf = "Bank Terminal: error").CatDiv(':', 2).Cat(value), 0xffff);
		}
	ENDCATCH;
	return err;
}
