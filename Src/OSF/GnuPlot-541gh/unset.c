// GNUPLOT - unset.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop

static void free_arrowstyle(struct arrowstyle_def *);
static void unset_encoding();
static void unset_historysize();
static void unset_loadpath();
static void unset_minitics(GpAxis *);
static void unset_psdir();
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
	_Pb.set_iterator = CheckForIteration();
	if(empty_iteration(_Pb.set_iterator)) {
		// Skip iteration [i=start:end] where start > end 
		while(!Pgm.EndOfCommand()) 
			Pgm.Shift();
		_Pb.set_iterator = cleanup_iteration(_Pb.set_iterator);
		return;
	}
	if(forever_iteration(_Pb.set_iterator)) {
		_Pb.set_iterator = cleanup_iteration(_Pb.set_iterator);
		IntError(save_token, "unbounded iteration not accepted here");
	}
	found_token = Pgm.LookupTableForCurrentToken(&set_tbl[0]);
	// HBB 20000506: rationalize occurrences of c_token++ ... 
	if(found_token != S_INVALID)
		Pgm.Shift();
	save_token = Pgm.GetCurTokenIdx();
ITERATE:
	switch(found_token) {
		case S_ANGLES: UnsetAngles(); break;
		case S_ARROW:  UnsetArrow(); break;
		case S_AUTOSCALE: UnsetAutoScale(); break;
		case S_BARS: UnsetBars(); break;
		case S_BORDER: UnsetBorder(); break;
		case S_BOXDEPTH: UnsetBoxDepth(); break;
		case S_BOXWIDTH: UnsetBoxWidth(); break;
		case S_CLIP: UnsetClip(); break;
		case S_CNTRPARAM: UnsetCntrParam(); break;
		case S_CNTRLABEL: UnsetCntrLabel(); break;
		case S_CLABEL: _3DBlk.clabel_onecolor = true; break; /* deprecated command */
		case S_CONTOUR: UnsetContour(); break;
		case S_CORNERPOLES: Gg.CornerPoles = FALSE; break;
		case S_DASHTYPE: UnsetDashType(); break;
		case S_DGRID3D: UnsetDGrid3D(); break;
		case S_DEBUG: GpU.debug = 0; break;
		case S_DUMMY: UnsetDummy(); break;
		case S_ENCODING: unset_encoding(); break;
		case S_DECIMALSIGN: UnsetDecimalSign(); break;
		case S_FIT: UnsetFit(); break;
		case S_FORMAT:
		    Pgm.Rollback();
		    SetFormat();
		    break;
		case S_GRID: UnsetGrid(); break;
		case S_HIDDEN3D: UnsetHidden3D(); break;
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
		case S_JITTER: UnsetJitter(); break;
		case S_KEY: UnsetKey(); break;
		case S_LABEL: UnsetLabel(); break;
		case S_LINETYPE: UnsetLineType(); break;
		case S_LINK:
		case S_NONLINEAR:
		    Pgm.Rollback();
		    LinkCommand();
		    break;
		case S_LOADPATH: unset_loadpath(); break;
		case S_LOCALE: UnsetLocale(); break;
		case S_LOGSCALE: UnsetLogScale(); break;
		case S_MACROS: break; // Aug 2013 - macros are always enabled 
		case S_MAPPING: UnsetMapping(); break;
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
			    _Df.df_fortran_constants = false;
			    Pgm.Shift();
			    break;
		    }
		    else if(Pgm.AlmostEqualsCur("miss$ing")) {
			    UnsetMissing();
			    Pgm.Shift();
			    break;
		    }
		    else if(Pgm.AlmostEqualsCur("sep$arators")) {
			    ZFREE(_Df.df_separators);
			    Pgm.Shift();
			    break;
		    }
		    else if(Pgm.AlmostEqualsCur("com$mentschars")) {
			    FREEANDASSIGN(_Df.df_commentschars, sstrdup(DEFAULT_COMMENTS_CHARS));
			    Pgm.Shift();
			    break;
		    }
		    else if(Pgm.AlmostEqualsCur("bin$ary")) {
			    DfUnsetDatafileBinary();
			    Pgm.Shift();
			    break;
		    }
		    else if(Pgm.AlmostEqualsCur("nofpe_trap")) {
			    _Df.df_nofpe_trap = false;
			    Pgm.Shift();
			    break;
		    }
		    else if(Pgm.AlmostEqualsCur("columnhead$ers")) {
			    _Df.df_columnheaders = false;
			    Pgm.Shift();
			    break;
		    }
		    _Df.df_fortran_constants = false;
		    UnsetMissing();
		    ZFREE(_Df.df_separators);
		    FREEANDASSIGN(_Df.df_commentschars, sstrdup(DEFAULT_COMMENTS_CHARS));
		    DfUnsetDatafileBinary();
		    _Df.df_columnheaders = false;
		    break;
		case S_MICRO: UnsetMicro(); break;
		case S_MINUS_SIGN: UnsetMinusSign(); break;
		case S_MONOCHROME: UnsetMonochrome(); break;
		case S_MOUSE: UnsetMouse(); break;
		case S_MULTIPLOT: TermEndMultiplot(GPT.P_Term); break;
		case S_OFFSETS: UnsetOffsets(); break;
		case S_ORIGIN: UnsetOrigin(); break;
		case SET_OUTPUT: UnsetOutput(); break;
		case S_OVERFLOW: Ev.OverflowHandling = INT64_OVERFLOW_IGNORE; break;
		case S_PARAMETRIC: UnsetParametric(); break;
		case S_PM3D: UnsetPm3D(); break;
		case S_PALETTE: UnsetPalette(); break;
		case S_COLORBOX: UnsetColorBox(); break;
		case S_POINTINTERVALBOX: UnsetPointIntervalBox(); break;
		case S_POINTSIZE: UnsetPointSize(); break;
		case S_POLAR: UnsetPolar(); break;
		case S_PRINT: UnsetPrint(); break;
		case S_PSDIR: unset_psdir(); break;
		case S_OBJECT: UnsetObject(); break;
		case S_WALL:
		    for(i = 0; i < 5; i++)
			    UnsetWall(i);
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
		case S_SURFACE: UnsetSurface(); break;
		case S_TABLE: UnsetTable(); break;
		case S_TERMINAL: UnsetTerminal(); break;
		case S_TICS: UnsetAllTics(); break;
		case S_TICSCALE: IntWarnCurToken("Deprecated syntax - use 'set tics scale default'"); break;
		case S_TICSLEVEL: 
		case S_XYPLANE: UnsetTicsLevel(); break;
		case S_TIMEFMT: UnsetTimeFmt(); break;
		case S_TIMESTAMP: UnsetTimeStamp(); break;
		case S_TITLE: GpAxis::UnsetLabelOrTitle(&Gg.LblTitle); break;
		case S_VIEW: UnsetView(); break;
		case S_VGRID: UnsetVGrid(); break;
		case S_ZERO: UnsetZero(); break;
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
		    AxS.raxis = FALSE;
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
	if(NextIteration(_Pb.set_iterator)) {
		Pgm.SetTokenIdx(save_token);
		goto ITERATE;
	}
	UpdateGpvalVariables(GPT.P_Term, 0);
	_Pb.set_iterator = cleanup_iteration(_Pb.set_iterator);
}
//
// process 'unset angles' command 
//
void GnuPlot::UnsetAngles()
{
	Gg.ang2rad = 1.0;
}
//
// process 'unset arrow' command 
//
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
		for(this_arrow = Gg.P_FirstArrow, prev_arrow = NULL; this_arrow; prev_arrow = this_arrow, this_arrow = this_arrow->next) {
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
void GnuPlot::UnsetArrowStyles()
{
	free_arrowstyle(Gg.P_FirstArrowStyle);
	Gg.P_FirstArrowStyle = NULL;
}

static void free_arrowstyle(arrowstyle_def * pArrowStyle)
{
	if(pArrowStyle) {
		free_arrowstyle(pArrowStyle->next); // @recursion
		SAlloc::F(pArrowStyle);
	}
}
//
// Deletes all pixmaps.
//
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
			FREEANDASSIGN(pixmap, prev->next);
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
void GnuPlot::UnsetBorder()
{
	// pThis is not the effect as with reset, as the border is enabled, by default 
	Gg.draw_border = 0;
}
//
// process 'unset style boxplot' command 
//
void GnuPlot::UnsetBoxPlot()
{
	boxplot_style defstyle = DEFAULT_BOXPLOT_STYLE;
	Gg.boxplot_opts = defstyle;
}
//
// process 'unset boxdepth' command 
//
void GnuPlot::UnsetBoxDepth()
{
	_Plt.boxdepth = 0.0;
}
//
// process 'unset boxwidth' command 
//
void GnuPlot::UnsetBoxWidth()
{
	V.BoxWidth = -1.0;
	V.BoxWidthIsAbsolute = true;
}
//
// process 'unset fill' command 
//
void GnuPlot::UnsetFillStyle()
{
	Gg.default_fillstyle.fillstyle = FS_EMPTY;
	Gg.default_fillstyle.filldensity = 100;
	Gg.default_fillstyle.fillpattern = 0;
	Gg.default_fillstyle.border_color.type = TC_DEFAULT;
}
//
// process 'unset clip' command 
//
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
//
// process 'unset cntrparam' command 
//
void GnuPlot::UnsetCntrParam()
{
	_Cntr.ContourPts = DEFAULT_NUM_APPROX_PTS;
	_Cntr.ContourKind = CONTOUR_KIND_LINEAR;
	_Cntr.ContourOrder = DEFAULT_CONTOUR_ORDER;
	_Cntr.ContourLevels = DEFAULT_CONTOUR_LEVELS;
	_Cntr.ContourLevelsKind = LEVELS_AUTO;
	_Cntr.ContourFirstLineType = 0;
	_Cntr.ContourSortLevels = FALSE;
}
//
// process 'unset cntrlabel' command 
//
void GnuPlot::UnsetCntrLabel()
{
	_3DBlk.clabel_onecolor = FALSE;
	_3DBlk.clabel_start = 5;
	_3DBlk.clabel_interval = 20;
	strcpy(_Cntr.ContourFormat, "%8.3g");
	ZFREE(_3DBlk.clabel_font);
}
//
// process 'unset contour' command 
//
void GnuPlot::UnsetContour()
{
	_3DBlk.draw_contour = CONTOUR_NONE;
}
//
// process 'unset dashtype' command 
//
void GnuPlot::UnsetDashType()
{
	if(Pgm.EndOfCommand()) {
		// delete all 
		while(Gg.P_FirstCustomDashtype)
			DeleteDashType((custom_dashtype_def *)NULL, Gg.P_FirstCustomDashtype);
	}
	else {
		int tag = IntExpression();
		for(custom_dashtype_def * p_this = Gg.P_FirstCustomDashtype, * prev = NULL; p_this; prev = p_this, p_this = p_this->next) {
			if(p_this->tag == tag) {
				DeleteDashType(prev, p_this);
				break;
			}
		}
	}
}
//
// process 'unset dgrid3d' command 
//
void GnuPlot::UnsetDGrid3D()
{
	_Plt.dgrid3d_row_fineness = 10;
	_Plt.dgrid3d_col_fineness = 10;
	_Plt.dgrid3d_norm_value = 1;
	_Plt.dgrid3d_mode = DGRID3D_QNORM;
	_Plt.dgrid3d_x_scale = 1.0;
	_Plt.dgrid3d_y_scale = 1.0;
	_Plt.dgrid3d = FALSE;
}
//
// process 'unset dummy' command 
//
void GnuPlot::UnsetDummy()
{
	strcpy(_Pb.set_dummy_var[0], "x");
	strcpy(_Pb.set_dummy_var[1], "y");
	for(int i = 2; i < MAX_NUM_VAR; i++)
		*_Pb.set_dummy_var[i] = '\0';
}
//
// process 'unset encoding' command 
//
static void unset_encoding()
{
	GPT._Encoding = S_ENC_DEFAULT;
}
//
// process 'unset decimalsign' command 
//
void GnuPlot::UnsetDecimalSign()
{
	ZFREE(GpU.decimalsign);
	ZFREE(GpU.numeric_locale);
}
//
// process 'unset fit' command 
//
void GnuPlot::UnsetFit()
{
	ZFREE(_Fit.fitlogfile);
	_Fit.fit_errorvariables = TRUE;
	_Fit.fit_covarvariables = FALSE;
	_Fit.fit_errorscaling = TRUE;
	_Fit.fit_prescale = TRUE;
	_Fit.fit_verbosity = BRIEF;
	DelUdvByName(FITLIMIT, FALSE);
	_Fit.epsilon_abs = 0.;
	DelUdvByName(FITMAXITER, FALSE);
	DelUdvByName(FITSTARTLAMBDA, FALSE);
	DelUdvByName(FITLAMBDAFACTOR, FALSE);
	ZFREE(_Fit.fit_script);
	_Fit.fit_wrap = 0;
	// do not reset fit_v4compatible 
}
//
// process 'unset grid' command 
//
void GnuPlot::UnsetGrid()
{
	// FIXME HBB 20000506: there is no command to explicitly reset the
	// linetypes for major and minor gridlines. This function should
	// do that, maybe... 
	/*AXIS_INDEX*/int i = (AXIS_INDEX)0;
	// grid_selection = GRID_OFF; 
	for(; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++) {
		AxS[i].gridmajor = FALSE;
		AxS[i].gridminor = FALSE;
	}
	AxS.polar_grid_angle = 0;
	AxS.grid_vertical_lines = FALSE;
	AxS.grid_spiderweb = FALSE;
}
//
// process 'unset hidden3d' command 
//
void GnuPlot::UnsetHidden3D()
{
	_3DBlk.hidden3d = FALSE;
}

void GnuPlot::UnsetHistogram()
{
	histogram_style foo; // = DEFAULT_HISTOGRAM_STYLE;
	SAlloc::F(Gg.histogram_opts.title.font);
	FreeHistogramList(&Gg.histogram_opts);
	Gg.histogram_opts = foo;
}

void GnuPlot::UnsetTextboxStyle()
{
	textbox_style foo = DEFAULT_TEXTBOX_STYLE;
	for(int i = 0; i < NUM_TEXTBOX_STYLES; i++) {
		Gg.textbox_opts[i] = foo;
		if(i > 0)
			Gg.textbox_opts[i].linewidth = 0.;
	}
}
//
// process 'unset historysize' command DEPRECATED 
//
static void unset_historysize()
{
	GPO.Hist.HistorySize = -1; // don't ever truncate the history
}
//
// process 'unset isosamples' command 
//
void GnuPlot::UnsetIsoSamples()
{
	// HBB 20000506: was freeing 2D data structures although
	// isosamples are only used by 3D plots. 
	sp_free(_Plt.first_3dplot);
	_Plt.first_3dplot = NULL;
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
void GnuPlot::UnsetKey()
{
	legend_key * p_key = &Gg.KeyT;
	p_key->visible = FALSE;
}
//
// process 'unset label' command 
//
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

void GnuPlot::UnsetLineStyle(linestyle_def ** ppHead)
{
	int tag = IntExpression();
	linestyle_def * p_this, * prev;
	for(p_this = *ppHead, prev = NULL; p_this; prev = p_this, p_this = p_this->next) {
		if(p_this->tag == tag) {
			delete_linestyle(ppHead, prev, p_this);
			break;
		}
	}
}

void GnuPlot::UnsetLineType()
{
	if(Pgm.EqualsCur("cycle")) {
		GPT.LinetypeRecycleCount = 0;
		Pgm.Shift();
	}
	else if(!Pgm.EndOfCommand())
		UnsetLineStyle(&Gg.P_FirstPermLineStyle);
}

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
void GnuPlot::UnsetLocale()
{
	LocaleHandler(ACTION_INIT, NULL);
}
// 
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
			axis = lookup_table_nth_reverse(axisname_tbl, NUMBER_OF_MAIN_VISIBLE_AXES, Pgm.P_InputLine + Pgm.GetCurTokenStartIndex() + i);
			if(axis < 0) {
				Pgm.ÑTok().StartIdx += i;
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
void GnuPlot::UnsetMapping()
{
	// assuming same as points 
	_Plt.mapping3d = MAP3D_CARTESIAN;
}
//
// process 'unset {blrt}margin' command 
//
void GpPosition::UnsetMargin()
{
	scalex = character;
	x = -1.0;
}
//
// process 'unset micro' command 
//
void GnuPlot::UnsetMicro()
{
	GpU.use_micro = FALSE;
}
//
// process 'unset minus_sign' command 
//
void GnuPlot::UnsetMinusSign()
{
	GpU.use_minus_sign = FALSE;
}
//
// process 'unset datafile' command 
//
void GnuPlot::UnsetMissing()
{
	ZFREE(_Df.missing_val);
}
//
// process 'unset mouse' command 
//
void GnuPlot::UnsetMouse()
{
#ifdef USE_MOUSE
	mouse_setting.on = 0;
	UpdateStatusLine(); // wipe status line 
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

void GnuPlot::UnsetMonthDayTics(AXIS_INDEX axIdx)
{
	AxS[axIdx].ticdef.type = TIC_COMPUTED;
}

//void unset_monochrome()
void GnuPlot::UnsetMonochrome()
{
	GPT.Flags &= ~GpTerminalBlock::fMonochrome;
	if(Pgm.EqualsCur("lt") || Pgm.AlmostEqualsCur("linet$ype")) {
		Pgm.Shift();
		if(!Pgm.EndOfCommand())
			UnsetLineStyle(&Gg.P_FirstMonoLineStyle);
	}
	GPT.P_Term->flags &= ~TERM_MONOCHROME;
}
//
// process 'unset offsets' command 
//
void GnuPlot::UnsetOffsets()
{
	Gr.LOff.x = Gr.ROff.x = 0.0;
	Gr.TOff.y = Gr.BOff.y = 0.0;
}
//
// process 'unset origin' command 
//
void GnuPlot::UnsetOrigin()
{
	V.Offset.SetZero();
}
//
// process 'unset output' command 
//
void GnuPlot::UnsetOutput()
{
	if(GPT.Flags & GpTerminalBlock::fMultiplot)
		IntErrorCurToken("you can't change the output in multiplot mode");
	else {
		TermSetOutput(GPT.P_Term, NULL);
		ZFREE(GPT.P_OutStr); // means STDOUT 
	}
}
//
// process 'unset print' command 
//
void GnuPlot::UnsetPrint()
{
	PrintSetOutput(NULL, FALSE, FALSE);
}
//
// process 'unset psdir' command 
//
static void unset_psdir()
{
	ZFREE(GPT.P_PS_PsDir);
}
//
// process 'unset parametric' command 
//
void GnuPlot::UnsetParametric()
{
	if(Gg.Parametric) {
		Gg.Parametric = false;
		if(!Gg.Polar) { // keep t for polar 
			UnsetDummy();
			if(_Plt.interactive)
				fprintf(stderr, "\n\tdummy variable is x for curves, x/y for surfaces\n");
		}
	}
}
//
// process 'unset palette' command 
//
void GnuPlot::UnsetPalette()
{
	Pgm.Shift();
	fprintf(stderr, "you can't unset the palette.\n");
}
//
// reset colorbox to default settings 
//
void GnuPlot::ResetColorBox()
{
	Gg.ColorBox = default_color_box;
}
//
// process 'unset colorbox' command: reset to default settings and then
// switch it off 
//
void GnuPlot::UnsetColorBox()
{
	ResetColorBox();
	Gg.ColorBox.where = SMCOLOR_BOX_NO;
}
//
// process 'unset pm3d' command 
//
void GnuPlot::UnsetPm3D()
{
	_Pm3D.pm3d.implicit = PM3D_EXPLICIT;
	// reset styles, required to 'plot something' after e.g. 'set pm3d map' 
	if(Gg.data_style == PM3DSURFACE) Gg.data_style = POINTSTYLE;
	if(Gg.func_style == PM3DSURFACE) Gg.func_style = LINES;
}
//
// process 'unset pointintervalbox' command 
//
void GnuPlot::UnsetPointIntervalBox()
{
	Gg.PointIntervalBox = 1.0;
}
//
// process 'unset pointsize' command 
//
void GnuPlot::UnsetPointSize()
{
	Gg.PointSize = 1.0;
}
//
// process 'unset polar' command 
//
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
			strcpy(_Pb.set_dummy_var[0], "x");
			if(_Plt.interactive)
				fprintf(stderr, "\n\tdummy variable is x for curves\n");
		}
	}
	AxS.raxis = FALSE;
	AxS.ThetaOrigin = 0.0;
	AxS.ThetaDirection = 1.0;
	// Clear and reinitialize THETA axis structure 
	AxS.Theta().UnsetTics();
	unset_minitics(&AxS.Theta());
	AxS.Theta().min = 0.;
	AxS.Theta().max = 360.;
	AxS.Theta().ticdef = default_axis_ticdef;
	AxS.Theta().index = THETA_index;
	FREEANDASSIGN(AxS.Theta().formatstring, sstrdup(DEF_FORMAT));
	AxS.Theta().ticscale = 1.0;
	AxS.Theta().miniticscale = 0.5;
	AxS.Theta().TicIn = TRUE;
	AxS.Theta().tic_rotate = 0;
}
//
// process 'unset samples' command 
//
void GnuPlot::UnsetSamples()
{
	// HBB 20000506: unlike unset_isosamples(), pThis one *has* to clear 2D data structures! 
	CpFree(_Plt.P_FirstPlot);
	_Plt.P_FirstPlot = NULL;
	sp_free(_Plt.first_3dplot);
	_Plt.first_3dplot = NULL;
	Gg.Samples1 = SAMPLES;
	Gg.Samples2 = SAMPLES;
}
//
// process 'unset size' command 
//
void GnuPlot::UnsetSize()
{
	V.Size.Set(1.0f);
}
//
// process 'unset style' command 
//
void GnuPlot::UnsetStyle()
{
	if(Pgm.EndOfCommand()) {
		Gg.data_style = POINTSTYLE;
		Gg.func_style = LINES;
		while(Gg.P_FirstLineStyle)
			delete_linestyle(&Gg.P_FirstLineStyle, NULL, Gg.P_FirstLineStyle);
		UnsetFillStyle();
		UnsetStyleRectangle();
		UnsetStyleCircle();
		UnsetStyleEllipse();
		UnsetHistogram();
		UnsetBoxPlot();
		UnsetTextboxStyle();
		Pgm.Shift();
		return;
	}
	switch(Pgm.LookupTableForCurrentToken(show_style_tbl)) {
		case SHOW_STYLE_DATA:
		    Gg.data_style = POINTSTYLE;
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_FUNCTION:
		    Gg.func_style = LINES;
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_LINE:
		    Pgm.Shift();
		    if(Pgm.EndOfCommand()) {
			    while(Gg.P_FirstLineStyle)
				    delete_linestyle(&Gg.P_FirstLineStyle, NULL, Gg.P_FirstLineStyle);
		    }
		    else
			    UnsetLineStyle(&Gg.P_FirstLineStyle);
		    break;
		case SHOW_STYLE_FILLING:
		    UnsetFillStyle();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_HISTOGRAM:
		    UnsetHistogram();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_ARROW:
		    UnsetArrowStyles();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_RECTANGLE:
		    UnsetStyleRectangle();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_CIRCLE:
		    UnsetStyleCircle();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_ELLIPSE:
		    UnsetStyleEllipse();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_TEXTBOX:
		    UnsetTextboxStyle();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_BOXPLOT:
		    UnsetBoxPlot();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_PARALLEL:
		    UnsetStyleParallel();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_SPIDERPLOT:
		    UnsetStyleSpiderPlot();
		    Pgm.Shift();
		    break;
		default:
		    IntErrorCurToken("unrecognized style");
	}
}

void GnuPlot::UnsetSpiderPlot()
{
	if(Gg.SpiderPlot) {
		Gg.SpiderPlot = false;
		Gg.data_style = POINTSTYLE;
		V.AspectRatio = 0.0f;
	}
}

void GnuPlot::UnsetStyleSpiderPlot()
{
	spider_web spiderweb; // = DEFAULT_SPIDERPLOT_STYLE;
	Gg.SpiderPlotStyle = spiderweb;
}
//
// process 'unset surface' command 
//
void GnuPlot::UnsetSurface()
{
	_3DBlk.draw_surface = false;
}
//
// process 'unset table' command 
//
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
void GnuPlot::UnsetTerminal()
{
	udvt_entry * original_terminal = Ev.GetUdvByName("GNUTERM");
	if(GPT.Flags & GpTerminalBlock::fMultiplot)
		TermEndMultiplot(GPT.P_Term);
	TermReset(GPT.P_Term);
	// FIXME: change is correct but reported result is truncated 
	if(original_terminal && original_terminal->udv_value.Type != NOTDEFINED) {
		char * termname = sstrdup(original_terminal->udv_value.v.string_val);
		if(strchr(termname, ' '))
			*strchr(termname, ' ') = '\0';
		GPT._TermOptions.Z();
		GPT.P_Term = ChangeTerm(termname, strlen(termname));
		SAlloc::F(termname);
	}
	GpU.screen_ok = FALSE;
}
//
// process 'unset ticslevel' command 
//
void GnuPlot::UnsetTicsLevel()
{
	_3DBlk.xyplane.z = 0.5;
	_3DBlk.xyplane.absolute = FALSE;
}
//
// Process 'unset timeformat' command 
//
void GnuPlot::UnsetTimeFmt()
{
	FREEANDASSIGN(AxS.P_TimeFormat, sstrdup(TIMEFMT));
}
//
// process 'unset timestamp' command 
//
void GnuPlot::UnsetTimeStamp()
{
	GpAxis::UnsetLabelOrTitle(&Gg.LblTime);
	Gg.LblTime.rotate = 0;
	Gg.TimeLabelBottom = TRUE;
}
//
// process 'unset view' command 
//
void GnuPlot::UnsetView()
{
	_3DBlk.splot_map = false;
	_3DBlk.xz_projection = false;
	_3DBlk.yz_projection = false;
	_3DBlk.in_3d_polygon = false;
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
void GnuPlot::UnsetZero()
{
	Gg.Zero = ZERO;
}
//
// process 'unset {x|y|z|x2|y2}data' command 
//
void GnuPlot::UnsetTimeData(AXIS_INDEX axis)
{
	AxS[axis].datatype = DT_NORMAL;
	AxS[axis].tictype = DT_NORMAL;
}
//
// process 'unset {x|y|z|x2|y2|t|u|v|r}range' command 
//
void GnuPlot::UnsetRange(AXIS_INDEX axIdx)
{
	GpAxis * p_ax = &AxS[axIdx];
	p_ax->writeback_min = p_ax->set_min = axis_defaults[axIdx].min;
	p_ax->writeback_max = p_ax->set_max = axis_defaults[axIdx].max;
	p_ax->set_autoscale = AUTOSCALE_BOTH;
	p_ax->MinConstraint = CONSTRAINT_NONE;
	p_ax->MaxConstraint = CONSTRAINT_NONE;
	p_ax->range_flags = 0;
}
//
// process 'unset {x|y|x2|y2|z}zeroaxis' command 
//
void GnuPlot::UnsetZeroAxis(AXIS_INDEX axIdx)
{
	if(AxS[axIdx].zeroaxis != &default_axis_zeroaxis)
		SAlloc::F(AxS[axIdx].zeroaxis);
	AxS[axIdx].zeroaxis = NULL;
}
//
// process 'unset zeroaxis' command 
//
void GnuPlot::UnsetAllZeroAxes()
{
	for(/*AXIS_INDEX*/int axis = (AXIS_INDEX)0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++)
		UnsetZeroAxis((AXIS_INDEX)axis);
}

// process 'unset [xyz]{2}label command 
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
	bool save_interactive = _Plt.interactive;
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
		UpdateGpvalVariables(GPT.P_Term, 4);
		if(Pgm.AlmostEqualsCur("err$orstate")) {
			Pgm.Shift();
			return;
		}
		else {
		#ifdef USE_MOUSE
			// Reset key bindings only 
			if(Pgm.EqualsCur("bind")) {
				BindRemoveAll();
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
			//extern bool successful_initialization;
			if(successful_initialization) {
				FILE * fp = fopen("/tmp/gnuplot_debug.sav", "w+");
				replot_line[0] = '\0';
				SaveAll(fp);
				rewind(fp);
				LoadFile(fp, sstrdup("/tmp/gnuplot_debug.sav"), 1);
				// load_file closes fp 
			}
		#endif
			// Kludge alert, HBB 20000506: set to noninteractive mode, to
			// suppress some of the commentary output by the individual
			// unset_...() routines. 
			_Plt.interactive = false;
			UnsetSamples();
			UnsetIsoSamples();
			UnsetJitter();
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
			UnsetStyleRectangle();
			UnsetStyleCircle();
			UnsetStyleEllipse();
			UnsetPixmaps(); // delete pixmaps 
			// 'polar', 'parametric' and 'dummy' are interdependent, so be sure to keep the order intact 
			UnsetPolar();
			UnsetParametric();
			UnsetDummy();
			UnsetSpiderPlot();
			UnsetStyleSpiderPlot();
			GpAxis::UnsetLabelOrTitle(&Gg.LblTitle);
			ResetKey();
			UnsetView();
			for(axis = (AXIS_INDEX)0; axis<AXIS_ARRAY_SIZE; axis++) {
				GpAxis * this_axis = &AxS[axis];
				// Free contents before overwriting with default values 
				this_axis->Destroy();
				// Fill with generic values, then customize 
				memcpy(this_axis, &default_axis_state, sizeof(GpAxis));
				this_axis->formatstring = sstrdup(DEF_FORMAT);
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
			UnsetStyleParallel();
			AxS.DestroyShadowAxes();
			AxS.raxis = false;
			for(i = 2; i < MAX_TICLEVEL; i++)
				AxS.ticscale[i] = 1;
			UnsetTimeFmt();
			UnsetBoxPlot();
			UnsetBoxDepth();
			UnsetBoxWidth();
			Gg.ClipPoints = false;
			Gg.ClipLines1 = true;
			Gg.ClipLines2 = false;
			Gg.ClipRadial = false;
			Gg.border_lp = default_border_lp;
			Gg.border_layer = LAYER_FRONT;
			Gg.draw_border = 31;
			Gg.CornerPoles = true;
			_3DBlk.draw_surface = true;
			_3DBlk.implicit_surface = true;
			Gg.data_style = POINTSTYLE;
			Gg.func_style = LINES;
			// Reset individual plot style options to the default 
			Gg.filledcurves_opts_data.closeto = FILLEDCURVES_CLOSED;
			Gg.filledcurves_opts_func.closeto = FILLEDCURVES_CLOSED;
			UnsetGrid();
			AxS.grid_lp = default_grid_lp;
			AxS.mgrid_lp = default_grid_lp;
			AxS.polar_grid_angle = 0;
			AxS.grid_layer = LAYER_BEHIND;
			AxS.grid_tics_in_front = FALSE;
			for(i = 0; i < 5; i++)
				UnsetWall(i);
			SET_REFRESH_OK(E_REFRESH_NOT_OK, 0);
			ResetHidden3DOptions();
			_3DBlk.hidden3d = FALSE;
			UnsetAngles();
			ResetBars();
			UnsetMapping();
			UnsetSize();
			V.AspectRatio = 0.0f; // don't force it 
			Gr.RgbMax = 255.0;
			UnsetOrigin();
			UnsetTimeStamp();
			UnsetOffsets();
			UnsetContour();
			UnsetCntrParam();
			UnsetCntrLabel();
			UnsetZero();
			UnsetDGrid3D();
			UnsetTicsLevel();
			V.MarginB.UnsetMargin();
			V.MarginL.UnsetMargin();
			V.MarginR.UnsetMargin();
			V.MarginT.UnsetMargin();
			UnsetPointSize();
			UnsetPointIntervalBox();
			Pm3DReset();
			ResetColorBox();
			ResetPalette();
			DfUnsetDatafileBinary();
			UnsetFillStyle();
			UnsetHistogram();
			UnsetTextboxStyle();
			Gg.PreferLineStyles = false;
		#ifdef USE_MOUSE
			mouse_setting = default_mouse_setting;
		#endif
			// restore previous multiplot offset and margins 
			if(GPT.Flags & GpTerminalBlock::fMultiplot)
				MultiplotReset();
			UnsetMissing();
			ZFREE(_Df.df_separators);
			FREEANDASSIGN(_Df.df_commentschars, sstrdup(DEFAULT_COMMENTS_CHARS));
			DfInit();
			{ // Preserve some settings for `reset`, but not for `unset fit` 
				verbosity_level save_verbosity = _Fit.fit_verbosity;
				const bool save_errorscaling = _Fit.fit_errorscaling;
				UnsetFit();
				_Fit.fit_verbosity = save_verbosity;
				_Fit.fit_errorscaling = save_errorscaling;
			}
			UpdateGpvalVariables(GPT.P_Term, 0); // update GPVAL_ inner variables 
			// HBB 20000506: set 'interactive' back to its real value: 
			_Plt.interactive = save_interactive;
		}
	}
}

void GnuPlot::UnsetStyleRectangle()
{
	//GpObject foo(GpObject::defRectangle);// = DEFAULT_RECTANGLE_STYLE;
	//default_rectangle = foo;
	Gg.default_rectangle.SetDefaultRectangleStyle();
}

void GnuPlot::UnsetStyleCircle()
{
	//GpObject foo(GpObject::defCircle);// = DEFAULT_CIRCLE_STYLE;
	//default_circle = foo;
	Gg.default_circle.SetDefaultCircleStyle();
}

void GnuPlot::UnsetStyleEllipse()
{
	//GpObject foo(GpObject::defEllipse);// = DEFAULT_ELLIPSE_STYLE;
	//default_ellipse = foo;
	Gg.default_ellipse.SetDefaultEllipseStyle();
}

void GnuPlot::UnsetStyleParallel()
{
	pa_style parallel_axis_default; // = DEFAULT_PARALLEL_AXIS_STYLE;
	Gg.ParallelAxisStyle = parallel_axis_default;
}

void GnuPlot::UnsetWall(int which)
{
	Gg.GridWall[which].layer = LAYER_BEHIND;
}
//
// Invoked by "reset session".  There is no command line "reset mouse" 
//
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
