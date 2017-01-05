/*
 * $Id: term.h,v 1.69 2016/01/11 18:51:38 sfeam Exp $
 */

/* GNUPLOT - term.h */

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
 */
#define GP_ENH_EST 1		/* estimate string length of enhanced text */
	#define POSTSCRIPT_DRIVER 1	/* include post.trm */
#define PSLATEX_DRIVER 1	/* include pslatex.trm */
#if defined(PSLATEX_DRIVER) && !defined(POSTSCRIPT_DRIVER)
	#define POSTSCRIPT_DRIVER
#endif
/* Define SHORT_TERMLIST to select a few terminals. It is easier
 * to define the macro and list desired terminals in this section.
 * Sample configuration for a Unix workstation
 */
#ifdef SHORT_TERMLIST
	#include "dumb.trm" // dumb terminal 
	#ifdef GP_ENH_EST
		#include "estimate.trm"	/* used for enhanced text processing */
	#endif
	#ifdef POSTSCRIPT_DRIVER
		#ifdef  PSLATEX_DRIVER
			#undef PSLATEX_DRIVER
		#endif
		//#include "post.trm"		/* postscript */
	#endif
	#ifdef X11
		#include "x11.trm"		/* X Window system */
	#endif				/* X11 */
	#ifdef _Windows
		#include "win.trm"		/* MS-Windows */
	#endif
#else // include all applicable terminals not commented out 
//
/// Platform dependent part
//
#ifdef __BEOS__
	//#include "be.trm"
#endif
#if defined(_Windows)
	#if defined(DJGPP) && (!defined(DJSVGA) || (DJSVGA != 0)) // MSDOS with djgpp compiler 
		//#include "djsvga.trm"
	#endif
	// All other Compilers 
	#ifndef _Windows
		#ifdef PC
	// uncomment the next line to include SuperVGA support 
			#define BGI_NAME "svga256" // the name of the SVGA.BGI for Borland C 
	// this also triggers the inclusion of Super VGA support 
			//#include "pc.trm"		/* all PC types except MS WINDOWS */
		#endif
	#else				/* _Windows */
		#include "win.trm"		/* MS-Windows */
	#endif				/* _Windows */
#endif
//
#ifdef HAVE_FRAMEWORK_AQUATERM // Apple Mac OS X 
	//#include "aquaterm.trm" // support for AquaTerm.app 
#endif
//
// Terminals for various Unix platforms
//
#ifdef LINUXVGA // Linux VGA 
	//#include "linux.trm"
	#if defined(VGAGL) && defined (THREEDKIT) // Linux VGAGL 
		//#include "vgagl.trm"
	#endif
#endif
#ifdef UIS // VAX Windowing System requires UIS libraries 
	//#include "vws.trm"
#endif
//
// Terminals not relevant for MSDOS, MS-Windows 
//
#if !(defined(MSDOS) || defined(_Windows))
	#ifdef HAVE_GPIC
		//#include "gpic.trm" /* gpic for groff */
	#endif
	#ifdef VMS
		//#include "regis.trm" /* REGIS graphics language */
	#endif
	//#include "t410x.trm" /* Tektronix 4106, 4107, 4109 and 420x terminals */
	//#include "tek.trm" /* a Tek 4010 and others including VT-style */
#endif
//
// These terminals can be used on any system 
//
#ifdef X11
	#include "x11.trm"		/* X Window System */
	#include "xlib.trm"		/* dumps x11 commands to gpoutfile */
#endif
//
// Adobe Illustrator Format */
// obsolete: use 'set term postscript level1 */
// #include "ai.trm" */
#if(defined(HAVE_GD_PNG) || defined(HAVE_CAIROPDF))
	//#include "write_png_image.c" // HTML Canvas terminal
#endif
//#include "canvas.trm"
//#include "cgm.trm" /* Computer Graphics Metafile (eg ms office) */
//#include "corel.trm" /* CorelDraw! eps format */
#ifdef DEBUG
	#include "debug.trm" // debugging terminal 
#endif
#include "dumb.trm" // dumb terminal 
#ifdef HAVE_LIBCACA
	//#include "caca.trm" /* caca: color ascii art terminal using libcaca */
#endif
//#include "dxf.trm" /* DXF format for use with AutoCad (Release 10.x) */
//#include "emf.trm" // Enhanced Metafile Format driver 
/* Roland DXY800A plotter */
/* #include "dxy.trm" */
/* QMS/EXCL laserprinter (Talaris 1590 and others) */
/* #include "excl.trm" */

#include "fig.trm" /* fig graphics */
/* #include "grass.trm" */ /* geographical info system */
/* #include "hp26.trm" */ /* HP2623A "ET head" 1980 era graphics terminal */
/* #include "hp2648.trm" */ /* HP2647 and 2648 */
/* HP7475, HP7220 plotters, and (hopefully) lots of others */
//#include "hpgl.trm"
#ifndef NO_BITMAP_SUPPORT
	//#include "hp500c.trm" // HP DeskJet 500 C 
	//#include "hpljii.trm" // HP Laserjet II 
	//#include "hppj.trm"   // HP PrintJet 
#endif /* NO_BITMAP_SUPPORT */
/* #include "imagen.trm" */ /* Imagen laser printers */
/* #include "kyo.trm" */ /* Kyocera Prescribe printer */
#ifdef HAVE_MIF
	//#include "mif.trm" /* Frame Maker MIF 3.00 format driver */
#endif
#ifdef HAVE_LIBPDF
	//#include "pdf.trm" // Adobe Portable Document Format (PDF) NOTE THAT PDF REQUIRES A SEPARATE LIBRARY : see term/pdf.trm 
#endif
#if defined(HAVE_GD_PNG) || defined(HAVE_GD_JPEG) || defined(HAVE_GD_GIF)
	//#include "gd.trm"
#endif
#ifdef POSTSCRIPT_DRIVER
	//#include "post.trm" // postscript
#endif
//#include "qms.trm" /* QMS laser printers */
#include "svg.trm" // W3C Scalable Vector Graphics file 
//#include "tgif.trm" // x11 tgif tool
// @sobolev #include "tkcanvas.trm" // tcl/tk with perl extensions
#ifndef NO_BITMAP_SUPPORT
	//#include "pbm.trm" /* portable bit map */
	//
	// wire printers
	//
	#define EPSONP /* Epson LX-800, Star NL-10, NX-1000 and lots of others */
	#define EPS60 /* Epson-style 60-dot per inch printers */
	#define EPS180 /* Epson-style 180-dot per inch (24 pin) printers */
	#define NEC
	#define OKIDATA
	#define STARC
	#define DPU414 /* Seiko DPU-414 thermal printer */
	#define TANDY60 /* Tandy DMP-130 series 60-dot per inch graphics */
	//#include "epson.trm" /* the common driver file for all of these */
#endif
//
// TeX related terminals 
//
#define EMTEX
#define EEPIC
#include "latex.trm" // latex and emtex 
#ifdef PSLATEX_DRIVER
	//#include "pslatex.trm" // latex/tex with picture in postscript 
#endif
#include "eepic.trm" // EEPIC-extended LaTeX driver, for EEPIC users 
//#include "tpic.trm" // TPIC specials for TeX 
//#include "pstricks.trm" // LaTeX picture environment with PSTricks macros 
//#include "texdraw.trm" // TeXDraw drawing package for LaTeX 
//#include "metafont.trm" // METAFONT 
//#include "metapost.trm" // METAPOST 
//#include "context.trm" // ConTeXt 
#ifdef USE_GGI_DRIVER
	//#include "ggi.trm"
#endif
#ifdef GP_ENH_EST
	#include "estimate.trm"
#endif
#ifdef WXWIDGETS
	//#include "wxt.trm" // WXWIDGETS
#endif
#ifdef HAVE_CAIROPDF
	#include "cairo.trm"
#endif
#ifdef HAVE_LUA
	//#include "lua.trm"
#endif
#ifdef QTTERM
	//#include "qt.trm"
#endif

#endif /* !SHORT_TERMLIST */
