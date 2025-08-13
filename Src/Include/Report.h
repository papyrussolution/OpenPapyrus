// REPORT.H
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2015, 2016, 2019, 2025
// @codepage UTF-8
//
#ifndef __REPORT_H
#define __REPORT_H

class TVRez;
struct PEExportOptions;
//
// Встроенные переменные
//
#define BIVAR_CURDATE             101
#define BIVAR_CURTIME             102
#define BIVAR_PAGE                103
//
// Типы зон отчета
//
#define FIRST_TEXT_ID            1001
#define RPT_HEAD                   -1
#define RPT_FOOT                   -2
#define PAGE_HEAD                  -3
#define PAGE_FOOT                  -4
#define GROUP_HEAD                 -5
#define GROUP_FOOT                 -6
#define DETAIL_BODY                -7
//
// Форматирующие флаги полей
//
#define FLDFMT_BOLD            0x0001
#define FLDFMT_ITALIC          0x0002
#define FLDFMT_UNDERLINE       0x0004
#define FLDFMT_SUPERSCRIPT     0x0008
#define FLDFMT_SUBSCRIPT       0x0010
#define FLDFMT_ESC             0x0020
#define FLDFMT_NOREPEATONPG    0x0040
#define FLDFMT_NOREPEATONRPT   0x0080
#define FLDFMT_STRETCHVERT     0x0100
#define FLDFMT_AGGR            0x0800 // Поле содержит агрегатную функцию
#define FLDFMT_SKIP            0x1000 // Пропустить печать пол
//
// Форматирующие флаги групп
//
#define GRPFMT_NEWPAGE         0x0001
#define GRPFMT_SWAPHEAD        0x0002
#define GRPFMT_SWAPFOOT        0x0004
#define GRPFMT_REPRINTHEAD     0x0008
#define GRPFMT_RESETPGNMB      0x0010
#define GRPFMT_SUMMARYONBOTTOM 0x0020 // Итог на дно страницы
//
// Опции печати
//
#define SPRN_EJECTBEFORE       0x00000001 // Прогон страницы перед печатью
#define SPRN_EJECTAFTER        0x00000002 // Прогон страницы после печати
#define SPRN_NLQ               0x00000004
#define SPRN_CPI10             0x00000008
#define SPRN_CPI12             0x00000010
#define SPRN_CONDS             0x00000020
#define SPRN_LANDSCAPE         0x00000040 // Альбомная ориентация листа (иначе книжная)
#define SPRN_SKIPGRPS          0x00004000 // Пропустить группировку при печати
#define SPRN_TRUEPRINTER       0x00008000 // Служебный флаг (истинный принтер)
#define SPRN_PREVIEW           0x00010000 // Предварительный просмотр
#define SPRN_DONTRENAMEFILES   0x00020000 // Обычно, при печати файлы данных переименовываются дабы
	// не мешать работе предыдущему (последующему) сеансу печати. Если эта опция передается в
	// функцию CrystalReportPrint, то файлы переименовываться не будут (иногда это важно).
#define SPRN_USEDUPLEXPRINTING 0x00040000 // Использовать дуплексную печать
//
// Опции функции SPrinter::setEffect
//
#define FONT_BOLD              0x0001
#define FONT_ITALIC            0x0002
#define FONT_UNDERLINE         0x0004
#define FONT_SUPERSCRIPT       0x0008
#define FONT_SUBSCRIPT         0x0010

#define SPCMDSET_DEFAULT            0
#define SPCMDSET_EPSON              1
#define SPCMDSET_PCL                2

#define SPMRG_LEFT                  1
#define SPMRG_RIGHT                 2
#define SPMRG_TOP                   3
#define SPMRG_BOTTOM                4

#define SPQLTY_DRAFT                1
#define SPQLTY_NLQ                  2

#define SPCPI_10                    1
#define SPCPI_12                    2
#define SPCPI_COND                  3

#define SPFS_BOLD                   1
#define SPFS_ITALIC                 2
#define SPFS_UNDERLINE              3

class SPrnCmdSet {
public:
	static SPrnCmdSet * CreateInstance(long, long = 0);

	SPrnCmdSet();
	virtual ~SPrnCmdSet();
	//
	// Каждая из виртуальных функций этого (и порожденных) классов
	// должна занести в буфер, заданный последним параметром, управляющую
	// последовательность и вернуть количество символов в этой
	// последовательности. Не следует завершать управляющую
	// последовательность нулем, так как длина все равно определяетс
	// возвращаемым значением.
	//
	virtual int InitPrinter(char *) { return 0; }
	virtual int ResetPrinter(char *) { return 0; }
	//
	// SetPageLength задает длину листа (строк - НЕ ДЮЙМОВ !)
	//
	virtual int SetPageLength(int, char *) { return 0; }
	virtual int SetOrientation(int /* 0 - portrait, !0 - landscapce */, char *) { return 0; }
	virtual int SetMargin(int what, int, char *) { return 0; }
	virtual int SetQuality(int, char *) { return 0; }
	virtual int SetCPI(int, char *) { return 0; }
	virtual int SetFontStyle(int, int on_off, char *) { return 0; }
	virtual int SetLinesPerInch(int, char *) { return 0; }
private:
	int    dummy;
};
//
// Descr: Параметры экспорта данных отчета средствами Crystal Reports.
//   Нужны для того, чтобы заменить интерактивные функции Crystal Reports в рамках перевода системы на arch-x64
//
struct CrystalReportExportParam { // @v12.3.7
	//
	// Descr: Возвращает имя dll-модуля, отвечающего за вывод в формате format
	//
	static bool GetFormatDll(uint format, SString & rDllModuleName);
	//
	// Descr: Возвращает имя dll-модуля, отвечающего за вывод результат в виде, определенном аргументом dest (destApp || destFile)
	//
	static bool GetDestDll(uint dest, SString & rDllModuleName);
	CrystalReportExportParam();
	CrystalReportExportParam(const CrystalReportExportParam & rS);
	CrystalReportExportParam & FASTCALL operator = (const CrystalReportExportParam & rS);
	CrystalReportExportParam & FASTCALL Z();
	bool   FASTCALL Copy(const CrystalReportExportParam & rS);
	int    LoadFromIniFile(const char * pReportName);
	int    TranslateToCrrExportOptions(PEExportOptions & rEo);

	enum {
		crexpfmtPdf     = SFileFormat::Pdf,     // (UXFPdfType) params: page-range
		crexpfmtRtf     = SFileFormat::Rtf,     // (UXFRichTextFormatType) params: page-range
		crexpfmtHtml    = SFileFormat::Html,    // params: page-range, fHtmlPageNavigator, fHtmlSeparatePages, directory, base-file-name
		crexpfmtExcel   = SFileFormat::Xls,     // (UXFXls7ExtType)
		crexpfmtWinWord = SFileFormat::WinWord, // (UXFWordWinType) params: page-range
		crexpfmtCsv     = SFileFormat::Csv,     // (UXFCommaSeparatedType, UXFTabSeparatedType, UXFCharSeparatedType)
	};
	enum {
		destFile = 1,
		destApp  = 2,
	};
	enum {
		fSelectedPages             = 0x0001,
		fHtmlPageNavigator         = 0x0002,
		fHtmlSeparatePages         = 0x0004,
		fXlsColumnHeadings         = 0x0008,
		fXlsUseConstColumnWidth    = 0x0010,
		fXlsTabularFormat          = 0x0020,
		fXlsCreatePgBrkForEachPage = 0x0040,
		fXlsCvtDateToString        = 0x0080,
		fXlsShowGrid               = 0x0100,
		fCsvUseReportNumberFormat  = 0x0200,
		fCsvUseReportDateFormat    = 0x0400, 
		fCsvQuoteText              = 0x0800,
		fSilent                    = 0x1000  // Не интерактивный режим
	};
	uint    Flags;
	uint    Format;
	uint    Destination;          // destXXX 
	uint    XlsConstColumnWidth;
	uint    XlsBaseAreaType;      // One of the 7 Section types defined in "Crpe.h". The default value is PE_SECT_DETAIL.
	uint    XlsBaseAreaGroupNum;  // If baseAreaType is either GroupHeader or
		//GroupFooter and there are more than one groups, we need to give the group number. The default value is 1.
	uint    CsvFieldSeparator;
	IntRange PageRange; // if Flags & fSelectedPages
	SString DestFileName;
};
//
//
//
struct ReportDescrEntry {
	//
	// Descr: Типы параметров описания отчетов в report.ini и stdrpt.ini
	//
	enum {
		tUnkn = 0,
		tComment,
		tData,
		tDescr,
		tDiffIdByScope,
		tModifDate,
		tStd,
		tFormat,
		tDestination,
		tSilent,
		tExistFile
	};
	static int FASTCALL GetIniToken(const char * pBuf, SString * pFileName);

	ReportDescrEntry();
	int    SetReportFileName(const char * pFileName);

	enum {
		fInheritedTblNames = 0x0001,
		fDiff_ID_ByScope   = 0x0002, // Если этот флаг установлен, то наименования полей идентификаторов
			// записей в таблицах, соответствующих разным областям будут отличаться.
			// Эта опция необходима из-за того, что единовременно перевести все структуры на
			// различающиеся наименования таких полей невозможно по причине необходимости верификации
			// соответствующих отчетов
		fTddoResource      = 0x0004
	};
	long   Flags;
	SString ReportPath_;
	SString Description_;
	SString DataName_;
	SString OutputFormat;
};

struct PrnDlgAns {
	explicit PrnDlgAns(const char * pReportName);
	explicit PrnDlgAns(const PrnDlgAns & rS);
	~PrnDlgAns();
	PrnDlgAns & FASTCALL operator = (const PrnDlgAns & rS);
	int    SetupReportEntries(const char * pContextSymb);
	PrnDlgAns & FASTCALL Copy(const PrnDlgAns & rS);

	enum {
		aUndef = 0,
		aPrint = 1,
		aExport,
		aPreview,
		aExportXML,
		aPrepareData,
		aPrepareDataAndExecCR,
		aExportTDDO
	};
	enum {
		fForceDDF  = 0x0001,
		fEMail     = 0x0002, // Действителен при Dest == aExport
		fUseDuplexPrinting = 0x0004  // Дуплексная печать
	};
	long   Dest;
	int    Selection;
	uint   NumCopies;
	long   Flags;
	SString ReportName;
	SString DefPrnForm;
	SString PrepareDataPath;
	SString Printer;
	SString EmailAddr;
	SString ContextSymb;
	CrystalReportExportParam ExpParam; // @v12.3.9
	TSCollection <ReportDescrEntry> Entries;
	DEVMODEA * P_DevMode;
private:
	int    PreprocessReportFileName(const char * pFileName, ReportDescrEntry * pEntry);
};

class SPrinter {
public:
	SPrinter();
	~SPrinter();
	int    setupCmdSet(long cmdsetID, long extra = 0);
	int    setPort(char *);
	int    startDoc();
	int    endDoc();
	int    abortDoc();
	int    startPage();
	int    endPage();
	int    escape(int, char *);
	int    printLine(const char * buf, size_t maxLen);
	int    printChar(int c);
	int    checkPort();
	uint   getStyle();
	int    setEffect(int);
	int    initDevice();
	int    resetDevice();
	int    checkNWCapture();
	//
	// Descr: Функция HandlePrintError может быть установленна прикладной программой для интерактивной обработки
	//   ошибки печати. Если эта функция возвращает 0, то печать будет прервана по ошибке, в противном случае
	//   последует повторная попытка. По умолчанию эта функция равна NULL, что обрабатывается как завершение по ошибке.
	//
	static int (*HandlePrintError)(int errCode);
	SPrnCmdSet * cmdSet;
	char * device;
	char   port[64];
	int    prnport;
	int    captured; // Признак захвата порта сетевым сервером печати
	int    fd;       // file handler
	int    pgl;
	int    pgw;
	int    leftMarg;
	int    rightMarg;
	int    topMarg;
	int    bottomMarg;
	uint   options;
};

SPrinter * getDefaultPrinter();
//
// Функция ReportIterator должна возвращать следующие значения:
// -1 - Конец итерации
//  0 - Ошибка
//  1 - Успешная итерация //
//  2 - Успешная итерация, агрегатные функции не пересчитывать
//
typedef int (*ReportIterator)(int first);

class SReport {
public:
	struct Field {
		int16  id;
		int    name; // @ Index in SReport::text buffer or -1
		TYPEID type;
		union {
			long   format;
			int16  len;  // text
		};
		uint   fldfmt;
		union {
			void * data;
			long   offs; // text
		};
		char * lastval;
	};
	struct Aggr {
		int16  fld;
		int16  aggr;  // Агрегатная функци
		int16  dpnd;  // Поле, которое требуется агрегировать
		int16  scope; // -1 весь отчет, 0.. номер группы
		union {
			double   rtemp;
			double * ptemp;
		};
	};
	struct Group {
		int16   band;
		int16 * fields;
		char  * lastval;
	};
	struct Band {
		int     addField(int id);
		int16   kind;
		int16   ht;
		int16   group;
		uint16  options;
		int16 * fields;
	};

	static int defaultIterator(int);
	static SArray * FillRptArray();
	explicit SReport(const char *);
	SReport(uint rezID, long flags /* INIREPF_XXX */);
	~SReport();
	bool   IsValid() const;
	void   disableGrouping();
	int    addField(int id, TYPEID typ, long fmt, uint rptfmt, char * nam);
	int    setAggrToField(int fld, int aggr, int dpnd);
	int    addText(char * txt);
	int    addBand(SReport::Band * band, int * grp_fld, uint * pos);
	int    calcAggr(int grp, int mode); // 0 - init, 1 - calc, 2 - summary
	int    check();
	int    setData(int id, void * data);
	int    skipField(int id, int enable);
	int    setIterator(ReportIterator);
	int    setPrinter(SPrinter*);
	int    setDefaultPrinter();
	int    getNumCopies() const;
	void   setNumCopies(int);
	int    enumFields(SReport::Field **, SReport::Band *, int *);
	int    getFieldName(SReport::Field *, char * buf, size_t buflen);
	int    getFieldName(int id, char * buf, size_t buflen);
	int    printDataField(SReport::Field * f);
	int    printPageHead(int kind, int _newpage);
	int    printGroupHead(int kind, int grp);
	int    checkval(int16 * flds, char ** ptr);
	int    printDetail();
	int    printTitle(int kind);
	int    writeResource(FILE *, uint);
	int    readResource(TVRez *, uint resID);
	SReport::Band * searchBand(int kind, int grp);
	int    createDataFiles(const char * pDataName, const char * pRptPath);
	const  SString & getDataName() const { return DataName; }
	int    PrnDest;
	int    Export();
	int    preview();
private:
	// @v12.3.9 @obsolete int    prepareData();
	// @v12.3.9 @obsolete int    createBodyDataFile(SString & rFileName, SCollection * fldIDs);
	// @v12.3.9 @obsolete int    createVarDataFile(SString & rFileName, SCollection * fldIDs);
public:
	enum rptFlags {
		DisableGrouping = 0x0001,
		FooterOnBottom  = 0x0002,
		Landscape       = 0x0004,
		PrintingNoAsk   = 0x0010,
		NoRepError      = 0x0020, // Не выдавать сообщение об ошибке
		XmlExport       = 0x0040, // Экспорт в XML
		Preview = 0x0080  // Предварительный просмотр
	};
	SString Name;
	SString DataName;
	long   main_id;
	int    page;
	int    line;
	int    fldCount;
	Field * fields;
	int    agrCount;
	Aggr * agrs;
	int    grpCount;
	Group * groups;
	int    bandCount;
	Band * bands;
	//
	// Следующие три поля необходимы для записи в ресурс и передачи классу SPrinter
	//
	int    PageLen;
	int    LeftMarg;
	int    PrnOptions;
	int    PageHdHt; // Высота верхнего колонтитула
	int    PageFtHt; // Высота нижнего колонтитула
	int    Count;
	SPrinter * P_Prn;
	ReportIterator iterator;
private:
	int    Error;
	int    NumCopies;
	size_t TextLen;
	char * P_Text;
};
//
// Descr: Структура параметров печати для реализации межпроцессного интерфейса для вывода 32-битного CrystalReports в отдельный процесс
//
class CrystalReportPrintParamBlock { // @v11.9.5 @construction
public:
	CrystalReportPrintParamBlock() : Action(actionUndef), InternalFlags(0), NumCopies(0), Options(0)
	{
		MEMSZERO(DevMode);
	}
	enum {
		intfDevModeValid = 0x0001
	};
	enum {
		actionUndef = 0,
		actionPrint = 1,
		actionPreview,
		actionExport
	};
	int    Action;
	uint   InternalFlags;
	uint32 NumCopies;
	uint32 Options;
	DEVMODEA DevMode;
	SString ReportPath;
	SString ReportName;
	SString EmailAddr;   // for Action == actionExport
	SString Dir;
	SString Printer;
	CrystalReportExportParam ExpParam; // @v12.3.10
};

class CrystalReportPrintReply {
public:
	CrystalReportPrintReply() : ErrCode(0)
	{
	}
	int    ErrCode;
	SString ErrMsg;
};

int EditPrintParam(PrnDlgAns * pData);
int CrystalReportPrint(const char * pReportPath, const char * pDir, const char * pPrinter, int numCopies, int options, const DEVMODEA *pDevMode);  //erik{DEVMODEA *pDevMode} add param v10.4.10
int CrystalReportPrint2(const CrystalReportPrintParamBlock & rBlk, CrystalReportPrintReply & rReply); // @v11.9.5 @construction
int CrystalReportExport(const char * pReportPath, const char * pDir, const char * pReportName, const char * pEMailAddr, int options);

#endif /* __REPORT_H */

