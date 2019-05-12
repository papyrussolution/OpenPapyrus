// BNKSBERBANK.CPP
// @codepage UTF-8
// Библиотека для работы с терминалами, обслуживающими карты Сбербанка
//
#include <ppdrvapi.h>

extern PPDrvSession DRVS;

#define EXPORT	extern "C" __declspec (dllexport)
#define THROWERR(expr,val)     { if(!(expr)) { DRVS.SetErrCode(val); goto __scatch; } }

// Номера операций
#define SBRBNK_FUNC_PAY           1 // Оплата
#define SBRBNK_FUNC_REFUND        3 // Возврат/отмена оплаты
#define SBRBNK_FUNC_CLOSEDAY      7 // Закрытие дня

// Типы карт
#define SBRBNK_CRDTYPE_OPER      0 // Выбор типа осуществляется оператором из меню терминала
#define SBRBNK_CRDTYPE_VISA      1 // Visa
#define SBRBNK_CRDTYPE_MASTERCRD 2 // Eurocard/Mastercard
#define SBRBNK_CRDTYPE_MAESTRO   3 // Cirrus/Maestro
#define SBRBNK_CRDTYPE_INT     101 // Международные карты с микропроцессором
// Могут быть определены дополнительные типы карт

// Коды ошибок банковской библиотеки
#define SBRBNK_ERR_OK        0 // Операция выполнена успешно

// Коды ошибок данной библиотеки
#define SBRBNK_ERR_NOTCONNECTED     6002 // Соединение не установлено
#define SBRBNK_ERR_PAY              6003 // Ошибка операции оплаты
#define SBRBNK_ERR_REFUND           6004 // Ошибка операции возврата
#define SBRBNK_ERR_CLOSEDAY         6005 // Ошибка операции закрытия дня

struct AuthAnswerSt {
	int    TType;        // IN: Тип транзакции
	ulong  Amount_;      // IN: Сумма в копейках
	char   Rcode[3];     // OUT: Код результата авторизации
	char   AMessage[16]; // OUT: Словесное пояснение результата
	int    CType;        // OUT: Тип карты 
	char * P_Check;      // OUT: Образ чека, должен освобождаться GlobalFree в вызывающей программе
};

typedef int (__cdecl * CardAuthProc)(char *, AuthAnswerSt *);
typedef int (__cdecl * CloseDayProc)(struct AuthAnswerSt * pAuthAns);
typedef int (__cdecl * TestPinpadProc)();
//typedef int (__cdecl * GlobalFreeProc) (); // Освобождает поле Check, если оно вернулось непустым

class PPDrvSberTrmnl : public PPBaseDriver {
public:
	PPDrvSberTrmnl() : LogNum(0), P_Lib(0), CardAuth(0), CloseDay(0), TestPinpad(0)
	{
		//GlobalFree = 0;
		SString file_name;
		getExecPath(file_name);
		DRVS.SetLogFileName(file_name.SetLastSlash().Cat("EMULTrmnl.log"));
	}
	~PPDrvSberTrmnl() 
	{ 
		Release(); 
	}
	int    ProcessCommand(const SString & rCmd, const char * pInputData, SString & rOutput);
	int    Init(const char * pLibPath); // Инициализация библиотеки Сбербанка
	int    Connect();
	int    Disconnect();
	int    Release();
	int    Pay(double amount, SString & rSlip);
	int    Refund(double amount, SString & rSlip);
	/*int    GetSessReport(SString & rCheck);*/ // Итоги дня	
private:
	long   LogNum;
	SString SlipFileName;
	SDynLibrary * P_Lib;
	CardAuthProc   CardAuth;
    CloseDayProc   CloseDay;
	TestPinpadProc TestPinpad;
	//GlobalFreeProc GlobalFree;

	class  OpLogBlock {
	public:
		OpLogBlock(const char * pLogFileName, const char * pOp, const char * pExtMsg) : StartClk(clock()), Op(pOp), ExtMsg(pExtMsg) //конструктор. на вход идут 3 строки 
		{																															//+ время с  начала работы программы (в тактах)
			LogFileName = 0/*pLogFileName*/; // @v9.7.1 pLogFileName-->0
			LogFileName = pLogFileName;
			if (LogFileName.NotEmpty() && Op.NotEmpty()) {
				SString line_buf;
				line_buf.Cat(getcurdatetime_(), DATF_DMY | DATF_CENTURY, TIMF_HMS).Tab().Cat(Op).Tab().Cat("start");
				if (ExtMsg.NotEmpty())
					line_buf.Tab().Cat(ExtMsg);
				SLS.LogMessage(LogFileName, line_buf, 8192);
			}
		}
		~OpLogBlock()
		{
			if (LogFileName.NotEmpty() && Op.NotEmpty()) {
				const long end_clk = clock();
				SString line_buf;
				line_buf.Cat(getcurdatetime_(), DATF_DMY | DATF_CENTURY, TIMF_HMS).Tab().Cat(Op).Tab().Cat("finish").Tab().Cat(end_clk - StartClk);
				SLS.LogMessage(LogFileName, line_buf, 8192);
			}
		}
		const long StartClk;
		SString LogFileName; //строки
		SString Op;
		SString ExtMsg;
	};
};

static const SIntToSymbTabEntry _ErrMsgTab[] = {
	{SBRBNK_ERR_NOTCONNECTED,      "Соединение не установлено"},
	{SBRBNK_ERR_PAY,               "Ошибка операции оплаты"},
	{SBRBNK_ERR_REFUND,            "Ошибка операции возврата"},
	{SBRBNK_ERR_CLOSEDAY,          "Ошибка операции закрытия дня"}
};

PPDRV_INSTANCE_ERRTAB(SberTrmnl, 1, 0, PPDrvSberTrmnl, _ErrMsgTab);

int PPDrvSberTrmnl::Init(const char * pLibPath)
{
	OpLogBlock __oplb("Bnk_copy.log", "Init", 0);
	return 1;
}

int PPDrvSberTrmnl::Release() 
{	
	OpLogBlock __oplb("Bnk_copy.log", "Release", 0);
	return 1;
}

// Параметры для подключения смотрит в конфиге
// Чтбы проверить наличие соединения, запросим список терминалов/валют
int PPDrvSberTrmnl::Connect()
{
	OpLogBlock __oplb("Bnk_emul.log", "Connect", 0);
	return 1;
}

int PPDrvSberTrmnl::Disconnect()
{
	OpLogBlock __oplb("Bnk_emul.log", "Disconnect", 0);
	return 1;
}

int PPDrvSberTrmnl::Pay(double amount, SString & rSlip)
{
	OpLogBlock __oplb("Bnk_emul.log", "Pay", 0);
	return 1;

}

// virtual
int PPDrvSberTrmnl::ProcessCommand(const SString & rCmd, const char * pInputData, SString & rOutput)
{
	int    err = 0;
	SString value;
	PPDrvInputParamBlock pb(pInputData);
	if (rCmd == "INIT") {
		SString dll_path = (pb.Get("DLLPATH", value) > 0) ? value : "";
		THROW(Init(dll_path));
	}
	else if (rCmd == "RELEASE") {
		THROW(Release());
	}
	else if (rCmd == "CONNECT") {
		THROW(Connect());
	}
	else if (rCmd == "SETCONFIG") {
		// Запомним логический номер терминала
		LogNum = (pb.Get("LOGNUM", value) > 0) ? value.ToLong() : 0;
	}
	else if (rCmd == "DISCONNECT") {
		THROW(Disconnect());
	}
	else if (rCmd == "PAY") {
		double amount = (pb.Get("AMOUNT", value) > 0) ? value.ToReal() : 0;
		THROW(Pay(amount, rOutput));
	}
	else { // Если дана неизвестная  команда, то сообщаем об этом
		DRVS.SetErrCode(serrInvCommand);
		err = 1;
	}
	CATCH
		err = 1;
	{
		SString msg_buf;
		DRVS.GetErrText(-1, value);
		DRVS.Log((msg_buf = "Bank Terminal: error").CatDiv(':', 2).Cat(value), 0xffff);
	}
	ENDCATCH;
	return err;
}
