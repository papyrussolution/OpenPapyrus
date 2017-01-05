/*
 * $Id: pm3d.h,v 1.32 2015/11/13 04:03:57 sfeam Exp $
 */

/* GNUPLOT - pm3d.h */

/*[
 *
 * Petr Mikulik, since December 1998
 * Copyright: open source as much as possible
 *
 *
 * What is here: #defines, global variables and declaration of routines for
 * the pm3d plotting mode
 *
]*/


/* avoid multiple includes */
#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif
#ifndef TERM_HELP
#ifndef PM3D_H
#define PM3D_H
//#include "graph3d.h" /* surface_points */
//
// Global options for pm3d algorithm (to be accessed by set / show)
//
//
// where to plot pm3d: base or top (color map) or surface (color surface)
// The string pm3d.where can be any combination of the #defines below.
// For instance, "b" plot at botton only, "st" plots firstly surface, then top, etc.
//
#define PM3D_AT_BASE	'b'
#define PM3D_AT_TOP	't'
#define PM3D_AT_SURFACE	's'
/*
  options for flushing scans (for pm3d.flush)
  Note: new terminology compared to my pm3d program; in gnuplot it became
  begin and right instead of left and right
*/
#define PM3D_FLUSH_BEGIN   'b'
#define PM3D_FLUSH_END     'r'
#define PM3D_FLUSH_CENTER  'c'

/*
  direction of taking the scans: forward = as the scans are stored in the
  file; backward = opposite direction, i.e. like from the end of the file
*/
#define PM3D_SCANS_AUTOMATIC  'a'
#define PM3D_SCANS_FORWARD    'f'
#define PM3D_SCANS_BACKWARD   'b'
#define PM3D_DEPTH            'd'

/*
  clipping method:
    PM3D_CLIP_1IN: all 4 points of the quadrangle must be defined and at least
		   1 point of the quadrangle must be in the x and y ranges
    PM3D_CLIP_4IN: all 4 points of the quadrangle must be in the x and y ranges
*/
#define PM3D_CLIP_1IN '1'
#define PM3D_CLIP_4IN '4'
//
// is pm3d plotting style implicit or explicit?
//
enum PM3D_IMPL_MODE {
    PM3D_EXPLICIT = 0,
    PM3D_IMPLICIT = 1
};
//
// from which corner take the color?
//
enum PM3D_WHICH_CORNERS2COLOR {
    /* keep the following order of PM3D_WHICHCORNER_C1 .. _C4 */
    PM3D_WHICHCORNER_C1 = 0, 	/* corner 1: first scan, first point   */
    PM3D_WHICHCORNER_C2 = 1, 	/* corner 2: first scan, second point  */
    PM3D_WHICHCORNER_C3 = 2, 	/* corner 3: second scan, first point  */
    PM3D_WHICHCORNER_C4 = 3,	/* corner 4: second scan, second point */
    /* the rest can be in any order */
    PM3D_WHICHCORNER_MEAN    = 4, /* average z-value from all 4 corners */
    PM3D_WHICHCORNER_GEOMEAN = 5, /* geometrical mean of 4 corners */
    PM3D_WHICHCORNER_HARMEAN = 6, /* harmonic mean of 4 corners */
    PM3D_WHICHCORNER_MEDIAN  = 7, /* median of 4 corners */
    PM3D_WHICHCORNER_RMS     = 8, /* root mean square of 4 corners*/
    PM3D_WHICHCORNER_MIN     = 9, /* minimum of 4 corners */
    PM3D_WHICHCORNER_MAX     = 10,/* maximum of 4 corners */
    PM3D_COLOR_BY_NORMAL     = 11 /* derive color from surface normal (not currently used) */
};

/*
  structure defining all properties of pm3d plotting mode
  (except for the properties of the smooth color box, see GpGg.ColorBox instead)
*/
struct pm3d_struct {
	pm3d_struct()
	{
		/*
			pm3d_struct pm3d = {
				"s",                    // where[6] 
				PM3D_FLUSH_BEGIN,       // flush 
				0,                      // no flushing triangles 
				PM3D_SCANS_AUTOMATIC,   // scans direction is determined automatically 
				PM3D_CLIP_4IN,          // clipping: all 4 points in ranges 
				PM3D_EXPLICIT,          // implicit 
				PM3D_WHICHCORNER_MEAN,  // color from which corner(s) 
				1,                      // interpolate along scanline 
				1,                      // interpolate between scanlines 
				DEFAULT_LP_STYLE_TYPE   // for the border 
			};
		*/
		STRNSCPY(where, "s");
		flush = PM3D_FLUSH_BEGIN;
		ftriangles = 0;
		direction = PM3D_SCANS_AUTOMATIC;
		clip = PM3D_CLIP_4IN;
		implicit = PM3D_EXPLICIT;
		which_corner_color = PM3D_WHICHCORNER_MEAN;
		interp_i = 1;
		interp_j = 1;
		border.SetDefault2();
	}
	char where[7];	/* base, top, surface */
	char flush;   	/* left, right, center */
	char ftriangles;   	/* 0/1 (don't) draw flushing triangles */
	char direction;	/* forward, backward */
	char clip;		/* 1in, 4in */
	PM3D_IMPL_MODE implicit; // 1: [default] draw ALL surfaces with pm3d; 0: only surfaces specified with 'with pm3d'
	PM3D_WHICH_CORNERS2COLOR which_corner_color; // default: average color from all 4 points
	int interp_i;		/* # of interpolation steps along scanline */
	int interp_j;		/* # of interpolation steps between scanlines */
	lp_style_type border;	/* LT_NODRAW to disable.  From `set pm3d border <linespec> */
};

extern pm3d_struct pm3d;

struct lighting_model {
  double strength;	/* 0 = no lighting model; 1 = full shading */
  double spec;		/* specular component 0-1 */
  double ambient;	/* ambient component 0-1 */
  double Phong;		/* Phong exponent */
  int rot_z;		/* illumination angle */
  int rot_x;		/* illumination angle */
  bool fixed;	/* true means the light does not rotate */
};

extern lighting_model pm3d_shade;

/* Used to initialize `set pm3d border` */
// extern lp_style_type default_pm3d_border;

// Used by routine filled_quadrangle() in color.c 
extern lp_style_type pm3d_border_lp;	/* FIXME: Needed anymore? */
//
// Declaration of routines
//
int get_pm3d_at_option(char *pm3d_where);
void pm3d_depth_queue_clear();
void pm3d_depth_queue_flush();
void pm3d_reset();
void pm3d_draw_one(surface_points* plots);
double z2cb(double z);
double cb2gray(double cb);
void pm3d_rearrange_scan_array(surface_points* this_plot, iso_curve*** first_ptr, int* first_n, int* first_invert, iso_curve*** second_ptr, int* second_n, int* second_invert);
void set_plot_with_palette(int plot_num, int plot_mode);

bool is_plot_with_palette();
bool is_plot_with_colorbox();

#endif /* PM3D_H */
#endif /* TERM_HELP */

/* eof pm3d.h */
