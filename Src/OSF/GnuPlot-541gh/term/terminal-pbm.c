// GNUPLOT - pbm.trm 
// Copyright 1990 - 1993, 1998, 2004
//
/*
 * This file is included by ../term.c.
 * This terminal driver supports: pbm
 * AUTHORS: Russell Lang
 * send your comments or suggestions to (gnuplot-info@lists.sourceforge.net).
 */
/* The following pbmplus drivers use the generic bit mapped graphics
   routines from bitmap.c to build up a bit map in memory.  The driver
   interchanges columns and lines in order to access entire lines
   easily and returns the lines to get bits in the right order :
   (x,y) -> (y,XMAX-1-x). */
/* This interchange is done by calling b_makebitmap() with reversed
   xmax and ymax, and then setting b_rastermode to TRUE.  b_setpixel()
   will then perform the interchange before each pixel is plotted */
/* See Jef Poskanzer's excellent PBMplus package for more details of
   the Portable BitMap format and for programs to convert PBM files
   to other bitmap formats. */
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
	register_term(pbm_driver)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void PBM_options(GpTermEntry * pThis, GnuPlot * pGp);
TERM_PUBLIC void PBM_init(GpTermEntry * pThis);
TERM_PUBLIC void PBM_reset(GpTermEntry * pThis);
TERM_PUBLIC void PBM_setfont(GpTermEntry * pThis);
TERM_PUBLIC void PBM_graphics(GpTermEntry * pThis);
TERM_PUBLIC void PBM_monotext(GpTermEntry * pThis);
TERM_PUBLIC void PBM_graytext(GpTermEntry * pThis);
TERM_PUBLIC void PBM_colortext(GpTermEntry * pThis);
TERM_PUBLIC void PBM_text(GpTermEntry * pThis);
TERM_PUBLIC void PBM_linetype(GpTermEntry * pThis, int linetype);
TERM_PUBLIC void PBM_point(GpTermEntry * pThis, uint x, uint y, int point);
//#endif /* TERM_PROTO */

/* make XMAX and YMAX a multiple of 8 */
#define PBM_XMAX (640)
#define PBM_YMAX (480)
#define PBM_VCHAR (FNT5X9_VCHAR)
#define PBM_HCHAR (FNT5X9_VCHAR)
#define PBM_VTIC FNT5X9_HBITS
#define PBM_HTIC FNT5X9_HBITS

#ifdef TERM_BODY

static int pbm_font = 1;        /* small font */
static int pbm_mode = 0;        /* 0:monochrome 1:gray 2:color */

/* Only needed for dubious backward compatibility with 'set size'
 * in pre-4.2 versions that didn't support 'set term size'
 */
static bool PBM_explicit_size = FALSE;

/* 7=black, 0=white */
static int pgm_gray[] = { 7, 1, 6, 5, 4, 3, 2, 1, 7 };  /* grays  */
/* bit3=!intensify, bit2=!red, bit1=!green, bit0=!blue */
static int ppm_color[] = { 15, 8, 3, 5, 6, 2, 4, 1, 11, 13, 14 };  /* colors */

enum PBM_id {
	PBM_SMALL, PBM_MEDIUM, PBM_LARGE,
	PBM_MONOCHROME, PBM_GRAY, PBM_COLOR, PBM_SIZE,
	PBM_OTHER
};

static struct gen_table PBM_opts[] =
{
	{ "s$mall", PBM_SMALL },
	{ "me$dium", PBM_MEDIUM },
	{ "l$arge", PBM_LARGE },
	{ "mo$nochrome", PBM_MONOCHROME },
	{ "g$ray", PBM_GRAY },
	{ "c$olor", PBM_COLOR },
	{ "c$olour", PBM_COLOR },
	{ "size", PBM_SIZE },
	{ NULL, PBM_OTHER }
};

TERM_PUBLIC void PBM_options(GpTermEntry * pThis, GnuPlot * pGp)
{
	int xpixels = PBM_XMAX;
	int ypixels = PBM_YMAX;
	GpValue a;
	pbm_font = 1;
	pbm_mode = 0;
	term_options[0] = NUL;
	while(!pGp->Pgm.EndOfCommand()) {
		switch(pGp->Pgm.LookupTableForCurrentToken(&PBM_opts[0])) {
			case PBM_SMALL:
			    pbm_font = 1;
			    pGp->Pgm.Shift();
			    break;
			case PBM_MEDIUM:
			    pbm_font = 2;
			    pGp->Pgm.Shift();
			    break;
			case PBM_LARGE:
			    pbm_font = 3;
			    pGp->Pgm.Shift();
			    break;
			case PBM_MONOCHROME:
			    pbm_mode = 0;
			    pThis->flags |= TERM_MONOCHROME;
			    pGp->Pgm.Shift();
			    break;
			case PBM_GRAY:
			    pbm_mode = 1;
			    pGp->Pgm.Shift();
			    break;
			case PBM_COLOR:
			    pbm_mode = 2;
			    pThis->flags &= ~TERM_MONOCHROME;
			    pGp->Pgm.Shift();
			    break;
			case PBM_SIZE:
			    pGp->Pgm.Shift();
			    if(pGp->Pgm.EndOfCommand()) {
				    pThis->MaxX = PBM_XMAX;
				    pThis->MaxY = PBM_YMAX;
				    PBM_explicit_size = FALSE;
			    }
			    else {
				    xpixels = static_cast<int>(real(pGp->ConstExpress(&a)));
				    if(pGp->Pgm.EqualsCur(",")) {
					    pGp->Pgm.Shift();
					    ypixels = static_cast<int>(real(pGp->ConstExpress(&a)));
				    }
				    PBM_explicit_size = TRUE;
			    }
			    if(xpixels > 0)
				    pThis->MaxX = xpixels;
			    if(ypixels > 0)
				    pThis->MaxY = ypixels;
			    break;
			case PBM_OTHER:
			default:
			    // reset to default, since term is already set 
			    pbm_font = 1;
			    pbm_mode = 0;
			    pGp->IntErrorCurToken("expecting: {small, medium, large} and {monochrome, gray, color}");
			    break;
		}
	}
	pThis->TicV = (pThis->MaxX < pThis->MaxY) ? pThis->MaxX/100 : pThis->MaxY/100;
	SETMAX(pThis->TicV, 1);
	pThis->TicH = pThis->TicV;
	// setup options string 
	switch(pbm_font) {
		case 1: strcat(term_options, "small"); break;
		case 2: strcat(term_options, "medium"); break;
		case 3: strcat(term_options, "large"); break;
	}
	switch(pbm_mode) {
		case 0: strcat(term_options, " monochrome"); break;
		case 1: strcat(term_options, " gray"); break;
		case 2: strcat(term_options, " color"); break;
	}
	if(PBM_explicit_size)
		sprintf(term_options + strlen(term_options), " size %d,%d", pThis->MaxX, pThis->MaxY);
}

TERM_PUBLIC void PBM_init(GpTermEntry * pThis)
{
	PBM_setfont(pThis); // HBB 980226: call it here! 
}

TERM_PUBLIC void PBM_reset(GpTermEntry * pThis)
{
	fflush_binary(); // Only needed for VMS 
}

TERM_PUBLIC void PBM_setfont(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	switch(pbm_font) {
		case 1:
		    p_gp->BmpCharSize(FNT5X9);
		    pThis->ChrV = FNT5X9_VCHAR;
		    pThis->ChrH = FNT5X9_HCHAR;
		    break;
		case 2:
		    p_gp->BmpCharSize(FNT9X17);
		    pThis->ChrV = FNT9X17_VCHAR;
		    pThis->ChrH = FNT9X17_HCHAR;
		    break;
		case 3:
		    p_gp->BmpCharSize(FNT13X25);
		    pThis->ChrV = FNT13X25_VCHAR;
		    pThis->ChrH = FNT13X25_HCHAR;
		    break;
	}
}

TERM_PUBLIC void PBM_graphics(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int numplanes = 1;
	uint xpixels = pThis->MaxX;
	uint ypixels = pThis->MaxY;
	// 'set size' should not affect the size of the canvas in pixels,
	// but versions prior to 4.2 did not have a separate 'set term size'
	if(!PBM_explicit_size) {
		xpixels *= p_gp->V.Size.x;
		ypixels *= p_gp->V.Size.y;
	}
	switch(pbm_mode) {
		case 1: numplanes = 3; break;
		case 2: numplanes = 4; break;
	}
	// HBB 980226: this is not the right place to do this: setfont() influences
	// fields of the termtable entry, and therefore must be called by init() already. 
	// PBMsetfont(); 
	// rotate plot -90 degrees by reversing XMAX and YMAX and by
	// setting b_rastermode to TRUE 
	p_gp->BmpMakeBitmap(ypixels, xpixels, numplanes);
	p_gp->_Bmp.b_rastermode = TRUE;
	if(pbm_mode != 0)
		b_setlinetype(pThis, 0); // solid lines 
}

static void PBM_monotext(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int x, j, row;
	fputs("P4\n", gpoutfile);
	fprintf(gpoutfile, "%u %u\n", p_gp->_Bmp.b_ysize, p_gp->_Bmp.b_xsize);
	// dump bitmap in raster mode 
	for(x = p_gp->_Bmp.b_xsize - 1; x >= 0; x--) {
		row = (p_gp->_Bmp.b_ysize / 8) - 1;
		for(j = row; j >= 0; j--) {
			fputc((char)(*((*p_gp->_Bmp.b_p)[j] + x)), gpoutfile);
		}
	}
	p_gp->BmpFreeBitmap();
}

static void PBM_graytext(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int x, j, row;
	int i, value;
	int mask, plane1, plane2, plane3;
	fprintf(gpoutfile, "P5\n%u %u\n%u\n", p_gp->_Bmp.b_ysize, p_gp->_Bmp.b_xsize, 255);
	// dump bitmap in raster mode 
	for(x = p_gp->_Bmp.b_xsize - 1; x >= 0; x--) {
		row = (p_gp->_Bmp.b_ysize / 8) - 1;
		for(j = row; j >= 0; j--) {
			mask = 0x80;
			plane1 = (*((*p_gp->_Bmp.b_p)[j] + x));
			plane2 = (*((*p_gp->_Bmp.b_p)[j + p_gp->_Bmp.b_psize] + x));
			plane3 = (*((*p_gp->_Bmp.b_p)[j + p_gp->_Bmp.b_psize + p_gp->_Bmp.b_psize] + x));
			for(i = 0; i < 8; i++) {
				// HBB: The values below are set to span the full range from 0 up to 255 in 7 steps: 
				value = 255;
				if(plane1 & mask)
					value -= 36;
				if(plane2 & mask)
					value -= 73;
				if(plane3 & mask)
					value -= 146;
				fputc((char)(value), gpoutfile);
				mask >>= 1;
			}
		}
	}
	p_gp->BmpFreeBitmap();
}

static void PBM_colortext(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	fprintf(gpoutfile, "P6\n%u %u\n%u\n", p_gp->_Bmp.b_ysize, p_gp->_Bmp.b_xsize, 255);
	// dump bitmap in raster mode 
	for(int x = p_gp->_Bmp.b_xsize - 1; x >= 0; x--) {
		int row = (p_gp->_Bmp.b_ysize / 8) - 1;
		for(int j = row; j >= 0; j--) {
			int mask = 0x80;
			int plane1 = (*((*p_gp->_Bmp.b_p)[j] + x));
			int plane2 = (*((*p_gp->_Bmp.b_p)[j + p_gp->_Bmp.b_psize] + x));
			int plane3 = (*((*p_gp->_Bmp.b_p)[j + p_gp->_Bmp.b_psize + p_gp->_Bmp.b_psize] + x));
			int plane4 = (*((*p_gp->_Bmp.b_p)[j + p_gp->_Bmp.b_psize + p_gp->_Bmp.b_psize + p_gp->_Bmp.b_psize] + x));
			for(int i = 0; i < 8; i++) {
				int red = (plane3 & mask) ? 1 : 3;
				int green = (plane2 & mask) ? 1 : 3;
				int blue = (plane1 & mask) ? 1 : 3;
				if(plane4 & mask) {
					red--;
					green--;
					blue--;
				}
				// HBB: '85' is exactly 255/3, so this spans the full range of colors in three steps: 
				fputc((char)(red * 85), gpoutfile);
				fputc((char)(green * 85), gpoutfile);
				fputc((char)(blue * 85), gpoutfile);
				mask >>= 1;
			}
		}
	}
	p_gp->BmpFreeBitmap();
}

TERM_PUBLIC void PBM_text(GpTermEntry * pThis)
{
	switch(pbm_mode) {
		case 0: PBM_monotext(pThis); break;
		case 1: PBM_graytext(pThis); break;
		case 2: PBM_colortext(pThis); break;
	}
}

TERM_PUBLIC void PBM_linetype(GpTermEntry * pThis, int linetype)
{
	if(linetype < -2)
		linetype = LT_BLACK;
	switch(pbm_mode) {
		case 0:
		    b_setlinetype(pThis, linetype);
		    break;
		case 1:
		    if(linetype >= 7)
			    linetype %= 7;
		    b_setvalue(pThis, pgm_gray[linetype + 2]);
		    break;
		case 2:
		    if(linetype >= 9)
			    linetype %= 9;
		    b_setvalue(pThis, ppm_color[linetype + 2]);
		    break;
	}
}

TERM_PUBLIC void PBM_point(GpTermEntry * pThis, uint x, uint y, int point)
{
	if(pbm_mode == 0)
		GnuPlot::LineAndPoint(pThis, x, y, point);
	else
		GnuPlot::DoPoint(pThis, x, y, point);
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE

TERM_TABLE_START(pbm_driver)
	"pbm", 
	"Portable bitmap [small medium large] [monochrome gray color]",
	PBM_XMAX, 
	PBM_YMAX, 
	PBM_VCHAR,
	PBM_HCHAR, 
	PBM_VTIC, 
	PBM_HTIC, 
	PBM_options,
	PBM_init, 
	PBM_reset, 
	PBM_text, 
	GnuPlot::NullScale,
	PBM_graphics, 
	b_move, 
	b_vector, 
	PBM_linetype,
	b_put_text, 
	b_text_angle, 
	GnuPlot::NullJustifyText, 
	PBM_point,
	GnuPlot::DoArrow, 
	set_font_null,
	0,                              /* pointsize */
	TERM_CAN_MULTIPLOT | TERM_BINARY,
	0, 
	0, 
	b_boxfill,
	b_linewidth,
	#ifdef USE_MOUSE
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL,
	#endif
	NULL /* make_palette */, NULL, NULL /* set_color */,
	b_filled_polygon 
TERM_TABLE_END(pbm_driver)

#undef LAST_TERM
#define LAST_TERM pbm_driver

#endif /* TERM_TABLE */

#ifdef TERM_HELP
START_HELP(pbm)
"1 pbm",
"?commands set terminal pbm",
"?set terminal pbm",
"?set term pbm",
"?terminal pbm",
"?term pbm",
"?pbm",
" Note: only available if gnuplot is configured --with-bitmap-terminals.",
" Syntax:",
"       set terminal pbm {<fontsize>} {<mode>} {size <x>,<y>}",
"",
" where <fontsize> is `small`, `medium`, or `large` and <mode> is `monochrome`,",
" `gray` or `color`.  The default plot size is 640 pixels wide and 480 pixels",
" high. The output size is white-space padded to the nearest multiple of",
" 8 pixels on both x and y. This empty space may be cropped later if needed.",
"",
" The output of the `pbm` driver depends upon <mode>: `monochrome` produces a",
" portable bitmap (one bit per pixel), `gray` a portable graymap (three bits",
" per pixel) and `color` a portable pixmap (color, four bits per pixel).",
"",
" The output of this driver can be used with various image conversion and",
" manipulation utilities provided by NETPBM.  Based on Jef Poskanzer's",
" PBMPLUS package, NETPBM provides programs to convert the above PBM formats",
" to GIF, TIFF, MacPaint, Macintosh PICT, PCX, X11 bitmap and many others.",
" Complete information is available at http://netpbm.sourceforge.net/.",
"",
" Examples:",
"       set terminal pbm small monochrome                # defaults",
"       set terminal pbm color medium size 800,600",
"       set output '| pnmrotate 45 | pnmtopng > tilted.png'  # uses NETPBM"
END_HELP(pbm)
#endif /* TERM_HELP */
