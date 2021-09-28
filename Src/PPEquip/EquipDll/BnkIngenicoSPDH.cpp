// Dll для работы с банковским терминалом INGENICO по протоколу SPDH (банк ВТБ)
// @codepage UTF-8 // @v10.4.5
//
#pragma hdrstop
#include <ppdrvapi.h>
#include <slib.h>

extern PPDrvSession DRVS;

#define EXPORT	extern "C" __declspec (dllexport)
#define THROWERR_ADD(expr,val) { if(!(expr)) { SetAddError(val); goto __scatch; } }
#define THROWERR(expr,val)     { if(!(expr)) { DRVS.SetErrCode(val); goto __scatch; } }

//int	ErrorCode = 0;
//int LastError = 0;
int AddError = 0; // Код ошибки, раскрывающий глобальную ошибку из LastError
				// используется при INGVTB_NOTINITED, INGVTB_NOTCONNECTED
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// Коды команд
#define OPER_PAY			1	// ОПЛАТА
#define OPER_REFUND			3	// ВОЗВРАТ
#define OPER_CLOSESESS		11	// ЗАКРЫТИЕ СМЕНЫ
#define OPER_CLOSEDAY		12	// ЗАКРЫТИЕ ДНЯ
#define OPER_TESTCONNECT	96	// ТЕСТ СВЯЗИ

// Коды ошибок
#define INGVTB_NOTENOUGHPARAM	300	// Не достаточно параметров для работы устройства
#define INGVTB_UNCNKOWNCOMMAND	301	// Передана неизвестная команда
#define INGVTB_NOTINITED		302	// Ошибка инициализации
#define INGVTB_NOTCONNECTED		303	// Соединение не установлено

#define	INGVTB_NOTENOUGHMEM		400 // Не достаточный размер выходного буфера
#define INGVTB_DLLFILENOTFOUND	401 // Файл arccom.dll не найден
#define INGVTB_INIFILENOTFOUND	402 // Файл cashreg.ini не найден
#define INGVTB_CHECKFILENOTFOUND	403 // Файл check.out не найден
#define INGVTB_NODLLPATH		404 // Не указан путь к файлу arccom.dll
#define INGVTB_TRANSERR			405 // Ошибка транзакции

/*struct ErrMessage {
	uint Id;
	const char * P_Msg;
};*/

enum {
	lnProcCode = 7,
	lnTrack2 = 60,
	lnPan = 20,
	lnExpiry = 5,
	lnAmount = 13,
	lnCurrency = 4,
	lnTerminalID = 9,
	lnSpdhTerminalID=17,
	lnRetrievalReference = 13,
	lnAuthIdentResponse = 9,
	lnResponseCode = 4,
	lnCardType = 80,
	lnDate = 7,
	lnTime = 7,
	lnBatchNum = 8,
	lnTrack2Credit = 50,
	lnPinblock = 17,
	lnPayData = 50,
	lnPayId = 3,
	lnMtid = 5,
	lnReceivedTextMsg=80,
	lnAID=80,
	lnApplicationLabel=80,
	lnTVR=80
};

#if 0 // {
struct UserAuthInt {
	int handle;
	int abg_id;
	int operType; //[in] Код операции (кассовый)
	char track2[lnTrack2]; //[in] Трек2
	char pan[lnPan]; //[out] PAN
	char expiry[ lnExpiry ]; //[out] Expiry Date ГГММ
	char pay_acc[lnPan]; //не используется
	char additional_payment_data[80]; //не используется
	char amount[ lnAmount ]; //[in] Сумма в копейках
	char original_amount[ lnAmount ]; //[in]Оригинальная сумма в копейках
	char currency[ lnCurrency ]; //[in] Код валюты
	char terminalID[ lnTerminalID ]; //[out] ID терминала
	char rrn[ lnRetrievalReference ]; //[in][out] Ссылка (заполнять для операций, где нужно, в остальных случаях - пусто)
	char authCode[ lnAuthIdentResponse ]; //[in][out] Код авторизации
	char responseCode[ lnResponseCode ]; //[out] Код ответа
	char cardType[lnCardType]; //[out] Название типа карты
	char date[lnDate]; //[out] Дата транзакции
	char time[lnTime]; //[out] Время транзакции
	char payment_data[lnPayData]; //не используется
	char data_to_print[lnPayData]; //не используется
	char home_operator[lnPayData]; //не используется
	char received_text_message[lnReceivedTextMsg]; //не используется
	char text_message[lnReceivedTextMsg]; //[out] Расшифровка
	char AID[lnAID]; //[out]EMV AID
	char ApplicationLabel[lnApplicationLabel]; //[out]EMV ApplicationLabel
	char TVR[lnTVR]; //[out]EMV TVR
	int system_res; //не используется
	// ТОЛЬКО ДЛЯ МОДУЛЯ АРКУС2 С ФУНКЦИОНАЛОМ HRS
	char enc_data[64]; //[in][out] шифрованные данные карты (PAN)
};
#endif

struct UserAuthIntSt {
	UserAuthIntSt()
	{
		THISZERO();
	}
	UserAuthIntSt & Z()
	{
		THISZERO();
		return *this;
	}
	int    handle;
	int    abg_id;
	int    operType; //[in] Код операции (кассовый)
	char   track2[lnTrack2]; //[in] Трек2
	char   pan[lnPan]; //[out] PAN
	char   expiry[lnExpiry]; //[out] Expiry Date ГГММ
	char   pay_acc[lnPan]; //не используется
	char   additional_payment_data[80]; //не используется
	char   amount[lnAmount]; //[in] Сумма в копейках
	char   original_amount[lnAmount]; //[in]Оригинальная сумма в копейках
	char   currency[lnCurrency]; //[in] Код валюты
	char   terminalID[lnTerminalID]; //[out] ID терминала
	char   rrn[lnRetrievalReference]; //[in][out] Ссылка (заполнять только для тех операций, для которых она нужна, в остальных случаях должна быть пуста)
	char   authCode[lnAuthIdentResponse]; //[in][out] Код авторизации
	char   responseCode[lnResponseCode]; //[out] Код ответа
	char   cardType[lnCardType]; //[out] Название типа карты
	char   date[lnDate]; //[out] Дата транзакции
	char   time[lnTime]; //[out] Время транзакции
	char   payment_data[lnPayData]; //не используется
	char   data_to_print[lnPayData]; //не используется
	char   home_operator[lnPayData]; //не используется
	char   received_text_message[lnReceivedTextMsg];//не используется
	char   text_message[lnReceivedTextMsg]; //[out] Расшифровкаа
	char   AID[lnAID]; //[out]EMV AID
	char   ApplicationLabel[lnApplicationLabel]; //[out]EMV ApplicationLabel
	char   TVR[lnTVR]; //[out]EMV TVR
	int    system_res; //не используется
	// ТОЛЬКО ДЛЯ МОДУЛЯ АРКУС2 С ФУНКЦИОНАЛОМ HRS
	char   enc_data[64]; //[in][out] шифрованные данные карты(PAN)
};

typedef int (__cdecl * ProcDll)(UserAuthIntSt *);

class PPDrvIngenicoTrmnl : public PPBaseDriver {
public:
	PPDrvIngenicoTrmnl();
	~PPDrvIngenicoTrmnl();
	//int    RunOneCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize);
	virtual int ProcessCommand(const SString & rCmd, const char * pInputData, SString & rOutput);
	int    Init(const char * pDllPath, SString & rOutput);
	int    Connect();
	int    Disconnect();
	int    SetCfg();
	//int    GetLastErrorText(char * pBuf, size_t bufSize);
	int    Pay(double amount, SString & rSlip); // Оплата
	int    Refund(double amount, SString & rSlip); // Возврат
	int    GetSessReport(SString & rOutput); // Отчет об операциях за день

	SString IniPath; // Путь к файлу ini\cashreg.ini
	SString OutPath; // Путь к файлу check.out
private:
	int    ReadReport(SString & rOutput);
	SDynLibrary * P_Lib;
	ProcDll ProcessOw;
	int Port;
	//SString Amount; // Сумма оплаты/возврата
	SString LogNum;
	SString TransErrStr; // Описание ошибки транзакции
	SString LastStr; // Содержит строку, которая не поместилась в выходной буфер
	//UserAuthIntSt Transaction;
};

//static PPDrvIngenicoTrmnl * P_BnkIngVtb = 0;

//int FASTCALL SetError(int errCode);
//int FASTCALL SetError(char * pErrCode);
int	FASTCALL SetAddError(int errCode);
//int GetLastErrorText(char * pBuf, size_t bufSize);
//int FASTCALL SetError(int errCode) { ErrorCode = errCode; return 1; }
//int FASTCALL SetError(SString & rErrCode) { ErrorCode = rErrCode.ToLong(); return 1; }
int	FASTCALL SetAddError(int errCode) { AddError = errCode; return 1; };
//static int Init(const char * pLibName);
//int Release();

static const /*ErrMessage*/SIntToSymbTabEntry _ErrMsgTab_DC[] = {
	{ INGVTB_NOTENOUGHPARAM,    "Не достаточно параметров для работы устройства" },
	{ INGVTB_UNCNKOWNCOMMAND,	"Передана неизвестная команда" },
	{ INGVTB_NOTINITED,			"Ошибка инициализации" },
	{ INGVTB_NOTCONNECTED,		"Соединение не установлено" },
	{ INGVTB_NOTENOUGHMEM,		"Не достаточный размер выходного буфера" },
	{ INGVTB_DLLFILENOTFOUND,	"Файл arccom.dll не найден" },
	{ INGVTB_INIFILENOTFOUND,	"Файл cashreg.ini не найден" },
	{ INGVTB_CHECKFILENOTFOUND,	"Файл check.out не найден" },
	{ INGVTB_NODLLPATH,			"Не указан путь к файлу arccom.dll" },
	{ INGVTB_TRANSERR,			"Ошибка проведения транзакции" },
	{ 999,                      "Пин-пад не отвечает" }
};

PPDRV_INSTANCE_ERRTAB(InpasTrmnl, 1, 0, PPDrvIngenicoTrmnl, _ErrMsgTab_DC);

/*int PPDrvIngenicoTrmnl::GetLastErrorText(char * pBuf, size_t bufSize)
{
	int  ok = 0;
	SString msg;
	if(LastError == INGVTB_TRANSERR)
		msg.Z().Cat(TransErrStr);
	else {
		SIntToSymbTab_GetSymb(_ErrMsgTab_DC, SIZEOFARRAY(_ErrMsgTab_DC), LastError, msg);
		msg.Transf(CTRANSF_UTF8_TO_OUTER); // @v10.4.5
	}
	if(msg.IsEmpty())
		msg.Cat("Error").Space().Cat(LastError);
	if(AddError) {
		if(AddError == INGVTB_TRANSERR)
			msg.CatDiv(':', 2).Cat(TransErrStr);
		else {
			SString temp_buf;
			msg.CatDiv(':', 2);
			if(SIntToSymbTab_GetSymb(_ErrMsgTab_DC, SIZEOFARRAY(_ErrMsgTab_DC), AddError, temp_buf)) {
				temp_buf.Transf(CTRANSF_UTF8_TO_OUTER); // @v10.4.5
				msg.Cat(temp_buf);
			}
			else
				msg.Cat("Error").Space().Cat(AddError);
		}
	}
	if((msg.Len() + 1) > bufSize) {
		LastStr.Z().Cat(msg);
		ok = 2;
	}
	else {
		memcpy(pBuf, msg, msg.Len() + 1);
		AddError = 0;
	}
	return ok;
}*/

/*int GetLastErrorText(char * pBuf, size_t bufSize)
{
	int  ok = 0;
	SString msg;
	SIntToSymbTab_GetSymb(_ErrMsgTab_DC, SIZEOFARRAY(_ErrMsgTab_DC), LastError, msg);
	if(msg.IsEmpty())
		msg.Cat("Error").Space().Cat(LastError);
	if(AddError) {
		SString temp_buf;
		msg.CatDiv(':', 2);
		if(SIntToSymbTab_GetSymb(_ErrMsgTab_DC, SIZEOFARRAY(_ErrMsgTab_DC), AddError, temp_buf))
			msg.Cat(temp_buf);
		else
			msg.Cat("Error").Space().Cat(AddError);
	}
	if(msg.Len() + 1 > bufSize)
		ok = 2;
	else {
		memcpy(pBuf, msg, msg.Len() + 1);
		AddError = 0;
	}
	return ok;
}*/

//BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) { return true; }
#if 0 // {
EXPORT int RunCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
{
	int    ok = 0;
	THROW(pCmd && pOutputData && outSize);
	if(sstreqi_ascii(pCmd, "INIT")) {
		StringSet pairs(';', pInputData);
		SString s_pair, s_param, param_val;
		for(uint i = 0; pairs.get(&i, s_pair) > 0;){
			s_pair.Divide('=', s_param, param_val);
			if(!s_param.IsEqiAscii("DLLPATH"))
				break;
		}
		THROWERR(Init(param_val), INGVTB_NOTINITED);
	}
	else if(sstreqi_ascii(pCmd, "RELEASE"))
		THROW(Release())
	else if(P_BnkIngVtb)
		ok = P_BnkIngVtb->RunOneCommand(pCmd, pInputData, pOutputData, outSize);
	else if(sstreqi_ascii(pCmd, "GETLASTERRORTEXT")) { // При ошибке иницииализации
		ok = GetLastErrorText(pOutputData, outSize);
	}
	CATCH
		ok = 1;
		_itoa(ErrorCode, pOutputData, 10);
		LastError = ErrorCode;
		ErrorCode = 0;
	ENDCATCH;
	return ok;
}
#endif // } 0

//int PPDrvIngenicoTrmnl::RunOneCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
int PPDrvIngenicoTrmnl::ProcessCommand(const SString & rCmd, const char * pInputData, SString & rOutput)
{
	int    ok = 0;
	int    err = 0;
	SString value;
	PPDrvInputParamBlock pb(pInputData);
	SString s_pair, s_param, param_val;
	StringSet pairs(';', pInputData);
	if(rCmd == "GETLASTERRORTEXT") { 
		rOutput = TransErrStr;
		ok = 1;
	}
	else {
		TransErrStr.Z();
		if(rCmd == "INIT") {
			pb.Get("DLLPATH", value);
			THROW(Init(value, rOutput));
		}
		else if(rCmd == "RELEASE") {
			ProcessOw = 0;
			ZDELETE(P_Lib);
		}
		else if(rCmd == "CONNECT") {
			for(uint i = 0; pairs.get(&i, s_pair) > 0;) {
				s_pair.Divide('=', s_param, param_val);
				if(s_param.IsEqiAscii("PORT"))
					Port = param_val.ToLong();
			}
			THROWERR(Connect(), INGVTB_NOTCONNECTED);
		}
		else if(rCmd == "DISCONNECT") {
			THROW(Disconnect())
		}
		else if(rCmd == "SETCONFIG") {
			for(uint i = 0; pairs.get(&i, s_pair) > 0;) {
				s_pair.Divide('=', s_param, param_val);
				if(s_param.IsEqiAscii("LOGNUM"))
					LogNum = param_val;
			}
			THROW(SetCfg());
		}	
		else if(rCmd == "GETCONFIG") {
			ok = 0;
		}
		else if(rCmd == "PAY") {
			/*for(uint i = 0; pairs.get(&i, s_pair) > 0;) {
				s_pair.Divide('=', s_param, param_val);
				if(s_param.IsEqiAscii("AMOUNT"))
					Amount = param_val;
			}*/
			double amount = (pb.Get("AMOUNT", value) > 0) ? value.ToReal() : 0.0;
			THROW(Pay(amount, rOutput));
		}
		else if(rCmd == "REFUND") {
			/*for(uint i = 0; pairs.get(&i, s_pair) > 0;) {
				s_pair.Divide('=', s_param, param_val);
				if(s_param.IsEqiAscii("AMOUNT"))
					Amount = param_val;
			}*/
			double amount = (pb.Get("AMOUNT", value) > 0) ? value.ToReal() : 0.0;
			THROW(Refund(amount, rOutput));
		}
		else if(rCmd == "GETBANKREPORT") {
			//THROW((ok = GetSessReport(pOutputData, outSize)) != 1);
			GetSessReport(rOutput);
		}
		else { // Если дана неизвестная  команда, то сообщаем об этом
			THROWERR(0, INGVTB_UNCNKOWNCOMMAND);
		}
	}
	CATCH
		ok = 1;
		if(TransErrStr.NotEmpty())
			rOutput = TransErrStr;
		/*else
			rOutput.Z().Cat(ErrorCode);*/
		//_itoa(ErrorCode, pOutputData, 10);
		//LastError = ErrorCode;
	ENDCATCH;
	//ErrorCode = 0;
	return ok;
}

int PPDrvIngenicoTrmnl::Init(const char * pLibName, SString & rOutput)
{
	int    ok = 1;
	SPathStruc path_struct;
	THROWERR_ADD(!isempty(pLibName), INGVTB_NODLLPATH);
	P_Lib = new SDynLibrary(pLibName);
	THROWERR_ADD(P_Lib && P_Lib->IsValid(), INGVTB_DLLFILENOTFOUND);
	ProcessOw = reinterpret_cast<ProcDll>(P_Lib->GetProcAddr("ProcessOw"));
	THROWERR_ADD(P_Lib && ProcessOw, INGVTB_DLLFILENOTFOUND);
	//THROW(SETIFZ(P_BnkIngVtb, new PPDrvIngenicoTrmnl));
	path_struct.Split(pLibName);
	path_struct.Dir.ReplaceStr("DLL\\", "INI\\", 1);
	IniPath.Z().Cat(path_struct.Drv).Colon().Cat(path_struct.Dir).Cat("cashreg.ini");
	{
		size_t pos = 0;
		int exit = 0;
		SString r_str;
		SFile file(IniPath, SFile::mRead);
		THROWERR_ADD(file.IsValid(), INGVTB_INIFILENOTFOUND);
		while(!exit && (file.ReadLine(r_str) > 0)) {
			if(r_str.Search("CHEQ_FILE=", 0, 1, &pos)) {
				r_str.Excise(0, pos + strlen("CHEQ_FILE="));
				r_str.TrimRightChr('\n');
				exit = 1;
			}
		}
		path_struct.Dir.ReplaceStr("INI\\", "\\", 1);
		OutPath.Z().Cat(path_struct.Drv).Colon().Cat(path_struct.Dir).Cat(/*"cheq.out"*/r_str);
	}
	CATCH
		ZDELETE(P_Lib);
		ok = 0;
	ENDCATCH;
	return ok;
}

PPDrvIngenicoTrmnl::PPDrvIngenicoTrmnl() : P_Lib(0), ProcessOw(0), Port(0)
{
	//LastError = 0;
	IniPath.Z();
	OutPath.Z();
	LogNum.Z();
	//Transaction.Clear();
}

PPDrvIngenicoTrmnl::~PPDrvIngenicoTrmnl()
{
}

int PPDrvIngenicoTrmnl::Connect()
{
	int    ok = 1;
	int    exit = 0;
	int    result = 0;
	size_t pos = 0;
	int    tell = 0;
	//SString r_str, w_str;
	// Меняем в файле настроек номер порта
	/*SFile file(IniPath, SFile::mRead);
	THROWERR_ADD(file.IsValid(), INGVTB_INIFILENOTFOUND);
	while(!exit && (file.ReadLine(r_str) > 0)) {
		if(r_str.Search("PORT=", 0, 1, &pos)) {
			tell = file.Tell();
			file.Close();
			file.Open(IniPath, SFile::mReadWrite);
			THROWERR_ADD(file.IsValid(), INGVTB_INIFILENOTFOUND);
			GetComDvcSymb(comdvcsCom, Port + 1, 0, port_name, SIZEOFARRAY(port_name));
			w_str.CopyFrom(r_str);
			w_str.Excise(pos + 5, w_str.Len()).Cat(port_name);
			file.Seek(tell - r_str.Len() - 1);
			file.WriteLine(w_str);
			file.Close();
			exit = 1;
		}
	}*/
	UserAuthIntSt ta_;
	ta_.operType = OPER_TESTCONNECT;
	result = ProcessOw(&ta_);
	//r_str.Z().Cat(ta_.responseCode);
	//r_str.Z().Cat(ta_.text_message);
	//r_str.Z().Cat(ta_.rrn);
	//r_str.Z().Cat(ta_.authCode);
	TransErrStr.Z().Cat(ta_.text_message);
	THROWERR_ADD(result == 0, INGVTB_TRANSERR);
	CATCHZOK
	return ok;
}

int PPDrvIngenicoTrmnl::Disconnect()
{
	return 1;
}

int PPDrvIngenicoTrmnl::SetCfg()
{
	return 1;
}

int PPDrvIngenicoTrmnl::Pay(double amount, SString & rSlip)
{
	rSlip.Z();
	int    ok = 1;
	int    result = 0;
	SString temp_buf;
	UserAuthIntSt ta_;
	// @v11.0.9 STRNSCPY(ta_.terminalID, LogNum);
	ta_.operType = OPER_PAY;
	STRNSCPY(ta_.currency, "643"); // тип валюты: рубли // @v11.0.9 "810"-->"643"
	temp_buf.Cat(amount, MKSFMTD(0, 0, 0));
	STRNSCPY(ta_.amount, temp_buf);
	result = ProcessOw(&ta_);
	//temp_buf.Z().Cat(ta_.responseCode);
	//temp_buf.Z().Cat(ta_.text_message);
	//temp_buf.Z().Cat(ta_.currency);
	//temp_buf.Z().Cat(ta_.amount);
	//temp_buf.Z().Cat(ta_.rrn);
	//temp_buf.Z().Cat(ta_.authCode);
	TransErrStr.Z().Cat(ta_.text_message);
	THROWERR(result == 0, INGVTB_TRANSERR);
	ReadReport(rSlip);
	CATCHZOK
	return ok;
}
	
int PPDrvIngenicoTrmnl::Refund(double amount, SString & rSlip)
{
	rSlip.Z();
	int    ok = 1;
	int    result = 0;
	SString temp_buf;
	UserAuthIntSt ta_;
	// @v11.0.9 STRNSCPY(ta_.terminalID, LogNum);
	ta_.operType = OPER_REFUND;
	STRNSCPY(ta_.currency, "643"); // тип валюты: рубли // @v11.0.9 "810"-->"643"
	temp_buf.Cat(amount, MKSFMTD(0, 0, 0));
	STRNSCPY(ta_.amount, temp_buf);
	result = ProcessOw(&ta_);
	//temp_buf.Z().Cat(ta_.responseCode);
	//temp_buf.Z().Cat(ta_.text_message);
	//temp_buf.Z().Cat(ta_.currency);
	//temp_buf.Z().Cat(ta_.amount);
	//temp_buf.Z().Cat(ta_.rrn);
	//temp_buf.Z().Cat(ta_.authCode);
	TransErrStr.Z().Cat(ta_.text_message);
	THROWERR(result == 0, INGVTB_TRANSERR);
	ReadReport(rSlip);
	CATCHZOK
	return ok;
}

int PPDrvIngenicoTrmnl::ReadReport(SString & rOutput)
{
	rOutput.Z();
	int    ok = 1;

	int64  file_size = 0;
	SString temp_buf;
	SFile file(OutPath, SFile::mRead);
	THROWERR(file.IsValid(), INGVTB_CHECKFILENOTFOUND);
	file.CalcSize(&file_size);
	while(file.ReadLine(temp_buf) > 0) {
		temp_buf.Chomp();
		for(size_t i = 0; i < temp_buf.Len(); i++) {
			if(temp_buf.C(i) > 0 && temp_buf.C(i) < 32) // Отсекаем управляющие символы, кроме конца строки, пробела, перевода строки
				if(temp_buf.C(i) != '\n')
					temp_buf.Excise(i, 1);
		}
		rOutput.Cat(temp_buf).CR();
	}
	CATCHZOK
	return ok;
}

int PPDrvIngenicoTrmnl::GetSessReport(SString & rOutput)
{
	rOutput.Z();
	int    ok = 1;
	UserAuthIntSt ta_;
	// @v11.0.9 STRNSCPY(ta_.terminalID, LogNum);
	ta_.operType = OPER_CLOSEDAY;
	int    result = ProcessOw(&ta_);
	//temp_buf.Z().Cat(ta_.responseCode);
	//temp_buf.Z().Cat(ta_.text_message);
	//temp_buf.Z().Cat(ta_.rrn);
	//temp_buf.Z().Cat(ta_.authCode);
	TransErrStr.Z().Cat(ta_.text_message);
	THROWERR(result == 0, INGVTB_TRANSERR);
	ReadReport(rOutput);
	CATCHZOK
	return ok;
}


