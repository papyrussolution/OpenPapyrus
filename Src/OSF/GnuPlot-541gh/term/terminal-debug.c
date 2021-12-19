// GNUPLOT - debug.trm 
// Copyright 1990 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
/*
 * This file is included by ../term.c.
 *
 * This terminal driver supports: DEBUG
 * AUTHORS: luecken@udel.edu
 * send your comments or suggestions to (luecken@udel.edu).
 */
/*
 * adapted to the new terminal layout by Stefan Bodewig (Dec. 1995)
 * generalised to have *all* defined capabilities by HBB (June 1997)
 */
#include <gnuplot.h>
#pragma hdrstop
#include "driver.h"

// @experimental {
#define TERM_BODY
#define TERM_PUBLIC static
#define TERM_TABLE
#define TERM_TABLE_START(x) GpTermEntry_Static x {
#define TERM_TABLE_END(x)   };
// } @experimental

#ifdef TERM_REGISTER
	register_term(debug)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void DEBUG_init(GpTermEntry_Static * pThis);
TERM_PUBLIC void DEBUG_graphics(GpTermEntry_Static * pThis);
TERM_PUBLIC void DEBUG_text(GpTermEntry_Static * pThis);
TERM_PUBLIC void DEBUG_linetype(GpTermEntry_Static * pThis, int linetype);
TERM_PUBLIC void DEBUG_move(GpTermEntry_Static * pThis, uint x, uint y);
TERM_PUBLIC void DEBUG_vector(GpTermEntry_Static * pThis, uint x, uint y);
TERM_PUBLIC void DEBUG_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str);
TERM_PUBLIC void DEBUG_reset(GpTermEntry_Static * pThis);
TERM_PUBLIC int  DEBUG_justify_text(GpTermEntry_Static * pThis, enum JUSTIFY mode);
TERM_PUBLIC int  DEBUG_text_angle(GpTermEntry_Static * pThis, int ang);
TERM_PUBLIC void DEBUG_point(GpTermEntry_Static * pThis, uint x, uint y, int pointstyle);
TERM_PUBLIC void DEBUG_arrow(GpTermEntry_Static * pThis, uint sx, uint sy, uint ex, uint ey, int head);
TERM_PUBLIC int  DEBUG_set_font(GpTermEntry_Static * pThis, const char * font);
TERM_PUBLIC void DEBUG_pointsize(GpTermEntry_Static * pThis, double pointsize);
TERM_PUBLIC void DEBUG_suspend(GpTermEntry_Static * pThis);
TERM_PUBLIC void DEBUG_resume(GpTermEntry_Static * pThis);
TERM_PUBLIC void DEBUG_fillbox(GpTermEntry_Static * pThis, int style, uint x1, uint y1, uint width, uint height);
TERM_PUBLIC void DEBUG_linewidth(GpTermEntry_Static * pThis, double linewidth);
TERM_PUBLIC void DEBUG_filled_polygon(GpTermEntry_Static * pThis, int, gpiPoint *);
TERM_PUBLIC void DEBUG_set_color(GpTermEntry_Static * pThis, const t_colorspec *);
TERM_PUBLIC void DEBUG_layer(GpTermEntry_Static * pThis, t_termlayer syncpoint);
TERM_PUBLIC void DEBUG_path(GpTermEntry_Static * pThis, int p);
TERM_PUBLIC void DEBUG_image(GpTermEntry_Static * pThis, uint m, uint n, coordval * image, const gpiPoint * corner, t_imagecolor color_mode);

#define DEBUG_XMAX 512
#define DEBUG_YMAX 390

#define DEBUG_XLAST (DEBUG_XMAX - 1)
#define DEBUG_YLAST (DEBUG_XMAX - 1)

// Assume a character size of 1, or a 7 x 10 grid. 
#define DEBUG_VCHAR     10
#define DEBUG_HCHAR     7
#define DEBUG_VTIC      (DEBUG_YMAX/70)
#define DEBUG_HTIC      (DEBUG_XMAX/75)
//#endif /* TERM_PROTO */

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY

int DEBUG_linetype_last;
int DEBUG_xlast;
int DEBUG_ylast;

TERM_PUBLIC void DEBUG_init(GpTermEntry_Static * pThis)
{
	fputs("init\n", GPT.P_GpOutFile);
	DEBUG_linetype_last = LT_NODRAW;
}

TERM_PUBLIC void DEBUG_graphics(GpTermEntry_Static * pThis)
{
	DEBUG_xlast = DEBUG_ylast = 0;
	fputs("graphics\n", GPT.P_GpOutFile);
}

TERM_PUBLIC void DEBUG_text(GpTermEntry_Static * pThis)
{
	fputs("text\n", GPT.P_GpOutFile);
}

TERM_PUBLIC void DEBUG_linetype(GpTermEntry_Static * pThis, int linetype)
{
	/*
	   if (linetype != DEBUG_linetype_last){
	   fprintf(GPT.P_GpOutFile,"l%d",linetype);
	   DEBUG_linetype_last = linetype;
	   }
	 */
	fprintf(GPT.P_GpOutFile, "line %d\n", linetype);
}

TERM_PUBLIC void DEBUG_move(GpTermEntry_Static * pThis, uint x, uint y)
{
	/*
	   if (x != DEBUG_xlast || y != DEBUG_ylast){
	   fprintf(GPT.P_GpOutFile,"mm");
	   DEBUG_xlast = x;
	   DEBUG_ylast = y;
	   }
	 */
	fprintf(GPT.P_GpOutFile, "move %d, %d\t(%d, %d)\n", x, y, x - DEBUG_xlast, y - DEBUG_ylast);
	DEBUG_xlast = x;
	DEBUG_ylast = y;
}

TERM_PUBLIC void DEBUG_vector(GpTermEntry_Static * pThis, uint x, uint y)
{
	/*
	   if (x != DEBUG_xlast || y != DEBUG_ylast){
	   fprintf(GPT.P_GpOutFile,"vv");
	   DEBUG_xlast = x;
	   DEBUG_ylast = y;
	   }
	 */
	fprintf(GPT.P_GpOutFile, "vect %d, %d\t(%d, %d)\n", x, y, x - DEBUG_xlast, y - DEBUG_ylast);
	DEBUG_xlast = x;
	DEBUG_ylast = y;
}

TERM_PUBLIC void DEBUG_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str)
{
	/*
	   DEBUG_move(x,y);
	   fprintf(GPT.P_GpOutFile,"tx%s\r",str);
	 */
	fputs("put_text calls:", GPT.P_GpOutFile);
	DEBUG_move(pThis, x, y);
	fprintf(GPT.P_GpOutFile, "put_text '%s'\n", str);
}

TERM_PUBLIC void DEBUG_reset(GpTermEntry_Static * pThis)
{
	fputs("reset", GPT.P_GpOutFile);
}

TERM_PUBLIC int DEBUG_justify_text(GpTermEntry_Static * pThis, enum JUSTIFY mode)
{
	fputs("justify ", GPT.P_GpOutFile);
	switch(mode) {
		case CENTRE: fputs("centre", GPT.P_GpOutFile); break;
		case RIGHT: fputs("right", GPT.P_GpOutFile); break;
		default: 
		case LEFT: fputs("left", GPT.P_GpOutFile); break;
	}
	fputs("\n", GPT.P_GpOutFile);
	return TRUE;
}

TERM_PUBLIC int DEBUG_text_angle(GpTermEntry_Static * pThis, int ang)
{
	fprintf(GPT.P_GpOutFile, "text_angle %d:", ang);
	switch(ang) {
		case 0: fputs(": horizontal\n", GPT.P_GpOutFile); break;
		case 1: fputs(": upwards\n", GPT.P_GpOutFile); break;
		default: fputs(": \a*undefined*\n", GPT.P_GpOutFile); break;
	}
	return TRUE;
}

TERM_PUBLIC void DEBUG_point(GpTermEntry_Static * pThis, uint x, uint y, int pointstyle)
{
	fprintf(GPT.P_GpOutFile, "point at (%ud,%ud), pointstyle %d\n", x, y, pointstyle);
}

TERM_PUBLIC void DEBUG_arrow(GpTermEntry_Static * pThis, uint sx, uint sy, uint ex, uint ey, int head)
{
	fprintf(GPT.P_GpOutFile, "arrow from (%ud,%ud) to (%ud,%ud), %s head\n", sx, sy, ex, ey, head ? "with" : "without");
}

TERM_PUBLIC int DEBUG_set_font(GpTermEntry_Static * pThis, const char * font)
{
	fprintf(GPT.P_GpOutFile, "set font to \"%s\"\n", font ? (*font ? font : "\aempty string!") : "\aNULL string!");
	return TRUE;
}

TERM_PUBLIC void DEBUG_pointsize(GpTermEntry_Static * pThis, double pointsize)
{
	fprintf(GPT.P_GpOutFile, "set pointsize to %lf\n", pointsize);
}

TERM_PUBLIC void DEBUG_suspend(GpTermEntry_Static * pThis)
{
	fputs("suspended terminal driver\n", GPT.P_GpOutFile);
}

TERM_PUBLIC void DEBUG_resume(GpTermEntry_Static * pThis)
{
	fputs("resumed terminal driver\n", GPT.P_GpOutFile);
}

TERM_PUBLIC void DEBUG_fillbox(GpTermEntry_Static * pThis, int style, uint x1, uint y1, uint width, uint height)
{
	fprintf(GPT.P_GpOutFile, "fillbox/clear at (%ud,%ud), area (%ud,%ud), style %d)\n", x1, y1, width, height, style);
}

TERM_PUBLIC void DEBUG_linewidth(GpTermEntry_Static * pThis, double linewidth)
{
	fprintf(GPT.P_GpOutFile, "set linewidth %lf\n", linewidth);
}

TERM_PUBLIC void DEBUG_filled_polygon(GpTermEntry_Static * pThis, int points, gpiPoint * corners)
{
	fprintf(GPT.P_GpOutFile, "polygon with %d vertices\n", points);
}

TERM_PUBLIC void DEBUG_set_color(GpTermEntry_Static * pThis, const t_colorspec * colorspec)
{
	//extern void save_pm3dcolor();
	fprintf(GPT.P_GpOutFile, "set_color:  ");
	save_pm3dcolor(GPT.P_GpOutFile, colorspec);
	fprintf(GPT.P_GpOutFile, "\n");
}

TERM_PUBLIC void DEBUG_layer(GpTermEntry_Static * pThis, t_termlayer syncpoint)
{
	char * l = "";
	switch(syncpoint) {
		case TERM_LAYER_RESET:              l = "reset"; break;
		case TERM_LAYER_BACKTEXT:           l = "backtext"; break;
		case TERM_LAYER_FRONTTEXT:          l = "fronttext"; break;
		case TERM_LAYER_BEGIN_GRID:         l = "begin grid"; break;
		case TERM_LAYER_END_GRID:           l = "end grid"; break;
		case TERM_LAYER_END_TEXT:           l = "end text"; break;
		case TERM_LAYER_BEFORE_PLOT:        l = "before plot"; break;
		case TERM_LAYER_AFTER_PLOT:         l = "after plot"; break;
		case TERM_LAYER_BEGIN_KEYSAMPLE:    l = "begin keysample"; break;
		case TERM_LAYER_END_KEYSAMPLE:      l = "end keysample"; break;
		case TERM_LAYER_RESET_PLOTNO:       l = "reset plotno"; break;
		case TERM_LAYER_BEFORE_ZOOM:        l = "before zoom"; break;
		case TERM_LAYER_BEGIN_PM3D_MAP:     l = "begin pm3d map"; break;
		case TERM_LAYER_END_PM3D_MAP:       l = "end pm3d map"; break;
		default:                            l = "unknown"; break;
	}
	fprintf(GPT.P_GpOutFile, "layer %s\n", l);
}

TERM_PUBLIC void DEBUG_path(GpTermEntry_Static * pThis, int p)
{
	fprintf(GPT.P_GpOutFile, "path %d\n", p);
}

TERM_PUBLIC void DEBUG_image(GpTermEntry_Static * pThis, uint m, uint n, coordval * image, const gpiPoint * corner, t_imagecolor color_mode)
{
	fprintf(GPT.P_GpOutFile, "image size = %d x %d\n", m, n);
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE

TERM_TABLE_START(debug_driver)
	"debug", 
	"debugging driver",
	DEBUG_XMAX, 
	DEBUG_YMAX, 
	DEBUG_VCHAR, 
	DEBUG_HCHAR,
	DEBUG_VTIC, 
	DEBUG_HTIC, 
	GnuPlot::OptionsNull, 
	DEBUG_init, 
	DEBUG_reset,
	DEBUG_text, 
	GnuPlot::NullScale, 
	DEBUG_graphics, 
	DEBUG_move, 
	DEBUG_vector,
	DEBUG_linetype, 
	DEBUG_put_text, 
	DEBUG_text_angle,
	DEBUG_justify_text, 
	DEBUG_point, 
	DEBUG_arrow, 
	DEBUG_set_font,
	DEBUG_pointsize,
	TERM_CAN_MULTIPLOT,
	DEBUG_suspend, 
	DEBUG_resume, 
	DEBUG_fillbox, 
	DEBUG_linewidth,
	#ifdef USE_MOUSE
		0, 
		0, 
		0, 
		0, 
		0,     /* no mouse support */
	#endif
	0, 
	0,     /* no palette */
	DEBUG_set_color,
	DEBUG_filled_polygon,
	DEBUG_image,
	0, 
	0, 
	0,     /* no enhanced text */
	DEBUG_layer, 
	DEBUG_path 
TERM_TABLE_END(debug_driver)

#undef LAST_TERM
#define LAST_TERM debug_driver

#endif /* TERM_TABLE */
#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
START_HELP(debug)
"1 debug",
"?commands set terminal debug",
"?set terminal debug",
"?set term debug",
"?terminal debug",
"?term debug",
"?debug",
" This terminal is provided to allow for the debugging of `gnuplot`.  It is",
" likely to be of use only for users who are modifying the source code."
END_HELP(debug)
#endif
