// GNUPLOT - dxf.trm 
// Copyright 1991 - 1993, 1998, 2004
/*
 * This file is included by ../term.c.
 * This terminal driver supports: AutoCad (Release 10.x) dxf file format (import with AutoCad dxfin command)
 * AUTHOR: Florian Hiss  (fhis1231@w204zrz.zrz.tu-berlin.de)
 * send your comments or suggestions to (gnuplot-info@lists.sourceforge.net).
 */
/*
 * adapted to the new terminal layout by Stefan Bodewig (Dec. 1995)
 */
#include <gnuplot.h>
#pragma hdrstop
#include "driver.h"

// @experimental {
#define TERM_BODY
#define TERM_PUBLIC static
#define TERM_TABLE
#define TERM_TABLE_START(x) GpTermEntry x {
#define TERM_TABLE_END(x)   };
// } @experimental

#ifdef TERM_REGISTER
	register_term(dxf)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void DXF_init(GpTermEntry * pThis);
TERM_PUBLIC void DXF_graphics(GpTermEntry * pThis);
TERM_PUBLIC void DXF_text(GpTermEntry * pThis);
TERM_PUBLIC void DXF_linetype(GpTermEntry * pThis, int linetype);
TERM_PUBLIC void DXF_move(GpTermEntry * pThis, uint x, uint y);
TERM_PUBLIC void DXF_vector(GpTermEntry * pThis, uint ux, uint uy);
TERM_PUBLIC void DXF_put_text(GpTermEntry * pThis, uint x, uint y, const char str[]);
TERM_PUBLIC int  DXF_text_angle(GpTermEntry * pThis, int ang);
TERM_PUBLIC int  DXF_justify_text(GpTermEntry * pThis, enum JUSTIFY mode);
TERM_PUBLIC void DXF_reset(GpTermEntry * pThis);

#define DXF_XMAX (120.0 * DXF_UNIT)
#define DXF_YMAX (80.0 * DXF_UNIT)
#if 0 // HBB 20030626: old version 
	#define DXF_HTIC (0.01 * DXF_XMAX)      /* 1.0 percent */
	#define DXF_VTIC (0.01 * DXF_YMAX)      /* 1.0 percent */
#else
	// HBB 20030626: make them have the same length in DXF_UNITs ! 
	#define DXF_HTIC (2.0 * DXF_UNIT)
	#define DXF_VTIC (2.0 * DXF_UNIT)
#endif
#define DXF_HCHAR (0.014 * DXF_XMAX)    /* 1.4 percent */
#define DXF_VCHAR (0.026 * DXF_YMAX)    /* 2.6 percent */
//#endif /* TERM_PROTO */

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY

#define DXF_UNIT 60.0
#define LINEWIDTH 0.0351        /* default line width is 1 pt */

/* 120 (autocad units) wide by 80 (autocad units) high (default)
 * use the GNUPLOT 'set size' command to change the defaults */
/* actual text height */
#define DXF_TEXTHEIGHT (0.7 * DXF_VCHAR)
/* actual text width, only a guess, we don't know the width of
 * a character of given height of the AutoCad STANDARD text font,
 * so change it if you like */
#define DXF_TEXTWIDTH (0.7 * DXF_HCHAR)
#define DXF_LINE_TYPES 7 /* number of line types we support. see below  */
#define MAX_LAYER 7 /* number of layers used for the drawing. see below */
#define LT_SCALE 1 /* line type scaling */

static uint DXF_posx;
static uint DXF_posy;
/* linetype is mapped to a layer. see below. */
static uint dxf_linetype;
static enum JUSTIFY dxf_justify = LEFT;
static float dxf_angle = 0.0;   /* 0 is horizontal, 90.0 is vertical */
static const char * text_style = "STANDARD"; /* text style used in the entire drawing */

#define TEXT_LAYER 0 /* text always resides on layer 0 */

/* each linetype resides on its own layer. each layer has its own color.
 * this avoids difficulties that AutoCad has with proper scaling of
 * the linetypes.
 * change the colors according to your needs */
static const char * layer_name[] = { "0", "1", "2", "3", "4", "5", "6" };

/* the colours are white, red, yellow, green, cyan, blue, magenta.
 * change them according to your needs.
 * when using a black and white plotting device the colours map to different
 * line thicknesses. see description of AutoCad print / plot command */
static const char * layer_colour[] = { "7", "1", "2", "3", "4", "5", "6" };

/* support line types AutoCad has to offer by default. */
static const char * layer_lines[] = { "CONTINUOUS", "DASHED", "HIDDEN", "CENTER", "PHANTOM", "DOT", "DASHDOT" };

static bool vector_was_last = FALSE;

TERM_PUBLIC void DXF_init(GpTermEntry * pThis)
{
	DXF_posx = DXF_posy = 0;
	dxf_linetype = 0;
	dxf_angle = 0.0;
	vector_was_last = FALSE;
}

TERM_PUBLIC void DXF_graphics(GpTermEntry * pThis)
{
	int i;
	static char dxfi1[] =
	    "\
999\n\
%% GNUPLOT: dxf file for AutoCad\n\
  0\nSECTION\n  2\nHEADER\n\
  9\n$EXTMIN\n\
 10\n0.000\n 20\n0.000\n\
  9\n$EXTMAX\n\
 10\n%-6.3f\n 20\n%-6.3f\n\
  9\n$LIMMIN\n\
 10\n0.000\n 20\n0.000\n\
  9\n$LIMMAX\n\
 10\n%-6.3f\n 20\n%-6.3f\n\
  9\n$TEXTSTYLE\n  7\n%s\n\
  9\n$TEXTSIZE\n 40\n%-6.3f\n\
  9\n$PLINEWID\n 40\n%-6.4f\n\
  9\n$LTSCALE\n  40\n%-6.3f\n\
  9\n$COORDS\n 70\n  1\n\
  9\n$CELTYPE\n 6\nBYLAYER\n\
  9\n$CLAYER\n  8\n0\n\
  9\n$CECOLOR\n 62\n   %s\n\
  9\n$MENU\n  1\nacad\n\
  0\nENDSEC\n\
  0\nSECTION\n  2\nTABLES\n";
	static char dxfi2[] =
	    "\
0\nTABLE\n  2\nLTYPE\n 70\n    %d\n\
0\nLTYPE\n  2\nCONTINUOUS\n 70\n    64\n\
  3\nSolid line\n 72\n    65\n 73\n      0\n 40\n0.0\n\
  0\nLTYPE\n  2\nDASHED\n 70\n    64\n\
  3\n__ __ __ __ __ __ __ __ __ __ __ __ __ __ __\n\
 72\n    65\n 73\n     2\n 40\n0.75\n 49\n0.5\n 49\n-0.25\n\
  0\nLTYPE\n  2\nHIDDEN\n 70\n    64\n\
  3\n_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n\
 72\n    65\n 73\n     2\n 40\n0.375\n 49\n0.25\n 49\n-0.125\n\
  0\nLTYPE\n  2\nCENTER\n 70\n    64\n\
  3\n____ _ ____ _ ____ _ ____ _ ____ _ ____ _ ____\n\
 72\n    65\n 73\n     4\n 40\n2.0\n 49\n1.25\n 49\n-0.25\n\
 49\n0.25\n 49\n-0.25\n\
  0\nLTYPE\n  2\nPHANTOM\n 70\n    64\n\
  3\n_____ _ _ _____ _ _ _____ _ _ _____ _ _ ____\n\
 72\n    65\n 73\n     6\n 40\n2.5\n 49\n1.25\n\
 49\n-0.25\n 49\n0.25\n 49\n-0.25\n 49\n0.25\n 49\n-0.25\n\
  0\nLTYPE\n  2\nDOT\n 70\n    64\n\
  3\n...............................................\n\
 72\n    65\n 73\n     2\n 40\n0.25\n 49\n0.0\n 49\n-0.25\n\
  0\nLTYPE\n  2\nDASHDOT\n 70\n    64\n\
  3\n__ . __ . __ . __ . __ . __ . __ . __ . __ . __\n\
 72\n    65\n 73\n     4\n 40\n1.0\n 49\n0.5\n 49\n-0.25\n\
 49\n0.0\n 49\n-0.25\n\
  0\nENDTAB\n";

	fprintf(GPT.P_GpOutFile, dxfi1, pThis->MaxX / DXF_UNIT, pThis->MaxY / DXF_UNIT, pThis->MaxX / DXF_UNIT, pThis->MaxY / DXF_UNIT,
	    text_style, DXF_TEXTHEIGHT / DXF_UNIT, LINEWIDTH, (double)LT_SCALE, layer_colour[0]);
	/* the linetype table */
	fprintf(GPT.P_GpOutFile, dxfi2, DXF_LINE_TYPES);
	/* the layer table */
	fprintf(GPT.P_GpOutFile, "  0\nTABLE\n  2\nLAYER\n 70\n   %-d\n", MAX_LAYER);
	for(i = 1; i <= MAX_LAYER; i++)
		fprintf(GPT.P_GpOutFile, "  0\nLAYER\n  2\n%s\n 70\n   64\n62\n   %s\n  6\n%s\n", layer_name[i-1], layer_colour[i-1], layer_lines[i-1]);
	/* no blocks for insertion */
	/* start the entity section */
	fputs("  0\nENDTAB\n0\nENDSEC\n\
  0\nSECTION\n  2\nBLOCKS\n  0\nENDSEC\n\
  0\nSECTION\n\
  2\nENTITIES\n", GPT.P_GpOutFile);
}

TERM_PUBLIC void DXF_text(GpTermEntry * pThis)
{
	if(vector_was_last)
		fputs("  0\nSEQEND\n", GPT.P_GpOutFile);
	fputs("  0\nENDSEC\n  0\nEOF\n", GPT.P_GpOutFile);
}

TERM_PUBLIC void DXF_linetype(GpTermEntry * pThis, int linetype)
{
	linetype = ABS(linetype);
	linetype = linetype % DXF_LINE_TYPES;
	dxf_linetype = linetype;
}

TERM_PUBLIC void DXF_move(GpTermEntry * pThis, uint x, uint y)
{
	DXF_posx = x;
	DXF_posy = y;
	if(vector_was_last)
		fputs("  0\nSEQEND\n", GPT.P_GpOutFile);
	vector_was_last = FALSE;
	fprintf(GPT.P_GpOutFile,
	    "\
  0\nPOLYLINE\n  8\n%s\n 66\n   1\n\
  6\n%s\n\
  0\nVERTEX\n  8\n%s\n\
  6\n%s\n\
 10\n%-6.3f\n 20\n%-6.3f\n 30\n0.000\n",
	    layer_name[dxf_linetype], layer_lines[dxf_linetype], layer_name[dxf_linetype], layer_lines[dxf_linetype], DXF_posx / DXF_UNIT, DXF_posy / DXF_UNIT);
}

TERM_PUBLIC void DXF_vector(GpTermEntry * pThis, uint ux, uint uy)
{
	DXF_posx = ux;
	DXF_posy = uy;
	vector_was_last = TRUE;
	fprintf(GPT.P_GpOutFile, "\
  0\nVERTEX\n  8\n%s\n\
  6\n%s\n\
  10\n%-6.3f\n  20\n%-6.3f\n  30\n0.000\n", layer_name[dxf_linetype], layer_lines[dxf_linetype], DXF_posx / DXF_UNIT, DXF_posy / DXF_UNIT);
}

TERM_PUBLIC void DXF_put_text(GpTermEntry * pThis, uint x, uint y, const char str[])
{
	int stl;
	float xleftpos, yleftpos, xrightpos, yrightpos;
	// shut up gcc warnings  - SB 
	xleftpos = yleftpos = xrightpos = yrightpos = 1.0; /* dummy */
	// ignore empty strings 
	if(!str[0])
		return;
	stl = 0;
	while(str[stl])
		++stl;          /* get string length */
	if(vector_was_last)
		fputs("  0\nSEQEND\n", GPT.P_GpOutFile);
	vector_was_last = FALSE;
	fprintf(GPT.P_GpOutFile, "  0\nTEXT\n  8\n%s\n", layer_name[TEXT_LAYER]);
	if(dxf_angle != 90.0) {
		switch(dxf_justify) {
			case LEFT:
			    xleftpos = (float)x;
			    yleftpos = (float)(y - DXF_VCHAR / 4.0);
			    xrightpos = (float)(x + stl * DXF_TEXTWIDTH);
			    yrightpos = yleftpos;
			    break;
			case RIGHT:
			    xleftpos = (float)(x - stl * DXF_TEXTWIDTH);
			    yleftpos = (float)(y - DXF_VCHAR / 4.0);
			    xrightpos = (float)x;
			    yrightpos = yleftpos;
			    break;
			case CENTRE:
			    xleftpos = (float)(x - stl * DXF_TEXTWIDTH / 2.0);
			    yleftpos = (float)(y - DXF_VCHAR / 4.0);
			    xrightpos = (float)x; /* center point */
			    yrightpos = yleftpos;
			    break;
		}
	}
	else {
		switch(dxf_justify) {
			case LEFT:
			    xleftpos = (float)(x + DXF_VCHAR / 4.0);
			    yleftpos = (float)y;
			    xrightpos = xleftpos;
			    yrightpos = (float)(y + stl * DXF_TEXTWIDTH);
			    break;
			case RIGHT:
			    xleftpos = (float)(x + DXF_VCHAR / 4.0);
			    yleftpos = (float)(y - stl * DXF_HCHAR);
			    xrightpos = xleftpos;
			    yrightpos = (float)y;
			    break;
			case CENTRE:
			    xleftpos = (float)(x + DXF_VCHAR / 4.0);
			    yleftpos = (float)(y - stl * DXF_TEXTWIDTH / 2.0);
			    xrightpos = xleftpos;
			    yrightpos = (float)y; /* center point */
			    break;
		}
	}

	fprintf(GPT.P_GpOutFile, "\
 10\n%-6.3f\n 20\n%-6.3f\n 30\n0.000\n\
 40\n%-6.3f\n  1\n%s\n 50\n%-6.3f\n\
  7\n%s\n",
	    xleftpos / DXF_UNIT, yleftpos / DXF_UNIT,
	    DXF_TEXTHEIGHT / DXF_UNIT, str, dxf_angle,
	    text_style);

	if(dxf_justify != LEFT) {
		fprintf(GPT.P_GpOutFile, " 72\n%d\n\
 11\n%-6.3f\n 21\n%-6.3f\n 31\n0.000\n",
		    dxf_justify,
		    xrightpos / DXF_UNIT, yrightpos / DXF_UNIT);
	}
}

TERM_PUBLIC int DXF_text_angle(GpTermEntry * pThis, int ang)
{
	dxf_angle = (ang ? 90.0f : 0.0f);
	return TRUE;
}

TERM_PUBLIC int DXF_justify_text(GpTermEntry * pThis, enum JUSTIFY mode)
{
	dxf_justify = mode;
	return TRUE;
}

TERM_PUBLIC void DXF_reset(GpTermEntry * pThis)
{
	DXF_posx = DXF_posy = 0;
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE
TERM_TABLE_START(dxf_driver)
	"dxf", 
	"dxf-file for AutoCad (default size 120x80)",
	(uint)DXF_XMAX, 
	(uint)DXF_YMAX, 
	(uint)DXF_VCHAR, 
	(uint)DXF_HCHAR,
	(uint)DXF_VTIC, 
	(uint)DXF_HTIC, 
	GnuPlot::OptionsNull, 
	DXF_init, 
	DXF_reset,
	DXF_text, 
	GnuPlot::NullScale, 
	DXF_graphics, 
	DXF_move, 
	DXF_vector,
	DXF_linetype, 
	DXF_put_text, 
	DXF_text_angle,
	DXF_justify_text, 
	GnuPlot::DoPoint, 
	GnuPlot::DoArrow, 
	set_font_null 
TERM_TABLE_END(dxf_driver)

#undef LAST_TERM
#define LAST_TERM dxf_driver

#endif /* TERM_TABLE */
#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
START_HELP(dxf)
"1 dxf",
"?commands set terminal dxf",
"?set terminal dxf",
"?set term dxf",
"?terminal dxf",
"?term dxf",
"?dxf",
" Terminal driver `dxf` for export to  AutoCad (Release 10.x).",
" It has no options. The default size is 120x80 AutoCad units.",
" `dxf` uses seven colors (white, red, yellow, green, cyan, blue and magenta)",
" that can be changed only by modifying the source file.  If a black-and-white",
" plotting device is used the colors are mapped to differing line thicknesses.",
" Note: someone please update this terminal to the 2012 DXF standard!"
END_HELP(dxf)
#endif /* TERM_HELP */
