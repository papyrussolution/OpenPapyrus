// GNUPLOT - term.h 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
/*
 * term.h: terminal support definitions
 *   Edit this file depending on the set of terminals you wish to support.
 * Comment out the terminal types that you don't want or don't have, and
 * uncomment those that you want included. Be aware that some terminal
 * types will require changes in the makefile LIBS definition.
 */
/*
 * first draft after all terminals are converted to new layout
 * Stefan Bodewig Dec. 1995
 */
/*
 * >>> CONFIGURATION OPTIONS FOLLOW <<<  PLEASE READ
 *
 * pslatex and epslatex support is now provided by the combination of
 * post.trm and pslatex.trm.  You cannot build pslatex without post.
 * Both drivers are selected by default, but you can disable them below.
 *
 * Enhanced text support is pretty much required for all terminals now.
 * If you build without GP_ENH_EST text layout will be degraded.
 */
#define GP_ENH_EST 1            // estimate string length of enhanced text 
#define POSTSCRIPT_DRIVER 1     // include post.trm 
// (moved to gnuplot.h) #define PSLATEX_DRIVER 1        // include pslatex.trm 
#if defined(PSLATEX_DRIVER) && !defined(POSTSCRIPT_DRIVER)
	#define POSTSCRIPT_DRIVER
#endif
#ifdef GP_ENH_EST
	//#include "estimate.trm" /* used for enhanced text processing */
#endif
// 
// Define SHORT_TERMLIST to select a few terminals. It is easier
// to define the macro and list desired terminals in this section.
// Sample configuration for a Unix workstation
// 
#ifdef SHORT_TERMLIST
	#include "dumb.trm"             /* dumb terminal */
	#ifdef POSTSCRIPT_DRIVER
	#ifdef  PSLATEX_DRIVER
		#undef PSLATEX_DRIVER
	#endif
		#include "post.trm"             /* postscript */
	#endif
	#ifdef X11
		#include "x11.trm"              /* X Window system */
	#endif                          /* X11 */
	#ifdef _WIN32
		// @experimental #include "win.trm"              /* MS-Windows */
	#endif
#else // include all applicable terminals not commented out 
//
// Platform dependent part
//
#ifdef __BEOS__
	#include "be.trm"
#endif
// MSDOS with djgpp compiler or _WIN32/Mingw or X11 
#if (defined(DJGPP) && (!defined(DJSVGA) || (DJSVGA != 0))) || defined(HAVE_GRX)
	#include "djsvga.trm"
#endif
/****************************************************************************/
/* Windows */
#ifdef _WIN32
	// @experimental #include "win.trm" // Windows GDI/GDI+/Direct2D 
#endif
/* Apple Mac OS X */
#ifdef HAVE_FRAMEWORK_AQUATERM
	#include "aquaterm.trm" // support for AquaTerm.app 
#endif
//
// Terminals for various Unix platforms
//
#ifdef UIS
	#include "vws.trm" // VAX Windowing System requires UIS libraries 
#endif
// Terminals not relevant for MSDOS, MS-Windows 
#if !defined(_WIN32)
	#include "linux-vgagl.trm" /* This is not really a terminal, it generates a help section for  using the linux console. */
	#ifdef HAVE_GPIC
		#include "gpic.trm" // gpic for groff 
	#endif
	#if defined(HAVE_REGIS)
		#include "regis.trm" // REGIS graphics language 
	#endif
	#ifdef WITH_TEKTRONIX
		#include "t410x.trm" // Tektronix 4106, 4107, 4109 and 420x terminals 
		#include "tek.trm" // a Tek 4010 and others including VT-style 
	#endif
#endif
//
// These terminals can be used on any system 
#ifdef X11
	#include "x11.trm"  // X Window System 
	#include "xlib.trm" // dumps x11 commands to gpoutfile 
#endif
// #include "ai.trm" // Adobe Illustrator Format. obsolete: use 'set term postscript level1 
#if (defined(HAVE_GD_PNG) || defined(HAVE_CAIROPDF))
	#include "write_png_image.c" /* HTML Canvas terminal */
#endif
//#include "canvas.trm"
//#include "cgm.trm" /* Computer Graphics Metafile (eg ms office) */
// #include "corel.trm" // CorelDraw! eps format 
//#ifdef DEBUG
	//#include "debug.trm" // debugging terminal 
//#endif
//#include "dumb.trm" // dumb terminal 
#ifdef HAVE_LIBCACA
	#include "caca.trm" /* caca: color ascii art terminal using libcaca */
#endif
#ifndef NO_BITMAP_SUPPORT
	//#include "block.trm" // pseudo-graphics using block or Braille characters 
#endif
// 
// Legacy terminal for export to AutoCad (Release 10.x)
// DWGR10 format (1988)
// Still included by popular demand although basically untouched for 20+ years.
// Someone please update this terminal to adhere to a newer DXF standard!
// http://images.autodesk.com/adsk/files/autocad_2012_pdf_dxf-reference_enu.pdf
// 
//#include "dxf.trm"
//#include "emf.trm" // Enhanced Metafile Format driver 
// #include "dxy.trm"  // Roland DXY800A plotter 
// #include "excl.trm" // QMS/EXCL laserprinter (Talaris 1590 and others) 
// #include "fig.trm" // fig graphics 
// #include "grass.trm" // geographical info system */
// #include "hp26.trm" // HP2623A "ET head" 1980 era graphics terminal */
// #include "hp2648.trm" // HP2647 and 2648 */
//#include "hpgl.trm" // HP7475, HP7220 plotters, and (hopefully) lots of others 
#ifndef NO_BITMAP_SUPPORT
	//#include "hp500c.trm" // HP DeskJet 500 C 
	//#include "hpljii.trm" // HP Laserjet II 
	//#include "hppj.trm"   // HP PrintJet 
#endif
// #include "imagen.trm" // Imagen laser printers 
// #include "kyo.trm" // Kyocera Prescribe printer 
#ifdef HAVE_MIF
	#include "mif.trm" // Frame Maker MIF 3.00 format driver 
#endif
// DEPRECATED since 5.0.6
// PDF terminal based on non-free library PDFlib or PDFlib-lite from GmbH.
// #include "pdf.trm" 
#if defined(HAVE_GD_PNG) || defined(HAVE_GD_JPEG) || defined(HAVE_GD_GIF)
	#include "gd.trm"
#endif
#ifdef POSTSCRIPT_DRIVER
	//#include "post.trm" // postscript 
#endif
//#include "qms.trm"  // QMS laser printers 
//#include "svg.trm" // W3C Scalable Vector Graphics file 
#ifdef HAVE_TGIF
	#include "tgif.trm" // x11 tgif tool 
#endif
// @sobolev #include "tkcanvas.trm" /* tcl/tk with perl extensions */
#ifndef NO_BITMAP_SUPPORT
	//#include "pbm.trm" // portable bit map 
	// wire printers 
	//#define EPSONP // Epson LX-800, Star NL-10, NX-1000 and lots of others 
	//#define EPS60  // Epson-style 60-dot per inch printers 
	//#define EPS180 // Epson-style 180-dot per inch (24 pin) printers 
	//#define NEC
	//#define OKIDATA
	//#define STARC
	//#define DPU414  // Seiko DPU-414 thermal printer 
	//#define TANDY60 // Tandy DMP-130 series 60-dot per inch graphics 
	//#include "epson.trm" // the common driver file for all of these 
#endif /* NO_BITMAP_SUPPORT */
// TeX related terminals start here 
//#include "pict2e.trm" // LaTeX2e picture environment 
#ifdef PSLATEX_DRIVER
	//#include "pslatex.trm" // latex/tex with picture in postscript 
#endif
//#include "pstricks.trm" // LaTeX picture environment with PSTricks macros 
//#include "texdraw.trm" // TeXDraw drawing package for LaTeX 
//#include "metafont.trm" // METAFONT 
#ifdef WITH_METAPOST
	//#include "metapost.trm" /* METAPOST */
#endif
//#include "context.trm" /* ConTeXt */
// 
// DEPRECATED latex terminals no longer built by default
// 
#if 0
	#define EMTEX
	#define EEPIC
	#define OLD_LATEX_TERMINAL
	#include "latex.trm"    /* latex and emtex */
	#include "eepic.trm"    /* EEPIC-extended LaTeX driver */
	#include "tpic.trm"     /* TPIC specials for TeX */
#else
	//#include "latex_old.h" /* deprecation notice for docs */
	// 
	// This section is added to the user manual in place of the 
	// terminal descriptions for latex emtex eepic and tpic if these
	// terminals are disabled, as they are by default in version 5.4
	// 
	#ifdef TERM_HELP
		START_HELP(latex)
		"1 latex",
		"?set terminal latex",
		"?set term latex",
		"?terminal latex",
		"?term latex",
		"?latex",
		"?set terminal emtex",
		"?set terminal eepic",
		"?set terminal tpic",
		"?set term emtex",
		"?set term eepic",
		"?set term tpic",
		"?terminal emtex",
		"?terminal eepic",
		"?terminal tpic",
		"?emtex",
		"?eepic",
		"?tpic",
		" Gnuplot provides a variety of terminals for use with TeX/LaTeX.",
		"",
		" (1) TeX/LaTeX compatible terminals based on use of PostScript",
		" See  `epslatex`, `pslatex`, `mp` (metapost),  and `pstricks`.",
		"",
		" (2) TeX/LaTeX compatible terminals based on cairo graphics",
		" See `cairolatex`.",
		"",
		" (3) The `tikz` terminal uses an external lua script (see `lua`)",
		" to produce files for the PGF and TikZ packages.",
		" Use the command `set term tikz help` to print terminal options.",
		"",
		" (4) The `pict2e` terminal (added in version 5.4) replaces a set of legacy",
		" terminals `latex`, `emtex`, `eepic`, and `tpic` present in older versions",
		" of gnuplot. See `pict2e`.",
		"",
		" (5) Others, see `context`, `mf` (metafont).",
		""
		END_HELP(latex)
	#endif
#undef OLD_LATEX_TERMINAL
#endif
// End of TeX related terminals 
#ifdef USE_GGI_DRIVER
	#include "ggi.trm"
#endif
#ifdef WXWIDGETS
	#include "wxt.trm" /* WXWIDGETS */
#endif
#ifdef HAVE_CAIROPDF
	#include "cairo.trm"
#endif
#ifdef HAVE_WEBP
	#include "webp.trm" /* webp must come after cairo */
#endif
#ifdef HAVE_LUA
	#include "lua.trm"
#endif
#ifdef QTTERM
	#include "qt.trm"
#endif

#endif /* !SHORT_TERMLIST */
