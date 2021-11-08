// GNUPLOT - pict2e.trm 
// Copyright 1990 - 1993, 1998, 2004, 2018, 2019
/*
 * This file is included by term.h.
 *
 * This terminal driver supports: LaTeX2e picture environment (pict2e).
 * The code is derived from latex.trm by David Kotz, Russell Lang.
 * AUTHORS: David Kotz, Russell Lang (latex/emtex), Bastian Maerkisch (pict2e)
 */
/* LaTeX terminal using the LaTeX2e pict2e package.
 * Unlike the older LaTeX terminals which use the original picture environment,
 * it supports most modern gnuplot features and does not carry the weight of
 * backward-compatibility.
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

// transparency for pict2e width pdflatex and package "transparent"
#define PICT2E_TRANSPARENT
#ifdef TERM_REGISTER
	register_term(pict2e)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void PICT2E_options();
TERM_PUBLIC void PICT2E_init(GpTermEntry * pThis);
TERM_PUBLIC void PICT2E_graphics(GpTermEntry * pThis);
TERM_PUBLIC void PICT2E_text(GpTermEntry * pThis);
TERM_PUBLIC void PICT2E_reset(GpTermEntry * pThis);
TERM_PUBLIC int  PICT2E_justify_text(GpTermEntry * pThis, enum JUSTIFY mode);
TERM_PUBLIC int  PICT2E_text_angle(GpTermEntry * pThis, int ang);
TERM_PUBLIC void PICT2E_put_text(GpTermEntry * pThis, uint x, uint y, const char str[]);
TERM_PUBLIC int  PICT2E_make_palette(GpTermEntry * pThis, t_sm_palette * palette);
TERM_PUBLIC void PICT2E_set_color(GpTermEntry * pThis, const t_colorspec * colorspec);
TERM_PUBLIC void PICT2E_linetype(GpTermEntry * pThis, int linetype);
TERM_PUBLIC void PICT2E_dashtype(GpTermEntry * pThis, int dt, t_dashtype * custom_dash_pattern);
TERM_PUBLIC void PICT2E_move(GpTermEntry * pThis, uint x, uint y);
TERM_PUBLIC void PICT2E_point(GpTermEntry * pThis, uint x, uint y, int number);
TERM_PUBLIC void PICT2E_vector(GpTermEntry * pThis, uint ux, uint uy);
TERM_PUBLIC void PICT2E_arrow(GpTermEntry * pThis, uint sx, uint sy, uint ex, uint ey, int head);
TERM_PUBLIC void PICT2E_fillbox(GpTermEntry * pThis, int style, uint x1, uint y1, uint width, uint height);
TERM_PUBLIC void PICT2E_filled_polygon(GpTermEntry * pThis, int points, gpiPoint * corners);

#define PICT2E_PTS_PER_INCH (72.27)
#define PICT2E_DPI (600)        /* resolution of printer we expect to use */
#define PICT2E_UNIT (PICT2E_PTS_PER_INCH/PICT2E_DPI)    /* dot size in pt */

// 5 inches wide by 3 inches high (default) 
#define PICT2E_XMAX (5*PICT2E_DPI)      /* (PICT2E_PTS_PER_INCH/PICT2E_UNIT*5.0) */
#define PICT2E_YMAX (3*PICT2E_DPI)      /* (PICT2E_PTS_PER_INCH/PICT2E_UNIT*3.0) */

#define PICT2E_HTIC (5*PICT2E_DPI/72)           /* (5 pts) */
#define PICT2E_VTIC (5*PICT2E_DPI/72)           /* (5 pts) */
#define PICT2E_HCHAR (PICT2E_DPI*53/10/72)      /* (5.3 pts) */
#define PICT2E_VCHAR (PICT2E_DPI*11/72) /* (11 pts) */
//#endif

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY

//static int pict2e_posx;
//static int pict2e_posy;
//static int pict2e_fontsize = 10;
//static char pict2e_font[MAX_ID_LEN+1] = "";
//static enum JUSTIFY pict2e_justify = LEFT;
//static int pict2e_angle = 0;
//static bool pict2e_explicit_size = FALSE;
//static GpSizeUnits pict2e_explicit_units = INCHES;

// Default line-drawing character 
// the definition of plotpoint varies with linetype 
#define PICT2E_DOT "\\usebox{\\plotpoint}"
#define PICT2E_TINY_DOT "\\rule[-0.5pt]{1pt}{1pt}"      /* for dots plot style */
#define PICT2E_POINT_TYPES 15
#define PICT2E_NUM_COLORS 6
static const char * PICT2E_lt_colors[] = { "red", "green", "blue", "magenta", "cyan", "yellow" };

// COLORS 
static void PICT2E_apply_color();
static void PICT2E_apply_opacity();

//static bool pict2e_use_color = TRUE; /* LATEX terminal option */
//static char pict2e_color[32] = "";
//static char pict2e_new_color[32] = "";
//#ifdef PICT2E_TRANSPARENT
//static int pict2e_opacity = 100;
//static int pict2e_new_opacity = 100;
//#endif
//static bool pict2e_have_color = FALSE;

// POINTS 
//static bool pict2e_points = TRUE;  // use TeX symbols for points
//static int pict2e_pointsize = 0;
static const char * pict2e_point_type[PICT2E_POINT_TYPES] = {
	"\\makebox(0,0){$%s+$}",
	"\\makebox(0,0){$%s\\times$}",
	"\\makebox(0,0){$%s\\ast$}",
	"\\raisebox{-.8pt}{\\makebox(0,0){$%s\\Box$}}",
	"\\makebox(0,0){$%s\\blacksquare$}",
	"\\makebox(0,0){$%s\\circ$}",
	"\\makebox(0,0){$%s\\bullet$}",
	"\\makebox(0,0){$%s\\triangle$}",
	"\\makebox(0,0){$%s\\blacktriangle$}",
	"\\makebox(0,0){$%s\\triangledown$}",
	"\\makebox(0,0){$%s\\blacktriangledown$}",
	"\\makebox(0,0){$%s\\lozenge$}",
	"\\makebox(0,0){$%s\\blacklozenge$}",
	"\\makebox(0,0){$%s\\heartsuit$}",
	"\\makebox(0,0){$%s\\spadesuit$}"
};

// LINES 
static void PICT2E_linesize();
static void PICT2E_solid_line(int x1, int x2, int y1, int y2);
static void PICT2E_dot_line(int x1, int x2, int y1, int y2);
static void PICT2E_flushdot();
static void PICT2E_pushpath(uint x, uint y);
static void PICT2E_endline();
static void PICT2E_flushline();

#define PICT2E_DOT_SPACE 3.0
#define PICT2E_LINEMAX 100 // max value for linecount 

//static float pict2e_size = 0; // current line thickness 
//static float pict2e_lw;
//static float pict2e_lw_scale = 1.0;
//static bool pict2e_rounded = FALSE;
//static float pict2e_dotspace = 0; // current dotspace of line in points 
//static bool pict2e_inline; // are we in the middle of a line 
//static int pict2e_linecount = 0; // number of points in line so far 
//static uint pict2e_path[PICT2E_LINEMAX][2]; // point stack 
//static bool pict2e_moved = TRUE; // pen is up after move 
//static float pict2e_dotsize; // size of PICT2E_DOT in units 
//static bool pict2e_needsdot = FALSE; // does dotted line need termination? 

// AREA FILLING 
static int PICT2E_fill(int style);
typedef enum {
	pict2e_no_fill, 
	pict2e_fill, 
	pict2e_fill_and_restore, 
	pict2e_fill_transparent
} pict2e_fill_cmds;

// ARROWS 
static void PICT2E_do_arrow(int, int, int, int, int);
//static bool pict2e_arrows = TRUE;  // use LaTeX arrows

// OPTIONS 
enum PICT2E_id {
	PICT2E_DEFAULT,
	PICT2E_SIZE,
	PICT2E_FONT,
	// PICT2E_STANDALONE, PICT2E_INPUT,
	PICT2E_COLOR, PICT2E_MONOCHROME,
	PICT2E_LINEWIDTH,
	PICT2E_ROUNDED, PICT2E_BUTT,
	PICT2E_POINTSCALE,
	PICT2E_TEXARROWS, PICT2E_GPARROWS,
	PICT2E_TEXPOINTS, PICT2E_GPPOINTS,
	PICT2E_NORMALPOINTS, PICT2E_SMALLPOINTS, PICT2E_TINYPOINTS,
	// PICT2E_BACKGROUND,
	PICT2E_OTHER
};

static struct gen_table PICT2E_opts[] =
{
	{ "d$efault", PICT2E_DEFAULT },
	{ "fo$nt", PICT2E_FONT },
	{ "si$ze", PICT2E_SIZE },
	// { "stand$alone", PICT2E_STANDALONE },
	// { "inp$ut", PICT2E_INPUT },
	{ "color", PICT2E_COLOR },
	{ "colour", PICT2E_COLOR },
	{ "mono$chrome", PICT2E_MONOCHROME },
	// { "backg$round", PICT2E_BACKGROUND },
	{ "lw", PICT2E_LINEWIDTH },
	{ "linew$idth", PICT2E_LINEWIDTH },
	{ "round$ed", PICT2E_ROUNDED },
	{ "butt", PICT2E_BUTT },
	// { "points$cale", PICT2E_POINTSCALE },
	// { "ps", PICT2E_POINTSCALE },
	{ "texarrows", PICT2E_TEXARROWS },
	{ "gparrows", PICT2E_GPARROWS },
	{ "texpoints", PICT2E_TEXPOINTS },
	{ "gppoints", PICT2E_GPPOINTS },
	{ "norm$alpoints", PICT2E_NORMALPOINTS },
	{ "sma$llpoints", PICT2E_SMALLPOINTS },
	{ "tin$ypoints", PICT2E_TINYPOINTS },
	{ NULL, PICT2E_OTHER }
};

struct GpPict2E_TerminalBlock {
	GpPict2E_TerminalBlock() : /*pict2e_posx(0), pict2e_posy(0),*/ pict2e_fontsize(10), pict2e_justify(LEFT),
		pict2e_angle(0), pict2e_explicit_size(false), pict2e_explicit_units(INCHES), pict2e_use_color(true), pict2e_have_color(false),
		#ifdef PICT2E_TRANSPARENT
		pict2e_opacity(100), pict2e_new_opacity(100),
		#endif
		pict2e_points(true), pict2e_pointsize(0),
		pict2e_size(0.0f), pict2e_lw(0.0f), pict2e_lw_scale(1.0f), pict2e_dotspace(0.0f),pict2e_linecount(0), pict2e_dotsize(0.0f),
		pict2e_rounded(false), pict2e_inline(false), pict2e_moved(true), pict2e_needsdot(false),
		pict2e_arrows(true)
	{
		PTR32(pict2e_font)[0] = 0;
		PTR32(pict2e_color)[0] = 0;
		PTR32(pict2e_new_color)[0] = 0;
		memzero(pict2e_path, sizeof(pict2e_path));
	}
	//int    pict2e_posx;
	//int    pict2e_posy;
	SPoint2I Pos;
	int    pict2e_fontsize;
	char   pict2e_font[MAX_ID_LEN+1];
	enum   JUSTIFY pict2e_justify;
	int    pict2e_angle;
	GpSizeUnits pict2e_explicit_units;
	char   pict2e_color[32];
	char   pict2e_new_color[32];
	#ifdef PICT2E_TRANSPARENT
	int    pict2e_opacity;
	int    pict2e_new_opacity;
	#endif
	int    pict2e_pointsize;
	float  pict2e_size; // current line thickness 
	float  pict2e_lw;
	float  pict2e_lw_scale;
	float  pict2e_dotspace; // current dotspace of line in points 
	float  pict2e_dotsize; // size of PICT2E_DOT in units 
	int    pict2e_linecount; // number of points in line so far 
	uint   pict2e_path[PICT2E_LINEMAX][2]; // point stack 
	bool   pict2e_explicit_size;
	bool   pict2e_use_color; // LATEX terminal option 
	bool   pict2e_have_color;
	bool   pict2e_points;  // use TeX symbols for points
	bool   pict2e_rounded;
	bool   pict2e_inline; // are we in the middle of a line 
	bool   pict2e_moved; // pen is up after move 
	bool   pict2e_needsdot; // does dotted line need termination? 
	bool   pict2e_arrows;  // use LaTeX arrows
};

static GpPict2E_TerminalBlock _Pict2E;

TERM_PUBLIC void PICT2E_options(GpTermEntry * pThis, GnuPlot * pGp)
{
	char * s;
	_Pict2E.pict2e_explicit_size = false;
	while(!pGp->Pgm.EndOfCommand()) {
		enum PICT2E_id cmd = (enum PICT2E_id)pGp->Pgm.LookupTableForCurrentToken(&PICT2E_opts[0]);
		switch(cmd) {
			case PICT2E_DEFAULT:
			    _Pict2E.pict2e_font[0] = '\0';
			    pGp->Pgm.Shift();
			    break;
			case PICT2E_FONT:
			    pGp->Pgm.Shift();
			    if((s = pGp->TryToGetString()) != NULL) {
				    int fontsize;
				    char * comma = strrchr(s, ',');
				    if(comma && (1 == sscanf(comma + 1, "%i", &fontsize))) {
					    _Pict2E.pict2e_fontsize = fontsize;
					    if(_Pict2E.pict2e_fontsize <= 1)
						    _Pict2E.pict2e_fontsize = 10;
					    *comma = '\0';
				    }
				    if(*s)
					    strnzcpy(_Pict2E.pict2e_font, s, MAX_ID_LEN);
				    SAlloc::F(s);
			    }
			    break;
			case PICT2E_SIZE: {
			    float xmax_t = 5.0f;
				float ymax_t = 3.0f;
			    pGp->Pgm.Shift();
			    _Pict2E.pict2e_explicit_size = TRUE;
			    _Pict2E.pict2e_explicit_units = pGp->ParseTermSize(&xmax_t, &ymax_t, INCHES);
			    pThis->MaxX = static_cast<uint>(xmax_t * PICT2E_DPI / 72);
			    pThis->MaxY = static_cast<uint>(ymax_t * PICT2E_DPI / 72);
			    break;
		    }
			case PICT2E_COLOR:
			    _Pict2E.pict2e_use_color = TRUE;
			    pThis->flags &= ~TERM_MONOCHROME;
			    pGp->Pgm.Shift();
			    break;
			case PICT2E_MONOCHROME:
			    _Pict2E.pict2e_use_color = FALSE;
			    pThis->flags |= TERM_MONOCHROME;
			    pGp->Pgm.Shift();
			    break;
			case PICT2E_GPARROWS:
			    _Pict2E.pict2e_arrows = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case PICT2E_TEXARROWS:
			    _Pict2E.pict2e_arrows = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PICT2E_GPPOINTS:
			    _Pict2E.pict2e_points = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case PICT2E_TEXPOINTS:
			    _Pict2E.pict2e_points = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PICT2E_BUTT:
			    _Pict2E.pict2e_rounded = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case PICT2E_ROUNDED:
			    _Pict2E.pict2e_rounded = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PICT2E_LINEWIDTH:
			    pGp->Pgm.Shift();
			    _Pict2E.pict2e_lw_scale = pGp->FloatExpression();
			    if(_Pict2E.pict2e_lw_scale < 0.0)
				    _Pict2E.pict2e_lw_scale = 1.0;
			    break;
			case PICT2E_NORMALPOINTS:
			    _Pict2E.pict2e_pointsize = 0;
			    pGp->Pgm.Shift();
			    break;
			case PICT2E_SMALLPOINTS:
			    _Pict2E.pict2e_pointsize = 1;
			    pGp->Pgm.Shift();
			    break;
			case PICT2E_TINYPOINTS:
			    _Pict2E.pict2e_pointsize = 2;
			    pGp->Pgm.Shift();
			    break;
			case PICT2E_OTHER:
			default:
			    pGp->Pgm.Shift();
			    pGp->IntErrorCurToken("unrecognized option");
		}
	}
	// tell gnuplot core about char sizes. Horizontal spacing
	// is about half the text pointsize
	pThis->ChrV = (uint)(_Pict2E.pict2e_fontsize * PICT2E_DPI / 72);
	pThis->ChrH = (uint)(_Pict2E.pict2e_fontsize * PICT2E_DPI / 144);
	slprintf(GPT._TermOptions, "font \"%s,%d\"", _Pict2E.pict2e_font, _Pict2E.pict2e_fontsize);
	if(_Pict2E.pict2e_explicit_size) {
		if(_Pict2E.pict2e_explicit_units == CM)
			slprintf_cat(GPT._TermOptions, "size %.2fcm, %.2fcm ", 2.54 * (float)pThis->MaxX / (PICT2E_DPI), 2.54 * (float)pThis->MaxY / (PICT2E_DPI));
		else
			slprintf_cat(GPT._TermOptions, "size %.2fin, %.2fin ", (float)pThis->MaxX / (PICT2E_DPI), (float)pThis->MaxY / (PICT2E_DPI));
	}
	slprintf_cat(GPT._TermOptions, _Pict2E.pict2e_use_color ? " color" : " monochrome");
	slprintf_cat(GPT._TermOptions, " linewidth %.1f", _Pict2E.pict2e_lw_scale);
	slprintf_cat(GPT._TermOptions, _Pict2E.pict2e_points ? " texpoints" : " gppoints");
	slprintf_cat(GPT._TermOptions, _Pict2E.pict2e_pointsize == 1 ? " smallpoints" : (_Pict2E.pict2e_pointsize == 2 ? " tinypoints" : " normalpoints"));
	slprintf_cat(GPT._TermOptions, _Pict2E.pict2e_arrows ? " texarrows" : " gparrows");
}

TERM_PUBLIC void PICT2E_init(GpTermEntry * pThis)
{
	fprintf(GPT.P_GpOutFile, "%% GNUPLOT: LaTeX2e picture (pict2e)\n\\setlength{\\unitlength}{%fpt}\n", PICT2E_UNIT);
	fputs("\\ifx\\plotpoint\\undefined\\newsavebox{\\plotpoint}\\fi\n", GPT.P_GpOutFile);
#ifdef PICT2E_TRANSPARENT
	fputs(// test if command \transparent is available
		"\\ifx\\transparent\\undefined%\n"
		"    \\providecommand{\\gpopaque}{}%\n"
		"    \\providecommand{\\gptransparent}[2]{\\color{.!#2}}%\n"
		"\\else%\n"
		"    \\providecommand{\\gpopaque}{\\transparent{1.0}}%\n"
		"    \\providecommand{\\gptransparent}[2]{\\transparent{#1}}%\n"
		"\\fi%\n",
		GPT.P_GpOutFile);
#endif
}

TERM_PUBLIC void PICT2E_graphics(GpTermEntry * pThis)
{
	// set size of canvas 
	if(!_Pict2E.pict2e_explicit_size) {
		pThis->MaxX = PICT2E_XMAX;
		pThis->MaxY = PICT2E_YMAX;
	}
	fprintf(GPT.P_GpOutFile, "\\begin{picture}(%d,%d)(0,0)\n", pThis->MaxX, pThis->MaxY);
	if(_Pict2E.pict2e_font[0])
		fprintf(GPT.P_GpOutFile, "\\font\\gnuplot=%s10 at %dpt\n\\gnuplot\n", _Pict2E.pict2e_font, _Pict2E.pict2e_fontsize);
	if(_Pict2E.pict2e_rounded)
		fputs("\\roundjoin\\roundcap\n", GPT.P_GpOutFile);
	else
		fputs("\\miterjoin\\buttcap\n", GPT.P_GpOutFile);
	_Pict2E.pict2e_lw = _Pict2E.pict2e_lw_scale;
	_Pict2E.pict2e_size = 0;
	_Pict2E.pict2e_color[0] = 0;
	PICT2E_linetype(pThis, LT_AXIS);
	_Pict2E.pict2e_inline = FALSE;
	_Pict2E.pict2e_linecount = 0;
	_Pict2E.Pos.Z();
}

TERM_PUBLIC void PICT2E_text(GpTermEntry * pThis)
{
	PICT2E_endline();
	fputs("\\end{picture}\n", GPT.P_GpOutFile);
	_Pict2E.Pos.Z(); // current position 
	_Pict2E.pict2e_moved = TRUE; // pen is up after move 
}

TERM_PUBLIC void PICT2E_reset(GpTermEntry * pThis)
{
	_Pict2E.Pos.Z(); // current position 
	_Pict2E.pict2e_moved = TRUE; // pen is up after move 
}

static void PICT2E_linesize()
{
	float size;
	PICT2E_endline();
	// Find the new desired line thickness. 
	size = _Pict2E.pict2e_lw * 0.4f;
	// If different from current size, redefine \plotpoint 
	if(size != _Pict2E.pict2e_size) {
		fprintf(GPT.P_GpOutFile, "\\sbox{\\plotpoint}{\\rule[%.3fpt]{%.3fpt}{%.3fpt}}%%\n", -size / 2, size, size);
		fprintf(GPT.P_GpOutFile, "\\linethickness{%.1fpt}%%\n", size);
	}
	_Pict2E.pict2e_size = size;
	_Pict2E.pict2e_dotsize = static_cast<float>(size / PICT2E_UNIT);
	_Pict2E.pict2e_moved = TRUE;            /* reset */
}

TERM_PUBLIC void PICT2E_linetype(GpTermEntry * pThis, int linetype)
{
	t_colorspec colorspec;
	PICT2E_endline();
	// save values for set_color 
	colorspec.type = TC_LT;
	colorspec.lt = linetype;
	PICT2E_set_color(pThis, &colorspec);
	// all but axis lines are solid by default 
	if(linetype == LT_AXIS)
		_Pict2E.pict2e_dotspace = PICT2E_DOT_SPACE;
	else
		_Pict2E.pict2e_dotspace = 0.0;
}

TERM_PUBLIC void PICT2E_dashtype(GpTermEntry * pThis, int dt, t_dashtype * custom_dash_pattern)
{
	if(dt >= 0) {
		int linetype = dt % 3;
		_Pict2E.pict2e_dotspace = static_cast<float>(PICT2E_DOT_SPACE * linetype);
	}
	else if(dt == DASHTYPE_SOLID) {
		_Pict2E.pict2e_dotspace = 0.0;
	}
	else if(dt == DASHTYPE_AXIS) {
		_Pict2E.pict2e_dotspace = PICT2E_DOT_SPACE;
	}
	else if(dt == DASHTYPE_CUSTOM) {
		/* not supported */
	}
}

TERM_PUBLIC void PICT2E_linewidth(GpTermEntry * pThis, double linewidth)
{
	_Pict2E.pict2e_lw = static_cast<float>(linewidth * _Pict2E.pict2e_lw_scale);
}

TERM_PUBLIC void PICT2E_move(GpTermEntry * pThis, uint x, uint y)
{
	PICT2E_endline();
	_Pict2E.Pos.Set(x, y);
	_Pict2E.pict2e_moved = TRUE; // reset 
}

TERM_PUBLIC void PICT2E_point(GpTermEntry * pThis, uint x, uint y, int number)
{
	const char * size[] = { "", "\\scriptstyle", "\\scriptscriptstyle" };
	char point[80];
	PICT2E_apply_color();
	PICT2E_apply_opacity();
	if(_Pict2E.pict2e_points) {
		PICT2E_move(pThis, x, y);
		// Print the character defined by 'number'; number < 0 means to use a dot, otherwise one of the defined points. 
		if(number >= 0)
			snprintf(point, sizeof(point), pict2e_point_type[number % PICT2E_POINT_TYPES], size[_Pict2E.pict2e_pointsize]);
		fprintf(GPT.P_GpOutFile, "\\put(%d,%d){%s}\n", x, y, (number < 0 ? PICT2E_TINY_DOT : point));
	}
	else {
		GnuPlot::DoPoint(pThis, x, y, number);
	}
}

static void PICT2E_endline()
{
	PICT2E_flushline();
	PICT2E_flushdot();
}

static void PICT2E_pushpath(uint x, uint y)
{
	if(_Pict2E.pict2e_linecount < PICT2E_LINEMAX) {
		_Pict2E.pict2e_path[_Pict2E.pict2e_linecount][0] = x;
		_Pict2E.pict2e_path[_Pict2E.pict2e_linecount][1] = y;
		_Pict2E.pict2e_linecount++;
	}
}

TERM_PUBLIC void PICT2E_vector(GpTermEntry * pThis, uint ux, uint uy)
{
	if(!_Pict2E.pict2e_inline) {
		PICT2E_apply_color();
		PICT2E_apply_opacity();
		PICT2E_linesize();
	}
	if(_Pict2E.pict2e_dotspace == 0.0)
		PICT2E_solid_line(_Pict2E.Pos.x, (int)ux, _Pict2E.Pos.y, (int)uy);
	else
		PICT2E_dot_line(_Pict2E.Pos.x, (int)ux, _Pict2E.Pos.y, (int)uy);
	_Pict2E.Pos.Set(ux, uy);
}

static void PICT2E_dot_line(int x1, int x2, int y1, int y2)
{
	static float PICT2E_left; /* fraction of space left after last dot */
	// we draw a dotted line using the current dot spacing 
	if(_Pict2E.pict2e_moved)
		PICT2E_left = 1.0; /* reset after a move */
	// zero-length line? 
	if(x1 == x2 && y1 == y2) {
		if(_Pict2E.pict2e_moved)
			// plot a dot 
			fprintf(GPT.P_GpOutFile, "\\put(%u,%u){%s}\n", x1, y1, PICT2E_DOT);
	}
	else {
		float dotspace = static_cast<float>(_Pict2E.pict2e_dotspace / PICT2E_UNIT);
		float x, y;     /* current position */
		float xinc, yinc; /* increments */
		float slope;    /* slope of line */
		float lastx = -1; /* last x point plotted */
		float lasty = -1; /* last y point plotted */
		int numdots = 0; /* number of dots in this section */
		// first, figure out increments for x and y 
		if(x2 == x1) {
			xinc = 0.0;
			yinc = (y2 - y1 > 0) ? dotspace : -dotspace;
		}
		else {
			slope = ((float)y2 - y1) / ((float)x2 - x1);
			xinc = static_cast<float>(dotspace / sqrt(1 + slope * slope) * sign(x2 - x1));
			yinc = slope * xinc;
		}
		// now draw the dotted line 
		// we take into account where we last placed a dot 
		for(x = x1 + xinc * (1 - PICT2E_left), y = y1 + yinc * (1 - PICT2E_left);
		    (x2 - x) * xinc >= 0 && (y2 - y) * yinc >= 0; /* same sign or zero */
		    lastx = x, x += xinc, lasty = y, y += yinc)
			numdots++;
		if(numdots == 1)
			fprintf(GPT.P_GpOutFile, "\\put(%.2f,%.2f){%s}\n", lastx, lasty, PICT2E_DOT);
		else if(numdots > 0)
			fprintf(GPT.P_GpOutFile, "\\multiput(%u,%u)(%.3f,%.3f){%u}{%s}\n", x1, y1, xinc, yinc, numdots, PICT2E_DOT);
		// how much is left over, as a fraction of dotspace? 
		if(xinc != 0.0) { /* xinc must be nonzero */
			if(lastx >= 0)
				PICT2E_left = ABS(x2 - lastx) / ABS(xinc);
			else
				PICT2E_left += ABS(x2 - x1) / ABS(xinc);
		}
		else if(lasty >= 0)
			PICT2E_left = ABS(y2 - lasty) / ABS(yinc);
		else
			PICT2E_left += ABS(y2 - y1) / ABS(yinc);
	}
	_Pict2E.pict2e_needsdot = (PICT2E_left > 0);
	_Pict2E.pict2e_moved = FALSE;
}

static void PICT2E_flushdot()
{
	if(_Pict2E.pict2e_needsdot)
		fprintf(GPT.P_GpOutFile, "\\put(%d,%d){%s}\n", _Pict2E.Pos.x, _Pict2E.Pos.y, PICT2E_DOT);
	_Pict2E.pict2e_needsdot = FALSE;
}

TERM_PUBLIC void PICT2E_arrow(GpTermEntry * pThis, uint sx, uint sy, uint ex, uint ey, int head)
{
	PICT2E_apply_color();
	PICT2E_apply_opacity();
	PICT2E_linesize();
	if(_Pict2E.pict2e_arrows)
		// This mostly converts the parameters to signed.
		PICT2E_do_arrow(sx, sy, ex, ey, head);
	else
		GnuPlot::DoArrow(pThis, sx, sy, ex, ey, head);
	_Pict2E.Pos.Set(ex, ey);
}

static void PICT2E_do_arrow(int sx, int sy, int ex, int ey/* start and end points */, int head)
{
	int dx = ex - sx;
	int dy = ey - sy;
	float len;
	if((head & BOTH_HEADS) == BACKHEAD) {
		// we need to draw only the backhead, so we exchange start and stop coordinates 
		int tx, ty;
		tx = ex;  ex = sx;  sx = tx;
		ty = ey;  ey = sy;  sy = ty;
		dx *= -1;
		dy *= -1;
		head &= ~BOTH_HEADS;
		head |= END_HEAD;
	}
	// pict2e has no restriction on slope
	len = static_cast<float>(sqrt(dx * dx + dy * dy));
	dx /= len / 100.0;
	dy /= len / 100.0;
	// TODO: divide by GCD
	if((head & HEADS_ONLY) != 0) {
		if((head & END_HEAD) != 0) {
			fprintf(GPT.P_GpOutFile, "\\put(%d,%d){\\vector(%d,%d){0}}\n", ex, ey, dx, dy);
		}
	}
	else {
		fprintf(GPT.P_GpOutFile, "\\put(%d,%d){\\%s(%d,%d){%d}}\n", sx, sy, head ? "vector" : "line", dx, dy, dx != 0 ? ABS(ex - sx) : ABS(ey - sy));
	}
	if((head & BACKHEAD) != 0) {
		fprintf(GPT.P_GpOutFile, "\\put(%d,%d){\\vector(%d,%d){0}}\n", sx, sy, -dx, -dy);
	}
}

TERM_PUBLIC void PICT2E_put_text(GpTermEntry * pThis, uint x, uint y, const char str[])
{
	static const char * justify[] = { "[l]", "", "[r]" };
	// ignore empty strings 
	if(!str[0])
		return;
	PICT2E_endline();
	PICT2E_apply_color();
	PICT2E_apply_opacity();
	fprintf(GPT.P_GpOutFile, "\\put(%d,%d)", x, y);
	if(_Pict2E.pict2e_angle != 0)
		fprintf(GPT.P_GpOutFile, "{\\rotatebox{%d}", _Pict2E.pict2e_angle);
	fprintf(GPT.P_GpOutFile, "{\\makebox(0,0)%s{%s}}", justify[_Pict2E.pict2e_justify], str);
	if(_Pict2E.pict2e_angle != 0)
		fputs("}", GPT.P_GpOutFile);
	fputs("\n", GPT.P_GpOutFile);
}

TERM_PUBLIC int PICT2E_justify_text(GpTermEntry * pThis, enum JUSTIFY mode)
{
	_Pict2E.pict2e_justify = mode;
	return TRUE;
}

TERM_PUBLIC int PICT2E_text_angle(GpTermEntry * pThis, int ang)
{
	_Pict2E.pict2e_angle = ang;
	return TRUE;
}

TERM_PUBLIC int PICT2E_make_palette(GpTermEntry * pThis, t_sm_palette * palette)
{
	return 0; // we can do continuous colors 
}

static void PICT2E_apply_color()
{
	if(strcmp(_Pict2E.pict2e_new_color, _Pict2E.pict2e_color) != 0) {
		strcpy(_Pict2E.pict2e_color, _Pict2E.pict2e_new_color);
		if(_Pict2E.pict2e_use_color) {
			fputs(_Pict2E.pict2e_new_color, GPT.P_GpOutFile);
			_Pict2E.pict2e_have_color = TRUE;
		}
	}
}

static void PICT2E_apply_opacity()
{
	if(!_Pict2E.pict2e_use_color)
		return;
#ifdef PICT2E_TRANSPARENT
	if(_Pict2E.pict2e_opacity != _Pict2E.pict2e_new_opacity) {
		_Pict2E.pict2e_opacity = _Pict2E.pict2e_new_opacity;
		if(!_Pict2E.pict2e_have_color)
			fputs(_Pict2E.pict2e_color, GPT.P_GpOutFile);
		if(_Pict2E.pict2e_opacity != 100)
			fprintf(GPT.P_GpOutFile, "\\gptransparent{%.2f}{%d}\n", _Pict2E.pict2e_opacity / 100.0, _Pict2E.pict2e_opacity);
		else
			fputs("\\gpopaque\n", GPT.P_GpOutFile);
		_Pict2E.pict2e_have_color = FALSE;
	}
#endif
}

TERM_PUBLIC void PICT2E_set_color(GpTermEntry * pThis, const t_colorspec * colorspec)
{
	if(!_Pict2E.pict2e_use_color)
		return;
	switch(colorspec->type) {
		case TC_RGB: {
		    double r = (double)((colorspec->lt >> 16 ) & 255) / 255.0;
		    double g = (double)((colorspec->lt >> 8 ) & 255) / 255.0;
		    double b = (double)(colorspec->lt & 255) / 255.0;
		    snprintf(_Pict2E.pict2e_new_color, sizeof(_Pict2E.pict2e_new_color), "\\color[rgb]{%3.2f,%3.2f,%3.2f}\n", r, g, b);
		    break;
	    }
		case TC_LT: {
		    int linetype = colorspec->lt;
		    const char * colorname;
		    if(linetype == LT_BACKGROUND)
			    colorname = "white";
		    else if(linetype < 0 || !_Pict2E.pict2e_use_color)
			    colorname = "black";
		    else
			    colorname = PICT2E_lt_colors[linetype % PICT2E_NUM_COLORS];
		    snprintf(_Pict2E.pict2e_new_color, sizeof(_Pict2E.pict2e_new_color), "\\color{%s}\n", colorname);
		    break;
	    }
		case TC_FRAC: {
		    rgb_color color;
		    pThis->P_Gp->Rgb1MaxColorsFromGray(colorspec->value, &color);
		    snprintf(_Pict2E.pict2e_new_color, sizeof(_Pict2E.pict2e_new_color), "\\color[rgb]{%3.2f,%3.2f,%3.2f}\n", color.r, color.g, color.b);
		    break;
	    }
		default:
		    break;
	}
}

static void PICT2E_flushline()
{
	if(_Pict2E.pict2e_inline) {
		int i;
		if(_Pict2E.pict2e_linecount >= 2) {
			if(_Pict2E.pict2e_linecount == 2) {
				// short line segment
				fputs("\\Line", GPT.P_GpOutFile);
			}
			else if((_Pict2E.pict2e_path[0][0] == _Pict2E.pict2e_path[_Pict2E.pict2e_linecount-1][0]) && (_Pict2E.pict2e_path[0][1] == _Pict2E.pict2e_path[_Pict2E.pict2e_linecount-1][1])) {
				// closed path
				fputs("\\polygon", GPT.P_GpOutFile);
				_Pict2E.pict2e_linecount--;
			}
			else {
				// multiple connected line segments
				fputs("\\polyline", GPT.P_GpOutFile);
			}
			for(i = 0; i < _Pict2E.pict2e_linecount; i++)
				fprintf(GPT.P_GpOutFile, "(%d,%d)", _Pict2E.pict2e_path[i][0], _Pict2E.pict2e_path[i][1]);
			fputs("\n", GPT.P_GpOutFile);
		}
		_Pict2E.pict2e_inline = FALSE;
		_Pict2E.pict2e_linecount = 0;
	}
}

static void PICT2E_solid_line(int x1, int x2, int y1, int y2)
{
	if(!_Pict2E.pict2e_inline)
		PICT2E_pushpath(x1, y1);
	_Pict2E.pict2e_inline = TRUE;
	PICT2E_pushpath(x2, y2);
	if(_Pict2E.pict2e_linecount == PICT2E_LINEMAX) {
		PICT2E_flushline();
		PICT2E_pushpath(x2, y2);
	}
	_Pict2E.Pos.Set(x2, y2);
}

static int PICT2E_fill(int style)
{
	int fill = pict2e_fill;
	int opt = style >> 4;
	switch(style & 0xf) {
		case FS_SOLID:
		    if(_Pict2E.pict2e_use_color) {
			    if(opt != 100) {
				    _Pict2E.pict2e_color[0] = '\0';
				    fprintf(GPT.P_GpOutFile, "\\color{.!%d}\n", opt);
				    fill = pict2e_fill_and_restore;
			    }
			    else {
				    fill = pict2e_fill;
			    }
		    }
		    else if(opt < 50) {
			    fill = pict2e_no_fill;
		    }
		    break;
		case FS_TRANSPARENT_SOLID:
		    if(_Pict2E.pict2e_use_color) {
			    if(opt != 100) {
#ifdef PICT2E_TRANSPARENT
				    _Pict2E.pict2e_new_opacity = opt;
				    fill = pict2e_fill_transparent;
#else
				    pict2e_color[0] = '\0';
				    fprintf(GPT.P_GpOutFile, "\\color{.!%d}\n", opt);
				    fill = pict2e_fill_and_restore;
#endif
			    }
			    else {
				    fill = pict2e_fill;
			    }
		    }
		    else if(opt < 50) {
			    fill = pict2e_no_fill;
		    }
		    break;
		case FS_PATTERN:
		case FS_TRANSPARENT_PATTERN:
		    if(_Pict2E.pict2e_use_color) {
			    opt %= 4;
			    if(opt == 0)
				    fputs("\\color{white}\n", GPT.P_GpOutFile);
			    else if(opt == 1)
				    fputs("\\color{.!50}\n", GPT.P_GpOutFile);
			    else if(opt == 2)
				    fputs("\\color{.!20}\n", GPT.P_GpOutFile);
			    else if(opt == 3)
				    if(strcmp(_Pict2E.pict2e_color, "\\color{black}\n") != 0) {
					    fputs("\\color{black}\n", GPT.P_GpOutFile);
					    _Pict2E.pict2e_color[0] = '\0';
				    }
			    if(opt != 3)
				    _Pict2E.pict2e_color[0] = '\0';
			    fill = pict2e_fill_and_restore;
		    }
		    else {
			    if((opt % 2) == 0)
				    fill = pict2e_no_fill;
			    else
				    fill = pict2e_fill;
		    }
		    break;
		case FS_EMPTY:
		    if(_Pict2E.pict2e_use_color) {
			    _Pict2E.pict2e_color[0] = '\0';
			    fputs("\\color{white}\n", GPT.P_GpOutFile);
			    fill = pict2e_fill_and_restore;
		    }
		    else {
			    fill = pict2e_no_fill;
		    }
		    break;
		case FS_DEFAULT:
		default:
		    /* use currently active color */
		    fill = pict2e_fill;
		    break;
	}
	return fill;
}

TERM_PUBLIC void PICT2E_fillbox(GpTermEntry * pThis, int style, uint x1, uint y1, uint width, uint height)
{
	int ret;
	PICT2E_move(pThis, x1, y1);
	PICT2E_apply_color();
	// determine fill color
	if((ret = PICT2E_fill(style)) == pict2e_no_fill)
		return;
	PICT2E_apply_opacity();
	// outline box
	fprintf(GPT.P_GpOutFile, "\\polygon*(%d,%d)(%d,%d)(%d,%d)(%d,%d)\n", x1, y1, x1 + width, y1, x1 + width, y1 + height, x1, y1 + height);
#ifdef PICT2E_TRANSPARENT
	_Pict2E.pict2e_new_opacity = 100;
#endif
}

TERM_PUBLIC void PICT2E_filled_polygon(GpTermEntry * pThis, int points, gpiPoint * corners)
{
	int i, ret;
	PICT2E_move(pThis, corners[0].x, corners[0].y);
	PICT2E_apply_color();
	// determine fill color
	if((ret = PICT2E_fill(corners->style)) == pict2e_no_fill)
		return;
	PICT2E_apply_opacity();
	// no need to list the endpoint
	if((corners[0].x == corners[points-1].x) && (corners[0].y == corners[points-1].y))
		points--;
	// need at least 3 unique points
	if(points < 3)
		return;
	fprintf(GPT.P_GpOutFile, "\\polygon*(%d,%d)", corners[0].x, corners[0].y);
	for(i = 0; i < points; i++)
		fprintf(GPT.P_GpOutFile, "(%d,%d)", corners[i].x, corners[i].y);
	fputs("\n", GPT.P_GpOutFile);
#ifdef PICT2E_TRANSPARENT
	_Pict2E.pict2e_new_opacity = 100;
#endif
}

#endif /* TERM_BODY */
#endif /* TERM_PROTO_ONLY */

#ifdef TERM_TABLE

TERM_TABLE_START(pict2e_driver)
	"pict2e", 
	"LaTeX2e picture environment",
	PICT2E_XMAX, 
	PICT2E_YMAX, 
	PICT2E_VCHAR, 
	PICT2E_HCHAR,
	PICT2E_VTIC, 
	PICT2E_HTIC, 
	PICT2E_options, 
	PICT2E_init, 
	PICT2E_reset,
	PICT2E_text, 
	GnuPlot::NullScale, 
	PICT2E_graphics, 
	PICT2E_move, 
	PICT2E_vector,
	PICT2E_linetype,
	PICT2E_put_text,
	PICT2E_text_angle,
	PICT2E_justify_text, 
	PICT2E_point, 
	PICT2E_arrow, 
	set_font_null,
	NULL,     /* pointsize */
	TERM_IS_LATEX | TERM_CAN_DASH | TERM_LINEWIDTH,     /* flags */
	NULL, 
	NULL,     /* suspend, resume */
	PICT2E_fillbox, 
	PICT2E_linewidth,
	#ifdef USE_MOUSE
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL,
	#endif
	PICT2E_make_palette, 
	0,
	PICT2E_set_color,
	PICT2E_filled_polygon,
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
	PICT2E_dashtype 
TERM_TABLE_END(pict2e_driver)

#undef LAST_TERM
#define LAST_TERM pict2e_driver

#endif /* TERM_TABLE */

#ifdef TERM_HELP
START_HELP(pict2e)
"1 pict2e",
"?commands set terminal pict2e",
"?set terminal pict2e",
"?set term pict2e",
"?terminal pict2e",
"?term pict2e",
"?pict2e",
" The `pict2e` terminal uses the LaTeX2e variant of the picture environment.",
" It replaces terminals which were based on the original LaTeX picture",
" environment: `latex`, `emtex`, `tpic`, and `eepic`. (EXPERIMENTAL)",
"",
" Alternatives to this terminal with a more complete support of gnuplot's",
" features are `tikz`, `pstricks`, `cairolatex`, `pslatex`, `epslatex`",
" and `mp`.",
"",
" Syntax:",
"       set terminal pict2e",
"                    {font \"{<fontname>}{,<fontsize>}\"}",
"                    {size <XX>{unit}, <YY>{unit}}",
"                    {color | monochrome}",
"                    {linewidth <lw>} {rounded | butt}",
"                    {texarrows | gparrows} {texpoints | gppoints}",
"                    {smallpoints | tinypoints | normalpoints}",
"",
" This terminal requires the following standard LaTeX packages: `pict2e`,",
" `xcolor`, `graphics`/`graphicx` and `amssymb`. For pdflatex, the",
" `transparent` package is used to support transparency.",
"",
" By default the plot will inherit font settings from the embedding document.",
" You have the option to force a font with the `font` option, like cmtt",
" (Courier) or cmr (Roman), instead. In this case you may also force a specific",
" fontsize. Otherwise the fontsize argument is used to estimate the required",
" space for text.",
" Unless your driver is capable of building fonts at any size (e.g. dvips),",
" stick to the standard 10, 11 and 12 point sizes.",
"",
" The default size for the plot is 5 inches by 3 inches. The `size` option",
" changes this to whatever the user requests. By default the X and Y sizes",
" are taken to be in inches, but other units are possible (currently only cm).",
"",
" With `texpoints`, points are drawn using LaTeX commands like \"\\Diamond\"",
" and \"\\Box\".  These are provided by the the latexsym package, which is part",
" of the base distribution and thus part of any LaTeX implementation.",
" Other point types use symbols from the amssymb package.",
" With `gppoints`, the terminal will use gnuplot's internal routines for",
" drawing point symbols instead.",
"",
" With the `texpoints` option, you can select three different point sizes:",
" `normalpoints`, `smallpoints`, and `tinypoints`.",
"",
" `color` causes gnuplot to produce \\color{...} commands so that the graphs",
" are colored. Using this option, you must include \\usepackage{xcolor}",
" in the preamble of your LaTeX document. `monochrome` will avoid the use of",
" any color commands in the output.",
" Transparent color fill is available if pdflatex is used.",
"",
" `linewidth` sets the scale factor for the width of lines.",
" `rounded` sets line caps and line joins to be rounded. `butt` sets butt",
" caps and mitered joins and is the default.",
"",
" `pict2e` only supports dotted lines, but not dashed lines.",
" All default line types are solid. Use `set linetype` with the `dashtype`",
" property to change.",
"",
" `texarrows` draws `arrow`s using LaTeX commands which are shorter but do",
" not offer all options. `gparrows` selects drawing arrows using gnuplot's own",
" routine for full functionality instead.",
""
END_HELP(pict2e)
#endif /* TERM_HELP */
