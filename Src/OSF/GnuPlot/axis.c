/* GNUPLOT - axis.c */

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

#include <gnuplot.h>
#pragma hdrstop

//default_axis_zeroaxis
const lp_style_type GpAxis::DefaultAxisZeroAxis(lp_style_type::defZeroAxis); // = DEFAULT_AXIS_ZEROAXIS; // zeroaxis drawing
//GpAxisBlock GpAxB; // @global

GpAxisBlock::GpAxisBlock() : XAxis(FIRST_X_AXIS), YAxis(FIRST_Y_AXIS), ZAxis(FIRST_Z_AXIS),
	default_axis_ticdef(), default_axis_label()
{
	TicScale[0] = 1.0;
	TicScale[1] = 0.5;
	TicScale[2] = 1.0;
	TicScale[3] = 1.0;
	TicScale[4] = 1.0;
	//
	TicStart = 0;
	TicDirection = 0;
	TicMirror = 0;
	TicText = 0;
	RotateTics = 0;
	TicHJust = 0;
	TicVJust = 0;
	//
	NumParallelAxes = 0;
	P_ParallelAxis = 0;
	//
	/*
	XAxis = FIRST_X_AXIS;
	YAxis = FIRST_Y_AXIS;
	ZAxis = FIRST_Z_AXIS;
	*/
	SaveAutoScaleX.Set(0.0, 0.0);
	SaveAutoScaleY.Set(0.0, 0.0);
	//
	//
	//
	vol_this_tic = 0.0;
	vol_previous_tic = 0.0;
	P_TimeFormat = NULL;
	grid_layer = LAYER_BEHIND;
	polar_grid_angle = 0.0;
	widest_tic_strlen = 0;
	grid_tics_in_front = false;
	raxis = true;
	inside_zoom = false;
}

GpAxis & GpAxisBlock::operator [] (size_t idx)
{
	return AxA[idx];
}

//double GpAxisBlock::GetTicScale(int ticLevel, const GpAxis * pAx) const
double GpAxisBlock::GetTicScale(const GpTicCallbackParam * pP) const
{
	return ((pP->TicLevel <= 0) ? pP->P_Ax->ticscale : ((pP->TicLevel == 1) ? 
		pP->P_Ax->miniticscale : ((pP->TicLevel < MAX_TICLEVEL) ? TicScale[pP->TicLevel] : 0.0)));
}

bool GpGadgets::InAxisRange(double val, int axIdx) const
{
	return AxA[axIdx].InRange(val) ? true : false;
}

bool GpGadgets::InAxisRange2(double val1, int axIdx1, double val2, int axIdx2) const
{
	const GpAxis & r_ax1 = AxA[axIdx1];
	const GpAxis & r_ax2 = AxA[axIdx2];
	return (r_ax1.InRange(val1) && r_ax2.InRange(val2)) ? true : false;
}

bool GpGadgets::InAxisRange3(double val1, int axIdx1, double val2, int axIdx2, double val3, int axIdx3) const
{
	const GpAxis & r_ax1 = AxA[axIdx1];
	const GpAxis & r_ax2 = AxA[axIdx2];
	const GpAxis & r_ax3 = AxA[axIdx3];
	return (r_ax1.InRange(val1) && r_ax2.InRange(val2) && r_ax3.InRange(val3)) ? true : false;
}
//
// If we encounter a parallel axis index higher than any used so far,
// extend parallel_axis[] to hold the corresponding data.
// Returns pointer to the new axis.
//
GpAxis * GpAxisBlock::ExtendParallelAxis(uint paxis)
{
	if(paxis > NumParallelAxes) {
		P_ParallelAxis = (GpAxis *)gp_realloc(P_ParallelAxis, paxis * sizeof(GpAxis), "extend parallel_axes");
		for(uint i = NumParallelAxes; i < paxis; i++)
			InitParallelAxis(&P_ParallelAxis[i], (AXIS_INDEX)i);
		NumParallelAxes = (uint)paxis;
	}
	return &P_ParallelAxis[paxis-1];
}

void GpAxisBlock::InitParallelAxis(GpAxis * pAx, AXIS_INDEX index)
{
	//memcpy(this_axis, &default_axis_state, sizeof(GpAxis));
	pAx->SetDefault();
	pAx->formatstring = gp_strdup(DEF_FORMAT);
	pAx->Index = index + PARALLEL_AXES;
	pAx->ticdef.rangelimited = true;
	pAx->SetAutoScale |= (AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX);
}

//
// process 'set ticscale' command
//
//static void set_ticscale()
void GpAxisBlock::SetTicScale(GpCommand & rC)
{
	int i, ticlevel;
	++rC.CToken;
	if(rC.AlmostEq("def$ault")) {
		++rC.CToken;
		for(i = 0; i < AXIS_ARRAY_SIZE; ++i) {
			AxA[i].ticscale = 1.0;
			AxA[i].miniticscale = 0.5;
		}
		TicScale[0] = 1.0;
		TicScale[1] = 0.5;
		for(ticlevel = 2; ticlevel < MAX_TICLEVEL; ticlevel++)
			TicScale[ticlevel] = 1.0;
	}
	else {
		double lminiticscale;
		double lticscale = rC.RealExpression();
		if(rC.Eq(",")) {
			++rC.CToken;
			lminiticscale = rC.RealExpression();
		}
		else {
			lminiticscale = 0.5 * lticscale;
		}
		for(i = 0; i < AXIS_ARRAY_SIZE; ++i) {
			AxA[i].ticscale = lticscale;
			AxA[i].miniticscale = lminiticscale;
		}
		ticlevel = 2;
		while(rC.Eq(",")) {
			++rC.CToken;
			TicScale[ticlevel++] = rC.RealExpression();
			if(ticlevel >= MAX_TICLEVEL)
				break;
		}
	}
}

/* HBB 20000725: gather all per-axis variables into a struct, and set
 * up a single large array of such structs. Next step might be to use
 * isolated GpAxis structs, instead of an array.
 * EAM 2013: tried that.  The problem is that all the routines and macros
 * that manipulate axis data take an index, not a pointer.  We'd have to
 * rewrite all of them and it just didn't seem worth it.
 * Instead I've added additional non-standard entries on the end, used for
 * parallel axis plots if nothing else.
 * Note: This array is now initialized in reset_command().
 */
GpAxis * shadow_axis_array; // @global Only if nonlinear axes are in use
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

// const GpAxis default_axis_state; // = DEFAULT_AXIS_STRUCT;

/* EAM DEBUG - Dynamic allocation of parallel axes. */
//GpAxis * parallel_axis = NULL; // @global
//int    num_parallel_axes = 0;  // @global
//
// HBB 20000506 new variable: parsing table for use with the table
// module, to help generalizing set/show/unset/save, where possible
//
const GenTable axisname_tbl[] =
{
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

/* penalty for doing tics by callback in gen_tics is need for global
 * variables to communicate with the tic routines. Dont need to be
 * arrays for this */
/* HBB 20000416: they may not need to be array[]ed, but it'd sure
 * make coding easier, in some places... */
/* HBB 20000416: for the testing, these are global... */
/* static */
/*
int    tic_start;
int    tic_direction;
int    tic_mirror;
int    tic_text;
int    rotate_tics;
int    tic_hjust;
int    tic_vjust;
*/

/* grid drawing */
/* int grid_selection = GRID_OFF; */
//#define DEFAULT_GRID_LP {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 0.5, 0.0, DEFAULT_P_CHAR, {TC_LT, LT_AXIS, 0.0}, DEFAULT_DASHPATTERN}
//
// Tic scale for tics with level > 1.  0 means 'inherit minitics scale'
//
// double ticscale[MAX_TICLEVEL] = {1, 0.5, 1, 1, 1}; // @global

const lp_style_type default_grid_lp(lp_style_type::defGrid); // = DEFAULT_GRID_LP;
lp_style_type grid_lp(lp_style_type::defGrid);  // = DEFAULT_GRID_LP;
lp_style_type mgrid_lp(lp_style_type::defGrid); // = DEFAULT_GRID_LP;
//
// internal prototypes
//
static double make_auto_time_minitics(t_timelevel, double);
static double make_tics(GpAxis *, int);
static double quantize_time_tics(GpAxis *, double, double, int);
static double time_tic_just(t_timelevel, double);
static double round_outward(GpAxis *, bool, double);
//static bool axis_position_zeroaxis(AXIS_INDEX);
//static void load_one_range(GpAxis * axis, double * a, t_autoscale * pAutoscale, t_autoscale which);
static double quantize_duodecimal_tics(double, int);
static void get_position_type(GpCommand & rC, enum position_type * type, AXIS_INDEX *axes);

/* ---------------------- routines ----------------------- */

/* check range and take logs of min and max if logscale
 * this also restores min and max for ranges like [10:-10]
 */

double GpGadgets::LogValueChecked(AXIS_INDEX axIdx, double coord, const char * pWhat)
{
	const GpAxis & r_ax = AxA[axIdx];
	if(r_ax.Flags & GpAxis::fLog) {
		if(coord <= 0.0) {
			IntError(GpC, NO_CARET, "%s has %s coord of %g; must be above 0 for log scale!", pWhat, GetAxisName(axIdx), coord);
		}
		else 
			return r_ax.DoLog(coord);
	}
	return (coord);
}

/* }}} */

const char * GpAxisBlock::GetAxisName(int axis)
{
	if(axis >= PARALLEL_AXES) {
		(AxNameBuf = 0).Cat("paxis").Space().CatLongZ(axis-PARALLEL_AXES+1, 2);
	}
	else
		AxNameBuf = axis_defaults[axis].name;
	return AxNameBuf;
}

void GpGadgets::InitSampleRange(const GpAxis * pAx)
{
	AxA[SAMPLE_AXIS].range_flags = 0;
	AxA[SAMPLE_AXIS].Range = pAx->Range;
	AxA[SAMPLE_AXIS].P_LinkToPrmr = pAx->P_LinkToPrmr;
}
/* {{{ axis_checked_extend_empty_range() */
/*
 * === SYNOPSIS ===
 *
 * This function checks whether the data and/or plot range in a given axis
 * is too small (which would cause divide-by-zero and/or infinite-loop
 * problems later on).  If so,
 * - if autoscaling is in effect for this axis, we widen the range
 * - otherwise, we abort with a call to  int_error()  (which prints out
 *   a suitable error message, then (hopefully) aborts this command and
 *   returns to the command prompt or whatever).
 *
 *
 * === HISTORY AND DESIGN NOTES ===
 *
 * 1998 Oct 4, Jonathan Thornburg <jthorn@galileo.thp.univie.ac.at>
 *
 * This function used to be a (long) macro  FIXUP_RANGE(GpAxis, WHICH)
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
 *             not +GPVL, max not -GPVL), and int_error() out
 *             if it isn't.
 *
 * Global Variables:
 * auto_array, min_array, max_array (in out) (defined in axis.[ch]):
 *    variables describing the status of autoscaling and range ends, for
 *    each of the possible axes.
 *
 * GpC.CToken = (in) (defined in plot.h) Used in formatting an error message.
 *
 */
void GpGadgets::AxisCheckedExtendEmptyRange(AXIS_INDEX axis, const char * mesg)
{
	GpAxis & r_ax = AxA[axis];
	// These two macro definitions set the range-widening policy:
#define FIXUP_RANGE__WIDEN_ZERO_ABS     1.0  // widen [0:0] by +/- this absolute amount
#define FIXUP_RANGE__WIDEN_NONZERO_REL  0.01 // widen [nonzero:nonzero] by -/+ this relative amount
	RealRange drange = r_ax.Range;
	// HBB 20000501: this same action was taken just before most of
	// the invocations of this function, so I moved it into here.
	// Only do this if 'mesg' is non-NULL --> pass NULL if you don't want the test
	if(mesg && (r_ax.Range.low == GPVL || r_ax.Range.upp == -GPVL))
		IntError(GpC, GpC.CToken, mesg);
	if(drange.GetDistance() == 0.0) {
		// empty range
		if(r_ax.AutoScale) {
			// range came from autoscaling ==> widen it
			double widen = (drange.upp == 0.0) ? FIXUP_RANGE__WIDEN_ZERO_ABS : FIXUP_RANGE__WIDEN_NONZERO_REL * fabs(drange.upp);
			if(!(axis == FIRST_Z_AXIS && !mesg)) /* set view map */
				fprintf(stderr, "Warning: empty %s range [%g:%g], ", GetAxisName(axis), drange.low, drange.upp);
			// HBB 20010525: correctly handle single-ended autoscaling, too:
			if(r_ax.AutoScale & AUTOSCALE_MIN)
				r_ax.Range.low -= widen;
			if(r_ax.AutoScale & AUTOSCALE_MAX)
				r_ax.Range.upp += widen;
			if(!(axis == FIRST_Z_AXIS && !mesg)) // set view map
				fprintf(stderr, "adjusting to [%g:%g]\n", r_ax.Range.low, r_ax.Range.upp);
		}
		else {
			// user has explicitly set the range (to something empty) ==> we're in trouble
			IntError(GpC, NO_CARET, "Can't plot with an empty %s range!", GetAxisName(axis));
		}
	}
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
int looks_like_numeric(const char * format)
{
	if(!(format = strchr(format, '%')))
		return 0;
	while(++format && (*format == ' ' || *format == '-' || *format == '+' || *format == '#'))
		; // do nothing
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
	const char * p_timeformat = GpGg.P_TimeFormat;
	struct tm t_min, t_max;
	char tempfmt[MAX_ID_LEN+1];
	memzero(tempfmt, sizeof(tempfmt));
	if(pAx->tictype != DT_TIMEDATE || !looks_like_numeric(pAx->formatstring)) {
		// The simple case: formatstring is usable, so use it!
		strncpy(tempfmt, pAx->formatstring, MAX_ID_LEN);
		// Ensure enough precision to distinguish tics
		if(!strcmp(tempfmt, DEF_FORMAT)) {
			double axmin = pAx->DeLogValue(pAx->Range.low);
			double axmax = pAx->DeLogValue(pAx->Range.upp);
			int precision = (int)ceil(-log10(MIN(fabs(axmax-axmin), fabs(axmin))));
			// FIXME: Does horrible things for large value of precision
			// FIXME: Didn't I have a better patch for this?
			if((axmin*axmax > 0) && 4 < precision && precision < 10)
				sprintf(tempfmt, "%%.%df", precision);
		}
		free(pAx->ticfmt);
		pAx->ticfmt = _strdup(tempfmt);
	}
	else { // Else, have to invent an output format string
		ggmtime(&t_min, time_tic_just(pAx->timelevel, pAx->Range.low));
		ggmtime(&t_max, time_tic_just(pAx->timelevel, pAx->Range.upp));
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
				if(strchr(p_timeformat, 'm') < strchr(p_timeformat, 'd')) {
					strcpy(tempfmt, "%m/%d/%");
				}
				else {
					strcpy(tempfmt, "%d/%m/%");
				}
				if(((int)(t_max.tm_year / 100)) != ((int)(t_min.tm_year / 100))) {
					strcat(tempfmt, "Y");
				}
				else {
					strcat(tempfmt, "y");
				}
			}
			else {
				// Copy day/month order over from input format
				if(strchr(p_timeformat, 'm') < strchr(p_timeformat, 'd')) {
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
		free(pAx->ticfmt);
		pAx->ticfmt = _strdup(tempfmt);
	}
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
//
// {{{ make_tics()
// Implement TIC_COMPUTED case, i.e. automatically choose a usable
// ticking interval for the given axis. For the meaning of the guide
// parameter, see the comment on quantize_normal_tics()
//
static double make_tics(GpAxis * pAx, int guide)
{
	double tic = 1.0;
	const double xr = fabs(pAx->Range.GetDistance());
	if(xr != 0.0) {
		if(xr >= GPVL)
			int_warn(NO_CARET, "%s axis range undefined or overflow", GpGg.GetAxisName(pAx->Index));
		tic = quantize_normal_tics(xr, guide);
		// FIXME HBB 20010831: disabling this might allow short log axis to receive better ticking...
		if((pAx->Flags & GpAxis::fLog) && tic < 1.0)
			tic = 1.0;
		if(pAx->tictype == DT_TIMEDATE)
			tic = quantize_time_tics(pAx, tic, xr, guide);
	}
	return tic;
}

/* }}} */

/* {{{ quantize_duodecimal_tics */
/* HBB 20020220: New function, to be used to properly tic axes with a
 * duodecimal reference, as used in times (60 seconds, 60 minuts, 24
 * hours, 12 months). Derived from quantize_normal_tics(). The default
 * guide is assumed to be 12, here, not 20 */
static double quantize_duodecimal_tics(double arg, int guide)
{
	/* order of magnitude of argument: */
	double power = pow(12.0, floor(log(arg)/log(12.0)));
	double xnorm = arg / power; /* approx number of decades */
	double posns = guide / xnorm; /* approx number of tic posns per decade */

	if(posns > 24)
		return power / 24;  /* half a smaller unit --- shouldn't happen */
	else if(posns > 12)
		return power / 12;  /* one smaller unit */
	else if(posns > 6)
		return power / 6;  /* 2 smaller units = one-6th of a unit */
	else if(posns > 4)
		return power / 4;  /* 3 smaller units = quarter unit */
	else if(posns > 2)
		return power / 2;  /* 6 smaller units = half a unit */
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
static double round_outward(GpAxis * this_axis,     /* Axis to work on */
    bool upwards,           /* extend upwards or downwards? */
    double input)               /* the current endpoint */
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
//
// setup_tics allows max number of tics to be specified but users don't
// like it to change with size and font, so we always call with max=20.
// Note that if format is '', yticlin = 0, so this gives division by zero.
//
//void setup_tics(GpAxis * pAx, int aMax)
void GpAxis::SetupTics(int aMax)
{
	double _tic = 0.0;
	const t_ticdef * p_ticdef = &(ticdef);
	// Do we or do we not extend the axis range to the	
	// next integer multiple of the ticstep?
	bool autoextend_min = (AutoScale & AUTOSCALE_MIN) && !(AutoScale & AUTOSCALE_FIXMIN);
	bool autoextend_max = (AutoScale & AUTOSCALE_MAX) && !(AutoScale & AUTOSCALE_FIXMAX);
	if(P_LinkToPrmr || P_LinkToScnd)
		autoextend_min = autoextend_max = false;
	// Apply constraints on autoscaled axis if requested:
	// The range is _expanded_ here only.  Limiting the range is done
	// in the macro STORE_WITH_LOG_AND_UPDATE_RANGE() of axis.h  
	if(AutoScale & AUTOSCALE_MIN) {
		if(min_constraint & CONSTRAINT_UPPER) {
			SETMIN(Range.low, Ub.low);
		}
	}
	if(AutoScale & AUTOSCALE_MAX) {
		if(max_constraint & CONSTRAINT_LOWER) {
			SETMAX(Range.upp, Lb.upp);
		}
	}
	// HBB 20000506: if no tics required for pAx axis, do
	// nothing. This used to be done exactly before each call of setup_tics, anyway...
	if(ticmode) {
		if(p_ticdef->type == TIC_SERIES) {
			ticstep = _tic = p_ticdef->def.series.incr;
			autoextend_min = autoextend_min && (p_ticdef->def.series.start == -GPVL);
			autoextend_max = autoextend_max && (p_ticdef->def.series.end == GPVL);
		}
		else if(p_ticdef->type == TIC_COMPUTED) {
			ticstep = _tic = make_tics(this, aMax);
		}
		else {
			// user-defined, day or month 
			autoextend_min = autoextend_max = false;
		}
		// If an explicit stepsize was set, axis->timelevel wasn't defined,
		// leading to strange misbehaviours of minor tics on time axes.
		// We used to call quantize_time_tics, but that also caused strangeness.
		if(tictype == DT_TIMEDATE && p_ticdef->type == TIC_SERIES) {
			if(_tic >= 365*24*60*60.) 
				timelevel = TIMELEVEL_YEARS;
			else if(_tic >=  28*24*60*60.) 
				timelevel = TIMELEVEL_MONTHS;
			else if(_tic >=   7*24*60*60.) 
				timelevel = TIMELEVEL_WEEKS;
			else if(_tic >=     24*60*60.) 
				timelevel = TIMELEVEL_DAYS;
			else if(_tic >=        60*60.) 
				timelevel = TIMELEVEL_HOURS;
			else if(_tic >=           60.) 
				timelevel = TIMELEVEL_MINUTES;
			else 
				timelevel = TIMELEVEL_SECONDS;
		}
		if(autoextend_min) {
			Range.low = round_outward(this, !(Range.low < Range.upp), Range.low);
			if(min_constraint & CONSTRAINT_LOWER)
				SETMAX(Range.low, Lb.low);
		}
		if(autoextend_max) {
			Range.upp = round_outward(this, Range.low < Range.upp, Range.upp);
			if(max_constraint & CONSTRAINT_UPPER)
				SETMIN(Range.upp, Ub.upp);
		}
		// Set up ticfmt. If necessary (time axis, but not time/date output format),
		// make up a formatstring that suits the range of data */
		copy_or_invent_formatstring(this);
	}
}
//
// {{{  gen_tics */
// We use any of GRID_X/Y/X2/Y2/Z and  _MX/_MX2/etc - caller is expected
// to clear the irrelevent fields from global grid bitmask.
// Mar 2015: Modified to take an axis pointer rather than an index into GpGg[].
//
void GpGadgets::GenTics(GpTermEntry * pT, GpAxis & rAx, /*tic_callback*/TicCallback callback)
{
	// EAM FIXME - This really shouldn't happen, but it triggers for instance
	// if x2tics or y2tics are autoscaled but there is no corresponding data.
	if(rAx.Range.low < GPVL && rAx.Range.upp > -GPVL) {
		const t_ticdef * def = &rAx.ticdef;
		t_minitics_status minitics = rAx.minitics; // off/default/auto/explicit
		// gprintf uses log10() of base
		if(rAx.base == 0.0)
			rAx.base = 10.0;
		const double log10_base = log10(rAx.base);
		lp_style_type lgrd = grid_lp;
		lp_style_type mgrd = mgrid_lp;
		if(!(rAx.Flags & GpAxis::fGridMajor))
			lgrd.l_type = LT_NODRAW;
		if(!(rAx.Flags & GpAxis::fGridMinor))
			mgrd.l_type = LT_NODRAW;
		if(def->def.user) {     /* user-defined tic entries */
			const ticmark * mark = def->def.user;
			const double uncertain = rAx.Range.GetDistance() / 10;
			double internal_min = rAx.Range.low - SIGNIF * uncertain;
			double internal_max = rAx.Range.upp + SIGNIF * uncertain;
			double polar_shift = 0;
			//
			// polar labels always +ve, and if rmin has been set, they are relative to rmin.
			//
			if(IsPolar && rAx.Index == POLAR_AXIS) {
				if(!(GetR().AutoScale & AUTOSCALE_MIN))
					polar_shift = GetR().Range.low;
				internal_min = GetX().Range.low - SIGNIF * uncertain;
				internal_max = GetX().Range.upp + SIGNIF * uncertain;
			}
			for(mark = def->def.user; mark; mark = mark->next) {
				char   label[MAX_ID_LEN]; // Scratch space to construct a label
				char * ticlabel; // Points either to ^^ or to some existing text
				double internal;
				// This condition is only possible if we are in polar mode
				if(rAx.Index == POLAR_AXIS) {
#ifdef NONLINEAR_AXES
					if(rAx.P_LinkToPrmr)
						internal = rAx.P_LinkToPrmr->EvalLinkFunction(mark->position) - rAx.P_LinkToPrmr->EvalLinkFunction(polar_shift);
					else
#endif
						internal = rAx.LogValue(mark->position) - rAx.LogValue(polar_shift);
				}
				else {
					internal = rAx.LogValue(mark->position) - polar_shift;
				}
				if(inrange(internal, internal_min, internal_max)) {
					if(mark->level < 0) {
						ticlabel = mark->label; // label read from data file
					}
					else if(mark->label && !strchr(mark->label, '%')) {
						ticlabel = mark->label; // string constant that contains no format keys
					}
					else {
						if(rAx.Index >= PARALLEL_AXES)
							// FIXME: needed because axis->ticfmt is not maintained for parallel axes
							gprintf(label, sizeof(label), NZOR(mark->label, rAx.formatstring), log10_base, mark->position);
						else if(rAx.tictype == DT_TIMEDATE)
							gstrftime(label, MAX_ID_LEN-1, NZOR(mark->label, rAx.ticfmt), mark->position);
						else if(rAx.tictype == DT_DMS)
							gstrdms(label, NZOR(mark->label, rAx.ticfmt), mark->position);
						else
							gprintf(label, sizeof(label), NZOR(mark->label, rAx.ticfmt), log10_base, mark->position);
						ticlabel = label;
					}
					// use NULL instead of label for minor tics with level 1,
					// however, allow labels for minor tics with levels > 1
					{
						GpTicCallbackParam tcb_blk(&rAx, internal, (mark->level==1) ? NULL : ticlabel, mark->level, (mark->level>0) ? mgrd : lgrd, 0);
						((*this).*(callback))(pT, &tcb_blk);
						//callback(&rAx, internal, (mark->level==1) ? NULL : ticlabel, mark->level, (mark->level>0) ? mgrd : lgrd, NULL);
					}
					// Polar axis tics are mirrored across the origin
					if(rAx.Index == POLAR_AXIS && (rAx.ticmode & TICS_MIRROR)) {
						const int save_gridline = lgrd.l_type;
						lgrd.l_type = LT_NODRAW;
						//
						GpTicCallbackParam tcb_blk(&rAx, -internal, (mark->level==1) ? NULL : ticlabel, mark->level, (mark->level>0) ? mgrd : lgrd, 0);
						((*this).*(callback))(pT, &tcb_blk);
						//callback(&rAx, -internal, (mark->level==1) ? NULL : ticlabel, mark->level, (mark->level>0) ? mgrd : lgrd, NULL);
						lgrd.l_type = save_gridline;
					}
				}
			}
			if(def->type == TIC_USER)
				return;
		}
		// series-tics
		// need to distinguish user co-ords from internal co-ords.
		// - for logscale, internal = log(user), else internal = user
		// 
		// The minitics are a bit of a drag - we need to distinuish
		// the cases step>1 from step == 1.
		// If step = 1, we are looking at 1,10,100,1000 for example, so
		// minitics are 2,5,8, ...  - done in user co-ordinates
		// If step>1, we are looking at 1,1e6,1e12 for example, so
		// minitics are 10,100,1000,... - done in internal co-ords
		// 
		{
			double tic; // loop counter 
			double internal; // in internal co-ords 
			double user; // in user co-ords 
			double start, step, end;
			int    nsteps;
			double lmin = rAx.Range.low;
			double lmax = rAx.Range.upp;
			double internal_min, internal_max; /* to allow for rounding errors */
			double ministart = 0, ministep = 1, miniend = 1; /* internal or user - depends on step */
			if(lmin > lmax) {
				Exchange(&lmin, &lmax); // hmm - they have set reversed range for some reason
			}
			/* {{{  choose start, step and end */
			switch(def->type) {
				case TIC_SERIES:
					if(rAx.Flags & GpAxis::fLog && rAx.Index != POLAR_AXIS) {
						// we can tolerate start <= 0 if step and end > 0 
						if(def->def.series.end <= 0 || def->def.series.incr <= 0)
							return;  /* just quietly ignore */
						step  = rAx.DoLog(def->def.series.incr);
						// includes case 'undefined, i.e. -GPVL
						start = (def->def.series.start <= 0)  ? (step * floor(lmin / step)) : rAx.DoLog(def->def.series.start);
						end   = (def->def.series.end == GPVL) ? (step * ceil(lmax / step)) : rAx.DoLog(def->def.series.end);
					}
					else {
						start = def->def.series.start;
						step  = def->def.series.incr;
						end   = def->def.series.end;
						if(start == -GPVL)
							start = step * floor(lmin / step);
						if(end == GPVL)
							end = step * ceil(lmax / step);
					}
					break;
				case TIC_COMPUTED:
	#ifdef NONLINEAR_AXES
					if(rAx.P_LinkToPrmr && rAx.ticdef.logscaling) {
						rAx.ticstep = make_tics(rAx.P_LinkToPrmr, 20);
						lmin = rAx.P_LinkToPrmr->Range.low;
						lmax = rAx.P_LinkToPrmr->Range.upp;
						/* It may be that we _always_ want ticstep = 1.0 */
						SETMAX(rAx.ticstep, 1.0);
					}
	#endif
					/* round to multiple of step */
					start = rAx.ticstep * floor(lmin / rAx.ticstep);
					step = rAx.ticstep;
					end = rAx.ticstep * ceil(lmax / rAx.ticstep);
					break;
				case TIC_MONTH:
					start = floor(lmin);
					end = ceil(lmax);
					step = floor((end - start) / 12);
					SETMAX(step, 1);
					break;
				case TIC_DAY:
					start = floor(lmin);
					end = ceil(lmax);
					step = floor((end - start) / 14);
					SETMAX(step, 1);
					break;
				default:
					IntError(GpC, NO_CARET, "Internal error : unknown tic type");
					return; // avoid gcc -Wall warning about start 
			}
			/* }}} */

			/* {{{  ensure ascending order */
			if(start > end)
				Exchange(&start, &end);
			step = fabs(step);
			/* }}} */
			if((minitics != MINI_OFF) && (rAx.miniticscale != 0)) {
				FPRINTF((stderr, "axis.c: %d  start = %g end = %g step = %g base = %g\n", __LINE__, start, end, step, rAx.base));
				/* {{{  figure out ministart, ministep, miniend */
				if(minitics == MINI_USER) {
					// they have said what they want
					if(rAx.mtic_freq <= 0)
						minitics = MINI_OFF;
					else if((rAx.Flags & GpAxis::fLog) || (rAx.P_LinkToPrmr && def->logscaling)) {
						ministart = ministep = step / rAx.mtic_freq * rAx.base;
						miniend = step * rAx.base;
						// Suppress minitics that would lie on top of major tic
						while(ministart <= 1)
							ministart += ministep;
					}
					else {
						ministart = ministep = step / rAx.mtic_freq;
						miniend = step;
					}
				}
				else if((rAx.Flags & GpAxis::fLog) || (rAx.P_LinkToPrmr && def->logscaling)) {
					if(step > 1.5) { /* beware rounding errors */
						// {{{  10,100,1000 case 
						// no more than five minitics 
						// could be INT_MAX but 54K is already ridiculous
						ministart = (step < 65535) ? (int)(0.2 * step) : (0.2 * step);
						SETMAX(ministart, 1.0);
						ministep = ministart;
						miniend = step;
						/* }}} */
					}
					else {
						// {{{  2,5,8 case
						miniend = rAx.base;
						if((end - start) >= 10)
							minitics = MINI_OFF;
						else if((end - start) >= 5) {
							ministart = 2;
							ministep = 3;
						}
						else {
							ministart = 2;
							ministep = 1;
						}
						// }}}
					}
				}
				else if(rAx.tictype == DT_TIMEDATE) {
					ministart = ministep = make_auto_time_minitics(rAx.timelevel, step);
					miniend = step * 0.9;
				}
				else if(minitics == MINI_AUTO) {
					const int k = (int)(fabs(step)/pow(10., floor(log10(fabs(step)))));
					// so that step == k times some power of 10
					ministart = ministep = (k==2 ? 0.5 : 0.2) * step;
					miniend = step;
				}
				else
					minitics = MINI_OFF;
				if(ministep <= 0)
					minitics = MINI_OFF;  // dont get stuck in infinite loop
				/* }}} */
			}
			// {{{  a few tweaks and checks
			// watch rounding errors
			end += SIGNIF * step;
			// HBB 20011002: adjusting the endpoints doesn't make sense if
			// some oversmart user used a ticstep (much) larger than the yrange itself
			if(step < (fabs(lmax) + fabs(lmin))) {
				internal_max = lmax + step * SIGNIF;
				internal_min = lmin - step * SIGNIF;
			}
			else {
				internal_max = lmax;
				internal_min = lmin;
			}
			if(step == 0)
				return; // just quietly ignore them!
			// }}}
			// This protects against user error, not precision errors
			if((internal_max-internal_min)/step > pT->xmax) {
				int_warn(NO_CARET, "Too many axis ticks requested (>%.0g)", (internal_max-internal_min)/step);
				return;
			}
			/* This protects against infinite loops if the separation between       */
			/* two ticks is less than the precision of the control variables.       */
			/* The for(...) loop here must exactly describe the true loop below.    */
			/* Furthermore, compiler optimization can muck up pAx test, so we	*/
			/* tell the compiler that the control variables are volatile.           */
			nsteps = 0;
			vol_previous_tic = start-step;
			for(vol_this_tic = start; vol_this_tic <= end; vol_this_tic += step) {
				if(fabs(vol_this_tic - vol_previous_tic) < (step/4.)) {
					step = end - start;
					nsteps = 2;
					int_warn(NO_CARET, "tick interval too small for machine precision");
					break;
				}
				vol_previous_tic = vol_this_tic;
				nsteps++;
			}
			for(tic = start; nsteps > 0; tic += step, nsteps--) {
				/* {{{  calc internal and user co-ords */
				if(rAx.Index == POLAR_AXIS) {
					internal = tic; // Defer translation until after limit check
				}
				else if(rAx.Flags & GpAxis::fLog) {
					// log scale => dont need to worry about zero ? 
					internal = tic;
					user = rAx.UndoLog(internal);
	#ifdef NONLINEAR_AXES
				}
				else if(rAx.P_LinkToPrmr && def->type == TIC_COMPUTED && def->logscaling) {
					internal = tic;
					user = rAx.P_LinkToPrmr->P_LinkToScnd->EvalLinkFunction(tic);
	#endif
				}
				else {
					// Normal case (no log, no link) 
					internal = (rAx.tictype == DT_TIMEDATE) ? time_tic_just(rAx.timelevel, tic) : tic;
					user = CheckZero(internal, step);
				}
				/* }}} */
				if(internal > internal_max)
					break;  /* gone too far - end of series = GPVL perhaps */
				if(internal >= internal_min) {
					// {{{  draw tick via callback
					switch(def->type) {
						case TIC_DAY:
							{
								int d = (long)floor(user + 0.5) % 7;
								if(d < 0)
									d += 7;
								GpTicCallbackParam tc_blk(&rAx, internal, abbrev_day_names[d], 0, lgrd, def->def.user);
								((*this).*(callback))(pT, &tc_blk);
								//(*callback)(&rAx, internal, abbrev_day_names[d], 0, lgrd, def->def.user);
							}
							break;
						case TIC_MONTH:
							{
								int m = (long)floor(user - 1) % 12;
								if(m < 0)
									m += 12;
								GpTicCallbackParam tc_blk(&rAx, internal, abbrev_month_names[m], 0, lgrd, def->def.user);
								((*this).*(callback))(pT, &tc_blk);
								//(*callback)(&rAx, internal, abbrev_month_names[m], 0, lgrd, def->def.user);
							}
							break;
						default:
							{ // comp or series
								char   label[MAX_ID_LEN]; // Leave room for enhanced text markup
								double position = 0;
								if(rAx.tictype == DT_TIMEDATE) {
									gstrftime(label, MAX_ID_LEN-1, rAx.ticfmt, (double)user); // If they are doing polar time plot, good luck to them
								}
								else if(rAx.tictype == DT_DMS) {
									gstrdms(label, rAx.ticfmt, (double)user);
								}
								else if(IsPolar && rAx.Index == POLAR_AXIS) {
									const double _min = (GetR().AutoScale & AUTOSCALE_MIN) ? 0 : GetR().Range.low;
									double r = fabs(user) + _min;
									//
									// POLAR_AXIS is the only sane axis, where the actual value
									// is stored and we shift its position just before plotting
									//
									internal = rAx.LogValue(tic) - rAx.LogValue(GetR().Range.low);
									r = tic;
									gprintf(label, sizeof(label), rAx.ticfmt, log10_base, r);
								}
								else if(rAx.Index >= PARALLEL_AXES) {
									// FIXME: needed because ticfmt is not maintained for parallel axes
									gprintf(label, sizeof(label), rAx.formatstring, log10_base, user);
								}
								else {
									gprintf(label, sizeof(label), rAx.ticfmt, log10_base, user);
								}
								// This is where we finally decided to put the tic mark
	#ifdef NONLINEAR_AXES
								if(rAx.P_LinkToPrmr && def->type == TIC_COMPUTED && def->logscaling)
									position = user;
								else
	#endif
									position = internal;
								// Range-limited tic placement
								if(def->rangelimited && !rAx.DataRange.CheckX(position))
									continue;
								{
									GpTicCallbackParam tc_blk(&rAx, position, label, 0, lgrd, def->def.user);
									((*this).*(callback))(pT, &tc_blk);
									//(*callback)(&rAx, position, label, 0, lgrd, def->def.user);
								}
								// Polar axis tics are mirrored across the origin
								if(rAx.Index == POLAR_AXIS && (rAx.ticmode & TICS_MIRROR)) {
									const int save_gridline = lgrd.l_type;
									lgrd.l_type = LT_NODRAW;
									{
										GpTicCallbackParam tc_blk(&rAx, -position, label, 0, lgrd, def->def.user);
										((*this).*(callback))(pT, &tc_blk);
										//(*callback)(&rAx, -position, label, 0, lgrd, def->def.user);
									}
									lgrd.l_type = save_gridline;
								}
							}
					}
					// }}}
				}
				if((minitics != MINI_OFF) && (rAx.miniticscale != 0)) {
					// {{{  process minitics
					for(double mplace = ministart; mplace < miniend; mplace += ministep) {
						double mtic, temptic;
						if(rAx.tictype == DT_TIMEDATE) {
							mtic = time_tic_just((t_timelevel)(rAx.timelevel - 1), internal + mplace);
							temptic = mtic;
#ifdef NONLINEAR_AXES
						}
						else if(rAx.P_LinkToPrmr && def->logscaling) {
							mtic = user * mplace;
							temptic = internal + rAx.P_LinkToPrmr->EvalLinkFunction(mplace);
							FPRINTF((stderr, "minitic internal %g user %g mplace %g step %g temptic %g -> mtic %g\n", internal, user, mplace, step, temptic, mtic));
#endif
						}
						else {
							mtic = internal + ((rAx.Flags & GpAxis::fLog && step <= 1.5) ? rAx.DoLog(mplace) : mplace);
							temptic = mtic;
						}
						if(IsPolar && rAx.Index == POLAR_AXIS)
							temptic += GetR().Range.low;
						// Range-limited tic placement
						if(!def->rangelimited || rAx.DataRange.CheckX(temptic)) {
							if(inrange(temptic, internal_min, internal_max) && inrange(temptic, start - step * SIGNIF, end + step * SIGNIF)) {
								GpTicCallbackParam tc_blk(&rAx, mtic, 0, 1, mgrd, 0);
								((*this).*(callback))(pT, &tc_blk);
								//(*callback)(&rAx, mtic, NULL, 1, mgrd, NULL);
							}
						}
					}
					// }}}
				}
			}
		}
	}
}

/* }}} */

/* {{{ time_tic_just() */
/* justify ticplace to a proper date-time value */
static double time_tic_just(t_timelevel level, double ticplace)
{
	if(level > TIMELEVEL_SECONDS) {
		struct tm tm;
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
		/* skip it, I have not bothered with weekday so far */
		if(level >= TIMELEVEL_MONTHS) { /* units of month */
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
	}
	return ticplace;
}

/* }}} */
//
// {{{ axis_output_tics()
// HBB 20000416: new routine. Code like this appeared 4 times, once
// per 2D axis, in graphics.c. Always slightly different, of course,
// but generally, it's always the same. I distinguish two GpCoordinate
// directions, here. One is the direction of the axis itself (the one
// it's "running" along). I refer to the one orthogonal to it as "non-running", below.
// ARG(ticlabel_position) 'non-running' GpCoordinate
// ARG(zeroaxis_basis)    axis to base 'non-running' position of zeroaxis on
//
void GpGadgets::AxisOutputTics(GpTermEntry * pT, AXIS_INDEX axIdx, int * pTicLabelPosition, AXIS_INDEX zeroAxisBasis, /*tic_callback*/TicCallback callback)
{
	GpAxis & r_ax = AxA[axIdx];
	const bool is_vertical = ((axIdx == FIRST_Y_AXIS) || (axIdx == SECOND_Y_AXIS));
	const bool is_second = AXIS_IS_SECOND(axIdx);
	double axis_coord = 0.0; /* GpCoordinate of this axIdx along non-running axIdx */
	const int  axis_position = AXIS_IS_SECOND(zeroAxisBasis) ? AxA[zeroAxisBasis].TermBounds.upp : AxA[zeroAxisBasis].TermBounds.low;
	const int  mirror_position = AXIS_IS_SECOND(zeroAxisBasis) ? AxA[zeroAxisBasis].TermBounds.low : AxA[zeroAxisBasis].TermBounds.upp;
	if(axIdx >= PARALLEL_AXES)
		axis_coord = axIdx - PARALLEL_AXES + 1;
	if(r_ax.ticmode) {
		// set the globals needed by the _callback() function
		if(r_ax.tic_rotate == TEXT_VERTICAL && (*pT->text_angle)(TEXT_VERTICAL)) {
			TicHJust = is_vertical ? CENTRE : (is_second ? LEFT : RIGHT);
			TicVJust = is_vertical ? (is_second ? JUST_TOP : JUST_BOT) : JUST_CENTRE;
			RotateTics = TEXT_VERTICAL;
			// FIXME HBB 20000501: why would we want this?
			if(axIdx == FIRST_Y_AXIS)
				(*pTicLabelPosition) += pT->VChr / 2;
			// EAM - allow rotation by arbitrary angle in degrees      */
			//       Justification of ytic labels is a problem since   */
			//	 the position is already [mis]corrected for length */
		}
		else if(r_ax.tic_rotate && (*pT->text_angle)(r_ax.tic_rotate)) {
			switch(axIdx) {
				case FIRST_Y_AXIS: /* EAM Purely empirical shift - is there a better? */
				    *pTicLabelPosition += (int)(pT->HChr * 2.5);
				    TicHJust = RIGHT;
					break;
				case SECOND_Y_AXIS:         TicHJust = LEFT;  break;
				case FIRST_X_AXIS:          TicHJust = LEFT;  break;
				case SECOND_X_AXIS:         TicHJust = LEFT;  break;
				default:                    TicHJust = LEFT;  break;
			}
			TicVJust = JUST_CENTRE;
			RotateTics = r_ax.tic_rotate;
		}
		else {
			TicHJust = is_vertical ? (is_second ? LEFT : RIGHT) : CENTRE;
			TicVJust = is_vertical ? JUST_CENTRE : (is_second ? JUST_BOT : JUST_TOP);
			RotateTics = 0;
		}
		if(r_ax.Flags & GpAxis::fManualJustify)
			TicHJust = r_ax.label.pos;
		else
			r_ax.label.pos = (JUSTIFY)TicHJust;
		TicMirror = (r_ax.ticmode & TICS_MIRROR) ? mirror_position : -1/* no thank you */;
		if((r_ax.ticmode & TICS_ON_AXIS) && !(AxA[zeroAxisBasis].Flags & GpAxis::fLog) && AxA[zeroAxisBasis].InRange(axis_coord)) {
			TicStart = Map(zeroAxisBasis, axis_coord);
			TicDirection = is_second ? 1 : -1;
			if(AxA[axIdx].ticmode & TICS_MIRROR)
				TicMirror = TicStart;
			//
			// put text at boundary if axIdx is close to boundary and the
			// corresponding boundary is switched on
			//
			if(is_vertical) {
				if(((is_second ? -1 : 1) * (TicStart - axis_position) > (int)(3 * pT->HChr)) || (!is_second && (!(DrawBorder & 2))) || (is_second && (!(DrawBorder & 8))))
					TicText = TicStart;
				else
					TicText = axis_position;
				TicText += (is_second ? 1 : -1) * pT->HChr;
			}
			else {
				if(((is_second ? -1 : 1) * (TicStart - axis_position) > (int)(2 * pT->VChr)) || (!is_second && (!(DrawBorder & 1))) || (is_second && (!(DrawBorder & 4))))
					TicText = (int)(TicStart + (is_second ? 0 : -r_ax.ticscale * pT->VTic));
				else
					TicText = axis_position;
				TicText -= pT->VChr;
			}
		}
		else {
			// tics not on axIdx --> on border
			TicStart = axis_position;
			TicDirection = ((r_ax.Flags & GpAxis::fTicIn) ? 1 : -1) * (is_second ? -1 : 1);
			TicText = *pTicLabelPosition;
		}
		// go for it
		GenTics(pT, AxA[axIdx], callback);
		(*pT->text_angle)(0); // reset rotation angle 
	}
}

/* }}} */

bool GpGadgets::PositionZeroAxis(AXIS_INDEX axis)
{
	bool is_inside = false;
	GpAxis & r_ax = AxA[axis];
	// NB: This is the only place that axis->term_zero is set.
	//     So it is important to reach here before plotting.
	if((r_ax.Range.low > 0.0 && r_ax.Range.upp > 0.0) || (r_ax.Flags & GpAxis::fLog)) {
		r_ax.term_zero = (r_ax.Range.upp < r_ax.Range.low) ? r_ax.TermBounds.upp : r_ax.TermBounds.low;
	}
	else if(r_ax.Range.low < 0.0 && r_ax.Range.upp < 0.0) {
		r_ax.term_zero = (r_ax.Range.upp < r_ax.Range.low) ? r_ax.TermBounds.low : r_ax.TermBounds.upp;
	}
	else {
		r_ax.term_zero = Map(axis, 0.0);
		is_inside = true;
	}
	return is_inside;
}

void GpGadgets::Draw2DZeroAxis(GpTermEntry * pT, AXIS_INDEX axIdx, AXIS_INDEX crossaxis)
{
	const GpAxis & r_ax = AxA[axIdx];
	if(PositionZeroAxis(crossaxis) && r_ax.zeroaxis) {
		ApplyLpProperties(pT, r_ax.zeroaxis);
		if(oneof2(axIdx, FIRST_X_AXIS, SECOND_X_AXIS)) {
			// zeroaxis is horizontal, at y == 0
			pT->move(r_ax.TermBounds.low, AxA[crossaxis].term_zero);
			pT->vector(r_ax.TermBounds.upp, AxA[crossaxis].term_zero);
		}
		else if(oneof2(axIdx, FIRST_Y_AXIS, SECOND_Y_AXIS)) {
			// zeroaxis is vertical, at x == 0
			pT->move(AxA[crossaxis].term_zero, r_ax.TermBounds.low);
			pT->vector(AxA[crossaxis].term_zero, r_ax.TermBounds.upp);
		}
	}
}

//double get_num_or_time(GpAxis * pAx)
double GpGadgets::GetNumOrTime(GpCommand & rC, GpAxis * pAx)
{
	double value = 0.0;
	if(pAx && (pAx->datatype == DT_TIMEDATE) && rC.IsStringValue(rC.CToken)) {
		struct tm tm;
		double usec;
		char * ss = rC.TryToGetString();
		if(gstrptime(ss, P_TimeFormat, &tm, &usec))
			value = (double)gtimegm(&tm) + usec;
		free(ss);
	}
	else {
		value = rC.RealExpression();
	}
	return value;
}

//static void load_one_range(GpAxis * pAx, double * a, t_autoscale * pAutoscale, t_autoscale which)
void GpAxis::LoadOneRange(GpCommand & rC, double * pA, t_autoscale * pAutoscale, t_autoscale which)
{
	double number;
	assert(oneof2(which, AUTOSCALE_MIN, AUTOSCALE_MAX));
	if(rC.Eq("*")) {
		//  easy:  do autoscaling!
		*pAutoscale |= which;
		if(which==AUTOSCALE_MIN) {
			min_constraint &= ~CONSTRAINT_LOWER;
			Lb.low = 0.0; // dummy entry
		}
		else {
			max_constraint &= ~CONSTRAINT_LOWER;
			Lb.upp = 0.0; // dummy entry
		}
		rC.CToken++;
	}
	else {
		/*  this _might_ be autoscaling with constraint or fixed value */
		/*  The syntax of '0 < *...' confuses the parser as he will try to
		    include the '<' as a comparison operator in the expression.
		    Setting scanning_range_in_progress will stop the parser from
		    trying to build an action table if he finds '<' followed by '*'
		    (which would normaly trigger a 'invalid expression'),  */
		rC.P.IsScanningRangeInProgress = true;
		number = GpGg.GetNumOrTime(rC, this);
		rC.P.IsScanningRangeInProgress = false;
		if(rC.EndOfCommand())
			GpGg.IntError(rC, rC.CToken, "unfinished range");
		if(rC.Eq("<")) {
			// this _seems_ to be autoscaling with lower bound
			rC.CToken++;
			if(rC.EndOfCommand()) {
				GpGg.IntError(rC, rC.CToken, "unfinished range with constraint");
			}
			else if(rC.Eq("*")) {
				// okay:  this _is_ autoscaling with lower bound!
				*pAutoscale |= which;
				if(which == AUTOSCALE_MIN) {
					min_constraint |= CONSTRAINT_LOWER;
					Lb.low = number;
				}
				else {
					max_constraint |= CONSTRAINT_LOWER;
					Lb.upp = number;
				}
				rC.CToken++;
			}
			else {
				GpGg.IntError(rC, rC.CToken, "malformed range with constraint");
			}
		}
		else if(rC.Eq(">")) {
			GpGg.IntError(rC, rC.CToken, "malformed range with constraint (use '<' only)");
		}
		else {
			// no autoscaling-with-lower-bound but simple fixed value only
			*pAutoscale &= ~which;
			if(which==AUTOSCALE_MIN) {
				min_constraint = CONSTRAINT_NONE;
				Ub.low = 0.0; // dummy entry
			}
			else {
				max_constraint = CONSTRAINT_NONE;
				Ub.upp = 0.0; // dummy entry
			}
			*pA = number;
		}
	}
	if(*pAutoscale & which) {
		// check for upper bound only if autoscaling is on
		if(rC.EndOfCommand())
			GpGg.IntError(rC, rC.CToken, "unfinished range");
		if(rC.Eq("<")) {
			/*  looks like upper bound up to now...  */
			rC.CToken++;
			if(rC.EndOfCommand())
				GpGg.IntError(rC, rC.CToken, "unfinished range with constraint");
			number = GpGg.GetNumOrTime(rC, this);
			// this autoscaling has an upper bound:
			if(which==AUTOSCALE_MIN) {
				min_constraint |= CONSTRAINT_UPPER;
				Ub.low = number;
			}
			else {
				max_constraint |= CONSTRAINT_UPPER;
				Ub.upp = number;
			}
		}
		else if(rC.Eq(">")) {
			GpGg.IntError(rC, rC.CToken, "malformed range with constraint (use '<' only)");
		}
		else {
			// there is _no_ upper bound on this autoscaling
			if(which==AUTOSCALE_MIN) {
				min_constraint &= ~CONSTRAINT_UPPER;
				Ub.low = 0.0; // dummy entry
			}
			else {
				max_constraint &= ~CONSTRAINT_UPPER;
				Ub.upp = 0.0; // dummy entry
			}
		}
	}
	else if(!rC.EndOfCommand()) {
		// no autoscaling = fixed value --> complain about constraints
		if(rC.Eq("<") || rC.Eq(">") ) {
			GpGg.IntError(rC, rC.CToken, "no upper bound constraint allowed if not autoscaling");
		}
	}
	// Consitency check
	if(*pAutoscale & which) {
		if(which == AUTOSCALE_MIN && min_constraint==CONSTRAINT_BOTH) {
			if(Ub.low < Lb.low) {
				int_warn(rC.CToken, "Upper bound of constraint < lower bound:  Turning of constraints.");
				min_constraint = CONSTRAINT_NONE;
			}
		}
		if(which == AUTOSCALE_MAX && max_constraint==CONSTRAINT_BOTH) {
			if(Ub.upp < Lb.upp) {
				int_warn(rC.CToken, "Upper bound of constraint < lower bound:  Turning of constraints.");
				max_constraint = CONSTRAINT_NONE;
			}
		}
	}
}
//
// {{{ load_range() */
// loads a range specification from the input line into variables 'a' and 'b'
//
//t_autoscale load_range(GpAxis * this_axis, double * a, double * b, t_autoscale autoscale)
t_autoscale GpAxis::LoadRange(GpCommand & rC, /*double * pA, double * pB*/RealRange & rRange, t_autoscale autoscale)
{
	if(rC.Eq("]")) {
		min_constraint = CONSTRAINT_NONE;
		max_constraint = CONSTRAINT_NONE;
	}
	else {
		if(rC.EndOfCommand()) {
			GpGg.IntError(rC, rC.CToken, "starting range value or ':' or 'to' expected");
		}
		else if(!rC.Eq("to") && !rC.Eq(":")) {
			LoadOneRange(rC, &rRange.low, &autoscale, AUTOSCALE_MIN);
		}
		if(!rC.Eq("to") && !rC.Eq(":"))
			GpGg.IntError(rC, rC.CToken, "':' or keyword 'to' expected");
		rC.CToken++;
		if(!rC.Eq("]")) {
			LoadOneRange(rC, &rRange.upp, &autoscale, AUTOSCALE_MAX);
		}
		// Not all the code can deal nicely with +/- infinity
		if(rRange.low < -GPVL)
			rRange.low = -GPVL;
		if(rRange.upp > GPVL)
			rRange.upp = GPVL;
	}
	return autoscale;
}

/* }}} */

/* we determine length of the widest tick label by getting gen_ticks to
 * call this routine with every label
 */

//void widest_tic_callback(GpTermEntry * pT, GpTicCallbackParam * pP)
void GpGadgets::WidestTicCallback(GpTermEntry * pT, GpTicCallbackParam * pP)
{
	// historically, minitics used to have no text,
	// but now they can, except at ticlevel 1
	// (and this restriction is there only for compatibility reasons) 
	if(pP->TicLevel != 1) {
		const int len = label_width(pP->P_Text, NULL);
		if(len > widest_tic_strlen)
			widest_tic_strlen = len;
	}
}
//
// get and set routines for range writeback ULIG *
//
//void save_writeback_all_axes()
void GpGadgets::SaveWritebackAllAxes()
{
	for(/*AXIS_INDEX*/int axis = FIRST_AXES; axis < AXIS_ARRAY_SIZE; axis++) {
		GpAxis & r_ax = AxA[axis];
		if(r_ax.range_flags & RANGE_WRITEBACK) {
			//r_ax.writeback_min = DelogValue((AXIS_INDEX)axis, r_ax.Range.low);
			//r_ax.writeback_max = DelogValue((AXIS_INDEX)axis, r_ax.Range.upp);
			r_ax.WritebackRange.Set(DelogValue((AXIS_INDEX)axis, r_ax.Range.low), DelogValue((AXIS_INDEX)axis, r_ax.Range.upp));
		}
	}
}

//void check_axis_reversed(AXIS_INDEX axis)
void GpGadgets::CheckAxisReversed(AXIS_INDEX axis)
{
	GpAxis & r_ax = AxA[axis];
	if(((r_ax.AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_NONE) && (r_ax.SetRange.upp < r_ax.SetRange.low)) {
		r_ax.Range = r_ax.SetRange;
	}
}
//
// Range checks for the color axis.
//
void GpGadgets::SetCbMinMax()
{
	GpAxis & r_ax = AxA[COLOR_AXIS]; //GpGg.GetCB();
	if(r_ax.SetAutoScale & AUTOSCALE_MIN) {
		if(r_ax.Range.low >= GPVL)
			r_ax.Range.low = DelogValue(FIRST_Z_AXIS, GetZ().Range.low);
	}
	r_ax.Range.low = LogValueChecked(COLOR_AXIS, r_ax.Range.low, "color axis");
	if(r_ax.SetAutoScale & AUTOSCALE_MAX) {
		if(r_ax.Range.upp <= -GPVL)
			r_ax.Range.upp = DelogValue(FIRST_Z_AXIS, GetZ().Range.upp);
	}
	r_ax.Range.upp = LogValueChecked(COLOR_AXIS, r_ax.Range.upp, "color axis");
	if(r_ax.Range.low > r_ax.Range.upp)
		Exchange(&r_ax.Range.low, &r_ax.Range.upp);
#ifdef NONLINEAR_AXES
	if(r_ax.P_LinkToPrmr)
		clone_linked_axes(&r_ax, r_ax.P_LinkToPrmr);
#endif
}

//void save_autoscaled_ranges(const GpAxis * pAxX, const GpAxis * pAxY)
void GpAxisBlock::SaveAutoscaledRanges(const GpAxis * pAxX, const GpAxis * pAxY)
{
	if(pAxX)
		SaveAutoScaleX = pAxX->Range;
	if(pAxY)
		SaveAutoScaleY = pAxY->Range;
}

void GpAxisBlock::RestoreAutoscaledRanges(GpAxis * pXAx, GpAxis * pYAx)
{
	if(pXAx)
		pXAx->Range = SaveAutoScaleX;
	if(pYAx)
		pYAx->Range = SaveAutoScaleY;
}

static void get_position_type(GpCommand & rC, enum position_type * type, AXIS_INDEX * axes)
{
	if(rC.AlmostEq("fir$st")) {
		++rC.CToken;
		*type = first_axes;
	}
	else if(rC.AlmostEq("sec$ond")) {
		++rC.CToken;
		*type = second_axes;
	}
	else if(rC.AlmostEq("gr$aph")) {
		++rC.CToken;
		*type = graph;
	}
	else if(rC.AlmostEq("sc$reen")) {
		++rC.CToken;
		*type = screen;
	}
	else if(rC.AlmostEq("char$acter")) {
		++rC.CToken;
		*type = character;
	}
	switch(*type) {
		case first_axes:
		    *axes = FIRST_AXES;
		    return;
		case second_axes:
		    *axes = SECOND_AXES;
		    return;
		default:
		    *axes = NO_AXIS;
		    return;
	}
}
//
// get_position() - reads a position for label,arrow,key,... 
//
//void get_position(GpPosition * pPos)
void GpGadgets::GetPosition(GpCommand & rC, GpPosition * pPos)
{
	GetPositionDefault(rC, pPos, first_axes, 3);
}
//
// get_position() - reads a position for label,arrow,key,...
// with given default GpCoordinate system
// ndim = 2 only reads x,y
// otherwise it reads x,y,z
//
//void get_position_default(GpPosition * pos, enum position_type default_type, int ndim)
void GpGadgets::GetPositionDefault(GpCommand & rC, GpPosition * pos, enum position_type default_type, int ndim)
{
	AXIS_INDEX axes;
	enum position_type type = default_type;
	memzero(pos, sizeof(GpPosition));
	get_position_type(rC, &type, &axes);
	pos->SetX(type, GetNumberOrTime(rC, axes, FIRST_X_AXIS));
	if(rC.Eq(",")) {
		++rC.CToken;
		get_position_type(rC, &type, &axes);
		pos->SetY(type, GetNumberOrTime(rC, axes, FIRST_Y_AXIS));
	}
	else {
		pos->SetY(type, 0.0);
	}
	// Resolves ambiguous syntax when trailing comma ends a plot command 
	if(ndim != 2 && rC.Eq(",")) {
		++rC.CToken;
		get_position_type(rC, &type, &axes);
		// HBB 2015-01-28: no secondary Z axis, so patch up if it was selected 
		if(type == second_axes) {
			type = first_axes;
			axes = FIRST_AXES;
		}
		pos->SetZ(type, GetNumberOrTime(rC, axes, FIRST_Z_AXIS));
	}
	else
		pos->SetZ(type, 0.0); // same as y 
}

/*
 * Add a single tic mark, with label, to the list for this axis.
 * To avoid duplications and overprints, sort the list and allow
 * only one label per position.
 * EAM - called from set.c during `set xtics` (level >= 0)
 *       called from datafile.c during `plot using ::xtic()` (level = -1)
 */
void add_tic_user(GpAxis * pAx, char * pLabel, double position, int level)
{
	if(pLabel || level >= 0) {
		ticmark * tic, * newtic;
		ticmark listhead;
		// Mark this axis as user-generated ticmarks only, unless the 
		// mix flag indicates that both user- and auto- tics are OK.  
		if(!pAx->ticdef.def.mix)
			pAx->ticdef.type = TIC_USER;
		// Walk along list to sorted positional order 
		listhead.next = pAx->ticdef.def.user;
		listhead.position = -DBL_MAX;
		for(tic = &listhead; tic->next && (position > tic->next->position); tic = tic->next) {
			;
		}
		if(!tic->next || position < tic->next->position) {
			// Make a new ticmark 
			newtic = (ticmark*)malloc(sizeof(ticmark));
			newtic->position = position;
			// Insert it in the list 
			newtic->next = tic->next;
			tic->next = newtic;
		}
		else {
			// The new tic must duplicate position of tic->next 
			if(position != tic->next->position)
				int_warn(NO_CARET, "add_tic_user: list sort error");
			newtic = tic->next;
			// Don't over-write a major tic with a minor tic 
			if(level == 1)
				return;
			// User-specified tics are preferred to autogenerated tics
			if(level == 0 && newtic->level > 1)
				return;
			// FIXME: But are they preferred to data-generated tics?
			if(newtic->level < level)
				return;
			ZFREE(newtic->label);
		}
		newtic->level = level;
		newtic->label = pLabel ? gp_strdup(pLabel) : 0;
		// Make sure the listhead is kept 
		pAx->ticdef.def.user = listhead.next;
	}
}
/*
 * Degrees/minutes/seconds geographic GpCoordinate format
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
	bool EWflag = false;
	bool NSflag = false;
	char compass = ' ';
	char * c, * cfmt;

	/* Limit the range to +/- 180 degrees */
	if(value > 180.)
		value -= 360.;
	if(value < -180.)
		value += 360.;

	degrees = fabs(value);
	Degrees = floor(degrees);
	minutes = (degrees - (double)Degrees) * 60.;
	Minutes = floor(minutes);
	seconds = (degrees - (double)Degrees) * 3600. -  (double)Minutes*60.;
	Seconds = floor(seconds);

	for(c = cfmt = gp_strdup(format); *c; ) {
		if(*c++ == '%') {
			while(*c && !strchr("DdMmSsEN%", *c))
				c++;
			switch(*c) {
				case 'D':   *c = 'g'; dtype = 1; degrees = Degrees; break;
				case 'd':   *c = 'f'; dtype = 2; break;
				case 'M':   *c = 'g'; mtype = 1; minutes = Minutes; break;
				case 'm':   *c = 'f'; mtype = 2; break;
				case 'S':   *c = 'g'; stype = 1; seconds = Seconds; break;
				case 's':   *c = 'f'; stype = 2; break;
				case 'E':   *c = 'c'; EWflag = true; break;
				case 'N':   *c = 'c'; NSflag = true; break;
				case '%':   GpGg.IntError(GpC, NO_CARET, "unrecognized format: \"%s\"", format);
			}
		}
	}

	/* By convention the minus sign goes only in front of the degrees */
	/* Watch out for round-off errors! */
	if(value < 0 && !EWflag && !NSflag) {
		if(dtype > 0) degrees = -fabs(degrees);
		else if(mtype > 0) minutes = -fabs(minutes);
		else if(stype > 0) seconds = -fabs(seconds);
	}
	if(EWflag)
		compass = (value == 0) ? ' ' : (value < 0) ? 'W' : 'E';
	if(NSflag)
		compass = (value == 0) ? ' ' : (value < 0) ? 'S' : 'N';

	/* This is tricky because we have to deal with the possibility that
	 * the user may not have specified all the possible format components
	 */
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
	free(cfmt);
}

/*
 * EAM Nov 2012
 * This routine used to be macros PARSE_RANGE, PARSE_NAMED_RANGE
 */

/* Accepts a range of the form [MIN:MAX] or [var=MIN:MAX]
 * Returns
 *	 0 = no range spec present
 *	-1 = range spec with no attached variable name
 *	>0 = token indexing the attached variable name
 */
//int parse_range(AXIS_INDEX axis)
int GpAxisBlock::ParseRange(AXIS_INDEX axis, GpCommand & rC)
{
	GpAxis & r_ax = AxA[axis];
	if(rC.Eq("[")) {
		int dummy_token = -1;
		rC.CToken++;
		// If the range starts with "[var=" return the token of the named variable
		if(rC.IsLetter(rC.CToken) && rC.Eq(rC.CToken + 1, "=")) {
			dummy_token = rC.CToken;
			rC.CToken += 2;
		}
		r_ax.AutoScale = r_ax.LoadRange(rC, r_ax.Range, r_ax.AutoScale);
		// EXPERIMENTAL: optional sample interval
		if(axis == SAMPLE_AXIS) {
			r_ax.SAMPLE_INTERVAL = 0;
			if(rC.Eq(":")) {
				rC.CToken++;
				r_ax.SAMPLE_INTERVAL = rC.RealExpression();
			}
		}
		if(!rC.Eq("]"))
			GpGg.IntError(rC, rC.CToken, "']' expected");
		rC.CToken++;
		return dummy_token;
	}
	else
		return 0;
}
//
// Called if an in-line range is encountered while inside a zoom command 
//
//void parse_skip_range()
void GpCommand::ParseSkipRange()
{
	while(!Eq(CToken++, "]"))
		if(EndOfCommand())
			break;
}
//
// When a secondary axis (axis2) is linked to the corresponding primary
// axis (axis1), this routine copies the relevant range/scale data
//
void clone_linked_axes(GpAxis * axis1, GpAxis * axis2)
{
	double testmin, testmax;
	bool   suspect = false;
	// DEBUG: This sanity check is only here for debugging
	if(axis1 != axis2->P_LinkToPrmr && axis1 != axis2->P_LinkToScnd) {
		fprintf(stderr, "clone_linked_axes called for axes that are not linked\n");
		// No linkage, so nothing to do here
	}
	else {
		memcpy(axis2, axis1, AXIS_CLONE_SIZE);
		if(axis2->link_udf && axis2->link_udf->at) {
			// FIXME: In order to handle logscale axes, the code below would have to unlog the
			// axis1 min/max; apply and check the mappings; then re-log and store the values
			// for axis2. And after that the tics still come out wrong.
			if(axis2->Flags & GpAxis::fLog && axis2->link_udf)
				int_warn(NO_CARET, "cannot handle via/inverse linked log-scale axes");
			// Transform the min/max limits of linked secondary axis
			axis2->SetRange.Set(axis2->EvalLinkFunction(axis1->SetRange.low), axis2->EvalLinkFunction(axis1->SetRange.upp));
			axis2->Range.Set(axis2->EvalLinkFunction(axis1->Range.low), axis2->EvalLinkFunction(axis1->Range.upp));
			if(fisnan(axis2->Range.low) || fisnan(axis2->SetRange.low) || fisnan(axis2->Range.upp) || fisnan(axis2->SetRange.upp))
				int_warn(NO_CARET, "axis mapping function must return a real value");
			// Confirm that the inverse mapping actually works, at least at the endpoints
			// FIXME:  Should we test values in between the endpoints also?
			testmin = axis1->EvalLinkFunction(axis2->SetRange.low);
			testmax = axis1->EvalLinkFunction(axis2->SetRange.upp);
			if(fabs(testmin - axis1->SetRange.low) != 0 && fabs((testmin - axis1->SetRange.low) / testmin) > 1.e-6)
				suspect = true;
			if(fabs(testmax - axis1->SetRange.upp) != 0 && fabs((testmax - axis1->SetRange.upp) / testmax) > 1.e-6)
				suspect = true;
			if(suspect) {
				int_warn(NO_CARET, "could not confirm linked axis inverse mapping function");
				fprintf(stderr, "\tmin: %g inv(via(min)): %g",   axis1->SetRange.low, testmin);
				fprintf(stderr, "  max: %g inv(via(max)): %g\n", axis1->SetRange.upp, testmax);
			}
		}
	}
}

void * GpAxisBlock::CreateAxesCopy()
{
	GpAxis * p_copy = (GpAxis *)malloc(sizeof(AxA));
	if(p_copy)
		memcpy(p_copy, AxA, sizeof(AxA));
	return p_copy;
}

void GpAxisBlock::RestoreAxesCopy(const void * pCopy)
{
	if(pCopy)
		memcpy(AxA, pCopy, sizeof(AxA));
}

void GpAxisBlock::DestroyAxesCopy(void * pCopy)
{
	free(pCopy);
}

double GpGadgets::GetNumberOrTime(GpCommand & rC, AXIS_INDEX baseAxIdx, AXIS_INDEX axIdx)
{
	GpAxis * p_ax = (baseAxIdx == NO_AXIS) ? 0 : (AxA+baseAxIdx+axIdx);
	return GetNumOrTime(rC, p_ax);
}

void GpAxis::UnlogInterval(RealRange & rR, bool checkrange)
{
	if(Flags & GpAxis::fLog) {
		if(checkrange && (rR.low <= 0.0 || rR.upp <= 0.0))
			GpGg.IntError(GpC, NO_CARET, "%s range must be greater than 0 for log scale", GpGg.GetAxisName(Index));
		rR.low = (rR.low <= 0.0) ? -GPVL : DoLog(rR.low);
		rR.upp = (rR.upp <= 0.0) ? -GPVL : DoLog(rR.upp);
	}
}
//
// Evaluate the function linking secondary axis to primary axis
//
//double eval_link_function(GpAxis * axis, double raw_coord)
double GpAxis::EvalLinkFunction(double raw_coord) const
{
	t_value a;
	int    dummy_var = (abs(Index) == FIRST_Y_AXIS || abs(Index) == SECOND_Y_AXIS) ? 1 : 0;
	link_udf->dummy_values[1-dummy_var].type = INVALID_NAME;
	link_udf->dummy_values[dummy_var].SetComplex(raw_coord, 0.0);
	GpGg.Ev.EvaluateAt(link_udf->at, &a);
	if(GpGg.Ev.undefined || a.type != CMPLX) {
		FPRINTF((stderr, "EvalLinkFunction(%g) returned %s\n", raw_coord, undefined ? "undefined" : "unexpected type"));
		a = GpGg.Ev.udv_NaN->udv_value;
	}
	if(fisnan(a.v.cmplx_val.real))
		GpGg.Ev.undefined = true;
	return a.v.cmplx_val.real;
}

#ifdef NONLINEAR_AXES
//
// Obtain and initialize a shadow axis.
// The details are hidden from the rest of the code (dynamic/static allocation, etc).
//
GpAxis * get_shadow_axis(GpAxis * pAx)
{
	GpAxis * primary = NULL;
	GpAxis * secondary = pAx;
	int i;
	/* This implementation uses a dynamically allocated array of shadow pAx	*/
	/* structures that is allocated on first use and reused after that.     */
	if(!shadow_axis_array) {
		shadow_axis_array = (GpAxis *)malloc(NUMBER_OF_MAIN_VISIBLE_AXES * sizeof(GpAxis));
		for(i = 0; i<NUMBER_OF_MAIN_VISIBLE_AXES; i++) {
			//memcpy(&shadow_axis_array[i], &default_axis_state, sizeof(GpAxis));
			shadow_axis_array[i].SetDefault();
		}
	}
	if(pAx->Index != SAMPLE_AXIS && pAx->Index < NUMBER_OF_MAIN_VISIBLE_AXES)
		primary = &shadow_axis_array[pAx->Index];
	else
		GpGg.IntError(GpC, NO_CARET, "invalid shadow pAx");
	primary->Index = -secondary->Index;
	return primary;
}

#endif
//
// Check for linked-axis GpCoordinate transformation given by command
//    set {x|y}2r link via <expr1> inverse <expr2>
// If we are plotting on the secondary axis in this case, apply the inverse
// transform to get back to the primary GpCoordinate system before mapping.
//
//int map_x(double value)
int GpGadgets::MapX(double value)
{
	const GpAxis * p_primary = GetX().P_LinkToPrmr;
	if(p_primary && p_primary->link_udf->at) {
		value = p_primary->EvalLinkFunction(value);
		return p_primary->Map(value);
	}
	else 
		return Map(XAxis, value);
}

//int map_y(double value)
int GpGadgets::MapY(double value)
{
	const GpAxis * p_primary = GetY().P_LinkToPrmr;
	if(p_primary && p_primary->link_udf->at) {
		value = p_primary->EvalLinkFunction(value);
		return p_primary->Map(value);
	}
	else
		return Map(YAxis, value);
}
//
// Utility routine to propagate rrange into corresponding x and y ranges
//
void GpGadgets::RRangeToXY()
{
	const double _min = (GetR().SetAutoScale & AUTOSCALE_MIN) ? 0.0 : GetR().SetRange.low;
	if(GetR().SetAutoScale & AUTOSCALE_MAX) {
		GetX().SetAutoScale = AUTOSCALE_BOTH;
		GetY().SetAutoScale = AUTOSCALE_BOTH;
	}
	else {
		GetX().SetAutoScale = AUTOSCALE_NONE;
		GetY().SetAutoScale = AUTOSCALE_NONE;
		if(GetR().Flags & GpAxis::fLog)
			GetX().SetRange.upp = DoLog(POLAR_AXIS, GetR().SetRange.upp) - DoLog(POLAR_AXIS, _min);
#ifdef NONLINEAR_AXES
		else if(GetR().P_LinkToPrmr)
			GetX().SetRange.upp = GetR().P_LinkToPrmr->EvalLinkFunction(GetR().SetRange.upp) - GetR().P_LinkToPrmr->Range.low;
#endif
		else
			GetX().SetRange.upp = GetR().SetRange.upp - _min;
		GetY().SetRange.upp = GetX().SetRange.upp;
		GetY().SetRange.low = GetX().SetRange.low = -GetX().SetRange.upp;
	}
}
