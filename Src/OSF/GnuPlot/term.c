/* GNUPLOT - term.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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

/* This module is responsible for looking after the terminal
 * drivers at the lowest level. Only this module (should)
 * know about all the various rules about interpreting
 * the terminal capabilities exported by the terminal
 * drivers in the table.
 *
 * Note that, as far as this module is concerned, a
 * terminal session lasts only until _either_ terminal
 * or output file changes. Before either is changed,
 * the terminal is shut down.
 *
 * Entry points : (see also term/README)
 *
 * term_set_output() : called when  set output  invoked
 *
 * term_initialise()  : optional. Prepare the terminal for first
 *                use. It protects itself against subsequent calls.
 *
 * term_start_plot() : called at start of graph output. Calls term_init
 *                     if necessary
 *
 * term_apply_lp_properties() : apply linewidth settings
 *
 * term_end_plot() : called at the end of a plot
 *
 * term_reset() : called during int_error handling, to shut
 *                terminal down cleanly
 *
 * term_start_multiplot() : called by   set multiplot
 *
 * term_end_multiplot() : called by  set nomultiplot
 *
 * term_check_multiplot_okay() : called just before an interactive
 *                        prompt is issued while in multiplot mode,
 *                        to allow terminal to suspend if necessary,
 *                        Raises an error if interactive multiplot
 *                       is not supported.
 */

#include <gnuplot.h>
#pragma hdrstop
#include "driver.h"
#include "term.h"
#include "version.h"
#ifdef USE_MOUSE
	//#include "mouse.h"
#else
	/* Some terminals (svg GpGg.Canvas) can provide mousing information */
	/* even if the interactive gnuplot session itself cannot.      */
	long mouse_mode = 0;
	char* mouse_alt_string = NULL;
#endif
#ifdef WIN32
	/* FIXME: Prototypes are in win/wcommon.h */
	FILE * open_printer();     /* in wprinter.c */
	void close_printer(FILE * outfile);
	#include "win/winmain.h"
	#ifdef __MSC__
		#include <malloc.h>
		#include <io.h>
	#else
		//#include <alloc.h>
	#endif                         /* MSC */
#endif /* _Windows */

static int termcomp(const void * a, const void * b);

// Externally visible variables 
// the central instance: the current terminal's interface structure 
GpTermEntry * term = NULL;  // unknown 
char   term_options[MAX_LINE_LEN+1] = ""; // ... and its options string 
// the 'output' file name and handle 
char * outstr = NULL; // means "STDOUT" 
FILE * gpoutfile;
//
// Output file where the PostScript output goes to. See term_api.h for more details.
//
FILE * gppsfile = 0;
char * PS_psdir = NULL;
//bool   term_initialised; // true if terminal has been initialized 
// The qt and wxt terminals cannot be used in the same session. 
// Whichever one is used first to plot, this locks out the other. 
void * term_interlock = NULL;
//bool   IsMonochrome = false; /* true if "set monochrome" */
//bool   multiplot = false;/* true if in multiplot mode */
enum   set_encoding_id encoding; /* text output encoding, for terminals that support it */

/* table of encoding names, for output of the setting */
const char * encoding_names[] = {
	"default", 
	"iso_8859_1", 
	"iso_8859_2", 
	"iso_8859_9", 
	"iso_8859_15",
	"cp437", 
	"cp850", 
	"cp852", 
	"cp950", 
	"cp1250", 
	"cp1251", 
	"cp1252", 
	"cp1254",
	"koi8r", 
	"koi8u", 
	"sjis", 
	"utf8", 
	NULL
};
// 'set encoding' options 
const GenTable set_encoding_tbl[] =
{
	{ "def$ault", S_ENC_DEFAULT },
	{ "utf$8", S_ENC_UTF8 },
	{ "iso$_8859_1", S_ENC_ISO8859_1 },
	{ "iso_8859_2", S_ENC_ISO8859_2 },
	{ "iso_8859_9", S_ENC_ISO8859_9 },
	{ "iso_8859_15", S_ENC_ISO8859_15 },
	{ "cp4$37", S_ENC_CP437 },
	{ "cp850", S_ENC_CP850 },
	{ "cp852", S_ENC_CP852 },
	{ "cp950", S_ENC_CP950 },
	{ "cp1250", S_ENC_CP1250 },
	{ "cp1251", S_ENC_CP1251 },
	{ "cp1252", S_ENC_CP1252 },
	{ "cp1254", S_ENC_CP1254 },
	{ "koi8$r", S_ENC_KOI8_R },
	{ "koi8$u", S_ENC_KOI8_U },
	{ "sj$is", S_ENC_SJIS },
	{ NULL, S_ENC_INVALID }
};

const char * arrow_head_names[4] = {
	"nohead", 
	"head", 
	"backhead", 
	"heads"
};

enum { 
	IPC_BACK_UNUSABLE = -2, 
	IPC_BACK_CLOSED = -1 
};

#ifdef PIPE_IPC
	static SELECT_TYPE_ARG1 ipc_back_fd = IPC_BACK_CLOSED; // HBB 20020225: currently not used anywhere outside term.c 
#endif
int    gp_resolution = 72; // resolution in dpi for converting pixels to size units 
// Support for enhanced text mode. Declared extern in term_api.h 
char   enhanced_text[MAX_LINE_LEN+1] = "";
char * enhanced_cur_text = NULL;
double enhanced_fontscale = 1.0;
char   enhanced_escape_format[16] = "";
double enhanced_max_height = 0.0, enhanced_min_height = 0.0;
bool   ignore_enhanced_text = false; // flag variable to disable enhanced output of filenames, mainly
int    linetype_recycle_count = 0; // Recycle count for user-defined linetypes 
int    mono_recycle_count = 0;
//
// Internal variables 
//
//static bool   term_graphics = false; // true if terminal is in graphics mode
//static bool   term_suspended = false; // we have suspended the driver, in multiplot mode 
//static bool   opened_binary = false;  // true if? 
//static bool   term_force_init = false; // true if require terminal to be initialized 
//static double term_pointsize = 1.0;    // internal pointsize for do_point 

/* Internal prototypes: */

//static void term_suspend();
static void term_close_output();
static void null_linewidth(double);
static void do_point(uint x, uint y, int number);
static void do_pointsize(double size);
static void line_and_point(uint x, uint y, int number);
static void do_arrow(uint sx, uint sy, uint ex, uint ey, int head);
static void null_dashtype(int type, t_dashtype *custom_dash_pattern);
static int null_text_angle(int ang);
static int null_justify_text(enum JUSTIFY just);
static int null_scale(double x, double y);
static void null_layer(t_termlayer layer);
static int null_set_font(const char* font);
static void null_set_color(t_colorspec * colorspec);
static void options_null(GpCommand & rC);
static void graphics_null();
static void UNKNOWN_null();
static void MOVE_null(uint, uint);
static void LINETYPE_null(int);
static void PUTTEXT_null(uint, uint, const char *);
static int strlen_tex(const char*);
static char * stylefont(const char * fontname, bool isbold, bool isitalic);

//static size_units parse_term_size(float * xsize, float * ysize, size_units def_units);

#ifdef VMS
	char * vms_init();
	void vms_reset();
	void term_mode_tek();
	void term_mode_native();
	void term_pasthru();
	void term_nopasthru();
	void fflush_binary();
	#define FOPEN_BINARY(file) fopen(file, "wb", "rfm=fix", "bls=512", "mrs=512")
#else /* !VMS */
	#define FOPEN_BINARY(file) fopen(file, "wb")
#endif /* !VMS */
#if defined(WIN32)
	#include <fcntl.h>
	#ifndef O_BINARY
		#ifdef _O_BINARY
			#define O_BINARY _O_BINARY
		#else
			#define O_BINARY O_BINARY_is_not_defined
		#endif
	#endif
#endif
#ifdef __EMX__
	#include <io.h>
	#include <fcntl.h>
#endif

#if defined(__WATCOMC__) || defined(__MSC__)
# include <io.h>        /* for setmode() */
#endif

#define NICE_LINE               0
#define POINT_TYPES             6

#ifndef DEFAULTTERM
# define DEFAULTTERM NULL
#endif

/* interface to the rest of gnuplot - the rules are getting
 * too complex for the rest of gnuplot to be allowed in
 */

#if defined(PIPES)
static bool output_pipe_open = false;
#endif /* PIPES */

static void term_close_output()
{
	FPRINTF((stderr, "term_close_output\n"));
	GpGg.opened_binary = false;
	if(!outstr)             /* ie using stdout */
		return;
#if defined(PIPES)
	if(output_pipe_open) {
		(void)pclose(gpoutfile);
		output_pipe_open = false;
	}
	else
#endif /* PIPES */
#ifdef _Windows
	if(_stricmp(outstr, "PRN") == 0)
		close_printer(gpoutfile);
	else
#endif
	if(gpoutfile != gppsfile)
		fclose(gpoutfile);
	gpoutfile = stdout;     /* Don't dup... */
	ZFREE(outstr);
	SFile::ZClose(&gppsfile);
}
//
// assigns dest to outstr, so it must be allocated or NULL
// and it must not be outstr itself !
//
void GpGadgets::TermSetOutput(GpTermEntry * pT, char * pDest)
{
	FILE * f = NULL;
	FPRINTF((stderr, "term_set_output\n"));
	assert(pDest == NULL || pDest != outstr);
	if(IsMultiPlot) {
		fputs("In multiplot mode you can't change the output\n", stderr);
		return;
	}
	if(pT && term_initialised) {
		(*pT->reset)();
		term_initialised = false;
		// switch off output to special postscript file (if used) 
		gppsfile = NULL;
	}
	if(pDest == NULL) { // stdout 
		term_close_output();
	}
	else {
#if defined(PIPES)
		if(*dest == '|') {
			restrict_popen();
#ifdef _Windows
			f = (pT && (pT->flags & TERM_BINARY)) ? popen(dest + 1, "wb") : popen(dest + 1, "w");
#else
			f = popen(dest + 1, "w");
#endif
			if(f == (FILE*)NULL)
				os_error(Gp__C.CToken, "cannot create pipe; output not changed");
			else
				output_pipe_open = true;
		}
		else {
#endif /* PIPES */

#ifdef _Windows
		if(outstr && _stricmp(outstr, "PRN") == 0) {
			// we can't call open_printer() while printer is open, so 
			close_printer(gpoutfile); // close printer immediately if open 
			gpoutfile = stdout; // and reset output to stdout 
			ZFREE(outstr);
		}
		if(_stricmp(pDest, "PRN") == 0) {
			if((f = open_printer()) == (FILE*)NULL)
				os_error(Gp__C.CToken, "cannot open printer temporary file; output may have changed");
		}
		else
#endif

		{
			f = (pT && (pT->flags & TERM_BINARY)) ? FOPEN_BINARY(pDest) : fopen(pDest, "w");
			if(f == (FILE*)NULL)
				os_error(Gp__C.CToken, "cannot open file; output not changed");
		}
#if defined(PIPES)
	}
#endif
		term_close_output();
		gpoutfile = f;
		outstr = pDest;
		opened_binary = (pT && (pT->flags & TERM_BINARY));
	}
}

void GpGadgets::TermInitialise()
{
	FPRINTF((stderr, "term_initialise()\n"));
	if(!term)
		IntErrorNoCaret("No terminal defined");
	//
	// check if we have opened the output file in the wrong mode
	// (text/binary), if set term comes after set output
	// This was originally done in change_term, but that
	// resulted in output files being truncated
	//
	if(outstr && (term->flags & TERM_NO_OUTPUTFILE)) {
		if(IsInteractive)
			fprintf(stderr, "Closing %s\n", outstr);
		term_close_output();
	}
	if(outstr && (((term->flags & TERM_BINARY) && !opened_binary) || ((!(term->flags & TERM_BINARY) && opened_binary)))) {
		//
		// this is nasty - we cannot just term_set_output(outstr)
		// since term_set_output will first free outstr and we
		// end up with an invalid pointer. I think I would
		// prefer to defer opening output file until first plot.
		//
		char * temp = (char *)malloc(strlen(outstr) + 1);
		if(temp) {
			FPRINTF((stderr, "term_initialise: reopening \"%s\" as %s\n", outstr, term->flags & TERM_BINARY ? "binary" : "text"));
			strcpy(temp, outstr);
			TermSetOutput(term, temp); /* will free outstr */
			if(temp != outstr) {
				free(temp);
				temp = outstr;
			}
		}
		else
			fputs("Cannot reopen output file in binary", stderr);
		// and carry on, hoping for the best ! 
	}
#if defined (_Windows)
	else if(!outstr && (term->flags & TERM_BINARY)) {
		// binary to stdout in non-interactive session... 
		fflush(stdout);
		_setmode(_fileno(stdout), O_BINARY);
	}
#endif
	if(!term_initialised || term_force_init) {
		FPRINTF((stderr, "- calling term->init()\n"));
		(*term->init)();
		term_initialised = true;
	}
}

//void term_start_plot()
void GpGadgets::TermStartPlot(GpTermEntry * pT)
{
	FPRINTF((stderr, "term_start_plot()\n"));
	if(!term_initialised)
		TermInitialise();
	if(!term_graphics) {
		FPRINTF((stderr, "- calling pT->graphics()\n"));
		(*pT->graphics)();
		term_graphics = true;
	}
	else if(IsMultiPlot && term_suspended) {
		if(pT->resume) {
			FPRINTF((stderr, "- calling pT->resume()\n"));
			(*pT->resume)();
		}
		term_suspended = false;
	}
	// Sync point for epslatex text positioning 
	pT->_Layer(TERM_LAYER_RESET);
	// Because PostScript plots may be viewed out of order, make sure 
	// Each new plot makes no assumption about the previous palette. 
	if(pT->flags & TERM_IS_POSTSCRIPT)
		invalidate_palette();
	// Set Canvas size to full range of current terminal coordinates 
	Canvas.xleft  = 0;
	Canvas.xright = pT->xmax - 1;
	Canvas.ybot   = 0;
	Canvas.ytop   = pT->ymax - 1;
}

//void term_end_plot()
void GpGadgets::TermEndPlot(GpTermEntry * pT)
{
	FPRINTF((stderr, "term_end_plot()\n"));
	if(term_initialised) {
		// Sync point for epslatex text positioning 
		pT->_Layer(TERM_LAYER_END_TEXT);
		if(!IsMultiPlot) {
			FPRINTF((stderr, "- calling pT->text()\n"));
			(*pT->text)();
			term_graphics = false;
		}
		else {
			MultiplotNext(pT);
		}
#ifdef VMS
		if(opened_binary)
			fflush_binary();
		else
#endif /* VMS */
		(void)fflush(gpoutfile);
#ifdef USE_MOUSE
		RecalcStatusLine();
		UpdateRuler(pT);
#endif
	}
}

void term_reset()
{
	FPRINTF((stderr, "term_reset()\n"));
#ifdef USE_MOUSE
	// Make sure that ^C will break out of a wait for 'pause mouse' 
	paused_for_mouse = 0;
#ifdef WIN32
	kill_pending_Pause_dialog();
#endif
#endif
	if(GpGg.term_initialised) {
		if(GpGg.term_suspended) {
			if(term->resume) {
				FPRINTF((stderr, "- calling term->resume()\n"));
				(*term->resume)();
			}
			GpGg.term_suspended = false;
		}
		if(GpGg.term_graphics) {
			(*term->text)();
			GpGg.term_graphics = false;
		}
		if(GpGg.term_initialised) {
			(*term->reset)();
			GpGg.term_initialised = false;
			// switch off output to special postscript file (if used) 
			gppsfile = NULL;
		}
	}
}

//void term_apply_lp_properties(GpTermEntry * pT, lp_style_type * lp)
void GpGadgets::ApplyLpProperties(GpTermEntry * pT, lp_style_type * pLp)
{
	// This function passes all the line and point properties to the
	// terminal driver and issues the corresponding commands.
	// 
	// Alas, sometimes it might be necessary to give some help to
	// this function by explicitly issuing additional '(*term)(...)' commands.
	// 
	int lt = pLp->l_type;
	int dt = pLp->d_type;
	t_dashtype custom_dash_pattern = pLp->custom_dash_pattern;
	t_colorspec colorspec = pLp->pm3d_color;
	if(pLp->flags & LP_SHOW_POINTS) {
		// change points, too
		// Currently, there is no 'pointtype' function.  For points
		// there is a special function also dealing with (x,y) coordinates.
		// 
		pT->pointsize((pLp->p_size >= 0) ? pLp->p_size : PtSz);
	}
	// _first_ set the line width, _then_ set the line type !
	// The linetype might depend on the linewidth in some terminals.
	pT->linewidth(pLp->l_width);
	// LT_DEFAULT (used only by "set errorbars"?) means don't change it
	// FIXME: If this causes problems, test also for LP_ERRORBAR_SET   
	if(lt == LT_DEFAULT)
		;
	else {
		// The paradigm for handling linetype and dashtype in version 5 is 
		// linetype < 0 (e.g. LT_BACKGROUND, LT_NODRAW) means some special 
		// category that will be handled directly by pT->linetype().     
		// linetype > 0 is now redundant. It used to encode both a color   
		// and a dash pattern.  Now we have separate mechanisms for those. 
		if(LT_COLORFROMCOLUMN < lt && lt < 0)
			pT->linetype(lt);
		else if(pT->set_color == null_set_color) {
			pT->linetype(lt-1);
			return;
		}
		else // All normal lines will be solid unless a dashtype is given 
			pT->linetype(LT_SOLID);
	}
	// Apply dashtype or user-specified dash pattern, which may override  
	// the terminal-specific dot/dash pattern belonging to this linetype. 
	if(lt == LT_AXIS)
		;  // LT_AXIS is a special linetype that may incorporate a dash pattern 
	else if(dt == DASHTYPE_CUSTOM)
		pT->dashtype(dt, &custom_dash_pattern);
	else if(dt == DASHTYPE_SOLID)
		pT->dashtype(dt, NULL);
	else if(dt >= 0) {
		// The null_dashtype() routine or a version 5 terminal's private  
		// dashtype routine converts this into a call to pT->linetype() 
		// yielding the same result as in version 4 except possibly for a 
		// different line width.
		pT->dashtype(dt, NULL);
	}
	// Finally adjust the color of the line */
	ApplyPm3DColor(pT, &colorspec);
}

//void term_start_multiplot()
void GpGadgets::TermStartMultiplot(GpTermEntry * pT, GpCommand & rC)
{
	FPRINTF((stderr, "term_start_multiplot()\n"));
	MultiplotStart(pT, rC);
#ifdef USE_MOUSE
	UpdateStatusline(pT);
#endif
}

void GpGadgets::TermEndMultiplot(GpTermEntry * pT)
{
	FPRINTF((stderr, "term_end_multiplot()\n"));
	if(IsMultiPlot) {
		if(term_suspended) {
			if(pT->resume)
				(*pT->resume)();
			term_suspended = false;
		}
		MultiplotEnd();
		TermEndPlot(pT);
#ifdef USE_MOUSE
		UpdateStatusline(pT);
#endif
	}
}

void DrawAxisLabel(uint x, uint y, const GpAxis & rAx, JUSTIFY hor, VERT_JUSTIFY vert, bool dontRotate)
{
	term->DrawMultiline(x, y, rAx.label.text, hor, vert, dontRotate ? rAx.label.rotate : 0, rAx.label.font);
}

void GpTermEntry::DrawMultiline(uint x, uint y, char * pText, JUSTIFY hor, VERT_JUSTIFY vert, int angle, const char * pFont)
{
	if(pText) {
		char * p = pText;
		// EAM 9-Feb-2003 - Set pFont before calculating sizes 
		if(pFont && *pFont)
			(*set_font)(pFont);
		if(vert != JUST_TOP) {
			// count lines and adjust y 
			int lines = 0; // number of linefeeds - one fewer than lines 
			while(*p) {
				if(*p++ == '\n')
					++lines;
			}
			if(angle)
				x -= (vert * lines * VChr) / 2;
			else
				y += (vert * lines * VChr) / 2;
		}
		for(;; ) { // we will explicitly break out 
			if(pText && (p = strchr(pText, '\n')) != NULL)
				*p = 0; // terminate the string 
			if((*justify_text)(hor)) {
				if(on_page(x, y))
					(*put_text)(x, y, pText);
			}
			else {
				int len = estimate_strlen(pText);
				int hfix, vfix;
				if(angle == 0) {
					hfix = hor * HChr * len / 2;
					vfix = 0;
				}
				else {
					// Attention: This relies on the numeric values of enum JUSTIFY! 
					hfix = (int)(hor * HChr * len * cos(angle * DEG2RAD) / 2 + 0.5);
					vfix = (int)(hor * VChr * len * sin(angle * DEG2RAD) / 2 + 0.5);
				}
				if(on_page(x - hfix, y - vfix))
					(*put_text)(x - hfix, y - vfix, pText);
			}
			if(angle == 90 || angle == TEXT_VERTICAL)
				x += VChr;
			else if(angle == -90 || angle == -TEXT_VERTICAL)
				x -= VChr;
			else
				y -= VChr;
			if(!p)
				break;
			else
				*p = '\n'; // put it back 
			pText = p + 1;
		} // unconditional branch back to the for(;;) - just a goto ! 
		if(pFont && *pFont)
			(*set_font)("");
	}
}

static void do_point(uint x, uint y, int number)
{
	int htic, vtic;
	GpTermEntry * t = term;
	/* use solid lines for point symbols */
	if(term->dashtype != null_dashtype)
		term->dashtype(DASHTYPE_SOLID, NULL);
	if(number < 0) {        /* do dot */
		t->_Move(x, y);
		t->_Vector(x, y);
		return;
	}
	number %= POINT_TYPES;
	/* should be in term_tbl[] in later version */
	htic = (int)((GpGg.term_pointsize * t->HTic / 2));
	vtic = (int)((GpGg.term_pointsize * t->VTic / 2));

	/* point types 1..4 are same as in postscript, png and x11
	   point types 5..6 are "similar"
	   (note that (number) GpGg.Gp__C.Eq (pointtype-1)
	 */
	switch(number) {
		case 4:         /* do diamond */
		    t->_Move(x - htic, y);
		    t->_Vector(x, y - vtic);
		    t->_Vector(x + htic, y);
		    t->_Vector(x, y + vtic);
		    t->_Vector(x - htic, y);
		    t->_Move(x, y);
		    t->_Vector(x, y);
		    break;
		case 0:         /* do plus */
		    t->_Move(x - htic, y);
		    t->_Vector(x - htic, y);
		    t->_Vector(x + htic, y);
		    t->_Move(x, y - vtic);
		    t->_Vector(x, y - vtic);
		    t->_Vector(x, y + vtic);
		    break;
		case 3:         /* do box */
		    t->_Move(x - htic, y - vtic);
		    t->_Vector(x - htic, y - vtic);
		    t->_Vector(x + htic, y - vtic);
		    t->_Vector(x + htic, y + vtic);
		    t->_Vector(x - htic, y + vtic);
		    t->_Vector(x - htic, y - vtic);
		    t->_Move(x, y);
		    t->_Vector(x, y);
		    break;
		case 1:         /* do X */
		    t->_Move(x - htic, y - vtic);
		    t->_Vector(x - htic, y - vtic);
		    t->_Vector(x + htic, y + vtic);
		    t->_Move(x - htic, y + vtic);
		    t->_Vector(x - htic, y + vtic);
		    t->_Vector(x + htic, y - vtic);
		    break;
		case 5:         /* do triangle */
		    t->_Move(x, y + (4 * vtic / 3));
		    t->_Vector(x - (4 * htic / 3), y - (2 * vtic / 3));
		    t->_Vector(x + (4 * htic / 3), y - (2 * vtic / 3));
		    t->_Vector(x, y + (4 * vtic / 3));
		    t->_Move(x, y);
		    t->_Vector(x, y);
		    break;
		case 2:         /* do star */
		    t->_Move(x - htic, y);
		    t->_Vector(x - htic, y);
		    t->_Vector(x + htic, y);
		    t->_Move(x, y - vtic);
		    t->_Vector(x, y - vtic);
		    t->_Vector(x, y + vtic);
		    t->_Move(x - htic, y - vtic);
		    t->_Vector(x - htic, y - vtic);
		    t->_Vector(x + htic, y + vtic);
		    t->_Move(x - htic, y + vtic);
		    t->_Vector(x - htic, y + vtic);
		    t->_Vector(x + htic, y - vtic);
		    break;
	}
}

static void do_pointsize(double size)
{
	GpGg.term_pointsize = (size >= 0 ? size : 1);
}
//
// general point routine
//
static void line_and_point(uint x, uint y, int number)
{
	// temporary(?) kludge to allow terminals with bad linetypes to make nice marks 
	term->_LineType(NICE_LINE);
	do_point(x, y, number);
}
/*
 * general arrow routine
 *
 * I set the angle between the arrowhead and the line 15 degree.
 * The length of arrowhead varies depending on the line length
 * within the the range [0.3*(the-tic-length), 2*(the-tic-length)].
 * No head is printed if the arrow length is zero.
 *
 *            Yasu-hiro Yamazaki(hiro@rainbow.physics.utoronto.ca)
 *            Jul 1, 1993
 */

#define COS15 (0.96593)         /* cos of 15 degree */
#define SIN15 (0.25882)         /* sin of 15 degree */

#define HEAD_LONG_LIMIT  (2.0)  /* long  limit of arrowhead length */
#define HEAD_SHORT_LIMIT (0.3)  /* short limit of arrowhead length */
                                /* their units are the "tic" length */

#define HEAD_COEFF  (0.3)       /* default value of head/line length ratio */

int curr_arrow_headlength; /* access head length + angle without changing API */
double curr_arrow_headangle;    /* angle in degrees */
double curr_arrow_headbackangle;  /* angle in degrees */
arrowheadfill curr_arrow_headfilled;      /* arrow head filled or not */
bool curr_arrow_headfixedsize;        /* Adapt the head size for short arrows or not */

static void do_arrow(uint usx, uint usy/* start point */, uint uex, uint uey/* end point (point of arrowhead) */, int headstyle)
{
	/* Clipping and angle calculations do not work if coords are unsigned! */
	int sx = (int)usx;
	int sy = (int)usy;
	int ex = (int)uex;
	int ey = (int)uey;

	GpTermEntry * t = term;
	float len_tic = (float)(((double)(t->HTic + t->VTic)) / 2.0);
	/* average of tic sizes */
	/* (dx,dy) : vector from end to start */
	double dx = sx - ex;
	double dy = sy - ey;
	double len_arrow = sqrt(dx * dx + dy * dy);
	gpiPoint head_points[5];
	int xm = 0, ym = 0;
	BoundingBox * clip_save;
	t_arrow_head head = (t_arrow_head)((headstyle < 0) ? -headstyle : headstyle);
	// negative headstyle means draw heads only, no shaft 
	//
	// The arrow shaft was clipped already in do_clip_arrow() but we still */
	// need to clip the head here
	//
	clip_save = GpGg.P_Clip;
	GpGg.P_Clip = (term->flags & TERM_CAN_CLIP) ? NULL : &GpGg.Canvas;
	//
	// Calculate and draw arrow heads.
	// Draw no head for arrows with length = 0, or, to be more specific,
	// length < SMathConst::Epsilon, because len_arrow will almost always be != 0.
	//
	if((head != NOHEAD) && fabs(len_arrow) >= SMathConst::Epsilon) {
		int x1, y1, x2, y2;
		if(curr_arrow_headlength <= 0) {
			/* An arrow head with the default size and angles */
			double coeff_shortest = len_tic * HEAD_SHORT_LIMIT / len_arrow;
			double coeff_longest = len_tic * HEAD_LONG_LIMIT / len_arrow;
			double head_coeff = MAX(coeff_shortest, MIN(HEAD_COEFF, coeff_longest));
			/* we put the arrowhead marks at 15 degrees to line */
			x1 = (int)((COS15 * dx - SIN15 * dy) * head_coeff);
			y1 = (int)((SIN15 * dx + COS15 * dy) * head_coeff);
			x2 = (int)((COS15 * dx + SIN15 * dy) * head_coeff);
			y2 = (int)((-SIN15 * dx + COS15 * dy) * head_coeff);
			/* backangle defaults to 90 deg */
			xm = (int)((x1 + x2)/2);
			ym = (int)((y1 + y2)/2);
		}
		else {
			/* An arrow head with the length + angle specified explicitly.	*/
			/* Assume that if the arrow is shorter than the arrowhead, this is	*/
			/* because of foreshortening in a 3D plot.                      */
			double alpha = curr_arrow_headangle * DEG2RAD;
			double beta = curr_arrow_headbackangle * DEG2RAD;
			double phi = atan2(-dy, -dx); /* azimuthal angle of the vector */
			double backlen, effective_length;
			double dx2, dy2;
			effective_length = curr_arrow_headlength;
			if(!curr_arrow_headfixedsize && (curr_arrow_headlength > len_arrow/2.)) {
				effective_length = len_arrow/2.;
				alpha = atan(tan(alpha)*((double)curr_arrow_headlength/effective_length));
				beta = atan(tan(beta)*((double)curr_arrow_headlength/effective_length));
			}
			backlen = sin(alpha) / sin(beta);
			/* anticlock-wise head segment */
			x1 = -(int)(effective_length * cos(alpha - phi));
			y1 =  (int)(effective_length * sin(alpha - phi));
			/* clock-wise head segment */
			dx2 = -effective_length * cos(phi + alpha);
			dy2 = -effective_length * sin(phi + alpha);
			x2 = (int)(dx2);
			y2 = (int)(dy2);
			/* back point */
			xm = (int)(dx2 + backlen*effective_length * cos(phi + beta));
			ym = (int)(dy2 + backlen*effective_length * sin(phi + beta));
		}
		if((head & END_HEAD) && !GpGg.ClipPoint(ex, ey)) {
			head_points[0].x = ex + xm;
			head_points[0].y = ey + ym;
			head_points[1].x = ex + x1;
			head_points[1].y = ey + y1;
			head_points[2].x = ex;
			head_points[2].y = ey;
			head_points[3].x = ex + x2;
			head_points[3].y = ey + y2;
			head_points[4].x = ex + xm;
			head_points[4].y = ey + ym;
			if(curr_arrow_headfilled >= AS_FILLED) {
				// draw filled forward arrow head
				head_points->style = FS_OPAQUE;
				if(t->filled_polygon)
					(*t->filled_polygon)(5, head_points);
			}
			/* draw outline of forward arrow head */
			if(curr_arrow_headfilled == AS_NOFILL) {
				GpGg.DrawClipPolygon(t, 3, head_points+1);
			}
			else if(curr_arrow_headfilled != AS_NOBORDER) {
				GpGg.DrawClipPolygon(t, 5, head_points);
			}
		}
		// backward arrow head 
		if((head & BACKHEAD) && !GpGg.ClipPoint(sx, sy)) {
			head_points[0].x = sx - xm;
			head_points[0].y = sy - ym;
			head_points[1].x = sx - x1;
			head_points[1].y = sy - y1;
			head_points[2].x = sx;
			head_points[2].y = sy;
			head_points[3].x = sx - x2;
			head_points[3].y = sy - y2;
			head_points[4].x = sx - xm;
			head_points[4].y = sy - ym;
			if(curr_arrow_headfilled >= AS_FILLED) {
				// draw filled backward arrow head
				head_points->style = FS_OPAQUE;
				if(t->filled_polygon)
					(*t->filled_polygon)(5, head_points);
			}
			// draw outline of backward arrow head
			if(curr_arrow_headfilled == AS_NOFILL) {
				GpGg.DrawClipPolygon(t, 3, head_points+1);
			}
			else if(curr_arrow_headfilled != AS_NOBORDER) {
				GpGg.DrawClipPolygon(t, 5, head_points);
			}
		}
	}
	// Draw the line for the arrow
	if(headstyle >= 0) {
		if((head & BACKHEAD) && (fabs(len_arrow) >= SMathConst::Epsilon) && (curr_arrow_headfilled != AS_NOFILL) ) {
			sx -= xm;
			sy -= ym;
		}
		if((head & END_HEAD) && (fabs(len_arrow) >= SMathConst::Epsilon) && (curr_arrow_headfilled != AS_NOFILL) ) {
			ex += xm;
			ey += ym;
		}
		GpGg.DrawClipLine(term, sx, sy, ex, ey);
	}
	// Restore previous clipping box 
	GpGg.P_Clip = clip_save;
}

#ifdef EAM_OBJECTS
/* Generic routine for drawing circles or circular arcs.          */
/* If this feature proves useful, we can add a new terminal entry */
/* point term->arc() to the API and let terminals either provide  */
/* a private implemenation or use this generic one.               */

void do_arc(uint cx, uint cy, double radius, double arc_start,  double arc_end/* Limits of arc in degress */, int style, bool wedge)
{
	gpiPoint vertex[250];
	int i, segments;
	double aspect;
	bool complete_circle;
	// Protect against out-of-range values 
	while(arc_start < 0)
		arc_start += 360.;
	while(arc_end > 360.)
		arc_end -= 360.;
	// Always draw counterclockwise 
	while(arc_end < arc_start)
		arc_end += 360.;
	/* Choose how finely to divide this arc into segments */
	/* FIXME: INC=2 causes problems for gnuplot_x11 */
#define INC 3.0
	segments = (int)((arc_end - arc_start) / INC);
	if(segments < 1)
		segments = 1;
	// Calculate the vertices 
	aspect = (double)term->VTic / (double)term->HTic;
#ifdef WIN32
	if(strcmp(term->name, "windows") == 0)
		aspect = 1.;
#endif
	for(i = 0; i < segments; i++) {
		vertex[i].x = (int)(cx + cos(DEG2RAD * (arc_start + i*INC)) * radius);
		vertex[i].y = (int)(cy + sin(DEG2RAD * (arc_start + i*INC)) * radius * aspect);
	}
#undef INC
	vertex[segments].x = (int)(cx + cos(DEG2RAD * arc_end) * radius);
	vertex[segments].y = (int)(cy + sin(DEG2RAD * arc_end) * radius * aspect);
	if(fabs(arc_end - arc_start) > .1 && fabs(arc_end - arc_start) < 359.9) {
		vertex[++segments].x = cx;
		vertex[segments].y = cy;
		vertex[++segments].x = vertex[0].x;
		vertex[segments].y = vertex[0].y;
		complete_circle = false;
	}
	else
		complete_circle = true;

	if(style) { /* Fill in the center */
		gpiPoint fillarea[250];
		int in;
		GpGg.ClipPolygon(vertex, fillarea, segments, &in);
		fillarea[0].style = style;
		if(term->filled_polygon)
			term->filled_polygon(in, fillarea);
	}
	else { // Draw the arc 
		if(!wedge && !complete_circle)
			segments -= 2;
		GpGg.DrawClipPolygon(term, segments+1, vertex);
	}
}

#endif /* EAM_OBJECTS */

#define TERM_PROTO
#define TERM_BODY
#define TERM_PUBLIC static

#include "term.h"

#undef TERM_PROTO
#undef TERM_BODY
#undef TERM_PUBLIC

/* Dummy functions for unavailable features */
/* return success if they asked for default - this simplifies code
 * where param is passed as a param. Client can first pass it here,
 * and only if it fails do they have to see what was trying to be done
 */

/* change angle of text.  0 is horizontal left to right.
 * 1 is vertical bottom to top (90 deg rotate)
 */
static int null_text_angle(int ang)
{
	return (ang == 0);
}

/* change justification of text.
 * modes are LEFT (flush left), CENTRE (centred), RIGHT (flush right)
 */
static int null_justify_text(enum JUSTIFY just)
{
	return (just == LEFT);
}

/*
 * Deprecated terminal function (pre-version 3)
 */
static int null_scale(double x, double y)
{
	(void)x;                
	(void)y;
	GpGg.IntErrorNoCaret("Attempt to call deprecated terminal function");
	return false;           /* can't be done */
}

static void null_layer(t_termlayer layer)
{
	(void)layer;            
}

static void options_null(GpCommand & rC)
{
	term_options[0] = '\0'; /* we have no options */
}

static void graphics_null()
{
	fprintf(stderr,
	    "WARNING: Plotting with an 'unknown' terminal.\n"
	    "No output will be generated. Please select a terminal with 'set terminal'.\n");
}

static void UNKNOWN_null()
{
}

static void MOVE_null(uint x, uint y)
{
	(void)x;                
	(void)y;
}

static void LINETYPE_null(int t)
{
	(void)t;                
}

static void PUTTEXT_null(uint x, uint y, const char * s)
{
	(void)s;                
	(void)x;
	(void)y;
}

static void null_linewidth(double s)
{
	(void)s;                
}

static int null_set_font(const char * font)
{
	(void)font;             
	return false;           /* Never used!! */
}

static void null_set_color(t_colorspec * colorspec)
{
	if(colorspec->type == TC_LT)
		term->_LineType(colorspec->lt);
}

static void null_dashtype(int type, t_dashtype * custom_dash_pattern)
{
	(void)custom_dash_pattern; /* ignore */
	/*
	 * If the terminal does not support user-defined dashtypes all we can do
	 * do is fall through to the old (pre-v5) assumption that the dashtype,
	 * if any, is part of the linetype.  We also assume that the color will
	 * be adjusted after this.
	 */
	if(type <= 0)
		type = LT_SOLID;
	term->_LineType(type);
}

/* setup the magic macros to compile in the right parts of the
 * terminal drivers included by term.h
 */
#define TERM_TABLE
#define TERM_TABLE_START(x) , {
#define TERM_TABLE_END(x)   }
//
// term_tbl[] contains an entry for each terminal.  "unknown" must be the
// first, since term is initialized to 0.
//
static GpTermEntry term_tbl[] =
{
	{
		"unknown", 
		"Unknown terminal type - not a plotting device",
		100, 
		100, 
		1, 
		1,
		1, 
		1, 
		options_null, 
		UNKNOWN_null, 
		UNKNOWN_null,
		UNKNOWN_null, 
		null_scale, 
		graphics_null, 
		MOVE_null, 
		MOVE_null,
		LINETYPE_null, 
		PUTTEXT_null
	}
#include "term.h"
};

#define TERMCOUNT (sizeof(term_tbl) / sizeof(term_tbl[0]))

void list_terms()
{
	int i;
	char * line_buffer = (char *)malloc(BUFSIZ);
	int sort_idxs[TERMCOUNT];
	/* sort terminal types alphabetically */
	for(i = 0; i < TERMCOUNT; i++)
		sort_idxs[i] = i;
	qsort(sort_idxs, TERMCOUNT, sizeof(int), termcomp);
	/* now sort_idxs[] contains the sorted indices */
	StartOutput();
	strcpy(line_buffer, "\nAvailable terminal types:\n");
	OutLine(line_buffer);
	for(i = 0; i < TERMCOUNT; i++) {
		sprintf(line_buffer, "  %15s  %s\n", term_tbl[sort_idxs[i]].name, term_tbl[sort_idxs[i]].description);
		OutLine(line_buffer);
	}
	EndOutput();
	free(line_buffer);
}

/* Return string with all terminal names.
   Note: caller must free the returned names after use.
 */
char* get_terminals_names()
{
	int i;
	char * buf = (char *)malloc(TERMCOUNT*15); /* max 15 chars per name */
	char * names;
	int sort_idxs[TERMCOUNT];
	// sort terminal types alphabetically 
	for(i = 0; i < TERMCOUNT; i++)
		sort_idxs[i] = i;
	qsort(sort_idxs, TERMCOUNT, sizeof(int), termcomp);
	/* now sort_idxs[] contains the sorted indices */
	strcpy(buf, " "); // let the string have leading and trailing " " in order to search via strstrt(GPVAL_TERMINALS, " png ");
	for(i = 0; i < TERMCOUNT; i++)
		sprintf(buf+strlen(buf), "%s ", term_tbl[sort_idxs[i]].name);
	names = (char *)malloc(strlen(buf)+1);
	strcpy(names, buf);
	free(buf);
	return names;
}

static int termcomp(const void * arga, const void * argb)
{
	const int * a = (const int *)arga;
	const int * b = (const int *)argb;
	return( strcasecmp(term_tbl[*a].name, term_tbl[*b].name) );
}
//
// set_term: get terminal number from name on command line
// will change 'term' variable if successful
//
GpTermEntry * set_term(GpCommand & rC)                   
{
	GpTermEntry * t = NULL;
	char * input_name = NULL;
	if(!rC.EndOfCommand()) {
		input_name = rC.P_InputLine + rC.P_Token[rC.CToken].start_index;
		t = change_term(input_name, rC.P_Token[rC.CToken].length);
		if(!t && rC.IsStringValue(rC.CToken) && (input_name = rC.TryToGetString())) {
			t = change_term(input_name, strlen(input_name));
			free(input_name);
		}
		else {
			rC.CToken++;
		}
	}
	if(!t) {
		change_term("unknown", 7);
		GpGg.IntError(rC.CToken-1, "unknown or ambiguous terminal type; type just 'set terminal' for a list");
	}
	// otherwise the type was changed 
	return (t);
}

/* change_term: get terminal number from name and set terminal type
 *
 * returns NULL for unknown or ambiguous, otherwise is terminal
 * driver pointer
 */
GpTermEntry * change_term(const char * origname, int length)                    
{
	GpTermEntry * t = NULL;
	bool   ambiguous = false;
	char * name = (char*)origname; // For backwards compatibility only 
	if(!strncmp(origname, "X11", length)) {
		name = "x11";
		length = 3;
	}
#ifdef HAVE_CAIROPDF
	// To allow "set term eps" as short for "set term epscairo" 
	if(!strncmp(origname, "eps", length)) {
		name = "epscairo";
		length = 8;
	}
#endif
	for(int i = 0; i < TERMCOUNT; i++) {
		if(!strncmp(name, term_tbl[i].name, length)) {
			if(t)
				ambiguous = true;
			t = term_tbl + i;
			// Exact match is always accepted
			if(length == strlen(term_tbl[i].name)) {
				ambiguous = false;
				break;
			}
		}
	}
	if(t) {
		if(ambiguous)
			t = 0;
		else {
			// Success: set terminal type now
			term = t;
			GpGg.term_initialised = false;
			// check that optional fields are initialised to something
			SETIFZ(term->text_angle, null_text_angle);
			SETIFZ(term->justify_text, null_justify_text);
			SETIFZ(term->point, do_point);
			SETIFZ(term->arrow, do_arrow);
			SETIFZ(term->pointsize, do_pointsize);
			SETIFZ(term->linewidth, null_linewidth);
			SETIFZ(term->layer, null_layer);
			if(term->tscale <= 0)
				term->tscale = 1.0;
			SETIFZ(term->set_font, null_set_font);
			if(term->set_color == 0) {
				term->set_color = null_set_color;
				term->flags |= TERM_NULL_SET_COLOR;
			}
			SETIFZ(term->dashtype, null_dashtype);
			if(GpGg.IsInteractive)
				fprintf(stderr, "Terminal type set to '%s'\n", term->name);
			// Invalidate any terminal-specific structures that may be active 
			invalidate_palette();
		}
	}
	return t;
}

/*
 * Routine to detect what terminal is being used (or do anything else
 * that would be nice).  One anticipated (or allowed for) side effect
 * is that the global ``term'' may be set.
 * The environment variable GNUTERM is checked first; if that does
 * not exist, then the terminal hardware is checked, if possible,
 * and finally, we can check $TERM for some kinds of terminals.
 * A default can be set with -DDEFAULTTERM=myterm in the Makefile
 * or #define DEFAULTTERM myterm in term.h
 */
/* thanks to osupyr!alden (Dave Alden) for the original GNUTERM code */
void init_terminal()
{
	char * term_name = DEFAULTTERM;
#if(defined(MSDOS) && !defined(_Windows)) || defined(SUN) || defined(X11)
	char * env_term = NULL; /* from TERM environment var */
#endif
#ifdef X11
	char * display = NULL;
#endif
	char * gnuterm = NULL;
	/* GNUTERM environment variable is primary */
	gnuterm = getenv("GNUTERM");
	if(gnuterm != (char*)NULL) {
		term_name = gnuterm;
	}
	else {
#ifdef VMS
		term_name = vms_init();
#endif /* VMS */
#ifdef __BEOS__
		env_term = getenv("TERM");
		if(!term_name && env_term && strcmp(env_term, "beterm") == 0)
			term_name = "be";
#endif /* BeOS */
#ifdef QTTERM
		SETIFZ(term_name, "qt");
#endif
#ifdef WXWIDGETS
		SETIFZ(term_name, "wxt");
#endif
#ifdef _Windows
		// let the wxWidgets terminal be the default when available 
		SETIFZ(term_name, "win");
#endif /* _Windows */
#if defined(__APPLE__) && defined(__MACH__) && defined(HAVE_FRAMEWORK_AQUATERM)
		term_name = "aqua"; // Mac OS X with AquaTerm installed 
#endif
#ifdef X11
		env_term = getenv("TERM"); /* try $TERM */
		if(!term_name && env_term && strcmp(env_term, "xterm") == 0)
			term_name = "x11";
		display = getenv("DISPLAY");
		if(!term_name && display)
			term_name = "x11";
		if(X11_Display)
			term_name = "x11";
#endif
#ifdef DJGPP
		term_name = "svga";
#endif
#ifdef GRASS
		term_name = "grass";
#endif
	// set linux terminal only if LINUX_setup was successfull, if we are on X11
	// LINUX_setup has failed, also if we are logged in by network 
#ifdef LINUXVGA
		if(LINUX_graphics_allowed)
#if defined(VGAGL) && defined (THREEDKIT)
			term_name = "vgagl";
#else
			term_name = "linux";
#endif
#endif
	}
	// We have a name, try to set term type 
	if(term_name != NULL && *term_name != '\0') {
		int namelength = strlen(term_name);
		UdvtEntry * name = GpGg.Ev.AddUdvByName("GNUTERM");
		Gstring(&name->udv_value, gp_strdup(term_name));
		if(strchr(term_name, ' '))
			namelength = strchr(term_name, ' ') - term_name;
		/* Force the terminal to initialize default fonts, etc.	This prevents */
		/* segfaults and other strangeness if you set GNUTERM to "post" or    */
		/* "png" for example. However, calling X11_options() is expensive due */
		/* to the fork+execute of gnuplot_x11 and x11 can tolerate not being  */
		/* initialized until later.                                           */
		/* Note that GpGg.Gp__C.P_InputLine[] is blank at this point.	              */
		if(change_term(term_name, namelength)) {
			if(strcmp(term->name, "x11"))
				term->options(GpGg.Gp__C);
			return;
		}
		fprintf(stderr, "Unknown or ambiguous terminal name '%s'\n", term_name);
	}
	change_term("unknown", 7);
}
//
// test terminal by drawing border and text 
// called from command test 
//
//void test_term()
void GpGadgets::TestTerm(GpTermEntry * pT, GpCommand & rC)
{
	const char * str;
	int x, y, xl, yl, i;
	int xmax_t, ymax_t, x0, y0;
	char label[MAX_ID_LEN];
	int key_entry_height;
	int p_width;
	bool already_in_enhanced_text_mode;
	t_colorspec black; // = BLACK_COLORSPEC;
	black.SetBlack();
	already_in_enhanced_text_mode = (pT->flags & TERM_ENHANCED_TEXT) ? true : false;
	if(!already_in_enhanced_text_mode)
		rC.DoString("set termopt enh");
	TermStartPlot(pT);
	screen_ok = false;
	xmax_t = (int)(pT->xmax * XSz);
	ymax_t = (int)(pT->ymax * YSz);
	x0 = (int)(XOffs * pT->xmax);
	y0 = (int)(YOffs * pT->ymax);
	p_width = (int)(PtSz * pT->HTic);
	key_entry_height = (int)(PtSz * pT->VTic * 1.25);
	SETMAX(key_entry_height, (int)pT->VChr);
	// Sync point for epslatex text positioning 
	pT->_Layer(TERM_LAYER_FRONTTEXT);
	// border linetype 
	(*pT->linewidth)(1.0);
	pT->_LineType(LT_BLACK);
	{
		newpath(pT);
		pT->_Move(x0, y0);
		pT->_Vector(x0 + xmax_t - 1, y0);
		pT->_Vector(x0 + xmax_t - 1, y0 + ymax_t - 1);
		pT->_Vector(x0, y0 + ymax_t - 1);
		pT->_Vector(x0, y0);
		closepath(pT);
	}
	// Echo back the current terminal type
	if(!strcmp(term->name, "unknown"))
		IntErrorNoCaret("terminal type is unknown");
	else {
		char tbuf[64];
		(*pT->justify_text)(LEFT);
		sprintf(tbuf, "%s  terminal test", term->name);
		pT->_PutText(x0 + pT->HChr * 2, y0 + ymax_t - pT->VChr, tbuf);
		sprintf(tbuf, "gnuplot version %s.%s  ", gnuplot_version, gnuplot_patchlevel);
		pT->_PutText(x0 + pT->HChr * 2, (uint)(y0 + ymax_t - pT->VChr * 2.25), tbuf);
	}
	pT->_LineType(LT_AXIS);
	pT->_Move(x0 + xmax_t / 2, y0);
	pT->_Vector(x0 + xmax_t / 2, y0 + ymax_t - 1);
	pT->_Move(x0, y0 + ymax_t / 2);
	pT->_Vector(x0 + xmax_t - 1, y0 + ymax_t / 2);
	// test width and height of characters 
	pT->_LineType(LT_SOLID);
	{
		newpath(pT);
		pT->_Move(x0 + xmax_t / 2 - pT->HChr * 10, y0 + ymax_t / 2 + pT->VChr / 2);
		pT->_Vector(x0 + xmax_t / 2 + pT->HChr * 10, y0 + ymax_t / 2 + pT->VChr / 2);
		pT->_Vector(x0 + xmax_t / 2 + pT->HChr * 10, y0 + ymax_t / 2 - pT->VChr / 2);
		pT->_Vector(x0 + xmax_t / 2 - pT->HChr * 10, y0 + ymax_t / 2 - pT->VChr / 2);
		pT->_Vector(x0 + xmax_t / 2 - pT->HChr * 10, y0 + ymax_t / 2 + pT->VChr / 2);
		closepath(pT);
	}
	pT->_PutText(x0 + xmax_t / 2 - pT->HChr * 10, y0 + ymax_t / 2, "12345678901234567890");
	pT->_PutText(x0 + xmax_t / 2 - pT->HChr * 10, (uint)(y0 + ymax_t / 2 + pT->VChr * 1.4), "test of character width:");
	pT->_LineType(LT_BLACK);

	// Test for enhanced text 
	if(pT->flags & TERM_ENHANCED_TEXT) {
		char * tmptext1 =   "Enhanced text:   {x@_{0}^{n+1}}";
		char * tmptext2 = "&{Enhanced text:  }{/:Bold Bold}{/:Italic  Italic}";
		pT->_PutText((uint)(x0 + xmax_t * 0.5), (uint)(y0 + ymax_t * 0.40), tmptext1);
		pT->_PutText((uint)(x0 + xmax_t * 0.5), (uint)(y0 + ymax_t * 0.35), tmptext2);
		(*pT->set_font)("");
		if(!already_in_enhanced_text_mode)
			rC.DoString("set termopt noenh");
	}
	// test justification 
	(*pT->justify_text)(LEFT);
	pT->_PutText(x0 + xmax_t / 2, y0 + ymax_t / 2 + pT->VChr * 6, "left justified");
	str = "centre+d text";
	if((*pT->justify_text)(CENTRE))
		pT->_PutText(x0 + xmax_t / 2, y0 + ymax_t / 2 + pT->VChr * 5, str);
	else
		pT->_PutText(x0 + xmax_t / 2 - strlen(str) * pT->HChr / 2, y0 + ymax_t / 2 + pT->VChr * 5, str);
	str = "right justified";
	if((*pT->justify_text)(RIGHT))
		pT->_PutText(x0 + xmax_t / 2, y0 + ymax_t / 2 + pT->VChr * 4, str);
	else
		pT->_PutText(x0 + xmax_t / 2 - strlen(str) * pT->HChr, y0 + ymax_t / 2 + pT->VChr * 4, str);
	/* test text angle */
	pT->_LineType(1);
	str = "rotated ce+ntred text";
	if((*pT->text_angle)(TEXT_VERTICAL)) {
		if((*pT->justify_text)(CENTRE))
			pT->_PutText(x0 + pT->VChr, y0 + ymax_t / 2, str);
		else
			pT->_PutText(x0 + pT->VChr, y0 + ymax_t / 2 - strlen(str) * pT->HChr / 2, str);
		(*pT->justify_text)(LEFT);
		str = " rotated by +45 deg";
		(*pT->text_angle)(45);
		pT->_PutText(x0 + pT->VChr * 3, y0 + ymax_t / 2, str);
		(*pT->justify_text)(LEFT);
		str = " rotated by -45 deg";
		(*pT->text_angle)(-45);
		pT->_PutText(x0 + pT->VChr * 2, y0 + ymax_t / 2, str);
	}
	else {
		(*pT->justify_text)(LEFT);
		pT->_PutText(x0 + pT->HChr * 2, y0 + ymax_t / 2 - pT->VChr * 2, "can'pT rotate text");
	}
	(*pT->justify_text)(LEFT);
	(*pT->text_angle)(0);

	// test tic size 
	{
		const double _ts = AxA[FIRST_X_AXIS].ticscale;
		pT->_LineType(2);
		pT->_Move  ((uint)(x0 + xmax_t / 2 + pT->HTic * (1 + _ts)), y0 + (uint)ymax_t - 1);
		pT->_Vector((uint)(x0 + xmax_t / 2 + pT->HTic * (1 + _ts)), (uint)(y0 + ymax_t - _ts * pT->VTic));
		pT->_Move  ((uint)(x0 + xmax_t / 2), y0 + (uint)(ymax_t - pT->VTic * (1 + _ts)));
		pT->_Vector((uint)(x0 + xmax_t / 2 + _ts * pT->HTic), (uint)(y0 + ymax_t - pT->VTic * (1 + _ts)));
		(*pT->justify_text)(RIGHT);
		pT->_PutText(x0 + (uint)(xmax_t / 2 - 1* pT->HChr), y0 + (uint)(ymax_t - pT->VChr), "show ticscale");
		(*pT->justify_text)(LEFT);
		pT->_LineType(LT_BLACK);
	}
	// test line and point types 
	x = x0 + xmax_t - pT->HChr * 7 - p_width;
	y = y0 + ymax_t - key_entry_height;
	(*pT->pointsize)(PtSz);
	for(i = -2; y > y0 + key_entry_height; i++) {
		lp_style_type ls; // = DEFAULT_LP_STYLE_TYPE;
		ls.SetDefault2();
		ls.l_width = 1;
		load_linetype(&ls, i+1);
		ApplyLpProperties(pT, &ls);
		sprintf(label, "%d", i + 1);
		if((*pT->justify_text)(RIGHT))
			pT->_PutText(x, y, label);
		else
			pT->_PutText(x - strlen(label) * pT->HChr, y, label);
		pT->_Move(x + pT->HChr, y);
		pT->_Vector(x + pT->HChr * 5, y);
		if(i >= -1)
			(*pT->point)(x + pT->HChr * 6 + p_width / 2, y, i);
		y -= key_entry_height;
	}
	// test some arrows 
	(*pT->linewidth)(1.0);
	pT->_LineType(0);
	(*pT->dashtype)(DASHTYPE_SOLID, NULL);
	x = (int)(x0 + xmax_t * 0.28);
	y = (int)(y0 + ymax_t * 0.5);
	xl = pT->HTic * 7;
	yl = pT->VTic * 7;
	i = curr_arrow_headfilled;
	curr_arrow_headfilled = AS_NOFILL;
	(*pT->arrow)(x, y, x + xl, y, END_HEAD);
	curr_arrow_headfilled = (arrowheadfill)1;
	(*pT->arrow)(x, y, x - xl, y, END_HEAD);
	curr_arrow_headfilled = (arrowheadfill)2;
	(*pT->arrow)(x, y, x, y + yl, END_HEAD);
	curr_arrow_headfilled = AS_EMPTY;
	(*pT->arrow)(x, y, x, y - yl, END_HEAD);
	curr_arrow_headfilled = AS_NOBORDER;
	xl = pT->HTic * 5;
	yl = pT->VTic * 5;
	(*pT->arrow)(x - xl, y - yl, x + xl, y + yl, END_HEAD | BACKHEAD);
	(*pT->arrow)(x - xl, y + yl, x, y, NOHEAD);
	curr_arrow_headfilled = AS_EMPTY;
	(*pT->arrow)(x, y, x + xl, y - yl, BACKHEAD);
	curr_arrow_headfilled = (arrowheadfill)i;
	// test line widths 
	(void)(*pT->justify_text)(LEFT);
	xl = xmax_t / 10;
	yl = ymax_t / 25;
	x = (int)(x0 + xmax_t * 0.075);
	y = y0 + yl;
	for(i = 1; i<7; i++) {
		(*pT->linewidth)((float)(i)); pT->_LineType(LT_BLACK);
		pT->_Move(x, y); pT->_Vector(x+xl, y);
		sprintf(label, "  lw %1d", i);
		pT->_PutText(x+xl, y, label);
		y += yl;
	}
	pT->_PutText(x, y, "linewidth");
	// test native dashtypes (_not_ the 'set mono' sequence) 
	(void)(*pT->justify_text)(LEFT);
	xl = xmax_t / 10;
	yl = ymax_t / 25;
	x = (int)(x0 + xmax_t * 0.3);
	y = y0 + yl;
	for(i = 0; i<5; i++) {
		(*pT->linewidth)(1.0);
		pT->_LineType(LT_SOLID);
		(*pT->dashtype)(i, NULL);
		(*pT->set_color)(&black);
		pT->_Move(x, y); pT->_Vector(x+xl, y);
		sprintf(label, "  dt %1d", i+1);
		pT->_PutText(x+xl, y, label);
		y += yl;
	}
	pT->_PutText(x, y, "dashtype");
	// test fill patterns 
	x = (int)(x0 + xmax_t * 0.5);
	y = y0;
	xl = xmax_t / 40;
	yl = ymax_t / 8;
	(*pT->linewidth)((float)(1));
	pT->_LineType(LT_BLACK);
	(*pT->justify_text)(CENTRE);
	pT->_PutText(x+xl*7, (uint)(y + yl+pT->VChr*1.5), "pattern fill");
	for(i = 0; i<9; i++) {
		int style = ((i<<4) + FS_PATTERN);
		if(pT->fillbox)
			(*pT->fillbox)(style, x, y, xl, yl);
		{
			newpath(pT);
			pT->_Move(x, y);
			pT->_Vector(x, y+yl);
			pT->_Vector(x+xl, y+yl);
			pT->_Vector(x+xl, y);
			pT->_Vector(x, y);
			closepath(pT);
		}
		sprintf(label, "%2d", i);
		pT->_PutText(x+xl/2, (uint)(y+yl+pT->VChr*0.5), label);
		x += (int)(xl * 1.5);
	}

	{
		int cen_x = x0 + (int)(0.70 * xmax_t);
		int cen_y = y0 + (int)(0.83 * ymax_t);
		int radius = xmax_t / 20;

		/* test pm3d -- filled_polygon(), but not set_color() */
		if(pT->filled_polygon) {
			int i, j;
#define NUMBER_OF_VERTICES 6
			int n = NUMBER_OF_VERTICES;
			gpiPoint corners[NUMBER_OF_VERTICES+1];
#undef  NUMBER_OF_VERTICES
			for(j = 0; j<=1; j++) {
				int ix = cen_x + j*radius;
				int iy = cen_y - j*radius/2;
				for(i = 0; i < n; i++) {
					corners[i].x = (int)(ix + radius * cos(2* M_PI* i/n));
					corners[i].y = (int)(iy + radius * sin(2* M_PI* i/n));
				}
				corners[n].x = corners[0].x;
				corners[n].y = corners[0].y;
				if(j == 0) {
					pT->_LineType(2);
					corners->style = FS_OPAQUE;
				}
				else {
					pT->_LineType(1);
					corners->style = FS_TRANSPARENT_SOLID + (50<<4);
				}
				term->filled_polygon(n+1, corners);
			}
			str = "filled polygons:";
		}
		else
			str = "No filled polygons";
		pT->_LineType(LT_BLACK);
		i = ((*pT->justify_text)(CENTRE)) ? 0 : pT->HChr * strlen(str) / 2;
		pT->_PutText(cen_x + i, (uint)(cen_y + radius + pT->VChr * 0.5), str);
	}
	TermEndPlot(pT);
}

#ifdef VMS
/* these are needed to modify terminal characteristics */
#ifndef VWS_XMAX
/* avoid duplicate warning; VWS includes these */
	#include <descrip.h>
	#include <ssdef.h>
#endif                         /* !VWS_MAX */
	#include <iodef.h>
	#include <ttdef.h>
	#include <tt2def.h>
	#include <dcdef.h>
	#include <stat.h>
	#include <fab.h>
	// If you use WATCOM C or a very strict ANSI compiler, you may have to
	// delete or comment out the following 3 lines: */
	#ifndef TT2$M_DECCRT3          /* VT300 not defined as of VAXC v2.4 */
		#define TT2$M_DECCRT3 0X80000000
	#endif
	static ushort chan;
	static int old_char_buf[3], cur_char_buf[3];
	$DESCRIPTOR(sysoutput_desc, "SYS$OUTPUT");

/* Look first for decw$display (decterms do regis).  Determine if we
 * have a regis terminal and save terminal characteristics */
char * vms_init()
{
	/* Save terminal characteristics in old_char_buf and
	   initialise cur_char_buf to current settings. */
	int i;
#ifdef X11
	if(getenv("DECW$DISPLAY"))
		return ("x11");
#endif
	atexit(vms_reset);
	sys$assign(&sysoutput_desc, &chan, 0, 0);
	sys$qiow(0, chan, IO$_SENSEMODE, 0, 0, 0, old_char_buf, 12, 0, 0, 0, 0);
	for(i = 0; i < 3; ++i)
		cur_char_buf[i] = old_char_buf[i];
	sys$dassgn(chan);

	/* Test if terminal is regis */
	if((cur_char_buf[2] & TT2$M_REGIS) == TT2$M_REGIS)
		return ("regis");
	return (NULL);
}

/* set terminal to original state */
void vms_reset()
{
	int i;
	sys$assign(&sysoutput_desc, &chan, 0, 0);
	sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, old_char_buf, 12, 0, 0, 0, 0);
	for(i = 0; i < 3; ++i)
		cur_char_buf[i] = old_char_buf[i];
	sys$dassgn(chan);
}

/* set terminal mode to tektronix */
void term_mode_tek()
{
	long status;
	if(gpoutfile != stdout)
		return;         /* don't modify if not stdout */
	sys$assign(&sysoutput_desc, &chan, 0, 0);
	cur_char_buf[0] = 0x004A0000 | DC$_TERM | (TT$_TEK401X << 8);
	cur_char_buf[1] = (cur_char_buf[1] & 0x00FFFFFF) | 0x18000000;

	cur_char_buf[1] &= ~TT$M_CRFILL;
	cur_char_buf[1] &= ~TT$M_ESCAPE;
	cur_char_buf[1] &= ~TT$M_HALFDUP;
	cur_char_buf[1] &= ~TT$M_LFFILL;
	cur_char_buf[1] &= ~TT$M_MECHFORM;
	cur_char_buf[1] &= ~TT$M_NOBRDCST;
	cur_char_buf[1] &= ~TT$M_NOECHO;
	cur_char_buf[1] &= ~TT$M_READSYNC;
	cur_char_buf[1] &= ~TT$M_REMOTE;
	cur_char_buf[1] |= TT$M_LOWER;
	cur_char_buf[1] |= TT$M_TTSYNC;
	cur_char_buf[1] |= TT$M_WRAP;
	cur_char_buf[1] &= ~TT$M_EIGHTBIT;
	cur_char_buf[1] &= ~TT$M_MECHTAB;
	cur_char_buf[1] &= ~TT$M_SCOPE;
	cur_char_buf[1] |= TT$M_HOSTSYNC;

	cur_char_buf[2] &= ~TT2$M_APP_KEYPAD;
	cur_char_buf[2] &= ~TT2$M_BLOCK;
	cur_char_buf[2] &= ~TT2$M_DECCRT3;
	cur_char_buf[2] &= ~TT2$M_LOCALECHO;
	cur_char_buf[2] &= ~TT2$M_PASTHRU;
	cur_char_buf[2] &= ~TT2$M_REGIS;
	cur_char_buf[2] &= ~TT2$M_SIXEL;
	cur_char_buf[2] |= TT2$M_BRDCSTMBX;
	cur_char_buf[2] |= TT2$M_EDITING;
	cur_char_buf[2] |= TT2$M_INSERT;
	cur_char_buf[2] |= TT2$M_PRINTER;
	cur_char_buf[2] &= ~TT2$M_ANSICRT;
	cur_char_buf[2] &= ~TT2$M_AVO;
	cur_char_buf[2] &= ~TT2$M_DECCRT;
	cur_char_buf[2] &= ~TT2$M_DECCRT2;
	cur_char_buf[2] &= ~TT2$M_DRCS;
	cur_char_buf[2] &= ~TT2$M_EDIT;
	cur_char_buf[2] |= TT2$M_FALLBACK;

	status = sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, cur_char_buf, 12, 0, 0, 0, 0);
	if(status == SS$_BADPARAM) {
		/* terminal fallback utility not installed on system */
		cur_char_buf[2] &= ~TT2$M_FALLBACK;
		sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, cur_char_buf, 12, 0, 0, 0, 0);
	}
	else {
		if(status != SS$_NORMAL)
			lib$signal(status, 0, 0);
	}
	sys$dassgn(chan);
}

/* set terminal mode back to native */
void term_mode_native()
{
	int i;
	if(gpoutfile != stdout)
		return;         /* don't modify if not stdout */
	sys$assign(&sysoutput_desc, &chan, 0, 0);
	sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, old_char_buf, 12, 0, 0, 0, 0);
	for(i = 0; i < 3; ++i)
		cur_char_buf[i] = old_char_buf[i];
	sys$dassgn(chan);
}

/* set terminal mode pasthru */
void term_pasthru()
{
	if(gpoutfile != stdout)
		return;         /* don't modify if not stdout */
	sys$assign(&sysoutput_desc, &chan, 0, 0);
	cur_char_buf[2] |= TT2$M_PASTHRU;
	sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, cur_char_buf, 12, 0, 0, 0, 0);
	sys$dassgn(chan);
}

/* set terminal mode nopasthru */
void term_nopasthru()
{
	if(gpoutfile != stdout)
		return;         /* don't modify if not stdout */
	sys$assign(&sysoutput_desc, &chan, 0, 0);
	cur_char_buf[2] &= ~TT2$M_PASTHRU;
	sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, cur_char_buf, 12, 0, 0, 0, 0);
	sys$dassgn(chan);
}

void fflush_binary()
{
	typedef short int INT16; /* signed 16-bit integers */
	INT16 k;        /* loop index */
	if(gpoutfile != stdout) {
		/* Stupid VMS fflush() raises error and loses last data block
		   unless it is full for a fixed-length record binary file.
		   Pad it here with NULL characters. */
		for(k = (INT16)((*gpoutfile)->_cnt); k > 0; --k)
			putc('\0', gpoutfile);
		fflush(gpoutfile);
	}
}

#endif /* VMS */

/*
 * This is an abstraction of the enhanced text mode originally written
 * for the postscript terminal driver by David Denholm and Matt Heffron.
 * I have split out a terminal-independent recursive syntax-parser
 * routine that can be shared by all drivers that want to add support
 * for enhanced text mode.
 *
 * A driver that wants to make use of this common framework must provide
 * three new entries in TERM_TABLE:
 *      void *enhanced_open   (char *fontname, double fontsize, double base,
 *                             bool widthflag, bool showflag,
 *                             int overprint)
 *      void *enhanced_writec (char c)
 *      void *enhanced_flush  ()
 *
 * Each driver also has a separate ENHXX_put_text() routine that replaces
 * the normal (term->put_text) routine while in enhanced mode.
 * This routine must initialize the following globals used by the shared code:
 *      enhanced_fontscale      converts font size to device resolution units
 *      enhanced_escape_format  used to process octal escape characters \xyz
 *
 * I bent over backwards to make the output of the revised code identical
 * to the output of the original postscript version.  That means there is
 * some cruft left in here (enhanced_max_height for one thing) that is
 * probably irrelevant to any new drivers using the code.
 *
 * Ethan A Merritt - November 2003
 */

#ifdef DEBUG_ENH
	#define ENH_DEBUG(x) printf x;
#else
	#define ENH_DEBUG(x)
#endif

void do_enh_writec(int c)
{
	/* note: c is meant to hold a char, but is actually an int, for
	 * the same reasons applying to putc() and friends */
	*enhanced_cur_text++ = c;
}

/*
 * Process a bit of string, and return the last character used.
 * p is start of string
 * brace is true to keep processing to }, false to do one character only
 * fontname & fontsize are obvious
 * base is the current baseline
 * widthflag is true if the width of this should count,
 *              false for zero width boxes
 * showflag is true if this should be shown,
 *             false if it should not be shown (like TeX \phantom)
 * overprint is 0 for normal operation,
 *              1 for the underprinted text (included in width calculation),
 *              2 for the overprinted text (not included in width calc)
 *              (overprinted text is centered horizontally on underprinted text
 */

const char * enhanced_recursion(const char * p, bool brace,
    char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
{
	bool wasitalic, wasbold;

	/* Keep track of the style of the font passed in at this recursion level */
	wasitalic = (strstr(fontname, ":Italic") != NULL);
	wasbold = (strstr(fontname, ":Bold") != NULL);
	FPRINTF((stderr, "RECURSE WITH \"%s\", %d %s %.1f %.1f %d %d %d", p, brace, fontname, fontsize, base, widthflag, showflag, overprint));
	/* Start each recursion with a clean string */
	(term->enhanced_flush)();
	if(base + fontsize > enhanced_max_height) {
		enhanced_max_height = base + fontsize;
		ENH_DEBUG(("Setting max height to %.1f\n", enhanced_max_height));
	}
	if(base < enhanced_min_height) {
		enhanced_min_height = base;
		ENH_DEBUG(("Setting min height to %.1f\n", enhanced_min_height));
	}

	while(*p) {
		float shift;

		/*
		 * EAM Jun 2009 - treating bytes one at a time does not work for multibyte
		 * encodings, including utf-8. If we hit a byte with the high bit set, test
		 * whether it starts a legal UTF-8 sequence and if so copy the whole thing.
		 * Other multibyte encodings are still a problem.
		 * Gnuplot's other defined encodings are all single-byte; for those we
		 * really do want to treat one byte at a time.
		 */
		if((*p & 0x80) && (encoding == S_ENC_DEFAULT || encoding == S_ENC_UTF8)) {
			ulong utf8char;
			const char * nextchar = p;
			(term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
			if(utf8toulong(&utf8char, &nextchar)) { /* Legal UTF8 sequence */
				while(p < nextchar)
					(term->enhanced_writec)(*p++);
				p--;
			}
			else {                          /* Some other multibyte encoding? */
				(term->enhanced_writec)(*p);
			}
/* shige : for Shift_JIS */
		}
		else if((*p & 0x80) && (encoding == S_ENC_SJIS)) {
			(term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
			(term->enhanced_writec)(*(p++));
			(term->enhanced_writec)(*p);
		}
		else
			switch(*p) {
				case '}':
				    /*{{{  deal with it*/
				    if(brace)
					    return (p);
				    GpGg.IntWarn(NO_CARET, "enhanced text parser - spurious }");
				    break;
				/*}}}*/

				case '_':
				case '^':
				    /*{{{  deal with super/sub script*/
				    shift = (*p == '^') ? 0.5f : -0.3f;
				    (term->enhanced_flush)();
				    p = enhanced_recursion(p + 1, false, fontname, fontsize * 0.8,
				    base + shift * fontsize, widthflag,
				    showflag, overprint);
				    break;
				/*}}}*/
				case '{':
			    {
				    bool isitalic = false, isbold = false, isnormal = false;
				    const char * start_of_fontname = NULL;
				    const char * end_of_fontname = NULL;
				    char * localfontname = NULL;
				    char ch;
				    float f = (float)fontsize;
					float ovp;

				    /* Mar 2014 - this will hold "fontfamily{:Italic}{:Bold}" */
				    char * styledfontname = NULL;

				    /*{{{  recurse (possibly with a new font) */

				    ENH_DEBUG(("Dealing with {\n"));

				    /* get vertical offset (if present) for overprinted text */
				    while(*++p == ' ') ;
				    if(overprint == 2) {
					    char * end;
					    ovp = (float)strtod(p, &end);
					    p = end;
					    if(term->flags & TERM_IS_POSTSCRIPT)
						    base = ovp*f;
					    else
						    base += ovp*f;
				    }
				    --p; /* HBB 20001021: bug fix: 10^{2} broken */

				    if(*++p == '/') {
					    /* then parse a fontname, optional fontsize */
					    while(*++p == ' ')
						    ;  /* do nothing */
					    if(*p=='-') {
						    while(*++p == ' ')
							    ;  /* do nothing */
					    }
					    start_of_fontname = p;
					    while((ch = *p) > ' ' && ch != '=' && ch != '*' && ch != '}' && ch != ':')
						    ++p;
					    end_of_fontname = p;
					    do {
						    if(ch == '=') {
							    /* get optional font size */
							    char * end;
							    p++;
							    ENH_DEBUG(("Calling strtod(\"%s\") ...", p));
							    f = (float)strtod(p, &end);
							    p = end;
							    ENH_DEBUG(("Returned %.1f and \"%s\"\n", f, p));
							    if(f == 0)
								    f = (float)fontsize;
							    else
								    f *= (float)enhanced_fontscale;  /* remember the scaling */
							    ENH_DEBUG(("Font size %.1f\n", f));
						    }
						    else if(ch == '*') {
							    /* get optional font size scale factor */
							    char * end;
							    p++;
							    ENH_DEBUG(("Calling strtod(\"%s\") ...", p));
							    f = (float)strtod(p, &end);
							    p = end;
							    ENH_DEBUG(("Returned %.1f and \"%s\"\n", f, p));
							    if(f)
								    f *= (float)fontsize;  /* apply the scale factor */
							    else
								    f = (float)fontsize;
							    ENH_DEBUG(("Font size %.1f\n", f));
						    }
						    else if(ch == ':') {
							    /* get optional style markup attributes */
							    p++;
							    if(!strncmp(p, "Bold", 4))
								    isbold = true;
							    if(!strncmp(p, "Italic", 6))
								    isitalic = true;
							    if(!strncmp(p, "Normal", 6))
								    isnormal = true;
							    while(isalpha((uchar)*p)) {
								    p++;
							    }
						    }
					    } while(((ch = *p) == '=') || (ch == ':') || (ch == '*'));

					    if(ch == '}')
						    GpGg.IntWarn(NO_CARET, "bad syntax in enhanced text string");
					    if(*p == ' ') /* Eat up a single space following a font spec */
						    ++p;
					    if(!start_of_fontname || (start_of_fontname == end_of_fontname)) {
						    /* Use the font name passed in to us */
						    localfontname = gp_strdup(fontname);
					    }
					    else {
						    /* We found a new font name {/Font ...} */
						    int len = end_of_fontname - start_of_fontname;
						    localfontname = (char *)malloc(len+1);
						    strncpy(localfontname, start_of_fontname, len);
						    localfontname[len] = '\0';
					    }
				    }
				    /*}}}*/

				    /* Collect cumulative style markup before passing it in the font name */
				    isitalic = (wasitalic || isitalic) && !isnormal;
				    isbold = (wasbold || isbold) && !isnormal;
				    styledfontname = stylefont(localfontname ? localfontname : fontname, isbold, isitalic);
				    p = enhanced_recursion(p, true, styledfontname, f, base, widthflag, showflag, overprint);
				    (term->enhanced_flush)();
				    free(styledfontname);
				    free(localfontname);

				    break;
			    } /* case '{' */
				case '@':
				    /*{{{  phantom box - prints next 'char', then restores currentpoint */
				    (term->enhanced_flush)();
				    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, 3);
				    p = enhanced_recursion(++p, false, fontname, fontsize, base,
				    widthflag, showflag, overprint);
				    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, 4);
				    break;
				/*}}}*/

				case '&':
				    /*{{{  character skip - skips space equal to length of character(s) */
				    (term->enhanced_flush)();

				    p = enhanced_recursion(++p, false, fontname, fontsize, base,
				    widthflag, false, overprint);
				    break;
				/*}}}*/

				case '~':
				    /*{{{ overprinted text */
				    /* the second string is overwritten on the first, centered
				     * horizontally on the first and (optionally) vertically
				     * shifted by an amount specified (as a fraction of the
				     * current fontsize) at the beginning of the second string

				     * Note that in this implementation neither the under- nor
				     * overprinted string can contain syntax that would result
				     * in additional recursions -- no subscripts,
				     * superscripts, or anything else, with the exception of a
				     * font definition at the beginning of the text */

				    (term->enhanced_flush)();
				    p = enhanced_recursion(++p, false, fontname, fontsize, base,
				    widthflag, showflag, 1);
				    (term->enhanced_flush)();
				    if(!*p)
					    break;
				    p = enhanced_recursion(++p, false, fontname, fontsize, base,
				    false, showflag, 2);

				    overprint = 0; /* may not be necessary, but just in case . . . */
				    break;
				/*}}}*/

				case '(':
				case ')':
				    /*{{{  an escape and print it */
				    /* special cases */
				    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
				    if(term->flags & TERM_IS_POSTSCRIPT)
					    (term->enhanced_writec)('\\');
				    (term->enhanced_writec)(*p);
				    break;
				/*}}}*/

				case '\\':
				    /*{{{  Enhanced mode always uses \xyz as an octal character representation
				           but each terminal type must give us the actual output format wanted.
				           pdf.trm wants the raw character code, which is why we use strtol();
				           most other terminal types want some variant of "\\%o". */
				    if(p[1] >= '0' && p[1] <= '7') {
					    char * e, escape[16], octal[4] = {'\0', '\0', '\0', '\0'};

					    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
					    octal[0] = *(++p);
					    if(p[1] >= '0' && p[1] <= '7') {
						    octal[1] = *(++p);
						    if(p[1] >= '0' && p[1] <= '7')
							    octal[2] = *(++p);
					    }
					    sprintf(escape, enhanced_escape_format, strtol(octal, NULL, 8));
					    for(e = escape; *e; e++) {
						    (term->enhanced_writec)(*e);
					    }
					    break;
					    /* This was the original (prior to version 4) enhanced text code specific */
					    /* to the reserved characters of PostScript.  Some of it was mis-applied  */
					    /* to other terminal types until fixed in Mar 2012.                       */
				    }
				    else if(term->flags & TERM_IS_POSTSCRIPT) {
					    if(p[1]=='\\' || p[1]=='(' || p[1]==')') {
						    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
						    (term->enhanced_writec)('\\');
					    }
					    else if(strchr("^_@&~{}", p[1]) == NULL) {
						    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
						    (term->enhanced_writec)('\\');
						    (term->enhanced_writec)('\\');
						    break;
					    }
				    }
				    ++p;

				    /* HBB 20030122: Avoid broken output if there's a \
				     * exactly at the end of the line */
				    if(*p == '\0') {
					    GpGg.IntWarn(NO_CARET, "enhanced text parser -- spurious backslash");
					    break;
				    }

				    /* SVG requires an escaped '&' to be passed as something else */
				    /* FIXME: terminal-dependent code does not belong here */
				    if(*p == '&' && encoding == S_ENC_DEFAULT && !strcmp(term->name, "svg")) {
					    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
					    (term->enhanced_writec)('\376');
					    break;
				    }

				/* just go and print it (fall into the 'default' case) */
				/*}}}*/
				default:
				    /*{{{  print it */
				    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
				    (term->enhanced_writec)(*p);
				    /*}}}*/
			} /* switch (*p) */

		/* like TeX, we only do one character in a recursion, unless it's
		 * in braces
		 */

		if(!brace) {
			(term->enhanced_flush)();
			return(p); /* the ++p in the outer copy will increment us */
		}

		if(*p) /* only not true if { not terminated, I think */
			++p;
	} /* while (*p) */

	(term->enhanced_flush)();
	return p;
}

/* Strip off anything trailing the requested font name,
 * then add back markup requests.
 */
char * stylefont(const char * fontname, bool isbold, bool isitalic)
{
	char * div;
	char * markup = (char *)malloc(strlen(fontname) + 16);
	strcpy(markup, fontname);
	if((div = strchr(markup, ':')))
		*div = '\0';
	if(isbold)
		strcat(markup, ":Bold");
	if(isitalic)
		strcat(markup, ":Italic");

	FPRINTF((stderr, "MARKUP FONT: %s -> %s\n", fontname, markup));
	return markup;
}

/* Called after the end of recursion to check for errors */
void enh_err_check(const char * str)
{
	if(*str == '}')
		GpGg.IntWarn(NO_CARET, "enhanced text mode parser - ignoring spurious }");
	else
		GpGg.IntWarn(NO_CARET, "enhanced text mode parsing error");
}

/*
 * Text strings containing control information for enhanced text mode
 * contain more characters than will actually appear in the output.
 * This makes it hard to estimate how much horizontal space on the plot
 * (e.g. in the key box) must be reserved to hold them.  To approximate
 * the eventual length we switch briefly to the dummy terminal driver
 * "estimate.trm" and then switch back to the current terminal.
 * If better, perhaps terminal-specific methods of estimation are
 * developed later they can be slotted into this one call site.
 */
int estimate_strlen(char * text)
{
	int len;
	if((term->flags & TERM_IS_LATEX))
		len = strlen_tex(text);
	else
#ifdef GP_ENH_EST
	if(strchr(text, '\n') || (term->flags & TERM_ENHANCED_TEXT)) {
		GpTermEntry * tsave = term;
		term = &ENHest;
		term->put_text(0, 0, text);
		len = term->xmax;
		FPRINTF((stderr, "Estimating length %d height %g for enhanced text string \"%s\"\n", len, (double)(term->ymax)/10., text));
		term = tsave;
	}
	else if(encoding == S_ENC_UTF8)
		len = strwidth_utf8(text);
	else
#endif
		len = strlen(text);
	return len;
}
//
// Use estimate.trm to mock up a non-enhanced approximation of the original string.
//
char * estimate_plaintext(char * enhancedtext)
{
	if(enhancedtext) {
		estimate_strlen(enhancedtext);
		return ENHest_plaintext;
	}
	else
		return 0;
}

void ignore_enhanced(bool flag)
{
	/* Force a return to the default font */
	if(flag && !ignore_enhanced_text) {
		ignore_enhanced_text = true;
		term->set_font("");
	}
	ignore_enhanced_text = flag;
}

/* Simple-minded test for whether the point (x,y) is in bounds for the current terminal.
 * Some terminals can do their own clipping, and can clip partial objects.
 * If the flag TERM_CAN_CLIP is set, we skip this relative crude test and let the
 * driver or the hardware handle clipping.
 */
bool on_page(int x, int y)
{
	if(term->flags & TERM_CAN_CLIP)
		return true;
	else if((0 < x && x < (int)term->xmax) && (0 < y && y < (int)term->ymax))
		return true;
	else
		return false;
}
//
// Utility routine for drivers to accept an explicit size for the output image.
//
//size_units parse_term_size(float * xsize, float * ysize, size_units default_units)
size_units GpCommand::ParseTermSize(float * pXSize, float * pYSize, size_units defaultUnits)
{
	size_units units = defaultUnits;
	if(EndOfCommand())
		GpGg.IntErrorCurToken("size requires two numbers:  pXSize, pYSize");
	*pXSize = (float)RealExpression();
	if(AlmostEq("in$ches")) {
		CToken++;
		units = INCHES;
	}
	else if(Eq("cm")) {
		CToken++;
		units = CM;
	}
	switch(units) {
		case INCHES: *pXSize *= gp_resolution; break;
		case CM:     *pXSize *= (float)gp_resolution / 2.54f; break;
		case PIXELS:
		default:             break;
	}
	if(!Eq(CToken++, ","))
		GpGg.IntErrorCurToken("size requires two numbers:  pXSize, pYSize");
	*pYSize = (float)RealExpression();
	if(AlmostEq("in$ches")) {
		CToken++;
		units = INCHES;
	}
	else if(Eq("cm")) {
		CToken++;
		units = CM;
	}
	switch(units) {
		case INCHES:        *pYSize *= gp_resolution; break;
		case CM:            *pYSize *= (float)gp_resolution / 2.54f; break;
		case PIXELS:
		default:             break;
	}
	if(*pXSize < 1 || *pYSize < 1)
		GpGg.IntErrorCurToken("size: out of range");
	return units;
}
//
// Wrappers for newpath and closepath
//
void newpath(GpTermEntry * pT)
{
	if(pT->path)
		pT->path(0);
}

void closepath(GpTermEntry * pT)
{
	if(pT->path)
		pT->path(1);
}

/* Squeeze all fill information into the old style parameter.
 * The terminal drivers know how to extract the information.
 * We assume that the style (int) has only 16 bit, therefore we take
 * 4 bits for the style and allow 12 bits for the corresponding fill parameter.
 * This limits the number of styles to 16 and the fill parameter's
 * values to the range 0...4095, which seems acceptable.
 */
int style_from_fill(fill_style_type * fs)
{
	int fillpar, style;
	switch(fs->fillstyle) {
		case FS_SOLID:
		case FS_TRANSPARENT_SOLID:
		    fillpar = fs->filldensity;
		    style = ((fillpar & 0xfff) << 4) + fs->fillstyle;
		    break;
		case FS_PATTERN:
		case FS_TRANSPARENT_PATTERN:
		    fillpar = fs->fillpattern;
		    style = ((fillpar & 0xfff) << 4) + fs->fillstyle;
		    break;
		case FS_EMPTY:
		default:
		    style = FS_EMPTY; // solid fill with background color 
		    break;
	}
	return style;
}
//
// Load dt with the properties of a user-defined dashtype.
// Return: DASHTYPE_SOLID or DASHTYPE_CUSTOM or a positive number
// if no user-defined dashtype was found.
//
int load_dashtype(t_dashtype * dt, int tag)
{
	t_dashtype loc_dt = DEFAULT_DASHPATTERN;
	custom_dashtype_def * p_this = GpGg.first_custom_dashtype;
	while(p_this != NULL) {
		if(p_this->tag == tag) {
			*dt = p_this->dashtype;
			memcpy(dt->dstring, p_this->dashtype.dstring, sizeof(dt->dstring));
			return p_this->d_type;
		}
		else {
			p_this = p_this->next;
		}
	}
	// not found, fall back to default, terminal-dependent dashtype
	*dt = loc_dt;
	return tag - 1;
}

void lp_use_properties(lp_style_type * lp, int tag)
{
	/*  This function looks for a linestyle defined by 'tag' and copies
	 *  its data into the structure 'lp'.
	 */

	linestyle_def * p_this;
	int save_flags = lp->flags;
	p_this = GpGg.first_linestyle;
	while(p_this != NULL) {
		if(p_this->tag == tag) {
			*lp = p_this->lp_properties;
			lp->flags = save_flags;
			return;
		}
		else {
			p_this = p_this->next;
		}
	}

	/* No user-defined style with p_this tag; fall back to default line type. */
	load_linetype(lp, tag);
}
//
// Load lp with the properties of a user-defined linetype
//
void load_linetype(lp_style_type * lp, int tag)
{
	linestyle_def * p_this;
	bool recycled = false;
recycle:
	if((tag > 0) && (GpGg.IsMonochrome || (term->flags & TERM_MONOCHROME))) {
		for(p_this = GpGg.first_mono_linestyle; p_this; p_this = p_this->next) {
			if(tag == p_this->tag) {
				*lp = p_this->lp_properties;
				return;
			}
		}

		/* This linetype wasn't defined explicitly.		*/
		/* Should we recycle one of the first N linetypes?	*/
		if(tag > mono_recycle_count && mono_recycle_count > 0) {
			tag = (tag-1) % mono_recycle_count + 1;
			goto recycle;
		}

		return;
	}
	p_this = GpGg.first_perm_linestyle;
	while(p_this != NULL) {
		if(p_this->tag == tag) {
			/* Always load color, width, and dash properties */
			lp->l_type = p_this->lp_properties.l_type;
			lp->l_width = p_this->lp_properties.l_width;
			lp->pm3d_color = p_this->lp_properties.pm3d_color;
			lp->d_type = p_this->lp_properties.d_type;
			lp->custom_dash_pattern = p_this->lp_properties.custom_dash_pattern;

			/* Needed in version 5.0 to handle old terminals (pbm hpgl ...) */
			/* with no support for user-specified colors */
			if(term->set_color == null_set_color)
				lp->l_type = tag;

			/* Do not recycle point properties. */
			/* FIXME: there should be a separate command "set pointtype cycle N" */
			if(!recycled) {
				lp->p_type = p_this->lp_properties.p_type;
				lp->p_interval = p_this->lp_properties.p_interval;
				lp->p_size = p_this->lp_properties.p_size;
				memcpy(lp->p_char, p_this->lp_properties.p_char, sizeof(lp->p_char));
			}
			return;
		}
		else {
			p_this = p_this->next;
		}
	}

	/* This linetype wasn't defined explicitly.		*/
	/* Should we recycle one of the first N linetypes?	*/
	if(tag > linetype_recycle_count && linetype_recycle_count > 0) {
		tag = (tag-1) % linetype_recycle_count + 1;
		recycled = true;
		goto recycle;
	}

	/* No user-defined linetype with p_this tag; fall back to default line type. */
	/* NB: We assume that the remaining fields of lp have been initialized. */
	lp->l_type = tag - 1;
	lp->pm3d_color.type = TC_LT;
	lp->pm3d_color.lt = lp->l_type;
	lp->d_type = DASHTYPE_SOLID;
	lp->p_type = (tag <= 0) ? -1 : tag - 1;
}

/*
 * Version 5 maintains a parallel set of linetypes for "set monochrome" mode.
 * This routine allocates space and initializes the default set.
 */
void init_monochrome()
{
	lp_style_type mono_default[6];// = DEFAULT_MONO_LINETYPES;
	// #define DEFAULT_LP_STYLE_TYPE {0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, DEFAULT_COLORSPEC, DEFAULT_DASHPATTERN}
	for(uint i = 0; i < SIZEOFARRAY(mono_default); i++) {
		mono_default[i].SetDefault2();
		mono_default[i].pm3d_color.SetBlack();
	}
	/*
		flags = 0;
		l_type = LT_BLACK;
		p_type = 0;
		d_type = DASHTYPE_SOLID;
		p_interval = 0;
		l_width = 1.0;
		p_size = PTSZ_DEFAULT;
		MEMSZERO(p_char);
		pm3d_color.SetDefault();
		custom_dash_pattern.SetDefault();
	*/
	mono_default[1].d_type = 1;
	mono_default[2].d_type = 2;
	mono_default[3].d_type = 3;
	mono_default[4].d_type = 0;
	mono_default[4].l_width = 2.0;
	mono_default[5].d_type = DASHTYPE_CUSTOM;
	mono_default[5].l_width = 1.2;
	mono_default[5].custom_dash_pattern.SetPattern(16.f, 8.0f, 2.0f, 5.0f, 2.0f, 5.0f, 2.0f, 8.0f);
	if(GpGg.first_mono_linestyle == NULL) {
		int i, n = sizeof(mono_default) / sizeof(lp_style_type);
		linestyle_def * p_new;
		/* copy default list into active list */
		for(i = n; i>0; i--) {
			p_new = (linestyle_def *)malloc(sizeof(linestyle_def));
			p_new->next = GpGg.first_mono_linestyle;
			p_new->lp_properties = mono_default[i-1];
			p_new->tag = i;
			GpGg.first_mono_linestyle = p_new;
		}
	}
}

/*
 * Totally bogus estimate of TeX string lengths.
 * Basically
 * - don't count anything inside square braces
 * - count regexp \[a-zA-z]* as a single character
 * - ignore characters {}$^_
 */
int strlen_tex(const char * str)
{
	const char * s = str;
	int len = 0;

	if(!strpbrk(s, "{}$[]\\")) {
		len = strlen(s);
		FPRINTF((stderr, "strlen_tex(\"%s\") = %d\n", s, len));
		return len;
	}

	while(*s) {
		switch(*s) {
			case '[':
			    while(*s && *s != ']') s++;
			    s++;
			    break;
			case '\\':
			    s++;
			    while(*s && isalpha((uchar)*s)) s++;
			    len++;
			    break;
			case '{':
			case '}':
			case '$':
			case '_':
			case '^':
			    s++;
			    break;
			default:
			    s++;
			    len++;
		}
	}

	FPRINTF((stderr, "strlen_tex(\"%s\") = %d\n", str, len));
	return len;
}

/* The check for asynchronous events such as hotkeys and mouse clicks is
 * normally done in term->waitforinput() while waiting for the next input
 * from the command line.  If input is currently coming from a file or
 * pipe instead, as with a "load" command, then this path would not be
 * triggered automatically and these events would back up until input
 * returned to the command line.  These code paths can explicitly call
 * check_for_mouse_events() so that event processing is handled sooner.
 */
void check_for_mouse_events()
{
#ifdef USE_MOUSE
	if(GpGg.term_initialised && term->waitforinput) {
		term->waitforinput(TERM_ONLY_CHECK_MOUSING);
	}
#endif
#ifdef WIN32
	// Process windows GUI events (e.g. for text window, or wxt and windows terminals) 
	WinMessageLoop();
	// On Windows, Ctrl-C only sets this flag. 
	// The next block duplicates the behaviour of inter(). 
	if(GpGg.ctrlc_flag) {
		GpGg.ctrlc_flag = false;
		term_reset();
		putc('\n', stderr);
		fprintf(stderr, "Ctrl-C detected!\n");
		bail_to_command_line(); // return to prompt 
	}
#endif
}

