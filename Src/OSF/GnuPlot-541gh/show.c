// GNUPLOT - show.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
/*
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 */
#include <gnuplot.h>
#pragma hdrstop
#ifdef _WIN32
	#include "win/winmain.h"
#endif
#ifdef HAVE_LIBCACA
	#include <caca.h>
#endif

/******** Local functions ********/

static void disp_at(struct at_type *, int);
static void show_autoscale();
static void show_border();
//static void show_boxwidth();
static void show_boxplot();
static void show_fillstyle();
static void show_contour();
//static void show_dashtype(int);
static void show_dgrid3d();
static void show_mapping();
static void show_dummy();
static void show_styles(const char * name, enum PLOT_STYLE style);
static void show_style_rectangle();
static void show_raxis();
static void show_keytitle();
static void show_output();
static void show_overflow();
static void show_parametric();
static void show_pm3d();
static void show_palette_colornames();
static void show_pointsize();
static void show_pointintervalbox();
static void show_rgbmax();
static void show_encoding();
static void show_decimalsign();
static void show_polar();
static void show_print();
static void show_psdir();
static void show_angles();
//static void show_samples();
//static void show_isosamples();
//static void show_view();
static void show_surface();
static void show_hidden3d();
static void show_increment();
static void show_history();
static void show_textbox();
static void show_origin();
static void show_term();
static void show_mtics(GpAxis *);
static void show_nonlinear();
static void show_data_is_timedate(AXIS_INDEX);
static void show_timefmt();
static void show_locale();
static void show_loadpath();
static void show_fontpath();
static void show_zero();
static void show_micro();
static void show_minus_sign();
static void show_mouse();
static void show_plot();
static void show_linestyle(int tag);
static void show_functions();

static int var_show_all = 0;

/* following code segments appear over and over again */
#define SHOW_ALL_NL { if(!var_show_all) putc('\n', stderr); }

#define PROGRAM "G N U P L O T"
//
// The 'show' command 
//
//void show_command()
void GnuPlot::ShowCommand()
{
	enum set_id token_found;
	int tag = 0;
	char * error_message = NULL;
	Pgm.Shift();
	token_found = (enum set_id)Pgm.LookupTableForCurrentToken(&set_tbl[0]);
	// rationalize c_token advancement stuff a bit: 
	if(token_found != S_INVALID)
		Pgm.Shift();
	switch(token_found) {
		case S_ACTIONTABLE: ShowAt(); break;
		case S_ALL: ShowAll(); break;
		case S_VERSION:
		    show_version(stderr);
		    break;
		case S_AUTOSCALE:
		    show_autoscale();
		    break;
		case S_BARS: SaveBars(stderr); break;
		case S_BIND:
		    while(!Pgm.EndOfCommand()) 
				Pgm.Shift();
		    Pgm.Rollback();
			BindCommand();
		    break;
		case S_BORDER:
		    show_border();
		    break;
		case S_BOXWIDTH:
		case S_BOXDEPTH: ShowBoxWidth(); break;
		case S_CLIP: ShowClip(); break;
		case S_CLABEL:
		// contour labels are shown with 'show contour' 
		case S_CONTOUR:
		case S_CNTRPARAM:
		case S_CNTRLABEL:
		    show_contour();
		    break;
		case S_DEBUG:
		    fprintf(stderr, "debug level is %d\n", debug);
		    break;
		case S_DGRID3D:
		    show_dgrid3d();
		    break;
		case S_MACROS:
		    /* Aug 2013: macros are always enabled */
		    break;
		case S_MAPPING:
		    show_mapping();
		    break;
		case S_DUMMY:
		    show_dummy();
		    break;
		case S_FORMAT: ShowFormat(); break;
		case S_FUNCTIONS:
		    show_functions();
		    break;
		case S_GRID: ShowGrid(); break;
		case S_RAXIS:
		    show_raxis();
		    break;
		case S_PAXIS: ShowPAxis(); break;
		case S_ZEROAXIS:
		    ShowZeroAxis(FIRST_X_AXIS);
		    ShowZeroAxis(SECOND_X_AXIS);
		    ShowZeroAxis(FIRST_Y_AXIS);
		    ShowZeroAxis(SECOND_Y_AXIS);
		    ShowZeroAxis(FIRST_Z_AXIS);
		    break;
		case S_XZEROAXIS: ShowZeroAxis(FIRST_X_AXIS); break;
		case S_YZEROAXIS: ShowZeroAxis(FIRST_Y_AXIS); break;
		case S_X2ZEROAXIS: ShowZeroAxis(SECOND_X_AXIS); break;
		case S_Y2ZEROAXIS: ShowZeroAxis(SECOND_Y_AXIS); break;
		case S_ZZEROAXIS: ShowZeroAxis(FIRST_Z_AXIS); break;

#define CHECK_TAG_GT_ZERO                                       \
	if(!Pgm.EndOfCommand()) {                                  \
		tag = IntExpression();                             \
		if(tag <= 0) {                                     \
			error_message =  "tag must be > zero";          \
			break;                                          \
		}                                               \
	}                                                       \
	putc('\n', stderr);

		case S_LABEL:
		    CHECK_TAG_GT_ZERO;
		    ShowLabel(tag);
		    break;
		case S_ARROW:
		    CHECK_TAG_GT_ZERO;
		    ShowArrow(tag);
		    break;
		case S_LINESTYLE:
		    CHECK_TAG_GT_ZERO;
		    show_linestyle(tag);
		    break;
		case S_LINETYPE:
		    CHECK_TAG_GT_ZERO;
		    ShowLineType(Gg.P_FirstPermLineStyle, tag);
		    break;
		case S_MONOCHROME:
		    fprintf(stderr, "monochrome mode is %s\n", monochrome ? "active" : "not active");
		    if(Pgm.EqualsCur("lt") || Pgm.AlmostEqualsCur("linet$ype")) {
			    Pgm.Shift();
			    CHECK_TAG_GT_ZERO;
		    }
		    ShowLineType(Gg.P_FirstMonoLineStyle, tag);
		    break;
		case S_DASHTYPE:
		    CHECK_TAG_GT_ZERO;
		    ShowDashType(tag);
		    break;
		case S_LINK: ShowLink(); break;
		case S_NONLINEAR:
		    show_nonlinear();
		    break;
		case S_KEY: ShowKey(); break;
		case S_LOGSCALE: ShowLogScale(); break;
		case S_MICRO:
		    show_micro();
		    break;
		case S_MINUS_SIGN:
		    show_minus_sign();
		    break;
		case S_OFFSETS: ShowOffsets(); break;
		case S_LMARGIN: /* HBB 20010525: handle like 'show margin' */
		case S_RMARGIN:
		case S_TMARGIN:
		case S_BMARGIN:
		case S_MARGIN: ShowMargin(); break;
		case SET_OUTPUT:
		    show_output();
		    break;
		case S_OVERFLOW:
		    show_overflow();
		    break;
		case S_PARAMETRIC:
		    show_parametric();
		    break;
		case S_PM3D:
		    show_pm3d();
		    break;
		case S_PALETTE: ShowPalette(); break;
		case S_COLORBOX: ShowColorBox(); break;
		case S_COLORMAP:
		    save_colormaps(stderr);
		    Pgm.Shift();
		    break;
		case S_COLORNAMES:
		case S_COLORSEQUENCE:
		    Pgm.Rollback();
		    show_palette_colornames();
		    break;
		case S_POINTINTERVALBOX:
		    show_pointintervalbox();
		    break;
		case S_POINTSIZE:
		    show_pointsize();
		    break;
		case S_RGBMAX:
		    show_rgbmax();
		    break;
		case S_DECIMALSIGN:
		    show_decimalsign();
		    break;
		case S_ENCODING:
		    show_encoding();
		    break;
		case S_FIT: ShowFit(); break;
		case S_FONTPATH:
		    show_fontpath();
		    break;
		case S_POLAR:
		    show_polar();
		    break;
		case S_PRINT:
		    show_print();
		    break;
		case S_PSDIR:
		    show_psdir();
		    break;
		case S_OBJECT:
		    if(Pgm.AlmostEqualsCur("rect$angle"))
			    Pgm.Shift();
		    CHECK_TAG_GT_ZERO;
		    SaveObject(stderr, tag);
		    break;
		case S_WALL:
		    save_walls(stderr);
		    break;
		case S_ANGLES:
		    show_angles();
		    break;
		case S_SAMPLES: ShowSamples(); break;
		case S_PIXMAP: SavePixmaps(stderr); break;
		case S_ISOSAMPLES: ShowIsoSamples(); break;
		case S_ISOSURFACE: ShowIsoSurface(); break;
		case S_JITTER:
		    show_jitter();
		    break;
		case S_VIEW: ShowView(); break;
		case S_DATA:
		    error_message = "keyword 'data' deprecated, use 'show style data'";
		    break;
		case S_STYLE: ShowStyle(); break;
		case S_SURFACE:
		    show_surface();
		    break;
		case S_HIDDEN3D:
		    show_hidden3d();
		    break;
		case S_HISTORYSIZE:
		case S_HISTORY:
		    show_history();
		    break;
		case S_SIZE: ShowSize(); break;
		case S_ORIGIN:
		    show_origin();
		    break;
		case S_TERMINAL:
		    show_term();
		    break;
		case S_TICS:
		case S_TICSLEVEL:
		case S_TICSCALE:
		    ShowTics(TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);
		    break;
		case S_MXTICS:
		    show_mtics(&AxS[FIRST_X_AXIS]);
		    break;
		case S_MYTICS:
		    show_mtics(&AxS[FIRST_Y_AXIS]);
		    break;
		case S_MZTICS:
		    show_mtics(&AxS[FIRST_Z_AXIS]);
		    break;
		case S_MCBTICS:
		    show_mtics(&AxS[COLOR_AXIS]);
		    break;
		case S_MX2TICS:
		    show_mtics(&AxS[SECOND_X_AXIS]);
		    break;
		case S_MY2TICS:
		    show_mtics(&AxS[SECOND_Y_AXIS]);
		    break;
		case S_MRTICS:
		    show_mtics(&AxS.__R());
		    break;
		case S_MTTICS:
		    show_mtics(&AxS.Theta());
		    break;
		case S_XYPLANE:
		    if(xyplane.absolute)
			    fprintf(stderr, "\txyplane intercepts z axis at %g\n", xyplane.z);
		    else
			    fprintf(stderr, "\txyplane %g\n", xyplane.z);
		    break;
		case S_TIMESTAMP: ShowTimeStamp(); break;
		case S_RRANGE: ShowRange(POLAR_AXIS); break;
		case S_TRANGE: ShowRange(T_AXIS); break;
		case S_URANGE: ShowRange(U_AXIS); break;
		case S_VRANGE: ShowRange(V_AXIS); break;
		case S_XRANGE: ShowRange(FIRST_X_AXIS); break;
		case S_YRANGE: ShowRange(FIRST_Y_AXIS); break;
		case S_X2RANGE: ShowRange(SECOND_X_AXIS); break;
		case S_Y2RANGE: ShowRange(SECOND_Y_AXIS); break;
		case S_ZRANGE: ShowRange(FIRST_Z_AXIS); break;
		case S_CBRANGE: ShowRange(COLOR_AXIS); break;
		case S_TITLE: ShowTitle(); break;
		case S_XLABEL: ShowAxisLabel(FIRST_X_AXIS); break;
		case S_YLABEL: ShowAxisLabel(FIRST_Y_AXIS); break;
		case S_ZLABEL: ShowAxisLabel(FIRST_Z_AXIS); break;
		case S_CBLABEL: ShowAxisLabel(COLOR_AXIS); break;
		case S_RLABEL: ShowAxisLabel(POLAR_AXIS); break;
		case S_X2LABEL: ShowAxisLabel(SECOND_X_AXIS); break;
		case S_Y2LABEL: ShowAxisLabel(SECOND_Y_AXIS); break;
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
		case S_VGRID: ShowVGrid(); break;
		case S_ZERO:
		    show_zero();
		    break;
		case S_DATAFILE: ShowDataFile(); break;
		case S_TABLE: ShowTable(); break;
		case S_MOUSE:
		    show_mouse();
		    break;
		case S_PLOT:
		    show_plot();
#if defined(USE_READLINE)
		    if(!Pgm.EndOfCommand()) {
			    if(Pgm.AlmostEqualsCur("a$dd2history")) {
				    Pgm.Shift();
				    add_history(replot_line);
			    }
		    }
#endif
		    break;
		case S_VARIABLES: ShowVariables(); break;
/* FIXME: get rid of S_*DTICS, S_*MTICS cases */
		case S_XTICS:
		case S_XDTICS:
		case S_XMTICS: ShowTics(TRUE, FALSE, FALSE, TRUE, FALSE, FALSE); break;
		case S_YTICS:
		case S_YDTICS:
		case S_YMTICS: ShowTics(FALSE, TRUE, FALSE, FALSE, TRUE, FALSE); break;
		case S_ZTICS:
		case S_ZDTICS:
		case S_ZMTICS: ShowTics(FALSE, FALSE, TRUE, FALSE, FALSE, FALSE); break;
		case S_CBTICS:
		case S_CBDTICS:
		case S_CBMTICS: ShowTics(FALSE, FALSE, FALSE, FALSE, FALSE, TRUE); break;
		case S_RTICS: ShowTicdef(POLAR_AXIS); break;
		case S_TTICS: ShowTicDefp(&AxS.Theta()); break;
		case S_X2TICS:
		case S_X2DTICS:
		case S_X2MTICS: ShowTics(FALSE, FALSE, FALSE, TRUE, FALSE, FALSE); break;
		case S_Y2TICS:
		case S_Y2DTICS:
		case S_Y2MTICS: ShowTics(FALSE, FALSE, FALSE, FALSE, TRUE, FALSE); break;
		case S_MULTIPLOT:
		    fprintf(stderr, "multiplot mode is %s\n", multiplot ? "on" : "off");
		    break;
		case S_TERMOPTIONS:
		    fprintf(stderr, "Terminal options are '%s'\n", (*term_options) ? term_options : "[none]");
		    break;
		case S_THETA:
		    fprintf(stderr, "Theta increases %s with origin at %s of plot\n",
				theta_direction > 0 ? "counterclockwise" : "clockwise", theta_origin == 180 ? "left" : theta_origin ==  90 ? "top" : theta_origin == -90 ? "bottom" : "right");
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
		IntErrorCurToken(error_message);
	screen_ok = FALSE;
	putc('\n', stderr);
#undef CHECK_TAG_GT_ZERO
}
//
// process 'show actiontable|at' command
// not documented
//
//static void show_at()
void GnuPlot::ShowAt()
{
	putc('\n', stderr);
	disp_at(TempAt(), 0);
	Pgm.Shift();
}
//
// called by show_at(), and recursively by itself 
//
static void disp_at(struct at_type * curr_at, int level)
{
	int i, j;
	union argument * arg;
	for(i = 0; i < curr_at->a_count; i++) {
		putc('\t', stderr);
		for(j = 0; j < level; j++)
			putc(' ', stderr); /* indent */
		// print name of instruction 
		fputs(_FuncTab2[(int)(curr_at->actions[i].index)].P_Name, stderr);
		arg = &(curr_at->actions[i].arg);
		// now print optional argument 
		switch(curr_at->actions[i].index) {
			case PUSH:
			    fprintf(stderr, " %s\n", arg->udv_arg->udv_name);
			    break;
			case PUSHC:
			    putc(' ', stderr);
			    GPO.DispValue(stderr, &(arg->v_arg), TRUE);
			    putc('\n', stderr);
			    break;
			case PUSHD1:
			    fprintf(stderr, " %c dummy\n", arg->udf_arg->udf_name[0]);
			    break;
			case PUSHD2:
			    fprintf(stderr, " %c dummy\n", arg->udf_arg->udf_name[1]);
			    break;
			case CALL:
			    fprintf(stderr, " %s", arg->udf_arg->udf_name);
			    if(level < 6) {
				    if(arg->udf_arg->at) {
					    putc('\n', stderr);
					    disp_at(arg->udf_arg->at, level + 2); /* recurse! */
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
					    disp_at(arg->udf_arg->at, level + 2); /* recurse! */
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
			    fprintf(stderr, " %d\n", (int)(arg->v_arg.v.int_val));
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
void GnuPlot::ShowAll()
{
	var_show_all = 1;
	show_version(stderr);
	show_autoscale();
	SaveBars(stderr);
	show_border();
	ShowBoxWidth();
	ShowClip();
	show_contour();
	show_dgrid3d();
	show_mapping();
	show_dummy();
	ShowFormat();
	ShowStyle();
	ShowGrid();
	show_raxis();
	ShowZeroAxis(FIRST_X_AXIS);
	ShowZeroAxis(FIRST_Y_AXIS);
	ShowZeroAxis(FIRST_Z_AXIS);
	ShowLabel(0);
	ShowArrow(0);
	ShowKey();
	ShowLogScale();
	ShowOffsets();
	ShowMargin();
	show_micro();
	show_minus_sign();
	show_output();
	show_print();
	show_parametric();
	ShowPalette();
	ShowColorBox();
	show_pm3d();
	show_pointsize();
	show_pointintervalbox();
	show_rgbmax();
	show_encoding();
	show_decimalsign();
	ShowFit();
	show_polar();
	show_angles();
	SaveObject(stderr, 0);
	ShowSamples();
	ShowIsoSamples();
	ShowView();
	show_surface();
	show_hidden3d();
	show_history();
	ShowSize();
	show_origin();
	show_term();
	ShowTics(TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);
	show_mtics(&AxS[FIRST_X_AXIS]);
	show_mtics(&AxS[FIRST_Y_AXIS]);
	show_mtics(&AxS[FIRST_Z_AXIS]);
	show_mtics(&AxS[SECOND_X_AXIS]);
	show_mtics(&AxS[SECOND_Y_AXIS]);
	ShowXyzLabel("", "time", &Gg.LblTime);
	if(Gg.Parametric || Gg.Polar) {
		if(!Gg.Is3DPlot)
			ShowRange(T_AXIS);
		else {
			ShowRange(U_AXIS);
			ShowRange(V_AXIS);
		}
	}
	ShowRange(FIRST_X_AXIS);
	ShowRange(FIRST_Y_AXIS);
	ShowRange(SECOND_X_AXIS);
	ShowRange(SECOND_Y_AXIS);
	ShowRange(FIRST_Z_AXIS);
	show_jitter();
	ShowTitle();
	ShowAxisLabel(FIRST_X_AXIS);
	ShowAxisLabel(FIRST_Y_AXIS);
	ShowAxisLabel(FIRST_Z_AXIS);
	ShowAxisLabel(SECOND_X_AXIS);
	ShowAxisLabel(SECOND_Y_AXIS);
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
	ShowDataFile();
#ifdef USE_MOUSE
	show_mouse();
#endif
	show_plot();
	ShowVariables();
	show_functions();
	var_show_all = 0;
}

/* process 'show version' command */
void show_version(FILE * fp)
{
	/* If printed to a file, we prefix everything with
	 * a hash mark to comment out the version information.
	 */
	char prefix[6];         /* "#    " */
	char * p = prefix;
	char fmt[2048];

	prefix[0] = '#';
	prefix[1] = prefix[2] = prefix[3] = prefix[4] = ' ';
	prefix[5] = NUL;

	/* Construct string of configuration options used to build */
	/* this particular copy of gnuplot. Executed once only.    */
	if(!compile_options) {
		compile_options = (char *)gp_alloc(1024, "compile_options");
		{
			/* The following code could be a lot simpler if
			 * it wasn't for Borland's broken compiler ...
			 */
			const char * rdline =
#ifdef READLINE
			    "+"
#else
			    "-"
#endif
			    "READLINE  ";

			const char * gnu_rdline =
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
			    "HISTORY  "
			    "";

			const char * libcerf =
#ifdef HAVE_LIBCERF
			    "+LIBCERF  ";
#else
			    "";
#endif

			const char * libamos =
#ifdef HAVE_AMOS
			    "+AMOS  ";
#else
			    "";
#endif

			const char * have_cexint =
#ifdef HAVE_CEXINT
			    "+CEXINT  ";
#else
			    "";
#endif

			const char * complexfunc =
#ifdef HAVE_COMPLEX_FUNCS
			    "+COMPLEX_FUNCS  ";
#else
			    "";
#endif

			const char * libgd =
#ifdef HAVE_LIBGD
#ifdef HAVE_GD_PNG
			    "+GD_PNG  "
#endif
#ifdef HAVE_GD_JPEG
			    "+GD_JPEG  "
#endif
#ifdef HAVE_GD_TTF
			    "+GD_TTF  "
#endif
#ifdef HAVE_GD_GIF
			    "+GD_GIF  "
#endif
#ifdef GIF_ANIMATION
			    "+ANIMATION  "
#endif
#else
			    "-LIBGD  "
#endif
			    "";

			const char * nocwdrc =
#ifdef USE_CWDRC
			    "+"
#else
			    "-"
#endif
			    "USE_CWDRC  ";

			const char * x11 =
#ifdef X11
			    "+X11  "
#ifdef EXTERNAL_X11_WINDOW
			    "+X11_EXTERNAL "
#endif
#endif
			    "";

			const char * use_mouse =
#ifdef USE_MOUSE
			    "+USE_MOUSE  "
#endif
			    "";

			const char * hiddenline =
			    "+HIDDEN3D_QUADTREE  "
			    "";

			const char * plotoptions =
			    "+OBJECTS  "
#ifdef USE_STATS
			    "+STATS "
#else
			    "-STATS "
#endif
#ifdef HAVE_EXTERNAL_FUNCTIONS
			    "+EXTERNAL_FUNCTIONS "
#endif
			    "";

			const char * unicodebuild =
#if defined(_WIN32) && defined(UNICODE)
			    "+UNICODE  ";
#else
			    "";
#endif

			sprintf(compile_options, "    %s%s\n    %s%s\n    %s%s%s%s\n    %s\n    %s%s%s%s\n",
			    rdline, gnu_rdline, unicodebuild, plotoptions,
			    complexfunc, libcerf, libamos, have_cexint,
			    libgd,
			    nocwdrc, x11, use_mouse, hiddenline
			    );
		}
		compile_options = (char *)gp_realloc(compile_options, strlen(compile_options)+1, "compile_options");
	}
	/* The only effect of fp == NULL is to load the compile_options string */
	if(fp == NULL)
		return;
	if(fp == stderr) {
		/* No hash mark - let p point to the trailing '\0' */
		p += sizeof(prefix) - 1;
	}
	else {
#ifdef BINDIR
#ifdef X11
		fprintf(fp, "#!%s/gnuplot -persist\n#\n", BINDIR);
#else
		fprintf(fp, "#!%s/gnuplot\n#\n", BINDIR);
#endif                          /* not X11 */
#endif /* BINDIR */
	}

	strcpy(fmt,
	    "\
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
	strcat(fmt, "\
%s\tfaq, bugs, etc:   type \"help FAQ\"\n\
%s\timmediate help:   type \"help\"  (plot window: hit 'h')\n\
");

	fprintf(fp, fmt,
	    p,                  /* empty line */
	    p, PROGRAM,
	    p, gnuplot_version, gnuplot_patchlevel, gnuplot_date,
	    p,                  /* empty line */
	    p, gnuplot_copyright,
	    p,                  /* authors */
	    p,                  /* empty line */
	    p,                  /* website */
#ifdef DEVELOPMENT_VERSION
	    p, help_email,      /* mailing list */
#endif
	    p,                  /* type "help" */
	    p                   /* type "help seeking-assistance" */
	    );

	/* show version long */
	if(GPO.Pgm.AlmostEqualsCur("l$ong")) {
		GPO.Pgm.Shift();
		fprintf(stderr, "\nCompile options:\n%s", compile_options);
		fprintf(stderr, "    %d-bit integer arithmetic\n", (int)sizeof(intgr_t)*8);

#ifdef X11
		{
			char * driverdir = getenv("GNUPLOT_DRIVER_DIR");
			SETIFZ(driverdir, X11_DRIVER_DIR);
			fprintf(stderr, "GNUPLOT_DRIVER_DIR = \"%s\"\n", driverdir);
		}
#endif

		{
			char * psdir = getenv("GNUPLOT_PS_DIR");
#ifdef GNUPLOT_PS_DIR
			if(psdir == NULL)
				psdir = GNUPLOT_PS_DIR;
#endif
			if(psdir != NULL)
				fprintf(stderr, "GNUPLOT_PS_DIR     = \"%s\"\n", psdir);
		}
		{
#ifndef _WIN32
			char * helpfile = NULL;
			if((helpfile = getenv("GNUHELP")) == NULL)
				helpfile = HELPFILE;
			fprintf(stderr, "HELPFILE           = \"%s\"\n", helpfile);
#else /* _WIN32 */
			fprintf(stderr, "HELPFILE           = \"" TCHARFMT "\"\n", winhelpname);
#endif
		}
#if defined(_WIN32) && !defined(WGP_CONSOLE)
		fprintf(stderr, "MENUNAME           = \"" TCHARFMT "\"\n", szMenuName);
#endif
#ifdef HAVE_LIBCACA
		fprintf(stderr, "libcaca version    : %s\n", caca_get_version());
#endif
	} /* show version long */
}
//
// process 'show autoscale' command 
//
static void show_autoscale()
{
	SHOW_ALL_NL;

#define SHOW_AUTOSCALE(axis) {                                                \
		const t_autoscale ascale = GPO.AxS[axis].set_autoscale; \
		fprintf(stderr, "\t%s: %s%s%s%s%s, ", axis_name(axis), \
		    (ascale & AUTOSCALE_BOTH) ? "ON" : "OFF", ((ascale & AUTOSCALE_BOTH) == AUTOSCALE_MIN) ? " (min)" : "", \
		    ((ascale & AUTOSCALE_BOTH) == AUTOSCALE_MAX) ? " (max)" : "", (ascale & AUTOSCALE_FIXMIN) ? " (fixmin)" : "", \
		    (ascale & AUTOSCALE_FIXMAX) ? " (fixmax)" : ""); }

	fputs("\tautoscaling is ", stderr);
	if(GPO.Gg.Parametric) {
		SHOW_AUTOSCALE(T_AXIS);
		SHOW_AUTOSCALE(U_AXIS);
		SHOW_AUTOSCALE(V_AXIS);
	}
	if(GPO.Gg.Polar) {
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

/* process 'show border' command */
static void show_border()
{
	SHOW_ALL_NL;
	if(!draw_border)
		fprintf(stderr, "\tborder is not drawn\n");
	else {
		fprintf(stderr, "\tborder %d (0x%X) is drawn in %s layer with\n\t ",
		    draw_border, draw_border, border_layer == LAYER_BEHIND ? "behind" : border_layer == LAYER_BACK ? "back" : "front");
		save_linetype(stderr, &border_lp, FALSE);
		fputc('\n', stderr);
	}
}
//
// process 'show boxwidth' command 
//
//static void show_boxwidth()
void GnuPlot::ShowBoxWidth()
{
	SHOW_ALL_NL;
	if(V.BoxWidth < 0.0)
		fputs("\tboxwidth is auto\n", stderr);
	else {
		fprintf(stderr, "\tboxwidth is %g %s\n", V.BoxWidth, (V.BoxWidthIsAbsolute) ? "absolute" : "relative");
	}
	fprintf(stderr, "\tboxdepth is %g\n", boxdepth);
}

/* process 'show boxplot' command */
static void show_boxplot()
{
	fprintf(stderr, "\tboxplot representation is %s\n", boxplot_opts.plotstyle == FINANCEBARS ? "finance bar" : "box and whisker");
	fprintf(stderr, "\tboxplot range extends from the ");
	if(boxplot_opts.limit_type == 1)
		fprintf(stderr, "  median to include %5.2f of the points\n", boxplot_opts.limit_value);
	else
		fprintf(stderr, "  box by %5.2f of the interquartile distance\n", boxplot_opts.limit_value);
	if(boxplot_opts.outliers)
		fprintf(stderr, "\toutliers will be drawn using point type %d\n", boxplot_opts.pointtype+1);
	else
		fprintf(stderr, "\toutliers will not be drawn\n");
	fprintf(stderr, "\tseparation between boxplots is %g\n", boxplot_opts.separation);
	fprintf(stderr, "\tfactor labels %s\n",
	    (boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X)    ? "will be put on the x axis"  :
	    (boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X2)   ? "will be put on the x2 axis" :
	    (boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_AUTO) ? "are automatic" : "are off");
	fprintf(stderr, "\tfactor labels will %s\n", boxplot_opts.sort_factors ? "be sorted alphabetically" : "appear in the order they were found");
}

/* process 'show fillstyle' command */
static void show_fillstyle()
{
	SHOW_ALL_NL;
	switch(default_fillstyle.fillstyle) {
		case FS_SOLID:
		case FS_TRANSPARENT_SOLID:
		    fprintf(stderr, "\tFill style uses %s solid colour with density %.3f", default_fillstyle.fillstyle == FS_SOLID ? "" : "transparent", default_fillstyle.filldensity/100.0);
		    break;
		case FS_PATTERN:
		case FS_TRANSPARENT_PATTERN:
		    fprintf(stderr, "\tFill style uses %s patterns starting at %d", default_fillstyle.fillstyle == FS_PATTERN ? "" : "transparent", default_fillstyle.fillpattern);
		    break;
		default:
		    fprintf(stderr, "\tFill style is empty");
	}
	if(default_fillstyle.border_color.type == TC_LT && default_fillstyle.border_color.lt == LT_NODRAW)
		fprintf(stderr, " with no border\n");
	else {
		fprintf(stderr, " with border ");
		save_pm3dcolor(stderr, &default_fillstyle.border_color);
		fprintf(stderr, "\n");
	}
}
//
// process 'show clip' command 
//
//static void show_clip()
void GnuPlot::ShowClip()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tpoint clip is %s\n", Gg.ClipPoints ? "ON" : "OFF");
	fprintf(stderr, "\t%s lines with one end out of range (clip one)\n", Gg.ClipLines1 ? "clipping" : "not drawing");
	fprintf(stderr, "\t%s lines with both ends out of range (clip two)\n", Gg.ClipLines2 ? "clipping" : "not drawing");
	fprintf(stderr, "\t%sclipping lines on polar plot at maximum radius\n", Gg.ClipRadial ? "" : "not ");
}
//
// process 'show cntrparam|cntrlabel|contour' commands 
//
static void show_contour()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tcontour for surfaces are %s", (draw_contour) ? "drawn" : "not drawn\n");
	if(draw_contour) {
		fprintf(stderr, " in %d levels on ", contour_levels);
		switch(draw_contour) {
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
			    fprintf(stderr, "\t\t%d incremental levels starting at %g, step %g, end %g\n",
					contour_levels, contour_levels_list[0], contour_levels_list[1], contour_levels_list[0] + (contour_levels - 1) * contour_levels_list[1]);
			    // contour-levels counts both ends 
			    break;
		}
		/* Show contour label options */
		fprintf(stderr, "\tcontour lines are drawn in %s linetypes\n", clabel_onecolor ? "the same" : "individual");
		fprintf(stderr, "\tformat for contour labels is '%s' font '%s'\n", contour_format, clabel_font ? clabel_font : "");
		fprintf(stderr, "\ton-plot labels placed at segment %d with interval %d\n", clabel_start, clabel_interval);
		if(contour_firstlinetype > 0)
			fprintf(stderr, "\tfirst contour linetype will be %d\n", contour_firstlinetype);
		else
			fprintf(stderr, "\tfirst contour linetype will be chosen automatically\n");
		fprintf(stderr, "\tcontour levels will be %ssorted\n", contour_sortlevels ? "" : "un");
	}
}
//
// process 'show dashtype' command (tag 0 means show all) 
//
//static void show_dashtype(int tag)
void GnuPlot::ShowDashType(int tag)
{
	bool showed = FALSE;
	for(custom_dashtype_def * this_dashtype = Gg.P_FirstCustomDashtype; this_dashtype; this_dashtype = this_dashtype->next) {
		if(tag == 0 || tag == this_dashtype->tag) {
			showed = TRUE;
			fprintf(stderr, "\tdashtype %d, ", this_dashtype->tag);
			save_dashtype(stderr, this_dashtype->d_type, &(this_dashtype->dashtype));
			fputc('\n', stderr);
		}
	}
	if(tag > 0 && !showed)
		IntErrorCurToken("dashtype not found");
}

/* process 'show dgrid3d' command */
static void show_dgrid3d()
{
	SHOW_ALL_NL;
	if(dgrid3d)
		if(dgrid3d_mode == DGRID3D_QNORM) {
			fprintf(stderr, "\tdata grid3d is enabled for mesh of size %dx%d, norm=%d\n", dgrid3d_row_fineness, dgrid3d_col_fineness, dgrid3d_norm_value);
		}
		else if(dgrid3d_mode == DGRID3D_SPLINES) {
			fprintf(stderr, "\tdata grid3d is enabled for mesh of size %dx%d, splines\n", dgrid3d_row_fineness, dgrid3d_col_fineness);
		}
		else {
			fprintf(stderr, "\tdata grid3d is enabled for mesh of size %dx%d, kernel=%s,\n\tscale factors x=%f, y=%f%s\n",
			    dgrid3d_row_fineness, dgrid3d_col_fineness, reverse_table_lookup(dgrid3d_mode_tbl, dgrid3d_mode),
			    dgrid3d_x_scale, dgrid3d_y_scale, dgrid3d_kdensity ? ", kdensity2d mode" : "");
		}
	else
		fputs("\tdata grid3d is disabled\n", stderr);
}

/* process 'show mapping' command */
static void show_mapping()
{
	SHOW_ALL_NL;
	fputs("\tmapping for 3-d data is ", stderr);
	switch(mapping3d) {
		case MAP3D_CARTESIAN: fputs("cartesian\n", stderr); break;
		case MAP3D_SPHERICAL: fputs("spherical\n", stderr); break;
		case MAP3D_CYLINDRICAL: fputs("cylindrical\n", stderr); break;
	}
}

/* process 'show dummy' command */
static void show_dummy()
{
	SHOW_ALL_NL;
	fputs("\tdummy variables are ", stderr);
	for(int i = 0; i < MAX_NUM_VAR; i++) {
		if(*set_dummy_var[i] == '\0') {
			fputs("\n", stderr);
			break;
		}
		else {
			fprintf(stderr, "%s ", set_dummy_var[i]);
		}
	}
}
//
// process 'show format' command 
//
//static void show_format()
void GnuPlot::ShowFormat()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\ttic format is:\n");
	AxS.SaveAxisFormat(stderr, FIRST_X_AXIS);
	AxS.SaveAxisFormat(stderr, FIRST_Y_AXIS);
	AxS.SaveAxisFormat(stderr, SECOND_X_AXIS);
	AxS.SaveAxisFormat(stderr, SECOND_Y_AXIS);
	AxS.SaveAxisFormat(stderr, FIRST_Z_AXIS);
	AxS.SaveAxisFormat(stderr, COLOR_AXIS);
	AxS.SaveAxisFormat(stderr, POLAR_AXIS);
}
//
// process 'show style' sommand 
//
//static void show_style()
void GnuPlot::ShowStyle()
{
	int tag = 0;
#define CHECK_TAG_GT_ZERO                                       \
	if(!Pgm.EndOfCommand()) {                                  \
		tag = static_cast<int>(RealExpression());     \
		if(tag <= 0)                                       \
			IntErrorCurToken("tag must be > zero");        \
	}
	switch(Pgm.LookupTableForCurrentToken(&show_style_tbl[0])) {
		case SHOW_STYLE_DATA:
		    SHOW_ALL_NL;
		    show_styles("Data", data_style);
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_FUNCTION:
		    SHOW_ALL_NL;
		    show_styles("Functions", func_style);
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_LINE:
		    Pgm.Shift();
		    CHECK_TAG_GT_ZERO;
		    show_linestyle(tag);
		    break;
		case SHOW_STYLE_FILLING:
		    show_fillstyle();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_INCREMENT:
		    show_increment();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_HISTOGRAM:
		    ShowHistogram();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_TEXTBOX:
		    show_textbox();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_PARALLEL:
		    save_style_parallel(stderr);
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_SPIDERPLOT:
		    save_style_spider(stderr);
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_ARROW:
		    Pgm.Shift();
		    CHECK_TAG_GT_ZERO;
		    ShowArrowStyle(tag);
		    break;
		case SHOW_STYLE_BOXPLOT:
		    show_boxplot();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_RECTANGLE:
		    show_style_rectangle();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_CIRCLE:
		    ShowStyleCircle();
		    Pgm.Shift();
		    break;
		case SHOW_STYLE_ELLIPSE:
		    ShowStyleEllipse();
		    Pgm.Shift();
		    break;
		default:
		    /* show all styles */
		    show_styles("Data", data_style);
		    show_styles("Functions", func_style);
		    show_linestyle(0);
		    show_fillstyle();
		    show_increment();
		    ShowHistogram();
		    show_textbox();
		    save_style_parallel(stderr);
		    ShowArrowStyle(0);
		    show_boxplot();
		    show_style_rectangle();
		    ShowStyleCircle();
		    ShowStyleEllipse();
		    break;
	}
#undef CHECK_TAG_GT_ZERO
}

/* called by show_style() - defined for aesthetic reasons */
static void show_style_rectangle()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tRectangle style is %s, fill color ", default_rectangle.layer > 0 ? "front" : default_rectangle.layer < 0 ? "behind" : "back");
	save_pm3dcolor(stderr, &default_rectangle.lp_properties.pm3d_color);
	fprintf(stderr, ", lw %.1f ", default_rectangle.lp_properties.l_width);
	fprintf(stderr, ", fillstyle");
	save_fillstyle(stderr, &default_rectangle.fillstyle);
}

//static void show_style_circle()
void GnuPlot::ShowStyleCircle()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tCircle style has default radius ");
	ShowPosition(&default_circle.o.circle.extent, 1);
	fprintf(stderr, " [%s]", default_circle.o.circle.wedge ? "wedge" : "nowedge");
	fputs("\n", stderr);
}

//static void show_style_ellipse()
void GnuPlot::ShowStyleEllipse()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tEllipse style has default size ");
	ShowPosition(&default_ellipse.o.ellipse.extent, 2);
	fprintf(stderr, ", default angle is %.1f degrees", default_ellipse.o.ellipse.orientation);
	switch(default_ellipse.o.ellipse.type) {
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

/* called by show_data() and show_func() */
static void show_styles(const char * name, enum PLOT_STYLE style)
{
	fprintf(stderr, "\t%s are plotted with ", name);
	save_data_func_style(stderr, name, style);
}
//
// called by show_func() 
//
static void show_functions()
{
	udft_entry * udf = GPO.Ev.P_FirstUdf;
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
void GnuPlot::ShowGrid()
{
	SHOW_ALL_NL;
	if(!some_grid_selected()) {
		fputs("\tgrid is OFF\n", stderr);
	}
	else {
		// HBB 20010806: new storage method for grid options: 
		fprintf(stderr, "\t%s grid drawn at", (polar_grid_angle != 0) ? "Polar" : "Rectangular");
	#define SHOW_GRID(axis)                                         \
		if(AxS[axis].gridmajor)                             \
			fprintf(stderr, " %s", axis_name(axis));        \
		if(AxS[axis].gridminor)                             \
			fprintf(stderr, " m%s", axis_name(axis));
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
		save_linetype(stderr, &(grid_lp), FALSE);
		fprintf(stderr, "\n\tMinor grid drawn with");
		save_linetype(stderr, &(mgrid_lp), FALSE);
		fputc('\n', stderr);
		if(grid_vertical_lines)
			fprintf(stderr, "\tVertical grid lines in 3D plots\n");
		if(polar_grid_angle)
			fprintf(stderr, "\tGrid radii drawn every %f %s\n", polar_grid_angle / ang2rad, (ang2rad == 1.0) ? "radians" : "degrees");
		if(grid_spiderweb)
			fprintf(stderr, "\tGrid shown in spiderplots\n");
		fprintf(stderr, "\tGrid drawn at %s\n", (grid_layer==-1) ? "default layer" : ((grid_layer==0) ? "back" : "front"));
	}
}

static void show_raxis()
{
	fprintf(stderr, "\traxis is %sdrawn\n", raxis ? "" : "not ");
}

//static void show_paxis()
void GnuPlot::ShowPAxis()
{
	GpAxis * paxis;
	const int p = IntExpression();
	if(p <= 0 || p > AxS.GetParallelAxisCount())
		IntErrorCurToken("no such parallel axis is active");
	paxis = &AxS.Parallel(p-1);
	fputs("\t", stderr);
	if(Pgm.EndOfCommand() || Pgm.EqualsCur("range"))
		save_prange(stderr, paxis);
	if(Pgm.EndOfCommand() || Pgm.AlmostEqualsCur("tic$s"))
		ShowTicDefp(paxis);
	if(Pgm.EndOfCommand() || Pgm.EqualsCur("label")) {
		fprintf(stderr, "\t");
		SaveAxisLabelOrTitle(stderr, axis_name((AXIS_INDEX)paxis->index), "label", &paxis->label, TRUE);
	}
	if(paxis->zeroaxis)
		save_linetype(stderr, paxis->zeroaxis, FALSE);
	Pgm.Shift();
}
//
// process 'show {x|y|z}zeroaxis' command 
//
//static void show_zeroaxis(AXIS_INDEX axis)
void GnuPlot::ShowZeroAxis(AXIS_INDEX axis)
{
	SHOW_ALL_NL;
	if(AxS[axis].zeroaxis) {
		fprintf(stderr, "\t%szeroaxis is drawn with", axis_name(axis));
		save_linetype(stderr, AxS[axis].zeroaxis, FALSE);
		fputc('\n', stderr);
	}
	else
		fprintf(stderr, "\t%szeroaxis is OFF\n", axis_name(axis));
}
//
// Show label number <tag> (0 means show all) 
//
//static void show_label(int tag)
void GnuPlot::ShowLabel(int tag)
{
	bool showed = FALSE;
	for(text_label * this_label = Gg.P_FirstLabel; this_label; this_label = this_label->next) {
		if(tag == 0 || tag == this_label->tag) {
			showed = TRUE;
			fprintf(stderr, "\tlabel %d \"%s\" at ", this_label->tag, (this_label->text==NULL) ? "" : conv_text(this_label->text));
			ShowPosition(&this_label->place, 3);
			if(this_label->hypertext)
				fprintf(stderr, " hypertext");
			switch(this_label->pos) {
				case LEFT: fputs(" left", stderr); break;
				case CENTRE: fputs(" centre", stderr); break;
				case RIGHT: fputs(" right", stderr); break;
			}
			if(this_label->rotate)
				fprintf(stderr, " rotated by %d degrees (if possible)", this_label->rotate);
			else
				fprintf(stderr, " not rotated");
			fprintf(stderr, " %s ", this_label->layer ? "front" : "back");
			if(this_label->font != NULL)
				fprintf(stderr, " font \"%s\"", this_label->font);
			if(this_label->textcolor.type)
				save_textcolor(stderr, &this_label->textcolor);
			if(this_label->noenhanced)
				fprintf(stderr, " noenhanced");
			if((this_label->lp_properties.flags & LP_SHOW_POINTS) == 0)
				fprintf(stderr, " nopoint");
			else {
				fprintf(stderr, " point with color of");
				save_linetype(stderr, &(this_label->lp_properties), TRUE);
				fprintf(stderr, " offset ");
				ShowPosition(&this_label->offset, 3);
			}

			if(this_label->boxed) {
				fprintf(stderr, " boxed");
				if(this_label->boxed > 0)
					fprintf(stderr, " bs %d", this_label->boxed);
			}
			/* Entry font added by DJL */
			fputc('\n', stderr);
		}
	}
	if(tag > 0 && !showed)
		IntErrorCurToken("label not found");
}
//
// Show arrow number <tag> (0 means show all) 
//
//static void show_arrow(int tag)
void GnuPlot::ShowArrow(int tag)
{
	bool showed = FALSE;
	for(arrow_def * this_arrow = Gg.P_FirstArrow; this_arrow != NULL; this_arrow = this_arrow->next) {
		if(tag == 0 || tag == this_arrow->tag) {
			showed = TRUE;
			fprintf(stderr, "\tarrow %d, %s %s %s", this_arrow->tag,
			    arrow_head_names[this_arrow->arrow_properties.head],
			    (this_arrow->arrow_properties.headfill==AS_FILLED) ? "filled" :
			    (this_arrow->arrow_properties.headfill==AS_EMPTY) ? "empty" :
			    (this_arrow->arrow_properties.headfill==AS_NOBORDER) ? "noborder" :
			    "nofilled",
			    this_arrow->arrow_properties.layer ? "front" : "back");
			save_linetype(stderr, &(this_arrow->arrow_properties.lp_properties), FALSE);
			fprintf(stderr, "\n\t  from ");
			ShowPosition(&this_arrow->start, 3);
			if(this_arrow->type == arrow_end_absolute) {
				fputs(" to ", stderr);
				ShowPosition(&this_arrow->end, 3);
			}
			else if(this_arrow->type == arrow_end_relative) {
				fputs(" rto ", stderr);
				ShowPosition(&this_arrow->end, 3);
			}
			else { /* arrow_end_oriented */
				fputs(" length ", stderr);
				ShowPosition(&this_arrow->end, 1);
				fprintf(stderr, " angle %g deg", this_arrow->angle);
			}
			if(this_arrow->arrow_properties.head_length > 0) {
				static char * msg[] = {"(first x axis) ", "(second x axis) ", "(graph units) ", "(screen units) "};
				fprintf(stderr, "\n\t  arrow head: length %s%g, angle %g deg",
				    this_arrow->arrow_properties.head_lengthunit == first_axes ? "" : msg[this_arrow->arrow_properties.head_lengthunit],
				    this_arrow->arrow_properties.head_length, this_arrow->arrow_properties.head_angle);
				if(this_arrow->arrow_properties.headfill != AS_NOFILL)
					fprintf(stderr, ", backangle %g deg", this_arrow->arrow_properties.head_backangle);
			}
			putc('\n', stderr);
		}
	}
	if(tag > 0 && !showed)
		IntErrorCurToken("arrow not found");
}
//
// process 'show keytitle' command 
//
static void show_keytitle()
{
	legend_key * key = &GPO.Gg.KeyT;
	SHOW_ALL_NL;
	fprintf(stderr, "\tkey title is \"%s\"\n", conv_text(key->title.text));
	if(key->title.font && *(key->title.font))
		fprintf(stderr, "\t  font \"%s\"\n", key->title.font);
}
//
// process 'show key' command 
//
//static void show_key()
void GnuPlot::ShowKey()
{
	legend_key * key = &Gg.KeyT;
	SHOW_ALL_NL;
	if(!(key->visible)) {
		fputs("\tkey is OFF\n", stderr);
		if(key->auto_titles == COLUMNHEAD_KEYTITLES)
			fputs("\ttreatment of first record as column headers remains in effect\n", stderr);
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
			    fputs(key->fixed ? " fixed" : " inside", stderr);
		    else if(key->region == GPKEY_AUTO_EXTERIOR_LRTBC)
			    fputs(" outside", stderr);
		    else {
			    switch(key->margin) {
				    case GPKEY_TMARGIN: fputs(" tmargin", stderr); break;
				    case GPKEY_BMARGIN: fputs(" bmargin", stderr); break;
				    case GPKEY_LMARGIN: fputs(" lmargin", stderr); break;
				    case GPKEY_RMARGIN: fputs(" rmargin", stderr); break;
			    }
		    }
		    fputs("\n", stderr);
		    break;
	    }
		case GPKEY_USER_PLACEMENT:
		    fprintf(stderr, "\t%s %s of ",
			key->vpos == JUST_BOT ? "bottom" : key->vpos == JUST_CENTRE ? "center" : "top",
			key->hpos == RIGHT ? "right" : key->hpos == LEFT ? "left" : "center");
		    fputs("key is at ", stderr);
		    ShowPosition(&key->user_pos, 2);
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
		save_linetype(stderr, &(key->box), FALSE);
		fputc('\n', stderr);
	}
	else
		fprintf(stderr, "not boxed\n");

	if(key->front) {
		fprintf(stderr, "\tkey box is opaque");
		if(key->fillcolor.lt != LT_BACKGROUND)
			save_pm3dcolor(stderr, &key->fillcolor);
		fprintf(stderr, " \n");
	}

	fprintf(stderr,
	    "\
\tsample length is %g characters\n\
\tvertical spacing is %g characters\n\
\twidth adjustment is %g characters\n\
\theight adjustment is %g characters\n\
\tcurves are%s automatically titled %s\n",
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

//void show_position(const GpPosition * pPos, int ndim)
void GnuPlot::ShowPosition(const GpPosition * pPos, int ndim)
{
	fprintf(stderr, "(");
	SavePosition(stderr, pPos, ndim, FALSE);
	fprintf(stderr, ")");
}

/* helper function for "show log" */
static int show_log(GpAxis * axis)
{
	if(axis->log) {
		fprintf(stderr, " %s", axis_name((AXIS_INDEX)axis->index));
		if(axis->base != 10.)
			fprintf(stderr, " (base %g)", axis->base);
		return 1;
	}
	return 0;
}
//
// process 'show logscale' command 
//
//static void show_logscale()
void GnuPlot::ShowLogScale()
{
	int count = 0;
	SHOW_ALL_NL;
	fprintf(stderr, "\tlogscaling on ");
	count += show_log(&AxS[FIRST_X_AXIS]);
	count += show_log(&AxS[FIRST_Y_AXIS]);
	count += show_log(&AxS[FIRST_Z_AXIS]);
	count += show_log(&AxS[SECOND_X_AXIS]);
	count += show_log(&AxS[SECOND_Y_AXIS]);
	count += show_log(&AxS[COLOR_AXIS]);
	count += show_log(&AxS[POLAR_AXIS]);
	fputs(count ? "\n" : "none\n", stderr);
}
//
// process 'show offsets' command 
//
//static void show_offsets()
void GnuPlot::ShowOffsets()
{
	SHOW_ALL_NL;
	SaveOffsets(stderr, "\toffsets are");
}
//
// process 'show margin' command 
//
//static void show_margin()
void GnuPlot::ShowMargin()
{
	SHOW_ALL_NL;
	if(V.MarginL.scalex == screen)
		fprintf(stderr, "\tlmargin is set to screen %g\n", V.MarginL.x);
	else if(V.MarginL.x >= 0)
		fprintf(stderr, "\tlmargin is set to %g\n", V.MarginL.x);
	else
		fputs("\tlmargin is computed automatically\n", stderr);
	if(V.MarginR.scalex == screen)
		fprintf(stderr, "\trmargin is set to screen %g\n", V.MarginR.x);
	else if(V.MarginR.x >= 0)
		fprintf(stderr, "\trmargin is set to %g\n", V.MarginR.x);
	else
		fputs("\trmargin is computed automatically\n", stderr);
	if(V.MarginB.scalex == screen)
		fprintf(stderr, "\tbmargin is set to screen %g\n", V.MarginB.x);
	else if(V.MarginB.x >= 0)
		fprintf(stderr, "\tbmargin is set to %g\n", V.MarginB.x);
	else
		fputs("\tbmargin is computed automatically\n", stderr);
	if(V.MarginT.scalex == screen)
		fprintf(stderr, "\ttmargin is set to screen %g\n", V.MarginT.x);
	else if(V.MarginT.x >= 0)
		fprintf(stderr, "\ttmargin is set to %g\n", V.MarginT.x);
	else
		fputs("\ttmargin is computed automatically\n", stderr);
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

/* process 'show print' command */
static void show_print()
{
	SHOW_ALL_NL;
	if(print_out_var == NULL)
		fprintf(stderr, "\tprint output is sent to '%s'\n", print_show_output());
	else
		fprintf(stderr, "\tprint output is saved to datablock %s\n", print_show_output());
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
//
// process 'show overflow' command 
//
static void show_overflow()
{
	fprintf(stderr, "\t64-bit integer overflow %s\n", GPO.Ev.OverflowHandling == INT64_OVERFLOW_UNDEFINED ? "is treated as an undefined value" :
	    GPO.Ev.OverflowHandling == INT64_OVERFLOW_NAN ? "is treated as NaN (not a number)" :
	    GPO.Ev.OverflowHandling == INT64_OVERFLOW_TO_FLOAT ? "becomes a floating point value" : "is ignored");
}
//
// process 'show parametric' command 
//
static void show_parametric()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tparametric is %s\n", (GPO.Gg.Parametric ? "ON" : "OFF"));
}

//static void show_palette_rgbformulae()
void GnuPlot::ShowPalette_RgbFormulae()
{
	fprintf(stderr, "\t  * there are %i available rgb color mapping formulae:", SmPltt.colorFormulae);
	// print the description of the color formulae 
	int i = 0;
	while(*(ps_math_color_formulae[2*i]) ) {
		if(i % 3 == 0)
			fputs("\n\t    ", stderr);
		fprintf(stderr, "%2i: %-15s", i, ps_math_color_formulae[2*i+1]);
		i++;
	}
	fputs("\n", stderr);
	fputs("\t  * negative numbers mean inverted=negative colour component\n", stderr);
	fprintf(stderr, "\t  * thus the ranges in `set pm3d rgbformulae' are -%i..%i\n", SmPltt.colorFormulae-1, SmPltt.colorFormulae-1);
	Pgm.Shift();
}

//static void show_palette_fit2rgbformulae()
void GnuPlot::ShowPalette_Fit2RgbFormulae()
{
#define rgb_distance(r, g, b) ((r)*(r) + (g)*(g) + (b)*(b))
	int pts = 32; /* resolution: nb of points in the discrete raster for comparisons */
	int i, p, ir, ig, ib;
	int rMin = 0, gMin = 0, bMin = 0;
	int maxFormula = SmPltt.colorFormulae - 1; /* max formula number */
	double gray, dist, distMin;
	rgb_color * currRGB;
	int * formulaeSeq;
	double ** formulae;
	Pgm.Shift();
	if(SmPltt.colorMode == SMPAL_COLOR_MODE_RGB && SmPltt.CModel == C_MODEL_RGB) {
		fprintf(stderr, "\tCurrent palette is\n\t    set palette rgbformulae %i,%i,%i\n", SmPltt.formulaR, SmPltt.formulaG, SmPltt.formulaB);
		return;
	}
	// allocate and fill R, G, B values rastered on pts points 
	currRGB = (rgb_color*)gp_alloc(pts * sizeof(rgb_color), "RGB pts");
	for(p = 0; p < pts; p++) {
		gray = (double)p / (pts - 1);
		Rgb1FromGray(gray, &(currRGB[p]));
	}
	// organize sequence of rgb formulae 
	formulaeSeq = (int *)gp_alloc((2*maxFormula+1) * sizeof(int), "formulaeSeq");
	for(i = 0; i <= maxFormula; i++)
		formulaeSeq[i] = i;
	for(i = 1; i <= maxFormula; i++)
		formulaeSeq[maxFormula+i] = -i;
	// allocate and fill all +-formulae on the interval of given number of points 
	formulae = (double **)gp_alloc((2*maxFormula+1) * sizeof(double*), "formulae");
	for(i = 0; i < 2*maxFormula+1; i++) {
		formulae[i] = (double *)gp_alloc(pts * sizeof(double), "formulae pts");
		for(p = 0; p < pts; p++) {
			double gray = (double)p / (pts - 1);
			formulae[i][p] = GetColorValueFromFormula(formulaeSeq[i], gray);
		}
	}
	/* Now go over all rastered formulae, compare them to the current one, and
	   find the minimal distance.
	 */
	distMin = VERYLARGE;
	for(ir = 0; ir <    2*maxFormula+1; ir++) {
		for(ig = 0; ig < 2*maxFormula+1; ig++) {
			for(ib = 0; ib < 2*maxFormula+1; ib++) {
				dist = 0; /* calculate distance of the two rgb profiles */
				for(p = 0; p < pts; p++) {
					double tmp = rgb_distance(
						currRGB[p].r - formulae[ir][p],
						currRGB[p].g - formulae[ig][p],
						currRGB[p].b - formulae[ib][p]);
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
	fprintf(stderr, "\tThe best match of the current palette corresponds to\n\t    set palette rgbformulae %i,%i,%i\n", rMin, gMin,
	    bMin);
#undef rgb_distance
	for(i = 0; i < 2*maxFormula+1; i++)
		SAlloc::F(formulae[i]);
	SAlloc::F(formulae);
	SAlloc::F(formulaeSeq);
	SAlloc::F(currRGB);
}

//static void show_palette_palette()
void GnuPlot::ShowPalette_Palette()
{
	int i;
	int colors = 128;
	double gray;
	rgb_color rgb1;
	rgb255_color rgb255;
	FILE * f;
	/* How to format the table:
	    0: 1. gray=0.1111, (r,g,b)=(0.3333,0.0014,0.6428), #5500a4 =  85   0 164
	    1: 0.3333  0.0014  0.6428
	    2: 85      0       164
	    3: 0x5500a4
	 */
	int format = 0;
	Pgm.Shift();
	while(!Pgm.EndOfCommand()) {
		if(Pgm.EqualsCur("float")) {
			format = 1;
			Pgm.Shift();
		}
		else if(Pgm.EqualsCur("int")) {
			format = 2;
			Pgm.Shift();
		}
		else if(Pgm.EqualsCur("hex")) {
			format = 3;
			Pgm.Shift();
		}
		else {
			colors = IntExpression();
			if(colors < 2)
				colors = 128;
		}
	}
	f = (print_out) ? print_out : stderr;
	fprintf(stderr, "%s palette with %i discrete colors", (SmPltt.colorMode == SMPAL_COLOR_MODE_GRAY) ? "Gray" : "Color", colors);
	if(print_out_name)
		fprintf(stderr, " saved to \"%s\".", print_out_name);
	for(i = 0; i < colors; i++) {
		char line[80];
		// colours equidistantly from [0,1]  
		gray = (double)i / (colors - 1);
		if(SmPltt.Positive == SMPAL_NEGATIVE)
			gray = 1 - gray;
		Rgb1FromGray(gray, &rgb1);
		rgb255_from_rgb1(rgb1, &rgb255);
		switch(format) {
			case 1:
			    sprintf(line, "%0.4f\t%0.4f\t%0.4f", rgb1.r, rgb1.g, rgb1.b);
			    break;
			case 2:
			    sprintf(line, "%i\t%i\t%i", (int)rgb255.r, (int)rgb255.g, (int)rgb255.b);
			    break;
			case 3:
			    sprintf(line, "0x%06x", (int)rgb255.r<<16 | (int)rgb255.g<<8 | (int)rgb255.b);
			    break;
			default:
			    sprintf(line, "%3i. gray=%0.4f, (r,g,b)=(%0.4f,%0.4f,%0.4f), #%02x%02x%02x = %3i %3i %3i",
					i, gray, rgb1.r, rgb1.g, rgb1.b, (int)rgb255.r, (int)rgb255.g, (int)rgb255.b, (int)rgb255.r, (int)rgb255.g, (int)rgb255.b);
			    break;
		}
		if(print_out_var)
			append_to_datablock(&print_out_var->udv_value, sstrdup(line) );
		else
			fprintf(f, "%s\n", line);
	}
}

//static void show_palette_gradient()
void GnuPlot::ShowPalette_Gradient()
{
	double gray, r, g, b;
	Pgm.Shift();
	if(SmPltt.colorMode != SMPAL_COLOR_MODE_GRADIENT) {
		fputs("\tcolor mapping *not* done by defined gradient.\n", stderr);
	}
	else {
		for(int i = 0; i < SmPltt.GradientNum; i++) {
			gray = SmPltt.P_Gradient[i].pos;
			r = SmPltt.P_Gradient[i].col.r;
			g = SmPltt.P_Gradient[i].col.g;
			b = SmPltt.P_Gradient[i].col.b;
			fprintf(stderr, "%3i. gray=%0.4f, (r,g,b)=(%0.4f,%0.4f,%0.4f), #%02x%02x%02x = %3i %3i %3i\n",
				i, gray, r, g, b, (int)(255*r+.5), (int)(255*g+.5), (int)(255*b+.5), (int)(255*r+.5), (int)(255*g+.5), (int)(255*b+.5) );
		}
	}
}
//
// Helper function for show_palette_colornames() 
//
static void show_colornames(const struct gen_table * tbl)
{
	int i = 0;
	while(tbl->key) {
		// Print color names and their rgb values, table with 1 column 
		int r = ((tbl->value >> 16 ) & 255);
		int g = ((tbl->value >> 8 ) & 255);
		int b = (tbl->value & 255);
		fprintf(stderr, "\n  %-18s ", tbl->key);
		fprintf(stderr, "#%02x%02x%02x = %3i %3i %3i", r, g, b, r, g, b);
		++tbl;
		++i;
	}
	fputs("\n", stderr);
	GPO.Pgm.Shift();
}

static void show_palette_colornames()
{
	fprintf(stderr, "\tThere are %d predefined color names:", num_predefined_colors);
	show_colornames(pm3d_color_names_tbl);
}

//static void show_palette()
void GnuPlot::ShowPalette()
{
	// no option given, i.e. "show palette" 
	if(Pgm.EndOfCommand()) {
		fprintf(stderr, "\tpalette is %s\n", SmPltt.colorMode == SMPAL_COLOR_MODE_GRAY ? "GRAY" : "COLOR");
		switch(SmPltt.colorMode) {
			default:
			case SMPAL_COLOR_MODE_GRAY: break;
			case SMPAL_COLOR_MODE_RGB:
			    fprintf(stderr, "\trgb color mapping by rgbformulae are %i,%i,%i\n", SmPltt.formulaR, SmPltt.formulaG, SmPltt.formulaB);
			    break;
			case SMPAL_COLOR_MODE_GRADIENT:
			    fputs("\tcolor mapping by defined gradient\n", stderr);
			    break;
			case SMPAL_COLOR_MODE_FUNCTIONS:
			    fputs("\tcolor mapping is done by user defined functions\n", stderr);
			    if(SmPltt.Afunc.at && SmPltt.Afunc.definition)
				    fprintf(stderr, "\t  A-formula: %s\n", SmPltt.Afunc.definition);
			    if(SmPltt.Bfunc.at && SmPltt.Bfunc.definition)
				    fprintf(stderr, "\t  B-formula: %s\n", SmPltt.Bfunc.definition);
			    if(SmPltt.Cfunc.at && SmPltt.Cfunc.definition)
				    fprintf(stderr, "\t  C-formula: %s\n", SmPltt.Cfunc.definition);
			    break;
			case SMPAL_COLOR_MODE_CUBEHELIX:
			    fprintf(stderr, "\tCubehelix color palette: start %g cycles %g saturation %g\n", SmPltt.cubehelix_start, SmPltt.cubehelix_cycles, SmPltt.cubehelix_saturation);
			    break;
		}
		fprintf(stderr, "\tfigure is %s\n", (SmPltt.Positive == SMPAL_POSITIVE) ? "POSITIVE" : "NEGATIVE");
		fprintf(stderr, "\tall color formulae ARE%s written into output postscript file\n", !SmPltt.ps_allcF ? " NOT" : "");
		fputs("\tallocating ", stderr);
		if(SmPltt.UseMaxColors)
			fprintf(stderr, "MAX %i", SmPltt.UseMaxColors);
		else
			fputs("ALL remaining", stderr);
		fputs(" color positions for discrete palette terminals\n", stderr);
		fputs("\tColor-Model: ", stderr);
		switch(SmPltt.CModel) {
			default:
			case C_MODEL_RGB: fputs("RGB\n", stderr); break;
			case C_MODEL_CMY: fputs("CMY\n", stderr); break;
			case C_MODEL_HSV:
			    if(SmPltt.HSV_offset != 0)
				    fprintf(stderr, "HSV start %.2f\n", SmPltt.HSV_offset);
			    else
				    fputs("HSV\n", stderr);
			    break;
		}
		fprintf(stderr, "\tgamma is %.4g\n", SmPltt.gamma);
		return;
	}
	if(Pgm.AlmostEqualsCur("pal$ette")) {
		// 'show palette palette <n>' 
		ShowPalette_Palette();
		return;
	}
	else if(Pgm.AlmostEqualsCur("gra$dient")) {
		// 'show palette gradient' 
		ShowPalette_Gradient();
		return;
	}
	else if(Pgm.AlmostEqualsCur("rgbfor$mulae")) {
		// 'show palette rgbformulae' 
		ShowPalette_RgbFormulae();
		return;
	}
	else if(Pgm.EqualsCur("colors") || Pgm.AlmostEqualsCur("color$names")) {
		// 'show palette colornames' 
		show_palette_colornames();
		return;
	}
	else if(Pgm.AlmostEqualsCur("fit2rgb$formulae")) {
		// 'show palette fit2rgbformulae' 
		ShowPalette_Fit2RgbFormulae();
		return;
	}
	else { // wrong option to "show palette" 
		IntErrorCurToken("Expecting 'gradient' or 'palette <n>' or 'rgbformulae' or 'colornames'");
	}
}

//static void show_colorbox()
void GnuPlot::ShowColorBox()
{
	Pgm.Shift();
	if(!Gg.ColorBox.border) {
		fprintf(stderr, "\tcolor box without border");
	}
	else {
		fprintf(stderr, "\tcolor box with border lt");
		if(Gg.ColorBox.border_lt_tag > 0)
			fprintf(stderr, " %d", Gg.ColorBox.border_lt_tag);
		else
			fprintf(stderr, " default");
		fprintf(stderr, " cbtics lt");
		if(Gg.ColorBox.cbtics_lt_tag > 0)
			fprintf(stderr, " %d ", Gg.ColorBox.cbtics_lt_tag);
		else
			fprintf(stderr, " same ");
	}
	if(Gg.ColorBox.where != SMCOLOR_BOX_NO) {
		if(Gg.ColorBox.layer == LAYER_FRONT) 
			fputs("drawn front\n\t", stderr);
		else 
			fputs("drawn back\n\t", stderr);
	}
	switch(Gg.ColorBox.where) {
		case SMCOLOR_BOX_NO:
		    fputs("NOT drawn\n", stderr);
		    break;
		case SMCOLOR_BOX_DEFAULT:
		    fputs("at DEFAULT position\n", stderr);
		    break;
		case SMCOLOR_BOX_USER:
		    fputs("at USER origin: ", stderr);
		    ShowPosition(&Gg.ColorBox.origin, 2);
		    fputs("\n\t          size: ", stderr);
		    ShowPosition(&Gg.ColorBox.size, 2);
		    fputs("\n", stderr);
		    break;
		default: /* should *never* happen */
		    IntError(NO_CARET, "Argh!");
	}
	if(Gg.ColorBox.rotation == 'v')
		fprintf(stderr, "\tcolor gradient is vertical %s\n", Gg.ColorBox.invert ? " (inverted)" : "");
	else
		fprintf(stderr, "\tcolor gradient is horizontal\n");
}

static void show_pm3d()
{
	GPO.Pgm.Shift();
	fprintf(stderr, "\tpm3d style is %s\n", PM3D_IMPLICIT == pm3d.implicit ? "implicit (pm3d draw for all surfaces)" : "explicit (draw pm3d surface according to style)");
	fputs("\tpm3d plotted at ", stderr);
	{ 
		for(int i = 0; pm3d.where[i]; i++) {
			if(i > 0) 
				fputs(", then ", stderr);
			switch(pm3d.where[i]) {
				case PM3D_AT_BASE: fputs("BOTTOM", stderr); break;
				case PM3D_AT_SURFACE: fputs("SURFACE", stderr); break;
				case PM3D_AT_TOP: fputs("TOP", stderr); break;
			}
		}
		fputs("\n", stderr);
	}
	if(pm3d.direction == PM3D_DEPTH) {
		fprintf(stderr, "\ttrue depth ordering\n");
	}
	else if(pm3d.direction != PM3D_SCANS_AUTOMATIC) {
		fprintf(stderr, "\ttaking scans in %s direction\n", pm3d.direction == PM3D_SCANS_FORWARD ? "FORWARD" : "BACKWARD");
	}
	else {
		fputs("\ttaking scans direction automatically\n", stderr);
	}
	fputs("\tsubsequent scans with different nb of pts are ", stderr);
	if(pm3d.flush == PM3D_FLUSH_CENTER) 
		fputs("CENTERED\n", stderr);
	else 
		fprintf(stderr, "flushed from %s\n", pm3d.flush == PM3D_FLUSH_BEGIN ? "BEGIN" : "END");
	fprintf(stderr, "\tflushing triangles are %sdrawn\n", pm3d.ftriangles ? "" : "not ");
	fputs("\tclipping: ", stderr);
	if(pm3d.clip == PM3D_CLIP_1IN)
		fputs("at least 1 point of the quadrangle in x,y ranges\n", stderr);
	else if(pm3d.clip == PM3D_CLIP_1IN)
		fputs("all 4 points of the quadrangle in x,y ranges\n", stderr);
	else
		fputs("smooth clip to zrange\n", stderr);
	if(pm3d.no_clipcb)
		fputs("\t         quadrangles with out-of-range cb will not be drawn\n", stderr);
	if(pm3d.border.l_type == LT_NODRAW) {
		fprintf(stderr, "\tpm3d quadrangles will have no border\n");
	}
	else {
		fprintf(stderr, "\tpm3d quadrangle borders will default to ");
		save_linetype(stderr, &(pm3d.border), FALSE);
		fprintf(stderr, "\n");
	}
	if(pm3d_shade.strength > 0) {
		fprintf(stderr, "\tlighting primary component %g specular component %g", pm3d_shade.strength, pm3d_shade.spec);
		fprintf(stderr, " second spot contribution %g\n", pm3d_shade.spec2);
	}
	fprintf(stderr, "\tsteps for bilinear interpolation: %d,%d\n", pm3d.interp_i, pm3d.interp_j);
	fprintf(stderr, "\tquadrangle color according to ");
	switch(pm3d.which_corner_color) {
		case PM3D_WHICHCORNER_MEAN: fputs("averaged 4 corners\n", stderr); break;
		case PM3D_WHICHCORNER_GEOMEAN: fputs("geometrical mean of 4 corners\n", stderr); break;
		case PM3D_WHICHCORNER_HARMEAN: fputs("harmonic mean of 4 corners\n", stderr); break;
		case PM3D_WHICHCORNER_MEDIAN: fputs("median of 4 corners\n", stderr); break;
		case PM3D_WHICHCORNER_MIN: fputs("minimum of 4 corners\n", stderr); break;
		case PM3D_WHICHCORNER_MAX: fputs("maximum of 4 corners\n", stderr); break;
		case PM3D_WHICHCORNER_RMS: fputs("root mean square of 4 corners\n", stderr); break;
		default: fprintf(stderr, "corner %i\n", pm3d.which_corner_color - PM3D_WHICHCORNER_C1 + 1);
	}
}
//
// process 'show pointsize' command 
//
static void show_pointsize()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tpointsize is %g\n", GPO.Gg.PointSize);
}
//
// process 'show pointintervalbox' command 
//
static void show_pointintervalbox()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tpointintervalbox is %g\n", GPO.Gg.PointIntervalBox);
}

/* process 'show rgbmax' command */
static void show_rgbmax()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tRGB image color components are in range [0:%g]\n", GPO.Gr.RgbMax);
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
	if(decimalsign)
		fprintf(stderr, "\tdecimalsign for output is %s \n", decimalsign);
	else
		fprintf(stderr, "\tdecimalsign for output has default value (normally '.')\n");
	fprintf(stderr, "\tdegree sign for output is %s \n", degree_sign);
}
//
// process 'show micro' command 
//
static void show_micro()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tmicro character for output is %s \n", (use_micro && micro) ? micro : "u");
}
//
// process 'show minus_sign' command 
//
static void show_minus_sign()
{
	SHOW_ALL_NL;
	if(use_minus_sign && minus_sign)
		fprintf(stderr, "\tminus sign for output is %s \n", minus_sign);
	else
		fprintf(stderr, "\tno special minus sign\n");
}
//
// process 'show fit' command 
//
//static void show_fit()
void GnuPlot::ShowFit()
{
	udvt_entry * v = NULL;
	double d;
	SHOW_ALL_NL;
	switch(fit_verbosity) {
		case QUIET:
		    fprintf(stderr, "\tfit will not output results to console.\n");
		    break;
		case RESULTS:
		    fprintf(stderr, "\tfit will only print final results to console and log-file.\n");
		    break;
		case BRIEF:
		    fprintf(stderr, "\tfit will output brief results to console and log-file.\n");
		    if(fit_wrap)
			    fprintf(stderr, "\toutput of long lines will be wrapped at column %i.\n", fit_wrap);
		    break;
		case VERBOSE:
		    fprintf(stderr, "\tfit will output verbose results to console and log-file.\n");
		    break;
	}
	fprintf(stderr, "\tfit can handle up to %d independent variables\n", MIN(MAX_NUM_VAR, MAXDATACOLS-2));
	fprintf(stderr, "\tfit will%s prescale parameters by their initial values\n", fit_prescale ? "" : " not");
	fprintf(stderr, "\tfit will%s place parameter errors in variables\n", fit_errorvariables ? "" : " not");
	fprintf(stderr, "\tfit will%s place covariances in variables\n", fit_covarvariables ? "" : " not");
	fprintf(stderr, "\tfit will%s scale parameter errors with the reduced chi square\n", fit_errorscaling ? "" : " not");
	if(fit_suppress_log) {
		fprintf(stderr, "\tfit will not create a log file\n");
	}
	else if(fitlogfile) {
		fprintf(stderr, "\tlog-file for fits was set by the user to \n\t'%s'\n", fitlogfile);
	}
	else {
		char * logfile = getfitlogfile();
		if(logfile) {
			fprintf(stderr, "\tlog-file for fits is unchanged from the environment default of\n\t\t'%s'\n", logfile);
			SAlloc::F(logfile);
		}
	}
	v = Ev.GetUdvByName((char *)FITLIMIT);
	d = (v && (v->udv_value.type != NOTDEFINED)) ? real(&(v->udv_value)) : -1.0;
	fprintf(stderr, "\tfits will be considered to have converged if  delta chisq < chisq * %g", ((d > 0.) && (d < 1.)) ? d : DEF_FIT_LIMIT);
	if(epsilon_abs > 0.)
		fprintf(stderr, " + %g", epsilon_abs);
	fprintf(stderr, "\n");
	v = Ev.GetUdvByName((char *)FITMAXITER);
	if(v  && (v->udv_value.type != NOTDEFINED) && (real(&(v->udv_value)) > 0))
		fprintf(stderr, "\tfit will stop after a maximum of %i iterations\n", (int)real(&(v->udv_value)));
	else
		fprintf(stderr, "\tfit has no limit in the number of iterations\n");
	v = Ev.GetUdvByName((char *)FITSTARTLAMBDA);
	d = (v && (v->udv_value.type != NOTDEFINED)) ? real(&(v->udv_value)) : -1.0;
	if(d > 0.)
		fprintf(stderr, "\tfit will start with lambda = %g\n", d);
	v = Ev.GetUdvByName((char *)FITLAMBDAFACTOR);
	d = (v && (v->udv_value.type != NOTDEFINED)) ? real(&(v->udv_value)) : -1.0;
	if(d > 0.)
		fprintf(stderr, "\tfit will change lambda by a factor of %g\n", d);
	if(fit_v4compatible)
		fprintf(stderr, "\tfit command syntax is backwards compatible to version 4\n");
	else
		fprintf(stderr, "\tfit will default to `unitweights` if no `error`keyword is given on the command line.\n");
	fprintf(stderr, "\tfit can run the following command when interrupted:\n\t\t'%s'\n", getfitscript());
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
	fprintf(stderr, "\tpolar is %s\n", (GPO.Gg.Polar ? "ON" : "OFF"));
}
//
// process 'show angles' command 
//
static void show_angles()
{
	SHOW_ALL_NL;
	fputs("\tAngles are in ", stderr);
	if(ang2rad == 1)
		fputs("radians\n", stderr);
	else
		fputs("degrees\n", stderr);
}
//
// process 'show samples' command 
//
//static void show_samples()
void GnuPlot::ShowSamples()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tsampling rate is %d, %d\n", Gg.Samples1, Gg.Samples2);
}
//
// process 'show isosamples' command
//
//static void show_isosamples()
void GnuPlot::ShowIsoSamples()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tiso sampling rate is %d, %d\n", Gg.IsoSamples1, Gg.IsoSamples2);
}
//
// process 'show view' command 
//
//static void show_view()
void GnuPlot::ShowView()
{
	SHOW_ALL_NL;
	fputs("\tview is ", stderr);
	if(splot_map == TRUE) {
		fprintf(stderr, "map scale %g\n", _3DBlk.MapviewScale);
		return;
	}
	else if(xz_projection) {
		fprintf(stderr, "xz projection\n");
	}
	else if(yz_projection) {
		fprintf(stderr, "yz projection\n");
	}
	else {
		fprintf(stderr, "%g rot_x, %g rot_z, %g scale, %g scale_z\n", _3DBlk.SurfaceRotX, _3DBlk.SurfaceRotZ, _3DBlk.SurfaceScale, _3DBlk.SurfaceZScale);
	}
	fprintf(stderr, "\t\t%s axes are %s\n", (V.AspectRatio3D == 2) ? "x/y" : ((V.AspectRatio3D == 3) ? "x/y/z" : ""),
	    (V.AspectRatio3D >= 2) ? "on the same scale" : "independently scaled");
	fprintf(stderr, "\t\t azimuth %g\n", _3DBlk.Azimuth);
}
//
// process 'show surface' command 
//
static void show_surface()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tsurface is %sdrawn %s\n", draw_surface ? "" : "not ", implicit_surface ? "" : "only if explicitly requested");
}

/* process 'show hidden3d' command */
static void show_hidden3d()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\thidden surface is %s\n", hidden3d ? "removed" : "drawn");
	show_hidden3doptions();
}

static void show_increment()
{
	fprintf(stderr, "\tPlot lines increment over ");
	if(GPO.Gg.PreferLineStyles)
		fprintf(stderr, "user-defined line styles rather than default line types\n");
	else
		fprintf(stderr, "default linetypes\n");
}

//static void show_histogram()
void GnuPlot::ShowHistogram()
{
	fprintf(stderr, "\tHistogram style is ");
	SaveHistogramOpts(stderr);
}

static void show_textbox()
{
	save_style_textbox(stderr);
}

/* process 'show history' command */
static void show_history()
{
#ifndef GNUPLOT_HISTORY
	fprintf(stderr, "\tThis copy of gnuplot was not built to use a command history file\n");
#endif
	fprintf(stderr, "\t history size %d%s,  %s,  %s\n", gnuplot_history_size, gnuplot_history_size<0 ? "(unlimited)" : "",
	    history_quiet ? "quiet" : "numbers", history_full ? "full" : "suppress duplicates");
}
//
// process 'show size' command 
//
//static void show_size()
void GnuPlot::ShowSize()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tsize is scaled by %g,%g\n", V.Size.x, V.Size.y);
	if(V.AspectRatio > 0.0f)
		fprintf(stderr, "\tTry to set aspect ratio to %g:1.0\n", V.AspectRatio);
	else if(V.AspectRatio == 0.0f)
		fputs("\tNo attempt to control aspect ratio\n", stderr);
	else
		fprintf(stderr, "\tTry to set LOCKED aspect ratio to %g:1.0\n", -V.AspectRatio);
}
//
// process 'show origin' command 
//
static void show_origin()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\torigin is set to %g,%g\n", GPO.V.Offset.X, GPO.V.Offset.Y);
}
//
// process 'show term' command 
//
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
void GnuPlot::ShowTics(bool showx, bool showy, bool showz, bool showx2, bool showy2, bool showcb)
{
	int i;
	SHOW_ALL_NL;
	fprintf(stderr, "\ttics are in %s of plot\n", (grid_tics_in_front) ? "front" : "back");
	if(showx)
		ShowTicdef(FIRST_X_AXIS);
	if(showx2)
		ShowTicdef(SECOND_X_AXIS);
	if(showy)
		ShowTicdef(FIRST_Y_AXIS);
	if(showy2)
		ShowTicdef(SECOND_Y_AXIS);
	if(showz)
		ShowTicdef(FIRST_Z_AXIS);
	if(showcb)
		ShowTicdef(COLOR_AXIS);
	fprintf(stderr, "\tScales for user tic levels 2-%d are: ", MAX_TICLEVEL-1);
	for(i = 2; i<MAX_TICLEVEL; i++)
		fprintf(stderr, " %g%c", ticscale[i], i<MAX_TICLEVEL-1 ? ',' : '\n');
	screen_ok = FALSE;
}

/* process 'show m[xyzx2y2cb]tics' commands */
static void show_mtics(GpAxis * axis)
{
	char * name = axis_name((AXIS_INDEX)axis->index);
	switch(axis->minitics) {
		case MINI_OFF:
		    fprintf(stderr, "\tminor %stics are off\n", name);
		    break;
		case MINI_DEFAULT:
		    fprintf(stderr,
			"\
\tminor %stics are off for linear scales\n\
\tminor %stics are computed automatically for log scales\n",
			name,
			name);
		    break;
		case MINI_AUTO:
		    fprintf(stderr, "\tminor %stics are computed automatically\n", name);
		    break;
		case MINI_USER:
		    fprintf(stderr, "\
\tminor %stics are drawn with %d subintervals between major xtic marks\n",
			name, (int)axis->mtic_freq);
		    break;
		default:
		    GPO.IntError(NO_CARET, "Unknown minitic type in show_mtics()");
	}
}
//
// process 'show timestamp' command 
//
//static void show_timestamp()
void GnuPlot::ShowTimeStamp()
{
	SHOW_ALL_NL;
	ShowXyzLabel("", "timestamp", &Gg.LblTime);
	fprintf(stderr, "\twritten in %s corner\n", (Gg.TimeLabelBottom ? "bottom" : "top"));
}
//
// process 'show [xyzx2y2rtuv]range' commands 
//
//static void show_range(AXIS_INDEX axis)
void GnuPlot::ShowRange(AXIS_INDEX axis)
{
	SHOW_ALL_NL;
	if(AxS[axis].datatype == DT_TIMEDATE)
		fprintf(stderr, "\tset %sdata time\n", axis_name(axis));
	fprintf(stderr, "\t");
	save_prange(stderr, &AxS[axis]);
}
//
// called by the functions below 
//
//static void show_xyzlabel(const char * name, const char * suffix, text_label * label)
void GnuPlot::ShowXyzLabel(const char * pName, const char * pSuffix, text_label * pLabel)
{
	if(pLabel) {
		fprintf(stderr, "\t%s%s is \"%s\", offset at ", pName, pSuffix, pLabel->text ? conv_text(pLabel->text) : "");
		ShowPosition(&pLabel->offset, 3);
		fprintf(stderr, pLabel->pos == LEFT ? " left justified" : pLabel->pos == RIGHT ? " right justified" : "");
	}
	else
		return;
	if(pLabel->font)
		fprintf(stderr, ", using font \"%s\"", conv_text(pLabel->font));
	if(pLabel->tag == ROTATE_IN_3D_LABEL_TAG)
		fprintf(stderr, ", parallel to axis in 3D plots");
	else if(pLabel->rotate)
		fprintf(stderr, ", rotated by %d degrees in 2D plots", pLabel->rotate);
	if(pLabel->textcolor.type)
		save_textcolor(stderr, &pLabel->textcolor);
	if(pLabel->noenhanced)
		fprintf(stderr, " noenhanced");
	putc('\n', stderr);
}
//
// process 'show title' command 
//
//static void show_title()
void GnuPlot::ShowTitle()
{
	SHOW_ALL_NL;
	ShowXyzLabel("", "title", &Gg.LblTitle);
}
//
// process 'show {x|y|z|x2|y2}label' command 
//
//static void show_axislabel(AXIS_INDEX axis)
void GnuPlot::ShowAxisLabel(AXIS_INDEX axIdx)
{
	SHOW_ALL_NL;
	ShowXyzLabel(axis_name(axIdx), "label", &AxS[axIdx].label);
}
//
// process 'show [xyzx2y2]data' commands 
//
static void show_data_is_timedate(AXIS_INDEX axis)
{
	SHOW_ALL_NL;
	fprintf(stderr, "\t%s is set to %s\n", axis_name(axis), GPO.AxS[axis].datatype == DT_TIMEDATE ? "time" :
	    GPO.AxS[axis].datatype == DT_DMS ? "geographic" :  /* obsolete */ "numerical");
}
//
// process 'show timeformat' command 
//
static void show_timefmt()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tDefault format for reading time data is \"%s\"\n", P_TimeFormat);
}
//
// process 'show link' command 
//
//static void show_link()
void GnuPlot::ShowLink()
{
	if(Pgm.EndOfCommand() || Pgm.AlmostEqualsCur("x$2"))
		save_link(stderr, &AxS[SECOND_X_AXIS]);
	if(Pgm.EndOfCommand() || Pgm.AlmostEqualsCur("y$2"))
		save_link(stderr, &AxS[SECOND_Y_AXIS]);
	if(!Pgm.EndOfCommand())
		Pgm.Shift();
}
//
// process 'show link' command 
//
static void show_nonlinear()
{
	for(int axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++)
		save_nonlinear(stderr, &GPO.AxS[axis]);
}
//
// process 'show locale' command 
//
static void show_locale()
{
	SHOW_ALL_NL;
	dump_locale();
}
//
// process 'show loadpath' command 
//
static void show_loadpath()
{
	SHOW_ALL_NL;
	dump_loadpath();
}
//
// process 'show fontpath' command 
//
static void show_fontpath()
{
	const char * env_fontpath = getenv("GNUPLOT_FONTPATH");
	SHOW_ALL_NL;
	fprintf(stderr, "\tdirectory from 'set fontpath': %s\n", PS_fontpath ? PS_fontpath : "none");
	fprintf(stderr, "\tenvironmental variable GNUPLOT_FONTPATH: %s\n", env_fontpath ? env_fontpath : "none");
}
//
// process 'show zero' command 
//
static void show_zero()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tzero is %g\n", GPO.Gg.Zero);
}
//
// process 'show datafile' command 
//
//static void show_datafile()
void GnuPlot::ShowDataFile()
{
	SHOW_ALL_NL;
	if(Pgm.EndOfCommand() || Pgm.AlmostEqualsCur("miss$ing")) {
		if(missing_val == NULL)
			fputs("\tNo missing data string set for datafile\n", stderr);
		else if(!strcmp(missing_val, "NaN"))
			fprintf(stderr, "\tall NaN (not-a-number) values will be treated as missing data\n");
		else
			fprintf(stderr, "\t\"%s\" in datafile is interpreted as missing value\n", missing_val);
	}
	if(Pgm.EndOfCommand() || Pgm.AlmostEqualsCur("sep$arators")) {
		if(df_separators)
			fprintf(stderr, "\tdatafile fields separated by any of %d characters \"%s\"\n", (int)strlen(df_separators), df_separators);
		else
			fprintf(stderr, "\tdatafile fields separated by whitespace\n");
	}
	if(Pgm.EndOfCommand() || Pgm.AlmostEqualsCur("com$mentschars")) {
		fprintf(stderr, "\tComments chars are \"%s\"\n", df_commentschars);
	}
	if(Pgm.EndOfCommand() || Pgm.AlmostEqualsCur("columnhead$ers")) {
		if(df_columnheaders)
			fprintf(stderr, "\tFirst line is always treated as headers rather than data\n");
		else
			fprintf(stderr, "\tFirst line is treated as headers only if accessed explicitly\n");
	}
	if(df_fortran_constants)
		fputs("\tDatafile parsing will accept Fortran D or Q constants\n", stderr);
	if(df_nofpe_trap)
		fputs("\tNo floating point exception handler during data input\n", stderr);
	if(Pgm.AlmostEqualsCur("bin$ary")) {
		if(!Pgm.EndOfCommand())
			Pgm.Shift();
		if(Pgm.EndOfCommand()) {
			// 'show datafile binary' 
			df_show_binary(stderr);
			fputc('\n', stderr);
		}
		if(Pgm.EndOfCommand() || Pgm.AlmostEqualsCur("datas$izes"))
			df_show_datasizes(stderr); // 'show datafile binary datasizes' 
		if(Pgm.EndOfCommand())
			fputc('\n', stderr);
		if(Pgm.EndOfCommand() || Pgm.AlmostEqualsCur("filet$ypes"))
			df_show_filetypes(stderr); // 'show datafile binary filetypes' 
	}
	if(!Pgm.EndOfCommand())
		Pgm.Shift();
}
//
// process 'show table' command 
//
//static void show_table()
void GnuPlot::ShowTable()
{
	char foo[2] = {0, 0};
	foo[0] = (Tab.P_Sep && *Tab.P_Sep) ? *Tab.P_Sep : '\t';
	SHOW_ALL_NL;
	if(Tab.Mode)
		fprintf(stderr, "\ttable mode is on, field separator %s\n", foo[0] == '\t' ? "tab" : foo[0] == ',' ? "comma" : foo[0] == ' ' ? "space" : foo);
	else
		fprintf(stderr, "\ttable mode is off\n");
}

/* process 'show mouse' command */
static void show_mouse()
{
#ifdef USE_MOUSE
	SHOW_ALL_NL;
	if(mouse_setting.on) {
		fprintf(stderr, "\tmouse is on\n");
		if(mouse_setting.annotate_zoom_box) {
			fprintf(stderr, "\tzoom coordinates will be drawn\n");
		}
		else {
			fprintf(stderr, "\tno zoom coordinates will be drawn\n");
		}
		if(mouse_setting.polardistance) {
			fprintf(stderr, "\tdistance to ruler will be show in polar coordinates\n");
		}
		else {
			fprintf(stderr, "\tno polar distance to ruler will be shown\n");
		}
		if(mouse_setting.doubleclick > 0) {
			fprintf(stderr, "\tdouble click resolution is %d ms\n", mouse_setting.doubleclick);
		}
		else {
			fprintf(stderr, "\tdouble click resolution is off\n");
		}
		if(mouse_mode == MOUSE_COORDINATES_FUNCTION)
			fprintf(stderr, "\tcoordinate readout via mouseformat function %s\n", mouse_readout_function.definition);
		else if(mouse_mode == MOUSE_COORDINATES_ALT)
			fprintf(stderr, "\tcoordinate readout via mouseformat '%s'\n", mouse_alt_string);
		else
			fprintf(stderr, "\tcoordinate readout via mouseformat %d\n", (int)mouse_mode);
		fprintf(stderr, "\tformat for individual coordinates is '%s'\n", mouse_setting.fmt);
		if(mouse_setting.label) {
			fprintf(stderr, "\tButton 2 draws persistent labels with options \"%s\"\n", mouse_setting.labelopts);
		}
		else {
			fprintf(stderr, "\tButton 2 draws temporary labels\n");
		}
		fprintf(stderr, "\tzoom factors are x: %g   y: %g\n", mouse_setting.xmzoom_factor, mouse_setting.ymzoom_factor);
		fprintf(stderr, "\tzoomjump is %s\n", mouse_setting.warp_pointer ? "on" : "off");
		fprintf(stderr, "\tcommunication commands will %sbe shown\n", mouse_setting.verbose ? "" : "not ");
	}
	else {
		fprintf(stderr, "\tmouse is off\n");
	}
#else  /* USE_MOUSE */
	GPO.IntWarn(NO_CARET, "this copy of gnuplot has no mouse support");
#endif /* USE_MOUSE */
}
//
// process 'show plot' command 
//
static void show_plot()
{
	SHOW_ALL_NL;
	fprintf(stderr, "\tlast plot command was: %s\n", replot_line);
}
//
// process 'show variables' command 
//
//static void show_variables()
void GnuPlot::ShowVariables()
{
	udvt_entry * udv = Ev.P_FirstUdv;
	int    len;
	bool   show_all = FALSE;
	char   leading_string[MAX_ID_LEN+1] = {'\0'};
	if(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("all"))
			show_all = TRUE;
		else
			Pgm.CopyStr(leading_string, Pgm.GetCurTokenIdx(), MAX_ID_LEN);
		Pgm.Shift();
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
			/* In the default case skip GPVAL_ variables */
			udv = udv->next_udv;
			continue;
		}
		else if(!strncmp(udv->udv_name, "GPFUN_", 6)) {
			/* Skip GPFUN_ variables; these are reported by "show function" */
			udv = udv->next_udv;
			continue;
		}
		if(udv->udv_value.type == NOTDEFINED) {
			FPRINTF((stderr, "\t%-*s is undefined\n", len, udv->udv_name));
		}
		else {
			fprintf(stderr, "\t%-*s ", len, udv->udv_name);
			fputs("= ", stderr);
			DispValue(stderr, &(udv->udv_value), TRUE);
			putc('\n', stderr);
		}
		udv = udv->next_udv;
	}
}
//
// Show line style number <tag> (0 means show all) 
//
static void show_linestyle(int tag)
{
	linestyle_def * this_linestyle;
	bool showed = FALSE;
	for(this_linestyle = GPO.Gg.P_FirstLineStyle; this_linestyle; this_linestyle = this_linestyle->next) {
		if(tag == 0 || tag == this_linestyle->tag) {
			showed = TRUE;
			fprintf(stderr, "\tlinestyle %d, ", this_linestyle->tag);
			save_linetype(stderr, &this_linestyle->lp_properties, TRUE);
			fputc('\n', stderr);
		}
	}
	if(tag > 0 && !showed)
		GPO.IntErrorCurToken("linestyle not found");
}
//
// Show linetype number <tag> (0 means show all) 
//
//static void show_linetype(linestyle_def * pListHead, int tag)
void GnuPlot::ShowLineType(linestyle_def * pListHead, int tag)
{
	bool showed = FALSE;
	int recycle_count = 0;
	for(linestyle_def * this_linestyle = pListHead; this_linestyle; this_linestyle = this_linestyle->next) {
		if(tag == 0 || tag == this_linestyle->tag) {
			showed = TRUE;
			fprintf(stderr, "\tlinetype %d, ", this_linestyle->tag);
			save_linetype(stderr, &(this_linestyle->lp_properties), TRUE);
			fputc('\n', stderr);
		}
	}
	if(tag > 0 && !showed)
		IntErrorCurToken("linetype not found");
	if(pListHead == Gg.P_FirstPermLineStyle)
		recycle_count = linetype_recycle_count;
	else if(pListHead == Gg.P_FirstMonoLineStyle)
		recycle_count = mono_recycle_count;
	if(tag == 0 && recycle_count > 0)
		fprintf(stderr, "\tLinetypes repeat every %d unless explicitly defined\n", recycle_count);
}
//
// Show arrow style number <tag> (0 means show all) 
//
//static void show_arrowstyle(int tag)
void GnuPlot::ShowArrowStyle(int tag)
{
	bool showed = FALSE;
	for(arrowstyle_def * p_arrowstyle = Gg.P_FirstArrowStyle; p_arrowstyle; p_arrowstyle = p_arrowstyle->next) {
		if(tag == 0 || tag == p_arrowstyle->tag) {
			showed = TRUE;
			fprintf(stderr, "\tarrowstyle %d, ", p_arrowstyle->tag);
			fflush(stderr);
			fprintf(stderr, "\t %s %s", arrow_head_names[p_arrowstyle->arrow_properties.head], p_arrowstyle->arrow_properties.layer ? "front" : "back");
			save_linetype(stderr, &(p_arrowstyle->arrow_properties.lp_properties), FALSE);
			fputc('\n', stderr);
			if(p_arrowstyle->arrow_properties.head > 0) {
				fprintf(stderr, "\t  arrow heads: %s, ", (p_arrowstyle->arrow_properties.headfill==AS_FILLED) ? "filled" :
				    (p_arrowstyle->arrow_properties.headfill==AS_EMPTY) ? "empty" : (p_arrowstyle->arrow_properties.headfill==AS_NOBORDER) ? "noborder" : "nofilled");
				if(p_arrowstyle->arrow_properties.head_length > 0) {
					static char * msg[] = {"(first x axis) ", "(second x axis) ", "(graph units) ", "(screen units) ", "(character units) "};
					fprintf(stderr, " length %s%g, angle %g deg",
					    p_arrowstyle->arrow_properties.head_lengthunit == first_axes ? "" : msg[p_arrowstyle->arrow_properties.head_lengthunit],
					    p_arrowstyle->arrow_properties.head_length, p_arrowstyle->arrow_properties.head_angle);
					if(p_arrowstyle->arrow_properties.headfill != AS_NOFILL)
						fprintf(stderr, ", backangle %g deg", p_arrowstyle->arrow_properties.head_backangle);
				}
				else {
					fprintf(stderr, " (default length and angles)");
				}
				fprintf(stderr, (p_arrowstyle->arrow_properties.head_fixedsize) ? " fixed\n" : "\n");
			}
		}
	}
	if(tag > 0 && !showed)
		IntErrorCurToken("arrowstyle not found");
}
//
// called by show_tics 
//
//static void show_ticdefp(const GpAxis * pAx)
void GnuPlot::ShowTicDefp(const GpAxis * pAx)
{
	ticmark * t;
	const char * ticfmt = conv_text(pAx->formatstring);
	fprintf(stderr, "\t%s-axis tics are %s, \tmajor ticscale is %g and minor ticscale is %g\n",
	    axis_name((AXIS_INDEX)pAx->index), (pAx->TicIn ? "IN" : "OUT"), pAx->ticscale, pAx->miniticscale);
	fprintf(stderr, "\t%s-axis tics:\t", axis_name((AXIS_INDEX)pAx->index));
	switch(pAx->ticmode & TICS_MASK) {
		case NO_TICS:
		    fputs("OFF\n", stderr);
		    return;
		case TICS_ON_AXIS:
		    fputs("on axis", stderr);
		    if(pAx->ticmode & TICS_MIRROR)
			    fprintf(stderr, " and mirrored %s", (pAx->TicIn ? "OUT" : "IN"));
		    break;
		case TICS_ON_BORDER:
		    fputs("on border", stderr);
		    if(pAx->ticmode & TICS_MIRROR)
			    fputs(" and mirrored on opposite border", stderr);
		    break;
	}
	if(pAx->ticdef.rangelimited && !Gg.SpiderPlot)
		fprintf(stderr, "\n\t  tics are limited to data range");
	fputs("\n\t  labels are ", stderr);
	if(pAx->manual_justify) {
		switch(pAx->tic_pos) {
			case LEFT: fputs("left justified, ", stderr); break;
			case RIGHT: fputs("right justified, ", stderr); break;
			case CENTRE: fputs("center justified, ", stderr); break;
		}
	}
	else
		fputs("justified automatically, ", stderr);
	fprintf(stderr, "format \"%s\"", ticfmt);
	fprintf(stderr, "%s", pAx->tictype == DT_DMS ? " geographic" : pAx->tictype == DT_TIMEDATE ? " timedate" : "");
	if(pAx->ticdef.enhanced == FALSE)
		fprintf(stderr, "  noenhanced");
	if(pAx->tic_rotate) {
		fprintf(stderr, " rotated");
		fprintf(stderr, " by %d", pAx->tic_rotate);
		fputs(" in 2D mode, terminal permitting,\n\t", stderr);
	}
	else
		fputs(" and are not rotated,\n\t", stderr);
	fputs("    offset ", stderr);
	ShowPosition(&pAx->ticdef.offset, 3);
	fputs("\n\t", stderr);
	switch(pAx->ticdef.type) {
		case TIC_COMPUTED: fputs("  intervals computed automatically\n", stderr); break;
		case TIC_MONTH: fputs("  Months computed automatically\n", stderr); break;
		case TIC_DAY: fputs("  Days computed automatically\n", stderr); break;
		case TIC_SERIES: 
			{
				fputs("  series", stderr);
				if(pAx->ticdef.def.series.start != -VERYLARGE) {
					fputs(" from ", stderr);
					save_num_or_time_input(stderr, pAx->ticdef.def.series.start, pAx);
				}
				fprintf(stderr, " by %g%s", pAx->ticdef.def.series.incr,
				pAx->datatype == DT_TIMEDATE ? " secs" : "");
				if(pAx->ticdef.def.series.end != VERYLARGE) {
					fputs(" until ", stderr);
					save_num_or_time_input(stderr, pAx->ticdef.def.series.end, pAx);
				}
				putc('\n', stderr);
			}
			break;
		case TIC_USER: fputs("  no auto-generated tics\n", stderr); break;
		default: {
		    IntError(NO_CARET, "unknown ticdef type in show_ticdef()");
		    /* NOTREACHED */
	    }
	}
	if(pAx->ticdef.def.user) {
		fputs("\t  explicit list (", stderr);
		for(t = pAx->ticdef.def.user; t != NULL; t = t->next) {
			if(t->label)
				fprintf(stderr, "\"%s\" ", conv_text(t->label));
			save_num_or_time_input(stderr, t->position, pAx);
			if(t->level)
				fprintf(stderr, " %d", t->level);
			if(t->next)
				fputs(", ", stderr);
		}
		fputs(")\n", stderr);
	}
	if(pAx->ticdef.textcolor.type != TC_DEFAULT) {
		fputs("\t ", stderr);
		save_textcolor(stderr, &pAx->ticdef.textcolor);
		fputs("\n", stderr);
	}
	if(pAx->ticdef.font && *pAx->ticdef.font) {
		fprintf(stderr, "\t  font \"%s\"\n", pAx->ticdef.font);
	}
}
//
// called by show_tics 
//
//static void show_ticdef(AXIS_INDEX axis)
void GnuPlot::ShowTicdef(AXIS_INDEX axis)
{
	ShowTicDefp(&AxS[axis]);
}
//
// Display a value in human-readable form. 
//
//void disp_value(FILE * fp, GpValue * val, bool need_quotes)
void GnuPlot::DispValue(FILE * fp, const GpValue * pVal, bool needQuotes)
{
	fprintf(fp, "%s", GPO.ValueToStr(pVal, needQuotes));
}
//
// convert unprintable characters as \okt, tab as \t, newline \n .. 
//
const char * FASTCALL conv_text(const char * t)
{
	static char * empty = "";
	static char * r = NULL, * s;
	if(!t) 
		return empty;
	else {
		// is this enough? 
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
}
