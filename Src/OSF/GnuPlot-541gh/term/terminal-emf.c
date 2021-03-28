// Hey Emacs this is -*- C -*- 
// GNUPLOT - emf.trm 
/*[
 * Copyright 1998, 2004
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
 * This file is included by ../term.c and ../docs/termdoc.c.
 *
 * This terminal driver supports:
 *   Enhanced Metafile Format
 *
 * TODO
 *
 * HISTORY
 *
 * 5.0   19-May-2014 Karl Ratzsch
 * - Revised point types to include pentagons
 * 4.6.1 04-Sep-2012 Shige Takeno
 * - Revised order of object handling   Deselect/Delete/Create/Select
 * - Defer application of new dash+color style until needed for a new line segment
 *
 * 4.6.1 04-Sep-2012 Ethan A Merritt
 * - Fix dashed line option, but make solid lines the default
 * - Seek to start of output file after plotting so that successive plots will
 *   overwrite each other cleanly rather than producing a corrupt file.
 *
 * 4.5   28-Nov-2010 Ethan A Merritt
 * - Use the EMR_ELLIPSE primitive to draw point types 6 and 7
 * - Switch to using linecap=flat and linejoin=miter by default,
 *   but add option "rounded/butt" to toggle this
 *
 * 4.3.1 12-Sep-2008 Ethan A Merritt
 * - enhanced text mode
 * - Two variants are here. One uses the TA_UPDATECP mode to track character position.
 *   This works great horizontally, but I could not find a way to introduce a vertical
 *   offset to handle subscripts and superscripts.
 * - The second variant tracks both x and y by estimating the character width/height.
 *   This causes visible imperfections in the character spacing.
 * - Rotated enhanced text not yet supported
 *
 * 1.0.11 06-Dec-2004 Ethan A Merritt
 * - implement pThis->set_color(), pThis->filled_polygon(), and pThis->fillbox()
 *   RGB colors supported, but not yet PM3D palettes
 * 1.0.10 08-Jul-2004 Hans-Bernhard Broeker
 * - cleaned up to match gnuplot CodeStyle conventions (one line per statement,
 *   even in macro bodies, no meddling with assert()).
 * - purged K&R definitions
 * 1.0.9 03-Jun-2004 Stephane Barbaray <stephane.barbaray@compodata.com>, Ethan Merritt <merritt@u.washington.edu>
 * - fixed linewidth bug
 * - all is now really assumed as 1024x768@96dpi,
 *   before it was a mix between 1600x1200@120dpi and 1024x768@96dpi,
 *   so font may now render differently than before...
 * - pointsize rework (size twice also now)
 * - HCHAR and VCHAR are more efficiently computed
 * 1.0.8 06-May-2004 Stephane Barbaray <stephane.barbaray@compodata.com>
 * - fixed to work with MS security patch (kb835732) applied, because MS introduced bugs!!!
 * - EMR_EXTTEXTOUTW (84) is now EMR_EXTTEXTOUTA (83)
 * 1.0.7 3-Feb-2003 Ethan A Merritt
 * - modify text and point color handling to match other terminal types
 * - FIXME! alignment of rotated text is not correct.
 * 1.0.6 25-Jul-2002 Ethan A Merritt <merritt@u.washington.edu>
 * - generalized text rotation and justification
 * 1.0.5 2000/07/20
 * - Handles were not freed at all, resulting to resource leaks when viewing on Windows 9x (not on NT4/W2000!!!)
 * 1.0.4 2000/06/28
 * - Emulated dashed vectors are now looking better
 * - 15 colors * 8 pointstyles = 120 pointtypes
 * 1.0.3 2000/03/29
 * - default font is now Arial 12
 * - implemented options (color/mono,dashed/solid,font)
 * - 15 colors * 5 dashtypes = 75 linetypes
 * 1.0.2 2000/03/22
 * - Polygon and Polyline structures are not working for Windows 9X, I
 *   really don't know why, replaced with lineto/moveto couples...
 * - Texts are now displayed in GM_Compatible mode because GM_Advanced is
 *   displaying correctly but it does not print correctly with Word97!
 * - Text centering now works best according to escapement/orientation
 * - Now there is 8 colors * 5 dashtypes = 40 linetypes
 * - Successfully Working on Linux Suse 6.1 (x86)
 *
 * 1.0.1 2000/03/16
 * - Unicode text have be to long aligned in EMF files (exttextoutw)
 * - Problems with text transparence (SetBkMode was not called)
 * - Null brush created for *not* filling polygon
 *
 * 1.0.0 2000/03/15
 * - Only tested on x86 Win32
 *
 * AUTHOR
 *   Stephane Barbaray <stephane.barbaray@compodata.com>
 *   Some code based on cgm.trm
 *
 * send your comments or suggestions to (gnuplot-info@lists.sourceforge.net).
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
	register_term(emf)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void EMF_options(GpTermEntry * pThis, GnuPlot * pGp);
TERM_PUBLIC void EMF_init(GpTermEntry * pThis);
TERM_PUBLIC void EMF_reset(GpTermEntry * pThis);
TERM_PUBLIC void EMF_text(GpTermEntry * pThis);
TERM_PUBLIC void EMF_graphics(GpTermEntry * pThis);
TERM_PUBLIC void EMF_move(GpTermEntry * pThis, uint x, uint y);
TERM_PUBLIC void EMF_dashed_vector(GpTermEntry * pThis, uint ux, uint uy);
TERM_PUBLIC void EMF_solid_vector(GpTermEntry * pThis, uint ux, uint uy);
TERM_PUBLIC void EMF_linetype(GpTermEntry * pThis, int linetype);
TERM_PUBLIC void EMF_dashtype(GpTermEntry * pThis, int type, t_dashtype * custom_dash_type);
TERM_PUBLIC void EMF_linecolor(GpTermEntry * pThis, int color);
TERM_PUBLIC void EMF_load_dashtype(GpTermEntry * pThis, int dashtype);
TERM_PUBLIC void EMF_linewidth(GpTermEntry * pThis, double width);
TERM_PUBLIC void EMF_put_text(GpTermEntry * pThis, uint x, uint y, const char * str);
TERM_PUBLIC int  EMF_text_angle(GpTermEntry * pThis, int ang);
TERM_PUBLIC int  EMF_justify_text(GpTermEntry * pThis, enum JUSTIFY mode);
TERM_PUBLIC void EMF_point(GpTermEntry * pThis, uint x, uint y, int number);
TERM_PUBLIC void EMF_set_pointsize(GpTermEntry * pThis, double size);
TERM_PUBLIC int  EMF_set_font(GpTermEntry * pThis, const char *);
TERM_PUBLIC int  EMF_make_palette(GpTermEntry * pThis, t_sm_palette * palette);
TERM_PUBLIC void EMF_previous_palette(GpTermEntry * pThis);
TERM_PUBLIC void EMF_set_color(GpTermEntry * pThis, const t_colorspec * colorspec);
TERM_PUBLIC void EMF_filled_polygon(GpTermEntry * pThis, int, gpiPoint *);
TERM_PUBLIC void EMF_fillbox(GpTermEntry * pThis, int, uint, uint, uint, uint);
TERM_PUBLIC void EMF_flush_dashtype(GpTermEntry * pThis);

// Enhanced text support 
TERM_PUBLIC void ENHemf_put_text(GpTermEntry * pThis, uint x, uint y, const char * str);
TERM_PUBLIC void ENHemf_OPEN(GpTermEntry * pThis, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
TERM_PUBLIC void ENHemf_FLUSH(GpTermEntry * pThis);

#undef RGB
#define RGB(r, g, b) ((long)(((uchar)(r) | ((short)((uchar)(g)) << 8)) | (((long)(uchar)(b)) << 16)))

//#ifndef GPMIN
	//#define MIN(a, b) (a < b ? a : b)
//#endif
//#ifndef GPMAX
	//#define MAX(a, b) (a > b ? a : b)
//#endif

#define EMF_PX2HM 26.37
#define EMF_PT2HM 35.28
#define EMF_10THDEG2RAD (3.14159265359/1800)
#define EMF_XMAX (1024 * EMF_PX2HM)
#define EMF_YMAX (768 * EMF_PX2HM)
#define EMF_HTIC (EMF_XMAX / 160)
#define EMF_VTIC EMF_HTIC
#define EMF_FONTNAME "Arial"
#define EMF_FONTSIZE 12
#define EMF_HCHAR ((EMF_FONTSIZE * EMF_PT2HM) * 0.6)
#define EMF_VCHAR ((EMF_FONTSIZE * EMF_PT2HM) * 1.3)
#define EMF_LINE_TYPES 5        /* number of line types we support */
#define EMF_COLORS 15           /* number of colors we support */
#define EMF_POINTS 15           /* number of markers we support */
#define EMF_MAX_SEGMENTS 104    /* maximum # polyline coordinates */

#define EMF_HANDLE_PEN          1
#define EMF_HANDLE_FONT         2
#define EMF_HANDLE_BRUSH        3
#define EMF_HANDLE_MAX          4
/*
   typedef  enum {
   EMF_PS_COSMETIC = 0x00000000,
   EMF_PS_ENDCAP_ROUND = 0x00000000,
   EMF_PS_JOIN_ROUND = 0x00000000,
   EMF_PS_SOLID = 0x00000000,
   EMF_PS_DASH = 0x00000001,
   EMF_PS_DOT = 0x00000002,
   EMF_PS_DASHDOT = 0x00000003,
   EMF_PS_DASHDOTDOT = 0x00000004,
   EMF_PS_NULL = 0x00000005,
   EMF_PS_INSIDEFRAME = 0x00000006,
   EMF_PS_USERSTYLE = 0x00000007,
   EMF_PS_ALTERNATE = 0x00000008,
   EMF_PS_ENDCAP_SQUARE = 0x00000100,
   EMF_PS_ENDCAP_FLAT = 0x00000200,
   EMF_PS_JOIN_BEVEL = 0x00001000,
   EMF_PS_JOIN_MITER = 0x00002000,
   EMF_PS_GEOMETRIC = 0x00010000
   } EMF_PenStyle;
 */

#define EMF_STOCK_OBJECT_FLAG   ((ulong)0x1 << 31)
#define EMF_STOCK_OBJECT_WHITE_BRUSH    (EMF_STOCK_OBJECT_FLAG + 0x00)
#define EMF_STOCK_OBJECT_BLACK_PEN      (EMF_STOCK_OBJECT_FLAG + 0x07)
#define EMF_STOCK_OBJECT_DEFAULT_FONT   (EMF_STOCK_OBJECT_FLAG + 0x0A)

#define EMF_write_emr(type, size) {             \
		EMF_write_long(type);                       \
		EMF_write_long(size);                       \
		_EMF.emf_record_count++;                         \
}
#define EMF_write_sizel(width, height) {        \
		EMF_write_long(width);                      \
		EMF_write_long(height);                     \
}
#define EMF_write_points(x, y) {                \
		EMF_write_short(x);                         \
		EMF_write_short(y);                         \
}
#define EMF_write_pointl(x, y) {                \
		EMF_write_long(x);                          \
		EMF_write_long(y);                          \
}
#define EMF_write_rectl(left, top, right, bottom) {     \
		EMF_write_long(left);                               \
		EMF_write_long(top);                                \
		EMF_write_long(right);                              \
		EMF_write_long(bottom);                             \
}

#define EMF_EOF() {                             \
		EMF_write_emr(14, 0x14);                    \
		EMF_write_long(0);                          \
		EMF_write_long(0x10);                       \
		EMF_write_long(20);                         \
}
#define EMF_SetMapMode(mode) {                  \
		EMF_write_emr(17, 0x0C);                    \
		EMF_write_long(mode);                       \
}
#define EMF_SetWindowExtEx(width, height) {     \
		EMF_write_emr(9, 0x10);                     \
		EMF_write_sizel(width, height);             \
}
#define EMF_SetWindowOrgEx(width, height) {     \
		EMF_write_emr(10, 0x10);                    \
		EMF_write_sizel(width, height);             \
}
#define EMF_SetViewportExtEx(width, height) {   \
		EMF_write_emr(11, 0x10);                    \
		EMF_write_sizel(width, height);             \
}
#define EMF_SetViewportOrgEx(width, height) {   \
		EMF_write_emr(12, 0x10);                    \
		EMF_write_sizel(width, height);             \
}
#define EMF_SetTextColor(color) {               \
		EMF_write_emr(24, 0x0C);                    \
		EMF_write_long(color);                      \
}
#define EMF_MoveToEx(x, y) {                     \
		EMF_write_emr(27, 0x10);                    \
		EMF_write_pointl(x, y);                     \
}
#define EMF_LineTo(x, y) {                       \
		EMF_write_emr(54, 0x10);                    \
		EMF_write_pointl(x, y);                     \
}
#define EMF_CreatePen(handle, type, width, color) {     \
		EMF_write_emr(38, 0x1C);                            \
		EMF_write_long(handle);                             \
		EMF_write_long(type);                               \
		EMF_write_long(width);                              \
		EMF_write_long(0);                                  \
		EMF_write_long(color);                              \
}
#define EMF_CreateBrush(handle, type, color, hatch) {   \
		EMF_write_emr(39, 0x18);                            \
		EMF_write_long(handle);                             \
		EMF_write_long(type);                               \
		EMF_write_long(color);                              \
		EMF_write_long(hatch);                              \
}
#define EMF_SelectObject(handle) {              \
		EMF_write_emr(37, 0x0C);                    \
		EMF_write_long(handle);                     \
}
#define EMF_DeleteObject(handle) {              \
		EMF_write_emr(40, 0x0C);                    \
		EMF_write_long(handle);                     \
}
#define EMF_Ellipse(left, top, right, bottom)  {   \
		EMF_write_emr(42, 0x18);                    \
		EMF_write_rectl(left, top, right, bottom)      \
}
#define EMF_SetTextAlign(align) {               \
		EMF_write_emr(22, 0x0C);                    \
		EMF_write_long(align);                      \
}
#define EMF_SetBkMode(mode) {                   \
		EMF_write_emr(18, 0x0C);                    \
		EMF_write_long(mode);                       \
}
#define EMF_SaveDC() {                          \
		EMF_write_emr(33, 0x0C);                    \
		EMF_write_long(0);                          \
}
#define EMF_RestoreDC() {                       \
		EMF_write_emr(34, 0x0C);                    \
		EMF_write_long(1);                          \
}
#define EMF_CreatePolygon(nvert) {              \
		EMF_write_emr(3, (7+2*nvert)*4);            \
		EMF_write_rectl(0, 0, 0, 0); /* Bounds */     \
		EMF_write_long(nvert);                      \
}

// shige 
// Write the EMR, the header, and a single-entry colormap 
#define EMF_CreateMonoBrush(handle) {   \
		EMF_write_emr(93, 0x6c);    \
		EMF_write_long(handle);     \
		EMF_write_long(2); /* DIB_PAL_INDICES = use current color */ \
		EMF_write_long(0x24); /* offset to DIB header */ \
		EMF_write_long(0x28); /* size of DIB header */ \
		EMF_write_long(0x4c); /* offset to DIB bits */ \
		EMF_write_long(0x20); /* size of DIB bits */ \
		EMF_write_long(0x20000000); /* start of DIB header */ \
		EMF_write_long(0x28);        \
		EMF_write_sizel(16, 8); /* width, height */ \
		EMF_write_short(1); /* must be 1 */ \
		EMF_write_short(1); /* bits per pixel */ \
		EMF_write_long(0); /* compression type */ \
		EMF_write_long(32); /* size of image in bytes */ \
		EMF_write_long(0); /* x pixels per meter */ \
		EMF_write_long(0); /* y pixels per meter */ \
		EMF_write_long(0); /* # entries in color table */ \
		EMF_write_long(0); /* # color table entries used */ \
}

//#endif /* TERM_PROTO */

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY

//#include <ctype.h> // for isspace() 
#ifdef HAVE_ICONV
	#include <iconv.h>
#endif

struct GpEMF_TerminalBlock {
	GpEMF_TerminalBlock() : /*emf_posx(0), emf_posy(0),*/emf_record_count(0), emf_linetype(1), emf_dashtype(0), emf_color(0),
		emf_textcolor(LT_UNDEFINED), emf_pentype(0x2200), emf_graphics(FALSE), emf_dashed(TRUE), emf_monochrome(FALSE),
		emf_background(0xffffff), emf_linewidth(0.0), emf_linewidth_factor(1.0), emf_dashlength(1.0), emf_coords(0),
		emf_fontsize(EMF_FONTSIZE), emf_last_fontsize(-1.0f), emf_last_fontname(0), emf_justify(LEFT), emf_defaultfontsize(EMF_FONTSIZE),
		emf_vert_text(0), emf_step_index(0), emf_step(0), emf_tic(0), emf_tic707(0), emf_tic866(0), emf_tic500(0), emf_tic1241(0), emf_tic1077(0),
		emf_tic621(0), emf_tic9511(0), emf_tic5878(0), emf_tic8090(0), emf_tic3090(0), emf_tweak(true), emf_fontscale(1.0), emf_dashtype_count(0)
	{
		MEMSZERO(emf_polyline);
		STRNSCPY(emf_fontname, EMF_FONTNAME);
		STRNSCPY(emf_defaultfontname, EMF_FONTNAME);
		MEMSZERO(emf_step_sizes);
		MEMSZERO(emf_dashpattern);
	}
	//int    emf_posx;
	//int    emf_posy;
	SPoint2I Pos;
	int    emf_record_count;
	int    emf_linetype;
	int    emf_dashtype;
	long   emf_color;
	long   emf_textcolor;
	ulong  emf_pentype;           // cap=flat join=miter 
	uint   emf_polyline[EMF_MAX_SEGMENTS]; // stored polyline coordinates 
	uint   emf_graphics;
	uint   emf_dashed;
	uint   emf_monochrome;
	uint   emf_background; // defaults to white 
	double emf_linewidth; // line width in plot units 
	double emf_linewidth_factor;
	double emf_dashlength;
	int    emf_coords; // # polyline coordinates saved 
	char   emf_fontname[255];
	float  emf_fontsize;
	float  emf_last_fontsize;
	char * emf_last_fontname;
	enum   JUSTIFY emf_justify;
	char   emf_defaultfontname[255];
	float  emf_defaultfontsize;
	int    emf_vert_text; // text orientation -- nonzero for vertical 
	int    emf_step_sizes[8]; // array of currently used dash lengths in plot units 
	int    emf_step_index; // index into emf_step_sizes[] 
	int    emf_step;       // amount of current dash not yet drawn, in plot units 
	int    emf_tic;
	int    emf_tic707;
	int    emf_tic866;
	int    emf_tic500;
	int    emf_tic1241;
	int    emf_tic1077;
	int    emf_tic621;
	int    emf_tic9511;
	int    emf_tic5878;
	int    emf_tic8090;
	int    emf_tic3090; // marker dimensions 
	bool   emf_tweak; // Empirical hack to adjust character widths 
	double emf_fontscale;
	int    emf_dashtype_count; // count > 0 if EMF_load_dashtype needed before drawing 
	int    emf_dashpattern[8]; // filled by EMF_dashtype 
};

static GpEMF_TerminalBlock _EMF;

//static int emf_posx;
//static int emf_posy;
//static int emf_record_count = 0;
//static int emf_linetype = 1;
//static int emf_dashtype = 0;
//static long emf_color = 0L;
//static long emf_textcolor = LT_UNDEFINED;
//static ulong emf_pentype = 0x2200;              /* cap=flat join=miter */
//static uint emf_polyline[EMF_MAX_SEGMENTS];     /* stored polyline coordinates */
//static uint emf_graphics = FALSE;
//static uint emf_dashed = TRUE;
//static uint emf_monochrome = FALSE;
//static uint emf_background = 0xffffff; /* defaults to white */
//static double emf_linewidth;    /* line width in plot units */
//static double emf_linewidth_factor = 1.0;
//static double emf_dashlength = 1.0;
//static int emf_coords = 0;      /* # polyline coordinates saved */
//static char emf_fontname[255] = EMF_FONTNAME;
//static float emf_fontsize = EMF_FONTSIZE;
//static float emf_last_fontsize = -1;
//static char * emf_last_fontname = NULL;
//static enum JUSTIFY emf_justify = LEFT;
//static char emf_defaultfontname[255] = EMF_FONTNAME;
//static float emf_defaultfontsize = EMF_FONTSIZE;
//static int emf_vert_text = 0;   /* text orientation -- nonzero for vertical */
//static int emf_step_sizes[8];   /* array of currently used dash lengths in plot units */
//static int emf_step_index = 0;  /* index into emf_step_sizes[] */
//static int emf_step = 0;        /* amount of current dash not yet drawn, in plot units */
//static int emf_tic;
//static int emf_tic707;
//static int emf_tic866;
//static int emf_tic500;
//static int emf_tic1241;
//static int emf_tic1077;
//static int emf_tic621;
//static int emf_tic9511;
//static int emf_tic5878;
//static int emf_tic8090;
//static int emf_tic3090; // marker dimensions 
//static bool emf_tweak = TRUE;   /* Empirical hack to adjust character widths */
//static double emf_fontscale = 1.0;
//static int emf_dashtype_count = 0;      /* count > 0 if EMF_load_dashtype needed before drawing */
//static int emf_dashpattern[8];          /* filled by EMF_dashtype */

// shige: hatch pattern (from src/win/wgraph.c) 
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
};
#define pattern_num (sizeof(pattern_bitmaps)/(sizeof(*pattern_bitmaps)))

static void EMF_flush_polyline(GpTermEntry * pThis);
//static void EMF_flush_polygon();
static void FASTCALL EMF_write_byte(int);
static void FASTCALL EMF_write_short(int);
static void FASTCALL EMF_write_long(ulong);
static void EMF_write_float(double);
static void EMF_setfont();

#define ANSI_CHARSET          0
#define DEFAULT_CHARSET       1
#define CHINESEBIG5_CHARSET 136
#define GREEK_CHARSET       161
#define TURKISH_CHARSET     162
#define BALTIC_CHARSET      186
#define RUSSIAN_CHARSET     204
#define EASTEUROPE_CHARSET  238
#define KOI8_CHARSET        242

/* Text alignment */
#define GP_TA_NOUPDATECP       0x00
#define GP_TA_UPDATECP         0x01
#define GP_TA_LEFT             0x00
#define GP_TA_RIGHT            0x02
#define GP_TA_CENTER           0x06
#define GP_TA_TOP              0x00
#define GP_TA_BOTTOM           0x08
#define GP_TA_BASELINE         0x18

/* ExtTextOut options */
#if 0
#define ETO_NO_RECT        0x100
#define ETO_PDY           0x2000
#endif

static void EMF_setfont()
{
	int i, count;
	int bold = 400;
	char italic = 0, underline = 0, strikeout = 0;
	char font[32];
	char * sub;
	if(!_EMF.emf_graphics)
		return;
	count = MIN(strlen(_EMF.emf_fontname), 31);
	if(((sub = strstr(_EMF.emf_fontname, " bold")) != NULL) || ((sub = strstr(_EMF.emf_fontname, " Bold")) != NULL)) {
		bold = 700;
		count = MIN(sub - _EMF.emf_fontname, count);
	}
	if(((sub = strstr(_EMF.emf_fontname, " italic")) != NULL) || ((sub = strstr(_EMF.emf_fontname, " Italic")) != NULL)) {
		italic = 1;
		count = MIN(sub - _EMF.emf_fontname, count);
	}
	if(((sub = strstr(_EMF.emf_fontname, " underline")) != NULL) || ((sub = strstr(_EMF.emf_fontname, " Underline")) != NULL)) {
		underline = 1;
		count = MIN(sub - _EMF.emf_fontname, count);
	}
	if(((sub = strstr(_EMF.emf_fontname, " strikeout")) != NULL) || ((sub = strstr(_EMF.emf_fontname, " Strikeout")) != NULL) || ((sub = strstr(_EMF.emf_fontname, " StrikeOut")) != NULL)) {
		strikeout = 1;
		count = MIN(sub - _EMF.emf_fontname, count);
	}
	strnzcpy(font, _EMF.emf_fontname, count + 1);
	EMF_SelectObject(EMF_STOCK_OBJECT_DEFAULT_FONT);
	EMF_DeleteObject(EMF_HANDLE_FONT);
	// SB 20040506: was not complete size was 104, now it is 332 
	EMF_write_emr(82, 332);
	EMF_write_long(EMF_HANDLE_FONT);
	EMF_write_long((long)(-_EMF.emf_fontsize * EMF_PT2HM * _EMF.emf_fontscale)); /* height */
	EMF_write_long(0);              /* width */
	EMF_write_long(_EMF.emf_vert_text);  /* escapement */
	EMF_write_long(_EMF.emf_vert_text);  /* orientation */
	EMF_write_long(bold);           /* weight */
	EMF_write_byte(italic);         /* italic */
	EMF_write_byte(underline);      /* underline */
	EMF_write_byte(strikeout);      /* strikeout */
	// charset: could be extended? 
	switch(encoding) {
		case S_ENC_CP1250:
		case S_ENC_ISO8859_2:
		    EMF_write_byte(EASTEUROPE_CHARSET);
		    break;
		case S_ENC_KOI8_R:
		case S_ENC_KOI8_U:
		    EMF_write_byte(KOI8_CHARSET);
		    break;
		case S_ENC_CP1254:
		case S_ENC_ISO8859_9:
		    EMF_write_byte(TURKISH_CHARSET);
		    break;
		case S_ENC_CP950:
		    EMF_write_byte(CHINESEBIG5_CHARSET);
		    break;
		default:
		    EMF_write_byte(DEFAULT_CHARSET);
	}
	EMF_write_byte(0); // out precision 
	EMF_write_byte(0); // clip precision 
	EMF_write_byte(0); // quality 
	EMF_write_byte(0); // pitch and family 
	for(i = 0; i < 32; i++) {
		// face name (max 32) 
		EMF_write_byte((char)(i < sstrleni(font) ? font[i] : 0));
		EMF_write_byte(0);
	}
	// SB 20040506: modification following 
	for(i = 0; i < 64; i++) {
		// FULL face name (max 64) 
		EMF_write_byte((char)(i < sstrleni(font) ? font[i] : 0));
		EMF_write_byte(0);
	}
	for(i = 0; i < 32; i++) {
		// style name (max 32) 
		EMF_write_byte(0);
		EMF_write_byte(0);
	}
	EMF_write_long(0);      /* version */
	EMF_write_long(0);      /* Style size */
	EMF_write_long(0);      /* Match */
	EMF_write_long(0);      /* reserved */
	EMF_write_long(0);      /* VendorId */
	EMF_write_long(0);      /* Culture */
	for(i = 0; i < 10; i++)
		EMF_write_byte(0); /* Panose (ignored) */
	EMF_write_byte(0);      /* pad (long aligned) */
	EMF_write_byte(0);      /* pad (long aligned) */
	/* SB 20040506: End of modification */

	EMF_SelectObject(EMF_HANDLE_FONT);
}

static void EMF_flush_polygon(GpTermEntry * pThis)
{
	int i = 0;
	if(_EMF.emf_coords > 0) {
		EMF_flush_dashtype(pThis);
		EMF_MoveToEx(_EMF.emf_polyline[i++], pThis->MaxY - _EMF.emf_polyline[i++]);
		while(i < _EMF.emf_coords * 2)
			EMF_LineTo(_EMF.emf_polyline[i++], pThis->MaxY - _EMF.emf_polyline[i++]);
		EMF_LineTo(_EMF.emf_polyline[0], pThis->MaxY - _EMF.emf_polyline[1]);
		_EMF.emf_coords = 0;
	}
}

static void EMF_flush_polyline(GpTermEntry * pThis)
{
	if(_EMF.emf_coords == 0)
		return;
	EMF_flush_dashtype(pThis);
	if(_EMF.emf_coords <= 2) {
		EMF_MoveToEx(_EMF.emf_polyline[0], pThis->MaxY - _EMF.emf_polyline[1]);
		EMF_LineTo(_EMF.emf_polyline[2], pThis->MaxY - _EMF.emf_polyline[3]);
	}
	else {
		int i = 0;
		EMF_MoveToEx(_EMF.emf_polyline[i++], pThis->MaxY - _EMF.emf_polyline[i++]);
		while(i < _EMF.emf_coords * 2)
			EMF_LineTo(_EMF.emf_polyline[i++], pThis->MaxY - _EMF.emf_polyline[i++]);
	}
	_EMF.emf_coords = 0;
}
//
// HBB 20040708: the following keep K&R argument types for now 
//
static void FASTCALL EMF_write_byte(int value)
{
	char c = value;
	fwrite(&c, 1, 1, gpoutfile);
}

static void FASTCALL EMF_write_short(int value)
{
	short actual_value = value;
	char c[2];
	c[1] = (actual_value >> 8) & 255; /* convert to x86 order */
	c[0] = actual_value & 255;
	fwrite(c, 1, 2, gpoutfile);
}

static void FASTCALL EMF_write_long(ulong value)
{
	char c[4];
	c[3] = (value >> 24) & 0xFFL;   /* convert to x86 order */
	c[2] = (value >> 16) & 0xFFL;
	c[1] = (value >> 8) & 0xFFL;
	c[0] = value & 0xFFL;
	fwrite(c, 1, 4, gpoutfile);
}
//
// FIXME HBB 20001103: this only works as given iff 'float' is the
// same format as on x86's, i.e. IEEE 4-byte floating point format 
//
static void EMF_write_float(double value)
{
	char c[4];
	union {
		long l;
		float f;
	} u;
	u.f = static_cast<float>(value);
	c[3] = (u.l >> 24) & 0xFFL; /* convert to x86 order */
	c[2] = (u.l >> 16) & 0xFFL;
	c[1] = (u.l >> 8) & 0xFFL;
	c[0] = u.l & 0xFFL;
	fwrite(c, 1, 4, gpoutfile);
}

TERM_PUBLIC void EMF_options(GpTermEntry * pThis, GnuPlot * pGp)
{
	char * s;
	int emf_bgnd_rgb = 0;
	float new_defaultfontsize = _EMF.emf_defaultfontsize;
	// Annoying hack to handle the case of 'set termoption' after 
	// we have already initialized the terminal.                  
	if(!pGp->Pgm.AlmostEquals(pGp->Pgm.GetPrevTokenIdx(), "termopt$ion")) {
		pThis->MaxX = static_cast<uint>(EMF_XMAX);
		pThis->MaxY = static_cast<uint>(EMF_YMAX);
		_EMF.emf_monochrome = FALSE;
		_EMF.emf_background = 0xffffff;
		_EMF.emf_tweak = TRUE;
		// Default to enhanced text 
		pThis->put_text = ENHemf_put_text;
		pThis->flags |= TERM_ENHANCED_TEXT;
	}
	while(!pGp->Pgm.EndOfCommand()) {
		if(pGp->Pgm.AlmostEqualsCur("de$fault")) {
			strcpy(_EMF.emf_defaultfontname, EMF_FONTNAME);
			_EMF.emf_defaultfontsize = EMF_FONTSIZE;
			_EMF.emf_monochrome = FALSE;
			pThis->flags &= ~TERM_MONOCHROME;
			pGp->Pgm.Shift();
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("m$onochrome")) {
			_EMF.emf_monochrome = TRUE;
			pThis->flags |= TERM_MONOCHROME;
			pGp->Pgm.Shift();
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("c$olor") || pGp->Pgm.AlmostEqualsCur("c$olour")) {
			_EMF.emf_monochrome = FALSE;
			pThis->flags &= ~TERM_MONOCHROME;
			pGp->Pgm.Shift();
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("da$shed") || pGp->Pgm.AlmostEqualsCur("s$olid")) {
			// dashed lines always enabled in version 5 
			pGp->Pgm.Shift();
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("round$ed")) {
			_EMF.emf_pentype = 0x0;
			pGp->Pgm.Shift();
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("butt")) {
			_EMF.emf_pentype = 0x2200;
			pGp->Pgm.Shift();
			continue;
		}
		if(pGp->Pgm.EqualsCur("dl") || pGp->Pgm.AlmostEqualsCur("dashl$ength")) {
			pGp->Pgm.Shift();
			_EMF.emf_dashlength = pGp->RealExpression();
			if(_EMF.emf_dashlength < 0.5)
				_EMF.emf_dashlength = 1.0;
			continue;
		}
		if(pGp->Pgm.EqualsCur("lw") || pGp->Pgm.AlmostEqualsCur("linew$idth")) {
			pGp->Pgm.Shift();
			_EMF.emf_linewidth_factor = pGp->RealExpression();
			if(_EMF.emf_linewidth_factor < 0.1)
				_EMF.emf_linewidth_factor = 1.0;
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("enh$anced")) {
			pGp->Pgm.Shift();
			pThis->put_text = ENHemf_put_text;
			pThis->flags |= TERM_ENHANCED_TEXT;
			continue;
		}
		else if(pGp->Pgm.AlmostEqualsCur("noenh$anced")) {
			pGp->Pgm.Shift();
			pThis->put_text = EMF_put_text;
			pThis->flags &= ~TERM_ENHANCED_TEXT;
		}

		if(pGp->Pgm.AlmostEqualsCur("back$ground")) {
			pGp->Pgm.Shift();
			emf_bgnd_rgb = pGp->ParseColorName();
			_EMF.emf_background = RGB((emf_bgnd_rgb>>16)&0xFF, (emf_bgnd_rgb>>8)&0xFF, emf_bgnd_rgb&0xFF);
		}

		if(pGp->Pgm.AlmostEqualsCur("nopro$portional")) {
			pGp->Pgm.Shift();
			_EMF.emf_tweak = FALSE;
		}
		if(pGp->Pgm.AlmostEqualsCur("si$ze")) {
			int tempxmax = 1024;
			int tempymax = 768;
			pGp->Pgm.Shift();
			if(!pGp->Pgm.EndOfCommand()) {
				tempxmax = static_cast<int>(pGp->RealExpression());
				if(pGp->Pgm.EqualsCur(",")) {
					pGp->Pgm.Shift();
					tempymax = static_cast<int>(pGp->RealExpression());
				}
			}
			if(tempxmax > 0)
				pThis->MaxX = static_cast<uint>(tempxmax * EMF_PX2HM);
			if(tempymax > 0)
				pThis->MaxY = static_cast<uint>(tempymax * EMF_PX2HM);
			pThis->TicH = pThis->MaxX / 160;
			pThis->TicV = pThis->TicH;
			continue;
		}
		if(pGp->Pgm.EqualsCur("fontscale")) {
			pGp->Pgm.Shift();
			_EMF.emf_fontscale = pGp->Pgm.EndOfCommand() ? -1 : pGp->RealExpression();
			if(_EMF.emf_fontscale <= 0)
				_EMF.emf_fontscale = 1.0;
			continue;
		}
		if(pGp->Pgm.EqualsCur("font"))
			pGp->Pgm.Shift();
		// @fallthrough to old-style bare font name 
		if((s = pGp->TryToGetString())) {
			char * comma = strrchr(s, ',');
			if(comma && (1 == sscanf(comma+1, "%f", &new_defaultfontsize))) {
				*comma = '\0';
			}
			if(*s)
				strnzcpy(_EMF.emf_defaultfontname, s, sizeof(_EMF.emf_defaultfontname));
			SAlloc::F(s);
			if(pGp->Pgm.IsANumber(pGp->Pgm.GetCurTokenIdx()))
				new_defaultfontsize = pGp->FloatExpression();
			continue;
		}
		break;
	} /* while(!end of command) */
	if(!pGp->Pgm.EndOfCommand()) {
		/* We have old-style bare font size specified */
		new_defaultfontsize = pGp->FloatExpression();
	}
	if(new_defaultfontsize > 0)
		_EMF.emf_defaultfontsize = new_defaultfontsize;
	sprintf(term_options, "%s %s font \"%s,%g\"", _EMF.emf_monochrome ? "monochrome" : "color", _EMF.emf_pentype ? "butt" : "rounded", _EMF.emf_defaultfontname, _EMF.emf_defaultfontsize);
	if(pThis->flags & TERM_ENHANCED_TEXT)
		strcat(term_options, " enhanced ");
	if(_EMF.emf_fontscale != 1.0)
		sprintf(&(term_options[strlen(term_options)]), " fontscale %.1f", _EMF.emf_fontscale);
	if(pThis->MaxX != (int)EMF_XMAX || pThis->MaxY != (int)EMF_YMAX)
		sprintf(&(term_options[strlen(term_options)]), " size %d,%d ", (int)(0.5+pThis->MaxX/EMF_PX2HM), (int)(0.5+pThis->MaxY/EMF_PX2HM));
	if(_EMF.emf_linewidth_factor != 1.0)
		sprintf(&(term_options[strlen(term_options)]), " lw %.1f", _EMF.emf_linewidth_factor);
	if(_EMF.emf_dashlength != 1.0)
		sprintf(&(term_options[strlen(term_options)]), " dashlength %.1f", _EMF.emf_dashlength);
	if(emf_bgnd_rgb)
		sprintf(&(term_options[strlen(term_options)]), " background \"#%06x\"", emf_bgnd_rgb);
}

TERM_PUBLIC void EMF_init(GpTermEntry * pThis)
{
	_EMF.Pos.Z();
	_EMF.emf_linetype = 0;
	_EMF.emf_vert_text = 0;
	_EMF.emf_graphics = FALSE;
}

TERM_PUBLIC void EMF_graphics(GpTermEntry * pThis)
{
	int width    = static_cast<int>(0.5 + pThis->MaxX/EMF_PX2HM);
	int height   = static_cast<int>(0.5 + pThis->MaxY/EMF_PX2HM);
	int mmwidth  = static_cast<int>(0.5 + (pThis->MaxX/EMF_PX2HM) * (270.0/1024.0));
	int mmheight = static_cast<int>(0.5 + (pThis->MaxY/EMF_PX2HM) * (200.0/768.0));
	// header start 
	_EMF.emf_record_count = 0;
	EMF_write_emr(1, 100);
	EMF_write_long(0);      /* rclBounds */
	EMF_write_long(0);
	EMF_write_long(static_cast<ulong>(pThis->MaxX / EMF_PX2HM));
	EMF_write_long(static_cast<ulong>(pThis->MaxY / EMF_PX2HM));
	EMF_write_long(0);      /* rclFrame */
	EMF_write_long(0);
	EMF_write_long(pThis->MaxX);
	EMF_write_long(pThis->MaxY);
	EMF_write_long(0x464D4520); /* signature */
	EMF_write_long(0x00010000); /* version */
	EMF_write_long(0);      /* nBytes */
	EMF_write_long(0);      /* nRecords */
	EMF_write_short(EMF_HANDLE_MAX); /* nHandles, MUST NOT BE 0 */
	EMF_write_short(0);     /* reserved */
	EMF_write_long(0);      /* descSize */
	EMF_write_long(0);      /* descOff */
	EMF_write_long(0);      /* nPalEntries */
	EMF_write_long(width);  /* ref dev pixwidth, default 1024 */
	EMF_write_long(height); /* ref dev pixheight, default 768 */
	EMF_write_long(mmwidth); /* ref dev mwidth, default 270 */
	EMF_write_long(mmheight); /* ref dev mheight, default 200 */
	EMF_write_long(0);      /* cbPixelFormat  */
	EMF_write_long(0);      /* offPixelFormat  */
	EMF_write_long(0);      /* bOpenGL */
	_EMF.emf_graphics = TRUE;
	// header end 
	EMF_SetMapMode(8);      /* forcing anisotropic mode */
	EMF_SetWindowExtEx(pThis->MaxX, pThis->MaxY);     /* setting logical (himetric) size      */
	EMF_SetViewportExtEx(pThis->MaxX / EMF_PX2HM, pThis->MaxY / EMF_PX2HM);   /* setting device (pixel) size */
	// Paint with background color 
	if(_EMF.emf_background != 0xffffff)
		EMF_fillbox(pThis, FS_EMPTY, 0, 0, pThis->MaxX, pThis->MaxY);
	EMF_CreatePen(EMF_HANDLE_PEN, _EMF.emf_pentype, 1, 0x000000); /* init default pen */
	EMF_SelectObject(EMF_HANDLE_PEN);
	EMF_SetBkMode(1);       /* transparent background for text */
	EMF_CreateBrush(EMF_HANDLE_BRUSH, 1, 0, 0);     /* transparent brush for polygons */
	EMF_SelectObject(EMF_HANDLE_BRUSH);
	SAlloc::F(_EMF.emf_last_fontname); /* invalidate any previous font */
	_EMF.emf_last_fontname = NULL;
	EMF_set_font(pThis, NULL); // init default font 
	_EMF.emf_color = _EMF.emf_textcolor = LT_UNDEFINED;
}

TERM_PUBLIC int EMF_set_font(GpTermEntry * pThis, const char * font)
{
	// FIXME: This condition is somehow triggered by enhanced_recursion 
	if(font == _EMF.emf_fontname)
		;
	else if(font && *font) {
		float tempsize;
		int sep = strcspn(font, ",");
		if(sep > 0)
			strnzcpy(_EMF.emf_fontname, font, MIN(sep + 1, 32));
		if(sep < sstrleni(font) && sscanf(font+sep+1, "%f", &tempsize))
			_EMF.emf_fontsize = tempsize;
	}
	else {
		strcpy(_EMF.emf_fontname, _EMF.emf_defaultfontname);
		_EMF.emf_fontsize = _EMF.emf_defaultfontsize;
	}
	// Skip redundant requests for the same font 
	if(_EMF.emf_last_fontname && sstreq(_EMF.emf_last_fontname, _EMF.emf_fontname) && _EMF.emf_last_fontsize == _EMF.emf_fontsize) {
		return TRUE;
	}
	else {
		SAlloc::F(_EMF.emf_last_fontname);
		_EMF.emf_last_fontname = sstrdup(_EMF.emf_fontname);
		_EMF.emf_last_fontsize = _EMF.emf_fontsize;
	}
	pThis->ChrH = static_cast<uint>(0.6 * (_EMF.emf_fontsize * EMF_PT2HM * _EMF.emf_fontscale));
	pThis->ChrV = static_cast<uint>(1.3 * (_EMF.emf_fontsize * EMF_PT2HM * _EMF.emf_fontscale));
	EMF_setfont();
	return TRUE;
}

TERM_PUBLIC void EMF_text(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	long pos;
	EMF_flush_polyline(pThis);
	_EMF.emf_graphics = FALSE;
	// shige: 08/30 2012
	// FIXME:
	// The following command prevents export of a spurious rectangle
	// (not the bounding box) on some Windows systems.  Why? How?
	EMF_MoveToEx(_EMF.emf_polyline[0], pThis->MaxY - _EMF.emf_polyline[1]);
	// writing end of metafile 
	EMF_SelectObject(EMF_STOCK_OBJECT_DEFAULT_FONT);
	EMF_DeleteObject(EMF_HANDLE_FONT);
	EMF_SelectObject(EMF_STOCK_OBJECT_BLACK_PEN);
	EMF_DeleteObject(EMF_HANDLE_PEN);
	EMF_SelectObject(EMF_STOCK_OBJECT_WHITE_BRUSH);
	EMF_DeleteObject(EMF_HANDLE_BRUSH);
	EMF_EOF();
	// update the header 
	pos = static_cast<long>(ftell(gpoutfile));
	if(pos < 0) {
		p_gp->TermGraphics = false;
		p_gp->IntError(NO_CARET, "emf: cannot reset output file");
	}
	else {
		fseek(gpoutfile, 48L, SEEK_SET);
		EMF_write_long(pos);
		EMF_write_long(_EMF.emf_record_count);
	}
	// Reset to start of output file.  If the user mistakenly tries to	
	// plot again into the same file, it will overwrite the original	
	// rather than corrupting it.					
	// FIXME:  An alternative would be to open a new output file.   
	fseek(gpoutfile, 0L, SEEK_SET);
}

TERM_PUBLIC void EMF_linetype(GpTermEntry * pThis, int linetype)
{
	EMF_flush_polyline(pThis);
	if(linetype == LT_NODRAW)
		EMF_load_dashtype(pThis, DASHTYPE_NODRAW);
	else
		EMF_linecolor(pThis, linetype);
	_EMF.emf_linetype = linetype;
	if(linetype == LT_SOLID)
		EMF_load_dashtype(pThis, 0);
	if(linetype == LT_AXIS)
		EMF_load_dashtype(pThis, 2);
}

TERM_PUBLIC void EMF_dashtype(GpTermEntry * pThis, int type, t_dashtype * custom_dash_type)
{
	int i;
	switch(type) {
		case DASHTYPE_SOLID:
		    EMF_load_dashtype(pThis, 0);
		    break;
		case DASHTYPE_CUSTOM:
		    for(i = 0; i < 8; i++)
			    _EMF.emf_dashpattern[i] = static_cast<int>(custom_dash_type->pattern[i]);
		    EMF_load_dashtype(pThis, DASHTYPE_CUSTOM);
		    break;
		default:
		    EMF_load_dashtype(pThis, type);
		    break;
	}
}

TERM_PUBLIC void EMF_linecolor(GpTermEntry * pThis, int linecolor)
{
	static long color_table_data[] =
	{
		RGB(255, 0, 0), /* red */
		RGB(0, 255, 0), /* green */
		RGB(0, 0, 255), /* blue */
		RGB(255, 0, 255), /* magenta */
		RGB(0, 0, 128), /* dark blue */
		RGB(128, 0, 0), /* dark red */
		RGB(0, 128, 128), /* dark cyan */
		RGB(0, 0, 0),   /* black */
		RGB(128, 128, 128), /* grey */
		RGB(0, 128, 64), /* very dark cyan */
		RGB(128, 128, 0), /* dark yellow */
		RGB(128, 0, 128), /* dark magenta */
		RGB(192, 192, 192), /* light grey */
		RGB(0, 255, 255), /* cyan */
		RGB(255, 255, 0) /* yellow */
	};
	if(linecolor == LT_BACKGROUND)
		_EMF.emf_color = _EMF.emf_background;
	else {
		linecolor = (linecolor < 0 || _EMF.emf_monochrome) ? 7 : (linecolor % EMF_COLORS);
		_EMF.emf_color = color_table_data[linecolor];
	}
	EMF_flush_polyline(pThis);
}

TERM_PUBLIC int EMF_make_palette(GpTermEntry * pThis, t_sm_palette * palette)
{
	return 0; // can do continuous colors 
}

TERM_PUBLIC void EMF_previous_palette(GpTermEntry * pThis)
{
	// do nothing 
}

TERM_PUBLIC void EMF_set_color(GpTermEntry * pThis, const t_colorspec * colorspec)
{
	GnuPlot * p_gp = pThis->P_Gp;
	rgb255_color rgb255;
	EMF_flush_polyline(pThis);
	if(colorspec->type == TC_LT) {
		EMF_linecolor(pThis, colorspec->lt);
	}
	else if(colorspec->type == TC_FRAC) {
		p_gp->Rgb255MaxColorsFromGray(colorspec->value, &rgb255);
		_EMF.emf_color = RGB(rgb255.r, rgb255.g, rgb255.b);
	}
	else if(colorspec->type == TC_RGB) {
		_EMF.emf_color = RGB(colorspec->lt >> 16 & 0xff, colorspec->lt >> 8 & 0xff, colorspec->lt & 0xff);
	}
	/*
	   else {
	    fprintf(stderr, "unhandled colorspec type %d\n", colorspec->type);
	   }
	 */
	// Force reevaluation of dash type 
	_EMF.emf_dashtype_count++;
}

TERM_PUBLIC void EMF_filled_polygon(GpTermEntry * pThis, int points, gpiPoint * corners)
{
	int i;
	ulong color = _EMF.emf_color;
	int fillpar = corners->style >> 4;
	int style = corners->style & 0xf;
	switch(style) {
		case FS_EMPTY: // fill with background color 
		    color = _EMF.emf_background;
		    break;
		case FS_PATTERN:
		case FS_TRANSPARENT_PATTERN:
#if 1 /* fill pattern implemented as bitmaps, implementation further down */
		    break;
#else
		    /* pattern fill implemented as partial density */
		    fillpar *= 12;
		    // @fallthrough
#endif
		case FS_SOLID: /* solid fill */
		    if(fillpar >= 0 && fillpar < 100) {
			    double density = (double)fillpar / 100.0;
			    color = ((int)((double)((_EMF.emf_color>>16)&0xff)*density) << 16) + ((int)((double)((_EMF.emf_color>>8)&0xff)*density) << 8) + ((int)((double)(_EMF.emf_color&0xff)*density));
			    color += ((int)(255.*(1.-density)) << 16) + ((int)(255.*(1.-density)) << 8) + ((int)(255.*(1.-density)));
		    }
		    break;
		default:
		    break;
	}
	EMF_flush_dashtype(pThis);
	// MS documentation says not to delete an object while it is selected 
	EMF_SelectObject(EMF_STOCK_OBJECT_BLACK_PEN);
	EMF_SelectObject(EMF_STOCK_OBJECT_WHITE_BRUSH);
	EMF_DeleteObject(EMF_HANDLE_BRUSH);
	if(oneof2(style, FS_PATTERN, FS_TRANSPARENT_PATTERN)) {
		// Implementation of bitmapped pattern fill 
		const uchar * pattern = pattern_bitmaps[fillpar % pattern_num];
		_EMF.emf_textcolor = color;
		EMF_SetTextColor(_EMF.emf_textcolor);
		EMF_CreateMonoBrush(EMF_HANDLE_BRUSH);
		for(i = (PATTERN_BITMAP_LENGTH - 1); i >= 1; i -= 2)
			EMF_write_long(pattern[i] | (pattern[i-1] << 8));
	}
	else
		EMF_CreateBrush(EMF_HANDLE_BRUSH, 0, color, 0);
	EMF_SelectObject(EMF_HANDLE_BRUSH);
	EMF_DeleteObject(EMF_HANDLE_PEN);
	EMF_CreatePen(EMF_HANDLE_PEN, _EMF.emf_pentype, static_cast<ulong>(_EMF.emf_linewidth * EMF_PX2HM), color);
	EMF_SelectObject(EMF_HANDLE_PEN);
	EMF_CreatePolygon(points);
	for(i = 0; i<points; i++)
		EMF_write_pointl(corners[i].x, pThis->MaxY - corners[i].y);
	// Force re-evaluation of linetype next time we draw a line 
	_EMF.emf_linetype = LT_UNDEFINED;
	_EMF.emf_dashtype = LT_UNDEFINED;
}

TERM_PUBLIC void EMF_fillbox(GpTermEntry * pThis, int style, uint x1, uint y1, uint width, uint height)
{
	gpiPoint corner[4];
	corner[0].x = x1;        corner[0].y = y1;
	corner[1].x = x1+width;  corner[1].y = y1;
	corner[2].x = x1+width;  corner[2].y = y1+height;
	corner[3].x = x1;        corner[3].y = y1+height;
	corner->style = style;
	EMF_filled_polygon(pThis, 4, corner);
}

TERM_PUBLIC void EMF_linewidth(GpTermEntry * pThis, double width)
{
	EMF_flush_polyline(pThis);
	width *= _EMF.emf_linewidth_factor;
	if(width == _EMF.emf_linewidth)
		return;
	_EMF.emf_linewidth = width;
	// The linewidth is applied at the same time as the dash pattern 
	_EMF.emf_dashtype_count++;
}

TERM_PUBLIC void EMF_flush_dashtype(GpTermEntry * pThis)
{
	if(_EMF.emf_dashtype_count > 0) {
		EMF_load_dashtype(pThis, _EMF.emf_dashtype);
		_EMF.emf_dashtype_count = 0;
	}
}
// 
// Resets _both_ line color and dash type!
// 
TERM_PUBLIC void EMF_load_dashtype(GpTermEntry * pThis, int dashtype)
{
	int i, j;
	double empirical_scale = 0.50;
	// Each group of 8 entries in dot_length[] defines a dash
	// pattern.  Entries in each group are alternately length of
	// line and length of whitespace, in units of 2/3 of the linewidth.
	static int dot_length[(EMF_LINE_TYPES+1) * 8] = { 
		/* 0 - solid             */
		8, 5, 8, 5, 8, 5, 8, 5, /* 1 - dashes            */
		2, 4, 2, 4, 2, 4, 2, 4, /* 2 - dotted            */
		8, 4, 2, 4, 8, 4, 2, 4, /* 3 - dash-dot          */
		9, 4, 2, 4, 2, 0, 0, 4, /* 4 - dash-dot-dot      */
		1, 1, 1, 1, 1, 1, 1, 1 /* Placeholder for custom pattern */
	};
	_EMF.emf_dashtype = dashtype;
	if(dashtype >= 0)
		dashtype = dashtype % EMF_LINE_TYPES;
	if(dashtype == LT_AXIS)
		dashtype = 2;
	if(dashtype == DASHTYPE_CUSTOM) {
		dashtype = EMF_LINE_TYPES; // Point to placeholder array 
		for(i = 0; i < 8; i += 1)
			dot_length[(EMF_LINE_TYPES-1)*8 + i] = _EMF.emf_dashpattern[i] * fceili(_EMF.emf_linewidth * empirical_scale/2.0);
	}
	if(dashtype == DASHTYPE_NODRAW) {
		dashtype = EMF_LINE_TYPES; // Point to placeholder array 
		for(i = 0; i < 7; i++)
			dot_length[(EMF_LINE_TYPES-1)*8 + i] = 0;
		dot_length[(EMF_LINE_TYPES-1)*8 + 7] = 10;
	}
	if(dashtype < 1 || !_EMF.emf_dashed) { // solid mode 
		EMF_SelectObject(EMF_STOCK_OBJECT_BLACK_PEN);
		EMF_DeleteObject(EMF_HANDLE_PEN);
		EMF_CreatePen(EMF_HANDLE_PEN, _EMF.emf_pentype, static_cast<ulong>(_EMF.emf_linewidth * EMF_PX2HM), _EMF.emf_color);
		EMF_SelectObject(EMF_HANDLE_PEN);
		pThis->vector = EMF_solid_vector;
	}
	else {
		// Since win32 dashed lines works only with 1 pixel linewith we must emulate 
		EMF_SelectObject(EMF_STOCK_OBJECT_BLACK_PEN);
		EMF_DeleteObject(EMF_HANDLE_PEN);
		EMF_CreatePen(EMF_HANDLE_PEN, _EMF.emf_pentype, static_cast<ulong>(_EMF.emf_linewidth * EMF_PX2HM), _EMF.emf_color);
		EMF_SelectObject(EMF_HANDLE_PEN);
		pThis->vector = EMF_dashed_vector;
		// set up dash dimensions 
		j = (dashtype - 1) * 8;
		for(i = 0; i < 8; i++, j++) {
			_EMF.emf_step_sizes[i] = static_cast<int>(dot_length[j] * _EMF.emf_dashlength * EMF_PX2HM * _EMF.emf_linewidth * empirical_scale);
		}
		// first thing drawn will be a line 
		_EMF.emf_step = _EMF.emf_step_sizes[0];
		_EMF.emf_step_index = 0;
	}
}

TERM_PUBLIC void EMF_move(GpTermEntry * pThis, uint x, uint y)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(x >= pThis->MaxX || y >= pThis->MaxY) {
		p_gp->IntWarn(NO_CARET, "emf_move: (%d,%d) out of range", x, y);
		x = MIN(x, pThis->MaxX); 
		y = MIN(y, pThis->MaxY);
	}
	if(x == _EMF.Pos.x && y == _EMF.Pos.y)
		return;
	EMF_flush_polyline(pThis);
	_EMF.Pos.Set(x, y);
}

TERM_PUBLIC void EMF_dashed_vector(GpTermEntry * pThis, uint ux, uint uy)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int xa, ya;
	int dx, dy, adx, ady;
	int dist; /* approximate distance in plot units from starting point to end point. */
	long remain;/* approximate distance in plot units remaining to specified end point. */
	if(ux >= pThis->MaxX || uy >= pThis->MaxY)
		p_gp->IntWarn(NO_CARET, "emf_dashed_vector: (%d,%d) out of range", ux, uy);
	dx = (ux - _EMF.Pos.x);
	dy = (uy - _EMF.Pos.y);
	adx = abs(dx);
	ady = abs(dy * 10);
	/* using the approximation sqrt(x**2 + y**2)  ~  x + (5*x*x)/(12*y)   when x > y.
	   Note ordering of calculations to avoid overflow on 16 bit architectures */
	if(10 * adx < ady)
		dist = (ady / 2 + 25 * adx / ady * adx / 6 * 5) / 5;
	else {
		if(adx == 0)
			return;
		dist = (adx * 10 + (ady / 24) * (ady / adx)) / 10;
	}
	remain = dist;
	xa = _EMF.Pos.x;
	ya = _EMF.Pos.y;
	while(remain > _EMF.emf_step) {
		remain -= _EMF.emf_step;
		if(_EMF.emf_step_index & 1) {
			xa = (int)(ux - (remain * dx) / dist);
			ya = (int)(uy - (remain * dy) / dist);
			EMF_move(pThis, xa, ya);
		}
		else {
			EMF_solid_vector(pThis, (int)(ux - (remain * dx) / dist), (int)(uy - (remain * dy) / dist));
		}
		if(++_EMF.emf_step_index >= 8)
			_EMF.emf_step_index = 0;
		_EMF.emf_step = _EMF.emf_step_sizes[_EMF.emf_step_index];
	}
	if(_EMF.emf_step_index & 1)
		EMF_move(pThis, ux, uy);
	else
		EMF_solid_vector(pThis, ux, uy);
	_EMF.emf_step -= (int)remain;
}

TERM_PUBLIC void EMF_solid_vector(GpTermEntry * pThis, uint ux, uint uy)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(ux >= pThis->MaxX || uy >= pThis->MaxY)
		p_gp->IntWarn(NO_CARET, "emf_solid_vector: (%d,%d) out of range", ux, uy);
	if(ux == _EMF.Pos.x && uy == _EMF.Pos.y)
		return;
	if(_EMF.emf_coords * 2 > EMF_MAX_SEGMENTS - 2)
		EMF_flush_polyline(pThis);
	if(_EMF.emf_coords == 0) {
		_EMF.emf_polyline[0] = _EMF.Pos.x;
		_EMF.emf_polyline[1] = _EMF.Pos.y;
		_EMF.emf_coords++;
	}
	_EMF.Pos.x = _EMF.emf_polyline[_EMF.emf_coords * 2] = ux;
	_EMF.Pos.y = _EMF.emf_polyline[_EMF.emf_coords * 2 + 1] = uy;
	_EMF.emf_coords++;
}

TERM_PUBLIC void EMF_put_text(GpTermEntry * pThis, uint x, uint y, const char str[])
{
	GnuPlot * p_gp = pThis->P_Gp;
	int i, alen;
	int slen = strlen(str);
	int nchars = slen;
#ifdef HAVE_ICONV
	char * wstr = 0;
	if(encoding == S_ENC_UTF8 || encoding == S_ENC_SJIS) {
		iconv_t cd;
		size_t wsize, wlen, mblen;
		const char * str_start = str;
		char * wstr_start;
		char * FromEnc;
		nchars = strlen(str);
		mblen = nchars;
		wlen = wsize = 2 * mblen + 2;
		wstr = SAlloc::M(wlen);
		wstr_start = wstr;
		if(encoding == S_ENC_UTF8) 
			FromEnc = "UTF-8";
		else 
			FromEnc = "Shift_JIS";
		if((cd = iconv_open("UTF-16LE", FromEnc)) == (iconv_t)-1)
			p_gp->IntWarn(NO_CARET, "iconv_open failed");
		else {
			if(iconv(cd, (void*)&str_start, &mblen, &wstr_start, &wlen) == (size_t)-1)
				p_gp->IntWarn(NO_CARET, "iconv failed");
			iconv_close(cd);
			slen = wsize - wlen;
			nchars = slen / 2;
		}
	}
#endif
	if(slen <=0) 
		return;    /* shige: 08/30 2012 */
	alen = slen;
	EMF_flush_polyline(pThis);
	if(_EMF.emf_textcolor != _EMF.emf_color) {
		EMF_SetTextColor(_EMF.emf_color);
		_EMF.emf_textcolor = _EMF.emf_color;
	}
	// Use ansi unless we can convert to unicode 
	if(alen % 4)
		alen += 4 - (slen % 4); /* Structure must be long aligned! */
#ifdef HAVE_ICONV
	if(encoding == S_ENC_UTF8 || encoding == S_ENC_SJIS) {
		EMF_write_emr(84, 76 + alen + nchars * 4); /* ExtTextOutW, UTF16-LE version */
	}
	else
#endif
	{
		EMF_write_emr(83, 76 + alen + nchars * 4); /* ExtTextOutA, ANSI char version! */
	}
	EMF_write_rectl(0, 0, 0, 0); /* bounding, never used */
	EMF_write_long(1);      /* GM_Compatible mode for advanced scaling */
	EMF_write_float(EMF_PX2HM); /* x scale */
	EMF_write_float(EMF_PX2HM); /* y scale */
	/* positioning... y is recentered from bottom reference set in
	 * text align */
	EMF_write_pointl(x + (long)((pThis->ChrV/ 2) * sin(_EMF.emf_vert_text * EMF_10THDEG2RAD)), pThis->MaxY - y + (long)(pThis->ChrV / 2 * cos(_EMF.emf_vert_text * EMF_10THDEG2RAD)));
	EMF_write_long(nchars); /* true number of characters */
	EMF_write_long(76);     /* offset to text */
	EMF_write_long(0);      /* options, none */
	EMF_write_rectl(0, 0, 0, 0); /* rectangle clipping not used */
	EMF_write_long(0);      /* offset to intercharacter spacing array */
	                        /* can't be used since we don't know anything */
	                        /* about the font properties being used */

#ifdef HAVE_ICONV
	if(encoding == S_ENC_UTF8 || encoding == S_ENC_SJIS) {
		for(i = 0; i < alen; i++)
			EMF_write_byte(i < slen ? wstr[i] : 0); /* writing text */
		SAlloc::F(wstr);
	}
	else
#endif
	for(i = 0; i < alen; i++)
		EMF_write_byte(i < slen ? str[i] : 0);  /* writing text */
	for(i = 0; i < nchars; i++)
		// writing intercharacter spacing array (but we don't use it) 
		EMF_write_long(300);
	// Invalidate current position 
	_EMF.Pos.x = _EMF.Pos.y = -2000;
}

TERM_PUBLIC int EMF_text_angle(GpTermEntry * pThis, int ang)
{
	// Win GDI rotation is scaled in tenth of degrees, so... 
	switch(ang) {
		case 0:         /* left right */
		    if(_EMF.emf_vert_text != 0) {
			    _EMF.emf_vert_text = 0;
			    EMF_setfont();
		    }
		    break;
		case TEXT_VERTICAL: /* bottom up */
		    if(_EMF.emf_vert_text != 900) {
			    _EMF.emf_vert_text = 900;
			    EMF_setfont();
		    }
		    break;
		default: // the general case 
		    _EMF.emf_vert_text = 10 * ang;
		    EMF_setfont();
		    break;
	}
	return TRUE;
}

TERM_PUBLIC int EMF_justify_text(GpTermEntry * pThis, enum JUSTIFY mode)
{
	int align = GP_TA_BOTTOM;
	_EMF.emf_justify = mode;
	switch(mode) {
		case LEFT: align |= GP_TA_LEFT; break;
		case RIGHT: align |= GP_TA_RIGHT; break;
		case CENTRE: align |= GP_TA_CENTER; break;
	}
	EMF_SetTextAlign(align);
	return TRUE;
}

TERM_PUBLIC void EMF_reset(GpTermEntry * pThis)
{
	_EMF.Pos.x = _EMF.Pos.y = 0;
	_EMF.emf_graphics = FALSE;
}

TERM_PUBLIC void EMF_point(GpTermEntry * pThis, uint x, uint y, int number)
{
	int old_dashtype;
	gpiPoint corners[12];
	corners->style = FS_OPAQUE;
	EMF_flush_polyline(pThis); // Calls EMF_flush_dashtype 
	old_dashtype = _EMF.emf_dashtype;
	_EMF.emf_dashtype = 0;
	_EMF.emf_dashtype_count++;
	// A few special point types 
	if(69 <= number && number <= 73) {
		int emf_color_save = _EMF.emf_color;
		_EMF.emf_color = _EMF.emf_background;
		switch(number) {
			case 69:  EMF_point(pThis, x, y, 4); break;
			case 70:  EMF_point(pThis, x, y, 6); break;
			case 71:  EMF_point(pThis, x, y, 8); break;
			case 72:  EMF_point(pThis, x, y, 10); break;
			case 73:  EMF_point(pThis, x, y, 12); break;
		}
		_EMF.emf_color = emf_color_save;
		switch(number) {
			case 69:  EMF_point(pThis, x, y, 3); break;
			case 70:  EMF_point(pThis, x, y, 5); break;
			case 71:  EMF_point(pThis, x, y, 7); break;
			case 72:  EMF_point(pThis, x, y, 9); break;
			case 73:  EMF_point(pThis, x, y, 11); break;
		}
		_EMF.emf_dashtype = old_dashtype;
		_EMF.emf_dashtype_count++;
		return;
	}
	// draw dot 
	EMF_move(pThis, x, y);
	EMF_solid_vector(pThis, x + 1, y);
	number = number % EMF_POINTS;
	switch(number) {
		case 0:         /* draw plus */
		    EMF_move(pThis, x - _EMF.emf_tic, y);
		    EMF_solid_vector(pThis, x + _EMF.emf_tic, y);
		    EMF_move(pThis, x, y - _EMF.emf_tic);
		    EMF_solid_vector(pThis, x, y + _EMF.emf_tic);
		    break;
		case 1:         /* draw X */
		    EMF_move(pThis, x - _EMF.emf_tic707, y - _EMF.emf_tic707);
		    EMF_solid_vector(pThis, x + _EMF.emf_tic707, y + _EMF.emf_tic707);
		    EMF_move(pThis, x - _EMF.emf_tic707, y + _EMF.emf_tic707);
		    EMF_solid_vector(pThis, x + _EMF.emf_tic707, y - _EMF.emf_tic707);
		    break;
		case 2: // draw star (asterisk) 
		    EMF_move(pThis, x, y - _EMF.emf_tic);
		    EMF_solid_vector(pThis, x, y + _EMF.emf_tic);
		    EMF_move(pThis, x + _EMF.emf_tic866, y - _EMF.emf_tic500);
		    EMF_solid_vector(pThis, x - _EMF.emf_tic866, y + _EMF.emf_tic500);
		    EMF_move(pThis, x + _EMF.emf_tic866, y + _EMF.emf_tic500);
		    EMF_solid_vector(pThis, x - _EMF.emf_tic866, y - _EMF.emf_tic500);
		    break;
		case 3: // draw box 
		    EMF_move(pThis, x - _EMF.emf_tic707, y - _EMF.emf_tic707);
		    EMF_solid_vector(pThis, x + _EMF.emf_tic707, y - _EMF.emf_tic707);
		    EMF_solid_vector(pThis, x + _EMF.emf_tic707, y + _EMF.emf_tic707);
		    EMF_solid_vector(pThis, x - _EMF.emf_tic707, y + _EMF.emf_tic707);
		    EMF_flush_polygon(pThis);
		    break;
		case 4:         /* draw filled box */
		    corners[0].x = x - _EMF.emf_tic707; corners[0].y = y - _EMF.emf_tic707;
		    corners[1].x = x + _EMF.emf_tic707; corners[1].y = y - _EMF.emf_tic707;
		    corners[2].x = x + _EMF.emf_tic707; corners[2].y = y + _EMF.emf_tic707;
		    corners[3].x = x - _EMF.emf_tic707; corners[3].y = y + _EMF.emf_tic707;
		    EMF_filled_polygon(pThis, 4, corners);
		    break;
		case 5:
		    y = pThis->MaxY-y; /* EAM - WTF? */
		    EMF_SelectObject(EMF_STOCK_OBJECT_BLACK_PEN);
		    EMF_SelectObject(EMF_STOCK_OBJECT_WHITE_BRUSH);
		    EMF_DeleteObject(EMF_HANDLE_BRUSH);
		    EMF_CreateBrush(EMF_HANDLE_BRUSH, 1, 0, 0); /* transparent brush */
		    EMF_SelectObject(EMF_HANDLE_BRUSH);
		    EMF_DeleteObject(EMF_HANDLE_PEN);
		    EMF_CreatePen(EMF_HANDLE_PEN, _EMF.emf_pentype, static_cast<ulong>(_EMF.emf_linewidth * EMF_PX2HM), _EMF.emf_color);
		    EMF_SelectObject(EMF_HANDLE_PEN);
		    EMF_Ellipse((long)(x-_EMF.emf_tic), (long)(y-_EMF.emf_tic), (long)(x+_EMF.emf_tic), (long)(y+_EMF.emf_tic));
		    break;
		case 6: /* filled circle */
		    y = pThis->MaxY-y; /* EAM - WTF? */
		    EMF_SelectObject(EMF_STOCK_OBJECT_WHITE_BRUSH);
		    EMF_DeleteObject(EMF_HANDLE_BRUSH);
		    EMF_CreateBrush(EMF_HANDLE_BRUSH, 0, _EMF.emf_color, 0);
		    EMF_SelectObject(EMF_HANDLE_BRUSH);
		    EMF_SelectObject(EMF_STOCK_OBJECT_BLACK_PEN);
		    EMF_DeleteObject(EMF_HANDLE_PEN);
		    EMF_CreatePen(EMF_HANDLE_PEN, _EMF.emf_pentype, static_cast<ulong>(_EMF.emf_linewidth * EMF_PX2HM), _EMF.emf_color);
		    EMF_SelectObject(EMF_HANDLE_PEN);
		    EMF_Ellipse((long)(x-_EMF.emf_tic), (long)(y-_EMF.emf_tic), (long)(x+_EMF.emf_tic), (long)(y+_EMF.emf_tic));
		    break;
		case 7: // draw triangle (point up) 
		    EMF_move(pThis, x, y + _EMF.emf_tic1241);
		    EMF_solid_vector(pThis, x - _EMF.emf_tic1077, y - _EMF.emf_tic621);
		    EMF_solid_vector(pThis, x + _EMF.emf_tic1077, y - _EMF.emf_tic621);
		    EMF_flush_polygon(pThis);
		    break;
		case 8: /* filled triangle point up */
		    corners[0].x = x; corners[0].y = y + _EMF.emf_tic1241;
		    corners[1].x = x - _EMF.emf_tic1077; corners[1].y = y - _EMF.emf_tic621;
		    corners[2].x = x + _EMF.emf_tic1077; corners[2].y = y - _EMF.emf_tic621;
		    EMF_filled_polygon(pThis, 3, corners);
		    break;
		case 9: // draw triangle (point down) 
		    EMF_move(pThis, x, y - _EMF.emf_tic1241);
		    EMF_solid_vector(pThis, x - _EMF.emf_tic1077, y + _EMF.emf_tic621);
		    EMF_solid_vector(pThis, x + _EMF.emf_tic1077, y + _EMF.emf_tic621);
		    EMF_flush_polygon(pThis);
		    break;
		case 10: /* filled triangle point down */
		    corners[0].x = x; corners[0].y = y - _EMF.emf_tic1241;
		    corners[1].x = x - _EMF.emf_tic1077; corners[1].y = y + _EMF.emf_tic621;
		    corners[2].x = x + _EMF.emf_tic1077; corners[2].y = y + _EMF.emf_tic621;
		    EMF_filled_polygon(pThis, 3, corners);
		    break;
		case 11: // draw diamond 
		    EMF_move(pThis, x - _EMF.emf_tic, y);
		    EMF_solid_vector(pThis, x, y - _EMF.emf_tic);
		    EMF_solid_vector(pThis, x + _EMF.emf_tic, y);
		    EMF_solid_vector(pThis, x, y + _EMF.emf_tic);
		    EMF_flush_polygon(pThis);
		    break;
		case 12: /* filled diamond */
		    corners[0].x = x - _EMF.emf_tic; corners[0].y = y;
		    corners[1].x = x; corners[1].y = y - _EMF.emf_tic;
		    corners[2].x = x + _EMF.emf_tic; corners[2].y = y;
		    corners[3].x = x; corners[3].y = y + _EMF.emf_tic;
		    EMF_filled_polygon(pThis, 4, corners);
		    break;
		case 13:        /* draw pentagon */
		    EMF_move(pThis, x + _EMF.emf_tic5878, y + _EMF.emf_tic8090);
		    EMF_solid_vector(pThis, x - _EMF.emf_tic5878, y + _EMF.emf_tic8090);
		    EMF_solid_vector(pThis, x - _EMF.emf_tic9511, y - _EMF.emf_tic3090);
		    EMF_solid_vector(pThis, x,                  y - _EMF.emf_tic);
		    EMF_solid_vector(pThis, x + _EMF.emf_tic9511, y - _EMF.emf_tic3090);
		    EMF_flush_polygon(pThis);
		    break;
		case 14: /* filled pentagon */
		    corners[0].x = x + _EMF.emf_tic5878;  corners[0].y = y + _EMF.emf_tic8090;
		    corners[1].x = x - _EMF.emf_tic5878;  corners[1].y = y + _EMF.emf_tic8090;
		    corners[2].x = x - _EMF.emf_tic9511;  corners[2].y = y - _EMF.emf_tic3090;
		    corners[3].x = x;                corners[3].y = y - _EMF.emf_tic;
		    corners[4].x = x + _EMF.emf_tic9511;  corners[4].y = y - _EMF.emf_tic3090;
		    EMF_filled_polygon(pThis, 5, corners);
		    break;
	}
/* end_points: */
	_EMF.emf_dashtype = old_dashtype;
	_EMF.emf_dashtype_count++;
}

TERM_PUBLIC void EMF_set_pointsize(GpTermEntry * pThis, double size)
{
	if(size < 0)
		size = 1;
	_EMF.emf_tic = static_cast<int>(size * pThis->TicH);
	_EMF.emf_tic707 = ffloori((double)_EMF.emf_tic * 0.707 + 0.5);
	_EMF.emf_tic866 = _EMF.emf_tic * 13 / 15;
	_EMF.emf_tic500 = _EMF.emf_tic / 2;
	_EMF.emf_tic1241 = _EMF.emf_tic * 36 / 29;
	_EMF.emf_tic1077 = _EMF.emf_tic * 14 / 13;
	_EMF.emf_tic9511 = static_cast<int>(_EMF.emf_tic * 0.9511);
	_EMF.emf_tic5878 = static_cast<int>(_EMF.emf_tic * 0.5878);
	_EMF.emf_tic8090 = static_cast<int>(_EMF.emf_tic * 0.8090);
	_EMF.emf_tic3090 = static_cast<int>(_EMF.emf_tic * 0.3090);
	_EMF.emf_tic621 = _EMF.emf_tic * 18 / 29;
}
/*
 * Ethan A Merritt September 2008
 *	- Support for enhanced text mode
 * PROBLEMS:
 *	- Rotated enhanced text is not handled
 *	- The proportional spacing hack is really ugly
 *	  ETO_PDY is supposed to handle this, but pre-Vista Windows
 *	  doesn't support the flag so it's of no real use.
 */

static bool ENHemf_opened_string;

/* used in determining height of processed text */
static float ENHemf_base;

/* use these so that we don't over-write the current font settings */
static float ENHemf_fontsize;
static char   * ENHemf_font;

static bool ENHemf_show = TRUE;
static bool ENHemf_sizeonly = FALSE;
static int ENHemf_overprint = 0;

TERM_PUBLIC void ENHemf_OPEN(GpTermEntry * pThis, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
{
	GnuPlot * p_gp = pThis->P_Gp;
	// If the overprint code requests a save or restore, that's all we do 
#define EMF_AVG_WID 0.8
#undef TA_UPDATECP_MODE
#ifdef TA_UPDATECP_MODE
	if(overprint == 3) {
		EMF_SaveDC();
		return;
	}
	else if(overprint == 4) {
		EMF_RestoreDC();
		return;
	}
#else
	static int save_x, save_y;
	if(overprint == 3) {
		save_x = _EMF.Pos.x;
		save_y = _EMF.Pos.y;
		return;
	}
	else if(overprint == 4) {
		_EMF.Pos.x = save_x;
		_EMF.Pos.y = save_y;
		return;
	}
#endif
	if(!ENHemf_opened_string) {
		int i;
		ENHemf_opened_string = TRUE;
		p_gp->Enht.P_CurText = &p_gp->Enht.Text[0];
		SAlloc::F(ENHemf_font);
		ENHemf_font = sstrdup(fontname);
		for(i = 0; ENHemf_font[i]; i++)
			if(ENHemf_font[i] == ':') ENHemf_font[i] = ' ';
		ENHemf_fontsize = static_cast<float>(fontsize);
		ENHemf_base = static_cast<float>(base * _EMF.emf_fontscale);
		ENHemf_show = showflag;
		ENHemf_overprint = overprint;
	}
}
//
// Write a string fragment and update the current position 
//
TERM_PUBLIC void ENHemf_FLUSH(GpTermEntry * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	uint x, y;
	int x_offset, y_offset;
	char * str;
	int i;
	int incr_x;
	double strl;
	if(ENHemf_opened_string) {
		*p_gp->Enht.P_CurText = '\0';
		ENHemf_opened_string = FALSE;
		x = _EMF.Pos.x;
		y = _EMF.Pos.y;
		if(1) {
			char save_font[256];
			float save_fontsize = _EMF.emf_fontsize;
			strcpy(save_font, _EMF.emf_fontname);
			_EMF.emf_fontsize = ENHemf_fontsize;
			EMF_set_font(pThis, ENHemf_font);
			_EMF.emf_fontsize = save_fontsize;
			strcpy(_EMF.emf_fontname, save_font);
		}
		str = p_gp->Enht.Text;
#ifndef GP_TA_UPDATEPC_MODE
		// We are especially bad at guessing the width of whitespace. 
		// Best is to pile up all our errors on top of leading space. 
		i = strspn(p_gp->Enht.Text, " ");
		if(i > 0) {
			double blank = i * pThis->ChrH * EMF_AVG_WID;
			x += cos(_EMF.emf_vert_text * EMF_10THDEG2RAD) * blank;
			y += sin(_EMF.emf_vert_text * EMF_10THDEG2RAD) * blank;
			_EMF.Pos.x = x;
			_EMF.Pos.y = y;
			str += i;
		}
#endif
		x_offset = static_cast<int>(sin(_EMF.emf_vert_text * EMF_10THDEG2RAD) * ENHemf_base * EMF_PX2HM);
		y_offset = static_cast<int>(cos(_EMF.emf_vert_text * EMF_10THDEG2RAD) * ENHemf_base * EMF_PX2HM);
		if(ENHemf_show && !ENHemf_sizeonly)
			EMF_put_text(pThis, x-x_offset, y+y_offset, str);
		if(encoding == S_ENC_UTF8) {
			// The strwidth_utf8() function approximates the space occupied 
			// by a UTF8 string, taking into account that CJK characters    
			// are rougnly twice as wide as a typical ascii character.      
			strl = strwidth_utf8(str);
		}
		else {
			// Otherwise we assume 1 equal-width character per byte. 
			strl = strlen(str);
		}
		if(_EMF.emf_tweak) {
			// Tweak estimated length of rendered string by counting "thin" 
			// characters and "wide" characters.                            
			// In principle EMF will accept an array of char widths, but    
			// most EMF viewers don't implement this option (ETO_PDY).      
			int thin = 0, wide = 0;
			for(i = 0; i < sstrleni(str); i++) {
				if((encoding == S_ENC_UTF8) && ((str[i] & 0x100)))
					continue;
				if(strchr(" ijl.,;:|!()[]I-'", str[i]))
					thin++;
				if(('A' <= str[i] && str[i] <= 'Z') || strchr("mw<>", str[i]))
					wide++;
				if(strchr(" i.,;:|!'", str[i])) /* really thin */
					thin++;
			}
			strl += 0.30 * wide;
			strl -= 0.15 * thin;
		}
		incr_x = static_cast<int>(strl * EMF_AVG_WID * pThis->ChrH);
		// Attempt to handle slanted text. Not entirely successful 
		_EMF.Pos.x = static_cast<int>(x + incr_x * cos(_EMF.emf_vert_text * EMF_10THDEG2RAD));
		_EMF.Pos.y = static_cast<int>(y + incr_x * sin(_EMF.emf_vert_text * EMF_10THDEG2RAD));
		FPRINTF((stderr, "fontwidth = %d text box: %d x %d\n",
		    (int)(EMF_AVG_WID * pThis->ChrH),
		    (int)(incr_x * cos(emf_vert_text * EMF_10THDEG2RAD)),
		    (int)(incr_x * sin(emf_vert_text * EMF_10THDEG2RAD))));
		if(ENHemf_overprint == 1) {
			_EMF.Pos.x -= 0.5 * incr_x * cos(_EMF.emf_vert_text * EMF_10THDEG2RAD);
			_EMF.Pos.y -= 0.5 * incr_x * sin(_EMF.emf_vert_text * EMF_10THDEG2RAD);
		}
	}
}

TERM_PUBLIC void ENHemf_put_text(GpTermEntry * pThis, uint x, uint y, const char * str)
{
	GnuPlot * p_gp = pThis->P_Gp;
	char * original_string = (char *)str;
	if(isempty(str))
		return;
	// if there are no magic characters, we should just be able punt the string to EMF_put_text()
	if(!strstr(str, "\\U+"))
		if(p_gp->Enht.Ignore || !strpbrk(str, "{}^_@&~")) {
			// FIXME: do something to ensure default font is selected 
			EMF_put_text(pThis, x, y, str);
			return;
		}
	EMF_move(pThis, x, y);
	if(_EMF.emf_textcolor != _EMF.emf_color) {
		EMF_SetTextColor(_EMF.emf_color);
		_EMF.emf_textcolor = _EMF.emf_color;
	}
	// set up the global variables needed by enhanced_recursion() 
	p_gp->Enht.FontScale = 1.0;
	strnzcpy(p_gp->Enht.EscapeFormat, "&#x%2.2x;", sizeof(p_gp->Enht.EscapeFormat));
	ENHemf_opened_string = FALSE;
	ENHemf_overprint = 0;
	ENHemf_fontsize = _EMF.emf_fontsize;
	if(_EMF.emf_justify == RIGHT || _EMF.emf_justify == CENTRE)
		ENHemf_sizeonly = TRUE;
#ifdef UPDATECP_MODE
	EMF_SetTextAlign(GP_TA_BASELINE|GP_TA_LEFT|GP_TA_UPDATECP);
#else
	EMF_SetTextAlign(GP_TA_BASELINE|GP_TA_LEFT|GP_TA_NOUPDATECP);
#endif
	/* Set the recursion going. We say to keep going until a
	 * closing brace, but we don't really expect to find one.
	 * If the return value is not the nul-terminator of the
	 * string, that can only mean that we did find an unmatched
	 * closing brace in the string. We increment past it (else
	 * we get stuck in an infinite loop) and try again.
	 */
	while(*(str = enhanced_recursion(pThis, (char *)str, TRUE, _EMF.emf_fontname, ENHemf_fontsize, 0.0, TRUE, TRUE, 0))) {
		(pThis->enhanced_flush)(pThis);
		// I think we can only get here if *str == '}' 
		p_gp->EnhErrCheck(str);
		if(!*++str)
			break; /* end of string */
		/* else carry on and process the rest of the string */
	}
	/* EAM May 2010 - 2-pass text justification */
	/* We can do text justification by running the entire top level string */
	/* through 2 times, with the ENHgd_sizeonly flag set the first time.   */
	/* After seeing where the final position is, we then offset the start  */
	/* point accordingly and run it again without the sizeonly flag set.   */
	if(_EMF.emf_justify == RIGHT || _EMF.emf_justify == CENTRE) {
		enum JUSTIFY justification = _EMF.emf_justify;
		int x_offset = _EMF.Pos.x - x;
		int y_offset = (_EMF.emf_vert_text == 0) ? 0 : _EMF.Pos.y - y;
		_EMF.emf_justify = LEFT;
		ENHemf_sizeonly = FALSE;
		if(justification == RIGHT)
			ENHemf_put_text(pThis, x - x_offset, y - y_offset, original_string);
		else if(justification == CENTRE)
			ENHemf_put_text(pThis, x - x_offset/2, y - y_offset/2, original_string);
		_EMF.emf_justify = justification;
	}
	// Restore everything we messed with 
	EMF_setfont();
	ZFREE(_EMF.emf_last_fontname); // invalidate any previous font 
	ENHemf_base = 0;
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE
TERM_TABLE_START(emf_driver)
	"emf", 
	"Enhanced Metafile format",
	(uint)EMF_XMAX, 
	(uint)EMF_YMAX, 
	(uint)EMF_VCHAR, 
	(uint)EMF_HCHAR,
	(uint)EMF_VTIC, 
	(uint)EMF_HTIC, 
	EMF_options, 
	EMF_init, 
	EMF_reset,
	EMF_text, 
	GnuPlot::NullScale, 
	EMF_graphics, 
	EMF_move, 
	EMF_solid_vector,
	EMF_linetype, 
	EMF_put_text, 
	EMF_text_angle,
	EMF_justify_text, 
	EMF_point, 
	GnuPlot::DoArrow, 
	EMF_set_font,
	EMF_set_pointsize,
	TERM_BINARY|TERM_CAN_DASH|TERM_LINEWIDTH|TERM_FONTSCALE,
	NULL,                                   /* suspend */
	NULL,                                   /* resume  */
	EMF_fillbox,
	EMF_linewidth,
	#ifdef USE_MOUSE
	0, 
	0, 
	0, 
	0, 
	0,    /* no mouse support for emf */
	#endif
	EMF_make_palette,
	EMF_previous_palette,
	EMF_set_color,
	EMF_filled_polygon,
	NULL,     /* image */
	ENHemf_OPEN, 
	ENHemf_FLUSH, 
	do_enh_writec,
	NULL,          /* layer */
	NULL,          /* path */
	0.0,
	NULL,          /* hypertext */
	NULL,
	NULL,          /* modify_plots */
	EMF_dashtype 
TERM_TABLE_END(emf_driver)
#undef LAST_TERM
#define LAST_TERM emf_driver

#endif /* TERM_TABLE */
#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
START_HELP(emf)
"1 emf",
"?commands set terminal emf",
"?set terminal emf",
"?set term emf",
"?terminal emf",
"?term emf",
"?emf",
" The `emf` terminal generates an Enhanced Metafile Format file.",
" This file format is recognized by many Windows applications.",
"",
" Syntax:",
"       set terminal emf {color | monochrome}",
"                        {enhanced {noproportional}}",
"                        {rounded | butt}",
"                        {linewidth <LW>} {dashlength <DL>}",
"                        {size XX,YY} {background <rgb_color>}",
"                        {font \"<fontname>{,<fontsize>}\"}",
"                        {fontscale <scale>}",
"",
" In `monochrome` mode successive line types cycle through dash patterns.",
" `linewidth <factor>` multiplies all line widths by this factor.",
" `dashlength <factor>` is useful for thick lines.",
" <fontname> is the name of a font; and ",
" `<fontsize>` is the size of the font in points.",
"",
" The nominal size of the output image defaults to 1024x768 in arbitrary",
" units. You may specify a different nominal size using the `size` option.",
"",
" Enhanced text mode tries to approximate proportional character spacing.",
" If you are using a monospaced font, or don't like the approximation, you",
" can turn off this correction using the `noproportional` option.",
"",
" The default settings are `color font \"Arial,12\" size 1024,768`",
" Selecting `default` sets all options to their default values.",
"",
" Examples:",
"       set terminal emf 'Times Roman Italic, 12'"
END_HELP(emf)
#endif /* TERM_HELP */
