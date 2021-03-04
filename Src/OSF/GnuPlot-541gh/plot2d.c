// GNUPLOT - plot2d.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop
//
// static prototypes 
//
//static int check_or_add_boxplot_factor(curve_points * plot, const char* string, double x);
//static void add_tics_boxplot_factors(curve_points * plot);

//curve_points * P_FirstPlot = NULL; // the curves/surfaces of the plot 
//static udft_entry Plot2D_Func;
//static double histogram_rightmost = 0.0; // Highest x-coord of histogram so far 
//static text_label histogram_title;       // Subtitle for this histogram 
//static int stack_count = 0; // counter for stackheight 
//static GpCoordinate * stackheight = NULL; // Scratch space for y autoscale 
//static int paxis_start;   // PARALLELPLOT bookkeeping 
//static int paxis_end;     // PARALLELPLOT bookkeeping 
//static int paxis_current; // PARALLELPLOT bookkeeping 
// If the user tries to plot a complex-valued function without reducing it to
// some derived value like abs(f(z)), it generates a non-obvious error message
// "all points y value undefined". Try to detect this case by counting complex
// values as they are encountered so that a better error message is possible.
//static int Plot2D_NComplexValues = 0;
// 
// cp_extend() reallocates a curve_points structure to hold "num"
// points. This will either expand or shrink the storage.
// 
void cp_extend(curve_points * cp, int num)
{
	if(num != cp->p_max) {
		if(num > 0) {
			cp->points = (GpCoordinate *)SAlloc::R(cp->points, num * sizeof(cp->points[0]));
			if(cp->varcolor)
				cp->varcolor = (double *)SAlloc::R(cp->varcolor, num * sizeof(double));
			cp->p_max = num;
			cp->p_max -= 1; // Set trigger point for reallocation ahead of	
				// true end in case two slots are used at once (e.g. redundant final point of closed curve)	
		}
		else {
			// FIXME: Does this ever happen?  Should it call cp_free() instead? 
			ZFREE(cp->points);
			cp->p_max = 0;
			ZFREE(cp->varcolor);
			free_labels(cp->labels);
			cp->labels = NULL;
		}
	}
}
// 
// In the parametric case we can say plot [a= -4:4] [-2:2] [-1:1] sin(a),a**2
// while in the non-parametric case we would say only plot [b= -2:2] [-1:1] sin(b)
// 
//void plotrequest()
void GnuPlot::PlotRequest(GpTermEntry * pTerm)
{
	int dummy_token = 0;
	/*AXIS_INDEX*/int axis;
	if(!pTerm) // unknown 
		IntErrorCurToken("use 'set term' to set terminal type first");
	Gg.Is3DPlot = false;
	if(Gg.Parametric && strcmp(_Pb.set_dummy_var[0], "u") == 0)
		strcpy(_Pb.set_dummy_var[0], "t");
	// initialize the arrays from the 'set' scalars 
	AxS[FIRST_X_AXIS].Init(FALSE);
	AxS[FIRST_Y_AXIS].Init(TRUE);
	AxS[SECOND_X_AXIS].Init(FALSE);
	AxS[SECOND_Y_AXIS].Init(TRUE);
	AxS[T_AXIS].Init(FALSE);
	AxS[U_AXIS].Init(FALSE);
	AxS[V_AXIS].Init(FALSE);
	AxS[POLAR_AXIS].Init(TRUE);
	AxS[COLOR_AXIS].Init(TRUE);
	//
	// Always be prepared to restore the autoscaled values on "refresh" Dima Kogan April 2018
	// 
	for(axis = (AXIS_INDEX)0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
		GpAxis * this_axis = &AxS[axis];
		if(this_axis->set_autoscale != AUTOSCALE_NONE)
			this_axis->range_flags |= RANGE_WRITEBACK;
	}
	// Nonlinear mapping of x or y via linkage to a hidden primary axis. 
	// The user set autoscale for the visible axis; apply it also to the hidden axis. 
	for(axis = (AXIS_INDEX)0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
		GpAxis * secondary = &AxS[axis];
		if(axis == SAMPLE_AXIS)
			continue;
		if(secondary->linked_to_primary && secondary->linked_to_primary->index == -secondary->index) {
			GpAxis * primary = secondary->linked_to_primary;
			primary->set_autoscale = secondary->set_autoscale;
			primary->Init(true);
		}
	}
	// If we are called from a mouse zoom operation we should ignore
	// any range limits because otherwise the zoom won't zoom.		
	if(inside_zoom) {
		while(Pgm.EqualsCur("[")) {
			Pgm.ParseSkipRange();
		}
	}
	// Range limits for the entire plot are optional but must be given
	// in a fixed order. The keyword 'sample' terminates range parsing.
	if(Gg.Parametric || Gg.Polar) {
		dummy_token = ParseRange(T_AXIS);
		ParseRange(FIRST_X_AXIS);
	}
	else {
		dummy_token = ParseRange(FIRST_X_AXIS);
	}
	ParseRange(FIRST_Y_AXIS);
	ParseRange(SECOND_X_AXIS);
	ParseRange(SECOND_Y_AXIS);
	if(Pgm.EqualsCur("sample") && Pgm.EqualsNext("["))
		Pgm.Shift();
	// Clear out any tick labels read from data files in previous plot 
	for(axis = (AXIS_INDEX)0; axis<AXIS_ARRAY_SIZE; axis++) {
		t_ticdef * ticdef = &AxS[axis].ticdef;
		if(ticdef->def.user)
			ticdef->def.user = prune_dataticks(ticdef->def.user);
		if(!ticdef->def.user && ticdef->type == TIC_USER)
			ticdef->type = TIC_COMPUTED;
	}
	for(axis = (AXIS_INDEX)0; axis < AxS.GetParallelAxisCount(); axis++) {
		t_ticdef * ticdef = &AxS.Parallel(axis).ticdef;
		if(ticdef->def.user)
			ticdef->def.user = prune_dataticks(ticdef->def.user);
		if(!ticdef->def.user && ticdef->type == TIC_USER)
			ticdef->type = TIC_COMPUTED;
	}
	// use the default dummy variable unless changed 
	if(dummy_token > 0)
		Pgm.CopyStr(_Pb.c_dummy_var[0], dummy_token, MAX_ID_LEN);
	else
		strcpy(_Pb.c_dummy_var[0], _Pb.set_dummy_var[0]);
	EvalPlots(pTerm);
}
// 
// Helper function for refresh command.  Reexamine each data point and update the
// flags for INRANGE/OUTRANGE/UNDEFINED based on the current limits for that axis.
// Normally the axis limits are already known at this point. But if the user has
// forced "set autoscale" since the previous plot or refresh, we need to reset the
// axis limits and try to approximate the full auto-scaling behaviour.
// 
//void refresh_bounds(curve_points * pFirstPlot, int nplots)
void GnuPlot::RefreshBounds(curve_points * pFirstPlot, int nplots)
{
	const curve_points * this_plot = pFirstPlot;
	int iplot; // plot index 
	for(iplot = 0; iplot < nplots; iplot++, this_plot = this_plot->next) {
		GpAxis * x_axis = &AxS[this_plot->AxIdx_X];
		GpAxis * y_axis = &AxS[this_plot->AxIdx_Y];
		// IMAGE clipping is done elsewhere, so we don't need INRANGE/OUTRANGE checks 
		if(oneof2(this_plot->plot_style, IMAGE, RGBIMAGE)) {
			if(x_axis->set_autoscale || y_axis->set_autoscale)
				ProcessImage(term, this_plot, IMG_UPDATE_AXES);
			continue;
		}
		for(int i = 0; i < this_plot->p_count; i++) {
			GpCoordinate * point = &this_plot->points[i];
			if(point->type == UNDEFINED)
				continue;
			else
				point->type = INRANGE;
			// This autoscaling logic is identical to that in refresh_3dbounds() in plot3d.c
			if(!this_plot->noautoscale) {
				x_axis->AutoscaleOnePoint(point->x);
				if(this_plot->plot_style & PLOT_STYLE_HAS_VECTOR)
					x_axis->AutoscaleOnePoint(point->xhigh);
			}
			if(!x_axis->InRange(point->x)) {
				point->type = OUTRANGE;
				continue;
			}
			if(!this_plot->noautoscale) {
				y_axis->AutoscaleOnePoint(point->y);
				if(this_plot->plot_style == VECTOR)
					y_axis->AutoscaleOnePoint(point->yhigh);
			}
			if(!y_axis->InRange(point->y)) {
				point->type = OUTRANGE;
				continue;
			}
		}
		if(oneof2(this_plot->plot_style, BOXES, IMPULSES))
			ImpulseRangeFiddling(this_plot);
	}
	this_plot = pFirstPlot;
	for(iplot = 0; iplot < nplots; iplot++, this_plot = this_plot->next) {
		// handle 'reverse' ranges 
		AxS.CheckRange(this_plot->AxIdx_X);
		AxS.CheckRange(this_plot->AxIdx_Y);
		// Make sure the bounds are reasonable, and tweak them if they aren't 
		AxisCheckedExtendEmptyRange(this_plot->AxIdx_X, NULL);
		AxisCheckedExtendEmptyRange(this_plot->AxIdx_Y, NULL);
	}
}
//
// current_plot->token is after datafile spec, for error reporting
// it will later be moved past title/with/linetype/pointtype
//
//static int get_data(curve_points * current_plot)
int GnuPlot::GetData(curve_points * pPlot)
{
	int i /* num. points ! */, j;
	int ngood;
	int max_cols, min_cols; /* allowed range of column numbers */
	GpCoordinate * cp;
	double v[MAXDATACOLS];
	memzero(v, sizeof(v));
	if(pPlot->varcolor == NULL) {
		bool variable_color = FALSE;
		if((pPlot->lp_properties.pm3d_color.type == TC_RGB) && (pPlot->lp_properties.pm3d_color.value < 0))
			variable_color = TRUE;
		if(pPlot->lp_properties.pm3d_color.type == TC_Z)
			variable_color = TRUE;
		if(pPlot->lp_properties.pm3d_color.type == TC_COLORMAP)
			variable_color = TRUE;
		if(pPlot->lp_properties.l_type == LT_COLORFROMCOLUMN)
			variable_color = TRUE;
		if(pPlot->plot_smooth != SMOOTH_NONE && pPlot->plot_smooth != SMOOTH_ZSORT) {
			/* FIXME:  It would be possible to support smooth cspline lc palette */
			/* but it would require expanding and interpolating plot->varcolor   */
			/* in parallel with the y values.                                    */
			variable_color = FALSE;
		}
		if(variable_color) {
			pPlot->varcolor = (double *)SAlloc::M(pPlot->p_max * sizeof(double));
		}
	}
	/* eval_plots has already opened file */

	/* HBB 2000504: For most 2D plot styles the 'z' coordinate is unused.
	 * Set it to NO_AXIS to account for that. For styles that use
	 * the z coordinate as a real coordinate (i.e. not a width or
	 * 'delta' component, change the setting inside the switch: */
	pPlot->AxIdx_Z = NO_AXIS;
	/* HBB NEW 20060427: if there's only one, explicit using column,
	 * it's y data.  df_axis[] has to reflect that, so df_readline()
	 * will expect time/date input. */
	if(_Df.df_no_use_specs == 1)
		_Df.df_axis[0] = _Df.df_axis[1];
	switch(pPlot->plot_style) { /* set maximum columns to scan */
		case XYERRORLINES:
		case XYERRORBARS:
		case BOXXYERROR:
		    min_cols = 4;
		    max_cols = 7;
		    if(_Df.df_no_use_specs >= 6) {
			    // HBB 20060427: signal 3rd and 4th column are absolute x data --- needed so time/date parsing works 
			    _Df.df_axis[2] = _Df.df_axis[3] = _Df.df_axis[0];
			    // and 5th and 6th are absolute y data 
			    _Df.df_axis[4] = _Df.df_axis[5] = _Df.df_axis[1];
		    }
		    break;
		case FINANCEBARS:
		    // HBB 20000504: use 'z' coordinate for y-axis quantity 
		    pPlot->AxIdx_Z = pPlot->AxIdx_Y;
		    min_cols = 5;
		    max_cols = 6;
		    // HBB 20060427: signal 3rd and 4th column are absolute y data --- needed so time/date parsing works
		    _Df.df_axis[2] = _Df.df_axis[3] = _Df.df_axis[4] = _Df.df_axis[1];
		    break;
		case BOXPLOT:
		    min_cols = 2; /* fixed x, lots of y data points */
		    max_cols = 4; /* optional width, optional factor */
		    ExpectString(4);
		    break;
		case CANDLESTICKS:
		    pPlot->AxIdx_Z = pPlot->AxIdx_Y;
		    min_cols = 5;
		    max_cols = 7;
		    _Df.df_axis[2] = _Df.df_axis[3] = _Df.df_axis[4] = _Df.df_axis[1];
		    break;
		case BOXERROR:
		    min_cols = 3;
		    max_cols = 6;
		    /* There are four possible cases: */
		    /* 3 cols --> (x,y,dy), auto dx */
		    /* 4 cols, boxwidth==-2 --> (x,y,ylow,yhigh), auto dx */
		    /* 4 cols, boxwidth!=-2 --> (x,y,dy,dx) */
		    /* 5 cols --> (x,y,ylow,yhigh,dx) */
		    /* In each case an additional column may hold variable color */
		    if((_Df.df_no_use_specs == 4 && V.BoxWidth == -2.0) || _Df.df_no_use_specs >= 5)
			    // HBB 20060427: signal 3rd and 4th column are absolute y data --- needed so time/date parsing works 
			    _Df.df_axis[2] = _Df.df_axis[3] = _Df.df_axis[1];
		    break;
		case VECTOR: /* x, y, dx, dy, variable color or arrow style */
		case ARROWS: /* x, y, len, ang, variable color or arrow style */
		    min_cols = 4;
		    max_cols = 5;
		    break;
		case XERRORLINES:
		case XERRORBARS:
		    min_cols = 3;
		    max_cols = 5;
		    if(_Df.df_no_use_specs >= 4)
			    // HBB 20060427: signal 3rd and 4th column are absolute x data --- needed so time/date parsing works 
			    _Df.df_axis[2] = _Df.df_axis[3] = _Df.df_axis[0];
		    break;
		case YERRORLINES:
		case YERRORBARS:
		    min_cols = 2;
		    max_cols = 5;
		    if(_Df.df_no_use_specs >= 4)
			    // HBB 20060427: signal 3rd and 4th column are absolute y data --- needed so time/date parsing works
			    _Df.df_axis[2] = _Df.df_axis[3] = _Df.df_axis[1];
		    break;
		case HISTOGRAMS:
		    min_cols = 1;
		    max_cols = 3;
		    /* Stacked histogram get out of sync if "missing" values in the
		     * input data are silently ignored.  require_value() forces a
		     * value of NaN to be returned in this case.
		     */
		    if(oneof2(Gg.histogram_opts.type, HT_STACKED_IN_TOWERS, HT_STACKED_IN_LAYERS))
			    RequireValue(1);
		    break;
		case BOXES:
		    min_cols = 1;
		    max_cols = 4;
		    break;
		case FILLEDCURVES:
		    min_cols = 1;
		    max_cols = 3;
		    _Df.df_axis[2] = _Df.df_axis[1]; // Both curves use same y axis 
		    break;
		case IMPULSES: /* 2 + possible variable color */
		case POLYGONS:
		case LINES:
		case DOTS:
		    min_cols = 1;
		    max_cols = 3;
		    break;
		case LABELPOINTS:
		    /* 3 column data: X Y Label */
		    /* extra columns allow variable pointsize, pointtype, and/or rotation */
		    min_cols = 3;
		    max_cols = 6;
		    ExpectString(3);
		    break;
		case IMAGE:
		    min_cols = 3;
		    max_cols = 3;
		    break;
		case RGBIMAGE:
		    min_cols = 3;
		    max_cols = 6;
		    break;
		case RGBA_IMAGE:
		    min_cols = 3;
		    max_cols = 6;
		    break;
		case CIRCLES: /* 3 + possible variable color, or 5 + possible variable color */
		    min_cols = 2;
		    max_cols = 6;
		    break;
		case ELLIPSES:
		    min_cols = 2; /* x, y, major axis, minor axis */
		    max_cols = 6; /* + optional angle, possible variable color */
		    break;
		case POINTSTYLE:
		case LINESPOINTS:
		    /* 1 column: y coordinate only */
		    /* 2 columns x and y coordinates */
		    /* Allow 1 extra column because of 'pointsize variable' */
		    /* Allow 1 extra column because of 'pointtype variable' */
		    /* Allow 1 extra column because of 'lc rgb variable'    */
		    min_cols = 1;
		    max_cols = 5;
		    break;
		case PARALLELPLOT:
		case SPIDERPLOT:
		    /* 1 column: y coordinate only */
		    /* extra columns for variable color, pointtype, etc */
		    min_cols = 1;
		    max_cols = 4;
		    break;

		case TABLESTYLE:
		    min_cols = 1;
		    max_cols = MAXDATACOLS;
		    break;

		default:
		    min_cols = 1;
		    max_cols = 2;
		    break;
	}
	/* Restictions on plots with "smooth" option */
	switch(pPlot->plot_smooth) {
		case SMOOTH_NONE:
		    break;
		case SMOOTH_ZSORT:
		    min_cols = 3;
		    if(pPlot->plot_style != POINTSTYLE)
			    IntError(NO_CARET, "'smooth zsort' only possible in plots 'with points'");
		    break;
		case SMOOTH_ACSPLINES:
		    max_cols++;
		    break;
		default:
		    if(_Df.df_no_use_specs > 2 && pPlot->plot_style != FILLEDCURVES)
			    IntWarn(NO_CARET, "extra columns ignored by smoothing option");
		    break;
	}
	if(pPlot->plot_smooth && pPlot->plot_style == FILLEDCURVES) {
		if(pPlot->filledcurves_options.closeto == FILLEDCURVES_CLOSED) {
			if(pPlot->plot_smooth == SMOOTH_CSPLINES)
				pPlot->plot_smooth = SMOOTH_PATH;
			if(pPlot->plot_smooth != SMOOTH_PATH) {
				pPlot->plot_smooth = SMOOTH_NONE;
				IntWarn(NO_CARET, "only 'smooth path' or 'smooth cspline' is supported for closed curves");
			}
		}
	}
	/* May 2013 - Treating timedata columns as strings allows
	 * functions column(N) and column("HEADER") to work on time data.
	 * Sep 2014: But the column count is wrong for HISTOGRAMS
	 */
	if(pPlot->plot_style != HISTOGRAMS) {
		if(AxS[pPlot->AxIdx_X].datatype == DT_TIMEDATE)
			ExpectString(1);
		if(AxS[pPlot->AxIdx_Y].datatype == DT_TIMEDATE)
			ExpectString(2);
	}
	if(_Df.df_no_use_specs > max_cols)
		IntError(NO_CARET, "Too many using specs for this style");
	if(_Df.df_no_use_specs > 0 && _Df.df_no_use_specs < min_cols)
		IntError(NO_CARET, "Not enough columns for this style");
	i = 0; ngood = 0;
	// If the user has set an explicit locale for numeric input, apply it 
	// here so that it affects data fields read from the input file.      
	set_numeric_locale();
	// Initial state 
	_Df.df_warn_on_missing_columnheader = true;
	while((j = DfReadLine(v, max_cols)) != DF_EOF) {
		if(i >= pPlot->p_max) {
			/* overflow about to occur. Extend size of points[]
			 * array. Double the size, and add 1000 points, to avoid
			 * needlessly small steps. */
			cp_extend(pPlot, i + i + 1000);
		}
		// Assume range is OK; we will check later 
		pPlot->points[i].type = (j == 0) ? UNDEFINED : INRANGE;
		/* First handle all the special cases (j <= 0) */
		switch(j) {
			case 0:
			    DfClose();
			    IntError(pPlot->token, "Bad data on line %d of file %s", _Df.df_line_number, NZOR(_Df.df_filename, ""));
			    continue;
			case DF_COMPLEX_VALUE:
			    _Plt.Plot2D_NComplexValues++;
			    fprintf(stderr, "plot2d.c:%d caught a complex value\n", __LINE__);
			// Fall through to normal undefined case 
			case DF_UNDEFINED:
			    /* Version 5 - We are now trying to pass back all available info even
			     * if one of the requested columns was missing or undefined.
			     */
			    pPlot->points[i].type = UNDEFINED;
			    if(_Df.missing_val && sstreq(_Df.missing_val, "NaN")) {
				    j = DF_MISSING;
				    // fall through to short-circuit for missing data 
			    }
			    else {
				    j = _Df.df_no_use_specs;
				    break;
				    // continue with normal processing for this line 
			    }

			case DF_MISSING:
			    // Plot type specific handling of missing points goes here. 
			    if(oneof2(pPlot->plot_style, PARALLELPLOT, SPIDERPLOT)) {
				    pPlot->points[i].type = UNDEFINED;
				    j = _Df.df_no_use_specs;
				    break;
			    }
			    if(pPlot->plot_style == HISTOGRAMS) {
				    pPlot->points[i].type = UNDEFINED;
				    i++;
			    }
			    if(pPlot->plot_style == TABLESTYLE) {
				    j = _Df.df_no_use_specs;
				    break;
			    }
			    continue;
			case DF_FIRST_BLANK:
			    /* The binary input routines generate DF_FIRST_BLANK at the end
			     * of scan lines, so that the data may be used for the isometric
			     * splots.  Rather than turning that off inside the binary
			     * reading routine based upon the plot mode, DF_FIRST_BLANK is
			     * ignored for certain plot types requiring 3D coordinates in
			     * MODE_PLOT.
			     */
			    if(pPlot->plot_style == IMAGE || pPlot->plot_style == RGBIMAGE || pPlot->plot_style == RGBA_IMAGE)
				    continue;
			    // make type of next point undefined, but recognizable 
			    pPlot->points[i] = blank_data_line;
			    i++;
			    continue;
			case DF_SECOND_BLANK:
			    // second blank line. We dont do anything (we did everything when we got FIRST one)
			    continue;
			case DF_FOUND_KEY_TITLE:
			    DfSetKeyTitle(pPlot);
			    continue;
			case DF_KEY_TITLE_MISSING:
			    fprintf(stderr, "get_data: key title not found in requested column\n");
			    continue;
			case DF_COLUMN_HEADERS:
			    continue;
			default:
			    if(j < 0) {
				    DfClose();
				    IntErrorCurToken("internal error : df_readline returned %d : datafile line %d", j, _Df.df_line_number);
			    }
			    break; /* Not continue!! */
		}
		// We now know that j > 0, i.e. there is some data on this input line 
		ngood++;
		// "plot ... with table" bypasses all the column interpretation 
		if(pPlot->plot_style == TABLESTYLE) {
			TabulateOneLine(v, _Df.df_strings, j);
			continue;
		}
		// June 2010 - New mechanism for variable color                  
		// If variable color is requested, take the color value from the 
		// final column of input and decrement the column count by one.  
		if(pPlot->varcolor) {
			static char * errmsg = "Not enough columns for variable color";
			switch(pPlot->plot_style) {
				case CANDLESTICKS:
				case FINANCEBARS:
				    if(j < 6) 
						IntError(NO_CARET, errmsg);
				    break;
				case XYERRORLINES:
				case XYERRORBARS:
				case BOXXYERROR:
				    if(j != 7 && j != 5) 
						IntError(NO_CARET, errmsg);
				    break;
				case VECTOR:
				case ARROWS:
				    if(j < 5) 
						IntError(NO_CARET, errmsg);
				    break;
				case LABELPOINTS:
				case BOXERROR:
				case XERRORLINES:
				case XERRORBARS:
				case YERRORLINES:
				case YERRORBARS:
				    if(j < 4) 
						IntError(NO_CARET, errmsg);
				    break;
				case CIRCLES:
				    if(j == 5 || j < 3) 
						IntError(NO_CARET, errmsg);
				    break;
				case ELLIPSES:
				case BOXES:
				case POINTSTYLE:
				case LINESPOINTS:
				case IMPULSES:
				case LINES:
				case DOTS:
				    if(j < 3) 
						IntError(NO_CARET, errmsg);
				    break;
				case PARALLELPLOT:
				case SPIDERPLOT:
				    if(j < 1) 
						IntError(NO_CARET, errmsg);
				    break;
				case BOXPLOT:
				    // Only the key sample uses this value 
				    v[j++] = pPlot->base_linetype + 1;
				    break;
				default:
				    break;
			}
			pPlot->varcolor[i] = v[--j];
		}
		/* Unusual special cases */
		// In spiderplots the implicit "x coordinate" v[0] is really the axis number. 
		// Add this at the front and shift all other using specs to the right.        
		if(Gg.SpiderPlot) {
			for(int is = j++; is>0; is--)
				v[is] = v[is-1];
			v[0] = _Plt.paxis_current;
		}
		// Single data value - is it y with implicit x or something else? 
		if(j == 1 && !(pPlot->plot_style == HISTOGRAMS)) {
			if(default_smooth_weight(pPlot->plot_smooth))
				v[1] = 1.0;
			else {
				v[1] = v[0];
				v[0] = _Df.df_datum;
			}
			j = 2;
		}
		/* May 2018:  The huge switch statement below is now organized by plot	*/
		/* style.  Each plot style can have its own understanding of what the	*/
		/* value in a particular field of the "using" specifier represents.	*/
		/* E.g. the 3rd field might be z or radius or color.			*/
		switch(pPlot->plot_style) {
			case LINES:
			case DOTS:
			case IMPULSES:
		    { // x y [acspline weight] 
			    coordval w; // only for (pPlot->plot_smooth == SMOOTH_ACSPLINES) 
			    w = (j > 2) ? v[2] : 1.0;
			    Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[1], v[1], w);
			    break;
		    }
			case POINTSTYLE:
			case LINESPOINTS:
		    { /* x y {z} {var_ps} {var_pt} {lc variable} */
			/* NB: assumes CRD_PTSIZE == xlow CRD_PTTYPE == xhigh CRD_PTCHAR == ylow */
			    int var = 2; /* column number for next variable spec */
			    coordval weight = (pPlot->plot_smooth == SMOOTH_ACSPLINES) ? v[2] : 1.0;
			    coordval var_ps = pPlot->lp_properties.PtSize;
			    coordval var_pt = pPlot->lp_properties.PtType;
			    coordval var_char = 0;
			    if(pPlot->plot_smooth == SMOOTH_ZSORT)
				    weight = v[var++];
			    if(var_pt == PT_VARIABLE) {
				    if(isnan(v[var]) && _Df.df_tokens[var]) {
					    strnzcpy((char *)(&var_char), _Df.df_tokens[var], sizeof(coordval));
					    truncate_to_one_utf8_char((char *)(&var_char));
				    }
				    var_pt = v[var++];
			    }
			    if(var_ps == PTSZ_VARIABLE)
				    var_ps = v[var++];
			    if(var > j)
				    IntError(NO_CARET, "Not enough using specs");
			    if(var_pt < 0)
				    var_pt = 0;
			    Store2DPoint(pPlot, i++, v[0], v[1],
				var_ps, var_pt, var_char, v[1], weight);
			    break;
		    }
			case LABELPOINTS:
		    { /* x y string {rotate variable}
				 *            {point {pt variable} {ps variable}}
				 *            {tc|lc variable}
				 */
			    int var = 3; /* column number for next variable spec */
			    coordval var_rotation = pPlot->labels->rotate;
			    coordval var_ps = pPlot->labels->lp_properties.PtSize;
			    coordval var_pt = pPlot->labels->lp_properties.PtType;
			    if(pPlot->labels->tag == VARIABLE_ROTATE_LABEL_TAG)
				    var_rotation = v[var++];
			    if(var_pt == PT_VARIABLE)
				    var_pt = v[var++];
			    if(var_ps == PTSZ_VARIABLE)
				    var_ps = v[var++];
			    if(var > j)
				    IntError(NO_CARET, "Not enough using specs");
			    Store2DPoint(pPlot, i, v[0], v[1],
				var_ps, var_pt, var_rotation, v[1], 0.0);
			    // Allocate and fill in a text_label structure to match it 
			    if(pPlot->points[i].type != UNDEFINED) {
				    StoreLabel(term, pPlot->labels, &(pPlot->points[i]), i, _Df.df_tokens[2], pPlot->varcolor ? pPlot->varcolor[i] : 0.0);
			    }
			    i++;
			    break;
		    }
			case STEPS:
			case FSTEPS:
			case FILLSTEPS:
			case HISTEPS:
		    { /* x y */
			    Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[1], v[1], -1.0);
			    break;
		    }
			case CANDLESTICKS:
			case FINANCEBARS:
		    { /* x yopen ylow yhigh yclose [xhigh] */
			    coordval yopen = v[1];
			    coordval ylow = v[2];
			    coordval yhigh = v[3];
			    coordval yclose = v[4];
			    coordval xlow = v[0];
			    coordval xhigh = v[0];
			    /* NB: plot_c_bars will set xhigh = xlow + 2*(x-xlow) */
			    if(j > 5 && v[5] > 0)
				    xlow = v[0] - v[5]/2.;
			    Store2DPoint(pPlot, i++, v[0], yopen, xlow, xhigh, ylow, yhigh, yclose);
			    break;
		    }
			case XERRORLINES:
			case XERRORBARS:
		    { /* x y xdelta   or    x y xlow xhigh */
			    coordval xlow  = (j > 3) ? v[2] : v[0] - v[2];
			    coordval xhigh = (j > 3) ? v[3] : v[0] + v[2];
			    Store2DPoint(pPlot, i++, v[0], v[1], xlow, xhigh, v[1], v[1], 0.0);
			    break;
		    }
			case YERRORLINES:
			case YERRORBARS:
		    { /* x y ydelta   or    x y ylow yhigh */
			    coordval ylow  = (j > 3) ? v[2] : v[1] - v[2];
			    coordval yhigh = (j > 3) ? v[3] : v[1] + v[2];
			    Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], ylow, yhigh, -1.0);
			    break;
		    }
			case BOXERROR:
		    { /* 3 columns:  x y ydelta
				 * 4 columns:  x y ydelta xdelta     (if xdelta <=0 use boxwidth)
				 * 5 columns:  x y ylow yhigh xdelta (if xdelta <=0 use boxwidth)
				 * ==========
				 * DEPRECATED
				 * 4 columns:  x y ylow yhigh      (boxwidth == -2)
				 */
			    coordval xlow, xhigh, ylow, yhigh, width;
			    if(j == 3) {
				    xlow  = v[0];
				    xhigh = v[0];
				    ylow  = v[1] - v[2];
				    yhigh = v[1] + v[2];
				    width = -1.0;
			    }
			    else if(j == 4) {
				    if(v[3] <= 0)
					    v[3] = V.BoxWidth;
				    xlow  = (V.BoxWidth == -2.0) ? v[0] : v[0] - v[3]/2.;
				    xhigh = (V.BoxWidth == -2.0) ? v[0] : v[0] + v[3]/2.;
				    ylow  = (V.BoxWidth == -2.0) ? v[2] : v[1] - v[2];
				    yhigh = (V.BoxWidth == -2.0) ? v[3] : v[1] + v[2];
				    width = (V.BoxWidth == -2.0) ? -1.0 : 0.0;
			    }
			    else {
				    if(v[4] <= 0)
					    v[4] = V.BoxWidth;
				    xlow  = v[0] - v[4]/2.;
				    xhigh = v[0] + v[4]/2.;
				    ylow  = v[2];
				    yhigh = v[3];
				    width = 0.0;
			    }
			    Store2DPoint(pPlot, i++, v[0], v[1], xlow, xhigh, ylow, yhigh, width);
			    break;
		    }
			case XYERRORLINES:
			case XYERRORBARS:
			case BOXXYERROR:
		    { /* 4 columns: x y xdelta ydelta
				 * 6 columns: x y xlow xhigh ylow yhigh
				 */
			    coordval xlow  = (j>5) ? v[2] : v[0] - v[2];
			    coordval xhigh = (j>5) ? v[3] : v[0] + v[2];
			    coordval ylow  = (j>5) ? v[4] : v[1] - v[3];
			    coordval yhigh = (j>5) ? v[5] : v[1] + v[3];
			    Store2DPoint(pPlot, i++, v[0], v[1], xlow, xhigh, ylow, yhigh, 0.0);
			    if(j == 5)
				    IntError(NO_CARET, "wrong number of columns for this plot style");
			    break;
		    }
			case BOXES:
		    { /* 2 columns: x y (width depends on "set boxwidth")
				 * 3 columns: x y xdelta
				 * 4 columns: x y xlow xhigh
				 */
			    coordval xlow  = v[0];
			    coordval xhigh = v[0];
			    coordval width = 0.0;
			    double base = AxS[pPlot->AxIdx_X].base;
			    if(j == 2) {
				    /* For boxwidth auto, we cannot calculate xlow/xhigh yet since they
				     * depend on both adjacent boxes.  This is signalled by storing -1
				     * in point->z to indicate xlow/xhigh must be calculated later.
				     */
				    if(V.BoxWidth > 0 && V.BoxWidthIsAbsolute) {
					    xlow = (AxS[pPlot->AxIdx_X].log) ? v[0] * pow(base, -V.BoxWidth/2.0) : v[0] - V.BoxWidth / 2.0;
					    xhigh = (AxS[pPlot->AxIdx_X].log) ? v[0] * pow(base, V.BoxWidth/2.0) : v[0] + V.BoxWidth / 2.0;
				    }
				    else {
					    width = -1.0;
				    }
			    }
			    else if(j == 3) {
				    xlow  = v[0] - v[2]/2;
				    xhigh = v[0] + v[2]/2;
			    }
			    else if(j == 4) {
				    xlow  = v[2];
				    xhigh = v[3];
			    }
			    Store2DPoint(pPlot, i++, v[0], v[1], xlow, xhigh, v[1], v[1], width);
			    break;
		    }
			case FILLEDCURVES:
		    { /* 2 columns:  x y
				 * 3 columns:  x y1 y2
				 */
			    coordval y1 = v[1];
			    coordval y2;
			    coordval w = 0.0; /* only needed for SMOOTH_ACSPLINES) */
			    if(j==2) {
				    if(oneof2(pPlot->filledcurves_options.closeto, FILLEDCURVES_CLOSED, FILLEDCURVES_DEFAULT))
					    y2 = y1;
				    else
					    y2 = pPlot->filledcurves_options.at;
			    }
			    else {
				    y2 = v[2];
				    if(pPlot->filledcurves_options.closeto == FILLEDCURVES_DEFAULT)
					    pPlot->filledcurves_options.closeto = FILLEDCURVES_BETWEEN;
				    if(oneof3(pPlot->filledcurves_options.closeto, FILLEDCURVES_BETWEEN, FILLEDCURVES_ABOVE, FILLEDCURVES_BELOW)) {
					    switch(pPlot->plot_smooth) {
						    case SMOOTH_NONE:
						    case SMOOTH_CSPLINES:
						    case SMOOTH_SBEZIER:
							break;
						    case SMOOTH_ACSPLINES:
							w = (j > 3) ? v[3] : 1.0;
							break;
						    default:
							IntWarn(NO_CARET, "use csplines, acsplines or sbezier to smooth non-closed filledcurves");
							pPlot->plot_smooth = SMOOTH_NONE;
							break;
					    }
				    }
			    }
			    Store2DPoint(pPlot, i++, v[0], y1, v[0], v[0], y1, y2, w);
			    break;
		    }
			case POLYGONS:
		    { // Nothing yet to distinguish this from filledcurves 
			    Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[1], v[1], 0);
			    break;
		    }
			case BOXPLOT:
		    { /* 2 columns:  x data
				 * 3 columns:  x data width
				 * 4 columns:  x data width factor
				 */
			    coordval extra = DEFAULT_BOXPLOT_FACTOR;
			    coordval xlow =  (j > 2) ? v[0] - v[2]/2. : v[0];
			    coordval xhigh = (j > 2) ? v[0] + v[2]/2. : v[0];
			    if(j == 4)
				    extra = CheckOrAddBoxplotFactor(pPlot, _Df.df_tokens[3], v[0]);
			    Store2DPoint(pPlot, i++, v[0], v[1], xlow, xhigh, v[1], v[1], extra);
			    break;
		    }
			case VECTOR:
		    { /* 4 columns:	x y xdelta ydelta [arrowstyle variable] */
			    coordval xlow  = v[0];
			    coordval xhigh = v[0] + v[2];
			    coordval ylow  = v[1];
			    coordval yhigh = v[1] + v[3];
			    coordval arrowstyle = (j == 5) ? v[4] : 0.0;
			    Store2DPoint(pPlot, i++, v[0], v[1], xlow, xhigh, ylow, yhigh, arrowstyle);
			    break;
		    }
			case ARROWS:
		    { /* 4 columns:	x y len ang [arrowstyle variable] */
			    coordval xlow  = v[0];
			    coordval ylow  = v[1];
			    coordval len = v[2];
			    coordval ang = v[3];
			    coordval arrowstyle = (j == 5) ? v[4] : 0.0;
			    Store2DPoint(pPlot, i++, v[0], v[1], xlow, len, ylow, ang, arrowstyle);
			    break;
		    }
			case CIRCLES:
		    { /* x y
				 * x y radius
				 * x y radius arc_begin arc_end
				 */
			    coordval x = v[0];
			    coordval y = v[1];
			    coordval xlow = x;
			    coordval xhigh = x;
			    coordval arc_begin = (j >= 5) ? v[3] : 0.0;
			    coordval arc_end = (j >= 5) ? v[4] : 360.0;
			    coordval radius = DEFAULT_RADIUS;
			    if(j >= 3 && v[2] >= 0) {
				    xlow  = x - v[2];
				    xhigh = x + v[2];
				    radius = 0.0;
			    }
			    Store2DPoint(pPlot, i++, x, y, xlow, xhigh, arc_begin, arc_end, radius);
			    break;
		    }
			case ELLIPSES:
		    { /* x y
				 * x y diam  (used for both major and minor axis)
				 * x y major_diam minor_diam
				 * x y major_diam minor_diam orientation
				 */
			    coordval x = v[0];
			    coordval y = v[1];
			    coordval major_axis = (j >= 3) ? v[2] : 0.0;
			    coordval minor_axis = (j >= 4) ? v[3] : (j >= 3) ? v[2] : 0.0;
			    coordval orientation = (j >= 5) ? v[4] : 0.0;
			    coordval flag = (major_axis <= 0 || minor_axis <= 0) ?  DEFAULT_RADIUS : 0;
			    if(j == 2) /* FIXME: why not also for j == 3 or 4? */
				    orientation = Gg.default_ellipse.o.ellipse.orientation;
			    Store2DPoint(pPlot, i++, x, y,
				major_axis, minor_axis, orientation, 0.0 /* not used */,
				flag);
			    break;
		    }
			case IMAGE:
		    { /* x y color_value */
			    Store2DPoint(pPlot, i++, v[0], v[1], v[0], v[0], v[1], v[1], v[2]);
			    break;
		    }
			case RGBIMAGE:
			case RGBA_IMAGE:
		    { // x y red green blue [alpha] 
			    Store2DPoint(pPlot, i, v[0], v[1], v[0], v[0], v[1], v[1], 0.0);
			    // If there is only one column of image data, it must be 32-bit ARGB 
			    if(j==3) {
				    uint argb = static_cast<uint>(v[2]);
				    v[2] = (argb >> 16) & 0xff;
				    v[3] = (argb >> 8) & 0xff;
				    v[4] = (argb) & 0xff;
				    /* The alpha channel convention is unfortunate */
				    v[5] = 255 - (uint)((argb >> 24) & 0xff);
			    }
			    cp = &(pPlot->points[i]);
			    cp->CRD_R = v[2];
			    cp->CRD_G = v[3];
			    cp->CRD_B = v[4];
			    cp->CRD_A = v[5]; /* Alpha channel */
			    i++;
			    break;
		    }
			case HISTOGRAMS:
		    { /* 1 column:	y
				 * 2 columns:	y yerr		(set style histogram errorbars)
				 * 3 columns:	y ymin ymax	(set style histogram errorbars)
				 */
			    coordval x = _Df.df_datum;
			    coordval y = v[0];
			    coordval ylow  = v[0];
			    coordval yhigh = v[0];
			    coordval width = (V.BoxWidth > 0.0) ? V.BoxWidth : 1.0;
			    coordval xlow  = x - width / 2.0;
			    coordval xhigh = x + width / 2.0;
			    if(Gg.histogram_opts.type == HT_ERRORBARS) {
				    if(j == 1)
					    IntErrorCurToken("No column given for errorbars in using specifier");
				    if(j == 2) {
					    ylow  = y - v[1];
					    yhigh = y + v[1];
				    }
				    else {
					    ylow   = v[1];
					    yhigh  = v[2];
				    }
			    }
			    else if(j > 1)
				    IntErrorCurToken("Too many columns in using specification");
			    if(Gg.histogram_opts.type == HT_STACKED_IN_TOWERS) {
				    _Plt.histogram_rightmost = pPlot->histogram_sequence + pPlot->histogram->start;
				    pPlot->histogram->end = _Plt.histogram_rightmost;
			    }
			    else if(x + pPlot->histogram->start > _Plt.histogram_rightmost) {
				    _Plt.histogram_rightmost = x + pPlot->histogram->start;
				    pPlot->histogram->end = _Plt.histogram_rightmost;
			    }
			    Store2DPoint(pPlot, i++, x, y, xlow, xhigh, ylow, yhigh, 0.0);
			    break;
		    }
			case PARALLELPLOT:
		    { // Similar to histogram plots, each parallel axis gets a separate comma-separated plot element with a single "using" spec.
			    coordval x = AxS.Parallel(_Plt.paxis_current-1).paxis_x;
			    coordval y = v[1];
			    Store2DPoint(pPlot, i++, x, y, x, x, y, y, 0.0);
			    break;
		    }
			case SPIDERPLOT:
		    { // Spider plots are essentially parallelaxis plots in polar coordinates.
			    coordval var_color = pPlot->varcolor ? pPlot->varcolor[i] : i;
			    coordval var_pt = pPlot->lp_properties.PtType;
			    coordval theta = _Plt.paxis_current;
			    coordval r = v[1];
			    var_pt = (var_pt == PT_VARIABLE) ? v[2] : var_pt + 1;
			    Store2DPoint(pPlot, i++, theta, r, theta, var_pt, r, var_color, 0.0);
			    break;
		    }
			// These exist for 3D (splot) but not for 2D (plot) 
			case PM3DSURFACE:
			case SURFACEGRID:
			case ZERRORFILL:
			case ISOSURFACE:
			    IntError(NO_CARET, "This plot style only available for splot");
			    break;
			// If anybody hits this it is because we missed handling a plot style above. To be fixed immediately!
			default:
			    IntError(NO_CARET, "This plot style must have been missed in the grand code reorganization");
			    break;
		} /* switch (plot->plot_style) */
	} /* while more input data */
	// This removes an extra point caused by blank lines after data. 
	if(i > 0 && pPlot->points[i-1].type == UNDEFINED)
		i--;
	pPlot->p_count = i;
	cp_extend(pPlot, i); /* shrink to fit */
	DfClose();
	// We are finished reading user input; return to C locale for internal use 
	reset_numeric_locale();
	// Deferred evaluation of plot title now that we know column headers 
	ReevaluatePlotTitle(pPlot);
	return ngood; //0 indicates an 'empty' file 
}
//
// called by get_data for each point 
//
//static void store2d_point(curve_points * current_plot, int i/* point number */, double x, double y, double xlow, double xhigh, double ylow, double yhigh, double width/* BOXES widths: -1 -> autocalc, 0 ->  use xlow/xhigh */)
void GnuPlot::Store2DPoint(curve_points * pPlot, int i/* point number */, 
	double x, double y, double xlow, double xhigh, double ylow, double yhigh, double width/* BOXES widths: -1 -> autocalc, 0 ->  use xlow/xhigh */)
{
	GpCoordinate * cp = &(pPlot->points[i]);
	GpAxis * x_axis_ptr, * y_axis_ptr;
	coord_type * y_type_ptr;
	coord_type dummy_type = INRANGE; /* sometimes we dont care about outranging */
	bool excluded_range = FALSE;
	// FIXME this destroys any UNDEFINED flag assigned during input 
	cp->type = INRANGE;
	if(Gg.Polar) {
		double theta = x;
		GpAxis * theta_axis = &AxS[T_AXIS];
		// "x" is really the polar angle theta,	so check it against trange. 
		if(theta < theta_axis->data_min)
			theta_axis->data_min = theta;
		if(theta > theta_axis->data_max)
			theta_axis->data_max = theta;
		if(theta < theta_axis->min && (theta <= theta_axis->max || theta_axis->max == -VERYLARGE)) {
			if((theta_axis->autoscale & AUTOSCALE_MAX) == 0)
				excluded_range = TRUE;
		}
		if(theta > theta_axis->max && (theta >= theta_axis->min || theta_axis->min == VERYLARGE)) {
			if((theta_axis->autoscale & AUTOSCALE_MIN) == 0)
				excluded_range = TRUE;
		}
		// "y" at this point is really "r", so check it against rrange.	
		if(y < AxS.__R().data_min)
			AxS.__R().data_min = y;
		if(y > AxS.__R().data_max)
			AxS.__R().data_max = y;
		// Convert from polar to cartesian coordinates and check ranges 
		if(PolarToXY(x, y, &x, &y, TRUE) == OUTRANGE)
			cp->type = OUTRANGE;
		// Some plot styles use xhigh and yhigh for other quantities, 
		// which polar mode transforms would break		      
		if(pPlot->plot_style == CIRCLES) {
			double radius = (xhigh - xlow)/2.0;
			xlow = x - radius;
			xhigh = x + radius;
		}
		else {
			// Jan 2017 - now skipping range check on rhigh, rlow 
			PolarToXY(xhigh, yhigh, &xhigh, &yhigh, FALSE);
			PolarToXY(xlow, ylow, &xlow, &ylow, FALSE);
		}
	}
	/* Version 5: Allow to store Inf or NaN
	 *  We used to exit immediately in this case rather than storing anything
	 */
	x_axis_ptr = &AxS[pPlot->AxIdx_X];
	dummy_type = cp->type;  /* Save result of range check on x */
	y_axis_ptr = &AxS[pPlot->AxIdx_Y];
	store_and_update_range(&(cp->x), x, &(cp->type), x_axis_ptr, pPlot->noautoscale);
	store_and_update_range(&(cp->y), y, &(cp->type), y_axis_ptr, pPlot->noautoscale);
	// special cases for the "y" axes of parallel axis plots 
	if((pPlot->plot_style == PARALLELPLOT) || (pPlot->plot_style == SPIDERPLOT)) {
		y_type_ptr = &dummy_type; /* Use xrange test result as a start point */
		y_axis_ptr = &AxS.Parallel(pPlot->AxIdx_P-1);
		store_and_update_range(&(cp->y), y, y_type_ptr, y_axis_ptr, FALSE);
	}
	else {
		dummy_type = INRANGE;
	}
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
		case ARROWS:
		case PARALLELPLOT:
		case SPIDERPLOT:
		    cp->xlow = xlow;
		    cp->xhigh = xhigh;
		    cp->ylow = ylow;
		    cp->yhigh = yhigh;
		    break;
		case BOXES:     /* auto-scale to xlow xhigh */
		case BOXPLOT:   /* auto-scale to xlow xhigh, factor is already in z */
		    cp->ylow = ylow; /* ylow yhigh not really needed but store them anyway */
		    cp->yhigh = yhigh;
		    STORE_AND_UPDATE_RANGE(cp->xlow, xlow, dummy_type, pPlot->AxIdx_X, pPlot->noautoscale, cp->xlow = -VERYLARGE);
		    STORE_AND_UPDATE_RANGE(cp->xhigh, xhigh, dummy_type, pPlot->AxIdx_X, pPlot->noautoscale, cp->xhigh = -VERYLARGE);
		    break;
		case CIRCLES:
		    cp->yhigh = yhigh;
		    STORE_AND_UPDATE_RANGE(cp->xlow, xlow, dummy_type, pPlot->AxIdx_X, pPlot->noautoscale, cp->xlow = -VERYLARGE);
		    STORE_AND_UPDATE_RANGE(cp->xhigh, xhigh, dummy_type, pPlot->AxIdx_X, pPlot->noautoscale, cp->xhigh = -VERYLARGE);
		    cp->ylow = ylow; /* arc begin */
		    cp->xhigh = yhigh; /* arc end */
		    if(fabs(ylow) > 1000.0 || fabs(yhigh) > 1000.0) /* safety check for insane arc angles */
			    cp->type = UNDEFINED;
		    break;
		case ELLIPSES:
		    /* We want to pass the parameters to the ellipse drawing routine as they are,
		     * so we have to calculate the extent of the ellipses for autoscaling here.
		     * Properly calculating the correct extent of a rotated ellipse, respecting
		     * axis scales and all would be very hard.
		     * So we just use the larger of the two axes, multiplied by some empirical factors
		     * to ensure^Whope that all parts of the ellipses will be in the auto-scaled area. */
		    /* xlow = major axis, xhigh = minor axis, ylow = orientation */
#define YRANGE_FACTOR ((pPlot->ellipseaxes_units == ELLIPSEAXES_YY) ? 1.0 : 1.4)
#define XRANGE_FACTOR ((pPlot->ellipseaxes_units == ELLIPSEAXES_XX) ? 1.1 : 1.0)
		    STORE_AND_UPDATE_RANGE(cp->xlow, x-0.5*MAX(xlow, xhigh)*XRANGE_FACTOR, dummy_type, pPlot->AxIdx_X, pPlot->noautoscale, cp->xlow = -VERYLARGE);
		    STORE_AND_UPDATE_RANGE(cp->xhigh, x+0.5*MAX(xlow, xhigh)*XRANGE_FACTOR, dummy_type, pPlot->AxIdx_X, pPlot->noautoscale, cp->xhigh = -VERYLARGE);
		    STORE_AND_UPDATE_RANGE(cp->ylow, y-0.5*MAX(xlow, xhigh)*YRANGE_FACTOR, dummy_type, pPlot->AxIdx_Y, pPlot->noautoscale, cp->ylow = -VERYLARGE);
		    STORE_AND_UPDATE_RANGE(cp->yhigh, y+0.5*MAX(xlow, xhigh)*YRANGE_FACTOR, dummy_type, pPlot->AxIdx_Y, pPlot->noautoscale, cp->yhigh = -VERYLARGE);
		    /* So after updating the axes we re-store the parameters */
		    cp->xlow = xlow; /* major axis */
		    cp->xhigh = xhigh; /* minor axis */
		    cp->ylow = ylow; /* orientation */
		    break;
		case IMAGE:
		    STORE_AND_UPDATE_RANGE(cp->CRD_COLOR, width, dummy_type, COLOR_AXIS, pPlot->noautoscale, NOOP);
		    break;
		default:        /* auto-scale to xlow xhigh ylow yhigh */
		    STORE_AND_UPDATE_RANGE(cp->xlow, xlow, dummy_type, pPlot->AxIdx_X, pPlot->noautoscale, cp->xlow = -VERYLARGE);
		    STORE_AND_UPDATE_RANGE(cp->xhigh, xhigh, dummy_type, pPlot->AxIdx_X, pPlot->noautoscale, cp->xhigh = -VERYLARGE);
		    STORE_AND_UPDATE_RANGE(cp->ylow, ylow, dummy_type, pPlot->AxIdx_Y, pPlot->noautoscale, cp->ylow = -VERYLARGE);
		    STORE_AND_UPDATE_RANGE(cp->yhigh, yhigh, dummy_type, pPlot->AxIdx_Y, pPlot->noautoscale, cp->yhigh = -VERYLARGE);
		    break;
	}
	// HBB 20010214: if z is not used for some actual value, just
	// store 'width' to that axis and be done with it 
	if((int)pPlot->AxIdx_Z == NO_AXIS)
		cp->z = width;
	else
		STORE_AND_UPDATE_RANGE(cp->z, width, dummy_type, pPlot->AxIdx_Z, pPlot->noautoscale, cp->z = -VERYLARGE);
	// If we have variable color corresponding to a z-axis value, use it to autoscale 
	if(pPlot->lp_properties.pm3d_color.type == TC_Z && pPlot->varcolor)
		STORE_AND_UPDATE_RANGE(pPlot->varcolor[i], pPlot->varcolor[i], dummy_type, COLOR_AXIS, pPlot->noautoscale, NOOP);
	// Same thing for colormap z-values 
	if(pPlot->lp_properties.pm3d_color.type == TC_COLORMAP && pPlot->varcolor && pPlot->lp_properties.P_Colormap)
		STORE_AND_UPDATE_RANGE(pPlot->varcolor[i], pPlot->varcolor[i], dummy_type, COLOR_AXIS, pPlot->noautoscale, NOOP);
	// July 2014 - Some points are excluded because they fall outside of trange	
	// even though they would be inside the plot if drawn.			
	if(excluded_range)
		cp->type = EXCLUDEDRANGE;
}
// 
// We abuse the labels structure to store a list of boxplot labels ("factors").
// Check if <string> is already among the known factors, if not, add it to the list.
// 
//static int check_or_add_boxplot_factor(curve_points * pPlot, const char * pString, double x)
int GnuPlot::CheckOrAddBoxplotFactor(curve_points * pPlot, const char * pString, double x)
{
	int index = DEFAULT_BOXPLOT_FACTOR;
	// If there is no factor column (4th using spec) fall back to a single boxplot 
	if(pString) {
		text_label * label, * prev_label, * new_label;
		// Remove the trailing garbage, quotes etc. from the string 
		char * trimmed_string = DfParseStringField(pString);
		if(strlen(trimmed_string) > 0) {
			bool is_new = FALSE;
			prev_label = pPlot->labels;
			if(!prev_label)
				IntError(NO_CARET, "boxplot labels not initialized");
			for(label = prev_label->next; label; label = label->next, prev_label = prev_label->next) {
				// check if string is already stored 
				if(sstreq(trimmed_string, label->text))
					break;
				// If we are keeping a sorted list, test against current entry (insertion sort).
				if(Gg.boxplot_opts.sort_factors) {
					if(strcmp(trimmed_string, label->text) < 0) {
						is_new = TRUE;
						break;
					}
				}
			}
			// not found, so we add it now 
			if(!label || is_new) {
				new_label = (text_label *)SAlloc::M(sizeof(text_label));
				memcpy(new_label, pPlot->labels, sizeof(text_label));
				new_label->next = label;
				new_label->tag = pPlot->boxplot_factors++;
				new_label->text = sstrdup(trimmed_string);
				new_label->place.x = pPlot->points[0].x;
				prev_label->next = new_label;
				label = new_label;
			}
			index = label->tag;
		}
		SAlloc::F(trimmed_string);
	}
	return index;
}
//
// Add tic labels to the boxplots,
// showing which level of the factor variable they represent 
//
//static void add_tics_boxplot_factors(curve_points * pPlot)
void GnuPlot::AddTicsBoxplotFactors(curve_points * pPlot)
{
	int i = 0;
	AXIS_INDEX boxplot_labels_axis = Gg.boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X  ? FIRST_X_AXIS : Gg.boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X2 ? SECOND_X_AXIS : AxS.Idx_X;
	for(text_label * p_label = pPlot->labels->next; p_label; p_label = p_label->next) {
		AddTicUser(&AxS[boxplot_labels_axis], p_label->text, pPlot->points->x + i * Gg.boxplot_opts.separation, -1);
		i++;
	}
}
//
// Autoscaling of box plots cuts off half of the box on each end. 
// Add a half-boxwidth to the range in this case.  EAM Aug 2007   
//
//static void box_range_fiddling(const curve_points * pPlot)
void GnuPlot::BoxRangeFiddling(const curve_points * pPlot)
{
	const int i = pPlot->p_count - 1;
	if(i > 0) {
		if(AxS[pPlot->AxIdx_X].autoscale & AUTOSCALE_MIN) {
			if(pPlot->points[0].type != UNDEFINED && pPlot->points[1].type != UNDEFINED) {
				double xlow;
				if(V.BoxWidthIsAbsolute)
					xlow = pPlot->points[0].x - V.BoxWidth;
				else
					xlow = pPlot->points[0].x - (pPlot->points[1].x - pPlot->points[0].x) / 2.0;
				SETMIN(AxS[pPlot->AxIdx_X].min, xlow);
			}
		}
		if(AxS[pPlot->AxIdx_X].autoscale & AUTOSCALE_MAX) {
			if(pPlot->points[i].type != UNDEFINED && pPlot->points[i-1].type != UNDEFINED) {
				double xhigh;
				if(V.BoxWidthIsAbsolute)
					xhigh = pPlot->points[i].x + V.BoxWidth;
				else
					xhigh = pPlot->points[i].x + (pPlot->points[i].x - pPlot->points[i-1].x) / 2.0;
				SETMAX(AxS[pPlot->AxIdx_X].max, xhigh);
			}
		}
	}
}
//
// Autoscaling of boxplots with no explicit width cuts off the outer edges of the box 
//
//static void boxplot_range_fiddling(curve_points * plot)
void GnuPlot::BoxPlotRangeFiddling(curve_points * pPlot)
{
	double extra_width;
	int N;
	if(pPlot->p_count > 0) {
		// Create a tic label for each boxplot category 
		if(pPlot->boxplot_factors > 0) {
			if(Gg.boxplot_opts.labels != BOXPLOT_FACTOR_LABELS_OFF)
				AddTicsBoxplotFactors(pPlot);
		}
		// Sort the points and removed any that are undefined 
		N = Gr.FilterBoxplot(pPlot);
		pPlot->p_count = N;
		if(pPlot->points[0].type == UNDEFINED)
			IntError(NO_CARET, "boxplot has undefined x coordinate");
		// If outliers were processed, that has taken care of autoscaling on y.
		// If not, we need to calculate the whisker bar ends to determine yrange.
		if(Gg.boxplot_opts.outliers)
			AxS.RestoreAutoscaledRanges(&AxS[pPlot->AxIdx_X], NULL);
		else
			AxS.RestoreAutoscaledRanges(&AxS[pPlot->AxIdx_X], &AxS[pPlot->AxIdx_Y]);
		AutoscaleBoxPlot(term, pPlot);
		extra_width = pPlot->points[0].xhigh - pPlot->points[0].xlow;
		if(extra_width == 0)
			extra_width = (V.BoxWidth > 0.0 && V.BoxWidthIsAbsolute) ? V.BoxWidth : 0.5;
		if(extra_width < 0)
			extra_width = -extra_width;
		if(AxS[pPlot->AxIdx_X].autoscale & AUTOSCALE_MIN) {
			if(AxS[pPlot->AxIdx_X].min >= pPlot->points[0].x)
				AxS[pPlot->AxIdx_X].min -= 1.5 * extra_width;
			else if(AxS[pPlot->AxIdx_X].min >= pPlot->points[0].x - extra_width)
				AxS[pPlot->AxIdx_X].min -= 1 * extra_width;
		}
		if(AxS[pPlot->AxIdx_X].autoscale & AUTOSCALE_MAX) {
			const double nfactors = MAX(0, pPlot->boxplot_factors - 1);
			const double plot_max = pPlot->points[0].x + nfactors * Gg.boxplot_opts.separation;
			if(AxS[pPlot->AxIdx_X].max <= plot_max)
				AxS[pPlot->AxIdx_X].max = plot_max + 1.5 * extra_width;
			else if(AxS[pPlot->AxIdx_X].max <= plot_max + extra_width)
				AxS[pPlot->AxIdx_X].max += extra_width;
		}
	}
}
//
// Since the stored x values for histogrammed data do not correspond exactly 
// to the eventual x coordinates, we need to modify the x axis range bounds. 
// Also the two stacked histogram modes need adjustment of the y axis bounds.
//
//static void histogram_range_fiddling(curve_points * pPlot)
void GnuPlot::HistogramRangeFiddling(curve_points * pPlot)
{
	double xlow, xhigh;
	int i;
	//
	// EAM FIXME - HT_STACKED_IN_TOWERS forcibly resets xmin, which is only
	// correct if no other plot came first.
	//
	switch(Gg.histogram_opts.type) {
		case HT_STACKED_IN_LAYERS:
		    if(AxS[pPlot->AxIdx_Y].autoscale & AUTOSCALE_MAX) {
			    if(pPlot->histogram_sequence == 0) {
				    SAlloc::F(_Plt.stackheight);
				    _Plt.stackheight = (GpCoordinate *)SAlloc::M(pPlot->p_count * sizeof(GpCoordinate));
				    for(_Plt.stack_count = 0; _Plt.stack_count < pPlot->p_count; _Plt.stack_count++) {
					    _Plt.stackheight[_Plt.stack_count].yhigh = 0;
					    _Plt.stackheight[_Plt.stack_count].ylow = 0;
				    }
			    }
			    else if(pPlot->p_count > _Plt.stack_count) {
				    _Plt.stackheight = (GpCoordinate *)SAlloc::R(_Plt.stackheight, pPlot->p_count * sizeof(GpCoordinate));
				    for(; _Plt.stack_count < pPlot->p_count; _Plt.stack_count++) {
					    _Plt.stackheight[_Plt.stack_count].yhigh = 0;
					    _Plt.stackheight[_Plt.stack_count].ylow = 0;
				    }
			    }
			    for(i = 0; i < _Plt.stack_count; i++) {
				    if(pPlot->points[i].type != UNDEFINED) {
						if(pPlot->points[i].y >= 0)
							_Plt.stackheight[i].yhigh += pPlot->points[i].y;
						else
							_Plt.stackheight[i].ylow += pPlot->points[i].y;
						if(AxS[pPlot->AxIdx_Y].max < _Plt.stackheight[i].yhigh)
							AxS[pPlot->AxIdx_Y].max = _Plt.stackheight[i].yhigh;
						if(AxS[pPlot->AxIdx_Y].min > _Plt.stackheight[i].ylow)
							AxS[pPlot->AxIdx_Y].min = _Plt.stackheight[i].ylow;
					}
			    }
		    }
		// fall through to checks on x range 
		case HT_CLUSTERED:
		case HT_ERRORBARS:
		    if(!AxS[FIRST_X_AXIS].autoscale)
			    break;
		    if(AxS[FIRST_X_AXIS].autoscale & AUTOSCALE_MIN) {
			    xlow = pPlot->histogram->start - 1.0;
			    if(AxS[FIRST_X_AXIS].min > xlow)
				    AxS[FIRST_X_AXIS].min = xlow;
		    }
		    if(AxS[FIRST_X_AXIS].autoscale & AUTOSCALE_MAX) {
			    // FIXME - why did we increment p_count on UNDEFINED points? 
			    while(pPlot->points[pPlot->p_count-1].type == UNDEFINED) {
				    pPlot->p_count--;
				    if(!pPlot->p_count)
					    IntError(NO_CARET, "All points in histogram UNDEFINED");
			    }
			    xhigh = pPlot->points[pPlot->p_count-1].x;
			    xhigh += pPlot->histogram->start + 1.0;
			    if(AxS[FIRST_X_AXIS].max < xhigh)
				    AxS[FIRST_X_AXIS].max = xhigh;
		    }
		    break;
		case HT_STACKED_IN_TOWERS:
		    // FIXME: Rather than trying to reproduce the layout along X 
		    // we should just track the actual xmin/xmax as we go.       
		    if(AxS[FIRST_X_AXIS].set_autoscale) {
			    if((AxS[FIRST_X_AXIS].set_autoscale & AUTOSCALE_MIN)) {
				    xlow = -1.0;
				    if(AxS[FIRST_X_AXIS].min > xlow)
					    AxS[FIRST_X_AXIS].min = xlow;
			    }
			    xhigh = pPlot->histogram_sequence;
			    xhigh += pPlot->histogram->start + 1.0;
			    if(AxS[FIRST_X_AXIS].max != xhigh)
				    AxS[FIRST_X_AXIS].max  = xhigh;
		    }
		    if(AxS[FIRST_Y_AXIS].set_autoscale) {
			    double ylow, yhigh;
			    for(i = 0, yhigh = ylow = 0.0; i < pPlot->p_count; i++)
				    if(pPlot->points[i].type != UNDEFINED) {
					    if(pPlot->points[i].y >= 0)
						    yhigh += pPlot->points[i].y;
					    else
						    ylow += pPlot->points[i].y;
				    }
			    if(AxS[FIRST_Y_AXIS].set_autoscale & AUTOSCALE_MAX)
				    if(AxS[pPlot->AxIdx_Y].max < yhigh)
					    AxS[pPlot->AxIdx_Y].max = yhigh;
			    if(AxS[FIRST_Y_AXIS].set_autoscale & AUTOSCALE_MIN)
				    if(AxS[pPlot->AxIdx_Y].min > ylow)
					    AxS[pPlot->AxIdx_Y].min = ylow;
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
//void polar_range_fiddling(const curve_points * pPlot)
void GnuPlot::PolarRangeFiddling(const curve_points * pPlot)
{
	if(AxS[POLAR_AXIS].set_autoscale & AUTOSCALE_MAX) {
		const double plotmax_x = MAX(AxS[pPlot->AxIdx_X].max, -AxS[pPlot->AxIdx_X].min);
		const double plotmax_y = MAX(AxS[pPlot->AxIdx_Y].max, -AxS[pPlot->AxIdx_Y].min);
		const double plotmax = MAX(plotmax_x, plotmax_y);
		if((AxS[pPlot->AxIdx_X].set_autoscale & AUTOSCALE_BOTH) == AUTOSCALE_BOTH) {
			AxS[pPlot->AxIdx_X].max = plotmax;
			AxS[pPlot->AxIdx_X].min = -plotmax;
		}
		if((AxS[pPlot->AxIdx_Y].set_autoscale & AUTOSCALE_BOTH) == AUTOSCALE_BOTH) {
			AxS[pPlot->AxIdx_Y].max = plotmax;
			AxS[pPlot->AxIdx_Y].min = -plotmax;
		}
	}
}
//
// Extend auto-scaling of y-axis to include zero 
//
//static void impulse_range_fiddling(const curve_points * pPlot)
void GnuPlot::ImpulseRangeFiddling(const curve_points * pPlot)
{
	if(AxS[pPlot->AxIdx_Y].log)
		return;
	if(AxS[pPlot->AxIdx_Y].autoscale & AUTOSCALE_MIN) {
		if(AxS[pPlot->AxIdx_Y].min > 0)
			AxS[pPlot->AxIdx_Y].min = 0;
	}
	if(AxS[pPlot->AxIdx_Y].autoscale & AUTOSCALE_MAX) {
		if(AxS[pPlot->AxIdx_Y].max < 0)
			AxS[pPlot->AxIdx_Y].max = 0;
	}
}
//
// Clean up x and y axis bounds for parallel plots 
//
//static void parallel_range_fiddling(const curve_points * pPlot)
void GnuPlot::ParallelRangeFiddling(const curve_points * pPlot)
{
	int num_parallelplots = 0;
	while(pPlot) {
		if(pPlot->plot_style == PARALLELPLOT) {
			const double x = AxS.Parallel(pPlot->AxIdx_P-1).paxis_x;
			AxS[pPlot->AxIdx_X].AutoscaleOnePoint(x-1.0);
			AxS[pPlot->AxIdx_X].AutoscaleOnePoint(x+1.0);
			num_parallelplots++;
		}
		pPlot = pPlot->next;
	}
	// The normal y axis is not used by parallel plots, so if no 
	// range is established then we get lots of warning messages 
	if(num_parallelplots > 0) {
		if(AxS[FIRST_Y_AXIS].min == VERYLARGE)
			AxS[FIRST_Y_AXIS].min = 0.0;
		if(AxS[FIRST_Y_AXIS].max == -VERYLARGE)
			AxS[FIRST_Y_AXIS].max = 1.0;
	}
}
//
// Clean up x and y axis bounds for spider plots 
//
//static void spiderplot_range_fiddling(curve_points * plot)
void GnuPlot::SpiderPlotRangeFiddling(curve_points * plot)
{
	while(plot) {
		if(plot->plot_style == SPIDERPLOT) {
			// The normal x and y axes are not used by spider plots, so if no 
			// range is established then we get lots of warning messages 
			if(AxS[plot->AxIdx_X].autoscale & AUTOSCALE_MIN)
				AxS[FIRST_X_AXIS].min = -1.0;
			if(AxS[plot->AxIdx_X].autoscale & AUTOSCALE_MAX)
				AxS[FIRST_X_AXIS].max =  1.0;
			if(AxS[plot->AxIdx_Y].autoscale & AUTOSCALE_MIN)
				AxS[FIRST_Y_AXIS].min = -1.0;
			if(AxS[plot->AxIdx_Y].autoscale & AUTOSCALE_MAX)
				AxS[FIRST_Y_AXIS].max =  1.0;
			return;
		}
		plot = plot->next;
	}
}
//
// store_label() is called by get_data for each point 
// This routine is exported so it can be shared by plot3d 
//
//text_label * store_label(text_label * pListHead, GpCoordinate * cp, int i/* point number */, char * string/* start of label string */, 
	//double colorval/* used if text color derived from palette */)
text_label * GnuPlot::StoreLabel(GpTermEntry * pTerm, text_label * pListHead, GpCoordinate * cp, int i/* point number */, char * string/* start of label string */, 
	double colorval/* used if text color derived from palette */)
{           
	static text_label * tl = NULL;
	int textlen;
	if(!pListHead)
		IntError(NO_CARET, "text_label list was not initialized");
	// If listhead->next is NULL, the list is currently empty and we will 
	// insert this label at the head.  Otherwise tl already points to the 
	// tail (previous insertion) and we will add the new label there.     
	if(pListHead->next == NULL)
		tl = pListHead;
	// Allocate a new label structure and fill it in 
	tl->next = (text_label *)SAlloc::M(sizeof(text_label));
	memcpy(tl->next, tl, sizeof(text_label));
	tl = tl->next;
	tl->next = (text_label*)NULL;
	tl->tag = i;
	tl->place.x = cp->x;
	tl->place.y = cp->y;
	tl->place.z = cp->z;
	// optional variables from user spec 
	tl->rotate = static_cast<int>(cp->CRD_ROTATE);
	tl->lp_properties.PtType = static_cast<int>(cp->CRD_PTTYPE);
	tl->lp_properties.PtSize = cp->CRD_PTSIZE;
	// Check for optional (textcolor palette ...) 
	if(tl->textcolor.type == TC_Z)
		tl->textcolor.value = colorval;
	// Check for optional (textcolor rgb variable) 
	else if(pListHead->textcolor.type == TC_RGB && pListHead->textcolor.value < 0)
		tl->textcolor.lt = static_cast<int>(colorval);
	// Check for optional (textcolor variable) 
	else if(pListHead->textcolor.type == TC_VARIABLE) {
		lp_style_type lptmp;
		if(Gg.PreferLineStyles)
			LpUseProperties(pTerm, &lptmp, (int)colorval);
		else
			LoadLineType(pTerm, &lptmp, (int)colorval);
		tl->textcolor = lptmp.pm3d_color;
	}
	if(pListHead->lp_properties.flags & LP_SHOW_POINTS) {
		// Check for optional (point linecolor palette ...) 
		if(tl->lp_properties.pm3d_color.type == TC_Z)
			tl->lp_properties.pm3d_color.value = colorval;
		// Check for optional (point linecolor rgb variable) 
		else if(pListHead->lp_properties.pm3d_color.type == TC_RGB && pListHead->lp_properties.pm3d_color.value < 0)
			tl->lp_properties.pm3d_color.lt = static_cast<int>(colorval);
		// Check for optional (point linecolor variable) 
		else if(pListHead->lp_properties.l_type == LT_COLORFROMCOLUMN) {
			lp_style_type lptmp;
			if(Gg.PreferLineStyles)
				LpUseProperties(pTerm, &lptmp, (int)colorval);
			else
				LoadLineType(pTerm, &lptmp, (int)colorval);
			tl->lp_properties.pm3d_color = lptmp.pm3d_color;
		}
	}
	SETIFZ(string, ""); // Check for null string (no label) 
	textlen = 0;
	// Handle quoted separators and quoted quotes 
	if(_Df.df_separators) {
		bool in_quote = false;
		while(string[textlen]) {
			if(string[textlen] == '"')
				in_quote = !in_quote;
			else if(strchr(_Df.df_separators, string[textlen]) && !in_quote)
				break;
			textlen++;
		}
		while(textlen > 0 && isspace((uchar)string[textlen-1]))
			textlen--;
	}
	else {
		// This is the normal case (no special separator character) 
		if(*string == '"') {
			for(textlen = 1; string[textlen] && string[textlen] != '"'; textlen++)
				;
		}
		while(string[textlen] && !isspace((uchar)string[textlen]))
			textlen++;
	}
	// Strip double quote from both ends 
	if(string[0] == '"' && string[textlen-1] == '"')
		textlen -= 2, string++;
	tl->text = (char *)SAlloc::M(textlen+1);
	strncpy(tl->text, string, textlen);
	tl->text[textlen] = '\0';
	parse_esc(tl->text);
	FPRINTF((stderr, "LABELPOINT %f %f \"%s\" \n", tl->place.x, tl->place.y, tl->text));
	FPRINTF((stderr, "           %g %g %g %g %g %g %g\n", cp->x, cp->y, cp->xlow, cp->xhigh, cp->ylow, cp->yhigh, cp->z));
	return tl;
}
// 
// This parses the plot command after any global range specifications.
// To support autoscaling on the x axis, we want any data files to define the
// x range, then to plot any functions using that range. We thus parse the input
// twice, once to pick up the data files, and again to pick up the functions.
// Definitions are processed twice, but that won't hurt.
// 
//static void eval_plots()
void GnuPlot::EvalPlots(GpTermEntry * pTerm)
{
	int    i;
	curve_points * p_plot = NULL;
	curve_points ** tp_ptr;
	/*t_uses_axis*/int uses_axis[AXIS_ARRAY_SIZE];
	bool   some_functions = false;
	bool   some_tables = false;
	int    plot_num, line_num;
	bool   was_definition = false;
	int    pattern_num;
	char * xtitle = NULL;
	int    begin_token = Pgm.GetCurTokenIdx(); /* so we can rewind for second pass */
	int    start_token = 0, end_token;
	legend_key * key = &Gg.KeyT;
	char   orig_dummy_var[MAX_ID_LEN+1];
	int    nbins = 0;
	double binlow = 0.0;
	double binhigh = 0.0;
	double binwidth = 0.0;
	// Histogram bookkeeping 
	double newhist_start = 0.0;
	int histogram_sequence = -1;
	int newhist_color = 1;
	int newhist_pattern = LT_UNDEFINED;
	_Plt.histogram_rightmost = 0.0;
	free_histlist(&Gg.histogram_opts);
	InitHistogram(NULL, NULL);
	// Parallel plot bookkeeping 
	_Plt.paxis_start = -1;
	_Plt.paxis_end = -1;
	_Plt.paxis_current = -1;
	uses_axis[FIRST_X_AXIS] = uses_axis[FIRST_Y_AXIS] = uses_axis[SECOND_X_AXIS] = uses_axis[SECOND_Y_AXIS] = (t_uses_axis)0;
	// Original Comment follows: 
	// Reset P_FirstPlot. This is usually done at the end of this function.
	// If there is an error within this function, the memory is left allocated,
	// since we cannot call cp_free if the list is incomplete. Making sure that
	// the list structure is always valid requires some rewriting */
	// EAM Apr 2007 - but we need to keep the previous structures around in
	// order to be able to refresh/zoom them without re-reading all the data.
	CpFree(_Plt.P_FirstPlot);
	_Plt.P_FirstPlot = NULL;
	tp_ptr = &_Plt.P_FirstPlot;
	plot_num = 0;
	line_num = 0; // default line type 
	pattern_num = Gg.default_fillstyle.fillpattern; // default fill pattern 
	strcpy(orig_dummy_var, _Pb.c_dummy_var[0]);
	Gg.InParametric = false;
	xtitle = NULL;
	Gg.VolatileData = false; // Assume that the input data can be re-read later 
	_Plt.Plot2D_NComplexValues = 0; // Track complex values so that we can warn about trying to plot them 
	/* ** First Pass: Read through data files ***
	 * This pass serves to set the xrange and to parse the command, as well
	 * as filling in every thing except the function data. That is done after
	 * the xrange is defined.
	 */
	_Pb.plot_iterator = CheckForIteration();
	while(TRUE) {
		/* Forgive trailing comma on a multi-element plot command */
		if(Pgm.EndOfCommand()) {
			if(plot_num == 0)
				IntErrorCurToken("function to plot expected");
			break;
		}
		p_plot = NULL;
		if(!Gg.InParametric && !was_definition)
			start_token = Pgm.GetCurTokenIdx();
		if(Pgm.AlmostEqualsCur("newhist$ogram")) {
			lp_style_type lp(lp_style_type::defCommon); //= DEFAULT_LP_STYLE_TYPE;
			fill_style_type fs;
			int previous_token;
			Pgm.Shift();
			histogram_sequence = -1;
			memzero(&_Plt.histogram_title, sizeof(text_label));
			if(_Plt.histogram_rightmost > 0)
				newhist_start = _Plt.histogram_rightmost + 2;
			lp.l_type = line_num;
			newhist_color = lp.l_type + 1;
			fs.fillpattern = LT_UNDEFINED;
			do {
				previous_token = Pgm.GetCurTokenIdx();
				if(Pgm.EqualsCur("at")) {
					Pgm.Shift();
					newhist_start = RealExpression();
				}
				// Store title in temporary variable and then copy into the 
				// new histogram structure when it is allocated.            
				if(!_Plt.histogram_title.text && Pgm.IsStringValue(Pgm.GetCurTokenIdx())) {
					_Plt.histogram_title.textcolor = Gg.histogram_opts.title.textcolor;
					_Plt.histogram_title.boxed = Gg.histogram_opts.title.boxed;
					_Plt.histogram_title.pos = Gg.histogram_opts.title.pos;
					_Plt.histogram_title.text = TryToGetString();
					_Plt.histogram_title.font = sstrdup(Gg.histogram_opts.title.font);
					ParseLabelOptions(&_Plt.histogram_title, 2);
				}
				// Allow explicit starting color or pattern for this histogram 
				if(Pgm.EqualsCur("lt") || Pgm.AlmostEqualsCur("linet$ype")) {
					Pgm.Shift();
					newhist_color = IntExpression();
				}
				fs.fillstyle = FS_SOLID;
				fs.filldensity = 100;
				fs.border_color = Gg.default_fillstyle.border_color;
				ParseFillStyle(&fs);
			} while(Pgm.GetCurTokenIdx() != previous_token);
			newhist_pattern = fs.fillpattern;
			if(!Pgm.EqualsCur(","))
				IntErrorCurToken("syntax error");
		}
		else if(Pgm.AlmostEqualsCur("newspider$plot")) {
			Pgm.Shift();
			_Plt.paxis_current = 0;
			if(!Pgm.EqualsCur(","))
				IntErrorCurToken("syntax error (missing comma)");
		}
		else if(IsDefinition(Pgm.GetCurTokenIdx())) {
			Define();
			if(Pgm.EqualsCur(","))
				Pgm.Shift();
			was_definition = TRUE;
			continue;
		}
		else {
			int specs = 0;
			/* for datafile plot, record datafile spec for title */
			char* name_str;
			bool duplication = FALSE;
			bool set_smooth = FALSE, set_axes = FALSE, set_title = FALSE;
			bool set_with = FALSE, set_lpstyle = FALSE;
			bool set_fillstyle = FALSE;
			bool set_fillcolor = FALSE;
			bool set_labelstyle = FALSE;
			bool set_ellipseaxes_units = FALSE;
			double paxis_x = -VERYLARGE;
			t_colorspec fillcolor = DEFAULT_COLORSPEC;
			/* CHANGE: Aug 2017
			 * Allow sampling both u and v so that it is possible to do
			 * plot sample [u=min:max:inc] [v=min:max:inc] '++' ... with image
			 */
			GpValue original_value_sample_var, original_value_sample_var2;
			int sample_range_token, v_range_token;
			plot_num++;
			// Check for a sampling range
			AxS.InitSampleRange(&AxS[FIRST_X_AXIS], DATA);
			sample_range_token = ParseRange(SAMPLE_AXIS);
			v_range_token = 0;
			if(sample_range_token != 0) {
				AxS[SAMPLE_AXIS].range_flags |= RANGE_SAMPLED;
				// If the sample was specifically on u we need to check v also 
				if(Pgm.Equals(sample_range_token, "u")) {
					AxS[U_AXIS].min = AxS[SAMPLE_AXIS].min;
					AxS[U_AXIS].max = AxS[SAMPLE_AXIS].max;
					AxS[U_AXIS].autoscale = AxS[SAMPLE_AXIS].autoscale;
					AxS[U_AXIS].SAMPLE_INTERVAL = AxS[SAMPLE_AXIS].SAMPLE_INTERVAL;
					AxS[U_AXIS].range_flags = AxS[SAMPLE_AXIS].range_flags;
					v_range_token = ParseRange(V_AXIS);
					if(v_range_token != 0)
						AxS[V_AXIS].range_flags |= RANGE_SAMPLED;
				}
			}
			was_definition = FALSE;
			// Allow replacement of the dummy variable in a function 
			if(sample_range_token > 0)
				Pgm.CopyStr(_Pb.c_dummy_var[0], sample_range_token, MAX_ID_LEN);
			else if(sample_range_token < 0)
				strcpy(_Pb.c_dummy_var[0], _Pb.set_dummy_var[0]);
			else
				strcpy(_Pb.c_dummy_var[0], orig_dummy_var);
			Pgm.dummy_func = &_Plt.Plot2D_Func; /* needed by parsing code */
			name_str = StringOrExpress(NULL);
			Pgm.dummy_func = NULL;
			if(name_str) { // data file to plot 
				if(Gg.Parametric && Gg.InParametric)
					IntErrorCurToken("previous parametric function not fully specified");
				if(sample_range_token !=0 && *name_str != '+')
					IntWarn(sample_range_token, "Ignoring sample range in non-sampled data plot");
				if(*name_str == '$' && !GetDatablock(name_str))
					IntError(Pgm.GetPrevTokenIdx(), "cannot plot voxel data");
				if(*tp_ptr)
					p_plot = *tp_ptr;
				else { // no memory malloc()'d there yet 
					p_plot = CpAlloc(MIN_CRV_POINTS);
					*tp_ptr = p_plot;
				}
				p_plot->plot_type = DATA;
				p_plot->plot_style = Gg.data_style;
				p_plot->plot_smooth = SMOOTH_NONE;
				p_plot->filledcurves_options = Gg.filledcurves_opts_data;
				// Only relevant to "with table" 
				free_at(Tab.P_FilterAt);
				Tab.P_FilterAt = NULL;
				free_at(_Df.df_plot_title_at); // Mechanism for deferred evaluation of plot title 
				// up to MAXDATACOLS cols 
				DfSetPlotMode(MODE_PLOT); // Needed for binary datafiles 
				specs = DfOpen(name_str, MAXDATACOLS, p_plot);
				// Store a pointer to the named variable used for sampling 
				p_plot->sample_var  = (sample_range_token > 0) ? AddUdv(sample_range_token) : Ev.AddUdvByName(_Pb.c_dummy_var[0]);
				p_plot->sample_var2 = (v_range_token > 0) ? AddUdv(v_range_token) : Ev.AddUdvByName(_Pb.c_dummy_var[1]);
				// Save prior value of sample variables so we can restore them later 
				original_value_sample_var = p_plot->sample_var->udv_value;
				original_value_sample_var2 = p_plot->sample_var2->udv_value;
				p_plot->sample_var->udv_value.SetNotDefined();
				p_plot->sample_var2->udv_value.SetNotDefined();
				Gcomplex(&(p_plot->sample_var->udv_value), 0.0, 0.0); /* Not sure this is necessary */
				p_plot->token = end_token = Pgm.GetPrevTokenIdx(); /* include modifiers in default title */
			}
			else if(Pgm.EqualsCur("keyentry")) {
				Pgm.Shift();
				if(*tp_ptr)
					p_plot = *tp_ptr;
				else { // no memory malloc()'d there yet 
					p_plot = CpAlloc(MIN_CRV_POINTS);
					*tp_ptr = p_plot;
				}
				p_plot->plot_type = KEYENTRY;
				p_plot->plot_style = LABELPOINTS;
				p_plot->token = end_token = Pgm.GetPrevTokenIdx();
			}
			else { // function to plot 
				some_functions = TRUE;
				if(Gg.Parametric) // working on x parametric function 
					Gg.InParametric = !Gg.InParametric;
				if(Gg.SpiderPlot)
					IntError(NO_CARET, "spiderplot is not possible for functions");
				if(*tp_ptr) {
					p_plot = *tp_ptr;
					cp_extend(p_plot, Gg.Samples1 + 1);
				}
				else { // no memory malloc()'d there yet
					p_plot = CpAlloc(Gg.Samples1 + 1);
					*tp_ptr = p_plot;
				}
				p_plot->plot_type = FUNC;
				p_plot->plot_style = Gg.func_style;
				p_plot->filledcurves_options = Gg.filledcurves_opts_func;
				end_token = Pgm.GetPrevTokenIdx();
			} /* end of IS THIS A FILE OR A FUNC block */
			// axis defaults 
			AxS.Idx_X = FIRST_X_AXIS;
			AxS.Idx_Y = FIRST_Y_AXIS;
			// Set this before parsing any modifying options 
			p_plot->base_linetype = line_num;
			// pm 25.11.2001 allow any order of options 
			while(!Pgm.EndOfCommand()) {
				int save_token = Pgm.GetCurTokenIdx();
				/* bin the data if requested */
				if(Pgm.EqualsCur("bins")) {
					if(set_smooth) {
						duplication = TRUE;
						break;
					}
					Pgm.Shift();
					p_plot->plot_smooth = SMOOTH_BINS;
					nbins = Gg.Samples1;
					if(Pgm.EqualsCur("=")) {
						Pgm.Shift();
						nbins = IntExpression();
						if(nbins <= 0)
							nbins = Gg.Samples1;
					}
					binlow = binhigh = 0.0;
					if(Pgm.EqualsCur("binrange")) {
						Pgm.Shift();
						if(!ParseRange(SAMPLE_AXIS))
							IntErrorCurToken("incomplete bin range");
						binlow  = AxS[SAMPLE_AXIS].min;
						binhigh = AxS[SAMPLE_AXIS].max;
					}
					binwidth = -1;
					if(Pgm.EqualsCur("binwidth")) {
						Pgm.Shift();
						if(!Pgm.EqualsCur("="))
							IntErrorCurToken("expecting binwidth=<width>");
						Pgm.Shift();
						binwidth = RealExpression();
					}
					continue;
				}
				/*  deal with smooth */
				if(Pgm.AlmostEqualsCur("s$mooth")) {
					int found_token;
					if(set_smooth) {
						duplication = TRUE;
						break;
					}
					Pgm.Shift();
					found_token = Pgm.LookupTableForCurrentToken(plot_smooth_tbl);
					Pgm.Shift();
					switch(found_token) {
						case SMOOTH_BINS:
						    // catch the "bins" keyword by itself on the next pass 
						    Pgm.Rollback();
						    continue;
						case SMOOTH_UNWRAP:
						case SMOOTH_FREQUENCY:
						case SMOOTH_FREQUENCY_NORMALISED:
						    p_plot->plot_smooth = (PLOT_SMOOTH)found_token;
						    break;
						case SMOOTH_KDENSITY:
						    ParseKDensityOptions(p_plot);
						/* Fall through */
						case SMOOTH_ACSPLINES:
						case SMOOTH_BEZIER:
						case SMOOTH_CSPLINES:
						case SMOOTH_SBEZIER:
						case SMOOTH_UNIQUE:
						case SMOOTH_CUMULATIVE:
						case SMOOTH_CUMULATIVE_NORMALISED:
						case SMOOTH_MONOTONE_CSPLINE:
						case SMOOTH_PATH:
						    p_plot->plot_smooth = (PLOT_SMOOTH)found_token;
						    p_plot->plot_style = LINES; /* can override later */
						    break;
						case SMOOTH_ZSORT:
						    p_plot->plot_smooth = SMOOTH_ZSORT;
						    p_plot->plot_style = POINTSTYLE;
						    break;
						case SMOOTH_NONE:
						default:
						    IntErrorCurToken("unrecognized 'smooth' option");
						    break;
					}
					set_smooth = TRUE;
					continue;
				}
				/* look for axes/axis */
				if(Pgm.AlmostEqualsCur("ax$es")|| Pgm.AlmostEqualsCur("ax$is")) {
					if(set_axes) {
						duplication = TRUE;
						break;
					}
					if(Gg.Parametric && Gg.InParametric)
						IntErrorCurToken("previous parametric function not fully specified");
					Pgm.Shift();
					switch(Pgm.LookupTableForCurrentToken(&plot_axes_tbl[0])) {
						case AXES_X1Y1:
						    AxS.Idx_X = FIRST_X_AXIS;
						    AxS.Idx_Y = FIRST_Y_AXIS;
						    Pgm.Shift();
						    break;
						case AXES_X2Y2:
						    AxS.Idx_X = SECOND_X_AXIS;
						    AxS.Idx_Y = SECOND_Y_AXIS;
						    Pgm.Shift();
						    break;
						case AXES_X1Y2:
						    AxS.Idx_X = FIRST_X_AXIS;
						    AxS.Idx_Y = SECOND_Y_AXIS;
						    Pgm.Shift();
						    break;
						case AXES_X2Y1:
						    AxS.Idx_X = SECOND_X_AXIS;
						    AxS.Idx_Y = FIRST_Y_AXIS;
						    Pgm.Shift();
						    break;
						case AXES_NONE:
						default:
						    IntErrorCurToken("axes must be x1y1, x1y2, x2y1 or x2y2");
						    break;
					}
					set_axes = TRUE;
					continue;
				}
				// Allow this plot not to affect autoscaling 
				if(Pgm.AlmostEqualsCur("noauto$scale")) {
					Pgm.Shift();
					p_plot->noautoscale = TRUE;
					continue;
				}
				// deal with title 
				ParsePlotTitle(p_plot, xtitle, NULL, &set_title);
				if(save_token != Pgm.GetCurTokenIdx())
					continue;
				// deal with style 
				if(Pgm.AlmostEqualsCur("w$ith")) {
					if(set_with) {
						duplication = TRUE;
						break;
					}
					if(Gg.Parametric && Gg.InParametric)
						IntErrorCurToken("\"with\" allowed only after parametric function fully specified");
					p_plot->plot_style = GetStyle();
					if(p_plot->plot_style == FILLEDCURVES) {
						// read a possible option for 'with filledcurves' 
						GetFilledCurvesStyleOptions(&p_plot->filledcurves_options);
					}
					if(oneof3(p_plot->plot_style, IMAGE, RGBIMAGE, RGBA_IMAGE)) {
						if(p_plot->plot_type != DATA)
							IntErrorCurToken("This plot style is only for data files");
						else
							GetImageOptions(&p_plot->image_properties);
					}
					if((p_plot->plot_type == FUNC) && ((p_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR) || 
						oneof3(p_plot->plot_style, LABELPOINTS, PARALLELPLOT, SPIDERPLOT))) {
						IntWarnCurToken("This plot style is only for datafiles, reverting to \"points\"");
						p_plot->plot_style = POINTSTYLE;
					}
					set_with = TRUE;
					continue;
				}
				if(p_plot->plot_style == TABLESTYLE) {
					if(Pgm.EqualsCur("if")) {
						if(Tab.P_FilterAt) {
							duplication = TRUE;
							break;
						}
						Pgm.Shift();
						Tab.P_FilterAt = PermAt();
						continue;
					}
				}
				/* pick up line/point specs and other style-specific keywords
				 * - point spec allowed if style uses points, ie style&2 != 0
				 * - keywords for lt and pt are optional
				 */
				if(p_plot->plot_style == CANDLESTICKS) {
					if(Pgm.AlmostEqualsCur("whisker$bars")) {
						p_plot->arrow_properties.head = BOTH_HEADS;
						Pgm.Shift();
						if(Pgm.IsANumber(Pgm.GetCurTokenIdx()) || Pgm.TypeUdv(Pgm.GetCurTokenIdx()) == INTGR || Pgm.TypeUdv(Pgm.GetCurTokenIdx()) == CMPLX)
							p_plot->arrow_properties.head_length = RealExpression();
					}
				}
				if(p_plot->plot_style == PARALLELPLOT) {
					if(Pgm.EqualsCur("at")) {
						Pgm.Shift();
						paxis_x = RealExpression();
						continue;
					}
				}
				if(p_plot->plot_style & PLOT_STYLE_HAS_VECTOR) {
					int stored_token = Pgm.GetCurTokenIdx();
					if(!set_lpstyle) {
						default_arrow_style(&(p_plot->arrow_properties));
						if(Gg.PreferLineStyles)
							LpUseProperties(pTerm, &(p_plot->arrow_properties.lp_properties), line_num+1);
						else
							LoadLineType(pTerm, &(p_plot->arrow_properties.lp_properties), line_num+1);
					}
					ArrowParse(&(p_plot->arrow_properties), TRUE);
					if(stored_token != Pgm.GetCurTokenIdx()) {
						if(set_lpstyle) {
							duplication = TRUE;
							break;
						}
						else {
							set_lpstyle = TRUE;
							continue;
						}
					}
				}
				// pick up the special 'units' keyword the 'ellipses' style allows 
				if(p_plot->plot_style == ELLIPSES) {
					int stored_token = Pgm.GetCurTokenIdx();
					if(!set_ellipseaxes_units)
						p_plot->ellipseaxes_units = Gg.default_ellipse.o.ellipse.type;
					if(Pgm.AlmostEqualsCur("unit$s")) {
						Pgm.Shift();
						if(Pgm.EqualsCur("xy")) {
							p_plot->ellipseaxes_units = ELLIPSEAXES_XY;
						}
						else if(Pgm.EqualsCur("xx")) {
							p_plot->ellipseaxes_units = ELLIPSEAXES_XX;
						}
						else if(Pgm.EqualsCur("yy")) {
							p_plot->ellipseaxes_units = ELLIPSEAXES_YY;
						}
						else {
							IntErrorCurToken("expecting 'xy', 'xx' or 'yy'");
						}
						Pgm.Shift();
					}
					if(stored_token != Pgm.GetCurTokenIdx()) {
						if(set_ellipseaxes_units) {
							duplication = TRUE;
							break;
						}
						else {
							set_ellipseaxes_units = TRUE;
							continue;
						}
					}
				}
				// Most plot styles accept line and point properties 
				// but do not want font or text properties           
				if(p_plot->plot_style != LABELPOINTS) {
					int stored_token = Pgm.GetCurTokenIdx();
					lp_style_type lp(lp_style_type::defCommon); //= DEFAULT_LP_STYLE_TYPE;
					int new_lt = 0;
					lp.l_type = line_num;
					lp.PtType = line_num;
					lp.d_type = line_num;
					// user may prefer explicit line styles 
					if(Gg.PreferLineStyles)
						LpUseProperties(pTerm, &lp, line_num+1);
					else
						LoadLineType(pTerm, &lp, line_num+1);
					if(p_plot->plot_style == BOXPLOT) {
						lp.PtType = Gg.boxplot_opts.pointtype;
						lp.PtSize = PTSZ_DEFAULT;
						if(!Gg.boxplot_opts.outliers)
							p_plot->noautoscale = TRUE;
					}
					if(p_plot->plot_style == SPIDERPLOT) {
						lp = Gg.SpiderPlotStyle.lp_properties;
					}
					new_lt = LpParse(pTerm, &lp, LP_ADHOC, p_plot->plot_style & PLOT_STYLE_HAS_POINT);
					if(stored_token != Pgm.GetCurTokenIdx()) {
						if(set_lpstyle) {
							duplication = TRUE;
							break;
						}
						else {
							p_plot->lp_properties = lp;
							set_lpstyle = TRUE;
							if(new_lt)
								p_plot->base_linetype = new_lt - 1;
							continue;
						}
					}
				}
				// Labels can have font and text property info as plot options
				// In any case we must allocate one instance of the text style
				// that all labels in the plot will share.                     
				if((p_plot->plot_style == LABELPOINTS) || (p_plot->plot_style & PLOT_STYLE_HAS_POINT && p_plot->lp_properties.PtType == PT_CHARACTER)) {
					int stored_token = Pgm.GetCurTokenIdx();
					if(p_plot->labels == NULL) {
						p_plot->labels = new_text_label(-1);
						p_plot->labels->pos = CENTRE;
						p_plot->labels->layer = LAYER_PLOTLABELS;
					}
					if((p_plot->plot_style & PLOT_STYLE_HAS_POINT) && p_plot->lp_properties.PtType == PT_CHARACTER) {
						if(!set_labelstyle)
							p_plot->labels->textcolor.type = TC_DEFAULT;
					}
					ParseLabelOptions(p_plot->labels, 2);
					if(stored_token != Pgm.GetCurTokenIdx()) {
						if(set_labelstyle) {
							duplication = TRUE;
							break;
						}
						else {
							set_labelstyle = TRUE;
							continue;
						}
					}
				}
				// Some plots have a fill style as well 
				if(p_plot->plot_style & PLOT_STYLE_HAS_FILL) {
					int stored_token = Pgm.GetCurTokenIdx();
					if(Pgm.EqualsCur("fs") || Pgm.AlmostEqualsCur("fill$style")) {
						if(p_plot->plot_style == SPIDERPLOT)
							p_plot->fill_properties = Gg.SpiderPlotStyle.fillstyle;
						else
							p_plot->fill_properties = Gg.default_fillstyle;
						p_plot->fill_properties.fillpattern = pattern_num;
						ParseFillStyle(&p_plot->fill_properties);
						if(p_plot->plot_style == FILLEDCURVES && p_plot->fill_properties.fillstyle == FS_EMPTY)
							p_plot->fill_properties.fillstyle = FS_SOLID;
						set_fillstyle = TRUE;
					}
					if(Pgm.EqualsCur("fc") || Pgm.AlmostEqualsCur("fillc$olor")) {
						ParseColorSpec(&fillcolor, TC_VARIABLE);
						set_fillcolor = TRUE;
					}
					if(stored_token != Pgm.GetCurTokenIdx())
						continue;
				}
				break; /* unknown option */
			} /* while (!Pgm.EndOfCommand()) */
			if(duplication)
				IntErrorCurToken("duplicated or contradicting arguments in plot options");
			if(p_plot->plot_style == TABLESTYLE) {
				if(!Tab.Mode)
					IntError(NO_CARET, "'with table' requires a previous 'set table'");
				ExpectString(-1);
				some_tables = TRUE;
			}

			if(p_plot->plot_style == SPIDERPLOT && !Gg.SpiderPlot)
				IntError(NO_CARET, "'with spiderplot' requires a previous 'set spiderplot'");
#if (0)
			if(Gg.SpiderPlot && p_plot->plot_style != SPIDERPLOT)
				IntError(NO_CARET, "only plots 'with spiderplot' are possible in spiderplot mode");
#endif
			// set default values for title if this has not been specified 
			p_plot->title_is_automated = FALSE;
			if(!set_title) {
				p_plot->title_no_enhanced = TRUE; /* filename or function cannot be enhanced */
				if(key->auto_titles == FILENAME_KEYTITLES) {
					Pgm.MCapture(&(p_plot->title), start_token, end_token);
					if(Gg.InParametric)
						xtitle = p_plot->title;
					p_plot->title_is_automated = TRUE;
				}
				else if(xtitle)
					xtitle[0] = '\0';
			}
			// Vectors will be drawn using linetype from arrow style, so we
			// copy this to overall plot linetype so that the key sample matches 
			if(p_plot->plot_style & PLOT_STYLE_HAS_VECTOR) {
				if(!set_lpstyle) {
					if(Gg.PreferLineStyles)
						LpUseProperties(pTerm, &(p_plot->arrow_properties.lp_properties), line_num+1);
					else
						LoadLineType(pTerm, &(p_plot->arrow_properties.lp_properties), line_num+1);
					ArrowParse(&p_plot->arrow_properties, TRUE);
				}
				p_plot->lp_properties = p_plot->arrow_properties.lp_properties;
				set_lpstyle = TRUE;
			}
			// No line/point style given. As lp_parse also supplies
			// the defaults for linewidth and pointsize, call it now
			// to define them. 
			if(!set_lpstyle) {
				p_plot->lp_properties.l_type = line_num;
				p_plot->lp_properties.l_width = 1.0;
				p_plot->lp_properties.PtType = line_num;
				p_plot->lp_properties.d_type = line_num;
				p_plot->lp_properties.PtSize = Gg.PointSize;
				// user may prefer explicit line styles 
				if(Gg.PreferLineStyles)
					LpUseProperties(pTerm, &p_plot->lp_properties, line_num+1);
				else
					LoadLineType(pTerm, &p_plot->lp_properties, line_num+1);
				if(p_plot->plot_style == BOXPLOT) {
					p_plot->lp_properties.PtType = Gg.boxplot_opts.pointtype;
					p_plot->lp_properties.PtSize = PTSZ_DEFAULT;
				}
				if(p_plot->plot_style == SPIDERPLOT) {
					p_plot->lp_properties.PtType = Gg.SpiderPlotStyle.lp_properties.PtType;
					p_plot->lp_properties.PtSize = Gg.SpiderPlotStyle.lp_properties.PtSize;
					p_plot->lp_properties.l_width = Gg.SpiderPlotStyle.lp_properties.l_width;
					p_plot->lp_properties.pm3d_color.type = TC_DEFAULT;
				}
				LpParse(pTerm, &p_plot->lp_properties, LP_ADHOC, p_plot->plot_style & PLOT_STYLE_HAS_POINT);
			}
			// If this plot style uses a fillstyle and we saw an explicit 
			// fill color, save it in lp_properties now.		  
			if(p_plot->plot_style & PLOT_STYLE_HAS_FILL) {
				if(set_fillcolor)
					p_plot->lp_properties.pm3d_color = fillcolor;
			}
			// Some low-level routines expect to find the pointflag attribute 
			// in lp_properties (they don't have access to the full header.   
			if(p_plot->plot_style & PLOT_STYLE_HAS_POINT)
				p_plot->lp_properties.flags |= LP_SHOW_POINTS;
			// Rule out incompatible line/point/style options 
			if(p_plot->plot_type == FUNC) {
				if((p_plot->plot_style & PLOT_STYLE_HAS_POINT) && (p_plot->lp_properties.PtSize == PTSZ_VARIABLE))
					p_plot->lp_properties.PtSize = 1.0;
			}
			if(Gg.Polar) 
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
					    IntError(NO_CARET, "This plot style is not available in polar mode");
				}
			// If we got this far without initializing the fill style, do it now 
			if(p_plot->plot_style & PLOT_STYLE_HAS_FILL) {
				if(!set_fillstyle) {
					p_plot->fill_properties = (p_plot->plot_style == SPIDERPLOT) ? Gg.SpiderPlotStyle.fillstyle : Gg.default_fillstyle;
					p_plot->fill_properties.fillpattern = pattern_num;
					ParseFillStyle(&p_plot->fill_properties);
				}
				if(oneof2(p_plot->fill_properties.fillstyle, FS_PATTERN, FS_TRANSPARENT_PATTERN))
					pattern_num = p_plot->fill_properties.fillpattern + 1;
				if(p_plot->plot_style == FILLEDCURVES && p_plot->fill_properties.fillstyle == FS_EMPTY)
					p_plot->fill_properties.fillstyle = FS_SOLID;
			}
			p_plot->AxIdx_X = AxS.Idx_X;
			p_plot->AxIdx_Y = AxS.Idx_Y;
			// If we got this far without initializing the character font, do it now 
			if(p_plot->plot_style & PLOT_STYLE_HAS_POINT && p_plot->lp_properties.PtType == PT_CHARACTER) {
				if(p_plot->labels == NULL) {
					p_plot->labels = new_text_label(-1);
					p_plot->labels->pos = CENTRE;
					ParseLabelOptions(p_plot->labels, 2);
				}
			}
			// If we got this far without initializing the label list, do it now 
			if(p_plot->plot_style == LABELPOINTS) {
				if(p_plot->labels == NULL) {
					p_plot->labels = new_text_label(-1);
					p_plot->labels->pos = CENTRE;
					p_plot->labels->layer = LAYER_PLOTLABELS;
				}
				p_plot->labels->place.scalex = (AxS.Idx_X == SECOND_X_AXIS) ? second_axes : first_axes;
				p_plot->labels->place.scaley = (AxS.Idx_Y == SECOND_Y_AXIS) ? second_axes : first_axes;
				// Needed for variable color - June 2010 
				p_plot->lp_properties.pm3d_color = p_plot->labels->textcolor;
				if(p_plot->labels->textcolor.type == TC_VARIABLE)
					p_plot->lp_properties.l_type = LT_COLORFROMCOLUMN;
				// We want to trigger the variable color mechanism even if
				// there was no 'textcolor variable/palette/rgb var' ,
				// but there was a 'point linecolor variable/palette/rgb var'. 
				if((p_plot->labels->lp_properties.flags & LP_SHOW_POINTS) && p_plot->labels->textcolor.type != TC_Z && 
					p_plot->labels->textcolor.type != TC_VARIABLE && (p_plot->labels->textcolor.type != TC_RGB || 
					p_plot->labels->textcolor.value >= 0)) {
					if((p_plot->labels->lp_properties.pm3d_color.type == TC_RGB) && (p_plot->labels->lp_properties.pm3d_color.value < 0)) {
						p_plot->lp_properties.pm3d_color = p_plot->labels->lp_properties.pm3d_color;
					}
					if(p_plot->labels->lp_properties.pm3d_color.type == TC_Z)
						p_plot->lp_properties.pm3d_color.type = TC_Z;
					if(p_plot->labels->lp_properties.l_type == LT_COLORFROMCOLUMN)
						p_plot->lp_properties.l_type = LT_COLORFROMCOLUMN;
				}
			}
			// We can skip a lot of stuff if this is not a real plot 
			if(p_plot->plot_type == KEYENTRY)
				goto SKIPPED_EMPTY_FILE;
			// Initialize the label list in case the BOXPLOT style needs it to store factors 
			if(p_plot->plot_style == BOXPLOT) {
				SETIFZ(p_plot->labels, new_text_label(-1));
				// We only use the list to store strings, so this is all we need here. 
			}
			// Initialize histogram data structure 
			if(p_plot->plot_style == HISTOGRAMS) {
				if(AxS[AxS.Idx_X].log)
					IntErrorCurToken("Log scale on X is incompatible with histogram plots\n");
				if((Gg.histogram_opts.type == HT_STACKED_IN_LAYERS || Gg.histogram_opts.type == HT_STACKED_IN_TOWERS) && AxS[AxS.Idx_Y].log)
					IntErrorCurToken("Log scale on Y is incompatible with stacked histogram plot\n");
				p_plot->histogram_sequence = ++histogram_sequence;
				// Current histogram always goes at the front of the list 
				if(p_plot->histogram_sequence == 0) {
					p_plot->histogram = (histogram_style *)SAlloc::M(sizeof(histogram_style));
					InitHistogram(p_plot->histogram, &_Plt.histogram_title);
					p_plot->histogram->start = newhist_start;
					p_plot->histogram->startcolor = newhist_color;
					p_plot->histogram->startpattern = newhist_pattern;
				}
				else {
					p_plot->histogram = Gg.histogram_opts.next;
					p_plot->histogram->clustersize++;
				}
				// Normally each histogram gets a new set of colors, but in 
				// 'newhistogram' you can force a starting color instead.   
				if(!set_lpstyle && p_plot->histogram->startcolor != LT_UNDEFINED)
					LoadLineType(pTerm, &p_plot->lp_properties, p_plot->histogram_sequence + p_plot->histogram->startcolor);
				if(p_plot->histogram->startpattern != LT_UNDEFINED)
					p_plot->fill_properties.fillpattern = p_plot->histogram_sequence + p_plot->histogram->startpattern;
			}
			// Parallel plot data bookkeeping 
			if(oneof2(p_plot->plot_style, PARALLELPLOT, SPIDERPLOT)) {
				if(_Plt.paxis_start < 0) {
					_Plt.paxis_start = 1;
					_Plt.paxis_current = 0;
				}
				_Plt.paxis_current++;
				_Plt.paxis_end = _Plt.paxis_current;
				if(_Plt.paxis_current > AxS.GetParallelAxisCount())
					AxS.ExtendParallelAxis(_Plt.paxis_current);
				p_plot->AxIdx_P = (AXIS_INDEX)_Plt.paxis_current;
				AxS.Parallel(_Plt.paxis_current-1).Init(true);
				AxS.Parallel(_Plt.paxis_current-1).paxis_x = (paxis_x > -VERYLARGE) ? paxis_x : (double)_Plt.paxis_current;
			}
			// Currently polygons are just treated as filled curves 
			if(p_plot->plot_style == POLYGONS) {
				p_plot->filledcurves_options.closeto = FILLEDCURVES_CLOSED;
			}
			// Styles that use palette 
			// we can now do some checks that we deferred earlier 
			if(p_plot->plot_type == DATA) {
				if(specs < 0) {
					// Error check to handle missing or unreadable file 
					++line_num;
					p_plot->plot_type = NODATA;
					goto SKIPPED_EMPTY_FILE;
				}
				// Reset flags to auto-scale X axis to contents of data set 
				if(!(uses_axis[AxS.Idx_X] & USES_AXIS_FOR_DATA) && AxS.__X().autoscale) {
					GpAxis * scaling_axis = &AxS[p_plot->AxIdx_X];
					if(scaling_axis->autoscale & AUTOSCALE_MIN)
						scaling_axis->min = VERYLARGE;
					if(scaling_axis->autoscale & AUTOSCALE_MAX)
						scaling_axis->max = -VERYLARGE;
				}
				if(AxS.__X().datatype == DT_TIMEDATE) {
					if(specs < 2)
						IntErrorCurToken("Need full using spec for x time data");
				}
				if(AxS.__Y().datatype == DT_TIMEDATE) {
					if(specs < 1)
						IntErrorCurToken("Need using spec for y time data");
				}
				// NB: df_axis is used only for timedate data and 3D cbticlabels 
				_Df.df_axis[0] = AxS.Idx_X;
				_Df.df_axis[1] = AxS.Idx_Y;
				// separate record of datafile and func 
				uses_axis[AxS.Idx_X] |= USES_AXIS_FOR_DATA;
				uses_axis[AxS.Idx_Y] |= USES_AXIS_FOR_DATA;
			}
			else if(!Gg.Parametric || !Gg.InParametric) {
				// for x part of a parametric function, axes are possibly wrong 
				// separate record of data and func 
				uses_axis[AxS.Idx_X] |= USES_AXIS_FOR_FUNC;
				uses_axis[AxS.Idx_Y] |= USES_AXIS_FOR_FUNC;
			}
			// These plot styles are not differentiated by line/point properties 
			if(!Gg.InParametric && p_plot->plot_style != IMAGE && p_plot->plot_style != RGBIMAGE && p_plot->plot_style != RGBA_IMAGE) {
				++line_num;
			}
			// Image plots require 2 input dimensions 
			if(oneof3(p_plot->plot_style, IMAGE, RGBIMAGE, RGBA_IMAGE)) {
				if(!_Df.df_filename || sstreq(_Df.df_filename, "+"))
					IntError(NO_CARET, "image plots need more than 1 coordinate dimension ");
			}
			if(p_plot->plot_type == DATA) {
				// get_data() will update the ranges of autoscaled axes, but some 
				// plot modes, e.g. 'smooth cnorm' and 'boxplot' with nooutliers, 
				// do not want all the points included in autoscaling.  Save the  
				// current autoscaled ranges here so we can restore them later.   
				AxS.SaveAutoscaledRanges(&AxS[p_plot->AxIdx_X], &AxS[p_plot->AxIdx_Y]);
				// actually get the data now 
				if(GetData(p_plot) == 0) {
					if(!forever_iteration(_Pb.plot_iterator))
						IntWarn(NO_CARET, "Skipping data file with no valid points");
					p_plot->plot_type = NODATA;
					goto SKIPPED_EMPTY_FILE;
				}
				// Sep 2017 - Check for all points bad or out of range  
				// (normally harmless but must not cause infinite loop) 
				if(forever_iteration(_Pb.plot_iterator)) {
					int ninrange = 0;
					for(int n = 0; n < p_plot->p_count; n++)
						if(p_plot->points[n].type == INRANGE)
							ninrange++;
					if(ninrange == 0) {
						p_plot->plot_type = NODATA;
						goto SKIPPED_EMPTY_FILE;
					}
				}
				// If we are to bin the data, do that first 
				if(p_plot->plot_smooth == SMOOTH_BINS) {
					MakeBins(p_plot, nbins, binlow, binhigh, binwidth);
				}
				// Restore auto-scaling prior to smoothing operation 
				switch(p_plot->plot_smooth) {
					case SMOOTH_FREQUENCY:
					case SMOOTH_FREQUENCY_NORMALISED:
					case SMOOTH_CUMULATIVE:
					case SMOOTH_CUMULATIVE_NORMALISED:
					    AxS.RestoreAutoscaledRanges(&AxS[p_plot->AxIdx_X], &AxS[p_plot->AxIdx_Y]);
					    break;
					default:
					    break;
				}
				// Fiddle the auto-scaling data for specific plot styles 
				if(p_plot->plot_style == HISTOGRAMS)
					HistogramRangeFiddling(p_plot);
				if(p_plot->plot_style == BOXES)
					BoxRangeFiddling(p_plot);
				if(p_plot->plot_style == BOXPLOT)
					BoxPlotRangeFiddling(p_plot);
				if(p_plot->plot_style == IMPULSES)
					ImpulseRangeFiddling(p_plot);
				if(Gg.Polar)
					PolarRangeFiddling(p_plot);
				// sort 
				switch(p_plot->plot_smooth) {
					// sort and average, if the style requires 
					case SMOOTH_UNIQUE:
					case SMOOTH_FREQUENCY:
					case SMOOTH_FREQUENCY_NORMALISED:
					case SMOOTH_CUMULATIVE:
					case SMOOTH_CUMULATIVE_NORMALISED:
					case SMOOTH_CSPLINES:
					case SMOOTH_ACSPLINES:
					case SMOOTH_SBEZIER:
					case SMOOTH_MONOTONE_CSPLINE:
					    sort_points(p_plot);
					    CpImplode(p_plot);
					    break;
					case SMOOTH_ZSORT:
					    zsort_points(p_plot);
					    break;
					case SMOOTH_NONE:
					case SMOOTH_PATH:
					case SMOOTH_BEZIER:
					case SMOOTH_KDENSITY:
					default:
					    break;
				}
				switch(p_plot->plot_smooth) {
					// create new data set by evaluation of interpolation routines 
					case SMOOTH_UNWRAP:
					    GenInterpUnwrap(p_plot);
					    break;
					case SMOOTH_FREQUENCY:
					case SMOOTH_FREQUENCY_NORMALISED:
					case SMOOTH_CUMULATIVE:
					case SMOOTH_CUMULATIVE_NORMALISED:
					    // These commands all replace the original data  
					    // so we must reevaluate min/max for autoscaling 
					    GenInterpFrequency(p_plot);
					    RefreshBounds(p_plot, 1);
					    break;
					case SMOOTH_CSPLINES:
					case SMOOTH_ACSPLINES:
					case SMOOTH_BEZIER:
					case SMOOTH_SBEZIER:
					    GenInterp(p_plot);
					    RefreshBounds(p_plot, 1);
					    break;
					case SMOOTH_KDENSITY:
					    GenInterp(p_plot);
					    Ev.FillGpValFoat("GPVAL_KDENSITY_BANDWIDTH",
						fabs(p_plot->smooth_parameter));
					    break;
					case SMOOTH_MONOTONE_CSPLINE: McsInterp(p_plot); break;
					case SMOOTH_PATH: Gen2DPathSplines(p_plot); break;
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
					ProcessImage(pTerm, p_plot, IMG_UPDATE_AXES);
				}
			}
SKIPPED_EMPTY_FILE:
			// Note position in command line for second pass 
			p_plot->token = Pgm.GetCurTokenIdx();
			tp_ptr = &(p_plot->next);
			// restore original value of sample variables 
			if(name_str) {
				p_plot->sample_var->udv_value = original_value_sample_var;
				p_plot->sample_var2->udv_value = original_value_sample_var2;
			}
		} /* !is_defn */
		if(Gg.InParametric) {
			if(Pgm.EqualsCur(",")) {
				Pgm.Shift();
				continue;
			}
			else
				break;
		}
		// Iterate-over-plot mechanism 
		if(empty_iteration(_Pb.plot_iterator) && p_plot)
			p_plot->plot_type = NODATA;
		if(forever_iteration(_Pb.plot_iterator) && !p_plot)
			IntError(NO_CARET, "unbounded iteration in something other than a data plot");
		else if(forever_iteration(_Pb.plot_iterator) && (p_plot->plot_type == NODATA)) {
			FPRINTF((stderr, "Ending * iteration at %d\n", _Pb.plot_iterator->iteration));
			/* Clearing the plot title ensures that it will not appear in the key */
			ZFREE(p_plot->title);
		}
		else if(forever_iteration(_Pb.plot_iterator) && (p_plot->plot_type != DATA)) {
			IntError(NO_CARET, "unbounded iteration in something other than a data plot");
		}
		else if(NextIteration(_Pb.plot_iterator)) {
			Pgm.SetTokenIdx(start_token);
			continue;
		}
		_Pb.plot_iterator = cleanup_iteration(_Pb.plot_iterator);
		if(Pgm.EqualsCur(",")) {
			Pgm.Shift();
			_Pb.plot_iterator = CheckForIteration();
		}
		else
			break;
	}
	if(Gg.Parametric && Gg.InParametric)
		IntError(NO_CARET, "parametric function not fully specified");
/*** Second Pass: Evaluate the functions ***/
	/*
	 * Everything is defined now, except the function data. We expect
	 * no syntax errors, etc, since the above parsed it all. This
	 * makes the code below simpler. If y is autoscaled, the yrange
	 * may still change.  we stored last token of each plot, so we
	 * dont need to do everything again */

	// parametric or polar fns can still affect x ranges 
	if(!Gg.Parametric && !Gg.Polar) {
		/* If we were expecting to autoscale on X but found no usable
		 * points in the data files, then the axis limits are still sitting
		 * at +/- VERYLARGE.  The default range for bare functions is [-10:10].
		 * Or we could give up and fall through to "x range invalid".
		 */
		if((some_functions || some_tables) && uses_axis[FIRST_X_AXIS])
			if(AxS[FIRST_X_AXIS].max == -VERYLARGE || AxS[FIRST_X_AXIS].min == VERYLARGE) {
				AxS[FIRST_X_AXIS].min = -10;
				AxS[FIRST_X_AXIS].max = 10;
			}
		if(uses_axis[FIRST_X_AXIS] & USES_AXIS_FOR_DATA) {
			AxisCheckedExtendEmptyRange(FIRST_X_AXIS, "x range is invalid"); // check that x1min -> x1max is not too small 
		}
		if(uses_axis[SECOND_X_AXIS] & USES_AXIS_FOR_DATA) {
			AxisCheckedExtendEmptyRange(SECOND_X_AXIS, "x2 range is invalid"); // check that x2min -> x2max is not too small 
		}
		else if(AxS[SECOND_X_AXIS].autoscale) {
			// copy x1's range 
			// FIXME:  merge both cases into update_secondary_axis_range 
			if(AxS[SECOND_X_AXIS].linked_to_primary) {
				UpdateSecondaryAxisRange(&AxS[FIRST_X_AXIS]);
			}
			else {
				if(AxS[SECOND_X_AXIS].autoscale & AUTOSCALE_MIN)
					AxS[SECOND_X_AXIS].min = AxS[FIRST_X_AXIS].min;
				if(AxS[SECOND_X_AXIS].autoscale & AUTOSCALE_MAX)
					AxS[SECOND_X_AXIS].max = AxS[FIRST_X_AXIS].max;
			}
		}
	}
	if(some_functions) {
		// call the controlled variable t, since x_min can also mean smallest x 
		double t_min = 0., t_max = 0., t_step = 0.;
		if(Gg.Parametric || Gg.Polar) {
			if(!(uses_axis[FIRST_X_AXIS] & USES_AXIS_FOR_DATA)) {
				// these have not yet been set to full width 
				if(AxS[FIRST_X_AXIS].autoscale & AUTOSCALE_MIN)
					AxS[FIRST_X_AXIS].min = VERYLARGE;
				if(AxS[FIRST_X_AXIS].autoscale & AUTOSCALE_MAX)
					AxS[FIRST_X_AXIS].max = -VERYLARGE;
			}
			if(!(uses_axis[SECOND_X_AXIS] & USES_AXIS_FOR_DATA)) {
				if(AxS[SECOND_X_AXIS].autoscale & AUTOSCALE_MIN)
					AxS[SECOND_X_AXIS].min = VERYLARGE;
				if(AxS[SECOND_X_AXIS].autoscale & AUTOSCALE_MAX)
					AxS[SECOND_X_AXIS].max = -VERYLARGE;
			}
		}
		if(Gg.Parametric || Gg.Polar) {
			t_min = AxS[T_AXIS].min;
			t_max = AxS[T_AXIS].max;
			t_step = (t_max - t_min) / (Gg.Samples1 - 1);
		}
		// else we'll do it on each plot (see below) 
		tp_ptr = &_Plt.P_FirstPlot;
		plot_num = 0;
		p_plot = _Plt.P_FirstPlot;
		Pgm.SetTokenIdx(begin_token); // start over 
		_Pb.plot_iterator = CheckForIteration();
		// Read through functions 
		while(TRUE) {
			if(!Gg.InParametric && !was_definition)
				start_token = Pgm.GetCurTokenIdx();
			if(IsDefinition(Pgm.GetCurTokenIdx())) {
				Define();
				if(Pgm.EqualsCur(","))
					Pgm.Shift();
				was_definition = TRUE;
				continue;
			}
			else {
				struct at_type * at_ptr;
				char * name_str;
				int sample_range_token;
				was_definition = FALSE;
				// Forgive trailing comma on a multi-element plot command 
				if(Pgm.EndOfCommand() || p_plot == NULL) {
					IntWarnCurToken("ignoring trailing comma in plot command");
					break;
				}
				// HBB 20000820: now globals in 'axis.c' 
				AxS.Idx_X = p_plot->AxIdx_X;
				AxS.Idx_Y = p_plot->AxIdx_Y;
				plot_num++;
				// Check for a sampling range. 
				// Only relevant to function plots, and only needed in second pass. 
				if(!Gg.Parametric && !Gg.Polar)
					AxS.InitSampleRange(&AxS[AxS.Idx_X], FUNC);
				sample_range_token = ParseRange(SAMPLE_AXIS);
				Pgm.dummy_func = &_Plt.Plot2D_Func;
				if(Pgm.AlmostEqualsCur("newhist$ogram")) {
					name_str = ""; // Make sure this isn't interpreted as a function 
				}
				else if(Pgm.AlmostEqualsCur("newspider$plot")) {
					name_str = "";
				}
				else if(Pgm.EqualsCur("keyentry")) {
					name_str = "";
				}
				else {
					/* Allow replacement of the dummy variable in a function */
					if(sample_range_token > 0)
						Pgm.CopyStr(_Pb.c_dummy_var[0], sample_range_token, MAX_ID_LEN);
					else if(sample_range_token < 0)
						strcpy(_Pb.c_dummy_var[0], _Pb.set_dummy_var[0]);
					else
						strcpy(_Pb.c_dummy_var[0], orig_dummy_var);
					// WARNING: do NOT free name_str 
					name_str = StringOrExpress(&at_ptr);
				}
				if(!name_str) { /* function to plot */
					if(Gg.Parametric) { // toggle parametric axes 
						Gg.InParametric = !Gg.InParametric;
					}
					if(p_plot->plot_style == TABLESTYLE)
						IntWarn(NO_CARET, "'with table' requires a data source not a pure function");
					_Plt.Plot2D_Func.at = at_ptr;
					if(!Gg.Parametric && !Gg.Polar) {
						t_min = AxS[SAMPLE_AXIS].min;
						t_max = AxS[SAMPLE_AXIS].max;
						if(AxS[SAMPLE_AXIS].linked_to_primary) {
							GpAxis * primary = AxS[SAMPLE_AXIS].linked_to_primary;
							if(primary->log && !(t_min > 0 && t_max > 0))
								IntError(NO_CARET, "logscaled axis must have positive range");
							t_min = EvalLinkFunction(primary, t_min);
							t_max = EvalLinkFunction(primary, t_max);
							FPRINTF((stderr, "sample range on primary axis: %g %g\n", t_min, t_max));
						}
						else {
							check_log_limits(&AxS.__X(), t_min, t_max);
						}
						t_step = (t_max - t_min) / (Gg.Samples1 - 1);
					}
					for(i = 0; i < Gg.Samples1; i++) {
						double x, temp;
						GpValue a;
						double t = t_min + i * t_step;
						if(Gg.Parametric) {
							// SAMPLE_AXIS is not relevant in parametric mode 
						}
						else if(AxS[SAMPLE_AXIS].linked_to_primary) {
							const GpAxis * vis = AxS[SAMPLE_AXIS].linked_to_primary->linked_to_secondary;
							t = EvalLinkFunction(vis, t_min + i * t_step);
						}
						else {
							/* Zero is often a special point in a function domain. */
							/* Make sure we don't miss it due to round-off error.  */
							if((fabs(t) < 1.e-9) && (fabs(t_step) > 1.e-6))
								t = 0.0;
						}
						x = t;
						Gcomplex(&_Plt.Plot2D_Func.dummy_values[0], x, 0.0);
						EvaluateAt(_Plt.Plot2D_Func.at, &a);
						if(Ev.IsUndefined_) {
							p_plot->points[i].type = UNDEFINED;
							continue;
						}
						// Imaginary values are treated as UNDEFINED 
						if(fabs(imag(&a)) > Gg.Zero && !isnan(real(&a))) {
							p_plot->points[i].type = UNDEFINED;
							_Plt.Plot2D_NComplexValues++;
							continue;
						}
						// Jan 2010 - initialize all fields! 
						memzero(&p_plot->points[i], sizeof(GpCoordinate));
						temp = real(&a);
						// width of box not specified 
						p_plot->points[i].z = -1.0;
						// for the moment 
						p_plot->points[i].type = INRANGE;
						if(Gg.Parametric) {
							// The syntax is plot x, y XnYnaxes
							// so we do not know the actual plot axes until
							// the y plot and cannot do range-checking now.
							p_plot->points[i].x = t;
							p_plot->points[i].y = temp;
							if(V.BoxWidth >= 0.0 && V.BoxWidthIsAbsolute)
								p_plot->points[i].z = 0.0;
						}
						else if(Gg.Polar) {
							double y;
							double theta = x;
							// Convert from polar to cartesian coordinates and check ranges
							if(PolarToXY(theta, temp, &x, &y, TRUE) == OUTRANGE)
								p_plot->points[i].type = OUTRANGE; ;
							if((p_plot->plot_style == FILLEDCURVES) && (p_plot->filledcurves_options.closeto == FILLEDCURVES_ATR)) {
								double xhigh, yhigh;
								PolarToXY(theta, p_plot->filledcurves_options.at, &xhigh, &yhigh, TRUE);
								STORE_AND_UPDATE_RANGE(p_plot->points[i].xhigh, xhigh, p_plot->points[i].type, AxS.Idx_X, p_plot->noautoscale, goto come_here_if_undefined);
								STORE_AND_UPDATE_RANGE(p_plot->points[i].yhigh, yhigh, p_plot->points[i].type, AxS.Idx_Y, p_plot->noautoscale, goto come_here_if_undefined);
							}
							STORE_AND_UPDATE_RANGE(p_plot->points[i].x, x, p_plot->points[i].type, AxS.Idx_X, p_plot->noautoscale, goto come_here_if_undefined);
							STORE_AND_UPDATE_RANGE(p_plot->points[i].y, y, p_plot->points[i].type, AxS.Idx_Y, p_plot->noautoscale, goto come_here_if_undefined);
						}
						else { // neither parametric or polar 
							p_plot->points[i].x = t;
							// A sampled function can only be OUTRANGE if it has a private range 
							if(sample_range_token != 0) {
								double xx = t;
								if(!AxS[p_plot->AxIdx_X].InRange(xx))
									p_plot->points[i].type = OUTRANGE;
							}
							// For boxes [only] check use of boxwidth 
							if(p_plot->plot_style == BOXES && V.BoxWidth >= 0.0 && V.BoxWidthIsAbsolute) {
								double xlow, xhigh;
								coord_type dmy_type = INRANGE;
								p_plot->points[i].z = 0;
								if(AxS[p_plot->AxIdx_X].log) {
									double base = AxS[p_plot->AxIdx_X].base;
									xlow  = x * pow(base, -V.BoxWidth/2.0);
									xhigh = x * pow(base, V.BoxWidth/2.0);
								}
								else {
									xlow  = x - V.BoxWidth/2.0;
									xhigh = x + V.BoxWidth/2.0;
								}
								STORE_AND_UPDATE_RANGE(p_plot->points[i].xlow, xlow, dmy_type, AxS.Idx_X, p_plot->noautoscale, NOOP);
								dmy_type = INRANGE;
								STORE_AND_UPDATE_RANGE(p_plot->points[i].xhigh, xhigh, dmy_type, AxS.Idx_X, p_plot->noautoscale, NOOP);
							}
							if(p_plot->plot_style == FILLEDCURVES) {
								p_plot->points[i].xhigh = p_plot->points[i].x;
								STORE_AND_UPDATE_RANGE(p_plot->points[i].yhigh, p_plot->filledcurves_options.at, p_plot->points[i].type, AxS.Idx_Y, TRUE, NOOP);
							}
							// Fill in additional fields needed to draw a circle 
							if(p_plot->plot_style == CIRCLES) {
								p_plot->points[i].z = DEFAULT_RADIUS;
								p_plot->points[i].ylow = 0;
								p_plot->points[i].xhigh = 360;
							}
							else if(p_plot->plot_style == ELLIPSES) {
								p_plot->points[i].z = DEFAULT_RADIUS;
								p_plot->points[i].ylow = Gg.default_ellipse.o.ellipse.orientation;
							}
							STORE_AND_UPDATE_RANGE(p_plot->points[i].y, temp, p_plot->points[i].type, Gg.InParametric ? AxS.Idx_X : AxS.Idx_Y, p_plot->noautoscale, goto come_here_if_undefined);
							/* could not use a continue in this case */
come_here_if_undefined:
							; /* ansi requires a statement after a label */
						}
					} /* loop over Gg.Samples1 */
					p_plot->p_count = i; /* Gg.Samples1 */
				}
				Pgm.SetTokenIdx(p_plot->token); // skip all modifiers func / whole of data plots 
				// used below 
				tp_ptr = &(p_plot->next);
				p_plot = p_plot->next;
			}
			if(Gg.InParametric) {
				if(Pgm.EqualsCur(",")) {
					Pgm.Shift();
					continue;
				}
			}
			// Iterate-over-plot mechanism 
			if(forever_iteration(_Pb.plot_iterator) && p_plot->plot_type == NODATA) {
				_Pb.plot_iterator = cleanup_iteration(_Pb.plot_iterator);
				Pgm.SetTokenIdx(start_token);
				continue;
			}
			if(NextIteration(_Pb.plot_iterator)) {
				Pgm.SetTokenIdx(start_token);
				continue;
			}
			_Pb.plot_iterator = cleanup_iteration(_Pb.plot_iterator);
			if(Pgm.EqualsCur(",")) {
				Pgm.Shift();
				_Pb.plot_iterator = CheckForIteration();
			}
			else
				break;
		}
		// when step debugging, set breakpoint here to get through
		// the 'read function' loop above quickly 
		if(Gg.Parametric) {
			// Now actually fix the plot pairs to be single plots
			// also fixes up polar&&parametric fn plots 
			ParametricFixup(_Plt.P_FirstPlot, &plot_num);
			// we omitted earlier check for range too small 
			AxisCheckedExtendEmptyRange(FIRST_X_AXIS, NULL);
			if(uses_axis[SECOND_X_AXIS]) {
				AxisCheckedExtendEmptyRange(SECOND_X_AXIS, NULL);
			}
		}
		// This is the earliest that polar autoscaling can be done for function plots 
		if(Gg.Polar)
			PolarRangeFiddling(_Plt.P_FirstPlot);
	}
	// if P_FirstPlot is NULL, we have no functions or data at all. This can
	// happen if you type "plot x=5", since x=5 is a variable assignment.
	if(!plot_num || !_Plt.P_FirstPlot) {
		IntErrorCurToken("no functions or data to plot");
	}
	// Is this too severe? 
	if(_Plt.Plot2D_NComplexValues > 3)
		IntWarn(NO_CARET, "Did you try to plot a complex-valued function?");
	if(!uses_axis[FIRST_X_AXIS] && !uses_axis[SECOND_X_AXIS])
		if(_Plt.P_FirstPlot->plot_type == NODATA)
			IntError(NO_CARET, "No data in plot");
	/* Parallelaxis plots do not use the normal y axis so if no other plots
	 * are present yrange may still be undefined. We fix that now.
	 * In the absence of parallelaxis plots this call does nothing.
	 */
	ParallelRangeFiddling(_Plt.P_FirstPlot);
	/* The x/y values stored during data entry for spider plots are not
	 * true x/y values.  Reset x/y ranges to [-1:+1].
	 */
	if(Gg.SpiderPlot)
		SpiderPlotRangeFiddling(_Plt.P_FirstPlot);
	/* gnuplot version 5.0 always used x1 to track autoscaled range
	 * regardless of whether x1 or x2 was used to plot the data.
	 * In version 5.2 we track the x1/x2 axis data limits separately.
	 * However if x1 and x2 are linked to each other we must now
	 * reconcile their data limits before plotting.
	 */
	if(uses_axis[FIRST_X_AXIS] && uses_axis[SECOND_X_AXIS]) {
		GpAxis * primary   = &AxS[FIRST_X_AXIS];
		GpAxis * secondary = &AxS[SECOND_X_AXIS];
		ReconcileLinkedAxes(primary, secondary);
	}
	if(uses_axis[FIRST_Y_AXIS] && uses_axis[SECOND_Y_AXIS]) {
		GpAxis * primary   = &AxS[FIRST_Y_AXIS];
		GpAxis * secondary = &AxS[SECOND_Y_AXIS];
		ReconcileLinkedAxes(primary, secondary);
	}
	if(uses_axis[FIRST_X_AXIS]) {
		if(AxS[FIRST_X_AXIS].max == -VERYLARGE || AxS[FIRST_X_AXIS].min == VERYLARGE)
			IntError(NO_CARET, "all points undefined!");
		AxS.CheckRange(FIRST_X_AXIS);
	}
	else {
		assert(uses_axis[SECOND_X_AXIS]);
	}
	if(uses_axis[SECOND_X_AXIS]) {
		if(AxS[SECOND_X_AXIS].max == -VERYLARGE || AxS[SECOND_X_AXIS].min == VERYLARGE)
			IntError(NO_CARET, "all points undefined!");
		AxS.CheckRange(SECOND_X_AXIS);
	}
	else {
		assert(uses_axis[FIRST_X_AXIS]);
	}
	// For nonlinear axes, but must also be compatible with "set link x".   
	// min/max values were tracked during input for the visible axes.       
	// Now we use them to update the corresponding shadow (nonlinear) ones. 
	UpdatePrimaryAxisRange(&AxS[FIRST_X_AXIS]);
	UpdatePrimaryAxisRange(&AxS[SECOND_X_AXIS]);
	if(p_plot && p_plot->plot_style == TABLESTYLE) {
		; // the y axis range has no meaning in this case 
	}
	else if(p_plot && p_plot->plot_style == PARALLELPLOT) {
		; // we should maybe check one of the parallel axes?
	}
	else if(uses_axis[FIRST_Y_AXIS] && AxS[FIRST_Y_AXIS].IsNonLinear()) {
		AxisCheckedExtendEmptyRange(FIRST_Y_AXIS, "all points y value undefined!");
		UpdatePrimaryAxisRange(&AxS[FIRST_Y_AXIS]);
	}
	else if(uses_axis[FIRST_Y_AXIS]) {
		AxisCheckedExtendEmptyRange(FIRST_Y_AXIS, "all points y value undefined!");
		AxS.CheckRange(FIRST_Y_AXIS);
	}
	if(uses_axis[SECOND_Y_AXIS] && AxS[SECOND_Y_AXIS].linked_to_primary) {
		AxisCheckedExtendEmptyRange(SECOND_Y_AXIS, "all points y2 value undefined!");
		UpdatePrimaryAxisRange(&AxS[SECOND_Y_AXIS]);
	}
	else if(uses_axis[SECOND_Y_AXIS]) {
		AxisCheckedExtendEmptyRange(SECOND_Y_AXIS, "all points y2 value undefined!");
		AxS.CheckRange(SECOND_Y_AXIS);
	}
	else {
		assert(uses_axis[FIRST_Y_AXIS]);
	}
	// This call cannot be in boundary(), called from do_plot(), because
	// it would cause logscaling problems if do_plot() itself was called for
	// refresh rather than for plot/replot.
	SetPlotWithPalette(0, MODE_PLOT);
	if(IsPlotWithPalette())
		SetCbMinMax();
	// if we get here, all went well, so record this line for replot 
	if(Pgm.plot_token != -1) {
		// note that m_capture also frees the old replot_line 
		Pgm.MCapture(&Pgm.replot_line, Pgm.plot_token, Pgm.GetPrevTokenIdx());
		Pgm.plot_token = -1;
		Ev.FillGpValString("GPVAL_LAST_PLOT", Pgm.replot_line);
	}
	if(Tab.Mode) {
		PrintTable(_Plt.P_FirstPlot, plot_num);
	}
	else {
		DoPlot(pTerm, _Plt.P_FirstPlot, plot_num);
		/* after do_plot(), AxS[].min and .max
		 * contain the plotting range actually used (rounded
		 * to tic marks, not only the min/max data values)
		 *  --> save them now for writeback if requested
		 */
		SaveWritebackAllAxes();
		// Mark these plots as safe for quick refresh 
		SET_REFRESH_OK(E_REFRESH_OK_2D, plot_num);
	}
	UpdateGpvalVariables(1); // update GPVAL_ variables available to user 
}
// 
// The hardest part of this routine is collapsing the FUNC plot types in the
// list (which are guaranteed to occur in (x,y) pairs while preserving the
// non-FUNC type plots intact.  This means we have to work our way through
// various lists.  Examples (hand checked): start_plot:F1->F2->NULL ==>
// F2->NULL start_plot:F1->F2->F3->F4->F5->F6->NULL ==> F2->F4->F6->NULL
// start_plot:F1->F2->D1->D2->F3->F4->D3->NULL ==> F2->D1->D2->F4->D3->NULL
// 
//static void parametric_fixup(curve_points * pStartPlot, int * pPlotNum)
void GnuPlot::ParametricFixup(curve_points * pStartPlot, int * pPlotNum)
{
	curve_points * xp = 0;
	curve_points * new_list = NULL;
	curve_points * free_list = NULL;
	curve_points ** last_pointer = &new_list;
	int i;
	int curve = 0;
	// 
	// Ok, go through all the plots and move FUNC types together.  Note: this
	// originally was written to look for a NULL next pointer, but gnuplot
	// wants to be sticky in grabbing memory and the right number of items in
	// the plot list is controlled by the plot_num variable.
	// 
	// Since gnuplot wants to do this sticky business, a free_list of
	// curve_points is kept and then tagged onto the end of the plot list as
	// this seems more in the spirit of the original memory behavior than
	// simply freeing the memory.  I'm personally not convinced this sort of
	// concern is worth it since the time spent computing points seems to
	// dominate any garbage collecting that might be saved here...
	// 
	new_list = xp = pStartPlot;
	while(++curve <= *pPlotNum) {
		if(xp->plot_type == FUNC) {
			// Here's a FUNC parametric function defined as two parts. 
			curve_points * yp = xp->next;
			--(*pPlotNum);
			assert(xp->p_count == yp->p_count);
			// IMPORTANT: because syntax is   plot x(t), y(t) XnYnaxes ..., only the y function axes are correct
			//
			// Go through all the points assigning the y's from xp to be
			// the x's for yp. In polar mode, we need to check max's and
			// min's as we go.
			//
			for(i = 0; i < yp->p_count; ++i) {
				double x, y;
				if(Gg.Polar) {
					const double r = yp->points[i].y;
					const double t = xp->points[i].y;
					// Convert from polar to cartesian coordinate and check ranges */
					// Note: The old in-line conversion checked AxS.__R().max against fabs(r).
					// That's not what PolarToXY() is currently doing.
					if(PolarToXY(t, r, &x, &y, TRUE) == OUTRANGE)
						yp->points[i].type = OUTRANGE;
				}
				else {
					x = xp->points[i].y;
					y = yp->points[i].y;
				}
				if(V.BoxWidth >= 0.0 && V.BoxWidthIsAbsolute) {
					coord_type dmy_type = INRANGE;
					STORE_AND_UPDATE_RANGE(yp->points[i].xlow, x - V.BoxWidth/2.0, dmy_type, yp->AxIdx_X, xp->noautoscale, NOOP);
					dmy_type = INRANGE;
					STORE_AND_UPDATE_RANGE(yp->points[i].xhigh, x + V.BoxWidth/2.0, dmy_type, yp->AxIdx_X, xp->noautoscale, NOOP);
				}
				STORE_AND_UPDATE_RANGE(yp->points[i].x, x, yp->points[i].type, yp->AxIdx_X, xp->noautoscale, NOOP);
				STORE_AND_UPDATE_RANGE(yp->points[i].y, y, yp->points[i].type, yp->AxIdx_Y, xp->noautoscale, NOOP);
			}
			// move xp to head of free list 
			xp->next = free_list;
			free_list = xp;
			// append yp to new_list 
			*last_pointer = yp;
			last_pointer = &(yp->next);
			xp = yp->next;
		}
		else {          /* data plot */
			assert(*last_pointer == xp);
			last_pointer = &(xp->next);
			xp = xp->next;
		}
	} // loop over plots 
	_Plt.P_FirstPlot = new_list;
	// Ok, stick the free list at the end of the curve_points plot list. 
	*last_pointer = free_list;
}
// 
// handle keyword options for "smooth kdensity {bandwidth <val>} {period <val>}
// 
//static void parse_kdensity_options(curve_points * pPlot)
void GnuPlot::ParseKDensityOptions(curve_points * pPlot)
{
	bool done = FALSE;
	pPlot->smooth_parameter = -1; /* Default */
	pPlot->smooth_period = 0;
	while(!done) {
		if(Pgm.AlmostEqualsCur("band$width")) {
			Pgm.Shift();
			pPlot->smooth_parameter = RealExpression();
		}
		else if(Pgm.AlmostEqualsCur("period")) {
			Pgm.Shift();
			pPlot->smooth_period = RealExpression();
		}
		else
			done = TRUE;
	}
}
// 
// Shared by plot and splot
// 
//void parse_plot_title(curve_points * this_plot, char * xtitle, char * ytitle, bool * set_title)
void GnuPlot::ParsePlotTitle(curve_points * pPlot, char * pXTitle, char * pYTitle, bool * pSetTitle)
{
	legend_key * key = &Gg.KeyT;
	if(Pgm.AlmostEqualsCur("t$itle") || Pgm.AlmostEqualsCur("not$itle")) {
		if(*pSetTitle)
			IntErrorCurToken("duplicate title");
		*pSetTitle = true;
		// title can be enhanced if not explicitly disabled 
		pPlot->title_no_enhanced = !key->enhanced;
		if(Pgm.AlmostEquals(Pgm.CToken++, "not$itle"))
			pPlot->title_is_suppressed = TRUE;
		if(Gg.Parametric || pPlot->title_is_suppressed) {
			if(Gg.InParametric)
				IntErrorCurToken("title allowed only after parametric function fully specified");
			ASSIGN_PTR(pXTitle, 0); // Remove default title . 
			ASSIGN_PTR(pYTitle, 0); // Remove default title . 
			if(Pgm.EqualsCur(","))
				return;
		}
		// This catches both "title columnheader" and "title columnhead(foo)" 
		// FIXME:  but it doesn't catch "title sprintf( f(columnhead(foo)) )" 
		if(Pgm.AlmostEqualsCur("col$umnheader")) {
			_Pb.parse_1st_row_as_headers = true;
		}
		// This ugliness is because columnheader can be either a keyword 
		// or a function name.  Yes, the design could have been better. 
		if(Pgm.AlmostEqualsCur("col$umnheader") && !(Pgm.AlmostEqualsCur("columnhead$er") && Pgm.EqualsNext("(")) ) {
			DfSetKeyTitleColumnHead(pPlot);
		}
		else if(Pgm.EqualsCur("at")) {
			*pSetTitle = false;
		}
		else {
			int save_token = Pgm.GetCurTokenIdx();
			// If the command is "plot ... notitle <optional title text>" 
			// we can throw the result away now that we have stepped over it  
			if(pPlot->title_is_suppressed) {
				char * skip = TryToGetString();
				SAlloc::F(skip);
				// In the very common case of a string constant, use it as-is. 
				// This guarantees that the title is only entered in the key once per
				// data file rather than once per data set within the file.
			}
			else if(Pgm.IsString(Pgm.GetCurTokenIdx()) && !Pgm.EqualsNext(".")) {
				free_at(_Df.df_plot_title_at);
				_Df.df_plot_title_at = NULL;
				SAlloc::F(pPlot->title);
				pPlot->title = TryToGetString();
				// Create an action table that can generate the title later 
			}
			else {
				free_at(_Df.df_plot_title_at);
				_Df.df_plot_title_at = PermAt();
				// We can evaluate the title for a function plot immediately 
				// FIXME: or this code could go into eval_plots() so that    
				//        function and data plots are treated the same way.  
				if(oneof4(pPlot->plot_type, FUNC, FUNC3D, VOXELDATA, KEYENTRY)) {
					GpValue a;
					EvaluateAt(_Df.df_plot_title_at, &a);
					if(a.Type == STRING) {
						SAlloc::F(pPlot->title);
						pPlot->title = a.v.string_val;
					}
					else
						IntWarn(save_token, "expecting string for title");
					free_at(_Df.df_plot_title_at);
					_Df.df_plot_title_at = NULL;
				}
			}
		}
		if(Pgm.EqualsCur("at")) {
			int save_token = ++Pgm.CToken;
			pPlot->title_position = (GpPosition *)SAlloc::M(sizeof(GpPosition));
			if(Pgm.EqualsCur("end")) {
				pPlot->title_position->scalex = character;
				pPlot->title_position->x = 1;
				pPlot->title_position->y = LEFT;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("beg$inning")) {
				pPlot->title_position->scalex = character;
				pPlot->title_position->x = -1;
				pPlot->title_position->y = RIGHT;
				Pgm.Shift();
			}
			else {
				GetPositionDefault(pPlot->title_position, screen, 2);
			}
			if(save_token == Pgm.GetCurTokenIdx())
				IntErrorCurToken("expecting \"at {beginning|end|<xpos>,<ypos>}\"");
			if(Pgm.EqualsCur("right")) {
				if(pPlot->title_position->scalex == character)
					pPlot->title_position->y = RIGHT;
				Pgm.Shift();
			}
			if(Pgm.EqualsCur("left")) {
				if(pPlot->title_position->scalex == character)
					pPlot->title_position->y = LEFT;
				Pgm.Shift();
			}
		}
	}
	if(Pgm.AlmostEqualsCur("enh$anced")) {
		Pgm.Shift();
		pPlot->title_no_enhanced = FALSE;
	}
	else if(Pgm.AlmostEqualsCur("noenh$anced")) {
		Pgm.Shift();
		pPlot->title_no_enhanced = TRUE;
	}
}
// 
// If a plot component title (key entry) was provided as a string expression
// rather than a simple string constant, we saved the expression to evaluate
// after the corresponding data has been input. This routine is called once
// for each data set in the input data stream, which would potentially generate
// a separate key entry for each data set.  We can short-circuit this by
// clearing the saved string expression after generating the first title.
// 
//void reevaluate_plot_title(curve_points * pPlot)
void GnuPlot::ReevaluatePlotTitle(curve_points * pPlot)
{
	GpValue a;
	if(_Df.df_plot_title_at) {
		_Df.evaluate_inside_using = true;
		EvaluateAt(_Df.df_plot_title_at, &a);
		_Df.evaluate_inside_using = false;
		if(!Ev.IsUndefined_ && a.Type == STRING) {
			SAlloc::F(pPlot->title);
			pPlot->title = a.v.string_val;
			// Special case where the "title" is used as a tic label 
			if(pPlot->plot_style == HISTOGRAMS && Gg.histogram_opts.type == HT_STACKED_IN_TOWERS) {
				double xpos = pPlot->histogram_sequence + pPlot->histogram->start;
				AddTicUser(&AxS[FIRST_X_AXIS], pPlot->title, xpos, -1);
			}
			// FIXME: good or bad to suppress all but the first generated title for a file containing multiple data sets?
			else {
				free_at(_Df.df_plot_title_at);
				_Df.df_plot_title_at = NULL;
			}
		}
	}
	if(pPlot->plot_style == PARALLELPLOT && !pPlot->title_is_automated) {
		const double xpos = AxS.Parallel(pPlot->AxIdx_P-1).paxis_x;
		AddTicUser(&AxS[FIRST_X_AXIS], pPlot->title, xpos, -1);
	}
}
