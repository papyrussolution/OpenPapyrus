// GNUPLOT - hpgl.trm 
// Copyright 1990 - 1993, 1998, 2004, 2018
/*
 * This file is included by ../term.h.
 *
 * This terminal driver supports:
 *  hpgl, hp7550, hp7580b, HP Laserjet III
 *  hp7550 has been replaced by  "hpgl 8 eject"
 *  hp7580b has been replaced by "hpgl 4"
 *  pcl5e/c compatible printers
 *
 * AUTHORS: Colin Kelley, Thomas Williams, Russell Lang
 * send your comments or suggestions to (gnuplot-info@lists.sourceforge.net).
 */
/*
 *
 * Modified for expanded HPGL/2 and PCL utilites
 *  Tom Swiler (tom@silica.mse.ufl.edu)
 * Modified June 1995 Ian MacPhedran to support newterm format
 * Modified October 1995 Ian MacPhedran to simplify HPGL terminals
 * Modified January 96 by David Denholm and Emmanuel Bigler for cp850
 *                         and iso international character sets
 * Modified February 99 by Jeremy Brenes to give PCL5 terminal optional
 *   multi-pen support (6 pen default), a default 34" plotting width for
 *   use with large color plotters such as the HP Designjet 750C,
 *   various alternative plot sizes, and variable fontsizes;
 *   Also decreased the HPGL terminal's fixed fontsize to make it more
 *   reasonable when plots get scaled to 34"
 * Modified July 99 by Jeremy Brenes to make extended plot area smaller;
 *   added solid/dashed lines option, additional font types, mixed fonts
 * Modified November 99 by Jeremy Brenes to add a postscript pointtypes
 *   option, special purpose negative pointtypes, and a pointsize function
 * Modified March 18 by Bastian Maerkisch:  enhancements to the pcl5 driver:
 *   option parser more in line with current standards:
 *   terminal options in arbitrary order; common canvas size and font options
 *   remove monochrome/solid/dashed option
 *   change option defaults to 8 pens (PCL5c spec), letter size, and pspoints;
 *   filled polygons and filled boxes; palette and RGB colors;
 *   explicit vertical center alignment of text instead of relying on font
 *   metric guess; rotate text by arbitrary angles; request font "symbol set"
 *   according to current encoding; bold/italic font support; UTF-8 encoding;
 *   ignore case/spaces/underscores in font names; more (typical) fonts;
 *   permit some alternative font alias from non-HP vendors; enhanced text;
 *   variable line width; optional rounded line ends and joins;
 *   version 5 dashed lines, including custom dash types; optimized point
 *   symbols; linewidth/pointsize/fontscale options;
 *   make use of bool type; separate help texts for hpgl and pcl5;
 *
 */
#include <gnuplot.h>
#pragma hdrstop
#define HPGL
#define PCL
#include "driver.h"

// @experimental {
#define TERM_BODY
#define TERM_PUBLIC static
#define TERM_TABLE
#define TERM_TABLE_START(x) GpTermEntry_Static x {
#define TERM_TABLE_END(x)   };
// } @experimental

#ifdef TERM_REGISTER
	register_term(hpgl)
	register_term(pcl5)
#endif /* TERM_REGISTER */

//#ifdef TERM_PROTO
TERM_PUBLIC void HPGL_options();
TERM_PUBLIC void PCL_options(GpTermEntry_Static * pThis, GnuPlot * pGp);
TERM_PUBLIC void HPGL_init(GpTermEntry_Static * pThis);
/* TERM_PUBLIC void HPGL2_init(); */
TERM_PUBLIC void PCL_init(GpTermEntry_Static * pThis);
TERM_PUBLIC void HPGL_graphics(GpTermEntry_Static * pThis);
TERM_PUBLIC void HPGL2_graphics(GpTermEntry_Static * pThis);
TERM_PUBLIC void PCL_graphics(GpTermEntry_Static * pThis);
TERM_PUBLIC void HPGL_text(GpTermEntry_Static * pThis);
/* TERM_PUBLIC void HPGL2_text(GpTermEntry_Static * pThis); */
TERM_PUBLIC void PCL_text(GpTermEntry_Static * pThis);
TERM_PUBLIC void HPGL_linetype(GpTermEntry_Static * pThis, int linetype);
TERM_PUBLIC void HPGL2_linetype(GpTermEntry_Static * pThis, int linetype);
TERM_PUBLIC void HPGL_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str);
TERM_PUBLIC void HPGL2_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str);
TERM_PUBLIC void HPGL_move(GpTermEntry_Static * pThis, uint x, uint y);
TERM_PUBLIC void HPGL_vector(GpTermEntry_Static * pThis, uint x, uint y);
TERM_PUBLIC void HPGL2_move(GpTermEntry_Static * pThis, uint x, uint y);
TERM_PUBLIC void HPGL2_encode(int d);
TERM_PUBLIC int  HPGL_text_angle(GpTermEntry_Static * pThis, int ang);
TERM_PUBLIC int  HPGL2_text_angle(GpTermEntry_Static * pThis, int ang);
TERM_PUBLIC void HPGL_reset(GpTermEntry_Static * pThis);
/* TERM_PUBLIC void HPGL2_reset(); */
TERM_PUBLIC void PCL_reset(GpTermEntry_Static * pThis);
TERM_PUBLIC int  HPGL2_justify_text(GpTermEntry_Static * pThis, enum JUSTIFY just);
TERM_PUBLIC int  HPGL2_set_font(GpTermEntry_Static * pThis, const char * font);
TERM_PUBLIC void HPGL2_point(GpTermEntry_Static * pThis, uint x, uint y, int number);
TERM_PUBLIC void HPGL2_neg_point(GpTermEntry_Static * pThis, uint x, uint y, int number);
TERM_PUBLIC void HPGL2_pointsize(GpTermEntry_Static * pThis, double size);
TERM_PUBLIC void HPGL2_linewidth(GpTermEntry_Static * pThis, double linewidth);
TERM_PUBLIC void HPGL2_fillbox(GpTermEntry_Static * pThis, int style, uint x1, uint y1, uint width, uint height);
TERM_PUBLIC void HPGL2_filled_polygon(GpTermEntry_Static * pThis, int points, gpiPoint * corners);
TERM_PUBLIC void HPGL2_set_color(GpTermEntry_Static * pThis, const t_colorspec * colorspec);
TERM_PUBLIC int HPGL2_make_palette(GpTermEntry_Static * pThis, t_sm_palette * palette);
TERM_PUBLIC void HPGL2_enh_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char str[]);
TERM_PUBLIC void HPGL2_enh_open(GpTermEntry_Static * pThis, const char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
TERM_PUBLIC void HPGL2_enh_flush(GpTermEntry_Static * pThis);
#define GOT_HPGL_PROTO
//#endif /* TERM_PROTO */

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY
#ifndef M_SQRT2
	#define M_SQRT2 1.41
#endif
/*
 * The maximum plot size, in plotter units.
 * Note that the actual size of larger plots may be limited by
 * available printer memory.
 */

#define HPGL_PUPI       1016    /* Plotter units per inch */

/* Letter */
#define HPGL_XMAX_A     10000
#define HPGL_YMAX_A     7500
/* Legal */
#define HPGL_XMAX_B     13000
#define HPGL_YMAX_B     7500
/* Noextended */
#define HPGL_XMAX_C     45333
#define HPGL_YMAX_C     34000
/* Extended */
#define HPGL_XMAX_D     52000
#define HPGL_YMAX_D     34000
/* A4 */
#define HPGL_XMAX_E     10000
#define HPGL_YMAX_E     7900

#define HPGL_XMAX       HPGL_XMAX_A
#define HPGL_YMAX       HPGL_YMAX_A

#define PCL_XMAX        HPGL_XMAX_A
#define PCL_YMAX        (HPGL_YMAX_A-60)

/*
 * Tic sizes
 */

#define HPGL_VTIC       (HPGL_YMAX/70)
#define HPGL_HTIC       (HPGL_YMAX/70)

#define PCL_VTIC        ((HPGL_YMAX_C-60)/320)
#define PCL_HTIC        ((HPGL_YMAX_C-60)/320)

/*
 * Font size for HPGL
 */

#define HPGL_VCHAR      (HPGL_YMAX/100*8/10)    /* 0.8% */
#define HPGL_HCHAR      (HPGL_XMAX/100*3/10)    /* 0.3% */

/*
 * Font size for HPGL/2
 */

#define HPGL2_DEF_POINT 12      /* Height of font */

#define HPGL2_DEF_PITCH (3 * 72 / (HPGL2_DEF_POINT * 2))
#define HPGL2_VCHAR     ((int)HPGL_PUPI * HPGL2_DEF_POINT / 72)
#define HPGL2_HCHAR     (HPGL2_VCHAR * 2 / 3)

/*
 * Number of available point types for HPGL/2
 */

#define HPGL2_NUM_NOPSPOINTS 6; /* for nopspoints option */
#define HPGL2_NUM_PSPOINTS 75; /* for pspoints option */

/*
 * Control constants
 */

#define DOWN            0       /* Pen is down */
#define UP              1       /* Pen is up */
#define UNKNOWN         -10     /* Unknown status for lots of things */

/*
 * For Polyline Encoded, either use base 64 or base 32.
 * Save space with base 64, but get 8-bit characters.
 */

#define HPGL2_BASE64 1

#if HPGL2_BASE64
#define HPGL2_BITS 6
#define HPGL2_LOW_OFFS 63
#define HPGL2_HIGH_OFFS 191
#define HPGL2_MASK 63
#else
#define HPGL2_BITS 5
#define HPGL2_LOW_OFFS 63
#define HPGL2_HIGH_OFFS 95
#define HPGL2_MASK 31
#endif

/*
 * Local Prototypes
 */

static void HPGL2_put_text_here(const char * str, bool centeralign);
static int HPGL2_set_font_size(GpTermEntry_Static * pThis, const char * font, double size);
static int HPGL2_map_encoding();
static void HPGL2_end_poly();
static void HPGL2_dot(GpTermEntry_Static * pThis, int x, int y);
static void HPGL2_diamond(GpTermEntry_Static * pThis, int x, int y, int htic, int vtic);
static void HPGL2_filled_diamond(GpTermEntry_Static * pThis, int x, int y, int htic, int vtic);
static void HPGL2_pentagon(GpTermEntry_Static * pThis, int x, int y, int htic, int vtic);

/*
 * Data structures for options
 */
struct HPGL2_font_str {
	const char * compare;
	const char * alt;
	const char * name;
	int symbol_set;
	int spacing;
	double pitch;
	double height;
	int posture;
	int stroke_weight;
	int italic;
	int bold;
	int typeface;
};

struct PCL_mode_str {
	const char * name;
	const char * command;
	uint xmax;
	uint ymax;
};

/*
 * The default font goes first.
 */
static struct HPGL2_font_str HPGL2_font_table[] = {
	{ "Un$ivers", "Swiss 742$ SWC", "univers", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 1, 3, 4148 },
	{ "St$ick", NULL, "stick", 277, 0, HPGL2_DEF_PITCH, 0.0, 0, 0, 1, 3, 48 },
	{ "Al$bertus", "Flare$serif 821", "albertus", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 1, 0, 4, 4362 },
	{ "An$tique_Olive", "Inci$sed 901", "antique_olive", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 1, 3, 4168 },
	{ "Ar$ial", "Swiss 721", "arial", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 1, 3, 16602 },
	{ "Av$ant Garde Gothic", "Geom$etric 711", "avant_garde_gothic", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 1, 2, 24607 },
	{ "Bo$okman", "Bookman SWA", "bookman", 277, 1, 0.0, HPGL2_DEF_POINT, 0, -3, 1, 2, 24623 },
	{ "Zapf Ch$ancery", "Chan$cery 801", "zapf_chancery", 277, 1, 0.0, HPGL2_DEF_POINT, 1, 0, 1, 0, 45099 },
	{ "Cl$arendon Condensed", "Clarendon 701", "clarendon", 277, 1, 0.0, HPGL2_DEF_POINT, 4, 3, 4, 3, 4140 },
	{ "Cor$onet", "Ribbon$ 131", "coronet", 277, 1, 0.0, HPGL2_DEF_POINT, 1, 0, 1, 0, 4116 },
	{ "Cou$rier", "FixedPitch 810", "courier", 277, 0, HPGL2_DEF_PITCH, 0.0, 0, 0, 1, 3, 4099 },
	{ "CourierPS", "Courier SWA", "courier", 277, 0, HPGL2_DEF_PITCH, 0.0, 0, 0, 1, 3, 24579 },
	{ "CG Ti$mes", "Dutch 801$ SWA", "cg_times", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 1, 3, 4101 },
	{ "Ga$ramond Antiqua", "Aldine 430", "garamond_antigua", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 1, 3, 4197 },
	{ "Helv$etica", "Swiss$ SWA", "helvetica", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 1, 3, 24580 },
	{ "Helvetica N$arrow", "SwissNarrow$ SWA", "helvetica_narrow", 277, 1, 0.0, HPGL2_DEF_POINT, 4, 0, 5, 3, 24580 },
	{ "Le$tter Gothic", "FixedPitch 850", "letter_gothic", 277, 0, HPGL2_DEF_PITCH, 0.0, 0, 0, 1, 3, 4102 },
	{ "Ma$rigold", "Calli$graphic 401", "marigold", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 0, 0, 4297 },
	{ "New Cen$tury Schlbk", "Cent$urySchbk", "new_century_schlbk", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 1, 3, 24703 },
	{ "CG O$mega", "Zapf Humanist$ 601", "cg_omega", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 1, 3, 4113 },
	{ "Pa$latino", "Zapf Callig$raphic 801", "palatino", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 1, 3, 24591 },
	{ "Ti$mes_New_Roman", "Dutch 801$ SWM", "times_new_roman", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 1, 3, 16901 },
	{ "Times Ro$man", "Dutch SWA", "times_roman", 277, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 1, 3, 25093 },
	{ "Zapf Di$ngbats", "Ding$ Dings", "zapf_dingbats", 460, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 0, 0, 45101 },
	{ "Symbol$ Set", NULL, "symbol", 621, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 0, 0, 4142 },
	{ "Symbol PS", "Symbol Set SWA", "symbol_ps", 621, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 0, 0, 45358 },
	{ "Tr$uetype_Symbols", NULL, "truetype_symbols", 621, 1, 0.0, HPGL2_DEF_POINT, 0, 0, 0, 0, 16686 },
	{ "Wi$ngdings", "More WingBats$ SWC", "wingdings", 18540, 1, 0.0, HPGL2_DEF_POINT, 1, 0, 1, 0, 31402 }
};

#define HPGL2_FONTS (sizeof(HPGL2_font_table) / sizeof(struct HPGL2_font_str))
static struct HPGL2_font_str * HPGL2_font = &HPGL2_font_table[0];

/*
 * The default mode goes first.  Landscape style plots are probably the
 * most compatible with other HPGL devices.
 */

static struct PCL_mode_str PCL_mode_table[] = {
	{ "landscape", "\033&l1O" },
	{ "portrait", "\033&l0O" }
};
#define PCL_MODES (sizeof(PCL_mode_table) / sizeof(struct PCL_mode_str))
static struct PCL_mode_str PCL_mode = { NULL, NULL, PCL_XMAX, PCL_YMAX };

/* encoding vector for cp850 , characters 128 (0200) -> 255 (0377) */

static char hpgl_cp_850[128][4] = {
/* 0200 */ "\0164\017",
/* 0201 */ "\016O\017",
/* 0202 */ "\016E\017",
/* 0203 */ "\016@\017",
/* 0204 */ "\016L\017",
/* 0205 */ "\016H\017",
/* 0206 */ "\016T\017",
/* 0207 */ "\0165\017",

/* 0210 */ "\016A\017",
/* 0211 */ "\016M\017",
/* 0212 */ "\016I\017",
/* 0213 */ "\016]\017",
/* 0214 */ "\016Q\017",
/* 0215 */ "\016Y\017",
/* 0216 */ "\016X\017",
/* 0217 */ "\016P\017",

/* 0220 */ "\016\134\017",
/* 0221 */ "\016W\017",
/* 0222 */ "\016S\017",
/* 0223 */ "\016B\017",
/* 0224 */ "\016N\017",
/* 0225 */ "\016J\017",
/* 0226 */ "\016C\017",
/* 0227 */ "\016K\017",

/* 0230 */ "\016o\017",
/* 0231 */ "\016Z\017",
/* 0232 */ "\016[\017",
/* 0233 */ "\016V\017",
/* 0234 */ "\016;\017",
/* 0235 */ "\016R\017",
/* 0236 */ "",
/* 0237 */ "\016>\017",

/* 0240 */ "\016D\017",
/* 0241 */ "\016U\017",
/* 0242 */ "\016F\017",
/* 0243 */ "\016G\017",
/* 0244 */ "\0167\017",
/* 0245 */ "\0166\017",
/* 0246 */ "\016y\017",
/* 0247 */ "\016z\017",

/* 0250 */ "\0169\017",

/* 0251 */ "",
/* 0252 */ "",

/* 0253 */ "\016x\017",
/* 0254 */ "\016w\017",
/* 0255 */ "\0168\017",
/* 0256 */ "\016{\017",
/* 0257 */ "\016}\017",

/* 0260 */ "",
/* 0261 */ "",
/* 0262 */ "",
/* 0263 */ "",
/* 0264 */ "",

/* 0265 */ "\016`\017",
/* 0266 */ "\016\042\017",
/* 0267 */ "\016!\017",

/* 0270 */ "",
/* 0271 */ "",
/* 0272 */ "",
/* 0273 */ "",
/* 0274 */ "",

/* 0275 */ "\016?\017",
/* 0276 */ "\016<\017",

/* 0277 */ "",

/* 0300 */ "",
/* 0301 */ "",
/* 0302 */ "",
/* 0303 */ "",
/* 0304 */ "",
/* 0305 */ "",

/* 0306 */ "\016b\017",
/* 0307 */ "\016a\017",

/* 0310 */ "",
/* 0311 */ "",
/* 0312 */ "",
/* 0313 */ "",
/* 0314 */ "",
/* 0315 */ "",
/* 0316 */ "",

/* 0317 */ "\016:\017",

/* 0320 */ "\016d\017",
/* 0321 */ "\016c\017",
/* 0322 */ "\016$\017",
/* 0323 */ "\016%\017",
/* 0324 */ "\016#\017",

/* 0325 */ "",

/* 0326 */ "\016e\017",
/* 0327 */ "\016&\017",

/* 0330 */ "\016'\017",

/* 0331 */ "",
/* 0332 */ "",
/* 0333 */ "",
/* 0334 */ "",
/* 0335 */ "",

/* 0336 */ "\016f\017",
/* 0337 */ "",

/* 0340 */ "\016g\017",
/* 0341 */ "\016^\017",
/* 0342 */ "\016_\017",
/* 0343 */ "\016h\017",
/* 0344 */ "\016j\017",
/* 0345 */ "\016i\017",
/* 0346 */ "",
/* 0347 */ "\016q\017",

/* 0350 */ "\016p\017",
/* 0351 */ "\016m\017",
/* 0352 */ "\016.\017",
/* 0353 */ "\016-\017",
/* 0354 */ "",
/* 0355 */ "",
/* 0356 */ "\0160\017",
/* 0357 */ "\016(\017",

/* 0360 */ "\016v\017",
/* 0361 */ "\016~\017",
/* 0362 */ "",
/* 0363 */ "",
/* 0364 */ "",
/* 0365 */ "\016=\017",
/* 0366 */ "",
/* 0367 */ "",

/* 0370 */ "\016z\017",
/* 0371 */ "\016+\017",
/* 0372 */ "",
/* 0373 */ "",
/* 0374 */ "",
/* 0375 */ "",
/* 0376 */ "",
/* 0377 */ ""
};

/* encoding vector for iso-8859-1 , characters 128 (0200) -> 255 (0377) */

static char hpgl_iso_8859_1[128][4] = {
/* 0200 */ "",
/* 0201 */ "",
/* 0202 */ "",
/* 0203 */ "",
/* 0204 */ "",
/* 0205 */ "",
/* 0206 */ "",
/* 0207 */ "",

/* 0210 */ "",
/* 0211 */ "",
/* 0212 */ "",
/* 0213 */ "",
/* 0214 */ "",
/* 0215 */ "",
/* 0216 */ "",
/* 0217 */ "",

/* 0220 */ "",
/* 0221 */ "\016\017",
/* 0222 */ "\016\017",
/* 0223 */ "",
/* 0224 */ "",
/* 0225 */ "",
/* 0226 */ "",
/* 0227 */ "",

/* 0230 */ "",
/* 0231 */ "",
/* 0232 */ "",
/* 0233 */ "",
/* 0234 */ "",
/* 0235 */ "",
/* 0236 */ "",
/* 0237 */ "",

/* 0240 */ "",
/* 0241 */ "\0168\017",
/* 0242 */ "\0165\017",
/* 0243 */ "\016;\017",
/* 0244 */ "\016:\017",
/* 0245 */ "\016<\017",
/* 0246 */ "\017|\017",
/* 0247 */ "\016=\017",

/* 0250 */ "\016+\017",
/* 0251 */ "",
/* 0252 */ "\016y\017",
/* 0253 */ "\016{\017",
/* 0254 */ "",
/* 0255 */ "",
/* 0256 */ "",
/* 0257 */ "\0160\017",

/* 0260 */ "\016z\017",
/* 0261 */ "\016~\017",
/* 0262 */ "",
/* 0263 */ "",
/* 0264 */ "",
/* 0265 */ "",
/* 0266 */ "",
/* 0267 */ "",

/* 0270 */ "",
/* 0271 */ "",
/* 0272 */ "\016z\017",
/* 0273 */ "\016}\017",
/* 0274 */ "\016w\017",
/* 0275 */ "\016x\017",
/* 0276 */ "",
/* 0277 */ "\0169\017",

/* 0300 */ "\016!\017",
/* 0301 */ "\016`\017",
/* 0302 */ "\016\042\017",
/* 0303 */ "\016a\017",
/* 0304 */ "\016X\017",
/* 0305 */ "\016P\017",
/* 0306 */ "\016S\017",
/* 0307 */ "\0164\017",

/* 0310 */ "\016#\017",
/* 0311 */ "\016\134\017",
/* 0312 */ "\016$\017",
/* 0313 */ "\016%\017",
/* 0314 */ "\016f\017",
/* 0315 */ "\016e\017",
/* 0316 */ "\016\046\017",
/* 0317 */ "\016'\017",

/* 0320 */ "\016c\017",
/* 0321 */ "\0166\017",
/* 0322 */ "\016h\017",
/* 0323 */ "\016g\017",
/* 0324 */ "\016_\017",
/* 0325 */ "\016i\017",
/* 0326 */ "\016Z\017",
/* 0327 */ "",

/* 0330 */ "\016R\017",
/* 0331 */ "\016-\017",
/* 0332 */ "\016m\017",
/* 0333 */ "\016.\017",
/* 0334 */ "\016[\017",
/* 0335 */ "",
/* 0336 */ "\016p\017",
/* 0337 */ "\016^\017",

/* 0340 */ "\016H\017",
/* 0341 */ "\016D\017",
/* 0342 */ "\016@\017",
/* 0343 */ "\016b\017",
/* 0344 */ "\016L\017",
/* 0345 */ "\016T\017",
/* 0346 */ "\016W\017",
/* 0347 */ "\0165\017",

/* 0350 */ "\016I\017",
/* 0351 */ "\016E\017",
/* 0352 */ "\016A\017",
/* 0353 */ "\016M\017",
/* 0354 */ "\016Y\017",
/* 0355 */ "\016U\017",
/* 0356 */ "\016Q\017",
/* 0357 */ "\016]\017",

/* 0360 */ "\016d\017",
/* 0361 */ "\0167\017",
/* 0362 */ "\016J\017",
/* 0363 */ "\016F\017",
/* 0364 */ "\016B\017",
/* 0365 */ "\016j\017",
/* 0366 */ "\016N\017",
/* 0367 */ "",

/* 0370 */ "\016V\017",
/* 0371 */ "\016K\017",
/* 0372 */ "\016G\017",
/* 0373 */ "\016C\017",
/* 0374 */ "\016O\017",
/* 0375 */ "",
/* 0376 */ "\016q\017",
/* 0377 */ "\016o\017"
};

/*
 * Static variables to keep track of where we are, etc.
 */

static int HPGL_ang = 0;
static int HPGL_x = UNKNOWN;
static int HPGL_y = UNKNOWN;
static int HPGL_penstate = UNKNOWN;
static int HPGL_pentype;

/* pen */
static int HPGL2_pentype = LT_UNDEFINED;
static int HPGL2_pen = 1;
/* (poly-) lines */
static double HPGL2_lw = 0;
static bool HPGL2_in_pe = FALSE;
static bool HPGL2_lost;
/* point symbols */
static double HPGL2_psize = 1.0; /* Default point size */
/* font/text */
static int HPGL2_font_num_current = 0; /* current font */
static double HPGL2_point_size_current = 0; /* current pointsize */
static double HPGL2_is_italic = FALSE;
static double HPGL2_is_bold = FALSE;
static enum JUSTIFY HPGL2_justification = LEFT;
/* enhanced text */
static double HPGL2_base; /* distance above initial baseline */
static double HPGL2_base_save; /* saved baseline for overprint=3/4 */
static bool HPGL2_sizeonly; /* first pass for right/center aligned text */
static bool HPGL2_opened_string;
static bool HPGL2_show;
static int HPGL2_overprint;
static bool HPGL2_widthflag;
static float HPGL2_enh_fontscale = 1;

/*
 * The subroutines, grouped by function for different versions.
 */

static int HPGL_numpen;
static int HPGL_eject;

TERM_PUBLIC void HPGL_options(GpTermEntry_Static * pThis, GnuPlot * pGp)
{
	HPGL_numpen = 6; /* default to six pens */
	HPGL_eject = 0; /* default to no eject */
	while(!pGp->Pgm.EndOfCommand()) {
		if(pGp->Pgm.AlmostEqualsCur("eje$ct"))
			HPGL_eject = 1;
		else if(pGp->Pgm.IsANumber(pGp->Pgm.GetCurTokenIdx())) {
			HPGL_numpen = (int)pGp->Real(&pGp->Pgm.P_Token[pGp->Pgm.CToken].LVal);
			if(HPGL_numpen <= 0) {
				HPGL_numpen = 6;
				pGp->IntErrorCurToken("Number of pens must be positive");
			}
		}
		else
			pGp->IntErrorCurToken("expecting \"eject\" or number of pens");
		pGp->Pgm.Shift();
	}
	slprintf(GPT._TermOptions, "%d pens %s", HPGL_numpen, HPGL_eject ? "eject" : "noeject");
}

static int PCL_landscape = TRUE;
static int HPGL2_numpen = 8; // default to 8 pen color 
static bool HPGL2_pspointset = TRUE; // default to PS point types 
static int HPGL2_font_num = 0; // font from options 
static double HPGL2_point_size = HPGL2_DEF_POINT; // pointsize from options 
static const char * PCL_dim = "letter"; // default plotting dimensions 
static char PCL_dim_buf[256];
static float HPGL2_fontscale = 1.0;
static float HPGL2_pointscale = 1.0;
static float HPGL2_linewidth_scale = 1.0;
static bool HPGL2_rounded = FALSE;

enum PCL_id {
	HPGL2_COLOR,
	HPGL2_FONT, HPGL2_SIZE,
	HPGL2_ENHANCED, HPGL2_NOENHANCED,
	HPGL2_NOPSPOINTS, HPGL2_PSPOINTS,
	HPGL2_ROUNDED, HPGL2_NOROUNDED,
	HPGL2_LINEWIDTH, HPGL2_POINTSIZE, HPGL2_FONTSCALE,
	PCL_PORTRAIT, PCL_LANDSCAPE,
	PCL_EXTENDED, PCL_NOEXTENDED, PCL_LEGAL, PCL_LETTER, PCL_A4,
	PCL_INVALID
};

static struct gen_table PCL_opts[] =
{
	{ "col$ors", HPGL2_COLOR },
	{ "col$ours", HPGL2_COLOR },
	{ "font", HPGL2_FONT },
	{ "n$opspoints", HPGL2_NOPSPOINTS },
	{ "psp$oints", HPGL2_PSPOINTS },
	{ "butt", HPGL2_NOROUNDED },
	{ "rou$nded", HPGL2_ROUNDED },
	{ "lw", HPGL2_LINEWIDTH },
	{ "linew$idth", HPGL2_LINEWIDTH },
	{ "ps", HPGL2_POINTSIZE },
	{ "points$ize", HPGL2_POINTSIZE },
	{ "fs", HPGL2_FONTSCALE },
	{ "fonts$cale", HPGL2_FONTSCALE },
	{ "po$rtrait", PCL_PORTRAIT },
	{ "la$ndscape", PCL_LANDSCAPE },
	{ "size", HPGL2_SIZE },
	{ "enh$anced", HPGL2_ENHANCED },
	{ "noenh$anced", HPGL2_NOENHANCED },
	{ NULL, PCL_INVALID }
};

static struct gen_table PCL_size_opts[] =
{
	{ "ext$ended", PCL_EXTENDED },
	{ "noext$ended", PCL_NOEXTENDED },
	{ "leg$al", PCL_LEGAL },
	{ "let$ter", PCL_LETTER },
	{ "a4", PCL_A4 },
	{ NULL, PCL_INVALID }
};

static bool almost_equal_string(const char * test, const char * str)
{
	int i, j;
	int after = 0;
	int start = 0;
	int length = strlen(test);
	if(test == NULL || str == NULL)
		return FALSE;
	for(i = 0, j = 0; i < length + after; i++, j++) {
		// ignore spaces and underscores 
		while(str[i] == ' ' || str[i] == '_' || str[i] == '-') i++;
		while(test[start + j] == ' ' || test[start + j] == '_' || test[start + j] == '-') j++;
		if(toupper(str[i]) != toupper(test[start + j])) {
			if(str[i] != '$') {
				return FALSE;
			}
			else {
				after = 1;
				start--; // back up token ptr 
			}
		}
	}
	// i now beyond end of test string 
	return (after || str[i] == '$' || !str[i]);
}

TERM_PUBLIC void PCL_options(GpTermEntry_Static * pThis, GnuPlot * pGp)
{
	int i;
	SETIFZ(PCL_mode.name, PCL_mode_table[0].name);
	while(!pGp->Pgm.EndOfCommand()) {
		switch(pGp->Pgm.LookupTableForCurrentToken(PCL_opts)) {
			case HPGL2_COLOR:
			    pGp->Pgm.Shift();
			    HPGL2_numpen = pGp->Pgm.EndOfCommand() ? 8 : pGp->IntExpression();
			    if(HPGL2_numpen < 2) {
				    HPGL2_numpen = 8;
				    pGp->IntErrorCurToken("Number of pens must be larger than two.");
			    }
			    break;
			case HPGL2_FONT: {
			    char * s;
			    pGp->Pgm.Shift();
			    if(pGp->IsStringValue(pGp->Pgm.GetCurTokenIdx()) && (s = pGp->TryToGetString())) {
				    double fontsize;
				    char * comma = sstrrchr(s, ',');
				    if(comma && (sscanf(comma + 1, "%lf", &fontsize) == 1)) {
					    HPGL2_point_size = fontsize;
					    *comma = '\0';
				    }
				    else {
					    HPGL2_point_size = HPGL2_DEF_POINT;
				    }
				    HPGL2_point_size_current = HPGL2_point_size;
				    if(*s) {
					    for(i = 0; i < HPGL2_FONTS && !almost_equal_string(s, HPGL2_font_table[i].compare) && !almost_equal_string(s, HPGL2_font_table[i].alt); i++)
							;
					    if(i < HPGL2_FONTS) {
						    HPGL2_font = &HPGL2_font_table[i];
						    HPGL2_font_num_current = HPGL2_font_num = i;
					    }
					    else {
						    pGp->IntErrorCurToken("expecting font: stick, cg_times, univers, zapf_dingbats, antique_olive,\n\tarial, courier, garamond_antigua, letter_gothic, cg_omega, albertus,\ntimes_new_roman, clarendon, coronet, marigold, truetype_symbols, or wingdings");
					    }
				    }
				    if(HPGL2_font->spacing)
					    HPGL2_font->height = HPGL2_point_size;
				    else
					    HPGL2_font->pitch = 72 * 3 / (HPGL2_point_size * 2);
			    }
			    else {
				    pGp->IntErrorCurToken("expecting font string");
			    }
			    break;
		    }
			case HPGL2_NOPSPOINTS:
			    pGp->Pgm.Shift();
			    HPGL2_pspointset = FALSE;
			    break;
			case HPGL2_PSPOINTS:
			    pGp->Pgm.Shift();
			    HPGL2_pspointset = TRUE;
			    break;
			case HPGL2_ROUNDED:
			    pGp->Pgm.Shift();
			    HPGL2_rounded = TRUE;
			    break;
			case HPGL2_NOROUNDED:
			    pGp->Pgm.Shift();
			    HPGL2_rounded = FALSE;
			    break;
			case HPGL2_LINEWIDTH:
			    pGp->Pgm.Shift();
			    HPGL2_linewidth_scale = pGp->FloatExpression();
			    if(HPGL2_linewidth_scale <= 0)
				    HPGL2_linewidth_scale = 1.0f;
			    break;
			    break;
			case HPGL2_POINTSIZE:
			    pGp->Pgm.Shift();
			    HPGL2_pointscale =  pGp->FloatExpression();
			    if(HPGL2_pointscale <= 0)
				    HPGL2_pointscale = 1.0f;
			    break;
			case HPGL2_FONTSCALE:
			    pGp->Pgm.Shift();
			    HPGL2_fontscale =  pGp->FloatExpression();
			    if(HPGL2_fontscale <= 0)
				    HPGL2_fontscale = 1.0f;
			    break;
			case HPGL2_ENHANCED:
			    pGp->Pgm.Shift();
			    pThis->SetFlag(TERM_ENHANCED_TEXT);
			    pThis->put_text = HPGL2_enh_put_text;
			    break;
			case HPGL2_NOENHANCED:
			    pGp->Pgm.Shift();
			    pThis->ResetFlag(TERM_ENHANCED_TEXT);
			    pThis->put_text = HPGL2_put_text;
			    break;
			case PCL_LANDSCAPE:
			    pGp->Pgm.Shift();
			    PCL_landscape = TRUE;
			    PCL_mode.name = PCL_mode_table[0].name;
			    break;
			case PCL_PORTRAIT:
			    pGp->Pgm.Shift();
			    PCL_landscape = FALSE;
			    PCL_mode.name = PCL_mode_table[1].name;
			    break;
			case HPGL2_SIZE:
			    pGp->Pgm.Shift();
			    if(pGp->Pgm.EndOfCommand()) {
				    pGp->IntErrorCurToken("size argument expected");
			    }
			    else 
					switch(pGp->Pgm.LookupTableForCurrentToken(PCL_size_opts)) {
					    case PCL_EXTENDED:
						PCL_mode.xmax = (HPGL_XMAX_D);
						PCL_mode.ymax = (HPGL_YMAX_D - 60);
						PCL_dim = "extended";
						pGp->Pgm.Shift();
						break;
					    case PCL_NOEXTENDED:
						PCL_mode.xmax = (HPGL_XMAX_C);
						PCL_mode.ymax = (HPGL_YMAX_C - 60);
						PCL_dim = "noextended";
						pGp->Pgm.Shift();
						break;
					    case PCL_LEGAL:
						PCL_mode.xmax = (HPGL_XMAX_B);
						PCL_mode.ymax = (HPGL_YMAX_B - 60);
						PCL_dim = "legal";
						pGp->Pgm.Shift();
						break;
					    case PCL_LETTER:
						PCL_mode.xmax = (HPGL_XMAX_A);
						PCL_mode.ymax = (HPGL_YMAX_A - 60);
						PCL_dim = "letter";
						pGp->Pgm.Shift();
						break;
					    case PCL_A4:
						PCL_mode.xmax = (HPGL_XMAX_E);
						PCL_mode.ymax = (HPGL_YMAX_E - 60);
						PCL_dim = "a4";
						pGp->Pgm.Shift();
						break;
					    default: {
						/* no known abbreviation */
						float xmax, ymax;
						GpSizeUnits explicit_units;
						const char * unit = "in";
						float scale = 1;
						explicit_units = pGp->ParseTermSize(&xmax, &ymax, INCHES);
						PCL_mode.xmax = static_cast<uint>(xmax / GpResolution * HPGL_PUPI);
						PCL_mode.ymax = static_cast<uint>(ymax / GpResolution * HPGL_PUPI);
						if(xmax <= 0)
							PCL_mode.xmax = PCL_XMAX;
						if(ymax <= 0)
							PCL_mode.ymax = PCL_YMAX;
						switch(explicit_units) {
							case CM:
							    scale = 2.54f;
							    unit = "cm";
							    break;
/*
                    case MM:
                        scale = 25.4f;
                        unit = "mm";
                        break;
 */
							case INCHES:
							default:
							    break;
						}
						sprintf(PCL_dim_buf, "%.2f%s, %.2f%s", PCL_mode.xmax / HPGL_PUPI / scale, unit, PCL_mode.ymax / HPGL_PUPI / scale, unit);
						PCL_dim = PCL_dim_buf;
					}
				    }
			    break;
			default:
			    pGp->IntErrorCurToken("unrecognized terminal option");
			    break;
		}
	}
	slprintf(GPT._TermOptions, "%senhanced %s size %s %s %d font \"%s, %.1f\" %s %s linewidth %.1f pointsize %.1f fontscale %.1f",
	    (pThis->flags & TERM_ENHANCED_TEXT) ? "" : "no", PCL_mode.name, PCL_dim, "color", HPGL2_numpen,
	    HPGL2_font->name, HPGL2_point_size, HPGL2_pspointset ? "pspoints" : "nopspoints", HPGL2_rounded ? "rounded" : "butt",
	    HPGL2_linewidth_scale, HPGL2_pointscale, HPGL2_fontscale);
}

TERM_PUBLIC void HPGL_init(GpTermEntry_Static * pThis)
{
}

/* void HPGL2_init ()
   {
   } */

TERM_PUBLIC void PCL_init(GpTermEntry_Static * pThis)
{
	//
	// Reset printer, set to one copy, orientation of user's choice.
	// Make the change to the new orientation all at once.
	//
	if(PCL_landscape) {
		fprintf(GPT.P_GpOutFile, "\033E\033&l1X%s\n", PCL_mode_table[0].command);
		pThis->SetMax(PCL_mode.xmax, PCL_mode.ymax);
	}
	else {
		fprintf(GPT.P_GpOutFile, "\033E\033&l1X%s\n", PCL_mode_table[1].command);
		pThis->SetMax(PCL_mode.ymax, PCL_mode.xmax);
	}
	if(GPT._Encoding == S_ENC_UTF8)
		fputs("\033&t83P\n", GPT.P_GpOutFile);
	//
	// Enter HPGL/2 graphics mode
	//
	fputs("\033%0B", GPT.P_GpOutFile);
}

TERM_PUBLIC void HPGL_graphics(GpTermEntry_Static * pThis)
{
	fputs("\033.Y\n\033.I81;;17:\033.N;19:\033.M500:\n", GPT.P_GpOutFile);
/*	       1
        1. enable eavesdropping
 */
	fprintf(GPT.P_GpOutFile, "IN;%s\nSC0,%d,0,%d;\nSR%f,%f;\n", oneof2(GPT._Encoding, S_ENC_CP850, S_ENC_ISO8859_1) ? "CA7;" : "",
	    HPGL_XMAX, HPGL_YMAX, ((double)(HPGL_HCHAR) * 200 / 3 / HPGL_XMAX), ((double)(HPGL_VCHAR) * 100 / 2 / HPGL_YMAX));
/*	 1    2             3
        1. reset to power-up defaults
        2. set SCaling
        3. set character size
 */
	HPGL_ang = 0;
}

static int HPGL2_map_encoding()
{
	// we only remap Roman-8 symbol set
	if(HPGL2_font->symbol_set != 277)
		return HPGL2_font->symbol_set;
	switch(GPT._Encoding) {
		case S_ENC_UTF8:
		// FIXME: Fall through. That way at least the some one byte codes map correctly. 
		// return 590; 
		case S_ENC_ISO8859_1:
		    return 14;
		case S_ENC_ISO8859_2:
		    return 78;
		case S_ENC_ISO8859_9:
		    return 174;
		case S_ENC_ISO8859_15:
		    return 302;
		case S_ENC_CP437:
		    return 341;
		case S_ENC_CP850:
		    return 405; /* 437 - CP858 */
		case S_ENC_CP852:
		    return 565;
		case S_ENC_CP1250:
		    return 193; /* Windows 3.1 Latin 2 */
		case S_ENC_CP1251:
		    return 306; /* Windows 3.1 Latin/Cyrillic */
		case S_ENC_CP1252:
		    return 629; /* Windows 3.1 Latin 1 */
		case S_ENC_CP1254:
		    return 180; /* Windows 3.1 Latin 5 */
		/* Cannot handle
		    S_ENC_CP950, S_ENC_KOI8_R, S_ENC_KOI8_U, S_ENC_SJIS
		 */
		case S_ENC_DEFAULT:
		default:
		    return 277; /* HP Roman-8 */
	}
}

TERM_PUBLIC void HPGL2_graphics(GpTermEntry_Static * pThis)
{
/*
 * IN - Initialize
 * SP - Select pen
 * SD - Set default font
 */
	pThis->SetCharSize(pThis->CV() * 2 / 3, static_cast<uint>((int)HPGL_PUPI * HPGL2_point_size * HPGL2_fontscale / 72));
	fprintf(GPT.P_GpOutFile, "INNP8SP1SD1,%d,2,%d,", HPGL2_map_encoding(), HPGL2_font->spacing);
	HPGL2_pen = 1;
	if(HPGL2_font->spacing)
		fprintf(GPT.P_GpOutFile, "4,%.1f,", HPGL2_font->height * HPGL2_fontscale);
	else
		fprintf(GPT.P_GpOutFile, "3,%.1f,", HPGL2_font->pitch * HPGL2_fontscale);
	fprintf(GPT.P_GpOutFile, "5,%d,6,%d,7,%d;SS;\n", HPGL2_font->posture, HPGL2_font->stroke_weight, HPGL2_font->typeface);
	if(HPGL2_rounded) /* default is mitered joins/butt ends */
		fputs("LA1,4,2,4;", GPT.P_GpOutFile);
/*
 * Add a set of user-defined dashed linetypes.
 * Of course, the UL's below can be edited to user preference.
 * LT1 is used for LT_AXIS and is always solid, LT8 is reserved for custom dash patterns.
   UL1,100;
   UL8,6,6,6,6,6,6,0,6,6,6,6,6,6,6,0,6;
 */
	fputs("\
UL2,8,8,9,8,8,9,8,8,9,8,8,9;\n\
UL3,6,6,6,7,6,6,6,7,6,6,6,7,6,6,6,7;\n\
UL4,5,5,5,10,5,5,5,10,5,5,5,10;\n\
UL5,5,5,5,5,5,8,5,5,5,5,5,8,5,5,5,5,5,9;\n\
UL6,8,8,0,9,8,8,0,9,8,8,0,9;\n\
UL7,4,4,4,4,0,4,4,4,4,4,0,4,4,4,4,4,0,4;\n",
	    GPT.P_GpOutFile);
/*
 * Control variables
 */
	HPGL_ang = 0; /* Horizontal */
	HPGL2_is_bold = HPGL2_is_italic = FALSE;
	HPGL2_justification = LEFT;
	HPGL2_in_pe = FALSE; /* Not in PE command */
	HPGL2_lost = TRUE; /* Pen position is unknown */
	HPGL_penstate = UP; /* Pen is up */
}

TERM_PUBLIC void PCL_graphics(GpTermEntry_Static * pThis)
{
/*
 * Enter HPGL/2 graphics mode
 */
	fputs("\033%0B", GPT.P_GpOutFile);
	HPGL2_graphics(pThis);
}

TERM_PUBLIC void HPGL_text(GpTermEntry_Static * pThis)
{
	if(HPGL_eject == 0) {
		fputs("PUSP0;\033.Z\n\0", GPT.P_GpOutFile);
/*		 1 2   3
        1. pen up
        2. park pen
        3. disable eavesdropping
 */
	}
	else {
		fputs("PUSP0;PG;\033.Z\n\0", GPT.P_GpOutFile);
/*		 1 2   3  4
        1. pen up
        2. park pen
        3. page eject
        4. disable eavesdropping
 */
	}
	HPGL_penstate = UP;
}

#if 0                           /* not used */
void HPGL2_text(GpTermEntry_Static * pThis)
{
	HPGL2_end_poly();
/*
 * Pen up, park pen
 */
	fputs("PUSP0;", GPT.P_GpOutFile);
	HPGL_penstate = UP;
}

#endif

TERM_PUBLIC void PCL_text(GpTermEntry_Static * pThis)
{
	HPGL2_end_poly();
	// Go into PCL mode and eject the page
	fputs("\033%1A\033&l0H\n\0", GPT.P_GpOutFile);
}

TERM_PUBLIC void HPGL_linetype(GpTermEntry_Static * pThis, int linetype)
{
	if(linetype < -2)
		linetype = LT_BLACK;
/* allow for set number of pens */
	linetype = (linetype + 2) % HPGL_numpen + 1;
/* only select pen if necessary */
	if(HPGL_pentype != linetype) {
		fprintf(GPT.P_GpOutFile, "PU;\nSP%d;\n", linetype);
		HPGL_pentype = linetype;
		HPGL_penstate = UP;
	}
}

TERM_PUBLIC void HPGL2_linetype(GpTermEntry_Static * pThis, int linetype)
{
	t_colorspec colorspec;
	HPGL2_end_poly();
	// set color according to linetype first 
	colorspec.type = TC_LT;
	colorspec.lt = linetype;
	HPGL2_set_color(pThis, &colorspec);
	//  now only adjust the width and line style 
	// only select pen if necessary  // FIXME: need more checks than only linetype 
	// if (linetype != HPGL2_pentype) { 
	if(linetype >= 0) {
		fprintf(GPT.P_GpOutFile, "PW%.2f;\nLT;", HPGL2_lw);
		/* Borders and Tics */
	}
	else if(linetype == LT_BLACK) {
		fprintf(GPT.P_GpOutFile, "PW%.2f;\nLT", HPGL2_lw);
		/* Axes and Grids */
	}
	else if(linetype == LT_AXIS) {
		fprintf(GPT.P_GpOutFile, "PW%.2f;\nLT1,.25", HPGL2_lw);
	}
	else if(linetype <= LT_NODRAW) {
		fprintf(GPT.P_GpOutFile, "PW%.2f;\nLT", HPGL2_lw);
	}
	HPGL_penstate = UP;
	HPGL2_pentype = linetype;
	/* } */
}

TERM_PUBLIC void HPGL2_dashtype(GpTermEntry_Static * pThis, int type, t_dashtype * custom_dash_pattern)
{
	HPGL2_end_poly();
	if(type > 0) { /* predefined linetype */
		type = type % 6 + 1;
		if(type == 1)
			fputs("LT;", GPT.P_GpOutFile);
		else /* range 2..7 as defined above */
			fprintf(GPT.P_GpOutFile, "LT%d,%d", type, (HPGL2_lw > 0 ? (int)(2 * HPGL2_lw / 0.25) : 2));
	}
	else switch(type) {
			case DASHTYPE_AXIS:
			    fputs("LT1,.25", GPT.P_GpOutFile);
			    break;
			case 0:
			case DASHTYPE_SOLID:
			    fputs("LT;", GPT.P_GpOutFile);
			    break;
			case DASHTYPE_CUSTOM: {
			    int i, count = 0;
			    float len = 0.0;
			    fputs("UL8", GPT.P_GpOutFile);
			    // normalize total pattern length to 100 
			    while((custom_dash_pattern->pattern[count] != 0.) && (count < DASHPATTERN_LENGTH)) {
				    len += custom_dash_pattern->pattern[count];
				    count++;
			    }
			    if(!len) 
					len = 1.0;
			    for(i = 0; i < count; i++) {
				    fprintf(GPT.P_GpOutFile, ",%d", (int)(100 * custom_dash_pattern->pattern[i] / len + 0.5));
			    }
			    fprintf(GPT.P_GpOutFile, "LT%d,%d", 8, (HPGL2_lw > 0 ? (int)(2 * HPGL2_lw / 0.25) : 2));
			    break;
		    }
		}
}

TERM_PUBLIC void HPGL_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str)
{
	if(HPGL_ang == 1)
		HPGL_move(pThis, x + HPGL_VCHAR / 4, y);
	else
		HPGL_move(pThis, x, y - HPGL_VCHAR / 4);
	if(GPT._Encoding == S_ENC_CP850) {
		uchar * s;
		fputs("LB", GPT.P_GpOutFile);
		for(s = (uchar *)str; *s; ++s)
			if(*s >= 128 && hpgl_cp_850[*s - 128][0])
				fputs(hpgl_cp_850[*s - 128], GPT.P_GpOutFile);
			else
				putc(*s, GPT.P_GpOutFile);
		fputs("\003\n", GPT.P_GpOutFile);
	}
	else if(GPT._Encoding == S_ENC_ISO8859_1) {
		uchar * s;
		fputs("LB", GPT.P_GpOutFile);
		for(s = (uchar *)str; *s; ++s)
			if(*s >= 128 && hpgl_iso_8859_1[*s - 128][0])
				fputs(hpgl_iso_8859_1[*s - 128], GPT.P_GpOutFile);
			else
				putc(*s, GPT.P_GpOutFile);
		fputs("\003\n", GPT.P_GpOutFile);
	}
	else
		fprintf(GPT.P_GpOutFile, "LB%s\003\n", str);
}

TERM_PUBLIC void HPGL2_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str)
{
	// Position the pen
	HPGL2_move(pThis, x, y);
	HPGL2_end_poly();
	HPGL2_put_text_here(str, TRUE);
}

static void HPGL2_put_text_here(const char * str, bool centeralign)
{
/*
 * Print the text string
 */
	if((GPT._Encoding == S_ENC_UTF8) && contains8bit(str) && ((HPGL_ang % 90) == 0)) {
		// EXPERIMENTAL
		/* Unfortunately UTF-8 output is only documented in PCL mode, but not in HP-GL/2 mode.
		   Hence, we switch back to PCL for this, set font properties etc. and print the text.
		   Regrettably, text output is very limited: no vertical center alignment, no arbitrary
		   angle and text justification only with tricks.
		 */
		/* Aim to correct vertical alignment, may or may not be "OK" for current font.
		   This is not required for enhanced text since there we do baseline alignment
		   in HPGL/2 mode, too. */
		if(centeralign)
			fputs("CP0,-0.3\n", GPT.P_GpOutFile);
		/* PCL mode, current HP-GL/2 position */
		fputs("\033%1A", GPT.P_GpOutFile);
		/* Symbol Set 18N=UTF-8 */
		fputs("\033(18N", GPT.P_GpOutFile);
		/* Set PCL font */
		fprintf(GPT.P_GpOutFile, "\033(s%dp", HPGL2_font->spacing);
		if(HPGL2_font->spacing)
			fprintf(GPT.P_GpOutFile, "%.2fv", HPGL2_point_size_current * HPGL2_fontscale);
		else
			fprintf(GPT.P_GpOutFile, "%.2fh", HPGL2_font->pitch * HPGL2_fontscale);
		fprintf(GPT.P_GpOutFile, "%ds", HPGL2_is_italic ? HPGL2_font->italic : HPGL2_font->posture);
		fprintf(GPT.P_GpOutFile, "%db", HPGL2_is_bold ? HPGL2_font->bold : HPGL2_font->stroke_weight);
		fprintf(GPT.P_GpOutFile, "%dT", HPGL2_font->typeface);
		/* Text parsing method 83=UTF-8 */
		fputs("\033&t83P", GPT.P_GpOutFile);
		/* Tricky text justification */
		switch(HPGL2_justification) {
			case LEFT:
			    /* set text angle only */
			    fprintf(GPT.P_GpOutFile, "\033&a%dP", HPGL_ang % 360);
			    break;
			case CENTRE:
			    /* "ghost" printing the text:
			       transparent pattern, white pattern fill, upside-down */
			    fprintf(GPT.P_GpOutFile, "\033*vo1T\033&a%dP", (180 + HPGL_ang) % 360);
			    /* half font size to locate center */
			    if(HPGL2_font->spacing) {
				    fprintf(GPT.P_GpOutFile, "\033(s%.2fV", HPGL2_point_size_current * HPGL2_fontscale / 2);
				    fputs(str, GPT.P_GpOutFile);
				    fprintf(GPT.P_GpOutFile, "\033(s%.2fV", HPGL2_point_size_current * HPGL2_fontscale);
			    }
			    else {
				    fprintf(GPT.P_GpOutFile, "\033(s%.2fH", HPGL2_font->pitch * HPGL2_fontscale / 2);
				    fputs(str, GPT.P_GpOutFile);
				    fprintf(GPT.P_GpOutFile, "\033(s%.2fH", HPGL2_font->pitch * HPGL2_fontscale);
			    }
			    fprintf(GPT.P_GpOutFile, "\033*v1oT\033&a%dP", HPGL_ang % 360);
			    break;
			case RIGHT:
			    /* "ghost" printing the text:
			       transparent pattern, white pattern fill, upside-down */
			    fprintf(GPT.P_GpOutFile, "\033*vo1T\033&a%dP", (180 + HPGL_ang) % 360);
			    fputs(str, GPT.P_GpOutFile);
			    fprintf(GPT.P_GpOutFile, "\033*v1oT\033&a%dP", HPGL_ang % 360);
			    break;
		}
		/* Color palettes are preserved when switching from and to PCL,
		   so we can just refer to the HP-GL/2 pen to set the PCL color. */
		fprintf(GPT.P_GpOutFile, "\033*v%dS", HPGL2_pen);
		/* If this is the first of two passes of enhanced mode printing,
		   only move the cursor */
		if(HPGL2_sizeonly)
			fputs("\033*vo1T", GPT.P_GpOutFile);
		/* Here comes the actual text... */
		fputs(str, GPT.P_GpOutFile);
		if(HPGL2_sizeonly)
			fputs("\033*v1oT", GPT.P_GpOutFile);
		/* HP-GL/2 mode, current PCL position */
		fputs("\033%1B\n", GPT.P_GpOutFile);
		/* restore baseline */
		if(centeralign)
			fputs("CP0,0.3\n", GPT.P_GpOutFile);
	}
	else {
		fprintf(GPT.P_GpOutFile, "LB%s\003\n", str);
	}
	HPGL2_lost = TRUE;
}
/*
 * Some early HPGL plotters (e.g. HP7220C) require the
 * Pen Up/Down and Pen (move) Absolute commands to be separate.
 */
TERM_PUBLIC void HPGL_move(GpTermEntry_Static * pThis, uint x, uint y)
{
	if(HPGL_x != x || HPGL_y != y) { /* only move if necessary */
		fprintf(GPT.P_GpOutFile, "PU;PA%d,%d;\n", x, y);
		HPGL_penstate = UP;
		HPGL_x = x;
		HPGL_y = y;
	}
}

TERM_PUBLIC void HPGL_vector(GpTermEntry_Static * pThis, uint x, uint y)
{
	if(HPGL_penstate != DOWN) {
		fprintf(GPT.P_GpOutFile, "PD;PA%d,%d;\n", x, y);
		HPGL_penstate = DOWN;
	}
	else
		fprintf(GPT.P_GpOutFile, "PA%d,%d;\n", x, y);
	HPGL_x = x;
	HPGL_y = y;
}

TERM_PUBLIC void HPGL2_move(GpTermEntry_Static * pThis, uint x, uint y)
{
	int dx, dy;
	if(HPGL2_in_pe) {
		dx = x - HPGL_x;
		dy = y - HPGL_y;
		fputs("<", GPT.P_GpOutFile);
	}
	else {
#if HPGL2_BASE64
		fputs("PE<", GPT.P_GpOutFile);
#else
		fputs("PE7<", GPT.P_GpOutFile);
#endif
		if(HPGL2_lost) {
			dx = x;
			dy = y;
			HPGL2_lost = FALSE;
			fputs("=", GPT.P_GpOutFile);
		}
		else {
			dx = x - HPGL_x;
			dy = y - HPGL_y;
		}
		HPGL2_in_pe = TRUE;
	}
#if HPGL2_EXPLICIT_PD
	if(HPGL_penstate == DOWN)
		HPGL_penstate = UP;
#endif
	HPGL2_encode(dx);
	HPGL2_encode(dy);
	fputs("\n", GPT.P_GpOutFile);
	HPGL_x = x;
	HPGL_y = y;
}

TERM_PUBLIC void HPGL2_vector(GpTermEntry_Static * pThis, uint x, uint y)
{
	int dx, dy;
	if(HPGL2_in_pe) {
		dx = x - HPGL_x;
		dy = y - HPGL_y;
	}
	else {
#if HPGL2_BASE64
		fputs("PE", GPT.P_GpOutFile);
#else
		fputs("PE7", GPT.P_GpOutFile);
#endif
		if(HPGL2_lost) {
			dx = x;
			dy = y;
			HPGL2_lost = FALSE;
			fputs("=", GPT.P_GpOutFile);
		}
		else {
			dx = x - HPGL_x;
			dy = y - HPGL_y;
		}
		HPGL2_in_pe = TRUE;
	}
#if HPGL2_EXPLICIT_PD
	// Put the pen down in the current position, relative vector of 0,0.
	if(HPGL_penstate == UP) {
		fputc((char)HPGL2_HIGH_OFFS, GPT.P_GpOutFile);
		fputc((char)HPGL2_HIGH_OFFS, GPT.P_GpOutFile);
		HPGL_penstate = DOWN;
	}
#endif
	HPGL2_encode(dx);
	HPGL2_encode(dy);
	fputs("\n", GPT.P_GpOutFile);
	HPGL_x = x;
	HPGL_y = y;
}

static FORCEINLINE void HPGL2_move_R(GpTermEntry_Static * pThis, double x, double y) { return HPGL2_move(pThis, static_cast<uint>(x), static_cast<uint>(y)); }
static FORCEINLINE void HPGL2_vector_R(GpTermEntry_Static * pThis, double x, double y) { return HPGL2_vector(pThis, static_cast<uint>(x), static_cast<uint>(y)); }
static FORCEINLINE void HPGL2_vector_I(GpTermEntry_Static * pThis, int x, int y) { return HPGL2_vector(pThis, static_cast<uint>(x), static_cast<uint>(y)); }
/*
 * Routine to encode position in base 32 or base 64 characters
 */
TERM_PUBLIC void HPGL2_encode(int d)
{
	int c;
	if((d <<= 1) < 0)
		d = 1 - d;
	do {
		c = d & HPGL2_MASK;
		d >>= HPGL2_BITS;
		if(d > 0)
			fputc((char)(c + HPGL2_LOW_OFFS), GPT.P_GpOutFile);
		else
			fputc((char)(c + HPGL2_HIGH_OFFS), GPT.P_GpOutFile);
	} while(d > 0);
}

static void HPGL2_end_poly()
{
/*
 * If in Polyline Encoded command, leave Polyline Encoded command
 */
	if(HPGL2_in_pe) {
		fputs(";\n", GPT.P_GpOutFile);
		HPGL2_in_pe = FALSE;
	}
}

TERM_PUBLIC int HPGL_text_angle(GpTermEntry_Static * pThis, int ang)
{
	HPGL_ang = (ang == -90 || ang == 270) ? -1 : (ang ? 1 : 0);
	if(HPGL_ang == 0)               /* Horizontal */
		fputs("DI1,0;\n", GPT.P_GpOutFile);
	else if(HPGL_ang == -1)         /* Vertical Down */
		fputs("DI0,-1;\n", GPT.P_GpOutFile);
	else                            /* Vertical Up */
		fputs("DI0,1;\n", GPT.P_GpOutFile);
	return TRUE;
}

TERM_PUBLIC int HPGL2_text_angle(GpTermEntry_Static * pThis, int ang)
{
	HPGL2_end_poly();
	while(ang < 0) {
		ang += 360;
	}
	ang %= 360;
	HPGL_ang = ang;
	switch(ang) {
		case 0: fputs("DI1,0", GPT.P_GpOutFile); break; /* Horizontal */
		case 45: fputs("DI1,1", GPT.P_GpOutFile); break;
		case 90: fputs("DI0,1", GPT.P_GpOutFile); break; /* Vertical Up */
		case 180: fputs("DI-1,0", GPT.P_GpOutFile); break;
		case 270: fputs("DI0,-1", GPT.P_GpOutFile); break; /* Vertical Down */
		case 315: fputs("DI1,-1", GPT.P_GpOutFile); break;
		default:
		    fprintf(GPT.P_GpOutFile, "DI%d,%d", (int)(100 * cos(ang * SMathConst::PiDiv180) + 0.5), (int)(100 * sin(ang * SMathConst::PiDiv180) + 0.5));
	}
	return TRUE;
}

TERM_PUBLIC void HPGL_reset(GpTermEntry_Static * pThis)
{
/*
 * do nothing
 */
}

#if 0
void HPGL2_reset()
{
/*
 * Park the pen
 * Advance a page
 * End with ";"
 */
	fputs("SP0PG;\n", GPT.P_GpOutFile);
}

#endif

TERM_PUBLIC void PCL_reset(GpTermEntry_Static * pThis)
{
	/*
	 * Return to PCL mode
	 * Printer reset (conditional eject)
	 */
	fputs("\033%0A\033E\n", GPT.P_GpOutFile);
}

TERM_PUBLIC int HPGL2_justify_text(GpTermEntry_Static * pThis, enum JUSTIFY just)
{
	HPGL2_end_poly();
	HPGL2_justification = just;
	switch(just) {
		case LEFT: fputs("LO2", GPT.P_GpOutFile); break;
		case CENTRE: fputs("LO5", GPT.P_GpOutFile); break;
		case RIGHT: fputs("LO8", GPT.P_GpOutFile); break;
		default: return 0;
	}
	return 1;
}

TERM_PUBLIC int HPGL2_set_font(GpTermEntry_Static * pThis, const char * font)
{
	char name[MAX_ID_LEN+1];
	int sep;
	int int_size;
	double size;
	SETIFZ(font, "");
	sep = strcspn(font, ",");
	strncpy(name, font, sizeof(name)-1);
	if(sep < sizeof(name))
		name[sep] = '\0';
	// determine font size, use default from options if invalid 
	int_size = 0;
	// FIXME: use strtod instead 
	sscanf(&(font[sep + 1]), "%d", &int_size);
	if(int_size > 0)
		size = int_size;
	else
		size = HPGL2_point_size;
	return HPGL2_set_font_size(pThis, name, size);
}

static int HPGL2_set_font_size(GpTermEntry_Static * pThis, const char * font, double size)
{
	//struct GpTermEntry * t = term;
	int i;
	bool italic = FALSE;
	bool bold = FALSE;
	char * p1;
	char * p2;
	double scale = HPGL2_fontscale * HPGL2_enh_fontscale;
	HPGL2_end_poly();
	// determine font, use default from options if invalid 
	if((p1 = (char *)strstr(font, ":Italic")) != NULL) // @badcast
		italic = TRUE;
	if((p2 = (char *)strstr(font, ":Bold")) != NULL) // @badcast
		bold = TRUE;
	ASSIGN_PTR(p1, '\0');
	ASSIGN_PTR(p2, '\0');
	for(i = 0; i < HPGL2_FONTS && !almost_equal_string(font, HPGL2_font_table[i].compare) && !almost_equal_string(font, HPGL2_font_table[i].alt); i++)
		;
	if(i >= HPGL2_FONTS)
		i = HPGL2_font_num;
	// apply font changes only if necessary 
	if(size == HPGL2_point_size_current && i == HPGL2_font_num_current && italic == LOGIC(HPGL2_is_italic) && bold == LOGIC(HPGL2_is_bold))
		return FALSE;
	HPGL2_font = &HPGL2_font_table[i];
	HPGL2_font_num_current = i;
	HPGL2_point_size_current = size;
	HPGL2_is_italic = italic;
	HPGL2_is_bold = bold;
	pThis->SetCharSize(pThis->CV() * 2 / 3, static_cast<uint>(HPGL_PUPI * HPGL2_point_size_current * scale / 72));
	fprintf(GPT.P_GpOutFile, "SD1,%d,2,%d,", HPGL2_map_encoding(), HPGL2_font->spacing);
	if(HPGL2_font->spacing) {
		HPGL2_font->height = HPGL2_point_size_current;
		fprintf(GPT.P_GpOutFile, "4,%.1f,", HPGL2_font->height * scale);
	}
	else {
		HPGL2_font->pitch = 72 * 3 / (HPGL2_point_size_current * 2);
		fprintf(GPT.P_GpOutFile, "3,%.1f,", HPGL2_font->pitch * scale);
	}
	fprintf(GPT.P_GpOutFile, "5,%d,6,%d,7,%d;SS;\n", italic ? HPGL2_font->italic : HPGL2_font->posture,
	    bold ? HPGL2_font->bold : HPGL2_font->stroke_weight, HPGL2_font->typeface);
	return TRUE;
}

static void HPGL2_dot(GpTermEntry_Static * pThis, int x, int y)
{
	HPGL2_move(pThis, x, y);
	HPGL2_vector(pThis, x, y);
}

static void HPGL2_diamond(GpTermEntry_Static * pThis, int x, int y, int htic, int vtic)
{
	HPGL2_move(pThis, x - htic, y);
	HPGL2_vector(pThis, x, y - vtic);
	HPGL2_vector(pThis, x + htic, y);
	HPGL2_vector(pThis, x, y + vtic);
	HPGL2_vector(pThis, x - htic, y);
}

static void HPGL2_filled_diamond(GpTermEntry_Static * pThis, int x, int y, int htic, int vtic)
{
	HPGL2_move(pThis, x - htic, y);
	HPGL2_end_poly();
	fputs("PM0;\n", GPT.P_GpOutFile);
	HPGL2_vector(pThis, x, y - vtic);
	HPGL2_vector(pThis, x + htic, y);
	HPGL2_vector(pThis, x, y + vtic);
	HPGL2_vector(pThis, x - htic, y);
	HPGL2_end_poly();
	fputs("PM2;FP;\n", GPT.P_GpOutFile);
}

static void HPGL2_pentagon(GpTermEntry_Static * pThis, int x, int y, int htic, int vtic)
{
	HPGL2_move(pThis, x, y + (3 * vtic / 4));
	HPGL2_vector_R(pThis, x - (cos(0.1 * acos(-1)) * 3 * htic / 4), y + (sin(0.1 * acos(-1)) * 3 * vtic / 4));
	HPGL2_vector_R(pThis, x - (sin(0.2 * acos(-1)) * 3 * htic / 4), y - (cos(0.2 * acos(-1)) * 3 * vtic / 4));
	HPGL2_vector_R(pThis, x + (sin(0.2 * acos(-1)) * 3 * htic / 4), y - (cos(0.2 * acos(-1)) * 3 * vtic / 4));
	HPGL2_vector_R(pThis, x + (cos(0.1 * acos(-1)) * 3 * htic / 4), y + (sin(0.1 * acos(-1)) * 3 * vtic / 4));
	HPGL2_vector_R(pThis, x, y + (3 * vtic / 4));
}

TERM_PUBLIC void HPGL2_point(GpTermEntry_Static * pThis, uint x, uint y, int number)
{
	int htic, vtic;
	int htic2, vtic2;
	// make sure that we use a solid line type 
	HPGL2_end_poly();
	fputs("LT;", GPT.P_GpOutFile);
	if(HPGL2_pspointset) {          /* postscript style points */
		if(number >= 100) {
			HPGL2_neg_point(pThis, x, y, number - 120);
		}
		else {
			htic = (int)(HPGL2_psize * PCL_HTIC / 2);
			vtic = (int)(HPGL2_psize * PCL_VTIC / 2);
			number %= HPGL2_NUM_PSPOINTS;
			switch(number) {
				case -1: /* dot */
				    HPGL2_dot(pThis, x, y);
				    break;
				case 0: /* plus */
				    HPGL2_move(pThis, x - htic, y);
				    HPGL2_vector(pThis, x + htic, y);
				    HPGL2_move(pThis, x, y - vtic);
				    HPGL2_vector(pThis, x, y + vtic);
				    break;
				case 1: /* X */
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_vector(pThis, x + htic, y + vtic);
				    HPGL2_move(pThis, x - htic, y + vtic);
				    HPGL2_vector(pThis, x + htic, y - vtic);
				    break;
				case 2: /* star */
				    HPGL2_move(pThis, x - htic, y);
				    HPGL2_vector(pThis, x + htic, y);
				    HPGL2_move(pThis, x, y - vtic);
				    HPGL2_vector(pThis, x, y + vtic);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_vector(pThis, x + htic, y + vtic);
				    HPGL2_move(pThis, x - htic, y + vtic);
				    HPGL2_vector(pThis, x + htic, y - vtic);
				    break;
				case 3: /* hollow square 1 */
				    HPGL2_dot(pThis, x, y);
				    HPGL2_move(pThis, x - (3 * htic / 4), y - (3 * vtic / 4));
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", (3 * htic / 2), (3 * vtic / 2));
				    break;
				case 4: /* solid square 1 */
				    HPGL2_move(pThis, x - (3 * htic / 4), y - (3 * vtic / 4));
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RA%.2f,%.2f;EP;\n", ((double)x + (3 * htic / 4)), ((double)y + (3 * vtic / 4)));
				    break;
				case 5: /* hollow circle 1 */
				    HPGL2_dot(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "CI%.2f;\n", ((double)3 * (htic) / 4));
				    break;
				case 6: /* solid circle 1 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%.2f,0,360;EP;\n", ((double)3 * (htic) / 4));
				    break;
				case 7: /* hollow triangle 1 */
				    HPGL2_move(pThis, x, y + (3 * vtic / 4));
				    HPGL2_vector_R(pThis, x - (3 * sqrt(3) * htic / 8), y - (3 * vtic / 8));
				    HPGL2_vector_R(pThis, x + (3 * sqrt(3) * htic / 8), y - (3 * vtic / 8));
				    HPGL2_vector(pThis, x, y + (3 * vtic / 4));
				    HPGL2_dot(pThis, x, y);
				    break;
				case 8: /* solid triangle 1 */
				    HPGL2_move(pThis, x, y + (3 * vtic / 4));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector_R(pThis, x - (3 * sqrt(3) * htic / 8), y - (3 * vtic / 8));
				    HPGL2_vector_R(pThis, x + (3 * sqrt(3) * htic / 8), y - (3 * vtic / 8));
				    HPGL2_vector(pThis, x, y + (3 * vtic / 4));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 9: /* hollow triangle 2 */
				    HPGL2_move(pThis, x, y - (3 * vtic / 4));
				    HPGL2_vector_R(pThis, x - (3 * sqrt(3) * htic / 8), y + (3 * vtic / 8));
				    HPGL2_vector_R(pThis, x + (3 * sqrt(3) * htic / 8), y + (3 * vtic / 8));
				    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
				    HPGL2_dot(pThis, x, y);
				    break;
				case 10: /* solid triangle 2 */
				    HPGL2_move(pThis, x, y - (3 * vtic / 4));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector_R(pThis, x - (3 * sqrt(3) * htic / 8), y + (3 * vtic / 8));
				    HPGL2_vector_R(pThis, x + (3 * sqrt(3) * htic / 8), y + (3 * vtic / 8));
				    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 11: /* hollow diamond 1 */
				    HPGL2_diamond(pThis, x, y, (3 * htic / 4), (3 * vtic / 4));
				    HPGL2_dot(pThis, x, y);
				    break;
				case 12: /* solid diamond 1 */
				    HPGL2_filled_diamond(pThis, x, y, (3 * htic / 4), (3 * vtic / 4));
				    break;
				case 13: /* hollow pentagon 1 */
				    HPGL2_pentagon(pThis, x, y, htic, vtic);
				    HPGL2_dot(pThis, x, y);
				    break;
				case 14: /* solid pentagon */
				    HPGL2_move(pThis, x, y + (3 * vtic / 4));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_pentagon(pThis, x, y, htic, vtic);
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 15: /* hollow circle 2 */
				    HPGL2_move(pThis, x, y + vtic);
				    HPGL2_vector(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "CI%d;\n", htic);
				    break;
				case 16: /* semisolid circle 1 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,0,90;CI%d;\n", htic, htic);
				    break;
				case 17: /* semisolid circle 2 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,90,90;CI%d;\n", htic, htic);
				    break;
				case 18: /* semisolid circle 3 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,0,180;CI%d;\n", htic, htic);
				    break;
				case 19: /* semisolid circle 4 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,180,90;CI%d;\n", htic, htic);
				    break;
				case 20: /* semisolid circle 5 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,0,90;WG%d,180,90;CI%d;\n", htic, htic, htic);
				    break;
				case 21: /* semisolid circle 6 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,90,180;CI%d;\n", htic, htic);
				    break;
				case 22: /* semisolid circle 7 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,0,270;CI%d;\n", htic, htic);
				    break;
				case 23: /* semisolid circle 8 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,270,90;CI%d;\n", htic, htic);
				    break;
				case 24: /* semisolid circle 9 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,270,180;CI%d;\n", htic, htic);
				    break;
				case 25: /* semisolid circle 10 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,90,90;WG%d,270,90;CI%d;\n", htic, htic, htic);
				    break;
				case 26: /* semisolid circle 11 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,270,270;CI%d;\n", htic, htic);
				    break;
				case 27: /* semisolid circle 12 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,180,180;CI%d;\n", htic, htic);
				    break;
				case 28: /* semisolid circle 13 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,180,270;CI%d;\n", htic, htic);
				    break;
				case 29: /* semisolid circle 14 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,90,270;CI%d;\n", htic, htic);
				    break;
				case 30: /* solid circle 2 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "WG%d,0,360;EP;\n", htic);
				    break;
				case 31: /* hollow square 2 */
				    HPGL2_move(pThis, x, y + vtic);
				    HPGL2_vector(pThis, x, y);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 32: /* semisolid square 1 */
				    HPGL2_move(pThis, x + htic, y + vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", -htic, -vtic);
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", -2 * htic, -2 * vtic);
				    break;
				case 33: /* semisolid square 2 */
				    HPGL2_move(pThis, x - htic, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, vtic);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 34: /* semisolid square 3 */
				    HPGL2_move(pThis, x - htic, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", 2 * htic, vtic);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 35: /* semisolid square 4 */
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, vtic);
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 36: /* semisolid square 5 */
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, vtic);
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, vtic);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 37: /* semisolid square 6 */
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, 2 * vtic);
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 38: /* semisolid square 7 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, vtic);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, 2 * vtic);
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 39: /* semisolid square 8 */
				    HPGL2_move(pThis, x, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, vtic);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 40: /* semisolid square 9 */
				    HPGL2_move(pThis, x, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, 2 * vtic);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 41: /* semisolid square 10 */
				    HPGL2_move(pThis, x - htic, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, vtic);
				    HPGL2_move(pThis, x, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, vtic);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 42: /* semisolid square 11 */
				    HPGL2_move(pThis, x, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, 2 * vtic);
				    HPGL2_move(pThis, x - htic, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, vtic);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 43: /* semisolid square 12 */
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;EP;\n", 2 * htic, vtic);
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 44: /* semisolid square 13 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, vtic);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;EP;\n", 2 * htic, vtic);
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 45: /* semisolid square 14 */
				    HPGL2_move(pThis, x - htic, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", htic, vtic);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;\n", 2 * htic, vtic);
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 46: /* solid square 2 */
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "RR%d,%d;EP;\n", 2 * htic, 2 * vtic);
				    break;
				case 47: /* hollow diamond 2 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_diamond(pThis, x, y, htic2, vtic2);
				    HPGL2_move(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x, y);
				    break;
				case 48: /* semisolid diamond 1 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    /* Note using filled polygon leads to rounding errors:
				       HPGL2_filled_diamond(x + (htic2 / 4), y, (htic2 / 2), y + (vtic2 / 2));
				     */
				    HPGL2_diamond(pThis, x, y, htic2, vtic2);
				    HPGL2_move(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 49: /* semisolid diamond 2 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x, y);
				    HPGL2_vector(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 50: /* semisolid diamond 3 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 51: /* semisolid diamond 4 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 52: /* semisolid diamond 5 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    HPGL2_move(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 53: /* semisolid diamond 6 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 54: /* semisolid diamond 7 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 55: /* semisolid diamond 8 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 56: /* semisolid diamond 9 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 57: /* semisolid diamond 10 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x, y);
				    HPGL2_vector(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    HPGL2_move(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 58: /* semisolid diamond 11 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x, y);
				    HPGL2_vector(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 59: /* semisolid diamond 12 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 60: /* semisolid diamond 13 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x, y);
				    HPGL2_vector(pThis, x - (htic2 / 2), y - (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 61: /* semisolid diamond 14 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x, y + vtic2);
				    HPGL2_vector(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector(pThis, x - htic2, y);
				    HPGL2_vector(pThis, x, y - vtic2);
				    HPGL2_vector(pThis, x + htic2, y);
				    HPGL2_vector(pThis, x + (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_vector(pThis, x, y);
				    HPGL2_vector(pThis, x - (htic2 / 2), y + (vtic2 / 2));
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 62: /* solid diamond 2 */
				    htic2 = static_cast<int>(htic * M_SQRT2);
				    vtic2 = static_cast<int>(vtic * M_SQRT2);
				    HPGL2_move(pThis, x - htic2, y);
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_diamond(pThis, x, y, htic2, vtic2);
				    HPGL2_end_poly();
				    fputs("PM2;FP;EP;\n", GPT.P_GpOutFile);
				    break;
				case 63: /* hollow square 3 */
				    HPGL2_move(pThis, x - (3 * htic / 4), y - (3 * vtic / 4));
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", (3 * htic / 2), (3 * vtic / 2));
				    break;
				case 64: /* hollow circle 3 */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "CI%.2f;\n", ((double)3 * (htic) / 4));
				    break;
				case 65: /* hollow triangle 3 */
				    HPGL2_move(pThis, x, y + (3 * vtic / 4));
				    HPGL2_vector_R(pThis, x - (3 * sqrt(3) * htic / 8), y - (3 * vtic / 8));
				    HPGL2_vector_R(pThis, x + (3 * sqrt(3) * htic / 8), y - (3 * vtic / 8));
				    HPGL2_vector(pThis, x, y + (3 * vtic / 4));
				    break;
				case 66: /* hollow triangle 4 */
				    HPGL2_move(pThis, x, y - (3 * vtic / 4));
				    HPGL2_vector_R(pThis, x - (3 * sqrt(3) * htic / 8), y + (3 * vtic / 8));
				    HPGL2_vector_R(pThis, x + (3 * sqrt(3) * htic / 8), y + (3 * vtic / 8));
				    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
				    break;
				case 67: /* hollow diamond 3 */
				    HPGL2_diamond(pThis, x, y, (3 * htic / 4), (3 * vtic / 4));
				    break;
				case 68: /* hollow pentagon 2 */
				    HPGL2_pentagon(pThis, x, y, htic, vtic);
				    break;
				case 69: /* opaque square */
				    HPGL2_move(pThis, x - (3 * htic / 4), y - (3 * vtic / 4));
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "FT;TR0;SP0;RR%d,%d;SP%d;EP;TR;\n", (3 * htic / 2), (3 * vtic / 2), HPGL2_pen);
				    break;
				case 70: /* opaque circle */
				    HPGL2_move(pThis, x, y);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "FT;TR0;SP0;WG%.2f,0,360;SP%d;EP;TR;\n", ((double)3 * (htic) / 4), HPGL2_pen);
				    break;
				case 71: /* opaque triangle 1 */
				    HPGL2_move(pThis, x, y + (3 * vtic / 4));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector_R(pThis, x - (3 * sqrt(3) * htic / 8), y - (3 * vtic / 8));
				    HPGL2_vector_R(pThis, x + (3 * sqrt(3) * htic / 8), y - (3 * vtic / 8));
				    HPGL2_vector(pThis, x, y + (3 * vtic / 4));
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "PM2;FT;TR0;SP0;FP;SP%d;EP;TR;\n", HPGL2_pen);
				    break;
				case 72: /* opaque triangle 2 */
				    HPGL2_move(pThis, x, y - (3 * vtic / 4));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_vector_R(pThis, x - (3 * sqrt(3) * htic / 8), y + (3 * vtic / 8));
				    HPGL2_vector_R(pThis, x + (3 * sqrt(3) * htic / 8), y + (3 * vtic / 8));
				    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "PM2;FT;TR0;SP0;FP;SP%d;EP;TR;\n", HPGL2_pen);
				    break;
				case 73: /* opaque diamond */
				    HPGL2_move(pThis, x - (3 * htic / 4), y);
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_diamond(pThis, x, y, (3 * htic / 4), (3 * vtic / 4));
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "PM2;FT;TR0;SP0;FP;SP%d;EP;TR;\n", HPGL2_pen);
				    break;
				case 74: /* opaque pentagon */
				    HPGL2_move(pThis, x, y + (3 * vtic / 4));
				    HPGL2_end_poly();
				    fputs("PM0;\n", GPT.P_GpOutFile);
				    HPGL2_pentagon(pThis, x, y, htic, vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "PM2;FT;TR0;SP0;FP;SP%d;EP;TR;\n", HPGL2_pen);
				    break;
			}
		}
	}
	else {                  /* default style points */
		if(number >= 100) {
			HPGL2_neg_point(pThis, x, y, number - 120);
		}
		else {
			htic = (int)(HPGL2_psize * PCL_HTIC / 2);
			vtic = (int)(HPGL2_psize * PCL_VTIC / 2);
			number %= HPGL2_NUM_NOPSPOINTS;
			switch(number) {
				case -1: /* dot */
				    HPGL2_dot(pThis, x, y);
				    break;
				case 0: /* plus */
				    HPGL2_move(pThis, x - htic, y);
				    HPGL2_vector(pThis, x + htic, y);
				    HPGL2_move(pThis, x, y - vtic);
				    HPGL2_vector(pThis, x, y + vtic);
				    break;
				case 1: /* X */
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_vector(pThis, x + htic, y + vtic);
				    HPGL2_move(pThis, x - htic, y + vtic);
				    HPGL2_vector(pThis, x + htic, y - vtic);
				    break;
				case 2: /* star */
				    HPGL2_move(pThis, x - htic, y);
				    HPGL2_vector(pThis, x + htic, y);
				    HPGL2_move(pThis, x, y - vtic);
				    HPGL2_vector(pThis, x, y + vtic);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_vector(pThis, x + htic, y + vtic);
				    HPGL2_move(pThis, x - htic, y + vtic);
				    HPGL2_vector(pThis, x + htic, y - vtic);
				    break;
				case 3: /* box */
				    HPGL2_dot(pThis, x, y);
				    HPGL2_move(pThis, x - htic, y - vtic);
				    HPGL2_end_poly();
				    fprintf(GPT.P_GpOutFile, "ER%d,%d;\n", 2 * htic, 2 * vtic);
				    break;
				case 4: /* triangle */
				    HPGL2_move(pThis, x, y + (4 * vtic / 3));
				    HPGL2_vector(pThis, x - (4 * htic / 3), y - (2 * vtic / 3));
				    HPGL2_vector(pThis, x + (4 * htic / 3), y - (2 * vtic / 3));
				    HPGL2_vector(pThis, x, y + (4 * vtic / 3));
				    HPGL2_dot(pThis, x, y);
				    break;
				case 5: /* diamond */
				    HPGL2_diamond(pThis, x, y, htic, vtic);
				    HPGL2_dot(pThis, x, y);
				    break;
			}
		}
	}
}
/*
 * This used to be for special purpose negative point types, which are no
 * longer possible with newer gnuplot versions.  They are remapped above to
 * the range 100-120.
 */
TERM_PUBLIC void HPGL2_neg_point(GpTermEntry_Static * pThis, uint x, uint y, int number)
{
	int htic = (int)(HPGL2_psize * PCL_HTIC / 2);
	int vtic = (int)(HPGL2_psize * PCL_VTIC / 2);

	//number = -((-number + 1) % 20);
	//printf("point %i\n", number);

	switch(number) {
		case -20:       /* well 18 */
		    HPGL2_move(pThis, x - htic, y - vtic);
		    HPGL2_vector(pThis, x + htic, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "CI%.2f;\n", ((double)3 * (htic) / 4));
		    break;
		case -19:       /* well 17 */
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y - (vtic / 2));
		    HPGL2_move(pThis, x, y + (vtic / 2));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x - (htic / 4), y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x + (htic / 4), y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "CI%.2f;\n", ((double)(htic) / 2));
		    break;
		case -18:       /* well 16 */
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "CI%.2f;\n", ((double)3 * (htic) / 4));
		    break;
		case -17:       /* well 15 */
		    HPGL2_move(pThis, x - htic, y - vtic);
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x + (sqrt(2) * htic / 2), y - (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x + htic, y + vtic);
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "EW%.2f,0,180;\n", ((double)3 * (htic) / 4));
		    fprintf(GPT.P_GpOutFile, "WG%.2f,180,180;EP;\n", ((double)3 * (htic) / 4));
		    break;
		case -16:       /* well 14 */
		    HPGL2_move(pThis, x - htic, y - vtic);
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x + htic, y + vtic);
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "WG%.2f,0,360;EP;\n", ((double)3 * (htic) / 4));
		    break;
		case -15:       /* well 13 */
		    HPGL2_move(pThis, x - htic, y - vtic);
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x + (sqrt(2) * htic / 2), y - (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x + htic, y + vtic);
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "WG%.2f,0,360;EP;\n", ((double)3 * (htic) / 4));
		    break;
		case -14:       /* well 12 */
		    HPGL2_move(pThis, x - htic, y - vtic);
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x + (sqrt(2) * htic / 2), y - (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x + htic, y + vtic);
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "CI%.2f;\n", ((double)3 * (htic) / 4));
		    break;
		case -13:       /* well 11 */
		    HPGL2_move(pThis, x - htic, y - vtic);
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x + htic, y + vtic);
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "WG%.2f,0,360;EP;\n", ((double)3 * (htic) / 4));
		    break;
		case -12:       /* well 10 */
		    HPGL2_move_R(pThis, x + (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "EW%.2f,0,180;\n", ((double)3 * (htic) / 4));
		    fprintf(GPT.P_GpOutFile, "WG%.2f,180,180;EP;\n", ((double)3 * (htic) / 4));
		    break;
		case -11:       /* well 9 */
		    HPGL2_move_R(pThis, x + (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "CI%.2f;\n", ((double)3 * (htic) / 4));
		    break;
		case -10:       /* well 8 */
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "EW%.2f,0,180;\n", ((double)3 * (htic) / 4));
		    fprintf(GPT.P_GpOutFile, "WG%.2f,180,180;EP;\n", ((double)3 * (htic) / 4));
		    break;
		case -9:        /* well 7 */
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y - (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x + (sqrt(2) * htic / 2), y - (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x + (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "EW%.2f,0,180;\n", ((double)3 * (htic) / 4));
		    fprintf(GPT.P_GpOutFile, "WG%.2f,180,180;EP;\n", ((double)3 * (htic) / 4));
		    break;
		case -8:        /* well 6 */
		    HPGL2_move_R(pThis, x + (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "WG%.2f,0,360;EP;\n", ((double)3 * (htic) / 4));
		    break;
		case -7:        /* well 5 */
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y - (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x + (sqrt(2) * htic / 2), y - (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x + (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "WG%.2f,0,360;EP;\n", ((double)3 * (htic) / 4));
		    break;
		case -6:        /* well 4 */
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y - (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x + (sqrt(2) * htic / 2), y - (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y - (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x + (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x + (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move_R(pThis, x - (sqrt(2) * htic / 2), y + (sqrt(2) * vtic / 2));
		    HPGL2_vector_R(pThis, x - (3 * sqrt(2) * htic / 8), y + (3 * sqrt(2) * vtic / 8));
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "CI%.2f;\n", ((double)3 * (htic) / 4));
		    break;
		case -5:        /* well 3 */
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "WG%.2f,0,360;EP;\n", ((double)3 * (htic) / 4));
		    break;
		case -4:        /* well 2 */
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "CI%.2f;\n", ((double)3 * (htic) / 4));
		    break;
		case -3:        /* well 1 */
		    HPGL2_move(pThis, x, y);
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "CI%.2f;\n", ((double)3 * (htic) / 4));
		    break;
		case -2:        /* v box */
		    HPGL2_move(pThis, x - htic, y);
		    HPGL2_vector(pThis, x - (3 * htic / 4), y);
		    HPGL2_move(pThis, x + (3 * htic / 4), y);
		    HPGL2_vector(pThis, x + htic, y);
		    HPGL2_move(pThis, x, y - vtic);
		    HPGL2_vector(pThis, x, y - (3 * vtic / 4));
		    HPGL2_move(pThis, x, y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x, y + vtic);
		    HPGL2_move(pThis, x - (3 * htic / 4), y - (3 * vtic / 4));
		    HPGL2_vector(pThis, x + (3 * htic / 4), y - (3 * vtic / 4));
		    HPGL2_vector(pThis, x + (3 * htic / 4), y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x - (3 * htic / 4), y + (3 * vtic / 4));
		    HPGL2_vector(pThis, x - (3 * htic / 4), y - (3 * vtic / 4));
		    HPGL2_move(pThis, x - (htic / 2), y + (vtic / 2));
		    HPGL2_vector(pThis, x, y - (vtic / 2));
		    HPGL2_vector(pThis, x + (htic / 2), y + (vtic / 2));
		    break;
		default:        /* dot */
		    HPGL2_dot(pThis, x, y);
		    break;
	}
}

TERM_PUBLIC void HPGL2_pointsize(GpTermEntry_Static * pThis, double size)
{
	HPGL2_psize = (size >= 0 ? size : 1) * HPGL2_pointscale;
}

TERM_PUBLIC void HPGL2_linewidth(GpTermEntry_Static * pThis, double linewidth)
{
	double save_lw = HPGL2_lw;
	HPGL2_end_poly();
	if(linewidth * HPGL2_linewidth_scale > 0.1)
		HPGL2_lw = linewidth * HPGL2_linewidth_scale * 0.25;
	else
		HPGL2_lw = 0;
	// enforce setting of line properties 
	// TODO: set PW right here 
	if(HPGL2_lw != save_lw)
		HPGL2_pentype = LT_UNDEFINED;
}

TERM_PUBLIC void HPGL2_fillbox(GpTermEntry_Static * pThis, int fillstyle, uint x1, uint y1, uint width, uint height)
{
	/* fillpar:
	 * - solid   : 0 - 100
	 * - pattern : 0 - 100
	 */
	int fillpar = fillstyle >> 4;
	int style = fillstyle & 0xf;
	FPRINTF((stderr, "fillbox style = %x par = %i\n", style, fillpar));
	HPGL2_end_poly();
	/* move to start point */
	HPGL2_move(pThis, x1, y1);
	HPGL2_end_poly();
	fputs("PD;", GPT.P_GpOutFile);
	HPGL_penstate = DOWN;
	switch(style) {
		case FS_EMPTY: /* fill with background color */
		    /* select pen 0: white */
		    fprintf(GPT.P_GpOutFile, "PU;SP0;PD;TR0;FT%i;RR%i,%i;FT;SP%d;TR1;\n", 0, width, height, HPGL2_pen);
		    break;
		case FS_DEFAULT:
		    fprintf(GPT.P_GpOutFile, "FT1RR%i,%i;\n", width, height);
		    break;
		case FS_SOLID: /* shaded fill */
		    if((fillpar != 100) || (HPGL2_pen == 0))
			    fputs("TR0;", GPT.P_GpOutFile);
		/* deliberately fall through */
		case FS_TRANSPARENT_SOLID:
		    if(fillpar == 100)
			    fputs("FT1;", GPT.P_GpOutFile);
		    else
			    fprintf(GPT.P_GpOutFile, "FT%i,%i;", 10, fillpar);
		    fprintf(GPT.P_GpOutFile, "RR%i,%i;FT;\n", width, height);
		    if((style == FS_SOLID) && ((fillpar != 100) || (HPGL2_pen == 0)))
			    fputs("TR1;", GPT.P_GpOutFile);
		    break;
		case FS_PATTERN: /* pattern fill */
		    fputs("TR0;", GPT.P_GpOutFile);
		/* deliberately fall through */
		case FS_TRANSPARENT_PATTERN: {
		    const char * pattern[] = {
			    /* white; cross hatch; fine cross hatch fine; solid */
			    "UP;SP0;FT1", "FT4,70,45;", "FT4,50,45;", "FT1;",
			    /* hatching;  hatching;  fine hatching; fine hatching */
			    "FT21,4;", "FT21,3;", "FT3,40,120;", "FT3,40,60;"
		    };
		    fputs(pattern[fillpar % 8], GPT.P_GpOutFile);
		    fprintf(GPT.P_GpOutFile, "RR%i,%i;FT;\n", width, height);
		    if(fillpar % 8 == 0)
			    fprintf(GPT.P_GpOutFile, "UP;SP%d", HPGL2_pen);
		    if(style == FS_PATTERN)
			    fputs("TR1;", GPT.P_GpOutFile);
		    break;
	    }
		default:
		    /* should never happen */
		    break;
	}
}

TERM_PUBLIC void HPGL2_filled_polygon(GpTermEntry_Static * pThis, int points, gpiPoint * corners)
{
	int fillpar = corners->style >> 4;
	int style = corners->style & 0xf;
	int i;
	FPRINTF((stderr, "filled polygon style = %x par = %i\n", style, fillpar));
	HPGL2_end_poly();
	/* move to start point */
	HPGL2_move(pThis, corners[0].x, corners[0].y);
	HPGL2_end_poly();
	/* enter polygon mode */
	fputs("PD;PM0;", GPT.P_GpOutFile);
	HPGL_penstate = DOWN;

	/* draw polygon */
	for(i = 1; i < points; i++)
		HPGL2_vector(pThis, corners[i].x, corners[i].y);
	if((corners[points-1].x != corners[0].x) || (corners[points-1].y != corners[0].y))
		HPGL2_vector(pThis, corners[0].x, corners[0].y);
	HPGL2_end_poly();
	/* exit polygon mode */
	fputs("PM2;", GPT.P_GpOutFile);
	switch(style) {
		case FS_EMPTY: /* fill with background color */
		    /* select pen 0: white */
		    fprintf(GPT.P_GpOutFile, "PU;SP0;FT1;TR0;FP;FT;PU;TR1;SP%d\n", HPGL2_pen);
		    break;
		case FS_DEFAULT:
		    fillpar = 100;
		    fputs("FT1FP;", GPT.P_GpOutFile);
		    break;
		case FS_SOLID: /* shaded fill */
		    if((fillpar != 100) || (HPGL2_pen == 0))
			    fputs("TR0;", GPT.P_GpOutFile);
		/* deliberately fall through */
		case FS_TRANSPARENT_SOLID:
		    if(fillpar == 100)
			    fputs("FT1;", GPT.P_GpOutFile);
		    else
			    fprintf(GPT.P_GpOutFile, "FT%i,%i;", 10, fillpar);
		    if((style == FS_SOLID) && ((fillpar != 100) || (HPGL2_pen == 0)))
			    fputs("FP;FT;TR1\n", GPT.P_GpOutFile);
		    else
			    fputs("FP;FT;\n", GPT.P_GpOutFile);
		    break;
		case FS_PATTERN: /* pattern fill */
		    fputs("TR0;", GPT.P_GpOutFile);
		/* deliberately fall through */
		case FS_TRANSPARENT_PATTERN: {
		    const char * pattern[] = {
			    /* white; cross hatch; fine cross hatch fine; solid */
			    "UP;SP0;FT1", "FT4,70,45;", "FT4,50,45;", "FT1;",
			    /* hatching;  hatching;  fine hatching; fine hatching */
			    "FT21,4;", "FT21,3;", "FT3,40,120;", "FT3,40,60;"
		    };
		    fputs(pattern[fillpar % 8], GPT.P_GpOutFile);
		    if(style == FS_PATTERN)
			    fputs("FP;FT;TR1\n", GPT.P_GpOutFile);
		    else
			    fputs("FP;FT;\n", GPT.P_GpOutFile);
		    if(fillpar % 8 == 0)
			    fprintf(GPT.P_GpOutFile, "PU;SP%d", HPGL2_pen);
		    break;
	    }
		default:
		    /* should never happen */
		    break;
	}
}

TERM_PUBLIC void HPGL2_set_color(GpTermEntry_Static * pThis, const t_colorspec * colorspec)
{
	GnuPlot * p_gp = pThis->P_Gp;
	double gray = colorspec->value;
	int linetype = colorspec->lt;
	FPRINTF((stderr, "set_color : %i\n", HPGL2_pentype));
	HPGL2_end_poly();
	switch(colorspec->type) {
		case TC_LT:
		    if(linetype >= 0) {
			    /* red, green, blue, magenta, cyan, yellow */
			    int colorseq[6] = { 2, 3, 5, 6, 7, 4 };
			    /* Note: pen 0 is white, 1 is black */
			    if(HPGL2_numpen <= 2)
				    linetype = 1;
			    else {
				    /* HPGL2_numpen is guaranteed to be > 2 */
				    linetype %= (HPGL2_numpen - 2);
				    if(linetype < 6)
					    linetype = colorseq[linetype];
				    else
					    linetype += 2;
			    }
			    fprintf(GPT.P_GpOutFile, "PU;\nSP%d;PC%d;\n", linetype, linetype);
			    HPGL2_pen = linetype;
		    }
		    else if(linetype == LT_BLACK) {
			    /* Borders and Tics */
			    fputs("PU;\nSP1;PC1;\n", GPT.P_GpOutFile);
			    HPGL2_pen = 1;
		    }
		    else if(linetype == LT_AXIS) {
			    /* Axes and Grids */
			    fputs("PU;\nSP1;PC1;\n", GPT.P_GpOutFile);
			    HPGL2_pen = 1;
		    }
		    else if(linetype <= LT_NODRAW) {
			    fputs("PU;\nSP0;PC0;", GPT.P_GpOutFile);
			    HPGL2_pen = 0;
		    }
		    HPGL_penstate = UP;
		    break;
		case TC_RGB: {
		    uint rgb = colorspec->lt;
		    HPGL2_end_poly();
		    fprintf(GPT.P_GpOutFile, "PC%i,%i,%i,%i;\n", HPGL2_pen, (rgb >> 16) & 0xff, (rgb >>  8) & 0xff, rgb & 0xff);
		    break;
	    }
		case TC_FRAC: {
		    rgb255_color color;
		    HPGL2_end_poly();
		    p_gp->Rgb255MaxColorsFromGray(gray, &color);
		    fprintf(GPT.P_GpOutFile, "PC%i,%i,%i,%i;\n", HPGL2_pen, (int)color.r, (int)color.g, (int)color.b);
		    break;
	    }
		default:
		    break;
	}
}

TERM_PUBLIC int HPGL2_make_palette(GpTermEntry_Static * pThis, t_sm_palette * palette)
{
	if(palette == NULL)
		return 0;
	return 0;
}

TERM_PUBLIC void HPGL2_enh_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char str[])
{
	GnuPlot * p_gp = pThis->P_Gp;
	enum JUSTIFY just = HPGL2_justification;
	int angle = HPGL_ang;
	char * fontname = (char *)HPGL2_font->name;
	double fontsize = HPGL2_point_size_current;
	int pass, num_passes;
	const char * original_str = str;
	if(isempty(str))
		return;
	if(fontsize == 0.0)
		fontsize = HPGL2_DEF_POINT;
	// If no enhanced text processing is needed, we can use the plain  
	// vanilla put_text() routine instead of this fancy recursive one. 
	if(p_gp->Enht.Ignore || (!strpbrk(str, "{}^_@&~") && !contains_unicode(str))) {
		HPGL2_put_text(pThis, x, y, str);
		return;
	}
	HPGL2_move(pThis, x, y);
	HPGL2_end_poly();
	// Align with baseline 
	fputs("LO1", GPT.P_GpOutFile);
	// Adjust baseline position: 
	fputs("CP0,-0.3\n", GPT.P_GpOutFile);
	// Set up global variables needed by enhanced_recursion() 
	p_gp->Enht.FontScale = 1.0;
	HPGL2_opened_string = FALSE;
	HPGL2_base = 0.0;
	strncpy(p_gp->Enht.EscapeFormat, "%c", sizeof(p_gp->Enht.EscapeFormat));
	// Text justification requires two passes. During the first pass we 
	// don't draw anything, we just move the "cursor".                  
	// Without justification one pass is enough.                        
	if(HPGL2_justification == LEFT) {
		num_passes = 1;
		HPGL2_sizeonly = FALSE;
	}
	else {
		num_passes = 2;
		HPGL2_sizeonly = TRUE;
		// Draw "upside-down" 
		HPGL2_text_angle(pThis, HPGL_ang + 180);
		if(just == CENTRE) {
			// set font size to half 
			HPGL2_enh_fontscale = 0.5;
			HPGL2_point_size_current = -1;
			HPGL2_set_font_size(pThis, fontname, fontsize);
		}
		// print invisibly: white pen, no outline, transparent fill 
		fputs("SP0CF2TR\n", GPT.P_GpOutFile);
		// We actually print everything left to right. 
		HPGL2_justification = LEFT;
	}
	for(pass = 1; pass <= num_passes; pass++) {
		/* Set the recursion going. We say to keep going until a
		 * closing brace, but we don't really expect to find one.
		 * If the return value is not the nul-terminator of the
		 * string, that can only mean that we did find an unmatched
		 * closing brace in the string. We increment past it (else
		 * we get stuck in an infinite loop) and try again.
		 */
		while(*(str = enhanced_recursion(pThis, str, TRUE, fontname, fontsize, 0.0, TRUE, TRUE, 0))) {
			pThis->enhanced_flush(pThis);
			if(!*++str)
				break; /* end of string */
			/* else carry on and process the rest of the string */
		}
		/* In order to do text justification we need to do a second pass */
		if(num_passes == 2 && pass == 1) {
			// do the actual printing in the next pass 
			HPGL2_sizeonly = FALSE;
			// revert to normal text direction 
			HPGL2_text_angle(pThis, angle);
			// print visibly with normal pen and filling 
			fprintf(GPT.P_GpOutFile, "SP%dCF\n", HPGL2_pen);
			if(just == CENTRE) {
				// restore font size to normal 
				HPGL2_enh_fontscale = 1.0;
				HPGL2_point_size_current = -1;
				HPGL2_set_font_size(pThis, fontname, fontsize);
			}
		}
		str = original_str;
	}
	// restore text alignment 
	HPGL2_justify_text(pThis, LEFT);
	HPGL2_lost = TRUE;
	// restore font 
	HPGL2_enh_fontscale = 1.0;
	HPGL2_set_font(pThis, "");
}

TERM_PUBLIC void HPGL2_enh_open(GpTermEntry_Static * pThis, const char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
{
	GnuPlot * p_gp = pThis->P_Gp;
	const int scale = static_cast<int>(0.9 * HPGL_PUPI / 72.0); // scaling of base offset 
	/* There are two special cases:
	 * overprint = 3 means save current position
	 * overprint = 4 means restore saved position
	 */
	/* Note: saving current position only possible in PCL mode */
	if(overprint == 3) {
		// PCL mode; push position; restore HP-GL/2 mode 
		fputs("\033%1A\033&f0S\033%1B\n", GPT.P_GpOutFile);
		HPGL2_base_save = HPGL2_base;
		return;
	}
	else if(overprint == 4) {
		// PCL mode; pop position; restore HP-GL/2 mode 
		fputs("\033%1A\033&f1S\033%1B\n", GPT.P_GpOutFile);
		HPGL2_base = HPGL2_base_save;
		return;
	}
	if(!HPGL2_opened_string) {
		double base_shift;
		// Start new text fragment 
		HPGL2_opened_string = TRUE;
		p_gp->Enht.P_CurText = p_gp->Enht.Text;
		// Keep track of whether we are supposed to show this string 
		HPGL2_show = showflag;
		// 0/1/2  no overprint / 1st pass / 2nd pass 
		HPGL2_overprint = overprint;
		// widthflag FALSE means do not update text position after printing 
		// Note: Currently, this is only set by overprinting, which handled already. 
		HPGL2_widthflag = widthflag;
		// Select font 
		if(!isempty(fontname))
			HPGL2_set_font_size(pThis, fontname, fontsize);
		// base is distance above initial baseline.  Scale change in base to printer units, so we can do relative moves. 
		base_shift = (base - HPGL2_base) * scale;
		fprintf(GPT.P_GpOutFile, "PR%d,%d", (int)(-sin(HPGL_ang * SMathConst::PiDiv180) * base_shift), (int)( cos(HPGL_ang * SMathConst::PiDiv180) * base_shift));
		HPGL2_base = base;
	}
}

TERM_PUBLIC void HPGL2_enh_flush(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(HPGL2_opened_string) {
		*p_gp->Enht.P_CurText = '\0';
		// print the string fragment, perhaps invisibly 
		if(!HPGL2_show && !HPGL2_sizeonly) {
			fputs("SP0TRCF2;\n", GPT.P_GpOutFile); /* draw invisibly */
			HPGL2_put_text_here(p_gp->Enht.Text, FALSE);
			fprintf(GPT.P_GpOutFile, "SP%dCF;\n", HPGL2_pen);
		}
		else {
			HPGL2_put_text_here(p_gp->Enht.Text, FALSE);
		}
		if(HPGL2_sizeonly) {
			/* nothing more to do */
		}
		else if(HPGL2_overprint == 1) {
			/* Overprinting without knowing font metrics requires some tricks.
			   Note: does currently not work for UTF-8 text.
			 */
			double fontsize = HPGL2_point_size;
			const char * name = HPGL2_font->name;
			// Save current position after the "underprinted" text 
			fputs("\033%1A\033&f0S\033%1B\n", GPT.P_GpOutFile);
			// locate center of the first string by moving the cursor using half font size in the opposite direction 
			HPGL2_enh_fontscale *= 0.5;
			HPGL2_point_size_current = -1;
			HPGL2_set_font_size(pThis, name, fontsize);
			HPGL2_text_angle(pThis, HPGL_ang + 180);
			// Print text, switch to center alignment 
			fprintf(GPT.P_GpOutFile, "SP0CF2LB%s\003SP%dCFLO5", p_gp->Enht.Text, HPGL2_pen);
			HPGL2_text_angle(pThis, HPGL_ang + 180);
			HPGL2_enh_fontscale *= 2;
			HPGL2_point_size_current = -1;
			HPGL2_set_font_size(pThis, name, fontsize);
		}
		else if(HPGL2_overprint == 2) {
			// restore position to the end of the first string, set left justification 
			fputs("\033%1A\033&f1S\033%1B\nLO1", GPT.P_GpOutFile);
		}
		HPGL2_opened_string = FALSE;
	}
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE
TERM_TABLE_START(hpgl_driver)
	"hpgl", 
	"HP7475 and relatives [number of pens] [eject]",
	HPGL_XMAX, 
	HPGL_YMAX, 
	HPGL_VCHAR, 
	HPGL_HCHAR,
	HPGL_VTIC, 
	HPGL_HTIC, 
	HPGL_options, 
	HPGL_init, 
	HPGL_reset,
	HPGL_text, 
	GnuPlot::NullScale, 
	HPGL_graphics, 
	HPGL_move, 
	HPGL_vector,
	HPGL_linetype, 
	HPGL_put_text, 
	HPGL_text_angle, 
	GnuPlot::NullJustifyText, 
	GnuPlot::DoPoint, 
	GnuPlot::DoArrow, 
	set_font_null 
TERM_TABLE_END(hpgl_driver)
#undef LAST_TERM
#define LAST_TERM hpgl_driver

TERM_TABLE_START(pcl5_driver)
	"pcl5", 
	"PCL5e/PCL5c printers using HP-GL/2",
	PCL_XMAX, 
	PCL_YMAX, 
	HPGL2_VCHAR, 
	HPGL2_HCHAR,
	PCL_VTIC, 
	PCL_HTIC, 
	PCL_options, 
	PCL_init, 
	PCL_reset,
	PCL_text, 
	GnuPlot::NullScale, 
	PCL_graphics, 
	HPGL2_move, 
	HPGL2_vector,
	HPGL2_linetype, 
	HPGL2_enh_put_text, 
	HPGL2_text_angle,
	HPGL2_justify_text, 
	HPGL2_point, 
	GnuPlot::DoArrow, 
	HPGL2_set_font, 
	HPGL2_pointsize,
	TERM_CAN_DASH | TERM_LINEWIDTH | TERM_POINTSCALE | TERM_FONTSCALE | TERM_ENHANCED_TEXT,
	NULL, 
	NULL,     /* suspend/resume */
	HPGL2_fillbox,
	HPGL2_linewidth,
	#ifdef USE_MOUSE
	0, 
	0, 
	0, 
	0, 
	0,
	#endif
	HPGL2_make_palette, 
	0, // previous_palette 
	HPGL2_set_color,
	HPGL2_filled_polygon,
	NULL,     /* image */
	HPGL2_enh_open, 
	HPGL2_enh_flush, 
	do_enh_writec,
	NULL, NULL,     /* layer, path */
	0.,
	NULL,     /* hypertext */
	NULL,     /* boxed text */
	NULL,     /* modify_plots */
	HPGL2_dashtype 
TERM_TABLE_END(pcl5_driver)
#undef LAST_TERM
#define LAST_TERM pcl5_driver
#endif /* TERM_TABLE */
#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
START_HELP(hpgl)
"1 hpgl",
"?commands set terminal hpgl",
"?set terminal hpgl",
"?set term hpgl",
"?terminal hpgl",
"?term hpgl",
"?hpgl",
" The `hpgl` driver produces HPGL output for devices like the HP7475A plotter.",
" There are two options which can be set: the number of pens and `eject`,",
" which tells the plotter to eject a page when done.  The default is to use 6",
" pens and not to eject the page when done.",
"",
" The international character sets ISO-8859-1 and CP850 are recognized via",
" `set encoding iso_8859_1` or `set encoding cp850` (see `set encoding` for",
" details).",
"",
" Syntax:",
"       set terminal hpgl {<number_of_pens>} {eject}",
"",
" The selection",
"",
"       set terminal hpgl 8 eject",
"",
" is equivalent to the previous `hp7550` terminal, and the selection",
"",
"       set terminal hpgl 4",
"",
" is equivalent to the previous `hp7580b` terminal."
"",
" HPGL graphics can be imported by many software packages."
END_HELP(hpgl)

START_HELP(pcl5)
"1 pcl5",
"?commands set terminal pcl5",
"?set terminal pcl5",
"?set term pcl5",
"?terminal pcl5",
"?term pcl5",
"?pcl5",
" The `pcl5` driver supports PCL5e/PCL5c printers.  It (mostly) uses the"
" HP-GL/2 vector format.",
"",
" Syntax:",
"       set terminal pcl5 {<mode>} {{no}enhanced}",
"           {size <plotsize> | size <width>{unit},<height>{unit}}",
"           {font \"<fontname>,<size>\"} {pspoints | nopspoints}",
"           {fontscale <scale>} {pointsize <scale>} {linewidth <scale}",
"           {rounded|butt} {color <number_of_pens>}",
"",
" <mode> is `landscape` or `portrait`. <plotsize> is the physical",
" plotting size of the plot, which can be one of the following formats: `letter`",
" for standard (8 1/2\" X 11\") displays, `legal` for (8 1/2\" X 14\") displays,",
" `noextended` for (36\" X 48\") displays (a letter size ratio),",
" `extended` for (36\" X 55\") displays (almost a legal size ratio), or",
" `a4` for (296mm X 210mm) displays.  You can also explicitly specify the canvas",
" size using the `width` and `height` options. Default unit is `in`.",
" Default size is `letter`.",
"",
" <fontname> can be one of stick, univers (default), albertus, antique_olive,",
" arial, avant_garde_gothic, bookman, zapf_chancery, clarendon, coronet, courier",
" courier_ps, cg_times, garamond_antigua, helvetica, helvetica_narrow,",
" letter_gothic, marigold, new_century_schlbk, cg_omega, palatino, times_new_roman,",
" times_roman, zapf_dingbats, truetype_symbols, or wingdings.  Font names are",
" case-insensitive and underscores may be replaced by spaces or dashes or may be",
" left out.  <fontsize> is the font size in points.",
"",
" The point type selection can be the a limited default set by specifying",
" `nopspoints`, or the same set of point types as provided by the postscript terminal",
" by specifying `pspoints` (default).",
"",
" The `butt` option selects lines with butt ends and mitered joins (default),"
" whereas `rounded` selects rounded line ends and joins.",
"",
" Line widths, and point and font sizes can be scaled using the `linewidth`,"
" `pointscale`, or `fontscale` options, respectively."
"",
" `color` selects the number of pens <number_of_pens> used in plots.",
" Default is 8, minimum 2.",
"",
" Note that built-in support of some of these options is printer device",
" dependent. For instance, all the fonts are supposedly supported by the HP",
" Laserjet IV, but only a few (e.g. univers, stick) may be supported by the HP",
" Laserjet III and the Designjet 750C. Also, color obviously won't work on",
" monochrome devices, but newer ones will do grey-scale.",
"",
" Defaults: landscape, a4, 8 pens, univers, 12 point, pspoints, butt, no scaling",
"",
" The `pcl5` terminal will try to request fonts which match your `encoding`.",
" Note that this has highest priority, so you might end up with a different",
" font face.  The terminal's default `encoding` is `HP Roman-8`.",
"",
" Limitations:",
"",
" This terminal does not support alpha transparency. Transparent filling is",
" emulated using shading patterns. Boxed text is not implemented.",
"",
" The support for UTF-8 is limited.  Lacking the label mode for UTF-8 output",
" in HP-GL/2, the driver reverts to PCL for strings containing 8bit characters.",
" UTF-8 text is limited to angles of 0, 90, 180, and 270 degrees. Also vertical"
" alignment might be off depending on the font.",
"",
" Some enhanced text features (phantom box, overprinting) require using PCL",
" features in addition to HP-GL/2. This conforms to the specs but may not",
" work with your printer or software."
END_HELP(pcl5)
#endif /* TERM_HELP */
