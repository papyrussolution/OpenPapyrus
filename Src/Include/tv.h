// TV.H
// Copyright (c) A.Sobolev 1996-2021
// @codepage UTF-8
//
#ifndef __TV_H
#define __TV_H

#include <tvdefs.h>
#include <commctrl.h>
#include <db.h>

class  TView;
class  TGroup;
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
//
// @v10.9.6 
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
		fFreeText             = 0x0002 // @v10.7.7 Текст в основной строке вводится свободно без вызова окна поиска
	};
//protected:
	long   Flags;
	bool   CtrlTextMode; // Если true, то при вызове функции TransmitData, данные в элемент управления будут
		// устанавливаться в соответствии с внутренним форматом этого элемента.
		// Иначе, функция будет считать что ей передали id и необходимо произвести поиск текста соотвествующего этому id.
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
	void   Clear();
	int    FASTCALL operator == (const KeyDownCommand & rS) const { return (State == rS.State && Code == rS.Code); }
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
	//    0 - ошибка (внутреннее состояние не изменилось)
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
	TPoint Mouse;
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
	TPoint PrevSize;   // Размеры окна до получения этого сообщения //
	TPoint NewSize;    // Новые размеры окна.
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
	TPoint Coord;
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
			void  * infoPtr;
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
	int  FASTCALL isCmd(uint cmd) const;
	int  FASTCALL isKeyDown(uint keyCode) const;
	int  FASTCALL isCtlEvent(uint ctlID) const;
	int  FASTCALL isCbSelected(uint ctlID) const;
	int  FASTCALL isClusterClk(uint ctlID) const;
	int  FASTCALL wasFocusChanged(uint ctlID) const;
	int  wasFocusChanged2(uint ctl01, uint ctl02) const;
	int  wasFocusChanged3(uint ctl01, uint ctl02, uint ctl03) const;
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
	int    IsEmpty() const;
	int    has(int cmd) const;
	void   enableAll();
	void   enableCmd(int cmd, int is_enable);
	void   enableCmd(const TCommandSet&, int is_enable);
	void   operator += (int cmd);
	void   operator -= (int cmd);
	void   operator += (const TCommandSet&);
	void   operator -= (const TCommandSet&);
	TCommandSet & operator &= (const TCommandSet&);
	TCommandSet & operator |= (const TCommandSet&);
	friend TCommandSet operator & (const TCommandSet&, const TCommandSet&);
	friend TCommandSet operator | (const TCommandSet&, const TCommandSet&);
	friend int operator == (const TCommandSet& tc1, const TCommandSet& tc2);
	friend int operator != (const TCommandSet& tc1, const TCommandSet& tc2);
private:
	int    loc(int);
	int    mask(int);

	uint32 cmds[64];
};

int operator != (const TCommandSet& tc1, const TCommandSet& tc2);

// regex: (virtual)*[ \t]+void[ \t]+handleEvent\([ \t]*TEvent[ \t]*&[^)]*\)
#define DECL_HANDLE_EVENT      virtual void __fastcall handleEvent(TEvent & event)
// regex: void[ \t]+([a-zA-Z0-9_]+)::handleEvent\([ \t]*TEvent[ \t]*&[^)]*\)
#define IMPL_HANDLE_EVENT(cls) void __fastcall cls::handleEvent(TEvent & event)
#define EVENT_BARRIER(f) if(!EventBarrier()) { f; EventBarrier(1); }

#define DECL_DIALOG_DATA(typ) typedef typ DlgDataType; DlgDataType Data // @v10.5.8
#define DECL_DIALOG_SETDTS() int setDTS(const DlgDataType * pData)      // @v10.5.8
#define DECL_DIALOG_GETDTS() int getDTS(DlgDataType * pData)            // @v10.5.8
#define IMPL_DIALOG_SETDTS(cls) int cls::setDTS(const DlgDataType * pData)      // @v10.9.4
#define IMPL_DIALOG_GETDTS(cls) int cls::getDTS(DlgDataType * pData)            // @v10.9.4
//
// Descr: Контейнер для GDI-объектов. Удобен тем, что "бесконечное" количество предварительно
// созданных хандлеров можно затолкать в этот контейнер, а при рисовании манипулировать
// перечислением, элемент которого передается в функцию SPaintToolBox::Get() для получения //
// требуемого GDI-хандлера.
// Элементы в контейнере хранятся упорядоченно, по-этому, время извлечения очень мало.
// При разрушении контейнера все GDI-объекты, занесенные в него разрушаются.
//
#ifdef _WIN32_WCE
#define PS_DOT           3
#define PS_DASHDOT       4
#define PS_DASHDOTDOT    5
#define PS_INSIDEFRAME   7

#define BS_HATCHED       3
#define BS_DIBPATTERN    5
#define BS_DIBPATTERNPT  6

#define HS_HORIZONTAL    1
#define HS_VERTICAL      2
#define HS_FDIAGONAL     3
#define HS_BDIAGONAL     4
#define HS_CROSS         5
#define HS_DIAGCROSS     6
#endif // } _WIN32_WCE
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

	SFontDescr(const char * pFace, int size, int flags);
	void   Init();
	int    FASTCALL IsEqual(const SFontDescr & rS) const;
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    ToStr(SString & rBuf, long fmt) const;
	int    FASTCALL FromStr(const char *);
	int    FASTCALL SetLogFont(const LOGFONTA * pLf);
	int    FASTCALL SetLogFont(const LOGFONTW * pLf);
	LOGFONTA * FASTCALL MakeLogFont(LOGFONTA * pLf) const;
	LOGFONTW * FASTCALL MakeLogFont(LOGFONTW * pLf) const;

	int16  Size;        // @anchor Логический размер шрифта в пикселях
	int16  Flags;       // @flags SFontDescr::fXXX
	float  Weight;      // Толщина шрифта. 0.0f - не важно, 1.0f - нормальный, 2.0f - максимально толстый.
	uint8  CharSet;     // Набор символов XXX_CHARSET (win32)
	uint8  Reserve[15];
	SString Face;       // @anchor
private:
	int    FASTCALL Helper_SetLogFont(const void * pLf);
	int    FASTCALL Helper_MakeLogFont(void * pLf) const;
};
//
// Descr: Определитель текстового параграфа
//
struct SParaDescr { // @persistent
	SParaDescr();
	int    FASTCALL IsEqual(const SParaDescr &) const;
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    GetJustif() const;

	enum {
		fJustRight   = 0x0001, // Выравнивать строки по правому краю
		fJustCenter  = 0x0002  // Выравнивать строки по центру
	};
	TPoint LuIndent;    // Левый и верхний отступы
	TPoint RlIndent;    // Правый и нижний отступы
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
	FPoint Sz;      // Specifies the size of the smallest rectangle that completely encloses the glyph (its black box).
	FPoint Org;     // Specifies coordinates of the upper left corner of the smallest rectangle that completely encloses the glyph.
	FPoint Advance; // Specifies the distance from the origin of the current character cell to the origin of the next character cell.
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
		psDot         = PS_DOT,        // The pen is dotted. This style is valid only when the pen width is one or less in device units.
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
		bsNull         = BS_NULL,        // Hollow brush
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
		int    FASTCALL IsEqual(const Pen & rS) const;
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
		int    IsDashed() const;
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
		int    IsSimple() const;
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
		int    FASTCALL operator == (const Brush &) const;
		void   FASTCALL Copy(const Brush & rS);
		int    FASTCALL IsEqual(const Brush & rS) const;
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
		int    operator == (const Gradient &) const;
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
		int    FASTCALL IsEqual(const CStyle & rS) const;
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
	int    CreateFont(const char * pFace, int height, int flags);
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
	int    FASTCALL CreateInnerHandle(SDrawContext & rCtx);
	int    FASTCALL DestroyInnerHandle();
private:
	//
	// Descr: После вызова этой функции экземпляр теряет право владения сложными
	//   объектами, находящимися в его составе. То есть, метод Destroy не
	//   будет разрушать объекты, на которые ссылаются идентификаторы или
	//   освобождать область памяти, на которую ссылается SPaintObj::H
	//
	void   ResetOwnership();
	int    FASTCALL ProcessInnerHandle(SDrawContext * pCtx, int create);

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
	SPaintObj::Font * GetFont(SDrawContext & rCtx, int fontIdent);
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
		fWrap          = 0x0004, //
		fNoClip        = 0x0008, // Не ограничивать отрисовку границами
		fOneLine       = 0x0010, // Выводить текст в одну строку (не переносить и игнорировать переводы каретки)
		fPrecBkg       = 0x0020, // Закрашивать фон строго по тексту (в противном случае закрашивается вся область,
			// отведенная под вывод текста).
		fVCenter       = 0x0040, // Вертикальное центрирование
		fVBottom       = 0x0080  // Вертикальное выравнивание по нижней границе
			// @#(fVCenter^fVBottom)
	};
	struct Item {
		Item(FPoint p, const SGlyph * pGlyph, uint16 flags) : P(p), GlyphIdx(pGlyph ? pGlyph->Idx : -1), Flags(flags)
		{
		}
		enum {
			fUnderscore = 0x0001
		};
		int16  GlyphIdx; // -1 - не выводить символ
		uint16 Flags;    // Специальные опции отображения символа
		FPoint P;
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
	int    Copy(const STextLayout & rS);
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
	FPoint EndPoint;          // @*STextLayout::Arrange Точка, до которой простирается собственно текст.
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
	struct Surface {
		Surface();
		void * HCtx;  // Контекст устройства // @v10.3.1 uint32-->void *
		SImageBuffer * P_Img;
	};
	struct Capability {
		Capability();
		TPoint SizePt; // Размер устройства в точках
		FPoint SizeMm; // Размер устройства в миллиметрах
		FPoint PtPerInch; // Количество точек на дюйм
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
		FPoint MaxAdvance; // X - the maximum distance in the X direction that the the origin is advanced
			// for any glyph in the font.
			// Y - the maximum distance in the Y direction that the the origin is advanced for any glyph
			// in the font. this will be zero for normal fonts used for horizontal writing.
			// (The scripts of East Asia are sometimes written vertically.)
	};
	struct TextExt {
		RPoint Bearing; // X - the horizontal distance from the origin to the leftmost part of the glyphs as drawn.
			// Positive if the glyphs lie entirely to the right of the origin.
			// Y - the vertical distance from the origin to the topmost part of the glyphs as drawn.
			// Positive only if the glyphs lie completely below the origin; will usually be negative.
		RPoint Size;    // Size of the glyphs as drawn
		RPoint Advance; // X - distance to advance in the X direction after drawing these glyphs
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
	FPoint GetCurPoint();
	void   FASTCALL MoveTo(FPoint to);
	void   FASTCALL Line(FPoint to);
	void   FASTCALL LineV(float yTo);
	void   FASTCALL LineH(float xTo);
	void   FASTCALL Rect(const FRect & rRect);
	void   FASTCALL Rect(const TRect & rRect);
	void   RoundRect(const FRect &, float radius);
	int    FASTCALL Ellipse(const FRect & rRect);
	void   Arc(FPoint center, float radius, float startAngleRad, float endAngleRad);
	void   Bezier(FPoint middle1, FPoint middle2, FPoint end);
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
	TPoint GetTextSize(const char * pStr);
		// @>>BOOL GetTextExtentPoint32(HDC hdc, LPCTSTR lpString, int cbString, LPSIZE lpSize);
	int    TextOut(TPoint p, const char * pText);
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
	int    Implement_ArcSvg(FPoint radius, float xAxisRotation, int large_arc_flag, int sweep_flag, FPoint toPoint);
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
	int    FASTCALL MoveTo(TPoint);
	int    FASTCALL Line(TPoint);
	void   LineVert(int x, int yFrom, int yTo);
	void   LineHorz(int xFrom, int xTo, int y);
	void   SetBkTranparent();
	int    FASTCALL SetBkColor(COLORREF);
	int    FASTCALL SetTextColor(COLORREF);
	int    FASTCALL Rectangle(const TRect &);
	int    RoundRect(const TRect &, const TPoint & rRoundPt);
	int    FillRect(const TRect & rRect, HBRUSH brush);
	TPoint FASTCALL GetTextSize(const char * pStr);
		// @>>BOOL GetTextExtentPoint32(HDC hdc, LPCTSTR lpString, int cbString, LPSIZE lpSize);
	int    TextOut_(TPoint p, const char * pText);
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

class TBitmapHash {
public:
	TBitmapHash();
	~TBitmapHash();
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
	//static void * message(TView * pReceiver, uint what, uint command);
	//static void * message(TView * pReceiver, uint what, uint command, void * pInfoPtr);
	static void * FASTCALL messageCommand(TView * pReceiver, uint command);
	static void * FASTCALL messageCommand(TView * pReceiver, uint command, void * pInfoPtr);
	static void * FASTCALL messageBroadcast(TView * pReceiver, uint command);
	static void * FASTCALL messageBroadcast(TView * pReceiver, uint command, void * pInfoPtr);
	static void * FASTCALL messageKeyDown(TView * pReceiver, uint keyCode);
	static HFONT setFont(HWND hWnd, const char * pFontName, int height);
	//
	// Descr: Создает экземляр шрифта по описанию rFd.
	// Note: Эту функцию следует использовать везде вместо wingdi-функции CreateFontIndirect().
	//
	static void * CreateFont(const SFontDescr & rFd);
	static void * SetWindowProp(HWND hWnd, int propIndex, void * ptr);
	static void * FASTCALL SetWindowUserData(HWND hWnd, void * ptr);
	static long SetWindowProp(HWND hWnd, int propIndex, long value);
	static void * FASTCALL GetWindowProp(HWND hWnd, int propIndex);
	static void * FASTCALL GetWindowUserData(HWND hWnd);
	static long FASTCALL GetWindowStyle(HWND hWnd);
	static long FASTCALL GetWindowExStyle(HWND hWnd);
	static int  FASTCALL SGetWindowClassName(HWND hWnd, SString & rBuf);
	static int  FASTCALL SGetWindowText(HWND hWnd, SString & rBuf);
	static int  FASTCALL SSetWindowText(HWND hWnd, const char * pText);
	//
	// Descr: Специализированный метод, вызывающий для всех элементов группы TGroup
	//   метод P_WordSelBlk->OnAcceptInput()
	//
	static void CallOnAcceptInputForWordSelExtraBlocks(TGroup * pG);
	//
	// Descr: перебирает дочерние окна родительского окна hWnd и
	//   выполняет подстановку шаблонизированных текстовых строк
	//   вида "@textident" или "@{textident}"
	//   Применяется для окон, созданных в обход штатной загрузки диалогов TDialog
	//
	static void FASTCALL PreprocessWindowCtrlText(HWND hWnd);

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

	explicit TView(const TRect & bounds);
	TView();
	virtual ~TView();
	virtual int    FASTCALL valid(ushort command);
	//
	// Descr: Метод, используемый для передачи (извлечения) данных в (из)
	//   экземпляр объекта. Кроме того, метод реализует возрат размера данных объекта.
	//
	virtual int    TransmitData(int dir, void * pData);
	virtual void   setState(uint aState, bool enable);
	virtual int    handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	DECL_HANDLE_EVENT;
	int    commandEnabled(ushort command) const;
	void   FASTCALL enableCommands(const TCommandSet & commands, int is_enable);
	void   FASTCALL enableCommand(ushort command, int is_enable);
	void   getCommands(TCommandSet & commands) const;
	void   setCommands(const TCommandSet & commands);
	void   setBounds(const TRect & bounds);
	void   changeBounds(const TRect & bounds);
	uint   getHelpCtx();
	TView & SetId(uint id);
	uint   GetId() const;
	int    FASTCALL TestId(uint id) const;
	//
	// Descr: Возвращает !0 если поле состояние объекта содержит хотя бы один
	//   из флагов, установленных в параметре s.
	//
	int    FASTCALL IsInState(uint s) const;
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
	void   Show(int doShow);
	void   FASTCALL clearEvent(TEvent & event);
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
	int    IsConsistent() const;
	int    FASTCALL IsSubSign(uint) const;
	uint   GetSubSign() const { return SubSign; }
	int    GetEndModalCmd() const { return EndModalCmd; }
	//
	// Descr: Должна вызываться оконными (диалоговыми) функциями порожденных классов
	//   для восстановления оригинальной оконной функции
	//
	int    OnDestroy(HWND);
	void   SendToParent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void   SetWordSelBlock(WordSel_ExtraBlock *);
	int    HasWordSelector() const { return BIN(P_WordSelBlk); }
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
		int    operator !() const;
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
	TPoint ViewSize;
	TPoint ViewOrigin;
	uint32 ViewOptions;
	TView  * P_Next;
	TGroup * P_Owner;
	HWND   Parent;
	WNDPROC PrevWindowProc;
protected:
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

class TGroup : public TView {
public:
	explicit TGroup(const TRect & bounds);
	~TGroup();
	DECL_HANDLE_EVENT;
	virtual void   Insert_(TView *p);
	virtual void   setState(uint aState, bool enable);
	virtual int    TransmitData(int dir, void * pData);
	virtual int    FASTCALL valid(ushort command);
	ushort FASTCALL execView(TWindow * p);
	void   insertView(TView * p, TView * pTarget);
	void   FASTCALL remove(TView * p);
	void   removeView(TView * p);
	void   selectNext();
	void   forEach(void (*func)(TView *, void *), void *args);
	void   insertBefore(TView *p, TView *Target);
	TView * GetFirstView() const;
	TView * GetCurrentView() const { return P_Current; }
	TView * GetLastView() const { return P_Last; }
	void   redraw();
	uint   GetCurrId() const;
	int    FASTCALL IsCurrentView(const TView * pV) const;
	int    FASTCALL isCurrCtlID(uint ctlID) const;
	void   SetCurrentView(TView * p, selectMode mode);
protected:
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

class TWindow : public TGroup {
public:
	static int IsMDIClientWindow(HWND);

	TWindow(const TRect& bounds, const char * pTitle, short aNumber);
	~TWindow();
	void   endModal(ushort command);
	void * messageToCtrl(ushort ctl, ushort command, void * ptr);
	TView * FASTCALL getCtrlView(ushort ctl);
	TView * FASTCALL getCtrlByHandle(HWND h);
	HWND   H() const { return this ? HW : static_cast<HWND>(0); }
	HWND   FASTCALL getCtrlHandle(ushort ctlID);
	void   FASTCALL setCtrlReadOnly(ushort ctlID, int set);
	void   FASTCALL disableCtrl(ushort ctl, int toDisable);
	void   __cdecl  disableCtrls(int toDisable, ...);
	void   FASTCALL selectCtrl(ushort ctl);
	int    selectButton(ushort cmd);
	void   setCtrlOption(ushort id, ushort flag, int s);
	int    destroyCtrl(uint ctl);
	//
	// Функции setCtrlData и getCtrlData возвращают !0 если существует
	// управляющий элемент с ид. ctl и 0 в противном случае.
	//
	int    FASTCALL setCtrlData(ushort ctl, void *);
	int    FASTCALL getCtrlData(ushort ctl, void *);
	uint16 FASTCALL getCtrlUInt16(uint ctlID);
	long   FASTCALL getCtrlLong(uint ctl);
	double FASTCALL getCtrlReal(uint ctl);
	int    FASTCALL getCtrlString(uint ctlID, SString &);
	LDATE  FASTCALL getCtrlDate(uint ctlID);
	LTIME  FASTCALL getCtrlTime(uint ctlID);
	int    getCtrlDatetime(uint dtCtlID, uint tmCtlID, LDATETIME & rDtm);
	int    FASTCALL setCtrlUInt16(uint ctlID, int s);
	int    FASTCALL setCtrlLong(uint ctlID, long);
	int    FASTCALL setCtrlReal(uint ctlID, double);
	int    FASTCALL setCtrlString(uint ctlID, const SString &);
	int    FASTCALL setCtrlDate(uint ctlID, LDATE val);
	int    FASTCALL setCtrlTime(uint ctlID, LTIME val);
	int    setCtrlDatetime(uint dtCtlID, uint tmCtlID, LDATETIME dtm);
	int    setCtrlDatetime(uint dtCtlID, uint tmCtlID, LDATE dt, LTIME tm);
	//
	// Descr: Высокоуровневая функция, устанавливающая опцию option для SmartListBox::def
	//   с идентификатором ctlID (вызывает ListBoxDef::SetOption(option, 1)
	//
	int    setSmartListBoxOption(uint ctlID, uint option);
	void   FASTCALL drawCtrl(ushort ctlID);
	void   showCtrl(ushort ctl, int s /* 1 - show, 0 - hide */);
	void   showButton(uint cmd, int s /* 1 - show, 0 - hide */);
	int    setButtonText(uint cmd, const char * pText);
	int    setButtonBitmap(uint cmd, uint bmpID);
	void   FASTCALL setTitle(const char *);
	void   FASTCALL setOrgTitle(const char *);
	void   FASTCALL setSubTitle(const char *);
	int    setStaticText(ushort ctl, const char *);
	int    getStaticText(ushort ctl, SString &);
	const  SString & getTitle() const;
	void   close();
	DECL_HANDLE_EVENT;
	virtual void setState(uint aState, bool enable);
	int    translateKeyCode(ushort keyCode, uint * pCmd) const;
	void   setupToolbar(const ToolbarList * pToolBar);
	int    AddLocalMenuItem(uint ctrlId, uint buttonId, long keyCode, const char * pText);
	HWND   showToolbar();
	void   showLocalMenu();
	TRect  getClientRect() const;
	TRect  getRect() const;
	int    invalidateRect(const TRect &, int erase);
	int    invalidateRegion(const SRegion & rRgn, int erase);
	void   FASTCALL invalidateAll(int erase);
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
	//
	HWND   PrevInStack;
	HWND   HW; // hWnd;
	ToolbarList Toolbar;
private:
	void   FASTCALL Helper_SetTitle(const char *, int setOrgTitle);
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
	LocalMenuPool * P_Lmp;
};
//
// @construction {
//
#if 0 // @v10.9.12 (depricated) {
class SRectLayout {
public:
	//
	// Если не задан флаг dfOpp, то координата отсчитывается от левого верхнего угла.
	// Если задан флаг dfOpp, то координата отсчитывается от правого нижнего угла.
	//
	enum {
		dfAbs     = 0x0001, // Координата задана в абсолютных единицах
		dfRel     = 0x0002, // Координата задана в долях от общего размера контейнера. Значение кодируется как число с фиксированной точкой (4 знака)
		dfGravity = 0x0004, // Координата притягивается к соответствующей границе контейнера.
		dfOpp     = 0x0008  // Координата отсчитывается от противоположной стороны контейнера.
	};
	enum {
		inoOverlap = 1,
		inoVStack,
		inoHStack
	};
	struct Dim {
		void   Set(int v, int f);
		int16  Val;
		int16  Flags; // SRectLayout::dfXXX
	};
	struct Item {
		DECL_INVARIANT_C();

		Item();
		Item & SetLeft(int size, int pct);
		Item & SetRight(int size, int pct);
		Item & SetTop(int size, int pct);
		Item & SetBottom(int size, int pct);
		Item & SetCenter();

		Dim    Left;
		Dim    Top;
		Dim    Right;
		Dim    Bottom;
		int16  EmptyWidth;  // Ширина пустого элемента (-1 - та же, что и заполненного)
		int16  EmptyHeight; // Высота пустого элемента (-1 - та же, что и заполненного)
		int    InnerOrder;
	};

	SRectLayout();
	~SRectLayout();
	int    Add(long id, const Item &);
	int    InsertWindow(long itemId, TView * pView, int minWidth, int minHeight);
	int    RemoveWindow(long winId);
	int    GetWindowBounds(long winId, TRect & rBounds);
	int    SetContainerBounds(const TRect &);
	int    Arrange();
	int    GetItemBounds(long id, TRect & rBounds);
	int    Locate(TPoint p, uint * pItemPos) const;
private:
	struct RItem {
		enum {
			stNotEmpty = 0x0001
		};
		long   Id;
		Dim    Left;
		Dim    Top;
		Dim    Right;
		Dim    Bottom;
		int16  EmptyWidth;  // Ширина пустого элемента (-1 - та же, что и заполненного)
		int16  EmptyHeight; // Высота пустого элемента (-1 - та же, что и заполненного)
		int    InnerOrder;
		long   State;
		TRect  Bounds;
	};
	struct WItem {
		long   ItemId;
		TView * P_View;       // @notowned
		int16  MinWidth;
		int16  MinHeight;
		TRect  Bounds;
	};

	int    IsEmpty(long itemId) const;
	int    CalcCoord(Dim dim, int containerLow, int containerUpp, int gravitySide) const;

	TSVector <RItem> List;
	TSVector <WItem> WinList;
	TRect ContainerBounds;
};
#endif // } 0 @v10.9.12 (depricated) {
//
//
//
struct AbstractLayoutBlock { // @persistent
	enum {
		fContainerRow             = 0x0001, // Контейнер выстраивает дочерние элементы по строкам (ось X)
		fContainerCol             = 0x0002, // Контейнер выстраивает дочерние элементы по колонкам (ось Y)
			// if fContainerRow && fContainerCol, then no direction
		fContainerReverseDir      = 0x0004, // if horizontal then right-to-left, if vertical then bottom-to-top
		fContainerWrap            = 0x0008, // Если все элементы не вмещаются в один ряд, то контейнер переносит последующие элементы на следующий ряд.
		fContainerWrapReverse     = 0x0010, // ignored if !(Flags & fWrap)
		fEntryPositionAbsolute    = 0x0020, // else Relative
		fNominalDefL              = 0x0040, // Определена номинальная граница LEFT элемента   (Nominal.a.X)
		fNominalDefT              = 0x0080, // Определена номинальная граница TOP элемента    (Nominal.a.Y)
		fNominalDefR              = 0x0100, // Определена номинальная граница RIGHT элемента  (Nominal.b.X)
		fNominalDefB              = 0x0200, // Определена номинальная граница BOTTOM элемента (Nominal.b.Y)
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
		szByContainer =  3  // Размер определяется по размеру контейнера
	};
	//
	// Descr: Возвращает специальную 4-байтную сигнатуру, идентифицирующую объект в потоках сериализации.
	//
	static uint32 GetSerializeSignature();
	AbstractLayoutBlock();
	int    FASTCALL operator == (const AbstractLayoutBlock & rS) const;
	int    FASTCALL operator != (const AbstractLayoutBlock & rS) const;
	AbstractLayoutBlock & SetDefault();
	int    FASTCALL IsEqual(const AbstractLayoutBlock & rS) const;
	//
	// Descr: Проверяет параметры (возможно, не все) блока на непротиворечивость.
	// Returns:
	//   !0 - блок находится в консистентном состоянии
	//    0 - некоторые параметры блока противоречивы
	//
	int    Validate() const;
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
	FPoint CalcEffectiveSizeXY(float containerSizeX, float containerSizeY) const;
	void   SetFixedSizeX(float s);
	void   SetFixedSizeY(float s);
	void   SetVariableSizeX(uint var/* szXXX */, float s);
	void   SetVariableSizeY(uint var/* szXXX */, float s);
	void   SetFixedSize(const TRect & rR);
	int    GetContainerDirection() const; // returns DIREC_HORZ || DIREC_VERT || DIREC_UNKN
	void   SetContainerDirection(int direc /*DIREC_XXX*/);
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
	// Descr: Возвращает зону притяжения элемента (AbstractLayoutBlock::areaXXX) в зависимости от значений
	//   полей {GravityX, GravityY}
	//
	int    GetVArea() const;
	//
	// Descr: Устанавливает атрибуты GravityX и GravityY в зависимости от параметра
	//   area.
	// Returns:
	//   >0 - значения GravityX и GravityY успешно установлены. При этом они изменились.
	//   <0 - значения GravityX и GravityY успешно установлены, но ничего при этом не изменилось (они такими же и были).
	//    0 - ошибка (аргумент area не валиден либо что-то не так с внутренним состоянием объекта).
	//
	int    SetVArea(int area);
	static int RestrictVArea(int restrictingArea, const FRect & rRestrictingRect, int restrictedArea, FRect & rRestrictedRect);

	uint32 Flags;          // @flags fXXX
	uint16 SzX;            // AbstractLayoutBlock::szXXX Опции расчета размера по оси X
	uint16 SzY;            // AbstractLayoutBlock::szXXX Опции расчета размера по оси Y
	uint16 JustifyContent; // {alignStart}   AbstractLayoutBlock::alignXXX Выравнивание внутренних элементов вдоль основной оси
	uint16 AlignContent;   // {alignStretch} AbstractLayoutBlock::alignXXX Выравнивание внутренних элементов по кросс-оси
	uint16 AlignItems;     // {alignStretch} AbstractLayoutBlock::alignXXX
	uint16 AlignSelf;      // {alignAuto}    AbstractLayoutBlock::alignXXX
	uint16 GravityX;       // Gravity of this entry by X-axis. 0 || SIDE_LEFT || SIDE_RIGHT || SIDE_CENTER
	uint16 GravityY;       // Gravity of this entry by Y-axis. 0 || SIDE_TOP  || SIDE_BOTTOM || SIDE_CENTER 
	int32  Order;          // Порядковый номер элемента в линейном ряду потомков одного родителя //
	FRect  Nominal;        // Номинальные границы элемента. Заданы или нет определяется флагами fNominalDefL, fNominalDefT, fNominalDefR, fNominalDefB
	FPoint Size;           // Номинальный размер элемента. Если SzX != szFixed, то Size.X игнорируется, аналогично, если SzY != szFixed, то Size.Y игнорируется
	FRect  Padding;        // { 0.0f, 0.0f, 0.0f, 0.0f } Внешние поля элемента
	FRect  Margin;         // { 0.0f, 0.0f, 0.0f, 0.0f } Внутренние поля контейнера
	float  GrowFactor;     // Доля от размера всех элементов контейнера по продольной оси (определяемой флагами fContainerRow и fContainerCol)
	float  ShrinkFactor;   //
	float  Basis;          //
	float  AspectRatio;    // {0.0} Отношение высоты к ширине. Используется в случае, если одна из размерностей не определена
private:
	int    ParseSizeStr(const SString & rStr, float & rS) const;
};

class LayoutFlexItem {
	friend class LayoutFlexProcessor;
public:
	struct Result { // @persistent @size=24
		enum {
			fNotFit         = 0x0001, // Элемент не уместился в контейнер
			fDegradedWidth  = 0x0002, // Ширина элемента деградировала (меньше или равна 0)
			fDegradedHeight = 0x0004  // Высота элемента деградировала (меньше или равна 0)
		};
		Result();
		operator FRect() const;
		Result & CopyWithOffset(const Result & rS, float offsX, float offsY);
		float  Frame[4];
		uint32 Flags;
		uint32 Reserve;
	};
	struct IndexEntry {
		IndexEntry();
		uint32 ItemIdx; // Индекс элемента в TSCollection::this
		uint32 HglIdx;  // Индекс[1..] элемента в P_HgL. Если SVectorBase::GetCount(P_HgL) == 0, то 0
		Result R;       // Результат размещения
	};
	// size[0] == width, size[1] == height
	typedef void (__stdcall * FlexSelfSizingProc)(const LayoutFlexItem * pItem, float size[2]);
	typedef void (__stdcall * FlexSetupProc)(LayoutFlexItem * pItem, /*float size[4]*/const LayoutFlexItem::Result & rR);

	static void * GetManagedPtr(LayoutFlexItem * pItem);
	static void * GetParentsManagedPtr(LayoutFlexItem * pItem);

	LayoutFlexItem();
	~LayoutFlexItem();
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
	LayoutFlexItem * GetChild(uint idx);
	const  LayoutFlexItem * GetChildC(uint idx) const;
	void   SetCallbacks(FlexSelfSizingProc selfSizingCb, FlexSetupProc setupCb, void * managedPtr);
	int    SetLayoutBlock(const AbstractLayoutBlock & rAlb);
	AbstractLayoutBlock & GetLayoutBlock() { return ALB; }
	const AbstractLayoutBlock & GetLayoutBlockC() const { return ALB; }
	LayoutFlexItem * GetParent() { return P_Parent; }

	struct Param {
		enum {
			fPaginate                = 0x0001, // Рассчитывать раскладку по страницам.
			fStopOnFirstUnfittedItem = 0x0002  // Остановить расчет на первом невместившемся элементе
		};
		Param() : Flags(0), FirstItemIndex(0), ForceWidth(0.0f), ForceHeight(0.0f)
		{
		}
		uint   Flags;
		uint   FirstItemIndex; // Индекс элемента [0..], с которого начинать расчет
		float  ForceWidth;
		float  ForceHeight;
	};
	struct PagingResult {
		PagingResult() : LineCount(0), PageCount(0), LastFittedItemIndex(0)
		{
		}
		uint   LineCount;
		uint   PageCount;
		uint   LastFittedItemIndex;
	};
	int    Evaluate(const Param * pP, PagingResult * pPgR);
	LayoutFlexItem * InsertItem();
	void   DeleteItem(uint idx);
	void   DeleteAllItems();
	int    GetOrder() const;
	void   SetOrder(int o);
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
	// Descr: Рассчитывает минимальный прямоугольник, охватывающий все дочерние элементы
	//   контейнера. Размеры самого контейнера в расчет не принимаются.
	// Returns:
	//   1 - все дочерние элементы имеют определенный размер
	//  -1 - контейнер не имеет элементов 
	//  -2 - только часть дочерних элементов имеет определенный размер
	//   0 - ошибка (черт его знает, что там еще может произойти)
	//
	int    GetInnerCombinedFrame(FRect * pResult) const;
	//
	// Descr: Возвращает корневой элемент дерева, компонентом которого является this.
	//
	LayoutFlexItem * GetRoot();
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
		HomogeneousArray() : VariableFactor(vfNone)
		{
		}
		uint   VariableFactor;
	};

	int    InitHomogeneousArray(uint variableFactor /* HomogeneousArray::vfXXX */);
	int    AddHomogeneousEntry(long id, float vf);
protected:
	void   UpdateShouldOrderChildren();
	void   DoLayout(const Param & rP) const;
	void   DoFloatLayout(const Param & rP);
	void   DoLayoutChildren(uint childBeginIdx, uint childEndIdx, uint childrenCount, /*LayoutFlexProcessor*/void * pLayout) const;
	//
	// Descr: Завершает обработку искусственного элемента pCurrentLayout, устанавливает координаты его дочерних элементов
	//   с поправкой на rOffs и разрушает pCurrentLayout.
	//
	void   Helper_CommitInnerFloatLayout(LayoutFlexItem * pCurrentLayout, const FPoint & rOffs) const;
	/*flex_align*/int FASTCALL GetChildAlign(const LayoutFlexItem & rChild) const;
private:
	class IterIndex : public TSVector <IndexEntry> {
	public:
		IterIndex();
		LayoutFlexItem * GetHomogeneousItem(const LayoutFlexItem * pBaseItem, uint hgeIdx) const;
		void   ReleaseHomogeneousItem(LayoutFlexItem *) const;
	private:
		mutable TSCollection <LayoutFlexItem> HomogeneousEntryPool;
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
	//   -- пока имеет место путаница насчет того какие поля LayoutFlexItem меняются функциями расчета, а 
	//     какие нет. Потому я в некотором замешательстве.
	//
	void   MakeIndex(IterIndex & rIndex) const;
	//
	// Descr: Получает указатель на элемент по позиции idxPos в индексе rIndex.
	//   Если элемент является гомоморфной коллекцией, то возвращается указатель на суррогатный экземпляр.
	// Returns:
	//   0 - error. Скорее всего, инвалидный индекс.
	//
	const  LayoutFlexItem * GetChildByIndex(const IterIndex & rIndex, uint idxPos) const;
	int    CommitChildResult(const IterIndex & rIndex, uint idxPos, const LayoutFlexItem * pItem);
	//
	// Descr: Вызывается после завершения расчета элемента
	//   Вызывает DoLayout(R.Frame[2], R.Frame[3])
	//
	void   Commit_() const;
	bool   LayoutAlign(/*flex_align*/int align, float flexDim, uint childrenCount, float * pPos, float * pSpacing, bool stretchAllowed) const;

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
		stHomogeneousItem     = 0x0002  // Элемент является виртуальным экземпляром, сформированным по шаблону из HomogeneousArray основного элемента
	};
	AbstractLayoutBlock ALB;
	void * managed_ptr; // NULL // An item can store an arbitrary pointer, which can be used by bindings as the address of a managed object.
	// An item can provide a self_sizing callback function that will be called
	// during layout and which can customize the dimensions (width and height) of the item.
	FlexSelfSizingProc CbSelfSizing; // NULL
	FlexSetupProc CbSetup; // NULL
	LayoutFlexItem * P_Parent;
	uint   State;
	const  LayoutFlexItem * P_Link; // @transient При сложных схемах построения формируются искусственные лейауты, получающие
		// в этом поле ссылку на порождающий реальный элемент. 
	TSCollection <LayoutFlexItem> * P_Children;
	HomogeneousArray * P_HgL;
	mutable Result R;
};
/*
length-unit: % | mm | m | cm
length-unit-optional: length-unit | ;
measured-value: number unit-optional
range: measured-value..measured-value

box (x, y, x2, y2) // 4 values
box (width, height) // 2 values
box (width, undefined) // 2 values
box (50%, 10) // 2 values
box (40%..50%, 10..30) // 2 values
box (x, y, (width, height))

layout abc rowreverse wrap {
}
*/
//
//
//
struct TScrollBlock {
	TScrollBlock();
	int    SetupWindow(HWND hWnd) const;
	int    MoveToEdge(int side);
	int    Move(int side, int delta);
	int    Set(int x, int y);

	IntRange Rx;        // Диапазон горизонтального скроллирования //
	IntRange Ry;        // Диапазон вертикального скроллирования   //
	int    ScX;         // Горизонтальная позиция скроллера        //
	int    ScY;         // Вертикальная позиция скроллера          //
};
//
//
//
class TWindowBase : public TWindow {
public:
	static int RegWindowClass(int iconId);
	//
	// Descr: Структура, указатель на которую передается с сообщением cmInit
	//   (только для экземпляров, порожденных от TWindowBase).
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
	//
	// Опции Capability
	//
	enum {
		wbcDrawBuffer      = 0x0001, // Окно использует буферизованную перерисовку
		wbcLocalDragClient = 0x0002  // Окно является клиентом локального (в рамках процесса) обмена Drag'n'Drop
	};
	//
	// Descr: Опции функции TWindowBase::Create
	//
	enum {
		// @# coChild ^ coPopup
		coChild   = 0x0001,
		coPopup   = 0x0002,
		coMDI     = 0x0004,
		coScX     = 0x0008, // Окно создавать с горизонтальным скроллером
		coScY     = 0x0010, // Окно создавать с вертикальным скроллером
		coScXY    = (coScX|coScY),
		coMaxSize = 0x0020  // Окно создавать с максимальными размерами, допускаемыми родительским окном
	};

	~TWindowBase();
	int    Create(void * hParentWnd, long createOptions);
	int    AddChild(TWindowBase * pChildWindow, long createOptions, long zone);
	int    AddChildWithLayout(TWindowBase * pChildWindow, long createOptions, void * pLayout); // @v10.9.3
	//
	// Descr: Возвращает абстрактный указатель на объект LAYOUT, ассоциированный с окном this.
	// Returns:
	//   !0 - экземляр LAYOUT, ассоциированный с окном
	//    0 - с окном не ассоциирован объект LAYOUT
	//
	void * GetLayout();
protected:
	//
	// Descr: Присваивает экземаляру this элемент LAYOUT, на который указывает
	//   абстрактный указатель pLayout.
	//
	void   SetupLayoutItem(void * pLayout);
protected:
	static void __stdcall SetupLayoutItemFrame(LayoutFlexItem * pItem, const LayoutFlexItem::Result & rR); // @v10.9.3
	static void Helper_Finalize(HWND hWnd, TBaseBrowserWindow * pView); // @v10.9.11
	TWindowBase(LPCTSTR pWndClsName, int capability);
	DECL_HANDLE_EVENT;
	void   SetDefaultCursor();

	LayoutFlexItem * P_Lfc; // @v10.9.3 @construction
	SPaintToolBox Tb;
	TScrollBlock Sb;
private:
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
	int    MakeMouseEvent(uint msg, WPARAM wParam, LPARAM lParam, MouseEvent & rMe);
	void   RegisterMouseTracking(int force);

	const  SString ClsName;     // Window class name
	enum {
		wbsMDI                  = 0x0001,
		wbsUserSettingsChanged  = 0x0002,
		wbsMouseTrackRegistered = 0x0004
	};
	long   WbState;
	long   WbCapability;
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
	TPoint UlGap;      // Зазор между левой и верхней границами области и объектами
	TPoint LrGap;      // Зазор между правой и нижней границами области и объектами
	TPoint InnerGap;   // Зазор между объектами
	TPoint ForceSize;  // @unused Изменяет размер объектов до ForceSize. Если ForceSize.x <= 0,
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
		TPoint FigSize;       	 // Начальный размер фигуры инструмента.
		TPoint PicSize;       	 // Если !isZero() то переопределяет TWhatmanToolArray::Param::PicSize
		long   Flags;         	 // @flags
		SColor ReplacedColor; 	 // Цвет, который должен замещаться на какой-либо внешний цвет. Если ReplacedColor.IsEmpty(), то замещаемый цвет не определен.
		AbstractLayoutBlock Alb; // @v10.9.10
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
		TPoint PicSize;       // Размер иконки по умолчанию.
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
		TPoint FigSize;        	 // Исходный размер фигуры при размещении на ватмане
		TPoint PicSize;        	 // Размер иконки
		int32  Flags;          	 // @flags
		uint32 ExtDataP;       	 // Позиция дополнительных данных в буфере Pool (в кодировке MIME64)
		uint32 Id;             	 // Целочисленный идентификатор элемента
		SColor ReplacedColor;  	 //
		AbstractLayoutBlock Alb; // @v10.9.10
	};
	uint32 SrcFileVer;  // @transient Версия формата хранения файла, из которого был загружен данный экземпляр объекта
	uint32 SymbP;
	uint32 TextP;
	uint32 FileP;   // @transient Имя файла, из которого был извлечен данный экземпляр
	int32  Flags;
	TPoint PicSize;
	TArrangeParam Ap;
	StringSet Pool; // Контейнер строковых констант (символы, описания, пути к файлам и т.д.)
	SDrawGroup Dg;  // Контейнер для иконок и фигур.
	//
	// Принадлежность иконок и фигур элементу контейнера определяется по символу Item.Symb.
	// Иконка хранится с символом "{SYMB}-PIC", а фигура с символом "{SYMB}-FIG"
	//
	AbstractLayoutBlock ALB__; // @v10.9.9 Параметры размещения объекта, создаваемого в соответствии с данным инструментом
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
	virtual int HandleCommand(int cmd, void * pExt);
	virtual TWhatmanObject * Dup() const;
	virtual int Draw(TCanvas2 & rCanv);
	virtual int Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	//
	// Descr: Опции функции GetTextLayout()
	//
	enum {
		gtloBoundsOnly            = 0x0001, // Если объект требует отрисовки текста, то
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
	int    Setup(const TWhatmanToolArray::Item * pWtaItem); // @>>HandleCommand(cmdSetupByTool, 0)
	int    SetBounds(const TRect & rRect); // @>>HandleCommand(cmdSetBounds, 0)
	int    FASTCALL Copy(const TWhatmanObject & rS);
	TRect  GetBounds() const;
	TRect  GetTextBounds() const;
	//
	// Descr: Возвращает !0 если опция f установлена в поле TWhatmanObject::Options.
	//
	int    FASTCALL HasOption(int f) const;
	int    FASTCALL HasState(int f) const;
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
	const  SString & GetSymb() const;
	const  AbstractLayoutBlock & GetLayoutBlock() const;
	void   SetLayoutBlock(const AbstractLayoutBlock * pBlk);
	const  SString & GetLayoutContainerIdent() const;
	void   SetLayoutContainerIdent(const char * pIdent);

	enum {
		stCurrent            = 0x0001,
		stSelected           = 0x0002,
		stContainerCandidate = 0x0004  // @v10.9.6 Объект является кандидатом на превращение в контейнера-владельца для какого-либо иного объекта
	};
	enum {
		oMovable        = 0x0001, // Объект может перемещаться пользователем
		oResizable      = 0x0002, // Пользователь может менять размер объекта
		oDraggable      = 0x0004, // Объект используется для Drag'n'Drop обмена
		oBackground     = 0x0008, // Фоновый объект. Такой объект может быть только один. Его размер равен размеру ватмана.
			// При добавлении нового объекта с этим признаком, предыдущий уничтожается.
		oSelectable     = 0x0010, // Объект может быть выбран в окне, режим которого предполагает выбор некоторого объекта.
		oFrame          = 0x0020, // Активной частью объекта является только рамка.
		oMultSelectable = 0x0040, // Объект может быть включен в список множественного выбора объектов
		oContainer      = 0x0080  // @v10.9.6 Объект является контейнером. Это, в том числе, означает, что
			// иной объект может быть включен в этот контейнер.
			// Пока понятие принадлежности контейнеру принимаем эксклюзиным, то есть, объект может принадлежать
			// не более, чем одному контейнеру.
			// Мотивация: реализация layout
	};
protected:
	explicit TWhatmanObject(const char * pSymb);

	SString Symb;   //
	TextParam TextOptions;
	long   Options;
	long   State;       // @transient
private:
	TRect  Bounds;
	SString LayoutContainerIdent; // @v10.4.8 @persistent Символ родительского объекта типа Layout
	AbstractLayoutBlock Le2; // @v10.9.8 @persistent 
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
			fRule               = 0x0001, // Отображать линейки
			fGrid               = 0x0002, // Отображать сетку
			fDisableMoveObj     = 0x0004, // Объекты нельзя перемещать, даже если конкретный объект допускает это
			fDisableReszObj     = 0x0008, // Объекты нельзя изменять в размерах, даже если конкретный объект допускает это
			fSnapToGrid         = 0x0010, // При перемещении или изменении размеров объектов притягивать их координаты к решетке
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
		toolPenLayoutBorder,    // @v10.4.8
		toolPenContainerCandidateBorder // @v10.9.6
	};
	static uint32 GetSerializeSignature();
	explicit TWhatman(TWindow * pOwnerWin);
	~TWhatman();
	TPoint FASTCALL TransformPointToScreen(TPoint p) const;
	FPoint FASTCALL TransformPointToScreen(FPoint p) const;
	TPoint FASTCALL TransformScreenToPoint(TPoint p) const;
	TWindow * GetOwnerWindow() const;
	const  Param & GetParam() const;
	int    SetParam(const Param &);
	void   Clear();
	int    InsertObject(TWhatmanObject * pObj, int beforeIdx = -1);
	int    EditObject(int objIdx);
	int    RemoveObject(int idx);
	int    CheckUniqLayoutSymb(const TWhatmanObject * pObj) const;
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
	//    0 - ошибка: либо объект с индексом idx не существует, либо общая ошибка (например, SLERR_NOMEM).
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
	int    FindObjectByPoint(TPoint p, int * pIdx) const;
	int    FindContainerCandidateForObjectByPoint(TPoint p, const TWhatmanObject * pObj, int * pIdx) const;
	int    GetContaiterCandidateIdx() const { return ContainerCandidatePos; }
	void   SetupContainerCandidate(int idx, bool set);
	int    MoveObject(TWhatmanObject * pObj, const TRect & rRect);
	uint   GetObjectsCount() const;
	TWhatmanObject * FASTCALL GetObjectByIndex(int idx);
	const  TWhatmanObject * FASTCALL GetObjectByIndexC(int idx) const;
	int    FASTCALL InvalidateObjScope(const TWhatmanObject * pObj);
	int    GetObjTextLayout(const TWhatmanObject * pObj, STextLayout & rTl, int options);
	int    Draw(TCanvas2 & rCanv);
	//
	// Descr: Отрисовывает объект pObj на полотне rCanv с применение
	//   преобразования координат. Применяется клиентом класса для отрисовки
	//   объекта, не включенного в коллекцию this.
	//
	int    DrawSingleObject(TCanvas2 & rCanv, TWhatmanObject * pObj);
	int    DrawObjectContour(TCanvas2 & rCanv, const TWhatmanObject * pObj, const TPoint * pOffs);
	int    DrawMultSelContour(TCanvas2 & rCanv, const TPoint * pOffs);
	int    InvalidateMultSelContour(const TPoint * pOffs);
	//
	// @ARG(dir IN): SOW_XXX
	//
	int    ResizeObject(TWhatmanObject * pObj, int dir, TPoint toPoint, TRect * pResult);
	int    SetArea(const TRect & rArea);
	const  TRect & GetArea() const;
	//
	// ARG(mode):
	//   1 - start point
	//   2 - end point
	//   0 - reset
	//
	int    SetSelArea(TPoint p, int mode);
	const  TRect & GetSelArea() const;
	void   SetScrollPos(TPoint p);
	void   GetScrollRange(IntRange * pX, IntRange * pY) const;
	TPoint GetScrollDelta() const;
	int    SetTool(int toolId, int paintObjIdent);
	int    GetTool(int toolId) const;
	int    ArrangeObjects(const LongArray * pObjPosList, const TArrangeParam & rParam);
	int    ArrangeObjects2(const LongArray * pObjPosList, const TArrangeParam & rParam);
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
	int    Helper_ArrangeLayoutContainer(LayoutFlexItem * pParentLayout, WhatmanObjectLayoutBase * pC);

	uint32 SrcFileVer;  // @transient Версия формата хранения файла, из которого был загружен данный экземпляр объекта
	TRect  Area;        // @transient Видимая область
	TRect  SelArea;     // @transient Область, выделенная пользователем для выбора нескольких объектов
	TRect  ScrollRange; // @transient Автоматически рассчитываемый диапазон скроллирования области просмотра //
	TPoint ScrollPos;   // @transient Позиция скроллеров
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
	int    TidPenLayoutBorder; // @v10.4.8
	int    TidPenContainerCandidateBorder; // @v10.9.6
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
private:
	friend class TDialog;
	uint   Id;
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
	enum {
		kUnkn = 0,        // Неопределенный
		kDialog = 1,      // @anchor Диалог
		kInput = 2,       // Поле ввода
		kStatic,          // Статический текст
		kPushbutton,      // Кнопка
		kCheckbox,        // Одиночный флаг
		kRadioCluster,    // Кластер переключателей
		kCheckCluster,    // Кластер флагов
		kCombobox,        // Комбо-бокс
		kListbox,         // Список (возможно, многоколоночный)
		kTreeListbox,     // Древовидный список
		kFrame,           // Рамка
		kLabel,           // Текстовая этикетка, привязанная к другому элементу
		kRadiobutton,     // Радиокнопка (применяется только как связанный с kRadioCluster элемент)
		kCount            // @anchor Специальный элемент, равный количеству видов
	};

	static int  GetTextList(StrAssocArray & rList);
	static int  GetIdBySymb(const char * pSymb);
	explicit UiItemKind(int kind = kUnkn);
	int    Init(int kind);

	int32  Id;
	SString Symb;
	SString Text;
	TView * P_Cls;
};

class TDialog : public TWindow {
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

	static  INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
	static  void centerDlg(HWND);
	int     SetCtrlFont(uint ctrlID, const char * pFontName, int height);
	int     SetCtrlFont(uint ctlID, const SFontDescr & rFd);
	int     __cdecl SetCtrlsFont(const char * pFontName, int height, ...);
	int     SetCtrlToolTip(uint ctrlID, const char * pToolTipText);
	TDialog(const TRect & bounds, const char * pTitle);
	TDialog(uint resID, DialogPreProcFunc, void * extraPtr);
	explicit TDialog(uint resID);

	enum ConstructorOption {
		coNothing = 0,
		coExport = 1
	};
	TDialog(uint resID, ConstructorOption); // special constructor.
	~TDialog();
	virtual int    TransmitData(int dir, void * pData);
	virtual int    FASTCALL valid(ushort command);
	//
	// Descr: Запускает немодальное окно диалога
	//
	int    Insert();
	int    SetFont(const SFontDescr & rFd);
	void   ToCascade();
	//
	// Descr: Возвращает !0 если в поле состояния установлен флаг f.
	//
	int    FASTCALL CheckFlag(long f) const;
	int    FASTCALL addGroup(ushort grpID, CtrlGroup*);
	int    FASTCALL setGroupData(ushort, void *);
	int    FASTCALL getGroupData(ushort, void *);
	CtrlGroup * FASTCALL getGroup(ushort);
	long   getVirtButtonID(uint ctlID);
	TLabel * getCtlLabel(uint ctlID);
	int    getLabelText(uint ctlID, SString & rText);
	int    setLabelText(uint ctlID, const char * pText);
	int    GetCtlSymb(uint id, SString & rBuf) const;
	int    SaveUserSettings();
	int    RestoreUserSettings();
#ifndef _WIN32_WCE // {
	void   SetDlgTrackingSize(MINMAXINFO * pMinMaxInfo);
#endif // } _WIN32_WCE
	//int    SetRealRangeInput(uint ctlID, const RealRange *);
	//int    GetRealRangeInput(uint ctlID, RealRange *);
	//int    SetPeriodInput(uint ctlID, const DateRange *);
	//int    GetPeriodInput(uint ctlID, DateRange *);
	int    FASTCALL AddClusterAssoc(uint ctlID, long pos, long val);
	//
	// Descr: То же, что и AddClusterAssoc, но, кроме того, для radio-buttons
	//   устанавливает это же значение val как значение по умолчанию (pos = -1)
	//
	int    FASTCALL AddClusterAssocDef(uint ctlID, long pos, long val);
	int    FASTCALL SetClusterData(uint ctlID, long);
	int    FASTCALL GetClusterData(uint ctlID, long *);
	int    FASTCALL GetClusterData(uint ctlID, int16 *);
	long   FASTCALL GetClusterData(uint ctlID);
	void   DisableClusterItem(uint ctlID, int itemNo /* 0.. */, int toDisable = 1);
	int    SetClusterItemText(uint ctlID, int itemNo /* 0.. */, const char * pText);
	int    GetClusterItemByAssoc(uint ctlID, long val, int * pPos);
	int    SetDefaultButton(uint ctlID, int setDefault);
	int    SetCtrlBitmap(uint ctlID, uint bmID);
	int    SetupInputLine(uint ctlID, TYPEID typ, long fmt);
	void   SetupSpin(uint ctlID, uint buddyCtlID, int low, int upp, int cur);
	void   SetupCalendar(uint calCtlID, uint inputCtlID, int kind);
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
	// if (DlgFlags & fUserSettings)
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
	int    __cdecl LinkCtrlsToDlgBorders(long ctrlResizeFlags, ...);
	int    ResizeDlgToRect(const RECT * pRect);
	int    ResizeDlgToFullScreen();

	UserSettings Settings;
	long   DlgFlags;
	void * P_PrevData;
private:
	static int  FASTCALL PassMsgToCtrl(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void   Helper_Constructor(uint resID, DialogPreProcFunc dlgPreFunc, void * extraPtr, ConstructorOption co); // @<<TDialog::TDialog
	void    RemoveUnusedControls();
	TView * FASTCALL CtrlIdToView(long id) const;
	uint   GrpCount;
	CtrlGroup ** PP_Groups;
	SVector * P_FontsAry;
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
	//
	// Descr: Вспомогательная функция, испольуемая при формировании диалога из ресурсов
	//
	int    InsertCtl(TView * pCtl, uint id, const char * pSymb);
	int    SetCtlSymb(uint id, const char * pSymb);
	//
	TRect  InitRect;
	RECT   ResizedRect;  // @todo RECT-->TRect
	RECT   ToResizeRect; // @todo RECT-->TRect
	StrAssocArray * P_SymbList; // Специальный контейнер для хранения соотвествий идентификаторов элементов и их символов.
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
	int    IsWsVisible() const;

	uint   InputCtlId;
	WordSel_ExtraBlock * P_OuterWordSelBlk; // @notowner
	WordSelector * P_WordSel;
};

class TInputLine : public TView, private Helper_WordSelector {
public:
	static LRESULT CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	TInputLine(const TRect& bounds, TYPEID aType, long fmt);
	~TInputLine();
	virtual int    TransmitData(int dir, void * pData);
	virtual void   setState(uint aState, bool enable);
	virtual int    handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void   setupCombo(ComboBox *);
	void   setupFreeTextWordSelector(WordSel_ExtraBlock * pBlk);
	void   setFormat(long fmt);
	long   getFormat() const { return format; }
	void   setType(TYPEID typ);
	TYPEID getType() const { return type; }
	const char * getText();
	size_t getMaxLen() const { return maxLen; }
	void   setMaxLen(int newMaxLen);
	size_t getCaret();
	void   setCaret(size_t);
	void   getText(SString & rBuf) const;
	void   setText(const char *);
	void   disableDeleteSelection(int _disable);
	void   selectAll(int enable);

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

	SString Data;
	uint32 maxLen;
	TYPEID type;
	long   format;
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
		void   Reset();
		void   CheckIn();

		clock_t Last;
		double TmSum;
		double TmSqSum;
	};
	InputStat Stat;
	ComboBox * P_Combo;
private:
	void   Init();
	int    OnMouseWheel(int delta);
	int    OnPaste();
};

class TCalcInputLine : public TInputLine {
public:
	TCalcInputLine(uint virtButtonId, uint buttonCtrlId, TRect& bounds, TYPEID aType, long fmt);
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
private:
	static LRESULT CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual int    handleWindowsMessage(UINT, WPARAM, LPARAM);
	SDrawFigure * P_Fig;
	SString FigSymb;      // Символ векторной фигуры для отображения //
	SColor ReplacedColor; // Замещаемый цвет в векторном изображении
};

class TButton : public TView {
public:
	TButton(const TRect& bounds, const char *aTitle, ushort aCommand, ushort aFlags, uint bmpID = 0);
	~TButton();
	virtual int    handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual int    TransmitData(int dir, void * pData);
	virtual void   setState(uint aState, bool enable);
	void   press(ushort = 0);
	void   drawState(bool down);
	int    makeDefault(int enable, int sendMsg = 0);
	HBITMAP GetBitmap() const;
	uint   GetBmpID() const;
	int    LoadBitmap_(uint bmpID);
	int    SetBitmap(uint bmpID);
	ushort GetCommand() const;
	int    IsDefault() const;
	SString Title;
private:
	DECL_HANDLE_EVENT;

	ushort command;
	ushort flags;
	uint   BmpID;
	HBITMAP HBmp;
};

#define RADIOBUTTONS 1
#define CHECKBOXES   2

class TCluster : public TView {
public:
	TCluster(const TRect& bounds, int aKind, const StringSet * pStrings);
	~TCluster();
	virtual int    TransmitData(int dir, void * pData);
	virtual void   setState(uint aState, bool enable);
	bool   mark(int item);
	void   press(ushort item);
	uint   getNumItems() const;
	int    getText(int pos, char *, uint bufLen);
	int    setText(int pos, const char *);
	void   addItem(int, const char *);
	void   deleteItem(int);
	void   disableItem(int pos /* 0.. */, int disable);
	int    isItemEnabled(int item) const; // item = номер элемента в списке 0..
	void   deleteAll();
	int    isChecked(ushort item) const;  // item = (ushort)GetWindowLong(hWnd, GWL_ID);
	int    isEnabled(ushort item) const;  // item = (ushort)GetWindowLong(hWnd, GWL_ID);
	//
	// Три функции для ассоциирования элементов кластера с прикладными значениями.
	// pos == -1 соответствует значению по умолчанию.
	//
	int    addAssoc(long pos, long val);
	int    setDataAssoc(long);
	int    getDataAssoc(long *);
	int    getItemByAssoc(long val, int * pItem) const;
	int    getKind() const { return static_cast<int>(Kind); }
protected:
	virtual int    handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	int16  Kind;  // RADIOBUTTONS || CHECKBOXES
	ushort Value;
	int    Sel;
	int    DisableMask;
	SStrCollection Strings;
private:
	int    column(int item) const;
	int    row(int item) const;
	LAssocArray ValAssoc;
};

class TStaticText : public TView {
public:
	TStaticText(const TRect& bounds, const char * pText = 0);
	SString & getText(SString & rBuf) const;
	int    setText(const char *);
protected:
	virtual int handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	SString Text;
};

class TLabel : public TStaticText {
public:
	TLabel(const TRect& bounds, const char *aText, TView *aLink);
protected:
	DECL_HANDLE_EVENT;
	TView * link;
};

ushort messageBox(const char *msg, ushort aOptions);

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
// Метки Sign экземпляров класса ListBoxDef
//
#define LBDEFSIGN_INVALID  0x00000000 // Invalid (destroyed) instance
#define LBDEFSIGN_DEFAULT  0x1234ABCD // Instance of base class ListBoxDef or subclass hasn't own Sign
#define LBDEFSIGN_STD      0xABCD0001 // StdListBoxDef
#define LBDEFSIGN_STRASSOC 0xABCD0002 // StrAssocListBoxDef
#define LBDEFSIGN_STDTREE  0xABCD0003 // StdTreeListBoxDef
#define LBDEFSIGN_STRING   0xABCD0004 // StringListBoxDef
#define LBDEFSIGN_DBQ      0xABCD0005 // DBQListBoxDef
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
	virtual int    getCurString(SString & rBuf);
	virtual int    getCurData(void *);
	virtual int    search(const void *, CompFunc, int srchMode);
	virtual int    valid();
	virtual int    go(long);
	virtual int    step(long);
	virtual int    top();
	virtual int    bottom();
	virtual long   getRecsCount();
	virtual int    getIdList(LongArray &);
	virtual void * FASTCALL getRow_(long);
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
	virtual int    GetFrameSize();
	virtual int    GetFrameState();
	const  char * getText(long item, SString & rBuf);
	long   _topItem() const;
	long   _curItem() const;
	int    _isTreeList() const;
	void   SetOption(uint option, int set = 1);
	int    SetUserData(const void * pData, size_t size);
	int    GetUserData(void * pData, size_t * pSize) const;
	int    HasCapability(long c) const { return BIN(CFlags & c); }
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
	int    HasItemColorSpec() const;
	int    GetItemColor(long itemID, SColor * pFgColor, SColor * pBckgColor) const;
	long   GetCapability() const { return CFlags; }
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
	virtual int    valid();
	virtual long   getRecsCount();
	virtual int    getIdList(LongArray & rList);
	virtual void * FASTCALL getRow_(long r);
	virtual int    GetFrameSize();
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
	virtual int    valid();
	virtual long   getRecsCount();
	virtual int    getIdList(LongArray & rList);
	virtual void * FASTCALL getRow_(long r);
	virtual int    GetFrameSize();
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
	virtual int    valid();
	virtual int    go(long);
	virtual int    step(long);
	virtual int    top();
	virtual int    bottom();
	virtual long   getRecsCount();
	virtual int    getIdList(LongArray & rList);
	virtual void * FASTCALL getRow_(long);
	virtual int    getCurString(SString & rBuf);
	virtual int    getCurID(long *);
	virtual int    getCurData(void *);
	virtual int    search(const void * pPattern, CompFunc fcmp, int srchMode);
	virtual int    GetFrameSize();
	virtual int    GetFrameState();
	int    setArray(StrAssocArray *);
	int    GetStringByID(long id, SString & rBuf);
	int    GoByID(long id);
	int    FASTCALL HasChild(long id) const;
	int    GetListByParent(long parentId, LongArray & rList) const;
	int    GetParent(long child, long * pParent) const;
	int    GetChildList(long parentId, LongArray * pChildList);
protected:
	void   setupView();
	int    Helper_CreateTree();
	int    Helper_AddTreeItem(uint idx, UintHashTable & rAddedIdxList, uint32 * pPos);
private:
	StrAssocArray * P_SaList;
	struct TreeItem {
		long   Id;
		long   ParentId;
		void * H; // @v10.9.4 HTREEITEM-->(void *)
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

class StringListBoxDef : public StdListBoxDef {
public:
	StringListBoxDef(uint stringSize, uint aOptions);
	virtual int    addItem(long id, const char *, long * pPos = 0);
	virtual int    removeItem(long pos);
	virtual void   freeAll();
};

class DBQListBoxDef : public ListBoxDef {
public:
	DBQListBoxDef(DBQuery & rQuery, uint aOptions, uint aBufSize = 64);
	~DBQListBoxDef();
	virtual void   setViewHight(int);
	virtual void   getScrollData(long * pScrollDelta, long * pScrollPos);
	virtual int    valid();
	virtual int    go(long);
	virtual int    step(long);
	virtual int    top();
	virtual int    bottom();
	virtual long   getRecsCount();
	virtual int    getIdList(LongArray & rList);
	virtual void * FASTCALL getRow_(long);
	virtual int    TransmitData(int dir, void * pData);
	virtual int    refresh();
	virtual int    search(const void * pPattern, CompFunc fcmp, int srchMode);
	virtual int    GetFrameSize();
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
		stTreeList                   = 0x0001,  // Древовидный список
		stOwnerDraw                  = 0x0002,  // Диалог-владелец списка сам реализует функцию перерисовки
		stDataFounded                = 0x0004,  // Признак того, что (def->setData() != 0)
		stLButtonDown                = 0x0008,  // Левая кнопка мыши нажата на списке
		stInited                     = 0x0010,  // Выставляется функцией SmartListBox::onInit.
		stLBIsLinkedUISrchTextBlock  = 0x0020,  // Окно поиска будет прилинковано непосредственно к списку. При уничтожении фокус будет попадать на список.
		stOmitSearchByFirstChar      = 0x0040,  // Не обрабатывать ввод символа как сигнал к поиску
	};

	SmartListBox(const TRect & rRect, ListBoxDef * pDef, int isTree = 0);
	~SmartListBox();
	void   FASTCALL setDef(ListBoxDef * pDef);
	int    search(const void * pattern, CompFunc fcmp, int srchMode);
	int    FASTCALL getCurID(long * pId);
	int    FASTCALL getCurData(void * pData);
	int    FASTCALL getCurString(SString & rBuf);
	int    getText(long itemN  /* 0.. */, SString & rBuf);
	int    getID(long itemN, long * pID);
	int    isTreeList() const;
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
	int    SetupColumns(const char * pColsBuf);
	int    GetOrgColumnsDescr(SString & rBuf) const;
	void   setHorzRange(int);
	void   setRange(long aRange);
	void   search(char * firstLetter, int srchMode);
	void   setCompFunc(CompFunc f) { SrchFunc = f; }
	int    addItem(long id, const char * s, long * pPos = 0);
	int    removeItem(long pos);
	void   freeAll();
	void   FASTCALL focusItem(long item);
	int    SetupTreeWnd(void * hParent, long parentID); // @recursion // @v10.9.4 HTREEITEM-->(void *)
	void   Scroll(short sbCmd, int value);
	void   CreateScrollBar(int create);
	void   SetScrollBarPos(long pos, LPARAM lParam);
	//
	// Descr: Устанавливает или снимает состояние списка stTreeList.
	// Returns:
	//   1 Состояние установлено
	//   0 Состояние снято
	//
	int    SetTreeListState(int yes);
	void   SetOwnerDrawState();
	void   SetLBLnkToUISrchState();
	//
	// Descr: Устанавливает состояние stOmitSearchByFirstChar препятствующее
	//   появлению окна поиска в ответ на ввод символьной клавиши.
	//
	void   SetOmitSearchByFirstChar();
	int    HasState(long s) const;
	//
	// Перемещает окно Scrollbar в соответствии со списком. При этом старое окно Scrollbar разрушается и создается новое
	//
	void   MoveScrollBar(int autoHeight);

	ListBoxDef * def;
protected:
	int    GetStringByID(long id, SString & rBuf);
	int    GetImageIdxByID(long id, long * pIdx);
	void   SelectTreeItem();
	void   onInitDialog(int useScrollbar);
	int    FASTCALL onVKeyToItem(WPARAM wParam);
	int    GetMaxListHeight();
	void   Implement_Draw();
private:
	void   Helper_InsertColumn(uint pos);
	void   Helper_ClearTreeWnd();
	int    SetupTreeWnd2(uint32 parentP);

	struct ColumnDescr {
		uint   Width;
		uint   Format;   // ALIGN_XXX
		uint   TitlePos; // Позиция строки заголовка в StrPool
		long   Ident;    // Идентификатор столбца. Уникальность не проверяется.
	};
	long   State;
	long   Range;
	long   Top;
	long   Height;
	CompFunc SrchFunc;
	uint   SrchPatternPos; // Позиция последнего образца поиска в StrPool
	uint   ColumnsSpcPos;  // Позиция строки спецификации колонок в StrPool
	SArray Columns;
	StringSet StrPool;
	void * HIML; // @v10.9.4 HIMAGELIST-->(void *)
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
	ListWindow(ListBoxDef * pDef, const char * pTitle, int);

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
	int    FASTCALL getResult(long *);
	int    getString(SString & rBuf);
	int    getListData(void *);
	int    isTreeList() const;
	void   FASTCALL setDef(ListBoxDef * pDef);
	void   setCompFunc(CompFunc f);
	ListWindowSmartListBox * listBox() const;
	void   MoveWindow(HWND linkHwnd, long right); // @v10.7.7 int-->void
	void   MoveWindow(const RECT & rRect); // @v10.7.7 int-->void
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
	int    ViewRecent(); // @v10.7.7
	int    Activate();
	void   ActivateInput();
	int    CheckVisible() const;
	int    CheckActive() const;
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

	int    IsVisible;
	int    IsActive;
	SPaintToolBox Ptb;
	WordSel_ExtraBlock * P_Blk; // not owner
};

ListWindow * CreateListWindow(DBQuery & rQuery, uint options);
ListWindow * CreateListWindow(SArray * pAry, uint options, TYPEID);
ListWindow * CreateListWindow(StrAssocArray * pAry, uint options);
ListWindow * CreateListWindow(uint sz, uint options);
// WordSelector * CreateWordSelector(StrAssocArray * pAry, uint optons, UiWordSel_Helper * pHelper);

class ComboBoxInputLine : public TInputLine {
public:
	ComboBoxInputLine(ushort aId);
	virtual int  TransmitData(int dir, void * pData);
};

class ComboBox : public TView {
public:
	static LRESULT CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	ComboBox(const TRect &, ushort aFlags);
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
	TInputLine * link(void) const;
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
		stUndef         = 0x0004,
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
	BOOL   IsValid() const;
	int    GetCurrPos() const;
	HWND   H() const;
	HWND   GetToolbarHWND() const;
	uint   getItemsCount() const;
	const  ToolbarItem & getItem(uint idx/* 0.. */) const;
	const  ToolbarItem & getItemByID(uint /* 0.. */);
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

#define UISETTINGS_VERSION_MAJOR 1
#define UISETTINGS_VERSION_MINOR 9
#define TOOLBAR_OFFS 100000L

class UserInterfaceSettings { // @persistent @store(WinReg[HKCU\Software\Papyrus\UI]) @size=256
public:
	static const char * SubKey;  // "Software\\Papyrus\\UI";
	enum {
		fDontExitBrowserByEsc            = 0x00000001,
		fShowShortcuts                   = 0x00000002,
		fAddToBasketItemCurBrwItemAsQtty = 0x00000004, // При добавлении в корзину новой позиции из отчета в качестве начального количества использовать текущую ячейку в активном броузере
		fShowBizScoreOnDesktop           = 0x00000008, // Отображать бизнес показатели на рабочем столе
		fDisableNotFoundWindow           = 0x00000010, // Не отображать окно "Не найдено" при поиске в таблицах
		fUpdateReminder                  = 0x00000020, // Отображать напоминание об имеющихся обновлениях программы
		fTcbInterlaced                   = 0x00000040, // Горизонтальные полосы временной диаграммы отображать с черезстрочным изменением цвета. В противном случае = отделять строки линиями.
		fShowLeftTree                    = 0x00000080, // Показывать древовидную навигацию в левой части окна
		// @v10.9.3 fShowObjectsInLeftWindow         = 0x00000100, // @unused @v8.x.x Показывать диалоги редактирования списка объектов в левой части окна
		fDisableBeep                     = 0x00000200, // Запретить звуковые сигналы (ограниченная реализация)
		fBasketItemFocusPckg             = 0x00000400, // При вводе нового элемента товарной корзины фокус ввода устанавливать на
			// количество упаковок (а не единиц, как по умолчанию).
		fOldModifSignSelection           = 0x00000800, // Использовать технику выбора знака для строки документа модификации
			// товара, применявшуюся до v8.4.12 (выбор товара - выбор знака)
		fPollVoipService                 = 0x00001000, // Опрашивать VoIP сервис для обработки событий вызовов и звонков
		fExtGoodsSelMainName             = 0x00002000, // В списке расширенного выбора товара всегда показывать полные наименования товаров
			// Эта опция потенциально способно ускорить выборку поскольку не будет вынуждать программу лишний раз обращаться к записи товара
			// когда сокращенное наименование не совпадает с полным (see PPObjGoods::_Selector2()).
		fEnalbeBillMultiPrint            = 0x00004000, // @v10.3.0 Локальная установка флага PPBillConfig::Flags BCF_ALLOWMULTIPRINT
		fDisableBillMultiPrint           = 0x00008000, // @v10.3.0 Локальное отключение флага PPBillConfig::Flags BCF_ALLOWMULTIPRINT
			// If (fEnalbeBillMultiPrint ^ fDisableBillMultiPrint), то применяется общая конфигурация PPBillConfig
		fExtGoodsSelHideGenerics         = 0x00010000, // @v10.7.7 В списке расширенного выбора товара не показывать обобщенные товары
		fStringHistoryDisabled           = 0x00020000  // @v10.7.9 Запрет на использоватеня StringHistory (может быть проигнорирова при настройке более высокого уровня)
	};
	enum {
		wndVKDefault = 0,
		wndVKFlat    = 1,
		wndVKVector  = 2, //
		wndVKFancy   = 3  // Схема, ранее именовавшаяся как wndVKKind2 теперь обозначается wndVKFancy. Ее номер меняется,
			// вместо нее теперь будет использоваться схема wndVKVector
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
	int    WindowViewStyle;
	int    TableViewStyle;
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
	void   MoveWindow(const RECT &rRect);
	void   GetRect(RECT & rRect);
	void   Show(int show);
	void   MoveChilds(const RECT & rRect);
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
// @v10.9.3 #define SHCTSTAB_MAXTEXTLEN 20
#define SPEC_TITLEWND_ID    (1200 + 100)

class TProgram : public TGroup {
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
		stUiToolBoxInited = 0x0001 // Экземпляр UiToolBox был инициализирован вызовом InitUiToolBox
	};
	//
	// @todo Заменить все вызовы TProgram::GetInst на SLS.GetHInst
	//   Проверено: это одно и то же.
	//
	static HINSTANCE GetInst();
	static void IdlePaint();
	static void DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, const RECT & rDestRect, long xOffs,
		long yOffs, COLORREF cTransparentColor, COLORREF newBkgndColor, long fmt, POINT * pBmpSize);

	TProgram(HINSTANCE hInst, const char * pAppSymb, const char * pAppTitle);
	virtual ~TProgram();
	DECL_HANDLE_EVENT;
	virtual void run();
	TView * validView(TView *p);
	// @v10.0.02 void   idle();
	void   SetupTreeWnd(HMENU hMenu, void * hP); // @v10.9.4 HTREEITEM-->(void *)
	int    SizeMainWnd(HWND);
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
	int    CloseAllBrowsers();
	HWND   CreateDlg(uint dlgID, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	INT_PTR DlgBoxParam(uint dlgID, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	HBITMAP FASTCALL LoadBitmap_(uint bmID);
	HBITMAP FASTCALL FetchBitmap(uint bmID);
	HBITMAP FASTCALL FetchSystemBitmap(uint bmID);
	int    AddListToTree(long cmd, const char * pTitle, ListWindow * pLw);
	int    AddItemToMenu(const char * pTitle, void * ptr);
	int    UpdateItemInMenu(const char * pTitle, void * ptr);
	int    DelItemFromMenu(void * ptr);
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
	};

    int    InitUiToolBox();
	/*const*/ SPaintToolBox & GetUiToolBox() /*const*/ { return UiToolBox; }
	const SDrawFigure * LoadDrawFigureBySymb(const char * pSymb, TWhatmanToolArray::Item * pInfo) const;
	const SDrawFigure * LoadDrawFigureById(uint id, TWhatmanToolArray::Item * pInfo) const;

	static TProgram * application;   // @global
	TGroup   * P_DeskTop;
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
	UserInterfaceSettings UICfg;
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
	virtual int  LoadVectorTools(TWhatmanToolArray * pT);
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

	long   State;
	WNDPROC PrevCloseWndProc;
	HWND   H_FrameWnd;
	TBitmapHash BmH;
	SPaintToolBox UiToolBox;      // Набор инструментов для отрисовки компонентов пользовательского интерфейса.
	TWhatmanToolArray DvToolList; // Векторные изображения, загружаемые из внешнего файла
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
	int    Move();
	int    DoCommand(TPoint p);
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
#define CLASSNAME_DESKTOPWINDOW "PPYDESKTOPWINDOW"

struct SBrowserDataProcBlock {
	void   SetZero();
	void   FASTCALL Set(int32 i);
	void   Set(double i);
	void   FASTCALL Set(const char * pS);
	void   FASTCALL Set(const SString & rS);
	void   FASTCALL Set(LDATE dt);
	void   FASTCALL Set(LTIME tm);

	void * ExtraPtr;         // IN
	int    ColumnN;          // IN
	TYPEID TypeID;           // IN
	const  void * P_SrcData; // IN
	uint32 Color;            // OUT
	void * P_DestData;       // OUT
	SString TempBuf;         // @allocreuse Может использоваться реализацией функции SBrowserDataProc для ускорения работы
};

typedef int (FASTCALL * SBrowserDataProc)(SBrowserDataProcBlock * pBlk);

struct BroColumn {
	BroColumn();
	// @nodestructor
	TYPEID T;              // Data type
	uint   Offs;           // Offset from begining of row
	SBrowserDataProc UserProc;
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
	// Descr: Функция должна добавлять колонку в конец списка колонок
	// ARG(atPos IN): Позиция, в которую вставляется столбец. Если atPos < 0, то столбец
	//   добавляется в конец таблицы
	// ARG(pTxt  IN): Заголовок столбца. Может содержать символы \n для переноса на новую строку
	// ARG(fldNo IN): Номер столбца в структуре данных или смещение (в зависимости от контекста)
	// ARG(typ   IN): Тип данных. Если typ == 0, функция пытается самостоятельно опеределить тип
	//   данных по столбцу, указанному параметром fldNo
	// ARG(fmt   IN): Формат вывода данных (используйте MKSFMT или MKSFMTD)
	// ARG(opt   IN): Опции вывода данных в столбце (BCO_XXX see tvdefs.h)
	//
	virtual int   insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt);
	virtual int   insertColumn(int atPos, const char * pTxt, const char * pFldName, TYPEID typ, long fmt, uint opt);
	virtual void  setViewHight(int);
	virtual void  getScrollData(long * pScrollDelta, long * pScrollPos);
	virtual int   valid();
	virtual int   FASTCALL go(long);
	virtual int   FASTCALL step(long);
	virtual int   top();
	virtual int   bottom();
	virtual long  getRecsCount();
	virtual const void * FASTCALL getRow(long) const;
	// @v10.9.0 virtual int   FASTCALL getData(void *);
	// @v10.9.0 virtual int   FASTCALL setData(void *);
	virtual int   refresh();
	virtual int   search(const void * pPattern, CompFunc, int srchMode, int srchCol);
	virtual int   search2(const void * pSrchData, CompFunc, int srchMode, size_t offs);
	BroColumn & FASTCALL at(uint) const;
	void   initOffset(int);
	int    addColumn(const BroColumn *, int = UNDEF);
	int    removeColumn(int);
	int    setColumnTitle(int colN, const char * pText);
	int    addGroup(BroGroup *);
	const  BroGroup * groupOf(uint column, uint * pGrpPos = 0) const;
	uint   groupWidth(uint group, uint atColumn) const;
	uint   groupWidth(const BroGroup *, uint atColumn) const;
	int    GetCellData(const void * pRow, int column, TYPEID * pType, void * pDataBuf, size_t dataBufLen);
	int    GetCellData(long row, int column, TYPEID * pType, void * pDataBuf, size_t dataBufLen);
	char * getText(long row, int column, char * pBuf);
	//
	// Descr: Извлекает текст полностью (512 символов), независимо от ширины колонки в броузере
	//
	SString & getFullText(long row, int column, SString & rBuf);
	SString & getFullText(const void * pRowData, int column, SString & rBuf);
	char * getMultiLinesText(long, int, char *, uint = 0, uint * = 0);
	// @v10.6.3 (unused) int    setText(long, int, const char *);
	long   _topItem() const { return topItem; }
	long   _curItem() const { return curItem; }
	long   _curFrameItem() const { return (curItem - topItem); }
	int    FASTCALL isColInGroup(uint col, uint * idx) const;
	int    GetCapHeight() const;
	void   VerifyCapHeight();
	uint   GetGroupCount() const;
	const  BroGroup * FASTCALL GetGroup(uint) const;
	void   ClearGroupIndexies();
	uint * GetGroupIndexPtr(uint grpN);
	int    AddCrosstab(BroCrosstab *);
	uint   GetCrosstabCount() const;
	const  BroCrosstab * GetCrosstab(uint) const;
	int    FreeAllCrosstab();
	int    IsBOQ() const;
	int    IsEOQ() const;
	int    CheckFlag(uint) const;
	void   SetUserProc(SBrowserDataProc proc, void * extraPtr);
protected:
	SBrowserDataProc UserProc;
	void * ExtraPtr;
	int    capHight;
	uint   NumGroups;
	BroGroup * P_Groups;
	int    viewHight;
	long   scrollDelta;
	int    isBOQ;
	int    isEOQ;
	long   topItem;
	long   curItem;
private:
	virtual void FASTCALL freeItem(void *);

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
	virtual int   valid();
	virtual int   insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt);
	virtual long  getRecsCount();
	virtual const void * FASTCALL getRow(long) const;
	// @v10.9.0 virtual int   FASTCALL getData(void *);
	// @v10.9.0 virtual int   FASTCALL setData(void *);
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
	virtual int   insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID, long fmt, uint opt);
	virtual int   insertColumn(int atPos, const char * pTxt, const char * pFldName, TYPEID typ, long fmt, uint opt);
	virtual void  setViewHight(int);
	virtual void  getScrollData(long * pScrollDelta, long * pScrollPos);
	virtual int   valid();
	virtual int   FASTCALL go(long);
	virtual int   FASTCALL step(long);
	virtual int   top();
	virtual int   bottom();
	virtual long  getRecsCount();
	virtual const void * FASTCALL getRow(long) const;
	// @v10.9.0 virtual int   FASTCALL getData(void *);
	// @v10.9.0 virtual int   FASTCALL setData(void *);
	virtual int   refresh();
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

class TBaseBrowserWindow : public /*TWindow*/TWindowBase {
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
	// @v10.9.11 void   SetToolbarID(uint toolbarID);

	enum {
		IdBiasBrowser          = 0x00100000,
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
		bbsCancel       = 0x00000008, // @v10.3.4 Какой-то из виртуальных методов порожденного класса потребовал прекратить выполнение
		// Начиная с 0x00010000 флаги зарезервированы за наследующими классами
	};
	const  SString ClsName; // Window class name
	uint   ToolbarID;       // ID Toolbar'a для сохранения в реестре = LastCmd (команда по которой был запущен данный броузер) + TOOLBAR_OFFS (смещение)
	uint   ResourceID;
	TPoint PrevMouseCoord;
	long   BbState;
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
		int    FASTCALL SetLeftBottomCornerColor(COLORREF c); // returns strictly 1
		int    FASTCALL SetLeftTopCornerColor(COLORREF c); // returns strictly 1
		enum {
			fCorner           = 0x0001,
			fLeftBottomCorner = 0x0002,
			fRightFigCircle   = 0x0004
		};

		COLORREF Color;
		COLORREF Color2; // Цвет для нижнего левого угла
		COLORREF RightFigColor; // Цвет фигуры в правой части ячейки
		long   Flags;
		SString Description; // @v10.6.3
	private:
		CellStyle();
	};

	typedef int (* CellStyleFunc)(const void * pData, long col, int paintAction, CellStyle *, void * extraPtr);

	static int RegWindowClass(HINSTANCE hInst);
	static LRESULT CALLBACK BrowserWndProc(HWND, UINT, WPARAM, LPARAM);

	static LPCTSTR WndClsName;

	BrowserWindow(uint resID, DBQuery *, uint broDefOptions = 0);
	BrowserWindow(uint resID, SArray *, uint broDefOptions = 0);
	~BrowserWindow();
	//
	// Descr: Меняет запрос и, возможно, загружает другой ресурс таблицы для отображения.
	//   Если resID не равен текущему значению ResourceID, то загружает ресурс resID.
	// Returns:
	//   1 - запрос pQuery был установлен, но ресурс resID не отличается от this->ResourceID.
	//   2 - запрос pQuery был установлен и загружен ресурс resID
	//   0 - ошибка
	//
	int    ChangeResource(uint resID, DBQuery * pQuery, int force = 0);
	int    ChangeResource(uint resID, SArray * pArray, int force = 0);
	int    LoadToolbar(uint toolbarId);
	void   CalcRight();
	void   SetupScroll();
	int    insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt);
		// @>>BrowserDef::insertColumn(in, const char *, uint, TYPEID, long, uint)
	int    insertColumn(int atPos, const char * pTxt, const char * pFldName, TYPEID typ, long fmt, uint opt);
	int    removeColumn(int atPos);
	void   SetColumnWidth(int colNo, int width);
	void   SetupColumnWidth(uint colNo);
	int    SetColumnTitle(int conNo, const char * pText);
	void   SetFreeze(uint);
	LPRECT ItemRect(int hPos, int vPos, LPRECT, BOOL isFocus) const;
	LPRECT LineRect(int vPos, LPRECT, BOOL isFocus);
	int    GetColumnByX(int x) const;
	int    ItemByPoint(TPoint point, long * pHorzPos, long * pVertPos) const;
	enum {
		hdrzoneAny = 0,
		hdrzoneSortPoint = 1,
	};
	int    HeaderByPoint(TPoint point, int hdrzone, long * pVertPos) const;
	int    ItemByMousePos(long * pHorzPos, long * pVertPos);
	//
	// ARG(action IN):
	//   -1 - clear (alt + left button down)
	//   0  - clear, add one column (left button down)
	//   1  - add column (ctrl+left button down)
	//
	int    SelColByPoint(const POINT *, int action);
	void   FocusItem(int hPos, int vPos);
	int    IsResizePos(TPoint);
	void   Resize(TPoint p, int mode); // mode: 0 - toggle off, 1 - toggle on, 2 - process
	void   Refresh();
	BrowserDef * getDef();
	const BrowserDef * getDefC() const;
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

	// @v10.9.11 BrowserWindow * P_View__;
	enum {
		paintFocused = 0,
		paintNormal  = 1,
		paintClear   = 2,
		paintQueryDescription = 3 // @v10.6.3 Специальная опция, передаваемая в callback-функцию CellStyleFunc
			// для получения дополнительной информации о ячейке и ее окраске.
	};
protected:
	DECL_HANDLE_EVENT;
	int    WMHScroll(int sbType, int sbEvent, int thumbPos);
	int    WMHScrollMult(int sbEvent, int thumbPos, long * pOldTop);
	int    LoadResource(uint, void *, int, uint uOptions = 0);

	uint   RezID;
	// @v10.9.11 (moved to TBaseBrowserWindow) TToolbar * P_Toolbar;
private:
	virtual void Insert_(TView *p);
	virtual TBaseBrowserWindow::IdentBlock & GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk);
	void   __Init(/*BrowserDef * pDef*/);
	void   WMHCreate(LPCREATESTRUCT);
	long   CalcHdrWidth(int plusToolbar) const;
	int    IsLastPage(uint viewHeight); // AHTOXA
	void   ClearFocusRect(LPRECT);
	void   DrawCapBk(HDC, const RECT *, BOOL);
	void   DrawFocus(HDC, const RECT *, BOOL DrawOrClear, BOOL isCellCursor = 0);
	void   Paint();
	//
	// col = -1 - раскрашивать строчку целиком
	//
	int    PaintCell(HDC hdc, RECT r, long row, long col, int paintAction);
	int    search(void * pPattern, CompFunc fcmp, int srchMode);
	int    DrawTextUnderCursor(HDC hdc, char * pBuf, RECT * pTextRect, int fmt, int isLineCursor);
	void   AdjustCursorsForHdr();
	int    CalcRowsHeight(long topItem, long bottom = 0);
	void   DrawMultiLinesText(HDC hdc, char * pBuf, RECT * pTextRect, int fmt);
	int    FASTCALL CellRight(const BroColumn & rC) const;
	int    FASTCALL GetRowHeightMult(long row) const;
	int    FASTCALL GetRowTop(long row) const;

	long   InitPos;
	TView * P_Header;
	// @v10.9.11 (moved to TBaseBrowserWindow) int    ToolBarWidth;
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
	TPoint CliSz;   // Размер клиентской области окна
	TPoint ChrSz;   // Средний размер символов.
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
	LongArray SettledOrder;     // @v10.6.3 Индексы столбцов, по которым задана сортировка.
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
	static const char * WndClsName;

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
	int    Locate(TPoint p, Loc * pLoc) const;
	int    RestoreParameters(STimeChunkBrowser::Param & rParam);
protected:
	struct ResizeState {
		void   Setup(int kind, TPoint p);
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
		TPoint Org;          // Точка отсчета для изменения размеров (масштаба)
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
		const SRect * FASTCALL SearchPoint(TPoint p) const;
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
	void   DrawMoveSpot(TCanvas & rCanv, TPoint p);
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
	int    Resize(int mode, TPoint p);
	int    ProcessDblClk(TPoint p);
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
	int    Resize();
	int    CmpFileName(const char * pFileName);
	int    FileLoad(const char * pFileName, SCodepage cp, long flags);
	int    FileSave(const char * pFileName, long flags);
	int    FileClose();
private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ScintillaWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual TBaseBrowserWindow::IdentBlock & GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk);
	virtual int ProcessCommand(uint ppvCmd, const void * pHdr, void * pBrw);
	int    LoadToolbar(uint tbId);
	int    GetText(SString & rBuf);
	int    SetText(SString & rBuf);
	int    SaveChanges();
	int    SetEncoding(SCodepage cp);
	SCodepage SelectEncoding(SCodepage initCp) const;
	int    InsertWorkbookLink();
	int    BraceHtmlTag();
	int    UpdateIndicators();
	int    Run();

	enum {
		sstLastKeyDownConsumed = 0x0001
	};
	long   SysState;
	int    SpcMode;
	HWND   HwndSci;
	// @v10.9.11 (moved to TBaseBrowserWindow) TToolbar * P_Toolbar;
	// @v10.9.11 (moved to TBaseBrowserWindow) long   ToolBarWidth;
	//uint   ToolbarId;
	SString LexerSymb;
	WNDPROC OrgScintillaWndProc;
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
	TVRez(const char * fName, int useIndex = 0);
	~TVRez();
	int    open(const char *, int useIndex = 0);
	int    setHdrType();
	int    buildIndex();
	int    getChar();
	uint   getUINT();
	char * FASTCALL getString(char *, int kind = 0 /*0 - 866, 1 - w_char, 2 - 1251*/);
	SString & FASTCALL getString(SString & rBuf, int kind /*0 - 866, 1 - w_char, 2 - 1251*/);
	TRect  getRect();
	TYPEID getType(int defaultLen);
	long   getFormat(int defaultLen);
	int    readHeader(ulong ofs, WResHeaderInfo * hdr, ResPosition);
	int    findResource(uint rscID, uint rscType, long * pOffs = 0, long * pSz = 0);
	int    getSizeField(long *);
	long   getStreamPos();
	int    enumResources(uint rscType, uint * rscID, ulong * dwPos);
	FILE * getStream() const { return Stream; }
	int    CheckDialogs(const char * pLogFileName); // @debug

	int    error;
private:
	int    _readHeader16(ulong ofs, WResHeaderInfo * hdr, ResPosition);
	int    _readHeader32(ulong ofs, WResHeaderInfo * hdr, ResPosition);
	SString FileName;
	FILE   * Stream;
	SVector * P_Index;
	int      HeaderType;
};

extern int (* getUserControl)(TVRez *, TDialog*);

int    LoadToolbar(TVRez *, uint tbType, uint tbID, ToolbarList *);
//
#endif // } __TV_H
