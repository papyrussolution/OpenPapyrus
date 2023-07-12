/* GNUPLOT - gp_cairo.h */
/*[
 * Copyright 2005,2006   Timothee Lecomte
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
 *
 *
 * Alternatively, the contents of this file may be used under the terms of the
 * GNU General Public License Version 2 or later (the "GPL"), in which case the
 * provisions of GPL are applicable instead of those above. If you wish to allow
 * use of your version of this file only under the terms of the GPL and not
 * to allow others to use your version of this file under the above gnuplot
 * license, indicate your decision by deleting the provisions above and replace
 * them with the notice and other provisions required by the GPL. If you do not
 * delete the provisions above, a recipient may use your version of this file
 * under either the GPL or the gnuplot license.
]*/

/* -----------------------------------------------------
 * This code uses the cairo library, a 2D graphics library with
 * support for multiple output devices.
 * Cairo is distributed under the LGPL licence.
 *
 * See http://www.cairographics.org for details.

 * It also uses the pango library, a text-layout rendering library.
 * Pango is distributed under the LGPL licence.
 *
 * See http://www.pango.org for details.
 * -----------------------------------------------------*/

/* ------------------------------------------------------
 * This is the header for all cairo related functions,
 * which provide drawing facilities to implement gnuplot's needs.
 *
 * In particular, we have here :
 * - all the basic calls (lines, polygons for pm3d, custom patterns),
 * - image support,
 * - enhanced text mode
 *
 * The text rendering is done via pango.
 * ------------------------------------------------------*/

#ifndef GNUPLOT_WXT_CAIRO_H
# define GNUPLOT_WXT_CAIRO_H

#ifdef __cplusplus
// @sobolev extern "C" {
#endif

//#include "term_api.h" // for JUSTIFY, set_encoding_id, *term definitions, color.h 
//#include "getcolor.h" // for rgb functions 
#include <cairo.h> // for rgb functions 

#define GP_CAIRO_SCALE 20 // oversampling scale 

typedef struct rgba_color {
	double r;
	double g;
	double b;
	double alpha;
} rgba_color;

/* linked list in reverse order used to draw polygons more efficiently */
typedef struct path_item {
	gpiPoint *corners;
	int n;
	rgba_color color;
	struct path_item *previous;
} path_item;

/* plot structure containing all the information cairo needs to execute
 * the drawing commands.
 * Don't forget to update gp_cairo_initialize_plot when a new entry is added there. */
struct PlotCairoStruct {
	/* scaling and conversion between gnuplot and device coordinates.
	 * For a static terminal, scales are 1 and sizes are those of the term table.
	 * For an interactive terminal, they are used to handle window resizing.
	 * xmax and ymax are the sizes known by gnuplot.
	 * device_xmax and device_ymax are the device size, which may change when the window
	 * is resized.
	 * xmax and ymax are stored here in addition to term->xmax and term->ymax,
	 * to handle the case when another terminal is chosen (x11 for example), which
	 * may change these variables whereas we want to keep the one used for our plot */
	double xscale;
	double yscale;
	int device_xmax;
	int device_ymax;
	int xmax;
	int ymax;
	int oversampling_scale; // either GP_CAIRO_SCALE or 1, depending on rendering 
	// The caiolatex performs upsampling of the canvas to achieve
	// higher pixel density.  We have to account for that when
	// applying OPERATOR_SATURATE. 
	double upsampling_rate;
	// handle vertical/horizontal lines properly 
	double current_x;
	double current_y;
	double orig_current_x;
	double orig_current_y;
	// style data used while processing gnuplot commands 
	JUSTIFY justify_mode;
	int linetype;
	double linewidth;
	int linestyle;
	double pointsize;
	double dashlength;
	double current_dashpattern[8];
	double text_angle;
	rgba_color color;
	rgb_color background;
	/* font handling */
	int fontweight;
	int fontstyle;
	double fontsize;
	enum set_encoding_id encoding;
	bool   opened_path; // "polyline" 
	bool   success; /* state of the cairo context creation */
	bool   antialiasing;
	bool   oversampling;
	bool   polygons_saturate;
	bool   interrupt; /* flag set to true when the user hit ctrl-c */
	char   fontname[MAX_ID_LEN + 1];
	uint8  Reserve[1]; // @alignment
	t_linecap linecap;
	/* hinting option for horizontal and vertical lines :
	 * Hinting is the process of fitting outlines to the pixel grid
	 * in order to improve the appearance of the result.
	 * Since hinting outlines involves distorting them,
	 * it also reduces the faithfulness to the original outline shapes.
	 * hinting = 100 means full hinting
	 * hinting = 0 means no hinting */
	int hinting;
	cairo_t * cr; /* cairo drawing context */
	path_item * polygon_path_last; /* polygons list */
};

/* linetype enums */
enum {
	GP_CAIRO_SOLID,
	GP_CAIRO_DASH,
	GP_CAIRO_DOTS
};

/* correspondence between gnuplot's linetypes and colors */
rgb_color gp_cairo_linetype2color( int linetype );
void gp_cairo_set_background(rgb_color background);

/* functions to handle scaling between device and gnuplot coordinates */
double device_x(PlotCairoStruct * plot, double x);
double device_y(PlotCairoStruct * plot, double y);
double gnuplot_x(PlotCairoStruct * plot, double x);
double gnuplot_y(PlotCairoStruct * plot, double y);

/* initialize all fields of the plot structure */
void gp_cairo_initialize_plot(PlotCairoStruct *plot);
/* initialize the transformation matrix of the drawing context and other details */
/* Depends on the setting of xscale and yscale */
void gp_cairo_initialize_context(PlotCairoStruct *plot);

/* functions used to process gnuplot commands */
void gp_cairo_move(PlotCairoStruct *plot, int x, int y);
void gp_cairo_vector(PlotCairoStruct *plot, int x, int y);
void gp_cairo_stroke(PlotCairoStruct *plot);
void gp_cairo_draw_text(PlotCairoStruct *plot, int x1, int y1, const char* str, int *width, int *height);
void gp_cairo_draw_enhanced_text(PlotCairoStruct *plot, int x1, int y1, const char* str);
void gp_cairo_enhanced_init(PlotCairoStruct *plot, int len);
void gp_cairo_enhanced_finish(PlotCairoStruct *plot, int x, int y);
void gp_cairo_enhanced_open(PlotCairoStruct *plot, char* fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
void gp_cairo_enhanced_flush(PlotCairoStruct *plot);
void gp_cairo_enhanced_writec(PlotCairoStruct *plot, int character);
void gp_cairo_draw_point(PlotCairoStruct *plot, int x1, int y1, int style);
void gp_cairo_draw_fillbox(PlotCairoStruct *plot, int x, int y, int width, int height, int style);
void gp_cairo_draw_polygon(PlotCairoStruct *plot, int n, gpiPoint *corners);
void gp_cairo_end_polygon(PlotCairoStruct *plot);
void gp_cairo_draw_image(PlotCairoStruct *plot, unsigned int * image, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, int M, int N);
void gp_cairo_set_color(PlotCairoStruct *plot, rgb_color color, double alpha);
void gp_cairo_set_linestyle(PlotCairoStruct *plot, int linestyle);
void gp_cairo_set_linetype(PlotCairoStruct *plot, int linetype);
void gp_cairo_set_pointsize(PlotCairoStruct *plot, double pointsize);
void gp_cairo_set_justify(PlotCairoStruct *plot, JUSTIFY mode);
void gp_cairo_set_font(PlotCairoStruct *plot, const char *name, float fontsize);
void gp_cairo_set_linewidth(PlotCairoStruct *plot, double linewidth);
void gp_cairo_set_textangle(PlotCairoStruct *plot, double angle);

/* erase the contents of the cairo drawing context */
void gp_cairo_solid_background(PlotCairoStruct *plot);
void gp_cairo_clear_background(PlotCairoStruct *plot);

/* helps to fill term->h_char, v_char, h_tic, v_tic
 * Depends on plot->fontsize and fontname */
void gp_cairo_set_termvar(PlotCairoStruct *plot, uint *v_char, uint *h_char);
// translate plot->encoding int to char* suitable for glib 
const char* gp_cairo_get_encoding(PlotCairoStruct *plot);
// determine default font to use 
const char * gp_cairo_default_font(void);
// work-around for "bold font gets stuck" bug 
void gp_cairo_clear_bold_font(PlotCairoStruct *plot);
// Text boxes 
void gp_cairo_boxed_text(PlotCairoStruct *plot, int x, int y, int option);
void gp_cairo_set_dashtype(PlotCairoStruct *plot, int type, t_dashtype *custom_dash_pattern);
// explicitly set resolution 
void gp_cairo_set_resolution(int dpi);

#ifdef __cplusplus
// @sobolev }
#endif /*__cplusplus*/

#endif /* gnuplot_wxt_cairo_h */
