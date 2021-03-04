// GNUPLOT - term.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
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
 * term_initialise()  : optional. Prepare the terminal for first use. It protects itself against subsequent calls.
 * term_start_plot() : called at start of graph output. Calls term_init if necessary
 * term_apply_lp_properties() : apply linewidth settings
 * term_end_plot() : called at the end of a plot
 * term_reset() : called during int_error handling, to shut terminal down cleanly
 * term_start_multiplot() : called by   set multiplot
 * term_end_multiplot() : called by  set nomultiplot
 * term_check_multiplot_okay() : called just before an interactive prompt is issued while in multiplot mode,
 *   to allow terminal to suspend if necessary, Raises an error if interactive multiplot is not supported.
 */
#include <gnuplot.h>
#pragma hdrstop
#include "driver.h"
#include "term.h"
#ifndef USE_MOUSE
	// Some terminals (svg canvas) can provide mousing information 
	// even if the interactive gnuplot session itself cannot.      
	long mouse_mode = 0;
	char* mouse_alt_string = NULL;
	#define MOUSE_COORDINATES_FUNCTION 8    /* Normally an enum in mouse.h */
#endif
#ifdef _WIN32
	#include "win/winmain.h"
	#include "win/wcommon.h"
#endif

static int termcomp(const generic * a, const generic * b);

// Externally visible variables 
// the central instance: the current terminal's interface structure 
struct GpTermEntry * term = NULL;  /* unknown */
char term_options[MAX_LINE_LEN+1] = ""; /* ... and its options string */

// the 'output' file name and handle 
char * outstr = NULL;            /* means "STDOUT" */
FILE * gpoutfile;
// Output file where the PostScript output goes to. See term_api.h for more details. 
FILE * gppsfile = 0;
char * PS_psdir = NULL;
char * PS_fontpath = NULL;
//bool term_initialised; /* true if terminal has been initialized */
// The qt and wxt terminals cannot be used in the same session. 
// Whichever one is used first to plot, this locks out the other. 
void * term_interlock = NULL;
bool monochrome = FALSE; /* true if "set monochrome" */
bool multiplot = FALSE; /* true if in multiplot mode */
int multiplot_count = 0;
enum set_encoding_id encoding; /* text output encoding, for terminals that support it */
// table of encoding names, for output of the setting 
const char * encoding_names[] = {
	"default", "iso_8859_1", "iso_8859_2", "iso_8859_9", "iso_8859_15",
	"cp437", "cp850", "cp852", "cp950", "cp1250", "cp1251", "cp1252", "cp1254", "koi8r", "koi8u", "sjis", "utf8", NULL
};
// 'set encoding' options 
const struct gen_table set_encoding_tbl[] = {
	{ "def$ault",    S_ENC_DEFAULT },
	{ "utf$8",       S_ENC_UTF8 },
	{ "iso$_8859_1", S_ENC_ISO8859_1 },
	{ "iso_8859_2",  S_ENC_ISO8859_2 },
	{ "iso_8859_9",  S_ENC_ISO8859_9 },
	{ "iso_8859_15", S_ENC_ISO8859_15 },
	{ "cp4$37",      S_ENC_CP437 },
	{ "cp850",       S_ENC_CP850 },
	{ "cp852",       S_ENC_CP852 },
	{ "cp950",       S_ENC_CP950 },
	{ "cp1250",      S_ENC_CP1250 },
	{ "cp1251",      S_ENC_CP1251 },
	{ "cp1252",      S_ENC_CP1252 },
	{ "cp1254",      S_ENC_CP1254 },
	{ "koi8$r",      S_ENC_KOI8_R },
	{ "koi8$u",      S_ENC_KOI8_U },
	{ "sj$is",       S_ENC_SJIS },
	{ NULL,          S_ENC_INVALID }
};

const char * arrow_head_names[4] = {"nohead", "head", "backhead", "heads"};

enum { IPC_BACK_UNUSABLE = -2, IPC_BACK_CLOSED = -1 };
//
// Support for enhanced text mode. Declared extern in term_api.h 
//
// Recycle count for user-defined linetypes 
int    linetype_recycle_count = 0;
int    mono_recycle_count = 0;

// Internal prototypes: 
//static void term_close_output();
static void null_linewidth(GpTermEntry * pTerm, double);
static void null_dashtype(GpTermEntry * pTerm, int type, t_dashtype * custom_dash_pattern);
static void null_layer(GpTermEntry * pThis, t_termlayer layer);
static int  null_set_font(GpTermEntry * pThis, const char * font);
static void graphics_null(GpTermEntry * pThis);
static void UNKNOWN_null(GpTermEntry * pThis);
static void MOVE_null(GpTermEntry * pThis, uint, uint);
static void LINETYPE_null(GpTermEntry * pThis, int);
static void PUTTEXT_null(GpTermEntry * pThis, uint, uint, const char *);
static int strlen_tex(const char *);

#define FOPEN_BINARY(file) fopen(file, "wb")
#if defined(MSDOS) || defined(_WIN32)
	#if defined(__DJGPP__)
	#include <io.h>
	#endif
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
	#include <io.h> /* for setmode() */
#endif
#define NICE_LINE               0
#define POINT_TYPES             6
#ifndef DEFAULTTERM
	#define DEFAULTTERM NULL
#endif
// 
// interface to the rest of gnuplot - the rules are getting
// too complex for the rest of gnuplot to be allowed in
// 
#if defined(PIPES)
	static bool output_pipe_open = FALSE;
#endif

//static void term_close_output()
void GnuPlot::TermCloseOutput(GpTermEntry * pTerm)
{
	FPRINTF((stderr, "term_close_output\n"));
	TermOpenedBinary = false;
	if(outstr) { // ie using stdout 
	#if defined(PIPES)
		if(output_pipe_open) {
			pclose(gpoutfile);
			output_pipe_open = FALSE;
		}
		else
	#endif
	#ifdef _WIN32
		if(sstreqi_ascii(outstr, "PRN"))
			ClosePrinter(pTerm, gpoutfile);
		else
	#endif
		if(gpoutfile != gppsfile)
			fclose(gpoutfile);
		gpoutfile = stdout;     /* Don't dup... */
		ZFREE(outstr);
		SFile::ZClose(&gppsfile);
	}
}
// 
// assigns dest to outstr, so it must be allocated or NULL
// and it must not be outstr itself !
// 
//void term_set_output(char * dest)
void GnuPlot::TermSetOutput(GpTermEntry * pTerm, char * pDest)
{
	FILE * f = NULL;
	FPRINTF((stderr, "term_set_output\n"));
	assert(pDest == NULL || pDest != outstr);
	if(multiplot) {
		fputs("In multiplot mode you can't change the output\n", stderr);
		return;
	}
	if(pTerm && TermInitialised) {
		(pTerm->reset)(pTerm);
		pTerm->P_Gp = 0;
		TermInitialised = false;
		// switch off output to special postscript file (if used) 
		gppsfile = NULL;
	}
	if(!pDest) { // stdout 
		TermCloseOutput(pTerm);
	}
	else {
#if defined(PIPES)
		if(*pDest == '|') {
			RestrictPOpen();
#if defined(_WIN32 ) || defined(MSDOS)
			if(pTerm && (pTerm->flags & TERM_BINARY))
				f = popen(pDest + 1, "wb");
			else
				f = popen(pDest + 1, "w");
#else
			f = popen(pDest + 1, "w");
#endif
			if(f == (FILE*)NULL)
				os_error(Pgm.GetCurTokenIdx(), "cannot create pipe; output not changed");
			else
				output_pipe_open = TRUE;
		}
		else {
#endif /* PIPES */
#ifdef _WIN32
		if(outstr && sstreqi_ascii(outstr, "PRN")) {
			// we can't call open_printer() while printer is open, so 
			ClosePrinter(pTerm, gpoutfile); /* close printer immediately if open */
			gpoutfile = stdout; /* and reset output to stdout */
			SAlloc::F(outstr);
			outstr = NULL;
		}
		if(sstreqi_ascii(pDest, "PRN")) {
			if((f = open_printer()) == (FILE*)NULL)
				os_error(Pgm.GetCurTokenIdx(), "cannot open printer temporary file; output may have changed");
		}
		else
#endif
		{
			if(pTerm && (pTerm->flags & TERM_BINARY))
				f = FOPEN_BINARY(pDest);
			else
				f = fopen(pDest, "w");
			if(!f)
				os_error(Pgm.GetCurTokenIdx(), "cannot open file; output not changed");
		}
#if defined(PIPES)
	}
#endif
		TermCloseOutput(pTerm);
		gpoutfile = f;
		outstr = pDest;
		TermOpenedBinary = (pTerm && (pTerm->flags & TERM_BINARY));
	}
}

//void term_initialise()
void GnuPlot::TermInitialise(GpTermEntry * pTerm)
{
	FPRINTF((stderr, "term_initialise()\n"));
	if(!pTerm)
		IntError(NO_CARET, "No terminal defined");
	// check if we have opened the output file in the wrong mode
	// (text/binary), if set term comes after set output
	// This was originally done in change_term, but that
	// resulted in output files being truncated
	if(outstr && (pTerm->flags & TERM_NO_OUTPUTFILE)) {
		if(_Plt.interactive)
			fprintf(stderr, "Closing %s\n", outstr);
		TermCloseOutput(pTerm);
	}
	if(outstr && (((pTerm->flags & TERM_BINARY) && !TermOpenedBinary) || ((!(pTerm->flags & TERM_BINARY) && TermOpenedBinary)))) {
		// this is nasty - we cannot just term_set_output(outstr)
		// since term_set_output will first free outstr and we
		// end up with an invalid pointer. I think I would
		// prefer to defer opening output file until first plot.
		char * temp = (char *)SAlloc::M(strlen(outstr) + 1);
		if(temp) {
			FPRINTF((stderr, "term_initialise: reopening \"%s\" as %s\n", outstr, pTerm->flags & TERM_BINARY ? "binary" : "text"));
			strcpy(temp, outstr);
			TermSetOutput(pTerm, temp); /* will free outstr */
			if(temp != outstr) {
				SAlloc::F(temp);
				temp = outstr;
			}
		}
		else
			fputs("Cannot reopen output file in binary", stderr);
		// and carry on, hoping for the best ! 
	}
#if defined(_WIN32)
#ifdef _WIN32
	else if(!outstr && (pTerm->flags & TERM_BINARY))
#else
	else if(!outstr && !interactive && (pTerm->flags & TERM_BINARY))
#endif
	{
#if defined(_WIN32) && !defined(WGP_CONSOLE)
#ifdef PIPES
		if(!output_pipe_open)
#endif
		if(!outstr && !(pTerm->flags & TERM_NO_OUTPUTFILE))
			IntErrorCurToken("cannot output binary data to wgnuplot text window");
#endif
		// binary to stdout in non-interactive session... 
		fflush(stdout);
		_setmode(_fileno(stdout), O_BINARY);
	}
#endif
	if(!TermInitialised || TermForceInit) {
		FPRINTF((stderr, "- calling term->init()\n"));
		pTerm->P_Gp = this;
		(pTerm->init)(pTerm);
		TermInitialised = true;
#ifdef HAVE_LOCALE_H
		// This is here only from an abundance of caution (a.k.a. paranoia).
		// Some terminals (wxt qt caca) are known to change the locale when
		// initialized.  Others have been implicated (gd).  Rather than trying
		// to catch all such offenders one by one, cover for all of them here.
		setlocale(LC_NUMERIC, "C");
#endif
	}
}

//void term_start_plot()
void GnuPlot::TermStartPlot(GpTermEntry * pTerm)
{
	FPRINTF((stderr, "GnuPlot::TermStartPlot()\n"));
	if(!TermInitialised)
		TermInitialise(pTerm);
	if(!TermGraphics) {
		FPRINTF((stderr, "- calling term->graphics()\n"));
		(pTerm->graphics)(pTerm);
		TermGraphics = true;
	}
	else if(multiplot && TermSuspended) {
		if(pTerm->resume) {
			FPRINTF((stderr, "- calling term->resume()\n"));
			(pTerm->resume)(pTerm);
		}
		TermSuspended = false;
	}
	if(multiplot)
		multiplot_count++;
	(pTerm->layer)(pTerm, TERM_LAYER_RESET); // Sync point for epslatex text positioning 
	// Because PostScript plots may be viewed out of order, make sure 
	// Each new plot makes no assumption about the previous palette.
	if(pTerm->flags & TERM_IS_POSTSCRIPT)
		invalidate_palette();
	// Set canvas size to full range of current terminal coordinates 
	V.BbCanvas.xleft  = 0;
	V.BbCanvas.xright = pTerm->MaxX - 1;
	V.BbCanvas.ybot   = 0;
	V.BbCanvas.ytop   = pTerm->MaxY - 1;
}

//void term_end_plot()
void GnuPlot::TermEndPlot(GpTermEntry * pTerm)
{
	FPRINTF((stderr, "term_end_plot()\n"));
	if(TermInitialised) {
		// Sync point for epslatex text positioning 
		(pTerm->layer)(pTerm, TERM_LAYER_END_TEXT);
		if(!multiplot) {
			FPRINTF((stderr, "- calling term->text()\n"));
			(pTerm->text)(pTerm);
			TermGraphics = FALSE;
		}
		else {
			MultiplotNext();
		}
		fflush(gpoutfile);
#ifdef USE_MOUSE
		if(pTerm->set_ruler) {
			RecalcStatusLine();
			UpdateRuler(pTerm);
		}
#endif
	}
}

//static void term_suspend()
void GnuPlot::TermSuspend(GpTermEntry * pTerm)
{
	FPRINTF((stderr, "term_suspend()\n"));
	if(TermInitialised && !TermSuspended && pTerm->suspend) {
		FPRINTF((stderr, "- calling term->suspend()\n"));
		(pTerm->suspend)(pTerm);
		TermSuspended = true;
	}
}

//void term_reset()
void GnuPlot::TermReset(GpTermEntry * pTerm)
{
	FPRINTF((stderr, "term_reset()\n"));
#ifdef USE_MOUSE
	// Make sure that ^C will break out of a wait for 'pause mouse' 
	paused_for_mouse = 0;
#ifdef _WIN32
	kill_pending_Pause_dialog();
#endif
#endif
	if(TermInitialised) {
		if(TermSuspended) {
			if(pTerm->resume) {
				FPRINTF((stderr, "- calling term->resume()\n"));
				(pTerm->resume)(pTerm);
			}
			TermSuspended = false;
		}
		if(TermGraphics) {
			(pTerm->text)(pTerm);
			TermGraphics = false;
		}
		if(TermInitialised) {
			(pTerm->reset)(pTerm);
			pTerm->P_Gp = 0;
			TermInitialised = false;
			// switch off output to special postscript file (if used) 
			gppsfile = NULL;
		}
	}
}

//void term_apply_lp_properties(GpTermEntry * pTerm, const lp_style_type * lp)
void GnuPlot::TermApplyLpProperties(GpTermEntry * pTerm, const lp_style_type * lp)
{
	// This function passes all the line and point properties to the
	// terminal driver and issues the corresponding commands.
	// 
	// Alas, sometimes it might be necessary to give some help to
	// this function by explicitly issuing additional '(*pTerm)(...)' commands.
	// 
	int lt = lp->l_type;
	int dt = lp->d_type;
	t_dashtype custom_dash_pattern = lp->CustomDashPattern;
	t_colorspec colorspec = lp->pm3d_color;
	if((lp->flags & LP_SHOW_POINTS)) {
		// change points, too
		// Currently, there is no 'pointtype' function.  For points
		// there is a special function also dealing with (x,y) co-ordinates.
		(pTerm->pointsize)(pTerm, (lp->PtSize >= 0.0) ? lp->PtSize : Gg.PointSize);
	}
	//  _first_ set the line width, _then_ set the line type !
	// The linetype might depend on the linewidth in some terminals.
	pTerm->linewidth(pTerm, lp->l_width);
	// LT_DEFAULT (used only by "set errorbars"?) means don't change it 
	if(lt == LT_DEFAULT)
		;
	else
	// The paradigm for handling linetype and dashtype in version 5 is 
	// linetype < 0 (e.g. LT_BACKGROUND, LT_NODRAW) means some special 
	// category that will be handled directly by term->linetype().     
	// linetype > 0 is now redundant. It used to encode both a color   
	// and a dash pattern.  Now we have separate mechanisms for those. 
	if(LT_COLORFROMCOLUMN < lt && lt < 0)
		pTerm->linetype(pTerm, lt);
	else if(pTerm->set_color == GnuPlot::NullSetColor) {
		pTerm->linetype(pTerm, lt-1);
		return;
	}
	else // All normal lines will be solid unless a dashtype is given 
		pTerm->linetype(pTerm, LT_SOLID);
	// Version 5.3
	// If the line is not wanted at all, setting dashtype and color can only hurt
	if(lt == LT_NODRAW)
		return;
	// Apply dashtype or user-specified dash pattern, which may override
	// the terminal-specific dot/dash pattern belonging to this linetype. 
	if(lt == LT_AXIS)
		; // LT_AXIS is a special linetype that may incorporate a dash pattern 
	else if(dt == DASHTYPE_CUSTOM)
		pTerm->dashtype(pTerm, dt, &custom_dash_pattern);
	else if(dt == DASHTYPE_SOLID)
		pTerm->dashtype(pTerm, dt, NULL);
	else if(dt >= 0) {
		// The null_dashtype() routine or a version 5 terminal's private
		// dashtype routine converts this into a call to term->linetype()
		// yielding the same result as in version 4 except possibly for a 
		// different line width.
		pTerm->dashtype(pTerm, dt, NULL);
	}
	ApplyPm3DColor(pTerm, &colorspec); // Finally adjust the color of the line 
}

//void term_start_multiplot()
void GnuPlot::TermStartMultiplot(GpTermEntry * pTerm)
{
	FPRINTF((stderr, "term_start_multiplot()\n"));
	MultiplotStart(pTerm);
#ifdef USE_MOUSE
	UpdateStatusLine();
#endif
}

//void term_end_multiplot()
void GnuPlot::TermEndMultiplot(GpTermEntry * pTerm)
{
	FPRINTF((stderr, "term_end_multiplot()\n"));
	if(multiplot) {
		if(TermSuspended) {
			if(pTerm->resume)
				(pTerm->resume)(pTerm);
			TermSuspended = false;
		}
		MultiplotEnd();
		TermEndPlot(pTerm);
#ifdef USE_MOUSE
		UpdateStatusLine();
#endif
	}
}

//void term_check_multiplot_okay(bool f_interactive)
void GnuPlot::TermCheckMultiplotOkay(bool fInteractive)
{
	FPRINTF((stderr, "term_multiplot_okay(%d)\n", fInteractive));
	if(TermInitialised) { // they've not started yet 
		// make sure that it is safe to issue an interactive prompt
		// it is safe if
		//   it is not an interactive read, or
		//   the terminal supports interactive multiplot, or
		//   we are not writing to stdout and terminal doesn't
		//   refuse multiplot outright
		if(!fInteractive || (term->flags & TERM_CAN_MULTIPLOT) || ((gpoutfile != stdout) && !(term->flags & TERM_CANNOT_MULTIPLOT))) {
			// it's okay to use multiplot here, but suspend first 
			TermSuspend(term);
		}
		else {
			// uh oh: they're not allowed to be in multiplot here 
			TermEndMultiplot(term);
			// at this point we know that it is interactive and that the
			// terminal can either only do multiplot when writing to
			// to a file, or it does not do multiplot at all
			if(term->flags & TERM_CANNOT_MULTIPLOT)
				IntError(NO_CARET, "This terminal does not support multiplot");
			else
				IntError(NO_CARET, "Must set output to a file or put all multiplot commands on one input line");
		}
	}
}

void write_multiline(GpTermEntry * pTerm, int x, int y, char * text, JUSTIFY hor/* horizontal ... */,
    VERT_JUSTIFY vert/* ... and vertical just - text in hor direction despite angle */, int angle/* assume term has already been set for this */,
    const char * pFont/* NULL or "" means use default */)
{
	char * p = text;
	if(p) {
		// EAM 9-Feb-2003 - Set font before calculating sizes 
		if(!isempty(pFont))
			(pTerm->set_font)(pTerm, pFont);
		if(vert != JUST_TOP) {
			// count lines and adjust y 
			int lines = 0; // number of linefeeds - one fewer than lines 
			while(*p) {
				if(*p++ == '\n')
					++lines;
			}
			if(angle)
				x -= (vert * lines * pTerm->ChrV) / 2;
			else
				y += (vert * lines * pTerm->ChrV) / 2;
		}
		for(;;) { // we will explicitly break out 
			if(text && (p = strchr(text, '\n')) != NULL)
				*p = 0; // terminate the string 
			if(pTerm->justify_text(pTerm, hor)) {
				if(on_page(pTerm, x, y))
					pTerm->put_text(pTerm, x, y, text);
			}
			else {
				int len = estimate_strlen(text, NULL);
				int hfix, vfix;
				if(angle == 0) {
					hfix = hor * pTerm->ChrH * len / 2;
					vfix = 0;
				}
				else {
					// Attention: This relies on the numeric values of enum JUSTIFY! 
					hfix = static_cast<int>(hor * pTerm->ChrH * len * cos(angle * SMathConst::PiDiv180) / 2 + 0.5);
					vfix = static_cast<int>(hor * pTerm->ChrV * len * sin(angle * SMathConst::PiDiv180) / 2 + 0.5);
				}
				if(on_page(pTerm, x - hfix, y - vfix))
					pTerm->put_text(pTerm, x - hfix, y - vfix, text);
			}
			if(angle == 90 || angle == TEXT_VERTICAL)
				x += pTerm->ChrV;
			else if(angle == -90 || angle == -TEXT_VERTICAL)
				x -= pTerm->ChrV;
			else
				y -= pTerm->ChrV;
			if(!p)
				break;
			else {
				// put it back 
				*p = '\n';
			}
			text = p + 1;
		} // unconditional branch back to the for(;;) - just a goto ! 
		if(!isempty(pFont))
			(pTerm->set_font)(pTerm, "");
	}
}

//static void do_point(uint x, uint y, int number)
/*static*/void GnuPlot::DoPoint(GpTermEntry * pTerm, uint x, uint y, int number)
{
	//struct GpTermEntry * t = term;
	// use solid lines for point symbols 
	if(pTerm->dashtype != null_dashtype)
		pTerm->dashtype(pTerm, DASHTYPE_SOLID, NULL);
	if(number < 0) {        /* do dot */
		pTerm->move(pTerm, x, y);
		pTerm->vector(pTerm, x, y);
	}
	else {
		number %= POINT_TYPES;
		// should be in term_tbl[] in later version 
		const int htic = static_cast<int>(pTerm->P_Gp->TermPointSize * pTerm->TicH / 2);
		const int vtic = static_cast<int>(pTerm->P_Gp->TermPointSize * pTerm->TicV / 2);
		// point types 1..4 are same as in postscript, png and x11
		// point types 5..6 are "similar"
		// (note that (number) equals (pointtype-1)
		switch(number) {
			case 4:         /* do diamond */
				pTerm->move(pTerm, x - htic, y);
				pTerm->vector(pTerm, x, y - vtic);
				pTerm->vector(pTerm, x + htic, y);
				pTerm->vector(pTerm, x, y + vtic);
				pTerm->vector(pTerm, x - htic, y);
				pTerm->move(pTerm, x, y);
				pTerm->vector(pTerm, x, y);
				break;
			case 0:         /* do plus */
				pTerm->move(pTerm, x - htic, y);
				pTerm->vector(pTerm, x - htic, y);
				pTerm->vector(pTerm, x + htic, y);
				pTerm->move(pTerm, x, y - vtic);
				pTerm->vector(pTerm, x, y - vtic);
				pTerm->vector(pTerm, x, y + vtic);
				break;
			case 3: // do box 
				pTerm->move(pTerm, x - htic, y - vtic);
				pTerm->vector(pTerm, x - htic, y - vtic);
				pTerm->vector(pTerm, x + htic, y - vtic);
				pTerm->vector(pTerm, x + htic, y + vtic);
				pTerm->vector(pTerm, x - htic, y + vtic);
				pTerm->vector(pTerm, x - htic, y - vtic);
				pTerm->move(pTerm, x, y);
				pTerm->vector(pTerm, x, y);
				break;
			case 1: // do X 
				pTerm->move(pTerm, x - htic, y - vtic);
				pTerm->vector(pTerm, x - htic, y - vtic);
				pTerm->vector(pTerm, x + htic, y + vtic);
				pTerm->move(pTerm, x - htic, y + vtic);
				pTerm->vector(pTerm, x - htic, y + vtic);
				pTerm->vector(pTerm, x + htic, y - vtic);
				break;
			case 5: // do triangle 
				pTerm->move(pTerm, x, y + (4 * vtic / 3));
				pTerm->vector(pTerm, x - (4 * htic / 3), y - (2 * vtic / 3));
				pTerm->vector(pTerm, x + (4 * htic / 3), y - (2 * vtic / 3));
				pTerm->vector(pTerm, x, y + (4 * vtic / 3));
				pTerm->move(pTerm, x, y);
				pTerm->vector(pTerm, x, y);
				break;
			case 2:         /* do star */
				pTerm->move(pTerm, x - htic, y);
				pTerm->vector(pTerm, x - htic, y);
				pTerm->vector(pTerm, x + htic, y);
				pTerm->move(pTerm, x, y - vtic);
				pTerm->vector(pTerm, x, y - vtic);
				pTerm->vector(pTerm, x, y + vtic);
				pTerm->move(pTerm, x - htic, y - vtic);
				pTerm->vector(pTerm, x - htic, y - vtic);
				pTerm->vector(pTerm, x + htic, y + vtic);
				pTerm->move(pTerm, x - htic, y + vtic);
				pTerm->vector(pTerm, x - htic, y + vtic);
				pTerm->vector(pTerm, x + htic, y - vtic);
				break;
		}
	}
}

//static void do_pointsize(double size)
/*static*/void GnuPlot::DoPointSize(GpTermEntry * pThis, double size)
{
	pThis->P_Gp->TermPointSize = (size >= 0.0) ? size : 1.0;
}
// 
// general point routine
// 
//static void line_and_point(uint x, uint y, int number)
/*static*/void GnuPlot::LineAndPoint(GpTermEntry * pTerm, uint x, uint y, int number)
{
	// temporary(?) kludge to allow terminals with bad linetypes to make nice marks 
	pTerm->linetype(pTerm, NICE_LINE);
	GnuPlot::DoPoint(pTerm, x, y, number);
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

void GnuPlot::DrawArrow(GpTermEntry * pThis, uint usx, uint usy/* start point */, uint uex, uint uey/* end point (point of arrowhead) */, int headstyle)
{
	// Clipping and angle calculations do not work if coords are unsigned! 
	int sx = (int)usx;
	int sy = (int)usy;
	int ex = (int)uex;
	int ey = (int)uey;
	//struct GpTermEntry * t = term;
	double len_tic = ((double)(pThis->TicH + pThis->TicV)) / 2.0;
	// average of tic sizes 
	// (dx,dy) : vector from end to start 
	double dx = sx - ex;
	double dy = sy - ey;
	double len_arrow = sqrt(dx * dx + dy * dy);
	gpiPoint head_points[5];
	int xm = 0, ym = 0;
	BoundingBox * clip_save;
	// The arrow shaft was clipped already in draw_clip_arrow() but we still 
	// need to clip the head here. 
	clip_save = V.P_ClipArea;
	V.P_ClipArea = (pThis->flags & TERM_CAN_CLIP) ? NULL : &V.BbCanvas;
	// Calculate and draw arrow heads.
	// Draw no head for arrows with length = 0, or, to be more specific,
	// length < DBL_EPSILON, because len_arrow will almost always be != 0.
	if((headstyle & BOTH_HEADS) != NOHEAD && fabs(len_arrow) >= DBL_EPSILON) {
		int x1, y1, x2, y2;
		if(curr_arrow_headlength <= 0) {
			// An arrow head with the default size and angles 
			double coeff_shortest = len_tic * HEAD_SHORT_LIMIT / len_arrow;
			double coeff_longest = len_tic * HEAD_LONG_LIMIT / len_arrow;
			double head_coeff = MAX(coeff_shortest, MIN(HEAD_COEFF, coeff_longest));
			// we put the arrowhead marks at 15 degrees to line 
			x1 = (int)((COS15 * dx - SIN15 * dy) * head_coeff);
			y1 = (int)((SIN15 * dx + COS15 * dy) * head_coeff);
			x2 = (int)((COS15 * dx + SIN15 * dy) * head_coeff);
			y2 = (int)((-SIN15 * dx + COS15 * dy) * head_coeff);
			// backangle defaults to 90 deg 
			xm = (int)((x1 + x2)/2);
			ym = (int)((y1 + y2)/2);
		}
		else {
			// An arrow head with the length + angle specified explicitly.	
			// Assume that if the arrow is shorter than the arrowhead, this is	
			// because of foreshortening in a 3D plot.                      
			double alpha = curr_arrow_headangle * SMathConst::PiDiv180;
			double beta = curr_arrow_headbackangle * SMathConst::PiDiv180;
			double phi = atan2(-dy, -dx); /* azimuthal angle of the vector */
			double backlen;
			double dx2, dy2;
			double effective_length = curr_arrow_headlength;
			if(!curr_arrow_headfixedsize && (curr_arrow_headlength > len_arrow/2.)) {
				effective_length = len_arrow/2.;
				alpha = atan(tan(alpha)*((double)curr_arrow_headlength/effective_length));
				beta = atan(tan(beta)*((double)curr_arrow_headlength/effective_length));
			}
			backlen = sin(alpha) / sin(beta);
			// anticlock-wise head segment 
			x1 = -(int)(effective_length * cos(alpha - phi));
			y1 =  (int)(effective_length * sin(alpha - phi));
			// clock-wise head segment 
			dx2 = -effective_length * cos(phi + alpha);
			dy2 = -effective_length * sin(phi + alpha);
			x2 = (int)(dx2);
			y2 = (int)(dy2);
			// back point 
			xm = (int)(dx2 + backlen*effective_length * cos(phi + beta));
			ym = (int)(dy2 + backlen*effective_length * sin(phi + beta));
		}
		if((headstyle & END_HEAD) && !V.ClipPoint(ex, ey)) {
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
			if(!((headstyle & SHAFT_ONLY))) {
				if(curr_arrow_headfilled >= AS_FILLED) {
					// draw filled forward arrow head 
					head_points->style = FS_OPAQUE;
					if(pThis->filled_polygon)
						(pThis->filled_polygon)(pThis, 5, head_points);
				}
				// draw outline of forward arrow head 
				if(curr_arrow_headfilled == AS_NOFILL)
					DrawClipPolygon(pThis, 3, head_points+1);
				else if(curr_arrow_headfilled != AS_NOBORDER)
					DrawClipPolygon(pThis, 5, head_points);
			}
		}
		// backward arrow head 
		if((headstyle & BACKHEAD) && !V.ClipPoint(sx, sy)) {
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
			if(!((headstyle & SHAFT_ONLY))) {
				if(curr_arrow_headfilled >= AS_FILLED) {
					// draw filled backward arrow head 
					head_points->style = FS_OPAQUE;
					if(pThis->filled_polygon)
						(pThis->filled_polygon)(pThis, 5, head_points);
				}
				// draw outline of backward arrow head 
				if(curr_arrow_headfilled == AS_NOFILL)
					DrawClipPolygon(pThis, 3, head_points+1);
				else if(curr_arrow_headfilled != AS_NOBORDER)
					DrawClipPolygon(pThis, 5, head_points);
			}
		}
	}
	// Adjust the length of the shaft so that it doesn't overlap the head 
	if((headstyle & BACKHEAD) && (fabs(len_arrow) >= DBL_EPSILON) && (curr_arrow_headfilled != AS_NOFILL) ) {
		sx -= xm;
		sy -= ym;
	}
	if((headstyle & END_HEAD) && (fabs(len_arrow) >= DBL_EPSILON) && (curr_arrow_headfilled != AS_NOFILL) ) {
		ex += xm;
		ey += ym;
	}
	// Draw the line for the arrow. 
	if(!((headstyle & HEADS_ONLY)))
		DrawClipLine(pThis, sx, sy, ex, ey);
	V.P_ClipArea = clip_save; // Restore previous clipping box 
}

/*static*/void GnuPlot::DoArrow(GpTermEntry * pThis, uint usx, uint usy/* start point */, uint uex, uint uey/* end point (point of arrowhead) */, int headstyle)
{
	pThis->P_Gp->DrawArrow(pThis, usx, usy, uex, uey, headstyle);
}
//
// Generic routine for drawing circles or circular arcs.          
// If this feature proves useful, we can add a new terminal entry 
// point term->arc() to the API and let terminals either provide  
// a private implementation or use this generic one.               
//
//void do_arc(GpTermEntry * pTerm, int cx, int cy/* Center */, double radius, double arc_start, double arc_end/* Limits of arc in degrees */, int style, bool wedge)
void GnuPlot::DoArc(GpTermEntry * pTerm, int cx, int cy/* Center */, double radius, double arc_start, double arc_end/* Limits of arc in degrees */, int style, bool wedge)
{
	gpiPoint vertex[250];
	int i, segments;
	double aspect;
	bool complete_circle;
	// Protect against out-of-range values 
	while(arc_start < 0)
		arc_start += 360.0;
	while(arc_end > 360.0)
		arc_end -= 360.0;
	// Always draw counterclockwise 
	while(arc_end < arc_start)
		arc_end += 360.0;
	// Choose how finely to divide this arc into segments 
	// Note: INC=2 caused problems for gnuplot_x11 
#define INC 3.0
	segments = static_cast<int>((arc_end - arc_start) / INC);
	SETMAX(segments, 1);
	// Calculate the vertices 
	aspect = (double)pTerm->TicV / (double)pTerm->TicH;
	for(i = 0; i < segments; i++) {
		vertex[i].x = static_cast<int>(cx + cos(SMathConst::PiDiv180 * (arc_start + i*INC)) * radius);
		vertex[i].y = static_cast<int>(cy + sin(SMathConst::PiDiv180 * (arc_start + i*INC)) * radius * aspect);
	}
#undef INC
	vertex[segments].x = static_cast<int>(cx + cos(SMathConst::PiDiv180 * arc_end) * radius);
	vertex[segments].y = static_cast<int>(cy + sin(SMathConst::PiDiv180 * arc_end) * radius * aspect);
	if(fabs(arc_end - arc_start) > 0.1 && fabs(arc_end - arc_start) < 359.9) {
		vertex[++segments].x = cx;
		vertex[segments].y = cy;
		vertex[++segments].x = vertex[0].x;
		vertex[segments].y = vertex[0].y;
		complete_circle = FALSE;
	}
	else
		complete_circle = TRUE;
	if(style) { // Fill in the center 
		gpiPoint fillarea[250];
		int in;
		V.ClipPolygon(vertex, fillarea, segments, &in);
		fillarea[0].style = style;
		if(pTerm->filled_polygon)
			pTerm->filled_polygon(pTerm, in, fillarea);
	}
	else { // Draw the arc 
		if(!wedge && !complete_circle)
			segments -= 2;
		DrawClipPolygon(pTerm, segments+1, vertex);
	}
}

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
//
// change angle of text.  0 is horizontal left to right.
// 1 is vertical bottom to top (90 deg rotate)
//
//static int null_text_angle(int ang)
/*static*/int GnuPlot::NullTextAngle(GpTermEntry * pThis, int ang)
{
	return (ang == 0);
}
//
// change justification of text.
// modes are LEFT (flush left), CENTRE (centred), RIGHT (flush right)
//
//static int null_justify_text(enum JUSTIFY just)
/*static*/int GnuPlot::NullJustifyText(GpTermEntry * pThis, enum JUSTIFY just)
{
	return (just == LEFT);
}
//
// Deprecated terminal function (pre-version 3)
//
/*static*/int GnuPlot::NullScale(GpTermEntry * pThis, double x, double y)
{
	GPO.IntError(NO_CARET, "Attempt to call deprecated terminal function");
	return FALSE; // can't be done 
}

static void null_layer(GpTermEntry * pThis, t_termlayer layer)
{
}

//static void options_null()
/*static*/void GnuPlot::OptionsNull(GpTermEntry * pThis, GnuPlot * pGp)
{
	term_options[0] = '\0'; /* we have no options */
}

static void Func_Init_Null(GpTermEntry * pThis)
{
}

static void graphics_null(GpTermEntry * pThis)
{
	fprintf(stderr, "WARNING: Plotting with an 'unknown' terminal.\nNo output will be generated. Please select a terminal with 'set terminal'.\n");
}

static void UNKNOWN_null(GpTermEntry * pThis)
{
}

static void MOVE_null(GpTermEntry * pThis, uint x, uint y)
{
}

static void LINETYPE_null(GpTermEntry * pThis, int t)
{
}

static void PUTTEXT_null(GpTermEntry * pThis, uint x, uint y, const char * s)
{
}

static void null_linewidth(GpTermEntry * pTerm, double s)
{
}

static int null_set_font(GpTermEntry * pThis, const char * font)
{
	return FALSE; // Never used!! 
}

//static void null_set_color(t_colorspec * pColorSpec)
/*static*/void GnuPlot::NullSetColor(GpTermEntry * pTerm, const t_colorspec * pColorSpec)
{
	if(pColorSpec->type == TC_LT)
		term->linetype(pTerm, pColorSpec->lt);
}

static void null_dashtype(GpTermEntry * pTerm, int type, t_dashtype * custom_dash_pattern)
{
	/*
	 * If the terminal does not support user-defined dashtypes all we can do
	 * do is fall through to the old (pre-v5) assumption that the dashtype,
	 * if any, is part of the linetype.  We also assume that the color will
	 * be adjusted after this.
	 */
	if(type <= 0)
		type = LT_SOLID;
	term->linetype(pTerm, type);
}
// 
// setup the magic macros to compile in the right parts of the
// terminal drivers included by term.h
// 
#define TERM_TABLE
#define TERM_TABLE_START(x) ,{
#define TERM_TABLE_END(x)   }
// 
// term_tbl[] contains an entry for each terminal.  "unknown" must be the
//   first, since term is initialized to 0.
// 
extern GpTermEntry win_driver;
extern GpTermEntry canvas_driver;
extern GpTermEntry cgm_driver;
extern GpTermEntry svg_driver;
extern GpTermEntry domterm_driver;
extern GpTermEntry emf_driver;
extern GpTermEntry dumb_driver;
extern GpTermEntry block_driver;
extern GpTermEntry post_driver;
extern GpTermEntry epslatex_driver;
extern GpTermEntry pslatex_driver;
extern GpTermEntry pstex_driver;
extern GpTermEntry dxf_driver;
extern GpTermEntry fig_driver;
extern GpTermEntry hpgl_driver;
extern GpTermEntry pcl5_driver;
extern GpTermEntry debug_driver;
extern GpTermEntry pbm_driver;
extern GpTermEntry epson180_driver;
extern GpTermEntry epson60_driver;
extern GpTermEntry epsonlx_driver;
extern GpTermEntry nec_driver;
extern GpTermEntry okidata_driver;
extern GpTermEntry starc_driver;
extern GpTermEntry tandy60_driver;
extern GpTermEntry dpu414_driver;
extern GpTermEntry pict2e_driver;
extern GpTermEntry hp500c_driver;
extern GpTermEntry hpljii_driver;
extern GpTermEntry hpdj_driver;
extern GpTermEntry hppj_driver;
extern GpTermEntry pstricks_driver;
extern GpTermEntry texdraw_driver;
extern GpTermEntry mf_driver;
extern GpTermEntry context_driver;
extern GpTermEntry mp_driver;
extern GpTermEntry lua_driver;
extern GpTermEntry tikz_driver;
extern GpTermEntry corel_driver;
extern GpTermEntry cairolatex_driver;
extern GpTermEntry pdfcairo_driver;
extern GpTermEntry pngcairo_driver;
extern GpTermEntry tkcanvas;
extern GpTermEntry ENHest;
extern char * ENHest_plaintext; // terminal-estimate.c

static struct GpTermEntry term_tbl[] = {
	{
		"unknown", 
		"Unknown terminal type - not a plotting device",
		100, 
		100, 
		1, 
		1,
		1, 
		1, 
		GnuPlot::OptionsNull, 
		/*UNKNOWN_null*/Func_Init_Null, 
		UNKNOWN_null,
		UNKNOWN_null, 
		GnuPlot::NullScale, 
		graphics_null, 
		MOVE_null, 
		MOVE_null,
		LINETYPE_null, 
		PUTTEXT_null
	},
	win_driver,     // @experimental
	canvas_driver,  // @experimental
	cgm_driver,     // @experimental
	svg_driver,     // @experimental
	domterm_driver, // @experimental
	emf_driver,     // @experimental
	dumb_driver,    // @experimental
	post_driver,    // @experimental
	epslatex_driver, // @experimental
	pslatex_driver,  // @experimental
	pstex_driver,    // @experimental
	dxf_driver,      // @experimental 
	fig_driver,      // @experimental 
	hpgl_driver,     // @experimental 
	pcl5_driver,     // @experimental 
	debug_driver,    // @experimental 
	pbm_driver,      // @experimental 
	epson180_driver, // @experimental 
	epson60_driver,  // @experimental 
	epsonlx_driver,  // @experimental 
	nec_driver,      // @experimental 
	okidata_driver,  // @experimental 
	starc_driver,    // @experimental 
	tandy60_driver,  // @experimental 
	dpu414_driver,   // @experimental 
	pict2e_driver,   // @experimental 
	hp500c_driver,   // @experimental 
	hpljii_driver,   // @experimental 
	hpdj_driver,     // @experimental  
	hppj_driver,     // @experimental  
	pstricks_driver, // @experimental  
	texdraw_driver,  // @experimental  
	mf_driver,       // @experimental  
	context_driver,  // @experimental  
	mp_driver,       // @experimental  
	lua_driver,      // @experimental  
	tikz_driver,     // @experimental  
	corel_driver,    // @experimental  
	tkcanvas,        // @experimental  
#ifdef HAVE_CAIROPDF
	cairolatex_driver, // @experimental  
	pdfcairo_driver,   // @experimental  
	pngcairo_driver,   // @experimental 
#endif HAVE_CAIROPDF
	block_driver     // @experimental 
#include "term.h"
};

#define TERMCOUNT (sizeof(term_tbl) / sizeof(term_tbl[0]))

void list_terms()
{
	int i;
	char * line_buffer = (char *)SAlloc::M(BUFSIZ);
	int sort_idxs[TERMCOUNT];
	// sort terminal types alphabetically 
	for(i = 0; i < TERMCOUNT; i++)
		sort_idxs[i] = i;
	qsort(sort_idxs, TERMCOUNT, sizeof(int), termcomp);
	// now sort_idxs[] contains the sorted indices 
	StartOutput();
	strcpy(line_buffer, "\nAvailable terminal types:\n");
	OutLine(line_buffer);
	for(i = 0; i < TERMCOUNT; i++) {
		sprintf(line_buffer, "  %15s  %s\n", term_tbl[sort_idxs[i]].name, term_tbl[sort_idxs[i]].description);
		OutLine(line_buffer);
	}

	EndOutput();
	SAlloc::F(line_buffer);
}
// 
// Return string with all terminal names.
// Note: caller must free the returned names after use.
// 
char* get_terminals_names()
{
	int i;
	char * buf = (char *)SAlloc::M(TERMCOUNT*15); /* max 15 chars per name */
	int sort_idxs[TERMCOUNT];
	// sort terminal types alphabetically 
	for(i = 0; i < TERMCOUNT; i++)
		sort_idxs[i] = i;
	qsort(sort_idxs, TERMCOUNT, sizeof(int), termcomp);
	// now sort_idxs[] contains the sorted indices 
	strcpy(buf, " "); // let the string have leading and trailing " " in order to search via strstrt(GPVAL_TERMINALS, " png "); 
	for(i = 0; i < TERMCOUNT; i++)
		sprintf(buf+strlen(buf), "%s ", term_tbl[sort_idxs[i]].name);
	{
		char * names = (char *)SAlloc::M(strlen(buf)+1);
		strcpy(names, buf);
		SAlloc::F(buf);
		return names;
	}
}

static int termcomp(const generic * arga, const generic * argb)
{
	const int * a = (const int*)arga;
	const int * b = (const int*)argb;
	return strcasecmp(term_tbl[*a].name, term_tbl[*b].name);
}
//
// set_term: get terminal number from name on command line
// will change 'term' variable if successful
// 
//struct GpTermEntry * set_term()
GpTermEntry * GnuPlot::SetTerm()
{
	GpTermEntry * p_term = NULL;
	if(!Pgm.EndOfCommand()) {
		char * input_name = Pgm.P_InputLine + Pgm.GetCurTokenStartIndex();
		p_term = ChangeTerm(input_name, Pgm.GetCurTokenLength());
		if(!p_term && Pgm.IsStringValue(Pgm.GetCurTokenIdx()) && (input_name = TryToGetString())) {
			if(strchr(input_name, ' '))
				*strchr(input_name, ' ') = '\0';
			p_term = ChangeTerm(input_name, strlen(input_name));
			SAlloc::F(input_name);
		}
		else
			Pgm.Shift();
	}
	if(!p_term) {
		ChangeTerm("unknown", 7);
		IntError(Pgm.GetPrevTokenIdx(), "unknown or ambiguous terminal type; type just 'set terminal' for a list");
	}
	return p_term; // otherwise the type was changed 
}
// 
// change_term: get terminal number from name and set terminal type
// 
// returns NULL for unknown or ambiguous, otherwise is terminal driver pointer
// 
//GpTermEntry * change_term(const char * pOrigName, int length)
GpTermEntry * GnuPlot::ChangeTerm(const char * pOrigName, int length)
{
	int i;
	GpTermEntry * p_new_term = NULL;
	bool ambiguous = FALSE;
	// For backwards compatibility only 
	char * name = (char *)pOrigName;
	if(!strncmp(pOrigName, "X11", length)) {
		name = "x11";
		length = 3;
	}
#ifdef HAVE_CAIROPDF
	// To allow "set term eps" as short for "set term epscairo" 
	if(!strncmp(pOrigName, "eps", length)) {
		name = "epscairo";
		length = 8;
	}
#endif
#ifdef HAVE_LIBGD
	// To allow "set term sixel" as short for "set term sixelgd" 
	if(!strncmp(pOrigName, "sixel", length)) {
		name = "sixelgd";
		length = 7;
	}
#endif
	for(i = 0; i < TERMCOUNT; i++) {
		if(!strncmp(name, term_tbl[i].name, length)) {
			if(p_new_term)
				ambiguous = TRUE;
			p_new_term = term_tbl + i;
			// Exact match is always accepted 
			if(length == strlen(term_tbl[i].name)) {
				ambiguous = FALSE;
				break;
			}
		}
	}
	if(!p_new_term || ambiguous)
		return NULL;
	else {
		// Success: set terminal type now 
		TermInitialised = false;
		// check that optional fields are initialised to something 
		SETIFZ(p_new_term->text_angle, GnuPlot::NullTextAngle);
		SETIFZ(p_new_term->justify_text, GnuPlot::NullJustifyText);
		SETIFZ(p_new_term->point, GnuPlot::DoPoint);
		SETIFZ(p_new_term->arrow, GnuPlot::DoArrow);
		SETIFZ(p_new_term->pointsize, GnuPlot::DoPointSize);
		SETIFZ(p_new_term->linewidth, null_linewidth);
		SETIFZ(p_new_term->layer, null_layer);
		if(p_new_term->tscale <= 0)
			p_new_term->tscale = 1.0;
		SETIFZ(p_new_term->set_font, null_set_font);
		if(p_new_term->set_color == 0) {
			p_new_term->set_color = GnuPlot::NullSetColor;
			p_new_term->flags |= TERM_NULL_SET_COLOR;
		}
		SETIFZ(p_new_term->dashtype, null_dashtype);
		if(_Plt.interactive)
			fprintf(stderr, "\nTerminal type is now '%s'\n", p_new_term->name);
		invalidate_palette(); // Invalidate any terminal-specific structures that may be active 
		term = p_new_term;
		return p_new_term;
	}
}
/*
 * Find an appropriate initial terminal type.
 * The environment variable GNUTERM is checked first; if that does
 * not exist, then the terminal hardware is checked, if possible,
 * and finally, we can check $TERM for some kinds of terminals.
 * A default can be set with -DDEFAULTTERM=myterm in the Makefile
 * or #define DEFAULTTERM myterm in term.h
 */
// thanks to osupyr!alden (Dave Alden) for the original GNUTERM code 
//
void GnuPlot::InitTerminal()
{
	char * term_name = DEFAULTTERM;
#if defined(__BEOS__) || defined(X11)
	char * env_term = NULL; /* from TERM environment var */
#endif
#ifdef X11
	char * display = NULL;
#endif
	// GNUTERM environment variable is primary 
	char * gnuterm = getenv("GNUTERM");
	if(gnuterm) {
		// April 2017 - allow GNUTERM to include terminal options 
		char * set_term = "set term ";
		char * set_term_command = (char *)SAlloc::M(strlen(set_term) + strlen(gnuterm) + 4);
		strcpy(set_term_command, set_term);
		strcat(set_term_command, gnuterm);
		DoString(set_term_command);
		SAlloc::F(set_term_command);
		// replicate environmental variable GNUTERM for internal use 
		Gstring(&(Ev.AddUdvByName("GNUTERM")->udv_value), sstrdup(gnuterm));
		return;
	}
	else {
		if(!term_name && getenv("DOMTERM"))
			term_name = "domterm";
#ifdef __BEOS__
		env_term = getenv("TERM");
		if(!term_name && env_term && sstreq(env_term, "beterm"))
			term_name = "be";
#endif
#ifdef QTTERM
		SETIFZ(term_name, "qt");
#endif
#ifdef WXWIDGETS
		SETIFZ(term_name, "wxt");
#endif
#ifdef _WIN32
		SETIFZ(term_name, "win");
#endif
#if defined(__APPLE__) && defined(__MACH__) && defined(HAVE_FRAMEWORK_AQUATERM)
		term_name = "aqua"; // Mac OS X with AquaTerm installed 
#endif
#ifdef X11
		env_term = getenv("TERM"); /* try $TERM */
		if(!term_name && env_term && sstreq(env_term, "xterm"))
			term_name = "x11";
		display = getenv("DISPLAY");
		if(term_name == (char *)NULL && display != (char *)NULL)
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
	}
	// We have a name, try to set term type 
	if(!isempty(term_name)) {
		int namelength = strlen(term_name);
		udvt_entry * name = Ev.AddUdvByName("GNUTERM");
		Gstring(&name->udv_value, sstrdup(term_name));
		if(strchr(term_name, ' '))
			namelength = strchr(term_name, ' ') - term_name;
		// Force the terminal to initialize default fonts, etc.	This prevents 
		// segfaults and other strangeness if you set GNUTERM to "post" or    
		// "png" for example. However, calling X11_options() is expensive due 
		// to the fork+execute of gnuplot_x11 and x11 can tolerate not being  
		// initialized until later.                                           
		// Note that Pgm.P_InputLine[] is blank at this point.	              
		if(ChangeTerm(term_name, namelength)) {
			if(strcmp(term->name, "x11"))
				term->options(term, this);
			return;
		}
		else
			fprintf(stderr, "Unknown or ambiguous terminal name '%s'\n", term_name);
	}
	ChangeTerm("unknown", 7);
}
// 
// test terminal by drawing border and text 
// called from command test 
// 
//void test_term(GpTermEntry * pTerm)
void GnuPlot::TestTerminal(GpTermEntry * pTerm)
{
	static t_colorspec black = BLACK_COLORSPEC;
	//struct GpTermEntry * t = term;
	const char * str;
	int x, y, xl, yl, i;
	int xmax_t, ymax_t, x0, y0;
	char label[MAX_ID_LEN];
	int key_entry_height;
	int p_width;
	bool already_in_enhanced_text_mode = LOGIC(pTerm->flags & TERM_ENHANCED_TEXT);
	if(!already_in_enhanced_text_mode)
		DoString("set termopt enh");
	TermStartPlot(pTerm);
	screen_ok = FALSE;
	xmax_t = static_cast<int>(pTerm->MaxX * V.Size.x);
	ymax_t = static_cast<int>(pTerm->MaxY * V.Size.y);
	x0 = static_cast<int>(V.Offset.X * pTerm->MaxX);
	y0 = static_cast<int>(V.Offset.Y * pTerm->MaxY);
	p_width = static_cast<int>(Gg.PointSize * pTerm->TicH);
	key_entry_height = static_cast<int>(Gg.PointSize * pTerm->TicV * 1.25);
	SETMAX(key_entry_height, static_cast<int>(pTerm->ChrV));
	// Sync point for epslatex text positioning 
	(pTerm->layer)(pTerm, TERM_LAYER_FRONTTEXT);
	// border linetype 
	pTerm->linewidth(pTerm, 1.0);
	pTerm->linetype(pTerm, LT_BLACK);
	newpath(pTerm);
	pTerm->move(pTerm, x0, y0);
	pTerm->vector(pTerm, x0 + xmax_t - 1, y0);
	pTerm->vector(pTerm, x0 + xmax_t - 1, y0 + ymax_t - 1);
	pTerm->vector(pTerm, x0, y0 + ymax_t - 1);
	pTerm->vector(pTerm, x0, y0);
	closepath(pTerm);
	// Echo back the current terminal type 
	if(sstreq(pTerm->name, "unknown"))
		IntError(NO_CARET, "terminal type is unknown");
	else {
		char tbuf[64];
		pTerm->justify_text(pTerm, LEFT);
		sprintf(tbuf, "%s  terminal test", pTerm->name);
		pTerm->put_text(pTerm, x0 + pTerm->ChrH * 2, y0 + ymax_t - pTerm->ChrV, tbuf);
		sprintf(tbuf, "gnuplot version %s.%s  ", gnuplot_version, gnuplot_patchlevel);
		pTerm->put_text(pTerm, x0 + pTerm->ChrH * 2, static_cast<uint>(y0 + ymax_t - pTerm->ChrV * 2.25), tbuf);
	}
	pTerm->linetype(pTerm, LT_AXIS);
	pTerm->move(pTerm, x0 + xmax_t / 2, y0);
	pTerm->vector(pTerm, x0 + xmax_t / 2, y0 + ymax_t - 1);
	pTerm->move(pTerm, x0, y0 + ymax_t / 2);
	pTerm->vector(pTerm, x0 + xmax_t - 1, y0 + ymax_t / 2);
	// How well can we estimate width and height of characters?
	// Textbox fill shows true size, surrounding box shows the generic estimate
	// used to reserve space during plot layout.
	if(TRUE) {
		text_label sample; // = EMPTY_LABELSTRUCT;
		textbox_style save_opts = Gg.textbox_opts[0];
		textbox_style * textbox = &Gg.textbox_opts[0];
		sample.text = "12345678901234567890";
		sample.pos = CENTRE;
		sample.boxed = -1;
		textbox->opaque = TRUE;
		textbox->noborder = TRUE;
		textbox->fillcolor.type = TC_RGB;
		textbox->fillcolor.lt = 0xccccee;
		/* disable extra space around text */
		textbox->xmargin = 0;
		textbox->ymargin = 0;
		pTerm->linetype(pTerm, LT_SOLID);
		WriteLabel(pTerm, xmax_t/2, ymax_t/2, &sample);
		Gg.textbox_opts[0] = save_opts;
		sample.boxed = 0;
		sample.text = "true vs. estimated text dimensions";
		WriteLabel(pTerm, xmax_t/2, static_cast<int>(ymax_t/2 + 1.5 * pTerm->ChrV), &sample);
		newpath(pTerm);
		pTerm->move(pTerm, x0 + xmax_t / 2 - pTerm->ChrH * 10, y0 + ymax_t / 2 + pTerm->ChrV / 2);
		pTerm->vector(pTerm, x0 + xmax_t / 2 + pTerm->ChrH * 10, y0 + ymax_t / 2 + pTerm->ChrV / 2);
		pTerm->vector(pTerm, x0 + xmax_t / 2 + pTerm->ChrH * 10, y0 + ymax_t / 2 - pTerm->ChrV / 2);
		pTerm->vector(pTerm, x0 + xmax_t / 2 - pTerm->ChrH * 10, y0 + ymax_t / 2 - pTerm->ChrV / 2);
		pTerm->vector(pTerm, x0 + xmax_t / 2 - pTerm->ChrH * 10, y0 + ymax_t / 2 + pTerm->ChrV / 2);
		closepath(pTerm);
	}
	// Test for enhanced text 
	pTerm->linetype(pTerm, LT_BLACK);
	if(pTerm->flags & TERM_ENHANCED_TEXT) {
		const char * tmptext1 =   "Enhanced text:   {x@_{0}^{n+1}}";
		const char * tmptext2 = "&{Enhanced text:  }{/:Bold Bold}{/:Italic  Italic}";
		pTerm->put_text(pTerm, static_cast<uint>(x0 + xmax_t * 0.5), static_cast<uint>(y0 + ymax_t * 0.40), tmptext1);
		pTerm->put_text(pTerm, static_cast<uint>(x0 + xmax_t * 0.5), static_cast<uint>(y0 + ymax_t * 0.35), tmptext2);
		(pTerm->set_font)(pTerm, "");
		if(!already_in_enhanced_text_mode)
			DoString("set termopt noenh");
	}
	// test justification 
	pTerm->justify_text(pTerm, LEFT);
	pTerm->put_text(pTerm, x0 + xmax_t / 2, y0 + ymax_t / 2 + pTerm->ChrV * 6, "left justified");
	str = "centre+d text";
	if(pTerm->justify_text(pTerm, CENTRE))
		pTerm->put_text(pTerm, x0 + xmax_t / 2, y0 + ymax_t / 2 + pTerm->ChrV * 5, str);
	else
		pTerm->put_text(pTerm, x0 + xmax_t / 2 - strlen(str) * pTerm->ChrH / 2, y0 + ymax_t / 2 + pTerm->ChrV * 5, str);
	str = "right justified";
	if(pTerm->justify_text(pTerm, RIGHT))
		pTerm->put_text(pTerm, x0 + xmax_t / 2, y0 + ymax_t / 2 + pTerm->ChrV * 4, str);
	else
		pTerm->put_text(pTerm, x0 + xmax_t / 2 - strlen(str) * pTerm->ChrH, y0 + ymax_t / 2 + pTerm->ChrV * 4, str);
	// test tic size 
	pTerm->linetype(pTerm, 2);
	pTerm->move(pTerm, (uint)(x0 + xmax_t / 2 + pTerm->TicH * (1 + AxS[FIRST_X_AXIS].ticscale)), y0 + (uint)ymax_t - 1);
	pTerm->vector(pTerm, (uint)(x0 + xmax_t / 2 + pTerm->TicH * (1 + AxS[FIRST_X_AXIS].ticscale)),
	    (uint)(y0 + ymax_t - AxS[FIRST_X_AXIS].ticscale * pTerm->TicV));
	pTerm->move(pTerm, (uint)(x0 + xmax_t / 2), y0 + (uint)(ymax_t - pTerm->TicV * (1 + AxS[FIRST_X_AXIS].ticscale)));
	pTerm->vector(pTerm, (uint)(x0 + xmax_t / 2 + AxS[FIRST_X_AXIS].ticscale * pTerm->TicH), (uint)(y0 + ymax_t - pTerm->TicV * (1 + AxS[FIRST_X_AXIS].ticscale)));
	pTerm->justify_text(pTerm, RIGHT);
	pTerm->put_text(pTerm, x0 + (uint)(xmax_t / 2 - 1* pTerm->ChrH), y0 + (uint)(ymax_t - pTerm->ChrV), "show ticscale");
	pTerm->justify_text(pTerm, LEFT);
	pTerm->linetype(pTerm, LT_BLACK);
	// test line and point types 
	x = x0 + xmax_t - pTerm->ChrH * 7 - p_width;
	y = y0 + ymax_t - key_entry_height;
	(pTerm->pointsize)(pTerm, Gg.PointSize);
	for(i = -2; y > y0 + key_entry_height; i++) {
		lp_style_type ls; // = DEFAULT_LP_STYLE_TYPE;
		ls.l_width = 1;
		LoadLineType(pTerm, &ls, i+1);
		TermApplyLpProperties(pTerm, &ls);
		sprintf(label, "%d", i + 1);
		if(pTerm->justify_text(pTerm, RIGHT))
			pTerm->put_text(pTerm, x, y, label);
		else
			pTerm->put_text(pTerm, x - strlen(label) * pTerm->ChrH, y, label);
		pTerm->move(pTerm, x + pTerm->ChrH, y);
		pTerm->vector(pTerm, x + pTerm->ChrH * 5, y);
		if(i >= -1)
			pTerm->point(pTerm, x + pTerm->ChrH * 6 + p_width / 2, y, i);
		y -= key_entry_height;
	}
	// test arrows (should line up with rotated text) 
	pTerm->linewidth(pTerm, 1.0);
	pTerm->linetype(pTerm, 0);
	pTerm->dashtype(pTerm, DASHTYPE_SOLID, NULL);
	x = static_cast<int>(x0 + 2.0 * pTerm->ChrV);
	y = y0 + ymax_t/2;
	xl = pTerm->TicH * 7;
	yl = pTerm->TicV * 7;
	i = curr_arrow_headfilled;
	curr_arrow_headfilled = AS_NOBORDER;
	pTerm->arrow(pTerm, x, y-yl, x, y+yl, BOTH_HEADS);
	curr_arrow_headfilled = AS_EMPTY;
	pTerm->arrow(pTerm, x, y, x + xl, y + yl, END_HEAD);
	curr_arrow_headfilled = AS_NOFILL;
	pTerm->arrow(pTerm, x, y, x + xl, y - yl, END_HEAD);
	curr_arrow_headfilled = (arrowheadfill)i;
	// test text angle (should match arrows) 
	pTerm->linetype(pTerm, 0);
	str = "rotated ce+ntred text";
	if(pTerm->text_angle(pTerm, TEXT_VERTICAL)) {
		if(pTerm->justify_text(pTerm, CENTRE))
			pTerm->put_text(pTerm, x0 + pTerm->ChrV, y0 + ymax_t / 2, str);
		else
			pTerm->put_text(pTerm, x0 + pTerm->ChrV, y0 + ymax_t / 2 - strlen(str) * pTerm->ChrH / 2, str);
		pTerm->justify_text(pTerm, LEFT);
		str = "  rotate by +45";
		pTerm->text_angle(pTerm, 45);
		pTerm->put_text(pTerm, x0 + pTerm->ChrV * 3, y0 + ymax_t / 2, str);
		pTerm->justify_text(pTerm, LEFT);
		str = "  rotate by -45";
		pTerm->text_angle(pTerm, -45);
		pTerm->put_text(pTerm, x0 + pTerm->ChrV * 3, y0 + ymax_t / 2, str);
	}
	else {
		pTerm->justify_text(pTerm, LEFT);
		pTerm->put_text(pTerm, x0 + pTerm->ChrH * 2, y0 + ymax_t / 2, "cannot rotate text");
	}
	pTerm->justify_text(pTerm, LEFT);
	pTerm->text_angle(pTerm, 0);
	// test line widths 
	pTerm->justify_text(pTerm, LEFT);
	xl = xmax_t / 10;
	yl = ymax_t / 25;
	x = static_cast<int>(x0 + xmax_t * 0.075);
	y = y0 + yl;
	for(i = 1; i<7; i++) {
		pTerm->linewidth(pTerm, (double)(i)); pTerm->linetype(pTerm, LT_BLACK);
		pTerm->move(pTerm, x, y); 
		pTerm->vector(pTerm, x+xl, y);
		sprintf(label, "  lw %1d", i);
		pTerm->put_text(pTerm, x+xl, y, label);
		y += yl;
	}
	pTerm->put_text(pTerm, x, y, "linewidth");
	// test native dashtypes (_not_ the 'set mono' sequence) 
	pTerm->justify_text(pTerm, LEFT);
	xl = xmax_t / 10;
	yl = ymax_t / 25;
	x = static_cast<int>(x0 + xmax_t * 0.3);
	y = y0 + yl;
	for(i = 0; i<5; i++) {
		pTerm->linewidth(pTerm, 1.0);
		pTerm->linetype(pTerm, LT_SOLID);
		pTerm->dashtype(pTerm, i, NULL);
		pTerm->set_color(pTerm, &black);
		pTerm->move(pTerm, x, y); 
		pTerm->vector(pTerm, x+xl, y);
		sprintf(label, "  dt %1d", i+1);
		pTerm->put_text(pTerm, x+xl, y, label);
		y += yl;
	}
	pTerm->put_text(pTerm, x, y, "dashtype");
	// test fill patterns 
	x = static_cast<int>(x0 + xmax_t * 0.5);
	y = y0;
	xl = xmax_t / 40;
	yl = ymax_t / 8;
	pTerm->linewidth(pTerm, 1.0);
	pTerm->linetype(pTerm, LT_BLACK);
	pTerm->justify_text(pTerm, CENTRE);
	pTerm->put_text(pTerm, x+xl*7, static_cast<uint>(y + yl+pTerm->ChrV*1.5), "pattern fill");
	for(i = 0; i < 9; i++) {
		const int style = ((i<<4) + FS_PATTERN);
		if(pTerm->fillbox)
			(pTerm->fillbox)(pTerm, style, x, y, xl, yl);
		newpath(pTerm);
		pTerm->move(pTerm, x, y);
		pTerm->vector(pTerm, x, y+yl);
		pTerm->vector(pTerm, x+xl, y+yl);
		pTerm->vector(pTerm, x+xl, y);
		pTerm->vector(pTerm, x, y);
		closepath(pTerm);
		sprintf(label, "%2d", i);
		pTerm->put_text(pTerm, x+xl/2, static_cast<uint>(y+yl+pTerm->ChrV*0.5), label);
		x = static_cast<int>(x + xl * 1.5);
	}
	{
		int cen_x = x0 + (int)(0.70 * xmax_t);
		int cen_y = y0 + (int)(0.83 * ymax_t);
		int radius = xmax_t / 20;
		// test pm3d -- filled_polygon(), but not set_color() 
		if(pTerm->filled_polygon) {
			int i, j;
#define NUMBER_OF_VERTICES 6
			int n = NUMBER_OF_VERTICES;
			gpiPoint corners[NUMBER_OF_VERTICES+1];
#undef  NUMBER_OF_VERTICES
			for(j = 0; j<=1; j++) {
				int ix = cen_x + j*radius;
				int iy = cen_y - j*radius/2;
				for(i = 0; i < n; i++) {
					corners[i].x = static_cast<int>(ix + radius * cos(SMathConst::Pi2 * i/n));
					corners[i].y = static_cast<int>(iy + radius * sin(SMathConst::Pi2 * i/n));
				}
				corners[n].x = corners[0].x;
				corners[n].y = corners[0].y;
				if(j == 0) {
					pTerm->linetype(pTerm, 2);
					corners->style = FS_OPAQUE;
				}
				else {
					pTerm->linetype(pTerm, 1);
					corners->style = FS_TRANSPARENT_SOLID + (50<<4);
				}
				term->filled_polygon(term, n+1, corners);
			}
			str = "filled polygons:";
		}
		else
			str = "No filled polygons";
		pTerm->linetype(pTerm, LT_BLACK);
		i = (pTerm->justify_text(pTerm, CENTRE)) ? 0 : pTerm->ChrH * strlen(str) / 2;
		pTerm->put_text(pTerm, cen_x - i, static_cast<uint>(cen_y + radius + pTerm->ChrV * 0.5), str);
	}
	TermEndPlot(pTerm);
}
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

void do_enh_writec(GpTermEntry * pThis, int c)
{
	// Guard against buffer overflow 
	if(GPO.Enht.P_CurText >= /*ENHANCED_TEXT_MAX*/&(GPO.Enht.Text[sizeof(GPO.Enht.Text)]))
		return;
	// note: c is meant to hold a char, but is actually an int, for
	// the same reasons applying to putc() and friends 
	*GPO.Enht.P_CurText++ = c;
}
// 
// Process a bit of string, and return the last character used.
// p is start of string
// brace is TRUE to keep processing to }, FALSE to do one character only
// fontname & fontsize are obvious
// base is the current baseline
// widthflag is TRUE if the width of this should count, FALSE for zero width boxes
// showflag is TRUE if this should be shown, FALSE if it should not be shown (like TeX \phantom)
// overprint is 0 for normal operation,
//   1 for the underprinted text (included in width calculation),
//   2 for the overprinted text (not included in width calc)
//   (overprinted text is centered horizontally on underprinted text
// 
const char * enhanced_recursion(GpTermEntry * pTerm, const char * p, bool brace, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
{
	GnuPlot * p_gp = pTerm->P_Gp;
	// Keep track of the style of the font passed in at this recursion level 
	bool wasitalic = (strstr(fontname, ":Italic") != NULL);
	bool wasbold = (strstr(fontname, ":Bold") != NULL);
	FPRINTF((stderr, "RECURSE WITH \"%s\", %d %s %.1f %.1f %d %d %d", p, brace, fontname, fontsize, base, widthflag, showflag, overprint));
	// Start each recursion with a clean string 
	(pTerm->enhanced_flush)(pTerm);
	if(base + fontsize > p_gp->Enht.MaxHeight) {
		p_gp->Enht.MaxHeight = base + fontsize;
		ENH_DEBUG(("Setting max height to %.1f\n", p_gp->Enht.MaxHeight));
	}
	if(base < p_gp->Enht.MinHeight) {
		p_gp->Enht.MinHeight = base;
		ENH_DEBUG(("Setting min height to %.1f\n", p_gp->Enht.MinHeight));
	}
	while(*p) {
		double shift;
		// 
		// EAM Jun 2009 - treating bytes one at a time does not work for multibyte
		// encodings, including utf-8. If we hit a byte with the high bit set, test
		// whether it starts a legal UTF-8 sequence and if so copy the whole thing.
		// Other multibyte encodings are still a problem.
		// Gnuplot's other defined encodings are all single-byte; for those we
		// really do want to treat one byte at a time.
		// 
		if((*p & 0x80) && (encoding == S_ENC_DEFAULT || encoding == S_ENC_UTF8)) {
			ulong utf8char;
			const char * nextchar = p;
			(pTerm->enhanced_open)(pTerm, fontname, fontsize, base, widthflag, showflag, overprint);
			if(utf8toulong(&utf8char, &nextchar)) { /* Legal UTF8 sequence */
				while(p < nextchar)
					(pTerm->enhanced_writec)(pTerm, *p++);
				p--;
			}
			else {                          /* Some other multibyte encoding? */
				(pTerm->enhanced_writec)(pTerm, *p);
			}
/* shige : for Shift_JIS */
		}
		else if((*p & 0x80) && (encoding == S_ENC_SJIS)) {
			(pTerm->enhanced_open)(pTerm, fontname, fontsize, base, widthflag, showflag, overprint);
			(pTerm->enhanced_writec)(pTerm, *(p++));
			(pTerm->enhanced_writec)(pTerm, *p);
		}
		else
			switch(*p) {
				case '}':
				    /*{{{  deal with it*/
				    if(brace)
					    return (p);
				    p_gp->IntWarn(NO_CARET, "enhanced text parser - spurious }");
				    break;
				/*}}}*/
				case '_':
				case '^':
				    /*{{{  deal with super/sub script*/
				    shift = (*p == '^') ? 0.5 : -0.3;
				    (pTerm->enhanced_flush)(pTerm);
				    p = enhanced_recursion(pTerm, p + 1, FALSE, fontname, fontsize * 0.8, base + shift * fontsize, widthflag, showflag, overprint);
				    break;
				/*}}}*/
				case '{':
			    {
				    bool isitalic = FALSE, isbold = FALSE, isnormal = FALSE;
				    const char * start_of_fontname = NULL;
				    const char * end_of_fontname = NULL;
				    char * localfontname = NULL;
				    char ch;
				    double f = fontsize, ovp;
				    // Mar 2014 - this will hold "fontfamily{:Italic}{:Bold}" 
				    char * styledfontname = NULL;
				    /*{{{  recurse (possibly with a new font) */
				    ENH_DEBUG(("Dealing with {\n"));
				    /* 30 Sep 2016:  Remove incorrect whitespace-eating loop going */
				    /* waaay back to 31-May-2000 */        /* while (*++p == ' '); */
				    ++p;
				    /* get vertical offset (if present) for overprinted text */
				    if(overprint == 2) {
					    char * end;
					    ovp = strtod(p, &end);
					    p = end;
					    if(pTerm->flags & TERM_IS_POSTSCRIPT)
						    base = ovp*f;
					    else
						    base += ovp*f;
				    }
				    --p;
				    if(*++p == '/') {
					    /* then parse a fontname, optional fontsize */
					    while(*++p == ' ')
						    ; /* do nothing */
					    if(*p=='-') {
						    while(*++p == ' ')
							    ; /* do nothing */
					    }
					    start_of_fontname = p;
					    /* Allow font name to be in quotes.
					     * This makes it possible to handle font names containing spaces.
					     */
					    if(*p == '\'' || *p == '"') {
						    ++p;
						    while(*p != '\0' && *p != '}' && *p != *start_of_fontname)
							    ++p;
						    if(*p != *start_of_fontname) {
							    p_gp->IntWarn(NO_CARET, "cannot interpret font name %s", start_of_fontname);
							    break;
						    }
						    start_of_fontname++;
						    end_of_fontname = p++;
						    ch = *p;
					    }
					    else {
						    /* Normal unquoted font name */
						    while((ch = *p) > ' ' && ch != '=' && ch != '*' && ch != '}' && ch != ':')
							    ++p;
						    end_of_fontname = p;
					    }
					    do {
						    if(ch == '=') {
							    /* get optional font size */
							    char * end;
							    p++;
							    ENH_DEBUG(("Calling strtod(\"%s\") ...", p));
							    f = strtod(p, &end);
							    p = end;
							    ENH_DEBUG(("Returned %.1f and \"%s\"\n", f, p));
							    if(f == 0)
								    f = fontsize;
							    else
								    f *= p_gp->Enht.FontScale; /* remember the scaling */
							    ENH_DEBUG(("Font size %.1f\n", f));
						    }
						    else if(ch == '*') {
							    /* get optional font size scale factor */
							    char * end;
							    p++;
							    ENH_DEBUG(("Calling strtod(\"%s\") ...", p));
							    f = strtod(p, &end);
							    p = end;
							    ENH_DEBUG(("Returned %.1f and \"%s\"\n", f, p));
							    if(f != 0.0)
								    f *= fontsize; /* apply the scale factor */
							    else
								    f = fontsize;
							    ENH_DEBUG(("Font size %.1f\n", f));
						    }
						    else if(ch == ':') {
							    /* get optional style markup attributes */
							    p++;
							    if(!strncmp(p, "Bold", 4))
								    isbold = TRUE;
							    if(!strncmp(p, "Italic", 6))
								    isitalic = TRUE;
							    if(!strncmp(p, "Normal", 6))
								    isnormal = TRUE;
							    while(isalpha((uchar)*p)) {
								    p++;
							    }
						    }
					    } while(((ch = *p) == '=') || (ch == ':') || (ch == '*'));
					    if(ch == '}')
						    p_gp->IntWarn(NO_CARET, "bad syntax in enhanced text string");
					    if(*p == ' ') /* Eat up a single space following a font spec */
						    ++p;
					    if(!start_of_fontname || (start_of_fontname == end_of_fontname)) {
						    /* Use the font name passed in to us */
						    localfontname = sstrdup(fontname);
					    }
					    else {
						    /* We found a new font name {/Font ...} */
						    int len = end_of_fontname - start_of_fontname;
						    localfontname = (char *)SAlloc::M(len+1);
						    strncpy(localfontname, start_of_fontname, len);
						    localfontname[len] = '\0';
					    }
				    }
				    /*}}}*/
				    /* Collect cumulative style markup before passing it in the font name */
				    isitalic = (wasitalic || isitalic) && !isnormal;
				    isbold = (wasbold || isbold) && !isnormal;
				    styledfontname = GnuPlot::_StyleFont(localfontname ? localfontname : fontname, isbold, isitalic);
				    p = enhanced_recursion(pTerm, p, TRUE, styledfontname, f, base, widthflag, showflag, overprint);
				    (pTerm->enhanced_flush)(pTerm);
				    SAlloc::F(styledfontname);
				    SAlloc::F(localfontname);
				    break;
			    } /* case '{' */
				case '@':
				    /*{{{  phantom box - prints next 'char', then restores currentpoint */
				    (pTerm->enhanced_flush)(pTerm);
				    (pTerm->enhanced_open)(pTerm, fontname, fontsize, base, widthflag, showflag, 3);
				    p = enhanced_recursion(pTerm, ++p, FALSE, fontname, fontsize, base, widthflag, showflag, overprint);
				    (pTerm->enhanced_open)(pTerm, fontname, fontsize, base, widthflag, showflag, 4);
				    break;
				/*}}}*/

				case '&':
				    /*{{{  character skip - skips space equal to length of character(s) */
				    (pTerm->enhanced_flush)(pTerm);
				    p = enhanced_recursion(pTerm, ++p, FALSE, fontname, fontsize, base, widthflag, FALSE, overprint);
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
				    (pTerm->enhanced_flush)(pTerm);
				    p = enhanced_recursion(pTerm, ++p, FALSE, fontname, fontsize, base, widthflag, showflag, 1);
				    (pTerm->enhanced_flush)(pTerm);
				    if(!*p)
					    break;
				    p = enhanced_recursion(pTerm, ++p, FALSE, fontname, fontsize, base, FALSE, showflag, 2);
				    overprint = 0; /* may not be necessary, but just in case . . . */
				    break;
				/*}}}*/

				case '(':
				case ')':
				    //{{{  an escape and print it 
				    // special cases 
				    (pTerm->enhanced_open)(pTerm, fontname, fontsize, base, widthflag, showflag, overprint);
				    if(pTerm->flags & TERM_IS_POSTSCRIPT)
					    (pTerm->enhanced_writec)(pTerm, '\\');
				    (pTerm->enhanced_writec)(pTerm, *p);
				    break;
				/*}}}*/

				case '\\':
				    /*{{{  various types of escape sequences, some context-dependent */
				    (pTerm->enhanced_open)(pTerm, fontname, fontsize, base, widthflag, showflag, overprint);
				    /*     Unicode represented as \U+hhhhh where hhhhh is hexadecimal code point.
				     *     For UTF-8 encoding we translate hhhhh to a UTF-8 byte sequence and
				     *     output the bytes one by one.
				     */
				    if(p[1] == 'U' && p[2] == '+') {
					    if(encoding == S_ENC_UTF8) {
						    uint32_t codepoint;
						    uchar utf8char[8];
						    int i, length;
						    if(strlen(&(p[3])) < 4)
							    break;
						    if(sscanf(&(p[3]), "%5x", &codepoint) != 1)
							    break;
						    length = ucs4toutf8(codepoint, utf8char);
						    p += (codepoint > 0xFFFF) ? 7 : 6;
						    for(i = 0; i<length; i++)
							    (pTerm->enhanced_writec)(pTerm, utf8char[i]);
						    break;
					    }

					    /*     FIXME: non-utf8 environments not yet supported.
					     *     Note that some terminals may have an alternative way to handle
					     *unicode
					     *     escape sequences that is not dependent on encoding.
					     *     E.g. svg and html output could convert to xml sequences &#xhhhh;
					     *     For these cases we must retain the leading backslash so that the
					     *     unicode escape sequence can be recognized by the terminal driver.
					     */
					    (pTerm->enhanced_writec)(pTerm, p[0]);
					    break;
				    }

				    /* Enhanced mode always uses \xyz as an octal character representation
				     * but each terminal type must give us the actual output format wanted.
				     * pdf.trm wants the raw character code, which is why we use strtol();
				     * most other terminal types want some variant of "\\%o".
				     */
				    if(p[1] >= '0' && p[1] <= '7') {
					    char * e, escape[16], octal[4] = {'\0', '\0', '\0', '\0'};
					    octal[0] = *(++p);
					    if(p[1] >= '0' && p[1] <= '7') {
						    octal[1] = *(++p);
						    if(p[1] >= '0' && p[1] <= '7')
							    octal[2] = *(++p);
					    }
					    sprintf(escape, p_gp->Enht.EscapeFormat, strtol(octal, NULL, 8));
					    for(e = escape; *e; e++) {
						    (pTerm->enhanced_writec)(pTerm, *e);
					    }
					    break;
				    }

				    /* This was the original (prior to version 4) enhanced text code specific
				     * to the reserved characters of PostScript.
				     */
				    if(pTerm->flags & TERM_IS_POSTSCRIPT) {
					    if(p[1]=='\\' || p[1]=='(' || p[1]==')') {
						    (pTerm->enhanced_writec)(pTerm, '\\');
					    }
					    else if(strchr("^_@&~{}", p[1]) == NULL) {
						    (pTerm->enhanced_writec)(pTerm, '\\');
						    (pTerm->enhanced_writec)(pTerm, '\\');
						    break;
					    }
				    }
				    // Step past the backslash character in the input stream 
				    ++p;
				    // HBB: Avoid broken output if there's a \ exactly at the end of the line 
				    if(*p == '\0') {
					    p_gp->IntWarn(NO_CARET, "enhanced text parser -- spurious backslash");
					    break;
				    }
				    // SVG requires an escaped '&' to be passed as something else 
				    // FIXME: terminal-dependent code does not belong here 
				    if(*p == '&' && encoding == S_ENC_DEFAULT && sstreq(pTerm->name, "svg")) {
					    (pTerm->enhanced_writec)(pTerm, '\376');
					    break;
				    }
				    // print the character following the backslash 
				    (pTerm->enhanced_writec)(pTerm, *p);
				    break;
				/*}}}*/

				default:
				    /*{{{  print it */
				    (pTerm->enhanced_open)(pTerm, fontname, fontsize, base, widthflag, showflag, overprint);
				    (pTerm->enhanced_writec)(pTerm, *p);
				    /*}}}*/
			}/* switch (*p) */
		// like TeX, we only do one character in a recursion, unless it's in braces
		if(!brace) {
			(pTerm->enhanced_flush)(pTerm);
			return(p); /* the ++p in the outer copy will increment us */
		}
		if(*p) /* only not true if { not terminated, I think */
			++p;
	} /* while (*p) */
	(pTerm->enhanced_flush)(pTerm);
	return p;
}
//
// Strip off anything trailing the requested font name,
// then add back markup requests.
//
//char * stylefont(const char * pFontName, bool isBold, bool isItalic)
/*static*/char * GnuPlot::_StyleFont(const char * pFontName, bool isBold, bool isItalic)
{
	int    div;
	char * markup = (char *)SAlloc::M(strlen(pFontName) + 16);
	strcpy(markup, pFontName);
	// base font name can be followed by ,<size> or :Variant 
	if((div = strcspn(markup, ",:")))
		markup[div] = '\0';
	if(isBold)
		strcat(markup, ":Bold");
	if(isItalic)
		strcat(markup, ":Italic");
	FPRINTF((stderr, "MARKUP FONT: %s -> %s\n", pFontName, markup));
	return markup;
}
//
// Called after the end of recursion to check for errors 
//
void enh_err_check(const char * str)
{
	if(*str == '}')
		GPO.IntWarn(NO_CARET, "enhanced text mode parser - ignoring spurious }");
	else
		GPO.IntWarn(NO_CARET, "enhanced text mode parsing error");
}
// 
// Text strings containing control information for enhanced text mode
// contain more characters than will actually appear in the output.
// This makes it hard to estimate how much horizontal space on the plot
// (e.g. in the key box) must be reserved to hold them.  To approximate
// the eventual length we switch briefly to the dummy terminal driver
// "estimate.trm" and then switch back to the current terminal.
// If better, perhaps terminal-specific methods of estimation are
// developed later they can be slotted into this one call site.
// 
// Dec 2019: height is relative to original font size
//   DEBUG: currently pegged at 10pt - we should do better!
// 
int estimate_strlen(const char * text, double * height)
{
	int len;
	char * s;
	double estimated_fontheight = 1.0;
	if(term->flags & TERM_IS_LATEX)
		return strlen_tex(text);
#ifdef GP_ENH_EST
	if(strchr(text, '\n') || (term->flags & TERM_ENHANCED_TEXT)) {
		struct GpTermEntry * tsave = term;
		term = &ENHest;
		term->put_text(term, 0, 0, text);
		len = term->MaxX;
		estimated_fontheight = term->MaxY / 10.;
		term = tsave;
		// Assume that unicode escape sequences  \U+xxxx will generate a single character 
		// ENHest_plaintext is filled in by the put_text() call to estimate.trm           
		s = ENHest_plaintext;
		while((s = contains_unicode(s)) != NULL) {
			len -= 6;
			s += 6;
		}
		FPRINTF((stderr, "Estimating length %d height %g for enhanced text \"%s\"", len, estimated_fontheight, text));
		FPRINTF((stderr, "  plain text \"%s\"\n", ENHest_plaintext));
	}
	else if(encoding == S_ENC_UTF8)
		len = strwidth_utf8(text);
	else
#endif
	len = strlen(text);
	ASSIGN_PTR(height, estimated_fontheight);
	return len;
}
// 
// Use estimate.trm to mock up a non-enhanced approximation of the
// original string.
// 
char * estimate_plaintext(char * enhancedtext)
{
	if(enhancedtext == NULL)
		return NULL;
	estimate_strlen(enhancedtext, NULL);
	return ENHest_plaintext;
}

void ignore_enhanced(bool flag)
{
	GPO.Enht.Ignore = flag;
}
// 
// Simple-minded test for whether the point (x,y) is in bounds for the current terminal.
// Some terminals can do their own clipping, and can clip partial objects.
// If the flag TERM_CAN_CLIP is set, we skip this relative crude test and let the
// driver or the hardware handle clipping.
// 
bool on_page(GpTermEntry * pTerm, int x, int y)
{
	if(pTerm->flags & TERM_CAN_CLIP)
		return TRUE;
	if((0 < x && x < static_cast<int>(pTerm->MaxX)) && (0 < y && y < static_cast<int>(pTerm->MaxY)))
		return TRUE;
	return FALSE;
}
// 
// Utility routine for drivers to accept an explicit size for the output image.
// 
//GpSizeUnits parse_term_size(float * pXSize, float * pYSize, GpSizeUnits default_units)
GpSizeUnits GnuPlot::ParseTermSize(float * pXSize, float * pYSize, GpSizeUnits default_units)
{
	GpSizeUnits units = default_units;
	if(Pgm.EndOfCommand())
		IntErrorCurToken("size requires two numbers:  xsize, ysize");
	*pXSize = FloatExpression();
	if(Pgm.AlmostEqualsCur("in$ches")) {
		Pgm.Shift();
		units = INCHES;
	}
	else if(Pgm.EqualsCur("cm")) {
		Pgm.Shift();
		units = CM;
	}
	switch(units) {
		case INCHES: *pXSize *= GpResolution; break;
		case CM:     *pXSize *= (float)GpResolution / 2.54f; break;
		case PIXELS:
		default:     break;
	}
	if(!Pgm.EqualsCurShift(","))
		IntErrorCurToken("size requires two numbers:  xsize, ysize");
	*pYSize = FloatExpression();
	if(Pgm.AlmostEqualsCur("in$ches")) {
		Pgm.Shift();
		units = INCHES;
	}
	else if(Pgm.EqualsCur("cm")) {
		Pgm.Shift();
		units = CM;
	}
	switch(units) {
		case INCHES: *pYSize *= GpResolution; break;
		case CM:     *pYSize *= (float)GpResolution / 2.54f; break;
		case PIXELS:
		default:     break;
	}
	if(*pXSize < 1.0f || *pYSize < 1.0f)
		IntErrorCurToken("size: out of range");
	return units;
}
// 
// Wrappers for newpath and closepath
// 
void FASTCALL newpath(GpTermEntry * pTerm)
{
	if(pTerm->path)
		pTerm->path(pTerm, 0);
}

void FASTCALL closepath(GpTermEntry * pTerm)
{
	if(pTerm->path)
		pTerm->path(pTerm, 1);
}
// 
// Squeeze all fill information into the old style parameter.
// The terminal drivers know how to extract the information.
// We assume that the style (int) has only 16 bit, therefore we take
// 4 bits for the style and allow 12 bits for the corresponding fill parameter.
// This limits the number of styles to 16 and the fill parameter's
// values to the range 0...4095, which seems acceptable.
// 
int style_from_fill(const fill_style_type * fs)
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
		default: style = FS_EMPTY; break; // solid fill with background color 
	}
	return style;
}
/*
 * Load dt with the properties of a user-defined dashtype.
 * Return: DASHTYPE_SOLID or DASHTYPE_CUSTOM or a positive number
 * if no user-defined dashtype was found.
 */
int load_dashtype(t_dashtype * dt, int tag)
{
	t_dashtype loc_dt = DEFAULT_DASHPATTERN;
	for(custom_dashtype_def * p_this = GPO.Gg.P_FirstCustomDashtype; p_this;) {
		if(p_this->tag == tag) {
			*dt = p_this->dashtype;
			memcpy(dt->dstring, p_this->dashtype.dstring, sizeof(dt->dstring));
			return p_this->d_type;
		}
		else
			p_this = p_this->next;
	}
	// not found, fall back to default, terminal-dependent dashtype 
	*dt = loc_dt;
	return tag - 1;
}

//void lp_use_properties(GpTermEntry * pTerm, lp_style_type * lp, int tag)
void GnuPlot::LpUseProperties(GpTermEntry * pTerm, lp_style_type * pLp, int tag)
{
	// This function looks for a linestyle defined by 'tag' and copies its data into the structure 'lp'.
	int save_flags = pLp->flags;
	for(linestyle_def * p_this = Gg.P_FirstLineStyle; p_this;) {
		if(p_this->tag == tag) {
			*pLp = p_this->lp_properties;
			pLp->flags = save_flags;
			return;
		}
		else
			p_this = p_this->next;
	}
	LoadLineType(pTerm, pLp, tag); // No user-defined style with p_this tag; fall back to default line type. 
}
// 
// Load lp with the properties of a user-defined linetype
// 
//void load_linetype(GpTermEntry * pTerm, lp_style_type * pLp, int tag)
void GnuPlot::LoadLineType(GpTermEntry * pTerm, lp_style_type * pLp, int tag)
{
	linestyle_def * p_this;
	bool recycled = false;
recycle:
	if((tag > 0) && (monochrome || (pTerm && (pTerm->flags & TERM_MONOCHROME)))) {
		for(p_this = Gg.P_FirstMonoLineStyle; p_this; p_this = p_this->next) {
			if(tag == p_this->tag) {
				*pLp = p_this->lp_properties;
				return;
			}
		}
		// This linetype wasn't defined explicitly.		
		// Should we recycle one of the first N linetypes?	
		if(tag > mono_recycle_count && mono_recycle_count > 0) {
			tag = (tag-1) % mono_recycle_count + 1;
			goto recycle;
		}
		return;
	}
	p_this = Gg.P_FirstPermLineStyle;
	while(p_this) {
		if(p_this->tag == tag) {
			// Always load color, width, and dash properties 
			pLp->l_type = p_this->lp_properties.l_type;
			pLp->l_width = p_this->lp_properties.l_width;
			pLp->pm3d_color = p_this->lp_properties.pm3d_color;
			pLp->d_type = p_this->lp_properties.d_type;
			pLp->CustomDashPattern = p_this->lp_properties.CustomDashPattern;
			// Needed in version 5.0 to handle old terminals (pbm hpgl ...) 
			// with no support for user-specified colors 
			if(pTerm && pTerm->set_color == GnuPlot::NullSetColor)
				pLp->l_type = tag;
			// Do not recycle point properties. 
			// FIXME: there should be a separate command "set pointtype cycle N" 
			if(!recycled) {
				pLp->PtType = p_this->lp_properties.PtType;
				pLp->p_interval = p_this->lp_properties.p_interval;
				pLp->PtSize = p_this->lp_properties.PtSize;
				memcpy(pLp->p_char, p_this->lp_properties.p_char, sizeof(pLp->p_char));
			}
			return;
		}
		else
			p_this = p_this->next;
	}
	// This linetype wasn't defined explicitly.		
	// Should we recycle one of the first N linetypes?	
	if(tag > linetype_recycle_count && linetype_recycle_count > 0) {
		tag = (tag-1) % linetype_recycle_count + 1;
		recycled = TRUE;
		goto recycle;
	}
	// No user-defined linetype with p_this tag; fall back to default line type. 
	// NB: We assume that the remaining fields of lp have been initialized. 
	pLp->l_type = tag - 1;
	pLp->pm3d_color.type = TC_LT;
	pLp->pm3d_color.lt = pLp->l_type;
	pLp->d_type = DASHTYPE_SOLID;
	pLp->PtType = (tag <= 0) ? -1 : tag - 1;
}
// 
// Version 5 maintains a parallel set of linetypes for "set monochrome" mode.
// This routine allocates space and initializes the default set.
// 
//void init_monochrome()
void GnuPlot::InitMonochrome()
{
	//lp_style_type mono_default[] = DEFAULT_MONO_LINETYPES;
	lp_style_type mono_default[6];//= DEFAULT_MONO_LINETYPES;
	for(uint i = 0; i < SIZEOFARRAY(mono_default); i++) {
		mono_default[i].SetDefault2();
		mono_default[i].pm3d_color.SetBlack();
	}
	mono_default[1].d_type = 1;
	mono_default[2].d_type = 2;
	mono_default[3].d_type = 3;
	mono_default[4].d_type = 0;
	mono_default[4].l_width = 2.0;
	mono_default[5].d_type = DASHTYPE_CUSTOM;
	mono_default[5].l_width = 1.2;
	mono_default[5].CustomDashPattern.SetPattern(16.f, 8.0f, 2.0f, 5.0f, 2.0f, 5.0f, 2.0f, 8.0f);
	if(Gg.P_FirstMonoLineStyle == NULL) {
		int n = sizeof(mono_default) / sizeof(struct lp_style_type);
		// copy default list into active list 
		for(int i = n; i > 0; i--) {
			linestyle_def * p_new = (linestyle_def *)SAlloc::M(sizeof(linestyle_def));
			p_new->next = Gg.P_FirstMonoLineStyle;
			p_new->lp_properties = mono_default[i-1];
			p_new->tag = i;
			Gg.P_FirstMonoLineStyle = p_new;
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
			    if(*s) s++;
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
// 
// The check for asynchronous events such as hotkeys and mouse clicks is
// normally done in term->waitforinput() while waiting for the next input
// from the command line.  If input is currently coming from a file or
// pipe instead, as with a "load" command, then this path would not be
// triggered automatically and these events would back up until input
// returned to the command line.  These code paths can explicitly call
// check_for_mouse_events() so that event processing is handled sooner.
// 
//void check_for_mouse_events()
void GnuPlot::CheckForMouseEvents(GpTermEntry * pTerm)
{
#ifdef USE_MOUSE
	if(TermInitialised && pTerm->waitforinput) {
		pTerm->waitforinput(TERM_ONLY_CHECK_MOUSING);
	}
#endif
#ifdef _WIN32
	// Process windows GUI events (e.g. for text window, or wxt and windows terminals) 
	WinMessageLoop();
	// On Windows, Ctrl-C only sets this flag. 
	// The next block duplicates the behaviour of inter(). 
	if(_Plt.ctrlc_flag) {
		_Plt.ctrlc_flag = false;
		TermReset(pTerm);
		putc('\n', stderr);
		fprintf(stderr, "Ctrl-C detected!\n");
		bail_to_command_line(); // return to prompt 
	}
#endif
}

char * escape_reserved_chars(const char * str, const char * reserved)
{
	int i;
	int newsize = strlen(str);
	/* Count number of reserved characters */
	for(i = 0; str[i] != '\0'; i++) {
		if(strchr(reserved, str[i]))
			newsize++;
	}
	char * escaped_str = (char *)SAlloc::M(newsize + 1);
	// Prefix each reserved character with a backslash 
	for(i = 0, newsize = 0; str[i] != '\0'; i++) {
		if(strchr(reserved, str[i]))
			escaped_str[newsize++] = '\\';
		escaped_str[newsize++] = str[i];
	}
	escaped_str[newsize] = '\0';
	return escaped_str;
}
