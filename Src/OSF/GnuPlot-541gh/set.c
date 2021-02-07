// GNUPLOT - set.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
/*
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 */
#include <gnuplot.h>
#pragma hdrstop

static palette_color_mode pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_NONE;

static int  assign_arrow_tag();
static void set_border();
static void set_boxdepth();
static void set_boxwidth();
static void set_clip();
static void set_contour();
static void set_cornerpoles();
static void set_dgrid3d();
static void set_decimalsign();
static void set_dummy();
static void set_encoding();
static void set_label();
static int  assign_label_tag();
static void set_loadpath();
static void set_fontpath();
static void set_locale();
//static void set_mapping();
static void set_minus_sign();
static void set_micro();
static void set_missing();
static void set_output();
static void set_parametric();
static void set_pointsize();
static void set_pointintervalbox();
static void set_print();
static void set_psdir();
static void set_rgbmax();
static void set_samples();
//static void set_surface();
static void set_timefmt();
static void set_zero();
static void set_raxis();
static void set_xyplane();
static void set_ticslevel();

/******** Local functions ********/

static void set_xyzlabel(text_label * label);
static void load_tics(GpAxis * axis);
static void load_tic_user(GpAxis * axis);
static void load_tic_series(GpAxis * axis);
static int assign_arrowstyle_tag();
static void set_mttics(GpAxis * this_axis);
//static void set_spiderplot();

static const GpPosition default_position = {first_axes, first_axes, first_axes, 0., 0., 0.};
static const GpPosition default_offset = {character, character, character, 0., 0., 0.};
lp_style_type default_hypertext_point_style(lp_style_type::defHypertextPoint); // = {1, LT_BLACK, 4, DASHTYPE_SOLID, 0, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, {TC_RGB, 0x000000, 0.0}, DEFAULT_DASHPATTERN};

/******** The 'set' command ********/
//void set_command()
void GnuPlot::SetCommand()
{
	Pgm.Shift();
	// Mild form of backwards compatibility 
	// Allow "set no{foo}" rather than "unset foo" 
	const int _start_index = Pgm.GetCurTokenStartIndex();
	if(gp_input_line[_start_index] == 'n' && gp_input_line[_start_index+1] == 'o' && gp_input_line[_start_index+2] != 'n') {
		if(interactive)
			IntWarnCurToken("deprecated syntax, use \"unset\"");
		Pgm.P_Token[Pgm.CToken].start_index += 2;
		Pgm.P_Token[Pgm.CToken].length -= 2;
		Pgm.Rollback();
		UnsetCommand();
	}
	else {
		int save_token = Pgm.GetCurTokenIdx();
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
		save_token = Pgm.GetCurTokenIdx();
ITERATE:
		switch(Pgm.LookupTableForCurrentToken(&set_tbl[0])) {
			case S_ANGLES: SetAngles(); break;
			case S_ARROW: SetArrow(); break;
			case S_AUTOSCALE: SetAutoscale(); break;
			case S_BARS: SetBars(); break;
			case S_BORDER: set_border(); break;
			case S_BOXDEPTH: set_boxdepth(); break;
			case S_BOXWIDTH: set_boxwidth(); break;
			case S_CLIP: set_clip(); break;
			case S_COLOR: 
				unset_monochrome();
			    Pgm.Shift();
			    break;
			case S_COLORMAP: SetColorMap(); break;
			case S_COLORSEQUENCE: SetColorSequence(0); break;
			case S_CNTRPARAM: SetCntrParam(); break;
			case S_CNTRLABEL: SetCntrLabel(); break;
			case S_CONTOUR: SetContour(); break;
			case S_CORNERPOLES: set_cornerpoles(); break;
			case S_DASHTYPE: SetDashType(); break;
			case S_DGRID3D: set_dgrid3d(); break;
			case S_DEBUG:
			    /* Developer-only option (not in user documentation) */
			    /* Does nothing in normal use */
			    Pgm.Shift();
			    debug = IntExpression();
			    break;
			case S_DECIMALSIGN:
			    set_decimalsign();
			    break;
			case S_DUMMY:
			    set_dummy();
			    break;
			case S_ENCODING:
			    set_encoding();
			    break;
			case S_FIT: SetFit(); break;
			case S_FONTPATH:
			    set_fontpath();
			    break;
			case S_FORMAT: SetFormat(); break;
			case S_GRID: SetGrid(); break;
			case S_HIDDEN3D: SetHidden3D(); break;
			case S_HISTORYSIZE: /* Deprecated in favor of "set history size" */
			case S_HISTORY: SetHistory(); break;
			case S_PIXMAP: SetPixMap(); break;
			case S_ISOSAMPLES: SetIsoSamples(); break;
			case S_ISOSURFACE: SetIsoSurface(); break;
			case S_JITTER: SetJitter(); break;
			case S_KEY: SetKey(); break;
			case S_LINESTYLE: SetLineStyle(&first_linestyle, LP_STYLE); break;
			case S_LINETYPE:
			    if(Pgm.EqualsNext("cycle")) {
					Pgm.Shift();
					Pgm.Shift();
				    linetype_recycle_count = IntExpression();
			    }
			    else
				    SetLineStyle(&first_perm_linestyle, LP_TYPE);
			    break;
			case S_LABEL:
			    set_label();
			    break;
			case S_LINK:
			case S_NONLINEAR: LinkCommand(); break;
			case S_LOADPATH:
			    set_loadpath();
			    break;
			case S_LOCALE:
			    set_locale();
			    break;
			case S_LOGSCALE:
			    SetLogScale();
			    break;
			case S_MACROS:
			    /* Aug 2013 - macros are always enabled */
			    Pgm.Shift();
			    break;
			case S_MAPPING: SetMapping(); break;
			case S_MARGIN:
			    // Jan 2015: CHANGE to order <left>,<right>,<bottom>,<top> 
			    SetMargin(&V.MarginL);
			    if(!Pgm.EqualsCur(","))
				    break;
			    SetMargin(&V.MarginR);
			    if(!Pgm.EqualsCur(","))
				    break;
			    SetMargin(&V.MarginB);
			    if(!Pgm.EqualsCur(","))
				    break;
			    SetMargin(&V.MarginT);
			    break;
			case S_BMARGIN: SetMargin(&V.MarginB); break;
			case S_LMARGIN: SetMargin(&V.MarginL); break;
			case S_RMARGIN: SetMargin(&V.MarginR); break;
			case S_TMARGIN: SetMargin(&V.MarginT); break;
			case S_MICRO:
			    set_micro();
			    break;
			case S_MINUS_SIGN:
			    set_minus_sign();
			    break;
			case S_DATAFILE: SetDataFile(); break;
			case S_MOUSE: SetMouse(); break;
			case S_MONOCHROME: SetMonochrome(); break;
			case S_MULTIPLOT: TermStartMultiplot(term); break;
			case S_OFFSETS: SetOffsets(); break;
			case S_ORIGIN:
				{
					Pgm.Shift();
					if(Pgm.EndOfCommand()) {
						V.XOffset = 0.0f;
						V.YOffset = 0.0f;
					}
					else {
						V.XOffset = static_cast<float>(RealExpression());
						if(!Pgm.EqualsCur(","))
							IntErrorCurToken("',' expected");
						Pgm.Shift();
						V.YOffset = static_cast<float>(RealExpression());
					}
				}
			    break;
			case SET_OUTPUT:
			    set_output();
			    break;
			case S_OVERFLOW: SetOverflow(); break;
			case S_PARAMETRIC:
			    set_parametric();
			    break;
			case S_PM3D: SetPm3D(); break;
			case S_PALETTE: SetPalette(); break;
			case S_COLORBOX: SetColorBox(); break;
			case S_POINTINTERVALBOX:
			    set_pointintervalbox();
			    break;
			case S_POINTSIZE:
			    set_pointsize();
			    break;
			case S_POLAR: SetPolar(); break;
			case S_PRINT:
			    set_print();
			    break;
			case S_PSDIR:
			    set_psdir();
			    break;
			case S_OBJECT: SetObject(); break;
			case S_WALL:   SetWall(); break;
			case S_SAMPLES:
			    set_samples();
			    break;
			case S_RGBMAX:
			    set_rgbmax();
			    break;
			case S_SIZE: SetSize(); break;
			case S_SPIDERPLOT: SetSpiderPlot(); break;
			case S_STYLE: SetStyle(); break;
			case S_SURFACE: SetSurface(); break;
			case S_TABLE: SetTable(); break;
			case S_TERMINAL: SetTerminal(); break;
			case S_TERMOPTIONS: SetTermOptions(); break;
			case S_THETA: SetTheta(); break;
			case S_TICS: SetTics(); break;
			case S_TICSCALE: SetTicScale(); break;
			case S_TIMEFMT:
			    set_timefmt();
			    break;
			case S_TIMESTAMP: SetTimeStamp(); break;
			case S_TITLE:
			    set_xyzlabel(&title);
			    title.rotate = 0;
			    break;
			case S_VIEW: SetView(); break;
			case S_VGRID:
			    set_vgrid();
			    break;
			case S_VXRANGE:
			case S_VYRANGE:
			case S_VZRANGE: SetVGridRange(); break;
			case S_ZERO:
			    set_zero();
			    break;
			case S_MXTICS:
			case S_NOMXTICS:
			case S_XTICS:
			case S_NOXTICS:
			case S_XDTICS:
			case S_NOXDTICS:
			case S_XMTICS:
			case S_NOXMTICS: SetTicProp(&AxS[FIRST_X_AXIS]); break;
			case S_MYTICS:
			case S_NOMYTICS:
			case S_YTICS:
			case S_NOYTICS:
			case S_YDTICS:
			case S_NOYDTICS:
			case S_YMTICS:
			case S_NOYMTICS: SetTicProp(&AxS[FIRST_Y_AXIS]); break;
			case S_MX2TICS:
			case S_NOMX2TICS:
			case S_X2TICS:
			case S_NOX2TICS:
			case S_X2DTICS:
			case S_NOX2DTICS:
			case S_X2MTICS:
			case S_NOX2MTICS: SetTicProp(&AxS[SECOND_X_AXIS]); break;
			case S_MY2TICS:
			case S_NOMY2TICS:
			case S_Y2TICS:
			case S_NOY2TICS:
			case S_Y2DTICS:
			case S_NOY2DTICS:
			case S_Y2MTICS:
			case S_NOY2MTICS: SetTicProp(&AxS[SECOND_Y_AXIS]); break;
			case S_MZTICS:
			case S_NOMZTICS:
			case S_ZTICS:
			case S_NOZTICS:
			case S_ZDTICS:
			case S_NOZDTICS:
			case S_ZMTICS:
			case S_NOZMTICS: SetTicProp(&AxS[FIRST_Z_AXIS]); break;
			case S_MCBTICS:
			case S_NOMCBTICS:
			case S_CBTICS:
			case S_NOCBTICS:
			case S_CBDTICS:
			case S_NOCBDTICS:
			case S_CBMTICS:
			case S_NOCBMTICS: SetTicProp(&AxS[COLOR_AXIS]); break;
			case S_RTICS:
			case S_MRTICS: SetTicProp(&AxS[POLAR_AXIS]); break;
			case S_TTICS:  SetTicProp(&AxS.Theta()); break;
			case S_MTTICS:
			    set_mttics(&AxS.Theta());
			    break;
			case S_XDATA:
			    SetTimeData(&AxS[FIRST_X_AXIS]);
			    AxS[T_AXIS].datatype = AxS[U_AXIS].datatype = AxS[FIRST_X_AXIS].datatype;
			    break;
			case S_YDATA:
			    SetTimeData(&AxS[FIRST_Y_AXIS]);
			    AxS[V_AXIS].datatype = AxS[FIRST_X_AXIS].datatype;
			    break;
			case S_ZDATA:
			    SetTimeData(&AxS[FIRST_Z_AXIS]);
			    break;
			case S_CBDATA:
			    SetTimeData(&AxS[COLOR_AXIS]);
			    break;
			case S_X2DATA:
			    SetTimeData(&AxS[SECOND_X_AXIS]);
			    break;
			case S_Y2DATA:
			    SetTimeData(&AxS[SECOND_Y_AXIS]);
			    break;
			case S_XLABEL:
			    set_xyzlabel(&AxS[FIRST_X_AXIS].label);
			    break;
			case S_YLABEL:
			    set_xyzlabel(&AxS[FIRST_Y_AXIS].label);
			    break;
			case S_ZLABEL:
			    set_xyzlabel(&AxS[FIRST_Z_AXIS].label);
			    break;
			case S_CBLABEL:
			    set_xyzlabel(&AxS[COLOR_AXIS].label);
			    break;
			case S_RLABEL:
			    set_xyzlabel(&AxS[POLAR_AXIS].label);
			    break;
			case S_X2LABEL:
			    set_xyzlabel(&AxS[SECOND_X_AXIS].label);
			    break;
			case S_Y2LABEL:
			    set_xyzlabel(&AxS[SECOND_Y_AXIS].label);
			    break;
			case S_XRANGE:
			    SetRange(&AxS[FIRST_X_AXIS]);
			    break;
			case S_X2RANGE:
			    SetRange(&AxS[SECOND_X_AXIS]);
			    break;
			case S_YRANGE:
			    SetRange(&AxS[FIRST_Y_AXIS]);
			    break;
			case S_Y2RANGE:
			    SetRange(&AxS[SECOND_Y_AXIS]);
			    break;
			case S_ZRANGE:
			    SetRange(&AxS[FIRST_Z_AXIS]);
			    break;
			case S_CBRANGE:
			    SetRange(&AxS[COLOR_AXIS]);
			    break;
			case S_RRANGE:
			    SetRange(&AxS[POLAR_AXIS]);
			    if(polar)
				    RRangeToXY();
			    break;
			case S_TRANGE:
			    SetRange(&AxS[T_AXIS]);
			    break;
			case S_URANGE:
			    SetRange(&AxS[U_AXIS]);
			    break;
			case S_VRANGE:
			    SetRange(&AxS[V_AXIS]);
			    break;
			case S_PAXIS: SetPAxis(); break;
			case S_RAXIS:
			    set_raxis();
			    break;
			case S_XZEROAXIS: SetZeroAxis(FIRST_X_AXIS); break;
			case S_YZEROAXIS: SetZeroAxis(FIRST_Y_AXIS); break;
			case S_ZZEROAXIS: SetZeroAxis(FIRST_Z_AXIS); break;
			case S_X2ZEROAXIS: SetZeroAxis(SECOND_X_AXIS); break;
			case S_Y2ZEROAXIS: SetZeroAxis(SECOND_Y_AXIS); break;
			case S_ZEROAXIS: SetAllZeroAxis(); break;
			case S_XYPLANE:
			    set_xyplane();
			    break;
			case S_TICSLEVEL:
			    set_ticslevel();
			    break;
			default:
			    IntErrorCurToken("unrecognized option - see 'help set'.");
			    break;
		}
		if(next_iteration(set_iterator)) {
			Pgm.SetTokenIdx(save_token);
			goto ITERATE;
		}
	}
	UpdateGpvalVariables(0);
	set_iterator = cleanup_iteration(set_iterator);
}
//
// process 'set angles' command 
//
//static void set_angles()
void GnuPlot::SetAngles()
{
	Pgm.Shift();
	if(Pgm.EndOfCommand()) {
		// assuming same as defaults */
		ang2rad = 1;
	}
	else if(Pgm.AlmostEqualsCur("r$adians")) {
		Pgm.Shift();
		ang2rad = 1;
	}
	else if(Pgm.AlmostEqualsCur("d$egrees")) {
		Pgm.Shift();
		ang2rad = DEG2RAD;
	}
	else
		IntErrorCurToken("expecting 'radians' or 'degrees'");
	if(polar && AxS[T_AXIS].set_autoscale) {
		// set trange if in polar mode and no explicit range */
		AxS[T_AXIS].set_min = 0;
		AxS[T_AXIS].set_max = 2 * M_PI / ang2rad;
	}
}
// 
// process a 'set arrow' command 
// set arrow {tag} {from x,y} {to x,y} {{no}head} ...
// allow any order of options - pm 25.11.2001 
// 
//static void set_arrow()
void GnuPlot::SetArrow()
{
	arrow_def * p_this_arrow = NULL;
	arrow_def * p_new_arrow = NULL;
	arrow_def * p_prev_arrow = NULL;
	bool duplication = FALSE;
	bool set_start = FALSE;
	bool set_end = FALSE;
	int save_token;
	int tag;
	Pgm.Shift();
	// get tag 
	if(Pgm.AlmostEqualsCur("back$head") || Pgm.EqualsCur("front")
	    || Pgm.EqualsCur("from") || Pgm.EqualsCur("at")
	    || Pgm.EqualsCur("to") || Pgm.EqualsCur("rto")
	    || Pgm.EqualsCur("size")
	    || Pgm.EqualsCur("filled") || Pgm.EqualsCur("empty")
	    || Pgm.EqualsCur("as") || Pgm.EqualsCur("arrowstyle")
	    || Pgm.AlmostEqualsCur("head$s") || Pgm.EqualsCur("nohead")
	    || Pgm.AlmostEqualsCur("nobo$rder")) {
		tag = assign_arrow_tag();
	}
	else
		tag = IntExpression();
	if(tag <= 0)
		IntErrorCurToken("tag must be > 0");
	// OK! add arrow 
	if(first_arrow) { // skip to last arrow 
		for(p_this_arrow = first_arrow; p_this_arrow; p_prev_arrow = p_this_arrow, p_this_arrow = p_this_arrow->next)
			// is pThis the arrow we want? 
			if(tag <= p_this_arrow->tag)
				break;
	}
	if(!p_this_arrow || tag != p_this_arrow->tag) {
		p_new_arrow = (arrow_def *)gp_alloc(sizeof(arrow_def), "arrow");
		if(p_prev_arrow == NULL)
			first_arrow = p_new_arrow;
		else
			p_prev_arrow->next = p_new_arrow;
		p_new_arrow->tag = tag;
		p_new_arrow->next = p_this_arrow;
		p_this_arrow = p_new_arrow;
		p_this_arrow->start = default_position;
		p_this_arrow->end = default_position;
		p_this_arrow->angle = 0.0;
		p_this_arrow->type = arrow_end_undefined;
		default_arrow_style(&p_new_arrow->arrow_properties);
	}
	while(!Pgm.EndOfCommand()) {
		// get start position 
		if(Pgm.EqualsCur("from") || Pgm.EqualsCur("at")) {
			if(set_start) {
				duplication = TRUE; break;
			}
			Pgm.Shift();
			if(Pgm.EndOfCommand())
				IntErrorCurToken("start coordinates expected");
			// get coordinates 
			GetPosition(&p_this_arrow->start);
			set_start = TRUE;
			continue;
		}
		// get end or relative end position 
		if(Pgm.EqualsCur("to") || Pgm.EqualsCur("rto")) {
			if(set_end) {
				duplication = TRUE; 
				break;
			}
			if(Pgm.EqualsCur("rto"))
				p_this_arrow->type = arrow_end_relative;
			else
				p_this_arrow->type = arrow_end_absolute;
			Pgm.Shift();
			if(Pgm.EndOfCommand())
				IntErrorCurToken("end coordinates expected");
			// get coordinates 
			GetPosition(&p_this_arrow->end);
			set_end = TRUE;
			continue;
		}
		// get end position specified as length + orientation angle 
		if(Pgm.AlmostEqualsCur("len$gth")) {
			if(set_end) {
				duplication = TRUE; 
				break;
			}
			p_this_arrow->type = arrow_end_oriented;
			Pgm.Shift();
			GetPositionDefault(&p_this_arrow->end, first_axes, 1);
			set_end = TRUE;
			continue;
		}
		if(Pgm.AlmostEqualsCur("ang$le")) {
			Pgm.Shift();
			p_this_arrow->angle = RealExpression();
			continue;
		}
		// Allow interspersed style commands 
		save_token = Pgm.GetCurTokenIdx();
		ArrowParse(&p_this_arrow->arrow_properties, TRUE);
		if(save_token != Pgm.GetCurTokenIdx())
			continue;
		if(!Pgm.EndOfCommand())
			IntErrorCurToken("wrong argument in set arrow");
	} /* while (!Pgm.EndOfCommand()) */
	if(duplication)
		IntErrorCurToken("duplicate or contradictory arguments");
}
// 
// assign a new arrow tag
// arrows are kept sorted by tag number, so pThis is easy
// returns the lowest unassigned tag number
//
static int assign_arrow_tag()
{
	arrow_def * this_arrow;
	int last = 0;           /* previous tag value */
	for(this_arrow = first_arrow; this_arrow != NULL; this_arrow = this_arrow->next)
		if(this_arrow->tag == last + 1)
			last++;
		else
			break;

	return (last + 1);
}
//
// helper routine for 'set autoscale' on a single axis 
//
//static bool set_autoscale_axis(GpAxis * pThis)
bool GnuPlot::SetAutoscaleAxis(GpAxis * pThis)
{
	char keyword[16];
	char * name = (char*)&(axis_name((AXIS_INDEX)pThis->index)[0]);
	if(Pgm.EqualsCur(name)) {
		pThis->set_autoscale = AUTOSCALE_BOTH;
		pThis->min_constraint = CONSTRAINT_NONE;
		pThis->max_constraint = CONSTRAINT_NONE;
		Pgm.Shift();
		if(Pgm.AlmostEqualsCur("noext$end")) {
			pThis->set_autoscale |= AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX;
			Pgm.Shift();
		}
		return TRUE;
	}
	sprintf(keyword, "%smi$n", name);
	if(Pgm.AlmostEqualsCur(keyword)) {
		pThis->set_autoscale |= AUTOSCALE_MIN;
		pThis->min_constraint = CONSTRAINT_NONE;
		Pgm.Shift();
		return TRUE;
	}
	sprintf(keyword, "%sma$x", name);
	if(Pgm.AlmostEqualsCur(keyword)) {
		pThis->set_autoscale |= AUTOSCALE_MAX;
		pThis->max_constraint = CONSTRAINT_NONE;
		Pgm.Shift();
		return TRUE;
	}
	sprintf(keyword, "%sfix", name);
	if(Pgm.EqualsCur(keyword)) {
		pThis->set_autoscale |= AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX;
		Pgm.Shift();
		return TRUE;
	}
	sprintf(keyword, "%sfixmi$n", name);
	if(Pgm.AlmostEqualsCur(keyword)) {
		pThis->set_autoscale |= AUTOSCALE_FIXMIN;
		Pgm.Shift();
		return TRUE;
	}
	sprintf(keyword, "%sfixma$x", name);
	if(Pgm.AlmostEqualsCur(keyword)) {
		pThis->set_autoscale |= AUTOSCALE_FIXMAX;
		Pgm.Shift();
		return TRUE;
	}
	return FALSE;
}
//
// process 'set autoscale' command 
//
//static void set_autoscale()
void GnuPlot::SetAutoscale()
{
	int axis;
	Pgm.Shift();
	if(Pgm.EndOfCommand()) {
		for(axis = 0; axis<AXIS_ARRAY_SIZE; axis++)
			AxS[axis].set_autoscale = AUTOSCALE_BOTH;
		for(axis = 0; axis < AxS.GetParallelAxisCount(); axis++)
			AxS.Parallel(axis).set_autoscale = AUTOSCALE_BOTH;
		return;
	}
	else if(Pgm.EqualsCur("xy") || Pgm.EqualsCur("yx")) {
		AxS[FIRST_X_AXIS].set_autoscale = AxS[FIRST_Y_AXIS].set_autoscale =  AUTOSCALE_BOTH;
		AxS[FIRST_X_AXIS].min_constraint = AxS[FIRST_X_AXIS].max_constraint = AxS[FIRST_Y_AXIS].min_constraint = AxS[FIRST_Y_AXIS].max_constraint = CONSTRAINT_NONE;
		Pgm.Shift();
		return;
	}
	else if(Pgm.EqualsCur("paxis")) {
		Pgm.Shift();
		if(Pgm.EndOfCommand()) {
			for(axis = 0; axis < AxS.GetParallelAxisCount(); axis++)
				AxS.Parallel(axis).set_autoscale = AUTOSCALE_BOTH;
			return;
		}
		axis = IntExpression() - 1;
		if(0 <= axis && axis < AxS.GetParallelAxisCount()) {
			AxS.Parallel(axis).set_autoscale = AUTOSCALE_BOTH;
			return;
		}
		/* no return */
	}
	else if(Pgm.EqualsCur("fix") || Pgm.AlmostEqualsCur("noext$end")) {
		for(axis = 0; axis<AXIS_ARRAY_SIZE; axis++)
			AxS[axis].set_autoscale |= AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX;
		for(axis = 0; axis < AxS.GetParallelAxisCount(); axis++)
			AxS.Parallel(axis).set_autoscale |= AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX;
		Pgm.Shift();
		return;
	}
	else if(Pgm.AlmostEqualsCur("ke$epfix")) {
		for(axis = 0; axis<AXIS_ARRAY_SIZE; axis++)
			AxS[axis].set_autoscale |= AUTOSCALE_BOTH;
		for(axis = 0; axis < AxS.GetParallelAxisCount(); axis++)
			AxS.Parallel(axis).set_autoscale |= AUTOSCALE_BOTH;
		Pgm.Shift();
		return;
	}
	if(SetAutoscaleAxis(&AxS[FIRST_X_AXIS])) return;
	if(SetAutoscaleAxis(&AxS[FIRST_Y_AXIS])) return;
	if(SetAutoscaleAxis(&AxS[FIRST_Z_AXIS])) return;
	if(SetAutoscaleAxis(&AxS[SECOND_X_AXIS])) return;
	if(SetAutoscaleAxis(&AxS[SECOND_Y_AXIS])) return;
	if(SetAutoscaleAxis(&AxS[COLOR_AXIS])) return;
	if(SetAutoscaleAxis(&AxS[POLAR_AXIS])) return;
	/* FIXME: Do these commands make any sense? */
	if(SetAutoscaleAxis(&AxS[T_AXIS])) return;
	if(SetAutoscaleAxis(&AxS[U_AXIS])) return;
	if(SetAutoscaleAxis(&AxS[V_AXIS])) return;
	/* come here only if nothing found: */
	IntErrorCurToken("Invalid axis");
}
//
// process 'set bars' command 
//
//static void set_bars()
void GnuPlot::SetBars()
{
	int save_token;
	Pgm.Shift();
	if(Pgm.EndOfCommand())
		reset_bars();
	while(!Pgm.EndOfCommand()) {
		if(Pgm.EqualsCur("default")) {
			reset_bars();
			Pgm.Shift();
			return;
		}
		/* Jul 2015 - allow a separate line type for error bars */
		save_token = Pgm.GetCurTokenIdx();
		LpParse(&bar_lp, LP_ADHOC, FALSE);
		if(Pgm.GetCurTokenIdx() != save_token) {
			bar_lp.flags = LP_ERRORBAR_SET;
			continue;
		}
		if(Pgm.AlmostEqualsCur("s$mall")) {
			bar_size = 0.0;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("l$arge")) {
			bar_size = 1.0;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("full$width")) {
			bar_size = -1.0;
			Pgm.Shift();
		}
		else if(Pgm.EqualsCur("front")) {
			bar_layer = LAYER_FRONT;
			Pgm.Shift();
		}
		else if(Pgm.EqualsCur("back")) {
			bar_layer = LAYER_BACK;
			Pgm.Shift();
		}
		else
			bar_size = RealExpression();
	}
}

/* process 'set border' command */
static void set_border()
{
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand()) {
		draw_border = 31;
		border_layer = LAYER_FRONT;
		border_lp = default_border_lp;
	}
	while(!GPO.Pgm.EndOfCommand()) {
		if(GPO.Pgm.EqualsCur("front")) {
			border_layer = LAYER_FRONT;
			GPO.Pgm.Shift();
		}
		else if(GPO.Pgm.EqualsCur("back")) {
			border_layer = LAYER_BACK;
			GPO.Pgm.Shift();
		}
		else if(GPO.Pgm.EqualsCur("behind")) {
			border_layer = LAYER_BEHIND;
			GPO.Pgm.Shift();
		}
		else if(GPO.Pgm.EqualsCur("polar")) {
			draw_border |= 0x1000;
			GPO.Pgm.Shift();
		}
		else {
			int save_token = GPO.Pgm.GetCurTokenIdx();
			GPO.LpParse(&border_lp, LP_ADHOC, FALSE);
			if(save_token != GPO.Pgm.GetCurTokenIdx())
				continue;
			draw_border = GPO.IntExpression();
		}
	}
	/* This is the only place the user can change the border	*/
	/* so remember what he set.  If draw_border is later changed*/
	/* internally, we can still recover the user's preference.	*/
	user_border = draw_border;
}
//
// process 'set style boxplot' command 
//
//static void set_boxplot()
void GnuPlot::SetBoxPlot()
{
	Pgm.Shift();
	if(Pgm.EndOfCommand()) {
		boxplot_style defstyle = DEFAULT_BOXPLOT_STYLE;
		boxplot_opts = defstyle;
	}
	while(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("noout$liers")) {
			boxplot_opts.outliers = FALSE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("out$liers")) {
			boxplot_opts.outliers = TRUE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("point$type") || Pgm.EqualsCur("pt")) {
			Pgm.Shift();
			boxplot_opts.pointtype = IntExpression()-1;
		}
		else if(Pgm.EqualsCur("range")) {
			Pgm.Shift();
			boxplot_opts.limit_type = 0;
			boxplot_opts.limit_value = RealExpression();
		}
		else if(Pgm.AlmostEqualsCur("frac$tion")) {
			Pgm.Shift();
			boxplot_opts.limit_value = RealExpression();
			if(boxplot_opts.limit_value < 0 || boxplot_opts.limit_value > 1)
				IntError(Pgm.GetPrevTokenIdx(), "fraction must be less than 1");
			boxplot_opts.limit_type = 1;
		}
		else if(Pgm.AlmostEqualsCur("candle$sticks")) {
			Pgm.Shift();
			boxplot_opts.plotstyle = CANDLESTICKS;
		}
		else if(Pgm.AlmostEqualsCur("finance$bars")) {
			Pgm.Shift();
			boxplot_opts.plotstyle = FINANCEBARS;
		}
		else if(Pgm.AlmostEqualsCur("sep$aration")) {
			Pgm.Shift();
			boxplot_opts.separation = RealExpression();
			if(boxplot_opts.separation < 0)
				IntError(Pgm.GetPrevTokenIdx(), "separation must be > 0");
		}
		else if(Pgm.AlmostEqualsCur("lab$els")) {
			Pgm.Shift();
			if(Pgm.EqualsCur("off")) {
				boxplot_opts.labels = BOXPLOT_FACTOR_LABELS_OFF;
			}
			else if(Pgm.EqualsCur("x")) {
				boxplot_opts.labels = BOXPLOT_FACTOR_LABELS_X;
			}
			else if(Pgm.EqualsCur("x2")) {
				boxplot_opts.labels = BOXPLOT_FACTOR_LABELS_X2;
			}
			else if(Pgm.EqualsCur("auto")) {
				boxplot_opts.labels = BOXPLOT_FACTOR_LABELS_AUTO;
			}
			else
				IntError(Pgm.GetPrevTokenIdx(), "expecting 'x', 'x2', 'auto' or 'off'");
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("median$linewidth")) {
			Pgm.Shift();
			boxplot_opts.median_linewidth = RealExpression();
		}
		else if(Pgm.AlmostEqualsCur("so$rted")) {
			boxplot_opts.sort_factors = TRUE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("un$sorted")) {
			boxplot_opts.sort_factors = FALSE;
			Pgm.Shift();
		}
		else
			IntErrorCurToken("unrecognized option");
	}
}
//
// process 'set boxdepth' command (used by splot with boxes) 
//
static void set_boxdepth()
{
	GPO.Pgm.Shift();
	boxdepth = 0.0;
	if(GPO.Pgm.EqualsCur("square")) {
		GPO.Pgm.Shift();
		boxdepth = -1;
	}
	else if(!GPO.Pgm.EndOfCommand())
		boxdepth = GPO.RealExpression();
}
//
// process 'set boxwidth' command 
//
static void set_boxwidth()
{
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand()) {
		GPO.V.BoxWidth = -1.0;
		GPO.V.BoxWidthIsAbsolute = true;
	}
	else {
		GPO.V.BoxWidth = GPO.RealExpression();
	}
	if(GPO.Pgm.EndOfCommand())
		return;
	else {
		if(GPO.Pgm.AlmostEqualsCur("a$bsolute"))
			GPO.V.BoxWidthIsAbsolute = true;
		else if(GPO.Pgm.AlmostEqualsCur("r$elative"))
			GPO.V.BoxWidthIsAbsolute = false;
		else
			GPO.IntErrorCurToken("expecting 'absolute' or 'relative' ");
	}
	GPO.Pgm.Shift();
}
//
// process 'set clip' command 
//
static void set_clip()
{
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand()) {
		clip_points = TRUE; // assuming same as points 
	}
	else if(GPO.Pgm.AlmostEqualsCur("r$adial") || GPO.Pgm.EqualsCur("polar")) {
		clip_radial = TRUE;
		GPO.Pgm.Shift();
	}
	else if(GPO.Pgm.AlmostEqualsCur("p$oints")) {
		clip_points = TRUE;
		GPO.Pgm.Shift();
	}
	else if(GPO.Pgm.AlmostEqualsCur("o$ne")) {
		clip_lines1 = TRUE;
		GPO.Pgm.Shift();
	}
	else if(GPO.Pgm.AlmostEqualsCur("t$wo")) {
		clip_lines2 = TRUE;
		GPO.Pgm.Shift();
	}
	else
		GPO.IntErrorCurToken("expecting 'points', 'one', or 'two'");
}
//
// process 'set cntrparam' command 
//
//static void set_cntrparam()
void GnuPlot::SetCntrParam()
{
	Pgm.Shift();
	if(Pgm.EndOfCommand()) {
		// assuming same as defaults 
		contour_pts = DEFAULT_NUM_APPROX_PTS;
		contour_kind = CONTOUR_KIND_LINEAR;
		contour_order = DEFAULT_CONTOUR_ORDER;
		contour_levels = DEFAULT_CONTOUR_LEVELS;
		contour_levels_kind = LEVELS_AUTO;
		contour_firstlinetype = 0;
		return;
	}
	while(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("p$oints")) {
			Pgm.Shift();
			contour_pts = IntExpression();
		}
		else if(Pgm.AlmostEqualsCur("first$linetype")) {
			Pgm.Shift();
			contour_firstlinetype = IntExpression();
		}
		else if(Pgm.AlmostEqualsCur("sort$ed")) {
			Pgm.Shift();
			contour_sortlevels = TRUE;
		}
		else if(Pgm.AlmostEqualsCur("unsort$ed")) {
			Pgm.Shift();
			contour_sortlevels = FALSE;
		}
		else if(Pgm.AlmostEqualsCur("li$near")) {
			Pgm.Shift();
			contour_kind = CONTOUR_KIND_LINEAR;
		}
		else if(Pgm.AlmostEqualsCur("c$ubicspline")) {
			Pgm.Shift();
			contour_kind = CONTOUR_KIND_CUBIC_SPL;
		}
		else if(Pgm.AlmostEqualsCur("b$spline")) {
			Pgm.Shift();
			contour_kind = CONTOUR_KIND_BSPLINE;
		}
		else if(Pgm.AlmostEqualsCur("le$vels")) {
			Pgm.Shift();
			if(!(set_iterator && set_iterator->iteration)) {
				free_dynarray(&dyn_contour_levels_list);
				init_dynarray(&dyn_contour_levels_list, sizeof(double), 5, 10);
			}
			/*  RKC: I have modified the next two:
			 *   to use commas to separate list elements as in xtics
			 *   so that incremental lists start,incr[,end]as in "
			 */
			if(Pgm.AlmostEqualsCur("di$screte")) {
				contour_levels_kind = LEVELS_DISCRETE;
				Pgm.Shift();
				if(Pgm.EndOfCommand())
					IntErrorCurToken("expecting discrete level");
				else
					*(double*)nextfrom_dynarray(&dyn_contour_levels_list) = RealExpression();
				while(!Pgm.EndOfCommand()) {
					if(!Pgm.EqualsCur(","))
						IntErrorCurToken("expecting comma to separate discrete levels");
					Pgm.Shift();
					*(double*)nextfrom_dynarray(&dyn_contour_levels_list) = RealExpression();
				}
				contour_levels = dyn_contour_levels_list.end;
			}
			else if(Pgm.AlmostEqualsCur("in$cremental")) {
				int i = 0; /* local counter */
				contour_levels_kind = LEVELS_INCREMENTAL;
				Pgm.Shift();
				contour_levels_list[i++] = RealExpression();
				if(!Pgm.EqualsCur(","))
					IntErrorCurToken("expecting comma to separate start,incr levels");
				Pgm.Shift();
				if((contour_levels_list[i++] = RealExpression()) == 0)
					IntErrorCurToken("increment cannot be 0");
				if(!Pgm.EndOfCommand()) {
					if(!Pgm.EqualsCur(","))
						IntErrorCurToken("expecting comma to separate incr,stop levels");
					Pgm.Shift();
					// need to round up, since 10,10,50 is 5 levels, not four, but 10,10,49 is four
					dyn_contour_levels_list.end = i;
					contour_levels = (int)( (RealExpression()-contour_levels_list[0])/contour_levels_list[1] + 1.0);
				}
			}
			else if(Pgm.AlmostEqualsCur("au$to")) {
				contour_levels_kind = LEVELS_AUTO;
				Pgm.Shift();
				if(!Pgm.EndOfCommand())
					contour_levels = IntExpression();
			}
			else {
				if(contour_levels_kind == LEVELS_DISCRETE)
					IntErrorCurToken("Levels type is discrete, ignoring new number of contour levels");
				contour_levels = IntExpression();
			}
		}
		else if(Pgm.AlmostEqualsCur("o$rder")) {
			Pgm.Shift();
			int order = IntExpression();
			if(order < 2 || order > MAX_BSPLINE_ORDER)
				IntErrorCurToken("bspline order must be in [2..10] range.");
			contour_order = order;
		}
		else
			IntErrorCurToken("expecting 'linear', 'cubicspline', 'bspline', 'points', 'levels' or 'order'");
	}
}
//
// process 'set cntrlabel' command 
//
//static void set_cntrlabel()
void GnuPlot::SetCntrLabel()
{
	Pgm.Shift();
	if(Pgm.EndOfCommand()) {
		strcpy(contour_format, "%8.3g");
		clabel_onecolor = FALSE;
		return;
	}
	while(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("form$at")) {
			char * p_new;
			Pgm.Shift();
			if((p_new = TryToGetString()))
				safe_strncpy(contour_format, p_new, sizeof(contour_format));
			SAlloc::F(p_new);
		}
		else if(Pgm.EqualsCur("font")) {
			char * ctmp;
			Pgm.Shift();
			if((ctmp = TryToGetString())) {
				SAlloc::F(clabel_font);
				clabel_font = ctmp;
			}
		}
		else if(Pgm.AlmostEqualsCur("one$color")) {
			Pgm.Shift();
			clabel_onecolor = TRUE;
		}
		else if(Pgm.EqualsCur("start")) {
			Pgm.Shift();
			clabel_start = IntExpression();
			if(clabel_start <= 0)
				clabel_start = 5;
		}
		else if(Pgm.AlmostEqualsCur("int$erval")) {
			Pgm.Shift();
			clabel_interval = IntExpression();
		}
		else
			IntErrorCurToken("unrecognized option");
	}
}
//
// process 'set contour' command 
//
//static void set_contour()
void GnuPlot::SetContour()
{
	Pgm.Shift();
	if(Pgm.EndOfCommand())
		draw_contour = CONTOUR_BASE; // assuming same as points 
	else {
		if(Pgm.AlmostEqualsCur("ba$se"))
			draw_contour = CONTOUR_BASE;
		else if(Pgm.AlmostEqualsCur("s$urface"))
			draw_contour = CONTOUR_SRF;
		else if(Pgm.AlmostEqualsCur("bo$th"))
			draw_contour = CONTOUR_BOTH;
		else
			IntErrorCurToken("expecting 'base', 'surface', or 'both'");
		Pgm.Shift();
	}
}
//
// process 'set colorsequence command 
//
//void set_colorsequence(int option)
void GnuPlot::SetColorSequence(int option)
{
	ulong default_colors[] = DEFAULT_COLOR_SEQUENCE;
	ulong podo_colors[] = PODO_COLOR_SEQUENCE;
	if(option == 0) { /* Read option from command line */
		Pgm.Shift();
		if(Pgm.EqualsCur("default"))
			option = 1;
		else if(Pgm.EqualsCur("podo"))
			option = 2;
		else if(Pgm.EqualsCur("classic"))
			option = 3;
		else
			IntErrorCurToken("unrecognized color set");
	}
	if(option == 1 || option == 2) {
		int i;
		char * command;
		char * command_template = "set linetype %2d lc rgb 0x%06x";
		ulong * colors = default_colors;
		if(option == 2)
			colors = podo_colors;
		linetype_recycle_count = 8;
		for(i = 1; i <= 8; i++) {
			command = (char *)gp_alloc(strlen(command_template)+8, "dynamic command");
			sprintf(command, command_template, i, colors[i-1]);
			DoStringAndFree(command);
		}
	}
	else if(option == 3) {
		for(linestyle_def * p_this = first_perm_linestyle; p_this; p_this = p_this->next) {
			p_this->lp_properties.pm3d_color.type = TC_LT;
			p_this->lp_properties.pm3d_color.lt = p_this->tag-1;
		}
		linetype_recycle_count = 0;
	}
	else {
		IntErrorCurToken("Expecting 'classic' or 'default'");
	}
	Pgm.Shift();
}

static void set_cornerpoles()
{
	GPO.Pgm.Shift();
	cornerpoles = TRUE;
}
//
// process 'set dashtype' command 
//
//static void set_dashtype()
void GnuPlot::SetDashType()
{
	custom_dashtype_def * this_dashtype = NULL;
	custom_dashtype_def * new_dashtype = NULL;
	custom_dashtype_def * prev_dashtype = NULL;
	int tag, is_new = FALSE;
	Pgm.Shift();
	// get tag 
	if(Pgm.EndOfCommand() || ((tag = IntExpression()) <= 0))
		IntErrorCurToken("tag must be > zero");
	// Check if dashtype is already defined 
	for(this_dashtype = first_custom_dashtype; this_dashtype != NULL; prev_dashtype = this_dashtype, this_dashtype = this_dashtype->next)
		if(tag <= this_dashtype->tag)
			break;
	if(this_dashtype == NULL || tag != this_dashtype->tag) {
		t_dashtype loc_dt = DEFAULT_DASHPATTERN;
		new_dashtype = (struct custom_dashtype_def *)gp_alloc(sizeof(struct custom_dashtype_def), "dashtype");
		if(prev_dashtype)
			prev_dashtype->next = new_dashtype; /* add it to end of list */
		else
			first_custom_dashtype = new_dashtype; /* make it start of list */
		new_dashtype->tag = tag;
		new_dashtype->d_type = DASHTYPE_SOLID;
		new_dashtype->next = this_dashtype;
		new_dashtype->dashtype = loc_dt;
		this_dashtype = new_dashtype;
		is_new = TRUE;
	}
	if(Pgm.AlmostEqualsCur("def$ault")) {
		delete_dashtype(prev_dashtype, this_dashtype);
		is_new = FALSE;
		Pgm.Shift();
	}
	else {
		// FIXME: Maybe pThis should reject return values > 0 because 
		// otherwise we have potentially recursive definitions.      
		this_dashtype->d_type = ParseDashType(&this_dashtype->dashtype);
	}
	if(!Pgm.EndOfCommand()) {
		if(is_new)
			delete_dashtype(prev_dashtype, this_dashtype);
		IntErrorCurToken("Extraneous arguments to set dashtype");
	}
}
/*
 * Delete dashtype from linked list.
 */
void delete_dashtype(struct custom_dashtype_def * prev, struct custom_dashtype_def * pThis)
{
	if(pThis != NULL) {      /* there really is something to delete */
		if(pThis == first_custom_dashtype)
			first_custom_dashtype = pThis->next;
		else
			prev->next = pThis->next;
		SAlloc::F(pThis);
	}
}

/* process 'set dgrid3d' command */
static void set_dgrid3d()
{
	int token_cnt = 0; /* Number of comma-separated values read in */
	int gridx     = dgrid3d_row_fineness;
	int gridy     = dgrid3d_col_fineness;
	int normval   = dgrid3d_norm_value;
	double scalex = dgrid3d_x_scale;
	double scaley = dgrid3d_y_scale;
	// dgrid3d has two different syntax alternatives: classic and new. If there is a "mode" keyword, the syntax is new, otherwise it is classic.
	dgrid3d_mode  = DGRID3D_DEFAULT;
	dgrid3d_kdensity = FALSE;
	GPO.Pgm.Shift();
	while(!(GPO.Pgm.EndOfCommand()) ) {
		int tmp_mode = GPO.Pgm.LookupTableForCurrentToken(&dgrid3d_mode_tbl[0]);
		if(tmp_mode != DGRID3D_OTHER) {
			dgrid3d_mode = tmp_mode;
			GPO.Pgm.Shift();
		}
		switch(tmp_mode) {
			case DGRID3D_QNORM:
			    if(!(GPO.Pgm.EndOfCommand())) normval = GPO.IntExpression();
			    break;
			case DGRID3D_SPLINES:
			    break;
			case DGRID3D_GAUSS:
			case DGRID3D_CAUCHY:
			case DGRID3D_EXP:
			case DGRID3D_BOX:
			case DGRID3D_HANN:
			    if(!(GPO.Pgm.EndOfCommand()) && GPO.Pgm.AlmostEqualsCur("kdens$ity2d")) {
				    dgrid3d_kdensity = TRUE;
				    GPO.Pgm.Shift();
			    }
			    if(!(GPO.Pgm.EndOfCommand())) {
				    scalex = GPO.RealExpression();
				    scaley = scalex;
				    if(GPO.Pgm.EqualsCur(",")) {
					    GPO.Pgm.Shift();
					    scaley = GPO.RealExpression();
				    }
			    }
			    break;

			default: /* {rows}{,cols{,norm}}} */

			    if(GPO.Pgm.EqualsCur(",")) {
				    GPO.Pgm.Shift();
				    token_cnt++;
			    }
			    else if(token_cnt == 0) {
				    gridx = GPO.IntExpression();
				    gridy = gridx; /* gridy defaults to gridx, unless overridden below */
			    }
			    else if(token_cnt == 1) {
				    gridy = GPO.IntExpression();
			    }
			    else if(token_cnt == 2) {
				    normval = GPO.IntExpression();
			    }
			    else
				    GPO.IntErrorCurToken("Unrecognized keyword or unexpected value");
			    break;
		}
	}
	// we could warn here about floating point values being truncated... 
	if(gridx < 2 || gridx > 1000 || gridy < 2 || gridy > 1000)
		GPO.IntError(NO_CARET, "Number of grid points must be in [2:1000] - not changed!");
	// no mode token found: classic format 
	if(dgrid3d_mode == DGRID3D_DEFAULT)
		dgrid3d_mode = DGRID3D_QNORM;
	if(scalex < 0.0 || scaley < 0.0)
		GPO.IntError(NO_CARET, "Scale factors must be greater than zero - not changed!");
	dgrid3d_row_fineness = gridx;
	dgrid3d_col_fineness = gridy;
	dgrid3d_norm_value = normval;
	dgrid3d_x_scale = scalex;
	dgrid3d_y_scale = scaley;
	dgrid3d = TRUE;
}

/* process 'set decimalsign' command */
static void set_decimalsign()
{
	GPO.Pgm.Shift();
	/* Clear current setting */
	ZFREE(decimalsign);
	if(GPO.Pgm.EndOfCommand()) {
		reset_numeric_locale();
		ZFREE(numeric_locale);
#ifdef HAVE_LOCALE_H
	}
	else if(GPO.Pgm.EqualsCur("locale")) {
		GPO.Pgm.Shift();
		char * newlocale = GPO.TryToGetString();
		SETIFZ(newlocale, gp_strdup(setlocale(LC_NUMERIC, "")));
		SETIFZ(newlocale, gp_strdup(getenv("LC_ALL")));
		SETIFZ(newlocale, gp_strdup(getenv("LC_NUMERIC")));
		SETIFZ(newlocale, gp_strdup(getenv("LANG")));
		if(!setlocale(LC_NUMERIC, newlocale ? newlocale : ""))
			GPO.IntError(GPO.Pgm.GetPrevTokenIdx(), "Could not find requested locale");
		decimalsign = gp_strdup(get_decimal_locale());
		fprintf(stderr, "decimal_sign in locale is %s\n", decimalsign);
		/* Save pThis locale for later use, but return to "C" for now */
		SAlloc::F(numeric_locale);
		numeric_locale = newlocale;
		setlocale(LC_NUMERIC, "C");
#endif
	}
	else if(!(decimalsign = GPO.TryToGetString()))
		GPO.IntErrorCurToken("expecting string");
}
//
// process 'set dummy' command 
//
static void set_dummy()
{
	GPO.Pgm.Shift();
	for(int i = 0; i<MAX_NUM_VAR; i++) {
		if(GPO.Pgm.EndOfCommand())
			return;
		if(isalpha((uchar)gp_input_line[GPO.Pgm.GetCurTokenStartIndex()])) {
			GPO.Pgm.CopyStr(set_dummy_var[i], GPO.Pgm.GetCurTokenIdx(), MAX_ID_LEN);
			GPO.Pgm.Shift();
		}
		if(GPO.Pgm.EqualsCur(","))
			GPO.Pgm.Shift();
		else
			break;
	}
	if(!GPO.Pgm.EndOfCommand())
		GPO.IntErrorCurToken("unrecognized syntax");
}

/* process 'set encoding' command */
static void set_encoding()
{
	char * l = NULL;
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand()) {
		encoding = S_ENC_DEFAULT;
#ifdef HAVE_LOCALE_H
	}
	else if(GPO.Pgm.EqualsCur("locale")) {
		enum set_encoding_id newenc = encoding_from_locale();

		l = setlocale(LC_CTYPE, "");
		if(newenc == S_ENC_DEFAULT)
			GPO.IntWarn(NO_CARET, "Locale not supported by gnuplot: %s", l);
		if(newenc == S_ENC_INVALID)
			GPO.IntWarn(NO_CARET, "Error converting locale \"%s\" to codepage number", l);
		else
			encoding = newenc;
		GPO.Pgm.Shift();
#endif
	}
	else {
		int temp = GPO.Pgm.LookupTableForCurrentToken(&set_encoding_tbl[0]);
		char * senc;
		// allow string variables as parameter 
		if((temp == S_ENC_INVALID) && GPO.Pgm.IsStringValue(GPO.Pgm.GetCurTokenIdx()) && (senc = GPO.TryToGetString())) {
			for(int i = 0; encoding_names[i] != NULL; i++)
				if(strcmp(encoding_names[i], senc) == 0)
					temp = i;
			SAlloc::F(senc);
		}
		else {
			GPO.Pgm.Shift();
		}
		if(temp == S_ENC_INVALID)
			GPO.IntErrorCurToken("unrecognized encoding specification; see 'help encoding'.");
		encoding = (set_encoding_id)temp;
	}
	init_special_chars();
}
//
// process 'set fit' command 
//
//static void set_fit()
void GnuPlot::SetFit()
{
	int key;
	Pgm.Shift();
	while(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("log$file")) {
			char * tmp;
			Pgm.Shift();
			fit_suppress_log = FALSE;
			if(Pgm.EndOfCommand()) {
				ZFREE(fitlogfile);
			}
			else if(Pgm.EqualsCur("default")) {
				Pgm.Shift();
				ZFREE(fitlogfile);
			}
			else if((tmp = TryToGetString()) != NULL) {
				SAlloc::F(fitlogfile);
				fitlogfile = tmp;
			}
			else {
				IntErrorCurToken("expecting string");
			}
		}
		else if(Pgm.AlmostEqualsCur("nolog$file")) {
			fit_suppress_log = TRUE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("err$orvariables")) {
			fit_errorvariables = TRUE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("noerr$orvariables")) {
			fit_errorvariables = FALSE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("cov$ariancevariables")) {
			fit_covarvariables = TRUE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("nocov$ariancevariables")) {
			fit_covarvariables = FALSE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("errors$caling")) {
			fit_errorscaling = TRUE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("noerrors$caling")) {
			fit_errorscaling = FALSE;
			Pgm.Shift();
		}
		else if((key = Pgm.LookupTableForCurrentToken(fit_verbosity_level)) > 0) {
			fit_verbosity = (verbosity_level)key;
			Pgm.Shift();
		}
		else if(Pgm.EqualsCur("prescale")) {
			fit_prescale = TRUE;
			Pgm.Shift();
		}
		else if(Pgm.EqualsCur("noprescale")) {
			fit_prescale = FALSE;
			Pgm.Shift();
		}
		else if(Pgm.EqualsCur("limit")) {
			// preserve compatibility with FIT_LIMIT user variable 
			double value;
			Pgm.Shift();
			if(Pgm.EqualsCur("default")) {
				Pgm.Shift();
				value = 0.0;
			}
			else
				value = RealExpression();
			if(value > 0.0 && value < 1.0) {
				udvt_entry * v = Ev.AddUdvByName((char*)FITLIMIT);
				Gcomplex(&v->udv_value, value, 0);
			}
			else {
				Ev.DelUdvByName(FITLIMIT, FALSE);
			}
		}
		else if(Pgm.EqualsCur("limit_abs")) {
			Pgm.Shift();
			double value = RealExpression();
			epsilon_abs = (value > 0.) ? value : 0.;
		}
		else if(Pgm.EqualsCur("maxiter")) {
			// preserve compatibility with FIT_MAXITER user variable 
			int maxiter;
			Pgm.Shift();
			if(Pgm.EqualsCur("default")) {
				Pgm.Shift();
				maxiter = 0;
			}
			else
				maxiter = IntExpression();
			if(maxiter > 0) {
				udvt_entry * v = Ev.AddUdvByName(FITMAXITER);
				Ginteger(&v->udv_value, maxiter);
			}
			else {
				Ev.DelUdvByName(FITMAXITER, FALSE);
			}
		}
		else if(Pgm.EqualsCur("start_lambda")) {
			// preserve compatibility with FIT_START_LAMBDA user variable 
			double value;
			Pgm.Shift();
			if(Pgm.EqualsCur("default")) {
				Pgm.Shift();
				value = 0.;
			}
			else
				value = RealExpression();
			if(value > 0.) {
				udvt_entry * v = Ev.AddUdvByName(FITSTARTLAMBDA);
				Gcomplex(&v->udv_value, value, 0);
			}
			else {
				Ev.DelUdvByName(FITSTARTLAMBDA, FALSE);
			}
		}
		else if(Pgm.EqualsCur("lambda_factor")) {
			// preserve compatibility with FIT_LAMBDA_FACTOR user variable 
			double value;
			Pgm.Shift();
			if(Pgm.EqualsCur("default")) {
				Pgm.Shift();
				value = 0.0;
			}
			else
				value = RealExpression();
			if(value > 0.) {
				udvt_entry * v = Ev.AddUdvByName(FITLAMBDAFACTOR);
				Gcomplex(&v->udv_value, value, 0);
			}
			else {
				Ev.DelUdvByName(FITLAMBDAFACTOR, FALSE);
			}
		}
		else if(Pgm.EqualsCur("script")) {
			char * tmp;
			Pgm.Shift();
			if(Pgm.EndOfCommand()) {
				ZFREE(fit_script);
			}
			else if(Pgm.EqualsCur("default")) {
				Pgm.Shift();
				ZFREE(fit_script);
			}
			else if((tmp = TryToGetString())) {
				SAlloc::F(fit_script);
				fit_script = tmp;
			}
			else {
				IntErrorCurToken("expecting string");
			}
		}
		else if(Pgm.EqualsCur("wrap")) {
			Pgm.Shift();
			fit_wrap = IntExpression();
			if(fit_wrap < 0) fit_wrap = 0;
		}
		else if(Pgm.EqualsCur("nowrap")) {
			Pgm.Shift();
			fit_wrap = 0;
		}
		else if(Pgm.EqualsCur("v4")) {
			Pgm.Shift();
			fit_v4compatible = TRUE;
		}
		else if(Pgm.EqualsCur("v5")) {
			Pgm.Shift();
			fit_v4compatible = FALSE;
		}
		else {
			IntErrorCurToken("unrecognized option --- see `help set fit`");
		}
	} /* while (!end) */
}
//
// process 'set format' command 
//
//void set_format()
void GnuPlot::SetFormat()
{
	bool set_for_axis[AXIS_ARRAY_SIZE] = AXIS_ARRAY_INITIALIZER(FALSE);
	/*AXIS_INDEX*/int axis;
	char * format;
	td_type tictype = DT_UNINITIALIZED;
	Pgm.Shift();
	if((axis = (AXIS_INDEX)Pgm.LookupTableForCurrentToken(axisname_tbl)) >= 0) {
		set_for_axis[axis] = TRUE;
		Pgm.Shift();
	}
	else if(Pgm.EqualsCur("xy") || Pgm.EqualsCur("yx")) {
		set_for_axis[FIRST_X_AXIS] = set_for_axis[FIRST_Y_AXIS] = TRUE;
		Pgm.Shift();
	}
	else {
		/* Set all of them */
		for(axis = (AXIS_INDEX)0; axis < AXIS_ARRAY_SIZE; axis++)
			set_for_axis[axis] = TRUE;
	}
	if(Pgm.EndOfCommand()) {
		for(axis = FIRST_AXES; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
			if(set_for_axis[axis]) {
				SAlloc::F(AxS[axis].formatstring);
				AxS[axis].formatstring = gp_strdup(DEF_FORMAT);
				AxS[axis].tictype = DT_NORMAL;
			}
		}
		return;
	}
	if(!(format = TryToGetString()))
		IntErrorCurToken("expecting format string");
	if(Pgm.AlmostEqualsCur("time$date")) {
		tictype = DT_TIMEDATE;
		Pgm.Shift();
	}
	else if(Pgm.AlmostEqualsCur("geo$graphic")) {
		tictype = DT_DMS;
		Pgm.Shift();
	}
	else if(Pgm.AlmostEqualsCur("num$eric")) {
		tictype = DT_NORMAL;
		Pgm.Shift();
	}
	for(axis = FIRST_AXES; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
		if(set_for_axis[axis]) {
			SAlloc::F(AxS[axis].formatstring);
			AxS[axis].formatstring = gp_strdup(format);
			if(tictype != DT_UNINITIALIZED)
				AxS[axis].tictype = tictype;
		}
	}
	SAlloc::F(format);
}
//
// helper function for 'set grid' command 
//
//static bool grid_match(AXIS_INDEX axis, const char * pString)
bool GnuPlot::GridMatch(AXIS_INDEX axis, const char * pString)
{
	if(Pgm.AlmostEqualsCur(pString+2)) {
		if(pString[2] == 'm')
			AxS[axis].gridminor = TRUE;
		else
			AxS[axis].gridmajor = TRUE;
		Pgm.Shift();
		return TRUE;
	}
	else if(Pgm.AlmostEqualsCur(pString)) {
		if(pString[2] == 'm')
			AxS[axis].gridminor = FALSE;
		else
			AxS[axis].gridmajor = FALSE;
		Pgm.Shift();
		return TRUE;
	}
	return FALSE;
}
//
// process 'set grid' command 
//
//static void set_grid()
void GnuPlot::SetGrid()
{
	bool explicit_change = FALSE;
	Pgm.Shift();
	while(!Pgm.EndOfCommand()) {
		explicit_change = GridMatch(FIRST_X_AXIS, "nox$tics")
		    || GridMatch(FIRST_Y_AXIS, "noy$tics")
		    || GridMatch(FIRST_Z_AXIS, "noz$tics")
		    || GridMatch(SECOND_X_AXIS, "nox2$tics")
		    || GridMatch(SECOND_Y_AXIS, "noy2$tics")
		    || GridMatch(FIRST_X_AXIS, "nomx$tics")
		    || GridMatch(FIRST_Y_AXIS, "nomy$tics")
		    || GridMatch(FIRST_Z_AXIS, "nomz$tics")
		    || GridMatch(SECOND_X_AXIS, "nomx2$tics")
		    || GridMatch(SECOND_Y_AXIS, "nomy2$tics")
		    || GridMatch(COLOR_AXIS, "nocb$tics")
		    || GridMatch(COLOR_AXIS, "nomcb$tics")
		    || GridMatch(POLAR_AXIS, "nor$tics")
		    || GridMatch(POLAR_AXIS, "nomr$tics");
		if(explicit_change) {
			continue;
		}
		else if(Pgm.AlmostEqualsCur("po$lar")) {
			// Dec 2016 - zero or negative disables radial grid lines 
			AxS[POLAR_AXIS].gridmajor = TRUE; /* Enable both circles and radii */
			polar_grid_angle = 30*DEG2RAD;
			Pgm.Shift();
			if(MightBeNumeric(Pgm.GetCurTokenIdx())) {
				double ang = RealExpression();
				polar_grid_angle = (ang > 2.*M_PI) ? DEG2RAD*ang : ang2rad*ang;
			}
		}
		else if(Pgm.AlmostEqualsCur("nopo$lar")) {
			polar_grid_angle = 0; /* not polar grid */
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("spider$plot")) {
			grid_spiderweb = TRUE;
			Pgm.Shift();
		}
		else if(Pgm.EqualsCur("back")) {
			grid_layer = LAYER_BACK;
			Pgm.Shift();
		}
		else if(Pgm.EqualsCur("front")) {
			grid_layer = LAYER_FRONT;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("vert$ical")) {
			grid_vertical_lines = TRUE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("novert$ical")) {
			grid_vertical_lines = FALSE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("layerd$efault") || Pgm.EqualsCur("behind")) {
			grid_layer = LAYER_BEHIND;
			Pgm.Shift();
		}
		else { /* only remaining possibility is a line type */
			int save_token = Pgm.GetCurTokenIdx();
			LpParse(&grid_lp, LP_ADHOC, FALSE);
			if(Pgm.EqualsCur(",")) {
				Pgm.Shift();
				LpParse(&mgrid_lp, LP_ADHOC, FALSE);
			}
			else if(save_token != Pgm.GetCurTokenIdx())
				mgrid_lp = grid_lp;
			if(save_token == Pgm.GetCurTokenIdx())
				break;
		}
	}
	if(!explicit_change && !some_grid_selected()) {
		// no axis specified, thus select default grid 
		if(polar) {
			AxS[POLAR_AXIS].gridmajor = TRUE;
			polar_grid_angle = 30.*DEG2RAD;
		}
		else if(spiderplot) {
			grid_spiderweb = TRUE;
		}
		else {
			AxS[FIRST_X_AXIS].gridmajor = TRUE;
			AxS[FIRST_Y_AXIS].gridmajor = TRUE;
		}
	}
}
//
// process 'set hidden3d' command 
//
//static void set_hidden3d()
void GnuPlot::SetHidden3D()
{
	Pgm.Shift();
	SetHidden3DOptions();
	hidden3d = TRUE;
	SET_REFRESH_OK(E_REFRESH_NOT_OK, 0);
}

//static void set_history()
void GnuPlot::SetHistory()
{
	Pgm.Shift();
	while(!Pgm.EndOfCommand()) {
		if(Pgm.EqualsCur("quiet")) {
			Pgm.Shift();
			history_quiet = TRUE;
			continue;
		}
		else if(Pgm.AlmostEqualsCur("num$bers")) {
			Pgm.Shift();
			history_quiet = FALSE;
			continue;
		}
		else if(Pgm.EqualsCur("full")) {
			Pgm.Shift();
			history_full = TRUE;
			continue;
		}
		else if(Pgm.EqualsCur("trim")) {
			Pgm.Shift();
			history_full = FALSE;
			continue;
		}
		else if(Pgm.AlmostEqualsCur("def$ault")) {
			Pgm.Shift();
			history_quiet = FALSE;
			history_full = TRUE;
			gnuplot_history_size = HISTORY_SIZE;
			continue;
		}
		else if(Pgm.EqualsCur("size")) {
			Pgm.Shift();
			/* fall through */
		}
		// Catches both the deprecated "set historysize" and "set history size" 
		gnuplot_history_size = IntExpression();
#ifndef GNUPLOT_HISTORY
		IntWarn(NO_CARET, "This copy of gnuplot was built without support for command history.");
#endif
	}
}
//
// process 'set pixmap' command 
//
//static void set_pixmap()
void GnuPlot::SetPixMap()
{
	t_pixmap * this_pixmap = NULL;
	t_pixmap * new_pixmap = NULL;
	t_pixmap * prev_pixmap = NULL;
	char * temp = NULL;
	bool from_colormap = FALSE;
	int tag;
	Pgm.Shift();
	tag = IntExpression();
	if(tag <= 0)
		IntErrorCurToken("tag must be > 0");
	for(this_pixmap = pixmap_listhead; this_pixmap != NULL;
	    prev_pixmap = this_pixmap, this_pixmap = this_pixmap->next)
		if(tag <= this_pixmap->tag)
			break;
	if(this_pixmap == NULL || tag != this_pixmap->tag) {
		new_pixmap = (t_pixmap *)gp_alloc(sizeof(t_pixmap), "pixmap");
		if(prev_pixmap == NULL)
			pixmap_listhead = new_pixmap;
		else
			prev_pixmap->next = new_pixmap;
		memzero(new_pixmap, sizeof(t_pixmap));
		new_pixmap->tag = tag;
		new_pixmap->next = this_pixmap;
		new_pixmap->layer = LAYER_FRONT;
		this_pixmap = new_pixmap;
	}
	while(!Pgm.EndOfCommand()) {
		if(Pgm.EqualsCur("at")) {
			Pgm.Shift();
			GetPosition(&this_pixmap->pin);
			continue;
		}
		if(Pgm.EqualsCur("size")) {
			Pgm.Shift();
			GetPosition(&this_pixmap->extent);
			continue;
		}
		if(Pgm.EqualsCur("width")) {
			Pgm.Shift();
			GetPosition(&this_pixmap->extent);
			this_pixmap->extent.y = 0;
			continue;
		}
		if(Pgm.EqualsCur("height")) {
			Pgm.Shift();
			GetPosition(&this_pixmap->extent);
			this_pixmap->extent.scaley = this_pixmap->extent.scalex;
			this_pixmap->extent.y = this_pixmap->extent.x;
			this_pixmap->extent.x = 0;
			continue;
		}
		if((temp = TryToGetString())) {
			gp_expand_tilde(&temp);
			SAlloc::F(this_pixmap->filename);
			this_pixmap->filename = temp;
			continue;
		}
		if(Pgm.EqualsCur("colormap")) {
			Pgm.Shift();
			PixMapFromColorMap(this_pixmap);
			from_colormap = TRUE;
			continue;
		}
		if(Pgm.EqualsCur("behind")) {
			Pgm.Shift();
			this_pixmap->layer = LAYER_BEHIND;
			continue;
		}
		if(Pgm.EqualsCur("back")) {
			Pgm.Shift();
			this_pixmap->layer = LAYER_BACK;
			continue;
		}
		if(Pgm.EqualsCur("front")) {
			Pgm.Shift();
			this_pixmap->layer = LAYER_FRONT;
			continue;
		}
		if(Pgm.EqualsCur("center")) {
			Pgm.Shift();
			this_pixmap->center = TRUE;
			continue;
		}
		/* Unrecognized option */
		break;
	}
	if(from_colormap || this_pixmap->colormapname)
		return;
	if(!this_pixmap->filename)
		IntErrorCurToken("must give filename or colormap");
	// Enforce non-negative extents 
	SETMAX(this_pixmap->extent.x, 0.0);
	SETMAX(this_pixmap->extent.y, 0.0);
	df_read_pixmap(this_pixmap); // This will open the file and read in the pixmap pixels 
}
//
// process 'set isosamples' command 
//
//static void set_isosamples()
void GnuPlot::SetIsoSamples()
{
	Pgm.Shift();
	int tsamp1 = IntExpression();
	int tsamp2 = tsamp1;
	if(!Pgm.EndOfCommand()) {
		if(!Pgm.EqualsCur(","))
			IntErrorCurToken("',' expected");
		Pgm.Shift();
		tsamp2 = IntExpression();
	}
	if(tsamp1 < 2 || tsamp2 < 2)
		IntErrorCurToken("sampling rate must be > 1; sampling unchanged");
	else {
		curve_points * f_p = P_FirstPlot;
		surface_points * f_3dp = first_3dplot;
		P_FirstPlot = NULL;
		first_3dplot = NULL;
		GnuPlot::CpFree(f_p);
		sp_free(f_3dp);
		iso_samples_1 = tsamp1;
		iso_samples_2 = tsamp2;
	}
}
/* When plotting an external key, the margin and l/r/t/b/c are
   used to determine one of twelve possible positions.  They must
   be defined appropriately in the case where stack direction
   determines exact position. */
static void set_key_position_from_stack_direction(legend_key * key)
{
	if(key->stack_dir == GPKEY_VERTICAL) {
		switch(key->hpos) {
			case LEFT: key->margin = GPKEY_LMARGIN; break;
			case CENTRE: key->margin = (key->vpos == JUST_TOP) ? GPKEY_TMARGIN : GPKEY_BMARGIN; break;
			case RIGHT: key->margin = GPKEY_RMARGIN; break;
		}
	}
	else {
		switch(key->vpos) {
			case JUST_TOP: key->margin = GPKEY_TMARGIN; break;
			case JUST_CENTRE: key->margin = (key->hpos == LEFT) ? GPKEY_LMARGIN : GPKEY_RMARGIN; break;
			case JUST_BOT: key->margin = GPKEY_BMARGIN; break;
		}
	}
}
//
// process 'set key' command 
//
//static void set_key()
void GnuPlot::SetKey()
{
	bool   vpos_set = FALSE;
	bool   hpos_set = FALSE;
	bool   reg_set = FALSE;
	bool   sdir_set = FALSE;
	char * vpos_warn = "Multiple vertical position settings";
	char * hpos_warn = "Multiple horizontal position settings";
	char * reg_warn = "Multiple location region settings";
	char * sdir_warn = "Multiple stack direction settings";
	legend_key * key = &keyT;
	/* Only for backward compatibility with deprecated "set keytitle foo" */
	if(Pgm.AlmostEqualsCur("keyt$itle"))
		goto S_KEYTITLE;
	Pgm.Shift();
	key->visible = TRUE;
	while(!Pgm.EndOfCommand()) {
		switch(Pgm.LookupTableForCurrentToken(&set_key_tbl[0])) {
			case S_KEY_ON:
			    key->visible = TRUE;
			    break;
			case S_KEY_OFF:
			    key->visible = FALSE;
			    break;
			case S_KEY_DEFAULT:
			    reset_key();
			    break;
			case S_KEY_TOP:
			    if(vpos_set)
				    IntWarnCurToken(vpos_warn);
			    key->vpos = JUST_TOP;
			    vpos_set = TRUE;
			    break;
			case S_KEY_BOTTOM:
			    if(vpos_set)
				    IntWarnCurToken(vpos_warn);
			    key->vpos = JUST_BOT;
			    vpos_set = TRUE;
			    break;
			case S_KEY_LEFT:
			    if(hpos_set)
				    IntWarnCurToken(hpos_warn);
			    key->hpos = LEFT;
			    hpos_set = TRUE;
			    break;
			case S_KEY_RIGHT:
			    if(hpos_set)
				    IntWarnCurToken(hpos_warn);
			    key->hpos = RIGHT;
			    hpos_set = TRUE;
			    break;
			case S_KEY_CENTER:
			    if(!vpos_set) 
					key->vpos = JUST_CENTRE;
			    if(!hpos_set) 
					key->hpos = CENTRE;
			    if(vpos_set || hpos_set)
				    vpos_set = hpos_set = TRUE;
			    break;
			case S_KEY_VERTICAL:
			    if(sdir_set)
				    IntWarnCurToken(sdir_warn);
			    key->stack_dir = GPKEY_VERTICAL;
			    sdir_set = TRUE;
			    break;
			case S_KEY_HORIZONTAL:
			    if(sdir_set)
				    IntWarnCurToken(sdir_warn);
			    key->stack_dir = GPKEY_HORIZONTAL;
			    sdir_set = TRUE;
			    break;
			case S_KEY_OVER:
			    if(reg_set)
				    IntWarnCurToken(reg_warn);
			/* Fall through */
			case S_KEY_ABOVE:
			    if(!hpos_set)
				    key->hpos = CENTRE;
			    if(!sdir_set)
				    key->stack_dir = GPKEY_HORIZONTAL;
			    key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
			    key->margin = GPKEY_TMARGIN;
			    reg_set = TRUE;
			    break;
			case S_KEY_UNDER:
			    if(reg_set)
				    IntWarnCurToken(reg_warn);
			/* Fall through */
			case S_KEY_BELOW:
			    if(!hpos_set)
				    key->hpos = CENTRE;
			    if(!sdir_set)
				    key->stack_dir = GPKEY_HORIZONTAL;
			    key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
			    key->margin = GPKEY_BMARGIN;
			    reg_set = TRUE;
			    break;
			case S_KEY_INSIDE:
			    if(reg_set)
				    IntWarnCurToken(reg_warn);
			    key->region = GPKEY_AUTO_INTERIOR_LRTBC;
			    key->fixed = FALSE;
			    reg_set = TRUE;
			    break;
			case S_KEY_OUTSIDE:
			    if(reg_set)
				    IntWarnCurToken(reg_warn);
			    key->region = GPKEY_AUTO_EXTERIOR_LRTBC;
			    reg_set = TRUE;
			    break;
			case S_KEY_FIXED:
			    if(reg_set)
				    IntWarnCurToken(reg_warn);
			    key->region = GPKEY_AUTO_INTERIOR_LRTBC;
			    key->fixed = TRUE;
			    reg_set = TRUE;
			    break;
			case S_KEY_TMARGIN:
			    if(reg_set)
				    IntWarnCurToken(reg_warn);
			    key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
			    key->margin = GPKEY_TMARGIN;
			    reg_set = TRUE;
			    break;
			case S_KEY_BMARGIN:
			    if(reg_set)
				    IntWarnCurToken(reg_warn);
			    key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
			    key->margin = GPKEY_BMARGIN;
			    reg_set = TRUE;
			    break;
			case S_KEY_LMARGIN:
			    if(reg_set)
				    IntWarnCurToken(reg_warn);
			    key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
			    key->margin = GPKEY_LMARGIN;
			    reg_set = TRUE;
			    break;
			case S_KEY_RMARGIN:
			    if(reg_set)
				    IntWarnCurToken(reg_warn);
			    key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
			    key->margin = GPKEY_RMARGIN;
			    reg_set = TRUE;
			    break;
			case S_KEY_LLEFT: key->just = GPKEY_LEFT; break;
			case S_KEY_RRIGHT: key->just = GPKEY_RIGHT; break;
			case S_KEY_REVERSE: key->reverse = TRUE; break;
			case S_KEY_NOREVERSE: key->reverse = FALSE; break;
			case S_KEY_INVERT: key->invert = TRUE; break;
			case S_KEY_NOINVERT: key->invert = FALSE; break;
			case S_KEY_ENHANCED: key->enhanced = TRUE; break;
			case S_KEY_NOENHANCED: key->enhanced = FALSE; break;
			case S_KEY_BOX:
			    Pgm.Shift();
			    key->box.l_type = LT_BLACK;
			    if(!Pgm.EndOfCommand()) {
				    int old_token = Pgm.GetCurTokenIdx();
				    LpParse(&key->box, LP_ADHOC, FALSE);
				    if(old_token == Pgm.GetCurTokenIdx() && Pgm.IsANumber(Pgm.GetCurTokenIdx())) {
					    key->box.l_type = IntExpression() - 1;
					    Pgm.Shift();
				    }
			    }
			    Pgm.Rollback(); // is incremented after loop 
			    break;
			case S_KEY_NOBOX:
			    key->box.l_type = LT_NODRAW;
			    break;
			case S_KEY_SAMPLEN:
			    Pgm.Shift();
			    key->swidth = GPO.RealExpression();
			    Pgm.Rollback(); // it is incremented after loop 
			    break;
			case S_KEY_SPACING:
			    Pgm.Shift();
			    key->vert_factor = GPO.RealExpression();
			    if(key->vert_factor < 0.0)
				    key->vert_factor = 0.0;
			    Pgm.Rollback(); // it is incremented after loop 
			    break;
			case S_KEY_WIDTH:
			    Pgm.Shift();
			    key->width_fix = GPO.RealExpression();
			    Pgm.Rollback(); // it is incremented after loop 
			    break;
			case S_KEY_HEIGHT:
			    Pgm.Shift();
			    key->height_fix = GPO.RealExpression();
			    Pgm.Rollback(); // it is incremented after loop 
			    break;
			case S_KEY_AUTOTITLES:
				Pgm.Shift();
			    if(Pgm.AlmostEqualsCur("col$umnheader"))
				    key->auto_titles = COLUMNHEAD_KEYTITLES;
			    else {
				    key->auto_titles = FILENAME_KEYTITLES;
				    Pgm.Rollback();
			    }
			    break;
			case S_KEY_NOAUTOTITLES:
			    key->auto_titles = NOAUTO_KEYTITLES;
			    break;
			case S_KEY_TITLE:
S_KEYTITLE:
			    key->title.pos = CENTRE;
			    set_xyzlabel(&key->title);
			    Pgm.Rollback();
			    break;
			case S_KEY_NOTITLE:
			    ZFREE(key->title.text);
			    break;
			case S_KEY_FONT:
			    Pgm.Shift();
			    // Make sure they've specified a font 
			    if(!Pgm.IsStringValue(Pgm.GetCurTokenIdx()))
				    IntErrorCurToken("expected font");
			    else {
				    char * tmp = TryToGetString();
				    if(tmp) {
					    SAlloc::F(key->font);
					    key->font = tmp;
				    }
				    Pgm.Rollback();
			    }
			    break;
			case S_KEY_TEXTCOLOR:
				{
					t_colorspec lcolor = DEFAULT_COLORSPEC;
					ParseColorSpec(&lcolor, TC_VARIABLE);
					// Only for backwards compatibility 
					if(lcolor.type == TC_RGB && lcolor.value == -1.0)
						lcolor.type = TC_VARIABLE;
					key->textcolor = lcolor;
				}
			    Pgm.Rollback();
			    break;
			case S_KEY_MAXCOLS:
			    Pgm.Shift();
			    key->maxcols = (Pgm.EndOfCommand() || Pgm.AlmostEqualsCur("a$utomatic")) ? 0 : IntExpression();
			    if(key->maxcols < 0)
				    key->maxcols = 0;
			    Pgm.Rollback(); // it is incremented after loop 
			    break;
			case S_KEY_MAXROWS:
			    Pgm.Shift();
			    key->maxrows = (Pgm.EndOfCommand() || Pgm.AlmostEqualsCur("a$utomatic")) ? 0 : IntExpression();
			    if(key->maxrows < 0)
				    key->maxrows = 0;
			    Pgm.Rollback(); // it is incremented after loop 
			    break;
			case S_KEY_FRONT:
			    key->front = TRUE;
			    if(Pgm.AlmostEquals(Pgm.GetCurTokenIdx()+1, "fill$color") || Pgm.EqualsNext("fc")) {
				    Pgm.Shift();
				    ParseColorSpec(&key->fillcolor, TC_RGB);
				    Pgm.Rollback();
			    }
			    else
				    key->fillcolor = background_fill;
			    break;
			case S_KEY_NOFRONT:
			    key->front = FALSE;
			    break;
			case S_KEY_MANUAL:
			    Pgm.Shift();
			    if(reg_set)
				    IntWarnCurToken(reg_warn);
			    GetPosition(&key->user_pos);
			    key->region = GPKEY_USER_PLACEMENT;
			    reg_set = TRUE;
			    Pgm.Rollback(); // will be incremented again soon 
			    break;
			case S_KEY_INVALID:
			default:
			    IntErrorCurToken("unknown key option");
			    break;
		}
		Pgm.Shift();
	}
	if(key->region == GPKEY_AUTO_EXTERIOR_LRTBC)
		set_key_position_from_stack_direction(key);
	else if(key->region == GPKEY_AUTO_EXTERIOR_MARGIN) {
		if(vpos_set && (key->margin == GPKEY_TMARGIN || key->margin == GPKEY_BMARGIN))
			IntWarn(NO_CARET, "ignoring top/center/bottom; incompatible with tmargin/bmargin.");
		else if(hpos_set && (key->margin == GPKEY_LMARGIN || key->margin == GPKEY_RMARGIN))
			IntWarn(NO_CARET, "ignoring left/center/right; incompatible with lmargin/tmargin.");
	}
}

/* process 'set label' command */
/* set label {tag} {"label_text"{,<value>{,...}}} {<label options>} */
/* EAM Mar 2003 - option parsing broken out into separate routine */
static void set_label()
{
	text_label * this_label = NULL;
	text_label * new_label = NULL;
	text_label * prev_label = NULL;
	GpValue a;
	int save_token;
	int tag = -1;
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand())
		return;
	// The first item must be either a tag or the label text 
	save_token = GPO.Pgm.GetCurTokenIdx();
	if(GPO.Pgm.IsLetter(GPO.Pgm.GetCurTokenIdx()) && GPO.Pgm.TypeUdv(GPO.Pgm.GetCurTokenIdx()) == 0) {
		tag = assign_label_tag();
	}
	else {
		GPO.ConstExpress(&a);
		if(a.type == STRING) {
			GPO.Pgm.SetTokenIdx(save_token);
			tag = assign_label_tag();
		}
		else {
			tag = (int)real(&a);
		}
		a.Destroy();
	}
	if(tag <= 0)
		GPO.IntErrorCurToken("tag must be > zero");
	if(first_label) { // skip to last label 
		for(this_label = first_label; this_label != NULL;
		    prev_label = this_label, this_label = this_label->next)
			// is pThis the label we want? 
			if(tag <= this_label->tag)
				break;
	}
	// Insert pThis label into the list if it is a new one 
	if(!this_label || tag != this_label->tag) {
		new_label = new_text_label(tag);
		new_label->offset = default_offset;
		if(prev_label == NULL)
			first_label = new_label;
		else
			prev_label->next = new_label;
		new_label->next = this_label;
		this_label = new_label;
	}
	if(!GPO.Pgm.EndOfCommand()) {
		char * text;
		GPO.ParseLabelOptions(this_label, 0);
		text = GPO.TryToGetString();
		if(text) {
			SAlloc::F(this_label->text);
			this_label->text = text;
		}
	}
	// Now parse the label format and style options 
	GPO.ParseLabelOptions(this_label, 0);
}

/* assign a new label tag
 * labels are kept sorted by tag number, so pThis is easy
 * returns the lowest unassigned tag number
 */
static int assign_label_tag()
{
	int last = 0;           /* previous tag value */
	for(text_label * this_label = first_label; this_label != NULL; this_label = this_label->next)
		if(this_label->tag == last + 1)
			last++;
		else
			break;
	return (last + 1);
}

/* process 'set loadpath' command */
static void set_loadpath()
{
	/* We pick up all loadpath elements here before passing
	 * them on to set_var_loadpath()
	 */
	char * collect = NULL;
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand()) {
		clear_loadpath();
	}
	else while(!GPO.Pgm.EndOfCommand()) {
			char * ss;
			if((ss = GPO.TryToGetString())) {
				int len = (collect ? strlen(collect) : 0);
				gp_expand_tilde(&ss);
				collect = (char *)gp_realloc(collect, len+1+strlen(ss)+1, "tmp loadpath");
				if(len != 0) {
					strcpy(collect+len+1, ss);
					*(collect+len) = PATHSEP;
				}
				else
					strcpy(collect, ss);
				SAlloc::F(ss);
			}
			else {
				GPO.IntErrorCurToken("expected string");
			}
		}
	if(collect) {
		set_var_loadpath(collect);
		SAlloc::F(collect);
	}
}

/* process 'set fontpath' command */
/* Apr 2018 (V5.3) simplify pThis to a single directory */
static void set_fontpath()
{
	GPO.Pgm.Shift();
	SAlloc::F(PS_fontpath);
	PS_fontpath = GPO.TryToGetString();
}
//
// process 'set locale' command 
//
static void set_locale()
{
	char * s;
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand()) {
		init_locale();
	}
	else if((s = GPO.TryToGetString())) {
		set_var_locale(s);
		SAlloc::F(s);
	}
	else
		GPO.IntErrorCurToken("expected string");
}
//
// process 'set logscale' command 
//
//static void set_logscale()
void GnuPlot::SetLogScale()
{
	bool set_for_axis[AXIS_ARRAY_SIZE] = AXIS_ARRAY_INITIALIZER(FALSE);
	int axis;
	double newbase = 10;
	Pgm.Shift();
	if(Pgm.EndOfCommand()) {
		for(axis = 0; axis < POLAR_AXIS; axis++)
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
		if(!Pgm.EndOfCommand()) {
			newbase = fabs(GPO.RealExpression());
			if(newbase <= 1.0)
				IntErrorCurToken("log base must be > 1.0; logscale unchanged");
		}
	}
	for(axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
		if(set_for_axis[axis]) {
			static char command[128];
			char * dummy;
			if(!isalpha(axis_name((AXIS_INDEX)axis)[0]))
				continue;
			switch(axis) {
				case FIRST_Y_AXIS:
				case SECOND_Y_AXIS: dummy = "y"; break;
				case FIRST_Z_AXIS:
				case COLOR_AXIS: dummy = "z"; break;
				case POLAR_AXIS: dummy = "r"; break;
				default: dummy = "x"; break;
			}
			/* Avoid a warning message triggered by default axis range [-10:10] */
			if(AxS[axis].set_min <= 0 && AxS[axis].set_max > 0)
				AxS[axis].set_min = 0.1;
			/* Also forgive negative axis limits if we are currently autoscaling */
			if((AxS[axis].set_autoscale != AUTOSCALE_NONE) && (AxS[axis].set_min <= 0 || AxS[axis].set_max <= 0)) {
				AxS[axis].set_min = 0.1;
				AxS[axis].set_max = 10.;
			}
			if(newbase == 10.) {
				sprintf(command, "set nonlinear %s via log10(%s) inv 10**%s", axis_name((AXIS_INDEX)axis), dummy, dummy);
			}
			else {
				sprintf(command, "set nonlinear %s via log(%s)/log(%g) inv (%g)**%s", axis_name((AXIS_INDEX)axis), dummy, newbase, newbase, dummy);
			}
			do_string(command);
			AxS[axis].ticdef.logscaling = TRUE;
			AxS[axis].base = newbase;
			AxS[axis].log_base = log(newbase);
			AxS[axis].linked_to_primary->base = newbase;
			AxS[axis].linked_to_primary->log_base = log(newbase);
			// do_string("set nonlinear") cleared the log flags 
			AxS[axis].log = TRUE;
			AxS[axis].linked_to_primary->log = TRUE;
		}
	}
}
//
// process 'set mapping3d' command 
//
//static void set_mapping()
void GnuPlot::SetMapping()
{
	Pgm.Shift();
	if(Pgm.EndOfCommand())
		mapping3d = MAP3D_CARTESIAN; /* assuming same as points */
	else if(Pgm.AlmostEqualsCur("ca$rtesian"))
		mapping3d = MAP3D_CARTESIAN;
	else if(Pgm.AlmostEqualsCur("s$pherical"))
		mapping3d = MAP3D_SPHERICAL;
	else if(Pgm.AlmostEqualsCur("cy$lindrical"))
		mapping3d = MAP3D_CYLINDRICAL;
	else
		IntErrorCurToken("expecting 'cartesian', 'spherical', or 'cylindrical'");
	Pgm.Shift();
}
//
// process 'set {blrt}margin' command 
//
//static void set_margin(GpPosition * margin)
void GnuPlot::SetMargin(GpPosition * pMargin)
{
	pMargin->scalex = character;
	pMargin->x = -1;
	Pgm.Shift();
	if(!Pgm.EndOfCommand()) {
		if(Pgm.EqualsCur("at") && !Pgm.AlmostEquals(++Pgm.CToken, "sc$reen"))
			IntErrorCurToken("expecting 'screen <fraction>'");
		if(Pgm.AlmostEqualsCur("sc$reen")) {
			pMargin->scalex = screen;
			Pgm.Shift();
		}
		pMargin->x = GPO.RealExpression();
		if(pMargin->x < 0)
			pMargin->x = -1;
		if(pMargin->scalex == screen) {
			SETMAX(pMargin->x, 0.0);
			SETMIN(pMargin->x, 1.0);
		}
	}
}

/* process 'set micro' command */
static void set_micro()
{
	GPO.Pgm.Shift();
	use_micro = TRUE;
}

/* process 'set minus_sign' command */
static void set_minus_sign()
{
	GPO.Pgm.Shift();
	use_minus_sign = TRUE;
}

//static void set_separator(char ** xx_separators)
void GnuPlot::SetSeparator(char ** ppSeparators)
{
	Pgm.Shift();
	SAlloc::F(*ppSeparators);
	*ppSeparators = NULL;
	if(Pgm.EndOfCommand())
		return;
	if(Pgm.AlmostEqualsCur("white$space")) {
		Pgm.Shift();
	}
	else if(Pgm.EqualsCur("space")) {
		*ppSeparators = gp_strdup(" ");
		Pgm.Shift();
	}
	else if(Pgm.EqualsCur("comma")) {
		*ppSeparators = gp_strdup(",");
		Pgm.Shift();
	}
	else if(Pgm.EqualsCur("tab") || Pgm.EqualsCur("\'\\t\'")) {
		*ppSeparators = gp_strdup("\t");
		Pgm.Shift();
	}
	else if(!(*ppSeparators = TryToGetString())) {
		IntErrorCurToken("expected \"<separator_char>\"");
	}
}

//static void set_datafile_commentschars()
void GnuPlot::SetDataFileCommentsChars()
{
	char * s;
	Pgm.Shift();
	if(Pgm.EndOfCommand()) {
		SAlloc::F(df_commentschars);
		df_commentschars = gp_strdup(DEFAULT_COMMENTS_CHARS);
	}
	else if((s = TryToGetString())) {
		SAlloc::F(df_commentschars);
		df_commentschars = s;
	}
	else // Leave it the way it was 
		IntErrorCurToken("expected string with comments chars");
}

/* process 'set datafile missing' command */
static void set_missing()
{
	GPO.Pgm.Shift();
	ZFREE(missing_val);
	if(GPO.Pgm.EndOfCommand())
		return;
	if(GPO.Pgm.EqualsCur("NaN") || GPO.Pgm.EqualsCur("nan")) {
		missing_val = sstrdup("NaN");
		GPO.Pgm.Shift();
	}
	else if(!(missing_val = GPO.TryToGetString()))
		GPO.IntErrorCurToken("expected missing-value string");
}
//
// (version 5) 'set monochrome' command 
//
//static void set_monochrome()
void GnuPlot::SetMonochrome()
{
	monochrome = TRUE;
	if(!Pgm.EndOfCommand())
		Pgm.Shift();
	if(Pgm.AlmostEqualsCur("def$ault")) {
		Pgm.Shift();
		while(first_mono_linestyle)
			delete_linestyle(&first_mono_linestyle, first_mono_linestyle, first_mono_linestyle);
	}
	init_monochrome();
	if(Pgm.AlmostEqualsCur("linet$ype") || Pgm.EqualsCur("lt")) {
		// we can pass pThis off to the generic "set linetype" code 
		if(Pgm.EqualsNext("cycle")) {
			Pgm.Shift();
			Pgm.Shift();
			mono_recycle_count = IntExpression();
		}
		else
			SetLineStyle(&first_mono_linestyle, LP_TYPE);
	}
	if(!Pgm.EndOfCommand())
		IntErrorCurToken("unrecognized option");
}

//static void set_mouse()
void GnuPlot::SetMouse()
{
#ifdef USE_MOUSE
	char * ctmp;
	Pgm.Shift();
	mouse_setting.on = 1;
	while(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("do$ubleclick")) {
			Pgm.Shift();
			mouse_setting.doubleclick = static_cast<int>(GPO.RealExpression());
			if(mouse_setting.doubleclick < 0)
				mouse_setting.doubleclick = 0;
		}
		else if(Pgm.AlmostEqualsCur("nodo$ubleclick")) {
			mouse_setting.doubleclick = 0; /* double click off */
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("zoomco$ordinates")) {
			mouse_setting.annotate_zoom_box = 1;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("nozoomco$ordinates")) {
			mouse_setting.annotate_zoom_box = 0;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("po$lardistancedeg")) {
			mouse_setting.polardistance = 1;
			UpdateStatusLine();
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("polardistancet$an")) {
			mouse_setting.polardistance = 2;
			UpdateStatusLine();
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("nopo$lardistance")) {
			mouse_setting.polardistance = 0;
			UpdateStatusLine();
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("label$s")) {
			mouse_setting.label = 1;
			Pgm.Shift();
			// check if the optional argument "<label options>" is present 
			if(Pgm.IsStringValue(Pgm.GetCurTokenIdx()) && (ctmp = TryToGetString())) {
				SAlloc::F(mouse_setting.labelopts);
				mouse_setting.labelopts = ctmp;
			}
		}
		else if(Pgm.AlmostEqualsCur("nola$bels")) {
			mouse_setting.label = 0;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("ve$rbose")) {
			mouse_setting.verbose = 1;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("nove$rbose")) {
			mouse_setting.verbose = 0;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("zoomju$mp")) {
			mouse_setting.warp_pointer = 1;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("nozoomju$mp")) {
			mouse_setting.warp_pointer = 0;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("fo$rmat")) {
			Pgm.Shift();
			if(Pgm.IsStringValue(Pgm.GetCurTokenIdx()) && (ctmp = TryToGetString())) {
				if(mouse_setting.fmt != mouse_fmt_default)
					SAlloc::F(mouse_setting.fmt);
				mouse_setting.fmt = ctmp;
			}
			else
				mouse_setting.fmt = mouse_fmt_default;
		}
		else if(Pgm.AlmostEqualsCur("mo$useformat")) {
			Pgm.Shift();
			if(Pgm.EqualsCur("function")) {
				Pgm.Shift();
				int start_token = Pgm.GetCurTokenIdx()/*++c_token*/;
				if(!Pgm.EndOfCommand() || !mouse_readout_function.at) {
					free_at(mouse_readout_function.at);
					mouse_readout_function.at = perm_at();
					Pgm.MCapture(&mouse_readout_function.definition, start_token, Pgm.GetPrevTokenIdx());
				}
				// FIXME:  wants sanity check that pThis is a string-valued  function with parameters x and y 
				mouse_mode = MOUSE_COORDINATES_FUNCTION;
			}
			else if(Pgm.IsStringValue(Pgm.GetCurTokenIdx()) && (ctmp = TryToGetString())) {
				SAlloc::F(mouse_alt_string);
				mouse_alt_string = ctmp;
				if(!strlen(mouse_alt_string)) {
					ZFREE(mouse_alt_string);
					if(MOUSE_COORDINATES_ALT == mouse_mode)
						mouse_mode = MOUSE_COORDINATES_REAL;
				}
				else {
					mouse_mode = MOUSE_COORDINATES_ALT;
				}
				Pgm.Shift();
			}
			else {
				int itmp = IntExpression();
				if(itmp >= MOUSE_COORDINATES_REAL &&  itmp <= MOUSE_COORDINATES_FUNCTION) {
					if(MOUSE_COORDINATES_ALT == itmp && !mouse_alt_string)
						fprintf(stderr, "please 'set mouse mouseformat <fmt>' first.\n");
					else if(MOUSE_COORDINATES_FUNCTION == itmp && mouse_readout_function.at == NULL)
						fprintf(stderr, "please 'set mouse mouseformat function <f(x,y)>' first.\n");
					else
						mouse_mode = itmp;
				}
				else
					IntWarn(Pgm.GetPrevTokenIdx(), "not a valid mouseformat");
			}
		}
		else if(Pgm.AlmostEqualsCur("noru$ler")) {
			Pgm.Shift();
			set_ruler(FALSE, -1, -1);
		}
		else if(Pgm.AlmostEqualsCur("ru$ler")) {
			Pgm.Shift();
			if(Pgm.EndOfCommand() || !Pgm.EqualsCur("at")) {
				set_ruler(TRUE, -1, -1);
			}
			else { // set mouse ruler at ... 
				GpPosition where;
				int x, y;
				Pgm.Shift();
				if(Pgm.EndOfCommand())
					IntErrorCurToken("expecting ruler coordinates");
				GetPosition(&where);
				MapPosition(term, &where, &x, &y, "ruler at");
				set_ruler(TRUE, x, y);
			}
		}
		else if(Pgm.AlmostEqualsCur("zoomfac$tors")) {
			double x = 1.0, y = 1.0;
			Pgm.Shift();
			if(!Pgm.EndOfCommand()) {
				x = RealExpression();
				if(Pgm.EqualsCur(",")) {
					Pgm.Shift();
					y = RealExpression();
				}
			}
			mouse_setting.xmzoom_factor = x;
			mouse_setting.ymzoom_factor = y;
		}
		else {
			if(!Pgm.EndOfCommand())
				IntErrorCurToken("wrong option");
			break;
		}
	}
#else
	while(!Pgm.EndOfCommand())
		Pgm.Shift();
	IntWarn(NO_CARET, "pThis copy of gnuplot has no mouse support");
#endif
}
//
// process 'set offsets' command 
//
//static void set_offsets()
void GnuPlot::SetOffsets()
{
	Pgm.Shift();
	if(Pgm.EndOfCommand()) {
		loff.x = roff.x = toff.y = boff.y = 0.0;
	}
	else {
		loff.scalex = first_axes;
		if(Pgm.AlmostEqualsCur("gr$aph")) {
			loff.scalex = graph;
			Pgm.Shift();
		}
		loff.x = RealExpression();
		if(Pgm.EqualsCur(",")) {
			roff.scalex = first_axes;
			Pgm.Shift();
			if(Pgm.AlmostEqualsCur("gr$aph")) {
				roff.scalex = graph;
				Pgm.Shift();
			}
			roff.x = RealExpression();
			if(Pgm.EqualsCur(",")) {
				toff.scaley = first_axes;
				Pgm.Shift();
				if(Pgm.AlmostEqualsCur("gr$aph")) {
					toff.scaley = graph;
					Pgm.Shift();
				}
				toff.y = RealExpression();
				if(Pgm.EqualsCur(",")) {
					boff.scaley = first_axes;
					Pgm.Shift();
					if(Pgm.AlmostEqualsCur("gr$aph")) {
						boff.scaley = graph;
						Pgm.Shift();
					}
					boff.y = RealExpression();
				}
			}
		}
	}
}
//
// process 'set output' command 
//
static void set_output()
{
	char * testfile;
	GPO.Pgm.Shift();
	if(multiplot)
		GPO.IntErrorCurToken("you can't change the output in multiplot mode");
	if(GPO.Pgm.EndOfCommand()) {    /* no file specified */
		term_set_output(NULL);
		ZFREE(outstr); // means STDOUT 
	}
	else if((testfile = GPO.TryToGetString())) {
		gp_expand_tilde(&testfile);
		term_set_output(testfile);
		if(testfile != outstr) {
			SAlloc::F(testfile);
			testfile = outstr;
		}
		/* if we get here then it worked, and outstr now = testfile */
	}
	else
		GPO.IntErrorCurToken("expecting filename");
	invalidate_palette(); // Invalidate previous palette 
}
//
// process 'set print' command 
//
static void set_print()
{
	bool append_p = FALSE;
	char * testfile = NULL;
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand()) { // no file specified 
		print_set_output(NULL, FALSE, append_p);
	}
	else if(GPO.Pgm.EqualsCur("$") && GPO.Pgm.IsLetter(GPO.Pgm.GetCurTokenIdx()+1)) { /* datablock */
		// NB: has to come first because GPO.TryToGetString will choke on the datablock name 
		char * datablock_name = sstrdup(GPO.Pgm.ParseDatablockName());
		if(!GPO.Pgm.EndOfCommand()) {
			if(GPO.Pgm.EqualsCur("append")) {
				append_p = TRUE;
				GPO.Pgm.Shift();
			}
			else {
				GPO.IntErrorCurToken("expecting keyword \'append\'");
			}
		}
		print_set_output(datablock_name, TRUE, append_p);
	}
	else if((testfile = GPO.TryToGetString())) { /* file name */
		gp_expand_tilde(&testfile);
		if(!GPO.Pgm.EndOfCommand()) {
			if(GPO.Pgm.EqualsCur("append")) {
				append_p = TRUE;
				GPO.Pgm.Shift();
			}
			else
				GPO.IntErrorCurToken("expecting keyword \'append\'");
		}
		print_set_output(testfile, FALSE, append_p);
	}
	else
		GPO.IntErrorCurToken("expecting filename or datablock");
}
//
// process 'set psdir' command 
//
static void set_psdir()
{
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand()) {    /* no file specified */
		ZFREE(PS_psdir);
	}
	else if((PS_psdir = GPO.TryToGetString())) {
		gp_expand_tilde(&PS_psdir);
	}
	else
		GPO.IntErrorCurToken("expecting filename");
}
//
// process 'set overflow' command 
//
//static void set_overflow()
void GnuPlot::SetOverflow()
{
	Pgm.Shift();
	if(Pgm.EndOfCommand() || Pgm.EqualsCur("float"))
		Ev.OverflowHandling = INT64_OVERFLOW_TO_FLOAT;
	else if(Pgm.EqualsCur("undefined"))
		Ev.OverflowHandling = INT64_OVERFLOW_UNDEFINED;
	else if(Pgm.EqualsCur("NaN") || Pgm.EqualsCur("nan"))
		Ev.OverflowHandling = INT64_OVERFLOW_NAN;
	else
		IntErrorCurToken("unrecognized option");
	if(!Pgm.EndOfCommand())
		Pgm.Shift();
}
//
// process 'set parametric' command 
//
static void set_parametric()
{
	GPO.Pgm.Shift();
	if(!parametric) {
		parametric = TRUE;
		if(!polar) { /* already done for polar */
			strcpy(set_dummy_var[0], "t");
			strcpy(set_dummy_var[1], "y");
			if(interactive)
				fprintf(stderr, "\n\tdummy variable is t for curves, u/v for surfaces\n");
		}
	}
}

int enable_reset_palette = 1; // is resetting palette enabled? note: reset_palette() is disabled within 'test palette'
//
// default settings for palette 
//
//void reset_palette()
void GnuPlot::ResetPalette()
{
	if(enable_reset_palette) {
		SAlloc::F(SmPltt.P_Gradient);
		SAlloc::F(SmPltt.P_Color);
		free_at(SmPltt.Afunc.at);
		free_at(SmPltt.Bfunc.at);
		free_at(SmPltt.Cfunc.at);
		InitColor();
		pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_NONE;
	}
}
// 
// Process 'set palette defined' gradient specification */
// Syntax
//   set palette defined   -->  use default palette
//   set palette defined ( <pos1> <colorspec1>, ... , <posN> <colorspecN> )
//   <posX>  gray value, automatically rescaled to [0, 1]
//   <colorspecX>   :=  { "<color_name>" | "<X-style-color>" |  <r> <g> <b> }
//   <color_name>     predefined colors (see below)
//   <X-style-color>  "#rrggbb" with 2char hex values for red, green, blue
//   <r> <g> <b>      three values in [0, 1] for red, green and blue
// return 1 if named colors where used, 0 otherwise
// 
//static int set_palette_defined()
int GnuPlot::SetPaletteDefined()
{
	double p = 0, r = 0, g = 0, b = 0;
	int num, named_colors = 0;
	int actual_size = 8;
	invalidate_palette(); // Invalidate previous gradient 
	SAlloc::F(SmPltt.P_Gradient);
	SmPltt.P_Gradient = (gradient_struct *)gp_alloc(actual_size*sizeof(gradient_struct), "pm3d gradient");
	SmPltt.smallest_gradient_interval = 1;
	if(Pgm.EndOfCommand()) {
		// some default gradient 
		const double pal[][4] = { {0.0, 0.05, 0.05, 0.2}, {0.1, 0, 0, 1},
				    {0.3, 0.05, 0.9, 0.4}, {0.5, 0.9, 0.9, 0},
				    {0.7, 1, 0.6471, 0}, {0.8, 1, 0, 0},
				    {0.9, 0.94, 0.195, 0.195}, {1.0, 1, .8, .8} };
		for(int i = 0; i < 8; i++) {
			SmPltt.P_Gradient[i].pos = pal[i][0];
			SmPltt.P_Gradient[i].col.r = pal[i][1];
			SmPltt.P_Gradient[i].col.g = pal[i][2];
			SmPltt.P_Gradient[i].col.b = pal[i][3];
		}
		SmPltt.GradientNum = 8;
		SmPltt.CModel = C_MODEL_RGB;
		SmPltt.smallest_gradient_interval = 0.1; /* From pal[][] */
		Pgm.Rollback(); // Caller will increment! 
		return 0;
	}
	if(!Pgm.EqualsCur("(") )
		IntErrorCurToken("expected ( to start gradient definition");
	Pgm.Shift();
	num = -1;
	while(!Pgm.EndOfCommand()) {
		char * col_str;
		p = RealExpression();
		col_str = TryToGetString();
		if(col_str) {
			// Hex constant or X-style rgb value "#rrggbb" 
			if(col_str[0] == '#' || col_str[0] == '0') {
				// X-style specifier 
				int rr, gg, bb;
				if((sscanf(col_str, "#%2x%2x%2x", &rr, &gg, &bb) != 3 ) && (sscanf(col_str, "0x%2x%2x%2x", &rr, &gg, &bb) != 3 ))
					IntError(Pgm.GetPrevTokenIdx(), "Unknown color specifier. Use '#RRGGBB' of '0xRRGGBB'.");
				r = (double)(rr)/255.;
				g = (double)(gg)/255.;
				b = (double)(bb)/255.;

				/* Predefined color names.
				 * Could we move these definitions to some file that is included
				 * somehow during compilation instead hardcoding them?
				 */
			}
			else {
				int rgbval = lookup_table_entry(pm3d_color_names_tbl, col_str);
				if(rgbval < 0)
					IntError(Pgm.GetPrevTokenIdx(), "Unknown color name.");
				r = ((rgbval >> 16) & 255) / 255.;
				g = ((rgbval >> 8 ) & 255) / 255.;
				b = (rgbval & 255) / 255.;
				named_colors = 1;
			}
			SAlloc::F(col_str);
		}
		else {
			/* numerical rgb, hsv, xyz, ... values  [0,1] */
			r = RealExpression();
			if(r<0 || r>1) 
				IntError(Pgm.GetPrevTokenIdx(), "Value out of range [0,1].");
			g = RealExpression();
			if(g<0 || g>1) 
				IntError(Pgm.GetPrevTokenIdx(), "Value out of range [0,1].");
			b = RealExpression();
			if(b<0 || b>1) 
				IntError(Pgm.GetPrevTokenIdx(), "Value out of range [0,1].");
		}
		++num;
		if(num >= actual_size) {
			/* get more space for the gradient */
			actual_size += 10;
			SmPltt.P_Gradient = (gradient_struct *)gp_realloc(SmPltt.P_Gradient, actual_size*sizeof(gradient_struct), "pm3d gradient");
		}
		SmPltt.P_Gradient[num].pos = p;
		SmPltt.P_Gradient[num].col.r = r;
		SmPltt.P_Gradient[num].col.g = g;
		SmPltt.P_Gradient[num].col.b = b;
		if(Pgm.EqualsCur(")") ) break;
		if(!Pgm.EqualsCur(",") )
			IntErrorCurToken("expected comma");
		Pgm.Shift();
	}
	if(num <= 0) {
		ResetPalette();
		IntErrorCurToken("invalid palette syntax");
	}
	SmPltt.GradientNum = num + 1;
	CheckPaletteGrayscale();
	return named_colors;
}
// 
// process 'set palette colormap' command
// 
//static void set_palette_colormap()
void GnuPlot::SetPaletteColorMap()
{
	int i, actual_size;
	udvt_entry * colormap = GetColorMap(Pgm.GetCurTokenIdx());
	if(!colormap)
		IntErrorCurToken("expecting colormap name");
	ZFREE(SmPltt.P_Gradient);
	actual_size = colormap->udv_value.v.value_array[0].v.int_val;
	SmPltt.P_Gradient = (gradient_struct *)gp_alloc(actual_size*sizeof(gradient_struct), "gradient");
	SmPltt.GradientNum = actual_size;
	for(i = 0; i < actual_size; i++) {
		uint rgb24 = colormap->udv_value.v.value_array[i+1].v.int_val;
		SmPltt.P_Gradient[i].col.r = ((rgb24 >> 16) & 0xff) / 255.;
		SmPltt.P_Gradient[i].col.g = ((rgb24 >> 8) & 0xff) / 255.;
		SmPltt.P_Gradient[i].col.b = ((rgb24) & 0xff) / 255.;
		SmPltt.P_Gradient[i].pos = i;
	}
	CheckPaletteGrayscale();
}
// 
// process 'set palette file' command
// load a palette from file, honor datafile modifiers
// 
//static void set_palette_file()
void GnuPlot::SetPaletteFile()
{
	double v[4];
	int i, j, actual_size;
	char * file_name;
	Pgm.Shift();
	// get filename 
	if(!(file_name = TryToGetString()))
		IntErrorCurToken("missing filename");
	df_set_plot_mode(MODE_QUERY);   /* Needed only for binary datafiles */
	DfOpen(file_name, 4, NULL);
	SAlloc::F(file_name);
	ZFREE(SmPltt.P_Gradient);
	actual_size = 10;
	SmPltt.P_Gradient = (gradient_struct *)gp_alloc(actual_size*sizeof(gradient_struct), "gradient");
	i = 0;
	// values are clipped to [0,1] without notice 
	while((j = df_readline(v, 4)) != DF_EOF) {
		if(i >= actual_size) {
			actual_size += 10;
			SmPltt.P_Gradient = (gradient_struct *)gp_realloc(SmPltt.P_Gradient, actual_size*sizeof(gradient_struct), "pm3d gradient");
		}
		switch(j) {
			case 3:
			    SmPltt.P_Gradient[i].col.r = clip_to_01(v[0]);
			    SmPltt.P_Gradient[i].col.g = clip_to_01(v[1]);
			    SmPltt.P_Gradient[i].col.b = clip_to_01(v[2]);
			    SmPltt.P_Gradient[i].pos = i;
			    break;
			case 4:
			    SmPltt.P_Gradient[i].col.r = clip_to_01(v[1]);
			    SmPltt.P_Gradient[i].col.g = clip_to_01(v[2]);
			    SmPltt.P_Gradient[i].col.b = clip_to_01(v[3]);
			    SmPltt.P_Gradient[i].pos = v[0];
			    break;
			default:
			    df_close();
			    IntErrorCurToken("Bad data on line %d", df_line_number);
			    break;
		}
		++i;
	}
	df_close();
	if(!i)
		IntErrorCurToken("No valid palette found");
	SmPltt.GradientNum = i;
	CheckPaletteGrayscale();
}
// 
// Process a 'set palette function' command.
// Three functions with fixed dummy variable gray are registered which
// map gray to the different color components.
// The dummy variable must be 'gray'.
// 
//static void set_palette_function()
void GnuPlot::SetPaletteFunction()
{
	int start_token;
	char saved_dummy_var[MAX_ID_LEN+1];
	Pgm.Shift();
	strcpy(saved_dummy_var, c_dummy_var[0]);
	// set dummy variable 
	strncpy(c_dummy_var[0], "gray", MAX_ID_LEN);
	// Afunc 
	start_token = Pgm.GetCurTokenIdx();
	if(SmPltt.Afunc.at) {
		free_at(SmPltt.Afunc.at);
		SmPltt.Afunc.at = NULL;
	}
	dummy_func = &SmPltt.Afunc;
	SmPltt.Afunc.at = perm_at();
	if(!SmPltt.Afunc.at)
		IntError(start_token, "not enough memory for function");
	Pgm.MCapture(&(SmPltt.Afunc.definition), start_token, Pgm.GetPrevTokenIdx());
	dummy_func = NULL;
	if(!Pgm.EqualsCur(","))
		IntErrorCurToken("expected comma");
	Pgm.Shift();
	// Bfunc 
	start_token = Pgm.GetCurTokenIdx();
	if(SmPltt.Bfunc.at) {
		free_at(SmPltt.Bfunc.at);
		SmPltt.Bfunc.at = NULL;
	}
	dummy_func = &SmPltt.Bfunc;
	SmPltt.Bfunc.at = perm_at();
	if(!SmPltt.Bfunc.at)
		IntError(start_token, "not enough memory for function");
	Pgm.MCapture(&(SmPltt.Bfunc.definition), start_token, Pgm.GetPrevTokenIdx());
	dummy_func = NULL;
	if(!Pgm.EqualsCur(","))
		IntErrorCurToken("expected comma");
	Pgm.Shift();
	// Cfunc 
	start_token = Pgm.GetCurTokenIdx();
	if(SmPltt.Cfunc.at) {
		free_at(SmPltt.Cfunc.at);
		SmPltt.Cfunc.at = NULL;
	}
	dummy_func = &SmPltt.Cfunc;
	SmPltt.Cfunc.at = perm_at();
	if(!SmPltt.Cfunc.at)
		IntError(start_token, "not enough memory for function");
	Pgm.MCapture(&(SmPltt.Cfunc.definition), start_token, Pgm.GetPrevTokenIdx());
	dummy_func = NULL;
	strcpy(c_dummy_var[0], saved_dummy_var);
}
// 
// Normalize gray scale of gradient to fill [0,1] and
// complain if gray values are not strictly increasing.
// Maybe automatic sorting of the gray values could be a feature.
// 
//static void check_palette_grayscale()
void GnuPlot::CheckPaletteGrayscale()
{
	int i;
	double off, f;
	gradient_struct * gradient = SmPltt.P_Gradient;
	// check if gray values are sorted 
	for(i = 0; i < SmPltt.GradientNum-1; ++i) {
		if(gradient[i].pos > gradient[i+1].pos)
			IntErrorCurToken("Palette gradient not monotonic");
	}
	// fit gray axis into [0:1]:  subtract offset and rescale 
	off = gradient[0].pos;
	f = 1.0 / (gradient[SmPltt.GradientNum-1].pos-off);
	for(i = 1; i < SmPltt.GradientNum-1; ++i) {
		gradient[i].pos = f*(gradient[i].pos-off);
	}
	// paranoia on the first and last entries 
	gradient[0].pos = 0.0;
	gradient[SmPltt.GradientNum-1].pos = 1.0;
	// save smallest interval 
	SmPltt.smallest_gradient_interval = 1.0;
	for(i = 1; i < SmPltt.GradientNum-1; ++i) {
		if(((gradient[i].pos - gradient[i-1].pos) > 0) && (SmPltt.smallest_gradient_interval > (gradient[i].pos - gradient[i-1].pos)))
			SmPltt.smallest_gradient_interval = (gradient[i].pos - gradient[i-1].pos);
	}
}

#define CHECK_TRANSFORM  do {                             \
		if(transform_defined)                                \
			IntErrorCurToken("inconsistent palette options"); \
		transform_defined = 1;                                \
}  while(0)
//
// Process 'set palette' command 
//
//static void set_palette()
void GnuPlot::SetPalette()
{
	int transform_defined = 0;
	int named_color = 0;
	Pgm.Shift();
	if(Pgm.EndOfCommand()) /* reset to default settings */
		ResetPalette();
	else { // go through all options of 'set palette' 
		for(; !Pgm.EndOfCommand(); Pgm.Shift()) {
			switch(Pgm.LookupTableForCurrentToken(&set_palette_tbl[0])) {
				/* positive and negative picture */
				case S_PALETTE_POSITIVE: /* "pos$itive" */
				    SmPltt.Positive = SMPAL_POSITIVE;
				    continue;
				case S_PALETTE_NEGATIVE: /* "neg$ative" */
				    SmPltt.Positive = SMPAL_NEGATIVE;
				    continue;
				// Now the options that determine the palette of smooth colours 
				// gray or rgb-coloured 
				case S_PALETTE_GRAY: /* "gray" */
				    SmPltt.colorMode = SMPAL_COLOR_MODE_GRAY;
				    continue;
				case S_PALETTE_GAMMA: /* "gamma" */
				    Pgm.Shift();
				    SmPltt.gamma = RealExpression();
				    Pgm.Rollback();
				    continue;
				case S_PALETTE_COLOR: /* "col$or" */
				    if(pm3d_last_set_palette_mode != SMPAL_COLOR_MODE_NONE)
					    SmPltt.colorMode = pm3d_last_set_palette_mode;
				    else
					    SmPltt.colorMode = SMPAL_COLOR_MODE_RGB;
				    continue;
				// rgb color mapping formulae: rgb$formulae r,g,b (3 integers) 
				case S_PALETTE_RGBFORMULAE: { // "rgb$formulae" 
				    int i;
				    char * formerr = "color formula out of range (use `show palette rgbformulae' to display the range)";
				    CHECK_TRANSFORM;
				    Pgm.Shift();
				    i = IntExpression();
				    if(abs(i) >= SmPltt.colorFormulae)
					    IntErrorCurToken(formerr);
				    SmPltt.formulaR = i;
				    if(!Pgm.Equals(Pgm.CToken--, ","))
					    continue;
					Pgm.Shift();
					Pgm.Shift();
				    i = IntExpression();
				    if(abs(i) >= SmPltt.colorFormulae)
					    IntErrorCurToken(formerr);
				    SmPltt.formulaG = i;
				    if(!Pgm.Equals(Pgm.CToken--, ","))
					    continue;
					Pgm.Shift();
					Pgm.Shift();
				    i = IntExpression();
				    if(abs(i) >= SmPltt.colorFormulae)
					    IntErrorCurToken(formerr);
				    SmPltt.formulaB = i;
				    Pgm.Rollback();
				    SmPltt.colorMode = SMPAL_COLOR_MODE_RGB;
				    pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_RGB;
				    continue;
			    } /* rgbformulae */
				/* rgb color mapping based on the "cubehelix" scheme proposed by */
				/* D A Green (2011)  http://arxiv.org/abs/1108.5083		     */
				case S_PALETTE_CUBEHELIX: { /* cubehelix */
				    bool done = FALSE;
				    CHECK_TRANSFORM;
				    SmPltt.colorMode = SMPAL_COLOR_MODE_CUBEHELIX;
				    SmPltt.CModel = C_MODEL_RGB;
				    SmPltt.cubehelix_start = 0.5;
				    SmPltt.cubehelix_cycles = -1.5;
				    SmPltt.cubehelix_saturation = 1.0;
				    Pgm.Shift();
				    do {
					    if(Pgm.EqualsCur("start")) {
						    Pgm.Shift();
						    SmPltt.cubehelix_start = RealExpression();
					    }
					    else if(Pgm.AlmostEqualsCur("cyc$les")) {
						    Pgm.Shift();
						    SmPltt.cubehelix_cycles = RealExpression();
					    }
					    else if(Pgm.AlmostEqualsCur("sat$uration")) {
						    Pgm.Shift();
						    SmPltt.cubehelix_saturation = RealExpression();
					    }
					    else
						    done = TRUE;
				    } while(!done);
				    Pgm.Rollback();
				    continue;
			    } /* cubehelix */
				case S_PALETTE_COLORMAP: { /* colormap */
				    CHECK_TRANSFORM;
				    Pgm.Shift();
				    SetPaletteColorMap();
				    SmPltt.colorMode = SMPAL_COLOR_MODE_GRADIENT;
				    pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_GRADIENT;
				    continue;
			    }
				case S_PALETTE_DEFINED: { /* "def$ine" */
				    CHECK_TRANSFORM;
				    Pgm.Shift();
				    named_color = SetPaletteDefined();
				    SmPltt.colorMode = SMPAL_COLOR_MODE_GRADIENT;
				    pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_GRADIENT;
				    continue;
			    }
				case S_PALETTE_FILE: { /* "file" */
				    CHECK_TRANSFORM;
				    SetPaletteFile();
				    SmPltt.colorMode = SMPAL_COLOR_MODE_GRADIENT;
				    pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_GRADIENT;
				    Pgm.Rollback();
				    continue;
			    }
				case S_PALETTE_FUNCTIONS: { /* "func$tions" */
				    CHECK_TRANSFORM;
				    SetPaletteFunction();
				    SmPltt.colorMode = SMPAL_COLOR_MODE_FUNCTIONS;
				    pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_FUNCTIONS;
				    Pgm.Rollback();
				    continue;
			    }
				case S_PALETTE_MODEL: { /* "mo$del" */
				    int model;
				    Pgm.Shift();
				    if(Pgm.EndOfCommand())
					    IntErrorCurToken("expected color model");
				    model = Pgm.LookupTableForCurrentToken(&color_model_tbl[0]);
				    if(model == -1)
					    IntErrorCurToken("unknown color model");
				    if(model == C_MODEL_XYZ)
					    IntWarnCurToken("CIE/XYZ not supported");
				    SmPltt.CModel = model;
				    if(model == C_MODEL_HSV && Pgm.EqualsNext("start")) {
						Pgm.Shift();
						Pgm.Shift();
					    SmPltt.HSV_offset = RealExpression();
					    SmPltt.HSV_offset = clip_to_01(SmPltt.HSV_offset);
					    Pgm.Rollback();
				    }
				    continue;
			    }
				// ps_allcF: write all rgb formulae into PS file? 
				case S_PALETTE_NOPS_ALLCF: /* "nops_allcF" */
				    SmPltt.ps_allcF = false;
				    continue;
				case S_PALETTE_PS_ALLCF: /* "ps_allcF" */
				    SmPltt.ps_allcF = true;
				    continue;
				// max colors used 
				case S_PALETTE_MAXCOLORS: { /* "maxc$olors" */
				    Pgm.Shift();
				    int i = IntExpression();
				    if(i < 0 || i == 1)
					    IntWarnCurToken("maxcolors must be > 1");
				    else
					    SmPltt.UseMaxColors = i;
				    Pgm.Rollback();
				    continue;
			    }
			} /* switch over palette lookup table */
			IntErrorCurToken("invalid palette option");
		} /* end of while !end of command over palette options */
	} /* else(arguments found) */
	if(named_color && SmPltt.CModel != C_MODEL_RGB && interactive)
		IntWarn(NO_CARET, "Named colors will produce strange results if not in color mode RGB.");
	invalidate_palette(); // Invalidate previous palette 
}

#undef CHECK_TRANSFORM
// 
// V5.5 EXPERIMENTAL
// set colormap new <colormap-name>
// set colormap <colormap-name> range [min:max]
// 
//void set_colormap()
void GnuPlot::SetColorMap()
{
	Pgm.Shift();
	if(Pgm.EqualsCur("new"))
		NewColorMap();
	else if(Pgm.EqualsNext("range"))
		SetColorMapRange();
}
// 
// 'set colormap new <colormap>'
// saves color mapping of current palette into an array that can later be used
// to substitute for the main palettes, as in
// splot foo with pm3d fc palette <colormap>
// 
//static void new_colormap()
void GnuPlot::NewColorMap()
{
	udvt_entry * array;
	GpValue * A;
	double gray;
	rgb_color rgb1;
	rgb255_color rgb255;
	int colormap_size = 256;
	int i;
	// Create or recycle a udv containing an array with the requested name 
	Pgm.Shift();
	if(!Pgm.IsLetter(Pgm.GetCurTokenIdx()))
		IntErrorCurToken("illegal colormap name");
	array = AddUdv(Pgm.GetCurTokenIdx());
	array->udv_value.Destroy();
	Pgm.Shift();
	/* Take size from current palette */
	if(SmPltt.UseMaxColors > 0 && SmPltt.UseMaxColors <= 256)
		colormap_size = SmPltt.UseMaxColors;
	array->udv_value.v.value_array = (GpValue *)gp_alloc((colormap_size+1) * sizeof(GpValue), "colormap");
	array->udv_value.type = ARRAY;
	/* Element zero of the new array is not visible but contains the size
	 */
	A = array->udv_value.v.value_array;
	A[0].v.int_val = colormap_size;
	A[0].type = COLORMAP_ARRAY;

	/* FIXME: Leverage the known structure of value.v as a union
	 *        to overload both the colormap value as v.int_val
	 *        and the min/max range as v.cmplx_val.imag
	 *        There is nothing wrong with pThis in terms of available
	 *        storage, but a new member field of the union would be cleaner.
	 * Initialize to min = max = 0, which means use current cbrange.
	 * A different min/max can be written later via set_colormap();
	 */
	A[1].v.cmplx_val.imag = 0.0;    /* min */
	A[2].v.cmplx_val.imag = 0.0;    /* max */

	/* All other elements contain a 24 or 32 bit [A]RGB color value
	 * generated from the current palette.
	 */
	for(i = 0; i < colormap_size; i++) {
		gray = (double)i / (colormap_size-1);
		if(SmPltt.Positive == SMPAL_NEGATIVE)
			gray = 1.0 - gray;
		Rgb1FromGray(gray, &rgb1);
		rgb255_from_rgb1(rgb1, &rgb255);
		A[i+1].type = INTGR;
		A[i+1].v.int_val = (int)rgb255.r<<16 | (int)rgb255.g<<8 | (int)rgb255.b;
	}
}

/* set colormap <colormap-name> range [min:max]
 * FIXME: parsing the bare colormap name rather than a string works
 *        but that means you can't put the name in a variable.
 */
//static void set_colormap_range()
void GnuPlot::SetColorMapRange()
{
	udvt_entry * colormap = GetColorMap(Pgm.GetCurTokenIdx());
	double cm_min, cm_max;
	if(!colormap)
		IntErrorCurToken("not a colormap");
	Pgm.Shift();
	if(!Pgm.EqualsCur("range") || !Pgm.Equals(++Pgm.CToken, "["))
		IntErrorCurToken("syntax: set colormap <name> range [min:max]");
	Pgm.Shift();
	cm_min = RealExpression();
	Pgm.Shift();
	cm_max = RealExpression();
	if(!Pgm.EqualsCur("]"))
		IntErrorCurToken("syntax: set colormap <name> range [min:max]");
	Pgm.Shift();
	colormap->udv_value.v.value_array[1].v.cmplx_val.imag = cm_min;
	colormap->udv_value.v.value_array[2].v.cmplx_val.imag = cm_max;
}
//
// process 'set colorbox' command 
//
//static void set_colorbox()
void GnuPlot::SetColorBox()
{
	Pgm.Shift();
	if(Pgm.EndOfCommand()) /* reset to default position */
		color_box.where = SMCOLOR_BOX_DEFAULT;
	else { /* go through all options of 'set colorbox' */
		for(; !Pgm.EndOfCommand(); Pgm.Shift()) {
			switch(Pgm.LookupTableForCurrentToken(&set_colorbox_tbl[0])) {
				/* vertical or horizontal color gradient */
				case S_COLORBOX_VERTICAL: /* "v$ertical" */
				    color_box.rotation = 'v';
				    continue;
				case S_COLORBOX_HORIZONTAL: /* "h$orizontal" */
				    color_box.rotation = 'h';
				    continue;
				/* color box where: default position */
				case S_COLORBOX_DEFAULT: /* "def$ault" */
				    color_box.where = SMCOLOR_BOX_DEFAULT;
				    continue;
				/* color box where: position by user */
				case S_COLORBOX_USER: /* "u$ser" */
				    color_box.where = SMCOLOR_BOX_USER;
				    continue;
				/* color box layer: front or back */
				case S_COLORBOX_FRONT: /* "fr$ont" */
				    color_box.layer = LAYER_FRONT;
				    continue;
				case S_COLORBOX_BACK: /* "ba$ck" */
				    color_box.layer = LAYER_BACK;
				    continue;
				/* border of the color box */
				case S_COLORBOX_BORDER: /* "bo$rder" */
				    color_box.border = 1;
				    Pgm.Shift();
				    if(!Pgm.EndOfCommand()) {
					    /* expecting a border line type */
					    color_box.border_lt_tag = IntExpression();
					    if(color_box.border_lt_tag <= 0)
						    color_box.border_lt_tag = -1;
					    Pgm.Rollback();
				    }
				    continue;
				case S_COLORBOX_CBTICS: /* "cbtics" */
				    Pgm.Shift();
				    color_box.cbtics_lt_tag = IntExpression();
				    if(color_box.cbtics_lt_tag <= 0)
					    color_box.cbtics_lt_tag = -1;
				    Pgm.Rollback();
				    continue;
				case S_COLORBOX_BDEFAULT: /* "bd$efault" */
				    color_box.border_lt_tag = -1; /* use default border */
				    color_box.cbtics_lt_tag = 0; /* and cbtics */
				    continue;
				case S_COLORBOX_NOBORDER: /* "nobo$rder" */
				    color_box.border = 0;
				    continue;
				// colorbox origin 
				case S_COLORBOX_ORIGIN: /* "o$rigin" */
				    Pgm.Shift();
				    if(Pgm.EndOfCommand()) {
					    IntErrorCurToken("expecting screen value [0 - 1]");
				    }
				    else {
					    // FIXME: should be 2 but old save files may have 3 
					    GetPositionDefault(&color_box.origin, screen, 3);
				    }
				    Pgm.Rollback();
				    continue;
				/* colorbox size */
				case S_COLORBOX_SIZE: /* "s$ize" */
				    Pgm.Shift();
				    if(Pgm.EndOfCommand()) {
					    IntErrorCurToken("expecting screen value [0 - 1]");
				    }
				    else {
					    // FIXME: should be 2 but old save files may have 3 
					    GetPositionDefault(&color_box.size, screen, 3);
				    }
				    Pgm.Rollback();
				    continue;
				case S_COLORBOX_INVERT: // Flip direction of color gradient + cbaxis 
				    color_box.invert = TRUE;
				    continue;
				case S_COLORBOX_NOINVERT: // Flip direction of color gradient + cbaxis 
				    color_box.invert = FALSE;
				    continue;
			} /* switch over colorbox lookup table */
			IntErrorCurToken("invalid colorbox option");
		} /* end of while !end of command over colorbox options */
		if(color_box.where == SMCOLOR_BOX_NO) /* default: draw at default position */
			color_box.where = SMCOLOR_BOX_DEFAULT;
	}
}
//
// process 'set pm3d' command 
//
//static void set_pm3d()
void GnuPlot::SetPm3D()
{
	Pgm.Shift();
	int c_token0 = Pgm.GetCurTokenIdx();
	if(Pgm.EndOfCommand()) { /* assume default settings */
		pm3d_reset(); /* sets pm3d.implicit to PM3D_EXPLICIT and pm3d.where to "s" */
		pm3d.implicit = PM3D_IMPLICIT; /* for historical reasons */
	}
	else { /* go through all options of 'set pm3d' */
		for(; !Pgm.EndOfCommand(); Pgm.Shift()) {
			switch(Pgm.LookupTableForCurrentToken(&set_pm3d_tbl[0])) {
				/* where to plot */
				case S_PM3D_AT: /* "at" */
				    Pgm.Shift();
				    if(GetPm3DAtOption(&pm3d.where[0]))
					    return; /* error */
				    Pgm.Rollback();
#if 1
				    if(Pgm.GetCurTokenIdx() == c_token0+1)
					    /* for historical reasons: if "at" is the first option of pm3d,
					     * like "set pm3d at X other_opts;", then implicit is switched on */
					    pm3d.implicit = PM3D_IMPLICIT;
#endif
				    continue;
				case S_PM3D_INTERPOLATE: /* "interpolate" */
				    Pgm.Shift();
				    if(Pgm.EndOfCommand()) {
					    IntErrorCurToken("expecting step values i,j");
				    }
				    else {
					    pm3d.interp_i = IntExpression();
					    if(!Pgm.EqualsCur(","))
						    IntErrorCurToken("',' expected");
					    Pgm.Shift();
					    pm3d.interp_j = IntExpression();
					    Pgm.Rollback();
				    }
				    continue;
				/* forward and backward drawing direction */
				case S_PM3D_SCANSFORWARD: /* "scansfor$ward" */
				    pm3d.direction = PM3D_SCANS_FORWARD;
				    continue;
				case S_PM3D_SCANSBACKWARD: /* "scansback$ward" */
				    pm3d.direction = PM3D_SCANS_BACKWARD;
				    continue;
				case S_PM3D_SCANS_AUTOMATIC: /* "scansauto$matic" */
				    pm3d.direction = PM3D_SCANS_AUTOMATIC;
				    continue;
				case S_PM3D_DEPTH: /* "dep$thorder" */
				    pm3d.direction = PM3D_DEPTH;
				    if(Pgm.EqualsNext("base")) {
					    pm3d.base_sort = TRUE;
					    Pgm.Shift();
				    }
				    else {
					    pm3d.base_sort = FALSE;
				    }
				    continue;
				/* flush scans: left, right or center */
				case S_PM3D_FLUSH: /* "fl$ush" */
				    Pgm.Shift();
				    if(Pgm.AlmostEqualsCur("b$egin"))
					    pm3d.flush = PM3D_FLUSH_BEGIN;
				    else if(Pgm.AlmostEqualsCur("c$enter"))
					    pm3d.flush = PM3D_FLUSH_CENTER;
				    else if(Pgm.AlmostEqualsCur("e$nd"))
					    pm3d.flush = PM3D_FLUSH_END;
				    else
					    IntErrorCurToken("expecting flush 'begin', 'center' or 'end'");
				    continue;
				/* clipping method */
				case S_PM3D_CLIP_1IN: /* "clip1$in" */
				    pm3d.clip = PM3D_CLIP_1IN;
				    continue;
				case S_PM3D_CLIP_4IN: /* "clip4$in" */
				    pm3d.clip = PM3D_CLIP_4IN;
				    continue;
				case S_PM3D_CLIP_Z: /* "clip" */
				    pm3d.clip = PM3D_CLIP_Z;
				    if(Pgm.EqualsNext("z")) /* DEPRECATED */
					    Pgm.Shift();
				    continue;
				case S_PM3D_CLIPCB: pm3d.no_clipcb = FALSE; continue;
				case S_PM3D_NOCLIPCB: pm3d.no_clipcb = TRUE; continue;
				/* setup everything for plotting a map */
				case S_PM3D_MAP: /* "map" */
				    pm3d.where[0] = 'b'; 
					pm3d.where[1] = 0; /* set pm3d at b */
				    data_style = PM3DSURFACE;
				    func_style = PM3DSURFACE;
				    splot_map = TRUE;
				    continue;
				/* flushing triangles */
				case S_PM3D_FTRIANGLES: /* "ftr$iangles" */
				    pm3d.ftriangles = 1;
				    continue;
				case S_PM3D_NOFTRIANGLES: /* "noftr$iangles" */
				    pm3d.ftriangles = 0;
				    continue;
				// deprecated pm3d "hidden3d" option, now used for borders 
				case S_PM3D_HIDDEN:
				    if(Pgm.IsANumber(Pgm.GetCurTokenIdx()+1)) {
					    Pgm.Shift();
					    load_linetype(term, &pm3d.border, IntExpression());
					    Pgm.Rollback();
					    continue;
				    }
				/* fall through */
				case S_PM3D_BORDER: /* border {linespec} */
				    Pgm.Shift();
				    pm3d.border = default_pm3d_border;
				    LpParse(&pm3d.border, LP_ADHOC, FALSE);
				    Pgm.Rollback();
				    continue;
				case S_PM3D_NOHIDDEN:
				case S_PM3D_NOBORDER:
				    pm3d.border.l_type = LT_NODRAW;
				    continue;
				case S_PM3D_SOLID: /* "so$lid" */
				case S_PM3D_NOTRANSPARENT: /* "notr$ansparent" */
				case S_PM3D_NOSOLID: /* "noso$lid" */
				case S_PM3D_TRANSPARENT: /* "tr$ansparent" */
				    if(interactive)
					    IntWarnCurToken("Deprecated syntax --- ignored");
				case S_PM3D_IMPLICIT: /* "i$mplicit" */
				case S_PM3D_NOEXPLICIT: /* "noe$xplicit" */
				    pm3d.implicit = PM3D_IMPLICIT;
				    continue;
				case S_PM3D_NOIMPLICIT: /* "noi$mplicit" */
				case S_PM3D_EXPLICIT: /* "e$xplicit" */
				    pm3d.implicit = PM3D_EXPLICIT;
				    continue;
				case S_PM3D_WHICH_CORNER: /* "corners2color" */
				    Pgm.Shift();
				    if(Pgm.EqualsCur("mean"))
					    pm3d.which_corner_color = PM3D_WHICHCORNER_MEAN;
				    else if(Pgm.EqualsCur("geomean"))
					    pm3d.which_corner_color = PM3D_WHICHCORNER_GEOMEAN;
				    else if(Pgm.EqualsCur("harmean"))
					    pm3d.which_corner_color = PM3D_WHICHCORNER_HARMEAN;
				    else if(Pgm.EqualsCur("median"))
					    pm3d.which_corner_color = PM3D_WHICHCORNER_MEDIAN;
				    else if(Pgm.EqualsCur("min"))
					    pm3d.which_corner_color = PM3D_WHICHCORNER_MIN;
				    else if(Pgm.EqualsCur("max"))
					    pm3d.which_corner_color = PM3D_WHICHCORNER_MAX;
				    else if(Pgm.EqualsCur("rms"))
					    pm3d.which_corner_color = PM3D_WHICHCORNER_RMS;
				    else if(Pgm.EqualsCur("c1"))
					    pm3d.which_corner_color = PM3D_WHICHCORNER_C1;
				    else if(Pgm.EqualsCur("c2"))
					    pm3d.which_corner_color = PM3D_WHICHCORNER_C2;
				    else if(Pgm.EqualsCur("c3"))
					    pm3d.which_corner_color = PM3D_WHICHCORNER_C3;
				    else if(Pgm.EqualsCur("c4"))
					    pm3d.which_corner_color = PM3D_WHICHCORNER_C4;
				    else
					    IntErrorCurToken("expecting 'mean', 'geomean', 'harmean', 'median', 'min', 'max', 'c1', 'c2', 'c3' or 'c4'");
				    continue;
				case S_PM3D_NOLIGHTING_MODEL:
				    pm3d_shade.strength = 0.0;
				    continue;
				case S_PM3D_LIGHTING_MODEL:
				    ParseLightingOptions();
				    continue;
			} /* switch over pm3d lookup table */
			IntErrorCurToken("invalid pm3d option");
		} /* end of while !end of command over pm3d options */
		if(PM3D_SCANS_AUTOMATIC == pm3d.direction && PM3D_FLUSH_BEGIN != pm3d.flush) {
			pm3d.direction = PM3D_SCANS_FORWARD;
			FPRINTF((stderr, "pm3d: `scansautomatic' and `flush %s' are incompatible\n", PM3D_FLUSH_END == pm3d.flush ? "end" : "center"));
		}
	}
}
//
// process 'set pointintervalbox' command 
//
static void set_pointintervalbox()
{
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand())
		pointintervalbox = 1.0;
	else
		pointintervalbox = GPO.RealExpression();
	if(pointintervalbox <= 0)
		pointintervalbox = 1.0;
}
//
// process 'set pointsize' command 
//
static void set_pointsize()
{
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand())
		pointsize = 1.0;
	else
		pointsize = GPO.RealExpression();
	if(pointsize <= 0)
		pointsize = 1.0;
}
//
// process 'set polar' command 
//
//static void set_polar()
void GnuPlot::SetPolar()
{
	Pgm.Shift();
	if(!polar) {
		polar = true;
		raxis = true;
		if(!parametric) {
			if(interactive)
				(void)fprintf(stderr, "\n\tdummy variable is t for curves\n");
			strcpy(set_dummy_var[0], "t");
		}
		if(AxS[T_AXIS].set_autoscale) {
			AxS[T_AXIS].set_min = 0.0; // only if user has not set a range manually 
			AxS[T_AXIS].set_max = 2 * M_PI / ang2rad; // 360 if degrees, 2pi if radians 
		}
		if(AxS[POLAR_AXIS].set_autoscale != AUTOSCALE_BOTH)
			RRangeToXY();
	}
}
/*
 * Process command     'set object <tag> {rectangle|ellipse|circle|polygon}'
 * set object {tag} rectangle {from <bottom_left> {to|rto} <top_right>}
 *                     {{at|center} <xcen>,<ycen> size <w>,<h>}
 *                     {fc|fillcolor <colorspec>} {lw|linewidth <lw>}
 *                     {fs <fillstyle>} {front|back|behind}
 *                     {default}
 * EAM Jan 2005
 */
//static void set_object()
void GnuPlot::SetObject()
{
	int tag;
	// The next token must either be a tag or the object type 
	Pgm.Shift();
	if(Pgm.AlmostEqualsCur("rect$angle") || Pgm.AlmostEqualsCur("ell$ipse") || Pgm.AlmostEqualsCur("circ$le") || Pgm.AlmostEqualsCur("poly$gon"))
		tag = -1; // We'll figure out what it really is later 
	else {
		tag = IntExpression();
		if(tag <= 0)
			IntErrorCurToken("tag must be > zero");
	}
	if(Pgm.AlmostEqualsCur("rect$angle")) {
		SetObj(tag, OBJ_RECTANGLE);
	}
	else if(Pgm.AlmostEqualsCur("ell$ipse")) {
		SetObj(tag, OBJ_ELLIPSE);
	}
	else if(Pgm.AlmostEqualsCur("circ$le")) {
		SetObj(tag, OBJ_CIRCLE);
	}
	else if(Pgm.AlmostEqualsCur("poly$gon")) {
		SetObj(tag, OBJ_POLYGON);
	}
	else if(tag > 0) {
		// Look for existing object with pThis tag 
		t_object * this_object = first_object;
		for(; this_object != NULL; this_object = this_object->next)
			if(tag == this_object->tag)
				break;
		if(this_object && tag == this_object->tag) {
			Pgm.Rollback();
			SetObj(tag, this_object->object_type);
		}
		else
			IntErrorCurToken("unknown object");
	}
	else
		IntErrorCurToken("unrecognized object type");
}

static t_object * new_object(int tag, int object_type, t_object * pNew)
{
	t_object def_rect(t_object::defRectangle); // = DEFAULT_RECTANGLE_STYLE;
	t_object def_ellipse(t_object::defEllipse); // = DEFAULT_ELLIPSE_STYLE;
	t_object def_circle(t_object::defCircle); // = DEFAULT_CIRCLE_STYLE;
	t_object def_polygon(t_object::defPolygon); // = DEFAULT_POLYGON_STYLE;
	if(!pNew)
		pNew = (t_object *)gp_alloc(sizeof(GpObject), "object");
	else if(pNew->object_type == OBJ_POLYGON)
		SAlloc::F(pNew->o.polygon.vertex);
	if(object_type == OBJ_RECTANGLE) {
		*pNew = def_rect;
		pNew->lp_properties.l_type = LT_DEFAULT; /* Use default rectangle color */
		pNew->fillstyle.fillstyle = FS_DEFAULT; /* and default fill style */
	}
	else if(object_type == OBJ_ELLIPSE)
		*pNew = def_ellipse;
	else if(object_type == OBJ_CIRCLE)
		*pNew = def_circle;
	else if(object_type == OBJ_POLYGON)
		*pNew = def_polygon;
	else
		GPO.IntError(NO_CARET, "object initialization failure");
	pNew->tag = tag;
	pNew->object_type = object_type;
	return pNew;
}

//static void set_obj(int tag, int obj_type)
void GnuPlot::SetObj(int tag, int objType)
{
	t_rectangle * this_rect = NULL;
	t_ellipse * this_ellipse = NULL;
	t_circle * this_circle = NULL;
	t_polygon * this_polygon = NULL;
	t_object * this_object = NULL;
	t_object * new_obj = NULL;
	t_object * prev_object = NULL;
	bool got_fill = FALSE;
	bool got_lt = FALSE;
	bool got_fc = FALSE;
	bool got_corners = FALSE;
	bool got_center = FALSE;
	bool got_origin = FALSE;
	Pgm.Shift();
	// We are setting the default, not any particular rectangle 
	if(tag < -1) {
		Pgm.Rollback();
		if(objType == OBJ_RECTANGLE) {
			this_object = &default_rectangle;
			this_rect = &this_object->o.rectangle;
		}
		else
			IntErrorCurToken("Unknown object type");
	}
	else {
		// Look for existing object with pThis tag 
		for(this_object = first_object; this_object; prev_object = this_object, this_object = this_object->next)
			if(0 < tag && tag <= this_object->tag) // is pThis the one we want? 
				break;
		// Insert pThis rect into the list if it is a new one 
		if(this_object == NULL || tag != this_object->tag) {
			if(tag == -1)
				tag = (prev_object) ? prev_object->tag+1 : 1;
			new_obj = new_object(tag, objType, NULL);
			if(prev_object == NULL)
				first_object = new_obj;
			else
				prev_object->next = new_obj;
			new_obj->next = this_object;
			this_object = new_obj;
			// V5 CHANGE: Apply default rectangle style now rather than later 
			if(objType == OBJ_RECTANGLE) {
				this_object->fillstyle = default_rectangle.fillstyle;
				this_object->lp_properties = default_rectangle.lp_properties;
			}
		}
		// Over-write old object if the type has changed 
		else if(this_object->object_type != objType) {
			t_object * save_link = this_object->next;
			new_obj = new_object(tag, objType, this_object);
			this_object->next = save_link;
		}
		this_rect = &this_object->o.rectangle;
		this_ellipse = &this_object->o.ellipse;
		this_circle = &this_object->o.circle;
		this_polygon = &this_object->o.polygon;
	}
	while(!Pgm.EndOfCommand()) {
		int save_token = Pgm.GetCurTokenIdx();
		switch(objType) {
			case OBJ_RECTANGLE:
			    if(Pgm.EqualsCur("from")) {
				    // Read in the bottom left and upper right corners 
				    Pgm.Shift();
				    GetPosition(&this_rect->bl);
				    if(Pgm.EqualsCur("to")) {
					    Pgm.Shift();
					    GetPosition(&this_rect->tr);
				    }
				    else if(Pgm.EqualsCur("rto")) {
					    Pgm.Shift();
					    GetPositionDefault(&this_rect->tr, this_rect->bl.scalex, 2);
					    if(this_rect->bl.scalex != this_rect->tr.scalex || this_rect->bl.scaley != this_rect->tr.scaley)
						    IntErrorCurToken("relative coordinates must match in type");
					    this_rect->tr.x += this_rect->bl.x;
					    this_rect->tr.y += this_rect->bl.y;
				    }
				    else
					    IntErrorCurToken("Expecting to or rto");
				    got_corners = TRUE;
				    this_rect->type = 0;
				    continue;
			    }
			    else if(Pgm.EqualsCur("at") || Pgm.AlmostEqualsCur("cen$ter")) {
				    // Read in the center position 
				    Pgm.Shift();
				    GetPosition(&this_rect->center);
				    got_center = TRUE;
				    this_rect->type = 1;
				    continue;
			    }
			    else if(Pgm.EqualsCur("size")) {
				    // Read in the width and height 
				    Pgm.Shift();
				    GetPosition(&this_rect->extent);
				    got_center = TRUE;
				    this_rect->type = 1;
				    continue;
			    }
			    break;
			case OBJ_CIRCLE:
			    if(Pgm.EqualsCur("at") || Pgm.AlmostEqualsCur("cen$ter")) {
				    // Read in the center position 
				    Pgm.Shift();
				    GetPosition(&this_circle->center);
				    continue;
			    }
			    else if(Pgm.EqualsCur("size") || Pgm.EqualsCur("radius")) {
				    // Read in the radius 
				    Pgm.Shift();
				    GetPosition(&this_circle->extent);
				    continue;
			    }
			    else if(Pgm.EqualsCur("arc")) {
				    // Start and end angle for arc 
					Pgm.Shift();
				    if(Pgm.EqualsCur("[")) {
					    double arc;
					    Pgm.Shift();
					    arc = GPO.RealExpression();
					    if(fabs(arc) > 1000.)
						    IntError(Pgm.GetPrevTokenIdx(), "Angle out of range");
					    else
						    this_circle->arc_begin = arc;
					    if(Pgm.EqualsCurShift(":")) {
						    arc = GPO.RealExpression();
						    if(fabs(arc) > 1000.)
							    IntError(Pgm.GetPrevTokenIdx(), "Angle out of range");
						    else
							    this_circle->arc_end = arc;
						    if(Pgm.EqualsCurShift("]"))
							    continue;
					    }
				    }
				    IntError(--Pgm.CToken, "Expecting arc [<begin>:<end>]");
			    }
			    else if(Pgm.EqualsCur("wedge")) {
				    Pgm.Shift();
				    this_circle->wedge = TRUE;
				    continue;
			    }
			    else if(Pgm.EqualsCur("nowedge")) {
				    Pgm.Shift();
				    this_circle->wedge = FALSE;
				    continue;
			    }
			    break;

			case OBJ_ELLIPSE:
			    if(Pgm.EqualsCur("at") || Pgm.AlmostEqualsCur("cen$ter")) {
				    // Read in the center position 
				    Pgm.Shift();
				    GetPosition(&this_ellipse->center);
				    continue;
			    }
			    else if(Pgm.EqualsCur("size")) {
				    // Read in the width and height 
				    Pgm.Shift();
				    GetPosition(&this_ellipse->extent);
				    continue;
			    }
			    else if(Pgm.AlmostEqualsCur("ang$le")) {
				    Pgm.Shift();
				    this_ellipse->orientation = GPO.RealExpression();
				    continue;
			    }
			    else if(Pgm.AlmostEqualsCur("unit$s")) {
				    Pgm.Shift();
				    if(Pgm.EqualsCur("xy") || Pgm.EndOfCommand())
					    this_ellipse->type = ELLIPSEAXES_XY;
				    else if(Pgm.EqualsCur("xx"))
					    this_ellipse->type = ELLIPSEAXES_XX;
				    else if(Pgm.EqualsCur("yy"))
					    this_ellipse->type = ELLIPSEAXES_YY;
				    else
					    IntErrorCurToken("expecting 'xy', 'xx' or 'yy'");
				    Pgm.Shift();
				    continue;
			    }
			    break;
			case OBJ_POLYGON:
			    if(Pgm.EqualsCur("from")) {
				    Pgm.Shift();
				    this_polygon->vertex = (GpPosition *)gp_realloc(this_polygon->vertex, sizeof(GpPosition), "polygon vertex");
				    GetPosition(&this_polygon->vertex[0]);
				    this_polygon->type = 1;
				    got_origin = TRUE;
				    continue;
			    }
			    if(!got_corners && (Pgm.EqualsCur("to") || Pgm.EqualsCur("rto"))) {
				    while(Pgm.EqualsCur("to") || Pgm.EqualsCur("rto")) {
					    if(!got_origin)
						    goto polygon_error;
					    this_polygon->vertex = (GpPosition *)gp_realloc(this_polygon->vertex, (this_polygon->type+1) * sizeof(GpPosition), "polygon vertex");
					    if(Pgm.EqualsCurShift("to")) {
						    GetPosition(&this_polygon->vertex[this_polygon->type]);
					    }
					    else { /* "rto" */
						    int v = this_polygon->type;
						    GetPositionDefault(&this_polygon->vertex[v],
							this_polygon->vertex->scalex, 2);
						    if(this_polygon->vertex[v].scalex != this_polygon->vertex[v-1].scalex || this_polygon->vertex[v].scaley != this_polygon->vertex[v-1].scaley)
							    IntErrorCurToken("relative coordinates must match in type");
						    this_polygon->vertex[v].x += this_polygon->vertex[v-1].x;
						    this_polygon->vertex[v].y += this_polygon->vertex[v-1].y;
					    }
					    this_polygon->type++;
					    got_corners = TRUE;
				    }
				    if(got_corners && memcmp(&this_polygon->vertex[this_polygon->type-1], &this_polygon->vertex[0], sizeof(struct GpPosition))) {
					    fprintf(stderr, "Polygon is not closed - adding extra vertex\n");
					    this_polygon->vertex = (GpPosition *)gp_realloc(this_polygon->vertex, (this_polygon->type+1) * sizeof(GpPosition), "polygon vertex");
					    this_polygon->vertex[this_polygon->type] = this_polygon->vertex[0];
					    this_polygon->type++;
				    }
				    continue;
			    }
			    break;
polygon_error:
			    ZFREE(this_polygon->vertex);
			    this_polygon->type = 0;
			    IntErrorCurToken("Unrecognized polygon syntax");
			/* End of polygon options */
			default:
			    IntErrorCurToken("unrecognized object type");
		} /* End of object-specific options */
		//
		// The rest of the options apply to any type of object 
		//
		if(Pgm.EqualsCur("front")) {
			this_object->layer = LAYER_FRONT;
			Pgm.Shift();
			continue;
		}
		else if(Pgm.EqualsCur("back")) {
			this_object->layer = LAYER_BACK;
			Pgm.Shift();
			continue;
		}
		else if(Pgm.EqualsCur("behind")) {
			this_object->layer = LAYER_BEHIND;
			Pgm.Shift();
			continue;
		}
		else if(Pgm.EqualsCur("fb")) {
			// Not documented.  Used by test code for grid walls 
			this_object->layer = LAYER_FRONTBACK;
			Pgm.Shift();
			continue;
		}
		else if(Pgm.AlmostEqualsCur("depth$order")) {
			// Requests that pThis object be sorted with pm3d quadrangles 
			this_object->layer = LAYER_DEPTHORDER;
			Pgm.Shift();
			continue;
		}
		else if(Pgm.AlmostEqualsCur("def$ault")) {
			if(tag < 0) {
				IntErrorCurToken("Invalid command - did you mean 'unset style rectangle'?");
			}
			else {
				this_object->lp_properties.l_type = LT_DEFAULT;
				this_object->fillstyle.fillstyle = FS_DEFAULT;
			}
			got_fill = got_lt = TRUE;
			Pgm.Shift();
			continue;
		}
		else if(Pgm.EqualsCur("clip")) {
			this_object->clip = OBJ_CLIP;
			Pgm.Shift();
			continue;
		}
		else if(Pgm.EqualsCur("noclip")) {
			this_object->clip = OBJ_NOCLIP;
			Pgm.Shift();
			continue;
		}
		// Now parse the style options; default to whatever the global style is 
		if(!got_fill) {
			if(new_obj) {
				this_object->fillstyle = (this_object->object_type == OBJ_RECTANGLE) ? default_rectangle.fillstyle : default_fillstyle;
			}
			ParseFillStyle(&this_object->fillstyle);
			if(Pgm.GetCurTokenIdx() != save_token) {
				got_fill = true;
				continue;
			}
		}
		// Parse the colorspec 
		if(!got_fc) {
			if(Pgm.EqualsCur("fc") || Pgm.AlmostEqualsCur("fillc$olor")) {
				this_object->lp_properties.l_type = LT_BLACK; /* Anything but LT_DEFAULT */
				ParseColorSpec(&this_object->lp_properties.pm3d_color, TC_FRAC);
				if(this_object->lp_properties.pm3d_color.type == TC_DEFAULT)
					this_object->lp_properties.l_type = LT_DEFAULT;
			}
			if(Pgm.GetCurTokenIdx() != save_token) {
				got_fc = TRUE;
				continue;
			}
		}
		// Line properties (will be used for the object border if the fillstyle has one. 
		// LP_NOFILL means don't eat fillcolor here since at is set separately with "fc". 
		if(!got_lt) {
			lp_style_type lptmp = this_object->lp_properties;
			LpParse(&lptmp, LP_NOFILL, FALSE);
			if(Pgm.GetCurTokenIdx() != save_token) {
				this_object->lp_properties.l_width = lptmp.l_width;
				this_object->lp_properties.d_type = lptmp.d_type;
				this_object->lp_properties.custom_dash_pattern = lptmp.custom_dash_pattern;
				got_lt = TRUE;
				continue;
			}
		}
		IntErrorCurToken("Unrecognized or duplicate option");
	}
	if(got_center && got_corners)
		IntError(NO_CARET, "Inconsistent options");
}

//static void set_wall()
void GnuPlot::SetWall()
{
	t_object * p_object = NULL;
	Pgm.Shift();
	if(Pgm.AlmostEqualsCur("y0")) {
		p_object = &grid_wall[WALL_Y0_TAG];
		p_object->layer = LAYER_FRONTBACK;
		Pgm.Shift();
	}
	else if(Pgm.AlmostEqualsCur("x0")) {
		p_object = &grid_wall[WALL_X0_TAG];
		p_object->layer = LAYER_FRONTBACK;
		Pgm.Shift();
	}
	else if(Pgm.AlmostEqualsCur("y1")) {
		p_object = &grid_wall[WALL_Y1_TAG];
		p_object->layer = LAYER_FRONTBACK;
		Pgm.Shift();
	}
	else if(Pgm.AlmostEqualsCur("x1")) {
		p_object = &grid_wall[WALL_X1_TAG];
		p_object->layer = LAYER_FRONTBACK;
		Pgm.Shift();
	}
	else if(Pgm.AlmostEqualsCur("z0")) {
		p_object = &grid_wall[WALL_Z0_TAG];
		p_object->layer = LAYER_FRONTBACK;
		Pgm.Shift();
	}
	else if(Pgm.EndOfCommand()) {
		grid_wall[WALL_Y0_TAG].layer = LAYER_FRONTBACK;
		grid_wall[WALL_X0_TAG].layer = LAYER_FRONTBACK;
		grid_wall[WALL_Z0_TAG].layer = LAYER_FRONTBACK;
	}
	// Now parse the style options 
	while(p_object && !Pgm.EndOfCommand()) {
		lp_style_type lptmp;
		int save_token = Pgm.GetCurTokenIdx();
		// fill style 
		ParseFillStyle(&p_object->fillstyle);
		// fill color 
		if(Pgm.EqualsCur("fc") || Pgm.AlmostEqualsCur("fillc$olor")) {
			p_object->lp_properties.l_type = LT_BLACK; // Anything but LT_DEFAULT 
			ParseColorSpec(&p_object->lp_properties.pm3d_color, TC_RGB);
			continue;
		}
		// Line properties (for the object border if the fillstyle has one.  
		// LP_NOFILL means don't eat fillcolor here since at is set by "fc". 
		lptmp = p_object->lp_properties;
		LpParse(&lptmp, LP_NOFILL, FALSE);
		if(Pgm.GetCurTokenIdx() != save_token) {
			p_object->lp_properties.l_width = lptmp.l_width;
			p_object->lp_properties.d_type = lptmp.d_type;
			p_object->lp_properties.custom_dash_pattern = lptmp.custom_dash_pattern;
			continue;
		}
		if(Pgm.GetCurTokenIdx() == save_token)
			IntErrorCurToken("unrecognized option");
	}
}

static void set_rgbmax()
{
	GPO.Pgm.Shift();
	rgbmax = GPO.Pgm.EndOfCommand() ? 255 : GPO.RealExpression();
	if(rgbmax <= 0)
		rgbmax = 255;
}

/* process 'set samples' command */
static void set_samples()
{
	int tsamp1, tsamp2;
	GPO.Pgm.Shift();
	tsamp1 = GPO.IntExpression();
	tsamp2 = tsamp1;
	if(!GPO.Pgm.EndOfCommand()) {
		if(!GPO.Pgm.EqualsCur(","))
			GPO.IntErrorCurToken("',' expected");
		GPO.Pgm.Shift();
		tsamp2 = GPO.IntExpression();
	}
	if(tsamp1 < 2 || tsamp2 < 2)
		GPO.IntErrorCurToken("sampling rate must be > 1; sampling unchanged");
	else {
		surface_points * f_3dp = first_3dplot;
		first_3dplot = NULL;
		sp_free(f_3dp);
		samples_1 = tsamp1;
		samples_2 = tsamp2;
	}
}
//
// process 'set size' command 
//
//static void set_size()
void GnuPlot::SetSize()
{
	Pgm.Shift();
	if(Pgm.EndOfCommand()) {
		V.XSize = 1.0f;
		V.YSize = 1.0f;
	}
	else {
		if(Pgm.AlmostEqualsCur("sq$uare")) {
			V.AspectRatio = 1.0f;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("ra$tio")) {
			Pgm.Shift();
			V.AspectRatio = static_cast<float>(RealExpression());
		}
		else if(Pgm.AlmostEqualsCur("nora$tio") || Pgm.AlmostEqualsCur("nosq$uare")) {
			V.AspectRatio = 0.0f;
			Pgm.Shift();
		}
		if(!Pgm.EndOfCommand()) {
			V.XSize = static_cast<float>(RealExpression());
			if(Pgm.EqualsCur(",")) {
				Pgm.Shift();
				V.YSize = static_cast<float>(RealExpression());
			}
			else
				V.YSize = V.XSize;
		}
	}
	if(V.XSize <= 0.0f || V.YSize <=0.0f) {
		V.XSize = V.YSize = 1.0f;
		IntError(NO_CARET, "Illegal value for size");
	}
}
//
// process 'set style' command 
//
//static void set_style()
void GnuPlot::SetStyle()
{
	Pgm.Shift();
	switch(Pgm.LookupTableForCurrentToken(&show_style_tbl[0])) {
		case SHOW_STYLE_DATA:
		    data_style = get_style();
		    if(data_style == FILLEDCURVES) {
			    get_filledcurves_style_options(&filledcurves_opts_data);
			    if(filledcurves_opts_func.closeto == FILLEDCURVES_DEFAULT)
				    filledcurves_opts_data.closeto = FILLEDCURVES_CLOSED;
		    }
		    break;
		case SHOW_STYLE_FUNCTION:
	    {
		    enum PLOT_STYLE temp_style = get_style();
		    if((temp_style & PLOT_STYLE_HAS_ERRORBAR) || oneof6(temp_style, LABELPOINTS, HISTOGRAMS, IMAGE, RGBIMAGE, RGBA_IMAGE, PARALLELPLOT))
			    IntErrorCurToken("style not usable for function plots, left unchanged");
		    else
			    func_style = temp_style;
		    if(func_style == FILLEDCURVES) {
			    get_filledcurves_style_options(&filledcurves_opts_func);
			    if(filledcurves_opts_func.closeto == FILLEDCURVES_DEFAULT)
				    filledcurves_opts_func.closeto = FILLEDCURVES_CLOSED;
		    }
		    break;
	    }
		case SHOW_STYLE_LINE: SetLineStyle(&first_linestyle, LP_STYLE); break;
		case SHOW_STYLE_FILLING: ParseFillStyle(&default_fillstyle); break;
		case SHOW_STYLE_ARROW: SetArrowStyle(); break;
		case SHOW_STYLE_RECTANGLE:
		    Pgm.Shift();
		    SetObj(-2, OBJ_RECTANGLE);
		    break;
		case SHOW_STYLE_CIRCLE:
		    Pgm.Shift();
		    while(!Pgm.EndOfCommand()) {
			    if(Pgm.AlmostEqualsCur("r$adius")) {
				    Pgm.Shift();
				    GetPosition(&default_circle.o.circle.extent);
			    }
			    else if(Pgm.AlmostEqualsCur("wedge$s")) {
				    Pgm.Shift();
				    default_circle.o.circle.wedge = TRUE;
			    }
			    else if(Pgm.AlmostEqualsCur("nowedge$s")) {
				    Pgm.Shift();
				    default_circle.o.circle.wedge = FALSE;
			    }
			    else if(Pgm.EqualsCur("clip")) {
				    Pgm.Shift();
				    default_circle.clip = OBJ_CLIP;
			    }
			    else if(Pgm.EqualsCur("noclip")) {
				    Pgm.Shift();
				    default_circle.clip = OBJ_NOCLIP;
			    }
			    else
				    IntErrorCurToken("unrecognized style option");
		    }
		    break;
		case SHOW_STYLE_ELLIPSE:
		    Pgm.Shift();
		    while(!Pgm.EndOfCommand()) {
			    if(Pgm.EqualsCur("size")) {
				    Pgm.Shift();
				    GetPosition(&default_ellipse.o.ellipse.extent);
				    if(default_ellipse.o.ellipse.extent.x < 0.0)
					    default_ellipse.o.ellipse.extent.x = 0.0;
				    if(default_ellipse.o.ellipse.extent.y < 0.0)
					    default_ellipse.o.ellipse.extent.y = 0.0;
				    Pgm.Rollback();
			    }
			    else if(Pgm.AlmostEqualsCur("ang$le")) {
				    Pgm.Shift();
				    if(MightBeNumeric(Pgm.GetCurTokenIdx())) {
					    default_ellipse.o.ellipse.orientation = GPO.RealExpression();
					    Pgm.Rollback();
				    }
			    }
			    else if(Pgm.AlmostEqualsCur("unit$s")) {
				    Pgm.Shift();
				    if(Pgm.EqualsCur("xy") || Pgm.EndOfCommand())
					    default_ellipse.o.ellipse.type = ELLIPSEAXES_XY;
				    else if(Pgm.EqualsCur("xx"))
					    default_ellipse.o.ellipse.type = ELLIPSEAXES_XX;
				    else if(Pgm.EqualsCur("yy"))
					    default_ellipse.o.ellipse.type = ELLIPSEAXES_YY;
				    else
					    IntErrorCurToken("expecting 'xy', 'xx' or 'yy'");
			    }
			    else if(Pgm.EqualsCur("clip")) {
				    Pgm.Shift();
				    default_ellipse.clip = OBJ_CLIP;
			    }
			    else if(Pgm.EqualsCur("noclip")) {
				    Pgm.Shift();
				    default_ellipse.clip = OBJ_NOCLIP;
			    }
			    else
				    IntErrorCurToken("expecting 'units {xy|xx|yy}', 'angle <number>' or 'size <position>'");
			    Pgm.Shift();
		    }
		    break;
		case SHOW_STYLE_HISTOGRAM: ParseHistogramStyle(&histogram_opts, HT_CLUSTERED, histogram_opts.gap); break;
		case SHOW_STYLE_TEXTBOX:
	    {
		    textbox_style * textbox = &textbox_opts[0];
		    int tag = 0;
		    Pgm.Shift();
		    while(!Pgm.EndOfCommand()) {
			    if(Pgm.AlmostEqualsCur("op$aque")) {
				    textbox->opaque = TRUE;
				    Pgm.Shift();
			    }
			    else if(Pgm.AlmostEqualsCur("trans$parent")) {
				    textbox->opaque = FALSE;
				    Pgm.Shift();
			    }
			    else if(Pgm.AlmostEqualsCur("mar$gins")) {
				    Pgm.Shift();
				    if(Pgm.EndOfCommand()) {
					    textbox->xmargin = 1.;
					    textbox->ymargin = 1.;
					    break;
				    }
				    textbox->xmargin = GPO.RealExpression();
					SETMAX(textbox->xmargin, 0.0);
				    textbox->ymargin = textbox->xmargin;
				    if(Pgm.EqualsCur(",")) {
					    Pgm.Shift();
					    textbox->ymargin = GPO.RealExpression();
						SETMAX(textbox->ymargin, 0.0);
				    }
			    }
			    else if(Pgm.AlmostEqualsCur("fillc$olor") || Pgm.EqualsCur("fc")) {
				    ParseColorSpec(&textbox->fillcolor, TC_RGB);
			    }
			    else if(Pgm.AlmostEqualsCur("nobo$rder")) {
				    Pgm.Shift();
				    textbox->noborder = TRUE;
				    textbox->border_color.type = TC_LT;
				    textbox->border_color.lt = LT_NODRAW;
			    }
			    else if(Pgm.AlmostEqualsCur("bo$rdercolor")) {
				    Pgm.Shift();
				    textbox->noborder = FALSE;
				    textbox->border_color.type = TC_LT;
				    textbox->border_color.lt = LT_BLACK;
				    if(Pgm.EndOfCommand())
					    continue;
				    if(Pgm.EqualsCur("lt"))
					    Pgm.Rollback();
				    ParseColorSpec(&textbox->border_color, TC_RGB);
			    }
			    else if(Pgm.AlmostEqualsCur("linew$idth") || Pgm.EqualsCur("lw")) {
				    Pgm.Shift();
				    textbox->linewidth = RealExpression();
			    }
			    else if(!tag) {
				    tag = IntExpression();
				    if(tag >= NUM_TEXTBOX_STYLES)
					    IntError(NO_CARET, "only %d textbox styles supported\n", NUM_TEXTBOX_STYLES-1);
				    if(tag > 0)
					    textbox = &textbox_opts[tag];
			    }
			    else
				    IntErrorCurToken("unrecognized option");
			    // only check for tag as first option 
				SETIFZ(tag, -1);
		    }
		    // sanity checks 
		    if(textbox->linewidth <= 0)
			    textbox->linewidth = 1.0;
		    break;
	    }
		case SHOW_STYLE_INCREMENT:
		    Pgm.Shift();
		    if(Pgm.EndOfCommand() || Pgm.AlmostEqualsCur("def$ault"))
			    prefer_line_styles = FALSE;
		    else if(Pgm.AlmostEqualsCur("u$serstyles"))
			    prefer_line_styles = TRUE;
		    else
			    IntErrorCurToken("unrecognized option");
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_BOXPLOT: SetBoxPlot(); break;
		case SHOW_STYLE_PARALLEL: SetStyleParallel(); break;
		case SHOW_STYLE_SPIDERPLOT: SetStyleSpiderPlot(); break;
		default:
		    IntErrorCurToken("unrecognized option - see 'help set style'");
	}
}
//
// process 'set surface' command 
//
//static void set_surface()
void GnuPlot::SetSurface()
{
	Pgm.Shift();
	draw_surface = TRUE;
	implicit_surface = TRUE;
	if(!Pgm.EndOfCommand()) {
		if(Pgm.EqualsCur("implicit"))
			;
		else if(Pgm.EqualsCur("explicit"))
			implicit_surface = FALSE;
		Pgm.Shift();
	}
}
//
// process 'set table' command 
//
//static void set_table()
void GnuPlot::SetTable()
{
	char * tablefile;
	Pgm.Shift();
	int filename_token = Pgm.GetCurTokenIdx();
	bool append = FALSE;
	SFile::ZClose(&table_outfile);
	table_var = NULL;
	if(Pgm.EqualsCur("$") && Pgm.IsLetter(Pgm.GetCurTokenIdx()+1)) { /* datablock */
		// NB: has to come first because TryToGetString will choke on the datablock name 
		table_var = Ev.AddUdvByName(Pgm.ParseDatablockName());
		if(table_var == NULL)
			IntErrorCurToken("Error allocating datablock");
		if(Pgm.EqualsCur("append")) {
			Pgm.Shift();
			append = TRUE;
		}
		if(!append || table_var->udv_value.type != DATABLOCK) {
			table_var->udv_value.Destroy();
			table_var->udv_value.type = DATABLOCK;
			table_var->udv_value.v.data_array = NULL;
		}
	}
	else if((tablefile = TryToGetString())) { /* file name */
		// 'set table "foo"' creates a new output file 
		// 'set table "foo" append' writes to the end of an existing output file 
		gp_expand_tilde(&tablefile);
		if(Pgm.EqualsCur("append")) {
			Pgm.Shift();
			append = TRUE;
		}
		if(!(table_outfile = fopen(tablefile, (append ? "a" : "w"))))
			os_error(filename_token, "cannot open table output file");
		SAlloc::F(tablefile);
	}
	if(Pgm.AlmostEqualsCur("sep$arator")) {
		SetSeparator(&table_sep);
	}
	table_mode = TRUE;
}
//
// process 'set terminal' command 
//
//static void set_terminal()
void GnuPlot::SetTerminal()
{
	Pgm.Shift();
	if(multiplot)
		IntErrorCurToken("You can't change the terminal in multiplot mode");
	if(Pgm.EndOfCommand()) {
		list_terms();
		screen_ok = FALSE;
		return;
	}
	// `set term push' 
	if(Pgm.EqualsCur("push")) {
		push_terminal(interactive);
		Pgm.Shift();
		return;
	} // set term push 
#ifdef USE_MOUSE
	event_reset(reinterpret_cast<gp_event_t *>(1)); /* cancel zoombox etc. */
#endif
	term_reset();
	// `set term pop' 
	if(Pgm.EqualsCur("pop")) {
		pop_terminal();
		Pgm.Shift();
		return;
	} /* set term pop */
	// `set term <normal terminal>' 
	// NB: if set_term() exits via IntError() then term will not be changed 
	term = SetTerm();
	// get optional mode parameters
	// not all drivers reset the option string before
	// strcat-ing to it, so we reset it for them
	*term_options = 0;
	term->options();
	if(interactive && *term_options)
		fprintf(stderr, "Options are '%s'\n", term_options);
	if((term->flags & TERM_MONOCHROME))
		init_monochrome();
	// Sanity check:
	// The most common failure mode found by fuzzing is a divide-by-zero
	// caused by initializing the basic unit of the current terminal character
	// size to zero.  I keep patching the individual terminals, but a generic
	// sanity check may at least prevent a crash due to mistyping.
	if(term->ChrH <= 0 || term->ChrV <= 0) {
		IntWarn(NO_CARET, "invalid terminal font size");
		term->ChrH = 10;
		term->ChrV = 10;
	}
}
// 
// Accept a single terminal option to apply to the current terminal if possible.
// If the current terminal cannot support pThis option, we silently ignore it.
// Only reasonably common terminal options are supported.
// 
// If necessary, the code in term->options() can detect that it was called
// from here because in pThis case almost_equals(c_token-1, "termopt$ion");
// 
//static void set_termoptions()
void GnuPlot::SetTermOptions()
{
	bool  ok_to_call_terminal = FALSE;
	const int save_end_of_line = Pgm.NumTokens;
	Pgm.Shift();
	if(Pgm.EndOfCommand() || !term)
		return;
	if(Pgm.AlmostEqualsCur("enh$anced") || Pgm.AlmostEqualsCur("noenh$anced")) {
		Pgm.NumTokens = MIN(Pgm.NumTokens, Pgm.GetCurTokenIdx()+1);
		if(term->enhanced_open)
			ok_to_call_terminal = TRUE;
		else
			Pgm.Shift();
	}
	else if(Pgm.EqualsCur("font") || Pgm.EqualsCur("fname")) {
		Pgm.NumTokens = MIN(Pgm.NumTokens, Pgm.GetCurTokenIdx()+2);
		ok_to_call_terminal = TRUE;
	}
	else if(Pgm.EqualsCur("fontscale")) {
		Pgm.NumTokens = MIN(Pgm.NumTokens, Pgm.GetCurTokenIdx()+2);
		if(term->flags & TERM_FONTSCALE)
			ok_to_call_terminal = TRUE;
		else {
			Pgm.Shift();
			RealExpression(); /* Silently ignore the request */
		}
	}
	else if(Pgm.EqualsCur("pointscale") || Pgm.EqualsCur("ps")) {
		Pgm.NumTokens = MIN(Pgm.NumTokens, Pgm.GetCurTokenIdx()+2);
		if(term->flags & TERM_POINTSCALE)
			ok_to_call_terminal = TRUE;
		else {
			Pgm.Shift();
			RealExpression(); /* Silently ignore the request */
		}
	}
	else if(Pgm.EqualsCur("lw") || Pgm.AlmostEqualsCur("linew$idth")) {
		Pgm.NumTokens = MIN(Pgm.NumTokens, Pgm.GetCurTokenIdx()+2);
		if(term->flags & TERM_LINEWIDTH)
			ok_to_call_terminal = TRUE;
		else {
			Pgm.Shift();
			RealExpression(); /* Silently ignore the request */
		}
	}
	else if(Pgm.AlmostEqualsCur("dash$ed") || Pgm.EqualsCur("solid")) {
		Pgm.NumTokens = MIN(Pgm.NumTokens, ++Pgm.CToken); // Silently ignore the request 
	}
	else if(Pgm.AlmostEqualsCur("dashl$ength") || Pgm.EqualsCur("dl")) {
		Pgm.NumTokens = MIN(Pgm.NumTokens, Pgm.GetCurTokenIdx()+2);
		if(term->flags & TERM_CAN_DASH)
			ok_to_call_terminal = TRUE;
		else {
			Pgm.Shift();
			Pgm.Shift();
		}
	}
	else if(!strcmp(term->name, "gif") && Pgm.EqualsCur("delay") && Pgm.NumTokens == 4) {
		ok_to_call_terminal = TRUE;
	}
	else {
		IntErrorCurToken("This option cannot be changed using 'set termoption'");
	}
	if(ok_to_call_terminal) {
		*term_options = 0;
		(term->options)();
	}
	Pgm.NumTokens = save_end_of_line;
}
//
// Various properties of the theta axis in polar mode 
//
//static void set_theta()
void GnuPlot::SetTheta()
{
	Pgm.Shift();
	while(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("r$ight"))
			theta_origin = 0.0;
		else if(Pgm.AlmostEqualsCur("t$op"))
			theta_origin = 90.0;
		else if(Pgm.AlmostEqualsCur("l$eft"))
			theta_origin = 180.0;
		else if(Pgm.AlmostEqualsCur("b$ottom"))
			theta_origin = -90.;
		else if(Pgm.EqualsCur("clockwise") || Pgm.EqualsCur("cw"))
			theta_direction = -1;
		else if(Pgm.EqualsCur("counterclockwise") || Pgm.EqualsCur("ccw"))
			theta_direction = 1;
		else
			IntErrorCurToken("unrecognized option");
		Pgm.Shift();
	}
}
//
// process 'set tics' command 
//
//static void set_tics()
void GnuPlot::SetTics()
{
	int    i;
	bool   global_opt = FALSE;
	int    save_token = Pgm.GetCurTokenIdx();
	// There are a few options that set_tic_prop doesn't handle
	// because they are global rather than per-axis.
	while(!Pgm.EndOfCommand()) {
		if(Pgm.EqualsCur("front")) {
			grid_tics_in_front = TRUE;
			global_opt = TRUE;
		}
		else if(Pgm.EqualsCur("back")) {
			grid_tics_in_front = FALSE;
			global_opt = TRUE;
		}
		else if(Pgm.AlmostEqualsCur("sc$ale")) {
			SetTicScale();
			global_opt = TRUE;
		}
		Pgm.Shift();
	}
	// Otherwise we iterate over axes and apply the options to each 
	for(i = 0; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++) {
		Pgm.SetTokenIdx(save_token);
		SetTicProp(&AxS[i]);
	}
	// if tics are off, reset to default (border) 
	if(Pgm.EndOfCommand() || global_opt) {
		for(i = 0; i < NUMBER_OF_MAIN_VISIBLE_AXES; ++i) {
			if((AxS[i].ticmode & TICS_MASK) == NO_TICS) {
				if(oneof2(i, SECOND_X_AXIS, SECOND_Y_AXIS))
					continue; /* don't switch on secondary axes by default */
				AxS[i].ticmode = TICS_ON_BORDER;
				if(oneof3(i, FIRST_X_AXIS, FIRST_Y_AXIS, COLOR_AXIS))
					AxS[i].ticmode |= TICS_MIRROR;
			}
		}
	}
}
//
// process 'set ticscale' command 
//
//static void set_ticscale()
void GnuPlot::SetTicScale()
{
	Pgm.Shift();
	if(Pgm.AlmostEqualsCur("def$ault")) {
		Pgm.Shift();
		for(int i = 0; i < AXIS_ARRAY_SIZE; ++i) {
			AxS[i].ticscale = 1.0;
			AxS[i].miniticscale = 0.5;
		}
		ticscale[0] = 1.0;
		ticscale[1] = 0.5;
		for(int ticlevel = 2; ticlevel < MAX_TICLEVEL; ticlevel++)
			ticscale[ticlevel] = 1.0;
	}
	else {
		double lminiticscale;
		double lticscale = RealExpression();
		if(Pgm.EqualsCur(",")) {
			Pgm.Shift();
			lminiticscale = RealExpression();
		}
		else {
			lminiticscale = 0.5 * lticscale;
		}
		for(int i = 0; i < NUMBER_OF_MAIN_VISIBLE_AXES; ++i) {
			AxS[i].ticscale = lticscale;
			AxS[i].miniticscale = lminiticscale;
		}
		for(int ticlevel = 2; Pgm.EqualsCur(",");) {
			Pgm.Shift();
			ticscale[ticlevel++] = RealExpression();
			if(ticlevel >= MAX_TICLEVEL)
				break;
		}
	}
}
// 
// process 'set ticslevel' command 
// is datatype 'time' relevant here ? 
// 
static void set_ticslevel()
{
	GPO.Pgm.Shift();
	xyplane.z = GPO.RealExpression();
	xyplane.absolute = FALSE;
}
// 
// process 'set xyplane' command 
// is datatype 'time' relevant here ? 
// 
static void set_xyplane()
{
	GPO.Pgm.Shift();
	if(GPO.Pgm.EqualsCur("at")) {
		GPO.Pgm.Shift();
		xyplane.z = GPO.RealExpression();
		xyplane.absolute = TRUE;
		return;
	}
	else if(!GPO.Pgm.AlmostEqualsCur("rel$ative")) { // deprecated syntax 
		GPO.Pgm.Rollback();
	}
	set_ticslevel();
}
//
// Process 'set P_TimeFormat' command */
// V5: fallback default if timecolumn(N,"format") not used during input.
// Use "set {axis}tics format" to control the output format.
//
static void set_timefmt()
{
	char * ctmp;
	GPO.Pgm.Shift();
	if((ctmp = GPO.TryToGetString())) {
		SAlloc::F(P_TimeFormat);
		P_TimeFormat = ctmp;
	}
	else {
		SAlloc::F(P_TimeFormat);
		P_TimeFormat = gp_strdup(TIMEFMT);
	}
}
//
// process 'set timestamp' command 
//
//static void set_timestamp()
void GnuPlot::SetTimeStamp()
{
	bool got_format = FALSE;
	char * p_new;
	Pgm.Shift();
	while(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("t$op")) {
			timelabel_bottom = FALSE;
			Pgm.Shift();
			continue;
		}
		else if(Pgm.AlmostEqualsCur("b$ottom")) {
			timelabel_bottom = TRUE;
			Pgm.Shift();
			continue;
		}
		if(Pgm.AlmostEqualsCur("r$otate")) {
			timelabel.rotate = TEXT_VERTICAL;
			Pgm.Shift();
			continue;
		}
		else if(Pgm.AlmostEqualsCur("n$orotate")) {
			timelabel.rotate = 0;
			Pgm.Shift();
			continue;
		}
		if(Pgm.AlmostEqualsCur("off$set")) {
			Pgm.Shift();
			GetPositionDefault(&(timelabel.offset), character, 3);
			continue;
		}
		if(Pgm.EqualsCur("font")) {
			Pgm.Shift();
			p_new = TryToGetString();
			SAlloc::F(timelabel.font);
			timelabel.font = p_new;
			continue;
		}
		if(Pgm.EqualsCur("tc") || Pgm.AlmostEqualsCur("text$color")) {
			ParseColorSpec(&(timelabel.textcolor), TC_VARIABLE);
			continue;
		}
		if(!got_format && ((p_new = TryToGetString()))) {
			// we have a format string 
			SAlloc::F(timelabel.text);
			timelabel.text = p_new;
			got_format = TRUE;
			continue;
		}
		IntErrorCurToken("unrecognized option");
	}
	SETIFZ(timelabel.text, gp_strdup(DEFAULT_TIMESTAMP_FORMAT));
	timelabel.pos = (timelabel.rotate && !timelabel_bottom) ? RIGHT : LEFT;
}
//
// process 'set view' command 
//
//static void set_view()
void GnuPlot::SetView()
{
	int i;
	bool was_comma = TRUE;
	static const char errmsg1[] = "rot_%c must be in [0:360] degrees range; view unchanged";
	static const char errmsg2[] = "%sscale must be > 0; view unchanged";
	double local_vals[4];
	Pgm.Shift();
	// 'set view map' establishes projection onto the xy plane 
	if(Pgm.EqualsCur("map") || (Pgm.AlmostEqualsCur("proj$ection") && Pgm.EqualsNext("xy"))) {
		splot_map = TRUE;
		xz_projection = yz_projection = FALSE;
		mapview_scale = 1.0;
		azimuth = 0;
		if(Pgm.AlmostEqualsCur("proj$ection"))
			Pgm.Shift();
		Pgm.Shift();
		if(Pgm.EqualsCur("scale")) {
			Pgm.Shift();
			mapview_scale = static_cast<float>(RealExpression());
		}
		if(V.AspectRatio3D != 0) {
			V.AspectRatio = -1.0f;
			V.AspectRatio3D = 0;
		}
		return;
	}
	if(splot_map)
		splot_map = false; // default is no map 
	// 'set view projection {xz|yz} establishes projection onto xz or yz plane 
	if(Pgm.AlmostEqualsCur("proj$ection")) {
		Pgm.Shift();
		xz_projection = yz_projection = FALSE;
		if(Pgm.EqualsCur("xz"))
			xz_projection = TRUE;
		else if(Pgm.EqualsCur("yz"))
			yz_projection = TRUE;
		else
			IntErrorCurToken("expecting xy or xz or yz");
		Pgm.Shift();
		// FIXME: should these be deferred to do_3dplot()? 
		xyplane.z = 0.0;
		xyplane.absolute = FALSE;
		azimuth = -90;
		AxS[FIRST_Z_AXIS].tic_pos = CENTRE;
		AxS[FIRST_Z_AXIS].manual_justify = TRUE;
		return;
	}
	if(Pgm.AlmostEqualsCur("equal$_axes")) {
		Pgm.Shift();
		if(Pgm.EndOfCommand() || Pgm.EqualsCur("xy")) {
			V.AspectRatio3D = 2;
			Pgm.Shift();
		}
		else if(Pgm.EqualsCur("xyz")) {
			V.AspectRatio3D = 3;
			Pgm.Shift();
		}
		return;
	}
	else if(Pgm.AlmostEqualsCur("noequal$_axes")) {
		// FIXME: set aspect_ratio = 0 ?? 
		V.AspectRatio3D = 0;
		Pgm.Shift();
		return;
	}
	if(Pgm.EqualsCur("azimuth")) {
		Pgm.Shift();
		azimuth = static_cast<float>(RealExpression());
		return;
	}
	local_vals[0] = surface_rot_x;
	local_vals[1] = surface_rot_z;
	local_vals[2] = surface_scale;
	local_vals[3] = surface_zscale;
	for(i = 0; i < 4 && !(Pgm.EndOfCommand());) {
		if(Pgm.EqualsCur(",")) {
			if(was_comma) i++;
			was_comma = TRUE;
			Pgm.Shift();
		}
		else {
			if(!was_comma)
				IntErrorCurToken("',' expected");
			local_vals[i] = RealExpression();
			i++;
			was_comma = FALSE;
		}
	}
	if(local_vals[0] < 0 || local_vals[0] > 360)
		IntErrorCurToken(errmsg1, 'x');
	if(local_vals[1] < 0 || local_vals[1] > 360)
		IntErrorCurToken(errmsg1, 'z');
	if(local_vals[2] < 1e-6)
		IntErrorCurToken(errmsg2, "");
	if(local_vals[3] < 1e-6)
		IntErrorCurToken(errmsg2, "z");
	xz_projection = yz_projection = FALSE;
	surface_rot_x = static_cast<float>(local_vals[0]);
	surface_rot_z = static_cast<float>(local_vals[1]);
	surface_scale = static_cast<float>(local_vals[2]);
	surface_zscale = static_cast<float>(local_vals[3]);
	surface_lscale = logf(surface_scale);
}
//
// process 'set zero' command 
//
static void set_zero()
{
	GPO.Pgm.Shift();
	zero = GPO.RealExpression();
}
//
// process 'set {x|y|z|x2|y2}data' command 
//
//static void set_timedata(GpAxis * pAx)
void GnuPlot::SetTimeData(GpAxis * pAx)
{
	Pgm.Shift();
	pAx->datatype = DT_NORMAL;
	if(Pgm.AlmostEqualsCur("t$ime")) {
		pAx->datatype = DT_TIMEDATE;
		Pgm.Shift();
	}
	else if(Pgm.AlmostEqualsCur("geo$graphic")) {
		pAx->datatype = DT_DMS;
		Pgm.Shift();
	}
	// FIXME: pThis provides approximate backwards compatibility 
	//        but may be more trouble to explain than it's worth 
	pAx->tictype = pAx->datatype;
}

//static void set_range(GpAxis * pAx)
void GnuPlot::SetRange(GpAxis * pAx)
{
	Pgm.Shift();
	if(Pgm.AlmostEqualsCur("re$store")) {
		Pgm.Shift();
		pAx->set_min = pAx->writeback_min;
		pAx->set_max = pAx->writeback_max;
		pAx->set_autoscale = AUTOSCALE_NONE;
	}
	else {
		if(Pgm.EqualsCur("[")) {
			Pgm.Shift();
			pAx->set_autoscale = LoadRange(pAx, &pAx->set_min, &pAx->set_max, pAx->set_autoscale);
			if(!Pgm.EqualsCur("]"))
				IntErrorCurToken("expecting ']'");
			Pgm.Shift();
		}
		while(!Pgm.EndOfCommand()) {
			if(Pgm.AlmostEqualsCur("rev$erse")) {
				Pgm.Shift();
				pAx->range_flags |= RANGE_IS_REVERSED;
			}
			else if(Pgm.AlmostEqualsCur("norev$erse")) {
				Pgm.Shift();
				pAx->range_flags &= ~RANGE_IS_REVERSED;
			}
			else if(Pgm.AlmostEqualsCur("wr$iteback")) {
				Pgm.Shift();
				pAx->range_flags |= RANGE_WRITEBACK;
			}
			else if(Pgm.AlmostEqualsCur("nowri$teback")) {
				Pgm.Shift();
				pAx->range_flags &= ~RANGE_WRITEBACK;
			}
			else if(Pgm.AlmostEqualsCur("ext$end")) {
				Pgm.Shift();
				pAx->set_autoscale &= ~(AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX);
			}
			else if(Pgm.AlmostEqualsCur("noext$end")) {
				Pgm.Shift();
				pAx->set_autoscale |= AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX;
			}
			else
				IntErrorCurToken("unrecognized option");
		}
	}
	// If pThis is one end of a linked axis pair, replicate the new range to the
	// linked axis, possibly via a mapping function.
	if(pAx->linked_to_secondary)
		CloneLinkedAxes(pAx, pAx->linked_to_secondary);
	else if(pAx->linked_to_primary)
		CloneLinkedAxes(pAx, pAx->linked_to_primary);
}
/*
 * set paxis <axis> {range <range-options>}
 *                  {tics <tic-options>}
 *                  {label <label options>} (only used for spiderplots)
 *                  {<lp-options>} (only used for spiderplots)
 */
//static void set_paxis()
void GnuPlot::SetPAxis()
{
	Pgm.Shift();
	int p = IntExpression();
	if(p <= 0)
		IntError(Pgm.GetPrevTokenIdx(), "illegal paxis");
	if(p > AxS.GetParallelAxisCount())
		AxS.ExtendParallelAxis(p);
	while(!Pgm.EndOfCommand()) {
		if(Pgm.EqualsCur("range"))
			SetRange(&AxS.Parallel(p-1));
		else if(Pgm.AlmostEqualsCur("tic$s"))
			SetTicProp(&AxS.Parallel(p-1));
		else if(Pgm.EqualsCur("label"))
			set_xyzlabel(&AxS.Parallel(p-1).label);
		else {
			int save_token = Pgm.GetCurTokenIdx();
			lp_style_type axis_line(lp_style_type::defCommon);// = DEFAULT_LP_STYLE_TYPE;
			LpParse(&axis_line, LP_ADHOC, FALSE);
			if(Pgm.GetCurTokenIdx() != save_token) {
				SAlloc::F(AxS.Parallel(p-1).zeroaxis);
				AxS.Parallel(p-1).zeroaxis = (lp_style_type *)gp_alloc(sizeof(lp_style_type), NULL);
				memcpy(AxS.Parallel(p-1).zeroaxis, &axis_line, sizeof(lp_style_type));
			}
			else
				IntErrorCurToken("expecting 'range' or 'tics' or 'label'");
		}
	}
}

static void set_raxis()
{
	raxis = TRUE;
	GPO.Pgm.Shift();
}
//
// process 'set {xyz}zeroaxis' command 
//
//static void set_zeroaxis(AXIS_INDEX axis)
void GnuPlot::SetZeroAxis(AXIS_INDEX axis)
{
	Pgm.Shift();
	if(AxS[axis].zeroaxis != (void*)(&default_axis_zeroaxis))
		SAlloc::F(AxS[axis].zeroaxis);
	if(Pgm.EndOfCommand())
		AxS[axis].zeroaxis = (lp_style_type *)(&default_axis_zeroaxis);
	else {
		// Some non-default style for the zeroaxis 
		AxS[axis].zeroaxis = (lp_style_type *)gp_alloc(sizeof(lp_style_type), "zeroaxis");
		*(AxS[axis].zeroaxis) = default_axis_zeroaxis;
		LpParse(AxS[axis].zeroaxis, LP_ADHOC, FALSE);
	}
}
//
// process 'set zeroaxis' command 
//
//static void set_allzeroaxis()
void GnuPlot::SetAllZeroAxis()
{
	int save_token = Pgm.GetCurTokenIdx();
	SetZeroAxis(FIRST_X_AXIS);
	Pgm.SetTokenIdx(save_token);
	SetZeroAxis(FIRST_Y_AXIS);
	Pgm.SetTokenIdx(save_token);
	SetZeroAxis(FIRST_Z_AXIS);
}
//
// Implements 'set tics' 'set xtics' 'set ytics' etc 
//
//static void set_tic_prop(GpAxis * pThisAxis)
void GnuPlot::SetTicProp(GpAxis * pThisAxis)
{
	bool   all_axes = FALSE;  /* distinguish the global command "set tics" */
	char   nocmd[12];         /* fill w/ "no"+axis_name+suffix */
	char * cmdptr = NULL, * sfxptr = NULL;
	AXIS_INDEX axis = (AXIS_INDEX)pThisAxis->index;
	if(Pgm.AlmostEqualsCur("tic$s") && (axis < PARALLEL_AXES))
		all_axes = TRUE;
	if(axis < NUMBER_OF_MAIN_VISIBLE_AXES) {
		strcpy(nocmd, "no");
		cmdptr = &nocmd[2];
		strcpy(cmdptr, axis_name(axis));
		sfxptr = &nocmd[strlen(nocmd)];
		strcpy(sfxptr, "t$ics"); /* STRING */
	}
	if(axis == AxS.Theta().index)
		cmdptr = "ttics";
	// This loop handles all cases except "set no{axisname}" 
	if(Pgm.AlmostEqualsCur(cmdptr) || all_axes || axis >= PARALLEL_AXES) {
		bool axisset = FALSE;
		bool mirror_opt = FALSE; /* set to true if (no)mirror option specified) */
		pThisAxis->ticdef.def.mix = FALSE;
		Pgm.Shift();
		do {
			if(Pgm.AlmostEqualsCur("ax$is")) {
				axisset = TRUE;
				pThisAxis->ticmode &= ~TICS_ON_BORDER;
				pThisAxis->ticmode |= TICS_ON_AXIS;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("bo$rder")) {
				pThisAxis->ticmode &= ~TICS_ON_AXIS;
				pThisAxis->ticmode |= TICS_ON_BORDER;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("mi$rror")) {
				pThisAxis->ticmode |= TICS_MIRROR;
				mirror_opt = true;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("nomi$rror")) {
				pThisAxis->ticmode &= ~TICS_MIRROR;
				mirror_opt = true;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("in$wards")) {
				pThisAxis->TicIn = true;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("out$wards")) {
				pThisAxis->TicIn = false;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("sc$ale")) {
				Pgm.Shift();
				if(Pgm.AlmostEqualsCur("def$ault")) {
					pThisAxis->ticscale = 1.0;
					pThisAxis->miniticscale = 0.5;
					Pgm.Shift();
				}
				else {
					pThisAxis->ticscale = GPO.RealExpression();
					if(Pgm.EqualsCur(",")) {
						Pgm.Shift();
						pThisAxis->miniticscale = GPO.RealExpression();
					}
					else
						pThisAxis->miniticscale = 0.5 * pThisAxis->ticscale;
					// Global "set tics scale" allows additional levels 
					if(all_axes) {
						while(Pgm.EqualsCur(",")) {
							Pgm.Shift();
							GPO.RealExpression();
						}
					}
				}
			}
			else if(Pgm.AlmostEqualsCur("ro$tate")) {
				pThisAxis->tic_rotate = TEXT_VERTICAL;
				Pgm.Shift();
				if(Pgm.EqualsCur("by")) {
					Pgm.Shift();
					pThisAxis->tic_rotate = IntExpression();
				}
			}
			else if(Pgm.AlmostEqualsCur("noro$tate")) {
				pThisAxis->tic_rotate = 0;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("off$set")) {
				Pgm.Shift();
				GetPositionDefault(&pThisAxis->ticdef.offset, character, 3);
			}
			else if(Pgm.AlmostEqualsCur("nooff$set")) {
				Pgm.Shift();
				pThisAxis->ticdef.offset = default_offset;
			}
			else if(Pgm.AlmostEqualsCur("l$eft")) {
				pThisAxis->tic_pos = LEFT;
				pThisAxis->manual_justify = TRUE;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("c$entre") || Pgm.AlmostEqualsCur("c$enter")) {
				pThisAxis->tic_pos = CENTRE;
				pThisAxis->manual_justify = TRUE;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("ri$ght")) {
				pThisAxis->tic_pos = RIGHT;
				pThisAxis->manual_justify = TRUE;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("autoj$ustify")) {
				pThisAxis->manual_justify = FALSE;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("range$limited")) {
				pThisAxis->ticdef.rangelimited = TRUE;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("norange$limited")) {
				pThisAxis->ticdef.rangelimited = FALSE;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("f$ont")) {
				Pgm.Shift();
				// Make sure they've specified a font 
				if(!Pgm.IsStringValue(Pgm.GetCurTokenIdx()))
					IntErrorCurToken("expected font");
				else {
					ZFREE(pThisAxis->ticdef.font);
					pThisAxis->ticdef.font = TryToGetString();
				}
				// The geographic/timedate/numeric options are new in version 5 
			}
			else if(Pgm.AlmostEqualsCur("geo$graphic")) {
				Pgm.Shift();
				pThisAxis->tictype = DT_DMS;
			}
			else if(Pgm.AlmostEqualsCur("time$date")) {
				Pgm.Shift();
				pThisAxis->tictype = DT_TIMEDATE;
			}
			else if(Pgm.AlmostEqualsCur("numeric")) {
				Pgm.Shift();
				pThisAxis->tictype = DT_NORMAL;
			}
			else if(Pgm.EqualsCur("format")) {
				char * format;
				Pgm.Shift();
				if(Pgm.EndOfCommand())
					format = gp_strdup(DEF_FORMAT);
				else if(!((format = TryToGetString())))
					IntErrorCurToken("expected format");
				SAlloc::F(pThisAxis->formatstring);
				pThisAxis->formatstring  = format;
			}
			else if(Pgm.AlmostEqualsCur("enh$anced")) {
				Pgm.Shift();
				pThisAxis->ticdef.enhanced = TRUE;
			}
			else if(Pgm.AlmostEqualsCur("noenh$anced")) {
				Pgm.Shift();
				pThisAxis->ticdef.enhanced = FALSE;
			}
			else if(Pgm.EqualsCur("tc") || Pgm.AlmostEqualsCur("text$color")) {
				ParseColorSpec(&pThisAxis->ticdef.textcolor, axis == FIRST_Z_AXIS ? TC_Z : TC_FRAC);
			}
			else if(Pgm.AlmostEqualsCur("au$tofreq")) {
				// auto tic interval 
				Pgm.Shift();
				if(!pThisAxis->ticdef.def.mix) {
					free_marklist(pThisAxis->ticdef.def.user);
					pThisAxis->ticdef.def.user = NULL;
				}
				pThisAxis->ticdef.type = TIC_COMPUTED;
			}
			else if(Pgm.AlmostEqualsCur("log$scale")) {
				Pgm.Shift();
				pThisAxis->ticdef.logscaling = TRUE;
			}
			else if(Pgm.AlmostEqualsCur("nolog$scale")) {
				Pgm.Shift();
				pThisAxis->ticdef.logscaling = FALSE;
			}
			else if(Pgm.EqualsCur("add")) {
				Pgm.Shift();
				pThisAxis->ticdef.def.mix = TRUE;
			}
			else if(all_axes && (Pgm.EqualsCur("front") || Pgm.EqualsCur("back"))) {
				// only relevant to global command set_tics() and will be applied there 
				Pgm.Shift();
			}
			else if(!Pgm.EndOfCommand()) {
				load_tics(pThisAxis);
			}
		} while(!Pgm.EndOfCommand());
		// "set tics" will take care of restoring proper defaults 
		if(all_axes)
			return;
		// if tics are off and not set by axis, reset to default (border) 
		if(((pThisAxis->ticmode & TICS_MASK) == NO_TICS) && (!axisset)) {
			if(axis >= PARALLEL_AXES)
				pThisAxis->ticmode |= TICS_ON_AXIS;
			else
				pThisAxis->ticmode |= TICS_ON_BORDER;
			if((mirror_opt == FALSE) && ((axis == FIRST_X_AXIS) || (axis == FIRST_Y_AXIS) || (axis == COLOR_AXIS))) {
				pThisAxis->ticmode |= TICS_MIRROR;
			}
		}
	}
	/* The remaining command options cannot work for parametric or parallel axes */
	if(axis >= NUMBER_OF_MAIN_VISIBLE_AXES)
		return;
	if(Pgm.AlmostEqualsCur(nocmd)) {     /* NOSTRING */
		pThisAxis->ticmode &= ~TICS_MASK;
		Pgm.Shift();
	}
/* other options */
	strcpy(sfxptr, "m$tics"); /* MONTH */
	if(Pgm.AlmostEqualsCur(cmdptr)) {
		if(!pThisAxis->ticdef.def.mix) {
			free_marklist(pThisAxis->ticdef.def.user);
			pThisAxis->ticdef.def.user = NULL;
		}
		pThisAxis->ticdef.type = TIC_MONTH;
		Pgm.Shift();
	}
	if(Pgm.AlmostEqualsCur(nocmd)) {     /* NOMONTH */
		pThisAxis->ticdef.type = TIC_COMPUTED;
		Pgm.Shift();
	}
	strcpy(sfxptr, "d$tics"); /* DAYS */
	if(Pgm.AlmostEqualsCur(cmdptr)) {
		if(!pThisAxis->ticdef.def.mix) {
			free_marklist(pThisAxis->ticdef.def.user);
			pThisAxis->ticdef.def.user = NULL;
		}
		pThisAxis->ticdef.type = TIC_DAY;
		Pgm.Shift();
	}
	if(Pgm.AlmostEqualsCur(nocmd)) {     /* NODAYS */
		pThisAxis->ticdef.type = TIC_COMPUTED;
		Pgm.Shift();
	}
	*cmdptr = 'm';
	strcpy(cmdptr + 1, axis_name(axis));
	strcat(cmdptr, "t$ics");  /* MINISTRING */
	if(Pgm.AlmostEqualsCur(cmdptr)) {
		Pgm.Shift();
		if(Pgm.EndOfCommand()) {
			pThisAxis->minitics = MINI_AUTO;
		}
		else if(Pgm.AlmostEqualsCur("def$ault")) {
			pThisAxis->minitics = MINI_DEFAULT;
			Pgm.Shift();
		}
		else {
			int freq = IntExpression();
			if(freq > 0 && freq < 101) {
				pThisAxis->mtic_freq = freq;
				pThisAxis->minitics = MINI_USER;
			}
			else {
				pThisAxis->minitics = MINI_DEFAULT;
				IntWarn(Pgm.GetPrevTokenIdx(), "Expecting number of intervals");
			}
		}
	}
	if(Pgm.AlmostEqualsCur(nocmd)) { // NOMINI
		pThisAxis->minitics = MINI_OFF;
		Pgm.Shift();
	}
}
/*
 * minor tics around perimeter of polar grid circle (theta).
 * This version works like other axes (parameter is # of subintervals)
 * but it might be more reasonable to simply take increment in degrees.
 */
static void set_mttics(GpAxis * this_axis)
{
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand()) {
		this_axis->minitics = MINI_AUTO;
		GPO.Pgm.Shift();
	}
	else {
		int freq = GPO.IntExpression();
		if(freq > 0 && freq < 361) {
			this_axis->mtic_freq = freq;
			this_axis->minitics = MINI_USER;
		}
		else {
			this_axis->minitics = MINI_AUTO;
			GPO.IntWarn(GPO.Pgm.GetPrevTokenIdx(), "Expecting number of intervals");
		}
	}
}

/* process a 'set {x/y/z}label command */
/* set {x/y/z}label {label_text} {offset {x}{,y}} {<fontspec>} {<textcolor>} */
static void set_xyzlabel(text_label * label)
{
	GPO.Pgm.Shift();
	if(GPO.Pgm.EndOfCommand()) { // no label specified 
		ZFREE(label->text);
	}
	else {
		GPO.ParseLabelOptions(label, 0);
		if(!GPO.Pgm.EndOfCommand()) {
			char * text = GPO.TryToGetString();
			if(text) {
				SAlloc::F(label->text);
				label->text = text;
			}
		}
		GPO.ParseLabelOptions(label, 0);
	}
}
// 
// Change or insert a new linestyle in a list of line styles.
// Supports the old 'set linestyle' command (backwards-compatible)
// and the new "set style line" and "set linetype" commands.
// destination_class is either LP_STYLE or LP_TYPE.
// 
//static void set_linestyle(linestyle_def ** head, lp_class destination_class)
void GnuPlot::SetLineStyle(linestyle_def ** ppHead, lp_class destinationClass)
{
	linestyle_def * this_linestyle = NULL;
	linestyle_def * new_linestyle = NULL;
	linestyle_def * prev_linestyle = NULL;
	int tag;
	Pgm.Shift();
	// get tag 
	if(Pgm.EndOfCommand() || ((tag = IntExpression()) <= 0))
		IntErrorCurToken("tag must be > zero");
	// Check if linestyle is already defined 
	for(this_linestyle = *ppHead; this_linestyle != NULL; prev_linestyle = this_linestyle, this_linestyle = this_linestyle->next) {
		if(tag <= this_linestyle->tag)
			break;
	}
	if(this_linestyle == NULL || tag != this_linestyle->tag) {
		// Default style is based on linetype with the same tag id 
		lp_style_type loc_lp(lp_style_type::defCommon); // = DEFAULT_LP_STYLE_TYPE;
		loc_lp.l_type = tag - 1;
		loc_lp.p_type = tag - 1;
		loc_lp.d_type = DASHTYPE_SOLID;
		loc_lp.pm3d_color.type = TC_LT;
		loc_lp.pm3d_color.lt = tag - 1;
		new_linestyle = (linestyle_def *)gp_alloc(sizeof(linestyle_def), "linestyle");
		if(prev_linestyle != NULL)
			prev_linestyle->next = new_linestyle; /* add it to end of list */
		else
			*ppHead = new_linestyle; /* make it start of list */
		new_linestyle->tag = tag;
		new_linestyle->next = this_linestyle;
		new_linestyle->lp_properties = loc_lp;
		this_linestyle = new_linestyle;
	}
	if(Pgm.AlmostEqualsCur("def$ault")) {
		delete_linestyle(ppHead, prev_linestyle, this_linestyle);
		Pgm.Shift();
	}
	else
		LpParse(&this_linestyle->lp_properties, destinationClass, TRUE); // pick up a line spec; dont allow ls, do allow point type 
	if(!Pgm.EndOfCommand())
		IntErrorCurToken("Extraneous arguments to set %s", ppHead == &first_perm_linestyle ? "linetype" : "style line");
}

/*
 * Delete linestyle from linked list.
 * Called with pointers to the head of the list,
 * to the previous linestyle (not strictly necessary),
 * and to the linestyle to delete.
 */
void delete_linestyle(struct linestyle_def ** head, struct linestyle_def * prev, struct linestyle_def * pThis)
{
	if(pThis) { // there really is something to delete 
		if(pThis == *head)
			*head = pThis->next;
		else
			prev->next = pThis->next;
		SAlloc::F(pThis);
	}
}

// ======================================================== 
// process a 'set arrowstyle' command 
// set style arrow {tag} {nohead|head|backhead|heads} {size l,a{,b}} {{no}filled} {linestyle...} {layer n}
//
//static void set_arrowstyle()
void GnuPlot::SetArrowStyle()
{
	arrowstyle_def * this_arrowstyle = NULL;
	arrowstyle_def * new_arrowstyle = NULL;
	arrowstyle_def * prev_arrowstyle = NULL;
	arrow_style_type loc_arrow;
	int tag;
	default_arrow_style(&loc_arrow);
	Pgm.Shift();
	// get tag 
	if(!Pgm.EndOfCommand()) {
		// must be a tag expression! 
		tag = IntExpression();
		if(tag <= 0)
			IntErrorCurToken("tag must be > zero");
	}
	else
		tag = assign_arrowstyle_tag(); // default next tag 
	// search for arrowstyle 
	if(first_arrowstyle) { // skip to last arrowstyle 
		for(this_arrowstyle = first_arrowstyle; this_arrowstyle; prev_arrowstyle = this_arrowstyle, this_arrowstyle = this_arrowstyle->next)
			if(tag <= this_arrowstyle->tag) // is pThis the arrowstyle we want? 
				break;
	}
	if(this_arrowstyle == NULL || tag != this_arrowstyle->tag) {
		// adding the arrowstyle 
		new_arrowstyle = (arrowstyle_def *)gp_alloc(sizeof(arrowstyle_def), "arrowstyle");
		default_arrow_style(&(new_arrowstyle->arrow_properties));
		if(prev_arrowstyle != NULL)
			prev_arrowstyle->next = new_arrowstyle; /* add it to end of list */
		else
			first_arrowstyle = new_arrowstyle; /* make it start of list */
		new_arrowstyle->arrow_properties.tag = tag;
		new_arrowstyle->tag = tag;
		new_arrowstyle->next = this_arrowstyle;
		this_arrowstyle = new_arrowstyle;
	}
	if(Pgm.EndOfCommand())
		this_arrowstyle->arrow_properties = loc_arrow;
	else if(Pgm.AlmostEqualsCur("def$ault")) {
		this_arrowstyle->arrow_properties = loc_arrow;
		Pgm.Shift();
	}
	else
		ArrowParse(&this_arrowstyle->arrow_properties, FALSE); // pick up a arrow spec : dont allow arrowstyle 
	if(!Pgm.EndOfCommand())
		IntErrorCurToken("extraneous or out-of-order arguments in set arrowstyle");
}
// 
// assign a new arrowstyle tag
// arrowstyles are kept sorted by tag number, so pThis is easy
// returns the lowest unassigned tag number
// 
static int assign_arrowstyle_tag()
{
	int last = 0; // previous tag value 
	for(arrowstyle_def * p_this = first_arrowstyle; p_this; p_this = p_this->next)
		if(p_this->tag == last + 1)
			last++;
		else
			break;
	return (last + 1);
}

/* For set [xy]tics... command */
static void load_tics(GpAxis * this_axis)
{
	if(GPO.Pgm.EqualsCur("(")) { /* set : TIC_USER */
		GPO.Pgm.Shift();
		load_tic_user(this_axis);
	}
	else {                  /* series : TIC_SERIES */
		load_tic_series(this_axis);
	}
}

/* load TIC_USER definition */
/* (tic[,tic]...)
 * where tic is ["string"] value [level]
 * Left paren is already scanned off before entry.
 */
static void load_tic_user(GpAxis * this_axis)
{
	char * ticlabel;
	double ticposition;
	/* Free any old tic labels */
	if(!this_axis->ticdef.def.mix && !(set_iterator && set_iterator->iteration)) {
		free_marklist(this_axis->ticdef.def.user);
		this_axis->ticdef.def.user = NULL;
	}
	/* Mark pThis axis as user-generated ticmarks only, unless the */
	/* mix flag indicates that both user- and auto- tics are OK.  */
	if(!this_axis->ticdef.def.mix)
		this_axis->ticdef.type = TIC_USER;
	while(!GPO.Pgm.EndOfCommand() && !GPO.Pgm.EqualsCur(")")) {
		int ticlevel = 0;
		int save_token;
		/* syntax is  (  {'format'} value {level} {, ...} )
		 * but for timedata, the value itself is a string, which
		 * complicates things somewhat
		 */
		/* has a string with it? */
		save_token = GPO.Pgm.GetCurTokenIdx();
		ticlabel = GPO.TryToGetString();
		if(ticlabel && this_axis->datatype == DT_TIMEDATE && (GPO.Pgm.EqualsCur(",") || GPO.Pgm.EqualsCur(")"))) {
			GPO.Pgm.SetTokenIdx(save_token);
			ZFREE(ticlabel);
		}
		/* in any case get the value */
		ticposition = get_num_or_time(this_axis);
		if(!GPO.Pgm.EndOfCommand() && !GPO.Pgm.EqualsCur(",") && !GPO.Pgm.EqualsCur(")")) {
			ticlevel = GPO.IntExpression(); /* tic level */
		}
		/* add to list */
		add_tic_user(this_axis, ticlabel, ticposition, ticlevel);
		SAlloc::F(ticlabel);
		/* expect "," or ")" here */
		if(!GPO.Pgm.EndOfCommand() && GPO.Pgm.EqualsCur(","))
			GPO.Pgm.Shift(); /* loop again */
		else
			break;  /* hopefully ")" */
	}
	if(GPO.Pgm.EndOfCommand() || !GPO.Pgm.EqualsCur(")")) {
		free_marklist(this_axis->ticdef.def.user);
		this_axis->ticdef.def.user = NULL;
		GPO.IntErrorCurToken("expecting right parenthesis )");
	}
	GPO.Pgm.Shift();
}

void free_marklist(ticmark * list)
{
	while(list) {
		ticmark * freeable = list;
		list = list->next;
		SAlloc::F(freeable->label);
		SAlloc::F(freeable);
	}
}
// 
// Remove tic labels that were read from a datafile during a previous plot
// via the 'using xtics(n)' mechanism.  These have tick level < 0.
// 
ticmark * prune_dataticks(struct ticmark * list) 
{
	ticmark a = {0.0, NULL, 0, NULL};
	ticmark * b = &a;
	ticmark * tmp;
	while(list) {
		if(list->level < 0) {
			SAlloc::F(list->label);
			tmp = list->next;
			SAlloc::F(list);
			list = tmp;
		}
		else {
			b->next = list;
			b = list;
			list = list->next;
		}
	}
	b->next = NULL;
	return a.next;
}
//
// load TIC_SERIES definition 
// [start,]incr[,end] 
//
static void load_tic_series(GpAxis * this_axis)
{
	double incr, end;
	int incr_token;
	t_ticdef * tdef = &(this_axis->ticdef);
	double start = get_num_or_time(this_axis);
	if(!GPO.Pgm.EqualsCur(",")) {
		/* only step specified */
		incr_token = GPO.Pgm.GetCurTokenIdx();
		incr = start;
		start = -VERYLARGE;
		end = VERYLARGE;
	}
	else {
		GPO.Pgm.Shift();
		incr_token = GPO.Pgm.GetCurTokenIdx();
		incr = get_num_or_time(this_axis);
		if(!GPO.Pgm.EqualsCur(",")) {
			/* only step and increment specified */
			end = VERYLARGE;
		}
		else {
			GPO.Pgm.Shift();
			end = get_num_or_time(this_axis);
		}
	}
	if(start < end && incr <= 0)
		GPO.IntError(incr_token, "increment must be positive");
	if(start > end && incr >= 0)
		GPO.IntError(incr_token, "increment must be negative");
	if(start > end) {
		/* put in order */
		double numtics = floor((end * (1 + SIGNIF) - start) / incr);
		end = start;
		start = end + numtics * incr;
		incr = -incr;
	}
	if(!tdef->def.mix) { /* remove old list */
		free_marklist(tdef->def.user);
		tdef->def.user = NULL;
	}
	tdef->type = TIC_SERIES;
	tdef->def.series.start = start;
	tdef->def.series.incr = incr;
	tdef->def.series.end = end;
}
/*
 * new_text_label() allocates and initializes a text_label structure.
 * This routine is also used by the plot and splot with labels commands.
 */
text_label * new_text_label(int tag) 
{
	text_label * p_new = (text_label *)gp_alloc(sizeof(text_label), "text_label");
	memzero(p_new, sizeof(text_label));
	p_new->tag = tag;
	p_new->place = default_position;
	p_new->pos = LEFT;
	p_new->textcolor.type = TC_DEFAULT;
	p_new->lp_properties.p_type = 1;
	p_new->offset = default_offset;
	return(p_new);
}
// 
// Parse the sub-options for label style and placement.
// This is called from set_label, and from plot2d and plot3d
// to handle options for 'plot with labels'
// Note: ndim = 2 means we are inside a plot command,
//   ndim = 3 means we are inside an splot command
//   ndim = 0 in a set command
// 
//void parse_label_options(struct text_label * this_label, int ndim)
void GnuPlot::ParseLabelOptions(text_label * pLabel, int ndim)
{
	GpPosition pos;
	char * font = NULL;
	enum JUSTIFY just = LEFT;
	int  rotate = 0;
	bool set_position = false;
	bool set_just = false;
	bool set_point = false;
	bool set_rot = false;
	bool set_font = false;
	bool set_offset = false;
	bool set_layer = false;
	bool set_textcolor = false;
	bool set_hypertext = false;
	int  layer = LAYER_BACK;
	bool axis_label = (pLabel->tag <= NONROTATING_LABEL_TAG);
	bool hypertext = FALSE;
	GpPosition offset = default_offset;
	t_colorspec textcolor = {TC_DEFAULT, 0, 0.0};
	lp_style_type loc_lp(lp_style_type::defCommon);//= DEFAULT_LP_STYLE_TYPE;
	loc_lp.flags = LP_NOT_INITIALIZED;
	// Now parse the label format and style options 
	while(!Pgm.EndOfCommand()) {
		// get position 
		if((ndim == 0) && !set_position && Pgm.EqualsCur("at") && !axis_label) {
			Pgm.Shift();
			GetPosition(&pos);
			set_position = TRUE;
			continue;
		}
		// get justification 
		if(!set_just) {
			if(Pgm.AlmostEqualsCur("l$eft")) {
				just = LEFT;
				Pgm.Shift();
				set_just = TRUE;
				continue;
			}
			else if(Pgm.AlmostEqualsCur("c$entre") || Pgm.AlmostEqualsCur("c$enter")) {
				just = CENTRE;
				Pgm.Shift();
				set_just = TRUE;
				continue;
			}
			else if(Pgm.AlmostEqualsCur("r$ight")) {
				just = RIGHT;
				Pgm.Shift();
				set_just = TRUE;
				continue;
			}
		}
		// get rotation (added by RCC) 
		if(Pgm.AlmostEqualsCur("rot$ate")) {
			Pgm.Shift();
			set_rot = TRUE;
			rotate = pLabel->rotate;
			if(Pgm.EqualsCur("by")) {
				Pgm.Shift();
				rotate = IntExpression();
				if(pLabel->tag == ROTATE_IN_3D_LABEL_TAG)
					pLabel->tag = NONROTATING_LABEL_TAG;
			}
			else if(Pgm.AlmostEqualsCur("para$llel")) {
				if(pLabel->tag >= 0)
					IntErrorCurToken("invalid option");
				Pgm.Shift();
				pLabel->tag = ROTATE_IN_3D_LABEL_TAG;
			}
			else if(Pgm.AlmostEqualsCur("var$iable")) {
				if(ndim == 2) /* only in 2D plot with labels */
					pLabel->tag = VARIABLE_ROTATE_LABEL_TAG;
				else
					set_rot = FALSE;
				Pgm.Shift();
			}
			else
				rotate = TEXT_VERTICAL;
			continue;
		}
		else if(Pgm.AlmostEqualsCur("norot$ate")) {
			rotate = 0;
			Pgm.Shift();
			set_rot = TRUE;
			if(pLabel->tag == ROTATE_IN_3D_LABEL_TAG)
				pLabel->tag = NONROTATING_LABEL_TAG;
			continue;
		}
		// get font (added by DJL) 
		if(!set_font && Pgm.EqualsCur("font")) {
			Pgm.Shift();
			if((font = TryToGetString())) {
				set_font = TRUE;
				continue;
			}
			else
				IntErrorCurToken("'fontname,fontsize' expected");
		}
		// Flag pThis as hypertext rather than a normal label 
		if(!set_hypertext && Pgm.AlmostEqualsCur("hyper$text")) {
			Pgm.Shift();
			hypertext = TRUE;
			set_hypertext = TRUE;
			if(!set_point)
				loc_lp = default_hypertext_point_style;
			continue;
		}
		else if(!set_hypertext && Pgm.AlmostEqualsCur("nohyper$text")) {
			Pgm.Shift();
			hypertext = FALSE;
			set_hypertext = TRUE;
			continue;
		}
		// get front/back (added by JDP) 
		if((ndim == 0) && !set_layer && !axis_label) {
			if(Pgm.EqualsCur("back")) {
				layer = LAYER_BACK;
				Pgm.Shift();
				set_layer = TRUE;
				continue;
			}
			else if(Pgm.EqualsCur("front")) {
				layer = LAYER_FRONT;
				Pgm.Shift();
				set_layer = TRUE;
				continue;
			}
		}
		if(Pgm.EqualsCur("boxed")) {
			int tag = -1;
			Pgm.Shift();
			if(Pgm.EqualsCur("bs")) {
				Pgm.Shift();
				tag = IntExpression() % (NUM_TEXTBOX_STYLES);
			}
			pLabel->boxed = tag;
			continue;
		}
		else if(Pgm.AlmostEqualsCur("nobox$ed")) {
			pLabel->boxed = 0;
			Pgm.Shift();
			continue;
		}
		if(!axis_label && (loc_lp.flags == LP_NOT_INITIALIZED || set_hypertext)) {
			if(Pgm.AlmostEqualsCur("po$int")) {
				Pgm.Shift();
				int stored_token = Pgm.GetCurTokenIdx();
				lp_style_type tmp_lp;
				loc_lp.flags = LP_SHOW_POINTS;
				tmp_lp = loc_lp;
				LpParse(&tmp_lp, LP_ADHOC, TRUE);
				if(stored_token != Pgm.GetCurTokenIdx())
					loc_lp = tmp_lp;
				set_point = TRUE;
				continue;
			}
			else if(Pgm.AlmostEqualsCur("nopo$int")) {
				loc_lp.flags = 0;
				Pgm.Shift();
				continue;
			}
		}
		if(!set_offset && Pgm.AlmostEqualsCur("of$fset")) {
			Pgm.Shift();
			GetPositionDefault(&offset, character, ndim);
			set_offset = TRUE;
			continue;
		}
		if((Pgm.EqualsCur("tc") || Pgm.AlmostEqualsCur("text$color")) && !set_textcolor) {
			ParseColorSpec(&textcolor, TC_VARIABLE);
			set_textcolor = TRUE;
			continue;
		}
		if(Pgm.AlmostEqualsCur("noenh$anced")) {
			pLabel->noenhanced = TRUE;
			Pgm.Shift();
			continue;
		}
		else if(Pgm.AlmostEqualsCur("enh$anced")) {
			pLabel->noenhanced = FALSE;
			Pgm.Shift();
			continue;
		}
		/* Coming here means that none of the previous 'if's struck
		 * its "continue" statement, i.e.  whatever is in the command
		 * line is forbidden by the 'set label' command syntax.
		 * On the other hand, 'plot with labels' may have additional stuff coming up.
		 */
		break;
	} /* while(!Pgm.EndOfCommand()) */
	if(!set_position)
		pos = default_position;
	/* OK! copy the requested options into the label */
	if(set_position)
		pLabel->place = pos;
	if(set_just)
		pLabel->pos = just;
	if(set_rot)
		pLabel->rotate = rotate;
	if(set_layer)
		pLabel->layer = layer;
	if(set_font) {
		SAlloc::F(pLabel->font);
		pLabel->font = font;
	}
	if(set_textcolor)
		pLabel->textcolor = textcolor;
	if((loc_lp.flags & LP_NOT_INITIALIZED) == 0)
		pLabel->lp_properties = loc_lp;
	if(set_offset)
		pLabel->offset = offset;
	if(set_hypertext)
		pLabel->hypertext = hypertext;
	// Make sure the z coord and the z-coloring agree 
	if(pLabel->textcolor.type == TC_Z)
		pLabel->textcolor.value = pLabel->place.z;
	if(pLabel->lp_properties.pm3d_color.type == TC_Z)
		pLabel->lp_properties.pm3d_color.value = pLabel->place.z;
}
// 
// <histogramstyle> = {clustered {gap <n>} | rowstacked | columnstacked 
//   errorbars {gap <n>} {linewidth <lw>}}
//   {title <title_options>}
//static void parse_histogramstyle(histogram_style * hs, t_histogram_type def_type, int def_gap)
void GnuPlot::ParseHistogramStyle(histogram_style * pHs, t_histogram_type defType, int defGap)
{
	text_label title_specs; // = EMPTY_LABELSTRUCT;
	// Set defaults 
	pHs->type  = defType;
	pHs->gap   = defGap;
	if(Pgm.EndOfCommand())
		return;
	if(!Pgm.EqualsCur("hs") && !Pgm.AlmostEqualsCur("hist$ogram"))
		return;
	Pgm.Shift();
	while(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("clust$ered")) {
			pHs->type = HT_CLUSTERED;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("error$bars")) {
			pHs->type = HT_ERRORBARS;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("rows$tacked")) {
			pHs->type = HT_STACKED_IN_LAYERS;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("columns$tacked")) {
			pHs->type = HT_STACKED_IN_TOWERS;
			Pgm.Shift();
		}
		else if(Pgm.EqualsCur("gap")) {
			Pgm.Shift();
			if(Pgm.IsANumber(Pgm.GetCurTokenIdx()))
				pHs->gap = IntExpression();
			else
				IntErrorCurToken("expected gap value");
		}
		else if(Pgm.AlmostEqualsCur("ti$tle")) {
			title_specs.offset = pHs->title.offset;
			set_xyzlabel(&title_specs);
			ZFREE(title_specs.text);
			ZFREE(pHs->title.font);
			pHs->title = title_specs;
		}
		else if((Pgm.EqualsCur("lw") || Pgm.AlmostEqualsCur("linew$idth")) && (pHs->type == HT_ERRORBARS)) {
			Pgm.Shift();
			pHs->bar_lw = RealExpression();
			if(pHs->bar_lw <= 0)
				pHs->bar_lw = 1;
		}
		else
			break; // We hit something unexpected 
	}
}
// 
// set pm3d lighting {primary <fraction>} {specular <fraction>}
// 
//static void parse_lighting_options()
void GnuPlot::ParseLightingOptions()
{
	Pgm.Shift();
	// TODO: Add separate "set" commands for these 
	pm3d_shade.ambient = 1.0;
	pm3d_shade.Phong = 5.0; /* Phong exponent */
	pm3d_shade.rot_x = 45;  /* illumination angle */
	pm3d_shade.rot_z = 85;  /* illumination angle */
	pm3d_shade.fixed = TRUE; /* TRUE means the light does not rotate */
	pm3d_shade.spec2 = 0.0; /* red specular highlights on back surface */
	/* This is what you get from simply "set pm3d lighting" */
	pm3d_shade.strength = 0.5; /* contribution of primary light source */
	pm3d_shade.spec = 0.2;  /* contribution of specular highlights */
	while(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("primary")) {
			Pgm.Shift();
			pm3d_shade.strength = RealExpression();
			pm3d_shade.strength = clip_to_01(pm3d_shade.strength);
			continue;
		}
		if(Pgm.AlmostEqualsCur("spec$ular")) {
			Pgm.Shift();
			pm3d_shade.spec = RealExpression();
			pm3d_shade.spec = clip_to_01(pm3d_shade.spec);
			continue;
		}
		if(Pgm.EqualsCur("spec2")) {
			Pgm.Shift();
			pm3d_shade.spec2 = RealExpression();
			pm3d_shade.spec2 = clip_to_01(pm3d_shade.spec2);
			continue;
		}
		break;
	}
	Pgm.Rollback();
}
//
// affects both 'set style parallelaxis' and 'set style spiderplot' 
//
//static void set_style_parallel()
void GnuPlot::SetStyleParallel()
{
	Pgm.Shift();
	while(!Pgm.EndOfCommand()) {
		const int save_token = Pgm.GetCurTokenIdx();
		LpParse(&parallel_axis_style.lp_properties, LP_ADHOC, FALSE);
		if(save_token != Pgm.GetCurTokenIdx())
			continue;
		if(Pgm.EqualsCur("front"))
			parallel_axis_style.layer = LAYER_FRONT;
		else if(Pgm.EqualsCur("back"))
			parallel_axis_style.layer = LAYER_BACK;
		else
			IntErrorCurToken("unrecognized option");
		Pgm.Shift();
	}
}

//static void set_spiderplot()
void GnuPlot::SetSpiderPlot()
{
	Pgm.Shift();
	draw_border = 0;
	unset_all_tics();
	V.AspectRatio = 1.0f;
	polar = FALSE;
	keyT.auto_titles = NOAUTO_KEYTITLES;
	data_style = SPIDERPLOT;
	spiderplot = TRUE;
}

//static void set_style_spiderplot()
void GnuPlot::SetStyleSpiderPlot()
{
	Pgm.Shift();
	while(!Pgm.EndOfCommand()) {
		const int save_token = Pgm.GetCurTokenIdx();
		ParseFillStyle(&spiderplot_style.fillstyle);
		LpParse(&spiderplot_style.lp_properties,  LP_ADHOC, TRUE);
		if(save_token == Pgm.GetCurTokenIdx())
			break;
	}
}
//
// Utility routine to propagate rrange into corresponding x and y ranges 
//
//void rrange_to_xy()
void GnuPlot::RRangeToXY()
{
	// An inverted R axis makes no sense in most cases.
	// One reasonable use is to project altitude/azimuth spherical coordinates
	// so that the zenith (azimuth = 90) is in the center and the horizon
	// (azimuth = 0) is at the perimeter.
	if(AxS.__R().set_min > AxS.__R().set_max) {
		if(AxS.__R().IsNonLinear())
			IntError(NO_CARET, "cannot invert nonlinear R axis");
		inverted_raxis = true;
	}
	else {
		inverted_raxis = false;
	}
	const double __min = (AxS.__R().set_autoscale & AUTOSCALE_MIN) ? 0.0 : AxS.__R().set_min;
	if(AxS.__R().set_autoscale & AUTOSCALE_MAX) {
		AxS.__X().set_autoscale = AUTOSCALE_BOTH;
		AxS.__Y().set_autoscale = AUTOSCALE_BOTH;
	}
	else {
		AxS.__X().set_autoscale = AUTOSCALE_NONE;
		AxS.__Y().set_autoscale = AUTOSCALE_NONE;
		if(AxS.__R().IsNonLinear())
			AxS.__X().set_max = EvalLinkFunction(AxS.__R().linked_to_primary, AxS.__R().set_max) - EvalLinkFunction(AxS.__R().linked_to_primary, __min);
		else
			AxS.__X().set_max = fabs(AxS.__R().set_max - __min);
		AxS.__Y().set_max = AxS.__X().set_max;
		AxS.__Y().set_min = AxS.__X().set_min = -AxS.__X().set_max;
	}
}

//static void set_datafile()
void GnuPlot::SetDataFile()
{
	Pgm.Shift();
	while(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("miss$ing"))
			set_missing();
		else if(Pgm.AlmostEqualsCur("sep$arators"))
			SetSeparator(&df_separators);
		else if(Pgm.AlmostEqualsCur("com$mentschars"))
			SetDataFileCommentsChars();
		else if(Pgm.AlmostEqualsCur("bin$ary"))
			DfSetDataFileBinary();
		else if(Pgm.AlmostEqualsCur("fort$ran")) {
			df_fortran_constants = TRUE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("nofort$ran")) {
			df_fortran_constants = FALSE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("fpe_trap")) {
			df_nofpe_trap = FALSE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("nofpe_trap")) {
			df_nofpe_trap = TRUE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("columnhead$ers")) {
			df_columnheaders = TRUE;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("nocolumnhead$ers")) {
			df_columnheaders = FALSE;
			Pgm.Shift();
		}
		else
			IntErrorCurToken("expecting datafile modifier");
	}
}
