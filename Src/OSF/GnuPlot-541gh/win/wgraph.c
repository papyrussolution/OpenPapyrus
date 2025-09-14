// GNUPLOT - win/wgraph.c 
// Copyright 1992, 1993, 1998, 2004   Maurice Castro, Russell Lang
//
#include <gnuplot.h>
#pragma hdrstop

//#define USE_WINGDI // Enable define to include the GDI backend 

/* sanity check */
#if !defined(USE_WINGDI) && !defined(HAVE_GDIPLUS) && !defined(HAVE_D2D)
	#error "No valid windows terminal backend enabled."
#endif
/* Use GDI instead of GDI+ while rotating splots */
#if defined(HAVE_GDIPLUS) && defined(USE_WINGDI)
	#define FASTROT_WITH_GDI
#endif
#define STRICT
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <direct.h>           /* for _chdir */
#include "winmain.h"
#include "wresourc.h"
#include "wcommon.h"
#include "wgnuplib.h"
#ifdef HAVE_GDIPLUS
	#include "wgdiplus.h"
#endif
#ifdef HAVE_D2D
	#include "wd2d.h"
#endif
#ifndef WM_MOUSEHWHEEL /* requires _WIN32_WINNT >= 0x0600 */
	#define WM_MOUSEHWHEEL 0x020E
#endif

/* Names of window classes */
static const LPCTSTR szGraphClass = TEXT("wgnuplot_graph");
static const LPCTSTR szGraphParentClass = TEXT("wgnuplot_graphwindow");

#ifdef USE_MOUSE
/* Petr Mikulik, February 2001
 * Declarations similar to src/os2/gclient.c -- see section
 * "PM: Now variables for mouse" there in.
 */
 //
// Status of the ruler 
//
static struct Ruler {
	bool on; // ruler active ? 
	int x;
	int y; // ruler position 
} ruler = {FALSE, 0, 0, };
//
// Status of the line from ruler to cursor 
//
static struct RulerLineTo {
	bool on; // ruler line active ? 
	int x;
	int y; // ruler line end position (previous cursor position) 
} ruler_lineto = {FALSE, 0, 0, };
//
// Status of zoom box 
//
static struct Zoombox {
	bool on; /* set to TRUE during zooming */
	POINT from;
	POINT to; /* corners of the zoom box */
	LPCSTR text1;
	LPCSTR text2; /* texts in the corners (i.e. positions) */
} zoombox = { FALSE, {0, 0}, {0, 0}, NULL, NULL };

// Mouse support routines 
static void     Wnd_exec_event(GW * lpgw, LPARAM lparam, char type, int par1);
static void     GetMousePosViewport(GW * lpgw, int * mx, int * my);
static void     Draw_XOR_Text(GW * lpgw, const char * text, size_t length, int x, int y);
#endif
static void     UpdateStatusLine(GW * lpgw, const char text[]);
static void     UpdateToolbar(GW * lpgw);
#ifdef USE_MOUSE
	static void     DrawRuler(GW * lpgw);
	static void     DrawRulerLineTo(GW * lpgw);
	static void     DrawZoomBox(GW * lpgw);
	static void     LoadCursors(GW * lpgw);
	static void     DestroyCursors(GW * lpgw);
#endif /* USE_MOUSE */
static void     DrawFocusIndicator(GW * lpgw);

/* ================================== */

#define WGDEFCOLOR 15
COLORREF wginitcolor[WGDEFCOLOR] =  {
	RGB(255, 0, 0),   /* red */
	RGB(0, 255, 0),   /* green */
	RGB(0, 0, 255),   /* blue */
	RGB(255, 0, 255), /* magenta */
	RGB(0, 0, 128),   /* dark blue */
	RGB(128, 0, 0),   /* dark red */
	RGB(0, 128, 128), /* dark cyan */
	RGB(0, 0, 0),             /* black */
	RGB(128, 128, 128), /* grey */
	RGB(0, 128, 64),  /* very dark cyan */
	RGB(128, 128, 0), /* dark yellow */
	RGB(128, 0, 128), /* dark magenta */
	RGB(192, 192, 192), /* light grey */
	RGB(0, 255, 255), /* cyan */
	RGB(255, 255, 0), /* yellow */
};
#define WGDEFSTYLE 5
int wginitstyle[WGDEFSTYLE] = {PS_SOLID, PS_DASH, PS_DOT, PS_DASHDOT, PS_DASHDOTDOT};

#ifdef USE_WINGDI
/* bitmaps for filled boxes (ULIG) */
/* zeros represent the foreground color and ones represent the background color */
#define PATTERN_BITMAP_LENGTH 16
static const uchar pattern_bitmaps[][PATTERN_BITMAP_LENGTH] = {
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, /* no fill */
	{0xFE, 0xFE, 0x7D, 0x7D, 0xBB, 0xBB, 0xD7, 0xD7, 0xEF, 0xEF, 0xD7, 0xD7, 0xBB, 0xBB, 0x7D, 0x7D}, /* cross-hatch (1) */
	{0x77, 0x77, 0xAA, 0xAA, 0xDD, 0xDD, 0xAA, 0xAA, 0x77, 0x77, 0xAA, 0xAA, 0xDD, 0xDD, 0xAA, 0xAA}, /* double cross-hatch (2) */
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* solid fill (3) */
	{0x7F, 0x7F, 0xBF, 0xBF, 0xDF, 0xDF, 0xEF, 0xEF, 0xF7, 0xF7, 0xFB, 0xFB, 0xFD, 0xFD, 0xFE, 0xFE}, /* diagonals (4) */
	{0xFE, 0xFE, 0xFD, 0xFD, 0xFB, 0xFB, 0xF7, 0xF7, 0xEF, 0xEF, 0xDF, 0xDF, 0xBF, 0xBF, 0x7F, 0x7F}, /* diagonals (5) */
	{0x77, 0x77, 0x77, 0x77, 0xBB, 0xBB, 0xBB, 0xBB, 0xDD, 0xDD, 0xDD, 0xDD, 0xEE, 0xEE, 0xEE, 0xEE}, /* steep diagonals (6) */
	{0xEE, 0xEE, 0xEE, 0xEE, 0xDD, 0xDD, 0xDD, 0xDD, 0xBB, 0xBB, 0xBB, 0xBB, 0x77, 0x77, 0x77, 0x77} /* steep diagonals (7) */
#if (0)
	, {0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0x3F, 0x3F, 0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0x3F, 0x3F}, /* shallow diagonals (old 5) */
	{0x3F, 0x3F, 0xCF, 0xCF, 0xF3, 0xF3, 0xFC, 0xFC, 0x3F, 0x3F, 0xCF, 0xCF, 0xF3, 0xF3, 0xFC, 0xFC} /* shallow diagonals (old 6) */
#endif
};
#define pattern_num (sizeof(pattern_bitmaps)/(sizeof(*pattern_bitmaps)))
static HBRUSH pattern_brush[pattern_num];
static BITMAP pattern_bitdata[pattern_num];
static HBITMAP pattern_bitmap[pattern_num];

static bool brushes_initialized = FALSE;
#endif

/* Internal state of enhanced text processing.
   Do not access outside draw_enhanced_text, GraphEnhancedOpen or
   GraphEnhancedFlush.
 */
enhstate_struct enhstate;

#ifdef USE_WINGDI
static struct {
	HDC hdc; /* device context */
} enhstate_gdi;
#endif

/* ================================== */

/* prototypes for module-local functions */

LRESULT CALLBACK WndGraphProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam); // @callback
LRESULT CALLBACK WndGraphParentProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam); // @callback
INT_PTR CALLBACK LineStyleDlgProc(HWND hdlg, UINT wmsg, WPARAM wparam, LPARAM lparam); // @callback(DLGPROC)

static void     DestroyBlocks(GW * lpgw);
static BOOL     AddBlock(GW * lpgw);
static void     StorePen(GW * lpgw, int i, COLORREF ref, int colorstyle, int monostyle);
static void     MakePens(GW * lpgw, HDC hdc);
static void     DestroyPens(GW * lpgw);
static void     Wnd_GetTextSize(HDC hdc, LPCSTR str, size_t len, int * cx, int * cy);
static BOOL     GetPlotRect(GW * lpgw, LPRECT rect);
static void     MakeFonts(GW * lpgw, LPRECT lprect, HDC hdc);
static void     DestroyFonts(GW * lpgw);
static void     SelFont(GW * lpgw);
#ifdef USE_WINGDI
	static void     SetFont(GW * lpgw, HDC hdc);
	static void     GraphChangeFont(GW * lpgw, LPCTSTR font, int fontsize, HDC hdc, RECT rect);
	static void     dot(HDC hdc, int xdash, int ydash);
	static uint GraphGetTextLength(GW * lpgw, HDC hdc, LPCSTR text, bool escapes);
	static void     draw_text_justify(HDC hdc, int justify);
	static void     draw_put_text(GW * lpgw, HDC hdc, int x, int y, char * str, bool escapes);
	static void     draw_image(GW * lpgw, HDC hdc, char * image, POINT corners[4], uint width, uint height, int color_mode);
	static void     drawgraph(GW * lpgw, HDC hdc, LPRECT rect);
#endif
static void     CopyClip(GW * lpgw);
static void     SaveAsEMF(GW * lpgw);
static void     CopyPrint(GW * lpgw);
static void     WriteGraphIni(GW * lpgw);
static void     ReadGraphIni(GW * lpgw);
//static void     track_tooltip(GW * lpgw, int x, int y);
static COLORREF GetColor(HWND hwnd, COLORREF ref);

#define WIN_CUSTOM_PENS // @sobolev
#ifdef WIN_CUSTOM_PENS
	static void     UpdateColorSample(HWND hdlg);
	static BOOL     LineStyle(GW * lpgw);
#endif
static void GraphUpdateMenu(GW * lpgw);
// 
// Helper functions for GraphOp():
// 
/* destroy memory blocks holding graph operations */
static void DestroyBlocks(GW * lpgw)
{
	struct GWOPBLK * next;
	struct GWOP * gwop;
	uint i;
	struct GWOPBLK * p_this = lpgw->gwopblk_head;
	while(p_this) {
		next = p_this->next;
		SETIFZ(p_this->gwop, (struct GWOP *)GlobalLock(p_this->hblk));
		if(p_this->gwop) {
			// free all text strings within p_this block 
			gwop = p_this->gwop;
			for(i = 0; i<GWOPMAX; i++) {
				if(gwop->htext)
					LocalFree(gwop->htext);
				gwop++;
			}
		}
		GlobalUnlock(p_this->hblk);
		GlobalFree(p_this->hblk);
		LocalFreePtr(p_this);
		p_this = next;
	}
	lpgw->gwopblk_head = NULL;
	lpgw->gwopblk_tail = NULL;
	lpgw->nGWOP = 0;
}

/* add a new memory block for graph operations */
/* returns TRUE if block allocated */
static BOOL AddBlock(GW * lpgw)
{
	HGLOBAL hblk;
	struct GWOPBLK * p_this;
	// create new block 
	struct GWOPBLK * next = (struct GWOPBLK *)LocalAllocPtr(LHND, sizeof(struct GWOPBLK) );
	if(next == NULL)
		return FALSE;
	hblk = GlobalAlloc(GHND, GWOPMAX*sizeof(struct GWOP));
	if(hblk == NULL)
		return FALSE;
	next->hblk = hblk;
	next->gwop = (struct GWOP *)NULL;
	next->next = (struct GWOPBLK *)NULL;
	next->used = 0;
	/* attach it to list */
	p_this = lpgw->gwopblk_tail;
	if(p_this == NULL) {
		lpgw->gwopblk_head = next;
	}
	else {
		p_this->next = next;
		p_this->gwop = (struct GWOP *)NULL;
		GlobalUnlock(p_this->hblk);
	}
	lpgw->gwopblk_tail = next;
	next->gwop = (struct GWOP *)GlobalLock(next->hblk);
	return BIN(next->gwop);
}

//void GraphOp(GW * lpgw, UINT op, UINT x, UINT y, LPCSTR str)
void GW::Op(UINT op, UINT x, UINT y, LPCSTR str)
{
	if(str)
		OpSize(op, x, y, str, strlen(str) + 1);
	else
		OpSize(op, x, y, 0, 0);
}

//void GraphOpSize(GW * lpgw, UINT op, UINT x, UINT y, LPCSTR str, DWORD size)
void GW::OpSize(UINT op, UINT x, UINT y, LPCSTR str, DWORD size)
{
	struct GWOP * gwop;
	struct GWOPBLK * p_this = gwopblk_tail;
	if(!p_this || p_this->used >= GWOPMAX) {
		// not enough space so get new block 
		if(!AddBlock(this))
			return;
		p_this = gwopblk_tail;
	}
	gwop = &p_this->gwop[p_this->used];
	gwop->op = op;
	gwop->x = x;
	gwop->y = y;
	gwop->htext = 0;
	if(str) {
		gwop->htext = LocalAlloc(LHND, size);
		char * npstr = (char *)LocalLock(gwop->htext);
		if(gwop->htext && (npstr != (char *)NULL))
			memcpy(npstr, str, size);
		LocalUnlock(gwop->htext);
	}
	p_this->used++;
	nGWOP++;
	buffervalid = FALSE;
}
//
// Initialize the GW struct:
// set default values and read options from ini file 
//
void GraphInitStruct(GW * lpgw)
{
	if(!lpgw->initialized) {
#ifndef WIN_CUSTOM_PENS
		int i;
#endif
		lpgw->initialized = TRUE;
		if(lpgw != _WinM.P_ListGraphs) {
			TCHAR titlestr[100];
			// copy important fields from window #0 
			GW * graph0 = _WinM.P_ListGraphs;
			lpgw->IniFile = graph0->IniFile;
			lpgw->hInstance = graph0->hInstance;
			lpgw->hPrevInstance = graph0->hPrevInstance;
			lpgw->lptw = graph0->lptw;

			/* window title */
			wsprintf(titlestr, TEXT("%s %i"), WINGRAPHTITLE, lpgw->Id);
			lpgw->Title = _tcsdup(titlestr);
		}
		else {
			lpgw->Title = _tcsdup(WINGRAPHTITLE);
		}
		lpgw->fontscale = 1.;
		lpgw->linewidth = 1.;
		lpgw->pointscale = 1.;
		lpgw->color = TRUE;
		lpgw->dashed = FALSE;
		lpgw->IniSection = TEXT("WGNUPLOT");
		lpgw->fontsize = WINFONTSIZE;
		lpgw->maxkeyboxes = 0;
		lpgw->keyboxes = 0;
		lpgw->buffervalid = FALSE;
		lpgw->maxhideplots = MAXPLOTSHIDE;
		lpgw->hideplot = (BOOL*)SAlloc::C(MAXPLOTSHIDE, sizeof(BOOL));
		/* init pens */
#ifndef WIN_CUSTOM_PENS
		StorePen(lpgw, 0, RGB(0, 0, 0), PS_SOLID, PS_SOLID);
		StorePen(lpgw, 1, RGB(192, 192, 192), PS_DOT, PS_DOT);
		for(i = 0; i < WGNUMPENS; i++) {
			COLORREF ref = wginitcolor[i % WGDEFCOLOR];
			int colorstyle = wginitstyle[(i / WGDEFCOLOR) % WGDEFSTYLE];
			int monostyle  = wginitstyle[i % WGDEFSTYLE];
			StorePen(lpgw, i + 2, ref, colorstyle, monostyle);
		}
#endif
		ReadGraphIni(lpgw);
	}
}

/* Prepare Graph window for being displayed by windows, update
 * the window's menus and show it */
void GraphInit(GW * lpgw)
{
	HMENU sysmenu;
	WNDCLASS wndclass;
	if(!lpgw->hPrevInstance) {
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = WndGraphParentProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 2 * sizeof(void *);
		wndclass.hInstance = lpgw->hInstance;
		wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hCursor = NULL;
		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = szGraphParentClass;
		RegisterClass(&wndclass);

		wndclass.lpfnWndProc = WndGraphProc;
		wndclass.hIcon = NULL;
		wndclass.lpszClassName = szGraphClass;
		RegisterClass(&wndclass);
	}
	if(!lpgw->bDocked) {
		lpgw->hWndGraph = CreateWindow(szGraphParentClass, lpgw->Title, WS_OVERLAPPEDWINDOW,
			lpgw->Origin_.x, lpgw->Origin_.y, lpgw->Size_.x, lpgw->Size_.y, NULL, NULL, lpgw->hInstance, lpgw);
	}
#ifndef WGP_CONSOLE
	else {
		RECT rect;
		SIZE size;
		// Note: whatever we set here as initial window size will be overridden by DockedUpdateLayout() below.
		GetClientRect(_WinM.TxtWin.hWndParent, &rect);
		DockedGraphSize(lpgw->lptw, &size, TRUE);
		lpgw->Origin_.Set(rect.right - 200, _WinM.TxtWin.ButtonHeight);
		lpgw->Size_.Set(size.cx, size.cy);
		lpgw->hWndGraph = CreateWindow(szGraphParentClass, lpgw->Title, WS_CHILD, lpgw->Origin_.x, lpgw->Origin_.y,
			lpgw->Size_.x, lpgw->Size_.y, _WinM.TxtWin.hWndParent, NULL, lpgw->hInstance, lpgw);
	}
#endif
	if(lpgw->hWndGraph)
		SetClassLongPtr(lpgw->hWndGraph, GCLP_HICON, (LONG_PTR)LoadIcon(lpgw->hInstance, TEXT("GRPICON")));
	if(!lpgw->bDocked)
		lpgw->hStatusbar = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | (lpgw->bDocked ? 0 : SBARS_SIZEGRIP),
			0, 0, 0, 0, lpgw->hWndGraph, (HMENU)ID_GRAPHSTATUS, lpgw->hInstance, lpgw);
	if(lpgw->hStatusbar) {
		RECT rect;
		// auto-adjust size 
		::SendMessage(lpgw->hStatusbar, WM_SIZE, (WPARAM)0, (LPARAM)0);
		ShowWindow(lpgw->hStatusbar, SW_SHOWNOACTIVATE);
		// make room 
		GetWindowRect(lpgw->hStatusbar, &rect);
		lpgw->StatusHeight = rect.bottom - rect.top;
	}
	else {
		lpgw->StatusHeight = 0;
	}
	/* create a toolbar */
	lpgw->hToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD | TBSTYLE_LIST | TBSTYLE_TOOLTIPS, // TBSTYLE_WRAPABLE
		0, 0, 0, 0, lpgw->hWndGraph, (HMENU)ID_TOOLBAR, lpgw->hInstance, lpgw);
	if(lpgw->hToolbar) {
		RECT rect;
		int i;
		TBBUTTON button;
		BOOL ret;
		TCHAR buttontext[10];
		uint num = 0;
		UINT dpi = GetDPI();
		TBADDBITMAP bitmap = {0};
		::SendMessage(lpgw->hToolbar, TB_SETBITMAPSIZE, (WPARAM)0, (dpi > 96) ? MAKELPARAM(24, 24) : MAKELPARAM(16, 16));
		// load standard toolbar icons: standard, history & view 
		::SendMessage(lpgw->hToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
		bitmap.hInst = HINST_COMMCTRL;
		bitmap.nID = (dpi > 96)  ? IDB_STD_LARGE_COLOR : IDB_STD_SMALL_COLOR;
		::SendMessage(lpgw->hToolbar, TB_ADDBITMAP, 0, (WPARAM)&bitmap);
		bitmap.nID = (dpi > 96)  ? IDB_HIST_LARGE_COLOR : IDB_HIST_SMALL_COLOR;
		::SendMessage(lpgw->hToolbar, TB_ADDBITMAP, 0, (WPARAM)&bitmap);
		bitmap.nID = (dpi > 96)  ? IDB_VIEW_LARGE_COLOR : IDB_VIEW_SMALL_COLOR;
		::SendMessage(lpgw->hToolbar, TB_ADDBITMAP, 0, (WPARAM)&bitmap);
		// create buttons 
		memzero(&button, sizeof(button));
		button.fsState = TBSTATE_ENABLED;
		button.fsStyle = BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_NOPREFIX;
		button.iString = 0;
		// copy 
		button.iBitmap = STD_COPY;
		button.idCommand = M_COPY_CLIP;
		ret = ::SendMessage(lpgw->hToolbar, TB_INSERTBUTTON, (WPARAM)num++, (LPARAM)&button);
		// print 
		button.iBitmap = STD_PRINT;
		button.idCommand = M_PRINT;
		ret = ::SendMessage(lpgw->hToolbar, TB_INSERTBUTTON, (WPARAM)num++, (LPARAM)&button);
		// save as EMF 
		button.iBitmap = STD_FILESAVE;
		button.idCommand = M_SAVE_AS_EMF;
		ret = ::SendMessage(lpgw->hToolbar, TB_INSERTBUTTON, (WPARAM)num++, (LPARAM)&button);
		// options 
		button.iBitmap = STD_PROPERTIES;
		button.idCommand = 0; /*unused*/
		button.iString = (INT_PTR)TEXT("Options");
		button.fsStyle = BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_NOPREFIX | BTNS_WHOLEDROPDOWN;
		ret = ::SendMessage(lpgw->hToolbar, TB_INSERTBUTTON, (WPARAM)num++, (LPARAM)&button);
		// TODO: Add the following buttons: replot/refresh, toggle grid(?), previous/next zoom, autoscale, help
		button.fsStyle = BTNS_AUTOSIZE | BTNS_NOPREFIX | BTNS_SEP;
		ret = ::SendMessage(lpgw->hToolbar, TB_INSERTBUTTON, (WPARAM)num++, (LPARAM)&button);
		// hide grid 
		button.iBitmap = STD_CUT;
		button.idCommand = M_HIDEGRID;
		button.fsStyle = BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_NOPREFIX | BTNS_CHECK;
		button.iString = (INT_PTR)TEXT("Grid");
		ret = ::SendMessage(lpgw->hToolbar, TB_INSERTBUTTON, (WPARAM)num++, (LPARAM)&button);
		// hide graphs 
		for(i = 0; i < MAXPLOTSHIDE; i++) {
			button.iBitmap = STD_CUT;
			button.idCommand = M_HIDEPLOT + i;
			wsprintf(buttontext, TEXT("%i"), i + 1);
			button.iString = (UINT_PTR)buttontext;
			button.dwData = i;
			ret = ::SendMessage(lpgw->hToolbar, TB_INSERTBUTTON, (WPARAM)num++, (LPARAM)&button);
		}
		// auto-resize and show 
		::SendMessage(lpgw->hToolbar, TB_AUTOSIZE, (WPARAM)0, (LPARAM)0);
		ShowWindow(lpgw->hToolbar, SW_SHOWNOACTIVATE);
		// make room 
		GetWindowRect(lpgw->hToolbar, &rect);
		lpgw->ToolbarHeight = rect.bottom - rect.top;
	}
	lpgw->hPopMenu = CreatePopupMenu();
	// actions 
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_COPY_CLIP, TEXT("&Copy to Clipboard (Ctrl+C)"));
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_SAVE_AS_EMF, TEXT("&Save as EMF... (Ctrl+S)"));
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_SAVE_AS_BITMAP, TEXT("S&ave as Bitmap..."));
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_PRINT, TEXT("&Print..."));
	// settings 
	AppendMenu(lpgw->hPopMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->graphtotop ? MF_CHECKED : MF_UNCHECKED), M_GRAPH_TO_TOP, TEXT("Bring to &Top"));
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->color ? MF_CHECKED : MF_UNCHECKED), M_COLOR, TEXT("C&olor"));
	AppendMenu(lpgw->hPopMenu, MF_SEPARATOR, 0, NULL);
#ifdef USE_WINGDI
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->gdiplus ? MF_CHECKED : MF_UNCHECKED), M_GDI, TEXT("&GDI backend"));
#endif
#ifdef HAVE_GDIPLUS
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->gdiplus ? MF_CHECKED : MF_UNCHECKED),
	    M_GDIPLUS, TEXT("GDI&+ backend"));
#endif
#ifdef HAVE_D2D
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->d2d ? MF_CHECKED : MF_UNCHECKED),
	    M_D2D, TEXT("Direct&2D backend"));
#endif
	AppendMenu(lpgw->hPopMenu, MF_SEPARATOR, 0, NULL);
#if defined(HAVE_GDIPLUS) || defined(HAVE_D2D)
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->oversample ? MF_CHECKED : MF_UNCHECKED), M_OVERSAMPLE, TEXT("O&versampling"));
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->antialiasing ? MF_CHECKED : MF_UNCHECKED), M_ANTIALIASING, TEXT("&Antialiasing"));
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->polyaa ? MF_CHECKED : MF_UNCHECKED), M_POLYAA, TEXT("Antialiasing of pol&ygons"));
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->fastrotation ? MF_CHECKED : MF_UNCHECKED), M_FASTROTATE, TEXT("Fast &rotation w/o antialiasing"));
	AppendMenu(lpgw->hPopMenu, MF_SEPARATOR, 0, NULL);
#endif
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_BACKGROUND, TEXT("&Background..."));
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_CHOOSE_FONT, TEXT("&Font..."));
#ifdef WIN_CUSTOM_PENS
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_LINESTYLE, TEXT("&Line Styles..."));
#endif
	/* save settings */
	AppendMenu(lpgw->hPopMenu, MF_SEPARATOR, 0, NULL);
	if(lpgw->IniFile) {
		TCHAR buf[MAX_PATH];
		wsprintf(buf, TEXT("&Update %s"), lpgw->IniFile);
		AppendMenu(lpgw->hPopMenu, MF_STRING, M_WRITEINI, buf);
	}
	/* Update menu items */
	GraphUpdateMenu(lpgw);
	/* modify the system menu to have the new items we want */
	sysmenu = GetSystemMenu(lpgw->hWndGraph, 0);
	AppendMenu(sysmenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(sysmenu, MF_POPUP, (UINT_PTR)lpgw->hPopMenu, TEXT("&Options"));
	AppendMenu(sysmenu, MF_STRING, M_ABOUT, TEXT("&About"));
#ifndef WGP_CONSOLE
	if(!IsWindowVisible(lpgw->lptw->hWndParent)) {
		AppendMenu(sysmenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(sysmenu, MF_STRING, M_COMMANDLINE, TEXT("C&ommand Line"));
	}
#endif
	/* determine the size of the window decoration: border, toolbar, caption, statusbar etc. */
	{
		RECT wrect, rect;
		/* get real window size, not lpgw->Size */
		GetWindowRect(lpgw->hWndGraph, &wrect);
		GetClientRect(lpgw->hWndGraph, &rect);
		lpgw->Decoration_.Set(wrect.right - wrect.left + rect.left - rect.right, wrect.bottom - wrect.top + rect.top - rect.bottom + lpgw->ToolbarHeight + lpgw->StatusHeight);
		// 2020-10-07 shige: get real value of Size.{x,y} for CW_USEDEFAULT 
		if(lpgw->Size_.x == CW_USEDEFAULT || lpgw->Size_.y == CW_USEDEFAULT) {
			lpgw->Size_.Set(wrect.right - wrect.left, wrect.bottom - wrect.top);
		}
	}
	// resize to match requested canvas size 
	if(!lpgw->bDocked && lpgw->Canvas_.x) {
		lpgw->Size_.Set(lpgw->Canvas_.x + lpgw->Decoration_.x, lpgw->Canvas_.y + lpgw->Decoration_.y);
		SetWindowPos(lpgw->hWndGraph, HWND_BOTTOM, lpgw->Origin_.x, lpgw->Origin_.y, lpgw->Size_.x, lpgw->Size_.y, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
	}
	// Finally, create the window for the actual graph
	lpgw->hGraph = CreateWindow(szGraphClass, lpgw->Title, WS_CHILD, 0, lpgw->ToolbarHeight,
		lpgw->Size_.x - lpgw->Decoration_.x, lpgw->Size_.y - lpgw->Decoration_.y, lpgw->hWndGraph, NULL, lpgw->hInstance, lpgw);
	// initialize font (and pens)
	{
		RECT rect;
		HDC hdc = GetDC(lpgw->hGraph);
		MakePens(lpgw, hdc);
		GetPlotRect(lpgw, &rect);
		MakeFonts(lpgw, &rect, hdc);
		ReleaseDC(lpgw->hGraph, hdc);
	}
	ShowWindow(lpgw->hGraph, SW_SHOWNOACTIVATE);
	ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
#ifndef WGP_CONSOLE
	// update docked window layout 
	if(lpgw->bDocked)
		DockedUpdateLayout(lpgw->lptw);
#endif
}

void GraphUpdateWindowPosSize(GW * lpgw)
{
	// resize to match requested canvas size 
	if(lpgw->Canvas_.x != 0) {
		lpgw->Size_.Set(lpgw->Canvas_.x + lpgw->Decoration_.x, lpgw->Canvas_.y + lpgw->Decoration_.y);
	}
	SetWindowPos(lpgw->hWndGraph, HWND_BOTTOM, lpgw->Origin_.x, lpgw->Origin_.y, lpgw->Size_.x, lpgw->Size_.y,
	    SWP_NOACTIVATE | SWP_NOZORDER);
}
//
// close a graph window 
//
void GraphClose(GW * lpgw)
{
#ifdef USE_MOUSE
	Wnd_exec_event(lpgw, (LPARAM)0, GE_reset, 0); // Pass it through mouse handling to check for "bind Close" 
#endif
	// close window 
	::DestroyWindow(lpgw->hWndGraph);
	WinMessageLoop();
	lpgw->hWndGraph = NULL;
	lpgw->hGraph = NULL;
	lpgw->hStatusbar = NULL;
	lpgw->hToolbar = NULL;
#ifdef HAVE_D2D
	d2dReleaseRenderTarget(lpgw);
#endif
	lpgw->locked = TRUE;
	DestroyBlocks(lpgw);
	lpgw->locked = FALSE;
}

void GraphStart(GW * lpgw, double pointsize)
{
	lpgw->locked = TRUE;
	lpgw->buffervalid = FALSE;
	DestroyBlocks(lpgw);
	lpgw->org_pointsize = pointsize;
	if(!lpgw->hWndGraph || !IsWindow(lpgw->hWndGraph))
		GraphInit(lpgw);
	if(IsIconic(lpgw->hWndGraph) || !IsWindowVisible(lpgw->hWndGraph))
		ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
	if(lpgw->graphtotop) {
		/* HBB NEW 20040221: avoid grabbing the keyboard focus
		 * unless mouse mode is on */
#ifdef USE_MOUSE
		if(mouse_setting.on) {
#ifndef WGP_CONSOLE
			if(lpgw->bDocked)
				SetFocus(lpgw->hWndGraph);
			else
#endif
			BringWindowToTop(lpgw->hWndGraph);
			return;
		}
#endif /* USE_MOUSE */
		SetWindowPos(lpgw->hWndGraph, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE|SWP_NOSIZE);
	}
}

void GraphEnd(GW * lpgw)
{
	RECT rect;
	GetClientRect(lpgw->hGraph, &rect);
	InvalidateRect(lpgw->hGraph, &rect, 1);
	lpgw->buffervalid = FALSE;
	lpgw->locked = FALSE;
	UpdateWindow(lpgw->hGraph);
#ifdef USE_MOUSE
	gp_exec_event(GE_plotdone, 0, 0, 0, 0, lpgw->Id); /* notify main program */
#endif
}

/* shige */
void GraphChangeTitle(GW * lpgw)
{
	if(GraphHasWindow(lpgw))
		SetWindowText(lpgw->hWndGraph, lpgw->Title);
}

void GraphResume(GW * lpgw)
{
	lpgw->locked = TRUE;
}

void GraphPrint(GW * lpgw)
{
	if(GraphHasWindow(lpgw))
		SendMessage(lpgw->hWndGraph, WM_COMMAND, M_PRINT, 0L);
}

void GraphRedraw(GW * lpgw)
{
	lpgw->buffervalid = FALSE;
	if(GraphHasWindow(lpgw))
		SendMessage(lpgw->hGraph, WM_COMMAND, M_REBUILDTOOLS, 0L);
}

bool GraphHasWindow(GW * lpgw)
{
	return (lpgw && lpgw->hWndGraph && IsWindow(lpgw->hWndGraph));
}
//
// Helper functions for bookkeeping of pens, brushes and fonts 
//
// Set up LOGPEN structures based on information coming from wgnuplot.ini, via ReadGraphIni() 
//
static void StorePen(GW * lpgw, int i, COLORREF ref, int colorstyle, int monostyle)
{
	LOGPEN * plp = &lpgw->colorpen[i];
	plp->lopnColor = ref;
	if(colorstyle < 0) {
		plp->lopnWidth.x = -colorstyle;
		plp->lopnStyle = 0;
	}
	else {
		plp->lopnWidth.x = 1;
		plp->lopnStyle = colorstyle % 5;
	}
	plp->lopnWidth.y = 0;
	plp = &lpgw->monopen[i];
	plp->lopnColor = RGB(0, 0, 0);
	if(monostyle < 0) {
		plp->lopnWidth.x = -monostyle;
		plp->lopnStyle = 0;
	}
	else {
		plp->lopnWidth.x = 1;
		plp->lopnStyle = monostyle % 5;
	}
	plp->lopnWidth.y = 0;
}
//
// Prepare pens and brushes (--> colors) for use by the driver. Pens are (now) created
// on-the-fly (--> DeleteObject(SelectObject(...)) idiom), but the brushes are still
// all created statically, and kept until the window is closed 
//
static void MakePens(GW * lpgw, HDC hdc)
{
#ifdef USE_WINGDI
	LOGPEN pen;
	if(GetDeviceCaps(hdc, NUMCOLORS) == 2 || !lpgw->color) {
		pen = lpgw->monopen[1];
		pen.lopnWidth.x = static_cast<LONG>(pen.lopnWidth.x * lpgw->linewidth);
		lpgw->hapen = CreatePenIndirect(&pen); // axis 
		lpgw->hbrush = CreateSolidBrush(lpgw->background);
		for(int i = 0; i < WGNUMPENS + 2; i++)
			lpgw->colorbrush[i] = CreateSolidBrush(lpgw->monopen[i].lopnColor);
	}
	else {
		pen = lpgw->colorpen[1];
		pen.lopnWidth.x = static_cast<LONG>(pen.lopnWidth.x * lpgw->linewidth);
		lpgw->hapen = CreatePenIndirect(&pen); // axis 
		lpgw->hbrush = CreateSolidBrush(lpgw->background);
		for(int i = 0; i < WGNUMPENS + 2; i++)
			lpgw->colorbrush[i] = CreateSolidBrush(lpgw->colorpen[i].lopnColor);
	}
	lpgw->hnull = CreatePen(PS_NULL, 0, 0); /* border for filled areas */
	// build pattern brushes for filled boxes (ULIG) 
	if(!brushes_initialized) {
		for(int i = 0; i < pattern_num; i++) {
			pattern_bitdata[i].bmType       = 0;
			pattern_bitdata[i].bmWidth      = 16;
			pattern_bitdata[i].bmHeight     = 8;
			pattern_bitdata[i].bmWidthBytes = 2;
			pattern_bitdata[i].bmPlanes     = 1;
			pattern_bitdata[i].bmBitsPixel  = 1;
			pattern_bitdata[i].bmBits       = (char *)pattern_bitmaps[i];
			pattern_bitmap[i] = CreateBitmapIndirect(&pattern_bitdata[i]);
			pattern_brush[i] = CreatePatternBrush(pattern_bitmap[i]);
		}
		brushes_initialized = TRUE;
	}
#endif
}
//
// Undo effect of MakePens(). To be called just before the window is closed. 
//
static void DestroyPens(GW * lpgw)
{
#ifdef USE_WINGDI
	DeleteObject(lpgw->hbrush);
	DeleteObject(lpgw->hnull);
	DeleteObject(lpgw->hapen);
	DeleteObject(lpgw->hsolid);
	for(int i = 0; i < WGNUMPENS + 2; i++)
		DeleteObject(lpgw->colorbrush[i]);
	DeleteObject(lpgw->hnull);
	// delete brushes used for filled areas 
	if(brushes_initialized) {
		for(int i = 0; i < pattern_num; i++) {
			DeleteObject(pattern_bitmap[i]);
			DeleteObject(pattern_brush[i]);
		}
		brushes_initialized = FALSE;
	}
#endif
}

/* ================================== */

/* HBB 20010218: new function. An isolated snippet from MakeFont(), now also
 * used in Wnd_put_tmptext() to size the temporary bitmap. */
static void Wnd_GetTextSize(HDC hdc, LPCSTR str, size_t len, int * cx, int * cy)
{
	SIZE size;
	// FIXME: Do we need to call the Unicode version here? 
	GetTextExtentPoint32A(hdc, str, len, &size);
	*cx = size.cx;
	*cy = size.cy;
}

static BOOL GetPlotRect(GW * lpgw, LPRECT rect)
{
	return GetClientRect(lpgw->hGraph, rect);
}

#ifdef USE_WINGDI
static void GetPlotRectInMM(GW * lpgw, LPRECT rect, HDC hdc)
{
	GetPlotRect(lpgw, rect);
	// Taken from http://msdn.microsoft.com/en-us/library/dd183519(VS.85).aspx
	//
	// Determine the picture frame dimensions.
	// iWidthMM is the display width in millimeters.
	// iHeightMM is the display height in millimeters.
	// iWidthPels is the display width in pixels.
	// iHeightPels is the display height in pixels
	int iWidthMM = GetDeviceCaps(hdc, HORZSIZE);
	int iHeightMM = GetDeviceCaps(hdc, VERTSIZE);
	int iWidthPels = GetDeviceCaps(hdc, HORZRES);
	int iHeightPels = GetDeviceCaps(hdc, VERTRES);
	// Convert client coordinates to .01-mm units.
	// Use iWidthMM, iWidthPels, iHeightMM, and
	// iHeightPels to determine the number of
	// .01-millimeter units per pixel in the x-
	//  and y-directions.
	rect->left = (rect->left * iWidthMM * 100) / iWidthPels;
	rect->top = (rect->top * iHeightMM * 100) / iHeightPels;
	rect->right = (rect->right * iWidthMM * 100) / iWidthPels;
	rect->bottom = (rect->bottom * iHeightMM * 100) / iHeightPels;
}

static bool TryCreateFont(GW * lpgw, LPCTSTR fontface, HDC hdc)
{
	if(fontface)
		_tcsncpy(lpgw->lf.lfFaceName, fontface, LF_FACESIZE);
	lpgw->hfonth = CreateFontIndirect(&(lpgw->lf));
	if(lpgw->hfonth != 0) {
		// Test if we actually got the requested font. Note that this also seems to work
		// with GDI's magic font substitutions (e.g. Helvetica->Arial, Times->Times New Roman) 
		HFONT hfontold = (HFONT)SelectObject(hdc, lpgw->hfonth);
		if(hfontold) {
			TCHAR fontname[MAXFONTNAME];
			GetTextFace(hdc, MAXFONTNAME, fontname);
			SelectObject(hdc, hfontold);
			if(_tcscmp(fontname, lpgw->lf.lfFaceName) == 0)
				return TRUE;
			else {
				FPRINTF((stderr, "Warning: GDI would have substituted \"%s\" for \"%s\"\n", fontname, lpgw->lf.lfFaceName));
				DeleteObject(lpgw->hfonth);
				lpgw->hfonth = 0;
				return FALSE;
			}
		}
		return TRUE;
	}
	else
		return FALSE;
	return FALSE;
}

#endif

static void MakeFonts(GW * lpgw, LPRECT lprect, HDC hdc)
{
#ifdef USE_WINGDI
	HFONT hfontold;
	TEXTMETRIC tm;
	int result;
	LPTSTR p;
	int cx, cy;
#endif

#ifdef HAVE_D2D
	lpgw->dpi = GetDeviceCaps(hdc, LOGPIXELSY);
#endif

#ifdef HAVE_GDIPLUS
	if(lpgw->gdiplus) {
#ifdef FASTROT_WITH_GDI
		if(!(lpgw->rotating && lpgw->fastrotation)) {
#endif
		InitFont_gdiplus(lpgw, hdc, lprect);
		return;
#ifdef FASTROT_WITH_GDI
	}
#endif
	}
#endif
#ifdef HAVE_D2D
	if(lpgw->d2d) {
		InitFont_d2d(lpgw, hdc, lprect);
		return;
	}
#endif

#ifdef USE_WINGDI
	lpgw->rotate = FALSE;
	memzero(&(lpgw->lf), sizeof(LOGFONT));
	_tcsncpy(lpgw->lf.lfFaceName, lpgw->fontname, LF_FACESIZE);
	lpgw->lf.lfHeight = -MulDiv(static_cast<int>(lpgw->fontsize * lpgw->fontscale), GetDeviceCaps(hdc, LOGPIXELSY), 72);
	lpgw->lf.lfCharSet = DEFAULT_CHARSET;
	if(((p = _tcsstr(lpgw->fontname, TEXT(" Italic"))) != NULL) || ((p = _tcsstr(lpgw->fontname, TEXT(":Italic"))) != NULL)) {
		lpgw->lf.lfFaceName[(uint)(p - lpgw->fontname)] = '\0';
		lpgw->lf.lfItalic = TRUE;
	}
	if(((p = _tcsstr(lpgw->fontname, TEXT(" Bold"))) != NULL) || ((p = _tcsstr(lpgw->fontname, TEXT(":Bold"))) != NULL)) {
		lpgw->lf.lfFaceName[(uint)(p - lpgw->fontname)] = '\0';
		lpgw->lf.lfWeight = FW_BOLD;
	}
	lpgw->lf.lfOutPrecision = OUT_OUTLINE_PRECIS;
	lpgw->lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lpgw->lf.lfQuality = lpgw->antialiasing ? CLEARTYPE_QUALITY : PROOF_QUALITY;
	// Handle zero length type face name by replacing with default name 
	if(lpgw->lf.lfFaceName[0] == 0)
		_tcsncpy(lpgw->lf.lfFaceName, GraphDefaultFont(), LF_FACESIZE);
	if(!TryCreateFont(lpgw, NULL, hdc)) {
		//static const char warn_font_not_available[] =
		//	"Warning:  font \"" TCHARFMT "\" not available, trying \"" TCHARFMT "\" instead.\n";
		static const char err_giving_up[] = "Error:  could not substitute another font. Giving up.\n";
		if(_tcscmp(lpgw->fontname, lpgw->deffontname) != 0) {
			//fprintf(stderr, warn_font_not_available, lpgw->fontname, lpgw->deffontname);
			if(!TryCreateFont(lpgw, lpgw->deffontname, hdc)) {
				if(_tcscmp(lpgw->deffontname, GraphDefaultFont()) != 0) {
					//fprintf(stderr, warn_font_not_available, lpgw->deffontname,
					// GraphDefaultFont());
					if(!TryCreateFont(lpgw, GraphDefaultFont(), hdc))
						fprintf(stderr, err_giving_up);
				}
				else
					fprintf(stderr, err_giving_up);
			}
		}
		else {
			if(_tcscmp(lpgw->fontname, GraphDefaultFont()) != 0) {
				//fprintf(stderr, warn_font_not_available, lpgw->fontname, GraphDefaultFont());
				if(!TryCreateFont(lpgw, GraphDefaultFont(), hdc))
					fprintf(stderr, err_giving_up);
			}
			else
				fprintf(stderr, "Error:  font \"" TCHARFMT "\" not available, but don't know which font to substitute.\n", lpgw->fontname);
		}
	}
	// we do need a 90 degree font 
	if(lpgw->hfontv)
		DeleteObject(lpgw->hfontv);
	lpgw->lf.lfEscapement = 900;
	lpgw->lf.lfOrientation = 900;
	lpgw->hfontv = CreateFontIndirect(&(lpgw->lf));
	// save text size 
	hfontold = (HFONT)SelectObject(hdc, lpgw->hfonth);
	Wnd_GetTextSize(hdc, STextConst::P_Digits, 10, &cx, &cy);
	lpgw->ChrS.y = MulDiv(cy, lpgw->MaxS.y, lprect->bottom - lprect->top);
	lpgw->ChrS.x = MulDiv(cx, lpgw->MaxS.x, 10 * (lprect->right - lprect->left));
	// CMW: Base tick size on character size 
	lpgw->TicS.x = MulDiv(lpgw->ChrS.x, 2, 5);
	cy = MulDiv(cx, 2 * GetDeviceCaps(hdc, LOGPIXELSY), 50 * GetDeviceCaps(hdc, LOGPIXELSX));
	lpgw->TicS.y = MulDiv(cy, lpgw->MaxS.y, lprect->bottom - lprect->top);
	// find out if we can rotate text 90deg 
	SelectObject(hdc, lpgw->hfontv);
	result = GetDeviceCaps(hdc, TEXTCAPS);
	if((result & TC_CR_90) || (result & TC_CR_ANY))
		lpgw->rotate = TRUE;
	GetTextMetrics(hdc, (TEXTMETRIC*)&tm);
	if(tm.tmPitchAndFamily & TMPF_VECTOR)
		lpgw->rotate = TRUE; // vector fonts can all be rotated 
	if(tm.tmPitchAndFamily & TMPF_TRUETYPE)
		lpgw->rotate = TRUE; // truetype fonts can all be rotated 
	// store text metrics for later use 
	lpgw->tmHeight = tm.tmHeight;
	lpgw->tmAscent = tm.tmAscent;
	lpgw->tmDescent = tm.tmDescent;
	SelectObject(hdc, hfontold);
#endif
}

static void DestroyFonts(GW * lpgw)
{
#ifdef USE_WINGDI
	if(lpgw->hfonth) {
		DeleteObject(lpgw->hfonth);
		lpgw->hfonth = 0;
	}
	if(lpgw->hfontv) {
		DeleteObject(lpgw->hfontv);
		lpgw->hfontv = 0;
	}
#endif
}

#ifdef USE_WINGDI
	static void SetFont(GW * lpgw, HDC hdc)
	{
		SelectObject(hdc, lpgw->hfonth);
		if(lpgw->rotate && lpgw->angle) {
			if(lpgw->hfontv)
				DeleteObject(lpgw->hfontv);
			lpgw->lf.lfEscapement = lpgw->lf.lfOrientation  = lpgw->angle * 10;
			lpgw->hfontv = CreateFontIndirect(&(lpgw->lf));
			if(lpgw->hfontv)
				SelectObject(hdc, lpgw->hfontv);
		}
	}
#endif

static void SelFont(GW * lpgw)
{
	LOGFONT lf;
	CHOOSEFONT cf;
	HDC hdc;
	TCHAR * p;
	/* Set all structure fields to zero. */
	memzero(&cf, sizeof(CHOOSEFONT));
	memzero(&lf, sizeof(LOGFONT));
	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = lpgw->hWndGraph;
	_tcsncpy(lf.lfFaceName, lpgw->fontname, LF_FACESIZE);
	if((p = _tcsstr(lpgw->fontname, TEXT(" Bold"))) != NULL) {
		lf.lfWeight = FW_BOLD;
		lf.lfFaceName[p - lpgw->fontname] = '\0';
	}
	else {
		lf.lfWeight = FW_NORMAL;
	}
	if((p = _tcsstr(lpgw->fontname, TEXT(" Italic"))) != NULL) {
		lf.lfItalic = TRUE;
		lf.lfFaceName[p - lpgw->fontname] = '\0';
	}
	else {
		lf.lfItalic = FALSE;
	}
	lf.lfCharSet = DEFAULT_CHARSET;
	hdc = GetDC(lpgw->hGraph);
	lf.lfHeight = -MulDiv(lpgw->fontsize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(lpgw->hGraph, hdc);
	cf.lpLogFont = &lf;
	cf.nFontType = SCREEN_FONTTYPE;
	cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_SCALABLEONLY;
	if(ChooseFont(&cf)) {
		_tcscpy(lpgw->fontname, lf.lfFaceName);
		lpgw->fontsize = cf.iPointSize / 10;
		if(cf.nFontType & BOLD_FONTTYPE)
			_tcscat(lpgw->fontname, TEXT(" Bold"));
		if(cf.nFontType & ITALIC_FONTTYPE)
			_tcscat(lpgw->fontname, TEXT(" Italic"));
		/* set current font as default font */
		_tcscpy(lpgw->deffontname, lpgw->fontname);
		lpgw->deffontsize = lpgw->fontsize;
		SendMessage(lpgw->hGraph, WM_COMMAND, M_REBUILDTOOLS, 0L);
	}
}

#ifdef USE_MOUSE
/* ================================== */

static void LoadCursors(GW * lpgw)
{
	// 3 of them are standard cursor shapes: 
	lpgw->hptrDefault = LoadCursor(NULL, IDC_ARROW);
	lpgw->hptrZooming = LoadCursor(NULL, IDC_SIZEALL);
	lpgw->hptrCrossHair = LoadCursor(NULL, IDC_CROSS);
	// the other 2 are kept in the resource file: 
	lpgw->hptrScaling = LoadCursor(lpgw->hInstance, MAKEINTRESOURCE(IDC_SCALING));
	lpgw->hptrRotating = LoadCursor(lpgw->hInstance, MAKEINTRESOURCE(IDC_ROTATING));
	lpgw->hptrCurrent = lpgw->hptrCrossHair;
}

static void DestroyCursors(GW * lpgw)
{
	/* No-op. Cursors from LoadCursor() don't need destroying */
}

#endif /* USE_MOUSE */

/* ================================== */

#ifdef USE_WINGDI
static void dot(HDC hdc, int xdash, int ydash)
{
	MoveTo(hdc, xdash, ydash);
	LineTo(hdc, xdash, ydash + 1);
}
#endif

uint luma_from_color(uint red, uint green, uint blue)
{
	/* convert to gray */
	return (uint)(red * 0.30 + green * 0.59 + blue * 0.11);
}

#ifdef USE_WINGDI
static uint GraphGetTextLength(GW * lpgw, HDC hdc, LPCSTR text, bool escapes)
{
	if(!text)
		return 0;
	else {
		SIZE size;
		LPWSTR textw = escapes ? UnicodeTextWithEscapes(text, lpgw->encoding) : UnicodeText(text, lpgw->encoding);
		if(textw) {
			GetTextExtentPoint32W(hdc, textw, wcslen(textw), &size);
			SAlloc::F(textw);
		}
		else {
			GetTextExtentPoint32A(hdc, text, strlen(text), &size);
		}
		return size.cx;
	}
}
#endif

/* local enhanced text helper functions */
#ifdef USE_WINGDI

static void EnhancedSetFont()
{
	GraphChangeFont(enhstate.lpgw, enhstate.fontname, static_cast<int>(enhstate.fontsize), enhstate_gdi.hdc, *(enhstate.rect));
	SetFont(enhstate.lpgw, enhstate_gdi.hdc);
}

static uint EnhancedTextLength(char * text)
{
	return GraphGetTextLength(enhstate.lpgw, enhstate_gdi.hdc, text, TRUE);
}

static void EnhancedPutText(int x, int y, char * text)
{
	draw_put_text(enhstate.lpgw, enhstate_gdi.hdc, x, y, text, TRUE);
}

static void EnhancedCleanup()
{
	// restore text alignment 
	draw_text_justify(enhstate_gdi.hdc, enhstate.lpgw->justify);
}

static void draw_enhanced_init(HDC hdc)
{
	enhstate.set_font = &EnhancedSetFont;
	enhstate.text_length = &EnhancedTextLength;
	enhstate.put_text = &EnhancedPutText;
	enhstate.cleanup = &EnhancedCleanup;
	enhstate_gdi.hdc = hdc;
	enhstate.res_scale = GetDeviceCaps(hdc, LOGPIXELSY) / 96.;
	SetTextAlign(hdc, TA_LEFT | TA_BASELINE);
}

#endif // USE_WINGDI

/* enhanced text functions shared with wgdiplus.cpp/wd2d.cpp */

LPWSTR UnicodeTextWithEscapes(LPCSTR str, enum set_encoding_id encoding)
{
	LPWSTR p;
	LPWSTR textw = UnicodeText(str, encoding);
	if(encoding == S_ENC_UTF8)
		return textw;  // Escapes already handled in core gnuplot
	p = wcsstr(textw, L"\\");
	if(p) {
		// make a copy of the string
		LPWSTR w = (LPWSTR)SAlloc::M(wcslen(textw) * sizeof(WCHAR));
		wcsncpy(w, textw, (p - textw));
		// q points at end of new string
		LPWSTR q = w + (p - textw);
		// r is the remaining string to copy
		LPWSTR r = p;
		*q = 0;
		do {
			uint32_t codepoint;
			size_t length = 0;
			WCHAR wstr[3];
			// copy intermediate characters
			if(p > r) {
				size_t n = (p - r);
				wcsncat(w, r, n);
				r += n;
				q += n;
			}
			// Handle Unicode escapes
			if(p[1] == L'U' && p[2] == L'+') {
				swscanf(&(p[3]), L"%5x", &codepoint);
				// Windows does not offer an API for direct conversion from UTF-32 to UTF-16.
				// So we convert "by hand".
				if((codepoint <= 0xD7FF) || (codepoint >= 0xE000 && codepoint <= 0xFFFF)) {
					wstr[0] = codepoint;
					length = 1;
				}
				else if(codepoint <= 0x10FFFF) {
					codepoint -= 0x10000;
					wstr[0] = 0xD800 + (codepoint >> 10);
					wstr[1] = 0xDC00 + (codepoint & 0x3FF);
					length = 2;
				}
			}
			if(length > 0) {
				p += (codepoint > 0xFFFF) ? 8 : 7;
				for(size_t i = 0; i < length; i++, q++)
					*q = wstr[i];
			}
			else if(p[1] == '\\') {
				p++;
			}
			else {
				*q = L'\\';
				q++;
			}
			*q = 0;
			r = p;
			p++;
			p = wcsstr(p, L"\\U+");
		} while(p);
		if(r)
			wcscat(w, r);
		SAlloc::F(textw);
		return w;
	}
	return textw;
}

void GraphEnhancedOpen(GpTermEntry * pThis, const char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
{
	GnuPlot * p_gp = pThis->P_Gp;
	const int win_scale = 1; /* scaling of base offset */
	// There are two special cases:
	// overprint = 3 means save current position
	// overprint = 4 means restore saved position
	if(overprint == 3) {
		enhstate.PtPreserve = enhstate.Pt;
		return;
	}
	else if(overprint == 4) {
		enhstate.Pt = enhstate.PtPreserve;
		return;
	}
	if(!enhstate.opened_string) {
		// Start new text fragment 
		enhstate.opened_string = TRUE;
		p_gp->Enht.P_CurText = p_gp->Enht.Text;
		// Keep track of whether we are supposed to show this string 
		enhstate.show = showflag;
		// 0/1/2  no overprint / 1st pass / 2nd pass 
		enhstate.overprint = overprint;
		// widthflag FALSE means do not update text position after printing 
		enhstate.widthflag = widthflag;
		// Select font 
		if(!isempty(fontname)) {
#ifdef UNICODE
			MultiByteToWideChar(CP_ACP, 0, fontname, -1, enhstate.fontname, MAXFONTNAME);
#else
			strcpy(enhstate.fontname, fontname);
#endif
		}
		else {
			_tcscpy(enhstate.fontname, enhstate.lpgw->deffontname);
		}
		enhstate.fontsize = fontsize;
		enhstate.set_font();
		// Scale fractional font height to vertical units of display 
		// TODO: Proper use of OUTLINEFONTMETRICS would yield better results. 
		enhstate.base = win_scale * base * enhstate.lpgw->fontscale * enhstate.res_scale;
	}
}

void GraphEnhancedFlush()
{
	int width, height;
	uint x, y, len;
	double angle = SMathConst::PiDiv180 * enhstate.lpgw->angle;
	if(enhstate.opened_string) {
		*GPO.Enht.P_CurText = '\0';
		// print the string fragment, perhaps invisibly 
		// NB: base expresses offset from current y pos 
		x = static_cast<uint>(enhstate.Pt.x - enhstate.base * sin(angle));
		y = static_cast<uint>(enhstate.Pt.y - enhstate.base * cos(angle));
		// calculate length of string first 
		len = enhstate.text_length(GPO.Enht.Text);
		width  = static_cast<int>(cos(angle) * len);
		height = static_cast<int>(-sin(angle) * len);
		if(enhstate.widthflag && !enhstate.sizeonly && !enhstate.overprint) {
			// do not take rotation into account 
			int ypos = static_cast<int>(-enhstate.base);
			enhstate.totalwidth += len;
			SETMIN(enhstate.totalasc,  ypos + enhstate.shift - enhstate.lpgw->tmAscent);
			SETMAX(enhstate.totaldesc, ypos + enhstate.shift + enhstate.lpgw->tmDescent);
		}
		// display string 
		if(enhstate.show && !enhstate.sizeonly)
			enhstate.put_text(x, y, GPO.Enht.Text);
		// update drawing position according to text length 
		if(!enhstate.widthflag) {
			width = 0;
			height = 0;
		}
		if(enhstate.sizeonly) {
			// This is the first pass for justified printing.        
			// We just adjust the starting position for second pass. 
			if(enhstate.lpgw->justify == RIGHT) {
				enhstate.Pt.x -= width;
				enhstate.Pt.y -= height;
			}
			else if(enhstate.lpgw->justify == CENTRE) {
				enhstate.Pt.x -= width / 2;
				enhstate.Pt.y -= height / 2;
			}
			// nothing to do for LEFT justified text 
		}
		else if(enhstate.overprint == 1) {
			// Save current position 
			enhstate.PtPreserve.Set(enhstate.Pt.x + width, enhstate.Pt.y + height);
			// First pass of overprint, leave position in center of fragment 
			enhstate.Pt.x += width / 2;
			enhstate.Pt.y += height / 2;
		}
		else if(enhstate.overprint == 2) {
			// Restore current position,
			// this sets the position behind the overprinted text 
			enhstate.Pt = enhstate.PtPreserve;
		}
		else {
			// Normal case is to update position to end of fragment 
			enhstate.Pt.x += width;
			enhstate.Pt.y += height;
		}
		enhstate.opened_string = FALSE;
	}
}

int draw_enhanced_text(GW * lpgw, LPRECT rect, int x, int y, const char * str)
{
	const char * original_string = str;
	uint pass, num_passes;
	GpTermEntry * tsave;
	TCHAR save_fontname[MAXFONTNAME];
	int save_fontsize;
	// Init enhanced text state 
	enhstate.lpgw = lpgw;
	enhstate.rect = rect;
	enhstate.opened_string = FALSE;
	_tcscpy(enhstate.fontname, lpgw->fontname);
	enhstate.fontsize = lpgw->fontsize;
	// Store the start position 
	enhstate.Pt.Set(x, y);
	enhstate.totalwidth = 0;
	enhstate.totalasc = 0;
	enhstate.totaldesc = 0;
	// Save font information 
	_tcscpy(save_fontname, lpgw->fontname);
	save_fontsize = lpgw->fontsize;
	// Set up global variables needed by enhanced_recursion() 
	GPO.Enht.FontScale = 1.0;
	strnzcpy(GPO.Enht.EscapeFormat, "%c", sizeof(GPO.Enht.EscapeFormat));
	// Text justification requires two passes. During the first pass we 
	// don't draw anything, we just measure the space it will take.     
	// Without justification one pass is enough                         
	if(enhstate.lpgw->justify == LEFT) {
		num_passes = 1;
	}
	else {
		num_passes = 2;
		enhstate.sizeonly = TRUE;
	}
	// We actually print everything left to right. 
	// Adjust baseline position: 
	enhstate.shift = lpgw->tmHeight/2 - lpgw->tmDescent;
	enhstate.Pt.x += sin(lpgw->angle * SMathConst::PiDiv180) * enhstate.shift;
	enhstate.Pt.y += cos(lpgw->angle * SMathConst::PiDiv180) * enhstate.shift;
	// enhanced_recursion() uses the callback functions of the current terminal. So we have to switch temporarily. 
	if(WIN_term) {
		tsave = GPT.P_Term;
		GPT.P_Term = WIN_term;
	}
	for(pass = 1; pass <= num_passes; pass++) {
		/* Set the recursion going. We say to keep going until a
		 * closing brace, but we don't really expect to find one.
		 * If the return value is not the nul-terminator of the
		 * string, that can only mean that we did find an unmatched
		 * closing brace in the string. We increment past it (else
		 * we get stuck in an infinite loop) and try again.
		 */
#ifdef UNICODE
		char save_fontname_a[MAXFONTNAME];
		WideCharToMultiByte(CP_ACP, 0, save_fontname, MAXFONTNAME, save_fontname_a, MAXFONTNAME, 0, 0);
#else
		char * save_fontname_a = save_fontname;
#endif
		while(*(str = enhanced_recursion(GPT.P_Term, str, TRUE, save_fontname_a, save_fontsize, 0.0, TRUE, TRUE, 0))) {
			GraphEnhancedFlush();
			if(!*++str)
				break; /* end of string */
			/* else carry on and process the rest of the string */
		}
		/* In order to do text justification we need to do a second pass
		 * that uses information stored during the first pass, see
		 * GraphEnhancedFlush().
		 */
		if(pass == 1) {
			/* do the actual printing in the next pass */
			enhstate.sizeonly = FALSE;
			str = original_string;
		}
	}
	// restore terminal 
	if(WIN_term) 
		GPT.P_Term = tsave;
	// restore font 
	_tcscpy(enhstate.fontname, save_fontname);
	enhstate.fontsize = save_fontsize;
	enhstate.set_font();
	// clean-up 
	enhstate.cleanup();
	return enhstate.totalwidth;
}

void draw_get_enhanced_text_extend(PRECT extend)
{
	switch(enhstate.lpgw->justify) {
		case LEFT:
		    extend->left  = 0;
		    extend->right = enhstate.totalwidth;
		    break;
		case CENTRE:
		    extend->left  = enhstate.totalwidth / 2;
		    extend->right = enhstate.totalwidth / 2;
		    break;
		case RIGHT:
		    extend->left  = enhstate.totalwidth;
		    extend->right = 0;
		    break;
	}
	extend->top = -enhstate.totalasc;
	extend->bottom = enhstate.totaldesc;
}

#ifdef USE_WINGDI
static void draw_text_justify(HDC hdc, int justify)
{
	switch(justify) {
		case LEFT: SetTextAlign(hdc, TA_LEFT | TA_TOP); break;
		case RIGHT: SetTextAlign(hdc, TA_RIGHT | TA_TOP); break;
		case CENTRE: SetTextAlign(hdc, TA_CENTER | TA_TOP); break;
	}
}

static void draw_put_text(GW * lpgw, HDC hdc, int x, int y, char * str, bool escapes)
{
	SetBkMode(hdc, TRANSPARENT);
	// support text encoding 
	if((lpgw->encoding == S_ENC_DEFAULT) || (lpgw->encoding == S_ENC_INVALID)) {
		TextOutA(hdc, x, y, str, strlen(str));
	}
	else {
		LPWSTR textw = escapes ? UnicodeTextWithEscapes(str, lpgw->encoding) : UnicodeText(str, lpgw->encoding);
		if(textw) {
			TextOutW(hdc, x, y, textw, wcslen(textw));
			SAlloc::F(textw);
		}
		else {
			// print this only once 
			if(lpgw->encoding != lpgw->encoding_error) {
				fprintf(stderr, "windows terminal: encoding %s not supported\n", encoding_names[lpgw->encoding]);
				lpgw->encoding_error = lpgw->encoding;
			}
			// fall back to standard encoding 
			TextOutA(hdc, x, y, str, strlen(str));
		}
	}
	SetBkMode(hdc, OPAQUE);
}

static void draw_new_pens(GW * lpgw, HDC hdc, LOGPEN cur_penstruct)
{
	HPEN old_hapen = lpgw->hapen;
	HPEN old_hsolid = lpgw->hsolid;
	if(cur_penstruct.lopnWidth.x <= 1) {
		/* shige: work-around for Windows clipboard bug */
		lpgw->hapen = CreatePenIndirect((LOGPEN*)&cur_penstruct);
		lpgw->hsolid = CreatePen(PS_SOLID, 1, cur_penstruct.lopnColor);
	}
	else {
		LOGBRUSH lb;
		lb.lbStyle = BS_SOLID;
		lb.lbColor = cur_penstruct.lopnColor;
		lpgw->hapen = ExtCreatePen(PS_GEOMETRIC | cur_penstruct.lopnStyle | 
			(lpgw->rounded ? PS_ENDCAP_ROUND | PS_JOIN_ROUND : PS_ENDCAP_SQUARE | PS_JOIN_MITER), cur_penstruct.lopnWidth.x, &lb, 0, 0);
		lpgw->hsolid = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | (lpgw->rounded ? PS_ENDCAP_ROUND | PS_JOIN_ROUND : PS_ENDCAP_SQUARE | PS_JOIN_MITER),
			cur_penstruct.lopnWidth.x, &lb, 0, 0);
	}
	SelectObject(hdc, lpgw->hapen);
	DeleteObject(old_hapen);
	DeleteObject(old_hsolid);
}

static void draw_new_brush(GW * lpgw, HDC hdc, COLORREF color)
{
	HBRUSH new_brush = CreateSolidBrush(color);
	SelectObject(hdc, new_brush);
	if(lpgw->hcolorbrush)
		DeleteObject(lpgw->hcolorbrush);
	lpgw->hcolorbrush = new_brush;
}

#endif // USE_WINGDI

void draw_update_keybox(GW * lpgw, uint plotno, int x, int y)
{
	if(plotno) {
		if(plotno > lpgw->maxkeyboxes) {
			lpgw->maxkeyboxes += 10;
			lpgw->keyboxes = static_cast<RECT *>(SAlloc::R(lpgw->keyboxes, lpgw->maxkeyboxes * sizeof(RECT)));
			for(uint i = plotno - 1; i < lpgw->maxkeyboxes; i++) {
				lpgw->keyboxes[i].left = INT_MAX;
				lpgw->keyboxes[i].right = 0;
				lpgw->keyboxes[i].bottom = INT_MAX;
				lpgw->keyboxes[i].top = 0;
			}
		}
		LPRECT bb = lpgw->keyboxes + plotno - 1;
		// @v11.0.1 SETMIN/SETMAX {
		SETMIN(bb->left, x);
		SETMAX(bb->right, x);
		SETMIN(bb->bottom, y);
		SETMAX(bb->top, y);
		// } @v11.0.1 SETMIN/SETMAX
	}
}

#ifdef USE_WINGDI
static void draw_grey_out_key_box(GW * lpgw, HDC hdc, int plotno)
{
	const RECT * p_bb = lpgw->keyboxes + plotno - 1;
	const int width  = p_bb->right - p_bb->left + 1;
	const int height = p_bb->top - p_bb->bottom + 1;
	HDC memdc = CreateCompatibleDC(hdc);
	HBITMAP membmp = CreateCompatibleBitmap(hdc, width, height);
	HBITMAP oldbmp = (HBITMAP)SelectObject(memdc, membmp);
	HBRUSH oldbrush = (HBRUSH)SelectObject(memdc, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
	PatBlt(memdc, 0, 0, width, height, PATCOPY);
	{
		BLENDFUNCTION ftn;
		ftn.AlphaFormat = 0; // no alpha channel in bitmap 
		ftn.SourceConstantAlpha = (UCHAR)(128); // global alpha 
		ftn.BlendOp = AC_SRC_OVER;
		ftn.BlendFlags = 0;
		AlphaBlend(hdc, p_bb->left, p_bb->bottom, width, height, memdc, 0, 0, width, height, ftn);
	}
	SelectObject(memdc, oldbrush);
	SelectObject(memdc, oldbmp);
	DeleteObject(membmp);
	DeleteDC(memdc);
}

static void draw_image(GW * lpgw, HDC hdc, char * image, POINT corners[4], uint width, uint height, int color_mode)
{
	if(image) {
		BITMAPINFO bmi;
		HRGN hrgn;
		INITWINBITMAPINFO(bmi);
		bmi.bmiHeader.biWidth = width;
		bmi.bmiHeader.biHeight = height;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biCompression = BI_RGB;
		// create clip region 
		hrgn = CreateRectRgn(MIN(corners[2].x, corners[3].x), MIN(corners[2].y, corners[3].y), MAX(corners[2].x, corners[3].x) + 1, MAX(corners[2].y, corners[3].y) + 1);
		SelectClipRgn(hdc, hrgn);
		if(color_mode != IC_RGBA) {
			char * dibimage;
			bmi.bmiHeader.biBitCount = 24;
			if(!lpgw->color) {
				// create a copy of the color image 
				int pad_bytes = (4 - (3 * width) % 4) % 4; /* scan lines start on ULONG boundaries */
				int image_size = (width * 3 + pad_bytes) * height;
				dibimage = (char *)SAlloc::M(image_size);
				memcpy(dibimage, image, image_size);
				for(uint y = 0; y < height; y++) {
					for(uint x = 0; x < width; x++) {
						BYTE * p = (BYTE*)dibimage + y * (3 * width + pad_bytes) + x * 3;
						// convert to gray 
						const uint luma = luma_from_color(p[2], p[1], p[0]);
						p[0] = p[1] = p[2] = luma;
					}
				}
			}
			else {
				dibimage = image;
			}
			StretchDIBits(hdc, MIN(corners[0].x, corners[1].x), MIN(corners[0].y, corners[1].y),
				abs(corners[1].x - corners[0].x), abs(corners[1].y - corners[0].y), 0, 0, width, height, dibimage, &bmi, DIB_RGB_COLORS, SRCCOPY);
			if(!lpgw->color) 
				SAlloc::F(dibimage);
		}
		else {
			HDC memdc;
			HBITMAP membmp, oldbmp;
			UINT32 * pvBits;
			BLENDFUNCTION ftn;
			bmi.bmiHeader.biBitCount = 32;
			memdc = CreateCompatibleDC(hdc);
			membmp = CreateDIBSection(memdc, &bmi, DIB_RGB_COLORS, (void**)&pvBits, NULL, 0x0);
			oldbmp = (HBITMAP)SelectObject(memdc, membmp);
			memcpy(pvBits, image, width * height * 4);
			// convert to grayscale?
			if(!lpgw->color) {
				for(uint y = 0; y < height; y++) {
					for(uint x = 0; x < width; x++) {
						UINT32 * p = pvBits + y * width + x;
						// convert to gray 
						uint luma = luma_from_color(GetRValue(*p), GetGValue(*p), GetBValue(*p));
						*p = (*p & 0xff000000) | RGB(luma, luma, luma);
					}
				}
			}
			ftn.BlendOp = AC_SRC_OVER;
			ftn.BlendFlags = 0;
			ftn.AlphaFormat = AC_SRC_ALPHA; /* bitmap has an alpha channel */
			ftn.SourceConstantAlpha = 0xff;
			AlphaBlend(hdc, MIN(corners[0].x, corners[1].x), MIN(corners[0].y, corners[1].y),
				abs(corners[1].x - corners[0].x), abs(corners[1].y - corners[0].y), memdc, 0, 0, width, height, ftn);
			SelectObject(memdc, oldbmp);
			DeleteObject(membmp);
			DeleteDC(memdc);
		}
		SelectClipRgn(hdc, NULL);
	}
}
//
// This one is really the heart of this module: it executes the stored set of
// commands, whenever it changed or a redraw is necessary 
//
static void drawgraph(GW * lpgw, HDC hdc, LPRECT rect)
{
	// draw ops 
	uint ngwop = 0;
	struct GWOP * curptr;
	struct GWOPBLK * blkptr;
	// layers and hypertext 
	uint plotno = 0;
	BOOL gridline = FALSE;
	BOOL skipplot = FALSE;
	BOOL keysample = FALSE;
	BOOL interactive;
	LPWSTR hypertext = NULL;
	int hypertype = 0;
	// colors 
	BOOL isColor; // use colors? 
	COLORREF last_color = 0; // currently selected color 
	double alpha_c = 1.0; // alpha for transparency 
	// lines 
	double line_width = lpgw->linewidth; // current line width 
	double lw_scale = 1.;
	LOGPEN cur_penstruct; // current pen settings 
	// polylines and polygons 
	int polymax = 200; // size of ppt 
	int polyi = 0; // number of points in ppt 
	POINT * ppt; // storage of polyline/polygon-points 
	// filled polygons and boxes 
	uint fillstyle = 0; /* current fill style */
	BOOL transparent = FALSE; /* transparent fill? */
	double alpha = 0.; /* alpha for transarency */
	int pattern = 0; /* patter number */
	COLORREF fill_color = 0; /* color to use for fills */
	HBRUSH solid_brush = 0; /* current solid fill brush */
	int shadedblendcaps = SB_CONST_ALPHA; /* displays can always do AlphaBlend */
	bool warn_no_transparent = FALSE;
	/* images */
	POINT corners[4];   // image corners 
	int color_mode = 0; // image color mode 
#ifdef EAM_BOXED_TEXT
	struct s_boxedtext {
		bool boxing;
		t_textbox_options option;
		POINT margin;
		POINT start;
		RECT box;
		int angle;
	} boxedtext;
#endif
	// point symbols 
	bool ps_caching = FALSE;
	enum win_pointtypes last_symbol = W_invalid_pointtype;
	HDC cb_memdc = NULL;
	HBITMAP cb_old_bmp;
	HBITMAP cb_membmp;
	POINT cb_ofs;
	// coordinates and lengths 
	int xdash, ydash; /* the transformed coordinates */
	int rr, rl, rt, rb; /* coordinates of drawing area */
	int htic, vtic;                         /* tic sizes */
	int hshift, vshift; /* correction of text position */
	/* indices */
	int seq = 0;                            /* sequence counter for W_image and W_boxedtext */
	int i;
	if(lpgw->locked)
		return;
	/* clear hypertexts only in display sessions */
	interactive = (GetObjectType(hdc) == OBJ_MEMDC) || ((GetObjectType(hdc) == OBJ_DC) && (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASDISPLAY));
	if(interactive)
		clear_tooltips(lpgw);
	/* The GDI status query functions don't work on metafile, printer or
	 * plotter handles, so can't know whether the screen is actually showing
	 * color or not, if drawgraph() is being called from CopyClip().
	 * Solve by defaulting isColor to TRUE in those cases.
	 * Note that info on color capabilities of printers would be available
	 * via DeviceCapabilities().
	 * Also note that querying the technology of a metafile dc does not work.
	 * Query the type instead.
	 */
	isColor = (((GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL)) > 2) || (GetObjectType(hdc) == OBJ_ENHMETADC) || 
		(GetObjectType(hdc) == OBJ_METADC) || (GetDeviceCaps(hdc, TECHNOLOGY) == DT_PLOTTER) || (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASPRINTER));

	if(isColor) {
		SetBkColor(hdc, lpgw->background);
		FillRect(hdc, rect, lpgw->hbrush);
	}
	else {
		FillRect(hdc, rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
	}
	// Need to scale line widths for raster printers so they are the same as on screen 
	if((GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASPRINTER)) {
		HDC hdc_screen = GetDC(NULL);
		lw_scale = (double)GetDeviceCaps(hdc, LOGPIXELSX) / (double)GetDeviceCaps(hdc_screen, LOGPIXELSY);
		line_width *= lw_scale;
		ReleaseDC(NULL, hdc_screen);
		/* Does the printer support AlphaBlend with transparency (const alpha)? */
		shadedblendcaps = GetDeviceCaps(hdc, SHADEBLENDCAPS);
		warn_no_transparent = ((shadedblendcaps & SB_CONST_ALPHA) == 0);
	}
	// only cache point symbols when drawing to a screen
	ps_caching = !((GetObjectType(hdc) == OBJ_ENHMETADC) || (GetObjectType(hdc) == OBJ_METADC) || (GetDeviceCaps(hdc, TECHNOLOGY) == DT_PLOTTER) || (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASPRINTER));
	ppt = (POINT*)LocalAllocPtr(LHND, (polymax+1) * sizeof(POINT));
	rr = rect->right;
	rl = rect->left;
	rt = rect->top;
	rb = rect->bottom;
	htic = MulDiv(static_cast<int>(lpgw->org_pointsize * lpgw->TicS.x), rr - rl, lpgw->MaxS.x) + 1;
	vtic = MulDiv(static_cast<int>(lpgw->org_pointsize * lpgw->TicS.y), rb - rt, lpgw->MaxS.y) + 1;
	// (re-)init GDI fonts 
	GraphChangeFont(lpgw, lpgw->deffontname, lpgw->deffontsize, hdc, *rect);
	lpgw->angle = 0;
	SetFont(lpgw, hdc);
	lpgw->justify = LEFT;
	SetTextAlign(hdc, TA_LEFT | TA_TOP);
	// calculate text shift for horizontal text 
	hshift = 0;
	vshift = -lpgw->tmHeight / 2; /* centered */
	// init layer variables 
	lpgw->numplots = 0;
	lpgw->hasgrid = FALSE;
	{
		for(uint keyboxidx = 0; keyboxidx < lpgw->maxkeyboxes; keyboxidx++) {
			lpgw->keyboxes[keyboxidx].left = INT_MAX;
			lpgw->keyboxes[keyboxidx].right = 0;
			lpgw->keyboxes[keyboxidx].bottom = INT_MAX;
			lpgw->keyboxes[keyboxidx].top = 0;
		}
	}
#ifdef EAM_BOXED_TEXT
	boxedtext.boxing = FALSE;
#endif
	SelectObject(hdc, lpgw->hapen); /* background brush */
	SelectObject(hdc, lpgw->colorbrush[2]); /* first user pen */
	// do the drawing 
	blkptr = lpgw->gwopblk_head;
	curptr = NULL;
	if(blkptr) {
		SETIFZ(blkptr->gwop, (struct GWOP *)GlobalLock(blkptr->hblk));
		if(!blkptr->gwop)
			return;
		curptr = (struct GWOP *)blkptr->gwop;
	}
	if(curptr == NULL)
		return;
	while(ngwop < lpgw->nGWOP) {
		// transform the coordinates 
		xdash = MulDiv(curptr->x, rr - rl - 1, lpgw->MaxS.x) + rl;
		ydash = rb - MulDiv(curptr->y, rb - rt - 1, lpgw->MaxS.y) + rt - 1;
		// handle layer commands first 
		if(curptr->op == W_layer) {
			t_termlayer layer = (t_termlayer)(curptr->x);
			switch(layer) {
				case TERM_LAYER_BEFORE_PLOT:
				    plotno++;
				    lpgw->numplots = plotno;
				    if(plotno >= lpgw->maxhideplots) {
					    lpgw->maxhideplots += 10;
					    lpgw->hideplot = (BOOL*)SAlloc::R(lpgw->hideplot, lpgw->maxhideplots * sizeof(BOOL));
					    for(uint idx = plotno; idx < lpgw->maxhideplots; idx++)
						    lpgw->hideplot[idx] = FALSE;
				    }
				    if(plotno <= lpgw->maxhideplots)
					    skipplot = lpgw->hideplot[plotno-1];
				    break;
				case TERM_LAYER_AFTER_PLOT:
				    skipplot = FALSE;
				    break;
#if 0
				case TERM_LAYER_BACKTEXT:
				case TERM_LAYER_FRONTTEXT:
				    break;
#endif
				case TERM_LAYER_BEGIN_GRID:
				    gridline = TRUE;
				    lpgw->hasgrid = TRUE;
				    break;
				case TERM_LAYER_END_GRID:
				    gridline = FALSE;
				    break;
				case TERM_LAYER_BEGIN_KEYSAMPLE:
				    keysample = TRUE;
				    break;
				case TERM_LAYER_END_KEYSAMPLE:
				    /* grey out keysample if graph is hidden */
				    if((plotno <= lpgw->maxhideplots) && lpgw->hideplot[plotno-1])
					    draw_grey_out_key_box(lpgw, hdc, plotno);
				    keysample = FALSE;
				    break;
				case TERM_LAYER_RESET:
				case TERM_LAYER_RESET_PLOTNO:
				    plotno = 0;
				    break;
				default:
				    break;
			};
		}
		// hide this layer? 
		if(!(skipplot || (gridline && lpgw->hidegrid)) || keysample || oneof4(curptr->op, W_line_type, W_setcolor, W_pointsize, W_line_width)) {
			// special case hypertexts 
			if(hypertext && hypertype == TERM_HYPERTEXT_TOOLTIP) {
				// point symbols 
				if((curptr->op >= W_dot) && (curptr->op <= W_dot + WIN_POINT_TYPES)) {
					RECT rect;
					rect.left = xdash - htic;
					rect.right = xdash + htic;
					rect.top = ydash - vtic;
					rect.bottom = ydash + vtic;
					add_tooltip(lpgw, &rect, hypertext);
					hypertext = NULL;
				}
			}
			switch(curptr->op) {
				case 0: break; // have run past last in this block 
				case W_layer: break; // already handled above 
				case W_polyline: 
					{
						POINTL * poly = (POINTL*)LocalLock(curptr->htext);
						polyi = curptr->x;
						if(polyi >= polymax) {
							const int step = 200;
							polymax  = (polyi + step) / step;
							polymax *= step;
							ppt = (POINT *)LocalReAllocPtr(ppt, LHND, (polymax + 1) * sizeof(POINT));
						}
						for(i = 0; i < polyi; i++) {
							// transform the coordinates 
							ppt[i].x = MulDiv(poly[i].x, rr - rl - 1, lpgw->MaxS.x) + rl;
							ppt[i].y = rb - MulDiv(poly[i].y, rb - rt - 1, lpgw->MaxS.y) + rt - 1;
						}
						LocalUnlock(poly);
						Polyline(hdc, ppt, polyi);
						if(keysample) {
							draw_update_keybox(lpgw, plotno, ppt[0].x, ppt[0].y);
							draw_update_keybox(lpgw, plotno, ppt[polyi-1].x, ppt[polyi-1].y);
						}
					}
					break;
				case W_line_type: 
					{
						int cur_pen = (int)curptr->x % WGNUMPENS;
						/* create new pens */
						if(cur_pen > LT_NODRAW) {
							cur_pen += 2;
							cur_penstruct =  (lpgw->color && isColor) ?  lpgw->colorpen[cur_pen] : lpgw->monopen[cur_pen];
							cur_penstruct.lopnStyle = lpgw->dashed ? lpgw->monopen[cur_pen].lopnStyle : lpgw->colorpen[cur_pen].lopnStyle;
						}
						else if(cur_pen == LT_NODRAW) {
							cur_pen = WGNUMPENS;
							cur_penstruct.lopnStyle = PS_NULL;
							cur_penstruct.lopnColor = 0;
							cur_penstruct.lopnWidth.x = 1;
						}
						else { /* <= LT_BACKGROUND */
							cur_pen = WGNUMPENS;
							cur_penstruct.lopnStyle = PS_SOLID;
							cur_penstruct.lopnColor = lpgw->background;
							cur_penstruct.lopnWidth.x = 1;
						}
						cur_penstruct.lopnWidth.x *= line_width;
						draw_new_pens(lpgw, hdc, cur_penstruct);
						// select new brush 
						solid_brush = (cur_pen < WGNUMPENS) ? lpgw->colorbrush[cur_pen] : lpgw->hbrush;
						SelectObject(hdc, solid_brush);
						// set text color, also used for pattern fill 
						SetTextColor(hdc, cur_penstruct.lopnColor);
						// remember this color 
						last_color = cur_penstruct.lopnColor;
						fill_color = last_color;
						alpha_c = 1.0;
					}
					break;
				case W_dash_type: 
					{
						int dt = curptr->x;
						if(dt >= 0) {
							dt %= WGNUMPENS;
							dt += 2;
							cur_penstruct.lopnStyle = lpgw->monopen[dt].lopnStyle;
							draw_new_pens(lpgw, hdc, cur_penstruct);
						}
						else if(dt == DASHTYPE_SOLID) {
							cur_penstruct.lopnStyle = PS_SOLID;
							draw_new_pens(lpgw, hdc, cur_penstruct);
						}
						else if(dt == DASHTYPE_AXIS) {
							dt = 1;
							cur_penstruct.lopnStyle = lpgw->dashed ? lpgw->monopen[dt].lopnStyle : lpgw->colorpen[dt].lopnStyle;
							draw_new_pens(lpgw, hdc, cur_penstruct);
						}
						else if(dt == DASHTYPE_CUSTOM) {
							; /* ignored */
						}
					}
					break;
				case W_text_encoding: lpgw->encoding = (enum set_encoding_id)curptr->x; break;
				case W_put_text: 
					{
						char * str = (char *)LocalLock(curptr->htext);
						if(str) {
							int dxl, dxr;
							int slen, vsize;
							// shift correctly for rotated text 
							draw_put_text(lpgw, hdc, xdash + hshift, ydash + vshift, str, FALSE);
	#ifndef EAM_BOXED_TEXT
							if(keysample) {
	#else
							if(keysample || boxedtext.boxing) {
	#endif
								slen  = GraphGetTextLength(lpgw, hdc, str, FALSE);
								vsize = MulDiv(lpgw->ChrS.y, rb - rt, 2 * lpgw->MaxS.y);
								if(lpgw->justify == LEFT) {
									dxl = 0;
									dxr = slen;
								}
								else if(lpgw->justify == CENTRE) {
									dxl = dxr = slen / 2;
								}
								else {
									dxl = slen;
									dxr = 0;
								}
							}
							if(keysample) {
								draw_update_keybox(lpgw, plotno, xdash - dxl, ydash - vsize);
								draw_update_keybox(lpgw, plotno, xdash + dxr, ydash + vsize);
							}
	#ifdef EAM_BOXED_TEXT
							if(boxedtext.boxing) {
								if(boxedtext.box.left > (xdash - boxedtext.start.x - dxl))
									boxedtext.box.left = xdash - boxedtext.start.x - dxl;
								if(boxedtext.box.right < (xdash - boxedtext.start.x + dxr))
									boxedtext.box.right = xdash - boxedtext.start.x + dxr;
								if(boxedtext.box.top > (ydash - boxedtext.start.y - vsize))
									boxedtext.box.top = ydash - boxedtext.start.y - vsize;
								if(boxedtext.box.bottom < (ydash - boxedtext.start.y + vsize))
									boxedtext.box.bottom = ydash - boxedtext.start.y + vsize;
								/* We have to remember the text angle as well. */
								boxedtext.angle = lpgw->angle;
							}
	#endif
						}
						LocalUnlock(curptr->htext);
					}
					break;
				case W_enhanced_text: {
				    char * str = (char *)LocalLock(curptr->htext);
				    if(str) {
					    RECT extend;
					    draw_enhanced_init(hdc);
					    draw_enhanced_text(lpgw, rect, xdash, ydash, str);
					    draw_get_enhanced_text_extend(&extend);
					    if(keysample) {
						    draw_update_keybox(lpgw, plotno, xdash - extend.left, ydash - extend.top);
						    draw_update_keybox(lpgw, plotno, xdash + extend.right, ydash + extend.bottom);
					    }
#ifdef EAM_BOXED_TEXT
					    if(boxedtext.boxing) {
						    if(boxedtext.box.left > (boxedtext.start.x - xdash - extend.left))
							    boxedtext.box.left = boxedtext.start.x - xdash - extend.left;
						    if(boxedtext.box.right < (boxedtext.start.x - xdash + extend.right))
							    boxedtext.box.right = boxedtext.start.x - xdash + extend.right;
						    if(boxedtext.box.top > (boxedtext.start.y - ydash - extend.top))
							    boxedtext.box.top = boxedtext.start.y - ydash - extend.top;
						    if(boxedtext.box.bottom < (boxedtext.start.y - ydash + extend.bottom))
							    boxedtext.box.bottom = boxedtext.start.y - ydash + extend.bottom;
						    /* We have to store the text angle as well. */
						    boxedtext.angle = lpgw->angle;
					    }
#endif
				    }
				    LocalUnlock(curptr->htext);
				    break;
			    }
				case W_hypertext:
				    if(interactive) {
					    /* Make a copy for future reference */
					    char * str = (char *)LocalLock(curptr->htext);
					    SAlloc::F(hypertext);
					    hypertext = UnicodeText(str, lpgw->encoding);
					    hypertype = curptr->x;
					    LocalUnlock(curptr->htext);
				    }
				    break;
#ifdef EAM_BOXED_TEXT
				case W_boxedtext:
				    if(seq == 0) {
					    boxedtext.option = curptr->x;
					    seq++;
					    break;
				    }
				    seq = 0;
				    switch(boxedtext.option) {
					    case TEXTBOX_INIT:
						/* initialise bounding box */
						boxedtext.box.left   = boxedtext.box.right = 0;
						boxedtext.box.bottom = boxedtext.box.top   = 0;
						boxedtext.start.x = xdash;
						boxedtext.start.y = ydash;
						/* Note: initialising the text angle here would be best IMHO,
						   but current core code does not set this until the actual
						   print-out is done. */
						boxedtext.angle = lpgw->angle;
						boxedtext.boxing = TRUE;
						break;
					    case TEXTBOX_OUTLINE:
					    case TEXTBOX_BACKGROUNDFILL: {
						/* draw rectangle */
						int dx = boxedtext.margin.x;
						int dy = boxedtext.margin.y;
#if 0
						printf("left %i right %i top %i bottom %i angle %i\n",
						    boxedtext.box.left - dx, boxedtext.box.right + dx,
						    boxedtext.box.top - dy, boxedtext.box.bottom + dy,
						    boxedtext.angle);
#endif
						if(((boxedtext.angle % 90) == 0) && (alpha_c == 1.)) {
							RECT rect;

							switch(boxedtext.angle) {
								case 0:
								    rect.left   = +boxedtext.box.left   - dx;
								    rect.right  = +boxedtext.box.right  + dx;
								    rect.top    = +boxedtext.box.top    - dy;
								    rect.bottom = +boxedtext.box.bottom + dy;
								    break;
								case 90:
								    rect.left   = +boxedtext.box.top    - dy;
								    rect.right  = +boxedtext.box.bottom + dy;
								    rect.top    = -boxedtext.box.right  - dx;
								    rect.bottom = -boxedtext.box.left   + dx;
								    break;
								case 180:
								    rect.left   = -boxedtext.box.right  - dx;
								    rect.right  = -boxedtext.box.left   + dx;
								    rect.top    = -boxedtext.box.bottom - dy;
								    rect.bottom = -boxedtext.box.top    + dy;
								    break;
								case 270:
								    rect.left   = -boxedtext.box.bottom - dy;
								    rect.right  = -boxedtext.box.top    + dy;
								    rect.top    = +boxedtext.box.left   - dx;
								    rect.bottom = +boxedtext.box.right  + dx;
								    break;
							}
							rect.left   += boxedtext.start.x;
							rect.right  += boxedtext.start.x;
							rect.top    += boxedtext.start.y;
							rect.bottom += boxedtext.start.y;
							if(boxedtext.option == TEXTBOX_OUTLINE)
								FrameRect(hdc, &rect, lpgw->hcolorbrush);
							else
								/* Fill bounding box with current color. */
								FillRect(hdc, &rect, lpgw->hcolorbrush);
						}
						else {
							double theta = boxedtext.angle * SMathConst::PiDiv180;
							double sin_theta = sin(theta);
							double cos_theta = cos(theta);
							POINT rect[5];
							rect[0].x =  (boxedtext.box.left   - dx) * cos_theta + (boxedtext.box.top    - dy) * sin_theta;
							rect[0].y = -(boxedtext.box.left   - dx) * sin_theta + (boxedtext.box.top    - dy) * cos_theta;
							rect[1].x =  (boxedtext.box.left   - dx) * cos_theta + (boxedtext.box.bottom + dy) * sin_theta;
							rect[1].y = -(boxedtext.box.left   - dx) * sin_theta + (boxedtext.box.bottom + dy) * cos_theta;
							rect[2].x =  (boxedtext.box.right  + dx) * cos_theta + (boxedtext.box.bottom + dy) * sin_theta;
							rect[2].y = -(boxedtext.box.right  + dx) * sin_theta + (boxedtext.box.bottom + dy) * cos_theta;
							rect[3].x =  (boxedtext.box.right  + dx) * cos_theta + (boxedtext.box.top    - dy) * sin_theta;
							rect[3].y = -(boxedtext.box.right  + dx) * sin_theta + (boxedtext.box.top    - dy) * cos_theta;
							for(i = 0; i < 4; i++) {
								rect[i].x += boxedtext.start.x;
								rect[i].y += boxedtext.start.y;
							}
							{
								HBRUSH save_brush;
								HPEN save_pen;
								if(boxedtext.option == TEXTBOX_OUTLINE) {
									save_brush = SelectObject(hdc, GetStockBrush(NULL_BRUSH));
									save_pen = SelectObject(hdc, lpgw->hapen);
								}
								else {
									/* Fill bounding box with current color. */
									save_brush = SelectObject(hdc, lpgw->hcolorbrush);
									save_pen = SelectObject(hdc, GetStockPen(NULL_PEN));
								}
								Polygon(hdc, rect, 4);
								SelectObject(hdc, save_brush);
								SelectObject(hdc, save_pen);
							}
						}
						boxedtext.boxing = FALSE;
						break;
					}
					    case TEXTBOX_MARGINS:
						/* Adjust size of whitespace around text: default is 1/2 char height + 2
						   char widths. */
						boxedtext.margin.x = MulDiv(curptr->x * lpgw->hchar, rr - rl, 1000 * lpgw->MaxS.x);
						boxedtext.margin.y = MulDiv(curptr->y * lpgw->hchar, rr - rl, 1000 * lpgw->MaxS.x);
						break;
					    default:
						break;
				    }
				    break;
#endif
				case W_fillstyle:
				    // HBB 20010916: new entry, needed to squeeze the many
				    // parameters of a filled box call through the bottleneck
				    // of the fixed number of parameters in GraphOp() and struct GWOP, respectively. 
				    fillstyle = curptr->x;
				    transparent = FALSE;
				    alpha = 0.;
				    // FIXME: This shouldn't be necessary...
				    polyi = 0;
				    switch(fillstyle & 0x0f) {
					    case FS_TRANSPARENT_SOLID:
							alpha = (fillstyle >> 4) / 100.;
							if((shadedblendcaps & SB_CONST_ALPHA) != 0) {
								transparent = TRUE;
								/* we already have a brush with that color */
							}
							else {
								/* Printer does not support AlphaBlend() */
								COLORREF color =
									RGB(255 - alpha * (255 - GetRValue(last_color)),
									255 - alpha * (255 - GetGValue(last_color)),
									255 - alpha * (255 - GetBValue(last_color)));
								solid_brush = lpgw->hcolorbrush;
								fill_color = color;
								draw_new_brush(lpgw, hdc, fill_color);
								fillstyle = (fillstyle & 0xfffffff0) | FS_SOLID;
								if(warn_no_transparent) {
									fprintf(stderr, "Warning: Transparency not supported on this device.\n");
									warn_no_transparent = FALSE; /* Warn only once */
								}
							}
							break;
					    case FS_SOLID: {
							if(alpha_c < 1.0) {
								alpha = alpha_c;
								fill_color = last_color;
							}
							else if((int)(fillstyle >> 4) == 100) {
								// special case this common choice 
								// FIXME: we should already have that!
								fill_color = last_color;
							}
							else {
								const int _fs = static_cast<int>(fillstyle >> 4);
								double density = MINMAX(_fs, 0, 100) * 0.01;
								COLORREF color = RGB(255 - density * (255 - GetRValue(last_color)),
									255 - density * (255 - GetGValue(last_color)), 255 - density * (255 - GetBValue(last_color)));
								fill_color = color;
							}
							draw_new_brush(lpgw, hdc, fill_color);
							solid_brush = lpgw->hcolorbrush;
							break;
					}
					    case FS_TRANSPARENT_PATTERN:
							if((shadedblendcaps & SB_CONST_ALPHA) != 0) {
								transparent = TRUE;
								alpha = 1.;
							}
							else {
								/* Printers do not support AlphaBlend() */
								fillstyle = (fillstyle & 0xfffffff0) | FS_PATTERN;
								if(warn_no_transparent) {
									fprintf(stderr, "Warning: Transparency not supported on this device.\n");
									warn_no_transparent = FALSE; /* Warn only once */
								}
							}
							/* intentionally fall through */
					    case FS_PATTERN:
							// style == 2 --> use fill pattern according to fillpattern. Pattern number is enumerated 
							pattern = MAX(fillstyle >> 4, 0) % pattern_num;
							SelectObject(hdc, pattern_brush[pattern]);
							break;
					    case FS_EMPTY:
							// FIXME: Instead of filling with background color, we should not fill at all in this case! 
							// fill with background color 
							SelectObject(hdc, lpgw->hbrush);
							fill_color = lpgw->background;
							solid_brush = lpgw->hbrush;
							break;
					    case FS_DEFAULT:
					    default:
							break; // Leave the current brush and color in place 
				    }
				    break;
				case W_move:
				    ppt[0].x = xdash;
				    ppt[0].y = ydash;
				    break;
				case W_boxfill: { /* ULIG */
				    /* NOTE: the x and y passed with this call are the coordinates of the
				     * lower right corner of the box. The upper left corner was stored into
				     * ppt[0] by a preceding W_move, and the style was set
				     * by a W_fillstyle call. */
				    POINT p;
				    UINT width = abs(xdash - ppt[0].x);
				    UINT height = abs(ppt[0].y - ydash);
				    p.x = MIN(ppt[0].x, xdash);
				    p.y = MIN(ydash, ppt[0].y);
				    if(transparent) {
					    BLENDFUNCTION ftn;
					    HBRUSH old_brush;
					    /* create memory device context for bitmap */
					    HDC memdc = CreateCompatibleDC(hdc);
					    /* create standard bitmap, no alpha channel needed */
					    HBITMAP membmp = CreateCompatibleBitmap(hdc, width, height);
					    HBITMAP oldbmp = (HBITMAP)SelectObject(memdc, membmp);
					    /* prepare memory context */
					    SetTextColor(memdc, fill_color);
					    if((fillstyle & 0x0f) == FS_TRANSPARENT_PATTERN)
						    old_brush = (HBRUSH)SelectObject(memdc, pattern_brush[pattern]);
					    else
						    old_brush = (HBRUSH)SelectObject(memdc, solid_brush);
					    /* draw into memory bitmap */
					    PatBlt(memdc, 0, 0, width, height, PATCOPY);
					    /* copy bitmap back */
					    if((fillstyle & 0x0f) == FS_TRANSPARENT_PATTERN) {
						    TransparentBlt(hdc, p.x, p.y, width, height, memdc, 0, 0, width, height, 0x00ffffff);
					    }
					    else {
						    ftn.AlphaFormat = 0; /* no alpha channel in bitmap */
						    ftn.SourceConstantAlpha = (UCHAR)(alpha * 0xff); /* global alpha */
						    ftn.BlendOp = AC_SRC_OVER;
						    ftn.BlendFlags = 0;
						    AlphaBlend(hdc, p.x, p.y, width, height, memdc, 0, 0, width, height, ftn);
					    }
					    /* clean up */
					    SelectObject(memdc, old_brush);
					    SelectObject(memdc, oldbmp);
					    DeleteObject(membmp);
					    DeleteDC(memdc);
				    }
				    else {
					    /* not transparent */
					    /* FIXME: this actually is transparent, but probably shouldn't be */
					    PatBlt(hdc, p.x, p.y, width, height, PATCOPY);
					    /*
					            SelectObject(hdc, lpgw->hnull);
					            Rectangle(hdc, p.x, p.y, p.x + width + 1, p.y + height + 1);
					            SelectObject(hdc, lpgw->hapen);
					     */
				    }
				    polyi = 0;
				    if(keysample)
					    draw_update_keybox(lpgw, plotno, xdash + 1, ydash);
				    break;
			    }
				case W_text_angle:
				    if(lpgw->angle != (int)curptr->x) {
					    lpgw->angle = (int)curptr->x;
					    SetFont(lpgw, hdc);
					    // recalculate shifting of rotated text 
					    hshift = static_cast<int>(-sin(SMathConst::PiDiv180 * lpgw->angle) * lpgw->tmHeight / 2.0);
					    vshift = static_cast<int>(-cos(SMathConst::PiDiv180 * lpgw->angle) * lpgw->tmHeight / 2.0);
				    }
				    break;
				case W_justify:
				    draw_text_justify(hdc, curptr->x);
				    lpgw->justify = curptr->x;
				    break;
				case W_font: {
				    int size = curptr->x;
				    char * font = (char *)LocalLock(curptr->htext);
				    // GraphChangeFont already handles font==NULL and size==0,
				    // so the checks below are a bit paranoid...
#ifdef UNICODE
				    TCHAR tfont[MAXFONTNAME];
				    if(font)
					    MultiByteToWideChar(CP_ACP, 0, font, -1, tfont, MAXFONTNAME);
#else
				    LPTSTR tfont = font;
#endif
				    GraphChangeFont(lpgw, (font ? tfont : lpgw->deffontname), (size > 0 ? size : lpgw->deffontsize), hdc, *rect);
				    LocalUnlock(curptr->htext);
				    SetFont(lpgw, hdc);
				    // recalculate shifting of rotated text */
				    hshift = static_cast<int>(-sin(SMathConst::PiDiv180 * lpgw->angle) * lpgw->tmHeight / 2.0);
				    vshift = static_cast<int>(-cos(SMathConst::PiDiv180 * lpgw->angle) * lpgw->tmHeight / 2.0);
				    break;
			    }
				case W_pointsize:
				    if(curptr->x > 0) {
					    double pointsize = curptr->x / 100.0;
					    htic = MulDiv(static_cast<int>(pointsize * lpgw->pointscale * lpgw->TicS.x), rr - rl, lpgw->MaxS.x) + 1;
					    vtic = MulDiv(static_cast<int>(pointsize * lpgw->pointscale * lpgw->TicS.y), rb - rt, lpgw->MaxS.y) + 1;
				    }
				    else {
					    htic = vtic = 0;
				    }
				    // invalidate point symbol cache 
				    last_symbol = W_invalid_pointtype;
				    break;
				case W_line_width:
				    /* HBB 20000813: this may look strange, but it ensures
				     * that linewidth is exactly 1 iff it's in default
				     * state */
				    line_width = curptr->x == 100 ? 1 : (curptr->x / 100.0);
				    line_width *= lpgw->linewidth * lw_scale;
				    // invalidate point symbol cache 
				    last_symbol = W_invalid_pointtype;
				    break;

				case W_setcolor: {
				    COLORREF color;
				    if(curptr->htext) { /* TC_LT */
					    int pen = (int)curptr->x % WGNUMPENS;
					    color = (pen <= LT_NODRAW) ? lpgw->background : (lpgw->color ? lpgw->colorpen[pen+2].lopnColor : lpgw->monopen[pen+2].lopnColor);
					    alpha_c = 1.0;
				    }
				    else {                                      /* TC_RGB */
					    rgb255_color rgb255;
					    rgb255.r = (curptr->y & 0xff);
					    rgb255.g = (curptr->x >> 8);
					    rgb255.b = (curptr->x & 0xff);
					    alpha_c = 1. - ((curptr->y >> 8) & 0xff) / 255.;
					    if(lpgw->color || ((rgb255.r == rgb255.g) && (rgb255.r == rgb255.b))) {
						    /* Use colors or this is already gray scale */
						    color = RGB(rgb255.r, rgb255.g, rgb255.b);
					    }
					    else {
						    /* convert to gray */
						    uint luma = luma_from_color(rgb255.r, rgb255.g, rgb255.b);
						    color = RGB(luma, luma, luma);
					    }
				    }
				    /* solid fill brush */
				    draw_new_brush(lpgw, hdc, color);
				    solid_brush = lpgw->hcolorbrush;
				    /* create new pen, too */
				    cur_penstruct.lopnColor = color;
				    draw_new_pens(lpgw, hdc, cur_penstruct);
				    /* set text color, which is also used for pattern fill */
				    SetTextColor(hdc, color);
				    /* invalidate point symbol cache */
				    if(last_color != color)
					    last_symbol = W_invalid_pointtype;
				    /* remember this color */
				    last_color = color;
				    fill_color = color;
				    break;
			    }
				case W_filled_polygon_pt: {
				    /* a point of the polygon is coming */
				    if(polyi >= polymax) {
					    polymax += 200;
					    ppt = (POINT*)LocalReAllocPtr(ppt, LHND, (polymax+1) * sizeof(POINT));
				    }
				    ppt[polyi].x = xdash;
				    ppt[polyi].y = ydash;
				    polyi++;
				    break;
			    }
				case W_filled_polygon_draw: {
				    // end of point series --> draw polygon now 
				    if(!transparent) {
					    // fill area without border 
					    SelectObject(hdc, lpgw->hnull);
					    Polygon(hdc, ppt, polyi);
					    SelectObject(hdc, lpgw->hapen); /* restore previous pen */
				    }
				    else {
					    // BM: To support transparent fill on Windows we draw the
					    // polygon into a memory bitmap using a memory device context.
					    // 
					    // We then associate an alpha value to the bitmap and use
					    // AlphaBlend() to copy the bitmap back.
					    // 
					    // Note: we could probably simplify and speed up the case of
					    // pattern fill by using TransparentBlt() instead.
					    // 
					    HDC memdc;
					    HBITMAP membmp, oldbmp;
					    long minx;
						long miny;
						long maxx;
						long maxy;
					    UINT32 width, height;
					    BITMAPINFO bmi;
					    UINT32 * pvBits;
					    //int x, y;
						int i;
					    POINT * points;
					    BLENDFUNCTION ftn;
					    UINT32 uAlpha = (UCHAR)(0xff * alpha);
					    // make sure the indicator for transparency is different fill_color 
					    UINT32 transparentColor = fill_color ^ 0x00ffffff;
					    COLORREF bkColor = RGB((transparentColor >> 16) & 0xff, (transparentColor >> 8) & 0xff, transparentColor & 0xff);
					    HBRUSH old_brush;
					    HPEN old_pen;
					    // find minimum rectangle enclosing our polygon. 
					    minx = maxx = ppt[0].x;
					    miny = maxy = ppt[0].y;
					    for(i = 1; i < polyi; i++) {
						    minx = smin(ppt[i].x, minx);
						    miny = smin(ppt[i].y, miny);
						    maxx = smax(ppt[i].x, maxx);
						    maxy = smax(ppt[i].y, maxy);
					    }
					    // now shift polygon points to upper left corner 
					    points = (POINT*)LocalAllocPtr(LHND, (polyi+1) * sizeof(POINT));
					    for(i = 0; i < polyi; i++) {
						    points[i].x = ppt[i].x - minx;
						    points[i].y = ppt[i].y - miny;
					    }
					    // create memory device context for bitmap 
					    memdc = CreateCompatibleDC(hdc);
					    // create memory bitmap with alpha channel and minimal size 
					    width  = maxx - minx;
					    height = maxy - miny;
						INITWINBITMAPINFO(bmi);
					    bmi.bmiHeader.biWidth = width;
					    bmi.bmiHeader.biHeight = height;
					    bmi.bmiHeader.biPlanes = 1;
					    bmi.bmiHeader.biBitCount = 32;
					    bmi.bmiHeader.biCompression = BI_RGB;
					    bmi.bmiHeader.biSizeImage = width * height * 4;
					    membmp = CreateDIBSection(memdc, &bmi, DIB_RGB_COLORS, (void**)&pvBits, NULL, 0x0);
					    oldbmp = (HBITMAP)SelectObject(memdc, membmp);
						{
							// clear bitmap, could also do it via GDI 
							for(uint ptidx = 0; ptidx < width * height; ptidx++)
								pvBits[ptidx] = transparentColor;
						}
					    // prepare the memory context 
					    SetTextColor(memdc, fill_color);
					    SetBkColor(memdc, bkColor);
					    old_pen = (HPEN)SelectObject(memdc, lpgw->hnull);
					    old_brush = (HBRUSH)SelectObject(memdc, ((fillstyle & 0x0f) == FS_TRANSPARENT_PATTERN) ? pattern_brush[pattern] : solid_brush);
					    // finally, draw polygon 
					    Polygon(memdc, points, polyi);
						{
							// add alpha channel to bitmap */
							// Note: this is really a pre-scaled alpha channel, see MSDN. To make life easy we only use global transparency, see below 
							for(uint yidx = 0; yidx < height; yidx++) {
								for(uint xidx = 0; xidx < width; xidx++) {
									UINT32 * p_pixel = &pvBits[xidx + yidx * width];
									//UINT32 pixel = pvBits[xidx + yidx * width];
									if(*p_pixel == transparentColor)
										*p_pixel  = 0x00000000; // completely transparent 
									else
										*p_pixel |= 0xff000000; // mark as completely opaque 
								}
							}
						}
					    // copy to device with alpha blending 
					    ftn.BlendOp = AC_SRC_OVER;
					    ftn.BlendFlags = 0;
					    ftn.AlphaFormat = AC_SRC_ALPHA; /* bitmap has an alpha channel */
					    ftn.SourceConstantAlpha = uAlpha;
					    AlphaBlend(hdc, minx, miny, width, height, memdc, 0, 0, width, height, ftn);
					    // clean up 
					    LocalFreePtr(points);
					    SelectObject(memdc, old_pen);
					    SelectObject(memdc, old_brush);
					    SelectObject(memdc, oldbmp);
					    DeleteObject(membmp);
					    DeleteDC(memdc);
				    }
				    polyi = 0;
			    }
			    break;

				case W_image: {
				    /* Due to the structure of gwop 6 entries are needed in total. */
				    if(seq == 0) {
					    /* First OP contains only the color mode */
					    color_mode = curptr->x;
				    }
				    else if(seq < 5) {
					    /* Next four OPs contain the `corner` array */
					    corners[seq-1].x = xdash;
					    corners[seq-1].y = ydash;
				    }
				    else {
					    /* The last OP contains the image and it's size */
					    char * image = (char *)LocalLock(curptr->htext);
					    uint width = curptr->x;
					    uint height = curptr->y;
					    draw_image(lpgw, hdc, image, corners, width, height, color_mode);
					    LocalUnlock(curptr->htext);
				    }
				    seq = (seq + 1) % 6;
			    }
			    break;
				default: {
				    int xofs, yofs;
				    HDC dc;
				    HBRUSH old_brush;
				    HPEN old_pen;
				    enum win_pointtypes symbol = (enum win_pointtypes)curptr->op;
				    // This covers only point symbols. All other codes should be handled in the switch statement. 
				    if((symbol < W_dot) || (symbol > W_last_pointtype))
					    break;
				    // draw cached point symbol 
				    if(ps_caching && (last_symbol == symbol) && cb_memdc) {
					    TransparentBlt(hdc, xdash - cb_ofs.x, ydash - cb_ofs.y, 2*htic+2, 2*vtic+2, cb_memdc, 0, 0, 2*htic+2, 2*vtic+2, 0x00ffffff);
					    break;
				    }
				    else if(cb_memdc) {
						SelectObject(cb_memdc, cb_old_bmp);
						DeleteObject(cb_membmp);
						DeleteDC(cb_memdc);
						cb_memdc = NULL;
					}
				    // caching of point symbols? 
				    if(ps_caching) {
					    RECT rect;
					    // create memory device context for bitmap 
					    dc = cb_memdc = CreateCompatibleDC(hdc);
					    // create standard bitmap, no alpha channel 
					    cb_membmp = CreateCompatibleBitmap(hdc, 2*htic+2, 2*vtic+2);
					    cb_old_bmp = (HBITMAP)SelectObject(dc, cb_membmp);
					    // prepare memory context 
					    SetTextColor(dc, fill_color);
					    old_brush = (HBRUSH)SelectObject(dc, solid_brush);
					    old_pen = (HPEN)SelectObject(dc, lpgw->hapen);
					    rect.left = rect.bottom = 0;
					    rect.top = 2*vtic+2;
					    rect.right = 2*htic+2;
					    FillRect(dc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
					    cb_ofs.x = xofs = htic+1;
					    cb_ofs.y = yofs = vtic+1;
					    last_symbol = symbol;
				    }
				    else {
					    dc = hdc;
					    xofs = xdash;
					    yofs = ydash;
				    }
				    switch(symbol) {
					    case W_dot: dot(dc, xofs, yofs); break;
					    case W_plus: /* do plus */
					    case W_star: /* do star: first plus, then cross */
							SelectObject(dc, lpgw->hsolid);
							MoveTo(dc, xofs - htic, yofs);
							LineTo(dc, xofs + htic, yofs);
							MoveTo(dc, xofs, yofs - vtic);
							LineTo(dc, xofs, yofs + vtic);
							SelectObject(dc, lpgw->hapen);
							if(symbol == W_plus)
								break;
					    case W_cross: /* do X */
							SelectObject(dc, lpgw->hsolid);
							MoveTo(dc, xofs - htic, yofs - vtic);
							LineTo(dc, xofs + htic, yofs + vtic);
							MoveTo(dc, xofs - htic, yofs + vtic);
							LineTo(dc, xofs + htic, yofs - vtic);
							SelectObject(dc, lpgw->hapen);
							break;
					    case W_circle: /* do open circle */
							SelectObject(dc, lpgw->hsolid);
							Arc(dc, xofs - htic, yofs - vtic, xofs + htic + 1, yofs + vtic + 1, xofs, yofs + vtic + 1, xofs, yofs + vtic + 1);
							dot(dc, xofs, yofs);
							SelectObject(dc, lpgw->hapen);
							break;
					    case W_fcircle: /* do filled circle */
							SelectObject(dc, lpgw->hsolid);
							Ellipse(dc, xofs-htic, yofs-vtic, xofs+htic+1, yofs+vtic+1);
							SelectObject(dc, lpgw->hapen);
							break;
					    default: // potentially closed figure 
							{ 
								POINT p[6];
								int i;
								int shape = 0;
								int filled = 0;
								int index = 0;
								static const float pointshapes[6][10] = {
									{-1, -1, +1, -1, +1, +1, -1, +1, 0, 0}, /* box */
									{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* dummy, circle */
									{ 0, -4.0f/3, -4.0f/3, 2.0f/3, 4.f/3,  2.0f/3, 0, 0},/* triangle */
									{ 0, 4.0f/3, -4.0f/3, -2.0f/3, 4.0f/3,  -2.0f/3, 0, 0},/* inverted triangle */
									{ 0, +1, -1,  0,  0, -1, +1,  0, 0, 0},/* diamond */
									{ 0, 1, 0.95106f, 0.30902f, 0.58779f, -0.80902f, -0.58779f, -0.80902f, -0.95106f, 0.30902f} /* pentagon */
								};
								// This should never happen since all other codes should be
								// handled in the switch statement. 
								if((symbol < W_box) || (symbol > W_last_pointtype))
									break;
								// Calculate index, instead of an ugly long switch statement;
								// Depends on definition of commands in wgnuplib.h.
								index = symbol - W_box;
								shape = index / 2;
								filled = (index % 2) > 0;
								for(i = 0; i < 5; ++i) {
									if(pointshapes[shape][i*2+1] == 0 && pointshapes[shape][i*2] == 0)
										break;
									p[i].x = static_cast<LONG>(xofs + htic * pointshapes[shape][i*2] + 0.5);
									p[i].y = static_cast<LONG>(yofs + vtic * pointshapes[shape][i*2+1] + 0.5);
								}
								if(filled) {
									// Filled polygon 
									SelectObject(dc, lpgw->hsolid);
									Polygon(dc, p, i);
									SelectObject(dc, lpgw->hapen);
								}
								else {
									// Outline polygon 
									//p[i].x = p[0].x;
									//p[i].y = p[0].y;
									p[i] = p[0];
									SelectObject(dc, lpgw->hsolid);
									Polyline(dc, p, i + 1);
									SelectObject(dc, lpgw->hapen);
									dot(dc, xofs, yofs);
								}
							}
				    }
				    if(ps_caching) {
					    // copy memory bitmap to screen 
					    TransparentBlt(hdc, xdash - xofs, ydash - yofs, 2*htic+2, 2*vtic+2, dc, 0, 0, 2*htic+2, 2*vtic+2, 0x00ffffff);
					    // partial clean up 
					    SelectObject(dc, old_brush);
					    SelectObject(dc, old_pen);
				    }
				    if(keysample) {
					    draw_update_keybox(lpgw, plotno, xdash + htic, ydash + vtic);
					    draw_update_keybox(lpgw, plotno, xdash - htic, ydash - vtic);
				    }
			    } /* default case */
			} /* switch(opcode) */
		} /* hide layer? */

		ngwop++;
		curptr++;
		if((uint)(curptr - blkptr->gwop) >= GWOPMAX) {
			GlobalUnlock(blkptr->hblk);
			blkptr->gwop = (struct GWOP *)NULL;
			if((blkptr = blkptr->next) == NULL)
				/* If exact multiple of GWOPMAX entries are queued,
				 * next will be NULL. Only the next GraphOp() call would
				 * have allocated a new block */
				return;
			SETIFZ(blkptr->gwop, (struct GWOP *)GlobalLock(blkptr->hblk));
			if(!blkptr->gwop)
				return;
			curptr = (struct GWOP *)blkptr->gwop;
		}
	} /* while (ngwop < lpgw->nGWOP) */
	/* cleanup */
	if(ps_caching && cb_memdc) {
		SelectObject(cb_memdc, cb_old_bmp);
		DeleteObject(cb_membmp);
		DeleteDC(cb_memdc);
		cb_memdc = NULL;
	}
	if(lpgw->hcolorbrush) {
		SelectObject(hdc, GetStockObject(BLACK_BRUSH));
		DeleteObject(lpgw->hcolorbrush);
		lpgw->hcolorbrush = NULL;
	}
	LocalFreePtr(ppt);
}

#endif // USE_WINGDI

/* ================================== */

/* save graph windows as enhanced metafile
 * The code in here is very similar to what CopyClip does...
 */
static void SaveAsEMF(GW * lpgw)
{
	static OPENFILENAME Ofn;
	static TCHAR lpstrCustomFilter[256] = { '\0' };
	static TCHAR lpstrFileName[MAX_PATH] = { '\0' };
	static TCHAR lpstrFileTitle[MAX_PATH] = { '\0' };
	Ofn.lStructSize = sizeof(OPENFILENAME);
	Ofn.hwndOwner = lpgw->hWndGraph;
	Ofn.lpstrInitialDir = NULL;
#if defined(HAVE_GDIPLUS) && defined(USE_WINGDI)
	Ofn.lpstrFilter = TEXT("Enhanced Metafile (*.emf)\0*.emf\0Enhanced Metafile+ (*.emf)\0*.emf\0");
#elif defined (USE_WINGDI)
	Ofn.lpstrFilter = TEXT("Enhanced Metafile (*.emf)\0*.emf\0");
#elif defined(HAVE_GDIPLUS)
	Ofn.lpstrFilter = TEXT("Enhanced Metafile+ (*.emf)\0*.emf\0");
#endif
	Ofn.lpstrCustomFilter = lpstrCustomFilter;
	Ofn.nMaxCustFilter = 255;
#if defined(HAVE_GDIPLUS) && defined(USE_WINGDI)
	/* Direct2D cannot do EMF. Fall back to GDI+ instead. */
	Ofn.nFilterIndex = (lpgw->gdiplus || lpgw->d2d ? 2 : 1);
#else
	Ofn.nFilterIndex = 1;
#endif
	Ofn.lpstrFile = lpstrFileName;
	Ofn.nMaxFile = MAX_PATH;
	Ofn.lpstrFileTitle = lpstrFileTitle;
	Ofn.nMaxFileTitle = MAX_PATH;
	Ofn.lpstrInitialDir = NULL;
	Ofn.lpstrTitle = NULL;
	Ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN | OFN_NOCHANGEDIR;
	Ofn.lpstrDefExt = TEXT("emf");
	if(GetSaveFileName(&Ofn) != 0) {
		HWND hwnd = lpgw->hGraph;
		RECT rect;
		// get the context 
		HDC hdc = GetDC(hwnd);
		GetPlotRect(lpgw, &rect);
		switch(Ofn.nFilterIndex) {
#ifdef USE_WINGDI
			case 1: { /* GDI Enhanced Metafile (EMF) */
			    HENHMETAFILE hemf;
			    HDC hmf;
			    RECT mfrect;
			    GetPlotRectInMM(lpgw, &mfrect, hdc);
			    hmf = CreateEnhMetaFile(hdc, Ofn.lpstrFile, &mfrect, NULL);
			    drawgraph(lpgw, hmf, &rect);
			    hemf = CloseEnhMetaFile(hmf);
			    DeleteEnhMetaFile(hemf);
			    break;
		    }
#else
			case 1:
#endif
#ifdef HAVE_GDIPLUS
			case 2: {/* GDI+ Enhanced Metafile (EMF+) */
#ifndef UNICODE
			    LPWSTR wfile = UnicodeText(Ofn.lpstrFile, S_ENC_DEFAULT);
			    metafile_gdiplus(lpgw, hdc, &rect, wfile);
			    SAlloc::F(wfile);
#else
			    metafile_gdiplus(lpgw, hdc, &rect, Ofn.lpstrFile);
#endif
			    break;
		    }
#endif
			default:
			    MessageBox(lpgw->hWndGraph, TEXT("Unable to save EMF data."), TEXT("gnuplot: save as EMF"), MB_OK | MB_ICONERROR);
			    break;
		}
		ReleaseDC(hwnd, hdc);
	}
}

HBITMAP GraphGetBitmap(GW * lpgw)
{
	RECT rect;
	HDC mem;
	HBITMAP bitmap;
	HDC hdc = GetDC(lpgw->hGraph);
	GetPlotRect(lpgw, &rect);
	// make a bitmap and copy it there 
	mem = CreateCompatibleDC(hdc);
	bitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
	if(bitmap) {
		// there is enough memory and the bitmap is available 
		HBITMAP oldbmp = (HBITMAP)SelectObject(mem, bitmap);
		// copy from screen 
		BitBlt(mem, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdc, rect.left, rect.top, SRCCOPY);
		SelectObject(mem, oldbmp);
	}
	DeleteDC(mem);
	ReleaseDC(lpgw->hGraph, hdc);
	return bitmap;
}
// 
// copy graph window to clipboard --- note that the Metafile is drawn at the full
// virtual resolution of the Windows terminal driver (24000 x 18000 pixels), to
// preserve as much accuracy as remotely possible 
// 
static void CopyClip(GW * lpgw)
{
	RECT rect;
	HBITMAP bitmap;
	HENHMETAFILE hemf = 0;
	HDC hdc;
	/* view the window */
	HWND hwnd = lpgw->hWndGraph;
	if(IsIconic(hwnd))
		ShowWindow(hwnd, SW_SHOWNORMAL);
	BringWindowToTop(hwnd);
	UpdateWindow(hwnd);
	// get a bitmap copy of the window 
	bitmap = GraphGetBitmap(lpgw);
	if(!bitmap) {
		MessageBeep(MB_ICONHAND);
		MessageBox(lpgw->hWndGraph, TEXT("Insufficient memory to copy to clipboard"), lpgw->Title, MB_ICONHAND | MB_OK);
	}
	// get the context 
	hwnd = lpgw->hGraph;
	hdc = GetDC(hwnd);
	GetPlotRect(lpgw, &rect);
	/* OK, bitmap done, now create an enhanced Metafile context
	 * and redraw the whole plot into that.
	 */
#ifdef HAVE_GDIPLUS
	if(lpgw->gdiplus) {
		hemf = clipboard_gdiplus(lpgw, hdc, &rect);
	}
	else
#endif
#if defined(HAVE_GDIPLUS) && defined(HAVE_D2D)
	/* Direct2D cannot do EMF. Fall back to GDI+. */
	if(lpgw->d2d) {
		hemf = clipboard_gdiplus(lpgw, hdc, &rect);
	}
	else
#endif
	{
#ifdef USE_WINGDI
		HDC hmf;
		RECT mfrect;
		// make copy of window's main status struct for modification 
		GW gwclip = *lpgw;
		gwclip.hfonth = gwclip.hfontv = 0;
		MakePens(&gwclip, hdc);
		MakeFonts(&gwclip, &rect, hdc);
		GetPlotRectInMM(lpgw, &mfrect, hdc);
		hmf = CreateEnhMetaFile(hdc, NULL, &mfrect, NULL);
		drawgraph(&gwclip, hmf, &rect);
		hemf = CloseEnhMetaFile(hmf);
		DestroyFonts(&gwclip);
		DestroyPens(&gwclip);
#endif
	}
	ReleaseDC(hwnd, hdc);
	// Now we have the Metafile and Bitmap prepared, post their contents to the Clipboard 
	OpenClipboard(lpgw->hWndGraph);
	EmptyClipboard();
	// Note that handles are owned by the system after calls to SetClipboardData()
	if(hemf)
		SetClipboardData(CF_ENHMETAFILE, hemf);
	else
		fprintf(stderr, "Error: no metafile data available.\n");
	if(bitmap)
		SetClipboardData(CF_BITMAP, bitmap);
	else
		fprintf(stderr, "Error: no bitmap data available.\n");
	CloseClipboard();
	DeleteEnhMetaFile(hemf);
}
//
// copy graph window to printer
//
static void CopyPrint(GW * lpgw)
{
	DOCINFO docInfo;
	HDC printer = NULL;
	HANDLE printerHandle;
	PRINTDLGEX pd;
	DEVNAMES * pDevNames;
	DEVMODE * pDevMode;
	LPCTSTR szDriver, szDevice, szOutput;
	HWND hwnd = lpgw->hWndGraph;
	RECT rect;
	GP_PRINT pr;
	PROPSHEETPAGE psp;
	HPROPSHEETPAGE hpsp;
	HDC hdc;
	uint dpiX, dpiY;
	// Print Property Sheet Dialog 
	memzero(&pr, sizeof(pr));
	GetPlotRect(lpgw, &rect);
	hdc = GetDC(hwnd);
	pr.pdef.x = MulDiv(rect.right - rect.left, 254, 10 * GetDeviceCaps(hdc, LOGPIXELSX));
	pr.pdef.y = MulDiv(rect.bottom  - rect.top, 254, 10 * GetDeviceCaps(hdc, LOGPIXELSY));
	pr.psize.x = -1; /* will be initialised to paper size whenever the printer driver changes */
	pr.psize.y = -1;
	ReleaseDC(hwnd, hdc);
	psp.dwSize      = sizeof(PROPSHEETPAGE);
	psp.dwFlags     = PSP_USETITLE;
	psp.hInstance   = lpgw->hInstance;
	psp.pszTemplate = TEXT("PrintSizeDlgBox");  //MAKEINTRESOURCE(DLG_FONT);
	psp.pszIcon     = NULL; // MAKEINTRESOURCE(IDI_FONT);
	psp.pfnDlgProc  = PrintSizeDlgProc;
	psp.pszTitle    = TEXT("Layout");
	psp.lParam      = (LPARAM)&pr;
	psp.pfnCallback = NULL;
	hpsp = CreatePropertySheetPage(&psp);
	memzero(&pd, sizeof(pd));
	pd.lStructSize = sizeof(pd);
	pd.hwndOwner = hwnd;
	pd.Flags = PD_NOPAGENUMS | PD_NOSELECTION | PD_NOCURRENTPAGE | PD_USEDEVMODECOPIESANDCOLLATE;
	pd.hDevNames = _WinM.hDevNames;
	pd.hDevMode = _WinM.hDevMode;
	pd.nCopies = 1;
	pd.nPropertyPages = 1;
	pd.lphPropertyPages = &hpsp;
	pd.nStartPage = START_PAGE_GENERAL;
	pd.lpCallback = (LPUNKNOWN)PrintingCallbackCreate(&pr);
	// remove the lower part of the "general" property sheet 
	pd.lpPrintTemplateName = TEXT("PrintDlgExEmpty");
	pd.hInstance = _WinM.P_GraphWin->hInstance;
	pd.Flags |= PD_ENABLEPRINTTEMPLATE;
	if(PrintDlgEx(&pd) != S_OK) {
		DWORD error = CommDlgExtendedError();
		if(error != 0)
			fprintf(stderr, "\nError:  Opening the print dialog failed with error code %04x.\n", error);
		PrintingCallbackFree(pd.lpCallback);
		return;
	}
	PrintingCallbackFree(pd.lpCallback);
	if(pd.dwResultAction != PD_RESULT_PRINT)
		return;
	// Print Size Dialog results 
	if(pr.psize.x < 0)
		pr.psize = pr.pdef; // apply default values 
	// See http://support.microsoft.com/kb/240082 
	pDevNames = (DEVNAMES*)GlobalLock(pd.hDevNames);
	pDevMode = (DEVMODE*)GlobalLock(pd.hDevMode);
	szDriver = (LPCTSTR)pDevNames + pDevNames->wDriverOffset;
	szDevice = (LPCTSTR)pDevNames + pDevNames->wDeviceOffset;
	szOutput = (LPCTSTR)pDevNames + pDevNames->wOutputOffset;
#if defined(HAVE_D2D11) && !defined(DCRENDERER)
	if(lpgw->d2d) {
		dpiX = dpiY = 96;  // DIPS
	}
	else
#endif
	{
		printer = CreateDC(szDriver, szDevice, szOutput, pDevMode);
		if(printer == NULL)
			goto cleanup; /* abort */
		dpiX = GetDeviceCaps(printer, LOGPIXELSX);
		dpiY = GetDeviceCaps(printer, LOGPIXELSY);
	}
	rect.left = MulDiv(pr.poff.x * 10, dpiX, 254);
	rect.top  = MulDiv(pr.poff.y * 10, dpiY, 254);
	rect.right = rect.left + MulDiv(pr.psize.x * 10, dpiX, 254);
	rect.bottom = rect.top + MulDiv(pr.psize.y * 10, dpiY, 254);
	pr.hdcPrn = printer;
	PrintRegister(&pr);
	EnableWindow(hwnd, FALSE);
	pr.bUserAbort = FALSE;
	pr.szTitle = lpgw->Title;
	pr.hDlgPrint = CreateDialogParam(hdllInstance, TEXT("CancelDlgBox"), hwnd, PrintDlgProc, (LPARAM)&pr);
	SetAbortProc(printer, PrintAbortProc);
	SetWindowLongPtr(GetDlgItem(pr.hDlgPrint, CANCEL_PROGRESS), GWL_STYLE, WS_CHILD | WS_VISIBLE | PBS_MARQUEE);
	::SendMessage(::GetDlgItem(pr.hDlgPrint, CANCEL_PROGRESS), PBM_SETMARQUEE, 1, 0);
#if defined(HAVE_D2D11) && !defined(DCRENDERER)
	if(lpgw->d2d) {
		// handle the rest in C++
		print_d2d(lpgw, pDevMode, szDevice, &rect);
	}
	else {
#endif
#ifdef HAVE_GDIPLUS
#ifndef HAVE_D2D11
	if(lpgw->gdiplus || lpgw->d2d)
#else
	if(lpgw->gdiplus)
#endif
		OpenPrinter((LPTSTR)szDevice, &printerHandle, NULL);
#endif
	INITWINAPISTRUCT(docInfo);
	docInfo.lpszDocName = lpgw->Title;
	if(StartDoc(printer, &docInfo) > 0 && StartPage(printer) > 0) {
#ifdef HAVE_GDIPLUS
#ifndef HAVE_D2D11
		if(lpgw->gdiplus || lpgw->d2d) {
#else
		if(lpgw->gdiplus) {
#endif
			// Print using GDI+ 
			print_gdiplus(lpgw, printer, printerHandle, &rect);
		}
		else
#endif
		{
#ifdef USE_WINGDI
			SetMapMode(printer, MM_TEXT);
			SetBkMode(printer, OPAQUE);
			DestroyFonts(lpgw);
			MakeFonts(lpgw, &rect, printer);
			DestroyPens(lpgw);
			MakePens(lpgw, printer);
			drawgraph(lpgw, printer, &rect);
			hdc = GetDC(hwnd);
			DestroyFonts(lpgw);
			MakeFonts(lpgw, &rect, hdc);
			ReleaseDC(hwnd, hdc);
#endif // USE_WINGDI
		}
		if(EndPage(printer) <= 0) {
			fputs("Error when finalising the print page. Aborting.\n", stderr);
			AbortDoc(printer);
		}
		else if(EndDoc(printer) <= 0) {
			fputs("Error: Could not end printer document.\n", stderr);
		}
	}
	else {
		fputs("Error: Unable to start printer document.\n", stderr);
	}

#if defined(HAVE_D2D11) && !defined(DCRENDERER)
}

#endif
	if(!pr.bUserAbort) {
		EnableWindow(hwnd, TRUE);
		DestroyWindow(pr.hDlgPrint);
	}
#ifdef HAVE_GDIPLUS
	if(printerHandle)
		ClosePrinter(printerHandle);
#endif
	if(printer)
		DeleteDC(printer);
	PrintUnregister(&pr);
cleanup:
	GlobalUnlock(pd.hDevMode);
	GlobalUnlock(pd.hDevNames);
	/* We no longer free these but preserve them for the next time
	   GlobalFree(pd.hDevMode);
	   GlobalFree(pd.hDevNames);
	 */
	_WinM.hDevNames = pd.hDevNames;
	_WinM.hDevMode = pd.hDevMode;
	/* make certain that the screen pen set is restored */
	SendMessage(lpgw->hGraph, WM_COMMAND, M_REBUILDTOOLS, 0L);
}
//
// INI file stuff 
//
static void WriteGraphIni(GW * lpgw)
{
	RECT rect;
	LPTSTR file = lpgw->IniFile;
	LPCTSTR section = lpgw->IniSection;
	TCHAR profile[80];
	UINT dpi;
#ifdef WIN_CUSTOM_PENS
	int i;
#endif
	if(!file || !section)
		return;
	// Only save window size and position for standalone graph windows. 
	if(!lpgw->bDocked) {
		if(IsIconic(lpgw->hWndGraph))
			ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
		// Rescale window size to 96dpi. 
		GetWindowRect(lpgw->hWndGraph, &rect);
		dpi = GetDPI();
		wsprintf(profile, TEXT("%d %d"), MulDiv(rect.left, 96, dpi), MulDiv(rect.top, 96, dpi));
		WritePrivateProfileString(section, TEXT("GraphOrigin"), profile, file);
		if(lpgw->Canvas_.x) {
			wsprintf(profile, TEXT("%d %d"), MulDiv(lpgw->Canvas_.x, 96, dpi), MulDiv(lpgw->Canvas_.y, 96, dpi));
			WritePrivateProfileString(section, TEXT("GraphSize"), profile, file);
		}
		else if(lpgw->Size_.x != CW_USEDEFAULT) {
			wsprintf(profile, TEXT("%d %d"), MulDiv(lpgw->Size_.x - lpgw->Decoration_.x, 96, dpi), MulDiv(lpgw->Size_.y - lpgw->Decoration_.y, 96, dpi));
			WritePrivateProfileString(section, TEXT("GraphSize"), profile, file);
		}
	}
	wsprintf(profile, TEXT("%s,%d"), lpgw->deffontname, lpgw->deffontsize);
	WritePrivateProfileString(section, TEXT("GraphFont"), profile, file);
	_tcscpy(WIN_inifontname, lpgw->deffontname);
	WIN_inifontsize = lpgw->deffontsize;
	wsprintf(profile, TEXT("%d"), lpgw->color);
	WritePrivateProfileString(section, TEXT("GraphColor"), profile, file);
	wsprintf(profile, TEXT("%d"), lpgw->graphtotop);
	WritePrivateProfileString(section, TEXT("GraphToTop"), profile, file);
	wsprintf(profile, TEXT("%d"), lpgw->oversample);
	WritePrivateProfileString(section, TEXT("GraphGDI+Oversampling"), profile, file);
	wsprintf(profile, TEXT("%d"), lpgw->gdiplus);
	WritePrivateProfileString(section, TEXT("GraphGDI+"), profile, file);
	wsprintf(profile, TEXT("%d"), lpgw->d2d);
	WritePrivateProfileString(section, TEXT("GraphD2D"), profile, file);
	wsprintf(profile, TEXT("%d"), lpgw->antialiasing);
	WritePrivateProfileString(section, TEXT("GraphAntialiasing"), profile, file);
	wsprintf(profile, TEXT("%d"), lpgw->polyaa);
	WritePrivateProfileString(section, TEXT("GraphPolygonAA"), profile, file);
	wsprintf(profile, TEXT("%d"), lpgw->fastrotation);
	WritePrivateProfileString(section, TEXT("GraphFastRotation"), profile, file);
	wsprintf(profile, TEXT("%d %d %d"), GetRValue(lpgw->background), GetGValue(lpgw->background), GetBValue(lpgw->background));
	WritePrivateProfileString(section, TEXT("GraphBackground"), profile, file);
#ifdef WIN_CUSTOM_PENS
	// now save pens 
	for(i = 0; i < WGNUMPENS + 2; i++) {
		TCHAR entry[32];
		LPLOGPEN pc;
		LPLOGPEN pm;
		if(i == 0)
			_tcscpy(entry, TEXT("Border"));
		else if(i == 1)
			_tcscpy(entry, TEXT("Axis"));
		else
			wsprintf(entry, TEXT("Line%d"), i - 1);
		pc = &lpgw->colorpen[i];
		pm = &lpgw->monopen[i];
		wsprintf(profile, TEXT("%d %d %d %d %d"), GetRValue(pc->lopnColor), GetGValue(pc->lopnColor), GetBValue(pc->lopnColor),
		    (pc->lopnWidth.x != 1) ? -pc->lopnWidth.x : pc->lopnStyle, (pm->lopnWidth.x != 1) ? -pm->lopnWidth.x : pm->lopnStyle);
		WritePrivateProfileString(section, entry, profile, file);
	}
#endif
}

LPCTSTR GraphDefaultFont()
{
	return (GetACP() == 932/* Japanese Shift-JIS */) ? TEXT(WINJPFONT) : TEXT(WINFONT);
}

static void ReadGraphIni(GW * lpgw)
{
	LPTSTR file = lpgw->IniFile;
	LPCTSTR section = lpgw->IniSection;
	TCHAR profile[81];
	LPTSTR p;
	int r, g, b;
	BOOL bOKINI;
	UINT dpi;
#ifdef WIN_CUSTOM_PENS
	int i;
	int colorstyle, monostyle;
#endif
	bOKINI = (file && section);
	if(!bOKINI)
		profile[0] = '\0';
	if(bOKINI)
		GetPrivateProfileString(section, TEXT("GraphOrigin"), TEXT(""), profile, 80, file);
	if((p = GetInt(profile, &lpgw->Origin_.x)) == NULL)
		lpgw->Origin_.x = CW_USEDEFAULT;
	if((p = GetInt(p, &lpgw->Origin_.y)) == NULL)
		lpgw->Origin_.y = CW_USEDEFAULT;
	if(bOKINI)
		GetPrivateProfileString(section, TEXT("GraphSize"), TEXT(""), profile, 80, file);
	if((p = GetInt(profile, &lpgw->Size_.x)) == NULL || lpgw->Size_.x < 1)
		lpgw->Size_.x = CW_USEDEFAULT;
	if((p = GetInt(p, &lpgw->Size_.y)) == NULL || lpgw->Size_.y < 1)
		lpgw->Size_.y = CW_USEDEFAULT;
	/* Saved size and origin are normalised to 96dpi. */
	dpi = GetDPI();
	if(lpgw->Origin_.x != CW_USEDEFAULT)
		lpgw->Origin_.x = MulDiv(lpgw->Origin_.x, dpi, 96);
	if(lpgw->Origin_.y != CW_USEDEFAULT)
		lpgw->Origin_.y = MulDiv(lpgw->Origin_.y, dpi, 96);
	if(lpgw->Size_.x != CW_USEDEFAULT)
		lpgw->Size_.x = MulDiv(lpgw->Size_.x, dpi, 96);
	if(lpgw->Size_.y != CW_USEDEFAULT)
		lpgw->Size_.y = MulDiv(lpgw->Size_.y, dpi, 96);
	if((lpgw->Size_.x != CW_USEDEFAULT) && (lpgw->Size_.y != CW_USEDEFAULT)) {
		lpgw->Canvas_ = lpgw->Size_;
	}
	else // 2020-10-07 shige: Initialize of Canvas.{x,y} 
		lpgw->Canvas_.Z();
	if(bOKINI)
		GetPrivateProfileString(section, TEXT("GraphFont"), TEXT(""), profile, 80, file);
	{
		LPTSTR size = _tcsrchr(profile, TEXT(','));
		if(size) {
			*size++ = '\0';
			if((p = GetInt(size, (LPINT)&lpgw->fontsize)) == NULL)
				lpgw->fontsize = WINFONTSIZE;
		}
		_tcscpy(lpgw->fontname, profile);
		if(lpgw->fontsize == 0)
			lpgw->fontsize = WINFONTSIZE;
		if(!(*lpgw->fontname))
			_tcscpy(lpgw->fontname, GraphDefaultFont());
		/* set current font as default font */
		_tcscpy(lpgw->deffontname, lpgw->fontname);
		lpgw->deffontsize = lpgw->fontsize;
		_tcscpy(WIN_inifontname, lpgw->deffontname);
		WIN_inifontsize = lpgw->deffontsize;
	}

	if(bOKINI)
		GetPrivateProfileString(section, TEXT("GraphColor"), TEXT(""), profile, 80, file);
	if((p = GetInt(profile, (LPINT)&lpgw->color)) == NULL)
		lpgw->color = TRUE;

	if(bOKINI)
		GetPrivateProfileString(section, TEXT("GraphToTop"), TEXT(""), profile, 80, file);
	if((p = GetInt(profile, (LPINT)&lpgw->graphtotop)) == NULL)
		lpgw->graphtotop = TRUE;

	if(bOKINI)
		GetPrivateProfileString(section, TEXT("GraphGDI+Oversampling"), TEXT(""), profile, 80, file);
	if((p = GetInt(profile, (LPINT)&lpgw->oversample)) == NULL)
		lpgw->oversample = TRUE;

#ifdef HAVE_GDIPLUS
	if(bOKINI)
		GetPrivateProfileString(section, TEXT("GraphGDI+"), TEXT(""), profile, 80, file);
	if((p = GetInt(profile, (LPINT)&lpgw->gdiplus)) == NULL)
		lpgw->gdiplus = TRUE;
#endif

#ifdef HAVE_D2D
	if(bOKINI)
		GetPrivateProfileString(section, TEXT("GraphD2D"), TEXT(""), profile, 80, file);
	if((p = GetInt(profile, (LPINT)&lpgw->d2d)) == NULL)
		lpgw->d2d = TRUE;
	// D2D setting overrides the GDI+ setting
	if(lpgw->d2d)
		lpgw->gdiplus = FALSE;
#endif

	if(bOKINI)
		GetPrivateProfileString(section, TEXT("GraphAntialiasing"), TEXT(""), profile, 80, file);
	if((p = GetInt(profile, (LPINT)&lpgw->antialiasing)) == NULL)
		lpgw->antialiasing = TRUE;
	if(bOKINI)
		GetPrivateProfileString(section, TEXT("GraphPolygonAA"), TEXT(""), profile, 80, file);
	if((p = GetInt(profile, (LPINT)&lpgw->polyaa)) == NULL)
		lpgw->polyaa = TRUE;
	if(bOKINI)
		GetPrivateProfileString(section, TEXT("GraphFastRotation"), TEXT(""), profile, 80, file);
	if((p = GetInt(profile, (LPINT)&lpgw->fastrotation)) == NULL)
		lpgw->fastrotation = FALSE;
	lpgw->background = RGB(255, 255, 255);
	if(bOKINI)
		GetPrivateProfileString(section, TEXT("GraphBackground"), TEXT(""), profile, 80, file);
	if(((p = GetInt(profile, (LPINT)&r)) != NULL) && ((p = GetInt(p, (LPINT)&g)) != NULL) && ((p = GetInt(p, (LPINT)&b)) != NULL))
		lpgw->background = RGB(r, g, b);
#ifdef WIN_CUSTOM_PENS
	StorePen(lpgw, 0, RGB(0, 0, 0), PS_SOLID, PS_SOLID);
	if(bOKINI)
		GetPrivateProfileString(section, TEXT("Border"), TEXT(""), profile, 80, file);
	if(((p = GetInt(profile, (LPINT)&r)) != NULL) && ((p = GetInt(p, (LPINT)&g)) != NULL) && ((p = GetInt(p, (LPINT)&b)) != NULL) &&
	    ((p = GetInt(p, (LPINT)&colorstyle)) != NULL) && ((p = GetInt(p, (LPINT)&monostyle)) != NULL))
		StorePen(lpgw, 0, RGB(r, g, b), colorstyle, monostyle);
	StorePen(lpgw, 1, RGB(192, 192, 192), PS_DOT, PS_DOT);
	if(bOKINI)
		GetPrivateProfileString(section, TEXT("Axis"), TEXT(""), profile, 80, file);
	if(((p = GetInt(profile, (LPINT)&r)) != NULL) && ((p = GetInt(p, (LPINT)&g)) != NULL) &&
	    ((p = GetInt(p, (LPINT)&b)) != NULL) && ((p = GetInt(p, (LPINT)&colorstyle)) != NULL) && ((p = GetInt(p, (LPINT)&monostyle)) != NULL) )
		StorePen(lpgw, 1, RGB(r, g, b), colorstyle, monostyle);
	for(i = 0; i < WGNUMPENS; i++) {
		TCHAR entry[32];
		COLORREF ref = wginitcolor[i % WGDEFCOLOR];
		colorstyle = wginitstyle[(i / WGDEFCOLOR) % WGDEFSTYLE];
		monostyle  = wginitstyle[i % WGDEFSTYLE];
		StorePen(lpgw, i + 2, ref, colorstyle, monostyle);
		wsprintf(entry, TEXT("Line%d"), i + 1);
		if(bOKINI)
			GetPrivateProfileString(section, entry, TEXT(""), profile, 80, file);
		if(((p = GetInt(profile, (LPINT)&r)) != NULL) && ((p = GetInt(p, (LPINT)&g)) != NULL) && ((p = GetInt(p, (LPINT)&b)) != NULL) &&
		    ((p = GetInt(p, (LPINT)&colorstyle)) != NULL) && ((p = GetInt(p, (LPINT)&monostyle)) != NULL) )
			StorePen(lpgw, i + 2, RGB(r, g, b), colorstyle, monostyle);
	}
#endif
}

/* ================================== */
//
// Hypertext support functions 
//
void add_tooltip(GW * lpgw, PRECT rect, LPWSTR text)
{
	int idx = lpgw->numtooltips;
	// Extend buffer, if necessary 
	if(lpgw->numtooltips >= lpgw->maxtooltips) {
		lpgw->maxtooltips += 10;
		lpgw->tooltips = (tooltips *)SAlloc::R(lpgw->tooltips, lpgw->maxtooltips * sizeof(struct tooltips));
	}
	lpgw->tooltips[idx].rect = *rect;
	lpgw->tooltips[idx].text = text;
	lpgw->numtooltips++;
	if(!lpgw->hTooltip) {
		TOOLINFO ti;
		// Create new tooltip. 
		HWND hwnd = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, lpgw->hGraph, NULL, lpgw->hInstance, NULL);
		lpgw->hTooltip = hwnd;
		// Associate the tooltip with the rect area.
		INITWINAPISTRUCT(ti);
		ti.uFlags   = TTF_SUBCLASS;
		ti.hwnd     = lpgw->hGraph;
		ti.hinst    = lpgw->hInstance;
		ti.uId      = 0;
		ti.rect     = *rect;
		ti.lpszText = (LPTSTR)text;
		::SendMessage(hwnd, TTM_ADDTOOLW, 0, (LPARAM)(LPTOOLINFO)&ti);
		::SendMessage(hwnd, TTM_SETDELAYTIME, TTDT_INITIAL, (LPARAM)100);
		::SendMessage(hwnd, TTM_SETDELAYTIME, TTDT_RESHOW, (LPARAM)100);
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE | SWP_NOACTIVATE);
	}
}

void clear_tooltips(GW * lpgw)
{
	for(uint i = 0; i < lpgw->numtooltips; i++)
		SAlloc::F(lpgw->tooltips[i].text);
	lpgw->numtooltips = 0;
	lpgw->maxtooltips = 0;
	SAlloc::F(lpgw->tooltips);
	lpgw->tooltips = NULL;
}

/* ================================== */

#ifdef WIN_CUSTOM_PENS

/* the "Line Styles..." dialog and its support functions */
/* FIXME HBB 20010218: this might better be delegated to a separate source file */

#define LS_DEFLINE 2
typedef struct tagLS {
	LONG_PTR widtype;
	int wid;
	HWND hwnd;
	int pen;                                /* current pen number */
	LOGPEN colorpen[WGNUMPENS+2]; /* logical color pens */
	LOGPEN monopen[WGNUMPENS+2]; /* logical mono pens */
} LS;
typedef LS *  LPLS;
#endif

static COLORREF GetColor(HWND hwnd, COLORREF ref)
{
	CHOOSECOLOR cc;
	COLORREF aclrCust[16];
	for(int i = 0; i < SIZEOFARRAY(aclrCust); i++) {
		aclrCust[i] = RGB(0, 0, 0);
	}
	memzero(&cc, sizeof(CHOOSECOLOR));
	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = hwnd;
	cc.lpCustColors = aclrCust;
	cc.rgbResult = ref;
	cc.Flags = CC_RGBINIT;
	if(ChooseColor(&cc))
		return cc.rgbResult;
	return ref;
}

#ifdef WIN_CUSTOM_PENS

// force update of owner draw button 
static void UpdateColorSample(HWND hdlg)
{
	RECT rect;
	POINT ptul, ptlr;
	GetWindowRect(GetDlgItem(hdlg, LS_COLORSAMPLE), &rect);
	ptul.x = rect.left;
	ptul.y = rect.top;
	ptlr.x = rect.right;
	ptlr.y = rect.bottom;
	ScreenToClient(hdlg, &ptul);
	ScreenToClient(hdlg, &ptlr);
	rect.left   = ptul.x;
	rect.top    = ptul.y;
	rect.right  = ptlr.x;
	rect.bottom = ptlr.y;
	InvalidateRect(hdlg, &rect, TRUE);
	UpdateWindow(hdlg);
}

// Window handler function for the "Line Styles" dialog 
INT_PTR CALLBACK LineStyleDlgProc(HWND hdlg, UINT wmsg, WPARAM wparam, LPARAM lparam) // @callback(DLGPROC)
{
	TCHAR buf[16];
	LPLS lpls;
	int i;
	UINT pen;
	LPLOGPEN plpm, plpc;
	lpls = (LPLS)GetWindowLongPtr(GetParent(hdlg), 4);
	switch(wmsg) {
		case WM_INITDIALOG:
		    pen = 2;
		    for(i = 0; i < WGNUMPENS + 2; i++) {
			    if(i == 0)
				    _tcscpy(buf, TEXT("Border"));
			    else if(i == 1)
				    _tcscpy(buf, TEXT("Axis"));
			    else
				    wsprintf(buf, TEXT("Line%d"), i - 1);
			    SendDlgItemMessage(hdlg, LS_LINENUM, LB_ADDSTRING, 0, (LPARAM)(buf));
		    }
		    SendDlgItemMessage(hdlg, LS_LINENUM, LB_SETCURSEL, pen, 0L);
		    SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_ADDSTRING, 0, (LPARAM)(TEXT("Solid")));
		    SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_ADDSTRING, 0, (LPARAM)(TEXT("Dash")));
		    SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_ADDSTRING, 0, (LPARAM)(TEXT("Dot")));
		    SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_ADDSTRING, 0, (LPARAM)(TEXT("DashDot")));
		    SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_ADDSTRING, 0, (LPARAM)(TEXT("DashDotDot")));
		    plpm = &lpls->monopen[pen];
		    SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_SETCURSEL, plpm->lopnStyle, 0L);
		    wsprintf(buf, TEXT("%d"), plpm->lopnWidth.x);
		    SetDlgItemText(hdlg, LS_MONOWIDTH, buf);
		    SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_ADDSTRING, 0, (LPARAM)(TEXT("Solid")));
		    SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_ADDSTRING, 0, (LPARAM)(TEXT("Dash")));
		    SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_ADDSTRING, 0, (LPARAM)(TEXT("Dot")));
		    SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_ADDSTRING, 0, (LPARAM)(TEXT("DashDot")));
		    SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_ADDSTRING, 0, (LPARAM)(TEXT("DashDotDot")));
		    plpc = &lpls->colorpen[pen];
		    SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_SETCURSEL, plpc->lopnStyle, 0L);
		    wsprintf(buf, TEXT("%d"), plpc->lopnWidth.x);
		    SetDlgItemText(hdlg, LS_COLORWIDTH, buf);
		    return TRUE;
		case WM_COMMAND:
		    pen = (UINT)SendDlgItemMessage(hdlg, LS_LINENUM, LB_GETCURSEL, 0, 0L);
		    plpm = &lpls->monopen[pen];
		    plpc = &lpls->colorpen[pen];
		    switch(LOWORD(wparam)) {
			    case LS_LINENUM:
					wsprintf(buf, TEXT("%d"), plpm->lopnWidth.x);
					SetDlgItemText(hdlg, LS_MONOWIDTH, buf);
					SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_SETCURSEL, plpm->lopnStyle, 0L);
					wsprintf(buf, TEXT("%d"), plpc->lopnWidth.x);
					SetDlgItemText(hdlg, LS_COLORWIDTH, buf);
					SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_SETCURSEL, plpc->lopnStyle, 0L);
					UpdateColorSample(hdlg);
					return FALSE;
			    case LS_MONOSTYLE:
					plpm->lopnStyle = (UINT)SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_GETCURSEL, 0, 0L);
					if(plpm->lopnStyle) {
						plpm->lopnWidth.x = 1;
						wsprintf(buf, TEXT("%d"), plpm->lopnWidth.x);
						SetDlgItemText(hdlg, LS_MONOWIDTH, buf);
					}
					return FALSE;
			    case LS_MONOWIDTH:
					GetDlgItemText(hdlg, LS_MONOWIDTH, buf, 15);
					GetInt(buf, (LPINT)&plpm->lopnWidth.x);
					if(plpm->lopnWidth.x != 1) {
						plpm->lopnStyle = 0;
						SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_SETCURSEL, plpm->lopnStyle, 0L);
					}
					return FALSE;
			    case LS_CHOOSECOLOR:
					plpc->lopnColor = GetColor(hdlg, plpc->lopnColor);
					UpdateColorSample(hdlg);
					return FALSE;
			    case LS_COLORSTYLE:
					plpc->lopnStyle = (UINT)SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_GETCURSEL, 0, 0L);
					if(plpc->lopnStyle) {
						plpc->lopnWidth.x = 1;
						wsprintf(buf, TEXT("%d"), plpc->lopnWidth.x);
						SetDlgItemText(hdlg, LS_COLORWIDTH, buf);
					}
					return FALSE;
			    case LS_COLORWIDTH:
					GetDlgItemText(hdlg, LS_COLORWIDTH, buf, 15);
					GetInt(buf, (LPINT)&plpc->lopnWidth.x);
					if(plpc->lopnWidth.x != 1) {
						plpc->lopnStyle = 0;
						SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_SETCURSEL, plpc->lopnStyle, 0L);
					}
					return FALSE;
			    case LS_DEFAULT:
					plpm = lpls->monopen;
					plpc = lpls->colorpen;
					/* border */
					plpc->lopnColor   = RGB(0, 0, 0);
					plpc->lopnStyle   = PS_SOLID;
					plpc->lopnWidth.x = 1;
					plpm->lopnStyle   = PS_SOLID;
					plpm->lopnWidth.x = 1;
					plpc++; plpm++;
					/* axis */
					plpc->lopnColor   = RGB(192, 192, 192);
					plpc->lopnStyle   = PS_DOT;
					plpc->lopnWidth.x = 1;
					plpm->lopnStyle   = PS_DOT;
					plpm->lopnWidth.x = 1;
					/* LineX */
					for(i = 0; i<WGNUMPENS; i++) {
						plpc++; plpm++;
						plpc->lopnColor   = wginitcolor[ i%WGDEFCOLOR ];
						plpc->lopnStyle   = wginitstyle[ (i/WGDEFCOLOR) % WGDEFSTYLE ];
						plpc->lopnWidth.x = 1;
						plpm->lopnStyle   = wginitstyle[ i%WGDEFSTYLE ];
						plpm->lopnWidth.x = 1;
					}
					/* update window */
					plpm = &lpls->monopen[pen];
					plpc = &lpls->colorpen[pen];
					SendDlgItemMessage(hdlg, LS_LINENUM, LB_SETCURSEL, pen, 0L);
					wsprintf(buf, TEXT("%d"), plpm->lopnWidth.x);
					SetDlgItemText(hdlg, LS_MONOWIDTH, buf);
					SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_SETCURSEL, plpm->lopnStyle, 0L);
					wsprintf(buf, TEXT("%d"), plpc->lopnWidth.x);
					SetDlgItemText(hdlg, LS_COLORWIDTH, buf);
					SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_SETCURSEL, plpc->lopnStyle, 0L);
					UpdateColorSample(hdlg);
					return FALSE;
			    case IDOK:
					EndDialog(hdlg, IDOK);
					return TRUE;
			    case IDCANCEL:
					EndDialog(hdlg, IDCANCEL);
					return TRUE;
		    }
		    break;
		case WM_DRAWITEM:
			{
				LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lparam;
				pen = (UINT)SendDlgItemMessage(hdlg, LS_LINENUM, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				plpc = &lpls->colorpen[pen];
				HBRUSH hBrush = CreateSolidBrush(plpc->lopnColor);
				FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
				FrameRect(lpdis->hDC, &lpdis->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
				DeleteObject(hBrush);
			}
		    return FALSE;
	}
	return FALSE;
}

/* GetWindowLongPtr(hwnd, 4) must be available for use */
static BOOL LineStyle(GW * lpgw)
{
	BOOL status = FALSE;
	LS ls;
	SetWindowLongPtr(lpgw->hWndGraph, 4, (LONG_PTR)&ls);
	memcpy(&ls.colorpen, &lpgw->colorpen, (WGNUMPENS + 2) * sizeof(LOGPEN));
	memcpy(&ls.monopen, &lpgw->monopen, (WGNUMPENS + 2) * sizeof(LOGPEN));
	if(DialogBox(hdllInstance, TEXT("LineStyleDlgBox"), lpgw->hWndGraph, LineStyleDlgProc) == IDOK) {
		memcpy(&lpgw->colorpen, &ls.colorpen, (WGNUMPENS + 2) * sizeof(LOGPEN));
		memcpy(&lpgw->monopen, &ls.monopen, (WGNUMPENS + 2) * sizeof(LOGPEN));
		status = TRUE;
	}
	SetWindowLongPtr(lpgw->hWndGraph, 4, 0L);
	return status;
}

#endif /* WIN_CUSTOM_PENS */

#ifdef USE_MOUSE
/* ================================== */
/* helper functions: wrapper around gp_exec_event and DrawZoomBox. */

static void Wnd_exec_event(GW * lpgw, LPARAM lparam, char type, int par1)
{
	static ulong lastTimestamp = 0;
	ulong thisTimestamp = GetMessageTime();
	int mx, my;
	bool old = FALSE;
	const int par2 = (type != GE_keypress/*no timestamp for key events*/) ? (thisTimestamp - lastTimestamp) : 0;
	// map events from inactive graph windows 
	if(lpgw != _WinM.P_GraphWin) {
		switch(type) {
			case GE_keypress:
			    type = GE_keypress_old;
			    old = TRUE;
			    break;
			case GE_buttonpress:
			    type = GE_buttonpress_old;
			    old = TRUE;
			    break;
			case GE_buttonrelease:
			    type = GE_buttonrelease_old;
			    old = TRUE;
			    break;
		}
	}
	if(GPT.P_Term && sstreq(GPT.P_Term->GetName(), "windows") && ((lpgw == _WinM.P_GraphWin) || old)) {
		GetMousePosViewport(lpgw, &mx, &my);
		gp_exec_event(type, mx, my, par1, par2, 0);
		lastTimestamp = thisTimestamp;
	}
	// FIXME: IMHO changing paused_for_mouse in terminal code is bad design. 
	// end pause mouse? 
	if((type == GE_buttonrelease) && (paused_for_mouse & PAUSE_CLICK) && (((par1 == 1) && (paused_for_mouse & PAUSE_BUTTON1)) ||
	    ((par1 == 2) && (paused_for_mouse & PAUSE_BUTTON2)) || ((par1 == 3) && (paused_for_mouse & PAUSE_BUTTON3)))) {
		paused_for_mouse = 0;
	}
	if((type == GE_keypress) && (paused_for_mouse & PAUSE_KEYSTROKE) && par1) {
		paused_for_mouse = 0;
	}
}
#endif /* USE_MOUSE */

/* ================================== */

static void GraphUpdateMenu(GW * lpgw)
{
	CheckMenuItem(lpgw->hPopMenu, M_COLOR, MF_BYCOMMAND | (lpgw->color ? MF_CHECKED : MF_UNCHECKED));
	if(lpgw->gdiplus)
		CheckMenuRadioItem(lpgw->hPopMenu, M_GDI, M_D2D, M_GDIPLUS, MF_BYCOMMAND);
	else if(lpgw->d2d)
		CheckMenuRadioItem(lpgw->hPopMenu, M_GDI, M_D2D, M_D2D, MF_BYCOMMAND);
	else
		CheckMenuRadioItem(lpgw->hPopMenu, M_GDI, M_D2D, M_GDI, MF_BYCOMMAND);
#if defined(HAVE_GDIPLUS) || defined(HAVE_D2D)
	if(lpgw->gdiplus || lpgw->d2d) {
		EnableMenuItem(lpgw->hPopMenu, M_ANTIALIASING, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(lpgw->hPopMenu, M_POLYAA, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(lpgw->hPopMenu, M_OVERSAMPLE, MF_BYCOMMAND | MF_ENABLED);
	}
	else {
		EnableMenuItem(lpgw->hPopMenu, M_ANTIALIASING, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(lpgw->hPopMenu, M_POLYAA, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(lpgw->hPopMenu, M_OVERSAMPLE, MF_BYCOMMAND | MF_GRAYED);
	}
	EnableMenuItem(lpgw->hPopMenu, M_FASTROTATE, MF_BYCOMMAND | (lpgw->gdiplus ? MF_ENABLED : MF_DISABLED));
	CheckMenuItem(lpgw->hPopMenu, M_ANTIALIASING, MF_BYCOMMAND | ((lpgw->gdiplus || lpgw->d2d) && lpgw->antialiasing ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(lpgw->hPopMenu, M_OVERSAMPLE, MF_BYCOMMAND | ((lpgw->gdiplus || lpgw->d2d) && lpgw->oversample ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(lpgw->hPopMenu, M_FASTROTATE, MF_BYCOMMAND | (lpgw->gdiplus && lpgw->fastrotation ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(lpgw->hPopMenu, M_POLYAA, MF_BYCOMMAND | ((lpgw->gdiplus || lpgw->d2d) && lpgw->polyaa ? MF_CHECKED : MF_UNCHECKED));
#endif
	CheckMenuItem(lpgw->hPopMenu, M_GRAPH_TO_TOP, MF_BYCOMMAND | (lpgw->graphtotop ? MF_CHECKED : MF_UNCHECKED));
}

LRESULT CALLBACK WndGraphParentProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) // @callback
{
	if(message == WM_CREATE) {
		GW * lpgw = (GW *)((CREATESTRUCT*)lParam)->lpCreateParams;
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)lpgw);
		if(lpgw->lptw && lpgw->lptw->DragPre && lpgw->lptw->DragPost)
			DragAcceptFiles(hwnd, TRUE);
		return 0;
	}
	else if(message == WM_ERASEBKGND)
		return 1; // we erase the background ourselves 
	else {
		HDC hdc;
		RECT rect;
		GW * lpgw = (GW *)GetWindowLongPtr(hwnd, 0);
		switch(message) {
			case WM_SIZE: 
				{
					BOOL rebuild = FALSE;
					if(lpgw->hStatusbar)
						SendMessage(lpgw->hStatusbar, WM_SIZE, wParam, lParam);
					if(lpgw->hToolbar) {
						::SendMessage(lpgw->hToolbar, WM_SIZE, wParam, lParam);
						/* make room */
						GetWindowRect(lpgw->hToolbar, &rect);
						lpgw->ToolbarHeight = rect.bottom - rect.top;
					}
					if((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)) {
						uint width, height;
						GetWindowRect(hwnd, &rect);
						width = rect.right - rect.left;
						height = rect.bottom - rect.top;
						/* Ignore minimize / de-minimize */
						if((lpgw->Size_.x != width) || (lpgw->Size_.y != height)) {
							lpgw->Size_.Set(width, height);
							rebuild = TRUE;
						}
					}
					// update the actual graph window
					{
						GetClientRect(hwnd, &rect);
						SetWindowPos(lpgw->hGraph, NULL, 0, lpgw->ToolbarHeight, rect.right - rect.left, rect.bottom - rect.top - lpgw->ToolbarHeight - lpgw->StatusHeight,
						SWP_NOACTIVATE | SWP_NOZORDER);
					}
					if(rebuild) {
						/* remake fonts */
						lpgw->buffervalid = FALSE;
						DestroyFonts(lpgw);
						GetPlotRect(lpgw, &rect);
						hdc = GetDC(lpgw->hGraph);
						MakeFonts(lpgw, &rect, hdc);
						ReleaseDC(lpgw->hGraph, hdc);
						InvalidateRect(lpgw->hGraph, &rect, 1);
						UpdateWindow(lpgw->hGraph);
					}
					// update internal variables
					if(lpgw->Size_.x == CW_USEDEFAULT) {
						lpgw->Size_.Set(LOWORD(lParam), HIWORD(lParam));
					}
				}
				break;
			case WM_MOVE:
				GetWindowRect(hwnd, &rect);
				lpgw->Origin_.Set(rect.left, rect.top);
				break;
			case WM_SYSCOMMAND:
				switch(LOWORD(wParam)) {
					case M_GRAPH_TO_TOP:
					case M_COLOR:
					case M_OVERSAMPLE:
					case M_GDI:
					case M_GDIPLUS:
					case M_D2D:
					case M_ANTIALIASING:
					case M_POLYAA:
					case M_FASTROTATE:
					case M_CHOOSE_FONT:
					case M_COPY_CLIP:
					case M_SAVE_AS_EMF:
					case M_SAVE_AS_BITMAP:
					case M_LINESTYLE:
					case M_BACKGROUND:
					case M_PRINT:
					case M_WRITEINI:
					case M_REBUILDTOOLS:
						SendMessage(lpgw->hGraph, WM_COMMAND, wParam, lParam);
						break;
	#ifndef WGP_CONSOLE
					case M_ABOUT:
						if(lpgw->lptw)
							AboutBox(hwnd, lpgw->lptw->AboutText);
						return 0;
	#endif
					case M_COMMANDLINE: 
						{
							HMENU sysmenu = GetSystemMenu(lpgw->hWndGraph, 0);
							int i = GetMenuItemCount(sysmenu);
							DeleteMenu(sysmenu, --i, MF_BYPOSITION);
							DeleteMenu(sysmenu, --i, MF_BYPOSITION);
							if(lpgw->lptw)
								ShowWindow(lpgw->lptw->hWndParent, SW_SHOWNORMAL);
						}
						break;
				}
				break;
			case WM_COMMAND:
			case WM_CHAR:
			case WM_KEYDOWN:
			case WM_KEYUP:
				SendMessage(lpgw->hGraph, message, wParam, lParam); // forward to graph window
				return 0;
			case WM_NOTIFY:
				switch(((LPNMHDR)lParam)->code) {
					case TBN_DROPDOWN: 
						{
							RECT rc;
							TPMPARAMS tpm;
							LPNMTOOLBAR lpnmTB = (LPNMTOOLBAR)lParam;
							SendMessage(lpnmTB->hdr.hwndFrom, TB_GETRECT, (WPARAM)lpnmTB->iItem, (LPARAM)&rc);
							MapWindowPoints(lpnmTB->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2);
							INITWINAPISTRUCT(tpm);
							tpm.rcExclude = rc;
							TrackPopupMenuEx(lpgw->hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, lpgw->hWndGraph, &tpm);
						}
						return TBDDRET_DEFAULT;
					case TTN_GETDISPINFO: 
						{
							LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
							UINT_PTR idButton = lpttt->hdr.idFrom;
							lpttt->hinst = 0;
							switch(idButton) {
								case M_COPY_CLIP: _tcscpy(lpttt->szText, TEXT("Copy graph to clipboard")); break;
								case M_PRINT: _tcscpy(lpttt->szText, TEXT("Print graph")); break;
								case M_SAVE_AS_EMF: _tcscpy(lpttt->szText, TEXT("Save graph as EMF")); break;
								case M_HIDEGRID: _tcscpy(lpttt->szText, TEXT("Do not draw grid lines")); break;
							}
							if((idButton >= M_HIDEPLOT) && (idButton < (M_HIDEPLOT + MAXPLOTSHIDE))) {
								uint index = (uint)idButton - (M_HIDEPLOT)+1;
								wsprintf(lpttt->szText, TEXT("Hide graph #%i"), index);
							}
							lpttt->uFlags |= TTF_DI_SETITEM;
						}
						return TRUE;
				}
				return FALSE;
			case WM_PARENTNOTIFY:
				// Message from status bar (or another child window): 
	#ifdef USE_MOUSE
				// Cycle through mouse-mode on button 1 click 
				if(LOWORD(wParam) == WM_LBUTTONDOWN) {
					int y = HIWORD(lParam);
					RECT rect;
					GetClientRect(hwnd, &rect);
					if(y > rect.bottom - lpgw->StatusHeight)
						Wnd_exec_event(lpgw, lParam, GE_keypress, (TCHAR)'1'); // simulate keyboard event '1' 
					return 0;
				}
	#endif
				// Context menu is handled below, everything else is not 
				if(LOWORD(wParam) != WM_CONTEXTMENU)
					return 1;
				// intentionally fall through 
			case WM_CONTEXTMENU: {
				// Note that this only works via mouse in `unset mouse`
				// mode. You can access the popup via the System menu,
				// status bar or keyboard (Shift-F10, Menu-Key) instead. 
				POINT pt;
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);
				if(pt.x == -1) {      /* keyboard activation */
					pt.x = pt.y = 0;
					ClientToScreen(hwnd, &pt);
				}
				TrackPopupMenu(lpgw->hPopMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
				return 0;
			}
		}
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}
//
// The toplevel function of this module: Window handler function of the graph window 
//
LRESULT CALLBACK WndGraphProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) // @callback
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
#ifdef USE_MOUSE
	static uint last_modifier_mask = -99;
#endif
	GW * lpgw = (GW *)GetWindowLongPtr(hwnd, 0);
#ifdef USE_MOUSE
	// mouse events first 
	if(lpgw == _WinM.P_GraphWin && mouse_setting.on) {
		switch(message) {
			case WM_MOUSEMOVE:
			    SetCursor(lpgw->hptrCurrent);
			    if(zoombox.on) {
				    //Wnd_refresh_zoombox(lpgw, lParam);
					//static void Wnd_refresh_zoombox(GW * lpgw, LPARAM lParam)
					if(lpgw == _WinM.P_GraphWin) {
						int mx, my;
						GetMousePosViewport(lpgw, &mx, &my);
						DrawZoomBox(lpgw); // erase current zoom box 
						zoombox.to.x = mx; 
						zoombox.to.y = my;
						DrawZoomBox(lpgw); // draw new zoom box 
					}
			    }
			    if(ruler.on && ruler_lineto.on) {
				    //Wnd_refresh_ruler_lineto(lpgw, lParam);
					//static void Wnd_refresh_ruler_lineto(GW * lpgw, LPARAM lParam)
					if(lpgw == _WinM.P_GraphWin) {
						int mx, my;
						GetMousePosViewport(lpgw, &mx, &my);
						DrawRulerLineTo(lpgw); // erase current line 
						ruler_lineto.x = mx; 
						ruler_lineto.y = my;
						DrawRulerLineTo(lpgw); // draw new line box 
					}
			    }
			    // track hypertexts 
			    //track_tooltip(lpgw, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				//static void track_tooltip(GW * lpgw, int x, int y)
				{
					static POINT p = {0, 0};
					// only update if mouse position changed 
					const int x = GET_X_LPARAM(lParam);
					const int y = GET_Y_LPARAM(lParam);
					if(p.x != x || p.y != y) {
						p.x = x; 
						p.y = y;
						for(uint i = 0; i < lpgw->numtooltips; i++) {
							if(PtInRect(&(lpgw->tooltips[i].rect), p)) {
								TOOLINFO ti;
								int width;
								INITWINAPISTRUCT(ti);
								ti.hwnd     = lpgw->hGraph;
								ti.hinst    = lpgw->hInstance;
								ti.rect     = lpgw->tooltips[i].rect;
								ti.lpszText = (LPTSTR)lpgw->tooltips[i].text;
								SendMessage(lpgw->hTooltip, TTM_NEWTOOLRECT, 0, (LPARAM)(LPTOOLINFO)&ti);
								SendMessage(lpgw->hTooltip, TTM_UPDATETIPTEXTW, 0, (LPARAM)(LPTOOLINFO)&ti);
								// Multi-line tooltip. 
								width = (wcschr(lpgw->tooltips[i].text, L'\n') == NULL) ? -1 : 200;
								SendMessage(lpgw->hTooltip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)width);
							}
						}
					}
				}
			    // track (show) mouse position -- send the event to gnuplot 
			    Wnd_exec_event(lpgw, lParam, GE_motion, wParam);
			    return 0L; // end of WM_MOUSEMOVE 
			case WM_LBUTTONDOWN: 
				{
					const int x = GET_X_LPARAM(lParam);
					const int y = GET_Y_LPARAM(lParam);
					// need to set input focus to current graph 
					if(lpgw->bDocked)
						SetFocus(hwnd);
					for(uint i = 0; (i < lpgw->numplots) && (i < lpgw->maxkeyboxes) && (i < lpgw->maxhideplots); i++) {
						if((lpgw->keyboxes[i].left != INT_MAX) && (x >= lpgw->keyboxes[i].left) && (x <= lpgw->keyboxes[i].right) &&
							(y <= lpgw->keyboxes[i].top) && (y >= lpgw->keyboxes[i].bottom)) {
							lpgw->hideplot[i] = !lpgw->hideplot[i];
							if(i < MAXPLOTSHIDE)
								::SendMessage(lpgw->hToolbar, TB_CHECKBUTTON, M_HIDEPLOT + i, (LPARAM)lpgw->hideplot[i]);
							lpgw->buffervalid = FALSE;
							GetClientRect(hwnd, &rect);
							InvalidateRect(hwnd, &rect, 1);
							UpdateWindow(hwnd);
							return 0L;
						}
					}
					Wnd_exec_event(lpgw, lParam, GE_buttonpress, 1);
				}
				return 0L;
			case WM_RBUTTONDOWN:
			    // FIXME HBB 20010207: this collides with the right-click context menu !!! 
			    Wnd_exec_event(lpgw, lParam, GE_buttonpress, 3);
			    return 0L;
			case WM_MBUTTONDOWN:
			    Wnd_exec_event(lpgw, lParam, GE_buttonpress, 2);
			    return 0L;
			case WM_MOUSEWHEEL: // shige, BM : mouse wheel support 
			case WM_MOUSEHWHEEL: 
				{
					const WORD fwKeys = LOWORD(wParam);
					const short zDelta = HIWORD(wParam);
					const int modifier_mask = ((fwKeys & MK_SHIFT) ? Mod_Shift : 0) | ((fwKeys & MK_CONTROL) ? Mod_Ctrl : 0) | ((fwKeys & MK_ALT) ? Mod_Alt : 0);
					if(last_modifier_mask != modifier_mask) {
						Wnd_exec_event(lpgw, lParam, GE_modifier, modifier_mask);
						last_modifier_mask = modifier_mask;
					}
					if(message == WM_MOUSEWHEEL) {
						Wnd_exec_event(lpgw, lParam, GE_buttonpress, zDelta > 0 ? 4 : 5);
						Wnd_exec_event(lpgw, lParam, GE_buttonrelease, zDelta > 0 ? 4 : 5);
					}
					else {
						Wnd_exec_event(lpgw, lParam, GE_buttonpress, zDelta > 0 ? 6 : 7);
						Wnd_exec_event(lpgw, lParam, GE_buttonrelease, zDelta > 0 ? 6 : 7);
					}
				}
				return 0L;
			case WM_LBUTTONUP: Wnd_exec_event(lpgw, lParam, GE_buttonrelease, 1); return 0L;
			case WM_RBUTTONUP: Wnd_exec_event(lpgw, lParam, GE_buttonrelease, 3); return 0L;
			case WM_MBUTTONUP: Wnd_exec_event(lpgw, lParam, GE_buttonrelease, 2); return 0L;
		} /* switch over mouse events */
	}
#endif /* USE_MOUSE */
	switch(message) {
#ifndef WGP_CONSOLE
		case WM_SETFOCUS:
		    if(lpgw->bDocked) {
			    char status[100];
			    // register input focus
			    lpgw->lptw->hWndFocus = hwnd;
			    DrawFocusIndicator(lpgw);
			    if(lpgw != _WinM.P_GraphWin) {
				    sprintf(status, "(inactive, window number %i)", lpgw->Id);
				    UpdateStatusLine(lpgw, status);
			    }
		    }
		    break;
		case WM_KILLFOCUS:
		    if(lpgw->bDocked) {
			    // remove focus indicator by enforcing a redraw
			    GetClientRect(hwnd, &rect);
			    InvalidateRect(hwnd, &rect, 1);
			    UpdateWindow(hwnd);
		    }
		    break;
#endif
		case WM_CHAR:
		    // All 'normal' keys (letters, digits and the likes) end up here... 
#ifndef DISABLE_SPACE_RAISES_CONSOLE
		    if(wParam == VK_SPACE) {
#ifndef WGP_CONSOLE
			    if(lpgw->bDocked)
				    SetFocus(lpgw->lptw->hWndText);
			    else
#endif
			    WinRaiseConsole();
			    return 0L;
		    }
#endif /* DISABLE_SPACE_RAISES_CONSOLE */
		    if(wParam == 'q') {
			    GraphClose(lpgw);
#ifndef WGP_CONSOLE
			    if(lpgw->bDocked) {
				    DockedUpdateLayout(lpgw->lptw);
				    SetFocus(lpgw->lptw->hWndText);
			    }
#endif
			    return 0L;
		    }
#ifdef USE_MOUSE
		    Wnd_exec_event(lpgw, lParam, GE_keypress, (TCHAR)wParam);
#endif
		    return 0L;
#ifdef USE_MOUSE
		/* "special" keys have to be caught from WM_KEYDOWN, as they
		 * don't generate WM_CHAR messages. */
		/* NB: It may not be possible to catch Alt-keys, this way */
		case WM_KEYUP:
			{
				// First, look for a change in modifier status 
				uint modifier_mask = ((GetKeyState(VK_SHIFT) < 0) ? Mod_Shift : 0 ) | ((GetKeyState(VK_CONTROL) < 0) ? Mod_Ctrl : 0) | ((GetKeyState(VK_MENU) < 0) ? Mod_Alt : 0);
				if(modifier_mask != last_modifier_mask) {
					Wnd_exec_event(lpgw, lParam, GE_modifier, modifier_mask);
					last_modifier_mask = modifier_mask;
				}
			}
		    // Ignore Key-Up events other than those of modifier keys 
		    break;
		case WM_KEYDOWN:
		    if(GetKeyState(VK_CONTROL) < 0 && wParam != VK_CONTROL) {
			    switch(wParam) {
				    case 'C': ::SendMessage(hwnd, WM_COMMAND, M_COPY_CLIP, 0L); break; // Ctrl-C: Copy to Clipboard 
				    case 'S': ::SendMessage(hwnd, WM_COMMAND, M_SAVE_AS_EMF, 0L); break; // Ctrl-S: Save As EMF 
				    case VK_END:
						// use CTRL-END as break key 
						GPO._Plt.ctrlc_flag = true;
						PostMessage(_WinM.P_GraphWin->hWndGraph, WM_NULL, 0, 0);
						break;
			    }
		    }
		    else {
			    // First, look for a change in modifier status 
			    uint modifier_mask = ((GetKeyState(VK_SHIFT) < 0) ? Mod_Shift : 0) | ((GetKeyState(VK_CONTROL) < 0) ? Mod_Ctrl : 0) | ((GetKeyState(VK_MENU) < 0) ? Mod_Alt : 0);
			    if(modifier_mask != last_modifier_mask) {
				    Wnd_exec_event(lpgw, lParam, GE_modifier, modifier_mask);
				    last_modifier_mask = modifier_mask;
			    }
		    }
		    switch(wParam) {
			    case VK_BACK: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_BackSpace); break;
			    case VK_TAB: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Tab); break;
			    case VK_RETURN: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Return); break;
			    case VK_PAUSE: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Pause); break;
			    case VK_SCROLL: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Scroll_Lock); break;
#if 0 /* HOW_IS_THIS_FOR_WINDOWS */
// HBB 20010215: not at all, AFAICS... :-( 
			    case VK_SYSRQ: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Sys_Req); break;
#endif
			    case VK_ESCAPE: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Escape); break;
			    case VK_DELETE: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Delete); break;
			    case VK_INSERT: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_KP_Insert); break;
			    case VK_HOME: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Home); break;
			    case VK_LEFT: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Left); break;
			    case VK_UP: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Up); break;
			    case VK_RIGHT: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Right); break;
			    case VK_DOWN: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Down); break;
			    case VK_END: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_End); break;
			    case VK_PRIOR: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_PageUp); break;
			    case VK_NEXT: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_PageDown); break;
			    case VK_F1: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F1); break;
			    case VK_F2: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F2); break;
			    case VK_F3: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F3); break;
			    case VK_F4: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F4); break;
			    case VK_F5: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F5); break;
			    case VK_F6: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F6); break;
			    case VK_F7: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F7); break;
			    case VK_F8: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F8); break;
			    case VK_F9: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F9); break;
			    case VK_F10: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F10); break;
			    case VK_F11: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F11); break;
			    case VK_F12: Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F12); break;
			    case VK_CANCEL: 
					GPO._Plt.ctrlc_flag = TRUE;
					PostMessage(_WinM.P_GraphWin->hWndGraph, WM_NULL, 0, 0);
					break;
		    }     /* switch (wParam) */
		    return 0L;
#endif /* USE_MOUSE */
		case WM_COMMAND:
		    switch(LOWORD(wParam)) {
			    case M_GRAPH_TO_TOP:
					lpgw->graphtotop = !lpgw->graphtotop;
					::SendMessage(hwnd, WM_COMMAND, M_REBUILDTOOLS, 0L);
					return 0;
			    case M_COLOR:
					lpgw->color = !lpgw->color;
					lpgw->dashed = !lpgw->color;
					::SendMessage(hwnd, WM_COMMAND, M_REBUILDTOOLS, 0L);
					WIN_update_options();
					return 0;
			    case M_OVERSAMPLE:
					lpgw->oversample = !lpgw->oversample;
					::SendMessage(hwnd, WM_COMMAND, M_REBUILDTOOLS, 0L);
					return 0;
			    case M_GDI:
					lpgw->gdiplus = FALSE;
					lpgw->d2d = FALSE;
					::SendMessage(hwnd, WM_COMMAND, M_REBUILDTOOLS, 0L);
					return 0;
			    case M_GDIPLUS:
					lpgw->gdiplus = TRUE;
					lpgw->d2d = FALSE;
					::SendMessage(hwnd, WM_COMMAND, M_REBUILDTOOLS, 0L);
					return 0;
			    case M_D2D:
					lpgw->d2d = TRUE;
					lpgw->gdiplus = FALSE;
#ifndef DCRENDRER
					// Only the DC renderer will use the memory bitmap
					if(lpgw->hBitmap) {
						DeleteObject(lpgw->hBitmap);
						lpgw->hBitmap = NULL;
					}
#endif
					// FIXME: Need more initialisation on backend change
					::SendMessage(hwnd, WM_COMMAND, M_REBUILDTOOLS, 0L);
					return 0;
			    case M_ANTIALIASING:
					lpgw->antialiasing = !lpgw->antialiasing;
					::SendMessage(hwnd, WM_COMMAND, M_REBUILDTOOLS, 0L);
					return 0;
			    case M_POLYAA:
					lpgw->polyaa = !lpgw->polyaa;
					::SendMessage(hwnd, WM_COMMAND, M_REBUILDTOOLS, 0L);
					return 0;
			    case M_FASTROTATE:
					lpgw->fastrotation = !lpgw->fastrotation;
					::SendMessage(hwnd, WM_COMMAND, M_REBUILDTOOLS, 0L);
					return 0;
			    case M_CHOOSE_FONT:
					SelFont(lpgw);
					WIN_update_options();
					return 0;
			    case M_PRINT:       CopyPrint(lpgw); return 0;
			    case M_COPY_CLIP:   CopyClip(lpgw); return 0;
			    case M_SAVE_AS_EMF: SaveAsEMF(lpgw); return 0;
			    case M_SAVE_AS_BITMAP:
#ifdef HAVE_GDIPLUS
					SaveAsBitmap(lpgw);
#endif
					return 0;
#ifdef WIN_CUSTOM_PENS
			    case M_LINESTYLE:
					if(LineStyle(lpgw))
						::SendMessage(hwnd, WM_COMMAND, M_REBUILDTOOLS, 0L);
					return 0;
#endif
			    case M_BACKGROUND:
					lpgw->background = GetColor(hwnd, lpgw->background);
					::SendMessage(hwnd, WM_COMMAND, M_REBUILDTOOLS, 0L);
					WIN_update_options();
					return 0;
			    case M_HIDEGRID:
					lpgw->hidegrid = ::SendMessage(lpgw->hToolbar, TB_ISBUTTONCHECKED, LOWORD(wParam), (LPARAM)0);
					lpgw->buffervalid = FALSE;
					GetClientRect(hwnd, &rect);
					InvalidateRect(hwnd, &rect, 1);
					UpdateWindow(hwnd);
					return 0;
			    case M_WRITEINI:
					WriteGraphIni(lpgw);
	#ifndef WGP_CONSOLE
					if(lpgw->lptw)
						WriteTextIni(lpgw->lptw);
	#endif
					WIN_update_options();
					return 0;
			    case M_REBUILDTOOLS:
					GraphUpdateMenu(lpgw);
					lpgw->buffervalid = FALSE;
					DestroyPens(lpgw);
					DestroyFonts(lpgw);
					hdc = GetDC(hwnd);
					MakePens(lpgw, hdc);
					GetPlotRect(lpgw, &rect);
					MakeFonts(lpgw, &rect, hdc);
					ReleaseDC(hwnd, hdc);
					GetClientRect(hwnd, &rect);
					InvalidateRect(hwnd, &rect, 1);
					UpdateWindow(hwnd);
					return 0;
		    }
		    /* handle toolbar events  */
		    if((LOWORD(wParam) >= M_HIDEPLOT) && (LOWORD(wParam) < (M_HIDEPLOT + MAXPLOTSHIDE))) {
			    uint button = LOWORD(wParam) - (M_HIDEPLOT);
			    if(button < lpgw->maxhideplots)
				    lpgw->hideplot[button] = ::SendMessage(lpgw->hToolbar, TB_ISBUTTONCHECKED, LOWORD(wParam), (LPARAM)0);
			    lpgw->buffervalid = FALSE;
			    GetClientRect(hwnd, &rect);
			    InvalidateRect(hwnd, &rect, 1);
			    UpdateWindow(hwnd);
			    return 0;
		    }
		    return 0;
		case WM_CONTEXTMENU: 
			{
				// Note that this only works via mouse in `unset mouse`
				// mode. You can access the popup via the System menu,
				// status bar or keyboard (Shift-F10, Menu-Key) instead. 
				POINT pt;
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);
				if(pt.x == -1) {      /* keyboard activation */
					pt.x = pt.y = 0;
					ClientToScreen(hwnd, &pt);
				}
				TrackPopupMenu(lpgw->hPopMenu, TPM_LEFTALIGN,
				pt.x, pt.y, 0, hwnd, NULL);
			}
			return 0;
		case WM_LBUTTONDOWN:
		    // need to set input focus to current graph 
		    if(lpgw->bDocked)
			    SetFocus(lpgw->hWndGraph);
		    break;
		case WM_CREATE:
		    lpgw = (GW *)((CREATESTRUCT*)lParam)->lpCreateParams;
		    SetWindowLongPtr(hwnd, 0, (LONG_PTR)lpgw);
#ifdef USE_MOUSE
		    LoadCursors(lpgw);
#endif
		    if(lpgw->lptw && lpgw->lptw->DragPre && lpgw->lptw->DragPost)
			    DragAcceptFiles(hwnd, TRUE);
		    return 0;
		case WM_ERASEBKGND:
		    return 1; /* we erase the background ourselves */
		case WM_PAINT: {
		    HDC memdc = NULL;
		    HBITMAP oldbmp;
		    LONG width, height;
		    LONG wwidth, wheight;
		    RECT memrect;
		    RECT wrect;
		    GetPlotRect(lpgw, &rect);
#if defined(HAVE_D2D) && !defined(DCRENDERER)
		    if(lpgw->d2d) {
			    BeginPaint(hwnd, &ps);
			    drawgraph_d2d(lpgw, hwnd, &rect);
			    EndPaint(hwnd, &ps);
		    }
		    else {
#endif
		    hdc = BeginPaint(hwnd, &ps);
		    SetMapMode(hdc, MM_TEXT);
		    SetBkMode(hdc, OPAQUE);
		    SetViewportExtEx(hdc, rect.right, rect.bottom, NULL);
		    // Was the window resized? 
		    GetWindowRect(lpgw->hWndGraph, &wrect);
		    wwidth =  wrect.right - wrect.left;
		    wheight = wrect.bottom - wrect.top;
		    if((lpgw->Size_.x != wwidth) || (lpgw->Size_.y != wheight)) {
			    DestroyFonts(lpgw);
			    MakeFonts(lpgw, &rect, hdc);
			    lpgw->buffervalid = FALSE;
		    }
		    // create memory device context for double buffering 
		    width = rect.right - rect.left;
		    height = rect.bottom - rect.top;
		    memdc = CreateCompatibleDC(hdc);
		    memrect.left = 0;
		    memrect.right = width;
		    memrect.top = 0;
		    memrect.bottom = height;
		    if(!lpgw->buffervalid || !lpgw->hBitmap) {
			    BOOL save_aa;
			    if(lpgw->hBitmap)
				    DeleteObject(lpgw->hBitmap);
			    lpgw->hBitmap = CreateCompatibleBitmap(hdc, memrect.right, memrect.bottom);
			    oldbmp = (HBITMAP)SelectObject(memdc, lpgw->hBitmap);
			    // Update window size 
			    lpgw->Size_.Set(wwidth, wheight);
			    // Temporarily switch off antialiasing during rotation (GDI+) 
			    save_aa = lpgw->antialiasing;
#ifndef FASTROT_WITH_GDI
			    if(lpgw->gdiplus && lpgw->rotating && lpgw->fastrotation)
				    lpgw->antialiasing = FALSE;
#endif
			    // draw into memdc, then copy to hdc 
#ifdef HAVE_GDIPLUS
#ifdef FASTROT_WITH_GDI
			    if(lpgw->gdiplus && !(lpgw->rotating && lpgw->fastrotation)) {
#else
			    if(lpgw->gdiplus) {
#endif
				    drawgraph_gdiplus(lpgw, memdc, &memrect);
			    }
			    else {
#endif
#if defined(HAVE_D2D) && defined(DCRENDERER)
			    if(lpgw->d2d) {
				    drawgraph_d2d(lpgw, memdc, &memrect);
			    }
			    else {
#endif
#ifdef USE_WINGDI
			    drawgraph(lpgw, memdc, &memrect);
#endif
#if defined(HAVE_D2D) && defined(DCRENDERER)
		    }
#endif
#ifdef HAVE_GDIPLUS
		    }
#endif
			    lpgw->antialiasing = save_aa; // restore antialiasing 
			    lpgw->buffervalid = !lpgw->locked; // drawing by gnuplot still in progress... 
		    }
		    else {
			    oldbmp = (HBITMAP)SelectObject(memdc, lpgw->hBitmap);
		    }
		    if(lpgw->buffervalid) {
			    BitBlt(hdc, rect.left, rect.top, width, height, memdc, 0, 0, SRCCOPY);
		    }
		    // select the old bitmap back into the device context 
		    if(memdc) {
			    SelectObject(memdc, oldbmp);
			    DeleteDC(memdc);
		    }
		    EndPaint(hwnd, &ps);
#if defined(HAVE_D2D) && !defined(DCRENDERER)
	    }
#endif
#ifndef WGP_CONSOLE
		    /* indicate input focus */
		    if(lpgw->bDocked && (GetFocus() == hwnd))
			    DrawFocusIndicator(lpgw);
#endif
#ifdef USE_MOUSE
		    // drawgraph() erases the plot window, so immediately after
		    // it has completed, all the 'real-time' stuff the gnuplot core
		    // doesn't know anything about has to be redrawn 
		    DrawRuler(lpgw);
		    DrawRulerLineTo(lpgw);
		    DrawZoomBox(lpgw);
#endif
		    UpdateToolbar(lpgw); // Update in case the number of graphs has changed 
		    return 0;
	    }
		case WM_SIZE:
		    if(GetPlotRect(lpgw, &rect)) {
			    if(lpgw->Canvas_.x != 0)
				    lpgw->Canvas_.Set(rect.right - rect.left, rect.bottom - rect.top);
#ifdef HAVE_D2D
			    if(lpgw->d2d)
				    d2dResize(lpgw, rect);
#endif
		    }
		    break;
#ifndef WGP_CONSOLE
		case WM_DROPFILES:
		    if(lpgw->lptw)
			    DragFunc(lpgw->lptw, (HDROP)wParam);
		    break;
#endif
		case WM_DESTROY:
		    lpgw->buffervalid = FALSE;
		    DeleteObject(lpgw->hBitmap);
		    lpgw->hBitmap = NULL;
		    clear_tooltips(lpgw);
		    DestroyWindow(lpgw->hTooltip);
		    lpgw->hTooltip = NULL;
		    DestroyPens(lpgw);
		    DestroyFonts(lpgw);
#ifdef USE_MOUSE
		    DestroyCursors(lpgw);
#endif
		    DragAcceptFiles(hwnd, FALSE);
		    lpgw->hWndGraph = NULL;
#ifndef WGP_CONSOLE
		    WinPersistTextClose();
#endif
		    return 0;
		case WM_CLOSE:
		    GraphClose(lpgw);
		    return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

#ifdef USE_WINGDI
static void GraphChangeFont(GW * lpgw, LPCTSTR font, int fontsize, HDC hdc, RECT rect)
{
	int newfontsize;
	bool remakefonts = FALSE;
	bool font_is_not_empty = !isempty(font);
	newfontsize = (fontsize != 0) ? fontsize : lpgw->deffontsize;
	if(font_is_not_empty) {
		remakefonts = (_tcscmp(lpgw->fontname, font) != 0) || (newfontsize != lpgw->fontsize);
	}
	else {
		remakefonts = (_tcscmp(lpgw->fontname, lpgw->deffontname) != 0) || (newfontsize != lpgw->fontsize);
	}
	remakefonts |= (lpgw->hfonth == 0);
	if(remakefonts) {
		lpgw->fontsize = newfontsize;
		_tcscpy(lpgw->fontname, font_is_not_empty ? font : lpgw->deffontname);
		SelectObject(hdc, GetStockObject(SYSTEM_FONT));
		DestroyFonts(lpgw);
		MakeFonts(lpgw, &rect, hdc);
	}
}
#endif
//
// close the terminal window 
//
void win_close_terminal_window(GW * lpgw)
{
	if(GraphHasWindow(lpgw)) {
		SendMessage(lpgw->hWndGraph, WM_CLOSE, 0L, 0L);
#ifndef WGP_CONSOLE
		if(lpgw->bDocked)
			DockedUpdateLayout(lpgw->lptw);
#endif
	}
}

#ifdef USE_MOUSE
// Implemented by Petr Mikulik, February 2001 --- the best Windows solutions come from OS/2 :-))
//
/* Firstly: terminal calls from win.trm */

/* Note that these all take lpgw as their first argument. It's an OO-type
 * 'this' pointer, sort of: it stores all the status information of the graph
 * window that we need, in a single large structure. */

void Graph_set_cursor(GW * lpgw, int c, int x, int y)
{
	switch(c) {
		case -4: /* switch off line between ruler and mouse cursor */
		    DrawRulerLineTo(lpgw);
		    ruler_lineto.on = FALSE;
		    break;
		case -3: /* switch on line between ruler and mouse cursor */
		    if(ruler.on && ruler_lineto.on)
			    break;
		    ruler_lineto.x = x;
		    ruler_lineto.y = y;
		    ruler_lineto.on = TRUE;
		    DrawRulerLineTo(lpgw);
		    break;
		case -2: { /* move mouse to the given point */
		    RECT rc;
		    POINT pt;
		    GetPlotRect(lpgw, &rc);
		    pt.x = MulDiv(x, rc.right - rc.left, lpgw->MaxS.x);
		    pt.y = rc.bottom - MulDiv(y, rc.bottom - rc.top, lpgw->MaxS.y);
		    MapWindowPoints(lpgw->hGraph, HWND_DESKTOP, &pt, 1);
		    SetCursorPos(pt.x, pt.y);
		    break;
	    }
		case -1: /* start zooming; zooming cursor */
		    zoombox.on = TRUE;
		    zoombox.from.x = zoombox.to.x = x;
		    zoombox.from.y = zoombox.to.y = y;
		    break;
		case 0: // standard cross-hair cursor 
		    SetCursor((lpgw->hptrCurrent = mouse_setting.on ? lpgw->hptrCrossHair : lpgw->hptrDefault));
		    // Once done with rotation we have to redraw with aa once more. (GDI+ only) 
		    if(lpgw->gdiplus && lpgw->rotating && lpgw->fastrotation) {
			    lpgw->rotating = FALSE;
			    if(lpgw->antialiasing)
				    GraphRedraw(lpgw);
		    }
		    else {
			    lpgw->rotating = FALSE;
		    }
		    break;
		case 1: // cursor during rotation 
		    SetCursor(lpgw->hptrCurrent = lpgw->hptrRotating);
		    lpgw->rotating = TRUE;
		    break;
		case 2: SetCursor(lpgw->hptrCurrent = lpgw->hptrScaling); break; // cursor during scaling 
		case 3: SetCursor(lpgw->hptrCurrent = lpgw->hptrZooming); break; // cursor during zooming 
	}
	if(c>=0 && zoombox.on) {  /* erase zoom box */
		DrawZoomBox(lpgw);
		zoombox.on = FALSE;
	}
	if(c>=0 && ruler_lineto.on) {  /* erase ruler line */
		DrawRulerLineTo(lpgw);
		ruler_lineto.on = FALSE;
	}
}

// set_ruler(int x, int y) term API: x<0 switches ruler off. 
void Graph_set_ruler(GW * lpgw, int x, int y)
{
	DrawRuler(lpgw); // remove previous drawing, if any 
	DrawRulerLineTo(lpgw);
	if(x < 0) {
		ruler.on = FALSE;
	}
	else {
		ruler.on = TRUE;
		ruler.x = x; ruler.y = y;
		DrawRuler(lpgw); // draw ruler at new positions 
		DrawRulerLineTo(lpgw);
	}
}

/* put_tmptext(int i, char c[]) term API
 *      i: 0..at statusline
 *	1, 2: at corners of zoom box with \r separating text
 */
void Graph_put_tmptext(GW * lpgw, int where, LPCSTR text)
{
	// Position of the annotation string (mouse movement) or zoom box
	// text or whatever temporary text added...
	switch(where) {
		case 0:
		    UpdateStatusLine(lpgw, text);
		    break;
		case 1:
		    DrawZoomBox(lpgw);
		    SAlloc::F((char *)zoombox.text1);
		    zoombox.text1 = sstrdup(text);
		    DrawZoomBox(lpgw);
		    break;
		case 2:
		    DrawZoomBox(lpgw);
		    SAlloc::F((char *)zoombox.text2);
		    zoombox.text2 = sstrdup(text);
		    DrawZoomBox(lpgw);
		    break;
		default:
		    ; /* should NEVER happen */
	}
}

void Graph_set_clipboard(GW * lpgw, LPCSTR s)
{
	// check: no string --> nothing to do 
	if(!isempty(s)) {
		HGLOBAL memHandle;
		LPSTR clipText;
		// Transport the string into a system-global storage area. In case
		// of (unlikely) allocation failures, fail silently 
		size_t length = strlen(s);
		if((memHandle = GlobalAlloc(GHND, length + 1)) == NULL)
			return;
		if((clipText = (LPSTR)GlobalLock(memHandle)) == NULL) {
			GlobalFree(memHandle);
			return;
		}
		strcpy(clipText, s);
		GlobalUnlock(memHandle);
		// Now post that memory area to the Clipboard 
		OpenClipboard(lpgw->hWndGraph);
		EmptyClipboard();
		SetClipboardData(CF_TEXT, memHandle);
		CloseClipboard();
	}
}

/* ================================================================= */

/* Secondly, support routines that implement direct mouse reactions. */

/* This routine gets the mouse/pointer position in the current window and
 * recalculates it to the viewport coordinates. */
static void GetMousePosViewport(GW * lpgw, int * mx, int * my)
{
	POINT pt;
	RECT rc;
	GetPlotRect(lpgw, &rc);
	/* HBB: has to be done this way. The simpler method by taking apart LPARM
	 * only works for mouse, but not for keypress events. */
	GetCursorPos(&pt);
	ScreenToClient(lpgw->hGraph, &pt);
	/* px=px(mx); mouse=>gnuplot driver coordinates */
	/* FIXME: classically, this would use MulDiv() call, and no floating point */
	/* BM: protect against zero window size - Vista does that when minimizing windows */
	*mx = *my = 0;
	if((rc.right - rc.left) != 0)
		*mx = (int)((pt.x - rc.left) * lpgw->MaxS.x / (rc.right - rc.left) + 0.5);
	if((rc.bottom -rc.top) != 0)
		*my = (int)((rc.bottom - pt.y) * lpgw->MaxS.y / (rc.bottom -rc.top) + 0.5);
}

/* HBB 20010218: Newly separated function: Draw text string in XOR mode.
 * That is surprisingly difficult using the Windows API: have to draw text
 * into a background bitmap, first, and then blit that onto the screen.
 *
 * x and y give the bottom-left corner of the bounding box into which the
 * text will be drawn */
static void Draw_XOR_Text(GW * lpgw, const char * text, size_t length, int x, int y)
{
	if(!isempty(text)) {
		HDC tempDC;
		int cx, cy;
		HDC hdc = GetDC(lpgw->hGraph);
		// Prepare background image buffer of the necessary size 
		Wnd_GetTextSize(hdc, text, length, &cx, &cy);
		HBITMAP bitmap = CreateCompatibleBitmap(hdc, cx, cy);
		// ... and a DeviceContext to access it by 
		tempDC = CreateCompatibleDC(hdc);
		DeleteObject(SelectObject(tempDC, bitmap));
		// FIXME: Do we need to handle encodings here?
		TextOutA(tempDC, 0, 0, text, length);
		/* Copy printed string to the screen window using
		   "target = target XOR (NOT source)" ROP, see "Ternary Raster Operations"
		   http://msdn.microsoft.com/en-us/library/dd145130%28VS.85%29.aspx
		 */
		BitBlt(hdc, x, y - cy, cx, cy, tempDC, 0, 0, 0x00990066);
		// Clean up behind ourselves 
		DeleteDC(tempDC);
		DeleteObject(bitmap);
		ReleaseDC(lpgw->hGraph, hdc);
	}
}

#endif
/* ================================== */
/*
 * Update the status line by the text; erase the previous text
 */
static void UpdateStatusLine(GW * lpgw, const char text[])
{
	if(lpgw) {
		//LPWSTR wtext = UnicodeText(text, encoding);
		if(!lpgw->bDocked) {
			if(lpgw->hStatusbar)
				SendMessageW(lpgw->hStatusbar, SB_SETTEXTW, (WPARAM)0, (LPARAM)SUcSwitch(text));
		}
		else {
			if(lpgw->lptw && lpgw->lptw->hStatusbar)
				SendMessageW(lpgw->lptw->hStatusbar, SB_SETTEXTW, (WPARAM)1, (LPARAM)SUcSwitch(text));
		}
		//SAlloc::F(wtext);
	}
}
/*
 * Update the toolbar to reflect the current the number of active plots
 */
static void UpdateToolbar(GW * lpgw)
{
	uint i;
	if(lpgw->hToolbar == NULL)
		return;
	::SendMessage(lpgw->hToolbar, TB_HIDEBUTTON, M_HIDEGRID, (LPARAM) !lpgw->hasgrid);
	if(!lpgw->hasgrid) {
		lpgw->hidegrid = FALSE;
		::SendMessage(lpgw->hToolbar, TB_CHECKBUTTON, M_HIDEGRID, (LPARAM)FALSE);
	}
	for(i = 0; i < MAX(MAXPLOTSHIDE, lpgw->maxhideplots); i++) {
		if(i < lpgw->numplots) {
			if(i < MAXPLOTSHIDE)
				::SendMessage(lpgw->hToolbar, TB_HIDEBUTTON, M_HIDEPLOT + i, (LPARAM)FALSE);
		}
		else {
			if(i < lpgw->maxhideplots)
				lpgw->hideplot[i] = FALSE;
			if(i < MAXPLOTSHIDE) {
				::SendMessage(lpgw->hToolbar, TB_HIDEBUTTON, M_HIDEPLOT + i, (LPARAM)TRUE);
				::SendMessage(lpgw->hToolbar, TB_CHECKBUTTON, M_HIDEPLOT + i, (LPARAM)FALSE);
			}
		}
	}
}
/*
 * Toggle active plots
 */
void GraphModifyPlots(GW * lpgw, uint ops, int plotno)
{
	bool changed = FALSE;
	for(uint i = 0; i < MIN(lpgw->numplots, lpgw->maxhideplots); i++) {
		if(plotno >= 0 && i != plotno)
			continue;
		switch(ops) {
			case MODPLOTS_INVERT_VISIBILITIES:
			    lpgw->hideplot[i] = !lpgw->hideplot[i];
			    changed = TRUE;
			    ::SendMessage(lpgw->hToolbar, TB_CHECKBUTTON, M_HIDEPLOT + i, (LPARAM)lpgw->hideplot[i]);
			    break;
			case MODPLOTS_SET_VISIBLE:
			    if(lpgw->hideplot[i] == TRUE) {
				    changed = TRUE;
				    ::SendMessage(lpgw->hToolbar, TB_CHECKBUTTON, M_HIDEPLOT + i, (LPARAM)FALSE);
			    }
			    lpgw->hideplot[i] = FALSE;
			    break;
			case MODPLOTS_SET_INVISIBLE:
			    if(lpgw->hideplot[i] == FALSE) {
				    changed = TRUE;
				    ::SendMessage(lpgw->hToolbar, TB_CHECKBUTTON, M_HIDEPLOT + i, (LPARAM)TRUE);
			    }
			    lpgw->hideplot[i] = TRUE;
			    break;
		}
	}
	if(changed) {
		RECT rect;
		lpgw->buffervalid = FALSE;
		GetClientRect(lpgw->hGraph, &rect);
		InvalidateRect(lpgw->hGraph, &rect, 1);
		UpdateToolbar(lpgw);
		UpdateWindow(lpgw->hGraph);
	}
}

#ifdef USE_MOUSE

/* Draw the ruler.
 */
static void DrawRuler(GW * lpgw)
{
	HDC hdc;
	int iOldRop;
	RECT rc;
	long rx, ry;
	if(!ruler.on || ruler.x < 0)
		return;
	hdc = GetDC(lpgw->hGraph);
	GetPlotRect(lpgw, &rc);
	rx = MulDiv(ruler.x, rc.right - rc.left, lpgw->MaxS.x);
	ry = rc.bottom - MulDiv(ruler.y, rc.bottom - rc.top, lpgw->MaxS.y);
	iOldRop = SetROP2(hdc, R2_NOT);
	MoveTo(hdc, rc.left, ry);
	LineTo(hdc, rc.right, ry);
	MoveTo(hdc, rx, rc.top);
	LineTo(hdc, rx, rc.bottom);
	SetROP2(hdc, iOldRop);
	ReleaseDC(lpgw->hGraph, hdc);
}

/* Draw the ruler line to cursor position.
 */
static void DrawRulerLineTo(GW * lpgw)
{
	HDC hdc;
	int iOldRop;
	RECT rc;
	long rx, ry, rlx, rly;
	if(!ruler.on || !ruler_lineto.on || ruler.x < 0 || ruler_lineto.x < 0)
		return;
	hdc = GetDC(lpgw->hGraph);
	GetPlotRect(lpgw, &rc);
	rx  = MulDiv(ruler.x, rc.right - rc.left, lpgw->MaxS.x);
	ry  = rc.bottom - MulDiv(ruler.y, rc.bottom - rc.top, lpgw->MaxS.y);
	rlx = MulDiv(ruler_lineto.x, rc.right - rc.left, lpgw->MaxS.x);
	rly = rc.bottom - MulDiv(ruler_lineto.y, rc.bottom - rc.top, lpgw->MaxS.y);
	iOldRop = SetROP2(hdc, R2_NOT);
	MoveTo(hdc, rx, ry);
	LineTo(hdc, rlx, rly);
	SetROP2(hdc, iOldRop);
	ReleaseDC(lpgw->hGraph, hdc);
}

/* Draw the zoom box.
 */
static void DrawZoomBox(GW * lpgw)
{
	if(zoombox.on) {
		RECT rc;
		HDC hdc = GetDC(lpgw->hGraph);
		GetPlotRect(lpgw, &rc);
		long fx = MulDiv(zoombox.from.x, rc.right - rc.left, lpgw->MaxS.x);
		long fy = rc.bottom - MulDiv(zoombox.from.y, rc.bottom - rc.top, lpgw->MaxS.y);
		long tx = MulDiv(zoombox.to.x, rc.right - rc.left, lpgw->MaxS.x);
		long ty = rc.bottom - MulDiv(zoombox.to.y, rc.bottom - rc.top, lpgw->MaxS.y);
		long text_y = MulDiv(lpgw->ChrS.y, rc.bottom - rc.top, lpgw->MaxS.y);
		int OldROP2 = SetROP2(hdc, R2_NOTXORPEN);
		HPEN OldPen = (HPEN)SelectObject(hdc, CreatePenIndirect((lpgw->color ? lpgw->colorpen : lpgw->monopen) + 1));
		Rectangle(hdc, fx, fy, tx, ty);
		DeleteObject(SelectObject(hdc, OldPen));
		SetROP2(hdc, OldROP2);
		ReleaseDC(lpgw->hGraph, hdc);
		if(zoombox.text1) {
			const char * separator = sstrchr(zoombox.text1, '\r');
			if(separator) {
				Draw_XOR_Text(lpgw, zoombox.text1, separator - zoombox.text1, fx, fy);
				Draw_XOR_Text(lpgw, separator + 1, strlen(separator + 1), fx, fy + text_y);
			}
			else
				Draw_XOR_Text(lpgw, zoombox.text1, strlen(zoombox.text1), fx, fy + lpgw->ChrS.y / 2);
		}
		if(zoombox.text2) {
			const char * separator = sstrchr(zoombox.text2, '\r');
			if(separator) {
				Draw_XOR_Text(lpgw, zoombox.text2, separator - zoombox.text2, tx, ty);
				Draw_XOR_Text(lpgw, separator + 1, strlen(separator + 1), tx, ty + text_y);
			}
			else
				Draw_XOR_Text(lpgw, zoombox.text2, strlen(zoombox.text2), tx, ty + lpgw->ChrS.y / 2);
		}
	}
}

static void DrawFocusIndicator(GW * lpgw)
{
	if(lpgw->bDocked) {
		RECT rect;
		GetPlotRect(lpgw, &rect);
		HDC hdc = GetDC(lpgw->hGraph);
		SelectObject(hdc, GetStockObject(DC_PEN));
		SelectObject(hdc, GetStockObject(NULL_BRUSH));
		SetDCPenColor(hdc, RGB(0, 0, 128));
		Rectangle(hdc, rect.left + 1, rect.top + 1, rect.right - 1, rect.bottom - 1);
		ReleaseDC(lpgw->hGraph, hdc);
	}
}
#endif /* USE_MOUSE */
