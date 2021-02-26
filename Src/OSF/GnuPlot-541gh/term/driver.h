// GNUPLOT - driver.h 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#ifndef TERM_DRIVER_H
#define TERM_DRIVER_H

#include "syscfg.h"
//#include <stdio.h>
//
// functions provided by term.c 
//
static void do_point(uint x, uint y, int number);
static void line_and_point(uint x, uint y, int number);
static int null_text_angle(int ang);
static int null_scale(double x, double y);
static void options_null(void);
static void UNKNOWN_null(GpTermEntry * pThis);
// static int set_font_null(const char *s);     */ /* unused */
#define set_font_null NULL
#define fflush_binary()

extern FILE * gpoutfile;
extern GpTermEntry * term;

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
void DUMB_init(GpTermEntry * pThis);
void DUMB_graphics(GpTermEntry * pThis);
void DUMB_linetype(GpTermEntry * pThis, int linetype);
void DUMB_options();
void DUMB_text(GpTermEntry * pThis);
void DUMB_reset(GpTermEntry * pThis);
void DUMB_move(GpTermEntry * pThis, uint x, uint y);
void DUMB_point(GpTermEntry * pThis, uint x, uint y, int point);
void DUMB_vector(GpTermEntry * pThis, uint x, uint y);
void DUMB_put_text(GpTermEntry * pThis, uint x, uint y, const char * str);
void DUMB_arrow(GpTermEntry * pThis, uint sx, uint sy, uint ex, uint ey, int head);

#ifndef NO_DUMB_ENHANCED_SUPPORT
	// To support "set term dumb enhanced" (don't ask why!) 
	void ENHdumb_put_text(GpTermEntry * pThis, uint x, uint y, const char str[]);
	void ENHdumb_OPEN(GpTermEntry * pThis, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
	void ENHdumb_FLUSH(GpTermEntry * pThis);
#else
	#define ENHdumb_put_text NULL
#endif
#ifndef NO_DUMB_COLOR_SUPPORT
	int  DUMB_make_palette(GpTermEntry * pThis, t_sm_palette * palette);
	void DUMB_set_color(GpTermEntry * pThis, const t_colorspec *);
#endif
//#endif // TERM_PROTO 

void PS_options(GpTermEntry * pThis, GnuPlot * pGp);
void PS_common_init(GpTermEntry * pThis, bool uses_fonts, uint xoff, uint yoff, uint bb_xmin, uint bb_ymin, uint bb_xmax, uint bb_ymax, const char ** dict);
void PS_init(GpTermEntry * pThis);
void PS_graphics(GpTermEntry * pThis);
void PS_text(GpTermEntry * pThis);
void PS_reset(GpTermEntry * pThis);
void PS_linetype(GpTermEntry * pThis, int linetype);
void PS_dashtype(GpTermEntry * pThis, int type, t_dashtype * custom_dash_type);
void PS_move(GpTermEntry * pThis, uint x, uint y);
void PS_vector(GpTermEntry * pThis, uint x, uint y);
void PS_put_text(GpTermEntry * pThis, uint x, uint y, const char * str);
int  PS_text_angle(GpTermEntry * pThis, int ang);
int  PS_justify_text(GpTermEntry * pThis, enum JUSTIFY mode);
void PS_point(GpTermEntry * pThis, uint x, uint y, int number);
int  PS_set_font(GpTermEntry * pThis, const char * font);
void PS_fillbox(GpTermEntry * pThis, int style, uint x1, uint y1, uint width, uint height);
void PS_linewidth(GpTermEntry * pThis, double linewidth); /* JFi [linewidth] */
void PS_pointsize(GpTermEntry * pThis, double ptsize); /* JFi [pointsize] */
int  PS_make_palette(GpTermEntry * pThis, t_sm_palette *);
void PS_previous_palette(GpTermEntry * pThis);
void PS_set_color(GpTermEntry * pThis, const t_colorspec *);
void PS_filled_polygon(GpTermEntry * pThis, int, gpiPoint *);
void PS_image(GpTermEntry * pThis, uint, uint, coordval *, gpiPoint *, t_imagecolor);
// To support "set term post enhanced" 
void ENHPS_put_text(GpTermEntry * pThis, uint x, uint y, const char * str);
int  ENHPS_set_font(GpTermEntry * pThis, const char * font);
void ENHPS_OPEN(GpTermEntry * pThis, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
void ENHPS_FLUSH(GpTermEntry * pThis);
void ENHPS_WRITEC(GpTermEntry * pThis, int c);
void PS_RememberFont(GpTermEntry * pThis, char * fname);
char * PS_escape_string(char * origstr, char * escapelist);
void PS_path(GpTermEntry * pThis, int p);
void PS_layer(GpTermEntry * pThis, t_termlayer syncpoint);
void ENHPS_boxed_text(GpTermEntry * pThis, uint, uint, int);

void cairotrm_set_color(GpTermEntry * pThis, const t_colorspec * colorspec);
void cairotrm_linetype(GpTermEntry * pThis, int linetype);
void EPSLATEX_set_color(GpTermEntry * pThis, const t_colorspec * colorspec);
void EPSLATEX_layer(GpTermEntry * pThis, t_termlayer syncpoint);
void EPSLATEX_boxed_text(GpTermEntry * pThis, uint, uint, int);
void EPSLATEX_linetype(GpTermEntry * pThis, int linetype);
void EPSLATEX_put_text(GpTermEntry * pThis, uint x, uint y, const char * str);

int  write_png_image(uint m, uint n, coordval * image, t_imagecolor color_mode, const char * filename);

extern ps_params_t * ps_params;
extern double PSLATEX_opacity; // = 1.0

#endif /* TERM_DRIVER_H */
