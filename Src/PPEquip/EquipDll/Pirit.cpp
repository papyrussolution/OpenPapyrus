// Dll для работы с ККМ Пирит
// @codepage UTF-8 // @v10.4.6
//
#pragma hdrstop
#include <slib.h>

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch(dwReason) {
		case DLL_PROCESS_ATTACH: SLS.Init("Papyrus-Drv-Pirit", static_cast<HINSTANCE>(hModule)); break;
		case DLL_THREAD_ATTACH: SLS.InitThread(); break;
		case DLL_THREAD_DETACH: SLS.ReleaseThread(); break;
		case DLL_PROCESS_DETACH: break;
	}
	return TRUE;
}

#define EXPORT	extern "C" __declspec (dllexport)
#define THROWERR(expr,val)     {if(!(expr)){SetError(val);goto __scatch;}}

//typedef unsigned char  uint8;

int	   ErrorCode = 0;
char   FS = 0x1C;
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// Для формирования пакета данных
#define	STX		0x02	// Начало пакета
#define	ETX		0x03	// Конец пакета

// Коды команд ККМ
#define ENQ	0x05	// Проверка связи
#define ACK	0x06	// ККМ на связи
#define CAN	0x18	// Прервать выполнение отчета

// Коды ошибок
#define PIRIT_ERRSTATUSFORFUNC     1 // 01h Функция невыполнима при данном статусе ККМ
#define PIRIT_ERRFUNCNUMINCOMMAND  2 // 02h В команде указан неверный номер функции
#define PIRIT_ERRCMDPARAMORFORMAT  3 // 03h Некорректный формат или параметр команды
#define PIRIT_ENDOFPAPER           8 // 08h Конец бумаги
#define PIRIT_PRNNOTREADY          9 // 09h Принтер не готов
#define PIRIT_SESSOVER24H         10 // 0Ah Текущая смена больше 24 часов. Установка даты времени больше чем на 24 часа.
#define PIRIT_DIFDATE             11 // Дата и время на ККМ отличаются от системных на 8 минут
#define PIRIT_DATELSLASTOP        12 // Системная дата меньше даты последней фискальной операции, зарегистрированной в ККМ
#define PIRIT_FATALERROR          32 // 20h Фатальная ошибка ККМ
#define PIRIT_FMOVERFLOW          33 // 21h Нет свободного места в фискальной памяти ККМ
#define PIRIT_OFDPROVIDER         54 // Ошибка оператора фискальных данных
#define PIRIT_ECRRFORMAT          65 // 41h Некорректный формат или параметр команды ЭКЛЗ
#define PIRIT_ECRACCIDENT         67 // 43h Авария ЭКЛЗ
#define PIRIT_KCACCIDENT          68 // 44h Авария КС (криптографического сопроцессора)в составе ЭКЛЗ
#define PIRIT_ECRTIMEOUT          69 // 45h Исчерпан временной ресурс использования ЭКЛЗ
#define PIRIT_ECROVERFLOW         70 // 46h ЭКЛЗ переполнена
#define PIRIT_ECRERRORDATETIME    71 // 47h Неверные дата или время
#define PIRIT_ECRNODATA           72 // 48h Нет запрошенных данных
#define PIRIT_ECRTOOMUCH          73 // 49h Переполнение (отрицательный итог документа, слишком много отделов для клиента)
#define PIRIT_NOANSWER            74 // 4Ah Нет ответа от ЭКЛЗ
#define PIRIT_ECRERREXCHANGE      75 // 4Bh Ошибка при обмене данными с ЭКЛЗ

#define PIRIT_NOTENOUGHPARAM	300	// Не достаточно параметров для работы устройства
#define PIRIT_UNCNKOWNCOMMAND	301	// Передана неизвестная команда
#define PIRIT_NOTINITED			302	// Ошибка инициализации
#define PIRIT_NOTCONNECTED		303	// Соединение не установлено

#define PIRIT_ECRERRORSTATUS            401	// Некорректное состояние ЭКЛЗ
#define PIRIT_ECRFMOVERFLOW             402	// ЭКЛЗ или ФП переполнена
#define PIRIT_ECRFATALERR               403	// Ошибка ЭКЛЗ. Обратитесь в ЦТО

#define PIRIT_NOTSENT                   500	// Ошибка передачи данных
#define PIRIT_NOTENOUGHMEM              501	// Недостаточный размер выходного массива
#define PIRIT_ERRLOGOSIZE               502	// Изображение должно иметь размеры: ширина - не более 576 точек, высота - 126 точек
#define PIRIT_ERRLOGOFORMAT             503	// Изображение должно быть монохромным в формате BMP
#define PIRIT_ECRARCHOPENED             504 // Архив ЭКЛЗ закрыт
#define PIRIT_ECRNOTACTIVE              505 // ЭКЛЗ не активирована
#define PIRIT_NOTENOUGHTMEMFORSESSCLOSE	506 // Нет памяти для закрытия смены в ФП
#define PIRIT_ERRFMPASS                 507 // Был введен неверный пароль доступа к ФП
#define PIRIT_SESSOPENEDTRYAGAIN        508 // Не было завершено закрытие смены, необходимо повторить операцию
#define PIRIT_PRNTROPENED               509 // 2 Открыта крышка принтера
#define PIRIT_PRNCUTERR                 510 // 3 Ошибка резчика принтера
#define PIRIT_NOCONCTNWITHPRNTR         511 // 7 Нет связи с принтером
//
// Значения флагов статуса
//
#define NOPRINT					0x00	// Нет печати
#define NOPAPER					0x01	// В принтере нет бумаги
#define PRINTAFTERNOPAPER		0x02	// Печатать после окончания ленты
#define PRINT					0x04	// Печать
#define CHECKOPENED				0x08	// Открыт чек
#define CHECKCLOSED				0x10	// Чек закрыт

struct Config {
	Config() : CashID(0), Name(0), LogNum(0), Port(0), BaudRate(0), DateTm(MAXDATETIME), Flags(0), ConnPass("PIRI"), ReadCycleCount(10), ReadCycleDelay(10)
	{
	}
	struct LogoStruct {
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
	CheckStruct() : CheckType(2), FontSize(3), CheckNum(0), Quantity(0.0), Price(0.0), Department(0), Ptt(0), Stt(0), TaxSys(0), Tax(0), 
		PaymCash(0.0), PaymBank(0.0), IncassAmt(0.0), ChZnProdType(0) //@erik v10.4.12 add "Stt(0),"
	{
	}
	void Clear()
	{
		FontSize = 3;
		Quantity = 0.0;
		Price = 0.0;
		Department = 0;
		Tax = 0;
		Ptt = 0; // @v10.4.1
		Stt = 0; // @erik v10.4.12
		// @v9.9.4 if(Text.NotEmpty())
		// @v9.9.4 	Text.Destroy();
		TaxSys = -1; // @v10.6.3 // @v10.6.4 0-->-1
		Text.Z(); // @v9.9.4
		Code.Z(); // @v9.9.4
		ChZnCode.Z(); // @v10.6.12
		ChZnGTIN.Z(); // @v10.7.2
		ChZnSerial.Z(); // @v10.7.2
		ChZnPartN.Z(); // @v10.7.8
		PaymCash = 0.0;
		PaymBank = 0.0;
		IncassAmt = 0.0;
		ChZnProdType = 0; // @v10.7.2
	}
	int    CheckType;
	int    FontSize;
	int    CheckNum;
	double Quantity;
	double Price;
	int    Department;
	// 
	// Система налогообложения
	// 0 	Основная
	// 1 	Упрощенная Доход
	// 2 	Упрощенная Доход минус Расход
	// 3 	Единый налог на вмененный доход
	// 4 	Единый сельскохозяйственный налог
	// 5 	Патентная система налогообложения
	// 
	int    TaxSys; // @v10.6.3 Система налогообложения
	int    Tax;
	int    Ptt; // @v10.4.1 // CCheckPacket::PaymentTermTag
	int    Stt; // @erik v10.4.12
	SString Text;
	SString Code; // @v9.5.7
	SString ChZnCode; // @v10.6.12
	SString ChZnGTIN; // @v10.7.2
	SString ChZnSerial; // @v10.7.2
	SString ChZnPartN;  // @v10.7.8
	int    ChZnProdType; // @v10.7.2
	double PaymCash;
	double PaymBank;
	double PaymCCrdCard;
	double IncassAmt;
};

class PiritEquip {
public:
	PiritEquip() : SessID(0), LastError(0), FatalFlags(0), LastStatus(0), RetTknzr("\x1c")
	{
		// @v10.0.12 {
		{
			CommPortTimeouts cpt;
			CommPort.GetTimeouts(&cpt);
			cpt.Get_Delay = 20;
			CommPort.SetTimeouts(&cpt);
		}
		// } @v10.0.12 
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
	~PiritEquip()
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
	int    ReturnCheckParam(const SString & rInput, char * output, size_t size);
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
		OpLogBlock(const char * pLogFileName, const char * pOp, const char * pExtMsg) : StartClk(clock()), Op(pOp), ExtMsg(pExtMsg), LogFileName(pLogFileName)
		{
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
		DvcTaxEntry() : Rate(0.0)
		{
			PTR32(Name)[0] = 0;
		}
		char   Name[64];
		double Rate;
	};
	DvcTaxEntry DvcTaxArray[8];

	int    ENQ_ACK();
	int    FormatPaym(double paym, SString & rStr);
	int    SetLogotype(SString & rPath, size_t size, uint height, uint width);
	int    PrintLogo(int print);
	void   GetDateTime(SYSTEMTIME sysDtTm, SString & rDateTime, int dt); // dt = 0 - возвращает форматировнную дату, dt = 1 - время //
	//
	// Descr: Получает таблицу налогов из настроек аппарата (DvcTaxArray)
	//
	int    GetTaxTab();
	int    IdentifyTaxEntry(double vatRate, int isVatFree) const;
	// @v10.4.6 void   GetLastCmdName(SString & rName) const;
	void   SetLastItems(const char * pCmd, const char * pParam);
	int    ReadConfigTab(int arg1, int arg2, SString & rOut, SString & rError);
	int    WriteConfigTab(int arg1, int arg2, int val, SString & rOut, SString & rError);
	int    ExecCmd(const char * pHexCmd, const char * pInput, SString & rOut, SString & rError);

	SString LogFileName;
	StringSet RetTknzr;
};

static PiritEquip * P_Pirit = 0;

static const SIntToSymbTabEntry Pirit_ErrMsg[] = {
	{PIRIT_ERRSTATUSFORFUNC, "Функция невыполнима при данном статусе ККМ"},
	{PIRIT_ERRFUNCNUMINCOMMAND, "В команде указан неверный номер функции"},
	{PIRIT_ERRCMDPARAMORFORMAT, "Некорректный формат или параметр команды"},
	{PIRIT_ENDOFPAPER,       "Конец бумаги"},
	{PIRIT_PRNNOTREADY,		 "Принтер не готов"},
	{PIRIT_SESSOVER24H,      "Текущая смена больше 24 часов. Установка даты времени больше чем на 24 часа."},
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
	{PIRIT_NOCONCTNWITHPRNTR,	 "Нет связи с принтером"},
	{0x50, "Превышен размер данных TLV" },
	{0x51, "Нет транспортного соединения" },
	{0x52, "Исчерпан ресурс КС" },
	{0x54, "Исчерпана память хранения документов для ОФД" },
	{0x55, "Время нахождения в очереди самого старого сообщения на выдачу более 30 календарных дней." },
	{0x56, "Продолжительность смены ФН более 24 часов" },
	{0x57, "Разница более чем на 5 минут отличается от разницы, определенной по внутреннему таймеру ФН." },
	{0x60, "Неверное сообщение от ОФД" },
	{0x61, "Нет связи с ФН" },
	{0x62, "Ошибка обмена с ФН" },
	{0x63, "Слишком длинная команда для посылки в ФН" },
};
/*
	0x00 	Команда выполнена без ошибок

	Ошибки выполнение команд
	0x01 	Функция невыполнима при данном статусе ККТ
	0x02 	В команде указан неверный номер функции
	0x03 	Некорректный формат или параметр команды

	Ошибки протокола передачи данных
	0x04 	Переполнение буфера коммуникационного порта
	0x05 	Таймаут при передаче байта информации
	0x06 	В протоколе указан неверный пароль
	0x07 	Ошибка контрольной суммы в команде

	Ошибки печатающего устройства
	0x08 	Конец бумаги
	0x09 	Принтер не готов

	Ошибки даты/времени
	0x0A 	Текущая смена больше 24 часов. Установка даты времени больше чем на 24 часа.
	0x0B 	Разница во времени, ККТ и указанной в команде начала работы, больше 8 минут
	0x0C 	Вводимая дата более ранняя, чем дата последней фискальной операции

	Прочие ошибки
	0x0E 	Отрицательный результат
	0x0F 	Для выполнения команды необходимо закрыть смену
	0x10 	Нет данных в журнале
	0x11 	Ошибка контрольной ленты
	0x12 	Ошибка посылки данных в ОФД

	Фатальные ошибки
	0x20 	Фатальная ошибка ККТ. Причины возникновения данной ошибки можно уточнить в ”Статусе фатальных ошибок ККТ” 

	Ошибки ФН
	0x41 	Некорректный формат или параметр команды ФН
	0x42 	Некорректное состояние ФН
	0x43 	Ошибка ФН
	0x44 	Ошибка КС (Криптографического сопроцессора) в составе ФН
	0x45 	Исчерпан временной ресурс использования ФН
	0x46 	ФН переполнен
	0x47 	Неверные дата или время
	0x48 	Нет запрошенных данных
	0x49 	Некорректные параметры команды
	0x50 	Превышен размер данных TLV
	0x51 	Нет транспортного соединения
	0x52 	Исчерпан ресурс КС
	0x54 	Исчерпана память хранения документов для ОФД
	0x55 	Время нахождения в очереди самого старого сообщения на выдачу более 30 календарных дней.
	0x56 	Продолжительность смены ФН более 24 часов
	0x57 	Разница более чем на 5 минут отличается от разницы, определенной по внутреннему таймеру ФН.
	0x60 	Неверное сообщение от ОФД
	0x61 	Нет связи с ФН
	0x62 	Ошибка обмена с ФН
	0x63 	Слишком длинная команда для посылки в ФН
*/

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

int PiritEquip::IdentifyTaxEntry(double vatRate, int isVatFree) const
{
	int    tax_entry_n = 0;
	int    tax_entry_id_result = -1;
	double _vat_rate = vatRate;
	{
		if(isVatFree)
			_vat_rate = 0.0;
		tax_entry_id_result = 0;
		// Освобожденные от НДС продавцы передают признак VATFREE но таблица не предусматривает такого элемента.
		// По этому, используя эмпирическое правило, считаем, что второй элемент таблицы, имеющий нулевую
		// ставку относится к VATFREE-продажам.
		uint   zero_entry_1 = 0; 
		uint   zero_entry_2 = 0; 
		for(uint tidx = 0; tidx < SIZEOFARRAY(DvcTaxArray); tidx++) {
			if(DvcTaxArray[tidx].Name[0] || tidx == 3) { // @v10.0.0 (|| tidx == 3) костыль - это "освобожден от ндс" в трактовке Дрим Касс
				const double entry_rate = DvcTaxArray[tidx].Rate;
				if(entry_rate == 0.0 && _vat_rate == 0.0) {
					if(!zero_entry_1) {
						zero_entry_1 = tidx+1;
						if(!isVatFree) {
							tax_entry_n = (int)tidx;
							tax_entry_id_result = tidx+1;
							break;
						}
					}
					else if(!zero_entry_2) {
						zero_entry_2 = tidx+1;
						if(isVatFree) {
							tax_entry_n = (int)tidx;
							tax_entry_id_result = tidx+1;
							break;
						}
					}
				}
				else if(feqeps(entry_rate, _vat_rate, 1E-5)) {
					tax_entry_n = (int)tidx;
					tax_entry_id_result = tidx+1;
					break;
				}
			}
		}
		if(!tax_entry_id_result) {
			if(isVatFree && zero_entry_1) {
				tax_entry_n = (int)(zero_entry_1-1);
				tax_entry_id_result = zero_entry_1;
			}
			else {
				//
				// Не нашли в таблице того, чего искали: включаем default-вариант, основанный на документации к драйверу
				//
				if(_vat_rate == 18.0 || _vat_rate == 20.0) { // @v10.2.10 (|| _vat_rate == 20.0)
					tax_entry_n = 0;
					tax_entry_id_result = 1;
				}
				else if(_vat_rate == 10.0) {
					tax_entry_n = 1;
					tax_entry_id_result = 2;
				}
				else if(_vat_rate == 0.0) {
					if(!isVatFree) {
						tax_entry_n = 2;
						tax_entry_id_result = 3;
					}
					else {
						tax_entry_n = 3;
						tax_entry_id_result = 4;
					}
				}
			}
		}
	}
	// @debug {
	if(LogFileName.NotEmpty()) {
		SString temp_buf;
		if(tax_entry_id_result < 0)
			(temp_buf = "TaxEntry isn't identified").CatDiv(':', 2).Cat(_vat_rate);
		else if(tax_entry_id_result == 0)
			(temp_buf = "TaxEntry isn't found").CatDiv(':', 2).Cat(_vat_rate);
		else if(tax_entry_id_result > 0)
			(temp_buf = "TaxEntry is found").CatDiv(':', 2).Cat(tax_entry_id_result-1).CatDiv(',', 2).Cat(_vat_rate);
		SLS.LogMessage(LogFileName, temp_buf);
	}
	// } @debug 
	return tax_entry_n;
}

int PiritEquip::ExecCmd(const char * pHexCmd, const char * pInput, SString & rOut, SString & rError)
{
	int    ok = 1;
	OpLogBlock __oplb(LogFileName, pHexCmd, 0);
	THROWERR(PutData(pHexCmd, pInput), PIRIT_NOTSENT);
	THROW(GetWhile(rOut, rError));
	CATCHZOK
	return ok;
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
	SString temp_buf;
	if(sstreqi_ascii(pCmd, "CONTINUEPRINT")) {
		if(!LastCmd.IsEqiAscii("PRINTFISCAL") && !LastCmd.IsEqiAscii("PRINTTEXT")) { // new
			cmd = LastCmd;
			temp_buf = LastParams;
			LastCmd.Z();
			LastParams.Z();
		}
	}
	else {
		cmd = pCmd;
		temp_buf = pInputData;
	}
	StringSet pairs(';', temp_buf);
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
			// @v10.2.5 THROWERR(!(flag & 0x0010), PIRIT_ECRARCHOPENED); // Архив ЭКЛЗ закрыт
			// @v10.2.5 THROWERR(!(flag & 0x0020), PIRIT_ECRNOTACTIVE); // ЭКЛЗ не активирована
			// @v10.2.3 THROWERR(!(flag & 0x0080), PIRIT_ERRFMPASS); // Был введен неверный пароль доступа к ФП
			// @v10.4.1 THROWERR(!(flag & 0x0100), PIRIT_SESSOPENEDTRYAGAIN); // Не было завершено закрытие смены, необходимо повторить операцию
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
			/*
				Настройки ККТ
				1 - Параметры ПУ (битовая маска)
				2 - Параметры чека (битовая маска)
				3 - Параметры отчета о закрытии смены (битовая маска)
				4 - Управление внешними устройствами (битовая маска)
				5 - Управление расчетами (битовая маска)
				6 - Управление расчетами и печатью налогов (битовая маска)
				10 - Логический номер ККТ
				11 - Дополнительная ячейка
				20 - Пароль для связи
				30 - Наименование и адрес организации (Массив 0..3)
				31 - Строки окончания чеков (Массив 0..4)
				32 - Названия типов платежей (Массив 0..15)
				40 - Название ставки налога (Массив 0..5)
				41 - Процент ставки налога (Массив 0..5)
				42 - Название налоговой группы в чеке
				50 - Наименование отдела/секции (Массив 1..16)
				51 - Название группы отделов/секции на отчете о закрытии
				52 - Наименование реквизита (Массив 1..5)
				54 - Реквизиты ЦТО (Массив 0..1)
				70 - Номер автомата
				71 - ИНН ОФД
				72 - Содержание QR-кода
				73 - IP-адрес ККТ
				74 - Маска подсети
				75 - IP-адрес шлюза
				76 - IP-адрес DNS
				77 - Адрес сервера ОФД для отправки документов
				78 - Порт сервера ОФД
				79 - Таймер ФН
				80 - Таймер С
				81 - Наименование ОФД
				82 - Электронная почта отправителя чека
				83 - URL сайта ФНС
				85 - Место расчетов
			*/
			SetLastItems(0, 0);
			str.Z();
			if(ReadConfigTab(10, 0, out_data, r_error) && out_data.NotEmptyS()) {
				RetTknzr.setBuf(out_data);
				if(RetTknzr.get(0U, temp_buf))
					str.CatEq("LOGNUM", temp_buf);
			}
			str.CatDivIfNotEmpty(';', 0).CatEq("CHECKSTRLEN", "130"); // @v9.1.8 44-->130
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
			THROW(ExecCmd("21", str, out_data, r_error));
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
			THROW(ExecCmd("20", str, out_data, r_error));
		}
		else if(cmd.IsEqiAscii("CUT")) { // @v10.2.2
			SetLastItems(cmd, pInputData);
			THROW(ExecCmd("34", str, out_data, r_error));
		}
		else if(cmd.IsEqiAscii("OPENCHECK")) {
			SetLastItems(cmd, pInputData);
			THROW(StartWork());
			for(uint i = 0; pairs.get(&i, s_pair) > 0;) {
				s_pair.Divide('=', s_param, param_val);
				if(s_param.IsEqiAscii("CHECKTYPE")) {
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
				else if(s_param.IsEqiAscii("CHECKNUM"))
					Check.CheckNum = param_val.ToLong();
				else if(s_param.IsEqiAscii("TAXSYSTEM")) {
					int    pp_tax_sys_id = param_val.ToLong();
					// Pirit values of tax system:
					// 0 	Основная
					// 1 	Упрощенная Доход
					// 2 	Упрощенная Доход минус Расход
					// 3 	Единый налог на вмененный доход
					// 4 	Единый сельскохозяйственный налог
					// 5 	Патентная система налогообложения
					switch(pp_tax_sys_id) {
						case /*TAXSYSK_GENERAL*/1: Check.TaxSys = 0; break;
						case /*TAXSYSK_SIMPLIFIED*/2: Check.TaxSys = 1; break;
						case /*TAXSYSK_PATENT*/3: Check.TaxSys = 5; break;
						case /*TAXSYSK_IMPUTED*/4: Check.TaxSys = 3; break;
						case /*TAXSYSK_SINGLEAGRICULT*/5: Check.TaxSys = 4; break;
						case /*TAXSYSK_SIMPLIFIED_PROFIT*/6: Check.TaxSys = 2; break; // @v10.6.4
						default: Check.TaxSys = -1; break; // @v10.6.4 0-->-1
					}
				}
			}
			THROW(RunCheck(0));
		}
		else if(cmd.IsEqiAscii("CLOSECHECK")) {
			SetLastItems(cmd, pInputData);
			// @v10.1.2 THROW(StartWork());
			Check.PaymCash = 0.0;
			Check.PaymBank = 0.0;
			Check.PaymCCrdCard = 0.0; // @v10.4.6
			for(uint i = 0; pairs.get(&i, s_pair) > 0;) {
				s_pair.Divide('=', s_param, param_val);
				if(s_param.IsEqiAscii("PAYMCASH"))
					Check.PaymCash = param_val.ToReal();
				else if(s_param.IsEqiAscii("PAYMCARD"))
					Check.PaymBank = param_val.ToReal();
				else if(s_param.IsEqiAscii("PAYMCCRD")) // @v10.4.6
					Check.PaymCCrdCard = param_val.ToReal();
			}
			THROW(RunCheck(1));
		}
		else if(cmd.IsEqiAscii("CHECKCORRECTION")) { // @v10.0.0
			struct CheckCorrectionBlock {
				CheckCorrectionBlock()
				{
					THISZERO();
					VatRate = -1.0;
				}
				char   OperName[32]; // CshrName
				double CashAmt;
				double BankAmt;
				double PrepayAmt;
				double PostpayAmt;
				double ReckonAmt; // Сумма встречным представлением
				int    Type;
				LDATE  Dt;
				char   DocNo[32];
				char   DocMemo[64];
				double Vat20Amt; // @v10.4.8
				double Vat18Amt;
				double Vat10Amt;
				double Vat0Amt;
				double VatFreeAmt;
				double Vat18_118Amt;
				double Vat10_110Amt;
				int    IsVatFree;
				double VatRate;
			};
			int    is_there_vatamt = 0;
			CheckCorrectionBlock blk;
			STRNSCPY(blk.OperName, CshrName);
			for(uint i = 0; pairs.get(&i, s_pair) > 0;) {
				s_pair.Divide('=', s_param, param_val);
				s_param.ToUpper();
				if(s_param == "PAYMCASH")
					blk.CashAmt = R2(param_val.ToReal());
				else if(s_param == "PAYMCARD")
					blk.BankAmt = R2(param_val.ToReal());
				else if(s_param == "PREPAY") {
				}
				else if(s_param == "POSTPAY") {
				}
				else if(s_param == "RECKONPAY") {
				}
				else if(s_param == "CODE") {
					STRNSCPY(blk.DocNo, param_val);
				}
				else if(s_param == "DATE") {
					blk.Dt = strtodate_(param_val, DATF_ISO8601|DATF_CENTURY);
				}
				else if(s_param == "TEXT") {
					STRNSCPY(blk.DocMemo, param_val);
				}
				else if(s_param == "VATRATE") {
					blk.VatRate = R2(param_val.ToReal());
				}
				else if(s_param == "VATFREE") {
					if(param_val.Empty() || param_val.IsEqiAscii("yes") || param_val.IsEqiAscii("true") || param_val == "1") 
						blk.IsVatFree = 1;
				}
				//
				else if(s_param == "VATAMOUNT20") {
					blk.Vat20Amt = R2(param_val.ToReal());
					if(blk.Vat20Amt != 0.0)
						is_there_vatamt = 1;
				}
				else if(s_param == "VATAMOUNT18") {
					blk.Vat18Amt = R2(param_val.ToReal());
					if(blk.Vat18Amt != 0.0)
						is_there_vatamt = 1;
				}
				else if(s_param == "VATAMOUNT10") {
					blk.Vat10Amt = R2(param_val.ToReal());
					if(blk.Vat10Amt != 0.0)
						is_there_vatamt = 1;
				}
				else if(s_param == "VATAMOUNT00" || s_param == "VATAMOUNT0") {
					blk.Vat0Amt = R2(param_val.ToReal());
					if(blk.Vat0Amt != 0.0)
						is_there_vatamt = 1;
				}
				else if(s_param == "VATFREEAMOUNT") {
					blk.VatFreeAmt = R2(param_val.ToReal());
					if(blk.VatFreeAmt != 0.0)
						is_there_vatamt = 1;
				}
			}
			if(!is_there_vatamt) {
				const double _amount = blk.BankAmt + blk.CashAmt;
				if(blk.VatRate == 20.0)
					blk.Vat20Amt = _amount;
				else if(blk.VatRate == 18.0)
					blk.Vat18Amt = _amount;
				else if(blk.VatRate == 10.0)
					blk.Vat10Amt = _amount;
				else if(blk.VatRate == 0.0)
					blk.Vat0Amt = _amount;
			}
			if(!checkdate(blk.Dt))
				blk.Dt = getcurdate_();
			{
				int    correction_type = 0;
				// 
				// Тип коррекции
				// Номер бита 	Пояснение
				// 0 	0 - самостоятельная коррекция, 1 - коррекция по предписанию
				// 1 	0 - приход, 1 - расход
				// 2..4 	Система налогообложения (см. ниже)
				// 5 	Если бит = 0, то вместо суммы налога в поле "Сумма налога по ставке 18%" 
				//   передается номер ставки налога, по которой исчисляется сумма налога, остальные суммы налогов не воспринимаются
				// 
				// Система налогообложения
				// Значение 	Пояснение
				// 0 	Основная
				// 1 	Упрощенная Доход
				// 2 	Упрощенная Доход минус Расход
				// 3 	Единый налог на вмененный доход
				// 4 	Единый сельскохозяйственный налог
				// 5 	Патентная система налогообложения
				// 
				int vat_entry_n = (blk.IsVatFree || blk.VatRate >= 0.0) ? IdentifyTaxEntry(blk.VatRate, blk.IsVatFree) : -1;
				if(vat_entry_n >= 0)
					correction_type &= ~0x10;
				else
					correction_type |= 0x10;
				if(blk.CashAmt < 0.0 || blk.BankAmt < 0.0 || blk.PrepayAmt < 0.0)
					correction_type |= 0x02;
				else
					correction_type &= ~0x02;

				SString in_data;
				// 
				// CMD 0x58
				// Parameters:
				//   (Имя оператора) Имя оператора
				//   (Дробное число) Сумма наличного платежа        "PAYMCASH"
				//   (Дробное число) Сумма электронного платежа     "PAYMCARD"
				//   (Дробное число) Сумма предоплатой              "PREPAY"
				//   (Дробное число) Сумма постоплатой              "POSTPAY"
				//   (Дробное число) Сумма встречным представлением "RECKONPAY"
				//   (Число) Тип коррекции
				//   (Дата) Дата документа основания коррекции           "DATE" 
				//   (Строка[1..32]) Номер документа основания коррекции "CODE"
				//   (Строка[1..64]) Наименование основания коррекции    "TEXT"
				//   (Дробное число) Сумма налога по ставке 18%          "VATAMOUNT18"
				//   (Дробное число) Сумма налога по ставке 10%          "VATAMOUNT10"
				//   (Дробное число) Сумма расчета по ставке 0%          "VATAMOUNT00"
				//   (Дробное число) Сумма расчета без налога            "VATFREEAMOUNT"
				//   (Дробное число) Сумма расчета по расч. ставке 18/118
				//   (Дробное число) Сумма расчета по расч. ставке 10/110
				//   
				CreateStr(CshrName, in_data);
				CreateStr(fabs(blk.CashAmt), in_data);
				CreateStr(fabs(blk.BankAmt), in_data);
				CreateStr(fabs(blk.PrepayAmt), in_data);
				CreateStr(fabs(blk.PostpayAmt), in_data);
				CreateStr(fabs(blk.ReckonAmt), in_data);
				CreateStr(correction_type, in_data);
				CreateStr(temp_buf.Z().Cat(blk.Dt, DATF_DMY|DATF_NODIV), in_data);
				CreateStr(blk.DocNo, in_data);
				CreateStr(blk.DocMemo, in_data);
				if(vat_entry_n >= 0) {
					CreateStr(vat_entry_n, in_data);
					CreateStr(0.0, in_data);
					CreateStr(0.0, in_data);
					CreateStr(0.0, in_data);
					CreateStr(0.0, in_data);
					CreateStr(0.0, in_data);
				}
				else {
					// @v10.4.8 CreateStr(fabs(blk.Vat18Amt), in_data);
					CreateStr(fabs(blk.Vat20Amt), in_data); // @v10.4.8 
					CreateStr(fabs(blk.Vat10Amt), in_data);
					CreateStr(fabs(blk.Vat0Amt), in_data);
					CreateStr(fabs(blk.VatFreeAmt), in_data);
					CreateStr(fabs(blk.Vat18_118Amt), in_data);
					CreateStr(fabs(blk.Vat10_110Amt), in_data);
				}
				CreateStr("", in_data); // @v10.4.8 Дополнительный реквизит чека (БСО)
				//
				{
					// 19/06/2019 11:51:02	58	start	Master197850000211061900001возврат нал. от 28/03300000
					OpLogBlock __oplb(LogFileName, "58", in_data); // @v10.4.10
					THROW(ExecCmd("58", in_data, out_data, r_error));
				}
			}
		}
		else if(cmd.IsEqiAscii("PRINTFISCAL")) {
			double _vat_rate = 0.0;
			int   is_vat_free = 0;
			SetLastItems(cmd, 0);
			// @v10.1.2 THROW(StartWork());
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
				else if(s_param == "CHZNCODE") // @v10.6.12
					Check.ChZnCode = param_val;
				else if(s_param == "CHZNGTIN") // @v10.7.2
					Check.ChZnGTIN = param_val;
				else if(s_param == "CHZNSERIAL") // @v10.7.2
					Check.ChZnSerial = param_val;
				else if(s_param == "CHZNPARTN") // @v10.7.8
					Check.ChZnPartN = param_val;
				else if(s_param == "CHZNPRODTYPE") // @v10.7.2
					Check.ChZnProdType = param_val.ToLong();
				else if(s_param == "VATRATE") { // @v9.7.1
					_vat_rate = R2(param_val.ToReal());
				}
				else if(s_param == "VATFREE") { // @v9.8.9
					if(param_val.Empty() || param_val.IsEqiAscii("yes") || param_val.IsEqiAscii("true") || param_val == "1") 
						is_vat_free = 1;
				}
				else if(s_param == "PAYMENTTERMTAG") { // @v10.4.1
					if(param_val.IsEqiAscii("PTT_FULL_PREPAY"))
						Check.Ptt = 1;
					else if(param_val.IsEqiAscii("PTT_PREPAY"))
						Check.Ptt = 2;
					else if(param_val.IsEqiAscii("PTT_ADVANCE"))
						Check.Ptt = 3;
					else if(param_val.IsEqiAscii("PTT_FULLPAYMENT"))
						Check.Ptt = 4;
					else if(param_val.IsEqiAscii("PTT_PARTIAL"))
						Check.Ptt = 5;
					else if(param_val.IsEqiAscii("PTT_CREDITHANDOVER"))
						Check.Ptt = 6;
					else if(param_val.IsEqiAscii("PTT_CREDIT"))
						Check.Ptt = 7;
				}
				//@erik v10.4.12{
				else if(s_param == "SUBJTERMTAG") {
					if(param_val.IsEqiAscii("STT_GOOD"))
						Check.Stt = 1;
					else if(param_val.IsEqiAscii("STT_EXCISABLEGOOD"))
						Check.Stt = 2;
					else if(param_val.IsEqiAscii("STT_EXECUTABLEWORK"))
						Check.Stt = 3;
					else if(param_val.IsEqiAscii("STT_SERVICE"))
						Check.Stt = 4;
					else if(param_val.IsEqiAscii("STT_BETTING"))
						Check.Stt = 5;
					else if(param_val.IsEqiAscii("STT_PAYMENTGAMBLING"))
						Check.Stt = 6;
					else if(param_val.IsEqiAscii("STT_BETTINGLOTTERY"))
						Check.Stt = 7;
					else if(param_val.IsEqiAscii("STT_PAYMENTLOTTERY"))
						Check.Stt = 8;
					else if(param_val.IsEqiAscii("STT_GRANTSRIGHTSUSEINTELLECTUALACTIVITY"))
						Check.Stt = 9;
					else if(param_val.IsEqiAscii("STT_ADVANCE"))
						Check.Stt = 10;
					else if(param_val.IsEqiAscii("STT_PAYMENTSPAYINGAGENT"))
						Check.Stt = 11;
					else if(param_val.IsEqiAscii("STT_SUBJTERM"))
						Check.Stt = 12;
					else if(param_val.IsEqiAscii("STT_NOTSUBJTERM"))
						Check.Stt = 13;
					else if(param_val.IsEqiAscii("STT_TRANSFERPROPERTYRIGHTS"))
						Check.Stt = 14;
					else if(param_val.IsEqiAscii("STT_NONOPERATINGINCOME"))
						Check.Stt = 15;
					else if(param_val.IsEqiAscii("STT_EXPENSESREDUCETAX"))
						Check.Stt = 16;
					else if(param_val.IsEqiAscii("STT_AMOUNTMERCHANTFEE"))
						Check.Stt = 17;
					else if(param_val.IsEqiAscii("STT_RESORTAEE"))
						Check.Stt = 18;
					else if(param_val.IsEqiAscii("STT_DEPOSIT"))
						Check.Stt = 19;
				}
				// } @erik v10.4.12
			}
			{
				if(Check.Price <= 0.0) {
					Check.Price = 0.0;
					is_vat_free = 1;
				}
				Check.Tax = IdentifyTaxEntry(_vat_rate, is_vat_free);
			}
			THROW(RunCheck(2));
		}
		else if(cmd.IsEqiAscii("PRINTTEXT")) {
			SetLastItems(cmd, 0);
			//LastParams = pInputData;
			//LastCmd = 0;
			// @v10.1.0 THROW(StartWork());
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
				THROW(ExecCmd("41", in_data, out_data, r_error));
			}
		}
		else if(cmd.IsEqiAscii("GETCHECKPARAM")) {
			SetLastItems(0, 0);
			str.Z();
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
			// @v10.1.9 THROW(StartWork());
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
		// @v10.8.1 {
		else if(cmd.IsEqiAscii("GETDEVICETIME")) {
			SString date_buf, time_buf;
			SString in_data, out_data, r_error;
			THROW(ExecCmd("13", in_data, out_data, r_error)); // Смотрим текщую дату/время на ККМ
			out_data.Divide(FS, date_buf, time_buf);
			LDATETIME dtm = ZERODATETIME;
			strtodate(date_buf, DATF_DMY|DATF_NODIV, &dtm.d);
			strtotime(time_buf, TIMF_HMS|TIMF_NODIV, &dtm.t);
			str.Z().Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0);
			memcpy(pOutputData, str, outSize);
			//CashDateTime.Z().Cat("Текущая дата на ККМ").CatDiv(':', 2).Cat(date).Space().Cat("Текущее время на ККМ").CatDiv(':', 2).Cat(time);
		}
		// } @v10.8.1 
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

#if 0 // @v10.4.6 {
void PiritEquip::GetLastCmdName(SString & rName) const
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
	else if(LastCmd.IsEqiAscii("CHECKCORRECTION"))
		rName = "Печать чека коррекции (0x58)";
	else
		rName = LastCmd;
}
#endif // } 0 @v10.4.6

SString & PiritEquip::LastErrorText(SString & rMsg)
{
	SIntToSymbTab_GetSymb(Pirit_ErrMsg, SIZEOFARRAY(Pirit_ErrMsg), LastError, rMsg);
	if(LastError == PIRIT_FATALERROR) {
		char str[16];
		memzero(str, sizeof(str));
		rMsg.Space().Cat(_itoa(FatalFlags, str, 10));
	}
	else if(LastError == PIRIT_DATELSLASTOP)
		rMsg.Cat(CashDateTime);
	else if(LastError == PIRIT_ERRSTATUSFORFUNC) {
		// @v9.6.9 GetStatus(status_str);
		//SString status_str = "unkn"; // @v9.6.9
		if(LastStatus == CHECKOPENED)
			rMsg.CatDiv(':', 2).Cat("Чек открыт");
		else if(LastStatus == CHECKCLOSED)
			rMsg.CatDiv(':', 2).Cat("Чек закрыт");
		{
			SString cmd_str;
			//GetLastCmdName(cmd_str);
			if(LastCmd.IsEqiAscii("CHECKSESSOVER"))
				cmd_str = "Проверка на длительность открытой сессии";
			else if(LastCmd.IsEqiAscii("ZREPORT"))
				cmd_str = "Печать Z-отчета";
			else if(LastCmd.IsEqiAscii("XREPORT"))
				cmd_str = "Печать X-отчета";
			else if(LastCmd.IsEqiAscii("OPENCHECK"))
				cmd_str = "Открытие чека";
			else if(LastCmd.IsEqiAscii("CLOSECHECK"))
				cmd_str = "Закрытие чека";
			else if(LastCmd.IsEqiAscii("PRINTFISCAL"))
				cmd_str = "Печать фискальной строки";
			else if(LastCmd.IsEqiAscii("PRINTTEXT"))
				cmd_str = "Печать текстовой строки";
			else if(LastCmd.IsEqiAscii("ANNULATE"))
				cmd_str = "Аннулирование чека";
			else if(LastCmd.IsEqiAscii("INCASHMENT"))
				cmd_str = "Внесение/изъятие денег";
			else if(LastCmd.IsEqiAscii("CHECKCORRECTION"))
				cmd_str = "Печать чека коррекции (0x58)";
			else
				cmd_str = LastCmd;
			rMsg.CatDiv(':', 2).Cat(cmd_str);
		}
	}
	else {
		if(rMsg.NotEmpty())
			rMsg.Space();
		rMsg.CatEq("ErrCode", static_cast<long>(LastError));
	}
	rMsg.Transf(CTRANSF_UTF8_TO_OUTER); // @v10.4.6
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

int PiritEquip::ENQ_ACK()
{
	const clock_t clk_init = clock();
	const uint max_clk = 2000;
	const uint max_tries = 3;
	const uint try_dealy = 50;
	uint  try_no = 0;
	SDelay(try_dealy);
	do {
		CommPort.PutChr(ENQ); // Проверка связи с ККМ
		int r = CommPort.GetChr();
		// @debug @v10.1.5 if(r == ACK || r == 0x30 || r == 0x34) // @v10.1.5 (|| r == 0x30 || r == 0x34)
		// @debug if(r > 0) { // @v10.1.5
		if(r == ACK) {
			return 1;
		}
		else {
			++try_no;
			if(try_no >= max_tries) {
				if(LogFileName.NotEmpty()) {
					SString msg_buf;
					(msg_buf = "Error ENQ_ACK() tries exceeded").Space().CatChar('(').Cat(try_no).CatChar(')').Space().CatEq("reply", (long)r);
					SLS.LogMessage(LogFileName, msg_buf, 8192);
				}
				return 0;
			}
			else {
				clock_t clk_current = clock();
				if((clk_current - clk_init) >= max_clk) {
					if(LogFileName.NotEmpty()) {
						SString msg_buf;
						(msg_buf = "Error ENQ_ACK() timeout exceeded").Space().CatChar('(').Cat(clk_current - clk_init).CatChar(')');
						SLS.LogMessage(LogFileName, msg_buf, 8192);
					}
					return 0;
				}
				else
					SDelay(try_dealy);
			}
		}
	} while(1);
}

int PiritEquip::SetConnection()
{
	int    ok = 1;
	int    r = 0;
	int    is_ready = 0;
	CommPortParams port_params;
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
	{
		//int ipr = CommPort.InitPort(Cfg.Port, 1/*ctsControl*/, 2/*rtsControl*/);
		int ipr = CommPort.InitPort(Cfg.Port, 0/*ctsControl*/, 0/*rtsControl*/);
		if(!ipr) {
			if(LogFileName.NotEmpty())
				SLS.LogMessage(LogFileName, "Error CommPort.InitPort", 8192);
			CALLEXCEPT();
		}
	}
	// @v9.5.7 delay(200);
	//SDelay(100); // @v10.0.14
	if((Cfg.ReadCycleCount > 0) || (Cfg.ReadCycleDelay > 0))
		CommPort.SetReadCyclingParams(Cfg.ReadCycleCount, Cfg.ReadCycleDelay);
	THROW(ENQ_ACK());
	if(LogFileName.NotEmpty()) {
		SString msg_buf;
		(msg_buf = "Connection is established").CatDiv(':', 2).CatEq("cbr", (long)port_params.Cbr);
		SLS.LogMessage(LogFileName, msg_buf, 8192);
	}
	CATCH
		CommPort.ClosePort(); // @v10.0.02
		if(LogFileName.NotEmpty()) {
			SString msg_buf;
			(msg_buf = "Error on connection").CatDiv(':', 2).CatEq("cbr", (long)port_params.Cbr);
			SLS.LogMessage(LogFileName, msg_buf, 8192);
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int PiritEquip::CloseConnection()
{
	/*if(H_Port != INVALID_HANDLE_VALUE)
		CloseHandle(H_Port);*/
	CommPort.ClosePort(); // @v10.0.02
	return 1;
}

int PiritEquip::ReadConfigTab(int arg1, int arg2, SString & rOut, SString & rError)
{
	rOut.Z();
	rError.Z();
	int   ok = 1;
	SString in_data;
	CreateStr(arg1, in_data);
	CreateStr(arg2, in_data);
	THROW(ExecCmd("11", in_data, rOut, rError));
	CATCHZOK
	return ok;
}

int PiritEquip::WriteConfigTab(int arg1, int arg2, int val, SString & rOut, SString & rError)
{
	rOut.Z();
	rError.Z();
	int   ok = 1;
	SString in_data;
	CreateStr(arg1, in_data);
	CreateStr(arg2, in_data);
	CreateStr(val, in_data);
	THROW(ExecCmd("12", in_data, rOut, rError));
	CATCHZOK
	return ok;
}

int PiritEquip::GetTaxTab()
{
	int    ok = 1;
	SString in_data;
	SString out_data;
	SString r_error;
	//SString raw_tax_val;
	SString temp_buf;
	SString log_buf;
	for(int i = 0; i < 6; i++) { // В пирите не более 6 налоговых ставок // @v10.1.2 (i < 5)-->(i < 6)
		MEMSZERO(DvcTaxArray[i]);
		//raw_tax_val.Z();
		//in_data.Z();
		/*
		CreateStr(40, in_data); // Наименование i-й налоговой ставки
		CreateStr(i, in_data);
		THROW(ExecCmd("11", in_data, out_data, r_error));
		*/
		THROW(ReadConfigTab(40, i, out_data, r_error)); // Наименование i-й налоговой ставки
		//if(out_data.NotEmpty()) {
		RetTknzr.setBuf(out_data);
		if(RetTknzr.get(0U, temp_buf)) {
			STRNSCPY(DvcTaxArray[i].Name, /*out_data*/temp_buf);
			/*
			in_data.Z();
			out_data.Z();
			CreateStr(41, in_data); // Значение i-й налоговой ставки
			CreateStr(i, in_data);
			THROW(ExecCmd("11", in_data, out_data, r_error));
			*/
			THROW(ReadConfigTab(41, i, out_data, r_error)); // Значение i-й налоговой ставки
			RetTknzr.setBuf(out_data);
			if(RetTknzr.get(0U, temp_buf)) {
				//raw_tax_val = out_data;
				DvcTaxArray[i].Rate = temp_buf.ToReal();
			}
		}
		else
			temp_buf.Z();
		if(i == 0)
			log_buf.Cat("DvcTaxArray").CatDiv(':', 2);
		else
			log_buf.CatDiv(';', 2);
		log_buf.Cat(DvcTaxArray[i].Name).CatDiv(',', 2).Cat(DvcTaxArray[i].Rate, MKSFMTD(0, 6, NMBF_NOTRAILZ)).CatChar('|').Cat(temp_buf);
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
	//SString out;
	//SString subdata;
	SString in_data;
	SString out_data;
	SString r_error;
	// Логический номер кассы
	/*
	CreateStr(10, in_data);
	CreateStr(0, in_data);
	CreateStr((int)Cfg.LogNum, in_data);
	THROW(ExecCmd("12", in_data, out_data, r_error));
	*/
	THROW(WriteConfigTab(10, 0, (int)Cfg.LogNum, out_data, r_error));
	{
#if 0 // @v10.0.0 {
		int    flag = 0;
		// Нумерация чеков ККМ
		/*
		in_data.Z();
		CreateStr(2, in_data);
		CreateStr(0, in_data);
		THROW(ExecCmd("11", in_data, out_data, r_error)); // Получаем параметры чека
		*/
		THROW(ReadConfigTab(2, 0, out_data, r_error)); 
		flag = out_data.ToLong();
		flag &= ~128; // Устанавливаем бит нумерации чеков ККМ
		//
		/*
		in_data.Z();
		CreateStr(2, in_data);
		CreateStr(0, in_data);
		CreateStr(flag, in_data);
		THROW(ExecCmd("12", in_data, out_data, r_error));
		*/
		THROW(WriteConfigTab(2, 0, flag, out_data, r_error));
#endif // } 0 @v10.0.0
	}
	// Получаем номер текущей сессии
	CreateStr(1, in_data);
	{
		THROW(ExecCmd("01", in_data, out_data, r_error)); // Запрос номера текущей сессии(смены)
		{
			//StringSet delim_out(FS, out_data);
			uint   i = 0;
			RetTknzr.setBuf(out_data);
			RetTknzr.get(&i, out_data); // В out считан номер запроса
			RetTknzr.get(&i, out_data); // В out считан номер текущей сессии(смены)
			SessID = out_data.ToLong();
		}
	}
	// Установка/снятие флага печати логотипа
	// @v10.2.5 THROW(PrintLogo((Cfg.Logo.Print == 1) ? 1 : 0))
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
		THROW(ExecCmd("13", in_data, out_data, r_error)); // Смотрим текщую дату/время на ККМ
		out_data.Divide(FS, date, time);
		CashDateTime.Z().Cat("Текущая дата на ККМ").CatDiv(':', 2).Cat(date).Space().Cat("Текущее время на ККМ").CatDiv(':', 2).Cat(time);
		in_data.Z();
		GetLocalTime(&sys_dt_tm);
		GetDateTime(sys_dt_tm, datetime, 0);
		CreateStr(datetime, in_data);
		GetDateTime(sys_dt_tm, datetime, 1);
		CreateStr(datetime, in_data);
		THROW(ExecCmd("10", in_data, out_data, r_error));
		THROW(GetCurFlags(2, flag));
		if(!(flag & 0x4) && (r_error.CmpNC("0B") == 0)) {  // Проверяем что смена закрыта и код ошибки "дата и время отличаются от текущих даты и времени ККМ более чем на 8 минут"
			in_data.Z();
			GetLocalTime(&sys_dt_tm);
			GetDateTime(sys_dt_tm, datetime, 0);
			CreateStr(datetime, in_data);
			GetDateTime(sys_dt_tm, datetime, 1);
			CreateStr(datetime, in_data);
			THROW(ExecCmd("14", in_data, out_data, r_error)); // Устанавливаем системные дату и время в ККМ
		}
	}
	CATCHZOK
	return ok;
}

int PiritEquip::GetCurFlags(int numFlags, int & rFlags)
{
	const  uint max_tries = 3; // @v10.1.00 10-->3
	int    ok = 1;
	SString out_data, r_error;
	uint count = 0;
	rFlags = 0;
	int    flags_fatal_state = 0;
	int    flags_current_state = 0;
	int    flags_doc_status = 0;
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
			uint    sp = 0;
			if(fl_pack.get(&sp, out_data)) {
				flags_fatal_state = out_data.ToLong();
				if(fl_pack.get(&sp, out_data)) {
					flags_current_state = out_data.ToLong();
					if(fl_pack.get(&sp, out_data)) {
						flags_doc_status = out_data.ToLong();
					}
				}
			}
			if(LogFileName.NotEmpty()) {
				(out_data = "Current state").CatDiv(':', 2).CatHex((long)flags_fatal_state).CatDiv(',', 2).
					CatHex((long)flags_current_state).CatDiv(',', 2).CatHex((long)flags_doc_status);
				SLS.LogMessage(LogFileName, out_data, 8192);
			}
			switch(numFlags) {
				case 1: rFlags = flags_fatal_state; break;
				case 2: rFlags = flags_current_state; break;
				case 3: rFlags = flags_doc_status; break;
			}
			/*for(sp = 0; fl_pack.get(&sp, out_data);) {
				if(++fc == numFlags)
					rFlags = out_data.ToLong();
			}
			ok = fc;*/
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
			in_data.Z();
			CreateStr(Check.CheckType, in_data);
			CreateStr(Check.Department, in_data);
			CreateStr(CshrName, in_data);
			// @v9.9.12 CreateStr("", in_data);
			CreateStr(Check.CheckNum, in_data); // @v9.9.12 
			if(Check.TaxSys >= 0 && Check.TaxSys <= 5) // @v10.6.4
				CreateStr(inrangeordefault(static_cast<long>(Check.TaxSys), 0L, 5L, 0L), in_data); // @v10.6.3
			THROW(ExecCmd("30", in_data, out_data, r_error));
			break;
		case 1: // Закрыть документ
			// Проверяем наличие открытого документа
			THROW(gcf_result = GetCurFlags(3, flag));
			if((flag > 4) || (gcf_result < 3)) { // @v10.1.9 @fix (flag >> 4)-->(flag > 4)
				const uint8 hb1 = (flag & 0x0F);
				const uint8 hb2 = (flag & 0xF0);
				if((gcf_result < 3) || (oneof2(hb1, 2, 3) && hb2 != 0x40)) { // Если открыт чек и не была произведена оплата, то операция оплаты
					in_data.Z();
					if(Check.PaymBank != 0.0) {
						CreateStr(1, in_data); // Тип оплаты
						FormatPaym(Check.PaymBank, str);
						CreateStr(str, in_data);
						CreateStr("", in_data);
						THROW(ExecCmd("47", in_data, out_data, r_error));
					}
					in_data.Z();
					if(Check.PaymCash != 0.0) {
						CreateStr(0, in_data); // Тип оплаты
						FormatPaym(Check.PaymCash, str);
						CreateStr(str, in_data);
						CreateStr("", in_data);
						THROW(ExecCmd("47", in_data, out_data, r_error));
					}
					// @v10.4.6 {
					in_data.Z();
					if(Check.PaymCCrdCard != 0) {
						CreateStr(13, in_data); // Тип оплаты // @v10.4.9 14-->13
						FormatPaym(Check.PaymCCrdCard, str);
						CreateStr(str, in_data);
						CreateStr("", in_data);
						THROW(ExecCmd("47", in_data, out_data, r_error));
					}
					// } @v10.4.6 
				}
				in_data.Z();
				if(Cfg.Flags & 0x08000000L)
					CreateStr(1, in_data); // Чек не отрезаем (только для сервисных документов)
				else
					CreateStr(0, in_data); // Чек отрезаем
				// @v10.2.10 CreateStr("", in_data); // @v10.1.9 Адрес покупателя
				// @v10.2.10 CreateStr(1, in_data); // @v10.1.9 (число) Система налогообложения
				// @v10.2.10 CreateStr((int)0, in_data); // @v10.1.9 (число) Разные флаги
				THROW(ExecCmd("31", in_data, out_data, r_error));
				// gcf_result = GetCurFlags(3, flag); // @v10.2.10 @debug 
			}
			// new {
			else {
				THROWERR(0, PIRIT_ERRSTATUSFORFUNC);
			}
			// } new
			break;
		case 2: // Печать фискальной строки
			/*
				(Строка[0...224]) Название товара
				(Строка[0..18]) Артикул или штриховой код товара/номер ТРК
				(Дробное число) Количество товара в товарной позиции
				(Дробное число) Цена товара по данному артикулу
				(Целое число) Номер ставки налога
				(Строка[0..4]) Номер товарной позиции
				(Целое число 1..16) Номер секции
				(Целое число) Пустой параметр
				(Строка[0...38]) Пустой параметр
				(Дробное число) Сумма скидки
				(Целое число) Признак способа расчета
					1 	Предоплата 100%
					2 	Предоплата
					3 	Аванс
					4 	Полный расчет
					5 	Частичный расчет и кредит
					6 	Передача в кредит
					7 	Оплата кредита
				(Целое число) Признак предмета расчета
					1 	о реализуемом товаре, за исключением подакцизного товара (наименование и иные сведения, описывающие товар)
					2 	о реализуемом подакцизном товаре (наименование и иные сведения, описывающие товар)
					3 	о выполняемой работе (наименование и иные сведения, описывающие работу)
					4 	об оказываемой услуге (наименование и иные сведения, описывающие услугу)
					5 	о приеме ставок при осуществлении деятельности по проведению азартных игр
					6 	о выплате денежных средств в виде выигрыша при осуществлении деятельности по проведению азартных игр
					7 	о приеме денежных средств при реализации лотерейных билетов, электронных лотерейных билетов, приеме лотерейных ставок при осуществлении деятельности по проведению лотерей
					8 	о выплате денежных средств в виде выигрыша при осуществлении деятельности по проведению лотерей
					9 	о предоставлении прав на использование результатов интеллектуальной деятельности или средств индивидуализации
					10 	об авансе, задатке, предоплате, кредите, взносе в счет оплаты, пени, штрафе, вознаграждении, бонусе и ином аналогичном предмете расчета
					11 	о вознаграждении пользователя, являющегося платежным агентом (субагентом), банковским платежным агентом (субагентом), комиссионером, поверенным или иным агентом
					12 	о предмете расчета, состоящем из предметов, каждому из которых может быть присвоено значение от «0» до «11»
					13 	о предмете расчета, не относящемуся к предметам расчета, которым может быть присвоено значение от «0» до «12»

				(Строка[3]) Код страны происхождения товара
				(Строка[0...24]) Номер таможенной декларации
				(Дробное число) Сумма акциза
			*/
			/*
				Установить дополнительные реквизиты позиции (0x24)

				Команда вызывается перед командой добавления товарной позиции и устанавливает дополнительные реквизиты. 
				Действие команды распространяется на одну товарную позицию в открытом документе.

				Каждый номер телефона должен начинаться с символа "+" и не должен превышать ограничение длины в 19 символов (включая "+").
				Входные параметры

					(Строка[1..32]) Код товарной номенклатуры
					(Строка[1..64]) Дополнительный реквизит предмета расчёта
					(Строка[1..16]) Единица измерения предмета расчёта
					(Целое число) Признак агента по предмету расчёта
					(Строка)[0..12] ИНН поставщика
					(Строка)[0..19] Телефон поставщика
					(Строка)[0..256] Наименование поставщика
					(Строка)[0..256] Адрес оператора перевода (для банк.пл.агента/банк.пл.субагента, иначе пустой)
					(Строка)[0..12] ИНН оператора перевода (для банк.пл.агента/банк.пл.субагента, иначе пустой)
					(Строка)[0..64] Наименование оператора перевода (для банк.пл.агента/банк.пл.субагента, иначе пустой)
					(Строка)[0..19] Телефон оператора перевода (для банк.пл.агента/банк.пл.субагента, иначе пустой)
					(Строка)[0..24] Операция платежного агента (для банк.пл.агента/банк.пл.субагента, иначе пустой)
					(Строка)[0..19] Телефон платежного агента (для пл.агента/пл.субагента, иначе пустой)
					(Строка)[0..19] Телефон оператора по приему платежей (для пл.агента/пл.субагента, иначе пустой)

				Ответные параметры: Нет
				Дополнительная информация

				Код товарной номенклатуры

				В поле «Код товарной номенклатуры» могут быть переданы шестнадцатеричные данные, каждый такой байт имеет вид: 
				$xy, где x и y – шестнадцатиричные цифры, например $C5 соответствует значению 0xC5. Количество предмет расчёта, 
				для которого задан код товарной номенклатуры, должно равняться единице. Поле передается в ФН без изменений. Структура его определяется ФФД.

				Начиная с версии 665.2.29, допустимы лишь кода, первые 2 байта которых соответствуют таблице:
					Байт 1 	Байт 2
					0x00 	0x05
					0x15 	0x20
					0x45 	0x08
					0x45 	0x0D
					0x49 	0x0E
					0x44 	0x4D
					0x52 	0x46
					0x52 	0x47
					0xC5 	0x14
					0xC5 	0x1E

				Признак агента по предмету расчета - битовая маска
					Номер бита 	Пояснение
					0 	Банковский платежный агент
					1 	Банковский платежный субагент
					2 	Платежный агент
					3 	Платежный субагент
					4 	Поверенный
					5 	Комиссионер
					6 	Агент
			*/
			THROW(GetCurFlags(3, flag)); // @v10.7.9 (moved up) 
			// @v10.6.12 @construction {
			if(Check.ChZnProdType && Check.ChZnGTIN.NotEmpty() && (Check.ChZnSerial.NotEmpty() || Check.ChZnPartN.NotEmpty())/*Check.ChZnCode.NotEmpty()*/) {
				in_data.Z();
				uint16 product_type_bytes = 0;
				uint8  chzn_1162_bytes[128];
				if(Check.ChZnProdType == 1) // GTCHZNPT_FUR
					product_type_bytes = 0x0002;
				else if(Check.ChZnProdType == 2) // GTCHZNPT_TOBACCO
					product_type_bytes = 0x0005;
				else if(Check.ChZnProdType == 3) // GTCHZNPT_SHOE
					product_type_bytes = 0x1520;
				else if(Check.ChZnProdType == 4) // GTCHZNPT_MEDICINE
					product_type_bytes = 0x0003;
				//const char * p_serial = Check.ChZnPartN.NotEmpty() ? Check.ChZnPartN.cptr() : Check.ChZnSerial.cptr(); // @v10.7.8
				const char * p_serial = Check.ChZnSerial.NotEmpty() ? Check.ChZnSerial.cptr() : Check.ChZnPartN.cptr(); // @v10.7.8
				int    rl = STokenRecognizer::EncodeChZn1162(product_type_bytes, Check.ChZnGTIN, p_serial, chzn_1162_bytes, sizeof(chzn_1162_bytes));
				if(rl > 0) {
					str.Z();
					for(int si = 0; si < rl; si++) {
						if(si < 8)
							str.CatChar('$').CatHex(chzn_1162_bytes[si]);
						else
							str.CatChar(chzn_1162_bytes[si]);
						//str.CatHex(chzn_1162_bytes[si]);
					}
					//str.Trim(32);
					//(str = Check.ChZnCode).Trim(32); // [1..32]
					CreateStr(str, in_data); // Код товарной номенклатуры
					/*if(Check.ChZnProdType == 4) // GTCHZNPT_MEDICINE
						CreateStr("mdlp", in_data); */
					CreateStr("[M]", in_data); 
					{
						const int do_check_ret = 1;
						OpLogBlock __oplb(LogFileName, "24", str);
						THROWERR(PutData("24", in_data), PIRIT_NOTSENT);
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
			// } @v10.6.12 
			// @v10.7.9 (moved up) THROW(GetCurFlags(3, flag));
			in_data.Z();
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
			CreateStr((int)0, in_data);    // @v10.4.1 пустой параметр (integer)
			CreateStr("", in_data);        // @v10.4.1 пустой параметр (string)
			CreateStr(0.0, in_data);       // @v10.4.1 скидка (real)
			CreateStr(Check.Ptt, in_data); // @v10.4.1 признак способа расчета (integer)
			CreateStr(Check.Stt, in_data); // @erikO v10.4.12 Признак предмета расчета(integer)
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
					/*
						№ бита	Значения атрибутов текста
						0..3 (N:шрифта)
							0 – Шрифт 13х24, 44 символа в строке
							1 – Шрифт 10х20
							2 – Шрифт 13х24 жирный
							3 – Шрифт 10х20 жирный
							4 – Шрифт 8х14, 56 символов в строке
							5 – Шрифт 24х45
							6 – Шрифт 24х45 жирный
						4	Печать двойной высоты текста
						5	Печать двойной ширины текста
						6	Не используется 
						7	Не используется
					*/
					if(oneof2(Check.FontSize, 1, 2))
						text_attr = 0x01; // Шрифт 13х24, 44 символа в строке
					else if(Check.FontSize == 3)
						text_attr = 0;
					else if(Check.FontSize == 4)
						text_attr = 0x10; // Шрифт 8х14, 56 символов в строке
					else if(Check.FontSize > 4)
						text_attr = (0x20|0x10);
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
				THROW(ExecCmd("32", 0, out_data, r_error));
			}
			break;
		case 5: // Внесение/изъятие наличности
			{
				SString str;
				in_data.Z();
				CreateStr("", in_data);
				FormatPaym(Check.IncassAmt, str);
				CreateStr(str, in_data);
				THROW(ExecCmd("48", in_data, out_data, r_error));
			}
			break;
	}
	CATCHZOK
	return ok;
}

int PiritEquip::ReturnCheckParam(const SString & rInput, char * pOutput, size_t size)
{
	int    ok = 0;
	int    r = 0;
	SString buf;
	SString in_data;
	SString out_data;
	SString r_error;
	SString str;
	SString s_output;
	StringSet params(';', rInput);
	for(uint i = 0; params.get(&i, buf) > 0;) {
		in_data.Z();
		out_data.Z();
		if(buf.IsEqiAscii("AMOUNT")) {
			CreateStr(1, in_data);
			THROW(ExecCmd("03", in_data, out_data, r_error));
			{
				StringSet dataset(FS, out_data);
				// @v10.1.6 {
				if(LogFileName.NotEmpty()) {
					(str = "AMOUNT (Cmd=03 Arg=1)").CatDiv(':', 2).Cat(out_data);
					SLS.LogMessage(LogFileName, str, 8192);
				}
				// } @v10.1.6 
				uint k = 0;
				dataset.get(&k, str); // Номер запроса
				dataset.get(&k, str);
				s_output.CatEq("AMOUNT", str).Semicol();
			}
		}
		else if(buf.IsEqiAscii("CHECKNUM")) {
			CreateStr(2, in_data);
			THROW(ExecCmd("03", in_data, out_data, r_error));
			{
				StringSet dataset(FS, out_data);
				// @v10.1.5 {
				if(LogFileName.NotEmpty()) {
					(str = "CHECKNUM (Cmd=03 Arg=2)").CatDiv(':', 2).Cat(out_data);
					SLS.LogMessage(LogFileName, str, 8192);
				}
				// } @v10.1.5 
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
				if(cc_number == 18 || cc_number < 0) // @v10.1.9 Костыль: Некоторые аппараты всегад возвращают 18-й номер чека. Трактуем это как 0.
					cc_number = 0;
				s_output.CatEq("CHECKNUM", (long)cc_number).Semicol();
			}
		}
		else if(buf.IsEqiAscii("CASHAMOUNT")) {
			int    succs = 0;
			for(uint t = 0; !succs && t < 4; t++) {
				CreateStr(7, in_data);
				THROW(ExecCmd("02", in_data, out_data, r_error));
				{
					StringSet dataset(FS, out_data);
					const uint dsc = dataset.getCount();
					if(dsc == 3) {
						uint k = 0;
						dataset.get(&k, str); // Номер запроса
						dataset.get(&k, str);
						s_output.CatEq("CASHAMOUNT", str).Semicol();
						succs = 1;
					}
				}
			}
			if(!succs) 
				s_output.CatEq("CASHAMOUNT", "0.0").Semicol();
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
	THROW(ExecCmd("81", 0, out_data, r_error));
	if(out_data.ToLong() == 0) {
		THROW(ExecCmd("80", 0, out_data, r_error));
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
	if(GetData(rOutData, rError) < 0) {
		rError = "00";
		ok = -1;
	}
	else {
		if(rError.NotEmpty()) {
			int  result_err_code = 0;
			int  src_err_code = -1;
			if(rError.Len() == 2 && ishex(rError.C(0)) && ishex(rError.C(1)))
				src_err_code = _texttohex32(rError.cptr(), 2);
			if(!oneof3(src_err_code, 0, 0x0B, 0x09)) {
				switch(src_err_code) {
					case 0x0C: result_err_code = PIRIT_DATELSLASTOP; break; // Системная дата меньше даты последней фискальной операции, зарегистрированной в ККМ
					case 0x20: result_err_code = PIRIT_FATALERROR; break; // Фатальная ошибка ККМ
					case 0x21: result_err_code = PIRIT_FMOVERFLOW; break; // Нет свободного места в фискальной памяти ККМ
					case 0x41: result_err_code = PIRIT_ECRRFORMAT; break; // Некорректный формат или параметр команды ЭКЛЗ
					case 0x42: result_err_code = PIRIT_ECRERRORSTATUS; break; // Некорректное состояние ЭКЛЗ
					case 0x43: result_err_code = PIRIT_ECRACCIDENT; break; // Авария ЭКЛЗ
					case 0x44: result_err_code = PIRIT_KCACCIDENT; break; // Авария КС (криптографического сопроцессора)в составе ЭКЛЗ
					case 0x45: result_err_code = PIRIT_ECRTIMEOUT; break; // Исчерпан временной ресурс использования ЭКЛЗ
					case 0x46: result_err_code = PIRIT_ECROVERFLOW; break; // ЭКЛЗ переполнена
					case 0x47: result_err_code = PIRIT_ECRERRORDATETIME; break; // Неверные дата или время
					case 0x48: result_err_code = PIRIT_ECRNODATA; break; // Нет запрошенных данных
					case 0x49: result_err_code = PIRIT_ECRTOOMUCH; break; // Переполнение (отрицательный итог документа, слишком много отделов для клиента)
					case 0x4A: result_err_code = PIRIT_NOANSWER; break; // Нет ответа от ЭКЛЗ
					case 0x4B: result_err_code = PIRIT_ECRERREXCHANGE; break; // Ошибка при обмене данными с ЭКЛЗ
					default:
						if(SIntToSymbTab_HasId(Pirit_ErrMsg, SIZEOFARRAY(Pirit_ErrMsg), src_err_code))
							result_err_code = src_err_code;
						break;
				}
				if(result_err_code) {
					THROWERR(0, result_err_code);
				}
				else {
					THROWERR(0, src_err_code);
				}
			}
		}
		count++;
	}
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
	// 01189011480055911722090010B90206621B2K662Y1M301
	// \x2PIRI124$00$03$11$30$c4$36$74$d7B2K662Y1M301\x1c\x3360\x10
	// \x2PIRI!00\x320
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
	const int fill_debug_buffer = 1;
	char   debug_packet[1024];
	size_t debug_packet_pos = 0;
	if(fill_debug_buffer) {
		// блок для отладки
		memzero(debug_packet, sizeof(debug_packet));
		THROW(CommPort.PutChr(STX));
		debug_packet[debug_packet_pos++] = STX;
		for(p = 0; p < r_pack.Len(); p++) {
			const char v = r_pack.C(p);
			THROW(CommPort.PutChr(v));
			debug_packet[debug_packet_pos++] = v;
		}
		THROW(CommPort.PutChr(ETX));
		debug_packet[debug_packet_pos++] = ETX;
		THROW(CommPort.PutChr(buf[0]));
		debug_packet[debug_packet_pos++] = buf[0];
		THROW(CommPort.PutChr(buf[1]));
		debug_packet[debug_packet_pos++] = buf[1];
	}
	else {
		THROW(CommPort.PutChr(STX));
		for(p = 0; p < r_pack.Len(); p++)
			THROW(CommPort.PutChr(r_pack.C(p)));
		THROW(CommPort.PutChr(ETX));
		THROW(CommPort.PutChr(buf[0]));
		THROW(CommPort.PutChr(buf[1]));
	}
	CATCHZOK
	return ok;
}

int PiritEquip::GetStatus(SString & rStatus)
{
	int    ok = 1;
	long   status = 0;
	int    flag = 0;
	SString in_data, out_data, r_error;
	in_data.Z();
	THROW(ExecCmd("04", in_data, out_data, r_error));
	flag = out_data.ToLong();
	if(flag & 2) { // В принтере нет бумаги
		status |= NOPAPER;
	}
	else {
		if(LastStatus & NOPAPER) {
			status |= PRINTAFTERNOPAPER;
		}
		if(r_error.CmpNC("09") == 0) { // Проверка ошибки здесь, чтобы не потерять статус
			if(flag & 0x1) {
				THROWERR(0, PIRIT_PRNNOTREADY)
			}
			else if(flag & 0x4) {
				THROWERR(0, PIRIT_PRNTROPENED)
			}
			else if(flag & 0x8) {
				THROWERR(0, PIRIT_PRNCUTERR)
			}
			else if(flag & 0x80) {
				THROWERR(0, PIRIT_NOCONCTNWITHPRNTR);
			}
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
	SString out_data;
	SString r_error;
	//SString in_data;
	/*
	CreateStr(1, in_data);
	CreateStr(0, in_data);
	{
		OpLogBlock __oplb(LogFileName, "11", 0);
		THROWERR(PutData("11", in_data), PIRIT_NOTSENT);
		out_data.Destroy();
		THROW(GetWhile(out_data, r_error));
	}
	*/
	THROW(ReadConfigTab(1, 0, out_data, r_error)); 
	flag = out_data.ToLong();
	SETFLAG(flag, 0x04, print);
	/*
	in_data.Destroy();
	CreateStr(1, in_data);
	CreateStr(0, in_data);
	CreateStr(flag, in_data);
	THROW(ExecCmd("12", in_data, out_data, r_error));
	*/
	THROW(WriteConfigTab(1, 0, flag, out_data, r_error));
	THROW(StartWork());
	CATCHZOK
	return ok;
}

void PiritEquip::GetDateTime(SYSTEMTIME sysDtTm, SString & rDateTime, int dt)
{
	switch(dt) {
		case 0: rDateTime.Z().CatLongZ(sysDtTm.wDay, 2).CatLongZ(sysDtTm.wMonth, 2).Cat(sysDtTm.wYear % 100); break;
		case 1: rDateTime.Z().CatLongZ(sysDtTm.wHour, 2).CatLongZ(sysDtTm.wMinute, 2).CatLongZ(sysDtTm.wSecond, 2); break;
	}
}
