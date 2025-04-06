// SHTRIHFR.CPP
// Copyright (c) V.Nasonov 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// Интерфейс (синхронный) с ККМ Штрих-ФР
//
#include <pp.h>
#pragma hdrstop
//
// BillTaxArray
//
struct BillTaxEntry { // @flat
	BillTaxEntry() : VAT(0), SalesTax(0), Amount(0.0)
	{
	}
	long   VAT;      // prec 0.01
	long   SalesTax; // prec 0.01
	double Amount;
};

class BillTaxArray : public SVector {
public:
	BillTaxArray() : SVector(sizeof(BillTaxEntry))
	{
	}
	int    Search(long VAT, long salesTax, uint * p = 0);
	int    Insert(const BillTaxEntry * pEntry, uint * p = 0);
	int    Add(BillTaxEntry * pEntry);
	BillTaxEntry & FASTCALL at(uint p);
};

IMPL_CMPFUNC(BillTaxEnKey, i1, i2) 
	{ RET_CMPCASCADE2(static_cast<const BillTaxEntry *>(i1), static_cast<const BillTaxEntry *>(i2), VAT, SalesTax); }

int BillTaxArray::Search(long VAT, long salesTax, uint * p)
{
	BillTaxEntry bte;
	bte.VAT      = VAT;
	bte.SalesTax = salesTax;
	return bsearch(&bte, p, PTR_CMPFUNC(BillTaxEnKey));
}

int BillTaxArray::Insert(const BillTaxEntry * pEntry, uint * p)
{
	return ordInsert(pEntry, p, PTR_CMPFUNC(BillTaxEnKey)) ? 1 : PPSetErrorSLib();
}

BillTaxEntry & FASTCALL BillTaxArray::at(uint p)
{
	return *static_cast<BillTaxEntry *>(SVector::at(p));
}

int BillTaxArray::Add(BillTaxEntry * e)
{
	int    ok = 1;
	uint   p;
	if(Search(e->VAT, e->SalesTax, &p)) {
		BillTaxEntry & r_bte = at(p);
		r_bte.Amount += e->Amount;
	}
	else {
		THROW(Insert(e));
	}
	CATCHZOK
	return ok;
}
//
//   Штрих-ФР
//
typedef ComDispInterface  FR_INTRF;

#define COM_PORT                   2
#define MAX_TIMEOUT              255   // Для Штрих-ФР max таймаут 255 мсек
#define CASH_AMOUNT_REG          241   // Денежный регистр, содержащий наличность в кассе за смену
#define CHECK_NUMBER_REG         152   // Операционный регистр, содержащий текущий номер чека
#define DEF_STRLEN                36   // Для Штрих-ФР длина строки
#define DEF_FONT_NUMBER            7   // Кол-во используемых шрифтов
#define DEF_LINEFEED_NUMBER        2   // Кол-во пропускаемых строк
#define DEF_DRAWER_NUMBER          0   // Номер денежного ящика
//
//   Таблица режимов кассы
//
#define FRCASHMODE_TBL             1   // Номер таблицы
#define FRCASHMODE_ROW             1   // Номер ряда для признаков
#define FRCASHMODE_AUTOCASHNULL    2   // Номер поля с признаком автоматического обнуления наличности
#define FRCASHMODE_OPENDRAWER      7   // Номер поля с признаком открывания денежного ящика
#define FRCASHMODE_CUTTING         8   // Номер поля с признаком отрезки чека
#define FRCASHMODE_USEWGHTSENSOR  15   // Номер поля с признаком использования весового датчика
#define FRCASHMODE_TAXALLCHK      17   // Номер поля с признаком начисления налогов на весь чек
#define FRCASHMODE_AUTOTIMING     18   // Номер поля с признаком автоматического перевода времени
#define FRCASHMODE_TAXPRINT       19   // Номер поля с признаком печати налогов
#define FRCASHMODE_SAVESTRING     22   // Номер поля с признаком сохранения строк в буфере
//
//   Таблица режимов касс Штрих-Мини-ФР-К И Штрих-Комбо-ФР-К (поля, отличные от Штрих-ФР)
//
#define COMBOCASHMODE_OPENDRAWER   6   // Номер поля с признаком открывания денежного ящика
#define COMBOCASHMODE_CUTTING      7   // Номер поля с признаком отрезки чека
#define COMBOCASHMODE_USEWGHTSENSOR 14 // Номер поля с признаком использования весового датчика
#define COMBOCASHMODE_TAXALLCHK   15   // Номер поля с признаком начисления налогов на весь чек
#define COMBOCASHMODE_AUTOTIMING  16   // Номер поля с признаком автоматического перевода времени
#define COMBOCASHMODE_TAXPRINT    17   // Номер поля с признаком печати налогов
#define COMBOCASHMODE_SAVESTRING  20   // Номер поля с признаком сохранения строк в буфере
//
//   Таблица кассиров и администраторов
//
#define FRCASHIER_TBL              2   // Номер таблицы
#define FRCASHIER_ROW              1   // Номер ряда для 1-го кассира
#define FRCASHIER_ADMINROW        30   // Номер ряда для администратора
#define FRCASHIER_FIELD_PSSW       1   // Номер поля с паролем кассира/администратора
#define FRCASHIER_FIELD_NAME       2   // Номер поля с именем кассира/администратора
//
//   Таблица перевода времени
//
#define FRTIMESWITCH_TBL           3   // Номер таблицы
#define FRTIMESWITCH_ALLOW         1   // Номер поля разрешения перевода
#define FRTIMESWITCH_DAY           2   // Номер поля числа
#define FRTIMESWITCH_MONTH         3   // Номер поля месяца
#define FRTIMESWITCH_SEASON        4   // Номер поля времени года
#define FRTIMESWITCH_MAXROW       20   // Max номер ряда
//
//   Таблица типов оплаты
//
#define FRPAYMTYPE_TBL             5   // Номер таблицы
#define FRPAYMTYPE_NAME            1   // Номер поля с наименованием оплаты
//
//   Таблица налоговых ставок
//
#define FRTAX_TBL                  6   // Номер таблицы
#define FRTAX_SALESTAX_ROW         1   // Номер ряда для налога с продаж
#define FRTAX_FIELD_TAXAMT         1   // Номер поля с величиной налога
#define FRTAX_FIELD_TAXNAME        2   // Номер поля с названием налога
//
//   Таблица формата чека
//
#define FRFORMAT_TBL               9   // Номер таблицы
#define FRFORMAT_ROW               1   // Номер ряда
#define FRFORMAT_FIELD_SIZE        3   // Номер поля с размером строки
//
//   Таблица настроек стандартного фискального подкладного документа
//
#define FRSLIPCONFIG_TBL          12   // Номер таблицы
#define FRSLIPCONFIG_ROW           1   // Номер ряда
#define FRSLIPCONFIG_TITLE_STRNUM  6   // Номер поля с номером строки заголовка
//
//   Таблица стандартных операций на подкладном документе
//
#define FRSLIPOPER_TBL            13   // Номер таблицы
#define FRSLIPOPER_ROW             1   // Номер ряда
#define FRSLIPOPER_FONTNUM         7   // Номер поля с номером шрифта строки
#define FRSLIPOPER_FIELD_SIZE     13   // Номер поля с размером строки
//
//   Режимы печати чеков
//
#define PRNMODE_NO_PRINT           0   // Нет печати
#define PRNMODE_NO_PRINT_NO_PAPER  1   // Нет печати, нет бумаги
#define PRNMODE_PRINT_NO_PAPER     2   // Печать, кончилась бумага
#define PRNMODE_AFTER_NO_PAPER     3   // Ожидание команды печати после режима 2
#define PRNMODE_PRINT              4   // Режим печати
#define PRNMODE_PRINT_LONG_REPORT  5   // Режим печати длинного отчета
//
//   Коды возврата при операциях печати
//
#define RESCODE_NO_CONNECTION     -1   // Код возврата "Нет связи: нет устройства"
#define RESCODE_NO_ERROR      0x0000
//#define RESCODE_OPEN_CHECK  0x004A   // 074 Код возврата "Открыт чек - операция невозможна"
//#define RESCODE_PRINT       0x0050   // 080 Код возврата "Идет печать предыдущей команды"
//#define RESCODE_SUBMODE_OFF 0x0072   // 114 Код возврата "Команда не поддерживается в данном подрежиме"
#define RESCODE_DVCCMDUNSUPP  0x0037   // 055 Код возврата "Команда не поддерживается в данной реализации ФР"
#define RESCODE_PAYM_LESS_SUM 0x0045   // 069 Код возврата "Сумма всех видов оплат меньше суммы чека"
#define RESCODE_MODE_OFF      0x0073   // 115 Код возврата "Команда не поддерживается в данном режиме"
#define RESCODE_SLIP_IS_EMPTY 0x00C5   // 197 Код возврата "Буфер подкладного документа пуст"
#define RESCODE_INVEKLZSTATE  163      // 163 Некорректное состояние ЭКЛЗ
//
//   Режимы ШТРИХ-ФР
//
#define FRMODE_OPEN_SESS           2   // Открытая смена
#define FRMODE_OPEN_SESS_LONG      3   // Открытый смена более 24 часов
#define FRMODE_CLOSE_SESS          4   // Закрытая смена
#define FRMODE_OPEN_CHECK          8   // Открытый чек
#define FRMODE_FULL_REPORT        11   // Печать полного фискального отчета
#define FRMODE_LONG_EKLZ_REPORT   12   // Печать длительного отчета ЭКЛЗ
#define FRMODE_PRINT_SLIPDOC      14   // Печать подкладного документа
//
//   Режимы печати подкладного документа
//
#define SLIPMODE_BEFORE_PRINT      0   // Ожидание загрузки подкладного документа
#define SLIPMODE_AFTER_PRINT       6   // Ожидание извлечения подкладного документа после печати
//
//   Модели устройств
//
#define SHTRIH_FRF                 0   // ШТРИХ-ФР-Ф
#define ELVES_FRF                  2   // ЭЛВЕС-МИНИ-ФР-Ф
#define FELIX_RF                   3   // ФЕЛИКС-Р Ф
#define SHTRIH_FRK                 4   // ШТРИХ-ФР-К
#define SHTRIH_950K                5   // ШТРИХ-950К
#define ELVES_FRK                  6   // ЭЛВЕС-ФР-К
#define SHTRIH_MINI_FRK            7   // ШТРИХ-МИНИ-ФР-К
#define SHTRIH_COMBO_FRK           9   // ШТРИХ-КОМБО-ФР-К
#define SHTRIH_POS_F              10   // ШТРИХ-POS-Ф
#define SHTRIH_950K_V2            11   // ШТРИХ-950К, версия 02
#define SHTRIH_COMBO_FRK_V2       12   // ШТРИХ-КОМБО-ФР-К, версия 02
#define SHTRIH_MINI_FRK_V2        14   // ШТРИХ-МИНИ-ФР-К, версия 02
#define SHTRIH_LIGHT_FRK         252   // ШТРИХ-LIGHT-ФР-К
//
//
//
class SCS_SHTRIHFRF : public PPSyncCashSession {
	const  bool IsDebugMode;
	uint8  Reserve[3]; // @alignment
public:
	SCS_SHTRIHFRF(PPID n, char * pName, char * pPort);
	~SCS_SHTRIHFRF();
	virtual int PrintCheck(CCheckPacket * pPack, uint flags);
	virtual int PrintCheckCopy(const CCheckPacket * pPack, const char * pFormatName, uint flags);
	virtual int PrintSlipDoc(const CCheckPacket * pPack, const char * pFormatName, uint flags);
	virtual int GetSummator(double * val);
	virtual int OpenSession_(PPID sessID);
	virtual int CloseSession(PPID sessID);
	virtual int PrintXReport(const CSessInfo *);
	virtual int PrintZReportCopy(const CSessInfo *);
	virtual int PrintIncasso(double sum, int isIncome);
	virtual int GetPrintErrCode();
	virtual int OpenBox();
	virtual int CheckForSessionOver();
	virtual int PrintBnkTermReport(const char * pZCheck);
	virtual int PreprocessChZnCode(int op, const char * pCode, double qtty, int uomId, uint uomFragm, CCheckPacket::PreprocessChZnCodeResult & rResult); // @v11.6.6
	virtual int Diagnostics(StringSet * pSs); // @v12.0.3
private:
	FR_INTRF * InitDriver();
	int  ConnectFR();
	int  SetupTables();
	int  AnnulateCheck();
	int  CheckForCash(double sum);
	int  CheckForEKLZOrFMOverflow();
	int  PrintReport(int withCleaning);
	int	 PrintDiscountInfo(const CCheckPacket * pPack, uint flags);
	int  GetCheckInfo(const PPBillPacket * pPack, BillTaxArray * pAry, long * pFlags, SString &rName);
	int  InitTaxTbl(BillTaxArray * pBTaxAry, PPIDArray * pVatAry, int * pPrintTaxAction);
	bool SetFR(int id, int    iVal);
	bool SetFR(int id, long   lVal);
	bool SetFR(int id, double dVal);
	bool SetFR(int id, const char * pStrVal);
	bool GetFR(int id, int    * pBuf);
	bool GetFR(int id, long   * pBuf);
	bool GetFR(int id, double * pBuf);
	bool GetFR(int id, char   * pBuf, size_t bufLen);
	int  ExecFR(int id);
	int  ExecFR_WithoutPassword(int id);
	int  AllowPrintOper(int id);
	void SetErrorMessage();
	void WriteLogFileToWriteTblErr(int tblNum, int rowNum, int fldNum, const char * pValue);
	void LogDebug(const char * pFunc, int ifcId, const char * pArg);
	//
	// Returns:
	//   0 only. In order to initialize return code
	//
	int  LogLastError(); 
	void WriteLogFile(int id);
	void CutLongTail(char * pBuf);
	void CutLongTail(SString & rBuf);
	void SetCheckLine(char pattern, char * pBuf);
	int  LineFeed(int lineCount, int useReceiptRibbon, int useJournalRibbon);
	int  ReadStringFromTbl(int tblNum, int rowNum, int fldNum, SString & rStr);
	int  ReadValueFromTbl(int tblNum, int rowNum, int fldNum, long * pValue);
	int  WriteStringToTbl(int tblNum, int rowNum, int fldNum, const char * pStr);
	int  WriteValueToTbl(int tblNum, int rowNum, int fldNum, long value);
	int  CheckForRibbonUsing(uint ribbonParam); // ribbonParam == SlipLineParam::RegTo
	int  Cut(int withCleaning);
	int  GetBarcodePrintMethodAndStd(int innerBarcodeStd, int * pOemMethod, int * pOemStd);
	int  GetResultCode(int methID);

	enum {
		ResultCode,                       // #00
		ResultCodeDescription,            // #01
		Password,                         // #02
		Beep,                             // #03
		ComNumber,                        // #04
		BaudRate,                         // #05
		Timeout,                          // #06
		GetExchangeParam,                 // #07
		SetExchangeParam,                 // #08
		Connect,                          // #09
		Disconnect,                       // #10
		Quantity,                         // #11
		Price,                            // #12
		Summ1,                            // #13
		Summ2,                            // #14
		Summ3,                            // #15
		// Tax1..Tax4 должны идти последовательно
		Tax1,                             // #16
		Tax2,                             // #17
		Tax3,                             // #18
		Tax4,                             // #19
		StringForPrinting,                // #20
		UseReceiptRibbon,                 // #21
		UseJournalRibbon,                 // #22
		PrintString,                      // #23
		PrintWideString,                  // #24
		StringQuantity,                   // #25
		FeedDocument,                     // #26
		DocumentName,                     // #27
		DocumentNumber,                   // #28
		PrintDocumentTitle,               // #29
		CheckType,                        // #30
		OpenCheck,                        // #31
		Sale,                             // #32
		ReturnSale,                       // #33
		CloseCheck,                       // #34
		CutCheck,                         // #35
		DrawerNumber,                     // #36
		OpenDrawer,                       // #37
		TableNumber,                      // #38
		RowNumber,                        // #39
		FieldNumber,                      // #40
		GetFieldStruct,                   // #41
		ReadTable,                        // #42
		WriteTable,                       // #43
		ValueOfFieldInteger,              // #44
		ValueOfFieldString,               // #45
		RegisterNumber,                   // #46
		GetOperationReg,                  // #47
		ContentsOfOperationRegister,      // #48
		GetCashReg,                       // #49
		ContentsOfCashRegister,           // #50
		GetECRStatus,                     // #51
		ECRMode,                          // #52
		ECRModeDescription,               // #53
		ECRAdvancedMode,                  // #54
		ReceiptRibbonOpticalSensor,       // #55
		JournalRibbonOpticalSensor,       // #56
		ContinuePrint,                    // #57
		CancelCheck,                      // #58
		PrintReportWithCleaning,          // #59
		PrintReportWithoutCleaning,       // #60
		UModel,                           // #61
		UMinorProtocolVersion,            // #62
		UMajorProtocolVersion,            // #63
		GetDeviceMetrics,                 // #64
		CashIncome,                       // #65
		CashOutcome,                      // #66
		ClearSlipDocumentBuffer,          // #67
		FillSlipDocumentWithUnfiscalInfo, // #68
		StringNumber,                     // #69
		PrintSlipDocument,                // #70
		IsClearUnfiscalInfo,              // #71
		InfoType,                         // #72
		EKLZIsPresent,                    // #73
		IsEKLZOverflow,                   // #74
		FMOverflow,                       // #75
		FreeRecordInFM,                   // #76
		IsFM24HoursOver,                  // #77
		IsDrawerOpen,                     // #78
		Department,                       // #79
		ECRModeStatus,                    // #80
		JournalRibbonIsPresent,           // #81
		ReceiptRibbonIsPresent,           // #82
		OutputReceipt,                    // #83
		ReceiptOutputType,                // #84
		PrintBarcodeGraph,                //
		BarcodeType,                      //
		BarCode,                          //
		FirstLineNumber,                  //
		LineNumber,                       //
		Summ4,                            //
		Summ5,                            //
		Summ6,                            //
		Summ7,                            //
		Summ8,                            //
		Summ9,                            //
		Summ10,                           //
		Summ11,                           //
		Summ12,                           //
		Summ13,                           //
		Summ14,                           //
		Summ15,                           //
		Summ16,                           //
		CloseCheckEx,                     //
		FNOperation,                      //
		PaymentTypeSign,                  // Признак способа расчета
		PaymentItemSign,                  // Признак предмета расчета 
		FNSendItemCodeData,               //
		FNSendItemBarcode,                // @v12.0.4 OFD 1.2
		MarkingType,                      //
		GTIN,                             //
		SerialNumber,                     //
		FNBeginSTLVTag,                   //
		TagID,                            //
		TagNumber,                        //
		TagType,                          //
		TagValueStr,                      //
		FNAddTag,                         //
		FNSendSTLVTag,                    //
		// Код системы налогообложения. Битовое поле:
		// Бит 5|Бит 4|Бит 3|Бит 2|Бит 1|Бит 0|Описание
		// 0     0     0     0     0     1     Основная
		// 0     0     0     0     1     0     Упрощенная система налогообложения доход
		// 0     0     0     1     0     0     Упрощенная система налогообложения доход минус расход
		// 0     0     1     0     0     0     Единый налог на вмененный доход
		// 0     1     0     0     0     0     Единый сельскохозяйственный налог
		// 1     0     0     0     0     0     Патентная система налогообложения
		TaxType,                          // @v11.2.11
		FNCheckItemBarcode,               // @v11.6.6
		FNCheckItemBarcode2,              // @v11.6.6
		FNAcceptMarkingCode,              // @v12.0.4
		FNDeclineMarkingCode,             // @v12.0.4
		BarcodeHex,                       // @v11.6.6
		ItemStatus,                       // @v11.6.6
		CheckItemMode,                    // @v11.6.6
		TLVDataHex,                       // @v11.6.6
		CheckItemLocalResult,             // @v11.6.6
		CheckItemLocalError,              // @v11.6.6 
		MarkingType2,                     // @v11.6.6
		KMServerErrorCode,                // @v11.6.6
		KMServerCheckingStatus,           // @v11.6.6 
		DivisionalQuantity,               // @v11.6.6
		Numerator,                        // @v11.6.6 
		Denominator,                      // @v11.6.6
		FNCloseCheckEx,                   // @v11.7.2
		ServerConnect,                    // @v12.0.3
		ServerDisconnect,                 // @v12.0.3
		Connect2,                         // @v12.0.3 
		ComputerName,                     // @v12.0.3 
		ServerConnected,                  // @v12.0.3 
		ServerVersion,                    // @v12.0.3 
		OpenSession,                      // @v12.0.4
		AuthKey,                          // @v12.0.4
	};
	//
	// Descr: Методы вывода штрихкодов
	//
	enum {
		bcmPrintBarcodeGraph = SCS_SHTRIHFRF::PrintBarcodeGraph, // !
	};
	enum DeviceTypes {
		devtypeUndef,
		devtypeShtrih,
		devtypeCombo,
		devtypeMini,
		devtypeLight
	};
	enum ShtrihFlags {
		sfConnected     = 0x0001, // установлена связь с Штрих-ФР, COM-порт занят
		sfCheckOpened   = 0x0002, // чек открыт
		sfCancelled     = 0x0004, // операция печати чека прервана пользователем
		sfOldShtrih     = 0x0008, // старая версия драйвера Штрих-ФР
		sfPrintSlip     = 0x0010, // печать подкладного документа
		sfDontUseCutter = 0x0020, // не использовать отрезчик чеков
		sfUseWghtSensor = 0x0040, // использовать весовой датчик
		sfUseFnMethods  = 0x0080  // Разрешение на использование fn-методов (параметр pp.ini [config] ShtrihFRUseFnMethods)
	};
	//
	// Descr: Варианты соединения с кассовым регистратором
	//
	enum {
		connmodeUndef = 0, // Соединение не установлено
		connmodeCom,       // Соединение через com-port
		connmodeComV2,     // Соединение через com-port (вызов Connect2)
		connmodeServer     // Соединение с сервером (возможно local) (вызов ServerConnect)
	};
	static FR_INTRF * P_DrvFRIntrf;
	static int  RefToIntrf;
	static uint PayTypeRegFlags;   // Флаги успешности получения интерфейсов для Summ1..Summ16
	enum {
		extmethfCloseCheckEx           = 0x00000001,
		extmethfFNCheckItemBarcode     = 0x00000002,
		extmethfFNCheckItemBarcode2    = 0x00000004,
		extmethfBarcodeHex             = 0x00000008,
		extmethfItemStatus             = 0x00000010, 
		extmethfCheckItemMode          = 0x00000020,
		extmethfTLVDataHex             = 0x00000040,
		extmethfCheckItemLocalResult   = 0x00000080,
		extmethfCheckItemLocalError    = 0x00000100,
		extmethfMarkingType2           = 0x00000200,
		extmethfKMServerErrorCode      = 0x00000400,
		extmethfKMServerCheckingStatus = 0x00000800,
		extmethfDivisionalQuantity     = 0x00001000,
		extmethfNumerator              = 0x00002000,
		extmethfDenominator            = 0x00004000,
		extmethfFNCloseCheckEx         = 0x00008000, // @v11.7.2
		extmethfServerConnect          = 0x00010000, // @v12.0.3
		extmethfServerDisconnect       = 0x00020000, // @v12.0.3
		extmethfConnect2               = 0x00040000, // @v12.0.3
		extmethfOpenSession            = 0x00080000, // @v12.0.4
		extmethfFNSendItemBarcode      = 0x00100000, // @v12.0.4
	};
	static uint ExtMethodsFlags;   // Флаги успешности получения расширенных методов драйвера
	long   CashierPassword;    // Пароль кассира
	long   AdmPassword;        // Пароль сист.администратора
	int    ResCode;            //
	int    ErrCode;            //
	int    DeviceType;         //
	int    SCardPaymEntryN;    // PPINIPARAM_SHTRIHFRSCARDPAYMENTRY Регистр аппарата, в который заносится оплата по корпоративной карте [1..16]
	long   CheckStrLen;        //
	long   Flags;              //
	int    ConnectionMode;     // @v12.0.5 connmodeXXX
	SVerT  ProtocolVer;        // @v11.7.11 @!ConnectFR()
	uint   RibbonParam;        //
	SString AdmName;           // Имя сист.администратора
};

FR_INTRF * SCS_SHTRIHFRF::P_DrvFRIntrf = 0; // @global
int  SCS_SHTRIHFRF::RefToIntrf = 0;         // @global
uint SCS_SHTRIHFRF::PayTypeRegFlags = 0;    // @global
uint SCS_SHTRIHFRF::ExtMethodsFlags = 0;    // @global

class CM_SHTRIHFRF : public PPCashMachine {
public:
	CM_SHTRIHFRF(PPID cashID) : PPCashMachine(cashID)
	{
	}
	PPSyncCashSession * SyncInterface();
};

PPSyncCashSession * CM_SHTRIHFRF::SyncInterface()
{
	PPSyncCashSession * cs = 0;
	if(IsValid()) {
		cs = new SCS_SHTRIHFRF(NodeID, NodeRec.Name, NodeRec.Port);
		CALLPTRMEMB(cs, Init(NodeRec.Name, NodeRec.Port));
	}
	return cs;
}

REGISTER_CMT(SHTRIHFRF, true, false);

SCS_SHTRIHFRF::SCS_SHTRIHFRF(PPID n, char * name, char * port) : PPSyncCashSession(n, name, port),
	CashierPassword(0), AdmPassword(0), ResCode(RESCODE_NO_ERROR), ErrCode(SYNCPRN_NO_ERROR),
	DeviceType(devtypeUndef), CheckStrLen(DEF_STRLEN), Flags(0), RibbonParam(0), SCardPaymEntryN(0),
	IsDebugMode(LOGIC(CConfig.Flags & CCFLG_DEBUG)), ConnectionMode(connmodeUndef)
{
	if(SCn.Flags & CASHF_NOTUSECHECKCUTTER)
		Flags |= sfDontUseCutter;
	RefToIntrf++;
	SETIFZ(P_DrvFRIntrf, InitDriver());
}
	
SCS_SHTRIHFRF::~SCS_SHTRIHFRF()
{
	if(Flags & sfConnected) {
		switch(ConnectionMode) {
			case connmodeCom:
			case connmodeComV2:
				if(!ExecFR(Disconnect))
					LogLastError();
				break;
			case connmodeServer:
				if(!ExecFR(ServerDisconnect))
					LogLastError();
				break;
			default:
				constexpr int ShtrihFR_ConnectionModeUndefined = 0;
				assert(ShtrihFR_ConnectionModeUndefined);
				break;
		}
	}
	if(--RefToIntrf == 0)
		ZDELETE(P_DrvFRIntrf);
}

/*virtual*/int SCS_SHTRIHFRF::Diagnostics(StringSet * pSs) // @v12.0.3
{
	int    ok = 1;
	int    server_connected = 0;
	char   server_version[256];
	server_version[0] = 0;
	// @debug {
	if(pSs) {
		/*
		P_DrvFRIntrf->GetProperty(ServerConnected, &server_connected);
		const char * p_computer_name = "localhost";
		SetFR(ComputerName, p_computer_name);
		ExecFR_WithoutPassword(ServerConnect);
		P_DrvFRIntrf->GetProperty(ServerConnected, &server_connected);
		P_DrvFRIntrf->GetProperty(ServerVersion, server_version, sizeof(server_version));
		ExecFR(Beep);
		*/
	}
	// } @debug 
	return ok;
}

int SCS_SHTRIHFRF::CheckForCash(double sum)
{
	double cash_sum = 0.0;
	return GetSummator(&cash_sum) ? ((cash_sum < sum) ? -1 : 1) : 0;
}

int SCS_SHTRIHFRF::CheckForEKLZOrFMOverflow()
{
	int    ok = 1;
	int    is_eklz_present = 0;
	int    is_eklz_overflow = 0;
	int    is_fm_overflow = 0;
	int    free_rec_in_fm = 0;
	THROW(ExecFR(GetECRStatus));
	THROW(GetFR(EKLZIsPresent, &is_eklz_present));
	if(is_eklz_present)
		THROW(GetFR(IsEKLZOverflow, &is_eklz_overflow));
	THROW(GetFR(FMOverflow, &is_fm_overflow));
	THROW(GetFR(FreeRecordInFM, &free_rec_in_fm));
	THROW_PP(!is_eklz_overflow && !is_fm_overflow && free_rec_in_fm > 2, PPERR_SYNCCASH_OVERFLOW);
	CATCH
		ok = LogLastError();
	ENDCATCH
	return ok;
}

int SCS_SHTRIHFRF::CheckForSessionOver()
{
	int    ok = -1;
	int    is_24_hours_over = 0;
	THROW(ConnectFR());
	THROW(ExecFR(GetECRStatus));
	THROW(GetFR(IsFM24HoursOver, &is_24_hours_over));
	if(is_24_hours_over)
		ok = 1;
	CATCH
		ok = LogLastError();
	ENDCATCH
	return ok;
}

int  SCS_SHTRIHFRF::CheckForRibbonUsing(uint ribbonParam)
{
	int    ok = 1;
	if(ribbonParam) {
		if((RibbonParam & SlipLineParam::fRegRegular) != (ribbonParam & SlipLineParam::fRegRegular)) {
			THROW(SetFR(UseReceiptRibbon, BIN(ribbonParam & SlipLineParam::fRegRegular)));
			SETFLAG(RibbonParam, SlipLineParam::fRegRegular, ribbonParam & SlipLineParam::fRegRegular);
		}
		if(DeviceType == devtypeShtrih && (RibbonParam & SlipLineParam::fRegJournal) != (ribbonParam & SlipLineParam::fRegJournal)) {
			THROW(SetFR(UseJournalRibbon, BIN(ribbonParam & SlipLineParam::fRegJournal)));
			SETFLAG(RibbonParam, SlipLineParam::fRegJournal, ribbonParam & SlipLineParam::fRegJournal);
		}
	}
	CATCHZOK
	return ok;
}

void SCS_SHTRIHFRF::CutLongTail(char * pBuf)
{
	if(pBuf && static_cast<long>(sstrlen(pBuf)) > CheckStrLen) {
		pBuf[CheckStrLen + 1] = 0;
		char * p = sstrrchr(pBuf, ' ');
		if(p)
			*p = 0;
		else
			pBuf[CheckStrLen] = 0;
	}
}

void SCS_SHTRIHFRF::CutLongTail(SString & rBuf)
{
	char  buf[256];
	rBuf.CopyTo(buf, sizeof(buf));
	CutLongTail(buf);
	rBuf = buf;
}

void SCS_SHTRIHFRF::SetCheckLine(char pattern, char * pBuf)
{
	if(pBuf) {
		memset(pBuf, pattern, CheckStrLen);
		pBuf[CheckStrLen] = 0;
	}
}

// "СУММА БЕЗ СКИДКИ;КАРТА;ВЛАДЕЛЕЦ;СКИДКА;Чек не напечатан;НАЛОГ С ПРОДАЖ;СУММА ПО СТАВКЕ НДС;НСП;ПОЛУЧАТЕЛЬ;КОПИЯ ЧЕКА;ВОЗВРАТ ПРОДАЖИ;ПРОДАЖА;ИТОГ;БЕЗНАЛИЧНАЯ ОПЛАТА"

int	SCS_SHTRIHFRF::PrintDiscountInfo(const CCheckPacket * pPack, uint flags)
{
	int    ok = 1;
	double amt = R2(fabs(MONEYTOLDBL(pPack->Rec.Amount)));
	double dscnt = R2(MONEYTOLDBL(pPack->Rec.Discount));
	if(flags & PRNCHK_RETURN)
		dscnt = -dscnt;
	if(dscnt > 0.0) {
		double  pcnt = round(dscnt * 100.0 / (amt + dscnt), 1);
		SString prn_str;
		SString temp_buf;
		SCardCore scc;
		THROW(SetFR(StringForPrinting, prn_str.Z().CatCharN('-', CheckStrLen)));
		THROW(ExecFR(PrintString));
		temp_buf.Z().Cat(amt + dscnt, SFMT_MONEY);
		PPLoadText(PPTXT_CCFMT_AMTWODISCOUNT, prn_str);
		prn_str.ToUpper().Transf(CTRANSF_INNER_TO_OUTER);
		prn_str.CatCharN(' ', CheckStrLen - prn_str.Len() - temp_buf.Len()).Cat(temp_buf);
		THROW(SetFR(StringForPrinting, prn_str));
		THROW(ExecFR(PrintString));
		if(scc.Search(pPack->Rec.SCardID, 0) > 0) {
			PPLoadText(PPTXT_CCFMT_CARD, prn_str);
			prn_str.ToUpper().Transf(CTRANSF_INNER_TO_OUTER);
			prn_str.Space().Cat(scc.data.Code);
			THROW(SetFR(StringForPrinting, prn_str));
			THROW(ExecFR(PrintString));
			if(scc.data.PersonID && GetPersonName(scc.data.PersonID, temp_buf) > 0) {
				PPLoadText(PPTXT_CCFMT_CARDOWNER, prn_str);
				prn_str.ToUpper().Transf(CTRANSF_INNER_TO_OUTER);
				prn_str.Space().Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
				CutLongTail(prn_str);
				THROW(SetFR(StringForPrinting, prn_str));
				THROW(ExecFR(PrintString));
			}
		}
		temp_buf.Z().Cat(dscnt, SFMT_MONEY);
		PPLoadText(PPTXT_CCFMT_DISCOUNT, prn_str);
		prn_str.ToUpper().Transf(CTRANSF_INNER_TO_OUTER);
		prn_str.Space().Cat(pcnt, MKSFMTD(0, (flags & PRNCHK_ROUNDINT) ? 0 : 1, NMBF_NOTRAILZ)).CatChar('%');
		prn_str.CatCharN(' ', CheckStrLen - prn_str.Len() - temp_buf.Len()).Cat(temp_buf);
		THROW(SetFR(StringForPrinting, prn_str));
		THROW(ExecFR(PrintString));
	}
	CATCHZOK
	return ok;
}

int SCS_SHTRIHFRF::LineFeed(int lineCount, int useReceiptRibbon, int useJournalRibbon)
{
	int    ok = 1, cur_receipt, cur_journal;
	THROW(GetFR(UseReceiptRibbon, &cur_receipt));
	THROW(GetFR(UseJournalRibbon, &cur_journal));
	if(cur_receipt != useReceiptRibbon)
		THROW(SetFR(UseReceiptRibbon, useReceiptRibbon));
	if(cur_journal != useJournalRibbon)
		THROW(SetFR(UseJournalRibbon, useJournalRibbon));
	THROW(SetFR(StringQuantity, lineCount));
	THROW(ExecFR(FeedDocument));
	if(cur_receipt != useReceiptRibbon)
		THROW(SetFR(UseReceiptRibbon, cur_receipt));
	if(cur_journal != useJournalRibbon)
		THROW(SetFR(UseJournalRibbon, cur_journal));
	CATCHZOK
	return ok;
}

static void WriteLogFile_PageWidthOver(const char * pFormatName)
{
	SString msg_fmt;
	SString msg;
	msg.Printf(PPLoadTextS(PPTXT_SLIPFMT_WIDTHOVER, msg_fmt), pFormatName);
	PPLogMessage(PPFILNAM_SHTRIH_LOG, msg, LOGMSGF_TIME|LOGMSGF_USER);
}

int SCS_SHTRIHFRF::Cut(int withCleaning)
{
	int    ok = 1;
	if(DeviceType == devtypeShtrih) {
		if(!(Flags & sfDontUseCutter))
			THROW(ExecFR(CutCheck));
		if(withCleaning) {
			THROW(LineFeed(DEF_LINEFEED_NUMBER, FALSE, TRUE));
		}
	}
	else if(oneof2(DeviceType, devtypeCombo, devtypeMini) && !(Flags & sfDontUseCutter)) {
		THROW(ExecFR(CutCheck));
	}
	CATCHZOK
	return ok;
}

int SCS_SHTRIHFRF::GetBarcodePrintMethodAndStd(int innerBarcodeStd, int * pOemMethod, int * pOemStd)
{
	int    ok = 1;
	int    oem_std = -1;
	int    oem_method = 0;
	switch(innerBarcodeStd) {
		case BARCSTD_CODE39: oem_std = 4; break;
		case BARCSTD_UPCA: oem_std = 0; break;
		case BARCSTD_UPCE: oem_std = 1; break;
		case BARCSTD_EAN13: oem_std = 2; break;
		case BARCSTD_EAN8: oem_std = 3; break;
		case BARCSTD_ANSI: oem_std = 6; break;
		case BARCSTD_CODE93: oem_std = 7; break;
		case BARCSTD_CODE128: oem_std = 8; break;
		case BARCSTD_PDF417: oem_std = 10; break;
		case BARCSTD_QR:
			oem_method = bcmPrintBarcodeGraph;
			oem_std = 3;
			break;
		default:
			ok = 0;
	}
	ASSIGN_PTR(pOemMethod, oem_method);
	ASSIGN_PTR(pOemStd, oem_std);
	return ok;
}

static int GetShtrihVatRateIdent(double vatRate, bool isVatFree) // @v11.2.12
{
	int   result = 0;
	if(isVatFree)
		result = 4;
	else {
		if(feqeps(vatRate, 20.0, 1E-3) || feqeps(vatRate, 18.0, 1E-3))
			result = 1;
		else if(feqeps(vatRate, 10.0, 1E-3))
			result = 2;
		else if(feqeps(vatRate, 0.0, 1E-3))
			result = 3;
	}
	return result;
}
//
// PrintCheck - коды возврата:
//   SYNCPRN_NO_ERROR           - OK
//   SYNCPRN_ERROR_AFTER_PRINT  - сбой после печати чека
//   SYNCPRN_ERROR              - сбой
//   SYNCPRN_ERROR_WHILE_PRINT  - сбой во время печати чека
//   SYNCPRN_CANCEL             - отмена до начала печати чека
//   SYNCPRN_CANCEL_WHILE_PRINT - отмена во время печати чека
// Если SYNCPRN_ERROR_WHILE_PRINT, SYNCPRN_CANCEL_WHILE_PRINT -
//   надо оставить флаг прерывания печати чека CASHF_LASTCHKCANCELLED в NodeRec.Flags
//
int SCS_SHTRIHFRF::PrintCheck(CCheckPacket * pPack, uint flags)
{
	const   bool use_fn_op = LOGIC(Flags & sfUseFnMethods);
	int     ok = 1;
	int     chk_no = 0;
	bool    is_format = false;
	SString temp_buf;
	SString fmt_buf;
	SString msg_buf;
	SString added_buf;
	SString chzn_sid;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR;
	THROW_INVARG(pPack);
	if(pPack->GetCount() == 0)
		ok = -1;
	else {
		SlipDocCommonParam sdc_param;
		double amt = fabs(R2(MONEYTOLDBL(pPack->Rec.Amount)));
		double sum = fabs(pPack->_Cash) + 0.001;
		double running_total = 0.0;
		double real_fiscal = 0.0;
		double real_nonfiscal = 0.0;
		pPack->HasNonFiscalAmount(&real_fiscal, &real_nonfiscal);
		const double _fiscal = (PPConst::Flags & PPConst::fDoSeparateNonFiscalCcItems) ? real_fiscal : (real_fiscal + real_nonfiscal);
		const CcAmountList & r_al = pPack->AL_Const();
		const bool   is_al = LOGIC(r_al.getCount());
		const bool   is_vat_free = (CnObj.IsVatFree(NodeID) > 0); // @v11.2.11
		const double amt_bnk = is_al ? r_al.Get(CCAMTTYP_BANK) : ((pPack->Rec.Flags & CCHKF_BANKING) ? _fiscal : 0.0);
		const double amt_cash = (PPConst::Flags & PPConst::fDoSeparateNonFiscalCcItems) ? (_fiscal - amt_bnk) : (is_al ? r_al.Get(CCAMTTYP_CASH) : (_fiscal - amt_bnk));
		const double amt_ccrd = is_al ? r_al.Get(CCAMTTYP_CRDCARD) : (real_fiscal + real_nonfiscal - _fiscal);
		PPID   tax_sys_id = 0;
		CnObj.GetTaxSystem(NodeID, pPack->Rec.Dt, &tax_sys_id);
		if(SCn.LocID)
			PPRef->Ot.GetTagStr(PPOBJ_LOCATION, SCn.LocID, PPTAG_LOC_CHZNCODE, chzn_sid);
		THROW(ConnectFR());
		if(flags & PRNCHK_LASTCHKANNUL)
			THROW(AnnulateCheck());
		if(flags & PRNCHK_RETURN && !(flags & PRNCHK_BANKING)) {
			int    is_cash;
			THROW(is_cash = CheckForCash(amt));
			THROW_PP(is_cash > 0, PPERR_SYNCCASH_NO_CASH);
		}
		if(P_SlipFmt) {
			int    prn_total_sale = 1;
			int    r = 0;
			SString line_buf;
			const SString format_name("CCheck");
			SlipLineParam sl_param;
			THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
			if(r > 0) {
				is_format = true;
				if(static_cast<long>(sdc_param.PageWidth) > CheckStrLen)
					WriteLogFile_PageWidthOver(format_name);
				RibbonParam = 0;
				CheckForRibbonUsing(sdc_param.RegTo);
				if(_fiscal != 0.0) {
					THROW(SetFR(CheckType, (flags & PRNCHK_RETURN) ? 2L : 0L));
					THROW(ExecFR(OpenCheck));
					Flags |= sfCheckOpened;
					if(use_fn_op) {
						/*
							полеОбъектККМ.CheckType = 0;
							полеОбъектККМ.OpenCheck();

							полеОбъектККМ.TagNumber = 1084;
							полеОбъектККМ.FNBeginSTLVTag();
							my_TagID = полеОбъектККМ.TagID;

							полеОбъектККМ.TagID = my_TagID;
							полеОбъектККМ.TagNumber = 1085;
							полеОбъектККМ.TagType = 7;
							полеОбъектККМ.TagValueStr = "mdlp";
							полеОбъектККМ.FNAddTag();

							полеОбъектККМ.TagID = my_TagID;
							полеОбъектККМ.TagNumber = 1086;
							полеОбъектККМ.TagType = 7;
							полеОбъектККМ.TagValueStr = "sid"+subject_id+"&";
							полеОбъектККМ.FNAddTag();

							полеОбъектККМ.FNSendSTLVTag();
						*/
						if(chzn_sid.NotEmpty()) {
							int   stlv_tag_id = 0;
							THROW(SetFR(TagNumber, 1084));
							THROW(ExecFR(FNBeginSTLVTag));
							LogDebug("GetProperty", TagID, 0); // @v11.6.12
							THROW_PP_S(P_DrvFRIntrf->GetProperty(TagID, &stlv_tag_id) > 0, PPERR_SHTRIHFRGETPROPFAULT, P_DrvFRIntrf->GetNameByID(TagID, SLS.AcquireRvlStr()));
							//
							THROW(SetFR(TagID, stlv_tag_id));
							THROW(SetFR(TagNumber, 1085));
							THROW(SetFR(TagType, 7));
							THROW(SetFR(TagValueStr, "mdlp"));
							THROW(ExecFR(FNAddTag));
							//
							THROW(SetFR(TagID, stlv_tag_id));
							THROW(SetFR(TagNumber, 1086));
							THROW(SetFR(TagType, 7));
							(temp_buf = "sid").Cat(chzn_sid).CatChar('&');
							THROW(SetFR(TagValueStr, temp_buf));
							THROW(ExecFR(FNAddTag));
							//
							THROW(ExecFR(FNSendSTLVTag));
						}
					}
				}
				else {
					THROW(SetFR(DocumentName, "" /*sdc_param.Title*/));
					THROW(ExecFR(PrintDocumentTitle));
				}
				for(P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
					if(sl_param.Flags & SlipLineParam::fRegFiscal) {
						CheckForRibbonUsing(SlipLineParam::fRegRegular|SlipLineParam::fRegJournal);
						//double _q = round(sl_param.Qtty, 3);
						//double _p = round(sl_param.Price, 2);
						const double _q = sl_param.Qtty;
						const double _p = sl_param.Price;
						const int    shtrih_vat_rate_ident = GetShtrihVatRateIdent(sl_param.VatRate, is_vat_free); // @v11.2.11
						running_total += (_q * _p);
						THROW(SetFR(Quantity, _q));
						THROW(SetFR(Price, fabs(_p)));
						{
							(temp_buf = sl_param.Text).Strip().Transf(CTRANSF_INNER_TO_OUTER).Trim(CheckStrLen);
							THROW(SetFR(StringForPrinting, temp_buf));
						}
						THROW(SetFR(Department, (sl_param.DivID > 16 || sl_param.DivID < 0) ? 0 :  sl_param.DivID));
						THROW(SetFR(Tax1, shtrih_vat_rate_ident)); // @v11.2.12 0L-->shtrih_vat_rate_ident
						if(use_fn_op) {
							/*{
								FNOperation
								PaymentTypeSign
								PaymentItemSign
								FNSendItemCodeData
								MarkingType
								GTIN
								SerialNumber
							}*/
							if(sl_param.PaymTermTag != CCheckPacket::pttUndef) {
								int    paym_type_sign = 0;
								//PaymentTypeSign
								switch(sl_param.PaymTermTag) {
									case CCheckPacket::pttFullPrepay: paym_type_sign = 1; break; // 1. Предоплата 100%
									case CCheckPacket::pttPrepay: paym_type_sign = 2; break; // 2. Частичная предоплата
									case CCheckPacket::pttAdvance: paym_type_sign = 3; break; // 3. Аванс
									case CCheckPacket::pttFullPayment: paym_type_sign = 4; break; // 4. Полный расчет
									case CCheckPacket::pttPartial: paym_type_sign = 5; break; // 5. Частичный расчет и кредит
									case CCheckPacket::pttCreditHandOver: paym_type_sign = 6; break; // 6. Передача в кредит
									case CCheckPacket::pttCredit: paym_type_sign = 7; break; // 7. Оплата кредита
								}
								if(paym_type_sign) {
									THROW(SetFR(PaymentTypeSign, paym_type_sign));
								}
							}
							if(sl_param.SbjTermTag != CCheckPacket::sttUndef) {
								int    paym_subj_sign = 0;
								/*
									1. Товар
									2. Подакцизный товар
									3. Работа
									4. Услуга
									5. Ставка азартной игры
									6. Выигрыш азартной игры
									7. Лотерейный билет
									8. Выигрыш лотереи
									9. Предоставление РИД
									10. Платеж
									11. Агентское вознаграждение
									12. Составной предмет расчета
									13. Иной предмет расчета
									14. Имущественное право
									15. Внереализационный доход
									16. Страховые взносы
									17. Торговый сбор
									18. Курортный сбор
								*/
								switch(sl_param.SbjTermTag) {
									case CCheckPacket::sttGood: paym_subj_sign = 1; break; // 1. Товар
									case CCheckPacket::sttExcisableGood: paym_subj_sign = 2; break; // 2. Подакцизный товар
									case CCheckPacket::sttExecutableWork: paym_subj_sign = 3; break; // 3. Работа
									case CCheckPacket::sttService: paym_subj_sign = 4; break; // 4. Услуга
									case CCheckPacket::sttBetting: paym_subj_sign = 5; break; // 5. Ставка азартной игры
									case CCheckPacket::sttPaymentGambling: paym_subj_sign = 6; break; // 6. Выигрыш азартной игры
									case CCheckPacket::sttBettingLottery: paym_subj_sign = 7; break; // 7. Лотерейный билет
									case CCheckPacket::sttPaymentLottery: paym_subj_sign = 8; break; // 8. Выигрыш лотереи
									case CCheckPacket::sttGrantRightsUseIntellectualActivity: paym_subj_sign = 9; break; // 9. Предоставление РИД
									case CCheckPacket::sttAdvance: paym_subj_sign = 10; break; // 10. Платеж
									case CCheckPacket::sttPaymentsPayingAgent: paym_subj_sign = 11; break; // 11. Агентское вознаграждение
									case CCheckPacket::sttSubjTerm: paym_subj_sign = 12; break; // 12. Составной предмет расчета
									case CCheckPacket::sttNotSubjTerm: paym_subj_sign = 13; break; // 13. Иной предмет расчета
									case CCheckPacket::sttTransferPropertyRights: paym_subj_sign = 14; break; // 14. Имущественное право
									case CCheckPacket::sttNonOperatingIncome: paym_subj_sign = 15; break; // 15. Внереализационный доход
									//case CCheckPacket::sttExpensesReduceTax: str_id = DVCPARAM_STT_EXPENSESREDUCETAX; break;
									case CCheckPacket::sttAmountMerchantFee: paym_subj_sign = 17; break; // 17. Торговый сбор
									case CCheckPacket::sttResortFee: paym_subj_sign = 18; break; // 18. Курортный сбор
									//case CCheckPacket::sttDeposit: str_id = DVCPARAM_SUBJTERMTAG; break;
								}
								if(paym_subj_sign) {
									THROW(SetFR(PaymentItemSign, paym_subj_sign));
								}
							}
							if(flags & PRNCHK_RETURN) {
								THROW(SetFR(CheckType, 2L));
							}
							else {
								THROW(SetFR(CheckType, 1L));
							}
							THROW(ExecFR(FNOperation));
							if(sl_param.ChZnProductType && sl_param.ChZnGTIN.NotEmpty() && sl_param.ChZnSerial.NotEmpty()) {
								int    marking_type = 0;
								switch(sl_param.ChZnProductType) {
									case GTCHZNPT_FUR: marking_type = 0x5246; break;
									case GTCHZNPT_TOBACCO: marking_type = 0x444D; break;
									case GTCHZNPT_ALTTOBACCO: marking_type = 0x444D; break; // @v11.9.0
									case GTCHZNPT_SHOE: marking_type = 0x444D; break;
									case GTCHZNPT_MEDICINE: marking_type = 0x444D; break;
									case GTCHZNPT_CARTIRE: marking_type = 0x444D; break;
									case GTCHZNPT_MILK: marking_type = 0x444D; break; // @v11.5.7
									case GTCHZNPT_WATER: marking_type = 0x444D; break; // @v11.5.7
									case GTCHZNPT_DIETARYSUPPLEMENT: marking_type = 0x444D; break; // @v12.0.4
									case GTCHZNPT_BEER: marking_type = 0x444D; break; // @v12.0.4
									case GTCHZNPT_DRAFTBEER_AWR: marking_type = 0x444D; break; // @v12.0.5
									case GTCHZNPT_DRAFTBEER: marking_type = 0x444D; break; // @v12.0.4
									case GTCHZNPT_TEXTILE: marking_type = 0x444D; break; // @v12.0.4
									case GTCHZNPT_ANTISEPTIC: marking_type = 0x444D; break; // @v12.0.5
									case GTCHZNPT_MEDICALDEVICES: marking_type = 0x444D; break; // @v12.1.2
									case GTCHZNPT_SOFTDRINKS: marking_type = 0x444D; break; // @v12.1.10
									case GTCHZNPT_NONALCBEER: marking_type = 0x444D; break; // @v12.2.6 
								}
								if(marking_type) {
									/* @v12.0.4
									THROW(SetFR(MarkingType, marking_type));
									THROW(SetFR(GTIN, sl_param.ChZnGTIN));
									temp_buf = sl_param.ChZnSerial;
									THROW(SetFR(SerialNumber, temp_buf));
									THROW(ExecFR(FNSendItemCodeData));
									*/
									{ // @v12.0.4 @construction 
										const uint code_len = sl_param.ChZnCode.Len();
										temp_buf.Z();
										for(uint cidx = 0; cidx < code_len; cidx++) {
											temp_buf.CatHex(static_cast<uint8>(sl_param.ChZnCode[cidx]));
										}
										THROW(SetFR(BarcodeHex, temp_buf));
										THROW(SetFR(MarkingType, marking_type));
										THROW(ExecFR(FNSendItemBarcode));
									}
								}
							}
						}
						else {
							THROW(ExecFR((flags & PRNCHK_RETURN) ? ReturnSale : Sale));
						}
						Flags |= sfCheckOpened;
						prn_total_sale = 0;
					}
					else if(sl_param.Kind == sl_param.lkBarcode) {
						;
					}
					else if(sl_param.Kind == sl_param.lkSignBarcode) {
						if(line_buf.NotEmptyS()) {
							CheckForRibbonUsing(SlipLineParam::fRegRegular);
							int    oem_method = 0;
							int    oem_std = 0;
							if(GetBarcodePrintMethodAndStd(sl_param.BarcodeStd, &oem_method, &oem_std) > 0) {
								/*if(oem_method == bcmPrintBarcodeUsingPrinter) {
									//Password
									//BarCode
									//LineNumber
									//BarcodeType
									//BarWidth
									//FontType
									//HRIPosition
								}
								else*/ if(oem_method == bcmPrintBarcodeGraph) {
									/*
                                    BarCode
                                    LineNumber
                                    BarcodeType
                                    BarWidth
                                    BarcodeAlignment
                                    PrintBarcodeText
                                    */
									THROW(SetFR(BarCode, line_buf));
									THROW(SetFR(BarcodeType, oem_std));
									THROW(SetFR(FirstLineNumber, 0L));
									THROW(SetFR(LineNumber, NZOR(sl_param.BarcodeHt, 260L)));
									THROW(ExecFR(PrintBarcodeGraph));
								}
							}
						}
					}
					else {
						CheckForRibbonUsing(sl_param.Flags);
						THROW(SetFR(StringForPrinting, line_buf.Trim((sl_param.Font > 1) ? CheckStrLen / 2 : CheckStrLen)));
						THROW(ExecFR((sl_param.Font > 1) ? PrintWideString : PrintString));
					}
				}
				running_total = fabs(running_total);
				CheckForRibbonUsing(SlipLineParam::fRegRegular|SlipLineParam::fRegJournal);
				THROW(SetFR(StringForPrinting, ""));
				if(prn_total_sale) {
					// @v11.2.11 {
					const double fixed_vat_rate = 20.0; // ? sl_param.VatRate
					const int    shtrih_vat_rate_ident = GetShtrihVatRateIdent(fixed_vat_rate, is_vat_free); // @v11.2.12
					// } @v11.2.11 
					if(_fiscal != 0.0) {
						if(!pPack->GetCount()) {
							THROW(SetFR(Quantity, 1L));
							THROW(SetFR(Price, amt));
							THROW(SetFR(Tax1, shtrih_vat_rate_ident)); // @v11.2.11 0-->shtrih_vat_rate_ident
							THROW(ExecFR((flags & PRNCHK_RETURN) ? ReturnSale : Sale));
							Flags |= sfCheckOpened;
							running_total += amt;
						}
						else /*if(fiscal != 0.0)*/ {
							THROW(SetFR(Quantity, 1L));
							THROW(SetFR(Price, _fiscal));
							THROW(SetFR(Tax1, shtrih_vat_rate_ident)); // @v11.2.11 0-->shtrih_vat_rate_ident
							THROW(ExecFR((flags & PRNCHK_RETURN) ? ReturnSale : Sale));
							Flags |= sfCheckOpened;
							running_total += _fiscal;
						}
					}
				}
				else if(running_total != amt) {
					PPLoadText(PPTXT_SHTRIH_RUNNGTOTALGTAMT, fmt_buf);
					const char * p_sign = (running_total > amt) ? " > " : ((running_total < amt) ? " < " : " ?==? ");
					added_buf.Z().Cat(running_total, MKSFMTD(0, 12, NMBF_NOTRAILZ)).Cat(p_sign).Cat(amt, MKSFMTD(0, 12, NMBF_NOTRAILZ));
					msg_buf.Printf(fmt_buf, added_buf.cptr());
					PPLogMessage(PPFILNAM_SHTRIH_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
				}
			}
		}
		if(!is_format) {
			CCheckLineTbl::Rec ccl;
			for(uint pos = 0; pPack->EnumLines(&pos, &ccl);) {
				const int division = (ccl.DivID >= CHECK_LINE_IS_PRINTED_BIAS) ? ccl.DivID - CHECK_LINE_IS_PRINTED_BIAS : ccl.DivID;
				// Наименование товара
				GetGoodsName(ccl.GoodsID, temp_buf);
				temp_buf.Strip().Transf(CTRANSF_INNER_TO_OUTER).Trim(CheckStrLen);
				THROW(SetFR(StringForPrinting, temp_buf));
				// Цена
				THROW(SetFR(Price, R2(intmnytodbl(ccl.Price) - ccl.Dscnt)));
				// Количество
				THROW(SetFR(Quantity, R3(fabs(ccl.Quantity))));
				{
					int _dep = (division > 16 || division < 0) ? 0 : division;
					if(_dep > 0 && _dep <= 16)
						THROW(SetFR(Department, _dep));
				}
				THROW(SetFR(Tax1, 0L));
				THROW(ExecFR((flags & PRNCHK_RETURN) ? ReturnSale : Sale));
				Flags |= sfCheckOpened;
			}
			// Информация о скидке
			if(DeviceType == devtypeShtrih)
				THROW(SetFR(UseJournalRibbon, FALSE));
			THROW(PrintDiscountInfo(pPack, flags));
			if(DeviceType == devtypeShtrih)
				THROW(SetFR(UseJournalRibbon, TRUE));
			temp_buf.Z().CatCharN('=', CheckStrLen);
			THROW(SetFR(StringForPrinting, temp_buf));
		}
		{
			if(running_total > sum || ((flags & PRNCHK_BANKING) && running_total != sum))
				sum = running_total;
			const double __amt_bnk = amt_bnk;
			const double __amt_ccrd = amt_ccrd;
			const double __amt_cash = sum - __amt_bnk - __amt_ccrd;
			const int ccrd_entry_n = inrangeordefault(SCardPaymEntryN, 1, 16, (ExtMethodsFlags & extmethfCloseCheckEx) ? 14 : 2);
			PPID  ccrd_entry_id = 0;
			switch(ccrd_entry_n) {
				case  1: ccrd_entry_id =  Summ1; break;
				case  2: ccrd_entry_id =  Summ2; break;
				case  3: ccrd_entry_id =  Summ3; break;
				case  4: ccrd_entry_id =  Summ4; break;
				case  5: ccrd_entry_id =  Summ5; break;
				case  6: ccrd_entry_id =  Summ6; break;
				case  7: ccrd_entry_id =  Summ7; break;
				case  8: ccrd_entry_id =  Summ8; break;
				case  9: ccrd_entry_id =  Summ9; break;
				case 10: ccrd_entry_id = Summ10; break;
				case 11: ccrd_entry_id = Summ11; break;
				case 12: ccrd_entry_id = Summ12; break;
				case 13: ccrd_entry_id = Summ13; break;
				case 14: ccrd_entry_id = Summ14; break;
				case 15: ccrd_entry_id = Summ15; break;
				case 16: ccrd_entry_id = Summ16; break;
			}
			if(PayTypeRegFlags & (1U << ccrd_entry_n) && !ccrd_entry_id) {
				ccrd_entry_id = Summ2;
			}
			{
				msg_buf.Z().Cat("Payment").CatDiv(':', 2).Cat("PayTypeRegFlags-hex").Eq().CatHex(static_cast<ulong>(PayTypeRegFlags));
				msg_buf.Space().Cat("PayTypeRegFlags-dec").Eq().Cat(PayTypeRegFlags);
				msg_buf.Space().CatEq("sum", sum, MKSFMTD(0, 6, 0)).Space().CatEq("amt_cash", __amt_cash, MKSFMTD(0, 6, 0)).Space().
					CatEq("amt_bnk", __amt_bnk, MKSFMTD(0, 6, 0)).Space().CatEq("amt_ccrd", __amt_ccrd, MKSFMTD(0, 6, 0));
				PPLogMessage(PPFILNAM_SHTRIH_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			}
			// 1  - наличная оплата
			// 2  - безналичная оплата
			// 14 - предварительная оплата
			// 15 - последующая оплата
			// @v10.6.0 if(amt_bnk != 0.0)
			THROW(SetFR(Summ1, (ccrd_entry_id == Summ1) ? (__amt_cash + __amt_ccrd) : __amt_cash));
			THROW(SetFR(Summ2, (ccrd_entry_id == Summ2) ? (__amt_bnk  + __amt_ccrd) : __amt_bnk));
			if(!oneof3(ccrd_entry_id, Summ1, Summ2, 0)) {
				THROW(SetFR(ccrd_entry_id, __amt_ccrd));
			}
			// @v11.2.11 {
			if(ExtMethodsFlags & extmethfCloseCheckEx) {
				// Код системы налогообложения. Битовое поле:
				// Бит 5|Бит 4|Бит 3|Бит 2|Бит 1|Бит 0|Описание
				// 0     0     0     0     0     1     Основная
				// 0     0     0     0     1     0     Упрощенная система налогообложения доход
				// 0     0     0     1     0     0     Упрощенная система налогообложения доход минус расход
				// 0     0     1     0     0     0     Единый налог на вмененный доход
				// 0     1     0     0     0     0     Единый сельскохозяйственный налог
				// 1     0     0     0     0     0     Патентная система налогообложения
				// TaxType   
				int   shtrih_taxtype = 0;
				switch(tax_sys_id) {
					case TAXSYSK_GENERAL:           shtrih_taxtype = 0x001; break;
					case TAXSYSK_SIMPLIFIED:        shtrih_taxtype = 0x002; break;
					case TAXSYSK_SIMPLIFIED_PROFIT: shtrih_taxtype = 0x004; break;
					case TAXSYSK_PATENT:            shtrih_taxtype = 0x020; break;
					case TAXSYSK_IMPUTED:           shtrih_taxtype = 0x008; break;
					case TAXSYSK_SINGLEAGRICULT:    shtrih_taxtype = 0x010; break;
				}
				if(shtrih_taxtype) {
					THROW(SetFR(TaxType, shtrih_taxtype));
					// @v11.2.12 @debug {
					{
						//PPTXT_LOG_CCTAXSYSTEMSET                  "Для кассового чека установлена система налогообложения %s"
						PPLoadText(PPTXT_LOG_CCTAXSYSTEMSET, fmt_buf);
						temp_buf.Z().Cat(tax_sys_id).Space().Cat("->").Space().CatHex(static_cast<long>(shtrih_taxtype));
						msg_buf.Printf(fmt_buf, temp_buf.cptr());
						PPLogMessage(PPFILNAM_SHTRIH_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
					}
					// } @v11.2.12
				}
			}
			// } @v11.2.11
		}
		if(_fiscal != 0.0) {
			// @v11.7.2 {
			if(true/*ExtMethodsFlags & extmethfFNCloseCheckEx && ProtocolVer.IsGe(5, 0, 0)*/) { // @v11.7.11 (&& ProtocolVer.IsGt(5, 0, 0))
				THROW(ExecFR(FNCloseCheckEx));
			} // } @v11.7.2 
			else if(ExtMethodsFlags & extmethfCloseCheckEx) {
				THROW(ExecFR(CloseCheckEx));
			}
			else {
				THROW(ExecFR(CloseCheck));
			}
		}
		else {
			THROW(SetFR(ReceiptOutputType, 0));
			THROW(ExecFR(OutputReceipt));
		}
		Flags &= ~sfCheckOpened;
		ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
		THROW(Cut(1));
		THROW(SetFR(RegisterNumber, CHECK_NUMBER_REG));
		THROW(ExecFR(GetOperationReg));
		THROW(GetFR(ContentsOfOperationRegister, &chk_no));
		pPack->Rec.Code = chk_no;
		ErrCode = SYNCPRN_NO_ERROR;
	}
	CATCH
		LogLastError(); // @v11.6.9
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			if(ErrCode != SYNCPRN_ERROR_AFTER_PRINT) {
				SString no_print_txt;
				PPLoadText(PPTXT_CHECK_NOT_PRINTED, no_print_txt);
				ErrCode = (Flags & sfCheckOpened) ? SYNCPRN_CANCEL_WHILE_PRINT : SYNCPRN_CANCEL;
				PPLogMessage(PPFILNAM_SHTRIH_LOG, CCheckCore::MakeCodeString(&pPack->Rec, 0, no_print_txt), LOGMSGF_TIME|LOGMSGF_USER);
				ok = 0;
			}
		}
		else {
			SetErrorMessage();
			ExecFR(Beep);
			if(Flags & sfCheckOpened)
				ErrCode = SYNCPRN_ERROR_WHILE_PRINT;
			ok = 0;
		}
	ENDCATCH
	return ok;
}

int SCS_SHTRIHFRF::OpenBox()
{
	int     ok = -1, is_drawer_open = 0;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR;
	THROW(ConnectFR());
	THROW(ExecFR(GetECRStatus));
	THROW(GetFR(IsDrawerOpen, &is_drawer_open));
	if(!is_drawer_open) {
		THROW(SetFR(DrawerNumber, DEF_DRAWER_NUMBER));
		THROW(ExecFR(OpenDrawer));
		ok = 1;
	}
	CATCH
		LogLastError(); // @v11.6.9
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			if(ErrCode != SYNCPRN_ERROR_AFTER_PRINT) {
				ErrCode = (Flags & sfCheckOpened) ? SYNCPRN_CANCEL_WHILE_PRINT : SYNCPRN_CANCEL;
				ok = 0;
			}
		}
		else {
			SetErrorMessage();
			ExecFR(Beep);
			if(Flags & sfCheckOpened)
				ErrCode = SYNCPRN_ERROR_WHILE_PRINT;
			ok = 0;
		}
	ENDCATCH
	return ok;
}

int SCS_SHTRIHFRF::GetCheckInfo(const PPBillPacket * pPack, BillTaxArray * pAry, long * pFlags, SString &rName)
{
	int    ok = 1, wovatax = 0;
	long   flags = 0;
	BillTaxEntry  bt_entry;
	PPID   main_org_id;
	if(GetMainOrgID(&main_org_id)) {
		PersonTbl::Rec prec;
		if(SearchObject(PPOBJ_PERSON, main_org_id, &prec) > 0 && prec.Flags & PSNF_NOVATAX)
			wovatax = 1;
	}
	THROW_INVARG(pPack && pAry);
	RVALUEPTR(flags, pFlags);
	if(pPack->OpTypeID == PPOPT_ACCTURN) {
		long   s_tax = 0;
		double amt1 = 0.0, amt2 = 0.0;
		PPObjAmountType amtt_obj;
		TaxAmountIDs    tais;
		double sum = BR2(pPack->Rec.Amount);
		if(sum < 0.0)
			flags |= PRNCHK_RETURN;
		sum = fabs(sum);
		amtt_obj.GetTaxAmountIDs(tais, 1);
		if(tais.STaxAmtID)
			s_tax = tais.STaxRate;
		if(tais.VatAmtID[0])
			amt1 = fabs(pPack->Amounts.Get(tais.VatAmtID[0], 0L));
		if(tais.VatAmtID[1])
			amt2 = fabs(pPack->Amounts.Get(tais.VatAmtID[1], 0L));
		bt_entry.VAT = (amt1 || amt2) ? ((amt1 > amt2) ? tais.VatRate[0] : tais.VatRate[1]) : 0;
		bt_entry.SalesTax = s_tax;
		bt_entry.Amount   = sum;
		THROW(pAry->Add(&bt_entry));
	}
	else {
		PPTransferItem * ti;
		PPObjGoods  goods_obj;
		if(pPack->OpTypeID == PPOPT_GOODSRETURN) {
			PPOprKind op_rec;
			if(GetOpData(pPack->Rec.OpID, &op_rec) > 0 && IsExpendOp(op_rec.LinkOpID) > 0)
				flags |= PRNCHK_RETURN;
		}
		else if(pPack->OpTypeID == PPOPT_GOODSRECEIPT)
			flags |= PRNCHK_RETURN;
		for(uint i = 0; pPack->EnumTItems(&i, &ti);) {
			int re;
			PPGoodsTaxEntry gtx;
			THROW(goods_obj.FetchTaxEntry2(ti->GoodsID, 0/*lotID*/, 0/*taxPayerID*/, pPack->Rec.Dt, pPack->Rec.OpID, &gtx));
			bt_entry.VAT = wovatax ? 0 : gtx.VAT;
			re = (ti->Flags & PPTFR_RMVEXCISE) ? 1 : 0;
			bt_entry.SalesTax = ((CConfig.Flags & CCFLG_PRICEWOEXCISE) ? !re : re) ? 0 : gtx.SalesTax;
			bt_entry.Amount   = ti->CalcAmount();
			THROW(pAry->Add(&bt_entry));
		}
	}
	if(pPack->Rec.Object)
		GetArticleName(pPack->Rec.Object, rName);
	CATCHZOK
	ASSIGN_PTR(pFlags, flags);
	return ok;
}

int SCS_SHTRIHFRF::InitTaxTbl(BillTaxArray * pBTaxAry, PPIDArray * pVatAry, int * pPrintTaxAction)
{
	int    ok = 1, print_tax_action = 0;
	uint   pos;
	long   cshr_pssw = CashierPassword;
	long   s_tax = 0;
	pVatAry->freeAll();
	for(pos = 0; pos < pBTaxAry->getCount(); pos++) {
		BillTaxEntry & bte = pBTaxAry->at(pos);
		if(bte.SalesTax)
			s_tax = bte.SalesTax;
		if(bte.VAT && !pVatAry->bsearch(bte.VAT))
			THROW_SL(pVatAry->ordInsert(bte.VAT, 0));
	}
	// Выбор режима печати налогов
	CashierPassword = AdmPassword;
	THROW(SetFR(TableNumber, FRCASHMODE_TBL));
	THROW(SetFR(RowNumber,   FRCASHMODE_ROW));
	THROW(SetFR(FieldNumber, oneof2(DeviceType, devtypeCombo, devtypeMini) ? COMBOCASHMODE_TAXALLCHK : FRCASHMODE_TAXALLCHK));
	THROW(ExecFR(GetFieldStruct));
	THROW(ExecFR(ReadTable));
	THROW(GetFR(ValueOfFieldInteger, &print_tax_action));
	// Начисление налогов на весь чек мы не используем,
	// а для Штрих-Комбо  и Штрих_Мини нельзя настраивать таблицу налоговых ставок при открытой смене
	if(print_tax_action || DeviceType == devtypeCombo || DeviceType == devtypeMini)
		print_tax_action = 0;
	else {
		THROW(SetFR(TableNumber, FRCASHMODE_TBL));
		THROW(SetFR(RowNumber,   FRCASHMODE_ROW));
		THROW(SetFR(FieldNumber, FRCASHMODE_TAXPRINT));
		THROW(ExecFR(GetFieldStruct));
		THROW(ExecFR(ReadTable));
		THROW(GetFR(ValueOfFieldInteger, &print_tax_action));
	}
	if(print_tax_action) {
		SString temp_buf;
		SString vat_str;
		// Настройка таблицы налоговых ставок
		// Налог с продаж
		THROW(SetFR(TableNumber, FRTAX_TBL));
		if(s_tax) {
			THROW(SetFR(RowNumber,   FRTAX_SALESTAX_ROW));
			THROW(SetFR(FieldNumber, FRTAX_FIELD_TAXAMT));
			THROW(ExecFR(GetFieldStruct));
			THROW(SetFR(ValueOfFieldInteger, s_tax));
			THROW(ExecFR(WriteTable));
			THROW(SetFR(FieldNumber, FRTAX_FIELD_TAXNAME));
			THROW(ExecFR(GetFieldStruct));
			{
				PPLoadText(PPTXT_CCFMT_STAX, temp_buf); // "НАЛОГ С ПРОДАЖ"
				temp_buf.ToUpper().Transf(CTRANSF_INNER_TO_OUTER);
				THROW(SetFR(ValueOfFieldString, temp_buf));
			}
			THROW(ExecFR(WriteTable));
		}
		// Ставки НДС
		for(pos = 0; pos < pVatAry->getCount(); pos++) {
			THROW(SetFR(RowNumber, (long)(FRTAX_SALESTAX_ROW + pos + 1)));
			THROW(SetFR(FieldNumber, FRTAX_FIELD_TAXAMT));
			THROW(ExecFR(GetFieldStruct));
			THROW(SetFR(ValueOfFieldInteger, pVatAry->at(pos)));
			THROW(ExecFR(WriteTable));
			THROW(SetFR(FieldNumber, FRTAX_FIELD_TAXNAME));
			THROW(ExecFR(GetFieldStruct));
			{
				PPLoadString("vat", temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER).Space();
				(vat_str = temp_buf).Cat(fdiv100i(pVatAry->at(pos)), MKSFMTD(0, 2, NMBF_NOTRAILZ)).CatChar('%');
				THROW(SetFR(ValueOfFieldString, vat_str));
			}
			THROW(ExecFR(WriteTable));
		}
	}
	CATCH
		ok = LogLastError(); // @v11.6.9
	ENDCATCH
	CashierPassword = cshr_pssw;
	ASSIGN_PTR(pPrintTaxAction, print_tax_action);
	return ok;
}

int SCS_SHTRIHFRF::PrintSlipDoc(const CCheckPacket * pPack, const char * pFormatName, uint flags)
{
	int    ok = -1;
	SString  temp_buf;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
	THROW_INVARG(pPack);
	THROW(ConnectFR());
	if(IsDebugMode) {
		(temp_buf = "SCS_SHTRIHFRF::PrintSlipDoc entry").CatDiv(':', 2).CatEq("DeviceType", devtypeCombo).CatDiv(';', 2).CatEq("SlipFormat", pFormatName);
		PPLogMessage(PPFILNAM_SHTRIH_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_USER);
	}
	if(DeviceType == devtypeCombo && P_SlipFmt) {
		int   r = 1;
		SString line_buf;
		const SString format_name((pFormatName && pFormatName[0]) ? pFormatName : "SlipDocument");
		StringSet head_lines(reinterpret_cast<const char *>(&r));
		SlipDocCommonParam  sdc_param;
		THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
		if(r > 0) {
			int   str_num, print_head_lines = 0, fill_head_lines = 1;
			SlipLineParam sl_param;
			Flags |= sfPrintSlip;
			THROW(ExecFR(ClearSlipDocumentBuffer));
			for(str_num = 0, P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
				if(sl_param.Font > 1 && sl_param.Font <= DEF_FONT_NUMBER)
					line_buf.PadLeft(1, sl_param.Font).PadLeft(1, 27);
				if(print_head_lines) {
					for(uint i = 0; head_lines.get(&i, temp_buf);) {
						THROW(SetFR(StringForPrinting, temp_buf));
						THROW(SetFR(StringNumber, ++str_num));
						THROW(ExecFR(FillSlipDocumentWithUnfiscalInfo));
					}
					print_head_lines = 0;
				}
				else if(fill_head_lines)
					if(str_num < (int)sdc_param.HeadLines)
						head_lines.add(line_buf);
					else
						fill_head_lines = 0;
				THROW(SetFR(StringForPrinting, line_buf));
				THROW(SetFR(StringNumber, ++str_num));
				THROW(ExecFR(FillSlipDocumentWithUnfiscalInfo));
				if(str_num == sdc_param.PageLength) {
					THROW(SetFR(IsClearUnfiscalInfo, FALSE));
					THROW(SetFR(InfoType, 0L));
					THROW(ExecFR(PrintSlipDocument));
					print_head_lines = 1;
					str_num = 0;
				}
			}
			if(str_num) {
				THROW(SetFR(IsClearUnfiscalInfo, FALSE));
				THROW(SetFR(InfoType, 0L));
				THROW(ExecFR(PrintSlipDocument));
			}
			ok = 1;
		}
	}
	else if(IsDebugMode) {
		(temp_buf = "SCS_SHTRIHFRF::PrintSlipDoc printing skiped");
		PPLogMessage(PPFILNAM_SHTRIH_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_USER);
	}
	ErrCode = SYNCPRN_NO_ERROR;
	CATCH
		ok = LogLastError(); // @v11.6.9
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
			ok = (ExecFR(Beep), 0);
		}
	ENDCATCH
	Flags &= ~sfPrintSlip;
	return ok;
}

int SCS_SHTRIHFRF::PrintCheckCopy(const CCheckPacket * pPack, const char * pFormatName, uint flags)
{
	int     ok = 1;
	bool    is_format = false;
	SlipDocCommonParam  sdc_param;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
	THROW_INVARG(pPack);
	THROW(ConnectFR());
	THROW(SetFR(DocumentNumber, pPack->Rec.Code));
	if(P_SlipFmt) {
		int   r = 0;
		SString  line_buf;
		const SString format_name(isempty(pFormatName) ? ((flags & PRNCHK_RETURN) ? "CCheckRetCopy" : "CCheckCopy") : pFormatName);
		SlipLineParam sl_param;
		THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
		if(r > 0) {
			is_format = true;
			if(static_cast<long>(sdc_param.PageWidth) > CheckStrLen)
				WriteLogFile_PageWidthOver(format_name);
			RibbonParam = 0;
			CheckForRibbonUsing(sdc_param.RegTo);
			THROW(SetFR(DocumentName, sdc_param.Title));
			THROW(ExecFR(PrintDocumentTitle));
			for(P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
				CheckForRibbonUsing(sl_param.Flags);
				THROW(SetFR(StringForPrinting, line_buf.Trim((sl_param.Font > 1) ? CheckStrLen / 2 : CheckStrLen)));
				THROW(ExecFR((sl_param.Font > 1) ? PrintWideString : PrintString));
			}
		}
	}
	if(!is_format) {
		uint    pos;
		SString prn_str;
		SString temp_buf;
		CCheckLineTbl::Rec ccl;
		if(DeviceType == devtypeShtrih)
			THROW(SetFR(UseJournalRibbon, FALSE));
		{
			PPLoadText(PPTXT_CCFMT_CHKCOPY, temp_buf);
			temp_buf.ToUpper().Transf(CTRANSF_INNER_TO_OUTER);
			THROW(SetFR(DocumentName, temp_buf));
		}
		THROW(ExecFR(PrintDocumentTitle));
		{
			PPLoadText((flags & PRNCHK_RETURN) ? PPTXT_CCFMT_RETURN : PPTXT_CCFMT_SALE, temp_buf);
			temp_buf.ToUpper().Transf(CTRANSF_INNER_TO_OUTER);
			THROW(SetFR(StringForPrinting, temp_buf));
		}
		THROW(ExecFR(PrintString));
		for(pos = 0; pPack->EnumLines(&pos, &ccl);) {
			double  price = intmnytodbl(ccl.Price) - ccl.Dscnt;
			double  qtty  = R3(fabs(ccl.Quantity));
			GetGoodsName(ccl.GoodsID, prn_str);
			CutLongTail(prn_str.Transf(CTRANSF_INNER_TO_OUTER));
			THROW(SetFR(StringForPrinting, prn_str));
			THROW(ExecFR(PrintString));
			if(qtty != 1.0) {
				temp_buf.Z().Cat(qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatDiv('X', 1).Cat(price, SFMT_MONEY);
				THROW(SetFR(StringForPrinting, prn_str.Z().CatCharN(' ', CheckStrLen - temp_buf.Len()).Cat(temp_buf)));
				THROW(ExecFR(PrintString));
			}
			temp_buf.Z().CatEq(0, qtty * price, SFMT_MONEY);
			THROW(SetFR(StringForPrinting, prn_str.Z().CatCharN(' ', CheckStrLen - temp_buf.Len()).Cat(temp_buf)));
			THROW(ExecFR(PrintString));
		}
		THROW(PrintDiscountInfo(pPack, flags));
		THROW(SetFR(StringForPrinting, prn_str.Z().CatCharN('=', CheckStrLen)));
		THROW(ExecFR(PrintString));
		temp_buf.Z().CatEq(0, fabs(MONEYTOLDBL(pPack->Rec.Amount)), SFMT_MONEY);
		PPLoadText(PPTXT_CCFMT_TOTAL, prn_str);
		prn_str.ToUpper().Transf(CTRANSF_INNER_TO_OUTER);
		prn_str.CatCharN(' ', CheckStrLen / 2 - prn_str.Len() - temp_buf.Len()).Cat(temp_buf);
		THROW(SetFR(StringForPrinting, prn_str));
		THROW(ExecFR(PrintWideString));
	}
	THROW(LineFeed(6, TRUE, FALSE));
	THROW(Cut(1));
	ErrCode = SYNCPRN_NO_ERROR;
	CATCH
		ok = LogLastError(); // @v11.6.9
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
			ok = (ExecFR(Beep), 0);
		}
	ENDCATCH
	return ok;
}

int SCS_SHTRIHFRF::PrintReport(int withCleaning)
{
	int    ok = 1;
	int    mode = 0;
	long   cshr_pssw = 0;
	ResCode = RESCODE_NO_ERROR;
	THROW(ConnectFR());
	// Закрыть сессию можно только под паролем администратора
	cshr_pssw = CashierPassword;
	CashierPassword = AdmPassword;
	//
	Flags |= sfCheckOpened;
	THROW(ExecFR(GetECRStatus));
	THROW(GetFR(ECRMode, &mode));
	if(withCleaning) {
		THROW_PP(mode != FRMODE_CLOSE_SESS, PPERR_SYNCCASH_DAYCLOSED);
		THROW(ExecFR(PrintReportWithCleaning));
	}
	else {
		THROW(ExecFR(PrintReportWithoutCleaning));
	}
	THROW(Cut(withCleaning));
	CATCH
		ok = LogLastError(); // @v11.6.9
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
			ok = (ExecFR(Beep), 0);
		}
	ENDCATCH
	if(Flags & sfCheckOpened) {
		Flags &= ~sfCheckOpened;
		CashierPassword = cshr_pssw;
	}
	return ok;
}

/*virtual*/int SCS_SHTRIHFRF::OpenSession_(PPID sessID)
{
	int    ok = 1;
	ResCode = RESCODE_NO_ERROR;
	THROW(ConnectFR());
	THROW(ExecFR(OpenSession));
	//Arr_In.Z();
	//THROW(ExecPrintOper(DVCCMD_OPENSESSION, Arr_In, Arr_Out));
	CATCHZOK
	return ok;
}

int SCS_SHTRIHFRF::CloseSession(PPID sessID) { return PrintReport(1); }
int SCS_SHTRIHFRF::PrintXReport(const CSessInfo *) { return PrintReport(0); }

int SCS_SHTRIHFRF::PrintZReportCopy(const CSessInfo * pInfo)
{
	int  ok = -1;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
	THROW_INVARG(pInfo);
	THROW(ConnectFR());
	THROW(SetFR(DocumentNumber, pInfo->Rec.SessNumber));
	if(P_SlipFmt) {
		int   r = 0;
		SString line_buf;
		const SString format_name("ZReportCopy");
		SlipDocCommonParam  sdc_param;
		THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
		if(r > 0) {
			SlipLineParam sl_param;
			if(sdc_param.PageWidth > (uint)CheckStrLen)
				WriteLogFile_PageWidthOver(format_name);
			RibbonParam = 0;
			CheckForRibbonUsing(sdc_param.RegTo);
			THROW(SetFR(DocumentName, sdc_param.Title));
			THROW(ExecFR(PrintDocumentTitle));
			for(P_SlipFmt->InitIteration(pInfo); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
				CheckForRibbonUsing(sl_param.Flags);
				THROW(SetFR(StringForPrinting, line_buf.Trim((sl_param.Font > 1) ? CheckStrLen / 2 : CheckStrLen)));
				THROW(ExecFR((sl_param.Font > 1) ? PrintWideString : PrintString));
			}
			THROW(LineFeed(6, TRUE, FALSE));
			THROW(Cut(1));
		}
	}
	ErrCode = SYNCPRN_NO_ERROR;
	CATCH
		ok = LogLastError(); // @v11.6.9
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
			ok = (ExecFR(Beep), 0);
		}
	ENDCATCH
	return ok;
}

int SCS_SHTRIHFRF::PrintIncasso(double sum, int isIncome)
{
	int    ok = 1;
	ResCode = RESCODE_NO_ERROR;
	THROW(ConnectFR());
	Flags |= sfCheckOpened;
	THROW(SetFR(Summ1, sum));
	if(isIncome) {
		THROW(LineFeed(6, TRUE, FALSE));
		THROW(ExecFR(CashIncome));
	}
	else {
		int    is_cash;
		THROW(is_cash = CheckForCash(sum));
		THROW_PP(is_cash > 0, PPERR_SYNCCASH_NO_CASH);
		THROW(LineFeed(6, TRUE, FALSE));
		THROW(ExecFR(CashOutcome));
	}
	THROW(Cut(1));
	CATCH
		ok = LogLastError(); // @v11.6.9
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
			ok = (ExecFR(Beep), 0);
		}
	ENDCATCH
	Flags &= ~sfCheckOpened;
	return ok;
}

int SCS_SHTRIHFRF::GetPrintErrCode()
{
	return ErrCode;
}

int SCS_SHTRIHFRF::GetSummator(double * val)
{
	int    ok = 1;
	double cash_amt = 0.0;
	ResCode = RESCODE_NO_ERROR;
	THROW(ConnectFR());
	THROW(SetFR(RegisterNumber, CASH_AMOUNT_REG));
	THROW(ExecFR(GetCashReg));
	THROW(GetFR(ContentsOfCashRegister, &cash_amt));
	CATCH
		ok = LogLastError(); // @v11.6.9
		SetErrorMessage();
	ENDCATCH
	ASSIGN_PTR(val, cash_amt);
	return ok;
}

FR_INTRF * SCS_SHTRIHFRF::InitDriver()
{
	struct IfcEntry {
		const char * P_Symb;
		int    Id;
		uint * P_SuccState;
		uint   SuccStateMask;
	};
	#define IFC_ENTRY_SS(symb, fail_state, fail_state_mask) { #symb, symb, &fail_state, fail_state_mask }
	#define IFC_ENTRY(symb) { #symb, symb, 0, 0 }
	IfcEntry ifc_list[] = {
		IFC_ENTRY(ResultCode),
		IFC_ENTRY(ResultCodeDescription),
		IFC_ENTRY(Password),
		IFC_ENTRY(Beep),
		IFC_ENTRY(ComNumber),
		IFC_ENTRY(BaudRate),
		IFC_ENTRY(Timeout),
		IFC_ENTRY(GetExchangeParam),
		IFC_ENTRY(SetExchangeParam),
		IFC_ENTRY(Connect),
		IFC_ENTRY_SS(ServerConnect, ExtMethodsFlags, extmethfServerConnect), // @v12.0.3
		IFC_ENTRY_SS(ServerDisconnect, ExtMethodsFlags, extmethfServerDisconnect), // @v12.0.3
		IFC_ENTRY_SS(Connect2, ExtMethodsFlags, extmethfConnect2), // @v12.0.3
		IFC_ENTRY_SS(OpenSession, ExtMethodsFlags, extmethfOpenSession), // @v12.0.4
		IFC_ENTRY(AuthKey),                          // @v12.0.4
		IFC_ENTRY(ComputerName),                     // @v12.0.3 
		IFC_ENTRY(ServerConnected),                  // @v12.0.3 
		IFC_ENTRY(ServerVersion),                    // @v12.0.3
		IFC_ENTRY(Disconnect),
		IFC_ENTRY(Quantity),
		IFC_ENTRY(Price),
		IFC_ENTRY_SS(Summ1, PayTypeRegFlags, (1U <<  1)),
		IFC_ENTRY_SS(Summ2, PayTypeRegFlags, (1U <<  2)),
		IFC_ENTRY_SS(Summ3, PayTypeRegFlags, (1U <<  3)),
		IFC_ENTRY_SS(Summ4, PayTypeRegFlags, (1U <<  4)),
		IFC_ENTRY_SS(Summ5, PayTypeRegFlags, (1U <<  5)),
		IFC_ENTRY_SS(Summ6, PayTypeRegFlags, (1U <<  6)),
		IFC_ENTRY_SS(Summ7, PayTypeRegFlags, (1U <<  7)),
		IFC_ENTRY_SS(Summ8, PayTypeRegFlags, (1U <<  8)),
		IFC_ENTRY_SS(Summ9, PayTypeRegFlags, (1U <<  9)),
		IFC_ENTRY_SS(Summ10, PayTypeRegFlags, (1U << 10)),
		IFC_ENTRY_SS(Summ11, PayTypeRegFlags, (1U << 11)),
		IFC_ENTRY_SS(Summ12, PayTypeRegFlags, (1U << 12)),
		IFC_ENTRY_SS(Summ13, PayTypeRegFlags, (1U << 13)),
		IFC_ENTRY_SS(Summ14, PayTypeRegFlags, (1U << 14)),
		IFC_ENTRY_SS(Summ15, PayTypeRegFlags, (1U << 15)),
		IFC_ENTRY_SS(Summ16, PayTypeRegFlags, (1U << 16)),
		IFC_ENTRY_SS(CloseCheckEx,   ExtMethodsFlags, extmethfCloseCheckEx),
		IFC_ENTRY_SS(FNCloseCheckEx, ExtMethodsFlags, extmethfFNCloseCheckEx), // @v11.7.2
		// @v11.2.11 {
		//if(ExtMethodsFlags & extmethfCloseCheckEx)
		IFC_ENTRY(TaxType), // must be after CloseCheckEx
		// } @v11.2.11
		IFC_ENTRY(Tax1),
		IFC_ENTRY(Tax2),
		IFC_ENTRY(Tax3),
		IFC_ENTRY(Tax4),
		IFC_ENTRY(StringForPrinting),
		IFC_ENTRY(UseReceiptRibbon),
		IFC_ENTRY(UseJournalRibbon),
		IFC_ENTRY(PrintString),
		IFC_ENTRY(PrintWideString),
		IFC_ENTRY(StringQuantity),
		IFC_ENTRY(FeedDocument),
		IFC_ENTRY(DocumentName),
		IFC_ENTRY(DocumentNumber),
		IFC_ENTRY(PrintDocumentTitle),
		IFC_ENTRY(CheckType),
		IFC_ENTRY(OpenCheck),
		IFC_ENTRY(Sale),
		IFC_ENTRY(ReturnSale),
		IFC_ENTRY(CloseCheck),
		IFC_ENTRY(CutCheck),
		IFC_ENTRY(DrawerNumber),
		IFC_ENTRY(OpenDrawer),
		IFC_ENTRY(TableNumber),
		IFC_ENTRY(RowNumber),
		IFC_ENTRY(FieldNumber),
		IFC_ENTRY(GetFieldStruct),
		IFC_ENTRY(ReadTable),
		IFC_ENTRY(WriteTable),
		IFC_ENTRY(ValueOfFieldInteger),
		IFC_ENTRY(ValueOfFieldString),
		IFC_ENTRY(RegisterNumber),
		IFC_ENTRY(GetOperationReg),
		IFC_ENTRY(ContentsOfOperationRegister),
		IFC_ENTRY(GetCashReg),
		IFC_ENTRY(ContentsOfCashRegister),
		IFC_ENTRY(GetECRStatus),
		IFC_ENTRY(ECRMode),
		IFC_ENTRY(ECRModeDescription),
		IFC_ENTRY(ECRAdvancedMode),
		IFC_ENTRY(ReceiptRibbonOpticalSensor),
		IFC_ENTRY(JournalRibbonOpticalSensor),
		IFC_ENTRY(ContinuePrint),
		IFC_ENTRY(CancelCheck),
		IFC_ENTRY(PrintReportWithCleaning),
		IFC_ENTRY(PrintReportWithoutCleaning),
		IFC_ENTRY(UModel),
		IFC_ENTRY(UMajorProtocolVersion),
		IFC_ENTRY(UMinorProtocolVersion),
		IFC_ENTRY(GetDeviceMetrics),
		IFC_ENTRY(CashIncome),
		IFC_ENTRY(CashOutcome),
		IFC_ENTRY(ClearSlipDocumentBuffer),
		IFC_ENTRY(FillSlipDocumentWithUnfiscalInfo),
		IFC_ENTRY(StringNumber),
		IFC_ENTRY(PrintSlipDocument),
		IFC_ENTRY(IsClearUnfiscalInfo),
		IFC_ENTRY(InfoType),
		IFC_ENTRY(EKLZIsPresent),
		IFC_ENTRY(IsEKLZOverflow),
		IFC_ENTRY(FMOverflow),
		IFC_ENTRY(FreeRecordInFM),
		IFC_ENTRY(IsFM24HoursOver),
		IFC_ENTRY(IsDrawerOpen),
		IFC_ENTRY(Department),
		IFC_ENTRY(ECRModeStatus),
		IFC_ENTRY(JournalRibbonIsPresent),
		IFC_ENTRY(ReceiptRibbonIsPresent),
		IFC_ENTRY(OutputReceipt),
		IFC_ENTRY(ReceiptOutputType),
		IFC_ENTRY(PrintBarcodeGraph),
		IFC_ENTRY(BarcodeType),
		IFC_ENTRY(BarCode),
		IFC_ENTRY(FirstLineNumber),
		IFC_ENTRY(LineNumber),
		IFC_ENTRY(FNOperation),        //
		IFC_ENTRY(PaymentTypeSign),    // Признак способа расчета
		IFC_ENTRY(PaymentItemSign),    // Признак предмета расчета 
		IFC_ENTRY(FNSendItemCodeData), //
		IFC_ENTRY(MarkingType),        //
		IFC_ENTRY(GTIN),               //
		IFC_ENTRY(SerialNumber),       //
		IFC_ENTRY(FNBeginSTLVTag), //
		IFC_ENTRY(TagID),          //
		IFC_ENTRY(TagNumber),      //
		IFC_ENTRY(TagType),        //
		IFC_ENTRY(TagValueStr),    //
		IFC_ENTRY(FNAddTag),       //
		IFC_ENTRY(FNSendSTLVTag),  //
		IFC_ENTRY(FNAcceptMarkingCode),  // @v12.0.4
		IFC_ENTRY(FNDeclineMarkingCode), // @v12.0.4
		IFC_ENTRY_SS(FNCheckItemBarcode,     ExtMethodsFlags, extmethfFNCheckItemBarcode),     // @v11.6.6 / !
		IFC_ENTRY_SS(FNCheckItemBarcode2,    ExtMethodsFlags, extmethfFNCheckItemBarcode2),    // @v11.6.6 / !
		IFC_ENTRY_SS(BarcodeHex,             ExtMethodsFlags, extmethfBarcodeHex),             // @v11.6.6
		IFC_ENTRY_SS(ItemStatus,             ExtMethodsFlags, extmethfItemStatus),             // @v11.6.6
		IFC_ENTRY_SS(CheckItemMode,          ExtMethodsFlags, extmethfCheckItemMode),          // @v11.6.6
		IFC_ENTRY_SS(TLVDataHex,             ExtMethodsFlags, extmethfTLVDataHex),             // @v11.6.6
		IFC_ENTRY_SS(CheckItemLocalResult,   ExtMethodsFlags, extmethfCheckItemLocalResult),   // @v11.6.6
		IFC_ENTRY_SS(CheckItemLocalError,    ExtMethodsFlags, extmethfCheckItemLocalError),    // @v11.6.6 
		IFC_ENTRY_SS(MarkingType2,           ExtMethodsFlags, extmethfMarkingType2),           // @v11.6.6 / !
		IFC_ENTRY_SS(KMServerErrorCode,      ExtMethodsFlags, extmethfKMServerErrorCode),      // @v11.6.6
		IFC_ENTRY_SS(KMServerCheckingStatus, ExtMethodsFlags, extmethfKMServerCheckingStatus), // @v11.6.6 
		IFC_ENTRY_SS(DivisionalQuantity,     ExtMethodsFlags, extmethfDivisionalQuantity),     // @v11.6.6 / !
		IFC_ENTRY_SS(Numerator,              ExtMethodsFlags, extmethfNumerator),              // @v11.6.6 / !
		IFC_ENTRY_SS(Denominator,            ExtMethodsFlags, extmethfDenominator),            // @v11.6.6 / !
		IFC_ENTRY_SS(FNSendItemBarcode,      ExtMethodsFlags, extmethfFNSendItemBarcode), // @v12.0.4
	};

//31/03/23 10:42:38	master	Ошибка инициализации COM-метода или свойства 'FNCheckItemBarcode2': Не определен домен данных в конфигурации глобального обмена
//31/03/23 10:42:38	master	Ошибка инициализации COM-метода или свойства 'MarkingType2': Не определен домен данных в конфигурации глобального обмена
//31/03/23 10:42:38	master	Ошибка инициализации COM-метода или свойства 'DivisionalQuantity': Не определен домен данных в конфигурации глобального обмена
//31/03/23 10:42:38	master	Ошибка инициализации COM-метода или свойства 'Numerator': Не определен домен данных в конфигурации глобального обмена
//31/03/23 10:42:38	master	Ошибка инициализации COM-метода или свойства 'Denominator': Не определен домен данных в конфигурации глобального обмена

	#undef IFC_ENTRY
	int    ok = 1;
	SString fmt_buf;
	SString msg_buf;
	SString temp_buf;
	SCS_SHTRIHFRF::PayTypeRegFlags = 0;
	FR_INTRF * p_drv = new ComDispInterface;
	THROW_MEM(p_drv);
	THROW_SL(p_drv->Init("AddIn.DrvFR"));
	for(uint i = 0; i < SIZEOFARRAY(ifc_list); i++) {
		const auto & r_entry = ifc_list[i];
		assert(!isempty(r_entry.P_Symb));
		assert(r_entry.Id >= 0);
		if(r_entry.Id != TaxType || ExtMethodsFlags & extmethfCloseCheckEx) {
			int r = p_drv->AssignIDByName(r_entry.P_Symb, r_entry.Id);
			if(r > 0) {
				if(r_entry.P_SuccState && r_entry.SuccStateMask)
					*r_entry.P_SuccState |= r_entry.SuccStateMask;
			}
			else {
				PPLoadText(PPTXT_LOG_SHTRIH_INITIFCMETHFAULT, fmt_buf);
				msg_buf.Printf(fmt_buf, r_entry.P_Symb);
				PPGetLastErrorMessage(1, temp_buf);
				if(temp_buf.NotEmpty())
					msg_buf.CatDiv(':', 2).Cat(temp_buf);
				//PPTXT_LOG_SHTRIH_INITIFCMETHFAULT         "Ошибка инициализации COM-метода или свойства '%s'"         "Ошибка инициализации COM-метода или свойства '%s'"
				PPLogMessage(PPFILNAM_SHTRIH_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
				if(!r_entry.P_SuccState && !r_entry.SuccStateMask)
					ok = 0;
			}
		}
	}
	CATCHZOK
	if(!ok) {
		PPLogMessage(PPFILNAM_SHTRIH_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
		ZDELETE(p_drv);
	}
	return p_drv;
}

int SCS_SHTRIHFRF::AnnulateCheck()
{
	int    ok = -1;
	int    mode = 0;
	int    adv_mode = 0;
	int    cut = 0;
	int    dont_use_cont_prn = 0;
	PPIniFile ini_file;
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_SHTRIHFRNOUSECONTPRN, &dont_use_cont_prn);
	THROW(ExecFR(GetECRStatus));
	if(!dont_use_cont_prn) {
		// Проверка на незавершенную печать
		THROW(GetFR(ECRAdvancedMode, &adv_mode));
		if(adv_mode == PRNMODE_AFTER_NO_PAPER) {
			Flags |= sfCheckOpened;
			THROW(ExecFR(ContinuePrint));
			do {
				THROW(ExecFR(GetECRStatus));
				THROW(GetFR(ECRAdvancedMode, &adv_mode));
			} while(adv_mode == PRNMODE_PRINT);
			Flags &= ~sfCheckOpened;
			cut = 1;
		}
	}
	// Проверка на наличие открытого чека, который надо аннулировать
	THROW(GetFR(ECRMode, &mode));
	if(mode == FRMODE_OPEN_CHECK) {
		Flags |= sfCheckOpened | sfCancelled;
		PPMessage(mfInfo|mfOK, PPINF_SHTRIHFR_CHK_ANNUL);
		THROW(ExecFR(CancelCheck));
		Flags &= ~(sfCheckOpened | sfCancelled);
		cut = 1;
		ok = 1;
	}
	if(cut && oneof3(DeviceType, devtypeShtrih, devtypeCombo, devtypeMini) && !(Flags & sfDontUseCutter))
		THROW(ExecFR(CutCheck));
	CATCH
		ok = LogLastError(); // @v11.6.9
	ENDCATCH
	return ok;
}

int SCS_SHTRIHFRF::ReadStringFromTbl(int tblNum, int rowNum, int fldNum, SString & rStr)
{
	int    ok = 1;
	char   buf[256];
	THROW(SetFR(TableNumber, tblNum));
	THROW(SetFR(RowNumber,   rowNum));
	THROW(SetFR(FieldNumber, fldNum));
	THROW(ExecFR(GetFieldStruct));
	THROW(ExecFR(ReadTable));
	THROW(GetFR(ValueOfFieldString, buf, sizeof(buf)));
	CATCH
		buf[0] = 0;
		ok = 0;
	ENDCATCH
	rStr = buf;
	return ok;
}

int SCS_SHTRIHFRF::ReadValueFromTbl(int tblNum, int rowNum, int fldNum, long * pValue)
{
	int   ok = 1;
	long  val = 0;
	THROW(SetFR(TableNumber, tblNum));
	THROW(SetFR(RowNumber,   rowNum));
	THROW(SetFR(FieldNumber, fldNum));
	THROW(ExecFR(GetFieldStruct));
	THROW(ExecFR(ReadTable));
	THROW(GetFR(ValueOfFieldInteger, &val));
	CATCHZOK
	ASSIGN_PTR(pValue, val);
	return ok;
}

int SCS_SHTRIHFRF::WriteStringToTbl(int tblNum, int rowNum, int fldNum, const char * pStr)
{
	int   ok = 1;
	THROW(SetFR(TableNumber, tblNum));
	THROW(SetFR(RowNumber,   rowNum));
	THROW(SetFR(FieldNumber, fldNum));
	THROW(ExecFR(GetFieldStruct));
	THROW(SetFR(ValueOfFieldString, pStr));
	THROW(ExecFR(WriteTable));
	CATCH
		ok = (WriteLogFileToWriteTblErr(tblNum, rowNum, fldNum, pStr), 0);
	ENDCATCH
	return ok;
}

int SCS_SHTRIHFRF::WriteValueToTbl(int tblNum, int rowNum, int fldNum, long value)
{
	int     ok = 1;
	SString str_val;
	THROW(SetFR(TableNumber, tblNum));
	THROW(SetFR(RowNumber,   rowNum));
	THROW(SetFR(FieldNumber, fldNum));
	THROW(ExecFR(GetFieldStruct));
	THROW(SetFR(ValueOfFieldInteger, value));
	THROW(ExecFR(WriteTable));
	CATCH
		ok = (WriteLogFileToWriteTblErr(tblNum, rowNum, fldNum, str_val.Cat(value)), 0);
	ENDCATCH
	return ok;
}

int SCS_SHTRIHFRF::SetupTables()
{
	int    ok = 1;
	long   cshr_pssw;
	SString temp_buf;//cshr_name;
	SString cshr_str;
	PPSyncCashSession::GetCurrentUserName(temp_buf);
	PPLoadString("cashier", cshr_str);
	cshr_str.Space().Cat(temp_buf).Transf(CTRANSF_INNER_TO_OUTER);
	// Получаем пароль кассира
	THROW(ReadValueFromTbl(FRCASHIER_TBL, FRCASHIER_ROW, FRCASHIER_FIELD_PSSW, &cshr_pssw));
	// Приходится проверять незакрытый чек, иначе не удастся записать имя кассира
	CashierPassword = cshr_pssw;
	THROW(AnnulateCheck());
	// Записываем в таблицы настройки (только под паролем администратора)
	CashierPassword = AdmPassword;
	// Имя кассира
	// @v11.6.12 (@development) THROW(WriteStringToTbl(FRCASHIER_TBL, FRCASHIER_ROW, FRCASHIER_FIELD_NAME, cshr_str));
	// Имя администратора
	THROW(WriteStringToTbl(FRCASHIER_TBL, FRCASHIER_ADMINROW, FRCASHIER_FIELD_NAME, AdmName.NotEmpty() ? AdmName : cshr_str));
	// Настройки режима работы кассы
	//    Установить автоматическое обнуление наличности
	THROW(WriteValueToTbl(FRCASHMODE_TBL, FRCASHMODE_ROW, FRCASHMODE_AUTOCASHNULL, 1));
	//    Установить открывание денежного ящика
	//THROW(WriteValueToTbl(FRCASHMODE_TBL, FRCASHMODE_ROW, (DeviceType == devtypeCombo ||
	//	DeviceType == devtypeMini) ? COMBOCASHMODE_OPENDRAWER : FRCASHMODE_OPENDRAWER, 1);
	//    Нет автоматической отрезки чека
	THROW(WriteValueToTbl(FRCASHMODE_TBL, FRCASHMODE_ROW,
		(DeviceType == devtypeCombo || DeviceType == devtypeMini) ? COMBOCASHMODE_CUTTING : FRCASHMODE_CUTTING, 0));
	/* @v9.7.4 Проблемы с модернизированными аппаратами
	//    Установить использование весовых датчиков
	THROW(WriteValueToTbl(FRCASHMODE_TBL, FRCASHMODE_ROW, oneof2(DeviceType, devtypeCombo, devtypeMini) ?
		COMBOCASHMODE_USEWGHTSENSOR : FRCASHMODE_USEWGHTSENSOR, BIN(Flags & sfUseWghtSensor)));
	*/
	//    Установить автоматический перевод времени
	//THROW(WriteValueToTbl(FRCASHMODE_TBL, FRCASHMODE_ROW, (DeviceType == devtypeCombo ||
	//	DeviceType == devtypeMini) ? COMBOCASHMODE_AUTOTIMING : FRCASHMODE_AUTOTIMING, 1));
	//    Не сохранять строки в буфере чека
	/* @v9.7.5 В новых аппаратах путаница с нумерацией полей. Просто ничего не будем писать в это поле.
	if(oneof2(DeviceType, devtypeCombo, devtypeMini)) {
		THROW(WriteValueToTbl(FRCASHMODE_TBL, FRCASHMODE_ROW, COMBOCASHMODE_SAVESTRING, 0));
	}
	else {
		THROW(WriteValueToTbl(FRCASHMODE_TBL, FRCASHMODE_ROW, FRCASHMODE_SAVESTRING, 0));
	}
	*/
	/* Современные версии Штриха запрещают редактирование таблицы перевода времени
	THROW(ReadValueFromTbl(FRCASHMODE_TBL, FRCASHMODE_ROW, (DeviceType == devtypeCombo ||
		DeviceType == devtypeMini) ? COMBOCASHMODE_AUTOTIMING : FRCASHMODE_AUTOTIMING, &auto_timing));
	// Установка автоматического перевода времени
	if(!auto_timing) {
		int    i, k, d, m, y;
		SString str_year;
		LDATE  cur_dt = getcurdate_();
		decodedate(&d, &m, &y, &cur_dt);
		for(i = 1; i <= FRTIMESWITCH_MAXROW; i++) {
			long    td, tm, ty;
			THROW(ReadStringFromTbl(FRTIMESWITCH_TBL, i, FRTIMESWITCH_SEASON, str_year));
			ty = str_year.ToLong();
			if(ty >= y) {
				THROW(ReadValueFromTbl(FRTIMESWITCH_TBL, i, FRTIMESWITCH_DAY, &td));
				THROW(ReadValueFromTbl(FRTIMESWITCH_TBL, i, FRTIMESWITCH_MONTH, &tm));
				if(diffdate(encodedate(td, tm, ty), cur_dt) >= 0)
					break;
			}
		}
		for(k = 1; k < i; k++) {
			THROW(WriteValueToTbl(FRTIMESWITCH_TBL, k, FRTIMESWITCH_ALLOW, 0));
		}
		THROW(WriteValueToTbl(FRCASHMODE_TBL, FRCASHMODE_ROW, (DeviceType == devtypeCombo ||
			DeviceType == devtypeMini) ? COMBOCASHMODE_AUTOTIMING : FRCASHMODE_AUTOTIMING, 1));
	}
	*/
	//
	// Наименования типов оплат
	//
	{
		PPLoadText(PPTXT_CCFMT_CASHLESSPAYM, temp_buf); // БЕЗНАЛИЧНАЯ ОПЛАТА
		temp_buf.ToUpper().Transf(CTRANSF_INNER_TO_OUTER);
		THROW(WriteStringToTbl(FRPAYMTYPE_TBL, 2, FRPAYMTYPE_NAME, temp_buf));
	}
	THROW(WriteStringToTbl(FRPAYMTYPE_TBL, 3, FRPAYMTYPE_NAME, ""));
	THROW(WriteStringToTbl(FRPAYMTYPE_TBL, 4, FRPAYMTYPE_NAME, ""));
	// Формат чека
	if(!(Flags & sfOldShtrih)) {
		THROW(ReadValueFromTbl(FRFORMAT_TBL, FRFORMAT_ROW, FRFORMAT_FIELD_SIZE, &CheckStrLen));
	}
	CATCHZOK
	CashierPassword = cshr_pssw;
	return ok;
}

int SCS_SHTRIHFRF::ConnectFR()
{
	int    ok = -1;
	const char * p_server_name = "localhost"; // @v12.0.3
	if(Flags & sfConnected) {
		if(RefToIntrf > 1) {
			if(sstreqi_ascii(Port, "server")) { 
				THROW(ExecFR(ServerDisconnect));
				THROW(SetFR(ComputerName, p_server_name));
				THROW(ExecFR_WithoutPassword(ServerConnect));
			}
			else {
				THROW(ExecFR(Disconnect));
				THROW(SetFR(ComNumber, Handle));
				THROW(ExecFR(Connect));
			}
			THROW(AnnulateCheck());
		}
	}
	else {
		int    model_type = 0;
		int    major_prot_ver = 0;
		int    minor_prot_ver = 0;
		int    not_use_wght_sensor = 0;
		int    temp_int = 0;
		SString buf, buf1;
		PPIniFile ini_file;
		Flags &= ~sfUseFnMethods;
		SCardPaymEntryN = ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_SHTRIHFRSCARDPAYMENTRY, &temp_int) ? inrangeordefault(temp_int, 1, 16, 0) : 0;
		THROW_PP(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_SHTRIHFRPASSWORD, buf) > 0, PPERR_SHTRIHFRADMPASSW);
		buf.Divide(',', buf1, AdmName);
		CashierPassword = AdmPassword = buf1.ToLong();
		AdmName.Strip().Transf(CTRANSF_INNER_TO_OUTER);
		// @v12.0.3 {
		if(sstreqi_ascii(Port, "server")) { 
			// ServerConnect
			int    server_connected = 0;
			char   server_version[256];
			server_version[0] = 0;
			P_DrvFRIntrf->GetProperty(ServerConnected, &server_connected);
			THROW(SetFR(ComputerName, p_server_name));
			THROW(ExecFR_WithoutPassword(ServerConnect));
			P_DrvFRIntrf->GetProperty(ServerConnected, &server_connected);
			P_DrvFRIntrf->GetProperty(ServerVersion, server_version, sizeof(server_version));
			//ExecFR(Beep);
			Flags |= sfConnected;
			ConnectionMode = connmodeServer;
		} // } @v12.0.3 
		else {
			//#define DEF_BAUD_RATE              2   
			//#define MAX_BAUD_RATE              6   // Для Штрих-ФР max скорость обмена 115200 бод
			const int __def_baud_rate = 2; // Для Штрих-ФР скорость обмена по умолчанию 9600 бод
			const int __max_baud_rate = 6; // Для Штрих-ФР max скорость обмена 115200 бод

			int    baud_rate;
			long   def_baud_rate = __def_baud_rate;
			int    def_timeout = -1;
			/*
				Тайм-аут приема байта. Тайм-аут приема байта нелинейный. Диапазон допустимых значений
				[0…255] распадается на три диапазона:
				-- в диапазоне [0…150] каждая единица соответствует 1 мс, т.е. данным диапазоном задаются значения тайм-аута от 0 до 150 мс;
				-- в диапазоне [151…249] каждая единица соответствует 150 мс, т.е. данным диапазоном задаются значения тайм-аута от 300 мс до 15 сек;
				-- в диапазоне [250…255] каждая единица соответствует 15 сек, т.е. данным диапазоном задаются значения тайм-аута от 30 сек до 105 сек.
			*/
			if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_SHTRIHFRCONNECTPARAM, buf) > 0) {
				SString  buf2;
				if(buf.Divide(',', buf1, buf2) > 0)
					def_timeout = buf2.ToLong();
				def_baud_rate = buf1.ToLong();
				if(def_baud_rate > __max_baud_rate)
					def_baud_rate = __def_baud_rate;
			}
			THROW_PP(PortType == COM_PORT, PPERR_SYNCCASH_INVPORT);
			THROW(SetFR(ComNumber, Handle));
			if(def_timeout >= 0 && def_timeout < MAX_TIMEOUT) {
				// @v11.6.12 (@development) THROW(SetFR(Timeout, def_timeout));
			}
			THROW((ok = ExecFR(Connect)) > 0 || ResCode == RESCODE_NO_CONNECTION);
			for(baud_rate = 0; !ok && baud_rate <= __max_baud_rate; baud_rate++) {
				THROW(SetFR(BaudRate, baud_rate));
				THROW((ok = ExecFR(Connect)) > 0 || ResCode == RESCODE_NO_CONNECTION);
			}
			THROW(ok > 0);
			Flags |= sfConnected;
			ConnectionMode = connmodeCom; // @v12.0.3
			THROW(GetFR(BaudRate, &baud_rate));
			if(baud_rate != def_baud_rate) {
				THROW(SetFR(BaudRate, def_baud_rate));
				THROW(ExecFR(SetExchangeParam));
				THROW(ExecFR(Disconnect));
				THROW(ExecFR(Connect));
			}
		}
		THROW(ExecFR(GetDeviceMetrics) > 0);
		THROW(GetFR(UModel, &model_type));
		THROW(GetFR(UMajorProtocolVersion, &major_prot_ver));
		THROW(GetFR(UMinorProtocolVersion, &minor_prot_ver));
		ProtocolVer.Set(major_prot_ver, minor_prot_ver, 0); // @v11.7.11
		if(oneof2(model_type, SHTRIH_FRF, SHTRIH_FRK))
			DeviceType = devtypeShtrih;
		else if(oneof2(model_type, SHTRIH_MINI_FRK, SHTRIH_MINI_FRK_V2))
			DeviceType = devtypeMini;
		else if(oneof2(model_type, SHTRIH_COMBO_FRK, SHTRIH_COMBO_FRK_V2))
			DeviceType = devtypeCombo;
		SETFLAG(Flags, sfOldShtrih, (DeviceType == devtypeShtrih) && major_prot_ver < 2 && minor_prot_ver < 2);
		THROW(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_SHTRIHFRNOTUSEWEIGHTSENSOR, &not_use_wght_sensor));
		SETFLAG(Flags, sfUseWghtSensor, !not_use_wght_sensor);
		{
			int  use_fr_meths = 0;
			THROW(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_SHTRIHFRUSEFNMETHODS, &use_fr_meths));
			SETFLAG(Flags, sfUseFnMethods, use_fr_meths == 1); 
		}
		THROW(SetupTables());
	}
	CATCH
		ok = LogLastError(); // @v11.6.9
		if(Flags & sfConnected) {
			SetErrorMessage();
			ExecFR(Disconnect);
		}
		else {
			Flags |= sfConnected;
			SetErrorMessage();
		}
		Flags &= ~sfConnected;
	ENDCATCH
	return ok;
}

bool SCS_SHTRIHFRF::SetFR(int id, int iVal) 
{ 
	bool   ok = 1;
	THROW_PP(P_DrvFRIntrf, PPERR_SHTRIHFRIFCNOTINITED);
	LogDebug("SetProperty", id, 0); // @v11.6.12
	THROW_PP_S(P_DrvFRIntrf->SetProperty(id, iVal) > 0, PPERR_SHTRIHFRSETPROPFAULT, P_DrvFRIntrf->GetNameByID(id, SLS.AcquireRvlStr()));
	CATCHZOK
	return ok;
}

bool SCS_SHTRIHFRF::SetFR(int id, long lVal) 
{ 
	bool   ok = 1;
	THROW_PP(P_DrvFRIntrf, PPERR_SHTRIHFRIFCNOTINITED);
	LogDebug("SetProperty", id, 0); // @v11.6.12
	THROW_PP_S(P_DrvFRIntrf->SetProperty(id, lVal) > 0, PPERR_SHTRIHFRSETPROPFAULT, P_DrvFRIntrf->GetNameByID(id, SLS.AcquireRvlStr()));
	CATCHZOK
	return ok;
}

bool SCS_SHTRIHFRF::SetFR(int id, double dVal) 
{ 
	bool   ok = 1;
	THROW_PP(P_DrvFRIntrf, PPERR_SHTRIHFRIFCNOTINITED);
	LogDebug("SetProperty", id, 0); // @v11.6.12
	THROW_PP_S(P_DrvFRIntrf->SetProperty(id, dVal) > 0, PPERR_SHTRIHFRSETPROPFAULT, P_DrvFRIntrf->GetNameByID(id, SLS.AcquireRvlStr()));
	CATCHZOK
	return ok;
}

bool SCS_SHTRIHFRF::SetFR(int id, const char * pStrVal) 
{ 
	bool   ok = 1;
	THROW_PP(P_DrvFRIntrf, PPERR_SHTRIHFRIFCNOTINITED);
	LogDebug("SetProperty", id, 0); // @v11.6.12
	THROW_PP_S(P_DrvFRIntrf->SetProperty(id, pStrVal) > 0, PPERR_SHTRIHFRSETPROPFAULT, P_DrvFRIntrf->GetNameByID(id, SLS.AcquireRvlStr()));
	CATCHZOK
	return ok;
}

bool SCS_SHTRIHFRF::GetFR(int id, int * pBuf) 
{ 
	bool   ok = 1;
	THROW_PP(P_DrvFRIntrf, PPERR_SHTRIHFRIFCNOTINITED);
	LogDebug("GetProperty", id, 0); // @v11.6.12
	THROW_PP_S(P_DrvFRIntrf->GetProperty(id, pBuf) > 0, PPERR_SHTRIHFRGETPROPFAULT, P_DrvFRIntrf->GetNameByID(id, SLS.AcquireRvlStr())); 
	// @v11.6.12 {
	if(IsDebugMode && pBuf) {
		SString temp_buf;
		if(id == ECRMode) {
			LogDebug("GetProperty(ECRMode) result", -1, temp_buf.Z().Cat(*pBuf)); 
		}
		else if(id == ECRAdvancedMode) {
			LogDebug("GetProperty(ECRAdvancedMode) result", -1, temp_buf.Z().Cat(*pBuf)); 
		}
		else if(id == UMajorProtocolVersion) { // @v11.7.11
			LogDebug("GetProperty(UMajorProtocolVersion) result", -1, temp_buf.Z().Cat(*pBuf)); 
		}
		else if(id == UMinorProtocolVersion) { // @v11.7.11
			LogDebug("GetProperty(UMajorProtocolVersion) result", -1, temp_buf.Z().Cat(*pBuf)); 
		}
	}
	// } @v11.6.12 
	CATCHZOK
	return ok;
}

bool SCS_SHTRIHFRF::GetFR(int id, long * pBuf) 
{ 
	bool   ok = 1;
	THROW_PP(P_DrvFRIntrf, PPERR_SHTRIHFRIFCNOTINITED);
	LogDebug("GetProperty", id, 0); // @v11.6.12
	THROW_PP_S(P_DrvFRIntrf->GetProperty(id, pBuf) > 0, PPERR_SHTRIHFRGETPROPFAULT, P_DrvFRIntrf->GetNameByID(id, SLS.AcquireRvlStr())); 
	CATCHZOK
	return ok;
}

bool SCS_SHTRIHFRF::GetFR(int id, double * pBuf) 
{ 
	bool   ok = 1;
	THROW_PP(P_DrvFRIntrf, PPERR_SHTRIHFRIFCNOTINITED);
	LogDebug("GetProperty", id, 0); // @v11.6.12
	THROW_PP_S(P_DrvFRIntrf->GetProperty(id, pBuf) > 0, PPERR_SHTRIHFRGETPROPFAULT, P_DrvFRIntrf->GetNameByID(id, SLS.AcquireRvlStr())); 
	CATCHZOK
	return ok;
}

bool SCS_SHTRIHFRF::GetFR(int id, char * pBuf, size_t bufLen) 
{ 
	bool   ok = 1;
	THROW_PP(P_DrvFRIntrf, PPERR_SHTRIHFRIFCNOTINITED);
	LogDebug("GetProperty", id, 0); // @v11.6.12
	THROW_PP_S(P_DrvFRIntrf->GetProperty(id, pBuf, bufLen) > 0, PPERR_SHTRIHFRGETPROPFAULT, P_DrvFRIntrf->GetNameByID(id, SLS.AcquireRvlStr())); 
	CATCHZOK
	return ok;
}

int SCS_SHTRIHFRF::GetResultCode(int methID)
{
	const  int func_without_retcode_checking[] = { GetECRStatus, Beep, Connect, OpenCheck, FNOperation, CloseCheckEx, CloseCheck,
		FeedDocument, FNCheckItemBarcode, FNCheckItemBarcode2, 
		FNAcceptMarkingCode, FNDeclineMarkingCode, OpenSession, FNSendItemCodeData, PrintBarcodeGraph };
	int    ok = 1;
	{
		if(methID == ServerConnect) {
			int    server_connected = 0;
			LogDebug("GetProperty", ServerConnected, 0); // @v11.6.12
			THROW_PP_S(P_DrvFRIntrf->GetProperty(ServerConnected, &server_connected) > 0, PPERR_SHTRIHFRGETPROPFAULT, P_DrvFRIntrf->GetNameByID(ServerConnected, SLS.AcquireRvlStr())); 
			if(server_connected) {
				ResCode = RESCODE_NO_ERROR;
			}
			else {
				ResCode = RESCODE_NO_CONNECTION;
			}
		}
		else {
			bool do_check_result_code = true;
			for(uint i = 0; do_check_result_code && i < SIZEOFARRAY(func_without_retcode_checking); i++) {
				if(methID == func_without_retcode_checking[i])
					do_check_result_code = false;
			}
			if(do_check_result_code) {
				LogDebug("GetProperty", ResultCode, 0); // @v11.6.12
				THROW_PP_S(P_DrvFRIntrf->GetProperty(ResultCode, &ResCode) > 0, PPERR_SHTRIHFRGETPROPFAULT, P_DrvFRIntrf->GetNameByID(ResultCode, SLS.AcquireRvlStr())); 
			}
			else
				ResCode = RESCODE_NO_ERROR;
		}
	}
	if(ResCode != RESCODE_NO_ERROR) {
		SString addendum_msg_buf;
		addendum_msg_buf.Cat(P_DrvFRIntrf->GetNameByID(methID, SLS.AcquireRvlStr())).Space().CatEq("ResCode", ResCode);
		THROW_PP_S(ResCode == RESCODE_NO_ERROR, PPERR_SHTRIHFRINVRESULTCODE, addendum_msg_buf);
	}
	CATCHZOK
	return ok;
}

int SCS_SHTRIHFRF::ExecFR(int id)
{
	int    ok = 1;
	THROW_PP(P_DrvFRIntrf, PPERR_SHTRIHFRIFCNOTINITED);
	LogDebug("SetProperty", Password, 0); // @v11.6.12
	THROW_PP_S(P_DrvFRIntrf->SetProperty(Password, CashierPassword) > 0, PPERR_SHTRIHFRSETPROPFAULT, P_DrvFRIntrf->GetNameByID(Password, SLS.AcquireRvlStr()));
	THROW(ExecFR_WithoutPassword(id));
	CATCHZOK
	return ok;
}

int SCS_SHTRIHFRF::ExecFR_WithoutPassword(int id)
{
	int    ok = 1;
	THROW_PP(P_DrvFRIntrf, PPERR_SHTRIHFRIFCNOTINITED);
	LogDebug("CallMethod", id, 0); // @v11.6.12
	THROW_PP_S(P_DrvFRIntrf->CallMethod(id) > 0, PPERR_SHTRIHFRCALLMETHFAULT, P_DrvFRIntrf->GetNameByID(id, SLS.AcquireRvlStr()));
	THROW(GetResultCode(id));
	CATCHZOK
	return ok;
}

static bool IsModeOffPrint(int mode) { return oneof5(mode, FRMODE_OPEN_SESS, FRMODE_CLOSE_SESS, FRMODE_OPEN_CHECK, FRMODE_FULL_REPORT, FRMODE_LONG_EKLZ_REPORT) ? false : true; }

void SCS_SHTRIHFRF::WriteLogFileToWriteTblErr(int tblNum, int rowNum, int fldNum, const char * pValue)
{
	if(IsDebugMode) {
		SString msg_fmt, msg, value;
		(value = pValue).Transf(CTRANSF_OUTER_TO_INNER);
		msg.Printf(PPLoadTextS(PPTXT_LOG_SHTRIH_WRITETBLERR, msg_fmt), tblNum, rowNum, fldNum, value.cptr());
		PPLogMessage(PPFILNAM_SHTRIH_LOG, msg, LOGMSGF_TIME|LOGMSGF_USER);
	}
}

void SCS_SHTRIHFRF::LogDebug(const char * pFunc, int ifcId, const char * pArg)
{
	if(IsDebugMode) {
		SString temp_buf;
		SString msg_buf;
		if(!isempty(pFunc))
			msg_buf.Cat(pFunc);
		if(ifcId >= 0 && P_DrvFRIntrf) {
			P_DrvFRIntrf->GetNameByID(ifcId, temp_buf);
			msg_buf.CatDivConditionally(',', 2, msg_buf.NotEmpty()).Cat(temp_buf);
		}
		if(!isempty(pArg)) {
			msg_buf.CatDivConditionally(',', 2, msg_buf.NotEmpty()).Cat(pArg);
		}
		if(msg_buf.NotEmpty())
			PPLogMessage(PPFILNAM_SHTRIH_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
	}
}

int  SCS_SHTRIHFRF::LogLastError()
{
	SString & r_msg_buf = SLS.AcquireRvlStr();
	PPGetLastErrorMessage(1, r_msg_buf);
	PPLogMessage(PPFILNAM_SHTRIH_LOG, r_msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
	return 0;
}

void SCS_SHTRIHFRF::WriteLogFile(int id)
{
	if(IsDebugMode) {
		int     adv_mode = 0;
		size_t  pos = 0;
		char    mode_descr[MAX_PATH];
		SString msg_fmt, msg;
		SString err_msg = DS.GetConstTLA().AddedMsgString;
		SString oper_name;
		PPLoadText(PPTXT_LOG_SHTRIH, msg_fmt);
		P_DrvFRIntrf->GetNameByID(id, oper_name);
		memzero(mode_descr, sizeof(mode_descr));
		GetFR(ECRModeDescription, mode_descr, sizeof(mode_descr) - 1);
		SCharToOem(mode_descr);
		if(err_msg.SearchChar('\n', &pos))
			err_msg.Trim(pos);
		GetFR(ECRAdvancedMode, &adv_mode);
		msg.Printf(msg_fmt, oper_name.cptr(), err_msg.cptr(), mode_descr, adv_mode);
		PPLogMessage(PPFILNAM_SHTRIH_LOG, msg, LOGMSGF_TIME|LOGMSGF_USER);
	}
}
//
// Функция AllowPrintOper разбирается со всеми ситуациями,
//  которые могут возникнуть при печати чека.
// Код возврата: 1 - операция печати разрешена, 0 - запрещена.
//
int SCS_SHTRIHFRF::AllowPrintOper(int id)
{
	int    ok = 1;
	int    mode = 0;
	int    adv_mode = PRNMODE_NO_PRINT;
	int    slip_mode_status = SLIPMODE_BEFORE_PRINT;
	int    last_res_code = ResCode;
	int    is_chk_rbn = 1;
	int    is_jrn_rbn = 1;
	int    wait_prn_err = 0;
	SString temp_buf;
	SetErrorMessage();
	// Ожидание окончания операции печати
	do {
		THROW(ExecFR(GetECRStatus));
		THROW(GetFR(ECRMode,         &mode));
		THROW(GetFR(ECRAdvancedMode, &adv_mode));
		if(Flags & sfPrintSlip) {
			THROW(GetFR(ECRModeStatus, &slip_mode_status));
		}
		else {
			THROW(GetFR(ReceiptRibbonOpticalSensor, &is_chk_rbn));
			if(is_chk_rbn && (Flags & sfUseWghtSensor))
				THROW(GetFR(ReceiptRibbonIsPresent, &is_chk_rbn));
			if(DeviceType == devtypeShtrih) {
				THROW(GetFR(JournalRibbonOpticalSensor, &is_jrn_rbn));
				if(is_jrn_rbn && Flags & sfUseWghtSensor)
					THROW(GetFR(JournalRibbonIsPresent, &is_jrn_rbn));
			}
		}
		if((!(Flags & sfPrintSlip) && adv_mode == PRNMODE_NO_PRINT) || oneof2(adv_mode, PRNMODE_PRINT, PRNMODE_PRINT_LONG_REPORT))
			wait_prn_err = 1;
		// @v11.6.12 {
		if(IsDebugMode) {
			temp_buf.Z().CatEq("adv_mode", adv_mode).CatDiv(',', 2).CatEq("slip_mode_status", slip_mode_status);
			LogDebug("loop AllowPrintOper", -1, temp_buf); 
		}
		// } @v11.6.12 
	} while(oneof2(adv_mode, PRNMODE_PRINT, PRNMODE_PRINT_LONG_REPORT) || !oneof2(slip_mode_status, SLIPMODE_BEFORE_PRINT, SLIPMODE_AFTER_PRINT));
	if(Flags & sfPrintSlip) {
		if(oneof2(slip_mode_status, SLIPMODE_BEFORE_PRINT, SLIPMODE_AFTER_PRINT)) {
			ExecFR(Beep);
			wait_prn_err = 1;
			if(PPMessage(mfConf|mfYesNo, PPCFM_CONTSLIPDOCPRINT) != cmYes) {
				Flags |= sfCancelled;
				ok = 0;
			}
		}
	}
	else {
		// На всякий случай помечаем, что чек открыт (иначе при сбое операции открытия чека неизвестно: чек уже открыт или нет)
		if(mode == FRMODE_OPEN_CHECK)
			Flags |= sfCheckOpened;
		// Ожидание заправки чековой ленты или выхода из режима, когда нельзя печатать чек
		while(ok && (oneof2(adv_mode, PRNMODE_NO_PRINT_NO_PAPER, PRNMODE_PRINT_NO_PAPER) || (last_res_code == RESCODE_MODE_OFF && IsModeOffPrint(mode)))) {
			int  send_msg = 0, r;
			if(!is_chk_rbn) {
				PPSetError(PPERR_SYNCCASH_NO_CHK_RBN);
				send_msg = 1;
			}
			else if(!is_jrn_rbn) {
				PPSetError(PPERR_SYNCCASH_NO_JRN_RBN);
				send_msg = 1;
			}
			else
				WriteLogFile(id);
			ExecFR(Beep);
			wait_prn_err = 1;
			r = PPError();
			if((!send_msg && r != cmOK) || (send_msg && ExecFR(Beep) && PPMessage(mfConf|mfYesNo, PPCFM_SETPAPERTOPRINT) != cmYes)) {
				Flags |= sfCancelled;
				ok = 0;
			}
			THROW(ExecFR(GetECRStatus));
			THROW(GetFR(ECRMode,         &mode));
			THROW(GetFR(ECRAdvancedMode, &adv_mode));
		}
		// Проверяем, надо ли завершить печать после заправки ленты
		if(adv_mode == PRNMODE_AFTER_NO_PAPER) {
			WriteLogFile(id);
			THROW(ExecFR(ContinuePrint));
			THROW(ExecFR(GetECRStatus));
			THROW(GetFR(ECRMode,         &mode));
			THROW(GetFR(ECRAdvancedMode, &adv_mode));
			wait_prn_err = 1;
		}
		//
		//   Это, конечно, не отрывок из "Илиады", а очередная попытка
		//   справиться с идиотскими ошибками, возникающими из-за этой дерьмовой ЭКЛЗ.
		//
		if(mode == FRMODE_OPEN_CHECK && id == CutCheck) {
			WriteLogFile(id);
			SString  err_msg(DS.GetConstTLA().AddedMsgString), added_msg;
			if(PPLoadText(PPTXT_APPEAL_CTO, added_msg))
				err_msg.CR().Cat("\003").Cat(added_msg);
			PPSetAddedMsgString(err_msg);
			ExecFR(Beep);
			PPError();
			ok = -1;
		}
	}
	//
	// Если ситуация не связана непосредственно с процессом печати, выдаем сообщение об ошибке
	// @v5.9.2 При закрытии чека - сумма оплаты меньше суммы чека - не связано с процессом печати, но wait_prn_err == 1
	//
	if(!wait_prn_err || oneof2(last_res_code, RESCODE_PAYM_LESS_SUM, RESCODE_INVEKLZSTATE)) {
		WriteLogFile(id);
		ExecFR(Beep);
		PPError();
		Flags |= sfCancelled;
		ok = 0;
	}
	CATCH
		ok = LogLastError(); // @v11.6.9
	ENDCATCH
	return ok;
}

void SCS_SHTRIHFRF::SetErrorMessage()
{
	char   err_buf[MAX_PATH];
	memzero(err_buf, sizeof(err_buf));
	if((Flags & sfConnected) && ResCode != RESCODE_NO_ERROR && GetFR(ResultCodeDescription, err_buf, sizeof(err_buf)-1)) {
		SString err_msg(err_buf);
		if(ResCode == RESCODE_MODE_OFF && ExecFR(GetECRStatus) > 0) {
			char    mode_descr[MAX_PATH];
			memzero(mode_descr, sizeof(mode_descr));
			if(GetFR(ECRModeDescription, mode_descr, sizeof(mode_descr)-1)) {
				SString temp_buf;
				PPLoadText(PPTXT_CCFMT_MODE, temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				err_msg.CR().CatChar('\003').Cat(temp_buf).CatDiv(':', 2).Cat(mode_descr);
			}
		}
		err_msg.Transf(CTRANSF_OUTER_TO_INNER);
		PPSetError(PPERR_SYNCCASH, err_msg);
	}
}

/*virtual*/int SCS_SHTRIHFRF::PreprocessChZnCode(int op, const char * pCode, double qtty, int uomId, uint uomFragm, CCheckPacket::PreprocessChZnCodeResult & rResult) // @v11.6.6
{
	int    ok = -1;
	SString temp_buf;
	SString msg_buf;
	if(op == ppchzcopInit) {
		//THROW(SetFR(CheckType, 0L/*Продажа*/));
		//THROW(ExecFR(OpenCheck));
		//Flags |= sfCheckOpened;
	}
	else if(op == ppchzcopCancel) {
		//if(Flags & sfCheckOpened) {
			//THROW(ExecFR(CancelCheck));
			//Flags &= ~(sfCheckOpened | sfCancelled);
		//}
	}
	else if(op == ppchzcopCheck) {
		rResult.CheckResult = 0;
		rResult.Reason = 0;
		rResult.ProcessingResult = 0;
		rResult.ProcessingCode = 0;
		rResult.Status = 0;
		if(!isempty(pCode)) {
			GtinStruc gts;
			if(PPChZnPrcssr::InterpretChZnCodeResult(PPChZnPrcssr::ParseChZnCode(pCode, gts, 0)) == PPChZnPrcssr::chznciReal) {
				if(gts.GetToken(GtinStruc::fldGTIN14, &temp_buf)) {
					SString serial;
					SString partn;
					SString result_chzn_code;
					SString left, right;
					SString gtin(temp_buf);
					char  tlv_data_hex[512];
					result_chzn_code.Cat(temp_buf);
					if(gts.GetToken(GtinStruc::fldSerial, &temp_buf)) {
						result_chzn_code.Cat(temp_buf);
						serial = temp_buf;
					}
					if(gts.GetToken(GtinStruc::fldPart, &temp_buf)) {
						if(serial.IsEmpty())
							result_chzn_code.Cat(temp_buf);
						partn = temp_buf;
					}
					//
					ResCode = RESCODE_NO_ERROR;
					/*if(ExtMethodsFlags & extmethfFNCheckItemBarcode2) {
						THROW(ConnectFR());
						//
						const uint code_len = sstrlen(pCode);
						temp_buf.Z();
						for(uint cidx = 0; cidx < code_len; cidx < code_len) {
							temp_buf.CatHex(static_cast<uint8>(pCode[cidx]));
						}
						THROW(SetFR(BarcodeHex, temp_buf));
						THROW(ExecFR(FNCheckItemBarcode2));
					}
					else*/
					if(ExtMethodsFlags & extmethfFNCheckItemBarcode) {
						const bool use_FNCheckItemBarcode2 = true; // Если false, то используем FNCheckItemBarcode
						if(!(Flags & sfCheckOpened)) {
							THROW(ConnectFR());
						}
						//
						//THROW(SetFR(BarCode, pCode));
						{
							const uint code_len = sstrlen(pCode);
							temp_buf.Z();
							for(uint cidx = 0; cidx < code_len; cidx++) { // @v11.7.11 @fix (я - долбоеб. это - точно!) (cidx < code_len)-->cidx++
								temp_buf.CatHex(static_cast<uint8>(pCode[cidx]));
							}
							THROW(SetFR(BarcodeHex, temp_buf));
						}
						THROW(SetFR(ItemStatus, 1L));
						THROW(SetFR(CheckItemMode, 0L));
						if(use_FNCheckItemBarcode2) {
							//IFC_ENTRY_SS(DivisionalQuantity,     ExtMethodsFlags, extmethfDivisionalQuantity),     // @v11.6.6 / !
							//IFC_ENTRY_SS(Numerator,              ExtMethodsFlags, extmethfNumerator),              // @v11.6.6 / !
							//IFC_ENTRY_SS(Denominator,            ExtMethodsFlags, extmethfDenominator),            // @v11.6.6 / !
							THROW(SetFR(DivisionalQuantity, false));
							tlv_data_hex[0] = 0;
							THROW(ExecFR(FNCheckItemBarcode2));
						}
						else {
							THROW(SetFR(TLVDataHex, ""));
							THROW(ExecFR(FNCheckItemBarcode));
						}
						{
							msg_buf.Z().Cat("FNCheckItemBarcode req").CatDiv(':', 2).CatEq("BarCode", pCode).CatDiv(',', 2).
								CatEq("ItemStatus", 1).CatDiv(',', 2).CatEq("CheckItemMode", 0).CatDiv(',', 2).
								CatEq("TLVDataHex", tlv_data_hex);
							PPLogMessage(PPFILNAM_SHTRIH_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
						}
						//
						{
							int   item_local_result = 0;
							int   item_local_error = 0;
							int   marking_type = 0; // tag 2100
							int   svr_err_code = 0;
							int   svr_checking_status = 0; // tag 2106
							tlv_data_hex[0] = 0;
							THROW(GetFR(CheckItemLocalResult, &item_local_result));
							THROW(GetFR(CheckItemLocalError, &item_local_error));
							THROW(GetFR(MarkingType2, &marking_type));
							THROW(GetFR(KMServerErrorCode, &svr_err_code));
							THROW(GetFR(KMServerCheckingStatus, &svr_checking_status));
							THROW(GetFR(TLVDataHex, tlv_data_hex, sizeof(tlv_data_hex)-1));
							rResult.CheckResult = svr_checking_status;
							rResult.Reason = item_local_error;
							rResult.ProcessingResult = item_local_result; // @?
							rResult.ProcessingCode = 0; // @?
							rResult.Status = 0; // @?
							{
								msg_buf.Z().Cat("FNCheckItemBarcode rep").CatDiv(':', 2).CatEq("CheckItemLocalResult", item_local_result).CatDiv(',', 2).
									CatEq("CheckItemLocalError", item_local_error).CatDiv(',', 2).CatEq("MarkingType2", marking_type).CatDiv(',', 2).
									CatEq("KMServerErrorCode", svr_err_code).CatDiv(',', 2).CatEq("TLVDataHex", tlv_data_hex);
								PPLogMessage(PPFILNAM_SHTRIH_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
							}
							ok = 1;
						}
					}
				}
			}
		}
	}
	else if(op == ppchzcopAccept) {
		THROW(ExecFR(FNAcceptMarkingCode));
	}
	else if(op == ppchzcopReject) {
		THROW(ExecFR(FNDeclineMarkingCode));
	}
	CATCH
		ok = LogLastError(); // @v11.6.9
	ENDCATCH
	return ok;
}

// @vmiller
int SCS_SHTRIHFRF::PrintBnkTermReport(const char * pZCheck)
{
	int     ok = 1;
	StringSet str_set('\n', pZCheck);
	SString line_buf;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
	THROW(ConnectFR());
	RibbonParam = 0;
	CheckForRibbonUsing(SlipLineParam::fRegRegular);
	THROW(SetFR(DocumentName, ""));
	THROW(ExecFR(PrintDocumentTitle));
	for(uint pos = 0; str_set.get(&pos, line_buf);) {
		CheckForRibbonUsing(SlipLineParam::fRegRegular);
		CutLongTail(line_buf);
		THROW(SetFR(StringForPrinting, line_buf));
		THROW(ExecFR(PrintString));
	}
	THROW(LineFeed(3, TRUE, FALSE));
	THROW(Cut(1));
	CATCHZOK
	return ok;
}
