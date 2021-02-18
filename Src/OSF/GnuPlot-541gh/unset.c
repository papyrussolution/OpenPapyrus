// GNUPLOT - unset.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop

static void unset_angles();
static void free_arrowstyle(struct arrowstyle_def *);
//static void unset_bars();
static void unset_border();
static void unset_boxplot();
static void unset_boxdepth();
static void unset_boxwidth();
static void unset_fillstyle();
static void unset_cntrparam();
static void unset_cntrlabel();
static void unset_contour();
static void unset_dgrid3d();
static void unset_encoding();
static void unset_decimalsign();
static void unset_grid();
static void unset_hidden3d();
static void unset_histogram();
static void unset_textbox_style();
static void unset_historysize();
//static void unset_isosamples();
static void unset_key();
static void unset_linestyle(struct linestyle_def ** head);
static void unset_style_rectangle();
static void unset_style_circle();
static void unset_style_ellipse();
static void unset_style_parallel();
static void unset_style_spiderplot();
static void unset_wall(int which);
static void unset_loadpath();
static void unset_locale();
static void unset_mapping();
static void unset_missing();
static void unset_micro();
static void unset_minus_sign();
static void unset_mouse();
static void unset_minitics(GpAxis *);
static void unset_pm3d();
static void unset_print();
static void unset_psdir();
static void unset_surface();
static void unset_ticslevel();
static void unset_timefmt();
//static void unset_view();
static void unset_zero();
static void reset_mouse();
//
// The 'unset' command 
//
//void unset_command()
void GnuPlot::UnsetCommand()
{
	int found_token;
	int save_token;
	int i;
	Pgm.Shift();
	save_token = Pgm.GetCurTokenIdx();
	set_iterator = CheckForIteration();
	if(empty_iteration(set_iterator)) {
		// Skip iteration [i=start:end] where start > end 
		while(!Pgm.EndOfCommand()) 
			Pgm.Shift();
		set_iterator = cleanup_iteration(set_iterator);
		return;
	}
	if(forever_iteration(set_iterator)) {
		set_iterator = cleanup_iteration(set_iterator);
		IntError(save_token, "unbounded iteration not accepted here");
	}
	found_token = Pgm.LookupTableForCurrentToken(&set_tbl[0]);
	// HBB 20000506: rationalize occurrences of c_token++ ... 
	if(found_token != S_INVALID)
		Pgm.Shift();
	save_token = Pgm.GetCurTokenIdx();
ITERATE:
	switch(found_token) {
		case S_ANGLES: unset_angles(); break;
		case S_ARROW:  UnsetArrow(); break;
		case S_AUTOSCALE: UnsetAutoScale(); break;
		case S_BARS: UnsetBars(); break;
		case S_BORDER: unset_border(); break;
		case S_BOXDEPTH: unset_boxdepth(); break;
		case S_BOXWIDTH: unset_boxwidth(); break;
		case S_CLIP: UnsetClip(); break;
		case S_CNTRPARAM: unset_cntrparam(); break;
		case S_CNTRLABEL: unset_cntrlabel(); break;
		case S_CLABEL: clabel_onecolor = TRUE; break; /* deprecated command */
		case S_CONTOUR: unset_contour(); break;
		case S_CORNERPOLES: Gg.CornerPoles = FALSE; break;
		case S_DASHTYPE: UnsetDashType(); break;
		case S_DGRID3D: unset_dgrid3d(); break;
		case S_DEBUG: debug = 0; break;
		case S_DUMMY: UnsetDummy(); break;
		case S_ENCODING: unset_encoding(); break;
		case S_DECIMALSIGN: unset_decimalsign(); break;
		case S_FIT: UnsetFit(); break;
		case S_FORMAT:
		    Pgm.Rollback();
		    SetFormat();
		    break;
		case S_GRID: unset_grid(); break;
		case S_HIDDEN3D: unset_hidden3d(); break;
		case S_HISTORY: break; /* FIXME: reset to default values? */
		case S_HISTORYSIZE: unset_historysize(); break; /* Deprecated */
		case S_PIXMAP:
		    if(Pgm.EndOfCommand())
			    UnsetPixmaps();
		    else {
			    i = IntExpression();
			    UnsetPixmap(i);
		    }
		    break;
		case S_ISOSAMPLES: UnsetIsoSamples(); break;
		case S_JITTER: unset_jitter(); break;
		case S_KEY: unset_key(); break;
		case S_LABEL: UnsetLabel(); break;
		case S_LINETYPE: UnsetLineType(); break;
		case S_LINK:
		case S_NONLINEAR:
		    Pgm.Rollback();
		    LinkCommand();
		    break;
		case S_LOADPATH: unset_loadpath(); break;
		case S_LOCALE: unset_locale(); break;
		case S_LOGSCALE: UnsetLogScale(); break;
		case S_MACROS: break; // Aug 2013 - macros are always enabled 
		case S_MAPPING: unset_mapping(); break;
		case S_MARGIN:
		    V.MarginL.UnsetMargin();
		    V.MarginR.UnsetMargin();
		    V.MarginT.UnsetMargin();
		    V.MarginB.UnsetMargin();
		    break;
		case S_BMARGIN: V.MarginB.UnsetMargin(); break;
		case S_LMARGIN: V.MarginL.UnsetMargin(); break;
		case S_RMARGIN: V.MarginR.UnsetMargin(); break;
		case S_TMARGIN: V.MarginT.UnsetMargin(); break;
		case S_DATAFILE:
		    if(Pgm.AlmostEqualsCur("fort$ran")) {
			    df_fortran_constants = FALSE;
			    Pgm.Shift();
			    break;
		    }
		    else if(Pgm.AlmostEqualsCur("miss$ing")) {
			    unset_missing();
			    Pgm.Shift();
			    break;
		    }
		    else if(Pgm.AlmostEqualsCur("sep$arators")) {
			    ZFREE(df_separators);
			    Pgm.Shift();
			    break;
		    }
		    else if(Pgm.AlmostEqualsCur("com$mentschars")) {
			    SAlloc::F(df_commentschars);
			    df_commentschars = gp_strdup(DEFAULT_COMMENTS_CHARS);
			    Pgm.Shift();
			    break;
		    }
		    else if(Pgm.AlmostEqualsCur("bin$ary")) {
			    df_unset_datafile_binary();
			    Pgm.Shift();
			    break;
		    }
		    else if(Pgm.AlmostEqualsCur("nofpe_trap")) {
			    df_nofpe_trap = FALSE;
			    Pgm.Shift();
			    break;
		    }
		    else if(Pgm.AlmostEqualsCur("columnhead$ers")) {
			    df_columnheaders = FALSE;
			    Pgm.Shift();
			    break;
		    }
		    df_fortran_constants = FALSE;
		    unset_missing();
		    ZFREE(df_separators);
		    SAlloc::F(df_commentschars);
		    df_commentschars = gp_strdup(DEFAULT_COMMENTS_CHARS);
		    df_unset_datafile_binary();
		    df_columnheaders = FALSE;
		    break;
		case S_MICRO: unset_micro(); break;
		case S_MINUS_SIGN: unset_minus_sign(); break;
		case S_MONOCHROME: UnsetMonochrome(); break;
		case S_MOUSE: unset_mouse(); break;
		case S_MULTIPLOT: TermEndMultiplot(term); break;
		case S_OFFSETS: UnsetOffsets(); break;
		case S_ORIGIN: UnsetOrigin(); break;
		case SET_OUTPUT: UnsetOutput(); break;
		case S_OVERFLOW: Ev.OverflowHandling = INT64_OVERFLOW_IGNORE; break;
		case S_PARAMETRIC: UnsetParametric(); break;
		case S_PM3D: unset_pm3d(); break;
		case S_PALETTE: UnsetPalette(); break;
		case S_COLORBOX: UnsetColorBox(); break;
		case S_POINTINTERVALBOX: UnsetPointIntervalBox(); break;
		case S_POINTSIZE: UnsetPointSize(); break;
		case S_POLAR: UnsetPolar(); break;
		case S_PRINT: unset_print(); break;
		case S_PSDIR: unset_psdir(); break;
		case S_OBJECT: UnsetObject(); break;
		case S_WALL:
		    for(i = 0; i < 5; i++)
			    unset_wall(i);
		    break;
		case S_RTICS: AxS[POLAR_AXIS].UnsetTics(); break;
		case S_TTICS: AxS.Theta().UnsetTics(); break;
		case S_PAXIS:
		    i = IntExpression();
		    if(Pgm.AlmostEqualsCur("tic$s")) {
			    if(i > 0 && i <= AxS.GetParallelAxisCount())
				    AxS.Parallel(i-1).UnsetTics();
			    Pgm.Shift();
		    }
		    break;
		case S_RGBMAX: Gr.RgbMax = 255.0; break;
		case S_SAMPLES: UnsetSamples(); break;
		case S_SIZE: UnsetSize(); break;
		case S_SPIDERPLOT: UnsetSpiderPlot(); break;
		case S_STYLE: UnsetStyle(); break;
		case S_SURFACE: unset_surface(); break;
		case S_TABLE: UnsetTable(); break;
		case S_TERMINAL: UnsetTerminal(); break;
		case S_TICS: UnsetAllTics(); break;
		case S_TICSCALE: IntWarnCurToken("Deprecated syntax - use 'set tics scale default'"); break;
		case S_TICSLEVEL: 
		case S_XYPLANE: unset_ticslevel(); break;
		case S_TIMEFMT: unset_timefmt(); break;
		case S_TIMESTAMP: UnsetTimeStamp(); break;
		case S_TITLE: GpAxis::UnsetLabelOrTitle(&Gg.LblTitle); break;
		case S_VIEW: UnsetView(); break;
		case S_VGRID: UnsetVGrid(); break;
		case S_ZERO: unset_zero(); break;
		/* FIXME - are the tics correct? */
		case S_MXTICS: unset_minitics(&AxS[FIRST_X_AXIS]); break;
		case S_XTICS: AxS[FIRST_X_AXIS].UnsetTics(); break;
		case S_XDTICS:
		case S_XMTICS: UnsetMonthDayTics(FIRST_X_AXIS); break;
		case S_MYTICS: unset_minitics(&AxS[FIRST_Y_AXIS]); break;
		case S_YTICS: AxS[FIRST_Y_AXIS].UnsetTics(); break;
		case S_YDTICS:
		case S_YMTICS: UnsetMonthDayTics(FIRST_X_AXIS); break;
		case S_MX2TICS: unset_minitics(&AxS[SECOND_X_AXIS]); break;
		case S_X2TICS: AxS[SECOND_X_AXIS].UnsetTics(); break;
		case S_X2DTICS:
		case S_X2MTICS: UnsetMonthDayTics(FIRST_X_AXIS); break;
		case S_MY2TICS: unset_minitics(&AxS[SECOND_Y_AXIS]); break;
		case S_Y2TICS: AxS[SECOND_Y_AXIS].UnsetTics(); break;
		case S_Y2DTICS:
		case S_Y2MTICS: UnsetMonthDayTics(SECOND_Y_AXIS); break;
		case S_MZTICS: unset_minitics(&AxS[FIRST_Z_AXIS]); break;
		case S_ZTICS: AxS[FIRST_Z_AXIS].UnsetTics(); break;
		case S_ZDTICS:
		case S_ZMTICS: UnsetMonthDayTics(FIRST_X_AXIS); break;
		case S_MCBTICS: unset_minitics(&AxS[COLOR_AXIS]); break;
		case S_CBTICS: AxS[COLOR_AXIS].UnsetTics(); break;
		case S_CBDTICS:
		case S_CBMTICS: UnsetMonthDayTics(FIRST_X_AXIS); break;
		case S_MRTICS: unset_minitics(&AxS[POLAR_AXIS]); break;
		case S_MTTICS: unset_minitics(&AxS.Theta()); break;
		case S_XDATA: UnsetTimeData(FIRST_X_AXIS); break;
		case S_YDATA: UnsetTimeData(FIRST_Y_AXIS); break;
		case S_ZDATA: UnsetTimeData(FIRST_Z_AXIS); break;
		case S_CBDATA: UnsetTimeData(COLOR_AXIS); break;
		case S_X2DATA: UnsetTimeData(SECOND_X_AXIS); break;
		case S_Y2DATA: UnsetTimeData(SECOND_Y_AXIS); break;
		case S_XLABEL: UnsetAxisLabel(FIRST_X_AXIS); break;
		case S_YLABEL: UnsetAxisLabel(FIRST_Y_AXIS); break;
		case S_ZLABEL: UnsetAxisLabel(FIRST_Z_AXIS); break;
		case S_CBLABEL: UnsetAxisLabel(COLOR_AXIS); break;
		case S_RLABEL: UnsetAxisLabel(POLAR_AXIS); break;
		case S_X2LABEL: UnsetAxisLabel(SECOND_X_AXIS); break;
		case S_Y2LABEL: UnsetAxisLabel(SECOND_Y_AXIS); break;
		case S_XRANGE: UnsetRange(FIRST_X_AXIS); break;
		case S_X2RANGE: UnsetRange(SECOND_X_AXIS); break;
		case S_YRANGE: UnsetRange(FIRST_Y_AXIS); break;
		case S_Y2RANGE: UnsetRange(SECOND_Y_AXIS); break;
		case S_ZRANGE: UnsetRange(FIRST_Z_AXIS); break;
		case S_CBRANGE: UnsetRange(COLOR_AXIS); break;
		case S_RRANGE: UnsetRange(POLAR_AXIS); break;
		case S_TRANGE: UnsetRange(T_AXIS); break;
		case S_URANGE: UnsetRange(U_AXIS); break;
		case S_VRANGE: UnsetRange(V_AXIS); break;
		case S_RAXIS:
		    raxis = FALSE;
		    Pgm.Shift();
		    break;
		case S_XZEROAXIS: UnsetZeroAxis(FIRST_X_AXIS); break;
		case S_YZEROAXIS: UnsetZeroAxis(FIRST_Y_AXIS); break;
		case S_ZZEROAXIS: UnsetZeroAxis(FIRST_Z_AXIS); break;
		case S_X2ZEROAXIS: UnsetZeroAxis(SECOND_X_AXIS); break;
		case S_Y2ZEROAXIS: UnsetZeroAxis(SECOND_Y_AXIS); break;
		case S_ZEROAXIS: UnsetAllZeroAxes(); break;
		case S_INVALID:
		default: IntErrorCurToken("Unrecognized option.  See 'help unset'."); break;
	}
	if(NextIteration(set_iterator)) {
		Pgm.SetTokenIdx(save_token);
		goto ITERATE;
	}
	UpdateGpvalVariables(0);
	set_iterator = cleanup_iteration(set_iterator);
}
//
// process 'unset angles' command 
//
static void unset_angles()
{
	ang2rad = 1.0;
}
//
// process 'unset arrow' command 
//
//static void unset_arrow()
void GnuPlot::UnsetArrow()
{
	arrow_def * this_arrow;
	arrow_def * prev_arrow;
	if(Pgm.EndOfCommand()) {
		// delete all arrows 
		while(Gg.P_FirstArrow)
			Gg.DeleteArrow((arrow_def *)NULL, Gg.P_FirstArrow);
	}
	else {
		// get tag 
		int tag = IntExpression();
		if(!Pgm.EndOfCommand())
			IntErrorCurToken("extraneous arguments to unset arrow");
		for(this_arrow = Gg.P_FirstArrow, prev_arrow = NULL; this_arrow != NULL; prev_arrow = this_arrow, this_arrow = this_arrow->next) {
			if(this_arrow->tag == tag) {
				Gg.DeleteArrow(prev_arrow, this_arrow);
				return; /* exit, our job is done */
			}
		}
	}
}
// 
// delete arrow from linked list started by first_arrow.
// called with pointers to the previous arrow (prev) and the
// arrow to delete (pThis).
// If there is no previous arrow (the arrow to delete is
// first_arrow) then call with prev = NULL.
// 
//static void delete_arrow(arrow_def * pPrev, arrow_def * pThis)
void GpGadgets::DeleteArrow(arrow_def * pPrev, arrow_def * pThis)
{
	if(pThis) { // there really is something to delete 
		if(pPrev) // there is a previous arrow 
			pPrev->next = pThis->next;
		else // pThis = first_arrow so change first_arrow 
			P_FirstArrow = pThis->next;
		SAlloc::F(pThis);
	}
}
//
// delete the whole list of arrow styles 
//
//static void unset_arrowstyles()
void GnuPlot::UnsetArrowStyles()
{
	free_arrowstyle(Gg.P_FirstArrowStyle);
	Gg.P_FirstArrowStyle = NULL;
}

static void free_arrowstyle(struct arrowstyle_def * arrowstyle)
{
	if(arrowstyle) {
		free_arrowstyle(arrowstyle->next); // @recursion
		SAlloc::F(arrowstyle);
	}
}
//
// Deletes all pixmaps.
//
//static void unset_pixmaps(void)
void GnuPlot::UnsetPixmaps()
{
	t_pixmap * next;
	for(t_pixmap * pixmap = Gg.P_PixmapListHead; pixmap; pixmap = next) {
		SAlloc::F(pixmap->filename);
		SAlloc::F(pixmap->colormapname);
		SAlloc::F(pixmap->image_data);
		next = pixmap->next;
		SAlloc::F(pixmap);
	}
	Gg.P_PixmapListHead = NULL;
}
// 
// Deletes a single pixmap
// 
//static void unset_pixmap(int i)
void GnuPlot::UnsetPixmap(int i)
{
	t_pixmap * prev = Gg.P_PixmapListHead;
	for(t_pixmap * pixmap = Gg.P_PixmapListHead; pixmap;) {
		if(pixmap->tag == i) {
			if(pixmap == Gg.P_PixmapListHead)
				prev = Gg.P_PixmapListHead = pixmap->next;
			else
				prev->next = pixmap->next;
			SAlloc::F(pixmap->filename);
			SAlloc::F(pixmap->colormapname);
			SAlloc::F(pixmap->image_data);
			SAlloc::F(pixmap);
			pixmap = prev->next;
		}
		else {
			prev = pixmap;
			pixmap = pixmap->next;
		}
	}
}
//
// process 'unset autoscale' command 
//
//static void unset_autoscale()
void GnuPlot::UnsetAutoScale()
{
	if(Pgm.EndOfCommand()) {
		int axis;
		for(axis = 0; axis<AXIS_ARRAY_SIZE; axis++)
			AxS[axis].set_autoscale = (t_autoscale)FALSE;
		for(axis = 0; axis < AxS.GetParallelAxisCount(); axis++)
			AxS.Parallel(axis).set_autoscale = (t_autoscale)FALSE;
	}
	else if(Pgm.EqualsCur("xy") || Pgm.EqualsCur("tyx")) {
		AxS[FIRST_X_AXIS].set_autoscale = AxS[FIRST_Y_AXIS].set_autoscale = AUTOSCALE_NONE;
		Pgm.Shift();
	}
	else {
		// HBB 20000506: parse axis name, and unset the right element of the array: 
		AXIS_INDEX axis = (AXIS_INDEX)Pgm.LookupTableForCurrentToken(axisname_tbl);
		if(axis >= 0) {
			AxS[axis].set_autoscale = AUTOSCALE_NONE;
			Pgm.Shift();
		}
	}
}
//
// process 'unset bars' command 
//
//static void unset_bars()
void GnuPlot::UnsetBars()
{
	Gr.BarSize = 0.0;
	Gr.BarLp.flags &= ~LP_ERRORBAR_SET;
}
//
// reset is different from unset 
// This gets called from 'set bars default' also 
//
//void reset_bars()
void GnuPlot::ResetBars()
{
	lp_style_type def(lp_style_type::defCommon);// = DEFAULT_LP_STYLE_TYPE;
	Gr.BarLp = def;
	Gr.BarLp.l_type = LT_DEFAULT;
	Gr.BarLp.pm3d_color.type = TC_VARIABLE;
	Gr.BarSize = 1.0;
	Gr.BarLayer = LAYER_FRONT;
	Gr.BarLp.flags = 0;
}
//
// process 'unset border' command 
//
static void unset_border()
{
	// pThis is not the effect as with reset, as the border is enabled, by default 
	draw_border = 0;
}
//
// process 'unset style boxplot' command 
//
static void unset_boxplot()
{
	boxplot_style defstyle = DEFAULT_BOXPLOT_STYLE;
	boxplot_opts = defstyle;
}
//
// process 'unset boxdepth' command 
//
static void unset_boxdepth()
{
	boxdepth = 0.0;
}
//
// process 'unset boxwidth' command 
//
static void unset_boxwidth()
{
	GPO.V.BoxWidth = -1.0;
	GPO.V.BoxWidthIsAbsolute = true;
}
//
// process 'unset fill' command 
//
static void unset_fillstyle()
{
	default_fillstyle.fillstyle = FS_EMPTY;
	default_fillstyle.filldensity = 100;
	default_fillstyle.fillpattern = 0;
	default_fillstyle.border_color.type = TC_DEFAULT;
}
//
// process 'unset clip' command 
//
//static void unset_clip()
void GnuPlot::UnsetClip()
{
	if(Pgm.EndOfCommand()) {
		// same as all three 
		Gg.ClipPoints = false;
		Gg.ClipLines1 = false;
		Gg.ClipLines2 = false;
		Gg.ClipRadial = false;
	}
	else if(Pgm.AlmostEqualsCur("r$adial") || Pgm.EqualsCur("polar"))
		Gg.ClipRadial = false;
	else if(Pgm.AlmostEqualsCur("p$oints"))
		Gg.ClipPoints = false;
	else if(Pgm.AlmostEqualsCur("o$ne"))
		Gg.ClipLines1 = false;
	else if(Pgm.AlmostEqualsCur("t$wo"))
		Gg.ClipLines2 = false;
	else
		IntErrorCurToken("expecting 'points', 'one', 'two', or 'radial'");
	Pgm.Shift();
}

/* process 'unset cntrparam' command */
static void unset_cntrparam()
{
	contour_pts = DEFAULT_NUM_APPROX_PTS;
	contour_kind = CONTOUR_KIND_LINEAR;
	contour_order = DEFAULT_CONTOUR_ORDER;
	contour_levels = DEFAULT_CONTOUR_LEVELS;
	contour_levels_kind = LEVELS_AUTO;
	contour_firstlinetype = 0;
	contour_sortlevels = FALSE;
}

/* process 'unset cntrlabel' command */
static void unset_cntrlabel()
{
	clabel_onecolor = FALSE;
	clabel_start = 5;
	clabel_interval = 20;
	strcpy(contour_format, "%8.3g");
	ZFREE(clabel_font);
}
//
// process 'unset contour' command 
//
static void unset_contour()
{
	draw_contour = CONTOUR_NONE;
}
//
// process 'unset dashtype' command 
//
//static void unset_dashtype()
void GnuPlot::UnsetDashType()
{
	if(Pgm.EndOfCommand()) {
		// delete all 
		while(Gg.P_FirstCustomDashtype)
			delete_dashtype((custom_dashtype_def *)NULL, Gg.P_FirstCustomDashtype);
	}
	else {
		int tag = IntExpression();
		for(custom_dashtype_def * p_this = Gg.P_FirstCustomDashtype, * prev = NULL; p_this; prev = p_this, p_this = p_this->next) {
			if(p_this->tag == tag) {
				delete_dashtype(prev, p_this);
				break;
			}
		}
	}
}
//
// process 'unset dgrid3d' command 
//
static void unset_dgrid3d()
{
	dgrid3d_row_fineness = 10;
	dgrid3d_col_fineness = 10;
	dgrid3d_norm_value = 1;
	dgrid3d_mode = DGRID3D_QNORM;
	dgrid3d_x_scale = 1.0;
	dgrid3d_y_scale = 1.0;
	dgrid3d = FALSE;
}
//
// process 'unset dummy' command 
//
//static void unset_dummy()
void GnuPlot::UnsetDummy()
{
	strcpy(set_dummy_var[0], "x");
	strcpy(set_dummy_var[1], "y");
	for(int i = 2; i < MAX_NUM_VAR; i++)
		*set_dummy_var[i] = '\0';
}

/* process 'unset encoding' command */
static void unset_encoding()
{
	encoding = S_ENC_DEFAULT;
}

/* process 'unset decimalsign' command */
static void unset_decimalsign()
{
	ZFREE(decimalsign);
	ZFREE(numeric_locale);
}
//
// process 'unset fit' command 
//
//static void unset_fit()
void GnuPlot::UnsetFit()
{
	ZFREE(fitlogfile);
	fit_errorvariables = TRUE;
	fit_covarvariables = FALSE;
	fit_errorscaling = TRUE;
	fit_prescale = TRUE;
	fit_verbosity = BRIEF;
	Ev.DelUdvByName(FITLIMIT, FALSE);
	epsilon_abs = 0.;
	Ev.DelUdvByName(FITMAXITER, FALSE);
	Ev.DelUdvByName(FITSTARTLAMBDA, FALSE);
	Ev.DelUdvByName(FITLAMBDAFACTOR, FALSE);
	ZFREE(fit_script);
	fit_wrap = 0;
	// do not reset fit_v4compatible 
}
//
// process 'unset grid' command 
//
static void unset_grid()
{
	/* FIXME HBB 20000506: there is no command to explicitly reset the
	 * linetypes for major and minor gridlines. This function should
	 * do that, maybe... */
	/*AXIS_INDEX*/int i = (AXIS_INDEX)0;
	/* grid_selection = GRID_OFF; */
	for(; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++) {
		GPO.AxS[i].gridmajor = FALSE;
		GPO.AxS[i].gridminor = FALSE;
	}
	polar_grid_angle = 0;
	grid_vertical_lines = FALSE;
	grid_spiderweb = FALSE;
}

/* process 'unset hidden3d' command */
static void unset_hidden3d()
{
	hidden3d = FALSE;
}

static void unset_histogram()
{
	histogram_style foo; // = DEFAULT_HISTOGRAM_STYLE;
	SAlloc::F(histogram_opts.title.font);
	free_histlist(&histogram_opts);
	histogram_opts = foo;
}

static void unset_textbox_style()
{
	textbox_style foo = DEFAULT_TEXTBOX_STYLE;
	for(int i = 0; i < NUM_TEXTBOX_STYLES; i++) {
		textbox_opts[i] = foo;
		if(i > 0)
			textbox_opts[i].linewidth = 0.;
	}
}
//
// process 'unset historysize' command DEPRECATED 
//
static void unset_historysize()
{
	gnuplot_history_size = -1; /* don't ever truncate the history. */
}
//
// process 'unset isosamples' command 
//
//static void unset_isosamples()
void GnuPlot::UnsetIsoSamples()
{
	// HBB 20000506: was freeing 2D data structures although
	// isosamples are only used by 3D plots. 
	sp_free(first_3dplot);
	first_3dplot = NULL;
	Gg.IsoSamples1 = ISO_SAMPLES;
	Gg.IsoSamples2 = ISO_SAMPLES;
}

//void reset_key()
void GnuPlot::ResetKey()
{
	legend_key temp_key;// = DEFAULT_KEY_PROPS;
	ZFREE(Gg.KeyT.font);
	ZFREE(Gg.KeyT.title.text);
	ZFREE(Gg.KeyT.title.font);
	memcpy(&Gg.KeyT, &temp_key, sizeof(Gg.KeyT));
}
//
// process 'unset key' command 
//
static void unset_key()
{
	legend_key * key = &GPO.Gg.KeyT;
	key->visible = FALSE;
}
//
// process 'unset label' command 
//
//static void unset_label()
void GnuPlot::UnsetLabel()
{
	if(Pgm.EndOfCommand()) {
		// delete all labels 
		while(Gg.P_FirstLabel)
			DeleteLabel(0, Gg.P_FirstLabel);
	}
	else {
		// get tag 
		text_label * this_label;
		text_label * prev_label;
		int tag = IntExpression();
		if(!Pgm.EndOfCommand())
			IntErrorCurToken("extraneous arguments to unset label");
		for(this_label = Gg.P_FirstLabel, prev_label = NULL; this_label; prev_label = this_label, this_label = this_label->next) {
			if(this_label->tag == tag) {
				DeleteLabel(prev_label, this_label);
				return; // exit, our job is done 
			}
		}
	}
}
// 
// delete label from linked list started by first_label.
// called with pointers to the previous label (prev) and the
// label to delete (pThis).
// If there is no previous label (the label to delete is
// first_label) then call with prev = NULL.
// 
//static void delete_label(text_label * pPrev, struct text_label * pThis)
void GnuPlot::DeleteLabel(text_label * pPrev, struct text_label * pThis)
{
	if(pThis) { // there really is something to delete 
		if(pPrev) // there is a previous label
			pPrev->next = pThis->next;
		else // pThis = first_label so change first_label 
			Gg.P_FirstLabel = pThis->next;
		SAlloc::F(pThis->text);
		SAlloc::F(pThis->font);
		SAlloc::F(pThis);
	}
}

static void unset_linestyle(linestyle_def ** head)
{
	int tag = GPO.IntExpression();
	linestyle_def * p_this, * prev;
	for(p_this = *head, prev = NULL; p_this; prev = p_this, p_this = p_this->next) {
		if(p_this->tag == tag) {
			delete_linestyle(head, prev, p_this);
			break;
		}
	}
}

//static void unset_linetype()
void GnuPlot::UnsetLineType()
{
	if(Pgm.EqualsCur("cycle")) {
		linetype_recycle_count = 0;
		Pgm.Shift();
	}
	else if(!Pgm.EndOfCommand())
		unset_linestyle(&Gg.P_FirstPermLineStyle);
}

//static void unset_object()
void GnuPlot::UnsetObject()
{
	if(Pgm.EndOfCommand()) {
		// delete all objects 
		while(Gg.P_FirstObject)
			DeleteObject(0, Gg.P_FirstObject);
	}
	else {
		// get tag 
		const int tag = IntExpression();
		if(!Pgm.EndOfCommand())
			IntErrorCurToken("extraneous arguments to unset rectangle");
		for(GpObject * this_object = Gg.P_FirstObject, * prev_object = NULL; this_object; prev_object = this_object, this_object = this_object->next) {
			if(this_object->tag == tag) {
				DeleteObject(prev_object, this_object);
				return; // exit, our job is done 
			}
		}
	}
}
// 
// delete object from linked list started by first_object.
// called with pointers to the previous object (prev) and the
// object to delete (pThis).
// If there is no previous object (the object to delete is
// first_object) then call with prev = NULL.
// 
//static void delete_object(GpObject * prev, GpObject * pThis)
void GnuPlot::DeleteObject(GpObject * pPrev, GpObject * pThis)
{
	if(pThis) { // there really is something to delete 
		if(pPrev) // there is a previous rectangle 
			pPrev->next = pThis->next;
		else // pThis = first_object so change first_object 
			Gg.P_FirstObject = pThis->next;
		// NOTE:  Must free contents as well 
		if(pThis->object_type == OBJ_POLYGON)
			SAlloc::F(pThis->o.polygon.vertex);
		SAlloc::F(pThis);
	}
}
// 
// process 'unset loadpath' command 
// 
static void unset_loadpath()
{
	clear_loadpath();
}
// 
// process 'unset locale' command 
// 
static void unset_locale()
{
	init_locale();
}
// 
//static void reset_logscale(GpAxis * pAx)
void GnuPlot::ResetLogScale(GpAxis * pAx)
{
	bool undo_rlog = (pAx->index == POLAR_AXIS && pAx->log);
	pAx->log = FALSE;
	// Do not zero the base because we can still use it for gprintf formats
	// %L and %l with linear axis scaling.
	// this_axis->base = 0.0;
	if(undo_rlog)
		RRangeToXY();
}
//
// process 'unset logscale' command 
//
//static void unset_logscale()
void GnuPlot::UnsetLogScale()
{
	bool set_for_axis[AXIS_ARRAY_SIZE] = AXIS_ARRAY_INITIALIZER(FALSE);
	int axis;
	if(Pgm.EndOfCommand()) {
		for(axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++)
			set_for_axis[axis] = TRUE;
	}
	else {
		// do reverse search because of "x", "x1", "x2" sequence in axisname_tbl 
		for(int i = 0; i < Pgm.GetCurTokenLength();) {
			axis = lookup_table_nth_reverse(axisname_tbl, NUMBER_OF_MAIN_VISIBLE_AXES, gp_input_line + Pgm.GetCurTokenStartIndex() + i);
			if(axis < 0) {
				Pgm.P_Token[Pgm.CToken].start_index += i;
				IntErrorCurToken("invalid axis");
			}
			set_for_axis[axisname_tbl[axis].value] = TRUE;
			i += strlen(axisname_tbl[axis].key);
		}
		Pgm.Shift();
	}
	for(axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
		if(set_for_axis[axis]) {
			static char command[64];
			if(!isalpha(axis_name((AXIS_INDEX)axis)[0]))
				continue;
			if(AxS[axis].log) {
				sprintf(command, "unset nonlinear %s", axis_name((AXIS_INDEX)axis));
				DoString(command);
				AxS[axis].log = FALSE;
			}
			AxS[axis].ticdef.logscaling = FALSE;
		}
	}
}
//
// process 'unset mapping3d' command 
//
static void unset_mapping()
{
	// assuming same as points 
	mapping3d = MAP3D_CARTESIAN;
}
//
// process 'unset {blrt}margin' command 
//
//static void unset_margin(GpPosition * margin)
void GpPosition::UnsetMargin()
{
	scalex = character;
	x = -1.0;
}
//
// process 'unset micro' command 
//
static void unset_micro()
{
	use_micro = FALSE;
}
//
// process 'unset minus_sign' command 
//
static void unset_minus_sign()
{
	use_minus_sign = FALSE;
}
//
// process 'unset datafile' command 
//
static void unset_missing()
{
	ZFREE(missing_val);
}
//
// process 'unset mouse' command 
//
static void unset_mouse()
{
#ifdef USE_MOUSE
	mouse_setting.on = 0;
	GPO.UpdateStatusLine(); // wipe status line 
#endif
}
//
// process 'unset mxtics' command 
//
static void unset_minitics(GpAxis * pAx)
{
	pAx->minitics = MINI_OFF;
	pAx->mtic_freq = 10.0;
}
//
// process 'unset {x|y|x2|y2|z}tics' command 
//
//void unset_all_tics()
void GnuPlot::UnsetAllTics()
{
	for(int i = 0; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++)
		AxS[i].UnsetTics();
}

//static void unset_tics(GpAxis * this_axis)
void GpAxis::UnsetTics()
{
	GpPosition tics_nooffset = { character, character, character, 0., 0., 0.};
	ticmode = NO_TICS;
	ZFREE(ticdef.font);
	ticdef.textcolor.type = TC_DEFAULT;
	ticdef.textcolor.lt = 0;
	ticdef.textcolor.value = 0;
	ticdef.offset = tics_nooffset;
	ticdef.rangelimited = FALSE;
	ticdef.enhanced = TRUE;
	tic_rotate = 0;
	ticscale = 1.0;
	miniticscale = 0.5;
	TicIn = TRUE;
	manual_justify = FALSE;
	free_marklist(ticdef.def.user);
	ticdef.def.user = NULL;
	if(index >= PARALLEL_AXES)
		ticdef.rangelimited = TRUE;
}

//static void unset_month_day_tics(AXIS_INDEX axis)
void GnuPlot::UnsetMonthDayTics(AXIS_INDEX axIdx)
{
	AxS[axIdx].ticdef.type = TIC_COMPUTED;
}

//void unset_monochrome()
void GnuPlot::UnsetMonochrome()
{
	monochrome = FALSE;
	if(Pgm.EqualsCur("lt") || Pgm.AlmostEqualsCur("linet$ype")) {
		Pgm.Shift();
		if(!Pgm.EndOfCommand())
			unset_linestyle(&Gg.P_FirstMonoLineStyle);
	}
	term->flags &= ~TERM_MONOCHROME;
}
//
// process 'unset offsets' command 
//
//static void unset_offsets()
void GnuPlot::UnsetOffsets()
{
	Gr.LOff.x = Gr.ROff.x = 0.0;
	Gr.TOff.y = Gr.BOff.y = 0.0;
}
//
// process 'unset origin' command 
//
//static void unset_origin()
void GnuPlot::UnsetOrigin()
{
	V.Offset.SetZero();
}
//
// process 'unset output' command 
//
//static void unset_output()
void GnuPlot::UnsetOutput()
{
	if(multiplot) {
		IntErrorCurToken("you can't change the output in multiplot mode");
		return;
	}
	else {
		TermSetOutput(term, NULL);
		ZFREE(outstr); // means STDOUT 
	}
}

/* process 'unset print' command */
static void unset_print()
{
	print_set_output(NULL, FALSE, FALSE);
}

/* process 'unset psdir' command */
static void unset_psdir()
{
	ZFREE(PS_psdir);
}
//
// process 'unset parametric' command 
//
//static void unset_parametric()
void GnuPlot::UnsetParametric()
{
	if(Gg.Parametric) {
		Gg.Parametric = false;
		if(!Gg.Polar) { // keep t for polar 
			UnsetDummy();
			if(interactive)
				fprintf(stderr, "\n\tdummy variable is x for curves, x/y for surfaces\n");
		}
	}
}
//
// process 'unset palette' command 
//
//static void unset_palette()
void GnuPlot::UnsetPalette()
{
	Pgm.Shift();
	fprintf(stderr, "you can't unset the palette.\n");
}
//
// reset colorbox to default settings 
//
//static void reset_colorbox()
void GnuPlot::ResetColorBox()
{
	Gg.ColorBox = default_color_box;
}
//
// process 'unset colorbox' command: reset to default settings and then
// switch it off 
//
//static void unset_colorbox()
void GnuPlot::UnsetColorBox()
{
	ResetColorBox();
	GPO.Gg.ColorBox.where = SMCOLOR_BOX_NO;
}
//
// process 'unset pm3d' command 
//
static void unset_pm3d()
{
	pm3d.implicit = PM3D_EXPLICIT;
	// reset styles, required to 'plot something' after e.g. 'set pm3d map' 
	if(data_style == PM3DSURFACE) data_style = POINTSTYLE;
	if(func_style == PM3DSURFACE) func_style = LINES;
}
//
// process 'unset pointintervalbox' command 
//
//static void unset_pointintervalbox()
void GnuPlot::UnsetPointIntervalBox()
{
	Gg.PointIntervalBox = 1.0;
}
//
// process 'unset pointsize' command 
//
//static void unset_pointsize()
void GnuPlot::UnsetPointSize()
{
	Gg.PointSize = 1.0;
}
//
// process 'unset polar' command 
//
//static void unset_polar()
void GnuPlot::UnsetPolar()
{
	if(Gg.Polar) {
		Gg.Polar = false;
		if(Gg.Parametric && AxS[T_AXIS].set_autoscale) {
			// only if user has not set an explicit range 
			AxS[T_AXIS].set_min = axis_defaults[T_AXIS].min;
			AxS[T_AXIS].set_max = axis_defaults[T_AXIS].min;
		}
		if(!Gg.Parametric) {
			strcpy(set_dummy_var[0], "x");
			if(interactive)
				fprintf(stderr, "\n\tdummy variable is x for curves\n");
		}
	}
	raxis = FALSE;
	theta_origin = 0.0;
	theta_direction = 1.0;
	// Clear and reinitialize THETA axis structure 
	AxS.Theta().UnsetTics();
	unset_minitics(&AxS.Theta());
	AxS.Theta().min = 0.;
	AxS.Theta().max = 360.;
	AxS.Theta().ticdef = default_axis_ticdef;
	AxS.Theta().index = THETA_index;
	SAlloc::F(AxS.Theta().formatstring);
	AxS.Theta().formatstring = gp_strdup(DEF_FORMAT);
	AxS.Theta().ticscale = 1.0;
	AxS.Theta().miniticscale = 0.5;
	AxS.Theta().TicIn = TRUE;
	AxS.Theta().tic_rotate = 0;
}
//
// process 'unset samples' command 
//
//static void unset_samples()
void GnuPlot::UnsetSamples()
{
	// HBB 20000506: unlike unset_isosamples(), pThis one *has* to clear 2D data structures! 
	GnuPlot::CpFree(P_FirstPlot);
	P_FirstPlot = NULL;
	sp_free(first_3dplot);
	first_3dplot = NULL;
	Gg.Samples1 = SAMPLES;
	Gg.Samples2 = SAMPLES;
}
//
// process 'unset size' command 
//
//static void unset_size()
void GnuPlot::UnsetSize()
{
	V.Size.Set(1.0f);
}
//
// process 'unset style' command 
//
//static void unset_style()
void GnuPlot::UnsetStyle()
{
	if(Pgm.EndOfCommand()) {
		data_style = POINTSTYLE;
		func_style = LINES;
		while(Gg.P_FirstLineStyle)
			delete_linestyle(&Gg.P_FirstLineStyle, NULL, Gg.P_FirstLineStyle);
		unset_fillstyle();
		unset_style_rectangle();
		unset_style_circle();
		unset_style_ellipse();
		unset_histogram();
		unset_boxplot();
		unset_textbox_style();
		Pgm.Shift();
		return;
	}
	switch(Pgm.LookupTableForCurrentToken(show_style_tbl)) {
		case SHOW_STYLE_DATA:
		    data_style = POINTSTYLE;
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_FUNCTION:
		    func_style = LINES;
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_LINE:
		    Pgm.Shift();
		    if(Pgm.EndOfCommand()) {
			    while(Gg.P_FirstLineStyle)
				    delete_linestyle(&Gg.P_FirstLineStyle, NULL, Gg.P_FirstLineStyle);
		    }
		    else {
			    unset_linestyle(&Gg.P_FirstLineStyle);
		    }
		    break;
		case SHOW_STYLE_FILLING:
		    unset_fillstyle();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_HISTOGRAM:
		    unset_histogram();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_ARROW:
		    UnsetArrowStyles();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_RECTANGLE:
		    unset_style_rectangle();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_CIRCLE:
		    unset_style_circle();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_ELLIPSE:
		    unset_style_ellipse();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_TEXTBOX:
		    unset_textbox_style();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_BOXPLOT:
		    unset_boxplot();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_PARALLEL:
		    unset_style_parallel();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_SPIDERPLOT:
		    unset_style_spiderplot();
		    Pgm.Shift();
		    break;
		default:
		    IntErrorCurToken("unrecognized style");
	}
}

//static void unset_spiderplot()
void GnuPlot::UnsetSpiderPlot()
{
	if(Gg.SpiderPlot) {
		Gg.SpiderPlot = false;
		data_style = POINTSTYLE;
		V.AspectRatio = 0.0f;
	}
}

static void unset_style_spiderplot()
{
	spider_web spiderweb; // = DEFAULT_SPIDERPLOT_STYLE;
	GPO.Gg.SpiderPlotStyle = spiderweb;
}
//
// process 'unset surface' command 
//
static void unset_surface()
{
	draw_surface = false;
}
//
// process 'unset table' command 
//
//static void unset_table()
void GnuPlot::UnsetTable()
{
	SFile::ZClose(&Tab.P_TabOutFile);
	Tab.P_Var = 0;
	Tab.Mode = false;
}
//
// process 'unset terminal' command 
// Aug 2012:  restore original terminal type 
//
//static void unset_terminal()
void GnuPlot::UnsetTerminal()
{
	udvt_entry * original_terminal = Ev.GetUdvByName("GNUTERM");
	if(multiplot)
		TermEndMultiplot(term);
	TermReset(term);
	// FIXME: change is correct but reported result is truncated 
	if(original_terminal && original_terminal->udv_value.type != NOTDEFINED) {
		char * termname = gp_strdup(original_terminal->udv_value.v.string_val);
		if(strchr(termname, ' '))
			*strchr(termname, ' ') = '\0';
		*term_options = '\0';
		term = ChangeTerm(termname, strlen(termname));
		SAlloc::F(termname);
	}
	screen_ok = FALSE;
}
//
// process 'unset ticslevel' command 
//
static void unset_ticslevel()
{
	xyplane.z = 0.5;
	xyplane.absolute = FALSE;
}

/* Process 'unset timeformat' command */
static void unset_timefmt()
{
	SAlloc::F(P_TimeFormat);
	P_TimeFormat = gp_strdup(TIMEFMT);
}
//
// process 'unset timestamp' command 
//
//static void unset_timestamp()
void GnuPlot::UnsetTimeStamp()
{
	GpAxis::UnsetLabelOrTitle(&Gg.LblTime);
	Gg.LblTime.rotate = 0;
	Gg.TimeLabelBottom = TRUE;
}
//
// process 'unset view' command 
//
//static void unset_view()
void GnuPlot::UnsetView()
{
	splot_map = false;
	xz_projection = false;
	yz_projection = false;
	in_3d_polygon = false;
	V.AspectRatio3D = 0;
	_3DBlk.SurfaceRotZ = 30.0f;
	_3DBlk.SurfaceRotX = 60.0f;
	_3DBlk.SurfaceScale = 1.0f;
	_3DBlk.SurfaceLScale = 0.0f;
	_3DBlk.SurfaceZScale = 1.0f;
	_3DBlk.Azimuth = 0.0f;
}
//
// process 'unset zero' command 
//
static void unset_zero()
{
	GPO.Gg.Zero = ZERO;
}
//
// process 'unset {x|y|z|x2|y2}data' command 
//
//static void unset_timedata(AXIS_INDEX axis)
void GnuPlot::UnsetTimeData(AXIS_INDEX axis)
{
	AxS[axis].datatype = DT_NORMAL;
	AxS[axis].tictype = DT_NORMAL;
}
//
// process 'unset {x|y|z|x2|y2|t|u|v|r}range' command 
//
//static void unset_range(AXIS_INDEX axis)
void GnuPlot::UnsetRange(AXIS_INDEX axIdx)
{
	GpAxis * p_ax = &AxS[axIdx];
	p_ax->writeback_min = p_ax->set_min = axis_defaults[axIdx].min;
	p_ax->writeback_max = p_ax->set_max = axis_defaults[axIdx].max;
	p_ax->set_autoscale = AUTOSCALE_BOTH;
	p_ax->min_constraint = CONSTRAINT_NONE;
	p_ax->max_constraint = CONSTRAINT_NONE;
	p_ax->range_flags = 0;
}
//
// process 'unset {x|y|x2|y2|z}zeroaxis' command 
//
//static void unset_zeroaxis(AXIS_INDEX axis)
void GnuPlot::UnsetZeroAxis(AXIS_INDEX axIdx)
{
	if(AxS[axIdx].zeroaxis != &default_axis_zeroaxis)
		SAlloc::F(AxS[axIdx].zeroaxis);
	AxS[axIdx].zeroaxis = NULL;
}
//
// process 'unset zeroaxis' command 
//
//static void unset_all_zeroaxes()
void GnuPlot::UnsetAllZeroAxes()
{
	for(/*AXIS_INDEX*/int axis = (AXIS_INDEX)0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++)
		UnsetZeroAxis((AXIS_INDEX)axis);
}

// process 'unset [xyz]{2}label command 
//static void unset_axislabel(AXIS_INDEX axis)
void GnuPlot::UnsetAxisLabel(AXIS_INDEX axis)
{
	GpAxis::UnsetLabelOrTitle(&AxS[axis].label);
	AxS[axis].label = default_axis_label;
	if(oneof3(axis, FIRST_Y_AXIS, SECOND_Y_AXIS, COLOR_AXIS))
		AxS[axis].label.rotate = TEXT_VERTICAL;
}
// 
// The 'reset' command 
// HBB 20000506: I moved pThis here, from set.c, because 'reset' really
// is more like a big lot of 'unset' commands, rather than a bunch of
// 'set's. The benefit is that it can make use of many of the
// unset_something() contained in pThis module, i.e. you now have one
// place less to keep in sync if the semantics or defaults of any
// option is changed. This is only true for options for which 'unset'
// state is the default, however, e.g. not for 'surface', 'bars' and
// some others. 
// 
//void reset_command()
void GnuPlot::ResetCommand()
{
	int i;
	/*AXIS_INDEX*/int axis;
	bool save_interactive = interactive;
	Pgm.Shift();
	// Reset session state as well as internal graphics state 
	if(Pgm.EqualsCur("session")) {
		Ev.ClearUdfList();
		Ev.InitConstants();
		InitSession();
		reset_mouse();
		return;
	}
	else {
		// Reset error state (only?) 
		UpdateGpvalVariables(4);
		if(Pgm.AlmostEqualsCur("err$orstate")) {
			Pgm.Shift();
			return;
		}
		else {
		#ifdef USE_MOUSE
			// Reset key bindings only 
			if(Pgm.EqualsCur("bind")) {
				bind_remove_all();
				Pgm.Shift();
				return;
			}
		#endif
			if(!(Pgm.EndOfCommand())) {
				IntWarnCurToken("invalid option, expecting 'session', 'bind' or 'errorstate'");
				while(!(Pgm.EndOfCommand()))
					Pgm.Shift();
			}
		#if (0)
			// DEBUG - enable pThis code for testing
			// The *.dem unit tests all end with a "reset" command.
			// In order to test save/load from a wide variety of states we can intercept
			// pThis command and insert save/load before execution.
			extern bool successful_initialization;
			if(successful_initialization) {
				FILE * fp = fopen("/tmp/gnuplot_debug.sav", "w+");
				replot_line[0] = '\0';
				SaveAll(fp);
				rewind(fp);
				LoadFile(fp, gp_strdup("/tmp/gnuplot_debug.sav"), 1);
				// load_file closes fp 
			}
		#endif
			// Kludge alert, HBB 20000506: set to noninteractive mode, to
			// suppress some of the commentary output by the individual
			// unset_...() routines. 
			interactive = FALSE;
			UnsetSamples();
			UnsetIsoSamples();
			unset_jitter();
			// delete arrows 
			while(Gg.P_FirstArrow)
				Gg.DeleteArrow(0, Gg.P_FirstArrow);
			UnsetArrowStyles();
			// delete labels 
			while(Gg.P_FirstLabel)
				DeleteLabel(0, Gg.P_FirstLabel);
			// delete linestyles 
			while(Gg.P_FirstLineStyle)
				delete_linestyle(&Gg.P_FirstLineStyle, NULL, Gg.P_FirstLineStyle);
			// delete objects 
			while(Gg.P_FirstObject)
				DeleteObject(0, Gg.P_FirstObject);
			unset_style_rectangle();
			unset_style_circle();
			unset_style_ellipse();
			UnsetPixmaps(); // delete pixmaps 
			// 'polar', 'parametric' and 'dummy' are interdependent, so be sure to keep the order intact 
			UnsetPolar();
			UnsetParametric();
			UnsetDummy();
			UnsetSpiderPlot();
			unset_style_spiderplot();
			GpAxis::UnsetLabelOrTitle(&Gg.LblTitle);
			ResetKey();
			UnsetView();
			for(axis = (AXIS_INDEX)0; axis<AXIS_ARRAY_SIZE; axis++) {
				GpAxis * this_axis = &AxS[axis];
				// Free contents before overwriting with default values 
				this_axis->Destroy();
				// Fill with generic values, then customize 
				memcpy(this_axis, &default_axis_state, sizeof(GpAxis));
				this_axis->formatstring = gp_strdup(DEF_FORMAT);
				this_axis->index = axis;
				UnsetAxisLabel((AXIS_INDEX)axis); // sets vertical label for y/y2/cb 
				UnsetRange((AXIS_INDEX)axis); // copies min/max from axis_defaults 
				// 'tics' default is on for some, off for the other axes: 
				this_axis->UnsetTics();
				unset_minitics(this_axis);
				this_axis->ticdef = default_axis_ticdef;
				this_axis->minitics = MINI_DEFAULT;
				this_axis->ticmode = axis_defaults[axis].ticmode;
				ResetLogScale(this_axis);
			}
			// Free all previously allocated parallel axis structures 
			for(axis = (AXIS_INDEX)0; axis < AxS.GetParallelAxisCount(); axis++) {
				GpAxis * this_axis = &AxS.Parallel(axis);
				this_axis->Destroy();
			}
			AxS.DestroyParallelAxes();
			unset_style_parallel();
			AxS.DestroyShadowAxes();
			raxis = FALSE;
			for(i = 2; i<MAX_TICLEVEL; i++)
				ticscale[i] = 1;
			unset_timefmt();
			unset_boxplot();
			unset_boxdepth();
			unset_boxwidth();
			Gg.ClipPoints = false;
			Gg.ClipLines1 = true;
			Gg.ClipLines2 = false;
			Gg.ClipRadial = false;
			border_lp = default_border_lp;
			border_layer = LAYER_FRONT;
			draw_border = 31;
			Gg.CornerPoles = true;
			draw_surface = true;
			implicit_surface = true;
			data_style = POINTSTYLE;
			func_style = LINES;
			// Reset individual plot style options to the default 
			filledcurves_opts_data.closeto = FILLEDCURVES_CLOSED;
			filledcurves_opts_func.closeto = FILLEDCURVES_CLOSED;
			unset_grid();
			grid_lp = default_grid_lp;
			mgrid_lp = default_grid_lp;
			polar_grid_angle = 0;
			grid_layer = LAYER_BEHIND;
			grid_tics_in_front = FALSE;
			for(i = 0; i < 5; i++)
				unset_wall(i);
			SET_REFRESH_OK(E_REFRESH_NOT_OK, 0);
			reset_hidden3doptions();
			hidden3d = FALSE;
			unset_angles();
			ResetBars();
			unset_mapping();
			UnsetSize();
			V.AspectRatio = 0.0f; // don't force it 
			Gr.RgbMax = 255.0;
			UnsetOrigin();
			UnsetTimeStamp();
			UnsetOffsets();
			unset_contour();
			unset_cntrparam();
			unset_cntrlabel();
			unset_zero();
			unset_dgrid3d();
			unset_ticslevel();
			V.MarginB.UnsetMargin();
			V.MarginL.UnsetMargin();
			V.MarginR.UnsetMargin();
			V.MarginT.UnsetMargin();
			UnsetPointSize();
			UnsetPointIntervalBox();
			pm3d_reset();
			ResetColorBox();
			ResetPalette();
			df_unset_datafile_binary();
			unset_fillstyle();
			unset_histogram();
			unset_textbox_style();
			Gg.PreferLineStyles = false;
		#ifdef USE_MOUSE
			mouse_setting = default_mouse_setting;
		#endif
			// restore previous multiplot offset and margins 
			if(multiplot)
				MultiplotReset();
			unset_missing();
			ZFREE(df_separators);
			SAlloc::F(df_commentschars);
			df_commentschars = gp_strdup(DEFAULT_COMMENTS_CHARS);
			df_init();
			{ // Preserve some settings for `reset`, but not for `unset fit` 
				verbosity_level save_verbosity = fit_verbosity;
				bool save_errorscaling = fit_errorscaling;
				UnsetFit();
				fit_verbosity = save_verbosity;
				fit_errorscaling = save_errorscaling;
			}
			UpdateGpvalVariables(0); /* update GPVAL_ inner variables */
			// HBB 20000506: set 'interactive' back to its real value: 
			interactive = save_interactive;
		}
	}
}

static void unset_style_rectangle()
{
	//GpObject foo(GpObject::defRectangle);// = DEFAULT_RECTANGLE_STYLE;
	//default_rectangle = foo;
	default_rectangle.SetDefaultRectangleStyle();
}

static void unset_style_circle()
{
	//GpObject foo(GpObject::defCircle);// = DEFAULT_CIRCLE_STYLE;
	//default_circle = foo;
	default_circle.SetDefaultCircleStyle();
}

static void unset_style_ellipse()
{
	//GpObject foo(GpObject::defEllipse);// = DEFAULT_ELLIPSE_STYLE;
	//default_ellipse = foo;
	default_ellipse.SetDefaultEllipseStyle();
}

static void unset_style_parallel()
{
	pa_style parallel_axis_default; // = DEFAULT_PARALLEL_AXIS_STYLE;
	GPO.Gg.ParallelAxisStyle = parallel_axis_default;
}

static void unset_wall(int which)
{
	grid_wall[which].layer = LAYER_BEHIND;
}

/* Invoked by "reset session".  There is no command line "reset mouse" */
static void reset_mouse()
{
#ifdef USE_MOUSE
	free_at(mouse_readout_function.at); /* sets to NULL */
	ZFREE(mouse_readout_function.definition);
	ZFREE(mouse_alt_string);
	mouse_mode = MOUSE_COORDINATES_REAL;
	mouse_setting = default_mouse_setting;
#endif
}
