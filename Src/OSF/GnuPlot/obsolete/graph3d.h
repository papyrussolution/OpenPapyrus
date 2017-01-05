/*
 * $Id: graph3d.h,v 1.50 2016/04/23 00:36:22 sfeam Exp $
 */

/* GNUPLOT - graph3d.h */

/*[
 * Copyright 1999, 2004   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_GRAPH3D_H
#define GNUPLOT_GRAPH3D_H

/* #if... / #include / #define collection: */

//#include "syscfg.h"
//#include "gp_types.h"
#include "gadgets.h"
//#include "term_api.h"

struct GpAxis;
struct ticmark;

//
// Type definitions
//
enum t_dgrid3d_mode {
    DGRID3D_DEFAULT,
    DGRID3D_QNORM,
    DGRID3D_SPLINES,
    DGRID3D_GAUSS,
    DGRID3D_EXP,
    DGRID3D_CAUCHY,
    DGRID3D_BOX,
    DGRID3D_HANN,
    DGRID3D_OTHER
};

enum  t_contour_placement {
    // Where to place contour maps if at all
    CONTOUR_NONE,
    CONTOUR_BASE,
    CONTOUR_SRF,
    CONTOUR_BOTH
};

typedef double transform_matrix[4][4]; /* HBB 990826: added */

struct gnuplot_contours {
    gnuplot_contours *next;
    GpCoordinate * coords;
    char   isNewLevel;
    char   label[32];
    int    num_pts;
    double z;
};

struct iso_curve {
    iso_curve * next;
    int    p_max;   // how many points are allocated 
    int    p_count; // count of points in points 
    GpCoordinate * points;
};

struct surface_points {
    surface_points *next_sp; // pointer to next plot in linked list 
    int    token;            // last token used, for second parsing pass 
    PLOT_TYPE  plot_type;    // DATA2D? DATA3D? FUNC2D FUNC3D? NODATA? 
    PLOT_STYLE plot_style;   // style set by "with" or by default 
    char * title;            // plot title, a.k.a. key entry 
    t_position *title_position;	// title at {beginning|end|<xpos>,<ypos>} 
    bool   title_no_enhanced;	// don't typeset title in enhanced mode 
    bool   title_is_filename;	// not used in 3D 
    bool   title_is_suppressed;// true if 'notitle' was specified 
    bool   noautoscale;	// ignore data from this plot during autoscaling 
    lp_style_type lp_properties;
    arrow_style_type arrow_properties;
    fill_style_type fill_properties;	// FIXME: ignored in 3D 
    text_label *labels;	// Only used if plot_style == LABELPOINTS 
    t_image image_properties;	// only used if plot_style is IMAGE, RGBIMAGE or RGBA_IMAGE 
    UdvtEntry *sample_var;	// Only used if plot has private sampling range 
    // 2D and 3D plot structure fields overlay only to this point 
    bool opt_out_of_hidden3d; // set by "nohidden" option to splot command 
    bool opt_out_of_contours; // set by "nocontours" option to splot command 
    bool opt_out_of_surface;  // set by "nosurface" option to splot command 
    bool pm3d_color_from_column;
    int hidden3d_top_linetype;	// before any calls to load_linetype() 
    int has_grid_topology;
    int iteration;		// needed for tracking iteration 
    // Data files only - num of isolines read from file. For functions,  
    // num_iso_read is the number of 'primary' isolines (in x direction) 
    int num_iso_read;
    gnuplot_contours *contours; // NULL if not doing contours. 
    iso_curve *iso_crvs;	// the actual data 
    char pm3d_where[7];		// explicitly given base, top, surface 
};

struct t_xyplane { 
	void   Set(double z, bool isAbs)
	{
		Z = z;
		IsAbsolute = isAbs;
	}
    double Z; 
    bool   IsAbsolute;
};

enum WHICHGRID { 
	ALLGRID, 
	FRONTGRID, 
	BACKGRID, 
	BORDERONLY 
};

#define ISO_SAMPLES 10		// default number of isolines per splot 

class GpGraphics3D {
public:
	GpGraphics3D()
	{
		draw_contour = CONTOUR_NONE; // is contouring wanted ? 
		clabel_interval = 20;               /* label every 20th contour segment */
		clabel_start = 5;                   /*       starting with the 5th */
		clabel_font = NULL;               /* default to current font */
		clabel_onecolor = false;       /* use same linetype for all contours */
		draw_surface = true; /* Draw the surface at all? (false if only contours are wanted) */
		implicit_surface = true; /* Always create a gridded surface when lines are read from a data file */
		hidden3d = false; /* Was hidden3d display selected by user? */
		hidden3d_layer = LAYER_BACK;
		xmiddle = 0;
		ymiddle = 0;
		xscaler = 0;
		yscaler = 0;
		surface_rot_z = 30.0;
		surface_rot_x = 60.0;
		surface_scale = 1.0;
		surface_zscale = 1.0;
		surface_lscale = 0.0;
		mapview_scale = 1.0;
		splot_map = 0 /*false*/; // Set by 'set view map': 
		xyplane.Set(0.5, false); // position of the base plane, as given by 'set ticslevel' or 'set xyplane' 
		iso_samples_1 = ISO_SAMPLES;
		iso_samples_2 = ISO_SAMPLES;
		Scale3d.Set(0.0, 0.0, 0.0);
		Center3d.Set(0.0, 0.0, 0.0);
		floor_z = 0.0;
		ceiling_z = 0.0;
		base_z = 0.0; // made exportable for PM3D
		memzero(trans_mat, sizeof(trans_mat));
		axis3d_o_x = 0;
		axis3d_o_y = 0;
		axis3d_x_dx = 0;
		axis3d_x_dy = 0;
		axis3d_y_dx = 0;
		axis3d_y_dy = 0;
	}
	void   Do3DPlot(surface_points * pPlots, int pcount/* count of plots in linked list */, int quick/* !=0 means plot only axes etc., for quick rotation */);
	void   Cntr3DLines(gnuplot_contours * cntr, lp_style_type * lp);
	void   Cntr3DPoints(gnuplot_contours * cntr, lp_style_type * lp);
	void   Cntr3DLabels(gnuplot_contours * cntr, char * level_text, text_label * label);
	void   Cntr3DImpulses(gnuplot_contours * cntr, lp_style_type * lp);
	void   Draw3DGraphBox(surface_points * plot, int plot_num, WHICHGRID whichgrid, int current_layer);
	void   Plot3DLines(surface_points * plot);
	void   Plot3DLinesPm3D(surface_points * plot);
	void   Boundary3D(surface_points * plots, int count);
	bool   GetArrow3D(arrow_def* arrow, int* sx, int* sy, int* ex, int* ey);
	int    FindMaxlKeys3D(surface_points * plots, int count, int * kcnt);
	void   PlaceLabels3D(text_label * listhead, int layer);
	void   PlaceArrows3D(int layer);
	int    Map3DGetPosition(t_position * pos, const char * what, double * xpos, double * ypos, double * zpos);
	void   Setup3DBoxCorners();

	static void XTickCallback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid/* linetype or -2 for none */, ticmark * userlabels/* currently ignored in 3D plots */);
	static void YTickCallback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels/* currently ignored in 3D plots */);
	static void ZTickCallback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels/* currently ignored in 3D plots */);

	int    xmiddle;
	int    ymiddle;
	int    xscaler;
	int    yscaler;
	double floor_z;
	double ceiling_z;
	double base_z; // made exportable for PM3D 
	transform_matrix trans_mat;
	RPoint3 Scale3d;
	RPoint3 Center3d;
	t_contour_placement draw_contour;
	int    clabel_start;
	int    clabel_interval;
	char * clabel_font;
	bool   clabel_onecolor;
	bool   draw_surface;
	bool   implicit_surface;
	bool   hidden3d; // is hidden3d display wanted? 
	int    hidden3d_layer; // LAYER_FRONT or LAYER_BACK
	float  surface_rot_z;
	float  surface_rot_x;
	float  surface_scale;
	float  surface_zscale;
	float  surface_lscale;
	float  mapview_scale;
	int    splot_map;
	t_xyplane xyplane;
	int    iso_samples_1;
	int    iso_samples_2;
#ifdef USE_MOUSE
	int    axis3d_o_x;
	int    axis3d_o_y;
	int    axis3d_x_dx;
	int    axis3d_x_dy;
	int    axis3d_y_dx;
	int    axis3d_y_dy;
#endif
};

extern GpGraphics3D Gp3Gr;

// Variables of graph3d.c needed by other modules: 
#if 0 // {
extern int    xmiddle, ymiddle, xscaler, yscaler;
extern double floor_z;
extern double ceiling_z, base_z; /* made exportable for PM3D */
extern transform_matrix trans_mat;
//extern double xscale3d, yscale3d, zscale3d;
//extern double xcenter3d, ycenter3d, zcenter3d;
extern RPoint3 Scale3d;
extern RPoint3 Center3d;
extern t_contour_placement draw_contour;
extern int clabel_start;
extern int clabel_interval;
extern char *clabel_font;
extern bool	clabel_onecolor;
extern bool	draw_surface;
extern bool	implicit_surface;
extern bool	hidden3d; /* is hidden3d display wanted? */
extern int hidden3d_layer;	/* LAYER_FRONT or LAYER_BACK */
extern float surface_rot_z;
extern float surface_rot_x;
extern float surface_scale;
extern float surface_zscale;
extern float surface_lscale;
extern float mapview_scale;
extern int splot_map;
extern t_xyplane xyplane;
extern int iso_samples_1;
extern int iso_samples_2;
#ifdef USE_MOUSE
	extern int axis3d_o_x, axis3d_o_y, axis3d_x_dx, axis3d_x_dy, axis3d_y_dx, axis3d_y_dy;
#endif
#endif // } 0 
//
// Prototypes from file "graph3d.c" 
//
//void do_3dplot(surface_points *plots, int pcount, int quick);
void map3d_position(t_position *pos, int *x, int *y, const char *what);
void map3d_position_double(t_position *pos, double *x, double *y, const char *what);
void map3d_position_r(t_position *pos, int *x, int *y, const char *what);

#endif /* GNUPLOT_GRAPH3D_H */
