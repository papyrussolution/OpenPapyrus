/*
 * $Id: graphics.h,v 1.68 2016/01/28 23:54:13 sfeam Exp $
 */

/* GNUPLOT - graphics.h */

/*[
 * Copyright 1999, 2004
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

#ifndef GNUPLOT_GRAPHICS_H
# define GNUPLOT_GRAPHICS_H

//#include "syscfg.h"
//#include "gp_types.h"
//#include "gadgets.h"
//#include "term_api.h"
//
// types defined for 2D plotting
//
struct curve_points {
    curve_points *next;	/* pointer to next plot in linked list */
    int token;			/* last token used, for second parsing pass */
    enum PLOT_TYPE plot_type;	/* DATA2D? DATA3D? FUNC2D FUNC3D? NODATA? */
    enum PLOT_STYLE plot_style;	/* style set by "with" or by default */
    char *title;		/* plot title, a.k.a. key entry */
    t_position *title_position;	/* title at {beginning|end|<xpos>,<ypos>} */
    bool title_no_enhanced;	/* don't typeset title in enhanced mode */
    bool title_is_filename;	/* true if title was auto-generated from filename */
    bool title_is_suppressed;/* true if 'notitle' was specified */
    bool noautoscale;	/* ignore data from this plot during autoscaling */
    lp_style_type lp_properties;
    arrow_style_type arrow_properties;
    fill_style_type fill_properties;
    text_label *labels;	/* Only used if plot_style == LABELPOINTS */
    t_image image_properties;	/* only used if plot_style is IMAGE or RGB_IMAGE */
    UdvtEntry *sample_var;	/* Only used if plot has private sampling range */

    /* 2D and 3D plot structure fields overlay only to this point */
    filledcurves_opts filledcurves_options;
    int    base_linetype; // before any calls to load_linetype(), lc variable 
				/* analogous to hidden3d_top_linetype in graph3d.h  */
    int    ellipseaxes_units;              /* Only used if plot_style == ELLIPSES */    
    histogram_style *histogram;	/* Only used if plot_style == HISTOGRAM */
    int    histogram_sequence;	/* Ordering of this dataset within the histogram */
    enum PLOT_SMOOTH plot_smooth; /* which "smooth" method to be used? */
    double smooth_parameter;	/* e.g. optional bandwidth for smooth kdensity */
    int    boxplot_factors;	/* Only used if plot_style == BOXPLOT */
    int    p_max;			/* how many points are allocated */
    int    p_count;		/* count of points in points */
    int    x_axis;			/* FIRST_X_AXIS or SECOND_X_AXIS */
    int    y_axis;			/* FIRST_Y_AXIS or SECOND_Y_AXIS */
    int    z_axis;			/* same as either x_axis or y_axis, for 5-column plot types */
    int    n_par_axes;		/* Only used for parallel axis plots */
    double **z_n;		/* Only used for parallel axis plots */
    double * varcolor;		/* Only used if plot has variable color */
    GpCoordinate * points;
};

// externally visible variables of graphics.h 

class GpGraphics {
public:
	GpGraphics()
	{
		loff.Set(first_axes, first_axes, first_axes, 0.0, 0.0, 0.0);
		roff.Set(first_axes, first_axes, first_axes, 0.0, 0.0, 0.0);
		toff.Set(first_axes, first_axes, first_axes, 0.0, 0.0, 0.0);
		boff.Set(first_axes, first_axes, first_axes, 0.0, 0.0, 0.0);
		BarSize = 1.0;
		BarLayer = LAYER_FRONT;
	}
	void   DoPlot(curve_points * plots, int pcount);
	void   PlotBars(curve_points * plot);
	void   PlotLines(curve_points * plot);
	void   PlotDots(curve_points * plot);
	void   PlotFilledCurves(curve_points * plot);
	void   PlotEllipses(curve_points * plot);
	void   PlotVectors(curve_points * plot);
	void   PlotBetweenCurves(curve_points * plot);
	void   PlotFBars(curve_points * plot);
	void   PlotCBars(curve_points * plot);
	void   PlotBoxPlot(curve_points * plot);
	void   FillBetween(double x1, double xu1, double yl1, double yu1, double x2, double xu2, double yl2, double yu2, curve_points * plot);
	void   AdjustOffsets();
	// 'set offset' status variables
	t_position loff;
	t_position roff;
	t_position toff;
	t_position boff;
	// 'set bar' status
	double BarSize;
	int    BarLayer;
	lp_style_type BarLp;
};

extern GpGraphics GpGr;
/*
// 'set offset' status variables
extern t_position loff, roff, toff, boff;
// 'set bar' status
extern double GpGr.BarSize;
extern int GpGr.BarLayer;
extern lp_style_type GpGr.BarLp;
*/
//
// function prototypes 
//
//void do_plot(curve_points *, int);
void map_position(t_position * pos, int *x, int *y, const char *what);
void map_position_r(t_position* pos, double* x, double* y, const char* what);
void init_histogram(histogram_style *hist, text_label *title);
void free_histlist(histogram_style *hist);

enum t_procimg_action {
    IMG_PLOT,
    IMG_UPDATE_AXES,
    IMG_UPDATE_CORNERS
};

void process_image(void *plot, t_procimg_action action);
bool check_for_variable_color(curve_points *plot, double *colorvalue);

#ifdef EAM_OBJECTS
	void place_objects(t_object * listhead, int layer, int dimensions);
	void do_ellipse(int dimensions, t_ellipse *e, int style, bool do_own_mapping);
	void do_polygon(int dimensions, t_polygon *p, int style, t_clip_object clip);
#else
	#define place_objects(listhead,layer,dimensions) /* void() */
#endif

int filter_boxplot(curve_points *);

#endif /* GNUPLOT_GRAPHICS_H */
