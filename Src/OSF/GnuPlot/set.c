/* GNUPLOT - set.c */

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

/*
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 */
#include <gnuplot.h>
#pragma hdrstop
#ifdef HAVE_ICONV
	#include <iconv.h>
#endif
#ifdef HAVE_LANGINFO_H
	#include <langinfo.h>
#endif

static palette_color_mode pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_NONE;

static int assign_arrow_tag();
static void set_cntrparam(GpCommand & rC);
static void set_decimalsign(GpCommand & rC);
static void set_degreesign(char*);
static void set_dummy(GpCommand & rC);
static void set_encoding(GpCommand & rC);
//static void set_fit(GpCommand & rC);
static void set_history(GpCommand & rC);
static void set_loadpath(GpCommand & rC);
static void set_fontpath(GpCommand & rC);
static void set_locale(GpCommand & rC);
static void set_datafile_commentschars(GpCommand & rC);
#ifdef USE_MOUSE
#endif
static void set_print(GpCommand & rC);
static void set_table(GpCommand & rC);
static void set_termoptions(GpCommand & rC);
//static void set_range(GpCommand & rC, GpAxis * pAx);

/******** Local functions ********/

static int assign_arrowstyle_tag();
static void parse_histogramstyle(GpCommand & rC, histogram_style *hs, t_histogram_type def_type, int def_gap);
static void set_style_parallel(GpCommand & rC);
//static void parse_lighting_options(GpCommand & rC);
//
// The 'set' command
//
void GpGadgets::SetCommand(GpCommand & rC)
{
	rC.CToken++;
	// Mild form of backwards compatibility 
	// Allow "set no{foo}" rather than "unset foo" 
	if(rC.P_InputLine[rC.P_Token[rC.CToken].start_index] == 'n' && rC.P_InputLine[rC.P_Token[rC.CToken].start_index+1] == 'o' && rC.P_InputLine[rC.P_Token[rC.CToken].start_index+2] != 'n') {
		if(IsInteractive)
			int_warn(rC.CToken, "deprecated syntax, use \"unset\"");
		rC.P_Token[rC.CToken].start_index += 2;
		rC.P_Token[rC.CToken].length -= 2;
		rC.CToken--;
		UnsetCommand(rC);
	}
	else {
		int save_token = rC.CToken;
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
		save_token = rC.CToken;
ITERATE:
		switch(rC.LookupTable(&set_tbl[0], rC.CToken)) {
			case S_ANGLES: SetAngles(rC); break;
			case S_ARROW: SetArrow(rC); break;
			case S_AUTOSCALE: SetAutoscale(rC); break;
			case S_BARS: SetBars(rC); break;
			case S_BORDER: SetBorder(rC); break;
			case S_BOXWIDTH: SetBoxWidth(rC); break;
			case S_CLABEL: 
				//set_clabel(); 
				//static void set_clabel()
				{
					char * new_format;
					rC.CToken++;
					clabel_onecolor = false;
					if((new_format = rC.TryToGetString())) {
						strncpy(contour_format, new_format, sizeof(contour_format));
						free(new_format);
					}
				}
				break;
			case S_CLIP: SetClip(rC); break;
			case S_COLOR: 
				UnsetMonochrome(rC); 
				rC.CToken++; 
				break;
			case S_COLORSEQUENCE: SetColorSequence(rC, 0); break;
			case S_CNTRPARAM: set_cntrparam(rC); break;
			case S_CNTRLABEL: SetCntrLabel(rC); break;
			case S_CONTOUR: SetContour(rC); break;
			case S_DASHTYPE: SetDashType(rC); break;
			case S_DGRID3D: SetDGrid3D(rC); break;
			case S_DECIMALSIGN: set_decimalsign(rC); break;
			case S_DUMMY: set_dummy(rC); break;
			case S_ENCODING: set_encoding(rC); break;
			case S_FIT: SetFit(rC, GpF); break;
			case S_FONTPATH: set_fontpath(rC); break;
			case S_FORMAT: SetFormat(rC); break;
			case S_GRID: SetGrid(rC); break;
			case S_HIDDEN3D: 
				//set_hidden3d(); 
				//
				// process 'set hidden3d' command 
				//
				//static void set_hidden3d()
				{
					rC.CToken++;
					SetHidden3DOptions(rC);
					hidden3d = true;
				}
				break;
			case S_HISTORYSIZE: // Deprecated in favor of "set history size"
			case S_HISTORY: set_history(rC); break;
			case S_ISOSAMPLES: SetIsoSamples(rC); break;
			case S_JITTER: SetJitter(rC); break;
			case S_KEY: SetKey(rC); break;
			case S_LINESTYLE: SetLineStyle(rC, &first_linestyle, LP_STYLE); break;
			case S_LINETYPE:
			    if(rC.Eq(rC.CToken+1, "cycle")) {
				    rC.CToken += 2;
				    linetype_recycle_count = rC.IntExpression();
			    }
			    else
				    SetLineStyle(rC, &first_perm_linestyle, LP_TYPE);
			    break;
			case S_LABEL: SetLabel(rC); break;
			case S_LINK:
			case S_NONLINEAR: LinkCommand(rC); break;
			case S_LOADPATH: set_loadpath(rC); break;
			case S_LOCALE: set_locale(rC); break;
			case S_LOGSCALE: SetLogScale(rC); break;
			case S_MACROS:
			    // Aug 2013 - macros are always enabled
			    rC.CToken++;
			    break;
			case S_MAPPING: SetMapping(rC); break;
			case S_MARGIN:
			    // Jan 2015: CHANGE to order <left>,<right>,<bottom>,<top>
			    LMrg.SetMargin(rC);
			    if(!rC.Eq(","))
				    break;
			    RMrg.SetMargin(rC);
			    if(!rC.Eq(","))
				    break;
			    BMrg.SetMargin(rC);
			    if(!rC.Eq(","))
				    break;
			    TMrg.SetMargin(rC);
			    break;
			case S_BMARGIN: BMrg.SetMargin(rC); break;
			case S_LMARGIN: LMrg.SetMargin(rC); break;
			case S_RMARGIN: RMrg.SetMargin(rC); break;
			case S_TMARGIN: TMrg.SetMargin(rC); break;
			case S_DATAFILE:
				if(rC.AlmostEq(++rC.CToken, "miss$ing")) {
				    //set_missing();
					//static void set_missing()
					{
						rC.CToken++;
						ZFREE(GpDf.missing_val);
						if(!rC.EndOfCommand() && !(GpDf.missing_val = rC.TryToGetString()))
							IntErrorCurToken("expected missing-value string");
					}
				}
			    else if(rC.AlmostEq("sep$arators"))
				    GpDf.SetSeparator(rC);
			    else if(rC.AlmostEq("com$mentschars"))
				    set_datafile_commentschars(rC);
			    else if(rC.AlmostEq("bin$ary"))
				    GpDf.DfSetDatafileBinary(rC);
			    else if(rC.AlmostEq("fort$ran")) {
				    GpDf.df_fortran_constants = true;
				    rC.CToken++;
			    }
			    else if(rC.AlmostEq("nofort$ran")) {
				    GpDf.df_fortran_constants = false;
				    rC.CToken++;
			    }
			    else if(rC.AlmostEq("fpe_trap")) {
				    GpDf.df_nofpe_trap = false;
				    rC.CToken++;
			    }
			    else if(rC.AlmostEq("nofpe_trap")) {
				    GpDf.df_nofpe_trap = true;
				    rC.CToken++;
			    }
			    else
				    IntErrorCurToken("expecting datafile modifier");
			    break;
#ifdef USE_MOUSE
			case S_MOUSE:
			    SetMouse(rC);
			    break;
#endif
			case S_MONOCHROME: SetMonochrome(rC); break;
			case S_MULTIPLOT: TermStartMultiplot(term, rC); break;
			case S_OFFSETS: SetOffsets(rC); break;
			case S_ORIGIN: SetOrigin(rC); break;
			case SET_OUTPUT: 
				//set_output(); 
				//static void set_output()
				{
					char * testfile;
					rC.CToken++;
					if(IsMultiPlot)
						IntErrorCurToken("you can't change the output in multiplot mode");
					if(rC.EndOfCommand()) {    /* no file specified */
						term_set_output(NULL);
						ZFREE(outstr); // means STDOUT
					}
					else if((testfile = rC.TryToGetString())) {
						gp_expand_tilde(&testfile);
						term_set_output(testfile);
						if(testfile != outstr) {
							free(testfile);
							testfile = outstr;
						}
						// if we get here then it worked, and outstr now = testfile
					}
					else
						IntErrorCurToken("expecting filename");
					invalidate_palette(); // Invalidate previous palette
				}
				break;
			case S_PARAMETRIC: SetParametric(rC); break;
			case S_PM3D: SetPm3D(rC); break;
			case S_PALETTE: SetPalette(rC); break;
			case S_COLORBOX: SetColorbox(rC); break;
			case S_POINTINTERVALBOX: SetPointIntervalBox(rC); break;
			case S_POINTSIZE: SetPointSize(rC); break;
			case S_POLAR: SetPolar(rC); break;
			case S_PRINT: set_print(rC); break;
			case S_PSDIR: 
				//set_psdir(); 
				//static void set_psdir()
				{
					rC.CToken++;
					if(rC.EndOfCommand()) { // no file specified
						ZFREE(PS_psdir);
					}
					else if((PS_psdir = rC.TryToGetString())) {
						gp_expand_tilde(&PS_psdir);
					}
					else
						IntErrorCurToken("expecting filename");
				}
				break;
#ifdef EAM_OBJECTS
			case S_OBJECT: SetObject(rC); break;
#endif
			case S_SAMPLES: SetSamples(rC); break;
			case S_SIZE: SetSize(rC); break;
			case S_STYLE: SetStyle(rC); break;
			case S_SURFACE: SetSurface(rC); break;
			case S_TABLE: set_table(rC); break;
			case S_TERMINAL: SetTerminal(rC); break;
			case S_TERMOPTIONS: set_termoptions(rC); break;
			case S_TICS: SetTics(rC); break;
			case S_TICSCALE: SetTicScale(rC); break;
			case S_TIMEFMT: 
				//set_timefmt(); 
				//
				// Process 'set P_TimeFormat' command 
				// HBB 20000507: changed this to a per-axis setting. I.e. you can now
				// have separate P_TimeFormat parse strings, different axes 
				// V5 Oct 2014: But that was never documented, and makes little sense since
				// the input format is a property of the data file, not the graph axis.
				// Revert to a single global default P_TimeFormat as documented.
				// If the default is not sufficient, use timecolumn(N,"format") on input.
				// Use "set {axis}tics format" to control the output format.
				// 
				{
					rC.CToken++;
					ZFREE(P_TimeFormat);
					char * ctmp = rC.TryToGetString();
					P_TimeFormat = NZOR(ctmp, gp_strdup(TIMEFMT));
				}
				break;
			case S_TIMESTAMP: SetTimestamp(rC); break;
			case S_TITLE: SetXYZLabel(rC, &title); break;
			case S_VIEW: SetView(rC); break;
			case S_ZERO: 
				//set_zero(); 
				// process 'set zero' command 
				{
					t_value a;
					rC.CToken++;
					Zero = magnitude(rC.ConstExpress(&a));
				}
				break;
			case S_MXTICS:
			case S_NOMXTICS:
			case S_XTICS:
			case S_NOXTICS:
			case S_XDTICS:
			case S_NOXDTICS:
			case S_XMTICS:
			case S_NOXMTICS: SetTicProp(AxA[FIRST_X_AXIS], rC); break;
			case S_MYTICS:
			case S_NOMYTICS:
			case S_YTICS:
			case S_NOYTICS:
			case S_YDTICS:
			case S_NOYDTICS:
			case S_YMTICS:
			case S_NOYMTICS: SetTicProp(AxA[FIRST_Y_AXIS], rC); break;
			case S_MX2TICS:
			case S_NOMX2TICS:
			case S_X2TICS:
			case S_NOX2TICS:
			case S_X2DTICS:
			case S_NOX2DTICS:
			case S_X2MTICS:
			case S_NOX2MTICS: SetTicProp(AxA[SECOND_X_AXIS], rC); break;
			case S_MY2TICS:
			case S_NOMY2TICS:
			case S_Y2TICS:
			case S_NOY2TICS:
			case S_Y2DTICS:
			case S_NOY2DTICS:
			case S_Y2MTICS:
			case S_NOY2MTICS: SetTicProp(AxA[SECOND_Y_AXIS], rC); break;
			case S_MZTICS:
			case S_NOMZTICS:
			case S_ZTICS:
			case S_NOZTICS:
			case S_ZDTICS:
			case S_NOZDTICS:
			case S_ZMTICS:
			case S_NOZMTICS: SetTicProp(AxA[FIRST_Z_AXIS], rC); break;
			case S_MCBTICS:
			case S_NOMCBTICS:
			case S_CBTICS:
			case S_NOCBTICS:
			case S_CBDTICS:
			case S_NOCBDTICS:
			case S_CBMTICS:
			case S_NOCBMTICS: SetTicProp(AxA[COLOR_AXIS], rC); break;
			case S_RTICS:
			case S_NORTICS:
			case S_MRTICS:
			case S_NOMRTICS: SetTicProp(AxA[POLAR_AXIS], rC); break;
			case S_XDATA:
			    AxA[FIRST_X_AXIS].SetTimeData(rC);
			    AxA[T_AXIS].datatype = AxA[U_AXIS].datatype = AxA[FIRST_X_AXIS].datatype;
			    break;
			case S_YDATA:
			    AxA[FIRST_Y_AXIS].SetTimeData(rC);
			    AxA[V_AXIS].datatype = AxA[FIRST_X_AXIS].datatype;
			    break;
			case S_ZDATA: AxA[FIRST_Z_AXIS].SetTimeData(rC); break;
			case S_CBDATA: AxA[COLOR_AXIS].SetTimeData(rC); break;
			case S_X2DATA: AxA[SECOND_X_AXIS].SetTimeData(rC); break;
			case S_Y2DATA: AxA[SECOND_Y_AXIS].SetTimeData(rC); break;
			case S_XLABEL: SetXYZLabel(rC, &AxA[FIRST_X_AXIS].label); break;
			case S_YLABEL: SetXYZLabel(rC, &AxA[FIRST_Y_AXIS].label); break;
			case S_ZLABEL: SetXYZLabel(rC, &AxA[FIRST_Z_AXIS].label); break;
			case S_CBLABEL: SetXYZLabel(rC, &AxA[COLOR_AXIS].label); break;
			case S_X2LABEL: SetXYZLabel(rC, &AxA[SECOND_X_AXIS].label); break;
			case S_Y2LABEL: SetXYZLabel(rC, &AxA[SECOND_Y_AXIS].label); break;
			case S_XRANGE: SetRange(rC, &AxA[FIRST_X_AXIS]); break;
			case S_X2RANGE: SetRange(rC, &AxA[SECOND_X_AXIS]); break;
			case S_YRANGE: SetRange(rC, &AxA[FIRST_Y_AXIS]); break;
			case S_Y2RANGE: SetRange(rC, &AxA[SECOND_Y_AXIS]); break;
			case S_ZRANGE: SetRange(rC, &AxA[FIRST_Z_AXIS]); break;
			case S_CBRANGE: SetRange(rC, &AxA[COLOR_AXIS]); break;
			case S_RRANGE:
			    SetRange(rC, &AxA[POLAR_AXIS]);
			    if(IsPolar)
				    RRangeToXY();
			    break;
			case S_TRANGE: SetRange(rC, &AxA[T_AXIS]); break;
			case S_URANGE: SetRange(rC, &AxA[U_AXIS]); break;
			case S_VRANGE: SetRange(rC, &AxA[V_AXIS]); break;
			case S_PAXIS: 
				//set_paxis(); 
				{
					rC.CToken++;
					int    p = rC.IntExpression();
					if(p <= 0 || p > MAX_PARALLEL_AXES)
						IntError(rC.CToken-1, "illegal paxis");
					if(p > (int)NumParallelAxes)
						ExtendParallelAxis(p);
					if(rC.Eq("range"))
						SetRange(rC, &P_ParallelAxis[p-1]);
					else if(rC.AlmostEq("tic$s"))
						SetTicProp(P_ParallelAxis[p-1], rC);
					else
						IntErrorCurToken("expecting 'range' or 'tics'");
				}
				break;
			case S_RAXIS: 
				//set_raxis(); 
				raxis = true;
				rC.CToken++;
				break;
			case S_XZEROAXIS: SetZeroAxis(rC, FIRST_X_AXIS); break;
			case S_YZEROAXIS: SetZeroAxis(rC, FIRST_Y_AXIS); break;
			case S_ZZEROAXIS: SetZeroAxis(rC, FIRST_Z_AXIS); break;
			case S_X2ZEROAXIS: SetZeroAxis(rC, SECOND_X_AXIS); break;
			case S_Y2ZEROAXIS: SetZeroAxis(rC, SECOND_Y_AXIS); break;
			case S_ZEROAXIS: 
				{
					//set_allzeroaxis();
					const int save_token = rC.CToken;
					SetZeroAxis(rC, FIRST_X_AXIS);
					rC.CToken = save_token;
					SetZeroAxis(rC, FIRST_Y_AXIS);
					rC.CToken = save_token;
					SetZeroAxis(rC, FIRST_Z_AXIS);
				}
				break;
			case S_XYPLANE: SetXYPlane(rC); break;
			case S_TICSLEVEL: SetTicsLevel(rC); break;
			default:
			    IntErrorCurToken("unrecognized option - see 'help set'.");
			    break;
		}
		if(next_iteration(rC.P.P_SetIterator)) {
			rC.CToken = save_token;
			goto ITERATE;
		}
	}
	GpGg.Ev.UpdateGpValVariables(0);
	rC.P.CleanupSetIterator();
}
//
// process 'set angles' command 
//
//static void set_angles(GpCommand & rC)
void GpGadgets::SetAngles(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand()) {
		Ang2Rad = 1; // assuming same as defaults
	}
	else if(rC.AlmostEq("r$adians")) {
		rC.CToken++;
		Ang2Rad = 1;
	}
	else if(rC.AlmostEq("d$egrees")) {
		rC.CToken++;
		Ang2Rad = DEG2RAD;
	}
	else
		IntErrorCurToken("expecting 'radians' or 'degrees'");
	if(IsPolar && AxA[T_AXIS].SetAutoScale) {
		// set trange if in polar mode and no explicit range
		AxA[T_AXIS].SetRange.Set(0.0, 2 * M_PI / Ang2Rad);
	}
}
//
// process a 'set arrow' command 
// set arrow {tag} {from x,y} {to x,y} {{no}head} ... 
// allow any order of options - pm 25.11.2001 
//
//static void set_arrow(GpCommand & rC)
void GpGadgets::SetArrow(GpCommand & rC)
{
	arrow_def * this_arrow = NULL;
	arrow_def * new_arrow = NULL;
	arrow_def * prev_arrow = NULL;
	bool duplication = false;
	bool set_start = false;
	bool set_end = false;
	int save_token;
	int tag;

	rC.CToken++;

	/* get tag */
	if(rC.AlmostEq("back$head") || rC.Eq("front") || rC.Eq("from") || rC.Eq("at")
	    || rC.Eq("to") || rC.Eq("rto") || rC.Eq("size") || rC.Eq("filled") || rC.Eq("empty")
	    || rC.Eq("as") || rC.Eq("arrowstyle") || rC.AlmostEq("head$s") || rC.Eq("nohead") || rC.AlmostEq("nobo$rder")) {
		tag = assign_arrow_tag();
	}
	else
		tag = rC.IntExpression();
	if(tag <= 0)
		IntErrorCurToken("tag must be > 0");
	// OK! add arrow 
	if(first_arrow != NULL) { /* skip to last arrow */
		for(this_arrow = first_arrow; this_arrow != NULL;
		    prev_arrow = this_arrow, this_arrow = this_arrow->next)
			/* is this the arrow we want? */
			if(tag <= this_arrow->tag)
				break;
	}
	if(this_arrow == NULL || tag != this_arrow->tag) {
		new_arrow = (arrow_def *)malloc(sizeof(arrow_def));
		if(prev_arrow == NULL)
			first_arrow = new_arrow;
		else
			prev_arrow->next = new_arrow;
		new_arrow->tag = tag;
		new_arrow->next = this_arrow;
		this_arrow = new_arrow;

		this_arrow->start.Set(first_axes, first_axes, first_axes, 0., 0., 0.);
		this_arrow->end.Set(first_axes, first_axes, first_axes, 0., 0., 0.);
		this_arrow->angle = 0.0;

		default_arrow_style(&(new_arrow->arrow_properties));
	}

	while(!rC.EndOfCommand()) {
		/* get start position */
		if(rC.Eq("from") || rC.Eq("at")) {
			if(set_start) {
				duplication = true;
				break;
			}
			rC.CToken++;
			if(rC.EndOfCommand())
				IntErrorCurToken("start coordinates expected");
			// get coordinates 
			GetPosition(rC, &this_arrow->start);
			set_start = true;
			continue;
		}
		// get end or relative end position
		if(rC.Eq("to") || rC.Eq("rto")) {
			if(set_end) {
				duplication = true;
				break;
			}
			this_arrow->type = (rC.Eq("rto")) ? arrow_end_relative : arrow_end_absolute;
			rC.CToken++;
			if(rC.EndOfCommand())
				IntErrorCurToken("end coordinates expected");
			// get coordinates 
			GetPosition(rC, &this_arrow->end);
			set_end = true;
			continue;
		}
		/* get end position specified as length + orientation angle */
		if(rC.AlmostEq("len$gth")) {
			if(set_end) {
				duplication = true;
				break;
			}
			this_arrow->type = arrow_end_oriented;
			rC.CToken++;
			GetPositionDefault(rC, &this_arrow->end, first_axes, 1);
			set_end = true;
			continue;
		}
		if(rC.AlmostEq("ang$le")) {
			rC.CToken++;
			this_arrow->angle = rC.RealExpression();
			continue;
		}
		// Allow interspersed style commands
		save_token = rC.CToken;
		arrow_parse(rC, &this_arrow->arrow_properties, true);
		if(save_token != rC.CToken)
			continue;
		if(!rC.EndOfCommand())
			IntErrorCurToken("wrong argument in set arrow");
	}
	if(duplication)
		IntErrorCurToken("duplicate or contradictory arguments");
}

/* assign a new arrow tag
 * arrows are kept sorted by tag number, so this is easy
 * returns the lowest unassigned tag number
 */
static int assign_arrow_tag()
{
	arrow_def * this_arrow;
	int last = 0;           /* previous tag value */
	for(this_arrow = GpGg.first_arrow; this_arrow != NULL; this_arrow = this_arrow->next)
		if(this_arrow->tag == last + 1)
			last++;
		else
			break;
	return (last + 1);
}
//
// helper routine for 'set autoscale' on a single axis
//
static bool set_autoscale_axis(GpCommand & rC, GpAxis * pAx)
{
	bool   result = true;
	char keyword[16];
	const char * name = (const char*)&(GpGg.GetAxisName(pAx->Index)[0]);
	if(rC.Eq(name)) {
		pAx->SetAutoScale = AUTOSCALE_BOTH;
		pAx->min_constraint = CONSTRAINT_NONE;
		pAx->max_constraint = CONSTRAINT_NONE;
		++rC.CToken;
	}
	else {
		sprintf(keyword, "%smi$n", name);
		if(rC.AlmostEq(keyword)) {
			pAx->SetAutoScale |= AUTOSCALE_MIN;
			pAx->min_constraint = CONSTRAINT_NONE;
			++rC.CToken;
		}
		else {
			sprintf(keyword, "%sma$x", name);
			if(rC.AlmostEq(keyword)) {
				pAx->SetAutoScale |= AUTOSCALE_MAX;
				pAx->max_constraint = CONSTRAINT_NONE;
				++rC.CToken;
			}
			else {
				sprintf(keyword, "%sfix", name);
				if(rC.Eq(keyword)) {
					pAx->SetAutoScale |= (AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX);
					++rC.CToken;
				}
				else {
					sprintf(keyword, "%sfixmi$n", name);
					if(rC.AlmostEq(keyword)) {
						pAx->SetAutoScale |= AUTOSCALE_FIXMIN;
						++rC.CToken;
					}
					else {
						sprintf(keyword, "%sfixma$x", name);
						if(rC.AlmostEq(keyword)) {
							pAx->SetAutoScale |= AUTOSCALE_FIXMAX;
							++rC.CToken;
						}
						else
							result = false;
					}
				}
			}
		}
	}
	return result;
}
//
// process 'set autoscale' command 
//
//static void set_autoscale(GpCommand & rC)
void GpAxisBlock::SetAutoscale(GpCommand & rC)
{
	int    ax_idx;
	rC.CToken++;
	if(rC.EndOfCommand()) {
		for(ax_idx = 0; ax_idx < AXIS_ARRAY_SIZE; ax_idx++)
			AxA[ax_idx].SetAutoScale = AUTOSCALE_BOTH;
		for(ax_idx = 0; ax_idx < (int)NumParallelAxes; ax_idx++)
			P_ParallelAxis[ax_idx].SetAutoScale = AUTOSCALE_BOTH;
	}
	else if(rC.Eq("xy") || rC.Eq("yx")) {
		AxA[FIRST_X_AXIS].SetAutoScale = AxA[FIRST_Y_AXIS].SetAutoScale =  AUTOSCALE_BOTH;
		AxA[FIRST_X_AXIS].min_constraint = CONSTRAINT_NONE;
		AxA[FIRST_X_AXIS].max_constraint = CONSTRAINT_NONE;
		AxA[FIRST_Y_AXIS].min_constraint = CONSTRAINT_NONE;
		AxA[FIRST_Y_AXIS].max_constraint = CONSTRAINT_NONE;
		rC.CToken++;
	}
	else if(rC.Eq("fix") || rC.AlmostEq("noext$end")) {
		for(ax_idx = 0; ax_idx < AXIS_ARRAY_SIZE; ax_idx++)
			AxA[ax_idx].SetAutoScale |= (AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX);
		for(ax_idx = 0; ax_idx < (int)NumParallelAxes; ax_idx++)
			P_ParallelAxis[ax_idx].SetAutoScale |= (AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX);
		rC.CToken++;
	}
	else if(rC.AlmostEq("ke$epfix")) {
		for(ax_idx = 0; ax_idx < AXIS_ARRAY_SIZE; ax_idx++)
			AxA[ax_idx].SetAutoScale |= AUTOSCALE_BOTH;
		for(ax_idx = 0; ax_idx < (int)NumParallelAxes; ax_idx++)
			P_ParallelAxis[ax_idx].SetAutoScale |= AUTOSCALE_BOTH;
		rC.CToken++;
	}
	else {
		if(set_autoscale_axis(rC, &AxA[FIRST_X_AXIS])) return;
		if(set_autoscale_axis(rC, &AxA[FIRST_Y_AXIS])) return;
		if(set_autoscale_axis(rC, &AxA[FIRST_Z_AXIS])) return;
		if(set_autoscale_axis(rC, &AxA[SECOND_X_AXIS])) return;
		if(set_autoscale_axis(rC, &AxA[SECOND_Y_AXIS])) return;
		if(set_autoscale_axis(rC, &AxA[COLOR_AXIS])) return;
		if(set_autoscale_axis(rC, &AxA[POLAR_AXIS])) return;
		// FIXME: Do these commands make any sense?
		if(set_autoscale_axis(rC, &AxA[T_AXIS])) return;
		if(set_autoscale_axis(rC, &AxA[U_AXIS])) return;
		if(set_autoscale_axis(rC, &AxA[V_AXIS])) return;
		// come here only if nothing found:
		GpGg.IntErrorCurToken("Invalid range");
	}
}
//
// process 'set bars' command 
//
//static void set_bars()
void GpGraphics::SetBars(GpCommand & rC)
{
	int save_token;
	rC.CToken++;
	if(rC.EndOfCommand())
		ResetBars();
	while(!rC.EndOfCommand()) {
		if(rC.Eq("default")) {
			ResetBars();
			++rC.CToken;
			return;
		}
		/* Jul 2015 - allow a separate line type for error bars */
		save_token = rC.CToken;
		GpGg.LpParse(rC, BarLp, LP_ADHOC, false);
		if(rC.CToken != save_token) {
			BarLp.flags = LP_ERRORBAR_SET;
			continue;
		}
		if(rC.AlmostEq("s$mall")) {
			BarSize = 0.0;
			++rC.CToken;
		}
		else if(rC.AlmostEq("l$arge")) {
			BarSize = 1.0;
			++rC.CToken;
		}
		else if(rC.AlmostEq("full$width")) {
			BarSize = -1.0;
			++rC.CToken;
		}
		else if(rC.Eq("front")) {
			BarLayer = LAYER_FRONT;
			++rC.CToken;
		}
		else if(rC.Eq("back")) {
			BarLayer = LAYER_BACK;
			++rC.CToken;
		}
		else {
			BarSize = rC.RealExpression();
		}
	}
}
//
// process 'set border' command 
//
//static void set_border()
void GpGadgets::SetBorder(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand()) {
		DrawBorder = 31;
		BorderLayer = LAYER_FRONT;
		BorderLp = DefaultBorderLp;
	}
	while(!rC.EndOfCommand()) {
		if(rC.Eq("front")) {
			BorderLayer = LAYER_FRONT;
			rC.CToken++;
		}
		else if(rC.Eq("back")) {
			BorderLayer = LAYER_BACK;
			rC.CToken++;
		}
		else if(rC.Eq("behind")) {
			BorderLayer = LAYER_BEHIND;
			rC.CToken++;
		}
		else {
			int save_token = rC.CToken;
			LpParse(rC, BorderLp, LP_ADHOC, false);
			if(save_token != rC.CToken)
				continue;
			DrawBorder = rC.IntExpression();
		}
	}

	/* This is the only place the user can change the border	*/
	/* so remember what he set.  If DrawBorder is later changed*/
	/* internally, we can still recover the user's preference.	*/
	UserBorder = DrawBorder;
}
//
// process 'set style boxplot' command 
//
//static void set_boxplot()
void GpGadgets::SetBoxPlot(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand()) {
		boxplot_opts.SetDefault();
	}
	while(!rC.EndOfCommand()) {
		if(rC.AlmostEq("noout$liers")) {
			boxplot_opts.outliers = false;
			rC.CToken++;
		}
		else if(rC.AlmostEq("out$liers")) {
			boxplot_opts.outliers = true;
			rC.CToken++;
		}
		else if(rC.AlmostEq("point$type") || rC.Eq("pt")) {
			rC.CToken++;
			boxplot_opts.pointtype = rC.IntExpression()-1;
		}
		else if(rC.Eq("range")) {
			rC.CToken++;
			boxplot_opts.limit_type = 0;
			boxplot_opts.limit_value = rC.RealExpression();
		}
		else if(rC.AlmostEq("frac$tion")) {
			rC.CToken++;
			boxplot_opts.limit_value = rC.RealExpression();
			if(boxplot_opts.limit_value < 0 || boxplot_opts.limit_value > 1)
				IntError(rC.CToken-1, "fraction must be less than 1");
			boxplot_opts.limit_type = 1;
		}
		else if(rC.AlmostEq("candle$sticks")) {
			rC.CToken++;
			boxplot_opts.plotstyle = CANDLESTICKS;
		}
		else if(rC.AlmostEq("finance$bars")) {
			rC.CToken++;
			boxplot_opts.plotstyle = FINANCEBARS;
		}
		else if(rC.AlmostEq("sep$aration")) {
			rC.CToken++;
			boxplot_opts.separation = rC.RealExpression();
			if(boxplot_opts.separation < 0)
				IntError(rC.CToken-1, "separation must be > 0");
		}
		else if(rC.AlmostEq("lab$els")) {
			rC.CToken++;
			if(rC.Eq("off")) {
				boxplot_opts.labels = BOXPLOT_FACTOR_LABELS_OFF;
			}
			else if(rC.Eq("x")) {
				boxplot_opts.labels = BOXPLOT_FACTOR_LABELS_X;
			}
			else if(rC.Eq("x2")) {
				boxplot_opts.labels = BOXPLOT_FACTOR_LABELS_X2;
			}
			else if(rC.Eq("auto")) {
				boxplot_opts.labels = BOXPLOT_FACTOR_LABELS_AUTO;
			}
			else
				IntError(rC.CToken-1, "expecting 'x', 'x2', 'auto' or 'off'");
			rC.CToken++;
		}
		else if(rC.AlmostEq("so$rted")) {
			boxplot_opts.sort_factors = true;
			rC.CToken++;
		}
		else if(rC.AlmostEq("un$sorted")) {
			boxplot_opts.sort_factors = false;
			rC.CToken++;
		}
		else
			IntErrorCurToken("unrecognized option");
	}
}
//
// process 'set boxwidth' command 
//
//static void set_boxwidth()
void GpGadgets::SetBoxWidth(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand()) {
		boxwidth = -1.0;
		boxwidth_is_absolute = true;
	}
	else {
		boxwidth = rC.RealExpression();
	}
	if(rC.EndOfCommand())
		return;
	else {
		if(rC.AlmostEq("a$bsolute"))
			boxwidth_is_absolute = true;
		else if(rC.AlmostEq("r$elative"))
			boxwidth_is_absolute = false;
		else
			IntErrorCurToken("expecting 'absolute' or 'relative' ");
	}
	rC.CToken++;
}
//
// process 'set clip' command 
//
//static void set_clip()
void GpGadgets::SetClip(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand()) {
		ClipPoints = true; // assuming same as points 
	}
	else if(rC.AlmostEq("p$oints")) {
		ClipPoints = true;
		rC.CToken++;
	}
	else if(rC.AlmostEq("o$ne")) {
		ClipLines1 = true;
		rC.CToken++;
	}
	else if(rC.AlmostEq("t$wo")) {
		ClipLines2 = true;
		rC.CToken++;
	}
	else
		IntErrorCurToken("expecting 'points', 'one', or 'two'");
}
//
// process 'set cntrparam' command 
//
static void set_cntrparam(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand()) {
		/* assuming same as defaults */
		contour_pts = DEFAULT_NUM_APPROX_PTS;
		contour_kind = CONTOUR_KIND_LINEAR;
		contour_order = DEFAULT_CONTOUR_ORDER;
		contour_levels = DEFAULT_CONTOUR_LEVELS;
		contour_levels_kind = LEVELS_AUTO;
	}
	else if(rC.AlmostEq("p$oints")) {
		rC.CToken++;
		contour_pts = rC.IntExpression();
	}
	else if(rC.AlmostEq("li$near")) {
		rC.CToken++;
		contour_kind = CONTOUR_KIND_LINEAR;
	}
	else if(rC.AlmostEq("c$ubicspline")) {
		rC.CToken++;
		contour_kind = CONTOUR_KIND_CUBIC_SPL;
	}
	else if(rC.AlmostEq("b$spline")) {
		rC.CToken++;
		contour_kind = CONTOUR_KIND_BSPLINE;
	}
	else if(rC.AlmostEq("le$vels")) {
		rC.CToken++;
		if(!(rC.P.P_SetIterator && rC.P.P_SetIterator->iteration)) {
			dyn_contour_levels_list.Destroy();
			dyn_contour_levels_list.Init(sizeof(double), 5, 10);
		}

		/*  RKC: I have modified the next two:
		 *   to use commas to separate list elements as in xtics
		 *   so that incremental lists start,incr[,end]as in "
		 */
		if(rC.AlmostEq("di$screte")) {
			contour_levels_kind = LEVELS_DISCRETE;
			rC.CToken++;
			if(rC.EndOfCommand())
				GpGg.IntErrorCurToken("expecting discrete level");
			else
				*(double*)dyn_contour_levels_list.GetNext() = rC.RealExpression();
			while(!rC.EndOfCommand()) {
				if(!rC.Eq(","))
					GpGg.IntErrorCurToken("expecting comma to separate discrete levels");
				rC.CToken++;
				*(double*)dyn_contour_levels_list.GetNext() = rC.RealExpression();
			}
			contour_levels = dyn_contour_levels_list.end;
		}
		else if(rC.AlmostEq("in$cremental")) {
			int i = 0; /* local counter */

			contour_levels_kind = LEVELS_INCREMENTAL;
			rC.CToken++;
			contour_levels_list[i++] = rC.RealExpression();
			if(!rC.Eq(","))
				GpGg.IntErrorCurToken("expecting comma to separate start,incr levels");
			rC.CToken++;
			if((contour_levels_list[i++] = rC.RealExpression()) == 0)
				GpGg.IntErrorCurToken("increment cannot be 0");
			if(!rC.EndOfCommand()) {
				if(!rC.Eq(","))
					GpGg.IntErrorCurToken("expecting comma to separate incr,stop levels");
				rC.CToken++;
				/* need to round up, since 10,10,50 is 5 levels, not four,
				 * but 10,10,49 is four
				 */
				dyn_contour_levels_list.end = i;
				contour_levels = (int)( (rC.RealExpression()-contour_levels_list[0])/contour_levels_list[1] + 1.0);
			}
		}
		else if(rC.AlmostEq("au$to")) {
			contour_levels_kind = LEVELS_AUTO;
			rC.CToken++;
			if(!rC.EndOfCommand())
				contour_levels = rC.IntExpression();
		}
		else {
			if(contour_levels_kind == LEVELS_DISCRETE)
				GpGg.IntErrorCurToken("Levels type is discrete, ignoring new number of contour levels");
			contour_levels = rC.IntExpression();
		}
	}
	else if(rC.AlmostEq("o$rder")) {
		int order;
		rC.CToken++;
		order = rC.IntExpression();
		if(order < 2 || order > MAX_BSPLINE_ORDER)
			GpGg.IntErrorCurToken("bspline order must be in [2..10] range.");
		contour_order = order;
	}
	else
		GpGg.IntErrorCurToken("expecting 'linear', 'cubicspline', 'bspline', 'points', 'levels' or 'order'");
}
//
// process 'set cntrlabel' command 
//
//static void set_cntrlabel()
void GpGadgets::SetCntrLabel(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand()) {
		strcpy(contour_format, "%8.3g");
		clabel_onecolor = false;
		return;
	}
	while(!rC.EndOfCommand()) {
		if(rC.AlmostEq("form$at")) {
			char * p_new;
			rC.CToken++;
			if((p_new = rC.TryToGetString()))
				strncpy(contour_format, p_new, sizeof(contour_format));
			free(p_new);
		}
		else if(rC.Eq("font")) {
			char * ctmp;
			rC.CToken++;
			if((ctmp = rC.TryToGetString())) {
				free(P_ClabelFont);
				P_ClabelFont = ctmp;
			}
		}
		else if(rC.AlmostEq("one$color")) {
			rC.CToken++;
			clabel_onecolor = true;
		}
		else if(rC.Eq("start")) {
			rC.CToken++;
			clabel_start = rC.IntExpression();
			if(clabel_start <= 0)
				clabel_start = 5;
		}
		else if(rC.AlmostEq("int$erval")) {
			rC.CToken++;
			clabel_interval = rC.IntExpression();
		}
		else {
			IntErrorCurToken("unrecognized option");
		}
	}
}
//
// process 'set contour' command 
//
//static void set_contour()
void GpGadgets::SetContour(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand())
		draw_contour = CONTOUR_BASE; // assuming same as points
	else {
		if(rC.AlmostEq("ba$se"))
			draw_contour = CONTOUR_BASE;
		else if(rC.AlmostEq("s$urface"))
			draw_contour = CONTOUR_SRF;
		else if(rC.AlmostEq("bo$th"))
			draw_contour = CONTOUR_BOTH;
		else
			IntErrorCurToken("expecting 'base', 'surface', or 'both'");
		rC.CToken++;
	}
}
//
// process 'set colorsequence command 
//
//void set_colorsequence(int option)
void GpGadgets::SetColorSequence(GpCommand & rC, int option)
{
	ulong default_colors[] = DEFAULT_COLOR_SEQUENCE;
	ulong podo_colors[] = PODO_COLOR_SEQUENCE;
	if(option == 0) { // Read option from command line 
		if(rC.Eq(++rC.CToken, "default"))
			option = 1;
		else if(rC.Eq("podo"))
			option = 2;
		else if(rC.Eq("classic"))
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
			command = (char *)malloc(strlen(command_template)+8);
			sprintf(command, command_template, i, colors[i-1]);
			rC.DoStringAndFree(command);
		}
	}
	else if(option == 3) {
		linestyle_def * p_this;
		for(p_this = first_perm_linestyle; p_this != NULL; p_this = p_this->next) {
			p_this->lp_properties.pm3d_color.type = TC_LT;
			p_this->lp_properties.pm3d_color.lt = p_this->tag-1;
		}
		linetype_recycle_count = 0;
	}
	else {
		IntErrorCurToken("Expecting 'classic' or 'default'");
	}
	rC.CToken++;
}
//
// process 'set dashtype' command 
//
//static void set_dashtype()
void GpGadgets::SetDashType(GpCommand & rC)
{
	custom_dashtype_def * this_dashtype = NULL;
	custom_dashtype_def * new_dashtype = NULL;
	custom_dashtype_def * prev_dashtype = NULL;
	int    tag;
	int    is_new = false;
	rC.CToken++;
	// get tag 
	if(rC.EndOfCommand() || ((tag = rC.IntExpression()) <= 0))
		IntErrorCurToken("tag must be > zero");
	// Check if dashtype is already defined 
	for(this_dashtype = first_custom_dashtype; this_dashtype != NULL;
	    prev_dashtype = this_dashtype, this_dashtype = this_dashtype->next)
		if(tag <= this_dashtype->tag)
			break;
	if(!this_dashtype || tag != this_dashtype->tag) {
		t_dashtype loc_dt = DEFAULT_DASHPATTERN;
		new_dashtype = (custom_dashtype_def *)malloc(sizeof(custom_dashtype_def));
		if(prev_dashtype)
			prev_dashtype->next = new_dashtype;  /* add it to end of list */
		else
			first_custom_dashtype = new_dashtype;  /* make it start of list */
		new_dashtype->tag = tag;
		new_dashtype->d_type = DASHTYPE_SOLID;
		new_dashtype->next = this_dashtype;
		new_dashtype->dashtype = loc_dt;
		this_dashtype = new_dashtype;
		is_new = true;
	}
	if(rC.AlmostEq("def$ault")) {
		delete_dashtype(prev_dashtype, this_dashtype);
		is_new = false;
		rC.CToken++;
	}
	else {
		/* FIXME: Maybe this should reject return values > 0 because */
		/* otherwise we have potentially recursive definitions.      */
		this_dashtype->d_type = rC.ParseDashType(&this_dashtype->dashtype);
	}
	if(!rC.EndOfCommand()) {
		if(is_new)
			delete_dashtype(prev_dashtype, this_dashtype);
		IntErrorCurToken("Extraneous arguments to set dashtype");
	}
}

/*
 * Delete dashtype from linked list.
 */
void delete_dashtype(custom_dashtype_def * prev, custom_dashtype_def * pThis)
{
	if(pThis != NULL) {      /* there really is something to delete */
		if(pThis == GpGg.first_custom_dashtype)
			GpGg.first_custom_dashtype = pThis->next;
		else
			prev->next = pThis->next;
		free(pThis);
	}
}
//
// process 'set dgrid3d' command 
//
//static void set_dgrid3d()
void GpGadgets::SetDGrid3D(GpCommand & rC)
{
	int token_cnt = 0; /* Number of comma-separated values read in */

	int gridx     = dgrid3d_row_fineness;
	int gridy     = dgrid3d_col_fineness;
	int normval   = dgrid3d_norm_value;
	double scalex = dgrid3d_x_scale;
	double scaley = dgrid3d_y_scale;
	// dgrid3d has two different syntax alternatives: classic and new.
	// If there is a "mode" keyword, the syntax is new, otherwise it is classic
	dgrid3d_mode  = DGRID3D_DEFAULT;
	dgrid3d_kdensity = false;
	rC.CToken++;
	while(!(rC.EndOfCommand()) ) {
		int tmp_mode = rC.LookupTable(&dgrid3d_mode_tbl[0], rC.CToken);
		if(tmp_mode != DGRID3D_OTHER) {
			dgrid3d_mode = tmp_mode;
			rC.CToken++;
		}
		switch(tmp_mode) {
			case DGRID3D_QNORM:
			    if(!(rC.EndOfCommand())) normval = rC.IntExpression();
			    break;
			case DGRID3D_SPLINES:
			    break;
			case DGRID3D_GAUSS:
			case DGRID3D_CAUCHY:
			case DGRID3D_EXP:
			case DGRID3D_BOX:
			case DGRID3D_HANN:
			    if(!(rC.EndOfCommand()) && rC.AlmostEq("kdens$ity2d")) {
				    dgrid3d_kdensity = true;
				    rC.CToken++;
			    }
			    if(!(rC.EndOfCommand())) {
				    scalex = rC.RealExpression();
				    scaley = scalex;
				    if(rC.Eq(",")) {
					    rC.CToken++;
					    scaley = rC.RealExpression();
				    }
			    }
			    break;
			default: // {rows}{,cols{,norm}}} 
			    if(rC.Eq(",")) {
				    rC.CToken++;
				    token_cnt++;
			    }
			    else if(token_cnt == 0) {
				    gridx = rC.IntExpression();
				    gridy = gridx; /* gridy defaults to gridx, unless overridden below */
			    }
			    else if(token_cnt == 1) {
				    gridy = rC.IntExpression();
			    }
			    else if(token_cnt == 2) {
				    normval = rC.IntExpression();
			    }
			    else
				    IntErrorCurToken("Unrecognized keyword or unexpected value");
			    break;
		}
	}
	// we could warn here about floating point values being truncated...
	if(gridx < 2 || gridx > 1000 || gridy < 2 || gridy > 1000)
		GpGg.IntErrorNoCaret("Number of grid points must be in [2:1000] - not changed!");
	// no mode token found: classic format
	if(dgrid3d_mode == DGRID3D_DEFAULT)
		dgrid3d_mode = DGRID3D_QNORM;
	if(scalex < 0.0 || scaley < 0.0)
		GpGg.IntErrorNoCaret("Scale factors must be greater than zero - not changed!");
	dgrid3d_row_fineness = gridx;
	dgrid3d_col_fineness = gridy;
	dgrid3d_norm_value = normval;
	dgrid3d_x_scale = scalex;
	dgrid3d_y_scale = scaley;
	dgrid3d = true;
}
//
// process 'set decimalsign' command 
//
static void set_decimalsign(GpCommand & rC)
{
	rC.CToken++;
	// Clear current setting
	ZFREE(decimalsign);
	if(rC.EndOfCommand()) {
		reset_numeric_locale();
		ZFREE(numeric_locale);
#ifdef HAVE_LOCALE_H
	}
	else if(rC.Eq("locale")) {
		char * newlocale = NULL;
		rC.CToken++;
		newlocale = rC.TryToGetString();
		SETIFZ(newlocale, gp_strdup(setlocale(LC_NUMERIC, "")));
		SETIFZ(newlocale, gp_strdup(getenv("LC_ALL")));
		SETIFZ(newlocale, gp_strdup(getenv("LC_NUMERIC")));
		SETIFZ(newlocale, gp_strdup(getenv("LANG")));
		if(!setlocale(LC_NUMERIC, newlocale ? newlocale : ""))
			IntError(rC.CToken-1, "Could not find requested locale");
		decimalsign = gp_strdup(get_decimal_locale());
		fprintf(stderr, "decimal_sign in locale is %s\n", decimalsign);
		// Save this locale for later use, but return to "C" for now
		free(numeric_locale);
		numeric_locale = newlocale;
		setlocale(LC_NUMERIC, "C");
#endif
	}
	else if(!(decimalsign = rC.TryToGetString()))
		GpGg.IntErrorCurToken("expecting string");
}
//
// process 'set dummy' command 
//
static void set_dummy(GpCommand & rC)
{
	rC.CToken++;
	for(int i = 0; i < MAX_NUM_VAR; i++) {
		if(rC.EndOfCommand())
			return;
		if(isalpha((uchar)rC.P_InputLine[rC.P_Token[rC.CToken].start_index]))
			rC.CopyStr(rC.P.SetDummyVar[i], rC.CToken++, MAX_ID_LEN);
		if(rC.Eq(","))
			rC.CToken++;
		else
			break;
	}
	if(!rC.EndOfCommand())
		GpGg.IntErrorCurToken("unrecognized syntax");
}
//
// process 'set encoding' command 
//
static void set_encoding(GpCommand & rC)
{
	char * l = NULL;
	rC.CToken++;
	if(rC.EndOfCommand()) {
		encoding = S_ENC_DEFAULT;
#ifdef HAVE_LOCALE_H
	}
	else if(rC.Eq("locale")) {
#ifndef WIN32
		l = setlocale(LC_CTYPE, "");
		if(l && (strstr(l, "utf") || strstr(l, "UTF")))
			encoding = S_ENC_UTF8;
		if(l && (strstr(l, "sjis") || strstr(l, "SJIS") || strstr(l, "932")))
			encoding = S_ENC_SJIS;
		/* FIXME: "set encoding locale" supports only sjis and utf8 on non-Windows systems */
#else
		l = setlocale(LC_CTYPE, "");
		/* preserve locale string, skip language information */
		char * cp_str = strchr(l, '.');
		if(cp_str) {
			cp_str++; /* Step past the dot in, e.g., German_Germany.1252 */
			unsigned cp = strtoul(cp_str, NULL, 10);
			/* The code below is the inverse to the code found in UnicodeText().
			   For a list of code page identifiers see
			   http://msdn.microsoft.com/en-us/library/dd317756%28v=vs.85%29.aspx
			 */
			switch(cp) {
				case 437:   encoding = S_ENC_CP437; break;
				case 850:   encoding = S_ENC_CP850; break;
				case 852:   encoding = S_ENC_CP852; break;
				case 932:   encoding = S_ENC_SJIS; break;
				case 950:   encoding = S_ENC_CP950; break;
				case 1250:  encoding = S_ENC_CP1250; break;
				case 1251:  encoding = S_ENC_CP1251; break;
				case 1252:  encoding = S_ENC_CP1252; break;
				case 1254:  encoding = S_ENC_CP1254; break;
				case 20866: encoding = S_ENC_KOI8_R; break;
				case 21866: encoding = S_ENC_KOI8_U; break;
				case 28591: encoding = S_ENC_ISO8859_1; break;
				case 28592: encoding = S_ENC_ISO8859_2; break;
				case 28599: encoding = S_ENC_ISO8859_9; break;
				case 28605: encoding = S_ENC_ISO8859_15; break;
				case 65001: encoding = S_ENC_UTF8; break;
				case 0:
				    int_warn(NO_CARET, "Error converting locale \"%s\" to codepage number", l);
				    encoding = S_ENC_DEFAULT;
				    break;
				default:
				    int_warn(NO_CARET, "Locale not supported by gnuplot: %s", l);
				    encoding = S_ENC_DEFAULT;
			}
		}
#endif
		rC.CToken++;
#endif
	}
	else {
		int temp = rC.LookupTable(&set_encoding_tbl[0], rC.CToken);
		char * senc;
		// allow string variables as parameter 
		if((temp == S_ENC_INVALID) && rC.IsStringValue(rC.CToken) && (senc = rC.TryToGetString())) {
			for(int i = 0; encoding_names[i] != NULL; i++)
				if(strcmp(encoding_names[i], senc) == 0)
					temp = i;
			free(senc);
		}
		else {
			rC.CToken++;
		}
		if(temp == S_ENC_INVALID)
			GpGg.IntErrorCurToken("unrecognized encoding specification; see 'help encoding'.");
		encoding = (set_encoding_id)temp;
	}
	set_degreesign(l); // Set degree sign to match encoding
}

static void set_degreesign(char * locale)
{
#if defined(HAVE_ICONV) && !(defined WIN32)
	char degree_utf8[3] = {'\302', '\260', '\0'};
	size_t lengthin = 3;
	size_t lengthout = 8;
	char * in = degree_utf8;
	char * out = degree_sign;
	iconv_t cd;

	if(locale) {
		/* This should work even if gnuplot doesn't understand the encoding */
#ifdef HAVE_LANGINFO_H
		char * cencoding = nl_langinfo(CODESET);
#else
		char * cencoding = strchr(locale, '.');
		if(cencoding) cencoding++;  /* Step past the dot in, e.g., ja_JP.EUC-JP */
#endif
		if(cencoding) {
			if(strcmp(cencoding, "UTF-8") == 0)
				strcpy(degree_sign, degree_utf8);
			else if((cd = iconv_open(cencoding, "UTF-8")) == (iconv_t)(-1))
				int_warn(NO_CARET, "iconv_open failed for %s", cencoding);
			else {
				if(iconv(cd, &in, &lengthin, &out, &lengthout) == (size_t)(-1))
					int_warn(NO_CARET, "iconv failed to convert degree sign");
				iconv_close(cd);
			}
		}
		return;
	}
#elif defined(WIN32)
	if(locale) {
		char * encoding = strchr(locale, '.');
		if(encoding) {
			unsigned cp;
			encoding++; /* Step past the dot in, e.g., German_Germany.1252 */
			/* iconv does not understand encodings returned by setlocale() */
			if(sscanf(encoding, "%i", &cp)) {
				wchar_t wdegreesign = 176; /* "\u00B0" */
				int n = WideCharToMultiByte(cp, WC_COMPOSITECHECK, &wdegreesign, 1, degree_sign, sizeof(degree_sign) - 1, NULL, NULL);
				degree_sign[n] = NUL;
			}
		}
		return;
	}
#else
	(void)locale; /* -Wunused argument */
#endif
	// These are the internally-known encodings
	memzero(degree_sign, sizeof(degree_sign));
	switch(encoding) {
		case S_ENC_UTF8:    degree_sign[0] = '\302'; degree_sign[1] = '\260'; break;
		case S_ENC_KOI8_R:
		case S_ENC_KOI8_U:  degree_sign[0] = '\234'; break;
		case S_ENC_CP437:
		case S_ENC_CP850:
		case S_ENC_CP852:   degree_sign[0] = '\370'; break;
		case S_ENC_SJIS:    break; /* should be 0x818B */
		case S_ENC_CP950:   break; /* should be 0xA258 */
		default:            degree_sign[0] = '\260'; break;
	}
}
//
// process 'set fit' command
//
void GpGadgets::SetFit(GpCommand & rC, GpFit & rF)
{
	rC.CToken++;
	while(!rC.EndOfCommand()) {
		if(rC.AlmostEq("log$file")) {
			char * tmp;
			rC.CToken++;
			rF.fit_suppress_log = false;
			if(rC.EndOfCommand()) {
				ZFREE(rF.fitlogfile);
			}
			else if(rC.Eq("default")) {
				rC.CToken++;
				ZFREE(rF.fitlogfile);
			}
			else if((tmp = rC.TryToGetString()) != NULL) {
				free(rF.fitlogfile);
				rF.fitlogfile = tmp;
			}
			else {
				IntErrorCurToken("expecting string");
			}
		}
		else if(rC.AlmostEq("nolog$file")) {
			rF.fit_suppress_log = true;
			rC.CToken++;
		}
		else if(rC.AlmostEq("err$orvariables")) {
			rF.fit_errorvariables = true;
			rC.CToken++;
		}
		else if(rC.AlmostEq("noerr$orvariables")) {
			rF.fit_errorvariables = false;
			rC.CToken++;
		}
		else if(rC.AlmostEq("cov$ariancevariables")) {
			rF.fit_covarvariables = true;
			rC.CToken++;
		}
		else if(rC.AlmostEq("nocov$ariancevariables")) {
			rF.fit_covarvariables = false;
			rC.CToken++;
		}
		else if(rC.AlmostEq("errors$caling")) {
			rF.fit_errorscaling = true;
			rC.CToken++;
		}
		else if(rC.AlmostEq("noerrors$caling")) {
			rF.fit_errorscaling = false;
			rC.CToken++;
		}
		else if(rC.Eq("quiet")) {
			rF.fit_verbosity = QUIET;
			rC.CToken++;
		}
		else if(rC.Eq("noquiet")) {
			rF.fit_verbosity = BRIEF;
			rC.CToken++;
		}
		else if(rC.Eq("results")) {
			rF.fit_verbosity = RESULTS;
			rC.CToken++;
		}
		else if(rC.Eq("brief")) {
			rF.fit_verbosity = BRIEF;
			rC.CToken++;
		}
		else if(rC.Eq("verbose")) {
			rF.fit_verbosity = VERBOSE;
			rC.CToken++;
		}
		else if(rC.Eq("prescale")) {
			rF.fit_prescale = true;
			rC.CToken++;
		}
		else if(rC.Eq("noprescale")) {
			rF.fit_prescale = false;
			rC.CToken++;
		}
		else if(rC.Eq("limit")) {
			/* preserve compatibility with FIT_LIMIT user variable */
			UdvtEntry * v;
			double value = 0.0;
			rC.CToken++;
			if(rC.Eq("default"))
				rC.CToken++;
			else
				value = rC.RealExpression();
			if((value > 0.) && (value < 1.)) {
				v = Ev.AddUdvByName(rF.FITLIMIT);
				v->udv_value.SetComplex(value, 0);
			}
			else {
				Ev.DelUdvByName(rF.FITLIMIT, false);
			}
		}
		else if(rC.Eq("limit_abs")) {
			rC.CToken++;
			double value = rC.RealExpression();
			rF.epsilon_abs = (value > 0.0) ? value : 0.0;
		}
		else if(rC.Eq("maxiter")) {
			/* preserve compatibility with FIT_MAXITER user variable */
			UdvtEntry * v;
			int maxiter;
			rC.CToken++;
			if(rC.Eq("default")) {
				rC.CToken++;
				maxiter = 0;
			}
			else
				maxiter = rC.IntExpression();
			if(maxiter > 0) {
				v = Ev.AddUdvByName(rF.FITMAXITER);
				v->udv_value.SetInt(maxiter);
			}
			else {
				Ev.DelUdvByName(rF.FITMAXITER, false);
			}
		}
		else if(rC.Eq("start_lambda")) {
			/* preserve compatibility with FIT_START_LAMBDA user variable */
			UdvtEntry * v;
			double value = 0.0;
			rC.CToken++;
			if(rC.Eq("default"))
				rC.CToken++;
			else
				value = rC.RealExpression();
			if(value > 0.0) {
				v = Ev.AddUdvByName(rF.FITSTARTLAMBDA);
				v->udv_value.SetComplex(value, 0);
			}
			else {
				Ev.DelUdvByName(rF.FITSTARTLAMBDA, false);
			}
		}
		else if(rC.Eq("lambda_factor")) {
			/* preserve compatibility with FIT_LAMBDA_FACTOR user variable */
			UdvtEntry * v;
			double value;
			rC.CToken++;
			if(rC.Eq("default")) {
				rC.CToken++;
				value = 0.;
			}
			else
				value = rC.RealExpression();
			if(value > 0.) {
				v = Ev.AddUdvByName(rF.FITLAMBDAFACTOR);
				v->udv_value.SetComplex(value, 0);
			}
			else {
				Ev.DelUdvByName(rF.FITLAMBDAFACTOR, false);
			}
		}
		else if(rC.Eq("script")) {
			char * tmp;
			rC.CToken++;
			if(rC.EndOfCommand()) {
				ZFREE(rF.fit_script);
			}
			else if(rC.Eq("default")) {
				rC.CToken++;
				ZFREE(rF.fit_script);
			}
			else if((tmp = rC.TryToGetString())) {
				free(rF.fit_script);
				rF.fit_script = tmp;
			}
			else {
				IntErrorCurToken("expecting string");
			}
		}
		else if(rC.Eq("wrap")) {
			rC.CToken++;
			rF.fit_wrap = rC.IntExpression();
			SETMAX(rF.fit_wrap, 0);
		}
		else if(rC.Eq("nowrap")) {
			rC.CToken++;
			rF.fit_wrap = 0;
		}
		else if(rC.Eq("v4")) {
			rC.CToken++;
			rF.fit_v4compatible = true;
		}
		else if(rC.Eq("v5")) {
			rC.CToken++;
			rF.fit_v4compatible = false;
		}
		else {
			IntErrorCurToken("unrecognized option --- see `help set fit`");
		}
	}
}
//
// process 'set format' command 
//
//void set_format()
void GpGadgets::SetFormat(GpCommand & rC)
{
	bool set_for_axis[AXIS_ARRAY_SIZE] = AXIS_ARRAY_INITIALIZER(false);
	int    ax_idx;
	char * p_format;
	td_type tictype = DT_UNINITIALIZED;
	rC.CToken++;
	if((ax_idx = rC.LookupTable(axisname_tbl, rC.CToken)) >= 0) {
		set_for_axis[ax_idx] = true;
		rC.CToken++;
	}
	else if(rC.Eq("xy") || rC.Eq("yx")) {
		set_for_axis[FIRST_X_AXIS] = set_for_axis[FIRST_Y_AXIS] = true;
		rC.CToken++;
	}
	else {
		/* Set all of them */
		for(ax_idx = 0; ax_idx < AXIS_ARRAY_SIZE; ax_idx++)
			set_for_axis[ax_idx] = true;
	}

	if(rC.EndOfCommand()) {
		for(ax_idx = FIRST_AXES; ax_idx <= POLAR_AXIS; ax_idx++) {
			if(set_for_axis[ax_idx]) {
				free(AxA[ax_idx].formatstring);
				AxA[ax_idx].formatstring = gp_strdup(DEF_FORMAT);
				AxA[ax_idx].tictype = DT_NORMAL;
			}
		}
		return;
	}
	if(!(p_format = rC.TryToGetString()))
		IntErrorCurToken("expecting p_format string");
	if(rC.AlmostEq("time$date")) {
		tictype = DT_TIMEDATE;
		rC.CToken++;
	}
	else if(rC.AlmostEq("geo$graphic")) {
		tictype = DT_DMS;
		rC.CToken++;
	}
	else if(rC.AlmostEq("num$eric")) {
		tictype = DT_NORMAL;
		rC.CToken++;
	}
	for(ax_idx = FIRST_AXES; ax_idx <= POLAR_AXIS; ax_idx++) {
		if(set_for_axis[ax_idx]) {
			free(AxA[ax_idx].formatstring);
			AxA[ax_idx].formatstring = gp_strdup(p_format);
			if(tictype != DT_UNINITIALIZED)
				AxA[ax_idx].tictype = tictype;
		}
	}
	free(p_format);
}

bool GpGadgets::GridMatch(GpCommand & rC, int axIdx, const char * pString)
{
	bool explicit_change = false;
	if(rC.AlmostEq(pString+2)) { 
		AxA[axIdx].SetGrid((pString[2] == 'm'), true);  
		explicit_change = true; 
		++rC.CToken;
	}
	else if(rC.AlmostEq(pString)) { 
		AxA[axIdx].SetGrid((pString[2] == 'm'), false); 
		explicit_change = true; 
		++rC.CToken; 
	}
	return explicit_change;
}
//
// process 'set grid' command
//
void GpGadgets::SetGrid(GpCommand & rC)
{
	bool explicit_change = false;
	rC.CToken++;
	#define GRID_MATCH(axis, string) if(GridMatch(rC, axis, string)) explicit_change = true; 
/*#define GRID_MATCH(axis, string)				\
	if(rC.AlmostEq(string+2))    { AxA[axis].SetGrid((string[2] == 'm'), true);  explicit_change = true; ++rC.CToken; \
	} \
	else if(rC.AlmostEq(string)) { AxA[axis].SetGrid((string[2] == 'm'), false); explicit_change = true; ++rC.CToken; }*/
	//
	while(!rC.EndOfCommand()) {
		GRID_MATCH(FIRST_X_AXIS, "nox$tics")
		else GRID_MATCH(FIRST_Y_AXIS, "noy$tics")
		else GRID_MATCH(FIRST_Z_AXIS, "noz$tics")
		else GRID_MATCH(SECOND_X_AXIS, "nox2$tics")
		else GRID_MATCH(SECOND_Y_AXIS, "noy2$tics")
		else GRID_MATCH(FIRST_X_AXIS, "nomx$tics")
		else GRID_MATCH(FIRST_Y_AXIS, "nomy$tics")
		else GRID_MATCH(FIRST_Z_AXIS, "nomz$tics")
		else GRID_MATCH(SECOND_X_AXIS, "nomx2$tics")
		else GRID_MATCH(SECOND_Y_AXIS, "nomy2$tics")
		else GRID_MATCH(COLOR_AXIS, "nocb$tics")
		else GRID_MATCH(COLOR_AXIS, "nomcb$tics")
		else GRID_MATCH(POLAR_AXIS, "nor$tics")
		else GRID_MATCH(POLAR_AXIS, "nomr$tics")
		else if(rC.AlmostEq("po$lar")) {
			if(!SomeGridSelected())
				AxA[POLAR_AXIS].Flags |= GpAxis::fGridMajor;
			polar_grid_angle = 30*DEG2RAD;
			rC.CToken++;
			if(rC.IsANumber(rC.CToken) || rC.TypeDdv(rC.CToken) == INTGR || rC.TypeDdv(rC.CToken) == CMPLX)
				polar_grid_angle = Ang2Rad * rC.RealExpression();
		}
		else if(rC.AlmostEq("nopo$lar")) {
			polar_grid_angle = 0; // not polar grid
			rC.CToken++;
		}
		else if(rC.Eq("back")) {
			grid_layer = LAYER_BACK;
			rC.CToken++;
		}
		else if(rC.Eq("front")) {
			grid_layer = LAYER_FRONT;
			rC.CToken++;
		}
		else if(rC.AlmostEq("layerd$efault") || rC.Eq("behind")) {
			grid_layer = LAYER_BEHIND;
			rC.CToken++;
		}
		else { // only remaining possibility is a line type
			int save_token = rC.CToken;
			LpParse(rC, grid_lp, LP_ADHOC, false);
			if(rC.Eq(","))
			{
				rC.CToken++;
				LpParse(rC, mgrid_lp, LP_ADHOC, false);
			}
			else if(save_token != rC.CToken)
				mgrid_lp = grid_lp;
			if(save_token == rC.CToken)
				break;
		}
	}
	if(!explicit_change && !SomeGridSelected()) {
		// no axis specified, thus select default grid
		if(IsPolar) {
			AxA[POLAR_AXIS].Flags |= GpAxis::fGridMajor;
		}
		else {
			AxA[FIRST_X_AXIS].Flags |= GpAxis::fGridMajor;
			AxA[FIRST_Y_AXIS].Flags |= GpAxis::fGridMajor;
		}
	}
}
//
// process 'set hidden3d' command 
//
//static void set_hidden3d() { GpGg.Gp__C.CToken++; GpGg.SetHidden3DOptions(GpGg.Gp__C); GpGg.hidden3d = true; }

static void set_history(GpCommand & rC)
{
	rC.CToken++;
	while(!rC.EndOfCommand()) {
		if(rC.Eq("quiet")) {
			rC.CToken++;
			rC.H.history_quiet = true;
			continue;
		}
		else if(rC.AlmostEq("num$bers")) {
			rC.CToken++;
			rC.H.history_quiet = false;
			continue;
		}
		else if(rC.Eq("full")) {
			rC.CToken++;
			rC.H.history_full = true;
			continue;
		}
		else if(rC.Eq("trim")) {
			rC.CToken++;
			rC.H.history_full = false;
			continue;
		}
		else if(rC.AlmostEq("def$ault")) {
			rC.CToken++;
			rC.H.history_quiet = false;
			rC.H.history_full = true;
			rC.H.gnuplot_history_size = HISTORY_SIZE;
			continue;
		}
		else if(rC.Eq("size")) {
			rC.CToken++;
			/* fall through */
		}
		// Catches both the deprecated "set historysize" and "set history size" 
		rC.H.gnuplot_history_size = rC.IntExpression();
#ifndef GNUPLOT_HISTORY
		int_warn(NO_CARET, "This copy of gnuplot was built without support for command history.");
#endif
	}
}
//
// process 'set isosamples' command 
//
void GpGadgets::SetIsoSamples(GpCommand & rC)
{
	rC.CToken++;
	int    tsamp1 = abs(rC.IntExpression());
	int    tsamp2 = tsamp1;
	if(!rC.EndOfCommand()) {
		if(!rC.Eq(","))
			IntErrorCurToken("',' expected");
		rC.CToken++;
		tsamp2 = abs(rC.IntExpression());
	}
	if(tsamp1 < 2 || tsamp2 < 2)
		IntErrorCurToken("sampling rate must be > 1; sampling unchanged");
	else {
		CurvePoints * f_p = P_FirstPlot;
		SurfacePoints * f_3dp = P_First3DPlot;
		P_FirstPlot = 0;
		P_First3DPlot = NULL;
		cp_free(f_p);
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
			case LEFT:
			    key->margin = GPKEY_LMARGIN;
			    break;
			case CENTRE:
			    if(key->vpos == JUST_TOP)
				    key->margin = GPKEY_TMARGIN;
			    else
				    key->margin = GPKEY_BMARGIN;
			    break;
			case RIGHT:
			    key->margin = GPKEY_RMARGIN;
			    break;
		}
	}
	else {
		switch(key->vpos) {
			case JUST_TOP:
			    key->margin = GPKEY_TMARGIN;
			    break;
			case JUST_CENTRE:
			    if(key->hpos == LEFT)
				    key->margin = GPKEY_LMARGIN;
			    else
				    key->margin = GPKEY_RMARGIN;
			    break;
			case JUST_BOT:
			    key->margin = GPKEY_BMARGIN;
			    break;
		}
	}
}
//
// process 'set key' command 
//
void GpGadgets::SetKey(GpCommand & rC)
{
	bool   vpos_set = false;
	bool   hpos_set = false;
	bool   reg_set = false;
	bool   sdir_set = false;
	char * vpos_warn = "Multiple vertical position settings";
	char * hpos_warn = "Multiple horizontal position settings";
	char * reg_warn = "Multiple location region settings";
	char * sdir_warn = "Multiple stack direction settings";
	legend_key * p_key = &keyT;
	// Only for backward compatibility with deprecated "set keytitle foo" 
	if(rC.AlmostEq("keyt$itle"))
		goto S_KEYTITLE;
	rC.CToken++;
	p_key->visible = true;
	while(!rC.EndOfCommand()) {
		switch(rC.LookupTable(&set_key_tbl[0], rC.CToken)) {
			case S_KEY_ON:
			    p_key->visible = true;
			    break;
			case S_KEY_OFF:
			    p_key->visible = false;
			    break;
			case S_KEY_DEFAULT:
			    ResetKey();
			    break;
			case S_KEY_TOP:
			    if(vpos_set)
				    int_warn(rC.CToken, vpos_warn);
			    p_key->vpos = JUST_TOP;
			    vpos_set = true;
			    break;
			case S_KEY_BOTTOM:
			    if(vpos_set)
				    int_warn(rC.CToken, vpos_warn);
			    p_key->vpos = JUST_BOT;
			    vpos_set = true;
			    break;
			case S_KEY_LEFT:
			    if(hpos_set)
				    int_warn(rC.CToken, hpos_warn);
			    p_key->hpos = LEFT;
			    hpos_set = true;
			    break;
			case S_KEY_RIGHT:
			    if(hpos_set)
				    int_warn(rC.CToken, hpos_warn);
			    p_key->hpos = RIGHT;
			    hpos_set = true;
			    break;
			case S_KEY_CENTER:
			    if(!vpos_set) 
					p_key->vpos = JUST_CENTRE;
			    if(!hpos_set) 
					p_key->hpos = CENTRE;
			    if(vpos_set || hpos_set)
				    vpos_set = hpos_set = true;
			    break;
			case S_KEY_VERTICAL:
			    if(sdir_set)
				    int_warn(rC.CToken, sdir_warn);
			    p_key->stack_dir = GPKEY_VERTICAL;
			    sdir_set = true;
			    break;
			case S_KEY_HORIZONTAL:
			    if(sdir_set)
				    int_warn(rC.CToken, sdir_warn);
			    p_key->stack_dir = GPKEY_HORIZONTAL;
			    sdir_set = true;
			    break;
			case S_KEY_OVER:
			    if(reg_set)
				    int_warn(rC.CToken, reg_warn);
			// Fall through 
			case S_KEY_ABOVE:
			    if(!hpos_set)
				    p_key->hpos = CENTRE;
			    if(!sdir_set)
				    p_key->stack_dir = GPKEY_HORIZONTAL;
			    p_key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
			    p_key->margin = GPKEY_TMARGIN;
			    reg_set = true;
			    break;
			case S_KEY_UNDER:
			    if(reg_set)
				    int_warn(rC.CToken, reg_warn);
			// Fall through 
			case S_KEY_BELOW:
			    if(!hpos_set)
				    p_key->hpos = CENTRE;
			    if(!sdir_set)
				    p_key->stack_dir = GPKEY_HORIZONTAL;
			    p_key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
			    p_key->margin = GPKEY_BMARGIN;
			    reg_set = true;
			    break;
			case S_KEY_INSIDE:
			    if(reg_set)
				    int_warn(rC.CToken, reg_warn);
			    p_key->region = GPKEY_AUTO_INTERIOR_LRTBC;
			    reg_set = true;
			    break;
			case S_KEY_OUTSIDE:
			    if(reg_set)
				    int_warn(rC.CToken, reg_warn);
			    p_key->region = GPKEY_AUTO_EXTERIOR_LRTBC;
			    reg_set = true;
			    break;
			case S_KEY_TMARGIN:
			    if(reg_set)
				    int_warn(rC.CToken, reg_warn);
			    p_key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
			    p_key->margin = GPKEY_TMARGIN;
			    reg_set = true;
			    break;
			case S_KEY_BMARGIN:
			    if(reg_set)
				    int_warn(rC.CToken, reg_warn);
			    p_key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
			    p_key->margin = GPKEY_BMARGIN;
			    reg_set = true;
			    break;
			case S_KEY_LMARGIN:
			    if(reg_set)
				    int_warn(rC.CToken, reg_warn);
			    p_key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
			    p_key->margin = GPKEY_LMARGIN;
			    reg_set = true;
			    break;
			case S_KEY_RMARGIN:
			    if(reg_set)
				    int_warn(rC.CToken, reg_warn);
			    p_key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
			    p_key->margin = GPKEY_RMARGIN;
			    reg_set = true;
			    break;
			case S_KEY_LLEFT:
			    p_key->just = GPKEY_LEFT;
			    break;
			case S_KEY_RRIGHT:
			    p_key->just = GPKEY_RIGHT;
			    break;
			case S_KEY_REVERSE:
			    p_key->reverse = true;
			    break;
			case S_KEY_NOREVERSE:
			    p_key->reverse = false;
			    break;
			case S_KEY_INVERT:
			    p_key->invert = true;
			    break;
			case S_KEY_NOINVERT:
			    p_key->invert = false;
			    break;
			case S_KEY_ENHANCED:
			    p_key->enhanced = true;
			    break;
			case S_KEY_NOENHANCED:
			    p_key->enhanced = false;
			    break;
			case S_KEY_BOX:
			    rC.CToken++;
			    p_key->box.l_type = LT_BLACK;
			    if(!rC.EndOfCommand()) {
				    int old_token = rC.CToken;
				    LpParse(rC, p_key->box, LP_ADHOC, false);
				    if(old_token == rC.CToken && rC.IsANumber(rC.CToken)) {
					    p_key->box.l_type = rC.IntExpression() - 1;
					    rC.CToken++;
				    }
			    }
			    rC.CToken--; /* is incremented after loop */
			    break;
			case S_KEY_NOBOX:
			    p_key->box.l_type = LT_NODRAW;
			    break;
			case S_KEY_SAMPLEN:
			    rC.CToken++;
			    p_key->swidth = rC.RealExpression();
			    rC.CToken--; /* it is incremented after loop */
			    break;
			case S_KEY_SPACING:
			    rC.CToken++;
			    p_key->vert_factor = rC.RealExpression();
				SETMAX(p_key->vert_factor, 0.0);
			    rC.CToken--; /* it is incremented after loop */
			    break;
			case S_KEY_WIDTH:
			    rC.CToken++;
			    p_key->width_fix = rC.RealExpression();
			    rC.CToken--; /* it is incremented after loop */
			    break;
			case S_KEY_HEIGHT:
			    rC.CToken++;
			    p_key->height_fix = rC.RealExpression();
			    rC.CToken--; /* it is incremented after loop */
			    break;
			case S_KEY_AUTOTITLES:
			    if(rC.AlmostEq(++rC.CToken, "col$umnheader"))
				    p_key->auto_titles = COLUMNHEAD_KEYTITLES;
			    else {
				    p_key->auto_titles = FILENAME_KEYTITLES;
				    rC.CToken--;
			    }
			    break;
			case S_KEY_NOAUTOTITLES:
			    p_key->auto_titles = NOAUTO_KEYTITLES;
			    break;
			case S_KEY_TITLE:
S_KEYTITLE:
			    p_key->title.pos = CENTRE;
			    SetXYZLabel(rC, &p_key->title);
			    rC.CToken--;
			    break;
			case S_KEY_NOTITLE:
			    ZFREE(p_key->title.text);
			    break;
			case S_KEY_FONT:
			    rC.CToken++;
			    // Make sure they've specified a font 
			    if(!rC.IsStringValue(rC.CToken))
				    IntErrorCurToken("expected font");
			    else {
				    char * tmp = rC.TryToGetString();
				    if(tmp) {
					    free(p_key->font);
					    p_key->font = tmp;
				    }
				    rC.CToken--;
			    }
			    break;
			case S_KEY_TEXTCOLOR:
				{
					t_colorspec lcolor; //  = DEFAULT_COLORSPEC;
					lcolor.SetDefault();
					rC.ParseColorSpec(&lcolor, TC_VARIABLE);
					// Only for backwards compatibility 
					if(lcolor.type == TC_RGB && lcolor.value == -1.0)
						lcolor.type = TC_VARIABLE;
					p_key->textcolor = lcolor;
				}
			    rC.CToken--;
			    break;
			case S_KEY_MAXCOLS:
			    rC.CToken++;
				p_key->maxcols = (rC.EndOfCommand() || rC.AlmostEq("a$utomatic")) ? 0 : rC.IntExpression();
			    if(p_key->maxcols < 0)
				    p_key->maxcols = 0;
			    rC.CToken--; // it is incremented after loop 
			    break;
			case S_KEY_MAXROWS:
			    rC.CToken++;
			    if(rC.EndOfCommand() || rC.AlmostEq("a$utomatic"))
				    p_key->maxrows = 0;
			    else
				    p_key->maxrows = rC.IntExpression();
			    if(p_key->maxrows < 0)
				    p_key->maxrows = 0;
			    rC.CToken--; // it is incremented after loop 
			    break;
			case S_KEY_FRONT:
			    p_key->front = true;
			    break;
			case S_KEY_NOFRONT:
			    p_key->front = false;
			    break;
			case S_KEY_MANUAL:
			    rC.CToken++;
			    if(reg_set)
				    int_warn(rC.CToken, reg_warn);
			    GetPosition(rC, &p_key->user_pos);
			    p_key->region = GPKEY_USER_PLACEMENT;
			    reg_set = true;
			    rC.CToken--; // will be incremented again soon
			    break;
			case S_KEY_INVALID:
			default:
			    IntErrorCurToken("unknown p_key option");
			    break;
		}
		rC.CToken++;
	}
	if(p_key->region == GPKEY_AUTO_EXTERIOR_LRTBC)
		set_key_position_from_stack_direction(p_key);
	else if(p_key->region == GPKEY_AUTO_EXTERIOR_MARGIN) {
		if(vpos_set && (p_key->margin == GPKEY_TMARGIN || p_key->margin == GPKEY_BMARGIN))
			int_warn(NO_CARET, "ignoring top/center/bottom; incompatible with TMrg/BMrg.");
		else if(hpos_set && (p_key->margin == GPKEY_LMARGIN || p_key->margin == GPKEY_RMARGIN))
			int_warn(NO_CARET, "ignoring left/center/right; incompatible with LMrg/TMrg.");
	}
}
//
// process 'set label' command 
// set label {tag} {"label_text"{,<value>{,...}}} {<label options>} 
// EAM Mar 2003 - option parsing broken out into separate routine 
//
//static void set_label()
void GpGadgets::SetLabel(GpCommand & rC)
{
	GpTextLabel * p_label = NULL;
	GpTextLabel * p_new_label = NULL;
	GpTextLabel * p_prev_label = NULL;
	t_value a;
	int tag = -1;
	rC.CToken++;
	if(!rC.EndOfCommand()) {
		// The first item must be either a tag or the label text 
		const int  save_token = rC.CToken;
		if(rC.IsLetter(rC.CToken) && rC.TypeDdv(rC.CToken) == 0) {
			tag = AssignLabelTag();
		}
		else {
			rC.ConstExpress(&a);
			if(a.type == STRING) {
				rC.CToken = save_token;
				tag = AssignLabelTag();
				gpfree_string(&a);
			}
			else {
				tag = (int)a.Real();
			}
		}
		if(tag <= 0)
			IntErrorCurToken("tag must be > zero");
		if(first_label) { // skip to last label 
			for(p_label = first_label; p_label != NULL;
				p_prev_label = p_label, p_label = p_label->next)
				// is this the label we want? 
				if(tag <= p_label->tag)
					break;
		}
		// Insert this label into the list if it is a new one 
		if(p_label == NULL || tag != p_label->tag) {
			p_new_label = new_text_label(tag);
			p_new_label->offset.Set(character, character, character, 0., 0., 0.);
			if(p_prev_label == NULL)
				first_label = p_new_label;
			else
				p_prev_label->next = p_new_label;
			p_new_label->next = p_label;
			p_label = p_new_label;
		}
		if(!rC.EndOfCommand()) {
			ParseLabelOptions(rC, p_label, 0);
			char * text = rC.TryToGetString();
			if(text) {
				free(p_label->text);
				p_label->text = text;
			}
		}
		// Now parse the label format and style options 
		ParseLabelOptions(rC, p_label, 0);
	}
}
//
// assign a new label tag
// labels are kept sorted by tag number, so this is easy
// returns the lowest unassigned tag number
//
//static int assign_label_tag()
int GpGadgets::AssignLabelTag()
{
	int last = 0;           /* previous tag value */
	for(GpTextLabel * this_label = first_label; this_label != NULL; this_label = this_label->next)
		if(this_label->tag == last + 1)
			last++;
		else
			break;
	return (last + 1);
}
//
// process 'set loadpath' command 
//
static void set_loadpath(GpCommand & rC)
{
	// We pick up all loadpath elements here before passing
	// them on to set_var_loadpath()
	//
	char * collect = NULL;
	rC.CToken++;
	if(rC.EndOfCommand()) {
		clear_loadpath();
	}
	else while(!rC.EndOfCommand()) {
			char * ss;
			if((ss = rC.TryToGetString())) {
				int len = (collect ? strlen(collect) : 0);
				gp_expand_tilde(&ss);
				collect = (char *)gp_realloc(collect, len+1+strlen(ss)+1, "tmp loadpath");
				if(len != 0) {
					strcpy(collect+len+1, ss);
					*(collect+len) = PATHSEP;
				}
				else
					strcpy(collect, ss);
				free(ss);
			}
			else {
				GpGg.IntErrorCurToken("expected string");
			}
		}
	if(collect) {
		set_var_loadpath(collect);
		free(collect);
	}
}
//
// process 'set fontpath' command 
//
static void set_fontpath(GpCommand & rC)
{
	// We pick up all fontpath elements here before passing them on to set_var_fontpath()
	char * collect = NULL;
	rC.CToken++;
	if(rC.EndOfCommand()) {
		clear_fontpath();
	}
	else {
		while(!rC.EndOfCommand()) {
			char * ss = rC.TryToGetString();
			if(ss) {
				size_t len = sstrlen(collect);
				gp_expand_tilde(&ss);
				collect = (char *)gp_realloc(collect, len+1+strlen(ss)+1, "tmp fontpath");
				if(len != 0) {
					strcpy(collect+len+1, ss);
					*(collect+len) = PATHSEP;
				}
				else
					strcpy(collect, ss);
				free(ss);
			}
			else {
				GpGg.IntErrorCurToken("expected string");
			}
		}
	}
	if(collect) {
		set_var_fontpath(collect);
		free(collect);
	}
}
//
// process 'set locale' command 
//
static void set_locale(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand()) {
		init_locale();
	}
	else {
		char * s = rC.TryToGetString();
		if(s) {
			set_var_locale(s);
			free(s);
		}
		else
			GpGg.IntErrorCurToken("expected string");
	}
}
//
// process 'set logscale' command 
//
//static void set_logscale()
void GpGadgets::SetLogScale(GpCommand & rC)
{
	bool set_for_axis[AXIS_ARRAY_SIZE] = AXIS_ARRAY_INITIALIZER(false);
	int axis;
	double newbase = 10;
	rC.CToken++;
	if(rC.EndOfCommand()) {
		for(axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++)
			set_for_axis[axis] = true;
	}
	else {
		// do reverse search because of "x", "x1", "x2" sequence in axisname_tbl
		int i = 0;
		while(i < rC.P_Token[rC.CToken].length) {
			axis = lookup_table_nth_reverse(axisname_tbl, NUMBER_OF_MAIN_VISIBLE_AXES, rC.P_InputLine + rC.P_Token[rC.CToken].start_index + i);
			if(axis < 0) {
				rC.P_Token[rC.CToken].start_index += i;
				IntErrorCurToken("invalid axis");
			}
			set_for_axis[axisname_tbl[axis].value] = true;
			i += strlen(axisname_tbl[axis].key);
		}
		rC.CToken++;
		if(!rC.EndOfCommand()) {
			newbase = fabs(rC.RealExpression());
			if(newbase <= 1.0)
				IntErrorCurToken("log base must be > 1.0; logscale unchanged");
		}
	}
	for(axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
		if(set_for_axis[axis]) {
			AxA[axis].Flags |= GpAxis::fLog;
			AxA[axis].base = newbase;
			AxA[axis].log_base = log(newbase);
			if((axis == POLAR_AXIS) && IsPolar)
				RRangeToXY();
		}
	}
	// Because the log scaling is applied during data input, a quick refresh
	// using existing stored data will not work if the log setting changes.
	SetRefreshOk(E_REFRESH_NOT_OK, 0);
}
//
// process 'set mapping3d' command 
//
//static void set_mapping()
void GpGadgets::SetMapping(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand())
		mapping3d = MAP3D_CARTESIAN; // assuming same as points
	else if(rC.AlmostEq("ca$rtesian"))
		mapping3d = MAP3D_CARTESIAN;
	else if(rC.AlmostEq("s$pherical"))
		mapping3d = MAP3D_SPHERICAL;
	else if(rC.AlmostEq("cy$lindrical"))
		mapping3d = MAP3D_CYLINDRICAL;
	else
		IntErrorCurToken("expecting 'cartesian', 'spherical', or 'cylindrical'");
	rC.CToken++;
}
//
// process 'set {blrt}margin' command
//
//static void set_margin(GpCommand & rC, GpPosition & rMargin)
void GpPosition::SetMargin(GpCommand & rC)
{
	SetX(character, -1.0);
	rC.CToken++;
	if(!rC.EndOfCommand()) {
		if(rC.Eq("at") && !rC.AlmostEq(++rC.CToken, "sc$reen"))
			GpGg.IntErrorCurToken("expecting 'screen <fraction>'");
		if(rC.AlmostEq("sc$reen")) {
			scalex = screen;
			rC.CToken++;
		}
		x = rC.RealExpression();
		if(x < 0)
			x = -1;
		if(scalex == screen) {
			SETMAX(x, 0);
			SETMIN(x, 1);
		}
	}
}

static void set_datafile_commentschars(GpCommand & rC)
{
	char * s;
	rC.CToken++;
	if(rC.EndOfCommand()) {
		free(GpDf.df_commentschars);
		GpDf.df_commentschars = gp_strdup(DEFAULT_COMMENTS_CHARS);
	}
	else if((s = rC.TryToGetString())) {
		free(GpDf.df_commentschars);
		GpDf.df_commentschars = s;
	}
	else // Leave it the way it was
		GpGg.IntErrorCurToken("expected string with comments chars");
}
//
// (version 5) 'set monochrome' command
//
//static void set_monochrome()
void GpGadgets::SetMonochrome(GpCommand & rC)
{
	IsMonochrome = true;
	if(!rC.EndOfCommand())
		rC.CToken++;
	if(rC.AlmostEq("def$ault")) {
		rC.CToken++;
		while(first_mono_linestyle)
			delete_linestyle(&first_mono_linestyle, first_mono_linestyle, first_mono_linestyle);
	}
	init_monochrome();
	if(rC.AlmostEq("linet$ype") || rC.Eq("lt")) {
		/* we can pass this off to the generic "set linetype" code */
		if(rC.Eq(rC.CToken+1, "cycle")) {
			rC.CToken += 2;
			mono_recycle_count = rC.IntExpression();
		}
		else
			SetLineStyle(rC, &first_mono_linestyle, LP_TYPE);
	}
	if(!rC.EndOfCommand())
		IntErrorCurToken("unrecognized option");
}

#ifdef USE_MOUSE
//static void set_mouse(GpCommand & rC)
void GpGadgets::SetMouse(GpCommand & rC)
{
	char * ctmp;
	rC.CToken++;
	Mse.Cfg.on = 1;
	while(!rC.EndOfCommand()) {
		if(rC.AlmostEq("do$ubleclick")) {
			++rC.CToken;
			Mse.Cfg.doubleclick = (int)rC.RealExpression();
			if(Mse.Cfg.doubleclick < 0)
				Mse.Cfg.doubleclick = 0;
		}
		else if(rC.AlmostEq("nodo$ubleclick")) {
			Mse.Cfg.doubleclick = 0; /* double click off */
			++rC.CToken;
		}
		else if(rC.AlmostEq("zoomco$ordinates")) {
			Mse.Cfg.annotate_zoom_box = 1;
			++rC.CToken;
		}
		else if(rC.AlmostEq("nozoomco$ordinates")) {
			Mse.Cfg.annotate_zoom_box = 0;
			++rC.CToken;
		}
		else if(rC.AlmostEq("po$lardistancedeg")) {
			Mse.Cfg.polardistance = 1;
			UpdateStatusline(term);
			++rC.CToken;
		}
		else if(rC.AlmostEq("polardistancet$an")) {
			Mse.Cfg.polardistance = 2;
			UpdateStatusline(term);
			++rC.CToken;
		}
		else if(rC.AlmostEq("nopo$lardistance")) {
			Mse.Cfg.polardistance = 0;
			UpdateStatusline(term);
			++rC.CToken;
		}
		else if(rC.AlmostEq("label$s")) {
			Mse.Cfg.label = 1;
			++rC.CToken;
			/* check if the optional argument "<label options>" is present */
			if(rC.IsStringValue(rC.CToken) && (ctmp = rC.TryToGetString())) {
				free(Mse.Cfg.labelopts);
				Mse.Cfg.labelopts = ctmp;
			}
		}
		else if(rC.AlmostEq("nola$bels")) {
			Mse.Cfg.label = 0;
			++rC.CToken;
		}
		else if(rC.AlmostEq("ve$rbose")) {
			Mse.Cfg.verbose = 1;
			++rC.CToken;
		}
		else if(rC.AlmostEq("nove$rbose")) {
			Mse.Cfg.verbose = 0;
			++rC.CToken;
		}
		else if(rC.AlmostEq("zoomju$mp")) {
			Mse.Cfg.warp_pointer = 1;
			++rC.CToken;
		}
		else if(rC.AlmostEq("nozoomju$mp")) {
			Mse.Cfg.warp_pointer = 0;
			++rC.CToken;
		}
		else if(rC.AlmostEq("fo$rmat")) {
			++rC.CToken;
			if(rC.IsStringValue(rC.CToken) && (ctmp = rC.TryToGetString())) {
				if(Mse.Cfg.fmt != mouse_fmt_default)
					free(Mse.Cfg.fmt);
				Mse.Cfg.fmt = ctmp;
			}
			else
				Mse.Cfg.fmt = mouse_fmt_default;
		}
		else if(rC.AlmostEq("mo$useformat")) {
			++rC.CToken;
			if(rC.IsStringValue(rC.CToken) && (ctmp = rC.TryToGetString())) {
				free(mouse_alt_string);
				mouse_alt_string = ctmp;
				if(!strlen(mouse_alt_string)) {
					ZFREE(mouse_alt_string);
					if(MOUSE_COORDINATES_ALT == mouse_mode)
						mouse_mode = MOUSE_COORDINATES_REAL;
				}
				else {
					mouse_mode = MOUSE_COORDINATES_ALT;
				}
				rC.CToken++;
			}
			else {
				int itmp = rC.IntExpression();
				if(itmp >= MOUSE_COORDINATES_REAL && itmp <= MOUSE_COORDINATES_ALT) {
					if(MOUSE_COORDINATES_ALT == itmp && !mouse_alt_string) {
						fprintf(stderr, "please 'set mouse mouseformat <fmt>' first.\n");
					}
					else {
						mouse_mode = itmp;
					}
				}
				else {
					fprintf(stderr, "should be: %d <= mouseformat <= %d\n", MOUSE_COORDINATES_REAL, MOUSE_COORDINATES_ALT);
				}
			}
		}
		else if(rC.AlmostEq("noru$ler")) {
			rC.CToken++;
			SetMouseRuler(false, -1, -1);
		}
		else if(rC.AlmostEq("ru$ler")) {
			rC.CToken++;
			if(rC.EndOfCommand() || !rC.Eq("at")) {
				SetMouseRuler(true, -1, -1);
			}
			else { /* set mouse ruler at ... */
				GpPosition where;
				int x, y;
				rC.CToken++;
				if(rC.EndOfCommand())
					IntErrorCurToken("expecting ruler coordinates");
				GetPosition(rC, &where);
				MapPosition(term, &where, &x, &y, "ruler at");
				SetMouseRuler(true, (int)x, (int)y);
			}
		}
		else if(rC.AlmostEq("zoomfac$tors")) {
			double x = 1.0, y = 1.0;
			rC.CToken++;
			if(!rC.EndOfCommand()) {
				x = rC.RealExpression();
				if(rC.Eq(",")) {
					rC.CToken++;
					y = rC.RealExpression();
				}
			}
			Mse.Cfg.xmzoom_factor = x;
			Mse.Cfg.ymzoom_factor = y;
		}
		else {
			if(!rC.EndOfCommand())
				IntErrorCurToken("wrong option");
			break;
		}
	}
}

#endif
//
// process 'set offsets' command 
//
//static void set_offsets()
void GpGadgets::SetOffsets(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand()) {
		loff.x = roff.x = toff.y = boff.y = 0.0;
		return;
	}
	loff.scalex = first_axes;
	if(rC.AlmostEq("gr$aph")) {
		loff.scalex = graph;
		rC.CToken++;
	}
	loff.x = rC.RealExpression();
	if(!rC.Eq(","))
		return;
	roff.scalex = first_axes;
	if(rC.AlmostEq(++rC.CToken, "gr$aph")) {
		roff.scalex = graph;
		rC.CToken++;
	}
	roff.x = rC.RealExpression();
	if(!rC.Eq(","))
		return;
	toff.scaley = first_axes;
	if(rC.AlmostEq(++rC.CToken, "gr$aph")) {
		toff.scaley = graph;
		rC.CToken++;
	}
	toff.y = rC.RealExpression();
	if(!rC.Eq(","))
		return;
	boff.scaley = first_axes;
	if(rC.AlmostEq(++rC.CToken, "gr$aph")) {
		boff.scaley = graph;
		rC.CToken++;
	}
	boff.y = rC.RealExpression();
}
//
// process 'set origin' command 
//
//static void set_origin()
void GpGadgets::SetOrigin(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand()) {
		XOffs = 0.0;
		YOffs = 0.0;
	}
	else {
		XOffs = (float)rC.RealExpression();
		if(!rC.Eq(","))
			IntErrorCurToken("',' expected");
		rC.CToken++;
		YOffs = (float)rC.RealExpression();
	}
}
//
// process 'set print' command 
//
static void set_print(GpCommand & rC)
{
	bool append_p = false;
	char * testfile = NULL;
	rC.CToken++;
	if(rC.EndOfCommand()) {    /* no file specified */
		rC.PrintSetOutput(NULL, false, append_p);
	}
	else if(rC.Eq("$") && rC.IsLetter(rC.CToken + 1)) { /* datablock */
		/* NB: has to come first because rC.TryToGetString will choke on the datablock name */
		char * datablock_name = _strdup(rC.ParseDataBlockName());
		if(!rC.EndOfCommand()) {
			if(rC.Eq("append")) {
				append_p = true;
				rC.CToken++;
			}
			else {
				GpGg.IntErrorCurToken("expecting keyword \'append\'");
			}
		}
		rC.PrintSetOutput(datablock_name, true, append_p);
	}
	else if((testfile = rC.TryToGetString())) { /* file name */
		gp_expand_tilde(&testfile);
		if(!rC.EndOfCommand()) {
			if(rC.Eq("append")) {
				append_p = true;
				rC.CToken++;
			}
			else {
				GpGg.IntErrorCurToken("expecting keyword \'append\'");
			}
		}
		rC.PrintSetOutput(testfile, false, append_p);
	}
	else
		GpGg.IntErrorCurToken("expecting filename or datablock");
}
//
// process 'set parametric' command 
//
//static void set_parametric()
void GpGadgets::SetParametric(GpCommand & rC)
{
	rC.CToken++;
	if(!IsParametric) {
		IsParametric = true;
		if(!IsPolar) { /* already done for polar */
			strcpy(rC.P.SetDummyVar[0], "t");
			strcpy(rC.P.SetDummyVar[1], "y");
			if(IsInteractive)
				fprintf(stderr, "\n\tdummy variable is t for curves, u/v for surfaces\n");
		}
	}
}

/* is resetting palette enabled?
 * note: reset_palette() is disabled within 'test palette'
 */
int enable_reset_palette = 1; // @global
//
// default settings for palette 
//
//void reset_palette()
void GpGadgets::ResetPalette()
{
	if(enable_reset_palette) {
		free(SmPalette.gradient);
		free(SmPalette.color);
		AtType::Destroy(SmPalette.Afunc.at);
		AtType::Destroy(SmPalette.Bfunc.at);
		AtType::Destroy(SmPalette.Cfunc.at);
		InitColor();
		pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_NONE;
	}
}
//
// Process 'set palette defined' gradient specification
// Syntax
//   set palette defined   -->  use default palette
//   set palette defined ( <pos1> <colorspec1>, ... , <posN> <colorspecN> )
//     <posX>  gray value, automatically rescaled to [0, 1]
//     <colorspecX>   :=  { "<color_name>" | "<X-style-color>" |  <r> <g> <b> }
//     <color_name>     predefined colors (see below)
//     <X-style-color>  "#rrggbb" with 2char hex values for red, green, blue
//        <r> <g> <b>      three values in [0, 1] for red, green and blue
//   return 1 if named colors where used, 0 otherwise
// 
//static int set_palette_defined()
int GpGadgets::SetPaletteDefined(GpCommand & rC)
{
	double p = 0, r = 0, g = 0, b = 0;
	int num, named_colors = 0;
	int actual_size = 8;

	/* Invalidate previous gradient */
	invalidate_palette();
	free(SmPalette.gradient);
	SmPalette.gradient = (gradient_struct *)malloc(actual_size*sizeof(gradient_struct));
	SmPalette.smallest_gradient_interval = 1;
	if(rC.EndOfCommand()) {
		// lets use some default gradient 
		const double pal[][4] = { {0.0, 0.05, 0.05, 0.2}, {0.1, 0, 0, 1},
		    {0.25, 0.7, 0.85, 0.9}, {0.4, 0, 0.75, 0},
		    {0.5, 1, 1, 0}, {0.7, 1, 0, 0},
		    {0.9, 0.6, 0.6, 0.6}, {1.0, 0.95, 0.95, 0.95} };
		for(int i = 0; i<8; i++) {
			SmPalette.gradient[i].pos = pal[i][0];
			//SmPalette.gradient[i].col.r = pal[i][1];
			//SmPalette.gradient[i].col.g = pal[i][2];
			//SmPalette.gradient[i].col.b = pal[i][3];
			SmPalette.gradient[i].col.Set(pal[i][1], pal[i][2], pal[i][3]);
		}
		SmPalette.gradient_num = 8;
		SmPalette.cmodel = C_MODEL_RGB;
		SmPalette.smallest_gradient_interval = 0.1; /* From pal[][] */
		rC.CToken--; /* Caller will increment! */
		return 0;
	}
	if(!rC.Eq("(") )
		IntErrorCurToken("expected ( to start gradient definition");
	++rC.CToken;
	num = -1;
	while(!rC.EndOfCommand()) {
		p = rC.RealExpression();
		char * col_str = rC.TryToGetString();
		if(col_str) {
			// either color name or X-style rgb value "#rrggbb" 
			if(col_str[0] == '#' || col_str[0] == '0') {
				// X-style specifier 
				int rr, gg, bb;
				if((sscanf(col_str, "#%2x%2x%2x", &rr, &gg, &bb) != 3 ) &&  (sscanf(col_str, "0x%2x%2x%2x", &rr, &gg, &bb) != 3 ))
					IntError(rC.CToken-1, "Unknown color specifier. Use '#RRGGBB' of '0xRRGGBB'.");
				r = (double)(rr)/255.;
				g = (double)(gg)/255.;
				b = (double)(bb)/255.;
			}
			else { /* some predefined names */
				/* Maybe we could scan the X11 rgb.txt file to look up color
				 * names?  Or at least move these definitions to some file
				 * which is included somehow during compilation instead
				 * hardcoding them. */
				/* Can't use lookupt_table() as it works for tokens only,
				  so we'll do it manually */
				const GenTable * tbl = pm3d_color_names_tbl;
				while(tbl->key) {
					if(!strcmp(col_str, tbl->key)) {
						r = (double)((tbl->value >> 16 ) & 255) / 255.;
						g = (double)((tbl->value >> 8 ) & 255) / 255.;
						b = (double)(tbl->value & 255) / 255.;
						break;
					}
					tbl++;
				}
				if(!tbl->key)
					IntError(rC.CToken-1, "Unknown color name.");
				named_colors = 1;
			}
			free(col_str);
		}
		else {
			/* numerical rgb, hsv, xyz, ... values  [0,1] */
			r = rC.RealExpression();
			if(r<0 || r>1)
				IntError(rC.CToken-1, "Value out of range [0,1].");
			g = rC.RealExpression();
			if(g<0 || g>1)
				IntError(rC.CToken-1, "Value out of range [0,1].");
			b = rC.RealExpression();
			if(b<0 || b>1)
				IntError(rC.CToken-1, "Value out of range [0,1].");
		}
		++num;
		if(num >= actual_size) {
			// get more space for the gradient 
			actual_size += 10;
			SmPalette.gradient = (gradient_struct *)gp_realloc(SmPalette.gradient, actual_size*sizeof(gradient_struct), "pm3d gradient");
		}
		SmPalette.gradient[num].pos = p;
		//SmPalette.gradient[num].col.r = r;
		//SmPalette.gradient[num].col.g = g;
		//SmPalette.gradient[num].col.b = b;
		SmPalette.gradient[num].col.Set(r, g, b);
		if(rC.Eq(")") ) break;
		if(!rC.Eq(",") )
			IntErrorCurToken("expected comma");
		++rC.CToken;
	}
	SmPalette.gradient_num = num + 1;
	CheckPaletteGrayscale(rC);
	return named_colors;
}
//
//  process 'set palette file' command
//  load a palette from file, honor datafile modifiers
//
void GpGadgets::SetPaletteFile(GpCommand & rC)
{
	int specs;
	double v[4];
	int i, j, actual_size;
	char * file_name;
	++rC.CToken;
	// get filename
	if(!(file_name = rC.TryToGetString()))
		IntErrorCurToken("missing filename");
	GpDf.DfSetPlotMode(MODE_QUERY);   /* Needed only for binary datafiles */
	specs = GpDf.DfOpen(rC, file_name, 4, NULL);
	free(file_name);
	if(specs > 0 && specs < 3)
		IntErrorCurToken("Less than 3 using specs for palette");
	ZFREE(SmPalette.gradient);
	actual_size = 10;
	SmPalette.gradient = (gradient_struct *)malloc(actual_size*sizeof(gradient_struct));
	i = 0;
	// values are simply clipped to [0,1] without notice 
	while((j = GpDf.DfReadLine(v, 4)) != DF_EOF) {
		if(i >= actual_size) {
			actual_size += 10;
			SmPalette.gradient = (gradient_struct*)gp_realloc(SmPalette.gradient, actual_size*sizeof(gradient_struct), "pm3d gradient");
		}
		switch(j) {
			case 3:
			    //SmPalette.gradient[i].col.r = clip_to_01(v[0]);
			    //SmPalette.gradient[i].col.g = clip_to_01(v[1]);
			    //SmPalette.gradient[i].col.b = clip_to_01(v[2]);
				SmPalette.gradient[i].col.Set(clip_to_01(v[0]), clip_to_01(v[1]), clip_to_01(v[2]));
			    SmPalette.gradient[i].pos = i;
			    break;
			case 4:
			    //SmPalette.gradient[i].col.r = clip_to_01(v[1]);
			    //SmPalette.gradient[i].col.g = clip_to_01(v[2]);
			    //SmPalette.gradient[i].col.b = clip_to_01(v[3]);
				SmPalette.gradient[i].col.Set(v[1], v[2], v[3]).Constrain();
			    SmPalette.gradient[i].pos = v[0];
			    break;
			default:
			    GpDf.DfClose();
			    IntErrorCurToken("Bad data on line %d", GpDf.df_line_number);
			    break;
		}
		++i;
	}
	GpDf.DfClose();
	if(i==0)
		IntErrorCurToken("No valid palette found");
	SmPalette.gradient_num = i;
	CheckPaletteGrayscale(rC);
}
//
//  Process a 'set palette function' command.
//  Three functions with fixed dummy variable gray are registered which
//  map gray to the different color components.
//  If ALLOW_DUMMY_VAR_FOR_GRAY is set:
//     A different dummy variable may proceed the formulae in quotes.
//     This syntax is different from the usual '[u=<start>:<end>]', but
//     as <start> and <end> are fixed to 0 and 1 you would have to type
//     always '[u=]' which looks strange, especially as just '[u]' wouldn't work.
//   If unset:  dummy variable is fixed to 'gray'.
//
//static void set_palette_function()
void GpGadgets::SetPaletteFunction(GpCommand & rC)
{
	int start_token;
	char saved_dummy_var[MAX_ID_LEN+1];

	++rC.CToken;
	strncpy(saved_dummy_var, rC.P.CDummyVar[0], MAX_ID_LEN);

	/* set dummy variable */
#ifdef ALLOW_DUMMY_VAR_FOR_GRAY
	if(rC.IsString(rC.CToken)) {
		quote_str(rC.P.CDummyVar[0], rC.CToken, MAX_ID_LEN);
		++rC.CToken;
	}
	else
#endif /* ALLOW_DUMMY_VAR_FOR_GRAY */
	strncpy(rC.P.CDummyVar[0], "gray", MAX_ID_LEN);
	// Afunc 
	start_token = rC.CToken;
	if(SmPalette.Afunc.at) {
		AtType::Destroy(SmPalette.Afunc.at);
		SmPalette.Afunc.at = NULL;
	}
	rC.P_DummyFunc = &SmPalette.Afunc;
	SmPalette.Afunc.at = rC.P.PermAt();
	if(!SmPalette.Afunc.at)
		GpGg.IntError(start_token, "not enough memory for function");
	rC.MCapture(&(SmPalette.Afunc.definition), start_token, rC.CToken-1);
	rC.P_DummyFunc = NULL;
	if(!rC.Eq(","))
		IntErrorCurToken("expected comma");
	++rC.CToken;
	/* Bfunc */
	start_token = rC.CToken;
	if(SmPalette.Bfunc.at) {
		AtType::Destroy(SmPalette.Bfunc.at);
		SmPalette.Bfunc.at = NULL;
	}
	rC.P_DummyFunc = &SmPalette.Bfunc;
	SmPalette.Bfunc.at = rC.P.PermAt();
	if(!SmPalette.Bfunc.at)
		GpGg.IntError(start_token, "not enough memory for function");
	rC.MCapture(&(SmPalette.Bfunc.definition), start_token, rC.CToken-1);
	rC.P_DummyFunc = NULL;
	if(!rC.Eq(","))
		IntErrorCurToken("expected comma");
	++rC.CToken;
	// Cfunc 
	start_token = rC.CToken;
	if(SmPalette.Cfunc.at) {
		AtType::Destroy(SmPalette.Cfunc.at);
		SmPalette.Cfunc.at = NULL;
	}
	rC.P_DummyFunc = &SmPalette.Cfunc;
	SmPalette.Cfunc.at = rC.P.PermAt();
	if(!SmPalette.Cfunc.at)
		GpGg.IntError(start_token, "not enough memory for function");
	rC.MCapture(&(SmPalette.Cfunc.definition), start_token, rC.CToken-1);
	rC.P_DummyFunc = NULL;
	strncpy(rC.P.CDummyVar[0], saved_dummy_var, MAX_ID_LEN);
}
//
// Normalize gray scale of gradient to fill [0,1] and
// complain if gray values are not strictly increasing.
// Maybe automatic sorting of the gray values could be a feature.
//
//static void check_palette_grayscale()
void GpGadgets::CheckPaletteGrayscale(GpCommand & rC)
{
	int i;
	double off, f;
	gradient_struct * gradient = SmPalette.gradient;
	// check if gray values are sorted
	for(i = 0; i<SmPalette.gradient_num-1; ++i) {
		if(gradient[i].pos > gradient[i+1].pos) {
			IntErrorCurToken("Gray scale not sorted in gradient.");
		}
	}
	// fit gray axis into [0:1]:  subtract offset and rescale
	off = gradient[0].pos;
	f = 1.0 / ( gradient[SmPalette.gradient_num-1].pos-off );
	for(i = 1; i<SmPalette.gradient_num-1; ++i) {
		gradient[i].pos = f*(gradient[i].pos-off);
	}
	// paranoia on the first and last entries
	gradient[0].pos = 0.0;
	gradient[SmPalette.gradient_num-1].pos = 1.0;
	// save smallest interval
	SmPalette.smallest_gradient_interval = 1.0;
	for(i = 1; i<SmPalette.gradient_num-1; ++i) {
		if(((gradient[i].pos - gradient[i-1].pos) > 0) && (SmPalette.smallest_gradient_interval > (gradient[i].pos - gradient[i-1].pos)))
			SmPalette.smallest_gradient_interval = (gradient[i].pos - gradient[i-1].pos);
	}
}

void GpGadgets::CheckTransform(GpCommand & rC, int & rTransformDefined)
{
	if(rTransformDefined)
		IntErrorCurToken("inconsistent palette options");
	rTransformDefined = 1;				     
}
//
// Process 'set palette' command 
//
//static void set_palette()
void GpGadgets::SetPalette(GpCommand & rC)
{
	int transform_defined = 0;
	int named_color = 0;
	rC.CToken++;
	if(rC.EndOfCommand()) // reset to default settings 
		ResetPalette();
	else { // go through all options of 'set palette' 
		for(; !rC.EndOfCommand(); rC.CToken++) {
			switch(rC.LookupTable(&set_palette_tbl[0], rC.CToken)) {
				// positive and negative picture 
				case S_PALETTE_POSITIVE: // "pos$itive" 
				    SmPalette.positive = SMPAL_POSITIVE;
				    continue;
				case S_PALETTE_NEGATIVE: // "neg$ative" 
				    SmPalette.positive = SMPAL_NEGATIVE;
				    continue;
				// Now the options that determine the palette of smooth colours 
				// gray or rgb-coloured 
				case S_PALETTE_GRAY: // "gray" 
				    SmPalette.colorMode = SMPAL_COLOR_MODE_GRAY;
				    continue;
				case S_PALETTE_GAMMA: // "gamma" 
				    ++rC.CToken;
				    SmPalette.gamma = rC.RealExpression();
				    --rC.CToken;
				    continue;
				case S_PALETTE_COLOR: // "col$or" 
				    SmPalette.colorMode = (pm3d_last_set_palette_mode != SMPAL_COLOR_MODE_NONE) ?
						pm3d_last_set_palette_mode : SMPAL_COLOR_MODE_RGB;
				    continue;
				// rgb color mapping formulae: rgb$formulae r,g,b (3 integers) 
				case S_PALETTE_RGBFORMULAE: { // "rgb$formulae" 
				    int i;
				    char * formerr = "color formula out of range (use `show palette rgbformulae' to display the range)";

				    CheckTransform(rC, transform_defined);
				    rC.CToken++;
				    i = rC.IntExpression();
				    if(abs(i) >= SmPalette.colorFormulae)
					    IntErrorCurToken(formerr);
				    SmPalette.formulaR = i;
				    if(!rC.Eq(rC.CToken--, ","))
					    continue;
				    rC.CToken += 2;
				    i = rC.IntExpression();
				    if(abs(i) >= SmPalette.colorFormulae)
					    IntErrorCurToken(formerr);
				    SmPalette.formulaG = i;
				    if(!rC.Eq(rC.CToken--, ","))
					    continue;
				    rC.CToken += 2;
				    i = rC.IntExpression();
				    if(abs(i) >= SmPalette.colorFormulae)
					    IntErrorCurToken(formerr);
				    SmPalette.formulaB = i;
				    rC.CToken--;
				    SmPalette.colorMode = SMPAL_COLOR_MODE_RGB;
				    pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_RGB;
				    continue;
			    } // rgbformulae 
				// rgb color mapping based on the "cubehelix" scheme proposed by 
				// D A Green (2011)  http://arxiv.org/abs/1108.5083		     
				case S_PALETTE_CUBEHELIX: { // cubehelix 
				    bool done = false;
				    CheckTransform(rC, transform_defined);
				    SmPalette.colorMode = SMPAL_COLOR_MODE_CUBEHELIX;
				    SmPalette.cubehelix_start = 0.5;
				    SmPalette.cubehelix_cycles = -1.5;
				    SmPalette.cubehelix_saturation = 1.0;
				    rC.CToken++;
				    do {
					    if(rC.Eq("start")) {
						    rC.CToken++;
						    SmPalette.cubehelix_start = rC.RealExpression();
					    }
					    else if(rC.AlmostEq("cyc$les")) {
						    rC.CToken++;
						    SmPalette.cubehelix_cycles = rC.RealExpression();
					    }
					    else if(rC.AlmostEq("sat$uration")) {
						    rC.CToken++;
						    SmPalette.cubehelix_saturation = rC.RealExpression();
					    }
					    else
						    done = true;
				    } while(!done);
				    --rC.CToken;
				    continue;
			    } // cubehelix 
				case S_PALETTE_DEFINED: { // "def$ine" 
				    CheckTransform(rC, transform_defined);
				    ++rC.CToken;
				    named_color = SetPaletteDefined(rC);
				    SmPalette.colorMode = SMPAL_COLOR_MODE_GRADIENT;
				    pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_GRADIENT;
				    continue;
			    }
				case S_PALETTE_FILE: { // "file" 
				    CheckTransform(rC, transform_defined);
				    SetPaletteFile(rC);
				    SmPalette.colorMode = SMPAL_COLOR_MODE_GRADIENT;
				    pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_GRADIENT;
				    --rC.CToken;
				    continue;
			    }
				case S_PALETTE_FUNCTIONS: // "func$tions"
					{
						CheckTransform(rC, transform_defined);
						SetPaletteFunction(rC);
						SmPalette.colorMode = SMPAL_COLOR_MODE_FUNCTIONS;
						pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_FUNCTIONS;
						--rC.CToken;
						continue;
					}
				case S_PALETTE_MODEL: // "mo$del"
					{
						++rC.CToken;
						if(rC.EndOfCommand())
							IntErrorCurToken("expected color model");
						const int model = rC.LookupTable(&color_model_tbl[0], rC.CToken);
						if(model == -1)
							IntErrorCurToken("unknown color model");
						SmPalette.cmodel = model;
						continue;
					}
				// ps_allcF: write all rgb formulae into PS file? 
				case S_PALETTE_NOPS_ALLCF: // "nops_allcF" 
				    SmPalette.ps_allcF = false;
				    continue;
				case S_PALETTE_PS_ALLCF: // "ps_allcF" 
				    SmPalette.ps_allcF = true;
				    continue;
				// max colors used 
				case S_PALETTE_MAXCOLORS: { // "maxc$olors" 
				    rC.CToken++;
				    int i = rC.IntExpression();
				    if(i < 0) 
						IntErrorCurToken("non-negative number required");
				    SmPalette.use_maxcolors = i;
				    --rC.CToken;
				    continue;
			    }
			}
			IntErrorCurToken("invalid palette option");
		}
	}
	if(named_color && SmPalette.cmodel != C_MODEL_RGB && IsInteractive)
		int_warn(NO_CARET, "Named colors will produce strange results if not in color mode RGB.");
	// Invalidate previous palette 
	invalidate_palette();
}
//
// process 'set colorbox' command 
//
//static void set_colorbox()
void GpGadgets::SetColorbox(GpCommand & rC)
{
	rC.CToken++;

	if(rC.EndOfCommand()) /* reset to default position */
		ColorBox.where = SMCOLOR_BOX_DEFAULT;
	else { /* go through all options of 'set colorbox' */
		for(; !rC.EndOfCommand(); rC.CToken++) {
			switch(rC.LookupTable(&set_colorbox_tbl[0], rC.CToken)) {
				/* vertical or horizontal color gradient */
				case S_COLORBOX_VERTICAL: /* "v$ertical" */
				    ColorBox.rotation = 'v';
				    continue;
				case S_COLORBOX_HORIZONTAL: /* "h$orizontal" */
				    ColorBox.rotation = 'h';
				    continue;
				/* color box where: default position */
				case S_COLORBOX_DEFAULT: /* "def$ault" */
				    ColorBox.where = SMCOLOR_BOX_DEFAULT;
				    continue;
				/* color box where: position by user */
				case S_COLORBOX_USER: /* "u$ser" */
				    ColorBox.where = SMCOLOR_BOX_USER;
				    continue;
				/* color box layer: front or back */
				case S_COLORBOX_FRONT: /* "fr$ont" */
				    ColorBox.layer = LAYER_FRONT;
				    continue;
				case S_COLORBOX_BACK: /* "ba$ck" */
				    ColorBox.layer = LAYER_BACK;
				    continue;
				/* border of the color box */
				case S_COLORBOX_BORDER: /* "bo$rder" */

				    ColorBox.border = 1;
				    rC.CToken++;

				    if(!rC.EndOfCommand()) {
					    /* expecting a border line type */
					    ColorBox.border_lt_tag = rC.IntExpression();
					    if(ColorBox.border_lt_tag <= 0) {
						    ColorBox.border_lt_tag = 0;
						    IntErrorCurToken("tag must be strictly positive (see `help set style line')");
					    }
					    --rC.CToken;
				    }
				    continue;
				case S_COLORBOX_BDEFAULT: /* "bd$efault" */
				    ColorBox.border_lt_tag = -1; /* use default border */
				    continue;
				case S_COLORBOX_NOBORDER: /* "nobo$rder" */
				    ColorBox.border = 0;
				    continue;
				/* colorbox origin */
				case S_COLORBOX_ORIGIN: /* "o$rigin" */
				    rC.CToken++;
				    if(rC.EndOfCommand()) {
					    IntErrorCurToken("expecting screen value [0 - 1]");
				    }
				    else {
					    // FIXME: should be 2 but old save files may have 3 
					    GetPositionDefault(rC, &ColorBox.origin, screen, 3);
				    }
				    rC.CToken--;
				    continue;
				/* colorbox size */
				case S_COLORBOX_SIZE: /* "s$ize" */
				    rC.CToken++;
				    if(rC.EndOfCommand()) {
					    IntErrorCurToken("expecting screen value [0 - 1]");
				    }
				    else {
					    // FIXME: should be 2 but old save files may have 3 
					    GetPositionDefault(rC, &ColorBox.size, screen, 3);
				    }
				    rC.CToken--;
				    continue;
			}
			IntErrorCurToken("invalid colorbox option");
		}
		if(ColorBox.where == SMCOLOR_BOX_NO) // default: draw at default position
			ColorBox.where = SMCOLOR_BOX_DEFAULT;
	}
}
//
// process 'set pm3d' command
//
//static void set_pm3d()
void GpGadgets::SetPm3D(GpCommand & rC)
{
	int c_token0 = ++rC.CToken;
	if(rC.EndOfCommand()) { /* assume default settings */
		pm3d_reset(); /* sets pm3d.implicit to PM3D_EXPLICIT and pm3d.where to "s" */
		Pm3D.implicit = PM3D_IMPLICIT; /* for historical reasons */
	}
	else { // go through all options of 'set pm3d' 
		for(; !rC.EndOfCommand(); rC.CToken++) {
			switch(rC.LookupTable(&set_pm3d_tbl[0], rC.CToken)) {
				// where to plot 
				case S_PM3D_AT: /* "at" */
				    rC.CToken++;
				    if(get_pm3d_at_option(rC, &Pm3D.where[0]))
					    return;  /* error */
				    rC.CToken--;
#if 1
				    if(rC.CToken == c_token0+1)
					    // for historical reasons: if "at" is the first option of pm3d,
					    // like "set pm3d at X other_opts;", then implicit is switched on
					    Pm3D.implicit = PM3D_IMPLICIT;
#endif
				    continue;
				case S_PM3D_INTERPOLATE: /* "interpolate" */
				    rC.CToken++;
				    if(rC.EndOfCommand()) {
					    IntErrorCurToken("expecting step values i,j");
				    }
				    else {
					    Pm3D.interp_i = rC.IntExpression();
					    if(!rC.Eq(","))
						    IntErrorCurToken("',' expected");
					    rC.CToken++;
					    Pm3D.interp_j = rC.IntExpression();
					    rC.CToken--;
				    }
				    continue;
				// forward and backward drawing direction 
				case S_PM3D_SCANSFORWARD: // "scansfor$ward"
				    Pm3D.direction = PM3D_SCANS_FORWARD;
				    continue;
				case S_PM3D_SCANSBACKWARD: // "scansback$ward" 
				    Pm3D.direction = PM3D_SCANS_BACKWARD;
				    continue;
				case S_PM3D_SCANS_AUTOMATIC: // "scansauto$matic" 
				    Pm3D.direction = PM3D_SCANS_AUTOMATIC;
				    continue;
				case S_PM3D_DEPTH: // "dep$thorder" 
				    Pm3D.direction = PM3D_DEPTH;
				    continue;
				// flush scans: left, right or center 
				case S_PM3D_FLUSH: // "fl$ush" 
				    rC.CToken++;
				    if(rC.AlmostEq("b$egin"))
					    Pm3D.flush = PM3D_FLUSH_BEGIN;
				    else if(rC.AlmostEq("c$enter"))
					    Pm3D.flush = PM3D_FLUSH_CENTER;
				    else if(rC.AlmostEq("e$nd"))
					    Pm3D.flush = PM3D_FLUSH_END;
				    else
					    IntErrorCurToken("expecting flush 'begin', 'center' or 'end'");
				    continue;
				// clipping method 
				case S_PM3D_CLIP_1IN: /* "clip1$in" */
				    Pm3D.clip = PM3D_CLIP_1IN;
				    continue;
				case S_PM3D_CLIP_4IN: /* "clip4$in" */
				    Pm3D.clip = PM3D_CLIP_4IN;
				    continue;
				// setup everything for plotting a map 
				case S_PM3D_MAP: /* "map" */
				    Pm3D.where[0] = 'b'; 
					Pm3D.where[1] = 0; /* set pm3d at b */
				    DataStyle = PM3DSURFACE;
				    FuncStyle = PM3DSURFACE;
				    splot_map = true;
				    continue;
				// flushing triangles 
				case S_PM3D_FTRIANGLES: // "ftr$iangles" 
				    Pm3D.ftriangles = 1;
				    continue;
				case S_PM3D_NOFTRIANGLES: /* "noftr$iangles" */
				    Pm3D.ftriangles = 0;
				    continue;
				/* deprecated pm3d "hidden3d" option, now used for borders */
				case S_PM3D_HIDDEN:
				    if(rC.IsANumber(rC.CToken+1)) {
					    rC.CToken++;
					    load_linetype(&Pm3D.border, rC.IntExpression());
					    rC.CToken--;
					    continue;
				    }
				/* fall through */
				case S_PM3D_BORDER: /* border {linespec} */
				    rC.CToken++;
				    Pm3D.border.SetDefault2(); // = default_pm3d_border;
				    LpParse(rC, Pm3D.border, LP_ADHOC, false);
				    rC.CToken--;
				    continue;
				case S_PM3D_NOHIDDEN:
				case S_PM3D_NOBORDER:
				    Pm3D.border.l_type = LT_NODRAW;
				    continue;
				case S_PM3D_SOLID: /* "so$lid" */
				case S_PM3D_NOTRANSPARENT: /* "notr$ansparent" */
				case S_PM3D_NOSOLID: /* "noso$lid" */
				case S_PM3D_TRANSPARENT: /* "tr$ansparent" */
				    if(IsInteractive)
					    int_warn(rC.CToken, "Deprecated syntax --- ignored");
				case S_PM3D_IMPLICIT: /* "i$mplicit" */
				case S_PM3D_NOEXPLICIT: /* "noe$xplicit" */
				    Pm3D.implicit = PM3D_IMPLICIT;
				    continue;
				case S_PM3D_NOIMPLICIT: /* "noi$mplicit" */
				case S_PM3D_EXPLICIT: /* "e$xplicit" */
				    Pm3D.implicit = PM3D_EXPLICIT;
				    continue;
				case S_PM3D_WHICH_CORNER: /* "corners2color" */
				    rC.CToken++;
				    if(rC.Eq("mean"))
					    Pm3D.which_corner_color = PM3D_WHICHCORNER_MEAN;
				    else if(rC.Eq("geomean"))
					    Pm3D.which_corner_color = PM3D_WHICHCORNER_GEOMEAN;
				    else if(rC.Eq("harmean"))
					    Pm3D.which_corner_color = PM3D_WHICHCORNER_HARMEAN;
				    else if(rC.Eq("median"))
					    Pm3D.which_corner_color = PM3D_WHICHCORNER_MEDIAN;
				    else if(rC.Eq("min"))
					    Pm3D.which_corner_color = PM3D_WHICHCORNER_MIN;
				    else if(rC.Eq("max"))
					    Pm3D.which_corner_color = PM3D_WHICHCORNER_MAX;
				    else if(rC.Eq("rms"))
					    Pm3D.which_corner_color = PM3D_WHICHCORNER_RMS;
				    else if(rC.Eq("c1"))
					    Pm3D.which_corner_color = PM3D_WHICHCORNER_C1;
				    else if(rC.Eq("c2"))
					    Pm3D.which_corner_color = PM3D_WHICHCORNER_C2;
				    else if(rC.Eq("c3"))
					    Pm3D.which_corner_color = PM3D_WHICHCORNER_C3;
				    else if(rC.Eq("c4"))
					    Pm3D.which_corner_color = PM3D_WHICHCORNER_C4;
				    else
					    IntErrorCurToken("expecting 'mean', 'geomean', 'harmean', 'median', 'min', 'max', 'c1', 'c2', 'c3' or 'c4'");
				    continue;
				case S_PM3D_NOLIGHTING_MODEL:
				    Pm3DShade.strength = 0.0;
				    continue;
				case S_PM3D_LIGHTING_MODEL:
				    ParseLightingOptions(rC);
				    continue;
			}
			IntErrorCurToken("invalid pm3d option");
		}
		if(PM3D_SCANS_AUTOMATIC == Pm3D.direction && PM3D_FLUSH_BEGIN != Pm3D.flush) {
			Pm3D.direction = PM3D_SCANS_FORWARD;
			// FIXME: Why isn't this combination supported? 
			FPRINTF((stderr, "pm3d: `scansautomatic' and `flush %s' are incompatible\n", PM3D_FLUSH_END == Pm3D.flush ? "end" : "center"));
		}
	}
}
//
// process 'set GpGg.pointintervalbox' command
//
//static void set_pointintervalbox()
void GpGadgets::SetPointIntervalBox(GpCommand & rC)
{
	rC.CToken++;
	PtIntervalBox = rC.EndOfCommand() ? 1.0 : rC.RealExpression();
	if(PtIntervalBox <= 0)
		PtIntervalBox = 1.0;
}
//
// process 'set pointsize' command
//
//static void set_pointsize()
void GpGadgets::SetPointSize(GpCommand & rC)
{
	rC.CToken++;
	PtSz = (rC.EndOfCommand()) ? 1.0 : rC.RealExpression();
	if(PtSz <= 0)
		PtSz = 1.0;
}
//
// process 'set polar' command
//
//static void set_polar()
void GpGadgets::SetPolar(GpCommand & rC)
{
	rC.CToken++;
	if(!IsPolar) {
		IsPolar = true;
		if(!IsParametric) {
			if(IsInteractive)
				fprintf(stderr, "\n\tdummy variable is t for curves\n");
			strcpy(rC.P.SetDummyVar[0], "t");
		}
		if(AxA[T_AXIS].SetAutoScale) {
			AxA[T_AXIS].SetRange.Set(0.0/* only if user has not set a range manually */, 2 * M_PI / Ang2Rad/* 360 if degrees, 2pi if radians */);
		}
		if(AxA[POLAR_AXIS].SetAutoScale != AUTOSCALE_BOTH)
			RRangeToXY();
	}
}

#ifdef EAM_OBJECTS
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
void GpGadgets::SetObject(GpCommand & rC)
{
	int tag;
	// The next token must either be a tag or the object type
	rC.CToken++;
	if(rC.AlmostEq("rect$angle") || rC.AlmostEq("ell$ipse") ||  rC.AlmostEq("circ$le") || rC.AlmostEq("poly$gon"))
		tag = -1;  /* We'll figure out what it really is later */
	else {
		tag = rC.IntExpression();
		if(tag <= 0)
			IntErrorCurToken("tag must be > zero");
	}
	if(rC.AlmostEq("rect$angle")) {
		SetObj(rC, tag, OBJ_RECTANGLE);
	}
	else if(rC.AlmostEq("ell$ipse")) {
		SetObj(rC, tag, OBJ_ELLIPSE);
	}
	else if(rC.AlmostEq("circ$le")) {
		SetObj(rC, tag, OBJ_CIRCLE);
	}
	else if(rC.AlmostEq("poly$gon")) {
		SetObj(rC, tag, OBJ_POLYGON);
	}
	else if(tag > 0) {
		/* Look for existing object with this tag */
		t_object * this_object = first_object;
		for(; this_object != NULL; this_object = this_object->next)
			if(tag == this_object->tag)
				break;
		if(this_object && tag == this_object->tag) {
			rC.CToken--;
			SetObj(rC, tag, this_object->object_type);
		}
		else
			IntErrorCurToken("unknown object");
	}
	else
		IntErrorCurToken("unrecognized object type");
}

static t_object * new_object(int tag, int object_type, t_object * pNew)
{
	t_object def_rect(t_object::defRectangle);
	t_object def_ellipse(t_object::defEllipse);
	t_object def_circle(t_object::defCircle);
	t_object def_polygon(t_object::defPolygon);

	if(!pNew)
		pNew = (t_object *)malloc(sizeof(t_object));
	else if(pNew->object_type == OBJ_POLYGON)
		free(pNew->o.polygon.vertex);
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
		GpGg.IntErrorNoCaret("object initialization failure");
	pNew->tag = tag;
	pNew->object_type = object_type;
	return pNew;
}

//static void set_obj(int tag, int obj_type)
void GpGadgets::SetObj(GpCommand & rC, int tag, int obj_type)
{
	t_rectangle * this_rect = NULL;
	t_ellipse * this_ellipse = NULL;
	t_circle * this_circle = NULL;
	t_polygon * p_polygon = NULL;
	t_object * p_obj = NULL;
	t_object * new_obj = NULL;
	t_object * prev_object = NULL;
	bool got_fill = false;
	bool got_lt = false;
	bool got_fc = false;
	bool got_corners = false;
	bool got_center = false;
	bool got_origin = false;

	rC.CToken++;
	//
	// We are setting the default, not any particular rectangle 
	//
	if(tag < -1) {
		rC.CToken--;
		if(obj_type == OBJ_RECTANGLE) {
			p_obj = &DefaultRectangle;
			this_rect = &p_obj->o.rectangle;
		}
		else
			IntErrorCurToken("Unknown object type");
	}
	else {
		// Look for existing object with this tag 
		for(p_obj = first_object; p_obj; prev_object = p_obj, p_obj = p_obj->next) {
			// is this the one we want? 
			if(0 < tag  &&  tag <= p_obj->tag)
				break;
		}
		// Insert this rect into the list if it is a new one 
		if(p_obj == NULL || tag != p_obj->tag) {
			if(tag == -1)
				tag = (prev_object) ? prev_object->tag+1 : 1;
			new_obj = new_object(tag, obj_type, NULL);
			if(prev_object == NULL)
				first_object = new_obj;
			else
				prev_object->next = new_obj;
			new_obj->next = p_obj;
			p_obj = new_obj;
			// V5 CHANGE: Apply default rectangle style now rather than later 
			if(obj_type == OBJ_RECTANGLE) {
				p_obj->fillstyle = DefaultRectangle.fillstyle;
				p_obj->lp_properties = DefaultRectangle.lp_properties;
			}
		}
		// Over-write old object if the type has changed 
		else if(p_obj->object_type != obj_type) {
			t_object * save_link = p_obj->next;
			new_obj = new_object(tag, obj_type, p_obj);
			p_obj->next = save_link;
		}
		this_rect = &p_obj->o.rectangle;
		this_ellipse = &p_obj->o.ellipse;
		this_circle = &p_obj->o.circle;
		p_polygon = &p_obj->o.polygon;
	}
	while(!rC.EndOfCommand()) {
		const int save_token = rC.CToken;
		switch(obj_type) {
			case OBJ_RECTANGLE:
			    if(rC.Eq("from")) {
				    // Read in the bottom left and upper right corners 
				    rC.CToken++;
				    GetPosition(rC, &this_rect->bl);
				    if(rC.Eq("to")) {
					    rC.CToken++;
					    GetPosition(rC, &this_rect->tr);
				    }
				    else if(rC.Eq("rto")) {
					    rC.CToken++;
					    GetPositionDefault(rC, &this_rect->tr, this_rect->bl.scalex, 2);
					    if(this_rect->bl.scalex != this_rect->tr.scalex || this_rect->bl.scaley != this_rect->tr.scaley)
						    IntErrorCurToken("relative coordinates must match in type");
					    this_rect->tr.x += this_rect->bl.x;
					    this_rect->tr.y += this_rect->bl.y;
				    }
				    else
					    IntErrorCurToken("Expecting to or rto");
				    got_corners = true;
				    this_rect->type = 0;
				    continue;
			    }
			    else if(rC.Eq("at") || rC.AlmostEq("cen$ter")) {
				    // Read in the center position 
				    rC.CToken++;
				    GetPosition(rC, &this_rect->center);
				    got_center = true;
				    this_rect->type = 1;
				    continue;
			    }
			    else if(rC.Eq("size")) {
				    // Read in the width and height 
				    rC.CToken++;
				    GetPosition(rC, &this_rect->extent);
				    got_center = true;
				    this_rect->type = 1;
				    continue;
			    }
			    break;

			case OBJ_CIRCLE:
			    if(rC.Eq("at") || rC.AlmostEq("cen$ter")) {
				    // Read in the center position 
				    rC.CToken++;
				    GetPosition(rC, &this_circle->center);
				    continue;
			    }
			    else if(rC.Eq("size") || rC.Eq("radius")) {
				    // Read in the radius 
				    rC.CToken++;
				    GetPosition(rC, &this_circle->extent);
				    continue;
			    }
			    else if(rC.Eq("arc")) {
				    // Start and end angle for arc 
				    if(rC.Eq(++rC.CToken, "[")) {
					    rC.CToken++;
					    double arc = rC.RealExpression();
					    if(fabs(arc) > 1000.)
						    IntError(rC.CToken-1, "Angle out of range");
					    else
						    this_circle->arc_begin = arc;
					    if(rC.Eq(rC.CToken++, ":")) {
						    arc = rC.RealExpression();
						    if(fabs(arc) > 1000.)
							    IntError(rC.CToken-1, "Angle out of range");
						    else
							    this_circle->arc_end = arc;
						    if(rC.Eq(rC.CToken++, "]"))
							    continue;
					    }
				    }
				    GpGg.IntError(--rC.CToken, "Expecting arc [<begin>:<end>]");
			    }
			    else if(rC.Eq("wedge")) {
				    rC.CToken++;
				    this_circle->wedge = true;
				    continue;
			    }
			    else if(rC.Eq("nowedge")) {
				    rC.CToken++;
				    this_circle->wedge = false;
				    continue;
			    }
			    break;

			case OBJ_ELLIPSE:
			    if(rC.Eq("at") || rC.AlmostEq("cen$ter")) {
				    /* Read in the center position */
				    rC.CToken++;
				    GetPosition(rC, &this_ellipse->center);
				    continue;
			    }
			    else if(rC.Eq("size")) {
				    /* Read in the width and height */
				    rC.CToken++;
				    GetPosition(rC, &this_ellipse->extent);
				    continue;
			    }
			    else if(rC.AlmostEq("ang$le")) {
				    rC.CToken++;
				    this_ellipse->orientation = rC.RealExpression();
				    continue;
			    }
			    else if(rC.AlmostEq("unit$s")) {
				    rC.CToken++;
				    if(rC.Eq("xy") || rC.EndOfCommand())
					    this_ellipse->type = ELLIPSEAXES_XY;
				    else if(rC.Eq("xx"))
					    this_ellipse->type = ELLIPSEAXES_XX;
				    else if(rC.Eq("yy"))
					    this_ellipse->type = ELLIPSEAXES_YY;
				    else {
					    IntErrorCurToken("expecting 'xy', 'xx' or 'yy'");
				    }
				    rC.CToken++;
				    continue;
			    }
			    break;

			case OBJ_POLYGON:
			    if(rC.Eq("from")) {
				    rC.CToken++;
				    p_polygon->vertex = (GpPosition *)gp_realloc(p_polygon->vertex, sizeof(GpPosition), "polygon vertex");
				    GetPosition(rC, &p_polygon->vertex[0]);
				    p_polygon->type = 1;
				    got_origin = true;
				    continue;
			    }
			    if(!got_corners && (rC.Eq("to") || rC.Eq("rto"))) {
				    while(rC.Eq("to") || rC.Eq("rto")) {
					    if(!got_origin)
						    goto polygon_error;
					    p_polygon->vertex = (GpPosition *)gp_realloc(p_polygon->vertex, (p_polygon->type+1) * sizeof(GpPosition), "polygon vertex");
					    if(rC.Eq(rC.CToken++, "to")) {
						    GetPosition(rC, &p_polygon->vertex[p_polygon->type]);
					    }
					    else { /* "rto" */
						    int v = p_polygon->type;
						    GetPositionDefault(rC, &p_polygon->vertex[v], p_polygon->vertex->scalex, 2);
						    if(p_polygon->vertex[v].scalex != p_polygon->vertex[v-1].scalex ||  p_polygon->vertex[v].scaley != p_polygon->vertex[v-1].scaley)
							    IntErrorCurToken("relative coordinates must match in type");
						    p_polygon->vertex[v].x += p_polygon->vertex[v-1].x;
						    p_polygon->vertex[v].y += p_polygon->vertex[v-1].y;
					    }
					    p_polygon->type++;
					    got_corners = true;
				    }
				    if(got_corners && memcmp(&p_polygon->vertex[p_polygon->type-1], &p_polygon->vertex[0], sizeof(GpPosition))) {
					    fprintf(stderr, "Polygon is not closed - adding extra vertex\n");
					    p_polygon->vertex = (GpPosition *)gp_realloc(p_polygon->vertex, (p_polygon->type+1) * sizeof(GpPosition), "polygon vertex");
					    p_polygon->vertex[p_polygon->type] = p_polygon->vertex[0];
					    p_polygon->type++;
				    }
				    continue;
			    }
			    break;
polygon_error:
			    ZFREE(p_polygon->vertex);
			    p_polygon->type = 0;
			    IntErrorCurToken("Unrecognized polygon syntax");
			// End of polygon options 
			default:
			    IntErrorCurToken("unrecognized object type");
		}
		// The rest of the options apply to any type of object 
		if(rC.Eq("front")) {
			p_obj->layer = LAYER_FRONT;
			rC.CToken++;
			continue;
		}
		else if(rC.Eq("back")) {
			p_obj->layer = LAYER_BACK;
			rC.CToken++;
			continue;
		}
		else if(rC.Eq("behind")) {
			p_obj->layer = LAYER_BEHIND;
			rC.CToken++;
			continue;
		}
		else if(rC.AlmostEq("def$ault")) {
			if(tag < 0) {
				IntErrorCurToken("Invalid command - did you mean 'unset style rectangle'?");
			}
			else {
				p_obj->lp_properties.l_type = LT_DEFAULT;
				p_obj->fillstyle.fillstyle = FS_DEFAULT;
			}
			got_fill = got_lt = true;
			rC.CToken++;
			continue;
		}
		else if(rC.Eq("clip")) {
			p_obj->clip = OBJ_CLIP;
			rC.CToken++;
			continue;
		}
		else if(rC.Eq("noclip")) {
			p_obj->clip = OBJ_NOCLIP;
			rC.CToken++;
			continue;
		}
		// Now parse the style options; default to whatever the global style is  
		if(!got_fill) {
			fill_style_type * p_def_style = (p_obj->object_type == OBJ_RECTANGLE) ? &DefaultRectangle.fillstyle : &DefaultFillStyle;
			if(new_obj)
				rC.ParseFillStyle(&p_obj->fillstyle, p_def_style->fillstyle,
				    p_def_style->filldensity, p_def_style->fillpattern, p_def_style->border_color);
			else
				rC.ParseFillStyle(&p_obj->fillstyle, p_obj->fillstyle.fillstyle,
				    p_obj->fillstyle.filldensity, p_obj->fillstyle.fillpattern, p_obj->fillstyle.border_color);
			if(rC.CToken != save_token) {
				got_fill = true;
				continue;
			}
		}
		// Parse the colorspec 
		if(!got_fc) {
			if(rC.Eq("fc") || rC.AlmostEq("fillc$olor")) {
				p_obj->lp_properties.l_type = LT_BLACK; /* Anything but LT_DEFAULT */
				rC.ParseColorSpec(&p_obj->lp_properties.pm3d_color, TC_FRAC);
				if(p_obj->lp_properties.pm3d_color.type == TC_DEFAULT)
					p_obj->lp_properties.l_type = LT_DEFAULT;
			}
			if(rC.CToken != save_token) {
				got_fc = true;
				continue;
			}
		}
		// Line properties (will be used for the object border if the fillstyle has one.
		// LP_NOFILL means don't eat fillcolor here since at is set separately with "fc".
		if(!got_lt) {
			lp_style_type lptmp = p_obj->lp_properties;
			LpParse(rC, lptmp, LP_NOFILL, false);
			if(rC.CToken != save_token) {
				p_obj->lp_properties.l_width = lptmp.l_width;
				p_obj->lp_properties.d_type = lptmp.d_type;
				p_obj->lp_properties.custom_dash_pattern = lptmp.custom_dash_pattern;
				got_lt = true;
				continue;
			}
		}
		IntErrorCurToken("Unrecognized or duplicate option");
	}
	if(got_center && got_corners)
		GpGg.IntErrorNoCaret("Inconsistent options");
}

#endif
//
// process 'set samples' command 
//
//static void set_samples()
void GpGadgets::SetSamples(GpCommand & rC)
{
	rC.CToken++;
	int tsamp1 = abs(rC.IntExpression());
	int tsamp2 = tsamp1;
	if(!rC.EndOfCommand()) {
		if(!rC.Eq(","))
			IntErrorCurToken("',' expected");
		rC.CToken++;
		tsamp2 = abs(rC.IntExpression());
	}
	if(tsamp1 < 2 || tsamp2 < 2)
		IntErrorCurToken("sampling rate must be > 1; sampling unchanged");
	else {
		SurfacePoints * f_3dp = P_First3DPlot;
		P_First3DPlot = NULL;
		sp_free(f_3dp);
		Samples1 = tsamp1;
		Samples2 = tsamp2;
	}
}
//
// process 'set size' command 
//
//static void set_size()
void GpGadgets::SetSize(GpCommand & rC)
{
	rC.CToken++;
	if(rC.EndOfCommand()) {
		XSz = 1.0;
		YSz = 1.0;
	}
	else {
		if(rC.AlmostEq("sq$uare")) {
			AspectRatio = 1.0;
			++rC.CToken;
		}
		else if(rC.AlmostEq("ra$tio")) {
			++rC.CToken;
			AspectRatio = (float)rC.RealExpression();
		}
		else if(rC.AlmostEq("nora$tio") || rC.AlmostEq("nosq$uare")) {
			AspectRatio = 0.0;
			++rC.CToken;
		}
		if(!rC.EndOfCommand()) {
			XSz = (float)rC.RealExpression();
			if(rC.Eq(",")) {
				rC.CToken++;
				YSz = (float)rC.RealExpression();
			}
			else {
				YSz = XSz;
			}
		}
	}
	if(XSz <= 0 || YSz <=0) {
		XSz = YSz = 1.0;
		GpGg.IntErrorNoCaret("Illegal value for size");
	}
}
//
// process 'set style' command 
//
//static void set_style()
void GpGadgets::SetStyle(GpCommand & rC)
{
	rC.CToken++;
	switch(rC.LookupTable(&show_style_tbl[0], rC.CToken)) {
		case SHOW_STYLE_DATA:
		    DataStyle = get_style(rC);
		    if(DataStyle == FILLEDCURVES) {
			    get_filledcurves_style_options(rC, &FilledcurvesOptsData);
			    if(!FilledcurvesOptsData.opt_given) // default value 
				    FilledcurvesOptsData.closeto = FILLEDCURVES_CLOSED;
		    }
		    break;
		case SHOW_STYLE_FUNCTION:
	    {
		    enum PLOT_STYLE temp_style = get_style(rC);
		    if((temp_style & PLOT_STYLE_HAS_ERRORBAR) || oneof6(temp_style, LABELPOINTS, HISTOGRAMS, IMAGE, RGBIMAGE, RGBA_IMAGE, PARALLELPLOT))
			    IntErrorCurToken("style not usable for function plots, left unchanged");
		    else
			    FuncStyle = temp_style;
		    if(FuncStyle == FILLEDCURVES) {
			    get_filledcurves_style_options(rC, &FilledcurvesOptsFunc);
			    if(!FilledcurvesOptsFunc.opt_given) /* default value */
				    FilledcurvesOptsFunc.closeto = FILLEDCURVES_CLOSED;
		    }
		    break;
	    }
		case SHOW_STYLE_LINE:
		    SetLineStyle(rC, &first_linestyle, LP_STYLE);
		    break;
		case SHOW_STYLE_FILLING:
		    rC.ParseFillStyle(&DefaultFillStyle,
		    DefaultFillStyle.fillstyle,
		    DefaultFillStyle.filldensity,
		    DefaultFillStyle.fillpattern,
		    DefaultFillStyle.border_color);
		    break;
		case SHOW_STYLE_ARROW:
		    SetArrowStyle(rC);
		    break;
#ifdef EAM_OBJECTS
		case SHOW_STYLE_RECTANGLE:
		    rC.CToken++;
		    SetObj(rC, -2, OBJ_RECTANGLE);
		    break;
		case SHOW_STYLE_CIRCLE:
		    rC.CToken++;
		    while(!rC.EndOfCommand()) {
			    if(rC.AlmostEq("r$adius")) {
				    rC.CToken++;
				    GetPosition(rC, &DefaultCircle.o.circle.extent);
			    }
			    else if(rC.AlmostEq("wedge$s")) {
				    rC.CToken++;
				    DefaultCircle.o.circle.wedge = true;
			    }
			    else if(rC.AlmostEq("nowedge$s")) {
				    rC.CToken++;
				    DefaultCircle.o.circle.wedge = false;
			    }
			    else if(rC.Eq("clip")) {
				    rC.CToken++;
				    DefaultCircle.clip = OBJ_CLIP;
			    }
			    else if(rC.Eq("noclip")) {
				    rC.CToken++;
				    DefaultCircle.clip = OBJ_NOCLIP;
			    }
			    else
				    IntErrorCurToken("unrecognized style option");
		    }
		    break;
		case SHOW_STYLE_ELLIPSE:
		    rC.CToken++;
		    while(!rC.EndOfCommand()) {
			    if(rC.Eq("size")) {
				    rC.CToken++;
				    GetPosition(rC, &DefaultCircle.o.ellipse.extent);
				    rC.CToken--;
			    }
			    else if(rC.AlmostEq("ang$le")) {
				    rC.CToken++;
				    if(rC.IsANumber(rC.CToken) || rC.TypeDdv(rC.CToken) == INTGR || rC.TypeDdv(rC.CToken) == CMPLX) {
					    DefaultEllipse.o.ellipse.orientation = rC.RealExpression();
					    rC.CToken--;
				    }
			    }
			    else if(rC.AlmostEq("unit$s")) {
				    rC.CToken++;
				    if(rC.Eq("xy") || rC.EndOfCommand()) {
					    DefaultEllipse.o.ellipse.type = ELLIPSEAXES_XY;
				    }
				    else if(rC.Eq("xx")) {
					    DefaultEllipse.o.ellipse.type = ELLIPSEAXES_XX;
				    }
				    else if(rC.Eq("yy")) {
					    DefaultEllipse.o.ellipse.type = ELLIPSEAXES_YY;
				    }
				    else {
					    IntErrorCurToken("expecting 'xy', 'xx' or 'yy'");
				    }
			    }
			    else if(rC.Eq("clip")) {
				    rC.CToken++;
				    DefaultEllipse.clip = OBJ_CLIP;
			    }
			    else if(rC.Eq("noclip")) {
				    rC.CToken++;
				    DefaultEllipse.clip = OBJ_NOCLIP;
			    }
			    else
				    IntErrorCurToken("expecting 'units {xy|xx|yy}', 'angle <number>' or 'size <position>'");

			    rC.CToken++;
		    }
		    break;
#endif
		case SHOW_STYLE_HISTOGRAM:
		    parse_histogramstyle(rC, &histogram_opts, HT_CLUSTERED, histogram_opts.gap);
		    break;
#ifdef EAM_BOXED_TEXT
		case SHOW_STYLE_TEXTBOX:
		    rC.CToken++;
		    while(!rC.EndOfCommand()) {
			    if(rC.AlmostEq("op$aque")) {
				    textbox_opts.opaque = true;
				    rC.CToken++;
			    }
			    else if(rC.AlmostEq("trans$parent")) {
				    textbox_opts.opaque = false;
				    rC.CToken++;
			    }
			    else if(rC.AlmostEq("mar$gins")) {
				    t_value a;

				    rC.CToken++;
				    if(rC.EndOfCommand()) {
					    textbox_opts.xmargin = 1.;
					    textbox_opts.ymargin = 1.;
					    break;
				    }
				    textbox_opts.xmargin = rC.ConstExpress(&a)->Real();
				    if(!rC.Eq(rC.CToken++, ",") || rC.EndOfCommand())
					    break;
				    textbox_opts.ymargin = rC.ConstExpress(&a)->Real();
			    }
			    else if(rC.AlmostEq("nobo$rder")) {
				    rC.CToken++;
				    textbox_opts.noborder = true;
			    }
			    else if(rC.AlmostEq("bo$rder")) {
				    rC.CToken++;
				    textbox_opts.noborder = false;
			    }
			    else
				    IntErrorCurToken("unrecognized option");
		    }
		    break;
#endif
		case SHOW_STYLE_INCREMENT:
#if 1 || defined(BACKWARDS_COMPATIBLE)
		    rC.CToken++;
		    if(rC.EndOfCommand() || rC.AlmostEq("def$ault"))
			    prefer_line_styles = false;
		    else if(rC.AlmostEq("u$serstyles"))
			    prefer_line_styles = true;
		    else
			    IntErrorCurToken("unrecognized option");
		    rC.CToken++;
#endif
		    break;
		case SHOW_STYLE_BOXPLOT:
		    SetBoxPlot(rC);
		    break;
		case SHOW_STYLE_PARALLEL:
		    set_style_parallel(rC);
		    break;
		default:
		    IntErrorCurToken("unrecognized option - see 'help set style'");
	}
}
//
// process 'set surface' command 
//
//static void set_surface()
void GpGadgets::SetSurface(GpCommand & rC)
{
	rC.CToken++;
	draw_surface = true;
	implicit_surface = true;
	if(!rC.EndOfCommand()) {
		if(rC.Eq("implicit"))
			;
		else if(rC.Eq("explicit"))
			implicit_surface = false;
		rC.CToken++;
	}
}
//
// process 'set table' command 
//
static void set_table(GpCommand & rC)
{
	char * tablefile;
	rC.CToken++;
	SFile::ZClose(&table_outfile);
	table_var = NULL;
	if(rC.Eq("$") && rC.IsLetter(rC.CToken + 1)) { // datablock 
		// NB: has to come first because rC.TryToGetString will choke on the datablock name 
		table_var = GpGg.Ev.AddUdvByName(rC.ParseDataBlockName());
		gpfree_string(&table_var->udv_value);
		gpfree_datablock(&table_var->udv_value);
		table_var->udv_value.type = DATABLOCK;
		table_var->udv_value.v.data_array = NULL;
	}
	else if((tablefile = rC.TryToGetString())) { // file name 
		// 'set table "foo"' creates a new output file 
		if(!(table_outfile = fopen(tablefile, "w")))
			os_error(rC.CToken, "cannot open table output file");
		free(tablefile);
	}
	table_mode = true;
}
//
// process 'set terminal' comamnd 
//
void GpGadgets::SetTerminal(GpCommand & rC)
{
	rC.CToken++;
	if(IsMultiPlot)
		IntErrorCurToken("You can't change the terminal in multiplot mode");
	if(rC.EndOfCommand()) {
		list_terms();
		screen_ok = false;
	}
	else {
		// `set term push'
		if(rC.Eq("push")) {
			push_terminal(IsInteractive);
			rC.CToken++;
		}
		else {
#ifdef USE_MOUSE
			EventReset(term, (GpEvent *)1); /* cancel zoombox etc. */
#endif
			term_reset();
			// `set term pop' 
			if(rC.Eq("pop")) {
				pop_terminal();
				rC.CToken++;
			}
			else {
				// `set term <normal terminal>'
				// NB: if set_term() exits via int_error() then term will not be changed 
				term = set_term(rC);
				// get optional mode parameters
				// not all drivers reset the option string before
				// strcat-ing to it, so we reset it for them
				*term_options = 0;
				term->options(rC);
				if(IsInteractive && *term_options)
					fprintf(stderr, "Options are '%s'\n", term_options);
				if(term->flags & TERM_MONOCHROME)
					init_monochrome();
			}
		}
	}
}

/*
 * Accept a single terminal option to apply to the current terminal if possible.
 * If the current terminal cannot support this option, we silently ignore it.
 * Only reasonably common terminal options are supported.
 *
 * If necessary, the code in term->options() can detect that it was called
 * from here because in this case almost_equals(GpGg.Gp__C.CToken-1, "termopt$ion");
 */

static void set_termoptions(GpCommand & rC)
{
	bool ok_to_call_terminal = false;
	int save_end_of_line = rC.NumTokens;
	rC.CToken++;
	if(rC.EndOfCommand() || !term)
		return;
	if(rC.AlmostEq("enh$anced") || rC.AlmostEq("noenh$anced")) {
		rC.NumTokens = MIN(rC.NumTokens, rC.CToken+1);
		if(term->enhanced_open)
			ok_to_call_terminal = true;
		else
			rC.CToken++;
	}
	else if(rC.Eq("font") ||  rC.Eq("fname")) {
		rC.NumTokens = MIN(rC.NumTokens, rC.CToken+2);
		ok_to_call_terminal = true;
	}
	else if(rC.Eq("fontscale")) {
		rC.NumTokens = MIN(rC.NumTokens, rC.CToken+2);
		if(term->flags & TERM_FONTSCALE)
			ok_to_call_terminal = true;
		else {
			rC.CToken++;
			rC.RealExpression(); /* Silently ignore the request */
		}
	}
	else if(rC.Eq("lw") || rC.AlmostEq("linew$idth")) {
		rC.NumTokens = MIN(rC.NumTokens, rC.CToken+2);
		if(term->flags & TERM_LINEWIDTH)
			ok_to_call_terminal = true;
		else {
			rC.CToken++;
			rC.RealExpression(); /* Silently ignore the request */
		}
	}
	else if(rC.AlmostEq("dash$ed") || rC.Eq("solid")) {
		rC.NumTokens = MIN(rC.NumTokens, rC.CToken+1);
		if(term->flags & TERM_CAN_DASH)
			ok_to_call_terminal = true;
		else
			rC.CToken++;
	}
	else if(rC.AlmostEq("dashl$ength") || rC.Eq("dl")) {
		rC.NumTokens = MIN(rC.NumTokens, rC.CToken+2);
		if(term->flags & TERM_CAN_DASH)
			ok_to_call_terminal = true;
		else
			rC.CToken += 2;
	}
	else if(!strcmp(term->name, "gif") && rC.Eq("delay") && rC.NumTokens==4) {
		ok_to_call_terminal = true;
	}
	else {
		GpGg.IntErrorCurToken("This option cannot be changed using 'set termoption'");
	}
	if(ok_to_call_terminal) {
		*term_options = 0;
		(term->options)(rC);
	}
	rC.NumTokens = save_end_of_line;
}
//
// process 'set tics' command 
//
//static void set_tics()
void GpGadgets::SetTics(GpCommand & rC)
{
	uint i = 0;
	bool axisset = false;
	bool mirror_opt = false; /* set to true if(no)mirror option specified) */
	++rC.CToken;
	if(rC.EndOfCommand()) {
		for(i = 0; i < AXIS_ARRAY_SIZE; ++i)
			AxA[i].Flags |= GpAxis::fTicIn;
	}
	while(!rC.EndOfCommand()) {
		if(rC.AlmostEq("ax$is")) {
			axisset = true;
			SetTicMode(-1, TICS_ON_AXIS, TICS_ON_BORDER);
			++rC.CToken;
		}
		else if(rC.AlmostEq("bo$rder")) {
			SetTicMode(-1, TICS_ON_BORDER, TICS_ON_AXIS);
			++rC.CToken;
		}
		else if(rC.AlmostEq("mi$rror")) {
			SetTicMode(-1, TICS_MIRROR, 0);
			mirror_opt = true;
			++rC.CToken;
		}
		else if(rC.AlmostEq("nomi$rror")) {
			SetTicMode(-1, 0, TICS_MIRROR);
			mirror_opt = true;
			++rC.CToken;
		}
		else if(rC.AlmostEq("in$wards")) {
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i)
				AxA[i].Flags |= GpAxis::fTicIn;
			++rC.CToken;
		}
		else if(rC.AlmostEq("out$wards")) {
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i)
				AxA[i].Flags &= ~GpAxis::fTicIn;
			++rC.CToken;
		}
		else if(rC.AlmostEq("sc$ale")) {
			SetTicScale(rC);
		}
		else if(rC.AlmostEq("ro$tate")) {
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i) {
				AxA[i].tic_rotate = TEXT_VERTICAL;
			}
			++rC.CToken;
			if(rC.Eq("by")) {
				int langle;
				++rC.CToken;
				langle = rC.IntExpression();
				for(i = 0; i < AXIS_ARRAY_SIZE; ++i)
					AxA[i].tic_rotate = langle;
			}
		}
		else if(rC.AlmostEq("noro$tate")) {
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i)
				AxA[i].tic_rotate = 0;
			++rC.CToken;
		}
		else if(rC.AlmostEq("l$eft")) {
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i) {
				AxA[i].label.pos = LEFT;
				AxA[i].Flags |= GpAxis::fManualJustify;
			}
			rC.CToken++;
		}
		else if(rC.AlmostEq("c$entre") || rC.AlmostEq("c$enter")) {
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i) {
				AxA[i].label.pos = CENTRE;
				AxA[i].Flags |= GpAxis::fManualJustify;
			}
			rC.CToken++;
		}
		else if(rC.AlmostEq("ri$ght")) {
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i) {
				AxA[i].label.pos = RIGHT;
				AxA[i].Flags |= GpAxis::fManualJustify;
			}
			rC.CToken++;
		}
		else if(rC.AlmostEq("autoj$ustify")) {
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i)
				AxA[i].Flags &= ~GpAxis::fManualJustify;
			rC.CToken++;
		}
		else if(rC.AlmostEq("off$set")) {
			GpPosition lpos;
			++rC.CToken;
			GpGg.GetPositionDefault(rC, &lpos, character, 3);
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i)
				AxA[i].ticdef.offset = lpos;
		}
		else if(rC.AlmostEq("nooff$set")) {
			++rC.CToken;
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i)
				AxA[i].ticdef.offset.Set(character, character, character, 0., 0., 0.);
		}
		else if(rC.AlmostEq("format")) {
			SetFormat(rC);
		}
		else if(rC.AlmostEq("enh$anced")) {
			++rC.CToken;
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i)
				AxA[i].ticdef.enhanced = true;
		}
		else if(rC.AlmostEq("noenh$anced")) {
			++rC.CToken;
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i)
				AxA[i].ticdef.enhanced = false;
		}
		else if(rC.AlmostEq("f$ont")) {
			++rC.CToken;
			// Make sure they've specified a font 
			if(!rC.IsStringValue(rC.CToken))
				IntErrorCurToken("expected font");
			else {
				char * lfont = rC.TryToGetString();
				for(i = 0; i < AXIS_ARRAY_SIZE; ++i) {
					free(AxA[i].ticdef.font);
					AxA[i].ticdef.font = gp_strdup(lfont);
				}
				free(lfont);
			}
		}
		else if(rC.Eq("tc") ||
		    rC.AlmostEq("text$color")) {
			t_colorspec lcolor;
			rC.ParseColorSpec(&lcolor, TC_FRAC);
			for(i = 0; i < AXIS_ARRAY_SIZE; ++i)
				AxA[i].ticdef.textcolor = lcolor;
		}
		else if(rC.Eq("front")) {
			grid_tics_in_front = true;
			++rC.CToken;
		}
		else if(rC.Eq("back")) {
			grid_tics_in_front = false;
			++rC.CToken;
		}
		else if(!rC.EndOfCommand()) {
			IntErrorCurToken("extraneous arguments in set tics");
		}
	}
	// if tics are off and not set by axis, reset to default (border)
	for(i = 0; i < AXIS_ARRAY_SIZE; ++i) {
		if(((AxA[i].ticmode & TICS_MASK) == NO_TICS) && (!axisset)) {
			if(!oneof2(i, SECOND_X_AXIS, SECOND_Y_AXIS)) { // don't switch on secondary axes by default
				AxA[i].ticmode = TICS_ON_BORDER;
				if((mirror_opt == false) && oneof3(i, FIRST_X_AXIS, FIRST_Y_AXIS, COLOR_AXIS))
					AxA[i].ticmode |= TICS_MIRROR;
			}
		}
	}
}
//
// process 'set ticslevel' command 
// is datatype 'time' relevant here ? 
//
//static void set_ticslevel()
void GpGadgets::SetTicsLevel(GpCommand & rC)
{
	rC.CToken++;
	xyplane.Set(rC.RealExpression(), false);
}
//
// process 'set xyplane' command 
// is datatype 'time' relevant here ? 
//
//static void set_xyplane()
void GpGadgets::SetXYPlane(GpCommand & rC)
{
	if(rC.Eq(++rC.CToken, "at")) {
		rC.CToken++;
		xyplane.Set(rC.RealExpression(), true);
		return;
	}
	else if(!rC.AlmostEq("rel$ative")) {
		rC.CToken--;
		// int_warn(NO_CARET, "deprecated syntax"); 
	}
	SetTicsLevel(rC);
}

// process 'set timestamp' command 
void GpGadgets::SetTimestamp(GpCommand & rC)
{
	bool got_format = false;
	char * p_new;
	rC.CToken++;
	while(!rC.EndOfCommand()) {
		if(rC.AlmostEq("t$op")) {
			timelabel_bottom = false;
			rC.CToken++;
			continue;
		}
		else if(rC.AlmostEq("b$ottom")) {
			timelabel_bottom = true;
			rC.CToken++;
			continue;
		}
		if(rC.AlmostEq("r$otate")) {
			timelabel_rotate = true;
			rC.CToken++;
			continue;
		}
		else if(rC.AlmostEq("n$orotate")) {
			timelabel_rotate = false;
			rC.CToken++;
			continue;
		}
		if(rC.AlmostEq("off$set")) {
			rC.CToken++;
			GetPositionDefault(rC, &(timelabel.offset), character, 3);
			continue;
		}
		if(rC.Eq("font")) {
			rC.CToken++;
			p_new = rC.TryToGetString();
			free(timelabel.font);
			timelabel.font = p_new;
			continue;
		}
		if(rC.Eq("tc") || rC.AlmostEq("text$color")) {
			rC.ParseColorSpec(&(timelabel.textcolor), TC_VARIABLE);
			continue;
		}
		if(!got_format && ((p_new = rC.TryToGetString()))) {
			/* we have a format string */
			free(timelabel.text);
			timelabel.text = p_new;
			got_format = true;
			continue;
		}
		IntErrorCurToken("unrecognized option");
	}
	SETIFZ(timelabel.text, gp_strdup(DEFAULT_TIMESTAMP_FORMAT));
}
//
// process 'set view' command 
//
void GpGadgets::SetView(GpCommand & rC)
{
	int i;
	bool was_comma = true;
	static const char errmsg1[] = "rot_%c must be in [0:%d] degrees range; view unchanged";
	static const char errmsg2[] = "%sscale must be > 0; view unchanged";
	double local_vals[4];
	rC.CToken++;
	if(rC.Eq("map")) {
		splot_map = true;
		mapview_scale = 1.0;
		rC.CToken++;
		if(rC.Eq("scale")) {
			rC.CToken++;
			mapview_scale = (float)rC.RealExpression();
		}
		return;
	}
	if(splot_map)
		splot_map = 0;  // default is no map
	if(rC.AlmostEq("equal$_axes")) {
		rC.CToken++;
		if(rC.EndOfCommand() || rC.Eq("xy")) {
			AspectRatio3D = 2;
			rC.CToken++;
		}
		else if(rC.Eq("xyz")) {
			AspectRatio3D = 3;
			rC.CToken++;
		}
		return;
	}
	else if(rC.AlmostEq("noequal$_axes")) {
		AspectRatio3D = 0;
		rC.CToken++;
		return;
	}
	local_vals[0] = surface_rot_x;
	local_vals[1] = surface_rot_z;
	local_vals[2] = surface_scale;
	local_vals[3] = surface_zscale;
	for(i = 0; i < 4 && !(rC.EndOfCommand()); ) {
		if(rC.Eq(",")) {
			if(was_comma)
				i++;
			was_comma = true;
			rC.CToken++;
		}
		else {
			if(!was_comma)
				IntErrorCurToken("',' expected");
			local_vals[i] = rC.RealExpression();
			i++;
			was_comma = false;
		}
	}
	if(local_vals[0] < 0 || local_vals[0] > 360)
		IntErrorCurToken(errmsg1, 'x', 360);
	if(local_vals[1] < 0 || local_vals[1] > 360)
		IntErrorCurToken(errmsg1, 'z', 360);
	if(local_vals[2] < 1e-6)
		IntErrorCurToken(errmsg2, "");
	if(local_vals[3] < 1e-6)
		IntErrorCurToken(errmsg2, "z");
	surface_rot_x = (float)local_vals[0];
	surface_rot_z = (float)local_vals[1];
	surface_scale = (float)local_vals[2];
	surface_zscale = (float)local_vals[3];
	surface_lscale = logf(surface_scale);
}
//
// process 'set {x|y|z|x2|y2}data' command 
//
//static void set_timedata(GpAxis * this_axis)
void GpAxis::SetTimeData(GpCommand & rC)
{
	rC.CToken++;
	datatype = DT_NORMAL;
	if(rC.AlmostEq("t$ime")) {
		datatype = DT_TIMEDATE;
		rC.CToken++;
	}
	else if(rC.AlmostEq("geo$graphic")) {
		datatype = DT_DMS;
		rC.CToken++;
	}
	// FIXME: this provides approximate backwards compatibility 
	//        but may be more trouble to explain than it's worth 
	tictype = datatype;
}

//static void set_range(GpCommand & rC, GpAxis * pAx)
void GpGadgets::SetRange(GpCommand & rC, GpAxis * pAx)
{
	rC.CToken++;
	if(rC.AlmostEq("re$store")) {
		rC.CToken++;
		//pAx->SetRange.Set(pAx->writeback_min, pAx->writeback_max);
		pAx->SetRange = pAx->WritebackRange;
		pAx->SetAutoScale = AUTOSCALE_NONE;
	}
	else {
		if(!rC.Eq("["))
			IntErrorCurToken("expecting '[' or 'restore'");
		rC.CToken++;
		pAx->SetAutoScale = pAx->LoadRange(rC, pAx->SetRange, pAx->SetAutoScale);
		if(!rC.Eq("]"))
			IntErrorCurToken("expecting ']'");
		rC.CToken++;
		while(!rC.EndOfCommand()) {
			if(rC.AlmostEq("rev$erse")) {
				++rC.CToken;
				pAx->range_flags |= RANGE_IS_REVERSED;
			}
			else if(rC.AlmostEq("norev$erse")) {
				++rC.CToken;
				pAx->range_flags &= ~RANGE_IS_REVERSED;
			}
			else if(rC.AlmostEq("wr$iteback")) {
				++rC.CToken;
				pAx->range_flags |= RANGE_WRITEBACK;
			}
			else if(rC.AlmostEq("nowri$teback")) {
				++rC.CToken;
				pAx->range_flags &= ~RANGE_WRITEBACK;
			}
			else if(rC.AlmostEq("ext$end")) {
				++rC.CToken;
				pAx->SetAutoScale &= ~(AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX);
			}
			else if(rC.AlmostEq("noext$end")) {
				++rC.CToken;
				pAx->SetAutoScale |= AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX;
			}
			else
				IntErrorCurToken("unrecognized option");
		}
	}
	// If this is one end of a linked axis pair, replicate the new range to the	*/
	// linked axis, possibly via a mapping function.                                */
	if(pAx->P_LinkToScnd)
		clone_linked_axes(pAx, pAx->P_LinkToScnd);
	else if(pAx->P_LinkToPrmr)
		clone_linked_axes(pAx, pAx->P_LinkToPrmr);
}
//
//static void set_raxis() { GpGg.raxis = true; GpGg.Gp__C.CToken++; }
//
// process 'set {xyz}zeroaxis' command 
//
//static void set_zeroaxis(AXIS_INDEX axis)
void GpGadgets::SetZeroAxis(GpCommand & rC, AXIS_INDEX axIdx)
{
	rC.CToken++;
	if(AxA[axIdx].zeroaxis != (void*)(&GpAxis::DefaultAxisZeroAxis))
		free(AxA[axIdx].zeroaxis);
	if(rC.EndOfCommand())
		AxA[axIdx].zeroaxis = (lp_style_type *)(&GpAxis::DefaultAxisZeroAxis);
	else {
		// Some non-default style for the zeroaxis 
		AxA[axIdx].zeroaxis = (lp_style_type *)malloc(sizeof(lp_style_type));
		*(AxA[axIdx].zeroaxis) = GpAxis::DefaultAxisZeroAxis;
		LpParse(rC, *AxA[axIdx].zeroaxis, LP_ADHOC, false);
	}
}
//
// Implements 'set tics' 'set xtics' 'set ytics' etc 
//
//static int set_tic_prop(GpAxis * pAx)
int GpGadgets::SetTicProp(GpAxis & rAx, GpCommand & rC)
{
	int    match = 0; // flag, set by matching a tic command 
	char   nocmd[12]; // fill w/ "no"+axis_name+suffix 
	char * cmdptr = NULL;
	char * sfxptr = NULL;
	AXIS_INDEX axis = (AXIS_INDEX)rAx.Index;
	if(axis < NUMBER_OF_MAIN_VISIBLE_AXES) {
		strcpy(nocmd, "no");
		cmdptr = &nocmd[2];
		strcpy(cmdptr, GetAxisName(axis));
		sfxptr = &nocmd[strlen(nocmd)];
		strcpy(sfxptr, "t$ics"); /* STRING */
	}
	if(rC.AlmostEq(cmdptr) || axis >= PARALLEL_AXES) {
		bool axisset = false;
		bool mirror_opt = false; // set to true if(no)mirror option specified)
		rAx.ticdef.def.mix = false;
		match = 1;
		++rC.CToken;
		do {
			if(rC.AlmostEq("ax$is")) {
				axisset = true;
				rAx.ticmode &= ~TICS_ON_BORDER;
				rAx.ticmode |= TICS_ON_AXIS;
				++rC.CToken;
			}
			else if(rC.AlmostEq("bo$rder")) {
				rAx.ticmode &= ~TICS_ON_AXIS;
				rAx.ticmode |= TICS_ON_BORDER;
				++rC.CToken;
			}
			else if(rC.AlmostEq("mi$rror")) {
				rAx.ticmode |= TICS_MIRROR;
				mirror_opt = true;
				++rC.CToken;
			}
			else if(rC.AlmostEq("nomi$rror")) {
				rAx.ticmode &= ~TICS_MIRROR;
				mirror_opt = true;
				++rC.CToken;
			}
			else if(rC.AlmostEq("in$wards")) {
				rAx.Flags |= GpAxis::fTicIn;
				++rC.CToken;
			}
			else if(rC.AlmostEq("out$wards")) {
				rAx.Flags &= ~GpAxis::fTicIn;
				++rC.CToken;
			}
			else if(rC.AlmostEq("sc$ale")) {
				++rC.CToken;
				if(rC.AlmostEq("def$ault")) {
					rAx.ticscale = 1.0;
					rAx.miniticscale = 0.5;
					++rC.CToken;
				}
				else {
					AxA[axis].ticscale = rC.RealExpression();
					if(rC.Eq(",")) {
						++rC.CToken;
						rAx.miniticscale = rC.RealExpression();
					}
					else
						rAx.miniticscale = 0.5 * rAx.ticscale;
				}
			}
			else if(rC.AlmostEq("ro$tate")) {
				AxA[axis].tic_rotate = TEXT_VERTICAL;
				++rC.CToken;
				if(rC.Eq("by")) {
					rC.CToken++;
					rAx.tic_rotate = rC.IntExpression();
				}
			}
			else if(rC.AlmostEq("noro$tate")) {
				rAx.tic_rotate = 0;
				++rC.CToken;
			}
			else if(rC.AlmostEq("off$set")) {
				++rC.CToken;
				GetPositionDefault(rC, &rAx.ticdef.offset,
				    character, 3);
			}
			else if(rC.AlmostEq("nooff$set")) {
				++rC.CToken;
				rAx.ticdef.offset.Set(character, character, character, 0., 0., 0.);
			}
			else if(rC.AlmostEq("l$eft")) {
				rAx.label.pos = LEFT;
				rAx.Flags |= GpAxis::fManualJustify;
				rC.CToken++;
			}
			else if(rC.AlmostEq("c$entre") || rC.AlmostEq("c$enter")) {
				rAx.label.pos = CENTRE;
				rAx.Flags |= GpAxis::fManualJustify;
				rC.CToken++;
			}
			else if(rC.AlmostEq("ri$ght")) {
				rAx.label.pos = RIGHT;
				rAx.Flags |= GpAxis::fManualJustify;
				rC.CToken++;
			}
			else if(rC.AlmostEq("autoj$ustify")) {
				rAx.Flags &= ~GpAxis::fManualJustify;
				rC.CToken++;
			}
			else if(rC.AlmostEq("range$limited")) {
				rAx.ticdef.rangelimited = true;
				++rC.CToken;
			}
			else if(rC.AlmostEq("norange$limited")) {
				rAx.ticdef.rangelimited = false;
				++rC.CToken;
			}
			else if(rC.AlmostEq("f$ont")) {
				++rC.CToken;
				/* Make sure they've specified a font */
				if(!rC.IsStringValue(rC.CToken))
					IntErrorCurToken("expected font");
				else {
					ZFREE(rAx.ticdef.font);
					rAx.ticdef.font = rC.TryToGetString();
				}

				/* The geographic/timedate/numeric options are new in version 5 */
			}
			else if(rC.AlmostEq("geo$graphic")) {
				++rC.CToken;
				rAx.tictype = DT_DMS;
			}
			else if(rC.AlmostEq("time$date")) {
				++rC.CToken;
				rAx.tictype = DT_TIMEDATE;
			}
			else if(rC.AlmostEq("numeric")) {
				++rC.CToken;
				rAx.tictype = DT_NORMAL;
			}
			else if(rC.Eq("format")) {
				char * format;
				++rC.CToken;
				if(!((format = rC.TryToGetString())))
					IntErrorCurToken("expected format");
				free(rAx.formatstring);
				rAx.formatstring  = format;
			}
			else if(rC.AlmostEq("enh$anced")) {
				++rC.CToken;
				rAx.ticdef.enhanced = true;
			}
			else if(rC.AlmostEq("noenh$anced")) {
				++rC.CToken;
				rAx.ticdef.enhanced = false;
			}
			else if(rC.Eq("tc") || rC.AlmostEq("text$color")) {
				rC.ParseColorSpec(&rAx.ticdef.textcolor, axis == FIRST_Z_AXIS ? TC_Z : TC_FRAC);
			}
			else if(rC.AlmostEq("au$tofreq")) {
				/* auto tic interval */
				++rC.CToken;
				if(!rAx.ticdef.def.mix) {
					ticmark::DestroyList(rAx.ticdef.def.user);
					rAx.ticdef.def.user = NULL;
				}
				rAx.ticdef.type = TIC_COMPUTED;
#ifdef NONLINEAR_AXES
			}
			else if(rC.AlmostEq("log$scale")) {
				++rC.CToken;
				rAx.ticdef.logscaling = true;
			}
			else if(rC.AlmostEq("nolog$scale")) {
				++rC.CToken;
				rAx.ticdef.logscaling = false;
#endif
			}
			else if(rC.Eq("add")) {
				++rC.CToken;
				rAx.ticdef.def.mix = true;
			}
			else if(!rC.EndOfCommand()) {
				//load_tics(&rAx);
				if(rC.Eq("(")) { // set : TIC_USER
					rC.CToken++;
					LoadTicUser(rC, &rAx);
				}
				else // series : TIC_SERIES 
					LoadTicSeries(rC, &rAx);
			}
		} while(!rC.EndOfCommand());
		// if tics are off and not set by axis, reset to default (border)
		if(((rAx.ticmode & TICS_MASK) == NO_TICS) && (!axisset)) {
			if(axis >= PARALLEL_AXES)
				rAx.ticmode |= TICS_ON_AXIS;
			else
				rAx.ticmode |= TICS_ON_BORDER;
			if(!mirror_opt && ((axis == FIRST_X_AXIS) || (axis == FIRST_Y_AXIS) || (axis == COLOR_AXIS))) {
				rAx.ticmode |= TICS_MIRROR;
			}
		}
	}
	// The remaining command options cannot work for parametric or parallel axes
	if(axis >= NUMBER_OF_MAIN_VISIBLE_AXES)
		return match;
	if(rC.AlmostEq(nocmd)) { // NOSTRING
		rAx.ticmode &= ~TICS_MASK;
		rC.CToken++;
		match = 1;
	}
	//
	// other options 
	//
	strcpy(sfxptr, "m$tics"); /* MONTH */
	if(rC.AlmostEq(cmdptr)) {
		if(!rAx.ticdef.def.mix) {
			ticmark::DestroyList(rAx.ticdef.def.user);
			rAx.ticdef.def.user = NULL;
		}
		rAx.ticdef.type = TIC_MONTH;
		++rC.CToken;
		match = 1;
	}
	if(rC.AlmostEq(nocmd)) {     /* NOMONTH */
		rAx.ticdef.type = TIC_COMPUTED;
		++rC.CToken;
		match = 1;
	}
	(void)strcpy(sfxptr, "d$tics"); /* DAYS */
	if(rC.AlmostEq(cmdptr)) {
		match = 1;
		if(!rAx.ticdef.def.mix) {
			ticmark::DestroyList(rAx.ticdef.def.user);
			rAx.ticdef.def.user = NULL;
		}
		rAx.ticdef.type = TIC_DAY;
		++rC.CToken;
	}
	if(rC.AlmostEq(nocmd)) {     /* NODAYS */
		rAx.ticdef.type = TIC_COMPUTED;
		++rC.CToken;
		match = 1;
	}
	*cmdptr = 'm';
	strcpy(cmdptr + 1, GetAxisName(axis));
	strcat(cmdptr, "t$ics"); // MINISTRING 
	if(rC.AlmostEq(cmdptr)) {
		rC.CToken++;
		match = 1;
		if(rC.EndOfCommand()) {
			rAx.minitics = MINI_AUTO;
		}
		else if(rC.AlmostEq("def$ault")) {
			rAx.minitics = MINI_DEFAULT;
			++rC.CToken;
		}
		else {
			int freq = rC.IntExpression();
			if(freq > 0 && freq < 101) {
				rAx.mtic_freq = freq;
				rAx.minitics = MINI_USER;
			}
			else {
				rAx.minitics = MINI_DEFAULT;
				int_warn(rC.CToken-1, "Expecting number of intervals");
			}
		}
	}
	if(rC.AlmostEq(nocmd)) {     /* NOMINI */
		rAx.minitics = MINI_OFF;
		rC.CToken++;
		match = 1;
	}
	return match;
}
//
// process a 'set {x/y/z}label command 
// set {x/y/z}label {label_text} {offset {x}{,y}} {<fontspec>} {<textcolor>} 
//
//static void set_xyzlabel(GpTextLabel * label)
void GpGadgets::SetXYZLabel(GpCommand & rC, GpTextLabel * pLabel)
{
	rC.CToken++;
	if(rC.EndOfCommand()) {    /* no pLabel specified */
		ZFREE(pLabel->text);
	}
	else {
		ParseLabelOptions(rC, pLabel, 0);
		if(!rC.EndOfCommand()) {
			char * text = rC.TryToGetString();
			if(text) {
				free(pLabel->text);
				pLabel->text = text;
			}
		}
		ParseLabelOptions(rC, pLabel, 0);
	}
}

/*
 * Change or insert a new linestyle in a list of line styles.
 * Supports the old 'set linestyle' command (backwards-compatible)
 * and the new "set style line" and "set linetype" commands.
 * destination_class is either LP_STYLE or LP_TYPE.
 */
//static void set_linestyle(linestyle_def ** head, lp_class destination_class)
void GpGadgets::SetLineStyle(GpCommand & rC, linestyle_def ** ppHead, lp_class destinationClass)
{
	linestyle_def * p_linestyle = NULL;
	linestyle_def * p_new_linestyle = NULL;
	linestyle_def * p_prev_linestyle = NULL;
	int tag;
	rC.CToken++;
	// get tag 
	if(rC.EndOfCommand() || ((tag = rC.IntExpression()) <= 0))
		IntErrorCurToken("tag must be > zero");
	// Check if linestyle is already defined 
	for(p_linestyle = *ppHead; p_linestyle; p_prev_linestyle = p_linestyle, p_linestyle = p_linestyle->next)
		if(tag <= p_linestyle->tag)
			break;
	if(p_linestyle == NULL || tag != p_linestyle->tag) {
		// Default style is based on linetype with the same tag id 
		lp_style_type loc_lp;
		loc_lp.SetDefault2();
		loc_lp.l_type = tag - 1;
		loc_lp.p_type = tag - 1;
		loc_lp.d_type = DASHTYPE_SOLID;
		loc_lp.pm3d_color.type = TC_LT;
		loc_lp.pm3d_color.lt = tag - 1;
		p_new_linestyle = (linestyle_def *)malloc(sizeof(linestyle_def));
		if(p_prev_linestyle != NULL)
			p_prev_linestyle->next = p_new_linestyle;  // add it to end of list 
		else
			*ppHead = p_new_linestyle;  // make it start of list 
		p_new_linestyle->tag = tag;
		p_new_linestyle->next = p_linestyle;
		p_new_linestyle->lp_properties = loc_lp;
		p_linestyle = p_new_linestyle;
	}
	if(rC.AlmostEq("def$ault")) {
		delete_linestyle(ppHead, p_prev_linestyle, p_linestyle);
		rC.CToken++;
	}
	else {
		// pick up a line spec; dont allow ls, do allow point type 
		LpParse(rC, p_linestyle->lp_properties, destinationClass, true);
	}
	if(!rC.EndOfCommand())
		IntErrorCurToken("Extraneous arguments to set %s", ppHead == &first_perm_linestyle ? "linetype" : "style line");
}

/*
 * Delete linestyle from linked list.
 * Called with pointers to the head of the list,
 * to the previous linestyle (not strictly necessary),
 * and to the linestyle to delete.
 */
void delete_linestyle(linestyle_def ** head, linestyle_def * prev, linestyle_def * pThis)
{
	if(pThis != NULL) {      /* there really is something to delete */
		if(pThis == *head)
			*head = pThis->next;
		else
			prev->next = pThis->next;
		free(pThis);
	}
}
//
// ======================================================== 
// process a 'set arrowstyle' command 
// set style arrow {tag} {nohead|head|backhead|heads} {size l,a{,b}} {{no}filled} {linestyle...} {layer n}
//
//static void set_arrowstyle()
void GpGadgets::SetArrowStyle(GpCommand & rC)
{
	arrowstyle_def * this_arrowstyle = NULL;
	arrowstyle_def * new_arrowstyle = NULL;
	arrowstyle_def * prev_arrowstyle = NULL;
	arrow_style_type loc_arrow;
	int tag;
	default_arrow_style(&loc_arrow);
	rC.CToken++;
	/* get tag */
	if(!rC.EndOfCommand()) {
		/* must be a tag expression! */
		tag = rC.IntExpression();
		if(tag <= 0)
			IntErrorCurToken("tag must be > zero");
	}
	else
		tag = assign_arrowstyle_tag();  /* default next tag */

	/* search for arrowstyle */
	if(first_arrowstyle != NULL) {  /* skip to last arrowstyle */
		for(this_arrowstyle = first_arrowstyle; this_arrowstyle != NULL;
		    prev_arrowstyle = this_arrowstyle,
		    this_arrowstyle = this_arrowstyle->next)
			/* is this the arrowstyle we want? */
			if(tag <= this_arrowstyle->tag)
				break;
	}

	if(this_arrowstyle == NULL || tag != this_arrowstyle->tag) {
		/* adding the arrowstyle */
		new_arrowstyle = (arrowstyle_def*)malloc(sizeof(arrowstyle_def));
		default_arrow_style(&(new_arrowstyle->arrow_properties));
		if(prev_arrowstyle != NULL)
			prev_arrowstyle->next = new_arrowstyle;  /* add it to end of list */
		else
			first_arrowstyle = new_arrowstyle;  /* make it start of list */
		new_arrowstyle->arrow_properties.tag = tag;
		new_arrowstyle->tag = tag;
		new_arrowstyle->next = this_arrowstyle;
		this_arrowstyle = new_arrowstyle;
	}

	if(rC.EndOfCommand())
		this_arrowstyle->arrow_properties = loc_arrow;
	else if(rC.AlmostEq("def$ault")) {
		this_arrowstyle->arrow_properties = loc_arrow;
		rC.CToken++;
	}
	else // pick up a arrow spec : dont allow arrowstyle 
		arrow_parse(rC, &this_arrowstyle->arrow_properties, false);
	if(!rC.EndOfCommand())
		IntErrorCurToken("extraneous or out-of-order arguments in set arrowstyle");
}

/* assign a new arrowstyle tag
 * arrowstyles are kept sorted by tag number, so this is easy
 * returns the lowest unassigned tag number
 */
static int assign_arrowstyle_tag()
{
	int last = 0; // previous tag value
	for(arrowstyle_def * p_this = GpGg.first_arrowstyle; p_this != NULL; p_this = p_this->next)
		if(p_this->tag == last + 1)
			last++;
		else
			break;
	return (last + 1);
}
//
// load TIC_USER definition
// (tic[,tic]...)
// where tic is ["string"] value [level]
// Left paren is already scanned off before entry.
//
//static void load_tic_user(GpAxis * this_axis)
void GpGadgets::LoadTicUser(GpCommand & rC, GpAxis * pAx)
{
	char * ticlabel;
	double ticposition;
	// Free any old tic labels
	if(!pAx->ticdef.def.mix && !(rC.P.P_SetIterator && rC.P.P_SetIterator->iteration)) {
		ticmark::DestroyList(pAx->ticdef.def.user);
		pAx->ticdef.def.user = NULL;
	}
	// Mark this axis as user-generated ticmarks only, unless the 
	// mix flag indicates that both user- and auto- tics are OK.  
	if(!pAx->ticdef.def.mix)
		pAx->ticdef.type = TIC_USER;
	while(!rC.EndOfCommand() && !rC.Eq(")")) {
		int ticlevel = 0;
		int save_token;
		// syntax is  (  {'format'} value {level} {, ...} )
		// but for timedata, the value itself is a string, which
		// complicates things somewhat
		//
		// has a string with it? 
		save_token = rC.CToken;
		ticlabel = rC.TryToGetString();
		if(ticlabel && pAx->datatype == DT_TIMEDATE && (rC.Eq(",") || rC.Eq(")"))) {
			rC.CToken = save_token;
			ZFREE(ticlabel);
		}
		// in any case get the value
		ticposition = GetNumOrTime(rC, pAx);
		if(!rC.EndOfCommand() && !rC.Eq(",") && !rC.Eq(")")) {
			ticlevel = rC.IntExpression(); // tic level 
		}
		// add to list 
		add_tic_user(pAx, ticlabel, ticposition, ticlevel);
		free(ticlabel);
		// expect "," or ")" here
		if(!rC.EndOfCommand() && rC.Eq(","))
			rC.CToken++;  // loop again 
		else
			break;  // hopefully ")" 
	}
	if(rC.EndOfCommand() || !rC.Eq(")")) {
		ticmark::DestroyList(pAx->ticdef.def.user);
		pAx->ticdef.def.user = NULL;
		IntErrorCurToken("expecting right parenthesis )");
	}
	rC.CToken++;
}
//
// Remove tic labels that were read from a datafile during a previous plot
// via the 'using xtics(n)' mechanism.  These have tick level < 0.
//
ticmark * prune_dataticks(ticmark * list)
{
	ticmark a;
	ticmark * b = &a;
	while(list) {
		if(list->level < 0) {
			free(list->label);
			ticmark * tmp = list->next;
			free(list);
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
//static void load_tic_series(GpAxis * this_axis)
void GpGadgets::LoadTicSeries(GpCommand & rC, GpAxis * pAx)
{
	double incr, end;
	int    incr_token;
	t_ticdef * tdef = &(pAx->ticdef);
	double start = GetNumOrTime(rC, pAx);
	if(!rC.Eq(",")) {
		// only step specified 
		incr = start;
		start = -GPVL;
		end = GPVL;
	}
	else {
		rC.CToken++;
		incr_token = rC.CToken;
		incr = GetNumOrTime(rC, pAx);
		if(!rC.Eq(",")) {
			end = GPVL; // only step and increment specified
		}
		else {
			rC.CToken++;
			end = GetNumOrTime(rC, pAx);
		}
	}
	if(start < end && incr <= 0)
		GpGg.IntError(incr_token, "increment must be positive");
	if(start > end && incr >= 0)
		GpGg.IntError(incr_token, "increment must be negative");
	if(start > end) {
		// put in order
		const double numtics = floor((end * (1 + SIGNIF) - start) / incr);
		end = start;
		start = end + numtics * incr;
		incr = -incr;
	}
	if(!tdef->def.mix) { /* remove old list */
		ticmark::DestroyList(tdef->def.user);
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
GpTextLabel * new_text_label(int tag)
{
	GpTextLabel * p_new_label = (GpTextLabel *)malloc(sizeof(GpTextLabel));
	memzero(p_new_label, sizeof(GpTextLabel));
	p_new_label->tag = tag;
	p_new_label->place.Set(first_axes, first_axes, first_axes, 0., 0., 0.);
	p_new_label->pos = LEFT;
	p_new_label->textcolor.type = TC_DEFAULT;
	p_new_label->lp_properties.p_type = 1;
	p_new_label->offset.Set(character, character, character, 0., 0., 0.);
	return p_new_label;
}
/*
 * Parse the sub-options for label style and placement.
 * This is called from set_label, and from plot2d and plot3d
 * to handle options for 'plot with labels'
 * Note: ndim = 2 means we are inside a plot command,
 *       ndim = 3 means we are inside an splot command
 *       ndim = 0 in a set command
 */
//void parse_label_options(GpTextLabel * this_label, int ndim)
void GpGadgets::ParseLabelOptions(GpCommand & rC, GpTextLabel * pLabel, int nDim)
{
	GpPosition pos;
	char * p_font_name = NULL;
	enum JUSTIFY just = LEFT;
	int rotate = 0;
	bool   set_position = false;
	bool   set_just = false;
	bool   set_point = false;
	bool   set_rot = false;
	bool   set_font = false;
	bool   set_offset = false;
	bool   set_layer = false;
	bool   set_textcolor = false;
	bool   set_hypertext = false;
	int    layer = LAYER_BACK;
	bool   axis_label = (pLabel->tag == -2);
	bool   hypertext = false;
	GpPosition offset;
	offset.Set(character, character, character, 0., 0., 0.);
	t_colorspec textcolor(TC_DEFAULT, 0, 0.0);
	lp_style_type loc_lp; // = DEFAULT_LP_STYLE_TYPE;
	loc_lp.SetDefault2();
	loc_lp.flags = LP_NOT_INITIALIZED;
	/* Now parse the label format and style options */
	while(!rC.EndOfCommand()) {
		/* get position */
		if((nDim == 0) && !set_position && rC.Eq("at") && !axis_label) {
			rC.CToken++;
			GetPosition(rC, &pos);
			set_position = true;
			continue;
		}
		/* get justification */
		if(!set_just) {
			if(rC.AlmostEq("l$eft")) {
				just = LEFT;
				rC.CToken++;
				set_just = true;
				continue;
			}
			else if(rC.AlmostEq("c$entre") || rC.AlmostEq("c$enter")) {
				just = CENTRE;
				rC.CToken++;
				set_just = true;
				continue;
			}
			else if(rC.AlmostEq("r$ight")) {
				just = RIGHT;
				rC.CToken++;
				set_just = true;
				continue;
			}
		}
		/* get rotation (added by RCC) */
		if(rC.AlmostEq("rot$ate")) {
			rC.CToken++;
			set_rot = true;
			rotate = pLabel->rotate;
			if(rC.Eq("by")) {
				rC.CToken++;
				rotate = rC.IntExpression();
			}
			else if(rC.AlmostEq("para$llel")) {
				if(pLabel->tag >= 0)
					IntErrorCurToken("invalid option");
				rC.CToken++;
				pLabel->tag = ROTATE_IN_3D_LABEL_TAG;
			}
			else
				rotate = TEXT_VERTICAL;
			continue;
		}
		else if(rC.AlmostEq("norot$ate")) {
			rotate = 0;
			rC.CToken++;
			set_rot = true;
			if(pLabel->tag == ROTATE_IN_3D_LABEL_TAG)
				pLabel->tag = NONROTATABLE_LABEL_TAG;
			continue;
		}
		/* get p_font_name (added by DJL) */
		if(!set_font && rC.Eq("font")) {
			rC.CToken++;
			if((p_font_name = rC.TryToGetString())) {
				set_font = true;
				continue;
			}
			else
				IntErrorCurToken("'fontname,fontsize' expected");
		}
		/* Flag this as hypertext rather than a normal label */
		if(!set_hypertext && rC.AlmostEq("hyper$text")) {
			rC.CToken++;
			hypertext = true;
			set_hypertext = true;
			if(!set_point) {
				//loc_lp = default_hypertext_point_style;
				/*
				static lp_style_type default_hypertext_point_style
					= {1, LT_BLACK, 4, DASHTYPE_SOLID, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, {TC_RGB, 0x000000, 0.0}, DEFAULT_DASHPATTERN};
				*/
				// #define DEFAULT_LP_STYLE_TYPE {0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, DEFAULT_COLORSPEC, DEFAULT_DASHPATTERN}
				loc_lp.SetDefault2();
				loc_lp.flags = 1;
				loc_lp.p_type = 4;
				loc_lp.pm3d_color.Set(TC_RGB, 0, 0.0);
			}
			continue;
		}
		else if(!set_hypertext && rC.AlmostEq("nohyper$text")) {
			rC.CToken++;
			hypertext = false;
			set_hypertext = true;
			continue;
		}
		/* get front/back (added by JDP) */
		if((nDim == 0) && !set_layer && !axis_label) {
			if(rC.Eq("back")) {
				layer = LAYER_BACK;
				rC.CToken++;
				set_layer = true;
				continue;
			}
			else if(rC.Eq("front")) {
				layer = LAYER_FRONT;
				rC.CToken++;
				set_layer = true;
				continue;
			}
		}
#ifdef EAM_BOXED_TEXT
		if(rC.Eq("boxed")) {
			pLabel->boxed = 1;
			rC.CToken++;
			continue;
		}
		else if(rC.Eq("noboxed")) {
			pLabel->boxed = 0;
			rC.CToken++;
			continue;
		}
#endif
		if(!axis_label && (loc_lp.flags == LP_NOT_INITIALIZED || set_hypertext)) {
			if(rC.AlmostEq("po$int")) {
				int stored_token = ++rC.CToken;
				lp_style_type tmp_lp;
				loc_lp.flags = LP_SHOW_POINTS;
				tmp_lp = loc_lp;
				LpParse(rC, tmp_lp, LP_ADHOC, true);
				if(stored_token != rC.CToken)
					loc_lp = tmp_lp;
				set_point = true;
				continue;
			}
			else if(rC.AlmostEq("nopo$int")) {
				loc_lp.flags = 0;
				rC.CToken++;
				continue;
			}
		}
		if(!set_offset && rC.AlmostEq("of$fset")) {
			rC.CToken++;
			GetPositionDefault(rC, &offset, character, nDim);
			set_offset = true;
			continue;
		}
		if((rC.Eq("tc") || rC.AlmostEq("text$color")) && !set_textcolor) {
			rC.ParseColorSpec(&textcolor, TC_VARIABLE);
			set_textcolor = true;
			continue;
		}
		if(rC.AlmostEq("noenh$anced")) {
			pLabel->noenhanced = true;
			rC.CToken++;
			continue;
		}
		else if(rC.AlmostEq("enh$anced")) {
			pLabel->noenhanced = false;
			rC.CToken++;
			continue;
		}
		/* Coming here means that none of the previous 'if's struck
		 * its "continue" statement, i.e.  whatever is in the command
		 * line is forbidden by the 'set label' command syntax.
		 * On the other hand, 'plot with labels' may have additional stuff coming up.
		 */
		break;
	} /* while(!rC.EndOfCommand()) */

	/* HBB 20011120: this chunk moved here, behind the while()
	 * loop. Only after all options have been parsed it's safe to
	 * overwrite the position if none has been specified. */
	if(!set_position)
		pos.Set(first_axes, first_axes, first_axes, 0., 0., 0.);

	/* OK! copy the requested options into the label */
	if(set_position)
		pLabel->place = pos;
	if(set_just)
		pLabel->pos = just;
	if(set_rot)
		pLabel->rotate = rotate;
	if(set_layer)
		pLabel->layer = layer;
	if(set_font)
		pLabel->font = p_font_name;
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
}

/* <histogramstyle> = {clustered {gap <n>} | rowstacked | columnstacked */
/*                     errorbars {gap <n>} {linewidth <lw>}}            */
/*                    {title <title_options>}                           */
static void parse_histogramstyle(GpCommand & rC, histogram_style * hs, t_histogram_type def_type, int def_gap)
{
	GpTextLabel title_specs; // = EMPTY_LABELSTRUCT;
	title_specs.SetEmpty();
	// Set defaults
	hs->type  = def_type;
	hs->gap   = def_gap;
	if(!rC.EndOfCommand()) {
		if(rC.Eq("hs") || rC.AlmostEq("hist$ogram")) {
			rC.CToken++;
			while(!rC.EndOfCommand()) {
				if(rC.AlmostEq("clust$ered")) {
					hs->type = HT_CLUSTERED;
					rC.CToken++;
				}
				else if(rC.AlmostEq("error$bars")) {
					hs->type = HT_ERRORBARS;
					rC.CToken++;
				}
				else if(rC.AlmostEq("rows$tacked")) {
					hs->type = HT_STACKED_IN_LAYERS;
					rC.CToken++;
				}
				else if(rC.AlmostEq("columns$tacked")) {
					hs->type = HT_STACKED_IN_TOWERS;
					rC.CToken++;
				}
				else if(rC.Eq("gap")) {
					if(rC.IsANumber(++rC.CToken))
						hs->gap = rC.IntExpression();
					else
						GpGg.IntErrorCurToken("expected gap value");
				}
				else if(rC.AlmostEq("ti$tle")) {
					title_specs.offset = hs->title.offset;
					GpGg.SetXYZLabel(rC, &title_specs);
					ZFREE(title_specs.text);
					ZFREE(hs->title.font);
					hs->title = title_specs;
				}
				else if((rC.Eq("lw") || rC.AlmostEq("linew$idth")) && (hs->type == HT_ERRORBARS)) {
					rC.CToken++;
					hs->bar_lw = rC.RealExpression();
					if(hs->bar_lw <= 0)
						hs->bar_lw = 1;
				}
				else
					break; // We hit something unexpected
			}
		}
	}
}
//
// set pm3d lighting {primary <fraction>} {specular <fraction>}
//
//static void parse_lighting_options(GpCommand & rC)
void GpGadgets::ParseLightingOptions(GpCommand & rC)
{
	rC.CToken++;
	// TODO: Add separate "set" commands for these 
	Pm3DShade.ambient = 1.0;
	Pm3DShade.Phong = 5.0; /* Phong exponent */
	Pm3DShade.rot_x = 45;  /* illumination angle */
	Pm3DShade.rot_z = -45; /* illumination angle */
	Pm3DShade.fixed = true; /* true means the light does not rotate */
	// This is what you get from simply "set pm3d lighting"
	Pm3DShade.strength = 0.5; /* contribution of primary light source */
	Pm3DShade.spec = 0.2;  /* contribution of specular highlights */
	while(!rC.EndOfCommand()) {
		if(rC.AlmostEq("primary")) {
			rC.CToken++;
			Pm3DShade.strength = rC.RealExpression();
			Pm3DShade.strength = clip_to_01(Pm3DShade.strength);
		}
		else if(rC.AlmostEq("spec$ular")) {
			rC.CToken++;
			Pm3DShade.spec = rC.RealExpression();
			Pm3DShade.spec = clip_to_01(Pm3DShade.spec);
		}
		else
			break;
	}
	rC.CToken--;
}
//
// process 'set style parallelaxis' command 
//
static void set_style_parallel(GpCommand & rC)
{
	rC.CToken++;
	while(!rC.EndOfCommand()) {
		const int save_token = rC.CToken;
		GpGg.LpParse(rC, parallel_axis_style.lp_properties,  LP_ADHOC, false);
		if(save_token == rC.CToken) {
			if(rC.Eq("front"))
				parallel_axis_style.layer = LAYER_FRONT;
			else if(rC.Eq("back"))
				parallel_axis_style.layer = LAYER_BACK;
			else
				GpGg.IntErrorCurToken("unrecognized option");
			rC.CToken++;
		}
	}
}

