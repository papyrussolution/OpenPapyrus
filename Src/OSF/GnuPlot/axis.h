/*
 * $Id: axis.h,v 1.144 2016/04/24 17:41:18 sfeam Exp $
 *
 */

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

#ifndef GNUPLOT_AXIS_H
#define GNUPLOT_AXIS_H

#ifndef DISABLE_NONLINEAR_AXES
	#define NONLINEAR_AXES
#endif

#include <gnuplot.h>
#include <stddef.h>		/* for offsetof() */
//#include "alloc.h"
//#include "gp_types.h"		/* for bool */
//#include "gadgets.h"
//#include "parse.h"		/* for const_*() */
//#include "tables.h"		/* for the axis name parse table */
//#include "util.h"		/* for int_error() */

/* typedefs / #defines */

/* give some names to some array elements used in command.c and grap*.c
 * maybe one day the relevant items in setshow will also be stored
 * in arrays.
 *
 * Always keep the following conditions alive:
 * SECOND_X_AXIS = FIRST_X_AXIS + SECOND_AXES
 * FIRST_X_AXIS & SECOND_AXES == 0
 */
#ifndef MAX_PARALLEL_AXES
	#define MAX_PARALLEL_AXES MAXDATACOLS-1
#endif

enum AXIS_INDEX {
    NO_AXIS = -2,
    ALL_AXES = -1,
    FIRST_Z_AXIS = 0,
#define FIRST_AXES FIRST_Z_AXIS
    FIRST_Y_AXIS,
    FIRST_X_AXIS,
    COLOR_AXIS,			/* fill gap */
    SECOND_Z_AXIS,		/* not used, yet */
#define SECOND_AXES SECOND_Z_AXIS
    SAMPLE_AXIS=SECOND_Z_AXIS,
    SECOND_Y_AXIS,
    SECOND_X_AXIS,
    POLAR_AXIS,
#define NUMBER_OF_MAIN_VISIBLE_AXES (POLAR_AXIS + 1)
    T_AXIS,
    U_AXIS,
    V_AXIS,		/* Last index into GpAxB[] */
    PARALLEL_AXES,	/* Parallel axis data is allocated dynamically */

    AXIS_ARRAY_SIZE = PARALLEL_AXES
};

/* HBB NOTE 2015-01-28: SECOND_Z_AXIS is not actually used */
#define AXIS_IS_SECOND(ax) (((ax) >= SECOND_Y_AXIS) && ((ax) <= SECOND_X_AXIS))
#define AXIS_IS_FIRST(ax)  (((ax) >=  FIRST_Z_AXIS) && ((ax) <=  FIRST_X_AXIS))
#define AXIS_MAP_FROM_FIRST_TO_SECOND(ax) (SECOND_AXES + ((ax) - FIRST_AXES))
#define AXIS_MAP_FROM_SECOND_TO_FIRST(ax) (FIRST_AXES + ((ax) - SECOND_AXES))

#define SAMPLE_INTERVAL mtic_freq // sample axis doesn't need mtics, so use the slot to hold sample interval 
//
// What kind of ticmarking is wanted?
//
enum t_ticseries_type {
    TIC_COMPUTED=1, 		/* default; gnuplot figures them */
    TIC_SERIES,			/* user-defined series */
    TIC_USER,			/* user-defined points */
    TIC_MONTH,   		/* print out month names ((mo-1)%12)+1 */
    TIC_DAY      		/* print out day of week */
};

enum td_type {
    DT_NORMAL=0,		/* default; treat values as pure numeric */
    DT_TIMEDATE,		/* old datatype */
    DT_DMS,			/* degrees minutes seconds */
    DT_UNINITIALIZED
};
//
// Defines one ticmark for TIC_USER style.
// If label==NULL, the value is printed with the usual format string.
// else, it is used as the format string (note that it may be a constant string, like "high" or "low").
//
struct ticmark {
	static void DestroyList(ticmark * pList)
	{
		while(pList) {
			ticmark * p_freeable = pList;
			pList = pList->next;
			free(p_freeable->label);
			free(p_freeable);
		}
	}
	ticmark()
	{
		position = 0.0;
		label = 0;
		level = 0;
		next = 0;
	}
    double position; // where on axis is this
    char * label;    // optional (format) string label
    int    level;    // 0=major tic, 1=minor tic
    ticmark * next;  // linked list
};

/* Tic-mark labelling definition; see set xtics */
struct t_ticdef /*ticdef*/ {
	t_ticdef()
	{
		SetDefault();
	}
	void   SetDefault()
	{
		//#define DEFAULT_AXIS_TICDEF {TIC_COMPUTED, NULL, {TC_DEFAULT, 0, 0.0}, {NULL, {0.,0.,0.}, false},  { character, character, character, 0., 0., 0. }, false, true, false }
		type = TIC_COMPUTED;
		font = 0;
		textcolor.Set(TC_DEFAULT, 0, 0.0);
		def.user = 0;
		def.series.start = 0.0;
		def.series.incr = 0.0;
		def.series.end = 0.0;
		def.mix = false;
		offset.Set(character, character, character, 0.0, 0.0, 0.0);
		rangelimited = false;
		enhanced = true;
		logscaling = false;
	}
    t_ticseries_type type;
    char * font;
    t_colorspec textcolor;
    struct {
	   ticmark * user; // for TIC_USER
	   struct { // for TIC_SERIES
		  double start;
		  double incr;
		  double end;		/* ymax, if VERYLARGE */
	   } series;
	   bool mix;		/* true to use both the above */
    } def;
    t_position offset;
    bool rangelimited;		/* Limit tics to data range */
    bool enhanced;			/* Use enhanced text mode or labels */
    bool logscaling;		/* place tics suitably for logscaled axis */
};

/* we want two auto modes for minitics - default where minitics are
 * auto for log/time and off for linear, and auto where auto for all
 * graphs I've done them in this order so that logscale-mode can
 * simply test bit 0 to see if it must do the minitics automatically.
 * similarly, conventional plot can test bit 1 to see if minitics are
 * required */
enum t_minitics_status {
    MINI_OFF,
    MINI_DEFAULT,
    MINI_USER,
    MINI_AUTO
};
//
// Values to put in the axis_tics[] variables that decides where the
// ticmarks should be drawn: not at all, on one or both plot borders,
// or the zeroaxes. These look like a series of values, but TICS_MASK
// shows that they're actually bit masks --> don't turn into an enum
//
#define NO_TICS        0
#define TICS_ON_BORDER 1
#define TICS_ON_AXIS   2
#define TICS_MASK      3
#define TICS_MIRROR    4

#if 0 /* HBB 20010806 --- move GRID flags into axis struct */
/* Need to allow user to choose grid at first and/or second axes tics.
 * Also want to let user choose circles at x or y tics for polar grid.
 * Also want to allow user rectangular grid for polar plot or polar
 * grid for parametric plot. So just go for full configurability.
 * These are bitmasks
 */
#define GRID_OFF    0
#define GRID_X      (1<<0)
#define GRID_Y      (1<<1)
#define GRID_Z      (1<<2)
#define GRID_X2     (1<<3)
#define GRID_Y2     (1<<4)
#define GRID_MX     (1<<5)
#define GRID_MY     (1<<6)
#define GRID_MZ     (1<<7)
#define GRID_MX2    (1<<8)
#define GRID_MY2    (1<<9)
#define GRID_CB     (1<<10)
#define GRID_MCB    (1<<11)
#endif /* 0 */

#define RANGE_WRITEBACK   1
#define RANGE_SAMPLED     2
#define RANGE_IS_REVERSED 4

#define DEFAULT_AXIS_TICDEF {TIC_COMPUTED, NULL, {TC_DEFAULT, 0, 0.0}, {NULL, {0.,0.,0.}, false},  { character, character, character, 0., 0., 0. }, false, true, false }
#define DEFAULT_AXIS_ZEROAXIS {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}

//char * axis_name(/*AXIS_INDEX*/int);

struct GpAxis {
	static const lp_style_type DefaultAxisZeroAxis; // zeroaxis linetype (flag type==-3 if none wanted)

	GpAxis()
	{
		SetDefault();
	}
	//
	// Free dynamic fields in an axis structure so that it can be safely deleted
	// or reinitialized.  Doesn't free the axis structure itself.
	//
	//static void free_axis_struct(GpAxis * this_axis)
	void Destroy()
	{
		ZFREE(formatstring);
		ZFREE(ticfmt);
		if(link_udf) {
			ZFREE(link_udf->at);
			ZFREE(link_udf->definition);
			ZFREE(link_udf);
		}
		ticmark::DestroyList(ticdef.def.user);
		ticdef.def.user = 0;
		ZFREE(ticdef.font);
		label.UnsetAxisLabelOrTitle();
		if(zeroaxis != &DefaultAxisZeroAxis)
			ZFREE(zeroaxis);
	}
	void SetDefault()
	{
		AutoScale = AUTOSCALE_BOTH;
		SetAutoScale = AUTOSCALE_BOTH;
		range_flags = 0;
		min = -10.0;
		max = 10.0;
		set_min = -10.0;
		set_max = 10.0;
		writeback_min = -10.0;
		writeback_max = 10.0;
		data_min = 0.0;
		data_max = 0.0;

		min_constraint = CONSTRAINT_NONE;
		max_constraint = CONSTRAINT_NONE;
		min_lb = 0.0;
		min_ub = 0.0;
		max_lb = 0.0;
		max_ub = 0.0;

		term_lower = 0;
		term_upper = 0;
		term_scale = 0.0;
		term_zero = 0;

		log = false;
		base = 0.0;
		log_base = 0.0;

		linked_to_primary = 0;
		linked_to_secondary = 0;
		link_udf = 0;

		ticmode = NO_TICS;
		ticdef.SetDefault();
		tic_rotate = 0;
		gridmajor = false;
		gridminor = false;
		minitics = MINI_DEFAULT;
		mtic_freq = 10.0;
		ticscale = 1.0;
		miniticscale = 0.5;
		ticstep = 0.0;
		tic_in = true;

		datatype = DT_NORMAL;
		tictype = DT_NORMAL;
		formatstring = 0;
		ticfmt = 0;
		timelevel = TIMELEVEL_UNDEF;
		index = 0;
		label.SetEmpty();
		manual_justify = false;
		zeroaxis = 0;
	}
	void   Init(bool infinite)
	{
		AutoScale = SetAutoScale;
		min = (infinite && (SetAutoScale & AUTOSCALE_MIN)) ?  VERYLARGE : set_min;
		max = (infinite && (SetAutoScale & AUTOSCALE_MAX)) ? -VERYLARGE : set_max;
		data_min = VERYLARGE;
		data_max = -VERYLARGE;
	}
	bool ValidateValue(double v) const
	{
		// These are flag bits, not constants!!!
		if((AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_BOTH)
			return true;
		else if(((AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_MIN) && (v <= max))
			return true;
		else if(((AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_MAX) && (v >= min))
			return true;
		else if(((AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_NONE) && ((v <= max) && (v >= min)))
			return(true);
		else
			return(false);
	}
	bool   InRange(double val) const
	{
		return inrange(val, min, max) ? true : false;
	}
	//#define axis_map(axis, variable) (int)((axis)->term_lower + ((variable) - (axis)->min) * (axis)->term_scale + 0.5)
	int    Map(double value) const
	{
		return (int)(term_lower + (value - min) * term_scale + 0.5);
	}
	//#define axis_mapback(axis, pos) (((double)(pos) - axis->term_lower)/axis->term_scale + axis->min)
	double MapBack(int pos) const
	{
		return (((double)(pos) - term_lower)/term_scale + min);
	}
	double DoLog(double value) const
	{
		return (::log(value) / log_base);
	}
	double UndoLog(double value) const
	{
		return ::exp(value * log_base);
	}
	//#define axis_log_value(axis,value) (axis->log ? axis->DoLog(value) : (value))
	double LogValue(double value) const
	{
		return log ? DoLog(value) : value;
	}
	//#define axis_de_log_value(axis,GpCoordinate) (axis->log ? axis->UndoLog(GpCoordinate) : (GpCoordinate))
	double DeLogValue(double value) const
	{
		return log ? UndoLog(value) : value;
	}
	//
	// this is used in a few places all over the code: undo logscaling of
	// a given range if necessary. If checkrange is true, will int_error() if
	// range is invalid
	//
	//void axis_unlog_interval(GpAxis * axis, double * pMin, double * pMax, bool checkrange)
	void UnlogInterval(double * pMin, double * pMax, bool checkrange);
	//void axis_invert_if_requested(GpAxis * axis)
	void InvertIfRequested()
	{
		if((range_flags & RANGE_IS_REVERSED) && AutoScale && (max > min))
			Exchange(&min, &max);
	}
	void AdjustAutoscale(double value)
	{
		if(SetAutoScale & AUTOSCALE_MIN)
			SETMIN(min, value);
		if(SetAutoScale & AUTOSCALE_MAX)
			SETMAX(max, value);
	}
	int SetAutoscaleMin(double setMin)
	{
		if(AutoScale & AUTOSCALE_MIN) {
			min = setMin;
			return 1;
		}
		else
			return 0;
	}
	int SetAutoscaleMax(double setMax)
	{
		if(AutoScale & AUTOSCALE_MAX) {
			max = setMax;
			return 1;
		}
		else
			return 0;
	}
	double GetRange() const
	{
		return (max - min);
	}
	void   SetScaleAndRange(uint lo, uint up)
	{
		term_scale = (up - lo) / (max - min);
		term_lower = lo;
		term_upper = up;
	#ifdef NONLINEAR_AXES
		if(linked_to_primary && linked_to_primary->index <= 0) {
			GpAxis * p_link = linked_to_primary;
			p_link->term_scale = (up - lo) / (p_link->max - p_link->min);
			p_link->term_lower = lo;
			p_link->term_upper = up;
		}
	#endif
	}
	void   SetGrid(int _minor /* 0 - major, !0 - minor */, bool value)
	{
		if(_minor)
			gridminor = value;
		else
			gridmajor = value;
	}
	//
	// process 'unset mxtics' command
	//
	void   UnsetMiniTics()
	{
		minitics = MINI_OFF;
		mtic_freq = 10.0;
	}
	void   UnsetTics()
	{
		ticmode = NO_TICS;
		ZFREE(ticdef.font);
		ticdef.textcolor.type = TC_DEFAULT;
		ticdef.textcolor.lt = 0;
		ticdef.textcolor.value = 0;
		ticdef.offset.Set(character, character, character, 0., 0., 0.);
		ticdef.rangelimited = false;
		ticdef.enhanced = true;
		tic_rotate = 0;
		ticscale = 1.0;
		miniticscale = 0.5;
		tic_in = true;
		manual_justify = false;
		ticmark::DestroyList(ticdef.def.user);
		ticdef.def.user = NULL;
		if(index >= PARALLEL_AXES)
			ticdef.rangelimited = true;
	}
	double EvalLinkFunction(double raw_coord);
	//
	// range of this axis
	//
    t_autoscale AutoScale;     // Which end(s) are autoscaled?
    t_autoscale SetAutoScale;  // what does 'set' think AutoScale to be?
    int    range_flags;        // flag bits about AutoScale/writeback:
	//
    // write auto-ed ranges back to variables for AutoScale
	//
    double min;           // 'transient' axis extremal values
    double max;
    double set_min;       // set/show 'permanent' values
    double set_max;
    double writeback_min; // ULIG's writeback implementation
    double writeback_max;
    double data_min;      // Not necessarily the same as axis min
    double data_max;
	//
	// range constraints
	//
    t_constraint min_constraint;
    t_constraint max_constraint;
    double min_lb, min_ub;     // min lower- and upper-bound
    double max_lb, max_ub;     // min lower- and upper-bound
    //
	// output-related quantities
	//
    int    term_lower; // low and high end of the axis on output,
    int    term_upper; // ... (in terminal coordinates)
    double term_scale; // scale factor: plot --> term coords
    uint   term_zero;  // position of zero axis
	//
	// log axis control
	//
    bool   log;      // log axis stuff: flag "islog?"
    double base;     // logarithm base value
    double log_base; // ln(base), for easier computations
	//
	// linked axis information (used only by x2, y2)
	// If axes are linked, the primary axis info will be cloned into the
	// secondary axis only up to this point in the structure.
	//
    GpAxis * linked_to_primary;   //Set only in a secondary axis
    GpAxis * linked_to_secondary; // Set only in a primary axis
    UdftEntry * link_udf;
	//
	// ticmark control variables
	//
    int    ticmode;       // tics on border/axis? mirrored?
    t_ticdef ticdef;      // tic series definition
    int tic_rotate;       // ticmarks rotated by this angle
    t_minitics_status minitics; // minor tic mode (none/auto/user)?
    double mtic_freq;    // minitic stepsize
    double ticscale;     // scale factor for tic marks (was (0..1])
    double miniticscale; // and for minitics
    double ticstep;      // increment used to generate tic placement
    bool   tic_in;         // tics to be drawn inward?
    bool   gridmajor;      // Grid lines wanted on major tics?
    bool   gridminor;      // Grid lines for minor tics?
    bool   manual_justify; // override automatic justification
	//
	// time/date axis control
	//
    td_type datatype; // {DT_NORMAL|DT_TIMEDATE} controls _input_
    td_type tictype;  // {DT_NORMAL|DT_TIMEDATE|DT_DMS} controls _output_
    char * formatstring; // the format string for output
    char * ticfmt;       // autogenerated alternative to formatstring (needed??)
    t_timelevel timelevel; // minimum time unit used to quantize ticks
	//
	// other miscellaneous fields
	//
    int index; // if this is a permanent axis, this indexes GpAxB[]
		// (index >= PARALLEL_AXES) indexes parallel axes
		// (index < 0) indicates a dynamically allocated structure
    text_label label;           // label string and position offsets
    lp_style_type *zeroaxis;	// usually points to GpAxis::DefaultAxisZeroAxis
};

// Function pointer type for callback functions to generate ticmarks
typedef void (*tic_callback)(GpAxis *, double, char *, int, lp_style_type, ticmark *);

#define DEF_FORMAT       "% h"            // default format for tic mark labels
#define DEF_FORMAT_LATEX "$%h$"
#define TIMEFMT          "%d/%m/%y,%H:%M" // default parse timedata string
#if 0 // {
#define DEFAULT_AXIS_STRUCT {						    \
	AUTOSCALE_BOTH, AUTOSCALE_BOTH, /* auto, set_auto */		    \
	0, 			/* range_flags for autoscaling */	    \
	-10.0, 10.0,		/* 3 pairs of min/max for axis itself */    \
	-10.0, 10.0,							    \
	-10.0, 10.0,							    \
	  0.0,  0.0,		/* and another min/max for the data */	    \
	CONSTRAINT_NONE, CONSTRAINT_NONE,  /* min and max constraints */    \
	0., 0., 0., 0.,         /* lower and upper bound for min and max */ \
	0, 0,   		/* terminal lower and upper coords */	    \
	0.,        		/* terminal scale */			    \
	0,        		/* zero axis position */		    \
	false, 0.0, 0.0,	/* log, base, log(base) */		    \
	NULL, NULL,		/* linked_to_primary, linked_to_secondary */\
	NULL,      		/* link function */                         \
	NO_TICS,		/* tic output positions (border, mirror) */ \
	DEFAULT_AXIS_TICDEF,	/* tic series definition */		    \
	0, false, false, 	/* tic_rotate, grid{major,minor} */	    \
	MINI_DEFAULT, 10.,	/* minitics, mtic_freq */		    \
	1.0, 0.5, 0.0, true,	/* ticscale, miniticscale, ticstep, tic_in */ \
	DT_NORMAL, DT_NORMAL,	/* datatype for input, output */	    \
	NULL, NULL,      	/* output format, another output format */  \
	TIMELEVEL_UNDEF, /* timelevel */ \
	0,			/* index (e.g.FIRST_Y_AXIS) */		    \
	EMPTY_LABELSTRUCT,	/* axis label */			    \
	false,			/* override automatic justification */	    \
	NULL			/* NULL means &GpAxis::DefaultAxisZeroAxis */	    \
}
#endif // } 0
//
// This much of the axis structure is cloned by the "set x2range link" command
//
#define AXIS_CLONE_SIZE offsetof(GpAxis, linked_to_primary)
//
// Table of default behaviours --- a subset of the struct above. Only
// those fields are present that differ from axis to axis.
//
struct AXIS_DEFAULTS {
    double min;     // default axis endpoints
    double max;
    char   name[4]; // axis name, like in "x2" or "t"
    int    ticmode; // tics on border/axis? mirrored?
};

//
// Tic levels 0 and 1 are maintained in the axis structure.
// Tic levels 2 - MAX_TICLEVEL have only one property - scale.
//
#define MAX_TICLEVEL 5
//
// macros to reduce code clutter caused by the array notation, mainly in graphics.c and fit.c
//
#define X_AXIS  GpAxB[GpAxB.XAxis]
#define Y_AXIS  GpAxB[GpAxB.YAxis]
#define Z_AXIS  GpAxB[GpAxB.ZAxis]
#define R_AXIS  GpAxB[POLAR_AXIS]
#define CB_AXIS GpAxB[COLOR_AXIS]

//extern GpAxis GpAxB[AXIS_ARRAY_SIZE]; // @global

class GpAxisBlock {
public:
	GpAxisBlock();
	GpAxis & operator [] (size_t idx);
	const  char * GetAxisName(int axis);
	double GetTicScale(int ticLevel, const GpAxis * pAx) const;
	//#define IN_AXIS_RANGE(val, axis) inrange((val), GpAxB[axis].min, GpAxB[axis].max)
	bool   InAxisRange(double val, int axIdx) const;
	bool   InAxisRange2(double val1, int axIdx1, double val2, int axIdx2) const;
	bool   InAxisRange3(double val1, int axIdx1, double val2, int axIdx2, double val3, int axIdx3) const;
	//
	//#define AXIS_MAP(axis, variable) (int)((GpAxB[axis].term_lower) + ((variable) - GpAxB[axis].min) * GpAxB[axis].term_scale + 0.5)
	int    Map(AXIS_INDEX axis, double value) const
	{
		return AxA[axis].Map(value);
	}
	//#define AXIS_MAPBACK(axis, pos) (((double)(pos)-GpAxB[axis].term_lower)/GpAxB[axis].term_scale + GpAxB[axis].min)
	double MapBack(AXIS_INDEX axis, int pos) const
	{
		return AxA[axis].MapBack(pos);
	}
	// HBB 20000430: New macros, logarithmize a value into a stored GpCoordinate
	//#define AXIS_DO_LOG(axis,value) (log(value) / GpAxB[axis].log_base)
	double DoLog(AXIS_INDEX axIdx, double value) const
	{
		return AxA[axIdx].DoLog(value);
	}
	//#define AXIS_UNDO_LOG(axis,value) exp((value) * GpAxB[axis].log_base)
	double UndoLog(AXIS_INDEX axIdx, double value) const
	{
		return AxA[axIdx].UndoLog(value);
	}
	// HBB 20000430: same, but these test if the axis is log, first:
	//#define AXIS_LOG_VALUE(axis,value) (GpAxB[axis].log ? GpAxB.DoLog(axis,value) : (value))
	double LogValue(AXIS_INDEX axIdx, double value) const
	{
		const GpAxis & r_ax = AxA[axIdx];
		return r_ax.log ? r_ax.DoLog(value) : value;
	}
	//#define AXIS_DE_LOG_VALUE(axis,coord) (GpAxB[axis].log ? GpAxB.UndoLog(axis,coord):(coord))
	double DelogValue(AXIS_INDEX axIdx, double coord) const
	{
		const GpAxis & r_ax = AxA[axIdx];
		return r_ax.log ? r_ax.UndoLog(coord) : coord;
	}
	//
	// If we encounter a parallel axis index higher than any used so far,
	// extend parallel_axis[] to hold the corresponding data.
	// Returns pointer to the new axis.
	//
	//GpAxis * extend_parallel_axis(int paxis)
	GpAxis * ExtendParallelAxis(uint paxis);
	//void   axis_output_tics(AXIS_INDEX axis, int * ticlabel_position, AXIS_INDEX zeroaxis_basis, tic_callback callback)
	void   AxisOutputTics(AXIS_INDEX axis, int * pTiclabelPosition, AXIS_INDEX zeroAxisBasis, tic_callback callback);
	double LogValueChecked(AXIS_INDEX axis, double coord, const char * pWhat);
	//
	void * CreateAxesCopy()
	{
		GpAxis * p_copy = (GpAxis *)gp_alloc(sizeof(AxA), "AxArry copy");
		if(p_copy)
			memcpy(p_copy, AxA, sizeof(AxA));
		return p_copy;
	}
	void   RestoreAxesCopy(const void * pCopy)
	{
		if(pCopy)
			memcpy(AxA, pCopy, sizeof(AxA));
	}
	void   DestroyAxesCopy(void * pCopy)
	{
		free(pCopy);
	}
	bool ValidateData(double v, int axIdx) const
	{
		return AxA[axIdx].ValidateValue(v);
	}
	//
	//void axis_revert_range(AXIS_INDEX axis)
	void RevertRange(AXIS_INDEX axis)
	{
		AxA[axis].InvertIfRequested();
	}
	//void axis_revert_and_unlog_range(AXIS_INDEX axis)
	void RevertAndUnlogRange(AXIS_INDEX axis)
	{
		GpAxis & r_ax = AxA[axis];
		r_ax.InvertIfRequested();
		r_ax.UnlogInterval(&r_ax.min, &r_ax.max, true);
	}
	void   SetTicMode(int axIdx, int setFlag, int resetFlag)
	{
		if(axIdx < 0) {
			for(size_t i = 0; i < SIZEOFARRAY(AxA); i++) {
				GpAxis & r_ax = AxA[i];
				r_ax.ticmode |= setFlag;
				r_ax.ticmode &= ~resetFlag;
			}
		}
		else if(axIdx < SIZEOFARRAY(AxA)) {
			GpAxis & r_ax = AxA[axIdx];
			r_ax.ticmode |= setFlag;
			r_ax.ticmode &= ~resetFlag;
		}
	}
	//bool some_grid_selected()
	bool   SomeGridSelected() const
	{
		for(/*AXIS_INDEX*/int i = FIRST_AXES; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++)
			if(AxA[i].gridmajor || AxA[i].gridminor)
				return true;
		return false;
	}
	void   AxisCheckedExtendEmptyRange(AXIS_INDEX axis, const char * mesg);

	double TicScale[MAX_TICLEVEL];
	// FIXME HBB 20010806: had better be collected into a struct that's passed to the callback
	//extern int tic_start, tic_direction, tic_mirror;
	// These are for passing on to write_multiline():
	//extern int tic_text, rotate_tics, tic_hjust, tic_vjust;

	int    TicStart;
	int    TicDirection;
	int    TicMirror;
	int    TicText;
	int    RotateTics;
	int    TicHJust;
	int    TicVJust;
	//
	// axes being used by the current plot
	// These are mainly convenience variables, replacing separate copies of
	// such variables originally found in the 2D and 3D plotting code
	//
	AXIS_INDEX XAxis; //= FIRST_X_AXIS;
	AXIS_INDEX YAxis; //= FIRST_Y_AXIS;
	AXIS_INDEX ZAxis; // = FIRST_Z_AXIS;

	uint   NumParallelAxes;
	GpAxis * P_ParallelAxis;
private:
	//
	// Fill in the starting values for a just-allocated  parallel axis structure
	//
	//void init_parallel_axis(GpAxis * this_axis, AXIS_INDEX index)
	void InitParallelAxis(GpAxis * pAx, AXIS_INDEX index);

	GpAxis AxA[AXIS_ARRAY_SIZE];
	SString AxNameBuf;
};

// macro for tic scale, used in all tic_callback functions
//#define tic_scale(ticlevel, axis) (ticlevel <= 0 ? axis->ticscale : ticlevel == 1 ? axis->miniticscale : ticlevel < MAX_TICLEVEL ? ticscale[ticlevel] : 0)

extern GpAxisBlock GpAxB; // @global
//
// global variables in axis.c
//
extern const  AXIS_DEFAULTS axis_defaults[AXIS_ARRAY_SIZE];
extern GpAxis * shadow_axis_array;
// EAM DEBUG - Dynamic allocation of parallel axes.
extern const  GenTable axisname_tbl[]; // A parsing table for mapping axis names into axis indices. For use by the set/show machinery, mainly
extern const  t_ticdef default_axis_ticdef;
extern const  text_label default_axis_label; // axis labels
extern const  lp_style_type default_grid_lp; // default grid linetype, to be used by 'unset grid' and 'reset'
extern char * P_TimeFormat;
extern int    grid_layer; // grid layer: LAYER_BEHIND LAYER_BACK LAYER_FRONT
extern bool   grid_tics_in_front; // Whether to draw the axis tic labels and tic marks in front of everything else
extern bool   raxis; // Whether or not to draw a separate polar axis in polar mode
//
// global variables for communication with the tic callback functions
//

// The remaining ones are for grid drawing; controlled by 'set grid':
// extern int grid_selection; --- comm'ed out, HBB 20010806
extern lp_style_type grid_lp; // linestyle for major grid lines
extern lp_style_type mgrid_lp; // linestyle for minor grid lines
extern double polar_grid_angle; // angle step in polar grid in radians
extern int    widest_tic_strlen; // Length of the longest tics label, set by widest_tic_callback():
extern bool   inside_zoom; // flag to indicate that in-line axis ranges should be ignored
// extern AXIS_INDEX x_axis, y_axis, z_axis; // axes being used by the current plot

/* -------- macros using these variables: */

/* Macros to map from user to terminal coordinates and back */
//#define AXIS_MAP(axis, variable) (int)((GpAxB[axis].term_lower) + ((variable) - GpAxB[axis].min) * GpAxB[axis].term_scale + 0.5)
//#define AXIS_MAPBACK(axis, pos) (((double)(pos)-GpAxB[axis].term_lower)/GpAxB[axis].term_scale + GpAxB[axis].min)

/* Same thing except that "axis" is a pointer, not an index */
//#define axis_map(axis, variable) (int)((axis)->term_lower + ((variable) - (axis)->min) * (axis)->term_scale + 0.5)
//#define axis_mapback(axis, pos) (((double)(pos) - axis->term_lower)/axis->term_scale + axis->min)

//#define axis_do_log(axis,value) (log(value) / axis->log_base)
//#define axis_undo_log(axis,value) exp((value) * axis->log_base)

//
// April 2015:  I'm not 100% sure, but I believe there is no longer
// any need to treat 2D and 3D axis initialization differently
//
#define AXIS_INIT3D(axis, islog_override, infinite) GpAxB[axis].Init(infinite)
#define AXIS_INIT2D(axis, infinite)                 GpAxB[axis].Init(infinite)

/* AXIS_INIT2D_REFRESH and AXIS_UPDATE2D_REFRESH(axis) are for volatile data */
#define AXIS_INIT2D_REFRESH(axis, infinite)				\
do {									\
    GpAxis *p_this = &GpAxB[axis];					\
    p_this->AutoScale = p_this->SetAutoScale; \
    p_this->min = (infinite && (p_this->SetAutoScale & AUTOSCALE_MIN)) ? VERYLARGE : p_this->LogValue(p_this->set_min);  \
    p_this->max = (infinite && (p_this->SetAutoScale & AUTOSCALE_MAX)) ? -VERYLARGE : p_this->LogValue(p_this->set_max); \
    p_this->log_base = p_this->log ? log(p_this->base) : 0;			\
} while(0)

#define AXIS_UPDATE2D_REFRESH(axis)					\
do {									\
    GpAxis * p_axis_ = &GpAxB[axis];				\
    if((p_axis_->SetAutoScale & AUTOSCALE_MIN) == 0)		\
		p_axis_->min = p_axis_->LogValue(p_axis_->set_min);	\
    if((p_axis_->SetAutoScale & AUTOSCALE_MAX) == 0)		\
		p_axis_->max = p_axis_->LogValue(p_axis_->set_max);	\
} while (0)

/* parse a position of the form
 *    [coords] x, [coords] y {,[coords] z}
 * where coords is one of first,second.graph,screen,character
 * if first or second, we need to take axis.datatype into account
 * FIXME: Cannot handle parallel axes
 */
#define GET_NUMBER_OR_TIME(store,axes,axis)				\
do { \
    GpAxis *this_axis = (axes == NO_AXIS) ? NULL : &(GpAxB[(axes)+(axis)]); \
    (store) = get_num_or_time(this_axis);				\
} while(0)

/* store VALUE or log(VALUE) in STORE, set TYPE as appropriate
 * Do OUT_ACTION or UNDEF_ACTION as appropriate
 * adjust range provided type is INRANGE (ie dont adjust y if x is outrange
 * VALUE must not be same as STORE
 * NOAUTOSCALE is per-plot property, whereas AUTOSCALE_XXX is per-axis.
 * Note: see the particular implementation for COLOR GpAxis below.
 */

#define ACTUAL_STORE_WITH_LOG_AND_UPDATE_RANGE(STORE, VALUE, TYPE, ax, NOAUTOSCALE, OUT_ACTION, UNDEF_ACTION, is_cb_axis)  \
do {									  \
    GpAxis *axis = ax; \
    double curval = (VALUE);						  \
    /* Version 5: OK to store infinities or NaN */			  \
    STORE = curval;							  \
    if(!(curval > -VERYLARGE && curval < VERYLARGE)) {		  \
	TYPE = UNDEFINED;						  \
	UNDEF_ACTION;							  \
	break;								  \
    }									  \
    if(axis->log) {							  \
	if(curval < 0.0) {						  \
	    STORE = not_a_number();					  \
	    TYPE = UNDEFINED;						  \
	    UNDEF_ACTION;						  \
	    break;							  \
	} else if(curval == 0.0) {					  \
	    STORE = -VERYLARGE;						  \
	    TYPE = OUTRANGE;						  \
	    OUT_ACTION;							  \
	    break;							  \
	} else {							  \
	    STORE = log(curval) / axis->log_base; /* AXIS_DO_LOG() */	  \
	}								  \
    }									  \
    if(NOAUTOSCALE)							  \
	break;  /* this plot is not being used for autoscaling */	  \
    if(TYPE != INRANGE)						  \
	break;  /* don't set y range if x is outrange, for example */	  \
    if((!is_cb_axis) && axis->linked_to_primary) {	  		  \
	axis = axis->linked_to_primary;					  \
	if(axis->link_udf->at) 					  \
	    curval = axis->EvalLinkFunction(curval);			  \
    } 									  \
	SETMIN(axis->data_min, curval); \
    if((curval < axis->min) && ((curval <= axis->max) || (axis->max == -VERYLARGE))) { \
	if(axis->AutoScale & AUTOSCALE_MIN)	{ \
	    if(axis->min_constraint & CONSTRAINT_LOWER) {		  \
		if(axis->min_lb <= curval) {				  \
		    axis->min = curval;					  \
		} else {						  \
		    axis->min = axis->min_lb;				  \
		    TYPE = OUTRANGE;					  \
		    OUT_ACTION;						  \
		    break;						  \
		}							  \
	    } else {							  \
		axis->min = curval;					  \
	    }								  \
	} else if(curval != axis->max) {				  \
	    TYPE = OUTRANGE;						  \
	    OUT_ACTION;							  \
	    break;							  \
	}								  \
    }									  \
	SETMAX(axis->data_max, curval); \
    if(curval > axis->max && (curval >= axis->min || axis->min == VERYLARGE)) { \
	if(axis->AutoScale & AUTOSCALE_MAX)	{			  \
	    if(axis->max_constraint & CONSTRAINT_UPPER) {		  \
		if(axis->max_ub >= curval) {		 		  \
		    axis->max = curval;					  \
		} else {						  \
		    axis->max = axis->max_ub;				  \
		    TYPE =OUTRANGE;					  \
		    OUT_ACTION;						  \
		    break;						  \
		}							  \
	    } else {							  \
		axis->max = curval;					  \
	    }								  \
	} else if(curval != axis->min) {				  \
	    TYPE = OUTRANGE;						  \
	    OUT_ACTION;							  \
	}								  \
    }									  \
} while(0)

/* normal calls go though this macro, marked as not being a color axis */
#define STORE_WITH_LOG_AND_UPDATE_RANGE(STORE, VALUE, TYPE, ax, NOAUTOSCALE, OUT_ACTION, UNDEF_ACTION)	 \
	if(ax != NO_AXIS) ACTUAL_STORE_WITH_LOG_AND_UPDATE_RANGE(STORE, VALUE, TYPE, (&GpAxB[ax]), NOAUTOSCALE, OUT_ACTION, UNDEF_ACTION, 0)

/* Implementation of the above for the color axis. It should not change
 * the type of the point (out-of-range color is plotted with the color
 * of the min or max color value).
 */
#define COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(STORE, VALUE, TYPE, ax, NOAUTOSCALE, OUT_ACTION, UNDEF_ACTION) \
{									  \
    coord_type c_type_tmp = TYPE;					  \
    ACTUAL_STORE_WITH_LOG_AND_UPDATE_RANGE(STORE, VALUE, c_type_tmp, &GpAxB[ax], NOAUTOSCALE, OUT_ACTION, UNDEF_ACTION, 1); \
}

/* #define NOOP (0) caused many warnings from gcc 3.2 */
#define NOOP ((void)0)

/* HBB 20000506: new macro to automatically build initializer lists
 * for arrays of AXIS_ARRAY_SIZE=11 equal elements */
#define AXIS_ARRAY_INITIALIZER(value) { value, value, value, value, value, value, value, value, value, value, value }

/* 'roundoff' check tolerance: less than one hundredth of a tic mark */
#define SIGNIF (0.01)
/* (DFK) Watch for cancellation error near zero on axes labels */
/* FIXME HBB 20000521: these seem not to be used much, anywhere... */
#define CheckZero(x,tic) (fabs(x) < ((tic) * SIGNIF) ? 0.0 : (x))
//
// functions exported by axis.c
//
t_autoscale load_range(GpAxis *, double *, double *, t_autoscale);
//void axis_unlog_interval(GpAxis *, double *, double *, bool);
//void axis_invert_if_requested(GpAxis *);
//void axis_revert_range(AXIS_INDEX);
//void axis_revert_and_unlog_range(AXIS_INDEX);
//void axis_init(GpAxis *this_axis, bool infinite);
//double axis_log_value_checked(AXIS_INDEX, double, const char *);
//void axis_checked_extend_empty_range(AXIS_INDEX, const char *mesg);
char * copy_or_invent_formatstring(GpAxis *);
double quantize_normal_tics(double, int);
void setup_tics(GpAxis *, int);
void gen_tics(GpAxis *, tic_callback);
//void axis_output_tics(AXIS_INDEX, int *, AXIS_INDEX, tic_callback);
void axis_draw_2d_zeroaxis(AXIS_INDEX, AXIS_INDEX);
//bool some_grid_selected();
void add_tic_user(GpAxis *, char *, double, int);
double get_num_or_time(GpAxis *);
void save_writeback_all_axes();
int  parse_range(AXIS_INDEX axis);
void parse_skip_range();
void check_axis_reversed(AXIS_INDEX axis);
/* set widest_tic_label: length of the longest tics label */
void widest_tic_callback(GpAxis *, double place, char *text, int ticlevel, lp_style_type grid, ticmark *);
void get_position(t_position *pos);
void get_position_default(t_position *pos, enum position_type default_type, int ndim);
void gstrdms(char *label, char *format, double value);
void clone_linked_axes(GpAxis *axis1, GpAxis *axis2);
GpAxis *get_shadow_axis(GpAxis *axis);
int map_x(double value);
int map_y(double value);
void set_cbminmax();
void save_autoscaled_ranges(const GpAxis *, const GpAxis *);
void restore_autoscaled_ranges(GpAxis *, GpAxis *);
void init_sample_range(GpAxis *axis);
//void init_parallel_axis(GpAxis *, AXIS_INDEX);
//GpAxis * extend_parallel_axis(int);
// Evaluate the function linking a secondary axis to its primary axis
//double eval_link_function(GpAxis *, double);

#endif /* GNUPLOT_AXIS_H */
