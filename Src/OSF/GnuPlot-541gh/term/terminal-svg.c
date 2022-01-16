// Hello, Emacs, this is -*-C-*- 
// GNUPLOT - svg.trm
// 
// This file is included by ../term.c.
// This terminal driver supports: W3C Scalable Vector Graphics
// AUTHOR: Amedeo Farello afarello@libero.it
// HEAVILY MODIFIED by: Hans-Bernhard Br"oker  broeker@physik.rwth-aachen.de
// DomTerm support: Per Bothner  <per@bothner.com>
// 
/* PM3D support by Johannes Zellner <johannes@zellner.org>, May-16-2002 */
/* set_color fixes by Petr Mikulik <mikulik@physics.muni.cz>, June-10-2002 */
/* ISO-Latin encoding, Font selection fixes, option "fixed|dynamic" by
 * Wilhelm Braunschober <Wilhelm.Braunschober@t-online.de>, Feb-21-2002 */

/*
 * Additional code for gnuplot versions 4.2 and 4.3
 * Ethan Merritt  <merritt@u.washington.edu>
 *
 *   Tweaked code for compatibility with Sodipodi svg viewer/editor.
 *   Added enhanced text support.
 *   Additional line properties.
 *   Increase resolution by adding a coordinate scale factor.
 *   Support dashed lines, TC_* color model.
 *   Change path markup from    style='attribute: foo'  to   attribute='foo'
 *
 * Additional code for gnuplot versions 4.5
 * Ethan Merritt  <merritt@u.washington.edu>
 *
 *   Wrap each plot in a named group <g id="name_plot_%02d">
 *   Set the name using 'set term svg name "foo"'
 *   Background option
 *   Bitmap image support by creating and linking to external png files
 *   Mouse-tracking with coordinate readout.
 *   Version 4.7 (April 2012) hypertext support
 *
 * Contributed by <plotter@piments.com>
 *   Javascript code to toggle plots on/off
 *
 * Revised font sizing Oct 2012
 *   specify font-size without "pt" units.
 *
 * Inline image data in Base64 encoding
 *   Daniel Sebald May 2016
 *
 * Transition to SVG 2.0
 *   remove DTD
 *   remove option "fontfile" and references to SVG fonts
 */
/*
 * Code for gnuplot version 4.5
 *   Bold -> font-weight:bold
 *   Italic -> font-style:italic
 * Rich Seymour <rseymour@usc.edu>
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
	register_term(svg)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void SVG_options(GpTermEntry_Static * pThis, GnuPlot * pGp);
TERM_PUBLIC void SVG_init(GpTermEntry_Static * pThis);
TERM_PUBLIC void SVG_graphics(GpTermEntry_Static * pThis);
TERM_PUBLIC void SVG_text(GpTermEntry_Static * pThis);
TERM_PUBLIC void SVG_linetype(GpTermEntry_Static * pThis, int linetype);
TERM_PUBLIC void SVG_dashtype(GpTermEntry_Static * pThis, int type, t_dashtype * custom_dash_type);
TERM_PUBLIC void SVG_move(GpTermEntry_Static * pThis, uint x, uint y);
TERM_PUBLIC void SVG_vector(GpTermEntry_Static * pThis, uint x, uint y);
TERM_PUBLIC void SVG_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str);
TERM_PUBLIC void SVG_reset(GpTermEntry_Static * pThis);
TERM_PUBLIC int SVG_justify_text(GpTermEntry_Static * pThis, enum JUSTIFY mode);
TERM_PUBLIC int SVG_text_angle(GpTermEntry_Static * pThis, int ang);
TERM_PUBLIC void SVG_point(GpTermEntry_Static * pThis, uint x, uint y, int pointstyle);
TERM_PUBLIC int SVG_set_font(GpTermEntry_Static * pThis, const char * font);
// TERM_PUBLIC void SVG_pointsize(double pointsize); 
TERM_PUBLIC void SVG_fillbox(GpTermEntry_Static * pThis, int style, uint x1, uint y1, uint width, uint height);
TERM_PUBLIC void SVG_linewidth(GpTermEntry_Static * pThis, double linewidth);
TERM_PUBLIC int SVG_make_palette(GpTermEntry_Static * pThis, t_sm_palette *);
TERM_PUBLIC void SVG_previous_palette(GpTermEntry_Static * pThis);
TERM_PUBLIC void SVG_set_color(GpTermEntry_Static * pThis, const t_colorspec *);
TERM_PUBLIC void SVG_filled_polygon(GpTermEntry_Static * pThis, int, gpiPoint *);
TERM_PUBLIC void SVG_layer(GpTermEntry_Static * pThis, t_termlayer syncpoint);
TERM_PUBLIC void ENHsvg_OPEN(GpTermEntry_Static * pThis, const char * pFontName, double, double, bool, bool, int);
TERM_PUBLIC void ENHsvg_FLUSH(GpTermEntry_Static * pThis);
TERM_PUBLIC void ENHsvg_put_text(GpTermEntry_Static * pThis, uint, uint, const char *);
TERM_PUBLIC void ENHsvg_writec(GpTermEntry_Static * pThis, int);
TERM_PUBLIC void SVG_path(GpTermEntry_Static * pThis, int p);
TERM_PUBLIC void SVG_hypertext(GpTermEntry_Static * pThis, int, const char *);
#ifdef WRITE_PNG_IMAGE
	TERM_PUBLIC void SVG_image(GpTermEntry_Static * pThis, uint m, uint n, coordval * image, gpiPoint * corner, t_imagecolor color_mode);
	static int SVG_imageno = 0;
#endif
#define SVG_SCALE       100.    /* Coordinate accuracy is 1/SVG_SCALE pixel */
#define PREC            2       /* Decimal places needed for SVG_SCALEd values */
#define Y(y) ((float)((int)pThis->MaxY - (int)y) / SVG_SCALE)
#define X(x) ((float)(x) / SVG_SCALE)
#define SVG_XMAX        (600 * SVG_SCALE)
#define SVG_YMAX        (480 * SVG_SCALE)
//#endif // TERM_PROTO 

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY

static t_sm_palette SVG_palette;
static uchar SVG_red = 0;
static uchar SVG_green = 0;
static uchar SVG_blue = 0;
static double SVG_alpha = 0.0;
static uchar SVG_color_mode = TC_DEFAULT;
static char * SVG_linecolor = NULL;
static char * SVG_name = NULL;
static char * SVG_scriptdir = NULL;
static bool SVG_mouseable = FALSE;
static bool SVG_standalone = FALSE;
static bool SVG_domterm = FALSE;
static bool SVG_emit_doctype = TRUE;
static bool SVG_animate = FALSE;

static bool SVG_groupFilledIsOpen = FALSE; /* open pm3d group flag*/
static bool SVG_inTextBox = FALSE;

struct SVG_PEN {
	double width;
	char color[8];
};

static uint SVG_xSize = static_cast<uint>(SVG_XMAX); /* plot horizontal size */
static uint SVG_ySize = static_cast<uint>(SVG_YMAX); /* plot vertical size*/
static bool SVG_fixed_size = TRUE; /* make SVG viewer size fixed */
static uint SVG_xLast = UINT_MAX; /* current pen horizontal position*/
static uint SVG_yLast = UINT_MAX; /* current pen vertical position*/
static int SVG_LineType = LT_NODRAW; /* current line type*/
static double SVG_LineWidth = 1.0; /* current line width*/
static double SVG_linewidth_factor = 1.0; /* Multiplier for linewidths */
static double SVG_dashlength = 1.0; /* Multiplier for dash patterns */
static t_linecap SVG_linecap = BUTT; /* linejoin and linecap */
static int SVG_TextAngle = 0; /* current text orientation*/
static enum JUSTIFY SVG_TextJust = LEFT; /* current text justification*/

/* default text font family: */
static char * SVG_fontNameDef = NULL;
static const char * SVG_fontStyleDef = NULL; /* default font style */
static const char * SVG_fontWeightDef = NULL; /* default font weight */
static double SVG_fontSizeDef = 12; /* default text size*/
/* current text font family: */
static char * SVG_fontNameCur = NULL;
static const char * SVG_fontStyleCur = NULL; /* current font style */
static const char * SVG_fontWeightCur = NULL; /* current font weight */
static double SVG_fontSizeCur = 12; /* current text size*/
static double SVG_fontscale = 1.0; /* multiplier for nominal font size */
static bool SVG_groupIsOpen = FALSE; /* open group flag*/
static bool SVG_pathIsOpen = FALSE; /* open path flag*/
static uint SVG_path_count = 0; /* size of current path*/
static struct SVG_PEN SVG_pens[16]; /* pen descriptors*/
static int SVG_fillPattern = -1; /* active fill pattern (-1 == undefined) */
static uint SVG_fillPatternIndex = 0;
static int SVG_background = -1;
static int SVG_plotno = 0;
static bool SVG_gridline = FALSE;
static bool SVG_hasgrid = FALSE;
static double SVG_fontAscent  = 0; /* estimated current font ascent*/
static double SVG_fontDescent = 0; /* estimated current font descent*/
static double SVG_fontLeading = 0; /* estimated current font leading*/
static double SVG_fontAvWidth = 0; /* estimated current font char average width*/

static short SVG_Pen_RealID(int);
static void SVG_PathOpen();
static void SVG_PathClose();
static void SVG_AddSpaceOrNewline();
static void SVG_GroupOpen();
static void SVG_GroupClose();
static void SVG_GroupFilledOpen();
static void SVG_GroupFilledClose();
static void SVG_StyleColor(const char *);
static void SVG_StyleFillColor();
static void SVG_local_reset(GpTermEntry_Static * pThis);
static void SVG_DefineFillPattern(int fillpat);

/* Stuff for enhanced text mode */
static int ENHsvg_string_state = 0;
static double ENHsvg_x_offset = 0;
static bool ENHsvg_preserve_spaces = FALSE;

/* Support for dashed lines */
#define SVG_dashtypes 5
static const char * SVG_defaultdashpattern[SVG_dashtypes] = { "", " 5,8", " 2,4", " 8,4,2,4", " 9,4,1,4,1,4" };
static const char * SVG_axis_dashpattern = "2,4";
static int SVG_dasharray[SVG_dashtypes][7] = {
	{ 0, 0, 0, 0, 0, 0, 0},
	{5, 8, 0, 0, 0, 0, 0},
	{2, 4, 0, 0, 0, 0, 0},
	{8, 4, 2, 4, 0, 0, 0},
	{9, 4, 1, 4, 1, 4, 0}
};
static const char * SVG_dashpattern = NULL;
static char SVG_custom_dash_pattern[64];

/* Hypertext support */
static double SVG_hypertext_fontSize = 0;
static char * SVG_hypertext_fontName = NULL;
static char * SVG_hypertext_fontStyle = NULL;
static char * SVG_hypertext_fontWeight = NULL;
static char * SVG_hypertext_text = NULL; // Support for embedded hypertext 
// 
// SVG_Pen_RealID
// 
static short SVG_Pen_RealID(int inPenCode)
{
	if(inPenCode >= 13)
		inPenCode %= 13; /* normalize pen code*/
	inPenCode += 3;
	if(inPenCode < 0)
		inPenCode = 0; /* LT_BACKGROUND should use background color */
	return (inPenCode);
}

// 
// SVG_GroupOpen
// 
static void SVG_GroupOpen()
{
	SVG_GroupFilledClose();
	if(!SVG_groupIsOpen) {
		fprintf(GPT.P_GpOutFile, "<g fill=\"none\" color=\"%s\" stroke=\"", SVG_pens[SVG_Pen_RealID(SVG_LineType)].color);
		if(SVG_color_mode == TC_RGB)
			fprintf(GPT.P_GpOutFile, "rgb(%3d, %3d, %3d)", SVG_red, SVG_green, SVG_blue);
		else if(SVG_color_mode == TC_LT)
			fprintf(GPT.P_GpOutFile, "%s", SVG_linecolor);
		else
			fprintf(GPT.P_GpOutFile, "currentColor");
		fprintf(GPT.P_GpOutFile, "\" ");
		fprintf(GPT.P_GpOutFile, "stroke-width=\"%.2f\" stroke-linecap=\"%s\" stroke-linejoin=\"%s\"",
		    SVG_pens[SVG_Pen_RealID(SVG_LineType)].width * SVG_linewidth_factor,
		    SVG_linecap == ROUNDED ? "round" : SVG_linecap == SQUARE ? "square" : "butt",
		    SVG_linecap == ROUNDED ? "round" : "miter");

		fprintf(GPT.P_GpOutFile, ">\n");

		SVG_groupIsOpen = TRUE;
	}
}
// 
// SVG_GroupClose
// 
static void SVG_GroupClose()
{
	SVG_GroupFilledClose();
	if(SVG_groupIsOpen) {
		fputs("</g>\n", GPT.P_GpOutFile);
		SVG_groupIsOpen = FALSE;
		SVG_fillPattern = -1;
	}
}
// 
// SVG_PathOpen
// 
static void SVG_PathOpen()
{
	if(!SVG_pathIsOpen) {
		SVG_GroupFilledClose();
		fputs("\t<path ", GPT.P_GpOutFile);
		/* Line color */
		if(SVG_LineType == LT_NODRAW)
			fprintf(GPT.P_GpOutFile, "stroke='none' ");
		else if(SVG_color_mode == TC_RGB)
			fprintf(GPT.P_GpOutFile, "stroke='rgb(%3d, %3d, %3d)' ",
			    SVG_red, SVG_green, SVG_blue);
		else if(SVG_color_mode == TC_LT)
			fprintf(GPT.P_GpOutFile, "stroke='%s' ", SVG_linecolor);

		/* Axis is always dotted */
		if(SVG_LineType == LT_AXIS)
			fprintf(GPT.P_GpOutFile, "stroke-dasharray='2,4' ");

		/* Other patterns were selected by a previous call to SVG_dashtype */
		else if(SVG_dashpattern)
			fprintf(GPT.P_GpOutFile, "stroke-dasharray='%s' ", SVG_dashpattern);

		/* RGBA */
		if(SVG_alpha != 0.0)
			fprintf(GPT.P_GpOutFile, "opacity='%4.2f' ", 1.0 - SVG_alpha);

		/* Mark grid lines so that we can toggle them on/off */
		if(SVG_gridline)
			fprintf(GPT.P_GpOutFile, "class=\"gridline\" ");

		fputs(" d='", GPT.P_GpOutFile);

		SVG_pathIsOpen = TRUE;
	}
}

/*------------------------------------------------------------------------------------------------------------------------------------
        SVG_PathClose
   ------------------------------------------------------------------------------------------------------------------------------------*/
static void SVG_PathClose()
{
	if(SVG_pathIsOpen) {
		SVG_GroupFilledClose();
		fprintf(GPT.P_GpOutFile, " '/>");
		SVG_path_count = 0;
		SVG_pathIsOpen = FALSE;
	}
}

/*------------------------------------------------------------------------------------------------------------------------------------
        SVG_AddSpaceOrNewline
   ------------------------------------------------------------------------------------------------------------------------------------*/
static void SVG_AddSpaceOrNewline()
{
	if(SVG_path_count % 8 == 0)     /* avoid excessive line length*/
		fputs("\n\t\t", GPT.P_GpOutFile);
	else
		fputs(" ", GPT.P_GpOutFile);
}

static void SVG_SetFont(GpTermEntry_Static * pThis, const char * name, double size)
{
	if(name != SVG_fontNameCur) {
		SAlloc::F(SVG_fontNameCur);
		SVG_fontNameCur = sstrdup(name);
	}
	SVG_fontSizeCur = size;
	// since we cannot interrogate SVG about text properties and according
	// to SVG 1.0 W3C Candidate Recommendation 2 August 2000 the
	// "line-height" of the 'text' element is defined to be equal to the
	// 'font-size' (!), we have to to define font properties in a less
	// than optimal way 
	SVG_fontAscent  = (SVG_fontSizeCur * 0.90 * SVG_SCALE);
	SVG_fontDescent = (SVG_fontSizeCur * 0.25 * SVG_SCALE);
	SVG_fontLeading = (SVG_fontSizeCur * 0.35 * SVG_SCALE);
	SVG_fontAvWidth = (SVG_fontSizeCur * 0.70 * SVG_SCALE);
	pThis->SetCharSize(static_cast<uint>(SVG_fontAvWidth), static_cast<uint>((SVG_fontAscent + SVG_fontDescent + SVG_fontLeading)));
}

static void SVG_GroupFilledOpen()
{
	if(!SVG_groupFilledIsOpen) {
		SVG_PathClose();
		fputs("\t<g stroke='none' shape-rendering='crispEdges'>\n", GPT.P_GpOutFile);
		SVG_groupFilledIsOpen = TRUE;
	}
}

static void SVG_GroupFilledClose()
{
	if(SVG_groupFilledIsOpen) {
		fputs("\t</g>\n", GPT.P_GpOutFile);
		SVG_groupFilledIsOpen = FALSE;
	}
}

static void SVG_StyleColor(const char* paint)
{
	if(SVG_color_mode == TC_RGB)
		fprintf(GPT.P_GpOutFile, "%s = 'rgb(%3d, %3d, %3d)'", paint, SVG_red, SVG_green, SVG_blue);
	else if(SVG_color_mode == TC_LT)
		fprintf(GPT.P_GpOutFile, "%s = '%s'", paint, SVG_linecolor);
	else
		fprintf(GPT.P_GpOutFile, "%s = 'currentColor'", paint);
}

static void SVG_StyleFillColor()
{
	SVG_StyleColor("fill");
}

static void SVG_DefineFillPattern(int fillpat)
{
	const char * path;
	const char * style = "stroke";
	fillpat %= 8;
	if(fillpat != SVG_fillPattern) {
		SVG_fillPattern = fillpat;
		SVG_PathClose();
		SVG_fillPatternIndex++;
		fprintf(GPT.P_GpOutFile, "\t<defs>\n\t\t<pattern id='gpPat%d' patternUnits='userSpaceOnUse' x='0' y='0' width='8' height='8'>\n", SVG_fillPatternIndex);
		switch(fillpat) {
			default:
			case 0:
			    path = "";
			    break;
			case 1:
			    path = "M0,0 L8,8 M0,8 L8,0";
			    break;
			case 2:
			    path = "M0,0 L8,8 M0,8 L8,0 M0,4 L4,8 L8,4 L4,0 L0,4";
			    break;
			case 3:
			    path = "M0,0 L0,8 L8,8 L8,0 L0,0";
			    style = "fill";
			    break;
			case 4:
			    path = "M-4,0 L8,12 M0,-4 L12,8";
			    break;
			case 5:
			    path = "M-4,8 L8,-4 M0,12 L12,0";
			    break;
			case 6:
			    path = "M-2,8 L4,-4 M0,12 L8,-4 M4,12 L10,0";
			    break;
			case 7:
			    path = "M-2,0 L4,12 M0,-4 L8,12 M4,-4 L10,8";
			    break;
		}
		if(*path) {
			const char * figure = "fill:none;";
			if(sstreq(style, "fill")) 
				figure = "stroke:none;";
			if(SVG_color_mode == TC_RGB)
				fprintf(GPT.P_GpOutFile, "\t\t\t<path style='%s %s:rgb(%d,%d,%d)' d='%s'/>\n", figure, style, SVG_red, SVG_green, SVG_blue, path);
			else if(SVG_color_mode == TC_LT)
				fprintf(GPT.P_GpOutFile, "\t\t\t<path style = '%s %s:%s' d= '%s'/>\n", figure, style, SVG_linecolor, path);
			else
				fprintf(GPT.P_GpOutFile, "\t\t\t<path style = '%s %s:currentColor' d='%s'/>\n", figure, style, path);
		}
		fputs("\t\t</pattern>\n" "\t</defs>\n", GPT.P_GpOutFile);
	}
}

static void SVG_MoveForced(GpTermEntry_Static * pThis, uint x, uint y)
{
	if(SVG_path_count > 512)
		SVG_PathClose();
	SVG_PathOpen();
	fprintf(GPT.P_GpOutFile, "M%.*f,%.*f", PREC, X(x), PREC, Y(y));
	SVG_path_count++;
	SVG_AddSpaceOrNewline();
	SVG_xLast = x;
	SVG_yLast = y;
}
// 
// SVG_options
// 
TERM_PUBLIC void SVG_options(GpTermEntry_Static * pThis, GnuPlot * pGp)
{
	// Annoying hack to handle the case of 'set termoption' after 
	// we have already initialized the terminal settings.         
	if(!pGp->Pgm.AlmostEquals(pGp->Pgm.GetPrevTokenIdx(), "termopt$ion"))
		SVG_local_reset(pThis);
	if(strcmp(pThis->name, "domterm") == 0) {
		SVG_emit_doctype = FALSE;
		SVG_domterm = TRUE;
	}
	else {
		SVG_emit_doctype = TRUE;
		SVG_domterm = FALSE;
	}
	// Minimal initialization in case we error out of options parsing 
	SVG_set_font(pThis, "");
	while(!pGp->Pgm.EndOfCommand()) {
		if(pGp->Pgm.AlmostEqualsCur("s$ize")) {
			double value;
			pGp->Pgm.Shift();
			if(pGp->Pgm.EndOfCommand())
				pGp->IntErrorCurToken("expecting x size");
			value = pGp->RealExpression();
			if(value < 2)
				pGp->IntErrorCurToken("x size out of range");
			SVG_xSize = static_cast<uint>(value * SVG_SCALE);
			if(pGp->Pgm.EqualsCur(","))
				pGp->Pgm.Shift();
			if(pGp->Pgm.EndOfCommand())
				pGp->IntErrorCurToken("expecting y size");
			value = pGp->RealExpression();
			if(value < 2)
				pGp->IntErrorCurToken("y size out of range");
			SVG_ySize = static_cast<uint>(value * SVG_SCALE);
			continue;
		}
		if(pGp->Pgm.EqualsCur("mouse") || pGp->Pgm.AlmostEqualsCur("mous$ing")) {
			pGp->Pgm.Shift();
			SVG_mouseable = TRUE;
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("stand$alone")) {
			pGp->Pgm.Shift();
			SVG_standalone = TRUE;
			continue;
		}
		if(pGp->Pgm.EqualsCur("name")) {
			pGp->Pgm.Shift();
			SVG_name = pGp->TryToGetString();
			if(!SVG_name)
				pGp->IntErrorCurToken("expecting a plot name");
			if(SVG_name[strspn(SVG_name, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_1234567890")])
				pGp->IntError(pGp->Pgm.GetPrevTokenIdx(), "name must contain only alphanumerics or _");
			continue;
		}
		if(pGp->Pgm.EqualsCur("jsdir")) {
			pGp->Pgm.Shift();
			SVG_scriptdir = pGp->TryToGetString();
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("d$ynamic")) {
			pGp->Pgm.Shift();
			SVG_fixed_size = FALSE;
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("fi$xed")) {
			pGp->Pgm.Shift();
			SVG_fixed_size = TRUE;
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("enh$anced")) {
			pGp->Pgm.Shift();
			pThis->put_text = ENHsvg_put_text;
			pThis->SetFlag(TERM_ENHANCED_TEXT);
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("noenh$anced")) {
			pGp->Pgm.Shift();
			pThis->put_text = SVG_put_text;
			pThis->ResetFlag(TERM_ENHANCED_TEXT);
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("fn$ame") || pGp->Pgm.AlmostEqualsCur("font")) {
			char * s, * comma;
			pGp->Pgm.Shift();
			if(!(s = pGp->TryToGetString()))
				pGp->IntErrorCurToken("expecting font name");
			comma = strrchr(s, ',');
			if(comma && (1 == sscanf(comma + 1, "%lf", &SVG_fontSizeDef)))
				*comma = '\0';
			if(*s) {
				char * bold, * italic;
				if(!((bold = strstr(s, " bold"))))
					bold = strstr(s, " Bold");
				if(!((italic = strstr(s, " italic"))))
					italic = strstr(s, " Italic");
				SAlloc::F(SVG_fontNameDef);
				SVG_fontNameDef = s;
				if(italic) {
					SVG_fontStyleDef = "italic";
					SVG_fontNameDef[strlen(s) - strlen(italic)] = '\0';
				}
				else {
					SVG_fontStyleDef = "normal";
				}
				if(bold) {
					SVG_fontWeightDef = "bold";
					SVG_fontNameDef[strlen(s) - strlen(bold)] = '\0';
				}
				else {
					SVG_fontWeightDef = "normal";
				}
			}
			else
				SAlloc::F(s);
			continue;
		}
		if(pGp->Pgm.EqualsCur("fontscale")) {
			pGp->Pgm.Shift();
			SVG_fontscale = pGp->RealExpression();
			if(SVG_fontscale <= 0)
				SVG_fontscale = 1.0;
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("linew$idth") || pGp->Pgm.EqualsCur("lw")) {
			pGp->Pgm.Shift();
			SVG_linewidth_factor = pGp->RealExpression();
			if(SVG_linewidth_factor <= 0.0)
				SVG_linewidth_factor = 1.0;
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("dashl$ength") || pGp->Pgm.EqualsCur("dl")) {
			pGp->Pgm.Shift();
			SVG_dashlength = pGp->RealExpression();
			if(SVG_dashlength < 0.5)
				SVG_dashlength = 1.0;
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("round$ed")) {
			pGp->Pgm.Shift();
			SVG_linecap = ROUNDED;
			continue;
		}
		if(pGp->Pgm.EqualsCur("square")) {
			pGp->Pgm.Shift();
			SVG_linecap = SQUARE;
			continue;
		}
		if(pGp->Pgm.EqualsCur("butt")) {
			pGp->Pgm.Shift();
			SVG_linecap = BUTT;
			continue;
		}
		// Not used in version 5 
		if(pGp->Pgm.EqualsCur("solid") || pGp->Pgm.AlmostEqualsCur("dash$ed")) {
			pGp->Pgm.Shift();
			continue;
		}
		if(pGp->Pgm.AlmostEqualsCur("backg$round")) {
			pGp->Pgm.Shift();
			SVG_background = pGp->ParseColorName();
			continue;
		}
		if(SVG_domterm && pGp->Pgm.AlmostEqualsCur("anim$ate")) {
			pGp->Pgm.Shift();
			SVG_animate = TRUE;
			continue;
		}
		pGp->IntErrorCurToken("unrecognized terminal option");
	}
	// I don't think any error checks on font name are possible; just set it 
	SVG_set_font(pThis, "");
	// Save options back into options string in normalized format 
	slprintf(GPT._TermOptions, "size %d,%d%s %s font '%s,%g' ", (int)(SVG_xSize/SVG_SCALE), (int)(SVG_ySize/SVG_SCALE),
	    SVG_fixed_size ? " fixed" : " dynamic", pThis->put_text == ENHsvg_put_text ? "enhanced" : "", SVG_fontNameCur, SVG_fontSizeCur);
	if(SVG_mouseable) {
		slprintf_cat(GPT._TermOptions, "mousing ");
	}
	if(SVG_standalone) {
		slprintf_cat(GPT._TermOptions, "standalone ");
	}
	if(SVG_name) {
		slprintf_cat(GPT._TermOptions, "name \"%s\" ", SVG_name);
	}
	slprintf_cat(GPT._TermOptions, SVG_linecap == ROUNDED ? "rounded " : SVG_linecap == SQUARE ? "square " : "butt ");
	slprintf_cat(GPT._TermOptions, "dashlength %.1f ", SVG_dashlength);
	if(SVG_linewidth_factor != 1.0) {
		slprintf_cat(GPT._TermOptions, "linewidth %3.1f ", SVG_linewidth_factor);
	}
	if(SVG_background >= 0) {
		slprintf_cat(GPT._TermOptions, "background \"#%06x\" ", SVG_background);
	}
	if(SVG_animate) {
		slprintf_cat(GPT._TermOptions, "animate ");
	}
}

static void SVG_local_reset(GpTermEntry_Static * pThis)
{
	SVG_xSize = static_cast<uint>(SVG_XMAX);
	SVG_ySize = static_cast<uint>(SVG_YMAX);
	SVG_fixed_size = true;
	SAlloc::F(SVG_fontNameDef);
	SVG_fontNameDef = sstrdup("Arial");
	SVG_fontSizeDef  = 12;
	SVG_mouseable = false;
	SVG_standalone = false;
	ZFREE(SVG_name);
	ZFREE(SVG_scriptdir);
	SVG_gridline = false;
	SVG_hasgrid = false;
	SVG_animate = false;
	// Default to enhanced text 
	pThis->put_text = ENHsvg_put_text;
	pThis->SetFlag(TERM_ENHANCED_TEXT);
}

TERM_PUBLIC void SVG_init(GpTermEntry_Static * pThis)
{
	/* setup pens*/
	SVG_pens[0].width = SVG_LineWidth;
	strcpy(SVG_pens[0].color, "white"); /* should really be background */
	SVG_pens[1].width = SVG_LineWidth;
	strcpy(SVG_pens[1].color, "black");
	SVG_pens[2].width = SVG_LineWidth;
	strcpy(SVG_pens[2].color, "gray");
	SVG_pens[3].width = SVG_LineWidth;
	strcpy(SVG_pens[3].color, "red");
	SVG_pens[4].width = SVG_LineWidth;
	strcpy(SVG_pens[4].color, "green");
	SVG_pens[5].width = SVG_LineWidth;
	strcpy(SVG_pens[5].color, "blue");
	SVG_pens[6].width = SVG_LineWidth;
	strcpy(SVG_pens[6].color, "cyan");
	SVG_pens[7].width = SVG_LineWidth;
	sprintf(SVG_pens[7].color, "#%2.2X%2.2X%2.2X", 21, 117, 69); /* pine green*/
	SVG_pens[8].width = SVG_LineWidth;
	sprintf(SVG_pens[8].color, "#%2.2X%2.2X%2.2X", 0, 0, 148); /* navy*/
	SVG_pens[9].width = SVG_LineWidth;
	sprintf(SVG_pens[9].color, "#%2.2X%2.2X%2.2X", 255, 153, 0); /* orange*/
	SVG_pens[10].width = SVG_LineWidth;
	sprintf(SVG_pens[10].color, "#%2.2X%2.2X%2.2X", 0, 153, 161); /* green blue*/
	SVG_pens[11].width = SVG_LineWidth;
	sprintf(SVG_pens[11].color, "#%2.2X%2.2X%2.2X", 214, 214, 69); /* olive*/
	SVG_pens[12].width = SVG_LineWidth;
	sprintf(SVG_pens[12].color, "#%2.2X%2.2X%2.2X", 163, 145, 255); /* cornflower*/
	SVG_pens[13].width = SVG_LineWidth;
	sprintf(SVG_pens[13].color, "#%2.2X%2.2X%2.2X", 255, 204, 0); /* gold*/
	SVG_pens[14].width = SVG_LineWidth;
	sprintf(SVG_pens[14].color, "#%2.2X%2.2X%2.2X", 214, 0, 120); /* mulberry*/
	SVG_pens[15].width = SVG_LineWidth;
	sprintf(SVG_pens[15].color, "#%2.2X%2.2X%2.2X", 171, 214, 0); /* green yellow*/

	if(SVG_background >= 0)
		sprintf(SVG_pens[0].color, "#%2.2X%2.2X%2.2X", (SVG_background >> 16)&0xff, (SVG_background >> 8)&0xff, (SVG_background)&0xff);
	SVG_LineType = LT_NODRAW;
	// set xmax, ymax
	pThis->SetMax(SVG_xSize, SVG_ySize);
	// set current font, including ChrH and ChrV 
	SVG_SetFont(pThis, SVG_fontNameCur, SVG_fontSizeCur);
	// set TicH, TicV
	pThis->SetTic(pThis->CV() / 2);
}
//
// write file header
//
static void SVG_write_preamble(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int len;
	double stroke_width;
	const char * svg_encoding = "";
	switch(GPT._Encoding) {
		case S_ENC_ISO8859_1:   svg_encoding = "encoding=\"iso-8859-1\" "; break;
		case S_ENC_ISO8859_2:   svg_encoding = "encoding=\"iso-8859-2\" "; break;
		case S_ENC_ISO8859_9:   svg_encoding = "encoding=\"iso-8859-9\" "; break;
		case S_ENC_ISO8859_15:  svg_encoding = "encoding=\"iso-8859-15\" "; break;
		case S_ENC_CP850:       svg_encoding = "encoding=\"ibm-850\" "; break;
		case S_ENC_CP852:       svg_encoding = "encoding=\"ibm-852\" "; break;
		case S_ENC_CP950:       svg_encoding = "encoding=\"cp950\" "; break;
		case S_ENC_CP1250:      svg_encoding = "encoding=\"windows-1250\" "; break;
		case S_ENC_CP1251:      svg_encoding = "encoding=\"windows-1251\" "; break;
		case S_ENC_CP1252:      svg_encoding = "encoding=\"windows-1252\" "; break;
		case S_ENC_KOI8_R:      svg_encoding = "encoding=\"koi8-r\" "; break;
		case S_ENC_KOI8_U:      svg_encoding = "encoding=\"koi8-u\" "; break;
		case S_ENC_SJIS:        svg_encoding = "encoding=\"Shift_JIS\" "; break;
		case S_ENC_CP437:       svg_encoding = ""; break;
		default: /* UTF-8 */
		    svg_encoding = "encoding=\"utf-8\" ";
		    break;
	}
	if(SVG_domterm) {
		// Erase all off-screen lines in terminal buffer 
		if(SVG_animate)
			fprintf(GPT.P_GpOutFile, "\033[3J");
		fprintf(GPT.P_GpOutFile, "\033]72;");
	}
	if(SVG_emit_doctype)
		fprintf(GPT.P_GpOutFile, "<?xml version=\"1.0\" %s standalone=\"no\"?>\n", svg_encoding);
	fprintf(GPT.P_GpOutFile, "<svg ");
	if(SVG_mouseable)
		fprintf(GPT.P_GpOutFile, " onload=\"if (typeof(gnuplot_svg)!='undefined') gnuplot_svg.Init(evt)\" ");
	if(SVG_fixed_size)
		fprintf(GPT.P_GpOutFile, "\n width=\"%u\" height=\"%u\"", (uint)(pThis->MaxX / SVG_SCALE), (uint)(pThis->MaxY / SVG_SCALE));
	fprintf(GPT.P_GpOutFile, "\n viewBox=\"0 0 %u %u\"\n", (uint)(pThis->MaxX / SVG_SCALE), (uint)(pThis->MaxY / SVG_SCALE));
	fprintf(GPT.P_GpOutFile, " xmlns=\"http://www.w3.org/2000/svg\"\n");
	fprintf(GPT.P_GpOutFile, " xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
#if (0)
	/* This should be required, but Firefox gets it totally wrong */
	fprintf(GPT.P_GpOutFile, " xml:space=\"preserve\"\n");
#endif
	fprintf(GPT.P_GpOutFile, ">\n\n");
	/* TODO: It would be nice to print the actual plot title here */
	fprintf(GPT.P_GpOutFile, "<title>%s</title>\n", SVG_name ? SVG_name : "Gnuplot");
	fprintf(GPT.P_GpOutFile, "<desc>Produced by GNUPLOT %s patchlevel %s </desc>\n\n", gnuplot_version, gnuplot_patchlevel);

/*
 * FIXME: This code could be shared with canvas.trm
 *	  Figure out the full URL to use for xlink:href="gnuplot_svg.js"
 */
	if(SVG_scriptdir == NULL) {
#ifdef GNUPLOT_JS_DIR
#if defined(_WIN32)
		SVG_scriptdir = RelativePathToGnuplot(GNUPLOT_JS_DIR);
#else
		SVG_scriptdir = sstrdup(GNUPLOT_JS_DIR); // use hardcoded _absolute_ path 
#endif
#else
		SVG_scriptdir = sstrdup("");
#endif /* GNUPLOT_JS_DIR */
	}
	len = strlen(SVG_scriptdir);
#if defined(_WIN32)
	if(*SVG_scriptdir && SVG_scriptdir[len-1] != '\\' && SVG_scriptdir[len-1] != '/') {
		SVG_scriptdir = (char *)SAlloc::R(SVG_scriptdir, len+2);
		if(SVG_scriptdir[len-1] == '\\') // use backslash if used in jsdir, otherwise slash 
			strcat(SVG_scriptdir, "\\");
		else
			strcat(SVG_scriptdir, "/");
	}
#else
	if(*SVG_scriptdir && SVG_scriptdir[len-1] != '/') {
		SVG_scriptdir = SAlloc::R(SVG_scriptdir, len+2);
		strcat(SVG_scriptdir, "/");
	}
#endif
	if(SVG_mouseable) {
		/* Inclusion of gnuplot_svg.js is sufficient to support toggling plots on/off */
		if(!SVG_standalone) {
			fprintf(GPT.P_GpOutFile, "<script type=\"text/javascript\" xlink:href=\"%sgnuplot_svg.js\"/>\n", SVG_scriptdir);
		}
		else {
			/* "standalone" option includes the mousing code in the file itself */
			char * fullname = NULL;
			const char * name = "gnuplot_svg.js";
			char buf[256];
			FILE * svg_js_fd;
			fullname = (char *)SAlloc::M(strlen(SVG_scriptdir) + strlen(name) + 4);
			strcpy(fullname, SVG_scriptdir);
			PATH_CONCAT(fullname, name);
			svg_js_fd = fopen(fullname, "r");
			if(!svg_js_fd)
				p_gp->IntWarn(NO_CARET, "Failed to insert javascript file %s\n", fullname);
			else {
				fprintf(GPT.P_GpOutFile, "<script type=\"text/javascript\" > <![CDATA[\n");
				while(fgets(buf, sizeof(buf), svg_js_fd))
					fputs(buf, GPT.P_GpOutFile);
				fprintf(GPT.P_GpOutFile, "]]>\n</script>\n");
				fclose(svg_js_fd);
			}
			SAlloc::F(fullname);
		}
	}
	if(SVG_mouseable) { /* FIXME: Should only do this for 2D plots */
		/* This is extra code to support tracking the mouse coordinates */
		fprintf(GPT.P_GpOutFile, "\n<!-- Tie mousing to entire bounding box of the plot -->\n");
		fprintf(GPT.P_GpOutFile, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"", 0, 0, (int)(pThis->MaxX/SVG_SCALE), (int)(pThis->MaxY/SVG_SCALE));
		fprintf(GPT.P_GpOutFile, " fill=\"#%06x\" stroke=\"black\" stroke-width=\"1\"\n", SVG_background >= 0 ? SVG_background : 0xffffff);
		fprintf(GPT.P_GpOutFile, "onclick=\"gnuplot_svg.toggleCoordBox(evt)\"  onmousemove=\"gnuplot_svg.moveCoordBox(evt)\"/>\n");
		fprintf(GPT.P_GpOutFile, "\n<!-- Also track mouse when it is on a plot element -->\n");
		fprintf(GPT.P_GpOutFile, "<g id=\"gnuplot_canvas\" onclick=\"gnuplot_svg.toggleCoordBox(evt)\" onmousemove=\"gnuplot_svg.moveCoordBox(evt)\">\n\n");
	}
	else {
		fprintf(GPT.P_GpOutFile, "<g id=\"gnuplot_canvas\">\n\n");
		fprintf(GPT.P_GpOutFile, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"", 0, 0, (int)(pThis->MaxX/SVG_SCALE), (int)(pThis->MaxY/SVG_SCALE));
		if(SVG_background >= 0)
			fprintf(GPT.P_GpOutFile, " fill=\"#%06x\"", SVG_background);
		else
			fprintf(GPT.P_GpOutFile, " fill=\"none\"");
		fprintf(GPT.P_GpOutFile, "/>\n");
	}

	/* Start prologue section of output file, and load fonts if requested */

	fprintf(GPT.P_GpOutFile, "<defs>\n");

	/* definitions of point symbols */
	/* FIXME: SVG scales linewidth along with the marker itself, and
	 * there seems to be no way to avoid that without copying the
	 * marker definition into the file, rather than referencing a
	 * defined one :-( That would make for much larger files */
	/* "\t<path id='gpPt3' stroke-width='%.3f' d='M-1,-1 h2 v2 h-2 z'/>\n" */
	stroke_width = 2.0 *SVG_SCALE / pThis->TicH;
	fprintf(GPT.P_GpOutFile,
	    "\n"
	    /* dot: */
	    "\t<circle id='gpDot' r='0.5' stroke-width='0.5' stroke='currentColor'/>\n"
	    /* 0 plus */
	    "\t<path id='gpPt0' stroke-width='%.3f' stroke='currentColor' d='M-1,0 h2 M0,-1 v2'/>\n"
	    /* 1 X */
	    "\t<path id='gpPt1' stroke-width='%.3f' stroke='currentColor' d='M-1,-1 L1,1 M1,-1 L-1,1'/>\n"
	    /* 2 star */
	    "\t<path id='gpPt2' stroke-width='%.3f' stroke='currentColor' d='M-1,0 L1,0 M0,-1 L0,1 M-1,-1 L1,1 M-1,1 L1,-1'/>\n"
	    /* 3 box */
	    "\t<rect id='gpPt3' stroke-width='%.3f' stroke='currentColor' x='-1' y='-1' width='2' height='2'/>\n"
	    /* 4 box                   filled */
	    "\t<rect id='gpPt4' stroke-width='%.3f' stroke='currentColor' fill='currentColor' x='-1' y='-1' width='2' height='2'/>\n"
	    /* 5 circle */
	    "\t<circle id='gpPt5' stroke-width='%.3f' stroke='currentColor' cx='0' cy='0' r='1'/>\n"
	    /* 6 circle (disk)         filled */
	    "\t<use xlink:href='#gpPt5' id='gpPt6' fill='currentColor' stroke='none'/>\n"
	    /* 7 triangle */
	    "\t<path id='gpPt7' stroke-width='%.3f' stroke='currentColor' d='M0,-1.33 L-1.33,0.67 L1.33,0.67 z'/>\n"
	    /* 8 triangle  filled */
	    "\t<use xlink:href='#gpPt7' id='gpPt8' fill='currentColor' stroke='none'/>\n"
	    /* 9 upside down triangle */
	    "\t<use xlink:href='#gpPt7' id='gpPt9' stroke='currentColor' transform='rotate(180)'/>\n"
	    /* 10 upside down triangle filled */
	    "\t<use xlink:href='#gpPt9' id='gpPt10' fill='currentColor' stroke='none'/>\n"
	    /* 11 diamond */
	    "\t<use xlink:href='#gpPt3' id='gpPt11' stroke='currentColor' transform='rotate(45)'/>\n"
	    /* 12 diamond               filled */
	    "\t<use xlink:href='#gpPt11' id='gpPt12' fill='currentColor' stroke='none'/>\n"
	    /* 13 pentagon */
	    "\t<path id='gpPt13' stroke-width='%.3f' stroke='currentColor' d='M0,1.330 L1.265,0.411 L0.782,-1.067 L-0.782,-1.076 L-1.265,0.411 z'/>\n"
	    /* 14 pentagon              filled */
	    "\t<use xlink:href='#gpPt13' id='gpPt14' fill='currentColor' stroke='none'/>\n"

	    /* NOTE: Fill patterns must be defined after the stroke color has been
	     * set to use the correct (current) stroke color. Therefore we can't
	     * define fill patterns here. */
	    "\t<filter id='textbox' filterUnits='objectBoundingBox' x='0' y='0' height='1' width='1'>\n"
	    "\t  <feFlood flood-color='%s' flood-opacity='1' result='bgnd'/>\n"
	    "\t  <feComposite in='SourceGraphic' in2='bgnd' operator='atop'/>\n"
	    "\t</filter>\n"

	    "\t<filter id='greybox' filterUnits='objectBoundingBox' x='0' y='0' height='1' width='1'>\n"
	    "\t  <feFlood flood-color='lightgrey' flood-opacity='1' result='grey'/>\n"
	    "\t  <feComposite in='SourceGraphic' in2='grey' operator='atop'/>\n"
	    "\t</filter>\n"

	    "</defs>\n", stroke_width, stroke_width, stroke_width, stroke_width, stroke_width, stroke_width, stroke_width, stroke_width, SVG_pens[0].color);
}

TERM_PUBLIC void SVG_graphics(GpTermEntry_Static * pThis)
{
	SVG_write_preamble(pThis);
	// We must force a new group with fill:none in order for multiple 
	// plots per page to work. Otherwise new plots are black-filled   
	SVG_GroupOpen();
	SVG_fillPattern = -1;
	SVG_fillPatternIndex = 0;
	SVG_groupFilledIsOpen = FALSE;
	SVG_color_mode = TC_DEFAULT;
	SVG_pathIsOpen = FALSE;
	// reset position
	SVG_xLast = SVG_yLast = UINT_MAX;
}

static void svg_mouse_param(GpTermEntry_Static * pThis, const char * gp_name, const char * js_name)
{
	GnuPlot * p_gp = pThis->P_Gp;
	udvt_entry * udv;
	if((udv = p_gp->Ev.AddUdvByName(gp_name))) {
		if(udv->udv_value.Type == INTGR) {
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.%s = ", js_name);
			fprintf(GPT.P_GpOutFile, PLD, udv->udv_value.v.int_val);
			fprintf(GPT.P_GpOutFile, "\n");
		}
		else if(udv->udv_value.Type == CMPLX) {
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.%s = %g;\n", js_name, udv->udv_value.v.cmplx_val.real);
		}
	}
}

TERM_PUBLIC void SVG_text(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	SVG_PathClose();
	SVG_GroupClose();
	if(SVG_mouseable) {
		GpAxis * this_axis;
		fprintf(GPT.P_GpOutFile, "\n<script type=\"text/javascript\"><![CDATA[\n");
		fprintf(GPT.P_GpOutFile, "// plot boundaries and axis scaling information for mousing \n");
		fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_term_xmax = %d;\n", (int)(pThis->MaxX / SVG_SCALE));
		fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_term_ymax = %d;\n", (int)(pThis->MaxY / SVG_SCALE));
		fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_xmin = %.1f;\n", (double)p_gp->V.BbPlot.xleft / SVG_SCALE);
		fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_xmax = %.1f;\n", (double)p_gp->V.BbPlot.xright / SVG_SCALE);
		fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_ybot = %.1f;\n", (double)(pThis->MaxY-p_gp->V.BbPlot.ybot) / SVG_SCALE);
		fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_ytop = %.1f;\n", (double)(pThis->MaxY-p_gp->V.BbPlot.ytop) / SVG_SCALE);
		fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_width = %.1f;\n", (double)(p_gp->V.BbPlot.xright - p_gp->V.BbPlot.xleft) / SVG_SCALE);
		fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_height = %.1f;\n", (double)(p_gp->V.BbPlot.ytop - p_gp->V.BbPlot.ybot) / SVG_SCALE);
		// Get true axis ranges as used in the plot 
		p_gp->UpdateGpvalVariables(pThis, 1);
#define MOUSE_PARAM(GP_NAME, js_NAME) svg_mouse_param(pThis, GP_NAME, js_NAME)
		if(p_gp->AxS[FIRST_X_AXIS].datatype != DT_TIMEDATE) {
			MOUSE_PARAM("GPVAL_X_MIN", "plot_axis_xmin");
			MOUSE_PARAM("GPVAL_X_MAX", "plot_axis_xmax");
		}
		// FIXME: Should this inversion be done at a higher level? 
		if(p_gp->Gg.Is3DPlot && p_gp->_3DBlk.splot_map) {
			MOUSE_PARAM("GPVAL_Y_MAX", "plot_axis_ymin");
			MOUSE_PARAM("GPVAL_Y_MIN", "plot_axis_ymax");
		}
		else {
			MOUSE_PARAM("GPVAL_Y_MIN", "plot_axis_ymin");
			MOUSE_PARAM("GPVAL_Y_MAX", "plot_axis_ymax");
		}
		fprintf(GPT.P_GpOutFile, "gnuplot_svg.polar_mode = %s;\n", p_gp->Gg.Polar ? "true" : "false");
		if(p_gp->Gg.Polar) {
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_axis_rmin = %g;\n", (p_gp->AxS.__R().autoscale & AUTOSCALE_MIN) ? 0.0 : p_gp->AxS.__R().set_min);
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_axis_rmax = %g;\n", p_gp->AxS.__R().set_max);
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.polar_theta0 = %d;\n", (int)p_gp->AxS.ThetaOrigin);
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.polar_sense = %d;\n", (int)p_gp->AxS.ThetaDirection);
		}
		if((p_gp->AxS[SECOND_X_AXIS].ticmode & TICS_MASK) != NO_TICS) {
			MOUSE_PARAM("GPVAL_X2_MIN", "plot_axis_x2min");
			MOUSE_PARAM("GPVAL_X2_MAX", "plot_axis_x2max");
		}
		else
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_axis_x2min = \"none\";\n");
		if((p_gp->AxS[SECOND_Y_AXIS].ticmode & TICS_MASK) != NO_TICS) {
			MOUSE_PARAM("GPVAL_Y2_MIN", "plot_axis_y2min");
			MOUSE_PARAM("GPVAL_Y2_MAX", "plot_axis_y2max");
		}
		else
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_axis_y2min = \"none\";\n");
#undef MOUSE_PARAM
		/*
		 * Note:
		 * Offline mousing cannot automatically deal with
		 * (1) nonlinear axes other than logscale
		 * (2) [x,y]->plot_coordinates as specified by mouse_mode 8
		 *		'set mouse mouseformat function <foo>'
		 *     Both of these states are noted by gnuplot_svg.plot_logaxis_* < 0
		 *     Linked axes that happen to be nonlinear are incorrectly treated as linear
		 * (3) generic user-specified coordinate format (mouse_alt_string)
		 *        'set mouse mouseformat "foo"'
		 *     Special case mouse_alt_string formats recognized by gnuplot_svg.js are
		 *     "Time", "Date", and "DateTime".
		 * FIXME: This all needs to be documented somewhere!
		 */
		#define is_nonlinear(axis) ((axis)->linked_to_primary && (axis)->link_udf->at && (axis)->index == -((axis)->linked_to_primary->index))

		this_axis = &p_gp->AxS[FIRST_X_AXIS];
		fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_logaxis_x = %d;\n", this_axis->log ? 1 : (mouse_mode == MOUSE_COORDINATES_FUNCTION || is_nonlinear(this_axis)) ? -1 : 0);
		this_axis = &p_gp->AxS[FIRST_Y_AXIS];
		fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_logaxis_y = %d;\n", this_axis->log ? 1 : (mouse_mode == MOUSE_COORDINATES_FUNCTION || is_nonlinear(this_axis)) ? -1 : 0);
		if(p_gp->Gg.Polar)
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_logaxis_r = %d;\n", p_gp->AxS[POLAR_AXIS].log ? 1 : 0);
		if(p_gp->AxS[FIRST_X_AXIS].datatype == DT_TIMEDATE) {
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_axis_xmin = %.3f;\n", p_gp->AxS[FIRST_X_AXIS].min);
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_axis_xmax = %.3f;\n", p_gp->AxS[FIRST_X_AXIS].max);
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_timeaxis_x = \"%s\";\n", (mouse_alt_string) ? mouse_alt_string : (mouse_mode == 4) ? "Date" : (mouse_mode == 5) ? "Time" : "DateTime");
		}
		else if(p_gp->AxS[FIRST_X_AXIS].datatype == DT_DMS) {
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_timeaxis_x = \"DMS\";\n");
		}
		else
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_timeaxis_x = \"\";\n");
		if(p_gp->AxS[FIRST_Y_AXIS].datatype == DT_DMS)
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_timeaxis_y = \"DMS\";\n");
		else
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.plot_timeaxis_y = \"\";\n");

		/* Hypertext font properties
		 * NB: These will apply to all hypertext in the plot
		 *     separate font for individual labels would require additional code
		 */
		fprintf(GPT.P_GpOutFile, "gnuplot_svg.hypertext_fontSize = %.1g;\n", SVG_hypertext_fontSize);
		if(SVG_hypertext_fontName)
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.hypertext_fontName = \"%s\";\n", SVG_hypertext_fontName);
		else
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.hypertext_fontName = null;\n");
		if(SVG_hypertext_fontStyle)
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.hypertext_fontStyle = \"%s\";\n", SVG_hypertext_fontStyle);
		else
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.hypertext_fontStyle = null;\n");
		if(SVG_hypertext_fontWeight)
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.hypertext_fontWeight = \"%s\";\n", SVG_hypertext_fontWeight);
		else
			fprintf(GPT.P_GpOutFile, "gnuplot_svg.hypertext_fontWeight = null;\n");

		fprintf(GPT.P_GpOutFile, "]]>\n</script>\n");
	} /* End of section writing out variables for mousing */

	/* Close off the group with id=gnuplot_canvas that wraps the entire plot */
	fprintf(GPT.P_GpOutFile, "</g>\n");

	/* Now create a text element to hold the mouse-tracking text. */
	/* It comes _after_ the plot group so that it floats on top.  */
	if(SVG_mouseable) {
		fprintf(GPT.P_GpOutFile, "\n  <text id=\"coord_text\" text-anchor=\"start\" pointer-events=\"none\"\n");
		fprintf(GPT.P_GpOutFile, "  font-size=\"12\" font-family=\"Arial\"\n");
		fprintf(GPT.P_GpOutFile, "  visibility=\"hidden\"> </text>\n");
	}

	/* Add a box and a text element to hold mouseover hypertext */
	if(SVG_mouseable) {
		fprintf(GPT.P_GpOutFile, "\n  <rect id=\"hypertextbox\" class=\"hypertextbox\" pointer-events=\"none\"\n");
		fprintf(GPT.P_GpOutFile, "  fill=\"white\" stroke=\"black\" opacity=\"0.8\"\n");
		fprintf(GPT.P_GpOutFile, "  height=\"16\" visibility=\"hidden\" />\n");
		fprintf(GPT.P_GpOutFile, "\n  <text id=\"hypertext\" class=\"hypertext\" pointer-events=\"none\"\n");
		fprintf(GPT.P_GpOutFile, "  font-size=\"12\" font-family=\"Arial\"\n");
		fprintf(GPT.P_GpOutFile, "  visibility=\"hidden\"> </text>\n");
	}

	/* Add a placeholder for an image linked to mouseover hypertext */
	if(SVG_mouseable) {
		fprintf(GPT.P_GpOutFile, "\n  <image id=\"hyperimage\" class=\"hyperimage\" pointer-events=\"none\"\n");
		fprintf(GPT.P_GpOutFile, "  fill=\"white\" stroke=\"black\" opacity=\"0.8\"\n");
		fprintf(GPT.P_GpOutFile, "  height=\"200\" width=\"300\" visibility=\"hidden\" />\n");
	}

	/* If there were any grid lines in this plot, add a button to toggle them */
	if(SVG_mouseable && SVG_hasgrid) {
		fprintf(GPT.P_GpOutFile, "\n  <image x='10' y='%d' width='16' height='16' ", (int)(pThis->MaxY/SVG_SCALE)-26);
		fprintf(GPT.P_GpOutFile,
		    "\n    xlink:href='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAABmJLR0QA/wD/AP+gvaeTAAAAM0lEQVQokWP8//8/AymACc5iZGQkyEDRQCwgyUn///9nhGtgZISy8TBGnTSCnMRIavIGAGPTWfVV7DcfAAAAAElFTkSuQmCC'");
		fprintf(GPT.P_GpOutFile, "\n    onclick='gnuplot_svg.toggleGrid();'/>\n");
	}

	fputs("</svg>\n\n", GPT.P_GpOutFile);

	if(SVG_domterm) {
		fprintf(GPT.P_GpOutFile, "\007");
		fflush(GPT.P_GpOutFile);
	}
}

TERM_PUBLIC void SVG_reset(GpTermEntry_Static * pThis)
{
}

TERM_PUBLIC void SVG_linetype(GpTermEntry_Static * pThis, int linetype)
{
	SVG_color_mode = TC_DEFAULT;
	if(TRUE || linetype != SVG_LineType) {
		SVG_PathClose();
		SVG_GroupClose();
		SVG_LineType = linetype;
		SVG_GroupOpen();
	}
	if(linetype == LT_AXIS)
		SVG_dashpattern = SVG_axis_dashpattern;
	if(linetype == LT_SOLID)
		SVG_dashpattern = NULL;
}

TERM_PUBLIC void SVG_dashtype(GpTermEntry_Static * pThis, int type, t_dashtype * custom_dash_type)
{
	int d, j;
	double empirical_scale = 0.50;
	// The dash pattern should depend on the `linewidth` and the terminal options `dashlength` and `linewidth`. 
	double dash_scale = SVG_pens[SVG_Pen_RealID(SVG_LineType)].width * SVG_linewidth_factor * SVG_dashlength * empirical_scale;
	SVG_dashpattern = NULL; /* Assume solid line */
	switch(type) {
		case DASHTYPE_SOLID:
		    break;
		case DASHTYPE_AXIS:
		    // Currently handled elsewhere via LT_AXIS 
		    break;
		case DASHTYPE_CUSTOM:
		    if(custom_dash_type) {
				// @v11.2.11 {
				char * p_new_pattern = SVG_custom_dash_pattern;
			    *p_new_pattern = '\0';
			    for(j = 0; j < 8 && custom_dash_type->pattern[j] > 0; j++) {
				    char * p = &p_new_pattern[strlen(SVG_dashpattern)];
				    snprintf(p, 8, "%.1f", custom_dash_type->pattern[j] * dash_scale);
				    if(j < 7 && custom_dash_type->pattern[j+1])
					    strcat(p, ",");
			    }
				SVG_dashpattern = p_new_pattern;
				// } @v11.2.11 
			    /* @v11.2.11
				SVG_dashpattern = SVG_custom_dash_pattern;
			    *SVG_dashpattern = '\0';
			    for(j = 0; j < 8 && custom_dash_type->pattern[j] > 0; j++) {
				    char * p = &SVG_dashpattern[strlen(SVG_dashpattern)];
				    snprintf(p, 8, "%.1f", custom_dash_type->pattern[j] * dash_scale);
				    if(j < 7 && custom_dash_type->pattern[j+1])
					    strcat(p, ",");
			    } */
		    }
		    break;
		default:
		    /* Fall back to whatever version 4 would have provided */
		    d = type % SVG_dashtypes;
		    if(d <= 0)
			    break;
		    if(dash_scale == 1.0) // Default dash length and sequence 
			    SVG_dashpattern = SVG_defaultdashpattern[d];
		    else { // Dash patterns scaled up by dashlength and linewidth 
				// @v11.2.11 {
				char * p_new_pattern = SVG_custom_dash_pattern;
			    *p_new_pattern = '\0';
			    j = 0;
			    do {
				    char * p = &p_new_pattern[strlen(SVG_dashpattern)];
				    snprintf(p, 8, "%.1f", SVG_dasharray[d][j] * dash_scale);
				    if(SVG_dasharray[d][++j])
					    strcat(p, ",");
			    } while(SVG_dasharray[d][j] > 0);
				SVG_dashpattern = p_new_pattern;
				// } @v11.2.11 
				/* @v11.2.11
			    SVG_dashpattern = SVG_custom_dash_pattern;
			    *SVG_dashpattern = '\0';
			    j = 0;
			    do {
				    char * p = &SVG_dashpattern[strlen(SVG_dashpattern)];
				    snprintf(p, 8, "%.1f", SVG_dasharray[d][j] * dash_scale);
				    if(SVG_dasharray[d][++j])
					    strcat(p, ",");
			    } while(SVG_dasharray[d][j] > 0);
				*/
		    }
		    break;
	}
}

TERM_PUBLIC void SVG_fillbox(GpTermEntry_Static * pThis, int style, uint x1, uint y1, uint width, uint height)
{
	gpiPoint corner[4];
	corner[0].x = x1;        corner[0].y = y1;
	corner[1].x = x1+width;  corner[1].y = y1;
	corner[2].x = x1+width;  corner[2].y = y1+height;
	corner[3].x = x1;        corner[3].y = y1+height;
	corner->style = style;
	SVG_filled_polygon(pThis, 4, corner);
}

TERM_PUBLIC void SVG_linewidth(GpTermEntry_Static * pThis, double linewidth)
{
	if(linewidth != SVG_LineWidth) {
		short k;
		SVG_LineWidth = linewidth;
		for(k = 0; k < 16; k++)
			SVG_pens[k].width = SVG_LineWidth;
		SVG_PathClose();
		SVG_GroupClose();
		SVG_GroupOpen();
	}
}

TERM_PUBLIC void SVG_move(GpTermEntry_Static * pThis, uint x, uint y)
{
	if(x != SVG_xLast || y != SVG_yLast) {
		SVG_MoveForced(pThis, x, y);
	}
}

TERM_PUBLIC void SVG_vector(GpTermEntry_Static * pThis, uint x, uint y)
{
	if(x != SVG_xLast || y != SVG_yLast) {
		if(!SVG_pathIsOpen) {
			// The SVG 'path' MUST have a 'moveto' as first command. 
			SVG_MoveForced(pThis, SVG_xLast, SVG_yLast);
		}
		fprintf(GPT.P_GpOutFile, "L%.*f,%.*f", PREC, X(x), PREC, Y(y));
		SVG_path_count++;
		SVG_AddSpaceOrNewline();
		SVG_xLast = x;
		SVG_yLast = y;
	}
}
// 
// SVG_point
// 
TERM_PUBLIC void SVG_point(GpTermEntry_Static * pThis, uint x, uint y, int number)
{
	GnuPlot * p_gp = pThis->P_Gp;
	char color_spec[0x40];
	if(SVG_color_mode == TC_RGB) {
		sprintf(color_spec, " color='rgb(%3d, %3d, %3d)'", SVG_red, SVG_green, SVG_blue);
		if(SVG_alpha != 0.0)
			sprintf(&color_spec[27], " opacity='%4.2f'", 1.0 - SVG_alpha);
	}
	else if(SVG_color_mode == TC_LT)
		sprintf(color_spec, " color='%s'", SVG_linecolor);
	else
		*color_spec = '\0';
	SVG_PathClose();
	if(SVG_hypertext_text) {
		fprintf(GPT.P_GpOutFile, "\t<g onmousemove=\"gnuplot_svg.showHypertext(evt,'%s')\" onmouseout=\"gnuplot_svg.hideHypertext()\"><title> </title>\n",
		    SVG_hypertext_text);
	}
	if(number < 0) {        /* do dot */
		fprintf(GPT.P_GpOutFile, "\t<use xlink:href='#gpDot' x='%.*f' y='%.*f'%s/>\n", PREC, X(x), PREC, Y(y), color_spec);
	}
	else {                  /* draw a point symbol */
		fprintf(GPT.P_GpOutFile, "\t<use xlink:href='#gpPt%u' transform='translate(%.*f,%.*f) scale(%.2f)'%s/>",
		    number % 15, PREC, X(x), PREC, Y(y), p_gp->TermPointSize * pThis->TicH / (2 * SVG_SCALE), color_spec);
	}
	SVG_xLast = x;
	SVG_yLast = y;
	if(SVG_hypertext_text) {
		fprintf(GPT.P_GpOutFile, "</g>\n");
		ZFREE(SVG_hypertext_text);
	}
	else {
		fprintf(GPT.P_GpOutFile, "\n");
	}
}

TERM_PUBLIC int SVG_justify_text(GpTermEntry_Static * pThis, enum JUSTIFY mode)
{
	SVG_TextJust = mode;
	return TRUE;
}

TERM_PUBLIC int SVG_text_angle(GpTermEntry_Static * pThis, int ang)
{
	// Can only do pure horizontal or vertical 
	SVG_TextAngle = ang;
	return TRUE;
}

TERM_PUBLIC void SVG_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str)
{
	const char * alignment;
	double vertical_offset;
	int h = x, v = y;
	SVG_PathClose();
	// horizontal justification
	switch(SVG_TextJust) {
		case LEFT: alignment = "start"; break;
		case CENTRE: alignment = "middle"; break;
		case RIGHT:
		default: // can't happen, just to make gcc happy 
		    alignment = "end";
		    break;
	}
	// vertical justification
	vertical_offset = (SVG_fontAscent - SVG_fontDescent) / 2.0;
	h += vertical_offset * sin(SVG_TextAngle * SMathConst::PiDiv180);
	v -= vertical_offset * cos(SVG_TextAngle * SMathConst::PiDiv180);

/* define text position and attributes */

	fprintf(GPT.P_GpOutFile, "\t<g transform=\"translate(%.*f,%.*f)", PREC, X(h), PREC, Y(v));
	if(SVG_TextAngle)
		fprintf(GPT.P_GpOutFile, " rotate(%i)", -SVG_TextAngle);
	fprintf(GPT.P_GpOutFile, "\" stroke=\"none\" fill=\"");

	if(SVG_color_mode == TC_RGB)
		fprintf(GPT.P_GpOutFile, "rgb(%d,%d,%d)", SVG_red, SVG_green, SVG_blue);
	else if(SVG_color_mode == TC_LT)
		fprintf(GPT.P_GpOutFile, "%s", SVG_linecolor);
	else
		fprintf(GPT.P_GpOutFile, "%s", SVG_pens[SVG_Pen_RealID(SVG_LineType)].color);
	fprintf(GPT.P_GpOutFile, "\" font-family=\"%s\" font-size=\"%.2f\" ",
	    SVG_fontNameCur, SVG_fontSizeCur * SVG_fontscale);
	if(SVG_fontWeightCur && strcmp(SVG_fontWeightCur, "normal"))
		fprintf(GPT.P_GpOutFile, " font-weight=\"%s\" ", SVG_fontWeightCur);
	if(SVG_fontStyleCur && strcmp(SVG_fontStyleCur, "normal"))
		fprintf(GPT.P_GpOutFile, " font-style=\"%s\" ", SVG_fontStyleCur);
	fprintf(GPT.P_GpOutFile, " text-anchor=\"%s\"", alignment);
	if(SVG_inTextBox)
		fprintf(GPT.P_GpOutFile, " style='filter:url(#textbox)'");
	fprintf(GPT.P_GpOutFile, ">\n");
	// output text (unless the enhanced_text processing is in action) 
	if(strstr(str, "  "))
		fputs("\t\t<text xml:space=\"preserve\">", GPT.P_GpOutFile);
	else
		fputs("\t\t<text>", GPT.P_GpOutFile);
	if(!ENHsvg_string_state) {
		while(*str) {
			/* Escape SVG reserved characters */
			switch(*str) {
				case '<':
				    fputs("&lt;", GPT.P_GpOutFile);
				    break;
				case '&':
				    if(str[1] == '#' && str[2] == 'x')
					    fputc(*str, GPT.P_GpOutFile);
				    else
					    fputs("&amp;", GPT.P_GpOutFile);
				    break;
				default:
				    fputc(*str, GPT.P_GpOutFile);
				    break;
			}

			str++;
		}
		fputs("</text>\n\t</g>\n", GPT.P_GpOutFile);
	}
}

TERM_PUBLIC int SVG_set_font(GpTermEntry_Static * pThis, const char * font)
{
	if(!font || !(*font)) {
		SAlloc::F(SVG_fontNameCur);
		SVG_fontNameCur = sstrdup(SVG_fontNameDef);
		SVG_fontSizeCur = SVG_fontSizeDef;
		SVG_fontStyleCur = SVG_fontStyleDef;
		SVG_fontWeightCur = SVG_fontWeightDef;
	}
	else {
		const char * bold;
		const char * italic;
		int sep;
		if(!((bold = strstr(font, " bold"))))
			bold = strstr(font, " Bold");
		if(!((italic = strstr(font, " italic"))))
			italic = strstr(font, " Italic");
		sep = strcspn(font, ",");
		if(sep > 0) {
			SAlloc::F(SVG_fontNameCur);
			SVG_fontNameCur = sstrdup(font);
			if(italic) {
				SVG_fontStyleCur = "italic";
				SVG_fontNameCur[strlen(font) - strlen(italic)] = '\0';
			}
			else {
				SVG_fontStyleCur = "normal";
			}
			if(bold) {
				SVG_fontWeightCur = "bold";
				SVG_fontNameCur[strlen(font) - strlen(bold)] = '\0';
			}
			else {
				SVG_fontWeightCur = "normal";
			}
			SVG_fontNameCur[sep] = '\0';
		}
		if(font[sep] == ',')
			sscanf(font + sep + 1, "%lf", &SVG_fontSizeCur);
	}
	// Set other font properties 
	SVG_SetFont(pThis, SVG_fontNameCur, SVG_fontSizeCur);
	return TRUE;
}

TERM_PUBLIC int SVG_make_palette(GpTermEntry_Static * pThis, t_sm_palette * palette)
{
	SVG_GroupFilledClose();
	if(palette) { // svg can do continuous colors 
		// save mapping formulae needed if SMPAL_COLOR_MODE_RGB 
		SVG_palette.colorMode = palette->colorMode;
		SVG_palette.formulaR = palette->formulaR;
		SVG_palette.formulaG = palette->formulaG;
		SVG_palette.formulaB = palette->formulaB;
		SVG_palette.Positive = palette->Positive;
	}
	return 0;
}

TERM_PUBLIC void SVG_set_color(GpTermEntry_Static * pThis, const t_colorspec * colorspec)
{
	GnuPlot * p_gp = pThis->P_Gp;
	rgb255_color rgb255;
	SVG_alpha = 0.0;
	if(colorspec->type == TC_LT) {
		if(SVG_linecolor != SVG_pens[SVG_Pen_RealID(colorspec->lt)].color) {
			SVG_linecolor = SVG_pens[SVG_Pen_RealID(colorspec->lt)].color;
			SVG_PathClose();
		}
		SVG_color_mode = TC_LT;
		return;
	}
	else if(colorspec->type == TC_FRAC) {
		p_gp->Rgb255MaxColorsFromGray(colorspec->value, &rgb255);
	}
	else if(colorspec->type == TC_RGB) {
		rgb255.r = colorspec->lt >> 16 & 0xff;
		rgb255.g = colorspec->lt >> 8 & 0xff;
		rgb255.b = colorspec->lt & 0xff;
		SVG_alpha = (double)(colorspec->lt >> 24 & 0xff) / 255.0;
	}
	else {
		return;
	}
	SVG_color_mode = TC_RGB;
	if(rgb255.r != SVG_red || rgb255.g != SVG_green || rgb255.b != SVG_blue) {
		/* pm3d color has changed. We must start a new path
		 * with a different line color. This is necessary when
		 * using "linetype palette". */
		SVG_PathClose();
		SVG_red = rgb255.r;
		SVG_green = rgb255.g;
		SVG_blue = rgb255.b;
	}
	return;
}

TERM_PUBLIC void SVG_previous_palette(GpTermEntry_Static * pThis)
{
	SVG_GroupFilledClose();
}

TERM_PUBLIC void SVG_filled_polygon(GpTermEntry_Static * pThis, int points, gpiPoint* corners)
{
	int i;
	int fillpar = corners->style >> 4;
	int style = corners->style &= 0xf;
	if(style == FS_PATTERN || style == FS_TRANSPARENT_PATTERN) {
		/* make sure the pattern is defined (with the current stroke color)
		 * must be defined AFTER the current group is opened with the color
		 * attribute set, as the patterns use 'currentColor' */
		SVG_DefineFillPattern(fillpar);
	}
	SVG_GroupFilledOpen();
	fputs("\t\t<polygon ", GPT.P_GpOutFile);
	switch(style) {
		case FS_EMPTY: /* fill with background color */
		    fprintf(GPT.P_GpOutFile, " fill = '%s'", SVG_pens[0].color);
		    break;
		case FS_SOLID: /* solid fill */
		case FS_TRANSPARENT_SOLID:
		    SVG_StyleFillColor();
		    if(SVG_alpha != 0.0)
			    fprintf(GPT.P_GpOutFile, " fill-opacity='%4.2f' ", 1.0 - SVG_alpha);
		    else if(fillpar >= 0 && fillpar < 100)
			    fprintf(GPT.P_GpOutFile, " fill-opacity = '%f'", fillpar * 0.01);
		    break;
		case FS_PATTERN: /* pattern fill */
		case FS_TRANSPARENT_PATTERN:
		    fprintf(GPT.P_GpOutFile, " fill = 'url(#gpPat%d)'",
			SVG_fillPatternIndex);
		    break;
		default:
		    SVG_StyleFillColor();
		    break;
	}
	fputs(" points = '", GPT.P_GpOutFile);
	for(i = 0; i < points; i++)
		fprintf(GPT.P_GpOutFile, "%.*f,%.*f%s", PREC, X(corners[i].x), PREC, Y(corners[i].y), i % 16 == 15 ? "\n" : " ");
	fputs("'/>\n", GPT.P_GpOutFile);
}

TERM_PUBLIC void SVG_layer(GpTermEntry_Static * pThis, t_termlayer syncpoint)
{
	GnuPlot * p_gp = pThis->P_Gp;
	const char * name = NULL;
	char panel[2] = {'\0', '\0'};
	// We must ignore all syncpoints that we don't recognize 
	switch(syncpoint) {
		default:
		    break;
		case TERM_LAYER_BEFORE_PLOT:
		    SVG_PathClose();
		    SVG_GroupClose();
		    ++SVG_plotno;
		    name = (SVG_name) ? SVG_name : "gnuplot";
		    if(GPT.Flags & GpTerminalBlock::fMultiplot && p_gp->GetMultiplotCurrentPanel() < 26)
			    panel[0] = 'a' + p_gp->GetMultiplotCurrentPanel();
		    fprintf(GPT.P_GpOutFile, "\t<g id=\"%s_plot_%d%s\" ", name, SVG_plotno, panel);
		    if(SVG_hypertext_text && *SVG_hypertext_text)
			    fprintf(GPT.P_GpOutFile, "><title>%s</title>\n", SVG_hypertext_text);
		    else
			    fprintf(GPT.P_GpOutFile, "><title>%s_plot_%d%s</title>\n", name, SVG_plotno, panel);
		    ZFREE(SVG_hypertext_text);
		    SVG_LineType = LT_UNDEFINED; /* Force a new group on next stroke */
		    break;
		case TERM_LAYER_AFTER_PLOT:
		    SVG_PathClose();
		    SVG_GroupClose();
		    fprintf(GPT.P_GpOutFile, "\t</g>\n");
		    SVG_LineType = LT_UNDEFINED; /* Force a new group on next stroke */
		    break;
		case TERM_LAYER_BEGIN_GRID:
		    SVG_gridline = TRUE;
		    SVG_hasgrid = TRUE;
		    break;
		case TERM_LAYER_END_GRID:
		    SVG_gridline = FALSE;
		    break;
		case TERM_LAYER_BEGIN_KEYSAMPLE:
		    if(SVG_mouseable) {
			    SVG_PathClose();
			    SVG_GroupFilledClose();
			    name = (SVG_name) ? SVG_name : "gnuplot";
			    if(GPT.Flags & GpTerminalBlock::fMultiplot && p_gp->GetMultiplotCurrentPanel() < 26)
				    panel[0] = 'a' + p_gp->GetMultiplotCurrentPanel();
			    fprintf(GPT.P_GpOutFile, "\t<g id=\"%s_plot_%d%s_keyentry\" visibility=\"visible\" ", name, SVG_plotno, panel);
			    fprintf(GPT.P_GpOutFile, "onclick=\"gnuplot_svg.toggleVisibility(evt,'%s_plot_%d%s')\"", name, SVG_plotno, panel);
			    fprintf(GPT.P_GpOutFile, ">\n");
		    }
		    break;
		case TERM_LAYER_END_KEYSAMPLE:
		    if(SVG_mouseable) {
			    SVG_PathClose();
			    SVG_GroupFilledClose();
			    fprintf(GPT.P_GpOutFile, "\t</g>\n");
		    }
		    break;
		case TERM_LAYER_RESET:
		case TERM_LAYER_RESET_PLOTNO:
		    SVG_plotno = 0;
		    break;
	}
}

/*------------------------------------------------------------------------------------------------------------------------------------
        SVG_image
   ------------------------------------------------------------------------------------------------------------------------------------*/
#ifdef WRITE_PNG_IMAGE
TERM_PUBLIC void SVG_image(GpTermEntry_Static * pThis, uint m, uint n, coordval * image, gpiPoint * corner, t_imagecolor color_mode)
{
	GnuPlot * p_gp = pThis->P_Gp;
	SVG_PathClose();
	// Map image onto the terminal's coordinate system. 
	fprintf(GPT.P_GpOutFile, "<image x='%.*f' y='%.*f' width='%.*f' height='%.*f' preserveAspectRatio='none' ",
	    PREC, X(corner[0].x), PREC, Y(corner[0].y), PREC, X(corner[1].x) - X(corner[0].x), PREC, Y(corner[1].y) - Y(corner[0].y));
	// Feb 2017 - always embed images 
	if(TRUE || SVG_standalone || SVG_domterm) {
		/* Embed the PNG file in SVG by converting to base64 */
		fprintf(GPT.P_GpOutFile, "xlink:href='data:image/png;base64,");
		if(write_png_base64_image(m, n, image, color_mode, GPT.P_GpOutFile))
			p_gp->OsError(NO_CARET, "SVG_image: could not write to gnuplot output file.");
		fprintf(GPT.P_GpOutFile, "'/>\n");
	}
	else {
		// Write the image to a png file 
		char * base_name = SVG_name ? SVG_name : "gp";
		int wpiresult;
		char * image_file = (char *)SAlloc::M(strlen(base_name)+16);
		sprintf(image_file, "%s_image_%02d.png", base_name, ++SVG_imageno);
		wpiresult = write_png_image(pThis, m, n, image, color_mode, image_file);
		// Reference the png image file 
		fprintf(GPT.P_GpOutFile, "xlink:href='%s_image_%02d.png'/>\n", base_name, SVG_imageno);
		SAlloc::F(image_file);
		if(wpiresult != 0)
			p_gp->OsError(NO_CARET, "SVG_image: could not write to PNG reference file.");
	}
}
#endif

// Enhanced text mode support starts here 
static double ENHsvg_base = 0.0;
static bool ENHsvg_opened_string = FALSE;
static int ENHsvg_charcount = 0;

TERM_PUBLIC void ENHsvg_OPEN(GpTermEntry_Static * pThis, const char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
{
	GnuPlot * p_gp = pThis->P_Gp;
	/* overprint = 1 means print the base text (leave position in center)
	 * overprint = 2 means print the overlying text
	 * overprint = 3 means save current position
	 * overprint = 4 means restore saved position
	 * EAM FIXME - Unfortunately I can find no way in the svg spec to do this.
	 * The best I can come up with is to count characters from here and then
	 * try to back up over them.
	 */
	switch(overprint) {
		case 2:
		    /* FIXME: If there are multiple overprint characters,
		     *  they all get piled on top of one another.
		     */
		    ENHsvg_FLUSH(pThis);
		    fprintf(GPT.P_GpOutFile, "<tspan dx=\"-%.1fem\" dy=\"%.1fpx\">", 0.5 * ENHsvg_charcount, ENHsvg_base-base);
		    ENHsvg_base = base;
		    ENHsvg_x_offset = 0.0;
		    p_gp->Enht.P_CurText = p_gp->Enht.Text;
		    ENHsvg_charcount = 0;
		    ENHsvg_opened_string = TRUE;
		    break;
		case 3:
		    ENHsvg_charcount = 0;
		    return;
		case 4:
		    /* Defer setting the offsets until the text arrives */
		    ENHsvg_x_offset = -0.5 * ENHsvg_charcount;
		    ENHsvg_base -= base;
		    ENHsvg_charcount = 0;
		    return;
		default:
		    break;
	}
	if(!ENHsvg_opened_string) {
		ENHsvg_opened_string = TRUE;
		p_gp->Enht.P_CurText = p_gp->Enht.Text;
		// Start a new textspan fragment 
		fputs("<tspan", GPT.P_GpOutFile);
		if(!fontname)
			fprintf(stderr, "ENHsvg_OPEN: null fontname\n");
		else {
			char * family = sstrdup(fontname);
			char * sep = strchr(family, ':');
			if(sep)
				*sep = '\0';
			if(strcmp(SVG_fontNameCur, family)) {
				SAlloc::F(SVG_fontNameCur);
				SVG_fontNameCur = family;
			}
			else {
				SAlloc::F(family);
			}
			fprintf(GPT.P_GpOutFile, " font-family=\"%s\" ", SVG_fontNameCur);
			if(strstr(fontname, ":Bold"))
				fprintf(GPT.P_GpOutFile, " font-weight=\"bold\" ");
			if(strstr(fontname, ":Italic"))
				fprintf(GPT.P_GpOutFile, " font-style=\"italic\" ");
		}
		if(SVG_fontSizeCur != fontsize) {
			SVG_fontSizeCur = fontsize;
			fprintf(GPT.P_GpOutFile, " font-size=\"%.1f\"", SVG_fontSizeCur * SVG_fontscale);
		}
		if(ENHsvg_x_offset != 0) {
			fprintf(GPT.P_GpOutFile, " dx=\"%.2fem\"", ENHsvg_x_offset);
			ENHsvg_x_offset = 0.0;
		}
		if(ENHsvg_base != base) {
			fprintf(GPT.P_GpOutFile, " dy=\"%.2fpx\"", ENHsvg_base-base);
			ENHsvg_base = base;
		}
		if(!showflag) {
			fprintf(GPT.P_GpOutFile, " fill=\"none\"");
		}
		if(ENHsvg_preserve_spaces) {
			fprintf(GPT.P_GpOutFile, " xml:space=\"preserve\"");
		}
		fputs(">", GPT.P_GpOutFile);
	}
}

TERM_PUBLIC void ENHsvg_FLUSH(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	char * s = p_gp->Enht.Text;
	int i;
	if(ENHsvg_opened_string) {
		ENHsvg_opened_string = FALSE;
		*p_gp->Enht.P_CurText = '\0';
		p_gp->Enht.P_CurText = p_gp->Enht.Text;
		/* DEBUG - expand unicode escape sequences \U+ABCD into &#xABCD;
		 * Triggers in two cases that I know of
		 * 1) encoding is not UTF-8  (probably should not happen for svg)
		 * 2) one too many backslashes in a single-quoted string
		 * We can't just substitute &#x for \U+ in place because the
		 * xml convention requires a trailing semicolon also.
		 * FIXME: this incorrectly handles the case where a legal hex character
		 * immediately follows a 4-char hex unicode entry point
		 * (e.g. the ab ligature in the unicode.dem).
		 */
		while((s = strstr(p_gp->Enht.P_CurText, "\\U+")) != NULL) {
			*s = '\0';
			fputs(p_gp->Enht.P_CurText, GPT.P_GpOutFile); /* everything up to the escape */
			fputs("&#x", GPT.P_GpOutFile); /* xml escape sequence */
			s += 3;                         /* start of hex codepoint */
			for(i = 0; i<5; i++, s++) { /* copy up to 5 hex characters */
				if(isxdigit(*s))
					fputc(*s, GPT.P_GpOutFile);
				else
					break;
			}
			fputs(";", GPT.P_GpOutFile); /* end of xml escape sequence */
			p_gp->Enht.P_CurText = s;
		}
		fputs(p_gp->Enht.P_CurText, GPT.P_GpOutFile); /* everything after the escape[s] */
		fputs("</tspan>", GPT.P_GpOutFile);
	}
}

TERM_PUBLIC void ENHsvg_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str)
{
	GnuPlot * p_gp = pThis->P_Gp;
	// We need local copies of the starting font properties 
	double fontsize = SVG_fontSizeCur;
	static char * fontname = NULL;
	SAlloc::F(fontname);
	fontname = sstrdup(SVG_fontNameCur);
	// We need the full set of tags for text, just as normal. But in 
	// the case of enhanced text ENHsvg_string_state == 1 tells the  
	// SVG_put_text() to return without actually putting the text.   
	if(p_gp->Enht.Ignore) {
		ENHsvg_string_state = 0;
		SVG_put_text(pThis, x, y, str);
		return;
	}
	else {
		ENHsvg_string_state = 1;
		SVG_put_text(pThis, x, y, str);
		ENHsvg_string_state = 0;
	}

	/* EAM FIXME - This is a total hack, to make up for the fact that all  */
	/* svg viewers I have tried fail to pick up the xml:space setting from */
	/* the environment. So it has to be set all over again for each text   */
	/* fragment. Without this, all whitespace is collapsed to a single ' '.*/
	if(strstr(str, "  "))
		ENHsvg_preserve_spaces = TRUE;
	/* Set up global variables needed by enhanced_recursion() */
	ENHsvg_charcount = 0;
	p_gp->Enht.FontScale = 1.0;
	strncpy(p_gp->Enht.EscapeFormat, "%c", sizeof(p_gp->Enht.EscapeFormat));
	while(*(str = enhanced_recursion(pThis, (char *)str, TRUE, fontname, fontsize, 0.0, TRUE, TRUE, 0))) {
		(pThis->enhanced_flush)(pThis);
		p_gp->EnhErrCheck(str);
		if(!*++str)
			break; /* end of string */
	}
	/* Make sure we leave with the same font properties as on entry */
	SAlloc::F(SVG_fontNameCur);
	SVG_fontNameCur = fontname;
	fontname = NULL;
	if(SVG_fontSizeCur != fontsize || ENHsvg_base != 0) {
		fprintf(GPT.P_GpOutFile, "<tspan font-size=\"%.1f\" dy=\"%.2f\"></tspan>", fontsize * SVG_fontscale, ENHsvg_base);
		SVG_fontSizeCur = fontsize;
		ENHsvg_base = 0;
	}
	ENHsvg_preserve_spaces = FALSE;
	/* Close the text section */
	fputs("</text>\n\t</g>\n", GPT.P_GpOutFile);
	return;
}

TERM_PUBLIC void ENHsvg_writec(GpTermEntry_Static * pThis, int c)
{
	GnuPlot * p_gp = pThis->P_Gp;
	// Kludge for phantom box accounting 
	ENHsvg_charcount++;
	// Escape SVG reserved characters. Are there any besides '<' and '&' ? 
	switch(c) {
		case '<':
		    *p_gp->Enht.P_CurText++ = '&';
		    *p_gp->Enht.P_CurText++ = 'l';
		    *p_gp->Enht.P_CurText++ = 't';
		    *p_gp->Enht.P_CurText++ = ';';
		    break;
		case '&':
		    *p_gp->Enht.P_CurText++ = '&';
		    *p_gp->Enht.P_CurText++ = 'a';
		    *p_gp->Enht.P_CurText++ = 'm';
		    *p_gp->Enht.P_CurText++ = 'p';
		    *p_gp->Enht.P_CurText++ = ';';
		    break;
		case '\n':
		    *p_gp->Enht.P_CurText++ = '\\';
		    *p_gp->Enht.P_CurText++ = 'n';
		    break;
		case '\376':
		    // This is an illegal UTF-8 byte; we use it to escape the reserved '&' 
		    if(GPT._Encoding == S_ENC_DEFAULT) {
			    *p_gp->Enht.P_CurText++ = '&';
			    break;
		    } /* else fall through */
		default:
		    *p_gp->Enht.P_CurText++ = c;
		    break;
	}
	// Never overflow the output buffer 
	if((p_gp->Enht.P_CurText - p_gp->Enht.Text) >= sizeof(p_gp->Enht.Text)-1)
		ENHsvg_FLUSH(pThis);
}

TERM_PUBLIC void SVG_path(GpTermEntry_Static * pThis, int p)
{
	switch(p) {
		case 1: // Close path 
		    fputs("Z ", GPT.P_GpOutFile);
		    SVG_PathClose();
		    break;
		case 0:
		    break;
	}
}

TERM_PUBLIC void SVG_hypertext(GpTermEntry_Static * pThis, int type, const char * text)
{
	GnuPlot * p_gp = pThis->P_Gp;
	switch(type) {
		case TERM_HYPERTEXT_TOOLTIP:
		case TERM_HYPERTEXT_TITLE:
		    SAlloc::F(SVG_hypertext_text);
		    if(text) {
			    char * buffer = (char *)SAlloc::M(2+5*strlen(text));
			    p_gp->Enht.P_CurText = buffer;
			    do {
				    ENHsvg_writec(pThis, *text);
			    } while(*text++);
			    SVG_hypertext_text = sstrdup(buffer);
			    p_gp->Enht.P_CurText = NULL;
			    SAlloc::F(buffer);
		    }
		    else {
			    SVG_hypertext_text = NULL;
		    }
		    break;
		case TERM_HYPERTEXT_FONT:
	    {
		    int sep;
		    ZFREE(SVG_hypertext_fontName);
		    ZFREE(SVG_hypertext_fontStyle);
		    ZFREE(SVG_hypertext_fontWeight);
		    SVG_hypertext_fontSize = 0;
		    if(!text || !(*text))
			    break;
		    sep = strcspn(text, ",: ");
		    if(sep > 0) {
			    SVG_hypertext_fontName = sstrdup(text);
			    SVG_hypertext_fontName[sep] = '\0';
		    }
		    sep = strcspn(text, ",");
		    if(sep > 0)
			    sscanf(text + sep + 1, "%lf", &SVG_hypertext_fontSize);
		    if(strstr(text, "italic") || strstr(text, "Italic"))
			    SVG_hypertext_fontStyle = sstrdup("italic");
		    if(strstr(text, "bold") || strstr(text, "Bold"))
			    SVG_hypertext_fontWeight = sstrdup("bold");
	    }
	    break;
		default:
		    break;
	}
}

TERM_PUBLIC void SVG_boxed_text(GpTermEntry_Static * pThis, uint x, uint y, int option)
{
	switch(option) {
		case TEXTBOX_INIT:
		    // Mark group containing next text item 
		    SVG_inTextBox = TRUE;
		    break;
		case TEXTBOX_OUTLINE:
		// Stroke the outline of the bounding box (FIXME: how???) 
		case TEXTBOX_BACKGROUNDFILL:
		    // Close the group, which will trigger application of the filter 
		    SVG_inTextBox = FALSE;
		    break;
		case TEXTBOX_MARGINS:
		    // Adjust the size of the bounding box 
		    break;
	}
}

#undef Y
#undef X
#undef PREC

#endif /* TERM_BODY */

#ifdef TERM_TABLE
TERM_TABLE_START(svg_driver)
	"svg", 
	"W3C Scalable Vector Graphics",
	0 /* xmax */, 
	0 /* ymax */, 
	0 /* vchar */, 
	0 /* hchar */,
	0 /* vtic */, 
	0 /* htic */,
	SVG_options, 
	SVG_init, 
	SVG_reset, 
	SVG_text, 
	GnuPlot::NullScale, 
	SVG_graphics,
	SVG_move, 
	SVG_vector, 
	SVG_linetype, 
	SVG_put_text, 
	SVG_text_angle,
	SVG_justify_text, 
	SVG_point, 
	GnuPlot::DoArrow, 
	SVG_set_font, 
	GnuPlot::DoPointSize,
	TERM_CAN_DASH | TERM_ALPHA_CHANNEL|TERM_LINEWIDTH,
	0, /* suspend */
	0, /* resume */
	SVG_fillbox, 
	SVG_linewidth,
	#ifdef USE_MOUSE
	0, 
	0, 
	0, 
	0, 
	0, /* no mouse support for svg */
	#endif
	SVG_make_palette,
	SVG_previous_palette,
	SVG_set_color,
	SVG_filled_polygon,
	#ifdef WRITE_PNG_IMAGE
	SVG_image,
	#else
	NULL, /* image */
	#endif
	ENHsvg_OPEN, 
	ENHsvg_FLUSH, 
	ENHsvg_writec, 
	SVG_layer,     /* layer */
	SVG_path,      /* path */
	SVG_SCALE,     /* pixel oversampling scale */
	SVG_hypertext,         /* hypertext support */
	SVG_boxed_text,        /* boxed text labels */
	NULL,                  /* modify_plots */
	SVG_dashtype          /* Version 5 dashtype support */
TERM_TABLE_END(svg_driver)

#undef LAST_TERM
#define LAST_TERM svg_driver

TERM_TABLE_START(domterm_driver)
	"domterm", 
	"DomTerm terminal emulator with embedded SVG",
	0 /* xmax */, 
	0 /* ymax */, 
	0 /* vchar */, 
	0 /* hchar */,
	0 /* vtic */, 
	0 /* htic */,
	SVG_options, 
	SVG_init, 
	SVG_reset, 
	SVG_text, 
	GnuPlot::NullScale, 
	SVG_graphics,
	SVG_move, 
	SVG_vector, 
	SVG_linetype, 
	SVG_put_text, 
	SVG_text_angle,
	SVG_justify_text, 
	SVG_point, 
	GnuPlot::DoArrow, 
	SVG_set_font, 
	GnuPlot::DoPointSize,
	TERM_CAN_DASH | TERM_ALPHA_CHANNEL|TERM_LINEWIDTH,
	0, /* suspend */
	0, /* resume */
	SVG_fillbox, 
	SVG_linewidth,
	#ifdef USE_MOUSE
	0, 
	0, 
	0, 
	0, 
	0,    /* no mouse support for svg */
	#endif
	SVG_make_palette,
	SVG_previous_palette,
	SVG_set_color,
	SVG_filled_polygon,
	#ifdef WRITE_PNG_IMAGE
		SVG_image,
	#else
		NULL,          /* image */
	#endif
	ENHsvg_OPEN, 
	ENHsvg_FLUSH, 
	ENHsvg_writec, 
	SVG_layer, /* layer */
	SVG_path,  /* path */
	SVG_SCALE, /* pixel oversampling scale */
	SVG_hypertext, /* hypertext support */
	SVG_boxed_text, /* boxed text labels */
	NULL, /* modify_plots */
	SVG_dashtype          /* Version 5 dashtype support */
TERM_TABLE_END(domterm_driver)

#undef LAST_TERM
#define LAST_TERM domterm_driver

#endif /* TERM_TABLE */
#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
START_HELP(svg)
"1 svg",
"?commands set terminal svg",
"?set terminal svg",
"?set term svg",
"?terminal svg",
"?term svg",
"?svg",
" This terminal produces files in the W3C Scalable Vector Graphics format.",
"",
" Syntax:",
"       set terminal svg {size <x>,<y> {|fixed|dynamic}}",
"                        {mouse} {standalone | jsdir <dirname>}",
"                        {name <plotname>}",
"                        {font \"<fontname>{,<fontsize>}\"} {{no}enhanced}",
"                        {fontscale <multiplier>}",
"                        {rounded|butt|square} {solid|dashed} {linewidth <lw>}",
"                        {background <rgb_color>}",
"",
" where <x> and <y> are the size of the SVG plot to generate,",
" `dynamic` allows a svg-viewer to resize plot, whereas the default",
" setting, `fixed`, will request an absolute size.",
"",
" `linewidth <w>` increases the width of all lines used in the figure",
" by a factor of <w>.",
"",
" <font> is the name of the default font to use (default Arial) and",
" <fontsize> is the font size (in points, default 12). SVG viewing",
" programs may substitute other fonts when the file is displayed.",
"",
" The enhanced text mode syntax is shared with other gnuplot terminal types.",
" See `enhanced` for more details.",
"",
" The `mouse` option tells gnuplot to add support for mouse tracking and for",
" toggling individual plots on/off by clicking on the corresponding key entry.",
" By default this is done by including a link that points to a script in a",
" local directory, usually /usr/local/share/gnuplot/<version>/js.",
" You can change this by using the `jsdir` option to specify either a",
" different local directory or a general URL. The latter is usually",
" appropriate if you are embedding the svg into a web page.",
" Alternatively, the `standalone` option embeds the mousing code in the svg",
" document itself rather than linking to an external resource.",
"",
" When an SVG file will be used in conjunction with external files,",
" e.g. if it is referenced by javascript code in a web page or parent document,",
" then a unique name is required to avoid potential conflicting references",
" to other SVG plots.  Use the `name` option to ensure uniqueness.",
""
END_HELP(svg)

START_HELP(domterm)
"1 domterm",
"?set terminal domterm",
"?terminal domterm",
"?set term domterm",
"?term domterm",
"?domterm",
" Syntax:",
"       set terminal domterm ",
"                        {font \"<fontname>{,<fontsize>}\"} {{no}enhanced}",
"                        {fontscale <multiplier>}",
"                        {rounded|butt|square} {solid|dashed} {linewidth <lw>}",
"                        {background <rgb_color>}",
"                        {animate}",
"",
" The `domterm` terminal device runs on the DomTerm terminal emulator",
" including the domterm and qtdomterm programs.",
" It supports SVG graphics embedded directly in the terminal output.",
" See http://domterm.org .",
"",
" For information on terminal options, please see the `svg` terminal.",
"",
"2 animate",
"?set term domterm animate",
"?term domterm animate",
"      set term domterm animate",
"",
" The `animate` option resets the cursor position to the terminal top left at",
" the start of every plot so that successive plots overwrite the same area on",
" the screen. This may be desirable in order to create an in-place animation.",
""
END_HELP(domterm)
#endif /* TERM_HELP */
