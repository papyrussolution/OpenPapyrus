// GNUPLOT - tkcanvas.trm 
// Copyright 1990 - 1993, 1998, 2004, 2014
//
/*
 * This file is included by ../term.c.
 *
 * This terminal driver supports:
 *  Tk canvas widgets under several scripting languages
 *  (currently Tcl, Perl/Tk, Perl/Tkx, Python, Ruby, Rexx)
 *
 * AUTHORS and HISTORY:
 *  original dxy.trm by Martin Yii, eln557h@monu3.OZ
 *  Further modified Jan 1990 by Russell Lang, rjl@monu1.cc.monash.oz
 *
 * port to the Tk/Tcl canvas widget
 *   D. Jeff Dionne, July 1995 jeff@ryeham.ee.ryerson.ca
 *   Alex Woo, woo@playfair.stanford.edu
 *
 * adapted to the new terminal layout by Alex Woo (Sept. 1996)
 *
 * extended interactive Tk/Tcl capabilities
 *   Thomas Sefzick, March 1999, t.sefzick@fz-juelich.de
 *
 * added the perltk.trm code written by Slaven Rezic <eserte@cs.tu-berlin.de>.
 * 'linewidth' and 'justify text' added, ends of plotted lines are now rounded.
 *   Thomas Sefzick, May 1999, t.sefzick@fz-juelich.de
 *
 * scale plot to fit into the actual size of the canvas as reported by
 * the window manager (the canvas itself doesn't report its real size).
 *   Matt Willis, October 1999, mattbwillis@my-deja.com
 *
 * cleaned up and generalized in order to accommodate an increasing
 * number of scripting languages; added support for Python and Ruby.
 * based on a patch dated October 2002.
 *   Joachim Wuttke, November 2014, jwuttke@users.sourceforge.net
 *
 * Add support for Perl/Tkx and Rexx.
 * Add support for rgb and palette color, filled boxes and polygons,
 * rotated text, (custom) dashed lines, background colour, closed paths,
 * rounded or butt line ends, Tk-arrows, optimized drawing of lines,
 * boxed text and bold and italic text. Add 'size' option to give the
 * code a hint of proper tic and font sizes. Add 'standalone' option to
 * create self-contained scripts. Add support for enhanced text and
 * external images for Tcl only.
 *   Bastian Maerkisch, December 2014, bmaerkisch@web.de
 *
 * BUGS or MISSING FEATURES:
 *  - enhanced text only for Tcl
 *  - option to change function name (multiple plots)
 *  - layer actions by adding tags to items
 *  - hypertext and image are missing
 *  - transparency is not possible
 *  - text encoding setting is ignored, we always use the system's default
 *  - The "interactive" mode has several issues:
 *    - The optimised line drawing, which merges adjacent segments into one
 *      path, renders the 'interactive mode' pretty useless.
 *    - It is not (yet) implemented at all for Python/Tkinter and Rexx/Tk.
 *    - Ruby/Tkinter: no support for user_gnuplot_coordinates().
 *  - gnuplot_xy:
 *      the definition (with 12 input and 4 output coordinates) is clumsy,
 *      and the implementation is unelegant.
 *  - we don't take advantage of object orientation; our Ruby code looks
 *      like an almost literal translation from Tcl (because that's what it is).
 *  - no support for Lua/Tk
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
	register_term(tkcanvas)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void TK_options(GpTermEntry_Static * pThis, GnuPlot * pGp);
TERM_PUBLIC void TK_init(GpTermEntry_Static * pThis);
TERM_PUBLIC void TK_graphics(GpTermEntry_Static * pThis);
TERM_PUBLIC void TK_text(GpTermEntry_Static * pThis);
TERM_PUBLIC void TK_linetype(GpTermEntry_Static * pThis, int linetype);
TERM_PUBLIC void TK_move(GpTermEntry_Static * pThis, uint x, uint y);
TERM_PUBLIC void TK_vector(GpTermEntry_Static * pThis, uint x, uint y);
TERM_PUBLIC int  TK_text_angle(GpTermEntry_Static * pThis, int ang);
TERM_PUBLIC void TK_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str);
TERM_PUBLIC void TK_reset(GpTermEntry_Static * pThis);
TERM_PUBLIC int  TK_justify_text(GpTermEntry_Static * pThis, enum JUSTIFY);
TERM_PUBLIC void TK_point(GpTermEntry_Static * pThis, uint, uint, int);
#if 0
TERM_PUBLIC void TK_arrow(GpTermEntry_Static * pThis, uint, uint, uint, uint, int);
#endif
TERM_PUBLIC int  TK_set_font(GpTermEntry_Static * pThis, const char * font);
TERM_PUBLIC void TK_enhanced_open(GpTermEntry_Static * pThis, const char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
TERM_PUBLIC void TK_enhanced_flush(GpTermEntry_Static * pThis);
TERM_PUBLIC void TK_linewidth(GpTermEntry_Static * pThis, double linewidth);
TERM_PUBLIC int  TK_make_palette(GpTermEntry_Static * pThis, t_sm_palette * palette);
TERM_PUBLIC void TK_color(GpTermEntry_Static * pThis, const t_colorspec * colorspec);
TERM_PUBLIC void TK_fillbox(GpTermEntry_Static * pThis, int style, uint x, uint y, uint w, uint h);
TERM_PUBLIC void TK_filled_polygon(GpTermEntry_Static * pThis, int points, gpiPoint * corners);
#ifdef WRITE_PNG_IMAGE
	TERM_PUBLIC void TK_image(GpTermEntry_Static * pThis, uint m, uint n, coordval * image, gpiPoint * corner, t_imagecolor color_mode);
#endif
TERM_PUBLIC void TK_dashtype(GpTermEntry_Static * pThis, int dt, t_dashtype * custom_dash_pattern);
TERM_PUBLIC void TK_boxed_text(GpTermEntry_Static * pThis, uint x, uint y, int option);

// nominal canvas size 
#define TK_XMAX 1000
#define TK_YMAX 1000
// char size and tic sizes in pixels 
#define TK_VCHAR        14      /* height of characters */
#define TK_HCHAR         6      /* width of characters including spacing */
#define TK_VTIC          8
#define TK_HTIC          8

//#endif /* TERM_PROTO */

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY

/* FIXME HBB 20000725: This needs to be fixed.  As is, this driver causes
 * the terminal layer to depend on several other core modules. This is a
 * design bug. "term" is supposed as 'frontier' layer: it should not be
 * dependent on any other code inside gnuplot */

/* text, font */
static int tk_angle = 0;
static char tk_anchor[7] = "w";
static enum JUSTIFY tk_justify;
static bool tk_next_text_use_font = FALSE;
static bool tk_boxed = FALSE;

/* enhanced text */
static bool tk_enhanced_opened_string = FALSE;
static bool tk_enhanced_show = FALSE;
static int tk_enhanced_base = 0;
static int tk_enhanced_overprint = 0;
static bool tk_enhanced_widthflag = FALSE;

/* vectors, polygons, paths */
static bool tk_rounded = FALSE;
static double tk_linewidth = 1.0;
static int tk_lastx = 0;
static int tk_lasty = 0;
static bool tk_in_path = FALSE;
static int * tk_path_x = NULL;
static int * tk_path_y = NULL;
static int tk_maxpath = 0;
static int tk_polygon_points = 0;
static char tk_dashpattern[3*DASHPATTERN_LENGTH];
static const char * tk_dashtypes[] = {
	"", "1 1", "", "3 1", "2 2", "3 1 1 1", "3 1 1 1 1 1"
};

/* color */
static const char * tk_colors[] = {
	"black", "gray", "red", "green", "blue", "magenta", "cyan", "brown"
};
static char tk_color[20] = "black";
static char tk_background[20] = "";
static char * tk_background_opt = NULL;

/* other options */
static bool tk_interactive = FALSE;
static bool tk_standalone = FALSE;
static int tk_width  = 800;
static int tk_height = 600;

/* images */
static int tk_image_counter = 0;

/* prototypes of local functions */
static void TK_put_noenhanced_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str);
static void TK_put_enhanced_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str);
static void TK_rectangle(int x1, int y1, int x2, int y2, const char * color, const char * stipple);
static void TK_add_path_point(int x, int y); /* add a new point to current path or line */
static void TK_flush_line(GpTermEntry_Static * pThis); // finish a poly-line 

enum TK_id {
	/* languages first (order is important as it is used as index!) */
	TK_LANG_TCL = 0, 
	TK_LANG_PERL, 
	TK_LANG_PYTHON, 
	TK_LANG_RUBY, 
	TK_LANG_REXX,
	TK_LANG_PERLTKX, 
	TK_LANG_MAX,
	/* other options */
	TK_INTERACTIVE, 
	TK_STANDALONE, 
	TK_INPUT,
	TK_NOROTTEXT, 
	TK_ROTTEXT, 
	TK_BACKGROUND, 
	TK_NOBACKGROUND,
	TK_ROUNDED, 
	TK_BUTT, 
	TK_SIZE, 
	TK_ENHANCED, 
	TK_NOENHANCED,
	TK_PIXELS, 
	TK_EXTERNALIMAGES, 
	TK_INLINEIMAGES,
	TK_OTHER
};

static int tk_script_language = TK_LANG_TCL;
static const char * tk_script_languages[TK_LANG_MAX] = { "tcl", "perl", "python", "ruby", "rexx", "perltkx" };

static struct gen_table TK_opts[] =
{
	{ "t$cl", TK_LANG_TCL },
	{ "pe$rltk", TK_LANG_PERL },
	{ "perltkx", TK_LANG_PERLTKX },
	{ "tkx", TK_LANG_PERLTKX },
	{ "py$thontkinter", TK_LANG_PYTHON },
	{ "ru$bytkinter", TK_LANG_RUBY },
	{ "re$xxtk", TK_LANG_REXX },
	{ "int$eractive", TK_INTERACTIVE },
	{ "inp$ut", TK_INPUT },
	{ "stand$alone", TK_STANDALONE },
	{ "nor$ottext", TK_NOROTTEXT },
	{ "rot$text", TK_ROTTEXT },
	{ "backg$round", TK_BACKGROUND },
	{ "noback$ground", TK_NOBACKGROUND },
	{ "round$ed", TK_ROUNDED },
	{ "butt", TK_BUTT },
	{ "size", TK_SIZE },
	{ "enh$anced", TK_ENHANCED },
	{ "noenh$anced", TK_NOENHANCED },
	{ "pix$els", TK_PIXELS },
	{ "inl$ineimages", TK_INLINEIMAGES },
	{ "ext$ernalimages", TK_EXTERNALIMAGES },
	{ NULL, TK_OTHER }
};

TERM_PUBLIC void TK_options(GpTermEntry_Static * pThis, GnuPlot * pGp)
{
	int cmd;
	tk_interactive = FALSE;
	while(!pGp->Pgm.EndOfCommand()) {
		switch(cmd = pGp->Pgm.LookupTableForCurrentToken(&TK_opts[0])) {
			case TK_LANG_TCL:
			case TK_LANG_PERL:
			case TK_LANG_PERLTKX:
			case TK_LANG_RUBY:
			case TK_LANG_PYTHON:
			case TK_LANG_REXX:
			    tk_script_language = cmd;
			    pGp->Pgm.Shift();
			    break;
			case TK_INTERACTIVE:
			    tk_interactive = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case TK_INPUT:
			    tk_standalone = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case TK_STANDALONE:
			    tk_standalone = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case TK_NOROTTEXT:
			    pThis->text_angle = GnuPlot::NullTextAngle;
			    pGp->Pgm.Shift();
			    break;
			case TK_ROTTEXT:
			    pThis->text_angle = TK_text_angle;
			    pGp->Pgm.Shift();
			    break;
			case TK_BACKGROUND: 
				{
					long rgb;
					int red, green, blue;
					pGp->Pgm.Shift();
					rgb = pGp->ParseColorName();
					SAlloc::F(tk_background_opt);
					tk_background_opt = NULL;
					pGp->Pgm.MCapture(&tk_background_opt, pGp->Pgm.GetPrevTokenIdx(), pGp->Pgm.GetCurTokenIdx());
					red   = (rgb >> 16) & 0xff;
					green = (rgb >>  8) & 0xff;
					blue  = (rgb      ) & 0xff;
					snprintf(tk_background, sizeof(tk_background), "#%02x%02x%02x", red, green, blue);
				}
				break;
			case TK_NOBACKGROUND:
			    tk_background[0] = '\0';
			    SAlloc::F(tk_background_opt);
			    tk_background_opt = NULL;
			    pGp->Pgm.Shift();
			    break;
			case TK_ROUNDED:
			    tk_rounded = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case TK_BUTT:
			    tk_rounded = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case TK_SIZE: {
			    pGp->Pgm.Shift();
			    if(pGp->Pgm.EndOfCommand())
				    pGp->IntErrorCurToken("size requires 'width,heigth'");
			    tk_width = static_cast<int>(pGp->RealExpression());
			    if(!pGp->Pgm.EqualsCurShift(","))
				    pGp->IntErrorCurToken("size requires 'width,heigth'");
			    tk_height = static_cast<int>(pGp->RealExpression());
			    if(tk_width < 1 || tk_height < 1)
				    pGp->IntErrorCurToken("size is out of range");
			    break;
		    }
			case TK_ENHANCED:
			    pGp->Pgm.Shift();
			    pThis->SetFlag(TERM_ENHANCED_TEXT);
			    break;
			case TK_NOENHANCED:
			    pGp->Pgm.Shift();
			    pThis->ResetFlag(TERM_ENHANCED_TEXT);
			    break;
			case TK_PIXELS:
			    pGp->Pgm.Shift();
			    pThis->image = NULL;
			    break;
			case TK_INLINEIMAGES:
			case TK_EXTERNALIMAGES:
			    pGp->Pgm.Shift();
#ifdef WRITE_PNG_IMAGE
			    pThis->image = TK_image;
#endif
			    break;
			case TK_OTHER:
			default:
			    pGp->Pgm.Shift();
			    pGp->IntErrorCurToken("unknown option");
			    break;
		}
	}
	// calculate the proper tic sizes and character size 
	pThis->SetCharSize(static_cast<uint>(TK_HCHAR * TK_XMAX / (double)tk_width + 0.5), static_cast<uint>(TK_VCHAR * TK_YMAX / (double)tk_height + 0.5));
	pThis->SetTic(static_cast<uint>(TK_HTIC  * TK_XMAX / (double)tk_width + 0.5), static_cast<uint>(TK_VTIC  * TK_YMAX / (double)tk_height + 0.5));
	// FIXME: image support only available for Tcl 
	if(pThis->image && tk_script_language != TK_LANG_TCL)
		pThis->image = NULL;
	// FIXME: enhanced text only available for Tcl 
	if((pThis->flags & TERM_ENHANCED_TEXT) && (tk_script_language != TK_LANG_TCL))
		pThis->ResetFlag(TERM_ENHANCED_TEXT);
	slprintf(GPT._TermOptions, "%s%s %s %s%s %s %s %s size %d,%d", tk_script_languages[tk_script_language],
	    tk_interactive ? " interactive" : "", tk_standalone ? "standalone" : "input",
	    (!tk_background[0]) ? "nobackground " : "background ", (!tk_background[0]) ? "" : tk_background_opt,
	    tk_rounded ? "rounded" : "butt", pThis->text_angle == GnuPlot::NullTextAngle ? "norottext" : "rottext",
	    pThis->image == NULL ? "pixels" : "externalimages", tk_width, tk_height);
}

TERM_PUBLIC void TK_init(GpTermEntry_Static * pThis)
{
	tk_image_counter = 0;
}

static const char * tk_standalone_init[TK_LANG_MAX] = {
	/* Tcl */
	"canvas .c -width %d -height %d\n"
	"pack .c\n"
	"gnuplot .c\n\n",
	/* Perl */
	"use Tk;\n"
	"my $top = MainWindow->new;\n"
	"my $c = $top->Canvas(-width => %d, -height => %d)->pack;\n"
	"gnuplot($c);\n"
	"MainLoop;\n",
	/* Python */
	"from tkinter import *\n"
	"from tkinter import font\n"
	"root = Tk()\n"
	"c = Canvas(root, width=%d, height=%d)\n"
	"c.pack()\n"
	"gnuplot(c)\n"
	"root.mainloop()\n",
	/* Ruby */
	"require 'tk'\n"
	"root = TkRoot.new { title 'Ruby/Tk' }\n"
	"c = TkCanvas.new(root, 'width'=>%d, 'height'=>%d) { pack  { } }\n"
	"gnuplot(c)\n"
	"Tk.mainloop\n",
	/* Rexx */
	"/**/\n"
	"call RxFuncAdd 'TkLoadFuncs', 'rexxtk', 'TkLoadFuncs'\n"
	"call TkLoadFuncs\n"
	"cv = TkCanvas('.c', '-width', %d, '-height', %d)\n"
	"call TkPack cv\n"
	"call gnuplot cv\n"
	"do forever\n"
	"   interpret 'call' TkWait()\n"
	"end\n"
	"return 0\n\n"
	"exit:\nquit:\n"
	"call TkDropFuncs\n"
	"exit 0\n",
	/* Perl/Tkx */
	"use Tkx;\n"
	"my $top = Tkx::widget->new(\".\");\n"
	"my $c = $top->new_tk__canvas(-width => %d, -height => %d);\n"
	"$c->g_pack;\n"
	"gnuplot($c);\n"
	"Tkx::MainLoop();\n"
};

static const char * tk_init_gnuplot[TK_LANG_MAX] = {
	/* Tcl */
	"proc %s cv {\n"
	"  $cv delete all\n"
	"  set cmx [expr\\\n"
	"    [winfo width $cv]-2*[$cv cget -border]"
	"-2*[$cv cget -highlightthickness]]\n"
	"  if {$cmx <= 1} {set cmx [$cv cget -width]}\n"
	"  set cmy [expr\\\n"
	"    [winfo height $cv]-2*[$cv cget -border]"
	"-2*[$cv cget -highlightthickness]]\n"
	"  if {$cmy <= 1} {set cmy [$cv cget -height]}\n",

	/* Perl */
	"sub %s {\n"
	"  my($cv) = @_;\n"
	"  $cv->delete('all');\n"
	"  my $cmx = $cv->width - 2 * $cv->cget(-border)\n"
	"            - 2 * $cv->cget(-highlightthickness);\n"
	"  if ($cmx <= 1) {\n"
	"    $cmx = ($cv->cget(-width));\n"
	"  }\n"
	"  my $cmy = $cv->height - 2 * $cv->cget(-border)\n"
	"            - 2 * $cv->cget(-highlightthickness);\n"
	"  if ($cmy <= 1) {\n"
	"    $cmy = ($cv->cget(-height));\n"
	"  }\n",

	/* Python */
	"def %s (cv):\n"
	"\tcv.delete('all')\n"
	"\tcmdelta = 2*(int(cv.cget('border'))+"
	"int(cv.cget('highlightthickness')))\n"
	"\tcmx = int(cv.cget('width'))-cmdelta\n"
	"\tif (cmx<=1):\n\t\tcmx = int(cv.cget('width'))\n"
	"\tcmy = int(cv.cget('height'))-cmdelta\n"
	"\tif (cmy<=1):\n\t\tcmy = int(cv.cget('height'))\n"
	"",

	/* Ruby (below, we NEED the blank in "- 2" !)*/
	"def %s(cv)\n"
	"  cv.delete('all')\n"
	"  cmx = cv.width - 2*cv.cget('border') - 2*cv.cget('highlightthickness')\n"
	"  cmx = cvcget.width  if (cmx <= 1)\n"
	"  cmy = cv.height - 2*cv.cget('border') - 2*cv.cget('highlightthickness')\n"
	"  cmy = cvcget.height if (cmy <= 1)\n"
	"",

	/* Rexx */
	"/**/\n"
	"call %s arg(1)\n"
	"return 0\n\n"
	"%s: procedure\n"
	"  cv = arg(1)\n"
	"  call TkCanvasDelete cv,'all'\n"
	"  cmx = TkCget(cv,'-width') - 2*TkCget(cv,'-border') - 2*TkCget(cv,'-highlightthickness')\n"
	"  if cmx <= 1 then; cmx = TkCget(cv,'-width')\n"
	"  cmy = TkCget(cv,'-height') - 2*TkCget(cv,'-border') - 2*TkCget(cv,'-highlightthickness')\n"
	"  if cmy <= 1 then; cmy = TkCget(cv,'-height')\n"
	"\n",

	/* Perl/Tkx */
	"sub %s {\n"
	"  my($cv) = @_;\n"
	"  $cv->delete('all');\n"
	"  my $cmx = $cv->get_width - 2 * $cv->cget(-border)\n"
	"            - 2 * $cv->cget(-highlightthickness);\n"
	"  if ($cmx <= 1) {\n"
	"    $cmx = ($cv->cget(-width));\n"
	"  }\n"
	"  my $cmy = $cv->get_height - 2 * $cv->cget(-border)\n"
	"            - 2 * $cv->cget(-highlightthickness);\n"
	"  if ($cmy <= 1) {\n"
	"    $cmy = ($cv->cget(-height));\n"
	"  }\n"
};

static const char * tk_set_background[TK_LANG_MAX] = {
	/* Tcl */
	"  $cv configure -bg %s\n",
	/* Perl */
	"  $cv->configure(-bg => q{%s});\n",
	/* Python */
	"\tcv.configure(bg='%s')\n",
	/* Ruby */
	"  cv.configure('bg'=>'%s')\n",
	/* Rexx */
	"  call TkConfigure cv, '-bg', '%s'\n",
	/* Perl/Tkx */
	"  $cv->configure(-bg => q{%s});\n"
};

TERM_PUBLIC void TK_graphics(GpTermEntry_Static * pThis)
{
	/*
	 * Here we start the definition of the `gnuplot` procedure.
	 * The resulting script code takes the actual width and height
	 * of the defined canvas and scales the plot to fit.
	 * You can tune the output for a particular size of the canvas by
	 * using the `size` option.
	 */
	const char * tk_function = "gnuplot";
	// Reset to start of output file.  If the user mistakenly tries to
	// plot again into the same file, it will overwrite the original
	// rather than corrupting it.
	if(GPT.P_GpOutFile != stdout) {
		fseek(GPT.P_GpOutFile, 0L, SEEK_SET);
		fflush(GPT.P_GpOutFile);
		if(ftruncate(_fileno(GPT.P_GpOutFile), (off_t)0) != 0)
			pThis->P_Gp->IntWarn(NO_CARET, "Error re-writing output file: %s", strerror(errno));
	}
	if(!tk_standalone && oneof2(tk_script_language, TK_LANG_PERL, TK_LANG_PERLTKX))
		tk_function = "";
	if(tk_standalone && (tk_script_language == TK_LANG_REXX))
		fprintf(GPT.P_GpOutFile, tk_standalone_init[tk_script_language], tk_width, tk_height);
	fprintf(GPT.P_GpOutFile, tk_init_gnuplot[tk_script_language], tk_function, tk_function);
	tk_angle = tk_lastx = tk_lasty = 0;
	strnzcpy(tk_color, tk_colors[0], sizeof(tk_color));
	// set background 
	if(tk_background[0]) {
		fprintf(GPT.P_GpOutFile, tk_set_background[tk_script_language], tk_background);
	}
}

TERM_PUBLIC void TK_reset(GpTermEntry_Static * pThis)
{
	SAlloc::F(tk_path_x);
	SAlloc::F(tk_path_y);
	tk_path_x = tk_path_y = NULL;
	tk_polygon_points = tk_maxpath = 0;
}

TERM_PUBLIC void TK_linetype(GpTermEntry_Static * pThis, int linetype)
{
	t_colorspec colorspec;
	colorspec.type = TC_LT;
	colorspec.lt = linetype;
	TK_color(pThis, &colorspec);
	TK_dashtype(pThis, DASHTYPE_SOLID, NULL);
}

TERM_PUBLIC int TK_make_palette(GpTermEntry_Static * pThis, t_sm_palette * palette)
{
	return 0; /* we can do RGB colors */
}

TERM_PUBLIC void TK_color(GpTermEntry_Static * pThis, const t_colorspec * colorspec)
{
	char tmp_color[20];
	strnzcpy(tmp_color, tk_color, sizeof(tmp_color));
	switch(colorspec->type) {
		case TC_LT: {
		    int linetype = colorspec->lt;
		    const char * color = NULL;
		    if(linetype == LT_BACKGROUND)
			    color = tk_background[0] ? tk_background : "white";
		    if(linetype == LT_NODRAW)
			    color = "";
		    if(!color) {
			    if(linetype < LT_BLACK)
				    linetype = LT_BLACK;
			    color = (char *)tk_colors[(linetype + 2) % 8];
		    }
		    strnzcpy(tmp_color, color, sizeof(tmp_color));
		    break;
	    }
		case TC_FRAC: {
		    rgb255_color rgb255;
		    // Immediately translate palette index to RGB colour 
		    pThis->P_Gp->Rgb255MaxColorsFromGray(colorspec->value, &rgb255);
		    snprintf(tmp_color, sizeof(tmp_color), "#%02x%02x%02x", rgb255.r, rgb255.g, rgb255.b);
		    break;
	    }
		case TC_RGB: {
		    int red = (colorspec->lt >> 16) & 0xff;
		    int green = (colorspec->lt >> 8) & 0xff;
		    int blue = (colorspec->lt) & 0xff;
		    snprintf(tmp_color, sizeof(tk_color), "#%02x%02x%02x", red, green, blue);
		    break;
	    }
		default:
		    break;
	}
	if(strcmp(tk_color, tmp_color) != 0) {
		TK_flush_line(pThis);
		strnzcpy(tk_color, tmp_color, sizeof(tk_color));
	}
}

TERM_PUBLIC void TK_linewidth(GpTermEntry_Static * pThis, double linewidth)
{
	if(fabs(tk_linewidth - linewidth) > FLT_EPSILON)
		TK_flush_line(pThis);
	tk_linewidth = linewidth;
}

TERM_PUBLIC void TK_dashtype(GpTermEntry_Static * pThis, int dt, t_dashtype * custom_dash_pattern)
{
	int i;
	char tmp_dashpattern[3*DASHPATTERN_LENGTH];
	bool preserve = FALSE;
	if(dt >= 0) {
		dt %= 5;
		dt += 2;
		strcpy(tmp_dashpattern, tk_dashtypes[dt]);
	}
	else if(dt == DASHTYPE_SOLID) {
		tmp_dashpattern[0] = '\0';
	}
	else if(dt == DASHTYPE_AXIS) {
		strcpy(tmp_dashpattern, tk_dashtypes[1]);
	}
	else if(dt == DASHTYPE_CUSTOM) {
		if(custom_dash_pattern->dstring[0]) {
			// Tk and gnuplot support the very same dash pattern syntax. 
			strncpy(tmp_dashpattern, custom_dash_pattern->dstring, sizeof(tmp_dashpattern)-1);
			preserve = TRUE; // do not change pattern 
		}
		else {
			tmp_dashpattern[0] = '\0';
			for(i = 0; (i < DASHPATTERN_LENGTH/2) && (fabs(custom_dash_pattern->pattern[2*i]) > FLT_EPSILON); i++) {
				char buf[32];
				snprintf(buf, sizeof(buf), "%d %d ",
				    (int)(custom_dash_pattern->pattern[2*i] * tk_linewidth),
				    (int)(custom_dash_pattern->pattern[2*i + 1] * tk_linewidth));
				strncat(tmp_dashpattern, buf, sizeof(tmp_dashpattern) - strlen(tmp_dashpattern)-1);
			}
			tmp_dashpattern[strlen(tmp_dashpattern)-1] = '\0';
		}
	}

	if((tk_script_language == TK_LANG_PYTHON) && !preserve) {
		for(i = 0; tmp_dashpattern[i]; i++)
			if(tmp_dashpattern[i] == ' ')
				tmp_dashpattern[i] = ',';
	}

	if(strcmp(tk_dashpattern, tmp_dashpattern) != 0) {
		TK_flush_line(pThis);
		strnzcpy(tk_dashpattern, tmp_dashpattern, sizeof(tk_dashpattern));
	}
}

TERM_PUBLIC void TK_move(GpTermEntry_Static * pThis, uint x, uint y)
{
	// terminate current path if we move to a disconnected position 
	if(tk_polygon_points > 0) {
		if((tk_path_x[tk_polygon_points-1] != x) || (tk_path_y[tk_polygon_points-1] != TK_YMAX - y))
			TK_flush_line(pThis);
		else
			return;
	}
	TK_add_path_point(x, TK_YMAX - y);
	tk_lastx = x;
	tk_lasty = TK_YMAX - y;
}

// FIXME HBB 20000725: should use AXIS_UNDO_LOG() macro... 
//#define TK_REAL_VALUE(value, axis) (p_gp->AxS[axis].log) ? pow(p_gp->AxS[axis].base, p_gp->AxS[axis].min + value*(p_gp->AxS[axis].GetRange())) : p_gp->AxS[axis].min + value*(p_gp->AxS[axis].GetRange())
//#define TK_X_VALUE(value) (double)(value-p_gp->V.BbPlot.xleft)/(double)(p_gp->V.BbPlot.xright-p_gp->V.BbPlot.xleft)
//#define TK_Y_VALUE(value) (double)((TK_YMAX-value)-p_gp->V.BbPlot.ybot)/(double)(p_gp->V.BbPlot.ytop-p_gp->V.BbPlot.ybot)

static double TkRealValue(GpTermEntry_Static * pThis, double value, int axIdx) 
{
	const GpAxis & r_ax = pThis->P_Gp->AxS[axIdx];
	return (r_ax.log) ? pow(r_ax.base, r_ax.Range.low + value*(r_ax.GetRange())) : r_ax.Range.low + value*(r_ax.GetRange());
}

static double TkValueX(GpTermEntry_Static * pThis, double value)
{
	const BoundingBox & r_bb = pThis->P_Gp->V.BbPlot;
	return (double)(value-r_bb.xleft)/(double)(r_bb.xright-r_bb.xleft);
}

static double TkValueY(GpTermEntry_Static * pThis, double value)
{
	const BoundingBox & r_bb = pThis->P_Gp->V.BbPlot;
	return (double)((TK_YMAX-value)-r_bb.ybot)/(double)(r_bb.ytop-r_bb.ybot);
}

static const char * tk_bind_init[TK_LANG_MAX] = {
	/* Tcl */
	"  $cv bind [\n  ",
	/* Perl */
	"  $cv->bind(\n  ",
	/* Python */
	"",
	/* Ruby */
	"",
	/* Rexx */
	"",
	/* Perl/Tkx */
	"  $cv->bind(\n  "
};

static const char * tk_line_segment_start[TK_LANG_MAX] = {
	/* Tcl */
	"  $cv create line\\\n",
	/* Perl */
	"  $cv->createLine(\n",
	/* Python */
	"\tcv.create_line(\\\n",
	/* Ruby */
	"  cl=TkcLine.new(cv,\\\n",
	/* Rexx */
	"  obj = TkCanvasLine(cv, ,\n",
	/* Perl/Tkx */
	"  $cv->create_line(\n"
};

static const char * tk_poly_point[TK_LANG_MAX] = {
	/* Tcl */
	"    [expr $cmx*%d/1000] [expr $cmy*%d/1000]\\\n",
	/* Perl */
	"    $cmx*%d/1000, $cmy*%d/1000,\n",
	/* Python */
	"\t\tcmx*%d/1000, cmy*%d/1000,\\\n",
	/* Ruby */
	"    cmx*%d/1000, cmy*%d/1000,\\\n",
	/* Rexx */
	"\tcmx*%d/1000, cmy*%d/1000, ,\n",
	/* Perl/Tkx */
	"    $cmx*%d/1000, $cmy*%d/1000,\n"
};

static const char * tk_line_segment_opt[TK_LANG_MAX] = {
	/* Tcl */
	"    -fill {%s} -width %.1f -capstyle %s -joinstyle %s",
	/* Perl */
	"    -fill => q{%s}, -width => %.1f, -capstyle => q{%s}, -joinstyle => q{%s}",
	/* Python */
	"\t\tfill='%s', width=%.1f, capstyle='%s', joinstyle='%s'",
	/* Ruby */
	"    'fill'=>'%s', 'width'=>%.1f, 'capstyle'=>'%s', 'joinstyle'=>'%s'",
	/* Rexx */
	"\t'-fill', '%s', '-width', '%.1f', '-capstyle', '%s', '-joinstyle', '%s'",
	/* Perl/Tkx */
	"    -fill => q{%s}, -width => %.1f, -capstyle => q{%s}, -joinstyle => q{%s}",
};

static const char * tk_line_segment_dash[TK_LANG_MAX] = {
	/* Tcl */
	" -dash {%s}",
	/* Perl */
	", -dash => q{%s}",
	/* Python */
	", dash=(%s)",
	/* Ruby */
	", 'dash'=>'%s'",
	/* Rexx */
	", '-dash', '%s'",
	/* Perl/Tkx */
	", -dash => q{%s}"
};

static const char * tk_line_segment_end[TK_LANG_MAX] = {
	/* Tcl */
	"\n",
	/* Perl */
	")",
	/* Python */
	")\n",
	/* Ruby */
	")\n",
	/* Rexx */
	")\n",
	/* Perl/Tkx */
	")"
};

static const char * tk_bind_main[TK_LANG_MAX] = {
	/* Tcl */
	"    ] <Button> \"gnuplot_xy %%W %f %f %f %f\\\n"
	"       %f %f %f %f",
	/* Perl */
	",\n    '<Button>' => "
	"[\\&gnuplot_xy, %f, %f, %f, %f,\n"
	"       %f, %f, %f, %f",
	/* Python */
	/* FIXME: how can one bind an event to a line segment in Python/TkCanvas ? */
	"",
	/* Ruby */
	"  cl.bind('Button', proc{ gnuplot_xy(%f, %f, %f, %f,\\\n"
	"    %f, %f, %f, %f",
	/* FIXME: Rexx interactive binding untested */
	"  call TkCanvasBind cv, obj, 'Button',  ,\n"
	"    'gnuplot_xy %f, %f, %f, %f,' ,\n"
	"    '%f, %f, %f, %f' ,\n"
	"    ",
	/* Perl/Tkx */
	",\n    '<Button>' => "
	"[\\&gnuplot_xy, %f, %f, %f, %f,\n"
	"       %f, %f, %f, %f"
};

static const char * tk_bind_f[TK_LANG_MAX] = {
	/* Tcl */
	" %f",
	/* Perl */
	", %f",
	/* Python */
	"",
	/* Ruby */
	", %f",
	/* Rexx */
	" || ', %f'",
	/* Perl/Tkx */
	", %f",
};

static const char * tk_bind_nil[TK_LANG_MAX] = {
	" {}", /* Tcl */
	", \"\"", /* Perl */
	"", /* Python */
	", ''", /* Ruby */
	" || ', \'\''", /* Rexx */
	", \"\"" /* Perl/Tkx */
};

static const char * tk_bind_end[TK_LANG_MAX] = {
	"\"\n", /* Tcl */
	"]);\n", /* Perl */
	"", /* Python */
	") })\n", /* Ruby */
	"\n", /* Rexx */
	"]);\n" /* Perl/Tkx */
};

static const char * tk_nobind[TK_LANG_MAX] = {
	"", /* Tcl */
	";\n", /* Perl */
	"", /* Python */
	"", /* Ruby */
	"", /* Rexx */
	";\n" /* Perl/Tkx */
};

TERM_PUBLIC void TK_vector(GpTermEntry_Static * pThis, uint x, uint y)
{
	if((x != tk_lastx) || (TK_YMAX - y != tk_lasty)) {
		/* vector without preceding move as e.g. in "with line lc variable" */
		if(tk_polygon_points == 0)
			TK_add_path_point(tk_lastx, tk_lasty);
		TK_add_path_point(x, TK_YMAX - y);
	}
	tk_lastx = x;
	tk_lasty = TK_YMAX - y;
	return;
}

static void TK_flush_line(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	int x, y, i;
	if(tk_in_path)
		tk_in_path = FALSE;
	if(tk_polygon_points < 2) {
		tk_polygon_points = 0;
		return;
	}
	/*
	 * this is the 1st part of the wrapper around the 'create line' command
	 * used to bind some actions to a line segment:
	 * bind {
	 *      normal create line command
	 *      } gnuplot_xy(some coordinates)
	 */
	/* prepare the binding mechanism */
	if(tk_interactive && !p_gp->Gg.Is3DPlot)
		fputs(tk_bind_init[tk_script_language], GPT.P_GpOutFile);
	// draw a line segment 
	fputs(tk_line_segment_start[tk_script_language], GPT.P_GpOutFile);
	for(i = 0; i < tk_polygon_points; i++)
		fprintf(GPT.P_GpOutFile, tk_poly_point[tk_script_language], tk_path_x[i], tk_path_y[i]);
	fprintf(GPT.P_GpOutFile, tk_line_segment_opt[tk_script_language], tk_color, tk_linewidth, tk_rounded ? "round" : "butt", tk_rounded ? "round" : "miter");
	if(tk_dashpattern[0])
		fprintf(GPT.P_GpOutFile, tk_line_segment_dash[tk_script_language], tk_dashpattern);
	fputs(tk_line_segment_end[tk_script_language], GPT.P_GpOutFile);

	/* finish the binding mechanism
	 * (which calls 'gnuplot_xy' for the line segment pointed to by
	 * the mouse cursor when a mouse button is pressed)
	 */
	x = tk_path_x[tk_polygon_points -1];
	y = tk_path_y[tk_polygon_points -1];
	if(tk_interactive && !p_gp->Gg.Is3DPlot) {
		fprintf(GPT.P_GpOutFile, tk_bind_main[tk_script_language],
		    TkRealValue(pThis, TkValueX(pThis, tk_lastx), FIRST_X_AXIS),
		    TkRealValue(pThis, TkValueY(pThis, tk_lasty), FIRST_Y_AXIS),
		    TkRealValue(pThis, TkValueX(pThis, tk_lastx), SECOND_X_AXIS),
		    TkRealValue(pThis, TkValueY(pThis, tk_lasty), SECOND_Y_AXIS),
		    TkRealValue(pThis, TkValueX(pThis, x), FIRST_X_AXIS),
		    TkRealValue(pThis, TkValueY(pThis, y), FIRST_Y_AXIS),
		    TkRealValue(pThis, TkValueX(pThis, x), SECOND_X_AXIS),
		    TkRealValue(pThis, TkValueY(pThis, y), SECOND_Y_AXIS));
		if(p_gp->AxS[FIRST_X_AXIS].log)
			fprintf(GPT.P_GpOutFile, tk_bind_f[tk_script_language], TkRealValue(pThis, TkValueX(pThis, 0.5 * (x + tk_lastx)), FIRST_X_AXIS));
		else
			fputs(tk_bind_nil[tk_script_language], GPT.P_GpOutFile);
		if(p_gp->AxS[FIRST_Y_AXIS].log)
			fprintf(GPT.P_GpOutFile, tk_bind_f[tk_script_language], TkRealValue(pThis, TkValueY(pThis, 0.5 * (y + tk_lasty)), FIRST_Y_AXIS));
		else
			fputs(tk_bind_nil[tk_script_language], GPT.P_GpOutFile);
		if(p_gp->AxS[SECOND_X_AXIS].log)
			fprintf(GPT.P_GpOutFile, tk_bind_f[tk_script_language], TkRealValue(pThis, TkValueX(pThis, 0.5 * (x + tk_lastx)), SECOND_X_AXIS));
		else
			fputs(tk_bind_nil[tk_script_language], GPT.P_GpOutFile);
		if(p_gp->AxS[SECOND_Y_AXIS].log)
			fprintf(GPT.P_GpOutFile, tk_bind_f[tk_script_language], TkRealValue(pThis, TkValueY(pThis, 0.5 * (y + tk_lasty)), SECOND_Y_AXIS));
		else
			fputs(tk_bind_nil[tk_script_language], GPT.P_GpOutFile);
		fputs(tk_bind_end[tk_script_language], GPT.P_GpOutFile);
	}
	else {
		fputs(tk_nobind[tk_script_language], GPT.P_GpOutFile);
	}
	tk_polygon_points = 0;
	tk_in_path = FALSE;
}

//#undef TK_REAL_VALUE
//#undef TK_X_VALUE
//#undef TK_Y_VALUE

TERM_PUBLIC int TK_text_angle(GpTermEntry_Static * pThis, int ang)
{
	tk_angle = ang;
	return TRUE;
}

static const char * tk_create_text_begin[TK_LANG_MAX] = {
	/* Tcl */
	"  $cv create text "
	"[expr $cmx * %d /1000] [expr $cmy * %d /1000]\\\n"
	"    -text {%s} -fill %s\\\n"
	"    -anchor %s",
	/* Perl */
	"  $cv->createText($cmx * %d / 1000, $cmy * %d / 1000,\n"
	"    -text => q{%s}, -fill => q{%s}, -anchor => '%s'",
	/* Python */
	"\tcv.create_text(cmx*%d/1000, cmy*%d/1000,\\\n"
	"\t\ttext='%s', fill='%s', anchor='%s'",
	/* Ruby */
	"  ct=TkcText.new(cv, cmx*%d/1000, cmy*%d/1000,\\\n"
	"    'text'=>'%s', 'fill'=>'%s', 'anchor'=>'%s'",
	/* Rexx */
	"  call TkCanvasText cv, cmx*%d/1000, cmy*%d/1000, ,\n"
	"\t'-text', '%s', '-fill', '%s', '-anchor', '%s'",
	/* Perl/Tkx */
	"  $cv->create_text($cmx * %d / 1000, $cmy * %d / 1000,\n"
	"    -text => q{%s}, -fill => q{%s}, -anchor => '%s'"
};

static const char * tk_create_text_font[TK_LANG_MAX] = {
	/* Tcl */
	" -font $font",
	/* Perl */
	",\n    -font => $font",
	/* Python */
	", font=gfont",
	/* Ruby */
	", 'font'=>font",
	/* Rexx */
	", '-font', font",
	/* Perl/Tkx */
	",\n    (defined $font ? (-font => $font) : ())"
};

static const char * tk_create_text_angle[TK_LANG_MAX] = {
	/* Tcl */
	" -angle %d",
	/* Perl */
	", -angle => %d",
	/* Python */
	", angle=%d",
	/* Ruby */
	", 'angle'=>%d",
	/* Rexx */
	", '-angle', %d",
	/* Perl/Tkx */
	", -angle => %d"
};

static const char * tk_tag[TK_LANG_MAX] = {
	/* Tcl */
	" -tags %s",
	/* Perl */
	", -tags => q{%s}",
	/* Python */
	", tags='%s'",
	/* Ruby */
	", 'tags'=>'%s'",
	/* Rexx */
	", '-tags', '%s'",
	/* Perl/Tkx */
	", -tags => q{%s}"
};

static const char * tk_create_text_end[TK_LANG_MAX] = {
	/* Tcl */
	"\n",
	/* Perl */
	");\n",
	/* Python */
	")\n",
	/* Ruby */
	")\n",
	/* Rexx */
	"\n",
	/* Perl/Tkx */
	");\n"
};

static void TK_put_noenhanced_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str)
{
	char * quoted_str = (char *)str;
	int i, newsize = 0;
	TK_flush_line(pThis);
	if(tk_script_language == TK_LANG_TCL) {
		quoted_str = escape_reserved_chars(str, "[]{}$;");
	}
	if((tk_script_language == TK_LANG_REXX) || (tk_script_language == TK_LANG_RUBY) || (tk_script_language == TK_LANG_PYTHON)) {
		/* Have to quote-protect "'" characters */
		for(i = 0; str[i] != '\0'; i++) {
			if(str[i] == '\'')
				newsize++;
			newsize++;
		}
		quoted_str = (char *)SAlloc::M(newsize + 1);
		for(i = 0, newsize = 0; str[i] != '\0'; i++) {
			if(str[i] == '\'')
				quoted_str[newsize++] = (tk_script_language == TK_LANG_REXX) ? '\'' : '\\';
			quoted_str[newsize++] = str[i];
		}
		quoted_str[newsize] = '\0';
	}
	y = TK_YMAX - y;
	fprintf(GPT.P_GpOutFile, tk_create_text_begin[tk_script_language], x, y, quoted_str, tk_color, tk_anchor);
	if(tk_next_text_use_font) {
		fputs(tk_create_text_font[tk_script_language], GPT.P_GpOutFile);
		tk_next_text_use_font = FALSE;
	}
	if(tk_angle != 0)
		fprintf(GPT.P_GpOutFile, tk_create_text_angle[tk_script_language], tk_angle);
	if(tk_boxed)
		fprintf(GPT.P_GpOutFile, tk_tag[tk_script_language], "boxedtext");
	fputs(tk_create_text_end[tk_script_language], GPT.P_GpOutFile);
	if(quoted_str != str)
		SAlloc::F(quoted_str);
}

static const char * tk_undef_font[TK_LANG_MAX] = {
	/* Tcl */
	"  catch {unset font}\n",
	/* Perl */
	"  undef $font;\n",
	/* Python */
	"",
	/* Ruby */
	"",
	/* Rexx */
	"  drop font\n",
	/* Perl/Tkx */
	"  undef $font;\n"
};

static const char * tk_set_font[TK_LANG_MAX] = {
	"  set font [font create -family {%s}", /* Tcl */
	"  $font = $cv->fontCreate(-family => q{%s}", /* Perl */
	"\tgfont = font.Font(family='%s'", /* Python */
	"  font = TkFont.new :family => '%s'", /* Ruby */
	"  font = TkFontCreate( , '-family', '%s'", /* Rexx */
	"  $font = Tkx::font_create(-family => q{%s}" /* Perl/Tkx */
};

static const char * tk_set_fsize[TK_LANG_MAX] = {
	/* Tcl */
	" -size %d",
	/* Perl */
	", -size => %d",
	/* Python */
	", size=%d",
	/* Ruby */
	", :size => %d",
	/* Rexx */
	", '-size', '%d'",
	/* Perl/Tkx */
	", -size => %d"
};

static const char * tk_set_fbold[TK_LANG_MAX] = {
	/* Tcl */
	" -weight bold",
	/* Perl */
	", -weight => q{bold}",
	/* Python */
	", weight='bold'",
	/* Ruby */
	", :weight => 'bold'",
	/* Rexx */
	", '-weight', 'bold'",
	/* Perl/Tkx */
	", -weight => q{bold}"
};

static const char * tk_set_fitalic[TK_LANG_MAX] = {
	/* Tcl */
	" -slant italic",
	/* Perl */
	", -slant => q{italic}",
	/* Python */
	", slant='italic'",
	/* Ruby */
	", :slant => 'italic'",
	/* Rexx */
	", '-slant', 'italic'",
	/* Perl/Tkx */
	", -slant => q{italic}"
};

static const char * tk_font_end[TK_LANG_MAX] = {
	/* Tcl */
	"]\n",
	/* Perl */
	");\n",
	/* Python */
	")\n",
	/* Ruby */
	"\n",
	/* Rexx */
	")\n",
	/* Perl/Tkx */
	");\n"
};

TERM_PUBLIC int TK_set_font(GpTermEntry_Static * pThis, const char * font)
{
	if(isempty(font)) {
		tk_next_text_use_font = FALSE;
		fputs(tk_undef_font[tk_script_language], GPT.P_GpOutFile);
	}
	else {
		int size = 0;
		size_t sep1 = strcspn(font, ",");
		size_t sep2 = strcspn(font, ":");
		size_t sep = MIN(sep1, sep2);
		bool isbold, isitalic;
		// extract font name 
		char * name = (char *)SAlloc::M(sep + 1);
		if(!name)
			return FALSE;
		strncpy(name, font, sep);
		name[sep] = '\0';
		// bold, italic 
		isbold = (strstr(font, ":Bold") != NULL);
		isitalic = (strstr(font, ":Italic") != NULL);
		// font size 
		if(sep1 < strlen(font))
			sscanf(&(font[sep1 + 1]), "%d", &size);
		fprintf(GPT.P_GpOutFile, tk_set_font[tk_script_language], name);
		if(size > 0)
			fprintf(GPT.P_GpOutFile, tk_set_fsize[tk_script_language], size);
		if(isbold)
			fputs(tk_set_fbold[tk_script_language], GPT.P_GpOutFile);
		if(isitalic)
			fputs(tk_set_fitalic[tk_script_language], GPT.P_GpOutFile);
		fputs(tk_font_end[tk_script_language], GPT.P_GpOutFile);
		tk_next_text_use_font = TRUE;

		SAlloc::F(name);
	}
	return TRUE;
}

TERM_PUBLIC void TK_enhanced_open(GpTermEntry_Static * pThis, const char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(overprint == 3) { /* save current position */
		fprintf(GPT.P_GpOutFile, "set xenh_save $xenh; set yenh_save $yenh;\n");
		return;
	}
	else if(overprint == 4) { /* restore saved position */
		fprintf(GPT.P_GpOutFile, "set xenh $xenh_save; set yenh $yenh_save;\n");
		return;
	}
	if(!tk_enhanced_opened_string) {
		bool isbold, isitalic;
		char * family, * sep;
		tk_enhanced_opened_string = TRUE;
		// Start new text fragment 
		p_gp->Enht.P_CurText = &p_gp->Enht.Text[0];
		// Scale fractional font height to vertical units of display 
		tk_enhanced_base = static_cast<int>(base * TK_HCHAR);
		// Keep track of whether we are supposed to show this string 
		tk_enhanced_show = showflag;
		// 0/1/2  no overprint / 1st pass / 2nd pass 
		tk_enhanced_overprint = overprint;
		// widthflag FALSE means do not update text position after printing 
		tk_enhanced_widthflag = widthflag;
		// set new font 
		family = sstrdup(fontname);
		sep = sstrchr(family, ':');
		ASSIGN_PTR(sep, '\0');
		isbold = (strstr(fontname, ":Bold") != NULL);
		isitalic = (strstr(fontname, ":Italic") != NULL);
		fprintf(GPT.P_GpOutFile, tk_set_font[tk_script_language], family);
		if(fontsize > 0)
			fprintf(GPT.P_GpOutFile, tk_set_fsize[tk_script_language], (int)(fontsize));
		if(isbold)
			fputs(tk_set_fbold[tk_script_language], GPT.P_GpOutFile);
		if(isitalic)
			fputs(tk_set_fitalic[tk_script_language], GPT.P_GpOutFile);
		fputs(tk_font_end[tk_script_language], GPT.P_GpOutFile);
		tk_next_text_use_font = TRUE;
		SAlloc::F(family);
	}
}

static const char * tk_enhanced_text_begin[TK_LANG_MAX] = {
	/* Tcl */
	"  set et [$cv create text $%s $%s\\\n"
	"    -text {%s} -fill %s\\\n"
	"    -anchor %s",
	/* Perl */
	"  $cv->createText($cmx * %d / 1000, $cmy * %d / 1000,\n"
	"    -text => q{%s}, -fill => q{%s}, -anchor => '%s'",
	/* Python */
	"\tcv.create_text(cmx*%d/1000, cmy*%d/1000,\\\n"
	"\t\ttext='%s', fill='%s', anchor='%s'",
	/* Ruby */
	"  ct=TkcText.new(cv, cmx*%d/1000, cmy*%d/1000,\\\n"
	"    'text'=>'%s', 'fill'=>'%s', 'anchor'=>'%s'",
	/* Rexx */
	"  call TkCanvasText cv, cmx*%d/1000, cmy*%d/1000, ,\n"
	"\t'-text', '%s', '-fill', '%s', '-anchor', '%s'",
	/* Perl/Tkx */
	"  $cv->create_text($cmx * %d / 1000, $cmy * %d / 1000,\n"
	"    -text => q{%s}, -fill => q{%s}, -anchor => '%s'"
};

static const char * tk_enhanced_text_end[TK_LANG_MAX] = {
	/* Tcl */
	"]\n",
	/* Perl */
	");\n",
	/* Python */
	")\n",
	/* Ruby */
	")\n",
	/* Rexx */
	"\n",
	/* Perl/Tkx */
	");\n"
};

TERM_PUBLIC void TK_enhanced_flush(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	const char * str = p_gp->Enht.Text; /* The fragment to print */
	if(!tk_enhanced_opened_string)
		return;
	*p_gp->Enht.P_CurText = '\0';
	/* print the string fragment in any case */
	/* NB: base expresses offset from current y pos */
	fprintf(GPT.P_GpOutFile, "set yenh [expr int($yenhb + %d)]\n",  (int)(-tk_enhanced_base/5 * cos(tk_angle * SMathConst::PiDiv180)));
	fprintf(GPT.P_GpOutFile, "set xenh [expr int($xenhb + %d)]\n",  (int)(-tk_enhanced_base/5 * sin(tk_angle * SMathConst::PiDiv180)));
	fprintf(GPT.P_GpOutFile, tk_enhanced_text_begin[tk_script_language], "xenh", "yenh", str, tk_color, tk_anchor);
	if(tk_next_text_use_font) {
		fputs(tk_create_text_font[tk_script_language], GPT.P_GpOutFile);
		tk_next_text_use_font = FALSE;
	}
	if(!tk_boxed)
		fprintf(GPT.P_GpOutFile, tk_tag[tk_script_language], "enhancedtext");
	else
		fprintf(GPT.P_GpOutFile, tk_tag[tk_script_language], "boxedtext");
	fputs(tk_enhanced_text_end[tk_script_language], GPT.P_GpOutFile);
	if(!tk_enhanced_widthflag)
		
		; // don't update position 
	else if(tk_enhanced_overprint == 1) {
		// First pass of overprint, leave position in center of fragment 
		// fprintf(GPT.P_GpOutFile, "incr xenh [expr ([lindex [$cv bbox $et] 2] - [lindex [$cv bbox $et] 0]) / 2]\n");
		fprintf(GPT.P_GpOutFile, "set width [expr ([lindex [$cv bbox $et] 2] - [lindex [$cv bbox $et] 0])]\n");
		fprintf(GPT.P_GpOutFile, "incr xenhb [expr int($width * %f)]\n", +cos(tk_angle * SMathConst::PiDiv180) / 2);
		fprintf(GPT.P_GpOutFile, "incr yenhb [expr int($width * %f)]\n", -sin(tk_angle * SMathConst::PiDiv180) / 2);
	}
	else {
		// Normal case is to update position to end of fragment 
		// fprintf(GPT.P_GpOutFile, "set xenh [lindex [$cv bbox $et] 2]\n"); 
		fprintf(GPT.P_GpOutFile, "set width [expr ([lindex [$cv bbox $et] 2] - [lindex [$cv bbox $et] 0])]\n");
		fprintf(GPT.P_GpOutFile, "incr xenhb [expr int($width * %f)]\n", +cos(tk_angle * SMathConst::PiDiv180));
		fprintf(GPT.P_GpOutFile, "incr yenhb [expr int($width * %f)]\n", -sin(tk_angle * SMathConst::PiDiv180));
	}
	if(tk_angle != 0)
		fprintf(GPT.P_GpOutFile, "$cv itemconfigure $et -angle %d\n", tk_angle);
	if(!tk_enhanced_show)
		fprintf(GPT.P_GpOutFile, "$cv delete $et\n");
	tk_enhanced_opened_string = FALSE;
}

static void TK_put_enhanced_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str)
{
	GnuPlot * p_gp = pThis->P_Gp;
	// Set up global variables needed by enhanced_recursion() 
	p_gp->Enht.FontScale = 1.0;
	strncpy(p_gp->Enht.EscapeFormat, "%c", sizeof(p_gp->Enht.EscapeFormat));
	tk_enhanced_opened_string = FALSE;
	tk_lastx = x;
	tk_lasty = TK_YMAX - y;
	fprintf(GPT.P_GpOutFile, "set xenh0 [expr $cmx * %d /1000]; set yenh0 [expr $cmy * %d /1000];\n", x, TK_YMAX - y);
	fprintf(GPT.P_GpOutFile, "set xenh $xenh0; set yenh $yenh0;\n");
	fprintf(GPT.P_GpOutFile, "set xenhb $xenh0; set yenhb $yenh0;\n");
	strcpy(tk_anchor, "w");
	// Set the recursion going. We say to keep going until a
	// closing brace, but we don't really expect to find one.
	// If the return value is not the nul-terminator of the
	// string, that can only mean that we did find an unmatched
	// closing brace in the string. We increment past it (else
	// we get stuck in an infinite loop) and try again.
	while(*(str = enhanced_recursion(pThis, str, TRUE, "" /* font */, 10 /* size */, 0.0, TRUE, TRUE, 0))) {
		pThis->enhanced_flush(pThis);
		// I think we can only get here if *str == '}' 
		p_gp->EnhErrCheck(str);
		if(!*++str)
			break; /* end of string */
		// else carry on and process the rest of the string 
	}
	if(tk_justify == RIGHT)
		fprintf(GPT.P_GpOutFile, "$cv move enhancedtext [expr ($xenh0 - $xenhb)] [expr ($yenh0 - $yenhb)]\n");
	else if(tk_justify == CENTRE)
		fprintf(GPT.P_GpOutFile, "$cv move enhancedtext [expr ($xenh0 - $xenhb)/2] [expr ($yenh0 - $yenhb)/2]\n");
	fprintf(GPT.P_GpOutFile, "$cv dtag enhancedtext\n");
}

TERM_PUBLIC void TK_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str)
{
	if(!isempty(str)) {
		// If no enhanced text processing is needed, we can use the plain  
		// vanilla put_text() routine instead of the fancy recursive one. 
		// FIXME: enhanced text only implemented for Tcl 
		if(!(pThis->flags & TERM_ENHANCED_TEXT) || pThis->P_Gp->Enht.Ignore || !strpbrk(str, "{}^_@&~") || (tk_script_language != TK_LANG_TCL))
			TK_put_noenhanced_text(pThis, x, y, str);
		else
			TK_put_enhanced_text(pThis, x, y, str);
	}
}

TERM_PUBLIC int TK_justify_text(GpTermEntry_Static * pThis, enum JUSTIFY anchor)
{
	int return_value;
	switch(anchor) {
		case RIGHT:
		    strcpy(tk_anchor, "e");
		    return_value = TRUE;
		    break;
		case CENTRE:
		    strcpy(tk_anchor, "center");
		    return_value = TRUE;
		    break;
		case LEFT:
		    strcpy(tk_anchor, "w");
		    return_value = TRUE;
		    break;
		default:
		    strcpy(tk_anchor, "w");
		    return_value = FALSE;
	}
	tk_justify = anchor;
	return return_value;
}

TERM_PUBLIC void TK_point(GpTermEntry_Static * pThis, uint x, uint y, int point)
{
	TK_flush_line(pThis);
	if(point >= 0) {
		GnuPlot::DoPoint(pThis, x, y, point);
	}
	else {
		// Emulate dots by a line of length 1 
		TK_dashtype(pThis, DASHTYPE_SOLID, NULL);
		TK_move(pThis, x, y);
		TK_vector(pThis, x, y+1);
		TK_flush_line(pThis);
	}
}

#if (0) /* private arrow routine does not handle HEAD_ONLY */
static char * tk_line_arrow[TK_LANG_MAX] = {
	/* Tcl */
	" -arrow %s",
	/* Perl */
	", -arrow => q{%s}",
	/* Python */
	", arrow='%s'",
	/* Ruby */
	", 'arrow'=>'%s'",
	/* Rexx */
	", '-arrow', '%s'",
	/* Perl/Tkx */
	", -arrow => q{%s}"
};

static char * tk_line_arrowshape[TK_LANG_MAX] = {
	/* Tcl */
	" -arrowshape {%d %d %d}",
	/* Perl */
	", -arrowshape => [%d, %d, %d]",
	/* Python */
	", arrowshape=(%d, %d, %d)",
	/* Ruby */
	", 'arrowshape'=>[%d, %d, %d]",
	/* Rexx */
	", '-arrowshape', '%d %d %d'",
	/* Perl/Tkx */
	", -arrowshape => [%d, %d, %d]"
};

TERM_PUBLIC void TK_arrow(GpTermEntry_Static * pThis, uint usx, uint usy, uint uex, uint uey, int head)
{
	/* NOHEAD = 0, END_HEAD = 1, BACKHEAD = 2, BOTH_HEADS = 3, HEADS_ONLY = 4 */
	const char * arrow[4] = { "none", "last", "first", "both" };
	/* NOTE: we really need integer arguments. Why are the arguments unsigned in the first place? */
	int sx = (int)usx;
	int sy = (int)usy;
	int ex = (int)uex;
	int ey = (int)uey;
	TK_flush_line();
	if(GPT.CArw.HeadFilled >= AS_FILLED) { // AS_FILLED, AS_NOBORDER 
		fputs(tk_line_segment_start[tk_script_language], GPT.P_GpOutFile);
		fprintf(GPT.P_GpOutFile, tk_poly_point[tk_script_language], sx, TK_YMAX - sy);
		fprintf(GPT.P_GpOutFile, tk_poly_point[tk_script_language], ex, TK_YMAX - ey);
		fprintf(GPT.P_GpOutFile, tk_line_segment_opt[tk_script_language], tk_color, tk_linewidth, tk_rounded ? "round" : "butt", tk_rounded ? "round" : "miter");
		if(GPT.CArw.HeadLength > 0) {
			// This should exactly mimic the behaviour of GnuPlot::DoArrow() 
			int width   = sin(GPT.CArw.HeadAngle * SMathConst::PiDiv180) * GPT.CArw.HeadLength;
			int tiplen  = cos(GPT.CArw.HeadAngle * SMathConst::PiDiv180) * GPT.CArw.HeadLength;
			int backlen = width / tan(GPT.CArw.HeadBackAngle * SMathConst::PiDiv180);
			int length  = tiplen - backlen;
			// impose lower limit on thickness of tips 
			if(4 * length < tiplen) length = tiplen / 4;
			if(length <= 1) length = 2;
			if(tiplen < 1) tiplen = 1;

			fprintf(GPT.P_GpOutFile, tk_line_arrow[tk_script_language], arrow[ (head & BOTH_HEADS) ]);
			fprintf(GPT.P_GpOutFile, tk_line_arrowshape[tk_script_language], length, tiplen, width);
		}
		else if(head != NOHEAD) {
			double dx = sx - ex;
			double dy = sy - ey;
			double len_arrow = sqrt(dx * dx + dy * dy);
			double len_tic = ((double)(pThis->TicH + pThis->TicV)) / 2.0;
			double head_coeff = MAX(len_tic * HEAD_SHORT_LIMIT, MIN(HEAD_COEFF * len_arrow, len_tic * HEAD_LONG_LIMIT));
			int length = (int)(COS15 * head_coeff);
			int width  = (int)(SIN15 * head_coeff);

			fprintf(GPT.P_GpOutFile, tk_line_arrow[tk_script_language], arrow[ (head & BOTH_HEADS) ]);
			fprintf(GPT.P_GpOutFile, tk_line_arrowshape[tk_script_language], length, length, width);
		}
		if(tk_dashpattern[0])
			fprintf(GPT.P_GpOutFile, tk_line_segment_dash[tk_script_language], tk_dashpattern);
		fputs(tk_line_segment_end[tk_script_language], GPT.P_GpOutFile);
		fputs(tk_nobind[tk_script_language], GPT.P_GpOutFile);
	}
	else { /* AS_NOFILL, AS_EMPTY */
		/* fall back to internal routine since we cannot do non-filled arrows */
		GnuPlot::DoArrow(sx, sy, ex, ey, head);
	}
}

#endif

static const char * tk_endblock[TK_LANG_MAX] = {
	/* Tcl */
	"}\n",
	/* Perl */
	"};\n",
	/* Python */
	"",
	/* Ruby */
	"end\n",
	/* Rexx */
	"return 0\n\n",
	/* Perl/Tkx */
	"};\n"
};

static const char * tk_info_procs[TK_LANG_MAX] = {
	/* Tcl */
	"proc gnuplot_plotarea {} {\n"
	"  return {%d %d %d %d}\n"
	"}\n"
	"proc gnuplot_axisranges {} {\n"
	"  return {%f %f %f %f\n"
	"          %f %f %f %f}\n"
	"}\n",

	/* Perl */
	"sub gnuplot_plotarea {\n"
	"  return (%d, %d, %d, %d);\n"
	"};\n"
	"sub gnuplot_axisranges {\n"
	"  return (%f, %f, %f, %f,\n"
	"          %f, %f, %f, %f);\n"
	"};\n",

	/* Python */
	"def gnuplot_plotarea():\n"
	"\treturn (%d, %d, %d, %d)\n"
	"def gnuplot_axisranges():\n"
	"\treturn (%f, %f, %f, %f,\\\n"
	"\t        %f, %f, %f, %f)\n",

	/* Ruby */
	"def gnuplot_plotarea()\n"
	"  return [%d, %d, %d, %d]\n"
	"end\n"
	"def gnuplot_axisranges()\n"
	"  return [%f, %f, %f, %f,\\\n"
	"          %f, %f, %f, %f]\n"
	"end\n",

	/* Rexx */
	"gnuplot_plotarea: procedure\n"
	"return '%d %d %d %d'\n"
	"\n"
	"gnuplot_axisranges: procedure\n"
	"return '%f %f %f %f ' || ,\n"
	"       '%f %f %f %f'\n"
	"\n",

	/* Perl/Tkx */
	"sub gnuplot_plotarea {\n"
	"  return (%d, %d, %d, %d);\n"
	"};\n"
	"sub gnuplot_axisranges {\n"
	"  return (%f, %f, %f, %f,\n"
	"          %f, %f, %f, %f);\n"
	"};\n"
};

static const char * tk_gnuplot_xy[] = {
	/* Tcl */
	"proc gnuplot_xy {win x1s y1s x2s y2s x1e y1e x2e y2e x1m y1m x2m y2m} {\n"
	"  if {([llength [info commands user_gnuplot_coordinates]])} {\n"
	"    set id [$win find withtag current]\n"
	"    user_gnuplot_coordinates $win $id \\\n"
	"      $x1s $y1s $x2s $y2s $x1e $y1e $x2e $y2e $x1m $y1m $x2m $y2m\n"
	"  } else {\n"
	"    if {[string length $x1m]>0} {puts -nonewline \" $x1m\"\n"
	"      } else {puts -nonewline \" [expr 0.5*($x1s+$x1e)]\"}\n"
	"    if {[string length $y1m]>0} {puts -nonewline \" $y1m\"\n"
	"      } else {puts -nonewline \" [expr 0.5*($y1s+$y1e)]\"}\n"
	"    if {[string length $x2m]>0} {puts -nonewline \" $x2m\"\n"
	"      } else {puts -nonewline \" [expr 0.5*($x2s+$x2e)]\"}\n"
	"    if {[string length $y2m]>0} {puts            \" $y2m\"\n"
	"      } else {puts            \" [expr 0.5*($y2s+$y2e)]\"}\n"
	"  }\n"
	"}\n",

	/* Perl */
	"sub gnuplot_xy {\n"
	"  my ($win, $x1s, $y1s, $x2s, $y2s, $x1e, $y1e, $x2e, $y2e,\n"
	"      $x1m, $y1m, $x2m, $y2m) = @_;\n"
	"  if (defined &user_gnuplot_coordinates) {\n"
	"    my $id = $win->find('withtag', 'current');\n"
	"    user_gnuplot_coordinates $win, $id, $x1s, $y1s, $x2s, $y2s,\n"
	"      $x1e, $y1e, $x2e, $y2e, $x1m, $y1m, $x2m, $y2m\n"
	"  } else {\n"
	"    print \" \", (length($x1m)>0 ? \"$x1m\": 0.5*($x1s+$x1e));\n"
	"    print \" \", (length($y1m)>0 ? \"$y1m\": 0.5*($y1s+$y1e));\n"
	"    print \" \", (length($x2m)>0 ? \"$x2m\": 0.5*($x2s+$x2e));\n"
	"    print \" \", (length($y2m)>0 ? \"$y2m\": 0.5*($y2s+$y2e));\n"
	"    print \"\\n\"\n"
	"  }\n"
	"};\n",
	/* Python */
	/* FIXME: how can one bind an event to a line segment in Python/TkCanvas ? */
	"",

	/* Ruby */
	"def gnuplot_xy(x1s, y1s, x2s, y2s, x1e, y1e, x2e, y2e,\n"
	"      x1m, y1m, x2m, y2m)\n"
	"    print \" \", x1m!='' ? x1m : 0.5*(x1s+x1e)\n"
	"    print \" \", y1m!='' ? y1m : 0.5*(y1s+y1e)\n"
	"    print \" \", x2m!='' ? x2m : 0.5*(x2s+x2e)\n"
	"    print \" \", y2m!='' ? y2m : 0.5*(y2s+y2e)\n"
	"    print \"\\n\""
	"end\n",

	/* Rexx */
	/* FIXME: Rexx gnuplot_xy is untested */
	"gnuplot_xy: procedure\n"
	"  x1s=arg(1); y1s=arg(2); x2s=arg(3); y2s=arg(4);\n"
	"  x1e=arg(5); y1e=arg(6); x2e=arg(7); y2e=arg(8);\n"
	"  x1m=arg(9); y1m=arg(10); x2m=arg(11); y2m=arg(12);\n"
	"\n"
	"  outstr = ''\n"
	"  if (x1m\\='') then outstr = outstr x1m\n"
	"  else outstr = outstr (0.5*(x1s+x1e))\n"
	"  if (y1m\\='') then outstr = outstr y1m\n"
	"  else outstr = outstr (0.5*(y1s+y1e))\n"
	"  if (x2m\\='') then outstr = outstr x2m\n"
	"  else outstr = outstr (0.5*(x2s+x2e))\n"
	"  if (y2m\\='') then outstr = outstr y2m\n"
	"  else outstr = outstr (0.5*(y2s+y2e))\n"
	"\n"
	"  call lineout ,outstr\n"
	"return\n\n,"

	/* Perl/Tkx */
	"sub gnuplot_xy {\n"
	"  my ($win, $x1s, $y1s, $x2s, $y2s, $x1e, $y1e, $x2e, $y2e,\n"
	"      $x1m, $y1m, $x2m, $y2m) = @_;\n"
	"  if (defined &user_gnuplot_coordinates) {\n"
	"    my $id = $win->find('withtag', 'current');\n"
	"    user_gnuplot_coordinates $win, $id, $x1s, $y1s, $x2s, $y2s,\n"
	"      $x1e, $y1e, $x2e, $y2e, $x1m, $y1m, $x2m, $y2m\n"
	"  } else {\n"
	"    print \" \", (length($x1m)>0 ? \"$x1m\": 0.5*($x1s+$x1e));\n"
	"    print \" \", (length($y1m)>0 ? \"$y1m\": 0.5*($y1s+$y1e));\n"
	"    print \" \", (length($x2m)>0 ? \"$x2m\": 0.5*($x2s+$x2e));\n"
	"    print \" \", (length($y2m)>0 ? \"$y2m\": 0.5*($y2s+$y2e));\n"
	"    print \"\\n\"\n"
	"  }\n"
	"};\n"
};

TERM_PUBLIC void TK_text(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	/*
	 * when switching back to text mode some procedures are generated which
	 * return important information about plotarea size and axis ranges:
	 * 'gnuplot_plotarea'
	 *     returns the plotarea size in tkcanvas units
	 * 'gnuplot_axisranges'
	 *     returns the min. and max. values of the axis
	 *     (these are essentially needed to set the size of the canvas
	 *     when the axis scaling is important.
	 * 'gnuplot_xy'
	 *     contains actions bound to line segments the mouse is pointing
	 *     to (see the above 'TK_vector' code):
	 *     if the user has defined a procedure named 'user_gnuplot_coordinates'
	 *     then 'gnuplot_xy' calls this procedure, otherwise is writes the
	 *     coordinates of the line segment the mouse cursor is pointing to
	 *     to standard output.
	 */
	TK_flush_line(pThis);
	fputs(tk_endblock[tk_script_language], GPT.P_GpOutFile);
	if(!p_gp->Gg.Is3DPlot)
		fprintf(GPT.P_GpOutFile, tk_info_procs[tk_script_language],
		    p_gp->V.BbPlot.xleft, p_gp->V.BbPlot.xright, TK_YMAX - p_gp->V.BbPlot.ytop, TK_YMAX - p_gp->V.BbPlot.ybot,
		    p_gp->AxS[FIRST_X_AXIS].Range.low,  p_gp->AxS[FIRST_X_AXIS].Range.upp, p_gp->AxS[FIRST_Y_AXIS].Range.low,  p_gp->AxS[FIRST_Y_AXIS].Range.upp,
		    p_gp->AxS[SECOND_X_AXIS].Range.low, p_gp->AxS[SECOND_X_AXIS].Range.upp, p_gp->AxS[SECOND_Y_AXIS].Range.low, p_gp->AxS[SECOND_Y_AXIS].Range.upp);
	if(tk_interactive)
		fputs(tk_gnuplot_xy[tk_script_language], GPT.P_GpOutFile);
	if(tk_standalone && (tk_script_language != TK_LANG_REXX))
		fprintf(GPT.P_GpOutFile, tk_standalone_init[tk_script_language], tk_width, tk_height);
	fflush(GPT.P_GpOutFile);
}

static const char * tk_rectangle[TK_LANG_MAX] = {
	/* Tcl */
	"  $cv create rectangle\\\n"
	"    [expr $cmx*%d/1000] [expr $cmy*%d/1000]\\\n"
	"    [expr $cmx*%d/1000] [expr $cmy*%d/1000]\\\n"
	"    -fill %s -outline {} -stipple {%s}\n",
	/* Perl */
	"  $cv->createRectangle("
	"$cmx*%d/1000, $cmy*%d/1000, $cmx*%d/1000, $cmy*%d/1000,\n"
	"    -fill => q{%s}, -outline => q{}, -stipple => q{%s});\n",
	/* Python */
	"\tcv.create_rectangle(cmx*%d/1000, cmy*%d/1000, cmx*%d/1000, cmy*%d/1000,\\\n"
	"\t\tfill='%s', outline='', stipple='%s')\n",
	/* Ruby */
	"  cr=TkcRectangle.new("
	"cv, cmx*%d/1000, cmy*%d/1000, cmx*%d/1000, cmy*%d/1000,\\\n"
	"    'fill'=>'%s', 'outline'=>'', 'stipple'=>'%s')\n",
	/* Rexx */
	"  obj = TkCanvasRectangle("
	"cv, cmx*%d/1000, cmy*%d/1000, cmx*%d/1000, cmy*%d/1000, ,\n"
	"\t'-fill', '%s', '-outline', '', '-stipple', '%s')\n",
	/* Perl/Tkx */
	"  $cv->create_rectangle("
	"$cmx*%d/1000, $cmy*%d/1000, $cmx*%d/1000, $cmy*%d/1000,\n"
	"    -fill => q{%s}, -outline => q{}, -stipple => q{%s});\n"
};

static void TK_rectangle(int x1, int y1, int x2, int y2, const char * color, const char * stipple)
{
	SETIFZ(color, "");
	SETIFZ(stipple, "");
	fprintf(GPT.P_GpOutFile, tk_rectangle[tk_script_language], x1, y1, x2, y2, color, stipple);
}

TERM_PUBLIC void TK_fillbox(GpTermEntry_Static * pThis, int style, uint x, uint y, uint w, uint h)
{
	const char * stipple = "";
	const char * color = tk_color;
	TK_flush_line(pThis);
	switch(style & 0x0f) {
		case FS_SOLID:
		case FS_TRANSPARENT_SOLID: {
		    int density = style >> 4;
		    if(density < 20)
			    stipple = "gray12";
		    else if(density < 38)
			    stipple = "gray25";
		    else if(density < 53)
			    stipple = "gray50";
		    else if(density < 88)
			    stipple = "gray75";
		    else
			    stipple = "";
		    break;
	    }
		case FS_PATTERN:
		case FS_TRANSPARENT_PATTERN: {
		    const char * patterns[] = {"gray50", "gray25", "gray12", "gray75", ""};
		    int pattern = style >> 4;
		    stipple = (char *)patterns[pattern % 5];
		    break;
	    }
		case FS_EMPTY:
		    color = tk_background[0] ? tk_background : "white";
		    break;
		case FS_DEFAULT:
		default:
		    break;
	}
	TK_rectangle(x, TK_YMAX - y, x + w, TK_YMAX - y - h, color, stipple);
}

static const char * tk_poly_begin[TK_LANG_MAX] = {
	/* Tcl */
	"  $cv create polygon\\\n",
	/* Perl */
	"  $cv->createPolygon(\n",
	/* Python */
	"\tcv.create_polygon(\\\n",
	/* Ruby */
	"  cp=TkcPolygon.new(cv,\\\n",
	/* Rexx */
	"  obj = TkCanvasPolygon(cv, ,\n",
	/* Perl/Tkx */
	"  $cv->create_polygon(\n"
};

static const char * tk_poly_end[TK_LANG_MAX] = {
	/* Tcl */
	"    -fill %s -outline {}\n",
	/* Perl */
	"    -fill => q{%s}, -outline => q{});\n",
	/* Python */
	"\t\tfill='%s', outline='')\n",
	/* Ruby */
	"    'fill'=>'%s', 'outline'=>'')\n",
	/* Rexx */
	"\t'-fill', '%s', '-outline', '')\n",
	/* Perl/Tkx */
	"    -fill => q{%s});\n",
};

TERM_PUBLIC void TK_filled_polygon(GpTermEntry_Static * pThis, int points, gpiPoint * corners)
{
	int i;
	TK_flush_line(pThis);
	// avoid duplicate last point 
	if((points > 2) && (corners[0].x == corners[points-1].x) && (corners[0].y == corners[points-1].y))
		points--;
	fputs(tk_poly_begin[tk_script_language], GPT.P_GpOutFile);
	for(i = 0; i < points; i++)
		fprintf(GPT.P_GpOutFile, tk_poly_point[tk_script_language], corners[i].x, TK_YMAX - corners[i].y);
	fprintf(GPT.P_GpOutFile, tk_poly_end[tk_script_language], tk_color);
}

TERM_PUBLIC void TK_path(GpTermEntry_Static * pThis, int p)
{
	if(p == 0) { // start new path 
		TK_flush_line(pThis);
		tk_in_path = TRUE;
		tk_polygon_points = 0;
		FPRINTF((stderr, "tkcanvas: newpath\n"));
	}
	else if(p == 1) { /* close path */
		int i;
		FPRINTF((stderr, "tkcanvas: closepath: %i points\n", tk_polygon_points));
		if(tk_polygon_points > 1) {
			fputs(tk_line_segment_start[tk_script_language], GPT.P_GpOutFile);
			for(i = 0; i < tk_polygon_points; i++)
				fprintf(GPT.P_GpOutFile, tk_poly_point[tk_script_language], tk_path_x[i], tk_path_y[i]);
			fprintf(GPT.P_GpOutFile, tk_line_segment_opt[tk_script_language], tk_color, tk_linewidth,
			    tk_rounded ? "round" : "butt", tk_rounded ? "round" : "miter");
			if(tk_dashpattern[0])
				fprintf(GPT.P_GpOutFile, tk_line_segment_dash[tk_script_language], tk_dashpattern);
			fputs(tk_line_segment_end[tk_script_language], GPT.P_GpOutFile);
			fputs(tk_nobind[tk_script_language], GPT.P_GpOutFile);
		}
		tk_in_path = FALSE;
		tk_polygon_points = 0;
	}
}

static void TK_add_path_point(int x, int y)
{
	if(tk_polygon_points >= tk_maxpath) {
		tk_maxpath += 10;
		tk_path_x = (int *)SAlloc::R(tk_path_x, tk_maxpath * sizeof(int));
		tk_path_y = (int *)SAlloc::R(tk_path_y, tk_maxpath * sizeof(int));
	}
	tk_path_x[tk_polygon_points] = x;
	tk_path_y[tk_polygon_points] = y;
	tk_polygon_points++;
	FPRINTF((stderr, "tkcanvas: new polygon point: %i %i\n", x, y));
}

#ifdef WRITE_PNG_IMAGE
TERM_PUBLIC void TK_image(GpTermEntry_Static * pThis, uint m, uint n, coordval * image, gpiPoint * corner, t_imagecolor color_mode)
{
	int width  = ABS(corner[0].x - corner[1].x);
	int height = ABS(corner[0].y - corner[1].y);
	char * basename = "gp";
	char * fname;
	TK_flush_line();
	// Write the image to a png file 
	fname = (char *)SAlloc::M(strlen(basename) + 16);
	sprintf(fname, "%s_image_%02d.png", basename, ++tk_image_counter);
	write_png_image(pThis, m, n, image, color_mode, fname);
	// FIXME: Only Tcl support, needs external `rescale` command. 
	fprintf(GPT.P_GpOutFile, "set image%d [image create photo -file {%s}]\n", tk_image_counter, fname);
	fprintf(GPT.P_GpOutFile, "set image%dr [resize $image%d [expr $cmx*%d/1000] [expr $cmy*%d/1000]]\n",
	    tk_image_counter, tk_image_counter, width, height);
	fprintf(GPT.P_GpOutFile, "$cv create image [expr $cmx*%d/1000] [expr $cmy*%d/1000] -anchor nw -image $image%dr\n",
	    corner[0].x, TK_YMAX - corner[0].y, tk_image_counter);
}
#endif

static const char * tk_box[TK_LANG_MAX] = {
	/* Tcl */
	"  $cv raise boxedtext [$cv create rectangle [$cv bbox boxedtext] -fill {%s} -outline {%s}]\n",
	/* Perl */
	"  $cv->raise(q{boxedtext}, $cv->createRectangle($cv->bbox(q{boxedtext}),\n"
	"    -fill => q{%s}, -outline => q{%s}));\n",
	/* Python */
	"\tcv.tag_raise('boxedtext', cv.create_rectangle(cv.bbox('boxedtext'),\\\n"
	"\t\tfill='%s', outline='%s'))\n",
	/* Ruby */
	"  cr=cv.raise('boxedtext', TkcRectangle.new(cv, cv.bbox('boxedtext'),\\\n"
	"    'fill'=>'%s', 'outline'=>'%s'))\n",
	/* Rexx */
	"", /* TkCanvasRaise is not available */
/*
    "  obj = TkCanvasRaise(cv, 'boxedtext', TkCanvasRectangle(TkBbox(cv, 'boxedtext'), ,\n"
    "\t'-fill', '%s', '-outline', '%s'))\n",
 */
	/* Perl/Tkx */
	"  $cv->raise(q{boxedtext}, $cv->create_rectangle($cv->bbox(q{boxedtext}),\n"
	"    -fill => q{%s}, -outline => q{%s}));\n"
};

static const char * tk_box_finish[TK_LANG_MAX] = {
	/* Tcl */
	"  $cv dtag boxedtext\n",
	/* Perl */
	"  $cv->dtag(q{boxedtext});\n",
	/* Python */
	"\tcv.dtag('boxedtext')\n",
	/* Ruby */
	"  cr=cv.dtag('boxedtext')\n",
	/* Rexx */
	"  obj = TkCanvasDTag(cv, 'boxedtext')\n",
	/* Perl/Tkx */
	"  $cv->dtag(q{boxedtext});\n"
};

TERM_PUBLIC void TK_boxed_text(GpTermEntry_Static * pThis, uint x, uint y, int option)
{
	switch(option) {
		case TEXTBOX_INIT:
		    tk_boxed = TRUE;
		    break;
		case TEXTBOX_BACKGROUNDFILL:
		    fprintf(GPT.P_GpOutFile, tk_box[tk_script_language], tk_color, "");
		    break;
		case TEXTBOX_GREY:
		    fprintf(GPT.P_GpOutFile, tk_box[tk_script_language], "grey75", "");
		    break;
		case TEXTBOX_OUTLINE:
		    fprintf(GPT.P_GpOutFile, tk_box[tk_script_language], "", "black");
		// @fallthrough, this also ends text box mode 
		case TEXTBOX_FINISH:
		    fputs(tk_box_finish[tk_script_language], GPT.P_GpOutFile);
		    tk_boxed = FALSE;
		    break;
		case TEXTBOX_MARGINS:
		    // FIXME: cannot resize margins 
		    break;
	}
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE

TERM_TABLE_START(tkcanvas)
	"tkcanvas", 
	"Tk canvas widget",
	TK_XMAX, 
	TK_YMAX, 
	TK_VCHAR, 
	TK_HCHAR, 
	TK_VTIC, 
	TK_HTIC,
	TK_options, 
	TK_init, 
	TK_reset,
	TK_text, 
	GnuPlot::NullScale, 
	TK_graphics, 
	TK_move, 
	TK_vector,
	TK_linetype, 
	TK_put_text, 
	GnuPlot::NullTextAngle,
	TK_justify_text, 
	TK_point, 
	GnuPlot::DoArrow, 
	TK_set_font,
	NULL /* set_pointsize */,
	TERM_CAN_MULTIPLOT |  TERM_ENHANCED_TEXT,
	/* FIXME: Options not yet implemented */
	/* TERM_CAN_DASH | TERM_LINEWIDTH | TERM_FONTSCALE, */
	NULL /* suspend */, 
	NULL /* resume */,
	TK_fillbox, 
	TK_linewidth,
	#ifdef USE_MOUSE
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL,
	#endif
	TK_make_palette, 
	NULL, 
	TK_color,
	TK_filled_polygon, 
	NULL /* image */,
	TK_enhanced_open, 
	TK_enhanced_flush, 
	do_enh_writec,
	NULL /* layer */, 
	TK_path,
	0.0,
	NULL /* hypertext */,
	TK_boxed_text,
	NULL, 
	TK_dashtype 
TERM_TABLE_END(tkcanvas)
#undef LAST_TERM
#define LAST_TERM tkcanvas

#endif /* TERM_TABLE */
#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
START_HELP(tkcanvas)
"1 tkcanvas",
"?commands set terminal tkcanvas",
"?set terminal tkcanvas",
"?set term tkcanvas",
"?terminal tkcanvas",
"?term tkcanvas",
"?tkcanvas",
" This terminal driver generates Tk canvas widget commands in one of the",
" following scripting languages: Tcl (default), Perl, Python, Ruby, or REXX.",
"",
" Syntax:",
"       set terminal tkcanvas {tcl | perl | perltkx | python | ruby | rexx}",
"                             {standalone | input}",
"                             {interactive}",
"                             {rounded | butt}",
"                             {nobackground | background <rgb color>}",
"                             {{no}rottext}",
"                             {size <width>,<height>}",
"                             {{no}enhanced}",
"                             {externalimages | pixels}",
"",
" Execute the following sequence of Tcl/Tk commands to display the result:",
"",
"       package require Tk",
"       # the following two lines are only required to support external images",
"       package require img::png",
"       source resize.tcl",
"       source plot.tcl",
"       canvas .c -width 800 -height 600",
"       pack .c",
"       gnuplot .c",
"",
" Or, for Perl/Tk use a program like this:",
"",
"       use Tk;",
"       my $top = MainWindow->new;",
"       my $c = $top->Canvas(-width => 800, -height => 600)->pack;",
"       my $gnuplot = do \"plot.pl\";",
"       $gnuplot->($c);",
"       MainLoop;",
"",
" Or, for Perl/Tkx use a program like this:",
"",
"       use Tkx;",
"       my $top = Tkx::widget->new(\".\");",
"       my $c = $top->new_tk__canvas(-width => 800, -height => 600);",
"       $c->g_pack;",
"       my $gnuplot = do \"plot.pl\";",
"       $gnuplot->($c);",
"       Tkx::MainLoop();",
"",
" Or, for Python/Tkinter use a program like this:",
"",
"       from tkinter import *",
"       from tkinter import font",
"       root = Tk()",
"       c = Canvas(root, width=800, height=600)",
"       c.pack()",
"       exec(open('plot.py').read())",
"       gnuplot(c)",
"       root.mainloop()",
"",
" Or, for Ruby/Tk use a program like this:",
"",
"       require 'tk'",
"       root = TkRoot.new { title 'Ruby/Tk' }",
"       c = TkCanvas.new(root, 'width'=>800, 'height'=>600) { pack  { } }",
"       load('plot.rb')",
"       gnuplot(c)",
"       Tk.mainloop",
"",
" Or, for Rexx/Tk use a program like this:",
"",
"       /**/",
"       call RxFuncAdd 'TkLoadFuncs', 'rexxtk', 'TkLoadFuncs'",
"       call TkLoadFuncs",
"       cv = TkCanvas('.c', '-width', 800, '-height', 600)",
"       call TkPack cv",
"       call 'plot.rex' cv",
"       do forever",
"           cmd = TkWait()",
"           if cmd = 'AWinClose' then leave",
"           interpret 'call' cmd",
"       end",
"",
" The code generated by `gnuplot` (in the above examples, this code is",
" written to \"plot.<ext>\") contains the following procedures:",
"",
" gnuplot(canvas)",
"    takes the name of a canvas as its argument.",
"    When called, it clears the canvas, finds the size of the canvas and",
"    draws the plot in it, scaled to fit.",
"",
" gnuplot_plotarea()",
"    returns a list containing the borders of the plotting area",
"    (xleft, xright, ytop, ybot) in canvas screen coordinates."
"    It works only for 2-dimensional plotting (`plot`).",
"",
" gnuplot_axisranges()",
"    returns the ranges of the two axes in plot coordinates",
"    (x1min, x1max, y1min, y1max, x2min, x2max, y2min, y2max).",
"    It works only for 2-dimensional plotting (`plot`).",
"",
" You can create self-contained, minimal scripts using the `standalone`",
" option.  The default is `input` which creates scripts which have to be",
" source'd (or loaded or called or whatever the adequate term is for the",
" language selected).",
"",
" If the `interactive` option is specified, mouse clicking on a line segment",
" will print the coordinates of its midpoint to stdout.",
" The user can supersede this behavior by supplying a procedure",
" user_gnuplot_coordinates which takes the following arguments:",
"   win id x1s y1s x2s y2s x1e y1e x2e y2e x1m y1m x2m y2m,",
" i.e. the name of the canvas and the id of the line segment followed by the",
" coordinates of its start and end point in the two possible axis ranges; the",
" coordinates of the midpoint are only filled for logarithmic axes.",
"",
" By default the canvas is `transparent`, but an explicit background color",
" can be set with the `background` option.",
"",
" `rounded` sets line caps and line joins to be rounded;",
" `butt` is the default:  butt caps and mitered joins.",
"",
" Text at arbitrary angles can be activated with the `rottext` option,",
" which requires Tcl/Tk 8.6 or later. The default is `norottext`.",
"",
" The `size` option tries to optimize the tic and font sizes for the given",
" canvas size.  By default an output size of 800 x 600 pixels is assumed.",
"",
" `enhanced` selects `enhanced text` processing (default), but is currently",
" only available for Tcl.",
"",
" The `pixels` (default) option selects the failsafe pixel-by-pixel image",
" handler, see also `image pixels`.",
" The `externalimages` option saves images as external png images, which",
" are later loaded and scaled by the tkcanvas code.  This option is only",
" available for Tcl and display may be slow in some situations since the",
" Tk image handler does not provide arbitrary scaling.  Scripts need to source",
" the provided rescale.tcl.",
"",
" Interactive mode is not yet implemented for Python/Tk and Rexx/Tk.",
" Interactive mode for Ruby/Tk does not yet support user_gnuplot_coordinates."
END_HELP(tkcanvas)
#endif
