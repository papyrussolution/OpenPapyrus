// GNUPLOT - win.trm 
// Copyright 1992 - 1993, 1998, 2004
//
/*
 *
 * AUTHORS
 *
 *   Gnuplot for Windows: Maurice Castro, Russell Lang
 *   Current maintainer: Bastian Maerkisch
 */
/* This file implements the terminal and printer display for gnuplot  */
/* under Microsoft Windows.                                           */
/*                                                                    */
/* The modifications to allow Gnuplot to run under Windows were made  */
/* by Maurice Castro (maurice@bruce.cs.monash.edu.au)                 */
/* and Russell Lang (rjl@monu1.cc.monash.edu.au)         19 Nov 1992  */
/*                                                                    */
// Edit this file with tabstop=4 (vi :se ts=4)                        */
//
// adapted to the new terminal layout by Stefan Bodewig (Dec. 1995)
//
#include <gnuplot.h>
#pragma hdrstop
#include "driver.h"
#include <win\wcommon.h>

// @experimental {
#define TERM_BODY
#define TERM_PUBLIC static
#define TERM_TABLE
#define TERM_TABLE_START(x) GpTermEntry x {
#define TERM_TABLE_END(x)   };
// } @experimental

#ifdef TERM_REGISTER
	register_term(windows)
#endif
// @experimental #ifdef TERM_PROTO
TERM_PUBLIC void WIN_options();
TERM_PUBLIC void WIN_init(GpTermEntry * pThis);
TERM_PUBLIC void WIN_reset(GpTermEntry * pThis);
TERM_PUBLIC void WIN_text(GpTermEntry * pThis);
TERM_PUBLIC void WIN_graphics(GpTermEntry * pThis);
TERM_PUBLIC void WIN_move(GpTermEntry * pThis, uint x, uint y);
TERM_PUBLIC void WIN_vector(GpTermEntry * pThis, uint x, uint y);
TERM_PUBLIC void WIN_linetype(GpTermEntry * pThis, int lt);
TERM_PUBLIC void WIN_dashtype(GpTermEntry * pThis, int type, t_dashtype * custom_dash_pattern);
TERM_PUBLIC void WIN_put_text(GpTermEntry * pThis, uint x, uint y, const char * str);
TERM_PUBLIC int  WIN_justify_text(GpTermEntry * pThis, enum JUSTIFY mode);
TERM_PUBLIC int  WIN_text_angle(GpTermEntry * pThis, int ang);
TERM_PUBLIC void WIN_point(GpTermEntry * pThis, uint x, uint y, int number);
TERM_PUBLIC void WIN_resume(GpTermEntry * pThis);
TERM_PUBLIC void WIN_set_pointsize(GpTermEntry * pThis, double);
TERM_PUBLIC void WIN_linewidth(GpTermEntry * pThis, double linewidth);
#ifdef USE_MOUSE
	TERM_PUBLIC void WIN_set_ruler(GpTermEntry * pThis, int, int);
	TERM_PUBLIC void WIN_set_cursor(GpTermEntry * pThis, int, int, int);
	TERM_PUBLIC void WIN_put_tmptext(GpTermEntry * pThis, int, const char str[]);
	TERM_PUBLIC void WIN_set_clipboard(GpTermEntry * pThis, const char[]);
	#ifdef WGP_CONSOLE
		TERM_PUBLIC int WIN_waitforinput(GpTermEntry * pThis, int);
	#endif
#endif
TERM_PUBLIC int WIN_make_palette(GpTermEntry * pThis, t_sm_palette * palette);
TERM_PUBLIC void WIN_set_color(GpTermEntry * pThis, const t_colorspec *);
TERM_PUBLIC void WIN_filled_polygon(GpTermEntry * pThis, int points, gpiPoint * corners);
TERM_PUBLIC void WIN_boxfill(GpTermEntry * pThis, int, uint, uint, uint, uint);
TERM_PUBLIC int WIN_set_font(GpTermEntry * pThis, const char * font);
TERM_PUBLIC void WIN_enhanced_open(GpTermEntry * pThis, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
TERM_PUBLIC void WIN_enhanced_flush(GpTermEntry * pThis);
TERM_PUBLIC void WIN_image(GpTermEntry * pThis, uint, uint, coordval *, const gpiPoint *, t_imagecolor);
TERM_PUBLIC void WIN_layer(GpTermEntry * pThis, t_termlayer syncpoint);
TERM_PUBLIC void WIN_hypertext(GpTermEntry * pThis, int type, const char * text);
TERM_PUBLIC void WIN_modify_plots(uint operations, int plotno);
// Initialization values - guess now, scale later 
#define WIN_XMAX (24000)
#define WIN_YMAX (18000)
#define WIN_HCHAR (WIN_XMAX/75)
#define WIN_VCHAR (WIN_YMAX/25)
#define WIN_HTIC (WIN_XMAX/160)
#define WIN_VTIC WIN_HTIC
// @experimental #endif /* TERM_PROTO */

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY

#include "win/winmain.h"

// Interface routines - create list of actions for Windows 

enum WIN_id { 
	WIN_DEFAULT, 
	WIN_MONOCHROME, 
	WIN_COLOR, 
	WIN_GTITLE, 
	WIN_ENHANCED, 
	WIN_NOENHANCED, 
	WIN_FONT, 
	WIN_SIZE, 
	WIN_WSIZE, 
	WIN_POSITION, 
	WIN_CLOSE, 
	WIN_BACKGROUND, 
	WIN_FONTSCALE, 
	WIN_LINEWIDTH, 
	WIN_POINTSCALE, 
	WIN_SOLID, 
	WIN_DASHED, 
	WIN_ROUND, 
	WIN_BUTT, 
	WIN_DOCKED, 
	WIN_STANDALONE, 
	WIN_LAYOUT, 
	WIN_OTHER 
};

static struct gen_table WIN_opts[] = {
	{ "d$efault", WIN_DEFAULT },
	{ "c$olor", WIN_COLOR },
	{ "c$olour", WIN_COLOR },
	{ "m$onochrome", WIN_MONOCHROME },
	{ "backg$round", WIN_BACKGROUND },
	{ "solid", WIN_SOLID },
	{ "dash$ed", WIN_DASHED },
	{ "round$ed", WIN_ROUND },
	{ "butt", WIN_BUTT },
	{ "enh$anced", WIN_ENHANCED },
	{ "noenh$anced", WIN_NOENHANCED },
	{ "font", WIN_FONT },
	{ "fonts$cale", WIN_FONTSCALE },
	{ "linewidth", WIN_LINEWIDTH },
	{ "lw", WIN_LINEWIDTH },
	{ "pointscale", WIN_POINTSCALE},
	{ "ps", WIN_POINTSCALE},
	{ "ti$tle", WIN_GTITLE },
	{ "siz$e", WIN_SIZE },
	{ "ws$ize", WIN_WSIZE },
	{ "pos$ition", WIN_POSITION },
	{ "cl$ose", WIN_CLOSE },
	{ "dock$ed", WIN_DOCKED },
	{ "standalone", WIN_STANDALONE },
	{ "lay$out", WIN_LAYOUT },
	{ NULL, WIN_OTHER }
};

typedef struct {
	uint n;
	uint max;
	POINT * point;
} path_points;

static int WIN_last_linetype = LT_NODRAW; /* HBB 20000813: linetype caching */
GpTermEntry * WIN_term = NULL;
TCHAR WIN_inifontname[MAXFONTNAME] = TEXT(WINFONT);
int WIN_inifontsize = WINFONTSIZE;
static path_points WIN_poly = {0, 0, NULL};

static void WIN_add_path_point(path_points * poly, int x, int y);
static bool WIN_docked = FALSE; /* docked window option is "sticky" */

static void WIN_add_path_point(path_points * poly, int x, int y)
{
	// Enlarge size of array of polygon points 
	if(poly->n >= poly->max) {
		poly->max += 10;
		poly->point = (POINT*)SAlloc::R(poly->point, poly->max * sizeof(POINT));
	}
	// Store point 
	poly->point[poly->n].x = x;
	poly->point[poly->n].y = y;
	poly->n++;
	FPRINTF((stderr, "new polygon/polyline point: %i %i\n", x, y));
}

static void FASTCALL WIN_flush_line(path_points * poly)
{
	if(poly) {
		if(poly->n > 1)
			_WinM.P_GraphWin->OpSize(W_polyline, poly->n, 0, (LPCSTR)poly->point, poly->n * sizeof(POINT));
		if(poly->n > 0) {
			// Save last path point in case there's a vector command without preceding move. 
			poly->point[0].x = poly->point[poly->n-1].x;
			poly->point[0].y = poly->point[poly->n-1].y;
			// Reset counter 
			poly->n = 0;
		}
	}
}

TERM_PUBLIC void WIN_options(GpTermEntry * pThis, GnuPlot * pGp)
{
	GpWinMainBlock & r_winm = _WinM;
	char * s;
	bool set_font = FALSE;
	bool set_fontsize = FALSE;
	bool set_title = FALSE;
	bool set_close = FALSE;
	bool set_dashed = FALSE;
	bool set_color = FALSE;
	bool set_background = FALSE;
	bool set_fontscale = FALSE;
	bool set_linewidth = FALSE;
	bool set_size = FALSE;
	bool set_position = FALSE;
	bool set_number = FALSE;
	bool set_wsize = FALSE;
	bool set_rounded = FALSE;
	bool set_docked = FALSE;
#ifndef WGP_CONSOLE
	bool set_layout = FALSE;
#endif
	bool set_pointscale = FALSE;
	bool color;
	bool dashed;
	bool rounded;
	COLORREF background;
	double fontscale;
	double linewidth;
	double pointscale;
	int  win_x = 0;
	int  win_y = 0;
	int  win_width = 0;
	int  win_height = 0;
	char * title;
	int  fontsize;
	char fontname[MAXFONTNAME];
	int  window_number;
	uint dock_cols;
	uint dock_rows;
	while(!pGp->Pgm.EndOfCommand()) {
		switch(pGp->Pgm.LookupTableForCurrentToken(&WIN_opts[0])) {
			case WIN_DEFAULT:
			    color = TRUE;
			    dashed = FALSE;
			    rounded = FALSE;
#ifdef UNICODE
			    WideCharToMultiByte(CP_ACP, 0, WIN_inifontname, -1, fontname, MAXFONTNAME, 0, 0);
#else
			    strcpy(fontname, WIN_inifontname);
#endif
			    fontsize = WIN_inifontsize;
			    fontscale = linewidth = pointscale = 1;
			    set_color = set_dashed = TRUE;
			    set_font = set_fontsize = TRUE;
			    set_fontscale = set_linewidth = set_pointscale = TRUE;
			    set_rounded = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case WIN_COLOR:
			    pGp->Pgm.Shift();
			    color = TRUE;
			    set_color = TRUE;
			    break;
			case WIN_MONOCHROME:
			    pGp->Pgm.Shift();
			    color = FALSE;
			    set_color = TRUE;
			    break;
			case WIN_BACKGROUND: 
				{
					int color;
					pGp->Pgm.Shift();
					color = pGp->ParseColorName();
					/* TODO: save original background color and color string,
					   add background color to status string
					 */
					background  =
					RGB(((color >> 16) & 0xff), ((color >> 8) & 0xff), (color & 0xff));
					set_background = TRUE;
				}
				break;
			case WIN_ENHANCED:
			    pGp->Pgm.Shift();
			    pThis->flags |= TERM_ENHANCED_TEXT;
			    break;
			case WIN_NOENHANCED:
			    pGp->Pgm.Shift();
			    pThis->flags &= ~TERM_ENHANCED_TEXT;
			    break;
			case WIN_FONTSCALE: 
				{
					pGp->Pgm.Shift();
					fontscale = pGp->RealExpression();
					if(fontscale <= 0) 
						fontscale = 1.0;
					set_fontscale = TRUE;
				}
				break;
			case WIN_LINEWIDTH: 
				{
					pGp->Pgm.Shift();
					linewidth = pGp->RealExpression();
					if(linewidth <= 0) 
						linewidth = 1.0;
					set_linewidth = TRUE;
				}
				break;
			case WIN_POINTSCALE:
			    pGp->Pgm.Shift();
			    pointscale = pGp->RealExpression();
			    if(pointscale <= 0.0) 
					pointscale = 1.0;
			    set_pointscale = TRUE;
			    break;
			case WIN_SOLID:
			    pGp->Pgm.Shift();
			    dashed = FALSE;
			    set_dashed = TRUE;
			    break;
			case WIN_DASHED:
			    pGp->Pgm.Shift();
			    dashed = TRUE;
			    set_dashed = TRUE;
			    break;
			case WIN_BUTT:
			    pGp->Pgm.Shift();
			    rounded = FALSE;
			    set_rounded = TRUE;
			    break;
			case WIN_ROUND:
			    pGp->Pgm.Shift();
			    rounded = TRUE;
			    set_rounded = TRUE;
			    break;
			case WIN_SIZE: {
			    double insize; /* shige 2019-02-27 for size 0 */
			    pGp->Pgm.Shift();
			    if(set_wsize)
				    pGp->IntErrorCurToken("conflicting size options");
			    if(pGp->Pgm.EndOfCommand())
				    pGp->IntErrorCurToken("size requires 'width,heigth'");
			    if((insize = pGp->RealExpression()) >= 1)
				    win_width = static_cast<int>(insize);
			    else
				    pGp->IntErrorCurToken("size is out of range");
			    if(!pGp->Pgm.EqualsCurShift(","))
				    pGp->IntErrorCurToken("size requires 'width,heigth'");
			    if((insize = pGp->RealExpression()) >= 1)
				    win_height = static_cast<int>(insize);
			    else
				    pGp->IntErrorCurToken("size is out of range");
			    set_size = TRUE;
			    break;
		    }
			case WIN_WSIZE: {
			    pGp->Pgm.Shift();
			    if(set_size)
				    pGp->IntErrorCurToken("conflicting size options");
			    if(pGp->Pgm.EndOfCommand())
				    pGp->IntErrorCurToken("windowsize requires 'width,heigth'");
			    win_width = static_cast<int>(pGp->RealExpression());
			    if(!pGp->Pgm.EqualsCurShift(","))
				    pGp->IntErrorCurToken("windowsize requires 'width,heigth'");
			    win_height = static_cast<int>(pGp->RealExpression());
			    if(win_width < 1 || win_height < 1)
				    pGp->IntErrorCurToken("windowsize canvas size is out of range");
			    set_wsize = TRUE;
			    break;
		    }
			case WIN_POSITION: {
			    pGp->Pgm.Shift();
			    if(pGp->Pgm.EndOfCommand())
				    pGp->IntErrorCurToken("position requires 'x,y'");
			    win_x = static_cast<int>(pGp->RealExpression());
			    if(!pGp->Pgm.EqualsCurShift(","))
				    pGp->IntErrorCurToken("position requires 'x,y'");
			    win_y = static_cast<int>(pGp->RealExpression());
			    if(win_x < 1 || win_y < 1)
				    pGp->IntErrorCurToken("position is out of range");
			    set_position = TRUE;
			    break;
		    }
			case WIN_GTITLE:
			    pGp->Pgm.Shift();
			    title = pGp->TryToGetString();
			    if(title == NULL)
				    pGp->IntErrorCurToken("expecting string argument");
			    if(sstreq(title, ""))
				    title = NULL;
			    set_title = TRUE;
			    break;
			case WIN_CLOSE:
			    pGp->Pgm.Shift();
			    set_close = TRUE;
			    break;
			case WIN_FONT:
			    pGp->Pgm.Shift();
			    // Code copied from ps.trm and modified for windows terminal 
			    if((s = pGp->TryToGetString())) {
				    char * comma;
				    if(set_font)
					    pGp->IntErrorCurToken("extraneous argument in set terminal %s", pThis->name);
				    comma = strrchr(s, ',');
				    if(comma && (1 == sscanf(comma + 1, "%i", &fontsize))) {
					    set_fontsize = TRUE;
					    *comma = '\0';
				    }
				    if(*s) {
					    set_font = TRUE;
					    strnzcpy(fontname, s, MAXFONTNAME);
					    SAlloc::F(s);
				    }
			    }
			    else {
				    if(set_fontsize)
					    pGp->IntErrorCurToken("extraneous argument in set terminal %s", pThis->name);
				    set_fontsize = TRUE;
				    fontsize = pGp->IntExpression();
			    }
			    break;
			case WIN_DOCKED:
			    pGp->Pgm.Shift();
			    if(!r_winm.P_GraphWin->bDocked && GraphHasWindow(r_winm.P_GraphWin))
				    pGp->IntErrorCurToken("Cannot change the mode of an open window.");
			    if(pGp->_Plt.persist_cl) {
				    fprintf(stderr, "Warning: cannot use docked graphs in persist mode\n");
			    }
			    else {
				    set_docked = TRUE;
				    WIN_docked = TRUE;
			    }
			    break;
			case WIN_STANDALONE:
			    pGp->Pgm.Shift();
			    if(r_winm.P_GraphWin->bDocked && GraphHasWindow(r_winm.P_GraphWin))
				    pGp->IntErrorCurToken("Cannot change the mode of an open window.");
			    set_docked = TRUE;
			    WIN_docked = FALSE;
			    break;
			case WIN_LAYOUT:
			    pGp->Pgm.Shift();
			    dock_rows = pGp->IntExpression();
			    if(pGp->Pgm.EndOfCommand() || !pGp->Pgm.EqualsCur(","))
				    pGp->IntErrorCurToken("expecting ', <num_cols>'");
			    if(dock_rows == 0) {
				    dock_rows = 1;
				    fprintf(stderr, "Warning: layout requires at least one row.\n");
			    }
			    pGp->Pgm.Shift();
			    if(pGp->Pgm.EndOfCommand())
				    pGp->IntErrorCurToken("expecting <num_cols>");
			    dock_cols = pGp->IntExpression();
			    if(dock_cols == 0) {
				    dock_cols = 1;
				    fprintf(stderr, "Warning: layout requires at least one column.\n");
			    }
#ifndef WGP_CONSOLE
			    set_layout = TRUE;
#endif
			    break;
			case WIN_OTHER:
			default:
			    window_number = pGp->IntExpression();
			    set_number = TRUE;
			    break;
		}
	}
	// change window? 
	if(set_number) {
		char status[100];
		GW * lpgw = r_winm.P_ListGraphs;
		while((lpgw->Id != window_number) && (lpgw->next))
			lpgw = lpgw->next;
		if(lpgw->Id != window_number) {
			// create new window 
			lpgw->next = (GW *)SAlloc::C(1, sizeof(GW));
			lpgw = lpgw->next;
			lpgw->Id = window_number;
			lpgw->bDocked = WIN_docked; /* "sticky" default */
		}
#ifdef USE_MOUSE
		// set status line of previous graph window 
		sprintf(status, "(inactive, window number %i)", r_winm.P_GraphWin->Id);
		Graph_put_tmptext(r_winm.P_GraphWin, 0, status);
		// reset status text 
		Graph_put_tmptext(lpgw, 0, "");
#endif
		r_winm.P_GraphWin = lpgw;
	}
	// apply settings 
	GraphInitStruct(r_winm.P_GraphWin);
	if(set_color) {
		r_winm.P_GraphWin->color = color;
		// Note: We no longer set TERM_MONOCHROME here, since colors and grayscale conversion are fully handled by drawgraph() 
		if(!set_dashed)
			r_winm.P_GraphWin->dashed = !color;
	}
	if(set_dashed)
		r_winm.P_GraphWin->dashed = dashed;
	if(set_rounded)
		r_winm.P_GraphWin->rounded = rounded;
	if(set_background)
		r_winm.P_GraphWin->background = background;
	if(set_fontscale)
		r_winm.P_GraphWin->fontscale = fontscale;
	if(set_linewidth)
		r_winm.P_GraphWin->linewidth = linewidth;
	if(set_pointscale)
		r_winm.P_GraphWin->pointscale = pointscale;
	if(set_size || set_wsize)
		r_winm.P_GraphWin->Size_.Set(win_width, win_height);
#ifdef WGP_CONSOLE
	if(set_docked && WIN_docked)
		WIN_docked = FALSE; /* silently ignore docked option for console mode gnuplot */
#endif
	if(!WIN_docked) {
		if(set_docked)
			r_winm.P_GraphWin->bDocked = WIN_docked;
		if(set_size)
			r_winm.P_GraphWin->Canvas_.Set(win_width, win_height);
		if(set_wsize)
			r_winm.P_GraphWin->Canvas_.Z();
		if(set_position)
			r_winm.P_GraphWin->Origin_.Set(win_x, win_y);
	}
	else {
		if(set_docked)
			r_winm.P_GraphWin->bDocked = WIN_docked;
	}
#ifndef WGP_CONSOLE
	if(set_layout) {
		r_winm.TxtWin.nDockRows = dock_rows;
		r_winm.TxtWin.nDockCols = dock_cols;
	}
#endif
	if(set_title) {
		SAlloc::F(r_winm.P_GraphWin->Title);
		r_winm.P_GraphWin->Title = title ? sstrdup(SUcSwitch(title)) : sstrdup(WINGRAPHTITLE);
//#ifdef UNICODE
		//r_winm.P_GraphWin->Title = (title) ?  UnicodeText(title, encoding) : _tcsdup(WINGRAPHTITLE);
//#else
		//r_winm.P_GraphWin->Title = (title) ?  title : sstrdup(WINGRAPHTITLE);
//#endif
		GraphChangeTitle(r_winm.P_GraphWin);
	}
	if(set_fontsize)
		r_winm.P_GraphWin->deffontsize = r_winm.P_GraphWin->fontsize = fontsize;
	if(set_font) {
		//LPWSTR wfontname = UnicodeText(fontname, encoding);
		const wchar_t * p_u_fontname = SUcSwitch(fontname);
		wcscpy(r_winm.P_GraphWin->fontname, p_u_fontname);
		wcscpy(r_winm.P_GraphWin->deffontname, p_u_fontname);
		//SAlloc::F(wfontname);
	}
	// font initialization 
	WIN_set_font(pThis, NULL);
	WIN_update_options();
	if(set_close) {
		win_close_terminal_window(r_winm.P_GraphWin);
		return;
	}
#ifndef WGP_CONSOLE
	// update text window 
	if(set_layout) {
		DockedUpdateLayout(&r_winm.TxtWin);
	}
#endif
	// update graph window 
	if((set_position || set_size || set_wsize) && GraphHasWindow(r_winm.P_GraphWin))
		GraphUpdateWindowPosSize(r_winm.P_GraphWin);
	if(GraphHasWindow(r_winm.P_GraphWin) && IsIconic(r_winm.P_GraphWin->hWndGraph))
		ShowWindow(r_winm.P_GraphWin->hWndGraph, SW_SHOWNORMAL);
	GraphRedraw(r_winm.P_GraphWin);
}

void WIN_update_options()
{
	bool set_font = FALSE;
	bool set_fontsize = FALSE;
	GW * p_gr_win = _WinM.P_GraphWin;
	// update term_options 
	slprintf(GPT._TermOptions, "%i %s %s %s %s %s", p_gr_win->Id, p_gr_win->color ? "color" : "monochrome",
	    p_gr_win->dashed ? "dashed" : "solid", p_gr_win->rounded ? "rounded" : "butt",
	    GPT.P_Term->flags & TERM_ENHANCED_TEXT ? "enhanced" : "noenhanced", p_gr_win->bDocked ? "docked" : "standalone");
#ifndef WGP_CONSOLE
	if(p_gr_win->bDocked) {
		char buf[128];
		sprintf(buf, " layout %i,%i", _WinM.TxtWin.nDockRows, _WinM.TxtWin.nDockCols);
		GPT._TermOptions.Cat(buf);
	}
#endif
	set_fontsize = (p_gr_win->deffontsize != WIN_inifontsize);
	set_font = (_tcscmp(p_gr_win->deffontname, WIN_inifontname) != 0);
	if(set_font || set_fontsize) {
		SString fnt_buf;
		if(!set_fontsize) {
			fnt_buf.Printf(" font \"" TCHARFMT "\"", p_gr_win->deffontname);
		}
		else {
			fnt_buf.Printf(" font \"" TCHARFMT ", %d\"", set_font ? p_gr_win->deffontname : TEXT(""), p_gr_win->deffontsize);
		}
		GPT._TermOptions.Cat(fnt_buf.cptr());
	}
	if(p_gr_win->background != RGB(255, 255, 255))
		slprintf_cat(GPT._TermOptions, " background \"#%0x%0x%0x\"", GetRValue(p_gr_win->background),
		    GetGValue(p_gr_win->background), GetBValue(p_gr_win->background));
	if(p_gr_win->fontscale != 1)
		slprintf_cat(GPT._TermOptions, " fontscale %.1f", p_gr_win->fontscale);
	if(p_gr_win->linewidth != 1)
		slprintf_cat(GPT._TermOptions, " linewidth %.1f", p_gr_win->linewidth);
	if(p_gr_win->pointscale != 1)
		slprintf_cat(GPT._TermOptions, " pointscale %.1f", p_gr_win->pointscale);
	if(!p_gr_win->bDocked) {
		if(p_gr_win->Canvas_.x)
			slprintf_cat(GPT._TermOptions, " size %li,%li", p_gr_win->Canvas_.x, p_gr_win->Canvas_.y);
		else if(p_gr_win->Size_.x != CW_USEDEFAULT)
			slprintf_cat(GPT._TermOptions, " wsize %li,%li", p_gr_win->Size_.x, p_gr_win->Size_.y);
	}
}

TERM_PUBLIC void WIN_init(GpTermEntry * pThis)
{
	if(!_WinM.P_GraphWin->hWndGraph) {
		_WinM.P_GraphWin->MaxS.Set(WIN_XMAX, WIN_YMAX);
		_WinM.P_GraphWin->TicS.Set(WIN_HTIC, WIN_VTIC);
		GraphInit(_WinM.P_GraphWin);
	}
	WIN_last_linetype = LT_NODRAW; // HBB 20000813: linetype caching 
	WIN_term = pThis;
}

TERM_PUBLIC void WIN_reset(GpTermEntry * pThis)
{
}

TERM_PUBLIC void WIN_text(GpTermEntry * pThis)
{
	WIN_flush_line(&WIN_poly);
	GraphEnd(_WinM.P_GraphWin);
}

TERM_PUBLIC void WIN_graphics(GpTermEntry * pThis)
{
	GraphStart(_WinM.P_GraphWin, pThis->P_Gp->Gg.PointSize);
	// Fix up the text size if the user has resized the window. 
	pThis->ChrH = _WinM.P_GraphWin->ChrS.x;
	pThis->ChrV = _WinM.P_GraphWin->ChrS.y;
	pThis->TicH = _WinM.P_GraphWin->TicS.x;
	pThis->TicV = _WinM.P_GraphWin->TicS.y;
	WIN_last_linetype = LT_NODRAW; // HBB 20000813: linetype caching 
	// Save current text encoding 
	_WinM.P_GraphWin->Op(W_text_encoding, GPT._Encoding, 0, NULL);
}

TERM_PUBLIC void WIN_move(GpTermEntry * pThis, uint x, uint y)
{
	// terminate current path only if we move to a disconnected position 
	if((WIN_poly.n > 0) && ((WIN_poly.point[WIN_poly.n-1].x != x) || (WIN_poly.point[WIN_poly.n-1].y != y))) {
		WIN_flush_line(&WIN_poly);
	}
	WIN_add_path_point(&WIN_poly, x, y);
}

TERM_PUBLIC void WIN_vector(GpTermEntry * pThis, uint x, uint y)
{
	if((WIN_poly.n == 0) || (WIN_poly.point[WIN_poly.n-1].x != x) || (WIN_poly.point[WIN_poly.n-1].y != y)) {
		if(WIN_poly.n == 0) {
			// vector command without preceding move: e.g. in "with line lc variable" 
			// Coordinates were saved with last flush already. 
			WIN_poly.n++;
		}
		WIN_add_path_point(&WIN_poly, x, y);
	}
}

TERM_PUBLIC void WIN_linetype(GpTermEntry * pThis, int lt)
{
	if(lt != WIN_last_linetype) {
		WIN_flush_line(&WIN_poly);
		_WinM.P_GraphWin->Op(W_line_type, lt, 0, NULL);
		WIN_last_linetype = lt;
	}
}

TERM_PUBLIC void WIN_dashtype(GpTermEntry * pThis, int dt, t_dashtype * custom_dash_pattern)
{
	WIN_flush_line(&WIN_poly);
	_WinM.P_GraphWin->OpSize(W_dash_type, dt, 0, (char *)custom_dash_pattern, sizeof(t_dashtype));
}

TERM_PUBLIC void WIN_put_text(GpTermEntry * pThis, uint x, uint y, const char * str)
{
	WIN_flush_line(&WIN_poly);
	if(!isempty(str)) {
		// If no enhanced text processing is needed, we can use the plain  
		// vanilla put_text() routine instead of this fancy recursive one. 
		if(!(pThis->flags & TERM_ENHANCED_TEXT) || pThis->P_Gp->Enht.Ignore || (!strpbrk(str, "{}^_@&~") && !contains_unicode(str)))
			_WinM.P_GraphWin->Op(W_put_text, x, y, str);
		else
			_WinM.P_GraphWin->Op(W_enhanced_text, x, y, str);
	}
}

TERM_PUBLIC int WIN_justify_text(GpTermEntry * pThis, enum JUSTIFY mode)
{
	_WinM.P_GraphWin->Op(W_justify, mode, 0, NULL);
	return TRUE;
}

TERM_PUBLIC int WIN_text_angle(GpTermEntry * pThis, int ang)
{
	if(_WinM.P_GraphWin->rotate)
		_WinM.P_GraphWin->Op(W_text_angle, ang, 0, NULL);
	return _WinM.P_GraphWin->rotate;
}

TERM_PUBLIC void WIN_point(GpTermEntry * pThis, uint x, uint y, int number)
{
	WIN_flush_line(&WIN_poly);
	/* draw point shapes later to save memory */
	/* HBB 20010411: secure against pointtype -1 or lower */
	if(number < -1)
		number = -1; /* refuse nonsense values */
	if(number >= 0)
		number %= WIN_POINT_TYPES;
	number += 1;
	_WinM.P_GraphWin->Op(W_dot + number, x, y, NULL);
}

TERM_PUBLIC void WIN_resume(GpTermEntry * pThis)
{
	GraphResume(_WinM.P_GraphWin);
}

TERM_PUBLIC void WIN_set_pointsize(GpTermEntry * pThis, double s)
{
	if(s < 0.0) 
		s = 1.0;
	// Pass the scale as a scaled-up integer. 
	_WinM.P_GraphWin->Op(W_pointsize, static_cast<UINT>(100 * s), 0, NULL);
}

TERM_PUBLIC void WIN_linewidth(GpTermEntry * pThis, double linewidth)
{
	// TODO: line width caching
	WIN_flush_line(&WIN_poly);
	WIN_last_linetype = LT_NODRAW; // invalidate cached linetype 
	_WinM.P_GraphWin->Op(W_line_width, static_cast<UINT>(100 * linewidth), 0, NULL);
}

#ifdef USE_MOUSE

/* Implemented by Petr Mikulik, February 2001 --- the best Windows solutions
 * come from OS/2 :-))
 */

TERM_PUBLIC void WIN_put_tmptext(GpTermEntry * pThis, int i, const char str[])
{
	Graph_put_tmptext(_WinM.P_GraphWin, i, str);
}

TERM_PUBLIC void WIN_set_ruler(GpTermEntry * pThis, int x, int y)
{
	Graph_set_ruler(_WinM.P_GraphWin, x, y);
}

TERM_PUBLIC void WIN_set_cursor(GpTermEntry * pThis, int c, int x, int y)
{
	Graph_set_cursor(_WinM.P_GraphWin, c, x, y);
}

TERM_PUBLIC void WIN_set_clipboard(GpTermEntry * pThis, const char s[])
{
	Graph_set_clipboard(_WinM.P_GraphWin, s);
}

#ifdef WGP_CONSOLE
	TERM_PUBLIC int WIN_waitforinput(GpTermEntry * pThis, int options)
	{
		// Not required: message handling already done elsewhere. 
		return (options == TERM_ONLY_CHECK_MOUSING) ? '\0' : ConsoleGetch();
	}
#endif /* WGP_CONSOLE */
#endif /* USE_MOUSE */
//
// Note: this used to be a verbatim copy of PM_image (pm.trm) with only minor changes
//
TERM_PUBLIC void WIN_image(GpTermEntry * pThis, uint M, uint N, coordval * image, const gpiPoint * corner, t_imagecolor color_mode)
{
	PBYTE rgb_image;
	uint image_size;
	uint pad_bytes;
	WIN_flush_line(&WIN_poly);
	/* BM: IC_PALETTE, IC_RGB and IC_RGBA images are converted to a format
	   suitable for Windows:
	    - sequence of lines is reversed
	    - each line starts at a 4 byte boundary
	    - 24bits RGB  for IC_PALETTE and IC_RGB
	    - 32bits RGBA for IC_RGBA
	 */
	if(oneof2(color_mode, IC_PALETTE, IC_RGB)) {
		pad_bytes = (4 - (3 * M) % 4) % 4; /* scan lines start on ULONG boundaries */
		image_size = (3 * M + pad_bytes) * N;
	}
	else if(color_mode == IC_RGBA) {
		pad_bytes = 0;
		image_size = M * N * 4;
	}
	else {
		pThis->P_Gp->IntWarn(NO_CARET, "Unknown color mode in WIN_image");
		return;
	}
	rgb_image = (PBYTE)SAlloc::M(image_size);
	if(color_mode == IC_PALETTE) {
		rgb_image += N * (3 * M + pad_bytes);
		for(uint y = 0; y < N; y++) {
			rgb_image -= 3 * M + pad_bytes;
			for(uint x = 0; x < M; x++) {
				rgb255_color rgb255;
				pThis->P_Gp->Rgb255MaxColorsFromGray(*image++, &rgb255);
				*(rgb_image++) = rgb255.b;
				*(rgb_image++) = rgb255.g;
				*(rgb_image++) = rgb255.r;
			}
			rgb_image -= 3 * M;
		}
	}
	else if(color_mode == IC_RGB) {
		rgb_image += N * (3 * M + pad_bytes);
		for(uint y = 0; y<N; y++) {
			rgb_image -= 3 * M + pad_bytes;
			for(uint x = 0; x<M; x++) {
				rgb255_color rgb255;
				rgb255.r = (BYTE)(*image++ *255 + 0.5);
				rgb255.g = (BYTE)(*image++ *255 + 0.5);
				rgb255.b = (BYTE)(*image++ *255 + 0.5);
				*(rgb_image++) = rgb255.b;
				*(rgb_image++) = rgb255.g;
				*(rgb_image++) = rgb255.r;
			}
			rgb_image -= 3 * M;
		}
	}
	else if(color_mode == IC_RGBA) {
		rgb_image += M * N * 4;
		for(uint y = 0; y<N; y++) {
			rgb_image -= 4 * M;
			for(uint x = 0; x<M; x++) {
				coordval red, green, blue, alpha;
				red   = *image++; /* RGB is [0:1] */
				green = *image++;
				blue  = *image++;
				alpha = *image++; /* BUT alpha is [0:255] */
				*(rgb_image++) = (BYTE)(blue  * alpha);
				*(rgb_image++) = (BYTE)(green * alpha);
				*(rgb_image++) = (BYTE)(red   * alpha);
				*(rgb_image++) = (BYTE)(alpha);
			}
			rgb_image -= 4 * M;
		}
	}
	// squeeze all the information into the buffer 
	if(oneof3(color_mode, IC_PALETTE, IC_RGB, IC_RGBA)) {
		_WinM.P_GraphWin->Op(W_image, color_mode,  0, NULL);
		_WinM.P_GraphWin->Op(W_image, corner[0].x, corner[0].y, NULL);
		_WinM.P_GraphWin->Op(W_image, corner[1].x, corner[1].y, NULL);
		_WinM.P_GraphWin->Op(W_image, corner[2].x, corner[2].y, NULL);
		_WinM.P_GraphWin->Op(W_image, corner[3].x, corner[3].y, NULL);
		// GraphOp() cannot be used here since the image might contain char(0), so use  GraphOpSize() instead 
		_WinM.P_GraphWin->OpSize(W_image, M, N, (LPCSTR)rgb_image, image_size);
	}
	SAlloc::F(rgb_image);
}

TERM_PUBLIC int WIN_make_palette(GpTermEntry * pThis, t_sm_palette * palette)
{
	// Win can do continuous colors. However, we round them only to WIN_PAL_COLORS levels
	// in order to pass an integer to GraphOp; it also reasonably limits
	// the number of colors if "copy to clipboard" is used.
	// EAM: Would it be better to use the approximate_palette() mechanism instead,
	// like the x11 terminal?
	return WIN_PAL_COLORS;
}

TERM_PUBLIC void WIN_set_color(GpTermEntry * pThis, const t_colorspec * colorspec)
{
	// TODO: color caching
	WIN_flush_line(&WIN_poly);
	switch(colorspec->type) {
		case TC_FRAC: 
			{
				// Immediately translate palette index to RGB colour 
				rgb255_color rgb255;
				pThis->P_Gp->Rgb255MaxColorsFromGray(colorspec->value, &rgb255);
				_WinM.P_GraphWin->Op(W_setcolor, (rgb255.g << 8) | rgb255.b, (rgb255.r), NULL);
			}
			break;
		case TC_RGB:
		    // highest byte of colorspec->lt contains alpha 
		    _WinM.P_GraphWin->Op(W_setcolor, (colorspec->lt) & 0xffff, (colorspec->lt >> 16) & 0xffff, NULL);
		    break;
		case TC_LT:
		    _WinM.P_GraphWin->Op(W_setcolor, colorspec->lt, 0, (LPCSTR)&WIN_set_color);
		    break;
		default:
		    break;
	}
	WIN_last_linetype = LT_NODRAW;
}

TERM_PUBLIC void WIN_filled_polygon(GpTermEntry * pThis, int points, gpiPoint * corners)
{
	_WinM.P_GraphWin->Op(W_fillstyle, corners->style, 0, NULL);
	// Eliminate duplicate polygon points. 
	if((corners[0].x == corners[points-1].x) && (corners[0].y == corners[points-1].y))
		points--;
	for(int i = 0; i < points; i++)
		_WinM.P_GraphWin->Op(W_filled_polygon_pt, corners[i].x, corners[i].y, NULL);
	_WinM.P_GraphWin->Op(W_filled_polygon_draw, points, 0, NULL);
}

TERM_PUBLIC void WIN_boxfill(GpTermEntry * pThis, int style, uint xleft, uint ybottom, uint width, uint height)
{
	WIN_flush_line(&WIN_poly);
	// split into multiple commands to squeeze through all the necessary info 
	_WinM.P_GraphWin->Op(W_fillstyle, style, 0, NULL);
	_WinM.P_GraphWin->Op(W_move, xleft, ybottom, NULL);
	_WinM.P_GraphWin->Op(W_boxfill, xleft + width, ybottom + height, NULL);
}

TERM_PUBLIC int WIN_set_font(GpTermEntry * pThis, const char * font)
{
	// Note: defer the determination of default font name and default font size until drawgraph() is executed. 
	if(isempty(font)) {
		_WinM.P_GraphWin->Op(W_font, 0, 0, NULL); // select default font 
	}
	else {
		int fontsize;
		const char * size = strrchr(font, ',');
		if(!size) {
			_WinM.P_GraphWin->Op(W_font, 0, 0, font); // only font name given 
		}
		else if(size == font) {
			// only font size given 
			sscanf(size + 1, "%i", &fontsize);
			_WinM.P_GraphWin->Op(W_font, fontsize, 0, NULL);
		}
		else {
			// full font information supplied 
			char fontname[MAXFONTNAME];
			memcpy(fontname, font, size - font);
			fontname[size-font] = '\0';
			sscanf(size + 1, "%i", &fontsize);
			_WinM.P_GraphWin->Op(W_font, fontsize, 0, fontname);
		}
	}
	return TRUE;
}
//
// BM: new callback functions for enhanced text
// These are only stubs that call functions in wgraph.c.
//
TERM_PUBLIC void WIN_enhanced_open(GpTermEntry * pThis, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
{
	GraphEnhancedOpen(pThis, fontname, fontsize, base, widthflag, showflag, overprint);
}

TERM_PUBLIC void WIN_enhanced_flush(GpTermEntry * pThis)
{
	GraphEnhancedFlush();
}

TERM_PUBLIC void WIN_layer(GpTermEntry * pThis, t_termlayer syncpoint)
{
	WIN_flush_line(&WIN_poly);
	// ignore LAYER_RESET in multiplot mode 
	if(oneof2(syncpoint, TERM_LAYER_RESET, TERM_LAYER_RESET_PLOTNO) && GPT.Flags & GpTerminalBlock::fMultiplot)
		return;
	_WinM.P_GraphWin->Op(W_layer, syncpoint, 0, NULL);
}

TERM_PUBLIC void WIN_hypertext(GpTermEntry * pThis, int type, const char * text)
{
	WIN_flush_line(&WIN_poly);
	_WinM.P_GraphWin->Op(W_hypertext, type, 0, text);
}

TERM_PUBLIC void WIN_boxed_text(GpTermEntry * pThis, uint x, uint y, int option)
{
	_WinM.P_GraphWin->Op(W_boxedtext, option, 0, NULL);
	_WinM.P_GraphWin->Op(W_boxedtext, x, y, NULL);
}

TERM_PUBLIC void WIN_modify_plots(uint operations, int plotno)
{
	GraphModifyPlots(_WinM.P_GraphWin, operations, plotno);
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE
TERM_TABLE_START(win_driver)
	"windows", 
	"Microsoft Windows",
	WIN_XMAX, 
	WIN_YMAX, 
	WIN_VCHAR, 
	WIN_HCHAR,
	WIN_VTIC, 
	WIN_HTIC, 
	WIN_options, 
	WIN_init, 
	WIN_reset,
	WIN_text, 
	GnuPlot::NullScale, 
	WIN_graphics, 
	WIN_move, 
	WIN_vector,
	WIN_linetype, 
	WIN_put_text, 
	WIN_text_angle,
	WIN_justify_text, 
	WIN_point, 
	GnuPlot::DoArrow, 
	WIN_set_font,
	WIN_set_pointsize,
	TERM_CAN_MULTIPLOT|TERM_NO_OUTPUTFILE|TERM_ALPHA_CHANNEL|TERM_CAN_DASH|TERM_LINEWIDTH|TERM_FONTSCALE|TERM_POINTSCALE|TERM_ENHANCED_TEXT,
	WIN_text /* suspend */, 
	WIN_resume,
	WIN_boxfill, 
	WIN_linewidth,
	#ifdef USE_MOUSE
		#ifdef WGP_CONSOLE
			WIN_waitforinput,
		#else
			0 /* WIN_waitforinput */,
		#endif /* WGP_CONSOLE */
		WIN_put_tmptext, 
		WIN_set_ruler, 
		WIN_set_cursor, 
		WIN_set_clipboard,
	#endif
	WIN_make_palette, 
	0 /* previous_palette */,
	WIN_set_color, 
	WIN_filled_polygon,
	WIN_image,
	WIN_enhanced_open, 
	WIN_enhanced_flush, 
	do_enh_writec,
	WIN_layer,
	0,     /* no term->path */
	0.0,     /* Scale (unused) */
	WIN_hypertext,
	WIN_boxed_text,
	WIN_modify_plots, 
	WIN_dashtype 
TERM_TABLE_END(win_driver)

#undef LAST_TERM
#define LAST_TERM win_driver

#endif /* TERM_TABLE */
#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
START_HELP(windows)
"1 windows",
"?commands set terminal windows",
"?set terminal windows",
"?set term windows",
"?terminal windows",
"?term windows",
"?windows",
" The `windows` terminal is a fast interactive terminal driver that uses the",
" Windows GDI to draw and write text. The cross-platform `terminal wxt` and",
" `terminal qt` are also supported on Windows.",
"",
" Syntax:",
"       set terminal windows {<n>}",
"                            {color | monochrome}",
"                            {solid | dashed}",
"                            {rounded | butt}",
"                            {enhanced | noenhanced}",
"                            {font <fontspec>}",
"                            {fontscale <scale>}",
"                            {linewidth <scale>}",
"                            {pointscale <scale>}",
"                            {background <rgb color>}",
"                            {title \"Plot Window Title\"}",
"                            {{size | wsize} <width>,<height>}",
"                            {position <x>,<y>}",
"                            {docked {layout <rows>,<cols>} | standalone}",
"                            {close}",
"",
" Multiple plot windows are supported: `set terminal win <n>` directs the",
" output to plot window number n.",
"",
" `color` and `monochrome` select colored or mono output,",
" `dashed` and `solid` select dashed or solid lines. Note that `color`",
" defaults to `solid`, whereas `monochrome` defaults to `dashed`.",
" `rounded` sets line caps and line joins to be rounded; `butt` is the",
" default, butt caps and mitered joins.",
" `enhanced` enables enhanced text mode features (subscripts,",
" superscripts and mixed fonts, see `enhanced text` for more information).",
" `<fontspec>` is in the format \"<fontface>,<fontsize>\", where \"<fontface>\"",
" is the name of a valid Windows font, and <fontsize> is the size of the font",
" in points and both components are optional.",
" Note that in previous versions of gnuplot the `font` statement could be left",
" out and <fontsize> could be given as a number without double quotes. This is",
" no longer supported.",
" `linewidth`, `fontscale`, `pointscale` can be used to scale the width of",
" lines, the size of text, or the size of the point symbols.",
" `title` changes the title of the graph window.",
" `size` defines the width and height of the window's drawing area in pixels,",
" `wsize` defines the actual size of the window itself and `position` defines",
" the origin of the window i.e. the position of the top left corner on the",
" screen (again in pixel). These options override any default settings",
" from the `wgnuplot.ini` file.",
"",
" `docked` embeds the graph window in the wgnuplot text window and the `size`",
" and `position` options are ignored.  Note that `docked` is not available for",
" console-mode gnuplot.  Setting this option changes the default for new"
" windows.  The initial default is `standalone`.  The `layout` option allows to",
" reserve a minimal number of columns and rows for graphs in docked mode.  If",
" there are more graphs than fit the given layout, additional rows will be added.",
" Graphs are sorted by the numerical id, filling rows first.",
"",
" Other options may be changed using the `graph-menu` or the initialization file",
" `wgnuplot.ini`.",
"",
/* FIXME:  Move to persist section */
" The Windows version normally terminates immediately as soon as the end of",
" any files given as command line arguments is reached (i.e. in non-interactive",
" mode), unless you specify `-` as the last command line option.",
" It will also not show the text-window at all, in this mode, only the plot.",
" By giving the optional argument `-persist` (same as for gnuplot under x11;",
" former Windows-only options `/noend` or `-noend` are still accepted as well),",
" will not close gnuplot. Contrary to gnuplot on other operating systems,",
" gnuplot's interactive command line is accessible after the -persist option.",
"",
" The plot window remains open when the gnuplot terminal is changed with a",
" `set term` command. The plot window can be closed with `set term windows close`.",
"",
" `gnuplot` supports different methods to create printed output on Windows,",
" see `windows printing`. The windows terminal supports data exchange with ",
" other programs via clipboard and EMF files, see `graph-menu`. You can also",
" use the `terminal emf` to create EMF files.",
"2 graph-menu",
"?commands set terminal windows graph-menu",
"?set terminal windows graph-menu",
"?set term windows graph-menu",
"?windows graph-menu",
"?graph-menu",
" The `gnuplot graph` window has the following options on a pop-up menu",
" accessed by pressing the right mouse button(*) or selecting `Options` from the",
" system menu or the toolbar:",
"",
" `Copy to Clipboard` copies a bitmap and an enhanced metafile picture.",
"",
" `Save as EMF...` allows the user to save the current graph window as",
" enhanced metafile (EMF or EMF+).",
"",
" `Save as Bitmap...` allows the user to save a copy of the graph as bitmap",
" file.",
"",
" `Print...` prints the graphics windows using a Windows printer driver and",
" allows selection of the printer and scaling of the output."
" See also `windows printing`.",
"",
" `Bring to Top` when checked raises the graph window to the top after every",
" plot.",
"",
" `Color` when checked enables color output.  When unchecked it forces",
" all grayscale output.  This is e.g. useful to test appearance of monochrome",
" printouts.",
"",
#ifdef USE_WINGDI
" `GDI backend` draws to the screen using Windows GDI. This is the classical",
" windows terminal, which is fast, but lacks many features such as",
" anti-aliasing, oversampling and full transparency support. It is now",
" deprecated.",
#else
" The `GDI backend` which uses the classic GDI API is deprecated and has been",
" disabled in this version.",
#endif
"",
" `GDI+ backend` draws to the screen using the GDI+ Windows API. It supports",
" full antialiasing, oversampling, transparency and custom dash patterns.",
" This was the default in versions 5.0 and 5.2.",
"",
" `Direct2D backend` uses Direct2D & DirectWrite APIs to draw. It uses graphic",
" card acceleration and is hence typically much faster.  Since Direct2D can"
" not create EMF data, saving and copying to clipboard of EMF data fall back"
" to GDI+ while bitmap data is generated by D2d.",
" This is the recommended and default backend since version 5.3.",
"",
" `Oversampling` draws diagonal lines at fractional pixel positions to avoid",
" \"wobbling\" effects.  Vertical or horizontal lines are still snapped",
" to integer pixel positions to avoid blurry lines.",
"",
" `Antialiasing` enables smoothing of lines and edges. Note that this slows",
" down drawing.  `Antialiasing of polygons` is enabled by default but might",
" slow down drawing with the GDI+ backend.",
"",
" `Fast rotation` switches antialiasing temporarily off while rotating the",
" graph with the mouse. This speeds up drawing considerably at the expense",
" of an additional redraw after releasing the mouse button.",
"",
" `Background...` sets the window background color.",
"",
" `Choose Font...` selects the font used in the graphics window.",
"",
#ifdef WIN_CUSTOM_PENS
" `Line Styles...` allows customization of the line colors and styles.",
"",
#endif
" `Update wgnuplot.ini` saves the current window locations, window sizes, text",
" window font, text window font size, graph window font, graph window font",
" size, background color to the initialization file `wgnuplot.ini`.",
"",
"^<HR align=\"left\" width=\"100\">",
" (*) Note that this menu is only available by pressing the right mouse button",
" with `unset mouse`.",
"2 printing",
"?commands set terminal windows printing",
"?set terminal windows printing",
"?set term windows printing",
"?windows printing",
"?printing",
"?screendump",
" In order of preference, graphs may be printed in the following ways:",
"",
" `1.` Use the `gnuplot` command `set terminal` to select a printer and `set",
" output` to redirect output to a file.",
"",
" `2.` Select the `Print...` command from the `gnuplot graph` window.  An extra",
" command `screendump` does this from the text window.",
"",
" `3.` If `set output \"PRN\"` is used, output will go to a temporary file.  When",
" you exit from `gnuplot` or when you change the output with another `set",
" output` command, a dialog box will appear for you to select a printer port.",
" If you choose OK, the output will be printed on the selected port, passing",
" unmodified through the print manager.  It is possible to accidentally (or",
" deliberately) send printer output meant for one printer to an incompatible",
" printer.",
"",
"2 text-menu", /* FIXME: this is not really related to the windows driver, but the windows platform */
"?commands set terminal windows text-menu",
"?set terminal windows text-menu",
"?set term windows text-menu",
"?windows text-menu",
"?text-menu",
" The `gnuplot text` window has the following options on a pop-up menu accessed",
" by pressing the right mouse button or selecting `Options` from the system",
" menu:",
"",
" `Copy to Clipboard` copies marked text to the clipboard.",
"",
" `Paste` copies text from the clipboard as if typed by the user.",
"",
" `Choose Font...` selects the font used in the text window.",
"",
" `System Colors` when selected makes the text window honor the System Colors",
" set using the Control Panel.  When unselected, text is black or blue on a",
" white background.",
"",
" `Wrap long lines` when selected lines longer than the current window width",
" are wrapped.",
"",
" `Update wgnuplot.ini` saves the current settings to the initialisation file",
" `wgnuplot.ini`, which is located in the user's application data directory.",
"",
"2 wgnuplot.mnu", /* FIXME: this is not really related to the windows driver, but the windows platform */
"?windows wgnuplot.mnu",
"?wgnuplot.mnu",
" If the menu file `wgnuplot.mnu` is found in the same directory as",
" `gnuplot`, then the menu specified in `wgnuplot.mnu` will be loaded.",
" Menu commands:",
"",
"  [Menu]      starts a new menu with the name on the following line.",
"  [EndMenu]   ends the current menu.",
"  [--]        inserts a horizontal menu separator.",
"  [|]         inserts a vertical menu separator.",
"  [Button]    puts the next macro on a push button instead of a menu.",
"",
" Macros take two lines with the macro name (menu entry) on the first line and",
" the macro on the second line.  Leading spaces are ignored.  Macro commands:",
"",
"  [INPUT]     Input string with prompt terminated by [EOS] or {ENTER}",
"  [EOS]       End Of String terminator. Generates no output.",
"  [OPEN]      Get name of a file to open, with the title of the dialog",
"              terminated by [EOS], followed by a default filename terminated",
"              by [EOS] or {ENTER}.",
"  [SAVE]      Get name of a file to save.  Parameters like [OPEN]",
"  [DIRECTORY] Get name of a directory, with the title of the dialog",
"              terminated by [EOS] or {ENTER}",
"",
" Macro character substitutions:",
"",
"  {ENTER}     Carriage Return '\\r'",
"  {TAB}       Tab '\\011'",
"  {ESC}       Escape '\\033'",
"  {^A}        '\\001'",
"  ...",
"  {^_}        '\\031'",
"",
" Macros are limited to 256 characters after expansion.",
"",
"2 wgnuplot.ini",
"?commands set terminal windows wgnuplot.ini",
"?set terminal windows wgnuplot.ini",
"?set term windows wgnuplot.ini",
"?windows wgnuplot.ini",
"?wgnuplot.ini",
" The Windows text window and the `windows` terminal will read some of their options from",
" the `[WGNUPLOT]` section of `wgnuplot.ini`.",
" This file is located in the user's application data directory. Here's a sample",
" `wgnuplot.ini` file:",
"",
"       [WGNUPLOT]",
"       TextOrigin=0 0",
"       TextSize=640 150",
"       TextFont=Consolas,9",
"       TextWrap=1",
"       TextLines=400",
"       TextMaximized=0",
"       SysColors=0",
"       GraphOrigin=0 150",
"       GraphSize=640 330",
"       GraphFont=Tahoma,10",
"       GraphColor=1",
"       GraphToTop=1",
"       GraphGDI+=1",
"       GraphD2D=0",
"       GraphGDI+Oversampling=1",
"       GraphAntialiasing=1",
"       GraphPolygonAA=1",
"       GraphFastRotation=1",
"       GraphBackground=255 255 255",
"       DockVerticalTextFrac=350",
"       DockHorizontalTextFrac=400",
#ifdef WIN_CUSTOM_PENS
"       Border=0 0 0 0 0",
"       Axis=192 192 192 2 2",
"       Line1=0 0 255 0 0",
"       Line2=0 255 0 0 1",
"       Line3=255 0 0 0 2",
"       Line4=255 0 255 0 3",
"       Line5=0 0 128 0 4",
#endif
"",
"^ <h3>Text window options</h3> ",
"",
" These settings apply to the wgnuplot text-window only."
"",
" The `TextOrigin` and `TextSize` entries specify the location and size of the",
" text window. If `TextMaximized` is non-zero, the window will be maximized.",
"",
" The `TextFont` entry specifies the text window font and size.",
"",
" The `TextWrap` entry selects wrapping of long text lines.",
"",
" The `TextLines` entry specifies the number of (unwrapped) lines the internal",
" buffer of the text window can hold. This value currently cannot be changed",
" from within wgnuplot.",
"",
" See `text-menu`.",
"",
"^ <h3>Docked graph options</h3>",
"",
" `DockVerticalTextFrac` and `DockHorizontalTextFrac` set the fraction of the",
" window reserved for the text window in permille of the vertical or horizontal",
" layout.",
"",
"^ <h3>Graph window options</h3>",
"",
" The `GraphFont` entry specifies the font name and size in points.",
#ifdef WIN_CUSTOM_PENS
"",
" The five",
" numbers given in the `Border`, `Axis` and `Line` entries are the `Red`",
" intensity (0--255), `Green` intensity, `Blue` intensity, `Color Linestyle`",
" and `Mono Linestyle`.  `Linestyles` are 0=SOLID, 1=DASH, 2=DOT, 3=DASHDOT,",
" 4=DASHDOTDOT.  In the sample `wgnuplot.ini` file above, Line 2 is a green",
" solid line in color mode, or a dashed line in monochrome mode.  The default",
" line width is 1 pixel.  If `Linestyle` is negative, it specifies the width of",
" a SOLID line in pixels.  Line1 and any linestyle used with the `points` style",
" must be SOLID with unit width.",
#endif
"",
" See `graph-menu`."
END_HELP(windows)
#endif /* TERM_HELP */
