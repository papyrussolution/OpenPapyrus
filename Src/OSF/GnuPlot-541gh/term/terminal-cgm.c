// Hey Emacs this is -*- C -*- 
// GNUPLOT - cgm.trm 
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
 * This terminal driver supports: Computer Graphics Metafile
 *
 * TODO
 *   better control over plot size (never cutting off labels, correct font sizes)

 * REFERENCES
 *
 *   ISO 8632-1:1992 Computer Graphics Metafile (CGM), Part 1,
 *   Functional Specification.
 *
 *   ISO 8632-1:1992 Computer Graphics Metafile (CGM), Part 3,
 *   Binary Encoding.
 *
 *   FIPS PUB 128 - Computer Graphics Metafile (CGM).
 *
 *   MIL-STD-2301A Computer Graphics Metafile (CGM) Implementation
 *   Standard for the National Imagery Transmission Format Standard, 5
 *   June 1998, http://164.214.2.51/ntb/baseline/docs/2301a/.  Only a
 *   subset of CGM version 1, but does include the binary format for
 *   that subset.
 *
 *   MIL-D-28003A "Digital Representation for Communication of
 *   Illustration Data: CGM Application Profile", 15 November 1991,
 *   http://www-cals.itsi.disa.mil/core/standards/28003aa1.pdf.
 *
 *   "The computer graphics metafile", Lofton R. Henderson and Anne
 *   M. Mumford, Butterworths, London, 1990, ISBN 0-408-02680-4.
 *
 * AUTHOR
 *   Jim Van Zandt <jrvz@comcast.net>
 *
 * send your comments or suggestions to the author or
 * gnuplot-info@lists.sourceforge.net.
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
	register_term(cgm)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void CGM_options();
TERM_PUBLIC void CGM_init(GpTermEntry_Static * pThis);
TERM_PUBLIC void CGM_reset(GpTermEntry_Static * pThis);
TERM_PUBLIC void CGM_text(GpTermEntry_Static * pThis);
TERM_PUBLIC void CGM_graphics(GpTermEntry_Static * pThis);
TERM_PUBLIC void CGM_move(GpTermEntry_Static * pThis, uint x, uint y);
TERM_PUBLIC void CGM_dashed_vector(GpTermEntry_Static * pThis, uint ux, uint uy);
TERM_PUBLIC void CGM_solid_vector(GpTermEntry_Static * pThis, uint ux, uint uy);
TERM_PUBLIC void CGM_linetype(GpTermEntry_Static * pThis, int linetype);
TERM_PUBLIC void CGM_linecolor(int color);
TERM_PUBLIC void CGM_dashtype(GpTermEntry_Static * pThis, int dashtype);
TERM_PUBLIC void CGM_linewidth(GpTermEntry_Static * pThis, double width);
TERM_PUBLIC void CGM_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str);
TERM_PUBLIC int  CGM_text_angle(GpTermEntry_Static * pThis, int ang);
TERM_PUBLIC int  CGM_justify_text(GpTermEntry_Static * pThis, enum JUSTIFY mode);
TERM_PUBLIC int  CGM_set_font(GpTermEntry_Static * pThis, const char * font);
TERM_PUBLIC void CGM_point(GpTermEntry_Static * pThis, uint x, uint y, int number);
TERM_PUBLIC void CGM_fillbox(GpTermEntry_Static * pThis, int style, uint x1, uint y1, uint width, uint height);
TERM_PUBLIC int  CGM_make_palette(GpTermEntry_Static * pThis, t_sm_palette * palette);
TERM_PUBLIC void CGM_set_color(GpTermEntry_Static * pThis, const t_colorspec *);
TERM_PUBLIC void CGM_filled_polygon(GpTermEntry_Static * pThis, int points, gpiPoint * corner);
TERM_PUBLIC void CGM_set_pointsize(GpTermEntry_Static * pThis, double size);
#define FATAL(msg) { slfprintf_stderr("%s\nFile %s line %d\n", msg, __FILE__, __LINE__); exit(EXIT_FAILURE); }
#define CGM_LARGE 32767
#define CGM_SMALL 32767/18*13   /* aspect ratio 1:.7222 */
#define CGM_MARGIN (CGM_LARGE/180)
// convert from plot units to pt 
#define CGM_PT ((pThis->MaxX + CGM_MARGIN)/cgm_plotwidth)
#define CGM_LINE_TYPES 9        /* number of line types we support */
#define CGM_COLORS 96           /* must not exceed size of pm3d_color_names_tbl[] */
#define CGM_POINTS 13           /* number of markers we support */
#define CGM_MAX_SEGMENTS 16382  /* maximum # polyline coordinates */
#define CGM_VCHAR (CGM_SMALL/360*12)
#define CGM_HCHAR (CGM_SMALL/360*12*5/9)
#define CGM_VTIC (CGM_LARGE/80)
#define CGM_HTIC (CGM_LARGE/80)
//#endif

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY

static int CGM_find_font(const char * name, int len, double * relwidth);
static int CGM_find_nearest_color(const t_colorspec * colorspec);

//#include <ctype.h>              /* for isspace() */

/* uncomment the following to enable assertions for this module only, regardless of compiler switches
 #ifdef NDEBUG
 #define DEFEAT_ASSERTIONS
 #endif
 #undef NDEBUG
 #include <assert.h>
 */

#define CGM_ADJ (sizeof(int)/sizeof(short))

static bool cgm_initialized = FALSE;
static uint cgm_posx;
static uint cgm_posy;
static uint cgm_linetype = 1;
static uint cgm_linetypes = CGM_LINE_TYPES + 3;
static uint cgm_dashtype = 0;
static uint cgm_color = 0;
static uint cgm_background = 0xffffff;
static int * cgm_polyline; /* stored polyline coordinates */
static int cgm_coords = 0; /* # polyline coordinates saved */
static int cgm_doing_polygon = 0; /* nonzero if creating polygon, else
                                   * creating polyline */
/* static enum JUSTIFY cgm_justify = LEFT; */ /*unused*/
static int cgm_step_sizes[8]; /* array of currently used dash lengths in plot units */
static int cgm_step_index = 0; /* index into cgm_step_sizes[] */
static int cgm_step = 0; /* amount of current dash not yet drawn, in plot units */
static int cgm_tic, cgm_tic707, cgm_tic866, cgm_tic500, cgm_tic1241, cgm_tic1077, cgm_tic621; /* marker dimensions */
struct cgm_properties {
	double angle; /* angle of text baseline (radians counter-clockwise from horizontal) */
	int font_index; /* font index */
	int char_height; /* character height in picture units */
	enum JUSTIFY justify_mode; /* how text is justified */
	int edge_visibility; /* nonzero if edge is visible */
	int edge_color;
	int fill_color;
	int interior_style;
	int hatch_index;
};

static struct cgm_properties
    cgm_current = {-1, -1, -1, (enum JUSTIFY)-1, -1, -1, -1, -1, -1}, /* written to file */
    cgm_next = {0, -2, -2, LEFT, -2, -2, -2, -1, -1}, /* needed for next text string/marker */
    cgm_reset = {-1, -1, -1, (enum JUSTIFY)-1, -1, -1, -1, -1, -1}; /* invalid entries */

static int cgm_user_color_count = 0;
static int cgm_user_color_max = 0;
static int cgm_smooth_colors = 0;
static int * cgm_user_color_table = (int *)0;
static int cgm_maximum_color_index = 255; /* Size of color table we will write */

struct fontdata {
	const char * name; /* the name of the font */
	double width; /* the width of the font, relative to
	                           Times Bold Italic.  The width
	                           adjustment can only be approximate.
	                           Of the standard fonts, only the
	                           four versions of "Courier" are
	                           monospaced. Also, metrics of the
	                           same font from different foundries
	                           are sometimes different.  */
};

static struct fontdata cgm_basic_font_data[] = {
	// these are WebCGM recommended fonts 
	{"Helvetica", 1.039},
	{"Helvetica Oblique", 1.099},
	{"Helvetica Bold", 1.083},
	{"Helvetica Bold Oblique", 1.011},
	{"Times Roman", .981},
	{"Times Bold", .985},
	{"Times Italic", .959},
	{"Times Bold Italic", 1.0},
	{"Courier", 1.327},
	{"Courier Bold", 1.327},
	{"Courier Oblique", 1.218},
	{"Courier Bold Oblique", 1.341},
	{"Symbol", .897},
	// These are basic public domain fonts required by MIL-D-28003A 
	{"Hershey/Cartographic_Roman", 1.2404},
	{"Hershey/Cartographic_Greek", .9094},
	{"Hershey/Simplex_Roman", 1.2369},
	{"Hershey/Simplex_Greek", .9129},
	{"Hershey/Simplex_Script", 1.4181},
	{"Hershey/Complex_Roman", 1.1150},
	{"Hershey/Complex_Greek", .9059},
	{"Hershey/Complex_Script", 1.3868},
	{"Hershey/Complex_Italic", 1.4146},
	{"Hershey/Complex_Cyrillic", 1.2056},
	{"Hershey/Duplex_Roman", 1.1707},
	{"Hershey/Triplex_Roman", 1.3240},
	{"Hershey/Triplex_Italic", 1.3310},
	{"Hershey/Gothic_German", 1.2056},
	{"Hershey/Gothic_English", 1.2021},
	{"Hershey/Gothic_Italian", 1.2021},
	{"Hershey/Symbol_Set_1", .9059},
	{"Hershey/Symbol_Set_2", .9059},
	{"Hershey/Symbol_Math", .9059},

	/* These are available in the Microsoft Office import filter.  By
	   default, the script font can apparently be accessed only via
	   the name "15".  */
	{"ZapfDingbats", 1.583},
	{"Script", 1.139},
	{"15", 1.139},

	/* in the Microsoft Office and Corel Draw import filters, these
	   are pseudonyms for some of the above */
	{"Helvetica Italic", 1.099},
	{"Helvetica Bold Italic", 1.011},
	{"Courier Italic", 1.218},
	{"Courier Bold Italic", 1.341},
	{"Times Oblique", .959},
	{"Times Bold Oblique", 1.0},

	{0, 0}
};

static struct fontdata * cgm_font_data = cgm_basic_font_data;

#define DEFAULT_CGMFONT "Helvetica Bold"
static char CGM_default_font[MAX_ID_LEN+1] = {'\0'};

/* variables to record the options */
static char cgm_font[32] = DEFAULT_CGMFONT;
static uint cgm_fontsize = 12;
static uint cgm_linewidth; /* line width in plot units */
static uint cgm_linewidth_pt = 1; /* line width in pt */
static bool cgm_monochrome = FALSE; /* colors enabled? */
static int  cgm_plotwidth = 432; /* assumed width of plot in pt. */
static bool cgm_portrait = FALSE; /* portrait orientation? */
static bool cgm_rotate = TRUE; /* text rotation enabled? */
static bool cgm_dashed = TRUE; /* dashed linestyles enabled? */
static bool cgm_nofontlist_mode = FALSE; /* omit font list? */

/* prototypes for static functions */
static void CGM_local_reset(GpTermEntry_Static * pThis);
static void CGM_flush_polyline();
static void CGM_flush_polygon();
static void CGM_write_char_record(int _cls, int cgm_id, int length, const char * data);
static void CGM_write_code(int _cls, int cgm_id, int length);
static void CGM_write_int(int value);
static void CGM_write_int_record(int _cls, int cgm_id, int length, int * data);
static void CGM_write_mixed_record(int _cls, int cgm_id, int numint, int * int_data, int numchar, const char * char_data);
static void CGM_write_byte_record(int _cls, int cgm_id, int length, char * data);

enum CGM_id {
	/* cgm mode */
	CGM_PORTRAIT, CGM_LANDSCAPE, CGM_DEFAULT,
	/* color */
	CGM_MONOCHROME, CGM_COLOR,
	/* rotation */
	CGM_ROTATE, CGM_NOROTATE,
	CGM_DASHED, CGM_SOLID,
	CGM_LINEWIDTH, CGM_WIDTH, CGM_NOFONTLIST, CGM_BACKGROUND,
	CGM_OTHER
};

static struct gen_table CGM_opts[] =
{
	{ "p$ortrait", CGM_PORTRAIT },
	{ "la$ndscape", CGM_LANDSCAPE },
	{ "de$fault", CGM_DEFAULT },
	{ "nof$ontlist", CGM_NOFONTLIST },
	{ "win$word6", CGM_NOFONTLIST }, /* deprecated */
	{ "m$onochrome", CGM_MONOCHROME },
	{ "c$olor", CGM_COLOR },
	{ "c$olour", CGM_COLOR },
	{ "r$otate", CGM_ROTATE },
	{ "nor$otate", CGM_NOROTATE },
	{ "da$shed", CGM_DASHED },
	{ "s$olid", CGM_SOLID },
	{ "li$newidth", CGM_LINEWIDTH },
	{ "lw", CGM_LINEWIDTH },
	{ "wid$th", CGM_WIDTH },
	{ "backg$round", CGM_BACKGROUND },
	{ NULL, CGM_OTHER }
};

TERM_PUBLIC void CGM_options(GpTermEntry_Static * pThis, GnuPlot * pGp)
{
	char * string;
	// Annoying hack to handle the case of 'set termoption' after 
	// we have already initialized the terminal.                  
	if(!pGp->Pgm.AlmostEquals(pGp->Pgm.GetPrevTokenIdx(), "termopt$ion"))
		CGM_local_reset(pThis);
	while(!pGp->Pgm.EndOfCommand()) {
		switch(pGp->Pgm.LookupTableForCurrentToken(&CGM_opts[0])) {
			case CGM_PORTRAIT:
			    cgm_portrait = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case CGM_LANDSCAPE:
			    cgm_portrait = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case CGM_DEFAULT:
			    CGM_local_reset(pThis);
			    pGp->Pgm.Shift();
			    break;
			case CGM_NOFONTLIST:
			    cgm_nofontlist_mode = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case CGM_MONOCHROME:
			    cgm_monochrome = TRUE;
			    pThis->SetFlag(TERM_MONOCHROME);
			    pGp->Pgm.Shift();
			    break;
			case CGM_COLOR:
			    cgm_monochrome = FALSE;
			    pThis->ResetFlag(TERM_MONOCHROME);
			    pGp->Pgm.Shift();
			    break;
			case CGM_ROTATE:
			    cgm_rotate = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case CGM_NOROTATE:
			    cgm_rotate = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case CGM_DASHED:
			    cgm_dashed = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case CGM_SOLID:
			    cgm_dashed = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case CGM_LINEWIDTH:
			    pGp->Pgm.Shift();
			    if(!pGp->Pgm.EndOfCommand()) {
				    cgm_linewidth_pt = pGp->IntExpression();
				    if(cgm_linewidth_pt == 0 || cgm_linewidth_pt > 10000) {
					    pGp->IntWarnCurToken("linewidth out of range");
					    cgm_linewidth_pt = 1;
				    }
			    }
			    break;
			case CGM_WIDTH:
			    pGp->Pgm.Shift();
			    if(!pGp->Pgm.EndOfCommand()) {
				    cgm_plotwidth = pGp->IntExpression();
				    if(cgm_plotwidth < 0 || cgm_plotwidth > 10000) {
					    pGp->IntWarnCurToken("width out of range");
					    cgm_plotwidth = 6 * 72;
				    }
			    }
			    break;
			case CGM_BACKGROUND:
			    pGp->Pgm.Shift();
			    cgm_background = pGp->ParseColorName();
			    if(cgm_user_color_count == 0) {
				    cgm_user_color_count = 1;
				    cgm_user_color_table = (int *)SAlloc::M(4 * sizeof(int));
				    cgm_user_color_table[0] = 0;
			    }
			    cgm_user_color_table[1] = cgm_background>>16 & 0xff;
			    cgm_user_color_table[2] = cgm_background>>8 & 0xff;
			    cgm_user_color_table[3] = cgm_background & 0xff;
			    break;
			case CGM_OTHER:
			default:
			    string = pGp->Pgm.P_InputLine + pGp->Pgm.GetCurTokenStartIndex();
			    // Silently ignore these, as they are not yet implemented 
			    if(pGp->Pgm.EqualsCur("dl") || pGp->Pgm.AlmostEqualsCur("dashl$ength")) {
				    pGp->Pgm.Shift();
				    pGp->RealExpression();
				    break;
			    }
			    if(string[0] == 'x') { /* set color */
				    ushort red, green, blue;
				    if(sscanf(string, "x%2hx%2hx%2hx", &red, &green, &blue) != 3)
					    pGp->IntErrorCurToken("invalid color spec, must be xRRGGBB");
				    if(cgm_user_color_count >= cgm_user_color_max) {
					    cgm_user_color_max = cgm_user_color_max*2 + 4;
					    cgm_user_color_table = (int *)SAlloc::R(cgm_user_color_table, (cgm_user_color_max*3+1)*sizeof(int));
					    // 1st table entry is the minimum color index value 
					    cgm_user_color_table[0] = 0;
				    }
				    cgm_user_color_table[1 + 3*cgm_user_color_count] = red;
				    cgm_user_color_table[2 + 3*cgm_user_color_count] = green;
				    cgm_user_color_table[3 + 3*cgm_user_color_count] = blue;
				    cgm_user_color_count++;
				    pGp->Pgm.Shift();
			    }
			    else {
				    char * s = NULL;
				    if(pGp->Pgm.EqualsCur("font"))
					    pGp->Pgm.Shift();
				    if(pGp->IsStringValue(pGp->Pgm.GetCurTokenIdx()) && (s = pGp->TryToGetString())) {
					    double relwidth;
					    int font_index;
					    char * comma = sstrchr(s, ',');
					    if(comma && (1 == sscanf(comma + 1, "%d", &cgm_fontsize)))
						    *comma = '\0';
					    if(*s)
						    font_index = CGM_find_font(s, strlen(s), &relwidth);
					    else
						    font_index = CGM_find_font(cgm_font, strlen(cgm_font), &relwidth);
					    if(font_index == 0) {
						    /* insert the font in the font table */
						    struct fontdata * new_font_data;
						    int i, n;
						    for(n = 0; cgm_font_data[n].name; n++)
							    ;
						    new_font_data = (fontdata *)SAlloc::M((n + 2)*sizeof(struct fontdata));
						    new_font_data->name = s;
						    // punt, since we don't know the real font width 
						    new_font_data->width = 1.0;
						    for(i = 0; i <= n; i++)
							    new_font_data[i+1] = cgm_font_data[i];
						    cgm_font_data = new_font_data;
						    font_index = 1;
					    }
					    else
						    SAlloc::F(s);
					    strnzcpy(cgm_font, cgm_font_data[font_index-1].name, sizeof(cgm_font));
				    }
				    else {
					    cgm_fontsize = pGp->IntExpression(); // the user is specifying the font size 
				    }
				    break;
			    }
		}
	}
	if(cgm_portrait) {
		pThis->SetMax(CGM_SMALL - CGM_MARGIN, CGM_LARGE - CGM_MARGIN);
	}
	else {
		pThis->SetMax(CGM_LARGE - CGM_MARGIN, CGM_SMALL - CGM_MARGIN);
	}
	{ // cgm_font, cgm_fontsize, and/or pThis->CV() may have changed 
		double w;
		CGM_find_font(cgm_font, strlen(cgm_font), &w);
		pThis->SetCharSize((uint)(cgm_fontsize*CGM_PT*.527*w), (uint)(cgm_fontsize*CGM_PT));
	}
	sprintf(CGM_default_font, "%s,%d", cgm_font, cgm_fontsize);
	// CGM_default_font holds the font and size set at 'set term' 
	slprintf(GPT._TermOptions, "%s %s %s %s %s width %d linewidth %d font \"%s, %d\"",
	    cgm_portrait ? "portrait" : "landscape", cgm_monochrome ? "monochrome" : "color",
	    cgm_rotate ? "rotate" : "norotate", cgm_dashed ? "dashed" : "solid",
	    cgm_nofontlist_mode ? "nofontlist" : "", cgm_plotwidth, cgm_linewidth_pt, cgm_font, cgm_fontsize);
	if(cgm_user_color_count) {
		for(int i = 0; i < cgm_user_color_count && (GPT._TermOptions.Len() + 9 < MAX_LINE_LEN); i++) {
			int red = cgm_user_color_table[1 + 3*i];
			int green = cgm_user_color_table[2 + 3*i];
			int blue = cgm_user_color_table[3 + 3*i];
			slprintf_cat(GPT._TermOptions, " x%02x%02x%02x", red, green, blue);
		}
	}
	if(cgm_user_color_count < CGM_COLORS) {
		int i, j;
		// fill in colors not set by the user with the default colors 
		// 1st table entry is the minimum color index value 
		cgm_user_color_table = (int *)SAlloc::R(cgm_user_color_table, (CGM_COLORS * 3 + 1) * sizeof(int));
		cgm_user_color_table[0] = 0;
		for(i = cgm_user_color_count, j = cgm_user_color_count * 3; i < CGM_COLORS; i++, j += 3) {
			cgm_user_color_table[j+1] = (pm3d_color_names_tbl[i].value >> 16) & 0xff;
			cgm_user_color_table[j+2] = (pm3d_color_names_tbl[i].value >>  8) & 0xff;
			cgm_user_color_table[j+3] = (pm3d_color_names_tbl[i].value      ) & 0xff;
		}
		cgm_user_color_count = CGM_COLORS;
	}
	/* not set if there is an error exit before we reach here */
	cgm_initialized = TRUE;
}

static void CGM_local_reset(GpTermEntry_Static * pThis)
{
	double w;
	strcpy(cgm_font, DEFAULT_CGMFONT);
	CGM_find_font(cgm_font, strlen(cgm_font), &w);
	cgm_fontsize = 12;
	pThis->SetCharSize((uint)(cgm_fontsize * CGM_PT * 0.527 * w), (uint)(cgm_fontsize * CGM_PT));
	cgm_linewidth_pt = 1;
	cgm_monochrome = FALSE;
	cgm_plotwidth = 6 * 72;
	cgm_portrait = FALSE;
	cgm_rotate = TRUE;
	cgm_dashed = TRUE;
	cgm_nofontlist_mode = FALSE;
	cgm_current = cgm_reset;
	cgm_user_color_count = 0;
}

TERM_PUBLIC void CGM_init(GpTermEntry_Static * pThis)
{
	cgm_posx = cgm_posy = 0;
	cgm_linetype = 0;
	cgm_next.angle = 0;
	cgm_next.interior_style = 1;
	cgm_next.hatch_index = 1;
	cgm_polyline = (int *)SAlloc::M(CGM_MAX_SEGMENTS*sizeof(int));
}

TERM_PUBLIC void CGM_graphics(GpTermEntry_Static * pThis)
{
	//struct GpTermEntry * t = term;
	static int version_data[] = { 1 };
	static int vdc_type_data[] = { 0 };
	static int integer_precision_data[] = { 16 };
	static int real_precision_data[] = { 1, 16, 16 };
	static int index_precision_data[] = { 16 };
	static int color_precision_data[] = { 16 };
	static int color_index_precision_data[] = { 16 };
	static int scaling_mode_data[] = { 0, 0, 0 };
	static int color_value_extent_data[] = { 0, 0, 0, 255, 255, 255 };
	static int color_selection_mode_data[] = { 0 };
	static int linewidth_specification_mode_data[] = { 0 };
	static int edge_width_specification_mode_data[] = { 0 };
	static int marker_size_specification_mode_data[] = { 0 };
	static int vdc_extent_data[] = { 0, 0, 0, 0 };
	static int line_type_data[] = { 1 };
	static int interior_style_data[] = { 1 }; /* 0=hollow 1=filled
	                                           * 2=pattern 3=hatch
	                                           * 4=empty */
	static int hatch_index_data[] = { 1 }; /* 1=horizontal 2=vertical
	                                        * 3=positive slope
	                                        * 4=negative slope
	                                        * 5=horizontal/vertical
	                                        * crosshatch
	                                        * 6=positive/negative
	                                        * slope crosshatch */

	static int elements_list_data[] =
	{
		0,              /* will be set to # elements in this list */
		0, 1,           /* Begin Metafile */
		0, 2,           /* End Metafile */
		0, 3,           /* Begin Picture */
		0, 4,           /* Begin Picture Body */
		0, 5,           /* End Picture */
		1, 1,           /* Metafile Version */
		1, 2,           /* Metafile Description */
		1, 3,           /* VDC Type */
		1, 4,           /* Integer Precision */
		1, 5,           /* Real Precision */
		1, 6,           /* Index Precision */
		1, 7,           /* Color Precision */
		1, 8,           /* Color Index Precision */
		1, 9,           /* Maximum Color Index */
		1, 10,          /* Color Value Extent */
		1, 13,          /* Font List */
		2, 1,           /* Scaling Mode */
		2, 2,           /* Color Selection Mode */
		2, 3,           /* Line Width Specification Mode */
		2, 4,           /* Marker Size Specification Mode */
		2, 5,           /* Edge Width Specification Mode */
		2, 6,           /* VDC Extent */
#ifdef NEVER
		/* disabled due to complaints from CGM import filters */
		3, 1,           /* VDC Integer Precision */
		3, 4,           /* Transparency */
		3, 6,           /* Clip Indicator */
#endif
		4, 1,           /* Polyline */
		4, 3,           /* Polymarker */
		4, 4,           /* Text */
		4, 7,           /* Polygon */
		4, 11,          /* Rectangle */
		4, 12,          /* Circle */
		4, 15,          /* Circular Arc Center */
		4, 16,          /* Circular Arc Center Close */
		4, 17,          /* Ellipse */
		4, 18,          /* Elliptical Arc */
		4, 19,          /* Elliptical Arc Close */
		5, 2,           /* Line Type */
		5, 3,           /* Line Width */
		5, 4,           /* Line Color */
		5, 6,           /* Marker Type */
		5, 7,           /* Marker Size */
		5, 8,           /* Marker Color */
		5, 10,          /* Text Font Index */
		5, 14,          /* Text Color */
		5, 15,          /* Character Height */
		5, 16,          /* Character Orientation */
		5, 18,          /* Text Alignment */
		5, 22,          /* Interior Style */
		5, 23,          /* Fill Color */
		5, 24,          /* Hatch Index */
		5, 27,          /* Edge Type */
		5, 28,          /* Edge Width */
		5, 29,          /* Edge Color */
		5, 30,          /* Edge Visibility */
		5, 34,          /* Color Table */
		6, 1,           /* Escape */
		7, 2            /* Application Data */
	};
	if(!cgm_initialized)
		pThis->P_Gp->IntError(NO_CARET, "cgm terminal initialization failed");
	// metafile description (_cls 1), including filename if available 
	if(!GPT.P_OutStr)
		CGM_write_char_record(0, 1, 1, GPT.P_OutStr);
	else
		CGM_write_char_record(0, 1, strlen(GPT.P_OutStr) + 1, GPT.P_OutStr);
	CGM_write_int_record(1, 1, 2, version_data);
	{
		char description_data[256];
		sprintf(description_data, "Gnuplot version %s patchlevel %s,Computer Graphics Metafile version 1 per MIL-D-28003A/BASIC-1.%d",
		    gnuplot_version, gnuplot_patchlevel, cgm_monochrome ? 0 : 2);
		CGM_write_char_record(1, 2, strlen(description_data), description_data);
	}
	elements_list_data[0] = (sizeof(elements_list_data) / CGM_ADJ - 2) / 4;
	CGM_write_int_record(1, 11, sizeof(elements_list_data) / CGM_ADJ, elements_list_data);
	CGM_write_int_record(1, 3, 2, vdc_type_data);
	CGM_write_int_record(1, 4, 2, integer_precision_data);
	CGM_write_int_record(1, 5, 6, real_precision_data);
	CGM_write_int_record(1, 6, 2, index_precision_data);
	CGM_write_int_record(1, 7, 2, color_precision_data);
	CGM_write_int_record(1, 8, 2, color_index_precision_data);
	CGM_write_int_record(1, 9, 2, &cgm_maximum_color_index);
	CGM_write_int_record(1, 10, sizeof(color_value_extent_data) / CGM_ADJ,
	    color_value_extent_data);
	if(cgm_nofontlist_mode == FALSE) {
		char * buf, * s;
		int i, lgh = 0;
		for(i = 0; cgm_font_data[i].name; i++)
			lgh += strlen(cgm_font_data[i].name) + 1;
		buf = (char *)SAlloc::M(lgh + 1);
		for(s = buf, i = 0; cgm_font_data[i].name; i++) {
			int lgh = strlen(cgm_font_data[i].name);
			*s++ = (char)lgh;
			strcpy(s, cgm_font_data[i].name);
			s += lgh;
		}
		CGM_write_byte_record(1, 13, lgh, buf);
		SAlloc::F(buf);
	}
	// picture description (classes 2 and 3) 
	CGM_write_char_record(0, 3, 8, "PICTURE1");
	CGM_write_int_record(2, 1, 6, scaling_mode_data);
	CGM_write_int_record(2, 2, 2, color_selection_mode_data);
	CGM_write_int_record(2, 3, 2, linewidth_specification_mode_data);
	CGM_write_int_record(2, 4, 2, marker_size_specification_mode_data);
	CGM_write_int_record(2, 5, 2, edge_width_specification_mode_data);
	vdc_extent_data[2] = pThis->MaxX + CGM_MARGIN;
	vdc_extent_data[3] = pThis->MaxY + CGM_MARGIN;
	CGM_write_int_record(2, 6, 8, vdc_extent_data);
	// picture body (classes 4 and 5) 
	CGM_write_int_record(0, 4, 0, NULL);
#ifdef NEVER            /* no need for these, since we accept the defaults */
	{
		static int vdc_integer_precision_data[] = { 16 };
		CGM_write_int_record(3, 1, 2, vdc_integer_precision_data);
	}
	{
		static int transparency_data[] = { 1 }; /* text background: 0=auxiliary color 1=transparent */
		CGM_write_int_record(3, 4, sizeof(transparency_data) / CGM_ADJ, transparency_data);
	}
	{
		static int clip_indicator_data[] = { 0 };
		CGM_write_int_record(3, 6, sizeof(clip_indicator_data) / CGM_ADJ, clip_indicator_data);
	}
#endif
	if(!cgm_monochrome)
		CGM_write_int_record(5, 34, (cgm_user_color_count*3+1)* sizeof(cgm_user_color_table[0])/CGM_ADJ, cgm_user_color_table);
	CGM_write_int_record(5, 2, sizeof(line_type_data) / CGM_ADJ, line_type_data); /* line type 1=SOLID */
	cgm_linewidth = cgm_linewidth_pt * CGM_PT;
	CGM_write_int_record(5, 3, sizeof(cgm_linewidth) / CGM_ADJ, (int *)&cgm_linewidth); /* line width */
	CGM_write_int_record(5, 28, sizeof(cgm_linewidth) / CGM_ADJ, (int *)&cgm_linewidth); /* edge width */
	CGM_write_int_record(5, 27,  sizeof(line_type_data) / CGM_ADJ, line_type_data); /* edge type 1=SOLID */
	CGM_linecolor(0);
	cgm_current = cgm_reset;
	cgm_next.char_height = pThis->CV();
	CGM_write_int_record(5, 22, 2, interior_style_data);
	CGM_write_int_record(5, 24, 2, hatch_index_data);
	{
		char buf[45];
		sprintf(buf, "%.31s,%d", cgm_font, cgm_fontsize);
		CGM_set_font(pThis, buf);
	}
	CGM_set_pointsize(pThis, pThis->P_Gp->Gg.PointSize);
	// Fill with background color if user has specified one 
	if(!cgm_monochrome && cgm_user_color_count > 0) {
		CGM_linecolor(LT_BACKGROUND);
		CGM_fillbox(pThis, FS_SOLID, 0, 0, pThis->MaxX, pThis->MaxY);
	}
}

/* Return the index for the font with the name `name'.  The index for
   the first font is 1.  Set relwidth to the width of the font,
   relative to Times Bold Italic.  If the font is not in the table,
   set *relwidth to 1.0 and return 0. */
static int CGM_find_font(const char * name, int numchar, double * relwidth)
{
	*relwidth = 1.0;
	for(int i = 0; cgm_font_data[i].name; i++)
		// strncasecmp is not standard, but defined by stdfn.c if not available 
		if(strlen(cgm_font_data[i].name) == numchar && strncasecmp(name, cgm_font_data[i].name, numchar) == 0) {
			*relwidth = cgm_font_data[i].width;
			return i+1;
		}
	return 0;
}

TERM_PUBLIC int CGM_set_font(GpTermEntry_Static * pThis, const char * font)
{
	int size, font_index;
	const char * comma = sstrchr(font, ',');
	int len;
	double width;
	// Allow null string to indicaute default font 
	if(isempty(font))
		font = CGM_default_font;
	// find font in font table, or use 1st font 
	len = comma ? (comma - font) : strlen(font);
	font_index = CGM_find_font(font, len, &width);
	SETIFZ(font_index, 1);
	cgm_next.font_index = font_index;
	{
		const char * s = cgm_font_data[font_index-1].name;
		strnzcpy(cgm_font, s, sizeof(cgm_font));
	}
	// set font size 
	size = cgm_fontsize;
	if(comma)
		sscanf(comma + 1, "%d", &size);
	if(size > 0) {
		pThis->SetCharSize(static_cast<uint>(size * CGM_PT * 0.527 * width), size * CGM_PT);
	}
	cgm_next.char_height = pThis->CV();
	return TRUE;
}

TERM_PUBLIC void CGM_text(GpTermEntry_Static * pThis)
{
	CGM_flush_polyline();
	CGM_write_int_record(0, 5, 0, NULL); /* end picture */
	CGM_write_int_record(0, 2, 0, NULL); /* end metafile */
}

TERM_PUBLIC void CGM_linetype(GpTermEntry_Static * pThis, int linetype)
{
	SETMAX(linetype, LT_NODRAW);
	if(linetype == cgm_linetype)
		return;
	cgm_linetype = linetype;
	CGM_linecolor(linetype);
	if(cgm_dashed) {
		CGM_dashtype(pThis, linetype); /* DBT 10-8-98    use dashes */
	}
	else {
		// dashes for gridlines, solid for everything else 
		CGM_dashtype(pThis, linetype == -1 ? 2 : 0);
	}
}

TERM_PUBLIC void CGM_linecolor(int linecolor)
{
	if(linecolor >= 0) {
		/* subtract 2 due to linetypes -2 / -1 */
		if(cgm_linetypes > 3)
			linecolor %= (cgm_linetypes - 3);
		else
			linecolor = 0;
	}
	else if(linecolor == LT_BACKGROUND && !cgm_monochrome) {
		linecolor = -3;
	}
	else if(linecolor <= LT_NODRAW)
		return;
	linecolor += 3;
	if(cgm_monochrome)
		cgm_color = linecolor = 1;
	if(linecolor == cgm_color)
		return;
	cgm_color = linecolor;
	cgm_next.fill_color = linecolor;
	CGM_flush_polyline();
	CGM_write_int_record(5,  4, 2, (int *)&cgm_color); /* line color */
	CGM_write_int_record(5, 14, 2, (int *)&cgm_color); /* text color */
}

TERM_PUBLIC void CGM_fillbox(GpTermEntry_Static * pThis, int style, uint x1, uint y1, uint width, uint height)
{
	gpiPoint corner[5];
	corner[0].x = x1;        corner[0].y = y1;
	corner[1].x = x1+width;  corner[1].y = y1;
	corner[2].x = x1+width;  corner[2].y = y1+height;
	corner[3].x = x1;        corner[3].y = y1+height;
	corner[4].x = x1;        corner[4].y = y1;
	corner->style = style;
	CGM_filled_polygon(pThis, 5, corner);
}

TERM_PUBLIC void CGM_linewidth(GpTermEntry_Static * pThis, double width)
{
	if(width <= 0)
		width = 0.5;
	int new_linewidth = static_cast<int>(width * cgm_linewidth_pt * CGM_PT);
	if(new_linewidth != cgm_linewidth) {
		CGM_flush_polyline();
		cgm_linewidth = new_linewidth;
		CGM_write_int_record(5, 3, sizeof(cgm_linewidth) / CGM_ADJ, (int *)&cgm_linewidth);
		CGM_dashtype(pThis, cgm_dashtype); /* have dash lengths recalculated */
	}
}

TERM_PUBLIC void CGM_dashtype(GpTermEntry_Static * pThis, int dashtype)
{
	int i, j;
	/* Each group of 8 entries in dot_length[] defines a dash
	   pattern.  Entries in each group are alternately length of
	   whitespace and length of line, in units of 2/3 of the
	   linewidth. */
	static int dot_length[CGM_LINE_TYPES * 8] =
	{                       /* 0 - solid             */
		5, 8, 5, 8, 5, 8, 5, 8, /* 1 - dashes            */
		5, 3, 5, 3, 5, 3, 5, 3, /* 2 - short dashes      */
		4, 1, 4, 1, 4, 1, 4, 1, /* 3 - dotted            */
		4, 8, 4, 1, 4, 8, 4, 1, /* 4 - dash-dot          */
		4, 9, 4, 1, 4, 1, 0, 0, /* 5 - dash-dot-dot      */
		4, 10, 4, 1, 4, 1, 4, 1, /* 6 - dash-dot-dot-dot  */
		4, 10, 4, 10, 4, 1, 0, 0, /* 7 - dash-dash-dot     */
		4, 10, 4, 10, 4, 1, 4, 1
	};                              /* 8 - dash-dash-dot-dot */
	if(dashtype == cgm_dashtype)
		return;
	cgm_dashtype = dashtype;
	CGM_flush_polyline();
	if(dashtype >= CGM_LINE_TYPES)
		dashtype = dashtype % CGM_LINE_TYPES;
	if(dashtype < 1) {
		pThis->vector = CGM_solid_vector;
		return;
	}
	pThis->vector = CGM_dashed_vector;
	// set up dash dimensions 
	j = (dashtype - 1) * 8;
	for(i = 0; i < 8; i++, j++) {
		if(dot_length[j])
			cgm_step_sizes[i] = (dot_length[j] * cgm_linewidth) * 2 / 3;
		else
			cgm_step_sizes[i] = 0;
	}
	// first thing drawn will be a line 
	cgm_step = cgm_step_sizes[1];
	cgm_step_index = 1;
}

TERM_PUBLIC void CGM_move(GpTermEntry_Static * pThis, uint x, uint y)
{
	SETMIN(x, pThis->MaxX);
	SETMIN(y, pThis->MaxY);
	if(x == cgm_posx && y == cgm_posy)
		return;
	CGM_flush_polyline();
	cgm_posx = x;
	cgm_posy = y;
}

TERM_PUBLIC int CGM_make_palette(GpTermEntry_Static * pThis, t_sm_palette * palette)
{
	if(palette) {
		int i, k;
		cgm_smooth_colors = palette->Colors;
		if(TRUE || CGM_COLORS + cgm_smooth_colors > cgm_user_color_max) {
			cgm_user_color_max = CGM_COLORS + cgm_smooth_colors;
			cgm_user_color_table = (int *)SAlloc::R(cgm_user_color_table, (cgm_user_color_max*3+1)*sizeof(int));
		}
		k = 1 + (CGM_COLORS)*3;
		for(i = 0; i < cgm_smooth_colors; i++) {
			cgm_user_color_table[k++] = static_cast<int>(palette->P_Color[i].r*255.9);
			cgm_user_color_table[k++] = static_cast<int>(palette->P_Color[i].g*255.9);
			cgm_user_color_table[k++] = static_cast<int>(palette->P_Color[i].b*255.9);
		}
		cgm_user_color_count = CGM_COLORS + cgm_smooth_colors;
		CGM_write_int_record(5, 34, (cgm_user_color_count*3+1)* sizeof(cgm_user_color_table[0])/CGM_ADJ, cgm_user_color_table);
		return 0;
	}
	else {
		return (cgm_maximum_color_index - CGM_COLORS);
	}
}

TERM_PUBLIC void CGM_set_color(GpTermEntry_Static * pThis, const t_colorspec * colorspec)
{
	if(colorspec->type == TC_LT) {
		CGM_linecolor(colorspec->lt);
		cgm_linetype = colorspec->lt;
		return;
	}
	else if(colorspec->type == TC_FRAC) {
		double gray = colorspec->value;
		// map [0...1] to interval [0...cgm_smooth_colors-1], then add offset to get past the default colors 
		cgm_next.fill_color = (gray <= 0) ? 0 : (int)(gray * cgm_smooth_colors);
		if(cgm_next.fill_color >= cgm_smooth_colors)
			cgm_next.fill_color = cgm_smooth_colors - 1;
		cgm_next.fill_color += CGM_COLORS;
	}
	else if(colorspec->type == TC_RGB) {
		// To truly support RGB we would have to write a new color table to the
		// output file every time the RGB matched no previous color. That seems 
		// prohibitive, so instead we just look for the closest match.          
		cgm_next.fill_color = CGM_find_nearest_color(colorspec);
	}
	else
		return; // Should not happen! 
	// EAM - force color immediately so that lines and text can use it 
	if(cgm_color != cgm_next.fill_color) {
		cgm_color = cgm_next.fill_color;
		cgm_linetype = cgm_color;
		CGM_flush_polyline();
		CGM_write_int_record(5,  4, 2, (int *)&cgm_color); /* line color */
		CGM_write_int_record(5, 14, 2, (int *)&cgm_color); /* text color */
	}
}

TERM_PUBLIC void CGM_filled_polygon(GpTermEntry_Static * pThis, int points, gpiPoint * corner)
{
	/* Note: This implementation cannot handle polygons with more than
	 * about 8190 edges.  The best fix is to implement continuation
	 * blocks.  If the high order bit of the "length" field of a block
	 * is set, then it is followed by another block with more data.
	 * This allows an arbitrary amount of data in a record.  However,
	 * we implement a big enough block that problems should be rare.
	 */
	/* We will use solid fill for patterns 0 and 3 */
	int hatch_index[] = {0, 6, 5, 0, 4, 3};
	int style = corner->style;
	int pattern = (style >> 4) % 6;
	int i;
	switch(style & 0xf) {
		case FS_SOLID:
		case FS_TRANSPARENT_SOLID:
		    cgm_next.interior_style = 1;
		    break;
		case FS_PATTERN:
		case FS_TRANSPARENT_PATTERN:
		    if(pattern == 0) {
			    /* FIXME - for unknown reasons, solid fill messes up the subsequent */
			    /* color state.  Just leave it empty and let the background show */
			    cgm_next.interior_style = 0; /* empty */
			    break;
		    }
		    if(pattern == 3) {
			    /* Fill with solid color */
			    cgm_next.interior_style = 1;
			    break;
		    }
		    /* The rest of the patterns are hatch-filled */
		    cgm_next.interior_style = 3; /* hatched */
		    cgm_next.hatch_index = hatch_index[pattern];
		    break;
		default: /* style == 0 or unknown --> fill with background color */
		    cgm_next.fill_color = 0;
		    cgm_next.interior_style = 1; /* solid */
		    break;
	}
	if(cgm_current.interior_style != cgm_next.interior_style) {
		cgm_current.interior_style = cgm_next.interior_style;
		CGM_write_int_record(5, 22, 2, &cgm_next.interior_style);
	}
	if(cgm_current.fill_color != cgm_next.fill_color) {
		cgm_current.fill_color = cgm_next.fill_color;
		CGM_write_int_record(5, 23, 2, &cgm_next.fill_color); /* fill color */
	}
	if(cgm_current.hatch_index != cgm_next.hatch_index && cgm_next.interior_style == 3) {
		cgm_current.hatch_index = cgm_next.hatch_index;
		CGM_write_int_record(5, 24, 2, &cgm_next.hatch_index);
	}
	cgm_next.edge_visibility = 0; /* We draw the borders elsewhere */
	if(cgm_current.edge_visibility != cgm_next.edge_visibility) {
		cgm_current.edge_visibility = cgm_next.edge_visibility;
		CGM_write_int_record(5, 30, 2, &cgm_current.edge_visibility);
	}
	CGM_move(pThis, corner[0].x, corner[0].y);
	cgm_doing_polygon = 1;
	for(i = 1; i < points; i++)
		CGM_solid_vector(pThis, corner[i].x, corner[i].y);
	CGM_flush_polygon();
	cgm_doing_polygon = 0;
}

static void CGM_flush_polyline()
{
	if(cgm_coords == 0)
		return;
	CGM_write_int_record(4, 1, cgm_coords * 2, cgm_polyline);
	cgm_coords = 0;
}

static void CGM_write_char_record(int _cls, int cgm_id, int numbytes, const char * data)
{
	int i;
	static uchar flag = 0xff;
	static uchar paddata = 0;
	char short_len;
	int pad = 0;
	int length = numbytes + 1;
	if(numbytes >= 255)
		length += 2; /* long string */
	if(length & 1)
		pad = 1; /* needs pad */
	CGM_write_code(_cls, cgm_id, length);
	if(numbytes < 255) {
		short_len = (char)numbytes;
		fwrite(&short_len, 1, 1, GPT.P_GpOutFile); /* write true length */
	}
	else {
		fwrite(&flag, 1, 1, GPT.P_GpOutFile);
		CGM_write_int(numbytes);
	}
	if(data)
		fwrite(data, 1, numbytes, GPT.P_GpOutFile); /* write string */
	else
		for(i = 0; i<numbytes+pad; i++)
			fputc('\0', GPT.P_GpOutFile); /* write null bytes */
	if(pad)
		fwrite(&paddata, 1, 1, GPT.P_GpOutFile);
}

static void CGM_write_byte_record(int _cls, int cgm_id, int numbytes, char * data)
{
	int pad;
	static uchar paddata = 0;

	pad = numbytes & 1;
	CGM_write_code(_cls, cgm_id, numbytes);
	fwrite(data, 1, numbytes, GPT.P_GpOutFile); /* write string */
	if(pad)
		fwrite(&paddata, 1, 1, GPT.P_GpOutFile);
}

static void CGM_write_int_record(int _cls, int cgm_id, int numbytes, int * data)
{
	assert((numbytes & 1) == 0);
	CGM_write_code(_cls, cgm_id, numbytes);
	numbytes >>= 1;
	for(int i = 0; i < numbytes; i++)
		CGM_write_int(data[i]);
}

static void CGM_write_mixed_record(int _cls, int cgm_id, int numint, int * int_data, int numchar, const char * char_data)
{
	int i;
	static uchar paddata = 0;
	static uchar flag = 0xff;
	char short_len;
	int pad = 0;
	int length = numchar + 1;
	if(numchar >= 255)
		length += 2; /* long string */
	if(length & 1)
		pad = 1; /* needs pad */
	CGM_write_code(_cls, cgm_id, numint * 2 + length);
	for(i = 0; i < numint; i++)
		CGM_write_int(int_data[i]); /* write integers */
	if(numchar < 255) {
		short_len = (char)numchar;
		fwrite(&short_len, 1, 1, GPT.P_GpOutFile); /* write string length */
	}
	else {
		fwrite(&flag, 1, 1, GPT.P_GpOutFile);
		CGM_write_int(numchar);
	}
	fwrite(char_data, 1, numchar, GPT.P_GpOutFile); /* write string */
	if(pad)
		fwrite(&paddata, 1, 1, GPT.P_GpOutFile);
}
/*
   Write the code word that starts a CGM record.
   bits in code word are as follows...
   cccciiiiiiilllll
   where
   cccc is a 4-bit _cls number
   iiiiiii is a 7-bit ID number
   lllll is a 5-bit length (# bytes following the code word, or
            31 followed by a word with the actual number)
 */
static void CGM_write_code(int _cls, int cgm_id, int length)
{
	uint code;
	assert((0 <= _cls) &&(_cls <16));
	assert((0 <= cgm_id) && (cgm_id < 128));
	assert(0 <= length);
	if(length < 31) {
		code = ((_cls &0x0f) <<12) | ((cgm_id & 0x7f) << 5) | ((length & 0x1f));
		CGM_write_int(code);
	}
	else {
		code = ((_cls &0x0f) <<12) | ((cgm_id & 0x7f) << 5) | 0x1f;
		CGM_write_int(code);
		CGM_write_int(length);
	}
}

static void CGM_write_int(int value)
{
	union {
		short s;
		char c[2];
	} u;
	assert(-32768 <= value);
	assert(value <= 32767);
	u.c[0] = (value >> 8) & 255; /* convert to network order */
	u.c[1] = value & 255;
	fwrite(&u.s, 1, 2, GPT.P_GpOutFile);
}
//
// Draw a dashed line to (ux,uy).  CGM has linestyles, but they are
// not usable -- at least with the Word for Windows 6.0 filter, where
// lines of significant width (even 1 pt) always come out solid.
// Therefore, we implement dashed lines here instead. 
//
TERM_PUBLIC void CGM_dashed_vector(GpTermEntry_Static * pThis, uint ux, uint uy)
{
	int xa, ya;
	int dx, dy, adx, ady;
	int dist; // approximate distance in plot units from starting point to specified end point. 
	long remain; // approximate distance in plot units remaining to specified end point. 
	SETMIN(ux, pThis->MaxX);
	SETMIN(uy, pThis->MaxY);
	dx = (ux - cgm_posx);
	dy = (uy - cgm_posy);
	adx = abs(dx);
	ady = abs(dy * 10);
	/* using the approximation
	   sqrt(x**2 + y**2)  ~  x + (5*x*x)/(12*y)   when x > y.
	   Note ordering of calculations to avoid overflow on 16 bit
	   architectures */
	if(10 * adx < ady)
		dist = (ady / 2 + 25 * adx / ady * adx / 6 * 5) / 5;
	else {
		if(adx == 0)
			return;
		dist = (adx * 10 + (ady / 24) * (ady / adx)) / 10;
	}
	remain = dist;
	xa = cgm_posx;
	ya = cgm_posy;
	while(remain > cgm_step) {
		remain -= cgm_step;
		if(cgm_step_index & 1)
			CGM_solid_vector(pThis, (int)(ux - (remain * dx) / dist), (int)(uy - (remain * dy) / dist));
		else {
			xa = (int)(ux - (remain * dx) / dist);
			ya = (int)(uy - (remain * dy) / dist);
			CGM_move(pThis, xa, ya);
		}
		if(++cgm_step_index >= 8)
			cgm_step_index = 0;
		cgm_step = cgm_step_sizes[cgm_step_index];
	}
	if(cgm_step_index & 1)
		CGM_solid_vector(pThis, ux, uy);
	else
		CGM_move(pThis, ux, uy);
	cgm_step -= (int)remain;
}

TERM_PUBLIC void CGM_solid_vector(GpTermEntry_Static * pThis, uint ux, uint uy)
{
	SETMIN(ux, pThis->MaxX);
	SETMIN(uy, pThis->MaxY);
	if(ux == cgm_posx && uy == cgm_posy)
		return;
	if(cgm_coords > CGM_MAX_SEGMENTS - 2) {
		if(cgm_doing_polygon)
			CGM_flush_polygon();
		else
			CGM_flush_polyline();
		cgm_polyline[cgm_coords++] = cgm_posx;
		cgm_polyline[cgm_coords++] = cgm_posy + CGM_MARGIN;
	}
	else if(cgm_coords == 0) {
		cgm_polyline[cgm_coords++] = cgm_posx;
		cgm_polyline[cgm_coords++] = cgm_posy + CGM_MARGIN;
	}
	cgm_polyline[cgm_coords++] = ux;
	cgm_polyline[cgm_coords++] = uy + CGM_MARGIN;
	cgm_posx = ux;
	cgm_posy = uy;
}

TERM_PUBLIC void CGM_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char str[])
{
	static int where[3] = { 0, 0, 1 }; // the final "1" signals that this is the last text in the string 
	const char * s = str;
	// sanity check - labels are not clipped 
	if((x > 32767) || (y > 32767))
		return;
	while(*s)
		if(!isspace((uchar)*s++))
			goto showit;
	return;
showit:
	CGM_flush_polyline();
	/* update the text characteristics if they have changed since the
	   last text string was output */
	if(cgm_current.font_index != cgm_next.font_index) {
		cgm_current.font_index = cgm_next.font_index;
		CGM_write_int_record(5, 10, 2, &cgm_next.font_index);
	}
	if(cgm_current.justify_mode != cgm_next.justify_mode) {
		static int data[6] = { 1, 3, 0, 0, 0, 0 };
		cgm_current.justify_mode = cgm_next.justify_mode;
		switch(cgm_current.justify_mode) {
			case LEFT: data[0] = 1; break;
			case CENTRE: data[0] = 2; break;
			case RIGHT: data[0] = 3; break;
			default: assert(0);
		}
		CGM_write_int_record(5, 18, 12, data);
	}
	if(cgm_current.char_height != cgm_next.char_height) {
		int h = cgm_next.char_height;
		cgm_current.char_height = h;
		h = h*2/3; /* gnuplot measures fonts by the
		                   baseline-to-baseline distance,
		                   while the CGM file needs the actual
		                   height of the upper case
		                   characters. */
		CGM_write_int_record(5, 15, 2, &h);
	}
	/* "angle" is the angle of the text baseline (counter-clockwise in
	   radians from horizontal).  This is a bit more general than
	   gnuplot needs right now. */
	if(cgm_current.angle != cgm_next.angle) {
		/* The first two elements of orient are components of a vector
		   "upward" with respect to the text.  The next two elements are
		   components of a vector along the baseline of the text. The
		   lengths of both vectors are equal to the baseline-to-baseline
		   distance in plot units. */
		static int orient[4];
		cgm_current.angle = cgm_next.angle;
		orient[0] = static_cast<int>(cgm_next.char_height*cos(cgm_next.angle+SMathConst::PiDiv2));
		orient[1] = static_cast<int>(cgm_next.char_height*sin(cgm_next.angle+SMathConst::PiDiv2));
		orient[2] = static_cast<int>(cgm_next.char_height*cos(cgm_next.angle));
		orient[3] = static_cast<int>(cgm_next.char_height*sin(cgm_next.angle));
		CGM_write_int_record(5, 16, 8, orient);
	}
	where[0] = x;
	where[1] = y + CGM_MARGIN;
	CGM_write_mixed_record(4, 4, 3, where, strlen(str), str);
	cgm_posx = cgm_posy = -2000;
}

TERM_PUBLIC int CGM_text_angle(GpTermEntry_Static * pThis, int ang)
{
	if(cgm_rotate) {
		cgm_next.angle = ang * SMathConst::PiDiv180;
		return TRUE;
	}
	else
		return ang ? FALSE : TRUE;
}

TERM_PUBLIC int CGM_justify_text(GpTermEntry_Static * pThis, enum JUSTIFY mode)
{
	cgm_next.justify_mode = mode;
	return TRUE;
}

TERM_PUBLIC void CGM_reset(GpTermEntry_Static * pThis)
{
	cgm_posx = cgm_posy = 0;
	SAlloc::F(cgm_polyline);
}

TERM_PUBLIC void CGM_point(GpTermEntry_Static * pThis, uint x, uint y, int number)
{
	int old_dashtype;
	if(number < 0) {        /* draw dot */
		CGM_move(pThis, x, y);
		CGM_solid_vector(pThis, x + 1, y);
		return;
	}
	number %= CGM_POINTS;
	CGM_flush_polyline();
	old_dashtype = cgm_dashtype;
	CGM_dashtype(pThis, 0);
	if(number >= 3)         /* using a polygon */
		cgm_next.interior_style = 1; /* solid */
	if(oneof5(number, 4, 6, 8, 10, 12)) {
		/* filled */
		cgm_next.edge_visibility = 0;
		cgm_next.fill_color = cgm_color;
	}
	else {
		/* NOT filled */
		cgm_next.edge_visibility = 1;
		cgm_next.interior_style = 0; /* empty */
		cgm_next.edge_color = cgm_color;
	}
	if(cgm_current.interior_style != cgm_next.interior_style) {
		cgm_current.interior_style = cgm_next.interior_style;
		CGM_write_int_record(5, 22, 2, &cgm_next.interior_style);
	}
	if(cgm_current.fill_color != cgm_next.fill_color) {
		cgm_current.fill_color = cgm_next.fill_color;
		CGM_write_int_record(5, 23, 2, &cgm_next.fill_color);
	}
	if(cgm_current.edge_visibility != cgm_next.edge_visibility) {
		cgm_current.edge_visibility = cgm_next.edge_visibility;
		CGM_write_int_record(5, 30, 2, &cgm_current.edge_visibility);
	}
	if(cgm_current.edge_visibility &&
	    cgm_current.edge_color != cgm_next.edge_color) {
		cgm_current.edge_color = cgm_next.edge_color;
		CGM_write_int_record(5, 29, 2, &cgm_current.edge_color);
	}

	switch(number) {
		case 0:         /* draw plus */
		    CGM_move(pThis, x - cgm_tic, y);
		    CGM_solid_vector(pThis, x + cgm_tic, y);
		    CGM_move(pThis, x, y - cgm_tic);
		    CGM_solid_vector(pThis, x, y + cgm_tic);
		    break;
		case 1:         /* draw X */
		    CGM_move(pThis, x - cgm_tic707, y - cgm_tic707);
		    CGM_solid_vector(pThis, x + cgm_tic707, y + cgm_tic707);
		    CGM_move(pThis, x - cgm_tic707, y + cgm_tic707);
		    CGM_solid_vector(pThis, x + cgm_tic707, y - cgm_tic707);
		    break;
		case 2:         /* draw star (asterisk) */
		    CGM_move(pThis, x, y - cgm_tic);
		    CGM_solid_vector(pThis, x, y + cgm_tic);
		    CGM_move(pThis, x + cgm_tic866, y - cgm_tic500);
		    CGM_solid_vector(pThis, x - cgm_tic866, y + cgm_tic500);
		    CGM_move(pThis, x + cgm_tic866, y + cgm_tic500);
		    CGM_solid_vector(pThis, x - cgm_tic866, y - cgm_tic500);
		    break;
		case 3:         /* draw box */
		case 4:
		    CGM_move(pThis, x - cgm_tic707, y - cgm_tic707);
		    CGM_solid_vector(pThis, x + cgm_tic707, y - cgm_tic707);
		    CGM_solid_vector(pThis, x + cgm_tic707, y + cgm_tic707);
		    CGM_solid_vector(pThis, x - cgm_tic707, y + cgm_tic707);
		    CGM_flush_polygon();
		    break;
		case 5:
		case 6:         /* draw circle (actually, dodecagon)
		                   (WinWord 6 accepts the CGM "circle"
		                   element, but the resulting circle
		                   is not correctly centered!) */
		    CGM_move(pThis, x, y - cgm_tic);
		    CGM_solid_vector(pThis, x + cgm_tic500, y - cgm_tic866);
		    CGM_solid_vector(pThis, x + cgm_tic866, y - cgm_tic500);
		    CGM_solid_vector(pThis, x + cgm_tic, y);
		    CGM_solid_vector(pThis, x + cgm_tic866, y + cgm_tic500);
		    CGM_solid_vector(pThis, x + cgm_tic500, y + cgm_tic866);
		    CGM_solid_vector(pThis, x, y + cgm_tic);
		    CGM_solid_vector(pThis, x - cgm_tic500, y + cgm_tic866);
		    CGM_solid_vector(pThis, x - cgm_tic866, y + cgm_tic500);
		    CGM_solid_vector(pThis, x - cgm_tic, y);
		    CGM_solid_vector(pThis, x - cgm_tic866, y - cgm_tic500);
		    CGM_solid_vector(pThis, x - cgm_tic500, y - cgm_tic866);
		    CGM_flush_polygon();
		    break;
		case 7:         /* draw triangle (point up) */
		case 8:
		    CGM_move(pThis, x, y + cgm_tic1241);
		    CGM_solid_vector(pThis, x - cgm_tic1077, y - cgm_tic621);
		    CGM_solid_vector(pThis, x + cgm_tic1077, y - cgm_tic621);
		    CGM_flush_polygon();
		    break;
		case 9:         /* draw triangle (point down) */
		case 10:
		    CGM_move(pThis, x, y - cgm_tic1241);
		    CGM_solid_vector(pThis, x - cgm_tic1077, y + cgm_tic621);
		    CGM_solid_vector(pThis, x + cgm_tic1077, y + cgm_tic621);
		    CGM_flush_polygon();
		    break;
		case 11:        /* draw diamond */
		case 12:
		    CGM_move(pThis, x - cgm_tic, y);
		    CGM_solid_vector(pThis, x, y - cgm_tic);
		    CGM_solid_vector(pThis, x + cgm_tic, y);
		    CGM_solid_vector(pThis, x, y + cgm_tic);
		    CGM_flush_polygon();
		    break;
	}
	CGM_dashtype(pThis, old_dashtype);
}

TERM_PUBLIC void CGM_set_pointsize(GpTermEntry_Static * pThis, double size)
{
	/* Markers were chosen to have approximately equal
	   areas.  Dimensions are as follows, in units of
	   cgm_tic:

	   plus, diamond: half height = 1

	   square, cross: half height = sqrt(1/2) ~ 12/17

	   triangle: half width = sqrt(sqrt(4/3)) ~ 14/13,
	   height = sqrt(3*sqrt(4/3)) ~ 54/29

	   star: half height = 1, half width = sqrt(3/4) ~ 13/15

	   dodecagon: coordinates of vertices are 0,
	   sin(30) = 1/2, cos(30) = sqrt(3/4) ~ 13/15, or 1

	   The fractions are approximates of the equivalent
	   continued fractions. */
	if(size < 0)
		size = 1;
	cgm_tic = static_cast<int>(size * pThis->TicH / 2);
	cgm_tic707 = cgm_tic * 12 / 17;
	cgm_tic866 = cgm_tic * 13 / 15;
	cgm_tic500 = cgm_tic / 2;
	cgm_tic1241 = cgm_tic * 36 / 29;
	cgm_tic1077 = cgm_tic * 14 / 13;
	cgm_tic621 = cgm_tic * 18 / 29;
}

static void CGM_flush_polygon()
{
	if(cgm_coords) {
		CGM_write_int_record(4, 7, cgm_coords * 2, cgm_polyline);
		cgm_coords = 0;
	}
}
/*
 * This terminal driver does not support true RGB color,
 * but we can at least try to find some reasonable approximation.
 */
#define CLOSE_ENOUGH 32         /* 0 would require a perfect match */
static int CGM_find_nearest_color(const t_colorspec * colorspec)
{
	int red   = (colorspec->lt >> 16) & 0xff;
	int green = (colorspec->lt >> 8) & 0xff;
	int blue  = colorspec->lt & 0xff;
	int closest = 0;
	int howclose = 1<<16;
	int i = 0;
	int k;
	int dr, dg, db, distance;
	for(k = 0; k<cgm_user_color_count; k++) {
		dr = cgm_user_color_table[++i] - red;
		dg = cgm_user_color_table[++i] - green;
		db = cgm_user_color_table[++i] - blue;
		distance = (dr*dr + dg*dg + db*db);
		if(distance < howclose) {
			closest = k;
			howclose = distance;
		}
		if(distance < CLOSE_ENOUGH)
			break;
	}
	FPRINTF((stderr, "CGM_find_nearest_color:  asked for %d %d %d\n", red, green, blue));
	FPRINTF((stderr, "         got index %3d             %d %d %d\n", closest,
	    cgm_user_color_table[closest*3], cgm_user_color_table[closest*3+1], cgm_user_color_table[closest*3+2]));
	return closest;
}

#undef CLOSE_ENOUGH

#ifdef DEFEAT_ASSERTIONS
#define NDEBUG
#include <assert.h>
#undef DEFEAT_ASSERTIONS
#endif /* DEFEAT_ASSERTIONS */

#endif /* TERM_BODY */

#ifdef TERM_TABLE
TERM_TABLE_START(cgm_driver)
	"cgm", 
	"Computer Graphics Metafile",
	CGM_LARGE - CGM_MARGIN, 
	CGM_SMALL - CGM_MARGIN, 
	CGM_VCHAR, 
	CGM_HCHAR,
	CGM_VTIC, 
	CGM_HTIC, 
	CGM_options, 
	CGM_init, 
	CGM_reset,
	CGM_text, 
	GnuPlot::NullScale, 
	CGM_graphics, 
	CGM_move, 
	CGM_solid_vector,
	CGM_linetype, 
	CGM_put_text, 
	CGM_text_angle,
	CGM_justify_text, 
	CGM_point, 
	GnuPlot::DoArrow, 
	CGM_set_font,
	CGM_set_pointsize,
	TERM_BINARY|TERM_CAN_DASH|TERM_LINEWIDTH,       /* various flags */
	NULL,                           /* after one plot of multiplot */
	NULL,                           /* before subsequent plot of multiplot */
	CGM_fillbox,
	CGM_linewidth,
	#ifdef USE_MOUSE
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL,
	/* waitforinput, put_tmptext, set_ruler, set_cursor, set_clipboard */
	#endif
	CGM_make_palette,
	NULL, /* _previous_palette */
	CGM_set_color,
	CGM_filled_polygon 
TERM_TABLE_END(cgm_driver)

#undef LAST_TERM
#define LAST_TERM cgm_driver

#endif /* TERM_TABLE */
#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
START_HELP(cgm)
"1 cgm",
"?commands set terminal cgm",
"?set terminal cgm",
"?set term cgm",
"?terminal cgm",
"?term cgm",
"?cgm",
" The `cgm` terminal generates a Computer Graphics Metafile, Version 1. ",
" This file format is a subset of the ANSI X3.122-1986 standard entitled",
" \"Computer Graphics - Metafile for the Storage and Transfer of Picture",
" Description Information\".",
"",
" Syntax:",
"       set terminal cgm {color | monochrome} {solid | dashed} {{no}rotate}",
"                        {<mode>} {width <plot_width>} {linewidth <line_width>}",
"                        {font \"<fontname>,<fontsize>\"}",
"                        {background <rgb_color>}",
"   [deprecated]         {<color0> <color1> <color2> ...}",
"",
" `solid` draws all curves with solid lines, overriding any dashed patterns;",
" <mode> is `landscape`, `portrait`, or `default`;",
" <plot_width> is the assumed width of the plot in points; ",
" <line_width> is the line width in points (default 1); ",
" <fontname> is the name of a font (see list of fonts below)",
" <fontsize> is the size of the font in points (default 12).",
"",
" The first six options can be in any order.  Selecting `default` sets all",
" options to their default values.",
"",
" The mechanism of setting line colors in the `set term` command is",
" deprecated.  Instead you should set the background using a separate",
" keyword and set the line colors using `set linetype`.",
" The deprecated mechanism accepted colors of the form 'xrrggbb', where x is",
" the literal character 'x' and 'rrggbb' are the red, green and blue components",
" in hex. The first color was used for the background, subsequent colors are",
" assigned to successive line types.",
"",
" Examples:",
"       set terminal cgm landscape color rotate dashed width 432 \\",
"                      linewidth 1  'Helvetica Bold' 12       # defaults",
"       set terminal cgm linewidth 2  14  # wider lines & larger font",
"       set terminal cgm portrait \"Times Italic\" 12",
"       set terminal cgm color solid      # no pesky dashes!",

"2 cgm font",
"?commands set terminal cgm font",
"?set terminal cgm font",
"?set term cgm font",
"?cgm font",
" The first part of a Computer Graphics Metafile, the metafile description,",
" includes a font table.  In the picture body, a font is designated by an",
" index into this table.  By default, this terminal generates a table with",
" the following 35 fonts, plus six more with `italic` replaced by",
" `oblique`, or vice-versa (since at least the Microsoft Office and Corel",
" Draw CGM import filters treat `italic` and `oblique` as equivalent):",
"",
"@start table - first is interactive cleartext form",
"       Helvetica",
"       Helvetica Bold",
"       Helvetica Oblique",
"       Helvetica Bold Oblique",
"       Times Roman",
"       Times Bold",
"       Times Italic",
"       Times Bold Italic",
"       Courier",
"       Courier Bold",
"       Courier Oblique",
"       Courier Bold Oblique",
"       Symbol",
"       Hershey/Cartographic_Roman",
"       Hershey/Cartographic_Greek",
"       Hershey/Simplex_Roman",
"       Hershey/Simplex_Greek",
"       Hershey/Simplex_Script",
"       Hershey/Complex_Roman",
"       Hershey/Complex_Greek",
"       Hershey/Complex_Script",
"       Hershey/Complex_Italic",
"       Hershey/Complex_Cyrillic",
"       Hershey/Duplex_Roman",
"       Hershey/Triplex_Roman",
"       Hershey/Triplex_Italic",
"       Hershey/Gothic_German",
"       Hershey/Gothic_English",
"       Hershey/Gothic_Italian",
"       Hershey/Symbol_Set_1",
"       Hershey/Symbol_Set_2",
"       Hershey/Symbol_Math",
"       ZapfDingbats",
"       Script",
"       15",
"#\\begin{tabular}{|lll|} \\hline",
"#\\multicolumn{3}{|c|}{CGM fonts}\\\\\\hline",
"#&Helvetica&Hershey/Cartographic\\_Roman\\\\",
"#&Helvetica Bold&Hershey/Cartographic\\_Greek\\\\",
"#&Helvetica Oblique&Hershey/Simplex\\_Roman\\\\",
"#&Helvetica Bold Oblique&Hershey/Simplex\\_Greek\\\\",
"#&Times Roman&Hershey/Simplex\\_Script\\\\",
"#&Times Bold&Hershey/Complex\\_Roman\\\\",
"#&Times Italic&Hershey/Complex\\_Greek\\\\",
"#&Times Bold Italic&Hershey/Complex\\_Italic\\\\",
"#&Courier&Hershey/Complex\\_Cyrillic\\\\",
"#&Courier Bold&Hershey/Duplex\\_Roman\\\\",
"#&Courier Oblique&Hershey/Triplex\\_Roman\\\\",
"#&Courier Bold Oblique&Hershey/Triplex\\_Italic\\\\",
"#&Symbol&Hershey/Gothic\\_German\\\\",
"#&ZapfDingbats&Hershey/Gothic\\_English\\\\",
"#&Script&Hershey/Gothic\\_Italian\\\\",
"#&15&Hershey/Symbol\\_Set\\_1\\\\",
"#&&Hershey/Symbol\\_Set\\_2\\\\",
"#&&Hershey/Symbol\\_Math\\\\",
"%c c l .",
"%@@CGM fonts",
"%_",
"%@@Helvetica",
"%@@Helvetica Bold",
"%@@Helvetica Oblique",
"%@@Helvetica Bold Oblique",
"%@@Times Roman",
"%@@Times Bold",
"%@@Times Italic",
"%@@Times Bold Italic",
"%@@Courier",
"%@@Courier Bold",
"%@@Courier Oblique",
"%@@Courier Bold Oblique",
"%@@Symbol",
"%@@Hershey/Cartographic_Roman",
"%@@Hershey/Cartographic_Greek",
"%@@Hershey/Simplex_Roman",
"%@@Hershey/Simplex_Greek",
"%@@Hershey/Simplex_Script",
"%@@Hershey/Complex_Roman",
"%@@Hershey/Complex_Greek",
"%@@Hershey/Complex_Script",
"%@@Hershey/Complex_Italic",
"%@@Hershey/Complex_Cyrillic",
"%@@Hershey/Duplex_Roman",
"%@@Hershey/Triplex_Roman",
"%@@Hershey/Triplex_Italic",
"%@@Hershey/Gothic_German",
"%@@Hershey/Gothic_English",
"%@@Hershey/Gothic_Italian",
"%@@Hershey/Symbol_Set_1",
"%@@Hershey/Symbol_Set_2",
"%@@Hershey/Symbol_Math",
"%@@ZapfDingbats",
"%@@Script",
"%@@15",
"@end table",
"^<table align=\"center\" border=\"1\" rules=\"groups\" frame=\"hsides\" cellpadding=\"3\">",
"^<colgroup>",
"^  <col align=\"left\">",
"^  <col align=\"left\">",
"^</colgroup>",
"^<thead>",
"^<tr><th colspan=2 align=\"center\">CGM fonts</th></tr>",
"^</thead>",
"^<tbody>",
"^<tr><td>Helvetica</td><td>Hershey/Cartographic_Roman</td></tr>",
"^<tr><td>Helvetica Bold</td><td>Hershey/Cartographic_Greek</td></tr>",
"^<tr><td>Helvetica Oblique</td><td>Hershey/Simplex_Roman</td></tr>",
"^<tr><td>Helvetica Bold Oblique</td><td>Hershey/Simplex_Greek</td></tr>",
"^<tr><td>Times Roman</td><td>Hershey/Simplex_Script</td></tr>",
"^<tr><td>Times Bold</td><td>Hershey/Complex_Roman</td></tr>",
"^<tr><td>Times Italic</td><td>Hershey/Complex_Greek</td></tr>",
"^<tr><td>Times Bold Italic</td><td>Hershey/Complex_Italic</td></tr>",
"^<tr><td>Courier</td><td>Hershey/Complex_Cyrillic</td></tr>",
"^<tr><td>Courier Bold</td><td>Hershey/Duplex_Roman</td></tr>",
"^<tr><td>Courier Oblique</td><td>Hershey/Triplex_Roman</td></tr>",
"^<tr><td>Courier Bold Oblique</td><td>Hershey/Triplex_Italic</td></tr>",
"^<tr><td>Symbol</td><td>Hershey/Gothic_German</td></tr>",
"^<tr><td>ZapfDingbats</td><td>Hershey/Gothic_English</td></tr>",
"^<tr><td>Script</td><td>Hershey/Gothic_Italian</td></tr>",
"^<tr><td>15</td><td>Hershey/Symbol_Set_1</td></tr>",
"^<tr><td></td><td>Hershey/Symbol_Set_2</td></tr>",
"^<tr><td></td><td>Hershey/Symbol_Math</td></tr>",
"^</tbody>",
"^</table>",
"",
" The first thirteen of these fonts are required for WebCGM.  The",
" Microsoft Office CGM import filter implements the 13 standard fonts",
" listed above, and also 'ZapfDingbats' and 'Script'.  However, the",
" script font may only be accessed under the name '15'.  For more on",
" Microsoft import filter font substitutions, check its help file which",
" you may find here:",
"   C:\\Program Files\\Microsoft Office\\Office\\Cgmimp32.hlp",
" and/or its configuration file, which you may find here:",
"   C:\\Program Files\\Common Files\\Microsoft Shared\\Grphflt\\Cgmimp32.cfg",
"",
" In the `set term` command, you may specify a font name which does not",
" appear in the default font table.  In that case, a new font table is",
" constructed with the specified font as its first entry. You must ensure",
" that the spelling, capitalization, and spacing of the name are",
" appropriate for the application that will read the CGM file.  (Gnuplot",
" and any MIL-D-28003A compliant application ignore case in font names.)",
" If you need to add several new fonts, use several `set term` commands.",
"",
" Example:",
"       set terminal cgm 'Old English'",
"       set terminal cgm 'Tengwar'",
"       set terminal cgm 'Arabic'",
"       set output 'myfile.cgm'",
"       plot ...",
"       set output",
"",
" You cannot introduce a new font in a `set label` command.",

"2 cgm fontsize",
"?commands set terminal cgm fontsize",
"?set terminal cgm fontsize",
"?set term cgm fontsize",
"?cgm fontsize",
" Fonts are scaled assuming the page is 6 inches wide.  If the `size`",
" command is used to change the aspect ratio of the page or the CGM file",
" is converted to a different width, the resulting font sizes will be",
" scaled up or down accordingly.  To change the assumed width, use the",
" `width` option.",

"2 cgm linewidth",
"?commands set terminal cgm linewidth",
"?set terminal cgm linewidth",
"?set term cgm linewidth",
"?cgm linewidth",
" The `linewidth` option sets the width of lines in pt.  The default width",
" is 1 pt.  Scaling is affected by the actual width of the page, as",
" discussed under the `fontsize` and `width` options.",

"2 cgm rotate",
"?commands set terminal cgm rotate",
"?set terminal cgm rotate",
"?set term cgm rotate",
"?cgm rotate",
" The `norotate` option may be used to disable text rotation.  For",
" example, the CGM input filter for Word for Windows 6.0c can accept",
" rotated text, but the DRAW editor within Word cannot.  If you edit a",
" graph (for example, to label a curve), all rotated text is restored to",
" horizontal.  The Y axis label will then extend beyond the clip boundary.",
" With `norotate`, the Y axis label starts in a less attractive location,",
" but the page can be edited without damage.  The `rotate` option confirms",
" the default behavior.",

"2 cgm solid",
"?set terminal cgm solid",
"?set term cgm solid",
"?cgm solid",
" The `solid` option may be used to disable dashed line styles in the",
" plots.  This is useful when color is enabled and the dashing of the",
" lines detracts from the appearance of the plot. The `dashed` option",
" confirms the default behavior, which gives a different dash pattern to",
" each line type.",

"2 cgm size",
"?commands set terminal cgm size",
"?set terminal cgm size",
"?set term cgm size",
"?cgm size",
" Default size of a CGM plot is 32599 units wide and 23457 units high for",
" landscape, or 23457 units wide by 32599 units high for portrait.",

"2 cgm width",
"?commands set terminal cgm width",
"?set terminal cgm width",
"?set term cgm width",
"?cgm width",
" All distances in the CGM file are in abstract units.  The application",
" that reads the file determines the size of the final plot.  By default,",
" the width of the final plot is assumed to be 6 inches (15.24 cm).  This",
" distance is used to calculate the correct font size, and may be changed",
" with the `width` option.  The keyword should be followed by the width in",
" points.  (Here, a point is 1/72 inch, as in PostScript.  This unit is",
" known as a \"big point\" in TeX.)  Gnuplot `expressions` can be used to",
" convert from other units.",
"",
" Example:",
"       set terminal cgm width 432            # default",
"       set terminal cgm width 6*72           # same as above",
"       set terminal cgm width 10/2.54*72     # 10 cm wide",

"2 cgm nofontlist",
"?commands set terminal cgm nofontlist",
"?set terminal cgm nofontlist",
"?set term cgm nofontlist",
"?cgm nofontlist",
"?set terminal cgm winword6",
"?set term cgm winword6",
"?cgm winword6",
" The default font table includes the fonts recommended for WebCGM, which",
" are compatible with the Computer Graphics Metafile input filter for",
" Microsoft Office and Corel Draw.  Another application might use",
" different fonts and/or different font names, which may not be",
" documented.  The `nofontlist` (synonym `winword6`) option deletes the font",
" table from the CGM file.  In this case, the reading application should",
" use a default table.  Gnuplot will still use its own default font table",
" to select font indices.  Thus, 'Helvetica' will give you an index of 1,",
" which should get you the first entry in your application's default font",
" table. 'Helvetica Bold' will give you its second entry, etc.",
""

END_HELP(cgm)
#endif /* TERM_HELP */

/*
 * Local Variables:
 * mode:C
 * eval: (c-set-style "k&r")
 * End:
 */
