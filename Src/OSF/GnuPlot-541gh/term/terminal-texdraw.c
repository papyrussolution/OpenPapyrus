// GNUPLOT - texdraw.trm 
// Copyright 1990 - 1993, 1998, 2004, 2018
//
/*
 * This file is included by ../term.c.
 *
 * This terminal driver supports: The TEXDRAW macros for LaTeX.
 *
 * AUTHORS
 *   Khun Yee Fung. Modified from eepic.trm.
 *   clipper@csd.uwo.ca
 *   January 20, 1992
 *
 *   Bastian Maerkisch
 *
 * send your comments or suggestions to (gnuplot-info@lists.sourceforge.net).
 */
/*
 *  This file contains the texdraw terminal driver, intended for use with the
 *  texdraw macro package for LaTeX. This is an alternative to the
 *  latex driver. You need texdraw.sty, and texdraw.tex in the texdraw package.
 *
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
	register_term(texdraw)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void TEXDRAW_init(GpTermEntry * pThis);
TERM_PUBLIC void TEXDRAW_options();
TERM_PUBLIC void TEXDRAW_graphics(GpTermEntry * pThis);
TERM_PUBLIC void TEXDRAW_text(GpTermEntry * pThis);
TERM_PUBLIC void TEXDRAW_reset(GpTermEntry * pThis);
TERM_PUBLIC void TEXDRAW_linetype(GpTermEntry * pThis, int linetype);
TERM_PUBLIC void TEXDRAW_dashtype(GpTermEntry * pThis, int dt, t_dashtype * custom_dash_pattern);
TERM_PUBLIC void TEXDRAW_linewidth(GpTermEntry * pThis, double linewidth);
TERM_PUBLIC void TEXDRAW_move(GpTermEntry * pThis, uint x, uint y);
TERM_PUBLIC void TEXDRAW_pointsize(double size);
TERM_PUBLIC void TEXDRAW_point(GpTermEntry * pThis, uint x, uint y, int number);
TERM_PUBLIC void TEXDRAW_vector(GpTermEntry * pThis, uint ux, uint uy);
TERM_PUBLIC void TEXDRAW_arrow(GpTermEntry * pThis, uint sx, uint sy, uint ex, uint ey, int head);
TERM_PUBLIC void TEXDRAW_put_text(GpTermEntry * pThis, uint x, uint y, const char str[]);
TERM_PUBLIC int TEXDRAW_justify_text(enum JUSTIFY mode);
TERM_PUBLIC int TEXDRAW_text_angle(int ang);
TERM_PUBLIC void TEXDRAW_set_color(GpTermEntry * pThis, const t_colorspec * colorspec);
TERM_PUBLIC int TEXDRAW_make_palette(GpTermEntry * pThis, t_sm_palette *);
TERM_PUBLIC void TEXDRAW_fillbox(GpTermEntry * pThis, int style, uint x1, uint y1, uint width, uint height);
TERM_PUBLIC void TEXDRAW_filled_polygon(GpTermEntry * pThis, int points, gpiPoint * corners);

#define TEXDRAW_PTS_PER_INCH (72.27)
/* resolution of printer we expect to use */
#define DOTS_PER_INCH (300)
/* dot size in pt */
#define TEXDRAW_UNIT (TEXDRAW_PTS_PER_INCH/DOTS_PER_INCH)

/* 5 inches wide by 3 inches high (default) */
#define TEXDRAW_XMAX (5*DOTS_PER_INCH)
#define TEXDRAW_YMAX (3*DOTS_PER_INCH)

#define TEXDRAW_HTIC (5*DOTS_PER_INCH/72)       /* (5./TEXDRAW_UNIT) */
#define TEXDRAW_VTIC (5*DOTS_PER_INCH/72)       /* (5./TEXDRAW_UNIT) */
#define TEXDRAW_HCHAR (DOTS_PER_INCH*53/10/72)  /* (5.3/TEXDRAW_UNIT) */
#define TEXDRAW_VCHAR (DOTS_PER_INCH*11/72)     /* (11./TEXDRAW_UNIT) */

#define GOT_TEXDRAW_PROTO
//#endif

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY

/* terminate any line in progress */
static void TEXDRAW_endline();

/* determine gray level according to fillstyle */
static double TEXDRAW_fill_gray(int style);

static uint TEXDRAW_posx;
static uint TEXDRAW_posy;
static enum JUSTIFY TEXDRAW_justify = LEFT;
static enum JUSTIFY TEXDRAW_last_justify = LEFT;
static int TEXDRAW_angle = 0;
static float TEXDRAW_scalefactor = 0.2409f;
static double TEXDRAW_xscale = 1.0;
static double TEXDRAW_yscale = 1.0;

/* for DOTS point style */
#define TEXDRAW_TINY_DOT "\\htext{$\\cdot$}"

/* POINTS */
#define TEXDRAW_POINT_TYPES 15  /* we supply more point types */

static const char * TEXDRAW_points[TEXDRAW_POINT_TYPES] = {
	"\\htext{%s$+$}",
	"\\htext{%s$\\times$}",
	"\\htext{%s$\\ast$}",
	"\\rmove(0 -8)\\htext{%s$\\Box$}",
	"\\htext{%s$\\blacksquare$}",
	"\\htext{%s$\\circ$}",
	"\\htext{%s$\\bullet$}",
	"\\htext{%s$\\triangle$}",
	"\\htext{%s$\\blacktriangle$}",
	"\\htext{%s$\\triangledown$}",
	"\\htext{%s$\\blacktriangledown$}",
	"\\htext{%s$\\lozenge$}",
	"\\htext{%s$\\blacklozenge$}",
	"\\htext{%s$\\heartsuit$}",
	"\\htext{%s$\\spadesuit$}",
};

/* LINES */
#define TEXDRAW_NUMLINES 5      /* number of linetypes below */
static const int TEXDRAW_lines[] = {
	2,                      /* -2 border */
	1,                      /* -1 axes   */
	2,                      /*  0 solid  */
	2,                      /*  1 solid  */
	2,                      /*  2 solid  */
};

#define TEXDRAW_NUMPAT 5
static const int TEXDRAW_dashpat[][6] = {
	{ 10, 6, 0, 0, 0, 0 },
	{  4, 8, 0, 0, 0, 0 },
	{ 10, 6, 4, 6, 0, 0 },
	{ 10, 6, 4, 6, 4, 6 },
};

static int TEXDRAW_last_type = 0; /* The line type selected most recently */
static int TEXDRAW_type; /* current line type */
static bool TEXDRAW_inline = FALSE; /* are we in the middle of a line */
static int TEXDRAW_linecount = 0; /* number of points in line so far */
#define TEXDRAW_LINEMAX 5 /* max value for linecount */
static double TEXDRAW_lw; /* linewidth scale factor */
static double TEXDRAW_last_lw;
static int TEXDRAW_dt; /* dashtype */

/* ARROWS */
static char TEXDRAW_arrow_type;
static int TEXDRAW_arrow_length;
static int TEXDRAW_arrow_width;

/* GRAY LEVEL */
static double TEXDRAW_gray;
static double TEXDRAW_last_gray;

/* OPTIONS */
static bool TEXDRAW_standalone = FALSE;
static bool TEXDRAW_rounded = TRUE;
static bool TEXDRAW_colortext = FALSE;
static bool TEXDRAW_psarrows = TRUE;
static bool TEXDRAW_texpoints = TRUE;
static GpSizeUnits TEXDRAW_explicit_units = INCHES;
static double TEXDRAW_size_x = 5.0;
static double TEXDRAW_size_y = 3.0;
static double TEXDRAW_lw_scale = 1.0;
static double TEXDRAW_ps = 1.0;
static double TEXDRAW_background = 1.0;

/* option names */
enum TEXDRAW_id { 
	TEXDRAW_DEFAULT,
	TEXDRAW_SIZE,
	TEXDRAW_STANDALONE, 
	TEXDRAW_INPUT,
	TEXDRAW_BLACKTEXT, 
	TEXDRAW_COLORTEXT,
	TEXDRAW_ROUNDED, 
	TEXDRAW_BUTT,
	TEXDRAW_LINEWIDTH, 
	TEXDRAW_POINTSCALE,
	TEXDRAW_PSARROWS, 
	TEXDRAW_GPARROWS,
	TEXDRAW_TEXPOINTS, 
	TEXDRAW_GPPOINTS,
	TEXDRAW_BACKGROUND,
	TEXDRAW_OTHER 
};

static struct gen_table TEXDRAW_opts[] =
{
	{ "def$ault", TEXDRAW_DEFAULT },
	{ "size", TEXDRAW_SIZE },
	{ "stand$alone", TEXDRAW_STANDALONE },
	{ "inp$ut", TEXDRAW_INPUT },
	{ "b$lacktext", TEXDRAW_BLACKTEXT },
	{ "colort$ext", TEXDRAW_COLORTEXT },
	{ "colourt$ext", TEXDRAW_COLORTEXT },
	{ "round$ed", TEXDRAW_ROUNDED },
	{ "butt", TEXDRAW_BUTT },
	{ "backg$round", TEXDRAW_BACKGROUND },
	{ "lw", TEXDRAW_LINEWIDTH },
	{ "linew$idth", TEXDRAW_LINEWIDTH },
	{ "points$cale", TEXDRAW_POINTSCALE },
	{ "ps", TEXDRAW_POINTSCALE },
	{ "psarrows", TEXDRAW_PSARROWS },
	{ "gparrows", TEXDRAW_GPARROWS },
	{ "texpoints", TEXDRAW_TEXPOINTS },
	{ "gppoints", TEXDRAW_GPPOINTS },
	{ NULL, TEXDRAW_OTHER }
};

TERM_PUBLIC void TEXDRAW_options(GpTermEntry * pThis, GnuPlot * pGp)
{
	char size_str[80] = "";
	int bg;
	while(!pGp->Pgm.EndOfCommand()) {
		switch((enum TEXDRAW_id)pGp->Pgm.LookupTableForCurrentToken(&TEXDRAW_opts[0])) {
			case TEXDRAW_DEFAULT:
			    TEXDRAW_standalone = FALSE;
			    TEXDRAW_rounded = TRUE;
			    TEXDRAW_colortext = TRUE;
			    TEXDRAW_psarrows = TRUE;
			    TEXDRAW_texpoints = TRUE;
			    TEXDRAW_lw_scale = TEXDRAW_ps = 1.0;
			    TEXDRAW_background = 1.0;
			    TEXDRAW_size_x = 5;
			    TEXDRAW_size_y = 3.0;
			    pThis->MaxX = static_cast<uint>(TEXDRAW_size_x * DOTS_PER_INCH);
			    pThis->MaxY = static_cast<uint>(TEXDRAW_size_y * DOTS_PER_INCH);
			    pThis->ChrV = TEXDRAW_VCHAR;
			    pThis->TicV = TEXDRAW_VTIC;
			    break;
			case TEXDRAW_SIZE: {
			    float width, height;
			    pGp->Pgm.Shift();
			    TEXDRAW_explicit_units = pGp->ParseTermSize(&width, &height, INCHES);
			    TEXDRAW_size_x = width  / GpResolution;
			    TEXDRAW_size_y = height / GpResolution;
			    pThis->MaxX = static_cast<uint>(TEXDRAW_size_x * DOTS_PER_INCH);
			    pThis->MaxY = static_cast<uint>(TEXDRAW_size_y * DOTS_PER_INCH);
			    pThis->ChrV = TEXDRAW_VCHAR;
			    pThis->TicV = TEXDRAW_VTIC;
			    break;
		    }
			case TEXDRAW_STANDALONE:
			    TEXDRAW_standalone = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case TEXDRAW_INPUT:
			    TEXDRAW_standalone = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case TEXDRAW_COLORTEXT:
			    TEXDRAW_colortext = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case TEXDRAW_BLACKTEXT:
			    TEXDRAW_colortext = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case TEXDRAW_BUTT:
			    TEXDRAW_rounded = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case TEXDRAW_ROUNDED:
			    TEXDRAW_rounded = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case TEXDRAW_LINEWIDTH:
			    pGp->Pgm.Shift();
			    TEXDRAW_lw_scale = pGp->RealExpression();
			    if(TEXDRAW_lw_scale < 0.0)
				    TEXDRAW_lw_scale = 1.0;
			    break;
			case TEXDRAW_POINTSCALE:
			    pGp->Pgm.Shift();
			    TEXDRAW_ps = pGp->RealExpression();
			    if(TEXDRAW_ps < 0.0)
				    TEXDRAW_ps = 1.0;
			    break;
			case TEXDRAW_BACKGROUND: {
			    int background;
			    pGp->Pgm.Shift();
			    background = parse_color_name();
			    int red   = static_cast<int>((double)((background >> 16) & 0xff));
			    int green = static_cast<int>((double)((background >>  8) & 0xff));
			    int blue  = static_cast<int>((double)( background        & 0xff));
			    TEXDRAW_background = (red * 0.30 + green * 0.59 + blue * 0.11) / 255;
			    break;
		    }
			case TEXDRAW_GPARROWS:
			    pGp->Pgm.Shift();
			    TEXDRAW_psarrows = FALSE;
			    break;
			case TEXDRAW_PSARROWS:
			    pGp->Pgm.Shift();
			    TEXDRAW_psarrows = TRUE;
			    break;
			case TEXDRAW_GPPOINTS:
			    pGp->Pgm.Shift();
			    TEXDRAW_texpoints = FALSE;
			    break;
			case TEXDRAW_TEXPOINTS:
			    pGp->Pgm.Shift();
			    TEXDRAW_texpoints = TRUE;
			    break;
			default:
			    pGp->IntErrorCurToken("Unknown terminal option");
		}
	}
	if(TEXDRAW_explicit_units == INCHES)
		snprintf(size_str, sizeof(size_str), "size %.2fin, %.2fin", TEXDRAW_size_x, TEXDRAW_size_y);
	else if(TEXDRAW_explicit_units == CM)
		snprintf(size_str, sizeof(size_str), "size %.2fcm, %.2fcm", TEXDRAW_size_x * 2.54, TEXDRAW_size_y * 2.54);
	// update terminal option string
	bg = static_cast<int>(TEXDRAW_background * 255);
	snprintf(term_options, MAX_LINE_LEN + 1,
	    "%s linewidth %.1f pointscale %.1f %stext "
	    "background \"#%02x%02x%02x\" "
	    "%sarrows %spoints %s",
	    TEXDRAW_rounded ? "rounded" : "butt",
	    TEXDRAW_lw_scale, TEXDRAW_ps,
	    TEXDRAW_colortext ? "color" : "black",
	    bg, bg, bg,
	    TEXDRAW_psarrows ? "ps" : "gp",
	    TEXDRAW_texpoints ? "tex" : "gp",
	    TEXDRAW_standalone ? "standalone" : "input");
}

TERM_PUBLIC void TEXDRAW_init(GpTermEntry * pThis)
{
	fputs("%% GNUPLOT: LaTeX using TEXDRAW macros\n", gpoutfile);
	if(TEXDRAW_standalone) {
		fputs(
			"\\documentclass[a4paper,10pt]{article}\n" \
			"\\usepackage{texdraw}\n" \
			"\\usepackage{latexsym}\n" \
			"\\usepackage{amssymb}\n" \
			"\\usepackage{xcolor}\n"
			"\\begin{document}\n",
			gpoutfile);
	}
}

TERM_PUBLIC void TEXDRAW_graphics(GpTermEntry * pThis)
{
	static char tdg1[] =
	    "\
\\btexdraw\n\
\\ifx\\pathDEFINED\\relax\\else\\let\\pathDEFINED\\relax\n\
 \\def\\QtGfr{\\ifx (\\TGre \\let\\YhetT\\cpath\\else\\let\\YhetT\\relax\\fi\\YhetT}\n\
 \\def\\path (#1 #2){\\move (#1 #2)\\futurelet\\TGre\\QtGfr}\n\
 \\def\\cpath (#1 #2){\\lvec (#1 #2)\\futurelet\\TGre\\QtGfr}\n\
\\fi\n\
\\drawdim pt\n\
\\setunitscale %2.2f\n\
\\linewd %d\n\
\\textref h:L v:C\n\
\\writeps{%d setlinecap} \\writeps{%d setlinejoin}\n";

	if(TEXDRAW_standalone)
		fputs("\\begin{figure}\n", gpoutfile);
	fprintf(gpoutfile, tdg1, TEXDRAW_scalefactor, TEXDRAW_lines[2], TEXDRAW_rounded ? 1 : 0, TEXDRAW_rounded ? 1 : 0);
	if(TEXDRAW_background == 1.) {
		/* enforce bounding box */
		fprintf(gpoutfile, "\\move (0 0) \\rmove (%d %d)\n", pThis->MaxX, pThis->MaxY);
	}
	else {
		fprintf(gpoutfile, "\\move (0 0) \\rlvec (%d 0) \\rlvec (0 %d) \\rlvec (%d 0) \\ifill f:%0.2f\n", pThis->MaxX, pThis->MaxY, -pThis->MaxX, TEXDRAW_background);
	}
	TEXDRAW_last_type = 0;
	TEXDRAW_type = 0;
	TEXDRAW_posx = TEXDRAW_posy = 0;
	TEXDRAW_lw = TEXDRAW_last_lw = 1.0;
	TEXDRAW_gray = TEXDRAW_last_gray = 0.0;
	TEXDRAW_arrow_type = 0;
	TEXDRAW_arrow_length = -1;
	TEXDRAW_arrow_width = -1;
	TEXDRAW_justify = TEXDRAW_last_justify = LEFT;
}

TERM_PUBLIC void TEXDRAW_text(GpTermEntry * pThis)
{
	TEXDRAW_endline();
	// fputs("\\drawbb\n", gpoutfile);
	fputs("\\etexdraw\n", gpoutfile);
	if(TEXDRAW_standalone)
		fputs("\\end{figure}\n\n", gpoutfile);
}

TERM_PUBLIC void TEXDRAW_reset(GpTermEntry * pThis)
{
	TEXDRAW_endline();
	TEXDRAW_posx = TEXDRAW_posy = 0;
	if(TEXDRAW_standalone)
		fputs("\\end{document}\n", gpoutfile);
}

TERM_PUBLIC void TEXDRAW_linetype(GpTermEntry * pThis, int linetype)
{
	TEXDRAW_endline();
	if(linetype >= TEXDRAW_NUMLINES - 2)
		linetype %= (TEXDRAW_NUMLINES - 2);
	TEXDRAW_type = linetype > -2 ? linetype : LT_BLACK;
	if(linetype == LT_AXIS)
		TEXDRAW_dashtype(pThis, DASHTYPE_AXIS, NULL);
	else
		TEXDRAW_dashtype(pThis, DASHTYPE_SOLID, NULL);
}

TERM_PUBLIC void TEXDRAW_dashtype(GpTermEntry * pThis, int dt, t_dashtype * custom_dash_pattern)
{
	TEXDRAW_endline();
	if(dt == DASHTYPE_SOLID) {
		dt = 0;
	}
	else if(dt == DASHTYPE_AXIS) {
		dt = 2;
	}
	else if(dt > 0) {
		dt %= TEXDRAW_NUMPAT;
	}
	if(dt == TEXDRAW_dt)
		return;
	if(dt == 0) {
		fputs("\\lpatt ()\n", gpoutfile);
		TEXDRAW_dt = 0;
	}
	else if(dt > 0) {
		int i;
		fputs("\\lpatt (", gpoutfile);
		for(i = 0; i < 6; i++) {
			if(TEXDRAW_dashpat[dt - 1][i] == 0)
				break;
			fprintf(gpoutfile, "%d ", (int)(TEXDRAW_dashpat[dt - 1][i] * TEXDRAW_lw));
		}
		fputs(")\n", gpoutfile);
		TEXDRAW_dt = dt;
	}
	else if(dt == DASHTYPE_CUSTOM) {
		/* not supported (yet) */
	}
}

TERM_PUBLIC void TEXDRAW_linewidth(GpTermEntry * pThis, double linewidth)
{
	TEXDRAW_lw = linewidth * TEXDRAW_lw_scale;
}

TERM_PUBLIC void TEXDRAW_move(GpTermEntry * pThis, uint x, uint y)
{
	TEXDRAW_endline();
	TEXDRAW_posx = x;
	TEXDRAW_posy = y;
}

TERM_PUBLIC void TEXDRAW_pointsize(double size)
{
	// We can only scale gnuplot's native point types
	GPO.TermPointSize = (size >= 0 ? size * TEXDRAW_ps : 1);
}

TERM_PUBLIC void TEXDRAW_point(GpTermEntry * pThis, uint x, uint y, int number)
{
	char colorstr[80] = "";
	TEXDRAW_move(pThis, x, y);
	if(!TEXDRAW_texpoints) {
		GnuPlot::DoPoint(pThis, x, y, number);
		return;
	}
	/* Print the character defined by 'number'; number < 0 means
	 * to use a dot, otherwise one of the defined points. */
	fprintf(gpoutfile, "\\move (%d %d)\n", (int)((double)x * TEXDRAW_xscale), (int)((double)y * TEXDRAW_yscale));
	if(TEXDRAW_last_justify != CENTRE) {
		fprintf(gpoutfile, "\\textref h:C v:C ");
		TEXDRAW_last_justify = CENTRE;
	}
	if(TEXDRAW_colortext && TEXDRAW_gray != 0)
		snprintf(colorstr, sizeof(colorstr), "\\color{black!%d!}", 100 - (int)(TEXDRAW_gray * 100));
	if(number < 0) {
		fprintf(gpoutfile, "%s\n", TEXDRAW_TINY_DOT);
	}
	else {
		fprintf(gpoutfile, TEXDRAW_points[number % TEXDRAW_POINT_TYPES], colorstr);
		fputc('\n', gpoutfile);
	}
}

TERM_PUBLIC void TEXDRAW_vector(GpTermEntry * pThis, uint ux, uint uy)
{
	if(!TEXDRAW_inline) {
		TEXDRAW_inline = TRUE;
		// Start a new line. This depends on line type 
		if((TEXDRAW_type != TEXDRAW_last_type) || (TEXDRAW_last_lw != TEXDRAW_lw)) {
			if(TEXDRAW_lines[TEXDRAW_type + 2] * TEXDRAW_lw != TEXDRAW_lines[TEXDRAW_last_type + 2] * TEXDRAW_last_lw)
				fprintf(gpoutfile, "\\linewd %d\n", (int)(TEXDRAW_lines[TEXDRAW_type + 2] * TEXDRAW_lw + 0.5));
			TEXDRAW_last_type = TEXDRAW_type;
			TEXDRAW_last_lw = TEXDRAW_lw;
		}
		if(TEXDRAW_gray != TEXDRAW_last_gray) {
			fprintf(gpoutfile, "\\setgray %0.2f\n", TEXDRAW_gray);
			TEXDRAW_last_gray = TEXDRAW_gray;
		}
		fprintf(gpoutfile, "\\path (%d %d)", (int)((double)TEXDRAW_posx * TEXDRAW_xscale), (int)((double)TEXDRAW_posy * TEXDRAW_yscale));
		TEXDRAW_linecount = 1;
	}
	else {
		/* Even though we are in middle of a path,
		 * we may want to start a new path command.
		 * If they are too long then latex will choke.
		 */
		if(TEXDRAW_linecount++ >= TEXDRAW_LINEMAX) {
			fputs("\n\\cpath ", gpoutfile);
			TEXDRAW_linecount = 1;
		}
	}
	fprintf(gpoutfile, "(%d %d)", (int)((double)ux * TEXDRAW_xscale), (int)((double)uy * TEXDRAW_yscale));
	TEXDRAW_posx = ux;
	TEXDRAW_posy = uy;
}

static void TEXDRAW_endline()
{
	if(TEXDRAW_inline) {
		putc('\n', gpoutfile);
		TEXDRAW_inline = FALSE;
	}
}

TERM_PUBLIC void TEXDRAW_arrow(GpTermEntry * pThis, uint sx, uint sy, uint ex, uint ey, int head)
{
	char text;
	char type = 'T'; // empty triangle
	// These are the default arrow sizes:
	int tiplen = static_cast<int>((0.16 * 72 / TEXDRAW_scalefactor + 0.5));
	int width = static_cast<int>((0.08 * 72 / TEXDRAW_scalefactor + 0.5));
	// Texdraw cannot only draw vector heads, fall back to built-in code.
	if(!TEXDRAW_psarrows || (head & HEADS_ONLY)) {
		GnuPlot::DoArrow(pThis, sx, sy, ex, ey, head);
		return;
	}
	switch(curr_arrow_headfilled) {
		case AS_NOFILL:
		    type = 'V'; // open V-shape
		    break;
		case AS_FILLED:
		case AS_NOBORDER:
		    type = 'F'; // filled triangle
		    break;
		case AS_EMPTY:
		    type = 'W'; // white filled triangle
		    break;
	}

	if(curr_arrow_headlength > 0) {
		width  = static_cast<int>(sin(curr_arrow_headangle * DEG2RAD) * curr_arrow_headlength);
		tiplen = static_cast<int>(cos(curr_arrow_headangle * DEG2RAD) * curr_arrow_headlength);
		if((curr_arrow_headbackangle - curr_arrow_headangle) <= 15)
			type = 'V'; // open V-shape
	}

	if(TEXDRAW_arrow_type != type) {
		fprintf(gpoutfile, "\\arrowheadtype t:%c\n", type);
		TEXDRAW_arrow_type = type;
	}
	if((TEXDRAW_arrow_length != tiplen) || (TEXDRAW_arrow_width != width)) {
		fprintf(gpoutfile, "\\arrowheadsize l:%d w:%d\n", tiplen, width);
		TEXDRAW_arrow_length = tiplen;
		TEXDRAW_arrow_width = width;
	}

	if((head & BOTH_HEADS) != 0)
		text = 'a'; // line with arrow
	else
		text = 'l'; // simple line
	if((head & END_HEAD) != 0 || (head & BOTH_HEADS) == 0) {
		fprintf(gpoutfile, "\\move (%d %d)\\%cvec (%d %d)\n", (int)((double)sx * TEXDRAW_xscale), (int)((double)sy * TEXDRAW_yscale),
		    text, (int)((double)ex * TEXDRAW_xscale), (int)((double)ey * TEXDRAW_yscale));
	}
	/* draw back-heads by drawing an arrow in the opposite direction */
	if((head & BACKHEAD) != 0) {
		fprintf(gpoutfile, "\\move (%d %d)\\%cvec (%d %d)\n", (int)((double)ex * TEXDRAW_xscale), (int)((double)ey * TEXDRAW_yscale),
		    text, (int)((double)sx * TEXDRAW_xscale), (int)((double)sy * TEXDRAW_yscale));
	}
	TEXDRAW_posx = ex;
	TEXDRAW_posy = ey;
}

TERM_PUBLIC void TEXDRAW_put_text(GpTermEntry * pThis, uint x, uint y, const char str[])
{
	char colorstr[80] = "";
	TEXDRAW_endline();
	fprintf(gpoutfile, "\\move (%d %d)", (int)((double)x * TEXDRAW_xscale), (int)((double)y * TEXDRAW_yscale));
	if(TEXDRAW_last_justify != TEXDRAW_justify) {
		TEXDRAW_last_justify = TEXDRAW_justify;
		if(TEXDRAW_justify == LEFT)
			fputs("\\textref h:L v:C ", gpoutfile);
		else if(TEXDRAW_justify == CENTRE)
			fputs("\\textref h:C v:C ", gpoutfile);
		else if(TEXDRAW_justify == RIGHT)
			fputs("\\textref h:R v:C ", gpoutfile);
	}

	if(TEXDRAW_colortext && TEXDRAW_gray != 0)
		snprintf(colorstr, sizeof(colorstr), "\\color{black!%d!}", 100 - (int)(TEXDRAW_gray * 100));
	if(TEXDRAW_angle == 0)
		fprintf(gpoutfile, "\\htext{%s%s}\n", colorstr, str);
	else if(TEXDRAW_angle == 90)
		fprintf(gpoutfile, "\\vtext{%s%s}\n", colorstr, str);
	else
		fprintf(gpoutfile, "\\rtext td:%d {%s%s}\n", TEXDRAW_angle, colorstr, str);
}

TERM_PUBLIC int TEXDRAW_justify_text(enum JUSTIFY mode)
{
	TEXDRAW_justify = mode;
	return (TRUE);
}

TERM_PUBLIC int TEXDRAW_text_angle(int ang)
{
	while(ang < 0) 
		ang += 360;
	ang %= 360;
	TEXDRAW_angle = ang;
	return (TRUE);
}

TERM_PUBLIC int TEXDRAW_make_palette(GpTermEntry * pThis, t_sm_palette * palette)
{
	return 0; // claim continuous colors 
}

TERM_PUBLIC void TEXDRAW_set_color(GpTermEntry * pThis, const t_colorspec * colorspec)
{
	// Users can choose any color as long as it is black. Enables dash patterns. 
	switch(colorspec->type) {
		case TC_FRAC:
		    TEXDRAW_gray = colorspec->value;
		    break;
		case TC_RGB: {
		    int red   = (colorspec->lt >> 16) & 0xff;
		    int green = (colorspec->lt >>  8) & 0xff;
		    int blue  = (colorspec->lt      ) & 0xff;
		    TEXDRAW_gray = (red * 0.30 + green * 0.59 + blue * 0.11) / 255;
		    break;
	    }
		case TC_LT:
		    /* any line type is black for now */
		    TEXDRAW_gray = 0.0;
		    break;
		default:
		    break;
	}
}

static double TEXDRAW_fill_gray(int style)
{
	double gray = TEXDRAW_gray;
	int pattern = style >> 4;
	int frac = style >> 4;
	static const double TEXDRAW_pat_gray[4] = { 1.0, 0.5, 0.8, 0.0 };
	switch(style & 0x0f) {
		case FS_SOLID:
		case FS_TRANSPARENT_SOLID:
		    if(frac < 100)
			    gray *= frac / 100.0;
		    break;
		case FS_PATTERN:
		case FS_TRANSPARENT_PATTERN:
		    gray = TEXDRAW_pat_gray[pattern % 4];
		    break;
		case FS_EMPTY:
		    gray = 1.0;
		    break;
		case FS_DEFAULT:
		default:
		    break;
	}
	return gray;
}

TERM_PUBLIC void TEXDRAW_fillbox(GpTermEntry * pThis, int style, uint x1, uint y1, uint width, uint height)
{
	double gray;
	TEXDRAW_endline();
	gray = TEXDRAW_fill_gray(style);
	// outline box using relative moves
	fprintf(gpoutfile, "\\move (%d %d)", x1, y1);
	fprintf(gpoutfile, "\\rlvec (%d %d)", width, 0);
	fprintf(gpoutfile, "\\rlvec (%d %d)", 0, height);
	fprintf(gpoutfile, "\\rlvec (%d %d)", -width, 0);
	// the polygon is closed automatically by fill
	fprintf(gpoutfile, "\\ifill f:%0.2f\n", gray);
}

TERM_PUBLIC void TEXDRAW_filled_polygon(GpTermEntry * pThis, int points, gpiPoint * corners)
{
	double gray;
	int i;
	TEXDRAW_endline();
	gray = TEXDRAW_fill_gray(corners->style);
	// outline polygon
	fprintf(gpoutfile, "\\move (%d %d)", corners[0].x, corners[0].y);
	for(i = 1; i < points; i++)
		fprintf(gpoutfile, "\\lvec (%d %d)", corners[i].x, corners[i].y);
	// fill polygon
	fprintf(gpoutfile, "\\ifill f:%0.2f\n", gray);
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE

TERM_TABLE_START(texdraw_driver)
	"texdraw",
	"LaTeX texdraw environment",
	TEXDRAW_XMAX, 
	TEXDRAW_YMAX, 
	TEXDRAW_VCHAR, 
	TEXDRAW_HCHAR,
	TEXDRAW_VTIC, 
	TEXDRAW_HTIC, 
	TEXDRAW_options, 
	TEXDRAW_init, 
	TEXDRAW_reset,
	TEXDRAW_text, 
	GnuPlot::NullScale, 
	TEXDRAW_graphics, 
	TEXDRAW_move, 
	TEXDRAW_vector,
	TEXDRAW_linetype, 
	TEXDRAW_put_text, 
	TEXDRAW_text_angle,
	TEXDRAW_justify_text, 
	TEXDRAW_point, 
	TEXDRAW_arrow, 
	set_font_null,
	TEXDRAW_pointsize,
	TERM_IS_LATEX | TERM_LINEWIDTH | TERM_POINTSCALE | TERM_MONOCHROME,
	0 /*suspend*/, 
	0 /*resume*/,
	TEXDRAW_fillbox,
	TEXDRAW_linewidth,
	#ifdef USE_MOUSE
	0, 
	0, 
	0, 
	0, 
	0,
	#endif
	TEXDRAW_make_palette, 
	0,
	TEXDRAW_set_color,
	TEXDRAW_filled_polygon,
	0,     /* image */
	0, 
	0, 
	0,     /* enhanced text */
	0,     /* layer */
	0,     /* path */
	0.0,     /* scale (unused) */
	0,     /* hypertext */
	0,
	0,
	TEXDRAW_dashtype 
TERM_TABLE_END(texdraw_driver)

#undef LAST_TERM
#define LAST_TERM texdraw_driver

#endif /* TERM_TABLE */

#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
START_HELP(texdraw)
"1 texdraw",
"?commands set terminal texdraw",
"?set terminal texdraw",
"?set term texdraw",
"?terminal texdraw",
"?term texdraw",
"?texdraw",
" The `texdraw` terminal driver supports the (La)TeX texdraw environment.  It is",
" intended for use with the texdraw package,",
" see https://www.ctan.org/tex-archive/graphics/texdraw/ .",
"",
"       set terminal texdraw",
"                      {size <XX>{unit},<YY>{unit}}",
"                      {standalone | input}",
"                      {blacktext | colortext | colourtext}",
"                      {linewidth <lw>} {rounded | butt}",
"                      {pointscale <ps>}",
"                      {psarrows | gparrows} {texpoints | gppoints}",
"                      {background <rgbcolor>}",
"",
" Note: Graphics are in grayscale only. Text is always black. Boxes and polygons",
" are filled using solid gray levels only. Patterns are not available.",
"",
" Points, among other things, are drawn using the LaTeX commands \"\\Diamond\" and",
" \"\\Box\".  These commands no longer belong to the LaTeX2e core; they are included",
" in the latexsym package, which is part of the base distribution and thus part",
" of any LaTeX implementation.  Please do not forget to use this package.",
" Other point types use symbols from the amssymb package. For compatibility with",
" plain TeX you need to specify the `gppoints` option.",
"",
" `standalone` produces a LaTeX file with possibly multiple plots, ready",
" to be compiled.  The default is `input` to produce a TeX file which can",
" be included.",
"",
" `blacktext` forces all text to be written in black. `colortext` enables",
" \"colored\" text. The default is `blacktext` and \"color\" means grayscale",
" really.",
"",
" `rounded` sets line caps and line joins to be rounded; `butt` sets butt",
" caps and mitered joins and is the default.",
"",
" `linewidth` and `pointscale` scale the width of lines and the size of point",
" symbols, respectively. `pointscale` only applies to `gppoints`.",
"",
" `psarrows` draws `arrow`s using TeXdraw commands which are shorter but do not",
" offer all options. `gparrows` selects drawing arrows using gnuplot's",
" own routine for full functionality instead.  Similarly, `texpoints`, and ",
" `gppoints` select LaTeX symbols or gnuplot's point drawing routines."
END_HELP(texdraw)
#endif /* TERM_HELP */
