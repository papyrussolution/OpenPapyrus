// GNUPLOT - axis.c 
// Copyright 2000, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop

/* HBB 20000725: gather all per-axis variables into a struct, and set
 * up a single large array of such structs. Next step might be to use
 * isolated AXIS structs, instead of an array.
 * EAM 2013: tried that.  The problem is that all the routines and macros
 * that manipulate axis data take an index, not a pointer.  We'd have to
 * rewrite all of them and it just didn't seem worth it.
 * Instead I've added additional non-standard entries on the end, used for
 * parallel axis plots if nothing else.
 * Note: This array is now initialized in reset_command().
 */
//GpAxis axis_array_Removed[AXIS_ARRAY_SIZE];
//GpAxis * shadow_axis_array_Removed; // Only if nonlinear axes are in use 

/*static*/void GpAxis::UnsetLabelOrTitle(text_label * pLabel)
{
	if(pLabel) {
		SAlloc::F(pLabel->text);
		SAlloc::F(pLabel->font);
		*pLabel = default_axis_label;
	}
}
// 
// Simplest form of autoscaling (no check on autoscale constraints).
// Used by refresh_bounds() and refresh_3dbounds().
// Used also by autoscale_boxplot.
// FIXME:  Reversed axes are skipped because not skipping them causes errors
//   if apply_zoom->refresh_request->refresh_bounds->autoscale_one_point.
//   But really autoscaling shouldn't be done at all in that case.
// 
void FASTCALL GpAxis::AutoscaleOnePoint(double x)
{
	if(!(range_flags & RANGE_IS_REVERSED)) {
		if(set_autoscale & AUTOSCALE_MIN && x < min)
			min = x;
		if(set_autoscale & AUTOSCALE_MAX && x > max)
			max = x;
	}
}
// 
// Free dynamic fields in an axis structure so that it can be safely deleted
// or reinitialized.  Doesn't free the axis structure itself.
// SAMPLE_AXIS is an exception because its link pointers are only copies of
// those in the real axis being sampled.
// 
void GpAxis::Destroy()
{
	ZFREE(formatstring);
	ZFREE(ticfmt);
	if(link_udf && index != SAMPLE_AXIS) {
		ZFREE(link_udf->at);
		ZFREE(link_udf->definition);
		ZFREE(link_udf);
	}
	free_marklist(ticdef.def.user);
	ZFREE(ticdef.font);
	UnsetLabelOrTitle(&label);
	if(zeroaxis != &default_axis_zeroaxis)
		ZFREE(zeroaxis);
}
//
// Keep defaults varying by axis in their own array, to ease initialization of the main array 
//
const AXIS_DEFAULTS axis_defaults[AXIS_ARRAY_SIZE] = {
	{ -10, 10, "z", TICS_ON_BORDER,               },
	{ -10, 10, "y", TICS_ON_BORDER | TICS_MIRROR, },
	{ -10, 10, "x", TICS_ON_BORDER | TICS_MIRROR, },
	{ -10, 10, "cb", TICS_ON_BORDER | TICS_MIRROR, },
	{ -10, 10, " z2", NO_TICS,                     },
	{ -10, 10, "y2", NO_TICS,                      },
	{ -10, 10, "x2", NO_TICS,                      },
	{ -0, 10, "r", TICS_ON_AXIS,                 },
	{ -5,  5, "t", NO_TICS,                      },
	{ -5,  5, "u", NO_TICS,                      },
	{ -5,  5, "v", NO_TICS,                      }
};

const GpAxis default_axis_state;// = DEFAULT_AXIS_STRUCT;

// Parallel axis structures are held in an array that is dynamically allocated on demand.
//GpAxis * parallel_axis_array_Removed = NULL;
//int num_parallel_axes_Removed = 0;
// 
// Separate axis THETA for tics around perimeter of polar grid
// Initialize to blank rather than DEFAULT_AXIS_STRUCT because
// it is too different from "real" axes for the defaults to make sense.
// The fields we do need are initialized in unset_polar().
// 
//GpAxis THETA_AXIS_Removed; // = {0};
// 
// HBB 20000506 new variable: parsing table for use with the table
// module, to help generalizing set/show/unset/save, where possible
// 
const struct gen_table axisname_tbl[] = {
	{ "z", FIRST_Z_AXIS},
	{ "y", FIRST_Y_AXIS},
	{ "x", FIRST_X_AXIS},
	{ "cb", COLOR_AXIS},
	{ " z2", SECOND_Z_AXIS},
	{ "y2", SECOND_Y_AXIS},
	{ "x2", SECOND_X_AXIS},
	{ "r", POLAR_AXIS},
	{ "t", T_AXIS},
	{ "u", U_AXIS},
	{ "v", V_AXIS},
	{ NULL, -1}
};

// penalty for doing tics by callback in gen_tics is need for global
// variables to communicate with the tic routines. Dont need to be arrays for this 
// HBB 20000416: they may not need to be array[]ed, but it'd sure
// make coding easier, in some places... 
// HBB 20000416: for the testing, these are global... 
int tic_start;
int tic_direction;
int tic_text;
int rotate_tics;
/*int*/JUSTIFY tic_hjust;
/*int*/VERT_JUSTIFY tic_vjust;
int tic_mirror;

// These are declare volatile in order to fool the compiler into not 
// optimizing out intermediate values, thus hiding loss of precision.
volatile double vol_this_tic;
volatile double vol_previous_tic;
const t_ticdef default_axis_ticdef; // = DEFAULT_AXIS_TICDEF;
double ticscale[MAX_TICLEVEL] = {1, 0.5, 1, 1, 1}; /* Tic scale for tics with level > 1.  0 means 'inherit minitics scale'  */
char * P_TimeFormat = NULL; /* global default time format */
const text_label default_axis_label;// = EMPTY_LABELSTRUCT; // axis labels 
const lp_style_type default_axis_zeroaxis(lp_style_type::defZeroAxis); // = DEFAULT_AXIS_ZEROAXIS; // zeroaxis drawing 
// grid drawing 
// int grid_selection = GRID_OFF; 
#define DEFAULT_GRID_LP {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 0, 0.5, 0.0, DEFAULT_P_CHAR, {TC_LT, LT_AXIS, 0.0}, DEFAULT_DASHPATTERN}
const lp_style_type default_grid_lp(lp_style_type::defGrid); // = DEFAULT_GRID_LP;
lp_style_type grid_lp(lp_style_type::defGrid);               // = DEFAULT_GRID_LP;
lp_style_type mgrid_lp(lp_style_type::defGrid);              // = DEFAULT_GRID_LP;
int    grid_layer = LAYER_BEHIND;
bool   grid_tics_in_front = FALSE;
bool   grid_vertical_lines = FALSE;
bool   grid_spiderweb = FALSE;
double polar_grid_angle = 0.0; // nonzero means a polar grid 
bool   raxis = FALSE;
double theta_origin = 0.0;  // default origin at right side
double theta_direction = 1; // counterclockwise from origin 
int    widest_tic_strlen; // Length of the longest tics label, set by widest_tic_callback()
bool   inside_zoom; // flag to indicate that in-line axis ranges should be ignored 

// axes being used by the current plot 
// These are mainly convenience variables, replacing separate copies of
// such variables originally found in the 2D and 3D plotting code 
//AXIS_INDEX x_axis = FIRST_X_AXIS;
//AXIS_INDEX y_axis = FIRST_Y_AXIS;
//AXIS_INDEX z_axis = FIRST_Z_AXIS;

// Only accessed by save_autoscaled_ranges() and restore_autoscaled_ranges() 
//static RealRange SaveAutoscaledRangeX;
//static RealRange SaveAutoscaledRangeY;

/* --------- internal prototypes ------------------------- */
static double make_auto_time_minitics(t_timelevel, double);
static double make_tics(GpAxis *, int);
static double quantize_time_tics(GpAxis *, double, double, int);
static double time_tic_just(t_timelevel, double);
static double round_outward(GpAxis *, bool, double);
static bool axis_position_zeroaxis(AXIS_INDEX);
static double quantize_duodecimal_tics(double, int);

/* ---------------------- routines ----------------------- */

void check_log_limits(const GpAxis * pAx, double min, double max)
{
	if(pAx->log) {
		if(min<= 0.0 || max <= 0.0)
			GPO.IntError(NO_CARET, "%s range must be greater than 0 for log scale", axis_name((AXIS_INDEX)pAx->index));
	}
}

/* {{{ axis_invert_if_requested() */

void axis_invert_if_requested(GpAxis * pAx)
{
	if(((pAx->range_flags & RANGE_IS_REVERSED)) && (pAx->autoscale != 0))
		reorder_if_necessary(pAx->max, pAx->min); // NB: The whole point of this is that we want max < min !!! 
}

//void FASTCALL axis_init(GpAxis * pAx, bool resetAutoscale)
void GpAxis::Init(bool resetAutoscale)
{
	autoscale = set_autoscale;
	min = (resetAutoscale && (set_autoscale & AUTOSCALE_MIN)) ?  VERYLARGE : set_min;
	max = (resetAutoscale && (set_autoscale & AUTOSCALE_MAX)) ? -VERYLARGE : set_max;
	data_min =  VERYLARGE;
	data_max = -VERYLARGE;
}

//void FASTCALL axis_check_range(AXIS_INDEX axis)
void  FASTCALL GpAxisSet::CheckRange(AXIS_INDEX idx)
{
	axis_invert_if_requested(&AxArray[idx]);
	check_log_limits(&AxArray[idx], AxArray[idx].min, AxArray[idx].max);
}

/* {{{ axis_log_value_checked() */
//double axis_log_value_checked(AXIS_INDEX axis, double coord, const char * what)
double GnuPlot::AxisLogValueChecked(AXIS_INDEX axis, double coord, const char * pWhat)
{
	if(AxS[axis].log && !(coord > 0.0))
		IntError(NO_CARET, "%s has %s coord of %g; must be above 0 for log scale!", pWhat, axis_name(axis), coord);
	return coord;
}

/* }}} */

char * FASTCALL axis_name(AXIS_INDEX axis)
{
	static char name[] = "primary 00 ";
	if(axis == THETA_index)
		return "t";
	if(axis >= PARALLEL_AXES) {
		sprintf(name, "paxis %d ", (axis-PARALLEL_AXES+1) & 0xff);
		return name;
	}
	if(axis < 0) {
		sprintf(name, "primary %2s", axis_defaults[-axis].name);
		return name;
	}
	return (char *)axis_defaults[axis].name;
}

//void init_sample_range(const GpAxis * axis, enum PLOT_TYPE plot_type)
void FASTCALL GpAxisSet::InitSampleRange(const GpAxis * pAxis, enum PLOT_TYPE plot_type)
{
	AxArray[SAMPLE_AXIS].range_flags = 0;
	AxArray[SAMPLE_AXIS].min = pAxis->min;
	AxArray[SAMPLE_AXIS].max = pAxis->max;
	AxArray[SAMPLE_AXIS].set_min = pAxis->set_min;
	AxArray[SAMPLE_AXIS].set_max = pAxis->set_max;
	AxArray[SAMPLE_AXIS].datatype = pAxis->datatype;
	// Functions are sampled along the x or y plot axis 
	// Data is drawn from pseudofile '+', assumed to be linear 
	// NB:  link_udf MUST NEVER BE FREED as it is only a copy
	if(plot_type == FUNC) {
		AxArray[SAMPLE_AXIS].linked_to_primary = pAxis->linked_to_primary;
		AxArray[SAMPLE_AXIS].link_udf = pAxis->link_udf;
	}
}
//
// Fill in the starting values for a just-allocated  parallel axis structure
//
void init_parallel_axis(GpAxis * pAxis, AXIS_INDEX index)
{
	memcpy(pAxis, &default_axis_state, sizeof(GpAxis));
	pAxis->formatstring = sstrdup(DEF_FORMAT);
	pAxis->index = index + PARALLEL_AXES;
	pAxis->ticdef.rangelimited = TRUE;
	pAxis->set_autoscale |= AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX;
	pAxis->Init(true);
}
// 
// If we encounter a parallel axis index higher than any used so far,
// extend parallel_axis[] to hold the corresponding data.
// Returns pointer to the new axis.
// 
//void extend_parallel_axis(int paxis)
void GpAxisSet::ExtendParallelAxis(int paxis)
{
	if(paxis > NumParallelAxes) {
		P_ParallelAxArray = (GpAxis *)SAlloc::R(P_ParallelAxArray, paxis * sizeof(GpAxis));
		for(int i = NumParallelAxes; i < paxis; i++)
			init_parallel_axis(&P_ParallelAxArray[i], (AXIS_INDEX)i);
		NumParallelAxes = paxis;
	}
}
// 
// Most of the crashes found during fuzz-testing of version 5.1 were a
// consequence of an axis range being corrupted, i.e. NaN or Inf.
// Corruption became easier with the introduction of nonlinear axes,
// but even apart from that autoscaling bad data could cause a fault.
// NB: Some platforms may need help with isnan() and isinf().
// 
//bool FASTCALL bad_axis_range(const GpAxis * axis)
bool GpAxis::BadRange() const
{
	if(isnan(this->min) || isnan(this->max))
		return true;
#ifdef isinf
	if(isinf(axis->min) || isinf(axis->max))
		return TRUE;
#endif
	return (this->max == -VERYLARGE || this->min == VERYLARGE) ? true : false;
}

//static void flip_projection_axis(GpAxis * pAx)
void GpAxis::FlipProjection()
{
	Exchange(&min, &max);
	if(linked_to_primary) {
		GpAxis * p_ax = linked_to_primary;
		Exchange(&p_ax->min, &p_ax->max);
	}
}

/* {{{ axis_checked_extend_empty_range() */
/*
 * === SYNOPSIS ===
 *
 * This function checks whether the data and/or plot range in a given axis
 * is too small (which would cause divide-by-zero and/or infinite-loop
 * problems later on).  If so,
 * - if autoscaling is in effect for this axis, we widen the range
 * - otherwise, we abort with a call to  GPO.IntError()  (which prints out
 *   a suitable error message, then (hopefully) aborts this command and
 *   returns to the command prompt or whatever).
 *
 *
 * === HISTORY AND DESIGN NOTES ===
 *
 * 1998 Oct 4, Jonathan Thornburg <jthorn@galileo.thp.univie.ac.at>
 *
 * This function used to be a (long) macro  FIXUP_RANGE(AXIS, WHICH)
 * which was (identically!) defined in  plot2d.c  and  plot3d.c .  As
 * well as now being a function instead of a macro, the logic is also
 * changed:  The "too small" range test no longer depends on 'set zero'
 * and is now properly scaled relative to the data magnitude.
 *
 * The key question in designing this function is the policy for just how
 * much to widen the data range by, as a function of the data magnitude.
 * This is to some extent a matter of taste.  IMHO the key criterion is
 * that (at least) all of the following should (a) not infinite-loop, and
 * (b) give correct plots, regardless of the 'set zero' setting:
 *      plot 6.02e23            # a huge number >> 1 / FP roundoff level
 *      plot 3                  # a "reasonable-sized" number
 *      plot 1.23e-12           # a small number still > FP roundoff level
 *      plot 1.23e-12 * sin(x)  # a small function still > FP roundoff level
 *      plot 1.23e-45           # a tiny number << FP roundoff level
 *      plot 1.23e-45 * sin(x)  # a tiny function << FP roundoff level
 *      plot 0          # or (more commonly) a data file of all zeros
 * That is, IMHO gnuplot should *never* infinite-loop, and it should *never*
 * producing an incorrect or misleading plot.  In contrast, the old code
 * would infinite-loop on most of these examples with 'set zero 0.0' in
 * effect, or would plot the small-amplitude sine waves as the zero function
 * with 'zero' set larger than the sine waves' amplitude.
 *
 * The current code plots all the above examples correctly and without
 * infinite looping.
 *
 * HBB 2000/05/01: added an additional up-front test, active only if
 *   the new 'mesg' parameter is non-NULL.
 *
 * === USAGE ===
 *
 * Arguments:
 * axis = (in) An integer specifying which axis (x1, x2, y1, y2, z, etc)
 *             we should do our stuff for.  We use this argument as an
 *             index into the global arrays  {min,max,auto}_array .  In
 *             practice this argument will typically be one of the constants
 *              {FIRST,SECOND}_{X,Y,Z}_AXIS  defined in plot.h.
 * mesg = (in) if non-NULL, will check if the axis range is valid (min
 *             not +VERYLARGE, max not -VERYLARGE), and GPO.IntError() out
 *             if it isn't.
 *
 * Global Variables:
 * auto_array, min_array, max_array (in out) (defined in axis.[ch]):
 *    variables describing the status of autoscaling and range ends, for
 *    each of the possible axes.
 *
 * c_token = (in) (defined in plot.h) Used in formatting an error message.
 *
 */
//void FASTCALL axis_checked_extend_empty_range(AXIS_INDEX axis, const char * mesg)
void GnuPlot::AxisCheckedExtendEmptyRange(AXIS_INDEX axis, const char * mesg)
{
	GpAxis * p_this_axis = &AxS[axis];
	// These two macro definitions set the range-widening policy: 
	// widen [0:0] by +/- this absolute amount 
#define FIXUP_RANGE__WIDEN_ZERO_ABS     1.0
	// widen [nonzero:nonzero] by -/+ this relative amount 
#define FIXUP_RANGE__WIDEN_NONZERO_REL  0.01
	double dmin = p_this_axis->min;
	double dmax = p_this_axis->max;
	// pass msg == NULL if for some reason you trust the axis range 
	if(mesg && p_this_axis->BadRange())
		IntErrorCurToken(mesg);
	if(dmax - dmin == 0.0) {
		// empty range 
		if(p_this_axis->autoscale) {
			// range came from autoscaling ==> widen it 
			double widen = (dmax == 0.0) ? FIXUP_RANGE__WIDEN_ZERO_ABS : FIXUP_RANGE__WIDEN_NONZERO_REL * fabs(dmax);
			if(!(axis == FIRST_Z_AXIS && !mesg)) /* set view map */
				fprintf(stderr, "Warning: empty %s range [%g:%g], ", axis_name(axis), dmin, dmax);
			// HBB 20010525: correctly handle single-ended autoscaling, too: 
			if(p_this_axis->autoscale & AUTOSCALE_MIN)
				p_this_axis->min -= widen;
			if(p_this_axis->autoscale & AUTOSCALE_MAX)
				p_this_axis->max += widen;
			if(!(axis == FIRST_Z_AXIS && !mesg)) /* set view map */
				fprintf(stderr, "adjusting to [%g:%g]\n", p_this_axis->min, p_this_axis->max);
		}
		else {
			// user has explicitly set the range (to something empty) 
			IntError(NO_CARET, "Can't plot with an empty %s range!", axis_name(axis));
		}
	}
}
//
// Simpler alternative routine for nonlinear axes (including log scale) 
//
//void FASTCALL axis_check_empty_nonlinear(const GpAxis * pAx)
void FASTCALL GnuPlot::AxisCheckEmptyNonLinear(const GpAxis * pAx)
{
	// Poorly defined via/inv nonlinear mappings can leave NaN in derived range 
	if(pAx->BadRange())
		goto undefined_axis_range_error;
	pAx = pAx->linked_to_primary;
	if(pAx->BadRange())
		goto undefined_axis_range_error;
	return;
undefined_axis_range_error:
	IntError(NO_CARET, "empty or undefined %s axis range", axis_name((AXIS_INDEX)pAx->index));
}

/* }}} */

/* {{{ make smalltics for time-axis */
static double make_auto_time_minitics(t_timelevel tlevel, double incr)
{
	double tinc = 0.0;
	if((int)tlevel < TIMELEVEL_SECONDS)
		tlevel = TIMELEVEL_SECONDS;
	switch(tlevel) {
		case TIMELEVEL_SECONDS:
		case TIMELEVEL_MINUTES:
		    if(incr >= 5)
			    tinc = 1;
		    if(incr >= 10)
			    tinc = 5;
		    if(incr >= 20)
			    tinc = 10;
		    if(incr >= 60)
			    tinc = 20;
		    if(incr >= 2 * 60)
			    tinc = 60;
		    if(incr >= 6 * 60)
			    tinc = 2 * 60;
		    if(incr >= 12 * 60)
			    tinc = 3 * 60;
		    if(incr >= 24 * 60)
			    tinc = 6 * 60;
		    break;
		case TIMELEVEL_HOURS:
		    if(incr >= 20 * 60)
			    tinc = 10 * 60;
		    if(incr >= 3600)
			    tinc = 30 * 60;
		    if(incr >= 2 * 3600)
			    tinc = 3600;
		    if(incr >= 6 * 3600)
			    tinc = 2 * 3600;
		    if(incr >= 12 * 3600)
			    tinc = 3 * 3600;
		    if(incr >= 24 * 3600)
			    tinc = 6 * 3600;
		    break;
		case TIMELEVEL_DAYS:
		    if(incr > 2 * 3600)
			    tinc = 3600;
		    if(incr > 4 * 3600)
			    tinc = 2 * 3600;
		    if(incr > 7 * 3600)
			    tinc = 3 * 3600;
		    if(incr > 13 * 3600)
			    tinc = 6 * 3600;
		    if(incr > DAY_SEC)
			    tinc = 12 * 3600;
		    if(incr > 2 * DAY_SEC)
			    tinc = DAY_SEC;
		    break;
		case TIMELEVEL_WEEKS:
		    if(incr > 2 * DAY_SEC)
			    tinc = DAY_SEC;
		    if(incr > 7 * DAY_SEC)
			    tinc = 7 * DAY_SEC;
		    break;
		case TIMELEVEL_MONTHS:
		    if(incr > 2 * DAY_SEC)
			    tinc = DAY_SEC;
		    if(incr > 15 * DAY_SEC)
			    tinc = 10 * DAY_SEC;
		    if(incr > 2 * MON_SEC)
			    tinc = MON_SEC;
		    if(incr > 6 * MON_SEC)
			    tinc = 3 * MON_SEC;
		    if(incr > 2 * YEAR_SEC)
			    tinc = YEAR_SEC;
		    break;
		case TIMELEVEL_YEARS:
		    if(incr > 2 * MON_SEC)
			    tinc = MON_SEC;
		    if(incr > 6 * MON_SEC)
			    tinc = 3 * MON_SEC;
		    if(incr > 2 * YEAR_SEC)
			    tinc = YEAR_SEC;
		    if(incr > 10 * YEAR_SEC)
			    tinc = 5 * YEAR_SEC;
		    if(incr > 50 * YEAR_SEC)
			    tinc = 10 * YEAR_SEC;
		    if(incr > 100 * YEAR_SEC)
			    tinc = 20 * YEAR_SEC;
		    if(incr > 200 * YEAR_SEC)
			    tinc = 50 * YEAR_SEC;
		    if(incr > 300 * YEAR_SEC)
			    tinc = 100 * YEAR_SEC;
		    break;
	}
	return (tinc);
}

/* }}} */

/* {{{ copy_or_invent_formatstring() */
/* Rarely called helper function looks_like_numeric() */
static int FASTCALL looks_like_numeric(const char * format)
{
	if(!(format = strchr(format, '%')))
		return 0;
	while(++format && (*format == ' ' || *format == '-' || *format == '+' || *format == '#'))
		;               /* do nothing */
	while(isdigit((uchar)*format) || *format == '.')
		++format;
	return (*format == 'e' || *format == 'f' || *format == 'g' || *format == 'h');
}
// 
// Either copies axis->formatstring to axis->ticfmt, or
// in case that's not applicable because the format hasn't been
// specified correctly, invents a time/date output format by looking
// at the range of values.  Considers time/date fields that don't
// change across the range to be unimportant 
// 
char * copy_or_invent_formatstring(GpAxis * pAx)
{
	struct tm t_min, t_max;
	char tempfmt[MAX_ID_LEN+1];
	memzero(tempfmt, sizeof(tempfmt));
	if(pAx->tictype != DT_TIMEDATE || !looks_like_numeric(pAx->formatstring)) {
		// The simple case: formatstring is usable, so use it! 
		strncpy(tempfmt, pAx->formatstring, MAX_ID_LEN);
		// Ensure enough precision to distinguish tics 
		if(!strcmp(tempfmt, DEF_FORMAT)) {
			double axmin = pAx->min;
			double axmax = pAx->max;
			int precision = fceili(-log10(MIN(fabs(axmax-axmin), fabs(axmin))));
			// FIXME: Does horrible things for large value of precision 
			// FIXME: Didn't I have a better patch for this? 
			if((axmin*axmax > 0) && 4 < precision && precision < 10)
				sprintf(tempfmt, "%%.%df", precision);
		}
		SAlloc::F(pAx->ticfmt);
		pAx->ticfmt = sstrdup(tempfmt);
		return pAx->ticfmt;
	}
	// Else, have to invent an output format string. 
	ggmtime(&t_min, time_tic_just(pAx->timelevel, pAx->min));
	ggmtime(&t_max, time_tic_just(pAx->timelevel, pAx->max));
	if(t_max.tm_year == t_min.tm_year && t_max.tm_yday == t_min.tm_yday) {
		// same day, skip date 
		if(t_max.tm_hour != t_min.tm_hour) {
			strcpy(tempfmt, "%H");
		}
		if(pAx->timelevel < TIMELEVEL_DAYS) {
			if(tempfmt[0])
				strcat(tempfmt, ":");
			strcat(tempfmt, "%M");
		}
		if(pAx->timelevel < TIMELEVEL_HOURS) {
			strcat(tempfmt, ":%S");
		}
	}
	else {
		if(t_max.tm_year != t_min.tm_year) {
			// different years, include year in ticlabel 
			// check convention, day/month or month/day 
			if(strchr(P_TimeFormat, 'm') < strchr(P_TimeFormat, 'd'))
				strcpy(tempfmt, "%m/%d/%");
			else {
				strcpy(tempfmt, "%d/%m/%");
			}
			if(((int)(t_max.tm_year / 100)) != ((int)(t_min.tm_year / 100)))
				strcat(tempfmt, "Y");
			else
				strcat(tempfmt, "y");
		}
		else {
			// Copy day/month order over from input format 
			if(strchr(P_TimeFormat, 'm') < strchr(P_TimeFormat, 'd')) {
				strcpy(tempfmt, "%m/%d");
			}
			else {
				strcpy(tempfmt, "%d/%m");
			}
		}
		if(pAx->timelevel < TIMELEVEL_WEEKS) {
			// Note: seconds can't be useful if there's more than 1 day's worth of data... 
			strcat(tempfmt, "\n%H:%M");
		}
	}
	SAlloc::F(pAx->ticfmt);
	pAx->ticfmt = sstrdup(tempfmt);
	return pAx->ticfmt;
}
/* }}} */

/* {{{ quantize_normal_tics() */
/* the guide parameter was intended to allow the number of tics
 * to depend on the relative sizes of the plot and the font.
 * It is the approximate upper limit on number of tics allowed.
 * But it did not go down well with the users.
 * A value of 20 gives the same behaviour as 3.5, so that is
 * hardwired into the calls to here. Maybe we will restore it
 * to the automatic calculation one day
 */

/* HBB 20020220: Changed to use value itself as first argument, not
 * log10(value).  Done to allow changing the calculation method
 * to avoid numerical problems */
double quantize_normal_tics(double arg, int guide)
{
	/* order of magnitude of argument: */
	double power = pow(10.0, floor(log10(arg)));
	double xnorm = arg / power; /* approx number of decades */
	/* we expect 1 <= xnorm <= 10 */
	double posns = guide / xnorm; /* approx number of tic posns per decade */
	/* with guide=20, we expect 2 <= posns <= 20 */
	double tics;

	/* HBB 20020220: Looking at these, I would normally expect
	 * to see posns*tics to be always about the same size. But we
	 * rather suddenly drop from 2.0 to 1.0 at tic step 0.5. Why? */
	/* JRV 20021117: fixed this by changing next to last threshold
	   from 1 to 2.  However, with guide=20, this doesn't matter. */
	if(posns > 40)
		tics = 0.05;    /* eg 0, .05, .10, ... */
	else if(posns > 20)
		tics = 0.1;     /* eg 0, .1, .2, ... */
	else if(posns > 10)
		tics = 0.2;     /* eg 0,0.2,0.4,... */
	else if(posns > 4)
		tics = 0.5;     /* 0,0.5,1, */
	else if(posns > 2)
		tics = 1;       /* 0,1,2,.... */
	else if(posns > 0.5)
		tics = 2;       /* 0, 2, 4, 6 */
	else
		/* getting desperate... the ceil is to make sure we
		 * go over rather than under - eg plot [-10:10] x*x
		 * gives a range of about 99.999 - tics=xnorm gives
		 * tics at 0, 99.99 and 109.98  - BAD !
		 * This way, inaccuracy the other way will round
		 * up (eg 0->100.0001 => tics at 0 and 101
		 * I think latter is better than former
		 */
		tics = ceil(xnorm);
	return (tics * power);
}

/* }}} */

/* {{{ make_tics() */
/* Implement TIC_COMPUTED case, i.e. automatically choose a usable
 * ticking interval for the given axis. For the meaning of the guide
 * parameter, see the comment on quantize_normal_tics() */
static double make_tics(GpAxis * pAx, int guide)
{
	double xr = fabs(pAx->min - pAx->max);
	if(xr == 0.0)
		return 1.0; // Anything will do, since we'll never use it 
	else {
		if(xr >= VERYLARGE)
			GPO.IntWarn(NO_CARET, "%s axis range undefined or overflow", axis_name((AXIS_INDEX)pAx->index));
		double tic = quantize_normal_tics(xr, guide);
		// FIXME HBB 20010831: disabling this might allow short log axis to receive better ticking... 
		if(pAx->log && tic < 1.0)
			tic = 1.0;
		return (pAx->tictype == DT_TIMEDATE) ? quantize_time_tics(pAx, tic, xr, guide) : tic;
	}
}

/* }}} */

/* {{{ quantize_duodecimal_tics */
/* HBB 20020220: New function, to be used to properly tic axes with a
 * duodecimal reference, as used in times (60 seconds, 60 minutes, 24
 * hours, 12 months). Derived from quantize_normal_tics(). The default
 * guide is assumed to be 12, here, not 20 */
static double quantize_duodecimal_tics(double arg, int guide)
{
	/* order of magnitude of argument: */
	double power = pow(12.0, floor(log(arg)/log(12.0)));
	double xnorm = arg / power; /* approx number of decades */
	double posns = guide / xnorm; /* approx number of tic posns per decade */
	if(posns > 24)
		return power / 24; /* half a smaller unit --- shouldn't happen */
	else if(posns > 12)
		return power / 12; /* one smaller unit */
	else if(posns > 6)
		return power / 6; /* 2 smaller units = one-6th of a unit */
	else if(posns > 4)
		return power / 4; /* 3 smaller units = quarter unit */
	else if(posns > 2)
		return power / 2; /* 6 smaller units = half a unit */
	else if(posns > 1)
		return power;   /* 0, 1, 2, ..., 11 */
	else if(posns > 0.5)
		return power * 2;       /* 0, 2, 4, ..., 10 */
	else if(posns > 1.0/3)
		return power * 3;       /* 0, 3, 6, 9 */
	else
		/* getting desperate... the ceil is to make sure we
		 * go over rather than under - eg plot [-10:10] x*x
		 * gives a range of about 99.999 - tics=xnorm gives
		 * tics at 0, 99.99 and 109.98  - BAD !
		 * This way, inaccuracy the other way will round
		 * up (eg 0->100.0001 => tics at 0 and 101
		 * I think latter is better than former
		 */
		return power * ceil(xnorm);
}

/* }}} */

/* {{{ quantize_time_tics */
/* Look at the tic interval given, and round it to a nice figure
 * suitable for time/data axes, i.e. a small integer number of
 * seconds, minutes, hours, days, weeks or months. As a side effec,
 * this routine also modifies the axis.timelevel to indicate
 * the units these tics are calculated in. */
static double quantize_time_tics(GpAxis * axis, double tic, double xr, int guide)
{
	int guide12 = guide * 3 / 5; /* --> 12 for default of 20 */
	axis->timelevel = TIMELEVEL_SECONDS;
	if(tic > 5) {
		/* turn tic into units of minutes */
		tic = quantize_duodecimal_tics(xr / 60.0, guide12) * 60;
		if(tic >= 60)
			axis->timelevel = TIMELEVEL_MINUTES;
	}
	if(tic > 5 * 60) {
		/* turn tic into units of hours */
		tic = quantize_duodecimal_tics(xr / 3600.0, guide12) * 3600;
		if(tic >= 3600)
			axis->timelevel = TIMELEVEL_HOURS;
	}
	if(tic > 3600) {
		/* turn tic into units of days */
		tic = quantize_duodecimal_tics(xr / DAY_SEC, guide12) * DAY_SEC;
		if(tic >= DAY_SEC)
			axis->timelevel = TIMELEVEL_DAYS;
	}
	if(tic > 2 * DAY_SEC) {
		/* turn tic into units of weeks */
		tic = quantize_normal_tics(xr / WEEK_SEC, guide) * WEEK_SEC;
		if(tic < WEEK_SEC) { /* force */
			tic = WEEK_SEC;
		}
		if(tic >= WEEK_SEC)
			axis->timelevel = TIMELEVEL_WEEKS;
	}
	if(tic > 3 * WEEK_SEC) {
		/* turn tic into units of month */
		tic = quantize_normal_tics(xr / MON_SEC, guide) * MON_SEC;
		if(tic < MON_SEC) { /* force */
			tic = MON_SEC;
		}
		if(tic >= MON_SEC)
			axis->timelevel = TIMELEVEL_MONTHS;
	}
	if(tic > MON_SEC) {
		/* turn tic into units of years */
		tic = quantize_duodecimal_tics(xr / YEAR_SEC, guide12) * YEAR_SEC;
		if(tic >= YEAR_SEC)
			axis->timelevel = TIMELEVEL_YEARS;
	}
	return (tic);
}

/* }}} */

/* {{{ round_outward */
/* HBB 20011204: new function (repeated code ripped out of setup_tics)
 * that rounds an axis endpoint outward. If the axis is a time/date
 * one, take care to round towards the next whole time unit, not just
 * a multiple of the (averaged) tic size */
static double round_outward(GpAxis * this_axis/* Axis to work on */, bool upwards/* extend upwards or downwards? */, double input/* the current endpoint */)
{
	double tic = this_axis->ticstep;
	double result = tic * (upwards ? ceil(input / tic) : floor(input / tic));
	if(this_axis->tictype == DT_TIMEDATE) {
		double ontime = time_tic_just(this_axis->timelevel, result);
		// FIXME: how certain is it that we don't want to *always* return 'ontime'? 
		if((upwards && (ontime > result)) || (!upwards && (ontime < result)))
			return ontime;
	}
	return result;
}
/* }}} */

/* {{{ setup_tics */
/* setup_tics allows max number of tics to be specified but users don't
 * like it to change with size and font, so we always call with max=20.
 * Note that if format is '', yticlin = 0, so this gives division by zero.
 */
void setup_tics(GpAxis * pAx, int max)
{
	double tic = 0.0;
	t_ticdef * ticdef = &(pAx->ticdef);
	// Do we or do we not extend the axis range to the	
	// next integer multiple of the ticstep?		
	bool autoextend_min = LOGIC((pAx->autoscale & AUTOSCALE_MIN) && !(pAx->autoscale & AUTOSCALE_FIXMIN));
	bool autoextend_max = LOGIC((pAx->autoscale & AUTOSCALE_MAX) && !(pAx->autoscale & AUTOSCALE_FIXMAX));
	if(pAx->linked_to_primary || pAx->linked_to_secondary)
		autoextend_min = autoextend_max = false;
	// Apply constraints on autoscaled axis if requested:
	// The range is _expanded_ here only.  Limiting the range is done
	// in the macro STORE_AND_UPDATE_RANGE() of axis.h
	if(pAx->autoscale & AUTOSCALE_MIN) {
		if(pAx->min_constraint & CONSTRAINT_UPPER)
			SETMIN(pAx->min, pAx->min_ub);
	}
	if(pAx->autoscale & AUTOSCALE_MAX) {
		if(pAx->max_constraint & CONSTRAINT_LOWER)
			SETMAX(pAx->max, pAx->max_lb);
	}
	// HBB 20000506: if no tics required for pAx axis, do
	// nothing. This used to be done exactly before each call of setup_tics, anyway... 
	if(pAx->ticmode) {
		if(ticdef->type == TIC_SERIES) {
			pAx->ticstep = tic = ticdef->def.series.incr;
			autoextend_min = autoextend_min && (ticdef->def.series.start == -VERYLARGE);
			autoextend_max = autoextend_max && (ticdef->def.series.end == VERYLARGE);
		}
		else if(ticdef->type == TIC_COMPUTED)
			pAx->ticstep = tic = make_tics(pAx, max);
		else
			autoextend_min = autoextend_max = false; // user-defined, day or month 
		// If an explicit stepsize was set, axis->timelevel wasn't defined,
		// leading to strange misbehaviours of minor tics on time axes.
		// We used to call quantize_time_tics, but that also caused strangeness.
		if(pAx->tictype == DT_TIMEDATE && ticdef->type == TIC_SERIES) {
			if(tic >= 365*24*60*60.0) pAx->timelevel = TIMELEVEL_YEARS;
			else if(tic >=  28*24*60*60.0) pAx->timelevel = TIMELEVEL_MONTHS;
			else if(tic >=   7*24*60*60.0) pAx->timelevel = TIMELEVEL_WEEKS;
			else if(tic >=     24*60*60.0) pAx->timelevel = TIMELEVEL_DAYS;
			else if(tic >=        60*60.0) pAx->timelevel = TIMELEVEL_HOURS;
			else if(tic >=           60.0) pAx->timelevel = TIMELEVEL_MINUTES;
			else pAx->timelevel = TIMELEVEL_SECONDS;
		}
		if(autoextend_min) {
			pAx->min = round_outward(pAx, !(pAx->min < pAx->max), pAx->min);
			if(pAx->min_constraint & CONSTRAINT_LOWER && pAx->min < pAx->min_lb)
				pAx->min = pAx->min_lb;
		}
		if(autoextend_max) {
			pAx->max = round_outward(pAx, pAx->min < pAx->max, pAx->max);
			if(pAx->max_constraint & CONSTRAINT_UPPER && pAx->max > pAx->max_ub)
				pAx->max = pAx->max_ub;
		}
		// Set up ticfmt. If necessary (time axis, but not time/date output format),
		// make up a formatstring that suits the range of data 
		copy_or_invent_formatstring(pAx);
	}
}

/* }}} */

/* {{{  gen_tics */
/*
 * Mar 2015: Modified to take an axis pointer rather than an index into GPO.AxS[].
 */
//void gen_tics(GpAxis * pThis, tic_callback callback)
void GnuPlot::GenTics(GpTermEntry * pTerm, GpAxis * pThis, GpTicCallback cbFunc)
{
	t_ticdef * def = &pThis->ticdef;
	t_minitics_status minitics = pThis->minitics; /* off/default/auto/explicit */
	lp_style_type lgrd = grid_lp;
	lp_style_type mgrd = mgrid_lp;
	// gprintf uses log10() of base 
	double log10_base;
	if(pThis->base == 0.0)
		pThis->base = 10.0;
	log10_base = log10(pThis->base);
	if(!pThis->gridmajor)
		lgrd.l_type = LT_NODRAW;
	if(!pThis->gridminor)
		mgrd.l_type = LT_NODRAW;
	// EAM FIXME - This really shouldn't happen, but it triggers for instance 
	// if x2tics or y2tics are autoscaled but there is no corresponding data. 
	if(pThis->min >= VERYLARGE || pThis->max <= -VERYLARGE)
		return;
	// user-defined tic entries
	// We place them exactly where requested.
	// Note: No minitics in pThis case
	if(def->def.user) {
		ticmark * mark = def->def.user;
		double uncertain = pThis->GetRange() / 10.0;
		//double internal_min = pThis->min - SIGNIF * uncertain;
		//double internal_max = pThis->max + SIGNIF * uncertain;
		RealRange internal_range;
		internal_range.Set(pThis->min - SIGNIF * uncertain, pThis->max + SIGNIF * uncertain);
		// polar labels always +ve, and if rmin has been set, they are relative to rmin.
		if(Gg.Polar && pThis->index == POLAR_AXIS) {
			//internal_min = AxS.__X().min - SIGNIF * uncertain;
			//internal_max = AxS.__X().max + SIGNIF * uncertain;
			internal_range.Set(AxS.__X().min - SIGNIF * uncertain, AxS.__X().max + SIGNIF * uncertain);
		}
		for(mark = def->def.user; mark; mark = mark->next) {
			char label[MAX_ID_LEN]; /* Scratch space to construct a label */
			char * ticlabel = 0; // Points either to ^^ or to some existing text 
			// This condition is only possible if we are in polar mode 
			const double internal = (pThis->index == POLAR_AXIS) ? PolarRadius(mark->position) : mark->position;
			if(pThis->index == THETA_index)
				; // No harm done if the angular placement wraps at 2pi 
			//else if(!inrange(internal, internal_min, internal_max))
			else if(!internal_range.CheckX(internal))
				continue;
			if(mark->level < 0) {
				ticlabel = mark->label; // label read from data file 
			}
			else if(mark->label && !strchr(mark->label, '%')) {
				ticlabel = mark->label; // string constant that contains no format keys 
			}
			else if(pThis->index >= PARALLEL_AXES) {
				// FIXME: needed because axis->ticfmt is not maintained for parallel axes 
				GPrintf(label, sizeof(label), mark->label ? mark->label : pThis->formatstring, log10_base, mark->position);
				ticlabel = label;
			}
			else if(pThis->tictype == DT_TIMEDATE) {
				gstrftime(label, MAX_ID_LEN-1, mark->label ? mark->label : pThis->ticfmt, mark->position);
				ticlabel = label;
			}
			else if(pThis->tictype == DT_DMS) {
				gstrdms(label, mark->label ? mark->label : pThis->ticfmt, mark->position);
				ticlabel = label;
			}
			else {
				GPrintf(label, sizeof(label), mark->label ? mark->label : pThis->ticfmt, log10_base, mark->position);
				ticlabel = label;
			}
			// use NULL instead of label for minor tics with level 1, however, allow labels for minor tics with levels > 1 
			(this->*cbFunc)(pTerm, pThis, internal, (mark->level==1) ? NULL : ticlabel, mark->level, (mark->level>0) ? mgrd : lgrd, NULL);
			// Polar axis tics are mirrored across the origin 
			if(pThis->index == POLAR_AXIS && (pThis->ticmode & TICS_MIRROR)) {
				int save_gridline = lgrd.l_type;
				lgrd.l_type = LT_NODRAW;
				(this->*cbFunc)(pTerm, pThis, -internal, (mark->level==1) ? NULL : ticlabel, mark->level, (mark->level>0) ? mgrd : lgrd, NULL);
				lgrd.l_type = save_gridline;
			}
		}
		if(def->type == TIC_USER)
			return;
	}
	/* series-tics, either TIC_COMPUTED ("autofreq") or TIC_SERIES (user-specified increment)
	 *
	 * We need to distinguish internal user coords from user coords.
	 * Now that we have nonlinear axes (as of version 5.2)
	 *      internal = primary axis, user = secondary axis
	 *		TIC_COMPUTED ("autofreq") tries for equal spacing on primary axis
	 *		TIC_SERIES   requests equal spacing on secondary (user) axis
	 *		minitics are always evenly spaced in user coords
	 */
	{
		double tic;     /* loop counter */
		double internal; /* in internal co-ords */
		double user;    /* in user co-ords */
		double start, step, end;
		int nsteps;
		//double internal_min, internal_max; // to allow for rounding errors 
		RealRange internal_range; // to allow for rounding errors 
		double ministart = 0.0; // internal or user - depends on step 
		double ministep  = 1.0; //
		double miniend   = 1.0; //
		double lmin = pThis->min;
		double lmax = pThis->max;
		reorder_if_necessary(lmin, lmax);
		/* {{{  choose start, step and end */
		switch(def->type) {
			case TIC_SERIES:
			    if(pThis->log && pThis->index != POLAR_AXIS) {
				    // we can tolerate start <= 0 if step and end > 0 
				    if(def->def.series.end <= 0 || def->def.series.incr <= 0)
					    return; /* just quietly ignore */
				    step = def->def.series.incr;
				    start = (def->def.series.start <= 0) ? (step * floor(lmin / step)) : def->def.series.start; // includes case 'undefined, i.e. -VERYLARGE 
				    end = (def->def.series.end == VERYLARGE) ? (step * ceil(lmax / step)) : def->def.series.end;
				    if(def->logscaling) {
					    // This tries to emulate earlier gnuplot versions in handling set log y; set ytics 10
					    if(start <= 0) {
						    start = step;
						    while(start > pThis->linked_to_primary->min)
							    start -= step;
					    }
					    else {
						    start = EvalLinkFunction(pThis->linked_to_primary, start);
					    }
					    step  = EvalLinkFunction(pThis->linked_to_primary, step);
					    end   = EvalLinkFunction(pThis->linked_to_primary, end);
					    lmin = pThis->linked_to_primary->min;
					    lmax = pThis->linked_to_primary->max;
				    }
			    }
			    else {
				    start = def->def.series.start;
				    step = def->def.series.incr;
				    end = def->def.series.end;
				    if(start == -VERYLARGE)
					    start = step * floor(lmin / step);
				    if(end == VERYLARGE)
					    end = step * ceil(lmax / step);
			    }
			    break;
			case TIC_COMPUTED:
			    if(pThis->IsNonLinear()) {
				    lmin = pThis->linked_to_primary->min;
				    lmax = pThis->linked_to_primary->max;
				    reorder_if_necessary(lmin, lmax);
				    pThis->ticstep = make_tics(pThis->linked_to_primary, 20);
				    // It may be that we _always_ want ticstep = 1.0 
					SETMAX(pThis->ticstep, 1.0);
			    }
			    // round to multiple of step 
			    start = pThis->ticstep * floor(lmin / pThis->ticstep);
			    step = pThis->ticstep;
			    end = pThis->ticstep * ceil(lmax / pThis->ticstep);
			    break;
			case TIC_MONTH:
			    start = floor(lmin);
			    end = ceil(lmax);
			    step = floor((end - start) / 12);
			    if(step < 1)
				    step = 1;
			    break;
			case TIC_DAY:
			    start = floor(lmin);
			    end = ceil(lmax);
			    step = floor((end - start) / 14);
			    if(step < 1)
				    step = 1;
			    break;
			default:
			    IntError(NO_CARET, "Internal error : unknown tic type");
			    return; /* avoid gcc -Wall warning about start */
		}
		/* }}} */
		reorder_if_necessary(start, end);
		step = fabs(step);
		if((minitics != MINI_OFF) && (pThis->miniticscale != 0)) {
			FPRINTF((stderr, "axis.c: %d  start = %g end = %g step = %g base = %g\n", __LINE__, start, end, step, pThis->base));
			/* {{{  figure out ministart, ministep, miniend */
			if(minitics == MINI_USER) {
				/* they have said what they want */
				if(pThis->mtic_freq <= 0) {
					minitics = MINI_OFF;
				}
				else if(pThis->IsNonLinear()) {
					// NB: In the case of TIC_COMPUTED pThis is wrong but we'll fix it later 
					double nsteps = pThis->mtic_freq;
					if(pThis->log && nsteps == pThis->base)
						nsteps -= 1;
					ministart = ministep = step / nsteps;
					miniend = step;
				}
				else {
					ministart = ministep = step / pThis->mtic_freq;
					miniend = step;
				}
			}
			else if(pThis->IsNonLinear() && pThis->ticdef.logscaling) {
				// FIXME: Not sure pThis works for all values of step 
				ministart = ministep = step / (pThis->base - 1);
				miniend = step;
			}
			else if(pThis->tictype == DT_TIMEDATE) {
				ministart = ministep = make_auto_time_minitics(pThis->timelevel, step);
				miniend = step * 0.9;
			}
			else if(minitics == MINI_AUTO) {
				int k = static_cast<int>(fabs(step)/pow(10.0, floor(log10(fabs(step)))));
				// so that step == k times some power of 10 
				ministart = ministep = (k==2 ? 0.5 : 0.2) * step;
				miniend = step;
			}
			else
				minitics = MINI_OFF;
			if(ministep <= 0)
				minitics = MINI_OFF; /* dont get stuck in infinite loop */
			/* }}} */
		}
		/* {{{  a few tweaks and checks */
		/* watch rounding errors */
		end += SIGNIF * step;
		/* HBB 20011002: adjusting the endpoints doesn't make sense if
		 * some oversmart user used a ticstep (much) larger than the
		 * yrange itself */
		if(step < (fabs(lmax) + fabs(lmin))) {
			//internal_max = lmax + step * SIGNIF;
			//internal_min = lmin - step * SIGNIF;
			internal_range.Set(lmin - step * SIGNIF, lmax + step * SIGNIF);
		}
		else {
			//internal_max = lmax;
			//internal_min = lmin;
			internal_range.Set(lmin, lmax);
		}
		if(step == 0)
			return; /* just quietly ignore them ! */
		/* }}} */

		// This protects against user error, not precision errors 
		{
			//if((internal_max-internal_min)/step > term->MaxX) {
			const double _sñ = internal_range.GetDistance()/step;
			if(_sñ > pTerm->MaxX) {
				IntWarn(NO_CARET, "Too many axis ticks requested (>%.0g)", _sñ);
				return;
			}
		}
		// This protects against infinite loops if the separation between       
		// two ticks is less than the precision of the control variables.       
		// The for(...) loop here must exactly describe the true loop below.    
		// Furthermore, compiler optimization can muck up pThis test, so we	
		// tell the compiler that the control variables are volatile.           
		nsteps = 0;
		vol_previous_tic = start-step;
		for(vol_this_tic = start; vol_this_tic <= end; vol_this_tic += step) {
			if(fabs(vol_this_tic - vol_previous_tic) < (step/4.)) {
				step = end - start;
				nsteps = 2;
				IntWarn(NO_CARET, "tick interval too small for machine precision");
				break;
			}
			vol_previous_tic = vol_this_tic;
			nsteps++;
		}
		// Special case.  I hate it. 
		if(pThis->index == THETA_index) {
			if(start == 0 && end > 360)
				nsteps--;
		}
		for(tic = start; nsteps > 0; tic += step, nsteps--) {
			// {{{  calc internal and user co-ords 
			if(pThis->index == POLAR_AXIS) {
				// Defer polar conversion until after limit check 
				internal = tic;
				user = tic;
			}
			else if(pThis->IsNonLinear()) {
				if(def->type == TIC_SERIES && def->logscaling)
					user = EvalLinkFunction(pThis, tic);
				else if(def->type == TIC_COMPUTED)
					user = EvalLinkFunction(pThis, tic);
				else
					user = tic;
				internal = tic; /* It isn't really, but pThis makes the range checks work */
			}
			else {
				// Normal case (no log, no link) 
				internal = (pThis->tictype == DT_TIMEDATE) ? time_tic_just(pThis->timelevel, tic) : tic;
				user = CheckZero(internal, step);
			}
			/* }}} */
			/* Allows placement of theta tics outside the range [0:360] */
			if(pThis->index == THETA_index) {
				if(internal > /*internal_max*/internal_range.upp)
					internal -= 360.0;
				if(internal < /*internal_min*/internal_range.low)
					internal += 360.0;
			}
			if(internal > /*internal_max*/internal_range.upp)
				break; // gone too far - end of series = VERYLARGE perhaps 
			if(internal >= /*internal_min*/internal_range.low) {
				// {{{  draw tick via callback 
				switch(def->type) {
					case TIC_DAY: {
					    int d = ffloori(user + 0.5) % 7;
					    if(d < 0)
						    d += 7;
					    (this->*cbFunc)(pTerm, pThis, internal, abbrev_day_names[d], 0, lgrd, def->def.user);
					    break;
				    }
					case TIC_MONTH: {
					    int m = ffloori(user - 1) % 12;
					    if(m < 0)
						    m += 12;
					    (this->*cbFunc)(pTerm, pThis, internal, abbrev_month_names[m], 0, lgrd, def->def.user);
					    break;
				    }
					default: { // comp or series 
					    char label[MAX_ID_LEN]; // Leave room for enhanced text markup 
					    double position = 0;
					    if(pThis->tictype == DT_TIMEDATE) {
						    // If they are doing polar time plot, good luck to them 
						    gstrftime(label, MAX_ID_LEN-1, pThis->ticfmt, (double)user);
					    }
					    else if(pThis->tictype == DT_DMS) {
						    gstrdms(label, pThis->ticfmt, (double)user);
					    }
					    else if(pThis->index == POLAR_AXIS) {
						    user = internal;
						    internal = PolarRadius(user);
						    GPrintf(label, sizeof(label), pThis->ticfmt, log10_base, tic);
					    }
					    else if(pThis->index >= PARALLEL_AXES) {
						    // FIXME: needed because ticfmt is not maintained for parallel axes
						    GPrintf(label, sizeof(label), pThis->formatstring, log10_base, user);
					    }
					    else {
						    GPrintf(label, sizeof(label), pThis->ticfmt, log10_base, user);
					    }
					    // This is where we finally decided to put the tic mark 
					    if(pThis->IsNonLinear() && def->type == TIC_SERIES && def->logscaling)
						    position = user;
					    else if(pThis->IsNonLinear() && (def->type == TIC_COMPUTED))
						    position = user;
					    else
						    position = internal;
					    // Range-limited tic placement 
					    if(def->rangelimited && !inrange(position, pThis->data_min, pThis->data_max))
						    continue;
					    // This writes the tic mark and label 
					    (this->*cbFunc)(pTerm, pThis, position, label, 0, lgrd, def->def.user);
					    // Polar axis tics are mirrored across the origin 
					    if(pThis->index == POLAR_AXIS && (pThis->ticmode & TICS_MIRROR)) {
						    const int save_gridline = lgrd.l_type;
						    lgrd.l_type = LT_NODRAW;
						    (this->*cbFunc)(pTerm, pThis, -position, label, 0, lgrd, def->def.user);
						    lgrd.l_type = save_gridline;
					    }
				    }
				}
				/* }}} */
			}
			if((minitics != MINI_OFF) && (pThis->miniticscale != 0)) {
				// {{{  process minitics 
				double mplace, mtic_user, mtic_internal;
				for(mplace = ministart; mplace < miniend; mplace += ministep) {
					if(pThis->tictype == DT_TIMEDATE) {
						mtic_user = time_tic_just((t_timelevel)(pThis->timelevel - 1), internal + mplace);
						mtic_internal = mtic_user;
					}
					else if((pThis->IsNonLinear() && (def->type == TIC_COMPUTED)) || (pThis->IsNonLinear() && (def->type == TIC_SERIES && def->logscaling))) {
						// Make up for bad calculation of ministart/ministep/miniend 
						double this_major = EvalLinkFunction(pThis, internal);
						double next_major = EvalLinkFunction(pThis, internal+step);
						mtic_user = this_major + mplace/miniend * (next_major - this_major);
						mtic_internal = EvalLinkFunction(pThis->linked_to_primary, mtic_user);
					}
					else if(pThis->IsNonLinear() && pThis->log) {
						mtic_user = internal + mplace;
						mtic_internal = EvalLinkFunction(pThis->linked_to_primary, mtic_user);
					}
					else {
						mtic_user = internal + mplace;
						mtic_internal = mtic_user;
					}
					if(Gg.Polar && pThis->index == POLAR_AXIS) {
						// FIXME: is pThis really the only case where	
						// mtic_internal is the correct position?	
						mtic_user = user + mplace;
						mtic_internal = PolarRadius(mtic_user);
						(this->*cbFunc)(pTerm, pThis, mtic_internal, NULL, 1, mgrd, NULL);
						continue;
					}
					// Range-limited tic placement 
					if(def->rangelimited && !inrange(mtic_user, pThis->data_min, pThis->data_max))
						continue;
					//if(inrange(mtic_internal, internal_min, internal_max) && inrange(mtic_internal, start - step * SIGNIF, end + step * SIGNIF))
					if(internal_range.CheckX(mtic_internal) && inrange(mtic_internal, start - step * SIGNIF, end + step * SIGNIF))
						(this->*cbFunc)(pTerm, pThis, mtic_user, NULL, 1, mgrd, NULL);
				}
				/* }}} */
			}
		}
	}
}

/* }}} */

/* {{{ time_tic_just() */
/* justify ticplace to a proper date-time value */
static double time_tic_just(t_timelevel level, double ticplace)
{
	struct tm tm;
	if(level <= TIMELEVEL_SECONDS) {
		return (ticplace);
	}
	else {
		ggmtime(&tm, ticplace);
		if(level >= TIMELEVEL_MINUTES) { /* units of minutes */
			if(tm.tm_sec > 55)
				tm.tm_min++;
			tm.tm_sec = 0;
		}
		if(level >= TIMELEVEL_HOURS) { /* units of hours */
			if(tm.tm_min > 55)
				tm.tm_hour++;
			tm.tm_min = 0;
		}
		if(level >= TIMELEVEL_DAYS) { /* units of days */
			if(tm.tm_hour > 22) {
				tm.tm_hour = 0;
				tm.tm_mday = 0;
				tm.tm_yday++;
				ggmtime(&tm, gtimegm(&tm));
			}
		}
		// skip it, I have not bothered with weekday so far 
		if(level >= TIMELEVEL_MONTHS) {/* units of month */
			if(tm.tm_mday > 25) {
				tm.tm_mon++;
				if(tm.tm_mon > 11) {
					tm.tm_year++;
					tm.tm_mon = 0;
				}
			}
			tm.tm_mday = 1;
		}
		ticplace = gtimegm(&tm);
		return (ticplace);
	}
}

/* }}} */

/* {{{ axis_output_tics() */
/* HBB 20000416: new routine. Code like this appeared 4 times, once
 * per 2D axis, in graphics.c. Always slightly different, of course,
 * but generally, it's always the same. I distinguish two coordinate
 * directions, here. One is the direction of the axis itself (the one
 * it's "running" along). I refer to the one orthogonal to it as
 * "non-running", below. */
//void axis_output_tics(GpTermEntry * pTerm, AXIS_INDEX axis/* axis number we're dealing with */, int * ticlabel_position/* 'non-running' coordinate */,
    //AXIS_INDEX zeroaxis_basis/* axis to base 'non-running' position of * zeroaxis on */, tic_callback callback/* tic-drawing callback function */)
void GnuPlot::AxisOutputTics(GpTermEntry * pTerm, AXIS_INDEX axis/* axis number we're dealing with */, int * ticlabel_position/* 'non-running' coordinate */,
		AXIS_INDEX zeroaxis_basis/* axis to base 'non-running' position of * zeroaxis on */, GpTicCallback callback/* tic-drawing callback function */)
{
	GpAxis * this_axis = &AxS[axis];
	const bool axis_is_vertical = oneof2(axis, FIRST_Y_AXIS, SECOND_Y_AXIS);
	const bool axis_is_second   = oneof2(axis, SECOND_X_AXIS, SECOND_Y_AXIS);
	int axis_position;      /* 'non-running' coordinate */
	int mirror_position;    /* 'non-running' coordinate, 'other' side */
	double axis_coord = 0.0; /* coordinate of this axis along non-running axis */
	if(oneof2(zeroaxis_basis, SECOND_X_AXIS, SECOND_Y_AXIS)) {
		axis_position = AxS[zeroaxis_basis].term_upper;
		mirror_position = AxS[zeroaxis_basis].term_lower;
	}
	else {
		axis_position = AxS[zeroaxis_basis].term_lower;
		mirror_position = AxS[zeroaxis_basis].term_upper;
	}
	if(axis >= PARALLEL_AXES)
		axis_coord = axis - PARALLEL_AXES + 1;
	if(this_axis->ticmode) {
		// set the globals needed by the _callback() function 
		if(this_axis->tic_rotate == TEXT_VERTICAL && pTerm->text_angle(pTerm, TEXT_VERTICAL)) {
			tic_hjust = axis_is_vertical ? CENTRE : (axis_is_second ? LEFT : RIGHT);
			tic_vjust = axis_is_vertical ? (axis_is_second ? JUST_TOP : JUST_BOT) : JUST_CENTRE;
			rotate_tics = TEXT_VERTICAL;
			if(axis == FIRST_Y_AXIS)
				(*ticlabel_position) += pTerm->ChrV / 2;
			/* EAM - allow rotation by arbitrary angle in degrees      */
			/*       Justification of ytic labels is a problem since   */
			/*	 the position is already [mis]corrected for length */
		}
		else if(this_axis->tic_rotate && pTerm->text_angle(pTerm, this_axis->tic_rotate)) {
			switch(axis) {
				case FIRST_Y_AXIS: /* EAM Purely empirical shift - is there a better? */
				    *ticlabel_position += pTerm->ChrH * 2.5;
				    tic_hjust = RIGHT; 
					break;
				case SECOND_Y_AXIS:         tic_hjust = LEFT;  break;
				case FIRST_X_AXIS:          tic_hjust = LEFT;  break;
				case SECOND_X_AXIS:         tic_hjust = LEFT;  break;
				default:                    tic_hjust = LEFT;  break;
			}
			tic_vjust = JUST_CENTRE;
			rotate_tics = this_axis->tic_rotate;
		}
		else {
			tic_hjust = axis_is_vertical ? (axis_is_second ? LEFT : RIGHT) : CENTRE;
			tic_vjust = axis_is_vertical ? JUST_CENTRE : (axis_is_second ? JUST_BOT : JUST_TOP);
			rotate_tics = 0;
		}
		if(this_axis->manual_justify)
			tic_hjust = this_axis->tic_pos;
		else
			this_axis->tic_pos = tic_hjust;
		tic_mirror = (this_axis->ticmode & TICS_MIRROR) ? mirror_position : -1/* no thank you */;
		if(this_axis->ticmode & TICS_ON_AXIS && !AxS[zeroaxis_basis].log && AxS[zeroaxis_basis].InRange(axis_coord)) {
			tic_start = AxS[zeroaxis_basis].MapI(axis_coord);
			tic_direction = axis_is_second ? 1 : -1;
			if(AxS[axis].ticmode & TICS_MIRROR)
				tic_mirror = tic_start;
			// put text at boundary if axis is close to boundary and the
			// corresponding boundary is switched on 
			if(axis_is_vertical) {
				if(((axis_is_second ? -1 : 1) * (tic_start - axis_position) > (3 * pTerm->ChrH)) || 
					(!axis_is_second && (!(Gg.draw_border & 2))) || (axis_is_second && (!(Gg.draw_border & 8))))
					tic_text = tic_start;
				else
					tic_text = axis_position;
				tic_text += (axis_is_second ? 1 : -1) * pTerm->ChrH;
			}
			else {
				if(((axis_is_second ? -1 : 1) * (tic_start - axis_position) > (2 * pTerm->ChrV)) || 
					(!axis_is_second && (!(Gg.draw_border & 1))) || (axis_is_second && (!(Gg.draw_border & 4))))
					tic_text = static_cast<int>(tic_start + (axis_is_second ? 0 : -this_axis->ticscale * pTerm->TicV));
				else
					tic_text = axis_position;
				tic_text -= pTerm->ChrV;
			}
		}
		else {
			// tics not on axis --> on border 
			tic_start = axis_position;
			tic_direction = (this_axis->TicIn ? 1 : -1) * (axis_is_second ? -1 : 1);
			tic_text = (*ticlabel_position);
		}
		// go for it 
		GenTics(pTerm, &AxS[axis], callback);
		pTerm->text_angle(pTerm, 0); // reset rotation angle 
	}
}

/* }}} */

/* {{{ axis_set_scale_and_range() */
void axis_set_scale_and_range(GpAxis * pAx, int lower, int upper)
{
	pAx->term_scale = (upper - lower) / pAx->GetRange();
	pAx->term_lower = lower;
	pAx->term_upper = upper;
	if(pAx->linked_to_primary && pAx->linked_to_primary->index <= 0) {
		pAx = pAx->linked_to_primary;
		pAx->term_scale = (upper - lower) / pAx->GetRange();
		pAx->term_lower = lower;
		pAx->term_upper = upper;
	}
}

/* }}} */

/* {{{ axis_position_zeroaxis */
static bool axis_position_zeroaxis(AXIS_INDEX axis)
{
	bool is_inside = FALSE;
	GpAxis * p_this = &GPO.AxS[axis];
	/* NB: This is the only place that axis->term_zero is set. */
	/*     So it is important to reach here before plotting.   */
	if((p_this->min > 0.0 && p_this->max > 0.0) || p_this->log) {
		p_this->term_zero = (p_this->max < p_this->min) ? p_this->term_upper : p_this->term_lower;
	}
	else if(p_this->min < 0.0 && p_this->max < 0.0) {
		p_this->term_zero = (p_this->max < p_this->min) ? p_this->term_lower : p_this->term_upper;
	}
	else {
		p_this->term_zero = GPO.AxS[axis].MapI(0.0);
		is_inside = TRUE;
	}
	return is_inside;
}
/* }}} */

//void axis_draw_2d_zeroaxis(GpTermEntry * pTerm, AXIS_INDEX axis, AXIS_INDEX crossaxis)
void GnuPlot::AxisDraw2DZeroAxis(GpTermEntry * pTerm, AXIS_INDEX axis, AXIS_INDEX crossaxis)
{
	GpAxis * p_this = &AxS[axis];
	if(axis_position_zeroaxis(crossaxis) && p_this->zeroaxis) {
		TermApplyLpProperties(pTerm, p_this->zeroaxis);
		if(oneof2(axis, FIRST_X_AXIS, SECOND_X_AXIS)) {
			// zeroaxis is horizontal, at y == 0 
			pTerm->move(pTerm, p_this->term_lower, AxS[crossaxis].term_zero);
			pTerm->vector(pTerm, p_this->term_upper, AxS[crossaxis].term_zero);
		}
		else if(oneof2(axis, FIRST_Y_AXIS, SECOND_Y_AXIS)) {
			// zeroaxis is vertical, at x == 0 
			pTerm->move(pTerm, AxS[crossaxis].term_zero, p_this->term_lower);
			pTerm->vector(pTerm, AxS[crossaxis].term_zero, p_this->term_upper);
		}
	}
}
// 
// Mouse zoom/scroll operations were constructing a series of "set [xyx2y2]range"
// commands for interpretation.  This caused loss of precision.
// This routine replaces the interpreted string with a direct update of the
// axis min/max.   Called from mouse.c (apply_zoom)
// 
//void set_explicit_range(GpAxis * pAx, double newmin, double newmax)
void GnuPlot::SetExplicitRange(GpAxis * pAx, double newmin, double newmax)
{
	pAx->set_min = newmin;
	pAx->set_autoscale &= ~AUTOSCALE_MIN;
	pAx->min_constraint = CONSTRAINT_NONE;
	pAx->set_max = newmax;
	pAx->set_autoscale &= ~AUTOSCALE_MAX;
	pAx->max_constraint = CONSTRAINT_NONE;
	// If this is one end of a linked axis pair, replicate the new range to the
	// linked axis, possibly via a mapping function.
	if(pAx->linked_to_secondary)
		CloneLinkedAxes(pAx, pAx->linked_to_secondary);
	else if(pAx->linked_to_primary)
		CloneLinkedAxes(pAx, pAx->linked_to_primary);
}

//double FASTCALL get_num_or_time(const GpAxis * axis)
double FASTCALL GnuPlot::GetNumOrTime(const GpAxis * pAx)
{
	double value = 0;
	if(pAx && (pAx->datatype == DT_TIMEDATE) && Pgm.IsStringValue(Pgm.GetCurTokenIdx())) {
		struct tm tm;
		double usec;
		char * ss;
		if((ss = TryToGetString()))
			if(GStrPTime(ss, P_TimeFormat, &tm, &usec, &value) == DT_TIMEDATE)
				value = (double)gtimegm(&tm) + usec;
		SAlloc::F(ss);
	}
	else {
		value = RealExpression();
	}
	return value;
}

//static void load_one_range(GpAxis * pAx, double * a, t_autoscale * autoscale, t_autoscale which)
void GnuPlot::LoadOneRange(GpAxis * pAx, double * pA, t_autoscale * pAutoscale, t_autoscale which)
{
	double number;
	assert(oneof2(which, AUTOSCALE_MIN, AUTOSCALE_MAX));
	if(Pgm.EqualsCur("*")) {
		// easy:  do autoscaling!  
		*pAutoscale |= which;
		if(which==AUTOSCALE_MIN) {
			pAx->min_constraint &= ~CONSTRAINT_LOWER;
			pAx->min_lb = 0; /*  dummy entry  */
		}
		else {
			pAx->max_constraint &= ~CONSTRAINT_LOWER;
			pAx->max_lb = 0; /*  dummy entry  */
		}
		Pgm.Shift();
	}
	else {
		/*  this _might_ be autoscaling with constraint or fixed value */
		/*  The syntax of '0 < *...' confuses the parser as he will try to
		    include the '<' as a comparison operator in the expression.
		    Setting scanning_range_in_progress will stop the parser from
		    trying to build an action table if he finds '<' followed by '*'
		    (which would normally trigger a 'invalid expression'),  */
		scanning_range_in_progress = TRUE;
		number = GetNumOrTime(pAx);
		scanning_range_in_progress = FALSE;
		if(Pgm.EndOfCommand())
			IntErrorCurToken("unfinished range");
		if(Pgm.EqualsCur("<")) {
			// this _seems_ to be autoscaling with lower bound 
			Pgm.Shift();
			if(Pgm.EndOfCommand()) {
				IntErrorCurToken("unfinished range with constraint");
			}
			else if(Pgm.EqualsCur("*")) {
				// okay:  this _is_ autoscaling with lower bound!  
				*pAutoscale |= which;
				if(which==AUTOSCALE_MIN) {
					pAx->min_constraint |= CONSTRAINT_LOWER;
					pAx->min_lb = number;
				}
				else {
					pAx->max_constraint |= CONSTRAINT_LOWER;
					pAx->max_lb = number;
				}
				Pgm.Shift();
			}
			else {
				IntErrorCurToken("malformed range with constraint");
			}
		}
		else if(Pgm.EqualsCur(">")) {
			IntErrorCurToken("malformed range with constraint (use '<' only)");
		}
		else {
			// no autoscaling-with-lower-bound but simple fixed value only  
			*pAutoscale &= ~which;
			if(which==AUTOSCALE_MIN) {
				pAx->min_constraint = CONSTRAINT_NONE;
				pAx->min_ub = 0; /*  dummy entry  */
			}
			else {
				pAx->max_constraint = CONSTRAINT_NONE;
				pAx->max_ub = 0; /*  dummy entry  */
			}
			*pA = number;
		}
	}
	if(*pAutoscale & which) {
		// check for upper bound only if autoscaling is on  
		if(Pgm.EndOfCommand()) 
			IntErrorCurToken("unfinished range");
		if(Pgm.EqualsCur("<")) {
			// looks like upper bound up to now... 
			Pgm.Shift();
			if(Pgm.EndOfCommand()) 
				IntErrorCurToken("unfinished range with constraint");
			number = GetNumOrTime(pAx);
			// this autoscaling has an upper bound: 
			if(which==AUTOSCALE_MIN) {
				pAx->min_constraint |= CONSTRAINT_UPPER;
				pAx->min_ub = number;
			}
			else {
				pAx->max_constraint |= CONSTRAINT_UPPER;
				pAx->max_ub = number;
			}
		}
		else if(Pgm.EqualsCur(">")) {
			IntErrorCurToken("malformed range with constraint (use '<' only)");
		}
		else {
			// there is _no_ upper bound on this autoscaling 
			if(which==AUTOSCALE_MIN) {
				pAx->min_constraint &= ~CONSTRAINT_UPPER;
				pAx->min_ub = 0; /*  dummy entry  */
			}
			else {
				pAx->max_constraint &= ~CONSTRAINT_UPPER;
				pAx->max_ub = 0; /*  dummy entry  */
			}
		}
	}
	else if(!Pgm.EndOfCommand()) {
		// no autoscaling = fixed value --> complain about constraints 
		if(Pgm.EqualsCur("<") || Pgm.EqualsCur(">") ) {
			IntErrorCurToken("no upper bound constraint allowed if not autoscaling");
		}
	}
	// Consistency check  
	if(*pAutoscale & which) {
		if(which==AUTOSCALE_MIN && pAx->min_constraint==CONSTRAINT_BOTH) {
			if(pAx->min_ub < pAx->min_lb) {
				IntWarnCurToken("Upper bound of constraint < lower bound:  Turning of constraints.");
				pAx->min_constraint = CONSTRAINT_NONE;
			}
		}
		if(which==AUTOSCALE_MAX && pAx->max_constraint==CONSTRAINT_BOTH) {
			if(pAx->max_ub < pAx->max_lb) {
				IntWarnCurToken("Upper bound of constraint < lower bound:  Turning of constraints.");
				pAx->max_constraint = CONSTRAINT_NONE;
			}
		}
	}
}
//
// {{{ load_range() */
// loads a range specification from the input line into variables 'a' and 'b' 
//
//t_autoscale load_range(GpAxis * pAx, double * pA, double * pB, t_autoscale autoscale)
t_autoscale GnuPlot::LoadRange(GpAxis * pAx, double * pA, double * pB, t_autoscale autoscale)
{
	if(Pgm.EqualsCur("]")) {
		pAx->min_constraint = CONSTRAINT_NONE;
		pAx->max_constraint = CONSTRAINT_NONE;
		return (autoscale);
	}
	if(Pgm.EndOfCommand()) {
		IntErrorCurToken("starting range value or ':' or 'to' expected");
	}
	else if(!Pgm.EqualsCur("to") && !Pgm.EqualsCur(":")) {
		LoadOneRange(pAx, pA, &autoscale, AUTOSCALE_MIN);
	}
	if(!Pgm.EqualsCur("to") && !Pgm.EqualsCur(":"))
		IntErrorCurToken("':' or keyword 'to' expected");
	Pgm.Shift();
	if(!Pgm.EqualsCur("]")) {
		LoadOneRange(pAx, pB, &autoscale, AUTOSCALE_MAX);
	}
	// Not all the code can deal nicely with +/- infinity 
	if(*pA < -VERYLARGE)
		*pA = -VERYLARGE;
	if(*pB > VERYLARGE)
		*pB = VERYLARGE;
	return (autoscale);
}

/* }}} */

/* we determine length of the widest tick label by getting gen_ticks to
 * call this routine with every label
 */

//void widest_tic_callback(GpAxis * this_axis, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels)
void GnuPlot::WidestTicCallback(GpTermEntry * pTerm, GpAxis * this_axis, double place, char * text, int ticlevel, const lp_style_type & rGrid, ticmark * userlabels) // callback
{
	// historically, minitics used to have no text,
	// but now they can, except at ticlevel 1 (and this restriction is there only for compatibility reasons) */
	if(ticlevel != 1) {
		const int len = label_width(text, NULL);
		if(len > widest_tic_strlen)
			widest_tic_strlen = len;
	}
}
//
// get and set routines for range writeback ULIG
//
//void save_writeback_all_axes()
void GnuPlot::SaveWritebackAllAxes()
{
	for(int axis = 0; axis < AXIS_ARRAY_SIZE; axis++) {
		if(AxS[axis].range_flags & RANGE_WRITEBACK) {
			AxS[axis].writeback_min = AxS[axis].min;
			AxS[axis].writeback_max = AxS[axis].max;
		}
	}
}

void check_axis_reversed(AXIS_INDEX axis)
{
	GpAxis * p_this = &GPO.AxS[axis];
	if(((p_this->autoscale & AUTOSCALE_BOTH) == AUTOSCALE_NONE) && (p_this->set_max < p_this->set_min)) {
		p_this->min = p_this->set_min;
		p_this->max = p_this->set_max;
	}
}

bool some_grid_selected()
{
	for(/*AXIS_INDEX*/int i = (AXIS_INDEX)0; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++)
		if(GPO.AxS[i].gridmajor || GPO.AxS[i].gridminor)
			return TRUE;
	// Dec 2016 - CHANGE 
	if(polar_grid_angle > 0)
		return TRUE;
	if(grid_spiderweb)
		return TRUE;
	return FALSE;
}
//
// Range checks for the color axis.
//
//void set_cbminmax()
void GnuPlot::SetCbMinMax()
{
	GpAxis & r_cb_ax = AxS.__CB();
	if(r_cb_ax.set_autoscale & AUTOSCALE_MIN) {
		if(r_cb_ax.min >= VERYLARGE)
			r_cb_ax.min = AxS.__Z().min;
	}
	r_cb_ax.min = AxisLogValueChecked(COLOR_AXIS, r_cb_ax.min, "color axis");
	if(r_cb_ax.set_autoscale & AUTOSCALE_MAX) {
		if(r_cb_ax.max <= -VERYLARGE)
			r_cb_ax.max = AxS.__Z().max;
	}
	r_cb_ax.max = AxisLogValueChecked(COLOR_AXIS, r_cb_ax.max, "color axis");
	ExchangeToOrder(&r_cb_ax.min, &r_cb_ax.max);
	if(r_cb_ax.linked_to_primary)
		CloneLinkedAxes(&r_cb_ax, r_cb_ax.linked_to_primary);
}

//void save_autoscaled_ranges(const GpAxis * pAxX, const GpAxis * pAxY)
void GpAxisSet::SaveAutoscaledRanges(const GpAxis * pAxX, const GpAxis * pAxY)
{
	if(pAxX) {
		SaveAutoscaledRangeX.low = pAxX->min;
		SaveAutoscaledRangeX.upp = pAxX->max;
	}
	if(pAxY) {
		SaveAutoscaledRangeY.low = pAxY->min;
		SaveAutoscaledRangeY.upp = pAxY->max;
	}
}

//void restore_autoscaled_ranges(GpAxis * pAxX, GpAxis * pAxY)
void GpAxisSet::RestoreAutoscaledRanges(GpAxis * pAxX, GpAxis * pAxY) const
{
	if(pAxX) {
		pAxX->min = SaveAutoscaledRangeX.low;
		pAxX->max = SaveAutoscaledRangeX.upp;
	}
	if(pAxY) {
		pAxY->min = SaveAutoscaledRangeY.low;
		pAxY->max = SaveAutoscaledRangeY.upp;
	}
}

//static void get_position_type(enum position_type * type, AXIS_INDEX * axes)
void GnuPlot::GetPositionType(enum position_type * type, AXIS_INDEX * axes)
{
	if(Pgm.AlmostEqualsCur("fir$st")) {
		Pgm.Shift();
		*type = first_axes;
	}
	else if(Pgm.AlmostEqualsCur("sec$ond")) {
		Pgm.Shift();
		*type = second_axes;
	}
	else if(Pgm.AlmostEqualsCur("gr$aph")) {
		Pgm.Shift();
		*type = graph;
	}
	else if(Pgm.AlmostEqualsCur("sc$reen")) {
		Pgm.Shift();
		*type = screen;
	}
	else if(Pgm.AlmostEqualsCur("char$acter")) {
		Pgm.Shift();
		*type = character;
	}
	else if(Pgm.EqualsCur("polar")) {
		Pgm.Shift();
		*type = polar_axes;
	}
	switch(*type) {
		case first_axes:
		case polar_axes: *axes = FIRST_AXES; return;
		case second_axes: *axes = SECOND_AXES; return;
		default: *axes = NO_AXIS; return;
	}
}
//
// get_position() - reads a position for label,arrow,key,... 
//
//void get_position(GpPosition * pos)
void GnuPlot::GetPosition(GpPosition * pos)
{
	GetPositionDefault(pos, first_axes, 3);
}

//
// parse a position of the form
//   [coords] x, [coords] y {,[coords] z}
// where coords is one of first,second.graph,screen,character
// if first or second, we need to take axis.datatype into account
// FIXME: Cannot handle parallel axes
// 
#define GET_NUMBER_OR_TIME(store, axes, axis)                             \
	do {                                                                    \
		const GpAxis * p_this_axis_ = (axes == NO_AXIS) ? NULL : &(AxS[(axes)+(axis)]); \
		(store) = GetNumOrTime(p_this_axis_);                               \
	} while(0)
// 
// get_position() - reads a position for label,arrow,key,...
// with given default coordinate system
// ndim = 2 only reads x,y
// otherwise it reads x,y,z
// 
//void get_position_default(GpPosition * pos, enum position_type default_type, int ndim)
void GnuPlot::GetPositionDefault(GpPosition * pos, enum position_type default_type, int ndim)
{
	AXIS_INDEX axes;
	enum position_type type = default_type;
	memzero(pos, sizeof(GpPosition));
	GetPositionType(&type, &axes);
	pos->scalex = type;
	GET_NUMBER_OR_TIME(pos->x, axes, FIRST_X_AXIS);
	if(Pgm.EqualsCur(",")) {
		Pgm.Shift();
		GetPositionType(&type, &axes);
		pos->scaley = type;
		GET_NUMBER_OR_TIME(pos->y, axes, FIRST_Y_AXIS);
	}
	else {
		pos->y = 0;
		pos->scaley = type;
	}
	// Resolves ambiguous syntax when trailing comma ends a plot command 
	if(ndim != 2 && Pgm.EqualsCur(",")) {
		Pgm.Shift();
		GetPositionType(&type, &axes);
		// HBB 2015-01-28: no secondary Z axis, so patch up if it was selected 
		if(type == second_axes) {
			type = first_axes;
			axes = FIRST_AXES;
		}
		pos->scalez = type;
		GET_NUMBER_OR_TIME(pos->z, axes, FIRST_Z_AXIS);
	}
	else {
		pos->z = 0;
		pos->scalez = type; /* same as y */
	}
}
// 
// Add a single tic mark, with label, to the list for this axis.
// To avoid duplications and overprints, sort the list and allow
// only one label per position.
// EAM - called from set.c during `set xtics` (level >= 0)
//   called from datafile.c during `plot using ::xtic()` (level = -1)
// 
//void add_tic_user(GpAxis * pAx, const char * pLabel, double position, int level)
void GnuPlot::AddTicUser(GpAxis * pAx, const char * pLabel, double position, int level)
{
	ticmark * tic, * newtic;
	ticmark listhead;
	if(!pLabel && level < 0)
		return;
	// Mark this axis as user-generated ticmarks only, unless the 
	// mix flag indicates that both user- and auto- tics are OK.  
	if(!pAx->ticdef.def.mix)
		pAx->ticdef.type = TIC_USER;
	// Walk along list to sorted positional order 
	listhead.next = pAx->ticdef.def.user;
	listhead.position = -DBL_MAX;
	for(tic = &listhead; tic->next && (position > tic->next->position); tic = tic->next) {
	}
	if((tic->next == NULL) || (position < tic->next->position)) {
		// Make a new ticmark 
		newtic = (ticmark *)SAlloc::M(sizeof(struct ticmark));
		newtic->position = position;
		// Insert it in the list 
		newtic->next = tic->next;
		tic->next = newtic;
	}
	else {
		// The new tic must duplicate position of tic->next 
		if(position != tic->next->position)
			IntWarn(NO_CARET, "add_tic_user: list sort error");
		newtic = tic->next;
		// Don't over-write a major tic with a minor tic 
		if(level == 1)
			return;
		// User-specified tics are preferred to autogenerated tics. 
		if(level == 0 && newtic->level > 1)
			return;
		// FIXME: But are they preferred to data-generated tics?    
		if(newtic->level < level)
			return;
		if(newtic->label) {
			SAlloc::F(newtic->label);
			newtic->label = NULL;
		}
	}
	newtic->level = level;
	newtic->label = pLabel ? sstrdup(pLabel) : 0;
	pAx->ticdef.def.user = listhead.next; // Make sure the listhead is kept 
}

/*
 * Degrees/minutes/seconds geographic coordinate format
 * ------------------------------------------------------------
 *  %D                  = integer degrees, truncate toward zero
 *  %<width.precision>d	= floating point degrees
 *  %M                  = integer minutes, truncate toward zero
 *  %<width.precision>m	= floating point minutes
 *  %S                  = integer seconds, truncate toward zero
 *  %<width.precision>s	= floating point seconds
 *  %E                  = E/W instead of +/-
 *  %N                  = N/S instead of +/-
 */
void gstrdms(char * label, char * format, double value)
{
	double Degrees, Minutes, Seconds;
	double degrees, minutes, seconds;
	int dtype = 0, mtype = 0, stype = 0;
	bool EWflag = FALSE;
	bool NSflag = FALSE;
	char compass = ' ';
	char * c, * cfmt;
	// Limit the range to +/- 180 degrees 
	if(value > 180.0)
		value -= 360.0;
	if(value < -180.0)
		value += 360.0;
	degrees = fabs(value);
	Degrees = floor(degrees);
	minutes = (degrees - (double)Degrees) * 60.0;
	Minutes = floor(minutes);
	seconds = (degrees - (double)Degrees) * 3600.0 -  (double)Minutes*60.0;
	Seconds = floor(seconds);
	for(c = cfmt = sstrdup(format); *c;) {
		if(*c++ == '%') {
			while(*c && !strchr("DdMmSsEN%", *c)) {
				if(!isdigit(*c) && !isspace(*c) && !ispunct(*c))
					GPO.IntError(NO_CARET, "unrecognized format: \"%s\"", format);
				c++;
			}
			switch(*c) {
				case 'D':   *c = 'g'; dtype = 1; degrees = Degrees; break;
				case 'd':   *c = 'f'; dtype = 2; break;
				case 'M':   *c = 'g'; mtype = 1; minutes = Minutes; break;
				case 'm':   *c = 'f'; mtype = 2; break;
				case 'S':   *c = 'g'; stype = 1; seconds = Seconds; break;
				case 's':   *c = 'f'; stype = 2; break;
				case 'E':   *c = 'c'; EWflag = TRUE; break;
				case 'N':   *c = 'c'; NSflag = TRUE; break;
				case '%':   GPO.IntError(NO_CARET, "unrecognized format: \"%s\"", format);
			}
		}
	}
	// By convention the minus sign goes only in front of the degrees 
	// Watch out for round-off errors! 
	if(value < 0 && !EWflag && !NSflag) {
		if(dtype > 0) degrees = -fabs(degrees);
		else if(mtype > 0) minutes = -fabs(minutes);
		else if(stype > 0) seconds = -fabs(seconds);
	}
	if(EWflag)
		compass = (value == 0) ? ' ' : (value < 0) ? 'W' : 'E';
	if(NSflag)
		compass = (value == 0) ? ' ' : (value < 0) ? 'S' : 'N';
	// This is tricky because we have to deal with the possibility that
	// the user may not have specified all the possible format components
	if(dtype == 0) { /* No degrees */
		if(mtype == 0) {
			if(stype == 0) /* Must be some non-DMS format */
				snprintf(label, MAX_ID_LEN, cfmt, value);
			else
				snprintf(label, MAX_ID_LEN, cfmt, seconds, compass);
		}
		else {
			if(stype == 0)
				snprintf(label, MAX_ID_LEN, cfmt, minutes, compass);
			else
				snprintf(label, MAX_ID_LEN, cfmt, minutes, seconds, compass);
		}
	}
	else {  /* Some form of degrees in first field */
		if(mtype == 0) {
			if(stype == 0)
				snprintf(label, MAX_ID_LEN, cfmt, degrees, compass);
			else
				snprintf(label, MAX_ID_LEN, cfmt, degrees, seconds, compass);
		}
		else {
			if(stype == 0)
				snprintf(label, MAX_ID_LEN, cfmt, degrees, minutes, compass);
			else
				snprintf(label, MAX_ID_LEN, cfmt, degrees, minutes, seconds, compass);
		}
	}
	SAlloc::F(cfmt);
}
// 
// Accepts a range of the form [MIN:MAX] or [var=MIN:MAX]
// Loads new limiting values into axis->min axis->max
// Returns
//   0 = no range spec present
//   -1 = range spec with no attached variable name
//   >0 = token indexing the attached variable name
// 
//int parse_range(AXIS_INDEX axis)
int GnuPlot::ParseRange(AXIS_INDEX axisIdx)
{
	GpAxis * this_axis = &AxS[axisIdx];
	int dummy_token = -1;
	if(!Pgm.EqualsCur("[")) // No range present 
		return 0;
	else if(Pgm.EqualsCur("[]")) { // Empty brackets serve as a place holder 
		Pgm.Shift();
		Pgm.Shift();
		return 0;
	}
	else {
		// If the range starts with "[var=" return the token of the named variable. 
		Pgm.Shift();
		if(Pgm.IsLetter(Pgm.GetCurTokenIdx()) && Pgm.EqualsNext("=")) {
			dummy_token = Pgm.GetCurTokenIdx();
			Pgm.Shift();
			Pgm.Shift();
		}
		this_axis->autoscale = LoadRange(this_axis, &this_axis->min, &this_axis->max, this_axis->autoscale);
		// Nonlinear axis - find the linear range equivalent 
		if(this_axis->linked_to_primary) {
			GpAxis * primary = this_axis->linked_to_primary;
			CloneLinkedAxes(this_axis, primary);
		}
		// This handles (imperfectly) the problem case
		//   set link x2 via f(x) inv g(x)
		//   plot [x=min:max][] something that involves x2
		// Other cases of in-line range changes on a linked axis may fail
		else if(this_axis->linked_to_secondary) {
			GpAxis * secondary = this_axis->linked_to_secondary;
			if(secondary->link_udf && secondary->link_udf->at != NULL)
				CloneLinkedAxes(this_axis, secondary);
		}
		if(oneof4(axisIdx, SAMPLE_AXIS, T_AXIS, U_AXIS, V_AXIS)) {
			this_axis->SAMPLE_INTERVAL = 0;
			if(Pgm.EqualsCur(":")) {
				Pgm.Shift();
				this_axis->SAMPLE_INTERVAL = RealExpression();
			}
		}
		if(!Pgm.EqualsCur("]"))
			IntErrorCurToken("']' expected");
		Pgm.Shift();
		return dummy_token;
	}
}
//
// Called if an in-line range is encountered while inside a zoom command 
//
//void parse_skip_range()
void GpProgram::ParseSkipRange()
{
	while(!Equals(CToken++, "]"))
		if(EndOfCommand())
			break;
}
//
// When a secondary axis (axis2) is linked to the corresponding primary
// axis (axis1), this routine copies the relevant range/scale data
//
//void clone_linked_axes(GpAxis * pAx1, GpAxis * pAx2)
void GnuPlot::CloneLinkedAxes(GpAxis * pAx1, GpAxis * pAx2)
{
	double testmin, testmax, scale;
	bool suspect = FALSE;
	memcpy(pAx2, pAx1, AXIS_CLONE_SIZE);
	if(pAx2->link_udf && pAx2->link_udf->at) {
		// Transform the min/max limits of linked secondary axis 
	inverse_function_sanity_check:
		pAx2->set_min = EvalLinkFunction(pAx2, pAx1->set_min);
		pAx2->set_max = EvalLinkFunction(pAx2, pAx1->set_max);
		pAx2->min = EvalLinkFunction(pAx2, pAx1->min);
		pAx2->max = EvalLinkFunction(pAx2, pAx1->max);
		// 
		// Confirm that the inverse mapping actually works, at least at the endpoints.
		// 
		// We makes sure that inverse_f( f(x) ) = x at the edges of our plot
		// bounds, and if not, we throw a warning, and we try to be robust to
		// numerical-precision errors causing false-positive warnings. We look at
		// the error relative to a scaling:
		// 
		// (inverse_f( f(x) ) - x) / scale
		// 
		// where the scale is the mean of (x(min edge of plot), x(max edge of
		// plot)). I.e. we only care about errors that are large on the scale of
		// the plot bounds we're looking at.
		// 
		if(isnan(pAx2->set_min) || isnan(pAx2->set_max))
			suspect = TRUE;
		testmin = EvalLinkFunction(pAx1, pAx2->set_min);
		testmax = EvalLinkFunction(pAx1, pAx2->set_max);
		scale = (fabs(pAx1->set_min) + fabs(pAx1->set_max))/2.0;
		if(isnan(testmin) || isnan(testmax))
			suspect = TRUE;
		if(fabs(testmin - pAx1->set_min) != 0 && fabs((testmin - pAx1->set_min) / scale) > 1.e-6)
			suspect = TRUE;
		if(fabs(testmax - pAx1->set_max) != 0 && fabs((testmax - pAx1->set_max) / scale) > 1.e-6)
			suspect = TRUE;
		if(suspect) {
			// Give it one chance to ignore a bad default range [-10:10] 
			if(((pAx1->autoscale & AUTOSCALE_MIN) == AUTOSCALE_MIN) && pAx1->set_min <= 0 && pAx1->set_max > 0.1) {
				pAx1->set_min = 0.1;
				suspect = FALSE;
				goto inverse_function_sanity_check;
			}
			IntWarn(NO_CARET, "could not confirm linked axis inverse mapping function");
			dump_axis_range(pAx1);
			dump_axis_range(pAx2);
		}
	}
}
//
// Evaluate the function linking secondary axis to primary axis 
//
//double eval_link_function(const GpAxis * pAx, double raw_coord)
double GnuPlot::EvalLinkFunction(const GpAxis * pAx, double raw_coord)
{
	udft_entry * link_udf = pAx->link_udf;
	// A test for if (undefined) is allowed only immediately following
	// either evalute_at() or eval_link_function().  Both must clear it
	// on entry so that the value on return reflects what really happened.
	Ev.IsUndefined_ = false;
	// Special case to speed up evaluation of log scaling 
	// benchmark timing summary
	// v4.6 (old-style logscale)	42.7 u 42.7 total
	// v5.1 (generic nonlinear)     57.5 u 66.2 total
	// v5.1 (optimized nonlinear)	42.1 u 42.2 total
	// 
	if(pAx->log) {
		if(pAx->linked_to_secondary) {
			if(raw_coord <= 0.0)
				return fgetnan();
			else
				return log(raw_coord) / pAx->log_base;
		}
		else if(pAx->linked_to_primary)
			return exp(raw_coord * pAx->log_base);
	}
	// This handles the case "set link x2" with no via/inverse mapping 
	if(!link_udf || !link_udf->at)
		return raw_coord;
	else {
		GpValue a;
		const int dummy_var = (abs(pAx->index) == FIRST_Y_AXIS || abs(pAx->index) == SECOND_Y_AXIS) ? 1 : 0;
		link_udf->dummy_values[1-dummy_var].type = INVALID_NAME;
		Gcomplex(&link_udf->dummy_values[dummy_var], raw_coord, 0.0);
		EvaluateAt(link_udf->at, &a);
		if(Ev.IsUndefined_ || a.type != CMPLX) {
			FPRINTF((stderr, "eval_link_function(%g) returned %s\n", raw_coord, Ev.IsUndefined_ ? "undefined" : "unexpected type"));
			a = Ev.P_UdvNaN->udv_value;
		}
		if(isnan(a.v.cmplx_val.real))
			Ev.IsUndefined_ = true;
		return a.v.cmplx_val.real;
	}
}
// 
// Obtain and initialize a shadow axis.
// The details are hidden from the rest of the code (dynamic/static allocation, etc).
// 
//GpAxis * get_shadow_axis(GpAxis * axis)
GpAxis * GpAxisSet::GetShadowAxis(GpAxis * pAxis)
{
	GpAxis * p_primary = NULL;
	GpAxis * p_secondary = pAxis;
	// This implementation uses a dynamically allocated array of shadow axis
	// structures that is allocated on first use and reused after that. 
	if(!P_ShadowAxArray) {
		P_ShadowAxArray = (GpAxis *)SAlloc::M(NUMBER_OF_MAIN_VISIBLE_AXES * sizeof(GpAxis));
		for(int i = 0; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++)
			memcpy(&P_ShadowAxArray[i], &default_axis_state, sizeof(GpAxis));
	}
	if(pAxis->index != SAMPLE_AXIS && pAxis->index < NUMBER_OF_MAIN_VISIBLE_AXES)
		p_primary = &P_ShadowAxArray[pAxis->index];
	else
		GPO.IntError(NO_CARET, "invalid shadow axis");
	p_primary->index = -p_secondary->index;
	return p_primary;
}

void GpAxisSet::DestroyShadowAxes()
{
	if(P_ShadowAxArray) {
		for(int i = 0; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++)
			P_ShadowAxArray[i].Destroy();
		ZFREE(P_ShadowAxArray);
	}
}

void GpAxisSet::DestroyParallelAxes()
{
	ZFREE(P_ParallelAxArray);
	NumParallelAxes = 0;
}
// 
// This is necessary if we are to reproduce the old logscaling.
// Extend the tic range on an independent log-scaled axis to the
// nearest power of 10.
// Transfer the new limits over to the user-visible secondary axis.
// 
//void extend_primary_ticrange(GpAxis * axis)
void GnuPlot::ExtendPrimaryTicRange(GpAxis * pAx)
{
	GpAxis * primary = pAx->linked_to_primary;
	if(pAx->ticdef.logscaling) {
		// This can happen on "refresh" if the axis was unused 
		if(primary->min >= VERYLARGE || primary->max <= -VERYLARGE)
			return;
		// NB: "zero" is the minimum non-zero value from "set zero" 
		if((primary->autoscale & AUTOSCALE_MIN) || fabs(primary->min - floor(primary->min)) < Gg.Zero) {
			primary->min = floor(primary->min);
			pAx->min = EvalLinkFunction(pAx, primary->min);
		}
		if((primary->autoscale & AUTOSCALE_MAX) || fabs(primary->max - ceil(primary->max)) < Gg.Zero) {
			primary->max = ceil(primary->max);
			pAx->max = EvalLinkFunction(pAx, primary->max);
		}
	}
}
// 
// As data is read in or functions evaluated, the min/max values are tracked
// for the secondary (visible) axes but not for the linked primary (linear) axis.
// This routine fills in the primary min/max from the secondary axis.
// 
//void update_primary_axis_range(GpAxis * secondary)
void GnuPlot::UpdatePrimaryAxisRange(GpAxis * pAxSecondary)
{
	GpAxis * p_ax_primary = pAxSecondary->linked_to_primary;
	if(p_ax_primary) {
		// nonlinear axis (secondary is visible; primary is hidden) 
		p_ax_primary->min = EvalLinkFunction(p_ax_primary, pAxSecondary->min);
		p_ax_primary->max = EvalLinkFunction(p_ax_primary, pAxSecondary->max);
		p_ax_primary->data_min = EvalLinkFunction(p_ax_primary, pAxSecondary->data_min);
		p_ax_primary->data_max = EvalLinkFunction(p_ax_primary, pAxSecondary->data_max);
	}
}
// 
// Same thing but in the opposite direction.  We read in data on the primary axis
// and want the autoscaling on a linked secondary axis to match.
// 
//void update_secondary_axis_range(GpAxis * pAxPrimary)
void GnuPlot::UpdateSecondaryAxisRange(GpAxis * pAxPrimary)
{
	GpAxis * p_ax_secondary = pAxPrimary->linked_to_secondary;
	if(p_ax_secondary) {
		p_ax_secondary->min = EvalLinkFunction(p_ax_secondary, pAxPrimary->min);
		p_ax_secondary->max = EvalLinkFunction(p_ax_secondary, pAxPrimary->max);
		p_ax_secondary->data_min = EvalLinkFunction(p_ax_secondary, pAxPrimary->data_min);
		p_ax_secondary->data_max = EvalLinkFunction(p_ax_secondary, pAxPrimary->data_max);
	}
}
// 
// gnuplot version 5.0 always maintained autoscaled range on x1
// specifically, transforming from x2 coordinates if necessary.
// In version 5.2 we track the x1 and x2 axis data limits separately.
// However if x1 and x2 are linked to each other we must reconcile
// their data limits before plotting.
// 
//void reconcile_linked_axes(GpAxis * pAxPrimary, GpAxis * pAxSecondary)
void GnuPlot::ReconcileLinkedAxes(GpAxis * pAxPrimary, GpAxis * pAxSecondary)
{
	double dummy;
	coord_type inrange = INRANGE;
	if((pAxPrimary->autoscale & AUTOSCALE_BOTH) != AUTOSCALE_NONE && pAxPrimary->linked_to_secondary) {
		double min_2_into_1 = EvalLinkFunction(pAxPrimary, pAxSecondary->data_min);
		double max_2_into_1 = EvalLinkFunction(pAxPrimary, pAxSecondary->data_max);
		// Merge pAxSecondary min/max into pAxPrimary data range 
		store_and_update_range(&dummy, min_2_into_1, &inrange, pAxPrimary, FALSE);
		store_and_update_range(&dummy, max_2_into_1, &inrange, pAxPrimary, FALSE);
		(void)dummy; // Otherwise the compiler complains about an unused variable 
		// Take the result back the other way to update pAxSecondary 
		pAxSecondary->min = EvalLinkFunction(pAxSecondary, pAxPrimary->min);
		pAxSecondary->max = EvalLinkFunction(pAxSecondary, pAxPrimary->max);
	}
}
// 
// Check for linked-axis coordinate transformation given by command
//   set {x|y}2r link via <expr1> inverse <expr2>
// If we are plotting on the secondary axis in this case, apply the inverse
// transform to get back to the primary coordinate system before mapping.
// 
//double map_x_double(double value)
double GnuPlot::MapX(double value)
{
	if(AxS[AxS.Idx_X].linked_to_primary) {
		GpAxis * primary = AxS[AxS.Idx_X].linked_to_primary;
		if(primary->link_udf->at) {
			value = EvalLinkFunction(primary, value);
			return Ev.IsUndefined_ ? fgetnan() : primary->Map(value);
		}
	}
	return AxS[AxS.Idx_X].Map(value);
}

//int map_x(double value)
int GnuPlot::MapiX(double value)
{
	const double x = MapX(value);
	return isnan(x) ? intNaN : GpAxis::MapRealToInt(x);
}

//double map_y_double(double value)
double GnuPlot::MapY(double value)
{
	if(AxS[AxS.Idx_Y].linked_to_primary) {
		GpAxis * primary = AxS[AxS.Idx_Y].linked_to_primary;
		if(primary->link_udf->at) {
			value = EvalLinkFunction(primary, value);
			return Ev.IsUndefined_ ? fgetnan() : primary->Map(value);
		}
	}
	return AxS[AxS.Idx_Y].Map(value);
}

//int map_y(double value)
int GnuPlot::MapiY(double value)
{
	const double y = MapY(value);
	return isnan(y) ? intNaN : GpAxis::MapRealToInt(y);
}
// 
// Convert polar coordinates [theta;r] to the corresponding [x;y]
// If update is TRUE then check and update rrange autoscaling
// 
//coord_type polar_to_xy(double theta, double r, double * x, double * y, bool update)
coord_type GnuPlot::PolarToXY(double theta, double r, double * x, double * y, bool update)
{
	coord_type status = INRANGE;
	// NB: Range checks from multiple original sites are consolidated here.
	// They were not all identical but I hope this version is close enough.
	// One caller (parametric fixup) did GPO.AxS.__R().max range checks
	// against fabs(r) rather than r.  Does that matter?  Did something break?
	if(update) {
		if(Gg.InvertedRaxis) {
			if(!inrange(r, AxS.__R().set_min, AxS.__R().set_max))
				status = OUTRANGE;
		}
		else {
			if(r < AxS.__R().min) {
				if(AxS.__R().autoscale & AUTOSCALE_MIN)
					AxS.__R().min = 0.0;
				else if(AxS.__R().min < 0.0)
					status = OUTRANGE;
				else if(r < 0 && -r > AxS.__R().max)
					status = OUTRANGE;
				else if(r >= 0)
					status = OUTRANGE;
			}
			if(r > AxS.__R().max) {
				if(AxS.__R().autoscale & AUTOSCALE_MAX) {
					if((AxS.__R().max_constraint & CONSTRAINT_UPPER) && (AxS.__R().max_ub < r))
						AxS.__R().max = AxS.__R().max_ub;
					else
						AxS.__R().max = r;
				}
				else
					status = OUTRANGE;
			}
		}
	}
	if(AxS.__R().IsNonLinear()) {
		GpAxis * shadow = AxS.__R().linked_to_primary;
		if(AxS.__R().log && r <= 0)
			r = fgetnan();
		else
			r = EvalLinkFunction(shadow, r) - shadow->min;
	}
	else if(Gg.InvertedRaxis) {
		r = AxS.__R().set_min - r;
	}
	else if((AxS.__R().autoscale & AUTOSCALE_MIN)) {
		; /* Leave it */
	}
	else if(r >= AxS.__R().min) {
		// We store internally as if plotting r(theta) - rmin 
		r = r - AxS.__R().min;
	}
	else if(r < -AxS.__R().min) {
		// If (r < AxS.__R().min < 0) we already flagged OUTRANGE above 
		// That leaves the case (r < 0 && AxS.__R().min >= 0) 
		r = r + AxS.__R().min;
	}
	else {
		*x = fgetnan();
		*y = fgetnan();
		return OUTRANGE;
	}
	// Correct for theta=0 position and handedness 
	theta = theta * theta_direction * Gg.ang2rad + theta_origin * DEG2RAD;
	*x = r * cos(theta);
	*y = r * sin(theta);
	return status;
}
// 
// converts polar coordinate r into a magnitude on x
// allowing for GPO.AxS.__R().min != 0, axis inversion, nonlinearity, etc.
// 
//double polar_radius(double r)
double GnuPlot::PolarRadius(double r)
{
	double px, py;
	PolarToXY(0.0, r, &px, &py, FALSE);
	return sqrt(px*px + py*py);
}
// 
// Print current axis range values to terminal.
// Mostly for debugging.
// 
void dump_axis_range(GpAxis * axis)
{
	if(axis) {
		fprintf(stderr, "    %10.10s axis min/max %10g %10g data_min/max %10g %10g\n", axis_name((AXIS_INDEX)axis->index), axis->min, axis->max, axis->data_min, axis->data_max);
		fprintf(stderr, "                set_min/max %10g %10g \t link:\t %s\n", axis->set_min, axis->set_max, axis->linked_to_primary ? axis_name((AXIS_INDEX)axis->linked_to_primary->index) : "none");
	}
}
// 
// This routine replaces former macro ACTUAL_STORE_AND_UPDATE_RANGE().
// 
// Version 5: OK to store infinities or NaN
// Return UNDEFINED so that caller can take action if desired.
// 
coord_type store_and_update_range(double * pStore, double curval, coord_type * pType, GpAxis * pAx, bool noautoscale)
{
	*pStore = curval;
	if(!(curval > -VERYLARGE && curval < VERYLARGE)) {
		*pType = UNDEFINED;
		return UNDEFINED;
	}
	else {
		if(pAx->log) {
			if(curval < 0.0) {
				*pType = UNDEFINED;
				return UNDEFINED;
			}
			else if(curval == 0.0) {
				*pType = OUTRANGE;
				return OUTRANGE;
			}
		}
		if(noautoscale)
			return (coord_type)0; /* this plot is not being used for autoscaling */
		else if(*pType != INRANGE)
			return (coord_type)0; /* don't set y range if x is outrange, for example */
		else {
			if((curval < pAx->min) && ((curval <= pAx->max) || (pAx->max == -VERYLARGE))) {
				if(pAx->autoscale & AUTOSCALE_MIN) {
					pAx->min = curval;
					if(pAx->min_constraint & CONSTRAINT_LOWER) {
						if(pAx->min_lb > curval) {
							pAx->min = pAx->min_lb;
							*pType = OUTRANGE;
							return OUTRANGE;
						}
					}
				}
				else if(curval != pAx->max) {
					*pType = OUTRANGE;
					return OUTRANGE;
				}
			}
			if(curval > pAx->max && (curval >= pAx->min || pAx->min == VERYLARGE)) {
				if(pAx->autoscale & AUTOSCALE_MAX) {
					pAx->max = curval;
					if(pAx->max_constraint & CONSTRAINT_UPPER) {
						if(pAx->max_ub < curval) {
							pAx->max = pAx->max_ub;
							*pType = OUTRANGE;
							return OUTRANGE;
						}
					}
				}
				else if(curval != pAx->min) {
					*pType = OUTRANGE;
				}
			}
			// Only update data min/max if the point is INRANGE Jun 2016 
			if(*pType == INRANGE) {
				SETMIN(pAx->data_min, curval);
				SETMAX(pAx->data_max, curval);
			}
			return (coord_type)0;
		}
	}
}
// 
// Simplest form of autoscaling (no check on autoscale constraints).
// Used by refresh_bounds() and refresh_3dbounds().
// Used also by autoscale_boxplot.
// FIXME:  Reversed axes are skipped because not skipping them causes errors
//   if apply_zoom->refresh_request->refresh_bounds->autoscale_one_point.
//   But really autoscaling shouldn't be done at all in that case.
// 
/*void FASTCALL autoscale_one_point(GpAxis * axis, double x)
{
	if(!(axis->range_flags & RANGE_IS_REVERSED)) {
		if(axis->set_autoscale & AUTOSCALE_MIN && x < axis->min)
			axis->min = x;
		if(axis->set_autoscale & AUTOSCALE_MAX && x > axis->max)
			axis->max = x;
	}
}*/
