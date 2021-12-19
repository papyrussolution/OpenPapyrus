// GNUPLOT - post.trm 
// Copyright 1990 - 1993, 1998, 1999, 2000, 2001, 2004
//
/*
 * This terminal driver supports: postscript
 *
 * AUTHORS: Russell Lang  <rjl@monu1.cc.monash.edu.au>
 *
 * modified 10/5/95 by drd - put in support for other postscript drivers
 * (enhpost, pslatex, ...) so they dont have to work quite so hard
 *
 * send your comments or suggestions to (gnuplot-info@lists.sourceforge.net).
 *
 * The 'postscript' driver produces landscape output 10" wide and 7" high.
 * To change font to Times-Roman and font size to 20pts use
 * 'set term postscript "Times-Roman" 20'.
 * To get a smaller (5" x 3.5") eps output use 'set term post eps'
 * and make only one plot per file.  Font size for eps will be half
 * the specified size.
 *
 * Erik Luijten 30/5/97: added %%CreationDate, made %%DocumentFonts conform to DSC, added version no. and patchl. to %%Creator
 * Petr Mikulik, Jan 1999: terminal entries for PM3D functionality
 * Dick Crawford 24/5/00: added 'a{}{}' syntax to allow for overprinting
 * Dan Sebald, 7 March 2003: terminal entry for image functionality
 * Harald Harders (h.harders@tu-bs.de), 2004-12-02: Moved all terminal settings into a single structure.
 * Harald Harders (h.harders@tu-bs.de), 2005-02-08: Merged functionality of postscript, pslatex, pstex, and epslatex terminals.
 * Ethan Merritt Mar 2006:   Break out prolog and character encodings into separate files loaded at runtime
 * Thomas Henlich Sep 2007:   Add support for UTF-8 encoding via the glyphshow
 * operator.  It supports PostScript Type1 fonts that use glyph names according
 * to the Adobe Glyph List For New Fonts.
 *
 * Christoph Bersch Nov 2011 (cvs Apr 2013): Add support for Level 3 /FlateDecode filter for plotstyle 'with image'.
 *
 * Ethan Merritt Sep 2016:  Support rotatation of boxed text, including text
 * strings using the glyphshow mechanism.  Special-case the 'minus sign' glyph
 * at U+2212 by unconditionally mapping it to /minus.

 * Ethan Merritt Jul 2019:  Consolidate PS_set_font and ENHPS_set_font,
 * handle `set font "/:Bold"` in either case (note superfluous /).
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
	register_term(post)
#endif

//#ifdef TERM_PROTO
static bool PS_newpath = FALSE;
static bool ENHps_opened_string = FALSE; /* try to cut out empty ()'s */
static int PS_in_textbox = 0;
//#endif /* TERM_PROTO */

#ifndef TERM_PROTO_ONLY

#ifdef TERM_BODY
	#include "post.h"
#ifdef HAVE_GD_PNG
	#include "gd.h"
	#define HAVE_DEFLATE_ENCODER
#else
	#ifdef HAVE_CAIROPDF
		#include "wxterminal/gp_cairo.h"
		#define HAVE_DEFLATE_ENCODER
	#endif
#endif
//
// Data structure implementing inclusion of font files 
//
#define POST_PARAMS_DEFAULT { \
		PSTERM_POSTSCRIPT, 50, 50, PSTERM_LANDSCAPE, FALSE, FALSE, FALSE, FALSE, FALSE, 1.0, 1.0, 1.0, FALSE, \
		FALSE, FALSE, FALSE, NULL, "Helvetica", 14., 1., FALSE, FALSE, 2000, 0.003, FALSE, TRUE, FALSE, {-1., -1., -1.} \
}

static ps_params_t post_params(PSTERM_POSTSCRIPT);// = POST_PARAMS_DEFAULT;
static const ps_params_t post_params_default(PSTERM_POSTSCRIPT);// = POST_PARAMS_DEFAULT;

#define EPSLATEX_PARAMS_DEFAULT { \
		PSTERM_EPSLATEX, 50, 50, PSTERM_EPS, FALSE, FALSE, FALSE, TRUE, FALSE, 1.0, 1.0, 1.0, FALSE, \
		FALSE, FALSE, FALSE, NULL, "", 11., 1., TRUE, FALSE, 2000, 0.003, FALSE, FALSE, FALSE, {-1., -1., -1.} \
}
static ps_params_t epslatex_params(PSTERM_EPSLATEX);// = EPSLATEX_PARAMS_DEFAULT;
static const ps_params_t epslatex_params_default(PSTERM_EPSLATEX);//= EPSLATEX_PARAMS_DEFAULT;

#define PSLATEX_PARAMS_DEFAULT { \
		PSTERM_PSLATEX, 0, 0, PSTERM_EPS, FALSE, FALSE, FALSE, TRUE, FALSE, 1.0, 1.0, 1.0, FALSE, \
		FALSE, FALSE, FALSE, NULL, "", 0., 1., FALSE, TRUE, 2000, 0.003, FALSE, FALSE, FALSE, {-1., -1., -1.} \
}
static ps_params_t pslatex_params(PSTERM_PSLATEX);// = PSLATEX_PARAMS_DEFAULT;
static const ps_params_t pslatex_params_default(PSTERM_PSLATEX); //= PSLATEX_PARAMS_DEFAULT;

#define PSTEX_PARAMS_DEFAULT { \
		PSTERM_PSTEX, 0, 0, PSTERM_EPS, FALSE, FALSE, FALSE, TRUE, FALSE, 1.0, 1.0, 1.0, FALSE, \
		FALSE, FALSE, FALSE, NULL, "", 0., 1., FALSE, TRUE, 2000, 0.003, FALSE, FALSE, FALSE, {-1., -1., -1.} \
}
static ps_params_t pstex_params(PSTERM_PSTEX);// = PSTEX_PARAMS_DEFAULT;
static const ps_params_t pstex_params_default(PSTERM_PSTEX);// = PSTEX_PARAMS_DEFAULT;

ps_params_t * ps_params = &post_params;

GpPostscriptBlock::GpPostscriptBlock() : FontSize(14.0f), FontSizePrevious(-1.0f), Page(0), PathCount(0), Ang(0), Justify(LEFT), EnhFontSize(0.0f),
	EnsPsInitialized(0), P_Params(&post_params), PsLatexAuxname(0)
{
	memzero(EnhFont, sizeof(EnhFont));
}

static void make_interpolation_code();
static void make_color_model_code();
static void write_component_array(const char * text, gradient_struct * grad, int cnt, int offset);
static void write_gradient_definition(gradient_struct * gradient, int cnt);
static void write_color_space(t_sm_palette * palette);
static void make_palette_formulae(GpTermEntry_Static * pThis);
static void PS_make_header(GpTermEntry_Static * pThis, t_sm_palette * palette);
static void PS_skip_image(GpTermEntry_Static * pThis, int bytes, int x0, int y0, int dx, int dy);
#ifndef GNUPLOT_PS_DIR
	static void PS_dump_header_to_file(GpTermEntry_Static * pThis, char * name);
#endif

static void delete_ps_fontfile(GpTermEntry_Static * pThis, ps_fontfile_def *, ps_fontfile_def *);
TERM_PUBLIC void PS_load_fontfile(GpTermEntry_Static * pThis, ps_fontfile_def *, bool);
TERM_PUBLIC void PS_load_fontfiles(GpTermEntry_Static * pThis, bool);
static char * fontpath_fullname(GpTermEntry_Static * pThis, const char * name, const char * dir);

static GpSizeUnits ps_explicit_units = INCHES;
static int    eps_explicit_x = 0;
static int    eps_explicit_y = 0;
static int    PS_pen_x;
static int    PS_pen_y;
static int    PS_taken;
static int    PS_linetype_last;
static double PS_linewidth_last;
static double PS_linewidth_current;
static bool   ps_explicit_size = FALSE;
static bool   PS_border = FALSE;
static bool   PS_relative_ok;

#define DOTS_PER_INCH (300)    /* resolution of printer we expect to use */

static void FASTCALL PsFlashPath(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(p_gp->TPsB.PathCount) {
		fputs("stroke\n", GPT.P_GpPsFile);
		p_gp->TPsB.PathCount = 0;
		PS_relative_ok = FALSE;
	}
}

//static char * pslatex_auxname = NULL; // name of auxiliary file 
// Routine to copy pre-existing prolog files into output stream 
static void PS_dump_prologue_file(GpTermEntry_Static * pThis, char *);
static void PS_load_glyphlist(GpTermEntry_Static * pThis);

static const char * OldEPSL_linetypes[] = {
/* Line Types */
	"% Redefine line types to match old epslatex driver\n",
	"/LTw { PL [] 1 setgray } def\n", /* background (assumed white) */
	"/LTb { BL [] 0 0 0 DL } def\n", /* border */
	"/LTa { AL [1 udl mul 2 udl mul] 0 setdash 0 0 0 setrgbcolor } def\n", /* axes */
	"/LT0 { PL [] 1 0 0 DL } def\n",
	"/LT1 { PL [8 dl1 5 dl1] 0 0 1 DL } def\n",
	"/LT2 { PL [4 dl1 4 dl1] 0 1 1 DL } def\n",
	"/LT3 { PL [8 dl1 5 dl1 0.5 dl1 5 dl1] 1 0 1 DL } def\n",
	NULL
};

static const char * ENHPS_header[] = {
/* For MFshow and MFwidth the tos is an array with the string and font info:  */
/*	[<fontname (a string)> <fontsize> <vertical offset> <width significant?> <printed?> <overprint> <text string>]
    */
/* EAM Mar 2004 - Add in a special case overprint 3 = save, overprint 4 = restore */
/* EAM Nov 2007 - Accommodate UTF-8 support (Gshow) */
/* EAM Sep 2016 - rotatable textbox */

	"/Metrics {ExtendTextBox Gswidth} def\n",
	"/Lwidth {currentpoint stroke M 0 vshift R Metrics} def\n",
	"/Rwidth {currentpoint stroke M dup stringwidth pop neg vshift R Metrics} def\n",
	"/Cwidth {currentpoint stroke M dup stringwidth pop -2 div vshift R Metrics} def\n",
	"/GLwidth {currentpoint stroke M 0 vshift R {ExtendTextBox} forall} def\n",
	"/GRwidth {currentpoint stroke M dup Gwidth vshift R {ExtendTextBox} forall} def\n",
	"/GCwidth {currentpoint stroke M dup Gwidth 2 div vshift R {ExtendTextBox} forall} def\n",
	"/GLwidth2 {0 Gwidth AddGlyphWidth} def\n",
	"/GRwidth2 {Gwidth -1 mul 0 AddGlyphWidth} def\n",
	"/GCwidth2 {Gwidth 2 div dup -1 mul AddGlyphWidth} def\n",
	"/AddGlyphWidth { dup TBx2 gt {userdict /TBx2 3 -1 roll put} {pop} ifelse\n",
	"                 dup TBx1 lt {userdict /TBx1 3 -1 roll put} {pop} ifelse } def\n",
	"/MFshow {\n",
	"   { dup 5 get 3 ge\n", /* EAM test for overprint 3 or 4 */
	"     { 5 get 3 eq {gsave} {grestore} ifelse }\n", /* EAM */
	"     {dup dup 0 get findfont exch 1 get scalefont setfont\n",
	"     [ currentpoint ] exch dup 2 get 0 exch R dup 5 get 2 ne {dup dup 6\n",
	"     get exch 4 get {textshow} {Metrics pop 0 R} ifelse }if dup 5 get 0 eq\n",
	"     {dup 3 get {2 get neg 0 exch R pop} {pop aload pop M} ifelse} {dup 5\n",
	"     get 1 eq {dup 2 get exch dup 3 get exch 6 get Gswidth pop -2 div\n",
	"     dup 0 R} {dup 6 get Gswidth pop -2 div 0 R 6 get\n",
	"     textshow 2 index {aload pop M neg 3 -1 roll neg R pop pop} {pop pop pop\n",
	"     pop aload pop M} ifelse }ifelse }ifelse }\n",
	"     ifelse }\n", /* EAM */
	"   forall} def\n",

/* get the width of the text */
/* HH 2005-07-24 - Add in a special case overprint 3 = save, 4 = restore
 * also for estimation of string width. This is done by interposing an
 * additional value on the stack. between XYsave and XYrestore,
 * this number is increased by the strings. By pop'ing this number, all
 * strings between XYsave and XYrestore are ignored. */
/* EAM Nov 2007 - GSwidth is to allow the operator to work either with a string
 * or with a glyph.  Needed for UTF-8 support. Gwidth may do it better. */
	"/Gswidth {dup type /stringtype eq {stringwidth} {pop (n) stringwidth} ifelse} def\n",
	"/MFwidth {0 exch { dup 5 get 3 ge { 5 get 3 eq { 0 } { pop } ifelse }\n",
	" {dup 3 get{dup dup 0 get findfont exch 1 get scalefont setfont\n",
	"     6 get Gswidth pop add} {pop} ifelse} ifelse} forall} def\n",

/* flush left show */
	"/MLshow { currentpoint stroke M\n",
	"  0 exch R\n  Blacktext {gsave 0 setgray MFshow grestore} {MFshow} ifelse } bind def\n",

/* flush right show */
	"/MRshow { currentpoint stroke M\n",
	"  exch dup MFwidth neg 3 -1 roll R\n  Blacktext {gsave 0 setgray MFshow grestore} {MFshow} ifelse } bind def\n",

/* centred show */
	"/MCshow { currentpoint stroke M\n",
	"  exch dup MFwidth -2 div 3 -1 roll R\n  Blacktext {gsave 0 setgray MFshow grestore} {MFshow} ifelse } bind def\n",

/* Save and restore for @-text (phantom box) */
	"/XYsave    { [( ) 1 2 true false 3 ()] } bind def\n",
	"/XYrestore { [( ) 1 2 true false 4 ()] } bind def\n",

	NULL
};

static const char psi4[] =
    "\
%%\n\
%% Support for boxed text - Ethan A Merritt Sep 2016\n%%\n\
/InitTextBox { userdict /TBy2 3 -1 roll put userdict /TBx2 3 -1 roll put\n\
           userdict /TBy1 3 -1 roll put userdict /TBx1 3 -1 roll put\n\
	   /Boxing true def } def\n\
/ExtendTextBox { dup type /stringtype eq\n\
    { Boxing { gsave dup false charpath pathbbox\n\
      dup TBy2 gt {userdict /TBy2 3 -1 roll put} {pop} ifelse\n\
      dup TBx2 gt {userdict /TBx2 3 -1 roll put} {pop} ifelse\n\
      dup TBy1 lt {userdict /TBy1 3 -1 roll put} {pop} ifelse\n\
      dup TBx1 lt {userdict /TBx1 3 -1 roll put} {pop} ifelse\n\
      grestore } if }\n\
    {} ifelse} def\n\
/PopTextBox { newpath TBx1 TBxmargin sub TBy1 TBymargin sub M\n\
               TBx1 TBxmargin sub TBy2 TBymargin add L\n\
	       TBx2 TBxmargin add TBy2 TBymargin add L\n\
	       TBx2 TBxmargin add TBy1 TBymargin sub L closepath } def\n\
/DrawTextBox { PopTextBox stroke /Boxing false def} def\n\
/FillTextBox { gsave PopTextBox fill grestore /Boxing false def} def\n\
0 0 0 0 InitTextBox\n\
/TBxmargin 20 def\n\
/TBymargin 20 def\n\
/Boxing false def\n\
/textshow { ExtendTextBox Gshow } def\n%%\n";

/* external/internal prologue files machinery */
#if defined(GNUPLOT_PS_DIR)
	#if defined(_WIN32)
		#include "win/winmain.h"
	#endif
#else /* GNUPLOT_PS_DIR */
	#include "PostScript/prologues.h"
#endif /* GNUPLOT_PS_DIR */

/* added to enhpost by Matt Heffron <heffron@falstaff.css.beckman.com> */
/* moved to post.trm by drd */

static struct PS_FontName {
	char * name;
	struct PS_FontName * next;
} * PS_DocFonts = NULL;

static char PS_default_font[2*MAX_ID_LEN] = {'H', 'e', 'l', 'v', 'e', 't', 'i', 'c', 'a', ',', '1', '4', '\0'};

/* given a font, look in store to see if it is there already
 * if so, return NULL. If not, reencode it if allowed to, otherwise
 * return an appropriate re-encode string
 */
static void PS_RememberFont(GpTermEntry_Static * pThis, char * fname)
{
	struct PS_FontName * fnp;
	char * recode = NULL;
	char * myfname = "Symbol";
	if(strcmp(fname, "Symbol-Oblique") != 0)
		myfname = fname;
	/* There is some confusion between a font name in an enhanced text */
	/* string ({/Font}) and a bare name from set_font("Font"), because */
	/* the user may have placed an unneeded / in front of the latter.  */
	/* This is particularly a problem if the request is for "/:Bold".  */
	if(*myfname == '/')
		myfname++;
	if(*myfname == ':')
		return;
	for(fnp = PS_DocFonts; fnp; fnp = fnp->next)
		if(strcmp(fnp->name, myfname) == 0)
			return;
	/* Ignore it if illegal characters will corrupt the PostScript syntax */
	if(strpbrk(myfname, "{}[]() "))
		return;
	/* we have not seen this font before; store name and apply encoding */
	fnp = (struct PS_FontName *)SAlloc::M(sizeof(struct PS_FontName));
	fnp->name = sstrdup(myfname);
	fnp->next = PS_DocFonts;
	PS_DocFonts = fnp;
	switch(GPT._Encoding) {
		case S_ENC_ISO8859_1:
		case S_ENC_UTF8: recode = "reencodeISO def\n"; break;
		case S_ENC_ISO8859_2: recode = "reencodeISO2 def\n"; break;
		case S_ENC_ISO8859_9: /* ISO8859-9 is Latin5 */
		case S_ENC_CP1254: recode = "reencodeISO9 def\n"; break;
		case S_ENC_ISO8859_15: recode = "reencodeISO15 def\n"; break; /* ISO8859-15 is Latin9 */
		case S_ENC_CP437: recode = "reencodeCP437 def\n"; break;
		case S_ENC_CP850: recode = "reencodeCP850 def\n"; break;
		case S_ENC_CP852: recode = "reencodeCP852 def\n"; break;
		case S_ENC_KOI8_R: recode = "reencodeKOI8R def\n"; break;
		case S_ENC_CP1250: recode = "reencodeCP1250 def\n"; break;
		case S_ENC_CP1251: recode = "reencodeCP1251 def\n"; break;
		case S_ENC_CP1252: recode = "reencodeCP1252 def\n"; break;
		case S_ENC_KOI8_U: recode = "reencodeKOI8U def\n"; break;
		default: break; /* do nothing */
	}
	if(recode) {
		if(ENHps_opened_string)
			ENHPS_FLUSH(pThis);
		fprintf(GPT.P_GpPsFile, "/%s %s", fnp->name, recode);
	}
}

char * PS_escape_string(char * origstr, char * escapelist)
{
	char * newstr = 0;
	if(!isempty(origstr)) {
		newstr = (char *)SAlloc::M(2*strlen(origstr)+1);
		if(newstr) {
			char * n;
			for(n = newstr; *origstr; *n++ = *origstr++) {
				if(strchr(escapelist, *origstr))
					*n++ = '\\';
			}
			*n = '\0';
		}
	}
	return newstr;
}

// HBB 990914: PS_SOLID is already used by the WIN32 API headers.
// Renamed to PS_SOLIDE, therefore... 
enum PS_id {
	PS_PORTRAIT, PS_LANDSCAPE,
	PS_EPSF, PS_DEFAULT, PS_ENHANCED, PS_NOENHANCED,
	PS_LATEX, EPSLATEX_STANDALONE, EPSLATEX_INPUT,
	PS_MONOCHROME, PS_COLOR, PS_BLACKTEXT, PS_COLORTEXT,
	PS_SOLIDE, PS_DASHED, PS_DASHLENGTH, PS_LINEWIDTH, PS_POINTSCALE,
	PS_SIMPLEX, PS_DUPLEX, PS_DEFAULTPLEX,
	PS_ROUNDED, PS_NOROUNDED, PS_CLIP, PS_NOCLIP, PS_FONTFILE, PS_NOFONTFILES,
	PS_PALFUNCPARAM,
	PS_LEVEL1, PS_LEVELDEFAULT, PS_LEVEL3, PS_FONT, PS_FONTSCALE,
	PSLATEX_ROTATE, PSLATEX_NOROTATE, PSLATEX_AUXFILE, PSLATEX_NOAUXFILE,
	PSLATEX_OLDSTYLE, PSLATEX_NEWSTYLE, EPSLATEX_HEADER, EPSLATEX_NOHEADER,
	PS_SIZE,
	PS_ADOBEGLYPHNAMES, PS_NOADOBEGLYPHNAMES,
	PS_BACKGROUND, PS_NOBACKGROUND,
	PS_OTHER
};

static struct gen_table PS_opts[] =
{
	{ "d$efault", PS_DEFAULT },
	{ "p$ortrait", PS_PORTRAIT },
	{ "l$andscape", PS_LANDSCAPE },
	{ "ep$sf", PS_EPSF },
	{ "enh$anced", PS_ENHANCED },
	{ "noenh$anced", PS_NOENHANCED },
	{ "m$onochrome", PS_MONOCHROME },
	{ "c$olor", PS_COLOR },
	{ "c$olour", PS_COLOR },
	{ "b$lacktext", PS_BLACKTEXT },
	{ "colort$ext", PS_COLORTEXT },
	{ "colourt$ext", PS_COLORTEXT },
	{ "so$lid", PS_SOLIDE },
	{ "da$shed", PS_DASHED },
	{ "dashl$ength", PS_DASHLENGTH },
	{ "dl", PS_DASHLENGTH },
	{ "linew$idth", PS_LINEWIDTH },
	{ "lw", PS_LINEWIDTH },
	{ "pointscale", PS_POINTSCALE },
	{ "ps", PS_POINTSCALE },
	{ "size", PS_SIZE },
	{ "si$mplex", PS_SIMPLEX },
	{ "du$plex", PS_DUPLEX },
	{ "defaultp$lex", PS_DEFAULTPLEX },
	{ "butt", PS_NOROUNDED },
	{ "rou$nded", PS_ROUNDED },
	{ "clip", PS_CLIP },
	{ "noclip", PS_NOCLIP },
	{ "fontf$ile", PS_FONTFILE },
	{ "fontscale", PS_FONTSCALE },
	{ "nofontf$iles", PS_NOFONTFILES },
	{ "palf$uncparam", PS_PALFUNCPARAM },
	{ "level1", PS_LEVEL1 },
	{ "leveldefault", PS_LEVELDEFAULT },
	{ "level3", PS_LEVEL3 },
	{ "font", PS_FONT },
	{ "stand$alone", EPSLATEX_STANDALONE },
	{ "inp$ut", EPSLATEX_INPUT },
	{ "header", EPSLATEX_HEADER },
	{ "noheader", EPSLATEX_NOHEADER },
	{ "r$otate", PSLATEX_ROTATE },
	{ "n$orotate", PSLATEX_NOROTATE },
	{ "a$uxfile", PSLATEX_AUXFILE },
	{ "noa$uxfile", PSLATEX_NOAUXFILE },
	{ "old$style", PSLATEX_OLDSTYLE },
	{ "new$style", PSLATEX_NEWSTYLE },
	{ "adobe$glyphnames", PS_ADOBEGLYPHNAMES },
	{ "noadobe$glyphnames", PS_NOADOBEGLYPHNAMES },
	{ "backg$round", PS_BACKGROUND },
	{ "nobackg$round", PS_NOBACKGROUND },
	{ NULL, PS_OTHER }
};

//#define PS_SCF (PS_SC * GPO.TPsB.P_Params->fontscale) // EAM March 2010 allow user to rescale fonts 
static float _ps_scf(const GnuPlot * pGp) { return (PS_SC * pGp->TPsB.P_Params->fontscale); }

void PS_options(GpTermEntry_Static * pThis, GnuPlot * pGp)
{
	char * s;
	char * ps_fontfile_char = NULL;
	char tmp_term_options[MAX_LINE_LEN+1] = "";
	bool set_orientation = FALSE;
	bool set_enhanced = FALSE;
	bool set_plex = FALSE;
	bool set_level = FALSE;
	bool set_color = FALSE;
	bool set_dashlen = FALSE;
	bool set_linewidth = FALSE;
	bool set_round = FALSE;
	bool set_pointscale = FALSE;
	bool set_clip = FALSE;
	bool set_palfunc = FALSE;
	bool set_colortext = FALSE;
	bool set_standalone = FALSE;
	bool set_epslheader = FALSE;
	bool set_pslrotate = FALSE;
	bool set_pslauxfile = FALSE;
	bool set_psloldstyle = FALSE;
	bool set_font = FALSE;
	bool set_fontsize = FALSE;
	bool set_fontscale = FALSE;
	// Annoying hack to handle the case of 'set termoption' after 
	// we have already initialized the terminal.                  
	if(!pGp->Pgm.AlmostEquals(pGp->Pgm.GetPrevTokenIdx(), "termopt$ion"))
		ps_explicit_size = FALSE;
	if(strcmp(pThis->name, "pstex") == 0)
		pGp->TPsB.P_Params = &pstex_params;
	else if(strcmp(pThis->name, "pslatex") == 0)
		pGp->TPsB.P_Params = &pslatex_params;
	else if(strcmp(pThis->name, "epslatex") == 0)
		pGp->TPsB.P_Params = &epslatex_params;
	else
		pGp->TPsB.P_Params = &post_params;
	if(pGp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT) {
		ZFREE(pGp->TPsB.PsLatexAuxname);
	}
	if(!pGp->Pgm.EndOfCommand()) {
		if(pGp->Pgm.LookupTableForCurrentToken(&PS_opts[0]) == PS_DEFAULT) {
			switch(pGp->TPsB.P_Params->terminal) {
				case PSTERM_POSTSCRIPT:
				    while(pGp->TPsB.P_Params->first_fontfile)
					    delete_ps_fontfile(pThis, 0, pGp->TPsB.P_Params->first_fontfile);
				    *pGp->TPsB.P_Params = post_params_default;
				    break;
				case PSTERM_EPSLATEX:
				    *pGp->TPsB.P_Params = epslatex_params_default;
				    break;
				case PSTERM_PSLATEX:
				    *pGp->TPsB.P_Params = pslatex_params_default;
				    break;
				case PSTERM_PSTEX:
				    *pGp->TPsB.P_Params = pstex_params_default;
				    break;
			}
			pGp->Pgm.Shift();
			if(!pGp->Pgm.EndOfCommand())
				goto PS_options_error;
		}
	}
	// Default to enhanced text 
	if(pGp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT) {
		pThis->put_text = ENHPS_put_text;
		pThis->set_font = ENHPS_set_font;
		pThis->SetFlag(TERM_ENHANCED_TEXT);
	}
	else {
		pThis->set_font = PS_set_font;
		pThis->ResetFlag(TERM_ENHANCED_TEXT);
	}
	while(!pGp->Pgm.EndOfCommand()) {
		switch(pGp->Pgm.LookupTableForCurrentToken(&PS_opts[0])) {
			case PS_PORTRAIT:
			    if(set_orientation || pGp->TPsB.P_Params->terminal != PSTERM_POSTSCRIPT)
				    goto PS_options_error;
			    set_orientation = TRUE;
			    pGp->TPsB.P_Params->psformat = PSTERM_PORTRAIT;
			    pGp->Pgm.Shift();
			    break;
			case PS_LANDSCAPE:
			    if(set_orientation || pGp->TPsB.P_Params->terminal != PSTERM_POSTSCRIPT)
				    goto PS_options_error;
			    set_orientation = TRUE;
			    pGp->TPsB.P_Params->psformat = PSTERM_LANDSCAPE;
			    pGp->Pgm.Shift();
			    break;
			case PS_EPSF:
			    if(set_orientation || pGp->TPsB.P_Params->terminal != PSTERM_POSTSCRIPT)
				    goto PS_options_error;
			    set_orientation = TRUE;
			    pGp->TPsB.P_Params->psformat = PSTERM_EPS;
			    pGp->Pgm.Shift();
			    break;
			case PS_LEVEL1:
			    if(set_level)
				    goto PS_options_error;
			    set_level = TRUE;
			    pGp->TPsB.P_Params->level1 = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PS_LEVELDEFAULT:
			    if(set_level)
				    goto PS_options_error;
			    set_level = TRUE;
			    pGp->TPsB.P_Params->level1 = FALSE;
			    pGp->TPsB.P_Params->level3 = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case PS_LEVEL3:
			    if(set_level)
				    goto PS_options_error;
			    set_level = TRUE;
			    pGp->TPsB.P_Params->level3 = TRUE;
#ifndef HAVE_DEFLATE_ENCODER
			    pGp->IntErrorCurToken("level3 output requires libgd or libcairo");
#endif
			    pGp->Pgm.Shift();
			    break;
			case PS_DEFAULT:
			    goto PS_options_error;
			    pGp->Pgm.Shift();
			    break;
			case PS_ENHANCED:
			    if(set_enhanced || pGp->TPsB.P_Params->terminal != PSTERM_POSTSCRIPT)
				    goto PS_options_error;
			    set_enhanced = TRUE;
			    pThis->put_text = ENHPS_put_text;
			    pThis->set_font = ENHPS_set_font;
			    pThis->SetFlag(TERM_ENHANCED_TEXT);
			    pGp->Pgm.Shift();
			    break;
			case PS_NOENHANCED:
			    if(set_enhanced || pGp->TPsB.P_Params->terminal != PSTERM_POSTSCRIPT)
				    goto PS_options_error;
			    set_enhanced = TRUE;
			    pThis->put_text = PS_put_text;
			    pThis->set_font = PS_set_font;
			    pThis->ResetFlag(TERM_ENHANCED_TEXT);
			    pGp->Pgm.Shift();
			    break;
#ifdef PSLATEX_DRIVER
			case EPSLATEX_STANDALONE:
			    if(set_standalone || pGp->TPsB.P_Params->terminal != PSTERM_EPSLATEX)
				    goto PS_options_error;
			    set_standalone = TRUE;
			    pGp->TPsB.P_Params->epslatex_standalone = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case EPSLATEX_INPUT:
			    if(set_standalone || pGp->TPsB.P_Params->terminal != PSTERM_EPSLATEX)
				    goto PS_options_error;
			    set_standalone = TRUE;
			    pGp->TPsB.P_Params->epslatex_standalone = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case EPSLATEX_HEADER:
			    if(set_epslheader || pGp->TPsB.P_Params->terminal != PSTERM_EPSLATEX)
				    goto PS_options_error;
			    set_epslheader = TRUE;
			    pGp->Pgm.Shift();
			    ZFREE(epslatex_header);
			    // Protect against pGp->IntError() bail from pGp->TryToGetString() 
			    epslatex_header = pGp->TryToGetString();
			    if(!epslatex_header)
				    pGp->IntErrorCurToken("String containing header information expected");
			    break;

			case EPSLATEX_NOHEADER:
			    if(set_epslheader || pGp->TPsB.P_Params->terminal != PSTERM_EPSLATEX)
				    goto PS_options_error;
			    set_epslheader = TRUE;
			    ZFREE(epslatex_header);
			    pGp->Pgm.Shift();
			    break;
			case PSLATEX_ROTATE:
			    if(set_pslrotate || ((pGp->TPsB.P_Params->terminal != PSTERM_PSLATEX) && (pGp->TPsB.P_Params->terminal != PSTERM_PSTEX)))
				    goto PS_options_error;
			    set_pslrotate = TRUE;
			    pGp->TPsB.P_Params->rotate = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PSLATEX_NOROTATE:
			    if(set_pslrotate || ((pGp->TPsB.P_Params->terminal != PSTERM_PSLATEX) &&
				(pGp->TPsB.P_Params->terminal != PSTERM_PSTEX)))
				    goto PS_options_error;
			    set_pslrotate = TRUE;
			    pGp->TPsB.P_Params->rotate = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case PSLATEX_AUXFILE:
			    if(set_pslauxfile || ((pGp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT) || (pGp->TPsB.P_Params->terminal == PSTERM_EPSLATEX)))
				    goto PS_options_error;
			    set_pslauxfile = TRUE;
			    pGp->TPsB.P_Params->useauxfile = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PSLATEX_NOAUXFILE:
			    if(set_pslauxfile || ((pGp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT) || (pGp->TPsB.P_Params->terminal == PSTERM_EPSLATEX)))
				    goto PS_options_error;
			    set_pslauxfile = TRUE;
			    pGp->TPsB.P_Params->useauxfile = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case PSLATEX_OLDSTYLE:
			    if(set_psloldstyle || pGp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT)
				    goto PS_options_error;
			    set_psloldstyle = TRUE;
			    pGp->TPsB.P_Params->oldstyle = TRUE;
			    if(pGp->TPsB.P_Params->terminal == PSTERM_EPSLATEX)
				    pGp->TPsB.P_Params->rounded = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PSLATEX_NEWSTYLE:
			    if(set_psloldstyle || pGp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT)
				    goto PS_options_error;
			    set_psloldstyle = TRUE;
			    pGp->TPsB.P_Params->oldstyle = FALSE;
			    pGp->Pgm.Shift();
			    break;
#endif
			case PS_MONOCHROME:
			    if(set_color)
				    goto PS_options_error;
			    set_color = TRUE;
			    pGp->TPsB.P_Params->color = FALSE;
			    pThis->SetFlag(TERM_MONOCHROME);
			    pGp->Pgm.Shift();
			    break;
			case PS_COLOR:
			    if(set_color)
				    goto PS_options_error;
			    set_color = TRUE;
			    pGp->TPsB.P_Params->color = TRUE;
			    pThis->ResetFlag(TERM_MONOCHROME);
			    pGp->Pgm.Shift();
			    break;
			case PS_BLACKTEXT:
			    if(set_colortext || ((pGp->TPsB.P_Params->terminal != PSTERM_POSTSCRIPT) &&
				(pGp->TPsB.P_Params->terminal != PSTERM_EPSLATEX)))
				    goto PS_options_error;
			    set_colortext = TRUE;
			    pGp->TPsB.P_Params->blacktext = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PS_COLORTEXT:
			    if(set_colortext || ((pGp->TPsB.P_Params->terminal != PSTERM_POSTSCRIPT) &&
				(pGp->TPsB.P_Params->terminal != PSTERM_EPSLATEX)))
				    goto PS_options_error;
			    set_colortext = TRUE;
			    pGp->TPsB.P_Params->blacktext = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case PS_SOLIDE:
			case PS_DASHED:
			    /* Version 5 always allows dashes */
			    pGp->TPsB.P_Params->solid = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case PS_DASHLENGTH:
			    if(set_dashlen)
				    goto PS_options_error;
			    set_dashlen = TRUE;
			    pGp->Pgm.Shift();
			    pGp->TPsB.P_Params->dash_length = pGp->FloatExpression();
			    if(pGp->TPsB.P_Params->dash_length <= 0.0)
				    pGp->TPsB.P_Params->dash_length = 1.0;
			    break;
			case PS_LINEWIDTH:
			    if(set_linewidth)
				    goto PS_options_error;
			    set_linewidth = TRUE;
			    pGp->Pgm.Shift();
			    pGp->TPsB.P_Params->linewidth_factor = pGp->FloatExpression();
			    if(pGp->TPsB.P_Params->linewidth_factor <= 0.0)
				    pGp->TPsB.P_Params->linewidth_factor = 1.0;
			    break;
			case PS_POINTSCALE:
			    if(set_pointscale)
				    goto PS_options_error;
			    set_pointscale = TRUE;
			    pGp->Pgm.Shift();
			    pGp->TPsB.P_Params->pointscale_factor = pGp->FloatExpression();
			    if(pGp->TPsB.P_Params->pointscale_factor <= 0.0)
				    pGp->TPsB.P_Params->pointscale_factor = 1.0;
			    break;
			case PS_SIMPLEX:
			    if(set_plex || pGp->TPsB.P_Params->terminal != PSTERM_POSTSCRIPT)
				    goto PS_options_error;
			    set_plex = TRUE;
			    pGp->TPsB.P_Params->duplex_state  = FALSE;
			    pGp->TPsB.P_Params->duplex_option = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PS_DUPLEX:
			    if(set_plex || pGp->TPsB.P_Params->terminal != PSTERM_POSTSCRIPT)
				    goto PS_options_error;
			    set_plex = TRUE;
			    pGp->TPsB.P_Params->duplex_state  = TRUE;
			    pGp->TPsB.P_Params->duplex_option = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PS_DEFAULTPLEX:
			    if(set_plex || pGp->TPsB.P_Params->terminal != PSTERM_POSTSCRIPT)
				    goto PS_options_error;
			    set_plex = TRUE;
			    pGp->TPsB.P_Params->duplex_option = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case PS_ROUNDED:
			    if(set_round)
				    goto PS_options_error;
			    set_round = TRUE;
			    pGp->TPsB.P_Params->rounded = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PS_NOROUNDED:
			    if(set_round)
				    goto PS_options_error;
			    set_round = TRUE;
			    pGp->TPsB.P_Params->rounded = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case PS_CLIP:
			    if(set_clip)
				    goto PS_options_error;
			    set_clip = TRUE;
			    pGp->TPsB.P_Params->clipped = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PS_NOCLIP:
			    if(set_clip)
				    goto PS_options_error;
			    set_clip = TRUE;
			    pGp->TPsB.P_Params->clipped = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case PS_FONTSCALE:
			    if(set_fontscale)
				    goto PS_options_error;
			    set_fontscale = TRUE;
			    pGp->Pgm.Shift();
			    pGp->TPsB.P_Params->fontscale = pGp->Pgm.EndOfCommand() ? -1.0f : pGp->FloatExpression();
			    if(pGp->TPsB.P_Params->fontscale <= 0)
				    pGp->TPsB.P_Params->fontscale = 1.0;
			    break;
			case PS_FONTFILE: {
			    bool deleteentry = FALSE;
			    char * fontfilename = NULL;
			    pGp->Pgm.Shift();
			    if(pGp->TPsB.P_Params->terminal != PSTERM_POSTSCRIPT)
				    goto PS_options_error;
			    if(pGp->Pgm.EqualsCur("add"))
				    pGp->Pgm.Shift();
			    else if(pGp->Pgm.AlmostEqualsCur("del$ete")) {
				    deleteentry = TRUE;
				    pGp->Pgm.Shift();
			    }
			    if(!(fontfilename = pGp->TryToGetString())) {
				    pGp->IntErrorCurToken("Font filename expected");
			    }
			    else {
				    bool filename_doubled = FALSE;
				    ps_fontfile_def * curr_ps_fontfile = pGp->TPsB.P_Params->first_fontfile;
				    ps_fontfile_def * prev_ps_fontfile = NULL;
				    ps_fontfile_def * new_ps_fontfile = (struct ps_fontfile_def *)SAlloc::M(sizeof(struct ps_fontfile_def));
				    new_ps_fontfile->fontfile_name = (char *)SAlloc::M(pGp->Pgm.CurTokenLen());
				    pGp->GpExpandTilde(&fontfilename);
				    new_ps_fontfile->fontfile_name = fontfilename;
				    new_ps_fontfile->fontname = NULL;
				    if(!deleteentry) {
#if defined(PIPES)
					    if(*(new_ps_fontfile->fontfile_name) == '<') {
						    new_ps_fontfile->fontfile_fullname = NULL;
					    }
					    else
#endif
					    { /* New search order (version 5.3) */
						/* (1) try absolute path or current directory */
						/* (2) fontpath_fullname will also check loadpath directories */
						    new_ps_fontfile->fontfile_fullname = fontpath_fullname(pThis, fontfilename, NULL);
						    // (3) try in directory from "set fontpath" 
						    if(!new_ps_fontfile->fontfile_fullname && GPT.P_PS_FontPath)
							    new_ps_fontfile->fontfile_fullname = fontpath_fullname(pThis, fontfilename, GPT.P_PS_FontPath);
						    // (4) environmental variable GNUPLOT_FONTPATH 
							SETIFZ(new_ps_fontfile->fontfile_fullname, fontpath_fullname(pThis, fontfilename, getenv("GNUPLOT_FONTPATH")));
						    if(!new_ps_fontfile->fontfile_fullname)
							    pGp->IntError(pGp->Pgm.GetPrevTokenIdx(), "Font file '%s' not found.\nTry setting GNUPLOT_FONTPATH in the environment or adding a 'set fontpath' command.", fontfilename);
					    }
				    }
				    new_ps_fontfile->next = NULL;

				    if(!deleteentry) {
					    LFS * lf = pGp->P_LfHead;
					    if(lf) {
						    while(lf->prev)
							    lf = lf->prev;
					    }
					    if((lf && lf->interactive) || pGp->_Plt.interactive)
						    PS_load_fontfile(pThis, new_ps_fontfile, FALSE);
				    }
				    if(pGp->TPsB.P_Params->first_fontfile) {
					    while(curr_ps_fontfile) {
						    if(strcmp(curr_ps_fontfile->fontfile_name, new_ps_fontfile->fontfile_name) == 0) {
							    filename_doubled = TRUE;
							    if(deleteentry) {
								    delete_ps_fontfile(pThis, prev_ps_fontfile, curr_ps_fontfile);
								    curr_ps_fontfile = NULL;
								    break;
							    }
						    }
						    prev_ps_fontfile = curr_ps_fontfile;
						    curr_ps_fontfile = curr_ps_fontfile->next;
					    }
					    if(!filename_doubled) {
						    if(!deleteentry)
							    prev_ps_fontfile->next = new_ps_fontfile;
						    else
							    pGp->IntWarn(pGp->Pgm.GetPrevTokenIdx(), "Can't delete Font filename '%s'",
								new_ps_fontfile->fontfile_name);
					    }
				    }
				    else {
					    if(!deleteentry)
						    pGp->TPsB.P_Params->first_fontfile = new_ps_fontfile;
					    else
						    pGp->IntWarn(pGp->Pgm.GetPrevTokenIdx(), "Can't delete Font filename '%s'", new_ps_fontfile->fontfile_name);
				    }
			    }
			    break;
		    }
			case PS_NOFONTFILES:
			    if(pGp->TPsB.P_Params->terminal != PSTERM_POSTSCRIPT)
				    goto PS_options_error;
			    while(pGp->TPsB.P_Params->first_fontfile != NULL)
				    delete_ps_fontfile(pThis, 0, pGp->TPsB.P_Params->first_fontfile);
			    pGp->Pgm.Shift();
			    break;
			case PS_ADOBEGLYPHNAMES:
			    pGp->TPsB.P_Params->adobeglyphnames = TRUE;
			    pGp->Pgm.Shift();
			    break;
			case PS_NOADOBEGLYPHNAMES:
			    pGp->TPsB.P_Params->adobeglyphnames = FALSE;
			    pGp->Pgm.Shift();
			    break;
			case PS_BACKGROUND: {
			    int ps_background;
			    pGp->Pgm.Shift();
			    ps_background = pGp->ParseColorName();
			    pGp->TPsB.P_Params->background.r = ((ps_background >> 16) & 0xff) / 255.0;
			    pGp->TPsB.P_Params->background.g = ((ps_background >> 8) & 0xff) / 255.0;
			    pGp->TPsB.P_Params->background.b = (ps_background & 0xff) / 255.0;
			    break;
		    }
			case PS_NOBACKGROUND:
			    pGp->Pgm.Shift();
			    pGp->TPsB.P_Params->background.r = pGp->TPsB.P_Params->background.g = pGp->TPsB.P_Params->background.b = -1.0;
			    break;
			case PS_PALFUNCPARAM:
			    if(set_palfunc)
				    goto PS_options_error;
			    set_palfunc = TRUE;
			    pGp->Pgm.Shift();
			    pGp->TPsB.P_Params->palfunc_samples = pGp->IntExpression();
			    if(pGp->TPsB.P_Params->palfunc_samples < 2)
				    pGp->TPsB.P_Params->palfunc_samples = 2;
			    if(!pGp->Pgm.EndOfCommand() && pGp->Pgm.EqualsCur(",")) {
				    pGp->Pgm.Shift();
				    pGp->TPsB.P_Params->palfunc_deviation = fabs(pGp->RealExpression());
				    if(pGp->TPsB.P_Params->palfunc_deviation >= 1)
					    pGp->IntError(pGp->Pgm.GetPrevTokenIdx(), "allowed deviation must be < 1");
			    }
			    break;

			case PS_SIZE:
		    {
			    float xmax_t, ymax_t;
			    pGp->Pgm.Shift();
			    ps_explicit_size = TRUE;
			    ps_explicit_units = pGp->ParseTermSize(&xmax_t, &ymax_t, INCHES);
			    // PostScript *always* works in pts, not locally defined dpi 
			    pThis->SetMax(static_cast<uint>(xmax_t * PS_SC * 72.0/GpResolution), static_cast<uint>(ymax_t * PS_SC * 72.0/GpResolution));
			    eps_explicit_x = 2 * pThis->MaxX;
			    eps_explicit_y = 2 * pThis->MaxY;
			    break;
		    }
			case PS_FONT:
			    pGp->Pgm.Shift();
			// @fallthrough to attempt to read font name 
			case PS_OTHER:
			default:
			    if((s = pGp->TryToGetString())) {
				    if(set_font)
					    goto PS_options_error;
				    set_font = TRUE;
				    if((pGp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT) ||
					(pGp->TPsB.P_Params->terminal == PSTERM_EPSLATEX)) {
					    char * comma = strrchr(s, ',');
					    if(comma && (1 == sscanf(comma+1, "%f", &pGp->TPsB.P_Params->fontsize))) {
						    set_fontsize = TRUE;
						    *comma = '\0';
					    }
					    if(*s) {
						    /* Filter out characters that would confuse PostScript */
						    if(strpbrk(s, "()[]{}| ")) {
							    pGp->IntWarn(pGp->Pgm.GetPrevTokenIdx(), "Illegal characters in PostScript font name.");
							    pGp->IntWarn(NO_CARET, "I will try to fix it but this may not work.");
							    while(strpbrk(s, "()[]{}| "))
								    *(strpbrk(s, "()[]{}| ")) = '-';
						    }
						    strnzcpy(pGp->TPsB.P_Params->font, s, sizeof(pGp->TPsB.P_Params->font));
					    }
					    SAlloc::F(s);
				    }
				    else
					    pGp->IntError(pGp->Pgm.GetPrevTokenIdx(), "terminal %s does not allow specification %s", pThis->name, "of font name");
			    }
			    else {
				    if(set_fontsize)
					    goto PS_options_error;
				    set_fontsize = TRUE;
				    // We have font size specified 
				    pGp->TPsB.P_Params->fontsize = pGp->FloatExpression();
				    if(pGp->TPsB.P_Params->fontsize <= 0)
					    pGp->TPsB.P_Params->fontsize = 14;
			    }
			    break;
		}
	}
	/* Sanity checks on fontsize */
	if(pGp->TPsB.P_Params->fontsize > 1000.)
		pGp->TPsB.P_Params->fontsize = 100.0;
	if(pGp->TPsB.P_Params->fontsize <= 0)
		pGp->TPsB.P_Params->fontsize = 14.0;
	switch(pGp->TPsB.P_Params->terminal) {
		case PSTERM_POSTSCRIPT:
		    pGp->TPsB.FontSize = pGp->TPsB.P_Params->fontsize;
		    break;
		case PSTERM_EPSLATEX:
		    pGp->TPsB.FontSize = 2 * pGp->TPsB.P_Params->fontsize;
		    break;
		case PSTERM_PSLATEX:
		case PSTERM_PSTEX:
		    if(pGp->TPsB.P_Params->fontsize > 0)
			    pGp->TPsB.FontSize = 2 * pGp->TPsB.P_Params->fontsize;
		    else
			    pGp->TPsB.FontSize = 20; /* default: 10pt */
		    break;
	}
	pThis->CV() = (uint)(pGp->TPsB.FontSize * _ps_scf(pGp));
	if(pGp->TPsB.P_Params->oldstyle)
		pThis->CH() = (uint)(pGp->TPsB.FontSize * _ps_scf(pGp) * 5/10);
	else
		pThis->CH() = (uint)(pGp->TPsB.FontSize * _ps_scf(pGp) * 6/10);
	snprintf(PS_default_font, sizeof(PS_default_font)-1, "%s, %.2g", pGp->TPsB.P_Params->font, pGp->TPsB.FontSize);
	if(pGp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT) {
		if(pGp->TPsB.P_Params->first_fontfile) {
			ps_fontfile_def * curr_ps_fontfile = pGp->TPsB.P_Params->first_fontfile;
			uint totlength = 0;
			char * running;
			while(curr_ps_fontfile) {
				totlength += strlen(curr_ps_fontfile->fontfile_name) + strlen(" fontfile \"\"");
				curr_ps_fontfile = curr_ps_fontfile->next;
			}
			curr_ps_fontfile = pGp->TPsB.P_Params->first_fontfile;
			ps_fontfile_char = (char *)SAlloc::M(totlength+1);
			running = ps_fontfile_char;
			while(curr_ps_fontfile) {
				sprintf(running, " fontfile \"%s\"", curr_ps_fontfile->fontfile_name);
				running += strlen(running);
				curr_ps_fontfile = curr_ps_fontfile->next;
			}
		}
	}
	// HBB 19990823: fixed the options string. It violated the 'save loadable output' rule 
	if(pGp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT)
		slprintf(GPT._TermOptions, "%s %s %s \\\n", pGp->TPsB.P_Params->psformat==PSTERM_EPS ? "eps" : (pGp->TPsB.P_Params->psformat==PSTERM_PORTRAIT ? "portrait" : "landscape"),
		    pThis->put_text == ENHPS_put_text ? "enhanced" : "noenhanced", pGp->TPsB.P_Params->duplex_option ? (pGp->TPsB.P_Params->duplex_state ? "duplex" : "simplex") : "defaultplex");
	else if(pGp->TPsB.P_Params->terminal != PSTERM_EPSLATEX)
		slprintf(GPT._TermOptions, "%s%s", pGp->TPsB.P_Params->rotate ? "rotate" : "norotate", pGp->TPsB.P_Params->useauxfile ? " auxfile" : "");
	else
		GPT._TermOptions.Z();
	sprintf(tmp_term_options, "   %s %s %s \\\n\
   dashlength %.1f linewidth %.1f pointscale %.1f %s %s \\\n",
	    pGp->TPsB.P_Params->level1 ? "level1" : (pGp->TPsB.P_Params->level3 ? "level3" : "leveldefault"),
	    pGp->TPsB.P_Params->color ? "color" : "monochrome",
	    pGp->TPsB.P_Params->blacktext ? "blacktext" : "colortext",
	    pGp->TPsB.P_Params->dash_length,
	    pGp->TPsB.P_Params->linewidth_factor,
	    pGp->TPsB.P_Params->pointscale_factor,
	    pGp->TPsB.P_Params->rounded ? "rounded" : "butt",
	    pGp->TPsB.P_Params->clipped ? "clip" : "noclip");
	GPT._TermOptions.Cat(tmp_term_options);
	if(pGp->TPsB.P_Params->background.r >= 0) {
		sprintf(tmp_term_options, "   background \"#%02x%02x%02x\" \\\n",
		    (int)(255 * pGp->TPsB.P_Params->background.r),
		    (int)(255 * pGp->TPsB.P_Params->background.g),
		    (int)(255 * pGp->TPsB.P_Params->background.b));
		GPT._TermOptions.Cat(tmp_term_options);
	}
	else {
		GPT._TermOptions.Cat("   nobackground \\\n");
	}
	sprintf(tmp_term_options, "   palfuncparam %d,%g \\\n   ", pGp->TPsB.P_Params->palfunc_samples, pGp->TPsB.P_Params->palfunc_deviation);
	GPT._TermOptions.Cat(tmp_term_options);
#ifdef PSLATEX_DRIVER
	if((pGp->TPsB.P_Params->terminal == PSTERM_PSTEX) ||
	    (pGp->TPsB.P_Params->terminal == PSTERM_PSLATEX)) {
		sprintf(tmp_term_options, "%s %s ", pGp->TPsB.P_Params->rotate ? "rotate" : "norotate", pGp->TPsB.P_Params->useauxfile ? "auxfile" : "noauxfile");
		GPT._TermOptions.Cat(tmp_term_options);
	}
	if(pGp->TPsB.P_Params->terminal == PSTERM_EPSLATEX) {
		sprintf(tmp_term_options, "%s ", pGp->TPsB.P_Params->epslatex_standalone ? "standalone" : "input");
		GPT._TermOptions.Cat(tmp_term_options);
	}
#endif
	if((GPT._Encoding == S_ENC_UTF8) && (pGp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT)) {
		sprintf(tmp_term_options, " %sadobeglyphnames \\\n   ", pGp->TPsB.P_Params->adobeglyphnames ? "" : "no");
		GPT._TermOptions.Cat(tmp_term_options);
	}
	if(ps_explicit_size) {
		if(ps_explicit_units == CM)
			sprintf(tmp_term_options, "size %.2fcm, %.2fcm ", 2.54*(float)pThis->MaxX/(72.*PS_SC), 2.54*(float)pThis->MaxY/(72.*PS_SC));
		else
			sprintf(tmp_term_options, "size %.2fin, %.2fin ", (float)pThis->MaxX/(72.*PS_SC), (float)pThis->MaxY/(72.*PS_SC));
		GPT._TermOptions.Cat(tmp_term_options);
	}
	if(pGp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT)
		sprintf(tmp_term_options, "\"%s\" %g%s ", pGp->TPsB.P_Params->font, pGp->TPsB.P_Params->fontsize, ps_fontfile_char ? ps_fontfile_char : "");
	else if(pGp->TPsB.P_Params->terminal == PSTERM_EPSLATEX)
		sprintf(tmp_term_options, "\"%s\" %g ", pGp->TPsB.P_Params->font, pGp->TPsB.P_Params->fontsize);
	else if(pGp->TPsB.P_Params->fontsize)
		sprintf(tmp_term_options, "%g ", pGp->TPsB.P_Params->fontsize);
	else
		tmp_term_options[0] = '\0';
	SAlloc::F(ps_fontfile_char);
	GPT._TermOptions.Cat(tmp_term_options);
	sprintf(tmp_term_options, " fontscale %3.1f ", pGp->TPsB.P_Params->fontscale);
	GPT._TermOptions.Cat(tmp_term_options);
	return;
PS_options_error:
	pGp->IntErrorCurToken("extraneous argument in set terminal %s", pThis->name);
	return;
}
//
// store settings passed to common_init() for use in PS_graphics()
// GnuPlot::TPsB.P_Params->psformat, etc are reserved for storing the term options
//
static bool ps_common_uses_fonts;
static uint ps_common_xoff, ps_common_yoff;

/* The default UTF8 code will use glyph identifiers uniXXXX for all glyphs above 0x0100.
 * If you define ADOBE_ENCODING_NAMES, then it will instead use the glyph names from the
 * file aglfn.txt.  Names in the range 0x0100 - 0x01FF correspond to those used by the
 * Latin1 encoding scheme.  This unicode code page deliberately uses the same character
 * mapping as Latin1.  Adobe also recommends names for many characters outside this
 * range, but not all fonts adhere to this.  You can substitute a different aglfn.txt
 * file at run time if you want to use a different scheme.
 */
#define ADOBE_ENCODING_NAMES 1

#if (ADOBE_ENCODING_NAMES)

typedef struct ps_glyph {
	ulong unicode;
	char * glyphname;
} ps_glyph;

static ps_glyph * aglist = NULL; // @global
static int aglist_alloc = 0; // @global
static int aglist_size = 0; // @global
static int psglyphs = 0; // @global

#endif

TERM_PUBLIC void PS_load_fontfile(GpTermEntry_Static * pThis, ps_fontfile_def * current_ps_fontfile, bool doload)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(current_ps_fontfile) {
		uint linesread = 0;
		FILE * ffont = NULL;
		char line[256];
		char ext[4];
		char cmd[256];
		char * fontname = NULL;
		int i;
#if defined(PIPES)
		char * envcmd = NULL;
		bool ispipe = FALSE;
#endif
		ext[0] = '\0';
		cmd[0] = '\0';
		if(doload)
			fprintf(GPT.P_GpPsFile, "%%%%BeginProcSet: %s\n", current_ps_fontfile->fontfile_name);
		// get filename extension if no pipe (if pipe *ext=='\0') 
#if defined(PIPES)
		if(*(current_ps_fontfile->fontfile_name) != '<') {
			/* Filename is given */
#endif
		if(strlen(current_ps_fontfile->fontfile_name) > 3)
			strcpy(ext, current_ps_fontfile->fontfile_name + strlen(current_ps_fontfile->fontfile_name) - 3);
		else
			strcpy(ext, current_ps_fontfile->fontfile_name);
		// make extension lowercase for comparison 
		for(i = 0; i<3; i++)
			ext[i] = tolower((uchar)ext[i]);
		if(!current_ps_fontfile->fontfile_fullname)
			p_gp->IntError(NO_CARET, "Font file '%s' not found", current_ps_fontfile->fontfile_name);
#if defined(PIPES)
	}
#endif
		if(isempty(ext)) {
#if defined(PIPES)
			// Pipe is given 
			p_gp->RestrictPOpen();
			ispipe = TRUE;
			strcpy(cmd, current_ps_fontfile->fontfile_name + 1);
			ffont = popen(cmd, "r");
			if(!ffont)
				p_gp->IntError(NO_CARET, "Could not execute pipe '%s'", current_ps_fontfile->fontfile_name + 1);
#endif
		}
		else if(sstreq(ext, "ttf") || sstreq(ext, "otf")) {
			// TrueType 
#if defined(PIPES)
			p_gp->RestrictPOpen();
			ispipe = TRUE;
			envcmd = getenv("GNUPLOT_TTFTOPFA");
			if(envcmd != NULL)
				sprintf(cmd, envcmd, current_ps_fontfile->fontfile_fullname);
			else
				sprintf(cmd, "ttf2pt1 -a -e -W 0 %s -", current_ps_fontfile->fontfile_fullname);
			if(isempty(cmd))
				p_gp->IntError(NO_CARET, "No command for automatic font conversion ttf->pfa defined");
			else {
				ffont = popen(cmd, "r");
				if(!ffont)
					p_gp->IntError(NO_CARET, "Could not execute command '%s'", cmd);
			}
#else
			p_gp->OsError(NO_CARET, "Automatic font conversion ttf->pfa not supported");
#endif
		}
		else if(strcmp(ext, "pfb") == 0) {
			/* PFB */
#if defined(PIPES)
			p_gp->RestrictPOpen();
			ispipe = TRUE;
			envcmd = getenv("GNUPLOT_PFBTOPFA");
			if(envcmd != NULL)
				sprintf(cmd, envcmd, current_ps_fontfile->fontfile_fullname);
			else
				sprintf(cmd, "pfbtops %s", current_ps_fontfile->fontfile_fullname);
			if(isempty(cmd))
				p_gp->IntError(NO_CARET, "No command for automatic font conversion pfb->pfa defined");
			else {
				ffont = popen(cmd, "r");
				if(!ffont)
					p_gp->IntError(NO_CARET, "Could not execute command '%s'", cmd);
			}
#else
			p_gp->OsError(NO_CARET, "Automatic font conversion pfb->pfa not supported");
#endif
		}
		else {
			/* PFA */
			if(!sstreq(ext, "pfa"))
				p_gp->IntWarn(NO_CARET, "Font file '%s' has unknown extension. Assume it is a pfa file", current_ps_fontfile->fontfile_name);
			ffont = p_gp->LoadPath_fopen(current_ps_fontfile->fontfile_fullname, "r");
			if(!ffont)
				p_gp->IntError(NO_CARET, "Font file '%s' not found", current_ps_fontfile->fontfile_name);
		}
		/* read the file */
		while(fgets(line, 255, ffont)) {
			/* test file format */
			if((linesread == 0) && (strstr(line, "%!PS-AdobeFont") != line) && (strstr(line, "%!FontType1") != line)) {
#if defined(PIPES)
				if(ispipe)
					p_gp->IntWarn(NO_CARET, "Command '%s' seems not to generate PFA data", cmd);
				else
#endif
				p_gp->IntWarn(NO_CARET, "Font file '%s' seems not to be a PFA file", current_ps_fontfile->fontfile_name);
			}
			/* get fontname */
			if(strstr(line, "/FontName") == line) {
				char * fnende = NULL;
				fontname = (char *)SAlloc::M(strlen(line)-9);
				strcpy(fontname, strstr(line+1, "/")+1);
				fnende = strstr(fontname, " ");
				*fnende = '\0';
				current_ps_fontfile->fontname = sstrdup(fontname);
				/* Print font name */
				if(!doload) {
					if(current_ps_fontfile->fontfile_fullname)
						fprintf(stderr, "Font file '%s' contains the font '%s'. Location:\n   %s\n", current_ps_fontfile->fontfile_name, 
							fontname, current_ps_fontfile->fontfile_fullname);
					else
						fprintf(stderr, "Pipe '%s' contains the font '%s'.\n", current_ps_fontfile->fontfile_name, fontname);
#if defined(PIPES)
					/* Stop reading font file in order to save time */
					/* This does not work for pipes because they give the */
					/* error message 'broken pipe' */
					if(!ispipe)
#endif
					break;
				}
			}

			if(doload)
				fputs(line, GPT.P_GpPsFile);
			++linesread;
		}
#if defined(PIPES)
		if(ispipe) {
			int exitcode;
			if((exitcode = pclose(ffont)) != 0)
				p_gp->IntError(NO_CARET, "Command '%s' generated error, exitcode is %d", cmd, exitcode);
		}
		else
#endif
		fclose(ffont);
		if(linesread == 0) {
#if defined(PIPES)
			if(ispipe)
				p_gp->IntError(NO_CARET, "Command '%s' generates empty output", cmd);
			else
#endif
			p_gp->IntError(NO_CARET, "Font file '%s' is empty", current_ps_fontfile->fontfile_name);
		}
		if(doload)
			fputs("%%EndProcSet\n", GPT.P_GpPsFile);
		/* Computer Modern Symbol font with corrected baseline if the
		 * font CMEX10 is embedded */
		if(doload && fontname && (strcmp(fontname, "CMEX10") == 0)) {
			fputs("%%BeginProcSet: CMEX10-Baseline\n", GPT.P_GpPsFile);
			fputs("/CMEX10-Baseline /CMEX10 findfont [1 0 0 1 0 1] makefont\n", GPT.P_GpPsFile);
			fputs("dup length dict begin {1 index /FID eq {pop pop} {def} ifelse} forall\n", GPT.P_GpPsFile);
			fputs("currentdict end definefont pop\n", GPT.P_GpPsFile);
			fputs("%%EndProcSet\n", GPT.P_GpPsFile);
		}
		ZFREE(fontname);
	}
}

TERM_PUBLIC void PS_load_fontfiles(GpTermEntry_Static * pThis, bool doload)
{
	ps_fontfile_def * current_ps_fontfile = pThis->P_Gp->TPsB.P_Params->first_fontfile;
	while(current_ps_fontfile) {
		PS_load_fontfile(pThis, current_ps_fontfile, doload);
		if(current_ps_fontfile->fontname)
			PS_RememberFont(pThis, current_ps_fontfile->fontname);
		current_ps_fontfile = current_ps_fontfile->next;
	}
}

TERM_PUBLIC void PS_common_init(GpTermEntry_Static * pThis, bool uses_fonts/* FALSE for (e)ps(la)tex */, uint xoff, uint yoff/* how much to translate by */,
    uint bb_xmin, uint bb_ymin, uint bb_xmax, uint bb_ymax/* bounding box */, const char ** dict/* extra entries for the dictionary */)
{
	GnuPlot * p_gp = pThis->P_Gp;
	static const char psi1[] = "\
%%%%Creator: gnuplot %s patchlevel %s\n\
%%%%CreationDate: %s\n\
%%%%DocumentFonts: %s\n";

	static const char psi2[] =
	    "\
%%%%EndComments\n\
%%%%BeginProlog\n\
/gnudict 256 dict def\ngnudict begin\n\
%%\n\
%% The following true/false flags may be edited by hand if desired.\n\
%% The unit line width and grayscale image gamma correction may also be changed.\n\
%%\n\
/Color %s def\n\
/Blacktext %s def\n\
/Solid %s def\n\
/Dashlength %g def\n\
/Landscape %s def\n\
/Level1 %s def\n\
/Level3 %s def\n\
/Rounded %s def\n\
/ClipToBoundingBox %s def\n\
/SuppressPDFMark false def\n\
/TransparentPatterns false def\n\
/gnulinewidth %.3f def\n\
/userlinewidth gnulinewidth def\n\
/Gamma 1.0 def\n\
/BackgroundColor {%.3f %.3f %.3f} def\n\
%%\n\
/vshift %d def\n\
/dl1 {\n\
  %.1f Dashlength userlinewidth gnulinewidth div mul mul mul\n\
  Rounded { currentlinewidth 0.75 mul sub dup 0 le { pop 0.01 } if } if\n\
} def\n\
/dl2 {\n\
  %.1f Dashlength userlinewidth gnulinewidth div mul mul mul\n\
  Rounded { currentlinewidth 0.75 mul add } if\n\
} def\n\
/hpt_ %.1f def\n\
/vpt_ %.1f def\n\
/hpt hpt_ def\n\
/vpt vpt_ def\n";

	static const char psi3[] =
	    "\
Level1 SuppressPDFMark or \n\
{} {\n\
/SDict 10 dict def\n\
systemdict /pdfmark known not {\n\
  userdict /pdfmark systemdict /cleartomark get put\n\
} if\n\
SDict begin [\n\
  /Title (%s)\n\
  /Subject (gnuplot plot)\n\
  /Creator (gnuplot %s patchlevel %s)\n\
%%  /Producer (gnuplot)\n\
%%  /Keywords ()\n\
  /CreationDate (%s)\n\
  /DOCINFO pdfmark\n\
end\n\
} ifelse\n";
	//struct GpTermEntry * t = term;
	int i;
	time_t now;
	char * timedate;
	ps_common_uses_fonts = uses_fonts;
	ps_common_xoff = xoff;
	ps_common_yoff = yoff;
	p_gp->TPsB.Page = 0;
	time(&now);
	timedate = asctime(localtime(&now));
	timedate[strlen(timedate)-1] = '\0';

#ifdef PSLATEX_DRIVER
	// Set files for (e)ps(la)tex terminals 
	switch(p_gp->TPsB.P_Params->terminal) {
		case PSTERM_EPSLATEX:
		    EPSLATEX_common_init(pThis);
		    break;
		case PSTERM_PSLATEX:
		case PSTERM_PSTEX:
		    PSTEX_common_init(pThis);
		    break;
		default:; // do nothing, just avoid a compiler warning 
	}
#endif
	if(p_gp->TPsB.P_Params->psformat == PSTERM_EPS)
		fputs("%!PS-Adobe-2.0 EPSF-2.0\n", GPT.P_GpPsFile);
	else
		fputs("%!PS-Adobe-2.0\n", GPT.P_GpPsFile);
	if(GPT.P_OutStr)
		fprintf(GPT.P_GpPsFile, "%%%%Title: %s\n", GPT.P_OutStr); // JFi
	fprintf(GPT.P_GpPsFile, psi1, gnuplot_version, gnuplot_patchlevel, timedate, uses_fonts ? "(atend)" : "");
	fprintf(GPT.P_GpPsFile, "%%%%BoundingBox: %d %d %d %d\n", xoff + bb_xmin, yoff + bb_ymin, xoff + bb_xmax, yoff + bb_ymax);
	if((p_gp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT) && (p_gp->TPsB.P_Params->psformat != PSTERM_EPS))
		fprintf(GPT.P_GpPsFile, "%%%%Orientation: %s\n", p_gp->TPsB.P_Params->psformat == PSTERM_LANDSCAPE ? "Landscape" : "Portrait");
	if(p_gp->TPsB.P_Params->psformat != PSTERM_EPS)
		fputs("%%Pages: (atend)\n", GPT.P_GpPsFile);
	fprintf(GPT.P_GpPsFile, psi2,
	    p_gp->TPsB.P_Params->color ? "true" : "false",
	    p_gp->TPsB.P_Params->blacktext ? "true" : "false",
	    p_gp->TPsB.P_Params->solid ? "true" : "false",
	    p_gp->TPsB.P_Params->dash_length,     /* dash length */
	    p_gp->TPsB.P_Params->psformat == PSTERM_LANDSCAPE ? "true" : "false",
	    p_gp->TPsB.P_Params->level1 ? "true" : "false",
	    p_gp->TPsB.P_Params->level3 ? "true" : "false",
	    p_gp->TPsB.P_Params->rounded ? "true" : "false",
	    p_gp->TPsB.P_Params->clipped ? "true" : "false",
	    PS_LW*p_gp->TPsB.P_Params->linewidth_factor,  /* line width */
	    p_gp->TPsB.P_Params->background.r,
	    p_gp->TPsB.P_Params->background.g,
	    p_gp->TPsB.P_Params->background.b, /* background color, used only if all components >= 0 */
	    (int)(pThis->CV())/(-3),      /* shift for vertical centring */
	    PS_SC*1.0,  /* dash length */
	    PS_SC*1.0,  /* dash length */
	    PS_HTIC/2.0,                /* half point width */
	    PS_VTIC/2.0); /* half point height */

	/* HH: Clip to BoundingBox if the corresponding flag is toggled */
	fprintf(GPT.P_GpPsFile,
	    "\
/doclip {\n\
  ClipToBoundingBox {\n\
    newpath %d %d moveto %d %d lineto %d %d lineto %d %d lineto closepath\n\
    clip\n\
  } if\n\
} def\n",
	    xoff + bb_xmin, yoff + bb_ymin, xoff + bb_xmax, yoff + bb_ymin, xoff + bb_xmax, yoff + bb_ymax, xoff + bb_xmin, yoff + bb_ymax);
	// Dump the body of the prologue 
	PS_dump_prologue_file(pThis, "prologue.ps");
	// insert font encoding vector 
	if(uses_fonts) {
		switch(GPT._Encoding) {
			case S_ENC_ISO8859_1:   PS_dump_prologue_file(pThis, "8859-1.ps"); break;
			case S_ENC_ISO8859_2:   PS_dump_prologue_file(pThis, "8859-2.ps"); break;
			case S_ENC_CP1254:
			case S_ENC_ISO8859_9:   PS_dump_prologue_file(pThis, "8859-9.ps"); break;
			case S_ENC_ISO8859_15:  PS_dump_prologue_file(pThis, "8859-15.ps"); break;
			case S_ENC_CP437:       PS_dump_prologue_file(pThis, "cp437.ps"); break;
			case S_ENC_CP850:       PS_dump_prologue_file(pThis, "cp850.ps"); break;
			case S_ENC_CP852:       PS_dump_prologue_file(pThis, "cp852.ps"); break;
			case S_ENC_CP1250:      PS_dump_prologue_file(pThis, "cp1250.ps"); break;
			case S_ENC_CP1251:      PS_dump_prologue_file(pThis, "cp1251.ps"); break;
			case S_ENC_CP1252:      PS_dump_prologue_file(pThis, "cp1252.ps"); break;
			case S_ENC_KOI8_R:      PS_dump_prologue_file(pThis, "koi8r.ps"); break;
			case S_ENC_KOI8_U:      PS_dump_prologue_file(pThis, "koi8u.ps"); break;
			case S_ENC_UTF8:        PS_dump_prologue_file(pThis, "utf-8.ps");
			    if(!aglist) 
					PS_load_glyphlist(pThis);
			    break;
			case S_ENC_DEFAULT:
			default:                break;
		}
	}
	// Redefine old epslatex linetypes if requested 
	if((p_gp->TPsB.P_Params->terminal == PSTERM_EPSLATEX) && p_gp->TPsB.P_Params->oldstyle) {
		for(i = 0; OldEPSL_linetypes[i] != NULL; i++)
			fprintf(GPT.P_GpPsFile, "%s", OldEPSL_linetypes[i]);
	}
	/* The use of statusdict and setduplexmode is not 'Standard'  */
	/* PostScript.  This method is used in Level 1 as a means of  */
	/* controlling device properties, and is device specific.     */
	/* Level 2/3 PostScript performs these functions via          */
	/* setpagedevice.  See PostScript Language Reference 3rd Ed.  */
	/* pages 426-32 and 679-80 for details.  The code below just  */
	/* makes the passing of _simplex_ work across levels 1-3 and  */
	/* the matter of Duplex and Tumble is left to others.         */
	/* Be aware that BRscript3 is not happy if presented with a   */
	/* single page and Duplex is forced on, whereas the Xerox     */
	/* Phaser 8560 sees there is only one page and uses simplex.  */

	if(p_gp->TPsB.P_Params->duplex_option) {
		if(p_gp->TPsB.P_Params->level1)
			fprintf(GPT.P_GpPsFile, "statusdict begin %s setduplexmode end\n", p_gp->TPsB.P_Params->duplex_state ? "true" : "false");
		else if(!p_gp->TPsB.P_Params->duplex_state) 
			fprintf(GPT.P_GpPsFile, "%%%%BeginFeature: *Duplex Simplex\n << /Duplex false >> setpagedevice\n%%%%EndFeature\n");
	}
	if(dict)
		while(*dict)
			fputs(*(dict++), GPT.P_GpPsFile);
	if(uses_fonts) {
		PS_load_fontfiles(pThis, TRUE);
		PS_RememberFont(pThis, p_gp->TPsB.P_Params->font);
	}
	/* This pdfmark has triggered many complaints and bug reports */
	/* because it causes epslatex (but not pdflatex) to overwrite */
	/* the actual TeX document title.  We can argue that that is  */
	/* an epslatex bug rather than a gnuplot bug, but people still*/
	/* complain.  I have created a flag SuppressPDFMark in the    */
	/* postscript prolog file that users can toggle to disable it.*/
	// HH: print pdf information interpreted by ghostscript/acrobat
	{
		char * outstr2 = PS_escape_string(GPT.P_OutStr, "()\\");
		fprintf(GPT.P_GpPsFile, psi3, outstr2 ? outstr2 : "", gnuplot_version, gnuplot_patchlevel, timedate);
		SAlloc::F(outstr2);
	}
	// Define macros supporting boxed text 
	fprintf(GPT.P_GpPsFile, psi4);
	fputs("end\n", GPT.P_GpPsFile);
	fputs("%%EndProlog\n", GPT.P_GpPsFile);
}
//
// the init fn for the postscript driver 
//
void PS_init(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	uint xmin_t = 0, ymin_t = 0, xmax_t = 0, ymax_t = 0;
	switch(p_gp->TPsB.P_Params->psformat) {
		case PSTERM_EPS:
		    if(ps_explicit_size) {
			    pThis->SetMax(eps_explicit_x, eps_explicit_y);
		    }
		    else {
			    pThis->SetMax(PS_XMAX, p_gp->TPsB.P_Params->oldstyle ? PS_YMAX_OLDSTYLE : PS_YMAX);
		    }
		    xmin_t = static_cast<uint>(pThis->MaxX * p_gp->V.Offset.x / (2*PS_SC));
		    xmax_t = static_cast<uint>(pThis->MaxX * (p_gp->V.Size.x + p_gp->V.Offset.x) / (2*PS_SC));
		    ymin_t = static_cast<uint>(pThis->MaxY * p_gp->V.Offset.y / (2*PS_SC));
		    ymax_t = static_cast<uint>(pThis->MaxY * (p_gp->V.Size.y + p_gp->V.Offset.y) / (2*PS_SC));
		    pThis->tscale = PS_SC * 2;
		    break;
		case PSTERM_PORTRAIT:
		    if(!ps_explicit_size) {
			    pThis->SetMax(PS_YMAX, PS_XMAX);
		    }
		    xmin_t = static_cast<uint>(pThis->MaxX * p_gp->V.Offset.x / PS_SC);
		    xmax_t = static_cast<uint>(pThis->MaxX * (p_gp->V.Size.x + p_gp->V.Offset.x) / PS_SC);
		    ymin_t = static_cast<uint>(pThis->MaxY * p_gp->V.Offset.y / PS_SC);
		    ymax_t = static_cast<uint>(pThis->MaxY * (p_gp->V.Size.y + p_gp->V.Offset.y) / PS_SC);
		    pThis->tscale = PS_SC;
		    break;
		case PSTERM_LANDSCAPE:
		    if(!ps_explicit_size) {
			    pThis->SetMax(PS_XMAX, PS_YMAX);
		    }
		    ymin_t = static_cast<uint>(pThis->MaxX * p_gp->V.Offset.x / PS_SC);
		    ymax_t = static_cast<uint>(pThis->MaxX * (p_gp->V.Size.x+p_gp->V.Offset.x) / PS_SC);
		    xmin_t = static_cast<uint>(pThis->MaxY * (1-p_gp->V.Size.y-p_gp->V.Offset.y) / PS_SC);
		    xmax_t = static_cast<uint>(pThis->MaxY * (1-p_gp->V.Offset.y) / PS_SC);
		    pThis->tscale = PS_SC;
		    break;
		default:
		    p_gp->IntError(NO_CARET, "invalid postscript format used");
	}
	// for enhanced postscript, copy p_gp->TPsB.P_Params->font to ps_enh_font
	// does no harm for non-enhanced
	strcpy(p_gp->TPsB.EnhFont, p_gp->TPsB.P_Params->font);
	p_gp->TPsB.EnhFontSize = p_gp->TPsB.FontSize;
	switch(p_gp->TPsB.P_Params->terminal) {
		case PSTERM_POSTSCRIPT:
		    GPT.P_GpPsFile = GPT.P_GpOutFile;
		    break;
		default:
#ifdef PSLATEX_DRIVER
		    PSTEX_reopen_output(pThis);
		    break;
		case PSTERM_EPSLATEX:
		    EPSLATEX_reopen_output(pThis, "eps");
#endif
		    break;
	}
	PS_common_init(pThis, p_gp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT, p_gp->TPsB.P_Params->xoff, p_gp->TPsB.P_Params->yoff,
	    xmin_t, ymin_t, xmax_t, ymax_t, (pThis->put_text == ENHPS_put_text) ? ENHPS_header : NULL);
	// Keep track of whether we have written the enhanced text dictionary yet 
	p_gp->TPsB.EnsPsInitialized = (pThis->put_text == ENHPS_put_text) ? 2 : 1;
}

void PS_graphics(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	//struct GpTermEntry * t = term;
	p_gp->TPsB.Page++;
	// if (p_gp->TPsB.P_Params->psformat != PSTERM_EPS) 
	fprintf(GPT.P_GpPsFile, "%%%%Page: %d %d\n", p_gp->TPsB.Page, p_gp->TPsB.Page);
	// If we are about to use enhanced text mode for the first time in a plot that 
	// was initialized previously without it, we need to write out the macros now 
	if(pThis->put_text == ENHPS_put_text && p_gp->TPsB.EnsPsInitialized == 1) {
		const char ** dict = ENHPS_header;
		while(*dict)
			fputs(*(dict++), GPT.P_GpPsFile);
		fprintf(stderr, "Writing out PostScript macros for enhanced text mode\n");
		p_gp->TPsB.EnsPsInitialized = 2;
	}
	fprintf(GPT.P_GpPsFile, "\
gnudict begin\ngsave\n\
doclip\n\
%d %d translate\n\
%.3f %.3f scale\n",
	    ps_common_xoff, ps_common_yoff,
	    (p_gp->TPsB.P_Params->psformat == PSTERM_EPS ? 0.5 : 1.0)/PS_SC,
	    (p_gp->TPsB.P_Params->psformat == PSTERM_EPS ? 0.5 : 1.0)/PS_SC);
	if(p_gp->TPsB.P_Params->psformat == PSTERM_LANDSCAPE)
		fprintf(GPT.P_GpPsFile, "90 rotate\n0 %d translate\n", -(int)(pThis->MaxY));
	fprintf(GPT.P_GpPsFile, "0 setgray\nnewpath\n");
	if(ps_common_uses_fonts)
		fprintf(GPT.P_GpPsFile, "(%s) findfont %d scalefont setfont\n",
		    p_gp->TPsB.P_Params->font, pThis->CV());
	p_gp->TPsB.PathCount = 0;
	PS_relative_ok = FALSE;
	PS_pen_x = PS_pen_y = -4000;
	PS_taken = 0;
	PS_linetype_last = LT_UNDEFINED;
	PS_linewidth_last = PS_linewidth_current = LT_UNDEFINED;
	p_gp->TPsB.FontSizePrevious = -1;
	if(p_gp->TPsB.P_Params->terminal != PSTERM_EPSLATEX) {
		/* set the background only if all components are >= 0 */
		fputs("BackgroundColor 0 lt 3 1 roll 0 lt exch 0 lt or or not {", GPT.P_GpPsFile);
		if(p_gp->TPsB.P_Params->psformat == PSTERM_EPS) {
			/* for eps files set the color only for the graphics area */
			fprintf(GPT.P_GpPsFile, "BackgroundColor C 1.000 0 0 %.2f %.2f BoxColFill", pThis->MaxX * p_gp->V.Size.x, pThis->MaxY * p_gp->V.Size.y);
		}
		else {
			/* otherwise set the page background color, the code is taken from the
			   TeX \pagecolor command (color package). */
			fputs("gsave BackgroundColor C clippath fill grestore", GPT.P_GpPsFile);
		}
		fputs("} if\n", GPT.P_GpPsFile);
	}
}

void PS_text(GpTermEntry_Static * pThis)
{
	pThis->P_Gp->TPsB.PathCount = 0;
	fputs("stroke\ngrestore\nend\nshowpage\n", GPT.P_GpPsFile);
	/* fprintf(stderr, "taken %d times\n",PS_taken); */
	/* informational:  tells how many times it was "cheaper"
	 * to do a relative moveto or lineto rather than an
	 * absolute one */
}

void PS_reset(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	fputs("%%Trailer\n", GPT.P_GpPsFile);
	if(ps_common_uses_fonts) {
		fputs("%%DocumentFonts: ", GPT.P_GpPsFile);
		while(PS_DocFonts) {
			struct PS_FontName * fnp;
			fnp = PS_DocFonts->next;
			fprintf(GPT.P_GpPsFile, "%s%s", PS_DocFonts->name, fnp ? " " : "\n");
			SAlloc::F(PS_DocFonts->name);
			SAlloc::F(PS_DocFonts);
			PS_DocFonts = fnp;
		}
	}
	if(p_gp->TPsB.P_Params->psformat != PSTERM_EPS)
		fprintf(GPT.P_GpPsFile, "%%%%Pages: %d\n", p_gp->TPsB.Page);
}

void PS_linetype(GpTermEntry_Static * pThis, int linetype)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(linetype == LT_NODRAW)
		PS_dashtype(pThis, DASHTYPE_NODRAW, NULL);
	if((p_gp->TPsB.P_Params->terminal == PSTERM_EPSLATEX) && p_gp->TPsB.P_Params->oldstyle)
		linetype = (linetype % 4) + 3;
	else
		linetype = (linetype % 9) + 3;
	if(linetype < 0) /* LT_NODRAW, LT_BACKGROUND, LT_UNDEFINED */
		linetype = 0;
	if(PS_linetype_last == linetype) 
		return;
	PS_relative_ok = FALSE;
	PsFlashPath(pThis);
	PS_linetype_last = linetype;
	PS_linewidth_last = PS_linewidth_current;
	if(PS_border && linetype == 1)
		fprintf(GPT.P_GpPsFile, "LTB\n");
	else
		fprintf(GPT.P_GpPsFile, "LT%c\n", "wba012345678"[linetype]);
	p_gp->TPsB.PathCount = 0;
}

void PS_dashtype(GpTermEntry_Static * pThis, int type, t_dashtype * custom_dash_type)
{
	int i;
	double empirical_scale = 0.50;
	switch(type) {
		case DASHTYPE_AXIS:
		    // Handled by LT_AXIS 
		    break;
		case DASHTYPE_SOLID:
		    PsFlashPath(pThis);
		    if(PS_linetype_last != LT_SOLID+3 && PS_linetype_last != LT_AXIS+3)
			    fprintf(GPT.P_GpPsFile, "[] 0 setdash\n");
		    break;
		case DASHTYPE_NODRAW:
		    PsFlashPath(pThis);
		    fprintf(GPT.P_GpPsFile, "[0 100] 0 setdash\n");
		    break;
		case DASHTYPE_CUSTOM:
		    PsFlashPath(pThis);
		    fprintf(GPT.P_GpPsFile, "[");
		    for(i = 0; i < DASHPATTERN_LENGTH && custom_dash_type->pattern[i] > 0; i++) {
			    fprintf(GPT.P_GpPsFile, "%.1f dl%d ",
				custom_dash_type->pattern[i] * empirical_scale, i%2 + 1);
		    }
		    fprintf(GPT.P_GpPsFile, "] 0 setdash\n");
		    break;
		default:
		    if(type > 0)
			    PS_linetype(pThis, type);
		    break;
	}
}

void PS_linewidth(GpTermEntry_Static * pThis, double linewidth)
{
	GnuPlot * p_gp = pThis->P_Gp;
	// HBB NEW 20031219: don't do anything if nothing changed 
	if(p_gp->TPsB.PathCount != 0 && PS_linewidth_last == linewidth)
		return;
	PsFlashPath(pThis);
	PS_linewidth_current = linewidth;
	PS_linetype_last = LT_UNDEFINED; /* disable cache for next linetype change */
	fprintf(GPT.P_GpPsFile, "%.3f UL\n", linewidth);
	/*
	    Documentation of the 'change linewidth' strategy of the postscript terminal:

	    1. define a new postscript variable with a default value:
	    /userlinewidth gnulinewidth def

	    2. define a new postscript command to change the contents of that variable:
	    /UL { gnulinewidth mul /userlinewidth exch def } def
	    usage:  multiplication_factor UL

	    3. modify the already known postscript command /PL for the plot lines:
	    /PL { stroke userlinewidth setlinewidth } def

	    4. issue the new command before every change of the plot linestyle:
	    example:
	    4.0 UL
	    LT0
	    result:
	    Linetype 0 is drawn four times as thick as defined by the contents
	    of the postscript variable 'gnulinewidth'.
	 */
}

void PS_pointsize(GpTermEntry_Static * pThis, double ptsize)
{
	ptsize *= pThis->P_Gp->TPsB.P_Params->pointscale_factor;
	fprintf(GPT.P_GpPsFile, "%.3f UP\n", ptsize);
	/*
	 *  Documentation of the 'change pointsize' strategy of the postscript
	 * terminal:
	 *
	 * 1. define two new postscript variables to hold the overall pointsize:
	 *    /hpt_  and  /vpt_
	 *
	 * 2. define a new postscript command to use the contents of these variables:
	 *    /UP { cf. definition above } def
	 *    usage:  multiplication_factor UP
	 *
	 * [3.] [doesn't exist, skip to next number]
	 *
	 * 4. issue the new command wherever you change the symbols (and linetype):
	 *    example:
	 *  2.5 UP
	 *  4.0 UL  % optionally change linewidth, too
	 *  LT0
	 *    result:
	 *  Next symbols will be drawn 2.5 times as big as defined by the
	 *  GNUPLOT `set pointsize` command (= overall pointsize).
	 */
}

void PS_move(GpTermEntry_Static * pThis, uint x, uint y)
{
	GnuPlot * p_gp = pThis->P_Gp;
	// Make this semi-dynamic and independent of architecture 
	char abso[5+2*INT_STR_LEN], rel[5+2*INT_STR_LEN];
	int dx = x - PS_pen_x;
	int dy = y - PS_pen_y;
	// can't cancel all null moves--need a move after stroke'ing 
	if(dx==0 && dy==0 && PS_relative_ok)
		return;
	sprintf(abso, "%d %d M\n", x, y);
	sprintf(rel, "%d %d R\n", dx, dy);
	if(PS_newpath) {
		fprintf(GPT.P_GpPsFile, "%d %d N\n", x, y);
		PS_newpath = FALSE;
	}
	else if(strlen(rel) < strlen(abso) && PS_relative_ok) {
		fputs(rel, GPT.P_GpPsFile);
		PS_taken++;
	}
	else
		fputs(abso, GPT.P_GpPsFile);
	PS_relative_ok = TRUE;
	p_gp->TPsB.PathCount += 1;
	PS_pen_x = x;
	PS_pen_y = y;
}

void PS_vector(GpTermEntry_Static * pThis, uint x, uint y)
{
	GnuPlot * p_gp = pThis->P_Gp;
	char abso[5+2*INT_STR_LEN], rel[5+2*INT_STR_LEN];
	int dx = x - PS_pen_x;
	int dy = y - PS_pen_y;
	if(dx==0 && dy==0)
		return;
	sprintf(abso, "%d %d L\n", x, y);
	sprintf(rel, "%d %d V\n", dx, dy);
	// The following PS_move() is executed only when the limit of ps_path_count
	// has been reached below: then PS_FLUSH_PATH has been called which has not
	// moved to currentpoint after the stroke. 
	if(!PS_relative_ok)
		PS_move(pThis, PS_pen_x, PS_pen_y);
	if(strlen(rel) < strlen(abso)) {
		fputs(rel, GPT.P_GpPsFile);
		PS_taken++; /* only used for debug info */
		p_gp->TPsB.PathCount += 1;
	}
	else {
		fputs(abso, GPT.P_GpPsFile);
		p_gp->TPsB.PathCount = 1; /* If we set it to zero, it may never get flushed */
	}
	/* Ghostscript has a "pile-up of rounding errors" bug: a sequence of many
	 * rmove's or rlineto's does not yield the same line as move's or lineto's.
	 * Therefore, we periodically force an update of the absolute position.
	 * There was a case when 400 rlineto's were too much, so let's set the limit
	 * just above the 242 segments used by do_arc() to draw a full circle.
	 * This runs into a second ghostscript bug, that mixing relative and absolute
	 * lineto with no intervening 'stroke' is ridiculously slow to render.
	 * So we stroke the partial line, update the position in absolute terms,
	 * then continue.  This whole section can go away if ghostscript/gv is fixed.
	 */
#define MAX_REL_PATHLEN 250
	if(p_gp->TPsB.PathCount >= MAX_REL_PATHLEN) {
		fprintf(GPT.P_GpPsFile, "stroke %d %d M\n", x, y);
		p_gp->TPsB.PathCount = 1;
	}
	PS_relative_ok = TRUE;
	PS_pen_x = x;
	PS_pen_y = y;
}

TERM_PUBLIC void PS_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str)
{
	GnuPlot * p_gp = pThis->P_Gp;
#define PS_NONE 0
#define PS_TEXT 1
#define PS_GLYPH 2
	ulong ch;
	if(isempty(str))
		return;
	if(PS_in_textbox > 0) {
		int save_ang = p_gp->TPsB.Ang;
		PS_in_textbox = -1;
		// Write once with no rotation and no actual "show" command 
		p_gp->TPsB.Ang = 0;
		PS_put_text(pThis, 0, 0, str);
		// Now restore the angle and the "show" command and fall through 
		fprintf(GPT.P_GpPsFile, "/Boxing false def\n");
		fprintf(GPT.P_GpPsFile, "grestore\n");
		p_gp->TPsB.Ang = save_ang;
		PS_in_textbox = 1;
	}
	if(PS_in_textbox >= 0)
		PS_move(pThis, x, y);
	if(p_gp->TPsB.Ang != 0)
		fprintf(GPT.P_GpPsFile, "currentpoint gsave translate %d rotate 0 0 M\n", p_gp->TPsB.Ang);
	else if(PS_in_textbox > 0)
		fprintf(GPT.P_GpPsFile, "gsave currentpoint translate\n");
	if(GPT._Encoding == S_ENC_UTF8 && contains8bit(str)) {
		// UTF-8 encoding with multibyte characters present 
		// Note: uses an intermediate array rather than direct output via fputs so
		// that the bounding box (used by boxed text) can be calculated in 2 passes:
		//   [char sequence] GLwidth [char sequence] GLwidth2
		// First pass updates height, second pass updates width.
		// There must surely be some some way to do this in one pass!
		char strarray[MAX_LINE_LEN];
		int mode = PS_NONE;
		char * c = strarray;
		*c++ = '[';
		for(utf8toulong(&ch, &str); ch != '\0'; utf8toulong(&ch, &str)) {
			if(ch < 0x100) {
				if(mode != PS_TEXT)
					*c++ = '(';
				if(ch == '(' || ch == ')' || ch == '\\')
					*c++ = '\\';
				*c++ = static_cast<char>(ch);
				mode = PS_TEXT;
			}
			else {
				int i;
				if(mode == PS_TEXT)
					*c++ = ')';
				*c++ = '/';
#if (ADOBE_ENCODING_NAMES)
				for(i = 0; i < psglyphs; i++) {
					if(aglist[i].unicode == ch) {
						*c = 0;
						strcat(c, aglist[i].glyphname);
						c += strlen(aglist[i].glyphname);
						break;
					}
				}
				if(i >= psglyphs) /* Must not have found a glyph name */
#endif
				{
					// Special case for the minus sign emitted by gprintf 
					if(ch == 0x2212)
						sprintf(c, "minus");
					else
						sprintf(c, (ch > 0xffff) ? "u%lX%c" : "uni%04lX%c", ch, 0);
				}
				while(*c) c++;
				mode = PS_GLYPH;
			}
		}

		if(mode == PS_TEXT)
			*c++ = ')';
		*c++ = ']';
		*c = 0;
		switch(p_gp->TPsB.Justify) {
			case LEFT:
			    if(PS_in_textbox < 0) {
				    fprintf(GPT.P_GpPsFile, "%s GLwidth\n", strarray);
				    fprintf(GPT.P_GpPsFile, "%s GLwidth2\n", strarray);
			    }
			    else
				    fprintf(GPT.P_GpPsFile, "%s GLshow\n", strarray);
			    break;
			case CENTRE:
			    if(PS_in_textbox < 0) {
				    fprintf(GPT.P_GpPsFile, "%s GCwidth\n", strarray);
				    fprintf(GPT.P_GpPsFile, "%s GCwidth2\n", strarray);
			    }
			    else
				    fprintf(GPT.P_GpPsFile, "%s GCshow\n", strarray);
			    break;
			case RIGHT:
			    if(PS_in_textbox < 0) {
				    fprintf(GPT.P_GpPsFile, "%s GRwidth\n", strarray);
				    fprintf(GPT.P_GpPsFile, "%s GRwidth2\n", strarray);
			    }
			    else
				    fprintf(GPT.P_GpPsFile, "%s GRshow\n", strarray);
			    break;
		}
	}
	else {
		/* plain old 8-bit mode (not UTF-8), or UTF-8 string with only 7-bit characters */
		putc('(', GPT.P_GpPsFile);
		ch = (char)*str++;
		while(ch!='\0') {
			if((ch=='(') || (ch==')') || (ch=='\\'))
				putc('\\', GPT.P_GpPsFile);
			putc((char)ch, GPT.P_GpPsFile);
			ch = (char)*str++;
		}
		switch(p_gp->TPsB.Justify) {
			case LEFT:
			    if(PS_in_textbox < 0)
				    fputs(") Lwidth\n", GPT.P_GpPsFile);
			    else
				    fputs(") Lshow\n", GPT.P_GpPsFile);
			    break;
			case CENTRE:
			    if(PS_in_textbox < 0)
				    fputs(") Cwidth\n", GPT.P_GpPsFile);
			    else
				    fputs(") Cshow\n", GPT.P_GpPsFile);
			    break;
			case RIGHT:
			    if(PS_in_textbox < 0)
				    fputs(") Rwidth\n", GPT.P_GpPsFile);
			    else
				    fputs(") Rshow\n", GPT.P_GpPsFile);
			    break;
		}
	}
	if(p_gp->TPsB.Ang != 0 && (PS_in_textbox == 0))
		fputs("grestore\n", GPT.P_GpPsFile);
	p_gp->TPsB.PathCount = 0;
	PS_relative_ok = FALSE;

#undef PS_NONE
#undef PS_TEXT
#undef PS_GLYPH
}

int PS_text_angle(GpTermEntry_Static * pThis, int ang)
{
	pThis->P_Gp->TPsB.Ang = ang;
	return TRUE;
}

int PS_justify_text(GpTermEntry_Static * pThis, enum JUSTIFY mode)
{
	pThis->P_Gp->TPsB.Justify = mode;
	return TRUE;
}

static int PS_common_set_font(GpTermEntry_Static * pThis, const char * font, int caller(GpTermEntry_Static * pThis, const char * font) )
{
	GnuPlot * p_gp = pThis->P_Gp;
	char * name;
	char * styledfontname = NULL;
	uint i;
	float size;
	size_t sep;
	bool wants_italic = FALSE;
	bool wants_bold = FALSE;
	if(!font || !(*font))
		font = PS_default_font;
	size = p_gp->TPsB.FontSize;
	sep = strcspn(font, ",");
	if(font[sep] == ',')
		sscanf(&(font[sep+1]), "%f", &size);
	if(sep == 0) {
		if(caller == ENHPS_set_font)
			name = sstrdup(p_gp->TPsB.P_Params->font);
		else
			name = sstrdup(PS_default_font);
		sep = strcspn(name, ",");
	}
	else
		name = sstrdup(font);
	name[sep] = '\0';
	// Check for set font "/:Bold" or set font ":Bold" 
	wants_italic = strstr(name, ":Italic");
	wants_bold = strstr(name, ":Bold");
	sep = strcspn(name, ":");
	styledfontname = GnuPlot::_StyleFont((sep == 0 || *name == '/') ? PS_default_font : name, wants_bold, wants_italic);
	FREEANDASSIGN(name, styledfontname);
	// PostScript does not like blanks in font names 
	for(i = 0; name[i] != '\0'; i++)
		if(name[i] == ' ') 
			name[i] = '-';
	if(size <= 0)
		size = p_gp->TPsB.FontSizePrevious;
	if(caller == ENHPS_set_font && !p_gp->Enht.Ignore) {
		p_gp->TPsB.EnhFontSize = size;
		strcpy(p_gp->TPsB.EnhFont, name);
		PS_RememberFont(pThis, name);
		size *= p_gp->TPsB.P_Params->fontscale;
	}
	else {
		// The normal case (caller == PS_set_font).
		// Unless TeX is doing the font handling, we will write the
		// new font info directly into the postscript output stream
		if(p_gp->TPsB.P_Params->terminal == PSTERM_POSTSCRIPT) {
			PS_RememberFont(pThis, name);
			fprintf(GPT.P_GpPsFile, "/%s findfont %g scalefont setfont\n", name, size * _ps_scf(p_gp));
			if(size != p_gp->TPsB.FontSizePrevious)
				fprintf(GPT.P_GpPsFile, "/vshift %d def\n", -(int)((size * _ps_scf(p_gp))/3.0));
			p_gp->TPsB.FontSizePrevious = size;
		}
	}
	SAlloc::F(name);
	pThis->SetCharSize((uint)(size * _ps_scf(p_gp) * 6/10), (uint)(size * _ps_scf(p_gp)));
	return TRUE;
}

int PS_set_font(GpTermEntry_Static * pThis, const char * font)
{
	return PS_common_set_font(pThis, font, PS_set_font);
}
//
// postscript point routines 
//
void PS_point(GpTermEntry_Static * pThis, uint x, uint y, int number)
{
	static const char * pointFNS[] = {
		"Pnt",  "Pls",   "Crs",    "Star",
		"Box",  "BoxF",  "Circle", "CircleF",
		"TriU", "TriUF", "TriD",   "TriDF",
		"Dia",  "DiaF",  "Pent",   "PentF",
		"C0",   "C1",    "C2",     "C3",
		"C4",   "C5",    "C6",     "C7",
		"C8",   "C9",    "C10",    "C11",
		"C12",  "C13",   "C14",    "C15",
		"S0",   "S1",    "S2",     "S3",
		"S4",   "S5",    "S6",     "S7",
		"S8",   "S9",    "S10",    "S11",
		"S12",  "S13",   "S14",    "S15",
		"D0",   "D1",    "D2",     "D3",
		"D4",   "D5",    "D6",     "D7",
		"D8",   "D9",    "D10",    "D11",
		"D12",  "D13",   "D14",    "D15",
		"BoxE", "CircE", "TriUE",  "TriDE",
		"DiaE", "PentE", "BoxW",   "CircW",
		"TriUW", "TriDW", "DiaW",   "PentW"
	};
	static const char * pointFNS_OldEPSL[] = {
		"Pnt",  "Dia",   "Circle", "Pls",
		"Crs",  "Box",   "DiaF",   "CircleF",
		"BoxF"
	};
	GnuPlot * p_gp = pThis->P_Gp;
	if(p_gp->TPsB.P_Params->terminal == PSTERM_EPSLATEX && p_gp->TPsB.P_Params->oldstyle) {
		if(number < 0)
			number = -1; /* negative types are all 'dot' */
		else
			number %= sizeof(pointFNS_OldEPSL)/sizeof(pointFNS_OldEPSL[0]) -1;
		fprintf(GPT.P_GpPsFile, "%d %d %s\n", x, y, pointFNS_OldEPSL[number+1]);
	}
	else {
		if(number < 0)
			number = -1; /* negative types are all 'dot' */
		else
			number %= sizeof(pointFNS)/sizeof(pointFNS[0]) -1;
		fprintf(GPT.P_GpPsFile, "%d %d %s\n", x, y, pointFNS[number+1]);
	}
	PS_relative_ok = FALSE;
	p_gp->TPsB.PathCount = 0;
	PS_linetype_last = LT_UNDEFINED; /* force next linetype change */
}

void PS_fillbox(GpTermEntry_Static * pThis, int style, uint x1, uint y1, uint width, uint height)
{
	double filldens;
	int pattern;
	PsFlashPath(pThis);
	switch(style & 0xf) {
		case FS_DEFAULT:
		    /* Fill with current color, wherever it came from */
		    fprintf(GPT.P_GpPsFile, "%d %d %d %d Rec fill\n", x1, y1, width, height);
		    break;
		case FS_SOLID:
		case FS_TRANSPARENT_SOLID:
		    /* style == 1 --> fill with intensity according to filldensity */
		    filldens = (style >> 4) / 100.0;
		    if(filldens < 0.0)
			    filldens = 0.0;
		    if(filldens > 1.0)
			    filldens = 1.0;
		    fprintf(GPT.P_GpPsFile, "%.3f %d %d %d %d BoxColFill\n",
			filldens, x1, y1, width, height);
		    break;

		case FS_TRANSPARENT_PATTERN:
		    fprintf(GPT.P_GpPsFile, "\n /TransparentPatterns true def\n");
		case FS_PATTERN:
		    /* style == 2 --> fill with pattern according to fillpattern */
		    /* the upper 3 nibbles of 'style' contain pattern number */
		    pattern = (style >> 4) % 8;
		    switch(pattern) {
			    default:
			    case 0: fprintf(GPT.P_GpPsFile, "%d %d %d %d BoxFill\n", x1, y1, width, height); break;
			    case 1: fprintf(GPT.P_GpPsFile, "%d %d %d %d %d %d 1 PatternFill\n", x1, y1, width, height, 80, -45); break;
			    case 2: fprintf(GPT.P_GpPsFile, "%d %d %d %d %d %d 2 PatternFill\n", x1, y1, width, height, 40,  45); break;
			    case 3: fprintf(GPT.P_GpPsFile, "1 %d %d %d %d BoxColFill\n", x1, y1, width, height); break;
			    case 4: fprintf(GPT.P_GpPsFile, "%d %d %d %d %d %d 0 PatternFill\n", x1, y1, width, height, 80,  45); break;
			    case 5: fprintf(GPT.P_GpPsFile, "%d %d %d %d %d %d 0 PatternFill\n", x1, y1, width, height, 80, -45); break;
			    case 6: fprintf(GPT.P_GpPsFile, "%d %d %d %d %d %d 0 PatternFill\n", x1, y1, width, height, 40,  30); break;
			    case 7: fprintf(GPT.P_GpPsFile, "%d %d %d %d %d %d 0 PatternFill\n", x1, y1, width, height, 40, -30); break;
		    }
		    break; // end of pattern filling part 
		case FS_EMPTY:
		default: fprintf(GPT.P_GpPsFile, "%d %d %d %d BoxFill\n", x1, y1, width, height); // fill with background color 
	}
	PS_relative_ok = FALSE;
	PS_linetype_last = LT_UNDEFINED;
}

/* ENHPOST */

/*
 * close a postscript string if it has been opened
 */
TERM_PUBLIC void ENHPS_FLUSH(GpTermEntry_Static * pThis)
{
	if(ENHps_opened_string) {
		fputs(")]\n", GPT.P_GpPsFile);
		ENHps_opened_string = FALSE;
	}
}

static char * ENHps_opensequence = NULL;
//
// open a postscript string
//
TERM_PUBLIC void ENHPS_OPEN(GpTermEntry_Static * pThis, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
{
	GnuPlot * p_gp = pThis->P_Gp;
	// overprint 3 means save current position; 4 means restore saved position
	// EAM FIXME - I couldn't figure out how to use less than the 7 parameters
	// that the normal case macro wants. Somebody more familiar with PostScript
	// than I am please clean it up!
	if(overprint == 3) {
		fputs("XYsave\n", GPT.P_GpPsFile);
		return;
	}
	else if(overprint == 4) {
		fputs("XYrestore\n", GPT.P_GpPsFile);
		return;
	}
	if(!ENHps_opened_string) {
		int safelen = strlen(fontname) + 40;
		bool show_this = showflag && (PS_in_textbox >= 0);
		FREEANDASSIGN(ENHps_opensequence, (char *)SAlloc::M(safelen));
		if(isempty(fontname))
			fontname = p_gp->TPsB.EnhFont;
		else
			PS_RememberFont(pThis, fontname);
		snprintf(ENHps_opensequence, safelen, "[(%s) %.1f %.1f %s %s %d ", fontname, fontsize, base, widthflag ? "true" : "false",
		    show_this ? "true" : "false", overprint);
		fprintf(GPT.P_GpPsFile, "%s(", ENHps_opensequence);
		ENHps_opened_string = TRUE;
	}
}
/*
 * Write one character from inside enhanced text processing.
 * This is trivial except in the case of multi-byte encoding.
 */
TERM_PUBLIC void ENHPS_WRITEC(GpTermEntry_Static * pThis, int c)
{
	static int in_utf8 = 0; /* nonzero means we are inside a multibyte sequence */
	static char utf8[6]; /* holds the multibyte sequence being accumulated   */
	static int nbytes = 0; /* number of bytes expected in the sequence */
	// UTF-8 Encoding 
	if(GPT._Encoding == S_ENC_UTF8 && (c & 0x80) != 0) {
		if(in_utf8 == 0) {
			nbytes = (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : (c & 0xF8) == 0xF0 ? 4 : 0;
			if(!nbytes) /* Illegal UTF8 char; hope it's printable  */
				fputc(c, GPT.P_GpPsFile);
			else
				utf8[in_utf8++] = c;
		}
		else {
			utf8[in_utf8++] = c;
			if(in_utf8 >= nbytes) {
				ulong wch = '\0';
				const char * str = &utf8[0];
				int i;
				utf8[nbytes] = '\0';
				in_utf8 = 0;
				utf8toulong(&wch, &str);
				if(wch < 0x100) { /* Single byte ISO8859-1 character */
					fputc(wch, GPT.P_GpPsFile);
					return;
				}
				// Finish off previous partial string, if any 
				ENHPS_FLUSH(pThis);
				// Write a new partial string for this glyph 
				fprintf(GPT.P_GpPsFile, "%s/", ENHps_opensequence);
#if (ADOBE_ENCODING_NAMES)
				for(i = 0; i < psglyphs; i++) {
					if(aglist[i].unicode == wch) {
						fputs(aglist[i].glyphname, GPT.P_GpPsFile);
						break;
					}
				}
				if(i >= psglyphs) /* Must not have found a glyph name */
#endif
				{
					if(wch == 0x2212)
						fprintf(GPT.P_GpPsFile, "minus");
					else
						fprintf(GPT.P_GpPsFile, (wch > 0xffff) ? "u%lX" : "uni%04lX", wch);
				}
				fprintf(GPT.P_GpPsFile, "]\n");
				// Mark string closed 
				ENHps_opened_string = FALSE;
			}
		}
		/* shige jan 2011 */
	}
	else if(GPT._Encoding == S_ENC_SJIS) {
		static bool in_sjis = FALSE;
		fputc(c, GPT.P_GpPsFile);
		if(in_sjis || (c & 0x80)) {
			// shige: This may remain original string instead octal bytes. 
			if(in_sjis) {
				in_sjis = 0;
				if((uint)(c) == '\\')
					fputc('\\', GPT.P_GpPsFile);
			}
			else {
				in_sjis = TRUE;
			}
		}
	}
	else
		fputc(c, GPT.P_GpPsFile); // Single byte character 
}
// 
// In enhanced text mode the font name_size are stored for
// use in the recursive text processing rather than being written
// to the output stream immediately.
// 
TERM_PUBLIC int ENHPS_set_font(GpTermEntry_Static * pThis, const char * font)
{
	return PS_common_set_font(pThis, font, ENHPS_set_font);
}

TERM_PUBLIC void ENHPS_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(p_gp->Enht.Ignore) {
		PS_put_text(pThis, x, y, str);
		return;
	}
	// flush any pending graphics (all the XShow routines do this...) 
	if(isempty(str))
		return;
	PsFlashPath(pThis);
	if(PS_in_textbox > 0) {
		int save_ang = p_gp->TPsB.Ang;
		PS_in_textbox = -1;
		// Write once with no rotation and no actual "show" command 
		p_gp->TPsB.Ang = 0;
		ENHPS_put_text(pThis, 0, 0, str);
		// Now restore the angle and the "show" command and fall through 
		p_gp->TPsB.Ang = save_ang;
		fprintf(GPT.P_GpPsFile, "/Boxing false def\n");
		fprintf(GPT.P_GpPsFile, "grestore\n");
		PS_in_textbox = 1;
	}
	/* FIXME: if there are no magic characters, we should just be able
	 * punt the string to PS_put_text(), which will give shorter
	 * ps output [eg tics and stuff rarely need extra processing],
	 * but we need to make sure that the current font is the
	 * default one before we can do that. {ie I tried it and it
	 * used the wrong font !}
	 * if (!strpbrk(str, "{}^_@&~"))
	 * {
	 *   - do something to ensure default font is selected
	 *   PS_put_text(x,y,str);
	 *   return;
	 * }
	 */

	if(PS_in_textbox >= 0)
		PS_move(pThis, x, y);
	if(p_gp->TPsB.Ang != 0)
		fprintf(GPT.P_GpPsFile, "currentpoint gsave translate %d rotate 0 0 moveto\n", p_gp->TPsB.Ang);
	else if(PS_in_textbox > 0)
		fprintf(GPT.P_GpPsFile, "gsave currentpoint translate\n");
	fputs("[ ", GPT.P_GpPsFile);
	// set up the global variables needed by enhanced_recursion() 
	p_gp->Enht.MaxHeight = -1000;
	p_gp->Enht.MinHeight = 1000;
	p_gp->Enht.FontScale = _ps_scf(p_gp);
	strnzcpy(p_gp->Enht.EscapeFormat, "\\%o", sizeof(p_gp->Enht.EscapeFormat));
	ENHps_opened_string = FALSE;
	/* Set the recursion going. We say to keep going until a
	 * closing brace, but we don't really expect to find one.
	 * If the return value is not the nul-terminator of the
	 * string, that can only mean that we did find an unmatched
	 * closing brace in the string. We increment past it (else
	 * we get stuck in an infinite loop) and try again.
	 *
	 * ps_enh_font and ps_enh_fontsize are either set to the
	 * the defaults set on option line, or have been set to
	 * "font,size". That is to say, p_gp->TPsB.P_Params->font is used only
	 * at startup and by ENHPS_set_font
	 */
	while(*(str = enhanced_recursion(pThis, str, TRUE, p_gp->TPsB.EnhFont, (double)(p_gp->TPsB.EnhFontSize * _ps_scf(p_gp)), 0.0, TRUE, TRUE, 0))) {
		ENHPS_FLUSH(pThis);
		// I think we can only get here if *str == '}' 
		p_gp->EnhErrCheck(str);
		if(!*++str)
			break; /* end of string */
		// else carry on and process the rest of the string 
	}
	p_gp->Enht.MaxHeight += p_gp->Enht.MinHeight;
	fprintf(GPT.P_GpPsFile, "] %.1f ", -p_gp->Enht.MaxHeight/3);
	switch(p_gp->TPsB.Justify) {
		case LEFT: fputs("MLshow\n", GPT.P_GpPsFile); break;
		case CENTRE: fputs("MCshow\n", GPT.P_GpPsFile); break;
		case RIGHT: fputs("MRshow\n", GPT.P_GpPsFile); break;
	}
	if(p_gp->TPsB.Ang != 0 && (PS_in_textbox == 0))
		fputs("grestore\n", GPT.P_GpPsFile);
	p_gp->TPsB.PathCount = 0;
	PS_relative_ok = FALSE;
	/* Apr 2018: Unlike other terminals, this leaves the last-used font
	 * in the enhanced text string active.  For a long time we dealt with
	 * this by calling term->set_font("") in GnuPlot::IgnoreEnhanced(), but that
	 * introduces other side effects and has no benefit (I think!) for
	 * other terminals.  Try doing it here instead.
	 */
	PS_set_font(pThis, "");
}

static void make_palette_formulae(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
#define R p_gp->SmPltt.formulaR
#define G p_gp->SmPltt.formulaG
#define B p_gp->SmPltt.formulaB
/* print the definition of R,G,B formulae */
	fputs("/InterpolatedColor false def\n", GPT.P_GpPsFile);
	if(p_gp->SmPltt.ps_allcF == 0) { /* print only those 3 used formulae */
		fprintf(GPT.P_GpPsFile, "/cF%i {%s} bind def\t%% %s\n", abs(R), ps_math_color_formulae[2*abs(R)], ps_math_color_formulae[2*abs(R)+1]);
		if(abs(G) != abs(R))
			fprintf(GPT.P_GpPsFile, "/cF%i {%s} bind def\t%% %s\n", abs(G), ps_math_color_formulae[2*abs(G)], ps_math_color_formulae[2*abs(G)+1]);
		if((abs(B) != abs(R)) && (abs(B) != abs(G)))
			fprintf(GPT.P_GpPsFile, "/cF%i {%s} bind def\t%% %s\n", abs(B), ps_math_color_formulae[2*abs(B)], ps_math_color_formulae[2*abs(B)+1]);
	}
	else { /* all color formulae are written into the output PostScript file */
		int i = 0;
		while(*(ps_math_color_formulae[2*i])) {
			fprintf(GPT.P_GpPsFile, "/cF%i {%s} bind def\t%% %s\n", i, ps_math_color_formulae[2*i], ps_math_color_formulae[2*i+1]);
			i++;
		}
	}
#undef R
#undef G
#undef B
}

TERM_PUBLIC void ENHPS_boxed_text(GpTermEntry_Static * pThis, uint x, uint y, int option)
{
	switch(option) {
		case TEXTBOX_INIT:
		    /* Initialize bounding box for the text string drawn at the origin */
		    /* Redefine /textshow so that we update the bounding box without actually writing */
		    fprintf(GPT.P_GpPsFile, "%d %d M\n", x, y);
		    fprintf(GPT.P_GpPsFile, "currentpoint gsave translate 0 0 moveto\n");
		    fprintf(GPT.P_GpPsFile, "0 0 0 0 InitTextBox\n");
		    /* This flags that when we write the string, we must do it twice.
		     * Once to update the bounding box,
		     * then rotate,
		     * the a second time to actually draw the string
		     */
		    PS_in_textbox = 1;
		    break;
		case TEXTBOX_OUTLINE:
		    // Stroke the outline of the accumulated bounding box 
		    fputs("DrawTextBox grestore\n", GPT.P_GpPsFile);
		    PS_in_textbox = 0;
		    break;
		case TEXTBOX_BACKGROUNDFILL:
		    // Fill bounding box with background color 
		    fputs("FillTextBox grestore\n", GPT.P_GpPsFile);
		    PS_in_textbox = 0;
		    break;
		case TEXTBOX_MARGINS:
		    // Change text margins 
		    fprintf(GPT.P_GpPsFile, "/TBxmargin %d def\n", (int)(20*x/100));
		    fprintf(GPT.P_GpPsFile, "/TBymargin %d def\n", (int)(20*y/100));
		    break;
		default:
		    break;
	}
}

static void make_interpolation_code()
{
	static const char * header[] = {
		"/grayindex {/gidx 0 def\n",
		"  {GrayA gidx get grayv ge {exit} if /gidx gidx 1 add def} loop} def\n",
		"/dgdx {grayv GrayA gidx get sub GrayA gidx 1 sub get\n",
		"  GrayA gidx get sub div} def \n",
		"/redvalue {RedA gidx get RedA gidx 1 sub get\n",
		"  RedA gidx get sub dgdxval mul add} def\n",
		"/greenvalue {GreenA gidx get GreenA gidx 1 sub get\n",
		"  GreenA gidx get sub dgdxval mul add} def\n",
		"/bluevalue {BlueA gidx get BlueA gidx 1 sub get\n",
		"  BlueA gidx get sub dgdxval mul add} def\n",
		"/interpolate {\n",
		"  grayindex grayv GrayA gidx get sub abs 1e-5 le\n",
		"    {RedA gidx get GreenA gidx get BlueA gidx get}\n",
		"    {/dgdxval dgdx def redvalue greenvalue bluevalue} ifelse} def\n",
		NULL,
	};
	for(int i = 0; header[i]; ++i) {
		fputs(header[i], GPT.P_GpPsFile);
	}
}

static void make_color_model_code()
{
	// Postscript version of the color space transformations in getcolor.c 
	static const char * header[] = {
		"/HSV2RGB {",
		"  exch dup 0.0 eq {pop exch pop dup dup} % achromatic gray\n",
		"  { /HSVs exch def /HSVv exch def 6.0 mul dup floor dup 3 1 roll sub\n ",
		"    /HSVf exch def /HSVi exch cvi def /HSVp HSVv 1.0 HSVs sub mul def\n",
		"	 /HSVq HSVv 1.0 HSVs HSVf mul sub mul def \n",
		"	 /HSVt HSVv 1.0 HSVs 1.0 HSVf sub mul sub mul def\n",
		"	 /HSVi HSVi 6 mod def 0 HSVi eq {HSVv HSVt HSVp}\n",
		"	 {1 HSVi eq {HSVq HSVv HSVp}{2 HSVi eq {HSVp HSVv HSVt}\n",
		"	 {3 HSVi eq {HSVp HSVq HSVv}{4 HSVi eq {HSVt HSVp HSVv}\n",
		"	 {HSVv HSVp HSVq} ifelse} ifelse} ifelse} ifelse} ifelse\n",
		"  } ifelse} def\n",
		"/Constrain {\n",
		"  dup 0 lt {0 exch pop}{dup 1 gt {1 exch pop} if} ifelse} def\n",
		"/CMY2RGB {",
		"  1 exch sub exch 1 exch sub 3 2 roll 1 exch sub 3 1 roll exch } def\n",
		"/XYZ2RGB {",
		"  3 copy -0.9017 mul exch -0.1187 mul add exch 0.0585 mul exch add\n",
		"  Constrain 4 1 roll 3 copy -0.0279 mul exch 1.999 mul add exch\n",
		"  -0.9844 mul add Constrain 5 1 roll -0.2891 mul exch -0.5338 mul add\n",
		"  exch 1.91 mul exch add Constrain 3 1 roll} def\n",
		"/SelectSpace {ColorSpace (HSV) eq {HSV2RGB}{ColorSpace (XYZ) eq {\n",
		"  XYZ2RGB}{ColorSpace (CMY) eq {CMY2RGB}{ColorSpace (YIQ) eq {YIQ2RGB}\n",
		"  if} ifelse} ifelse} ifelse} def\n",
		NULL,
	};
	for(int i = 0; header[i]; ++i) {
		fputs(header[i], GPT.P_GpPsFile);
	}
}

//static char * save_space(double gray)
/*static*/char * GpPostscriptBlock::SaveSpace(double gray)
{
	// printing the gray with 4 digits and without the leading 0 ... saving space 
	static char s[40];
	gray = 0.0001*(int)(gray*10000+0.5); /* round it to 4 digits */
	sprintf(s, "%.4g", gray);
	if(s[0] == '0' && s[1] == '.')
		return &(s[1]); // strip leading 0 
	else
		return s;
}

static void write_color_space(t_sm_palette * palette)
{
	/* write something like
	 *   /ColorSpace (HSV) def
	 * depending on the selected cmodel in palette */
	fputs("/ColorSpace ", GPT.P_GpPsFile);
	switch(palette->CModel) {
		case C_MODEL_RGB: fputs("(RGB)", GPT.P_GpPsFile); break;
		case C_MODEL_HSV: fputs("(HSV)", GPT.P_GpPsFile); break;
		case C_MODEL_CMY: fputs("(CMY)", GPT.P_GpPsFile); break;
		case C_MODEL_XYZ: fputs("(XYZ)", GPT.P_GpPsFile); break;
		default: fprintf(stderr, "%s:%d ooops: Unknown color model '%c'. Will be RGB\n", __FILE__, __LINE__, (char)(palette->CModel));
		    fputs("(RGB)", GPT.P_GpPsFile);
		    break;
	}
	fputs(" def\n", GPT.P_GpPsFile);
}

static void write_component_array(const char * text, gradient_struct * grad, int cnt, int offset)
{
	/* write something like
	 *     /RedA [ 0 .1 .2 .3 .35 .3 .2 .1 0 0 0 ] def
	 *  nicely formatted to GPT.P_GpPsFile
	 */
	int i = 0, len = 0;
	char * val;
	fprintf(GPT.P_GpPsFile, "/%s [", text);
	len = strlen(text) + 4;
	for(i = 0; i<cnt; ++i) {
		char * ref = (char *)(&(grad[i]));
		ref += offset;
		val = GpPostscriptBlock::SaveSpace(*((double*)(ref)));
		len += strlen(val) + 1;
		if(len > 77) {
			fputs("\n  ", GPT.P_GpPsFile);
			len = strlen(val) + 3;
		}
		fprintf(GPT.P_GpPsFile, "%s ", val);
	}
	fputs("] def\n", GPT.P_GpPsFile);
}

static void write_gradient_definition(gradient_struct * gradient, int cnt)
{
	/* some strange pointer acrobatic here, but it seems to work... */
	char * ref = (char *)(gradient);
	int p = (char *)(&(gradient[0].pos)) - ref;
	int r = (char *)(&(gradient[0].col.r)) - ref;
	int g = (char *)(&(gradient[0].col.g)) - ref;
	int b = (char *)(&(gradient[0].col.b)) - ref;
	write_component_array("GrayA", gradient, cnt, p);
	write_component_array("RedA", gradient, cnt, r);
	write_component_array("GreenA", gradient, cnt, g);
	write_component_array("BlueA", gradient, cnt, b);
}

static void PS_make_header(GpTermEntry_Static * pThis, t_sm_palette * palette)
{
	GnuPlot * p_gp = pThis->P_Gp;
	// write header for smooth colors 
	fputs("gsave % colour palette begin\n", GPT.P_GpPsFile);
	fprintf(GPT.P_GpPsFile, "/maxcolors %i def\n", p_gp->SmPltt.UseMaxColors);
	make_color_model_code();
	switch(p_gp->SmPltt.colorMode) {
		case SMPAL_COLOR_MODE_GRAY:
		    fputs("/InterpolatedColor false def\n", GPT.P_GpPsFile);
		    break; /* nothing to do for gray */
		case SMPAL_COLOR_MODE_RGB:
		    make_palette_formulae(pThis);
		    break;
		case SMPAL_COLOR_MODE_CUBEHELIX:
		case SMPAL_COLOR_MODE_FUNCTIONS: 
			{
				int cnt = 0;
				gradient_struct * p_gradient;
				fputs("/InterpolatedColor true def\n", GPT.P_GpPsFile);
				make_interpolation_code();
				p_gradient = p_gp->ApproximatePalette(palette, p_gp->TPsB.P_Params->palfunc_samples, p_gp->TPsB.P_Params->palfunc_deviation, &cnt);
				write_gradient_definition(p_gradient, cnt);
				SAlloc::F(p_gradient);
			}
			break;
		case SMPAL_COLOR_MODE_GRADIENT:
		    fputs("/InterpolatedColor true def\n", GPT.P_GpPsFile);
		    make_interpolation_code();
		    write_gradient_definition(palette->P_Gradient, palette->GradientNum);
		    break;
		default:
		    fprintf(stderr, "%s:%d ooops: Unknown color mode '%c'\n", __FILE__, __LINE__, (char)(p_gp->SmPltt.colorMode));
	}
	fputs("/pm3dround {maxcolors 0 gt {dup 1 ge\n", GPT.P_GpPsFile);
	fputs("\t{pop 1} {maxcolors mul floor maxcolors 1 sub div} ifelse} if} def\n", GPT.P_GpPsFile);
	fprintf(GPT.P_GpPsFile, "/pm3dGamma 1.0 %g Gamma mul div def\n", p_gp->SmPltt.gamma);
	write_color_space(palette);

/* Now print something like
   /g {dup cF7 exch dup cF5 exch cF15 setrgbcolor} bind def
 */
#define R p_gp->SmPltt.formulaR
#define G p_gp->SmPltt.formulaG
#define B p_gp->SmPltt.formulaB

/* 18.1.2009 Since the beginning of pm3d, the Color definition switched
   between gray and colour map. This led to ambiguities for custom colour
   palettes if they contain grays only. Thus let postscript choose always
   always colour palette for interpolated colours ('set palette defined',
   'set palette file') and colour/gray according to Color otherwise
   ('set palette gray', 'set palette rgb').
 */
	if(p_gp->SmPltt.colorMode == SMPAL_COLOR_MODE_GRAY)
		fputs("false { % COLOUR vs. GRAY map\n", GPT.P_GpPsFile);
	else
		fputs("Color InterpolatedColor or { % COLOUR vs. GRAY map\n", GPT.P_GpPsFile);

	fputs("  InterpolatedColor { %% Interpolation vs. RGB-Formula\n", GPT.P_GpPsFile);
	fputs("    /g {stroke pm3dround /grayv exch def interpolate\n", GPT.P_GpPsFile);
	fputs("        SelectSpace setrgbcolor} bind def\n", GPT.P_GpPsFile);
	fputs("  }{\n", GPT.P_GpPsFile);
	fputs("  /g {stroke pm3dround dup ", GPT.P_GpPsFile);
	if(R < 0)
		fputs("1 exch sub ", GPT.P_GpPsFile); /* negate */
	fprintf(GPT.P_GpPsFile, "cF%i Constrain exch dup ", abs(R));
	if(G < 0)
		fputs("1 exch sub ", GPT.P_GpPsFile); /* negate */
	fprintf(GPT.P_GpPsFile, "cF%i Constrain exch ", abs(G));
	if(R<0 || G<0 || B<0)
		fputs("\n\t", GPT.P_GpPsFile);
	if(B < 0)
		fputs("1 exch sub ", GPT.P_GpPsFile); /* negate */
	fprintf(GPT.P_GpPsFile, "cF%i Constrain ", abs(B));
	fputs("\n       SelectSpace setrgbcolor} bind def\n", GPT.P_GpPsFile);
	fputs("  } ifelse\n", GPT.P_GpPsFile);
	fputs("}{\n", GPT.P_GpPsFile);
	fputs("  /g {stroke pm3dround pm3dGamma exp setgray} bind def\n", GPT.P_GpPsFile);
	fputs("} ifelse\n", GPT.P_GpPsFile);
#undef R
#undef G
#undef B
}

int PS_make_palette(GpTermEntry_Static * pThis, t_sm_palette * pPalette)
{
	if(!pPalette) {
		return 0; // postscript can do continuous colors 
	}
	else {
		PS_make_header(pThis, pPalette);
		return 0;
	}
}

void PS_set_color(GpTermEntry_Static * pThis, const t_colorspec * colorspec)
{
	GnuPlot * p_gp = pThis->P_Gp;
	double gray;
	PS_linetype_last = LT_UNDEFINED; /* Force next call to linetype to be honored */
	if(PS_linewidth_last != PS_linewidth_current) {
		PS_linewidth_last = PS_linewidth_current;
		fprintf(GPT.P_GpPsFile, "PL ");
	}
	if(colorspec->type == TC_LT) {
		int linetype = colorspec->lt;
		PsFlashPath(pThis);
		if((p_gp->TPsB.P_Params->terminal == PSTERM_EPSLATEX) && p_gp->TPsB.P_Params->oldstyle)
			linetype = (linetype % 4) + 3;
		else
			linetype = (linetype % 9) + 3;
		if(linetype < 0) /* LT_NODRAW, LT_BACKGROUND, LT_UNDEFINED */
			linetype = 0;
		fprintf(GPT.P_GpPsFile, "LC%1c setrgbcolor\n", "wba012345678"[linetype]);
	}
	else if(colorspec->type == TC_RGB) {
		double r = (double)((colorspec->lt >> 16) & 255) / 255.0;
		double g = (double)((colorspec->lt >> 8) & 255) / 255.0;
		double b = (double)(colorspec->lt & 255) / 255.0;
		PsFlashPath(pThis);
		fprintf(GPT.P_GpPsFile, "%3.2f %3.2f %3.2f C\n", r, g, b);
	}
	if(colorspec->type != TC_FRAC)
		return;
	// map [0;1] to gray/colors 
	gray = colorspec->value;
	if(gray <= 0)
		fputs("0 g ", GPT.P_GpPsFile);
	else {
		if(gray >= 1)
			fputs("1 g ", GPT.P_GpPsFile);
		else
			fprintf(GPT.P_GpPsFile, "%s g ", GpPostscriptBlock::SaveSpace(gray));
	}
	PS_relative_ok = FALSE; /* "M" required because "g" forces stroke (??) */
}

void PS_filled_polygon(GpTermEntry_Static * pThis, int points, gpiPoint * corners)
{
	int i;
	float filldens = 1.0f;
	int pattern = 0;
	int style = corners->style;
	// Stroke the previous graphic element if required. 
	if(PS_relative_ok)
		PsFlashPath(pThis);
	if(points == 4 && style == FS_OPAQUE) {
		/* Special case for pm3d surface quadrangles
		 *  <x0> <y0> ... <x4> <y4> h
		 */
		fprintf(GPT.P_GpPsFile, "%i %i N", corners[0].x, corners[0].y);
		fprintf(GPT.P_GpPsFile, " %i %i %i %i %i %i h\n", corners[3].x-corners[2].x, corners[3].y-corners[2].y,
		    corners[2].x-corners[1].x, corners[2].y-corners[1].y, corners[1].x-corners[0].x, corners[1].y-corners[0].y);
	}
	else {
		/* General case for solid or pattern-filled polygons
		 * gsave <x0> <y0> N <x1> <y1> ... <xn> <yn> density PolyFill
		 */
		int fillpar = style >> 4;
		style = style &0xf;
		fprintf(GPT.P_GpPsFile, "gsave ");
		fprintf(GPT.P_GpPsFile, "%i %i N", corners[0].x, corners[0].y);
		for(i = 1; i < points; i++) {
			/* The rationale for mixing V and L is given in PS_vector */
			if(i % MAX_REL_PATHLEN)
				fprintf(GPT.P_GpPsFile, " %i %i V", corners[i].x-corners[i-1].x, corners[i].y-corners[i-1].y);
			else
				fprintf(GPT.P_GpPsFile, " %i %i L", corners[i].x, corners[i].y);
		}
		switch(style) {
			case FS_SOLID:
			case FS_TRANSPARENT_SOLID:
			    filldens = fillpar / 100.0f;
			    if(filldens < 0.0)
				    filldens = 0.0;
			    if(filldens >= 1.0)
				    fprintf(GPT.P_GpPsFile, " 1 PolyFill\n");
			    else
				    fprintf(GPT.P_GpPsFile, " %.2f PolyFill\n", filldens);
			    break;

			case FS_TRANSPARENT_PATTERN:
			    fprintf(GPT.P_GpPsFile, " /TransparentPatterns true def\n");
			case FS_PATTERN:
			    pattern = (fillpar) % 8;
			    if(pattern == 0) {
				    filldens = 0.5;
				    fprintf(GPT.P_GpPsFile, " %.1f PolyFill\n", filldens);
			    }
			    else {
				    fprintf(GPT.P_GpPsFile, " Pattern%d fill grestore\n", pattern);
			    }
			    break;

			default:
			    fputs(" 1 PolyFill\n", GPT.P_GpPsFile);
			    break;
		}
	}

	PS_relative_ok = FALSE;
}

#undef MAX_REL_PATHLEN

void PS_previous_palette(GpTermEntry_Static * pThis)
{
	// Needed to stroke the previous graphic element. 
	PsFlashPath(pThis);
	fputs("grestore % colour palette end\n", GPT.P_GpPsFile);
}

static void delete_ps_fontfile(GpTermEntry_Static * pThis, ps_fontfile_def * prev, ps_fontfile_def * pCurrent)
{
	if(pCurrent) { // there really is something to delete 
		FPRINTF((stderr, "Remove font/kerning file `%s'\n", pCurrent->fontfile_name));
		if(prev) // there is a previous ps_fontfile 
			prev->next = pCurrent->next;
		else // pThis = p_gp->TPsB.P_Params->first_fontfile so change p_gp->TPsB.P_Params->first_fontfile 
			pThis->P_Gp->TPsB.P_Params->first_fontfile = pCurrent->next;
		SAlloc::F(pCurrent->fontfile_name);
		SAlloc::F(pCurrent->fontfile_fullname);
		SAlloc::F(pCurrent->fontname);
		ZFREE(pCurrent);
	}
}

static void PS_encode85(ulong tuple4, uchar * tuple5)
{
	/* The compiler should know to carry out the powers of
	 * 85 computation at compilation time.
	 */
	tuple5[0] = static_cast<uchar>(tuple4/(85*85*85*85));
	tuple4 -= ((ulong)tuple5[0])*(85*85*85*85);
	tuple5[1] = static_cast<uchar>(tuple4/(85*85*85));
	tuple4 -= ((ulong)tuple5[1])*(85*85*85);
	tuple5[2] = static_cast<uchar>(tuple4/(85*85));
	tuple4 -= ((ulong)tuple5[2])*(85*85);
	tuple5[3] = static_cast<uchar>(tuple4/(85));
	tuple4 -= ((ulong)tuple5[3])*(85);
	tuple5[4] = static_cast<uchar>(tuple4);
}

/* Use libgd2 or libcairo to save bitmap images with deflate compression
   and PNG predictors.

   We prefer libgd over libcairo, because it supports bit packing if the
   image contains e.g. only 4 bit gray depth.

   libgd does not support gray scale images out of the box, like we need
   for all paletted images (the palette is constructed on Postscript
   side). The gray scale image can be emulated with a paletted image if
   the PNG palette is allocated manually before setting the first pixel.

   The best option would be to use libpng, because it would require only a
   single piece of code, but we had some trouble with setjmp.h being
   included from some header before this terminal driver file.
 */
#ifdef HAVE_DEFLATE_ENCODER
/* structure to store PNG image bytes */
typedef struct {
	uchar * buffer; /* the buffer which holds the png data */
	size_t size; /* the current total buffer size */
	size_t length; /* the used buffer length */
} png_buffer_encode_t;

/* Grab an unsigned 32 bit integer from a buffer in big-endian format.
   Code copied directly from libpng.h. */
static uint png_get_uint(uchar * buf)
{
	uint i = ((uint)(*buf) << 24) + ((uint)(*(buf + 1)) << 16) + ((uint)(*(buf + 2)) << 8) + (uint)(*(buf + 3));
	return (i);
}

/* Extract and concatenate all IDAT chunks.

   'encoded_image' contains the complete PNG image which has 'image_size' bytes.
   We need only the IDAT chunks, which are extracted and moved to the beginning
   of the 'encoded_image' buffer. The overall size of the IDAT chunks is returned
   by the function. */
static int png_extract_idat_chunks(uchar * encoded_image, int image_size)
{
	uchar * encoded_image_ptr;
	uint chunk_name, chunk_length;
	int buf_pos = 8; /* skip the PNG signature (first eight bit) */
	// The IDAT and IEND chunk names in correct byte order. 
	uint png_idat, png_iend;
	png_idat = png_get_uint((uchar *)"IDAT");
	png_iend = png_get_uint((uchar *)"IEND");
	encoded_image_ptr = encoded_image;
	while(buf_pos < image_size) {
		chunk_length = png_get_uint(encoded_image + buf_pos);
		buf_pos += 4;
		chunk_name = png_get_uint(encoded_image + buf_pos);
		buf_pos += 4;
		if(chunk_name == png_idat) {
			/* concat this IDAT chunk with previously found ones. The data is
			   shifted within the existing buffer, therefore we use memmove(). */
			memmove(encoded_image_ptr, encoded_image + buf_pos, chunk_length);
			encoded_image_ptr += chunk_length;
		}
		else if(chunk_name == png_iend) {
			/* found the IEND chunk, we are done */
			break;
		}
		buf_pos += chunk_length + 4; /* skip the CRC */
	}
	return encoded_image_ptr - encoded_image;
}

#endif

#ifdef HAVE_GD_PNG

static void write_png_image_to_buffer(GpTermEntry_Static * pThis, uint M, uint N, coordval * image, t_imagecolor color_mode, 
	int bits_per_component, int max_colors, double cscale, png_buffer_encode_t * png_buffer)
{
	GnuPlot * p_gp = pThis->P_Gp;
	gdImagePtr png_img;
	int m, n, pixel;
	if(color_mode == IC_RGB) {
		png_img = gdImageCreateTrueColor(M, N);
	}
	else {
		/* we would need a gray scale image, but libgd does not support
		   it directly. We emulate it with a palette image which in the end
		   does not matter for us. */
		png_img = gdImageCreatePalette(M, N);
	}
	if(!png_img) {
		p_gp->IntError(NO_CARET, "GNUPLOT (post.trm): failed to create libgd image structure");
	}
	if(color_mode == IC_RGB) {
		rgb_color rgb1;
		rgb255_color rgb255;
		for(n = 0; n < N; n++) {
			for(m = 0; m < M; m++) {
				rgb1.r = cscale * (*image++);
				rgb1.g = cscale * (*image++);
				rgb1.b = cscale * (*image++);
				rgb255_from_rgb1(rgb1, &rgb255);
				pixel = gdImageColorResolve(png_img, (int)rgb255.r, (int)rgb255.g, (int)rgb255.b);
				gdImageSetPixel(png_img, m, n, pixel);
			}
		}
	}
	else {
		int gray_tmp;
		/* preallocate all colors to ensure proper mapping of the image
		   palette to the gray values. Allocate only as many colors as
		   needed to enable bit packing of low-depth images. */
		for(n = 0; n < (1 << bits_per_component); n++) {
			if(gdImageColorAllocate(png_img, n, n, n) < 0) {
				p_gp->IntError(NO_CARET, "GNUPLOT(post.trm): libgd failed to allocate gray value %d\n", n);
			}
		}
		for(n = 0; n < N; n++) {
			for(m = 0; m < M; m++) {
				if(isnan(*image)) {
					pixel = gdImageColorResolve(png_img, 0, 0, 0);
				}
				else {
					gray_tmp = (int)((*image) * max_colors);
					if(gray_tmp  > (max_colors - 1)) {
						gray_tmp = max_colors - 1;
					}
					gray_tmp = (int)(gray_tmp * cscale);
					pixel = gdImageColorResolve(png_img, gray_tmp, gray_tmp, gray_tmp);
				}
				gdImageSetPixel(png_img, m, n, pixel);
				image++;
			}
		}
	}

	/* use a temporary buffer, because the image memory must be freed
	   with gdFree() and the png_buffer->buffer should not depend on
	   whether we used libcairo or libgd. */
	{
		char * png_buffer_tmp;
		int size;
		png_buffer_tmp = (char *)gdImagePngPtr(png_img, &size);
		png_buffer->size = size;
		png_buffer->buffer = SAlloc::M(png_buffer->size);
		memcpy(png_buffer->buffer, png_buffer_tmp, png_buffer->size);

		gdImageDestroy(png_img);
		gdFree(png_buffer_tmp);
	}
}

#elif defined(HAVE_CAIROPDF)
/* use libcairo only if libgd is not available. This may increase the output file size. */

static cairo_status_t write_png_data_to_buffer(void * png_ptr, const uchar * data, uint length)
{
	png_buffer_encode_t * p = (png_buffer_encode_t*)png_ptr;
	size_t new_size;
	size_t new_length = p->length + length;
	if(new_length > p->size) {
		// increase the buffer by the standard size of an IDAT chunk including the chunk name, length and CRC 
		new_size = p->size + 8192 + 12;
		if(new_length > new_size) {
			new_size = new_length;
		}
		p->buffer = (uchar *)SAlloc::R(p->buffer, new_size);
		p->size = new_size;
	}
	memcpy(p->buffer + p->length, data, length);
	p->length = new_length;
	return CAIRO_STATUS_SUCCESS;
}

static void write_png_image_to_buffer(GpTermEntry_Static * pThis, uint M, uint N, coordval * image, t_imagecolor color_mode, 
	int bits_per_component, int max_colors, double cscale, png_buffer_encode_t * png_buffer)
{
	uchar * image255;
	cairo_surface_t * image_surface;
	cairo_status_t cairo_stat;
	cairo_format_t format;
	int stride;
	int m, n;
	format = (color_mode == IC_RGB) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;
	stride = cairo_format_stride_for_width(format, M);
	image255 = (uchar *)SAlloc::M(N * stride);
	if(color_mode == IC_RGB) {
		/* Adapted from gp_cairo_helpers.c (use unsigned int to respect endianess of the platform). */
		rgb_color rgb1;
		rgb255_color rgb255;
		uint * image255_ptr;
		image255_ptr = (uint *)image255;
		for(n = 0; n < N; n++) {
			for(m = 0; m < M; m++) {
				rgb1.r = *image++;
				rgb1.g = *image++;
				rgb1.b = *image++;
				rgb255_from_rgb1(rgb1, &rgb255);
				*image255_ptr++ = (0xFF<<24) + (rgb255.r<<16) + (rgb255.g<<8) + rgb255.b;
			}
		}
	}
	else {
		uchar * image255_ptr;
		int gray_tmp;
		image255_ptr = image255;
		for(n = 0; n < M * N; n++) {
			gray_tmp = (int)((*image++) * max_colors);
			if(gray_tmp  > (max_colors - 1)) {
				gray_tmp = max_colors - 1;
			}
			*image255_ptr = (uchar)(gray_tmp * cscale);
			image255_ptr++;
		}
	}
	// now create the actual image surface from the data in 'image255' 
	image_surface = cairo_image_surface_create_for_data(image255, format, M, N, stride);
	cairo_stat = cairo_surface_write_to_png_stream(image_surface, (cairo_write_func_t)write_png_data_to_buffer, png_buffer);
	cairo_surface_destroy(image_surface);
	SAlloc::F(image255);
	if(cairo_stat != CAIRO_STATUS_SUCCESS) {
		pThis->P_Gp->IntError(NO_CARET, "GNUPLOT(post.trm): could not write cairo png image to buffer: %s.", cairo_status_to_string(cairo_stat));
	}
}

#endif /* defined(HAVE_CAIROPDF) */

#ifdef HAVE_DEFLATE_ENCODER

/* Encode the image using first deflate compression with PNG predictors
   followed by ASCII85 encoding to avoid potential problems with some
   characters contained in the binary stream (e.g 0x04, 0x13 etc).
   Some programs like dvips understand DSC like %%BeginBinary which could
   be used to mark the binary data, but other programs do not.
 */
static uchar * PS_encode_png_image(GpTermEntry_Static * pThis, uint M, uint N, coordval * image, t_imagecolor color_mode, int bits_per_component, int max_colors, double cscale, int * return_num_bytes)
{
	png_buffer_encode_t png_buffer;
	uchar * encoded_image_tmp, * encoded_image_tmp_ptr, * encoded_image, * encoded_image_ptr;
#define ASCII_PER_LINE 78
	int max_encoded_bytes;
	int i, j, i_line, n;
	ulong tuple4;
	uchar tuple5[5];
	png_buffer.size = png_buffer.length = 0;
	png_buffer.buffer = NULL;
	write_png_image_to_buffer(pThis, M, N, image, color_mode, bits_per_component, max_colors, cscale, &png_buffer);
	encoded_image_tmp = png_buffer.buffer;
	*return_num_bytes = png_extract_idat_chunks(encoded_image_tmp, png_buffer.size);
	if(*return_num_bytes == 0) {
		SAlloc::F(encoded_image_tmp);
		encoded_image_tmp = NULL;
		return encoded_image_tmp;
	}
	//
	// Apply ASCII85 encoding to the deflate data. Code adapted from PS_encode_image. 
	//
	// Compute max number of ASCII characters encoding will require. 
	max_encoded_bytes = (*return_num_bytes/4 + 1)*5 + 2; /* 5 tuples and additional ~> */
	max_encoded_bytes += max_encoded_bytes / ASCII_PER_LINE; /* newline characters */
	// allocate new memory for the resulting image. 
	encoded_image = (uchar *)SAlloc::M(max_encoded_bytes);
	encoded_image_ptr = encoded_image;
	encoded_image_tmp_ptr = encoded_image_tmp;
	i_line = ASCII_PER_LINE;
	j = 0;
	while(j <= (*return_num_bytes - 4)) {
		tuple4 = (ulong)png_get_uint(encoded_image_tmp_ptr);
		encoded_image_tmp_ptr += 4;
		j += 4;
		PS_encode85(tuple4, tuple5);
		for(i = 0; i < 5; i++) {
			sprintf((char *)(encoded_image_ptr++), "%c", tuple5[i]+'!');
			i_line--;
			if(!i_line) {
				i_line = ASCII_PER_LINE;
				*encoded_image_ptr++ = '\n';
			}
		}
	}
	/* see if there are some trailing bytes which do not make a complete 4-tuple.
	   These must be handled separately. */
	n = (*return_num_bytes - j);
	if(n > 0) {
		tuple4 = 0;
		for(i = 0; i < n; i++) {
			/* explicit type cast required! */
			tuple4 += ((ulong)*(encoded_image_tmp_ptr++)) << (24 - i*8);
		}
		PS_encode85(tuple4, tuple5);
		for(i = 0; i <= n; i++) {
			sprintf((char *)(encoded_image_ptr++), "%c", tuple5[i]+'!');
			i_line--;
			if(!i_line) {
				i_line = ASCII_PER_LINE;
				*encoded_image_ptr++ = '\n';
			}
		}
	}
	SAlloc::F(encoded_image_tmp);
	sprintf((char *)(encoded_image_ptr), "~>");
	encoded_image_ptr += 2;
	*return_num_bytes = encoded_image_ptr - encoded_image;
	return encoded_image;
}

#endif /* HAVE_DEFLATE_ENCODER */

enum PS_ENCODING {
	PS_ASCII_HEX,
	PS_ASCII85
} PS_ENCODING;
// 
// Returns pointer to encoded image, allocated on heap that the
// caller must free.  Can error to command line so make sure all
// heap memory is recorded in static pointers when calling this routine.
// 
static char * PS_encode_image(GpTermEntry_Static * pThis, uint M, uint N, coordval * image, t_imagecolor color_mode,
    int bits_per_component, int max_colors, double cscale, enum PS_ENCODING encoding, int * return_num_bytes)
{
	GnuPlot * p_gp = pThis->P_Gp;
	uint coord_remaining;
	coordval * coord_ptr;
	ushort i_line;
	uint i_element;
	uint end_of_line;
	ushort bits_remaining, bits_start;
	ulong tuple4;
	uchar tuple5[5];
	int max_encoded_bytes;
	char * encoded_image, * encoded_image_ptr;
	ulong total_bits;
#define ASCII_PER_LINE 78
	// 18.1.2009 RGB images ("plot ... with rgbimage") are drawn always in color,
	// i.e. for both "set term post color" and "set term post mono".
	total_bits = bits_per_component*M*N*((color_mode == IC_RGB /* && p_gp->TPsB.P_Params->color */) ? 3 : 1);
	// At the end of each image line, data is aligned to the nearest 8 bits,
	// which means potentially adding 7 bits per line.
	end_of_line = M;
	total_bits += N*7;
	// Compute max number of ascii characters encoding will require.
	if(encoding == PS_ASCII_HEX) {
		// Straight hex encoding 
		max_encoded_bytes = (total_bits/4 + 1);
		max_encoded_bytes += max_encoded_bytes / ASCII_PER_LINE; /* newline characters */
	}
	else {
		// ASCII85 encoding 
		max_encoded_bytes = (total_bits/32 + 1)*5 + 2; /* 5 tuples and additional ~> */
		max_encoded_bytes += max_encoded_bytes / ASCII_PER_LINE; /* newline characters */
	}
	// Reserve enough memory. 
	if(!(encoded_image = (char *)SAlloc::M(max_encoded_bytes)))
		p_gp->IntError(NO_CARET, "GNUPLOT (post.trm):  Error allocating memory.\n");
	encoded_image_ptr = encoded_image;
	coord_ptr = image;
	i_line = ASCII_PER_LINE;
	i_element = 0;
	coord_remaining = M*N;
	if(color_mode == IC_RGB /* && p_gp->TPsB.P_Params->color */) {
		end_of_line *= 3;
		coord_remaining *= 3;
	}
	bits_remaining = 32;
	bits_start = 0;
	tuple4 = 0;
	while(coord_remaining) {
		ushort us_tmp;
		if(0 /* color_mode == IC_RGB && !p_gp->TPsB.P_Params->color */) {
			coordval c_tmp;
			c_tmp = *coord_ptr++;
			c_tmp += *coord_ptr++;
			c_tmp += *coord_ptr++;
			us_tmp = (ushort)(c_tmp*(max_colors-1)/3.0 + 0.5);
		}
		else
			us_tmp = (ushort)((*coord_ptr++) * max_colors);
		SETMIN(us_tmp, max_colors-1);
		// Rescale to accommodate a mismatch between max_colors and # of bits 
		us_tmp = static_cast<ushort>(us_tmp * cscale);
		if(bits_remaining < bits_per_component) {
			tuple4 <<= bits_remaining;
			bits_start = bits_per_component - bits_remaining;
			bits_remaining = 0;
			tuple4 |= (us_tmp >> bits_start);
		}
		else {
			tuple4 <<= bits_per_component;
			tuple4 |= us_tmp;
			bits_remaining -= bits_per_component;
		}
		// If this is last pixel in line, pad to nearest 8 bits. 
		i_element++;
		if(i_element == end_of_line) {
			ushort bit_align = (bits_remaining & 0x7);
			tuple4 <<= bit_align;
			bits_remaining -= bit_align;
			i_element = 0;
		}
		// Check if another 4-tuple is complete. 
		if(!bits_remaining) {
			if(p_gp->TPsB.P_Params->level1) {
				// A straight hex encoding for every 4 bits. 
				uchar tuple8[8];
				int i;
				for(i = 7; i >= 0; i--) {
					tuple8[i] = tuple4 & 0xf;
					tuple4 >>= 4;
				}
				for(i = 0; i < 8; i++) {
					sprintf(encoded_image_ptr++, "%1x", tuple8[i]);
					i_line--;
					if(!i_line) {
						i_line = ASCII_PER_LINE; *encoded_image_ptr++ = '\n';
					}
				}
			}
			else {
				/* Convert to ASCII85 representation. */
				if(tuple4) {
					int i;
					PS_encode85(tuple4, tuple5);
					tuple4 = 0;
					for(i = 0; i < 5; i++) {
						sprintf(encoded_image_ptr++, "%c", tuple5[i]+'!');
						i_line--;
						if(!i_line) {
							i_line = ASCII_PER_LINE; *encoded_image_ptr++ = '\n';
						}
					}
				}
				else {
					*encoded_image_ptr++ = 'z';
					i_line--;
					if(!i_line) {
						i_line = ASCII_PER_LINE; *encoded_image_ptr++ = '\n';
					}
				}
			}
			// Now pick up any bits that may have not made it into the 4-tuple. 
			if(bits_start) {
				tuple4 = us_tmp - ((us_tmp>>bits_start)<<bits_start);
			}
			bits_remaining = 32 - bits_start;
			bits_start = 0;
		}
		coord_remaining--;
	}
	if(bits_remaining < 32) {
		int i;
		int n = 4 - bits_remaining/8;
		if(p_gp->TPsB.P_Params->level1) {
			// A straight hex encoding for every 4 bits. 
			uchar tuple8[8];
			for(i = 2*n-1; i >= 0; i--) {
				tuple8[i] = tuple4 & 0xf;
				tuple4 >>= 4;
			}
			for(i = 0; i < 2*n; i++) {
				sprintf(encoded_image_ptr++, "%1x", tuple8[i]);
				i_line--;
				if(!i_line) {
					i_line = ASCII_PER_LINE; *encoded_image_ptr++ = '\n';
				}
			}
		}
		else {
			/* Convert to ASCII85 representation.
			 *
			 * The case where not all bytes in a tuple are used is slightly different.
			 * There is no use of 'z' as a special character and the remaining bytes
			 * need to be filled.  Then use only a portion of the final 5-tuple.
			 */
			tuple4 <<= bits_remaining;
			PS_encode85(tuple4, tuple5);
			// Write first n+1 bytes. 
			for(i = 0; i <= n; i++) {
				sprintf(encoded_image_ptr++, "%c", tuple5[i]+'!');
				i_line--;
				if(!i_line) {
					i_line = ASCII_PER_LINE; 
					*encoded_image_ptr++ = '\n';
				}
			}
		}
	}
	if(!p_gp->TPsB.P_Params->level1) {
		sprintf(encoded_image_ptr, "~>");
		encoded_image_ptr += 2;
	}
	*return_num_bytes = (encoded_image_ptr - encoded_image);
	assert(*return_num_bytes <= max_encoded_bytes);
	return encoded_image;
}

static void print_five_operand_image(GpTermEntry_Static * pThis, uint M, uint N, const gpiPoint * corner, t_imagecolor color_mode, ushort bits_per_component)
{
	GnuPlot * p_gp = pThis->P_Gp;
	char * space = p_gp->TPsB.P_Params->level1 ? "" : "  ";
	fprintf(GPT.P_GpPsFile, "%sgsave\n", space);
	if(p_gp->SmPltt.colorMode == SMPAL_COLOR_MODE_GRAY)
		fprintf(GPT.P_GpPsFile, "%s{pm3dGamma exp} settransfer\n", space);
	fprintf(GPT.P_GpPsFile, "%s%d %d translate\n", space, corner[0].x, corner[0].y);
	fprintf(GPT.P_GpPsFile, "%s%d %d scale\n", space, (corner[1].x - corner[0].x), (corner[1].y - corner[0].y));
	fprintf(GPT.P_GpPsFile, "%s%d %d %d\n", space, M, N, bits_per_component);
	fprintf(GPT.P_GpPsFile, "%s[ %d 0 0 %d 0 0 ]\n", space, M, N);
	if(p_gp->TPsB.P_Params->level1) {
		fprintf(GPT.P_GpPsFile, "/imagebuf %d string def\n", (M*N*bits_per_component*((color_mode == IC_RGB /* && p_gp->TPsB.P_Params->color */) ? 3 : 1) + 7)/8);
		fputs("{currentfile imagebuf readhexstring pop}\n", GPT.P_GpPsFile);
	}
	else if(p_gp->TPsB.P_Params->level3) {
		fprintf(GPT.P_GpPsFile, "  currentfile /ASCII85Decode filter << /Predictor 15 /BitsPerComponent %d /Colors %d /Columns %d  >> /FlateDecode filter\n",
		    bits_per_component, (color_mode == IC_RGB) ? 3 : 1, M);
	}
	else {
		fprintf(GPT.P_GpPsFile, "  currentfile /ASCII85Decode filter\n");
	}
	if(color_mode == IC_RGB /* && p_gp->TPsB.P_Params->color */) {
		fprintf(GPT.P_GpPsFile, "%sfalse 3\n" "%scolorimage\n", space, space);
	}
	else
		fprintf(GPT.P_GpPsFile, "%simage\n", space);
}

void PS_image(GpTermEntry_Static * pThis, uint M, uint N, coordval * image, const gpiPoint * corner, t_imagecolor color_mode)
{
	GnuPlot * p_gp = pThis->P_Gp;
	char * encoded_image;
	int num_encoded_bytes;
	ushort bits_per_component = 0;
	int max_colors, i_tmp;
	bool five_operand_image;
	double cscale;
#define DEFAULT_BITS_PER_COMPONENT 8
#define DEFAULT_COMPONENT_MAX (1<<DEFAULT_BITS_PER_COMPONENT)
	if(p_gp->SmPltt.UseMaxColors > 0)
		max_colors = p_gp->SmPltt.UseMaxColors;
	else
		max_colors = DEFAULT_COMPONENT_MAX;
	i_tmp = 1;
	while(i_tmp < max_colors) {
		bits_per_component++;
		i_tmp <<= 1;
	}
	if(bits_per_component < 1 || bits_per_component > 12) {
		fprintf(stderr, "GNUPLOT (post.trm):  Component bits (%d) out of range.\n", bits_per_component);
		return;
	}
	if(bits_per_component > 8)
		bits_per_component = 12;
	else if(bits_per_component > 4)
		bits_per_component = 8;
	else if(bits_per_component > 2)
		bits_per_component = 4;
	if(p_gp->TPsB.P_Params->level3 && bits_per_component > 8) {
		// Postscript would support 12 bits, but PNG does not 
		fprintf(stderr, "GNUPLOT (post.trm): Component bits (%d) out of range for level3 deflate filter.\n", bits_per_component);
		return;
	}
	// Color and gray scale images do not need a palette and can use
	// the 5 operand form of the image routine.
#if 0
	// 18.1.2009 It was decided to use the custom palette (i.e. colours) also
	// for the monochrome postscript output.
	if((color_mode == IC_RGB) || (p_gp->SmPltt.colorMode == SMPAL_COLOR_MODE_GRAY) || !p_gp->TPsB.P_Params->color)
#else
	if((color_mode == IC_RGB) || (p_gp->SmPltt.colorMode == SMPAL_COLOR_MODE_RGB && !p_gp->TPsB.P_Params->color) || (p_gp->SmPltt.colorMode == SMPAL_COLOR_MODE_GRAY))
#endif
		five_operand_image = TRUE;
	else
		five_operand_image = FALSE;
	// The five operand image doesn't have a palette and the values are
	// such that 0 maps to 0.0 and 2^bits_per_component - 1 maps to 1.0
	// in the PostScript driver.  Without any other knowledge, we scale
	// things so that our max colors corresponds to 1.0.
	if(five_operand_image)
		cscale =  (float)((1 << bits_per_component)-1) / (float)(max_colors-1);
	else
		cscale = 1.0;
#ifdef HAVE_DEFLATE_ENCODER
	if(p_gp->TPsB.P_Params->level3)
		encoded_image = (char *)PS_encode_png_image(pThis, M, N, image, color_mode, bits_per_component, max_colors, cscale, &num_encoded_bytes);
	else
#endif
	encoded_image = PS_encode_image(pThis, M, N, image, color_mode, bits_per_component, max_colors, cscale, (p_gp->TPsB.P_Params->level1 ? PS_ASCII_HEX : PS_ASCII85), &num_encoded_bytes);
	fputs("%%%%BeginImage\n", GPT.P_GpPsFile);
	// Clip image to requested bounding box 
	fprintf(GPT.P_GpPsFile, "gsave %d %d N %d %d L %d %d L %d %d L Z clip\n", corner[2].x, corner[2].y, corner[2].x, corner[3].y, corner[3].x, corner[3].y, corner[3].x, corner[2].y);
	/* Color and gray scale images do not need a palette and can use
	 * the 5 operand form of the image routine.  For other types of
	 * palettes, the 1 operand form of the image routine must be used
	 * and an indexed palette needs to be constructed.
	 */
	if(five_operand_image) {
		if(p_gp->TPsB.P_Params->level1) {
			print_five_operand_image(pThis, M, N, corner, color_mode, bits_per_component);
		}
		else {
			fputs("InterpretLevel1 ", GPT.P_GpPsFile);
			if(p_gp->TPsB.P_Params->level3)
				fputs(" InterpretLevel3 not or ", GPT.P_GpPsFile);
			fputs("{\n", GPT.P_GpPsFile);
			PS_skip_image(pThis, num_encoded_bytes, corner[0].x, corner[0].y, corner[1].x - corner[0].x, corner[1].y - corner[0].y);
			fputs("} {\n", GPT.P_GpPsFile);
			print_five_operand_image(pThis, M, N, corner, color_mode, bits_per_component);
			fputs("} ifelse\n", GPT.P_GpPsFile);
		}
	}
	else {
		int allocated;
		ushort i_tuple;
		double fact = 1.0 / (double)(max_colors-1);
		if(!p_gp->TPsB.P_Params->level1) {
			fputs("InterpretLevel1 ", GPT.P_GpPsFile);
			if(p_gp->TPsB.P_Params->level3)
				fputs(" InterpretLevel3 not or ", GPT.P_GpPsFile);
			fputs("{\n", GPT.P_GpPsFile);
			PS_skip_image(pThis, num_encoded_bytes, corner[0].x, corner[0].y, corner[1].x - corner[0].x, corner[1].y - corner[0].y);
			fputs("} {\n", GPT.P_GpPsFile);
		}
		fputs("gsave\n", GPT.P_GpPsFile);
		fprintf(GPT.P_GpPsFile, "%d %d translate\n", corner[0].x, corner[0].y);
		fprintf(GPT.P_GpPsFile, "%d %d scale\n", (corner[1].x - corner[0].x), (corner[1].y - corner[0].y));
		fputs("%%%%BeginPalette\n", GPT.P_GpPsFile);
		fprintf(GPT.P_GpPsFile, "[ /Indexed\n  /DeviceRGB %d\n  <", (max_colors-1));
#define TUPLES_PER_LINE 8
		for(allocated = 0, i_tuple = 0; allocated < max_colors; allocated++, i_tuple--) {
			double gray = (double)allocated * fact;
			rgb255_color color;
			p_gp->Rgb255MaxColorsFromGray(gray, &color);
			if(!i_tuple) {
				fprintf(GPT.P_GpPsFile, "\n  "); i_tuple = TUPLES_PER_LINE;
			}
			fprintf(GPT.P_GpPsFile, " %2.2x%2.2x%2.2x", (int)color.r, (int)color.g, (int)color.b);
		}
		fputs("\n  >\n] setcolorspace\n", GPT.P_GpPsFile);
		fputs("%%%%EndPalette\n", GPT.P_GpPsFile);
		fprintf(GPT.P_GpPsFile, "<<\n  /ImageType 1\n  /Width %d\n  /Height %d\n", M, N);
		fprintf(GPT.P_GpPsFile, "  /BitsPerComponent %d\n  /ImageMatrix [ %d 0 0 %d 0 0 ]\n", bits_per_component, M, N);
		fprintf(GPT.P_GpPsFile, "  /Decode [ 0 %d ]\n", ((1<<bits_per_component)-1));
		if(p_gp->TPsB.P_Params->level1) {
			fprintf(GPT.P_GpPsFile, "  /imagebuf %d string def\n", (M*N*bits_per_component + 7)/8);
			fputs("  /DataSource {currentfile imagebuf readhexstring pop}\n", GPT.P_GpPsFile);
		}
		else if(p_gp->TPsB.P_Params->level3) {
			fprintf(GPT.P_GpPsFile, "  /DataSource currentfile /ASCII85Decode filter ");
			fprintf(GPT.P_GpPsFile, "<< /Predictor 15 /BitsPerComponent %d /Colors %d /Columns %d >> /FlateDecode filter\n",
			    bits_per_component, (color_mode == IC_RGB) ? 3 : 1, M);
		}
		else {
			fputs("  /DataSource currentfile /ASCII85Decode filter\n", GPT.P_GpPsFile);
		}
		fputs("  /MultipleDataSources false\n", GPT.P_GpPsFile);
		fputs("  /Interpolate false\n>>\nimage\n", GPT.P_GpPsFile);
		if(!p_gp->TPsB.P_Params->level1)
			fputs("} ifelse\n", GPT.P_GpPsFile);
	}
	/* Send encoded image to file. */
	{
		char * encoded_image_ptr;
		for(i_tmp = 0, encoded_image_ptr = encoded_image; i_tmp < num_encoded_bytes; i_tmp++)
			fputc(*encoded_image_ptr++, GPT.P_GpPsFile);
	}
	if(p_gp->TPsB.P_Params->level1)
		fputs("\ngrestore\n", GPT.P_GpPsFile);
	else {
		fputs("\nInterpretLevel1 not ", GPT.P_GpPsFile);
		if(p_gp->TPsB.P_Params->level3)
			fputs("InterpretLevel3 and ", GPT.P_GpPsFile);
		fputs("{\n  grestore\n} if\n", GPT.P_GpPsFile);
	}
	fputs("grestore\n", GPT.P_GpPsFile);
	fputs("%%%%EndImage\n", GPT.P_GpPsFile);
	SAlloc::F(encoded_image);
	return;
}
//
// Skip the following image and draw a box instead. 
//
static void PS_skip_image(GpTermEntry_Static * pThis, int bytes, int x0, int y0, int dx, int dy) 
{
	GnuPlot * p_gp = pThis->P_Gp;
	fputs("  %% Construct a box instead of image\n  LTb\n", GPT.P_GpPsFile);
	fprintf(GPT.P_GpPsFile, "  %d %d M\n", x0, y0);
	fprintf(GPT.P_GpPsFile, "  %d 0 V\n", dx);
	fprintf(GPT.P_GpPsFile, "  0 %d V\n", dy);
	fprintf(GPT.P_GpPsFile, "  %d 0 V\n", -dx);
	fprintf(GPT.P_GpPsFile, "  %d %d L\n", x0, y0);
	fputs("  40 -110 R\n", GPT.P_GpPsFile);
	fprintf(GPT.P_GpPsFile, "  (PS level %d image) Lshow\n", p_gp->TPsB.P_Params->level3 ? 3 : 2);
	fputs("  % Read data but ignore it\n", GPT.P_GpPsFile);
	if(bytes > 65535) {
		/* this is the usual string length limit for Level 1 interpreters. */
		fputs("  /imagebuf 65535 string def\n", GPT.P_GpPsFile);
		fprintf(GPT.P_GpPsFile, "  /imagebuf_rest %d string def\n", bytes % 65535);
		fprintf(GPT.P_GpPsFile, "   1 1 %d { pop currentfile imagebuf readstring } for\n", bytes / 65535);
		fputs("  currentfile imagebuf_rest readstring\n", GPT.P_GpPsFile);
	}
	else {
		fprintf(GPT.P_GpPsFile, "  /imagebuf %d string def\n", bytes);
		fputs("  currentfile imagebuf readstring\n", GPT.P_GpPsFile);
	}
}
// 
// Feb 2010 - Search order for prolog and other files
// 1) current setting of "set psdir <dir>"
// 2) environmental variable GNUPLOT_PS_DIR
// 3) hard-coded path selected at build time
// 4) directories in "set loadpath <dirlist>"
// 
static FILE * PS_open_prologue_file(GpTermEntry_Static * pThis, char * name)
{
	GnuPlot * p_gp = pThis->P_Gp;
	char * fullname = NULL;
	char * ps_prologue_dir = 0;
	char * ps_prologue_env = 0;
	FILE * prologue_fd = 0;
	// Allocate and load default directory into ps_prologue_dir.  
	// FIXME:  Why should we have to recalculate this every time? 
#ifdef GNUPLOT_PS_DIR
#if defined(_WIN32)
	ps_prologue_dir = RelativePathToGnuplot(GNUPLOT_PS_DIR);
#else
	ps_prologue_dir = sstrdup(GNUPLOT_PS_DIR); // use hardcoded _absolute_ path 
#endif
#endif // system-dependent ps_prologue_dir 
	// First try current setting of "set psdir" 
	if(GPT.P_PS_PsDir) {
		fullname = (char *)SAlloc::M(strlen(GPT.P_PS_PsDir) + strlen(name) + 4);
		strcpy(fullname, GPT.P_PS_PsDir);
		PATH_CONCAT(fullname, name);
		prologue_fd = fopen(fullname, "r");
		SAlloc::F(fullname);
	}
	// Second try environmental variable GNUPLOT_PS_DIR 
	if(!prologue_fd && (ps_prologue_env = getenv("GNUPLOT_PS_DIR"))) {
		fullname = (char *)SAlloc::M(strlen(ps_prologue_env) + strlen(name) + 4);
		strcpy(fullname, ps_prologue_env);
		PATH_CONCAT(fullname, name);
		prologue_fd = fopen(fullname, "r");
		SAlloc::F(fullname);
	}
#ifndef GNUPLOT_PS_DIR
	// We should have a built-in copy of the headers 
	if(!prologue_fd) {
		PS_dump_header_to_file(pThis, name);
		return NULL;
	}
#endif
	// Third try system default directory 
	if(!prologue_fd) {
		fullname = (char *)SAlloc::M(strlen(ps_prologue_dir) + strlen(name) + 4);
		strcpy(fullname, ps_prologue_dir);
		PATH_CONCAT(fullname, name);
		prologue_fd = fopen(fullname, "r");
		SAlloc::F(fullname);
	}
	SAlloc::F(ps_prologue_dir);
	// Last-gasp effort: look in loadpath directories 
	SETIFZ(prologue_fd, p_gp->LoadPath_fopen(name, "r"));
	if(!prologue_fd) {
		fprintf(stderr, "Can't find PostScript prologue file %s\n", name);
		loadpath_handler(ACTION_SHOW, NULL);
		fprintf(stderr, "Please copy %s to one of the above directories\n", name);
		fprintf(stderr, "or set the environmental variable GNUPLOT_PS_DIR\n");
		fprintf(stderr, "or set the loadpath appropriately\n");
		p_gp->IntError(NO_CARET, "Plot failed!");
	}
	return prologue_fd;
}

#ifndef GNUPLOT_PS_DIR
static void PS_dump_header_to_file(GpTermEntry_Static * pThis, char * name)
{
	GnuPlot * p_gp = pThis->P_Gp;
	const char ** dump = NULL;
	int i;
	// load from included header 
	if(sstreq(name, "8859-15.ps"))
		dump = prologue_8859_15_ps;
	else if(sstreq(name, "8859-1.ps"))
		dump = prologue_8859_1_ps;
	else if(sstreq(name, "8859-2.ps"))
		dump = prologue_8859_2_ps;
	else if(sstreq(name, "8859-9.ps"))
		dump = prologue_8859_9_ps;
	else if(sstreq(name, "cp1250.ps"))
		dump = prologue_cp1250_ps;
	else if(sstreq(name, "cp1251.ps"))
		dump = prologue_cp1251_ps;
	else if(sstreq(name, "cp1252.ps"))
		dump = prologue_cp1252_ps;
	else if(sstreq(name, "cp437.ps"))
		dump = prologue_cp437_ps;
	else if(sstreq(name, "cp850.ps"))
		dump = prologue_cp850_ps;
	else if(sstreq(name, "cp852.ps"))
		dump = prologue_cp852_ps;
	else if(sstreq(name, "koi8r.ps"))
		dump = prologue_koi8r_ps;
	else if(sstreq(name, "koi8u.ps"))
		dump = prologue_koi8u_ps;
	else if(sstreq(name, "utf-8.ps"))
		dump = prologue_utf_8_ps;
	else if(sstreq(name, "prologue.ps"))
		dump = prologue_prologue_ps;
	else
		p_gp->IntWarn(NO_CARET, "Requested Postscript prologue %s not included in this build of gnuplot", name);
	if(dump) {
		for(i = 0; dump[i] != NULL; ++i)
			fprintf(GPT.P_GpPsFile, "%s", dump[i]);
	}
}
#endif

static void PS_dump_prologue_file(GpTermEntry_Static * pThis, char * name)
{
	char buf[256];
	FILE * prologue_fd = PS_open_prologue_file(pThis, name);
	if(prologue_fd) {
		while(fgets(buf, sizeof(buf), prologue_fd))
			fputs(buf, GPT.P_GpPsFile);
		fclose(prologue_fd);
	}
}

static void PS_load_glyphlist(GpTermEntry_Static * pThis)
{
	char buf[256];
	char * next = NULL;
	uint code;
	int len;
	char glyph_name[32];
	FILE * prologue_fd = PS_open_prologue_file(pThis, "aglfn.txt");
	if(prologue_fd) {
		while(fgets(buf, sizeof(buf), prologue_fd)) {
			if(*buf == '#' || *buf == '\n')
				continue;
			code = strtol(buf, &next, 16);
			/* User control over whether Adobe glyph names are used for unicode   */
			/* entries above 0x0100.  I.e. when we see a UTF-8 alpha, do we write */
			/* /alpha rather than /uni03B1?   Some fonts want one or the other.   */
			/* This is controlled by 'set term post adobeglyphnames'.             */
			if(code >= 0x0100 && !pThis->P_Gp->TPsB.P_Params->adobeglyphnames)
				continue;
			next++;
			len = strchr(next, ';') - next;
			strncpy(glyph_name, next, len);
			glyph_name[len] = '\0';
			FPRINTF((stderr, "%04X   %s\n", code, glyph_name));
			if((aglist_size + static_cast<int>(sizeof(ps_glyph))) > aglist_alloc) {
				aglist_alloc += 2048;
				aglist = (ps_glyph *)SAlloc::R(aglist, aglist_alloc);
			}
			aglist[psglyphs].unicode = code;
			aglist[psglyphs].glyphname = sstrdup(glyph_name);
			aglist_size += sizeof(ps_glyph);
			psglyphs++;
		}
		fclose(prologue_fd);
	}
}

void PS_path(GpTermEntry_Static * pThis, int p)
{
	switch(p) {
		case 0: // Start new path 
		    PsFlashPath(pThis);
		    PS_newpath = TRUE;
		    break;
		case 1: // Close path 
		    fprintf(GPT.P_GpPsFile, "Z ");
		    PsFlashPath(pThis);
		    break;
	}
}

TERM_PUBLIC void PS_layer(GpTermEntry_Static * pThis, t_termlayer syncpoint)
{
	static int plotno = 0;
	// We must ignore all syncpoints that we don't recognize 
	switch(syncpoint) {
		default: break;
		case TERM_LAYER_BEFORE_PLOT: fprintf(GPT.P_GpPsFile, "%% Begin plot #%d\n", ++plotno); break;
		case TERM_LAYER_AFTER_PLOT: fprintf(GPT.P_GpPsFile, "%% End plot #%d\n", plotno); break;
		case TERM_LAYER_BEGIN_PM3D_MAP: fprintf(GPT.P_GpPsFile, "%%pm3d_map_begin\n"); break;
		case TERM_LAYER_END_PM3D_MAP: fprintf(GPT.P_GpPsFile, "%%pm3d_map_end\n"); break;
		case TERM_LAYER_BEGIN_BORDER: PS_border = TRUE; break;
		case TERM_LAYER_END_BORDER: PS_border = FALSE; break;
		case TERM_LAYER_RESET: plotno = 0; break;
	}
}
//
// helper function for set_fontpath
// This used to be a much more complicated routine in misc.c that did
// a recursive search of subdirectories.  Simplify drastically for 5.3.
//
static char * fontpath_fullname(GpTermEntry_Static * pThis, const char * name, const char * dir)
{
	char * fullname = NULL;
	FILE * fp;
	if(!dir) {
		fullname = sstrdup(name);
	}
	else {
		fullname = (char *)SAlloc::M(strlen(dir) + strlen(name) + 4);
		sprintf(fullname, "%s%c%s", dir, DIRSEP1, name);
	}
	if((fp = fopen(fullname, "r"))) {
		// We're good 
		fclose(fp);
	}
	else if(!dir && (fp = pThis->P_Gp->LoadPath_fopen(fullname, "r"))) {
		// Found it in a loadpath directory 
		FREEANDASSIGN(fullname, sstrdup(loadpath_fontname));
		fclose(fp);
	}
	else {
		ZFREE(fullname);
	}
	return fullname;
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE

TERM_TABLE_START(post_driver)
	"postscript",
	"PostScript graphics, including EPSF embedded files (*.eps)",
	PS_XMAX, 
	PS_YMAX, 
	PS_VCHAR, 
	PS_HCHAR,
	PS_VTIC, 
	PS_HTIC, 
	PS_options, 
	PS_init, 
	PS_reset,
	PS_text, 
	GnuPlot::NullScale, 
	PS_graphics, 
	PS_move, 
	PS_vector,
	PS_linetype, 
	PS_put_text, 
	PS_text_angle,
	PS_justify_text, 
	PS_point, 
	GnuPlot::DoArrow, 
	PS_set_font, 
	PS_pointsize,
	TERM_BINARY|TERM_IS_POSTSCRIPT|TERM_CAN_CLIP|TERM_CAN_DASH|TERM_MONOCHROME|TERM_LINEWIDTH|TERM_FONTSCALE|TERM_POINTSCALE,
	0 /*suspend*/, 
	0 /*resume*/, 
	PS_fillbox, 
	PS_linewidth,
	#ifdef USE_MOUSE
	0, 
	0, 
	0, 
	0, 
	0,     /* no mouse support for postscript */
	#endif
	PS_make_palette,
	PS_previous_palette,     /* write grestore */
	PS_set_color,
	PS_filled_polygon,
	PS_image,
	ENHPS_OPEN, 
	ENHPS_FLUSH, 
	ENHPS_WRITEC,
	PS_layer, // used only to insert comments 
	PS_path,
	PS_SC, // terminal to pixel coord scale factor 
	NULL, // hypertext 
	ENHPS_boxed_text,
	NULL, // modify_plots 
	PS_dashtype
TERM_TABLE_END(post_driver)

#undef LAST_TERM
#define LAST_TERM post_driver

#endif /* TERM_TABLE */

#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
/* This is a pseudo help section that is labeled with 00psglobal to be
 * sure that it is sorted in before `post', `epslatex', and `pslatex'.
 * This section just defines commonly used text snippets for all three
 * help sections defined in this file. Defining PS_COMMON_OPTS1,
 * PS_COMMON_OPTS2, and PS_COMMON_DOC1 outside START_HELP()...END_HELP()
 * does not work.
 * The last line before the END_HELP(00psglobal) contains one single line
 * of "text" that is necessary to avoid errors.
 */
START_HELP(00psglobal)
#define PS_COMMON_OPTS1 \
	"                               {level1 | leveldefault | level3}", \
	"                               {color | colour | monochrome}", \
	"                               {background <rgbcolor> | nobackground}", \
	"                               {dashlength | dl <DL>}", \
	"                               {linewidth | lw <LW>} {pointscale | ps <PS>}", \
	"                               {rounded | butt}", \
	"                               {clip | noclip}", \
	"                               {palfuncparam <samples>{,<maxdeviation>}}", \
	"                               {size <XX>{unit},<YY>{unit}}",
#define PS_COMMON_OPTS2 \
	"                               {blacktext | colortext | colourtext}", \
	"                               {{font} \"fontname{,fontsize}\" {<fontsize>}}", \
	"                               {fontscale <scale>}",
#define PS_COMMON_PROLOG_INFO \
	" If you see the error message", \
	"       \"Can't find PostScript prologue file ... \"", \
	" Please see and follow the instructions in `postscript prologue`.", \
	"",
#define PS_COMMON_DOC1 \
	" The option `color` enables color, while `monochrome` prefers black and white", \
	" drawing elements. Further, `monochrome` uses gray `palette` but it does not", \
	" change color of objects specified with an explicit `colorspec`." \
	"", \
	" `dashlength` or `dl` scales the length of dashed-line segments by <DL>,", \
	" which is a floating-point number greater than zero.", \
	" `linewidth` or `lw` scales all linewidths by <LW>.", \
	"", \
	" By default the generated PostScript code uses language features that were", \
	" introduced in PostScript Level 2, notably filters and pattern-fill of", \
	" irregular objects such as filledcurves.  PostScript Level 2 features are", \
	" conditionally protected so that PostScript Level 1 interpreters do not issue", \
	" errors but, rather, display a message or a PostScript Level 1 approximation.", \
	" The `level1` option substitutes PostScript Level 1 approximations of these", \
	" features and uses no PostScript Level 2 code.  This may be required by some", \
	" old printers and old versions of Adobe Illustrator.  The flag `level1` can be", \
	" toggled later by editing a single line in the PostScript output file to force", \
	" PostScript Level 1 interpretation.  In the case of files containing level 2", \
	" code, the above features will not appear or will be replaced by a note when", \
	" this flag is set or when the interpreting program does not indicate that it", \
	" understands level 2 PostScript or higher. The flag `level3` enables PNG", \
	" encoding for bitmapped images, which can reduce the output size considerably.", \
	"", \
	" `rounded` sets line caps and line joins to be rounded; `butt` is the", \
	" default, butt caps and mitered joins.", \
	"", \
	" `clip` tells PostScript to clip all output to the bounding box;", \
	" `noclip` is the default.", \
	"", \
	" `palfuncparam` controls how `set palette functions` are encoded as gradients", \
	" in the output. Analytic color component functions (set via", \
	" `set palette functions`) are encoded as linear interpolated gradients in the", \
	" postscript output:  The color component functions are sampled at <samples>", \
	" points and all points are removed from this gradient which can be removed", \
	" without changing the resulting colors by more than <maxdeviation>. For", \
	" almost every useful palette you may safely leave the defaults of", \
	" <samples>=2000 and <maxdeviation>=0.003 untouched.", \
	"", \
	" The default size for postscript output is 10 inches x 7 inches. The default", \
	" for eps output is 5 x 3.5 inches.  The `size` option changes this to", \
	" whatever the user requests. By default the X and Y sizes are taken to be in", \
	" inches, but other units are possibly (currently only cm). The BoundingBox", \
	" of the plot is correctly adjusted to contain the resized image.", \
	" Screen coordinates always run from 0.0 to 1.0 along the full length of the", \
	" plot edges as specified by the `size` option.", \
	" NB: `this is a change from the previously recommended method of using the", \
	" set size command prior to setting the terminal type`.  The old method left", \
	" the BoundingBox unchanged and screen coordinates did not correspond to the", \
	" actual limits of the plot.", \
	"",
""
END_HELP(00psglobal)

START_HELP(epslatex)
"1 epslatex",
"?commands set terminal epslatex",
"?set terminal epslatex",
"?set term epslatex",
"?terminal epslatex",
"?term epslatex",
"?epslatex",
" The `epslatex` driver generates output for further processing by LaTeX.",
"",
" Syntax:",
"       set terminal epslatex   {default}",
"       set terminal epslatex   {standalone | input}",
"                               {oldstyle | newstyle}",
PS_COMMON_OPTS1
"                               {header <header> | noheader}",
PS_COMMON_OPTS2
"",
" The epslatex terminal prints a plot as `terminal postscript eps`",
" but transfers the texts to LaTeX instead of including in the PostScript",
" code. Thus, many options are the same as in the `postscript terminal`.",
"",
" The appearance of the epslatex terminal changed between versions 4.0 and 4.2",
" to reach better consistency with the postscript terminal:",
" The plot size has been changed from 5 x 3 inches to 5 x 3.5 inches;",
" the character width is now estimated to be 60% of the font size",
" while the old epslatex terminal used 50%;  now, the larger number of",
" postscript linetypes and symbols are used.  To reach an appearance that is",
" nearly identical to the old one specify the option `oldstyle`. (In fact",
" some small differences remain: the symbol sizes are slightly different, the",
" tics are half as large as in the old terminal which can be changed using",
" `set tics scale`, and the arrows have all features as in the postscript",
" terminal.)",
"",
PS_COMMON_PROLOG_INFO
PS_COMMON_DOC1
" `blacktext` forces all text to be written in black even in color mode;",
"",
" The epslatex driver offers a special way of controlling text positioning:",
" (a) If any text string begins with '{', you also need to include a '}' at the",
" end of the text, and the whole text will be centered both horizontally",
" and vertically by LaTeX.  (b) If the text string begins with '[', you need",
" to continue it with: a position specification (up to two out of t,b,l,r,c),",
" ']{', the text itself, and finally, '}'. The text itself may be anything",
" LaTeX can typeset as an LR-box. \\rule{}{}'s may help for best positioning.",
" See also the documentation for the `pslatex` terminal driver.",
" To create multiline labels, use \\shortstack, for example",
"    set ylabel '[r]{\\shortstack{first line \\\\ second line}}' ",
"",
" The `back` option of `set label` commands is handled slightly different",
" than in other terminals. Labels using 'back' are printed behind all other",
" elements of the plot while labels using 'front' are printed above ",
" everything else.",
"",
" The driver produces two different files, one for the eps part of the figure",
" and one for the LaTeX part. The name of the LaTeX file is taken from the",
" `set output` command. The name of the eps file is derived by replacing",
" the file extension (normally `.tex`) with `.eps` instead.  There is no",
" LaTeX output if no output file is given!  Remember to close the",
" `output file` before next plot unless in `multiplot` mode.",
"",
" In your LaTeX documents use '\\input{filename}' to include the figure.",
" The `.eps` file is included by the command \\includegraphics{...}, so you",
" must also include \\usepackage{graphicx} in the LaTeX preamble.  If you",
" want to use coloured text (option `textcolour`) you also have to include",
" \\usepackage{color} in the LaTeX preamble.",
"",
" Pdf files can be made from the eps file using 'epstopdf'. If the graphics",
" package is properly configured, the LaTeX files can also be processed by",
" pdflatex without changes, using the pdf files instead of the eps files."
"",
" The behaviour concerning font selection depends on the header mode.",
" In all cases, the given font size is used for the calculation of proper",
" spacing. When not using the `standalone` mode the actual LaTeX font and",
" font size at the point of inclusion is taken, so use LaTeX commands for",
" changing fonts. If you use e.g. 12pt as font size for your LaTeX",
" document, use '\"\" 12' as options. The font name is ignored. If using",
" `standalone` the given font and font size are used, see below for a",
" detailed description.",
"",
" If text is printed coloured is controlled by the TeX booleans \\ifGPcolor",
" and \\ifGPblacktext. Only if \\ifGPcolor is true and \\ifGPblacktext is",
" false, text is printed coloured. You may either change them in the",
" generated TeX file or provide them globally in your TeX file, for example",
" by using",
"    \\newif\\ifGPblacktext",
"    \\GPblacktexttrue",
" in the preamble of your document. The local assignment is only done if no",
" global value is given.",
"",
" When using the epslatex terminal give the name of the TeX file in the",
" `set output` command including the file extension (normally \".tex\").",
" The eps filename is generated by replacing the extension by \".eps\".",
"",
" If using the `standalone` mode a complete LaTeX header is added to the",
" LaTeX file; and \"-inc\" is added to the filename of the eps file.",
" The `standalone` mode generates a TeX file that produces",
" output with the correct size when using dvips, pdfTeX, or VTeX.",
" The default, `input`, generates a file that has to be included into a",
" LaTeX document using the \\input command.",
"",
" If a font other than \"\" or \"default\" is given it is interpreted as",
" LaTeX font name.  It contains up to three parts, separated by a comma:",
" 'fontname,fontseries,fontshape'.  If the default fontshape or fontseries",
" are requested, they can be omitted.  Thus, the real syntax for the fontname",
" is '[fontname][,fontseries][,fontshape]'.  The naming convention for all",
" parts is given by the LaTeX font scheme.  The fontname is 3 to 4 characters",
" long and is built as follows: One character for the font vendor, two",
" characters for the name of the font, and optionally one additional",
" character for special fonts, e.g., 'j' for fonts with old-style numerals",
" or 'x' for expert fonts. The names of many fonts is described in",
"^ <a href=\"http://www.tug.org/fontname/fontname.pdf\">",
"           http://www.tug.org/fontname/fontname.pdf",
"^ </a>",
" For example, 'cmr' stands for Computer Modern Roman, 'ptm' for Times-Roman,",
" and 'phv' for Helvetica.  The font series denotes the thickness of the",
" glyphs, in most cases 'm' for normal (\"medium\") and 'bx' or 'b' for bold",
" fonts.  The font shape is 'n' for upright, 'it' for italics, 'sl' for",
" slanted, or 'sc' for small caps, in general.  Some fonts may provide",
" different font series or shapes.",
"",
" Examples:",
"",
" Use Times-Roman boldface (with the same shape as in the surrounding text):",
"       set terminal epslatex 'ptm,bx'",
" Use Helvetica, boldface, italics:",
"       set terminal epslatex 'phv,bx,it'",
" Continue to use the surrounding font in slanted shape:",
"       set terminal epslatex ',,sl'",
" Use small capitals:",
"       set terminal epslatex ',,sc'",
"",
" By this method, only text fonts are changed. If you also want to change",
" the math fonts you have to use the \"gnuplot.cfg\" file or the `header`",
" option, described below.",
"",
" In standalone mode, the font size is taken from the given font size in the",
" `set terminal` command. To be able to use a specified font size, a file",
" \"size<size>.clo\" has to reside in the LaTeX search path.  By default,",
" 10pt, 11pt, and 12pt are supported.  If the package \"extsizes\" is",
" installed, 8pt, 9pt, 14pt, 17pt, and 20pt are added.",
"",
" The `header` option takes a string as argument.  This string is written",
" into the generated LaTeX file.  If using the `standalone` mode, it is ",
" written into the preamble, directly before the \\begin{document} command.",
" In the `input` mode, it is placed directly after the \\begingroup command",
" to ensure that all settings are local to the plot.",
"",
" Examples:",
"",
" Use T1 fontencoding, change the text and math font to Times-Roman as well",
" as the sans-serif font to Helvetica:",
"     set terminal epslatex standalone header \\",
"     \"\\\\usepackage[T1]{fontenc}\\n\\\\usepackage{mathptmx}\\n\\\\usepackage{helvet}\"",
" Use a boldface font in the plot, not influencing the text outside the plot:",
"     set terminal epslatex input header \"\\\\bfseries\"",
"",
" If the file \"gnuplot.cfg\" is found by LaTeX it is input in the preamble",
" the LaTeX document, when using `standalone` mode.  It can be used for",
" further settings, e.g., changing the document font to Times-Roman,",
" Helvetica, and Courier, including math fonts (handled by \"mathptmx.sty\"):",
"       \\usepackage{mathptmx}",
"       \\usepackage[scaled=0.92]{helvet}",
"       \\usepackage{courier}",
" The file \"gnuplot.cfg\" is loaded before the header information given",
" by the `header` command.  Thus, you can use `header` to overwrite some of",
" settings performed using \"gnuplot.cfg\"",
""
END_HELP(epslatex)

START_HELP(pslatex)
"1 pslatex and pstex",
"?commands set terminal pslatex",
"?set terminal pslatex",
"?set term pslatex",
"?terminal pslatex",
"?term pslatex",
"?pslatex",
"?commands set terminal pstex",
"?set terminal pstex",
"?set term pstex",
"?terminal pstex",
"?term pstex",
"?pstex",
" The `pslatex` driver generates output for further processing by LaTeX,",
" while the `pstex` driver generates output for further processing by",
" TeX. `pslatex` uses \\specials understandable by dvips and xdvi. Figures",
" generated by `pstex` can be included in any plain-based format (including",
" LaTeX).",
"",
" Syntax:",
"       set terminal [pslatex | pstex] {default}",
"       set terminal [pslatex | pstex]",
"                               {rotate | norotate}",
"                               {oldstyle | newstyle}",
"                               {auxfile | noauxfile}",
PS_COMMON_OPTS1
"                               {<font_size>}",
"",
PS_COMMON_PROLOG_INFO
PS_COMMON_DOC1
" if `rotate` is specified, the y-axis label is rotated.",
" <font_size> is the size (in pts) of the desired font.",
"",
" If `auxfile` is specified, it directs the driver to put the PostScript",
" commands into an auxiliary file instead of directly into the LaTeX file.",
" This is useful if your pictures are large enough that dvips cannot handle",
" them.  The name of the auxiliary PostScript file is derived from the name of",
" the TeX file given on the `set output` command; it is determined by replacing",
" the trailing `.tex` (actually just the final extent in the file name) with",
" `.ps` in the output file name, or, if the TeX file has no extension, `.ps`",
" is appended.  The `.ps` is included into the `.tex` file by a",
" \\special{psfile=...} command.  Remember to close the `output file` before",
" next plot unless in `multiplot` mode.",
"",
" Gnuplot versions prior to version 4.2 generated plots of the size",
" 5 x 3 inches using the ps(la)tex terminal while the current version generates",
" 5 x 3.5 inches to be consistent with the postscript eps terminal.  In",
" addition, the character width is now estimated to be 60% of the font size",
" while the old epslatex terminal used 50%. To reach the old format specify",
" the option `oldstyle`.",
"",
" The pslatex driver offers a special way of controlling text positioning: ",
" (a) If any text string begins with '{', you also need to include a '}' at the",
" end of the text, and the whole text will be centered both horizontally",
" and vertically by LaTeX.  (b) If the text string begins with '[', you need",
" to continue it with: a position specification (up to two out of t,b,l,r),",
" ']{', the text itself, and finally, '}'. The text itself may be anything",
" LaTeX can typeset as an LR-box. \\rule{}{}'s may help for best positioning.",
"",
" The options not described here are identical to the `Postscript terminal`.",
" Look there if you want to know what they do.",
"",
" Examples:",
"       set term pslatex monochrome rotate       # set to defaults",
" To write the PostScript commands into the file \"foo.ps\":",
"       set term pslatex auxfile",
"       set output \"foo.tex\"; plot ...; set output",
" About label positioning:",
" Use gnuplot defaults (mostly sensible, but sometimes not really best):",
"        set title '\\LaTeX\\ -- $ \\gamma $'",
" Force centering both horizontally and vertically:",
"        set label '{\\LaTeX\\ -- $ \\gamma $}' at 0,0",
" Specify own positioning (top here):",
"        set xlabel '[t]{\\LaTeX\\ -- $ \\gamma $}'",
" The other label -- account for long ticlabels:",
"        set ylabel '[r]{\\LaTeX\\ -- $ \\gamma $\\rule{7mm}{0pt}}'",
"",
" Linewidths and pointsizes may be changed with `set style line`."
""
END_HELP(pslatex)

START_HELP(post)
"1 postscript",
"?commands set terminal postscript",
"?set terminal postscript",
"?set term postscript",
"?terminal postscript",
"?term postscript",
"?postscript",
" Several options may be set in the `postscript` driver.",
"",
" Syntax:",
"       set terminal postscript {default}",
"       set terminal postscript {landscape | portrait | eps}",
"                               {enhanced | noenhanced}",
"                               {defaultplex | simplex | duplex}",
"                               {fontfile {add | delete} \"<filename>\"",
"                                | nofontfiles} {{no}adobeglyphnames}",
PS_COMMON_OPTS1
PS_COMMON_OPTS2
PS_COMMON_PROLOG_INFO
"",
" `landscape` and `portrait` choose the plot orientation.",
" `eps` mode generates EPS (Encapsulated PostScript) output, which is just",
" regular PostScript with some additional lines that allow the file to be",
" imported into a variety of other applications.  (The added lines are",
" PostScript comment lines, so the file may still be printed by itself.)  To",
" get EPS output, use the `eps` mode and make only one plot per file.  In `eps`",
" mode the whole plot, including the fonts, is reduced to half of the default",
" size.",
"",
" `enhanced` enables enhanced text mode features (subscripts,",
" superscripts and mixed fonts). See `enhanced` for more information.",
" `blacktext` forces all text to be written in black even in color mode;",
"",
" Duplexing in PostScript is the ability of the printer to print on both",
" sides of the same sheet of paper.  With `defaultplex`, the default setting",
" of the printer is used; with `simplex` only one side is printed; `duplex`",
" prints on both sides (ignored if your printer can't do it).",
"",
" `\"<fontname>\"` is the name of a valid PostScript font; and `<fontsize>` is",
" the size of the font in PostScript points.",
" In addition to the standard postscript fonts, an oblique version of the",
" Symbol font, useful for mathematics, is defined. It is called",
" \"Symbol-Oblique\".",
"",
" `default` sets all options to their defaults: `landscape`, `monochrome`,",
" `dl 1.0`, `lw 1.0`, `defaultplex`, `enhanced`, \"Helvetica\" and",
" 14pt.  Default size of a PostScript plot is 10 inches wide and 7 inches high.",
PS_COMMON_DOC1
" Fonts listed by `fontfile` or `fontfile add` encapsulate the font",
" definitions of the listed font from a postscript Type 1 or TrueType font",
" file directly into the gnuplot output postscript file.  Thus, the enclosed",
" font can be used in labels, titles, etc.  See the section",
" `postscript fontfile` for more details.  With `fontfile delete`, a fontfile",
" is deleted from the list of embedded files.  `nofontfiles` cleans the list",
" of embedded fonts.",
"",
" Examples:",
"       set terminal postscript default       # old postscript",
"       set terminal postscript enhanced      # old enhpost",
"       set terminal postscript landscape 22  # old psbig",
"       set terminal postscript eps 14        # old epsf1",
"       set terminal postscript eps 22        # old epsf2",
"       set size 0.7,1.4; set term post portrait color \"Times-Roman\" 14",
"       set term post \"VAGRoundedBT_Regular\" 14 fontfile \"bvrr8a.pfa\"",
"",
" Linewidths and pointsizes may be changed with `set style line`.",
"",
" The `postscript` driver supports about 70 distinct pointtypes, selectable",
" through the `pointtype` option on `plot` and `set style line`.",
"",
" Several possibly useful files about `gnuplot`'s PostScript are included",
" in the /docs/psdoc subdirectory of the `gnuplot` distribution and at the",
" distribution sites.  These are \"ps_symbols.gpi\" (a `gnuplot` command file",
" that, when executed, creates the file \"ps_symbols.ps\" which shows all the",
" symbols available through the `postscript` terminal), \"ps_guide.ps\" (a",
" PostScript file that contains a summary of the enhanced syntax and a page",
" showing what the octal codes produce with text and symbol fonts),",
" \"ps_file.doc\" (a text file that contains a discussion of the organization",
" of a PostScript file written by `gnuplot`), and \"ps_fontfile_doc.tex\"",
" (a LaTeX file which contains a short documentation concerning the",
" encapsulation of LaTeX fonts with a glyph table of the math fonts).",
"",
" A PostScript file is editable, so once `gnuplot` has created one, you are",
" free to modify it to your heart's desire.  See the `editing postscript`",
" section for some hints.",
"2 editing postscript",
"?commands set terminal postscript editing",
"?set terminal postscript editing",
"?set term postscript editing",
"?terminal postscript editing",
"?term postscript editing",
"?editing_postscript",
"?editing postscript",
" The PostScript language is a very complex language---far too complex to",
" describe in any detail in this document.  Nevertheless there are some things",
" in a PostScript file written by `gnuplot` that can be changed without risk of",
" introducing fatal errors into the file.",
"",
" For example, the PostScript statement \"/Color true def\" (written into the",
" file in response to the command `set terminal postscript color`), may be",
" altered in an obvious way to generate a black-and-white version of a plot.",
" Similarly line colors, text colors, line weights and symbol sizes can also be",
" altered in straight-forward ways.  Text (titles and labels) can be edited to",
" correct misspellings or to change fonts.  Anything can be repositioned, and",
" of course anything can be added or deleted, but modifications such as these",
" may require deeper knowledge of the PostScript language.",
"",
" The organization of a PostScript file written by `gnuplot` is discussed in",
" the text file \"ps_file.doc\" in the docs/ps subdirectory of the gnuplot",
" source distribution.",
"2 postscript fontfile",
"?commands set terminal postscript fontfile",
"?set terminal postscript fontfile",
"?set term postscript fontfile",
"?terminal postscript fontfile",
"?term postscript fontfile",
"?postscript fontfile",
"?fontfile",
"       set term postscript ... fontfile {add|delete} <filename>",
" The `fontfile` or `fontfile add` option takes one file name as argument",
" and encapsulates this file into the postscript output in order to make",
" this font available for text elements (labels, tic marks, titles, etc.).",
" The `fontfile delete` option also takes one file name as argument. It",
" deletes this file name from the list of encapsulated files.",
"",
" The postscript terminal understands some",
" font file formats: Type 1 fonts in ASCII file format (extension \".pfa\"),",
" Type 1 fonts in binary file format (extension \".pfb\"), and TrueType",
" fonts (extension \".ttf\"). pfa files are understood directly, pfb and ttf",
" files are converted on the fly if appropriate conversion tools are",
" installed (see below). You have to specify the full filename including the",
" extension. Each `fontfile` option takes exact one font file name. This",
" option can be used multiple times in order to include more than one font",
" file.",
"",
" The search order used to find font files is",
" (1) absolute pathname or current working directory",
" (2) any of the directories specified by `set loadpath`",
" (3) the directory specified by `set fontpath`",
" (4) the directory given in environmental variable GNUPLOT_FONTPATH.",
" NB: This is a CHANGE from earlier versions of gnuplot.",
"",
" For using the encapsulated font file you have to specify the font name",
" (which normally is not the same as the file name). When embedding a",
" font file by using the `fontfile` option in interactive mode, the ",
" font name is printed on the screen. E.g.",
"    Font file 'p052004l.pfb' contains the font 'URWPalladioL-Bold'. Location:",
"    /usr/lib/X11/fonts/URW/p052004l.pfb",
"",
" When using pfa or pfb fonts, you can also find it out by looking into the",
" font file. There is a line similar to \"/FontName /URWPalladioL-Bold def\".",
" The middle string without the slash is the fontname, here",
" \"URWPalladioL-Bold\".",
" For TrueType fonts, this is not so easy since the font name is stored in a",
" binary format. In addition, they often have spaces in the font names which",
" is not supported by Type 1 fonts (in which a TrueType is converted on the",
" fly). The font names are changed in order to eliminate the spaces in the",
" fontnames. The easiest way to find out which font name is generated for",
" use with gnuplot, start gnuplot in interactive mode and type in",
" \"set terminal postscript fontfile '<filename.ttf>'\".",
"",
" For converting font files (either ttf or pfb) to pfa format, the conversion",
" tool has to read the font from a file and write it to standard output. If",
" the output cannot be written to standard output, on-the-fly conversion is",
" not possible.",
"",
" For pfb files \"pfbtops\" is a tool which can do this. If this program",
" is installed on your system the on the fly conversion should work.",
" Just try to encapsulate a pfb file. If the compiled in program call does",
" not work correctly you can specify how this program is called by",
" defining the environment variable GNUPLOT_PFBTOPFA e.g. to",
" \"pfbtops %s\". The `%s` will be replaced by the font file name and thus",
" has to exist in the string.",
"",
" If you don't want to do the conversion on the fly but get a pfa file of",
" the font you can use the tool \"pfb2pfa\" which is written in simple c",
" and should compile with any c compiler.",
" It is available from many ftp servers, e.g.",
"^ <a href=\"ftp://ftp.dante.de/tex-archive/fonts/utilities/ps2mf/\">",
"           ftp://ftp.dante.de/tex-archive/fonts/utilities/ps2mf/",
"^ </a>",
" In fact, \"pfbtopfa\" and \"pfb2ps\" do the same job. \"pfbtopfa\" puts",
" the resulting pfa code into a file, whereas \"pfbtops\" writes it to",
" standard output.",
"",
" TrueType fonts are converted into Type 1 pfa format, e.g.",
" by using the tool \"ttf2pt1\" which is available from",
"^ <a href=\"http://ttf2pt1.sourceforge.net/\">",
"           http://ttf2pt1.sourceforge.net/",
"^ </a>",
" If the builtin conversion does not",
" work, the conversion command can be changed by the environment variable",
" GNUPLOT_TTFTOPFA. For usage with ttf2pt1 it may be set to",
" \"ttf2pt1 -a -e -W 0 %s - \". Here again, `%s` stands for the",
" file name.",
"",
" For special purposes you also can use a pipe (if available for your",
" operating system). Therefore you start the file name definition with ",
" the character \"<\" and append a program call. This program has ",
" to write pfa data to standard output. Thus, a pfa file may be accessed",
" by `set fontfile \"< cat garamond.pfa\"`.",
"",
" For example, including Type 1 font files can be used for including the",
" postscript output in LaTeX documents. The \"european computer modern\"",
" font (which is a variant of the \"computer modern\" font) is available",
" in pfb format from any CTAN server, e.g.",
"^ <a href=\"ftp://ftp.dante.de/tex-archive/fonts/ps-type1/cm-super/\">",
"           ftp://ftp.dante.de/tex-archive/fonts/ps-type1/cm-super/",
"^ </a>",
" For example, the file \"sfrm1000.pfb\" contains the normal upright fonts",
" with serifs in the design size 10pt (font name \"SFRM1000\").",
" The computer modern fonts, which are still necessary for mathematics,",
" are available from",
"^ <a href=\"ftp://ftp.dante.de/tex-archive/fonts/cm/ps-type1/bluesky\">",
"           ftp://ftp.dante.de/tex-archive/fonts/cm/ps-type1/bluesky",
"^ </a>",
" With these you can use any character available in TeX. However, the",
" computer modern fonts have a strange encoding. (This is why you should not",
" use cmr10.pfb for text, but sfrm1000.pfb instead.)",
" The usage of TeX fonts is shown in one of the demos.",
" The file \"ps_fontfile_doc.tex\" in the /docs/psdoc subdirectory of the",
" `gnuplot` source distribution contains a table with glyphs of the TeX",
" mathfonts.",
"",
" If the font \"CMEX10\" is embedded (file \"cmex10.pfb\") gnuplot defines",
" the additional font \"CMEX10-Baseline\". It is shifted vertically in order",
" to fit better to the other glyphs (CMEX10 has its baseline at the top of",
" the symbols).",
"2 postscript prologue",
"?commands set terminal postscript prologue",
"?set terminal postscript prologue",
"?terminal postscript prologue",
"?postscript prologue",
"?prologue",
" Each PostScript output file includes a %%Prolog section and possibly some",
" additional user-defined sections containing, for example, character encodings.",
" These sections are copied from a set of PostScript prologue files that are",
" either compiled into the gnuplot executable or stored elsewhere on your",
" computer. A default directory where these files live is set at the time",
" gnuplot is built. However, you can override this default either by using the",
" gnuplot command `set psdir` or by defining an environment variable",
" GNUPLOT_PS_DIR. See `set psdir`.",
"2 postscript adobeglyphnames",
"?commands set terminal postscript adobeglyphnames",
"?set terminal postscript adobeglyphnames",
"?terminal postscript adobeglyphnames",
"?postscript adobeglyphnames",
"?adobeglyphnames",
"=UTF-8",
" This setting is only relevant to PostScript output with UTF-8 encoding.",
" It controls the names used to describe characters with Unicode entry points",
" higher than 0x00FF.  That is, all characters outside of the Latin1 set.",
" In general unicode characters do not have a unique name; they have only a",
" unicode identification code.  However, Adobe have a recommended scheme for",
" assigning names to certain ranges of characters (extended Latin, Greek, etc).",
" Some fonts use this scheme, others do not.  By default, gnuplot will use",
" the Adobe glyph names.  E.g. the lower case Greek letter alpha will be called",
" /alpha.  If you specific `noadobeglyphnames` then instead gnuplot will use",
" /uni03B1 to describe this character.  If you get this setting wrong, the",
" character may not be found even if it is present in the font.",
" It is probably always correct to use the default for Adobe fonts, but for",
" other fonts you may have to try both settings.  See also `fontfile`.",
"",
""
END_HELP(post)
#endif /* TERM_HELP */
