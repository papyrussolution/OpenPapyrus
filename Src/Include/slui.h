// SLUI.H
// Copyright (c) A.Sobolev 1996-2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#ifndef __SLUI_H
#define __SLUI_H

#include <tvdefs.h>
#include <commctrl.h>
#include <db.h>

class  TView;
class  TViewGroup;
class  TWindow;
class  TDialog;
class  TButton;
class  TLabel;
class  ComboBox;
class  TBaseBrowserWindow;
class  TCanvas;
class  TCanvas2;
class  WordSelector;
class  SrDatabase;
struct TDrawItemData;
class  TWhatman;
struct BroColumn;
//
// Макросы для блокировки локальных участков кода для предотвращения повторного 
// вхождения по причине того, что код на этом участке может неявно генерировать события, инициирующие
// рекурсию.
// @snippet
// if(event.isCmd(cmInputUpdated) && event.isCtlEvent(ctl_id)) {
//   UI_LOCAL_LOCK_ENTER
//     // Следующий оператор меняет содержимое строки в ответ на изменение ее самой.
//     // Внутри функции setCtrlString произойдет генерация события cmInputUpdated и,
//     // если этот код не ограничить конструкцией UI_LOCAL_LOCK_ENTER/UI_LOCAL_LOCK_LEAVE, то
//     // возникнет бесконечная рекурсия и неизбежное аварийное завершения сеанса по причине переполнения стека.
//     setCtrlString(ctl_id, temp_buf.Space().Cat("some_text"));
//   UI_LOCAL_LOCK_LEAVE
// } 
//
#define UI_LOCAL_LOCK_ENTER static bool __local_lock; if(!__local_lock) { __local_lock = true;
#define UI_LOCAL_LOCK_LEAVE __local_lock = false; }
//
//
//
class WordSel_ExtraBlock {
public:
	WordSel_ExtraBlock();
	WordSel_ExtraBlock(uint inputCtl, HWND hInputDlg, TDialog * pOutDlg, uint outCtlId, uint minSymbCount, long flags = 0);
	virtual ~WordSel_ExtraBlock();
	void   Init(uint inputCtl, HWND hInputDlg, TDialog * pOutDlg, uint outCtlId, uint minSymbCount, long flags = 0);
	virtual int Search(long id, SString & rBuf);
	//
	// Descr: Позволяет идентифицировать уникальный объект по текстовому ключу pText.
	//   Если функция не поддерживается, то должна вернуть -1.
	//   Если функция поддерживается, то она должна найти соответствие ключу pText и, если таковое
	//   существует, установить соответствующие значения *pID и rBuf и вернуть >0.
	//   Если функция поддерживается, и значение по ключу pText не найдено, то должна вернуть 0.
	//
	virtual int SearchText(const char * pText, long * pID, SString & rBuf);
	virtual StrAssocArray * GetList(const char * pText);
	virtual StrAssocArray * GetRecentList();
	//
	// Descr: Функция вызывается при подтверждении пользователем ввода данных (вероятно, речь может пока идти о кнопке [OK]).
	//   Функция может каким-либо образом зафиксировать факт согласия пользователем с данными.
	// Note: Фактически, функция вводится ради сохранения истории ввода данных пользователем в текстовых полях.
	//
	virtual void OnAcceptInput(const char * pText, long id);
	long   GetFlags() const { return Flags; }
	bool   IsTextMode() const { return CtrlTextMode; }
	void   SetTextMode(bool v);
	void   SetData(long id, const char * pText);
	//
	// Descr: Производит поиск текста по id, затем вызывает функциию SetData
	//
	void   SetupData(long id);
	int    GetData(long * pId, SString & rBuf);

	enum {
		fAlwaysSearchBySubStr = 0x0001,
		fFreeText             = 0x0002 // Текст в основной строке вводится свободно без вызова окна поиска
	};
//protected:
	long   Flags;
	bool   CtrlTextMode; // Если true, то при вызове функции TransmitData, данные в элемент управления будут
		// устанавливаться в соответствии с внутренним форматом этого элемента.
		// Иначе, функция будет считать что ей передали id и необходимо произвести поиск текста соответствующего этому id.
	uint8  Reserve[3]; // @alignment
	uint   MinSymbCount;
	uint   InputCtl;
	HWND   H_InputDlg;
	long   SelId;
	uint   OutCtlId;
	TDialog * P_OutDlg;
};
//
// Mouse button state masks
//
enum {
	mbLeftButton  = 0x01,
	mbRightButton = 0x02
};

struct KeyDownCommand { // @flat @noctr @size=4
	enum {
		stateAlt   = 0x0001,
		stateCtrl  = 0x0002,
		stateShift = 0x0004
	};
	//KeyDownCommand();
	KeyDownCommand & Z();
	bool   FASTCALL operator == (const KeyDownCommand & rS) const { return (State == rS.State && Code == rS.Code); }
	operator long() const { return *reinterpret_cast<const long *>(this); }
	int    GetKeyName(SString & rBuf, int onlySpecKeys = 0) const;
	int    SetKeyName(const char * pStr, uint * pLen);
	void   FASTCALL SetWinMsgCode(uint32 wParam);
	int    FASTCALL SetTvKeyCode(uint16 tvKeyCode);
	uint16 GetTvKeyCode() const;
	//
	// Descr: Транслирует символ chr во внутреннее состояние данного экземпляра класса в
	//   соответствии с текущей раскладкой клавиатуры.
	// Note: Позволяет превратить символ национального алфавита в скан-код.
	// Returns:
	//   !0 - функция выполнена успешно (внутреннее состояние изменилось)
	//    0 - error (внутреннее состояние не изменилось)
	//
	int    FASTCALL SetChar(uint chr);
	int    FASTCALL SetCharU(wchar_t chr);
	//
	// Descr: Преобразуем код виртуальной клавиши this->Code в символ.
	// Note: Функция "сырая" - преобразование только для латинских и специальных ASCII символов.
	// Returns:
	//   !0 - код символа, соответствующий виртуальной клавише
	//   0  - клавишу нельзя преобразовать в символ
	//
	uint   GetChar() const;

	uint16 State;
	uint16 Code;
};
//
// Descr: Если строка rSrc содержит non-ascii символы, то функция транслирует символы строки
//   в латинские в соответствии с клавиатурными кодами, которым соответствуют non-ascii знаки.
//
int    TranslateLocaleKeyboardTextToLatin(const SString & rSrc, SString & rResult);
//
// Descr: Структура, передаваемая с сообщением cmSetFont
//
struct SetFontEvent {
	SetFontEvent(void * pFontHandle, int doRedraw) : FontHandle(pFontHandle), DoRedraw(doRedraw)
	{
	}
	void * FontHandle;
	int    DoRedraw;
};

struct HelpEvent {
	enum {
		ctxtMenu = 1,
		ctxtWindow
	};
	int    ContextType; // ctxtXXX
	int    CtlId;
	void * H_Item;
	uint32 ContextId;
	SPoint2S Mouse;
};
//
// Descr: Структура, передаваемая с сообщением cmSize.
//
struct SizeEvent {
	enum {
		tMaxHide = 1,
		tMaximized,
		tMaxShow,
		tMinimized,
		tRestored
	};
	int    ResizeType; // tXXX тип события изменения размера.
	SPoint2S PrevSize; // Размеры окна до получения этого сообщения //
	SPoint2S NewSize;  // Новые размеры окна.
};
//
// Descr: Структура, передаваемая с сообщением cmPaint.
//
struct PaintEvent {
	PaintEvent();
	enum {
		tPaint = 1,
		tNcPaint,
		tEraseBackground
	};
	enum {
		fErase = 0x0001
	};
	int    PaintType;
	void * H_DeviceContext; // при PaintType == tPaint этот контекст нулевой.
		// HANDLE_EVENT должен самостоятельно получить контекст рисования у окна.
	long   Flags;
	TRect  Rect;
};
//
// Descr: Структура, передаваемая с сообщением cmMouse
//
struct MouseEvent {
	enum {
		tLDown = 1,  // Нажата левая кнопка мыши
		tLUp,        // Отпущена левая кнопка мыши
		tLDblClk,    // Двойной щелчек по левой кнопке мыши
		tRDown,      // Нажата правая кнопка мыши
		tRUp,        // Отпущена правая кнопка мыши
		tRDblClk,    // Двойной щелчек по правой кнопке мыши
		tMDown,      // Нажата средняя кнопка мыши (колесо)
		tMUp,        // Отпущена средняя кнопка мыши (колесо)
		tMDblClk,    // Двойной щелчек по средней кнопке мыши (колесу)
		tMove,       // Курсор мыши переместился //
		tWeel,       // Вращение колеса мыши //
		tHover,      // Курсор мыши замер на одном месте в течении заданного времени
		tLeave       // Курсор мыши вышел за пределы клиентской области окна
	};
	enum {
		fLeft      = 0x0001,
		fRight     = 0x0002,
		fMiddle    = 0x0004,
		fShift     = 0x0008,
		fControl   = 0x0010,
		fX1        = 0x0020,
		fX2        = 0x0040,
		fDrag      = 0x0080
	};
	int    Type;
	int    Flags;
	int    WeelDelta;
	SPoint2S Coord;
};
//
// Descr: Структура, передаваемая с сообщением cmScroll
//
struct ScrollEvent {
	enum {
		tBottom = 1,
		tTop,
		tEnd,
		tLineDown,
		tLineUp,
		tPageDown,
		tPageUp,
		tThumbPos,
		tThumbTrack
	};
	int    Dir;        // DIREC_HORZ || DIREC_VERT
	int    Type;
	IntRange Range;
	uint   PageSize;
	int    Pos;
	int    TrackPos;
	void * H_Wnd;
};
//
// Descr: Структура, передаваемая с сообщением cmDragndropObj
//
struct DragndropEvent {
	enum {
		acnGet = 1, // Окно получило курсор с перетаскиваемым объектом
		acnAccept,  // Пользователь опустил объект в окно (отпустил левую кнопку мыши в окне-приемнике)
		acnLeave    // Курсор с перетаскиваемым объектом ушел за пределы окна-приемника.
	};
	int    Action;
};
//
// Descr: Блок данных, посылаемый с сообщением cmNotifyForeignFocus
//
struct ForeignFocusEvent {
	TView * P_SrcView;
	long   FocusedItemIdent;
};

struct TEvent {
	//
	// Event codes
	//
	enum {
		evMouseDown = 0x0001,
		evMouseUp   = 0x0002,
		evMouseMove = 0x0004,
		evMouseAuto = 0x0008,
		evKeyDown   = 0x0010,
		evCommand   = 0x0100,
		evBroadcast = 0x0200,
		evWinCmd    = 0x0400, // @v9.6.5 @construction
		//
		// Event masks
		//
		evNothing   = 0x0000,
		evMouse     = 0x000f,
		evKeyboard  = 0x0010,
		evMessage   = 0xFF00
	};
	struct Mouse {
		uint8  buttons;
		int8   doubleClick;
		int16  WhereX;
		int16  WhereY;
	};
	struct KeyDown {
		struct Scan {
			uchar  charCode;
			uchar  scanCode;
		};
		union {
			ushort keyCode;
			Scan   charScan;
		};
		KeyDownCommand K;
	};
	struct Message {
		uint   command;
		union {
			void * infoPtr;
			TView * infoView;
			LPARAM  LP;
		};
		union {
			long    infoLong;
			ushort  infoWord;
			short   infoInt;
			uchar   infoByte;
			char    infoChar;
			WPARAM  WP;
		};
	};
	uint   what; // @firstmember
	union {
		Message message;
		Mouse   mouse;
		KeyDown keyDown;
	};

	TEvent();
	TEvent & setCmd(uint msg, TView * pInfoView);
	TEvent & setWinCmd(uint uMsg, WPARAM wParam, LPARAM lParam);
	uint getCtlID() const;
	bool FASTCALL isCmd(uint cmd) const;
	bool FASTCALL isKeyDown(uint keyCode) const;
	bool FASTCALL isCtlEvent(uint ctlID) const;
	bool FASTCALL isCbSelected(uint ctlID) const;
	bool FASTCALL isClusterClk(uint ctlID) const;
	bool FASTCALL wasFocusChanged(uint ctlID) const;
	bool wasFocusChanged2(uint ctl01, uint ctl02) const;
	bool wasFocusChanged3(uint ctl01, uint ctl02, uint ctl03) const;
	bool isCleared() const { return (what == evNothing && message.infoPtr != 0); } // @v12.2.6
	bool isCommandValidationFailed() const { return (what == evNothing && message.command == cmValidateCommand && message.infoPtr != 0); }
};

#define TVEVENT     event.what
#define TVCOMMAND   (event.what == TEvent::evCommand)
#define TVBROADCAST (event.what == TEvent::evBroadcast)
#define TVKEYDOWN   (event.what == TEvent::evKeyDown)
#define TVCMD       event.message.command
#define TVKEY       event.keyDown.keyCode
#define TVCHR       event.keyDown.charScan.charCode
#define TVSCN       event.keyDown.charScan.scanCode
#define TVINFOVIEW  event.message.infoView
#define TVINFOPTR   event.message.infoPtr
//
//
//
class TCommandSet {
public:
	TCommandSet();
	bool   IsEmpty() const;
	int    has(int cmd) const;
	void   enableAll();
	void   enableCmd(int cmd, bool toEnable);
	void   enableCmd(const TCommandSet&, bool toEnable);
	void   operator += (int cmd);
	void   operator -= (int cmd);
	void   operator += (const TCommandSet&);
	void   operator -= (const TCommandSet&);
	TCommandSet & operator &= (const TCommandSet&);
	TCommandSet & operator |= (const TCommandSet&);
	friend TCommandSet operator & (const TCommandSet&, const TCommandSet&);
	friend TCommandSet operator | (const TCommandSet&, const TCommandSet&);
	friend bool operator == (const TCommandSet& tc1, const TCommandSet& tc2);
	friend bool operator != (const TCommandSet& tc1, const TCommandSet& tc2);
private:
	int    loc(int);
	int    mask(int);

	uint32 cmds[64];
};

bool operator != (const TCommandSet& tc1, const TCommandSet& tc2);

// regex: (virtual)*[ \t]+void[ \t]+handleEvent\([ \t]*TEvent[ \t]*&[^)]*\)
#define DECL_HANDLE_EVENT      virtual void FASTCALL handleEvent(TEvent & event)
// regex: void[ \t]+([a-zA-Z0-9_]+)::handleEvent\([ \t]*TEvent[ \t]*&[^)]*\)
#define IMPL_HANDLE_EVENT(cls) void FASTCALL cls::handleEvent(TEvent & event)
#define EVENT_BARRIER(f) if(!EventBarrier()) { f; EventBarrier(1); }

#define DECL_DIALOG_DATA(typ) typedef typ DlgDataType; DlgDataType Data
#define DECL_DIALOG_SETDTS() int setDTS(const DlgDataType * pData)
#define DECL_DIALOG_GETDTS() int getDTS(DlgDataType * pData)
#define IMPL_DIALOG_SETDTS(cls) int cls::setDTS(const DlgDataType * pData)
#define IMPL_DIALOG_GETDTS(cls) int cls::getDTS(DlgDataType * pData)
//
// Descr: Контейнер для GDI-объектов. Удобен тем, что "бесконечное" количество предварительно
// созданных хандлеров можно затолкать в этот контейнер, а при рисовании манипулировать
// перечислением, элемент которого передается в функцию SPaintToolBox::Get() для получения //
// требуемого GDI-хандлера.
// Элементы в контейнере хранятся упорядоченно, по-этому, время извлечения очень мало.
// При разрушении контейнера все GDI-объекты, занесенные в него разрушаются.
// -------------------
//
// @construction {
// Универсальный (очень на это надеюсь) механизм управления скроллированием в одном измерении
//
// Будем рассматривать следующие режимы перемещения:
// по-страничный
//   при этом режиме фрейм перемещается так, что первый невидимый элемент, который предшествовал крайнему элементу страницы,
//   становится крайним элементом.
// по-строчный
// 
//
class SScroller {
public:
	DECL_INVARIANT_C();

	SScroller();
	SScroller(const SScroller & rS);
	~SScroller();
	SScroller & FASTCALL operator = (const SScroller & rS);
	SScroller & FASTCALL Copy(const SScroller & rS);
	SScroller & Z();
	//
	// Descr: Возвращает общее количество элементов под управлением экземпляра
	//
	uint   GetCount() const;
	//
	// Descr: Возвращает общий размер скроллируемой области.
	//
	float  GetSize() const { return P_ItemSizeList ? P_ItemSizeList->Sum() : (FixedItemSize * ItemCount); }
	//
	// Descr: Возвращает индекс текущей позиции.
	//
	uint   GetCurrentIndex() const;
	//
	// Descr: Возвращает значение текущей позиции в точках.
	//
	float  GetCurrentPoint() const;
	//
	// Descr: Возвращает индекс верхней позиции текущей страницы.
	//
	uint   GetCurrentPageTopIndex() const;
	//
	// Descr: Возвращает значение верхней позиции текущей страницы в точках.
	//
	float  GetCurrentPageTopPoint() const;
	//
	// Descr: Определяет, содержится ли индекс idx в видимой зоне скроллирования согласно внутреннему списку P_LineContent.
	// Note: Аргумент idx представлен знаковым целым числом поскольку контейнер P_LineContent содержит значения такого же типа.
	// Returns:
	//   >0 - индекс idx содержится в видимой зоне
	//    0 - индекс idx не содержится в видимой зоне
	//   <0 - не возможно ответить на запрос поскольку внутренний список P_LineContent не определен либо инвалиден.
	//
	int    FASTCALL CheckLineContentIndex(long idx) const;
	int    LineDown(uint ic, bool moveCursor);
	int    LineUp(uint ic, bool moveCursor);
	int    PageDown(uint pc);
	int    PageUp(uint pc);
	int    Top();
	int    Bottom();
	//
	// Descr: Устанавливает индекс текущей позиции
	//
	int    SetCurrentIndex(uint idx);
	uint   GetPageBottomIndex(uint topIdx) const;
	uint   GetPageTopIndex(uint bottomIdx) const;
	//
	// Элементы скроллируемого списка индексируются номерами [0..(ItemCount-1)]
	//
	enum {
		fDisabled    = 0x0001,
		fUsePaging   = 0x0002,
		fReverse     = 0x0004,
		fUseCursor   = 0x0008,  // Если установлен, то применяется понятие текущего элемента (ItemIdxCurrent).
			// Текущий элемент может перемещаться в пределах страницы без сдвига фрейма.
			// При отсутствии текущего элемента любое движение вызывает смещение фрейма.
		fFirstPageMoveToEdge = 0x0010   // При по-страничном перемещении (PageDown, PageUp) первое перемещение
			// осуществлять до соответствующей границы текущей страницы.
	};
	struct SetupBlock {
		SetupBlock();
		uint   ItemCount;
		float  ViewSize;
		float  FixedItemSize;
		FloatArray ItemSizeList; // Если FixedItemSize > 0.0 то ItemSizeList игнорируется в противном случае
			// assert(ItemSizeList.getCount() == ItemCount)
		TSCollection <LongArray> LineContent; // @v11.7.12
	};
	int    Setup(const SetupBlock & rBlk);

	struct Position {
		Position();
		Position & Z();
		uint   ItemIdxPageTop;
		uint   ItemIdxCurrent;
	};

	int    GetPosition(Position & rP) const;
	int    SetPosition(const Position & rP);
private:
	uint   AdjustTopIdx(uint idx) const;
	//
	// Descr: Возвращает положение элемента с индексом idx относительно первого элемента в точках.
	//
	float  GetAbsolutePosition(uint idx) const;

	uint   Flags;
	uint   ItemCount;
	uint   PageCurrent;
	float  ViewSize;
	float  FixedItemSize;
	//uint   ItemIdxPageTop;
	//uint   ItemIdxCurrent;
	Position P;
	FloatArray * P_ItemSizeList; // Используется если размеры элементов отличаются один от другого
	TSCollection <LongArray> * P_LineContent; // Список якорных идентификаторов, ассоциированных с элементами
		// assert(!P_LineContent || P_LineContent->getCount() == ItemCount)
	TSCollection <LongArray> * P_PageContent; // Список якорных идентификаторов, ассоциированных со страницами
		// assert(!P_PageContent || P_PageContent->getCount() == PageCount)
};

#pragma pack(push, 1)
class SUiLayoutParam { // @persistent // @size=100
public:
	enum {
		fContainerRow          = 0x00000001, // Контейнер выстраивает дочерние элементы по строкам (ось X)
		fContainerCol          = 0x00000002, // Контейнер выстраивает дочерние элементы по колонкам (ось Y)
			// if fContainerRow && fContainerCol, then no direction
		fContainerReverseDir   = 0x00000004, // if horizontal then right-to-left, if vertical then bottom-to-top
		fContainerWrap         = 0x00000008, // Если все элементы не вмещаются в один ряд, то контейнер переносит последующие элементы на следующий ряд.
		fContainerWrapReverse  = 0x00000010, // ignored if !(Flags & fWrap)
		fEntryPositionAbsolute = 0x00000020, // else Relative
		fNominalDefL           = 0x00000040, // Определена номинальная граница LEFT элемента   (Nominal.a.X)
		fNominalDefT           = 0x00000080, // Определена номинальная граница TOP элемента    (Nominal.a.Y)
		fNominalDefR           = 0x00000100, // Определена номинальная граница RIGHT элемента  (Nominal.b.X)
		fNominalDefB           = 0x00000200, // Определена номинальная граница BOTTOM элемента (Nominal.b.Y)
		fEvaluateScroller      = 0x00000400, // @v11.0.3 процессинговый флаг, предписывающий рассчитывать параметры скроллинга.
			// Если флаг не установлен, то скроллинг рассчитываться точно не будет. Если установлен, то - в зависимости
			// от параметров контейнера. Рассчитанные параметры скроллинга сохраняются по указателю SUiLayout::Result::P_Scrlr.
		//
		// Следующие 4 флага не применяются для определения внутреннего состояния, но нужны для индикации результата вычислений
		// (в частности, функциями GetNominalRect, GetNominalRectWithDefaults).
		//
		fNominalSzX            = 0x00000800, // @v12.3.2 Определен номинальный размер X.
		fNominalSzY            = 0x00001000, // @v12.3.2 Определен номинальный размер Y.
		fDefaultSzX            = 0x00002000, // @v12.3.3 Применен горизонтальный размер по умолчанию
		fDefaultSzY            = 0x00004000, // @v12.3.3 Применен вертикальный размер по умолчанию
	};
	enum {
		alignAuto = 0,
		alignStretch,
		alignCenter,
		alignStart,
		alignEnd,
		alignSpaceBetween,
		alignSpaceAround,
		alignSpaceEvenly
	};
	//
	// Descr: Зоны притяжения лейаутов. Применяются для вычисления размещения элементов
	//   в неориентированных контейнерах.
	//   В комментариях к каждому варианту приведена пара значений для горизонтального (X) и 
	//   вертикального (Y) притяжения (Gravity), которой этот вариант соответствует.
	//
	//   Порядок следования зон в этом enum'е соответствует порядку, в котором
	//   будут размещаться элементы. То есть, сначала углы, потом стороны, и, что осталось, отдается центральным элементам.
	// @todo Сделать управляемый порядок размещения //	
	//
	enum { // @persistent
		areaUndef = -1,
		areaCornerLU = 0, // {SIDE_LEFT, SIDE_TOP} 
		areaCornerRU,     // {SIDE_RIGHT, SIDE_TOP}
		areaCornerRB,     // {SIDE_RIGHT, SIDE_BOTTOM}
		areaCornerLB,     // {SIDE_LEFT, SIDE_BOTTOM}
		areaSideU,        // {0, SIDE_TOP} {SIDE_CENTER, SIDE_TOP}
		areaSideR,        // {SIDE_RIGHT, 0} {SIDE_RIGHT, SIDE_CENTER}
		areaSideB,        // {0, SIDE_BOTTOM} {SIDE_CENTER, SIDE_BOTTOM}
		areaSideL,        // {SIDE_LEFT, 0} {SIDE_LEFT, SIDE_CENTER}
		areaCenter        // {0, SIDE_CENTER} {0, 0} {SIDE_CENTER, 0} {SIDE_CENTER, SIDE_CENTER}
	};
	//
	// Descr: Опции расчета размера
	//
	enum {
		szInvalid     = -1, // Специальное значение, в некоторых случаях возвращаемое для индикации ошибки
		szUndef       =  0, // Размер никак не определен. Должен быть определен во время пересчета (если возможно)
		szFixed       =  1, // assert(Size.x > 0.0f)
		szByContent   =  2, // Размер определяется содержимым
		szByContainer =  3, // Размер определяется по размеру контейнера
	};
	//
	// Descr: Возвращает специальную 4-байтную сигнатуру, идентифицирующую объект в потоках сериализации.
	//
	static constexpr uint32 GetSerializeSignature() { return 0x15DE0522U; }
	static uint   GetNominalRectWithDefaults(const SUiLayoutParam * pLp, FRect & rR, float defWidth, float defHeight);
	static uint   GetNominalRectWithDefaults(const SUiLayoutParam * pLp, TRect & rR, float defWidth, float defHeight);
	SUiLayoutParam();
	SUiLayoutParam(const SUiLayoutParam & rS);
	//
	// Descr: конструктор для контейнера с наиболее популярными аргументами.
	//
	explicit SUiLayoutParam(int direction, uint justifyContent = alignAuto, uint alignContent = alignAuto);
	SUiLayoutParam & FASTCALL operator = (const SUiLayoutParam & rS);
	bool   FASTCALL operator == (const SUiLayoutParam & rS) const;
	bool   FASTCALL operator != (const SUiLayoutParam & rS) const;
	SUiLayoutParam & Z();
	bool   FASTCALL Copy(const SUiLayoutParam & rS);
	bool   FASTCALL IsEq(const SUiLayoutParam & rS) const;
	//
	// Descr: Проверяет параметры (возможно, не все) блока на непротиворечивость.
	// Returns:
	//   !0 - блок находится в консистентном состоянии
	//    0 - некоторые параметры блока противоречивы
	//
	bool   Validate() const;
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);
	//
	// Descr: Возвращает заданный размер по оси X.
	//   Если SzX == szFixed, то по указателю pS присваивается фиксированный размер, иначе - 0.0f
	// Returns:
	//   if oneof4(SzX, szUndef, szFixed, szByContent, szByContainer) then SzX, else szInvalid.
	//
	int    GetSizeX(float * pS) const;
	//
	// Descr: Возвращает заданный размер по оси Y.
	//   Если SzY == szFixed, то по указателю pS присваивается фиксированный размер, иначе - 0.0f
	// Returns:
	//   if oneof4(SzY, szUndef, szFixed, szByContent, szByContainer) then SzY, else szInvalid.
	//
	int    GetSizeY(float * pS) const;
	//
	// Descr: Если SzX == szByContainer то по указателю pS присваивает эффективное значение
	//   размера, вычисленное с учетом размера контейнера containerSize.
	//   Если SzX != szByContainer или что-то не так с containerSize либо с Size.X, то
	//   по указателю pS ничего присвоено не будет.
	// Returns:
	//   !0 - корректно вычислен эффективный размер, зависящий от размера контейнера
	//    0 - размер элемента не зависит от размера контейнера либо что-то не так с размерами.
	//
	int    GetSizeByContainerX(float containerSize, float * pS) const;
	//
	// Descr: Если SzY == szByContainer то по указателю pS присваивает эффективное значение
	//   размера, вычисленное с учетом размера контейнера containerSize.
	//   Если SzY != szByContainer или что-то не так с containerSize либо с Size.Y, то
	//   по указателю pS ничего присвоено не будет.
	// Returns:
	//   !0 - корректно вычислен эффективный размер, зависящий от размера контейнера
	//    0 - размер элемента не зависит от размера контейнера либо что-то не так с размерами.
	//
	int    GetSizeByContainerY(float containerSize, float * pS) const;
	float  CalcEffectiveSizeX(float containerSize) const;
	float  CalcEffectiveSizeY(float containerSize) const;
	SPoint2F CalcEffectiveSizeXY(float containerSizeX, float containerSizeY) const;
	SUiLayoutParam & SetFixedSizeX(float s);
	SUiLayoutParam & SetFixedSizeY(float s);
	SUiLayoutParam & SetVariableSizeX(uint var/* szXXX */, float s);
	SUiLayoutParam & SetVariableSizeY(uint var/* szXXX */, float s);
	SUiLayoutParam & SetFixedSize(const TRect & rR);
	SUiLayoutParam & SetMargin(const FRect & rMargin);
	SUiLayoutParam & SetMargin(float allSidesMargin);
	SUiLayoutParam & SetGrowFactor(float growFactor);
	int    GetContainerDirection() const; // returns DIREC_HORZ || DIREC_VERT || DIREC_UNKN
	SUiLayoutParam & SetContainerDirection(int direc /*DIREC_XXX*/);
	static SString & MarginsToString(const FRect & rR, SString & rBuf);
	static int    MarginsFromString(const char * pBuf, FRect & rR);
	SString & SizeToString(SString & rBuf) const;
	int    SizeFromString(const char * pBuf);
	//
	// Descr: Определяет являются ли координаты по оси X фиксированными.
	//
	bool   IsNominalFullDefinedX() const;
	//
	// Descr: Определяет являются ли координаты по оси Y фиксированными.
	//
	bool   IsNominalFullDefinedY() const;
	void   CopySizeXParamTo(SUiLayoutParam & rDest) const;
	void   CopySizeYParamTo(SUiLayoutParam & rDest) const;
	//
	// Descr: Вспомогательная функция, возвращающая кросс-направление относительно заданного
	//   направления direction.
	//   Если direction == DIREC_HORZ, то возвращает DIREC_VERT; если direction == DIREC_VERT, то возвращает DIREC_HORZ.
	//   Если !oneof2(direction, DIREC_HORZ, DIREC_VERT) то возвращает DIREC_UNKN.
	//
	static int GetCrossDirection(int direction);
	//
	// Descr: Определяет является ли позиция элемента абсолютной вдоль направления direction.
	// ARG(direction IN): DIREC_HORZ || DIREC_VERT
	//
	bool   IsPositionAbsolute(int direction) const;
	//
	// Descr: Определяет является ли позиция элемента по оси Y абсолютной.
	//   Понятие "абсолютная позиция по оси" подразумевает, что либо заданы фиксированные 
	//   начальная и конечная координаты по оси, либо размер элемента по оси фиксирован (Sz(X|Y)==szFixed) и фиксирована
	//   хотя бы одна из координат по оси.
	//
	bool   IsPositionAbsoluteX() const;
	//
	// Descr: Функция возвращает по ссылке rR номинальные границы объекта, если это возможно.
	// Result:
	//   Набор из флагов fNominalDefL, fNominalDefT, fNominalDefR, fNominalDefB в зависимости
	//   от того какие координаты и размеры явно определены.
	//
	uint   GetNominalRect(FRect & rR) const;
	float  GetAbsoluteLowX() const;
	float  GetAbsoluteLowY() const;
	float  GetAbsoluteSizeX() const;
	float  GetAbsoluteSizeY() const;
	//
	// Descr: Определяет является ли позиция элемента по оси Y абсолютной.
	//   Понятие "абсолютная позиция по оси" подразумевает, что либо заданы фиксированные 
	//   начальная и конечная координаты по оси, либо размер элемента по оси фиксирован (Sz(X|Y)==szFixed) и фиксирована
	//   хотя бы одна из координат по оси.
	//
	bool   IsPositionAbsoluteY() const;
	//
	// Descr: Возвращает зону притяжения элемента (SUiLayoutParam::areaXXX) в зависимости от значений
	//   полей {GravityX, GravityY}
	//
	int    GetVArea() const;
	//
	// Descr: Устанавливает атрибуты GravityX и GravityY в зависимости от параметра
	//   area.
	// Returns:
	//   >0 - значения GravityX и GravityY успешно установлены. При этом они изменились.
	//   <0 - значения GravityX и GravityY успешно установлены, но ничего при этом не изменилось (они такими же и были).
	//    0 - error (аргумент area не валиден либо что-то не так с внутренним состоянием объекта).
	//
	int    SetVArea(int area);
	static int RestrictVArea(int restrictingArea, const FRect & rRestrictingRect, int restrictedArea, FRect & rRestrictedRect);

	const uint32 Signature; // @v12.3.2 for serialization
	const uint32 Ver;       // @v12.3.2 for serialization
	uint32 Flags;          // @flags fXXX
	uint16 SzX;            // SUiLayoutParam::szXXX Опции расчета размера по оси X
	uint16 SzY;            // SUiLayoutParam::szXXX Опции расчета размера по оси Y
	uint16 JustifyContent; // {alignStart}   SUiLayoutParam::alignXXX Выравнивание внутренних элементов вдоль основной оси
	uint16 AlignContent;   // {alignStretch} SUiLayoutParam::alignXXX Выравнивание внутренних элементов по кросс-оси
	uint16 AlignItems;     // {alignStretch} SUiLayoutParam::alignXXX
	uint16 AlignSelf;      // {alignAuto}    SUiLayoutParam::alignXXX
	uint16 GravityX;       // Gravity of this entry by X-axis. 0 || SIDE_LEFT || SIDE_RIGHT || SIDE_CENTER
	uint16 GravityY;       // Gravity of this entry by Y-axis. 0 || SIDE_TOP || SIDE_BOTTOM || SIDE_CENTER 
	uint16 LinkRelation;   // @v12.3.2 SIDE_XXX Положение относительно связанного объекта. Сильно контекстно-зависимое поле. 
	uint16 Reserve;        // @v12.3.2 @alignment
	int32  Order;          // Порядковый номер элемента в линейном ряду потомков одного родителя //
	FRect  Nominal;        // Номинальные границы элемента. Заданы или нет определяется флагами fNominalDefL, fNominalDefT, fNominalDefR, fNominalDefB
	SPoint2F Size;         // Номинальный размер элемента. Если SzX != szFixed, то Size.X игнорируется, аналогично, если SzY != szFixed, то Size.Y игнорируется
	FRect  Padding;        // { 0.0f, 0.0f, 0.0f, 0.0f } Внешние поля элемента
	FRect  Margin;         // { 0.0f, 0.0f, 0.0f, 0.0f } Внутренние поля контейнера
	float  GrowFactor;     // Доля от размера всех элементов контейнера по продольной оси (определяемой флагами fContainerRow и fContainerCol)
	float  ShrinkFactor;   //
	float  Basis;          //
	float  AspectRatio;    // {0.0} Отношение высоты к ширине. Используется в случае, если одна из размерностей не определена
	SPoint2F MinSize;      // @v12.3.6 Минимальные размеры лейаута по ширине и высоте. Если значение в какой-то координате <= 0.0f, то значит в этом измерении ограничения нет
	uint8  Reserve2[24];   // @v12.3.2 // @v12.3.6 [32]-->[24]
private:
	int    ParseSizeStr(const SString & rStr, float & rS) const;
};
#pragma pack(pop)

class SUiLayout {
	friend class LayoutFlexProcessor;
public:
	//
	// Descr: Специализированная коллекция ссылок на лейауты, не владеющая этими ссылками
	//   (SCollection не содержит флага aryEachItem).
	//
	class RefCollection : private SCollection {
	public:
		RefCollection();
		RefCollection & Z();
		void   Add(const SUiLayout * pLo);
		uint   GetCount() const;
		const SUiLayout * Get(uint idx) const;
	};

	struct Result { // @persistent @size=24+sizeof(void *)
		enum {
			fNotFit = 0x0001, // Элемент не уместился в контейнер
			fDegradedWidth  = 0x0002, // Ширина элемента деградировала (меньше или равна 0)
			fDegradedHeight = 0x0004  // Высота элемента деградировала (меньше или равна 0)
		};
		Result();
		Result(const Result & rS);
		~Result();
		Result & FASTCALL operator = (const Result & rS);
		operator FRect() const;
		Result & CopyWithOffset(const Result & rS, float offsX, float offsY);
		float  Frame[4];
		uint32 Flags;
		uint32 Reserve;
		SScroller * P_Scrlr;
	};
	struct IndexEntry {
		IndexEntry();
		uint32 ItemIdx; // Индекс элемента в TSCollection::this
		uint32 HglIdx;  // Индекс[1..] элемента в P_HgL. Если SVectorBase::GetCount(P_HgL) == 0, то 0
		Result R;       // Результат размещения
	};
	// size[0] == width, size[1] == height
	typedef void (__stdcall * FlexSelfSizingProc)(const SUiLayout * pItem, float size[2]);
	typedef void (__stdcall * FlexSetupProc)(SUiLayout * pItem, /*float size[4]*/const SUiLayout::Result & rR);

	static void * GetManagedPtr(SUiLayout * pItem);
	static void * GetParentsManagedPtr(SUiLayout * pItem);

	SUiLayout();
	SUiLayout(const SUiLayout & rS);
	SUiLayout(const SUiLayoutParam & rP);
	~SUiLayout();
	bool   IsConsistent() const;
	bool   FASTCALL IsEq(const SUiLayout & rS) const;
	bool   FASTCALL operator == (const SUiLayout & rS) const { return IsEq(rS); }
	bool   FASTCALL operator != (const SUiLayout & rS) const { return !IsEq(rS); }
	SUiLayout & FASTCALL operator = (const SUiLayout & rS) { return Copy(rS); }
	SUiLayout & Copy(const SUiLayout & rS);
	//
	// Descr: Каноническая функция возвращающая ключ экземпляра для хэширования.
	//
	const void * GetHashKey(const void * pCtx, uint * pKeyLen) const;
	SJson * ToJsonObj() const;
	int   FromJsonObj(const SJson * pJs);
	//
	// Descr: Если экземпляр this имеет родителя, то обращается к родительскому
	//   объекту для того, что бы тот удалил его.
	// Returns:
	//   >0 - this имеет родителя и тот успешно убил его. В этом случае указатель this более
	//     не действителен.
	//   0  - this не имеет родителя (либо родитель не имеет в своем списке ссылки на this, что есть сбойная ситуация) и должен совершить суицид самостоятельно.
	//
	int    FatherKillMe();
	uint   GetChildrenCount() const;
	SUiLayout * GetChild(uint idx);
	const  SUiLayout * GetChildC(uint idx) const;
	void   SetCallbacks(FlexSelfSizingProc selfSizingCb, FlexSetupProc setupCb, void * managedPtr);
	int    SetLayoutBlock(const SUiLayoutParam & rAlb);
	SUiLayoutParam & GetLayoutBlock() { return ALB; }
	const SUiLayoutParam & GetLayoutBlockC() const { return ALB; }
	SUiLayout * GetParent() { return P_Parent; }

	struct Param {
		enum {
			fPaginate        = 0x0001, // Рассчитывать раскладку по страницам.
			fStopOnFirstUnfittedItem = 0x0002  // Остановить расчет на первом невместившемся элементе
		};
		Param() : Flags(0), FirstItemIndex(0)/*, ForceWidth(0.0f), ForceHeight(0.0f)*/
		{
		}
		uint   Flags;
		uint   FirstItemIndex; // Индекс элемента [0..], с которого начинать расчет
		SPoint2F ForceSize;
	};
	int    Evaluate(const Param * pP);
	SUiLayout * InsertItem();
	SUiLayout * InsertItem(void * pManagedPtr, const SUiLayoutParam * pAlb, int id = 0);
	SUiLayout * Insert(SUiLayout * pOriginalLayout);
	SUiLayout * InsertCopy(const SUiLayout * pOriginalLayout);
	void   DeleteItem(uint idx);
	void   DeleteAllItems();
	int    GetOrder() const;
	void   SetOrder(int o);
	int    GetID() const;
	//
	// Descr: Прямым образом устанавливает идентификатор элемента.
	//   Функция проверяет передаваемое значение на предмет того, чтобы оно было больше нуля.
	// Note: Функция не проверяет уникальность идентификатора, потому клиент класса обязан 
	//   сам побеспокоиться об уникальности устанавливаемого значения.
	// 
	// Returns:
	//   >0 - Установленное значение идентификатора
	//    0 - Ошибка
	//
	int    SetID(int id);
	const  SString & GetSymb() const;
	int    SetSymb(const char * pSymb);
	//
	// Descr: Делает элемент исключенным из списка дочерних элементов родителя (не удаляя его 'физически').
	//   То есть пересчет лейаутов будет работать так, будто этого элемента нет вообще.
	// Returns:
	//   true - статус элемента изменился //
	//   false - статус элемента не изменился (возможно, он уже был исключенным или ошибка какая-то).
	//
	bool   SetExcludedStatus();
	//
	// Descr: Возвращает элементу статут присутствия в списке дочерних элементов родителя.
	// Returns:
	//   true - статус элемента изменился //
	//   false - статус элемента не изменился (возможно, он уже был исключенным или ошибка какая-то).
	//
	bool   ResetExcludedStatus();
	//
	// Descr: Возвращает true если элемент исключен из рассмотрения среди дочерних элементов родителя.
	//
	bool   IsExcluded() const;
	//
	// Descr: Если идентификатора элемента (ID) нулевой, то инициализирует его так, чтобы он был уникальным
	//   в области определения контейнера верхнего уровня.
	//   Если идентфикатор уже не нулевой, то просто возвращает его значение.
	//
	int    SetupUniqueID();
	//
	// Descr: Функция ищет дочерний элемент рекурсивно по всему дереву лейаутов, начиная с this.
	//   Поиск осуществляется по критерию эквивалентности идентификатора элемента заначению параметра id.
	//   constant-версия.
	//
	const  SUiLayout * FindByIdC(int id) const;
	//
	// Descr: Функция ищет дочерний элемент рекурсивно по всему дереву лейаутов, начиная с this.
	//   Поиск осуществляется по критерию эквивалентности идентификатора элемента заначению параметра id.
	//   nonconstant-версия.
	//
	SUiLayout * FindById(int id);
	const  SUiLayout * FindBySymbC(const char * pSymb) const;
	SUiLayout * FindBySymb(const char * pSymb);
	//
	// Descr: Функция пытается вычислить минимальные допустимые размеры лейаута.
	// ARG(pMinSize OUT): @#vptr0 Указатель на структуру, по которому присваиваются найденные
	//   значения минимальных размеров по осям X и Y. Если по какой-либо из осей не удалось
	//   определить минимум, то соответствующее поле будет равно 0.0f.
	// Returns:
	//   0    - не удалось определить минимальный размер ни по одной из размерностей
	//   0x01 - удалось определить минимальный размер по оси X
	//   0x02 - удалось определить минимальный размер по оси Y
	//   0x03 - удалось определить минимальный размер по обеим осям координат
	//
	uint  GetMinSize(SPoint2F * pMinSize) const;
	//
	// Descr: Вычисляет полную ширину элемента без рассмотрения его внутренних компонентов.
	//   Полная ширина включает собственно ширину, а так же левые и правые поля и набивки
	//   (margin_left, margin_right, padding_left, padding_right).
	// Returns:
	//   !0 - номинальная ширина элемента представлена валидным числом (!fisnan(width)).
	//      В этом случае по адресу pS присваивается полная ширина элемента.
	//    0 - номинальная ширина элемента представлена инвалидным значением (fisnan(width)).
	//      В этом случае по адресу pS ничего не присваивается и значение по указателю остается неизменным.
	//
	int    GetFullWidth(float * pS) const;
	//
	// Descr: Вычисляет полную высоту элемента без рассмотрения его внутренних компонентов.
	//   Полная высота включает собственно ширину, а так же верхние и нижние поля и набивки
	//   (margin_top, margin_bottom, padding_top, padding_bottom).
	// Returns:
	//   !0 - номинальная высота элемента представлена валидным числом (!fisnan(height)).
	//      В этом случае по адресу pS присваивается полная высота элемента.
	//    0 - номинальная высота элемента представлена инвалидным значением (fisnan(height)).
	//      В этом случае по адресу pS ничего не присваивается и значение по указателю остается неизменным.
	//
	int    GetFullHeight(float * pS) const;
	//
	// Descr: Возвращает финальный расчетный прямоугольник элемента.
	//
	FRect  GetFrame() const;
	//
	// Descr: Возвращаете финальный расчетный прямоугольник элемента, скорректированный относительно
	//   родительских прямоугольников.
	//
	FRect  GetFrameAdjustedToParent() const;
	//
	// Descr: Ищет элемент с наименьшим размером, в котором находится точка {x, y}.
	// Note: Функция предполагает, что если точка не попадает в родительский элемент, то не попадает и 
	//   ни в один из дочерних элементов. В общем случае, это - сильное и не обязательно истинное утверждение.
	// Returns:
	//   !0 - указатель на минимальный элемент, включающий точку {x, y}
	//    0 - нет ни одного элемента, включающего точку {x, y}
	//
	const  SUiLayout * FindMinimalItemAroundPoint(float x, float y) const;
	//
	// Descr: Рассчитывает минимальный прямоугольник, охватывающий все дочерние элементы
	//   контейнера. Размеры самого контейнера в расчет не принимаются.
	// Returns:
	//   1 - все дочерние элементы имеют определенный размер
	//  -1 - контейнер не имеет элементов 
	//  -2 - только часть дочерних элементов имеет определенный размер
	//   0 - error (черт его знает, что там еще может произойти)
	//
	int    GetInnerCombinedFrame(FRect * pResult) const;
	//
	// Descr: Возвращает корневой элемент дерева, компонентом которого является this.
	//
	SUiLayout * GetRoot();
	const  Result & GetResultC() const { return R; }
	Result & GetResult_() { return R; }
	//
	// Descr: Гомогенный элемент. Вектор таких элементов (HomogeneousList) заменяет множество 
	//   однообразных элементов. За счет использования гомогенных списков я рассчитываю получить
	//   значительное ускорение и упрощенние работы в ряде частных случаев.
	//   За исключением опционального параметра Vf все остальные атрибуты элемента определяются
	//   элементом-владецем.
	//
	struct HomogeneousEntry {
		long   ID; // Уникальный (среди элементов HomogeneousArray) идентификатор
		float  Vf; // Значение изменяемого фактора, определяемого параметром HomogeneousArray::VariableFactor
	};
	struct HomogeneousArray : public TSVector <HomogeneousEntry> {
		enum {
			vfNone = 0,
			vfFixedSizeX,
			vfFixedSizeY,
			vfGrowFactor
		};
		HomogeneousArray();
		HomogeneousArray(const HomogeneousArray & rS);
		HomogeneousArray & FASTCALL operator = (const HomogeneousArray & rS);

		uint   VariableFactor;
	};

	int    InitHomogeneousArray(uint variableFactor /* HomogeneousArray::vfXXX */);
	int    AddHomogeneousEntry(long id, float vf);
	//
	// Descr: Типы комплексных лейаутов
	//
	enum { // @persistent
		cmplxtNone       = 0,
		// Для следующих 3 комплексов фиксированное базовое измерение - высота поля ввода
		cmplxtInpLbl     = 1, // Комплексный layout { [input-line] [label] }
		cmplxtInpLblBtn  = 2, // Комплексный layout { [input-line] [label] [square-button] }
		cmplxtInpLblBtn2 = 3, // Комплексный layout { [input-line] [label] [square-button] [square-button] }
	};
	//
	// Desc: Зарезервированные метки компонентов комплексного layout'а
	//
	enum {
		cmlxcUndef   = 0,
		cmlxcInput   = 1, 
		cmlxcLabel   = 2,
		cmlxcButton1 = 3,
		cmlxcButton2 = 4,
	};

	SUiLayout * FASTCALL FindComplexComponentId(uint id);
	//
	// Descr: Опции комплексных layout'ов
	//
	enum {
		clfLabelLeft = 0x0001, // Этикетка располагается слева от основного элемента. Иначе - сверху
	};
	//
	// Descr: Создает комплексный layout типа type.
	//   Внешние размеры результата не определены (должны быть установлены вызывающей функцией)
	//
	static SUiLayout * CreateComplexLayout(int type/*cmplxtXXX*/, uint flags/*clfXXX*/, float baseFixedMeasure, SUiLayout * pTopLevel);
protected:
	void   UpdateShouldOrderChildren();
	void   DoLayout(const Param & rP) const;
	void   DoFloatLayout(const Param & rP);
	void   DoLayoutChildren(uint childBeginIdx, uint childEndIdx, uint childrenCount, /*LayoutFlexProcessor*/void * pLayout, SScroller::SetupBlock * pSsb) const;
	//
	// Descr: Завершает обработку искусственного элемента pCurrentLayout, устанавливает координаты его дочерних элементов
	//   с поправкой на rOffs и разрушает pCurrentLayout.
	//
	void   Helper_CommitInnerFloatLayout(SUiLayout * pCurrentLayout, const SPoint2F & rOffs) const;
	/*flex_align*/int FASTCALL GetChildAlign(const SUiLayout & rChild) const;
private:
	class IterIndex : public TSVector <IndexEntry> {
	public:
		IterIndex();
		SUiLayout * GetHomogeneousItem(const SUiLayout * pBaseItem, uint hgeIdx) const;
		void   ReleaseHomogeneousItem(SUiLayout *) const;
	private:
		mutable TSCollection <SUiLayout> HomogeneousEntryPool;
	};
	//
	// Методы для внутренней индексации элементов контейнера.
	// Идея такая: 
	//   -- вызываем MakeIndex - получаем индекс всех элементов контейнера включая неявные гомогенные элементы 
	//   -- при построении раскладки обращаемся к элементам строго посредством GetChildByIndex
	//   -- закончив раскладку возвращаем контейнеру элементы вызовами CommitChildResult
	// Замечания:
	//   -- индексация учитывает атрибут ALB.Order
	//   -- представления гомогенных элементов физически в контейнере не существуют, потому SetChildByIndex
	//     будет реализовать не просто.
	//   -- пока имеет место путаница насчет того какие поля SUiLayout меняются функциями расчета, а 
	//     какие нет. Потому я в некотором замешательстве.
	//
	void   MakeIndex(IterIndex & rIndex) const;
	bool   IsThereNotExcludedChildren() const;
	//
	// Descr: Получает указатель на элемент по позиции idxPos в индексе rIndex.
	//   Если элемент является гомоморфной коллекцией, то возвращается указатель на суррогатный экземпляр.
	// Returns:
	//   0 - error. Скорее всего, инвалидный индекс.
	//
	const  SUiLayout * GetChildByIndex(const IterIndex & rIndex, uint idxPos) const;
	int    CommitChildResult(const IterIndex & rIndex, uint idxPos, const SUiLayout * pItem);
	//
	// Descr: Вызывается после завершения расчета элемента
	//   Вызывает DoLayout(R.Frame[2], R.Frame[3])
	//
	void   Commit_() const;
	int    SetupResultScroller(SScroller::SetupBlock * pSb) const; // Result is mutable
	bool   LayoutAlign(/*flex_align*/int align, float flexDim, uint childrenCount, float * pPos, float * pSpacing, bool stretchAllowed) const;
	//
	// Descr: Проходит по всех дочерним элементам и находит максимальное значение идентификатора.
	//   Функция нужна для автоматического присвоения уникального идентификатора одному из элементов (see func SetupUniqueID)
	//
	int    GetMaxComponentID() const;

	enum {
		setupfChildrenOnly = 0x0001 // Вызывать callback-функцию CbSetup только для дочерних элементов, но не для самого себя //
	};
	//
	// Descr: Вызывает функцию CbSetup для самого себя и рекурсивно для дочерних элементов
	// ARG(flags IN)
	//
	void   Setup(uint flags);
	//
	enum {
		stShouldOrderChildren = 0x0001,
		stHomogeneousItem     = 0x0002, // Элемент является виртуальным экземпляром, сформированным по шаблону из HomogeneousArray основного элемента
		stExcluded            = 0x0004, // @v12.3.7 Элемент исключается из рассмотрения при пересчете. 
	};
	uint32 Signature; // non const. Инициализиуется в ctr, обнуляется в dtr.
	SUiLayoutParam ALB;
	void * managed_ptr; // NULL // An item can store an arbitrary pointer, which can be used by bindings as the address of a managed object.
		// An item can provide a self_sizing callback function that will be called
		// during layout and which can customize the dimensions (width and height) of the item.
	FlexSelfSizingProc CbSelfSizing; // NULL
	FlexSetupProc CbSetup; // NULL
	SUiLayout * P_Parent;
	uint   State;
	uint16 CplxComponentId;
	uint16 Reserve;
	int    ID; // @v11.6.12 Идентификатор экземпляра. Определяется создающим клиентом класса.
	SString Symb; // @v11.7.9 Символьный идентификатор экземпляра.
	const  SUiLayout * P_Link; // @transient При сложных схемах построения формируются искусственные лейауты, получающие
		// в этом поле ссылку на порождающий реальный элемент. 
	TSCollection <SUiLayout> * P_Children;
	HomogeneousArray * P_HgL;
	mutable Result R;
};
//
// Descr: Набор цветов для пользовательского интерфейса. Цвет идентифицируется либо символом либо целочисленным идентификатором.
//   Должно быть задано либо то, либо это.
// 
class SColorSet : private SStrGroup { // @v11.7.10
public:
	//
	// Descr: Идентификаторы функций формирования цветов
	//
	enum {
		funcNone = 0,
		funcEmpty,     // ZEROCOLOR
		funcLerp,      // (color, color, factor)
		funcLighten,   // (color, factor)
		funcDarken,    // (color, factor)
		funcGrey,      // (whitePart)
	};
	explicit SColorSet(const char * pSymb = 0);
	bool   FASTCALL IsEq(const SColorSet & rS) const;
	bool   FASTCALL operator == (const SColorSet & rS) const { return IsEq(rS); }
	bool   FASTCALL operator != (const SColorSet & rS) const { return !IsEq(rS); }
	SColorSet & Z();
	const void * GetHashKey(const void * pCtx, uint * pKeyLen) const; // Descr: Каноническая функция возвращающая ключ экземпляра для хэширования.
	int    SetSymb(const char * pSymb);
	const  SString & GetSymb() const { return Symb; }
	SJson * ToJsonObj() const;
	int    FromJsonObj(const SJson * pJs);
	int    Get(const char * pSymb, const TSCollection <SColorSet> * pSetList, SColor & rC) const;
	int    Resolve(const TSCollection <SColorSet> * pSetList);
	bool   ValidateRefs(const TSCollection <SColorSet> * pSetList) const;
	//
	int    Test();
private:
	enum {
		argtNone = 0,
		argtAbsoluteColor,
		argtRefColor,
		argtNumber
	};
	struct ColorArg {
		ColorArg();
		bool FASTCALL IsEq(const ColorArg & rS) const { return (C == rS.C && F == rS.F && RefSymb.IsEqiAscii(rS.RefSymb)); }
		bool FASTCALL operator == (const ColorArg & rS) const { return IsEq(rS); }
		bool FASTCALL operator != (const ColorArg & rS) const { return !IsEq(rS); }
		ColorArg & Z();
		SString & ToStr(SString & rBuf) const;
		//
		// Descr: Возвращает тип аргумента (SColorSet::argtXXX)
		//
		int    GetType() const;

		SColor C;
		float  F;
		SString RefSymb; // Символ цвета, определенного где-то в ином месте.
			// Этот символ может быть представлен префиксом символа набора цветов (setssymb.colorsymb)
			// для того, чтобы можно было ссылаться на набор отличный от того, где 
			// собственно определен цвет, использующий ссылку.
			// Простая ссылка (colorsymb) указывает на цвет, определенный в текущем наборе.
	};
	struct ComplexColorBlock {
		ComplexColorBlock();
		ComplexColorBlock(const ComplexColorBlock & rS);
		bool   FASTCALL IsEq(const ComplexColorBlock & rS) const;
		ComplexColorBlock & FASTCALL operator = (const ComplexColorBlock & rS);
		ComplexColorBlock & Copy(const ComplexColorBlock & rS);
		ComplexColorBlock & Z();
		SString & ToStr(SString & rBuf) const;

		SColor C;
		SString RefSymb; // Символ другого цвета в коллекции
		int   Func;
		TSCollection <ColorArg> ArgList;
	};
	struct InnerEntry {
		InnerEntry();
		const void * GetHashKey(const void * pCtx, uint * pKeyLen) const; // Descr: Каноническая функция возвращающая ключ экземпляра для хэширования.
		uint   SymbP;
		SColor C;
		uint   CcbP; // Ссылка на позицию в CcC [1..CcC.getCount()]
	};
	// 
	// Returns:
	//   0 - error
	//   1 - color
	//   2 - number
	//   3 - reference
	//
	int    Helper_ParsePrimitive(SStrScan & rScan, ColorArg & rItem) const;
	int    ParseComplexColorBlock(const char * pText, ComplexColorBlock & rBlk) const;
	int    ResolveComplexColorBlock(const ComplexColorBlock & rBlk, const TSCollection <SColorSet> * pSetList, SColor & rC, StringSet & rRecurSymbList) const;
	//
	// Descr: Вставляет элемент набора с символом pSymb и значением pBlk.
	//   Объект по указателю pBlk переходит в полное владение функцией. В случае успеха, он будет включен в
	//   коллекцию CcC, в случае неудачи или если содержит простой абсолютный цвет - удален.
	//   Таким образом, клиент функции должен объект, передаваемый по указателю pBlk распределить динамически (new)
	//
	int    Put(const char * pSymb, ComplexColorBlock * pBlk);
	int    Get(const char * pSymb, ComplexColorBlock * pBlk) const;
	int    Get(const char * pSymb, const TSCollection <SColorSet> * pSetList, ComplexColorBlock * pBlk) const;
	int    Helper_Get(const char * pSymb, const TSCollection <SColorSet> * pSetList, SColor & rC, StringSet * pRecurSymbList) const;
	bool   FASTCALL IsInnerEntryEq(const InnerEntry & rE1, const SColorSet & rS, const InnerEntry & rE2) const;

	enum {
		stateResolved = 0x0001
	};
	uint   State; // @transient 
	SString Symb; // Символ набора цветов (не путать с символом отдельного цвета)
	TSHashCollection <InnerEntry> L;
	TSCollection <ComplexColorBlock> CcC;
};
//
// Descr: Цветовая тема пользовательского интерфейса
// 
class SColorTheme { // @v11.7.10 @construction
public:
	SColorTheme() : Id(0)
	{
	}
	SColorTheme & Copy(const SColorTheme & rS);
	const void * GetHashKey(const void * pCtx, uint * pKeyLen) const; // Descr: Каноническая функция возвращающая ключ экземпляра для хэширования.
private:
	uint   Id;
	SString Symb;
	TSCollection <SColorSet> CsL;
};
//
// Descr: Источник извлечения шрифта. Инициирующая причина ввода: загрузка шрифтов для проекта WsCtl (ImGui)
// 
class SFontSource { // @v11.7.10 @construction
public:
	SFontSource();
	bool   FASTCALL IsEq(const SFontSource & rS) const;
	bool   FASTCALL operator == (const SFontSource & rS) const { return IsEq(rS); }
	bool   FASTCALL operator != (const SFontSource & rS) const { return !IsEq(rS); }
	SFontSource & Z();
	const void * GetHashKey(const void * pCtx, uint * pKeyLen) const; // Descr: Каноническая функция возвращающая ключ экземпляра для хэширования.
	SJson * ToJsonObj() const;
	int    FromJsonObj(const SJson * pJs);

	SString Face;
	SString Src;
};
//
// Descr: Определитель шрифта
//
class SFontDescr { // @persistent
public:
	enum {
		fItalic    = 0x0001,
		fUnderline = 0x0002,
		fStrikeOut = 0x0004,
		fBold      = 0x0008, // Если Weight == 0.0f и Flags & fBold, то неявно применяется значение
			// Weight такое, чтобы шрифт выглядел утолщенным. Если Weight > 0.0, то этот флаг игнорируется.
		fAntialias = 0x0010
	};

	SFontDescr();
	SFontDescr(const char * pFace, int size, int flags);
	SFontDescr & Z();
	bool   FASTCALL operator == (const SFontDescr & rS) const { return IsEq(rS); }
	bool   FASTCALL IsEq(const SFontDescr & rS) const;
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    ToStr(SString & rBuf, long fmt) const;
	int    FASTCALL FromStr(const char *);
	int    FASTCALL SetLogFont(const LOGFONTA * pLf);
	int    FASTCALL SetLogFont(const LOGFONTW * pLf);
	LOGFONTA * FASTCALL MakeLogFont(LOGFONTA * pLf) const;
	LOGFONTW * FASTCALL MakeLogFont(LOGFONTW * pLf) const;
	SJson * ToJsonObj() const;
	int    FromJsonObj(const SJson * pJs);
	//
	// Descr: Преобразует json-массив pJs в коллекцию описаний шрифтов
	// Note: Список rList функцией не очищается перед исполнением (то есть, в него будут добавляться новые элементы, а те, что были останутся на месте)
	//
	static bool ListFromJsonArray(const SJson * pJs, TSCollection <SFontDescr> & rList);
	static SJson * ListToJsonArray(const TSCollection <SFontDescr> & rList);

	int16  Size;        // @anchor Логический размер шрифта в пикселях
	int16  Flags;       // @flags SFontDescr::fXXX
	float  Weight;      // Толщина шрифта. 0.0f - не важно, 1.0f - нормальный, 2.0f - максимально толстый.
	uint8  CharSet;     // Набор символов XXX_CHARSET (win32)
	uint8  Reserve[15];
	SString Face;       // @anchor
	SString Symb;       // @v11.9.10 Символ для идентификации этого экземпляра среди коллекции аналогичных
	SString ColorRefSymb; // @v11.9.10 Символ цвета шрифта. Введен для использования в стилях UI (UiDescription)
		// @todo Пока не сериализуется функцией Serialize поскольку для этого необходимо глубокое изменение формата сериализации во всем проекте.
private:
	int    FASTCALL Helper_SetLogFont(const void * pLf);
	int    FASTCALL Helper_MakeLogFont(void * pLf) const;
};
//
// Descr: Список скалярных величин, устанавливаемых в json-конфигурации интерфейса
//
class UiValueList { // @v11.9.4
public:
	enum {
		vStandaloneListWidth = 1,
		vStandaloneListHeight,
		vDesktopIconSize,
		vDesktopIconGap,
		vButtonStdHeight,
		vButtonStdWidth,
		vButtonDoubleWidth,
		vFontSmoothingType, // @v12.2.1 0=none; 1=standard; 2=cleartype
		vFontSmoothingContrast, // @v12.2.1 [1000..2200] default=1400 SPI_SETFONTSMOOTHINGCONTRAST
	};
	union ValueUnion {
		ValueUnion();
		bool   FASTCALL IsEq(const ValueUnion & rS) const;
		int    I;
		double R;
		char   T[128]; // utf-8
	};
	struct Entry { // @flat
		Entry();
		bool   FASTCALL IsEq(const Entry & rS) const;
		bool   FASTCALL operator == (const Entry & rS) const { return IsEq(rS); }
		uint   Id;
		ValueUnion V;
	};
	UiValueList();
	~UiValueList();
	bool   FASTCALL IsEq(const UiValueList & rS) const;
	UiValueList & Z();
	uint   GetCount() const { return L.getCount(); }
	int    Put(uint id, double v);
	int    Get(uint id, double & rV) const;
	int    Put(uint id, int v);
	int    Get(uint id, int & rV) const;
	int    Put(uint id, const char * pV);
	int    Get(uint id, SString & rV) const;
	SJson * ToJsonObj() const;
	int FromJsonObj(const SJson * pJsObj);
private:
	int    Implement_Put(const Entry & rEntry);
	bool   Implement_Get(uint id, Entry & rEntry) const;

	TSVector <Entry> L;
};

class UiDescription {
public:
	static SColor GetColorR(const UiDescription * pUid, const SColorSet * pColorSet, const char * pColorSymb, const SColor defaultC);
	UiDescription();
	~UiDescription();
	UiDescription & Z();
	UiDescription & Copy(const UiDescription & rS);
	bool   FASTCALL IsEq(const UiDescription & rS) const;
	bool   FASTCALL operator == (const UiDescription & rS) const { return IsEq(rS); }
	bool   FASTCALL operator != (const UiDescription & rS) const { return !IsEq(rS); }
	SJson * ToJsonObj() const;
	int FromJsonObj(const SJson * pJsObj);
	const SColorSet * GetColorSetC(const char * pCsSymb) const;
	SColorSet * GetColorSet(const char * pCsSymb);
	int    GetColor(const SColorSet * pColorSet, const char * pColorSymb, SColor & rC) const;
	SColor GetColorR(const SColorSet * pColorSet, const char * pColorSymb, const SColor defaultC) const;
	int    GetColor(const char * pColorSetSymb, const char * pColorSymb, SColor & rC) const;
	const  SFontSource * GetFontSourceC(const char * pSymb) const;
	const  SFontDescr * GetFontDescrC(const char * pSymb) const;
	const  SUiLayout * GetLayoutBySymbC(const char * pSymb) const;
	SUiLayout * GetLayoutBySymb(const char * pSymb);
	const  SUiLayout * GetLayoutByIdC(int id) const;
	SUiLayout * GetLayoutById(int id);
	void   SetSourceFileName(const char * pFileName);
	const  char * GetSourceFileName() const;
	//
	// Descr: Проверяет корректность ссылок в наборах цветов ClrList
	//
	bool   ValidateColorSetList();

	TSCollection <SFontSource> FontList;
	TSCollection <SFontDescr> FontDescrList; // @v11.9.10
	TSCollection <SColorSet> ClrList;
	TSCollection <SUiLayout> LoList;
	UiValueList VList; // @v11.9.4
private:
	SString SourceFileName; // @v11.9.7 Исходный файл, из которого был загружен экземпляр. Устанавливается из-вне методом SetSourceFileName(const char *)
};
//
// Descr: Определитель текстового параграфа
//
struct SParaDescr { // @persistent
	SParaDescr();
	bool   FASTCALL IsEq(const SParaDescr &) const;
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    GetJustif() const;

	enum {
		fJustRight   = 0x0001, // Выравнивать строки по правому краю
		fJustCenter  = 0x0002  // Выравнивать строки по центру
	};
	SPoint2S LuIndent;    // Левый и верхний отступы
	SPoint2S RlIndent;    // Правый и нижний отступы
	int16  StartIndent; // Отступ первой строки
	int16  Spacing;     // Интерлиньяж (расстояние между строками)
	int32  Flags;
	uint8  Reserve[16];
};
//
// Descr: Определитель символа в шрифте.
//
struct SGlyph {
	uint16 Chr;     // Unicode-символ
	int16  Idx;     // Индекс начертания символа в шрифте
	SPoint2F Sz;      // Specifies the size of the smallest rectangle that completely encloses the glyph (its black box).
	SPoint2F Org;     // Specifies coordinates of the upper left corner of the smallest rectangle that completely encloses the glyph.
	SPoint2F Advance; // Specifies the distance from the origin of the current character cell to the origin of the next character cell.
	float  LineAdv; // Смещение, на которое необходимо переместиться при переходе на новую строку.
		// Значение извлекается из метрики шрифта.
};
//
// Descr: Представление инструмента рисования. Обеспечивает унифицированный механизм
//   хранения и сериализации разнородных инструментов, виды которых определяются полем
//   SPaintObj::T (enum SPaintObj::tXXX).
//
class SPaintObj { // @persistent @store(SSerializeContext)
public:
	static const float DefaultMiterLimit; // 4.0f
	//
	// Descr: Типы объекта
	//
	enum {
		tUndef = 0, // Не определен
		tPen,       // Перо
		tBrush,     // Кисть
		tColor,     // Цвет
		tCursor,    // Курсор
		tBitmap,    // Битовая карта
		tFont,      // Шрифт
		tGradient,  // Градиент
		tParagraph, // Текстовый параграф
		tCStyle     // Стиль текстовых символов
	};
	//
	// Descr: Стили перьев (PEN)
	//
	enum {
		psSolid       = PS_SOLID,      // The pen is solid. (==0)
		psDash        = PS_DASH,       // The pen is dashed. This style is valid only when the pen width is one or less in device units.
		psDot = PS_DOT,        // The pen is dotted. This style is valid only when the pen width is one or less in device units.
		psDashDot     = PS_DASHDOT,    // The pen has alternating dashes and dots. This style is valid only when the pen width is one or less in device units.
		psDashDotDot  = PS_DASHDOTDOT, // The pen has alternating dashes and double dots. This style is valid only when the pen width is one or less in device units.
		psNull        = PS_NULL,       // The pen is invisible.
		psInsideFrame = PS_INSIDEFRAME // The pen is solid. When this pen is used in any GDI drawing
			// function that takes a bounding rectangle, the dimensions of the figure are shrunk so that
			// it fits entirely in the bounding rectangle, taking into account the width of the pen.
			// This applies only to geometric pens.
	};
	//
	// Descr: Стили кистей (BRUSH)
	//
	enum {
		bsSolid        = BS_SOLID,       // Solid brush
		bsNull = BS_NULL,        // Hollow brush
		bsHatched      = BS_HATCHED,     // Hatched brush
		bsPattern      = BS_PATTERN,     // Pattern brush defined by a memory bitmap
		bsDibPattern   = BS_DIBPATTERN,  // A pattern brush defined by a device-independent bitmap (DIB) specification.
			// If lbStyle is BS_DIBPATTERN, the lbHatch member contains a handle to a packed DIB.
		bsDibPatternPt = BS_DIBPATTERNPT // A pattern brush defined by a device-independent bitmap (DIB) specification.
			// If lbStyle is BS_DIBPATTERNPT, the lbHatch member contains a pointer to a packed DIB.
	};
	//
	// Descr: Форма окончания линий (line cap)
	//
	enum {
		lcButt = 1,
		lcRound,
		lcSquare
	};
	//
	// Descr: Вид соединения линий (line join)
	//
	enum {
		ljMiter = 1,
		ljRound,
		ljBevel
	};
	//
	// Descr: Стили штриховки кистей (BRUSH)
	//
	enum {
		bhsHorz      = HS_HORIZONTAL, // Horizontal hatch                       ----
		bhsVert      = HS_VERTICAL,   // Vertical hatch                         ||||
		bhsFDiagonal = HS_FDIAGONAL,  // 45-degree downward left-to-right hatch (\\\\)
		bhsBDiagonal = HS_BDIAGONAL,  // 45-degree upward left-to-right hatch   ////
		bhsCross     = HS_CROSS,      // Horizontal and vertical crosshatch     ++++
		bhsDiagCross = HS_DIAGCROSS,  // 45-degree crosshatch                   xxxx
	};
	//
	// Descr: Правило заливки
	//
	enum {
		frNonZero = 1,
		frEvenOdd
	};

	class Base { // @persistent @store(SSerializeContext)
	public:
		friend int FASTCALL _SetPaintObjInnerHandle(SPaintObj::Base * pBase, SDrawSystem sys, void * h);

		Base();
		Base(const Base & rS);
		void * GetHandle() const { return Handle; }
		SDrawSystem GetSys() const { return Sys; }
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	private:
		void * Handle;
		SDrawSystem Sys;
	};
	//
	// Descr: Собственное представление пера
	//
	class Pen : public Base { // @persistent @store(SSerializeContext)
	public:
		Pen();
		Pen(const Pen & rS);
		~Pen();
		Pen  & FASTCALL operator = (const Pen & rS);
		int    FASTCALL Copy(const Pen & rS);
		bool   FASTCALL IsEq(const Pen & rS) const;
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
		bool   IsDashed() const;
		//
		// Descr: Возвращает !0, если перо является предельно простым,
		//   то есть может быть определено только цветом.
		//   Остальные параметры пера должны быть следующими:
		//   W = 1.0
		//   S = SPaintObj::psSolid
		//   LineCap = SPaintObj::lcButt || 0
		//   Join = SPaintObj::ljMiter || 0
		//   MiterLimit = 4.0 || 0
		//   P_DashRule = 0
		//
		bool   IsSimple() const;
		int    FASTCALL SetSimple(SColor);
		int    AddDashItem(float f);

		SColor C;          // Цвет
		float  W__;        // Ширина
		int32  S;          // Стиль (SPaintObj::psXXX)
		int8   LineCap;    // SPaintObj::lcXXX
		int8   Join;       // SPaintObj::ljXXX
		uint16 Reserve;    // @alignment
		float  MiterLimit; //
		float  DashOffs;   //
		FloatArray * P_DashRule;
	};
	//
	// Descr: Собственное представление кисти
	//
	class Brush : public Base { // @persistent @store(SSerializeContext)
	public:
		Brush();
		Brush(const Brush & rS);
		Brush & FASTCALL operator = (const Brush &);
		bool   FASTCALL operator == (const Brush &) const;
		void   FASTCALL Copy(const Brush & rS);
		bool   FASTCALL IsEq(const Brush & rS) const;
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
		int    IsSimple() const;
		void   FASTCALL SetSimple(SColor);

		SColor C;          // Цвет
		int32  S;          // Стиль (SPaintObj::bsXXX)
		int8   Hatch;      // Штрихровка (SPaintObj::bhsXXX)
		int8   Rule;       // SPaintObj::frXXX Правило заливки //
		uint16 Reserve;    // @alignment
		int32  IdPattern;  // Идентификатор градиента или образца заполнения.
	};
	//
	// Descr: Собственное представление градиента
	//
	class Gradient : public Base { // @persistent @store(SSerializeContext)
	public:
		//
		// Descr: Виды градиента
		//
		enum {
			kLinear = 0, // Линейный градиент
			kRadial,     // Радиальный градиент
			kConical     // Конический градиент
		};
		//
		// Descr: Способ заполнения области
		//
		enum {
			sPad,        // Область дозаполняется краевыми цветами градиента
			sRepeat,     //
			sReflect
		};
		enum {
			uUserSpace,  // Координаты градиента указаны в пользовательских единицах
			uBB          // Координаты градиента указаны в терминах границ рисунка (Bounding Box)
		};
		struct Stop {
			float  Offs;
			SColor C;
		};
		//
		// Descr: Координаты линейного градиента
		//
		enum { lcX1 = 0, lcY1, lcX2, lcY2 };
		//
		// Descr: Координаты радиального градиента
		//
		enum { rcCX = 0, rcCY, rcR, rcFX, rcFY, rcFR };

		explicit Gradient(int kind = kLinear, int units = uUserSpace);
		Gradient & operator = (const Gradient & rS);
		bool   operator == (const Gradient &) const;
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
		int    SetPrototype(const Gradient & rS);
		int    SetLinearCoord(int coord, float val, int pct);
		int    SetRadialCoord(int coord, float val, int pct);
		float  GetLinearCoord(int coord, int * pPct) const;
		float  GetRadialCoord(int coord, int * pPct) const;
		int    GetUnits() const;
		int    AddStop(float offs, SColor c);
		uint   GetStopCount() const;
		const  Stop * GetStop(uint idx) const;

		int8   Kind;    // Gradient::kXXX
		int8   Spread;  // Gradient::sXXX
		LMatrix2D Tf;   // Матрица преобразования //
	private:
		int8   Unit;    // Gradient::uXXX
		uint8  PctUf;   // Флаги величин, заданных в процентах
			// Для линейного градиента:
			// 0x01 - x1, 0x02 - y1, 0x04 - x2, 0x08 - y2
			// Для радиального градиента:
			// 0x01 - cx, 0x02 - cy, 0x04 - r, 0x08 - fx, 0x10 - fy, 0x20 - fr (радиус фокуса)
		float  Coord[8];
		TSVector <Stop> StopList;
	};

	class Font : public Base, public SFontDescr { // @persistent @store(SSerializeContext)
	public:
		Font();
		~Font();
		operator HFONT () const;
		operator cairo_font_face_t * () const;
		operator cairo_scaled_font_t * () const;
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
		int    GetGlyph(SDrawContext & rCtx, uint16 chr, SGlyph * pGlyph);

		float  LineAdv; // Смещение, на которое необходимо переместиться при переходе на новую строку.
	};
	//
	// Descr: Представление стиля текстовых символов
	//
	class CStyle : public Base { // @persistent @store(SSerializeContext)
	public:
		CStyle();
		CStyle(int fontId, int penId, int brushId);
		bool   FASTCALL IsEq(const CStyle & rS) const;
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);

		int    FontId;
		int    PenId;
		int    BrushId;
		uint8  Reserve[16];
	};
	//
	// Descr: Представление формата текстового параграфа
	//
	class Para : public Base, public SParaDescr { // @persistent @store(SSerializeContext)
	public:
		Para();
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	};

	explicit SPaintObj(int ident = 0);
	~SPaintObj();
	void   Destroy();
	int    GetId() const { return Id; }
	int    GetType() const { return T; }

	Pen *  GetPen() const;
	Brush * GetBrush() const;
	Font * GetFont() const;
	Gradient * GetGradient() const;
	Para * GetParagraph() const;
	CStyle * GetCStyle() const;

	operator COLORREF () const;
	operator SColor () const;
	operator HGDIOBJ () const;
	operator HCURSOR () const;
	operator HBITMAP () const;
	operator cairo_pattern_t * () const;
	//
	// Descr: Опции функции SPaintObj::Copy
	//
	enum {
		cfLeaveId = 0x0001 // Не менять поле Id в объекте this
	};
	//
	// Descr: Копирует содержимое объекта rS в объект this.
	//   Копирование полное.
	//
	int    Copy(const SPaintObj & rS, long flags = 0);
	//
	// Descr: Копирует содержимое экземпляра в объект pDest.
	//   В результате вызова функции экземпляр this теряет
	//   теряет право владения внутренними объектами (см. SPaintObj::ResetOwnership()).
	//   Владение этими объектами переходит к pDest.
	//   По этой причине данный метод не имеет модификатора const.
	//
	int    CopyTo(SPaintObj * pDest);
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);

	int    CreateHPen(int style, float width, SColor c);
	int    CreateHBrush(int style, SColor c, int32 hatch);
	int    CreatePen(int style, float width, SColor c);
	//
	// Descr: Формирует объект типа tPen.
	//   Предыдущее содержимое объекта разрушается.
	//   Если pPen == 0, то просто разрушает объект.
	//
	int    CreatePen(const Pen * pPen);
	int    CreateBrush(int style, SColor c, int32 hatch);
	//
	// Descr: Формирует объект типа tBrush.
	//   Предыдущее содержимое объекта разрушается.
	//   Если pBrush == 0, то просто разрушает объект.
	//
	int    CreateBrush(const Brush * pBrush);
	int    CreateFont_(const char * pFace, int height, int flags);
	int    CreateGradient(const Gradient * pGradient);
	int    CreateGradientLinear(const FRect & rBound);
	int    CreateGradientRadial(const FShape::Circle & rBound);
	int    AddGradientStop(float offs, SColor c);
	int    CreateParagraph();
	int    CreateCStyle(int fontId, int penId, int brushId);
	int    CreateColor(SColor c);
	int    SetBitmap(uint bmpId);
	int    SetFont(HFONT);
	int    CreateCursor(uint cursorId);
	int    FASTCALL CreateInnerHandle(const SDrawContext & rCtx);
	int    FASTCALL DestroyInnerHandle();
private:
	//
	// Descr: После вызова этой функции экземпляр теряет право владения сложными
	//   объектами, находящимися в его составе. То есть, метод Destroy не
	//   будет разрушать объекты, на которые ссылаются идентификаторы или
	//   освобождать область памяти, на которую ссылается SPaintObj::H
	//
	void   ResetOwnership();
	int    FASTCALL ProcessInnerHandle(const SDrawContext * pCtx, int create);

	enum {
		fCairoPattern = 0x0001,
		fInner        = 0x0002, // Внутреннее представление объекта
			// (tPen : SPaintObj::Pen; tBrush : SPaintObj::Brush; tFont : SPaintObj::Font)
		fNotOwner     = 0x0004  // Экземпляр не владеет сложными объектами, на которые ссылается SPaintObj::H
	};

	int32  Id;
	int16  T;   // Тип объекта (SPaintObj::tXXX)
	uint16 F;   //
	void * H;   //
};
//
//   Идентификаторы инструментов делятся на три категории:
//   1. Жесткие (1..10000) Владелец SPaintToolBox определяет enum в котором перечислены эта идентификаторы
//   2. Жесткие зарезервированные (80001..90000) Сам SPaintToolBox определяет набор идентификаторов.
//   3. Динамические (100001..) Идентификатор создается функцией SPaintToolBox::CreateDynIdent
//
class SPaintToolBox : private TSArray <SPaintObj> { // @persistent @store(SSerializeContext)
public:
	//
	// Зарезервированные идентификаторы инструментов
	//
	enum {
		robjFirst = 80001,
		rbrWindow,
		rbrWindowFrame,
		rbr3DDkShadow,
		rbr3DLight,
		rbr3DFace,
		rbr3DShadow,
		rbr3DHilight
	};

	SPaintToolBox();
	~SPaintToolBox();
	SPaintToolBox & Z();
	int    FASTCALL Copy(const SPaintToolBox & rS);
	//
	// Descr: Копирует объект с идентификатором toolIdent из контейнера rS в this.
	//   Для создаваемого объекта в контейнере this формируется новый идентификатор.
	// Returns:
	//   Идентификатор копии объекта в контейнере this.
	//
	int    CopyToolFrom(const SPaintToolBox & rS, int toolIdent);
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    CreateReservedObjects();
	//
	// Descr: Создает пустой объект с идентификатором ident.
	//   Если функция отработала успешно, то возвращает указатель на
	//   созданный объект, который может быть инициализирован (например
	//   функцией SPaintObj::CreatePen(SPaintObj::Pen *) или аналогичной).
	//
	//   Если по идентификатору ident уже существует объект, то функция возвращает 0.
	// Note: Объект по возвращаемому указателю является собственностью контейнера
	//   SPaintToolBox.
	//
	SPaintObj * FASTCALL CreateObj(int ident);
	int     FASTCALL DeleteObj(int ident);
	SPaintObj * FASTCALL GetObj(int ident) const;
	SPaintObj * FASTCALL GetObjBySymb(const char * pSymb, int type) const;
	//
	// Descr: Ищет хэш-символ идентификатора ident. Если такой символ найден,
	//   то возвращает !0 и по ссылке rSymb присваивает строку символа.
	//   В противном случае возвращает 0 и по ссылке rSymb присваивает 0 (пустую строку).
	//
	int    GetSymb(int ident, SString & rSymb) const;
	//
	// Descr: Создает динамический идентификатор объекта.
	//   Если для параметра pSymb выполняется условие !isempty(pSymb),
	//   то создаваемый идентификатор ассоциируется с символом pSymb.
	//   Если символ pSymb уже существует в контейнере, то
	//   возвращается идентификатор, ассоциированный с ним.
	// Returns:
	//   >0 - идентификатор, который может быть использован для создания нового объекта.
	//   0  - ошибка.
	//
	int    FASTCALL CreateDynIdent(const char * pSymb = 0);
	//
	// Descr: Ищет идентификатор объекта, ассоциированный с символом pSymb.
	//   Функция ищет только идентификатор, ассоциированный с символом, но
	//   не пытается выяснить существует ли в контейнере объект, соответствующий
	//   этому идентификатору.
	// Returns:
	//   >0 - найденный идентификатор объекта
	//   0  - символ не найден.
	//
	int    FASTCALL SearchSymb(const char * pSymb) const;
	int    FASTCALL SearchColor(SColor c) const;
	//
	// Descr: Создает WinGdi-перо (HPEN) и сохраняет его с идентификатором ident.
	//
	int    SetPen(int ident, int style, int width, COLORREF c);
	int    SetDefaultPen(int style, int width, SColor c);
	int32  GetDefaultPen();
	//
	// Descr: Создает WinGdi-кисть (HBRUSH) и сохраняет его с идентификатором ident.
	//
	int    SetBrush(int ident, int style, COLORREF c, int32 hatch);
	int    SetColor(int ident, COLORREF);
	int    SetBitmap(int ident, uint bmpId);
	int    SetFont(int ident, HFONT);
	int    CreateCursor(int ident, uint cursorId);
	int    CreateColor(int ident, SColor c);
	int    CreatePen(int ident, int style, float width, SColor c);
	int    CreateBrush(int ident, int style, SColor c, int32 hatch, int patternId = 0);
	int    CreateFont_(int ident, const char * pFace, int height, int flags);
	int    CreateFont_(int ident, const SFontDescr & rFd); // @v12.2.6
	//
	// Descr: Создает экземпляр шрифта по образцу системного хандлера hFont.
	//   Если overrideHeight > 0, то переопределяет логический размер шрифта.
	//
	int    CreateFont_(int ident, HFONT hFont, int overrideHeight); // @v12.2.6
	int    CreateGradientLinear(int ident, const FRect &);
	int    CreateGradientRadial(int ident, const FShape::Circle &);
	int    AddGradientStop(int ident, float off, SColor c);
	int    CreateParagraph(int ident, const SParaDescr * pDescr);
	int    CreateCStyle(int ident, int fontId, int penId, int brushId);
	COLORREF FASTCALL GetColor(int ident) const;
	//
	// Descr: Вариант функции получения цвета, возвращающий 0, если
	//   объект по заданному дескриптору не идентифицирован.
	//
	int    GetColor(int ident, COLORREF * pC) const;
	HGDIOBJ FASTCALL Get(int ident) const;
	HCURSOR FASTCALL GetCursor(int ident) const;
	HBITMAP FASTCALL GetBitmap(int ident) const;
	SPaintObj::Font * GetFont(const SDrawContext & rCtx, int fontIdent);
	SPaintObj::Para * GetParagraph(int ident);
	int    GetGlyphId(SDrawContext & rCtx, int fontIdent, wchar_t chr);
	const  SGlyph * FASTCALL GetGlyph(int glyphId) const;
private:
	enum {
		stReservedObjects = 0x0001 // Созданы зарезервированные объекты
	};
	virtual void FASTCALL freeItem(void *);
	int    FASTCALL GetType(int ident) const;
	int32  DynIdentCount;
	int32  State;
	int32  DefaultPenId; //
	SymbHashTable Hash;  // Используется для хранения символьных идентификаторов, ассоциированных с целочисленными.
	struct GlyphEntry {
		int32  I;
		SGlyph G;
	};
	SVector GlyphList;
};

class STextLayout {
public:
	//
	// Descr: Флаги опций арранжировки текста и вывода
	//
	enum {
		fUnlimX        = 0x0001,
		fUnlimY        = 0x0002,
		fWrap  = 0x0004, //
		fNoClip        = 0x0008, // Не ограничивать отрисовку границами
		fOneLine       = 0x0010, // Выводить текст в одну строку (не переносить и игнорировать переводы каретки)
		fPrecBkg       = 0x0020, // Закрашивать фон строго по тексту (в противном случае закрашивается вся область,
			// отведенная под вывод текста).
		fVCenter       = 0x0040, // Вертикальное центрирование
		fVBottom       = 0x0080  // Вертикальное выравнивание по нижней границе
			// @#(fVCenter^fVBottom)
	};
	struct Item {
		Item(SPoint2F p, const SGlyph * pGlyph, uint16 flags);
		enum {
			fUnderscore = 0x0001
		};
		int16  GlyphIdx; // -1 - не выводить символ
		uint16 Flags;    // Специальные опции отображения символа
		SPoint2F P;
	};
	struct RenderGroup {
		RenderGroup();

		SPaintObj::Font * P_Font;
		int    PenId;
		int    BrushId;
		TSVector <STextLayout::Item> Items;
	};

	STextLayout();
	void   Reset();
	int    FASTCALL Copy(const STextLayout & rS);
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	const  SStringU & GetText() const;
	int    SetText(const char * pText);
	void   SetOptions(long flags, int defParaStyle = -1, int defCStyle = -1);
	int    FASTCALL HasOption(long f) const;
	int    AddParagraph(const char * pText, int paraStyleId);
	int    AddText(const char * pText, int cstyleId);
	int    SetTextStyle(uint startPos, uint len, int cstyleId);
	const FRect & GetBounds() const;
	//
	// Descr: Возврашает границы заливки перед выводом текста.
	//   При использовании опции fPrecBkg эти границы отличаются от GetBounds().
	//
	FRect  GetBkgBounds() const;
	void   FASTCALL SetBounds(const FRect & rBounds);
	int    EnumGroups(uint * pI, RenderGroup * pGroup);
	int    Arrange(SDrawContext & rCtx, SPaintToolBox & rTb);
	const  LongArray & GetGlyphIdList() const { return GlyphIdList; }
private:
	int    Preprocess(SDrawContext & rCtx, SPaintToolBox & rTb);
	int    FASTCALL CanWrap(uint pos);

	struct CStyle {
		uint32 Start;
		uint32 Len;
		int32  StyleIdent;
	};
	struct StyleGroup {
		uint   Pos;
		SPaintObj::Font * P_Font;
		int    PenId;
		int    BrushId;
	};
	enum {
		stPreprocessed = 0x0001,
		stArranged     = 0x0002
	};
	int32  DefParaStyleIdent; // Идентификатор стиля параграфа по умолчанию
	int32  DefCStyleIdent;    // Идентификатор стиля символов по умолчанию.
	long   Flags;
	long   State;
	FRect  Bounds;
	SPoint2F EndPoint;          // @*STextLayout::Arrange Точка, до которой простирается собственно текст.
	SStringU Text;
	LongArray GlyphIdList;                     // @transient
	TSVector <CStyle> CStyleList;              //
	LAssocArray ParaList;                      // Список параграфов: ассоцииации {позиция первого символа; стиль параграфа}
	TSVector <STextLayout::Item> List;         // @transient
	TSVector <STextLayout::StyleGroup> SgList; // @transient

	friend class TloRowState;
};

class TCanvas2 {
public:
	//
	// Descr: Высокоуровневая функция, трансформирующая фигуру pFig в иконку.
	// Note: Основным мотиватором для ввода функции являлась необходимость унифицировать механизм, используемый в нескольких
	//   модулях системы. Последщующий шаг - расширение механизма до транформации фигуры в иные форматы.
	// ARG(pTb IN): @#{vptr} Набор инструментов для рисования. Должен быть получен от экземпляра TProgram вызовом GetUiToolBox().
	//   Если аргумент нулевой, то функция вернет нулевой результат.
	// ARG(pFig IN): Фигура, которая должна быть преобразована в иконку. Если аргумент нулевой, то функция вернет нулевой результат.
	// ARG(size IN): Размеры результирующей иконки. Если хотя бы один из компонентов аргумента меньше или равен нулю, то функция вернет нулевой результат.
	// ARG(colorReplaced IN): Цвет, который должен быть замещен в фигуре pFig. Если colorReplaced.IsEmpty(), то замещаться ничего не будет,
	//   в противном случае цвет для замещения извлекается из набора инструментов pTb по идентификатору TProgram::tbiIconRegColor.
	// ARG(colorBackground IN): Цвет фона иконки.
	// Returns:
	//   Манипулятор иконки (HICON). Если в ходе выполнения функции произошла ошибка, то результат будет нулевым (!ret == true).
	//   После использования объект, на который ссылается манипулятор, должен быть разрушен вызовом ::DestroyIcon(ret)
	//
	static SPtrHandle TransformDrawFigureToIcon(SPaintToolBox * pTb, const SDrawFigure * pFig, SPoint2S size, SColor colorReplaced, SColor colorBackground);
	static bool TransformDrawFigureToBitmap(SPaintToolBox * pTb, const SDrawFigure * pFig, SPoint2S size, SColor colorReplaced, SColor colorBackground, SBuffer & rBuf);

	struct Surface {
		Surface();
		void * HCtx;  // Контекст устройства
		SImageBuffer * P_Img;
	};
	struct Capability {
		Capability();
		SPoint2S SizePt; // Размер устройства в точках
		SPoint2F SizeMm; // Размер устройства в миллиметрах
		SPoint2F PtPerInch; // Количество точек на дюйм
	};

	struct FontExt {
		float Ascent;      // The distance that the font extends above the baseline.
			// Note that this is not always exactly equal to the maximum of the extents
			// of all the glyphs in the font, but rather is picked to express the font designer's
			// intent as to how the font should align with elements above it.
		float Descent;     // The distance that the font extends below the baseline. This value is
			// positive for typical fonts that include portions below the baseline. Note that this
			// is not always exactly equal to the maximum of the extents of all the glyphs in the font,
			// but rather is picked to express the font designer's intent as to how the the font should
			// align with elements below it.
		float Height;      // The recommended vertical distance between baselines when setting consecutive
			// lines of text with the font. This is greater than ascent+descent by a quantity known as the
			// line spacing  or external leading. When space is at a premium, most fonts can be set with only
			// a distance of ascent+descent between lines.
		SPoint2F MaxAdvance; // X - the maximum distance in the X direction that the the origin is advanced
			// for any glyph in the font.
			// Y - the maximum distance in the Y direction that the the origin is advanced for any glyph
			// in the font. this will be zero for normal fonts used for horizontal writing.
			// (The scripts of East Asia are sometimes written vertically.)
	};
	struct TextExt {
		SPoint2R Bearing; // X - the horizontal distance from the origin to the leftmost part of the glyphs as drawn.
			// Positive if the glyphs lie entirely to the right of the origin.
			// Y - the vertical distance from the origin to the topmost part of the glyphs as drawn.
			// Positive only if the glyphs lie completely below the origin; will usually be negative.
		SPoint2R Size;    // Size of the glyphs as drawn
		SPoint2R Advance; // X - distance to advance in the X direction after drawing these glyphs
			// Y - distance to advance in the Y direction after drawing these glyphs.
			// Will typically be zero except for vertical text layout as found in East-Asian languages.
	};
	//
	// Descr: Варианты операторов, используемые для отрисовки
	//
	enum {
		oprCLEAR,
    	oprSOURCE,
    	oprOVER,
    	oprIN,
    	oprOUT,
    	oprATOP,
    	oprDEST,
    	oprDEST_OVER,
    	oprDEST_IN,
    	oprDEST_OUT,
    	oprDEST_ATOP,
    	oprXOR,
    	oprADD,
    	oprSATURATE,
    	oprMULTIPLY,
    	oprSCREEN,
    	oprOVERLAY,
    	oprDARKEN,
    	oprLIGHTEN,
    	oprCOLOR_DODGE,
    	oprCOLOR_BURN,
    	oprHARD_LIGHT,
    	oprSOFT_LIGHT,
    	oprDIFFERENCE,
    	oprEXCLUSION,
    	oprHSL_HUE,
    	oprHSL_SATURATION,
    	oprHSL_COLOR,
    	oprHSL_LUMINOSITY
	};
	TCanvas2(SPaintToolBox & rTb, HDC);
	TCanvas2(SPaintToolBox & rTb, SImageBuffer & rBuf);
	~TCanvas2();
	//
	// Descr: Возвращает манипулятор контекста рисования, ассоциированный с данным
	//   экземпляром.
	//   Если экземпляр не ассоциирован с контекстом рисования, то возвращает 0.
	//
	operator HDC() const;
	operator SDrawContext () const;
	//
	// Descr: Если экземпляр создан на базе буфера изображения SImageBuffer,
	//   то фозвращает указатель на этот буфер. В противном случае возвращает 0.
	//
	operator const SImageBuffer * () const;
	int    GetCapability(Capability * pCaps) const;
	SPaintToolBox & GetToolBox() { return R_Tb; }
	int    FASTCALL SelectObjectAndPush(HGDIOBJ);
	int    PopObject();
	int    FASTCALL PopObjectN(uint c);
	//
	// Primitives
	//
	SPoint2F GetCurPoint();
	void   FASTCALL MoveTo(SPoint2F to);
	void   FASTCALL Line(SPoint2F to);
	void   FASTCALL LineV(float yTo);
	void   FASTCALL LineH(float xTo);
	void   FASTCALL Rect(const FRect & rRect);
	void   FASTCALL Rect(const TRect & rRect);
	void   RoundRect(const FRect &, float radius);
	int    FASTCALL Ellipse(const FRect & rRect);
	void   Arc(SPoint2F center, float radius, float startAngleRad, float endAngleRad);
	void   Bezier(SPoint2F middle1, SPoint2F middle2, SPoint2F end);
	int    Text(const char * pText, int identFont);
	void   ClosePath();
	void   SubPath();
	void   FASTCALL GetClipExtents(FRect & rR);
	void   SetColorReplacement(SColor original, SColor replacement);
	void   ResetColorReplacement();
	int    FASTCALL SetOperator(int opr);
	int    GetOperator() const;
	//
	// Descr: Устанавливает матрицу преобразования равной rMtx.
	//
	void   FASTCALL SetTransform(const LMatrix2D & rMtx);
	//
	// Descr: Добавляет к существующей матрице преобразования матрицу rMtx.
	//
	void   FASTCALL AddTransform(const LMatrix2D & rMtx);
	void   FASTCALL GetTransform(LMatrix2D & rMtx) const;
	//
	// Descr: Подготавливает запись области отрисовки.
	//
	int    BeginScope();
	//
	// Descr: Завершает запись границ области отрисовки и возвращает
	//   регион, содержащий границы отрисовки, совершенной с момента
	//   предыдущего вызова BeginScope().
	//
	int    FASTCALL EndScope(SRegion & rR);
	int    Stroke(int paintObjIdent, int preserve);
	int    Fill(int paintObjIdent, int preserve);
	int    Fill(SColor c, int preserve);
	//
	// Descr: Сохраняет во внутреннем стеке текущую матрицу преобразования.
	//
	void   PushTransform();
	//
	// Descr: Восстанавливает из внутреннего стека матрицу преобразования,
	//   которая до этого была там сохранена вызовом PushTransform().
	//
	int    PopTransform();
	void   LineVert(int x, int yFrom, int yTo);
	void   LineHorz(int xFrom, int xTo, int y);
	void   Rect(const TRect & rRect, int penIdent, int brushIdent);
	void   Rect(const FRect & rRect, int penIdent, int brushIdent);
	void   RoundRect(const FRect &, float radius, int penIdent, int brushIdent);
	// @construction int    RoundRect(const TRect & rRect, int penIdent, int brushIdent);
	int    PatBlt(const TRect & rR, int brushId, int opr);
	int    FASTCALL SetBkColor(COLORREF);
	int    FASTCALL SetTextColor(COLORREF);
	void   SetBkTranparent();
	SPoint2S GetTextSize(const char * pStr);
		// @>>BOOL GetTextExtentPoint32(HDC hdc, LPCTSTR lpString, int cbString, LPSIZE lpSize);
	int    TextOut(SPoint2S p, const char * pText);
	int    _DrawText(const TRect & rRect, const char * pText, uint options);
	int    DrawTextLayout(STextLayout * pTlo);
	int    FASTCALL Draw(const SImageBuffer * pImg);
	int    FASTCALL Draw(const SDrawFigure * pDraw);
	int    FASTCALL Draw(const SDrawPath * pPath);
	int    FASTCALL Draw(const SDrawShape * pShape);
	int    FASTCALL Draw(const SDrawText * pText);
	int    FASTCALL Draw(const SDrawImage * pImg);
	int    FASTCALL Draw(const SDrawGroup * pDraw);
	int    FASTCALL Draw(const SDrawRef * pRef);

	enum {
		edgeRaisedInner = 0x0001,
		edgeSunkenInner = 0x0002,
		edgeRaisedOuter = 0x0004,
		edgeSunkenOuter = 0x0008,
		edgeBump        = edgeRaisedOuter|edgeSunkenInner,
		edgeEtched      = edgeSunkenOuter|edgeRaisedInner,
		edgeRaised      = edgeRaisedOuter|edgeRaisedInner,
		edgeSunken      = edgeSunkenOuter|edgeSunkenInner,
		edgeOuter       = edgeRaisedOuter|edgeSunkenOuter,
		edgeInner       = edgeRaisedInner|edgeSunkenInner
	};
	enum {
		borderLeft     = 0x0001,
		borderTop      = 0x0002,
		borderRight    = 0x0004,
		borderBottom   = 0x0008,
		borderDiagonal = 0x0010,

		borderTopLeft  = borderTop | borderLeft,
		borderTopRight = borderTop | borderRight,
		borderBottomLeft  = borderBottom | borderLeft,
		borderBottomRight = borderBottom | borderRight,
		borderRect        = borderLeft | borderTop | borderRight | borderBottom,

		borderMiddle   = 0x0800, // Fill in the middle
		borderSoft     = 0x1000, // For softer buttons
		borderAdjust   = 0x2000, // Calculate the space left over
		borderFlat     = 0x4000, // For flat rather than 3D borders
		borderMono     = 0x8000, // For monochrome borders

		borderDiagEndTopRight    = borderDiagonal|borderTop|borderRight,
		borderDiagEndTopLeft     = borderDiagonal|borderTop|borderLeft,
		borderDiagEndBottomLeft  = borderDiagonal|borderBottom|borderLeft,
		borderDiagEndBottomRight = borderDiagonal|borderBottom|borderRight
	};
	int    DrawEdge(TRect & rR, long edge, long flags);
	int    DrawFrame(const TRect & rR, int clFrame, int brushId);
	int    FASTCALL SelectFont(SPaintObj::Font * pFont);
private:
	enum {
		fOuterSurface   = 0x0001, // Surface имеет ссылку на внешний объект. Т.е. разрушать его не следует.
		fScopeRecording = 0x0002  // Объект в состоянии записи границ отрисовки в регион this->Scope.
	};
	class DrawingProcFrame {
	public:
		DrawingProcFrame(TCanvas2 * pCanv, const SDrawFigure * pFig);
		~DrawingProcFrame();
	private:
		TCanvas2 * P_Canv;
		const SDrawFigure * P_Fig;
		int    MtxAppl;
	};
	struct PatternWrapper {
		PatternWrapper();
		~PatternWrapper();
		void * P;
	};

	//void   Init(); // @<<TCanvas2::TCanvas2()
	int    FASTCALL SetCairoColor(SColor c);
	int    Helper_SelectPen(SPaintToolBox * pTb, int penId);
	int    Helper_SelectBrush(SPaintToolBox * pTb, int brushId, PatternWrapper & rPw);
	int    Implement_ArcSvg(SPoint2F radius, float xAxisRotation, int large_arc_flag, int sweep_flag, SPoint2F toPoint);
	int    Implement_Stroke(SPaintToolBox * pTb, int paintObjIdent, int preserve);
	//
	// Descr: Отрисовывает линии без выбора инструментов (предполагается, что они до этого уже были выбраны).
	//
	int    FASTCALL Implement_Stroke(int preserve);
	int    Implement_Fill(SPaintToolBox * pTb, int paintObjIdent, int preserve);
	int    Implement_StrokeAndFill(SPaintToolBox * pTb, int penIdent, int brushIdent);
	enum {
		dfoDrawSymbol = 0x0001 // Опция обеспечивающая отрисовку фигуры с признаком SDrawFigure::fSymbolGroup
	};
	int    FASTCALL Implement_DrawFigure(const SDrawFigure * pDraw, long options);

	Surface S;
	SRegion Scope;   // Границы области отрисовки между вызовами BeginScope() и EndScope()
	long   Flags;    // @flags
	SString TempBuf; // @allocreuse
	SPaintToolBox & R_Tb;
	TSStack <LMatrix2D> TmStk;
	SStack GdiObjStack; // Для совместимости с TCanvas на уровне функций

	struct ColorReplacement {
		ColorReplacement();
		void   Reset();
		void   Set(SColor org, SColor rpl);

		enum {
			fActive = 0x0001 // Замещение должно применяться
		};
		long   Flags;
		SColor  Original;
		SColor  Replacement;
	};
	ColorReplacement ClrRpl;
	//
	cairo_t * P_Cr;
	cairo_surface_t * P_CrS;
	SPaintObj::Font * P_SelectedFont;
};
//
//
//
class TCanvas {
public:
	TCanvas(HDC);
	~TCanvas();
	operator HDC() const;

	void   FASTCALL SetBounds(const TRect &);
	//const  TRect & GetBounds() const { return Bounds; }

	int    FASTCALL SelectObjectAndPush(HGDIOBJ);
	int    PopObject();
	int    FASTCALL PopObjectN(uint c);
	int    FASTCALL MoveTo(SPoint2S);
	int    FASTCALL Line(SPoint2S);
	void   LineVert(int x, int yFrom, int yTo);
	void   LineHorz(int xFrom, int xTo, int y);
	void   SetBkTranparent();
	int    FASTCALL SetBkColor(COLORREF);
	int    FASTCALL SetTextColor(COLORREF);
	int    FASTCALL Rectangle(const TRect &);
	int    RoundRect(const TRect &, const SPoint2S & rRoundPt);
	int    FillRect(const TRect & rRect, HBRUSH brush);
	SPoint2S FASTCALL GetTextSize(const char * pStr);
		// @>>BOOL GetTextExtentPoint32(HDC hdc, LPCTSTR lpString, int cbString, LPSIZE lpSize);
	int    TextOut_(SPoint2S p, const char * pText);
	int    DrawText_(const TRect & rRect, const char * pText, uint options);
private:
	enum {
		fOuterDC = 0x0001
	};
	HDC    H_Dc;
	long   Flags;
	TRect  Bounds;
	SStack ObjStack;
};
//
// Descr: Опознавательные идентификаторы некоторых порожденных от PPView классов
//   Этими значениями помечаются соответствующие классы в конструкторах
//   присвоением их переменной PPView::SubSign
//
#define TV_SUBSIGN_DIALOG    10  // TDialog
#define TV_SUBSIGN_BUTTON    11  // TButton
#define TV_SUBSIGN_INPUTLINE 12  // TInputLine
#define TV_SUBSIGN_CLUSTER   13  // Cluster of checkboxes or radiobuttons
#define TV_SUBSIGN_STATIC    14  // TStaticText
#define TV_SUBSIGN_LABEL     15  // TLabel
#define TV_SUBSIGN_LISTBOX   16  // SmartListBox
#define TV_SUBSIGN_COMBOBOX  17  // ComboBox
#define TV_SUBSIGN_IMAGEVIEW 18  // TImageView
#define TV_SUBSIGN_GROUPBOX  19  // @v12.2.3 TGroupBox

class TBitmapCache {
public:
	TBitmapCache();
	~TBitmapCache();
	HBITMAP    FASTCALL Get(uint bmpId);
	HBITMAP    FASTCALL GetSystem(uint bmpId);
private:
	struct Entry {
		long   ID;
		void * H;
	};
	TSVector <Entry> List;
};

class TView {
public:
	static void * FASTCALL messageCommand(TView * pReceiver, uint command);
	static void * STDCALL  messageCommand(TView * pReceiver, uint command, void * pInfoPtr);
	static void * FASTCALL messageBroadcast(TView * pReceiver, uint command);
	static void * STDCALL  messageBroadcast(TView * pReceiver, uint command, void * pInfoPtr);
	static void * FASTCALL messageKeyDown(TView * pReceiver, uint keyCode);
	static HFONT setFont(HWND hWnd, const char * pFontName, int height);
	//
	// Descr: Создает экземляр шрифта по описанию rFd.
	// Note: Эту функцию следует использовать везде вместо wingdi-функции CreateFontIndirect().
	//
	static void * CreateFont_(const SFontDescr & rFd);
	static void * SetWindowProp(HWND hWnd, int propIndex, void * ptr);
	static void * FASTCALL SetWindowUserData(HWND hWnd, void * ptr);
	static long SetWindowProp(HWND hWnd, int propIndex, long value);
	static void * FASTCALL GetWindowProp(HWND hWnd, int propIndex);
	static void * FASTCALL GetWindowUserData(HWND hWnd);
	static long FASTCALL SGetWindowStyle(HWND hWnd);
	static long FASTCALL SGetWindowExStyle(HWND hWnd);
	static int  FASTCALL SGetWindowClassName(HWND hWnd, SString & rBuf);
	static int  FASTCALL SGetWindowText(HWND hWnd, SString & rBuf);
	static int  FASTCALL SSetWindowText(HWND hWnd, const char * pText);
	//
	// Descr: Специализированный метод, вызывающий для всех элементов группы TViewGroup
	//   метод P_WordSelBlk->OnAcceptInput()
	//
	static void CallOnAcceptInputForWordSelExtraBlocks(TViewGroup * pG);
	//
	// Descr: перебирает дочерние окна родительского окна hWnd и
	//   выполняет подстановку шаблонизированных текстовых строк
	//   вида "@textident" или "@{textident}"
	//   Применяется для окон, созданных в обход штатной загрузки диалогов TDialog
	//
	static void FASTCALL PreprocessWindowCtrlText(HWND hWnd);
	//
	// Descr: Создает "родной" для операционной системы виджет, соответствующий
	//   элементу pV. Тип виджета определяется по сигнатуре pV.
	//   Если функция отработала успешно, то возвращает идентификатор "родного"
	//   элемента (например, HWND). Если нет, то 0.
	//
	static int64 CreateCorrespondingNativeItem(TView * pV);
	static bool  FASTCALL IsSubSign(const TView * pV, uint sign) { return (pV && pV->SubSign == sign); }

	enum phaseType {
		phFocused,
		phPreProcess,
		phPostProcess
	};
	enum selectMode {
		normalSelect,
		enterSelect,
		leaveSelect,
		forceSelect // same as normalSelect but don't check current selection
	};

	explicit TView(const TRect & rBounds);
	TView();
	virtual ~TView();
	// @v12.2.6 virtual int    FASTCALL valid(ushort command);
	//
	// Descr: Метод, используемый для передачи (извлечения) данных в (из)
	//   экземпляр объекта. Кроме того, метод реализует возрат размера данных объекта.
	//
	virtual int    TransmitData(int dir, void * pData);
	virtual void   setState(uint aState, bool enable);
	virtual int    handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	DECL_HANDLE_EVENT;
	int    commandEnabled(ushort command) const;
	void   STDCALL enableCommands(const TCommandSet & commands, bool toEnable);
	void   STDCALL enableCommand(ushort command, bool toEnable);
	void   getCommands(TCommandSet & commands) const;
	void   setCommands(const TCommandSet & commands);
	void   setBounds(const TRect & rBounds);
	void   setBounds(const FRect & rBounds); // @v12.3.2
	void   changeBounds(const TRect & rBounds);
	bool   IsCommandValid(ushort command); // @v12.2.6
	uint   getHelpCtx();
	TView & SetId(uint id);
	uint   GetId() const;
	bool   FASTCALL TestId(uint id) const;
	//
	// Descr: Возвращает !0 если поле состояние объекта содержит хотя бы один
	//   из флагов, установленных в параметре s.
	//
	bool   FASTCALL IsInState(uint s) const;
	void * FASTCALL MessageCommandToOwner(uint command);
	//
	// @v9.8.12 @construction
	// Descr: Функция вызывается для обработки Windows-сообщений, которые (по мнению централизованного
	//   обработчика сообщений) могут или должны быть обработаны конкретным TView самостоятельно.
	// Функция реализована в рамках работы по элиминированию виртуального метода TView::handlwWindowsMessage()
	// с переносом его функциональности в метод handleEvent().
	//
	int    HandleWinCmd(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		TEvent event;
		handleEvent(event.setWinCmd(uMsg, wParam, lParam));
		return BIN(event.what == TEvent::evNothing);
	}
	void   Draw_();
	//
	// Descr: Показывает или скрывает окно в зависимости от параметра doShow.
	// Note: Вместо этой функции рекомендуется использовать setState(sfVisible, doShow)
	// ARG(doShow IN): !0 - показывает окно, 0 - скрывает окно
	//
	void   Show(bool doShow);
	void   FASTCALL clearEvent(TEvent & event);
	void   FASTCALL NegativeReplyOnValidateCommand(TEvent & event);
	HWND   getHandle() const;
	void   select();
	//
	// Descr: Если this является текущим членом owner'а, то
	//   сбрасывает значение owner->curren в 0.
	//
	void   ResetOwnerCurrent();
	int    FASTCALL EventBarrier(int rmv = 0);
	TView * nextView() const;
	TView * prevView() const;
	TView * prev() const;
	TView * TopView();
	bool   IsConsistent() const;
	bool   FASTCALL IsSubSign(uint) const;
	uint   GetSubSign() const { return SubSign; }
	int    GetEndModalCmd() const { return EndModalCmd; }
	//
	// Descr: Должна вызываться оконными (диалоговыми) функциями порожденных классов
	//   для восстановления оригинальной оконной функции
	//
	int    OnDestroy(HWND);
	void   SendToParent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void   SetWordSelBlock(WordSel_ExtraBlock *);
	bool   HasWordSelector() const { return LOGIC(P_WordSelBlk); }
	//
	// Descr: Только для применения в функции TWindow::getCtrlView
	//
	uint16 GetId_Unsafe() const { return Id; }
private:
	uint32 Sign;    // Подпись экземпляра класса. Используется для идентификации инвалидных экземпляров.
protected:
	friend class EvBarrier;

	class EvBarrier {
	public:
		explicit EvBarrier(TView * pV);
		~EvBarrier();
		bool   operator !() const;
	private:
		int    Busy;
		TView * P_V;
	};
	uint32 SubSign; // Подпись подтипа класса. Порожденные классы сами определяют значение этого поля для идентификации "свой-чужой"
	uint16 Id;
	uint16 Reserve;
	uint32 Sf;      // Поле флагов состояния объекта (sfXXX)
	WordSel_ExtraBlock * P_WordSelBlk; // owner
public:
	SPoint2S ViewSize;
	SPoint2S ViewOrigin;
	uint32 ViewOptions;
	TView  * P_Next;
	TViewGroup * P_Owner;
	HWND   Parent;
	WNDPROC PrevWindowProc;
protected:
	static int Helper_SendCmSizeAsReplyOnWmSize(TView * pV, WPARAM wParam, LPARAM lParam);
	int    HandleKeyboardEvent(WPARAM wParam, int isPpyCodeType = 0);
	//
	// Descr: Эта функция должна вызываться наследуемыми классами в деструкторе
	//   для восстановления предыдущей оконной процедуры.
	//
	//   Так как иногда экземпляр объекта разрушается до того, как будет
	//   разрушено собственно окно, необходимо возвращать назад подставную оконную процедуру
	//   в деструкторе.
	//
	//   К сожалению, те же действия дублируются одновременно в самой подставной
	//   процедуре (по сообщению WM_DESTROY) функцией OnDestroy().
	//   Это приводит к некоторой запутанности кода.
	//
	int    RestoreOnDestruction();
	//
	// Descr: Получает текст управляющего элемента (SendDlgItemMessage(Parent, Id, WM_GETTEXT,...)
	//   и, если он является шаблоном (@...), заменяет его на значение по шаблону.
	//
	int    SetupText(SString * pText);

	int    EndModalCmd;         // Команда, по которой окно было закрыто (в том числе, выведено из модального состояния)
	uint32 HelpCtx;
	TCommandSet * P_CmdSet;
};

class TViewGroup : public TView {
public:
	explicit TViewGroup(const TRect & bounds);
	~TViewGroup();
	DECL_HANDLE_EVENT;
	virtual void   Insert_(TView *p);
	virtual void   setState(uint aState, bool enable);
	virtual int    TransmitData(int dir, void * pData);
	// @v12.2.6 virtual int    FASTCALL valid(ushort command);
	ushort FASTCALL execView(TWindow * p);
	void   insertView(TView * p, TView * pTarget);
	void   FASTCALL remove(TView * p);
	void   removeView(TView * p);
	void   selectNext();
	void   FASTCALL selectCtrl(ushort ctlID); // @v12.3.7 (moved from TWindow)
	void   forEach(void (*func)(TView *, void *), void *args);
	void   insertBefore(TView *p, TView *Target);
	TView * GetFirstView() const;
	TView * GetCurrentView() const { return P_Current; }
	TView * GetLastView() const { return P_Last; }
	TView * FASTCALL getCtrlView(ushort ctl); // @v12.3.7 moved from TWindow
	const TView * FASTCALL getCtrlViewC(ushort ctl) const; // @v12.3.7 moved from TWindow
	void   redraw();
	uint   GetCurrId() const;
	bool   FASTCALL IsCurrentView(const TView * pV) const;
	bool   FASTCALL isCurrCtlID(uint ctlID) const;
	void   SetCurrentView(TView * p, selectMode mode);
protected:
	bool   ValidateCommand(TEvent & rEv); // @v12.2.6 @non-virtual
	TView * P_Current;
	TView * P_Last;
	enum {
		fLockMsgChangedFocus = 0x001
	};
	uint   MsgLockFlags; // fLockMsgXXX
};
//
//
//
class TToolTip {
public:
	struct ToolItem {
		ToolItem();

		uint   Id;
		HWND   H;     // Окно, в котором активируется подсказка
		long   Param; // Дополнительное значение, ассоциированное с подсказкой
		TRect  R;     // Прямоугольник, в котором активируется подсказка
		SString Text; // Текст подсказки
	};

	static LRESULT CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	TToolTip(HWND hParent, uint maxWidthPix);
	~TToolTip();
	int    AddTool(ToolItem & rItem);
	uint   GetToolsCount();
	int    GetTool(uint idx /*0..GetToolsCount()-1*/, ToolItem & rItem);
	int    RemoveTool(uint ident);
	int    RemoveAllTools();
private:
	int    Create(HWND hParent);
	HWND   H;
	//uint   Counter;    // Счетчик для формирования внутренних уникальных идентификаторов подсказок
	uint   MaxWidthPix; // Максимальная ширина окна подстказки
};
//
//
//
struct ToolbarItem {
	ToolbarItem();
	enum {
		fHidden = 0x0001
	};
	long   Cmd;
	uint16 KeyCode;
	uint16 Flags;
	uint   BitmapIndex;
	char   ToolTipText[128];
};

class ToolbarList : private SVector {
public:
	ToolbarList();
	ToolbarList & FASTCALL operator = (const ToolbarList &);
	void   setBitmap(uint b);
	uint   getBitmap() const;
	int    addItem(const ToolbarItem *);
	int    enumItems(uint *, ToolbarItem *);
	uint   getItemsCount() const;
	uint   getVisibleItemsCount() const;
	const  ToolbarItem & FASTCALL getItem(uint idx) const;
	int    searchKeyCode(ushort keyCode, uint * pIdx) const;
	int    moveItem(uint pos, int up);
	void   clearAll();
private:
	uint   Bitmap;
};
//
//
//
class TMenuPopup {
public:
	TMenuPopup();
	~TMenuPopup();
	//
	// Descr: Возвращает количество элементов меню, успешно добавленных функцией Add или AddSubst.
	//
	uint   GetCount() const;
	//
	// Descr: Добавляет команду cmd с текстом pText.
	//   Если текст начинается с символа '@' то пытается извлечь текст команды функцией
	//   SlSession::LoadString по сигнатуре pText+1. Если таким образом текст извлечь не
	//   удалось, то текстом комадны будет сама сигнатура.
	// Note: строка pText должна быть в ANSI-кодировке.
	//
	int    Add(const char * pText, int cmd);
	int    Add(const char * pText, int cmd, int keyCode); // @v10.8.11
	int    AddSubstr(const char * pText, int idx, int cmd);
	int    AddSeparator();
	//
	// Descr: Опции функции TMenuPopup::Execute
	//
	enum {
		efRet        = 0x0001, // Возвращать результат выбора. Если этот флаг установлен, то
			// функция вернет номер выбранного элемента.
			// При установке флага меню не отсылает извещение окну hWnd.
	};
	int    Execute(HWND hWnd, long flags, uint * pCmd, uint * pKeyCode);
private:
	void * H;
	long   State;
	//uint   Count; // Количество "живых" элементов (не сепараторов). Изменяется функциям Add и AddSubstr
	struct Item {
		uint   Cmd;
		uint   KeyCode;
	};
	TSVector <Item> L;
};

class TWindow : public TViewGroup {
public:
	//
	// Опции Capability
	//
	enum {
		wbcDrawBuffer         = 0x0001, // Окно использует буферизованную перерисовку
		wbcLocalDragClient    = 0x0002, // Окно является клиентом локального (в рамках процесса) обмена Drag'n'Drop
		wbcStorableUserParams = 0x0004, // @v12.2.6 Если флаг установлен, то при пользовательском изменении ряда параметров (e.g. координаты и размер)
			// эти параметры сохраняются в persistent-хранилище.
	};
	//
	// Descr: Опции функции TWindowBase::Create
	//
	enum {
		// @# coChild ^ coPopup
		coChild     = 0x0001,
		coPopup     = 0x0002,
		coMDI       = 0x0004,
		coScX       = 0x0008, // Окно создавать с горизонтальным скроллером
		coScY       = 0x0010, // Окно создавать с вертикальным скроллером
		coScXY      = (coScX|coScY),
		coMaxSize   = 0x0020, // Окно создавать с максимальными размерами, допускаемыми родительским окном
	};
	//
	// Descr: Структура, указатель на которую передается с сообщением cmInit
	//   // @v12.2.5 moved from TWindowBase to TWindow // следующий текст более не актуален (только для экземпляров, порожденных от TWindowBase).
	//
	struct CreateBlock {
		void * H_Process;
		void * H_Parent;
		void * H_Menu;
		TRect  Coord;
		uint32 Style;
		uint32 ExStyle;
		void * Param;
		const  char * P_WndCls;
		const  char * P_Title;
	};

	static int IsMDIClientWindow(HWND);

	explicit TWindow(const TRect & rRect);
	explicit TWindow(long wbCapability);
	~TWindow();
	long   GetWbCapability() const { return WbCapability; }
	void   endModal(ushort command);
	void * messageToCtrl(ushort ctl, ushort command, void * ptr);
	TView * FASTCALL getCtrlByHandle(HWND h);
	HWND   H() const { return this ? HW : static_cast<HWND>(0); }
	HWND   FASTCALL getCtrlHandle(ushort ctlID);
	void   STDCALL  setCtrlReadOnly(ushort ctlID, bool set);
	void   STDCALL  disableCtrl(ushort ctl, int toDisable);
	void   CDECL    disableCtrls(int toDisable, ...);
	// @v12.3.7 (moved to TViewGroup) void   FASTCALL selectCtrl(ushort ctl); 
	int    selectButton(ushort cmd);
	void   setCtrlOption(ushort id, ushort flag, int s);
	int    SetFont(const SFontDescr & rFd);
	int    SetCtrlFont(uint ctrlID, const char * pFontName, int height);
	int    SetCtrlFont(uint ctlID, const SFontDescr & rFd);
	int    SetCtrlsFont(const char * pFontName, int height, ...);
	int    destroyCtrl(uint ctl);
	TLabel * FASTCALL GetCtrlLabel(uint ctlID);
	TLabel * FASTCALL GetCtrlLabel(TView * pV);
	int    getLabelText(uint ctlID, SString & rText);
	int    setLabelText(uint ctlID, const char * pText);
	//
	// Функции setCtrlData и getCtrlData возвращают !0 если существует
	// управляющий элемент с ид. ctl и 0 в противном случае.
	//
	int    STDCALL  setCtrlData(ushort ctl, void *);
	int    STDCALL  getCtrlData(ushort ctl, void *);
	uint16 FASTCALL getCtrlUInt16(uint ctlID);
	long   FASTCALL getCtrlLong(uint ctl);
	double FASTCALL getCtrlReal(uint ctl);
	int    STDCALL  getCtrlString(uint ctlID, SString &);
	LDATE  FASTCALL getCtrlDate(uint ctlID);
	LTIME  FASTCALL getCtrlTime(uint ctlID);
	int    getCtrlDatetime(uint dtCtlID, uint tmCtlID, LDATETIME & rDtm);
	int    STDCALL  setCtrlUInt16(uint ctlID, int s);
	int    STDCALL  setCtrlLong(uint ctlID, long);
	int    STDCALL  setCtrlReal(uint ctlID, double);
	int    STDCALL  setCtrlString(uint ctlID, const SString &);
	int    STDCALL  setCtrlDate(uint ctlID, LDATE val);
	int    STDCALL  setCtrlTime(uint ctlID, LTIME val);
	int    setCtrlDatetime(uint dtCtlID, uint tmCtlID, LDATETIME dtm);
	int    setCtrlDatetime(uint dtCtlID, uint tmCtlID, LDATE dt, LTIME tm);
	//
	// Descr: Высокоуровневая функция, устанавливающая опцию option для SmartListBox::def
	//   с идентификатором ctlID (вызывает ListBoxDef::SetOption(option, 1)
	//
	int    setSmartListBoxOption(uint ctlID, uint option);
	void   FASTCALL drawCtrl(ushort ctlID);
	void   showCtrl(ushort ctl, bool s/*1 - show, 0 - hide*/);
	void   showButton(uint cmd, bool s/*1 - show, 0 - hide*/);
	int    SetButtonText(uint cmd, const char * pText);
	int    setButtonBitmap(uint cmd, uint bmpID);
	void   FASTCALL setTitle(const char *);
	void   FASTCALL setOrgTitle(const char *);
	void   FASTCALL setSubTitle(const char *);
	int    setStaticText(ushort ctl, const char *);
	int    getStaticText(ushort ctl, SString &);
	const  SString & getTitle() const;
	void   close();
	DECL_HANDLE_EVENT;
	virtual void   setState(uint aState, bool enable);
	int    translateKeyCode(ushort keyCode, uint * pCmd) const;
	void   setupToolbar(const ToolbarList * pToolBar);
	//
	// Descr: Высокоуровневая функция загружающая элементы инструментальной панели
	//   из ресурса P_SlRez с типом TV_EXPTOOLBAR.
	//   Если загрузка была успешной, то вызывает setupToolbar() и возвращает 1,
	//   в противном случае возвращает 0.
	// Returns:
	//   >0 - панель инструментов успешно загружена
	//   <0 - параметр toolbarResourceId == 0
	//    0 - error загрузки панели инструментов
	//
	int    LoadToolbarResource(uint toolbarResourceId);
	int    AddLocalMenuItem(uint ctrlId, uint buttonId, long keyCode, const char * pText);
	HWND   showToolbar();
	void   showLocalMenu();
	TRect  getClientRect() const;
	TRect  getRect() const;
	void   invalidateRect(const TRect &, bool erase);
	void   invalidateRect(const FRect &, bool erase);
	void   invalidateRegion(const SRegion & rRgn, bool erase);
	void   FASTCALL invalidateAll(bool erase);
	//
	// Descr: Инициализирует ожидание системой события покидания мышью клиентской области
	//   окна и (или) "замирания" мыши над клиентской областью (MouseHover).
	//   Если после вызова этой функции произошло одно из указанных событий, то
	//   окно получит сообщение WM_MOUSELEAVE или WM_MOUSEHOVER, соответсвенно.
	//   Если leaveNotify == 0 && hoverTimeout < 0 тогда предыдущее ожидание (если было) отменяется.
	// Note: См. описание функции WinAPI _TrackMouseEvent().
	// ARG(leaveNotify  IN): система будет реагировать на покидание мышью клиентской области окна.
	// ARG(hoverTimeout IN): если hoverTimeout < 0, то система не будет реагировать на
	//   событие "замирания" мыши. Если hoverTimeout > 0, то сообщение WM_MOUSEHOVER
	//   будет послано после hoverTimeout миллисекунд "замирания". Если hoverTimeout == 0,
	//   то таймаут "замирания" определяется системными параметрами.
	// Returns:
	//   !0 - функция успешно выполнена.
	//   0  - ошибка.
	//
	int    RegisterMouseTracking(int leaveNotify, int hoverTimeout);
	//
	// Descr: Возвращает текстовый символ управляющего элемента, ассоциированного
	//   с идентификатором id.
	// Returns:
	//   1 - символ найден и присвоен по ссылке rBuf
	//   0 - либо не найден элемент с идентификатором id, либо у него нет символа.
	//
	int    GetCtlSymb(uint id, SString & rBuf) const;
	void   InvalidateLayoutRefList(const SUiLayout::RefCollection & rRedrawLoList, int erase);
	bool   SetLayout(SUiLayout * pLo);
	//
	// Descr: Возвращает абстрактный указатель на объект LAYOUT, ассоциированный с окном this.
	// Returns:
	//   !0 - экземляр LAYOUT, ассоциированный с окном
	//    0 - с окном не ассоциирован объект LAYOUT
	//
	SUiLayout * GetLayout();
	void   EvaluateLayout(const TRect & rR);
	//
	// ARG(extraPtr IN): Дополнительные параметры, зависящие от типа управляющего элемента.
	//
	int    InsertCtlWithCorrespondingNativeItem(TView * pCtl, uint id, const char * pSymb, void * extraPtr);
	HWND   GetPrevInStackWindowHandle() const { return PrevInStack; }
	void   SetPrevInStackWindowHandle(HWND h) { PrevInStack = h; }
	//
	// Descr: Устанавливает дополнительный суффикс для символа окна, используемого в качестве ключа
	//   для сохранения пользовательских параметров окна.
	//
	bool   SetStorableUserParamsSymbSuffix(const char * pSufix);
	//
	//
	//
	struct StorableUserParams { // @v12.2.6
		StorableUserParams();
		StorableUserParams & Z();
		bool   IsEmpty() const { return (Origin.IsZero() && Size.IsZero()); }
		bool   FromJsonObj(const SJson * pJs);
		SJson * ToJsonObj() const;

		SPoint2I Origin;
		SPoint2I Size;
	};
protected:
	static BOOL CALLBACK SetupCtrlTextProc(HWND hwnd, LPARAM lParam);
	static int PassMsgToCtrl(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void __stdcall SetupLayoutItemFrame(SUiLayout * pItem, const SUiLayout::Result & rR);
	//
	// Descr: Вспомогательная функция, используемая при динамическом формировании диалога или окна (в т.ч. из ресурсов)
	//
	int    InsertCtl(TView * pCtl, uint id, const char * pSymb);
	int    SetCtlSymb(uint id, const char * pSymb);
	TView * FASTCALL CtrlIdToView(long id) const;
	int    RedirectDrawItemMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	//
	// Descr: Присваивает экземаляру this элемент LAYOUT, на который указывает указатель pLayout.
	//
	void   SetupLayoutItem(SUiLayout * pLayout);
	// @v12.2.6 {
	//
	// Descr: Фиксирует (если необходимо) размер и позицию окна, установленные пользователем.
	//
	int    ReckonUserPosition(const TRect & rRect);
	int    StoreUserParams();
	int    RestoreUserParams();
	void   SetStorableUserParamsSymb(const char * pSymb);
	const  StorableUserParams * GetStorableUserParams() const { return P_StUsrP; } 
	// } @v12.2.6 
	HWND   PrevInStack;
	HWND   HW;
	ToolbarList ToolbarL; // @v12.2.4 Toolbar-->ToolbarL
	SUiLayout * P_Lfc;    // @v12.2.4 (moved from TWindowBase)
	long   WbCapability;  // @v12.2.4 (moved from TWindowBase)
private:
	void   STDCALL Helper_SetTitle(const char *, int setOrgTitle);
	TButton * FASTCALL SearchButton(uint cmd);

	class LocalMenuPool {
	public:
		explicit LocalMenuPool(TWindow * pWin);
		int    AddItem(uint ctrlId, uint buttonId, long keyCode, const char * pText);
		int    GetCtrlIdByButtonId(uint buttonId, uint * pCtrlId) const;
		int    GetButtonIdByCtrlId(uint ctrlId, uint * pButtonId) const;
		int    ShowMenu(uint buttonId);
	private:
		struct Item {
			uint   CtrlId;
			uint   ButtonId;
			long   KeyCode;
			uint   StrPos;
		};
		SVector List;
		StringSet StrPool;
		TWindow * P_Win; // @notowned
	};
	SString Title;
	SString OrgTitle; // Этот заголовок устанавливается функцией setTitle(..., 1)
		// для того, чтобы в дальнейшем можно было использовать определенное в нем
		// форматирование функцией setSubTitle
	SString StorableUserParamsSymb; // @v12.2.6 Символ, идентифицирующий сохраняемые пользовательские параметры окна во внешнем хранилище.
	LocalMenuPool * P_Lmp;
	StrAssocArray * P_SymbList; // Специальный контейнер для хранения соответствий идентификаторов элементов и их символов.
	SVector * P_FontsAry;
	StorableUserParams * P_StUsrP; // @v12.2.6 Сохраняемые пользовательские параметры окна. Применяются если (WbCapability & wbcStorableUserParams).
};
//
//
//
class TWindowBase : public TWindow {
public:
	static int RegWindowClass(int iconId);
	~TWindowBase();
	int    Create(void * hParentWnd, long createOptions);
	int    AddChild(TWindowBase * pChildWindow, long createOptions, long zone);
	int    AddChildWithLayout(TWindowBase * pChildWindow, long createOptions, SUiLayout * pLayout);
protected:
	static void Helper_Finalize(HWND hWnd, TBaseBrowserWindow * pView);
	TWindowBase(LPCTSTR pWndClsName, long wbCapability);
	DECL_HANDLE_EVENT;
	void   SetDefaultCursor();

	// @v12.2.4 (moved to TWindow) SUiLayout * P_Lfc;
	SPaintToolBox Tb;
private:
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
	void   MakeMouseEvent(uint msg, WPARAM wParam, LPARAM lParam, MouseEvent & rMe);
	void   RegisterMouseTracking(int force);

	const  SString ClsName;     // Window class name
	enum {
		wbsMDI                  = 0x0001,
		wbsUserSettingsChanged  = 0x0002,
		wbsMouseTrackRegistered = 0x0004
	};
	long   WbState;
	uint32 H_DrawBuf;
};
//
//
//
struct TArrangeParam { // @persistent
	TArrangeParam();
	TArrangeParam & Init(int dir = DIREC_HORZ);
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);

	int16  Dir;        // DIREC_HORZ || DIREC_VERT
	uint16 RowSize;    // Количество элементов в ряду
	SPoint2S UlGap;      // Зазор между левой и верхней границами области и объектами
	SPoint2S LrGap;      // Зазор между правой и нижней границами области и объектами
	SPoint2S InnerGap;   // Зазор между объектами
	SPoint2S ForceSize;  // @unused Изменяет размер объектов до ForceSize. Если ForceSize.x <= 0,
		// то ширина не меняется, если ForceSize.y <= 0, то высота не меняется.
};
//
// Descr: Объект, управляющий коллекцией инструментов, доступных для вставки в TWhatman.
//   Каждый инструмент может быть представлен иконкой и фигурой. Если иконка не определена
//   при создании инструмента, тогда она генерируется автоматически из фигуры (ее масштабированием до размера иконки).
//   Иконка отображает инструмент в панели инструментов. Фигура предназначена для отображения //
//   инструмента уже на полотне ватмана.
//   Инструменты идентифицируются символом. Если при создании инструмента символ его не
//   определен, то класс автоматически присваивает ему уникальный 8-значный символ.
//
class TWhatmanToolArray : private SVector {
public:
	//
	// Descr: Пользовательское представление инструмента.
	//
	struct Item { // @transient [@wtmtoolitem]
		explicit Item(const TWhatmanToolArray * pOwner = 0);
		enum {
			fDontEnlarge   = 0x0001, // Не увеличивать фигуру при изменении размеров объекта сверх предустановленного размере
			fDontKeepRatio = 0x0002, // Не сохранять пропорции фигуры при изменении размера объекта
			fGrayscale     = 0x0004  // (testing option) Преобразовывать фигуру в черно-белый цвет
		};
		uint32  Id;              // Целочисленный идентификатор элемента
		SString Symb;         	 // Уникальный символ элемента.
		SString Text;         	 // Текстовое описание инструмента.
		SString WtmObjSymb;   	 // Символ класса семейства TWhatmanObject, который создается посредством данного инструмента.
		SString FigPath;      	 // Имя файла, содержащего изображение фигуры инструмента
		SString PicPath;      	 // Имя файла, содержащего изображение иконки инструмента
		SPoint2S FigSize;       	 // Начальный размер фигуры инструмента.
		SPoint2S PicSize;       	 // Если !isZero() то переопределяет TWhatmanToolArray::Param::PicSize
		long   Flags;         	 // @flags
		SColor ReplacedColor; 	 // Цвет, который должен замещаться на какой-либо внешний цвет. Если ReplacedColor.IsEmpty(), то замещаемый цвет не определен.
		SUiLayoutParam Alb; // @v10.9.10
		const  TWhatmanToolArray * P_Owner; // @notowned
		uint32 ExtSize;         // Размер данных, используемый элементом в буфере ExtData
		uint8  ExtData[256];
		
	};
	//
	// Descr: Пользовательское представление параметров коллекции.
	//
	struct Param { // @transient
		Param();

		SString Symb;         // Символ коллекции.
		SString Text;         // Описание коллекции.
		SString FileName;     // Имя файла, из которого был извлечен объект
		long   Flags;         // @flags
		SPoint2S PicSize;       // Размер иконки по умолчанию.
		TArrangeParam Ap;     // Параметры упорядочивания иконок
	};
	static uint32 GetSerializeSignature();
	TWhatmanToolArray();
	~TWhatmanToolArray();
	TWhatmanToolArray & Z();
	int    SetParam(const Param &);
	void   GetParam(Param &) const;
	uint   GetCount() const;
	int    Set(Item & rItem, uint * pPos);
	int    Remove(uint pos);
	int    Get(uint pos, Item * pItem) const;
	int    SearchById(uint id, uint * pPos) const;
	int    SearchBySymb(const char * pSymb, uint * pPos) const;
	int    GetBySymb(const char * pSymb, Item * pItem) const;
	int    Pack();
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	const  SDrawFigure * GetFig(int figOrPic, uint pos, TWhatmanToolArray::Item * pItem) const;
	const  SDrawFigure * GetFig(int figOrPic, const char * pSymb, TWhatmanToolArray::Item * pItem) const;
	const  SDrawFigure * GetFigById(int figOrPic, uint id, TWhatmanToolArray::Item * pItem) const;
	int    Store(const char * pFileName);
	int    Load(const char * pFileName);
	//int    LockStorage(const char * pFileName);
private:
	SString & MakeUniqueSymb(SString & rBuf) const;
	int    CreateFigure(const Item & rItem, const char * pPath, int pic);
	int    UpdateFigures(Item & rItem);

	struct Entry { // @persistent @flat
		Entry();
		uint32 TextP;            // Позиция текста в буфере Pool
		uint32 SymbP;          	 // Позиция символа в буфере Pool
		uint32 WtmObjSymbP;    	 // Позиция символа класса объекта в буфере Pool
		uint32 FigPathP;       	 // Позиция пути до файла фигуры в буфере Pool
		uint32 PicPathP;       	 // Позиция пути до файла пиктограммы в буфере Pool
		SPoint2S FigSize;        	 // Исходный размер фигуры при размещении на ватмане
		SPoint2S PicSize;        	 // Размер иконки
		int32  Flags;          	 // @flags
		uint32 ExtDataP;       	 // Позиция дополнительных данных в буфере Pool (в кодировке MIME64)
		uint32 Id;             	 // Целочисленный идентификатор элемента
		SColor ReplacedColor;  	 //
		SUiLayoutParam Alb; // @v10.9.10
	};
	uint32 SrcFileVer;  // @transient Версия формата хранения файла, из которого был загружен данный экземпляр объекта
	uint32 SymbP;
	uint32 TextP;
	uint32 FileP;   // @transient Имя файла, из которого был извлечен данный экземпляр
	int32  Flags;
	SPoint2S PicSize;
	TArrangeParam Ap;
	StringSet Pool; // Контейнер строковых констант (символы, описания, пути к файлам и т.д.)
	SDrawGroup Dg;  // Контейнер для иконок и фигур.
	//
	// Принадлежность иконок и фигур элементу контейнера определяется по символу Item.Symb.
	// Иконка хранится с символом "{SYMB}-PIC", а фигура с символом "{SYMB}-FIG"
	//
	SUiLayoutParam ALB__; // @v10.9.9 Параметры размещения объекта, создаваемого в соответствии с данным инструментом
};
//
// Descr: Фабрика создания экземпляра объекта ватмана.
//
class TWhatmanObject;
typedef TWhatmanObject * (*FN_WTMOBJ_FACTORY)();

#define IMPLEMENT_WTMOBJ_FACTORY(symb, name) \
	struct FClsWtmo##symb { \
		static TWhatmanObject * Factory() { return new WhatmanObject##symb; } \
		FClsWtmo##symb() { TWhatmanObject::Register(#symb, name, FClsWtmo##symb::Factory); } \
	}; static FClsWtmo##symb _RegWtmo##symb;

#define WTMOBJ_REGISTER(symb, name) \
	IMPLEMENT_WTMOBJ_FACTORY(symb, name); \
	void WhatmanObject##symb::OnConstruction() { Symb = #symb; }
//
// Descr: Объекта ватмана.
//
class TWhatmanObject { // @persistent
public:
	friend class TWhatman;
	//
	// Descr: Параметры текста, сопоставленного объекту.
	//
	struct TextParam { // @persistent
		TextParam();
		void   SetDefault();

		int16  Side;        // SIDE_XXX Сторона объекта, с которой располагается текст.
			// Если Side == SIDE_CENTER, то центр текста располагается по центру объекта.
		uint16 Flags;       // @flags
		float  AlongSize;   // Размер текста вдоль той стороны, к которой он примыкает.
			// Если величина отрицательная, то абсолютное значение определяет размер в долях от стороны.
			// Нулевое значение означает, что размер текста вдоль примыкающей стороны равен этой строне.
			// Если Side == SIDE_CENTER, то этот размер отмеряется относительно горизонтальной стороны.
		float  AcrossSize;  // Размер текста перпендикулярно стороне, к которой он примыкает.
			// Если величина отрицательная, то абсолютное значение определяет размер в долях от стороны,
			// перпендикулярной той, к которой примыкает текст.
			// Нулевое значение означает, что размер текста перпеникулярно примыкающей стороне не ограничен.
			// Если Side == SIDE_CENTER, то этот размер отмеряется относительно вертикальной стороны.
		int32  CStyleIdent;
		int32  ParaIdent;
	};

	struct SelectObjRetBlock {
		SString WtmObjTypeSymb;
		int32  Val1;
		int32  Val2;
		SString ExtString;
	};
	//
	// Descr: Регистрирует класс, порожденный от TWhatmanObject.
	//
	static int Register(const char * pSymb, const char * pName, FN_WTMOBJ_FACTORY factory);
	//
	// Descr: Создает экземпляр класса, порожденного от TWhatmanObject по регистрационному
	//   идентификатору id.
	//
	static TWhatmanObject * CreateInstance(long id);
	//
	// Descr: Создает экземпляр класса, порожденного от TWhatmanObject по зарегистрированному
	//   символу pSymb.
	//
	static TWhatmanObject * CreateInstance(const char * pSymb);
	static int GetRegSymbById(long id, SString & rSymb);
	static long GetRegIdBySymb(const char * pSymb);
	static StrAssocArray * MakeStrAssocList();
	virtual ~TWhatmanObject();
	enum {
		cmdNone = 0,
		cmdSetBounds,      // (TRect *)
		cmdSetupByTool,    // (TWhatmanToolArray::Item *)
		cmdEditTool,       // (TWhatmanToolArray::Item *)
		cmdEdit,
		cmdDblClk,
		cmdMouseHover,
		cmdGetSelRetBlock, // (TWhatmanObject::SelectObjRetBlock *)
		cmdObjInserted     // Посылается объекту после того, как он был вставлен в контейнер.
	};
	//
	// Descr: Структура, передаваемая с командой cmdSetupByTool
	//
	class SetupByToolCmdBlock {
	public:
		explicit SetupByToolCmdBlock(const TWhatmanToolArray::Item * pToolItem);
		const TWhatmanToolArray::Item * P_ToolItem; // Инструмент, определяющий создание нового экземпляра объекта
		TSCollection <TWhatmanObject> AfterInsertChain; // Опциональный список объектов, которые должны быть вставлены после нового экземпляра объекта
			// Этот список может быть сформирован функцией HandleCommand для формирования комплексных объектов.
	private:
		SetupByToolCmdBlock(const SetupByToolCmdBlock & /*rS*/) {} // disable copy-constructor
		SetupByToolCmdBlock & operator = (const SetupByToolCmdBlock & /*rS*/) {} // disable assignment
	};
	virtual int HandleCommand(int cmd, void * pExt);
	virtual TWhatmanObject * Dup() const;
	virtual int Draw(TCanvas2 & rCanv);
	virtual int Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	//
	// Descr: Опции функции GetTextLayout()
	//
	enum {
		gtloBoundsOnly    = 0x0001, // Если объект требует отрисовки текста, то
			// функция обязана возвратить STextLayout в котором только определены границы текста.
		gtloQueryForArrangeObject = 0x0002  // Если флаг установлен, то функция должна
			// вернуть >0 если текст следует учесть при автоматической ранжировки
			// объектов функцией TWhatman::ArrangeObjects.
			// В этом случае GetTextLayout обязана определить границы текста.
	};
	virtual int GetTextLayout(STextLayout & rTl, int options) const;
	//
	// Descr: Редактирует элемент панели инструментов.
	//   Если поле pItem->Symb пустое, то элемент - новый (создаваемый),
	//   в противном случае элемент уже существует в контейнере инструментов
	//   и его символ менять нельзя.
	// Returns:
	//   >0 - редактирование элемента прошло успешно. Следует принять изменения.
	//   <0 - отказ от редактирования. Никакие изменения не принимаются.
	//   0  - ошибка.
	//
	int    EditTool(TWhatmanToolArray::Item * pItem); // @>>HandleCommand(cmdEditTool, pItem)
	//
	// Descr: Редактирует параметры объекта.
	//   >0 - редактирование параметров прошло успешно. Следует принять изменения.
	//   <0 - откраз от редактирования. Никакие изменения не принимаются.
	//   0  - ошибка.
	//
	int    Edit(); // @>>HandleCommand(cmdEdit, 0)
	//
	// Descr: Устанавливает параметры объекта в соответствии с элементом
	//   контейнера инструментов, посредством которого этот объект был создан.
	//
	int    Setup__(SetupByToolCmdBlock & rBlk); // @>>HandleCommand(cmdSetupByTool, 0)
	int    SetBounds(const TRect & rRect); // @>>HandleCommand(cmdSetBounds, 0)
	void   FASTCALL Copy(const TWhatmanObject & rS);
	TRect  GetBounds() const;
	TRect  GetTextBounds() const;
	//
	// Descr: Возвращает !0 если опция f установлена в поле TWhatmanObject::Options.
	//
	bool   FASTCALL HasOption(int f) const;
	bool   FASTCALL HasState(int f) const;
	int    SetTextOptions(const TextParam * pParam);
	const TextParam & GetTextOptions() const;
	//
	// Descr: Вспомогательная функция, возвращающая прямоугольник перерисовки,
	//   соответствующий границам объекта.
	//
	TRect  GetInvalidationRect() const;
	int    DrawToImage(SPaintToolBox & rTb, SImageBuffer & rImg);
	//
	// Descr: Заставляет окно, владеющее этим объектом (если таковое имеется)
	//   перерисовать данный объект.
	//
	int    Redraw();
	TWhatman * GetOwner() const;
	TWindow * GetOwnerWindow() const;
	const  SString & GetObjTypeSymb() const;
	const  SString & GetIdentSymb() const;
	int    SetIdentSymb(const char * pIdent);
	const  SUiLayoutParam & GetLayoutBlock() const;
	void   SetLayoutBlock(const SUiLayoutParam * pBlk);
	const  SString & GetLayoutContainerIdent() const;
	void   SetLayoutContainerIdent(const char * pIdent);

	enum {
		stCurrent    = 0x0001,
		stSelected   = 0x0002,
		stContainerCandidate = 0x0004  // @v10.9.6 Объект является кандидатом на превращение в контейнера-владельца для какого-либо иного объекта
	};
	enum {
		oMovable        = 0x0001, // Объект может перемещаться пользователем
		oResizable      = 0x0002, // Пользователь может менять размер объекта
		oDraggable      = 0x0004, // Объект используется для Drag'n'Drop обмена
		oBackground     = 0x0008, // Фоновый объект. Такой объект может быть только один. Его размер равен размеру ватмана.
			// При добавлении нового объекта с этим признаком, предыдущий уничтожается.
		oSelectable     = 0x0010, // Объект может быть выбран в окне, режим которого предполагает выбор некоторого объекта.
		oFrame  = 0x0020, // Активной частью объекта является только рамка.
		oMultSelectable = 0x0040, // Объект может быть включен в список множественного выбора объектов
		oContainer      = 0x0080  // @v10.9.6 Объект является контейнером. Это, в том числе, означает, что
			// иной объект может быть включен в этот контейнер.
			// Пока понятие принадлежности контейнеру принимаем эксклюзиным, то есть, объект может принадлежать
			// не более, чем одному контейнеру.
			// Мотивация: реализация layout
	};
protected:
	explicit TWhatmanObject(const char * pSymb);
	
	TextParam TextOptions;
	long   Options;
	long   State;       // @transient
private:
	SString WtmObjTypeSymb__;   // Символ идентификации класса объекта
	SString IdentSymb; // @v11.2.2 Символ идентификации экземпляра
	TRect  Bounds;
	SString LayoutContainerIdent; // @v10.4.8 @persistent Символ родительского объекта типа Layout
	SUiLayoutParam Le2; // @v10.9.8 @persistent 
	TWhatman * P_Owner; // @transient
};
//
// Descr: Базовый класс для реализации Layout. Создан для того, чтобы предоставить классу TWhatman
//   доступ к частичной имплементации Layout. Полная имплементация должна быть предоставлена
//   на более высоком уровне.
//
class WhatmanObjectLayoutBase : public TWhatmanObject {
public:
	const SString & GetContainerIdent() const { return ContainerIdent; }
	void   SetContainerIdent(const char * pIdent)
	{
		(ContainerIdent = pIdent).Strip();
	}
protected:
	WhatmanObjectLayoutBase();
	~WhatmanObjectLayoutBase();
	virtual TWhatmanObject * Dup() const;
	virtual int Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL Copy(const WhatmanObjectLayoutBase & rS);

	SString ContainerIdent; // Идентификатор контейнера для ссылки на него вложенных элементов
};
//
// Descr: Ватман. Контейнер, содержащий объекты TWhatmanObject, которые могут отрисовываться на
//   полотне, перемещаться и выполнять прочие функции визуализации.
//
class TWhatman { // @persistent
public:
	struct Param {
		enum {
			fRule       = 0x0001, // Отображать линейки
			fGrid       = 0x0002, // Отображать сетку
			fDisableMoveObj     = 0x0004, // Объекты нельзя перемещать, даже если конкретный объект допускает это
			fDisableReszObj     = 0x0008, // Объекты нельзя изменять в размерах, даже если конкретный объект допускает это
			fSnapToGrid = 0x0010, // При перемещении или изменении размеров объектов притягивать их координаты к решетке
				// (действует только если установлен флаг fGrid).
			fOneClickActivation = 0x0020  // Объект (TWhatmanObject::oSelectable)
				// активируется одним кликом мыши (вместо двойного щелчка).
		};
		Param();

		int32  Unit;
		double UnitFactor;
		double Scale;
		long   Flags;        // @flags
		TRect  InitSize;
		TRect  ScrollMargin; // Дополнительное пространство скроллирования (плюс к области, занимаемой фигурами) //
		uint8  Reserve[64];
		SString Name;
		SString Symb;
	};
	enum {
		toolPenObjBorder = 1,
		toolPenObjBorderSel,
		toolPenObjBorderCur,
		toolPenObjRszSq,        // Квадраты для изменения размеров объекта
		toolBrushObjRszSq,      // Заливка квадратов для изменения размеров объекта
		toolPenObjNonmovBorder, // Граница неперемещаемого объекта
		toolPenRule,
		toolBrushRule,
		toolPenGrid,
		toolPenSubGrid,
		toolPenLayoutBorder,     // @v10.4.8
		toolPenLayoutEvenBorder, // @v11.2.2
		toolPenLayoutOddBorder,  // @v11.2.2
		toolPenContainerCandidateBorder // @v10.9.6
	};
	static uint32 GetSerializeSignature();
	explicit TWhatman(TWindow * pOwnerWin);
	~TWhatman();
	SPoint2S FASTCALL TransformPointToScreen(SPoint2S p) const;
	SPoint2F FASTCALL TransformPointToScreen(SPoint2F p) const;
	SPoint2S FASTCALL TransformScreenToPoint(SPoint2S p) const;
	TWindow * GetOwnerWindow() const;
	const  Param & GetParam() const;
	int    SetParam(const Param &);
	void   Clear();
	int    InsertObject(TWhatmanObject * pObj, int beforeIdx = -1);
	int    EditObject(int objIdx);
	int    RemoveObject(int idx);
	int    CheckUniqLayoutSymb(const TWhatmanObject * pObj, const char * pIdentSymb) const;
	int    GetLayoutSymbList(StrAssocArray & rList) const;
	//
	// Descr: Перемещает объект с индексом idx на передний план.
	//   То есть, объект становится последним в списке ObjList.
	// Returns:
	//   >0 - объект перемещен на передний план.
	//   <0 - функция ничего не сделала либо потому, что индекс idx выходит
	//     за границы списка ObjList либо потому, что в списке всего один элемент.
	//
	int    BringObjToFront(int idx);
	//
	// Descr: Перемещает объект с индексом idx на задний план.
	//   То есть, объект становится первым в списке ObjList.
	// Returns:
	//   >0 - объект перемещен на передний план.
	//   <0 - функция ничего не сделала либо потому, что индекс idx выходит
	//     за границы списк ObjList либо потому, что в списке всего один элемент.
	//
	int    SendObjToBack(int idx);
	int    SetCurrentObject(int idx, int * pPrevCurObjIdx);
	int    FASTCALL GetCurrentObject(int * pIdx) const;
	//
	// Descr: Включает объект с индексом idx в спискок множественного выбора.
	// Returns:
	//   >0 - объект успешно добавлен
	//   -1 - объект не может быть добавлен по-скольку не имеет признака TWhatmanObject::oMultSelectable
	//   -2 - объект не добавлен, по-скольку уже находится в списке
	//    0 - error: либо объект с индексом idx не существует, либо общая ошибка (например, SLERR_NOMEM).
	//
	int    AddMultSelObject(int idx);
	//
	// Descr: Удаляет объект с индексом idx из списка множественного выбора.
	//   Если idx == -1, то список множественного выбора полностью очищается.
	// Returns:
	//   >0 - объект успешно удален из списка, либо (если idx == -1) список успешно очищен.
	//   <0 - объект не содержится в списке, либо (если idx == -1) список уже пуст.
	//
	int    RmvMultSelObject(int idx);
	int    SetupMultSelBySelArea();
	//
	// Descr: Определяет, включен ли объект с индексом idx в список множественного выбора.
	// Returns:
	//   !0 - объект с индексом idx включен в список множественного выбора.
	//   0  - объект с индексом idx не включен в список множественного выбора.
	//
	int    FASTCALL IsMultSelObject(int idx) const;
	int    FASTCALL HaveMultSelObjectsOption(int f) const;
	const  LongArray * GetMultSelIdxList() const;
	int    FindObjectByPoint(SPoint2S p, int * pIdx) const;
	int    FindContainerCandidateForObjectByPoint(SPoint2S p, const TWhatmanObject * pObj, int * pIdx) const;
	int    GetContaiterCandidateIdx() const { return ContainerCandidatePos; }
	void   SetupContainerCandidate(int idx, bool set);
	int    MoveObject(TWhatmanObject * pObj, const TRect & rRect);
	uint   GetObjectsCount() const;
	TWhatmanObject * FASTCALL GetObjectByIndex(int idx);
	const  TWhatmanObject * FASTCALL GetObjectByIndexC(int idx) const;
	const  WhatmanObjectLayoutBase * FASTCALL GetObjectAsLayoutByIndexC(int idx) const;
	WhatmanObjectLayoutBase * FASTCALL GetObjectAsLayoutByIndex(int idx);
	//
	// Descr: Возвращает индекс корневого контейнера, содержащего лейаут pC.
	// Return:
	//   >= 0 - индекс корневого контейнера
	//   <  0 - pC не является лейаутом либо не удалось найти корневой контейнер 
	//
	int    GetRootLayoutObjectIndex(const WhatmanObjectLayoutBase * pC) const;
	//
	// Descr: Возвращает родительский лейаут для объекта pC, который так же должен 
	//   являться лейаутом.
	// Returns:
	//   !0 - указатель на родительский лейаут
	//    0 - либо объект pC не является лейаутом, либо он, будучи лейаутом, находится на верхнем уровне (не имеет предка)
	//
	const  WhatmanObjectLayoutBase * GetParentLayoutObject(const WhatmanObjectLayoutBase * pC) const;
	int    FASTCALL InvalidateObjScope(const TWhatmanObject * pObj);
	int    GetObjTextLayout(const TWhatmanObject * pObj, STextLayout & rTl, int options);
	int    Draw(TCanvas2 & rCanv);
	//
	// Descr: Отрисовывает объект pObj на полотне rCanv с применение
	//   преобразования координат. Применяется клиентом класса для отрисовки
	//   объекта, не включенного в коллекцию this.
	//
	int    DrawSingleObject(TCanvas2 & rCanv, TWhatmanObject * pObj);
	int    DrawObjectContour(TCanvas2 & rCanv, const TWhatmanObject * pObj, const SPoint2S * pOffs);
	int    DrawMultSelContour(TCanvas2 & rCanv, const SPoint2S * pOffs);
	int    InvalidateMultSelContour(const SPoint2S * pOffs);
	//
	// @ARG(dir IN): SOW_XXX
	//
	int    ResizeObject(TWhatmanObject * pObj, int dir, SPoint2S toPoint, TRect * pResult);
	int    SetArea(const TRect & rArea);
	const  TRect & GetArea() const;
	//
	// ARG(mode):
	//   1 - start point
	//   2 - end point
	//   0 - reset
	//
	int    SetSelArea(SPoint2S p, int mode);
	const  TRect & GetSelArea() const;
	void   SetScrollPos(SPoint2S p);
	void   GetScrollRange(IntRange * pX, IntRange * pY) const;
	SPoint2S GetScrollDelta() const;
	int    SetTool(int toolId, int paintObjIdent);
	int    GetTool(int toolId) const;
	//int    ArrangeObjects(const LongArray * pObjPosList, const TArrangeParam & rParam);
	int    ArrangeObjects2(const LongArray * pObjPosList, const TArrangeParam & rParam, SScroller * pScrlr);
	SUiLayout * CreateLayout(WhatmanObjectLayoutBase * pC);
	int    ArrangeLayoutContainer(WhatmanObjectLayoutBase * pC);
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    Store(const char * pFileName);
	int    Load(const char * pFileName);
	static float GetRuleWidth();
private:
	struct ObjZone {
		int    I;
		FRect  R;
	};
	//
	// Descr: Засечка рулетки
	//
	struct RuleNotch {
		enum {
			fSnap = 0x0001 // Засечка должна "притягивать" объект
		};
		float  P; // Позиция засечки
		float  H; // Высота засечки в долях от высоты рулетки
		long   F; // Флаги
	};
	//
	// Descr: Параметры рулетки
	//
	struct Rule {
		void   Init();
		int    AddNotch(float p, float h, long f = 0);
		int    AddNotchList(uint c, float subDelta, const float * pHiList);
		int    GetNearest(float p, float size, float * pNearestP) const;
		double OneUnitLog10;
		double OneUnitDots;
		int    ScrollDelta; // @unit{px} Квант скроллирования //
		TSVector <RuleNotch> NotchList;
	};

	int    DrawObject(TCanvas2 & rCanv, TWhatmanObject * pObj);
	void   GetResizeRectList(const TWhatmanObject * pObj, ObjZone * pList) const;
	void   GetFrameRectList(const TWhatmanObject * pObj, ObjZone * pList) const;
	int    CalcRule(double ptPerInch, Rule & rResult) const;
	int    GetNotchList(const Rule & rRule, float size, float offs, int kind, TSVector <RuleNotch> & rList) const;
	int    SnapX(float p, float * pDest) const;
	int    SnapY(float p, float * pDest) const;
	int    CalcScrollRange();
	SUiLayout * Helper_CreateLayout(SUiLayout * pParentLayout, WhatmanObjectLayoutBase * pC, TSVector <uintptr_t> & rRecurList) const;
	// @v11.7.10 int    Helper_ArrangeLayoutContainer(SUiLayout * pParentLayout, WhatmanObjectLayoutBase * pC);

	uint32 SrcFileVer;  // @transient Версия формата хранения файла, из которого был загружен данный экземпляр объекта
	TRect  Area;        // @transient Видимая область
	TRect  SelArea;     // @transient Область, выделенная пользователем для выбора нескольких объектов
	TRect  ScrollRange; // @transient Автоматически рассчитываемый диапазон скроллирования области просмотра //
	SPoint2S ScrollPos; // @transient Позиция скроллеров
	Rule   RuleX;       // @transient
	Rule   RuleY;       // @transient
	Param  P;
	TSCollection <TWhatmanObject> ObjList;
	int    CurObjPos; // Позиция активного объекта. -1 - активного объекта нет.
	int    ContainerCandidatePos; // @v10.9.7 Позиция контейнера-кандидата на владение перемещаемым объектом. -1 - нет.
	LongArray * P_MultObjPosList; // Позиции объектов, к которым применен множественный выбор.
	TWindow * P_Wnd; // @notowned @transient
	int    TidPenObjBorder;
	int    TidPenObjBorderSel;
	int    TidPenObjBorderCur;
	int    TidPenObjRszSq;
	int    TidBrushObjRszSq;
	int    TidPenObjNonmovBorder; // toolPenObjNonmovBorder
	int    TidPenRule;
	int    TidBrushRule;
	int    TidPenGrid;
	int    TidPenSubGrid;
	int    TidPenLayoutBorder;
	int    TidPenLayoutEvenBorder; // @v11.2.2
	int    TidPenLayoutOddBorder; // @v11.2.2
	int    TidPenContainerCandidateBorder;
};
//
//
//
class CtrlGroup {
public:
	CtrlGroup();
	virtual ~CtrlGroup();
	virtual void handleEvent(TDialog *, TEvent &);
	virtual int setData(TDialog *, void *);
	virtual int getData(TDialog *, void *);
	void    SetPassive() { State |= stPassive; } // @v12.0.7
	void    SetActive() { State &= ~stPassive; } // @v12.0.7
	bool    IsPassive() const { return LOGIC(State & stPassive); } // @v12.0.7
private:
	friend class TDialog;
	enum {
		stPassive = 0x0001 // Группа пассивная. То есть, не отвечает на сообщения методом handleEvent
	};
	uint   Id;
	uint   State; // @v12.0.7
};

typedef int (* DialogPreProcFunc)(TDialog *, void * extraPtr);
//
// Descr: Структура описания вида элемента пользовательского интерфейса
//
struct UiItemKind { // @transient
	//
	// Descr: Числовые дескрипторы видов элементов пользовательского интерфейса
	// Attention: @persistent
	//
	enum { // @persistent
		kUnkn         =  0, // Неопределенный
		kDialog       =  1, // @anchor Диалог
		kInput        =  2, // Поле ввода
		kStatic       =  3, // Статический текст
		kPushbutton   =  4, // Кнопка
		kCheckbox     =  5, // Одиночный флаг
		kRadioCluster =  6, // Кластер переключателей
		kCheckCluster =  7, // Кластер флагов
		kCombobox     =  8, // Комбо-бокс
		kListbox      =  9, // Список (возможно, многоколоночный)
		kTreeListbox  = 10, // Древовидный список
		kFrame        = 11, // Рамка
		kLabel        = 12, // Текстовая этикетка, привязанная к другому элементу
		kRadiobutton  = 13, // Радиокнопка (применяется только как связанный с kRadioCluster элемент)
		kGenericView  = 14, // Обобщенный элемент view
		kImageView    = 15, // @v11.0.6 Изображение
		kCount,             // @anchor Специальный элемент, равный количеству видов
	};
	//
	// Descr: Битовые поля, используемые для представления булевых свойств элементов UI.
	//   Эти флаги не должны пересекаться по смыслу с опциями layout-свойств
	//
	enum { // @persistent
		fReadOnly   = 0x0001,
		fTabStop    = 0x0002,
		fStaticEdge = 0x0004,
		fDisabled   = 0x0008,
		fHidden     = 0x0010,
		fMultiLine  = 0x0020, // for inputline
		fWantReturn = 0x0040, // for inputline
		fPassword   = 0x0080, // for inputline
		fDefault    = 0x0100  // элемент имеет default-статус. Может применяться к pushbutton и radiobutton
	};

	static int  GetTextList(StrAssocArray & rList);
	static int  GetIdBySymb(const char * pSymb);
	static bool GetSymbById(int id, SString & rBuf);
	explicit UiItemKind(int kind = kUnkn);
	int    Init(int kind);

	int32  Id;
	SString Symb;
	SString Text;
	TView * P_Cls;
};

class TDialog : public TWindow {
	friend class PPDialogConstructor; // @v12.3.6 Этот класс получает доступ к внутренним полям и методам TDialog поскольку это необходимо
		// для создания экземпляра класса TDialog с использованием инфраструктуры DL600, которая находится на уровне проекта Papyrus.
public:
	//
	// Descr: Флаги функции TDialog::LoadDialog
	//
	enum {
		ldfDL600_Cvt = 0x0001
	};
	static int LoadDialog(TVRez * rez, uint dialogID, TDialog * dlg, long flags);
	//
	// Descr: Специализированная функция, используемая для обработки описаний диалогов.
	//
	static int GetSymbolBody(const char * pSymb, SString & rBodyBuf);
	//
	// Descr: Флаги состояния объекта (DlgFlags)
	//
	enum {
		fCentered      = 0x0001,
		fModified      = 0x0002,
		fUserSettings  = 0x0004,
		fCascade       = 0x0008,
		fResizeable    = 0x0010,
		fMouseResizing = 0x0020,
		fLarge         = 0x0040, // Диалог увеличин в размерах для использования с TouchScreen
		fExport        = 0x0080, // Экземпляр диалога создан для экспорта
	};
	enum ConstructorOption {
		coNothing = 0,
		coExport  = 1,
		coEmpty   = 2, // @v12.2.4 Предписывает создавать пустое окно диалога (без указания ресурса описания диалога)
	};
	static  INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
	static  void centerDlg(HWND);
	int     SetCtrlToolTip(uint ctrlID, const char * pToolTipText);
	// @v12.2.4 TDialog(const TRect & rRect, const char * pTitle);
	TDialog(const char * pTitle, long wbCapability, ConstructorOption co/*= coNothing*/); // @v12.2.4
	TDialog(uint resID, DialogPreProcFunc, void * extraPtr);
	explicit TDialog(uint resID);
	TDialog(uint resID, ConstructorOption co); // special constructor.
	~TDialog();
	virtual int    TransmitData(int dir, void * pData);
	// @v12.2.6 virtual int    FASTCALL valid(ushort command);
	//
	// Descr: Запускает немодальное окно диалога
	//
	int    Insert();
	void   ToCascade();
	//
	// Descr: Возвращает !0 если в поле состояния установлен флаг f.
	//
	bool   FASTCALL CheckFlag(long f) const;
	int    FASTCALL addGroup(ushort grpID, CtrlGroup*);
	int    FASTCALL setGroupData(ushort, void *);
	int    FASTCALL getGroupData(ushort, void *);
	CtrlGroup * FASTCALL getGroup(ushort);
	long   getVirtButtonID(uint ctlID);
	int    SaveUserSettings();
	int    RestoreUserSettings();
	void   SetDlgTrackingSize(MINMAXINFO * pMinMaxInfo);
	//int    SetRealRangeInput(uint ctlID, const RealRange *);
	//int    GetRealRangeInput(uint ctlID, RealRange *);
	//int    SetPeriodInput(uint ctlID, const DateRange *);
	//int    GetPeriodInput(uint ctlID, DateRange *);
	int    STDCALL AddClusterAssoc(uint ctlID, long pos, long val);
	//
	// Descr: То же, что и AddClusterAssoc, но, кроме того, для radio-buttons
	//   устанавливает это же значение val как значение по умолчанию (pos = -1)
	//
	int    STDCALL AddClusterAssocDef(uint ctlID, long pos, long val);
	int    STDCALL SetClusterData(uint ctlID, long);
	int    STDCALL GetClusterData(uint ctlID, int *);
	int    STDCALL GetClusterData(uint ctlID, long *);
	int    STDCALL GetClusterData(uint ctlID, int16 *);
	long   STDCALL GetClusterData(uint ctlID);
	void   DisableClusterItem(uint ctlID, int itemNo /* 0.. */, bool toDisable = true);
	void   DisableClusterItems(uint ctlID, const LongArray & rItemIdxList /* 0.. */, bool toDisable = true);
	int    SetClusterItemText(uint ctlID, int itemNo /* 0.. */, const char * pText);
	int    GetClusterItemByAssoc(uint ctlID, long val, int * pPos);
	int    SetDefaultButton(uint ctlID, bool setDefault);
	int    SetCtrlBitmap(uint ctlID, uint bmID);
	int    SetupInputLine(uint ctlID, TYPEID typ, long fmt);
	void   SetupSpin(uint ctlID, uint buddyCtlID, int low, int upp, int cur);
	void   SetupCalDate(uint calCtlID, uint inputCtlID);
	void   SetupCalPeriod(uint calCtlID, uint inputCtlID);
	void   SetCtrlState(uint ctlID, uint state, bool enable);
	//
	// Descr: Ассоциирует с элементом диалога специальный список выбора, который фильтруется по мере ввода текста.
	//   Если proc = 0, то используется GetListFromSmartLbx
	//   Если wordSelExtra = 0 и элемент ctlID является списком или комбобоксом, то wordSelExtra = (long)SmartListBox*
	//
	int    SetupWordSelector(uint ctlID, WordSel_ExtraBlock * pExtra, long id, int minSymbCount, long flags);
	int    ResetWordSelector(uint ctlID);

	TView * P_Frame;
protected:
	DECL_HANDLE_EVENT;

	struct UserSettings {
		int    Ver;
		int    Left;
		int    Top;
	};
	//
	// Устанавливает положение окна в соответствии с флагами.
	// if(DlgFlags & fUserSettings)
	//     Позиция верхнего левого угла устанавливается по координатам Settings.Left
	//     и Settigns.Top с поправкой на размеры экрана.
	// else if(DlgFlags & fCentered)
	//     Окно центрируется //
	// Вызывается из функции execute()
	//
	int    setupPosition();
	//
	// Изменение размеров окна диалога
	//
	enum CtrlResizeFlags {
		crfLinkLeft   = 0x0001,
		crfLinkRight  = 0x0002,
		crfLinkTop    = 0x0004,
		crfLinkBottom = 0x0008,
		crfResizeable = 0x0010,
		crfWClusters  = 0x0020
	};
#define CRF_LINK_LEFTRIGHT        (crfLinkLeft  | crfLinkRight)
#define CRF_LINK_LEFTTOP          (crfLinkLeft  | crfLinkTop)
#define CRF_LINK_RIGHTTOP         (crfLinkRight | crfLinkTop)
#define CRF_LINK_LEFTBOTTOM       (crfLinkLeft  | crfLinkBottom)
#define CRF_LINK_RIGHTBOTTOM      (crfLinkRight | crfLinkBottom)
#define CRF_LINK_TOPBOTTOM        (crfLinkTop   | crfLinkBottom)
#define CRF_LINK_LEFTTOPBOTTOM    (crfLinkLeft  | crfLinkTop   | crfLinkBottom)
#define CRF_LINK_RIGHTTOPBOTTOM   (crfLinkRight | crfLinkTop   | crfLinkBottom)
#define CRF_LINK_LEFTRIGHTBOTTOM  (crfLinkLeft  | crfLinkRight | crfLinkBottom)
#define CRF_LINK_LEFTRIGHTTOP     (crfLinkLeft  | crfLinkRight | crfLinkTop)
#define CRF_LINK_ALL              (crfLinkLeft  | crfLinkRight | crfLinkTop | crfLinkBottom)
	//
	// Descr: Устанавливает параметры поведения контрола при изменении размера диалога
	// ARG(ctrlID IN):                 ID описываемого контрола
	// ARG(xCtrl (x = l, t, r, b) IN): контролы, к которым привязан контрол (0 - к границе диалога, -1 - не привязан);
	//   если (ctrlResizeFlags & crfLinkXXX), то контролы связаны соответствующей стороной,
	//   иначе будут пересчитываться совместно (функция Helper_ToRecalcCtrlSet)
	//   для xCtrl == 0 соответствующий crfLinkXXX подразумевается по умолчанию;
	// ARG(ctrlResizeFlags IN):        CtrlResizeFlags, если (ctrlResizeFlags | crfResizeable),
	//   то растянется между указанными контролами, иначе переместится пропорционально
	//
	int    SetCtrlResizeParam(long ctrlID, long lCtrl, long tCtrl, long rCtrl, long bCtrl, long ctrlResizeFlags);
	int    LinkCtrlsToDlgBorders(long ctrlResizeFlags, ...);
	int    ResizeDlgToRect(const RECT * pRect);
	int    ResizeDlgToFullScreen();
	void   InitControls(HWND hwndDlg, WPARAM wParam, LPARAM lParam);

	struct BuildEmptyWindowParam {
		BuildEmptyWindowParam() : FontSize(0)
		{
		}
		SString FontFace;
		int   FontSize;
	};

	int    BuildEmptyWindow(const BuildEmptyWindowParam * pParam); // @v12.2.5
	bool   ValidateCommand(TEvent & rEv); // @v12.2.6

	UserSettings Settings;
	long   DlgFlags;
	void * P_PrevData;
private:
	void   Helper_Constructor(uint resID, DialogPreProcFunc dlgPreFunc, void * extraPtr, ConstructorOption co); // @<<TDialog::TDialog
	void   RemoveUnusedControls();
	void   SetupCalDate_Internal(uint calCtlID, uint inputCtlID);
	void   SetupCalPeriod_Internal(uint calCtlID, uint inputCtlID);

	uint   GrpCount;
	CtrlGroup ** PP_Groups;
	HWND   ToolTipsWnd;
	//
	struct ResizeParamEntry {
		long   CtrlID;
		HWND   CtrlWnd;
		long   Left;
		long   Top;
		long   Right;
		long   Bottom;
		long   Flags; // CtrlResizeFlags
	};
	TSVector <ResizeParamEntry> ResizeParamAry;
	//
	// Descr: Пересчитывает координаты контрола
	// ARG(firstCoord IN):           новая 1-ая координата диапазона, в котором должен находиться контрол
	// ARG(secondCoord IN):          новая 2-ая координата диапазона, в котором должен находиться контрол
	// ARG(pFirstCtrlCoord IN/OUT):  IN  - смещение от старой 1-ой координаты до контрола,
	//                               OUT - новая 1-ая координата контрола
	// ARG(pSecondCtrlCoord IN/OUT): IN  - смещение от старой 2-ой координаты до контрола,
	//                               OUT - новая 2-ая координата контрола
	// ARG(ctrlSize IN):             размер контрола
	// ARG(recalcParam IN):          параметр пересчета: 1 - сдвинуть к 1-ой координате, 2 - сдвинуть к 2-ой координате,
	//   3 - разместить в той же пропорции от краев диапазона, 4 - растянуть на весь диапазон
	//
	void   RecalcCtrlCoords(long firstCoord, long secondCoord, long * pFirstCtrlCoord, long * pSecondCtrlCoord, long ctrlSize, int recalcParam);
	int    Helper_ToRecalcCtrlSet(const RECT * pNewDlgRect, const ResizeParamEntry & rCtrlParam, TSVector <ResizeParamEntry> * pCoordAry, LongArray * pCalcedCtrlAry, int isXDim);
	int    Helper_ToResizeDlg(const RECT * pNewDlgRect);
	//void   SetupCalendar(uint calCtlID, uint inputCtlID, int kind);
	//
	TRect  InitRect;
	RECT   ResizedRect;  // @todo RECT-->TRect
	RECT   ToResizeRect; // @todo RECT-->TRect
public:
	long   resourceID;
	int    DefInputLine;
};
//
// Descr: Вспомогательный класс, реализующий высокоуровневый механизм работы с инлайновым списком выбора
//
class Helper_WordSelector {
protected:
	Helper_WordSelector(WordSel_ExtraBlock * pBlk, uint inputCtlId);
	~Helper_WordSelector();
	bool   IsWsVisible() const;

	uint   InputCtlId;
	WordSel_ExtraBlock * P_OuterWordSelBlk; // @notowner
	WordSelector * P_WordSel;
};

class TInputLine : public TView, private Helper_WordSelector {
public:
	static constexpr float DefHeight      = 21.0f; // Высота поля ввода в пикселях по умолчанию
	static constexpr float DefLabelHeight = 13.0f; // Высота этикетки в пикселях по умолчанию

	static LRESULT CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	enum {
		spcfReadOnly   = 0x0001,
		spcfMultiline  = 0x0002,
		spcfWantReturn = 0x0004
	};

	TInputLine(const TRect & rBounds, uint spcFlags, TYPEID aType, long fmt);
	~TInputLine();
	virtual int    TransmitData(int dir, void * pData);
	virtual void   setState(uint aState, bool enable);
	virtual int    handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	uint   GetSpcFlags() const { return SpcFlags; }
	//
	// Descr: Устанавливает максимальную длину текста поля ввода. 
	// Returns: 
	//   true - success
	//   false - fail (аргумент maxTextLen не должен превышать UINT16_MAX)
	//
	bool   SetupMaxTextLen(uint maxTextLen);
	void   setupCombo(ComboBox *);
	void   setupFreeTextWordSelector(WordSel_ExtraBlock * pBlk);
	void   setFormat(long fmt);
	long   getFormat() const { return Format; }
	void   setType(TYPEID typ);
	TYPEID getType() const { return Type; }
	const char * getText();
	size_t getMaxLen() const { return MaxLen; }
	void   setMaxLen(int newMaxLen);
	size_t getCaret();
	void   setCaret(size_t);
	void   getText(SString & rBuf) const;
	void   setText(const char *);
	void   disableDeleteSelection(int _disable);
	void   selectAll(int enable);
	//
	// Descr: Устанавливает в поле ввода текст, соответствующий диапазону дат pData.
	//   Должен корректно работать с полями типа S_ZSTRING и S_DATERANGE.
	// Returns:
	//   true - success
	//   false - error
	//
	bool   SetDateRange(const DateRange * pData);
	//
	// Descr: Получает из поля ввода значение диапазона дат pData.
	//   Должен корректно работать с полями типа S_ZSTRING и S_DATERANGE.
	// Returns:
	//   true - success
	//   false - error
	//
	bool   GetDateRange(long strtoperiodFlags, DateRange * pData);

	struct Statistics {
		enum {
			fSerialized = 0x0001,
			fPaste      = 0x0002
		};
		int    SymbCount;
		long   Flags;
		double IntervalMean;
		double IntervalStdDev;
	};

	int    GetStatistics(Statistics * pStat) const;
	ComboBox * GetCombo();

	static LPCTSTR WndClsName;
protected:
	DECL_HANDLE_EVENT;
	int    Implement_GetText();
	void   Implement_Draw();

	uint   SpcFlags; // spcfXXX // @v12.3.3 ushort flags-->uint ButtonFlags
	uint32 MaxLen;
	TYPEID Type;
	long   Format;
	enum {
		stValidStr      = 0x0001,
		stDisableDelSel = 0x0002,
		stPaste         = 0x0004, // Текущий текст в поле включает в себя данные, введенные копированием из буфера
		stSerialized    = 0x0008  // Текущий текст был введен "символ-за-символом"
	};
	long   InlSt;
	//
	// Descr: Статистика ввода символов
	//
	struct InputStat {
		InputStat();
		InputStat & Z();
		void   CheckIn();

		clock_t Last;
		double TmSum;
		double TmSqSum;
	};
	InputStat Stat;
	SString Data;
	ComboBox * P_Combo;
private:
	void   Init();
	void   Setup(void * pThisHandle, void * pParentHandle);
	int    OnMouseWheel(int delta);
	int    OnPaste();
};

class TCalcInputLine : public TInputLine {
public:
	TCalcInputLine(uint virtButtonId, uint buttonCtrlId, const TRect & rBounds, TYPEID aType, long fmt);
	~TCalcInputLine();
private:
	struct VirtButtonWndEx {
		explicit VirtButtonWndEx(const char * pSignature);

		char    Signature[24]; // @anchor - должен находится строго в начале структуры
		TDialog * P_Dlg;
		uint   FieldCtrlId;
		uint   ButtonCtrlId;
		WNDPROC PrevWndProc;
		HBITMAP HBmp;
	};
	static  LRESULT CALLBACK InLnCalcWindProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual int    handleWindowsMessage(UINT, WPARAM, LPARAM);
	DECL_HANDLE_EVENT;
	VirtButtonWndEx Vbwe;
	uint   VirtButtonId;
};

class TImageView : public TView {
public:
	TImageView(const TRect & rBounds, const char * pFigSymb);
	~TImageView();
	virtual int    TransmitData(int dir, void * pData);
	const SString & GetFigSymb() const { return FigSymb; } // @v11.0.6
	//
	// Descr: Передает экземпляру this созданную во-вне фигуру pFig.
	//   Объект this получает pFig в собственность - то есть, вызывающий код не должен пытаться разрушить pFig 
	//   после вызова SetOuterFigure().
	//   Если this до вызова SetOuterFigure() содержал собственный ненулевой экземпляр фигуры, то он будет разрушен.
	//   Вызов SetOuterFigure с нулевым аргументом просто приведет к разрушению внутреннего объекта фигуры.
	//
	void   SetOuterFigure(SDrawFigure * pFig); // @v11.1.5
private:
	DECL_HANDLE_EVENT;
	static LRESULT CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual int    handleWindowsMessage(UINT, WPARAM, LPARAM);
	SDrawFigure * P_Fig;
	SString FigSymb;      // Символ векторной фигуры для отображения //
	SColor ReplacedColor; // Замещаемый цвет в векторном изображении
};

class TButton : public TView {
public:
	enum {
		spcfDefault   = 0x0001,
		spcfLeftJust  = 0x0002,
		spcfBroadcast = 0x0004
	};
	TButton(const TRect & rBounds, const char * pTitle, uint command, uint spcFlags, uint bmpID = 0);
	~TButton();
	virtual int    handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual int    TransmitData(int dir, void * pData);
	virtual void   setState(uint aState, bool enable);
	void   MakeDefault(bool enable, bool sendMsg = false);
	const  SString & GetText() const { return Text; }
	void   SetText(const char * pText);
	HBITMAP GetBitmap() const;
	uint   GetBmpID() const;
	int    LoadBitmap_(uint bmpID);
	int    SetBitmap(uint bmpID);
	uint   GetCommand() const;
	bool   IsDefault() const;
	bool   SetSupplementFactors(uint role/*SUiCtrlSupplement::kXXX*/, uint linkCtrlId);
private:
	DECL_HANDLE_EVENT;
	//
	// Descr: Функция реализует ответ на нажатие кнопки.
	//   Вызывается из IMPL_HANDLE_EVENT(TButton) и TButton::handleWindowsMessage().
	//
	void   Press();

	uint   Command;  // @v12.3.3 ushort command-->uint Command
	uint   SpcFlags; // spcfXXX // @v12.3.3 ushort flags-->uint ButtonFlags
	uint   SupplementRole;       // @v12.3.7 SUiCtrlSupplement::kXXX Если кнопка является вспомогательным дополнением к другому управляющему элементу,
		// то в этом поле указана роль данной кнопки.
	uint   SupplementLinkCtrlId; // @v12.3.7 Идентификатор управляющего элемента для которого эта кнопка является вспомогательной.
	uint   BmpID;
	HBITMAP HBmp_;
	SString Text;
};
//
//
//
class TGroupBox : public TView { // @v12.2.3 Просто фрейм
public:
	explicit TGroupBox(const TRect & rBounds);
	~TGroupBox();
	virtual int    handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void   SetText(const char * pText);
	const  SString & GetText() const;
private:
	DECL_HANDLE_EVENT;
	SString Text;
};
//
//
//
#define RADIOBUTTONS 1
#define CHECKBOXES   2

class TCluster : public TView {
public:
	static constexpr float DefItemHeight       = 16.0f; // Высота одного элемента кластера в пикселях по умолчанию
	static constexpr float DefItemVerticalGap  = 2.0;  // Расстояние по вертикали между элементами (от нижней границы верхнего до верхней нижнего). При вертикальной раскладке.
	static constexpr float DefClusterPaddigTop = 16.0f; // Расстояние по вертикали от верхней границы кластера до верхней кромки первого элемента. 
	static constexpr float DefClusterPaddigBottom = 8.0f; // Расстояние по вертикали от нижней границы кластера до нижней кромки последнего элемента.
	static constexpr float DefClusterPaddigLeft = 8.0f; // Расстояние по горизонтали от левой границы кластера до левой кромки первого элемента. 

	struct Item {
		Item() : Flags(0), AssociatedValue(0)
		{
		}
		enum {
			fDefaultRadioButton = 0x0001 // Если установлен, то этот элемент radio-cluster'а является default'нтым
		};
		TRect  Bounds;
		uint   Flags;
		long   AssociatedValue; // Значение, сопоставленной клиентским вызовом с этим элементом кластера.
		SString Text;
	};
	enum {
		spcfSingleItemWithoutFrame = 0x0001 // Специальная опция, идентифицирующая кластер как одиночный checkbox без рамки.
			// Используется при динамическом формировании окна по внешнему описанию.
	};
	//
	// ARG(kind IN): Вид кластера. Либо RADIOBUTTONS, либо CHECKBOXES
	//
	TCluster(const TRect & rBounds, int kind, uint spcFlags, const char * pTitle, const StringSet * pStrings);
	// @todo TCluster(const TRect & rBounds, int kind, uint spcFlags, const TSCollection <Item> & rItemList);
	~TCluster();
	virtual int    TransmitData(int dir, void * pData);
	virtual void   setState(uint aState, bool enable);
	const  SString & GetRawText() const { return Text; }
	bool   mark(int item);
	void   press(ushort item);
	uint   getNumItems() const;
	int    GetText(int pos, SString & rBuf);
	int    SetText(int pos, const char * pText);
	void   AddItem(int item, const char * pText, const TRect * pRect);
	void   deleteItem(int);
	void   disableItem(int pos /* 0.. */, bool disable);
	bool   IsItemEnabled(int item) const; // item = номер элемента в списке 0..
	void   deleteAll();
	bool   IsChecked(uint itemIdx) const;  // item = (ushort)GetWindowLong(hWnd, GWL_ID);
	bool   IsEnabled(uint itemIdx) const;  // item = (ushort)GetWindowLong(hWnd, GWL_ID);
	uint   GetItemId(uint itemIdx) const;
	HWND   getItemHandle(uint itemIdx);
	//
	// Три функции для ассоциирования элементов кластера с прикладными значениями.
	// pos == -1 соответствует значению по умолчанию.
	//
	int    addAssoc(long pos, long val);
	int    setDataAssoc(long);
	bool   getDataAssoc(long *);
	int    getItemByAssoc(long val, int * pItem) const;
	int    GetKind() const { return static_cast<int>(Kind); } // @v12.3.3 getKind()-->GetKind()
	uint   GetSpcFlags() const { return SpcFlags; }
	const  Item * GetItemC(uint idx) const { return (idx < ItemList.getCount()) ? ItemList.at(idx) : 0; }
protected:
	DECL_HANDLE_EVENT; // @v12.3.5
	virtual int    handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	const  int32 Kind;  // RADIOBUTTONS || CHECKBOXES
	uint   SpcFlags; // @v12.3.3
	uint16 Value;   // Это поле надо сделать типа uint32 но для этого необходимо аккуратно поменять все вызовы TCluster::TransmitData
	uint16 Reserve; // @alignment
	int    Sel;   // Выбранный элемент кластера (RADIOBUTTONS)
	uint32 DisableMask; // @v11.6.2 int-->uint32
	// @v12.3.3 SStrCollection Strings;
	TSCollection <Item> ItemList; // @v12.3.3
private:
	void     ArrangeItems(int direction, bool tuneOnInit);
	//int    column(int item) const;
	//int    row(int item) const;
	LAssocArray ValAssoc; // @todo Перебросить эти данные в ItemList
	SString Text; // @v12.3.5 Текст заголовка для кластера. Если кластер состоит из единственного checkbox'а без фрейма, то этот заголовок не работает (фрейма нет)
};

class TStaticText : public TView {
public:
	enum {
		spcfStaticEdge = 0x0001 // Элемент обрамлен явно выраженной рамкой
	};
	TStaticText(const TRect & rBounds, uint spcFlags, const char * pText);
	const SString & GetRawText() const { return Text; }
	//
	// Descr: Возвращает текст, ассоциированный с native-элементом,
	//   соответствующим экземпляру. То есть, не this->Text, а
	//   извлекает строку из соответсвующего native-объекта
	//   (TView::SGetWindowText(GetDlgItem(Parent, Id), rBuf))
	//
	SString & GetText(SString & rBuf) const;
	int    SetText(const char *);
	uint   GetSpcFlags() const { return SpcFlags; }
protected:
	DECL_HANDLE_EVENT; // @v12.2.5
	virtual int handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	uint   SpcFlags; // @v12.3.3
	SString Text;
};

class TLabel : public TStaticText {
public:
	TLabel(const TRect & rBounds, const char * pText, TView * pLink);
protected:
	DECL_HANDLE_EVENT;
	TView * P_Link;
};

ushort SMessageBox(const char * pMsg, ushort options);

#define CLUSTER_ID(x) ((x)&4095)
#define BUTTON_ID(x)  ((x)>>12)
#define MAKE_BUTTON_ID(id,bid) ((id)+((bid)<<12))

#define DEFAULT_MAX_LEN  128
#define DEFAULT_CBX_CHAR 31      // Thick down arrow
#define DEFAULT_CBX_KEY  kbDown
//
// Дополнительный флаг поиска (см. srchXXX constants in SArray.H)
//
#define lbSrchByID  0x0800
//
// Descr: Абстрактный источник данных для списков
//
class ListBoxDef {
public:
	//
	// Capability flags
	//
	enum {
		cTree      = 0x0001, // tree view
		cCountable = 0x0002, // Класс может вернуть актуальное количество записей
		cFullInMem = 0x0004  // Данные полностью содержатся в памяти: можно относительно быстро просмотреть все записи списка.
	};

	ListBoxDef(uint aOptions, TYPEID aType);
	virtual ~ListBoxDef();
	virtual void   setViewHight(int vh);
	virtual void   getScrollData(long * pScrollDelta, long * pScrollPos);
	virtual int    getCurID(long * pId);
	inline  int    getCurID(int * pId) { return getCurID(reinterpret_cast<long *>(pId)); } // @v11.7.10
	virtual int    getCurString(SString & rBuf);
	virtual int    getCurData(void *);
	virtual int    search(const void *, CompFunc, int srchMode);
	virtual bool   IsValid() const;
	virtual int    go(long);
	virtual int    step(long);
	virtual int    top();
	virtual int    bottom();
	virtual long   GetRecsCount() const;
	virtual int    getIdList(LongArray &);
	virtual void * FASTCALL getRow_(long idx);
	//
	// Descr:
	//   (dir > 0): В зависимости от флагов options по адресу pData возвращает либо ID либо данные.
	//   (dir < 0): Пытается, используя ListBoxDef::search(), в соответствии с
	//     options найти в списке требуемые данные. Если поиск удался,
	//     то возвращает 1, в противном случае - 0.
	//
	//
	virtual int    TransmitData(int dir, void * pData);
	virtual int    refresh();
	virtual int    addItem(long id, const char *, long * pPos = 0);
	virtual int    removeItem(long pos);
	virtual void   freeAll();
	virtual int    GetFrameSize() const;
	virtual int    GetFrameState();
	const  char * getText(long item, SString & rBuf);
	long   _topItem() const;
	long   _curItem() const;
	bool   _isTreeList() const { return LOGIC(CFlags & cTree); }
	bool   _isSolid() const { return LOGIC(CFlags & cFullInMem); }
	void   SetOption(uint option, int set = 1);
	int    SetUserData(const void * pData, size_t size);
	int    GetUserData(void * pData, size_t * pSize) const;
	bool   HasCapability(long c) const { return LOGIC(CFlags & c); }
	int    GetImageIdxByID(long id, long * pIDx);
	void * CreateImageList(HINSTANCE hInst); // @v10.9.4 HIMAGELIST-->(void *)
	//
	// Descr: Устанавливает ассоциацию элемента списка, имеющего идентификатор itemID, с иконкой,
	//   идентифицируемой как imageID.
	//   Значение imageID должно быть либо нулевым, либо ссылаться на ресурс иконок ICON_XXX.
	//
	int    AddImageAssoc(long itemID, long imageID);
	//
	// Descr: Устанавливает ассоциацию элемента списка, имеющего идентификатор itemID, с векторной иконкой,
	//   идентифицируемой как imageID.
	//   Значение imageID должно быть либо нулевым, либо ссылаться на ресурс векторных изображений PPDV_XXX.
	// Note: Идентификатор imageID хранится в списке ImageAssoc с дополнительным битом 0x40000000 чтобы
	//   можно было отличить векторную иконку от растровой.
	//
	int    AddVecImageAssoc(long itemID, long imageID);
	void   ClearImageAssocList();
	int    SetItemColor(long itemID, SColor fgColor, SColor bckgColor);
	int    ResetItemColor(long itemID);
	bool   HasItemColorSpec() const;
	int    GetItemColor(long itemID, SColor * pFgColor, SColor * pBckgColor) const;
	long   GetCapability() const { return CFlags; }
	uint32 GetSignature() const { return Sign; }
	StrAssocArray * GetListByPattern(const char * pText);
//protected:
	uint   Options;
	int    ViewHight;
protected:
	//
	// Descr: Функция устанавливает параметры особенностей порожденного класса. Должны вызывать только из конструктора.
	//
	void   SetCapability(long cflags, uint32 sign);
	long   Format;
	long   ScrollDelta;
	TYPEID Type;
	long   TopItem;
	long   CurItem;
private:
	uint32 Sign;               // Подпись экземпляра класса. Используется для идентификации порожденных классов и инвалидных экземпляров
	long   CFlags;
	SBaseBuffer UserData;      //
	LAssocArray ImageAssoc;    // Список ид элементов и ассоциированных с ним id иконок
	LAssocArray ImageIdxAssoc; // Список ид элементов и ассоциированных с ним индексов картинок содержащихся в HIMAGELIST
	struct ColorItem {
		long   Id; // ИД элемента
		SColor F;  // Цвет символов
		SColor B;  // Цвет фона
	};
	TSVector <ColorItem> ColorAssoc;
};

class StdListBoxDef : public ListBoxDef {
public:
	StdListBoxDef(SArray * pArray, uint aOptions, TYPEID);
	~StdListBoxDef();
	virtual int    search(const void *, CompFunc, int srchMode);
	virtual bool   IsValid() const;
	virtual long   GetRecsCount() const;
	virtual int    getIdList(LongArray & rList);
	virtual void * FASTCALL getRow_(long r);
	virtual int    GetFrameSize() const;
	virtual int    GetFrameState();
	int    setArray(SArray *);
//protected:
	SArray * P_Data;
};

class StrAssocListBoxDef : public ListBoxDef {
public:
	StrAssocListBoxDef(StrAssocArray *, uint options);
	~StrAssocListBoxDef();
	virtual int    search(const void *, CompFunc, int srchMode);
	virtual bool   IsValid() const;
	virtual long   GetRecsCount() const;
	virtual int    getIdList(LongArray & rList);
	virtual void * FASTCALL getRow_(long r);
	virtual int    GetFrameSize() const;
	virtual int    GetFrameState();
	virtual int    addItem(long id, const char *, long * pPos = 0);
	virtual int    removeItem(long pos);
	virtual void   freeAll();
	int    setArray(StrAssocArray *);
	const StrAssocArray * getArray() const { return P_Data; }
protected:
	StrAssocArray * P_Data;
private:
	SBaseBuffer OneItem; // Временный буфер, возвращаемый функцией getRow_()
};

class StdTreeListBoxDef : public ListBoxDef {
public:
	friend class SmartListBox;

	StdTreeListBoxDef(StrAssocArray * pList, uint aOptions, TYPEID);
	~StdTreeListBoxDef();
	virtual void   setViewHight(int);
	virtual void   getScrollData(long * pScrollDelta, long * pScrollPos);
	virtual bool   IsValid() const;
	virtual int    go(long);
	virtual int    step(long);
	virtual int    top();
	virtual int    bottom();
	virtual long   GetRecsCount() const;
	virtual int    getIdList(LongArray & rList);
	virtual void * FASTCALL getRow_(long);
	virtual int    getCurString(SString & rBuf);
	virtual int    getCurID(long *);
	virtual int    getCurData(void *);
	virtual int    search(const void * pPattern, CompFunc fcmp, int srchMode);
	virtual int    GetFrameSize() const;
	virtual int    GetFrameState();
	int    setArray(StrAssocArray *);
	int    GetStringByID(long id, SString & rBuf);
	int    GoByID(long id);
	bool   FASTCALL HasChildren(long id) const;
	int    GetListByParent(long parentId, LongArray & rList) const;
	int    GetParent(long child, long * pParent) const;
	//int    GetChildList(long parentId, LongArray * pChildList);
protected:
	//void   setupView();
	int    Helper_CreateTree();
	int    Helper_AddTreeItem(uint idx, UintHashTable & rAddedIdxList, uint32 * pPos);
private:
	StrAssocArray * P_SaList;
	struct TreeItem {
		long   Id;
		long   ParentId;
		void * H;
		uint   P;
	};
	STree  T;
	struct Item {
		long   Id;
		long   ParentId;
		char   Txt[256];
	};
	Item   TempItem;
};

class StdTreeListBoxDef2_ : public ListBoxDef {
public:
	friend class SmartListBox;

	StdTreeListBoxDef2_(StrAssocTree * pList, uint aOptions, TYPEID);
	~StdTreeListBoxDef2_();
	virtual void   setViewHight(int);
	virtual void   getScrollData(long * pScrollDelta, long * pScrollPos);
	virtual bool   IsValid() const;
	virtual int    go(long);
	virtual int    step(long);
	virtual int    top();
	virtual int    bottom();
	virtual long   GetRecsCount() const;
	virtual int    getIdList(LongArray & rList);
	virtual void * FASTCALL getRow_(long);
	virtual int    getCurString(SString & rBuf);
	virtual int    getCurID(long *);
	virtual int    getCurData(void *);
	virtual int    search(const void * pPattern, CompFunc fcmp, int srchMode);
	virtual int    GetFrameSize() const;
	virtual int    GetFrameState();
	const  StrAssocTree * GetData() const { return P_SaList; }
	int    AddTopLevelRestrictionId(long id); // @v11.4.0
	//
	// Descr: Возвращает true если идентификатор id следует отображать среди узлов верхнего уровня.
	//
	bool   BelongToTopLevelResriction(long id) const; // @v11.4.0
	int    setArray(StrAssocTree *);
	int    GetStringByID(long id, SString & rBuf);
	int    GoByID(long id);
	bool   FASTCALL HasChildren(long id) const;
	int    GetListByParent(long parentId, LongArray & rList) const;
	int    GetParent(long child, long * pParent) const;
	//int    GetChildList(long parentId, LongArray * pChildList);
protected:
	//void   setupView();
	//int    Helper_CreateTree();
	//int    Helper_AddTreeItem(uint idx, UintHashTable & rAddedIdxList, uint32 * pPos);
private:
	StrAssocTree * P_SaList;
	struct TreeItem {
		long   Id;
		long   ParentId;
		void * H;
		uint   P;
	};
	//STree  T;
	struct Item {
		long   Id;
		long   ParentId;
		char   Txt[256];
	};
	Item   TempItem;
	LongArray * P_TopLevelRestrictionIdList; // @v11.4.0 Список идентификаторов узлов верхнего уровня, которыми следует ограничить отображение.
		// Эти ограничения касаются только верхнего уровня отображения. Если включенные идентификаторы встречаются и внутри дерева, то они отображаются.
	LongArray LL; // Линеаризованный список идентификаторов P_SaList используемый для базовых функций навигации.
	LAssocArray StrIndex; // @v11.2.11 Индекс соответствия идентификаторов элементов позициям строк в P_SaList
	struct SearchBlock { // @v11.2.11 
		SearchBlock();
		~SearchBlock();
		SearchBlock & Z();
		LongArray * P_LastResult; // Список идентификаторов, найденных по запросу текстового поиска. Нужен для поиска последующих позиций.
		SString LastPattern; // Последний образец, использованный для поиска. Важен для определения необходимости повторного сканирования дерева.
	};
	SearchBlock Sb;
};

class DBQListBoxDef : public ListBoxDef {
public:
	DBQListBoxDef(DBQuery & rQuery, uint aOptions, uint aBufSize = 64);
	~DBQListBoxDef();
	virtual void   setViewHight(int);
	virtual void   getScrollData(long * pScrollDelta, long * pScrollPos);
	virtual bool   IsValid() const;
	virtual int    go(long);
	virtual int    step(long);
	virtual int    top();
	virtual int    bottom();
	virtual long   GetRecsCount() const;
	virtual int    getIdList(LongArray & rList);
	virtual void * FASTCALL getRow_(long);
	virtual int    TransmitData(int dir, void * pData);
	virtual int    refresh();
	virtual int    search(const void * pPattern, CompFunc fcmp, int srchMode);
	virtual int    GetFrameSize() const;
	virtual int    GetFrameState();
	int    setQuery(DBQuery & rQuery, uint aBufSize = 32);
	int    setRestrict(DBQ & rQ);
protected:
	void   setupView();
	DBQuery * query;
};

class UiSearchTextBlock : Helper_WordSelector {
public:
	static int ExecDialog(HWND hWnd, uint ctlId, SString & rText, int isFirstLetter, WordSel_ExtraBlock * pBlk, int linkToList);
private:
	static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK InputCtlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	UiSearchTextBlock(HWND h, uint ctlId, char * pText, int firstLetter, WordSel_ExtraBlock * pBlk, int linkToList);
	~UiSearchTextBlock();

	HWND   H_Wnd; // SmartListBox window handle
	uint   Id;    // SmartListBox control id
	int    IsBnClicked;
	int    LinkToList;  // Данный блок будет прилинкован непосредственно к списку, в некоторых случаях нужно для корректного отображения и фокусировки.
	int    FirstLetter; // Первый символ, по которому был вызван диалог, следует отправить в окно ввода посредством эмуляции нажатия клавиши.
	//WordSel_ExtraBlock * P_WordSelBlk; // not owner
	//WordSelector * P_WordSel; //
	WNDPROC PrevInputCtlProc;
	SString Text;
};
//
//
//
extern const char * SLBColumnDelim; // "/^"

class SmartListBox : public TView {
	friend class ComboBox;
	friend class ListWindow;
public:
	//
	// Descr: Флаги состояний объекта
	//   Для доступа к состоянию используйте метод HasState(SmartListBox::stXXX)
	//
	enum {
		stTreeList           = 0x0001,  // Древовидный список
		stOwnerDraw          = 0x0002,  // Диалог-владелец списка сам реализует функцию перерисовки
		stDataFounded        = 0x0004,  // Признак того, что (def->setData() != 0)
		stLButtonDown        = 0x0008,  // Левая кнопка мыши нажата на списке
		stInited             = 0x0010,  // Выставляется функцией SmartListBox::onInit.
		stLBIsLinkedUISrchTextBlock  = 0x0020,  // Окно поиска будет прилинковано непосредственно к списку. При уничтожении фокус будет попадать на список.
		stOmitSearchByFirstChar      = 0x0040,  // Не обрабатывать ввод символа как сигнал к поиску
	};
	static bool IsValidS(const SmartListBox * pThis) { return (pThis && pThis->P_Def); }
	SmartListBox(const TRect & rRect, ListBoxDef * pDef, bool isTree);
	SmartListBox(const TRect & rRect, ListBoxDef * pDef, const char * pColumnsDeclaration);
	~SmartListBox();
	void   FASTCALL setDef(ListBoxDef * pDef);
	bool   Search_(const void * pattern, CompFunc fcmp, int srchMode);
	int    FASTCALL getCurID(long * pId);
	int    FASTCALL getCurID(int * pId) { return getCurID(reinterpret_cast<long *>(pId)); } // @v11.7.10
	int    FASTCALL getCurData(void * pData);
	int    FASTCALL getCurString(SString & rBuf);
	uint   GetSelectionList(LongArray * pList);
	int    getText(long itemN  /* 0.. */, SString & rBuf);
	int    getID(long itemN, long * pID);
	bool   IsTreeList() const { return (P_Def && P_Def->_isTreeList()); }
	bool   IsMultiColumn() const { return (Columns.getCount() > 0); }
	DECL_HANDLE_EVENT;
	virtual void   selectItem(long item);
	virtual int    TransmitData(int dir, void * pData);
	virtual void   setState(uint aState, bool enable);
	virtual int    handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	//
	// Descr: Вставляет колонку в многоколоночный список.
	// ARG(pos    IN): Позиция, в которой должна быть вставлена колонка.
	//   Если (pos < 0), то колонка вставляется в конец списка.
	// ARG(pTitle IN): @#{vptr0} Текст заголовка колонки. Если указатель
	//   нулевой, но заголовок будет пустым.
	// ARG(width  IN): Ширина колонки в символах (не в точках). Если
	//   width == 0, то ширина становится равной длине строки pTitle или
	//   1, если pTitle == 0.
	//   Если widht > 255, то ширина колонки устанавливается равной 255.
	// ARG(format IN): Параметр выравнивания колонки. Должно быть
	//   одно из следующих значений: 0, ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER.
	//   Если флаги ALIGN_XXX в format не указаны, то колонка выравнивается по левому краю.
	//
	//   Если в поле format установлен флаг STRF_OEM, то функция считает, что
	//   строка pTitle передана в OEM-кодировке.
	//   Если в поле format установлен флаг STRF_ANSI, то функция считает, что
	//   строка pTitle передана в ANSI-кодировке.
	//   Если format не содержит ни флага STRF_OEM, ни флага STRF_ANSI, либо оба эти флага,
	//   то функция считает, что строка передана в OEM-кодировке.
	//
	// Returns:
	//   !0 - Колонка добавлена
	//   0  - Ошибка
	//
	int    AddColumn(int pos, const char * pTitle, uint width, uint format, long ident);
	int    SearchColumnByIdent(long ident, uint * pPos) const;
	int    RemoveColumn(int pos);
	void   RemoveColumns();
	uint   GetColumnsCount() const { return Columns.getCount(); }
	int    SetupColumns(const char * pColsBuf);
	bool   GetOrgColumnsDescr(SString & rBuf) const;
	void   setHorzRange(int);
	void   setRange(long aRange);
	void   search(char * firstLetter, int srchMode);
	void   setCompFunc(CompFunc f) { SrchFunc = f; }
	int    addItem(long id, const char * s, long * pPos = 0);
	int    removeItem(long pos);
	void   freeAll();
	void   FASTCALL focusItem(long item);
	void   Scroll(short sbCmd, int value);
	void   CreateScrollBar(int create);
	void   SetScrollBarPos(long pos, LPARAM lParam);
	//
	// Descr: Устанавливает или снимает состояние списка stTreeList.
	// Returns:
	//   1 Состояние установлено
	//   0 Состояние снято
	//
	bool   SetTreeListState(bool yes);
	void   SetOwnerDrawState();
	void   SetLBLnkToUISrchState();
	//
	// Descr: Устанавливает состояние stOmitSearchByFirstChar препятствующее
	//   появлению окна поиска в ответ на ввод символьной клавиши.
	//
	void   SetOmitSearchByFirstChar();
	bool   HasState(long s) const;
	//
	// Перемещает окно Scrollbar в соответствии со списком. При этом старое окно Scrollbar разрушается и создается новое
	//
	void   MoveScrollBar(int autoHeight);

	ListBoxDef * P_Def; // @todo must be private
protected:
	int    GetStringByID(long id, SString & rBuf);
	int    GetImageIdxByID(long id, long * pIdx);
	void   SelectTreeItem();
	void   onInitDialog(int useScrollbar);
	// @v11.4.4 (inlined) int    FASTCALL onVKeyToItem(WPARAM wParam);
	int    GetMaxListHeight();
	void   Implement_Draw();
private:
	void   Helper_InsertColumn(uint pos);
	void   Helper_ClearTreeWnd();
	int    SetupTreeWnd2(void * pParent);
	//
	// Descr: Варианты автоматического расчета ширины колонок 
	//
	enum {
		auotocalccolszNo = 0, // Нет
		auotocalccolszNominal    = 1, // Пропорционально номинальным значениям ширины (заданным в ресурсе)
		auotocalccolszContent    = 2, // Пропорционально содержимому колонок
		auotocalccolszLogContent = 3, // Пропорционально логарифму содержимого колонок
	};

	void   CalculateColumnsWidth(int variant /*auotocalccolszXXX*/);

	struct ColumnDescr {
		uint   Width;
		uint   Format;   // ALIGN_XXX
		uint   TitlePos; // Позиция строки заголовка в StrPool
		long   Ident;    // Идентификатор столбца. Уникальность не проверяется.
	};
	long   State;
	long   Range;
	long   Top;
	long   ItemHeight; // @v12.2.4 Height-->ItemHeight
	CompFunc SrchFunc;
	uint   SrchPatternPos; // Позиция последнего образца поиска в StrPool
	uint   ColumnsSpcPos;  // Позиция строки спецификации колонок в StrPool
	SArray Columns;
	StringSet StrPool;
	void * HIML;
};

class ListWindowSmartListBox : public SmartListBox {
public:
	ListWindowSmartListBox(const TRect & r, ListBoxDef * pDef, int = 0);
	ComboBox * combo;
};

class ListWindow : public TDialog {
	friend class ComboBox;
public:
	ListWindow();
	explicit ListWindow(ListBoxDef * pDef);

	DECL_HANDLE_EVENT;
	void executeNM(HWND parent); // Немодальный диалог
	//
	// Descr: Если список окна содержит только один элемент, то возвращает
	//   его ИД по адресу pVal.
	// Returns:
	//   !0 - список окна содержит только один элемен, который успешно присвоен по указателю pVal
	//   0  - либо список окна содержит более одного элемента, либо содержит 0 элементов, либо
	//     не удалось получить единственный элемент списка.
	//
	int    getSingle(long * pVal);
	int    FASTCALL getResult(long * pVal);
	int    FASTCALL getResult(int * pVal) { return getResult(reinterpret_cast<long *>(pVal)); }
	int    getString(SString & rBuf);
	int    getListData(void *);
	bool   IsTreeList() const { return (P_Def && P_Def->_isTreeList()); }
	void   FASTCALL setDef(ListBoxDef * pDef);
	void   setCompFunc(CompFunc f);
	ListWindowSmartListBox * listBox() const;
	void   Move_(HWND linkHwnd, long right);
	void   Move_(const RECT & rRect);
	ListBoxDef * getDef() const { return P_Def; }
	void   SetToolbar(uint tbId);
	uint   GetToolbar() const { return TbId; }
protected:
	void   prepareForSearching(int firstLetter);
	ListBoxDef * P_Def;
	ListWindowSmartListBox * P_Lb; // box;
	int    PrepareSearchLetter;
	uint   TbId;
};

class WordSelectorSmartListBox : public ListWindowSmartListBox {
public:
	WordSelectorSmartListBox(const TRect & r, ListBoxDef * pDef);
	virtual int handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class WordSelector : public ListWindow {
public:
	WordSelector(WordSel_ExtraBlock * pBlk);
	void   FASTCALL setDef(ListBoxDef * pDef);
	int    Refresh(const char * pText);
	int    ViewRecent();
	int    Activate();
	void   ActivateInput();
	bool   CheckVisible() const;
	bool   CheckActive() const;
private:
	DECL_HANDLE_EVENT;
	void   DrawListItem2(TDrawItemData * pDrawItem);
	int    Helper_PullDown(const char * pText, int recent);

	enum {
		dummyFirst = 1,
		clrFocus,
		clrOdd,
		clrBkgnd,
	};
	//int    IsVisible_;
	//int    IsActive_;
	enum {
		wssVisible = 0x0001,
		wssActive  = 0x0002
	};
	uint   WsState;
	SPaintToolBox Ptb;
	WordSel_ExtraBlock * P_Blk; // not owner
};

ListWindow * CreateListWindow(DBQuery & rQuery, uint options);
ListWindow * CreateListWindow(SArray * pAry, uint options, TYPEID);
ListWindow * CreateListWindow(StrAssocArray * pAry, uint options);
// @v11.2.5 ListWindow * CreateListWindow(uint sz, uint options);
ListWindow * CreateListWindow_Simple(uint options); // @v11.2.5
// WordSelector * CreateWordSelector(StrAssocArray * pAry, uint optons, UiWordSel_Helper * pHelper);

class ComboBoxInputLine : public TInputLine {
public:
	ComboBoxInputLine(ushort aId);
	virtual int  TransmitData(int dir, void * pData);
};

class ComboBox : public TView {
public:
	static LRESULT CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	ComboBox(const TRect &, ushort aFlags, TInputLine * pCorrespondCtrl);
	ComboBox(const TRect &, ListBoxDef * aDef);
	~ComboBox();
	virtual void   setState(uint aState, bool enable);
	virtual int    TransmitData(int dir, void * pData);
	virtual void   selectItem(long item);
	virtual int    handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	DECL_HANDLE_EVENT;
	void   FASTCALL setDef(ListBoxDef * pDef);
	//
	// Descr: Устанавливает признак ComboBox::Undef.
	//
	void   setUndefTag(int set);
	void   setUndefID(long undefID);
	int    setDataByUndefID();
	int    setListWindow(ListWindow * pListWin);
	int    setListWindow(ListWindow * pListWin, long val);
	ListWindow * getListWindow() const { return P_ListWin; }
	void   setInputLineText(const char *);
	int    getInputLineText(char * pBuf, size_t bufLen);
	ListBoxDef * listDef() const { return P_Def; }
	void   setRange(long aRange);
	void   setupListWindow(int noUpdateSize);
	void   setupTreeListWindow(int noUpdateSize);
	void   search(const char * pFirstLetter, int srchMode);
	int    search(const void * pPattern, CompFunc fcmp, int srchMode);
	int    addItem(long id, const char * pS, long * pPos = 0);
	int    removeItem(long pos);
	void   freeAll();
	TInputLine * GetLink() const;
	void   SetLink(TInputLine * pLink);
private:
	void   Init(long flags);
	void   Scroll(short);

	HWND   hScrollBar;
	long   Range;
	long   Top;
	long   Flags; // cbxXXX
	long   NoDefID; // сохраняется id для комбо бокса загружающего данные только при открытии окошка listbox
	enum {
		stExecSemaphore = 0x0001, // Признак того, что (def->setData() != 0)
		stDataFounded   = 0x0002, // Если P_Def == 0 и в комбо-боксе удаляют данные, то установлен
		stUndef = 0x0004,
		stNoDefZero     = 0x0008
	};
	long   State;
	CompFunc SrchFunc;
	ListBoxDef * P_Def;
	TInputLine * P_ILink;
	ListWindow * P_ListWin;
	SString SearchPattern;
	SString Text;
};
//
// Descr: Структура, передаваемая с сообщением cmDrawItem
//
#define ODT_CHECKBOX (ODT_STATIC+10)
#define ODT_RADIOBTN (ODT_STATIC+11)
#define ODT_EDIT     (ODT_STATIC+12)

struct TDrawItemData {
	uint   CtlType;
	uint   CtlID;
	uint   ItemID;
	enum {
		iaDrawEntire = 0x0001, // =ODA_DRAWENTIRE
		iaSelect     = 0x0002, // =ODA_SELECT
		iaFocus      = 0x0004, // =ODA_FOCUS
		iaBackground = 0x0008  // Зарисовать фон
	};
	uint   ItemAction;
	uint   ItemState;
	HWND   H_Item;
	HDC    H_DC;
	RECT   ItemRect;
	TView * P_View;
	ulong  ItemData;
};
//
// Descr: Структура, передаваемая с сообщением cmCtlColor
//
struct TDrawCtrlData {
	enum {
		cStatic = 1,
		cEdit,
		cScrollBar
	};
	int    Src;   // IN   TDrawCtrlData::cXXX Тип управляющего элемента-источника сообщения.
	HWND   H_Ctl; // IN   Управляющий элемент, который будет перерисован
	HDC    H_DC;  // IN   Контекст рисования управляющего элемента
	HBRUSH H_Br;  // OUT  Кисть, которую передает обрабатывающая сообщение функция //
		// для отрисовки управляющего элемента
};

int SetWindowTransparent(HWND hWnd, int transparent /*0..100*/);

BOOL    CALLBACK ListSearchDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL    CALLBACK PropertySheetDialogProc(HWND, UINT, WPARAM, LPARAM);
//
// Toolbar
// There's a mine born by Osolotkin, 2000
//
#define TOOLBAR_ON_TOP		0
#define TOOLBAR_ON_BOTTOM	1
#define TOOLBAR_ON_LEFT		2
#define TOOLBAR_ON_RIGHT	3
#define TOOLBAR_ON_FREE		4

#define TBS_TEXT			0x0001
#define TBS_MENU			0x0002
#define TBS_AUTOSIZE		0x0004
#define TBS_NOMOVE			0x0008
#define TBS_LIST			0x0010

class TToolbar {
public:
	friend class TuneToolsDialog;

	explicit TToolbar(HWND hw, DWORD style = 0);
	~TToolbar();
	void   DestroyHWND();
	bool   IsValid() const;
	int    GetCurrPos() const;
	HWND   H() const;
	HWND   GetToolbarHWND() const;
	uint   getItemsCount() const;
	const  ToolbarItem & getItem(uint idx/*[0..]*/) const;
	const  ToolbarItem & getItemByID(uint id/*[0..]*/);
	int    Init(uint res = 0, uint type = 0);
	int    Init(const ToolbarList *);
	int    Init(uint cmdID, ToolbarList * pList);
	int    Show();
	void   Hide();
	LRESULT OnMainSize(int rightSpace = 0);
	int    SaveUserSettings(uint typeID);
	int    RestoreUserSettings(uint typeID, ToolbarList * pTbList);
	int    TranslateKeyCode(ushort keyCode, uint * pCmd) const;
private:
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
	static LRESULT CALLBACK ToolbarProc(HWND, UINT, WPARAM, LPARAM);
	static INT_PTR CALLBACK TuneToolsDlgProc(HWND, UINT, WPARAM, LPARAM);
	// Message Callback functions {
	LRESULT OnMoving(WPARAM, LPARAM);
	LRESULT OnLButtonDown(WPARAM, LPARAM);
	LRESULT OnLButtonDblclk(WPARAM, LPARAM);
	LRESULT OnNotify(WPARAM, LPARAM);
	LRESULT OnCommand(WPARAM, LPARAM);
	LRESULT OnSize(WPARAM, LPARAM);
	LRESULT OnMove(WPARAM, LPARAM);
	// }
	int    SetupToolbarWnd(DWORD style, const ToolbarList *);
	int    SetStyle(DWORD style = 0, int copy = 0);
	int    SelectMode();
	int    GetRegTbParam(uint typeID, char * pBuf, size_t bufLen);

	HWND   H_Wnd;
	HWND   H_Toolbar;
	HWND   H_MainWnd;
	HMENU  H_Menu;
	uint   VisibleCount;
	DWORD  Style;
	ToolbarList Items;
	long   Width;
	long   Height;
	// For movin'n'resizin'
	RECT   ClientRect;
	RECT   CurrRect;
	POINTS MousePoint;
	int    CurrPos;
	WNDPROC PrevToolProc;
};

#define WM_USER_KEYDOWN (WM_USER + 102)
//
//
//
//SchemaID 1(default scheme), 2 etc.
//Title color
//TitleDelim color
//Background color
//Txt color
//Cursor color
//CursorOverText color
//LineCursor color
//LineCursorOverText color
//Grid Horz color
//Grid Vert color

struct BrowserColorsSchema {   // size=42
	uint   SchemaID; // 1, 2 etc.
	COLORREF Title;
	COLORREF TitleDelim;
	COLORREF Background;
	COLORREF Text;
	COLORREF Cursor;
	COLORREF CursorOverText;
	COLORREF LineCursor;
	COLORREF LineCursorOverText;
	COLORREF GridHorizontal;
	COLORREF GridVertical;
};

#define NUMBRWCOLORSCHEMA        3
extern const BrowserColorsSchema BrwColorsSchemas[NUMBRWCOLORSCHEMA]; // @global

#define UISETTINGS_VERSION_MAJOR  1
#define UISETTINGS_VERSION_MINOR 10 // @v11.5.11 9-->10
#define TOOLBAR_OFFS 100000L

class UserInterfaceSettings { // @persistent @store(WinReg[HKCU\Software\Papyrus\UI]) @size=256
public:
	static const char * SubKey;  // "Software\\Papyrus\\UI";
	enum {
		fDontExitBrowserByEsc     = 0x00000001,
		fShowShortcuts            = 0x00000002,
		fAddToBasketItemCurBrwItemAsQtty = 0x00000004, // При добавлении в корзину новой позиции из отчета в качестве начального количества использовать текущую ячейку в активном броузере
		fShowBizScoreOnDesktop    = 0x00000008, // Отображать бизнес показатели на рабочем столе
		fDisableNotFoundWindow    = 0x00000010, // Не отображать окно "Не найдено" при поиске в таблицах
		fUpdateReminder           = 0x00000020, // Отображать напоминание об имеющихся обновлениях программы
		fTcbInterlaced            = 0x00000040, // Горизонтальные полосы временной диаграммы отображать с черезстрочным изменением цвета. В противном случае = отделять строки линиями.
		fShowLeftTree             = 0x00000080, // Показывать древовидную навигацию в левой части окна
		// @v10.9.3 fShowObjectsInLeftWindow = 0x00000100, // @unused @v8.x.x Показывать диалоги редактирования списка объектов в левой части окна
		fDisableBeep              = 0x00000200, // Запретить звуковые сигналы (ограниченная реализация)
		fBasketItemFocusPckg      = 0x00000400, // При вводе нового элемента товарной корзины фокус ввода устанавливать на
			// количество упаковок (а не единиц, как по умолчанию).
		fOldModifSignSelection    = 0x00000800, // Использовать технику выбора знака для строки документа модификации
			// товара, применявшуюся до v8.4.12 (выбор товара - выбор знака)
		fPollVoipService          = 0x00001000, // Опрашивать VoIP сервис для обработки событий вызовов и звонков
		fExtGoodsSelMainName      = 0x00002000, // В списке расширенного выбора товара всегда показывать полные наименования товаров
			// Эта опция потенциально способно ускорить выборку поскольку не будет вынуждать программу лишний раз обращаться к записи товара
			// когда сокращенное наименование не совпадает с полным (see PPObjGoods::_Selector2()).
		fEnalbeBillMultiPrint     = 0x00004000, // Локальная установка флага PPBillConfig::Flags BCF_ALLOWMULTIPRINT
		fDisableBillMultiPrint    = 0x00008000, // Локальное отключение флага PPBillConfig::Flags BCF_ALLOWMULTIPRINT
			// If (fEnalbeBillMultiPrint ^ fDisableBillMultiPrint), то применяется общая конфигурация PPBillConfig
		fExtGoodsSelHideGenerics  = 0x00010000, // В списке расширенного выбора товара не показывать обобщенные товары
		fStringHistoryDisabled    = 0x00020000, // Запрет на использоватеня StringHistory (может быть проигнорирова при настройке более высокого уровня)
		fDateTimePickerBefore1124 = 0x00040000  // @v11.2.6 Использовать старые (до v11.2.4) виджеты подбора даты/периода/времени 
	};
	enum {
		wndVKDefault = 0,
		wndVKFlat    = 1,
		wndVKVector  = 2, //
		wndVKFancy   = 3  // Схема, ранее именовавшаяся как wndVKKind2 теперь обозначается wndVKFancy. Ее номер меняется,
			// вместо нее теперь будет использоваться схема wndVKVector
	};
	//
	// Descr: Флаги поля BillItemTableFlags
	//
	enum {
		bitfUseCommCfgForBarcodeSerialOptions = 0x0001,
		bitfShowBarcode                       = 0x0002,
		bitfShowSerial                        = 0x0004,
		bitfShowMargin                        = 0x0008,
	};
	UserInterfaceSettings();
	void   Init();
	void   SetVersion();
	uint32 GetBrwColorSchema() const;
	int    Save();
	int    Restore();
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);

	int32  Ver;
	int32  Flags;
	int32  BillItemTableFlags; // @v11.5.11 bitfXXX flags
	int    WindowViewStyle;
	int    TableViewStyle; // 0 - default, 1 - scheme-01, 2 - scheme-02
	int    ListElemCount;
	SFontDescr TableFont;
	SFontDescr ListFont;
	SString SupportMail;
	SString SpecialInputDeviceSymb;
};

class TStatusWin : public TWindow {
public:
	struct StItem {
		long   Icon;
		uint   Cmd;
		COLORREF Color;
		COLORREF TextColor;
		char   str[160]; // @alignment(32)
	};

	TStatusWin();
	int    GetRect(RECT *);
	int    AddItem(const char * pStr, long icon = 0, COLORREF color = 0, uint cmd = 0, COLORREF textColor = 0);
	int    Update();
	int    RemoveItem(int pos); //if(pos == -1) freeAll()
	uint   GetCmdByCoord(POINT coord, TStatusWin::StItem * pItem = 0);
private:
	DECL_HANDLE_EVENT;
	int    SetItem(int pos, const char *);
	TSArray <StItem> Items; // @todo Заменить на StringSet
};
//
// Боковое окно
//
class TreeWindow {
public:
	struct ListWindowItem {
		ListWindowItem(long cmd, ListWindow * pLw);
		~ListWindowItem();
		long   Cmd;
		ListWindow * P_Lw;
	};

	explicit TreeWindow(HWND parentWnd);
	~TreeWindow();
	static INT_PTR CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void   DelItemCmdList(void * ptr);
	void   UpdateItemCmdList(const char * pTitle, void * ptr);
	void   AddItemCmdList(const char * pTitle, void * ptr);
	void   Setup(HMENU hMenu);
	int    IsVisible();
	void   Move_(const RECT &rRect);
	void   GetRect(RECT & rRect);
	void   Show(int show);
	void   MoveChildren(const RECT & rRect);
	void   Insert(long cmd, const char * pTitle, ListWindow * pLw);
	int    TranslateKeyCode(ushort keyCode, uint * pCmd) const;

	HWND   Hwnd;
private:
	class ShortcutsWindow {
	public:
		static INT_PTR CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		ShortcutsWindow();
		~ShortcutsWindow();
		HWND   Create(HWND parentWnd);
		void   Destroy();
		void   SelItem(void * ptr);
		void   AddItem(const char * pTitle, void * ptr);
		void   UpdateItem(const char * pTitle, void * ptr);
		void   DelItem(void * ptr);
		int    IsVisible() const;
		void   GetRect(RECT & rRect);
		void   MoveWindow(const RECT & rRect);

		HWND   Hwnd;
	private:
		HWND   HwndTT; // Tooltip window
	};
	ListWindowItem * GetListWinByCmd(long cmd, uint * pPos);
	ListWindowItem * GetListWinByHwnd(HWND hWnd, uint * pPos);
	void   MenuToList(HMENU hMenu, long parentId, StrAssocArray * pList);
	void   SetupCmdList(HMENU hMenu, void * hP); // @v10.9.4 HTREEITEM-->(void *)
	void   CloseItem(HWND hWnd);
	void   SelItem(HWND hWnd);
	void   ShowList(ListWindow * pLw);
	int    OnCommand(WPARAM wParam, LPARAM lParam);

	HWND   H_CmdList;
	TSCollection <ListWindowItem> Items;
	ListWindow * P_CurLw;
	ShortcutsWindow ShortcWnd;
	TToolbar * P_Toolbar;
};
//
//
//
#define DLG_SHORTCUTS       4096
#define CTL_SHORTCUTS_ITEMS 1014
#define SPEC_TITLEWND_ID    (1200 + 100)

class TProgram : public TViewGroup {
public:
	enum {
		wndtypNone = 0,
		wndtypDialog = 1,
		wndtypChildDialog,
		wndtypListDialog
	};
	//
	// Descr: Флаги состояний State
	//
	enum {
		stUiToolBoxInited       = 0x0001, // Экземпляр UiToolBox был инициализирован вызовом InitUiToolBox
		stUiSettingInited       = 0x0002, // @v11.2.6 Экземпляр UICfg был извлечен из реестра вызовом UICfg.Restore()
		stWinCompositionEnabled = 0x0004  // @v11.6.7 Результат вызова DwmIsCompositionEnabled положительный
	};
	//
	// Descr: Флаги конструктора TProgram::TProgram
	//
	enum {
		ctrfBorderless = 0x0001 // @v11.6.7 Главное окно - borderless
	};
	//
	// @todo Заменить все вызовы TProgram::GetInst на SLS.GetHInst
	//   Проверено: это одно и то же.
	//
	static HINSTANCE GetInst();
	static void IdlePaint();
	static void DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, const RECT & rDestRect, long xOffs,
		long yOffs, COLORREF cTransparentColor, COLORREF newBkgndColor, long fmt, POINT * pBmpSize);

	TProgram(HINSTANCE hInst, const char * pAppSymb, const char * pAppTitle, uint ctrflags);
	virtual ~TProgram();
	DECL_HANDLE_EVENT;
	virtual void run();
	TView * validView(TView *p);
	void   SetupTreeWnd(HMENU hMenu, void * hP);
	void   SizeMainWnd(HWND);
	int    GetStatusBarRect(RECT *);
	int    GetClientRect(RECT *);
	int    ClearStatusBar();
	int    AddStatusBarItem(const char *, long icon = 0, COLORREF = 0, uint cmd = 0, COLORREF textColor = 0);
	int    UpdateStatusBar();
	void   GotoSite();
	//
	// Если есть новые версии, то в зависимости от флага showSelDlg либо выводит диалог выбора новой версии, либо возвращает 1.
	//
	int    ViewNewVerList(int showSelDlg);
	//
	// ARG(kind IN): 0 - BrowserWindow, 1 - STimeChunkBrowser
	//
	TBaseBrowserWindow * FindBrowser(uint resID, int kind, const char * pFileName = 0);
	void   CloseAllBrowsers();
	HWND   CreateDlg(uint dlgID, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	INT_PTR DlgBoxParam(uint dlgID, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	HBITMAP FASTCALL LoadBitmap_(uint bmID);
	HBITMAP FASTCALL FetchBitmap(uint bmID);
	HBITMAP FASTCALL FetchSystemBitmap(uint bmID);
	int    AddListToTree(long cmd, const char * pTitle, ListWindow * pLw);
	int    AddItemToMenu(const char * pTitle, void * ptr);
	int    UpdateItemInMenu(const char * pTitle, void * ptr);
	void   DelItemFromMenu(void * ptr);
	int    SelectTabItem(const void * ptr);
	int    SetWindowViewByKind(HWND hWnd, int wndType);
	int    DrawControl(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	int    EraseBackground(TView * pView, HWND hWnd, HDC hDC, int ctlType);
	//
	// Descr: Цикл обработки Windows-сообщений.
	//   Завершается либо при возврате функции MsgLoopIter <= 0,
	//   либо когда rExitSignal != 0.
	//
	int    MsgLoop(TWindow * pV, int & rExitSignal);
	TRect  MakeCenterRect(int width, int height) const;
	HWND   GetFrameWindow() const;
	void   NotifyFrame(int post);
	int    PushModalWindow(TWindow * pV, HWND h);
	int    PopModalWindow(TWindow * pV, HWND * pH);
	int    TestWindowForEndModal(TWindow * pV);
	int    IsTreeVisible() const;
	void   GetTreeRect(RECT & rRect);
	HWND   GetTreeHWND() const;
	void   ShowLeftTree(bool visible);
	//
	// Descr: Специальные смещения идентификаторов, обозначающие нюансы состояний
	// отрисовываемых объектов. Базовые инструменты имеют идентификаторы кратные 10,
	// признаки сотояний - значения от 1 до 9. Если в наборе инструментов отсутствует
	// элемент с заданным идентификатором состояния, то следует использовать ближайшее
	// значение кратное 10, не превышающее заданное.
	//
	enum {
		tbisBase = 0,
		tbisSelect,
		tbisFocus,
		tbisDisable,
		tbisHover,
		tbisDefault
	};
	//
	// Descr: Идентификаторы инструментов рисования пользовательского интерфейса
	//
	enum {
        tbiDummyFirst       =  1,
        //
		tbiIconRegColor     =  2, // Регулярный цвет иконок
		tbiIconAlertColor   =  3, //
		tbiIconAccentColor  =  4,
		tbiIconPassiveColor =  5,
        //
        tbiButtonBrush      = 10,
        tbiButtonPen        = 20,
        tbiButtonTextColor  = 30,
        tbiButtonFont       = 40,
        //
        tbiButtonBrush_F    = 50, // Идентификатор для Fancy-интерфейса (более развиваться не будет)
        tbiButtonPen_F      = 60, // Идентификатор для Fancy-интерфейса (более развиваться не будет)
		//
		tbiBlackPen         = 70,
		tbiWhitePen         = 71,
		//
		tbiInvalInpBrush    = 80, // Кисть для индикации недопустимого ввода данных
		tbiInvalInp2Brush   = 81, // Кисть для индикации недопустимого ввода данных (вариант 2)
		tbiInvalInp3Brush   = 82, // Кисть для индикации недопустимого ввода данных (вариант 3)
		tbiListBkgBrush     = 83, // Кисть отрисовки фона строки списка
		tbiListBkgPen       = 84, // Перо отрисовки фона строки списка
		tbiListFocBrush     = 85, // Кисть отрисовки focuses строки списка
		tbiListFocPen       = 86, // Перо отрисовки focuses строки списка
		tbiListSelBrush     = 87, // Кисть отрисовки selected строки списка
		tbiListSelPen       = 88, // Перо отрисовки selected строки списка
		//
		tbiControlFont      = 110, // @v11.2.3 Шрифт для отрисовки управляющих элементов 
	};

	const  UserInterfaceSettings & GetUiSettings();
	int    UpdateUiSettings(const UserInterfaceSettings & rS);
    int    InitUiToolBox();
	// @v11.9.2 /*const*/ SPaintToolBox & GetUiToolBox() /*const*/ { return UiToolBox; }
	virtual SPaintToolBox * GetUiToolBox(); // @v11.9.2
	const SDrawFigure * LoadDrawFigureBySymb(const char * pSymb, TWhatmanToolArray::Item * pInfo) const;
	const SDrawFigure * LoadDrawFigureById(uint id, TWhatmanToolArray::Item * pInfo) const;

	static TProgram * application;   // @global
	TViewGroup * P_DeskTop;
	TWindow  * P_TopView;
	TToolbar * P_Toolbar;
	HWND   H_MainWnd;
	HWND   H_Desktop;
	HACCEL H_Accel;
	HICON  H_Icon;
	HWND   H_ShortcutsWnd;
	HWND   H_LogWnd;
	HWND   H_CloseWnd;
	HWND   H_TopOfStack;
	TSStack <HWND> ModalStack;
	TreeWindow * P_TreeWnd;
protected:
	virtual int  InitStatusBar();
	//
	// Descr: Порожденный класс может реализовать загрузку набора изображений
	//   в экземпляр TWhatmanToolArray.
	//   Если он это способен сделать, то должен вернуть значение >0, если
	//   не способен - <0.
	//   В случае ошибки - 0.
	//   Если переданный указатель pT == 0, то метод должен просто сообщить
	//   в возвращаемом значение о своей способности реализовать действие.
	//
	// @v11.9.2 virtual int  LoadVectorTools(TWhatmanToolArray * pT);
	virtual const TWhatmanToolArray * GetVectorTools() const; // @v11.9.2 
	
	TStatusWin * P_Stw;
	SString AppSymbol;
	SString AppTitle;
private:
	static HINSTANCE hInstance;      // @global @threadsafe
	//
	// Pattern
	// Должена быть декларирована в порожденном классе и вызвана
	// из конструктора.
	// В этой функции должны быть инициализированы:
	//       P_MenuBar, P_DeskTop, P_Stw и, возможно, P_Rez.
	//
	//int    InitDeskTop(); // Pattern. Must be defined in derived class and called from constructor.
	static LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL    CALLBACK CloseWndProc(HWND, UINT, WPARAM, LPARAM);
	SString & MakeModalStackDebugText(SString & rBuf) const;
	int    DrawButton2(HWND hwnd, DRAWITEMSTRUCT * pDi);
	int    DrawButton3(HWND hwnd, DRAWITEMSTRUCT * pDi);
	int    DrawInputLine3(HWND hwnd, DRAWITEMSTRUCT * pDi);
	int    GetDialogTextLayout(const SString & rText, int fontId, int penId, STextLayout & rTlo, int adj);
	void   HandleWindowCompositionChanged(); // @v11.6.7
	void   HandleWindowNcCalcSize(/*struct window * data,*/WPARAM wparam, LPARAM lparam); // @v11.6.7

	long   State;
	WNDPROC PrevCloseWndProc;
	HWND   H_FrameWnd;
	TBitmapCache BmH;
	// @v11.9.2 (replaced with virtual func GetUiToolBox) SPaintToolBox UiToolBox;      // Набор инструментов для отрисовки компонентов пользовательского интерфейса.
	// @v11.9.2 (replaced with virtual func GetVectorTools) TWhatmanToolArray DvToolList; // Векторные изображения, загружаемые из внешнего файла
	UserInterfaceSettings UICfg;
};

#define APPL    (TProgram::application)

struct TBButtonCfg { // size = 4
	uint16 KeyCode;
	uint8  State;
	uint8  Style;
};

struct ToolbarCfg { // size = sizeof(uint16) + Count * sizeof(TBButton)
	ToolbarCfg();
	~ToolbarCfg();
	int    Init();
	int    Init(const void * pBuf);
	size_t GetSize() const { return (sizeof(Count) + Count * sizeof(TBButtonCfg)); }
	int    GetBuf(void ** ppBuf, size_t bufLen) const;

	uint16 Count;
	uint16 Reserve; // @alignment
	TBButtonCfg * P_Buttons;
};
//
//
//
class STooltip {
public:
	STooltip();
	~STooltip();
	int    Init(HWND parent);
	void   Destroy();
	int    Add(const char * pText, const RECT * pRect, long id);
	int    Remove(long id);
private:
	HWND   HwndTT; // Tooltip window
	HWND   Parent;
};

class SMessageWindow {
public:
	enum {
		fShowOnCenter      = 0x00000001,
		fShowOnCursor      = 0x00000002,
		fTextAlignLeft     = 0x00000010, // Текст выравнивается по левой границе окна. В противном случае - по центру.
		fCloseOnMouseLeave = 0x00000100,
		fOpaque            = 0x00000200, // Не прозрачный фон
		fSizeByText        = 0x00000400,
		fChildWindow       = 0x00000800,
		fTopmost           = 0x00001000,
		fPreserveFocus     = 0x00002000,
		fLargeText         = 0x00004000, // Крупный текст (default * 2)
		fMaxImgSize        = 0x00008000, // Максимальный размер окна для подробного отображения картинки
		fShowOnRUCorner    = 0x00010000, // @v10.9.0 Отображать окно в правом верхнем углу
	};
	//
	// Descr: Разрушает все окна сообщений, которые имеют родительское окно parent.
	//   Если parent == 0, то разрушает все окна, которые были открыты.
	//
	static void FASTCALL DestroyByParent(HWND parent);
	SMessageWindow();
	~SMessageWindow();
	int    Open(SString & rText, const char * pImgPath, HWND parent, long cmd, long timer, COLORREF color, long flags, long extra);
	void   Destroy();
	int    Paint();
	void   Move();
	int    DoCommand(SPoint2S p);
	void * GetImage() { return P_Image; }

	COLORREF Color;
	HBRUSH   Brush;
	WNDPROC PrevImgProc;
private:
	static INT_PTR CALLBACK Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	int    SetFont(HWND hCtl);

	long   Cmd;
	long   Flags;
	long   Extra;
	POINT  PrevMouseCoord;
	SString Text;
	SString ImgPath;
	HWND   HWnd;
	HFONT  Font;
	void * P_Image;
};
//
//
//
#define MIN_COLUMN_WIDTH        8
// @v12.3.8 (moved to SlConst) #define CLASSNAME_DESKTOPWINDOW "PPYDESKTOPWINDOW"

struct SBrowserDataProcBlock {
	SBrowserDataProcBlock();
	SBrowserDataProcBlock & Z();
	SBrowserDataProcBlock & Setup(const BroColumn & rC, const void * pSrcData, void * pDestBuf, size_t destBufSize, void * extraPtr);
	void   SetZero();
	void   FASTCALL Set(int32 i);
	void   Set(int64 i); // @v12.3.9
	void   Set(double i);
	void   FASTCALL Set(const char * pS);
	void   FASTCALL Set(const SString & rS);
	void   FASTCALL Set(LDATE dt);
	void   FASTCALL Set(LTIME tm);
	void   FASTCALL Set(LDATETIME dtm); // @v11.1.7

	void * ExtraPtr;         // IN
	int    ColumnN;          // IN
	TYPEID TypeID;           // IN
	const  void * P_SrcData; // IN
	uint   DestBufSize;      // @v12.3.6 IN Размер буфера для получения данных, на который указывает P_DestData
	uint32 Color;            // OUT
	void * P_DestData;       // OUT
	SString TempBuf;         // @allocreuse Может использоваться реализацией функции SBrowserDataProc для ускорения работы
};

typedef int (* SBrowserDataProc)(SBrowserDataProcBlock * pBlk); // @v12.3.8 (removed FASTCALL)

struct BroColumn {
	BroColumn();
	// @nodestructor
	TYPEID T;              // Data type
	uint   Offs;           // Offset from begining of row
	SBrowserDataProc _ColumnUserProc; // @v12.3.8 UserProc-->_ColumnUserProc
	long   format;         // Output format
	uint   Options;        //
	char * text;           // Column's title
	uint   width;          // Width of display field (Internal use)
	uint   x;              // Internal use
	uint   index;          // Internal use
	uint   OrgOffs;        // Смещение поля, заданное при создании столбца
};

struct BroGroup {
	BroGroup();
	uint   NextColumn() const;

	uint   First;
	uint   Count;
	uint   Height;
	char * P_Text;
	uint   Index;    // Internal use
};

struct BroCrosstab {
	TYPEID Type;
	long   Format;   // Output format
	uint   Options;
	char * P_Text;   // Column's title
	uint   Width;    // Width of display field (Internal use)
};

class BrowserDef : public SArray {
public:
	BrowserDef(int captionHight, uint aOptions, void * extraPtr = 0);
	~BrowserDef();
	//
	// Descr: Функция должна добавить новую колонку в список.
	// ARG(atPos IN): Позиция, в которую вставляется столбец. Если atPos < 0, то столбец добавляется в конец таблицы
	// ARG(pTxt  IN): Заголовок столбца. Может содержать символы \n для переноса на новую строку
	// ARG(fldNo IN): Номер столбца в структуре данных или смещение (в зависимости от контекста)
	// ARG(typ   IN): Тип данных. Если typ == 0, функция пытается самостоятельно опеределить тип
	//   данных по столбцу, указанному параметром fldNo
	// ARG(fmt   IN): Формат вывода данных (используйте MKSFMT или MKSFMTD)
	// ARG(opt   IN): Опции вывода данных в столбце (BCO_XXX see tvdefs.h)
	//
	virtual int    insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt);
	virtual int    insertColumn(int atPos, const char * pTxt, const char * pFldName, TYPEID typ, long fmt, uint opt);
	virtual void   setViewHight(int);
	virtual void   getScrollData(long * pScrollDelta, long * pScrollPos);
	virtual bool   IsValid() const;
	virtual int    FASTCALL go(long);
	virtual int    FASTCALL step(long);
	virtual int    top();
	virtual int    bottom();
	virtual long   GetRecsCount() const;
	virtual const  void * FASTCALL getRow(long) const;
	virtual int    refresh();
	virtual int    search(const void * pPattern, CompFunc, int srchMode, int srchCol);
	virtual int    search2(const void * pSrchData, CompFunc, int srchMode, size_t offs);
	BroColumn & FASTCALL at(uint) const;
	void   initOffset(int);
	int    addColumn(const BroColumn *, int = UNDEF);
	int    removeColumn(int);
	int    setColumnTitle(int colN, const char * pText);
	int    AddColumnGroup(BroGroup *);
	const  BroGroup * groupOf(uint column, uint * pGrpPos = 0) const;
	uint   groupWidth(uint group, uint atColumn) const;
	uint   groupWidth(const BroGroup *, uint atColumn) const;
	int    GetCellData(const void * pRowData, int column, TYPEID * pType, void * pDataBuf, size_t dataBufLen);
	int    GetCellData(long row, int column, TYPEID * pType, void * pDataBuf, size_t dataBufLen);
	char * getText(long row, int column, char * pBuf);
	//
	// Descr: Извлекает текст полностью (512 символов), независимо от ширины колонки в броузере
	//
	SString & getFullText(long row, int column, SString & rBuf);
	SString & getFullText(const void * pRowData, int column, SString & rBuf);
	char * getMultiLinesText(long, int, char *, uint = 0, uint * = 0);
	long   _topItem() const { return topItem; }
	long   _curItem() const { return curItem; }
	long   _curFrameItem() const { return (curItem - topItem); }
	bool   STDCALL IsColInGroup(uint col, uint * pIdx) const;
	int    GetCapHeight() const;
	bool   SetCapHeight(int height); // @v12.2.2
	void   VerifyCapHeight();
	uint   GetGroupCount() const;
	const  BroGroup * FASTCALL GetGroup(uint) const;
	void   ClearGroupIndexies();
	uint * GetGroupIndexPtr(uint grpN);
	int    AddCrosstab(BroCrosstab *);
	uint   GetCrosstabCount() const;
	const  BroCrosstab * GetCrosstab(uint) const;
	int    FreeAllCrosstab();
	bool   IsBOQ() const;
	bool   IsEOQ() const;
	bool   CheckFlag(uint f) const;
	void   SetUserProc(SBrowserDataProc proc, void * extraPtr);
protected:
	SBrowserDataProc UserProc;
	void * ExtraPtr;
	int    capHight;
	uint   NumGroups;
	BroGroup * P_Groups;
	int    viewHight;
	long   scrollDelta;
	bool   isBOQ; // @v11.3.4 int-->bool
	bool   isEOQ; // @v11.3.4 int-->bool
	uint8  Reserve[2]; // @v11.3.4 @alignment
	long   topItem;
	long   curItem;
private:
	virtual void FASTCALL freeItem(void *);
	const void * Helper_GetCellData(const void * pRowData, int columnIdx, TYPEID * pTypeID, long * pFmt, uint * pCOptions, void * pOuterBuf, size_t outerBufSize);

	SArray * P_CtList;         // Список кросс-таб столбцов
	SBrowserDataProcBlock DpB;
public:
	uint   options;
};

class AryBrowserDef : public BrowserDef {
public:
	AryBrowserDef(SArray * pData, const BNFieldList * pFl, int captionHight, uint aOptions, void * extraPtr = 0);
	~AryBrowserDef();
	int     setArray(SArray * pData, const BNFieldList * pFl, int setupPosition /*= 1*/);
	const   SArray * getArray() const;
	virtual bool   IsValid() const;
	virtual int    insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt);
	virtual long   GetRecsCount() const;
	virtual const  void * FASTCALL getRow(long) const;
protected:
	SArray * P_Array;
	BNFieldList * P_Fields;
};
//
//
//
class DBQBrowserDef : public BrowserDef {
public:
	enum {
		defaultFrameSize = 100
	};
	DBQBrowserDef(DBQuery & rQuery, int captionHight, uint aOptions, uint aBufSize = defaultFrameSize);
	~DBQBrowserDef();
	const  DBQuery * getQuery() const { return query; }
	int    setQuery(DBQuery & rQuery, uint aBufSize = defaultFrameSize);
	virtual int    insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID, long fmt, uint opt);
	virtual int    insertColumn(int atPos, const char * pTxt, const char * pFldName, TYPEID typ, long fmt, uint opt);
	virtual void   setViewHight(int);
	virtual void   getScrollData(long * pScrollDelta, long * pScrollPos);
	virtual bool   IsValid() const;
	virtual int    FASTCALL go(long);
	virtual int    FASTCALL step(long);
	virtual int    top();
	virtual int    bottom();
	virtual long   GetRecsCount() const;
	virtual const  void * FASTCALL getRow(long) const;
	virtual int    refresh();
	// @v10.9.0 virtual int   FASTCALL getData(void *);
	// @v10.9.0 virtual int   FASTCALL setData(void *);
protected:
	void   setupView();
	DBQuery * query;
};
//
// Messages
//
// @v10.9.11 #define BRO_GETCURREC     WM_USER+1
#define BRO_GETCURCOL     WM_USER+2
#define BRO_DATACHG       WM_USER+3
// @v10.9.0 #define BRO_SETDATA       WM_USER+4 // LPARAM = far ptr to new data, WPARAM = parameter for BrowseDef::setData virtual member function
//
// Next messages are sending to parent window
//
#define BRO_ROWCHANGED    WM_USER+5 // WPARAM = HWND, LPARAM = MAKELPARAM(Vert Scroll Pos, 0)
#define BRO_COLCHANGED    WM_USER+6 // WPARAM = HWND, LPARAM = MAKELPARAM(Horz Scroll Pos, 0)
#define BRO_LDBLCLKNOTIFY WM_USER+7 // WPARAM = HWND, LPARAM = MAKELPARAM(xPos, yPos)
#define BRO_RDBLCLKNOTIFY WM_USER+8 // WPARAM = HWND, LPARAM = MAKELPARAM(xPos, yPos)

// @v10.9.11 #define MAXCAP            64 // Максимальная длина заголовка колонки или группы
// @v10.9.11 #define MAXDEPS           32 // Максимальное количество столбцов определяющих столбец типа bcoCalc
#define BRWCLASS_CEXTRA    0 // Дополнительные данные класса "BROWSE"
#define BRWCLASS_WEXTRA    8 // Дополнительные данные окна класса "BROWSE"
#define BRWL_USERDATA      4 // Смещение в BrowseWindow для данных пользователя //

struct BrowserRectCursors {
	RECT   CellCursor;
	RECT   LineCursor;
};

struct BrowserPens {
	BrowserPens();
	void   Destroy();

	HPEN   GridHorzPen;
	HPEN   GridVertPen;
	HPEN   DrawFocusPen;
	HPEN   ClearFocusPen;
	HPEN   TitlePen;
	HPEN   FocusOuterPen;
	HPEN   DefPen;
};

struct BrowserBrushes {
	BrowserBrushes();
	void   Destroy();

	HBRUSH DrawBrush;
	HBRUSH ClearBrush;
	HBRUSH TitleBrush;
	HBRUSH DefBrush;
	HBRUSH CursorBrush;
};

struct RowHeightInfo {
	long   Top;
	uint   HeightMult;
};

HWND FASTCALL GetNextBrowser(HWND hw, int reverse);

class TBaseBrowserWindow : public TWindowBase {
public:
	~TBaseBrowserWindow();
	struct IdentBlock {
		int    IdBias;
		SString ClsName;
		SString InstanceIdent;
	};
	virtual TBaseBrowserWindow::IdentBlock & GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk);
	//
	// Descr: Создает окно и вставляет его в общий стек окон в немодальном режиме.
	//   Если текущим активным окном не является APPL->H_MainWnd, то окно запускается //
	//   в модальном режиме.
	//   Внимание: в случае, если окно было запущено в модальном режиме, после завершения //
	//   функции оно окажется разрушенным, то есть указатель this теряет свою актуальность.
	// Returns:
	//   <0 - окно было загружено в НЕМОДАЛЬНОМ режиме и осталось на экране
	//        после выхода из функции.
	//   >0 - окно было загружено в МОДАЛЬНОМ режиме. Этим значением возвращается //
	//        команда, по которой был завершен модальный цикл.
	//   0  - ошибка.
	//
	int    Insert();
	uint   GetResID() const;
	void   SetResID(uint res);

	enum {
		IdBiasBrowser  = 0x00100000,
		IdBiasTimeChunkBrowser = 0x00200000,
		IdBiasTextBrowser      = 0x00800000
	};
protected:
	TBaseBrowserWindow(LPCTSTR pWndClsName);
	DECL_HANDLE_EVENT;
	static TBaseBrowserWindow * Helper_InitCreation(LPARAM lParam, void ** ppInitData);

	enum {
		bbsIsMDI        = 0x00000001,
		bbsDataOwner    = 0x00000002, // Объект владеет переданными из-вне данными.
		bbsWoScrollbars = 0x00000004,
		bbsCancel       = 0x00000008, // Какой-то из виртуальных методов порожденного класса потребовал прекратить выполнение
		// Начиная с 0x00010000 флаги зарезервированы за наследующими классами
	};
	const  SString ClsName; // Window class name
	uint   ResourceID;
	SPoint2S PrevMouseCoord;
	long   BbState;
	uint   ToolbarID;       // ID Toolbar'a для сохранения в реестре = LastCmd (команда по которой был запущен данный броузер) + TOOLBAR_OFFS (смещение)
	int    ToolBarWidth;
	TToolbar * P_Toolbar;
};

class BrowserWindow : public TBaseBrowserWindow {
public:
	class CellStyle {
		friend class BrowserWindow;
	public:
		int    FASTCALL SetFullCellColor(COLORREF c); // returns strictly 1
		int    FASTCALL SetRightFigCircleColor(COLORREF c); // returns strictly 1
		int    FASTCALL SetRightFigTriangleColor(COLORREF c); // @v11.1.12 returns strictly 1
		int    FASTCALL SetLeftBottomCornerColor(COLORREF c); // returns strictly 1
		int    FASTCALL SetLeftTopCornerColor(COLORREF c); // returns strictly 1
		enum {
			fCorner           = 0x0001,
			fLeftBottomCorner = 0x0002,
			fRightFigCircle   = 0x0004,
			fRightFigTriangle = 0x0008 // @v11.1.12
		};

		COLORREF Color;
		COLORREF Color2; // Цвет для нижнего левого угла
		COLORREF RightFigColor; // Цвет фигуры в правой части ячейки
		long   Flags;
		SString Description;
	private:
		CellStyle();
	};

	typedef int (* CellStyleFunc)(const void * pData, long col, int paintAction, CellStyle *, void * extraPtr);

	static int RegWindowClass(HINSTANCE hInst);
	static LRESULT CALLBACK BrowserWndProc(HWND, UINT, WPARAM, LPARAM);

	static LPCTSTR WndClsName;

	BrowserWindow(uint resID, DBQuery * pDataQuery, uint broDefOptions);
	BrowserWindow(uint resID, SArray * pDataArray, uint broDefOptions);
	//
	// Descr: Конструктор, содающий экземпляр с пустым BrowserDef (нет ресурса, из которого можно сформировать
	//   колонки и прочие параметры таблицы).
	//   Вызывающий модуль самостоятельно должен все сделать программно.
	//
	BrowserWindow(SArray * pDataArray, uint broDefOptions);
	~BrowserWindow();
	//
	// Descr: Меняет запрос и, возможно, загружает другой ресурс таблицы для отображения.
	//   Если resID не равен текущему значению ResourceID, то загружает ресурс resID.
	// Returns:
	//   1 - запрос pQuery был установлен, но ресурс resID не отличается от this->ResourceID.
	//   2 - запрос pQuery был установлен и загружен ресурс resID
	//   0 - error
	//
	int    ChangeResource(uint resID, uint extToolbarId, DBQuery * pQuery, bool force = false);
	int    ChangeResource(uint resID, uint extToolbarId, SArray * pArray, bool force = false);
	void   CalcRight();
	void   SetupScroll();
	int    insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt);
		// @>>BrowserDef::insertColumn(in, const char *, uint, TYPEID, long, uint)
	int    insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt, SBrowserDataProc proc); // @v12.3.8
	int    insertColumn(int atPos, const char * pTxt, const char * pFldName, TYPEID typ, long fmt, uint opt);
	int    removeColumn(int atPos);
	void   SetColumnWidth(int colNo, int width);
	void   SetupColumnsWith();
	int    SetColumnTitle(int conNo, const char * pText);
	void   SetFreeze(uint);
	LPRECT ItemRect(int hPos, int vPos, LPRECT, BOOL isFocus) const;
	LPRECT LineRect(int vPos, LPRECT, BOOL isFocus);
	int    GetColumnByX(int x) const;
	int    ItemByPoint(SPoint2S point, long * pHorzPos, long * pVertPos) const;
	enum {
		hdrzoneAny = 0,
		hdrzoneSortPoint = 1,
	};
	int    HeaderByPoint(SPoint2S point, int hdrzone, long * pVertPos) const;
	int    ItemByMousePos(long * pHorzPos, long * pVertPos);
	//
	// ARG(action IN):
	//   -1 - clear (alt + left button down)
	//   0  - clear, add one column (left button down)
	//   1  - add column (ctrl+left button down)
	//
	int    SelColByPoint(const POINT *, int action);
	void   FocusItem(int hPos, int vPos);
	int    IsResizePos(SPoint2S);
	void   Resize(SPoint2S p, int mode); // mode: 0 - toggle off, 1 - toggle on, 2 - process
	void   Refresh();
	BrowserDef * getDef();
	const  BrowserDef * getDefC() const;
	void   SetDefUserProc(SBrowserDataProc proc, void * extraPtr);
	void   go(long p);
	void   top();
	void   bottom();
	void   setRange(ushort aRange);
	void   search(const char * pFirstLetter, int srchMode);
	int    search2(const void * pSrchData, CompFunc, int srchMode, size_t offs);
	//
	// Descr: Возвращает текущую колонку в таблице.
	// Returns:
	//   Номер текущей колонки [0..P_Def->getCount()-1]
	//
	int    GetCurColumn() const;
	void   SetCurColumn(int col);
	void   setInitPos(long p);
	void   SetColorsSchema(uint32 schemaNum);
	int    CopyToClipboard();
	//
	// For modeless
	//
	const  void * getCurItem();
	const  void * getItemByPos(long pos);
	//
	// Descr: Сохраняет в реестре параметры таблицы, установленные пользователем.
	// ARG(ifChangedOnly IN): Если !0, то параметры будут сохранены только в случае, если
	//   были зафиксированы изменения (переменная IsUserSettingsChanged).
	//
	int    SaveUserSettings(int ifChangedOnly);
	int    RestoreUserSettings();
	const  UserInterfaceSettings * GetUIConfig() const;
	void   SetCellStyleFunc(CellStyleFunc, void * extraPtr);
	//
	// Descr: Возвращает цвет некоторой ячейки
	//
	int    GetCellColor(long row, long col, COLORREF * pColor);
	uint   GetRezID() const { return RezID; }
	//
	// Descr: Возвращает список номеров колонок, по котороым должны быть отсортированы данные
	//
	const  LongArray & GetSettledOrderList() const { return SettledOrder; }
	//
	// @v12.2.2
	// Descr: Ситуативная функция, рассчитывающая некоторые метрики окна. Содержание
	//   функции извлечено из WMHCreate() с целью пересчитать размер заголовка 
	//   в случае, если его параметры были изменены.
	//
	void   EvaluateSomeMetricsOnInit();

	enum {
		paintFocused = 0,
		paintNormal  = 1,
		paintClear   = 2,
		paintQueryDescription = 3 // Специальная опция, передаваемая в callback-функцию CellStyleFunc для получения дополнительной информации о ячейке и ее окраске.
	};
protected:
	DECL_HANDLE_EVENT;
	void   WMHScroll(int sbType, int sbEvent, int thumbPos);
	int    WMHScrollMult(int sbEvent, int thumbPos, long * pOldTop);
	int    LoadResource(uint rezID, void * pData, int dataKind, uint uOptions/*= 0*/);

	uint   RezID;
private:
	virtual void Insert_(TView *p);
	virtual TBaseBrowserWindow::IdentBlock & GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk);
	void   __Init();
	void   WMHCreate();
	long   CalcHdrWidth(int plusToolbar) const;
	int    IsLastPage(uint viewHeight); // AHTOXA
	void   ClearFocusRect(const RECT & rR);
	void   DrawCapBk(HDC, const RECT & rR, bool);
	void   DrawFocus(HDC, const RECT & rR, bool drawOrClear, bool isCellCursor = false);
	void   Paint();
	//
	// col = -1 - раскрашивать строчку целиком
	//
	int    PaintCell(HDC hdc, RECT r, long row, long col, int paintAction);
	int    search(void * pPattern, CompFunc fcmp, int srchMode);
	bool   DrawTextUnderCursor(HDC hdc, char * pBuf, RECT * pTextRect, uint fmt, int isLineCursor);
	void   AdjustCursorsForHdr();
	int    CalcRowsHeight(long topItem, long bottom = 0);
	void   DrawMultiLinesText(HDC hdc, char * pBuf, RECT * pTextRect, uint fmt);
	int    FASTCALL CellRight(const BroColumn & rC) const;
	int    FASTCALL GetRowHeightMult(long row) const;
	int    FASTCALL GetRowTop(long row) const;

	long   InitPos;
	TView * P_Header;
	BrowserDef * P_Def;
	LOGFONT FontRec;
	HGDIOBJ Font;
	BrowserPens Pens;
	BrowserBrushes Brushes;
	HFONT   DefFont;
	HCURSOR MainCursor;
	HCURSOR ResizeCursor;
	BrowserRectCursors RectCursors;
	int    ResizedCol;
	SPoint2S CliSz;   // Размер клиентской области окна
	SPoint2S ChrSz;   // Средний размер символов.
	int    YCell;   // Hight of cell
	int    CapOffs; // Offset from top of window to begining of rows
	uint   Left;
	uint   Right;
	uint   Freeze;
	int    IsUserSettingsChanged;
	uint   ViewHeight;
	uint   VScrollMax;
	uint   VScrollPos;
	uint   HScrollMax;
	uint   HScrollPos;
	SString SearchPattern;
	CompFunc SrchFunc;
	CellStyleFunc F_CellStyle;
	void * CellStyleFuncExtraPtr;
	int    LastResizeColumnPos; // Используется в методе Resize()
	SArray * P_RowsHeightAry;   // Высота строк видимой страницы (для многострочных броузеров)
	UserInterfaceSettings UICfg;
	LongArray SelectedColumns;  // Выбранные столбцы, для копирования в буфер обмена
	LongArray SettledOrder;     // Индексы столбцов, по которым задана сортировка.
		// Если индекс <0, то сортировка в обратном направлении (какое направление прямое а какое обратное определяет конкретный класс-наследник).
};
//
//
//
class STimeChunkBrowser : public TBaseBrowserWindow {
public:
	struct Param {
		Param();
		Param & Z();
		int    SetScrPeriod(const DateRange & rPeriod);
		enum {
			fSnapToQuant = 0x0001, // При создании или редактировании отрезков время округлять до ближайшего кванта
			fUseToolTip  = 0x0002, // Отображать подсказки при наведении курсора на прямоугольники
			fInterlaced  = 0x0004  // Отображать горизонтальные полосы с черезстрочным отличием цвета
		};
		enum {
			vPrcTime = 0, // Слева - процессоры, сверху - время //
			vHourDay      // Слева - часы, сверху - дни //
		};
		uint   Quant;          // Квант временной решетки (секунд). Для vHourDay - Квант одного дня (секунд)
		uint   PixQuant;       // Квант временной решетки (в пикселах). Для vHourDay - Квант одного дня (в пикселах)
		uint   PixRow;         // Высота одной полосы (в пикселах)
		uint   PixRowMargin;   // Вертикальное поле полосы с одной стороны (в пикселах)
		uint   HdrLevelHeight; // Высота отрисовки одного уровня заголовка (в пикселах)
		uint   TextZonePart;   // Процентная доля левой (текстовой) панели окна
		DateRange DefBounds;   // Границы периода по умолчанию.
		long   ViewType;       // Param::vXXX Тип отображения //
		DateRange ScrPeriod;   // Период, который должен отображаться в окне (по оси X).
		long   DaysCount;      // (Рекомендованное) количество дней, отображаемых по оси X в режиме vHourDay. default=7
		long   SingleRowIdx;   // Просмотр в режиме vHourDay единственной строки P_Data с индексом SingleRowIdx. -1 - undefined
		long   Flags;
		SString RegSaveParam;  // Имя параметра, по которому должны быть сохранены установки таблицы в системном реестре
	};
	//
	// Descr: Структура описания точки в области окна
	//
	struct Loc {
		enum {
			kWorkspace = 1,
			kChunk,
			kHeader,
			kLeftZone,
			kLeftHeader,     //
			kSeparator,      // Разделитель левой и правой частей
			kPicMode         // Иконка переключения режима просмотра
		};
		enum {
			pInner = 1,      // Внутренняя часть области
			pLeftEdge,       // Левая грань
			pTopEdge,        // Верхняя грань
			pRightEdge,      // Правая грань
			pBottomEdge,     // Нижняя грань
			pMoveSpot        // Пятно в активном отрезке, при нажатии на которое можно перемещать отрезок
		};
		union {
			long   RowId;    // Идентификатор строки грида, которой соответствует точка
			long   HdrLevel; // Индекс уровня заголовка, которому соответствует точка
		};
		long   EntryId;      // Идентификатор отрезка, которому соответствует точка
		STimeChunk Chunk;    // Значение временного отрезка EntryId
		LDATETIME Tm;        // "Точное" время, которому соответствует точка
		LDATETIME TmQuant;   // Время, округленное до кванта (в меньшую сторону), которому соответствует точка
		long   Kind;         // kXXX Вид позиции
		long   Pos;          // pXXX Особенность позиционирования точки в пределах отрезка
		long   Flags;        //
	};
	static int RegWindowClass(HINSTANCE hInst);
	// @v12.3.8 static const char * WndClsName;

	STimeChunkBrowser();
	~STimeChunkBrowser();

	enum {
		bmpModeGantt = 1,
		bmpModeHourDay,
		bmpBack
	};
	void   SetBmpId(int ident, uint bmpId);
	int    SetParam(const Param *);
	int    SetData(STimeChunkGrid *, int takeAnOwnership);
	int    FASTCALL IsKeepingData(const STimeChunkGrid *) const;
	//
	// Descr: Обновляет данные, измененные вне диаграммы
	//
	void   UpdateData();
	int    Locate(SPoint2S p, Loc * pLoc) const;
	int    RestoreParameters(STimeChunkBrowser::Param & rParam);
protected:
	struct ResizeState {
		void   Setup(int kind, SPoint2S p);
		enum {
			kNone = 0,    // Нет режима изменения размера (масштаба)
			kRescale = 1, // Изменение масштаба захватом левой грани элемента заголовка
			kSplit,       // Изменение ширины левой части окна
			kChunkLeft,   // Изменение длины отрезка захватом левой грани отрезка
			kChunkRight,  // Изменение длины отрезка захватом правой грани отрезка
			kMoveChunk,   // Перемещение отрезка
			kRowHeight,   // Высота строки
			kScroll,      // Скроллинг захватом мыщью точки в рабочей области
			kSwitchMode   // Переключение режима отображения //
		};
		int16  Kind;
		int16  HdrLevel;     // Уровень заголовка, на котором происходит изменение масштаба
		union {
			long   Quant;    // Это значение указывает на квант, левая грань которого захвачена мышью.
			long   ChunkId;  // Отрезок, размер которого меняется //
		};
		long   RowId;        // Ид строки, в которой находится в текущий момент перемещаемый отрезок
		long   Shift;        // Текущее смещение от начальной позиции
		SPoint2S Org;        // Точка отсчета для изменения размеров (масштаба)
		TRect  Prev;         //
	};
	struct State {
		STimeChunk Bounds;      // Границы области определения //
		uint   ScrollX;         // Скроллированное количество временных квантов
		uint   ScrollY;         // Скроллированное количество строк грида
		uint   ScrollLimitY;    // Рассчитанное значение лимита вертикального скроллирования //
		uint   QBounds;         // @<=Bounds Границы области определения в квантах (дабы каждый раз не вычислять)
		uint   HdrLevelCount;   // Количество уровней заголовков
		long   HdrLevel[8];     // Уровни заголовков. Каждый уровень задается количеством секунд
		uint   TextZonePart;    // Процентная доля левой (текстовой) панели окна. Зависит от переданного
			// из-вне значения Param::TextZonePart, способности источника данных передать строки для этой
			// панели и выбора пользователя.
		uint   Hd_VPixQuant;    // Количество пикселей в одном дне при режиме vHourDay
		uint   Hd_HQuant;       // Квант времени (секунд) по вертикальной шкале в режиме vHourDay
		uint   Hd_HPixQuant;    // Количество пикселей в одном кванте по вертикальной шкале в режиме vHourDay
		long   SelChunkId;      // -1 - undefined
		ResizeState Rsz;
	};
	struct RowState {
		long   Id;
		uint   Order;
		LAssocArray OrderList; // Соответствие между индексом элемента в списке и его
			// вертикальным расположением в строке.
			// Индекс элемента [1..], вертикальное положение [0..Order-1]
			// Если Order == 0 || Order == 1, то список пуст.
	};
	struct Area {
		Area();
		Area & Z();

		enum {
			fInited = 0x0001 // Структура инициализирована вызовом STimeChunkBrowser::GetArea()
		};
		long   Flags;          // Служебные флаги
		TRect  Full;           // Полная область отрисовки
		TRect  Left;           // Левая часть - служебная (включает всю высоту Full)
		TRect  Right;          // Правая часть - рабочая (включает всю высоту Full)
		TRect  Separator;      // Вертикальный разделитель левой и правой областей
		TRect  LeftHeader;     // Левая часть верхней заголовочной области
		TRect  RightHeader;    // Правая часть верхней заголовочной области
		TRect  PicMode;        // Область отрисовки иконки переключения режима. Если PicMode::IsEmpty, то - не отрисовывать
		//
		uint   Quant;          // Количество секунд в одном горизонтальном кванте времени.
			// При P.ViewType == vPrcTime this->Quant == P.Quant,
			// при P.ViewType == vHourDay this->Quant рассчитывается методом STimeChunkBrowser::GetArea().
		uint   PixQuant;       // Рассчитанное количество пикселей на горизонтальный квант времени.
			// При P.ViewType == vPrcTime this->PixQuant == P.PixQuant,
			// при P.ViewType == vHourDay this->PixQuant рассчитывается методом STimeChunkBrowser::GetArea().
		uint   PixPerHour;     // В режиме vHourDay высота одного часа в пикселах. В общем случае, может быть расчетным.
	};
	struct SRect : public TRect {
		SRect();
		SRect & Z();

		long   RowId;
		uint   DayN;  // Служебное значение, идентифицирующее номер даты на экране в режиме vHourDay
		STimeChunkAssoc C;
	};
	class SRectArray : public TSVector <SRect> {
	public:
		SRectArray();
		const SRect * FASTCALL SearchPoint(SPoint2S p) const;
	};

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual TBaseBrowserWindow::IdentBlock & GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk);
	//
	// Descr: Функция экспорта в Excel вынесена в виде виртуального метода из-за того, что
	//   слой SLIB не имеет собственного механизма экспорта данных в Excel.
	//   Модуль верхнего уровня должен самостоятельно реализовать этот метод.
	//
	virtual int ExportToExcel();
	int    FASTCALL GetArea(Area & rArea) const;
	STimeChunk FASTCALL GetBoundsTime(const Area & rArea) const;
	void   CalcHdTimeBounds(const Area & rArea, DateRange & rPeriod, uint & rMinHour, uint & rMaxHour) const;
	long   DiffTime(const LDATETIME & rEnd, const LDATETIME & rStart) const;
	LDATETIME AddTime(const LDATETIME & rStart, long sec) const;
	int    FASTCALL IsQuantVisible(long q) const;
	int    FASTCALL SecToPix(long t) const;
	long   FASTCALL PixToSec(int  p) const;
	int    ChunkToRectX(uint leftEdge, const STimeChunk & rChunk, const LDATETIME & rStart, TRect & rRect) const;
	TRect  GetMargin() const;
	int    FASTCALL InvalidateChunk(long chunkId);
	void   Paint();
	void   DrawMoveSpot(TCanvas & rCanv, SPoint2S p);
	//
	// Descr: Вычисляет области окна, занимаемые временными отрезками
	//
	int    CalcChunkRect(const Area * pArea, SRectArray & rRectList);
	//
	// Descr: Вычисляет прямоугольную область и порядок, соответствующие строке с идентификатором rowId
	// ARG(side IN): 0 - full, 1 - left, 2 - right
	//
	int    GetRowRect(long rowId, int side, int * pOrder, TRect * pRect) const;
	//
	// Descr: Вычисляет и объявляет недействительной область перерисовки
	//   при изменении размеров внутренних элементов
	//
	int    InvalidateResizeArea();
	uint   GetBottomRowIdx(uint startIdx) const;
	uint   GetScrollLimitY() const;
	void   Scroll(int sbType, int sbEvent, int thumbPos);
	//
	// ARG(mode IN): 1 - start, 0 - end, 2 - continue
	// Returns:
	//   >0 - окно находится в режиме изменения размеров
	//   <0 - окно не находится в режиме изменения размеров
	//
	int    Resize(int mode, SPoint2S p);
	int    ProcessDblClk(SPoint2S p);
	void   OnUpdateData();
	void   SetupScroll();
	int    GetChunkText(long chunkId, SString & rBuf);
	void   RegisterMouseTracking();
	void   FASTCALL GetStartPageDate(LDATE * pDt);
	const  RowState & FASTCALL GetRowState(long id) const;
	int    SelectChunkColor(const STimeChunkAssoc * pChunk, HBRUSH * pBrush);
	int    GetChunkColor(const STimeChunkAssoc * pChunk, STimeChunkGrid::Color * pClr) const;
	int    SaveParameters();
	int    SetupDate(LDATE dt);
	const  STimeChunkArray * GetCollapseList_() const;
	int    CopyToClipboard();
	enum {
		dummyFirst = 1,
		colorHeader,            // Цвет отрисовки заголовка таблицы
		colorMain,              // Основной цвет фона
		colorInterleave,        // Цвет фона черезстрочных линий
		colorWhiteText,         // Белый цвет текста
		colorBlackText,         // Черный цвет текста
		penQuantSeparator,      // Вертикальные линии на весь экран, отстоящие друг от друга на кратном кванту расстоянии
		penMainQuantSeparator,  // Вертикальные линии на весь экран, отстоящие друг от друга на расстоянии, равном кванту //
		penDaySeparator,        // @v6.8.1 Вертикальные линии на весь экран, отстоящие друг от друга на расстоянии, равном одному дню //
		penDefChunk,            // Контур регулярного отрезка
		penSelectedChunk,       // Контур выбранного отрезка
		penChunk,               // Контур невыделенного отрезка
		penResizedChunk,        // Контур отрезка, размеры которого изменяются //
		penMainSeparator,       // Линии раздела зон и уровней заголовков
		penCurrent,             // Линия текущего меомента времени
		brushNull,              // Пустая кисть (для незакрашенных контуров)
		brushHeader,            // (colorHeader) Кисть для отрисовки заголовка таблицы
		brushMain,              // (colorMain)   Кисть для отрисовки основной области окна
		brushDefChunk,          // Кисть по умолчанию для прямоугольников, отображающих временные отрезки
		brushMovedChunk,        // Кисть для отрисовки оригинала перемещаемого отрезка
		brushRescaleQuant,      // Кисть для отрисовки прямоугольника индикации смены масштаба
		brushInterleave,        // (colorInterleave) Кисть черезстрочного выделения //
		brushHoliday,           // Кисть отображения выходных дней
		brushHolidayInterleave, // Кисть отображения выходных дней
		fontHeader,             // Шрифт для вывода подписей к заголовочной шкале
		fontLeftText,           // Шрифт для вывода текста в левой части окна
		fontChunkText,          // Шрифт для вывода текста временных отрезков

		curRegular,            // Регулярный курсор
		curResizeHorz,         // Курсор изменения горизонтальных размеров
		curResizeVert,         // Курсор изменения вертикальных размеров
		curResizeRoze,         // Курсор перемещения //
		curCalendar,           // Курсор выбора даты
		curHand                // Курсор над инструментальной кнопкой
	};
	STimeChunkGrid * P_Data; // @notowned
	STimeChunkGrid DataStub; // Заглушка для нулевого указателя P_Data //
	SPaintToolBox Ptb;
	Param  P;
	State  St;
	enum {
		stMouseTrackRegistered = 0x0004  // Была вызвана функция RegisterMouseTrack()
	};
	long   Flags;
	uint   BmpId_ModeGantt;
	uint   BmpId_ModeHourDay;
	uint   BmpId_Back;
	TSCollection <RowState> RowStateList;
	StrAssocArray ChunkTextCache;
	LAssocArray ChunkColorCache; // хранит пары {status; brush}
	LAssocArray ColorBrushList;  // Список кистей, созданных для отрисовки отрезков
	SRectArray RL;               // Список прямоугольников, соответствующих элементам P_Data.
	TToolTip * P_Tt;
};
//
//
//
class SKeyAccelerator : public LAssocArray {
public:
	SKeyAccelerator();
	int    Set(const KeyDownCommand & rK, int cmd);
};

int ImpLoadToolbar(TVRez & rez, ToolbarList * pList);

class SScEditorStyleSet : private SStrGroup {
public:
	enum {
		sgUnkn = 0,
		sgLexer,
		sgGlobal
	};
	enum {
		FONTSTYLE_NONE      = 0x0000,
		FONTSTYLE_BOLD      = 0x0001, // @persistent
		FONTSTYLE_ITALIC    = 0x0002, // @persistent
		FONTSTYLE_UNDERLINE = 0x0004  // @persistent
	};
	struct Style {
		int    Group;
		int    LexerId;
		int    StyleId;
		SColor BgC;
		SColor FgC;
		int    FontStyle;
		uint   FontSize;
		SString LexerDescr;
		SString StyleName;
		SString FontFace;
		SString KeywordClass;
	};
	struct LangModel {
        int   LexerId;
        SString CommentLine;
        SString CommentStart;
        SString CommentEnd;
        SString ExtList;
	};
	struct LangModelKeywords {
        int   LexerId;
		SString KeywordClass;
		SString KeywordList;
	};
	SScEditorStyleSet();
	~SScEditorStyleSet();
	void   Destroy();
	int    GetStyle(int group, int lexerId, int styleId, Style & rS) const;
	int    GetStyles(int group, int lexerId, TSCollection <Style> * pList) const;
	int    GetModel(int lexerId, LangModel * pModel) const;
	int    GetModelKeywords(int lexerId, TSCollection <LangModelKeywords> * pList) const;
	int    ParseStylesXml(const char * pFileName);
	int    ParseModelXml(const char * pFileName);
private:
	struct InnerStyle {
		int    Group;
		int    LexerId;
		int    StyleId;
		SColor BgC;
		SColor FgC;
		int    FontStyle;
		uint   FontSize;
		uint   LexerDescrP;
		uint   StyleNameP;
		uint   FontFaceP;
		uint   KeywordClassP;
	};
	struct InnerLangModel {
        int    LexerId;
        uint   CommentLineP;
        uint   CommentStartP;
        uint   CommentEndP;
        uint   ExtListP;
	};
	struct InnerLangModelKeywords {
        int    LexerId;
		uint   KeywordClassP;
		uint   KeywordListP;
	};
	void   FASTCALL InnerToOuter(const InnerStyle & rS, Style & rD) const;
	void   FASTCALL InnerToOuter(const InnerLangModel & rS, LangModel & rD) const;
	void   FASTCALL InnerToOuter(const InnerLangModelKeywords & rS, LangModelKeywords & rD) const;
	int    ReadStyleAttributes(const xmlNode * pNode, InnerStyle & rS);

	TSVector <InnerLangModel> ML;
	TSVector <InnerLangModelKeywords> KwL;
	TSVector <InnerStyle> L;
};

class SScEditorBase {
public:
	enum {
		indicUnknWord = 27,
		indicStxRule  = 28 // @experimental Распознанное синтаксическое правило
	};
	SScEditorBase();
	~SScEditorBase();
	int    SetKeybAccelerator(KeyDownCommand & rK, int cmd);
protected:
	//
	// Descr: Константы для полей редактирования
	//
	enum {
		scmargeLineNumber = 0,
		scmargeSymbole    = 1,
		scmargeFolder     = 2,
	};
	void   Init(HWND hScW, int preserveFileName);
	int    Release();
	intptr_t CallFunc(uint msg);
	intptr_t CallFunc(uint msg, uintptr_t param1);
	intptr_t CallFunc(uint msg, uintptr_t param1, intptr_t param2);
	int    SetLexer(const char * pLexerName);
	void   SetSpecialStyle(const SScEditorStyleSet::Style & rStyle);
	void   ClearIndicator(int indicatorNumber);
	int32  GetCurrentPos();
	int32  FASTCALL SetCurrentPos(int32 pos);
	int    FASTCALL GetSelection(IntRange & rR);
	int    FASTCALL SetSelection(const IntRange * pR);
	int    FASTCALL GetSelectionText(SString & rBuf);
	enum {
		srfUseDialog = 0x0001
	};
	int    SearchAndReplace(long flags);

	typedef void * SciDocument;

	class Document {
	public:
		enum {
			stInit     = 0x0001,
			stDirty    = 0x0002,
			stReadOnly = 0x0004,
			stUtf8Mode = 0x0008,
			stNewFile  = 0x0010
		};
		Document();
		Document & FASTCALL Reset(int preserveFileName);
		long   SetState(long st, int set);

		SCodepageIdent OrgCp;
		SCodepageIdent Cp;
		SEOLFormat Eolf;
		long   State;
		SciDocument SciDoc;
		SString FileName;
	};
	SKeyAccelerator KeyAccel; // Ассоциации клавиатурных кодов с командами. {KeyDownCommand Key, long Val}
	SKeyAccelerator OuterKeyAccel; // Ассоциации клавиатурных кодов с командами, заданные из-вне: вливаются в KeyAccel
	intptr_t (*P_SciFn)(void * ptr, uint msg, uintptr_t wParam, intptr_t lParam);
	void * P_SciPtr;
	SSearchReplaceParam LastSrParam;
	Document Doc;
	STokenizer * P_Tknzr;
};

class STextBrowser : public TBaseBrowserWindow, public SScEditorBase {
public:
	static int RegWindowClass(HINSTANCE hInst);
	static LPCTSTR WndClsName;
	//
	// Descr: Флаги загрузки файла и сохранения файлов
	//
	enum {
		ofReadOnly          = 0x0001,
		ofInteractiveSaveAs = 0x0002
	};
	//
	// Descr: Специальные режимы работы с текстовыми документами
	//
	enum {
		spcmNo = 0,     // Обычный режим
		spcmSartrTest   // Режим тестирования базы данных SARTR
	};
	struct StatusBlock {
		uint   TextSize;  // Длина текста в байтах
		uint   LineCount; // Количество строк в тексте
		uint   LineNo;    // Номер текущей строки
		uint   ColumnNo;  // Номер текущей колонки
		SCodepageIdent Cp; // Идентификатор кодовой страницы
	};

	STextBrowser();
	STextBrowser(const char * pFileName, const char * pLexerSymb, int toolbarId = -1);
	~STextBrowser();
	int    Init(const char * pFileName, const char * pLexerSymb, int toolbarId = -1);
	int    GetStatus(StatusBlock * pSb);
	int    SetSpecialMode(int spcm);
	int    WMHCreate();
	HWND   GetSciWnd() const { return HwndSci; }
	void   Resize();
	int    CmpFileName(const char * pFileName);
	int    FileLoad(const char * pFileName, SCodepage cp, long flags);
	int    FileSave(const char * pFileName, long flags);
	int    FileClose();
private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ScintillaWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual TBaseBrowserWindow::IdentBlock & GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk);
	virtual int ProcessCommand(uint ppvCmd, const void * pHdr, void * pBrw);
	int    GetText(SString & rBuf);
	int    SetText(SString & rBuf);
	int    SaveChanges();
	int    SetEncoding(SCodepage cp);
	SCodepage SelectEncoding(SCodepage initCp) const;
	int    InsertWorkbookLink();
	int    BraceHtmlTag();
	int    UpdateIndicators();
	bool   IsFolded(size_t line);
	void   Fold(size_t line, bool mode);
	void   Expand(size_t & rLine, bool doExpand, bool force, int visLevels, int level);
	void   MarginClick(/*Sci_Position*/int position, int modifiers);
	void   RunMarkers(bool doHide, size_t searchStart, bool endOfDoc, bool doDelete);
	int    Run();

	enum {
		sstLastKeyDownConsumed = 0x0001
	};
	long   SysState;
	int    SpcMode;
	HWND   HwndSci;
	SString LexerSymb;
	WNDPROC OrgScintillaWndProc;
};
//
// 
//
class TWhatmanBrowser : public TBaseBrowserWindow { // @v11.0.0
public:
	struct Param {
		enum {
			fEditMode = 0x0001
		};
		Param() : Flags(0)
		{
		}
		uint32 Flags;
		SString WtmFileName;
		SString WtaFileName;
	};
	static int RegWindowClass(HINSTANCE hInst);
	static LPCTSTR WndClsName;
	TWhatmanBrowser(Param * pP);
	TWhatmanBrowser(const char * pFileName, int toolbarId = -1);
	~TWhatmanBrowser();
private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void   InitLayout();
	int    WMHCreate();
	Param  P;
};
//
//
//
class TVRez {
public:
	struct WResHeaderInfo {
		uint16 Type;
		int16  IdKind;   // 0 - uint16, !0 - string
		union {
			uint16 IntID;
			char   StrID[256];
		};
		uint16 Flags;
		uint32 Size;
		uint32 Next;
	};
	enum ResPosition {
		beginOfResource, sizeField, beginOfData, nextResource
	};
	enum {
		hdrUnknown, hdr16, hdr32
	};
	TVRez(const char * pFileName, bool useIndex = false);
	~TVRez();
	int    open(const char * pFileName, bool useIndex = false);
	int    setHdrType();
	int    buildIndex();
	int    getChar();
	//
	// Descr: Считывает из потока ресурса 2 байта и возвращает в виде uint
	//
	uint   getUINT();
	//
	// Descr: Считывает из потока ресурса 2 байта и возвращает в виде long.
	// Note: Еще раз - считывается 2 байта, а не 4 и не 8!
	//   Функция реализована только для того, чтобы не преобразовывать бесконечно (long)getUINT()
	//
	long   getLONG(); // @v12.2.3
	char * STDCALL getString(char *, int kind = 0 /*0 - 866, 1 - w_char, 2 - 1251*/);
	SString & STDCALL getString(SString & rBuf, int kind /*0 - 866, 1 - w_char, 2 - 1251*/);
	TRect  getRect();
	TYPEID getType(int defaultLen);
	long   getFormat(int defaultLen);
	int    readHeader(ulong ofs, WResHeaderInfo * hdr, ResPosition);
	int    findResource(uint rscID, uint rscType, long * pOffs = 0, long * pSz = 0);
	int    getSizeField(long *);
	long   getStreamPos();
	int    enumResources(uint rscType, uint * rscID, ulong * dwPos);
	//FILE * getStream() const { return Stream; }
	int    CheckDialogs(const char * pLogFileName); // @debug
	bool   Seek(long offs, int origin/*SEEK_SET || SEEK_CUR || SEEK_END*/);

	int    error;
private:
	int    _readHeader16(ulong ofs, WResHeaderInfo * hdr, ResPosition);
	int    _readHeader32(ulong ofs, WResHeaderInfo * hdr, ResPosition);
	SFile  F;
	//SString FileName;
	//FILE   * Stream;
	SVector * P_Index;
	int      HeaderType;
};

extern int (* getUserControl)(TVRez *, TDialog*);

int    LoadToolbar(TVRez *, uint tbType, uint tbID, ToolbarList *);
//
#endif // } __SLUI_H
