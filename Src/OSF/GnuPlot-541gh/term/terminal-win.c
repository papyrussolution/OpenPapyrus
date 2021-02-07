// Hello, Emacs, this is -*-C-*- 
// GNUPLOT - win.trm 
/*[
 * Copyright 1992 - 1993, 1998, 2004
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
#define TERM_TABLE_START(x) termentry x {
#define TERM_TABLE_END(x)   };
// } @experimental

#ifdef TERM_REGISTER
	register_term(windows)
#endif
// @experimental #ifdef TERM_PROTO
TERM_PUBLIC void WIN_options();
TERM_PUBLIC void WIN_init(termentry * pThis);
TERM_PUBLIC void WIN_reset();
TERM_PUBLIC void WIN_text();
TERM_PUBLIC void WIN_graphics();
TERM_PUBLIC void WIN_move(uint x, uint y);
TERM_PUBLIC void WIN_vector(uint x, uint y);
TERM_PUBLIC void WIN_linetype(int lt);
TERM_PUBLIC void WIN_dashtype(int type, t_dashtype * custom_dash_pattern);
TERM_PUBLIC void WIN_put_text(uint x, uint y, const char * str);
TERM_PUBLIC int WIN_justify_text(enum JUSTIFY mode);
TERM_PUBLIC int WIN_text_angle(int ang);
TERM_PUBLIC void WIN_point(uint x, uint y, int number);
TERM_PUBLIC void WIN_resume();
TERM_PUBLIC void WIN_set_pointsize(double);
TERM_PUBLIC void WIN_linewidth(double linewidth);
#ifdef USE_MOUSE
	TERM_PUBLIC void WIN_set_ruler(int, int);
	TERM_PUBLIC void WIN_set_cursor(int, int, int);
	TERM_PUBLIC void WIN_put_tmptext(int, const char str[]);
	TERM_PUBLIC void WIN_set_clipboard(const char[]);
	#ifdef WGP_CONSOLE
		TERM_PUBLIC int WIN_waitforinput(int);
	#endif
#endif
TERM_PUBLIC int WIN_make_palette(t_sm_palette * palette);
TERM_PUBLIC void WIN_set_color(t_colorspec *);
TERM_PUBLIC void WIN_filled_polygon(int points, gpiPoint * corners);
TERM_PUBLIC void WIN_boxfill(int, uint, uint, uint, uint);
TERM_PUBLIC int WIN_set_font(const char * font);
TERM_PUBLIC void WIN_enhanced_open(char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
TERM_PUBLIC void WIN_enhanced_flush();
TERM_PUBLIC void WIN_image(uint, uint, coordval *, gpiPoint *, t_imagecolor);
TERM_PUBLIC void WIN_layer(t_termlayer syncpoint);
TERM_PUBLIC void WIN_hypertext(int type, const char * text);
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
termentry * WIN_term = NULL;
TCHAR WIN_inifontname[MAXFONTNAME] = TEXT(WINFONT);
int WIN_inifontsize = WINFONTSIZE;
static path_points WIN_poly = {0, 0, NULL};

static void WIN_add_path_point(path_points * poly, int x, int y);
static bool WIN_docked = FALSE;  /* docked window option is "sticky" */

static void WIN_add_path_point(path_points * poly, int x, int y)
{
	// Enlarge size of array of polygon points 
	if(poly->n >= poly->max) {
		poly->max += 10;
		poly->point = (POINT*)gp_realloc(poly->point, poly->max * sizeof(POINT), "points");
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
			GraphOpSize(graphwin, W_polyline, poly->n, 0, (LPCSTR)poly->point, poly->n * sizeof(POINT));
		if(poly->n > 0) {
			/* Save last path point in case there's a vector command without preceding move. */
			poly->point[0].x = poly->point[poly->n - 1].x;
			poly->point[0].y = poly->point[poly->n - 1].y;
			/* Reset counter */
			poly->n = 0;
		}
	}
}

TERM_PUBLIC void WIN_options()
{
	char * s;
	bool set_font = FALSE, set_fontsize = FALSE;
	bool set_title = FALSE, set_close = FALSE;
	bool set_dashed = FALSE, set_color = FALSE;
	bool set_background = FALSE, set_fontscale = FALSE;
	bool set_linewidth = FALSE, set_size = FALSE;
	bool set_position = FALSE, set_number = FALSE;
	bool set_wsize = FALSE, set_rounded = FALSE;
	bool set_docked = FALSE;
#ifndef WGP_CONSOLE
	bool set_layout = FALSE;
#endif
	bool set_pointscale = FALSE;
	bool color, dashed, rounded;
	COLORREF background;
	double fontscale, linewidth;
	double pointscale;
	int win_x = 0;
	int win_y = 0;
	int win_width = 0;
	int win_height = 0;
	char * title;
	int fontsize;
	char fontname[MAXFONTNAME];
	int window_number;
	uint dock_cols, dock_rows;
	while(!GPO.Pgm.EndOfCommand()) {
		switch(GPO.Pgm.LookupTableForCurrentToken(&WIN_opts[0])) {
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
			    GPO.Pgm.Shift();
			    break;
			case WIN_COLOR:
			    GPO.Pgm.Shift();
			    color = TRUE;
			    set_color = TRUE;
			    break;
			case WIN_MONOCHROME:
			    GPO.Pgm.Shift();
			    color = FALSE;
			    set_color = TRUE;
			    break;
			case WIN_BACKGROUND: {
			    int color;
			    GPO.Pgm.Shift();
			    color = parse_color_name();
			    /* TODO: save original background color and color string,
			       add background color to status string
			     */
			    background  =
				RGB(((color >> 16) & 0xff), ((color >> 8) & 0xff), (color & 0xff));
			    set_background = TRUE;
			    break;
		    }
			case WIN_ENHANCED:
			    GPO.Pgm.Shift();
			    term->flags |= TERM_ENHANCED_TEXT;
			    break;
			case WIN_NOENHANCED:
			    GPO.Pgm.Shift();
			    term->flags &= ~TERM_ENHANCED_TEXT;
			    break;
			case WIN_FONTSCALE: {
			    GPO.Pgm.Shift();
			    fontscale = GPO.RealExpression();
			    if(fontscale <= 0) 
					fontscale = 1.0;
			    set_fontscale = TRUE;
			    break;
		    }
			case WIN_LINEWIDTH: {
			    GPO.Pgm.Shift();
			    linewidth = GPO.RealExpression();
			    if(linewidth <= 0) 
					linewidth = 1.0;
			    set_linewidth = TRUE;
			    break;
		    }
			case WIN_POINTSCALE:
			    GPO.Pgm.Shift();
			    pointscale = GPO.RealExpression();
			    if(pointscale <= 0.0) 
					pointscale = 1.0;
			    set_pointscale = TRUE;
			    break;
			case WIN_SOLID:
			    GPO.Pgm.Shift();
			    dashed = FALSE;
			    set_dashed = TRUE;
			    break;
			case WIN_DASHED:
			    GPO.Pgm.Shift();
			    dashed = TRUE;
			    set_dashed = TRUE;
			    break;
			case WIN_BUTT:
			    GPO.Pgm.Shift();
			    rounded = FALSE;
			    set_rounded = TRUE;
			    break;
			case WIN_ROUND:
			    GPO.Pgm.Shift();
			    rounded = TRUE;
			    set_rounded = TRUE;
			    break;
			case WIN_SIZE: {
			    double insize; /* shige 2019-02-27 for size 0 */
			    GPO.Pgm.Shift();
			    if(set_wsize)
				    GPO.IntErrorCurToken("conflicting size options");
			    if(GPO.Pgm.EndOfCommand())
				    GPO.IntErrorCurToken("size requires 'width,heigth'");
			    if((insize = GPO.RealExpression()) >= 1)
				    win_width = insize;
			    else
				    GPO.IntErrorCurToken("size is out of range");
			    if(!GPO.Pgm.EqualsCurShift(","))
				    GPO.IntErrorCurToken("size requires 'width,heigth'");
			    if((insize = GPO.RealExpression()) >= 1)
				    win_height = insize;
			    else
				    GPO.IntErrorCurToken("size is out of range");
			    set_size = TRUE;
			    break;
		    }
			case WIN_WSIZE: {
			    GPO.Pgm.Shift();
			    if(set_size)
				    GPO.IntErrorCurToken("conflicting size options");
			    if(GPO.Pgm.EndOfCommand())
				    GPO.IntErrorCurToken("windowsize requires 'width,heigth'");
			    win_width = GPO.RealExpression();
			    if(!GPO.Pgm.EqualsCurShift(","))
				    GPO.IntErrorCurToken("windowsize requires 'width,heigth'");
			    win_height = GPO.RealExpression();
			    if(win_width < 1 || win_height < 1)
				    GPO.IntErrorCurToken("windowsize canvas size is out of range");
			    set_wsize = TRUE;
			    break;
		    }
			case WIN_POSITION: {
			    GPO.Pgm.Shift();
			    if(GPO.Pgm.EndOfCommand())
				    GPO.IntErrorCurToken("position requires 'x,y'");
			    win_x = GPO.RealExpression();
			    if(!GPO.Pgm.EqualsCurShift(","))
				    GPO.IntErrorCurToken("position requires 'x,y'");
			    win_y = GPO.RealExpression();
			    if(win_x < 1 || win_y < 1)
				    GPO.IntErrorCurToken("position is out of range");
			    set_position = TRUE;
			    break;
		    }
			case WIN_GTITLE:
			    GPO.Pgm.Shift();
			    title = GPO.TryToGetString();
			    if(title == NULL)
				    GPO.IntErrorCurToken("expecting string argument");
			    if(strcmp(title, "") == 0)
				    title = NULL;
			    set_title = TRUE;
			    break;
			case WIN_CLOSE:
			    GPO.Pgm.Shift();
			    set_close = TRUE;
			    break;
			case WIN_FONT:
			    GPO.Pgm.Shift();
			    // Code copied from ps.trm and modified for windows terminal 
			    if((s = GPO.TryToGetString())) {
				    char * comma;
				    if(set_font)
					    GPO.IntErrorCurToken("extraneous argument in set terminal %s", term->name);
				    comma = strrchr(s, ',');
				    if(comma && (1 == sscanf(comma + 1, "%i", &fontsize))) {
					    set_fontsize = TRUE;
					    *comma = '\0';
				    }
				    if(*s) {
					    set_font = TRUE;
					    safe_strncpy(fontname, s, MAXFONTNAME);
					    SAlloc::F(s);
				    }
			    }
			    else {
				    if(set_fontsize)
					    GPO.IntErrorCurToken("extraneous argument in set terminal %s", term->name);
				    set_fontsize = TRUE;
				    fontsize = GPO.IntExpression();
			    }
			    break;
			case WIN_DOCKED:
			    GPO.Pgm.Shift();
			    if(!graphwin->bDocked && GraphHasWindow(graphwin))
				    GPO.IntErrorCurToken("Cannot change the mode of an open window.");
			    if(persist_cl) {
				    fprintf(stderr, "Warning: cannot use docked graphs in persist mode\n");
			    }
			    else {
				    set_docked = TRUE;
				    WIN_docked = TRUE;
			    }
			    break;
			case WIN_STANDALONE:
			    GPO.Pgm.Shift();
			    if(graphwin->bDocked && GraphHasWindow(graphwin))
				    GPO.IntErrorCurToken("Cannot change the mode of an open window.");
			    set_docked = TRUE;
			    WIN_docked = FALSE;
			    break;
			case WIN_LAYOUT:
			    GPO.Pgm.Shift();
			    dock_rows = GPO.IntExpression();
			    if(GPO.Pgm.EndOfCommand() || !GPO.Pgm.EqualsCur(","))
				    GPO.IntErrorCurToken("expecting ', <num_cols>'");
			    if(dock_rows == 0) {
				    dock_rows = 1;
				    fprintf(stderr, "Warning: layout requires at least one row.\n");
			    }
			    GPO.Pgm.Shift();
			    if(GPO.Pgm.EndOfCommand())
				    GPO.IntErrorCurToken("expecting <num_cols>");
			    dock_cols = GPO.IntExpression();
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
			    window_number = GPO.IntExpression();
			    set_number = TRUE;
			    break;
		}
	}
	// change window? 
	if(set_number) {
		char status[100];
		LPGW lpgw = listgraphs;
		while((lpgw->Id != window_number) && (lpgw->next))
			lpgw = lpgw->next;
		if(lpgw->Id != window_number) {
			/* create new window */
			lpgw->next = (GW *)calloc(1, sizeof(GW));
			lpgw = lpgw->next;
			lpgw->Id = window_number;
			lpgw->bDocked = WIN_docked; /* "sticky" default */
		}
#ifdef USE_MOUSE
		// set status line of previous graph window 
		sprintf(status, "(inactive, window number %i)", graphwin->Id);
		Graph_put_tmptext(graphwin, 0, status);
		// reset status text 
		Graph_put_tmptext(lpgw, 0, "");
#endif
		graphwin = lpgw;
	}
	// apply settings 
	GraphInitStruct(graphwin);
	if(set_color) {
		graphwin->color = color;
		// Note: We no longer set TERM_MONOCHROME here, since colors and grayscale conversion are fully handled by drawgraph() 
		if(!set_dashed)
			graphwin->dashed = !color;
	}
	if(set_dashed)
		graphwin->dashed = dashed;
	if(set_rounded)
		graphwin->rounded = rounded;
	if(set_background)
		graphwin->background = background;
	if(set_fontscale)
		graphwin->fontscale = fontscale;
	if(set_linewidth)
		graphwin->linewidth = linewidth;
	if(set_pointscale)
		graphwin->pointscale = pointscale;
	if(set_size || set_wsize) {
		graphwin->Size.x = win_width;
		graphwin->Size.y = win_height;
	}
#ifdef WGP_CONSOLE
	if(set_docked && WIN_docked)
		WIN_docked = FALSE; /* silently ignore docked option for console mode gnuplot */
#endif
	if(!WIN_docked) {
		if(set_docked)
			graphwin->bDocked = WIN_docked;
		if(set_size) {
			graphwin->Canvas.x = win_width;
			graphwin->Canvas.y = win_height;
		}
		if(set_wsize) {
			graphwin->Canvas.x = 0;
			graphwin->Canvas.y = 0;
		}
		if(set_position) {
			graphwin->Origin.x = win_x;
			graphwin->Origin.y = win_y;
		}
	}
	else {
		if(set_docked)
			graphwin->bDocked = WIN_docked;
	}
#ifndef WGP_CONSOLE
	if(set_layout) {
		textwin.nDockRows = dock_rows;
		textwin.nDockCols = dock_cols;
	}
#endif
	if(set_title) {
		SAlloc::F(graphwin->Title);
#ifdef UNICODE
		graphwin->Title = (title) ?  UnicodeText(title, encoding) : _tcsdup(WINGRAPHTITLE);
#else
		graphwin->Title = (title) ?  title : sstrdup(WINGRAPHTITLE);
#endif
		GraphChangeTitle(graphwin);
	}
	if(set_fontsize)
		graphwin->deffontsize = graphwin->fontsize = fontsize;
	if(set_font) {
#ifdef UNICODE
		LPWSTR wfontname = UnicodeText(fontname, encoding);
		wcscpy(graphwin->fontname, wfontname);
		wcscpy(graphwin->deffontname, wfontname);
		SAlloc::F(wfontname);
#else
		strcpy(graphwin->fontname, fontname);
		strcpy(graphwin->deffontname, fontname);
#endif
	}
	// font initialization 
	WIN_set_font(NULL);
	WIN_update_options();
	if(set_close) {
		win_close_terminal_window(graphwin);
		return;
	}
#ifndef WGP_CONSOLE
	// update text window 
	if(set_layout) {
		DockedUpdateLayout(&textwin);
	}
#endif
	/* update graph window */
	if((set_position || set_size || set_wsize) && GraphHasWindow(graphwin))
		GraphUpdateWindowPosSize(graphwin);
	if(GraphHasWindow(graphwin) && IsIconic(graphwin->hWndGraph))
		ShowWindow(graphwin->hWndGraph, SW_SHOWNORMAL);
	GraphRedraw(graphwin);
}

void WIN_update_options()
{
	bool set_font = FALSE, set_fontsize = FALSE;
	/* update term_options */
	sprintf(term_options, "%i %s %s %s %s %s", graphwin->Id, graphwin->color ? "color" : "monochrome",
	    graphwin->dashed ? "dashed" : "solid", graphwin->rounded ? "rounded" : "butt",
	    term->flags & TERM_ENHANCED_TEXT ? "enhanced" : "noenhanced", graphwin->bDocked ? "docked" : "standalone");
#ifndef WGP_CONSOLE
	if(graphwin->bDocked) {
		char buf[128];
		sprintf(buf, " layout %i,%i", textwin.nDockRows, textwin.nDockCols);
		strcat(term_options, buf);
	}
#endif
	set_fontsize = (graphwin->deffontsize != WIN_inifontsize);
	set_font = (_tcscmp(graphwin->deffontname, WIN_inifontname) != 0);
	if(set_font || set_fontsize) {
		char * fontstring = (char*)gp_alloc(_tcslen(graphwin->deffontname) + 24, "win font");
		if(!set_fontsize) {
			sprintf(fontstring, " font \"" TCHARFMT "\"", graphwin->deffontname);
		}
		else {
			sprintf(fontstring, " font \"" TCHARFMT ", %d\"", set_font ? graphwin->deffontname : TEXT(""), graphwin->deffontsize);
		}
		strcat(term_options, fontstring);
		SAlloc::F(fontstring);
	}
	if(graphwin->background != RGB(255, 255, 255))
		sprintf(&(term_options[strlen(term_options)]), " background \"#%0x%0x%0x\"", GetRValue(graphwin->background),
		    GetGValue(graphwin->background), GetBValue(graphwin->background));
	if(graphwin->fontscale != 1)
		sprintf(&(term_options[strlen(term_options)]), " fontscale %.1f", graphwin->fontscale);
	if(graphwin->linewidth != 1)
		sprintf(&(term_options[strlen(term_options)]), " linewidth %.1f", graphwin->linewidth);
	if(graphwin->pointscale != 1)
		sprintf(&(term_options[strlen(term_options)]), " pointscale %.1f", graphwin->pointscale);
	if(!graphwin->bDocked) {
		if(graphwin->Canvas.x != 0)
			sprintf(&(term_options[strlen(term_options)]), " size %li,%li", graphwin->Canvas.x, graphwin->Canvas.y);
		else if(graphwin->Size.x != CW_USEDEFAULT)
			sprintf(&(term_options[strlen(term_options)]), " wsize %li,%li", graphwin->Size.x, graphwin->Size.y);
	}
}

TERM_PUBLIC void WIN_init(termentry * pThis)
{
	if(!graphwin->hWndGraph) {
		graphwin->xmax = WIN_XMAX;
		graphwin->ymax = WIN_YMAX;
		graphwin->htic = WIN_HTIC;
		graphwin->vtic = WIN_VTIC;
		GraphInit(graphwin);
	}
	WIN_last_linetype = LT_NODRAW;  /* HBB 20000813: linetype caching */
	WIN_term = pThis;
}

TERM_PUBLIC void WIN_reset()
{
}

TERM_PUBLIC void WIN_text()
{
	WIN_flush_line(&WIN_poly);
	GraphEnd(graphwin);
}

TERM_PUBLIC void WIN_graphics()
{
	GraphStart(graphwin, pointsize);
	/* Fix up the text size if the user has resized the window. */
	term->ChrH = graphwin->hchar;
	term->ChrV = graphwin->vchar;
	term->TicH = graphwin->htic;
	term->TicV = graphwin->vtic;
	WIN_last_linetype = LT_NODRAW;          /* HBB 20000813: linetype caching */
	/* Save current text encoding */
	GraphOp(graphwin, W_text_encoding, encoding, 0, NULL);
}

TERM_PUBLIC void WIN_move(uint x, uint y)
{
	// terminate current path only if we move to a disconnected position 
	if((WIN_poly.n > 0) && ((WIN_poly.point[WIN_poly.n - 1].x != x) || (WIN_poly.point[WIN_poly.n - 1].y != y))) {
		WIN_flush_line(&WIN_poly);
	}
	WIN_add_path_point(&WIN_poly, x, y);
}

TERM_PUBLIC void WIN_vector(uint x, uint y)
{
	if((WIN_poly.n == 0) || (WIN_poly.point[WIN_poly.n - 1].x != x) || (WIN_poly.point[WIN_poly.n - 1].y != y)) {
		if(WIN_poly.n == 0) {
			/* vector command without preceding move: e.g. in "with line lc variable" */
			/* Coordinates were saved with last flush already. */
			WIN_poly.n++;
		}
		WIN_add_path_point(&WIN_poly, x, y);
	}
}

TERM_PUBLIC void WIN_linetype(int lt)
{
	if(lt != WIN_last_linetype) {
		WIN_flush_line(&WIN_poly);
		GraphOp(graphwin, W_line_type, lt, 0, NULL);
		WIN_last_linetype = lt;
	}
}

TERM_PUBLIC void WIN_dashtype(int dt, t_dashtype * custom_dash_pattern)
{
	WIN_flush_line(&WIN_poly);
	GraphOpSize(graphwin, W_dash_type, dt, 0, (char*)custom_dash_pattern, sizeof(t_dashtype));
}

TERM_PUBLIC void WIN_put_text(uint x, uint y, const char * str)
{
	WIN_flush_line(&WIN_poly);
	if(!isempty(str)) {
		// If no enhanced text processing is needed, we can use the plain  
		// vanilla put_text() routine instead of this fancy recursive one. 
		if(!(term->flags & TERM_ENHANCED_TEXT) || ignore_enhanced_text || (!strpbrk(str, "{}^_@&~") && !contains_unicode(str)))
			GraphOp(graphwin, W_put_text, x, y, str);
		else
			GraphOp(graphwin, W_enhanced_text, x, y, str);
	}
}

TERM_PUBLIC int WIN_justify_text(enum JUSTIFY mode)
{
	GraphOp(graphwin, W_justify, mode, 0, NULL);
	return (TRUE);
}

TERM_PUBLIC int WIN_text_angle(int ang)
{
	if(graphwin->rotate)
		GraphOp(graphwin, W_text_angle, ang, 0, NULL);
	return graphwin->rotate;
}

TERM_PUBLIC void WIN_point(uint x, uint y, int number)
{
	WIN_flush_line(&WIN_poly);
	/* draw point shapes later to save memory */
	/* HBB 20010411: secure against pointtype -1 or lower */
	if(number < -1)
		number = -1;            /* refuse nonsense values */
	if(number >= 0)
		number %= WIN_POINT_TYPES;
	number += 1;
	GraphOp(graphwin, W_dot + number, x, y, NULL);
}

TERM_PUBLIC void WIN_resume()
{
	GraphResume(graphwin);
}

TERM_PUBLIC void WIN_set_pointsize(double s)
{
	if(s < 0) s = 1;
	/* Pass the scale as a scaled-up integer. */
	GraphOp(graphwin, W_pointsize, (int)100 * s, 0, NULL);
}

TERM_PUBLIC void WIN_linewidth(double linewidth)
{
	// TODO: line width caching
	WIN_flush_line(&WIN_poly);
	WIN_last_linetype = LT_NODRAW;        /* invalidate cached linetype */
	GraphOp(graphwin, W_line_width, (int)100 * linewidth, 0, NULL);
}

#ifdef USE_MOUSE

/* Implemented by Petr Mikulik, February 2001 --- the best Windows solutions
 * come from OS/2 :-))
 */

TERM_PUBLIC void WIN_put_tmptext(int i, const char str[])
{
	Graph_put_tmptext(graphwin, i, str);
}

TERM_PUBLIC void WIN_set_ruler(int x, int y)
{
	Graph_set_ruler(graphwin, x, y);
}

TERM_PUBLIC void WIN_set_cursor(int c, int x, int y)
{
	Graph_set_cursor(graphwin, c, x, y);
}

TERM_PUBLIC void WIN_set_clipboard(const char s[])
{
	Graph_set_clipboard(graphwin, s);
}

#ifdef WGP_CONSOLE
	TERM_PUBLIC int WIN_waitforinput(int options)
	{
		// Not required: message handling already done elsewhere. 
		return (options == TERM_ONLY_CHECK_MOUSING) ? NUL : ConsoleGetch();
	}
#endif /* WGP_CONSOLE */
#endif /* USE_MOUSE */

/* Note: this used to be a verbatim copy of PM_image (pm.trm) with only minor changes */

TERM_PUBLIC void WIN_image(uint M, uint N, coordval * image, gpiPoint * corner, t_imagecolor color_mode)
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
	if((color_mode == IC_PALETTE) || (color_mode == IC_RGB)) {
		pad_bytes = (4 - (3 * M) % 4) % 4; /* scan lines start on ULONG boundaries */
		image_size = (3 * M + pad_bytes) * N;
	}
	else if(color_mode == IC_RGBA) {
		pad_bytes = 0;
		image_size = M * N * 4;
	}
	else {
		GPO.IntWarn(NO_CARET, "Unknown color mode in WIN_image");
		return;
	}
	rgb_image = (PBYTE)gp_alloc(image_size, "WIN RGB image");
	if(color_mode == IC_PALETTE) {
		rgb_image += N * (3 * M + pad_bytes);
		for(uint y = 0; y < N; y++) {
			rgb_image -= 3 * M + pad_bytes;
			for(uint x = 0; x < M; x++) {
				rgb255_color rgb255;
				GPO.Rgb255MaxColorsFromGray(*image++, &rgb255);
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
		GraphOp(graphwin, W_image, color_mode,  0, NULL);
		GraphOp(graphwin, W_image, corner[0].x, corner[0].y, NULL);
		GraphOp(graphwin, W_image, corner[1].x, corner[1].y, NULL);
		GraphOp(graphwin, W_image, corner[2].x, corner[2].y, NULL);
		GraphOp(graphwin, W_image, corner[3].x, corner[3].y, NULL);
		// GraphOp() cannot be used here since the image might contain char(0), so use  GraphOpSize() instead 
		GraphOpSize(graphwin, W_image, M, N, (LPCSTR)rgb_image, image_size);
	}
	SAlloc::F(rgb_image);
}

TERM_PUBLIC int WIN_make_palette(t_sm_palette * palette)
{
	/* Win can do continuous colors. However, we round them only to WIN_PAL_COLORS levels
	 * in order to pass an integer to GraphOp; it also reasonably limits
	 * the number of colors if "copy to clipboard" is used.
	 * EAM: Would it be better to use the approximate_palette() mechanism instead,
	 * like the x11 terminal?
	 */
	return WIN_PAL_COLORS;
}

TERM_PUBLIC void WIN_set_color(t_colorspec * colorspec)
{
	// TODO: color caching
	WIN_flush_line(&WIN_poly);
	switch(colorspec->type) {
		case TC_FRAC: {
		    // Immediately translate palette index to RGB colour 
		    rgb255_color rgb255;
		    GPO.Rgb255MaxColorsFromGray(colorspec->value, &rgb255);
		    GraphOp(graphwin, W_setcolor, (rgb255.g << 8) | rgb255.b, (rgb255.r), NULL);
		    break;
	    }
		case TC_RGB:
		    // highest byte of colorspec->lt contains alpha 
		    GraphOp(graphwin, W_setcolor, (colorspec->lt) & 0xffff, (colorspec->lt >> 16) & 0xffff, NULL);
		    break;
		case TC_LT:
		    GraphOp(graphwin, W_setcolor, colorspec->lt, 0, (LPCSTR)&WIN_set_color);
		    break;
		default:
		    break;
	}
	WIN_last_linetype = LT_NODRAW;
}

TERM_PUBLIC void WIN_filled_polygon(int points, gpiPoint * corners)
{
	int i;
	GraphOp(graphwin, W_fillstyle, corners->style, 0, NULL);
	// Eliminate duplicate polygon points. 
	if((corners[0].x == corners[points - 1].x) && (corners[0].y == corners[points - 1].y))
		points--;
	for(i = 0; i < points; i++)
		GraphOp(graphwin, W_filled_polygon_pt, corners[i].x, corners[i].y, NULL);
	GraphOp(graphwin, W_filled_polygon_draw, points, 0, NULL);
}

TERM_PUBLIC void WIN_boxfill(int style, uint xleft, uint ybottom, uint width, uint height)
{
	WIN_flush_line(&WIN_poly);
	// split into multiple commands to squeeze through all the necessary info 
	GraphOp(graphwin, W_fillstyle, style, 0, NULL);
	GraphOp(graphwin, W_move, xleft, ybottom, NULL);
	GraphOp(graphwin, W_boxfill, xleft + width, ybottom + height, NULL);
}

TERM_PUBLIC int WIN_set_font(const char * font)
{
	// Note: defer the determination of default font name and default font size until drawgraph() is executed. 
	if(isempty(font)) {
		GraphOp(graphwin, W_font, 0, 0, NULL); // select default font 
	}
	else {
		int fontsize;
		const char * size = strrchr(font, ',');
		if(size == NULL) {
			GraphOp(graphwin, W_font, 0, 0, font); // only font name given 
		}
		else if(size == font) {
			// only font size given 
			sscanf(size + 1, "%i", &fontsize);
			GraphOp(graphwin, W_font, fontsize, 0, NULL);
		}
		else {
			// full font information supplied 
			char fontname[MAXFONTNAME];
			memcpy(fontname, font, size - font);
			fontname[size-font] = '\0';
			sscanf(size + 1, "%i", &fontsize);
			GraphOp(graphwin, W_font, fontsize, 0, fontname);
		}
	}
	return TRUE;
}

/* BM: new callback functions for enhanced text
   These are only stubs that call functions in wgraph.c.
 */

TERM_PUBLIC void WIN_enhanced_open(char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
{
	GraphEnhancedOpen(fontname, fontsize, base, widthflag, showflag, overprint);
}

TERM_PUBLIC void WIN_enhanced_flush()
{
	GraphEnhancedFlush();
}

TERM_PUBLIC void WIN_layer(t_termlayer syncpoint)
{
	WIN_flush_line(&WIN_poly);
	/* ignore LAYER_RESET in multiplot mode */
	if(((syncpoint == TERM_LAYER_RESET) || (syncpoint == TERM_LAYER_RESET_PLOTNO)) && multiplot)
		return;
	GraphOp(graphwin, W_layer, syncpoint, 0, NULL);
}

TERM_PUBLIC void WIN_hypertext(int type, const char * text)
{
	WIN_flush_line(&WIN_poly);
	GraphOp(graphwin, W_hypertext, type, 0, text);
}

TERM_PUBLIC void WIN_boxed_text(uint x, uint y, int option)
{
	GraphOp(graphwin, W_boxedtext, option, 0, NULL);
	GraphOp(graphwin, W_boxedtext, x, y, NULL);
}

TERM_PUBLIC void WIN_modify_plots(uint operations, int plotno)
{
	GraphModifyPlots(graphwin, operations, plotno);
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
