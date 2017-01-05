/* GNUPLOT - show.c */

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
#include "version.h"
#ifdef USE_MOUSE
	//#include "mouse.h"
#endif
#ifdef WIN32
	#include "win/winmain.h"
#endif
#ifdef HAVE_LIBCACA
	#include <caca.h>
#endif

/******** Local functions ********/

static void disp_at(AtType *, int);
//static void show_clip();
static void show_contour();
static void show_styles(const char * name, enum PLOT_STYLE style);
#ifdef EAM_OBJECTS
static void show_style_circle();
static void show_style_ellipse();
#endif
//static void show_raxis();
//static void show_paxis();
//static void show_zeroaxis(AXIS_INDEX);
//static void show_label(int tag);
static void show_keytitle();
static void show_key();
static void show_logscale();
static void show_offsets();
static void show_output();
static void show_parametric();
//static void show_pm3d();
static void show_palette_palette(GpCommand & rC);
static void show_palette_colornames(GpCommand & rC);
static void show_pointsize();
static void show_pointintervalbox();
static void show_encoding();
static void show_decimalsign();
//static void show_fit();
static void show_polar();
static void show_psdir();
//static void show_angles();
//static void show_samples();
//static void show_isosamples();
static void show_hidden3d();
static void show_increment();
static void show_histogram();
#ifdef EAM_BOXED_TEXT
//static void show_textbox();
#endif
static void show_size();
static void show_origin();
static void show_term();
static void show_mtics(AXIS_INDEX);
static void show_range(AXIS_INDEX axis);
static void show_nonlinear();
static void show_xyzlabel(const char * name, const char * suffix, GpTextLabel * label);
static void show_title();
static void show_axislabel(AXIS_INDEX);
static void show_data_is_timedate(AXIS_INDEX);
static void show_timefmt();
static void show_locale();
static void show_loadpath();
static void show_fontpath();
static void show_zero();
static void show_datafile(GpCommand & rC);
#ifdef USE_MOUSE
	//static void show_mouse();
#endif
static void show_plot();
//static void show_variables();
static void show_linestyle(int tag);
static void show_linetype(linestyle_def * listhead, int tag);
static void show_arrowstyle(int tag);
//static void show_arrow(int tag);
void show_position(GpPosition * pos, int ndim);
static void show_functions();

static int var_show_all = 0;

/* following code segments appear over and over again */
#define SHOW_ALL_NL { if(!var_show_all) putc('\n', stderr); }

#define PROGRAM "G N U P L O T"
//
// The 'show' command
//
void GpGadgets::ShowCommand(GpCommand & rC)
{
	int tag = 0;
	char * error_message = NULL;
	rC.CToken++;
	enum set_id token_found = (enum set_id)rC.LookupTable(&set_tbl[0], rC.CToken);
	/* rationalize rC.CToken advancement stuff a bit: */
	if(token_found != S_INVALID)
		rC.CToken++;

	switch(token_found) {
		case S_ACTIONTABLE:
		    //show_at();
			//
			// process 'show actiontable|at' command not documented
			//
			//static void show_at()
			{
				putc('\n', stderr);
				disp_at(rC.P.TempAt(), 0);
				rC.CToken++;
			}
		    break;
		case S_ALL:
		    ShowAll(rC);
		    break;
		case S_VERSION:
		    show_version(rC, stderr);
		    break;
		case S_AUTOSCALE:
		    ShowAutoscale();
		    break;
		case S_BARS:
		    SaveBars(stderr);
		    break;
		case S_BIND:
		    while(!rC.EndOfCommand()) 
				rC.CToken++;
		    rC.CToken--;
		    BindCommand(rC);
		    break;
		case S_BORDER:
		    ShowBorder();
		    break;
		case S_BOXWIDTH:
		    ShowBoxWidth();
		    break;
		case S_CLIP:
		    ShowClip();
		    break;
		case S_CLABEL:
		/* contour labels are shown with 'show contour' */
		case S_CONTOUR:
		case S_CNTRPARAM:
		case S_CNTRLABEL:
		    show_contour();
		    break;
		case S_DGRID3D:
		    ShowDGrid3D();
		    break;
		case S_MACROS:
		    /* Aug 2013: macros are always enabled */
		    break;
		case S_MAPPING:
		    ShowMapping();
		    break;
		case S_DUMMY:
		    rC.ShowDummy();
		    break;
		case S_FORMAT:
		    ShowFormat();
		    break;
		case S_FUNCTIONS:
		    show_functions();
		    break;
		case S_GRID:
		    ShowGrid();
		    break;
		case S_RAXIS:
		    //show_raxis();
			fprintf(stderr, "\traxis is %sdrawn\n", raxis ? "" : "not ");
		    break;
		case S_PAXIS:
		    //show_paxis();
			//static void show_paxis()
			{
				int    p = rC.IntExpression();
				if(p <= 0 || p > (int)NumParallelAxes)
					IntError(rC, rC.CToken, "no such parallel axis is active");
				fputs("\n\t", stderr);
				if(rC.Eq("range"))
					save_prange(stderr, &P_ParallelAxis[p-1]);
				else if(rC.AlmostEq("tic$s"))
					ShowTicDefP(P_ParallelAxis[p-1]);
				rC.CToken++;
			}
		    break;
		case S_ZEROAXIS:
		    ShowZeroAxis(FIRST_X_AXIS);
		    ShowZeroAxis(FIRST_Y_AXIS);
		    ShowZeroAxis(FIRST_Z_AXIS);
		    break;
		case S_XZEROAXIS:
		    ShowZeroAxis(FIRST_X_AXIS);
		    break;
		case S_YZEROAXIS:
		    ShowZeroAxis(FIRST_Y_AXIS);
		    break;
		case S_X2ZEROAXIS:
		    ShowZeroAxis(SECOND_X_AXIS);
		    break;
		case S_Y2ZEROAXIS:
		    ShowZeroAxis(SECOND_Y_AXIS);
		    break;
		case S_ZZEROAXIS:
		    ShowZeroAxis(FIRST_Z_AXIS);
		    break;

#define CHECK_TAG_GT_ZERO if(!rC.EndOfCommand()) { \
		tag = rC.IntExpression();				    \
		if(tag <= 0) {					   \
			error_message =  "tag must be > zero";		\
			break;						\
		}						\
	}							\
	putc('\n', stderr);

		case S_LABEL:
		    CHECK_TAG_GT_ZERO;
		    ShowLabel(rC, tag);
		    break;
		case S_ARROW:
		    CHECK_TAG_GT_ZERO;
		    ShowArrow(rC, tag);
		    break;
		case S_LINESTYLE:
		    CHECK_TAG_GT_ZERO;
		    show_linestyle(tag);
		    break;
		case S_LINETYPE:
		    CHECK_TAG_GT_ZERO;
		    show_linetype(first_perm_linestyle, tag);
		    break;
		case S_MONOCHROME:
		    fprintf(stderr, "monochrome mode is %s\n", IsMonochrome ? "active" : "not active");
		    if(rC.Eq("lt") || rC.AlmostEq("linet$ype")) {
			    rC.CToken++;
			    CHECK_TAG_GT_ZERO;
		    }
		    show_linetype(first_mono_linestyle, tag);
		    break;
		case S_DASHTYPE:
		    CHECK_TAG_GT_ZERO;
		    //show_dashtype(tag);
			//static void show_dashtype(int tag)
			{
				bool showed = false;
				for(custom_dashtype_def * p_dashtype = first_custom_dashtype; p_dashtype != NULL; p_dashtype = p_dashtype->next) {
					if(tag == 0 || tag == p_dashtype->tag) {
						showed = true;
						fprintf(stderr, "\tdashtype %d, ", p_dashtype->tag);
						save_dashtype(stderr, p_dashtype->d_type, &(p_dashtype->dashtype));
						fputc('\n', stderr);
					}
				}
				if(tag > 0 && !showed)
					IntError(rC, rC.CToken, "dashtype not found");
			}
		    break;
		case S_LINK:
		    //show_link();
			{
				if(rC.EndOfCommand() || rC.AlmostEq("x$2"))
					save_link(stderr, &AxA[SECOND_X_AXIS]);
				if(rC.EndOfCommand() || rC.AlmostEq("y$2"))
					save_link(stderr, &AxA[SECOND_Y_AXIS]);
				if(!rC.EndOfCommand())
					rC.CToken++;
			}
		    break;
		case S_NONLINEAR:
		    show_nonlinear();
		    break;
		case S_KEY:
		    show_key();
		    break;
		case S_LOGSCALE:
		    show_logscale();
		    break;
		case S_OFFSETS:
		    show_offsets();
		    break;

		case S_LMARGIN: /* HBB 20010525: handle like 'show margin' */
		case S_RMARGIN:
		case S_TMARGIN:
		case S_BMARGIN:
		case S_MARGIN:
		    ShowMargin();
		    break;
		case SET_OUTPUT:
		    show_output();
		    break;
		case S_PARAMETRIC:
		    show_parametric();
		    break;
		case S_PM3D:
		    ShowPm3D(rC);
		    break;
		case S_PALETTE:
		    ShowPalette(rC);
		    break;
		case S_COLORBOX:
		    ShowColorBox(rC);
		    break;
		case S_COLORNAMES:
		case S_COLORSEQUENCE:
		    rC.CToken--;
		    show_palette_colornames(rC);
		    break;
		case S_POINTINTERVALBOX:
		    show_pointintervalbox();
		    break;
		case S_POINTSIZE:
		    show_pointsize();
		    break;
		case S_DECIMALSIGN:
		    show_decimalsign();
		    break;
		case S_ENCODING:
		    show_encoding();
		    break;
		case S_FIT:
		    ShowFit();
		    break;
		case S_FONTPATH:
		    show_fontpath();
		    break;
		case S_POLAR:
		    show_polar();
		    break;
		case S_PRINT:
		    rC.ShowPrint();
		    break;
		case S_PSDIR:
		    show_psdir();
		    break;
		case S_OBJECT:
#ifdef EAM_OBJECTS
		    if(rC.AlmostEq("rect$angle"))
			    rC.CToken++;
		    CHECK_TAG_GT_ZERO;
		    save_object(stderr, tag);
#endif
		    break;
		case S_ANGLES:
		    ShowAngles();
		    break;
		case S_SAMPLES:
		    ShowSamples();
		    break;
		case S_ISOSAMPLES:
		    ShowIsoSamples();
		    break;
		case S_JITTER:
		    show_jitter();
		    break;
		case S_VIEW:
		    ShowView();
		    break;
		case S_DATA:
		    error_message = "keyword 'data' deprecated, use 'show style data'";
		    break;
		case S_STYLE:
		    ShowStyle(rC);
		    break;
		case S_SURFACE:
		    ShowSurface();
		    break;
		case S_HIDDEN3D:
		    show_hidden3d();
		    break;
		case S_HISTORYSIZE:
		case S_HISTORY:
		    rC.ShowHistory();
		    break;
		case S_SIZE:
		    show_size();
		    break;
		case S_ORIGIN:
		    show_origin();
		    break;
		case S_TERMINAL:
		    show_term();
		    break;
		case S_TICS:
		case S_TICSLEVEL:
		case S_TICSCALE:
		case S_XYPLANE:
		    ShowTics(true, true, true, true, true, true);
		    break;
		case S_MXTICS:
		    show_mtics(FIRST_X_AXIS);
		    break;
		case S_MYTICS:
		    show_mtics(FIRST_Y_AXIS);
		    break;
		case S_MZTICS:
		    show_mtics(FIRST_Z_AXIS);
		    break;
		case S_MCBTICS:
		    show_mtics(COLOR_AXIS);
		    break;
		case S_MX2TICS:
		    show_mtics(SECOND_X_AXIS);
		    break;
		case S_MY2TICS:
		    show_mtics(SECOND_Y_AXIS);
		    break;
		case S_MRTICS:
		    show_mtics(POLAR_AXIS);
		    break;
		case S_TIMESTAMP:
		    ShowTimestamp();
		    break;
		case S_RRANGE:
		    show_range(POLAR_AXIS);
		    break;
		case S_TRANGE:
		    show_range(T_AXIS);
		    break;
		case S_URANGE:
		    show_range(U_AXIS);
		    break;
		case S_VRANGE:
		    show_range(V_AXIS);
		    break;
		case S_XRANGE:
		    show_range(FIRST_X_AXIS);
		    break;
		case S_YRANGE:
		    show_range(FIRST_Y_AXIS);
		    break;
		case S_X2RANGE:
		    show_range(SECOND_X_AXIS);
		    break;
		case S_Y2RANGE:
		    show_range(SECOND_Y_AXIS);
		    break;
		case S_ZRANGE:
		    show_range(FIRST_Z_AXIS);
		    break;
		case S_CBRANGE:
		    show_range(COLOR_AXIS);
		    break;
		case S_TITLE:
		    show_title();
		    break;
		case S_XLABEL:
		    show_axislabel(FIRST_X_AXIS);
		    break;
		case S_YLABEL:
		    show_axislabel(FIRST_Y_AXIS);
		    break;
		case S_ZLABEL:
		    show_axislabel(FIRST_Z_AXIS);
		    break;
		case S_CBLABEL:
		    show_axislabel(COLOR_AXIS);
		    break;
		case S_X2LABEL:
		    show_axislabel(SECOND_X_AXIS);
		    break;
		case S_Y2LABEL:
		    show_axislabel(SECOND_Y_AXIS);
		    break;
		case S_XDATA:
		    show_data_is_timedate(FIRST_X_AXIS);
		    break;
		case S_YDATA:
		    show_data_is_timedate(FIRST_Y_AXIS);
		    break;
		case S_X2DATA:
		    show_data_is_timedate(SECOND_X_AXIS);
		    break;
		case S_Y2DATA:
		    show_data_is_timedate(SECOND_Y_AXIS);
		    break;
		case S_ZDATA:
		    show_data_is_timedate(FIRST_Z_AXIS);
		    break;
		case S_CBDATA:
		    show_data_is_timedate(COLOR_AXIS);
		    break;
		case S_TIMEFMT:
		    show_timefmt();
		    break;
		case S_LOCALE:
		    show_locale();
		    break;
		case S_LOADPATH:
		    show_loadpath();
		    break;
		case S_ZERO:
		    show_zero();
		    break;
		case S_DATAFILE:
		    show_datafile(rC);
		    break;
#ifdef USE_MOUSE
		case S_MOUSE:
		    ShowMouse();
		    break;
#endif
		case S_PLOT:
		    show_plot();
#if defined(READLINE) || defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
		    if(!rC.EndOfCommand()) {
			    if(rC.AlmostEq("a$dd2history")) {
				    rC.CToken++;
				    rC.H.AddHistory(rC.P_ReplotLine);
			    }
		    }
#endif
		    break;
		case S_VARIABLES:
		    GpGg.Ev.ShowVariables(rC);
		    break;
/* FIXME: get rid of S_*DTICS, S_*MTICS cases */
		case S_XTICS:
		case S_XDTICS:
		case S_XMTICS:
		    ShowTics(true, false, false, true, false, false);
		    break;
		case S_YTICS:
		case S_YDTICS:
		case S_YMTICS:
		    ShowTics(false, true, false, false, true, false);
		    break;
		case S_ZTICS:
		case S_ZDTICS:
		case S_ZMTICS:
		    ShowTics(false, false, true, false, false, false);
		    break;
		case S_CBTICS:
		case S_CBDTICS:
		case S_CBMTICS:
		    ShowTics(false, false, false, false, false, true);
		    break;
		case S_RTICS:
		    ShowTicDef(POLAR_AXIS);
		    break;
		case S_X2TICS:
		case S_X2DTICS:
		case S_X2MTICS:
		    ShowTics(false, false, false, true, false, false);
		    break;
		case S_Y2TICS:
		case S_Y2DTICS:
		case S_Y2MTICS:
		    ShowTics(false, false, false, false, true, false);
		    break;
		case S_MULTIPLOT:
		    fprintf(stderr, "multiplot mode is %s\n", IsMultiPlot ? "on" : "off");
		    break;
		case S_TERMOPTIONS:
		    fprintf(stderr, "Terminal options are '%s'\n", (*term_options) ? term_options : "[none]");
		    break;
		/* HBB 20010525: 'set commands' that don't have an
		 * accompanying 'show' version, for no particular reason: */
		/* --- such case now, all implemented. */

		case S_INVALID:
		    error_message = "Unrecognized option. See 'help show'.";
		    break;
		default:
		    error_message = "invalid or deprecated syntax";
		    break;
	}
	if(error_message)
		IntError(rC, rC.CToken, error_message);
	screen_ok = false;
	putc('\n', stderr);
#undef CHECK_TAG_GT_ZERO
}
//
// called by show_at(), and recursively by itself 
//
static void disp_at(AtType * curr_at, int level)
{
	GpArgument * arg;
	for(int i = 0; i < curr_at->a_count; i++) {
		putc('\t', stderr);
		for(int  j = 0; j < level; j++)
			putc(' ', stderr); // indent
		// print name of instruction
		fputs(GpGg.Ev.ft[(int)(curr_at->actions[i].Index)].f_name, stderr);
		arg = &(curr_at->actions[i].arg);
		// now print optional argument
		switch(curr_at->actions[i].Index) {
			case PUSH:
			    fprintf(stderr, " %s\n", arg->udv_arg->udv_name);
			    break;
			case PUSHC:
			    putc(' ', stderr);
			    disp_value(stderr, &(arg->v_arg), true);
			    putc('\n', stderr);
			    break;
			case PUSHD1:
			    fprintf(stderr, " %c dummy\n",
			    arg->udf_arg->udf_name[0]);
			    break;
			case PUSHD2:
			    fprintf(stderr, " %c dummy\n",
			    arg->udf_arg->udf_name[1]);
			    break;
			case CALL:
			    fprintf(stderr, " %s", arg->udf_arg->udf_name);
			    if(level < 6) {
				    if(arg->udf_arg->at) {
					    putc('\n', stderr);
					    disp_at(arg->udf_arg->at, level + 2); // @recursion
				    }
				    else
					    fputs(" (undefined)\n", stderr);
			    }
			    else
				    putc('\n', stderr);
			    break;
			case CALLN:
			case SUM:
			    fprintf(stderr, " %s", arg->udf_arg->udf_name);
			    if(level < 6) {
				    if(arg->udf_arg->at) {
					    putc('\n', stderr);
					    disp_at(arg->udf_arg->at, level + 2); // @recursion
				    }
				    else
					    fputs(" (undefined)\n", stderr);
			    }
			    else
				    putc('\n', stderr);
			    break;
			case JUMP:
			case JUMPZ:
			case JUMPNZ:
			case JTERN:
			    fprintf(stderr, " +%d\n", arg->j_arg);
			    break;
			case DOLLARS:
			    fprintf(stderr, " %d\n", arg->v_arg.v.int_val);
			    break;
			default:
			    putc('\n', stderr);
		}
	}
}
//
// process 'show all' command 
//
//static void show_all()
void GpGadgets::ShowAll(GpCommand & rC)
{
	var_show_all = 1;
	show_version(rC, stderr);
	ShowAutoscale();
	SaveBars(stderr);
	ShowBorder();
	ShowBoxWidth();
	ShowClip();
	show_contour();
	ShowDGrid3D();
	ShowMapping();
	rC.ShowDummy();
	ShowFormat();
	ShowStyle(rC);
	ShowGrid();
	//show_raxis();
	fprintf(stderr, "\traxis is %sdrawn\n", raxis ? "" : "not ");
	ShowZeroAxis(FIRST_X_AXIS);
	ShowZeroAxis(FIRST_Y_AXIS);
	ShowZeroAxis(FIRST_Z_AXIS);
	ShowLabel(rC, 0);
	ShowArrow(rC, 0);
	show_key();
	show_logscale();
	show_offsets();
	ShowMargin();
	show_output();
	rC.ShowPrint();
	show_parametric();
	ShowPalette(rC);
	ShowColorBox(rC);
	ShowPm3D(rC);
	show_pointsize();
	show_pointintervalbox();
	show_encoding();
	show_decimalsign();
	ShowFit();
	show_polar();
	ShowAngles();
#ifdef EAM_OBJECTS
	save_object(stderr, 0);
#endif
	ShowSamples();
	ShowIsoSamples();
	ShowView();
	ShowSurface();
	show_hidden3d();
	rC.ShowHistory();
	show_size();
	show_origin();
	show_term();
	ShowTics(true, true, true, true, true, true);
	show_mtics(FIRST_X_AXIS);
	show_mtics(FIRST_Y_AXIS);
	show_mtics(FIRST_Z_AXIS);
	show_mtics(SECOND_X_AXIS);
	show_mtics(SECOND_Y_AXIS);
	show_xyzlabel("", "time", &timelabel);
	if(IsParametric || IsPolar) {
		if(!Is3DPlot)
			show_range(T_AXIS);
		else {
			show_range(U_AXIS);
			show_range(V_AXIS);
		}
	}
	show_range(FIRST_X_AXIS);
	show_range(FIRST_Y_AXIS);
	show_range(SECOND_X_AXIS);
	show_range(SECOND_Y_AXIS);
	show_range(FIRST_Z_AXIS);
	show_jitter();
	show_title();
	show_axislabel(FIRST_X_AXIS);
	show_axislabel(FIRST_Y_AXIS);
	show_axislabel(FIRST_Z_AXIS);
	show_axislabel(SECOND_X_AXIS);
	show_axislabel(SECOND_Y_AXIS);
	show_data_is_timedate(FIRST_X_AXIS);
	show_data_is_timedate(FIRST_Y_AXIS);
	show_data_is_timedate(SECOND_X_AXIS);
	show_data_is_timedate(SECOND_Y_AXIS);
	show_data_is_timedate(FIRST_Z_AXIS);
	show_timefmt();
	show_loadpath();
	show_fontpath();
	show_psdir();
	show_locale();
	show_zero();
	show_datafile(rC);
#ifdef USE_MOUSE
	ShowMouse();
#endif
	show_plot();
	Ev.ShowVariables(rC);
	show_functions();

	var_show_all = 0;
}
//
// process 'show version' command 
//
void show_version(GpCommand & rC, FILE *fp)
{
    // If printed to a file, we prefix everything with
    // a hash mark to comment out the version information.
    char prefix[6];		/* "#    " */
    char *p = prefix;
    char fmt[2048];
    prefix[0] = '#';
    prefix[1] = prefix[2] = prefix[3] = prefix[4] = ' ';
    prefix[5] = NUL;
    // Construct string of configuration options used to build 
    // this particular copy of gnuplot. Executed once only.    
    if(!compile_options) {
		compile_options = (char *)malloc(1024);
	{
	    // The following code could be a lot simpler if
		// it wasn't for Borland's broken compiler ...
	    const char * rdline =
#ifdef READLINE
		"+"
#else
		"-"
#endif
		"READLINE  ";
	    const char *gnu_rdline =
#if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
		"+"
#else
		"-"
#endif
#ifdef HAVE_LIBEDITLINE
		"LIBEDITLINE  "
#else
		"LIBREADLINE  "
#endif
#if defined(HAVE_LIBREADLINE) && defined(MISSING_RL_TILDE_EXPANSION)
		"+READLINE_IS_REALLY_EDITLINE  "
#endif
#ifdef GNUPLOT_HISTORY
		"+"
#else
		"-"
#endif
		"HISTORY  ";
	    const char *libcerf =
#ifdef HAVE_LIBCERF
		"+LIBCERF  ";
#else
		"";
#endif
	    const char *libgd =
#ifdef HAVE_LIBGD
# ifdef HAVE_GD_PNG
		"+GD_PNG  "
# endif
# ifdef HAVE_GD_JPEG
		"+GD_JPEG  "
# endif
# ifdef HAVE_GD_TTF
		"+GD_TTF  "
# endif
# ifdef HAVE_GD_GIF
		"+GD_GIF  "
# endif
# ifdef GIF_ANIMATION
		"+ANIMATION  "
# endif
#else
		"-LIBGD  "
#endif
		"";
	    const char *linuxvga =
#ifdef LINUXVGA
		"+LINUXVGA  "
#endif
		"";
	    const char *compatibility =
#ifdef BACKWARDS_COMPATIBLE
		"+BACKWARDS_COMPATIBILITY  "
#else
		"-BACKWARDS_COMPATIBILITY  "
#endif
		"";
	    const char *binary_files = "+BINARY_DATA  ";
	    const char *nocwdrc =
#ifdef USE_CWDRC
		"+"
#else
		"-"
#endif
		"USE_CWDRC  ";
	    const char *x11 =
#ifdef X11
		"+X11  +X11_POLYGON  "
#ifdef USE_X11_MULTIBYTE
		"+MULTIBYTE  "
#endif
#ifdef EXTERNAL_X11_WINDOW
		"+X11_EXTERNAL "
#endif
#endif
		"";
	    const char *use_mouse =
#ifdef USE_MOUSE
		"+USE_MOUSE  "
#endif
		"";
	    const char *hiddenline =
#ifdef HIDDEN3D_QUADTREE
		"+HIDDEN3D_QUADTREE  "
#else
# ifdef HIDDEN3D_GRIDBOX
		"+HIDDEN3D_GRIDBOX  "
# endif
#endif
		"";
	    const char * plotoptions= "+DATASTRINGS  +HISTOGRAMS  "
#ifdef EAM_OBJECTS
		"+OBJECTS  "
#endif
		"+STRINGVARS  "
		"+MACROS  "
		"+THIN_SPLINES  "
		"+IMAGE  "
		"+USER_LINETYPES "
#ifdef USE_STATS
		"+STATS "
#else
		"-STATS "
#endif
#ifdef HAVE_EXTERNAL_FUNCTIONS
		"+EXTERNAL_FUNCTIONS "
#endif
	    "";
	    sprintf(compile_options, "%s%s\n%s%s\n%s%s%s\n%s%s%s%s\n%s\n",
		    rdline, gnu_rdline, compatibility, binary_files,
		    libcerf, libgd, linuxvga,
		    nocwdrc, x11, use_mouse, hiddenline,
		    plotoptions);
	}

	compile_options = (char *)gp_realloc(compile_options, strlen(compile_options)+1, "compile_options");
    }
    // The only effect of fp == NULL is to load the compile_options string 
    if(fp == NULL)
		return;
    if(fp == stderr) {
	// No hash mark - let p point to the trailing '\0' 
	p += sizeof(prefix) - 1;
    } else {
#ifdef BINDIR
# ifdef X11
	fprintf(fp, "#!%s/gnuplot -persist\n#\n", BINDIR);
#  else
	fprintf(fp, "#!%s/gnuplot\n#\n", BINDIR);
# endif				/* not X11 */
#endif /* BINDIR */
    }

    strcpy(fmt, "\
%s\n\
%s\t%s\n\
%s\tVersion %s patchlevel %s    last modified %s\n\
%s\n\
%s\t%s\n\
%s\tThomas Williams, Colin Kelley and many others\n\
%s\n\
%s\tgnuplot home:     http://www.gnuplot.info\n\
");
#ifdef DEVELOPMENT_VERSION
    strcat(fmt, "%s\tmailing list:     %s\n");
#endif
    strcat(fmt, "%s\tfaq, bugs, etc:   type \"help FAQ\"\n%s\timmediate help:   type \"help\"  (plot window: hit 'h')\n");
    fprintf(fp, fmt,
	    p,			/* empty line */
	    p, PROGRAM,
	    p, gnuplot_version, gnuplot_patchlevel, gnuplot_date,
	    p,			/* empty line */
	    p, gnuplot_copyright,
	    p,			/* authors */
	    p,			/* empty line */
	    p,			/* website */
#ifdef DEVELOPMENT_VERSION
	    p, help_email,	/* mailing list */
#endif
	    p,			/* type "help" */
	    p 			/* type "help seeking-assistance" */
	    );
    /* show version long */
    if(rC.AlmostEq("l$ong")) {
	rC.CToken++;
	fprintf(stderr, "Compile options:\n%s", compile_options);
	fprintf(stderr, "MAX_PARALLEL_AXES=%d\n\n", MAX_PARALLEL_AXES);
#ifdef X11
	{
	    char *driverdir = getenv("GNUPLOT_DRIVER_DIR");
	    if(driverdir == NULL)
		driverdir = X11_DRIVER_DIR;
	    fprintf(stderr, "GNUPLOT_DRIVER_DIR = \"%s\"\n", driverdir);
	}
#endif
	{
	    char *psdir = getenv("GNUPLOT_PS_DIR");
#ifdef GNUPLOT_PS_DIR
		SETIFZ(psdir, GNUPLOT_PS_DIR);
#endif
	    if(psdir != NULL)
			fprintf(stderr, "GNUPLOT_PS_DIR     = \"%s\"\n", psdir);
	}
	{
	    char *helpfile = NULL;
#ifndef WIN32
	    if((helpfile = getenv("GNUHELP")) == NULL)
			helpfile = HELPFILE;
#else
	    helpfile = winhelpname;
#endif
	fprintf(stderr, "HELPFILE           = \"%s\"\n", helpfile);
	}
#if defined(WIN32) && !defined(WGP_CONSOLE)
	fprintf(stderr, "MENUNAME           = \"%s\"\n", szMenuName);
#endif
#ifdef HAVE_LIBCACA
	fprintf(stderr, "libcaca version    : %s\n", caca_get_version());
#endif
    } /* show version long */
}
//
// process 'show autoscale' command 
//
//static void show_autoscale()
void GpGadgets::ShowAutoscale()
{
	SHOW_ALL_NL;

#define SHOW_AUTOSCALE(axis) {						      \
		t_autoscale ascale = AxA[axis].SetAutoScale; \
		fprintf(stderr, "\t%s: %s%s%s%s%s, ", GetAxisName(axis), (ascale & AUTOSCALE_BOTH) ? "ON" : "OFF", \
		    ((ascale & AUTOSCALE_BOTH) == AUTOSCALE_MIN) ? " (min)" : "", \
		    ((ascale & AUTOSCALE_BOTH) == AUTOSCALE_MAX) ? " (max)" : "", \
		    (ascale & AUTOSCALE_FIXMIN) ? " (fixmin)" : "",		  \
		    (ascale & AUTOSCALE_FIXMAX) ? " (fixmax)" : "");		  \
}

	fputs("\tautoscaling is ", stderr);
	if(IsParametric) {
		if(Is3DPlot) {
			SHOW_AUTOSCALE(T_AXIS);
		}
		else {
			SHOW_AUTOSCALE(U_AXIS);
			SHOW_AUTOSCALE(V_AXIS);
		}
	}
	if(IsPolar) {
		SHOW_AUTOSCALE(POLAR_AXIS)
	}
	SHOW_AUTOSCALE(FIRST_X_AXIS);
	SHOW_AUTOSCALE(FIRST_Y_AXIS);
	fputs("\n\t               ", stderr);
	SHOW_AUTOSCALE(SECOND_X_AXIS);
	SHOW_AUTOSCALE(SECOND_Y_AXIS);
	fputs("\n\t               ", stderr);
	SHOW_AUTOSCALE(FIRST_Z_AXIS);
	SHOW_AUTOSCALE(COLOR_AXIS);
#undef SHOW_AUTOSCALE
}
//
// process 'show border' command 
//
//static void show_border()
void GpGadgets::ShowBorder()
{
	SHOW_ALL_NL;
	if(!DrawBorder)
		fprintf(stderr, "\tborder is not drawn\n");
	else {
		fprintf(stderr, "\tborder %d is drawn in %s layer with\n\t ", DrawBorder,
		    BorderLayer == LAYER_BEHIND ? "behind" : BorderLayer == LAYER_BACK ? "back" : "front");
		save_linetype(stderr, &BorderLp, false);
		fputc('\n', stderr);
	}
}
//
// process 'show boxwidth' command 
//
//static void show_boxwidth()
void GpGadgets::ShowBoxWidth()
{
	SHOW_ALL_NL;
	if(boxwidth < 0.0)
		fputs("\tboxwidth is auto\n", stderr);
	else
		fprintf(stderr, "\tboxwidth is %g %s\n", boxwidth, (boxwidth_is_absolute) ? "absolute" : "relative");
}
//
// process 'show boxplot' command 
//
//static void show_boxplot() // GpGg.boxplot_opts
void boxplot_style::Show()
{
	fprintf(stderr, "\tboxplot representation is %s\n", plotstyle == FINANCEBARS ? "finance bar" : "box and whisker");
	fprintf(stderr, "\tboxplot range extends from the ");
	if(limit_type == 1)
		fprintf(stderr, "  median to include %5.2f of the points\n", limit_value);
	else
		fprintf(stderr, "  box by %5.2f of the interquartile distance\n", limit_value);
	if(outliers)
		fprintf(stderr, "\toutliers will be drawn using point type %d\n", pointtype+1);
	else
		fprintf(stderr, "\toutliers will not be drawn\n");
	fprintf(stderr, "\tseparation between boxplots is %g\n", separation);
	fprintf(stderr, "\tfactor labels %s\n",
	    (labels == BOXPLOT_FACTOR_LABELS_X)    ? "will be put on the x axis"  :
	    (labels == BOXPLOT_FACTOR_LABELS_X2)   ? "will be put on the x2 axis" :
	    (labels == BOXPLOT_FACTOR_LABELS_AUTO) ? "are automatic" : "are off");
	fprintf(stderr, "\tfactor labels will %s\n", sort_factors ? "be sorted alphabetically" : "appear in the order they were found");
}
//
// process 'show fillstyle' command 
//
//static void show_fillstyle()
void GpGadgets::ShowFillStyle()
{
	SHOW_ALL_NL;
	switch(DefaultFillStyle.fillstyle) {
		case FS_SOLID:
		case FS_TRANSPARENT_SOLID:
		    fprintf(stderr, "\tFill style uses %s solid colour with density %.3f",
		    DefaultFillStyle.fillstyle == FS_SOLID ? "" : "transparent",
		    DefaultFillStyle.filldensity/100.0);
		    break;
		case FS_PATTERN:
		case FS_TRANSPARENT_PATTERN:
		    fprintf(stderr, "\tFill style uses %s patterns starting at %d",
		    DefaultFillStyle.fillstyle == FS_PATTERN ? "" : "transparent",
		    DefaultFillStyle.fillpattern);
		    break;
		default:
		    fprintf(stderr, "\tFill style is empty");
	}
	if(DefaultFillStyle.border_color.type == TC_LT && DefaultFillStyle.border_color.lt == LT_NODRAW)
		fprintf(stderr, " with no border\n");
	else {
		fprintf(stderr, " with border ");
		save_pm3dcolor(stderr, &DefaultFillStyle.border_color);
		fprintf(stderr, "\n");
	}
}
//
// process 'show clip' command 
//
//static void show_clip()
void GpGadgets::ShowClip()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tpoint clip is %s\n", (ClipPoints) ? "ON" : "OFF");
	fprintf(stderr, "\t%s lines with one end out of range (clip one)\n", ClipLines1 ? "clipping" : "not drawing");
	fprintf(stderr, "\t%s lines with both ends out of range (clip two)\n", ClipLines2 ? "clipping" : "not drawing");
}
//
// process 'show cntrparam|cntrlabel|contour' commands
//
static void show_contour()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tcontour for surfaces are %s", (GpGg.draw_contour) ? "drawn" : "not drawn\n");
	if(GpGg.draw_contour) {
		fprintf(stderr, " in %d levels on ", contour_levels);
		switch(GpGg.draw_contour) {
			case CONTOUR_BASE:
			    fputs("grid base\n", stderr);
			    break;
			case CONTOUR_SRF:
			    fputs("surface\n", stderr);
			    break;
			case CONTOUR_BOTH:
			    fputs("grid base and surface\n", stderr);
			    break;
			case CONTOUR_NONE:
			    /* should not happen --- be easy: don't complain... */
			    break;
		}
		switch(contour_kind) {
			case CONTOUR_KIND_LINEAR:
			    fputs("\t\tas linear segments\n", stderr);
			    break;
			case CONTOUR_KIND_CUBIC_SPL:
			    fprintf(stderr, "\t\tas cubic spline interpolation segments with %d pts\n", contour_pts);
			    break;
			case CONTOUR_KIND_BSPLINE:
			    fprintf(stderr, "\t\tas bspline approximation segments of order %d with %d pts\n", contour_order, contour_pts);
			    break;
		}
		switch(contour_levels_kind) {
			case LEVELS_AUTO:
			    fprintf(stderr, "\t\tapprox. %d automatic levels\n", contour_levels);
			    break;
			case LEVELS_DISCRETE:
		    {
			    int i;
			    fprintf(stderr, "\t\t%d discrete levels at ", contour_levels);
			    fprintf(stderr, "%g", contour_levels_list[0]);
			    for(i = 1; i < contour_levels; i++)
				    fprintf(stderr, ",%g ", contour_levels_list[i]);
			    putc('\n', stderr);
			    break;
		    }
			case LEVELS_INCREMENTAL:
			    fprintf(stderr,
			    "\t\t%d incremental levels starting at %g, step %g, end %g\n",
			    contour_levels,
			    contour_levels_list[0],
			    contour_levels_list[1],
			    contour_levels_list[0] + (contour_levels - 1) * contour_levels_list[1]);
			    /* contour-levels counts both ends */
			    break;
		}
		/* Show contour label options */
		fprintf(stderr, "\tcontour lines are drawn in %s linetypes\n", GpGg.clabel_onecolor ? "the same" : "individual");
		fprintf(stderr, "\tformat for contour labels is '%s' font '%s'\n", contour_format, NZOR(GpGg.P_ClabelFont, ""));
		fprintf(stderr, "\ton-plot labels placed at segment %d with interval %d\n", GpGg.clabel_start, GpGg.clabel_interval);
	}
}
//
// process 'show dgrid3d' command 
//
//static void show_dgrid3d()
void GpGadgets::ShowDGrid3D()
{
	SHOW_ALL_NL;
	if(dgrid3d)
		if(dgrid3d_mode == DGRID3D_QNORM) {
			fprintf(stderr, "\tdata grid3d is enabled for mesh of size %dx%d, norm=%d\n",
			    dgrid3d_row_fineness, dgrid3d_col_fineness, dgrid3d_norm_value);
		}
		else if(dgrid3d_mode == DGRID3D_SPLINES) {
			fprintf(stderr, "\tdata grid3d is enabled for mesh of size %dx%d, splines\n",
			    dgrid3d_row_fineness, dgrid3d_col_fineness);
		}
		else {
			fprintf(stderr, "\tdata grid3d is enabled for mesh of size %dx%d, kernel=%s,\n\tscale factors x=%f, y=%f%s\n",
			    dgrid3d_row_fineness, dgrid3d_col_fineness, reverse_table_lookup(dgrid3d_mode_tbl, dgrid3d_mode),
			    dgrid3d_x_scale, dgrid3d_y_scale, dgrid3d_kdensity ? ", kdensity2d mode" : "");
		}
	else
		fputs("\tdata grid3d is disabled\n", stderr);
}
//
// process 'show mapping' command 
//
//static void show_mapping()
void GpGadgets::ShowMapping()
{
	SHOW_ALL_NL;
	fputs("\tmapping for 3-d data is ", stderr);
	switch(mapping3d) {
		case MAP3D_CARTESIAN: fputs("cartesian\n", stderr); break;
		case MAP3D_SPHERICAL: fputs("spherical\n", stderr); break;
		case MAP3D_CYLINDRICAL: fputs("cylindrical\n", stderr); break;
	}
}
//
// process 'show dummy' command
//
//static void show_dummy()
void GpCommand::ShowDummy()
{
	SHOW_ALL_NL;
	fputs("\tdummy variables are ", stderr);
	for(int i = 0; i< MAX_NUM_VAR; i++) {
		if(*P.SetDummyVar[i] == '\0') {
			fputs("\n", stderr);
			break;
		}
		else {
			fprintf(stderr, "%s ", P.SetDummyVar[i]);
		}
	}
}
//
// process 'show format' command 
//
//static void show_format()
void GpGadgets::ShowFormat()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\ttic format is:\n");
#define SHOW_FORMAT(_axis)						\
	fprintf(stderr, "\t  %s-axis: \"%s\"%s\n", GetAxisName(_axis), conv_text(AxA[_axis].formatstring), \
	    AxA[_axis].tictype == DT_DMS ? " geographic" : AxA[_axis].tictype == DT_TIMEDATE ? " time" : "");
	SHOW_FORMAT(FIRST_X_AXIS);
	SHOW_FORMAT(FIRST_Y_AXIS);
	SHOW_FORMAT(SECOND_X_AXIS);
	SHOW_FORMAT(SECOND_Y_AXIS);
	SHOW_FORMAT(FIRST_Z_AXIS);
	SHOW_FORMAT(COLOR_AXIS);
	SHOW_FORMAT(POLAR_AXIS);
#undef SHOW_FORMAT
}
//
// process 'show style' sommand
//
//static void show_style(GpCommand & rC)
void GpGadgets::ShowStyle(GpCommand & rC)
{
	int tag = 0;
#define CHECK_TAG_GT_ZERO if(!rC.EndOfCommand()) { tag = (int)rC.RealExpression(); if(tag <= 0) IntError(rC, rC.CToken, "tag must be > zero"); }
	switch(rC.LookupTable(&show_style_tbl[0], rC.CToken)) {
		case SHOW_STYLE_DATA:
		    SHOW_ALL_NL;
		    show_styles("Data", DataStyle);
		    rC.CToken++;
		    break;
		case SHOW_STYLE_FUNCTION:
		    SHOW_ALL_NL;
		    show_styles("Functions", FuncStyle);
		    rC.CToken++;
		    break;
		case SHOW_STYLE_LINE:
		    rC.CToken++;
		    CHECK_TAG_GT_ZERO;
		    show_linestyle(tag);
		    break;
		case SHOW_STYLE_FILLING:
		    ShowFillStyle();
		    rC.CToken++;
		    break;
		case SHOW_STYLE_INCREMENT:
		    show_increment();
		    rC.CToken++;
		    break;
		case SHOW_STYLE_HISTOGRAM:
		    show_histogram();
		    rC.CToken++;
		    break;
#ifdef EAM_BOXED_TEXT
		case SHOW_STYLE_TEXTBOX:
		    ShowTextBox();
		    rC.CToken++;
		    break;
#endif
		case SHOW_STYLE_PARALLEL:
		    save_style_parallel(stderr);
		    rC.CToken++;
		    break;
		case SHOW_STYLE_ARROW:
		    rC.CToken++;
		    CHECK_TAG_GT_ZERO;
		    show_arrowstyle(tag);
		    break;
		case SHOW_STYLE_BOXPLOT:
		    boxplot_opts.Show();
		    rC.CToken++;
		    break;
#ifdef EAM_OBJECTS
		case SHOW_STYLE_RECTANGLE:
		    ShowStyleRectangle();
		    rC.CToken++;
		    break;
		case SHOW_STYLE_CIRCLE:
		    show_style_circle();
		    rC.CToken++;
		    break;
		case SHOW_STYLE_ELLIPSE:
		    show_style_ellipse();
		    rC.CToken++;
		    break;
#endif
		default:
		    /* show all styles */
		    show_styles("Data", DataStyle);
		    show_styles("Functions", FuncStyle);
		    show_linestyle(0);
		    ShowFillStyle();
		    show_increment();
		    show_histogram();
#ifdef EAM_BOXED_TEXT
		    ShowTextBox();
#endif
		    save_style_parallel(stderr);
		    show_arrowstyle(0);
		    boxplot_opts.Show();
#ifdef EAM_OBJECTS
		    ShowStyleRectangle();
		    show_style_circle();
		    show_style_ellipse();
#endif
		    break;
	}
#undef CHECK_TAG_GT_ZERO
}

#ifdef EAM_OBJECTS
// called by show_style() - defined for aesthetic reasons 
//static void show_style_rectangle()
void GpGadgets::ShowStyleRectangle()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tRectangle style is %s, fill color ",
	    DefaultRectangle.layer > 0 ? "front" :
	    DefaultRectangle.layer < 0 ? "behind" : "back");
	/* FIXME: Broke with removal of use_palette? */
	save_pm3dcolor(stderr, &DefaultRectangle.lp_properties.pm3d_color);
	fprintf(stderr, ", lw %.1f ", DefaultRectangle.lp_properties.l_width);
	fprintf(stderr, ", fillstyle");
	save_fillstyle(stderr, &DefaultRectangle.fillstyle);
}

static void show_style_circle()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tCircle style has default radius ");
	show_position(&GpGg.DefaultCircle.o.circle.extent, 1);
	fprintf(stderr, " [%s]", GpGg.DefaultCircle.o.circle.wedge ? "wedge" : "nowedge");
	fputs("\n", stderr);
}

static void show_style_ellipse()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tEllipse style has default size ");
	show_position(&GpGg.DefaultEllipse.o.ellipse.extent, 2);
	fprintf(stderr, ", default angle is %.1f degrees", GpGg.DefaultEllipse.o.ellipse.orientation);
	switch(GpGg.DefaultEllipse.o.ellipse.type) {
		case ELLIPSEAXES_XY:
		    fputs(", diameters are in different units (major: x axis, minor: y axis)\n", stderr);
		    break;
		case ELLIPSEAXES_XX:
		    fputs(", both diameters are in the same units as the x axis\n", stderr);
		    break;
		case ELLIPSEAXES_YY:
		    fputs(", both diameters are in the same units as the y axis\n", stderr);
		    break;
	}
}

#endif

/* called by show_data() and show_func() */
static void show_styles(const char * name, enum PLOT_STYLE style)
{
	fprintf(stderr, "\t%s are plotted with ", name);
	save_data_func_style(stderr, name, style);
}

/* called by show_func() */
static void show_functions()
{
	UdftEntry * udf = GpGg.Ev.first_udf;
	fputs("\n\tUser-Defined Functions:\n", stderr);
	while(udf) {
		if(udf->definition)
			fprintf(stderr, "\t%s\n", udf->definition);
		else
			fprintf(stderr, "\t%s is undefined\n", udf->udf_name);
		udf = udf->next_udf;
	}
}
//
// process 'show grid' command 
//
//static void show_grid()
void GpGadgets::ShowGrid()
{
	SHOW_ALL_NL;
	if(!SomeGridSelected()) {
		fputs("\tgrid is OFF\n", stderr);
	}
	else {
		// HBB 20010806: new storage method for grid options:
		fprintf(stderr, "\t%s grid drawn at", (polar_grid_angle != 0) ? "Polar" : "Rectangular");
	#define SHOW_GRID(axis)						\
		if(AxA[axis].Flags & GpAxis::fGridMajor) fprintf(stderr, " %s", GetAxisName(axis));	\
		if(AxA[axis].Flags & GpAxis::fGridMinor) fprintf(stderr, " m%s", GetAxisName(axis));
		SHOW_GRID(FIRST_X_AXIS);
		SHOW_GRID(FIRST_Y_AXIS);
		SHOW_GRID(SECOND_X_AXIS);
		SHOW_GRID(SECOND_Y_AXIS);
		SHOW_GRID(FIRST_Z_AXIS);
		SHOW_GRID(COLOR_AXIS);
		SHOW_GRID(POLAR_AXIS);
	#undef SHOW_GRID
		fputs(" tics\n", stderr);
		fprintf(stderr, "\tMajor grid drawn with");
		save_linetype(stderr, &(grid_lp), false);
		fprintf(stderr, "\n\tMinor grid drawn with");
		save_linetype(stderr, &(mgrid_lp), false);
		fputc('\n', stderr);
		if(polar_grid_angle)
			fprintf(stderr, "\tGrid radii drawn every %f %s\n", polar_grid_angle / Ang2Rad, (Ang2Rad == 1.0) ? "radians" : "degrees");
		fprintf(stderr, "\tGrid drawn at %s\n", (grid_layer==-1) ? "default layer" : ((grid_layer==0) ? "back" : "front"));
	}
}

// static void show_raxis() { fprintf(stderr, "\traxis is %sdrawn\n", GpGg.raxis ? "" : "not "); }
//
// process 'show {x|y|z}zeroaxis' command 
//
//static void show_zeroaxis(AXIS_INDEX axis)
void GpGadgets::ShowZeroAxis(AXIS_INDEX axis)
{
	SHOW_ALL_NL;
	if(AxA[axis].zeroaxis) {
		fprintf(stderr, "\t%szeroaxis is drawn with", GetAxisName(axis));
		save_linetype(stderr, AxA[axis].zeroaxis, false);
		fputc('\n', stderr);
	}
	else
		fprintf(stderr, "\t%szeroaxis is OFF\n", GetAxisName(axis));
	// If this is a 'first' axis. To output secondary axis, call self recursively:
	if(AXIS_IS_FIRST(axis)) {
		ShowZeroAxis((AXIS_INDEX)AXIS_MAP_FROM_FIRST_TO_SECOND(axis));
	}
}
//
// Show label number <tag> (0 means show all) 
//
//static void show_label(int tag)
void GpGadgets::ShowLabel(GpCommand & rC, int tag)
{
	GpTextLabel * p_label;
	bool showed = false;
	for(p_label = first_label; p_label != NULL; p_label = p_label->next) {
		if(tag == 0 || tag == p_label->tag) {
			showed = true;
			fprintf(stderr, "\tlabel %d \"%s\" at ", p_label->tag, (p_label->text==NULL) ? "" : conv_text(p_label->text));
			show_position(&p_label->place, 3);
			if(p_label->hypertext)
				fprintf(stderr, " hypertext");
			switch(p_label->pos) {
				case LEFT: {
				    fputs(" left", stderr);
				    break;
			    }
				case CENTRE: {
				    fputs(" centre", stderr);
				    break;
			    }
				case RIGHT: {
				    fputs(" right", stderr);
				    break;
			    }
			}
			if(p_label->rotate)
				fprintf(stderr, " rotated by %d degrees (if possible)", p_label->rotate);
			else
				fprintf(stderr, " not rotated");
			fprintf(stderr, " %s ", p_label->layer ? "front" : "back");
			if(p_label->font != NULL)
				fprintf(stderr, " font \"%s\"", p_label->font);
			if(p_label->textcolor.type)
				save_textcolor(stderr, &p_label->textcolor);
			if(p_label->noenhanced)
				fprintf(stderr, " noenhanced");
			if((p_label->lp_properties.flags & LP_SHOW_POINTS) == 0)
				fprintf(stderr, " nopoint");
			else {
				fprintf(stderr, " point with color of");
				save_linetype(stderr, &(p_label->lp_properties), true);
				fprintf(stderr, " offset ");
				show_position(&p_label->offset, 3);
			}
#ifdef EAM_BOXED_TEXT
			if(p_label->boxed)
				fprintf(stderr, " boxed");
#endif

			/* Entry font added by DJL */
			fputc('\n', stderr);
		}
	}
	if(tag > 0 && !showed)
		IntError(rC, rC.CToken, "label not found");
}
//
// Show arrow number <tag> (0 means show all) 
//
//static void show_arrow(int tag)
void GpGadgets::ShowArrow(GpCommand & rC, int tag)
{
	bool showed = false;
	for(arrow_def * p_arrow = first_arrow; p_arrow != NULL; p_arrow = p_arrow->next) {
		if(tag == 0 || tag == p_arrow->tag) {
			showed = true;
			fprintf(stderr, "\tarrow %d, %s %s %s",
			    p_arrow->tag,
			    arrow_head_names[p_arrow->arrow_properties.head],
			    (p_arrow->arrow_properties.headfill==AS_FILLED) ? "filled" :
			    (p_arrow->arrow_properties.headfill==AS_EMPTY) ? "empty" :
			    (p_arrow->arrow_properties.headfill==AS_NOBORDER) ? "noborder" :
			    "nofilled",
			    p_arrow->arrow_properties.layer ? "front" : "back");
			save_linetype(stderr, &(p_arrow->arrow_properties.lp_properties), false);
			fprintf(stderr, "\n\t  from ");
			show_position(&p_arrow->start, 3);
			if(p_arrow->type == arrow_end_absolute) {
				fputs(" to ", stderr);
				show_position(&p_arrow->end, 3);
			}
			else if(p_arrow->type == arrow_end_relative) {
				fputs(" rto ", stderr);
				show_position(&p_arrow->end, 3);
			}
			else { // arrow_end_oriented 
				fputs(" length ", stderr);
				show_position(&p_arrow->end, 1);
				fprintf(stderr, " angle %g deg", p_arrow->angle);
			}
			if(p_arrow->arrow_properties.head_length > 0) {
				static char * msg[] = {"(first x axis) ", "(second x axis) ", "(graph units) ", "(screen units) "};
				fprintf(stderr, "\n\t  arrow head: length %s%g, angle %g deg",
				    p_arrow->arrow_properties.head_lengthunit ==
				    first_axes ? "" : msg[p_arrow->arrow_properties.head_lengthunit],
				    p_arrow->arrow_properties.head_length,
				    p_arrow->arrow_properties.head_angle);
				if(p_arrow->arrow_properties.headfill != AS_NOFILL)
					fprintf(stderr, ", backangle %g deg", p_arrow->arrow_properties.head_backangle);
			}
			putc('\n', stderr);
		}
	}
	if(tag > 0 && !showed)
		IntError(rC, rC.CToken, "arrow not found");
}
//
// process 'show keytitle' command 
//
static void show_keytitle()
{
	legend_key * key = &GpGg.keyT;
	SHOW_ALL_NL;
	fprintf(stderr, "\tkey title is \"%s\"\n", conv_text(key->title.text));
	if(key->title.font && *(key->title.font))
		fprintf(stderr, "\t  font \"%s\"\n", key->title.font);
}
//
// process 'show key' command 
//
static void show_key()
{
	legend_key * key = &GpGg.keyT;
	SHOW_ALL_NL;
	if(!(key->visible)) {
		fputs("\tkey is OFF\n", stderr);
		return;
	}
	switch(key->region) {
		case GPKEY_AUTO_INTERIOR_LRTBC:
		case GPKEY_AUTO_EXTERIOR_LRTBC:
		case GPKEY_AUTO_EXTERIOR_MARGIN: {
		    fputs("\tkey is ON, position: ", stderr);
		    if(!(key->region == GPKEY_AUTO_EXTERIOR_MARGIN && (key->margin == GPKEY_TMARGIN || key->margin == GPKEY_BMARGIN))) {
			    if(key->vpos == JUST_TOP)
				    fputs("top", stderr);
			    else if(key->vpos == JUST_BOT)
				    fputs("bottom", stderr);
			    else
				    fputs("center", stderr);
		    }
		    if(!(key->region == GPKEY_AUTO_EXTERIOR_MARGIN && (key->margin == GPKEY_LMARGIN || key->margin == GPKEY_RMARGIN))) {
			    if(key->hpos == LEFT)
				    fputs(" left", stderr);
			    else if(key->hpos == RIGHT)
				    fputs(" right", stderr);
			    else if(key->vpos != JUST_CENTRE) /* Don't print "center" twice. */
				    fputs(" center", stderr);
		    }
		    if(key->stack_dir == GPKEY_VERTICAL) {
			    fputs(" vertical", stderr);
		    }
		    else {
			    fputs(" horizontal", stderr);
		    }
		    if(key->region == GPKEY_AUTO_INTERIOR_LRTBC)
			    fputs(" inside", stderr);
		    else if(key->region == GPKEY_AUTO_EXTERIOR_LRTBC)
			    fputs(" outside", stderr);
		    else {
			    switch(key->margin) {
				    case GPKEY_TMARGIN: fputs(" TMrg", stderr); break;
				    case GPKEY_BMARGIN: fputs(" BMrg", stderr); break;
				    case GPKEY_LMARGIN: fputs(" LMrg", stderr); break;
				    case GPKEY_RMARGIN: fputs(" RMrg", stderr); break;
			    }
		    }
		    fputs("\n", stderr);
		    break;
	    }
		case GPKEY_USER_PLACEMENT:
		    fputs("\tkey is at ", stderr);
		    show_position(&key->user_pos, 2);
		    putc('\n', stderr);
		    break;
	}

	fprintf(stderr, "\
\tkey is %s justified, %sreversed, %sinverted, %senhanced and ",
	    key->just == GPKEY_LEFT ? "left" : "right",
	    key->reverse ? "" : "not ",
	    key->invert ? "" : "not ",
	    key->enhanced ? "" : "not ");
	if(key->box.l_type > LT_NODRAW) {
		fprintf(stderr, "boxed\n\twith ");
		save_linetype(stderr, &(key->box), false);
		fputc('\n', stderr);
	}
	else
		fprintf(stderr, "not boxed\n");

	if(key->front)
		fprintf(stderr, "\tkey box is opaque and drawn in front of the graph\n");

	fprintf(
	    stderr,
	    "\
\tsample length is %g characters\n\
\tvertical spacing is %g characters\n\
\twidth adjustment is %g characters\n\
\theight adjustment is %g characters\n\
\tcurves are%s automatically titled %s\n"                                                                                                                                                                         ,
	    key->swidth,
	    key->vert_factor,
	    key->width_fix,
	    key->height_fix,
	    key->auto_titles ? "" : " not",
	    key->auto_titles == FILENAME_KEYTITLES ? "with filename" :
	    key->auto_titles == COLUMNHEAD_KEYTITLES
	    ? "with column header" : "");

	fputs("\tmaximum number of columns is ", stderr);
	if(key->maxcols > 0)
		fprintf(stderr, "%d for horizontal alignment\n", key->maxcols);
	else
		fputs("calculated automatically\n", stderr);
	fputs("\tmaximum number of rows is ", stderr);
	if(key->maxrows > 0)
		fprintf(stderr, "%d for vertical alignment\n", key->maxrows);
	else
		fputs("calculated automatically\n", stderr);
	if(key->font && *(key->font))
		fprintf(stderr, "\t  font \"%s\"\n", key->font);
	if(key->textcolor.type != TC_LT || key->textcolor.lt != LT_BLACK) {
		fputs("\t ", stderr);
		save_textcolor(stderr, &(key->textcolor));
		fputs("\n", stderr);
	}

	show_keytitle();
}

void show_position(GpPosition * pos, int ndim)
{
	fprintf(stderr, "(");
	save_position(stderr, pos, ndim, false);
	fprintf(stderr, ")");
}

/* process 'show logscale' command */
static void show_logscale()
{
	int count = 0;
	SHOW_ALL_NL;
#define SHOW_LOG(axis) { \
	if(GpGg[axis].Flags & GpAxis::fLog) \
		fprintf(stderr, "%s %s (base %g)", !count++ ? "\tlogscaling" : " and", GpGg.GetAxisName(axis), GpGg[axis].base); }

	SHOW_LOG(FIRST_X_AXIS);
	SHOW_LOG(FIRST_Y_AXIS);
	SHOW_LOG(FIRST_Z_AXIS);
	SHOW_LOG(SECOND_X_AXIS);
	SHOW_LOG(SECOND_Y_AXIS);
	SHOW_LOG(COLOR_AXIS);
	SHOW_LOG(POLAR_AXIS);
#undef SHOW_LOG
	if(count == 0)
		fputs("\tno logscaling\n", stderr);
	else if(count == 1)
		fputs(" only\n", stderr);
	else
		putc('\n', stderr);
}

/* process 'show offsets' command */
static void show_offsets()
{
	SHOW_ALL_NL;
	GpGg.SaveOffsets(stderr, "\toffsets are");
}

SString & _ShowMrg(const GpPosition & rMrg, const char * pNam, SString & rBuf)
{
	rBuf = 0;
	SString temp_buf;
	temp_buf.Tab().Cat(pNam).Space();
	if(rMrg.scalex == screen) {
		//fprintf(stderr, "is set to screen %g\n", GpGg.LMrg.x);
		temp_buf.Cat("is set to screen %g");
		rBuf.Printf(temp_buf, rMrg.x);
	}
	else if(rMrg.x >= 0) {
		//fprintf(stderr, "is set to %g\n", GpGg.LMrg.x);
		temp_buf.Cat("is set to %g");
		rBuf.Printf(temp_buf, rMrg.x);
	}
	else {
		//fputs("is computed automatically\n", stderr);
		temp_buf.Cat("is computed automatically");
		rBuf = temp_buf;
	}
	rBuf.CR();
	return rBuf;
}
//
// process 'show margin' command 
//
//static void show_margin()
void GpGadgets::ShowMargin()
{
	SHOW_ALL_NL;
	SString show_buf;
	fputs(_ShowMrg(LMrg, "lmargin", show_buf), stderr);
	fputs(_ShowMrg(RMrg, "rmargin", show_buf), stderr);
	fputs(_ShowMrg(BMrg, "bmargin", show_buf), stderr);
	fputs(_ShowMrg(BMrg, "tmargin", show_buf), stderr);
}

/* process 'show output' command */
static void show_output()
{
	SHOW_ALL_NL;
	if(outstr)
		fprintf(stderr, "\toutput is sent to '%s'\n", outstr);
	else
		fputs("\toutput is sent to STDOUT\n", stderr);
}
//
// process 'show print' command 
//
//static void show_print()
void GpCommand::ShowPrint()
{
	SHOW_ALL_NL;
	if(P_PrintOutVar == NULL)
		fprintf(stderr, "\tprint output is sent to '%s'\n", PrintShowOutput());
	else
		fprintf(stderr, "\tprint output is saved to datablock %s\n", PrintShowOutput());
}

/* process 'show print' command */
static void show_psdir()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tdirectory from 'set psdir': ");
	fprintf(stderr, "%s\n", PS_psdir ? PS_psdir : "none");
	fprintf(stderr, "\tenvironment variable GNUPLOT_PS_DIR: ");
	fprintf(stderr, "%s\n", getenv("GNUPLOT_PS_DIR") ? getenv("GNUPLOT_PS_DIR") : "none");
#ifdef GNUPLOT_PS_DIR
	fprintf(stderr, "\tdefault system directory \"%s\"\n", GNUPLOT_PS_DIR);
#else
	fprintf(stderr, "\tfall through to built-in defaults\n");
#endif
}

// process 'show parametric' command 
static void show_parametric()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tparametric is %s\n", GpGg.IsParametric ? "ON" : "OFF");
}

static void show_palette_palette(GpCommand & rC)
{
	int colors, i;
	double gray;
	rgb_color rgb1;
	rgb255_color rgb255;
	int how = 0; /* How to print table: 0: default large; 1: rgb 0..1; 2: integers 0..255 */
	FILE * f;
	rC.CToken++;
	if(rC.EndOfCommand())
		GpGg.IntError(rC, rC.CToken, "palette size required");
	colors = rC.IntExpression();
	if(colors<2) colors = 128;
	if(!rC.EndOfCommand()) {
		if(rC.AlmostEq("f$loat")) /* option: print r,g,b floats 0..1 values */
			how = 1;
		else if(rC.AlmostEq("i$nt")) /* option: print only integer 0..255 values */
			how = 2;
		else
			GpGg.IntError(rC, rC.CToken, "expecting no option or int or float");
		rC.CToken++;
	}
	i = (!rC.F_PrintOut || rC.F_PrintOut == stderr || rC.F_PrintOut == stdout);
	f = (rC.F_PrintOut) ? rC.F_PrintOut : stderr;
	fprintf(stderr, "%s palette with %i discrete colors", (GpGg.SmPalette.colorMode == SMPAL_COLOR_MODE_GRAY) ? "Gray" : "Color", colors);
	if(!i)
		fprintf(stderr, " saved to \"%s\".", rC.P_PrintOutName);
	else
		fprintf(stderr, ".\n");
	for(i = 0; i < colors; i++) {
		/* colours equidistantly from [0,1]  */
		gray = (double)i / (colors - 1);
		if(GpGg.SmPalette.positive == SMPAL_NEGATIVE)
			gray = 1 - gray;
		GpGg.RGB1FromGray(gray, &rgb1);
		rgb255_from_rgb1(rgb1, &rgb255);
		switch(how) {
			case 1:
			    fprintf(f, "%0.4f\t%0.4f\t%0.4f\n", rgb1.r, rgb1.g, rgb1.b);
			    break;
			case 2:
			    fprintf(f, "%i\t%i\t%i\n", (int)rgb255.r, (int)rgb255.g, (int)rgb255.b);
			    break;
			default:
			    fprintf(f, "%3i. gray=%0.4f, (r,g,b)=(%0.4f,%0.4f,%0.4f), #%02x%02x%02x = %3i %3i %3i\n", i, gray, rgb1.r, rgb1.g, rgb1.b,
					(int)rgb255.r, (int)rgb255.g, (int)rgb255.b, (int)rgb255.r, (int)rgb255.g, (int)rgb255.b);
		}
	}
}

//static void show_palette_gradient()
void GpGadgets::ShowPaletteGradient(GpCommand & rC)
{
	double gray, r, g, b;
	++rC.CToken;
	if(SmPalette.colorMode != SMPAL_COLOR_MODE_GRADIENT) {
		fputs("\tcolor mapping *not* done by defined gradient.\n", stderr);
	}
	else {
		for(int i = 0; i < SmPalette.gradient_num; i++) {
			gray = SmPalette.gradient[i].pos;
			r = SmPalette.gradient[i].col.r;
			g = SmPalette.gradient[i].col.g;
			b = SmPalette.gradient[i].col.b;
			fprintf(stderr, "%3i. gray=%0.4f, (r,g,b)=(%0.4f,%0.4f,%0.4f), #%02x%02x%02x = %3i %3i %3i\n",
				i, gray, r, g, b, (int)(255*r+.5), (int)(255*g+.5), (int)(255*b+.5), (int)(255*r+.5), (int)(255*g+.5), (int)(255*b+.5) );
		}
	}
}

static void show_palette_colornames(GpCommand & rC)
{
	fprintf(stderr, "\tThere are %d predefined color names:", num_predefined_colors);
	//show_colornames(pm3d_color_names_tbl);
	//static void show_colornames(const GenTable * tbl)
	{
		const GenTable * p_tbl = pm3d_color_names_tbl;
		for(int i = 0; p_tbl->key;) {
			// Print color names and their rgb values, table with 1 column 
			int r = ((p_tbl->value >> 16 ) & 255);
			int g = ((p_tbl->value >> 8 ) & 255);
			int b = (p_tbl->value & 255);
			fprintf(stderr, "\n  %-18s ", p_tbl->key);
			fprintf(stderr, "#%02x%02x%02x = %3i %3i %3i", r, g, b, r, g, b);
			++p_tbl;
			++i;
		}
		fputs("\n", stderr);
		++rC.CToken;
	}
}

//static void show_palette()
void GpGadgets::ShowPalette(GpCommand & rC)
{
	// no option given, i.e. "show palette" 
	if(rC.EndOfCommand()) {
		fprintf(stderr, "\tpalette is %s\n", SmPalette.colorMode == SMPAL_COLOR_MODE_GRAY ? "GRAY" : "COLOR");
		switch(SmPalette.colorMode) {
			case SMPAL_COLOR_MODE_GRAY: break;
			case SMPAL_COLOR_MODE_RGB:
			    fprintf(stderr, "\trgb color mapping by rgbformulae are %i,%i,%i\n", 
					SmPalette.formulaR, SmPalette.formulaG, SmPalette.formulaB);
			    break;
			case SMPAL_COLOR_MODE_GRADIENT:
			    fputs("\tcolor mapping by defined gradient\n", stderr);
			    break;
			case SMPAL_COLOR_MODE_FUNCTIONS:
			    fputs("\tcolor mapping is done by user defined functions\n", stderr);
			    if(SmPalette.Afunc.at && SmPalette.Afunc.definition)
				    fprintf(stderr, "\t  A-formula: %s\n", SmPalette.Afunc.definition);
			    if(SmPalette.Bfunc.at && SmPalette.Bfunc.definition)
				    fprintf(stderr, "\t  B-formula: %s\n", SmPalette.Bfunc.definition);
			    if(SmPalette.Cfunc.at && SmPalette.Cfunc.definition)
				    fprintf(stderr, "\t  C-formula: %s\n", SmPalette.Cfunc.definition);
			    break;
			case SMPAL_COLOR_MODE_CUBEHELIX:
			    fprintf(stderr, "\tCubehelix color palette: start %g cycles %g saturation %g\n",
			    SmPalette.cubehelix_start, SmPalette.cubehelix_cycles,
			    SmPalette.cubehelix_saturation);
			    break;
			default:
			    fprintf(stderr, "%s:%d oops: Unknown color mode '%c'.\n", __FILE__, __LINE__, (char)(SmPalette.colorMode) );
		}
		fprintf(stderr, "\tfigure is %s\n", SmPalette.positive == SMPAL_POSITIVE ? "POSITIVE" : "NEGATIVE");
		fprintf(stderr, "\tall color formulae ARE%s written into output postscript file\n", !SmPalette.ps_allcF ? " NOT" : "");
		fputs("\tallocating ", stderr);
		if(SmPalette.use_maxcolors)
			fprintf(stderr, "MAX %i", SmPalette.use_maxcolors);
		else
			fputs("ALL remaining", stderr);
		fputs(" color positions for discrete palette terminals\n", stderr);
		fputs("\tColor-Model: ", stderr);
		switch(SmPalette.cmodel) {
			case C_MODEL_RGB: fputs("RGB\n", stderr); break;
			case C_MODEL_HSV: fputs("HSV\n", stderr); break;
			case C_MODEL_CMY: fputs("CMY\n", stderr); break;
			case C_MODEL_YIQ: fputs("YIQ\n", stderr); break;
			case C_MODEL_XYZ: fputs("XYZ\n", stderr); break;
			default:
			    fprintf(stderr, "%s:%d ooops: Unknown color mode '%c'.\n", __FILE__, __LINE__, (char)(SmPalette.cmodel) );
		}
		fprintf(stderr, "\tgamma is %.4g\n", SmPalette.gamma);
	}
	else if(rC.AlmostEq("pal$ette")) {
		// 'show palette palette <n>' 
		show_palette_palette(rC);
	}
	else if(rC.AlmostEq("gra$dient")) {
		// 'show palette gradient' 
		ShowPaletteGradient(rC);
	}
	else if(rC.AlmostEq("rgbfor$mulae")) {
		// 'show palette rgbformulae'
		//show_palette_rgbformulae();
		//static void show_palette_rgbformulae()
		{
			int i = 0;
			fprintf(stderr, "\t  * there are %i available rgb color mapping formulae:", SmPalette.colorFormulae);
			// print the description of the color formulae 
			while(*(ps_math_color_formulae[2*i]) ) {
				if(i % 3 == 0)
					fputs("\n\t    ", stderr);
				fprintf(stderr, "%2i: %-15s", i, ps_math_color_formulae[2*i+1]);
				i++;
			}
			fputs("\n", stderr);
			fputs("\t  * negative numbers mean inverted=negative colour component\n", stderr);
			fprintf(stderr, "\t  * thus the ranges in `set pm3d rgbformulae' are -%i..%i\n", 
				SmPalette.colorFormulae-1, SmPalette.colorFormulae-1);
			++rC.CToken;
		}
	}
	else if(rC.Eq("colors") || rC.AlmostEq("color$names")) {
		// 'show palette colornames'
		show_palette_colornames(rC);
	}
	else if(rC.AlmostEq("fit2rgb$formulae")) {
		// 'show palette fit2rgbformulae'
		//show_palette_fit2rgbformulae();
		//static void show_palette_fit2rgbformulae()
		{
#define rgb_distance(r, g, b) ((r)*(r) + (g)*(g) + (b)*(b))
			int pts = 32; // resolution: nb of points in the discrete raster for comparisons 
			int i, p, ir, ig, ib;
			int rMin = 0, gMin = 0, bMin = 0;
			int maxFormula = SmPalette.colorFormulae - 1; // max formula number 
			double gray, dist, distMin;
			rgb_color * currRGB;
			int * formulaeSeq;
			double ** formulae;
			++rC.CToken;
			if(SmPalette.colorMode == SMPAL_COLOR_MODE_RGB && SmPalette.cmodel == C_MODEL_RGB) {
				fprintf(stderr, "\tCurrent palette is\n\t    set palette rgbformulae %i,%i,%i\n", 
					SmPalette.formulaR, SmPalette.formulaG, SmPalette.formulaB);
				return;
			}
			// allocate and fill R, G, B values rastered on pts points 
			currRGB = (rgb_color*)malloc(pts * sizeof(rgb_color));
			for(p = 0; p < pts; p++) {
				gray = (double)p / (pts - 1);
				RGB1FromGray(gray, &(currRGB[p]));
			}
			// organize sequence of rgb formulae 
			formulaeSeq = (int *)malloc((2*maxFormula+1) * sizeof(int));
			for(i = 0; i <= maxFormula; i++)
				formulaeSeq[i] = i;
			for(i = 1; i <= maxFormula; i++)
				formulaeSeq[maxFormula+i] = -i;
			// allocate and fill all +-formulae on the interval of given number of points 
			formulae = (double **)malloc((2*maxFormula+1) * sizeof(double*));
			for(i = 0; i < 2*maxFormula+1; i++) {
				formulae[i] = (double *)malloc(pts * sizeof(double));
				for(p = 0; p < pts; p++) {
					double gray = (double)p / (pts - 1);
					formulae[i][p] = GetColorValueFromFormula(formulaeSeq[i], gray);
				}
			}
			// Now go over all rastered formulae, compare them to the current one, and
			// find the minimal distance.
			distMin = GPVL;
			for(ir = 0; ir <    2*maxFormula+1; ir++) {
				for(ig = 0; ig < 2*maxFormula+1; ig++) {
					for(ib = 0; ib < 2*maxFormula+1; ib++) {
						dist = 0; // calculate distance of the two rgb profiles 
						for(p = 0; p < pts; p++) {
							double tmp = rgb_distance( currRGB[p].r - formulae[ir][p], currRGB[p].g - formulae[ig][p], currRGB[p].b - formulae[ib][p]);
							dist += tmp;
						}
						if(dist < distMin) {
							distMin = dist;
							rMin = formulaeSeq[ir];
							gMin = formulaeSeq[ig];
							bMin = formulaeSeq[ib];
						}
					}
				}
			}
			fprintf(stderr, "\tThe best match of the current palette corresponds to\n\t    set palette rgbformulae %i,%i,%i\n", rMin, gMin, bMin);
#undef rgb_distance
			for(i = 0; i < 2*maxFormula+1; i++)
				free(formulae[i]);
			free(formulae);
			free(formulaeSeq);
			free(currRGB);
		}
	}
	else { // wrong option to "show palette" 
		IntError(rC, rC.CToken, "Expecting 'gradient' or 'palette <n>' or 'rgbformulae' or 'colornames'");
	}
}

//static void show_colorbox()
void GpGadgets::ShowColorBox(GpCommand & rC)
{
	rC.CToken++;
	if(ColorBox.border) {
		fputs("\tcolor box with border, ", stderr);
		if(ColorBox.border_lt_tag >= 0)
			fprintf(stderr, "line type %d is ", ColorBox.border_lt_tag);
		else
			fputs("DEFAULT line type is ", stderr);
	}
	else {
		fputs("\tcolor box without border is ", stderr);
	}
	if(ColorBox.where != SMCOLOR_BOX_NO) {
		if(ColorBox.layer == LAYER_FRONT)
			fputs("drawn front\n\t", stderr);
		else
			fputs("drawn back\n\t", stderr);
	}
	switch(ColorBox.where) {
		case SMCOLOR_BOX_NO:
		    fputs("NOT drawn\n", stderr);
		    break;
		case SMCOLOR_BOX_DEFAULT:
		    fputs("at DEFAULT position\n", stderr);
		    break;
		case SMCOLOR_BOX_USER:
		    fputs("at USER origin: ", stderr);
		    show_position(&ColorBox.origin, 2);
		    fputs("\n\t          size: ", stderr);
		    show_position(&ColorBox.size, 2);
		    fputs("\n", stderr);
		    break;
		default: /* should *never* happen */
		    GpGg.IntError(GpC, NO_CARET, "Argh!");
	}
	fprintf(stderr, "\tcolor gradient is %s in the color box\n", ColorBox.rotation == 'v' ? "VERTICAL" : "HORIZONTAL");
}

//static void show_pm3d()
void GpGadgets::ShowPm3D(GpCommand & rC)
{
	rC.CToken++;
	fprintf(stderr, "\tpm3d style is %s\n", PM3D_IMPLICIT == Pm3D.implicit ? "implicit (pm3d draw for all surfaces)" : "explicit (draw pm3d surface according to style)");
	fputs("\tpm3d plotted at ", stderr);
	{ 
		for(int i = 0; Pm3D.where[i]; i++) {
			if(i>0) 
				fputs(", then ", stderr);
			switch(Pm3D.where[i]) {
				case PM3D_AT_BASE: fputs("BOTTOM", stderr); break;
				case PM3D_AT_SURFACE: fputs("SURFACE", stderr); break;
				case PM3D_AT_TOP: fputs("TOP", stderr); break;
			}
		}
		fputs("\n", stderr); 
	}
	if(Pm3D.direction == PM3D_DEPTH) {
		fprintf(stderr, "\ttrue depth ordering\n");
	}
	else if(Pm3D.direction != PM3D_SCANS_AUTOMATIC) {
		fprintf(stderr, "\ttaking scans in %s direction\n", Pm3D.direction == PM3D_SCANS_FORWARD ? "FORWARD" : "BACKWARD");
	}
	else {
		fputs("\ttaking scans direction automatically\n", stderr);
	}
	fputs("\tsubsequent scans with different nb of pts are ", stderr);
	if(Pm3D.flush == PM3D_FLUSH_CENTER)
		fputs("CENTERED\n", stderr);
	else
		fprintf(stderr, "flushed from %s\n", Pm3D.flush == PM3D_FLUSH_BEGIN ? "BEGIN" : "END");
	fprintf(stderr, "\tflushing triangles are %sdrawn\n", Pm3D.ftriangles ? "" : "not ");
	fputs("\tclipping: ", stderr);
	if(Pm3D.clip == PM3D_CLIP_1IN)
		fputs("at least 1 point of the quadrangle in x,y ranges\n", stderr);
	else
		fputs("all 4 points of the quadrangle in x,y ranges\n", stderr);
	if(Pm3D.border.l_type == LT_NODRAW) {
		fprintf(stderr, "\tpm3d quadrangles will have no border\n");
	}
	else {
		fprintf(stderr, "\tpm3d quadrangle borders will default to ");
		save_linetype(stderr, &Pm3D.border, false);
		fprintf(stderr, "\n");
	}
	if(Pm3DShade.strength > 0) {
		fprintf(stderr, "\tlighting primary component %g specular component %g\n", Pm3DShade.strength, Pm3DShade.spec);
	}
	fprintf(stderr, "\tsteps for bilinear interpolation: %d,%d\n", Pm3D.interp_i, Pm3D.interp_j);
	fprintf(stderr, "\tquadrangle color according to ");
	switch(Pm3D.which_corner_color) {
		case PM3D_WHICHCORNER_MEAN: fputs("averaged 4 corners\n", stderr); break;
		case PM3D_WHICHCORNER_GEOMEAN: fputs("geometrical mean of 4 corners\n", stderr); break;
		case PM3D_WHICHCORNER_HARMEAN: fputs("harmonic mean of 4 corners\n", stderr); break;
		case PM3D_WHICHCORNER_MEDIAN: fputs("median of 4 corners\n", stderr); break;
		case PM3D_WHICHCORNER_MIN: fputs("minimum of 4 corners\n", stderr); break;
		case PM3D_WHICHCORNER_MAX: fputs("maximum of 4 corners\n", stderr); break;
		case PM3D_WHICHCORNER_RMS: fputs("root mean square of 4 corners\n", stderr); break;
		default: fprintf(stderr, "corner %i\n", Pm3D.which_corner_color - PM3D_WHICHCORNER_C1 + 1);
	}
}

/* process 'show pointsize' command */
static void show_pointsize()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tpointsize is %g\n", GpGg.PtSz);
}

/* process 'show GpGg.pointintervalbox' command */
static void show_pointintervalbox()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tpointintervalbox is %g\n", GpGg.PtIntervalBox);
}

/* process 'show encoding' command */
static void show_encoding()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tnominal character encoding is %s\n", encoding_names[encoding]);
#ifdef HAVE_LOCALE_H
	fprintf(stderr, "\thowever LC_CTYPE in current locale is %s\n", setlocale(LC_CTYPE, NULL));
#endif
}

/* process 'show decimalsign' command */
static void show_decimalsign()
{
	SHOW_ALL_NL;

	set_numeric_locale();
	fprintf(stderr, "\tdecimalsign for input is  %s \n", get_decimal_locale());
	reset_numeric_locale();
	if(decimalsign!=NULL)
		fprintf(stderr, "\tdecimalsign for output is %s \n", decimalsign);
	else
		fprintf(stderr, "\tdecimalsign for output has default value (normally '.')\n");
	fprintf(stderr, "\tdegree sign for output is %s \n", degree_sign);
}
//
// process 'show fit' command
//
//static void show_fit()
void GpGadgets::ShowFit()
{
	UdvtEntry * v = NULL;
	double d;
	SHOW_ALL_NL;
	switch(GpF.fit_verbosity) {
		case QUIET:
		    fprintf(stderr, "\tfit will not output results to console.\n");
		    break;
		case RESULTS:
		    fprintf(stderr, "\tfit will only print final results to console and log-file.\n");
		    break;
		case BRIEF:
		    fprintf(stderr, "\tfit will output brief results to console and log-file.\n");
		    if(GpF.fit_wrap)
			    fprintf(stderr, "\toutput of long lines will be wrapped at column %i.\n", GpF.fit_wrap);
		    break;
		case VERBOSE:
		    fprintf(stderr, "\tfit will output verbose results to console and log-file.\n");
		    break;
	}
	fprintf(stderr, "\tfit can handle up to %d independent variables\n", MIN(MAX_NUM_VAR, MAXDATACOLS-2));
	fprintf(stderr, "\tfit will%s prescale parameters by their initial values\n", GpF.fit_prescale ? "" : " not");
	fprintf(stderr, "\tfit will%s place parameter errors in variables\n", GpF.fit_errorvariables ? "" : " not");
	fprintf(stderr, "\tfit will%s place covariances in variables\n", GpF.fit_covarvariables ? "" : " not");
	fprintf(stderr, "\tfit will%s scale parameter errors with the reduced chi square\n", GpF.fit_errorscaling ? "" : " not");
	if(GpF.fit_suppress_log) {
		fprintf(stderr, "\tfit will not create a log file\n");
	}
	else if(GpF.fitlogfile) {
		fprintf(stderr, "\tlog-file for fits was set by the user to \n\t'%s'\n", GpF.fitlogfile);
	}
	else {
		char * logfile = GpF.GetLogfile();
		if(logfile) {
			fprintf(stderr, "\tlog-file for fits is unchanged from the environment default of\n\t\t'%s'\n", logfile);
			free(logfile);
		}
	}
	v = Ev.GetUdvByName(GpF.FITLIMIT);
	d = (v && (v->udv_value.type != NOTDEFINED)) ? v->udv_value.Real() : -1.0;
	fprintf(stderr, "\tfits will be considered to have converged if  delta chisq < chisq * %g", ((d > 0.) && (d < 1.)) ? d : DEF_FIT_LIMIT);
	if(GpF.epsilon_abs > 0.)
		fprintf(stderr, " + %g", GpF.epsilon_abs);
	fprintf(stderr, "\n");
	v = Ev.GetUdvByName(GpF.FITMAXITER);
	if(v && (v->udv_value.type != NOTDEFINED) && (real_int(&(v->udv_value)) > 0))
		fprintf(stderr, "\tfit will stop after a maximum of %i iterations\n", real_int(&(v->udv_value)));
	else
		fprintf(stderr, "\tfit has no limit in the number of iterations\n");
	v = Ev.GetUdvByName(GpF.FITSTARTLAMBDA);
	d = (v && (v->udv_value.type != NOTDEFINED)) ? v->udv_value.Real() : -1.0;
	if(d > 0.)
		fprintf(stderr, "\tfit will start with lambda = %g\n", d);
	v = Ev.GetUdvByName(GpF.FITLAMBDAFACTOR);
	d = (v && (v->udv_value.type != NOTDEFINED)) ? v->udv_value.Real() : -1.0;
	if(d > 0.0)
		fprintf(stderr, "\tfit will change lambda by a factor of %g\n", d);
	if(GpF.fit_v4compatible)
		fprintf(stderr, "\tfit command syntax is backwards compatible to version 4\n");
	else
		fprintf(stderr, "\tfit will default to `unitweights` if no `error`keyword is given on the command line.\n");
	fprintf(stderr, "\tfit can run the following command when interrupted:\n\t\t'%s'\n", GpF.GetFitScript());
	v = Ev.GetUdvByName("GPVAL_LAST_FIT");
	if(v && v->udv_value.type != NOTDEFINED)
		fprintf(stderr, "\tlast fit command was: %s\n", v->udv_value.v.string_val);
}
//
// process 'show polar' command
//
static void show_polar()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tpolar is %s\n", (GpGg.IsPolar) ? "ON" : "OFF");
}

// process 'show angles' command 
//static void show_angles()
void GpGadgets::ShowAngles()
{
	SHOW_ALL_NL;
	fputs("\tAngles are in ", stderr);
	if(Ang2Rad == 1) {
		fputs("radians\n", stderr);
	}
	else {
		fputs("degrees\n", stderr);
	}
}

// process 'show samples' command 
//static void show_samples()
void GpGadgets::ShowSamples()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tsampling rate is %d, %d\n", Samples1, Samples2);
}

// process 'show isosamples' command 
//static void show_isosamples()
void GpGadgets::ShowIsoSamples()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tiso sampling rate is %d, %d\n", iso_samples_1, iso_samples_2);
}
//
// process 'show view' command 
//
void GpGadgets::ShowView()
{
	SHOW_ALL_NL;
	fputs("\tview is ", stderr);
	if(splot_map) {
		fprintf(stderr, "map scale %g\n", mapview_scale);
	}
	else {
		fprintf(stderr, "%g rot_x, %g rot_z, %g scale, %g scale_z\n", surface_rot_x, surface_rot_z, surface_scale, surface_zscale);
		fprintf(stderr, "\t\t%s axes are %s\n", AspectRatio3D == 2 ? "x/y" : AspectRatio3D == 3 ? "x/y/z" : "",
			AspectRatio3D >= 2 ? "on the same scale" : "independently scaled");
	}
}
//
// process 'show surface' command
//
//static void show_surface()
void GpGadgets::ShowSurface()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tsurface is %sdrawn %s\n", draw_surface ? "" : "not ", implicit_surface ? "" : "only if explicitly requested");
}

/* process 'show hidden3d' command */
static void show_hidden3d()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\thidden surface is %s\n", GpGg.hidden3d ? "removed" : "drawn");
	show_hidden3doptions();
}

static void show_increment()
{
	fprintf(stderr, "\tPlot lines increment over ");
	if(GpGg.prefer_line_styles)
		fprintf(stderr, "user-defined line styles rather than default line types\n");
	else
		fprintf(stderr, "default linetypes\n");
}

static void show_histogram()
{
	fprintf(stderr, "\tHistogram style is ");
	save_histogram_opts(stderr);
}

#ifdef EAM_BOXED_TEXT
//static void show_textbox()
void GpGadgets::ShowTextBox()
{
	fprintf(stderr, "\ttextboxes are %s ", textbox_opts.opaque ? "opaque" : "transparent");
	fprintf(stderr, "with margins %4.1f, %4.1f  and %s border\n", 
		textbox_opts.xmargin, textbox_opts.ymargin, textbox_opts.noborder ? "no" : "");
}
#endif
//
// process 'show history' command 
//
//static void show_history()
void GpCommand::ShowHistory()
{
#ifndef GNUPLOT_HISTORY
	fprintf(stderr, "\tThis copy of gnuplot was not built to use a command history file\n");
#endif
	fprintf(stderr, "\t history size %d%s,  %s,  %s\n",
	    H.gnuplot_history_size, (H.gnuplot_history_size < 0) ? "(unlimited)" : "",
	    H.history_quiet ? "quiet" : "numbers",
	    H.history_full ? "full" : "suppress duplicates");
}

/* process 'show size' command */
static void show_size()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tsize is scaled by %g,%g\n", GpGg.XSz, GpGg.YSz);
	if(GpGg.AspectRatio > 0)
		fprintf(stderr, "\tTry to set aspect ratio to %g:1.0\n", GpGg.AspectRatio);
	else if(GpGg.AspectRatio == 0)
		fputs("\tNo attempt to control aspect ratio\n", stderr);
	else
		fprintf(stderr, "\tTry to set LOCKED aspect ratio to %g:1.0\n", -GpGg.AspectRatio);
}

/* process 'show origin' command */
static void show_origin()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\torigin is set to %g,%g\n", GpGg.XOffs, GpGg.YOffs);
}

/* process 'show term' command */
static void show_term()
{
	SHOW_ALL_NL;
	if(term)
		fprintf(stderr, "   terminal type is %s %s\n", term->name, term_options);
	else
		fputs("\tterminal type is unknown\n", stderr);
}
//
// process 'show tics|[xyzx2y2cb]tics' commands 
//
//static void show_tics(bool showx, bool showy, bool showz, bool showx2, bool showy2, bool showcb)
void GpGadgets::ShowTics(bool showx, bool showy, bool showz, bool showx2, bool showy2, bool showcb)
{
	int i;
	SHOW_ALL_NL;
	if(xyplane.IsAbsolute)
		fprintf(stderr, "\txyplane intercepts z axis at %g\n", xyplane.Z);
	else
		fprintf(stderr, "\txyplane ticslevel is %g\n", xyplane.Z);
	fprintf(stderr, "\ttics are in %s of plot\n", (grid_tics_in_front) ? "front" : "back");
	if(showx)
		ShowTicDef(FIRST_X_AXIS);
	if(showx2)
		ShowTicDef(SECOND_X_AXIS);
	if(showy)
		ShowTicDef(FIRST_Y_AXIS);
	if(showy2)
		ShowTicDef(SECOND_Y_AXIS);
	if(showz)
		ShowTicDef(FIRST_Z_AXIS);
	if(showcb)
		ShowTicDef(COLOR_AXIS);
	fprintf(stderr, "\tScales for user tic levels 2-%d are: ", MAX_TICLEVEL-1);
	for(i = 2; i < MAX_TICLEVEL; i++)
		fprintf(stderr, " %g%c", TicScale[i], i<MAX_TICLEVEL-1 ? ',' : '\n');
	screen_ok = false;
}
//
// process 'show m[xyzx2y2cb]tics' commands
//
static void show_mtics(AXIS_INDEX axis)
{
	const char * p_ax_name = GpGg.GetAxisName(axis);
	switch(GpGg[axis].minitics) {
		case MINI_OFF:
		    fprintf(stderr, "\tminor %stics are off\n", p_ax_name);
		    break;
		case MINI_DEFAULT:
		    fprintf(stderr, "\tminor %stics are off for linear scales\n\tminor %stics are computed automatically for log scales\n", p_ax_name, p_ax_name);
		    break;
		case MINI_AUTO:
		    fprintf(stderr, "\tminor %stics are computed automatically\n", p_ax_name);
		    break;
		case MINI_USER:
		    fprintf(stderr, "\tminor %stics are drawn with %d subintervals between major xtic marks\n", p_ax_name, (int)GpGg[axis].mtic_freq);
		    break;
		default:
		    GpGg.IntError(GpC, NO_CARET, "Unknown minitic type in show_mtics()");
	}
}
//
// process 'show timestamp' command 
//
//static void show_timestamp()
void GpGadgets::ShowTimestamp()
{
	SHOW_ALL_NL;
	show_xyzlabel("", "time", &timelabel);
	fprintf(stderr, "\twritten in %s corner\n", (timelabel_bottom ? "bottom" : "top"));
	if(timelabel_rotate)
		fputs("\trotated if the terminal allows it\n\t", stderr);
	else
		fputs("\tnot rotated\n\t", stderr);
}

/* process 'show [xyzx2y2rtuv]range' commands */
static void show_range(AXIS_INDEX axis)
{
	SHOW_ALL_NL;
	if(GpGg[axis].datatype == DT_TIMEDATE)
		fprintf(stderr, "\tset %sdata time\n", GpGg.GetAxisName(axis));
	fprintf(stderr, "\t");
	save_prange(stderr, &GpGg[axis]);
}

/* called by the functions below */
static void show_xyzlabel(const char * name, const char * suffix, GpTextLabel * label)
{
	if(label) {
		fprintf(stderr, "\t%s%s is \"%s\", offset at ", name, suffix, label->text ? conv_text(label->text) : "");
		show_position(&label->offset, 3);
	}
	else
		return;

	if(label->font)
		fprintf(stderr, ", using font \"%s\"", conv_text(label->font));

	if(label->tag == ROTATE_IN_3D_LABEL_TAG)
		fprintf(stderr, ", parallel to axis in 3D plots");
	else if(label->rotate)
		fprintf(stderr, ", rotated by %d degrees in 2D plots", label->rotate);

	if(label->textcolor.type)
		save_textcolor(stderr, &label->textcolor);

	if(label->noenhanced)
		fprintf(stderr, " noenhanced");

	putc('\n', stderr);
}

/* process 'show title' command */
static void show_title()
{
	SHOW_ALL_NL;
	show_xyzlabel("", "title", &GpGg.title);
}

/* process 'show {x|y|z|x2|y2}label' command */
static void show_axislabel(AXIS_INDEX axis)
{
	SHOW_ALL_NL;
	show_xyzlabel(GpGg.GetAxisName(axis), "label", &GpGg[axis].label);
}

/* process 'show [xyzx2y2]data' commands */
static void show_data_is_timedate(AXIS_INDEX axis)
{
	SHOW_ALL_NL;
	fprintf(stderr, "\t%s is set to %s\n", GpGg.GetAxisName(axis),
	    GpGg[axis].datatype == DT_TIMEDATE ? "time" : GpGg[axis].datatype == DT_DMS ? "geographic" :  /* obsolete */ "numerical");
}

/* process 'show P_TimeFormat' command */
static void show_timefmt()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tDefault format for reading time data is \"%s\"\n", (const char *)GpGg.P_TimeFormat);
}

/* process 'show link' command */
static void show_nonlinear()
{
	for(int axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++)
		save_nonlinear(stderr, &GpGg[axis]);
}

/* process 'show locale' command */
static void show_locale()
{
	SHOW_ALL_NL;
	locale_handler(ACTION_SHOW, NULL);
}

/* process 'show loadpath' command */
static void show_loadpath()
{
	SHOW_ALL_NL;
	loadpath_handler(ACTION_SHOW, NULL);
}

/* process 'show fontpath' command */
static void show_fontpath()
{
	SHOW_ALL_NL;
	fontpath_handler(ACTION_SHOW, NULL);
}

/* process 'show zero' command */
static void show_zero()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tzero is %g\n", GpGg.Zero);
}
//
// process 'show datafile' command 
//
static void show_datafile(GpCommand & rC)
{
	SHOW_ALL_NL;
	if(rC.EndOfCommand() || rC.AlmostEq("miss$ing")) {
		if(GpDf.missing_val == NULL)
			fputs("\tNo missing data string set for datafile\n", stderr);
		else
			fprintf(stderr, "\t\"%s\" in datafile is interpreted as missing value\n", GpDf.missing_val);
	}
	if(rC.EndOfCommand() || rC.AlmostEq("sep$arators")) {
		if(GpDf.df_separators)
			fprintf(stderr, "\tdatafile fields separated by any of %d characters \"%s\"\n", (int)strlen(GpDf.df_separators), GpDf.df_separators);
		else
			fprintf(stderr, "\tdatafile fields separated by whitespace\n");
	}
	if(rC.EndOfCommand() || rC.AlmostEq("com$mentschars")) {
		fprintf(stderr, "\tComments chars are \"%s\"\n", GpDf.df_commentschars);
	}
	if(GpDf.df_fortran_constants)
		fputs("\tDatafile parsing will accept Fortran D or Q constants\n", stderr);
	if(GpDf.df_nofpe_trap)
		fputs("\tNo floating point exception handler during data input\n", stderr);
	if(rC.AlmostEq("bin$ary")) {
		if(!rC.EndOfCommand())
			rC.CToken++;
		if(rC.EndOfCommand()) {
			/* 'show datafile binary' */
			GpDf.DfShowBinary(stderr);
			fputc('\n', stderr);
		}
		if(rC.EndOfCommand() || rC.AlmostEq("datas$izes"))
			/* 'show datafile binary datasizes' */
			df_show_datasizes(stderr);
		if(rC.EndOfCommand())
			fputc('\n', stderr);
		if(rC.EndOfCommand() || rC.AlmostEq("filet$ypes"))
			/* 'show datafile binary filetypes' */
			df_show_filetypes(stderr);
	}
	if(!rC.EndOfCommand())
		rC.CToken++;
}

#ifdef USE_MOUSE
// process 'show mouse' command 
//static void show_mouse()
void GpGadgets::ShowMouse()
{
	SHOW_ALL_NL;
	if(Mse.Cfg.on) {
		fprintf(stderr, "\tmouse is on\n");
		if(Mse.Cfg.annotate_zoom_box) {
			fprintf(stderr, "\tzoom coordinates will be drawn\n");
		}
		else {
			fprintf(stderr, "\tno zoom coordinates will be drawn\n");
		}
		if(Mse.Cfg.polardistance) {
			fprintf(stderr, "\tdistance to ruler will be show in polar coordinates\n");
		}
		else {
			fprintf(stderr, "\tno polar distance to ruler will be shown\n");
		}
		if(Mse.Cfg.doubleclick > 0) {
			fprintf(stderr, "\tdouble click resolution is %d ms\n", Mse.Cfg.doubleclick);
		}
		else {
			fprintf(stderr, "\tdouble click resolution is off\n");
		}
		fprintf(stderr, "\tformatting numbers with \"%s\"\n", Mse.Cfg.fmt);
		fprintf(stderr, "\tformat for Button 2 is %d\n", (int)mouse_mode);
		if(mouse_alt_string) {
			fprintf(stderr, "\talternative format for Button 2 is '%s'\n", mouse_alt_string);
		}
		if(Mse.Cfg.label) {
			fprintf(stderr, "\tButton 2 draws persistent labels with options \"%s\"\n", Mse.Cfg.labelopts);
		}
		else {
			fprintf(stderr, "\tButton 2 draws temporary labels\n");
		}
		fprintf(stderr, "\tzoom factors are x: %g   y: %g\n", Mse.Cfg.xmzoom_factor, Mse.Cfg.ymzoom_factor);
		fprintf(stderr, "\tzoomjump is %s\n", Mse.Cfg.warp_pointer ? "on" : "off");
		fprintf(stderr, "\tcommunication commands will %sbe shown\n", Mse.Cfg.verbose ? "" : "not ");
	}
	else {
		fprintf(stderr, "\tmouse is off\n");
	}
}

#endif
//
// process 'show plot' command
//
static void show_plot()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tlast plot command was: %s\n", GpC.P_ReplotLine);
}

// process 'show variables' command 
//static void show_variables()
void GpEval::ShowVariables(GpCommand & rC)
{
	UdvtEntry * udv = first_udv;
	int len;
	bool show_all = false;
	char leading_string[MAX_ID_LEN+1] = {'\0'};
	if(!rC.EndOfCommand()) {
		if(rC.AlmostEq("all"))
			show_all = true;
		else
			rC.CopyStr(leading_string, rC.CToken, MAX_ID_LEN);
		rC.CToken++;
	}
	if(show_all)
		fputs("\n\tAll available variables:\n", stderr);
	else if(*leading_string)
		fprintf(stderr, "\n\tVariables beginning with %s:\n", leading_string);
	else
		fputs("\n\tUser and default variables:\n", stderr);
	while(udv) {
		len = strcspn(udv->udv_name, " ");
		if(*leading_string && strncmp(udv->udv_name, leading_string, strlen(leading_string))) {
			udv = udv->next_udv;
			continue;
		}
		else if(!show_all && !strncmp(udv->udv_name, "GPVAL_", 6) && !(*leading_string)) {
			// In the default case skip GPVAL_ variables
			udv = udv->next_udv;
			continue;
		}
		if(udv->udv_value.type == NOTDEFINED) {
			FPRINTF((stderr, "\t%-*s is undefined\n", len, udv->udv_name));
		}
		else {
			fprintf(stderr, "\t%-*s ", len, udv->udv_name);
			fputs("= ", stderr);
			disp_value(stderr, &(udv->udv_value), true);
			putc('\n', stderr);
		}
		udv = udv->next_udv;
	}
}

/* Show line style number <tag> (0 means show all) */
static void show_linestyle(int tag)
{
	linestyle_def * this_linestyle;
	bool showed = false;
	for(this_linestyle = GpGg.first_linestyle; this_linestyle != NULL; this_linestyle = this_linestyle->next) {
		if(tag == 0 || tag == this_linestyle->tag) {
			showed = true;
			fprintf(stderr, "\tlinestyle %d, ", this_linestyle->tag);
			save_linetype(stderr, &(this_linestyle->lp_properties), true);
			fputc('\n', stderr);
		}
	}
	if(tag > 0 && !showed)
		GpGg.IntError(GpC, GpC.CToken, "linestyle not found");
}

/* Show linetype number <tag> (0 means show all) */
static void show_linetype(linestyle_def * listhead, int tag)
{
	linestyle_def * this_linestyle;
	bool showed = false;
	int recycle_count = 0;
	for(this_linestyle = listhead; this_linestyle != NULL;
	    this_linestyle = this_linestyle->next) {
		if(tag == 0 || tag == this_linestyle->tag) {
			showed = true;
			fprintf(stderr, "\tlinetype %d, ", this_linestyle->tag);
			save_linetype(stderr, &(this_linestyle->lp_properties), true);
			fputc('\n', stderr);
		}
	}
	if(tag > 0 && !showed)
		GpGg.IntError(GpC, GpC.CToken, "linetype not found");
	if(listhead == GpGg.first_perm_linestyle)
		recycle_count = linetype_recycle_count;
	else if(listhead == GpGg.first_mono_linestyle)
		recycle_count = mono_recycle_count;
	if(tag == 0 && recycle_count > 0)
		fprintf(stderr, "\tLinetypes repeat every %d unless explicitly defined\n", recycle_count);
}
//
// Show arrow style number <tag> (0 means show all) 
//
static void show_arrowstyle(int tag)
{
	bool showed = false;
	for(arrowstyle_def * p_arrwstyle = GpGg.first_arrowstyle; p_arrwstyle != NULL; p_arrwstyle = p_arrwstyle->next) {
		if(tag == 0 || tag == p_arrwstyle->tag) {
			showed = true;
			fprintf(stderr, "\tarrowstyle %d, ", p_arrwstyle->tag);
			fflush(stderr);
			fprintf(stderr, "\t %s %s", p_arrwstyle->arrow_properties.head ? (p_arrwstyle->arrow_properties.head==2 ? " both heads " : " one head ") : " nohead",
			    p_arrwstyle->arrow_properties.layer ? "front" : "back");
			save_linetype(stderr, &(p_arrwstyle->arrow_properties.lp_properties), false);
			fputc('\n', stderr);
			if(p_arrwstyle->arrow_properties.head > 0) {
				fprintf(stderr, "\t  arrow heads: %s, ",
				    (p_arrwstyle->arrow_properties.headfill==AS_FILLED) ? "filled" :
				    (p_arrwstyle->arrow_properties.headfill==AS_EMPTY) ? "empty" :
				    (p_arrwstyle->arrow_properties.headfill==AS_NOBORDER) ? "noborder" :
				    "nofilled");
				if(p_arrwstyle->arrow_properties.head_length > 0) {
					static char * msg[] = { 
						"(first x axis) ", "(second x axis) ",
						"(graph units) ", "(screen units) ",
						"(character units) "
					};
					fprintf(stderr, " length %s%g, angle %g deg",
					    p_arrwstyle->arrow_properties.head_lengthunit ==
					    first_axes ? "" : msg[p_arrwstyle->arrow_properties.head_lengthunit],
					    p_arrwstyle->arrow_properties.head_length,
					    p_arrwstyle->arrow_properties.head_angle);
					if(p_arrwstyle->arrow_properties.headfill != AS_NOFILL)
						fprintf(stderr, ", backangle %g deg", p_arrwstyle->arrow_properties.head_backangle);
				}
				else {
					fprintf(stderr, " (default length and angles)");
				}
				fprintf(stderr, (p_arrwstyle->arrow_properties.head_fixedsize) ? " fixed\n" : "\n");
			}
		}
	}
	if(tag > 0 && !showed)
		GpGg.IntError(GpC, GpC.CToken, "arrowstyle not found");
}
//
// called by show_tics 
//
//static void show_ticdefp(GpAxis * pAx)
void GpGadgets::ShowTicDefP(GpAxis & rAx)
{
	ticmark * t;
	const char * ticfmt = conv_text(rAx.formatstring);
	fprintf(stderr, "\t%s-axis tics are %s, \tmajor ticscale is %g and minor ticscale is %g\n",
		GetAxisName(rAx.Index), ((rAx.Flags & GpAxis::fTicIn) ? "IN" : "OUT"), rAx.ticscale, rAx.miniticscale);
	fprintf(stderr, "\t%s-axis tics:\t", GetAxisName(rAx.Index));
	switch(rAx.ticmode & TICS_MASK) {
		case NO_TICS:
		    fputs("OFF\n", stderr);
		    return;
		case TICS_ON_AXIS:
		    fputs("on axis", stderr);
		    if(rAx.ticmode & TICS_MIRROR)
				fprintf(stderr, " and mirrored %s", ((rAx.Flags & GpAxis::fTicIn) ? "OUT" : "IN"));
		    break;
		case TICS_ON_BORDER:
		    fputs("on border", stderr);
		    if(rAx.ticmode & TICS_MIRROR)
			    fputs(" and mirrored on opposite border", stderr);
		    break;
	}
	if(rAx.ticdef.rangelimited)
		fprintf(stderr, "\n\t  tics are limited to data range");
	fputs("\n\t  labels are ", stderr);
	if(rAx.Flags & GpAxis::fManualJustify) {
		switch(rAx.label.pos) {
			case LEFT: {
			    fputs("left justified, ", stderr);
			    break;
		    }
			case RIGHT: {
			    fputs("right justified, ", stderr);
			    break;
		    }
			case CENTRE: {
			    fputs("center justified, ", stderr);
			    break;
		    }
		}
	}
	else
		fputs("justified automatically, ", stderr);
	fprintf(stderr, "format \"%s\"", ticfmt);
	fprintf(stderr, "%s", rAx.tictype == DT_DMS ? " geographic" : rAx.tictype == DT_TIMEDATE ? " timedate" : "");
	if(rAx.ticdef.enhanced == false)
		fprintf(stderr, "  noenhanced");
	if(rAx.tic_rotate) {
		fprintf(stderr, " rotated");
		fprintf(stderr, " by %d", rAx.tic_rotate);
		fputs(" in 2D mode, terminal permitting,\n\t", stderr);
	}
	else
		fputs(" and are not rotated,\n\t", stderr);
	fputs("    offset ", stderr);
	show_position(&rAx.ticdef.offset, 3);
	fputs("\n\t", stderr);
	switch(rAx.ticdef.type) {
		case TIC_COMPUTED: {
		    fputs("  intervals computed automatically\n", stderr);
		    break;
	    }
		case TIC_MONTH: {
		    fputs("  Months computed automatically\n", stderr);
		    break;
	    }
		case TIC_DAY: {
		    fputs("  Days computed automatically\n", stderr);
		    break;
	    }
		case TIC_SERIES: {
		    fputs("  series", stderr);
		    if(rAx.ticdef.def.series.start != -GPVL) {
			    fputs(" from ", stderr);
			    save_num_or_time_input(stderr, rAx.ticdef.def.series.start, &rAx);
		    }
		    fprintf(stderr, " by %g%s", rAx.ticdef.def.series.incr, rAx.datatype == DT_TIMEDATE ? " secs" : "");
		    if(rAx.ticdef.def.series.end != GPVL) {
			    fputs(" until ", stderr);
			    save_num_or_time_input(stderr, rAx.ticdef.def.series.end, &rAx);
		    }
		    putc('\n', stderr);
		    break;
	    }
		case TIC_USER: {
		    fputs("  no auto-generated tics\n", stderr);
		    break;
	    }
		default: {
		    GpGg.IntError(GpC, NO_CARET, "unknown ticdef type in show_ticdef()"); // NOTREACHED 
	    }
	}
	if(rAx.ticdef.def.user) {
		fputs("\t  explicit list (", stderr);
		for(t = rAx.ticdef.def.user; t != NULL; t = t->next) {
			if(t->label)
				fprintf(stderr, "\"%s\" ", conv_text(t->label));
			save_num_or_time_input(stderr, t->position, &rAx);
			if(t->level)
				fprintf(stderr, " %d", t->level);
			if(t->next)
				fputs(", ", stderr);
		}
		fputs(")\n", stderr);
	}
	if(rAx.ticdef.textcolor.type != TC_DEFAULT) {
		fputs("\t ", stderr);
		save_textcolor(stderr, &rAx.ticdef.textcolor);
		fputs("\n", stderr);
	}
	if(rAx.ticdef.font && *rAx.ticdef.font) {
		fprintf(stderr, "\t  font \"%s\"\n", rAx.ticdef.font);
	}
}
//
// called by show_tics 
//
//static void show_ticdef(AXIS_INDEX axis)
void GpGadgets::ShowTicDef(AXIS_INDEX axIdx)
{
	ShowTicDefP(AxA[axIdx]);
}

/* Display a value in human-readable form. */
void disp_value(FILE * fp, t_value * val, bool need_quotes)
{
	fprintf(fp, "%s", value_to_str(val, need_quotes));
}

/* convert unprintable characters as \okt, tab as \t, newline \n .. */
char * conv_text(const char * t)
{
	static char * empty = "";
	static char * r = NULL, * s;

	if(t==NULL) return empty;

	/* is this enough? */
	r = (char *)gp_realloc(r, 4 * (strlen(t) + 1), "conv_text buffer");
	s = r;
	while(*t != NUL) {
		switch(*t) {
			case '\t':
			    *s++ = '\\';
			    *s++ = 't';
			    break;
			case '\n':
			    *s++ = '\\';
			    *s++ = 'n';
			    break;
			case '\r':
			    *s++ = '\\';
			    *s++ = 'r';
			    break;
			case '"':
			case '\\':
			    *s++ = '\\';
			    *s++ = *t;
			    break;

			default:
			    if(encoding == S_ENC_UTF8)
				    *s++ = *t;
			    else if(isprint((uchar)*t))
				    *s++ = *t;
			    else {
				    *s++ = '\\';
				    sprintf(s, "%03o", (uchar)*t);
				    while(*s != NUL)
					    s++;
			    }
			    break;
		}
		t++;
	}
	*s = NUL;
	return r;
}

