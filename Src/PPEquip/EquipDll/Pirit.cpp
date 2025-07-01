// Dll для работы с ККМ Пирит
// @codepage UTF-8
//
#pragma hdrstop
#include <ppdrvapi.h>
#include <pp-ifm.h>

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

#define EXPORT	extern "C" __declspec(dllexport)
#define THROWERR(expr,val)     {if(!(expr)){SetError(val);goto __scatch;}}

static int ErrorCode = 0;
//static constexpr char FS = 0x1C;
//#define	FS_STR	"\x1C"	// Символ-разделитель пар параметров

// @v11.2.3 Следующие мнемоники заменены на CHR_XXX определенные в SLIB.H
// Для формирования пакета данных
// @v11.2.3 #define	STX		0x02	// Начало пакета
// @v11.2.3 #define	ETX		0x03	// Конец пакета
// Коды команд ККМ
// @v11.2.3 #define ENQ	    0x05	// Проверка связи
// @v11.2.3 #define ACK	    0x06	// ККМ на связи
// @v11.2.3 #define CAN	    0x18	// Прервать выполнение отчета

// Коды ошибок
#define PIRIT_ERRSTATUSFORFUNC     1 // 01h Функция невыполнима при данном статусе ККМ
#define PIRIT_ERRFUNCNUMINCOMMAND  2 // 02h В команде указан неверный номер функции
#define PIRIT_ERRCMDPARAMORFORMAT  3 // 03h Некорректный формат или параметр команды
#define PIRIT_ENDOFPAPER           8 // 08h Конец бумаги
#define PIRIT_PRNNOTREADY          9 // 09h Принтер не готов
#define PIRIT_SESSOVER24H         10 // 0Ah Текущая смена больше 24 часов. Установка даты времени больше чем на 24 часа.
#define PIRIT_DIFDATE             11 // Дата и время на ККМ отличаются от системных на 8 минут
#define PIRIT_DATELSLASTOP        12 // Системная дата меньше даты последней фискальной операции, зарегистрированной в ККМ
#define PIRIT_NODATAINJOURNAL     16 // @v11.8.8 Нет данных в журнале
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
	Config() : CashID(0), Name(0), LogNum(0), Port(0), BaudRate(0), DateTm(MAXDATETIME), Flags(0), LocalFlags(lfDoLog), ConnPass("PIRI"), ReadCycleCount(10), ReadCycleDelay(10)
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
	enum {
		lfDoLog = 0x0001 // @v11.5.0 Вести файл журнала операций
	};
	int    CashID;
	char * Name;
	uint   LogNum;
	int    Port;
	int    BaudRate;
	LDATETIME DateTm;
	long   Flags;
	long   LocalFlags;
	const  char * ConnPass; // Пароль для связи
	int    ReadCycleCount;
	int    ReadCycleDelay;
	LogoStruct Logo;
};

struct CheckStruct {
	CheckStruct() : CheckType(2), FontSize(3), CheckNum(0), Qtty(0.0), PhQtty(0.0), Price(0.0), Department(0), Ptt(0), Stt(0), UomId(0), UomFragm(0), 
		TaxSys(0), Tax(0), PaymCash(0.0), PaymBank(0.0), IncassAmt(0.0), ChZnProdType(0), ChZnPpResult(0), ChZnPpStatus(0), //@erik v10.4.12 add "Stt(0),"
		Timestamp(ZERODATETIME) /*@v11.2.3*/, PrescrDate(ZERODATE)/*@v11.8.0*/, ChZnPm_ReqTimestamp(0ULL)/*@v12.1.1*/
	{
	}
	CheckStruct & Z()
	{
		FontSize = 3;
		Qtty = 0.0;
		PhQtty = 0.0; // @v11.9.3
		Price = 0.0;
		Department = 0;
		Tax = 0;
		Ptt = 0;
		Stt = 0; // @erik v10.4.12
		UomId = 0; // @v11.9.5
		UomFragm = 0; // @v11.2.5
		TaxSys = -1;
		Text.Z();
		Code.Z();
		ChZnCode.Z();
		ChZnGTIN.Z();
		ChZnSerial.Z();
		ChZnPartN.Z();
		ChZnSid.Z();
		DraftBeerSimplifiedCode.Z(); // @v11.9.4
		PaymCash = 0.0;
		PaymBank = 0.0;
		IncassAmt = 0.0;
		ChZnProdType = 0;
		ChZnPpResult = 0; // @v11.1.11
		ChZnPpStatus = 0; // @v11.1.11
		ChZnPm_ReqId.Z();  // @v12.1.1
		ChZnPm_ReqTimestamp = 0; // @v12.1.1
		Timestamp.Z(); // @v11.2.3
		BuyersEmail.Z(); // @v11.3.6
		BuyersPhone.Z(); // @v11.3.6
		PrescrDate.Z();    // @v11.8.0 Рецепт: Дата  //
		PrescrSerial.Z(); // @v11.8.0 Рецепт: Серия //
		PrescrNumber.Z(); // @v11.8.0 Рецепт: Номер //
		return *this;
	}
	int    CheckType;
	int    FontSize;
	int    CheckNum;
	double Qtty;   // @v11.9.3 Quantity-->Qtty
	double PhQtty;
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
	int    TaxSys;       // Система налогообложения
	int    Tax;          //
	int    Ptt;          // CCheckPacket::PaymentTermTag
	int    Stt;          // @erik v10.4.12
	int    UomId;        // @v11.9.5 Ид единицы измерения (SUOM_XXX)
	int    UomFragm;     // @v11.2.5 Фрагментация единицы измерения //
	int    ChZnProdType; //
	double PaymCash;
	double PaymBank;
	double PaymCCrdCard;
	double IncassAmt;
	int    ChZnPpResult; // @v11.1.11 Результат проверки марки честный знак на фазе препроцессинга
	int    ChZnPpStatus; // @v11.1.11 Статус, присвоенный марке честный знак на фазе препроцессинга
	S_GUID ChZnPm_ReqId;  // @v12.1.1 ответ разрешительного режима чзн: уникальный идентификатор запроса
	int64  ChZnPm_ReqTimestamp; // @v12.1.1 ответ разрешительного режима чзн: дата и время формирования запроса
	LDATETIME Timestamp; // @v11.2.3 Дата и время чека
	SString Text;
	SString Code;        //
	SString ChZnCode;    //
	SString ChZnGTIN;    //
	SString ChZnSerial;  //
	SString ChZnPartN;   //
	SString ChZnSid;     // Ид предприятия для передачи в честный знак
	SString DraftBeerSimplifiedCode; // @v11.9.4 Код для упрощенного списания chzn-марки разливного пива для horeca
	SString BuyersEmail; // @v11.3.6
	SString BuyersPhone; // @v11.3.6
	LDATE  PrescrDate;    // @v11.8.0 Рецепт: Дата  //
	SString PrescrSerial; // @v11.8.0 Рецепт: Серия //
	SString PrescrNumber; // @v11.8.0 Рецепт: Номер //
};

class PiritEquip {
public:
	PiritEquip();
	~PiritEquip();
	int    RunOneCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize);
	int    SetConnection();
	void   CloseConnection();
	int    SetCfg();
	SString & LastErrorText(SString & rMsg);
	int    NotEnoughBuf(SString & rStr);
	//
	// Перед началом работы с ККМ надо задать время и дату. Если время и дата на ККМ
	// отличаются от переданных, то возвращается сообщение об ошибке.
	// ARG(force IN): если true, то  функция начала работы будет вызвана независимо от состояния флагов.
	//  Это сделано в @v11.3.2 с целью попытаться сбить фатальное состояние, которое изредка возникает после вызова команды 79/1 (запрос данных о марке chzn)
	//
	int    StartWork(bool force = false);
	// Получаем текущие флаги ККМ
	int    GetCurFlags(int numFlags, int & rFlags);
	//
	// 0 - Открыть документ
	// 1 - Закрыть документ
	// 2 - Печать фискальной строки
	// 3 - Печать текстовой строки
	// 4 - Аннулировать чек
	// 5 - Внесение/изъятие наличности
	// 6 - Открыть чек коррекции // @v12.3.4
	//	
	int    RunCheck(int opertype);
	int    ReturnCheckParam(const SString & rInput, char * output, size_t size);
	int    PutData(const char * pCommand, const char * pData);
	int    GetData(SString & rData, SString & rError);
	//
	// Descr: Печать реквизита для ОФД (0x57)
	// ARG(reqCode IN): Код реквизита
	//   1192 Дополнительный реквизит чека (БСО) (до 16 символов)
	//   1262 Идентификатор ФОИВ (3 цифры)
	//   1263 Дата документа основания (ддммгггг)
	//   1264 Номер документа основания (до 32 символов)
	//   1265 Значение отраслевого реквизита (до 256 символов)
	//   1271 Идентификатор операции (3 цифры)
	//   1272 Данные операции (до 64 символов)
	//   1273 Дата операции (ддммгг) или дата/время (ддммггччмм)
	// ARG(textAttr IN): Атрибуты текста
	//   опциональный параметр, представляющий собой битовую маску.
    //   0..3 бит - Номер шрифта
	//     0 - Шрифт 12х24
	//     1 - Шрифт 9х17
	//     2 - Шрифт 8х14 (реализован в VikiPrint80+ версии 665.3.0)
    //   4 бит - Печать двойной высоты текста
    //   5 бит - Печать двойной ширины текста
    //   6 бит - Если равен 1, то значение реквизита должно представлять собой число
	// ARG(pReqDescr IN): Описание реквизита (должно быть непустым для реквизита 1192)
	// ARG(pReqValue IN): Значение реквизита
	// ARG(rError OUT):
	//
	int    PutOfdReq(int reqCode, int textAttr, const char * pReqDescr, const char * pReqValue, SString & rError); // @v12.3.3
	//
	// Для получения ответа при выполнении длинных операций (аннулирование, открытие, закрытие, внесение/изъятие наличности, открыть ящик)
	//
	int    GetWhile(SString & rOutData, SString & rError);
	void   GetGlobalErrCode(); // Сделано для ошибок ЭКЛЗ
	int    OpenBox();
	int    GetStatus(SString & rStatus); // Возвращает статус ККМ (состояние принтера, статус документа)

	struct PreprocessChZnCodeResult {
		PreprocessChZnCodeResult() : CheckResult(0), Reason(0), ProcessingResult(0), ProcessingCode(0), Status(0)
		{
		}
		int    CheckResult;      // tag 2106 Результат проверки КМ в ФН (ofdtag-2106)
		int    Reason;           // Причина того, что КМ не проверен в ФН
		int    ProcessingResult; // tag 2005 Результаты обработки запроса (ofdtag-2005)
		int    ProcessingCode;   // tag 2105 Код обработки запроса (ofdtag-2105)
		int    Status;           // tag 2109 Сведения о статусе товара (ofdtag-2109)
	};
	enum {
		pchznmfReturn              = 0x0001, // Возврат
		pchznmfFractional          = 0x0002, // Дробный товар
		pchznmfDraftBeerSimplified = 0x0004  // @v11.9.4 Упрощенный режим проведения разливного пива
	};
	int    PreprocessChZnMark(const char * pMarkCode, double qtty, int uomId, uint uomFragm, uint flags, PreprocessChZnCodeResult * pResult);

	int    SessID;
	int    LastError;
	int    FatalFlags;	// Флаги фатального сотояния. Нужно для возвращения значения в сообщении об ошибке.
	int    LastStatus;  // Последний статус ККМ или документа
	SVerT  OfdVer;      // @v11.1.9
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
			Construct();
		}
		OpLogBlock(const char * pLogFileName, const char * pOp, const char * pParam, const char * pExtMsg) : StartClk(clock()), Op(pOp), 
			Param(pParam), ExtMsg(pExtMsg), LogFileName(pLogFileName)
		{
			Construct();
		}
		~OpLogBlock()
		{
			if(LogFileName.NotEmpty() && Op.NotEmpty()) {
				const long end_clk = clock();
				SString line_buf;
				line_buf.CatCurDateTime(DATF_DMY|DATF_CENTURY, TIMF_HMS).Tab().Cat(Op).Tab().Cat("finish").Tab().Cat(end_clk-StartClk);
				if(Reply.NotEmpty())
					line_buf.Tab().Cat(Reply);
				SLS.LogMessage(LogFileName, line_buf, 8192);
			}
		}
		void   SetReply(const char * pReply) 
		{
			Reply = pReply;
		}
	private:
		void   Construct()
		{
			if(LogFileName.NotEmpty() && Op.NotEmpty()) {
				SString line_buf;
				line_buf.CatCurDateTime(DATF_DMY|DATF_CENTURY, TIMF_HMS).Tab().Cat(Op).Tab().Cat("start");
				line_buf.Tab();
				/*@v11.9.9
				if(Param.NotEmpty())
					line_buf.Cat(Param);
				*/
				if(ExtMsg.NotEmpty())
					line_buf.Tab().Cat(ExtMsg);
				// @v11.9.9 {
				{
					line_buf.CatChar('|').Space().Cat(Op).Semicol();
					if(Param.NotEmpty()) {
						SString temp_buf;
						StringSet ss(CHR_FS, Param);
						for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
							line_buf.Cat(temp_buf).Semicol();
						}
					}
				}
				// } @v11.9.9 
				SLS.LogMessage(LogFileName, line_buf, 8192);
			}
		}
		const long StartClk;
		SString LogFileName;
		SString Op;
		SString ExtMsg;
		SString Param;
		SString Reply;
	};
	//
	// Налоговой ставки, установленная в аппарате
	//
	struct DvcTaxEntry {
		DvcTaxEntry() : Rate(0.0)
		{
			Name[0] = 0;
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
	void   SetLastItems(const char * pCmd, const char * pParam);
	int    ReadConfigTab(int arg1, int arg2, SString & rOut, SString & rError);
	int    WriteConfigTab(int arg1, int arg2, int val, SString & rOut, SString & rError);
	int    ExecCmd(const char * pHexCmd, const char * pInput, SString & rOut, SString & rError);

	SString LogFileName;
	StringSet RetTknzr;
};

static PiritEquip * P_Pirit = 0;

PiritEquip::PiritEquip() : SessID(0), LastError(0), FatalFlags(0), LastStatus(0), RetTknzr("\x1c")
{
	{
		CommPortTimeouts cpt;
		CommPort.GetTimeouts(&cpt);
		cpt.Get_Delay = 20;
		CommPort.SetTimeouts(&cpt);
	}
	Check.Z();
	{
		SString exe_file_name = SLS.GetExePath();
		if(exe_file_name.NotEmptyS()) {
			SFsPath ps;
			ps.Split(exe_file_name);
			ps.Nam = "pirit";
			ps.Ext = "log";
			ps.Merge(LogFileName);
		}
	}
}

PiritEquip::~PiritEquip()
{
}

static const SIntToSymbTabEntry Pirit_ErrMsg[] = {
	{ PIRIT_ERRSTATUSFORFUNC,    "Функция невыполнима при данном статусе ККМ"},
	{ PIRIT_ERRFUNCNUMINCOMMAND, "В команде указан неверный номер функции"},
	{ PIRIT_ERRCMDPARAMORFORMAT, "Некорректный формат или параметр команды"},
	{ PIRIT_ENDOFPAPER,       "Конец бумаги"},
	{ PIRIT_PRNNOTREADY,      "Принтер не готов"},
	{ PIRIT_SESSOVER24H,      "Текущая смена больше 24 часов. Установка даты времени больше чем на 24 часа."},
	{ PIRIT_DIFDATE,          "Дата и время на ККМ отличаются от системных на 8 минут. Проверьте время на компьютере"},
	{ PIRIT_DATELSLASTOP,	  "Системная дата меньше даты последней фискальной операции, зарегистрированной в ККМ. Проверьте время на компьютере. Если оно верно, обратитесь в ЦТО."},
	{ PIRIT_NODATAINJOURNAL,  "Нет данных в журнале" }, // @v11.8.8
	{ PIRIT_FATALERROR,		  "Фатальная ошибка ККМ"},
	{ PIRIT_FMOVERFLOW,		  "Нет свободного места в фискальной памяти ККМ"},
	{ PIRIT_OFDPROVIDER,      "Ошибка оператора фискальных данных"},
	{ PIRIT_ECRRFORMAT,		  "Некорректный формат или параметр команды ЭКЛЗ"},
	{ PIRIT_ECRACCIDENT,      "Авария ЭКЛЗ"},
	{ PIRIT_KCACCIDENT,		  "Авария КС (криптографического сопроцессора)в составе ЭКЛЗ"},
	{ PIRIT_ECRTIMEOUT,		  "Исчерпан временной ресурс использования ЭКЛЗ"},
	{ PIRIT_ECROVERFLOW,      "ЭКЛЗ переполнена"},
	{ PIRIT_ECRERRORDATETIME, "Неверные дата или время"},
	{ PIRIT_ECRNODATA,        "Нет запрошенных данных"},
	{ PIRIT_ECRTOOMUCH,       "Переполнение (отрицательный итог документа, слишком много отделов для клиента)"},
	{ PIRIT_NOANSWER,         "Нет ответа от ЭКЛЗ"},
	{ PIRIT_ECRERREXCHANGE,   "Ошибка при обмене данными с ЭКЛЗ"},
	{ PIRIT_NOTENOUGHPARAM,   "Не достаточно параметров для работы устройства"},
	{ PIRIT_UNCNKOWNCOMMAND,  "Передана неизвестная команда"},
	{ PIRIT_NOTINITED,        "Ошибка инициализации"},
	{ PIRIT_NOTCONNECTED,     "Соединение не установлено"},
	{ PIRIT_ECRERRORSTATUS,   "Некорректное состояние ЭКЛЗ"},
	{ PIRIT_ECRFMOVERFLOW,    "ЭКЛЗ или ФП переполнена"},
	{ PIRIT_ECRFATALERR,      "Ошибка ЭКЛЗ. Обратитесь в ЦТО"},
	{ PIRIT_NOTSENT,          "Ошибка передачи данных"},
	{ PIRIT_NOTENOUGHMEM,     "Недостаточный размер выходного массива"},
	{ PIRIT_ERRLOGOSIZE,      "Логотип должен иметь размеры: ширина - не более 576 точек, высота - 126 точек"},
	{ PIRIT_ERRLOGOFORMAT,    "Логотип должен быть монохромным в формате BMP"},
	{ PIRIT_ECRARCHOPENED,    "Архив ЭКЛЗ закрыт"},
	{ PIRIT_ECRNOTACTIVE,     "ЭКЛЗ не активирована"},
	{ PIRIT_NOTENOUGHTMEMFORSESSCLOSE,	 "Нет памяти для закрытия смены в ФП"},
	{ PIRIT_ERRFMPASS,		 "Был введен неверный пароль доступа к ФП"},
	{ PIRIT_SESSOPENEDTRYAGAIN,	 "Не было завершено закрытие смены, необходимо повторить операцию"},
	{ PIRIT_PRNTROPENED,		 "Открыта крышка принтера"},
	{ PIRIT_PRNCUTERR,		 "Ошибка резчика принтера"},
	{ PIRIT_NOCONCTNWITHPRNTR,	 "Нет связи с принтером"},
	{ 0x50, "Превышен размер данных TLV" },
	{ 0x51, "Нет транспортного соединения" },
	{ 0x52, "Исчерпан ресурс КС" },
	{ 0x54, "Исчерпана память хранения документов для ОФД" },
	{ 0x55, "Время нахождения в очереди самого старого сообщения на выдачу более 30 календарных дней." },
	{ 0x56, "Продолжительность смены ФН более 24 часов" },
	{ 0x57, "Разница более чем на 5 минут отличается от разницы, определенной по внутреннему таймеру ФН." },
	{ 0x60, "Неверное сообщение от ОФД" },
	{ 0x61, "Нет связи с ФН" },
	{ 0x62, "Ошибка обмена с ФН" },
	{ 0x63, "Слишком длинная команда для посылки в ФН" },
	{ 0x7E, "В реквизите 2007 содержится КМ, который ранее не проверялся в ФН" },
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

static void FASTCALL CreateStr(const char * pValue, SString & rDst) { rDst.Cat(pValue).CatChar(CHR_FS); }
static void FASTCALL CreateChZnCode(const char * pValue, SString & rDst) 
{ 
	//dst.Cat(pValue).CatChar(CHR_FS); 
	const size_t len = sstrlen(pValue);
	for(size_t i = 0; i < len; i++) {
		const char c = pValue[i];
		if(c == 0x1D)
			rDst.Cat("$1D");
		else
			rDst.CatChar(c);
	}
	rDst.CatChar(CHR_FS);
}
static void FASTCALL CreateStr(int value, SString & dst) { dst.Cat(value).CatChar(CHR_FS); }
static void FASTCALL CreateStr(int64 value, SString & dst) { dst.Cat(value).CatChar(CHR_FS); }
// @v11.2.11 static void FASTCALL CreateStr(double value, SString & dst) { dst.Cat(value, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar(CHR_FS); } // @v11.2.9 0-->MKSFMTD(0, 3, NMBF_NOTRAILZ)
static void FASTCALL CreateStr(double value, SString & dst) { dst.Cat(value).CatChar(CHR_FS); } // @v11.2.9 0-->MKSFMTD(0, 3, NMBF_NOTRAILZ) // @v11.2.11

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
			if(DvcTaxArray[tidx].Name[0] || tidx == 3) { // (|| tidx == 3) костыль - это "освобожден от ндс" в трактовке Дрим Кас
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
				if(_vat_rate == 18.0 || _vat_rate == 20.0) {
					tax_entry_n = 0;
					tax_entry_id_result = 1;
				}
				else if(_vat_rate == 10.0) {
					tax_entry_n = 1;
					tax_entry_id_result = 2;
				}
				// @v12.2.1 {
				else if(_vat_rate == 7.0) {
					tax_entry_n = 7;
				}
				else if(_vat_rate == 5.0) {
					tax_entry_n = 6;
				}
				// } @v12.2.1 
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
	return tax_entry_n;
}

int PiritEquip::ExecCmd(const char * pHexCmd, const char * pInput, SString & rOut, SString & rError)
{
	int    ok = 1;
	OpLogBlock __oplb(LogFileName, pHexCmd, pInput, 0);
	THROWERR(PutData(pHexCmd, pInput), PIRIT_NOTSENT);
	THROW(GetWhile(rOut, rError));
	__oplb.SetReply(rOut);
	CATCHZOK
	return ok;
}

int PiritEquip::RunOneCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
{
	int    ok = 0;
	int    val = 0;
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
	PPDrvInputParamBlock pb(temp_buf);
	if(LastError == PIRIT_NOTENOUGHMEM) {
		strnzcpy(pOutputData, LastStr, outSize);
		LastError = 0;
	}
	else { // if(LastError != NOTENOUGHMEM)
		if(cmd.IsEqiAscii("CONNECT")){
			SetLastItems(0, 0);
			if(pb.Get("PORT", param_val) > 0)
				Cfg.Port = param_val.ToLong();
			if(pb.Get("BAUDRATE", param_val) > 0)
				Cfg.BaudRate = param_val.ToLong();
			int    flag = 0;
			THROWERR(SetConnection(), PIRIT_NOTCONNECTED);
			THROW(GetCurFlags(2, flag));
			GetTaxTab();
		}
		else if(cmd.IsEqiAscii("CHECKSESSOVER")){
			SetLastItems(cmd, pInputData);
			int    flag = 0;
			THROW(GetCurFlags(2, flag));
			strcpy(pOutputData, (flag & 0x8) ? "1" : "0");
		}
		else if(cmd.IsEqiAscii("DISCONNECT")) {
			SetLastItems(0, 0);
			CloseConnection();
		}
		else if(cmd.IsEqiAscii("SETCONFIG")) {
			SetLastItems(0, 0);
			THROW(StartWork());
			if(pb.Get("LOGNUM", param_val) > 0)
				Cfg.LogNum = param_val.ToLong();
			/* @v11.5.0
			if(pb.Get("FLAGS", param_val) > 0)
				Cfg.Flags = param_val.ToLong();
			*/
			if(pb.Get("CSHRNAME", param_val) > 0)
				CshrName = param_val;
			if(pb.Get("PRINTLOGO", param_val) > 0)
				Cfg.Logo.Print = param_val.ToLong();
			// @v11.5.0 {
			if(pb.Get("LOGGING", param_val) > 0) {
				if(param_val.IsEqiAscii("false") || param_val.IsEqiAscii("no") || param_val == "0" )
					Cfg.LocalFlags &= ~Config::lfDoLog;
			}
			// } @v11.5.0 
			THROW(SetCfg());
		}
		else if(cmd.IsEqiAscii("SETLOGOTYPE")) {
			SetLastItems(0, 0);
			THROW(StartWork());
			if(pb.Get("LOGOTYPE", param_val) > 0)
				Cfg.Logo.Path = param_val;
			if(pb.Get("LOGOSIZE", param_val) > 0)
				Cfg.Logo.Size = param_val.ToLong();
			if(pb.Get("LOGOHEIGHT", param_val) > 0)
				Cfg.Logo.Height = param_val.ToLong();
			if(pb.Get("LOGOWIDTH", param_val) > 0)
				Cfg.Logo.Width = param_val.ToLong();
			THROW(SetLogotype(Cfg.Logo.Path, Cfg.Logo.Size, Cfg.Logo.Height, Cfg.Logo.Width));
		}
		else if(cmd.IsEqiAscii("DIAGNOSTICS")) { // @v11.1.9
			/*
				Номер запроса (DEC) 	Наименование Запроса 	Формат возвращаемых данных 	Комментарии
				1 	Вернуть заводской номер ККТ 	Строка
				2 	Вернуть идентификатор прошивки 	Целое число
				3 	Вернуть ИНН 	Строка
				4 	Вернуть регистрационный номер ККТ 	Строка
				5 	Вернуть дату и время последней фискальной операции 	Дата, Время
				6 	Вернуть дату регистрации / перерегистрации 	Дата
				7 	Вернуть сумму наличных в денежном ящике 	Дробное число
				8 	Вернуть номер следующего документа 	Целое число
				9 	Вернуть номер смены регистрации 	Целое число
				10 	Вернуть номер следующего X отчета 	Целое число
				11 	Вернуть текущий операционный счетчик 	Строка
				12 	Вернуть нарастающий итог 	Дробное число, Дробное число, Дробное число, Дробное число 	Продажа (приход), Возврат (возврат прихода), Покупка (расход), Возврат покупки (возврат расхода)
				15 	Вернуть тип прошивки 	Целое число 	0 - стандартная прошивка, 1 - отладочный комплект
				16 	Вернуть размер бумаги текущего дизайна 	Целое число 	0 - 80мм, 1 - 57мм
				17 	Вернуть дату и время открытия смены 	Дата, Время
				18 	Вернуть количество символов в строке 	Целое число 	Для этого запроса можно вводить дополнительный входной параметр – номер шрифта. По умолчанию номер шрифта = 0
				19 	Вернуть содержание регистра CID SD карты 	Строка 	Возвращается 16 байт регистра CID в HEX виде, начиная со старшего
				20 	Вернуть содержание регистра CSD SD карты 	Строка 	Возвращается 16 байт регистра CSD в HEX виде, начиная со старшего
				21 	Вернуть модель устройства 	Целое число 	1 - Viki Mini, 2 - Viki Tower, 3 - Viki Print 57, 4 - Viki Print 57+, 5 - Viki Print 80+
				22 	Вернуть битовую маску поддерживаемых интерфейсов и устройств. Если бит установлен - интерфейс или устройство может быть использовано 	Целое число 	Бит 0 - Зарезервирован, Бит 1 - Зарезервирован, Бит 2 - Наличие SD, Бит 3 - Наличие ФН
				23 	Вернуть систему налогообложения и режим работы и ФН 	Целое число, Целое число, Целое число 	Система налогообложения, Режим работы, Дополнительный режим работы
				24 	Вернуть максимальное количество дополнительных строк в начале и в конце чека 	Целое число, Целое число 	Максимальное количество строк в начале чека, Максимальное количество строк в конце чека
				40 	Вернуть состояние перехода на НДС 20% 	Целое число 	0 - переход на НДС 20% не был выполнен, 1 - ККТ работает с НДС 20%
				70 	Вернуть рабочий идентификатор прошивки 	Строка 	Формат x.y.z, где x,y,z - числа до 3х знаков
				71 	Вернуть рабочий идентификатор wifi 	Строка 	Формат x.y.z, где x,y,z - числа до 3х знаков
			*/
			SString result_buf;
			StringSet dataset(CHR_FS, 0);
			{
				CreateStr(1, str.Z());
				ExecCmd("02", str, out_data, r_error); // заводской номер ККТ / Строка
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					result_buf.CatEq("manuf-serial", str).Semicol();
				}
			}
			{
				CreateStr(2, str.Z());
				ExecCmd("02", str, out_data, r_error); // идентификатор прошивки / Целое число
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					result_buf.CatEq("device-firmware", str).Semicol();
				}
			}
			{
				CreateStr(3, str.Z());
				ExecCmd("02", str, out_data, r_error); // ИНН / Строка
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					result_buf.CatEq("inn", str).Semicol();
				}
			}
			{
				CreateStr(4, str.Z());
				ExecCmd("02", str, out_data, r_error); // регистрационный номер ККТ / Строка
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					result_buf.CatEq("device-reg-number", str).Semicol();
				}
			}
			{
				CreateStr(5, str.Z());
				ExecCmd("02", str, out_data, r_error); // дату и время последней фискальной операции / Дата, Время
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 2) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					temp_buf.Z();
					temp_buf.Cat(str);
					dataset.get(&k, str);
					temp_buf.Space().Cat(str);
					result_buf.CatEq("lastop-time", temp_buf).Semicol();
				}
			}
			{
				CreateStr(6, str.Z());
				ExecCmd("02", str, out_data, r_error); // дату регистрации/перерегистрации / Дата
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					result_buf.CatEq("device-reg-date", str).Semicol();
				}
			}
			{
				CreateStr(7, str.Z());
				ExecCmd("02", str, out_data, r_error); // сумму наличных в денежном ящике / Дробное число
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					result_buf.CatEq("cashbox", str).Semicol();
				}
			}
			{
				CreateStr(8, str.Z());
				ExecCmd("02", str, out_data, r_error); // номер следующего документа / Целое число
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					result_buf.CatEq("next-doc-number", str).Semicol();
				}
			}
			{
				CreateStr(9, str.Z());
				ExecCmd("02", str, out_data, r_error); // номер смены регистрации / Целое число
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					result_buf.CatEq("shift-number", str).Semicol();
				}
			}
			{
				CreateStr(10, str.Z());
				ExecCmd("02", str, out_data, r_error); // номер следующего X отчета / Целое число
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					result_buf.CatEq("next-xrep-number", str).Semicol();
				}
			}
			{
				CreateStr(11, str.Z());
				ExecCmd("02", str, out_data, r_error); // текущий операционный счетчик / Строка
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					result_buf.CatEq("opercounter", str).Semicol();
				}
			}
			{
				CreateStr(15, str.Z());
				ExecCmd("02", str, out_data, r_error); // тип прошивки / Целое число / 0 - стандартная прошивка, 1 - отладочный комплект
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					temp_buf.Z();
					if(str == "0")
						temp_buf = "std";
					else if(str == "1")
						temp_buf = "debug";
					else
						(temp_buf = "unkn").Space().CatParStr(str);
					result_buf.CatEq("firmware-type", temp_buf).Semicol();
				}
			}
			{
				CreateStr(16, str.Z());
				ExecCmd("02", str, out_data, r_error); // размер бумаги текущего дизайна / Целое число / 0 - 80мм, 1 - 57мм
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					temp_buf.Z();
					if(str == "0")
						temp_buf = "80mm";
					else if(str == "1")
						temp_buf = "57mm";
					else
						(temp_buf = "unkn").Space().CatParStr(str);
					result_buf.CatEq("paper-size", temp_buf).Semicol();
				}
			}
			{
				CreateStr(17, str.Z());
				ExecCmd("02", str, out_data, r_error); // дату и время открытия смены / Дата, Время
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					temp_buf.Z();
					temp_buf.Cat(str);
					dataset.get(&k, str);
					temp_buf.Space().Cat(str);
					result_buf.CatEq("shift-open-time", temp_buf).Semicol();
				}
			}
			{
				CreateStr(18, str.Z());
				ExecCmd("02", str, out_data, r_error); // количество символов в строке / Целое число / Для этого запроса можно вводить дополнительный входной параметр – номер шрифта. По умолчанию номер шрифта = 0
			}
			{
				CreateStr(21, str.Z());
				ExecCmd("02", str, out_data, r_error); // модель устройства / Целое число / 1 - Viki Mini, 2 - Viki Tower, 3 - Viki Print 57, 4 - Viki Print 57+, 5 - Viki Print 80+
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					temp_buf.Z();
					if(str == "1")
						temp_buf = "Viki Mini";
					else if(str == "2")
						temp_buf = "Viki Tower";
					else if(str == "3")
						temp_buf = "Viki Print 57";
					else if(str == "4")
						temp_buf = "Viki Print 57+";
					else if(str == "5")
						temp_buf = "Viki Print 80+";
					else
						(temp_buf = "unkn").Space().CatParStr(str);
					result_buf.CatEq("model", temp_buf).Semicol();
				}
			}
			{
				CreateStr(70, str.Z());
				ExecCmd("02", str, out_data, r_error); // рабочий идентификатор прошивки / Строка / Формат x.y.z, где x,y,z - числа до 3х знаков
				dataset.setBuf(out_data);
				const uint dsc = dataset.getCount();
				if(dsc > 1) {
					uint k = 0;
					dataset.get(&k, str); // Номер запроса
					dataset.get(&k, str);
					result_buf.CatEq("firmware-id", str).Semicol();
				}
			}
			if(outSize < result_buf.BufSize()){
				NotEnoughBuf(str);
				strnzcpy(pOutputData, result_buf.cptr(), outSize);
				ok = 2;
			}
			else
				strnzcpy(pOutputData, result_buf.cptr(), outSize);
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
			str.CatDivIfNotEmpty(';', 0).CatEq("CHECKSTRLEN", "130");
			if(outSize < str.BufSize()){
				NotEnoughBuf(str);
				memcpy(pOutputData, str, outSize);
				ok = 2;
			}
			else
				memcpy(pOutputData, str, str.BufSize());
		}
		else if(cmd.IsEqiAscii("OPENSESSION")) { // @v11.3.12
			int    flag = 0;
			SetLastItems(cmd, pInputData);
			THROW(StartWork());
			THROW(GetCurFlags(2, flag));
			if(!(flag & 0x4)) { // Если смена уже открыта, то ничего делать не надо
				/*
					(Имя оператора) Имя оператора
					(Строка) Адрес пользователя (тег 1009). Это поле используется, если указанные реквизиты отличны от реквизитов, переданных при формировании отчета о регистрации ККТ.
					(Строка) Место расчетов (тег 1187). Это поле используется, если указанные реквизиты отличны от реквизитов, переданных при формировании отчета о регистрации ККТ
				*/
				CreateStr(CshrName, str.Z());
				THROW(ExecCmd("23", str, out_data, r_error));
				THROW(GetCurFlags(2, flag));
				if(!(flag & 0x4))  // Проверяем флаг "смена открыта"
					ok = 0;
			}
		}
		else if(cmd.IsEqiAscii("ZREPORT")) {
			SetLastItems(cmd, pInputData);
			THROW(StartWork());
			CreateStr(CshrName, str.Z()); // @v11.3.12 str-->str.Z()
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
		else if(cmd.IsEqiAscii("CUT")) {
			SetLastItems(cmd, pInputData);
			THROW(ExecCmd("34", str, out_data, r_error));
		}
		else if(cmd.IsEqiAscii("ACCEPTCHZNCODE")) { // @v11.1.11
			str.Z();
			CreateStr(2, str);
			CreateStr(1, str);
			THROW(ExecCmd("79", str, out_data, r_error)); // query=2
		}
		else if(cmd.IsEqiAscii("REJECTCHZNCODE")) { // @v11.1.11
			str.Z();
			CreateStr(2, str);
			CreateStr(static_cast<int>(0), str);
			THROW(ExecCmd("79", str, out_data, r_error)); // query=2
		}
		else if(cmd.IsEqiAscii("PREPROCESSCHZNCODE")) { // @v11.1.10
			double qtty = 1.0;
			double phqtty = 0.0; // @v11.9.4
			SString result_buf;
			SString chzn_code;
			SString chzn_gtin;
			SString chzn_serial;
			SString chzn_partn;
			int   chzn_prodtype = 0;
			int   uom_fragm = 0;
			int   suom_id = 0; // @v11.9.4
			bool  is_draftbeer_simplified = false; // @v11.9.4
			PreprocessChZnCodeResult result;
			SetLastItems(cmd, pInputData);
			THROW(StartWork());
			if(pb.Get("QUANTITY", param_val) > 0)
				qtty = param_val.ToReal();
			if(pb.Get("PHQTTY", param_val) > 0) // @v11.9.4
				phqtty = param_val.ToReal();
			if(pb.Get("UOMID", param_val) > 0) // @v11.9.4
				suom_id = param_val.ToLong();
			if(pb.Get("DRAFTBEERSIMPLIFIED", param_val) > 0) // @v11.9.4
				is_draftbeer_simplified = true;
			if(pb.Get("CHZNCODE", param_val) > 0)
				chzn_code = param_val;
			if(pb.Get("CHZNGTIN", param_val) > 0)
				chzn_gtin = param_val;
			if(pb.Get("CHZNSERIAL", param_val) > 0)
				chzn_serial = param_val;
			if(pb.Get("CHZNPARTN", param_val) > 0)
				chzn_partn = param_val;
			if(pb.Get("CHZNPRODTYPE", param_val) > 0)
				chzn_prodtype = param_val.ToLong();
			if(pb.Get("UOMFRAGM", param_val) > 0) // @v11.2.5
				uom_fragm = inrangeordefault(param_val.ToLong(), 1L, 100000L, 0L);
			if(chzn_code.NotEmptyS()) {
				const char * p_serial = chzn_serial.NotEmpty() ? chzn_serial.cptr() : chzn_partn.cptr();
				if(!isempty(p_serial) && chzn_gtin.NotEmpty()) {
					(chzn_code = chzn_gtin).Cat(p_serial);
				}
				//int    rl = STokenRecognizer::EncodeChZn1162(product_type_bytes, Check.ChZnGTIN, p_serial, chzn_1162_bytes, sizeof(chzn_1162_bytes));
				uint pchznm_flags = 0;
				if(is_draftbeer_simplified)
					pchznm_flags |= pchznmfDraftBeerSimplified;
				THROW(PreprocessChZnMark(chzn_code, fabs(qtty), suom_id, uom_fragm, pchznm_flags, &result) > 0);
				result_buf.Z().CatEq("CheckResult", result.CheckResult).Semicol().
					CatEq("Reason", result.Reason).Semicol().
					CatEq("ProcessingResult", result.ProcessingResult).Semicol().
					CatEq("ProcessingCode", result.ProcessingCode).Semicol().
					CatEq("Status", result.Status);
				if(outSize < result_buf.BufSize()){
					NotEnoughBuf(str);
					strnzcpy(pOutputData, result_buf.cptr(), outSize);
					ok = 2;
				}
				else
					strnzcpy(pOutputData, result_buf.cptr(), outSize);
			}
		}
		else if(cmd.IsEqiAscii("OPENCHECK_CORRECTION")) { // @v12.3.4
			// @todo
			//THROW(RunCheck(6));
		}
		else if(cmd.IsEqiAscii("OPENCHECK")) {
			SetLastItems(cmd, pInputData);
			THROW(StartWork());
			if(pb.Get("CHECKTYPE", param_val) > 0) {
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
			if(pb.Get("CHECKNUM", param_val) > 0)
				Check.CheckNum = param_val.ToLong();
			if(pb.Get("CHECKTIMESTAMP", param_val) > 0) { // @v11.2.3
				LDATETIME dtm;
				if(dtm.Set(param_val, DATF_ISO8601CENT, 0))
					Check.Timestamp = dtm;
			}
			if(pb.Get("TAXSYSTEM", param_val) > 0) {
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
					case /*TAXSYSK_SIMPLIFIED_PROFIT*/6: Check.TaxSys = 2; break;
					default: Check.TaxSys = -1; break;
				}
			}
			// @v11.1.9 {
			if(pb.Get("OFDVER", param_val) > 0) {
				OfdVer.FromStr(param_val);
			}
			// } @v11.1.9
			// @v11.3.6 {
			if(pb.Get("PAPERLESS", param_val) > 0) {
				Check.CheckType |= 0x80;
			}
			// } @v11.3.6
			if(pb.Get("DRAFTBEERSIMPLIFIED", param_val) > 0) // @v11.9.4
				Check.DraftBeerSimplifiedCode = param_val;
			else
				Check.DraftBeerSimplifiedCode.Z(); // @v12.0.4
			// @v11.8.0 {
			{
				if(pb.Get("PRESCRDATE", param_val) > 0)
					Check.PrescrDate = strtodate_(param_val, DATF_ISO8601CENT);
				if(pb.Get("PRESCRSERIAL", param_val) > 0)
					Check.PrescrSerial = param_val;
				if(pb.Get("PRESCRNUMB", param_val) > 0)
					Check.PrescrNumber = param_val;
			}
			// } @v11.8.0 
			THROW(RunCheck(0));
		}
		else if(cmd.IsEqiAscii("CLOSECHECK")) {
			SetLastItems(cmd, pInputData);
			Check.PaymCash = 0.0;
			Check.PaymBank = 0.0;
			Check.PaymCCrdCard = 0.0;
			if(pb.Get("PAYMCASH", param_val) > 0)
				Check.PaymCash = param_val.ToReal();
			if(pb.Get("PAYMCARD", param_val) > 0)
				Check.PaymBank = param_val.ToReal();
			if(pb.Get("PAYMCCRD", param_val) > 0)
				Check.PaymCCrdCard = param_val.ToReal();
			if(pb.Get("CHZNSID", param_val) > 0)
				Check.ChZnSid = param_val;
			// @v11.3.6 {
			if(pb.Get("BUYERSPHONE", param_val) > 0)
				Check.BuyersPhone = param_val;
			if(pb.Get("BUYERSEMAIL", param_val) > 0)
				Check.BuyersEmail = param_val;
			// } @v11.3.6 
			THROW(RunCheck(1));
		}
		else if(cmd.IsEqiAscii("CHECKCORRECTION")) {
			int    is_there_vatamt = 0;
			CcFiscalCorrection blk;
			blk.VatRate = -1.0;
			blk.Operator = CshrName;
			if(pb.Get("PAYMCASH", param_val) > 0)
				blk.AmtCash = R2(param_val.ToReal());
			else if(pb.Get("PAYMCARD", param_val) > 0)
				blk.AmtBank = R2(param_val.ToReal());
			if(pb.Get("FISCALSIGN", param_val) > 0) { // @v12.3.3
				blk.FiscalSign = param_val;
			}
			if(pb.Get("PREPAY", param_val) > 0) {
			}
			if(pb.Get("POSTPAY", param_val) > 0) {
			}
			if(pb.Get("RECKONPAY", param_val) > 0) {
			}
			if(pb.Get("CODE", param_val) > 0) {
				blk.Code = param_val;
			}
			if(pb.Get("DATE", param_val) > 0) {
				blk.Dt = strtodate_(param_val, DATF_ISO8601CENT);
			}
			if(pb.Get("TEXT", param_val) > 0) {
				blk.DocMemo = param_val;
			}
			if(pb.Get("VATRATE", param_val) > 0) {
				blk.VatRate = R2(param_val.ToReal());
			}
			if(pb.Get("VATFREE", param_val) > 0) {
				if(param_val.IsEmpty() || param_val.IsEqiAscii("yes") || param_val.IsEqiAscii("true") || param_val == "1") {
					blk.Flags |= CcFiscalCorrection::fVatFree;
				}
			}
			{
				struct VatAmtListEntry {
					const char * P_Symb;
					double * P_Var;
				};
				/*non-static*/const VatAmtListEntry vat_amt_list[] = {
					{ "VATAMOUNT20", &blk.AmtVat20 },
					{ "VATAMOUNT18", &blk.AmtVat18 },
					{ "VATAMOUNT10", &blk.AmtVat10 },
					{ "VATAMOUNT05", &blk.AmtVat05 },
					{ "VATAMOUNT07", &blk.AmtVat07 },
					{ "VATAMOUNT00", &blk.AmtVat00 },
					{ "VATAMOUNT0",  &blk.AmtVat00 },
					{ "VATFREEAMOUNT",  &blk.AmtNoVat },
				};
				for(uint vli = 0; vli < SIZEOFARRAY(vat_amt_list); vli++) {
					const VatAmtListEntry & r_entry = vat_amt_list[vli];
					assert(!isempty(r_entry.P_Symb));
					assert(r_entry.P_Var != 0);
					if(pb.Get(r_entry.P_Symb, param_val) > 0) {
						*r_entry.P_Var = R2(param_val.ToReal());
						if(*r_entry.P_Var != 0.0)
							is_there_vatamt = 1;
					}
				}
			}
			if(!is_there_vatamt) {
				const double _amount = blk.AmtBank + blk.AmtCash;
				if(blk.VatRate == 20.0)
					blk.AmtVat20 = _amount;
				else if(blk.VatRate == 18.0)
					blk.AmtVat18 = _amount;
				else if(blk.VatRate == 10.0)
					blk.AmtVat10 = _amount;
				else if(feqeps(blk.VatRate, 5.0, 1E-6)) // @v12.2.10
					blk.AmtVat05 = _amount;
				else if(feqeps(blk.VatRate, 7.0, 1E-6)) // @v12.2.10
					blk.AmtVat07 = _amount;
				else if(blk.VatRate == 0.0)
					blk.AmtVat00 = _amount;
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
				const int vat_entry_n = ((blk.Flags & CcFiscalCorrection::fVatFree) || blk.VatRate >= 0.0) ? 
					IdentifyTaxEntry(blk.VatRate, LOGIC(blk.Flags & CcFiscalCorrection::fVatFree)) : 0/* @v12.3.3 (-1)-->(0)*/;
				SETFLAG(correction_type, 0x10, vat_entry_n < 0);
				SETFLAG(correction_type, 0x02, blk.AmtCash < 0.0 || blk.AmtBank < 0.0 || blk.AmtPrepay < 0.0);
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
				/* ???
					// 
					// CMD 0x58
					// Parameters:
					// (Имя оператора) Имя оператора
					// (Строка) Зарезервировано
					// (Строка) Зарезервировано
					// (Строка) Зарезервировано
					// (Строка) Зарезервировано
					// (Строка) Зарезервировано
					// (Число) Тип коррекции
					// (Дата) Дата документа основания коррекции
					// (Строка)[1..32] Номер документа основания коррекции
					// 
				*/
				CreateStr(CshrName, in_data);
				CreateStr(fabs(blk.AmtCash), in_data);
				CreateStr(fabs(blk.AmtBank), in_data);
				CreateStr(fabs(blk.AmtPrepay), in_data);
				CreateStr(fabs(blk.AmtPostpay), in_data);
				CreateStr(fabs(blk.AmtReckon), in_data);
				CreateStr(correction_type, in_data);
				CreateStr(temp_buf.Z().Cat(blk.Dt, DATF_DMY|DATF_NODIV), in_data);
				CreateStr(blk.Code, in_data);
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
					CreateStr(fabs(blk.AmtVat20), in_data);
					CreateStr(fabs(blk.AmtVat10), in_data);
					CreateStr(fabs(blk.AmtVat00), in_data);
					CreateStr(fabs(blk.AmtNoVat), in_data);
					CreateStr(/*fabs(blk.AmtVat18_118)*/0.0, in_data);
					CreateStr(/*fabs(blk.AmtVat10_110)*/0.0, in_data);
				}
				CreateStr("", in_data); // Дополнительный реквизит чека (БСО)
				//
				{
					// 19/06/2019 11:51:02	58	start	Master197850000211061900001возврат нал. от 28/03300000
					OpLogBlock __oplb(LogFileName, "58", in_data);
					THROW(ExecCmd("58", in_data, out_data, r_error));
				}
				// @todo PutOfdReq
				if(!isempty(blk.FiscalSign)) {
					SString local_err_code;
					THROW(PutOfdReq(1192, 12, "", blk.FiscalSign, local_err_code));
				}
			}
		}
		else if(cmd.IsEqiAscii("PRINTFISCAL")) {
			/*
				struct Prescription { // @v11.7.12
					Prescription();
					Prescription & Z();
					bool   IsValid() const;

					LDATE Dt;
					SString Serial;
					SString Number;
				};
				PRESCRDATE, PRESCRSERIAL, PRESCRNUMB
			*/
			double _vat_rate = 0.0;
			int   is_vat_free = 0;
			SetLastItems(cmd, 0);
			Check.Qtty = (pb.Get("QUANTITY", param_val) > 0) ? param_val.ToReal() : 0.0;
			Check.PhQtty = (pb.Get("PHQTTY", param_val) > 0) ? param_val.ToReal() : 0.0; // @v11.9.3
			if(pb.Get("UOMID", param_val) > 0) // @v11.9.5
				Check.UomId = param_val.ToLong();
			if(pb.Get("DRAFTBEERSIMPLIFIED", param_val) > 0) // @v11.9.4
				Check.DraftBeerSimplifiedCode = param_val;
			else
				Check.DraftBeerSimplifiedCode.Z(); // @v12.0.4
			if(pb.Get("PRICE", param_val) > 0)
				Check.Price = param_val.ToReal();
			Check.Department = (pb.Get("DEPARTMENT", param_val) > 0) ? param_val.ToLong() : 0;
			if(pb.Get("TEXT", param_val) > 0)
				Check.Text = param_val;
			if(pb.Get("CODE", param_val) > 0)
				Check.Code = param_val;
			if(pb.Get("CHZNCODE", param_val) > 0)
				Check.ChZnCode = param_val;
			if(pb.Get("CHZNGTIN", param_val) > 0)
				Check.ChZnGTIN = param_val;
			if(pb.Get("CHZNSERIAL", param_val) > 0)
				Check.ChZnSerial = param_val;
			if(pb.Get("CHZNPARTN", param_val) > 0)
				Check.ChZnPartN = param_val;
			if(pb.Get("CHZNPRODTYPE", param_val) > 0)
				Check.ChZnProdType = param_val.ToLong();
			if(pb.Get("CHZNPPRESULT", param_val) > 0) // @v11.1.11
				Check.ChZnPpResult = param_val.ToLong();
			if(pb.Get("CHZNPPSTATUS", param_val) > 0) // @v11.1.11
				Check.ChZnPpStatus = param_val.ToLong();
			if(pb.Get("CHZNSID", param_val) > 0) // @v11.2.4
				Check.ChZnSid = param_val;
			// @v12.1.1 {
			{
				if(pb.Get("CHZNPMREQID", param_val) > 0) {
					S_GUID uuid;
					if(uuid.FromStr(param_val))
						Check.ChZnPm_ReqId = uuid;
				}
				if(pb.Get("CHZNPMREQTIMESTAMP", param_val) > 0) {
					int64 ts = param_val.ToInt64();
					if(ts != 0)
						Check.ChZnPm_ReqTimestamp = ts;
				}
			}
			// } @v12.1.1 
			if(pb.Get("UOMFRAGM", param_val) > 0) // @v11.2.5
				Check.UomFragm = inrangeordefault(param_val.ToLong(), 1L, 100000L, 0L);
			if(pb.Get("VATRATE", param_val) > 0) {
				_vat_rate = R2(param_val.ToReal());
			}
			if(pb.Get("VATFREE", param_val) > 0) {
				if(param_val.IsEmpty() || param_val.IsEqiAscii("yes") || param_val.IsEqiAscii("true") || param_val == "1")
					is_vat_free = 1;
			}
			if(pb.Get("PAYMENTTERMTAG", param_val) > 0) {
				static const SIntToSymbTabEntry ptt_list[] = {
					{1, "PTT_FULL_PREPAY" },
					{2, "PTT_PREPAY" },
					{3, "PTT_ADVANCE"},
					{4, "PTT_FULLPAYMENT"},
					{5, "PTT_PARTIAL"},
					{6, "PTT_CREDITHANDOVER" },
					{7, "PTT_CREDIT" }
				};
				for(uint i = 0; i < SIZEOFARRAY(ptt_list); i++) {
					if(param_val.IsEqiAscii(ptt_list[i].P_Symb))
						Check.Ptt = ptt_list[i].Id;
				}
			}
			//@erik v10.4.12{
			if(pb.Get("SUBJTERMTAG", param_val) > 0) {
				static const SIntToSymbTabEntry stt_list[] = {
					{1, "STT_GOOD" },
					{2, "STT_EXCISABLEGOOD" },
					{3, "STT_EXECUTABLEWORK" },
					{4, "STT_SERVICE" },
					{5, "STT_BETTING" },
					{6, "STT_PAYMENTGAMBLING" },
					{7, "STT_BETTINGLOTTERY" },
					{8, "STT_PAYMENTLOTTERY" },
					{9, "STT_GRANTSRIGHTSUSEINTELLECTUALACTIVITY" },
					{10, "STT_ADVANCE" },
					{11, "STT_PAYMENTSPAYINGAGENT" },
					{12, "STT_SUBJTERM" },
					{13, "STT_NOTSUBJTERM" },
					{14, "STT_TRANSFERPROPERTYRIGHTS" },
					{15, "STT_NONOPERATINGINCOME" },
					{16, "STT_EXPENSESREDUCETAX" },
					{17, "STT_AMOUNTMERCHANTFEE" },
					{18, "STT_RESORTAEE" },
					{19, "STT_DEPOSIT" },
				};
				for(uint i = 0; i < SIZEOFARRAY(stt_list); i++) {
					if(param_val.IsEqiAscii(stt_list[i].P_Symb)) {
						Check.Stt = stt_list[i].Id;
						break;
					}
				}
				// } @erik v10.4.12
			}
			// @v11.8.0 {
			{
				Check.PrescrDate = (pb.Get("PRESCRDATE", param_val) > 0) ? strtodate_(param_val, DATF_ISO8601CENT) : ZERODATE;
				if(pb.Get("PRESCRSERIAL", param_val) > 0)
					Check.PrescrSerial = param_val;
				if(pb.Get("PRESCRNUMB", param_val) > 0)
					Check.PrescrNumber = param_val;
			}
			// } @v11.8.0 
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
			if(pb.Get("FONTSIZE", param_val) > 0)
				Check.FontSize = param_val.ToLong();
			if(pb.Get("TEXT", param_val) > 0)
				Check.Text = param_val;
			// @v11.0.9 {
			else
				Check.Text.Space();
			// } @v11.0.9
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
			if(pb.Get("TYPE", param_val) > 0) {
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
			if(pb.Get("WIDTH", param_val) > 0)
				bc_entry.Width = param_val.ToLong();
			if(pb.Get("HEIGHT", param_val) > 0)
				bc_entry.Height = param_val.ToLong();
			if(pb.Get("LABEL", param_val) > 0) {
				if(param_val.IsEqiAscii("above"))
					bc_entry.TextParam = 1;
				else if(param_val.IsEqiAscii("below"))
					bc_entry.TextParam = 2;
			}
			if(pb.Get("TEXT", param_val) > 0)
				(bc_entry.Code = param_val).Strip();
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
			if(pb.Get("AMOUNT", param_val)) // ! != 0 (пустое значение тоже подойдет)
				str.Cat("AMOUNT").Semicol();
			if(pb.Get("CHECKNUM", param_val))
				str.Cat("CHECKNUM").Semicol();
			if(pb.Get("CASHAMOUNT", param_val))
				str.Cat("CASHAMOUNT").Semicol();
			if(pb.Get("RIBBONPARAM", param_val))
				str.Cat("RIBBONPARAM").Semicol();
			ok = ReturnCheckParam(str, pOutputData, outSize); // Здесь может быть переполнение буфера
		}
		else if(cmd.IsEqiAscii("ANNULATE")) {
			SetLastItems(cmd, pInputData);
			THROW(StartWork());
			THROW(RunCheck(4));
		}
		else if(cmd.IsEqiAscii("INCASHMENT")) {
			SetLastItems(cmd, pInputData);
			if(pb.Get("AMOUNT", param_val) > 0)
				Check.IncassAmt = param_val.ToReal();
			THROW(RunCheck(5));
		}
		else if(cmd.IsEqiAscii("OPENBOX")) {
			SetLastItems(0, 0);
			THROW(OpenBox());
		}
		else if(cmd.IsEqiAscii("GETDEVICETIME")) {
			SString date_buf;
			SString time_buf;
			SString in_data;
			SString out_data;
			SString r_error;
			THROW(ExecCmd("13", in_data, out_data, r_error)); // Смотрим текщую дату/время на ККМ
			out_data.Divide(CHR_FS, date_buf, time_buf);
			LDATETIME dtm = ZERODATETIME;
			strtodate(date_buf, DATF_DMY|DATF_NODIV, &dtm.d);
			strtotime(time_buf, TIMF_HMS|TIMF_NODIV, &dtm.t);
			str.Z().Cat(dtm, DATF_ISO8601CENT, 0);
			memcpy(pOutputData, str, outSize);
			//CashDateTime.Z().Cat("Текущая дата на ККМ").CatDiv(':', 2).Cat(date).Space().Cat("Текущее время на ККМ").CatDiv(':', 2).Cat(time);
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
		else if(cmd.IsEmpty())
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
		if(LastStatus == CHECKOPENED)
			rMsg.CatDiv(':', 2).Cat("Чек открыт");
		else if(LastStatus == CHECKCLOSED)
			rMsg.CatDiv(':', 2).Cat("Чек закрыт");
		{
			SString cmd_str;
			if(LastCmd.IsEqiAscii("CHECKSESSOVER"))
				cmd_str = "Проверка на длительность открытой сессии";
			else if(LastCmd.IsEqiAscii("ZREPORT"))
				cmd_str = "Печать Z-отчета";
			else if(LastCmd.IsEqiAscii("XREPORT"))
				cmd_str = "Печать X-отчета";
			else if(LastCmd.IsEqiAscii("OPENCHECK"))
				cmd_str = "Открытие чека";
			else if(LastCmd.IsEqiAscii("OPENCHECK_CORRECTION")) // @v12.3.4
				cmd_str = "Открытие чека коррекции";
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
		rMsg.CatEq("ErrCode", LastError);
	}
	rMsg.Transf(CTRANSF_UTF8_TO_OUTER);
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
		CommPort.PutChr(CHR_ENQ); // Проверка связи с ККМ
		int r = CommPort.GetChr();
		if(r == CHR_ACK) {
			return 1;
		}
		else {
			++try_no;
			if(try_no >= max_tries) {
				if(LogFileName.NotEmpty()) {
					SString msg_buf;
					(msg_buf = "Error ENQ_ACK() tries exceeded").Space().CatChar('(').Cat(try_no).CatChar(')').Space().CatEq("reply", r);
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
	if((Cfg.ReadCycleCount > 0) || (Cfg.ReadCycleDelay > 0))
		CommPort.SetReadCyclingParams(Cfg.ReadCycleCount, Cfg.ReadCycleDelay);
	THROW(ENQ_ACK());
	if(LogFileName.NotEmpty()) {
		SString msg_buf;
		(msg_buf = "Connection is established").CatDiv(':', 2).CatEq("cbr", port_params.Cbr);
		SLS.LogMessage(LogFileName, msg_buf, 8192);
	}
	CATCH
		CommPort.ClosePort();
		if(LogFileName.NotEmpty()) {
			SString msg_buf;
			(msg_buf = "Error on connection").CatDiv(':', 2).CatEq("cbr", port_params.Cbr);
			SLS.LogMessage(LogFileName, msg_buf, 8192);
		}
		ok = 0;
	ENDCATCH
	return ok;
}

void PiritEquip::CloseConnection()
{
	CommPort.ClosePort();
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
	for(int i = 0; i < 6; i++) { // В пирите не более 6 налоговых ставок
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
	// @v11.5.0 {
	if(!(Cfg.LocalFlags & Config::lfDoLog)) {
		LogFileName.Z();
	}
	// } @v11.5.0 
	THROW(WriteConfigTab(10, 0, static_cast<int>(Cfg.LogNum), out_data, r_error));
	// Получаем номер текущей сессии
	CreateStr(1, in_data);
	{
		THROW(ExecCmd("01", in_data, out_data, r_error)); // Запрос номера текущей сессии(смены)
		{
			//StringSet delim_out(CHR_FS, out_data);
			uint   i = 0;
			RetTknzr.setBuf(out_data);
			RetTknzr.get(&i, out_data); // В out считан номер запроса
			RetTknzr.get(&i, out_data); // В out считан номер текущей сессии(смены)
			SessID = out_data.ToLong();
		}
	}
	CATCHZOK
	return ok;
}

int PiritEquip::StartWork(bool force)
{
	int    ok = 1;
	int    flag = 0;
	if(force) { // @v11.3.2
		flag = 0x1;
	}
	else {
		THROW(GetCurFlags(2, flag)); // Проверяем флаг "не была вызвана функция начало работы"
	}
	if(flag & 0x1) {
		SString temp_buf;
		SString date;
		SString time;
		SString datetime;
		SString in_data;
		SString out_data;
		SString r_error;
		SYSTEMTIME sys_dt_tm;
		THROW(ExecCmd("13", in_data, out_data, r_error)); // Смотрим текщую дату/время на ККМ
		out_data.Divide(CHR_FS, date, time);
		CashDateTime.Z().Cat("Текущая дата на ККМ").CatDiv(':', 2).Cat(date).Space().Cat("Текущее время на ККМ").CatDiv(':', 2).Cat(time);
		in_data.Z();
		GetLocalTime(&sys_dt_tm);
		GetDateTime(sys_dt_tm, datetime, 0);
		CreateStr(datetime, in_data);
		GetDateTime(sys_dt_tm, datetime, 1);
		CreateStr(datetime, in_data);
		THROW(ExecCmd("10", in_data, out_data, r_error));
		THROW(GetCurFlags(2, flag));
		if(!force) { // @v11.3.2
			if(!(flag & 0x4) && r_error.IsEqiAscii("0B")) {  // Проверяем что смена закрыта и код ошибки "дата и время отличаются от текущих даты и времени ККМ более чем на 8 минут"
				in_data.Z();
				GetLocalTime(&sys_dt_tm);
				GetDateTime(sys_dt_tm, datetime, 0);
				CreateStr(datetime, in_data);
				GetDateTime(sys_dt_tm, datetime, 1);
				CreateStr(datetime, in_data);
				THROW(ExecCmd("14", in_data, out_data, r_error)); // Устанавливаем системные дату и время в ККМ
			}
		}
	}
	CATCHZOK
	return ok;
}

int PiritEquip::GetCurFlags(int numFlags, int & rFlags)
{
	const  uint max_tries = 3;
	int    ok = 1;
	SString out_data;
	SString r_error;
	uint count = 0;
	rFlags = 0;
	int    flags_fatal_state = 0;
	int    flags_current_state = 0;
	int    flags_doc_status = 0;
	{
		OpLogBlock __oplb(LogFileName, "00", 0);
		THROWERR(PutData("00", 0), PIRIT_NOTSENT); // Запрос флагов статуса
		while(out_data.IsEmpty() && count < max_tries) {
			if(numFlags == 1) { // Если запрашиваем флаги фатального состояния, дабы не зациклиться
				GetData(out_data, r_error);
			}
			else {
				THROW(GetWhile(out_data, r_error));
			}
			count++;
		}
		{
			StringSet fl_pack(CHR_FS, out_data);
			int    fc = 0; // Считанное количество значений
			uint    sp = 0;
			if(fl_pack.get(&sp, out_data)) {
				flags_fatal_state = out_data.ToLong();
				if(fl_pack.get(&sp, out_data)) {
					flags_current_state = out_data.ToLong();
					if(fl_pack.get(&sp, out_data))
						flags_doc_status = out_data.ToLong();
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
		}
	}
	CATCHZOK
	return ok;
}
/*
	flags:
		pchznmfReturn     = 0x0001, // Возврат
		pchznmfFractional = 0x0002  // Дробный товар
*/
int PiritEquip::PreprocessChZnMark(const char * pMarkCode, double qtty, int uomId, uint uomFragm, uint flags, PreprocessChZnCodeResult * pResult)
{
	int    ok = 1;
	if(isempty(pMarkCode))
		ok = -1;
	else {
		SString temp_buf;
		SString in_data;
		SString out_data;
		SString r_error;
		//
			// --> 79/1
			// --> 79/2
			//
			// --> 79/15
		//
		//
		/*
			Входные параметры
				(Целое число) Номер запроса = 1
				(Строка)[0..128] Код маркировки
				(Целое число) Режим обработки кода маркировки = 0
				(Целое число) Планируемый статус товара(ofdtag-2003)
					Реквизит «планируемый статус товара» (ofdtag-2003) содержит сведения
					1 - Штучный товар, подлежащий обязательной маркировке средством идентификации, реализован
					2 - Мерный товар, подлежащий обязательной маркировке средством идентификации, в стадии реализации
					3 - Штучный товар, подлежащий обязательной маркировке средством идентификации, возвращен
					4 - Часть товара, подлежащего обязательной маркировке средством идентификации, возвращена
					255 - Статус товара, подлежащего обязательной маркировке средством идентификации, не изменился

				(Целое число) Количество товара (ofdtag-1023)
				(Целое число) Мера количества (ofdtag-2108)
					Реквизит «мера количества предмета расчета» (ofdtag-2108) содержит сведения
					0 - Применяется для предметов расчета, которые могут быть реализованы поштучно или единицами шт. или ед.
					10 - Грамм г
					11 - Килограмм кг
					12 - Тонна т
					20 - Сантиметр см
					21 - Дециметр дм
					22 - Метр м
					30 - Квадратный сантиметр кв. см
					31 - Квадратный дециметр кв. дм
					32 - Квадратный метр кв.
					40 - Миллилитр м или мл
					41 - Литр л
					42 - Кубический метр куб. м
					50 - Киловатт час кВт∙ч
					51 - Гигакалория Гкал
					70 - Сутки (день) сутки
					71 - Час час
					72 - Минута мин
					73 - Секунда с
					80 - Килобайт Кбайт
					81 - Мегабайт Мбайт
					82 - Гигабайт Гбайт
					83 - Терабайт Тбайт
					255 - Применяется при использовании иных единиц измерения, не поименованных в п.п. 1-23
				(Целое число) Режим работы

			Ответные параметры
				(Целое число) Номер запроса = 1
				(Целое число) Результат проверки КМ в ФН (ofdtag-2106)
				(Целое число) Причина того, что КМ не проверен в ФН
				(Целое число) Результаты обработки запроса (ofdtag-2005)
				(Целое число) Код обработки запроса (ofdtag-2105)
				(Целое число) Сведения о статусе товара (ofdtag-2109)
		*/
		in_data.Z();
		CreateStr(1, in_data);
		CreateChZnCode(pMarkCode, in_data);
		CreateStr(0, in_data);
		int   ps = 1; // Планируемый статус товара(ofdtag-2003)
		if(flags & pchznmfReturn)
			ps = (flags & pchznmfFractional) ? 4 : 2;
		else
			ps = (flags & pchznmfFractional) ? 2 : 1;
		CreateStr(ps, in_data);
		if(qtty > 0.0 && qtty < 1.0 && uomFragm > 0) {
			double ip = 0.0;
			double nmrtr = 0.0;
			double dnmntr = 0.0;
			if(fsplitintofractions(qtty, uomFragm, 1E-5, &ip, &nmrtr, &dnmntr))
				CreateStr(SLS.AcquireRvlStr().Cat(R0i(nmrtr)).Slash().Cat(R0i(dnmntr)), in_data);
			else
				CreateStr(1.0, in_data);
		}
		else {
			temp_buf.Z().Cat(qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ|NMBF_OMITEPS));
			CreateStr(/*static_cast<int>(qtty)*/temp_buf, in_data);
		}
		{
			// @v11.9.4 {
			int chzn_uom_id = 0;
			switch(uomId) {
				case SUOM_LITER: chzn_uom_id = 41; break;
				case SUOM_KILOGRAM: chzn_uom_id = 11; break;
			}
			// } @v11.9.4 
			CreateStr(chzn_uom_id, in_data); // uom
		}
		{
			// @v11.9.4 {
			int mode = 0;
			if(flags & pchznmfDraftBeerSimplified)
				mode = 2;
			// } @v11.9.4 
			CreateStr(mode, in_data); // Режим работы (Если = 1 - все равно проверять КМ в ИСМ, даже если ФН проверил код с отрицательным результатом)
		}
		THROW(ExecCmd("79", in_data, out_data, r_error)); // query=1
		if(pResult) {
			StringSet fl_pack(CHR_FS, out_data);
			int    fc = 0; // Считанное количество значений
			uint   sp = 0;
			if(fl_pack.get(&sp, out_data)) {
				int req_no = out_data.ToLong(); // номер запроса (1)
				if(fl_pack.get(&sp, out_data)) {
					pResult->CheckResult = out_data.ToLong();
					if(fl_pack.get(&sp, out_data)) {
						pResult->Reason = out_data.ToLong();
						if(fl_pack.get(&sp, out_data)) {
							pResult->ProcessingResult = out_data.ToLong();
							if(fl_pack.get(&sp, out_data)) {
								pResult->ProcessingCode = out_data.ToLong();
								if(fl_pack.get(&sp, out_data))
									pResult->Status = out_data.ToLong();
							}
						}
					}
				}
			}
		}
		// @v11.3.2 {
		{
			int fatal_flags = 0;
			if(GetCurFlags(1, fatal_flags) && fatal_flags != 0) {
				if(false) { // @v11.3.6
					if(LogFileName.NotEmpty()) {
						(out_data = "Try to reset fatal state flags after 79/1").Space().CatHex((long)fatal_flags);
						SLS.LogMessage(LogFileName, out_data, 8192);
					}
					THROW(StartWork(/*force*/true));
				}
			}
		}
		// } @v11.3.2 
	}
	CATCHZOK
	return ok;
}

int PiritEquip::RunCheck(int opertype)
{
	constexpr bool fractional_medcine_to_1291 = false/*не работает*/; // @v12.3.7 Если true, то дробное количество лекарств передаются в поле
		// количества команды 0x42 в виде n/m, в противном случае - в команде 0x24 в спец строке mdlp
	
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
			CreateStr(Check.CheckNum, in_data);
			if(Check.TaxSys >= 0 && Check.TaxSys <= 5)
				CreateStr(inrangeordefault(static_cast<long>(Check.TaxSys), 0L, 5L, 0L), in_data);
			THROW(ExecCmd("30", in_data, out_data, r_error));
			break;
		case 6: // Открыть чек коррекции
			{
				// @todo
				// THROW(ExecCmd("58", in_data, out_data, r_error));
			}
			break;
		case 1: // Закрыть документ
			// Проверяем наличие открытого документа
			THROW(gcf_result = GetCurFlags(3, flag));
			if((flag > 4) || (gcf_result < 3)) {
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
					in_data.Z();
					if(Check.PaymCCrdCard != 0) {
						CreateStr(13, in_data); // Тип оплаты //
						FormatPaym(Check.PaymCCrdCard, str);
						CreateStr(str, in_data);
						CreateStr("", in_data);
						THROW(ExecCmd("47", in_data, out_data, r_error));
					}
				}
				/*
					(Целое число) Флаг отрезки
					(Строка) Адрес покупателя
					(Число) Разные флаги
					(Строка) Зарезервировано
					(Строка) Зарезервировано
					(Строка) Зарезервировано
					(Строка) Название дополнительного реквизита пользователя
					(Строка) Значение дополнительного реквизита пользователя
					(Строка)[0..128] Покупатель
					(Строка)[0..12] ИНН покупателя
				*/
				in_data.Z();
				CreateStr((Cfg.Flags & 0x08000000L) ? 1 : 0, in_data); // Чек не отрезаем (1) для сервисных документов, для остальных - отрезаем (0)
				// Адрес покупателя {
				// @v11.3.6 {
				if(Check.BuyersEmail.NotEmptyS())
					CreateStr(Check.BuyersEmail, in_data); 
				else if(Check.BuyersPhone.NotEmptyS())
					CreateStr(Check.BuyersPhone, in_data); 
				else // } // @v11.3.6
					CreateStr("", in_data); 
				// } 
				CreateStr(static_cast<int>(0), in_data); // (число) Разные флаги
				CreateStr("", in_data); // Зарезервировано
				CreateStr("", in_data); // Зарезервировано
				CreateStr("", in_data); // Зарезервировано
				if(Check.ChZnSid.NotEmpty()) {
					CreateStr("mdlp", in_data); // Название дополнительного реквизита пользователя
					str.Z().Cat("sid").Cat(Check.ChZnSid).CatChar('&');
					CreateStr(str, in_data); // Значение дополнительного реквизита пользователя
					if(LogFileName.NotEmpty()) {
						(out_data = "1084").CatDiv(':', 2).CatEq("mdlp", str);
						SLS.LogMessage(LogFileName, out_data, 8192);
					}
				}
				THROW(ExecCmd("31", in_data, out_data, r_error));
				// gcf_result = GetCurFlags(3, flag); // @debug
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
			THROW(GetCurFlags(3, flag));
			{
				uint16 product_type_bytes = 0;
				uint8  chzn_1162_bytes[128];
				if(Check.DraftBeerSimplifiedCode.NotEmpty() && Check.PhQtty > 0.0) { 
					product_type_bytes = 0x444D;
					int    rl = STokenRecognizer::EncodeChZn1162(product_type_bytes, Check.ChZnGTIN, 0, chzn_1162_bytes, sizeof(chzn_1162_bytes));
					if(rl > 0) {
						str.Z();
						// @v11.1.10 {
						if(OfdVer.IsGe(1, 2, 0)) {
							{
								in_data.Z();
								CreateStr(15, in_data);
								CreateChZnCode(/*Check.ChZnCode*/Check.ChZnGTIN, in_data); // (Строка)[0..128] Код маркировки
								CreateStr(/*Check.ChZnPpStatus*/2, in_data); // (Целое число) Присвоенный статус товара (ofdtag-2110)
								CreateStr(0L, in_data); // (Целое число) Режим обработки кода маркировки (ofdtag-2102) = 0
								CreateStr(Check.ChZnPpResult, in_data); // (Целое число) Результат проведенной проверки КМ (ofdtag-2106)
								{
									int chzn_uom_id = 0;
									switch(Check.UomId) {
										case SUOM_LITER: chzn_uom_id = 41; break;
										case SUOM_KILOGRAM: chzn_uom_id = 11; break;
									}
									CreateStr(chzn_uom_id, in_data); // uom
								}
								THROW(ExecCmd("79", in_data, out_data, r_error)); // query=15
							}
							{
								in_data.Z(); // @v11.2.3 @fix
								CreateStr(str.Z(), in_data); // #1 (tag 1162) Код товарной номенклатуры (для офд 1.2 - пустая строка)
								CreateStr("[M]", in_data);
								CreateStr("", in_data);     // #3 (tag 1197)
								CreateStr((int)0, in_data); // #4 (tag 1222)
								CreateStr("", in_data); // #5 (tag 1226) ИНН поставщика 
								CreateStr("", in_data); // #6 (tag 1171) Телефон(ы) поставщика
								CreateStr("", in_data); // #7 (tag 1225) Наименование поставщика
								CreateStr("", in_data); // #8 (tag 1005) Адрес оператора перевода (для банк.пл.агента/субагента, иначе пустой) 
								CreateStr("", in_data); // #9 (tag 1016) ИНН оператора перевода (для банк.пл.агента/субагента, иначе пустой)
								CreateStr("", in_data); // #10 (tag 1026) Наименование оператора перевода (для банк.пл.агента/субагента, иначе пустой) 
								CreateStr("", in_data); // #11 (tag 1075) Телефон(ы) оператора перевода (для банк.пл.агента/субагента, иначе пустой) 
								CreateStr("", in_data); // #12 (tag 1044) Операция платежного агента (для банк.пл.агента/субагента, иначе пустой) 
								CreateStr("", in_data); // #13 (tag 1073) Телефон(ы) платежного агента (для пл.агента/субагента, иначе пустой) 
								CreateStr("", in_data); // #14 (tag 1074) Телефон(ы) оператора по приему платежей (для пл.агента/субагента, иначе пустой) 
								CreateStr("030"/*GTCHZNPT_DRAFTBEER*/, in_data); // #15 (tag 1262) Идентификатор ФОИВ. Значение определяется ФНС РФ. Параметр используется только при регистрации ККТ в режиме ФФД 1.2.
								// @v11.9.3 str.Z().Cat(checkdate(Check.Timestamp.d) ? Check.Timestamp.d : getcurdate_(), DATF_DMY|DATF_NODIV|DATF_CENTURY); // @v11.2.3 // @v11.2.7
								str.Z().Cat("26032022"); // @v11.9.3
								//str.Z().Cat(checkdate(Check.Timestamp.d) ? Check.Timestamp.d : getcurdate_(), DATF_DMY | DATF_NODIV | DATF_CENTURY);
								//
								CreateStr(str, in_data); // #16 (tag 1263) Дата документа основания. Допускается дата после 1999 года. 
									// Должен содержать сведения об НПА отраслевого регулирования. Параметр используется только при регистрации ККТ в режиме ФФД 1.2.
								/* @v11.9.3 if(Check.CheckNum > 0)
									str.Z().Cat(Check.CheckNum);
								else
									str = "83d185d1";
								*/
								str.Z().Cat("477"); // @v11.9.3
								CreateStr(str, in_data); // #17 (tag 1264) Номер документа основания. Должен содержать сведения об НПА отраслевого регулирования. 
									// Параметр используется только при регистрации ККТ в режиме ФФД 1.2.
								// @v11.2.3 {
								{
									// @v12.0.4 str.Z();
									str.Z().Cat("mode=horeca"); // @v12.0.4
									CreateStr(str, in_data); // #18 (tag 1265) Значение отраслевого реквизита. Значение определяется отраслевым НПА. 
										// Параметр используется только при регистрации ККТ в режиме ФФД 1.2.
								}
								THROW(ExecCmd("24", in_data, out_data, r_error));
								// } @v11.2.3 
							}
						}
					}
				}
				else if(Check.ChZnGTIN.NotEmpty() && (Check.ChZnSerial.NotEmpty() || Check.ChZnPartN.NotEmpty())) {
					in_data.Z();
					// @todo Заменить числовые константы на GTCHZNPT_XXX
					switch(Check.ChZnProdType) {
						case 1: product_type_bytes = 0x5246; break; // GTCHZNPT_FUR 
						case 2: product_type_bytes = 0x444D; break; // GTCHZNPT_TOBACCO
						case 11: product_type_bytes = 0x444D; break; // @v11.9.0 GTCHZNPT_ALTTOBACCO
						case 3: product_type_bytes = 0x444D; break; // GTCHZNPT_SHOE
						case 4: product_type_bytes = 0x444D; break; // GTCHZNPT_MEDICINE
						case 5: product_type_bytes = 0x444D; break; // GTCHZNPT_CARTIRE
						case 12: product_type_bytes = 0x444D; break; // @v11.9.4 GTCHZNPT_DRAFTBEER
						case 1012: product_type_bytes = 0x444D; break; // @v11.9.4 GTCHZNPT_DRAFTBEER_AWR
						case 14: product_type_bytes = 0x444D; break; // @v12.0.3 GTCHZNPT_BEER
						default: product_type_bytes = 0x444D; break; // @v11.0.5
					}
					const char * p_serial = Check.ChZnSerial.NotEmpty() ? Check.ChZnSerial.cptr() : Check.ChZnPartN.cptr();
					int    rl = STokenRecognizer::EncodeChZn1162(product_type_bytes, Check.ChZnGTIN, p_serial, chzn_1162_bytes, sizeof(chzn_1162_bytes));
					if(rl > 0) {
						PreprocessChZnCodeResult pczcr;
						str.Z();
						// @v11.1.10 {
						if(OfdVer.IsGe(1, 2, 0)) {
							if(Check.ChZnPpStatus > 0) {
								{
									// --> 79/1
									// --> 79/2
									//
									// --> 79/15
								}
								//PreprocessChZnMark(Check.ChZnCode, 1, 0, &pczcr);
								//int PiritEquip::PreprocessChZnMark(const char * pMarkCode, uint qtty, uint flags, PreprocessChZnCodeResult * pResult)
								// 79
								in_data.Z();
								CreateStr(15, in_data);
								CreateChZnCode(Check.ChZnCode, in_data); // (Строка)[0..128] Код маркировки
								CreateStr(Check.ChZnPpStatus, in_data); // (Целое число) Присвоенный статус товара (ofdtag-2110)
								CreateStr(0L, in_data); // (Целое число) Режим обработки кода маркировки (ofdtag-2102) = 0
								CreateStr(Check.ChZnPpResult, in_data); // (Целое число) Результат проведенной проверки КМ (ofdtag-2106)
								{
									// @v11.9.5 CreateStr(0L, in_data); // (Целое число) Мера количества [единица измерения то есть; 0 - штуки] (ofdtag-2108)
									// @v11.9.5 {
									int chzn_uom_id = 0;
									switch(Check.UomId) {
										case SUOM_LITER: chzn_uom_id = 41; break;
										case SUOM_KILOGRAM: chzn_uom_id = 11; break;
									}
									// } @v11.9.5 
									CreateStr(chzn_uom_id, in_data); // uom
								}
								THROW(ExecCmd("79", in_data, out_data, r_error)); // query=15
								//set_chzn_mark = false;
								//
								{
									/*

										(Строка[1..256]) Код товара (Тег 1163)
										(Строка[1..64]) Дополнительный реквизит предмета расчёта (Тег 1191)
										(Строка) Зарезервировано
										(Целое число) Признак агента по предмету расчёта (Тег 1222)
										(Строка)[0..12] ИНН поставщика (Тег 1226)
										(Строка)[0..40] Телефон(ы) поставщика (Тег 1171)
										(Строка)[0..256] Наименование поставщика (Тег 1225)
										(Строка)[0..256] Адрес оператора перевода (для банк.пл.агента/субагента, иначе пустой) (Тег 1005)
										(Строка)[0..12] ИНН оператора перевода (для банк.пл.агента/субагента, иначе пустой) (Тег 1016)
										(Строка)[0..64] Наименование оператора перевода (для банк.пл.агента/субагента, иначе пустой) (Тег 1026)
										(Строка)[0..40] Телефон(ы) оператора перевода (для банк.пл.агента/субагента, иначе пустой) (Тег 1075)
										(Строка)[0..24] Операция платежного агента (для банк.пл.агента/субагента, иначе пустой) (Тег 1044)
										(Строка)[0..60] Телефон(ы) платежного агента (для пл.агента/субагента, иначе пустой) (Тег 1073)
										(Строка)[0..60] Телефон(ы) оператора по приему платежей (для пл.агента/субагента, иначе пустой) (Тег 1074)
										(Строка)[0..3] (fiovId = 030) Идентификатор ФОИВ (тег 1262). Значение определяется ФНС РФ. Параметр используется только при регистрации ККТ в режиме ФФД 1.2.
										(Дата8) (documentDate = 26.03.2022) Дата документа основания (тег 1263) в формате ddmmyyyy. Должен содержать сведения об НПА отраслевого регулирования. Параметр используется только при регистрации ККТ в режиме ФФД 1.2.
										(Строка)[0..32] (documentNumber = 477) Номер документа основания (тег 1264). Должен содержать сведения об НПА отраслевого регулирования. Параметр используется только при регистрации ККТ в режиме ФФД 1.2.
										(Строка)[0..256] Значение отраслевого реквизита (тег 1265). Значение определяется отраслевым НПА. Параметр используется только при регистрации ККТ в режиме ФФД 1.2. При наличии нескольких реквизитов (теги 1262,1263,1264 и 1265) они должны разделяться символом "|" (0x7C)

									*/
									in_data.Z(); // @v11.2.3 @fix
									CreateStr(str.Z(), in_data); // #1 (tag 1162) Код товарной номенклатуры (для офд 1.2 - пустая строка)
									if(Check.ChZnProdType == 4) { // #2 (tag 1191) GTCHZNPT_MEDICINE
										str = "mdlp";
										if(!fractional_medcine_to_1291) { // @v12.3.7
											if(Check.Qtty > 0.0 && Check.Qtty < 1.0 && Check.UomFragm > 0) {
												double ip = 0.0;
												double nmrtr = 0.0;
												double dnmntr = 0.0;
												if(fsplitintofractions(Check.Qtty, Check.UomFragm, 1E-5, &ip, &nmrtr, &dnmntr))
													str.Cat(R0i(nmrtr)).Slash().Cat(R0i(dnmntr)).CatChar('&');
											}
										}
										CreateStr(str, in_data);
									}
									else
										CreateStr("[M]", in_data);
									CreateStr("", in_data);     // #3 (tag 1197)
									CreateStr((int)0, in_data); // #4 (tag 1222)
									CreateStr("", in_data); // #5 (tag 1226) ИНН поставщика 
									CreateStr("", in_data); // #6 (tag 1171) Телефон(ы) поставщика
									CreateStr("", in_data); // #7 (tag 1225) Наименование поставщика
									CreateStr("", in_data); // #8 (tag 1005) Адрес оператора перевода (для банк.пл.агента/субагента, иначе пустой) 
									CreateStr("", in_data); // #9 (tag 1016) ИНН оператора перевода (для банк.пл.агента/субагента, иначе пустой)
									CreateStr("", in_data); // #10 (tag 1026) Наименование оператора перевода (для банк.пл.агента/субагента, иначе пустой) 
									CreateStr("", in_data); // #11 (tag 1075) Телефон(ы) оператора перевода (для банк.пл.агента/субагента, иначе пустой) 
									CreateStr("", in_data); // #12 (tag 1044) Операция платежного агента (для банк.пл.агента/субагента, иначе пустой) 
									CreateStr("", in_data); // #13 (tag 1073) Телефон(ы) платежного агента (для пл.агента/субагента, иначе пустой) 
									CreateStr("", in_data); // #14 (tag 1074) Телефон(ы) оператора по приему платежей (для пл.агента/субагента, иначе пустой) 
									{
										if(Check.ChZnProdType == 4)
											str = "020";
										else if(oneof2(Check.ChZnProdType, 12, 1012)) // @v11.9.3 GTCHZNPT_DRAFTBEER // @v12.0.5 1012 (GTCHZNPT_DRAFTBEER_AWR)
											str = "030";
										else
											str = "030"; // @v12.1.3 .Z()-->"030"
										CreateStr(str, in_data); // #15 (tag 1262) Идентификатор ФОИВ. Значение определяется ФНС РФ. Параметр используется только при регистрации ККТ в режиме ФФД 1.2.
									}
									// @v11.9.3 str.Z().Cat(checkdate(Check.Timestamp.d) ? Check.Timestamp.d : getcurdate_(), DATF_DMY|DATF_NODIV|DATF_CENTURY); // @v11.2.3 // @v11.2.7
									str.Z().Cat("26032022"); // @v11.9.3
									CreateStr(str, in_data); // #16 (tag 1263) Дата документа основания. Допускается дата после 1999 года. 
										// Должен содержать сведения об НПА отраслевого регулирования. Параметр используется только при регистрации ККТ в режиме ФФД 1.2.
									/* @v11.9.3 if(Check.CheckNum > 0)
										str.Z().Cat(Check.CheckNum);
									else
										str = "83d185d1";
									*/
									str.Z().Cat("477"); // @v11.9.3
									CreateStr(str, in_data); // #17 (tag 1264) Номер документа основания. Должен содержать сведения об НПА отраслевого регулирования. 
										// Параметр используется только при регистрации ККТ в режиме ФФД 1.2.
									// @v11.2.3 {
									{
										str.Z();
										// 1265 "industryDetails": "tm=mdlp&sid=12121212121212&"
										// tm=mdlp&ps=45102&dn=АБV492&&781&dd=181110&sid=00752852194630&
										//
										// ps - серия рецепта;
										// dn - номер рецепта;
										// dd - дата рецепта.
										//
										if(Check.ChZnProdType == 4) {
											str.CatEq("tm", "mdlp");
											if(Check.ChZnSid.NotEmpty())
												str.CatChar('&').CatEq("sid", Check.ChZnSid);
											// @v11.8.0 {
											if(Check.PrescrNumber.NotEmpty()) {
												if(Check.PrescrSerial.NotEmpty())
													str.CatChar('&').CatEq("ps", Check.PrescrSerial);
												str.CatChar('&').CatEq("dn", Check.PrescrNumber);
												if(checkdate(Check.PrescrDate))
													str.CatChar('&').CatEq("dd", Check.PrescrDate, DATF_DMY|DATF_NODIV);
											}
											// } @v11.8.0 
										}
										// @v12.1.1 {
										if(!Check.ChZnPm_ReqId.IsZero() && Check.ChZnPm_ReqTimestamp) {
											if(str.NotEmpty() && str.Last() != '&')
												str.CatChar('&');
											str.CatEq("UUID", Check.ChZnPm_ReqId, S_GUID::fmtIDL|S_GUID::fmtLower);
											str.CatChar('&');
											str.CatEq("Time", Check.ChZnPm_ReqTimestamp);
										}
										// } @v12.1.1
										if(str.NotEmpty())
											str.CatChar('&');
										CreateStr(str, in_data); // #18 (tag 1265) Значение отраслевого реквизита. Значение определяется отраслевым НПА. 
											// Параметр используется только при регистрации ККТ в режиме ФФД 1.2.
									}
									THROW(ExecCmd("24", in_data, out_data, r_error));
									// } @v11.2.3 
								}
							}
						}
						else {
						// } @v11.1.10
							in_data.Z(); // @v11.2.3 @fix
							for(int si = 0; si < rl; si++) {
								if(si < 8)
									str.CatChar('$').CatHex(chzn_1162_bytes[si]);
								else
									str.CatChar(chzn_1162_bytes[si]);
							}
							CreateStr(str, in_data); // Код товарной номенклатуры
							if(Check.ChZnProdType == 4) { // GTCHZNPT_MEDICINE
								// @v11.2.6 {
								str = "mdlp";
								if(Check.Qtty > 0.0 && Check.Qtty < 1.0 && Check.UomFragm > 0) {
									// Для ОФД пре-1.2 дробное количество передает так не зависимо от fractional_medcine_to_1291
									double ip = 0.0;
									double nmrtr = 0.0;
									double dnmntr = 0.0;
									if(fsplitintofractions(Check.Qtty, Check.UomFragm, 1E-5, &ip, &nmrtr, &dnmntr))
										str.Cat(R0i(nmrtr)).Slash().Cat(R0i(dnmntr)).CatChar('&');
								}
								CreateStr(str, in_data);
								// } @v11.2.6 
								//CreateStr("mdlp", in_data);
							}
							else
								CreateStr("[M]", in_data);
							THROW(ExecCmd("24", in_data, out_data, r_error)); // @v11.2.3
						}
					}
				}
			}
			in_data.Z();
			(str = Check.Text).Trim(220); // [0..224]
			CreateStr(str, in_data); // Название товара
			(str = Check.Code).Trim(13); // [0..18]
			CreateStr(str, in_data); // Артикул или штрихкод
			{
				bool   qtty_done = false; // @v12.3.7
				double qtty = 0.0;
				double price = 0.0;
				if(Check.ChZnProdType == 1012 && Check.PhQtty > 0.0 && Check.UomId == SUOM_LITER) { // @v12.0.11 GTCHZNPT_DRAFTBEER_AWR
					qtty = Check.PhQtty;
					price = R2((Check.Qtty * Check.Price) / Check.PhQtty);
				}
				else {
					qtty = Check.Qtty;
					price = Check.Price;
				}
				// @v12.3.7 {
				if(Check.ChZnProdType == 4) { // #2 (tag 1191) GTCHZNPT_MEDICINE
					if(fractional_medcine_to_1291) { 
						if(Check.Qtty > 0.0 && Check.Qtty < 1.0 && Check.UomFragm > 0) {
							double ip = 0.0;
							double nmrtr = 0.0;
							double dnmntr = 0.0;
							if(fsplitintofractions(Check.Qtty, Check.UomFragm, 1E-5, &ip, &nmrtr, &dnmntr)) {
								str.Z().Cat(R0i(nmrtr)).Slash().Cat(R0i(dnmntr));
								CreateStr(str, in_data);
								qtty_done = true;
							}
						}
					}
				}
				// } @v12.3.7 
				if(!qtty_done) {
					CreateStr(qtty, in_data);
					qtty_done = true;
				}
				CreateStr(price, in_data);
			}
			CreateStr((int)Check.Tax, in_data); // Номер налоговой ставки
			CreateStr((int)0, in_data);    // Номер товарной позиции
			CreateStr(Check.Department, in_data); // Номер секции
			CreateStr((int)0, in_data);    // пустой параметр (integer)
			CreateStr("", in_data);        // пустой параметр (string)
			CreateStr(0.0, in_data);       // скидка (real)
			CreateStr(Check.Ptt, in_data); // признак способа расчета (integer)
			CreateStr(Check.Stt, in_data); // @erikO v10.4.12 Признак предмета расчета(integer)
			{
				const int do_check_ret = 1; // BIN(Check.Price == 0.0); // @v11.2.3 =1
				Check.Z();
				THROW(ExecCmd("42", in_data, out_data, r_error)); // @v11.2.3
				Check.Z();
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
					text_attr = 0x01; // @v11.3.6 
					if(oneof2(Check.FontSize, 1, 2))
						text_attr = 0x01; // Шрифт 13х24, 44 символа в строке
					else if(Check.FontSize == 3)
						text_attr = 0;
					else if(Check.FontSize == 4)
						text_attr = 0x10; // Шрифт 8х14, 56 символов в строке
					else if(Check.FontSize > 4)
						text_attr = (0x20|0x10);
					if((Check.Text.Len() + 1) > 54)
						Check.Text.Trim(52);
					CreateStr(Check.Text.ToOem(), in_data);
					if(text_attr != 0)
						CreateStr(text_attr, in_data);
					Check.Z();
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
					text_attr = 0x01; // @v11.3.6 0-->0x01
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
	for(uint i = 0; params.get(&i, buf);) {
		in_data.Z();
		out_data.Z();
		if(buf.IsEqiAscii("AMOUNT")) {
			CreateStr(1, in_data);
			THROW(ExecCmd("03", in_data, out_data, r_error));
			{
				StringSet dataset(CHR_FS, out_data);
				if(LogFileName.NotEmpty()) {
					(str = "AMOUNT (Cmd=03 Arg=1)").CatDiv(':', 2).Cat(out_data);
					SLS.LogMessage(LogFileName, str, 8192);
				}
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
				StringSet dataset(CHR_FS, out_data);
				if(LogFileName.NotEmpty()) {
					(str = "CHECKNUM (Cmd=03 Arg=2)").CatDiv(':', 2).Cat(out_data);
					SLS.LogMessage(LogFileName, str, 8192);
				}
				uint   k = 0;
				// Возвращаемые значения (в порядке следования в буфере ответа) {
				int    cc_type = 0; // Тип чека
				char   current_op_counter[64]; // Текущий операционный счетчик
				int    cc_number = 0; // Номер чека
				int    doc_number = 0; // Номер документа
				SString potential_cc_number1; // @v11.3.3
				SString potential_cc_number2; // @v11.3.3
				double cc_amount = 0.0; // Сумма чека
				double cc_discount = 0.0; // Сумма скидки по чеку
				double cc_markup = 0.0; // Сумма неценки по чеку
				char   _kpk[64]; // Строка КПК (?)
				// }
				for(uint count = 0; dataset.get(&k, str)/* && count < 3*/; count++) { // Еще номер запроса, беру номер /*документа*/ чека
					switch(count) {
						case 0: cc_type = str.ToLong(); break;
						case 1: STRNSCPY(current_op_counter, str); break;
						case 2: 
							potential_cc_number1 = str; // @v11.3.3
							cc_number = str.ToLong(); 
							break;
						case 3: doc_number = str.ToLong(); break;
						case 4: 
							potential_cc_number2 = str; // @v11.3.3
							cc_amount = str.ToReal(); 
							break;
						case 5: cc_discount = str.ToReal(); break;
						case 6: cc_markup = str.ToReal(); break;
						case 7: STRNSCPY(_kpk, str); break;
					}
				}
				// @v11.3.3 {
				// Какая-то несуразица: в протоколе описано одно, в реальности в некоторых случаях - другое
				if(potential_cc_number1.IsDec() && !potential_cc_number2.IsDec())
					cc_number = potential_cc_number1.ToLong();
				else if(!potential_cc_number1.IsDec() && potential_cc_number2.IsDec())
					cc_number = potential_cc_number2.ToLong();
				// } @v11.3.3 
				if(cc_number == 18 || cc_number < 0) // Костыль: Некоторые аппараты всегда возвращают 18-й номер чека. Трактуем это как 0.
					cc_number = 0;
				s_output.CatEq("CHECKNUM", cc_number).Semicol();
			}
		}
		else if(buf.IsEqiAscii("CASHAMOUNT")) {
			int    succs = 0;
			for(uint t = 0; !succs && t < 4; t++) {
				CreateStr(7, in_data);
				THROW(ExecCmd("02", in_data, out_data, r_error));
				{
					StringSet dataset(CHR_FS, out_data);
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

int PiritEquip::PutOfdReq(int reqCode, int textAttr, const char * pReqDescr, const char * pReqValue, SString & rError) // @v12.3.3
{
	int    ok = 1;
	SString in_data;
	SString out_data;
	CreateStr(reqCode, in_data);
	CreateStr(textAttr, in_data);
	CreateStr(pReqDescr, in_data);
	CreateStr(pReqValue, in_data);
	ok = ExecCmd("57", in_data, out_data, rError);
	return ok;
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
		SString & r_buf = SLS.AcquireRvlStr();
		do {
			r_buf.CatChar(c);
			c = CommPort.GetChr();
		} while(c != CHR_ETX && c != 0);
		{
			int    crc = 0;
			char   str_crc2[2];
			size_t p = 0;
			uint   i;
			SString & r_str_crc1 = SLS.AcquireRvlStr();
			r_buf.CatChar(c); // Добавили байт конца пакета
			c = CommPort.GetChr(); // Получили 1-й байт контрольной суммы
			r_buf.CatChar(c);
			c = CommPort.GetChr(); // Получили 2-й байт контрольной суммы
			r_buf.CatChar(c);
			THROW(r_buf.C(0) == CHR_STX);
			// Выделяем байты с информацией об ошибке
			r_buf.Sub(4, 2, rError);
			//
			// Считываем данные
			//
			for(i = 6; r_buf.C(i) != CHR_ETX; i++)
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
	SString & r_pack = SLS.AcquireRvlStr();
	size_t p = 0;
	// Формируем пакет (без байтов начала и конца пакета)
	// Пароль связи (4 байта)
	r_pack.Cat(Cfg.ConnPass);
	r_pack.CatChar(id_pack); // ИД пакета
	r_pack.Cat(pCommand); // Код команды (2 байта)
	r_pack.Cat(pData); // Данные
	// Считаем контрольную сумму пакета
	for(p = 0; p < r_pack.Len(); p++) {// STX в контрольную сумму не входит
		crc ^= static_cast<uchar>(r_pack.C(p));
	}
	crc ^= static_cast<uchar>(CHR_ETX); // Учитываем в контрольной сумме байт конца пакета
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
		THROW(CommPort.PutChr(CHR_STX));
		debug_packet[debug_packet_pos++] = CHR_STX;
		for(p = 0; p < r_pack.Len(); p++) {
			const char v = r_pack.C(p);
			THROW(CommPort.PutChr(v));
			debug_packet[debug_packet_pos++] = v;
		}
		THROW(CommPort.PutChr(CHR_ETX));
		debug_packet[debug_packet_pos++] = CHR_ETX;
		THROW(CommPort.PutChr(buf[0]));
		debug_packet[debug_packet_pos++] = buf[0];
		THROW(CommPort.PutChr(buf[1]));
		debug_packet[debug_packet_pos++] = buf[1];
	}
	else {
		THROW(CommPort.PutChr(CHR_STX));
		for(p = 0; p < r_pack.Len(); p++)
			THROW(CommPort.PutChr(r_pack.C(p)));
		THROW(CommPort.PutChr(CHR_ETX));
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
	SString in_data;
	SString out_data;
	SString r_error;
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
	LastStatus = status;
	if(status == CHECKCLOSED) // Этот статус не надо передавать во внешнюю программу
		status = 0;
	CATCHZOK
	rStatus.CatEq("STATUS", status);
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
	CreateStr(static_cast<int>(size) + 1, in_data);
	{
		OpLogBlock __oplb(LogFileName, "15", 0);
		THROWERR(PutData("15", in_data), PIRIT_NOTSENT);
		do {
			r = CommPort.GetChr();
		} while(r && r != CHR_ACK);
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
	int    ok = 1;
	int    flag = 0;
	SString out_data;
	SString r_error;
	THROW(ReadConfigTab(1, 0, out_data, r_error));
	flag = out_data.ToLong();
	SETFLAG(flag, 0x04, print);
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
