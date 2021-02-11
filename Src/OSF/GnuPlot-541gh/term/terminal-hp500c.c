// GNUPLOT - hp500c.trm 
// Copyright 1990 - 1993, 1998, 2004
/*
 * This file is included by ../term.c.
 *
 * This terminal driver supports: hpdj 500c
 *
 * AUTHORS
 *  John Engels      -- \
 *  Russell Lang     ----> HPLJII.trm
 *  Maurice Castro   -- /
 *  UdoHessenauer    ----> derived this version from the above one
 *
 * send your comments or suggestions to (gnuplot-info@lists.sourceforge.net).
 *
 */
/* The following HP Deskjet500c  driver uses generic bit mapped graphics
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
#define TERM_TABLE_START(x) termentry x {
#define TERM_TABLE_END(x)   };
// } @experimental

#ifdef TERM_REGISTER
	register_term(hp500c)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void HP500C_options();
TERM_PUBLIC void HP500C_init(termentry * pThis);
TERM_PUBLIC void HP500C_reset();
TERM_PUBLIC void HP500C_linetype(int linetype);
TERM_PUBLIC void HP500C_graphics();
TERM_PUBLIC void HP500C_text();
/* default values for term_tbl */
#define HP500C_75PPI_XMAX (1920/4)
#define HP500C_75PPI_YMAX (1920/4)
#define HP500C_75PPI_HCHAR (1920/4/6)
#define HP500C_75PPI_VCHAR (1920/4/10)
#define HP500C_75PPI_VTIC 5
#define HP500C_75PPI_HTIC 5
#define GOT_HP500C_PROTO
//#endif

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY

/* We define 4 different print qualities : 300ppi, 150ppi, 100ppi and
   75ppi.  (Pixel size = 1, 2, 3, 4 dots) */

#define HP500C_DPP (hpdj_dpp)   /* dots per pixel */
#define HP500C_PPI (300/HP500C_DPP)     /* pixel per inch */
// make XMAX and YMAX a multiple of 8 
#define HP500C_XMAX (8*(uint)(GPO.V.Size.x*1920/HP500C_DPP/8.0+0.9))
#define HP500C_YMAX (8*(uint)(GPO.V.Size.y*1920/HP500C_DPP/8.0+0.9))

#define HP500C_VCHAR (HP500C_PPI/6) // Courier font with 6 lines per inch 
#define HP500C_HCHAR (HP500C_PPI/10) // Courier font with 10 characters per inch 
#define HP500C_PUSH_CURSOR fputs("\033&f0S", gpoutfile) // Save current cursor position 
#define HP500C_POP_CURSOR fputs("\033&f1S", gpoutfile) // Restore cursor position 
#define HP500C_COURIER fputs("\033(0N\033(s0p10.0h12.0v0s0b3T\033&l6D", gpoutfile) // be sure to use courier font with 6lpi and 10cpi 

static int HP_compress(uchar * op, uchar * oe, uchar * cp);
static uchar HP_complement(int c);
static int HP_compress_to_TIFF(uchar * op, uchar * oe, uchar * cp);
static int HP_nocompress(uchar * op, uchar * oe, uchar * cp);

static int hpdj_dpp = 4;
static int HP_COMP_MODE = 0;

/* bm_pattern not appropriate for 300ppi graphics */
#ifndef GOT_300_PATTERN
#define GOT_300_PATTERN
static unsigned int b_300ppi_pattern[] =
{
	0xffff, 0x1111,
	0xffff, 0x3333, 0x0f0f, 0x3f3f, 0x0fff, 0x00ff, 0x33ff
};
#endif

TERM_PUBLIC void HP500C_options(TERMENTRY * pThis, GnuPlot * pGp)
{
	char opt[6];
#define HPDJCERROR "expecting dots per inch size 75, 100, 150 or 300 and/or compression method"
	while(!GPO.Pgm.EndOfCommand()) {
		if(GPO.Pgm.GetCurTokenLength() > 4)
			GPO.IntErrorCurToken(HPDJCERROR);
		/* almost_equals() won't accept numbers - use strcmp() instead */
		GPO.Pgm.Capture(opt, GPO.Pgm.GetCurTokenIdx(), GPO.Pgm.GetCurTokenIdx(), 6);
		if(!strcmp(opt, "75")) {
			hpdj_dpp = 4;
			HP_COMP_MODE = 0;
		}
		else if(!strcmp(opt, "100")) {
			hpdj_dpp = 3;
			HP_COMP_MODE = 0;
		}
		else if(!strcmp(opt, "150")) {
			hpdj_dpp = 2;
			HP_COMP_MODE = 0;
		}
		else if(!strcmp(opt, "300")) {
			hpdj_dpp = 1;
			HP_COMP_MODE = 0;
		}
		else if(!strcmp(opt, "rle")) {
			HP_COMP_MODE = 1;
		}
		else if(!strcmp(opt, "tiff")) {
			HP_COMP_MODE = 2;
		}
		GPO.Pgm.Shift();
	}
	term->MaxX = HP500C_XMAX;
	term->MaxY = HP500C_YMAX;
	switch(hpdj_dpp) {
		case 1:
		    strcpy(term_options, "300");
		    term->TicV = 15;
		    term->TicH = 15;
		    break;
		case 2:
		    strcpy(term_options, "150");
		    term->TicV = 8;
		    term->TicH = 8;
		    break;
		case 3:
		    strcpy(term_options, "100");
		    term->TicV = 6;
		    term->TicH = 6;
		    break;
		case 4:
		    strcpy(term_options, "75");
		    term->TicV = 5;
		    term->TicH = 5;
		    break;
	}
	switch(HP_COMP_MODE) {
		case 0:
		    strcat(term_options, " no comp");
		    break;
		case 1:
		    strcat(term_options, " RLE");
		    break;
		case 2:
		    strcat(term_options, " TIFF");
		    break;
		case 3:         /* not implemented yet */
		    strcat(term_options, " Delta Row");
		    break;
	}
}

TERM_PUBLIC void HP500C_init(termentry * pThis)
{
	// HBB 980226: all changes to pThis-> fields *must* happen here, not in graphics() !
	switch(hpdj_dpp) {
		case 1:
		    b_charsize(FNT13X25);
		    pThis->ChrV = FNT13X25_VCHAR;
		    pThis->ChrH = FNT13X25_HCHAR;
		    break;
		case 2:
		    b_charsize(FNT13X25);
		    pThis->ChrV = FNT13X25_VCHAR;
		    pThis->ChrH = FNT13X25_HCHAR;
		    break;
		case 3:
		    b_charsize(FNT9X17);
		    pThis->ChrV = FNT9X17_VCHAR;
		    pThis->ChrH = FNT9X17_HCHAR;
		    break;
		case 4:
		    b_charsize(FNT5X9);
		    pThis->ChrV = FNT5X9_VCHAR;
		    pThis->ChrH = FNT5X9_HCHAR;
		    break;
	}
}

TERM_PUBLIC void HP500C_reset()
{
	fflush_binary(); /* Only needed for VMS */
}

/* HP DeskJet 500c routines */

TERM_PUBLIC void HP500C_linetype(int linetype)
{
	if(linetype < 0)
		linetype = 7;
	else if(linetype >= 8) {
		linetype %= 8;
	}
	switch(linetype) {
		case 0: linetype = 6; break;
		case 1: linetype = 5; break;
		case 2: linetype = 3; break;
		case 3: linetype = 2; break;
		case 4: linetype = 1; break;
		case 5: linetype = 4; break;
		case 6: linetype = 7;
	}
	b_setvalue(linetype);
}

#if 0
	void HP500C_point(uint x, uint y, int value)
	{
		HP500C_linetype(value);
		GnuPlot::DoPoint(x, y, value);
	}
#endif

TERM_PUBLIC void HP500C_graphics()
{
	// HBB 980226: moved block of code from here to init() 
	// rotate plot -90 degrees by reversing XMAX and YMAX and by setting b_rastermode to TRUE 
	b_makebitmap(HP500C_YMAX, HP500C_XMAX, 3);
	b_rastermode = TRUE;
}
/*
 * Run-length encoding for the DeskJet. We have pairs of <count>
 * <what>, where count goes from 0 (meaning one count) to 255
 * this might double the size of the image.
 */

static int HP_compress(uchar * op, uchar * oe, uchar * cp)
{
	uchar * ce = cp;
	while(op < oe) {
		uchar prevchar = *op; /* remember char */
		uchar count = 1;      /* its read the first time */
		while(++op < oe && *op == prevchar && count < 255) {
			/* set op to the next char */
			count++; /* and count it  */
		}
		*ce++ = --count; /* were ready, so correct the count */
		*ce++ = prevchar; /* and store <what> */
	}
	*ce = 0;                /* just to be safe   */
	return ce - cp;         /* length of  cbufs */
}

static uchar HP_complement(int c)
{
	return (uchar)(256 - c);
}

static int HP_compress_to_TIFF(uchar * op,          /* original pointer */
    uchar * oe,          /* end of orig string */
    uchar * cp)          /* pointer for compressed data */
{
	uchar * countposition;
	uchar * ce = cp;
	while(op < oe) {
		uchar prevchar;
		uchar count;
		prevchar = *op; /* gelesenes Zeichen aufbewaren */
		count = 1;      /* bisher wurde es einmal gelesen */
		while(++op < oe && *op == prevchar && count < 128) {
			count++;
		}
		*ce = HP_complement(count - 1);
		/* remember count for building blocks of literal bytes */
		countposition = ce++;
		*ce++ = prevchar;

		if(count < 2) {
			while(op < oe && (prevchar != *op || *op != *(op + 1))) {
				/* only use rle for at least 3 equal bytes */
				*ce++ = *op;
				count++;
				prevchar = *op++;
				if(op > oe)
					puts("FATAL op> oe!!\n");
			}
			if(op < oe && prevchar == *op) {
				op--;
				count--;
				ce--;
			}
			*countposition = count - 1;
		}
	}
	return ce - cp;
}

static int HP_nocompress(uchar * op, uchar * oe, uchar * cp)
{
	uchar * ce = cp;
	while(op < oe)
		*ce++ = *op++;
	return ce - cp;
}

/* 0 compression raster bitmap dump. Compatible with HP DeskJet 500
   hopefully compatible with other HP Deskjet printers */

TERM_PUBLIC void HP500C_text()
{
	register int x, j, row, count = 0;
	uchar * obuf, * oe, * cbuf, * ce;
	if((obuf = (uchar*)malloc(100 * b_psize)) == 0)
		puts("FATAL!-- couldn't get enough memory for obuf");
	if((cbuf = (uchar*)malloc(400 * b_psize)) == 0)
		puts("FATAL!-- couldn't get enough memory for cbuf");

	oe = obuf;

	fprintf(gpoutfile, "\
\033*t%dR\
\033*r1A\
\033*b%1dM\
\033*r%dS\
\033*r-3U",
	    HP500C_PPI,
	    HP_COMP_MODE,
	    b_ysize);

	/* dump bitmap in raster mode */
	for(x = b_xsize - 1; x >= 0; x--) {
		row = (b_ysize / 8) - 1;
		for(j = row; j >= 0; j--) {
			*oe++ = (char)(*((*b_p)[j] + x));
		}
		switch(HP_COMP_MODE) {
			case 2: count = HP_compress_to_TIFF(obuf, oe, cbuf); break;
			case 1: count = HP_compress(obuf, oe, cbuf); break;
			case 0: count = HP_nocompress(obuf, oe, cbuf); break;
		}
		fprintf(gpoutfile, "\033*b%dV", count);
		ce = cbuf;
		while(count--)
			fputc(*ce++, gpoutfile);
		oe = obuf;

		for(j = row; j >= 0; j--) {
			*oe++ = (char)(*((*b_p)[j + b_psize] + x));
		}
		switch(HP_COMP_MODE) {
			case 2: count = HP_compress_to_TIFF(obuf, oe, cbuf); break;
			case 1: count = HP_compress(obuf, oe, cbuf); break;
			case 0: count = HP_nocompress(obuf, oe, cbuf); break;
		}
		fprintf(gpoutfile, "\033*b%dV", count);
		ce = cbuf;
		while(count--)
			fputc(*ce++, gpoutfile);
		oe = obuf;
		for(j = row; j >= 0; j--) {
			*oe++ = (char)(*((*b_p)[j + (2 * b_psize)] + x));
		}
		switch(HP_COMP_MODE) {
			case 2: count = HP_compress_to_TIFF(obuf, oe, cbuf); break;
			case 1: count = HP_compress(obuf, oe, cbuf); break;
			case 0: count = HP_nocompress(obuf, oe, cbuf); break;
		}
		fprintf(gpoutfile, "\033*b%dW", count);
		ce = cbuf;
		while(count--)
			fputc(*ce++, gpoutfile);
		oe = obuf;
	}
	fputs("\033*rbC", gpoutfile);
	SAlloc::F(cbuf);
	SAlloc::F(obuf);
	b_freebitmap();
	putc('\f', gpoutfile);
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE

TERM_TABLE_START(hp500c_driver)
	"hp500c", 
	"HP DeskJet 500c, [75 100 150 300] [rle tiff]",
	HP500C_75PPI_XMAX, 
	HP500C_75PPI_YMAX, 
	HP500C_75PPI_VCHAR,
	HP500C_75PPI_HCHAR, 
	HP500C_75PPI_VTIC, 
	HP500C_75PPI_HTIC, 
	HP500C_options,
	HP500C_init, 
	HP500C_reset, 
	HP500C_text, 
	GnuPlot::NullScale,
	HP500C_graphics, 
	b_move, 
	b_vector, 
	HP500C_linetype,
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
TERM_TABLE_END(hp500c_driver)

#undef LAST_TERM
#define LAST_TERM hp500c_driver

#endif /* TERM_TABLE */
#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
START_HELP(hp500c)
"1 hp500c",
"?commands set terminal hp500c",
"?set terminal hp500c",
"?set term hp500c",
"?terminal hp500c",
"?term hp500c",
"?hp500c",
" Note: only available if gnuplot is configured --with-bitmap-terminals.",
" The `hp500c` terminal driver supports the Hewlett Packard HP DeskJet 500c.",
" It has options for resolution and compression.",
"",
" Syntax:",
"       set terminal hp500c {<res>} {<comp>}",
"",
" where `res` can be 75, 100, 150 or 300 dots per inch and `comp` can be \"rle\",",
" or \"tiff\".  Any other inputs are replaced by the defaults, which are 75 dpi",
" and no compression.  Rasterization at the higher resolutions may require a",
" large amount of memory."
END_HELP(hp500c)
#endif /* TERM_HELP */
