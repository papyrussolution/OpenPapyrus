// GNUPLOT - hpljii.trm 
// Copyright 1990 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
/*
 * This file is included by ../term.c.
 * This terminal driver supports: hpljii, hpdj
 * AUTHORS: John Engels, Russell Lang, Maurice Castro
 * send your comments or suggestions to (gnuplot-info@lists.sourceforge.net).
 */
/* The following HP laserjet series II driver uses generic bit mapped graphics
   routines from bitmap.c to build up a bit map in memory.  The driver
   interchanges columns and lines in order to access entire lines
   easily and returns the lines to get bits in the right order :
   (x,y) -> (y,XMAX-1-x). */
/* This interchange is done by calling b_makebitmap() with reversed
   xmax and ymax, and then setting b_rastermode to TRUE.  b_setpixel()
   will then perform the interchange before each pixel is plotted */
/* by John Engels JENGELS@BNANDP51.BITNET, inspired by the hpljet driver
   of Jyrki Yli-Nokari */

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
	register_term(hpljii)
	register_term(hpdj)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void HPLJII_options();
TERM_PUBLIC void HPLJII_init(GpTermEntry * pThis);
TERM_PUBLIC void HPLJII_graphics(GpTermEntry * pThis);
TERM_PUBLIC void HPLJII_text(GpTermEntry * pThis);
TERM_PUBLIC void HPLJII_linetype(GpTermEntry * pThis, int linetype);
TERM_PUBLIC void HPLJII_put_text(GpTermEntry * pThis, uint x, uint y, const char * str);
TERM_PUBLIC void HPLJII_reset(GpTermEntry * pThis);
TERM_PUBLIC void HPDJ_graphics(GpTermEntry * pThis);
TERM_PUBLIC void HPDJ_text(GpTermEntry * pThis);
// default values for term_tbl 
#define HPLJII_75PPI_XMAX (1920/4)
#define HPLJII_75PPI_YMAX (1920/4)
#define HPLJII_75PPI_HCHAR (1920/4/6)
#define HPLJII_75PPI_VCHAR (1920/4/10)
#define HPLJII_75PPI_VTIC 5
#define HPLJII_75PPI_HTIC 5
//#endif

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY
//
// We define 4 different print qualities : 300ppi, 150ppi, 100ppi and 75ppi.  (Pixel size = 1, 2, 3, 4 dots) 
//
static int hplj_dpp = 4; // note: c is char, but must be declared int due to an old K&R ANSI-C strict HP cc 
#define HPLJII_DPP (hplj_dpp)   /* dots per pixel */
#define HPLJII_PPI (300/HPLJII_DPP)     /* pixel per inch */
// make XMAX and YMAX a multiple of 8 
static uint HPLJII_XMAX_(const GnuPlot * pGp) { return (8*(uint)(pGp->V.Size.x*1920/HPLJII_DPP/8.0+0.9)); }
static uint HPLJII_YMAX_(const GnuPlot * pGp) { return (8*(uint)(pGp->V.Size.y*1920/HPLJII_DPP/8.0+0.9)); }

#define HPLJII_VCHAR (HPLJII_PPI/6) // Courier font with 6 lines per inch 
#define HPLJII_HCHAR (HPLJII_PPI/10) // Courier font with 10 characters per inch 
#define HPLJII_PUSH_CURSOR fputs("\033&f0S", GPT.P_GpOutFile) // Save current cursor position 
#define HPLJII_POP_CURSOR fputs("\033&f1S", GPT.P_GpOutFile) // Restore cursor position 
#define HPLJII_COURIER fputs("\033(0N\033(s0p10.0h12.0v0s0b3T\033&l6D", GPT.P_GpOutFile) // be sure to use courier font with 6lpi and 10cpi 

static void HPLJII_putc(GpTermEntry * pThis, uint x, uint y, int c, int ang);
// bm_pattern not appropriate for 300ppi graphics 
#ifndef GOT_300_PATTERN
	#define GOT_300_PATTERN
	static uint b_300ppi_pattern[] = { 0xffff, 0x1111, 0xffff, 0x3333, 0x0f0f, 0x3f3f, 0x0fff, 0x00ff, 0x33ff };
#endif

TERM_PUBLIC void HPLJII_options(GpTermEntry * pThis, GnuPlot * pGp)
{
	char opt[4];
	int parse_error = 0;
	if(pGp->Pgm.EndOfCommand()) {
		PTR32(GPT.TermOptions)[0] = 0;
	}
	else {
		if(pGp->Pgm.GetCurTokenLength() > 3) {
			parse_error = 1; /* see below */
		}
		else {
			// almost_equals() won't accept numbers - use strcmp() instead 
			pGp->Pgm.Capture(opt, pGp->Pgm.GetCurTokenIdx(), pGp->Pgm.GetCurTokenIdx(), 4);
			if(sstreq(opt, "75")) {
				hplj_dpp = 4;
			}
			else if(sstreq(opt, "100")) {
				hplj_dpp = 3;
			}
			else if(sstreq(opt, "150")) {
				hplj_dpp = 2;
			}
			else if(sstreq(opt, "300")) {
				hplj_dpp = 1;
			}
			else {
				// error, but set dpi anyway, since term it already set 
				parse_error = 1;
			}
			pGp->Pgm.Shift();
		}
	}
	pThis->MaxX = HPLJII_XMAX_(pGp);
	pThis->MaxY = HPLJII_YMAX_(pGp);
	switch(hplj_dpp) {
		case 1:
		    strcpy(GPT.TermOptions, "300");
		    pThis->TicV = 15;
		    pThis->TicH = 15;
		    break;
		case 2:
		    strcpy(GPT.TermOptions, "150");
		    pThis->TicV = 8;
		    pThis->TicH = 8;
		    break;
		case 3:
		    strcpy(GPT.TermOptions, "100");
		    pThis->TicV = 6;
		    pThis->TicH = 6;
		    break;
		case 4:
		    strcpy(GPT.TermOptions, "75");
		    pThis->TicV = 5;
		    pThis->TicH = 5;
		    break;
	}
	if(parse_error)
		pGp->IntErrorCurToken("expecting dots per inch size 75, 100, 150 or 300");
}

TERM_PUBLIC void HPLJII_init(GpTermEntry * pThis)
{
	pThis->ChrV = HPLJII_VCHAR;
	pThis->ChrH = HPLJII_HCHAR;
}

TERM_PUBLIC void HPLJII_graphics(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	HPLJII_COURIER;
	HPLJII_PUSH_CURSOR;
	// rotate plot -90 degrees by reversing XMAX and YMAX and by setting b_rastermode to TRUE 
	p_gp->BmpMakeBitmap(HPLJII_YMAX_(p_gp), HPLJII_XMAX_(p_gp), 1);
	p_gp->_Bmp.b_rastermode = TRUE;
}
//
// HPLJIItext by rjl - no compression 
//
TERM_PUBLIC void HPLJII_text(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	fprintf(GPT.P_GpOutFile, "\033*t%dR", HPLJII_PPI);
	HPLJII_POP_CURSOR;
	fputs("\033*r1A", GPT.P_GpOutFile);
	// dump bitmap in raster mode 
	for(int x = p_gp->_Bmp.b_xsize - 1; x >= 0; x--) {
		const int row = (p_gp->_Bmp.b_ysize / 8) - 1;
		fprintf(GPT.P_GpOutFile, "\033*b0m%dW", p_gp->_Bmp.b_ysize / 8);
		for(int j = row; j >= 0; j--) {
			fputc((char)(*((*p_gp->_Bmp.b_p)[j] + x)), GPT.P_GpOutFile);
		}
	}
	fputs("\033*rB", GPT.P_GpOutFile);
	p_gp->BmpFreeBitmap();
	putc('\f', GPT.P_GpOutFile);
}

TERM_PUBLIC void HPLJII_linetype(GpTermEntry * pThis, int linetype)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(hplj_dpp == 1) {
		if(linetype >= 7)
			linetype %= 7;
		// b_pattern not appropriate for 300ppi graphics 
		p_gp->_Bmp.b_linemask = b_300ppi_pattern[linetype + 2];
		p_gp->_Bmp.b_maskcount = 0;
	}
	else {
		b_setlinetype(pThis, linetype);
	}
}

TERM_PUBLIC void HPLJII_put_text(GpTermEntry * pThis, uint x, uint y, const char * str)
{
	GnuPlot * p_gp = pThis->P_Gp;
	switch(p_gp->_Bmp.b_angle) {
		case 0:
		    y -= HPLJII_VCHAR / 5;
		    HPLJII_POP_CURSOR;
		    HPLJII_PUSH_CURSOR;
		    // (0,0) is the upper left point of the paper 
		    fprintf(GPT.P_GpOutFile, "\033*p%+dx%+dY", x * HPLJII_DPP, (HPLJII_YMAX_(p_gp) - y - 1) * HPLJII_DPP);
		    fputs(str, GPT.P_GpOutFile);
			// for (; *str; ++str, x += HPLJII_HCHAR)
				//HPLJII_putc (x, y, *str, p_gp->_Bmp.b_angle);
		    break;
		case 1:
		    y += (HPLJII_HCHAR - 2 * HPLJII_VCHAR) / 2;
		    y += (HPLJII_VCHAR + HPLJII_HCHAR) * strlen(str) / 2;
		    for(; *str; ++str, y -= HPLJII_VCHAR)
			    HPLJII_putc(pThis, x, y, *str, p_gp->_Bmp.b_angle);
		    break;
	}
}

static void HPLJII_putc(GpTermEntry * pThis, uint x, uint y, int c, int/*ang*/)
{
	HPLJII_POP_CURSOR;
	HPLJII_PUSH_CURSOR;
	// (0,0) is the upper left point of the paper 
	fprintf(GPT.P_GpOutFile, "\033*p%+dx%+dY", x * HPLJII_DPP, (HPLJII_YMAX_(pThis->P_Gp) - y - 1) * HPLJII_DPP);
	fputc(c, GPT.P_GpOutFile);
}

TERM_PUBLIC void HPLJII_reset(GpTermEntry * pThis)
{
	fflush_binary(); /* Only needed for VMS */
}
//
// HP DeskJet routines 
//
TERM_PUBLIC void HPDJ_graphics(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	switch(hplj_dpp) {
		case 1:
		    p_gp->BmpCharSize(FNT13X25);
		    pThis->ChrV = FNT13X25_VCHAR;
		    pThis->ChrH = FNT13X25_HCHAR;
		    break;
		case 2:
		    p_gp->BmpCharSize(FNT13X25);
		    pThis->ChrV = FNT13X25_VCHAR;
		    pThis->ChrH = FNT13X25_HCHAR;
		    break;
		case 3:
		    p_gp->BmpCharSize(FNT9X17);
		    pThis->ChrV = FNT9X17_VCHAR;
		    pThis->ChrH = FNT9X17_HCHAR;
		    break;
		case 4:
		    p_gp->BmpCharSize(FNT5X9);
		    pThis->ChrV = FNT5X9_VCHAR;
		    pThis->ChrH = FNT5X9_HCHAR;
		    break;
	}
	// rotate plot -90 degrees by reversing XMAX and YMAX and by setting b_rastermode to TRUE 
	p_gp->BmpMakeBitmap(HPLJII_YMAX_(p_gp), HPLJII_XMAX_(p_gp), 1);
	p_gp->_Bmp.b_rastermode = TRUE;
}
//
// 0 compression raster bitmap dump. Compatible with HP DeskJet 500
// hopefully compatible with other HP Deskjet printers
//
TERM_PUBLIC void HPDJ_text(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int x, j, row;
	fprintf(GPT.P_GpOutFile, "\033*b0M\033*t%dR\033*r1A", HPLJII_PPI);
	// dump bitmap in raster mode 
	for(x = p_gp->_Bmp.b_xsize - 1; x >= 0; x--) {
		row = (p_gp->_Bmp.b_ysize / 8) - 1;
		fprintf(GPT.P_GpOutFile, "\033*b%dW", p_gp->_Bmp.b_ysize / 8);
		for(j = row; j >= 0; j--) {
			fputc((char)(*((*p_gp->_Bmp.b_p)[j] + x)), GPT.P_GpOutFile);
		}
	}
	fputs("\033*rbC", GPT.P_GpOutFile);
	p_gp->BmpFreeBitmap();
	putc('\f', GPT.P_GpOutFile);
}

#endif

#ifdef TERM_TABLE

TERM_TABLE_START(hpljii_driver)
	"hpljii", 
	"HP Laserjet series II, [75 100 150 300]",
	HPLJII_75PPI_XMAX, 
	HPLJII_75PPI_YMAX, 
	HPLJII_75PPI_VCHAR,
	HPLJII_75PPI_HCHAR, 
	HPLJII_75PPI_VTIC, 
	HPLJII_75PPI_HTIC, 
	HPLJII_options,
	HPLJII_init, 
	HPLJII_reset, 
	HPLJII_text, 
	GnuPlot::NullScale,
	HPLJII_graphics, 
	b_move, 
	b_vector, 
	HPLJII_linetype,
	HPLJII_put_text, 
	b_text_angle, 
	GnuPlot::NullJustifyText, 
	GnuPlot::LineAndPoint,
	GnuPlot::DoArrow, 
	set_font_null, 
	0, 
	TERM_BINARY,
	0, 
	0, 
	b_boxfill 
TERM_TABLE_END(hpljii_driver)

#undef LAST_TERM
#define LAST_TERM hpljii_driver

TERM_TABLE_START(hpdj_driver)
	"hpdj", 
	"HP DeskJet 500, [75 100 150 300]",
	HPLJII_75PPI_XMAX, 
	HPLJII_75PPI_YMAX, 
	HPLJII_75PPI_VCHAR,
	HPLJII_75PPI_HCHAR, 
	HPLJII_75PPI_VTIC, 
	HPLJII_75PPI_HTIC, 
	HPLJII_options,
	HPLJII_init, 
	HPLJII_reset, 
	HPDJ_text, 
	GnuPlot::NullScale,
	HPDJ_graphics, 
	b_move, 
	b_vector, 
	HPLJII_linetype,
	b_put_text, 
	b_text_angle, 
	GnuPlot::NullJustifyText, 
	GnuPlot::LineAndPoint,
	GnuPlot::DoArrow, 
	set_font_null, 
	0, 
	TERM_BINARY,
	0, 
	0, 
	b_boxfill 
TERM_TABLE_END(hpdj_driver)

#undef LAST_TERM
#define LAST_TERM hpdj_driver

#endif
#endif

#ifdef TERM_HELP
START_HELP(hpljii)
"1 hpljii",
"?commands set terminal hpljii",
"?set terminal hpljii",
"?set term hpljii",
"?terminal hpljii",
"?term hpljii",
"?hpljii",
"?commands set terminal hpdj",
"?set terminal hpdj",
"?set term hpdj",
"?terminal hpdj",
"?term hpdj",
"?hpdj",
" Note: only available if gnuplot is configured --with-bitmap-terminals.",
" The `hpljii` terminal driver supports the HP Laserjet Series II printer.  The",
" `hpdj` driver supports the HP DeskJet 500 printer.  These drivers allow a",
" choice of resolutions.",
"",
" Syntax:",
"       set terminal hpljii | hpdj {<res>}",
"",
" where `res` may be 75, 100, 150 or 300 dots per inch; the default is 75.",
" Rasterization at the higher resolutions may require a large amount of memory.",
"",
" The `hp500c` terminal is similar to `hpdj`; `hp500c` additionally supports",
" color and compression."
END_HELP(hpljii)
#endif /* TERM_HELP */
