/* GNUPLOT - unset.c */

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

//static void unset_angles();
//static void unset_arrow();
//static void unset_arrowstyles();
static void free_arrowstyle(arrowstyle_def*);
//static void unset_autoscale();
//static void unset_bars();
//static void unset_border();
//static void unset_boxplot();
static void unset_cntrparam();
//static void unset_contour();
//static void unset_dashtype();
//static void unset_dummy();
static void unset_encoding();
static void unset_decimalsign();
//static void unset_fit();
//static void unset_hidden3d();
//static void unset_histogram();
#ifdef EAM_BOXED_TEXT
	static void unset_textbox_style();
#endif
//static void unset_historysize();
//static void unset_isosamples();
//static void unset_key();
//static void unset_label();
//static void unset_linestyle(linestyle_def** head);
//static void unset_linetype();
#ifdef EAM_OBJECTS
	//static void unset_style_rectangle();
	//static void unset_style_circle();
	//static void unset_style_ellipse();
#endif
static void unset_loadpath();
static void unset_locale();
static void unset_margin(GpPosition*);
static void unset_missing();
#ifdef USE_MOUSE
	//static void unset_mouse();
#endif
//static void unset_output();
//static void unset_pm3d();
//static void unset_palette();
static void unset_psdir();
//static void unset_surface();
//static void unset_table();
//static void unset_terminal();
static void unset_ticslevel();
static void unset_timefmt();

/******** The 'unset' command ********/
//void unset_command()
void GpGadgets::UnsetCommand(GpCommand & rC)
{
	int    found_token;
	int    save_token;
	int    i;
	rC.CToken++;
	save_token = rC.CToken;
	rC.P.P_SetIterator = rC.CheckForIteration();
	if(empty_iteration(rC.P.P_SetIterator)) {
		// Skip iteration [i=start:end] where start > end
		while(!rC.EndOfCommand())
			rC.CToken++;
		rC.P.CleanupSetIterator();
		return;
	}
	if(forever_iteration(rC.P.P_SetIterator)) {
		rC.P.CleanupSetIterator();
		GpGg.IntError(save_token, "unbounded iteration");
	}
	found_token = rC.LookupTable(&set_tbl[0], rC.CToken);
	// HBB 20000506: rationalize occurences of rC.CToken++ ...
	if(found_token != S_INVALID)
		rC.CToken++;
	save_token = rC.CToken;
ITERATE:
	switch(found_token) {
		case S_ANGLES:
		    Ang2Rad = 1.0; // unset_angles();
		    break;
		case S_ARROW:
		    UnsetArrow(rC);
		    break;
		case S_AUTOSCALE:
		    UnsetAutoScale(rC);
		    break;
		case S_BARS:
		    BarSize = 0.0; // unset_bars();
		    break;
		case S_BORDER:
		    DrawBorder = 0; // unset_border();
		    break;
		case S_BOXWIDTH:
		    UnsetBoxWidth();
		    break;
		case S_CLIP:
		    UnsetClip(rC);
		    break;
		case S_CNTRPARAM:
		    unset_cntrparam();
		    break;
		case S_CNTRLABEL:
		    UnsetCntrLabel();
		    break;
		case S_CLABEL: /* deprecated command */
		    clabel_onecolor = true;
		    break;
		case S_CONTOUR:
		    draw_contour = CONTOUR_NONE; // unset_contour();
		    break;
		case S_DASHTYPE:
		    UnsetDashType(rC);
		    break;
		case S_DGRID3D:
		    UnsetDGrid3D();
		    break;
		case S_DUMMY:
		    UnsetDummy(rC);
		    break;
		case S_ENCODING:
		    unset_encoding();
		    break;
		case S_DECIMALSIGN:
		    unset_decimalsign();
		    break;
		case S_FIT:
		    UnsetFit();
		    break;
		case S_FORMAT:
		    rC.CToken--;
		    SetFormat(rC);
		    break;
		case S_GRID:
		    UnsetGrid();
		    break;
		case S_HIDDEN3D:
		    //unset_hidden3d();
			hidden3d = false;
		    break;
		case S_HISTORY:
		    break; /* FIXME: reset to default values? */
		case S_HISTORYSIZE: /* Deprecated */
		    //unset_historysize();
			rC.H.gnuplot_history_size = -1; // don't ever truncate the history
		    break;
		case S_ISOSAMPLES:
		    UnsetIsoSamples();
		    break;
		case S_JITTER:
		    UnsetJitter();
		    break;
		case S_KEY:
		    //unset_key();
			{
				legend_key * p_key = &keyT;
				p_key->visible = false;
			}
		    break;
		case S_LABEL:
		    UnsetLabel(rC);
		    break;
		case S_LINETYPE:
		    UnsetLineType(rC);
		    break;
		case S_LINK:
		case S_NONLINEAR:
		    rC.CToken--;
		    LinkCommand(rC);
		    break;
		case S_LOADPATH:
		    unset_loadpath();
		    break;
		case S_LOCALE:
		    unset_locale();
		    break;
		case S_LOGSCALE:
		    UnsetLogscale(rC);
		    break;
		case S_MACROS:
		    /* Aug 2013 - macros are always enabled */
		    break;
		case S_MAPPING:
		    UnsetMapping();
		    break;
		case S_MARGIN:
		    unset_margin(&LMrg);
		    unset_margin(&RMrg);
		    unset_margin(&TMrg);
		    unset_margin(&BMrg);
		    break;
		case S_BMARGIN:
		    unset_margin(&BMrg);
		    break;
		case S_LMARGIN:
		    unset_margin(&LMrg);
		    break;
		case S_RMARGIN:
		    unset_margin(&RMrg);
		    break;
		case S_TMARGIN:
		    unset_margin(&TMrg);
		    break;
		case S_DATAFILE:
		    if(rC.AlmostEq("fort$ran")) {
			    GpDf.df_fortran_constants = false;
			    rC.CToken++;
			    break;
		    }
		    else if(rC.AlmostEq("miss$ing")) {
			    unset_missing();
			    rC.CToken++;
			    break;
		    }
		    else if(rC.AlmostEq("sep$arators")) {
			    ZFREE(GpDf.df_separators);
			    rC.CToken++;
			    break;
		    }
		    else if(rC.AlmostEq("com$mentschars")) {
			    free(GpDf.df_commentschars);
			    GpDf.df_commentschars = gp_strdup(DEFAULT_COMMENTS_CHARS);
			    rC.CToken++;
			    break;
		    }
		    else if(rC.AlmostEq("bin$ary")) {
			    GpDf.DfUnsetDatafileBinary();
			    rC.CToken++;
			    break;
		    }
		    else if(rC.AlmostEq("nofpe_trap")) {
			    GpDf.df_nofpe_trap = false;
			    rC.CToken++;
			    break;
		    }
		    GpDf.df_fortran_constants = false;
		    unset_missing();
		    ZFREE(GpDf.df_separators);
		    free(GpDf.df_commentschars);
		    GpDf.df_commentschars = gp_strdup(DEFAULT_COMMENTS_CHARS);
		    GpDf.DfUnsetDatafileBinary();
		    break;
		case S_MONOCHROME:
		    UnsetMonochrome(rC);
		    break;
#ifdef USE_MOUSE
		case S_MOUSE:
		    //unset_mouse();
			//static void unset_mouse()
			{
				Mse.Cfg.on = 0;
				UpdateStatusline(term); /* wipe status line */
			}
		    break;
#endif
		case S_MULTIPLOT:
		    TermEndMultiplot(term);
		    break;
		case S_OFFSETS:
		    UnsetOffsets();
		    break;
		case S_ORIGIN:
		    UnsetOrigin();
		    break;
		case SET_OUTPUT:
		    //unset_output();
			//static void unset_output()
			{
				if(IsMultiPlot) {
					IntErrorCurToken("you can't change the output in multiplot mode");
				}
				else {
					TermSetOutput(term, 0);
					ZFREE(outstr); // means STDOUT
				}
			}
		    break;
		case S_PARAMETRIC:
		    UnsetParametric(rC);
		    break;
		case S_PM3D:
		    //unset_pm3d();
			// process 'unset pm3d' command 
			{
				Pm3D.implicit = PM3D_EXPLICIT;
				// reset styles, required to 'plot something' after e.g. 'set pm3d map' 
				if(DataStyle == PM3DSURFACE) 
					DataStyle = POINTSTYLE;
				if(FuncStyle == PM3DSURFACE) 
					FuncStyle = LINES;
				Pm3D.border.l_type = LT_NODRAW;
			}
		    break;
		case S_PALETTE:
		    //unset_palette();
			//static void unset_palette()
			{
				rC.CToken++;
				fprintf(stderr, "you can't unset the palette.\n");
			}
		    break;
		case S_COLORBOX:
		    //unset_colorbox();
			ColorBox.SetDefault();
			ColorBox.where = SMCOLOR_BOX_NO;
		    break;
		case S_POINTINTERVALBOX:
		    //unset_pointintervalbox();
			PtIntervalBox = 1.0;
		    break;
		case S_POINTSIZE:
		    //unset_pointsize();
			PtSz = 1.0;
		    break;
		case S_POLAR:
		    UnsetPolar(rC);
		    break;
		case S_PRINT:
			// process 'unset print' command 
			rC.PrintSetOutput(NULL, false, false);
		    break;
		case S_PSDIR:
		    unset_psdir();
		    break;
#ifdef EAM_OBJECTS
		case S_OBJECT:
		    UnsetObject(rC);
		    break;
#endif
		case S_RTICS:
		    AxA[POLAR_AXIS].UnsetTics();
		    break;
		case S_PAXIS:
		    i = rC.IntExpression();
		    if(rC.AlmostEq("tic$s")) {
			    if(i > 0 && i <= (int)NumParallelAxes)
				    P_ParallelAxis[i-1].UnsetTics();
			    rC.CToken++;
		    }
		    break;
		case S_SAMPLES:
		    UnsetSamples();
		    break;
		case S_SIZE:
		    //unset_size();
			{
				XSz = 1.0;
				YSz = 1.0;
				ZSz = 1.0;
			}
		    break;
		case S_STYLE:
		    UnsetStyle(rC);
		    break;
		case S_SURFACE:
		    //unset_surface();
			//
			// process 'unset surface' command 
			//
			//static void unset_surface()
			{
				draw_surface = false;
			}
		    break;
		case S_TABLE:
		    //unset_table();
			//
			// process 'unset table' command 
			//
			//static void unset_table()
			{
				SFile::ZClose(&table_outfile);
				table_var = NULL;
				table_mode = false;
			}
		    break;
		case S_TERMINAL:
		    UnsetTerminal();
		    break;
		case S_TICS:
		    UnsetAllTics();
		    break;
		case S_TICSCALE:
		    IntWarn(rC.CToken, "Deprecated syntax - use 'set tics scale default'");
		    break;
		case S_TICSLEVEL:
		case S_XYPLANE:
		    unset_ticslevel();
		    break;
		case S_TIMEFMT:
		    unset_timefmt();
		    break;
		case S_TIMESTAMP:
		    UnsetTimestamp();
		    break;
		case S_TITLE:
		    title.UnsetAxisLabelOrTitle();
		    break;
		case S_VIEW:
		    UnsetView();
		    break;
		case S_ZERO:
		    UnsetZero();
		    break;
		// FIXME - are the tics correct? 
		case S_MXTICS:
		    AxA[FIRST_X_AXIS].UnsetMiniTics();
		    break;
		case S_XTICS:
		    AxA[FIRST_X_AXIS].UnsetTics();
		    break;
		case S_XDTICS:
		case S_XMTICS:
		    UnsetMonthDayTics(FIRST_X_AXIS);
		    break;
		case S_MYTICS:
		    AxA[FIRST_Y_AXIS].UnsetMiniTics();
		    break;
		case S_YTICS:
		    AxA[FIRST_Y_AXIS].UnsetTics();
		    break;
		case S_YDTICS:
		case S_YMTICS:
		    UnsetMonthDayTics(FIRST_X_AXIS);
		    break;
		case S_MX2TICS:
		    AxA[SECOND_X_AXIS].UnsetMiniTics();
		    break;
		case S_X2TICS:
		    AxA[SECOND_X_AXIS].UnsetTics();
		    break;
		case S_X2DTICS:
		case S_X2MTICS:
		    UnsetMonthDayTics(FIRST_X_AXIS);
		    break;
		case S_MY2TICS:
		    AxA[SECOND_Y_AXIS].UnsetMiniTics();
		    break;
		case S_Y2TICS:
		    AxA[SECOND_Y_AXIS].UnsetTics();
		    break;
		case S_Y2DTICS:
		case S_Y2MTICS:
		    UnsetMonthDayTics(FIRST_X_AXIS);
		    break;
		case S_MZTICS:
		    AxA[FIRST_Z_AXIS].UnsetMiniTics();
		    break;
		case S_ZTICS:
		    AxA[FIRST_Z_AXIS].UnsetTics();
		    break;
		case S_ZDTICS:
		case S_ZMTICS:
		    UnsetMonthDayTics(FIRST_X_AXIS);
		    break;
		case S_MCBTICS:
		    AxA[COLOR_AXIS].UnsetMiniTics();
		    break;
		case S_CBTICS:
		    AxA[COLOR_AXIS].UnsetTics();
		    break;
		case S_CBDTICS:
		case S_CBMTICS:
		    UnsetMonthDayTics(FIRST_X_AXIS);
		    break;
		case S_MRTICS:
		    AxA[POLAR_AXIS].UnsetMiniTics();
		    break;
		case S_XDATA:
		    UnsetTimedata(FIRST_X_AXIS);
		    break;
		case S_YDATA:
		    UnsetTimedata(FIRST_Y_AXIS);
		    break;
		case S_ZDATA:
		    UnsetTimedata(FIRST_Z_AXIS);
		    break;
		case S_CBDATA:
		    UnsetTimedata(COLOR_AXIS);
		    break;
		case S_X2DATA:
		    UnsetTimedata(SECOND_X_AXIS);
		    break;
		case S_Y2DATA:
		    UnsetTimedata(SECOND_Y_AXIS);
		    break;
		case S_XLABEL:
		    UnsetAxisLabel(FIRST_X_AXIS);
		    break;
		case S_YLABEL:
		    UnsetAxisLabel(FIRST_Y_AXIS);
		    break;
		case S_ZLABEL:
		    UnsetAxisLabel(FIRST_Z_AXIS);
		    break;
		case S_CBLABEL:
		    UnsetAxisLabel(COLOR_AXIS);
		    break;
		case S_X2LABEL:
		    UnsetAxisLabel(SECOND_X_AXIS);
		    break;
		case S_Y2LABEL:
		    UnsetAxisLabel(SECOND_Y_AXIS);
		    break;
		case S_XRANGE:
		    UnsetRange(FIRST_X_AXIS);
		    break;
		case S_X2RANGE:
		    UnsetRange(SECOND_X_AXIS);
		    break;
		case S_YRANGE:
		    UnsetRange(FIRST_Y_AXIS);
		    break;
		case S_Y2RANGE:
		    UnsetRange(SECOND_Y_AXIS);
		    break;
		case S_ZRANGE:
		    UnsetRange(FIRST_Z_AXIS);
		    break;
		case S_CBRANGE:
		    UnsetRange(COLOR_AXIS);
		    break;
		case S_RRANGE:
		    UnsetRange(POLAR_AXIS);
		    break;
		case S_TRANGE:
		    UnsetRange(T_AXIS);
		    break;
		case S_URANGE:
		    UnsetRange(U_AXIS);
		    break;
		case S_VRANGE:
		    UnsetRange(V_AXIS);
		    break;
		case S_RAXIS:
		    raxis = false;
		    rC.CToken++;
		    break;
		case S_XZEROAXIS:
		    UnsetZeroAxis(FIRST_X_AXIS);
		    break;
		case S_YZEROAXIS:
		    UnsetZeroAxis(FIRST_Y_AXIS);
		    break;
		case S_ZZEROAXIS:
		    UnsetZeroAxis(FIRST_Z_AXIS);
		    break;
		case S_X2ZEROAXIS:
		    UnsetZeroAxis(SECOND_X_AXIS);
		    break;
		case S_Y2ZEROAXIS:
		    UnsetZeroAxis(SECOND_Y_AXIS);
		    break;
		case S_ZEROAXIS:
		    UnsetAllZeroAxes();
		    break;
		case S_INVALID:
		default:
		    IntErrorCurToken("Unrecognized option.  See 'help unset'.");
		    break;
	}
	if(next_iteration(rC.P.P_SetIterator)) {
		rC.CToken = save_token;
		goto ITERATE;
	}
	GpGg.Ev.UpdateGpValVariables(0);
	rC.P.CleanupSetIterator();
}

void GpGadgets::UnsetArrow(GpCommand & rC)
{
	arrow_def * p_arrow;
	arrow_def * p_prev_arrow;
	if(rC.EndOfCommand()) {
		DestroyArrows();
	}
	else {
		// get tag
		int    tag = rC.IntExpression();
		if(!rC.EndOfCommand())
			IntErrorCurToken("extraneous arguments to unset arrow");
		for(p_arrow = first_arrow, p_prev_arrow = NULL; p_arrow; p_prev_arrow = p_arrow, p_arrow = p_arrow->next) {
			if(p_arrow->tag == tag) {
				DeleteArrow(p_prev_arrow, p_arrow);
				return; // exit, our job is done
			}
		}
	}
}
//
// delete the whole list of arrow styles
//
//static void unset_arrowstyles()
void GpGadgets::UnsetArrowStyles()
{
	free_arrowstyle(first_arrowstyle);
	first_arrowstyle = NULL;
}

static void free_arrowstyle(arrowstyle_def * arrowstyle)
{
	if(arrowstyle) {
		free_arrowstyle(arrowstyle->next);
		free(arrowstyle);
	}
}
//
// process 'unset autoscale' command 
//
//static void unset_autoscale()
void GpGadgets::UnsetAutoScale(GpCommand & rC)
{
	if(rC.EndOfCommand()) {
		int axis;
		for(axis = 0; axis < AXIS_ARRAY_SIZE; axis++)
			AxA[axis].SetAutoScale = (t_autoscale)false;
		for(axis = 0; axis < (int)NumParallelAxes; axis++)
			P_ParallelAxis[axis].SetAutoScale = (t_autoscale)false;
	}
	else if(rC.Eq("xy") || rC.Eq("tyx")) {
		AxA[FIRST_X_AXIS].SetAutoScale = AxA[FIRST_Y_AXIS].SetAutoScale = AUTOSCALE_NONE;
		rC.CToken++;
	}
	else {
		// HBB 20000506: parse axis name, and unset the right element of the array:
		int axis = rC.LookupTable(axisname_tbl, rC.CToken);
		if(axis >= 0) {
			AxA[axis].SetAutoScale = AUTOSCALE_NONE;
			rC.CToken++;
		}
	}
}
//
// reset is different from unset
// This gets called from 'set bars default' also
//
//void reset_bars()
void GpGraphics::ResetBars()
{
	BarLp.SetDefault2(); // DEFAULT_LP_STYLE_TYPE;
	BarLp.l_type = LT_DEFAULT;
	BarLp.pm3d_color.type = TC_VARIABLE;
	BarSize = 1.0;
	BarLayer = LAYER_FRONT;
}
//
// process 'unset boxwidth' command 
//
//static void unset_boxwidth()
void GpGadgets::UnsetBoxWidth()
{
	boxwidth = -1.0;
	boxwidth_is_absolute = true;
}
//
// process 'unset clip' command 
//
//static void unset_clip()
void GpGadgets::UnsetClip(GpCommand & rC)
{
	if(rC.EndOfCommand()) {
		// same as all three 
		ClipPoints = false;
		ClipLines1 = false;
		ClipLines2 = false;
	}
	else if(rC.AlmostEq("p$oints"))
		ClipPoints = false;
	else if(rC.AlmostEq("o$ne"))
		ClipLines1 = false;
	else if(rC.AlmostEq("t$wo"))
		ClipLines2 = false;
	else
		IntErrorCurToken("expecting 'points', 'one', or 'two'");
	rC.CToken++;
}

/* process 'unset cntrparam' command */
static void unset_cntrparam()
{
	contour_pts = DEFAULT_NUM_APPROX_PTS;
	contour_kind = CONTOUR_KIND_LINEAR;
	contour_order = DEFAULT_CONTOUR_ORDER;
	contour_levels = DEFAULT_CONTOUR_LEVELS;
	contour_levels_kind = LEVELS_AUTO;
}
//
// process 'unset cntrlabel' command 
//
//static void unset_cntrlabel()
void GpGraphics::UnsetCntrLabel()
{
	clabel_onecolor = false;
	clabel_start = 5;
	clabel_interval = 20;
	strcpy(contour_format, "%8.3g");
	ZFREE(P_ClabelFont);
}
//
// process 'unset contour' command 
// static void unset_contour() { GpGg.draw_contour = CONTOUR_NONE; }
//
// process 'unset dashtype' command 
//
//static void unset_dashtype()
void GpGadgets::UnsetDashType(GpCommand & rC)
{
	custom_dashtype_def * p_this, * prev;
	if(rC.EndOfCommand()) {
		// delete all 
		while(first_custom_dashtype)
			delete_dashtype((custom_dashtype_def*)NULL, first_custom_dashtype);
	}
	else {
		int tag = rC.IntExpression();
		for(p_this = first_custom_dashtype, prev = NULL; p_this != NULL;
		    prev = p_this, p_this = p_this->next) {
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
//static void unset_dgrid3d()
void GpGadgets::UnsetDGrid3D()
{
	dgrid3d_row_fineness = 10;
	dgrid3d_col_fineness = 10;
	dgrid3d_norm_value = 1;
	dgrid3d_mode = DGRID3D_QNORM;
	dgrid3d_x_scale = 1.0;
	dgrid3d_y_scale = 1.0;
	dgrid3d = false;
}
//
// process 'unset dummy' command
//
//static void unset_dummy()
void GpGadgets::UnsetDummy(GpCommand & rC)
{
	strcpy(rC.P.SetDummyVar[0], "x");
	strcpy(rC.P.SetDummyVar[1], "y");
	for(int i = 2; i < MAX_NUM_VAR; i++)
		*rC.P.SetDummyVar[i] = '\0';
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
void GpGadgets::UnsetFit()
{
	ZFREE(GpF.fitlogfile);
	GpF.fit_errorvariables = true;
	GpF.fit_covarvariables = false;
	GpF.fit_errorscaling = true;
	GpF.fit_prescale = true;
	GpF.fit_verbosity = BRIEF;
	Ev.DelUdvByName(GpF.FITLIMIT, false);
	GpF.epsilon_abs = 0.;
	Ev.DelUdvByName(GpF.FITMAXITER, false);
	Ev.DelUdvByName(GpF.FITSTARTLAMBDA, false);
	Ev.DelUdvByName(GpF.FITLAMBDAFACTOR, false);
	ZFREE(GpF.fit_script);
	GpF.fit_wrap = 0;
	// do not reset fit_v4compatible 
}
//
// process 'unset grid' command 
//
//static void unset_grid()
void GpGadgets::UnsetGrid()
{
	/* FIXME HBB 20000506: there is no command to explicitly reset the
	 * linetypes for major and minor gridlines. This function should
	 * do that, maybe... */
	/* grid_selection = GRID_OFF; */
	for(int i = FIRST_AXES; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++) {
		//AxA[i].gridmajor = false;
		//AxA[i].gridminor = false;
		AxA[i].Flags &= ~(GpAxis::fGridMajor|GpAxis::fGridMinor);
	}
}

//
// process 'unset {x|y|z|x2|y2|t|u|v|r}range' command
//
void GpGadgets::UnsetRange(AXIS_INDEX axis)
{
	GpAxis & r_ax = AxA[axis];
	r_ax.SetRange.Set(axis_defaults[axis].min, axis_defaults[axis].max);
	r_ax.WritebackRange = r_ax.SetRange;
	//r_ax.writeback_min = r_ax.SetRange.low = axis_defaults[axis].min;
	//r_ax.writeback_max = r_ax.SetRange.upp = axis_defaults[axis].max;
	r_ax.SetAutoScale = AUTOSCALE_BOTH;
	r_ax.min_constraint = CONSTRAINT_NONE;
	r_ax.max_constraint = CONSTRAINT_NONE;
	r_ax.range_flags = 0;
}

void GpGadgets::UnsetHistogram()
{
	histogram_style foo; // = DEFAULT_HISTOGRAM_STYLE;
	free(histogram_opts.title.font);
	free_histlist(&histogram_opts);
	histogram_opts = foo;
}

#ifdef EAM_BOXED_TEXT
static void unset_textbox_style()
{
	textbox_opts.SetDefault();
}

#endif

// process 'unset historysize' command DEPRECATED 
//static void unset_historysize() { GpGg.Gp__C.H.gnuplot_history_size = -1; } // don't ever truncate the history
//
// process 'unset isosamples' command 
//
//static void unset_isosamples()
void GpGadgets::UnsetIsoSamples()
{
	// HBB 20000506: was freeing 2D data structures although isosamples are only used by 3D plots.
	sp_free(P_First3DPlot);
	P_First3DPlot = NULL;
	iso_samples_1 = ISO_SAMPLES;
	iso_samples_2 = ISO_SAMPLES;
}

//void reset_key()
void GpGadgets::ResetKey()
{
	free(keyT.font);
	free(keyT.title.text);
	free(keyT.title.font);
	keyT.SetDefault();
}
//
// process 'unset key' command 
//
//static void unset_key() { legend_key * p_key = &GpGg.keyT; p_key->visible = false; }
//
// process 'unset label' command 
//
//static void unset_label()
void GpGadgets::UnsetLabel(GpCommand & rC)
{
	GpTextLabel * p_label;
	GpTextLabel * p_prev_label;
	int tag;
	if(rC.EndOfCommand()) {
		DestroyLabeles();
	}
	else {
		// get tag 
		tag = rC.IntExpression();
		if(!rC.EndOfCommand())
			IntErrorCurToken("extraneous arguments to unset label");
		for(p_label = first_label, p_prev_label = NULL; p_label != NULL; p_prev_label = p_label, p_label = p_label->next) {
			if(p_label->tag == tag) {
				DeleteLabel(p_prev_label, p_label);
				return; // exit, our job is done
			}
		}
	}
}

//static void unset_linestyle(linestyle_def ** head)
void GpGadgets::UnsetLineStyle(GpCommand & rC, linestyle_def ** ppHead)
{
	int tag = rC.IntExpression();
	linestyle_def * p_this, * prev;
	for(p_this = *ppHead, prev = NULL; p_this; prev = p_this, p_this = p_this->next) {
		if(p_this->tag == tag) {
			delete_linestyle(ppHead, prev, p_this);
			break;
		}
	}
}

//static void unset_linetype()
void GpGadgets::UnsetLineType(GpCommand & rC)
{
	if(rC.Eq("cycle")) {
		linetype_recycle_count = 0;
		rC.CToken++;
	}
	else if(!rC.EndOfCommand())
		UnsetLineStyle(rC, &first_perm_linestyle);
}

// process 'unset loadpath' command 
static void unset_loadpath()
{
	clear_loadpath();
}

/* process 'unset locale' command */
static void unset_locale()
{
	init_locale();
}

//static void reset_logscale(GpAxis * pAx)
void GpGadgets::ResetLogScale(GpAxis * pAx)
{
	bool undo_rlog = (pAx->Index == POLAR_AXIS && (pAx->Flags & GpAxis::fLog));
	pAx->Flags &= ~GpAxis::fLog;
	// Do not zero the base because we can still use it for gprintf formats
	// %L and %l with linear axis scaling.
	// pAx->base = 0.0;
	//
	if(undo_rlog)
		RRangeToXY();
}
//
// process 'unset logscale' command 
//
void GpGadgets::UnsetLogscale(GpCommand & rC)
{
	int axis;
	if(rC.EndOfCommand()) {
		// clean all the islog flags. This will hit some currently unused ones, too, but that's actually a good thing, IMHO
		for(axis = 0; axis < AXIS_ARRAY_SIZE; axis++)
			ResetLogScale(&AxA[axis]);
		for(axis = 0; axis < (int)NumParallelAxes; axis++)
			ResetLogScale(&P_ParallelAxis[axis]);
	}
	else {
		int i = 0;
		// do reverse search because of "x", "x1", "x2" sequence in axisname_tbl
		while(i < rC.P_Token[rC.CToken].length) {
			axis = lookup_table_nth_reverse(axisname_tbl, NUMBER_OF_MAIN_VISIBLE_AXES, rC.P_InputLine + rC.P_Token[rC.CToken].start_index + i);
			if(axis < 0) {
				rC.P_Token[rC.CToken].start_index += i;
				IntErrorCurToken("invalid axis");
			}
			ResetLogScale(&AxA[axisname_tbl[axis].value]);
			i += strlen(axisname_tbl[axis].key);
		}
		++rC.CToken;
	}
	// Because the log scaling is applied during data input, a quick refresh
	// using existing stored data will not work if the log setting changes.
	SetRefreshOk(E_REFRESH_NOT_OK, 0);
}
//
// process 'unset mapping3d' command
//
//static void unset_mapping()
void GpGadgets::UnsetMapping()
{
	// assuming same as points 
	mapping3d = MAP3D_CARTESIAN;
}
//
// process 'unset {blrt}margin' command 
//
static void unset_margin(GpPosition * margin)
{
	margin->SetX(character, -1.0);
}

/* process 'unset datafile' command */
static void unset_missing()
{
	ZFREE(GpDf.missing_val);
}

//void unset_monochrome(GpCommand & rC)
void GpGadgets::UnsetMonochrome(GpCommand & rC)
{
	IsMonochrome = false;
	if(rC.Eq("lt") || rC.AlmostEq("linet$ype")) {
		rC.CToken++;
		if(!rC.EndOfCommand())
			UnsetLineStyle(rC, &first_mono_linestyle);
	}
	term->flags &= ~TERM_MONOCHROME;
}

// process 'unset offsets' command
//static void unset_offsets()
void GpGadgets::UnsetOffsets()
{
	loff.x = roff.x = 0.0;
	toff.y = boff.y = 0.0;
}
//
// process 'unset origin' command 
//
//static void unset_origin()
void GpGadgets::UnsetOrigin()
{
	XOffs = 0.0;
	YOffs = 0.0;
}
//
// process 'unset psdir' command 
//
static void unset_psdir()
{
	ZFREE(PS_psdir);
}
//
// process 'unset parametric' command 
//
//static void unset_parametric()
void GpGadgets::UnsetParametric(GpCommand & rC)
{
	if(IsParametric) {
		IsParametric = false;
		if(!IsPolar) { // keep t for polar 
			UnsetDummy(rC);
			if(IsInteractive)
				fprintf(stderr, "\n\tdummy variable is x for curves, x/y for surfaces\n");
		}
	}
}

void GpGadgets::UnsetPolar(GpCommand & rC)
{
	if(IsPolar) {
		IsPolar = false;
		if(IsParametric && AxA[T_AXIS].SetAutoScale) {
			// only if user has not set an explicit range
			AxA[T_AXIS].SetRange.Set(axis_defaults[T_AXIS].min, axis_defaults[T_AXIS].min);
		}
		if(!IsParametric) {
			strcpy(rC.P.SetDummyVar[0], "x");
			if(IsInteractive)
				fprintf(stderr, "\n\tdummy variable is x for curves\n");
		}
	}
}
//
// process 'unset samples' command 
//
//static void unset_samples()
void GpGadgets::UnsetSamples()
{
	// HBB 20000506: unlike unset_isosamples(), this one *has* to clear 2D data structues!
	cp_free(P_FirstPlot);
	P_FirstPlot = NULL;
	sp_free(P_First3DPlot);
	P_First3DPlot = NULL;
	Samples1 = SAMPLES;
	Samples2 = SAMPLES;
}
//
// process 'unset size' command
//
//static void unset_size() { GpGg.XSz = 1.0; GpGg.YSz = 1.0; GpGg.ZSz = 1.0; }
//
// process 'unset style' command 
//
//static void unset_style()
void GpGadgets::UnsetStyle(GpCommand & rC)
{
	if(rC.EndOfCommand()) {
		DataStyle = POINTSTYLE;
		FuncStyle = LINES;
		while(first_linestyle)
			delete_linestyle(&first_linestyle, NULL, first_linestyle);
		UnsetFillStyle();
#ifdef EAM_OBJECTS
		DefaultRectangle.SetDefaultRectangleStyle();
		DefaultCircle.SetDefaultCircleStyle();
		DefaultEllipse.SetDefaultEllipseStyle();
#endif
		UnsetHistogram();
		boxplot_opts.SetDefault(); //unset_boxplot();
#ifdef EAM_BOXED_TEXT
		unset_textbox_style();
#endif
		rC.CToken++;
		return;
	}
	switch(rC.LookupTable(show_style_tbl, rC.CToken)) {
		case SHOW_STYLE_DATA:
		    DataStyle = POINTSTYLE;
		    rC.CToken++;
		    break;
		case SHOW_STYLE_FUNCTION:
		    FuncStyle = LINES;
		    rC.CToken++;
		    break;
		case SHOW_STYLE_LINE:
		    rC.CToken++;
		    if(rC.EndOfCommand()) {
			    while(first_linestyle)
				    delete_linestyle(&first_linestyle, NULL, first_linestyle);
		    }
		    else {
			    UnsetLineStyle(rC, &first_linestyle);
		    }
		    break;
		case SHOW_STYLE_FILLING:
		    UnsetFillStyle();
		    rC.CToken++;
		    break;
		case SHOW_STYLE_HISTOGRAM:
		    UnsetHistogram();
		    rC.CToken++;
		    break;
		case SHOW_STYLE_ARROW:
		    UnsetArrowStyles();
		    rC.CToken++;
		    break;
#ifdef EAM_OBJECTS
		case SHOW_STYLE_RECTANGLE:
		    //unset_style_rectangle();
			DefaultRectangle.SetDefaultRectangleStyle();
		    rC.CToken++;
		    break;
		case SHOW_STYLE_CIRCLE:
		    //unset_style_circle();
			DefaultCircle.SetDefaultCircleStyle();
		    rC.CToken++;
		    break;
		case SHOW_STYLE_ELLIPSE:
		    //unset_style_ellipse();
			DefaultEllipse.SetDefaultEllipseStyle();
		    rC.CToken++;
		    break;
#endif
#ifdef EAM_BOXED_TEXT
		case SHOW_STYLE_TEXTBOX:
		    unset_textbox_style();
		    rC.CToken++;
		    break;
#endif
		case SHOW_STYLE_BOXPLOT:
		    boxplot_opts.SetDefault(); // unset_boxplot();
		    rC.CToken++;
		    break;
		default:
		    IntErrorCurToken("unrecognized style");
	}
}
//
// process 'unset terminal' comamnd
// Aug 2012:  restore original terminal type
//
//static void unset_terminal()
void GpGadgets::UnsetTerminal()
{
	UdvtEntry * original_terminal = Ev.GetUdvByName("GNUTERM");
	if(IsMultiPlot)
		TermEndMultiplot(term);
	term_reset();
	if(original_terminal && original_terminal->udv_value.type != NOTDEFINED) {
		char * termname = original_terminal->udv_value.v.string_val;
		term = change_term(termname, strlen(termname));
	}
	screen_ok = false;
}

// process 'unset ticslevel' command 
static void unset_ticslevel()
{
	GpGg.xyplane.Set(0.5, false);
}

/* Process 'unset P_TimeFormat' command */
static void unset_timefmt()
{
	free(GpGg.P_TimeFormat);
	GpGg.P_TimeFormat = gp_strdup(TIMEFMT);
}
//
// process 'unset timestamp' command 
//
//static void unset_timestamp()
void GpGadgets::UnsetTimestamp()
{
	timelabel.UnsetAxisLabelOrTitle();
	timelabel_rotate = 0;
	timelabel_bottom = true;
}
//
// process 'unset view' command 
//
//static void unset_view()
void GpGraphics::UnsetView()
{
	splot_map = false;
	GpGg.AspectRatio3D = 0;
	surface_rot_z = 30.0;
	surface_rot_x = 60.0;
	surface_scale = 1.0;
	surface_lscale = 0.0;
	surface_zscale = 1.0;
}

//static void unset_axislabel(AXIS_INDEX axis)
void GpAxisBlock::UnsetAxisLabel(AXIS_INDEX axis)
{
	AxA[axis].label.UnsetAxisLabelOrTitle();
	AxA[axis].label = default_axis_label;
	if(oneof3(axis, FIRST_Y_AXIS, SECOND_Y_AXIS, COLOR_AXIS))
		AxA[axis].label.rotate = TEXT_VERTICAL;
}

/******** The 'reset' command ********/
/* HBB 20000506: I moved this here, from set.c, because 'reset' really
 * is more like a big lot of 'unset' commands, rather than a bunch of
 * 'set's. The benefit is that it can make use of many of the
 * unset_something() contained in this module, i.e. you now have one
 * place less to keep in sync if the semantics or defaults of any
 * option is changed. This is only true for options for which 'unset'
 * state is the default, however, e.g. not for 'surface', 'bars' and some others. */
//void reset_command()
void GpGadgets::ResetCommand(GpCommand & rC)
{
	int    i;
	int    axis;
	const bool save_interactive = IsInteractive;
	rC.CToken++;
	// Reset session state as well as internal graphics state
	if(rC.Eq("session")) {
		Ev.ClearUdfList();
		InitConstants();
		InitSession(rC);
		return;
	}
	// Reset error state (only?)
	Ev.UpdateGpValVariables(4);
	if(rC.AlmostEq("err$orstate")) {
		rC.CToken++;
		return;
	}
#ifdef USE_MOUSE
	// Reset key bindings only
	if(rC.Eq("bind")) {
		Mse.BindRemoveAll();
		rC.CToken++;
		return;
	}
#endif
	if(!(rC.EndOfCommand())) {
		IntWarn(rC.CToken, "invalid option, expecting 'bind' or 'errorstate'");
		while(!(rC.EndOfCommand()))
			rC.CToken++;
	}
	// Kludge alert, HBB 20000506: set to noninteractive mode, to
	// suppress some of the commentary output by the individual unset_...() routines
	IsInteractive = false;
	UnsetSamples();
	UnsetIsoSamples();
	UnsetJitter();
	DestroyArrows();
	UnsetArrowStyles();
	DestroyLabeles();
	// delete linestyles
	while(first_linestyle)
		delete_linestyle(&first_linestyle, NULL, first_linestyle);
#ifdef EAM_OBJECTS
	DestroyObjects();
	//unset_style_rectangle();
	//unset_style_circle();
	//unset_style_ellipse();
	DefaultRectangle.SetDefaultRectangleStyle();
	DefaultCircle.SetDefaultCircleStyle();
	DefaultEllipse.SetDefaultEllipseStyle();
#endif
	// 'polar', 'parametric' and 'dummy' are interdependent, so be sure to keep the order intact
	UnsetPolar(rC);
	UnsetParametric(rC);
	UnsetDummy(rC);
	title.UnsetAxisLabelOrTitle();
	ResetKey();
	UnsetView();
	for(axis = FIRST_AXES; axis < AXIS_ARRAY_SIZE; axis++) {
		GpAxis & r_ax = AxA[axis];
		// Free contents before overwriting with default values
		r_ax.Destroy();
		// Fill with generic values, then customize
		//memcpy(this_axis, &default_axis_state, sizeof(GpAxis));
		r_ax.SetDefault();
		r_ax.formatstring = gp_strdup(DEF_FORMAT);
		r_ax.Index = axis;
		UnsetAxisLabel((AXIS_INDEX)axis); /* sets vertical label for y/y2/cb */
		UnsetRange((AXIS_INDEX)axis); /* copies min/max from axis_defaults */
		// 'tics' default is on for some, off for the other axes:
		r_ax.UnsetTics();
		r_ax.UnsetMiniTics();
		r_ax.ticdef = default_axis_ticdef;
		r_ax.minitics = MINI_DEFAULT;
		r_ax.ticmode = axis_defaults[axis].ticmode;
		ResetLogScale(&r_ax);
	}
	// Free all previously allocated parallel axis structures
	for(axis = FIRST_AXES; axis < (int)NumParallelAxes; axis++) {
		P_ParallelAxis[axis].Destroy();
	}
	ZFREE(P_ParallelAxis);
	NumParallelAxes = 0;
#ifdef NONLINEAR_AXES
	if(shadow_axis_array) {
		for(i = 0; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++)
			shadow_axis_array[i].Destroy();
		ZFREE(shadow_axis_array);
	}
#endif
	raxis = true;
	for(i = 2; i < MAX_TICLEVEL; i++)
		TicScale[i] = 1.0;
	unset_timefmt();
	boxplot_opts.SetDefault(); // unset_boxplot();
	UnsetBoxWidth();
	ClipPoints = false;
	ClipLines1 = true;
	ClipLines2 = false;
	BorderLp = DefaultBorderLp;
	DrawBorder = 31;
	draw_surface = true;
	implicit_surface = true;
	DataStyle = POINTSTYLE;
	FuncStyle = LINES;
	// Reset individual plot style options to the default
	FilledcurvesOptsData.closeto = FILLEDCURVES_CLOSED;
	FilledcurvesOptsFunc.closeto = FILLEDCURVES_CLOSED;
	UnsetGrid();
	grid_lp = default_grid_lp;
	mgrid_lp = default_grid_lp;
	polar_grid_angle = 0;
	grid_layer = LAYER_BEHIND;
	grid_tics_in_front = false;
	SetRefreshOk(E_REFRESH_NOT_OK, 0);
	reset_hidden3doptions();
	hidden3d = false;
	Ang2Rad = 1.0; // unset_angles();
	ResetBars();
	UnsetMapping();
	//unset_size();
	{
		XSz = 1.0;
		YSz = 1.0;
		ZSz = 1.0;
	}
	AspectRatio = 0.0;     /* don't force it */
	UnsetOrigin();
	UnsetTimestamp();
	UnsetOffsets();
	draw_contour = CONTOUR_NONE; // unset_contour();
	unset_cntrparam();
	UnsetCntrLabel();
	UnsetZero();
	UnsetDGrid3D();
	unset_ticslevel();
	unset_margin(&BMrg);
	unset_margin(&LMrg);
	unset_margin(&RMrg);
	unset_margin(&TMrg);
	//unset_pointsize();
	PtSz = 1.0;
	//unset_pointintervalbox();
	PtIntervalBox = 1.0;
	pm3d_reset();
	//reset_colorbox();
	ColorBox.SetDefault();
	ResetPalette();
	GpDf.DfUnsetDatafileBinary();
	UnsetFillStyle();
	UnsetHistogram();
#ifdef EAM_BOXED_TEXT
	unset_textbox_style();
#endif
#ifdef BACKWARDS_COMPATIBLE
	prefer_line_styles = false;
#endif
#ifdef USE_MOUSE
	Mse.Cfg.SetDefault(); // = default_mouse_setting;
#endif
	unset_missing();
	ZFREE(GpDf.df_separators);
	free(GpDf.df_commentschars);
	GpDf.df_commentschars = gp_strdup(DEFAULT_COMMENTS_CHARS);
	GpDf.DfInit();
	{ // Preserve some settings for `reset`, but not for `unset fit`
		verbosity_level save_verbosity = GpF.fit_verbosity;
		bool save_errorscaling = GpF.fit_errorscaling;
		UnsetFit();
		GpF.fit_verbosity = save_verbosity;
		GpF.fit_errorscaling = save_errorscaling;
	}
	Ev.UpdateGpValVariables(0); // update GPVAL_ inner variables
	// HBB 20000506: set 'interactive' back to its real value:
	IsInteractive = save_interactive;
}

#ifdef EAM_OBJECTS
//static void unset_style_rectangle() { GpGg.DefaultRectangle.SetDefaultRectangleStyle(); }
//static void unset_style_circle() { GpGg.DefaultCircle.SetDefaultCircleStyle(); }
//static void unset_style_ellipse() { GpGg.DefaultEllipse.SetDefaultEllipseStyle(); }

#endif
