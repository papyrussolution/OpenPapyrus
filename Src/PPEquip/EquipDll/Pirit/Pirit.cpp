// Dll для работы с ККМ Пирит
//
#pragma hdrstop
#include <slib.h>

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch(dwReason) {
		case DLL_PROCESS_ATTACH:
			{
				SString product_name = "Papyrus-Drv-Pirit";
				SLS.Init(product_name, (HINSTANCE)hModule);
			}
			break;
		case DLL_THREAD_ATTACH:
			SLS.InitThread();
			break;
		case DLL_THREAD_DETACH:
			SLS.ReleaseThread();
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

#define EXPORT	extern "C" __declspec (dllexport)
#define THROWERR(expr,val)     {if(!(expr)){SetError(val);goto __scatch;}}

//typedef unsigned char  uint8;

int	ErrorCode = 0;
char FS = 0x1C;
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// Для формирования пакета данных
#define	STX		0x02	// Начало пакета
#define	ETX		0x03	// Конец пакета

// Коды команд ККМ
#define ENQ	0x05	// Проверка связи
#define ACK	0x06	// ККМ на связи
#define CAN	0x18	// Прервать выполнение отчета

// Коды ошибок
#define PIRIT_ERRSTATUSFORFUNC	  1 // 01h Функция невыполнима при данном статусе ККМ
#define PIRIT_ERRFUNCNUMINCOMMAND 2 // 02h В команде указан неверный номер функции
#define PIRIT_ERRCMDPARAMORFORMAT 3 // 03h Некорректный формат или параметр команды
#define PIRIT_PRNNOTREADY		  9 // 09h Принтер не готов
#define PIRIT_DIFDATE			 11	// Дата и время на ККМ отличаются от системных на 8 минут
#define PIRIT_DATELSLASTOP		 12	// Системная дата меньше даты последней фискальной операции, зарегистрированной в ККМ
#define PIRIT_FATALERROR		 32 // 20h Фатальная ошибка ККМ
#define PIRIT_FMOVERFLOW		 33 // 21h Нет свободного места в фискальной памяти ККМ
#define PIRIT_OFDPROVIDER        54 // Ошибка оператора фискальных данных
#define PIRIT_ECRRFORMAT		 65 // 41h Некорректный формат или параметр команды ЭКЛЗ
#define PIRIT_ECRACCIDENT		 67 // 43h Авария ЭКЛЗ
#define PIRIT_KCACCIDENT		 68 // 44h Авария КС (криптографического сопроцессора)в составе ЭКЛЗ
#define PIRIT_ECRTIMEOUT		 69 // 45h Исчерпан временной ресурс использования ЭКЛЗ
#define PIRIT_ECROVERFLOW		 70 // 46h ЭКЛЗ переполнена
#define PIRIT_ECRERRORDATETIME	 71 // 47h Неверные дата или время
#define PIRIT_ECRNODATA			 72 // 48h Нет запрошенных данных
#define PIRIT_ECRTOOMUCH		 73 // 49h Переполнение (отрицательный итог документа, слишком много отделов для клиента)
#define PIRIT_NOANSWER			 74 // 4Ah Нет ответа от ЭКЛЗ
#define PIRIT_ECRERREXCHANGE	 75 // 4Bh Ошибка при обмене данными с ЭКЛЗ

#define PIRIT_NOTENOUGHPARAM	300	// Не достаточно параметров для работы устройства
#define PIRIT_UNCNKOWNCOMMAND	301	// Передана неизвестная команда
#define PIRIT_NOTINITED			302	// Ошибка инициализации
#define PIRIT_NOTCONNECTED		303	// Соединение не установлено

#define PIRIT_ECRERRORSTATUS	401	// Некорректное состояние ЭКЛЗ
#define PIRIT_ECRFMOVERFLOW		402	// ЭКЛЗ или ФП переполнена
#define PIRIT_ECRFATALERR		403	// Ошибка ЭКЛЗ. Обратитесь в ЦТО

#define PIRIT_NOTSENT			500	// Ошибка передачи данных
#define PIRIT_NOTENOUGHMEM		501	// Недостаточный размер выходного массива
#define PIRIT_ERRLOGOSIZE		502	// Изображение должно иметь размеры: ширина - не более 576 точек, высота - 126 точек
#define PIRIT_ERRLOGOFORMAT		503	// Изображение должно быть монохромным в формате BMP
#define PIRIT_ECRARCHOPENED		504 // Архив ЭКЛЗ закрыт
#define PIRIT_ECRNOTACTIVE		505 // ЭКЛЗ не активирована
#define PIRIT_NOTENOUGHTMEMFORSESSCLOSE	506 // Нет памяти для закрытия смены в ФП
#define PIRIT_ERRFMPASS			507 // Был введен неверный пароль доступа к ФП
#define PIRIT_SESSOPENEDTRYAGAIN	508 // Не было завершено закрытие смены, необходимо повторить операцию
#define PIRIT_PRNTROPENED		509 // 2 Открыта крышка принтера
#define PIRIT_PRNCUTERR			510 // 3 Ошибка резчика принтера
#define PIRIT_NOCONCTNWITHPRNTR	511 // 7 Нет связи с принтером
//
// Значения флагов статуса
//
#define NOPRINT					0x00	// Нет печати
#define NOPAPER					0x01	// В принтере нет бумаги
#define PRINTAFTERNOPAPER		0x02	// Печатать после окончания ленты
#define PRINT					0x04	// Печать
#define CHECKOPENED				0x08	// Открыт чек
#define CHECKCLOSED				0x10	// Чек закрыт

// Описание статусов // new
#define CHECK_OPENED_STR		"Чек открыт"
#define CHECK_CLOSED_STR		"Чек закрыт"

struct ErrMessage {
	uint Id;
	const char * P_Msg;
};

struct Config
{
	Config() : CashID(0), Name(0), LogNum(0), Port(0), BaudRate(0), DateTm(MAXDATETIME), Flags(0), ConnPass("PIRI")
	{
		ReadCycleCount = 10; // @v9.6.9 0-->10
		ReadCycleDelay = 10; // @v9.6.9 0-->10
	};
	struct LogoStruct
	{
		LogoStruct() : Height(0), Width(0), Size(0), Print(0)
		{
		}
		SString Path;
		uint Height;
		uint Width;
		size_t Size;
		int Print;
	};
	int    CashID;
	char * Name;
	uint   LogNum;
	int    Port;
	int    BaudRate;
	LDATETIME DateTm;
	long   Flags;
	char * ConnPass; // Пароль для связи
	int    ReadCycleCount;
	int    ReadCycleDelay;
	LogoStruct Logo;
};

struct CheckStruct {
	CheckStruct() : CheckType(2), FontSize(3), CheckNum(0), Quantity(0.0), Price(0.0), Department(0), Tax(0), PaymCash(0.0), PaymCard(0.0), IncassAmt(0.0)
	{
	}
	void Clear()
	{
		FontSize = 3;
		Quantity = 0.0;
		Price = 0.0;
		Department = 0;
		Tax = 0;
		// @v9.9.4 if(Text.NotEmpty())
		// @v9.9.4 	Text.Destroy();
		Text.Z(); // @v9.9.4
		Code.Z(); // @v9.9.4
		PaymCash = 0.0;
		PaymCard = 0.0;
		IncassAmt = 0.0;
	}
	int    CheckType;
	int    FontSize;
	int    CheckNum;
	double Quantity;
	double Price;
	int    Department;
	int    Tax;
	SString Text;
	SString Code; // @v9.5.7
	double PaymCash;
	double PaymCard;
	double IncassAmt;
};

class PiritEquip {
public:
	SLAPI  PiritEquip() : SessID(0), LastError(0), FatalFlags(0), LastStatus(0)
	{
		Check.Clear();
		{
			SString exe_file_name = SLS.GetExePath();
			if(exe_file_name.NotEmptyS()) {
				SPathStruc ps;
				ps.Split(exe_file_name);
				ps.Nam = "pirit";
				ps.Ext = "log";
				ps.Merge(LogFileName);
			}
		}
	}
	SLAPI ~PiritEquip()
	{
	}
	int    RunOneCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize);
	int    SetConnection();
	int    CloseConnection();
	int    SetCfg();
	SString & LastErrorText(SString & rMsg);
	int    NotEnoughBuf(SString & rStr);
	//
	// Перед началом работы с ККМ надо задать время и дату. Если время и дата на ККМ
	// отличаются от переданных, то возвращается сообщение об ошибке.
	//
	int    StartWork();
	// Получаем текущие флаги ККМ
	int    GetCurFlags(int numFlags, int & rFlags);
	int    RunCheck(int opertype);
	int    ReturnCheckParam(SString & rInput, char * output, size_t size);
	int    PutData(const char * pCommand, const char * pData);
	int    GetData(SString & rData, SString & rError);
	//
	// Для получения ответа при выполнении длинных операций (аннулирование, открытие, закрытие, внесение/изъятие наличности, открыть ящик)
	//
	int    GetWhile(SString & rOutData, SString & rError); 
	void   GetGlobalErrCode(); // Сделано для ошибок ЭКЛЗ
	int    OpenBox();
	int    GetStatus(SString & rStatus); // Возвращает статус ККМ (состояние принтера, статус документа)

	int    SessID;
	int    LastError;
	int    FatalFlags;		// Флаги фатального сотояния. Нужно для возвращения значения в сообщении об ошибке.
	int    LastStatus;     // Последний статус ККМ или документа
	SString OrgAddr;	// Адрес организации
	SString CshrName;	// Имя кассира
	SString LastStr;	// Содержит строку, которая не поместилась в выходной буфер
	SString LastCmd;	// Последняя команда
	SString LastParams;	// Параметры последней команды
	SString CashDateTime; // Записываем дату/время ККМ при возникновении ошибки PIRIT_DATELSLASTOP, дабы вывести ее в сообщении.
	SCommPort CommPort;
	Config Cfg;
	CheckStruct Check;
private:
	class  OpLogBlock {
	public:
		OpLogBlock(const char * pLogFileName, const char * pOp, const char * pExtMsg) : StartClk(clock()), Op(pOp), ExtMsg(pExtMsg)
		{
			LogFileName = 0/*pLogFileName*/; // @v9.7.1 pLogFileName-->0
			//LogFileName = pLogFileName;
			if(LogFileName.NotEmpty() && Op.NotEmpty()) {
				SString line_buf;
				line_buf.Cat(getcurdatetime_(), DATF_DMY|DATF_CENTURY, TIMF_HMS).Tab().Cat(Op).Tab().Cat("start");
				if(ExtMsg.NotEmpty())
					line_buf.Tab().Cat(ExtMsg);
				SLS.LogMessage(LogFileName, line_buf, 8192);
			}
		}
		~OpLogBlock()
		{
			if(LogFileName.NotEmpty() && Op.NotEmpty()) {
				const long end_clk = clock();
				SString line_buf;
				line_buf.Cat(getcurdatetime_(), DATF_DMY|DATF_CENTURY, TIMF_HMS).Tab().Cat(Op).Tab().Cat("finish").Tab().Cat(end_clk-StartClk);
				SLS.LogMessage(LogFileName, line_buf, 8192);
			}
		}
		const long StartClk;
		SString LogFileName;
		SString Op;
		SString ExtMsg;
	};
	//
	// Налоговой ставки, установленная в аппарате
	//
	struct DvcTaxEntry {
		DvcTaxEntry()
		{
			THISZERO();
		}
		char   Name[64];
		double Rate;
	};
	DvcTaxEntry DvcTaxArray[8];

	int    FormatPaym(double paym, SString & rStr);
	int    SetLogotype(SString & rPath, size_t size, uint height, uint width);
	int    PrintLogo(int print);
	int    GetDateTime(SYSTEMTIME sysDtTm, SString & rDateTime, int dt); // dt = 0 - возвращает форматировнную дату, dt = 1 - время //
	//
	// Descr: Получает таблицу налогов из настроек аппарата (DvcTaxArray)
	//
	int    GetTaxTab();
	void   GetLastCmdName(SString & rName); // new
	void   SetLastItems(const char * pCmd, const char * pParam);

	SString LogFileName;
};

static PiritEquip * P_Pirit = 0;

ErrMessage ErrMsg[] = {
	{PIRIT_ERRSTATUSFORFUNC, "Функция невыполнима при данном статусе ККМ"},
	{PIRIT_ERRFUNCNUMINCOMMAND, "В команде указан неверный номер функции"},
	{PIRIT_ERRCMDPARAMORFORMAT, "Некорректный формат или параметр команды"},
	{PIRIT_PRNNOTREADY,		 "Принтер не готов"},
	{PIRIT_DIFDATE,			 "Дата и время на ККМ отличаются от системных на 8 минут. Проверьте время на компьютере"},
	{PIRIT_DATELSLASTOP,	 "Системная дата меньше даты последней фискальной операции, зарегистрированной в ККМ. Проверьте время на компьютере. Если оно верно, обратитесь в ЦТО."},
	{PIRIT_FATALERROR,		 "Фатальная ошибка ККМ"},
	{PIRIT_FMOVERFLOW,		 "Нет свободного места в фискальной памяти ККМ"},
	{PIRIT_OFDPROVIDER,      "Ошибка оператора фискальных данных"}, // @v9.6.4
	{PIRIT_ECRRFORMAT,		 "Некорректный формат или параметр команды ЭКЛЗ"},
	{PIRIT_ECRACCIDENT,		 "Авария ЭКЛЗ"},
	{PIRIT_KCACCIDENT,		 "Авария КС (криптографического сопроцессора)в составе ЭКЛЗ"},
	{PIRIT_ECRTIMEOUT,		 "Исчерпан временной ресурс использования ЭКЛЗ"},
	{PIRIT_ECROVERFLOW,		 "ЭКЛЗ переполнена"},
	{PIRIT_ECRERRORDATETIME, "Неверные дата или время"},
	{PIRIT_ECRNODATA,		 "Нет запрошенных данных"},
	{PIRIT_ECRTOOMUCH,		 "Переполнение (отрицательный итог документа, слишком много отделов для клиента)"},
	{PIRIT_NOANSWER,		 "Нет ответа от ЭКЛЗ"},
	{PIRIT_ECRERREXCHANGE,	 "Ошибка при обмене данными с ЭКЛЗ"},
	{PIRIT_NOTENOUGHPARAM,	 "Не достаточно параметров для работы устройства"},
	{PIRIT_UNCNKOWNCOMMAND,	 "Передана неизвестная команда"},
	{PIRIT_NOTINITED,		 "Ошибка инициализации"},
	{PIRIT_NOTCONNECTED,	 "Соединение не установлено"},
	{PIRIT_ECRERRORSTATUS,	 "Некорректное состояние ЭКЛЗ"},
	{PIRIT_ECRFMOVERFLOW,	 "ЭКЛЗ или ФП переполнена"},
	{PIRIT_ECRFATALERR,		 "Ошибка ЭКЛЗ. Обратитесь в ЦТО"},
	{PIRIT_NOTSENT,			 "Ошибка передачи данных"},
	{PIRIT_NOTENOUGHMEM,	 "Недостаточный размер выходного массива"},
	{PIRIT_ERRLOGOSIZE,		 "Логотип должен иметь размеры: ширина - не более 576 точек, высота - 126 точек"},
	{PIRIT_ERRLOGOFORMAT,	 "Логотип должен быть монохромным в формате BMP"},
	{PIRIT_ECRARCHOPENED,	 "Архив ЭКЛЗ закрыт"},
	{PIRIT_ECRNOTACTIVE,	 "ЭКЛЗ не активирована"},
	{PIRIT_NOTENOUGHTMEMFORSESSCLOSE,	 "Нет памяти для закрытия смены в ФП"},
	{PIRIT_ERRFMPASS,		 "Был введен неверный пароль доступа к ФП"},
	{PIRIT_SESSOPENEDTRYAGAIN,	 "Не было завершено закрытие смены, необходимо повторить операцию"},
	{PIRIT_PRNTROPENED,		 "Открыта крышка принтера"},
	{PIRIT_PRNCUTERR,		 "Ошибка резчика принтера"},
	{PIRIT_NOCONCTNWITHPRNTR,	 "Нет связи с принтером"}
};

int	FASTCALL SetError(int errCode);
int	FASTCALL SetError(char * pErrCode);
int FASTCALL SetError(int errCode) 
{ 
	ErrorCode = errCode; 
	return 1; 
}

int FASTCALL SetError(SString & rErrCode) 
{ 
	ErrorCode = rErrCode.ToLong(); 
	return 1; 
}

static int Init();
int Release();

#define	FS_STR	"\x1C"	// Символ-разделитель пар параметров

static void FASTCALL CreateStr(const char * pValue, SString & dst) { dst.Cat(pValue).Cat(FS_STR); }
static void FASTCALL CreateStr(int value, SString & dst) { dst.Cat(value).Cat(FS_STR); }
static void FASTCALL CreateStr(int64 value, SString & dst) { dst.Cat(value).Cat(FS_STR); }
static void FASTCALL CreateStr(double value, SString & dst) { dst.Cat(value).Cat(FS_STR); }

EXPORT int /*STDAPICALLTYPE*/ RunCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
{
	int    ok = 0;
	THROW(pCmd && pOutputData && outSize);
	if(sstreqi_ascii(pCmd, "INIT")) {
		THROWERR(Init(), PIRIT_NOTINITED);
	}
	else if(sstreqi_ascii(pCmd, "RELEASE")) {
		THROW(Release());
	}
	else if(P_Pirit)
		ok = P_Pirit->RunOneCommand(pCmd, pInputData, pOutputData, outSize);
	CATCH
		ok = 1;
		_itoa(ErrorCode, pOutputData, 10);
		if(P_Pirit)
			P_Pirit->LastError = ErrorCode;
		ErrorCode = 0;
	ENDCATCH;
	return ok;
}

void PiritEquip::SetLastItems(const char * pCmd, const char * pParam)
{
	LastCmd = pCmd;
	LastParams = pParam;
}

int PiritEquip::RunOneCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
{
	int    ok = 0, val = 0;
	SString s_pair;
	SString s_param;
	SString param_val;
	SString str;
	SString out_data;
	SString r_error;
	SString cmd;
	SString params;
	if(sstreqi_ascii(pCmd, "CONTINUEPRINT")) {
		if((LastCmd.CmpNC("PRINTFISCAL") != 0) && (LastCmd.CmpNC("PRINTTEXT") != 0)) { // new
			cmd = LastCmd;
			params = LastParams;
			LastCmd.Z();
			LastParams.Z();
		}
	}
	else {
		cmd = pCmd;
		params = pInputData;
	}
	StringSet pairs(';', params);
	if(LastError == PIRIT_NOTENOUGHMEM) {
		strnzcpy(pOutputData, LastStr, outSize);
		LastError = 0;
	}
	else { // if(LastError != NOTENOUGHMEM)
		if(cmd.IsEqiAscii("CONNECT")){
			SetLastItems(0, 0);
			for(uint i = 0; pairs.get(&i, s_pair) > 0;){
				s_pair.Divide('=', s_param, param_val);
				if(s_param.IsEqiAscii("PORT"))
					Cfg.Port = param_val.ToLong();
				else if(s_param.IsEqiAscii("BAUDRATE"))
					Cfg.BaudRate = param_val.ToLong();
			}
			int    flag = 0;
			THROWERR(SetConnection(), PIRIT_NOTCONNECTED);
			THROW(GetCurFlags(2, flag));
			THROWERR(!(flag & 0x10), PIRIT_ECRARCHOPENED); // Архив ЭКЛЗ закрыт
			THROWERR(!(flag & 0x20), PIRIT_ECRNOTACTIVE); // ЭКЛЗ не активирована
			THROWERR(!(flag & 0x80), PIRIT_ERRFMPASS); // Был введен неверный пароль доступа к ФП
			THROWERR(!(flag & 0x100), PIRIT_SESSOPENEDTRYAGAIN); // Не было завершено закрытие смены, необходимо повторить операцию
			GetTaxTab(); // @v9.7.1
		}
		else if(cmd.IsEqiAscii("CHECKSESSOVER")){
			SetLastItems(cmd, pInputData);
			int    flag = 0;
			THROW(GetCurFlags(2, flag));
			strcpy(pOutputData, (flag & 0x8) ? "1" : "0");
		}
		else if(cmd.IsEqiAscii("DISCONNECT")) {
			SetLastItems(0, 0);
			THROW(CloseConnection())
		}
		else if(cmd.IsEqiAscii("SETCONFIG")) {
			SetLastItems(0, 0);
			THROW(StartWork());
			for(uint i = 0; pairs.get(&i, s_pair) > 0;){
				s_pair.Divide('=', s_param, param_val);
				if(s_param.IsEqiAscii("LOGNUM"))
					Cfg.LogNum = param_val.ToLong();
				else if(s_param.IsEqiAscii("FLAGS"))
					Cfg.Flags = param_val.ToLong();
				else if(s_param.IsEqiAscii("CSHRNAME"))
					CshrName = param_val;
				else if(s_param.IsEqiAscii("PRINTLOGO"))
					Cfg.Logo.Print = param_val.ToLong();
			}
			THROW(SetCfg());
		}
		else if(cmd.IsEqiAscii("SETLOGOTYPE")) {
			SetLastItems(0, 0);
			THROW(StartWork());
			for(uint i = 0; pairs.get(&i, s_pair) > 0;){
				s_pair.Divide('=', s_param, param_val);
				if(s_param.IsEqiAscii("LOGOTYPE"))
					Cfg.Logo.Path = param_val;
				else if(s_param.IsEqiAscii("LOGOSIZE"))
					Cfg.Logo.Size = param_val.ToLong();
				else if(s_param.IsEqiAscii("LOGOHEIGHT"))
					Cfg.Logo.Height = param_val.ToLong();
				else if(s_param.IsEqiAscii("LOGOWIDTH"))
					Cfg.Logo.Width = param_val.ToLong();
			}
			THROW(SetLogotype(Cfg.Logo.Path, Cfg.Logo.Size, Cfg.Logo.Height, Cfg.Logo.Width));
		}
		else if(cmd.IsEqiAscii("GETCONFIG")) {
			SetLastItems(0, 0);
			str.Z().CatEq("CHECKSTRLEN", "130"); // @v9.1.8 44-->130
			if(outSize < str.BufSize()){
				NotEnoughBuf(str);
				memcpy(pOutputData, str, outSize);
				ok = 2;
			}
			else
				memcpy(pOutputData, str, str.BufSize());
		}
		else if(cmd.IsEqiAscii("ZREPORT")) {
			SetLastItems(cmd, pInputData);
			THROW(StartWork());
			CreateStr(CshrName, str);
			int    flag = 0;
			THROW(GetCurFlags(2, flag));
			THROWERR(!(flag & 0x40), PIRIT_NOTENOUGHTMEMFORSESSCLOSE); // Нет памяти для закрытия смены в ФП
			{
				OpLogBlock __oplb(LogFileName, "21", 0);
				THROWERR(PutData("21", str), PIRIT_NOTSENT);
				THROW(GetWhile(out_data, r_error));
			}
			// Ставлю проверку в двух местах, ибо не знаю, в какой момент этот флаг устанавливается
			THROW(GetCurFlags(2, flag));
			THROWERR(!(flag & 0x40), PIRIT_NOTENOUGHTMEMFORSESSCLOSE); // Нет памяти для закрытия смены в ФП
			if(!(flag & 0x4))  // Проверяем флаг "смена открыта"
				ok = 0;
		}
		else if(cmd.IsEqiAscii("XREPORT")) {
			SetLastItems(cmd, pInputData);
			THROW(StartWork());
			CreateStr(CshrName, str);
			{
				OpLogBlock __oplb(LogFileName, "20", 0);
				THROWERR(PutData("20", str), PIRIT_NOTSENT);
				THROW(GetWhile(out_data, r_error));
			}
		}
		else if(cmd.IsEqiAscii("OPENCHECK")) {
			SetLastItems(cmd, pInputData);
			THROW(StartWork());
			for(uint i = 0; pairs.get(&i, s_pair) > 0;) {
				s_pair.Divide('=', s_param, param_val);
				if(s_param.CmpNC("CHECKTYPE") == 0) {
					val = param_val.ToLong();
					switch(val) {
						// Это все если обычный режим формирования документа, а не пакетный
						case 0: Check.CheckType = 1; break;
						case 1: Check.CheckType = 2; break;
						case 2: Check.CheckType = 3; break;
						case 3: Check.CheckType = 4; break;
						case 4: Check.CheckType = 5; break;
					}
				}
				else if(s_param.CmpNC("CHECKNUM") == 0)
					Check.CheckNum = param_val.ToLong();
			}
			THROW(RunCheck(0));
		}
		else if(cmd.IsEqiAscii("CLOSECHECK")) {
			SetLastItems(cmd, pInputData);
			THROW(StartWork());
			for(uint i = 0; pairs.get(&i, s_pair) > 0;) {
				s_pair.Divide('=', s_param, param_val);
				if(s_param.CmpNC("PAYMCASH") == 0)
					Check.PaymCash = param_val.ToReal();
				else if(s_param.CmpNC("PAYMCARD") == 0)
					Check.PaymCard = param_val.ToReal();
			}
			THROW(RunCheck(1));
		}
		else if(cmd.IsEqiAscii("PRINTFISCAL")) {
			int   tax_entry_id_result = -1;
			double _vat_rate = 0.0;
			int   is_vat_free = 0;
			SetLastItems(cmd, 0);
			THROW(StartWork());
			for(uint i = 0; pairs.get(&i, s_pair) > 0;) {
				s_pair.Divide('=', s_param, param_val);
				s_param.ToUpper();
				if(s_param == "QUANTITY")
					Check.Quantity = param_val.ToReal();
				else if(s_param == "PRICE")
					Check.Price = param_val.ToReal();
				else if(s_param == "DEPARTMENT")
					Check.Department = param_val.ToLong();
				else if(s_param == "TEXT") 
					Check.Text = param_val;
				else if(s_param == "CODE")
					Check.Code = param_val;
				else if(s_param == "VATRATE") { // @v9.7.1
					_vat_rate = R2(param_val.ToReal());
				}
				else if(s_param == "VATFREE") { // @v9.8.9
					if(param_val.Empty() || param_val.IsEqiAscii("yes") || param_val.IsEqiAscii("true") || param_val == "1") 
						is_vat_free = 1;
				}
			}
			{
				Check.Tax = 0;
				// @v9.9.4 {
				if(Check.Price <= 0.0) {
					Check.Price = 0.0;
					is_vat_free = 1;
				}
				// } @v9.9.4 
				if(is_vat_free)
					_vat_rate = 0.0;
				tax_entry_id_result = 0;
				// Освобожденные от НДС продавцы передают признак VATFREE но таблица не предусматривает такого элемента.
				// По этому, используя эмпирическое правило, считаем, что второй элемент таблицы, имеющий нулевую
				// ставку относится к VATFREE-продажам.
				uint   zero_entry_1 = 0; 
				uint   zero_entry_2 = 0; 
				for(uint tidx = 0; tidx < SIZEOFARRAY(DvcTaxArray); tidx++) {
					if(DvcTaxArray[tidx].Name[0]) {
						const double entry_rate = DvcTaxArray[tidx].Rate;
						if(entry_rate == 0.0 && _vat_rate == 0.0) {
							if(!zero_entry_1) {
								zero_entry_1 = tidx+1;
								if(!is_vat_free) {
									Check.Tax = (int)tidx;
									tax_entry_id_result = tidx+1;
									break;
								}
							}
							else if(!zero_entry_2) {
								zero_entry_2 = tidx+1;
								if(is_vat_free) {
									Check.Tax = (int)tidx;
									tax_entry_id_result = tidx+1;
									break;
								}
							}
						}
						else if(feqeps(entry_rate, _vat_rate, 1E-5)) {
							Check.Tax = (int)tidx;
							tax_entry_id_result = tidx+1;
							break;
						}
					}
				}
				if(!tax_entry_id_result) {
					if(is_vat_free && zero_entry_1) {
						Check.Tax = (int)(zero_entry_1-1);
						tax_entry_id_result = zero_entry_1;
					}
					else {
						//
						// Не нашли в таблице того, чего искали: включаем default-вариант, основанный на документации к драйверу
						//
						if(_vat_rate == 18.0) {
							Check.Tax = 0;
							tax_entry_id_result = 1;
						}
						else if(_vat_rate == 10.0) {
							Check.Tax = 1;
							tax_entry_id_result = 2;
						}
						else if(_vat_rate == 0.0) {
							if(!is_vat_free) {
								Check.Tax = 2;
								tax_entry_id_result = 3;
							}
							else {
								Check.Tax = 3;
								tax_entry_id_result = 4;
							}
						}
					}
				}
			}
			// @debug {
			if(LogFileName.NotEmpty()) {
				if(tax_entry_id_result < 0)
					(str = "TaxEntry isn't identified").CatDiv(':', 2).Cat(_vat_rate);
				else if(tax_entry_id_result == 0)
					(str = "TaxEntry isn't found").CatDiv(':', 2).Cat(_vat_rate);
				else if(tax_entry_id_result > 0)
					(str = "TaxEntry is found").CatDiv(':', 2).Cat(tax_entry_id_result-1).CatDiv(',', 2).Cat(_vat_rate);
				SLS.LogMessage(LogFileName, str);
			}
			// } @debug 
			THROW(RunCheck(2));
		}
		else if(cmd.IsEqiAscii("PRINTTEXT")) {
			SetLastItems(cmd, 0);
			//LastParams = pInputData;
			//LastCmd = 0;
			THROW(StartWork());
			for(uint i = 0; pairs.get(&i, s_pair) > 0;) {
				s_pair.Divide('=', s_param, param_val);
				if(s_param.CmpNC("FONTSIZE") == 0)
					Check.FontSize = param_val.ToLong();
				else if(s_param.CmpNC("TEXT") == 0)
					Check.Text = param_val;
			}
			THROW(RunCheck(3));
		}
		else if(cmd.IsEqiAscii("PRINTBARCODE")) {
			struct PiritBarcodeEntry {
				PiritBarcodeEntry() : Width(0), Height(0), Std(0), TextParam(0)
				{
				}
				int    Width;
				int    Height;
				int    Std;
				int    TextParam;
				SString Code;
			};
			PiritBarcodeEntry bc_entry;
			// type: EAN8 EAN13 UPCA UPCE CODE39 IL2OF5 CODABAR PDF417 QRCODE
			// width (points)
			// height (points)
			// label : none below above
			// code:
			for(uint i = 0; pairs.get(&i, s_pair) > 0;) {
				s_pair.Divide('=', s_param, param_val);
				if(s_param.CmpNC("TYPE") == 0) {
					param_val.ToLower();
					if(oneof3(param_val, "code39", "code-39", "code 39"))
						bc_entry.Std = 4; // !
					else if(oneof4(param_val, "pdf417", "pdf-417", "pdf 417", "pdf"))
						bc_entry.Std = 7; // !
					else if(oneof3(param_val, "ean13", "ean-13", "ean 13"))
						bc_entry.Std = 2; // !
					else if(oneof3(param_val, "ean8", "ean-8", "ean 8"))
						bc_entry.Std = 3; // !
					else if(oneof3(param_val, "upca", "upc-a", "upc a"))
						bc_entry.Std = 0; // !
					else if(oneof3(param_val, "upce", "upc-e", "upc e"))
						bc_entry.Std = 1; // !
					else if(oneof4(param_val, "qr", "qr-code", "qr code", "qrcode"))
						bc_entry.Std = 8;
					else if(param_val == "interleaved2of5")
						bc_entry.Std = 5; // !
					else if(param_val == "codabar")
						bc_entry.Std = 6; // !
				}
				else if(s_param.IsEqiAscii("WIDTH")) {
					bc_entry.Width = param_val.ToLong();
				}
				else if(s_param.IsEqiAscii("HEIGHT")) {
					bc_entry.Height = param_val.ToLong();
				}
				else if(s_param.IsEqiAscii("LABEL")) {
					if(param_val.IsEqiAscii("above"))
						bc_entry.TextParam = 1;
					else if(param_val.IsEqiAscii("below"))
						bc_entry.TextParam = 2;
				}
				else if(s_param.IsEqiAscii("TEXT")) {
					(bc_entry.Code = param_val).Strip();
				}
			}
			{
				SString in_data;
				CreateStr(bc_entry.TextParam, in_data);
				CreateStr(bc_entry.Width, in_data);
				CreateStr(bc_entry.Height, in_data);
				CreateStr(bc_entry.Std, in_data);
				CreateStr(bc_entry.Code, in_data);
				//
				{
					OpLogBlock __oplb(LogFileName, "41", 0);
					THROWERR(PutData("41", in_data), PIRIT_NOTSENT);
					THROW(GetWhile(out_data, r_error));
				}
			}
		}
		else if(cmd.IsEqiAscii("GETCHECKPARAM")) {
			SetLastItems(0, 0);
			str = 0;
			for(uint i = 0; pairs.get(&i, s_pair) > 0;){
				s_pair.Divide('=', s_param, param_val);
				if(s_param.IsEqiAscii("AMOUNT"))
					str.Cat(s_param).Semicol();
				else if(s_param.IsEqiAscii("CHECKNUM"))
					str.Cat(s_param).Semicol();
				else if(s_param.IsEqiAscii("CASHAMOUNT"))
					str.Cat(s_param).Semicol();
				else if(s_param.IsEqiAscii("RIBBONPARAM"))
					str.Cat(s_param).Semicol();
			}
			ok = ReturnCheckParam(str, pOutputData, outSize); // Здесь может быть переполнение буфера
		}
		else if(cmd.IsEqiAscii("ANNULATE")) {
			SetLastItems(cmd, pInputData);
			THROW(StartWork());
			THROW(RunCheck(4));
		}
		else if(cmd.IsEqiAscii("INCASHMENT")) {
			SetLastItems(cmd, pInputData);
			THROW(StartWork());
			for(uint i = 0; pairs.get(&i, s_pair) > 0;){
				s_pair.Divide('=', s_param, param_val);
				if(s_param.IsEqiAscii("AMOUNT"))
					Check.IncassAmt = param_val.ToReal();
			}
			THROW(RunCheck(5));
		}
		else if(cmd.IsEqiAscii("OPENBOX")) {
			SetLastItems(0, 0);
			THROW(OpenBox());
		}
		else if(cmd.IsEqiAscii("GETECRSTATUS")) {
			THROW(GetStatus(str));
			if(outSize < str.BufSize()) {
				NotEnoughBuf(str);
				memcpy(pOutputData, str, outSize);
				ok = 2;
			}
			else
				memcpy(pOutputData, str, str.BufSize());
		}
		else if(cmd.IsEqiAscii("GETLASTERRORTEXT")) {
			LastErrorText(str);
			if(outSize < (str.Len()+1)){
				NotEnoughBuf(str);
				str.CopyTo(pOutputData, outSize);
				ok = 2;
			}
			else
				str.CopyTo(pOutputData, outSize);
		}
		else if(sstreqi_ascii(cmd, "CLEARSLIPBUF") || sstreqi_ascii(cmd, "FILLSLIPBUF") || sstreqi_ascii(cmd, "PRINTSLIPDOC") /*|| sstreqi_ascii(cmd, "CONTINUEPRINT")*/) {
			SetLastItems(0, 0);
			ok = 0;
		}
		else if(cmd.Empty())
			ok = 0;
		else { // Если дана неизвестная  команда, то сообщаем об этом
			memcpy(pOutputData, "2", sizeof("2"));
			ok = 1;
		}
	} // if(LastError != NOTENOUGHMEM)
	CATCH
		ok = 1;
		GetGlobalErrCode();
		_itoa(ErrorCode, pOutputData, 10);
	ENDCATCH;
	ErrorCode = 0;
	return ok;
}

int Init()
{
	SETIFZ(P_Pirit, new PiritEquip);
	return 1;
}

int Release()
{
	ZDELETE(P_Pirit);
	return 1;
}

SString & PiritEquip::LastErrorText(SString & rMsg)
{
	rMsg.Z();
	SString status_str, cmd_str;
	char str[16];
	memzero(str, sizeof(str));
	for(uint i = 0; i < SIZEOFARRAY(ErrMsg); i++) {
		if(LastError == ErrMsg[i].Id) {
			rMsg.Z().Cat(ErrMsg[i].P_Msg);
			break;
		}
	}
	if(LastError == PIRIT_FATALERROR) {
		rMsg.Space().Cat(_itoa(FatalFlags, str, 10));
	}
	else if(LastError == PIRIT_DATELSLASTOP) {
		rMsg.Cat(CashDateTime);
	}
	// new {
	else if(LastError == PIRIT_ERRSTATUSFORFUNC) {
		// @v9.6.9 GetStatus(status_str);
		status_str = "unkn"; // @v9.6.9
		if(LastStatus == CHECKOPENED)
			rMsg.CatDiv(':', 2).Cat(CHECK_OPENED_STR);
		else if(LastStatus == CHECKCLOSED)
			rMsg.CatDiv(':', 2).Cat(CHECK_CLOSED_STR);
		GetLastCmdName(cmd_str);
		rMsg.CatDiv(':', 2).Cat(cmd_str);
	}
	// } new
	else
		rMsg.Cat(LastError);
	return rMsg;
}

int PiritEquip::NotEnoughBuf(SString & rStr)
{
	ErrorCode = PIRIT_NOTENOUGHMEM;
	LastStr = rStr;
	const int size = rStr.BufSize();
	rStr.Z().Cat(size);
	return 1;
}

int PiritEquip::SetConnection()
{
	int    ok = 1;
	int    r = 0;
	int    is_ready = 0;
	CommPortParams port_params;
	//SString in_data;
	//SString out_data;
	//SString r_error;
	//SString log_str;
	CommPort.GetParams(&port_params);
	port_params.ByteSize = 8;
	port_params.Parity = NOPARITY;
	port_params.StopBits = ONESTOPBIT;
	switch(Cfg.BaudRate) {
		case 0: port_params.Cbr = cbr2400; break;
		case 1: port_params.Cbr = cbr4800; break;
		case 2: port_params.Cbr = cbr9600; break;
		case 3: port_params.Cbr = cbr14400; break;
		case 4: port_params.Cbr = cbr19200; break;
		case 5: port_params.Cbr = cbr38400; break;
		case 6: port_params.Cbr = cbr56000; break;
		case 7: port_params.Cbr = cbr57600; break;
		case 8: port_params.Cbr = cbr115200; break;
		case 9: port_params.Cbr = cbr128000; break;
		case 10: port_params.Cbr = cbr256000; break;
	}
	CommPort.SetParams(&port_params);
	THROW(CommPort.InitPort(Cfg.Port));
	// @v9.5.7 delay(200);
	CommPort.PutChr(ENQ); // Проверка связи с ККМ
	r = CommPort.GetChr();
	THROW(r == ACK);
#if 0 // @v9.7.1 {
	if(Cfg.BaudRate < 6) { // @v9.6.12 (<8)-->(<6)
		CreateStr(5, in_data);
		{
			OpLogBlock __oplb(LogFileName, "93", 0);
			THROWERR(PutData("93", in_data), PIRIT_NOTSENT); // Устанавливаем скорость ПУ 115200 бит/c
			THROW(GetWhile(out_data, r_error));
		}
		//
		// Устанавливаем параметры COM-порта, соответствующие новой скорости ПУ
		//
		port_params.Cbr = cbr115200;
		THROW(CommPort.SetParams(&port_params));

		THROW(CommPort.InitPort(Cfg.Port));
		// @v9.5.7 delay(200);
		CommPort.PutChr(ENQ); // Проверка связи с ККМ
		r = CommPort.GetChr();
		THROW(r == ACK);
	}
#endif // } 0 @v9.7.1
	if((Cfg.ReadCycleCount > 0) || (Cfg.ReadCycleDelay > 0))
		CommPort.SetReadCyclingParams(Cfg.ReadCycleCount, Cfg.ReadCycleDelay);
	CATCHZOK
	return ok;
}

int PiritEquip::CloseConnection()
{
	/*if(H_Port != INVALID_HANDLE_VALUE)
		CloseHandle(H_Port);*/
	return 1;
}

int PiritEquip::GetTaxTab()
{
	int    ok = 1;
	SString in_data;
	SString out_data;
	SString r_error;
	SString raw_tax_val;
	SString log_buf;
	for(int i = 0; i < 5; i++) { // В пирите не более 6 налоговых ставок

		MEMSZERO(DvcTaxArray[i]);

		in_data = 0;
		raw_tax_val = 0;
		CreateStr(40, in_data); // Наименование i-й налоговой ставки
		CreateStr(i, in_data);
		{
			OpLogBlock __oplb(LogFileName, "11", 0);
			THROWERR(PutData("11", in_data), PIRIT_NOTSENT);
			THROW(GetWhile(out_data, r_error));
		}
		if(out_data.NotEmpty()) {
			STRNSCPY(DvcTaxArray[i].Name, out_data);
			in_data = 0;
			out_data = 0;
			CreateStr(41, in_data); // Значение i-й налоговой ставки
			CreateStr(i, in_data);
			{
				OpLogBlock __oplb(LogFileName, "11", 0);
				THROWERR(PutData("11", in_data), PIRIT_NOTSENT);
				THROW(GetWhile(out_data, r_error));
			}
			raw_tax_val = out_data;
			DvcTaxArray[i].Rate = out_data.ToReal();
		}
		if(i == 0)
			log_buf.Cat("DvcTaxArray").CatDiv(':', 2);
		else
			log_buf.CatDiv(';', 2);
		log_buf.Cat(DvcTaxArray[i].Name).CatDiv(',', 2).Cat(DvcTaxArray[i].Rate, MKSFMTD(0, 6, NMBF_NOTRAILZ)).CatChar('|').Cat(raw_tax_val);
	}
	if(LogFileName.NotEmpty()) {
		SLS.LogMessage(LogFileName, log_buf, 8192);
	}
	CATCHZOK
	return ok;
}

int PiritEquip::SetCfg()
{
	int    ok = 1;
	int    err_code = 0;
	int    flag = 0;
	uint   i = 0;
	//SString out;
	//SString subdata;
	SString in_data;
	SString out_data;
	SString r_error;
	// Логический номер кассы
	CreateStr(10, in_data);
	CreateStr(0, in_data);
	CreateStr((int)Cfg.LogNum, in_data);
	{
		OpLogBlock __oplb(LogFileName, "12", 0);
		THROWERR(PutData("12", in_data), PIRIT_NOTSENT);
		THROW(GetWhile(out_data, r_error));
	}
	{
		in_data.Z();
		// Нумерация чеков ККМ
		CreateStr(2, in_data);
		CreateStr(0, in_data);
		{
			OpLogBlock __oplb(LogFileName, "11", 0);
			THROWERR(PutData("11", in_data), PIRIT_NOTSENT); // Получаем параметры чека
			THROW(GetWhile(out_data, r_error));
		}
		flag = out_data.ToLong();
		flag &= ~128; // Устанавливаем бит нумерации чеков ККМ
	}
	{
		in_data.Z();
		CreateStr(2, in_data);
		CreateStr(0, in_data);
		CreateStr(flag, in_data);
		{
			OpLogBlock __oplb(LogFileName, "12", 0);
			THROWERR(PutData("12", in_data), PIRIT_NOTSENT);
			THROW(GetWhile(out_data, r_error));
		}
	}
	// Получаем номер текущей сессии
	CreateStr(1, in_data);
	{
		OpLogBlock __oplb(LogFileName, "01", 0);
		THROWERR(PutData("01", in_data), PIRIT_NOTSENT); // Запрос номера текущей сессии(смены)
		THROW(GetWhile(out_data, r_error));
		{
			StringSet delim_out(FS, out_data);
			delim_out.get(&i, out_data); // В out считан номер запроса
			delim_out.get(&i, out_data); // В out считан номер текущей сессии(смены)
			SessID = out_data.ToLong();
		}
	}
	// Установка/снятие флага печати логотипа
	if(Cfg.Logo.Print == 1)
		THROW(PrintLogo(1))
	else
		THROW(PrintLogo(0));
	CATCHZOK
	return ok;
}

int PiritEquip::StartWork()
{
	int    ok = 1, flag = 0;
	THROW(GetCurFlags(2, flag)); // Проверяем флаг "не была вызвана функция начало работы"
	if(flag & 0x1) {
		SString datetime, in_data, out_data, r_error;
		SYSTEMTIME sys_dt_tm;
		SString date, time;
		{
			OpLogBlock __oplb(LogFileName, "13", 0);
			THROWERR(PutData("13", in_data), PIRIT_NOTSENT); // Смотрим текщую дату/время на ККМ
			THROW(GetWhile(out_data, r_error));
		}
		out_data.Divide(FS, date, time);
		CashDateTime.Z().Cat("Текущая дата на ККМ: ").Cat(date).Cat(" Текущее время на ККМ: ").Cat(time);
		in_data.Z();
		GetLocalTime(&sys_dt_tm);
		GetDateTime(sys_dt_tm, datetime, 0);
		CreateStr(datetime, in_data);
		GetDateTime(sys_dt_tm, datetime, 1);
		CreateStr(datetime, in_data);
		{
			OpLogBlock __oplb(LogFileName, "10", 0);
			THROWERR(PutData("10", in_data), PIRIT_NOTSENT);
			THROW(GetWhile(out_data, r_error));
		}
		THROW(GetCurFlags(2, flag));
		if(!(flag & 0x4) && (r_error.CmpNC("0B") == 0)) {  // Проверяем что смена закрыта и код ошибки "дата и время отличаются от текущих даты и времени ККМ более чем на 8 минут"
			in_data.Z();
			GetLocalTime(&sys_dt_tm);
			GetDateTime(sys_dt_tm, datetime, 0);
			CreateStr(datetime, in_data);
			GetDateTime(sys_dt_tm, datetime, 1);
			CreateStr(datetime, in_data);
			{
				OpLogBlock __oplb(LogFileName, "14", 0);
				THROWERR(PutData("14", in_data), PIRIT_NOTSENT); // Устанавливаем системные дату и время в ККМ
				THROW(GetWhile(out_data, r_error));
			}
		}
	}
	CATCHZOK
	return ok;
}

int PiritEquip::GetCurFlags(int numFlags, int & rFlags)
{
	const  uint max_tries = 10;
	int    ok = 1;
	SString out_data, r_error;
	uint count = 0;
	rFlags = 0;
	{
		OpLogBlock __oplb(LogFileName, "00", 0);
		THROWERR(PutData("00", 0), PIRIT_NOTSENT); // Запрос флагов статуса
		while(out_data.Empty() && count < max_tries) {
			if(numFlags == 1) { // Если запрашиваем флаги фатального состояния, дабы не зациклиться
				GetData(out_data, r_error);
			}
			else {
				THROW(GetWhile(out_data, r_error));
			}
			count++;
		}
		{
			// @v9.9.4 SString s_flags;
			StringSet fl_pack(FS, out_data);
			int    fc = 0; // Считанное количество значений
			// @v9.6.10 {
			for(uint sp = 0; fl_pack.get(&sp, /*s_flags*/out_data);) {
				if(++fc == numFlags)
					rFlags = /*s_flags*/out_data.ToLong();
			}
			ok = fc;
			// } @v9.6.10
			/* @v9.6.10
			for(uint j = 1, i = 0; j < (uint)numFlags+1; j++) {
				THROW(fl_pack.get(&i, s_flags));
			}
			*/
		}
	}
	// @v9.6.10 rFlags = s_flags.ToLong();
	CATCHZOK
	return ok;
}

int PiritEquip::RunCheck(int opertype)
{
	int    ok = 1;
	int    flag = 0;
	int    _halfbyte = 0;
	int    text_attr = 0;
	int    gcf_result = 0; // Результат вызова GetCurFlags
	uint   count = 0;
	SString in_data;
	SString out_data;
	SString r_error;
	SString str;
	switch(opertype) {
		case 0: // Открыть документ
			in_data = 0;
			CreateStr(Check.CheckType, in_data);
			CreateStr(Check.Department, in_data);
			CreateStr(CshrName, in_data);
			CreateStr("", in_data);
			{
				OpLogBlock __oplb(LogFileName, "30", 0);
				THROWERR(PutData("30", in_data), PIRIT_NOTSENT);
				THROW(GetWhile(out_data, r_error));
			}
			break;
		case 1: // Закрыть документ
			// Проверяем наличие открытого документа
			THROW(gcf_result = GetCurFlags(3, flag));
			if((flag >> 4) || (gcf_result < 3)) {
				const uint8 hb1 = (flag & 0x0F);
				const uint8 hb2 = (flag & 0xF0);
				if((gcf_result < 3) || (oneof2(hb1, 2, 3) && hb2 != 0x40)) { // Если открыт чек и не была произведена оплата, то операция оплаты
					in_data = 0;
					if(Check.PaymCard != 0.0) {
						CreateStr(1, in_data); // Тип оплаты
						FormatPaym(Check.PaymCard, str);
						CreateStr(str, in_data);
						CreateStr("", in_data);
						{
							OpLogBlock __oplb(LogFileName, "47", 0);
							THROWERR(PutData("47", in_data), PIRIT_NOTSENT);
							THROW(GetWhile(out_data, r_error));
						}
					}
					in_data = 0;
					if(Check.PaymCash != 0.0) {
						CreateStr(0, in_data); // Тип оплаты
						FormatPaym(Check.PaymCash, str);
						CreateStr(str, in_data);
						CreateStr("", in_data);
						{
							OpLogBlock __oplb(LogFileName, "47", 0);
							THROWERR(PutData("47", in_data), PIRIT_NOTSENT);
							THROW(GetWhile(out_data, r_error));
						}
					}
				}
				in_data = 0;
				if(Cfg.Flags & 0x08000000L)
					CreateStr(1, in_data); // Чек не отрезаем (только для сервисных документов)
				else
					CreateStr(0, in_data); // Чек отрезаем
				{
					OpLogBlock __oplb(LogFileName, "31", 0);
					THROWERR(PutData("31", in_data), PIRIT_NOTSENT);
					THROW(GetWhile(out_data, r_error));
				}
			}
			// new {
			else {
				THROWERR(0, PIRIT_ERRSTATUSFORFUNC);
			}
			// } new
			break;
		case 2: // Печать фискальной строки
			in_data.Z();
			THROW(GetCurFlags(3, flag));
			(str = Check.Text).Trim(220); // [0..224]
			CreateStr(str, in_data); // Название товара      // @v9.5.7 ""-->Check.Text 
			(str = Check.Code).Trim(13); // [0..18]
			CreateStr(str, in_data); // Артикул или штрихкод // @v9.5.7 ""-->Check.Code
			CreateStr(Check.Quantity, in_data);
			// @vmiller comment
			/*FormatPaym(Check.Price, str);
			CreateStr(str, in_data);*/
			CreateStr(Check.Price, in_data); // @vmiller
			CreateStr((int)Check.Tax, in_data); // @v9.5.7 Номер налоговой ставки // @v9.7.1 0-->Check.Tax
			CreateStr((int)0, in_data); // @v9.5.7 Номер товарной позиции
			CreateStr(Check.Department, in_data); // @v9.5.7 Номер секции
			{
				// @v9.9.4 const int do_check_ret = 0;
				const int do_check_ret = BIN(Check.Price == 0.0); // @v9.9.4
				Check.Clear();
				{
					OpLogBlock __oplb(LogFileName, "42", 0);
					THROWERR(PutData("42", in_data), PIRIT_NOTSENT);
					if(do_check_ret) {
						THROW(GetWhile(out_data, r_error));
					}
					else {
						out_data.Z();
						r_error = "00";
					}
				}
				Check.Clear();
			}
			break;
		case 3: // Печать текстовой строки
			{
				in_data.Z();
				THROW(gcf_result = GetCurFlags(3, flag));
				const uint8 hb1 = (flag & 0x0F);
				if(hb1 == 1) { // Текстовая строка для сервисного документа
					if((Check.FontSize == 1) || (Check.FontSize == 2))
						text_attr = 0x01;
					else if(Check.FontSize == 3)
						text_attr = 0;
					else if(Check.FontSize == 4)
						text_attr = 0x10;
					else if(Check.FontSize > 4)
						text_attr = 0x20 | 0x10;
					if(Check.Text.Len() + 1 > 54)
						Check.Text.Trim(52);
					CreateStr(Check.Text.ToOem(), in_data);
					if(text_attr != 0)
						CreateStr(text_attr, in_data);
					Check.Clear();
					{
						OpLogBlock __oplb(LogFileName, "40", 0);
						THROWERR(PutData("40", in_data), PIRIT_NOTSENT);
						{
							const int do_check_ret = 0;
							if(do_check_ret) {
								THROW(GetWhile(out_data, r_error));
							}
							else {
								out_data.Z();
								r_error = "00";
							}
						}
					}
				}
				else if(oneof2(hb1, 2, 3) || (gcf_result < 3)) { // Текстовая строка для чека
					in_data.Z();
					text_attr = 0;
					CreateStr(0, in_data);
					if(oneof2(Check.FontSize, 1, 2))
						text_attr = 0x01;
					else if(Check.FontSize == 3)
						text_attr = 0;
					else if(Check.FontSize == 4)
						text_attr = 0x10;
					else if(Check.FontSize > 4)
						text_attr = 0x20 | 0x10;
					if(Check.Text.Len() + 1 > 56)
						Check.Text.Trim(55);
					CreateStr(text_attr, in_data);
					Check.Text.ToOem();
					CreateStr(Check.Text, in_data);
					CreateStr("", in_data);
					CreateStr("", in_data);
					CreateStr("", in_data);
					{
						OpLogBlock __oplb(LogFileName, "49", 0);
						THROWERR(PutData("49", in_data), PIRIT_NOTSENT);
						{
							const int do_check_ret = 0;
							if(do_check_ret) {
								THROW(GetWhile(out_data, r_error));
							}
							else {
								out_data.Z();
								r_error = "00";
							}
						}
					}
				}
				// new {
				else {
					THROWERR(0, PIRIT_ERRSTATUSFORFUNC);
				}
				// } new
			}
			break;
		case 4: // Аннулировать чек
			// Проверяем наличие открытого документа
			THROW(GetCurFlags(3, flag));
			if((flag >> 4) != 0) {
				{
					OpLogBlock __oplb(LogFileName, "32", 0);
					THROWERR(PutData("32", 0), PIRIT_NOTSENT);
					THROW(GetWhile(out_data, r_error));
				}
			}
			break;
		case 5: // Внесение/изъятие наличности
			{
				SString str, b_point, a_point;
				in_data.Z();
				CreateStr("", in_data);
				FormatPaym(Check.IncassAmt, str);
				CreateStr(str, in_data);
				{
					OpLogBlock __oplb(LogFileName, "48", 0);
					THROWERR(PutData("48", in_data), PIRIT_NOTSENT);
					THROW(GetWhile(out_data, r_error));
				}
			}
			break;
	}
	CATCHZOK
	return ok;
}

int PiritEquip::ReturnCheckParam(SString & rInput, char * pOutput, size_t size)
{
	int    ok = 0;
	int    r = 0;
	SString buf, in_data, out_data, r_error, str, s_output;
	StringSet params(';', rInput);
	for(uint i = 0; params.get(&i, buf) > 0;) {
		in_data.Z();
		out_data.Z();
		if(buf.CmpNC("AMOUNT") == 0) {
			CreateStr(1, in_data);
			{
				OpLogBlock __oplb(LogFileName, "03", 0);
				THROWERR(PutData("03", in_data), PIRIT_NOTSENT);
				THROW(GetWhile(out_data, r_error));
			}
			{
				StringSet dataset(FS, out_data);
				uint k = 0;
				dataset.get(&k, str); // Номер запроса
				dataset.get(&k, str);
				s_output.Cat("AMOUNT").CatChar('=').Cat(str).Semicol();
			}
		}
		else if(buf.CmpNC("CHECKNUM") == 0) {
			CreateStr(2, in_data);
			{
				OpLogBlock __oplb(LogFileName, "03", 0);
				THROWERR(PutData("03", in_data), PIRIT_NOTSENT);
				THROW(GetWhile(out_data, r_error));
			}
			{
				StringSet dataset(FS, out_data);
				uint   k = 0;
				// Возвращаемые значения (в порядке следования в буфере ответа) {
				int    cc_type = 0; // Тип чека
				char   current_op_counter[64]; // Текущий операционный счетчик
				int    cc_number = 0; // Номер чека
				int    doc_number = 0; // Номер документа
				double cc_amount = 0.0; // Сумма чека
				double cc_discount = 0.0; // Сумма скидки по чеку
				double cc_markup = 0.0; // Сумма неценки по чеку
				char   _kpk[64]; // Строка КПК (?)
				// }
				for(uint count = 0; dataset.get(&k, str)/* && count < 3*/; count++) { // Еще номер запроса, беру номер /*документа*/ чека
					//count++;
					if(count == 0)
						cc_type = str.ToLong();
					else if(count == 1)
						STRNSCPY(current_op_counter, str);
					else if(count == 2)
						cc_number = str.ToLong();
					else if(count == 3)
						doc_number = str.ToLong();
					else if(count == 4)
						cc_amount = str.ToReal();
					else if(count == 5)
						cc_discount = str.ToReal();
					else if(count == 6)
						cc_markup = str.ToReal();
					else if(count == 7)
						STRNSCPY(_kpk, str);
				}
				s_output.Cat("CHECKNUM").CatChar('=').Cat(/*str*/cc_number).Semicol();
			}
		}
		else if(buf.CmpNC("CASHAMOUNT") == 0) {
			CreateStr(7, in_data);
			{
				OpLogBlock __oplb(LogFileName, "02", 0);
				THROWERR(PutData("02", in_data), PIRIT_NOTSENT);
				THROW(GetWhile(out_data, r_error));
			}
			{
				StringSet dataset(FS, out_data);
				uint k = 0;
				dataset.get(&k, str); // Номер запроса
				dataset.get(&k, str);
				s_output.Cat("CASHAMOUNT").CatChar('=').Cat(str).Semicol();
			}
		}
	}
	if(size < s_output.BufSize()){
		NotEnoughBuf(s_output);
		memcpy(pOutput, s_output, size);
		ok = 2;
	}
	else
		memcpy(pOutput, s_output, s_output.BufSize());
	CATCH
		ok = 1;
	ENDCATCH;
	return ok;
}

int PiritEquip::OpenBox()
{
	int    ok = 1;
	SString out_data, r_error;
	{
		OpLogBlock __oplb(LogFileName, "81", 0);
		THROWERR(PutData("81", 0), PIRIT_NOTSENT);
		THROW(GetWhile(out_data, r_error));
	}
	if(out_data.ToLong() == 0){
		{
			OpLogBlock __oplb(LogFileName, "80", 0);
			THROWERR(PutData("80", 0), PIRIT_NOTSENT);
			THROW(GetWhile(out_data, r_error));
		}
	}
	CATCHZOK
	return ok;
}

void PiritEquip::GetGlobalErrCode()
{
	LastError = ErrorCode; // Сюда пишем конкретный код, который нужен для описания ошибки
	switch(ErrorCode) {
		case PIRIT_FATALERROR: ErrorCode = PIRIT_ECRFATALERR; GetCurFlags(1, FatalFlags); break;
		case PIRIT_FMOVERFLOW: ErrorCode = PIRIT_ECRFMOVERFLOW; break;
		case PIRIT_ECRRFORMAT: ErrorCode = PIRIT_ECRFATALERR; break;
		case PIRIT_ECRACCIDENT: ErrorCode = PIRIT_ECRFATALERR; break;
		case PIRIT_KCACCIDENT: ErrorCode = PIRIT_ECRFATALERR; break;
		case PIRIT_ECRTIMEOUT: ErrorCode = PIRIT_ECRFATALERR; break;
		case PIRIT_ECROVERFLOW: ErrorCode = PIRIT_ECRFMOVERFLOW; break;
		case PIRIT_ECRERRORDATETIME: ErrorCode = PIRIT_ECRFATALERR; break;
		case PIRIT_ECRNODATA: ErrorCode = PIRIT_ECRFATALERR; break;
		case PIRIT_ECRTOOMUCH: ErrorCode = PIRIT_ECRFATALERR; break;
		case PIRIT_NOANSWER: ErrorCode = PIRIT_ECRFATALERR; break;
		case PIRIT_ECRERREXCHANGE: ErrorCode = PIRIT_ECRFATALERR; break;
	}
}

int PiritEquip::GetWhile(SString & rOutData, SString & rError)
{
	const  uint max_tries = 10;
	int    ok = 1;
	uint   count = 0;
	rError.Z();
	//while(rError.Empty() && count < max_tries) {
		if(GetData(rOutData, rError) < 0) {
			rError = "00";
			ok = -1;
		}
		else {
			if(rError.NotEmpty()) {
				if(rError != "00" && rError != "0B" && rError != "09") {
					THROWERR(rError.CmpNC("0C"), PIRIT_DATELSLASTOP); // Системная дата меньше даты последней фискальной операции, зарегистрированной в ККМ
					THROWERR(rError.CmpNC("20"), PIRIT_FATALERROR); // Фатальная ошибка ККМ
					THROWERR(rError.CmpNC("21"), PIRIT_FMOVERFLOW); // Нет свободного места в фискальной памяти ККМ
					THROWERR(rError.CmpNC("41"), PIRIT_ECRRFORMAT); // Некорректный формат или параметр команды ЭКЛЗ
					THROWERR(rError.CmpNC("42"), PIRIT_ECRERRORSTATUS); // Некорректное состояние ЭКЛЗ
					THROWERR(rError.CmpNC("43"), PIRIT_ECRACCIDENT); // Авария ЭКЛЗ
					THROWERR(rError.CmpNC("44"), PIRIT_KCACCIDENT); // Авария КС (криптографического сопроцессора)в составе ЭКЛЗ
					THROWERR(rError.CmpNC("45"), PIRIT_ECRTIMEOUT); // Исчерпан временной ресурс использования ЭКЛЗ
					THROWERR(rError.CmpNC("46"), PIRIT_ECROVERFLOW); // ЭКЛЗ переполнена
					THROWERR(rError.CmpNC("47"), PIRIT_ECRERRORDATETIME); // Неверные дата или время
					THROWERR(rError.CmpNC("48"), PIRIT_ECRNODATA); // Нет запрошенных данных
					THROWERR(rError.CmpNC("49"), PIRIT_ECRTOOMUCH); // Переполнение (отрицательный итог документа, слишком много отделов для клиента)
					THROWERR(rError.CmpNC("4A"), PIRIT_NOANSWER); // Нет ответа от ЭКЛЗ
					THROWERR(rError.CmpNC("4B"), PIRIT_ECRERREXCHANGE); // Ошибка при обмене данными с ЭКЛЗ
					THROWERR(0, rError.ToLong());
				}
			}
			count++;
		}
	//}
	CATCHZOK
	return ok;
}

int PiritEquip::GetData(SString & rData, SString & rError)
{
	rData.Z();
	rError.Z();
	int    ok = 1;
	int    c = 0;
	//
	// Получаем пакет ответа
	//
	if(CommPort.GetChr(&c)) {
		// @v9.9.4 SString buf;
		SString & r_buf = SLS.AcquireRvlStr(); // @v9.9.4 
		do {
			r_buf.CatChar(c);
			c = CommPort.GetChr();
		} while(c != ETX && c != 0);
		{
			int    crc = 0;
			char   str_crc2[2];
			size_t p = 0;
			uint   i;
			// @v9.9.4 SString str_crc1;
			SString & r_str_crc1 = SLS.AcquireRvlStr(); // @v9.9.4
			r_buf.CatChar(c); // Добавили байт конца пакета
			c = CommPort.GetChr(); // Получили 1-й байт контрольной суммы
			r_buf.CatChar(c);
			c = CommPort.GetChr(); // Получили 2-й байт контрольной суммы
			r_buf.CatChar(c);
			THROW(r_buf.C(0) == STX);
			// Выделяем байты с информацией об ошибке
			r_buf.Sub(4, 2, rError);
			//
			// Считываем данные
			//
			for(i = 6; r_buf.C(i) != ETX; i++)
				rData.CatChar(r_buf.C(i));
			// Считаем контрольную сумму
			for(i = 1; i < r_buf.Len()-2; i++)
				crc ^= ((uchar)r_buf.C(i));
			r_buf.Sub(r_buf.Len()-2, 2, r_str_crc1);
			_itoa(crc, str_crc2, 16);
			// Сверяем полученную и посчитанную контрольные суммы
			THROW(r_str_crc1.CmpNC(str_crc2) == 0);
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PiritEquip::PutData(const char * pCommand, const char * pData)
{
	int    ok = 1;
	int    crc = 0;
	char   buf[2];
	int    id_pack = 0x31;
	// @v9.9.4 SString pack;
	SString & r_pack = SLS.AcquireRvlStr(); // @v9.9.4
	size_t p = 0;
	// Формируем пакет (без байтов начала и конца пакета)
	// Пароль связи (4 байта)
	r_pack.Cat(Cfg.ConnPass);
	r_pack.CatChar(id_pack); // ИД пакета
	r_pack.Cat(pCommand); // Код команды (2 байта)
	r_pack.Cat(pData); // Данные
	// Считаем контрольную сумму пакета
	for(p = 0; p < r_pack.Len(); p++) {// STX в контрольную сумму не входит
		crc ^= ((uchar)r_pack.C(p));
	}
	crc ^= ((uchar)ETX); // Учитываем в контрольной сумме байт конца пакета
	_itoa(crc, buf, 16); // @vmiler comment
	if(buf[1] == 0) {
		buf[1] = buf[0];
		buf[0] = '0';
	}
	// Отправляем пакет на ККМ
	THROW(CommPort.PutChr(STX));
	for(p = 0; p < r_pack.Len(); p++)
		THROW(CommPort.PutChr(r_pack.C(p)));
	THROW(CommPort.PutChr(ETX));
	THROW(CommPort.PutChr(buf[0]));
	THROW(CommPort.PutChr(buf[1]));
	CATCHZOK
	return ok;
}

int PiritEquip::GetStatus(SString & rStatus)
{
	int    ok = 1;
	long   status = 0;
	int    flag = 0;
	SString in_data, out_data, r_error;
	in_data = 0;
	{
		OpLogBlock __oplb(LogFileName, "04", 0);
		THROWERR(PutData("04", in_data), PIRIT_NOTSENT);
		THROW(GetWhile(out_data, r_error));
	}
	flag = out_data.ToLong();
	if(flag & 2) { // В принтере нет бумаги
		status |= NOPAPER;
	}
	else {
		if(LastStatus & NOPAPER) {
			status |= PRINTAFTERNOPAPER;
		}
		if(r_error.CmpNC("09") == 0) { // Проверка ошибки здесь, чтобы не потерять статус
			if(flag & 0x1)
				THROWERR(0, PIRIT_PRNNOTREADY)
			else if(flag & 0x4)
				THROWERR(0, PIRIT_PRNTROPENED)
			else if(flag & 0x8)
				THROWERR(0, PIRIT_PRNCUTERR)
			else if(flag & 0x80)
				THROWERR(0, PIRIT_NOCONCTNWITHPRNTR);
		}
	}
	THROW(GetCurFlags(3, flag));
	if((flag >> 4) != 0) // Открыт документ
		status |= CHECKOPENED;
	else if((flag >> 4) == 0) // Документ закрыт
		status = CHECKCLOSED;
	if(rStatus.NotEmpty())
		rStatus.Destroy();
	/* @v9.6.9 if(r_error.Empty()) // Режим печати чека
		status |= PRINT;*/
	LastStatus = status;
	// new {
	if(status == CHECKCLOSED) // Этот статус не надо передавать во внешнюю программу
		status = 0;
	// } new
	// @v9.6.9 rStatus.CatEq("STATUS", status); 
	CATCHZOK
	rStatus.CatEq("STATUS", status); // @v9.6.9
	return ok;
}

int PiritEquip::FormatPaym(double paym, SString & rStr)
{
	SString b_point, a_point;
	rStr.Z().Cat(paym);
    rStr.Divide('.', b_point, a_point);
	if(a_point.Len() > 2)
		a_point.Trim(2);
	else if(a_point.Len() == 0)
		a_point.Cat("00");
	else if(a_point.Len() == 1)
		a_point.CatChar('0');
	rStr.Z().Cat(b_point).Dot().Cat(a_point);
	return 1;
}

int PiritEquip::SetLogotype(SString & rPath, size_t size, uint height, uint width)
{
	int    ok = 1, flag = 0;
	SString in_data, out_data, r_error;
	int n = 0, r = 0;
	FILE * file = 0;

	THROWERR((height == 126) && (width <= 576), PIRIT_ERRLOGOSIZE);
	{
		OpLogBlock __oplb(LogFileName, "16", 0);
		THROWERR(PutData("16", in_data), PIRIT_NOTSENT); // Удаляем старый логотип
		Sleep(10000);
		THROW(GetWhile(out_data, r_error));
	}
	file = fopen(rPath, "rb");
	THROW(file);
	CreateStr(((int)size + 1), in_data);
	{
		OpLogBlock __oplb(LogFileName, "15", 0);
		THROWERR(PutData("15", in_data), PIRIT_NOTSENT);
		do {
			r = CommPort.GetChr();
		} while(r && r != ACK); // @v9.5.7 (r &&)
	}
	{
		OpLogBlock __oplb(LogFileName, "1B", 0);
		n = 0x1B;
		THROW(CommPort.PutChr(n));
		for(uint i = 1; i < (size + 1); i++) {
			fread(&n, sizeof(char), 1, file);
			THROW(CommPort.PutChr(n));
		}
 		THROW(GetWhile(out_data, r_error));
	}
	CATCHZOK
	SFile::ZClose(&file);
	return ok;
}

int PiritEquip::PrintLogo(int print)
{
	int    ok = 1, flag = 0;
	SString in_data, out_data, r_error;
	CreateStr(1, in_data);
	CreateStr(0, in_data);
	{
		OpLogBlock __oplb(LogFileName, "11", 0);
		THROWERR(PutData("11", in_data), PIRIT_NOTSENT);
		out_data.Destroy();
		THROW(GetWhile(out_data, r_error));
	}
	flag = out_data.ToLong();
	SETFLAG(flag, 0x04, print);
	in_data.Destroy();
	CreateStr(1, in_data);
	CreateStr(0, in_data);
	CreateStr(flag, in_data);
	{
		OpLogBlock __oplb(LogFileName, "12", 0);
		THROWERR(PutData("12", in_data), PIRIT_NOTSENT);
		THROW(GetWhile(out_data, r_error));
	}
	THROW(StartWork());
	CATCHZOK
	return ok;
}

int PiritEquip::GetDateTime(SYSTEMTIME sysDtTm, SString & rDateTime, int dt)
{
	switch(dt) {
		case 0: rDateTime.Z().CatLongZ(sysDtTm.wDay, 2).CatLongZ(sysDtTm.wMonth, 2).Cat(sysDtTm.wYear % 100); break;
		case 1: rDateTime.Z().CatLongZ(sysDtTm.wHour, 2).CatLongZ(sysDtTm.wMinute, 2).CatLongZ(sysDtTm.wSecond, 2); break;
	}
	return 1;
}

void PiritEquip::GetLastCmdName(SString & rName)
{
	if(LastCmd.IsEqiAscii("CHECKSESSOVER"))
		rName = "Проверка на длительность открытой сессии";
	else if(LastCmd.IsEqiAscii("ZREPORT"))
		rName = "Печать Z-отчета";
	else if(LastCmd.IsEqiAscii("XREPORT"))
		rName = "Печать X-отчета";
	else if(LastCmd.IsEqiAscii("OPENCHECK"))
		rName = "Открытие чека";
	else if(LastCmd.IsEqiAscii("CLOSECHECK"))
		rName = "Закрытие чека";
	else if(LastCmd.IsEqiAscii("PRINTFISCAL"))
		rName = "Печать фискальной строки";
	else if(LastCmd.IsEqiAscii("PRINTTEXT"))
		rName = "Печать текстовой строки";
	else if(LastCmd.IsEqiAscii("ANNULATE"))
		rName = "Аннулирование чека";
	else if(LastCmd.IsEqiAscii("INCASHMENT"))
		rName = "Внесение/изъятие денег";
	else
		rName = LastCmd;
}
