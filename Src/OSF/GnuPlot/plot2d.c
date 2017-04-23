/* GNUPLOT - plot2d.c */

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
#include <gnuplot.h>
#pragma hdrstop

#define MIN_CRV_POINTS 100 // minimum size of points[] in CurvePoints

/* static prototypes */

static CurvePoints * cp_alloc(int num);
static void impulse_range_fiddling(CurvePoints* plot);
static int check_or_add_boxplot_factor(CurvePoints * plot, char* string, double x);
//
// internal and external variables
//
// the curves/surfaces of the plot
//CurvePoints * first_plot = NULL;
//
// function implementations
//
/*
 * cp_alloc() allocates a CurvePoints structure that can hold 'num'
 * points.   Initialize all fields to NULL.
 */
static CurvePoints * cp_alloc(int num)
{
	lp_style_type default_lp_properties; // = DEFAULT_LP_STYLE_TYPE;
	CurvePoints * cp = (CurvePoints*)malloc(sizeof(CurvePoints));
	memzero(cp, sizeof(CurvePoints));
	cp->p_max = (num >= 0 ? num : 0);
	if(num > 0)
		cp->points = (GpCoordinate *)malloc(num * sizeof(GpCoordinate));
	// Initialize various fields 
	cp->lp_properties = default_lp_properties;
	default_arrow_style(&(cp->arrow_properties));
	cp->fill_properties = GpGg.DefaultFillStyle;
	return (cp);
}
//
// cp_extend() reallocates a CurvePoints structure to hold "num"
// points. This will either expand or shrink the storage.
//
void cp_extend(CurvePoints * cp, int num)
{
	if(num != cp->p_max) {
		if(num > 0) {
			if(cp->points == NULL) {
				cp->points = (GpCoordinate *)malloc(num * sizeof(cp->points[0]));
			}
			else {
				cp->points = (GpCoordinate *)gp_realloc(cp->points, num * sizeof(cp->points[0]), "expanding curve points");
				if(cp->varcolor)
					cp->varcolor = (double *)gp_realloc(cp->varcolor, num * sizeof(double), "expanding curve variable colors");
				if(cp->z_n) {
					for(int i = 0; i < cp->n_par_axes; i++)
						cp->z_n[i] = (double *)gp_realloc(cp->z_n[i], num * sizeof(double), "expanding curve z_n[i]");
				}
			}
			cp->p_max = num;
		}
		else {
			ZFREE(cp->points);
			cp->p_max = 0;
			ZFREE(cp->varcolor);
			if(cp->z_n) {
				for(int i = 0; i < cp->n_par_axes; i++)
					free(cp->z_n[i]);
				ZFREE(cp->z_n);
				cp->n_par_axes = 0;
			}
		}
	}
}
//
// cp_free() releases any memory which was previously malloc()'d to hold
// curve points (and recursively down the linked list).
//
void cp_free(CurvePoints * cp)
{
	while(cp) {
		CurvePoints * next = cp->P_Next;
		ZFREE(cp->title);
		ZFREE(cp->title_position);
		ZFREE(cp->points);
		ZFREE(cp->varcolor);
		GpTextLabel::Destroy(cp->labels);
		cp->labels = 0;
		if(cp->z_n) {
			for(int i = 0; i < cp->n_par_axes; i++)
				free(cp->z_n[i]);
			ZFREE(cp->z_n);
			cp->n_par_axes = 0;
		}
		free(cp);
		cp = next;
	}
}
//
// In the parametric case we can say plot [a= -4:4] [-2:2] [-1:1] sin(a),a**2
// while in the non-parametric case we would say only plot [b= -2:2] [-1:1] sin(b)
//
//void plotrequest()
void GpGadgets::PlotRequest(GpCommand & rC)
{
	int dummy_token = 0;
	/*AXIS_INDEX*/ int t_axis;
	if(!term) // unknown 
		IntErrorCurToken("use 'set term' to set terminal type first");
	Is3DPlot = false;
	// Deactivate if 'set view map' is still running after the previous 'splot': 
	// EAM Jan 2012 - this should no longer be necessary, but it doesn't hurt.
	SplotMapDeactivate(rC);
	if(IsParametric && strcmp(rC.P.SetDummyVar[0], "u") == 0)
		strcpy(rC.P.SetDummyVar[0], "t");
	// initialise the arrays from the 'set' scalars 
	InitAxis(FIRST_X_AXIS, 0);
	InitAxis(FIRST_Y_AXIS, 1);
	InitAxis(SECOND_X_AXIS, 0);
	InitAxis(SECOND_Y_AXIS, 1);
	InitAxis(T_AXIS, 0);
	InitAxis(POLAR_AXIS, 1);
	InitAxis(COLOR_AXIS, 1);
#ifdef NONLINEAR_AXES
	// Nonlinear mapping of x or y via linkage to a hidden primary axis. 
	// The user set autoscale for the visible axis; apply it also to the hidden axis. 
	for(t_axis = FIRST_AXES; t_axis < NUMBER_OF_MAIN_VISIBLE_AXES; t_axis++) {
		if(t_axis != SAMPLE_AXIS) {
			GpAxis & r_ax_secondary = AxA[t_axis];
			GpAxis * p_ax_primary = r_ax_secondary.P_LinkToPrmr;
			if(p_ax_primary && p_ax_primary->Index == -t_axis) {
				p_ax_primary->SetAutoScale = r_ax_secondary.SetAutoScale;
				p_ax_primary->Init(1);
			}
		}
	}
#endif
	// If we are called from a mouse zoom operation we should ignore
	// any range limits because otherwise the zoom won't zoom.		
	if(inside_zoom) {
		while(rC.Eq("["))
			rC.ParseSkipRange();
	}
	// Range limits for the entire plot are optional but must be given	
	// in a fixed order. The keyword 'sample' terminates range parsing.	
	if(IsParametric || IsPolar) {
		dummy_token = ParseRange(T_AXIS, rC);
		ParseRange(FIRST_X_AXIS, rC);
	}
	else {
		dummy_token = ParseRange(FIRST_X_AXIS, rC);
	}
	ParseRange(FIRST_Y_AXIS, rC);
	ParseRange(SECOND_X_AXIS, rC);
	ParseRange(SECOND_Y_AXIS, rC);
	if(rC.Eq("sample") && rC.Eq(rC.CToken+1, "["))
		rC.CToken++;
	// Clear out any tick labels read from data files in previous plot 
	for(t_axis = FIRST_AXES; t_axis < AXIS_ARRAY_SIZE; t_axis++) {
		t_ticdef * ticdef = &AxA[t_axis].ticdef;
		if(ticdef->def.user)
			ticdef->def.user = prune_dataticks(ticdef->def.user);
		if(!ticdef->def.user && ticdef->type == TIC_USER)
			ticdef->type = TIC_COMPUTED;
	}
	// use the default dummy variable unless changed 
	if(dummy_token > 0)
		rC.CopyStr(rC.P.CDummyVar[0], dummy_token, MAX_ID_LEN);
	else
		strcpy(rC.P.CDummyVar[0], rC.P.SetDummyVar[0]);
	EvalPlots(rC);
}
//
// Helper function for refresh command.  Reexamine each data point and update the
// flags for INRANGE/OUTRANGE/UNDEFINED based on the current limits for that axis.
// Normally the axis limits are already known at this point. But if the user has
// forced "set autoscale" since the previous plot or refresh, we need to reset the
// axis limits and try to approximate the full auto-scaling behaviour.
//
//void refresh_bounds(CurvePoints * first_plot, int nplots)
void GpGadgets::RefreshBounds(GpTermEntry * pT, CurvePoints * pFirstPlot, int nplots)
{
	CurvePoints * this_plot = pFirstPlot;
	int iplot;      /* plot index */
	for(iplot = 0; iplot < nplots; iplot++, this_plot = this_plot->P_Next) {
		int i;  /* point index */
		GpAxis * x_axis = &AxA[this_plot->x_axis];
		GpAxis * y_axis = &AxA[this_plot->y_axis];
		// IMAGE clipping is done elsewhere, so we don't need INRANGE/OUTRANGE checks
		if(oneof2(this_plot->plot_style, IMAGE, RGBIMAGE)) {
			if(x_axis->SetAutoScale || y_axis->SetAutoScale)
				ProcessImage(pT, this_plot, IMG_UPDATE_AXES);
		}
		else {
			// FIXME: I'm not entirely convinced this test does what the comment says.
			//
			// If the state has been set to autoscale since the last plot,
			// mark everything INRANGE and re-evaluate the axis limits now.
			// Otherwise test INRANGE/OUTRANGE against previous data limits.
			//
			if(!this_plot->noautoscale) {
				if(x_axis->SetAutoScale & AUTOSCALE_MIN && x_axis->DataRange.low < x_axis->Range.low)
					x_axis->Range.low = x_axis->LogValue(x_axis->DataRange.low);
				if(x_axis->SetAutoScale & AUTOSCALE_MAX && x_axis->DataRange.upp > x_axis->Range.upp)
					x_axis->Range.upp = x_axis->LogValue(x_axis->DataRange.upp);
			}
			for(i = 0; i<this_plot->p_count; i++) {
				GpCoordinate  * point = &this_plot->points[i];
				if(point->type != UNDEFINED) {
					point->type = INRANGE;
					// This autoscaling logic is identical to that in refresh_3dbounds() in plot3d.c
					if(!this_plot->noautoscale)
						x_axis->AdjustAutoscale(point->x);
					if(!x_axis->InRange(point->x))
						point->type = OUTRANGE;
					else {
						if(!this_plot->noautoscale)
							y_axis->AdjustAutoscale(point->y);
						if(!y_axis->InRange(point->y))
							point->type = OUTRANGE;
					}
				}
			}
		}
	}
	this_plot = pFirstPlot;
	for(iplot = 0; iplot < nplots; iplot++, this_plot = this_plot->P_Next) {
		// handle 'reverse' ranges
		RevertRange((AXIS_INDEX)this_plot->x_axis);
		RevertRange((AXIS_INDEX)this_plot->y_axis);
		// Make sure the bounds are reasonable, and tweak them if they aren't
		AxisCheckedExtendEmptyRange((AXIS_INDEX)this_plot->x_axis, NULL);
		AxisCheckedExtendEmptyRange((AXIS_INDEX)this_plot->y_axis, NULL);
	}
}
//
// A quick note about boxes style. For boxwidth auto, we cannot
// calculate widths yet, since it may be sorted, etc. But if
// width is set, we must do it now, before logs of xmin/xmax are taken.
// We store -1 in point->z as a marker to mean width needs to be
// calculated, or 0 to mean that xmin/xmax are set correctly
// 
// current_plot->token is after datafile spec, for error reporting
// it will later be moved passed title/with/linetype/pointtype
// 
//static int get_data(CurvePoints * pPlot)
int GpGadgets::GetData(GpCommand & rC, CurvePoints * pPlot)
{
	int i /* num. points ! */, j;
	int ngood;
	IntRange cols_range; // allowed range of column numbers
	int    storetoken = pPlot->Token;
	GpCoordinate  * cp;
	double v[MAXDATACOLS];
	memzero(v, sizeof(v));
	if(!pPlot->varcolor) {
		bool variable_color = false;
		if((pPlot->lp_properties.pm3d_color.type == TC_RGB) && (pPlot->lp_properties.pm3d_color.value < 0))
			variable_color = true;
		if(pPlot->lp_properties.pm3d_color.type == TC_Z)
			variable_color = true;
		if(pPlot->lp_properties.l_type == LT_COLORFROMCOLUMN)
			variable_color = true;
		if(pPlot->plot_smooth != SMOOTH_NONE) {
			/* FIXME:  It would be possible to support smooth cspline lc palette */
			/* but it would require expanding and interpolating plot->varcolor   */
			/* in parallel with the y values.                                    */
			variable_color = false;
		}
		if(variable_color)
			pPlot->varcolor = (double *)malloc(pPlot->p_max * sizeof(double));
		if(variable_color && pPlot->plot_style == PARALLELPLOT) {
			free(pPlot->z_n[--(pPlot->n_par_axes)]); // Oops, we reserved one column of data too many 
		}
	}
	/* eval_plots has already opened file */

	/* HBB 2000504: For most 2D plot styles the 'z' GpCoordinate is unused.
	 * Set it to NO_AXIS to account for that. For styles that use
	 * the z GpCoordinate as a real GpCoordinate (i.e. not a width or
	 * 'delta' component, change the setting inside the switch: */
	pPlot->z_axis = NO_AXIS;

	/* HBB NEW 20060427: if there's only one, explicit using column,
	 * it's y data.  df_axis[] has to reflect that, so df_readline()
	 * will expect time/date input. */
	if(GpDf.df_no_use_specs == 1)
		GpDf.df_axis[0] = GpDf.df_axis[1];
	switch(pPlot->plot_style) { /* set maximum columns to scan */
		case XYERRORLINES:
		case XYERRORBARS:
		case BOXXYERROR:
			cols_range.Set(4, 7);
		    if(GpDf.df_no_use_specs >= 6) {
			    // HBB 20060427: signal 3rd and 4th column are absolute x data --- needed so time/date parsing works
			    GpDf.df_axis[2] = GpDf.df_axis[3] = GpDf.df_axis[0];
			    // and 5th and 6th are absolute y data
			    GpDf.df_axis[4] = GpDf.df_axis[5] = GpDf.df_axis[1];
		    }
		    break;
		case FINANCEBARS:
		    // HBB 20000504: use 'z' GpCoordinate for y-axis quantity
		    pPlot->z_axis = pPlot->y_axis;
			cols_range.Set(5, 6);
		    // HBB 20060427: signal 3rd and 4th column are absolute y data needed so time/date parsing works
		    GpDf.df_axis[2] = GpDf.df_axis[3] = GpDf.df_axis[4] = GpDf.df_axis[1];
		    break;
		case BOXPLOT:
			cols_range.Set(2, 4); // fixed x, lots of y data points - optional width, optional factor 
		    GpDf.ExpectString(4);
		    break;
		case CANDLESTICKS:
		    pPlot->z_axis = pPlot->y_axis;
			cols_range.Set(5, 7);
		    GpDf.df_axis[2] = GpDf.df_axis[3] = GpDf.df_axis[4] = GpDf.df_axis[1];
		    break;
		case BOXERROR:
			cols_range.Set(3, 6);
		    /* There are four possible cases: */
		    /* 3 cols --> (x,y,dy), auto dx */
		    /* 4 cols, boxwidth==-2 --> (x,y,ylow,yhigh), auto dx */
		    /* 4 cols, boxwidth!=-2 --> (x,y,dy,dx) */
		    /* 5 cols --> (x,y,ylow,yhigh,dx) */
		    /* In each case an additional column may hold variable color */
		    if((GpDf.df_no_use_specs == 4 && boxwidth == -2) || GpDf.df_no_use_specs >= 5)
			    // HBB 20060427: signal 3rd and 4th column are absolute y data --- needed so time/date parsing works
			    GpDf.df_axis[2] = GpDf.df_axis[3] = GpDf.df_axis[1];
		    break;
		case VECTOR: /* x, y, dx, dy, variable color or arrow style */
			cols_range.Set(4, 5);
		    break;
		case XERRORLINES:
		case XERRORBARS:
			cols_range.Set(3, 5);
		    if(GpDf.df_no_use_specs >= 4)
			    // HBB 20060427: signal 3rd and 4th column are absolute x data --- needed so time/date parsing works
			    GpDf.df_axis[2] = GpDf.df_axis[3] = GpDf.df_axis[0];
		    break;
		case YERRORLINES:
		case YERRORBARS:
			cols_range.Set(2, 5);
		    if(GpDf.df_no_use_specs >= 4)
			    // HBB 20060427: signal 3rd and 4th column are absolute y data --- needed so time/date parsing works
			    GpDf.df_axis[2] = GpDf.df_axis[3] = GpDf.df_axis[1];
		    break;
		case HISTOGRAMS:
			cols_range.Set(1, 3);
		    break;
		case BOXES:
			cols_range.Set(1, 4);
		    break;
		case FILLEDCURVES:
			cols_range.Set(1, 3);
		    GpDf.df_axis[2] = GpDf.df_axis[1]; /* Both curves use same y axis */
		    break;
		case IMPULSES: /* 2 + possible variable color */
		case LINES:
		case DOTS:
			cols_range.Set(1, 3);
		    break;
		case LABELPOINTS:
		    // 3 column data: X Y Label 
		    // 4th column allows rgb variable or pointsize variable 
			cols_range.Set(3, 4);
		    GpDf.ExpectString(3);
		    break;
		case IMAGE:
			cols_range.Set(3, 3);
		    break;
		case RGBIMAGE:
			cols_range.Set(5, 6);
		    break;
		case RGBA_IMAGE:
			cols_range.Set(6, 6);
		    break;
#ifdef EAM_OBJECTS
		case CIRCLES: /* 3 + possible variable color, or 5 + possible variable color */
			cols_range.Set(2, 6);
		    break;
		case ELLIPSES:
			cols_range.Set(2, 6); // x, y, major axis, minor axis - + optional angle, possible variable color
		    break;
#endif
		case POINTSTYLE:
		case LINESPOINTS:
		    /* 1 column: y GpCoordinate only */
		    /* 2 columns x and y coordinates */
		    /* Allow 1 extra column because of 'pointsize variable' */
		    /* Allow 1 extra column because of 'pointtype variable' */
		    /* Allow 1 extra column because of 'lc rgb variable'    */
			cols_range.Set(1, 5);
		    break;
		case PARALLELPLOT:
		    // Maximum number of parallel axes is fixed at compile time 
		    if(pPlot->n_par_axes > (int)NumParallelAxes)
			    ExtendParallelAxis(pPlot->n_par_axes);
		    // First N columns are data; one more is optional varcolor 
			cols_range.Set(pPlot->n_par_axes, pPlot->n_par_axes + 1);
		    // We have not yet read in any data, so we cannot do complete initialization 
		    for(j = 0; j < pPlot->n_par_axes; j++) {
			    P_ParallelAxis[j].Init(1);
		    }
		    break;
		case TABLESTYLE:
			cols_range.Set(1, MAXDATACOLS);
		    break;
		default:
			cols_range.Set(1, 2);
		    break;
	}
	// Restictions on plots with "smooth" option 
	switch(pPlot->plot_smooth) {
		case SMOOTH_NONE:
		    break;
		case SMOOTH_ACSPLINES:
			cols_range.upp = 3;
		    pPlot->z_axis = FIRST_Z_AXIS;
		    GpDf.df_axis[2] = FIRST_Z_AXIS;
		    break;
		default:
		    if(GpDf.df_no_use_specs > 2)
			    int_warn(NO_CARET, "extra columns ignored by smoothing option");
		    break;
	}
	// EXPERIMENTAL May 2013 - Treating timedata columns as strings allows 
	// functions column(N) and column("HEADER") to work on time data.	   
	// Sep 2014: But the column count is wrong for HISTOGRAMS 
	if(pPlot->plot_style != HISTOGRAMS) {
		if(AxA[pPlot->x_axis].datatype == DT_TIMEDATE)
			GpDf.ExpectString(1);
		if(AxA[pPlot->y_axis].datatype == DT_TIMEDATE)
			GpDf.ExpectString(2);
	}
	if(GpDf.df_no_use_specs > cols_range.upp) 
		GpGg.IntErrorNoCaret("Too many using specs for this style");
	if(GpDf.df_no_use_specs > 0 && GpDf.df_no_use_specs < cols_range.low)
		GpGg.IntErrorNoCaret("Not enough columns for this style");
	i = 0;
	ngood = 0;
	// If the user has set an explicit locale for numeric input, apply it 
	// here so that it affects data fields read from the input file.      
	set_numeric_locale();
	// Initial state 
	GpDf.df_warn_on_missing_columnheader = true;
	while((j = GpDf.DfReadLine(v, cols_range.upp)) != DF_EOF) {
		if(i >= pPlot->p_max) {
			/* overflow about to occur. Extend size of points[]
			 * array. Double the size, and add 1000 points, to avoid
			 * needlessly small steps. */
			cp_extend(pPlot, i + i + 1000);
		}
		/* Version 5	DEBUG DEBUG
		 * We are now trying to pass back all available info even if one of the requested
		 * columns was missing or undefined.  This check replaces the DF_UNDEFINED case in
		 * the main switch statement below.
		 */
		if(j == DF_UNDEFINED) {
			pPlot->points[i].type = UNDEFINED;
			j = GpDf.df_no_use_specs;
		}
		if(j > 0) {
			ngood++;
			/* June 2010 - New mechanism for variable color                  */
			/* If variable color is requested, take the color value from the */
			/* final column of input and decrement the column count by one.  */
			if(pPlot->varcolor) {
				static char * errmsg = "Not enough columns for variable color";
				switch(pPlot->plot_style) {
					case CANDLESTICKS:
					case FINANCEBARS:
					    if(j < 6) 
							GpGg.IntErrorNoCaret(errmsg);
					    break;
					case XYERRORLINES:
					case XYERRORBARS:
					case BOXXYERROR:
					    if(j != 7 && j != 5) 
							GpGg.IntErrorNoCaret(errmsg);
					    break;
					case VECTOR:
					    if(j < 5) 
							GpGg.IntErrorNoCaret(errmsg);
					    break;
					case LABELPOINTS:
					case BOXERROR:
					case XERRORLINES:
					case XERRORBARS:
					case YERRORLINES:
					case YERRORBARS:
					    if(j < 4) 
							GpGg.IntErrorNoCaret(errmsg);
					    break;
#ifdef EAM_OBJECTS
					case CIRCLES:
					    if(j == 5 || j < 3) 
							GpGg.IntErrorNoCaret(errmsg);
					    break;
					case ELLIPSES:
#endif
					case BOXES:
					case POINTSTYLE:
					case LINESPOINTS:
					case IMPULSES:
					case LINES:
					case DOTS:
					    if(j < 3) 
							GpGg.IntErrorNoCaret(errmsg);
					    break;
					case PARALLELPLOT:
					    if(j < 4) 
							GpGg.IntErrorNoCaret(errmsg);
					    break;
					case BOXPLOT:
					    v[j++] = pPlot->base_linetype + 1; // Only the key sample uses this value
					    break;
					default:
					    break;
				}
				pPlot->varcolor[i] = v[--j];
			}
			if(pPlot->plot_style == TABLESTYLE) {
				// Echo the values directly to the output file. FIXME: formats?
				int    col;
				int    dummy_type = INRANGE;
				FILE * outfile = NZOR(table_outfile, gpoutfile);
				if(table_var == NULL) {
					for(col = 0; col < j; col++)
						fprintf(outfile, " %g", v[col]);
					fprintf(outfile, "\n");
				}
				else {
					char   buf[64]; // buffer large enough to hold %g + 2 extra chars
					size_t size = sizeof(buf);
					char * line = (char*)malloc(size);
					size_t len = 0;
					line[0] = NUL;
					for(col = 0; col < j; col++) {
						snprintf(buf, sizeof(buf), " %g", v[col]);
						len = strappend(&line, &size, len, buf);
					}
					append_to_datablock(&table_var->udv_value, line);
				}
				// This tracks x range and avoids "invalid x range" error message
				STORE_WITH_LOG_AND_UPDATE_RANGE(pPlot->points[i].x, v[0], dummy_type, pPlot->x_axis, pPlot->noautoscale, NOOP, NOOP);
				continue;
			}
		}
		/* TODO: It would make more sense to organize the switch below by plot	*/
		/* type rather than by number of columns in use.  The mis-organization  */
		/* is particularly evident for parallel axis plots, to the point where	*/
		/* I decided the only reasonable option is to handle it separately.	*/
		if(pPlot->plot_style == PARALLELPLOT && j > 0) {
			int iaxis;
			/* FIXME: this apparently cannot trigger.  Good or bad? */
			if(j != pPlot->n_par_axes)
				fprintf(stderr, "Expecting %d input columns, got %d\n", pPlot->n_par_axes, j);
			/* Primary GpCoordinate structure holds only x range and 1st y value.	*/
			/* The x range brackets the parallel axes by 0.5 on either side.	*/
			Store2DPoint(pPlot, i, 1.0, v[0], 0.5, (double)(pPlot->n_par_axes)+0.5, v[0], v[0], 0.0);
			/* The parallel axis data is stored in separate arrays */
			for(iaxis = 0; iaxis < pPlot->n_par_axes; iaxis++) {
				int dummy_type = INRANGE;
				ACTUAL_STORE_WITH_LOG_AND_UPDATE_RANGE(pPlot->z_n[iaxis][i], v[iaxis], dummy_type, &P_ParallelAxis[iaxis], pPlot->noautoscale, NOOP, NOOP, 0);
			}
			i++;
		}
		else {
			// This "else" block currently handles all plot styles other than PARALLEL_AXES 
			switch(j) {
				default:
			    {
				    GpDf.DfClose();
				    IntErrorCurToken("internal error : df_readline returned %d : datafile line %d", j, GpDf.df_line_number);
			    }
				case DF_MISSING:
				    /* Plot type specific handling of missing points goes here. */
				    if(pPlot->plot_style == HISTOGRAMS) {
					    pPlot->points[i].type = UNDEFINED;
					    i++;
					    continue;
				    }
				    // Jun 2006 - Return to behavior of 3.7 and current docs:
				    //            do not interrupt plotted line because of missing data
				    FPRINTF((stderr, "Missing datum %d\n", i));
				    continue;
				case DF_UNDEFINED:
				    // NaN or bad result from extended using expression 
				    // Version 5:
				    // FIXME - can't actually get here because we trapped DF_UNDEFINED above
				    pPlot->points[i].type = UNDEFINED;
				    FPRINTF((stderr, "undefined point %g %g %g\n", v[0], v[1], v[2]));
				    if(pPlot->plot_style == IMAGE)
					    goto images;
				    i++;
				    continue;
				case DF_FIRST_BLANK:
				    /* The binary input routines generate DF_FIRST_BLANK at the end
				     * of scan lines, so that the data may be used for the isometric
				     * splots.  Rather than turning that off inside the binary
				     * reading routine based upon the plot mode, DF_FIRST_BLANK is
				     * ignored for certain plot types requiring 3D coordinates in
				     * MODE_PLOT.
				     */
				    if(oneof3(pPlot->plot_style, IMAGE, RGBIMAGE, RGBA_IMAGE))
					    continue;
				    // make type of next point undefined, but recognizable
				    pPlot->points[i] = GpDf.blank_data_line;
				    i++;
				    continue;
				case DF_SECOND_BLANK:
				    // second blank line. We dont do anything (we did everything when we got FIRST one)
				    continue;
				case DF_FOUND_KEY_TITLE:
				    GpDf.DfSetKeyTitle(pPlot);
				    continue;
				case DF_KEY_TITLE_MISSING:
				    fprintf(stderr, "get_data: key title not found in requested column\n");
				    continue;
				case DF_COLUMN_HEADERS:
				    continue;
				case 0: // not blank line, but df_readline couldn't parse it
			    {
				    GpDf.DfClose();
				    GpGg.IntError(pPlot->Token, "Bad data on line %d of file %s", GpDf.df_line_number, NZOR(GpDf.df_filename, ""));
			    }
				case 1:
				    /* only one number */
				    if(default_smooth_weight(pPlot->plot_smooth)) {
					    v[1] = 1.0;
				    }
				    else {
					    /* x is index, assign number to y */
					    v[1] = v[0];
					    v[0] = GpDf.df_datum;
					    /* nobreak */
				    }
				case 2:
H_ERR_BARS:
				    if(pPlot->plot_style == HISTOGRAMS) {
					    if(histogram_opts.type == HT_ERRORBARS) {
						    // The code is a tangle, but we can get here with j = 1, 2, or 3
						    if(j == 1)
							    IntErrorCurToken("Not enough columns in using specification");
						    else if(j == 2) {
							    v[3] = v[0] + v[1];
							    v[2] = v[0] - v[1];
						    }
						    else {
							    v[3] = v[2];
							    v[2] = v[1];
						    }
						    v[1] = v[0];
						    v[0] = GpDf.df_datum;
					    }
					    else if(j >= 2)
						    IntErrorCurToken("Too many columns in using specification");
					    else
						    v[2] = v[3] = v[1];
					    if(histogram_opts.type == HT_STACKED_IN_TOWERS) {
						    histogram_rightmost = pPlot->histogram_sequence + pPlot->histogram->start;
						    pPlot->histogram->end = histogram_rightmost;
					    }
					    else if(v[0] + pPlot->histogram->start > histogram_rightmost) {
						    histogram_rightmost = v[0] + pPlot->histogram->start;
						    pPlot->histogram->end = histogram_rightmost;
					    }
					    // Histogram boxwidths are always absolute
					    if(boxwidth > 0)
						    Store2DPoint(pPlot, i++, v[0], v[1], v[0] - boxwidth / 2, v[0] + boxwidth / 2, v[2], v[3], 0.0);
					    else
						    Store2DPoint(pPlot, i++, v[0], v[1], v[0] - 0.5, v[0] + 0.5, v[2], v[3], 0.0);
					    /* x, y */
					    /* ylow and yhigh are same as y */
				    }
				    else if( (pPlot->plot_style == BOXES) && boxwidth > 0 && boxwidth_is_absolute) {
					    /* calculate width now */
					    if(AxA[pPlot->x_axis].Flags & GpAxis::fLog) {
						    double base = AxA[pPlot->x_axis].base;
						    Store2DPoint(pPlot, i++, v[0], v[1], v[0] * pow(base, -boxwidth/2.), v[0] * pow(base, boxwidth/2.), v[1], v[1], 0.0);
					    }
					    else
						    Store2DPoint(pPlot, i++, v[0], v[1], v[0] - boxwidth / 2, v[0] + boxwidth / 2, v[1], v[1], 0.0);

#ifdef EAM_OBJECTS
				    }
				    else if(pPlot->plot_style == CIRCLES) {
					    /* x, y, default radius, full circle */
					    Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], 0., 360., DEFAULT_RADIUS);
				    }
				    else if(pPlot->plot_style == ELLIPSES) {
					    /* x, y, major axis = minor axis = default, default orientation */
					    Store2DPoint(pPlot, i++, v[0], v[1], 0.0, 0.0, 0.0, 0.0, DEFAULT_ELLIPSE);
#endif
				    }
				    else if(pPlot->plot_style == YERRORBARS) {
					    /* x is index, assign number to y */
					    v[2] = v[1];
					    v[1] = v[0];
					    v[0] = GpDf.df_datum;
					    Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[1] - v[2], v[1] + v[2], -1.0);
				    }
				    else if(pPlot->plot_style == BOXPLOT) {
					    Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[1], v[1],
					    DEFAULT_BOXPLOT_FACTOR);
				    }
				    else if(pPlot->plot_style == FILLEDCURVES) {
					    v[2] = pPlot->filledcurves_options.at;
					    Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[1], v[2], -1.0);
				    }
				    else {
					    double w;
					    if(oneof2(pPlot->plot_style, CANDLESTICKS, FINANCEBARS)) {
						    int_warn(storetoken, "This plot style does not work with 1 or 2 cols. Setting to points");
						    pPlot->plot_style = POINTSTYLE;
					    }
						w = (pPlot->plot_smooth == SMOOTH_ACSPLINES) ? 1.0/* Unit weights */ : -1.0/* Auto-width boxes in some styles */;
					    /* Set x/y high/low to exactly [x,y] */
					    Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[1], v[1], w);
				    }
				    break;

				case 3:
				    /* x, y, ydelta OR x, y, xdelta OR x, y, width */
				    if(pPlot->plot_smooth == SMOOTH_ACSPLINES)
					    Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[1], v[1], v[2]);
				    else
					    switch(pPlot->plot_style) {
						    case HISTOGRAMS:
								if(histogram_opts.type == HT_ERRORBARS)
									goto H_ERR_BARS;
								else
								// fall through 
								default:
								    int_warn(storetoken, "This plot style does not work with 3 cols. Setting to yerrorbars");
									pPlot->plot_style = YERRORBARS;
						    /* fall through */
						    case FILLEDCURVES:
								pPlot->filledcurves_options.closeto = FILLEDCURVES_BETWEEN;
								Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[1], v[2], -1.0);
								break;
						    case YERRORLINES:
						    case YERRORBARS:
						    case BOXERROR: /* x, y, dy */
								/* auto width if boxes, else ignored */
								Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[1] - v[2], v[1] + v[2], -1.0);
								break;
						    case XERRORLINES:
						    case XERRORBARS:
								Store2DPoint(pPlot, i++, v[0], v[1], v[0] - v[2], v[0] + v[2], v[1], v[1], 0.0);
								break;
						    case BOXES:
							/* calculate xmin and xmax here, so that logs are taken if if
							  necessary */
								Store2DPoint(pPlot, i++, v[0], v[1], v[0] - v[2] / 2, v[0] + v[2] / 2, v[1], v[1], 0.0);
								break;
						    case LABELPOINTS:
								/* Load the coords just as we would have for a point plot */
								Store2DPoint(pPlot, i, v[0], v[1], v[0], v[0], v[1], v[1], -1.0);
								/* Allocate and fill in a GpTextLabel structure to match it */
								if(pPlot->points[i].type != UNDEFINED)
									store_label(pPlot->labels, &(pPlot->points[i]), i, GpDf.df_tokens[2], pPlot->varcolor ? pPlot->varcolor[i] : 0.0);
								i++;
								break;
						    case IMAGE: /* x_center y_center color_value */
								Store2DPoint(pPlot, i, v[0], v[1], v[0], v[0], v[1], v[1], v[2]);
								cp = &(pPlot->points[i]);
								COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(cp->CRD_COLOR, v[2], cp->type,
								COLOR_AXIS, pPlot->noautoscale, NOOP, NOOP);
								i++;
								break;
						    case POINTSTYLE: /* x, y, variable point size or type */
						    case LINESPOINTS:
						    case IMPULSES:
						    case LINES:
						    case DOTS:
								Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[2], v[1], v[1], v[2]);
								break;
						    case BOXPLOT: /* x, y, width expanded to xlow, xhigh */
								Store2DPoint(pPlot, i++, v[0], v[1], v[0]-v[2]/2., v[0]+v[2]/2., v[1], v[1], DEFAULT_BOXPLOT_FACTOR);
								break;

#ifdef EAM_OBJECTS
						    case CIRCLES: /* x, y, radius */
								/* by default a full circle is drawn */
								/* negative radius means default radius -> set flag in width */
								Store2DPoint(pPlot, i++, v[0], v[1], v[0]-v[2], v[0]+v[2], 0.0, 360.0, (v[2] >= 0) ? 0.0 : DEFAULT_RADIUS);
								break;
						    case ELLIPSES: /* x, y, major axis = minor axis, 0 as orientation */
								Store2DPoint(pPlot, i++, v[0], v[1], fabs(v[2]), fabs(v[2]), 0.0, v[2], (v[2] >= 0) ? 0.0 : DEFAULT_RADIUS);
								break;
#endif
					    } /*inner switch */

				    break;

				case 4:
				    /* x, y, ylow, yhigh OR
				     * x, y, xlow, xhigh OR
				     * x, y, xdelta, ydelta OR
				     * x, y, ydelta, width
				     */

				    switch(pPlot->plot_style) {
					    default:
						int_warn(storetoken, "This plot style does not work with 4 cols. Setting to yerrorbars");
						pPlot->plot_style = YERRORBARS;
					    /* fall through */

					    case YERRORLINES:
					    case YERRORBARS:
						Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[2],
					    v[3], -1.0);
						break;

					    case BOXXYERROR: /* x, y, dx, dy */
					    case XYERRORLINES:
					    case XYERRORBARS:
						Store2DPoint(pPlot, i++, v[0], v[1], v[0] - v[2], v[0] + v[2], v[1] - v[3], v[1] + v[3], 0.0);
						break;

					    case BOXES:
						/* x, y, xmin, xmax */
						Store2DPoint(pPlot, i++, v[0], v[1], v[2], v[3], v[1], v[1], 0.0);
						break;
					    case XERRORLINES:
					    case XERRORBARS:
						/* x, y, xmin, xmax */
						Store2DPoint(pPlot, i++, v[0], v[1], v[2], v[3], v[1], v[1], 0.0);
						break;

					    case BOXERROR:
						if(boxwidth == -2)
							/* x,y, ylow, yhigh --- width automatic */
							Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[2], v[3], -1.0);
						else
							/* x, y, dy, width */
							Store2DPoint(pPlot, i++, v[0], v[1], v[0] - v[3] / 2, v[0] + v[3] / 2, v[1] - v[2], v[1] + v[2], 0.0);
						break;

					    case BOXPLOT: /* x, y, factor */
					{
						int factor_index = check_or_add_boxplot_factor(pPlot, GpDf.df_tokens[3], v[0]);
						Store2DPoint(pPlot, i++, v[0], v[1], v[0]-v[2]/2., v[0]+v[2]/2., v[1], v[1], factor_index);
					}
					break;
					    case VECTOR:
						/* x,y,dx,dy */
						Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0] + v[2], v[1], v[1] + v[3], 0.);
						break;
					    case LABELPOINTS:
						/* Load the coords just as we would have for a point plot */
						Store2DPoint(pPlot, i, v[0], v[1], v[0], v[0], v[1], v[1], v[3]);
						/* Allocate and fill in a GpTextLabel structure to match it */
						if(pPlot->points[i].type != UNDEFINED) {
							GpTextLabel * tl = store_label(pPlot->labels, &(pPlot->points[i]), i, GpDf.df_tokens[2],
								pPlot->varcolor ? pPlot->varcolor[i] : 0.0);
							tl->lp_properties.p_size = v[3];
						}
						i++;
						break;

					    case POINTSTYLE:
					    case LINESPOINTS:
						/* Either there is no using spec and more than 3 columns in the data
						  file */
						/* or this is x:y:variable_size:variable_type */
						Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[3], v[1], v[1], v[2]);
						break;

#ifdef EAM_OBJECTS
					    case ELLIPSES: /* x, y, major axis, minor axis, 0 as orientation */
						Store2DPoint(pPlot, i++, v[0], v[1], fabs(v[2]), fabs(v[3]), 0.0, v[2], ((v[2] >= 0) && (v[3] >= 0)) ? 0.0 : DEFAULT_RADIUS);
						break;
#endif
				    } /*inner switch */

				    break;

				case 5:
			    { /* x, y, ylow, yhigh, width  or  x open low high close */
				    switch(pPlot->plot_style) {
					    default:
						int_warn(storetoken, "Unrecognized 5 column plot style; resetting to boxerrorbars");
						pPlot->plot_style = BOXERROR;
					    /*fall through */
					    case BOXERROR: /* x, y, ylow, yhigh, width */
						Store2DPoint(pPlot, i++, v[0], v[1], v[0] - v[4] / 2, v[0] + v[4] / 2, v[2], v[3], 0.0);
						break;

					    case FINANCEBARS: /* x yopen ylow yhigh yclose */
					    case CANDLESTICKS:
						Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[2], v[3], v[4]);
						break;

					    case VECTOR:
						/* x,y,dx,dy, variable arrowstyle */
						Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0] + v[2], v[1], v[1] + v[3], v[4]);
						break;

#ifdef EAM_OBJECTS
					    case CIRCLES: /* x, y, radius, arc begin, arc end */
						/* negative radius means default radius -> set flag in width */
						Store2DPoint(pPlot, i++, v[0], v[1], v[0]-v[2], v[0]+v[2], v[3], v[4], (v[2] >= 0) ? 0.0 : DEFAULT_RADIUS);
						break;
					    case ELLIPSES: /* x, y, major axis, minor axis, orientation */
						Store2DPoint(pPlot, i++, v[0], v[1], fabs(v[2]), fabs(v[3]), v[4], v[2], ((v[2] >= 0) && (v[3] >= 0)) ? 0.0 : DEFAULT_RADIUS);
						break;
#endif

					    case RGBIMAGE: /* x_center y_center r_value g_value b_value (rgb) */
						goto images;

					    case POINTSTYLE:
					    case LINESPOINTS:
						/* If there is no using spec and more than 4 columns in the data file */
						/* then use only the first 4 columns  x:y:variable_size:variable_type */
						Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[3], v[1], v[1], v[2]);
						break;
				    } /* inner switch */

				    break;
			    }

				case 7:
				/* same as six columns. Width ignored */
				/* eh ? - fall through */
				case 6:
				    /* x, y, xlow, xhigh, ylow, yhigh */
				    switch(pPlot->plot_style) {
					    default:
						int_warn(storetoken, "This plot style does not work with 6 cols. Setting to xyerrorbars");
						pPlot->plot_style = XYERRORBARS;
					    /*fall through */
					    case XYERRORLINES:
					    case XYERRORBARS:
					    case BOXXYERROR:
						Store2DPoint(pPlot, i++, v[0], v[1], v[2], v[3], v[4],
					    v[5], 0.0);
						break;

					    case CANDLESTICKS:
						Store2DPoint(pPlot, i++, v[0], v[1], v[5] > 0 ? v[0]-v[5]/2. : v[0], v[0], v[2], v[3], v[4]);
						break;

images:
					    case RGBA_IMAGE: /* x_cent y_cent red green blue alpha */
					    case RGBIMAGE: /* x_cent y_cent red green blue */
						Store2DPoint(pPlot, i, v[0], v[1], v[0], v[0], v[1], v[1], v[2]);
						// We will autoscale the RGB components to  a total range [0:255]
						// so we don't need to do any fancy scaling here.
						cp = &(pPlot->points[i]);
						cp->CRD_R = v[2];
						cp->CRD_G = v[3];
						cp->CRD_B = v[4];
						cp->CRD_A = v[5]; /* Alpha channel */
						i++;
						break;
				    }
			}       /*switch */
		}               /* "else" case for all plot types */
	}                       /*while */
	/* This removes extra point caused by blank lines after data. */
	if(i>0 && pPlot->points[i-1].type == UNDEFINED)
		i--;
	pPlot->p_count = i;
	cp_extend(pPlot, i); /* shrink to fit */
	/* Last chance to substitute input values for placeholders in plot title */
	GpDf.DfSetKeyTitle(pPlot);
	GpDf.DfClose();
	/* We are finished reading user input; return to C locale for internal use */
	reset_numeric_locale();
	return ngood;               /* 0 indicates an 'empty' file */
}
//
// called by get_data for each point 
//
//static void store2d_point(CurvePoints * pPlot, int i /* point number */, double x, double y, double xlow, double xhigh,
//    double ylow, double yhigh, double width /* BOXES widths: -1 -> autocalc, 0 ->  use xlow/xhigh */)
void GpGadgets::Store2DPoint(CurvePoints * pPlot, int i /* point number */, double x, double y, double xlow, double xhigh,
    double ylow, double yhigh, double width /* BOXES widths: -1 -> autocalc, 0 ->  use xlow/xhigh */)
{
	GpCoordinate * cp = &(pPlot->points[i]);
	coord_type dummy_type = INRANGE; /* sometimes we dont care about outranging */
	bool excluded_range = false;

#ifdef BACKWARDS_COMPATIBLE
	/* jev -- pass data values thru user-defined function */
	/* div -- y is dummy variable 2 - copy value there */
	if(ydata_func.at) {
		t_value val;
		ydata_func.dummy_values[0].SetComplex(y, 0.0);
		ydata_func.dummy_values[2] = ydata_func.dummy_values[0];
		Ev.EvaluateAt(ydata_func.at, &val);
		y = undefined ? 0.0 : val.Real();
		ydata_func.dummy_values[0].SetComplex(ylow, 0.0);
		ydata_func.dummy_values[2] = ydata_func.dummy_values[0];
		Ev.EvaluateAt(ydata_func.at, &val);
		ylow = undefined ? 0 : val.Real();
		ydata_func.dummy_values[0].SetComplex(yhigh, 0.0);
		ydata_func.dummy_values[2] = ydata_func.dummy_values[0];
		Ev.EvaluateAt(ydata_func.at, &val);
		yhigh = undefined ? 0 : val.Real();
	}
#endif
	dummy_type = cp->type = INRANGE;
	if(IsPolar) {
		double newx, newy;
		const double theta = x * Ang2Rad;
		GpAxis * p_theta_axis = &AxA[T_AXIS];
		// "x" is really the polar angle theta,	so check it against trange
		SETMIN(p_theta_axis->DataRange.low, theta);
		SETMAX(p_theta_axis->DataRange.upp, theta);
		if(theta < p_theta_axis->Range.low && (theta <= p_theta_axis->Range.upp || p_theta_axis->Range.upp == -GPVL)) {
			if((p_theta_axis->AutoScale & AUTOSCALE_MAX) == 0)
				excluded_range = true;
		}
		if(theta > p_theta_axis->Range.upp && (theta >= p_theta_axis->Range.low || p_theta_axis->Range.low == GPVL)) {
			if(!(p_theta_axis->AutoScale & AUTOSCALE_MIN))
				excluded_range = true;
		}
		// "y" at this point is really "r", so check it against rrange
		SETMIN(GetR().DataRange.low, y);
		if(y < GetR().Range.low) {
			if(!GetR().SetAutoscaleMin(0.0))
				cp->type = OUTRANGE;
		}
		SETMAX(GetR().DataRange.upp, y);
		if(y > GetR().Range.upp) {
			if(GetR().AutoScale & AUTOSCALE_MAX) {
				GetR().Range.upp = ((GetR().max_constraint & CONSTRAINT_UPPER) && (GetR().Ub.upp < y)) ? GetR().Ub.upp : y;
			}
			else
				cp->type = OUTRANGE;
		}
		if(GetR().Flags & GpAxis::fLog) {
			if(GetR().Range.low <= 0 || GetR().AutoScale & AUTOSCALE_MIN) {
				IntErrorNoCaret("In log mode rrange must not include 0");
			}
			y = DoLog(POLAR_AXIS, y) - DoLog(POLAR_AXIS, GetR().Range.low);
#ifdef NONLINEAR_AXES
		}
		else if(GetR().P_LinkToPrmr) {
			GpAxis * shadow = GetR().P_LinkToPrmr;
			y = shadow->EvalLinkFunction(y) - shadow->Range.low;
#endif
		}
		else if(!(GetR().AutoScale & AUTOSCALE_MIN)) {
			y -= GetR().Range.low; // we store internally as if plotting r(t)-rmin 
		}
		newx = y * cos(x * Ang2Rad);
		newy = y * sin(x * Ang2Rad);
		y = newy;
		x = newx;
		// Some plot styles use xhigh and yhigh for other quantities,
		// which polar mode transforms would break	
		if(pPlot->plot_style == CIRCLES) {
			const double radius = (xhigh - xlow)/2.0;
			xlow = x - radius;
			xhigh = x + radius;
		}
		else {
			if(!(GetR().AutoScale & AUTOSCALE_MAX) && yhigh > GetR().Range.upp) {
				cp->type = OUTRANGE;
			}
			if(GetR().Flags & GpAxis::fLog) {
				yhigh = DoLog(POLAR_AXIS, yhigh) - DoLog(POLAR_AXIS, GetR().Range.low);
#ifdef NONLINEAR_AXES
			}
			else if(GetR().P_LinkToPrmr) {
				GpAxis * shadow = GetR().P_LinkToPrmr;
				yhigh = shadow->EvalLinkFunction(yhigh) - shadow->Range.low;
#endif
			}
			else if(!(GetR().AutoScale & AUTOSCALE_MIN)) {
				yhigh -= GetR().Range.low; // we store internally as if plotting r(t)-rmin 
			}
			newx = yhigh * cos(xhigh * Ang2Rad);
			newy = yhigh * sin(xhigh * Ang2Rad);
			yhigh = newy;
			xhigh = newx;
			if(!(GetR().AutoScale & AUTOSCALE_MAX) && ylow > GetR().Range.upp) {
				cp->type = OUTRANGE;
			}
			if(GetR().Flags & GpAxis::fLog) {
				ylow = DoLog(POLAR_AXIS, ylow) - DoLog(POLAR_AXIS, GetR().Range.low);
#ifdef NONLINEAR_AXES
			}
			else if(GetR().P_LinkToPrmr) {
				GpAxis * shadow = GetR().P_LinkToPrmr;
				ylow = shadow->EvalLinkFunction(ylow) - shadow->Range.low;
#endif
			}
			else if(!(GetR().AutoScale & AUTOSCALE_MIN)) {
				ylow -= GetR().Range.low; // we store internally as if plotting r(t)-rmin
			}
			newx = ylow * cos(xlow * Ang2Rad);
			newy = ylow * sin(xlow * Ang2Rad);
			ylow = newy;
			xlow = newx;
		}
	}
	// Version 5: Allow to store Inf or NaN
	// We used to exit immediately in this case rather than storing anything 
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->x, x, cp->type, pPlot->x_axis, pPlot->noautoscale, NOOP, NOOP);
	STORE_WITH_LOG_AND_UPDATE_RANGE(cp->y, y, cp->type, pPlot->y_axis, pPlot->noautoscale, NOOP, NOOP);
	switch(pPlot->plot_style) {
		case POINTSTYLE: /* Only x and y are relevant to axis scaling */
		case LINES:
		case LINESPOINTS:
		case LABELPOINTS:
		case DOTS:
		case IMPULSES:
		case STEPS:
		case FSTEPS:
		case HISTEPS:
		    cp->xlow = xlow;
		    cp->xhigh = xhigh;
		    cp->ylow = ylow;
		    cp->yhigh = yhigh;
		    break;
		case BOXES:     /* auto-scale to xlow xhigh */
		    cp->ylow = ylow;
		    cp->yhigh = yhigh;
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xlow,  xlow,  dummy_type, pPlot->x_axis, pPlot->noautoscale, NOOP, cp->xlow = -GPVL);
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xhigh, xhigh, dummy_type, pPlot->x_axis, pPlot->noautoscale, NOOP, cp->xhigh = -GPVL);
		    break;
		case BOXPLOT:           /* auto-scale to xlow xhigh, factor is already in z */
		    cp->ylow = ylow;    /* ylow yhigh not really needed but store them anyway */
		    cp->yhigh = yhigh;
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xlow,  xlow,  dummy_type, pPlot->x_axis, pPlot->noautoscale, NOOP, cp->xlow = -GPVL);
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xhigh, xhigh, dummy_type, pPlot->x_axis, pPlot->noautoscale, NOOP, cp->xhigh = -GPVL);
		    break;
#ifdef EAM_OBJECTS
		case CIRCLES:
		    cp->yhigh = yhigh;
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xlow,  xlow,  dummy_type, pPlot->x_axis, pPlot->noautoscale, NOOP, cp->xlow = -GPVL);
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xhigh, xhigh, dummy_type, pPlot->x_axis, pPlot->noautoscale, NOOP, cp->xhigh = -GPVL);
		    cp->ylow  = ylow; /* arc begin */
		    cp->xhigh = yhigh; /* arc end */
		    if(fabs(ylow) > 1000. || fabs(yhigh) > 1000.) /* safety check for insane arc angles */
			    cp->type = UNDEFINED;
		    break;
		case ELLIPSES:
			// We want to pass the parameters to the ellipse drawing routine as they are,
			// so we have to calculate the extent of the ellipses for autoscaling here.
			// Properly calculating the correct extent of a rotated ellipse, respecting
			// axis scales and all would be very hard.
			// So we just use the larger of the two axes, multiplied by some empirical factors
			// to ensure^Whope that all parts of the ellipses will be in the auto-scaled area
			// xlow = major axis, xhigh = minor axis, ylow = orientation
#define YRANGE_FACTOR ((pPlot->ellipseaxes_units == ELLIPSEAXES_YY) ? 1.0 : 1.4)
#define XRANGE_FACTOR ((pPlot->ellipseaxes_units == ELLIPSEAXES_XX) ? 1.1 : 1.0)
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xlow,  x-0.5*MAX(xlow, xhigh)*XRANGE_FACTOR, dummy_type, pPlot->x_axis, pPlot->noautoscale, NOOP, cp->xlow = -GPVL);
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xhigh, x+0.5*MAX(xlow, xhigh)*XRANGE_FACTOR, dummy_type, pPlot->x_axis, pPlot->noautoscale, NOOP, cp->xhigh = -GPVL);
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->ylow,  y-0.5*MAX(xlow, xhigh)*YRANGE_FACTOR, dummy_type, pPlot->y_axis, pPlot->noautoscale, NOOP, cp->ylow = -GPVL);
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->yhigh, y+0.5*MAX(xlow, xhigh)*YRANGE_FACTOR, dummy_type, pPlot->y_axis, pPlot->noautoscale, NOOP, cp->yhigh = -GPVL);
		    /* So after updating the axes we re-store the parameters */
		    cp->xlow = xlow; /* major axis */
		    cp->xhigh = xhigh; /* minor axis */
		    cp->ylow = ylow; /* orientation */
		    break;
#endif

		default:        /* auto-scale to xlow xhigh ylow yhigh */
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xlow,  xlow,  dummy_type, pPlot->x_axis, pPlot->noautoscale, NOOP, cp->xlow = -GPVL);
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->xhigh, xhigh, dummy_type, pPlot->x_axis, pPlot->noautoscale, NOOP, cp->xhigh = -GPVL);
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->ylow,  ylow,  dummy_type, pPlot->y_axis, pPlot->noautoscale, NOOP, cp->ylow = -GPVL);
		    STORE_WITH_LOG_AND_UPDATE_RANGE(cp->yhigh, yhigh, dummy_type, pPlot->y_axis, pPlot->noautoscale, NOOP, cp->yhigh = -GPVL);
		    break;
	}
	/* HBB 20010214: if z is not used for some actual value, just
	 * store 'width' to that axis and be done with it */
	if((int)pPlot->z_axis == NO_AXIS)
		cp->z = width;
	else
		STORE_WITH_LOG_AND_UPDATE_RANGE(cp->z, width, dummy_type, pPlot->z_axis, pPlot->noautoscale, NOOP, cp->z = -GPVL);
	// If we have variable color corresponding to a z-axis value, use it to autoscale
	// June 2010 - New mechanism for variable color
	if(pPlot->lp_properties.pm3d_color.type == TC_Z && pPlot->varcolor)
		COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(pPlot->varcolor[i], pPlot->varcolor[i], dummy_type, COLOR_AXIS, pPlot->noautoscale, NOOP, NOOP);

	/* July 2014 - Some points are excluded because they fall outside of trange	*/
	/* even though they would be inside the plot if drawn.			*/
	if(excluded_range)
		cp->type = EXCLUDEDRANGE;
}                               /* store2d_point */
//
// We abuse the labels structure to store a list of boxplot labels ("factors").
// Check if <string> is already among the known factors, if not, add it to the list.
//
static int check_or_add_boxplot_factor(CurvePoints * plot, char* string, double x)
{
	int    index = DEFAULT_BOXPLOT_FACTOR;
	// If there is no factor column (4th using spec) fall back to a single boxplot 
	if(string) {
		// Remove the trailing garbage, quotes etc. from the string 
		char * trimmed_string = GpDf.DfParseStringField(string);
		if(strlen(trimmed_string) > 0) {
			bool p_new = false;
			GpTextLabel * label;
			GpTextLabel * prev_label = plot->labels;
			if(!prev_label)
				GpGg.IntErrorNoCaret("boxplot labels not initialized");
			for(label = prev_label->next; label; label = label->next, prev_label = prev_label->next) {
				// check if string is already stored 
				if(!strcmp(trimmed_string, label->text))
					break;
				// If we are keeping a sorted list, test against current entry (insertion sort)
				if(GpGg.boxplot_opts.sort_factors) {
					if(strcmp(trimmed_string, label->text) < 0) {
						p_new = true;
						break;
					}
				}
			}
			// not found, so we add it now 
			if(!label || p_new) {
				GpTextLabel * new_label = (GpTextLabel *)malloc(sizeof(GpTextLabel));
				memcpy(new_label, plot->labels, sizeof(GpTextLabel));
				new_label->next = label;
				new_label->tag = plot->boxplot_factors++;
				new_label->text = gp_strdup(trimmed_string);
				new_label->place.x = plot->points[0].x;
				prev_label->next = new_label;
				label = new_label;
			}
			index = label->tag;
		}
		free(trimmed_string);
	}
	return index;
}
//
// Autoscaling of boxplots with no explicit width cuts off the outer edges of the box 
//
void GpGadgets::BoxPlotRangeFiddling(CurvePoints * pPlot)
{
	double extra_width;
	int    n;
	// Create a tic label for each boxplot category
	if(pPlot->boxplot_factors > 0) {
		if(boxplot_opts.labels != BOXPLOT_FACTOR_LABELS_OFF) {
			//
			// Add tic labels to the boxplots,
			// showing which level of the factor variable they represent
			//
			//add_tics_boxplot_factors(pPlot);
			//static void add_tics_boxplot_factors(CurvePoints * plot)
			int i = 0;
			AXIS_INDEX boxplot_labels_axis = (boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X) ? FIRST_X_AXIS : 
				((boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X2) ? SECOND_X_AXIS : XAxis);
			GpAxis & r_ax = AxA[boxplot_labels_axis];
			for(GpTextLabel * p_label = pPlot->labels->next; p_label; p_label = p_label->next) {
				add_tic_user(&r_ax, p_label->text, pPlot->points->x + i * boxplot_opts.separation, -1);
				i++;
			}
		}
	}
	// Sort the points and removed any that are undefined
	n = filter_boxplot(pPlot);
	pPlot->p_count = n;
	if(pPlot->points[0].type == UNDEFINED)
		GpGg.IntErrorNoCaret("boxplot has undefined x GpCoordinate");
	extra_width = pPlot->points[0].xhigh - pPlot->points[0].xlow;
	if(extra_width == 0)
		extra_width = (boxwidth > 0 && boxwidth_is_absolute) ? boxwidth : 0.5;
	if(extra_width < 0)
		extra_width = -extra_width;
	{
		GpAxis & r_ax_x = AxA[pPlot->x_axis];
		if(r_ax_x.AutoScale & AUTOSCALE_MIN) {
			if(r_ax_x.Range.low >= pPlot->points[0].x)
				r_ax_x.Range.low -= 1.5 * extra_width;
			else if(r_ax_x.Range.low >= pPlot->points[0].x - extra_width)
				r_ax_x.Range.low -= 1 * extra_width;
		}
		if(r_ax_x.AutoScale & AUTOSCALE_MAX) {
			double nfactors = MAX(0, pPlot->boxplot_factors - 1);
			double plot_max = pPlot->points[0].x + nfactors * boxplot_opts.separation;
			if(r_ax_x.Range.upp <= plot_max)
				r_ax_x.Range.upp = plot_max + 1.5 * extra_width;
			else if(r_ax_x.Range.upp <= plot_max + extra_width)
				r_ax_x.Range.upp += extra_width;
		}
	}
}
//
// Since the stored x values for histogrammed data do not correspond exactly
// to the eventual x coordinates, we need to modify the x axis range bounds.
// Also the two stacked histogram modes need adjustment of the y axis bounds.
//
//static void histogram_range_fiddling(CurvePoints * plot)
void GpGadgets::HistogramRangeFiddling(CurvePoints * pPlot)
{
	double xlow, xhigh;
	int i;
	//
	// EAM FIXME - HT_STACKED_IN_TOWERS forcibly resets xmin, which is only correct if no other pPlot came first.
	//
	switch(histogram_opts.type) {
		case HT_STACKED_IN_LAYERS:
		    if(AxA[pPlot->y_axis].AutoScale & AUTOSCALE_MAX) {
			    if(pPlot->histogram_sequence == 0) {
				    free(StackHeight);
				    StackHeight = (GpCoordinate *)malloc(pPlot->p_count * sizeof(GpCoordinate GPHUGE));
				    for(StackCount = 0; StackCount < pPlot->p_count; StackCount++) {
					    StackHeight[StackCount].yhigh = 0;
					    StackHeight[StackCount].ylow = 0;
				    }
			    }
			    else if(pPlot->p_count > StackCount) {
				    StackHeight = (GpCoordinate *)gp_realloc(StackHeight, pPlot->p_count * sizeof(GpCoordinate GPHUGE), "stackheight array");
				    for(; StackCount < pPlot->p_count; StackCount++) {
					    StackHeight[StackCount].yhigh = 0;
					    StackHeight[StackCount].ylow = 0;
				    }
			    }
			    for(i = 0; i < StackCount; i++) {
					if(pPlot->points[i].type != UNDEFINED) {
						if(pPlot->points[i].y >= 0)
							StackHeight[i].yhigh += pPlot->points[i].y;
						else
							StackHeight[i].ylow += pPlot->points[i].y;
						SETMAX(AxA[pPlot->y_axis].Range.upp, StackHeight[i].yhigh);
						SETMIN(AxA[pPlot->y_axis].Range.low, StackHeight[i].ylow);
					}
			    }
		    }
		/* fall through to checks on x range */
		case HT_CLUSTERED:
		case HT_ERRORBARS:
		    if(!AxA[FIRST_X_AXIS].AutoScale)
			    break;
		    if(AxA[FIRST_X_AXIS].AutoScale & AUTOSCALE_MIN) {
			    xlow = pPlot->histogram->start - 1.0;
			    SETMIN(AxA[FIRST_X_AXIS].Range.low, xlow);
		    }
		    if(AxA[FIRST_X_AXIS].AutoScale & AUTOSCALE_MAX) {
			    // FIXME - why did we increment p_count on UNDEFINED points? 
			    while(pPlot->points[pPlot->p_count-1].type == UNDEFINED) {
				    pPlot->p_count--;
				    if(!pPlot->p_count)
					    GpGg.IntErrorNoCaret("All points in histogram UNDEFINED");
			    }
			    xhigh = pPlot->points[pPlot->p_count-1].x;
			    xhigh += pPlot->histogram->start + 1.0;
			    SETMAX(AxA[FIRST_X_AXIS].Range.upp, xhigh);
		    }
		    break;
		case HT_STACKED_IN_TOWERS:
		    /* FIXME: Rather than trying to reproduce the layout along X */
		    /* we should just track the actual xmin/xmax as we go.       */
		    if(AxA[FIRST_X_AXIS].SetAutoScale) {
			    xlow = -1.0;
			    xhigh = pPlot->histogram_sequence;
			    xhigh += pPlot->histogram->start + 1.0;
			    SETMIN(AxA[FIRST_X_AXIS].Range.low, xlow);
				AxA[FIRST_X_AXIS].Range.upp = xhigh;
		    }
		    if(AxA[FIRST_Y_AXIS].SetAutoScale) {
			    double ylow, yhigh;
			    for(i = 0, yhigh = ylow = 0.0; i<pPlot->p_count; i++)
				    if(pPlot->points[i].type != UNDEFINED) {
					    if(pPlot->points[i].y >= 0)
						    yhigh += pPlot->points[i].y;
					    else
						    ylow += pPlot->points[i].y;
				    }
			    if(AxA[FIRST_Y_AXIS].SetAutoScale & AUTOSCALE_MAX)
				    SETMAX(AxA[pPlot->y_axis].Range.upp, yhigh);
			    if(AxA[FIRST_Y_AXIS].SetAutoScale & AUTOSCALE_MIN)
				    SETMIN(AxA[pPlot->y_axis].Range.low, ylow);
		    }
		    break;
	}
}
//
// If the plot is in polar coordinates and the r axis range is autoscaled,
// we need to apply the maximum radius found to both x and y.
// Otherwise the autoscaling will be done separately for x and y and the
// resulting plot will not be centered at the origin.
//
//void polar_range_fiddling(const CurvePoints * plot)
void GpAxisBlock::PolarRangeFiddling(const CurvePoints * plot)
{
	if(AxA[POLAR_AXIS].SetAutoScale & AUTOSCALE_MAX) {
		double plotmax_x = MAX(AxA[plot->x_axis].Range.upp, -AxA[plot->x_axis].Range.low);
		double plotmax_y = MAX(AxA[plot->y_axis].Range.upp, -AxA[plot->y_axis].Range.low);
		double plotmax = MAX(plotmax_x, plotmax_y);
		if((AxA[plot->x_axis].SetAutoScale & AUTOSCALE_BOTH) == AUTOSCALE_BOTH) {
			AxA[plot->x_axis].Range.upp = plotmax;
			AxA[plot->x_axis].Range.low = -plotmax;
		}
		if((AxA[plot->y_axis].SetAutoScale & AUTOSCALE_BOTH) == AUTOSCALE_BOTH) {
			AxA[plot->y_axis].Range.upp = plotmax;
			AxA[plot->y_axis].Range.low = -plotmax;
		}
	}
}
//
// Extend auto-scaling of y-axis to include zero
//
static void impulse_range_fiddling(CurvePoints * plot)
{
	if(!(GpGg[plot->y_axis].Flags & GpAxis::fLog)) {
		if(GpGg[plot->y_axis].AutoScale & AUTOSCALE_MIN) {
			SETMIN(GpGg[plot->y_axis].Range.low, 0);
		}
		if(GpGg[plot->y_axis].AutoScale & AUTOSCALE_MAX) {
			SETMAX(GpGg[plot->y_axis].Range.upp, 0);
		}
	}
}
//
// store_label() is called by get_data for each point 
// This routine is exported so it can be shared by plot3d 
//
GpTextLabel * store_label(GpTextLabel * listhead, GpCoordinate * cp,
    int i /* point number */, char * string/* start of label string */, double colorval/* used if text color derived from palette */)
{
	GpTextLabel * tl = listhead;
	int textlen;
	/* Walk through list to get to the end. Yes I know this is inefficient */
	/* but is anyone really going to plot so many labels that it matters?  */
	if(!tl)
		GpGg.IntErrorNoCaret("GpTextLabel list was not initialized");
	while(tl->next)
		tl = tl->next;
	/* Allocate a new label structure and fill it in */
	tl->next = (GpTextLabel *)malloc(sizeof(GpTextLabel));
	memcpy(tl->next, tl, sizeof(GpTextLabel));
	tl = tl->next;
	tl->next = 0;
	tl->tag = i;
	tl->place.x = cp->x;
	tl->place.y = cp->y;
	tl->place.z = cp->z;
	// Check for optional (textcolor palette ...) 
	if(tl->textcolor.type == TC_Z)
		tl->textcolor.value = colorval;
	// Check for optional (textcolor rgb variable) 
	else if(listhead->textcolor.type == TC_RGB && listhead->textcolor.value < 0)
		tl->textcolor.lt = (int)colorval;
	// Check for optional (textcolor variable) 
	else if(listhead->textcolor.type == TC_VARIABLE) {
		lp_style_type lptmp;
		if(GpGg.prefer_line_styles)
			lp_use_properties(&lptmp, (int)colorval);
		else
			load_linetype(&lptmp, (int)colorval);
		tl->textcolor = lptmp.pm3d_color;
	}
	if((listhead->lp_properties.flags & LP_SHOW_POINTS)) {
		// Check for optional (point linecolor palette ...) 
		if(tl->lp_properties.pm3d_color.type == TC_Z)
			tl->lp_properties.pm3d_color.value = colorval;
		// Check for optional (point linecolor rgb variable) 
		else if(listhead->lp_properties.pm3d_color.type == TC_RGB && listhead->lp_properties.pm3d_color.value < 0)
			tl->lp_properties.pm3d_color.lt = (int)colorval;
		// Check for optional (point linecolor variable) 
		else if(listhead->lp_properties.l_type == LT_COLORFROMCOLUMN) {
			lp_style_type lptmp;
			if(GpGg.prefer_line_styles)
				lp_use_properties(&lptmp, (int)colorval);
			else
				load_linetype(&lptmp, (int)colorval);
			tl->lp_properties.pm3d_color = lptmp.pm3d_color;
		}
	}
	// Check for null string (no label) 
	SETIFZ(string, "");
	textlen = 0;
	// FIXME EAM - this code is ugly but seems to work 
	// We need to handle quoted separators and quoted quotes 
	if(GpDf.df_separators) {
		bool in_quote = false;
		while(string[textlen]) {
			if(string[textlen] == '"')
				in_quote = !in_quote;
			else if(strchr(GpDf.df_separators, string[textlen]) && !in_quote)
				break;
			textlen++;
		}
		while(textlen > 0 && isspace((uchar)string[textlen-1]))
			textlen--;
	}
	else {
		/* This is the normal case (no special separator character) */
		if(*string == '"') {
			for(textlen = 1; string[textlen] && string[textlen] != '"'; textlen++) ;
		}
		while(string[textlen] && !isspace((uchar)string[textlen]))
			textlen++;
	}
	/* Strip double quote from both ends */
	if(string[0] == '"' && string[textlen-1] == '"')
		textlen -= 2, string++;
	tl->text = (char *)malloc(textlen+1);
	strncpy(tl->text, string, textlen);
	tl->text[textlen] = '\0';
	parse_esc(tl->text);
	FPRINTF((stderr, "LABELPOINT %f %f \"%s\" \n", tl->place.x, tl->place.y, tl->text));
	return tl;
}

/* HBB 20010610: mnemonic names for the bits stored in 'uses_axis' */
enum t_uses_axis {
	USES_AXIS_FOR_DATA = 1,
	USES_AXIS_FOR_FUNC = 2
};
//
// This parses the plot command after any global range specifications.
// To support autoscaling on the x axis, we want any data files to define the
// x range, then to plot any functions using that range. We thus parse the input
// twice, once to pick up the data files, and again to pick up the functions.
// Definitions are processed twice, but that won't hurt.
//
//static void eval_plots()
void GpGadgets::EvalPlots(GpCommand & rC)
{
	int    i;
	CurvePoints * p_plot = 0;
	CurvePoints ** tp_ptr = 0;
	/*t_uses_axis*/int uses_axis[AXIS_ARRAY_SIZE];
	int    some_functions = 0;
	int    plot_num, line_num;
	bool   was_definition = false;
	int    pattern_num;
	char * xtitle = NULL;
	int    begin_token = rC.CToken; /* so we can rewind for second pass */
	int    start_token = 0, end_token;
	int    highest_iteration = 0; /* last index reached in iteration [i=start:*] */
	legend_key * key = &keyT;
	char   orig_dummy_var[MAX_ID_LEN+1];
#ifdef SMOOTH_BINS_OPTION
	int    nbins = 0;
	double binlow, binhigh;
#endif
	double newhist_start = 0.0;
	int histogram_sequence = -1;
	int newhist_color = 1;
	int newhist_pattern = LT_UNDEFINED;
	histogram_rightmost = 0.0;
	free_histlist(&histogram_opts);
	init_histogram(NULL, NULL);

	uses_axis[FIRST_X_AXIS] = (t_uses_axis)0;
    uses_axis[FIRST_Y_AXIS] = (t_uses_axis)0;
    uses_axis[SECOND_X_AXIS] = (t_uses_axis)0;
    uses_axis[SECOND_Y_AXIS] = (t_uses_axis)0;

	/* Original Comment follows: */
	/* Reset first_plot. This is usually done at the end of this function.
	 * If there is an error within this function, the memory is left allocated,
	 * since we cannot call cp_free if the list is incomplete. Making sure that
	 * the list structure is always valid requires some rewriting */
	/* EAM Apr 2007 - but we need to keep the previous structures around in
	 * order to be able to refresh/zoom them without re-reading all the data.
	 */
	cp_free(P_FirstPlot);
	P_FirstPlot = NULL;
	tp_ptr = &(P_FirstPlot);
	plot_num = 0;
	line_num = 0;           /* default line type */
	pattern_num = DefaultFillStyle.fillpattern;    /* default fill pattern */
	strcpy(orig_dummy_var, rC.P.CDummyVar[0]);
	InParametric = false;
	xtitle = NULL;
	/* Assume that the input data can be re-read later */
	IsVolatileData = false;
	/* ** First Pass: Read through data files ***
	 * This pass serves to set the xrange and to parse the command, as well
	 * as filling in every thing except the function data. That is done after
	 * the xrange is defined.
	 */
	rC.P.P_PlotIterator = rC.CheckForIteration();
	while(true) {
		// Forgive trailing comma on a multi-element plot command
		if(rC.EndOfCommand()) {
			if(plot_num == 0)
				IntErrorCurToken("function to plot expected");
			break;
		}
		p_plot = NULL;
		if(!InParametric && !was_definition)
			start_token = rC.CToken;
		if(rC.AlmostEq("newhist$ogram")) {
			rC.CToken++;
			lp_style_type lp; // = DEFAULT_LP_STYLE_TYPE;
			fill_style_type fs;
			int previous_token;
			histogram_sequence = -1;
			memzero(&histogram_title, sizeof(GpTextLabel));
			if(histogram_rightmost > 0)
				newhist_start = histogram_rightmost + 2;
			lp.l_type = line_num;
			newhist_color = lp.l_type + 1;
			fs.fillpattern = LT_UNDEFINED;
			do {
				previous_token = rC.CToken;
				if(rC.Eq("at")) {
					rC.CToken++;
					newhist_start = rC.RealExpression();
				}
				// Store title in temporary variable and then copy into the
				// new histogram structure when it is allocated.
				if(!histogram_title.text && rC.IsStringValue(rC.CToken)) {
					histogram_title.textcolor = histogram_opts.title.textcolor;
					histogram_title.boxed = histogram_opts.title.boxed;
					histogram_title.pos = histogram_opts.title.pos;
					histogram_title.text = rC.TryToGetString();
					histogram_title.font = gp_strdup(histogram_opts.title.font);
					ParseLabelOptions(rC, &histogram_title, 2);
				}
				// Allow explicit starting color or pattern for this histogram
				if(rC.Eq("lt") || rC.AlmostEq("linet$ype")) {
					rC.CToken++;
					newhist_color = rC.IntExpression();
				}
				rC.ParseFillStyle(&fs, FS_SOLID, 100, fs.fillpattern, DefaultFillStyle.border_color);
			} while(rC.CToken != previous_token);
			newhist_pattern = fs.fillpattern;
			if(!rC.Eq(","))
				IntErrorCurToken("syntax error");
		}
		else if(rC.IsDefinition()) {
			rC.Define();
			if(rC.Eq(","))
				rC.CToken++;
			was_definition = true;
			continue;
		}
		else {
			int specs = 0;
			// for datafile plot, record datafile spec for title
			char* name_str;
			bool duplication = false;
			bool set_smooth = false, set_axes = false, set_title = false;
			bool set_with = false, set_lpstyle = false;
			bool set_fillstyle = false;
			bool set_labelstyle = false;
#ifdef EAM_OBJECTS
			bool set_ellipseaxes_units = false;
#endif
			int sample_range_token; // Only used by function plots
			plot_num++;
			// Check for a sampling range.
			InitSampleRange(&AxA[FIRST_X_AXIS]);
			sample_range_token = ParseRange(SAMPLE_AXIS, rC);
			if(sample_range_token != 0)
				AxA[SAMPLE_AXIS].range_flags |= RANGE_SAMPLED;
			was_definition = false;
			rC.P_DummyFunc = &plot_func;
			// Allow replacement of the dummy variable in a function
			if(sample_range_token > 0)
				rC.CopyStr(rC.P.CDummyVar[0], sample_range_token, MAX_ID_LEN);
			else if(sample_range_token < 0)
				strcpy(rC.P.CDummyVar[0], rC.P.SetDummyVar[0]);
			else
				strcpy(rC.P.CDummyVar[0], orig_dummy_var);
			// Should this be saved in "p_plot"?
			name_str = rC.P.StringOrExpress(rC, NULL);
			rC.P_DummyFunc = NULL;
			if(name_str) { /* data file to plot */
				if(IsParametric && InParametric)
					IntErrorCurToken("previous parametric function not fully specified");
				if(sample_range_token !=0 && *name_str != '+')
					int_warn(sample_range_token, "Ignoring sample range in non-sampled data plot");
				if(*tp_ptr)
					p_plot = *tp_ptr;
				else { // no memory malloc()'d there yet
					p_plot = cp_alloc(MIN_CRV_POINTS);
					*tp_ptr = p_plot;
				}
				p_plot->plot_type = DATA;
				p_plot->plot_style = DataStyle;
				p_plot->plot_smooth = SMOOTH_NONE;
				p_plot->filledcurves_options.opt_given = 0;
				// up to MAXDATACOLS cols
				GpDf.DfSetPlotMode(MODE_PLOT); // Needed for binary datafiles 
				specs = GpDf.DfOpen(rC, name_str, MAXDATACOLS, p_plot);
				// Store a pointer to the named variable used for sampling 
				if(sample_range_token > 0) {
					p_plot->sample_var = Ev.AddUdv(rC, sample_range_token);
				}
				else {
					// FIXME: This has the side effect of creating a named variable x 
					// or overwriting an existing variable x.  Maybe it should save   
					// and restore the pre-existing variable in this case?            
					p_plot->sample_var = Ev.AddUdvByName(rC.P.CDummyVar[0]);
				}
				if(p_plot->sample_var->udv_value.type == NOTDEFINED)
					p_plot->sample_var->udv_value.SetComplex(0.0, 0.0);
				// include modifiers in default title
				p_plot->Token = end_token = rC.CToken - 1;
			}
			else {
				// function to plot
				some_functions = 1;
				if(IsParametric) // working on x parametric function
					InParametric = !InParametric;
				if(*tp_ptr) {
					p_plot = *tp_ptr;
					cp_extend(p_plot, Samples1 + 1);
				}
				else { // no memory malloc()'d there yet
					p_plot = cp_alloc(Samples1 + 1);
					*tp_ptr = p_plot;
				}
				p_plot->plot_type = FUNC;
				p_plot->plot_style = FuncStyle;
				p_plot->filledcurves_options.opt_given = 0;
				end_token = rC.CToken - 1;
			}
			// axis defaults
			XAxis = FIRST_X_AXIS;
			YAxis = FIRST_Y_AXIS;
			// pm 25.11.2001 allow any order of options
			while(!rC.EndOfCommand()) {
				int save_token = rC.CToken;
#ifdef SMOOTH_BINS_OPTION
				// bin the data if requested
				if(rC.Eq("bins")) {
					if(set_smooth) {
						duplication = true;
						break;
					}
					rC.CToken++;
					p_plot->plot_smooth = SMOOTH_BINS;
					nbins = Samples1;
					if(rC.Eq("=")) {
						rC.CToken++;
						nbins = rC.IntExpression();
						if(nbins <= 0)
							nbins = Samples1;
					}
					binlow = binhigh = 0.0;
					if(rC.Eq("binrange")) {
						rC.CToken++;
						if(!ParseRange(SAMPLE_AXIS, rC))
							IntErrorCurToken("incomplete bin range");
						binlow  = AxA[SAMPLE_AXIS].Range.low;
						binhigh = AxA[SAMPLE_AXIS].Range.upp;
					}
					continue;
				}
#endif
				// deal with smooth
				if(rC.AlmostEq("s$mooth")) {
					int found_token;
					if(set_smooth) {
						duplication = true;
						break;
					}
					found_token = rC.LookupTable(plot_smooth_tbl, ++rC.CToken);
					rC.CToken++;
					switch(found_token) {
						case SMOOTH_UNWRAP:
						case SMOOTH_FREQUENCY:
						    p_plot->plot_smooth = (PLOT_SMOOTH)found_token;
						    break;
						case SMOOTH_KDENSITY:
						    p_plot->smooth_parameter = -1; /* Default */
						    if(rC.AlmostEq("band$width")) {
							    rC.CToken++;
							    p_plot->smooth_parameter = rC.RealExpression();
						    }
						/* Fall through */
						case SMOOTH_ACSPLINES:
						case SMOOTH_BEZIER:
						case SMOOTH_CSPLINES:
						case SMOOTH_SBEZIER:
						case SMOOTH_UNIQUE:
						case SMOOTH_CUMULATIVE:
						case SMOOTH_CUMULATIVE_NORMALISED:
						case SMOOTH_MONOTONE_CSPLINE:
						    p_plot->plot_smooth = (PLOT_SMOOTH)found_token;
						    p_plot->plot_style = LINES;
						    break;
						case SMOOTH_NONE:
						default:
						    IntErrorCurToken("unrecognized 'smooth' option");
						    break;
					}
					set_smooth = true;
					continue;
				}
				// look for axes/axis
				if(rC.AlmostEq("ax$es") || rC.AlmostEq("ax$is")) {
					if(set_axes) {
						duplication = true;
						break;
					}
					if(IsParametric && InParametric)
						IntErrorCurToken("previous parametric function not fully specified");
					rC.CToken++;
					switch(rC.LookupTable(&plot_axes_tbl[0], rC.CToken)) {
						case AXES_X1Y1:
						    XAxis = FIRST_X_AXIS;
						    YAxis = FIRST_Y_AXIS;
						    ++rC.CToken;
						    break;
						case AXES_X2Y2:
						    XAxis = SECOND_X_AXIS;
						    YAxis = SECOND_Y_AXIS;
						    ++rC.CToken;
						    break;
						case AXES_X1Y2:
						    XAxis = FIRST_X_AXIS;
						    YAxis = SECOND_Y_AXIS;
						    ++rC.CToken;
						    break;
						case AXES_X2Y1:
						    XAxis = SECOND_X_AXIS;
						    YAxis = FIRST_Y_AXIS;
						    ++rC.CToken;
						    break;
						case AXES_NONE:
						default:
						    IntErrorCurToken("axes must be x1y1, x1y2, x2y1 or x2y2");
						    break;
					}
					set_axes = true;
					continue;
				}
				// Allow this plot not to affect autoscaling
				if(rC.AlmostEq("noauto$scale")) {
					rC.CToken++;
					p_plot->noautoscale = true;
					continue;
				}
				// deal with title
				ParsePlotTitle(rC, p_plot, xtitle, NULL, &set_title);
				if(save_token != rC.CToken)
					continue;
				// deal with style
				if(rC.AlmostEq("w$ith")) {
					if(set_with) {
						duplication = true;
						break;
					}
					if(IsParametric && InParametric)
						IntErrorCurToken("\"with\" allowed only after parametric function fully specified");
					p_plot->plot_style = get_style(rC);
					if(p_plot->plot_style == FILLEDCURVES) {
						// read a possible option for 'with filledcurves'
						get_filledcurves_style_options(rC, &p_plot->filledcurves_options);
					}
					if(oneof3(p_plot->plot_style, IMAGE, RGBIMAGE, RGBA_IMAGE)) {
						if(p_plot->plot_type == FUNC)
							IntErrorCurToken("This plot style is only for data files");
						else
							get_image_options(&p_plot->image_properties);
					}
					if((p_plot->plot_type == FUNC) && ((p_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR)
						|| (p_plot->plot_style == LABELPOINTS) || (p_plot->plot_style == PARALLELPLOT))) {
						int_warn(rC.CToken, "This plot style is only for datafiles, reverting to \"points\"");
						p_plot->plot_style = POINTSTYLE;
					}
					if(p_plot->plot_style == TABLESTYLE) {
						if(!table_mode)
							IntErrorCurToken("'with table' requires a previous 'set table'");
					}

					/* Parallel plots require allocating additional storage.		*/
					/* NB: This will be one column more than needed if the final column	*/
					/*     contains variable color information. We will free it later.	*/
					if(p_plot->plot_style == PARALLELPLOT) {
						int i;
						if(GpDf.df_no_use_specs < 2)
							GpGg.IntErrorNoCaret("not enough 'using' columns");
						p_plot->n_par_axes = GpDf.df_no_use_specs;
						p_plot->z_n = (double **)malloc((GpDf.df_no_use_specs) * sizeof(double*));
						for(i = 0; i < p_plot->n_par_axes; i++)
							p_plot->z_n[i] = (double *)malloc(p_plot->p_max * sizeof(double));
					}

					set_with = true;
					continue;
				}

				/* pick up line/point specs and other style-specific keywords
				 * - point spec allowed if style uses points, ie style&2 != 0
				 * - keywords for lt and pt are optional
				 */
				if(p_plot->plot_style == CANDLESTICKS) {
					if(rC.AlmostEq("whisker$bars")) {
						p_plot->arrow_properties.head = BOTH_HEADS;
						rC.CToken++;
						if(rC.IsANumber(rC.CToken) || rC.TypeDdv(rC.CToken) == INTGR || rC.TypeDdv(rC.CToken) == CMPLX)
							p_plot->arrow_properties.head_length = rC.RealExpression();
					}
				}
				if(p_plot->plot_style == VECTOR) {
					int stored_token = rC.CToken;
					if(!set_lpstyle) {
						default_arrow_style(&(p_plot->arrow_properties));
						if(prefer_line_styles)
							lp_use_properties(&(p_plot->arrow_properties.lp_properties), line_num+1);
						else
							load_linetype(&(p_plot->arrow_properties.lp_properties), line_num+1);
					}
					arrow_parse(rC, &p_plot->arrow_properties, true);
					if(stored_token != rC.CToken) {
						if(set_lpstyle) {
							duplication = true;
							break;
						}
						else {
							set_lpstyle = true;
							continue;
						}
					}
				}

#ifdef EAM_OBJECTS
				/* pick up the special 'units' keyword the 'ellipses' style allows */
				if(p_plot->plot_style == ELLIPSES) {
					int stored_token = rC.CToken;
					if(!set_ellipseaxes_units)
						p_plot->ellipseaxes_units = DefaultEllipse.o.ellipse.type;
					if(rC.AlmostEq("unit$s")) {
						rC.CToken++;
						if(rC.Eq("xy")) {
							p_plot->ellipseaxes_units = ELLIPSEAXES_XY;
						}
						else if(rC.Eq("xx")) {
							p_plot->ellipseaxes_units = ELLIPSEAXES_XX;
						}
						else if(rC.Eq("yy")) {
							p_plot->ellipseaxes_units = ELLIPSEAXES_YY;
						}
						else {
							IntErrorCurToken("expecting 'xy', 'xx' or 'yy'");
						}
						rC.CToken++;
					}
					if(stored_token != rC.CToken) {
						if(set_ellipseaxes_units) {
							duplication = true;
							break;
						}
						else {
							set_ellipseaxes_units = true;
							continue;
						}
					}
				}
#endif
				// Most plot styles accept line and point properties
				// but do not want font or text properties
				if(p_plot->plot_style != LABELPOINTS) {
					int stored_token = rC.CToken;
					lp_style_type lp; // = DEFAULT_LP_STYLE_TYPE;
					int new_lt = 0;
					lp.l_type = line_num;
					lp.p_type = line_num;
					lp.d_type = line_num;
					p_plot->base_linetype = line_num;
					/* user may prefer explicit line styles */
					if(prefer_line_styles)
						lp_use_properties(&lp, line_num+1);
					else
						load_linetype(&lp, line_num+1);
					if(p_plot->plot_style == BOXPLOT)
						lp.p_type = boxplot_opts.pointtype;
					new_lt = LpParse(rC, lp, LP_ADHOC, (p_plot->plot_style & PLOT_STYLE_HAS_POINT) ? true : false);
					if(stored_token != rC.CToken) {
						if(set_lpstyle) {
							duplication = true;
							break;
						}
						else {
							p_plot->lp_properties = lp;
							set_lpstyle = true;
							if(new_lt)
								p_plot->base_linetype = new_lt - 1;
							if(p_plot->lp_properties.p_type != PT_CHARACTER)
								continue;
						}
					}
				}
				//
				// Labels can have font and text property info as plot options
				// In any case we must allocate one instance of the text style
				// that all labels in the plot will share.
				//
				if((p_plot->plot_style == LABELPOINTS) || (p_plot->plot_style & PLOT_STYLE_HAS_POINT && p_plot->lp_properties.p_type == PT_CHARACTER)) {
					int stored_token = rC.CToken;
					if(p_plot->labels == NULL) {
						p_plot->labels = new_text_label(-1);
						p_plot->labels->pos = CENTRE;
						p_plot->labels->layer = LAYER_PLOTLABELS;
					}
					ParseLabelOptions(rC, p_plot->labels, 2);
					if(stored_token != rC.CToken) {
						if(set_labelstyle) {
							duplication = true;
							break;
						}
						else {
							set_labelstyle = true;
							continue;
						}
					}
					else if(p_plot->lp_properties.p_type == PT_CHARACTER) {
						if(rC.Eq(","))
							break;
						else
							continue;
					}
				}
				// Some plots have a fill style as well
				if(p_plot->plot_style & PLOT_STYLE_HAS_FILL) {
					int stored_token = rC.CToken;
					if(rC.Eq("fs") || rC.AlmostEq("fill$style")) {
						rC.ParseFillStyle(&p_plot->fill_properties, DefaultFillStyle.fillstyle,
						    DefaultFillStyle.filldensity, pattern_num, DefaultFillStyle.border_color);
						if(p_plot->plot_style == FILLEDCURVES && p_plot->fill_properties.fillstyle == FS_EMPTY)
							p_plot->fill_properties.fillstyle = FS_SOLID;
						set_fillstyle = true;
					}
					if(rC.Eq("fc") || rC.AlmostEq("fillc$olor")) {
						rC.ParseColorSpec(&p_plot->lp_properties.pm3d_color, TC_VARIABLE);
					}
					if(stored_token != rC.CToken)
						continue;
				}
				break; /* unknown option */
			}
			if(duplication)
				IntErrorCurToken("duplicated or contradicting arguments in plot options");
			// set default values for title if this has not been specified
			p_plot->title_is_filename = false;
			if(!set_title) {
				p_plot->title_no_enhanced = true; /* filename or function cannot be enhanced */
				if(key->auto_titles == FILENAME_KEYTITLES) {
					rC.MCapture(&(p_plot->title), start_token, end_token);
					if(InParametric)
						xtitle = p_plot->title;
					p_plot->title_is_filename = true;
				}
				else if(xtitle != NULL)
					xtitle[0] = '\0';
			}

			/* Vectors will be drawn using linetype from arrow style, so we
			 * copy this to overall plot linetype so that the key sample matches */
			if(p_plot->plot_style == VECTOR) {
				if(!set_lpstyle) {
					if(prefer_line_styles)
						lp_use_properties(&(p_plot->arrow_properties.lp_properties), line_num+1);
					else
						load_linetype(&(p_plot->arrow_properties.lp_properties), line_num+1);
					arrow_parse(rC, &p_plot->arrow_properties, true);
				}
				p_plot->lp_properties = p_plot->arrow_properties.lp_properties;
				set_lpstyle = true;
			}
			/* No line/point style given. As lp_parse also supplies
			 * the defaults for linewidth and pointsize, call it now
			 * to define them. */
			if(!set_lpstyle) {
				p_plot->lp_properties.l_type = line_num;
				p_plot->lp_properties.l_width = 1.0;
				p_plot->lp_properties.p_type = line_num;
				p_plot->lp_properties.d_type = line_num;
				p_plot->lp_properties.p_size = PtSz;
				/* user may prefer explicit line styles */
				if(prefer_line_styles)
					lp_use_properties(&p_plot->lp_properties, line_num+1);
				else
					load_linetype(&p_plot->lp_properties, line_num+1);
				if(p_plot->plot_style == BOXPLOT)
					p_plot->lp_properties.p_type = boxplot_opts.pointtype;
				LpParse(rC, p_plot->lp_properties, LP_ADHOC, (p_plot->plot_style & PLOT_STYLE_HAS_POINT) ? true : false);
			}
			// Some low-level routines expect to find the pointflag attribute 
			// in lp_properties (they don't have access to the full header.   
			if(p_plot->plot_style & PLOT_STYLE_HAS_POINT)
				p_plot->lp_properties.flags |= LP_SHOW_POINTS;
			// Rule out incompatible line/point/style options 
			if(p_plot->plot_type == FUNC) {
				if((p_plot->plot_style & PLOT_STYLE_HAS_POINT) && (p_plot->lp_properties.p_size == PTSZ_VARIABLE))
					p_plot->lp_properties.p_size = 1;
			}
			if(IsPolar)
				switch(p_plot->plot_style) {
					case LINES:
					case POINTSTYLE:
					case IMPULSES:
					case LINESPOINTS:
					case DOTS:
					case VECTOR:
					case FILLEDCURVES:
					case LABELPOINTS:
					case CIRCLES:
					case YERRORBARS:
					case YERRORLINES:
					    break;
					default:
					    GpGg.IntErrorNoCaret("This plot style is not available in polar mode");
				}

			/* If we got this far without initializing the fill style, do it now */
			if(p_plot->plot_style & PLOT_STYLE_HAS_FILL) {
				if(!set_fillstyle)
					rC.ParseFillStyle(&p_plot->fill_properties, DefaultFillStyle.fillstyle, DefaultFillStyle.filldensity, pattern_num, DefaultFillStyle.border_color);
				if((p_plot->fill_properties.fillstyle == FS_PATTERN) ||(p_plot->fill_properties.fillstyle == FS_TRANSPARENT_PATTERN))
					pattern_num = p_plot->fill_properties.fillpattern + 1;
				if(p_plot->plot_style == FILLEDCURVES && p_plot->fill_properties.fillstyle == FS_EMPTY)
					p_plot->fill_properties.fillstyle = FS_SOLID;
			}
			p_plot->x_axis = XAxis;
			p_plot->y_axis = YAxis;
			// If we got this far without initializing the character font, do it now
			if(p_plot->plot_style & PLOT_STYLE_HAS_POINT && p_plot->lp_properties.p_type == PT_CHARACTER) {
				if(p_plot->labels == NULL) {
					p_plot->labels = new_text_label(-1);
					p_plot->labels->pos = CENTRE;
					ParseLabelOptions(rC, p_plot->labels, 2);
				}
			}
			// If we got this far without initializing the label list, do it now
			if(p_plot->plot_style == LABELPOINTS) {
				if(p_plot->labels == NULL) {
					p_plot->labels = new_text_label(-1);
					p_plot->labels->pos = CENTRE;
					p_plot->labels->layer = LAYER_PLOTLABELS;
				}
				p_plot->labels->place.scalex = (XAxis == SECOND_X_AXIS) ? second_axes : first_axes;
				p_plot->labels->place.scaley = (YAxis == SECOND_Y_AXIS) ? second_axes : first_axes;
				// Needed for variable color - June 2010
				p_plot->lp_properties.pm3d_color = p_plot->labels->textcolor;
				if(p_plot->labels->textcolor.type == TC_VARIABLE)
					p_plot->lp_properties.l_type = LT_COLORFROMCOLUMN;
				//
				// We want to trigger the variable color mechanism even if
				// there was no 'textcolor variable/palette/rgb var' ,
				// but there was a 'point linecolor variable/palette/rgb var'.
				//
				if((p_plot->labels->lp_properties.flags & LP_SHOW_POINTS) && !oneof2(p_plot->labels->textcolor.type, TC_Z, TC_VARIABLE) &&
				    (p_plot->labels->textcolor.type != TC_RGB || p_plot->labels->textcolor.value >= 0)) {
					if((p_plot->labels->lp_properties.pm3d_color.type == TC_RGB) && (p_plot->labels->lp_properties.pm3d_color.value < 0)) {
						p_plot->lp_properties.pm3d_color = p_plot->labels->lp_properties.pm3d_color;
					}
					if(p_plot->labels->lp_properties.pm3d_color.type == TC_Z)
						p_plot->lp_properties.pm3d_color.type = TC_Z;
					if(p_plot->labels->lp_properties.l_type == LT_COLORFROMCOLUMN)
						p_plot->lp_properties.l_type = LT_COLORFROMCOLUMN;
				}
			}
			// Initialize the label list in case the BOXPLOT style needs it to store factors
			if(p_plot->plot_style == BOXPLOT) {
				SETIFZ(p_plot->labels, new_text_label(-1));
				// We only use the list to store strings, so this is all we need here.
			}
			// Initialize histogram data structure
			if(p_plot->plot_style == HISTOGRAMS) {
				if(GetX().Flags & GpAxis::fLog)
					IntErrorCurToken("Log scale on X is incompatible with histogram plots\n");
				if(oneof2(histogram_opts.type, HT_STACKED_IN_LAYERS, HT_STACKED_IN_TOWERS) && GetY().Flags & GpAxis::fLog)
					IntErrorCurToken("Log scale on Y is incompatible with stacked histogram plot\n");
				p_plot->histogram_sequence = ++histogram_sequence;
				// Current histogram always goes at the front of the list
				if(p_plot->histogram_sequence == 0) {
					p_plot->histogram = (histogram_style *)malloc(sizeof(histogram_style));
					init_histogram(p_plot->histogram, &histogram_title);
					p_plot->histogram->start = newhist_start;
					p_plot->histogram->startcolor = newhist_color;
					p_plot->histogram->startpattern = newhist_pattern;
				}
				else {
					p_plot->histogram = histogram_opts.next;
					p_plot->histogram->clustersize++;
				}
				/* Normally each histogram gets a new set of colors, but in */
				/* 'newhistogram' you can force a starting color instead.   */
				if(!set_lpstyle && p_plot->histogram->startcolor != LT_UNDEFINED)
					load_linetype(&p_plot->lp_properties, p_plot->histogram_sequence + p_plot->histogram->startcolor);
				if(p_plot->histogram->startpattern != LT_UNDEFINED)
					p_plot->fill_properties.fillpattern = p_plot->histogram_sequence + p_plot->histogram->startpattern;
			}
			/* Styles that use palette */

			/* we can now do some checks that we deferred earlier */

			if(p_plot->plot_type == DATA) {
				if(specs < 0) {
					/* Error check to handle missing or unreadable file */
					++line_num;
					p_plot->plot_type = NODATA;
					goto SKIPPED_EMPTY_FILE;
				}
				/* Reset flags to auto-scale X axis to contents of data set */
				if(!(uses_axis[XAxis] & USES_AXIS_FOR_DATA) && GetX().AutoScale) {
					GpAxis * scaling_axis =  (XAxis == SECOND_X_AXIS && !GetX().P_LinkToPrmr) ? &AxA[SECOND_X_AXIS] : &AxA[FIRST_X_AXIS];
					scaling_axis->SetAutoscale(GPVL, -GPVL);
				}
				if(GetX().datatype == DT_TIMEDATE) {
					if(specs < 2)
						IntErrorCurToken("Need full using spec for x time data");
				}
				if(GetY().datatype == DT_TIMEDATE) {
					if(specs < 1)
						IntErrorCurToken("Need using spec for y time data");
				}
				// need other cols, but I'm lazy
				GpDf.df_axis[0] = XAxis;
				GpDf.df_axis[1] = YAxis;
				// separate record of datafile and func
				uses_axis[XAxis] |= USES_AXIS_FOR_DATA;
				uses_axis[YAxis] |= USES_AXIS_FOR_DATA;
			}
			else if(!IsParametric || !InParametric) {
				// for x part of a parametric function, axes are possibly wrong
				// separate record of data and func
				uses_axis[XAxis] |= USES_AXIS_FOR_FUNC;
				uses_axis[YAxis] |= USES_AXIS_FOR_FUNC;
			}
			// don't increment the default line/point properties if p_plot is an image
			if(!InParametric && !oneof3(p_plot->plot_style, IMAGE, RGBIMAGE, RGBA_IMAGE)) {
				++line_num;
			}
			if(p_plot->plot_type == DATA) {
				// get_data() will update the ranges of autoscaled axes, but some
				// plot modes, e.g. 'smooth cnorm' and 'boxplot' with nooutliers,
				// do not want all the points included in autoscaling.  Save the 
				// current autoscaled ranges here so we can restore them later. 
				SaveAutoscaledRanges(&AxA[p_plot->x_axis], &AxA[p_plot->y_axis]);
				// actually get the data now 
				if(GetData(rC, p_plot) == 0) {
					if(!forever_iteration(rC.P.P_PlotIterator))
						int_warn(NO_CARET, "Skipping data file with no valid points");
					p_plot->plot_type = NODATA;
					goto SKIPPED_EMPTY_FILE;
				}
#ifdef SMOOTH_BINS_OPTION
				// If we are to bin the data, do that first 
				if(p_plot->plot_smooth == SMOOTH_BINS) {
					make_bins(p_plot, nbins, binlow, binhigh);
				}
#endif
				// Fiddle the auto-scaling data for specific plot styles
				if(p_plot->plot_style == HISTOGRAMS)
					HistogramRangeFiddling(p_plot);
				if(p_plot->plot_style == BOXES) {
					//
					// Autoscaling of box plots cuts off half of the box on each end.
					// Add a half-boxwidth to the range in this case.  EAM Aug 2007
					//
					//box_range_fiddling(p_plot);
					//static void box_range_fiddling(CurvePoints * plot)
					const int pt_idx = p_plot->p_count - 1;
					if(pt_idx) {
						if(AxA[p_plot->x_axis].AutoScale & AUTOSCALE_MIN) {
							if(p_plot->points[0].type != UNDEFINED && p_plot->points[1].type != UNDEFINED) {
								double xlow = p_plot->points[0].x - (p_plot->points[1].x - p_plot->points[0].x) / 2.;
								xlow = DelogValue((AXIS_INDEX)p_plot->x_axis, xlow);
								SETMIN(AxA[p_plot->x_axis].Range.low, xlow);
							}
						}
						if(AxA[p_plot->x_axis].AutoScale & AUTOSCALE_MAX) {
							if(p_plot->points[pt_idx].type != UNDEFINED && p_plot->points[pt_idx-1].type != UNDEFINED) {
								double xhigh = p_plot->points[pt_idx].x + (p_plot->points[pt_idx].x - p_plot->points[pt_idx-1].x) / 2.;
								xhigh = DelogValue((AXIS_INDEX)p_plot->x_axis, xhigh);
								SETMAX(AxA[p_plot->x_axis].Range.upp, xhigh);
							}
						}
					}
				}
				if(p_plot->plot_style == BOXPLOT)
					BoxPlotRangeFiddling(p_plot);
				if(p_plot->plot_style == IMPULSES)
					impulse_range_fiddling(p_plot);
				if(oneof2(p_plot->plot_style, RGBIMAGE, RGBA_IMAGE)) {
					GetCB().SetAutoscale(0.0, 255.0);
				}
				if(p_plot->plot_style == TABLESTYLE) {
					GetY().Range.SetVal(not_a_number());
				}
				if(IsPolar) {
					PolarRangeFiddling(p_plot);
				}
				// sort
				switch(p_plot->plot_smooth) {
					// sort and average, if the style requires
					case SMOOTH_UNIQUE:
					case SMOOTH_FREQUENCY:
					case SMOOTH_CUMULATIVE:
					case SMOOTH_CUMULATIVE_NORMALISED:
					case SMOOTH_CSPLINES:
					case SMOOTH_ACSPLINES:
					case SMOOTH_SBEZIER:
					case SMOOTH_MONOTONE_CSPLINE:
					    sort_points(p_plot);
					    CpImplode(p_plot);
					case SMOOTH_NONE:
					case SMOOTH_BEZIER:
					case SMOOTH_KDENSITY:
					default:
					    break;
				}
				switch(p_plot->plot_smooth) {
					// create new data set by evaluation of interpolation routines
					case SMOOTH_UNWRAP:
					    gen_interp_unwrap(p_plot);
					    break;
					case SMOOTH_FREQUENCY:
					case SMOOTH_CUMULATIVE:
					    gen_interp_frequency(p_plot);
					    break;
					case SMOOTH_CUMULATIVE_NORMALISED:
					    RestoreAutoscaledRanges(NULL, &AxA[p_plot->y_axis]);
					    gen_interp_frequency(p_plot);
					    break;
					case SMOOTH_CSPLINES:
					case SMOOTH_ACSPLINES:
					case SMOOTH_BEZIER:
					case SMOOTH_SBEZIER:
					    GenInterp(p_plot);
					    break;
					case SMOOTH_KDENSITY:
					    GenInterp(p_plot);
					    Ev.FillGpValFloat("GPVAL_KDENSITY_BANDWIDTH",
					    fabs(p_plot->smooth_parameter));
					    break;
					case SMOOTH_MONOTONE_CSPLINE:
					    McsInterp(p_plot);
					    break;
					case SMOOTH_NONE:
					case SMOOTH_UNIQUE:
					default:
					    break;
				}

				/* Images are defined by a grid representing centers of pixels.
				 * Compensate for extent of the image so `set autoscale fix`
				 * uses outer edges of outer pixels in axes adjustment.
				 */
				if(oneof3(p_plot->plot_style, IMAGE, RGBIMAGE, RGBA_IMAGE)) {
					p_plot->image_properties.type = IC_PALETTE;
					ProcessImage(term, p_plot, IMG_UPDATE_AXES);
				}
			}
SKIPPED_EMPTY_FILE:
			// Note position in command line for second pass
			p_plot->Token = rC.CToken;
			tp_ptr = &(p_plot->P_Next);
		}
		if(InParametric) {
			if(rC.Eq(",")) {
				rC.CToken++;
				continue;
			}
			else
				break;
		}
		// Iterate-over-plot mechanism 
		if(empty_iteration(rC.P.P_PlotIterator) && p_plot) {
			p_plot->plot_type = NODATA;
		}
		else if(forever_iteration(rC.P.P_PlotIterator) && (p_plot->plot_type == NODATA)) {
			highest_iteration = rC.P.P_PlotIterator->iteration_current;
		}
		else if(forever_iteration(rC.P.P_PlotIterator) && (p_plot->plot_type == FUNC)) {
			GpGg.IntErrorNoCaret("unbounded iteration in function plot");
		}
		else if(next_iteration(rC.P.P_PlotIterator)) {
			rC.CToken = start_token;
			highest_iteration = rC.P.P_PlotIterator->iteration_current;
			continue;
		}
		rC.P.CleanupPlotIterator();
		if(rC.Eq(",")) {
			rC.CToken++;
			rC.P.P_PlotIterator = rC.CheckForIteration();
		}
		else
			break;
	}
	if(IsParametric && InParametric)
		GpGg.IntErrorNoCaret("parametric function not fully specified");

/*** Second Pass: Evaluate the functions ***/
	/*
	 * Everything is defined now, except the function data. We expect
	 * no syntax errors, etc, since the above parsed it all. This
	 * makes the code below simpler. If y is autoscaled, the yrange
	 * may still change.  we stored last token of each plot, so we
	 * dont need to do everything again */

	/* parametric or polar fns can still affect x ranges */
	if(!IsParametric && !IsPolar) {
#ifdef NONLINEAR_AXES
		/* Autoscaling on x used the primary axis, so we must transform */
		/* the range that was found back onto the visible X axis.	*/
		/* If we were _not_ autoscaling, well I guess it doesn't hurt.	*/
		/* NB: clone_linked_axes() is overkill.				*/
		if(AxA[FIRST_X_AXIS].P_LinkToPrmr) {
			clone_linked_axes(GpGg[FIRST_X_AXIS].P_LinkToPrmr, &GpGg[FIRST_X_AXIS]);
		}
#endif

		/* If we were expecting to autoscale on X but found no usable
		 * points in the data files, then the axis limits are still sitting
		 * at +/- GPVL.  The default range for bare functions is [-10:10].
		 * Or we could give up and fall through to "x range invalid".
		 */
		if(some_functions && uses_axis[FIRST_X_AXIS])
			if(AxA[FIRST_X_AXIS].Range.upp == -GPVL || AxA[FIRST_X_AXIS].Range.low == GPVL) {
				AxA[FIRST_X_AXIS].Range.Set(-10.0, 10.0);
			}
		// check that xmin -> xmax is not too small
		AxisCheckedExtendEmptyRange(FIRST_X_AXIS, "x range is invalid");
		if(AxA[SECOND_X_AXIS].P_LinkToPrmr) {
			clone_linked_axes(AxA[SECOND_X_AXIS].P_LinkToPrmr, &AxA[SECOND_X_AXIS]);
			// FIXME: This obsoletes OUTRANGE/INRANGE for secondary axis data
		}
		else if(uses_axis[SECOND_X_AXIS] & USES_AXIS_FOR_DATA) {
			// check that x2min -> x2max is not too small
			AxisCheckedExtendEmptyRange(SECOND_X_AXIS, "x2 range is invalid");
		}
		else if(AxA[SECOND_X_AXIS].AutoScale) {
			// copy x1's range
			AxA[SECOND_X_AXIS].SetAutoscale(AxA[FIRST_X_AXIS].Range.low, AxA[FIRST_X_AXIS].Range.upp);
		}
	}
	if(some_functions) {
		// call the controlled variable t, since x_min can also mean smallest x
		double t_step = 0.0;
		//double t_min = 0.0, t_max = 0.0, 
		RealRange t_range;
		t_range.SetVal(0.0);
		if(IsParametric || IsPolar) {
			if(!(uses_axis[FIRST_X_AXIS] & USES_AXIS_FOR_DATA)) {
				AxA[FIRST_X_AXIS].SetAutoscale(GPVL, -GPVL); // these have not yet been set to full width 
			}
			if(!(uses_axis[SECOND_X_AXIS] & USES_AXIS_FOR_DATA)) {
				AxA[SECOND_X_AXIS].SetAutoscale(GPVL, -GPVL);
			}
		}
		// FIXME HBB 20000430: here and elsewhere, the code explicitly
		// assumes that the dummy variables (t, u, v) cannot possibly
		// be logscaled in parametric or polar mode. Does this *really* hold? 
		if(IsParametric || IsPolar) {
			t_range = AxA[T_AXIS].Range;
			t_step = t_range.GetDistance() / (Samples1 - 1);
		}
		/* else we'll do it on each plot (see below) */

		tp_ptr = &(P_FirstPlot);
		plot_num = 0;
		p_plot = P_FirstPlot;
		rC.CToken = begin_token; // start over
		rC.P.P_PlotIterator = rC.CheckForIteration();
		// Read through functions
		while(true) {
			if(!InParametric && !was_definition)
				start_token = rC.CToken;
			if(rC.IsDefinition()) {
				rC.Define();
				if(rC.Eq(","))
					rC.CToken++;
				was_definition = true;
				continue;
			}
			else {
				AtType * at_ptr;
				char * name_str;
				int    sample_range_token;
				was_definition = false;
				// Forgive trailing comma on a multi-element plot command 
				if(rC.EndOfCommand() || !p_plot) {
					int_warn(rC.CToken, "ignoring trailing comma in plot command");
					break;
				}
				// HBB 20000820: now globals in 'axis.c' 
				XAxis = (AXIS_INDEX)p_plot->x_axis;
				YAxis = (AXIS_INDEX)p_plot->y_axis;
				plot_num++;
				// Check for a sampling range. 
				// Only relevant to function plots, and only needed in second pass. 
				InitSampleRange(&GetX());
				sample_range_token = ParseRange(SAMPLE_AXIS, rC);
				rC.P_DummyFunc = &plot_func;
				if(rC.AlmostEq("newhist$ogram")) {
					name_str = ""; // Make sure this isn't interpreted as a function
				}
				else {
					// Allow replacement of the dummy variable in a function
					if(sample_range_token > 0)
						rC.CopyStr(rC.P.CDummyVar[0], sample_range_token, MAX_ID_LEN);
					else if(sample_range_token < 0)
						strcpy(rC.P.CDummyVar[0], rC.P.SetDummyVar[0]);
					else
						strcpy(rC.P.CDummyVar[0], orig_dummy_var);
					// WARNING: do NOT free name_str
					name_str = rC.P.StringOrExpress(rC, &at_ptr);
				}
				if(!name_str) { /* function to plot */
					if(IsParametric) { /* toggle parametric axes */
						InParametric = !InParametric;
					}
					plot_func.at = at_ptr;
					if(!IsParametric && !IsPolar) {
						t_range = AxA[SAMPLE_AXIS].Range;
#ifdef NONLINEAR_AXES
						if(AxA[SAMPLE_AXIS].P_LinkToPrmr) {
							GpAxis * primary = AxA[SAMPLE_AXIS].P_LinkToPrmr;
							t_range.Set(primary->EvalLinkFunction(t_range.low), primary->EvalLinkFunction(t_range.upp));
							FPRINTF((stderr, "sample range on primary axis: %g %g\n", t_min, t_max));
						}
						else
#endif
						// FIXME: What if SAMPLE_AXIS is not x_axis?
						GetX().UnlogInterval(t_range, true);
						t_step = t_range.GetDistance() / (Samples1 - 1);
					}
					for(i = 0; i < Samples1; i++) {
						double x, temp;
						t_value a;
						double t = t_range.low + i * t_step;
#ifdef NONLINEAR_AXES
						if(AxA[SAMPLE_AXIS].P_LinkToPrmr) {
							GpAxis * p_vis = AxA[SAMPLE_AXIS].P_LinkToPrmr->P_LinkToScnd;
							t = p_vis->EvalLinkFunction(t_range.low + i * t_step);
						}
						else
#endif
						t = t_range.low + i * t_step;
						// Zero is often a special point in a function domain.	
						// Make sure we don't miss it due to round-off error.	
						// NB: This is a stricter test than CheckZero().        
						if((fabs(t) < 1.e-9) && (fabs(t_step) > 1.e-6))
							t = 0.0;
						// parametric/polar => NOT a log quantity
						x = (!IsParametric && !IsPolar) ? DelogValue(XAxis, t) : t;
						plot_func.dummy_values[0].SetComplex(x, 0.0);
						Ev.EvaluateAt(plot_func.at, &a);
						if(Ev.undefined || !IsZero(imag(&a))) {
							p_plot->points[i].type = UNDEFINED;
							continue;
						}
						else {
							// Jan 2010 - initialize all fields!
							memzero(&p_plot->points[i], sizeof(GpCoordinate));
						}
						temp = a.Real();
						/* width of box not specified */
						p_plot->points[i].z = -1.0;
						/* for the moment */
						p_plot->points[i].type = INRANGE;
						if(IsParametric) {
							/* we cannot do range-checking now, since for the x function we did not know which axes we were using
							 * DO NOT TAKE LOGS YET - do it in parametric_fixup
							 */
							/* ignored, actually... */
							p_plot->points[i].x = t;
							p_plot->points[i].y = temp;
							if(boxwidth >= 0 && boxwidth_is_absolute)
								p_plot->points[i].z = 0;
						}
						else if(IsPolar) {
							double y;
							double phi = x;
							if(temp > GetR().Range.upp) {
								if(!GetR().SetAutoscaleMax(temp))
									p_plot->points[i].type = OUTRANGE;
							}
							if(temp < GetR().Range.low) {
								GetR().SetAutoscaleMin(0.0);
							}
							if(GetR().Flags & GpAxis::fLog) {
								temp = DoLog(POLAR_AXIS, temp) - DoLog(POLAR_AXIS, GetR().Range.low);
#ifdef NONLINEAR_AXES
							}
							else if(GetR().P_LinkToPrmr) {
								GpAxis * shadow = GetR().P_LinkToPrmr;
								temp = shadow->EvalLinkFunction(temp) - shadow->Range.low;
#endif
							}
							else if(!(GetR().AutoScale & AUTOSCALE_MIN))
								temp -= GetR().Range.low;
							y = temp * sin(phi * Ang2Rad);
							x = temp * cos(phi * Ang2Rad);
							if((p_plot->plot_style == FILLEDCURVES) && (p_plot->filledcurves_options.closeto == FILLEDCURVES_ATR)) {
								double temp = p_plot->filledcurves_options.at;
								temp = LogValue(POLAR_AXIS, temp) - LogValue(POLAR_AXIS, GetR().Range.low);
								const double yhigh = temp * sin(phi * Ang2Rad);
								const double xhigh = temp * cos(phi * Ang2Rad);
								STORE_WITH_LOG_AND_UPDATE_RANGE(p_plot->points[i].xhigh, xhigh, p_plot->points[i].type, XAxis, p_plot->noautoscale, NOOP, goto come_here_if_undefined);
								STORE_WITH_LOG_AND_UPDATE_RANGE(p_plot->points[i].yhigh, yhigh, p_plot->points[i].type, YAxis, p_plot->noautoscale, NOOP, goto come_here_if_undefined);
							}
							STORE_WITH_LOG_AND_UPDATE_RANGE(p_plot->points[i].x, x, p_plot->points[i].type, XAxis, p_plot->noautoscale, NOOP, goto come_here_if_undefined);
							STORE_WITH_LOG_AND_UPDATE_RANGE(p_plot->points[i].y, y, p_plot->points[i].type, YAxis, p_plot->noautoscale, NOOP, goto come_here_if_undefined);
						}
						else { // neither parametric or polar
							// logscale ? log(x) : x
							p_plot->points[i].x = t;
							// A sampled function can only be OUTRANGE if it has a private range
							if(sample_range_token != 0) {
								double xx = DelogValue(XAxis, t);
								if(!AxA[p_plot->x_axis].InRange(xx))
									p_plot->points[i].type = OUTRANGE;
							}
							// For boxes [only] check use of boxwidth
							if((p_plot->plot_style == BOXES) &&  (boxwidth >= 0 && boxwidth_is_absolute)) {
								RealRange xrange;
								coord_type dmy_type = INRANGE;
								p_plot->points[i].z = 0;
								if(AxA[p_plot->x_axis].Flags & GpAxis::fLog) {
									const double base = AxA[p_plot->x_axis].base;
									xrange.Set(x * pow(base, -boxwidth/2.0), x * pow(base, boxwidth/2.0));
								}
								else {
									xrange.Set(x - boxwidth/2, x + boxwidth/2);
								}
								STORE_WITH_LOG_AND_UPDATE_RANGE(p_plot->points[i].xlow,  xrange.low, dmy_type, XAxis, p_plot->noautoscale, NOOP, NOOP);
								dmy_type = INRANGE;
								STORE_WITH_LOG_AND_UPDATE_RANGE(p_plot->points[i].xhigh, xrange.upp, dmy_type, XAxis, p_plot->noautoscale, NOOP, NOOP);
							}
							if(p_plot->plot_style == FILLEDCURVES) {
								p_plot->points[i].xhigh = p_plot->points[i].x;
								STORE_WITH_LOG_AND_UPDATE_RANGE(p_plot->points[i].yhigh, p_plot->filledcurves_options.at, p_plot->points[i].type, YAxis, true, NOOP, NOOP);
							}
							// Fill in additional fields needed to draw a circle
#ifdef EAM_OBJECTS
							if(p_plot->plot_style == CIRCLES) {
								p_plot->points[i].z = DEFAULT_RADIUS;
								p_plot->points[i].ylow = 0;
								p_plot->points[i].xhigh = 360;
							}
							else if(p_plot->plot_style == ELLIPSES) {
								p_plot->points[i].z = DEFAULT_RADIUS;
								p_plot->points[i].ylow = DefaultEllipse.o.ellipse.orientation;
							}
#endif
							STORE_WITH_LOG_AND_UPDATE_RANGE(p_plot->points[i].y, temp, p_plot->points[i].type,
								InParametric ? XAxis : YAxis, p_plot->noautoscale, NOOP, goto come_here_if_undefined);

							/* could not use a continue in this case */
come_here_if_undefined:
							; /* ansi requires a statement after a label */
						}
					}
					p_plot->p_count = i; // Samples1
				}
				// skip all modifers func / whole of data plots
				rC.CToken = p_plot->Token;
				// used below
				tp_ptr = &(p_plot->P_Next);
				p_plot = p_plot->P_Next;
			}
			// Jan 2014: Earlier 2.6 versions missed this case,
			//           breaking iteration over parametric plots
			if(InParametric) {
				if(rC.Eq(",")) {
					rC.CToken++;
					continue;
				}
			}
			// Iterate-over-plot mechanism
			if(next_iteration(rC.P.P_PlotIterator)) {
				if(rC.P.P_PlotIterator->iteration_current <= highest_iteration) {
					rC.CToken = start_token;
					continue;
				}
			}
			rC.P.CleanupPlotIterator();
			if(rC.Eq(",")) {
				rC.CToken++;
				rC.P.P_PlotIterator = rC.CheckForIteration();
			}
			else
				break;
		}
		// when step debugging, set breakpoint here to get through
		// the 'read function' loop above quickly
		if(IsParametric) {
			// Now actually fix the plot pairs to be single plots also fixes up polar&&parametric fn plots
			ParametricFixup(P_FirstPlot, &plot_num);
			// we omitted earlier check for range too small
			AxisCheckedExtendEmptyRange(FIRST_X_AXIS, NULL);
			if(uses_axis[SECOND_X_AXIS])
				AxisCheckedExtendEmptyRange(SECOND_X_AXIS, NULL);
		}
		// This is the earliest that polar autoscaling can be done for function plots
		if(IsPolar) {
			PolarRangeFiddling(P_FirstPlot);
		}
	} /* some_functions */
	//
	// if first_plot is NULL, we have no functions or data at all. This can
	// happen if you type "plot x=5", since x=5 is a variable assignment.
	//
	if(!plot_num || !P_FirstPlot) {
		IntErrorCurToken("no functions or data to plot");
	}
	if(!uses_axis[FIRST_X_AXIS] && !uses_axis[SECOND_X_AXIS])
		if(P_FirstPlot->plot_type == NODATA)
			GpGg.IntErrorNoCaret("No data in plot");
	if(uses_axis[FIRST_X_AXIS]) {
		if(AxA[FIRST_X_AXIS].IsRangeUndef())
			GpGg.IntErrorNoCaret("all points undefined!");
		RevertAndUnlogRange(FIRST_X_AXIS);
	}
	if(uses_axis[SECOND_X_AXIS]) {
		if(AxA[SECOND_X_AXIS].IsRangeUndef())
			GpGg.IntErrorNoCaret("all points undefined!");
		RevertAndUnlogRange(SECOND_X_AXIS);
	}
	else {
		// FIXME: If this triggers, doesn't it clobber linked axes? 
		//        Maybe we should just call clone_linked_axes()?    
		assert(uses_axis[FIRST_X_AXIS]);
		AxA[SECOND_X_AXIS].SetAutoscale(AxA[FIRST_X_AXIS].Range.low, AxA[FIRST_X_AXIS].Range.upp);
		if(!AxA[SECOND_X_AXIS].AutoScale)
			RevertAndUnlogRange(SECOND_X_AXIS);
	}
	if(!uses_axis[FIRST_X_AXIS]) {
		assert(uses_axis[SECOND_X_AXIS]);
		AxA[FIRST_X_AXIS].SetAutoscale(AxA[SECOND_X_AXIS].Range.low, AxA[SECOND_X_AXIS].Range.upp);
	}
#ifdef NONLINEAR_AXES
	if(uses_axis[FIRST_Y_AXIS] && AxA[FIRST_Y_AXIS].P_LinkToPrmr) {
		clone_linked_axes(AxA[FIRST_Y_AXIS].P_LinkToPrmr, &AxA[FIRST_Y_AXIS]);
	}
	else
#endif
	if(uses_axis[FIRST_Y_AXIS]) {
		AxisCheckedExtendEmptyRange(FIRST_Y_AXIS, "all points y value undefined!");
		RevertAndUnlogRange(FIRST_Y_AXIS);
	}
	if(uses_axis[SECOND_Y_AXIS] && AxA[SECOND_Y_AXIS].P_LinkToPrmr) {
		clone_linked_axes(AxA[SECOND_Y_AXIS].P_LinkToPrmr, &AxA[SECOND_Y_AXIS]);
	}
	else if(uses_axis[SECOND_Y_AXIS]) {
		AxisCheckedExtendEmptyRange(SECOND_Y_AXIS, "all points y2 value undefined!");
		RevertAndUnlogRange(SECOND_Y_AXIS);
	}
	else {
		// else we want to copy y2 range
		assert(uses_axis[FIRST_Y_AXIS]);
		AxA[SECOND_Y_AXIS].SetAutoscale(AxA[FIRST_Y_AXIS].Range.low, AxA[FIRST_Y_AXIS].Range.upp);
		// Log() fixup is only necessary if the range was *not* copied from
		// the (already logarithmized) yrange
		if(!AxA[SECOND_Y_AXIS].AutoScale)
			RevertAndUnlogRange(SECOND_Y_AXIS);
	}
	if(!uses_axis[FIRST_Y_AXIS]) {
		assert(uses_axis[SECOND_Y_AXIS]);
		AxA[FIRST_Y_AXIS].SetAutoscale(AxA[SECOND_Y_AXIS].Range.low, AxA[SECOND_Y_AXIS].Range.upp);
	}
	/* June 2014 - This call was in boundary(), called from do_plot()
	 * but it caused problems if do_plot() itself was called for a refresh
	 * rather than for plot/replot.  So we call it here instead.
	 */
	SetCbMinMax();

	/* the following ~5 lines were moved from the end of the
	* function to here, as do_plot calles term->text, which
	* itself might process input events in mouse enhanced
	* terminals. For redrawing to work, line capturing and
	* setting the plot_num must already be done before
	* entering do_plot(). Thu Jan 27 23:56:24 2000 (joze) */
	/* if we get here, all went well, so record this line for replot */
	if(rC.PlotToken != -1) {
		// note that m_capture also frees the old replot_line
		rC.MCapture(&rC.P_ReplotLine, rC.PlotToken, rC.CToken - 1);
		rC.PlotToken = -1;
		Ev.FillGpValString("GPVAL_LAST_PLOT", rC.P_ReplotLine);
	}
	if(table_mode) {
		print_table(P_FirstPlot, plot_num);
	}
	else {
		// do_plot now uses GpGg[]
		DoPlot(P_FirstPlot, plot_num);
		// after do_plot(), GpGg[].min and .max
		// contain the plotting range actually used (rounded
		// to tic marks, not only the min/max data values)
		// --> save them now for writeback if requested
		SaveWritebackAllAxes();
		// Mark these plots as safe for quick refresh
		SetRefreshOk(E_REFRESH_OK_2D, plot_num);
	}
	Ev.UpdateGpValVariables(1); // update GPVAL_ variables available to user
}

/*
 * The hardest part of this routine is collapsing the FUNC plot types in the
 * list (which are garanteed to occur in (x,y) pairs while preserving the
 * non-FUNC type plots intact.  This means we have to work our way through
 * various lists.  Examples (hand checked): start_plot:F1->F2->NULL ==>
 * F2->NULL start_plot:F1->F2->F3->F4->F5->F6->NULL ==> F2->F4->F6->NULL
 * start_plot:F1->F2->D1->D2->F3->F4->D3->NULL ==> F2->D1->D2->F4->D3->NULL
 *
 */
//static void parametric_fixup(CurvePoints * start_plot, int * plot_num)
void GpGadgets::ParametricFixup(CurvePoints * pStartPlot, int * pPlotNum)
{
	CurvePoints * xp;
	CurvePoints * new_list = NULL;
	CurvePoints * free_list = NULL;
	CurvePoints ** last_pointer = &new_list;
	int i, curve;
	/*
	 * Ok, go through all the plots and move FUNC types together.  Note: this
	 * originally was written to look for a NULL next pointer, but gnuplot
	 * wants to be sticky in grabbing memory and the right number of items in
	 * the plot list is controlled by the pPlotNum variable.
	 *
	 * Since gnuplot wants to do this sticky business, a free_list of
	 * CurvePoints is kept and then tagged onto the end of the plot list as
	 * this seems more in the spirit of the original memory behavior than
	 * simply freeing the memory.  I'm personally not convinced this sort of
	 * concern is worth it since the time spent computing points seems to
	 * dominate any garbage collecting that might be saved here...
	 */
	new_list = xp = pStartPlot;
	curve = 0;
	while(++curve <= *pPlotNum) {
		if(xp->plot_type == FUNC) {
			/* Here's a FUNC parametric function defined as two parts. */
			CurvePoints * yp = xp->P_Next;
			--(*pPlotNum);
			assert(xp->p_count == yp->p_count);
			/* because syntax is   plot x(t), y(t) axes ..., only
			 * the y function axes are correct
			 */

			/*
			 * Go through all the points assigning the y's from xp to be
			 * the x's for yp. In polar mode, we need to check max's and
			 * min's as we go.
			 */
			for(i = 0; i < yp->p_count; ++i) {
				if(IsPolar) {
					double r = yp->points[i].y;
					double t = xp->points[i].y * Ang2Rad;
					double x, y;
					if(!(GetR().AutoScale & AUTOSCALE_MAX) && r > GetR().Range.upp)
						yp->points[i].type = OUTRANGE;
					// Fill in the GetR() min/max if autoscaling 
					// EAM FIXME: This was Bug #1323.  What about log scale? 
					if((GetR().AutoScale & AUTOSCALE_MAX) && (fabs(r) > GetR().Range.upp)) {
						GetR().Range.upp = ((GetR().max_constraint & CONSTRAINT_UPPER) && (GetR().Ub.upp < fabs(r))) ? GetR().Ub.upp : fabs(r);
					}
					if(GetR().AutoScale & AUTOSCALE_MIN) {
						GetR().Range.low = 0.0;
					}
					else {
						// store internally as if plotting r(t)-rmin 
						r -= (r > 0) ? GetR().Range.low : -GetR().Range.low;
					}
					// Convert from polar to cartesian for plotting 
					x = r * cos(t);
					y = r * sin(t);

					if(boxwidth >= 0 && boxwidth_is_absolute) {
						coord_type dmy_type = INRANGE;
						STORE_WITH_LOG_AND_UPDATE_RANGE(yp->points[i].xlow, x - boxwidth/2, dmy_type, xp->x_axis, xp->noautoscale, NOOP, NOOP);
						dmy_type = INRANGE;
						STORE_WITH_LOG_AND_UPDATE_RANGE(yp->points[i].xhigh, x + boxwidth/2, dmy_type, xp->x_axis, xp->noautoscale, NOOP, NOOP);
					}
					/* we hadn't done logs when we stored earlier */
					STORE_WITH_LOG_AND_UPDATE_RANGE(yp->points[i].x, x, yp->points[i].type, xp->x_axis, xp->noautoscale, NOOP, NOOP);
					STORE_WITH_LOG_AND_UPDATE_RANGE(yp->points[i].y, y, yp->points[i].type, xp->y_axis, xp->noautoscale, NOOP, NOOP);
				}
				else {
					double x = xp->points[i].y;
					double y = yp->points[i].y;
					if(boxwidth >= 0 && boxwidth_is_absolute) {
						coord_type dmy_type = INRANGE;
						STORE_WITH_LOG_AND_UPDATE_RANGE(yp->points[i].xlow, x - boxwidth/2, dmy_type, yp->x_axis, xp->noautoscale, NOOP, NOOP);
						dmy_type = INRANGE;
						STORE_WITH_LOG_AND_UPDATE_RANGE(yp->points[i].xhigh, x + boxwidth/2, dmy_type, yp->x_axis, xp->noautoscale, NOOP, NOOP);
					}
					STORE_WITH_LOG_AND_UPDATE_RANGE(yp->points[i].x, x, yp->points[i].type, yp->x_axis, xp->noautoscale, NOOP, NOOP);
					STORE_WITH_LOG_AND_UPDATE_RANGE(yp->points[i].y, y, yp->points[i].type, yp->y_axis, xp->noautoscale, NOOP, NOOP);
				}
			}
			// move xp to head of free list 
			xp->P_Next = free_list;
			free_list = xp;
			// append yp to new_list 
			*last_pointer = yp;
			last_pointer = &(yp->P_Next);
			xp = yp->P_Next;
		}
		else { // data plot 
			assert(*last_pointer == xp);
			last_pointer = &(xp->P_Next);
			xp = xp->P_Next;
		}
	}
	P_FirstPlot = new_list;
	*last_pointer = free_list; // Ok, stick the free list at the end of the CurvePoints plot list
}
//
// Shared by plot and splot
//
//void parse_plot_title(CurvePoints * this_plot, char * xtitle, char * ytitle, bool * set_title)
void GpGadgets::ParsePlotTitle(GpCommand & rC, CurvePoints * pPlot, char * pXTitle, char * pYTitle, bool * pSetTitle)
{
	legend_key * key = &keyT;
	if(rC.AlmostEq("t$itle") || rC.AlmostEq("not$itle")) {
		if(*pSetTitle)
			IntErrorCurToken("duplicate title");
		*pSetTitle = true;
		/* title can be enhanced if not explicitly disabled */
		pPlot->title_no_enhanced = !key->enhanced;
		if(rC.AlmostEq(rC.CToken++, "not$itle"))
			pPlot->title_is_suppressed = true;
		if(IsParametric || pPlot->title_is_suppressed) {
			if(InParametric)
				IntErrorCurToken("title allowed only after parametric function fully specified");
			ASSIGN_PTR(pXTitle, 0);  /* Remove default title . */
			ASSIGN_PTR(pYTitle, 0);  /* Remove default title . */
			if(rC.Eq(","))
				return;
		}
		/* This ugliness is because columnheader can be either a keyword */
		/* or a function name.  Yes, the design could have been better. */
		if(rC.AlmostEq("col$umnheader") && !(rC.Eq("columnhead") && rC.Eq(rC.CToken+1, "(")) ) {
			GpDf.DfSetKeyTitleColumnHead(rC, pPlot);
		}
		else if(rC.Eq("at")) {
			*pSetTitle = false;
		}
		else {
			GpDf.evaluate_inside_using = true;
			char * temp = rC.TryToGetString();
			GpDf.evaluate_inside_using = false;
			if(!pPlot->title_is_suppressed && !(pPlot->title = temp))
				IntErrorCurToken("expecting \"title\" for plot");
		}
		if(rC.Eq("at")) {
			int save_token = ++rC.CToken;
			pPlot->title_position = (GpPosition *)malloc(sizeof(GpPosition));
			if(rC.Eq("end")) {
				pPlot->title_position->scalex = character;
				pPlot->title_position->x = 1;
				rC.CToken++;
			}
			else if(rC.AlmostEq("beg$inning")) {
				pPlot->title_position->scalex = character;
				pPlot->title_position->x = -1;
				rC.CToken++;
			}
			else {
				GetPositionDefault(rC, pPlot->title_position, screen, 2);
			}
			if(save_token == rC.CToken)
				IntErrorCurToken("expecting \"at {beginning|end|<xpos>,<ypos>}\"");
		}
	}
	if(rC.AlmostEq("enh$anced")) {
		rC.CToken++;
		pPlot->title_no_enhanced = false;
	}
	else if(rC.AlmostEq("noenh$anced")) {
		rC.CToken++;
		pPlot->title_no_enhanced = true;
	}
}

