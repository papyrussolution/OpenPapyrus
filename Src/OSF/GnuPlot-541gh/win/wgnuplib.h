/* GNUPLOT - win/wgnuplib.h */

/*[
 * Copyright 1982 - 1993, 1998, 2004   Russell Lang
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
   ]*/

/*
 * AUTHORS
 *
 *   Russell Lang
 */

#ifndef WGNUPLIB_H
#define WGNUPLIB_H

//#include <windows.h>
#include "screenbuf.h"

//#ifdef __cplusplus
//extern "C" {
//#endif

/* ================================== */
/* symbols for the two icons          */
#define TEXTICON 123
#define GRPICON 124

/* ================================== */
/* For WIN32 API's */
/* #define DEFAULT_CHARSET ANSI_CHARSET */
#define MoveTo(hdc, x, y) MoveToEx(hdc, x, y, (LPPOINT)NULL);

/* printf format for TCHAR arguments */
#ifdef UNICODE
	#define TCHARFMT "%ls"
#else
	#define TCHARFMT "%hs"
#endif

/* ================================== */
/* wprinter.c - windows printer routines */
void DumpPrinter(HWND hwnd, LPTSTR szAppName, LPCTSTR szFileName);

struct GP_PRINT {
	GP_PRINT() : hdcPrn(0), hDlgPrint(0), bUserAbort(FALSE), szTitle(0), bDriverChanged(FALSE), services(0), next(0)
	{
		MEMSZERO(pdef);
		MEMSZERO(psize);
		MEMSZERO(poff);
	}
	HDC    hdcPrn;
	HWND   hDlgPrint;
	BOOL   bUserAbort;
	LPCTSTR szTitle;
	POINT  pdef;
	POINT  psize;
	POINT  poff;
	BOOL   bDriverChanged;
	void * services;
	GP_PRINT * next;
};

//typedef GP_PRINT * GP_LPPRINT;
//
// wpause.c - pause window structure 
//
struct PW {
	PW()
	{
		THISZERO();
	}
	HINSTANCE hInstance; /* required */
	HINSTANCE hPrevInstance; /* required */
	LPWSTR Title;                   /* required */
	LPWSTR Message; /* required */
	POINT Origin;                   /* optional */
	HWND hWndParent; /* optional */
	HWND hWndPause;
	HWND hOK;
	HWND hCancel;
	BOOL bPause;
	BOOL bPauseCancel;
	BOOL bDefOK;
	WNDPROC lpfnOK;
	WNDPROC lpfnCancel;
	WNDPROC lpfnPauseButtonProc;
};

//typedef PW * LPPW;

bool MousableWindowOpened(GpTermEntry * pTerm);
int  PauseBox(GpTermEntry * pTerm, PW * lppw);

/* ================================== */
/* wmenu.c - menu structure */
#define BUTTONMAX 10
struct MW {
	MW()
	{
		THISZERO();
	}
	LPTSTR szMenuName; /* required */
	HMENU  hMenu;
	BYTE ** macro;
	BYTE * macrobuf;
	int    nCountMenu;
	DLGPROC lpProcInput;
	LPWSTR szPrompt;
	LPWSTR szAnswer;
	int    nChar;
	int    nButton;
	HWND   hToolbar;
	HWND   hButton[BUTTONMAX];
	int    hButtonID[BUTTONMAX];
};
//typedef MW * LPMW;
//
// wtext.c text window structure */
// If an optional item is not specified it must be zero 
//
#define MAXFONTNAME 80
struct TW {
	TW()
	{
		THISZERO();
	}
	int    TextKBHit() const;
	int    TextGetCh();
	void   TextStartEditing();
	void   TextToCursor();
	void   NewLine();
	void   UpdateText(int count);
	int    TextPutChW(WCHAR ch);
	GP_PRINT * P_Lpr;        // must be first 
	HINSTANCE hInstance;     // required 
	HINSTANCE hPrevInstance; // required 
	LPWSTR Title;            // required 
	MW   * P_LpMw;           // optional 
	POINT  ScreenSize;       // optional // size of the visible screen in characters */
	uint   KeyBufSize;       // optional 
	LPTSTR IniFile;          // optional 
	LPTSTR IniSection;       // optional 
	LPWSTR DragPre;          // optional 
	LPWSTR DragPost;         // optional 
	int    nCmdShow;         // optional 
	FARPROC shutdown;        // optional 
	HICON  hIcon;            // optional 
	LPTSTR AboutText;        // optional 
	HMENU  hPopMenu;
	HWND   hWndText;
	HWND   hWndParent;
	HWND   hWndToolbar;
	HWND   hStatusbar;
	HWND   hWndSeparator;
	HWND   hWndFocus;       // window with input focus 
	POINT  Origin;
	POINT  Size;
	SB     ScreenBuffer;
	BOOL   bWrap;           // wrap long lines 
	BYTE * KeyBuf;
	BYTE * KeyBufIn;
	BYTE * KeyBufOut;
	BYTE   Attr;
	BOOL   bFocus;
	BOOL   bGetCh;
	BOOL   bSysColors;
	HBRUSH hbrBackground;
	TCHAR  fontname[MAXFONTNAME]; // font name 
	int    fontsize;              // font size in pts 
	HFONT  hfont;
	int    CharAscent;
	int    ButtonHeight;
	int    StatusHeight;
	int    CaretHeight;
	int    CursorFlag;    // scroll to cursor after \n & \r 
	POINT  CursorPos;     // cursor position on screen 
	POINT  ClientSize;    // size of the client window in pixels 
	POINT  CharSize;
	POINT  ScrollPos;
	POINT  ScrollMax;
	POINT  MarkBegin;
	POINT  MarkEnd;
	BOOL   Marking;
	int    bSuspend;
	int    MaxCursorPos;
	// variables for docked graphs 
	UINT   nDocked;
	UINT   VertFracDock;
	UINT   HorzFracDock;
	UINT   nDockCols;
	UINT   nDockRows;
	UINT   SeparatorWidth;
	COLORREF SeparatorColor;
	BOOL   bFracChanging;
};
//typedef TW * LPTW;

#ifndef WGP_CONSOLE
//
// wtext.c - Text Window 
//
void   TextMessage();
int    TextInit(TW * lptw);
void   TextClose(TW * lptw);
//int    TextKBHit(const TW *);
//int    TextGetCh(TW *);
int    TextGetChE(TW *);
LPSTR  TextGetS(TW * lptw, LPSTR str, uint size);
int    TextPutCh(TW *, BYTE);
//int    TextPutChW(TW * lptw, WCHAR ch);
int    TextPutS(TW * lptw, LPSTR str);
//void   TextStartEditing(TW * lptw);
void   TextStopEditing(TW * lptw);
#if 0
	// The new screen buffer currently does not support these 
	void TextGotoXY(TW * lptw, int x, int y);
	int  TextWhereX(TW * lptw);
	int  TextWhereY(TW * lptw);
	void TextCursorHeight(TW * lptw, int height);
	void TextClearEOL(TW * lptw);
	void TextClearEOS(TW * lptw);
	void TextInsertLine(TW * lptw);
	void TextDeleteLine(TW * lptw);
	void TextScrollReverse(TW * lptw);
#endif
void TextAttr(TW * lptw, BYTE attr);
#endif /* WGP_CONSOLE */

void AboutBox(HWND hwnd, LPTSTR str);
//
// wgraph.c - graphics window 
//
// windows data 
//
#define WGNUMPENS 15 // number of different 'basic' pens supported (the ones you can modify by the 'Line styles...' dialog, and save to/from wgnuplot.ini). 
#define WIN_PAL_COLORS 4096 // maximum number of different colors per palette, used to be hardcoded (256) 
//
// hypertext structure
//
struct tooltips {
	LPWSTR text;
	RECT rect;
};
//
// Information about one graphical operation to be stored by the
// driver for the sake of redraws. Array of GWOP kept in global block 
//
struct GWOP {
	UINT op;
	UINT x;
	UINT y;
	HLOCAL htext;
};
//
// memory block for graph operations 
//
struct GWOPBLK { // kept in local memory 
	GWOPBLK * next;
	HGLOBAL hblk; // handle to a global block 
	GWOP * gwop; // pointer to global block if locked 
	UINT used; // number of GWOP's used 
};

#define GWOPMAX (4*4096) // Maximum number of GWOPBLK arrays to be remembered. 
//
// point types 
//
enum win_pointtypes {
	W_invalid_pointtype = 0,
	W_dot = 10,
	W_plus, 
	W_cross, 
	W_star,
	W_box, 
	W_fbox,
	W_circle, 
	W_fcircle,
	W_itriangle, 
	W_fitriangle,
	W_triangle, 
	W_ftriangle,
	W_diamond, 
	W_fdiamond,
	W_pentagon, 
	W_fpentagon,
	W_last_pointtype = W_fpentagon
};

// The dot is reserved for pt 0, number of (remaining) point types:
#define WIN_POINT_TYPES (W_last_pointtype - W_plus + 1)

/* ops */
enum win_draw_commands {
	W_endoflist = 0,
	W_point = 9,
	W_pointsize = 30,
	W_setcolor,
	W_polyline, 
	W_line_type, 
	W_dash_type, 
	W_line_width,
	W_put_text, 
	W_enhanced_text, 
	W_boxedtext,
	W_text_encoding, 
	W_font, 
	W_justify, 
	W_text_angle,
	W_filled_polygon_draw, 
	W_filled_polygon_pt,
	W_fillstyle,
	W_move, W_boxfill,
	W_image,
	W_layer,
	W_hypertext
};

struct GW {
	GW() : lpr(0), hInstance(0), hPrevInstance(0), Id(0), Title(0), lptw(0), IniFile(0), IniSection(0),
		hptrDefault(0), hptrCrossHair(0), hptrScaling(0), hptrRotating(0), hptrZooming(0), hptrCurrent(0), Flags(0)
	{
	}
	void   OpSize(UINT op, UINT x, UINT y, LPCSTR str, DWORD size);
	void   Op(UINT op, UINT x, UINT y, LPCSTR str);
	GP_PRINT * lpr; // must be first 
	// window properties etc. 
	HINSTANCE hInstance;    // required 
	HINSTANCE hPrevInstance; // required 
	int    Id;              // plot number 
	LPTSTR Title;           // required 
	TW *   lptw;            // associated text window, optional 
	LPTSTR IniFile;         // optional 
	LPTSTR IniSection;      // optional 
	// window size and position 
	BOOL   bDocked;    // is the graph docked to the text window? 
	SPoint2I Origin_;  // origin of graph window 
	SPoint2I Size_;    // size of graph window 
	SPoint2I Canvas_;  // requested size of the canvas 
	SPoint2I Decoration_; // extent of the window decorations 
	int    StatusHeight;  // height of status line area 
	int    ToolbarHeight; // height of the toolbar 
	// (subwindow) handles 
	HWND   hWndGraph;  // window handle of the top window 
	HWND   hGraph;     // window handle of the actual graph 
	HWND   hStatusbar; // window handle of status bar 
	HWND   hToolbar;   // window handle of the toolbar 
	HMENU  hPopMenu;   // popup menu 
	HWND   hTooltip;   // tooltip windows for hypertext 
	// command list 
	GWOPBLK * gwopblk_head; // graph command list 
	GWOPBLK * gwopblk_tail;
	uint nGWOP;
	//
	enum {
		fLocked       = 0x00000001, // locked if being written 
		fBufferValid  = 0x00000002, // indicates if hBitmap is valid 
		fInitialized  = 0x00000004, // already initialized? 
		fRotating     = 0x00000008, // are we currently rotating the graph? 
		fGraphToTop   = 0x00000010, // bring graph window to top after every plot? 
		fColor        = 0x00000020, // colored graph? 
		fOverSample   = 0x00000040, // oversampling? 
		fGdiPlus      = 0x00000080, // Use GDI+ only backend? 
		fD2D  = 0x00000100, // Use Direct2D backend?   
		fAntialiasing = 0x00000200,
		fPolyAa       = 0x00000400,
		fFastRotation = 0x00000800,
		fHasGrid      = 0x00001000,
		fHideGrid     = 0x00002000,
		fDashed       = 0x00004000,
		fRounded      = 0x00008000,
		fRotate       = 0x00010000,
		fBDocked      = 0x00020000   // is the graph docked to the text window? 
	};
	uint   Flags;
	//
	BOOL locked; // locked if being written 
	// off-screen bitmap used by GDI, GDI+ and D2D DCRenderer 
	HBITMAP hBitmap;  // bitmap of current graph 
	BOOL buffervalid; // indicates if hBitmap is valid 
	/* state */
	BOOL initialized; // already initialized? 
	BOOL rotating;    // are we currently rotating the graph? 
	/* options */
	BOOL graphtotop; // bring graph window to top after every plot? 
	BOOL color;      // colored graph? 
	BOOL oversample; // oversampling? 
	BOOL gdiplus;    // Use GDI+ only backend? 
	BOOL d2d;        // Use Direct2D backend? 
	BOOL antialiasing; /* anti-aliasing? */
	BOOL polyaa; /* anti-aliasing for polygons ? */
	BOOL fastrotation; /* rotate without anti-aliasing? */
	COLORREF background; /* background color */
	// plot properties 
	//int xmax; // required 
	//int ymax; // required 
	SPoint2I MaxS; // required 
	//int htic;  // horizontal size of point symbol (xmax units) 
	//int vtic;  // vertical size of point symbol (ymax units)
	SPoint2I TicS; // Size of point symbol (xmax units) 
	//int hchar; // horizontal size of character (xmax units) 
	//int vchar; // vertical size of character (ymax units)
	SPoint2I ChrS; // Size of character (xmax units) 
	// layers 
	uint   numplots;
	BOOL   hasgrid;
	BOOL   hidegrid;
	BOOL * hideplot; // array for handling hidden plots 
	uint   maxhideplots;
	RECT * keyboxes;
	uint maxkeyboxes;
	// hypertext 
	struct tooltips * tooltips;
	uint maxtooltips;
	uint numtooltips;
	// points and lines 
	double pointscale; /* scale factor for point sizes */
	double org_pointsize; /* original pointsize */
	BOOL   dashed; /* dashed lines? */
	BOOL   rounded; /* rounded line caps and joins? */
	double linewidth; /* scale factor for linewidth */
	LOGPEN colorpen[WGNUMPENS+2]; /* color pen definitions */
	LOGPEN monopen[WGNUMPENS+2]; /* mono pen definitions */
	// fonts 
	double fontscale; /* scale factor for font sizes */
	TCHAR  deffontname[MAXFONTNAME]; /* default font name */
	int    deffontsize; /* default font size */
	TCHAR  fontname[MAXFONTNAME]; /* current font name */
	int    fontsize; /* current font size in pts */
	int    angle; /* text angle */
	BOOL   rotate; /* can text be rotated? */
	int    justify; /* text justification */
	int    encoding_error; // last unknown encoding 
	enum   set_encoding_id encoding; // text encoding 
	LONG   tmHeight; // text metric of current font 
	LONG   tmAscent;
	LONG   tmDescent;
	HCURSOR hptrDefault;
	HCURSOR hptrCrossHair;
	HCURSOR hptrScaling;
	HCURSOR hptrRotating;
	HCURSOR hptrZooming;
	HCURSOR hptrCurrent;
#ifdef USE_WINGDI
	// GDI resources 
	HPEN hapen; /* stored current pen */
	HPEN hsolid; /* solid sibling of current pen */
	HPEN hnull; /* empty null pen */
	HBRUSH colorbrush[WGNUMPENS+2]; /* brushes to fill points */
	HBRUSH hbrush; /* background brush */
	HBRUSH hcolorbrush; /* color fill brush */
	HFONT hfonth; /* horizontal font */
	HFONT hfontv; /* rotated font (arbitrary angle) */
	LOGFONT lf; /* cached to speed up rotated fonts */
#endif
#ifdef HAVE_D2D
	/* Direct2D resources */
#if !defined(HAVE_D2D11) || defined(DCRENDERER)
	struct ID2D1RenderTarget * pRenderTarget;
#else
	struct ID2D1Device * pDirect2dDevice;
	struct ID2D1DeviceContext * pRenderTarget;
	struct IDXGISwapChain1 * pDXGISwapChain;
#endif
	int dpi;                                /* (nominal) resolution of output device */
#endif
	GW * next; // pointer to next window 
};
//typedef GW * LPGW;

struct enhstate_struct {
	GW * lpgw; /* graph window */
	LPRECT rect; /* rect to update */
	BOOL opened_string; /* started processing of substring? */
	BOOL show; /* print this substring? */
	int overprint; /* overprint flag */
	BOOL widthflag; /* FALSE for zero width boxes */
	BOOL sizeonly; /* only measure length of substring? */
	double base; /* current baseline position (above initial baseline) */
	//int xsave, ysave; // save text position for overprinted text 
	//int x, y; // current text position 
	SPoint2I PtPreserve; // save text position for overprinted text 
	SPoint2I Pt; // current text position 
	TCHAR fontname[MAXFONTNAME]; /* current font name */
	double fontsize; /* current font size */
	int totalwidth; /* total width of printed text */
	int totalasc; /* total height above center line */
	int totaldesc; /* total height below center line */
	double res_scale; /* scaling due to different resolution (printers) */
	int shift; /* baseline alignment */
	void (* set_font)();
	uint (* text_length)(char *);
	void (* put_text)(int, int, char *);
	void (* cleanup)();
};

extern enhstate_struct enhstate;

// No TEXT Macro required for fonts 
#define WINFONTSIZE 10
#ifndef WINFONT
	#define WINFONT "Tahoma"
#endif
#ifndef WINJPFONT
	#define WINJPFONT "MS PGothic"
#endif
#define WINGRAPHTITLE TEXT("gnuplot graph")

extern GpTermEntry * WIN_term;
extern TCHAR WIN_inifontname[MAXFONTNAME];
extern int WIN_inifontsize;

void GraphInitStruct(GW * lpgw);
void GraphInit(GW * lpgw);
void GraphUpdateWindowPosSize(GW * lpgw);
void GraphClose(GW * lpgw);
void GraphStart(GW * lpgw, double pointsize);
void GraphEnd(GW * lpgw);
void GraphChangeTitle(GW * lpgw);
void GraphResume(GW * lpgw);
//void GraphOp(GW * lpgw, UINT op, UINT x, UINT y, LPCSTR str);
//void GraphOpSize(GW * lpgw, UINT op, UINT x, UINT y, LPCSTR str, DWORD size);
void GraphPrint(GW * lpgw);
void GraphRedraw(GW * lpgw);
void GraphModifyPlots(GW * lpgw, uint operations, int plotno);
void win_close_terminal_window(GW * lpgw);
bool GraphHasWindow(GW * lpgw);
LPTSTR GraphDefaultFont();
#ifdef USE_MOUSE
	void Graph_set_cursor(GW * lpgw, int c, int x, int y);
	void Graph_set_ruler(GW * lpgw, int x, int y);
	void Graph_put_tmptext(GW * lpgw, int i, LPCSTR str);
	void Graph_set_clipboard(GW * lpgw, LPCSTR s);
#endif
/* BM: callback functions for enhanced text */
void GraphEnhancedOpen(GpTermEntry_Static * pThis, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
void GraphEnhancedFlush();
LPWSTR UnicodeTextWithEscapes(LPCSTR str, enum set_encoding_id encoding);
void WIN_update_options();

/* ================================== */

//#ifdef __cplusplus
//}
//#endif

#endif
