// GNUPLOT - pslatex.trm 
// Copyright 1990 - 1993, 1998, 2004
//
/*
 * This file is included by ../term.c.
 *
 * This file supplies the terminal drivers:
 *     pslatex  -- LaTeX with embedded postscript
 *     pstex    -- plain TeX with embedded postscript
 *     epslatex -- LaTeX using \includegraphics, postscript part in an
 *           external file
 * Some routines are also used by terminal drivers
 *     cairolatex
 *
 * AUTHORS: George Phillips, Russell Lang, David Kotz
 *  Petr Mikulik, May 2000: terminal entries for PM3D functionality
 *  Dan Sebald, 5 March 2003: terminal entry for image functionality
 *  Theo Hopman
 *      23 May 2003:
 *        - added epslatex support. Replaces epslatex.trm; pointtype
 *        and linetypes for epslatex terminal are now identical to
 *        those of pslatex terminal.
 *        - added arbitrary text rotations to all [e]ps[la]tex
 *        terminals.
 *      27 May 2004:
 *        - epslatex patch updated for gnuplot 4.0
 *
 *  Harald Harders (h.harders@tu-bs.de), 2005-02-08:
 *  - Merged functionality of postscript, pslatex, pstex, and
 *    epslatex terminals. Therefore deleted epslatex.trm.
 *  - Added a standalone mode to the epslatex mode for standalone use without
 *    an additional LaTeX document.
 *  - Fixed text rotation of ps(la)tex terminals.
 *
 * Sep 2019
 *  - use the same page size in \includegraphics statement on terminal close
 *    that was used for layout when the terminal was initialized.
 *
 * Send your comments or suggestions to (gnuplot-info@lists.sourceforge.net).
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
	register_term(pslatex)
	register_term(pstex)
	register_term(epslatex)
#endif
//#ifdef TERM_PROTO
// Common functions for epslatex and ps(la)tex 
// All these routines begin with PSLATEX_ 
extern void PSLATEX_reset(GpTermEntry_Static * pThis);
// Functions for ps(la)tex 
// All these routines begin with PSTEX_ 
//TERM_PUBLIC void PSTEX_reopen_output();
//TERM_PUBLIC void PSTEX_common_init();
TERM_PUBLIC void PSTEX_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str);
TERM_PUBLIC void PSTEX_text(GpTermEntry_Static * pThis);
// Functions for epslatex 
// All these routines begin with EPSLATEX_ 
// void EPSLATEX_reopen_output(char *);
//TERM_PUBLIC void EPSLATEX_common_init();
char * epslatex_header = NULL; // additional LaTeX header information for epslatex terminal 
//#endif /* TERM_PROTO */

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY

#include "post.h"

struct pstex_text_command {
	int    x;
	int    y;
	int    angle;
	int    justify;
	char * label;
	pstex_text_command * next;
};

static pstex_text_command * pstex_labels = NULL;
static int epslatex_text_layer = 0;

// Support for optimization of set_color 
static t_colorspec tex_previous_colorspec = {(colortype)-1, 0, 0.0}; /* Initialize to invalid type */
static char tex_current_color[64];
static char tex_rgb_colordef[64];
static bool tex_color_synced = FALSE;

// support for cairolatex 
#ifdef HAVE_CAIROPDF
	#define ISCAIROTERMINAL ((strcmp(GPT.P_Term->name, "cairolatex") == 0))
	//extern void cairotrm_set_color(GpTermEntry_Static * pThis, const t_colorspec * colorspec);
	//TERM_PUBLIC void cairotrm_linetype(int lt);
#else
	#define ISCAIROTERMINAL (FALSE)
#endif
//
// State variables for boxed text 
//
static bool PSLATEX_inbox = FALSE;
static bool PSLATEX_saved = FALSE;
static int PSLATEX_xbox, PSLATEX_ybox;
static double PSLATEX_xmargin = 1.0;
static double PSLATEX_ymargin = 1.0;
double PSLATEX_opacity = 1.0;
//
// Fix for "set size" different at the time terminal is opened/closed 
//
static double PSLATEX_pagesize_x;
static double PSLATEX_pagesize_y;
//
// Common functions for epslatex and ps(la)tex 
//
void PSLATEX_reset(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	switch(p_gp->TPsB.P_Params->terminal) {
		case PSTERM_EPSLATEX:
		    if(!ISCAIROTERMINAL)
			    PS_reset(pThis);
		    if(GPT.P_GpOutFile) {
			    fprintf(GPT.P_GpOutFile,
				"\
    \\gplbacktext\n\
    \\put(0,0){\\includegraphics[width={%.2fbp},height={%.2fbp}]{%s}}%%\n\
    \\gplfronttext\n\
  \\end{picture}%%\n\
\\endgroup\n", PSLATEX_pagesize_x / (2.0*PS_SC), PSLATEX_pagesize_y / (2.0*PS_SC), p_gp->TPsB.PsLatexAuxname);
			    if(p_gp->TPsB.P_Params->epslatex_standalone)
				    fputs("\\end{document}\n", GPT.P_GpOutFile);
		    }
		    break;
		case PSTERM_PSLATEX:
		    fputs("\
\\end{picture}%\n\
\\endgroup\n\
\\endinput\n", GPT.P_GpOutFile);
		    break;
		case PSTERM_PSTEX:
		    fputs("\
\\endGNUPLOTpicture\n\
\\endgroup\n\
\\endinput\n", GPT.P_GpOutFile);
		    break;
		default:; /* do nothing, just avoid a compiler warning */
	}
	ZFREE(p_gp->TPsB.PsLatexAuxname);
	if(GPT.P_GpPsFile && GPT.P_GpPsFile != GPT.P_GpOutFile) {
		fclose(GPT.P_GpPsFile);
		GPT.P_GpPsFile = NULL;
	}
}
//
// Functions for ps(la)tex 
//
void PSTEX_reopen_output(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(GPT.P_OutStr) {
		char * dotIndex;
		// if there's no extension, append ".ps" 
		if((dotIndex = strrchr(GPT.P_OutStr, '.')) == NULL)
			dotIndex = sstrchr(GPT.P_OutStr, '\0');
		// try to open the auxiliary file for the postscript parts. 
		if(p_gp->TPsB.P_Params->useauxfile) {
			// length of outstr plus ('.' or '\0') plus "eps" plus '\0' 
			p_gp->TPsB.PsLatexAuxname = (char *)SAlloc::R(p_gp->TPsB.PsLatexAuxname, dotIndex - GPT.P_OutStr + 5);
			if(p_gp->TPsB.PsLatexAuxname) {
				// include period or '\0' 
				strncpy(p_gp->TPsB.PsLatexAuxname, GPT.P_OutStr, (dotIndex - GPT.P_OutStr) + 1);
				// period or '\0' is overwritten with period, and "ps" appended 
				strcpy(p_gp->TPsB.PsLatexAuxname + (dotIndex - GPT.P_OutStr), ".ps");
				GPT.P_GpPsFile = fopen(p_gp->TPsB.PsLatexAuxname, "w");
				if(!GPT.P_GpPsFile) {
					fprintf(stderr, "Cannot open aux file %s for output. Switching off auxfile option.\n", p_gp->TPsB.PsLatexAuxname);
					SAlloc::F(p_gp->TPsB.PsLatexAuxname);
					p_gp->TPsB.PsLatexAuxname = NULL;
					p_gp->TPsB.P_Params->useauxfile = FALSE;
					GPT.P_GpPsFile = GPT.P_GpOutFile;
				}
			}
			else {
				fprintf(stderr, "Cannot make PostScript file name from %s\n", GPT.P_OutStr);
				fprintf(stderr, "Turning off auxfile option\n");
				p_gp->TPsB.P_Params->useauxfile = FALSE;
				GPT.P_GpPsFile = GPT.P_GpOutFile;
			}
		}
		else
			GPT.P_GpPsFile = GPT.P_GpOutFile;
	}
	else {
		if(p_gp->TPsB.P_Params->useauxfile) {
			fprintf(stderr, "Cannot use aux file on stdout. Switching off auxfile option.\n");
			p_gp->TPsB.P_Params->useauxfile = FALSE;
		}
		GPT.P_GpPsFile = GPT.P_GpOutFile;
	}
}

void PSTEX_common_init(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	PSLATEX_pagesize_x = pThis->MaxX * p_gp->V.Size.x;
	PSLATEX_pagesize_y = pThis->MaxY * p_gp->V.Size.y;
	switch(p_gp->TPsB.P_Params->terminal) {
		case PSTERM_PSLATEX:
		    fprintf(GPT.P_GpOutFile, "%% GNUPLOT: LaTeX picture with Postscript\n\
\\begingroup%%\n\
\\makeatletter%%\n\
\\newcommand{\\GNUPLOTspecial}{%%\n\
  \\@sanitize\\catcode`\\%%=14\\relax\\special}%%\n\
\\setlength{\\unitlength}{%.4fbp}%%\n",
			1.0 / (2*PS_SC));
		    fprintf(GPT.P_GpOutFile, "\\begin{picture}(%d,%d)(0,0)%%\n", (int)(PSLATEX_pagesize_x), (int)(PSLATEX_pagesize_y));
		    break;
		case PSTERM_PSTEX:
		    /* write plain TeX header */
		    fprintf(GPT.P_GpOutFile,
			"\
%% GNUPLOT: plain TeX with Postscript\n\
\\begingroup\n\
\\catcode`\\@=11\\relax\n\
\\def\\GNUPLOTspecial{%%\n\
  \\def\\do##1{\\catcode`##1=12\\relax}\\dospecials\n\
  \\catcode`\\{=1\\catcode`\\}=2\\catcode\\%%=14\\relax\\special}%%\n\
%%\n\
\\expandafter\\ifx\\csname GNUPLOTpicture\\endcsname\\relax\n\
  \\csname newdimen\\endcsname\\GNUPLOTunit\n\
  \\gdef\\GNUPLOTpicture(#1,#2){\\vbox to#2\\GNUPLOTunit\\bgroup\n\
    \\def\\put(##1,##2)##3{\\unskip\\raise##2\\GNUPLOTunit\n\
      \\hbox to0pt{\\kern##1\\GNUPLOTunit ##3\\hss}\\ignorespaces}%%\n\
    \\def\\ljust##1{\\vbox to0pt{\\vss\\hbox to0pt{##1\\hss}\\vss}}%%\n\
    \\def\\cjust##1{\\vbox to0pt{\\vss\\hbox to0pt{\\hss ##1\\hss}\\vss}}%%\n\
    \\def\\rjust##1{\\vbox to0pt{\\vss\\hbox to0pt{\\hss ##1}\\vss}}%%\n\
    \\def\\stack##1{\\let\\\\=\\cr\\tabskip=0pt\\halign{\\hfil ####\\hfil\\cr ##1\\crcr}}%%\n\
    \\def\\lstack##1{\\hbox to0pt{\\vbox to0pt{\\vss\\stack{##1}}\\hss}}%%\n\
    \\def\\cstack##1{\\hbox to0pt{\\hss\\vbox to0pt{\\vss\\stack{##1}}\\hss}}%%\n\
    \\def\\rstack##1{\\hbox to0pt{\\vbox to0pt{\\stack{##1}\\vss}\\hss}}%%\n\
    \\vss\\hbox to#1\\GNUPLOTunit\\bgroup\\ignorespaces}%%\n\
  \\gdef\\endGNUPLOTpicture{\\hss\\egroup\\egroup}%%\n\
\\fi\n\
\\GNUPLOTunit=%.4fbp\n", 1.0 / (2*PS_SC));
		    fprintf(GPT.P_GpOutFile, "\\GNUPLOTpicture(%d,%d)\n", (int)(PSLATEX_pagesize_x), (int)(PSLATEX_pagesize_y));
		    break;
		default:; /* do nothing, just avoid a compiler warning */
	}
	if(GPT.P_GpPsFile != GPT.P_GpOutFile) {
		// these are taken from the post.trm file computation of the bounding box, but without the X_OFF and Y_OFF 
		int urx = (int)(PSLATEX_pagesize_x / (2*PS_SC) + 0.5);
		int ury = (int)(PSLATEX_pagesize_y / (2*PS_SC) + 0.5);
		/* p_gp->TPsB.PsLatexAuxname is only != NULL with the `auxfile' option.
		 * If p_gp->TPsB.PsLatexAuxname is not a simple file name, but a path,
		 * we need to strip the path off the auxiliary file name,
		 * because tex file and ps aux file end up in the same directory! */
		char * psfile_basename = gp_basename(p_gp->TPsB.PsLatexAuxname);
		// generate special which xdvi and dvips can handle 
		fprintf(GPT.P_GpOutFile, "  \\special{psfile=%s llx=0 lly=0 urx=%d ury=%d rwi=%d}\n", psfile_basename, urx, ury, 10 * urx);
	}
	else
		fputs("  {\\GNUPLOTspecial{\"\n", GPT.P_GpOutFile);
	/* HH: really necessary?
	   p_gp->TPsB.Ang = 0;
	   p_gp->TPsB.Justify = 0;
	 */
	pstex_labels = (pstex_text_command *)NULL;
}

TERM_PUBLIC void PSTEX_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str)
{
	GnuPlot * p_gp = pThis->P_Gp;
	// ignore empty strings 
	if(!isempty(str)) {
		// Save the text for later printing after the core graphics 
		pstex_text_command * tc = (pstex_text_command *)SAlloc::M(sizeof(struct pstex_text_command));
		tc->x = x;
		tc->y = y;
		tc->label = (char *)SAlloc::M(strlen(str) + 1);
		strcpy(tc->label, str);
		tc->justify = p_gp->TPsB.Justify;
		tc->angle = p_gp->TPsB.Ang;
		tc->next = pstex_labels;
		pstex_labels = tc;
	}
}

TERM_PUBLIC void PSTEX_text(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	struct pstex_text_command * tc;
	PS_text(pThis);
	if(GPT.P_GpPsFile == GPT.P_GpOutFile)
		fputs("  }}%\n", GPT.P_GpOutFile);
	if(p_gp->TPsB.P_Params->fontsize) {
		if(p_gp->TPsB.P_Params->terminal == PSTERM_PSLATEX)
			fprintf(GPT.P_GpOutFile, "\\fontsize{%g}{\\baselineskip}\\selectfont\n", p_gp->TPsB.P_Params->fontsize);
		// Should have an else clause here to handle pstex equivalent 
	}
	for(tc = pstex_labels; tc != (struct pstex_text_command *)NULL; tc = tc->next) {
		fprintf(GPT.P_GpOutFile, "  \\put(%d,%d){", tc->x, tc->y);
		if((p_gp->TPsB.P_Params->rotate) && (tc->angle != 0))
			fprintf(GPT.P_GpOutFile, "%%\n  \\special{ps: gsave currentpoint currentpoint translate\n%d rotate neg exch neg exch translate}%%\n  ", 360 - tc->angle);
		if((p_gp->TPsB.P_Params->terminal == PSTERM_PSLATEX) && ((tc->label[0] == '{') || (tc->label[0] == '['))) {
			fprintf(GPT.P_GpOutFile, "\\makebox(0,0)%s", tc->label);
		}
		else
			switch(tc->justify) {
				case LEFT: 
					fprintf(GPT.P_GpOutFile, (p_gp->TPsB.P_Params->terminal == PSTERM_PSLATEX ? "\\makebox(0,0)[l]{\\strut{}%s}" : "\\ljust{\\strut{}%s}"), tc->label);
				    break;
				case CENTRE:
				    fprintf(GPT.P_GpOutFile, (p_gp->TPsB.P_Params->terminal == PSTERM_PSLATEX ? "\\makebox(0,0){\\strut{}%s}" : "\\cjust{\\strut{}%s}"), tc->label);
				    break;
				case RIGHT:
				    fprintf(GPT.P_GpOutFile, (p_gp->TPsB.P_Params->terminal == PSTERM_PSLATEX ? "\\makebox(0,0)[r]{\\strut{}%s}" : "\\rjust{\\strut{}%s}"), tc->label);
				    break;
			}
		if((p_gp->TPsB.P_Params->rotate) && (tc->angle != 0))
			fputs("%\n  \\special{ps: currentpoint grestore moveto}%\n  ", GPT.P_GpOutFile);
		fputs("}%\n", GPT.P_GpOutFile);
	}
	while(pstex_labels) {
		tc = pstex_labels->next;
		SAlloc::F(pstex_labels->label);
		SAlloc::F(pstex_labels);
		pstex_labels = tc;
	}
}
//
// Functions for epslatex 
//
// the common init function for the epslatex driver 
// used by pslatex epslatex cairolatex 
//
void EPSLATEX_common_init(GpTermEntry_Static * pThis)
{
	GnuPlot * p_gp = pThis->P_Gp;
	char * fontfamily = NULL;
	char * fontseries = NULL;
	char * fontshape = NULL;
	PSLATEX_pagesize_x = pThis->MaxX * p_gp->V.Size.x;
	PSLATEX_pagesize_y = pThis->MaxY * p_gp->V.Size.y;
	// cairo terminals use a different convention for xmax/ymax 
	if(sstreq(pThis->name, "cairolatex")) {
		PSLATEX_pagesize_x += 2*PS_SC * p_gp->V.Size.x;
		PSLATEX_pagesize_y += 2*PS_SC * p_gp->V.Size.y;
	}
	if(!GPT.P_GpOutFile) {
		char * temp = (char *)SAlloc::M(strlen(GPT.P_OutStr) + 1);
		if(temp) {
			strcpy(temp, GPT.P_OutStr);
			p_gp->TermSetOutput(pThis, temp); // will free outstr 
			if(temp != GPT.P_OutStr) {
				SAlloc::F(temp);
				temp = GPT.P_OutStr;
			}
		}
		else
			p_gp->OsError(p_gp->Pgm.GetCurTokenIdx(), "Cannot reopen output files");
	}
	if(!GPT.P_OutStr)
		p_gp->OsError(p_gp->Pgm.GetCurTokenIdx(), "epslatex terminal cannot write to standard output");
	if(GPT.P_GpOutFile) {
		const char * inputenc = p_gp->LatexInputEncoding(GPT._Encoding);
		fprintf(GPT.P_GpOutFile, "%% GNUPLOT: LaTeX picture with Postscript\n");
		tex_previous_colorspec.type = (colortype)-1; // Clear previous state 
		EPSLATEX_layer(pThis, TERM_LAYER_RESET); // Clear any leftover text-layering state 
		// Analyse LaTeX font name 'family,series,shape' 
		if((strlen(p_gp->TPsB.P_Params->font) > 0) && (strcmp(p_gp->TPsB.P_Params->font, "default") != 0)) {
			char * comma = NULL;
			fontfamily = (char *)SAlloc::M(strlen(p_gp->TPsB.P_Params->font)+1);
			fontseries = (char *)SAlloc::M(strlen(p_gp->TPsB.P_Params->font)+1);
			fontshape = (char *)SAlloc::M(strlen(p_gp->TPsB.P_Params->font)+1);
			strcpy(fontfamily, p_gp->TPsB.P_Params->font);
			*fontseries = '\0';
			*fontshape = '\0';
			if((comma = sstrchr(fontfamily, ',')) != NULL) {
				*comma = '\0';
				strcpy(fontseries, comma+1);
				if((comma = sstrchr(fontseries, ',')) != NULL) {
					*comma = '\0';
					strcpy(fontshape, comma+1);
				}
			}
		}
		if(p_gp->TPsB.P_Params->epslatex_standalone) {
			fprintf(GPT.P_GpOutFile,
			    "\
\\documentclass{minimal}\n\
%% Set font size\n\
\\makeatletter\n\
\\def\\@ptsize{%d}\n\
\\InputIfFileExists{size%d.clo}{}{%%\n\
   \\GenericError{(gnuplot) \\space\\space\\space\\@spaces}{%%\n\
      Gnuplot Error: File `size%d.clo' not found! Could not set font size%%\n\
   }{See the gnuplot documentation for explanation.%%\n\
   }{For using a font size a file `size<fontsize>.clo' has to exist.\n\
        Falling back ^^Jto default fontsize 10pt.}%%\n\
  \\def\\@ptsize{0}\n\
  \\input{size10.clo}%%\n\
}%%\n\
\\makeatother\n", (int)(p_gp->TPsB.P_Params->fontsize-10), (int)(p_gp->TPsB.P_Params->fontsize), (int)(p_gp->TPsB.P_Params->fontsize));
			if(!isempty(fontfamily))
				fprintf(GPT.P_GpOutFile, "\\renewcommand*\\rmdefault{%s}%%\n", fontfamily);
			if(!isempty(fontseries))
				fprintf(GPT.P_GpOutFile, "\\renewcommand*\\mddefault{%s}%%\n", fontseries);
			if(!isempty(fontshape))
				fprintf(GPT.P_GpOutFile, "\\renewcommand*\\updefault{%s}%%\n", fontshape);
			fputs("\
% Load packages\n\
\\usepackage{calc}\n\
\\usepackage{graphicx}\n\
\\usepackage{color}\n", GPT.P_GpOutFile);
			if(ISCAIROTERMINAL)
				fprintf(GPT.P_GpOutFile, "\\usepackage{transparent}\n");
			if(inputenc)
				fprintf(GPT.P_GpOutFile, "\\usepackage[%s]{inputenc}\n", inputenc);
			fprintf(GPT.P_GpOutFile,
			    "\
\\makeatletter\n\
%% Select an appropriate default driver (from TeXLive graphics.cfg)\n\
\\begingroup\n\
  \\chardef\\x=0 %%\n\
  %% check pdfTeX\n\
  \\@ifundefined{pdfoutput}{}{%%\n\
    \\ifcase\\pdfoutput\n\
    \\else\n\
      \\chardef\\x=1 %%\n\
    \\fi\n\
  }%%\n\
  %% check VTeX\n\
  \\@ifundefined{OpMode}{}{%%\n\
    \\chardef\\x=2 %%\n\
  }%%\n\
\\expandafter\\endgroup\n\
\\ifcase\\x\n\
  %% default case\n\
  \\PassOptionsToPackage{dvips}{geometry}\n\
\\or\n\
  %% pdfTeX is running in pdf mode\n\
  \\PassOptionsToPackage{pdftex}{geometry}\n\
\\else\n\
  %% VTeX is running\n\
  \\PassOptionsToPackage{vtex}{geometry}\n\
\\fi\n\
\\makeatother\n\
%% Set papersize\n\
\\usepackage[papersize={%.2fbp,%.2fbp},text={%.2fbp,%.2fbp}]{geometry}\n\
%% No page numbers and no paragraph indentation\n\
\\pagestyle{empty}\n\
\\setlength{\\parindent}{0bp}%%\n\
%% Load configuration file\n\
\\InputIfFileExists{gnuplot.cfg}{%%\n\
  \\typeout{Using configuration file gnuplot.cfg}%%\n\
}{%%\n\
 \\typeout{No configuration file gnuplot.cfg found.}%%\n\
}%%\n\
%s\n\
\\begin{document}\n",
			    PSLATEX_pagesize_x / (2.0*PS_SC),
			    PSLATEX_pagesize_y / (2.0*PS_SC),
			    PSLATEX_pagesize_x / (2.0*PS_SC),
			    PSLATEX_pagesize_y / (2.0*PS_SC),
			    epslatex_header ? epslatex_header : "%");
		}

		fputs("\\begingroup\n", GPT.P_GpOutFile);

		if(inputenc && GPT._Encoding != S_ENC_UTF8)
			fprintf(GPT.P_GpOutFile,
			    "\
  %% Encoding inside the plot.  In the header of your document, this encoding\n\
  %% should to defined, e.g., by using\n\
  %% \\usepackage[%s,<other encodings>]{inputenc}\n\
  \\inputencoding{%s}%%\n",
			    inputenc, inputenc);
		if(!p_gp->TPsB.P_Params->epslatex_standalone) {
			if(!isempty(fontfamily))
				fprintf(GPT.P_GpOutFile, "  \\fontfamily{%s}%%\n", fontfamily);
			if(!isempty(fontseries))
				fprintf(GPT.P_GpOutFile, "  \\fontseries{%s}%%\n", fontseries);
			if(!isempty(fontshape))
				fprintf(GPT.P_GpOutFile, "  \\fontshape{%s}%%\n", fontshape);
			if(fontfamily || fontseries || fontshape)
				fputs("  \\selectfont\n", GPT.P_GpOutFile);
			if(epslatex_header)
				fprintf(GPT.P_GpOutFile, "%s\n", epslatex_header);
		}
		fprintf(GPT.P_GpOutFile,
		    "\
  \\makeatletter\n\
  \\providecommand\\color[2][]{%%\n\
    \\GenericError{(gnuplot) \\space\\space\\space\\@spaces}{%%\n\
      Package color not loaded in conjunction with\n\
      terminal option `colourtext'%%\n\
    }{See the gnuplot documentation for explanation.%%\n\
    }{Either use 'blacktext' in gnuplot or load the package\n\
      color.sty in LaTeX.}%%\n\
    \\renewcommand\\color[2][]{}%%\n\
  }%%\n\
  \\providecommand\\includegraphics[2][]{%%\n\
    \\GenericError{(gnuplot) \\space\\space\\space\\@spaces}{%%\n\
      Package graphicx or graphics not loaded%%\n\
    }{See the gnuplot documentation for explanation.%%\n\
    }{The gnuplot epslatex terminal needs graphicx.sty or graphics.sty.}%%\n\
    \\renewcommand\\includegraphics[2][]{}%%\n\
  }%%\n\
  \\providecommand\\rotatebox[2]{#2}%%\n\
  \\@ifundefined{ifGPcolor}{%%\n\
    \\newif\\ifGPcolor\n\
    \\GPcolor%s\n\
  }{}%%\n\
  \\@ifundefined{ifGPblacktext}{%%\n\
    \\newif\\ifGPblacktext\n\
    \\GPblacktext%s\n\
  }{}%%\n\
  %% define a \\g@addto@macro without @ in the name:\n\
  \\let\\gplgaddtomacro\\g@addto@macro\n\
  %% define empty templates for all commands taking text:\n\
  \\gdef\\gplbacktext{}%%\n\
  \\gdef\\gplfronttext{}%%\n\
  \\makeatother\n",
		    (p_gp->TPsB.P_Params->color ? "true" : "false"),
		    (p_gp->TPsB.P_Params->blacktext ? "true" : "false") );

		/* use \expandafter\def\csname LT0\endcsname{...}
		 * instead of \def\LT0{...} because digits may not be part of
		 * \... sequences */
		fputs("\
  \\ifGPblacktext\n\
    % no textcolor at all\n\
    \\def\\colorrgb#1{}%\n\
    \\def\\colorgray#1{}%\n\
  \\else\n\
    % gray or color?\n\
    \\ifGPcolor\n\
      \\def\\colorrgb#1{\\color[rgb]{#1}}%\n\
      \\def\\colorgray#1{\\color[gray]{#1}}%\n\
      \\expandafter\\def\\csname LTw\\endcsname{\\color{white}}%\n\
      \\expandafter\\def\\csname LTb\\endcsname{\\color{black}}%\n\
      \\expandafter\\def\\csname LTa\\endcsname{\\color{black}}%\n",
		    GPT.P_GpOutFile);
		if(p_gp->TPsB.P_Params->oldstyle) {
			fputs("\
      \\expandafter\\def\\csname LT0\\endcsname{\\color[rgb]{1,0,0}}%\n\
      \\expandafter\\def\\csname LT1\\endcsname{\\color[rgb]{0,0,1}}%\n\
      \\expandafter\\def\\csname LT2\\endcsname{\\color[rgb]{0,1,1}}%\n\
      \\expandafter\\def\\csname LT3\\endcsname{\\color[rgb]{1,0,1}}%\n",
			    GPT.P_GpOutFile);
		}
		else {
			fputs("\
      \\expandafter\\def\\csname LT0\\endcsname{\\color[rgb]{1,0,0}}%\n\
      \\expandafter\\def\\csname LT1\\endcsname{\\color[rgb]{0,1,0}}%\n\
      \\expandafter\\def\\csname LT2\\endcsname{\\color[rgb]{0,0,1}}%\n\
      \\expandafter\\def\\csname LT3\\endcsname{\\color[rgb]{1,0,1}}%\n\
      \\expandafter\\def\\csname LT4\\endcsname{\\color[rgb]{0,1,1}}%\n\
      \\expandafter\\def\\csname LT5\\endcsname{\\color[rgb]{1,1,0}}%\n\
      \\expandafter\\def\\csname LT6\\endcsname{\\color[rgb]{0,0,0}}%\n\
      \\expandafter\\def\\csname LT7\\endcsname{\\color[rgb]{1,0.3,0}}%\n\
      \\expandafter\\def\\csname LT8\\endcsname{\\color[rgb]{0.5,0.5,0.5}}%\n",
			    GPT.P_GpOutFile);
		}
		fputs("\
    \\else\n\
      % gray\n\
      \\def\\colorrgb#1{\\color{black}}%\n\
      \\def\\colorgray#1{\\color[gray]{#1}}%\n\
      \\expandafter\\def\\csname LTw\\endcsname{\\color{white}}%\n\
      \\expandafter\\def\\csname LTb\\endcsname{\\color{black}}%\n\
      \\expandafter\\def\\csname LTa\\endcsname{\\color{black}}%\n\
      \\expandafter\\def\\csname LT0\\endcsname{\\color{black}}%\n\
      \\expandafter\\def\\csname LT1\\endcsname{\\color{black}}%\n\
      \\expandafter\\def\\csname LT2\\endcsname{\\color{black}}%\n\
      \\expandafter\\def\\csname LT3\\endcsname{\\color{black}}%\n\
      \\expandafter\\def\\csname LT4\\endcsname{\\color{black}}%\n\
      \\expandafter\\def\\csname LT5\\endcsname{\\color{black}}%\n\
      \\expandafter\\def\\csname LT6\\endcsname{\\color{black}}%\n\
      \\expandafter\\def\\csname LT7\\endcsname{\\color{black}}%\n\
      \\expandafter\\def\\csname LT8\\endcsname{\\color{black}}%\n\
    \\fi\n\
  \\fi\n",
		    GPT.P_GpOutFile);

		fprintf(GPT.P_GpOutFile, "\\setlength{\\unitlength}{%.4fbp}%%\n", 1.0 / (2*PS_SC));
		fprintf(GPT.P_GpOutFile, "\\ifx\\gptboxheight\\undefined%%\n\\newlength{\\gptboxheight}%%\n\\newlength{\\gptboxwidth}%%\n\\newsavebox{\\gptboxtext}%%\n\
    \\fi%%\n\
    \\setlength{\\fboxrule}{0.5pt}%%\n\
    \\setlength{\\fboxsep}{1pt}%%\n");
		fprintf(GPT.P_GpOutFile, "\\begin{picture}(%.2f,%.2f)%%\n", PSLATEX_pagesize_x, PSLATEX_pagesize_y);
	}
	if(p_gp->TPsB.P_Params->background.r >= 0) {
		fprintf(GPT.P_GpOutFile, "\\definecolor{gpBackground}{rgb}{%.3f, %.3f, %.3f}%%\n", p_gp->TPsB.P_Params->background.r, p_gp->TPsB.P_Params->background.g, p_gp->TPsB.P_Params->background.b);
		fprintf(GPT.P_GpOutFile, "\\put(0,0){\\colorbox{gpBackground}{\\makebox(%.2f,%.2f)[]{}}}%%\n", PSLATEX_pagesize_x, PSLATEX_pagesize_y);
	}
	SAlloc::F(fontfamily);
	SAlloc::F(fontseries);
	SAlloc::F(fontshape);
}

void EPSLATEX_put_text(GpTermEntry_Static * pThis, uint x, uint y, const char * str)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(GPT.P_GpOutFile) {
		if(!tex_color_synced) {
			fputs(tex_current_color, GPT.P_GpOutFile);
			fputs("%%\n", GPT.P_GpOutFile);
			tex_color_synced = TRUE;
		}
		if(PSLATEX_inbox) {
			if(PSLATEX_saved)
				return;
			fprintf(GPT.P_GpOutFile, "      \\settowidth{\\gptboxwidth}{\\widthof{%s}}\n", str);
			fprintf(GPT.P_GpOutFile, "\t\\advance\\gptboxwidth by %d\\fboxsep\n", (int)(2. * PSLATEX_xmargin + 0.5));
			fprintf(GPT.P_GpOutFile, "      \\savebox{\\gptboxtext}{\\parbox[c][\\totalheight+%d\\fboxsep]{\\gptboxwidth}{\\centering{%s}}}\n",
			    (int)(2. * PSLATEX_ymargin + 0.5), str);
			PSLATEX_xbox = x;
			PSLATEX_ybox = y;
			PSLATEX_saved = TRUE;
		}
		else {
			fprintf(GPT.P_GpOutFile, "      \\put(%d,%d){", x, y);
			if(p_gp->TPsB.Ang)
				fprintf(GPT.P_GpOutFile, "\\rotatebox{%d}{", p_gp->TPsB.Ang);
			if(((str[0] == '{') || (str[0] == '['))) {
				fprintf(GPT.P_GpOutFile, "\\makebox(0,0)%s", str);
			}
			else {
				switch(p_gp->TPsB.Justify) {
					case LEFT: fprintf(GPT.P_GpOutFile, "\\makebox(0,0)[l]{\\strut{}%s}", str); break;
					case CENTRE: fprintf(GPT.P_GpOutFile, "\\makebox(0,0){\\strut{}%s}", str); break;
					case RIGHT: fprintf(GPT.P_GpOutFile, "\\makebox(0,0)[r]{\\strut{}%s}", str); break;
				}
			}
			if(p_gp->TPsB.Ang)
				fputs("}", GPT.P_GpOutFile);
			fputs("}%\n", GPT.P_GpOutFile);
		}
	}
}
//
// assigns dest to outstr, so it must be allocated or NULL
// and it must not be outstr itself !
//
void EPSLATEX_reopen_output(GpTermEntry_Static * pThis, const char * ext)
{
	GnuPlot * p_gp = pThis->P_Gp;
	char * psoutstr = NULL;
	if(GPT.P_OutStr) {
		uint outstrlen = strlen(GPT.P_OutStr);
		if(strrchr(GPT.P_OutStr, '.') != &GPT.P_OutStr[outstrlen-4]) {
			p_gp->IntError(NO_CARET, "epslatex output file name must be of the form filename.xxx");
		}
		// copy filename to postsript output 
		psoutstr = (char *)SAlloc::M(outstrlen+5);
		strcpy(psoutstr, GPT.P_OutStr);
		if((!strncmp(&GPT.P_OutStr[outstrlen-4], ".eps", 4)) || (!strncmp(&GPT.P_OutStr[outstrlen-4], ".EPS", 4))) {
			if(p_gp->TPsB.P_Params->epslatex_standalone)
				p_gp->IntError(NO_CARET, "For epslatex standalone mode, you have to %s", "give the tex filename as output");
			// rename primary output (tex) 
			strncpy(&GPT.P_OutStr[outstrlen-4], ".tex", 5);
			// redirect FILE stream 
			GPT.P_GpPsFile = GPT.P_GpOutFile;
			GPT.P_GpOutFile = fopen(GPT.P_OutStr, "w");
			p_gp->IntWarn(NO_CARET, "Resetting primary output file to %s,\nPostScript output to %s", GPT.P_OutStr, psoutstr);
			if(!GPT.P_GpOutFile)
				p_gp->IntError(NO_CARET, "--- reopen failed");
		}
		else {
			char suffix[PATH_MAX];
			if(p_gp->TPsB.P_Params->epslatex_standalone)
				sprintf(suffix, "-inc.%s", ext);
			else
				sprintf(suffix, ".%s", ext);
			psoutstr[outstrlen-4] = '\0';
			strcat(psoutstr, suffix);
			// BM: Need binary output for PDF files. Does this have negative side effects for EPS? 
			GPT.P_GpPsFile = fopen(psoutstr, "wb");
		}
		if(!GPT.P_GpPsFile)
			p_gp->IntError(NO_CARET, "open of postscipt output file %s failed", psoutstr);
		// set the name for the \includegraphics command 
		p_gp->TPsB.PsLatexAuxname = sstrdup(psoutstr);
		p_gp->TPsB.PsLatexAuxname[strlen(psoutstr)-4] = '\0';
		SAlloc::F(psoutstr);
	}
}

void EPSLATEX_set_color(GpTermEntry_Static * pThis, const t_colorspec * colorspec)
{
	GnuPlot * p_gp = pThis->P_Gp;
	double gray;
#ifdef HAVE_CAIROPDF
	// Fancy footwork to deal with mono/grayscale plots 
	if(ISCAIROTERMINAL) {
		cairotrm_set_color(pThis, colorspec);
	}
	else
#endif
	{
		// Filter out duplicate requests 
		if(!memcmp(&tex_previous_colorspec, colorspec, sizeof(t_colorspec)))
			return;
		else
			memcpy(&tex_previous_colorspec, colorspec, sizeof(t_colorspec));
		PS_set_color(pThis, colorspec);
	}
	// Many [most? all?] of the set_color commands only affect the *.eps
	// output stream.  So rather than printing them all to the *.tex stream,
	// we update the current color and set a flag to say it has changed.
	// Only when some TeX object is output do we sync the current color by   
	// writing it out.
	tex_color_synced = FALSE;
	if(colorspec->type == TC_RGB) {
		double r = (double)((colorspec->lt >> 16 ) & 255) / 255.0;
		double g = (double)((colorspec->lt >> 8 ) & 255) / 255.0;
		double b = (double)(colorspec->lt & 255) / 255.0;
		sprintf(tex_current_color, "      \\colorrgb{%3.2f,%3.2f,%3.2f}", r, g, b);
		sprintf(tex_rgb_colordef, "\\definecolor{tbcol}{rgb}{%3.2f,%3.2f,%3.2f}", r, g, b);
	}
	if(colorspec->type == TC_LT) {
		int linetype = colorspec->lt;
		if(p_gp->TPsB.P_Params->oldstyle)
			linetype = (linetype % 4) + 3;
		else
			linetype = (linetype % 9) + 3;
		sprintf(tex_current_color, "      \\csname LT%c\\endcsname", "wba012345678"[linetype]);
		/* FIXME */
		sprintf(tex_rgb_colordef, "\\definecolor{tbcol}{rgb}{1,1,1}");
	}
	if(colorspec->type != TC_FRAC)
		return;
	// map [0;1] to gray/colors 
	gray = colorspec->value;
	if(p_gp->TPsB.P_Params->blacktext) {
		if(gray <= 0)
			sprintf(tex_current_color, "      \\color{black}");
		else if(gray >= 1)
			sprintf(tex_current_color, "      \\color{white}");
		else
			sprintf(tex_current_color, "      \\colorgray{%s}", GpPostscriptBlock::SaveSpace(gray));
	}
	else {
		rgb_color color;
		p_gp->Rgb1FromGray(colorspec->value, &color);
		sprintf(tex_current_color, "      \\colorrgb{%3.2f,%3.2f,%3.2f}", color.r, color.g, color.b);
		sprintf(tex_rgb_colordef, "\\definecolor{tbcol}{rgb}{%3.2f,%3.2f,%3.2f}", color.r, color.g, color.b);
	}
}

void EPSLATEX_linetype(GpTermEntry_Static * pThis, int linetype)
{
	t_colorspec tempcol = {TC_LT, 0, 0.0};
	tempcol.lt = linetype;
#ifdef HAVE_CAIROPDF
	if(ISCAIROTERMINAL)
		cairotrm_linetype(pThis, linetype);
	else
#endif
	PS_linetype(pThis, linetype);
	// This leads to redundant *.eps output 
	EPSLATEX_set_color(pThis, &tempcol);
}
//
// The TERM_LAYER mechanism is used here to signal a difference between
// "front" text and "back" text.
//
void EPSLATEX_layer(GpTermEntry_Static * pThis, t_termlayer syncpoint)
{
	static int plotno = 0;
	switch(syncpoint) {
		case TERM_LAYER_BEFORE_PLOT:
		    if(!ISCAIROTERMINAL)
			    fprintf(GPT.P_GpPsFile, "%% Begin plot #%d\n", ++plotno);
		    break;
		case TERM_LAYER_AFTER_PLOT:
		    PS_linetype(pThis, LT_UNDEFINED); /* Forces a stroke and resets linetype */
		    if(!ISCAIROTERMINAL)
			    fprintf(GPT.P_GpPsFile, "%% End plot #%d\n", plotno);
		    break;
		case TERM_LAYER_RESET: /* Start of plot; reset flag */
		    epslatex_text_layer = 0;
		    plotno = 0;
		    break;
		case TERM_LAYER_BACKTEXT: /* Start of "back" text layer */
		    if(epslatex_text_layer == 1)
			    break;
		    if(epslatex_text_layer == 2)
			    fputs("    }%\n", GPT.P_GpOutFile);
		    epslatex_text_layer = 1;
		    fputs("    \\gplgaddtomacro\\gplbacktext{%\n", GPT.P_GpOutFile);
		    tex_color_synced = FALSE;
		    break;
		case TERM_LAYER_FRONTTEXT:/* Start of "front" text layer */
		    if(epslatex_text_layer == 2)
			    break;
		    if(epslatex_text_layer == 1)
			    fputs("    }%\n", GPT.P_GpOutFile);
		    epslatex_text_layer = 2;
		    fputs("    \\gplgaddtomacro\\gplfronttext{%\n", GPT.P_GpOutFile);
		    tex_color_synced = FALSE;
		    break;

		case TERM_LAYER_END_TEXT: /* Close off front or back macro before leaving */
		    if(epslatex_text_layer == 1 || epslatex_text_layer == 2)
			    fputs("    }%\n", GPT.P_GpOutFile);
		    epslatex_text_layer = 0;
		    break;

		case TERM_LAYER_BEGIN_PM3D_MAP:
		    if(!ISCAIROTERMINAL)
			    if(GPT.P_GpPsFile && GPT.P_GpPsFile != GPT.P_GpOutFile)
				    fprintf(GPT.P_GpPsFile, "%%pm3d_map_begin\n");
		    break;

		case TERM_LAYER_END_PM3D_MAP:
		    if(!ISCAIROTERMINAL)
			    if(GPT.P_GpPsFile && GPT.P_GpPsFile != GPT.P_GpOutFile)
				    fprintf(GPT.P_GpPsFile, "%%pm3d_map_end\n");
		    break;

		default:
		    break;
	}
}

void EPSLATEX_boxed_text(GpTermEntry_Static * pThis, uint x, uint y, int option)
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(GPT.P_GpOutFile) {
		switch(option) {
			case TEXTBOX_INIT:
				// Initialize bounding box for this text string 
				PSLATEX_inbox = TRUE;
				PSLATEX_saved = FALSE;
				break;
			case TEXTBOX_FINISH:
				PSLATEX_inbox = FALSE;
				break;
			case TEXTBOX_OUTLINE:
				fputs("\t\\settowidth{\\gptboxwidth}{\\usebox{\\gptboxtext}}\n", GPT.P_GpOutFile);
				fputs("\t\\advance\\gptboxwidth by 2\\fboxsep\n", GPT.P_GpOutFile);
				fprintf(GPT.P_GpOutFile, "\t\\put(%d,%d)", PSLATEX_xbox, PSLATEX_ybox);
				switch(p_gp->TPsB.Justify) {
					case LEFT:
						fputs("{\\makebox(0,0)[l]", GPT.P_GpOutFile);
						fputs("{\\framebox[\\gptboxwidth]{\\usebox{\\gptboxtext}}}}\n", GPT.P_GpOutFile);
						break;
					case CENTRE:
						fputs("{\\makebox(0,0)", GPT.P_GpOutFile);
						fputs("{\\framebox[\\gptboxwidth][c]{\\usebox{\\gptboxtext}}}}\n", GPT.P_GpOutFile);
						break;
					case RIGHT:
						fputs("{\\makebox(0,0)[r]", GPT.P_GpOutFile);
						fputs("{\\framebox[\\gptboxwidth][r]{\\usebox{\\gptboxtext}}}}\n", GPT.P_GpOutFile);
						break;
				}
				PSLATEX_inbox = FALSE;
				break;
			case TEXTBOX_BACKGROUNDFILL:
				if(!tex_color_synced) {
					// FIXME: sync tex_current_color also? 
					fprintf(GPT.P_GpOutFile, "        %s\n", tex_rgb_colordef);
					tex_color_synced = TRUE;
				}
				fprintf(GPT.P_GpOutFile, "\t\\put(%d,%d)", PSLATEX_xbox, PSLATEX_ybox);
				switch(p_gp->TPsB.Justify) {
					case LEFT: fputs("{\\makebox(0,0)[l]{", GPT.P_GpOutFile); break;
					case CENTRE: fputs("{\\makebox(0,0){", GPT.P_GpOutFile); break;
					case RIGHT: fputs("{\\makebox(0,0)[r]{", GPT.P_GpOutFile); break;
				}
				if(PSLATEX_opacity < 1.0)
					fprintf(GPT.P_GpOutFile, "\\transparent{%.2f}", PSLATEX_opacity);
				fputs("\\colorbox{tbcol}{\\usebox{\\gptboxtext}}}}\n", GPT.P_GpOutFile);
				break;
			case TEXTBOX_MARGINS:
				PSLATEX_xmargin = (double)x / 100.0;
				PSLATEX_ymargin = (double)y / 100.0;
				break;
			default:
				break;
		}
	}
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE

#ifndef GOT_POST_PROTO
	#define TERM_PROTO_ONLY
	//#include "post.trm"
	#undef TERM_PROTO_ONLY
#endif

TERM_TABLE_START(epslatex_driver)
	"epslatex", 
	"LaTeX picture environment using graphicx package",
	PS_XMAX, 
	PS_YMAX, 
	EPSLATEX_VCHAR, 
	EPSLATEX_HCHAR,
	PS_VTIC, 
	PS_HTIC, 
	PS_options, 
	PS_init, 
	PSLATEX_reset,
	PS_text, 
	GnuPlot::NullScale, 
	PS_graphics, 
	PS_move,
	PS_vector, 
	EPSLATEX_linetype, 
	EPSLATEX_put_text, 
	PS_text_angle,
	PS_justify_text, 
	PS_point, 
	GnuPlot::DoArrow, 
	PS_set_font,
	PS_pointsize, 
	TERM_BINARY|TERM_IS_POSTSCRIPT|TERM_CAN_CLIP|TERM_IS_LATEX /*flags */,
	0 /*suspend */, 
	0 /*resume */,
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
	PS_previous_palette,
	EPSLATEX_set_color, 
	PS_filled_polygon,
	PS_image,
	0, 
	0, 
	0,     /* Enhanced text mode not used */
	EPSLATEX_layer,     /* Used to signal front/back text */
	PS_path,
	0.0,            /* scale */
	0,              /* hypertext */
	EPSLATEX_boxed_text,
	NULL,
	PS_dashtype 
TERM_TABLE_END(epslatex_driver)
#undef LAST_TERM
#define LAST_TERM epslatex_driver

TERM_TABLE_START(pslatex_driver)
	"pslatex", 
	"LaTeX picture environment with PostScript \\specials",
	PS_XMAX, 
	PS_YMAX, 
	PSTEX_VCHAR, 
	PSTEX_HCHAR,
	PS_VTIC, 
	PS_HTIC, 
	PS_options, 
	PS_init, 
	PSLATEX_reset,
	PSTEX_text, 
	GnuPlot::NullScale, 
	PS_graphics, 
	PS_move,
	PS_vector, 
	PS_linetype, 
	PSTEX_put_text, 
	PS_text_angle,
	PS_justify_text, 
	PS_point, 
	GnuPlot::DoArrow, 
	set_font_null,
	PS_pointsize, 
	TERM_CAN_CLIP|TERM_IS_LATEX /*flags */, 
	0, /*suspend */
	0, /*resume */
	PS_fillbox, 
	PS_linewidth,
	#ifdef USE_MOUSE
	0, 
	0, 
	0, 
	0, 
	0,    /* no mouse support for postscript */
	#endif
	PS_make_palette,
	PS_previous_palette,    /* write grestore */
	PS_set_color,
	PS_filled_polygon,
	PS_image,
	0, 
	0, 
	0, // No enhanced text mode because this is LaTeX 
	0, // layer 
	PS_path 
TERM_TABLE_END(pslatex_driver)
#undef LAST_TERM
#define LAST_TERM pslatex_driver

TERM_TABLE_START(pstex_driver)
	"pstex", 
	"plain TeX with PostScript \\specials",
	PS_XMAX, 
	PS_YMAX, 
	PSTEX_VCHAR, 
	PSTEX_HCHAR,
	PS_VTIC, 
	PS_HTIC, 
	PS_options, 
	PS_init, 
	PSLATEX_reset,
	PSTEX_text, 
	GnuPlot::NullScale, 
	PS_graphics, 
	PS_move,
	PS_vector, 
	PS_linetype, 
	PSTEX_put_text, 
	PS_text_angle,
	PS_justify_text, 
	PS_point, 
	GnuPlot::DoArrow, 
	set_font_null,
	PS_pointsize, 
	TERM_CAN_CLIP|TERM_IS_LATEX, // flags 
	0, // suspend 
	0, // resume 
	PS_fillbox, 
	PS_linewidth,
	#ifdef USE_MOUSE
	0, 
	0, 
	0, 
	0, 
	0, // no mouse support for postscript 
	#endif
	PS_make_palette,
	PS_previous_palette, // write grestore 
	PS_set_color,
	PS_filled_polygon,
	PS_image,
	0, 
	0, 
	0, // No enhanced text mode because this is LaTeX 
	0, // layer 
	PS_path 
TERM_TABLE_END(pstex_driver)
#undef LAST_TERM
#define LAST_TERM pstex_driver

#endif /* TERM_TABLE */
#endif /* TERM_PROTO_ONLY */
