/* GNUPLOT - driver.h */

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
#ifndef TERM_DRIVER_H
#define TERM_DRIVER_H

#include "syscfg.h"
//#include <stdio.h>
//
// functions provided by term.c 
//
static void do_point(unsigned int x, unsigned int y, int number);
static void line_and_point(unsigned int x, unsigned int y, int number);
static int null_text_angle(int ang);
static int null_justify_text(enum JUSTIFY just);
static int null_scale(double x, double y);
static void options_null(void);
static void UNKNOWN_null(void);
// static int set_font_null(const char *s);     */ /* unused */
#define set_font_null NULL
#define fflush_binary()

extern FILE * gpoutfile;
extern struct termentry * term;

#define sign(x) ((x) >= 0 ? 1 : -1) // for use by all drivers 

/* abs as macro is now uppercase, there are conflicts with a few C compilers
   that have abs as macro, even though ANSI defines abs as function
   (int abs(int)). Most calls to ABS in term/ could be changed to abs if
   they use only int arguments and others to fabs, but for the time being,
   all calls are done via the macro */
#ifndef ABS
	#define ABS(x) ((x) >= 0 ? (x) : -(x))
#endif /* ABS */

#define NICE_LINE		0
#define POINT_TYPES		6

//#ifdef TERM_PROTO
void DUMB_init(termentry * pThis);
void DUMB_graphics();
void DUMB_linetype(int linetype);
void DUMB_options();
void DUMB_text();
void DUMB_reset();
void DUMB_move(uint x, uint y);
void DUMB_point(uint x, uint y, int point);
void DUMB_vector(uint x, uint y);
void DUMB_put_text(uint x, uint y, const char * str);
void DUMB_arrow(uint sx, uint sy, uint ex, uint ey, int head);

#ifndef NO_DUMB_ENHANCED_SUPPORT
	// To support "set term dumb enhanced" (don't ask why!) 
	void ENHdumb_put_text(uint x, uint y, const char str[]);
	void ENHdumb_OPEN(char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
	void ENHdumb_FLUSH();
#else
	#define ENHdumb_put_text NULL
#endif
#ifndef NO_DUMB_COLOR_SUPPORT
	int  DUMB_make_palette(t_sm_palette * palette);
	void DUMB_set_color(t_colorspec *);
#endif
//#endif // TERM_PROTO 

void PS_options(TERMENTRY * pThis, GnuPlot * pGp);
void PS_common_init(bool uses_fonts, uint xoff, uint yoff, uint bb_xmin, uint bb_ymin, uint bb_xmax, uint bb_ymax, const char ** dict);
void PS_init(termentry * pThis);
void PS_graphics();
void PS_text();
void PS_reset();
void PS_linetype(int linetype);
void PS_dashtype(int type, t_dashtype * custom_dash_type);
void PS_move(uint x, uint y);
void PS_vector(uint x, uint y);
void PS_put_text(uint x, uint y, const char * str);
int  PS_text_angle(int ang);
int  PS_justify_text(enum JUSTIFY mode);
void PS_point(uint x, uint y, int number);
int  PS_set_font(const char * font);
void PS_fillbox(int style, uint x1, uint y1, uint width, uint height);
void PS_linewidth(double linewidth); /* JFi [linewidth] */
void PS_pointsize(double ptsize); /* JFi [pointsize] */
int  PS_make_palette(t_sm_palette *);
void PS_previous_palette();
void PS_set_color(t_colorspec *);
void PS_filled_polygon(int, gpiPoint *);
void PS_image(uint, uint, coordval *, gpiPoint *, t_imagecolor);
// To support "set term post enhanced" 
void ENHPS_put_text(uint x, uint y, const char * str);
int  ENHPS_set_font(const char * font);
void ENHPS_OPEN(char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
void ENHPS_FLUSH();
void ENHPS_WRITEC(int c);
void PS_RememberFont(char * fname);
char * PS_escape_string(char * origstr, char * escapelist);
void PS_path(int p);
void PS_layer(t_termlayer syncpoint);
void ENHPS_boxed_text(uint, uint, int);

#endif /* TERM_DRIVER_H */
