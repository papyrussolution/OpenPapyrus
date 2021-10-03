// GNUPLOT - hppj.trm 
// Copyright 1990 - 1993, 1998, 2004
/*
 * This file is included by ../term.c.
 *
 * This terminal driver supports: hppj
 *
 * AUTHORS: Dan Merget (danm@sr.hp.com)
 * This file was based on the hpljii file by: John Engels, Russell Lang, Maurice Castro
 * send your comments or suggestions to (gnuplot-info@lists.sourceforge.net).
 */
/* The following HP laserjet series II driver uses generic bit mapped graphics
 * routines from bitmap.c to build up a bit map in memory.
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
	register_term(hppj)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void HPPJ_options(GpTermEntry * pThis, GnuPlot * pGp);
TERM_PUBLIC void HPPJ_init(GpTermEntry * pThis);
TERM_PUBLIC void HPPJ_reset(GpTermEntry * pThis);
TERM_PUBLIC void HPPJ_graphics(GpTermEntry * pThis);
TERM_PUBLIC void HPPJ_text(GpTermEntry * pThis);
TERM_PUBLIC void HPPJ_linetype(GpTermEntry * pThis, int linetype);
// We define 3 different font sizes: 5x9, 9x17, and 13x25 
#define HPPJ_DPI 180            /* dots per inch */
#define HPPJ_PLANES 3           /* color planes */
#define HPPJ_COLORS (1 << HPPJ_PLANES)
// make XMAX and YMAX a multiple of 8 
#define HPPJ_XMAX (8*(uint)(9.5 * HPPJ_DPI / 8.0 + 0.9))
#define HPPJ_YMAX (8 * HPPJ_DPI)
// default values for term_tbl 
#define HPPJ_9x17_VCHAR FNT9X17_VCHAR
#define HPPJ_9x17_HCHAR FNT9X17_HCHAR
#define HPPJ_9x17_VTIC (FNT9X17_VCHAR / 2)
#define HPPJ_9x17_HTIC (FNT9X17_HCHAR / 2)
//#endif /* TERM_PROTO */

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY
static int hppj_font = FNT9X17;

TERM_PUBLIC void HPPJ_options(GpTermEntry * pThis, GnuPlot * pGp)
{
	char opt[10];
#define HPPJERROR "expecting font size FNT5X9, FNT9X17, or FNT13X25"
	PTR32(GPT.TermOptions)[0] = 0; // default to empty string and 9x17 font 
	hppj_font = FNT9X17; // in case of error or empty options
	if(!pGp->Pgm.EndOfCommand()) {
		if(pGp->Pgm.GetCurTokenLength() > 8) {
			pGp->IntErrorCurToken(HPPJERROR);
		}
		pGp->Pgm.Capture(opt, pGp->Pgm.GetCurTokenIdx(), pGp->Pgm.GetCurTokenIdx(), /*4 */ 9); /* HBB 980226 */
		if(sstreq(opt, "FNT5X9")) {
			hppj_font = FNT5X9;
			strcpy(GPT.TermOptions, "FNT5X9");
		}
		else if(sstreq(opt, "FNT9X17")) {
			hppj_font = FNT9X17;
			strcpy(GPT.TermOptions, "FNT9X17");
		}
		else if(sstreq(opt, "FNT13X25")) {
			hppj_font = FNT13X25;
			strcpy(GPT.TermOptions, "FNT13X25");
		}
		else {
			pGp->IntErrorCurToken(HPPJERROR);
		}
		pGp->Pgm.Shift();
	}
}

TERM_PUBLIC void HPPJ_init(GpTermEntry * pThis)
{
	// HBB 980226: moved this here, from graphics(): only init() may change fields of *term ! 
	switch(hppj_font) {
		case FNT5X9:
		    pThis->ChrV = FNT5X9_VCHAR;
		    pThis->ChrH = FNT5X9_HCHAR;
		    pThis->TicV = FNT5X9_VCHAR / 2;
		    pThis->TicH = FNT5X9_HCHAR / 2;
		    break;
		case FNT9X17:
		    pThis->ChrV = FNT9X17_VCHAR;
		    pThis->ChrH = FNT9X17_HCHAR;
		    pThis->TicV = FNT9X17_VCHAR / 2;
		    pThis->TicH = FNT9X17_HCHAR / 2;
		    break;
		case FNT13X25:
		    pThis->ChrV = FNT13X25_VCHAR;
		    pThis->ChrH = FNT13X25_HCHAR;
		    pThis->TicV = FNT13X25_VCHAR / 2;
		    pThis->TicH = FNT13X25_HCHAR / 2;
		    break;
	}
}

TERM_PUBLIC void HPPJ_reset(GpTermEntry * pThis)
{
	fflush_binary(); // Only needed for VMS 
}

TERM_PUBLIC void HPPJ_graphics(GpTermEntry * pThis)
{
	// HBB 980226: move a block of code from here to init() 
	pThis->P_Gp->BmpCharSize(hppj_font);
	pThis->P_Gp->BmpMakeBitmap(HPPJ_XMAX, HPPJ_YMAX, HPPJ_PLANES);
}

TERM_PUBLIC void HPPJ_text(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int x, plane, y;        /* loop indexes */
	int minRow, maxRow;     /* loop bounds */
	int numBytes;           /* Number of run-length coded bytes to output */
	int numReps;            /* Number of times the current byte is repeated */
	fprintf(GPT.P_GpOutFile,
	    "\
\033E\033*t%dR\033*r%dS\
\033*b0X\033*b0Y\033*r%dU\
\033*v%dA\033*v%dB\033*v%dC\033*v%dI\
\033*v%dA\033*v%dB\033*v%dC\033*v%dI\
\033*v%dA\033*v%dB\033*v%dC\033*v%dI\
\033*v%dA\033*v%dB\033*v%dC\033*v%dI\
\033*v%dA\033*v%dB\033*v%dC\033*v%dI\
\033*v%dA\033*v%dB\033*v%dC\033*v%dI\
\033*v%dA\033*v%dB\033*v%dC\033*v%dI\
\033*v%dA\033*v%dB\033*v%dC\033*v%dI\
\033*b1M\033*r1A",
	    HPPJ_DPI, HPPJ_YMAX, HPPJ_PLANES,
	    90, 88, 85, 0, 53, 8, 14, 1, 3, 26, 22, 2, 4, 4, 29, 3, 53, 5, 25, 4, 2, 22, 64, 5, 89, 83, 13, 6, 4, 4, 6, 7);
	// dump bitmap in raster mode using run-length encoding 
	for(x = HPPJ_XMAX - 1; x >= 0; --x) {
		for(plane = 0; plane < HPPJ_PLANES; plane++) {
			minRow = p_gp->_Bmp.b_psize * plane;
			maxRow = p_gp->_Bmp.b_psize * plane + p_gp->_Bmp.b_psize - 1;
			// Print column header 
			numBytes = 0;
			for(y = maxRow; y >= minRow; --y) {
				if(y == minRow || *((*p_gp->_Bmp.b_p)[y] + x) != *((*p_gp->_Bmp.b_p)[y-1] + x)) {
					numBytes += 2;
				}
			}
			fprintf(GPT.P_GpOutFile, "\033*b%d", numBytes);
			fputc((char)(plane < HPPJ_PLANES - 1 ? 'V' : 'W'), GPT.P_GpOutFile);
			// Print remainder of column *
			numReps = 0;
			for(y = maxRow; y >= minRow; --y) {
				if(y == minRow || *((*p_gp->_Bmp.b_p)[y] + x) != *((*p_gp->_Bmp.b_p)[y-1] + x)) {
					fputc((char)(numReps), GPT.P_GpOutFile);
					fputc((char)(*((*p_gp->_Bmp.b_p)[y] + x)), GPT.P_GpOutFile);
					numReps = 0;
				}
				else {
					numReps++;
				}
			}
		}
	}
	fputs("\033*r1B\033E", GPT.P_GpOutFile);
	p_gp->BmpFreeBitmap();
}

TERM_PUBLIC void HPPJ_linetype(GpTermEntry * pThis, int linetype)
{
	if(linetype >= 0) {
		b_setlinetype(pThis, 0);
		b_setvalue(pThis, (linetype % (HPPJ_COLORS - 1)) + 1);
	}
	else {
		b_setlinetype(pThis, linetype + 2);
		b_setvalue(pThis, HPPJ_COLORS - 1);
	}
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE

TERM_TABLE_START(hppj_driver)
	"hppj", 
	"HP PaintJet and HP3630 [FNT5X9 FNT9X17 FNT13X25]",
	HPPJ_XMAX, 
	HPPJ_YMAX,
	HPPJ_9x17_VCHAR, 
	HPPJ_9x17_HCHAR, 
	HPPJ_9x17_VTIC, 
	HPPJ_9x17_HTIC,
	HPPJ_options, 
	HPPJ_init, 
	HPPJ_reset, 
	HPPJ_text, 
	GnuPlot::NullScale, 
	HPPJ_graphics,
	b_move, 
	b_vector, 
	HPPJ_linetype, 
	b_put_text, 
	b_text_angle,
	GnuPlot::NullJustifyText, 
	GnuPlot::DoPoint, 
	GnuPlot::DoArrow, 
	set_font_null, 
	0, 
	TERM_BINARY,
	0, 
	0, 
	b_boxfill 
TERM_TABLE_END(hppj_driver)

#undef LAST_TERM
#define LAST_TERM hppj_driver

#endif /* TERM_TABLE */
#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
START_HELP(hppj)
"1 hppj",
"?commands set terminal hppj",
"?set terminal hppj",
"?set term hppj",
"?terminal hppj",
"?term hppj",
"?hppj",
" Note: only available if gnuplot is configured --with-bitmap-terminals.",
" The `hppj` terminal driver supports the HP PaintJet and HP3630 printers.  The",
" only option is the choice of font.",
"",
" Syntax:",
"       set terminal hppj {FNT5X9 | FNT9X17 | FNT13X25}",
"",
" with the middle-sized font (FNT9X17) being the default."
END_HELP(hppj)
#endif
