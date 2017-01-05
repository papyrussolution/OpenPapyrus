/*
 * gadgets.h,v 1.1.3.1 2000/05/03 21:47:15 hbb Exp
 */

/* GNUPLOT - gadgets.h */

/*[
 * Copyright 2000, 2004   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_GADGETS_H
# define GNUPLOT_GADGETS_H

//#include "syscfg.h"
//#include "term_api.h"

/* Types and variables concerning graphical plot elements that are not
 * *terminal-specific, are used by both* 2D and 3D plots, and are not
 * *assignable to any particular * axis. I.e. they belong to neither
 * *term_api, graphics, graph3d, nor * axis .h files.
 */

/* #if... / #include / #define collection: */

/* Type definitions */

/* Coordinate system specifications: x1/y1, x2/y2, graph-box relative
 * or screen relative GpCoordinate systems */
enum position_type {
	first_axes,
	second_axes,
	graph,
	screen,
	character
};

//
// This is the default state for the axis, timestamp, and plot title labels indicated by tag = -2
//
#define NONROTATABLE_LABEL_TAG -2
#define ROTATE_IN_3D_LABEL_TAG -3
#define EMPTY_LABELSTRUCT \
	{NULL, NONROTATABLE_LABEL_TAG, \
	 {character, character, character, 0.0, 0.0, 0.0}, CENTRE, 0, 0, \
	 0, \
	 NULL, NULL, {TC_LT, -2, 0.0}, DEFAULT_LP_STYLE_TYPE, \
	 {character, character, character, 0.0, 0.0, 0.0}, false, \
	 false}
//
// A full 3D position, with all 3 coordinates of possible using different axes.
// Used for 'set label', 'set arrow' positions and various offsets.
//
struct t_position : public RPoint3 {
	void   Set(position_type sx, position_type sy, position_type sz, double _x, double _y, double _z)
	{
		scalex = sx;
		scaley = sy;
		scalez = sz;
		RPoint3::Set(_x, _y, _z);
	}
	void   SetDefaultMargin()
	{
		Set(character, character, character, -1.0, -1.0, -1.0);
	}
	void   SetDefaultKeyPosition()
	{
		//#define DEFAULT_KEY_POSITION { graph, graph, graph, 0.9, 0.9, 0. }
		scalex = graph;
		scaley = graph;
		scalez = graph;
		RPoint3::Set(0.9, 0.9, 0.0);
	}
	position_type scalex;
	position_type scaley;
	position_type scalez;
	//double x;
	//double y;
	//double z;
};

//
// Linked list of structures storing 'set label' information
//
struct text_label {
	text_label()
	{
		SetEmpty();
	}
	void    SetEmpty()
	{
		/*
		   #define EMPTY_LABELSTRUCT \
		                {NULL, NONROTATABLE_LABEL_TAG, \
		                {character, character, character, 0.0, 0.0, 0.0}, CENTRE, 0, 0, \
		                0, \
		                NULL, NULL, {TC_LT, -2, 0.0}, DEFAULT_LP_STYLE_TYPE, \
		                {character, character, character, 0.0, 0.0, 0.0}, false, \
		                false}
		 */
		next = 0;
		tag = NONROTATABLE_LABEL_TAG;
		place.Set(character, character, character, 0.0, 0.0, 0.0);
		pos = CENTRE;
		rotate = 0;
		layer = 0;
		boxed = 0;
		text = 0;
		font = 0;
		textcolor.type = TC_LT;
		textcolor.lt = -2;
		textcolor.value = 0.0;
		lp_properties.SetDefault();
		offset.Set(character, character, character, 0.0, 0.0, 0.0);
		noenhanced = false;
		hypertext = false;
	}
	//
	// process 'unset [xyz]{2}label command
	//
	//static void unset_axislabel_or_title(text_label * label)
	void UnsetAxisLabelOrTitle()
	{
		t_position default_offset;
		default_offset.Set(character, character, character, 0., 0., 0.);
		ZFREE(text);
		ZFREE(font);
		offset = default_offset;
		textcolor.type = TC_DEFAULT;
	}
	text_label * next; // pointer to next label in linked list
	int tag; // identifies the label
	t_position place;
	enum JUSTIFY pos; // left/center/right horizontal justification

	int rotate;
	int layer;
	int boxed; // EAM_BOXED_TEXT
	char * text;
	char * font; // Entry font added by DJL
	t_colorspec textcolor;
	lp_style_type lp_properties;
	t_position offset;
	bool noenhanced;
	bool hypertext;
};

/* Datastructure for implementing 'set arrow' */
enum arrow_type {
	arrow_end_absolute,
	arrow_end_relative,
	arrow_end_oriented
};

struct arrow_def {
	arrow_def * next; /* pointer to next arrow in linked list */
	int tag;                /* identifies the arrow */
	arrow_type type;        /* how to interpret t_position end */
	t_position start;
	t_position end;
	double angle;           /* angle in degrees if type arrow_end_oriented */
	arrow_style_type arrow_properties;
};

#ifdef EAM_OBJECTS
/* The object types supported so far are OBJ_RECTANGLE, OBJ_CIRCLE, and OBJ_ELLIPSE */
struct t_rectangle {
	void   SetDefault()
	{
		THISZERO();
	}

	int type;      // 0 = corners;  1 = center + size
	t_position center; // center
	t_position extent; // width and height
	t_position bl; // bottom left
	t_position tr; // top right
};

#define DEFAULT_RADIUS (-1.0)
#define DEFAULT_ELLIPSE (-2.0)

struct t_circle {
	void   SetDefault()
	{
		// {.circle = {1, {0,0,0,0.,0.,0.}, {graph,0,0,0.02,0.,0.}, 0., 360., true }} }
		type = 1;
		MEMSZERO(center);
		extent.scalex = graph;
		extent.scaley = first_axes;
		extent.scalez = first_axes;
		extent.x = 0.02;
		extent.y = 0.0;
		extent.z = 0.0;
		arc_begin = 0.0;
		arc_end = 360.0;
		wedge = true;
	}

	int type;               /* not used */
	t_position center;      /* center */
	t_position extent;      /* radius */
	double arc_begin;
	double arc_end;
	bool wedge;         /* true = connect arc ends to center */
};

#define ELLIPSEAXES_XY (0)
#define ELLIPSEAXES_XX (1)
#define ELLIPSEAXES_YY (2)

struct t_ellipse {
	void   SetDefault()
	{
		// {.ellipse = {ELLIPSEAXES_XY, {0,0,0,0.,0.,0.}, {graph,graph,0,0.05,0.03,0.}, 0. }} }
		type = ELLIPSEAXES_XY;
		MEMSZERO(center);
		extent.scalex = graph;
		extent.scaley = graph;
		extent.scalez = first_axes;
		extent.x = 0.05;
		extent.y = 0.03;
		extent.z = 0.0;
		orientation = 0.0;
	}

	int type;               /* mapping of axes: ELLIPSEAXES_XY, ELLIPSEAXES_XX or ELLIPSEAXES_YY */
	t_position center;      /* center */
	t_position extent;      /* major and minor axes */
	double orientation;     /* angle of first axis to horizontal */
};

struct t_polygon {
	void   SetDefault()
	{
		// {.polygon = {0, NULL} } }
		type = 0;
		vertex = 0;
	}
	int    type;          // Number of vertices
	t_position * vertex;  // Array of vertices
};

enum t_clip_object {
	OBJ_CLIP,       // Clip to graph unless GpCoordinate type is screen 
	OBJ_NOCLIP,     // Clip to canvas, never to graph 
	OBJ_ALWAYS_CLIP // Not yet implemented 
};

#define OBJ_RECTANGLE (1)
#define OBJ_CIRCLE (2)
#define OBJ_ELLIPSE (3)
#define OBJ_POLYGON (4)
//
// Datastructure for 'set object'
//
struct t_object {
	enum {
		defUnkn = 0,
		defRectangle = 1,
		defCircle,
		defEllipse,
		defPolygon
	};

	t_object(int def)
	{
		if(def == defRectangle)
			SetDefaultRectangleStyle();
		else if(def == defCircle)
			SetDefaultCircleStyle();
		else if(def == defEllipse)
			SetDefaultEllipseStyle();
		else if(def == defPolygon)
			SetDefaultPolygonStyle();
	}

	void SetDefaultRectangleStyle()
	{
		/*
		   #define DEFAULT_RECTANGLE_STYLE
		        {
		                NULL,
		                -1,
		                0,
		                OBJ_RECTANGLE,
		                OBJ_CLIP,
		                { FS_SOLID, 100, 0, BLACK_COLORSPEC},
		                {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC,
		                   DEFAULT_DASHPATTERN},
		                {.rectangle = {0, {0,0,0,0.,0.,0.}, {0,0,0,0.,0.,0.}, {0,0,0,0.,0.,0.},
		                   {0,0,0,0.,0.,0.}}}
		        }
		 */
		next = 0;
		tag = -1;
		layer = 0;
		object_type = OBJ_RECTANGLE;
		clip = OBJ_CLIP;
		fillstyle.SetDefault();
		lp_properties.SetDefault();
		o.rectangle.SetDefault();
	}

	void SetDefaultCircleStyle()
	{
		/*
		   #define DEFAULT_CIRCLE_STYLE { NULL, -1, 0, OBJ_CIRCLE, OBJ_CLIP, \
		                {FS_SOLID, 100, 0, BLACK_COLORSPEC},                    \
		                {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC,
		                   DEFAULT_DASHPATTERN}, \
		                {.circle = {1, {0,0,0,0.,0.,0.}, {graph,0,0,0.02,0.,0.}, 0., 360., true }} }
		 */
		next = 0;
		tag = -1;
		layer = 0;
		object_type = OBJ_CIRCLE;
		clip = OBJ_CLIP;
		fillstyle.SetDefault();
		lp_properties.SetDefault();
		o.circle.SetDefault();
	}

	void SetRAxisCircleStyle()
	{
		next = 0;
		tag = 1;
		layer = 1;
		object_type = OBJ_CIRCLE;
		clip = OBJ_CLIP;
		fillstyle.SetDefault();
		lp_properties.SetDefault();
		lp_properties.l_width = 0.2;
		o.circle.SetDefault();
		/*
		        t_object raxis_circle = {
		                NULL, 1, 1, OBJ_CIRCLE, OBJ_CLIP, // link, tag, layer (front), object_type, clip //
		                {FS_SOLID, 100, 0, BLACK_COLORSPEC},
		                {0, LT_BACKGROUND, 0, DASHTYPE_AXIS, 0, 0.2, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC,
		                   DEFAULT_DASHPATTERN},
		                {.circle = {1, {0, 0, 0, 0., 0., 0.}, {graph, 0, 0, 0.02, 0., 0.}, 0., 360. }}
		        };
		 */
	}

	void SetDefaultEllipseStyle()
	{
		/*
		   #define DEFAULT_ELLIPSE_STYLE { NULL, -1, 0, OBJ_ELLIPSE, OBJ_CLIP, \
		                {FS_SOLID, 100, 0, BLACK_COLORSPEC},                    \
		                {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC,
		                   DEFAULT_DASHPATTERN}, \
		                {.ellipse = {ELLIPSEAXES_XY, {0,0,0,0.,0.,0.}, {graph,graph,0,0.05,0.03,0.}, 0. }} }
		 */
		next = 0;
		tag = -1;
		layer = 0;
		object_type = OBJ_ELLIPSE;
		clip = OBJ_CLIP;
		fillstyle.SetDefault();
		lp_properties.SetDefault();
		o.ellipse.SetDefault();
	}

	void SetDefaultPolygonStyle()
	{
		/*
		   #define DEFAULT_POLYGON_STYLE { NULL, -1, 0, OBJ_POLYGON, OBJ_CLIP, \
		                {FS_SOLID, 100, 0, BLACK_COLORSPEC},                    \
		                {0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 1.0, 0.0, DEFAULT_P_CHAR, BLACK_COLORSPEC,
		                   DEFAULT_DASHPATTERN}, \
		                {.polygon = {0, NULL} } }
		 */
		next = 0;
		tag = -1;
		layer = 0;
		object_type = OBJ_POLYGON;
		clip = OBJ_CLIP;
		fillstyle.SetDefault();
		lp_properties.SetDefault();
		lp_properties.l_type = LT_BLACK;
		o.polygon.SetDefault();
	}

	t_object * next;
	int tag;
	int layer;              /* behind or back or front */
	int object_type;        /* OBJ_RECTANGLE */
	t_clip_object clip;
	fill_style_type fillstyle;
	lp_style_type lp_properties;
	union {
		t_rectangle rectangle;
		t_circle circle;
		t_ellipse ellipse;
		t_polygon polygon;
	} o;
};

#endif
//
// Datastructure implementing 'set dashtype' 
//
struct custom_dashtype_def {
	custom_dashtype_def * next; /* pointer to next dashtype in linked list */
	int tag;                /* identifies the dashtype */
	int d_type;             /* for DASHTYPE_SOLID or CUSTOM */
	struct t_dashtype dashtype;
};

//
// Datastructure implementing 'set style line'
//
struct linestyle_def {
	linestyle_def * next;   /* pointer to next linestyle in linked list */
	int tag;                /* identifies the linestyle */
	lp_style_type lp_properties;
};

/* Datastructure implementing 'set style arrow' */
struct arrowstyle_def {
	arrowstyle_def * next; /* pointer to next arrowstyle in linked list */
	int tag;                /* identifies the arrowstyle */
	arrow_style_type arrow_properties;
};

//
// Plot layer definitions are collected here.
// Someday they might actually be used.
//
#define LAYER_BEHIND     -1
#define LAYER_BACK        0
#define LAYER_FRONT       1
#define LAYER_FOREGROUND  2     /* not currently used */
#define LAYER_PLOTLABELS 99
//
// For 'set style parallelaxis'
//
struct pa_style {
	pa_style() : lp_properties(lp_style_type::defParallelAxis)
	{
		layer = LAYER_FRONT;
	}

	lp_style_type lp_properties; /* used to draw the axes themselves */
	int layer;              /* front/back */
};

#define DEFAULT_PARALLEL_AXIS_STYLE {{0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 2.0, 0.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, \
	DEFAULT_DASHPATTERN}, LAYER_FRONT }

/* The stacking direction of the key box: (vertical, horizontal) */
typedef enum en_key_stack_direction {
	GPKEY_VERTICAL,
	GPKEY_HORIZONTAL
} t_key_stack_direction;

/* The region, with respect to the border, key is located: (inside, outside) */
typedef enum en_key_region {
	GPKEY_AUTO_INTERIOR_LRTBC, /* Auto placement, left/right/top/bottom/center */
	GPKEY_AUTO_EXTERIOR_LRTBC, /* Auto placement, left/right/top/bottom/center */
	GPKEY_AUTO_EXTERIOR_MARGIN, /* Auto placement, margin plus lrc or tbc */
	GPKEY_USER_PLACEMENT     /* User specified placement */
} t_key_region;

/* If exterior, there are 12 possible auto placements.  Since
   left/right/center with top/bottom/center can only define 9
   locations, further subdivide the exterior region into four
   subregions for which left/right/center (TMARGIN/BMARGIN)
   and top/bottom/center (LMARGIN/RMARGIN) creates 12 locations. */
typedef enum en_key_ext_region {
	GPKEY_TMARGIN,
	GPKEY_BMARGIN,
	GPKEY_LMARGIN,
	GPKEY_RMARGIN
} t_key_ext_region;

/* Key sample to the left or the right of the plot title? */
typedef enum en_key_sample_positioning {
	GPKEY_LEFT,
	GPKEY_RIGHT
} t_key_sample_positioning;

struct filledcurves_opts {
	filledcurves_opts()
	{
		THISZERO();
	}

	int opt_given; /* option given / not given (otherwise default) */
	int closeto; /* from list FILLEDCURVES_CLOSED, ... */
	double at; /* value for FILLEDCURVES_AT... */
	double aty; /* the other value for FILLEDCURVES_ATXY */
	int oneside; /* -1 if fill below bound only; +1 if fill above bound only */
};

//#define EMPTY_FILLEDCURVES_OPTS { 0, 0, 0.0, 0.0, 0 }

enum t_histogram_type /*histogram_type*/ {
	HT_NONE,
	HT_STACKED_IN_LAYERS,
	HT_STACKED_IN_TOWERS,
	HT_CLUSTERED,
	HT_ERRORBARS
};

struct histogram_style {
	histogram_style()
	{
		//#define DEFAULT_HISTOGRAM_STYLE { HT_CLUSTERED, 2, 1, 0.0, 0.0, LT_UNDEFINED, LT_UNDEFINED, 0, NULL,
		// EMPTY_LABELSTRUCT }
		type = HT_CLUSTERED;
		gap = 2;
		clustersize = 1;
		start = 0.0;
		end = 0.0;
		startcolor = LT_UNDEFINED;
		startpattern = LT_UNDEFINED;
		bar_lw = 0.0;
		next = 0;
		title.SetEmpty();
	}

	int type;        // enum t_histogram_type
	int gap;         // set style hist gap <n> (space between clusters)
	int clustersize; // number of datasets in this histogram
	double start;    // X-coord of first histogram entry
	double end;      // X-coord of last histogram entry
	int startcolor;  // LT_UNDEFINED or explicit color for first entry
	int startpattern; // LT_UNDEFINED or explicit pattern for first entry
	double bar_lw;   // linewidth for error bars
	histogram_style * next;
	text_label title;
};

#define DEFAULT_HISTOGRAM_STYLE { HT_CLUSTERED, 2, 1, 0.0, 0.0, LT_UNDEFINED, LT_UNDEFINED, 0, NULL, EMPTY_LABELSTRUCT }

typedef enum en_boxplot_factor_labels {
	BOXPLOT_FACTOR_LABELS_OFF,
	BOXPLOT_FACTOR_LABELS_AUTO,
	BOXPLOT_FACTOR_LABELS_X,
	BOXPLOT_FACTOR_LABELS_X2
} t_boxplot_factor_labels;

#define DEFAULT_BOXPLOT_FACTOR -1

typedef struct boxplot_style {
	int limit_type; /* 0 = multiple of interquartile 1 = fraction of points */
	double limit_value;
	bool outliers;
	int pointtype;
	int plotstyle;  /* CANDLESTICKS or FINANCEBARS */
	double separation; /* of boxplots if there are more than one factors */
	t_boxplot_factor_labels labels; /* Which axis to put the tic labels if there are factors */
	bool sort_factors;  /* Sort factors in alphabetical order? */
} boxplot_style;

extern boxplot_style boxplot_opts;
#define DEFAULT_BOXPLOT_STYLE { 0, 1.5, true, 6, CANDLESTICKS, 1.0, BOXPLOT_FACTOR_LABELS_AUTO, false }

#ifdef EAM_BOXED_TEXT
typedef struct textbox_style {
	bool opaque; /* True if the box is background-filled before writing into it */
	bool noborder; /* True if you want fill only, no lines */
	double xmargin; /* fraction of default margin to use */
	double ymargin; /* fraction of default margin to use */
} textbox_style;
#define DEFAULT_TEXTBOX_STYLE { false, false, 1.0, 1.0 }
#endif

/***********************************************************/
/* Variables defined by gadgets.c needed by other modules. */
/***********************************************************/
//
// bounding box position, in terminal coordinates
//
struct BoundingBox {
	void   SetZero()
	{
		THISZERO();
	}
	int    xleft;
	int    xright;
	int    ybot;
	int    ytop;
};
//
// EAM Feb 2003 - Move all global variables related to key into a
// single structure. Eventually this will allow multiple keys.  
//
enum keytitle_type {
	NOAUTO_KEYTITLES,
	FILENAME_KEYTITLES,
	COLUMNHEAD_KEYTITLES
};

struct legend_key {
	legend_key()
	{
		SetDefault();
	}

	void   SetDefault()
	{
		/*
		   #define DEFAULT_KEY_PROPS \
		                        { true, \
		                        GPKEY_AUTO_INTERIOR_LRTBC, GPKEY_RMARGIN, \
		                        DEFAULT_KEY_POSITION, \
		                        JUST_TOP, RIGHT, \
		                        GPKEY_RIGHT, GPKEY_VERTICAL, \
		                        4.0, 1.0, 0.0, 0.0, \
		                        FILENAME_KEYTITLES, \
		                        false, false, false, true, \
		                        DEFAULT_KEYBOX_LP, \
		                        NULL, {TC_LT, LT_BLACK, 0.0}, \
		                        {0,0,0,0}, 0, 0, \
		                        EMPTY_LABELSTRUCT}
		 */
		visible = true;
		region = GPKEY_AUTO_INTERIOR_LRTBC;
		margin = GPKEY_RMARGIN;
		user_pos.SetDefaultKeyPosition();
		vpos = JUST_TOP;
		hpos = RIGHT;
		just = GPKEY_RIGHT;
		stack_dir = GPKEY_VERTICAL;
		swidth = 4.0;
		vert_factor = 1.0;
		width_fix = 0.0;
		height_fix = 0.0;
		auto_titles = FILENAME_KEYTITLES;
		front = false;
		reverse = false;
		invert = false;
		enhanced = true;
		box.SetDefaultKeybox();
		font = 0;
		{
			textcolor.type = TC_LT;
			textcolor.lt = LT_BLACK;
			textcolor.value = 0.0;
		}
		MEMSZERO(bounds);
		maxcols = 0;
		maxrows = 0;
		title.SetEmpty();
	}

	bool visible;       // Do we show this key at all?
	t_key_region region;    // if so: where?
	t_key_ext_region margin; // if exterior: where outside?
	t_position user_pos;    // if user specified position, this is it
	VERT_JUSTIFY vpos;      // otherwise these guide auto-positioning
	JUSTIFY hpos;
	t_key_sample_positioning just;
	t_key_stack_direction stack_dir;
	double swidth;          // 'width' of the linestyle sample line in the key
	double vert_factor;     // user specified vertical spacing multiplier
	double width_fix;       // user specified additional (+/-) width of key titles
	double height_fix;
	keytitle_type auto_titles; // auto title curves unless plotted 'with notitle'
	bool front;         // draw key in a second pass after the rest of the graph
	bool reverse;       // key back to front
	bool invert;        // key top to bottom
	bool enhanced;      // enable/disable enhanced text of key titles
	lp_style_type box; // linetype of box around key:
	char * font;                    // Will be used for both key title and plot titles
	t_colorspec textcolor;  // Will be used for both key title and plot titles
	BoundingBox bounds;
	int maxcols;            // maximum no of columns for horizontal keys
	int maxrows;            // maximum no of rows for vertical keys
	text_label title;       // holds title line for the key as a whole
};

extern legend_key keyT;
//
// EAM Jan 2006 - Move colorbox structure definition to here from color.h
// in order to be able to use t_position
//
#define SMCOLOR_BOX_NO      'n'
#define SMCOLOR_BOX_DEFAULT 'd'
#define SMCOLOR_BOX_USER    'u'

struct color_box_struct {
	void   SetDefault()
	{
		where = SMCOLOR_BOX_DEFAULT;
		rotation = 'v';
		border = 1;
		border_lt_tag = LT_BLACK;
		layer = LAYER_FRONT;
		xoffset = 0;
		origin.Set(screen, screen, screen, 0.90, 0.2, 0.0);
		size.Set(screen, screen, screen, 0.05, 0.6, 0.0);
		bounds.SetZero();
	}
	char   where;
	// where
	// SMCOLOR_BOX_NO .. do not draw the colour box
	// SMCOLOR_BOX_DEFAULT .. draw it at default position and size
	// SMCOLOR_BOX_USER .. draw it at the position given by user
	//
	char   rotation; /* 'v' or 'h' vertical or horizontal box */
	char   border; /* if non-null, a border will be drawn around the box (default) */
	int    border_lt_tag;
	int    layer; /* front or back */
	int    xoffset; /* To adjust left or right, e.g. for y2tics */
	t_position origin;
	t_position size;
	BoundingBox bounds;
};

#define DEFAULT_MARGIN_POSITION {character, character, character, -1, -1, -1}
#define ZERO 1e-8 // default for 'zero' set option
//
// A macro to check whether 2D functionality is allowed in the last plot:
// either the plot is a 2D plot, or it is a suitably oriented 3D plot (e.g. map).
//
#define ALMOST2D (!GpGg.Is3DPlot || Gp3Gr.splot_map || (fabs(fmod(Gp3Gr.surface_rot_z, 90.0f))<0.1 && fabs(fmod(Gp3Gr.surface_rot_x, 180.0f))<0.1))
//#define SET_REFRESH_OK(ok, nplots) do { GpGg.RefreshOk = (ok); GpGg.RefreshNPlots = (nplots); } while(0)
#define SAMPLES 100             /* default number of samples for a plot */
#ifndef DEFAULT_TIMESTAMP_FORMAT
	#define DEFAULT_TIMESTAMP_FORMAT "%a %b %d %H:%M:%S %Y" // asctime() format
#endif
#define SOUTH           1 /* 0th bit */
#define WEST            2 /* 1th bit */
#define NORTH           4 /* 2th bit */
#define EAST            8 /* 3th bit */
#define border_east     (GpGg.DrawBorder & EAST)
#define border_west     (GpGg.DrawBorder & WEST)
#define border_south    (GpGg.DrawBorder & SOUTH)
#define border_north    (GpGg.DrawBorder & NORTH)
#define border_complete ((GpGg.DrawBorder & 15) == 15)

enum TRefresh_Allowed {
	E_REFRESH_NOT_OK = 0,
	E_REFRESH_OK_2D = 2,
	E_REFRESH_OK_3D = 3
};

class GpGadgets {
public:
	GpGadgets() :
		DefaultBorderLp(lp_style_type::defBorder),
		BackgroundLp(lp_style_type::defBkg),
		BorderLp(lp_style_type::defBorder),
		DefaultFillStyle(FS_EMPTY, 100, 0, t_colorspec(TC_DEFAULT, 0, 0.0))
	{
		/*
		default_color_box = {
			SMCOLOR_BOX_DEFAULT, 'v', 1, LT_BLACK, LAYER_FRONT, 0,
			{screen, screen, screen, 0.90, 0.2, 0.0},
			{screen, screen, screen, 0.05, 0.6, 0.0},
			{0, 0, 0, 0} };
		*/
		//DefColorBox.SetDefault();
		P_Clip = &PlotBounds;
		XSz = 1.0f;              // scale factor for size 
		YSz = 1.0f;              // scale factor for size 
		ZSz = 1.0f;              // scale factor for size 
		XOffs = 0.0f;            // x origin 
		YOffs = 0.0f;            // y origin 
		AspectRatio = 0.0f;       // don't attempt to force it 
		AspectRatio3D = 0;       // 2 will put x and y on same scale, 3 for z also 
		// EAM Augest 2006 - redefine margin as t_position so that absolute placement is possible
		LMrg.SetDefaultMargin(); // space between left edge and GpGg.PlotBounds.xleft in chars (-1: computed) 
		BMrg.SetDefaultMargin(); // space between bottom and GpGg.PlotBounds.ybot in chars (-1: computed) 
		RMrg.SetDefaultMargin(); // space between right egde and GpGg.PlotBounds.xright in chars (-1: computed) 
		TMrg.SetDefaultMargin(); // space between top egde and GpGg.PlotBounds.ytop in chars (-1: computed) 

		first_custom_dashtype = 0;
		first_arrow = 0;
		first_label = 0;
		first_linestyle = 0;
		first_perm_linestyle = 0;
		first_mono_linestyle = 0;
		first_arrowstyle = 0;
#ifdef EAM_OBJECTS
		first_object = 0;
#endif
		timelabel_rotate = false;
		timelabel_bottom = true;
		zero = ZERO; // zero threshold, may _not_ be 0! 
		pointsize = 1.0;
		pointintervalbox = 1.0;

		//const lp_style_type GpGg.DefaultBorderLp(lp_style_type::defBorder); // = DEFAULT_BORDER_LP;
		//const lp_style_type GpGg.BackgroundLp(lp_style_type::defBkg); // = {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, DEFAULT_DASHPATTERN};
		//lp_style_type GpGg.BorderLp(lp_style_type::defBorder); // = DEFAULT_BORDER_LP;
		// set border
		DrawBorder = 31;   // The current settings
		UserBorder = 31;   // What the user last set explicitly
		BorderLayer = LAYER_FRONT;
		// set samples 
		Samples1 = SAMPLES;
		Samples2 = SAMPLES;
		RefreshNPlots = 0; // FIXME: do_plot should be able to figure this out on its own! 
		current_x11_windowid = 0; // WINDOWID to be filled by terminals running on X11 (x11, wxt, qt, ...) 
		// set angles 
		Ang2Rad = 1.0;         /* 1 or pi/180, tracking angles_format */
		DataStyle = POINTSTYLE;
		FuncStyle = LINES;
		RefreshOk = E_REFRESH_NOT_OK; /* Flag to signal that the existing data is valid for a quick refresh */
		//fill_style_type GpGg.DefaultFillStyle = { FS_EMPTY, 100, 0, DEFAULT_COLORSPEC };
		//fill_style_type GpGg.DefaultFillStyle(FS_EMPTY, 100, 0, t_colorspec(TC_DEFAULT, 0, 0.0));
		// filledcurves style options 
		//filledcurves_opts GpGg.FilledcurvesOptsData; // = EMPTY_FILLEDCURVES_OPTS;
		//filledcurves_opts GpGg.FilledcurvesOptsFunc; // = EMPTY_FILLEDCURVES_OPTS;

		IsPolar = false;
		ClipLines1 = false;
		ClipLines2 = false;
		ClipPoints = false;
		IsParametric = false;
		InParametric = false;
		Is3DPlot = false; // If last plot was a 3d one
		IsVolatileData = false;
	}
	void   SetRefreshOk(TRefresh_Allowed ok, int nplots)
	{
		RefreshOk = ok;
		RefreshNPlots = nplots;
	}
	void UnsetFillStyle()
	{
		DefaultFillStyle.fillstyle = FS_EMPTY;
		DefaultFillStyle.filldensity = 100;
		DefaultFillStyle.fillpattern = 0;
		DefaultFillStyle.border_color.type = TC_DEFAULT;
	}
	//
	// delete arrow from linked list started by GpGg.first_arrow.
	// called with pointers to the previous arrow (prev) and the arrow to delete (this).
	// If there is no previous arrow (the arrow to delete is GpGg.first_arrow) then call with prev = NULL.
	//
	void DeleteArrow(arrow_def * pPrev, arrow_def * pArr)
	{
		if(pArr) { // there really is something to delete
			if(pPrev) // there is a previous arrow 
				pPrev->next = pArr->next;
			else // pArr = GpGg.first_arrow so change GpGg.first_arrow
				first_arrow = pArr->next;
			free(pArr);
		}
	}
	void DestroyArrows()
	{
		while(first_arrow)
			DeleteArrow(0, first_arrow);
	}
	//
	// delete label from linked list started by GpGg.first_label.
	// called with pointers to the previous label (prev) and the label to delete (this).
	// If there is no previous label (the label to delete is GpGg.first_label) then call with prev = NULL.
	//
	void DeleteLabel(text_label * pPrev, text_label * pLab)
	{
		if(pLab) { // there really is something to delete 
			if(pPrev) // there is a previous label 
				pPrev->next = pLab->next;
			else            // pLab = GpGg.first_label so change GpGg.first_label 
				first_label = pLab->next;
			free(pLab->text);
			free(pLab->font);
			free(pLab);
		}
	}
	void DestroyLabeles()
	{
		while(first_label)
			DeleteLabel(0, first_label);
	}
	color_box_struct ColorBox;
	//color_box_struct DefColorBox;
	BoundingBox PlotBounds; // Plot Boundary
	BoundingBox Canvas;      // Writable area on terminal
	BoundingBox * P_Clip; // Current clipping box

	float  XSz;             /* x scale factor for size */
	float  YSz;             /* y scale factor for size */
	float  ZSz;             /* z scale factor for size */
	float  XOffs;           /* x origin setting */
	float  YOffs;           /* y origin setting */
	float  AspectRatio;      /* 1.0 for square */
	int    AspectRatio3D;     /* 2 for equal scaling of x and y; 3 for z also */
	// plot border autosizing overrides, in characters (-1: autosize)
	t_position LMrg;
	t_position BMrg;
	t_position RMrg;
	t_position TMrg; 

	custom_dashtype_def * first_custom_dashtype;
	arrow_def * first_arrow;
	text_label * first_label;
	linestyle_def * first_linestyle;
	linestyle_def * first_perm_linestyle;
	linestyle_def * first_mono_linestyle;
	arrowstyle_def * first_arrowstyle;
#ifdef EAM_OBJECTS
	t_object * first_object; // Pointer to first object instance in linked list
#endif
	text_label title;
	text_label timelabel;
	int    timelabel_rotate;
	int    timelabel_bottom;
	double zero;             // zero threshold, not 0! 
	double pointsize;
	double pointintervalbox;

	const  lp_style_type BackgroundLp;
	const  lp_style_type DefaultBorderLp;
	lp_style_type BorderLp;
	int    DrawBorder;
	int    UserBorder;
	int    BorderLayer;
	int    Samples1;
	int    Samples2;
	int    RefreshNPlots;
	int    current_x11_windowid; // WINDOWID to be filled by terminals running on X11 (x11, wxt, qt, ...)
	double Ang2Rad; // 1 or pi/180
	enum PLOT_STYLE DataStyle;
	enum PLOT_STYLE FuncStyle;
	TRefresh_Allowed RefreshOk;
	fill_style_type DefaultFillStyle;
	filledcurves_opts FilledcurvesOptsData; // filledcurves style options set by 'set style [data|func] filledcurves opts'
	filledcurves_opts FilledcurvesOptsFunc;

	bool IsPolar;
	bool ClipLines1;
	bool ClipLines2;
	bool ClipPoints;
	bool IsParametric;
	bool InParametric;
	bool Is3DPlot; // If last plot was a 3d one
	bool IsVolatileData;
};

extern GpGadgets GpGg;

//extern color_box_struct GpGg.ColorBox;
//extern color_box_struct default_color_box;
//
// Holder for various image properties
//
struct t_image {
	t_imagecolor type; /* See above */
	bool fallback; /* true == don't use terminal-specific code */
	// image dimensions
	uint ncols;
	uint nrows;
};

//extern BoundingBox GpGg.PlotBounds; /* Plot Boundary */
//extern BoundingBox GpGg.Canvas;      /* Writable area on terminal */
//extern BoundingBox * GpGg.P_Clip;  /* Current clipping box */
//extern float xsize;             // x scale factor for size 
//extern float ysize;             // y scale factor for size 
//extern float zsize;             // z scale factor for size 
//extern float xoffset;           // x origin setting 
//extern float yoffset;           // y origin setting 
//extern float aspect_ratio;      // 1.0 for square 
//extern int aspect_ratio_3D;     // 2 for equal scaling of x and y; 3 for z also 
//extern t_position lmargin, bmargin, GpGg.RMrg, GpGg.TMrg; // plot border autosizing overrides, in characters (-1: autosize)
/*
extern custom_dashtype_def * first_custom_dashtype;
extern arrow_def * first_arrow;
extern text_label * first_label;
extern linestyle_def * first_linestyle;
extern linestyle_def * first_perm_linestyle;
extern linestyle_def * first_mono_linestyle;
extern arrowstyle_def * first_arrowstyle;
*/
extern pa_style parallel_axis_style;

#ifdef EAM_OBJECTS
//extern t_object * first_object;
#endif
/*
extern text_label title;
extern text_label timelabel;
extern int timelabel_rotate;
extern int timelabel_bottom;
extern double zero;             // zero threshold, not 0! 
extern double pointsize;
extern double pointintervalbox;
*/
/*
extern const lp_style_type GpGg.BackgroundLp;
extern const lp_style_type GpGg.DefaultBorderLp;
extern int GpGg.DrawBorder;
extern int user_border;
extern int GpGg.BorderLayer;
extern lp_style_type GpGg.BorderLp;
extern int GpGg.Samples1;
extern int GpGg.Samples2;
extern double ang2rad; // 1 or pi/180
extern enum PLOT_STYLE GpGg.DataStyle;
extern enum PLOT_STYLE GpGg.FuncStyle;
extern TRefresh_Allowed GpGg.RefreshOk;
extern int GpGg.RefreshNPlots;
extern int current_x11_windowid; // WINDOWID to be filled by terminals running on X11 (x11, wxt, qt, ...)
extern fill_style_type GpGg.DefaultFillStyle;
// filledcurves style options set by 'set style [data|func] filledcurves opts'
extern filledcurves_opts GpGg.FilledcurvesOptsData;
extern filledcurves_opts GpGg.FilledcurvesOptsFunc;
*/

// Prefer line styles over plain line types
#if 1 || defined(BACKWARDS_COMPATIBLE)
	extern bool prefer_line_styles;
#else
	#define prefer_line_styles false
#endif
extern histogram_style histogram_opts;
#ifdef EAM_BOXED_TEXT
	extern textbox_style textbox_opts;
#endif
//
// Functions exported by gadgets.c
//

// moved here from util3d:
void   draw_clip_line(int, int, int, int);
void   draw_clip_polygon(int, gpiPoint *);
void   draw_clip_arrow(int, int, int, int, int);
void   clip_polygon(gpiPoint *, gpiPoint *, int, int *);
int    clip_point(uint, uint);
void   clip_put_text(uint, uint, char *);
//
// moved here from graph3d:
//
void   clip_move(uint x, uint y);
void   clip_vector(uint x, uint y);
//
// Common routines for setting line or text color from t_colorspec
//
void   apply_pm3dcolor(t_colorspec * tc);
void   reset_textcolor(const t_colorspec * tc);

void default_arrow_style(arrow_style_type * arrow);
void apply_head_properties(arrow_style_type * arrow_properties);
void free_labels(text_label * tl);
void get_offsets(text_label * this_label, int * htic, int * vtic);
void write_label(uint x, uint y, text_label * label);
int label_width(const char *, int *);

#ifdef EAM_OBJECTS
	// Warning: C89 does not like the union initializers 
	extern t_object default_rectangle;
	extern t_object default_circle;
	extern t_object default_ellipse;
#endif

#endif /* GNUPLOT_GADGETS_H */
