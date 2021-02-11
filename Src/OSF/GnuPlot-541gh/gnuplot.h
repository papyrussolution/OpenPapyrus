// GNUPLOT.H
//
#include <slib.h>

//#define DEBUG

#define HAVE_STRING_H
#define HAVE_MATH_H
#define HAVE_LOCALE_H
#define HAVE_MEMCPY
#define HAVE_STDLIB_H
#define HAVE_STRSTR
#define HAVE_STRCHR
#define HAVE_STPCPY
#define HAVE_STRNLEN
#define HAVE_WCHAR_H
#define HAVE_STRERROR
#define HAVE_VFPRINTF
#define HAVE_TIME_T_IN_TIME_H
#define HAVE_FDOPEN
#define HAVE_SYS_STAT_H
//#define HAVE_LUA
//#define HAVE_CAIROPDF
//#define HAVE_AMOS
//#define HAVE_COMPLEX_FUNCS
#define HAVE_EXTERNAL_FUNCTIONS
#define STDC_HEADERS
#define HAVE_STDBOOL_H 1
#define HAVE__BOOL
#define HAVE_MEMSET
#define USE_WINGDI
#define USE_MOUSE
#define USE_STATS
#define BOXERROR_3D
#define GNUPLOT_HISTORY
#define READLINE
#define GNUPLOT_INT64_SUPPORT
#define PSLATEX_DRIVER // include pslatex.trm 
#define WITH_METAPOST

#include <signal.h>
#include <setjmp.h>
#include <memory.h>
#ifdef HAVE_WCHAR_H
	#include <wchar.h>
#endif
#include <tchar.h>
#if defined(_MSC_VER) || defined(__WATCOMC__)
	#include <io.h>
#endif
#ifdef _WIN32
	#include <fcntl.h>
#endif
#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif
#if CXX_OS_WINDOWS
	#define WIN_IPC
	#include <win\wtext.h>
#endif
#include <syscfg.h>
#include <stdfn.h>

const int GpResolution = 72; // resolution in dpi for converting pixels to size units 

struct TERMENTRY;
#define termentry TERMENTRY
class GnuPlot;

//#include <gp_types.h>
	#define MAX_ID_LEN 50           /* max length of an identifier */
	#define MAX_LINE_LEN 1024       /* maximum number of chars allowed on line */
	#define DEG2RAD (M_PI / 180.0)
	//
	// type_udv() will return 0 rather than type if udv does not exist 
	//
	enum DATA_TYPES {
		INTGR = 1,
		CMPLX,
		STRING,
		DATABLOCK,
		ARRAY,
		VOXELGRID,
		COLORMAP_ARRAY, /* sub-category of ARRAY containing packed ARGB values */
		NOTDEFINED,     /* exists, but value is currently undefined */
		INVALID_VALUE,  /* used only for error return by external functions */
		INVALID_NAME    /* used only to trap errors in linked axis function definition or a format specifier that does not match a variable type */
	};

	enum MODE_PLOT_TYPE {
		MODE_QUERY, 
		MODE_PLOT, 
		MODE_SPLOT
	};

	enum PLOT_TYPE {
		FUNC, 
		DATA, 
		FUNC3D, 
		DATA3D, 
		NODATA, 
		KEYENTRY, 
		VOXELDATA
	};
	// 
	// we explicitly assign values to the types, such that we can
	// perform bit tests to see if the style involves points and/or lines
	// bit 0 (val 1) = line, bit 1 (val 2) = point, bit 2 (val 4)= error
	// This allows rapid decisions about the sample drawn into the key, for example.
	// 
	// HBB 20010610: new enum, to make mnemonic names for these flags accessible everywhere 
	//
	enum PLOT_STYLE_FLAGS {
		PLOT_STYLE_HAS_LINE      = (1<<0),
		PLOT_STYLE_HAS_POINT     = (1<<1),
		PLOT_STYLE_HAS_ERRORBAR  = (1<<2),
		PLOT_STYLE_HAS_FILL      = (1<<3),
		PLOT_STYLE_HAS_VECTOR    = (1<<4),
		PLOT_STYLE_HAS_PM3DBORDER = (1<<5),
		PLOT_STYLE_BITS          = (1<<6)
	};

	enum PLOT_STYLE {
		LINES        =  0*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
		POINTSTYLE   =  1*PLOT_STYLE_BITS + PLOT_STYLE_HAS_POINT,
		IMPULSES     =  2*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
		LINESPOINTS  =  3*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_POINT | PLOT_STYLE_HAS_LINE),
		DOTS         =  4*PLOT_STYLE_BITS + 0,
		XERRORBARS   =  5*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_POINT | PLOT_STYLE_HAS_ERRORBAR),
		YERRORBARS   =  6*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_POINT | PLOT_STYLE_HAS_ERRORBAR),
		XYERRORBARS  =  7*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_POINT | PLOT_STYLE_HAS_ERRORBAR),
		BOXXYERROR   =  8*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_LINE | PLOT_STYLE_HAS_FILL),
		BOXES        =  9*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_LINE | PLOT_STYLE_HAS_FILL | PLOT_STYLE_HAS_PM3DBORDER),
		BOXERROR     = 10*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_LINE | PLOT_STYLE_HAS_FILL),
		STEPS        = 11*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
		FILLSTEPS    = 11*PLOT_STYLE_BITS + PLOT_STYLE_HAS_FILL,
		FSTEPS       = 12*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
		HISTEPS      = 13*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
		VECTOR       = 14*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE + PLOT_STYLE_HAS_VECTOR,
		CANDLESTICKS = 15*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_ERRORBAR | PLOT_STYLE_HAS_FILL),
		FINANCEBARS  = 16*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
		XERRORLINES  = 17*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_LINE | PLOT_STYLE_HAS_POINT | PLOT_STYLE_HAS_ERRORBAR),
		YERRORLINES  = 18*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_LINE | PLOT_STYLE_HAS_POINT | PLOT_STYLE_HAS_ERRORBAR),
		XYERRORLINES = 19*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_LINE | PLOT_STYLE_HAS_POINT | PLOT_STYLE_HAS_ERRORBAR),
		FILLEDCURVES = 21*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE + PLOT_STYLE_HAS_FILL,
		PM3DSURFACE  = 22*PLOT_STYLE_BITS + PLOT_STYLE_HAS_FILL,
		LABELPOINTS  = 23*PLOT_STYLE_BITS + 0,
		HISTOGRAMS   = 24*PLOT_STYLE_BITS + PLOT_STYLE_HAS_FILL,
		IMAGE        = 25*PLOT_STYLE_BITS + 0,
		RGBIMAGE     = 26*PLOT_STYLE_BITS + 0,
		RGBA_IMAGE   = 27*PLOT_STYLE_BITS + 0,
		CIRCLES      = 28*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE + PLOT_STYLE_HAS_FILL,
		BOXPLOT      = 29*PLOT_STYLE_BITS + PLOT_STYLE_HAS_FILL + PLOT_STYLE_HAS_POINT,
		ELLIPSES     = 30*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE + PLOT_STYLE_HAS_FILL,
		SURFACEGRID  = 31*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
		PARALLELPLOT = 32*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
		TABLESTYLE   = 33*PLOT_STYLE_BITS,
		ZERRORFILL   = 34*PLOT_STYLE_BITS + PLOT_STYLE_HAS_FILL,
		ARROWS       = 35*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE + PLOT_STYLE_HAS_VECTOR,
		ISOSURFACE   = 36*PLOT_STYLE_BITS + PLOT_STYLE_HAS_FILL + PLOT_STYLE_HAS_PM3DBORDER,
		SPIDERPLOT   = 37*PLOT_STYLE_BITS + PLOT_STYLE_HAS_FILL + PLOT_STYLE_HAS_POINT,
		POLYGONS     = 38*PLOT_STYLE_BITS + PLOT_STYLE_HAS_FILL,
		PLOT_STYLE_NONE = -1
	};

	enum PLOT_SMOOTH {
		SMOOTH_NONE = 0,
		SMOOTH_ACSPLINES,
		SMOOTH_BEZIER,
		SMOOTH_CSPLINES,
		SMOOTH_SBEZIER,
		SMOOTH_UNIQUE,
		SMOOTH_UNWRAP,
		SMOOTH_FREQUENCY,
		SMOOTH_CUMULATIVE,
		SMOOTH_KDENSITY,
		SMOOTH_CUMULATIVE_NORMALISED,
		SMOOTH_MONOTONE_CSPLINE,
		SMOOTH_BINS,
		SMOOTH_FREQUENCY_NORMALISED,
		SMOOTH_ZSORT,
		SMOOTH_PATH
	};

	struct cmplx {
		double real;
		double imag;
	};

	struct GpValue {
		GpValue & Init(DATA_TYPES dtyp)
		{
			type = dtyp;
			memzero(&v, sizeof(v));
			return *this;
		}
		void   Destroy();
		int    IntCheck() const;
		void   SetNotDefined() { type = NOTDEFINED; }
		enum DATA_TYPES type;
		union {
			intgr_t int_val;
			cmplx  cmplx_val;
			char * string_val;
			char ** data_array;
			GpValue * value_array;
			struct vgrid * vgrid;
		} v;
	};
	//
	// Defines the type of a coordinate 
	// INRANGE and OUTRANGE points have an x,y point associated with them 
	//
	enum coord_type {
		INRANGE,                /* inside plot boundary */
		OUTRANGE,               /* outside plot boundary, but defined */
		UNDEFINED,              /* not defined at all */
		EXCLUDEDRANGE           /* would be inside plot, but excluded for other reasons */
								/* e.g. in polar mode and outside of trange[tmin:tmax] */
	};
	// 
	// These are aliases of fields in 'GpCoordinate' used to hold
	// extra properties of 3D data points (i.e. anything other than x/y/z)
	// or of specific plot types.
	// The aliases are needed because the total number of data slots is limited
	// to 7: x y z xlow ylow xhigh yhigh
	// At some point we may need to expand GpCoordinate.
	// 
	#define CRD_R yhigh        /* Used by splot styles RGBIMAGE and RGBA_IMAGE */
	#define CRD_G xlow         /* Used by splot styles RGBIMAGE and RGBA_IMAGE */
	#define CRD_B xhigh        /* Used by splot styles RGBIMAGE and RGBA_IMAGE */
	#define CRD_A ylow         /* Used by splot styles RGBIMAGE and RGBA_IMAGE */
	#define CRD_COLOR yhigh    /* Used by all splot styles with variable color */
	#define CRD_ROTATE ylow    /* Used by "with labels" */
	#define CRD_PTSIZE xlow    /* Used by "with points|linespoints|labels" */
	#define CRD_PTTYPE xhigh   /* Used by "with points|linespoints|labels" */
	#define CRD_PTCHAR ylow    /* Used by "with points pt variable */
	#define CRD_ZLOW xlow      /* Used by splot style ZERRORFILL */
	#define CRD_ZHIGH xhigh    /* Used by splot style ZERRORFILL */
	#define CRD_XJITTER xlow   /* Used to hold jitter offset on x */
	#define CRD_YJITTER yhigh  /* Used to hold jitter offset on y */
	#define CRD_PATH xhigh     /* Used by 3D spline code to hold path coordinate */
	#define PATHCOORD 6        /*    must match sequence order of field CRD_PATH */

	struct GpCoordinate /*: public SPoint3R*/ {
		enum CtrBlank {
			ctrBlank = 1
		};
		GpCoordinate() : type(UNDEFINED), x(0.0), y(0.0), z(0.0), ylow(0.0), yhigh(0.0), xlow(0.0), xhigh(0.0)
		{
		}
		GpCoordinate(CtrBlank) : type(UNDEFINED), /*SPoint3R(-999.0, -999.0, -999.0)*/x(-999.0), y(-999.0), z(-999.0), ylow(-999.0), yhigh(-999.0), xlow(-999.0), xhigh(-999.0)
		{
		}
		enum coord_type type; // see above 
		coordval x;
		coordval y;
		coordval z;
		coordval ylow;  // ignored in 3d    
		coordval yhigh; // ignored in 3d    
		coordval xlow;  // ignored in 3d    
		coordval xhigh; // ignored in 3d    
	};

	enum lp_class {
		LP_TYPE   = 0, // lp_style_type defined by 'set linetype'
		LP_STYLE  = 1, // lp_style_type defined by 'set style line'
		LP_ADHOC  = 2, // lp_style_type used for single purpose
		LP_NOFILL = 3  // special treatment of fillcolor
	};
	//
	// Classes of time data 
	//
	enum td_type {
		DT_NORMAL = 0,          /* default; treat values as pure numeric */
		DT_TIMEDATE,            /* old datatype */
		DT_DMS,                 /* degrees minutes seconds */
		DT_UNINITIALIZED,
		DT_BAD                  /* something went wrong (e.g. in gstrptime) */
	};
	// 
	// Introduction of nonlinear axes makes it possible for an axis-mapping function
	// to return "undefined" or NaN. These cannot be encoded as an integer coordinate.
	// So we introduce an integer equivalent to NaN and provide a macro to test for
	// whether a coordinate mapping returned it.
	// 
	#define intNaN (~((uint)(~0)>>1))
//
//#include <gp_time.h>
	// 
	// Define the zero point for internal storage of time+date as some number of seconds
	// Through gnuplot version 4.6 this was taken as 1-jan-2000, i.e. 30 years off from
	// the conventional unix epoch date 1-jan-1970. This caused problems when converting
	// internal <-> external dates given in seconds, so now we change it to agree with
	// the rest of the unix world.
	// 
	#if (0)
		#define ZERO_YEAR	2000
		#define JAN_FIRST_WDAY 6	/* 1 jan 2000 was a Saturday (cal 1 2000 on unix) */
		#define SEC_OFFS_SYS	946684800.0	/*  zero gnuplot (2000) - zero system (1970) */
	#else
		#define ZERO_YEAR	1970
		#define JAN_FIRST_WDAY 4	/* 1 jan 1970 was a Thursday (cal 1 1970 on unix) */
		#define SEC_OFFS_SYS	0.0	/* difference between internal and external epochs */
		#define DEFAULT_TZ     0        /* offset in seconds relative to UTC, east = positive */
	#endif
	#ifdef HAVE_STRUCT_TM_TM_GMTOFF
		#define init_timezone(tm) tm->tm_gmtoff = DEFAULT_TZ
	#else
		#define init_timezone(tm)
	#endif
	//
	// defines used for timeseries, seconds 
	//
	#define YEAR_SEC	31557600.0	/* avg, incl. leap year */
	#define MON_SEC		2629800.0	/* YEAR_SEC / 12 */
	#define WEEK_SEC	604800.0
	#define DAY_SEC		86400.0
	//
	// Prototypes of functions exported by time.c 
	//
	td_type gstrptime(char *, char *, struct tm *, double *, double *); /* string to *tm */
	size_t gstrftime(char *, size_t, const char *, double); /* seconds to string */
	double gtimegm(struct tm *); /* *tm to seconds */
	int    ggmtime(struct tm *, double); /* seconds to *tm */
	int    tmweek(double); /* ISO week number from time in seconds since epoch */
//
//#include <gp_hist.h>
	#define HISTORY_SIZE 500
	// 
	// Variables of history.c needed by other modules:
	// 
	extern int  gnuplot_history_size;
	extern bool history_quiet;
	extern bool history_full;
	// 
	// GNU readline
	// 
	#if defined(HAVE_LIBREADLINE)
		#include <readline/history.h>
	#elif defined(HAVE_LIBEDITLINE) || defined(HAVE_WINEDITLINE)
		// NetBSD editline / WinEditLine (almost) compatible readline replacement
		#include <editline/readline.h>
	#elif defined(READLINE)
		//
		// gnuplot's built-in replacement history functions
		//
		typedef void * histdata_t;

		struct HIST_ENTRY {
			char * line;
			histdata_t data;
			HIST_ENTRY * prev;
			HIST_ENTRY * next;
		};

		extern int history_length;
		extern int history_base;

		void using_history();
		void clear_history();
		void add_history(char * line);
		void read_history(char *);
		int  write_history(char *);
		int  where_history();
		int  history_set_pos(int offset);
		HIST_ENTRY * history_get(int offset);
		HIST_ENTRY * current_history();
		HIST_ENTRY * previous_history();
		HIST_ENTRY * next_history();
		HIST_ENTRY * replace_history_entry(int which, const char * line, histdata_t data);
		HIST_ENTRY * remove_history(int which);
		histdata_t free_history_entry(HIST_ENTRY * histent);
		int  history_search(const char * string, int direction);
		int  history_search_prefix(const char * string, int direction);
	#endif
	#ifdef USE_READLINE
		//
		// extra functions provided by history.c 
		//
		int  gp_read_history(const char * filename);
		void write_history_n(const int, const char *, const char *);
		const char * history_find(char *);
		const char * history_find_by_number(int);
		int  history_find_all(char *);
	#endif
//
//#include <alloc.h>
	//
	// prototypes from "alloc.c"
	//
	generic * FASTCALL gp_alloc(size_t size, const char *message);
	generic * gp_realloc(generic *p, size_t size, const char *message);
//
//#include <eval.h>
	struct at_type;

	#define STACK_DEPTH 250 // maximum size of the execution stack 
	#define MAX_AT_LEN  150 // max number of entries in action table 
	//
	// These are used by add_action() to index the subroutine list ft[] in eval.c 
	//
	enum operators {
		// keep this in line with table in eval.c 
		PUSH, 
		PUSHC, 
		PUSHD1, 
		PUSHD2, 
		PUSHD, 
		POP,
		CALL, 
		CALLN, 
		SUM, 
		LNOT, 
		BNOT, 
		UMINUS,
		LOR, 
		LAND, 
		BOR, 
		XOR, 
		BAND, 
		EQ, 
		NE, 
		GT, 
		LT, 
		GE, 
		LE,
		LEFTSHIFT, 
		RIGHTSHIFT, 
		PLUS, 
		MINUS,
		MULT, 
		DIV, 
		MOD, 
		POWER, 
		FACTORIAL, 
		BOOLE,
		DOLLARS,
		CONCATENATE, 
		EQS, 
		NES, 
		RANGE, 
		INDEX, 
		CARDINALITY,
		ASSIGN,
		// only jump operators go between jump and sf_start, for is_jump() 
		JUMP, 
		JUMPZ, 
		JUMPNZ, 
		JTERN, 
		SF_START,
		// External function call 
	#ifdef HAVE_EXTERNAL_FUNCTIONS
		CALLE,
	#endif
		// functions specific to using spec 
		COLUMN, 
		STRINGCOLUMN, 
		STRCOL, 
		COLUMNHEAD
	};

	#define is_jump(operator) ((operator) >=(int)JUMP && (operator) <(int)SF_START)

	enum int64_overflow {
		INT64_OVERFLOW_IGNORE = 0, 
		INT64_OVERFLOW_TO_FLOAT, 
		INT64_OVERFLOW_UNDEFINED, 
		INT64_OVERFLOW_NAN
	};
	//
	// user-defined function table entry 
	//
	struct udft_entry {
		udft_entry * next_udf; /* pointer to next udf in linked list */
		char * udf_name;         /* name of this function entry */
		at_type * at;     /* pointer to action table to execute */
		char * definition;       /* definition of function as typed */
		int    dummy_num;           /* required number of input variables */
		GpValue dummy_values[MAX_NUM_VAR]; /* current value of dummy variables */
	};
	//
	// user-defined variable table entry 
	//
	struct udvt_entry {
		udvt_entry() : next_udv(0), udv_name(0) 
		{ 
			udv_value.Init(NOTDEFINED);
		}
		udvt_entry(const char * pName, DATA_TYPES dtyp) : next_udv(0), udv_name(pName)
		{
			udv_value.Init(dtyp);
		}
		udvt_entry * next_udv; // pointer to next value in linked list 
		const char * udv_name; // name of this value entry 
		GpValue udv_value;     // value it has 
	};
	//
	// p-code argument 
	//
	union argument {
		int    j_arg;      // offset for jump 
		GpValue v_arg;     // constant value 
		udvt_entry * udv_arg; // pointer to dummy variable 
		udft_entry * udf_arg; // pointer to udf to execute 
	#ifdef HAVE_EXTERNAL_FUNCTIONS
		struct exft_entry * exf_arg; // pointer to external function 
	#endif
	};

	// This type definition has to come after union argument has been declared. 
	typedef void (* FUNC_PTR)(union argument * arg);
	//
	// standard/internal function table entry 
	//
	struct ft_entry {
		const char * f_name; // pointer to name of this function 
		FUNC_PTR Func_; // address of function to call 
	};

	struct GpFuncEntry {
		int   FuncId;
		const char * P_Name; // pointer to name of this function 
		FUNC_PTR Func_; // address of function to call 
	};
	//
	// action table entry 
	//
	struct at_entry {
		enum operators index;   /* index of p-code function */
		union argument arg;
	};

	struct at_type {
		int    a_count; /* count of entries in .actions[] */
		at_entry actions[MAX_AT_LEN]; /* will usually be less than MAX_AT_LEN is malloc()'d copy */
	};
	// 
	// Variables of eval.c needed by other modules:
	// 
	// (replaced with _FuncTab2) {extern const ft_entry _FuncTab[]; // The table of builtin functions 
	extern const GpFuncEntry _FuncTab2[];

	class GpEval {
	public:
		GpEval() : UdvPi("pi", INTGR), P_FirstUdf(0), P_FirstUdv(&UdvPi), P_UdvI(0), P_UdvNaN(0), PP_UdvUserHead(0), 
			OverflowHandling(INT64_OVERFLOW_TO_FLOAT), IsUndefined_(false)
		{
		}
		udvt_entry * AddUdvByName(const char * pKey);
		udvt_entry * GetUdvByName(const char * pKey);
		void   DelUdvByName(const char * pKey, bool wildcard);
		void   ClearUdfList();
		void   InitConstants();
		void   FASTCALL FillGpValString(const char * var, const char * value);
		void   FASTCALL FillGpValInteger(const char * var, intgr_t value);
		void   FillGpValFoat(const char * var, double value);
		void   FillGpValComplex(const char * var, double areal, double aimag);

		udft_entry * P_FirstUdf; // user-def'd functions 
		udvt_entry * P_FirstUdv; // user-def'd variables 
		udvt_entry   UdvPi; // 'pi' variable 
		udvt_entry * P_UdvI; // 'I' (sqrt(-1)) 
		udvt_entry * P_UdvNaN; // 'NaN' variable 
		udvt_entry ** PP_UdvUserHead; // first udv that can be deleted 
		enum int64_overflow OverflowHandling;		
		bool   IsUndefined_;
	};
	//
	// Prototypes of functions exported by eval.c 
	//
	double gp_exp(double x);
	/* HBB 20010726: Moved these here, from util.h. */
	double FASTCALL real(const GpValue *);
	double FASTCALL imag(const GpValue *);
	double magnitude(GpValue *);
	double angle(GpValue *);
	GpValue * Gcomplex(GpValue *, double, double);
	GpValue * FASTCALL Ginteger(GpValue *, intgr_t);
	GpValue * FASTCALL Gstring(GpValue *, char *);
	//GpValue * FASTCALL pop_or_convert_from_string(GpValue *);
	// (replaced with GpValue::Destroy) void FASTCALL free_value(GpValue * a);
	void FASTCALL gpfree_string(GpValue * a);
	void gpfree_array(GpValue * a);
	void FASTCALL real_free_at(at_type * at_ptr);
	// Wrap real_free_at in a macro 
	#define free_at(at_ptr) do { real_free_at(at_ptr); at_ptr = NULL; } while(0)
//
//#include <parse.h>
	// externally usable types defined by parse.h 
	extern bool scanning_range_in_progress; /* exported variables of parse.c */
	extern char set_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1]; /* The choice of dummy variables, as set by 'set dummy', 'set polar' and 'set parametric' */
	// Dummy variables referenced by name in a fit command 
	// Sep 2014 (DEBUG) used to deduce how many independent variables 
	extern int  fit_dummy_var[MAX_NUM_VAR];
	// the currently used 'dummy' variables. Usually a copy of
	// set_dummy_var, but may be changed by the '(s)plot' command
	// containing an explicit range (--> 'plot [phi=0..pi]') 
	extern char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
	extern int at_highest_column_used; /* This is used by plot_option_using() */
	extern bool parse_1st_row_as_headers; /* This is checked by df_readascii() */
	extern udvt_entry * df_array; // This is used by df_open() and df_readascii() 
	// This is used to block re-definition of built-in functions 
	//extern int is_builtin_function(int t_num);
	// 
	// Protection mechanism for trying to parse a string followed by a + or - sign.
	// Also suppresses an undefined variable message if an unrecognized token
	// is encountered during try_to_get_string().
	// 
	extern bool string_result_only;
	//
	// Prototypes of exported functions in parse.c 
	//
	void   parse_reset_after_error();
	at_type * create_call_column_at(char *);
	at_type * create_call_columnhead();
	//udft_entry * add_udf(int t_num);
	void   cleanup_udvlist();
	//int    is_function(int t_num);
	//
	// Code that uses the iteration routines here must provide
	// a blank iteration structure to use for bookkeeping.     
	//
	struct GpIterator {
		GpIterator * next;          // linked list 
		udvt_entry *iteration_udv;
		GpValue original_udv_value; // prior value of iteration variable 
		char * iteration_string;
		int    iteration_start;
		int    iteration_end;
		int    iteration_increment;
		int    iteration_current; // start + increment * iteration 
		int    iteration;         // runs from 0 to (end-start)/increment 
		at_type * start_at;       // expression that evaluates to iteration_start 
		at_type * end_at;         // expression that evaluates to iteration_end 
	};

	extern GpIterator * plot_iterator; // Used for plot and splot 
	extern GpIterator * set_iterator;  // Used by set/unset commands 
	//
	// These are used by the iteration code 
	//
	//GpIterator * check_for_iteration();
	//bool next_iteration(GpIterator *);
	bool empty_iteration(GpIterator *);
	bool forever_iteration(GpIterator *);
	GpIterator * cleanup_iteration(GpIterator *);
	//void parse_link_via(struct udft_entry *);
//
//#include <util.h>
	#define NO_CARET (-1) // special token number meaning 'do not draw the "caret"', for int_error and friends: 
	#define DATAFILE (-2) // token number meaning 'the error was in the datafile, not the command line' 
	// 
	// TRUE if command just typed; becomes FALSE whenever we
	// send some other output to screen.  If FALSE, the command line
	// will be echoed to the screen before the ^ error message.
	// 
	extern bool screen_ok;
	extern char *decimalsign; /* decimal sign */
	extern char *numeric_locale;	/* LC_NUMERIC */
	extern char *time_locale;	/* LC_TIME */
	extern char degree_sign[8]; /* degree sign */
	// special characters used by gprintf() */
	extern const char *micro;
	extern const char *minus_sign;
	extern bool use_micro;
	extern bool use_minus_sign;
	extern const char *current_prompt; /* needed by is_error() and friends */
	extern int debug;
	//
	// Functions exported by util.c: 
	//
	// Command parsing helpers: 
	void   parse_esc(char *);
	char * gp_stradd(const char *, const char *);
	//#define isstringvalue(__c_token) (isstring(__c_token) || GPO.Pgm.TypeUdv(__c_token)==STRING)
		// HBB 20010726: IMHO this one belongs into alloc.c: 
	char * FASTCALL gp_strdup(const char *);
	//void   gprintf(char *, size_t, const char * pFormat, double, double);
	//void   gprintf_value(char *, size_t, const char * pFormat, double, const GpValue *);

	// Error message handling 
	#if defined(VA_START) && defined(STDC_HEADERS)
		#if defined(__GNUC__)
			void os_error(int, const char *, ...) __attribute__((noreturn));
			//void GPO.IntError(int, const char *, ...) __attribute__((noreturn));
			void common_error_exit(void) __attribute__((noreturn));
		#elif defined(_MSC_VER)
			__declspec(noreturn) void os_error(int, const char *, ...);
			//__declspec(noreturn) void GPO.IntError(int, const char *, ...);
			__declspec(noreturn) void common_error_exit();
		#else
			void os_error(int, const char *, ...);
			//void GPO.IntError(int, const char *, ...);
			void common_error_exit();
		#endif
		//void GPO.IntWarn(int, const char *, ...);
	#else
		void os_error();
		//void GPO.IntError();
		//void GPO.IntWarn();
		void common_error_exit();
	#endif
	void   squash_spaces(char *s, int remain);
	bool   existdir(const char *);
	bool   existfile(const char *);
	char * getusername();
	size_t gp_strlen(const char *s);
	char * gp_strchrn(const char *s, int N);
	bool   streq(const char *a, const char *b);
	size_t strappend(char **dest, size_t *size, size_t len, const char *src);
	char * value_to_str(GpValue *val, bool need_quotes);
	char * texify_title(char *title, int plot_type);

	// To disallow 8-bit characters in variable names, set this to 
	// #define ALLOWED_8BITVAR(c) FALSE 
	#define ALLOWED_8BITVAR(c) ((c)&0x80)
//
//
// Default line type is LT_BLACK; reset to this after changing colors 
//
#define LT_AXIS            (-1)
#define LT_BLACK           (-2) // Base line type 
#define LT_SOLID           (-2) // Synonym for base line type 
#define LT_NODRAW          (-3)
#define LT_BACKGROUND      (-4)
#define LT_UNDEFINED       (-5)
#define LT_COLORFROMCOLUMN (-6) // Used by hidden3d code
#define LT_DEFAULT         (-7)
//
// Default point size is taken from the global "pointsize" variable 
//
#define PTSZ_DEFAULT       (-2)
#define PTSZ_VARIABLE      (-3)
#define AS_VARIABLE        (-3)
//
enum t_fillstyle { 
	FS_EMPTY, 
	FS_SOLID, 
	FS_PATTERN, 
	FS_DEFAULT,
	FS_TRANSPARENT_SOLID, 
	FS_TRANSPARENT_PATTERN 
};
//
//#include <color.h>
	// 
	// In general, this file deals with colours, and in the current gnuplot
	// source layout it would correspond to structures and routines found in
	// driver.h, term.h and term.c.
	// 
	// Here we define structures which are required for the communication
	// of palettes between terminals and making palette routines.
	// 
	enum colortype {
		TC_DEFAULT      = 0,    /* Use default color, set separately */
		TC_LT           = 1,    /* Use the color of linetype <n> */
		TC_LINESTYLE    = 2,    /* Use the color of line style <n> */
		TC_RGB          = 3,    /* Explicit RGB triple provided by user */
		TC_CB           = 4,    /* "palette cb <value>" */
		TC_FRAC         = 5,    /* "palette frac <value> */
		TC_Z            = 6,    /* "palette z" */
		TC_VARIABLE     = 7,    /* only used for "tc", never "lc" */
		TC_COLORMAP     = 8     /* "palette colormap" */
	};
	//
	// Generalized pm3d-compatible color specifier
	// Supplements basic linetype choice 
	//
	struct t_colorspec {
		t_colorspec(colortype csType = TC_DEFAULT, int csLt = 0, double csValue = 0.0) : type(csType), lt(csLt), value(csValue)
		{
		}
		void   Set(colortype csType, int csLt, double csValue)
		{
			type = csType;
			lt = csLt;
			value = csValue;
		}
		void   SetDefault() { Set(TC_DEFAULT, 0, 0.0); }
		void   SetBlack() { Set(TC_LT, LT_BLACK, 0.0); }
		void   SetBackground() { Set(TC_LT, LT_BACKGROUND, 0.0); }
		colortype type; // TC_<type> definitions below 
		int    lt;    // used for TC_LT, TC_LINESTYLE and TC_RGB 
		double value; // used for TC_CB and TC_FRAC 
	};

	#define DEFAULT_COLORSPEC {TC_DEFAULT, 0, 0.0}
	#define BLACK_COLORSPEC {TC_LT, LT_BLACK, 0.0}
	#define BACKGROUND_COLORSPEC {TC_LT, LT_BACKGROUND, 0.0}
	//
	// EAM July 2004 - Disentangle polygon support and PM3D support 
	// a point (with integer coordinates) for use in polygon drawing 
	//
	struct gpiPoint {
		int    x;
		int    y;
		int    style;
	};
	//
	// color modes
	//
	enum palette_color_mode {
		SMPAL_COLOR_MODE_NONE = '0',
		SMPAL_COLOR_MODE_GRAY = 'g',  /* grayscale only */
		SMPAL_COLOR_MODE_RGB = 'r',   /* one of several fixed transforms */
		SMPAL_COLOR_MODE_FUNCTIONS = 'f', /* user defined transforms */
		SMPAL_COLOR_MODE_GRADIENT = 'd', /* interpolated table: explicitly defined or read from file */
		SMPAL_COLOR_MODE_CUBEHELIX = 'c'
	};
	// 
	// color gradient type
	// 
	enum palette_gradient_type {
		SMPAL_GRADIENT_TYPE_NONE     = 0,
		SMPAL_GRADIENT_TYPE_SMOOTH   = 1,/* smooth palette */
		SMPAL_GRADIENT_TYPE_DISCRETE = 2, /* (full) discrete palette */
		SMPAL_GRADIENT_TYPE_MIXED    = 3,/* partially discrete palette */
	};
	// 
	// Contains a colour in RGB scheme.
	// Values of  r, g and b  are all in range [0;1] 
	// 
	struct rgb_color {
		double r;
		double g;
		double b;
	};
	// 
	// Contains a colour in RGB scheme.
	// Values of  r, g and b  are uchars in range [0;255] 
	// 
	struct rgb255_color {
		static const uint AnsiTab16[16];
		uint   NearestAnsi() const;
		uint   ToAnsi256() const;
		uchar r;
		uchar g;
		uchar b;
	};
	// 
	// a point (with double coordinates) for use in polygon drawing 
	// the "c" field is used only inside the routine pm3d_plot() 
	// 
	struct gpdPoint {
		double x;
		double y;
		double z;
		double c;
	};
	// 
	// to build up gradients:  whether it is really red, green and blue or maybe
	// hue saturation and value in col depends on cmodel 
	// 
	struct gradient_struct {
		double pos;
		rgb_color col;
	};
	// 
	// inverting the colour for negative picture (default is positive picture) (for pm3d.positive)
	// 
	#define SMPAL_NEGATIVE  'n'
	#define SMPAL_POSITIVE  'p'
	//
	// Declaration of smooth palette, i.e. palette for smooth colours 
	//
	struct t_sm_palette {
		int    InterpolateColorFromGray(double gray, rgb_color * pColor) const;
		void   ColorComponentsFromGray(double gray, rgb_color * pColor) const;
		void   HsvToRgb(rgb_color * pColor) const;
		void   CheckGradientType();
		/** Constants: **/

		/* (Fixed) number of formulae implemented for gray index to RGB
		 * mapping in color.c.  Usage: somewhere in `set' command to check
		 * that each of the below-given formula R,G,B are lower than this
		 * value. */
		int colorFormulae;

		/** Values that can be changed by `set' and shown by `show' commands: **/

		/* can be SMPAL_COLOR_MODE_GRAY or SMPAL_COLOR_MODE_RGB */
		palette_color_mode colorMode;
		/* mapping formulae for SMPAL_COLOR_MODE_RGB */
		int    formulaR;
		int    formulaG;
		int    formulaB;
		char   Positive; // positive or negative figure 
		/* Now the variables that contain the discrete approximation of the
		 * desired palette of smooth colours as created by make_palette in
		 * pm3d.c.  This is then passed into terminal's make_palette, who
		 * transforms this [0;1] into whatever it supports.  */

		/* Only this number of colour positions will be used even though
		 * there are some more available in the discrete palette of the
		 * terminal.  Useful for multiplot.  Max. number of colours is taken
		 * if this value equals 0.  Unused by: PostScript */
		int    UseMaxColors;
		/* Number of colours used for the discrete palette. Equals to the
		 * result from term->make_palette(NULL), or restricted by
		 * use_maxcolor.  Used by: pm, gif. Unused by: PostScript */
		int    Colors;
		/* Table of RGB triplets resulted from applying the formulae. Used
		 * in the 2nd call to term->make_palette for a terminal with
		 * discrete colours. Unused by PostScript which calculates them
		 * analytically. */
		rgb_color * P_Color;

		/** Variables used by some terminals **/

		/* Option unique for output to PostScript file.  By default,
		 * ps_allcF=0 and only the 3 selected rgb color formulae are written
		 * into the header preceding pm3d map in the file.  If ps_allcF is
		 * non-zero, then print there all color formulae, so that it is easy
		 * to play with choosing manually any color scheme in the PS file
		 * (see the definition of "/g"). Like that you can get the
		 * Rosenbrock multiplot figure on my gnuplot.html#pm3d demo page.
		 * Note: this option is used by all terminals of the postscript
		 * family, i.e. postscript, pslatex, epslatex, so it will not be
		 * comfortable to move it to the particular .trm files. */
		bool   ps_allcF;
		// These variables are used to define interpolated color palettes:
		// gradient is an array if (gray,color) pairs.  This array is
		// gradient_num entries big.
		// Interpolated tables are used if colorMode==SMPAL_COLOR_MODE_GRADIENT */
		int    GradientNum;
		gradient_struct * P_Gradient;
		// Smallest nonzero gradient[i+1] - gradient[i].  If this is < (1/colors)
		// Then a truncated gray value may miss the gradient it belongs in. */
		double smallest_gradient_interval;
		// 
		// Identifier of color gradient type which is one of,
		//   1. Smooth gradient (SMPAL_GRADIENT_TYPE_SMOOTH)
		//   2. Discrete gradient (SMPAL_GRADIENT_TYPE_DISCRETE)
		//   3. Smooth and Discrete Mixed gradient (SMPAL_GRADIENT_TYPE_MIXED)
		// This value set by the routine 'check_palette_gradient_type'.
		// 
		int    GradientType;
		int    CModel; // the used color model: RGB, HSV, XYZ, etc. 
		// Three mapping function for gray->RGB/HSV/XYZ/etc. mapping used if colorMode == SMPAL_COLOR_MODE_FUNCTIONS 
		udft_entry Afunc; /* R for RGB, H for HSV, C for CMY, ... */
		udft_entry Bfunc; /* G for RGB, S for HSV, M for CMY, ... */
		udft_entry Cfunc; /* B for RGB, V for HSV, Y for CMY, ... */
		double gamma; /* gamma for gray scale and cubehelix palettes only */
		/* control parameters for the cubehelix palette scheme */
		double cubehelix_start; /* offset (radians) from colorwheel 0 */
		double cubehelix_cycles; /* number of times round the colorwheel */
		double cubehelix_saturation; /* color saturation */
		/* offset for HSV color mapping */
		double HSV_offset;      /* offset (radians) from colorwheel 0 */
	};

	// (replaced with GnuPlot::SmPltt) extern t_sm_palette sm_palette_Removed; // @global

	//void   init_color();  /* call once to initialize variables */
	// 
	// Make the colour palette. Return 0 on success
	// Put number of allocated colours into sm_palette.colors
	// 
	//int    make_palette();
	void   invalidate_palette();
	//void   check_palette_gradient_type();
	// 
	// Send current colour to the terminal
	// 
	void   set_color(termentry * pTerm, double gray);
	//void   set_rgbcolor_var(uint rgbvalue);
	//void   set_rgbcolor_const(uint rgbvalue);
	// 
	// Draw colour smooth box
	// 
	//void   draw_color_smooth_box(termentry * pTerm, int plotMode);
	// 
	// Support for user-callable routines
	// 
	//void   f_hsv2rgb(union argument *);
	//void   f_rgbcolor(union argument *);
	//void   f_palette(union argument *);
	// 
	// miscellaneous color conversions
	// 
	//uint   rgb_from_colorspec(struct t_colorspec * tc);
	// 
	// Support for colormaps (named palettes)
	// 
	uint   rgb_from_colormap(double gray, udvt_entry * colormap);
	double map2gray(double z, udvt_entry * colormap);
	void   get_colormap_range(udvt_entry * colormap, double * cm_min, double * cm_max);
//
//#include <tables.h>
	typedef void (* parsefuncp_t)();

	struct gen_ftable {
		const char * key;
		parsefuncp_t value;
	};
	//
	// The basic structure 
	//
	struct gen_table {
		const char * key;
		int value;
	};
	//
	// options for plot/splot 
	//
	enum plot_id {
		P_INVALID,
		P_AXES, 
		P_BINARY, 
		P_EVERY, 
		P_INDEX, 
		P_MATRIX, 
		P_SMOOTH, 
		P_THRU,
		P_TITLE, 
		P_NOTITLE, 
		P_USING, 
		P_WITH
	};
	//
	// options for plot ax[ei]s 
	//
	enum plot_axes_id {
		AXES_X1Y1, 
		AXES_X2Y2, 
		AXES_X1Y2, 
		AXES_X2Y1, 
		AXES_NONE
	};
	//
	// plot smooth parameters in plot.h 
	//
	// options for 'save' command 
	//
	enum save_id { 
		SAVE_INVALID, 
		SAVE_FUNCS, 
		SAVE_TERMINAL, 
		SAVE_SET, 
		SAVE_VARS,
		SAVE_FIT, 
		SAVE_DATABLOCKS 
	};
	// 
	// options for 'show' and 'set' commands
	// this is rather big, we might be better off with a hash table 
	// 
	enum set_id {
		S_INVALID,
		S_ACTIONTABLE, 
		S_ALL, 
		S_ANGLES, 
		S_ARROW, 
		S_AUTOSCALE, 
		S_BARS, 
		S_BIND, 
		S_BORDER,
		S_BOXDEPTH, 
		S_BOXWIDTH, 
		S_CLABEL, 
		S_CLIP, 
		S_CNTRPARAM, 
		S_CNTRLABEL, 
		S_CONTOUR,
		S_COLOR, 
		S_COLORMAP, 
		S_COLORSEQUENCE, 
		S_CORNERPOLES,
		S_DASHTYPE, 
		S_DATA, 
		S_DATAFILE, 
		S_DECIMALSIGN, 
		S_DGRID3D, 
		S_DUMMY, 
		S_ENCODING,
		S_FIT, S_FONTPATH, S_FORMAT, S_FUNCTIONS,
		S_GRID, S_HIDDEN3D, S_HISTORY, S_HISTORYSIZE, S_ISOSAMPLES, S_PIXMAP,
		S_JITTER, S_KEY, S_LABEL, S_LINK, S_NONLINEAR,
		S_LINESTYLE, S_LINETYPE, S_LOADPATH, S_LOCALE, S_LOGSCALE, S_MACROS,
		S_MAPPING, S_MARGIN, S_LMARGIN, S_RMARGIN, S_TMARGIN, S_BMARGIN, S_MISSING,
		S_MICRO, S_MINUS_SIGN,
		S_MOUSE,
		S_MONOCHROME, S_MULTIPLOT, S_MX2TICS, S_NOMX2TICS, S_MXTICS, S_NOMXTICS,
		S_MY2TICS, S_NOMY2TICS, S_MYTICS, S_NOMYTICS,
		S_MZTICS, S_NOMZTICS, S_MRTICS,
		S_OFFSETS, S_ORIGIN, SET_OUTPUT, S_OVERFLOW, S_PARAMETRIC,
		S_PALETTE, S_PM3D, S_COLORBOX, S_COLORNAMES,
		S_CBLABEL, S_CBRANGE, S_CBTICS, S_NOCBTICS, S_MCBTICS, S_NOMCBTICS,
		S_CBDATA, S_CBDTICS, S_NOCBDTICS, S_CBMTICS, S_NOCBMTICS, S_OBJECT, S_WALL,
		S_PLOT, S_POINTINTERVALBOX, S_POINTSIZE, S_POLAR, S_PRINT, S_PSDIR,
		S_RGBMAX, S_SAMPLES, S_SIZE, S_SURFACE, S_STYLE, S_SPIDERPLOT,
		S_TABLE, S_TERMINAL, S_TERMOPTIONS, S_THETA,
		S_TICS, S_TICSCALE, S_TICSLEVEL, S_TIMEFMT, S_TIMESTAMP, S_TITLE,
		S_TRANGE, S_URANGE, S_VARIABLES, S_VERSION, S_VIEW, S_VRANGE,
		S_VGRID, S_VXRANGE, S_VYRANGE, S_VZRANGE, S_ISOSURFACE,
		S_X2DATA, S_X2DTICS, S_NOX2DTICS, S_X2LABEL, S_X2MTICS, S_NOX2MTICS,
		S_X2RANGE, S_X2TICS, S_NOX2TICS,
		S_XDATA, S_XDTICS, S_NOXDTICS, S_XLABEL, S_XMTICS, S_NOXMTICS, S_XRANGE,
		S_XTICS, S_NOXTICS, S_XYPLANE,
		S_Y2DATA, S_Y2DTICS, S_NOY2DTICS, S_Y2LABEL, S_Y2MTICS, S_NOY2MTICS,
		S_Y2RANGE, S_Y2TICS, S_NOY2TICS,
		S_YDATA, S_YDTICS, S_NOYDTICS, S_YLABEL, S_YMTICS, S_NOYMTICS, S_YRANGE,
		S_YTICS, S_NOYTICS,
		S_ZDATA, S_ZDTICS, S_NOZDTICS, S_ZLABEL, S_ZMTICS, S_NOZMTICS, S_ZRANGE,
		S_ZTICS, S_NOZTICS,
		S_RTICS, S_RRANGE, S_RAXIS, S_RLABEL, S_PAXIS, S_TTICS, S_MTTICS,
		S_ZERO, S_ZEROAXIS, S_XZEROAXIS, S_X2ZEROAXIS, S_YZEROAXIS, S_Y2ZEROAXIS,
		S_ZZEROAXIS,
		S_DEBUG
	};

	enum set_hidden3d_id {
		S_HI_INVALID,
		S_HI_DEFAULTS, S_HI_OFFSET, S_HI_NOOFFSET, S_HI_TRIANGLEPATTERN,
		S_HI_UNDEFINED, S_HI_NOUNDEFINED, S_HI_ALTDIAGONAL, S_HI_NOALTDIAGONAL,
		S_HI_BENTOVER, S_HI_NOBENTOVER,
		S_HI_FRONT, S_HI_BACK
	};

	enum set_key_id {
		S_KEY_INVALID,
		S_KEY_TOP, S_KEY_BOTTOM, S_KEY_LEFT, S_KEY_RIGHT, S_KEY_CENTER,
		S_KEY_VERTICAL, S_KEY_HORIZONTAL, S_KEY_OVER, S_KEY_UNDER, S_KEY_MANUAL,
		S_KEY_INSIDE, S_KEY_OUTSIDE, S_KEY_FIXED, S_KEY_ABOVE, S_KEY_BELOW,
		S_KEY_TMARGIN, S_KEY_BMARGIN, S_KEY_LMARGIN, S_KEY_RMARGIN,
		S_KEY_LLEFT, S_KEY_RRIGHT, S_KEY_REVERSE, S_KEY_NOREVERSE,
		S_KEY_INVERT, S_KEY_NOINVERT,
		S_KEY_ENHANCED, S_KEY_NOENHANCED,
		S_KEY_BOX, S_KEY_NOBOX, S_KEY_SAMPLEN, S_KEY_SPACING, S_KEY_WIDTH,
		S_KEY_HEIGHT, S_KEY_TITLE, S_KEY_NOTITLE,
		S_KEY_FONT, S_KEY_TEXTCOLOR,
		S_KEY_AUTOTITLES, S_KEY_NOAUTOTITLES,
		S_KEY_DEFAULT, S_KEY_ON, S_KEY_OFF,
		S_KEY_MAXCOLS, S_KEY_MAXROWS,
		S_KEY_FRONT, S_KEY_NOFRONT
	};

	enum set_colorbox_id {
		S_COLORBOX_INVALID,
		S_COLORBOX_VERTICAL, S_COLORBOX_HORIZONTAL,
		S_COLORBOX_DEFAULT, S_COLORBOX_USER, S_COLORBOX_CBTICS,
		S_COLORBOX_BORDER, S_COLORBOX_BDEFAULT, S_COLORBOX_NOBORDER,
		S_COLORBOX_ORIGIN, S_COLORBOX_SIZE,
		S_COLORBOX_INVERT, S_COLORBOX_NOINVERT,
		S_COLORBOX_FRONT, S_COLORBOX_BACK
	};

	enum set_palette_id {
		S_PALETTE_INVALID,
		S_PALETTE_POSITIVE, S_PALETTE_NEGATIVE,
		S_PALETTE_GRAY, S_PALETTE_COLOR, S_PALETTE_RGBFORMULAE,
		S_PALETTE_NOPS_ALLCF, S_PALETTE_PS_ALLCF, S_PALETTE_MAXCOLORS,
		S_PALETTE_COLORMAP, S_PALETTE_DEFINED, S_PALETTE_FILE, S_PALETTE_FUNCTIONS,
		S_PALETTE_MODEL, S_PALETTE_GAMMA, S_PALETTE_CUBEHELIX
	};

	enum set_pm3d_id {
		S_PM3D_INVALID,
		S_PM3D_AT,
		S_PM3D_INTERPOLATE,
		S_PM3D_SCANSFORWARD, S_PM3D_SCANSBACKWARD, S_PM3D_SCANS_AUTOMATIC,
		S_PM3D_DEPTH,
		S_PM3D_FLUSH, S_PM3D_FTRIANGLES, S_PM3D_NOFTRIANGLES,
		S_PM3D_CLIP_Z, S_PM3D_CLIP_1IN, S_PM3D_CLIP_4IN, S_PM3D_CLIPCB, S_PM3D_NOCLIPCB,
		S_PM3D_MAP, S_PM3D_BORDER, S_PM3D_NOBORDER, S_PM3D_HIDDEN, S_PM3D_NOHIDDEN,
		S_PM3D_SOLID, S_PM3D_NOTRANSPARENT, S_PM3D_NOSOLID, S_PM3D_TRANSPARENT,
		S_PM3D_IMPLICIT, S_PM3D_NOEXPLICIT, S_PM3D_NOIMPLICIT, S_PM3D_EXPLICIT,
		S_PM3D_WHICH_CORNER,
		S_PM3D_LIGHTING_MODEL, S_PM3D_NOLIGHTING_MODEL
	};

	enum test_id {
		TEST_INVALID,
		TEST_TERMINAL,
		TEST_PALETTE
	};

	enum show_style_id {
		SHOW_STYLE_INVALID,
		SHOW_STYLE_DATA, SHOW_STYLE_FUNCTION, SHOW_STYLE_LINE,
		SHOW_STYLE_FILLING, SHOW_STYLE_ARROW,
		SHOW_STYLE_CIRCLE, SHOW_STYLE_ELLIPSE, SHOW_STYLE_RECTANGLE,
		SHOW_STYLE_INCREMENT, SHOW_STYLE_HISTOGRAM, SHOW_STYLE_BOXPLOT,
		SHOW_STYLE_PARALLEL, SHOW_STYLE_SPIDERPLOT,
		SHOW_STYLE_TEXTBOX
	};

	enum filledcurves_opts_id {
		FILLEDCURVES_CLOSED = 0,
		FILLEDCURVES_X1, 
		FILLEDCURVES_Y1, 
		FILLEDCURVES_X2, 
		FILLEDCURVES_Y2,
		/* requirement: FILLEDCURVES_ATX1 = FILLEDCURVES_X1+4 */
		FILLEDCURVES_ATX1, 
		FILLEDCURVES_ATY1, 
		FILLEDCURVES_ATX2, 
		FILLEDCURVES_ATY2,
		FILLEDCURVES_ATXY,
		FILLEDCURVES_ATR,
		FILLEDCURVES_ABOVE, 
		FILLEDCURVES_BELOW,
		FILLEDCURVES_BETWEEN,
		FILLEDCURVES_DEFAULT
	};

	extern const struct gen_table command_tbl[];
	extern const struct gen_table plot_axes_tbl[];
	extern const struct gen_table plot_smooth_tbl[];
	extern const struct gen_table dgrid3d_mode_tbl[];
	extern const struct gen_table save_tbl[];
	extern const struct gen_table set_tbl[];
	extern const struct gen_table test_tbl[];
	extern const struct gen_table set_key_tbl[];
	extern const struct gen_table set_colorbox_tbl[];
	extern const struct gen_table set_palette_tbl[];
	extern const struct gen_table set_pm3d_tbl[];
	extern const struct gen_table color_model_tbl[];
	extern const struct gen_table set_hidden3d_tbl[];
	extern const struct gen_table show_style_tbl[];
	extern const struct gen_table plotstyle_tbl[];
	extern const struct gen_table fit_verbosity_level[];

	// EAM Nov 2008 - this is now dynamic, so we can add colors on the fly 
	extern struct gen_table * user_color_names_tbl;
	extern struct gen_table * pm3d_color_names_tbl;
	extern const int num_predefined_colors;
	extern int num_userdefined_colors;

	//extern const struct gen_ftable command_ftbl[];
	extern const struct gen_table filledcurves_opts_tbl[];
	//
	// Function prototypes 
	//
	//int lookup_table(const struct gen_table *, int);
	parsefuncp_t lookup_ftable(const struct gen_ftable *, int);
	int lookup_table_entry(const struct gen_table * tbl, const char * search_str);
	int lookup_table_nth(const struct gen_table * tbl, const char * search_str);
	int lookup_table_nth_reverse(const struct gen_table * tbl, int table_len, const char * search_str);
	const char * reverse_table_lookup(const struct gen_table * tbl, int entry);
//
//#include <term_api.h>
	//
	// Pre-defined dash types 
	//
	#define DASHTYPE_NODRAW (-4)
	#define DASHTYPE_CUSTOM (-3)
	#define DASHTYPE_AXIS   (-2)
	#define DASHTYPE_SOLID  (-1)
	/* more...? */

	#define PT_CHARACTER  (-9) /* magic point type that indicates a character rather than a predefined symbol */
	#define PT_VARIABLE   (-8) /* magic point type that indicates true point type comes from a data column */
	#define PT_CIRCLE     (-7) /* magic point type that indicates we really want a circle drawn by do_arc() */
	// 
	// Constant value passed to (term->text_angle)(ang) to generate vertical
	// text corresponding to old keyword "rotate", which produced the equivalent
	// of "rotate by 90 right-justified".
	// 
	#define TEXT_VERTICAL (-270)
	// 
	// Type definitions 
	// 
	// 
	// this order means we can use  x-(just*strlen(text)*t->ChrH)/2 if term cannot justify
	// 
	enum JUSTIFY {
		LEFT,
		CENTRE,
		RIGHT
	};
	//
	// we use a similar trick for vertical justification of multi-line labels 
	//
	enum VERT_JUSTIFY {
		JUST_TOP,
		JUST_CENTRE,
		JUST_BOT
	};

	enum t_linecap {
		BUTT = 0,
		ROUNDED,
		SQUARE
	};
	//
	// custom dash pattern definition modeled after SVG terminal, but string specifications like "--.. " are also allowed and stored 
	//
	#define DASHPATTERN_LENGTH 8 

	struct t_dashtype {
		void   SetDefault()
		{
			THISZERO();
		}
		void   SetPattern(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8)
		{
			pattern[0] = a1;
			pattern[1] = a2;
			pattern[2] = a3;
			pattern[3] = a4;
			pattern[4] = a5;
			pattern[5] = a6;
			pattern[6] = a7;
			pattern[7] = a8;
		}
		float  pattern[DASHPATTERN_LENGTH];
		char   dstring[8];
	};

	#define DEFAULT_DASHPATTERN {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0, 0, 0, 0, 0, 0, 0, 0} }

	struct lp_style_type {  /* contains all Line and Point properties */
		enum {
			defCommon = 0,
			defBkg,
			defZeroAxis,
			defGrid,
			defParallelAxis,
			defBorder,
			defArrow,
			defHypertextPoint
		};
		lp_style_type(int def = defCommon) : flags(0), p_type(0), p_interval(0), p_number(0), P_Colormap(0)
		{
			memzero(p_char, sizeof(p_char));
			custom_dash_pattern.SetDefault();
			if(def == defBkg)
				SetDefault();
			else if(def == defZeroAxis) {
				// #define DEFAULT_AXIS_ZEROAXIS {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}
				l_type = LT_AXIS;
				d_type = DASHTYPE_AXIS;
				l_width = 1.0;
				p_size = PTSZ_DEFAULT;
				pm3d_color.SetBlack();
			}
			else if(def == defParallelAxis) {
				//#define DEFAULT_PARALLEL_AXIS_STYLE {{0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 2.0, 0.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, LAYER_FRONT }
				l_type = LT_BLACK;
				d_type = DASHTYPE_SOLID;
				l_width = 2.0;
				p_size = 0.0;
				pm3d_color.SetBlack();
			}
			else if(def == defGrid) {
				//#define DEFAULT_GRID_LP {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 0.5, 0.0, DEFAULT_P_CHAR, {TC_LT, LT_AXIS, 0.0}, DEFAULT_DASHPATTERN}
				l_type = LT_AXIS;
				d_type = DASHTYPE_AXIS;
				l_width = 0.5;
				p_size = 0.0;
				pm3d_color.Set(TC_LT, LT_AXIS, 0.0);
			}
			else if(def == defBorder) {
				//#define DEFAULT_BORDER_LP { 0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 1.0, 1.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN }
				l_type = LT_BLACK;
				d_type = DASHTYPE_SOLID;
				l_width = 1.0;
				p_size = 1.0;
				pm3d_color.SetBlack();
			}
			else if(def == defArrow) {
				//{0, LT_DEFAULT, 0, DASHTYPE_SOLID, 0, 0, 1.0, 0.0, DEFAULT_P_CHAR, DEFAULT_COLORSPEC, DEFAULT_DASHPATTERN};
				l_type = LT_DEFAULT;
				d_type = DASHTYPE_SOLID;
				l_width = 1.0;
				p_size = 0.0;
				pm3d_color.SetDefault();
			}
			else if(def == defHypertextPoint) {
				// {1, LT_BLACK, 4, DASHTYPE_SOLID, 0, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, {TC_RGB, 0x000000, 0.0}, DEFAULT_DASHPATTERN};
				flags = 1;
				l_type = LT_BLACK;
				p_type = 4;
				d_type = DASHTYPE_SOLID;
				l_width = 1.0;
				p_size = PTSZ_DEFAULT;
				pm3d_color.Set(TC_RGB, 0, 0.0);
			}
			else
				SetDefault2();
		}
		void    SetDefault()
		{
			// {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, DEFAULT_DASHPATTERN},
			flags = 0;
			l_type = LT_BACKGROUND;
			p_type = 0;
			d_type = DASHTYPE_SOLID;
			p_interval = 0;
			p_number = 0;
			l_width = 1.0;
			p_size = 0.0;
			memzero(p_char, sizeof(p_char));
			pm3d_color.SetBackground();
			custom_dash_pattern.SetDefault();
		}
		void   SetDefault2() // DEFAULT_LP_STYLE_TYPE
		{
			// #define DEFAULT_LP_STYLE_TYPE {0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, DEFAULT_COLORSPEC, DEFAULT_DASHPATTERN}
			flags = 0;
			l_type = LT_BLACK;
			p_type = 0;
			d_type = DASHTYPE_SOLID;
			p_interval = 0;
			p_number = 0;
			l_width = 1.0;
			p_size = PTSZ_DEFAULT;
			memzero(p_char, sizeof(p_char));
			pm3d_color.SetDefault();
			custom_dash_pattern.SetDefault();
		}
		void   SetDefaultKeybox()
		{
			// #define DEFAULT_KEYBOX_LP {0, LT_NODRAW, 0, DASHTYPE_SOLID, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}
			flags = 0;
			l_type = LT_NODRAW;
			p_type = 0;
			d_type = DASHTYPE_SOLID;
			p_interval = 0;
			p_number = 0;
			l_width = 1.0;
			p_size = PTSZ_DEFAULT;
			memzero(p_char, sizeof(p_char));
			pm3d_color.SetBlack();
			custom_dash_pattern.SetDefault();
		}
		int    flags;              /* e.g. LP_SHOW_POINTS */
		int    l_type;
		int    p_type;
		int    d_type;             /* Dashtype */
		int    p_interval;         /* Every Nth point in style LINESPOINTS */
		int    p_number;           /* specify number of points in style LINESPOINTS */
		double l_width;
		double p_size;
		char   p_char[8]; // string holding UTF-8 char used if p_type = PT_CHARACTER 
		t_colorspec pm3d_color;
		t_dashtype custom_dash_pattern; /* per-line, user defined dashtype */
		// ... more to come ? 
		udvt_entry * P_Colormap; // used to return private colormap from lp_parse 
	};

	#define DEFAULT_P_CHAR {0, 0, 0, 0, 0, 0, 0, 0}
	#define DEFAULT_LP_STYLE_TYPE {0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, DEFAULT_COLORSPEC, DEFAULT_DASHPATTERN}

	// Bit definitions for lp_style_type.flags 
	#define LP_SHOW_POINTS     (0x1) /* if not set, ignore the point properties of this line style */
	#define LP_NOT_INITIALIZED (0x2) /* internal flag used in set.c:parse_label_options */
	#define LP_EXPLICIT_COLOR  (0x4) /* set by lp_parse if the user provided a color spec */
	#define LP_ERRORBAR_SET    (0x8) /* set by "set errorbars <lineprops> */

	#define DEFAULT_COLOR_SEQUENCE { 0x9400d3, 0x009e73, 0x56b4e9, 0xe69f00, 0xf0e442, 0x0072b2, 0xe51e10, 0x000000 }
	#define PODO_COLOR_SEQUENCE { 0x000000, 0xe69f00, 0x56b4e9, 0x009e73, 0xf0e442, 0x0072b2, 0xd55e00, 0xcc79a7 }

	#define DEFAULT_MONO_LINETYPES { \
			{0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 0, 1.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
			{0, LT_BLACK, 0, 1 /* dt 2 */, 0, 0, 1.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
			{0, LT_BLACK, 0, 2 /* dt 3 */, 0, 0, 1.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
			{0, LT_BLACK, 0, 3 /* dt 4 */, 0, 0, 1.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
			{0, LT_BLACK, 0, 0 /* dt 1 */, 0, 0, 2.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
			{0, LT_BLACK, 0, DASHTYPE_CUSTOM, 0, 0, 1.2 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, \
			 {{16., 8., 2., 5., 2., 5., 2., 8.}, {0, 0, 0, 0, 0, 0, 0, 0}}} \
	}
	//
	// Note:  These are interpreted as bit flags, not ints 
	//
	enum t_arrow_head {
		NOHEAD = 0,
		END_HEAD = 1,
		BACKHEAD = 2,
		BOTH_HEADS = 3,
		HEADS_ONLY = 4,
		SHAFT_ONLY = 8
	};

	extern const char * arrow_head_names[4];

	enum arrowheadfill {
		AS_NOFILL = 0,
		AS_EMPTY,
		AS_FILLED,
		AS_NOBORDER
	};

	struct arrow_style_type {    /* contains all Arrow properties */
		int tag;                     /* -1 (local), AS_VARIABLE, or style index */
		int layer;                   /* 0 = back, 1 = front */
		lp_style_type lp_properties;
		/* head options */
		t_arrow_head head;           /* arrow head choice */
		/* struct GpPosition headsize; */  /* x = length, y = angle [deg] */
		double head_length;          /* length of head, 0 = default */
		int head_lengthunit;         /* unit (x1, x2, screen, graph) */
		double head_angle;           /* front angle / deg */
		double head_backangle;       /* back angle / deg */
		arrowheadfill headfill;      /* AS_FILLED etc */
		bool head_fixedsize;     /* Adapt the head size for short arrow shafts? */
		/* ... more to come ? */
	};
	//
	// Operations used by the terminal entry point term->layer(). 
	//
	enum t_termlayer {
		TERM_LAYER_RESET,
		TERM_LAYER_BACKTEXT,
		TERM_LAYER_FRONTTEXT,
		TERM_LAYER_BEGIN_BORDER,
		TERM_LAYER_END_BORDER,
		TERM_LAYER_BEGIN_GRID,
		TERM_LAYER_END_GRID,
		TERM_LAYER_END_TEXT,
		TERM_LAYER_BEFORE_PLOT,
		TERM_LAYER_AFTER_PLOT,
		TERM_LAYER_KEYBOX,
		TERM_LAYER_BEGIN_KEYSAMPLE,
		TERM_LAYER_END_KEYSAMPLE,
		TERM_LAYER_RESET_PLOTNO,
		TERM_LAYER_BEFORE_ZOOM,
		TERM_LAYER_BEGIN_PM3D_MAP,
		TERM_LAYER_END_PM3D_MAP,
		TERM_LAYER_BEGIN_PM3D_FLUSH,
		TERM_LAYER_END_PM3D_FLUSH,
		TERM_LAYER_BEGIN_IMAGE,
		TERM_LAYER_END_IMAGE,
		TERM_LAYER_BEGIN_COLORBOX,
		TERM_LAYER_END_COLORBOX,
		TERM_LAYER_3DPLOT
	};

	// Options used by the terminal entry point term->waitforinput(). 
	#define TERM_ONLY_CHECK_MOUSING 1
	#define TERM_WAIT_FOR_FONTPROPS 2
	#define TERM_EVENT_POLL_TIMEOUT 0       /* select() timeout in usec */

	// Options used by the terminal entry point term->hypertext(). 
	#define TERM_HYPERTEXT_TOOLTIP 0
	#define TERM_HYPERTEXT_TITLE   1
	#define TERM_HYPERTEXT_FONT    2

	struct fill_style_type {
		fill_style_type(int style, int density, int pattern) : fillstyle(style), filldensity(density), fillpattern(pattern)
		{
		}
		fill_style_type()
		{
			SetDefault();
		}
		void   SetDefault()
		{
			fillstyle = FS_SOLID;
			filldensity = 100;
			fillpattern = 0;
			border_color.SetBlack();
		}
		int fillstyle;
		int filldensity;
		int fillpattern;
		t_colorspec border_color;
	};
	//
	// Options used by the terminal entry point term->boxed_text() 
	//
	enum t_textbox_options {
		TEXTBOX_INIT = 0,
		TEXTBOX_OUTLINE,
		TEXTBOX_BACKGROUNDFILL,
		TEXTBOX_MARGINS,
		TEXTBOX_FINISH,
		TEXTBOX_GREY
	};

	#define FS_OPAQUE (FS_SOLID + (100<<4))
	//
	// Color construction for an image, palette lookup or rgb components. 
	//
	enum t_imagecolor { 
		IC_PALETTE, 
		IC_RGB, 
		IC_RGBA 
	};

	/* Operations possible with term->modify_plots() */
	#define MODPLOTS_SET_VISIBLE         (1<<0)
	#define MODPLOTS_SET_INVISIBLE       (1<<1)
	#define MODPLOTS_INVERT_VISIBILITIES (MODPLOTS_SET_VISIBLE|MODPLOTS_SET_INVISIBLE)
	// 
	// Values for the flags field of TERMENTRY
	// 
	#define TERM_CAN_MULTIPLOT    (1<<0)    /* tested if stdout not redirected */
	#define TERM_CANNOT_MULTIPLOT (1<<1)    /* tested if stdout is redirected  */
	#define TERM_BINARY           (1<<2)    /* open output file with "b"       */
	#define TERM_INIT_ON_REPLOT   (1<<3)    /* call term->init() on replot     */
	#define TERM_IS_POSTSCRIPT    (1<<4)    /* post, next, pslatex, etc        */
	#define TERM_ENHANCED_TEXT    (1<<5)    /* enhanced text mode is enabled   */
	#define TERM_NO_OUTPUTFILE    (1<<6)    /* terminal doesn't write to a file */
	#define TERM_CAN_CLIP         (1<<7)    /* terminal does its own clipping  */
	#define TERM_CAN_DASH         (1<<8)    /* terminal supports dashed lines  */
	#define TERM_ALPHA_CHANNEL    (1<<9)    /* alpha channel transparency      */
	#define TERM_MONOCHROME      (1<<10)    /* term is running in mono mode    */
	#define TERM_LINEWIDTH       (1<<11)    /* support for set term linewidth  */
	#define TERM_FONTSCALE       (1<<12)    /* terminal supports fontscale     */
	#define TERM_POINTSCALE      (1<<13)    /* terminal supports fontscale     */
	#define TERM_IS_LATEX        (1<<14)    /* text uses TeX markup            */
	#define TERM_EXTENDED_COLOR  (1<<15)    /* uses EXTENDED_COLOR_SPECS       */
	#define TERM_NULL_SET_COLOR  (1<<16)    /* no support for RGB color        */
	#define TERM_POLYGON_PIXELS  (1<<17)    /* filledpolygon rather than fillbox */
	// 
	// The terminal interface structure --- heart of the terminal layer.
	// 
	// It should go without saying that additional entries may be made
	// only at the end of this structure. Any fields added must be
	// optional - a value of 0 (or NULL pointer) means an older driver
	// does not support that feature - gnuplot must still be able to
	// function without that terminal feature
	// 
	struct TERMENTRY {
		const char * name;
		const char * description;
		uint  MaxX/*xmax*/;
		uint  MaxY/*ymax*/;
		uint  ChrV;
		uint  ChrH;
		uint  TicV;
		uint  TicH;
		void  (*options)(TERMENTRY * pThis, GnuPlot * pGp);
		void  (*init)(TERMENTRY * pThis);
		void  (*reset)();
		void  (*text)();
		int   (*scale)(double, double);
		void  (*graphics)();
		void  (*move)(uint, uint);
		void  (*vector)(uint, uint);
		void  (*linetype)(int);
		void  (*put_text)(uint, uint, const char*);
		// the following are optional. set term ensures they are not NULL 
		int   (*text_angle)(int);
		int   (*justify_text)(enum JUSTIFY);
		void  (*point)(uint, uint, int);
		void  (*arrow)(uint, uint, uint, uint, int headstyle);
		int   (*set_font)(const char * font);
		void  (*pointsize)(double); /* change pointsize */
		int   flags;
		void  (*suspend)(); /* called after one plot of multiplot */
		void  (*resume)(); /* called before plots of multiplot */
		void  (*fillbox)(int, uint, uint, uint, uint); /* clear in multiplot mode */
		void  (*linewidth)(double linewidth);
	#ifdef USE_MOUSE
		int   (*waitforinput)(int); /* used for mouse and hotkey input */
		void  (*put_tmptext)(int, const char []); // draws temporary text; int determines where: 0=statusline, 1,2: at corners of zoom box, with \r separating text above and below the point 
		void  (*set_ruler)(int, int); /* set ruler location; x<0 switches ruler off */
		void  (*set_cursor)(int, int, int); /* set cursor style and corner of rubber band */
		void  (*set_clipboard)(const char[]); /* write text into cut&paste buffer (clipboard) */
	#endif
		int   (*make_palette)(t_sm_palette *palette);
		// 
		// 1. if palette==NULL, then return nice/suitable
		// maximal number of colours supported by this terminal.
		// Returns 0 if it can make colours without palette (like postscript).
		// 2. if palette!=NULL, then allocate its own palette return value is undefined
		// 3. available: some negative values of max_colors for whatever can be useful
		// 
		void  (*previous_palette)();
		// 
		// release the palette that the above routine allocated and get
		// back the palette that was active before.
		// Some terminals, like displays, may draw parts of the figure
		// using their own palette. Those terminals that possess only
		// one palette for the whole plot don't need this routine.
		// 
		void  (*set_color)(t_colorspec *);
		// EAM November 2004 - revised to take a pointer to struct rgb_color,
		// so that a palette gray value is not the only option for specifying color.
		void  (*filled_polygon)(int points, gpiPoint *corners);
		void  (*image)(uint, uint, coordval *, gpiPoint *, t_imagecolor);
		// Enhanced text mode driver call-backs 
		void  (*enhanced_open)(char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
		void  (*enhanced_flush)();
		void  (*enhanced_writec)(int c);
		// Driver-specific synchronization or other layering commands.
		// Introduced as an alternative to the ugly sight of
		// driver-specific code strewn about in the core routines.
		// As of this point (July 2005) used only by pslatex.trm
		void (*layer)(t_termlayer);
		// Begin/End path control.
		// Needed by PostScript-like devices in order to join the endpoints of a polygon cleanly.
		void (*path)(int p);
		// Scale factor for converting terminal coordinates to output
		// pixel coordinates.  Used to provide data for external mousing code.
		double tscale;
		// Pass hypertext for inclusion in the output plot 
		void (*hypertext)(int type, const char * text);
		void (*boxed_text)(uint, uint, int);
		void (*modify_plots)(uint operations, int plotno);
		void (*dashtype)(int type, t_dashtype *custom_dash_pattern);
	};

	enum set_encoding_id {
		S_ENC_DEFAULT, 
		S_ENC_ISO8859_1, 
		S_ENC_ISO8859_2, 
		S_ENC_ISO8859_9, 
		S_ENC_ISO8859_15,
		S_ENC_CP437, 
		S_ENC_CP850, 
		S_ENC_CP852, 
		S_ENC_CP950,
		S_ENC_CP1250, 
		S_ENC_CP1251, 
		S_ENC_CP1252, 
		S_ENC_CP1254,
		S_ENC_KOI8_R, 
		S_ENC_KOI8_U, 
		S_ENC_SJIS,
		S_ENC_UTF8,
		S_ENC_INVALID
	};

	/* options handling */
	enum { 
		UNSET = -1, 
		no = 0, 
		yes = 1 
	};
	//
	// Variables of term.c needed by other modules: 
	//
	extern struct termentry * term; /* the terminal info structure, being the heart of the whole module */
	extern char term_options[MAX_LINE_LEN+1]; // Options string of the currently used terminal driver 
	extern int curr_arrow_headlength; /* access head length + angle without changing API */
	// angle in degrees 
	extern double curr_arrow_headangle;
	extern double curr_arrow_headbackangle;
	extern arrowheadfill curr_arrow_headfilled;
	extern bool curr_arrow_headfixedsize;

	// Recycle count for user-defined linetypes 
	extern int linetype_recycle_count;
	extern int mono_recycle_count;

	// Current 'output' file: name and open filehandle 
	extern char * outstr;
	extern FILE * gpoutfile;

	/* Output file where postscript terminal output goes to.
	   In particular:
			gppsfile == gpoutfile
					for 'set term': postscript, pstex
			gppsfile == PSLATEX_auxfile
					for 'set term': pslatex, cairolatex
			gppsfile == 0
					for all other terminals
	 */
	extern FILE * gppsfile;
	extern char * PS_psdir;
	extern char * PS_fontpath;       /* just a directory name */
	extern bool monochrome;
	extern bool multiplot;
	extern int multiplot_count;
	extern enum set_encoding_id encoding; /* 'set encoding' support: index of current encoding ... */
	extern const char * encoding_names[]; /* ... in table of encoding names: */
	extern const gen_table set_encoding_tbl[]; /* parsing table for encodings */
	//extern bool term_initialised; // mouse module needs this 
	// The qt and wxt terminals cannot be used in the same session. 
	// Whichever one is used first to plot, this locks out the other. 
	extern void * term_interlock;
	// Support for enhanced text mode. 
	//extern char   enhanced_text[MAX_LINE_LEN+1];
	//extern char * enhanced_cur_text;
	//extern double enhanced_fontscale;
	// give array size to allow the use of sizeof 
	//extern char enhanced_escape_format[16];
	//extern bool ignore_enhanced_text;
	//
	// Prototypes of functions exported by term.c 
	//
	//void term_set_output(char *);
	//void term_reset();
	void init_monochrome();
	struct termentry * change_term(const char * name, int length);

	void write_multiline(termentry * pTerm, int, int, char *, JUSTIFY, VERT_JUSTIFY, int, const char *);
	int estimate_strlen(const char * length, double * estimated_fontheight);
	char * estimate_plaintext(char *);
	void list_terms();
	char * get_terminals_names();
	//struct termentry * set_term();
	void init_terminal();
	//void test_term(termentry * pTerm);

	// Support for enhanced text mode. 
	const char * enhanced_recursion(termentry * pTerm, const char * p, bool brace, char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
	void enh_err_check(const char * str);
	/* note: c is char, but must be declared int due to K&R compatibility. */
	void do_enh_writec(int c);
	/* flag: don't use enhanced output methods --- for output of
	 * filenames, which usually looks bad using subscripts */
	void ignore_enhanced(bool flag);

	// Simple-minded test that point is with drawable area 
	bool on_page(int x, int y);

	// Convert a fill style into a backwards compatible packed form 
	int style_from_fill(const fill_style_type *);
	//
	// Terminal-independent routine to draw a circle or arc 
	//
	//void do_arc(termentry * pTerm, int cx, int cy, double radius, double arc_start, double arc_end, int style, bool wedge);
	int  load_dashtype(t_dashtype * dt, int tag);
	void lp_use_properties(lp_style_type * lp, int tag);
	void load_linetype(termentry * pTerm, lp_style_type * lp, int tag);

	// Wrappers for term->path() 
	void FASTCALL newpath(termentry * pTerm);
	void FASTCALL closepath(termentry * pTerm);
	// 
	// Generic wrapper to check for mouse events or hotkeys during
	// non-interactive input (e.g. "load")
	// 
	void check_for_mouse_events();

	// shared routined to add backslash in front of reserved characters 
	char * escape_reserved_chars(const char * str, const char * reserved);
//
//#include <gadgets.h>
	// 
	// Types and variables concerning graphical plot elements that are not
	// *terminal-specific, are used by both* 2D and 3D plots, and are not
	// *assignable to any particular * axis. I.e. they belong to neither
	// *term_api, graphics, graph3d, nor * axis .h files.
	// 
	// Type definitions 
	//
	// Coordinate system specifications: x1/y1, x2/y2, graph-box relative or screen relative coordinate systems 
	//
	enum position_type {
		first_axes,
		second_axes,
		graph,
		screen,
		character,
		polar_axes
	};
	// 
	// A full 3D position, with all 3 coordinates of possible using different axes.
	// Used for 'set label', 'set arrow' positions and various offsets.
	// 
	struct GpPosition {
		/*
		GpPosition() : scalex(character), scaley(character), scalez(character), x(0.0), y(0.0), z(0.0)
		{
		}
		GpPosition(position_type sx, position_type sy, position_type sz, double _x, double _y, double _z) : scalex(sx), scaley(sy), scalez(sz), x(_x), y(_y), z(_z)
		{
		}
		*/
		void   Set(position_type sx, position_type sy, position_type sz, double _x, double _y, double _z)
		{
			scalex = sx;
			scaley = sy;
			scalez = sz;
			x = _x;
			y = _y;
			z = _z;
		}
		void   SetX(position_type t, double v)
		{
			scalex = t;
			x = v;
		}
		void   SetY(position_type t, double v)
		{
			scaley = t;
			y = v;
		}
		void   SetZ(position_type t, double v)
		{
			scalez = t;
			z = v;
		}
		void   SetDefaultMargin()
		{
			Set(character, character, character, -1.0, -1.0, -1.0);
		}
		void   SetDefaultKeyPosition()
		{
			//#define DEFAULT_KEY_POSITION { graph, graph, graph, 0.9, 0.9, 0. }
			scalex = graph;
			scaley = graph;
			scalez = graph;
			x = 0.9;
			y = 0.9;
			z = 0.0;
		}
		void   UnsetMargin();
		enum position_type scalex;
		enum position_type scaley;
		enum position_type scalez;
		double x;
		double y;
		double z;
	};

	// This is the default state for the axis, timestamp, and plot title labels indicated by tag = -2 
	#define NONROTATING_LABEL_TAG -2
	#define ROTATE_IN_3D_LABEL_TAG -3
	#define VARIABLE_ROTATE_LABEL_TAG -4
	//
	// Linked list of structures storing 'set label' information 
	//
	struct text_label {
		text_label() : next(0), tag(NONROTATING_LABEL_TAG), pos(CENTRE), rotate(0), layer(0),
			boxed(0), text(0), font(0), noenhanced(false), hypertext(false), lp_properties(lp_style_type::defCommon),
			textcolor(TC_LT, -2, 0.0)
		{
			place.Set(character, character, character, 0.0, 0.0, 0.0);
			offset.Set(character, character, character, 0.0, 0.0, 0.0);
		}
		text_label * next; /* pointer to next label in linked list */
		int tag;                /* identifies the label */
		GpPosition place;
		enum JUSTIFY pos;       /* left/center/right horizontal justification */
		int rotate;
		int layer;
		int boxed;              /* 0 no box;  -1 default box props;  >0 boxstyle */
		char * text;
		char * font;            /* Entry font added by DJL */
		t_colorspec textcolor;
		lp_style_type lp_properties;
		GpPosition offset;
		bool noenhanced;
		bool hypertext;
	};

	#define EMPTY_LABELSTRUCT \
		{NULL, NONROTATING_LABEL_TAG, \
		 {character, character, character, 0.0, 0.0, 0.0}, CENTRE, 0, 0, \
		 0, \
		 NULL, NULL, {TC_LT, -2, 0.0}, DEFAULT_LP_STYLE_TYPE, \
		 {character, character, character, 0.0, 0.0, 0.0}, FALSE, \
		 FALSE}
	//
	// Datastructure for implementing 'set arrow' 
	//
	enum arrow_type {
		arrow_end_absolute,
		arrow_end_relative,
		arrow_end_oriented,
		arrow_end_undefined
	};

	struct arrow_def {
		arrow_def * next; /* pointer to next arrow in linked list */
		int tag;                /* identifies the arrow */
		arrow_type type;        /* how to interpret GpPosition end */
		GpPosition start;
		GpPosition end;
		double angle;           /* angle in degrees if type arrow_end_oriented */
		arrow_style_type arrow_properties;
	};
	//
	// The object types supported so far are OBJ_RECTANGLE, OBJ_CIRCLE, and OBJ_ELLIPSE 
	//
	struct t_rectangle {
		void   SetDefault()
		{
			THISZERO();
		}
		int type;               /* 0 = corners;  1 = center + size */
		GpPosition center;      /* center */
		GpPosition extent;      /* width and height */
		GpPosition bl;          /* bottom left */
		GpPosition tr;          /* top right */
	};

	#define DEFAULT_RADIUS (-1.0)
	#define DEFAULT_ELLIPSE (-2.0)

	struct t_circle {
		void   SetDefault()
		{
			// {.circle = {1, {0,0,0,0.,0.,0.}, {graph,0,0,0.02,0.,0.}, 0., 360., true }} }
			type = 1;
			memzero(&center, sizeof(center));
			extent.scalex = graph;
			extent.scaley = first_axes;
			extent.scalez = first_axes;
			extent.x = 0.02;
			extent.y = 0.0;
			extent.z = 0.0;
			arc_begin = 0.0;
			arc_end = 360.0;
			wedge = true;
		}
		int type;               /* not used */
		GpPosition center;      /* center */
		GpPosition extent;      /* radius */
		double arc_begin;
		double arc_end;
		bool wedge;     /* TRUE = connect arc ends to center */
	};

	#define ELLIPSEAXES_XY (0)
	#define ELLIPSEAXES_XX (1)
	#define ELLIPSEAXES_YY (2)

	struct t_ellipse {
		void   SetDefault()
		{
			// {.ellipse = {ELLIPSEAXES_XY, {0,0,0,0.,0.,0.}, {graph,graph,0,0.05,0.03,0.}, 0. }} }
			type = ELLIPSEAXES_XY;
			memzero(&center, sizeof(center));
			extent.scalex = graph;
			extent.scaley = graph;
			extent.scalez = first_axes;
			extent.x = 0.05;
			extent.y = 0.03;
			extent.z = 0.0;
			orientation = 0.0;
		}
		int type;               /* mapping of axes: ELLIPSEAXES_XY, ELLIPSEAXES_XX or ELLIPSEAXES_YY */
		GpPosition center;      /* center */
		GpPosition extent;      /* major and minor axes */
		double orientation;     /* angle of first axis to horizontal */
	};

	struct t_polygon {
		/*explicit t_polygon(int t = 0) : type(t), vertex(0)
		{
		}*/
		void   SetDefault()
		{
			// {.polygon = {0, NULL} } }
			type = 0;
			vertex = 0;
		}
		int type;               /* Number of vertices */
		GpPosition * vertex;    /* Array of vertices */
	};

	enum t_clip_object {
		OBJ_CLIP,       /* Clip to graph unless coordinate type is screen */
		OBJ_NOCLIP, /* Clip to canvas, never to graph */
		OBJ_ALWAYS_CLIP /* Not yet implemented */
	};

	#define OBJ_RECTANGLE (1)
	#define OBJ_CIRCLE (2)
	#define OBJ_ELLIPSE (3)
	#define OBJ_POLYGON (4)
	//
	// Plot layer definitions are collected here. 
	//
	#define LAYER_BEHIND     -1
	#define LAYER_BACK        0
	#define LAYER_FRONT       1
	#define LAYER_FOREGROUND  2     /* not currently used */
	#define LAYER_FRONTBACK   4     /* used only by grid walls */
	#define LAYER_DEPTHORDER  8     /* for objects to be included in pm3d depth sorting */
	#define LAYER_PLOT       16     /* currently used only by fig.trm */
	#define LAYER_PLOTLABELS 99
	//
	// Datastructure for 'set object' 
	//
	typedef struct GpObject {
		enum {
			defUnkn = 0,
			defRectangle = 1,
			defCircle,
			defEllipse,
			defPolygon,
			defGridWall
		};
		explicit GpObject(int def = defUnkn)
		{
			if(def == defRectangle)
				SetDefaultRectangleStyle();
			else if(def == defCircle)
				SetDefaultCircleStyle();
			else if(def == defEllipse)
				SetDefaultEllipseStyle();
			else if(def == defPolygon)
				SetDefaultPolygonStyle();
			else if(def == defGridWall)
				SetDefaultGridWall();
			else
				THISZERO();
		}
		void SetDefaultGridWall()
		{
			//#define WALL_Y0 { NULL, WALL_Y0_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, {FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
			next = 0;
			tag = 0;
			layer = LAYER_FRONTBACK;
			object_type = OBJ_POLYGON;
			clip = OBJ_CLIP;
			fillstyle.fillstyle = FS_TRANSPARENT_SOLID;
			fillstyle.filldensity = 50;
			fillstyle.fillpattern = 0;
			fillstyle.border_color.SetBlack();
			lp_properties.SetDefault();
			o.polygon.type = 5;
			o.polygon.vertex = 0;
		}
		void SetDefaultEllipseStyle()
		{
			next = 0;
			tag = -1;
			layer = 0;
			object_type = OBJ_ELLIPSE;
			clip = OBJ_CLIP;
			fillstyle.SetDefault();
			lp_properties.SetDefault();
			o.ellipse.SetDefault();
		}
		void SetDefaultPolygonStyle()
		{
			next = 0;
			tag = -1;
			layer = 0;
			object_type = OBJ_POLYGON;
			clip = OBJ_CLIP;
			fillstyle.SetDefault();
			lp_properties.SetDefault();
			lp_properties.l_type = LT_BLACK;
			o.polygon.SetDefault();
		}
		void SetDefaultRectangleStyle()
		{
			//{ NULL, -1, 0, OBJ_RECTANGLE, OBJ_CLIP, {FS_SOLID, 100, 0, BLACK_COLORSPEC}, \
			//{0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, DEFAULT_DASHPATTERN}, \
			//{.rectangle = {0, {0, 0, 0, 0., 0., 0.}, {0, 0, 0, 0., 0., 0.}, {0, 0, 0, 0., 0., 0.}, {0, 0, 0, 0., 0., 0.}}} }
			next = 0;
			tag = -1;
			layer = 0;
			object_type = OBJ_RECTANGLE;
			clip = OBJ_CLIP;
			fillstyle.SetDefault();
			lp_properties.SetDefault();
			o.rectangle.SetDefault();
		}
		void SetDefaultCircleStyle()
		{
			next = 0;
			tag = -1;
			layer = 0;
			object_type = OBJ_CIRCLE;
			clip = OBJ_CLIP;
			fillstyle.SetDefault();
			lp_properties.SetDefault();
			o.circle.SetDefault();
		}
		GpObject * next;
		int tag;
		int layer;              /* behind or back or front */
		int object_type;        /* e.g. OBJ_RECTANGLE */
		t_clip_object clip;
		fill_style_type fillstyle;
		lp_style_type lp_properties;
		union o {
			t_rectangle rectangle; 
			t_circle circle; 
			t_ellipse ellipse; 
			t_polygon polygon;
		} o;
	} t_object;
	//
	// Datastructure implementing 'set dashtype' 
	//
	struct custom_dashtype_def {
		custom_dashtype_def * next; /* pointer to next dashtype in linked list */
		int tag;                /* identifies the dashtype */
		int d_type;             /* for DASHTYPE_SOLID or CUSTOM */
		t_dashtype dashtype;
	};
	//
	// Datastructure implementing 'set style line' 
	//
	struct linestyle_def {
		linestyle_def * next; /* pointer to next linestyle in linked list */
		int tag;                /* identifies the linestyle */
		lp_style_type lp_properties;
	};
	//
	// Datastructure implementing 'set style arrow' 
	//
	struct arrowstyle_def {
		arrowstyle_def * next;/* pointer to next arrowstyle in linked list */
		int tag;                /* identifies the arrowstyle */
		arrow_style_type arrow_properties;
	};
	//
	// Datastructure for 'set pixmap 
	//
	struct t_pixmap {
		int tag;                /* index referring to this pixmap */
		t_pixmap * next; /* pointer to next pixmap in the linked list */
		uint ncols; /* image size */
		uint nrows; 
		GpPosition pin;         /* where it goes */
		GpPosition extent;      /* width dx;  dy implicitly dx*aspect_ratio */
		int    layer;              /* front/back/behind */
		bool   center;            /* position is center rather than lower left */
		char * filename;        /* where to read the pixmap pixmap */
		char * colormapname;    /* the colormap this was taken from */
		coordval * image_data;  /* pixel array RGBARGBA... */
	};
	//
	// Used by 'set style parallelaxis' and 'set style spiderplot' 
	//
	struct pa_style {
		//#define DEFAULT_PARALLEL_AXIS_STYLE {{0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 0, 2.0, 0.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, LAYER_FRONT}
		pa_style() : lp_properties(lp_style_type::defParallelAxis), layer(LAYER_FRONT)
		{
		}
		lp_style_type lp_properties;/* used to draw the axes themselves */
		int layer;              /* front/back */
	};

	#define DEFAULT_PARALLEL_AXIS_STYLE {{0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 0, 2.0, 0.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, LAYER_FRONT}

	struct spider_web {
		//#define DEFAULT_SPIDERPLOT_STYLE { DEFAULT_LP_STYLE_TYPE, {FS_EMPTY, 100, 0, DEFAULT_COLORSPEC} }
		spider_web() : lp_properties(lp_style_type::defCommon), fillstyle(FS_EMPTY, 100, 0)
		{
		}
		lp_style_type lp_properties;
		fill_style_type fillstyle;
	};

	extern struct spider_web spiderplot_style;
	#define DEFAULT_SPIDERPLOT_STYLE { DEFAULT_LP_STYLE_TYPE, {FS_EMPTY, 100, 0, DEFAULT_COLORSPEC} }
	//
	// The stacking direction of the key box: (vertical, horizontal) 
	//
	enum t_key_stack_direction {
		GPKEY_VERTICAL,
		GPKEY_HORIZONTAL
	};
	//
	// The region, with respect to the border, key is located: (inside, outside) 
	//
	enum t_key_region {
		GPKEY_AUTO_INTERIOR_LRTBC, /* Auto placement, left/right/top/bottom/center */
		GPKEY_AUTO_EXTERIOR_LRTBC, /* Auto placement, left/right/top/bottom/center */
		GPKEY_AUTO_EXTERIOR_MARGIN, /* Auto placement, margin plus lrc or tbc */
		GPKEY_USER_PLACEMENT     /* User specified placement */
	};
	// 
	// If exterior, there are 12 possible auto placements.  Since
	// left/right/center with top/bottom/center can only define 9
	// locations, further subdivide the exterior region into four
	// subregions for which left/right/center (TMARGIN/BMARGIN)
	// and top/bottom/center (LMARGIN/RMARGIN) creates 12 locations. 
	// 
	enum t_key_ext_region {
		GPKEY_TMARGIN,
		GPKEY_BMARGIN,
		GPKEY_LMARGIN,
		GPKEY_RMARGIN
	};
	// 
	// Key sample to the left or the right of the plot title? 
	// 
	enum t_key_sample_positioning {
		GPKEY_LEFT,
		GPKEY_RIGHT
	};

	struct filledcurves_opts {
		enum filledcurves_opts_id closeto; /* from list FILLEDCURVES_CLOSED, ... */
		double at; /* value for FILLEDCURVES_AT... */
		double aty; /* the other value for FILLEDCURVES_ATXY */
		int    oneside; /* -1 if fill below bound only; +1 if fill above bound only */
	};

	#define EMPTY_FILLEDCURVES_OPTS { FILLEDCURVES_DEFAULT, 0.0, 0.0, 0 }

	enum t_histogram_type {
		HT_NONE,
		HT_STACKED_IN_LAYERS,
		HT_STACKED_IN_TOWERS,
		HT_CLUSTERED,
		HT_ERRORBARS
	};

	struct histogram_style {
		histogram_style() : type(HT_CLUSTERED), gap(2), clustersize(1), start(0.0), end(0.0), startcolor(LT_UNDEFINED),
			startpattern(LT_UNDEFINED), bar_lw(0.0), next(0)
		{
		}
		int    type;       /* enum t_histogram_type */
		int    gap;        /* set style hist gap <n> (space between clusters) */
		int    clustersize; /* number of datasets in this histogram */
		double start;   /* X-coord of first histogram entry */
		double end;     /* X-coord of last histogram entry */
		int    startcolor; /* LT_UNDEFINED or explicit color for first entry */
		int    startpattern; /* LT_UNDEFINED or explicit pattern for first entry */
		double bar_lw;  /* linewidth for error bars */
		histogram_style * next;
		text_label title;
	};

	#define DEFAULT_HISTOGRAM_STYLE { HT_CLUSTERED, 2, 1, 0.0, 0.0, LT_UNDEFINED, LT_UNDEFINED, 0, NULL, EMPTY_LABELSTRUCT }

	enum t_boxplot_factor_labels {
		BOXPLOT_FACTOR_LABELS_OFF,
		BOXPLOT_FACTOR_LABELS_AUTO,
		BOXPLOT_FACTOR_LABELS_X,
		BOXPLOT_FACTOR_LABELS_X2
	};

	#define DEFAULT_BOXPLOT_FACTOR -1

	struct boxplot_style {
		int limit_type; /* 0 = multiple of interquartile 1 = fraction of points */
		double limit_value;
		bool outliers;
		int pointtype;
		int plotstyle;  /* CANDLESTICKS or FINANCEBARS */
		double median_linewidth;
		double separation; /* of boxplots if there are more than one factors */
		t_boxplot_factor_labels labels; /* Which axis to put the tic labels if there are factors */
		bool sort_factors; /* Sort factors in alphabetical order? */
	};

	extern boxplot_style boxplot_opts;
	#define DEFAULT_BOXPLOT_STYLE { 0, 1.5, TRUE, 6, CANDLESTICKS, -1.0, 1.0, BOXPLOT_FACTOR_LABELS_AUTO, FALSE }

	struct textbox_style {
		bool opaque;    // True if the box is background-filled before writing into it 
		bool noborder;  // True if you want fill only, no lines 
		double xmargin; // fraction of default margin to use 
		double ymargin; // fraction of default margin to use 
		double linewidth; // applied to border 
		t_colorspec border_color; // TC_LT + LT_NODRAW is "noborder" 
		t_colorspec fillcolor;  // only used if opaque is TRUE 
	};

	#define DEFAULT_TEXTBOX_STYLE { FALSE, FALSE, 1.0, 1.0, 1.0, BLACK_COLORSPEC, BACKGROUND_COLORSPEC }
	// 
	// Variables defined by gadgets.c needed by other modules. */
	// 
	//
	// bounding box position, in terminal coordinates 
	//
	struct BoundingBox {
		int xleft;
		int xright;
		int ybot;
		int ytop;
	};

	/* EAM Feb 2003 - Move all global variables related to key into a */
	/* single structure. Eventually this will allow multiple keys.    */

	enum keytitle_type {
		NOAUTO_KEYTITLES, FILENAME_KEYTITLES, COLUMNHEAD_KEYTITLES
	};

	struct legend_key {
		legend_key() : visible(true), region(GPKEY_AUTO_INTERIOR_LRTBC), margin(GPKEY_RMARGIN), vpos(JUST_TOP),
			hpos(RIGHT), fixed(true), just(GPKEY_RIGHT), stack_dir(GPKEY_VERTICAL), swidth(4.0), vert_factor(1.0),
			width_fix(0.0), height_fix(0.0), auto_titles(FILENAME_KEYTITLES), front(false), reverse(false),
			invert(false), enhanced(true), font(0), textcolor(TC_LT, LT_BLACK, 0.0), maxcols(0), maxrows(0)
		{
			user_pos.SetDefaultKeyPosition();
			box.SetDefaultKeybox();
		}
		bool visible;           /* Do we show this key at all? */
		t_key_region region;    /* if so: where? */
		t_key_ext_region margin; /* if exterior: where outside? */
		GpPosition user_pos; /* if user specified position, this is it */
		VERT_JUSTIFY vpos;      /* otherwise these guide auto-positioning */
		JUSTIFY hpos;
		bool fixed;     /* prevents key in 3D plot from rotating/scaling with plot */
		t_key_sample_positioning just;
		t_key_stack_direction stack_dir;
		double swidth;          /* 'width' of the linestyle sample line in the key */
		double vert_factor;     /* user specified vertical spacing multiplier */
		double width_fix;       /* user specified additional (+/-) width of key titles */
		double height_fix;
		keytitle_type auto_titles; /* auto title curves unless plotted 'with notitle' */
		bool front;     /* draw key in a second pass after the rest of the graph */
		bool reverse;           /* key back to front */
		bool invert;            /* key top to bottom */
		bool enhanced;          /* enable/disable enhanced text of key titles */
		lp_style_type box; /* linetype of box around key:  */
		char * font;            /* Will be used for both key title and plot titles */
		t_colorspec textcolor;   /* Will be used for both key title and plot titles */
		t_colorspec fillcolor;   /* only used if "set key front" */
		BoundingBox bounds;
		int maxcols;            /* maximum no of columns for horizontal keys */
		int maxrows;            /* maximum no of rows for vertical keys */
		text_label title;       /* holds title line for the key as a whole */
	};

	extern legend_key keyT;

	#define DEFAULT_KEYBOX_LP {0, LT_NODRAW, 0, DASHTYPE_SOLID, 0, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}
	#define DEFAULT_KEY_POSITION { graph, graph, graph, 0.9, 0.9, 0. }

	#define DEFAULT_KEY_PROPS \
		{ TRUE, GPKEY_AUTO_INTERIOR_LRTBC, GPKEY_RMARGIN, DEFAULT_KEY_POSITION, \
		  JUST_TOP, RIGHT, TRUE, GPKEY_RIGHT, GPKEY_VERTICAL, 4.0, 1.0, 0.0, 0.0, \
		  FILENAME_KEYTITLES, FALSE, FALSE, FALSE, TRUE, DEFAULT_KEYBOX_LP, \
		  NULL, {TC_LT, LT_BLACK, 0.0}, BACKGROUND_COLORSPEC, {0, 0, 0, 0}, 0, 0, EMPTY_LABELSTRUCT}
	// 
	// EAM Jan 2006 - Move colorbox structure definition to here from color.h
	// in order to be able to use struct GpPosition
	// 
	#define SMCOLOR_BOX_NO      'n'
	#define SMCOLOR_BOX_DEFAULT 'd'
	#define SMCOLOR_BOX_USER    'u'

	struct color_box_struct {
		char where;
		/* where
			SMCOLOR_BOX_NO .. do not draw the colour box
			SMCOLOR_BOX_DEFAULT .. draw it at default position and size
			SMCOLOR_BOX_USER .. draw it at the position given by user
		 */
		char rotation; /* 'v' or 'h' vertical or horizontal box */
		char border; /* if non-null, a border will be drawn around the box (default) */
		int  border_lt_tag;
		int  cbtics_lt_tag;
		int  layer; /* front or back */
		int  xoffset; /* To adjust left or right, e.g. for y2tics */
		GpPosition origin;
		GpPosition size;
		bool invert; /* gradient low->high runs top->bot rather than bot->top */
		BoundingBox bounds;
	};

	extern color_box_struct color_box;
	extern color_box_struct default_color_box;
	//
	// Holder for various image properties 
	//
	struct t_image {
		t_imagecolor type; /* See above */
		bool fallback; /* true == don't use terminal-specific code */
		uint ncols; // image dimensions 
		uint nrows; 
	};

	struct GpView {
		GpView() : P_ClipArea(&BbPlot), /*XSize(1.0f), YSize(1.0f), ZSize(1.0f),*/Size(1.0f), /*XOffset(0.0f), YOffset(0.0f),*/ Offset(0.0f), 
			AspectRatio(0.0f), AspectRatio3D(0), BoxWidth(-1.0), BoxWidthIsAbsolute(true)
		{
			MEMSZERO(BbPlot);
			MEMSZERO(BbPage);
			MEMSZERO(BbCanvas);
			MarginL.SetDefaultMargin();
			MarginB.SetDefaultMargin();
			MarginR.SetDefaultMargin();
			MarginT.SetDefaultMargin();
		}
		int    FASTCALL ClipPoint(int x, int y) const;
		int    ClipLine(int * x1, int * y1, int * x2, int * y2) const;
		void   ClipPolygon(const gpiPoint * pIn, gpiPoint * pOut, int in_length, int * out_length) const;
		BoundingBox BbPlot; // The graph box (terminal coordinates) calculated by boundary() or boundary3d() 
		BoundingBox BbPage; // The bounding box for 3D plots prior to applying view transformations 
		BoundingBox BbCanvas; // The bounding box for the entire drawable area  of current terminal 
		BoundingBox * P_ClipArea; // The bounding box against which clipping is to be done 
		//float  XSize;   // x scale factor for size 
		//float  YSize;   // y scale factor for size 
		//float  ZSize;   // z scale factor for size 
		SPoint3F Size; // scale factor for size 
		//float  XOffset; // x origin setting 
		//float  YOffset; // y origin setting 
		SPoint2F Offset;  // origin setting 
		float  AspectRatio;   // 1.0 for square. Don't attempt to force it 
		int    AspectRatio3D; // 2 for equal scaling of x and y; 3 for z also
		double BoxWidth;      // box width (automatic) for plot style "with boxes" 
		bool   BoxWidthIsAbsolute; // whether box width is absolute (default) or relative 
		// plot border autosizing overrides, in characters (-1: autosize) 
		GpPosition MarginL;
		GpPosition MarginB;
		GpPosition MarginR;
		GpPosition MarginT;
	};

	#define DEFAULT_MARGIN_POSITION {character, character, character, -1, -1, -1}

	//extern custom_dashtype_def * first_custom_dashtype;
	//extern arrow_def * first_arrow;
	//extern text_label * first_label;
	//extern linestyle_def * first_linestyle;
	//extern linestyle_def * first_perm_linestyle;
	//extern linestyle_def * first_mono_linestyle;
	//extern arrowstyle_def * first_arrowstyle;
	//extern t_pixmap * pixmap_listhead;
	extern pa_style parallel_axis_style;
	//extern GpObject * first_object;
	extern GpObject grid_wall[];
	extern text_label title;
	extern text_label timelabel;
	#ifndef DEFAULT_TIMESTAMP_FORMAT
		#define DEFAULT_TIMESTAMP_FORMAT "%a %b %d %H:%M:%S %Y" // asctime() format 
	#endif
	extern int  timelabel_bottom;
	extern bool polar;
	extern bool spiderplot;
	extern bool inverted_raxis;     /* true if GPO.AxS.__R().set_min > GPO.AxS.__R().set_max */

	#define ZERO 1e-8               /* default for 'zero' set option */
	extern double zero;             /* zero threshold, not 0! */
	extern double pointsize;
	extern double pointintervalbox;
	extern t_colorspec background_fill;

	#define SOUTH           1 /* 0th bit */
	#define WEST            2 /* 1th bit */
	#define NORTH           4 /* 2th bit */
	#define EAST            8 /* 3th bit */
	#define border_east     (draw_border & EAST)
	#define border_west     (draw_border & WEST)
	#define border_south    (draw_border & SOUTH)
	#define border_north    (draw_border & NORTH)
	extern int draw_border;
	extern int user_border;
	extern int border_layer;
	extern bool cornerpoles;
	extern lp_style_type border_lp;
	extern const lp_style_type background_lp;
	extern const lp_style_type default_border_lp;
	extern bool clip_lines1;
	extern bool clip_lines2;
	extern bool clip_points;
	extern bool clip_radial;
		
	#define SAMPLES 100             /* default number of samples for a plot */
	extern int samples_1;
	extern int samples_2;
	extern double ang2rad; /* 1 or pi/180 */
	extern enum PLOT_STYLE data_style;
	extern enum PLOT_STYLE func_style;
	extern bool parametric;
	extern bool in_parametric;
	extern bool is_3d_plot; /* If last plot was a 3d one. */
	// 
	// A macro to check whether 2D functionality is allowed in the last plot:
	// either the plot is a 2D plot, or it is a suitably oriented 3D plot (e.g. map).
	// 
	#define ALMOST2D (!is_3d_plot || splot_map || (fabs(fmod(surface_rot_z, 90.0))<0.1 && fabs(fmod(surface_rot_x, 180.0))<0.1))

	enum TRefresh_Allowed {
		E_REFRESH_NOT_OK = 0,
		E_REFRESH_OK_2D = 2,
		E_REFRESH_OK_3D = 3
	};

	extern TRefresh_Allowed refresh_ok;
	#define SET_REFRESH_OK(ok, nplots) do { refresh_ok = (ok); refresh_nplots = (nplots); } while(0)
	extern int refresh_nplots;
	extern bool volatile_data;
	extern int current_x11_windowid; /* WINDOWID to be filled by terminals running on X11 (x11, wxt, qt, ...) */
	// 
	// Functions exported by gadgets.c 
	// 
	void init_gadgets(); /* initialization (called once on program entry */
	// moved here from util3d: 
	//int  draw_clip_line(termentry * pTerm, int, int, int, int);
	void draw_clip_polygon(termentry * pTerm, int, gpiPoint *);
	//void draw_clip_arrow(termentry * pTerm, double, double, double, double, t_arrow_head);
	//void clip_polygon(const gpiPoint * pIn, gpiPoint *, int, int *);
	//int  FASTCALL clip_point(int, int);
	// moved here from graph3d: 
	void clip_move(int x, int y);
	//void clip_vector(termentry * pTerm, int x, int y);
	//void draw_polar_clip_line(termentry * pTerm, double, double, double, double);
	// Common routines for setting line or text color from t_colorspec 
	//void apply_pm3dcolor(termentry * pTerm, t_colorspec * tc);
	//void reset_textcolor(const struct t_colorspec * tc);
	// Timestamp code shared by 2D and 3D 
	void do_timelabel(int x, int y);

	extern fill_style_type default_fillstyle;

	// Warning: C89 does not like the union initializers 
	extern struct GpObject default_rectangle;
	#define DEFAULT_RECTANGLE_STYLE { NULL, -1, 0, OBJ_RECTANGLE, OBJ_CLIP, \
					  {FS_SOLID, 100, 0, BLACK_COLORSPEC},                    \
					  {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, \
					   DEFAULT_DASHPATTERN}, \
					  {.rectangle = \
					   {0, {0, 0, 0, 0., 0., 0.}, {0, 0, 0, 0., 0., 0.}, {0, 0, 0, 0., 0., 0.}, {0, 0, 0, 0., 0., 0.}}} }

	extern struct GpObject default_circle;
	#define DEFAULT_CIRCLE_STYLE { NULL, -1, 0, OBJ_CIRCLE, OBJ_CLIP, \
					   {FS_SOLID, 100, 0, BLACK_COLORSPEC},                    \
					   {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, \
					DEFAULT_DASHPATTERN}, \
					   {.circle = {1, {0, 0, 0, 0., 0., 0.}, {graph, 0, 0, 0.02, 0., 0.}, 0., 360., TRUE }} }

	extern struct GpObject default_ellipse;
	#define DEFAULT_ELLIPSE_STYLE { NULL, -1, 0, OBJ_ELLIPSE, OBJ_CLIP, \
					{FS_SOLID, 100, 0, BLACK_COLORSPEC},                    \
					{0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, \
					 DEFAULT_DASHPATTERN}, \
					{.ellipse = {ELLIPSEAXES_XY, {0, 0, 0, 0., 0., 0.}, {graph, graph, 0, 0.05, 0.03, 0.}, 0. }} }

	#define DEFAULT_POLYGON_STYLE { NULL, -1, 0, OBJ_POLYGON, OBJ_CLIP, \
					{FS_SOLID, 100, 0, BLACK_COLORSPEC},                    \
					{0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 0, 1.0, 0.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
					{.polygon = {0, NULL} } }

	#define WALL_Y0_TAG 0
	#define WALL_X0_TAG 1
	#define WALL_Y1_TAG 2
	#define WALL_X1_TAG 3
	#define WALL_Z0_TAG 4
	#define WALL_Y0_CORNERS { {graph, graph, graph, 0, 0, 0}, \
		{graph, graph, graph, 0, 0, 1}, {graph, graph, graph, 1, 0, 1}, {graph, graph, graph, 1, 0, 0}, {graph, graph, graph, 0, 0, 0} }
	#define WALL_X0_CORNERS { {graph, graph, graph, 0, 0, 0}, \
		{graph, graph, graph, 0, 1, 0}, {graph, graph, graph, 0, 1, 1}, {graph, graph, graph, 0, 0, 1}, {graph, graph, graph, 0, 0, 0} }
	#define WALL_Y1_CORNERS { {graph, graph, graph, 0, 1, 0}, \
		{graph, graph, graph, 1, 1, 0}, {graph, graph, graph, 1, 1, 1}, {graph, graph, graph, 0, 1, 1}, {graph, graph, graph, 0, 1, 0} }
	#define WALL_X1_CORNERS { {graph, graph, graph, 1, 0, 0}, \
		{graph, graph, graph, 1, 0, 1}, {graph, graph, graph, 1, 1, 1}, {graph, graph, graph, 1, 1, 0}, {graph, graph, graph, 1, 0, 0} }
	#define WALL_Z0_CORNERS { {graph, graph, graph, 0, 0, 0}, \
		{graph, graph, graph, 1, 0, 0}, {graph, graph, graph, 1, 1, 0}, {graph, graph, graph, 0, 1, 0}, {graph, graph, graph, 0, 0, 0} }
	#define WALL_Y_COLOR 0xcdb79e
	#define WALL_X_COLOR 0x228b22
	#define WALL_Z_COLOR 0xa0b6cd
	#define WALL_Y0 { NULL, WALL_Y0_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, \
		{FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
	#define WALL_Y1 { NULL, WALL_Y1_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, \
		{FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
	#define WALL_X0 { NULL, WALL_X0_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, \
		{FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
	#define WALL_X1 { NULL, WALL_X1_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, \
		{FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
	#define WALL_Z0 { NULL, WALL_Z0_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, \
		{FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }

	// filledcurves style options set by 'set style [data|func] filledcurves opts' 
	extern filledcurves_opts filledcurves_opts_data;
	extern filledcurves_opts filledcurves_opts_func;

	// Prefer line styles over plain line types 
	// Mostly for backwards compatibility 
	extern bool prefer_line_styles;
	extern histogram_style histogram_opts;

	// TODO: linked list rather than fixed size array 
	#define NUM_TEXTBOX_STYLES 4
	extern textbox_style textbox_opts[NUM_TEXTBOX_STYLES];

	void   default_arrow_style(arrow_style_type * arrow);
	void   apply_head_properties(const arrow_style_type * arrow_properties);
	void   free_labels(text_label * tl);
	void   get_offsets(text_label * this_label, int * htic, int * vtic);
	int    label_width(const char *, int *);
	bool   pm3d_objects();
	//void   place_title(int title_x, int title_y);
//
//#include <axis.h>
	// Aug 2017 - unconditional support for nonlinear axes 
	//#define nonlinear(axis) ((axis)->linked_to_primary != NULL && (axis)->link_udf->at != NULL)
	#define invalid_coordinate(x, y) ((uint)(x)==intNaN || (uint)(y)==intNaN)
	// 
	// give some names to some array elements used in command.c and grap*.c
	// maybe one day the relevant items in setshow will also be stored in arrays.
	// 
	// Always keep the following conditions alive:
	// SECOND_X_AXIS = FIRST_X_AXIS + SECOND_AXES
	// FIRST_X_AXIS & SECOND_AXES == 0
	// 
	enum AXIS_INDEX {
		NO_AXIS = -2,
		ALL_AXES = -1,
		FIRST_Z_AXIS = 0,
	#define FIRST_AXES FIRST_Z_AXIS
		FIRST_Y_AXIS,
		FIRST_X_AXIS,
		COLOR_AXIS,             /* fill gap */
		SECOND_Z_AXIS,          /* not used, yet */
	#define SECOND_AXES SECOND_Z_AXIS
		SAMPLE_AXIS = SECOND_Z_AXIS,
		SECOND_Y_AXIS,
		SECOND_X_AXIS,
		POLAR_AXIS,
	#define NUMBER_OF_MAIN_VISIBLE_AXES (POLAR_AXIS + 1)
		T_AXIS,
		U_AXIS,
		V_AXIS,         /* Last index into GPO.AxS[] */
		PARALLEL_AXES,  /* Parallel axis structures are allocated dynamically */
		THETA_index = 1234, /* Used to identify THETA_AXIS */
		AXIS_ARRAY_SIZE = PARALLEL_AXES
	};

	#define SAMPLE_INTERVAL mtic_freq // sample axis doesn't need mtics, so use the slot to hold sample interval 
	//
	// What kind of ticmarking is wanted? 
	//
	enum t_ticseries_type {
		TIC_COMPUTED = 1,       /* default; gnuplot figures them */
		TIC_SERIES,             /* user-defined series */
		TIC_USER,               /* user-defined points */
		TIC_MONTH,              /* print out month names ((mo-1)%12)+1 */
		TIC_DAY                 /* print out day of week */
	};
	// 
	// Defines one ticmark for TIC_USER style.
	// If label==NULL, the value is printed with the usual format string.
	// else, it is used as the format string (note that it may be a constant
	// string, like "high" or "low").
	// 
	struct ticmark {
		double position;        /* where on axis is this */
		char * label;           /* optional (format) string label */
		int level;              /* 0=major tic, 1=minor tic */
		ticmark * next;  /* linked list */
	};
	//
	// Tic-mark labelling definition; see set xtics 
	//
	struct t_ticdef {
		t_ticdef() : type(TIC_COMPUTED), font(0), rangelimited(false), enhanced(true), logscaling(false)
		{
			def.user = 0;
			def.series.start = 0.0;
			def.series.incr = 0.0;
			def.series.end = 0.0;
			def.mix = false;
			offset.Set(character, character, character, 0.0, 0.0, 0.0);
		}
		t_ticseries_type type;
		char * font;
		struct t_colorspec textcolor;
		struct {
			struct ticmark * user; // for TIC_USER 
			struct { // for TIC_SERIES 
				double start;
				double incr;
				double end;  // ymax, if VERYLARGE */
			} series;
			bool mix;      // TRUE to use both the above 
		} def;
		GpPosition offset;
		bool rangelimited; // Limit tics to data range 
		bool enhanced;     // Use enhanced text mode or labels 
		bool logscaling;   // place tics using old logscale algorithm 
	};
	// 
	// we want two auto modes for minitics - default where minitics are
	// auto for log/time and off for linear, and auto where auto for all
	// graphs I've done them in this order so that logscale-mode can
	// simply test bit 0 to see if it must do the minitics automatically.
	// similarly, conventional plot can test bit 1 to see if minitics are required 
	// 
	enum t_minitics_status {
		MINI_OFF,
		MINI_DEFAULT,
		MINI_USER,
		MINI_AUTO
	};
	// 
	// Values to put in the axis_tics[] variables that decides where the
	// ticmarks should be drawn: not at all, on one or both plot borders,
	// or the zeroaxes. These look like a series of values, but TICS_MASK
	// shows that they're actually bit masks --> don't turn into an enum
	// 
	#define NO_TICS        0
	#define TICS_ON_BORDER 1
	#define TICS_ON_AXIS   2
	#define TICS_MASK      3
	#define TICS_MIRROR    4
	// 
	// Tic levels 0 and 1 are maintained in the axis structure.
	// Tic levels 2 - MAX_TICLEVEL have only one property - scale.
	// 
	#define MAX_TICLEVEL 5
	extern double ticscale[MAX_TICLEVEL];
	// 
	// HBB 20010610: new type for storing autoscale activity. Effectively
	// two booleans (bits) in a single variable, so I'm using an enum with
	// all 4 possible bit masks given readable names. 
	// 
	enum t_autoscale {
		AUTOSCALE_NONE = 0,
		AUTOSCALE_MIN = 1<<0,
		AUTOSCALE_MAX = 1<<1,
		AUTOSCALE_BOTH = (1<<0 | 1 << 1),
		AUTOSCALE_FIXMIN = 1<<2,
		AUTOSCALE_FIXMAX = 1<<3
	};

	inline t_autoscale operator ~ (t_autoscale a1) { return (t_autoscale)~((int)a1); }
	inline t_autoscale operator | (t_autoscale a1, t_autoscale a2) { return (t_autoscale)((int)a1 | (int)a2); }
	inline t_autoscale operator &= (t_autoscale & rA1, t_autoscale a2) { return (rA1 = (t_autoscale)((int)rA1 & (int)a2)); }
	inline t_autoscale operator |= (t_autoscale & rA1, t_autoscale a2) { return (rA1 = (t_autoscale)((int)rA1 | (int)a2)); }

	enum t_constraint {
		CONSTRAINT_NONE  = 0,
		CONSTRAINT_LOWER = 1<<0,
		CONSTRAINT_UPPER = 1<<1,
		CONSTRAINT_BOTH  = (1<<0 | 1<<1)
	};

	inline t_constraint operator ~ (t_constraint a1) { return (t_constraint)~((int)a1); }
	inline t_constraint operator &= (t_constraint & rA1, t_constraint a2) { return (rA1 = (t_constraint)((int)rA1 & (int)a2)); }
	inline t_constraint operator |= (t_constraint & rA1, t_constraint a2) { return (rA1 = (t_constraint)((int)rA1 | (int)a2)); }
	//
	// The unit the tics of a given time/date axis are to interpreted in 
	// HBB 20040318: start at one, to avoid undershoot 
	//
	enum t_timelevel {
		TIMELEVEL_SECONDS = 1, TIMELEVEL_MINUTES, TIMELEVEL_HOURS,
		TIMELEVEL_DAYS, TIMELEVEL_WEEKS, TIMELEVEL_MONTHS,
		TIMELEVEL_YEARS
	};

	struct GpAxis {
		GpAxis() : autoscale(AUTOSCALE_BOTH), set_autoscale(AUTOSCALE_BOTH), range_flags(0), 
			min(-10.0), max(10.0), set_min(-10.0), set_max(10.0), writeback_min(-10.0), writeback_max(10.0), data_min(0.0), data_max(0.0),
			min_constraint(CONSTRAINT_NONE), max_constraint(CONSTRAINT_NONE), min_lb(0.0), min_ub(0.0), max_lb(0.0), max_ub(0.0),
			term_lower(0), term_upper(0), term_scale(0.0), term_zero(0), log(false), base(0.0), log_base(0.0),
			linked_to_primary(0), linked_to_secondary(0), link_udf(0), ticmode(NO_TICS), /*ticdef @ctr*/tic_rotate(0), tic_pos(CENTRE),
			gridmajor(false), gridminor(false), minitics(MINI_DEFAULT), mtic_freq(10.0),
			ticscale(1.0), miniticscale(0.5), ticstep(0.0), TicIn(true), datatype(DT_NORMAL), tictype(DT_NORMAL),
			formatstring(0), ticfmt(0), timelevel((t_timelevel)(0)), index(0), manual_justify(false), zeroaxis(0), paxis_x(0.0)
		{
		}
		//#define inrange(z, min, max) (((min)<(max)) ? (((z)>=(min)) && ((z)<=(max))) : (((z)>=(max)) && ((z)<=(min))))
		bool   InRange(double v) const { return ((min<max) ? ((v>=min) && (v<=max)) : ((v>=max) && (v<=min))); }
		static void UnsetLabelOrTitle(text_label * pLabel);
		//#define axis_map_toint(x) static_cast<int>((x) + 0.5)
		static int MapRealToInt(double x) { return static_cast<int>((x) + 0.5); }
		void   Destroy();
		void   UnsetTics();
		void   FASTCALL AutoscaleOnePoint(double x);
		//#define nonlinear(axis) ((axis)->linked_to_primary != NULL && (axis)->link_udf->at != NULL)
		bool   IsNonLinear() const { return (linked_to_primary && link_udf->at); }
		bool   BadRange() const;
		//#define axis_map_double(axis, variable) ((axis)->term_lower + ((variable) - (axis)->min) * (axis)->term_scale)
		double Map(double var) const { return (term_lower + (var - min) * term_scale); }
		int    MapI(double var) const { return static_cast<int>((term_lower + (var - min) * term_scale) + 0.5); }
		//#define axis_mapback(axis, pos) (((double)(pos) - (axis)->term_lower)/(axis)->term_scale + (axis)->min)
		double MapBack(int var) { return (((double)(var) - term_lower)/term_scale + min); }
		// range of this axis 
		t_autoscale autoscale;     // Which end(s) are autoscaled? */
		t_autoscale set_autoscale; // what does 'set' think autoscale to be? 
		int range_flags;        /* flag bits about autoscale/writeback: */
		// write auto-ed ranges back to variables for autoscale 
	#define RANGE_WRITEBACK   1
	#define RANGE_SAMPLED     2
	#define RANGE_IS_REVERSED 4
		double min;             /* 'transient' axis extremal values */
		double max;
		double set_min;         /* set/show 'permanent' values */
		double set_max;
		double writeback_min;   /* ULIG's writeback implementation */
		double writeback_max;
		double data_min;        /* Not necessarily the same as axis min */
		double data_max;
		// range constraints 
		t_constraint min_constraint;
		t_constraint max_constraint;
		double min_lb; // min lower- and upper-bound 
		double min_ub; 
		double max_lb; // min lower- and upper-bound
		double max_ub; 
		// output-related quantities 
		int    term_lower;         // low and high end of the axis on output,
		int    term_upper;         // ... (in terminal coordinates)
		double term_scale;      // scale factor: plot --> term coords
		uint   term_zero;       // position of zero axis 
		// log axis control 
		bool   log;       /* log axis stuff: flag "islog?" */
		double base;            /* logarithm base value */
		double log_base;        /* ln(base), for easier computations */
		// linked axis information (used only by x2, y2)
		// If axes are linked, the primary axis info will be cloned into the
		// secondary axis only up to this point in the structure.
		GpAxis * linked_to_primary; /* Set only in a secondary axis */
		GpAxis * linked_to_secondary; /* Set only in a primary axis */
		udft_entry * link_udf;
		// ticmark control variables 
		int    ticmode;    // tics on border/axis? mirrored?
		t_ticdef ticdef;   // tic series definition 
		int    tic_rotate; // ticmarks rotated by this angle 
		enum JUSTIFY tic_pos;   /* left/center/right tic label justification */
		bool gridmajor;         /* Grid lines wanted on major tics? */
		bool gridminor;         /* Grid lines for minor tics? */
		t_minitics_status minitics; /* minor tic mode (none/auto/user)? */
		double mtic_freq;       /* minitic stepsize */
		double ticscale;        /* scale factor for tic marks (was (0..1])*/
		double miniticscale;    /* and for minitics */
		double ticstep;         /* increment used to generate tic placement */
		bool   TicIn;          /* tics to be drawn inward?  */
		// time/date axis control 
		td_type datatype;       /* {DT_NORMAL|DT_TIMEDATE} controls _input_ */
		td_type tictype;        /* {DT_NORMAL|DT_TIMEDATE|DT_DMS} controls _output_ */
		char * formatstring;    /* the format string for output */
		char * ticfmt;          /* autogenerated alternative to formatstring (needed??) */
		t_timelevel timelevel;  /* minimum time unit used to quantize ticks */
		// other miscellaneous fields 
		int index;              /* if this is a permanent axis, this indexes GPO.AxS[] */
								/* (index >= PARALLEL_AXES) indexes parallel axes */
								/* (index < 0) indicates a dynamically allocated structure */
		text_label label;       /* label string and position offsets */
		bool manual_justify;    /* override automatic justification */
		lp_style_type * zeroaxis; /* usually points to default_axis_zeroaxis */
		double paxis_x;         /* x coordinate of parallel axis */
	};

	#define DEFAULT_AXIS_TICDEF {TIC_COMPUTED, NULL, {TC_DEFAULT, 0, 0.0}, {NULL, {0., 0., 0.}, FALSE}, \
		{ character, character, character, 0., 0., 0. }, FALSE, TRUE, FALSE }
	#define DEFAULT_AXIS_ZEROAXIS {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}

	#define DEFAULT_AXIS_STRUCT {                                               \
		AUTOSCALE_BOTH, AUTOSCALE_BOTH, /* auto, set_auto */                \
		0,              /* range_flags for autoscaling */           \
		-10.0, 10.0,    /* 3 pairs of min/max for axis itself */    \
		-10.0, 10.0,                                                        \
		-10.0, 10.0,                                                        \
		0.0,  0.0,      /* and another min/max for the data */      \
		CONSTRAINT_NONE, CONSTRAINT_NONE, /* min and max constraints */    \
		0., 0., 0., 0., /* lower and upper bound for min and max */ \
		0, 0,           /* terminal lower and upper coords */       \
		0.,             /* terminal scale */                        \
		0,              /* zero axis position */                    \
		FALSE, 10.0, 0.0, /* log, base, log(base) */                  \
		NULL, NULL,     /* linked_to_primary, linked_to_secondary */ \
		NULL,           /* link function */                         \
		NO_TICS,        /* tic output positions (border, mirror) */ \
		DEFAULT_AXIS_TICDEF, /* tic series definition */                 \
		0, CENTRE,      /* tic_rotate, horizontal justification */  \
		FALSE, FALSE,   /* grid{major,minor} */                     \
		MINI_DEFAULT, 10., /* minitics, mtic_freq */                   \
		1.0, 0.5, 0.0, TRUE, /* ticscale, miniticscale, ticstep, TicIn */ \
		DT_NORMAL, DT_NORMAL, /* datatype for input, output */            \
		NULL, NULL,     /* output format, another output format */  \
		0,              /* timelevel */                             \
		0,              /* index (e.g.FIRST_Y_AXIS) */              \
		EMPTY_LABELSTRUCT, /* axis label */                            \
		FALSE,          /* override automatic justification */      \
		NULL            /* NULL means &default_axis_zeroaxis */     \
	}

	#define AXIS_CLONE_SIZE offsetof(GpAxis, linked_to_primary) // This much of the axis structure is cloned by the "set x2range link" command 
	// 
	// Table of default behaviours --- a subset of the struct above. Only
	// those fields are present that differ from axis to axis. 
	// 
	struct AXIS_DEFAULTS {
		double min;     // default axis endpoints 
		double max;
		char   name[4]; // axis name, like in "x2" or "t" 
		int    ticmode; // tics on border/axis? mirrored? 
	};
	//
	// global variables in axis.c 
	//
	// extern GpAxis axis_array_Removed[AXIS_ARRAY_SIZE];
	//extern GpAxis * shadow_axis_array_Removed;
	extern const AXIS_DEFAULTS axis_defaults[AXIS_ARRAY_SIZE];
	extern const GpAxis default_axis_state;

	// Dynamic allocation of parallel axis structures 
	//extern GpAxis * parallel_axis_array_Removed;
	//extern int num_parallel_axes_Removed;

	// A parsing table for mapping axis names into axis indices. For use by the set/show machinery, mainly 
	extern const gen_table axisname_tbl[];
	extern const t_ticdef default_axis_ticdef;

	// default format for tic mark labels 
	#define DEF_FORMAT "% h"
	#define DEF_FORMAT_LATEX "$%h$"
	#define TIMEFMT "%d/%m/%y,%H:%M" // default parse timedata string 
	extern char * P_TimeFormat;

	extern const text_label default_axis_label; // axis labels 
	extern const lp_style_type default_axis_zeroaxis; // zeroaxis linetype (flag type==-3 if none wanted) 
	extern const lp_style_type default_grid_lp; // default grid linetype, to be used by 'unset grid' and 'reset' 
	extern int  grid_layer; // grid layer: LAYER_BEHIND LAYER_BACK LAYER_FRONT 
	extern bool grid_tics_in_front; // Whether to draw the axis tic labels and tic marks in front of everything else 
	extern bool grid_vertical_lines; // Whether to draw vertical grid lines in 3D 
	extern bool grid_spiderweb; // Whether to draw a grid in spiderplots 
	extern bool raxis; // Whether or not to draw a separate polar axis in polar mode 

	// global variables for communication with the tic callback functions 
	// FIXME HBB 20010806: had better be collected into a struct that's passed to the callback 
	extern int tic_start, tic_direction, tic_mirror;
	// These are for passing on to write_multiline(): 
	extern int tic_text;
	extern int rotate_tics;
	extern /*int*/JUSTIFY tic_hjust;
	extern /*int*/VERT_JUSTIFY tic_vjust;
	// The remaining ones are for grid drawing; controlled by 'set grid': 
	// extern int grid_selection; --- comm'ed out, HBB 20010806 
	extern lp_style_type grid_lp; /* linestyle for major grid lines */
	extern lp_style_type mgrid_lp; /* linestyle for minor grid lines */
	extern double polar_grid_angle; /* angle step in polar grid in radians */
	extern double theta_origin;     /* 0 = right side of plot */
	extern double theta_direction;  /* 1 = counterclockwise -1 = clockwise */
	extern int    widest_tic_strlen; /* Length of the longest tics label, set by widest_tic_callback(): */
	extern bool   inside_zoom; /* flag to indicate that in-line axis ranges should be ignored */

	class GpAxisSet {
	public:
		GpAxisSet() : P_ShadowAxArray(0), P_ParallelAxArray(0), NumParallelAxes(0), Idx_X(FIRST_X_AXIS), Idx_Y(FIRST_Y_AXIS), Idx_Z(FIRST_Z_AXIS)
		{
		}
		GpAxis & operator [](size_t idx) { return AxArray[idx]; }
		GpAxis & Theta() { return ThetaAx; }
		GpAxis & Parallel(int idx) { return P_ParallelAxArray[idx]; }
		GpAxis & __X() { return AxArray[Idx_X]; } // #define X_AXIS  GPO.AxS[GPO.AxS.Idx_X]
		GpAxis & __Y() { return AxArray[Idx_Y]; } // #define Y_AXIS  GPO.AxS[GPO.AxS.Idx_Y]
		GpAxis & __Z() { return AxArray[Idx_Z]; } // #define Z_AXIS  GPO.AxS[GPO.AxS.Idx_Z]
		GpAxis & __R() { return AxArray[POLAR_AXIS]; } // #define R_AXIS  GPO.AxS[POLAR_AXIS]
		GpAxis & __CB() { return AxArray[COLOR_AXIS]; } // #define GPO.AxS.__CB() GPO.AxS[COLOR_AXIS]

		bool   HasParallel() const { return LOGIC(P_ParallelAxArray); }
		int    GetParallelAxisCount() const { return NumParallelAxes; }
		void   ExtendParallelAxis(int);
		GpAxis * GetShadowAxis(GpAxis * pAxis);
		void   DestroyShadowAxes();
		void   DestroyParallelAxes();
		void   FASTCALL InitSampleRange(const GpAxis * pAxis, enum PLOT_TYPE plot_type);
		void   FASTCALL CheckRange(AXIS_INDEX idx);
		double MapX(double value) const;
		double MapY(double value) const;
		int    MapiX(double value) const;
		int    MapiY(double value) const;
		void   SaveAxisFormat(FILE * fp, AXIS_INDEX axis) const;
		bool   ValidateData(double v, AXIS_INDEX ax) const;

		AXIS_INDEX Idx_X; // axes being used by the current plot 
		AXIS_INDEX Idx_Y; // axes being used by the current plot 
		AXIS_INDEX Idx_Z; // axes being used by the current plot 
	private:
		GpAxis AxArray[AXIS_ARRAY_SIZE]; // axis_array
		GpAxis ThetaAx;           //
		GpAxis * P_ShadowAxArray; // shadow_axis_array
		GpAxis * P_ParallelAxArray; // parallel_axis_array
		int    NumParallelAxes;     // num_parallel_axes
	};
	//
	// -------- macros using these variables:
	//
	/* parse a position of the form
	 *    [coords] x, [coords] y {,[coords] z}
	 * where coords is one of first,second.graph,screen,character
	 * if first or second, we need to take axis.datatype into account
	 * FIXME: Cannot handle parallel axes
	 */
	#define GET_NUMBER_OR_TIME(store, axes, axis)                             \
		do {                                                                    \
			const GpAxis * p_this_axis_ = (axes == NO_AXIS) ? NULL : &(GPO.AxS[(axes)+(axis)]); \
			(store) = get_num_or_time(p_this_axis_);                               \
		} while(0)

	/*
	 * Gradually replacing extremely complex macro ACTUAL_STORE_AND_UPDATE_RANGE
	 * (called 50+ times) with a subroutine. The original logic was that in-line
	 * code was faster than calls to a subroutine, but on current hardware it is
	 * better to have one cached copy than to have 50 separate uncached copies.
	 *
	 * The difference between STORE_AND_UPDATE_RANGE and store_and_update_range
	 * is that the former takes an axis index and the latter an axis pointer.
	 */
	#define STORE_AND_UPDATE_RANGE(STORE, VALUE, TYPE, AXIS, NOAUTOSCALE, UNDEF_ACTION)      \
		if(AXIS != NO_AXIS) do { \
				if(store_and_update_range(&(STORE), VALUE, &(TYPE), (&GPO.AxS[AXIS]), NOAUTOSCALE) == UNDEFINED) { \
					UNDEF_ACTION; \
				} \
			} while(0)

	#define NOOP ((void)0) /* Use NOOP for UNDEF_ACTION if no action is wanted */

	// HBB 20000506: new macro to automatically build initializer lists for arrays of AXIS_ARRAY_SIZE=11 equal elements 
	#define AXIS_ARRAY_INITIALIZER(value) { value, value, value, value, value, value, value, value, value, value, value }

	#define SIGNIF (0.01) // 'roundoff' check tolerance: less than one hundredth of a tic mark 
	/* (DFK) Watch for cancellation error near zero on axes labels */
	/* FIXME HBB 20000521: these seem not to be used much, anywhere... */
	#define CheckZero(x, tic) (fabs(x) < ((tic) * SIGNIF) ? 0.0 : (x))
	//
	// Function pointer type for callback functions to generate ticmarks 
	//
	typedef void (* tic_callback)(GpAxis *, double, char *, int, lp_style_type, ticmark *);

	/* ------------ functions exported by axis.c */
	coord_type store_and_update_range(double * store, double curval, coord_type * type, GpAxis * axis, bool noautoscale);
	void   check_log_limits(const GpAxis *, double, double);
	void   axis_invert_if_requested(GpAxis *);
	void   FASTCALL axis_init(GpAxis * this_axis, bool infinite);
	void   FASTCALL axis_check_empty_nonlinear(const GpAxis * this_axis);
	char * copy_or_invent_formatstring(GpAxis *);
	double quantize_normal_tics(double, int);
	void   setup_tics(GpAxis *, int);
	void   axis_set_scale_and_range(GpAxis * axis, int lower, int upper);
	bool   some_grid_selected();
	void   add_tic_user(GpAxis *, char *, double, int);
	double FASTCALL get_num_or_time(const GpAxis *);
	void   check_axis_reversed(AXIS_INDEX axis);

	/* set widest_tic_label: length of the longest tics label */
	void   widest_tic_callback(GpAxis *, double place, char * text, int ticlevel, struct lp_style_type grid, struct ticmark *);
	void   gstrdms(char * label, char * format, double value);
	void   save_autoscaled_ranges(const GpAxis *, const GpAxis *);
	void   restore_autoscaled_ranges(GpAxis *, GpAxis *);
	char * FASTCALL axis_name(AXIS_INDEX);
	void   init_parallel_axis(GpAxis *, AXIS_INDEX);
	// For debugging 
	void   dump_axis_range(GpAxis * axis);

	// macro for tic scale, used in all tic_callback functions 
	#define tic_scale(ticlevel, axis) (ticlevel <= 0 ? axis->ticscale : ticlevel == 1 ? axis->miniticscale : ticlevel < MAX_TICLEVEL ? ticscale[ticlevel] : 0)
	// convenience macro to make sure min < max 
	#define reorder_if_necessary(min, max) do { if(max < min) { double temp = min; min = max; max = temp; } } while(0)
//
//#include <command.h>
	extern char * gp_input_line;
	extern size_t gp_input_line_len;
	extern int    inline_num;
	extern int    if_depth;         // old if/else syntax only 
	extern bool   if_open_for_else; // new if/else syntax only 

	struct lexical_unit {	/* produced by scanner */
		bool is_token;		/* true if token, false if a value */
		GpValue l_val;
		int start_index;		/* index of first char in token */
		int length;			/* length of token in chars */
	};

	extern char * replot_line;

	/* flag to disable `replot` when some data are sent through stdin;
	 * used by mouse/hotkey capable terminals */
	extern bool replot_disabled;

	#ifdef USE_MOUSE
		extern int paused_for_mouse;	/* Flag the end condition we are paused until */
		#define PAUSE_BUTTON1   001		/* Mouse button 1 */
		#define PAUSE_BUTTON2   002		/* Mouse button 2 */
		#define PAUSE_BUTTON3   004		/* Mouse button 3 */
		#define PAUSE_CLICK	007		/* Any button click */
		#define PAUSE_KEYSTROKE 010		/* Any keystroke */
		#define PAUSE_WINCLOSE	020		/* Window close event */
		#define PAUSE_ANY       077		/* Terminate on any of the above */
	#endif

	/* output file for the print command */
	extern FILE * print_out;
	extern udvt_entry * print_out_var;
	extern char * print_out_name;
	extern udft_entry * dummy_func;

	#ifndef STDOUT
		#define STDOUT 1
	#endif
	#ifdef _WIN32
		#define SET_CURSOR_WAIT SetCursor(LoadCursor((HINSTANCE) NULL, IDC_WAIT))
		#define SET_CURSOR_ARROW SetCursor(LoadCursor((HINSTANCE) NULL, IDC_ARROW))
	#else
		#define SET_CURSOR_WAIT        /* nought, zilch */
		#define SET_CURSOR_ARROW       /* nought, zilch */
	#endif
	//
	// input data, parsing variables 
	//
	//extern lexical_unit * token;
	//extern int token_table_size;
	extern int plot_token;
	//#define END_OF_COMMAND_Removed (c_token >= num_tokens || GPO.Pgm.Equals(c_token,";"))
	//extern int num_tokens;
	//extern int c_token; 

	enum ifstate {
		IF_INITIAL = 1, 
		IF_TRUE, 
		IF_FALSE
	};

	class GpProgram {
	public:
		GpProgram() : CToken(0), NumTokens(0), P_Token(0), TokenTableSize(0)
		{
		}
		size_t TokenLen(int t_num) const { return (size_t)(P_Token[t_num].length); }
		size_t CurTokenLen() const { return (size_t)(P_Token[CToken].length); }
		int    GetCurTokenLength() const { return P_Token[CToken].length; }
		int    GetCurTokenStartIndex() const { return P_Token[CToken].start_index; }
		int    GetCurTokenIdx() const { return CToken; }
		int    GetPrevTokenIdx() const { return (CToken-1); }
		bool   EndOfCommand() const { return (CToken >= NumTokens || Equals(CToken, ";")); }
		int    Equals(int t_num, const char * pStr) const;
		int    EqualsCur(const char * pStr) const { return Equals(CToken, pStr); }
		//
		// Descr: Проверяет равенство текущего токена строке pStr и, НЕ ЗАВИСИМО ОТ РЕЗУЛЬТАТА, смещает 
		//   текущую позицию токена вперед.
		//
		int    EqualsCurShift(const char * pStr) { return Equals(CToken++, pStr); }
		int    EqualsNext(const char * pStr) const { return Equals(CToken+1, pStr); }
		int    FASTCALL IsANumber(int tokN) const { return (!P_Token[tokN].is_token); }
		int    FASTCALL IsString(int t_num) const;
		bool   FASTCALL IsStringValue(int tokN) const { return (IsString(tokN) || TypeUdv(tokN) == STRING); }
		int    FASTCALL TypeUdv(int t_num) const;
		int    FASTCALL LookupTableForCurrentToken(const struct gen_table * tbl) const;
		int    AlmostEquals(int t_num, const char * pStr) const;
		int    AlmostEqualsCur(const char * pStr) const { return AlmostEquals(CToken, pStr); }
		void   SetTokenIdx(int tokN) { CToken = tokN; }
		void   ParseSkipRange();
		char * ParseDatablockName();
		void   BreakCommand();
		void   ContinueCommand();
		void   BindCommand();
		void   UndefineCommand();
		int    IsLetter(int t_num) const;
		void   OldIfCommand(at_type * expr);
		void   HelpCommand();
		void   DatablockCommand();
		int    Scanner(char ** ppExpression, size_t * pExpressionLen);
		void   ExtendTokenTable();
		int    ReadLine(const char * pPrompt, int start);
		int    DoLine();
		void   LoadFile(FILE * fp, char * pName, int calltype);
		void   CopyStr(char * pStr, int tokNum, int maxCount) const;
		void   Capture(char * pStr, int start, int end, int max) const;
		void   MCapture(char ** ppStr, int start, int end);
		void   MQuoteCapture(char ** ppStr, int start, int end);
		char * TokenToString(int tokN) const;
		int    FindClause(int * pClauseStart, int * pClauseEnd);
		int    Shift() { return CToken++; }
		int    Rollback() { return CToken--; }
		int    CToken;
		int    NumTokens;
		int    TokenTableSize;
		lexical_unit * P_Token;
	private:
		int    FASTCALL GetNum(char pStr[]);
	};

	void raise_lower_command(int);
	void raise_command();
	void lower_command();
	#ifdef X11
		extern void x11_raise_terminal_window(int);
		extern void x11_raise_terminal_group();
		extern void x11_lower_terminal_window(int);
		extern void x11_lower_terminal_group();
	#endif
	#ifdef _WIN32
		extern void win_raise_terminal_window(int);
		extern void win_raise_terminal_group();
		extern void win_lower_terminal_window(int);
		extern void win_lower_terminal_group();
	#endif
	#ifdef WXWIDGETS
		extern void wxt_raise_terminal_window(int);
		extern void wxt_raise_terminal_group();
		extern void wxt_lower_terminal_window(int);
		extern void wxt_lower_terminal_group();
	#endif
	extern void string_expand_macros();
	#ifdef USE_MOUSE
		//void bind_command();
		void restore_prompt();
	#else
		//#define bind_command()
	#endif
	void call_command();
	void changedir_command();
	void eval_command();
	void invalid_command();
	void load_command();
	void begin_clause();
	void clause_reset_after_error();
	void end_clause();
	void null_command();
	void printerr_command();
	void pwd_command();
	void reread_command();
	void screendump_command();
	void stats_command();
	void system_command();
	void test_command();
	void toggle_command();
	void update_command();
	void do_shell();
	//
	// Prototypes for functions exported by command.c 
	//
	void extend_input_line();
	void do_string(const char* s);
	bool iteration_early_exit();
	#ifdef USE_MOUSE
		void toggle_display_of_ipc_commands();
		int  display_ipc_commands();
	#endif
	void   print_set_output(char *, bool, bool); /* set print output file */
	char * print_show_output(); /* show print output file */
	int    do_system_func(const char *cmd, char **output);
//
//#include <variable.h>
	//
	// The death of global variables - part one. 
	//
	//
	// Generic defines 
	//
	#ifdef ACTION_NULL
		#undef ACTION_NULL
	#endif
	#ifdef ACTION_INIT
		#undef ACTION_INIT
	#endif
	#ifdef ACTION_SHOW
		#undef ACTION_SHOW
	#endif
	#ifdef ACTION_SET
		#undef ACTION_SET
	#endif
	#ifdef ACTION_GET
		#undef ACTION_GET
	#endif
	#ifndef ACTION_SAVE
		#undef ACTION_SAVE
	#endif
	#ifdef ACTION_CLEAR
		#undef ACTION_CLEAR
	#endif

	#define ACTION_NULL   0
	#define ACTION_INIT   (1<<0)
	#define ACTION_SHOW   (1<<1)
	#define ACTION_SET    (1<<2)
	#define ACTION_GET    (1<<3)
	#define ACTION_SAVE   (1<<4)
	#define ACTION_CLEAR  (1<<5)
	//
	// Loadpath related 
	//
	char *loadpath_handler(int, char *);

	#define init_loadpath()    loadpath_handler(ACTION_INIT,NULL)
	#define set_var_loadpath(path) loadpath_handler(ACTION_SET,(path))
	#define get_loadpath()     loadpath_handler(ACTION_GET,NULL)
	#define save_loadpath()    loadpath_handler(ACTION_SAVE,NULL)
	#define clear_loadpath()   loadpath_handler(ACTION_CLEAR,NULL)
	#define dump_loadpath()    loadpath_handler(ACTION_SHOW,NULL)
	//
	// Locale related 
	//
	char *locale_handler(int, char *);

	#define INITIAL_LOCALE ("C")
	#define init_locale()      locale_handler(ACTION_INIT,NULL)
	#define set_var_locale(path)   locale_handler(ACTION_SET,(path))
	#define get_time_locale()       locale_handler(ACTION_GET,NULL)
	#define dump_locale()      locale_handler(ACTION_SHOW,NULL)

	#ifdef HAVE_LOCALE_H
		#define set_numeric_locale() do { if(numeric_locale && strcmp(numeric_locale,"C")) setlocale(LC_NUMERIC,numeric_locale);} while(0)
		#define reset_numeric_locale() do { if(numeric_locale && strcmp(numeric_locale,"C")) setlocale(LC_NUMERIC,"C");} while(0)
		#define get_decimal_locale() (localeconv()->decimal_point)
	#else
		#define set_numeric_locale()
		#define reset_numeric_locale()
		#define get_decimal_locale() "."
	#endif
	extern char full_month_names[12][32];
	extern char abbrev_month_names[12][8];
	extern char full_day_names[7][32];
	extern char abbrev_day_names[7][8];
//
//#include <bitmap.h>
	// allow up to 16 bit width for character array 
	typedef uint char_row;
	typedef char_row const * char_box;

	#define FNT_CHARS   96          /* Number of characters in the font set */

	#define FNT5X9 0
	#define FNT5X9_VCHAR 11         /* vertical spacing between characters */
	#define FNT5X9_VBITS 9          /* actual number of rows of bits per char */
	#define FNT5X9_HCHAR 7          /* horizontal spacing between characters */
	#define FNT5X9_HBITS 5          /* actual number of bits per row per char */
	extern const char_row fnt5x9[FNT_CHARS][FNT5X9_VBITS];

	#define FNT9X17 1
	#define FNT9X17_VCHAR 21        /* vertical spacing between characters */
	#define FNT9X17_VBITS 17        /* actual number of rows of bits per char */
	#define FNT9X17_HCHAR 13        /* horizontal spacing between characters */
	#define FNT9X17_HBITS 9         /* actual number of bits per row per char */
	extern const char_row fnt9x17[FNT_CHARS][FNT9X17_VBITS];

	#define FNT13X25 2
	#define FNT13X25_VCHAR 31       /* vertical spacing between characters */
	#define FNT13X25_VBITS 25       /* actual number of rows of bits per char */
	#define FNT13X25_HCHAR 19       /* horizontal spacing between characters */
	#define FNT13X25_HBITS 13       /* actual number of bits per row per char */
	extern const char_row fnt13x25[FNT_CHARS][FNT13X25_VBITS];
	typedef uchar pixels;  /* the type of one set of 8 pixels in bitmap */
	typedef pixels * bitmap[];       /* the bitmap */
	extern bitmap * b_p;             /* global pointer to bitmap */
	extern uint b_xsize, b_ysize; /* the size of the bitmap */
	extern uint b_planes;   /* number of color planes */
	extern uint b_psize;    /* size of each plane */
	extern uint b_rastermode; /* raster mode rotates -90deg */
	extern uint b_linemask; /* 16 bit mask for dotted lines */
	extern uint b_angle;    /* rotation of text */
	extern int b_maskcount;
	//
	// Prototypes from file "bitmap.c" 
	//
	uint b_getpixel(uint, uint);
	void b_makebitmap(uint, uint, uint);
	void b_freebitmap();
	void b_charsize(uint);
	void b_setvalue(uint);
	void b_setlinetype(int);
	void b_linewidth(double linewidth);
	void b_move(uint, uint);
	void b_vector(uint, uint);
	void b_put_text(uint, uint, const char *);
	int  b_text_angle(int);
	void b_boxfill(int, uint, uint, uint, uint);
	void b_filled_polygon(int points, gpiPoint * corners);
//
//#include <graphics.h>
	//
	// types defined for 2D plotting 
	//
	//
	struct curve_points {
		curve_points *next;	/* pointer to next plot in linked list */
		int token;			/* last token used, for second parsing pass */
		enum PLOT_TYPE plot_type;	/* DATA2D? DATA3D? FUNC2D FUNC3D? NODATA? */
		enum PLOT_STYLE plot_style;	/* style set by "with" or by default */
		char *title;		/* plot title, a.k.a. key entry */
		GpPosition * title_position;	/* title at {beginning|end|<xpos>,<ypos>} */
		bool title_no_enhanced;	/* don't typeset title in enhanced mode */
		bool title_is_automated;/* TRUE if title was auto-generated */
		bool title_is_suppressed;/* TRUE if 'notitle' was specified */
		bool noautoscale;	/* ignore data from this plot during autoscaling */
		lp_style_type lp_properties;
		arrow_style_type arrow_properties;
		fill_style_type fill_properties;
		text_label * labels;	/* Only used if plot_style == LABELPOINTS */
		t_image image_properties;	/* only used if plot_style is IMAGE or RGB_IMAGE */
		udvt_entry *sample_var;	/* used by '+' if plot has private sampling range */
		udvt_entry *sample_var2;	/* used by '++'if plot has private sampling range */

		/* 2D and 3D plot structure fields overlay only to this point */
		filledcurves_opts filledcurves_options;
		int base_linetype;		/* before any calls to load_linetype(), lc variable */
					/* analogous to hidden3d_top_linetype in graph3d.h  */
		int ellipseaxes_units;              /* Only used if plot_style == ELLIPSES */    
		histogram_style *histogram;	/* Only used if plot_style == HISTOGRAM */
		int histogram_sequence;	/* Ordering of this dataset within the histogram */
		enum PLOT_SMOOTH plot_smooth; /* which "smooth" method to be used? */
		double smooth_parameter;	/* e.g. optional bandwidth for smooth kdensity */
		double smooth_period;	/* e.g. 2pi for a circular function */
		int boxplot_factors;	/* Only used if plot_style == BOXPLOT */
		int p_max;			/* how many points are allocated */
		int p_count;		/* count of points in points */
		AXIS_INDEX AxIdx_X/*x_axis*/; // FIRST_X_AXIS or SECOND_X_AXIS 
		AXIS_INDEX AxIdx_Y/*y_axis*/; // FIRST_Y_AXIS or SECOND_Y_AXIS 
		AXIS_INDEX AxIdx_Z/*z_axis*/; // same as either x_axis or y_axis, for 5-column plot types 
		int current_plotno; // Only used by "pn" option of linespoints 
		AXIS_INDEX AxIdx_P;/*p_axis*/;  // Only used for parallel axis plots 
		double * varcolor;  // Only used if plot has variable color 
		GpCoordinate * points;
	};
	//
	// externally visible variables of graphics.h 
	//
	// 'set offset' status variables 
	//
	extern GpPosition loff;
	extern GpPosition roff;
	extern GpPosition toff;
	extern GpPosition boff;
	extern bool retain_offsets;
	//
	// 'set bar' status 
	//
	extern double bar_size;
	extern int    bar_layer;
	extern lp_style_type bar_lp;
	extern double rgbmax; /* 'set rgbmax {0|255}' */
	//
	// function prototypes 
	//
	void init_histogram(histogram_style * hist, text_label *title);
	void free_histlist(histogram_style * hist);

	enum t_procimg_action {
		IMG_PLOT,
		IMG_UPDATE_AXES,
		IMG_UPDATE_CORNERS
	};

	int    filter_boxplot(curve_points *);
//
//#include <boundary.h>
	int find_maxl_keys(curve_points *plots, int count, int *kcnt);
	void advance_key(bool only_invert);
	bool at_left_of_key();
	//
	// Probably some of these could be made static 
	//
	extern int key_entry_height;
	extern int key_point_offset;
	extern int ylabel_x;
	extern int y2label_x;
	extern int xlabel_y;
	extern int x2label_y;
	extern int ylabel_y;
	extern int y2label_y;
	extern int xtic_y;
	extern int x2tic_y;
	extern int ytic_x;
	extern int y2tic_x;
	extern int key_cols;
	extern int key_rows;
	extern int key_count;
	extern int title_x;
	extern int title_y;
//
//#include <breaders.h>
	//
	// Prototypes of functions exported by breaders.c
	//
	void edf_filetype_function();
	void png_filetype_function();
	void gif_filetype_function();
	void jpeg_filetype_function();
	int  df_libgd_get_pixel(int i, int j, int component);
	bool df_read_pixmap(t_pixmap *pixmap);
//
//#include <getcolor.h>
	enum color_models_id {
		C_MODEL_RGB = 'r',
		C_MODEL_HSV = 'h',
		C_MODEL_CMY = 'c',
		C_MODEL_XYZ = 'x'
	};

	// main gray --> rgb color mapping 
	//void rgb1_from_gray(double gray, rgb_color * color);
	void rgb255_from_rgb1(rgb_color rgb1, rgb255_color * rgb255);
	// main gray --> rgb color mapping as above, with take care of palette maxcolors 
	//void rgb1maxcolors_from_gray(double gray, rgb_color * color);
	//void rgb255maxcolors_from_gray(double gray, rgb255_color * rgb255);
	//double quantize_gray(double gray);
	// HSV --> RGB user-visible function hsv2rgb(h,s,v) 
	uint hsv2rgb(rgb_color * color);
	// used to (de-)serialize color/gradient information 
	char * gradient_entry_to_str(gradient_struct * gs);
	void str_to_gradient_entry(char * s, gradient_struct * gs);
	// check if two palettes p1 and p2 differ 
	int palettes_differ(t_sm_palette * p1, t_sm_palette * p2);
	// construct minimal gradient to approximate palette 
	gradient_struct * approximate_palette(t_sm_palette * palette, int maxsamples, double allowed_deviation, int * gradient_num);
	double GetColorValueFromFormula(int formula, double x);
	extern const char * ps_math_color_formulae[];
//
//#include <plot.h>
	//
	// Type definitions 
	//
	// Variables of plot.c needed by other modules: 
	//
	extern bool interactive;
	extern bool noinputfiles;
	extern bool persist_cl;
	extern bool slow_font_startup;
	extern const char *user_shell;
	extern bool ctrlc_flag;
	extern bool terminate_flag;
	//
	// Prototypes of functions exported by plot.c 
	//
	#if defined(__GNUC__)
		void bail_to_command_line(void) __attribute__((noreturn));
	#else
		void bail_to_command_line();
	#endif
	//void init_constants();
	//void init_session();
	#if defined(_WIN32)
		int gnu_main(int argc, char **argv);
	#endif
	void interrupt_setup();
	void gp_expand_tilde(char **);
	void get_user_env();
	void restrict_popen();
	#ifdef GNUPLOT_HISTORY
		void cancel_history();
	#else
		#define cancel_history()  {}
	#endif
//
//#include <plot2d.h>
	// This allows a natural interpretation of providing only a single column in 'using' 
	#define default_smooth_weight(option) (oneof3(option, SMOOTH_BINS, SMOOTH_KDENSITY, SMOOTH_FREQUENCY))

	extern curve_points * P_FirstPlot; // Variables of plot2d.c needed by other modules: 

	// prototypes from plot2d.c 
	//void plotrequest();
	//void refresh_bounds(curve_points *first_plot, int nplots);
	// internal and external variables 
	void cp_extend(curve_points *cp, int num);
	text_label * store_label(text_label *, GpCoordinate *, int i, char * string, double colorval);
	//void parse_plot_title(curve_points *this_plot, char *xtitle, char *ytitle, bool *set_title);
	void reevaluate_plot_title(curve_points * this_plot);
//
//#include <plot3d.h>
	//
	// typedefs of plot3d.c 
	//
	enum t_data_mapping {
		MAP3D_CARTESIAN,
		MAP3D_SPHERICAL,
		MAP3D_CYLINDRICAL
	};
	//
	// Variables of plot3d.c needed by other modules: 
	//
	extern struct surface_points *first_3dplot;
	extern int plot3d_num;
	extern t_data_mapping mapping3d;
	extern int dgrid3d_row_fineness;
	extern int dgrid3d_col_fineness;
	extern int dgrid3d_norm_value;
	extern int dgrid3d_mode;
	extern double dgrid3d_x_scale;
	extern double dgrid3d_y_scale;
	extern bool	dgrid3d;
	extern bool dgrid3d_kdensity;
	extern double boxdepth;
	//
	// prototypes from plot3d.c 
	//
	//void plot3drequest();
	//void refresh_3dbounds(termentry * pTerm, surface_points * pFirstPlot, int nplots);
	void sp_free(struct surface_points *sp);
//
//#include <graph3d.h>
	//
	// Type definitions 
	//
	enum t_dgrid3d_mode {
		DGRID3D_DEFAULT,
		DGRID3D_QNORM,
		DGRID3D_SPLINES,
		DGRID3D_GAUSS,
		DGRID3D_EXP,
		DGRID3D_CAUCHY,
		DGRID3D_BOX,
		DGRID3D_HANN,
		DGRID3D_OTHER
	};

	enum t_contour_placement {
		/* Where to place contour maps if at all. */
		CONTOUR_NONE,
		CONTOUR_BASE,
		CONTOUR_SRF,
		CONTOUR_BOTH
	};

	typedef double transform_matrix[4][4]; /* HBB 990826: added */

	struct gnuplot_contours {
		gnuplot_contours * next;
		GpCoordinate * coords;
		char isNewLevel;
		char label[32];
		int num_pts;
		double z;
	};

	struct iso_curve {
		iso_curve * next;
		int    p_max;      // how many points are allocated 
		int    p_count;    // count of points in points 
		GpCoordinate * points;
	};

	struct surface_points {
		surface_points * next_sp; /* pointer to next plot in linked list */
		int token;              /* last token used, for second parsing pass */
		enum PLOT_TYPE plot_type; /* DATA2D? DATA3D? FUNC2D FUNC3D? NODATA? */
		enum PLOT_STYLE plot_style; /* style set by "with" or by default */
		char * title;           /* plot title, a.k.a. key entry */
		GpPosition * title_position; /* title at {beginning|end|<xpos>,<ypos>} */
		bool title_no_enhanced; /* don't typeset title in enhanced mode */
		bool title_is_automated;/* TRUE if title was auto-generated */
		bool title_is_suppressed;/* TRUE if 'notitle' was specified */
		bool noautoscale; /* ignore data from this plot during autoscaling */
		lp_style_type lp_properties;
		arrow_style_type arrow_properties;
		fill_style_type fill_properties;
		text_label * labels; /* Only used if plot_style == LABELPOINTS */
		struct t_image image_properties; /* only used if plot_style is IMAGE, RGBIMAGE or RGBA_IMAGE */
		udvt_entry * sample_var; /* used by '+' if plot has private sampling range */
		udvt_entry * sample_var2; /* used by '++' if plot has private sampling range */

		/* 2D and 3D plot structure fields overlay only to this point */

		enum PLOT_SMOOTH plot_smooth; /* EXPERIMENTAL: smooth lines in 3D */
		bool opt_out_of_hidden3d; /* set by "nohidden" option to splot command */
		bool opt_out_of_contours; /* set by "nocontours" option to splot command */
		bool opt_out_of_surface; /* set by "nosurface" option to splot command */
		bool pm3d_color_from_column;
		bool has_grid_topology;
		int hidden3d_top_linetype; /* before any calls to load_linetype() */
		int iteration;          /* needed for tracking iteration */
		struct vgrid * vgrid;   /* used only for voxel plots */
		double iso_level;       /* used only for voxel plots */
		/* Data files only - num of isolines read from file. For functions,  */
		/* num_iso_read is the number of 'primary' isolines (in x direction) */
		int num_iso_read;
		gnuplot_contours * contours; /* NULL if not doing contours. */
		iso_curve * iso_crvs; /* the actual data */
		char pm3d_where[7];     /* explicitly given base, top, surface */
	};
	//
	// Variables of graph3d.c needed by other modules:
	//
	extern int xmiddle, ymiddle, xscaler, yscaler;
	extern double floor_z, floor_z1;
	extern double ceiling_z, base_z; /* made exportable for PM3D */
	extern transform_matrix trans_mat;
	//extern double xscale3d;
	//extern double yscale3d;
	//extern double zscale3d;
	extern SPoint3R Scale3D;
	//extern double xcenter3d;
	//extern double ycenter3d;
	//extern double zcenter3d;
	extern SPoint3R Center3D;
	extern double radius_scaler;
	extern t_contour_placement draw_contour;
	extern bool clabel_onecolor;
	extern int clabel_start;
	extern int clabel_interval;
	extern char * clabel_font;
	extern bool draw_surface;
	extern bool implicit_surface;
	extern bool hidden3d; /* is hidden3d display wanted? */
	extern int hidden3d_layer;      /* LAYER_FRONT or LAYER_BACK */
	extern float surface_rot_z;
	extern float surface_rot_x;
	extern float surface_scale;
	extern float surface_zscale;
	extern float surface_lscale;
	extern float mapview_scale;
	extern float azimuth;
	extern bool splot_map, xz_projection, yz_projection;
	extern bool in_3d_polygon;

	struct t_xyplane {
		double z;
		bool absolute;
	};

	extern t_xyplane xyplane;

	#define ISO_SAMPLES 10          /* default number of isolines per splot */
	extern int iso_samples_1;
	extern int iso_samples_2;

	#ifdef USE_MOUSE
		extern int axis3d_o_x;
		extern int axis3d_o_y;
		extern int axis3d_x_dx;
		extern int axis3d_x_dy;
		extern int axis3d_y_dx;
		extern int axis3d_y_dy;
	#endif

	enum REPLOT_TYPE {
		NORMAL_REPLOT = 0, /* e.g. "replot" command */
		AXIS_ONLY_ROTATE, /* suppress replots during 3D rotation by ctrl-left-mouse */
		NORMAL_REFRESH, /* e.g. "refresh" command */
		QUICK_REFRESH   /* auto-generated refresh during 3D rotation */
	};
//
//#include <pm3d.h>
	#ifndef TERM_HELP
		// 
		// Global options for pm3d algorithm (to be accessed by set / show)
		// 
		// 
		// where to plot pm3d: base or top (color map) or surface (color surface)
		// The string pm3d.where can be any combination of the #defines below.
		// For instance, "b" plot at bottom only, "st" plots firstly surface, then top, etc.
		// 
		#define PM3D_AT_BASE    'b'
		#define PM3D_AT_TOP     't'
		#define PM3D_AT_SURFACE 's'
		// 
		// options for flushing scans (for pm3d.flush)
		// Note: new terminology compared to my pm3d program; in gnuplot it became
		// begin and right instead of left and right
		// 
		#define PM3D_FLUSH_BEGIN   'b'
		#define PM3D_FLUSH_END     'r'
		#define PM3D_FLUSH_CENTER  'c'
		// 
		// direction of taking the scans: forward = as the scans are stored in the
		// file; backward = opposite direction, i.e. like from the end of the file
		// 
		enum pm3d_scandir {
			PM3D_SCANS_AUTOMATIC,
			PM3D_SCANS_FORWARD,
			PM3D_SCANS_BACKWARD,
			PM3D_DEPTH
		};
		// 
		// clipping method:
		// PM3D_CLIP_1IN: all 4 points of the quadrangle must be defined and at least
		//   1 point of the quadrangle must be in the x and y ranges
		// PM3D_CLIP_4IN: all 4 points of the quadrangle must be in the x and y ranges
		// PM3D_CLIP_Z:   smooth clip to current zrange
		// 
		#define PM3D_CLIP_Z 0
		#define PM3D_CLIP_1IN '1'
		#define PM3D_CLIP_4IN '4'
		// 
		// is pm3d plotting style implicit or explicit?
		// 
		enum PM3D_IMPL_MODE {
			PM3D_EXPLICIT = 0,
			PM3D_IMPLICIT = 1
		};
		// 
		// from which corner take the color?
		// 
		enum PM3D_WHICH_CORNERS2COLOR {
			/* keep the following order of PM3D_WHICHCORNER_C1 .. _C4 */
			PM3D_WHICHCORNER_C1 = 0, /* corner 1: first scan, first point   */
			PM3D_WHICHCORNER_C2 = 1, /* corner 2: first scan, second point  */
			PM3D_WHICHCORNER_C3 = 2, /* corner 3: second scan, first point  */
			PM3D_WHICHCORNER_C4 = 3, /* corner 4: second scan, second point */
			/* the rest can be in any order */
			PM3D_WHICHCORNER_MEAN    = 4,/* average z-value from all 4 corners */
			PM3D_WHICHCORNER_GEOMEAN = 5, /* geometrical mean of 4 corners */
			PM3D_WHICHCORNER_HARMEAN = 6, /* harmonic mean of 4 corners */
			PM3D_WHICHCORNER_MEDIAN  = 7,/* median of 4 corners */
			PM3D_WHICHCORNER_RMS     = 8,/* root mean square of 4 corners*/
			PM3D_WHICHCORNER_MIN     = 9,/* minimum of 4 corners */
			PM3D_WHICHCORNER_MAX     = 10,/* maximum of 4 corners */
			PM3D_COLOR_BY_NORMAL     = 11/* derive color from surface normal (not currently used) */
		};
		// 
		// structure defining all properties of pm3d plotting mode
		// (except for the properties of the smooth color box, see color_box instead)
		// 
		struct pm3d_struct {
			char where[7];  /* base, top, surface */
			char flush;     /* left, right, center */
			char ftriangles; /* 0/1 (don't) draw flushing triangles */
			char clip;      /* 1in, 4in */
			bool no_clipcb; /* FALSE: out-of-range cb treated as 0  TRUE: treated as NaN */
			pm3d_scandir direction;
			/* If direction is "depth" sort by max z of 4 corners unless ... */
			bool base_sort; /*     use z values of projection to baseplane */
			bool zmean_sort; /*     sort on mean z rather than max z */
			PM3D_IMPL_MODE implicit;
			/* 1: [default] draw ALL surfaces with pm3d
			   0: only surfaces specified with 'with pm3d' */
			PM3D_WHICH_CORNERS2COLOR which_corner_color;
			/* default: average color from all 4 points */
			int interp_i;   /* # of interpolation steps along scanline */
			int interp_j;   /* # of interpolation steps between scanlines */
			lp_style_type border; /* LT_NODRAW to disable.  From `set pm3d border <linespec> */
		};

		extern pm3d_struct pm3d;

		struct lighting_model {
			double strength; /* 0 = no lighting model; 1 = full shading */
			double spec;    /* specular component 0-1 */
			double ambient; /* ambient component 0-1 */
			double Phong;   /* Phong exponent */
			int rot_z;      /* illumination angle */
			int rot_x;      /* illumination angle */
			bool fixed; /* TRUE means the light does not rotate */
			double spec2;   /* 2nd specular contribution from red spotlight on opposite side */
		};

		extern lighting_model pm3d_shade;
		extern struct lp_style_type default_pm3d_border; /* Used to initialize `set pm3d border` */
		extern bool track_pm3d_quadrangles; /* Set by plot styles that use pm3d quadrangles even in non-pm3d mode */
		// 
		// Declaration of routines
		// 
		//int    get_pm3d_at_option(char * pm3d_where);
		void   pm3d_depth_queue_clear();
		//void   pm3d_depth_queue_flush();
		void   pm3d_reset();
		//void   pm3d_draw_one(struct surface_points* plots);
		void   pm3d_add_quadrangle(struct surface_points* plot, gpdPoint * corners);
		void   pm3d_add_polygon(struct surface_points* plot, gpdPoint * corners, int vertices);
		void   pm3d_init_lighting_model();
		int    pm3d_side(GpCoordinate * p0, GpCoordinate * p1, GpCoordinate * p2);
		//double cb2gray(double cb);
		void   pm3d_rearrange_scan_array(surface_points* this_plot, iso_curve*** first_ptr, int* first_n, int* first_invert, iso_curve*** second_ptr, int* second_n, int* second_invert);
		//void   set_plot_with_palette(int plot_num, int plot_mode);
		void   pm3d_reset_after_error();
		bool   is_plot_with_palette();
		bool   is_plot_with_colorbox();
	#endif /* TERM_HELP */
//
//#include <misc.h>
	//
	// Variables of misc.c needed by other modules:
	//
	extern char * loadpath_fontname; /* Used by postscript terminal if a font file is found by loadpath_fopen() */
	/* these two are global so that plot.c can load them on program entry */
	extern char * call_args[10];
	extern int call_argc;
	//
	// Prototypes from file "misc.c" 
	//
	const char * expand_call_arg(int c);
	//void load_file(FILE * fp, char * name, int calltype);
	FILE * lf_top();
	//bool lf_pop();
	//void lf_push(FILE * fp, char * name, char * cmdline);
	void load_file_error();
	FILE * loadpath_fopen(const char *, const char *);
	void push_terminal(int is_interactive);
	void pop_terminal();
	/* moved here, from setshow */
	enum PLOT_STYLE get_style();
	void get_filledcurves_style_options(filledcurves_opts *);
	void filledcurves_options_tofile(filledcurves_opts *, FILE *);
	//int  lp_parse(lp_style_type * lp, lp_class destination_class, bool allow_point);
	//void arrow_parse(arrow_style_type *, bool);
	void arrow_use_properties(arrow_style_type * arrow, int tag);
	//void parse_fillstyle(fill_style_type * fs);
	//void parse_colorspec(t_colorspec * tc, int option);
	long lookup_color_name(char * string);
	long parse_color_name();
	bool FASTCALL need_fill_border(const fill_style_type * fillstyle);
	//udvt_entry * get_colormap(int token);
	//void pixmap_from_colormap(t_pixmap * pixmap);
	//void get_image_options(t_image * image);
	//int  parse_dashtype(t_dashtype * dt);
	// 
	// State information for load_file(), to recover from errors
	// and properly handle recursive load_file calls
	// 
	struct LFS {
		// new recursion level: 
		FILE * fp;               // file pointer for load file 
		char * name;             // name of file 
		char * cmdline;          // content of command string for do_string() */
		// last recursion level: 
		bool   interactive;      // value of interactive flag on entry 
		int    inline_num;       // inline_num on entry 
		int    depth;            // recursion depth 
		bool   if_open_for_else; // used by _new_ if/else syntax 
		char * input_line;       // Input line text to restore 
		lexical_unit * P_Tokens/*tokens*/;   // Input line tokens to restore 
		int    _NumTokens;/*num_tokens*/;       // How big is the above ? 
		int    _CToken/*c_token*/;          // Which one were we on ? 
		LFS  * prev;             // defines a stack 
		int    call_argc;        // This saves the _caller's_ argc 
		char * call_args[10];    // ARG0 through ARG9 from "call" command 
		GpValue argv[10];        // content of global ARGV[] array 
	};

	extern LFS * lf_head; // @global
//
//#include <util3d.h>
	// 
	// HBB 990828: moved all those variable decl's and #defines to new
	// file "graph3d.h", as the definitions are in graph3d.c, not in
	// util3d.c. Include that file from here, to ensure everything is known 
	//
	// All the necessary information about one vertex.
	//
	struct GpVertex {
		coordval x, y, z;         // vertex coordinates 
		lp_style_type * lp_style; // where to find point symbol type (if any) 
		coordval real_z;          // mostly used to track variable color 
		text_label * label;
		GpCoordinate * original; // original coordinates of this point used for variable pointsize, pointtype 
	};

	//typedef GpVertex * p_vertex;
	//
	// Utility macros for vertices: 
	//
	#define FLAG_VERTEX_AS_UNDEFINED(v) do { (v).z = -2.0; } while(0)
	#define VERTEX_IS_UNDEFINED(v) ((v).z == -2.0)
	#define V_EQUAL(a, b) ( GE(0.0, fabs((a)->x - (b)->x) + fabs((a)->y - (b)->y) + fabs((a)->z - (b)->z)) )
	//
	// Maps from normalized space to terminal coordinates 
	//
	#define TERMCOORD_DOUBLE(v, xvar, yvar)           \
		{                                               \
			xvar = (((v)->x * xscaler)) + xmiddle;      \
			yvar = (((v)->y * yscaler)) + ymiddle;      \
		}
	#define TERMCOORD(v, xvar, yvar)                  \
		{                                               \
			xvar = ((int)((v)->x * xscaler)) + xmiddle; \
			yvar = ((int)((v)->y * yscaler)) + ymiddle; \
		}
	//
	// Prototypes of functions exported by "util3d.c" 
	//
	void mat_scale(double sx, double sy, double sz, double mat[4][4]);
	void mat_rot_x(double teta, double mat[4][4]);
	void mat_rot_z(double teta, double mat[4][4]);
	void mat_mult(double mat_res[4][4], double mat1[4][4], double mat2[4][4]);
//
//#include <datafile.h>
	//
	// returns from DF_READLINE in datafile.c 
	// +ve is number of columns read 
	enum DF_STATUS {
		DF_BAD = 0,
		DF_GOOD = 1,
		DF_EOF = -1,
		DF_UNDEFINED = -2,
		DF_FIRST_BLANK = -3,
		DF_SECOND_BLANK = -4,
		DF_MISSING = -5,
		DF_FOUND_KEY_TITLE = -6,
		DF_KEY_TITLE_MISSING = -7,
		DF_STRINGDATA = -8,
		DF_COLUMN_HEADERS = -9,
		DF_COMPLEX_VALUE = -10
	};
	//
	// large file support (offsets potentially > 2GB) 
	//
	#if defined(HAVE_FSEEKO) && defined(HAVE_OFF_T)
		#define fseek(stream, pos, whence) fseeko(stream, pos, whence)
		#define ftell(stream) ftello(stream)
	#elif defined(_MSC_VER)
		#define fseek(stream, pos, whence) _fseeki64(stream, pos, whence)
		#define ftell(stream) _ftelli64(stream)
	#elif defined(__MINGW32__)
		#define fseek(stream, pos, whence) fseeko64(stream, pos, whence)
		#define ftell(stream) ftello64(stream)
	#endif
	//
	// Variables of datafile.c needed by other modules: 
	//
	extern int df_no_use_specs; /* how many using columns were specified in the current command */

	/* Maximum number of columns returned to caller by df_readline		*/
	/* Various data structures are dimensioned to hold this many entries.	*/
	/* As of June 2013, plot commands never ask for more than 7 columns of	*/
	/* data, but fit commands can use more. "fit" is also limited by	*/
	/* the number of parameters that can be passed	to a user function, so	*/
	/* let's try setting MAXDATACOLS to match.				*/
	/* At present this bumps it from 7 to 14.				*/
	#define MAXDATACOLS (MAX_NUM_VAR+2)

	extern int df_datum;   // suggested x value if none given 
	extern bool df_matrix; // is this a matrix splot? 
	extern bool df_binary; // is this a binary file? 
	extern bool df_voxelgrid; // was df_open called on something that turned out to be a voxel grid? 
	extern char * df_filename;
	extern int df_line_number;
	extern AXIS_INDEX df_axis[];
	/* Returned to caller by df_readline() */
	extern char * df_tokens[];
	extern GpValue df_strings[];       /* used only by TABLESTYLE */
	extern int df_last_col; /* number of columns in first row of data return to user in STATS_columns */
	extern int df_bad_matrix_values; /* number of matrix elements entered as missing or NaN */
	extern bool df_columnheaders; /* First row of data is known to contain headers rather than data */
	extern char * missing_val; /* string representing missing values, ascii datafiles */
	extern char * df_separators; /* input field separators, NULL if whitespace is the separator */
	extern char * df_commentschars; /* comments chars */
	extern bool plotted_data_from_stdin; /* flag if any 'inline' data are in use, for the current plot */

	/* Setting this allows the parser to recognize Fortran D or Q   */
	/* format constants in the input file. But it slows things down */
	extern bool df_fortran_constants;

	/* Setting this disables initialization of the floating point exception */
	/* handler before every expression evaluation in a using specifier.      */
	/* This can speed data input significantly, but assumes valid input.    */
	extern bool df_nofpe_trap;
	extern bool evaluate_inside_using;
	extern bool df_warn_on_missing_columnheader;
	//
	// Used by plot title columnhead, stats name columnhead 
	//
	extern char * df_key_title;
	extern struct at_type * df_plot_title_at;
	//
	// Prototypes of functions exported by datafile.c 
	//
	//int df_open(const char *, int, curve_points *);
	//int df_readline(double [], int);
	void df_close();
	void df_init();
	char * df_fgets(FILE *);
	void df_showdata();
	int df_2dbinary(curve_points *);
	int df_3dmatrix(struct surface_points *, int);
	void df_set_key_title(curve_points *);
	//void df_set_key_title_columnhead(const curve_points *);
	char * df_parse_string_field(const char *);
	int expect_string(const char column);
	void require_value(const char column);
	char * df_retrieve_columnhead(int column);
	void df_reset_after_error();

	struct use_spec_s {
		int column;
		int expected_type;
		struct at_type * at;
		int depends_on_column;
	};
	// 
	// Details about the records contained in a binary data file.
	// 
	enum df_translation_type {
		DF_TRANSLATE_DEFAULT, /* Gnuplot will position in first quadrant at origin. */
		DF_TRANSLATE_VIA_ORIGIN,
		DF_TRANSLATE_VIA_CENTER
	};

	enum df_sample_scan_type {
		DF_SCAN_POINT = -3, /* fastest */
		DF_SCAN_LINE  = -4,
		DF_SCAN_PLANE = -5 /* slowest */
	};
	//
	// To generate a swap, take the bit-wise complement of the lowest two bits. 
	//
	enum df_endianess_type {
		DF_LITTLE_ENDIAN,
		DF_PDP_ENDIAN,
		DF_DPD_ENDIAN,
		DF_BIG_ENDIAN,
		DF_ENDIAN_TYPE_LENGTH /* Must be last */
	};
	//
	// The various types of numerical types that can be read from a data file. 
	//
	enum df_data_type {
		DF_CHAR, 
		DF_UCHAR, 
		DF_SHORT, 
		DF_USHORT, 
		DF_INT,
		DF_UINT, 
		DF_LONG,  
		DF_ULONG, 
		DF_FLOAT,  
		DF_DOUBLE,
		DF_LONGLONG, 
		DF_ULONGLONG,
		DF_BAD_TYPE
	};
	#define DF_DEFAULT_TYPE DF_FLOAT

	/* Some macros for making the compiler figure out what function
	 * the "machine independent" names should execute to read the
	 * appropriately sized variable from a data file.
	 */
	#define SIGNED_TEST(val) ((val)==sizeof(long) ? DF_LONG : \
		((val)==sizeof(int64) ? DF_LONGLONG : \
		((val)==sizeof(int) ? DF_INT : \
		((val)==sizeof(short) ? DF_SHORT : \
		((val)==sizeof(char) ? DF_CHAR : DF_BAD_TYPE)))))
	#define UNSIGNED_TEST(val) ((val)==sizeof(ulong) ? DF_ULONG : \
		((val)==sizeof(uint64) ? DF_ULONGLONG : \
		((val)==sizeof(uint) ? DF_UINT : \
		((val)==sizeof(ushort) ? DF_USHORT : \
		((val)==sizeof(uchar) ? DF_UCHAR : DF_BAD_TYPE)))))
	#define FLOAT_TEST(val) ((val)==sizeof(float) ? DF_FLOAT : ((val)==sizeof(double) ? DF_DOUBLE : DF_BAD_TYPE))

	enum df_records_type {
		DF_CURRENT_RECORDS,
		DF_DEFAULT_RECORDS
	};

	struct df_binary_type_struct {
		df_data_type read_type;
		ushort read_size;
	};

	struct df_column_bininfo_struct {
		long skip_bytes;
		df_binary_type_struct column;
	};

	/* NOTE TO THOSE WRITING FILE TYPE FUNCTIONS
	 *
	 * "cart" means Cartesian, i.e., the (x,y,z) [or (r,t,z)] coordinate
	 * system of the plot.  "scan" refers to the scanning method of the
	 * file in question, i.e., first points, then lines, then planes.
	 * The important variables for a file type function to fill in are
	 * those beginning with "scan".  There is a tricky set of rules
	 * related to the "scan_cart" mapping, the file-specified variables,
	 * the default variables, and the command-line variables.  Basically,
	 * command line overrides data file which overrides default.  (Yes,
	 * like a confusing version of rock, paper, scissors.) So, from the
	 * file type function perspective, it is better to leave those
	 * variables which are not specifically known from file data or
	 * otherwise (e.g., sample periods "scan_delta") unaltered in case
	 * the user has issued "set datafile" to define defaults.
	 */
	struct df_binary_file_record_struct {
		int    cart_dim[3];              /* dimension array size, x/y/z */
		int    cart_dir[3];              /* 1 scan in positive direction, -1 negative, x/y/z */
		double cart_delta[3];         /* spacing between array points, x/y/z */
		df_translation_type cart_trans; /* translate via origin, center or default */
		double cart_cen_or_ori[3];    /* vector representing center or origin, x/y/z */
		double cart_alpha;            /* 2D rotation angle (rotate) */
		double cart_p[3];             /* 3D rotation normal vector (perpendicular) */
		df_sample_scan_type cart_scan[3]; /* how to assign the dimensions read from file when generating coordinates */
		bool   scan_generate_coord; /* whether or not Gnuplot should generate coordinates. */
		off_t  scan_skip[3];           /* skip bytes before the record, line, plane */
		// 
		// Not controllable by the user, only by file type functions.
		// These are all points/lines/planes format.
		// 
		int    scan_dim[3];              /* number of points, lines, planes */
		int    scan_dir[3];              /* 1 scan in positive direction wrt Cartesian coordinate system, -1 negative */
		double scan_delta[3];         /* sample period along points, lines, planes */
		df_translation_type scan_trans; /* translate via origin, center or default */
		double scan_cen_or_ori[3];    /* vector representing center or origin, x/y/z */
		// 
		// `matrix every ::lowx:lowy:` can select a submatrix.
		// This is its size.
		// 
		int    submatrix_ncols;
		int    submatrix_nrows;
		/* *** Do not modify outside of datafile.c!!! *** */
		char * memory_data;
	};

	extern df_binary_file_record_struct * df_bin_record;
	extern int df_num_bin_records;
	extern const GpCoordinate blank_data_line;
	extern use_spec_s use_spec[];
	//
	// Prototypes of functions exported by datafile.c 
	//
	void df_show_binary(FILE * fp);
	void df_show_datasizes(FILE * fp);
	void df_show_filetypes(FILE * fp);
	//void df_set_datafile_binary();
	void df_unset_datafile_binary();
	void df_add_binary_records(int, df_records_type);
	void df_extend_binary_columns(int);
	void df_set_skip_before(int col, int bytes);                /* Number of bytes to skip before a binary column. */
	#define df_set_skip_after(col, bytes) df_set_skip_before(col+1, bytes)  /* Number of bytes to skip after a binary column. */
	void df_set_read_type(int col, df_data_type type);          /* Type of data in the binary column. */
	df_data_type df_get_read_type(int col);                     /* Type of data in the binary column. */
	int df_get_read_size(int col);                              /* Size of data in the binary column. */
	int df_get_num_matrix_cols();
	void df_set_plot_mode(int);
//
//#include <datablock.h>
	//void datablock_command();
	char **get_datablock(const char *name);
	//char *parse_datablock_name();
	void gpfree_datablock(GpValue *datablock_value);
	void append_to_datablock(GpValue *datablock_value, const char * line);
	void append_multiline_to_datablock(GpValue *datablock_value, const char * lines);
	int datablock_size(GpValue *datablock_value);
//
//#include <fit.h>
	// defaults 
	#define DEF_FIT_LIMIT 1e-5

	// error interrupt for fitting routines 
	#define Eex(a)       { error_ex(NO_CARET, (a)); }
	#define Eex2(a, b)    { error_ex(NO_CARET, (a), (b)); }
	#define Eex3(a, b, c)  { error_ex(NO_CARET, (a), (b), (c)); }
	#define Eexc(c, a)    { error_ex((c), (a)); }
	#define Eexc2(c, a, b) { error_ex((c), (a), (b)); }
	//
	// Type definitions
	//
	enum verbosity_level {
		QUIET = 1, 
		RESULTS, 
		BRIEF, 
		VERBOSE
	};

	typedef char fixstr[MAX_ID_LEN+1];
	//
	// Exported Variables of fit.c 
	//
	extern const char * FITLIMIT;
	extern const char * FITSTARTLAMBDA;
	extern const char * FITLAMBDAFACTOR;
	extern const char * FITMAXITER;
	extern char * fitlogfile;
	extern bool fit_suppress_log;
	extern bool fit_errorvariables;
	extern bool fit_covarvariables;
	extern verbosity_level fit_verbosity;
	extern bool fit_errorscaling;
	extern bool fit_prescale;
	extern char * fit_script;
	extern double epsilon_abs; // absolute convergence criterion 
	extern int maxiter;
	extern int fit_wrap;
	extern bool fit_v4compatible;
	//
	// Prototypes of functions exported by fit.c 
	//
	#if defined(__GNUC__)
		void error_ex(int t_num, const char * str, ...) __attribute__((noreturn));
	#elif defined(_MSC_VER)
		__declspec(noreturn) void error_ex(int t_num, const char * str, ...);
	#else
		void error_ex(int t_num, const char * str, ...);
	#endif
	void   init_fit();
	void   update(char * pfile, char * npfile);
	void   save_fit(FILE * fp);
	//void   fit_command();
	size_t wri_to_fil_last_fit_cmd(FILE * fp);
	char * getfitlogfile();
	const  char * getfitscript();
	void   call_gnuplot(const double * par, double * data);
	bool   regress_check_stop(int iter, double chisq, double last_chisq, double lambda);
	void   fit_progress(int i, double chisq, double last_chisq, double* a, double lambda, FILE * device);
//
//#include <readline.h>
	//
	// Prototypes of functions exported by readline.c 
	//
	#if defined(HAVE_LIBREADLINE)
		#include <readline/readline.h>
	#elif defined(HAVE_LIBEDITLINE)
		#include <editline/readline.h>
	#endif
	#if defined(READLINE)
		char *readline(const char *);
	#elif defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
		int getc_wrapper(FILE* fp);
	#endif
	#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_SIGNAL_HANDLER)
		void wrap_readline_signal_handler();
	#else
		#define wrap_readline_signal_handler()
	#endif
//
//#include <save.h>
	//
	// Variables of save.c needed by other modules: 
	//
	extern const char *coord_msg[];
	//
	// Prototypes of functions exported by save.c 
	//
	void save_functions(FILE *fp);
	void save_variables(FILE *fp);
	void save_datablocks(FILE *fp);
	void save_colormaps(FILE *fp);
	void save_set(FILE *fp);
	void save_term(FILE *fp);
	void save_all(FILE *fp);
	void save_axis_label_or_title(FILE *fp, char *name, char *suffix, struct text_label *label, bool savejust);
	void save_position(FILE *, const GpPosition *, int, bool);
	void save_prange(FILE *, GpAxis *);
	void save_link(FILE *, GpAxis *);
	void save_nonlinear(FILE *, GpAxis *);
	void save_textcolor(FILE *, const struct t_colorspec *);
	void save_pm3dcolor(FILE *, const struct t_colorspec *);
	void save_fillstyle(FILE *, const struct fill_style_type *);
	void save_offsets(FILE *, char *);
	void save_histogram_opts(FILE *fp);
	void save_pixmaps(FILE *fp);
	void save_object(FILE *, int);
	void save_walls(FILE *);
	void save_style_textbox(FILE *);
	void save_style_parallel(FILE *);
	void save_style_spider(FILE *);
	void save_data_func_style(FILE *, const char *, enum PLOT_STYLE);
	void save_linetype(FILE *, lp_style_type *, bool);
	void save_dashtype(FILE *, int, const t_dashtype *);
	void save_num_or_time_input(FILE *, double x, const GpAxis *);
	//void save_axis_format(FILE *fp, AXIS_INDEX axis);
	void save_bars(FILE *);
	void save_array_content(FILE *, GpValue *);
//
//#include <scanner.h>
	//
	// Variables of scanner.c needed by other modules:
	//
	extern int curly_brace_count;
	//
	// Prototypes of functions exported by scanner.c 
	//
	bool legal_identifier(char *p);
	//int scanner(char **expression, size_t *line_lengthp);
//
//#include <setshow.h>
	// The set and show commands, in setshow.c 
	//void set_command();
	//void unset_command();
	//void reset_command();
	//void show_command();
	// and some accessible support functions 
	void show_version(FILE *fp);
	//void set_format();
	//void set_colorsequence(int option);
	const char * FASTCALL conv_text(const char * s);
	void delete_linestyle(struct linestyle_def **, struct linestyle_def *, struct linestyle_def *);
	void delete_dashtype(struct custom_dashtype_def *, struct custom_dashtype_def *);
	// void delete_arrowstyle(struct arrowstyle_def *, struct arrowstyle_def *); 
	void reset_key();
	void free_marklist(struct ticmark * list);
	extern int enable_reset_palette;
	//void reset_palette();
	void reset_bars();
	//void rrange_to_xy();
	//void unset_monochrome();
	void unset_all_tics();
	// Called from set_label(), plot2d.c and plot3d.c 
	//extern void parse_label_options(struct text_label *, int ndim);
	extern struct text_label * new_text_label(int tag);
	extern void disp_value(FILE *, GpValue *, bool);
	extern struct ticmark * prune_dataticks(struct ticmark *list);
//
//#include <stats.h>
	//void statsrequest();
//
//#include <external.h>
	#ifdef HAVE_EXTERNAL_FUNCTIONS
	// Prototypes from file "external.c" 
	void f_calle(union argument * x);
	struct at_type * external_at(const char *);
	void external_free(struct at_type *);

	#if defined(_WIN32)
		#include <windows.h>
		#include <stdio.h>
		typedef void * gp_dll_t;

		#define DLL_PATHSEP "\\"
		#define DLL_EXT  ".dll"
		#define DLL_OPEN(f) dll_open_w(f);
		#define DLL_CLOSE(dl) ((void)FreeLibrary((HINSTANCE)dl))
		#define DLL_SYM(dl, sym) ((void*)GetProcAddress((HINSTANCE)dl, (sym)))
		#define DLL_ERROR(dl) "dynamic library error"
	#elif defined(HAVE_DLFCN_H)
		#include <dlfcn.h>
		typedef void * gp_dll_t;

		#define DLL_PATHSEP "/"
		#define DLL_EXT  ".so"
		#define DLL_OPEN(f) dlopen((f), RTLD_NOW);
		#define DLL_CLOSE(dl) dlclose(dl)
		#define DLL_SYM(dl, sym) dlsym((dl), (sym))
		#define DLL_ERROR(dl) dlerror()
	#elif defined(HAVE_DL_H)
		#include <dl.h>
		typedef shl_t gp_dll_t;

		#define DLL_PATHSEP "/"
		#define DLL_EXT  ".so"
		#define DLL_OPEN(f) shl_load((f), BIND_IMMEDIATE, 0);
		#define DLL_CLOSE(dl) shl_unload(dl)
		__inline__ static DLL_SYM(gp_dll_t dl, const char * sym)
		{
			void * a;
			if(shl_findsym(&dl, sym, TYPE_PROCEDURE, &a))
				return a;
			else
				return 0x0;
		}
		#define DLL_ERROR(dl) strerror(errno)
	#else /* No DLL */
		#error "HAVE_EXTERNAL_FUNCTIONS requires a DLL lib"
	#endif /* No DLL */
	#endif /* HAVE_EXTERNAL_FUNCTIONS */
//
//#include <internal.h>
	// Prototypes from file "internal.c" 
	void eval_reset_after_error();
//
//#include <tabulate.h>
	//
	// Routines in tabulate.c needed by other modules:
	//
	//void print_table(curve_points * pFirstPlot, int plot_num);
	//void print_3dtable(int pcount);
	//bool tabulate_one_line(double v[], GpValue str[], int ncols);

	//extern FILE * table_outfile;
	//extern udvt_entry * table_var;
	//extern bool   table_mode;
	//extern char * table_sep;
	//extern at_type * table_filter_at;
//
//#include <voxelgrid.h>
	#define VOXEL_GRID_SUPPORT 1

	typedef float t_voxel;

	struct vgrid {
		int    size;       /* size x size x size array */
		double vxmin;
		double vxmax;
		double vxdelta;
		double vymin;
		double vymax;
		double vydelta;
		double vzmin;
		double vzmax;
		double vzdelta;
		double min_value; /* min non-zero voxel value */
		double max_value; /* max voxel value */
		double mean_value; /* mean non-zero voxel value */
		double stddev;  /* esd over non-zero voxels */
		double sum;
		int    nzero;
		t_voxel * vdata; /* points to 3D array of voxels */
	};

	struct isosurface_opt {
		int tessellation; /* 0 = mixed  1 = triangles only */
		int inside_offset; /* difference between front/back linetypes */
	};
	//
	// function prototypes 
	//
	//void   voxel_command();
	void   vclear_command();
	//void   vfill_command();
	//void   f_voxel(union argument * x);
	void   init_voxelsupport();
	//void   set_vgrid();
	//void   set_vgrid_range();
	void   show_vgrid();
	void   gpfree_vgrid(struct udvt_entry * grid);
	void   unset_vgrid();
	udvt_entry * get_vgrid_by_name(const char * name);
	//void   check_grid_ranges();
	t_voxel voxel(double vx, double vy, double vz);
	//void   set_isosurface();
	void   show_isosurface();
	void   vgrid_stats(vgrid * vgrid);
	//
	// variables 
	//
	extern isosurface_opt isosurface_options;
//
//#include <dynarray.h>
	struct dynarray {
		long size;              /* alloced size of the array */
		long end;               /* index of first unused entry */
		long increment;         /* amount to increment size by, on realloc */
		size_t entry_size;      /* size of the entries in this array */
		void * v;       /* the vector itself */
	};
	//
	// Prototypes 
	//
	void   init_dynarray(dynarray * array, size_t element, long size, long increment);
	void   free_dynarray(dynarray * array);
	void   extend_dynarray(dynarray * array, long increment);
	void   resize_dynarray(dynarray * array, long newsize);
	void * nextfrom_dynarray(dynarray * array);
	void   droplast_dynarray(dynarray * array);
//
//#include <amos_airy.h>
	// 
	// prototypes for user-callable functions wrapping routines
	// from the Amos collection of complex special functions
	// Ai(z) Airy function (complex argument)
	// Bi(z) Airy function (complex argument)
	// BesselJ(z,nu) Bessel function of the first kind J_nu(z)
	// BesselY(z,nu) Bessel function of the second kind Y_nu(z)
	// BesselI(z,nu) modified Bessel function of the first kind I_nu(z)
	// BesselK(z,nu) modified Bessel function of the second kind K_nu(z)
	// 
	#ifdef HAVE_AMOS
		void f_amos_Ai(union argument *x);
		void f_amos_Bi(union argument *x);
		void f_amos_BesselI(union argument *x);
		void f_amos_BesselJ(union argument *x);
		void f_amos_BesselK(union argument *x);
		void f_amos_BesselY(union argument *x);
		void f_Hankel1(union argument *x);
		void f_Hankel2(union argument *x);
	#endif
	// CEXINT is in libamos but not in libopenspecfun 
	void f_amos_cexint(union argument *x);
//
//#include <complexfun.h>
	#ifdef HAVE_COMPLEX_FUNCS
		//
		// Prototypes for complex functions
		//
		void f_Igamma(union argument *arg);
		void f_LambertW(union argument *arg);
		void f_lnGamma(union argument *arg);
		void f_Sign(union argument *arg);
		double igamma(double a, double z);
	#endif
//
//#include <libcerf.h>
	void   f_cerf(union argument *z);
	void   f_cdawson(union argument *z);
	void   f_faddeeva(union argument *z);
	void   f_voigtp(union argument *z);
	//void   f_voigt(union argument *z);
	void   f_erfi(union argument *z);
	void   f_VP_fwhm(union argument *z);
	void   f_FresnelC(union argument *z);
	void   f_FresnelS(union argument *z);
//
//#include <specfun.h>
	//
	// These are the more 'special' functions built into the stack machine.
	//
	double chisq_cdf(int dof, double chisqr);
	#ifndef HAVE_LIBCERF
		//void f_voigt(union argument * x);
	#endif
//
//#include <standard.h>
	void f_void(union argument *x);
//
//#include <version.h>
	extern const char gnuplot_version[];
	extern const char gnuplot_patchlevel[];
	extern const char gnuplot_date[];
	extern const char gnuplot_copyright[];
	extern const char faq_location[];
	extern const char bug_email[];
	extern const char help_email[];
	extern char * compile_options;
//
//#include <matrix.h>
	//
	// public functions
	//
	double  * vec(int n);
	int     * ivec(int n);
	double  ** matr(int r, int c);
	void    free_matr(double ** m);
	double  * redim_vec(double ** v, int n);
	void    solve(double ** a, int n, double ** b, int m);
	void    Givens(double ** C, double * d, double * x, int N, int n);
	void    Invert_RtR(double ** R, double ** I, int n);
	//
	// Functions for use by THIN_PLATE_SPLINES_GRID method
	//
	void    lu_decomp(double **, int, int *, double *);
	void    lu_backsubst(double **, int n, int *, double *);
	double   enorm_vec(int n, const double * x);
	double   sumsq_vec(int n, const double * x);
//
//#include <contour.h>
	#define DEFAULT_CONTOUR_LEVELS 5
	#define DEFAULT_NUM_APPROX_PTS 5
	#define DEFAULT_CONTOUR_ORDER  4
	#define MAX_BSPLINE_ORDER      10
	//
	// Type definitions 
	//
	enum t_contour_kind {
		/* Method of drawing the contour lines found */
		CONTOUR_KIND_LINEAR,
		CONTOUR_KIND_CUBIC_SPL,
		CONTOUR_KIND_BSPLINE
	};

	enum t_contour_levels_kind {
		/* How contour levels are set */
		LEVELS_AUTO,            /* automatically selected */
		LEVELS_INCREMENTAL,     /* user specified start & increment */
		LEVELS_DISCRETE         /* user specified discrete levels */
	};

	typedef double tri_diag[3]; /* Used to allocate the tri-diag matrix. */
	//
	// Variables of contour.c needed by other modules: 
	//
	extern char contour_format[32];
	extern t_contour_kind contour_kind;
	extern t_contour_levels_kind contour_levels_kind;
	extern int contour_levels;
	extern int contour_order;
	extern int contour_pts;
	extern int contour_firstlinetype;
	extern bool contour_sortlevels;
	extern dynarray dyn_contour_levels_list; /* storage for z levels to draw contours at */
	#define contour_levels_list ((double*)dyn_contour_levels_list.v)
	//
	// Prototypes of functions exported by contour.c 
	//
	//struct gnuplot_contours * contour(int num_isolines, struct iso_curve * iso_lines);
//
//#include <hidden3d.h>
	#define PT_ARROWHEAD -10
	#define PT_BACKARROW -11
	#define PT_BOTHHEADS -12
	//
	// Variables of hidden3d.c needed by other modules: 
	//
	extern bool disable_mouse_z;
	extern int hiddenBacksideLinetypeOffset;
	//
	// Prototypes of functions exported by hidden3d.c 
	//
	//void set_hidden3doptions();
	void show_hidden3doptions();
	void reset_hidden3doptions();
	void save_hidden3doptions(FILE *fp);
	void init_hidden_line_removal();
	void reset_hidden_line_removal();
	void term_hidden_line_removal();
	//void plot3d_hidden(termentry * pTerm, surface_points *plots, int pcount);
	//void draw_line_hidden(GpVertex *, GpVertex *, lp_style_type *);
	//void draw_label_hidden(GpVertex *, lp_style_type *, int, int);
//
//#include <vplot.h>
	// 
	// prototypes for external callers
	// 
	void vplot_points(struct surface_points *plot, double level);
	void vplot_isosurface(struct surface_points *plot, int downsample);
//
//#include <encoding.h>
	void init_encoding();
	enum set_encoding_id encoding_from_locale();
	void init_special_chars();
	const char * latex_input_encoding(enum set_encoding_id encoding);
	bool contains8bit(const char * s);
	bool utf8toulong(ulong * wch, const char ** str);
	int ucs4toutf8(uint32_t codepoint, uchar * utf8char);
	size_t strlen_utf8(const char * s);
	void truncate_to_one_utf8_char(char * orig);
	bool is_sjis_lead_byte(char c);
	size_t strlen_sjis(const char * s);

	#define advance_one_utf8_char(utf8str) \
		do { \
			if((*utf8str & 0x80) == 0x00) \
				utf8str += 1; \
			else if((*utf8str & 0xe0) == 0xc0) \
				utf8str += 2; \
			else if((*utf8str & 0xf0) == 0xe0) \
				utf8str += 3; \
			else if((*utf8str & 0xf8) == 0xf0) \
				utf8str += 4; \
			else /* invalid utf8 sequence */ \
				utf8str += 1; \
		} while(0)
//
//#include <jitter.h>
	enum jitterstyle {
		JITTER_DEFAULT = 0,
		JITTER_SWARM,
		JITTER_SQUARE,
		JITTER_ON_Y
	};

	struct t_jitter {
		struct GpPosition overlap;
		double spread;
		double limit;
		enum jitterstyle style;
	};

	extern t_jitter jitter;

	//extern void jitter_points(curve_points * plot);
	//extern void set_jitter();
	extern void show_jitter();
	extern void unset_jitter();
	extern void save_jitter(FILE *);
//
//#include <help.h>
	//
	// Exit status returned by help() 
	//
	#define	H_FOUND		0	/* found the keyword */
	#define	H_NOTFOUND	1	/* didn't find the keyword */
	#define	H_ERROR		(-1)	/* didn't find the help file */
	//
	// Prototypes from file "help.c" 
	//
	int  help(char *keyword, char *path, bool *subtopics);
	void FreeHelp();
	void StartOutput();
	void OutLine(const char *line);
	void EndOutput();
//
//#include <interpol.h>
	//
	// Prototypes of functions exported by interpol.c 
	//
	//void gen_interp(curve_points *plot);
	void gen_interp_unwrap(curve_points *plot);
	void gen_interp_frequency(curve_points *plot);
	//void mcs_interp(curve_points *plot);
	void sort_points(curve_points *plot);
	void zsort_points(curve_points *plot);
	//void cp_implode(curve_points *cp);
	//void make_bins(curve_points *plot, int nbins, double binlow, double binhigh, double binwidth);
	//void gen_3d_splines(struct surface_points *plot);
	void gen_2d_path_splines(curve_points *plot);
//
//#include <multiplot.h>
	//void   multiplot_start();
	//void   multiplot_end();
	//void   multiplot_next();
	//void   multiplot_reset();
	//int    multiplot_current_panel();
//
#include <mousecmn.h>
#if defined(USE_MOUSE) || defined(WIN_IPC)
	//#include "mouse.h"
		// 
		// Zoom queue
		// 
		struct t_zoom {
			SPoint2R _Min;
			SPoint2R _Max;
			SPoint2R _2Min;
			SPoint2R _2Max;
			//double xmin;
			//double ymin;
			//double xmax;
			//double ymax;
			//double x2min;
			//double y2min;
			//double x2max;
			//double y2max;
			t_zoom * prev;
			t_zoom * next;
		};

		struct mouse_setting_t {
			int on;            /* ...                                         */
			int doubleclick;   /* Button1 double / single click resolution    */
			int annotate_zoom_box; /* draw coordinates at zoom box                */
			int label;         /* draw real gnuplot labels on Button 2        */
			int polardistance; /* display dist. to ruler in polar coordinates */
			int verbose;       /* display ipc commands                        */
			int warp_pointer;  /* warp pointer after starting a zoom box      */
			double xmzoom_factor; /* scale factor for +/- zoom on x		  */
			double ymzoom_factor; /* scale factor for +/- zoom on y		  */
			char * fmt;        /* fprintf format for printing numbers         */
			char * labelopts;  /* label options                               */
		};

		#define DEFAULT_MOUSE_MODE    1 // start with mouse on by default 
		#define DEFAULT_MOUSE_SETTING { DEFAULT_MOUSE_MODE, 300/* ms */, 1, 0, 0, 0, 0, 1.0, 1.0, mouse_fmt_default, NULL }

		extern long mouse_mode;
		extern char* mouse_alt_string;
		extern mouse_setting_t default_mouse_setting;
		extern mouse_setting_t mouse_setting;
		extern char mouse_fmt_default[];
		extern udft_entry mouse_readout_function;

		enum {
			MOUSE_COORDINATES_REAL = 0,
			MOUSE_COORDINATES_REAL1, /* w/o brackets */
			MOUSE_COORDINATES_FRACTIONAL,
			MOUSE_COORDINATES_TIMEFMT,
			MOUSE_COORDINATES_XDATE,
			MOUSE_COORDINATES_XTIME,
			MOUSE_COORDINATES_XDATETIME,
			MOUSE_COORDINATES_ALT, /* alternative format as specified by the user */
			MOUSE_COORDINATES_FUNCTION = 8 /* value needed in term.c even if no USE_MOUSE */
		};

		//void event_plotdone();
		void recalc_statusline();
		//void update_ruler();
		void set_ruler(bool on, int mx, int my);
		//void UpdateStatusline();
		//void do_event(struct gp_event_t * ge);
		bool exec_event(char type, int mx, int my, int par1, int par2, int winid); /* wrapper for do_event() */
		int plot_mode(int mode);
		void event_reset(struct gp_event_t * ge);
		//
		// bind prototype(s) 
		//
		void bind_process(char* lhs, char* rhs, bool allwindows);
		void bind_remove_all();
	//
#endif
//#include "xdg.h"
#ifdef __unix__ // {
	//
	// List of possible XDG variables 
	//
	enum XDGVarType {
		kXDGNone = -1,
		kXDGConfigHome,  /* XDG_CONFIG_HOME */
		kXDGDataHome,    /* XDG_DATA_HOME */
		kXDGCacheHome,   /* XDG_CACHE_HOME */
		kXDGRuntimeDir,  /* XDG_RUNTIME_DIR */
		kXDGConfigDirs,  /* XDG_CONFIG_DIRS */
		kXDGDataDirs,    /* XDG_DATA_DIRS */
	};

	char * xdg_get_var(const XDGVarType idx);
	#endif // } __unix__ 
//
//#include <gpexecute.h>
	#ifdef PIPE_IPC
		extern int pipe_died;
		RETSIGTYPE pipe_died_handler(int signum);
	#endif
	#if defined(PIPE_IPC) || defined(WIN_IPC)
		struct gpe_fifo_t {
			gpe_fifo_t * prev;
			gp_event_t ge;
			gpe_fifo_t * next;
		};
		extern int buffered_output_pending;
	#endif /* PIPE_IPC || WIN_IPC */

	void gp_exec_event(char type, int mx, int my, int par1, int par2, int winid);
//
//
//
#define MIN_CRV_POINTS 100 // minimum size of points[] in curve_points 
//
// HBB 20010610: mnemonic names for the bits stored in 'uses_axis' 
//
enum t_uses_axis {
	USES_AXIS_FOR_DATA = 1,
	USES_AXIS_FOR_FUNC = 2
};

class GpStack {
public:
	GpStack();
	GpValue & Top();
	void Reset();
	// make sure stack's empty 
	void Check() const;
	bool MoreOnStack() const;
	void FASTCALL Push(GpValue * x);
	GpValue * FASTCALL Pop(GpValue * x);
private:
	GpValue St[STACK_DEPTH];
	int    Sp; // stack pointer 
};
//
// Used by terminals and by shared routine parse_term_size() 
//
enum GpSizeUnits {
	PIXELS,
	INCHES,
	CM
};

enum df_multivalue_type {
	DF_DELTA,
	DF_FLIP_AXIS,
	DF_FLIP,
	DF_SCAN,
	DF_ORIGIN,
	DF_CENTER,
	DF_ROTATION,
	DF_PERPENDICULAR,
	DF_SKIP
};

enum WHICHGRID { 
	ALLGRID, 
	FRONTGRID, 
	BACKGRID, 
	BORDERONLY 
};

struct MpLayout_ {
	MpLayout_() : auto_layout(false), current_panel(0), num_rows(0), num_cols(0), row_major(false), downwards(true), act_row(0), act_col(0),
		xscale(1.0), yscale(1.0), xoffset(0.0), yoffset(0.0), auto_layout_margins(false),
		prev_xsize(0.0), prev_ysize(0.0), prev_xoffset(0.0), prev_yoffset(0.0), title_height(0.0)
	{
		lmargin.Set(screen, screen, screen, 0.1, -1.0, -1.0);
		rmargin.Set(screen, screen, screen, 0.9, -1.0, -1.0);
		bmargin.Set(screen, screen, screen, 0.1, -1.0, -1.0);
		tmargin.Set(screen, screen, screen, 0.9, -1.0, -.01);
		xspacing.Set(screen, screen, screen, 0.05, -1.0, -1.0);
		yspacing.Set(screen, screen, screen, 0.05, -1.0, -1.0);
		prev_lmargin.SetDefaultMargin();
		prev_rmargin.SetDefaultMargin();
		prev_tmargin.SetDefaultMargin();
		prev_bmargin.SetDefaultMargin();
		//EMPTY_LABELSTRUCT, 0.0
	}
	bool   auto_layout;   // automatic layout if true 
	int    current_panel; // initialized to 0, incremented after each plot 
	int    num_rows;      // number of rows in layout 
	int    num_cols;      // number of columns in layout 
	bool   row_major;     // row major mode if true, column major else 
	bool   downwards;     // prefer downwards or upwards direction 
	int    act_row;       // actual row in layout 
	int    act_col;       // actual column in layout 
	double xscale;        // factor for horizontal scaling 
	double yscale;        // factor for vertical scaling 
	double xoffset;       // horizontal shift 
	double yoffset;       // horizontal shift 
	bool   auto_layout_margins;
	GpPosition lmargin;
	GpPosition rmargin;
	GpPosition bmargin;
	GpPosition tmargin;
	GpPosition xspacing;
	GpPosition yspacing;
	double prev_xsize;
	double prev_ysize;
	double prev_xoffset;
	double prev_yoffset;
	GpPosition prev_lmargin;
	GpPosition prev_rmargin;
	GpPosition prev_tmargin;
	GpPosition prev_bmargin;
	// values before 'set multiplot layout' 
	text_label title;    // goes above complete set of plots 
	double title_height; // fractional height reserved for title 
}; 

struct GpTabulate {
	GpTabulate() : P_TabOutFile(0), P_Var(0), P_Sep(0), P_FilterAt(0), P_OutFile(0), Mode(false)
	{
	}
	// File or datablock for output during 'set table' mode 
	FILE * P_TabOutFile;
	udvt_entry * P_Var;
	char * P_Sep;
	at_type * P_FilterAt;
	FILE * P_OutFile;
	bool   Mode;
};

struct Quadrangle {
	double gray;
	double z; // z value after rotation to graph coordinates for depth sorting 
	union {
		gpdPoint corners[4]; // The normal case. vertices stored right here 
		int array_index;     // Stored elsewhere if there are more than 4 
	} vertex;
	union { // Only used by depthorder processing 
		t_colorspec * colorspec;
		uint rgb_color;
	} qcolor;
	short fillstyle; // from plot->fill_properties 
	short type;      // QUAD_TYPE_NORMAL or QUAD_TYPE_4SIDES etc 
};
//
// Descr: One edge of the mesh. The edges are (currently) organized into a
//   linked list as a method of traversing them back-to-front. 
//
struct GpEdge {
	long   v1;
	long   v2;          // the vertices at either end 
	int    style;       // linetype index 
	lp_style_type * lp; // line/point style attributes 
	long   next;        // index of next edge in z-sorted list 
};

typedef int32 charcell; // UTF-8 support 

#define DUMB_XMAX 79
#define DUMB_YMAX 24

enum DUMB_id { 
	DUMB_FEED, 
	DUMB_NOFEED, 
	DUMB_ENH, 
	DUMB_NOENH,
	DUMB_SIZE, 
	DUMB_ASPECT, 
	DUMB_ANSI, 
	DUMB_ANSI256, 
	DUMB_ANSIRGB, 
	DUMB_NOCOLOR, 
	DUMB_OTHER 
};

struct TermDumbBlock {
	TermDumbBlock() : P_Matrix(0), P_Colors(0), XMax(DUMB_XMAX), YMax(DUMB_YMAX), Feed(true),
		ColorMode(0), Pen(0), X(0), Y(0)
	{
	}
	//#define DUMB_PIXEL(x, y) GPO.TDumbB.P_Matrix[GPO.TDumbB.XMax*(y)+(x)]
	charcell & Pixel(uint x, uint y) { return P_Matrix[XMax*(y)+(x)]; }
	charcell * P_Matrix; // matrix of characters 
	void   DumbRgbColor(rgb255_color rgb255, char * pColorString);
#ifndef NO_DUMB_COLOR_SUPPORT
	// matrix of colors 
	t_colorspec * P_Colors;
	t_colorspec Color;
	t_colorspec PrevColor;
#endif
	char Pen; // current character used to draw 
	int  X;   // current X position 
	int  Y;   // current Y position 
	int  XMax;
	int  YMax;
	bool Feed;
	int  ColorMode;
};
//
// Support for enhanced text mode. Declared extern in term_api.h 
//
struct GpEnhancedText {
	GpEnhancedText() : P_CurText(0), FontScale(1.0), MaxHeight(0.0), MinHeight(0.0), Ignore(false)
	{
		memzero(EscapeFormat, sizeof(EscapeFormat));
	}
	char   Text[MAX_LINE_LEN+1];
	char * P_CurText;
	double FontScale;
	char   EscapeFormat[16];
	double MaxHeight;
	double MinHeight;
	bool   Ignore; // flag variable to disable enhanced output of filenames, mainly. 
};

struct ps_fontfile_def {
	ps_fontfile_def * next; // pointer to next fontfile in linked list 
	char * fontfile_name;
	char * fontfile_fullname;
	char * fontname;
};
//
// Terminal type of postscript dialect 
//
enum PS_TERMINALTYPE {
	PSTERM_PSTEX, 
	PSTERM_PSLATEX, 
	PSTERM_EPSLATEX, 
	PSTERM_POSTSCRIPT
};

enum PS_PSFORMAT {
	PSTERM_EPS, 
	PSTERM_PORTRAIT, 
	PSTERM_LANDSCAPE
};
//
// One struct that takes all terminal parameters
// by Harald Harders <h.harders@tu-bs.de> 
//
struct ps_params_t {
	ps_params_t(PS_TERMINALTYPE tt) : first_fontfile(0)
	{
		Setup(tt);
	}
	void   Setup(PS_TERMINALTYPE tt)
	{
		first_fontfile = 0;
		if(tt == PSTERM_POSTSCRIPT) {
			terminal = PSTERM_POSTSCRIPT;
			xoff = 50;
			yoff = 50;
			psformat = PSTERM_LANDSCAPE;
			level1 = FALSE;
			level3 = FALSE;
			color = FALSE;
			blacktext = FALSE;
			solid = FALSE;
			dash_length = 1.0f;
			linewidth_factor = 1.0f;
			pointscale_factor = 1.0f;
			duplex_option = FALSE;
			duplex_state = FALSE;
			rounded = FALSE;
			clipped = FALSE; 
			fontsize = 14.0f;
			fontscale = 1.0f; 
			useauxfile = FALSE;
			rotate = FALSE;
			palfunc_samples = 2000;
			palfunc_deviation = 0.003;
			oldstyle = FALSE;
			epslatex_standalone = TRUE;
			adobeglyphnames = FALSE;
			STRNSCPY(font, "Helvetica");
		}
		else if(tt == PSTERM_EPSLATEX) {
			terminal = PSTERM_EPSLATEX;
			xoff = 50; 
			yoff = 50;
			psformat = PSTERM_EPS;
			level1 = FALSE;
			level3 = FALSE;
			color = FALSE;
			blacktext = TRUE;
			solid = FALSE;
			dash_length = 1.0f;
			linewidth_factor = 1.0f;
			pointscale_factor = 1.0f;
			duplex_option = FALSE;
			duplex_state = FALSE;
			rounded = FALSE;
			clipped = FALSE;
			fontsize = 11.0f;
			fontscale = 1.0f;
			useauxfile = TRUE;
			rotate = FALSE;
			palfunc_samples = 2000;
			palfunc_deviation = 0.003;
			oldstyle = FALSE;
			epslatex_standalone = FALSE;
			adobeglyphnames = FALSE;
			STRNSCPY(font, "");
		}
		else if(tt == PSTERM_PSLATEX) {
			terminal = PSTERM_PSLATEX;
			xoff = 0; 
			yoff = 0;
			psformat = PSTERM_EPS;
			level1 = FALSE; 
			level3 = FALSE;
			color = FALSE;
			blacktext = TRUE;
			solid = FALSE;
			dash_length = 1.0f;
			linewidth_factor = 1.0f;
			pointscale_factor = 1.0f;
			duplex_option = FALSE;
			duplex_state = FALSE;
			rounded = FALSE;
			clipped = FALSE;
			fontsize = 0.0f; 
			fontscale = 1.0f;
			useauxfile = FALSE;
			rotate = TRUE;
			palfunc_samples = 2000;
			palfunc_deviation = 0.003;
			oldstyle = FALSE;
			epslatex_standalone = FALSE;
			adobeglyphnames = FALSE;
			STRNSCPY(font, "");
		}
		else if(tt == PSTERM_PSTEX) {
			terminal = PSTERM_PSTEX;
			xoff = 0; 
			yoff = 0;
			psformat = PSTERM_EPS;
			level1 = FALSE; 
			level3 = FALSE;
			color = FALSE;
			blacktext = TRUE;
			solid = FALSE;
			dash_length = 1.0f;
			linewidth_factor = 1.0f;
			pointscale_factor = 1.0f;
			duplex_option = FALSE;
			duplex_state = FALSE;
			rounded = FALSE;
			clipped = FALSE;
			fontsize = 0.0f; 
			fontscale = 1.0f;
			useauxfile = FALSE;
			rotate = TRUE;
			palfunc_samples = 2000;
			palfunc_deviation = 0.003;
			oldstyle = FALSE;
			epslatex_standalone = FALSE;
			adobeglyphnames = FALSE;
			STRNSCPY(font, "");
		}
		background.r = -1.0;
		background.g = -1.0;
		background.b = -1.0;
	}
	enum   PS_TERMINALTYPE terminal;
	int    xoff;
	int    yoff;
	enum   PS_PSFORMAT psformat;
	bool   level1;
	bool   level3;
	bool   color;
	bool   blacktext;
	bool   solid;
	float  dash_length;
	float  linewidth_factor;
	float  pointscale_factor;
	bool   duplex_option;       /* one of duplex or simplex specified? */
	bool   duplex_state;
	bool   rounded;             /* rounded linecaps and linejoins */
	bool   clipped;             /* path clipped to BoundingBox? */
	ps_fontfile_def * first_fontfile;
	char   font[MAX_ID_LEN+1];      /* name of font */
	float  fontsize;                 /* size of font in pts */
	float  fontscale;                /* multiplier for nominal font size */
	bool   useauxfile;          /* only necessary for ps(la)tex */
	bool   rotate;              /* only necessary for ps(la)tex */
	int    palfunc_samples;          /* setable via "palf$uncparam" */
	double palfunc_deviation;     /* terminal option */
	bool   oldstyle;
	bool   epslatex_standalone;
	bool   adobeglyphnames;     /* Choice of output names for UTF8 */
	rgb_color background;
};

struct GpPostscriptBlock {
	GpPostscriptBlock();
	static char * SaveSpace(double gray);
	float  FontSize;
	float  FontSizePrevious;
	//
	// for enhanced mode, we keep a separate font name and size, which
	// is restored to the default value on font of ""
	//
	char   EnhFont[MAX_ID_LEN+1];
	float  EnhFontSize;
	int    EnsPsInitialized;
	int    Page;         /* page count */
	int    PathCount;   /* count of lines in path */
	int    Ang;                  /* text angle */
	enum   JUSTIFY Justify;  /* text is flush left */
	ps_params_t * P_Params;// = &post_params;
	char * PsLatexAuxname; // name of auxiliary file 
};

struct /*file_stats*/GpFileStats {
	long records;
	long blanks;
	long invalid;
	long outofrange;
	long blocks; /* blocks are separated by double blank lines */
	long header_records;
	int columns;
};

//
// Keeps info on a value and its index in the file 
//
struct GpPair {
	double val;
	long index;
};
//
// Collect results from analysis 
//
struct SglColumnStats {
	// Matrix dimensions 
	int    sx;
	int    sy;
	double mean;
	double adev;
	double stddev;
	double ssd;     /* sample standard deviation */
	double skewness;
	double kurtosis;
	double mean_err;
	double stddev_err;
	double skewness_err;
	double kurtosis_err;
	double sum;      /* sum x    */
	double sum_sq;   /* sum x**2 */
	GpPair min;
	GpPair max;
	double median;
	double lower_quartile;
	double upper_quartile;
	double cog_x; /* centre of gravity */
	double cog_y;
	// info on data points out of bounds? 
};

struct TwoColumnStats {
	double sum_xy;
	double slope;    /* linear regression */
	double intercept;
	double slope_err;
	double intercept_err;
	double correlation;
	double pos_min_y; /* x coordinate of min y */
	double pos_max_y; /* x coordinate of max y */
};

struct GpGadgets {
	GpGadgets() : P_FirstCustomDashtype(0), P_FirstLabel(0), P_FirstLineStyle(0), P_FirstPermLineStyle(0), P_FirstMonoLineStyle(0),
		P_FirstArrowStyle(0), P_PixmapListHead(0), P_FirstArrow(0), P_FirstObject(0)
	{
	}
	custom_dashtype_def * P_FirstCustomDashtype; // Pointer to first 'set dashtype' definition in linked list 
	text_label * P_FirstLabel; // Pointer to the start of the linked list of 'set label' definitions 
	linestyle_def * P_FirstLineStyle;
	linestyle_def * P_FirstPermLineStyle;
	linestyle_def * P_FirstMonoLineStyle;
	arrowstyle_def * P_FirstArrowStyle; // Pointer to first 'set style arrow' definition in linked list 
	t_pixmap * P_PixmapListHead; // Listhead for pixmaps 
	arrow_def * P_FirstArrow; // set arrow 
	GpObject  * P_FirstObject; // Pointer to first object instance in linked list 
};
//
// Function pointer type for callback functions to generate ticmarks 
//
typedef void (GnuPlot::*GpTicCallback)(GpAxis *, double, char *, int, lp_style_type, ticmark *);

class GnuPlot {
public:
	GnuPlot() : TermPointSize(1.0), TermInitialised(false), TermGraphics(false), TermSuspended(false), TermOpenedBinary(false), TermForceInit(false)
	{
	}
	void   PlotRequest(termentry * pTerm);

	static curve_points * CpAlloc(int num);
	// 
	// Descr: releases any memory which was previously malloc()'d to hold curve points (and recursively down the linked list).
	// 
	static void CpFree(curve_points * cp);
	static void DoArrow(uint usx, uint usy/* start point */, uint uex, uint uey/* end point (point of arrowhead) */, int headstyle);
	static void DoPoint(uint x, uint y, int number);
	static void LineAndPoint(uint x, uint y, int number);
	static void DoPointSize(double size);
	static void NullSetColor(t_colorspec * pColorSpec);
	static int  NullJustifyText(enum JUSTIFY just);
	static int  NullTextAngle(int ang);
	static void OptionsNull(TERMENTRY * pThis, GnuPlot * pGp);
	static char * _StyleFont(const char * pFontName, bool isBold, bool isItalic);
	//
	// Deprecated terminal function (pre-version 3)
	//
	static int NullScale(double x, double y);
	const  char * AnsiColorString(const t_colorspec * pColor, const t_colorspec * pPrevColor);
	void   InitSession();
	void   EvalPlots(termentry * pTerm);
	void   Eval3DPlots(termentry * pTerm);
	int    ComLine();
	void   DoPlot(termentry * pTerm, curve_points * plots, int pcount);
	void   Do3DPlot(termentry * pTerm, surface_points * plots, int pcount/* count of plots in linked list */, REPLOT_TYPE replot_mode/* replot/refresh/axes-only/quick-refresh */);
	void   DoSave3DPlot(termentry * pTerm, surface_points * pPlots, int pcount, REPLOT_TYPE quick);
	void   DoStringAndFree(char * cmdline);
	void   DoZoom(double xmin, double ymin, double x2min, double y2min, double xmax, double ymax, double x2max, double y2max);
	void   ZoomRescale_XYX2Y2(double a0, double a1, double a2, double a3, double a4, double a5, double a6,
		double a7, double a8, double a9, double a10, double a11, double a12, double a13, double a14, double a15, char msg[]);
	void   ZoomInX(int zoomKey);
	void   DoEvent(termentry * pTerm, gp_event_t * pGe);
	//
	// Return a new upper or lower axis limit that is a linear
	// combination of the current limits.
	//
	double Rescale(int axIdx, double w1, double w2);
	//
	// Return new lower and upper axis limits from expanding current limits
	// relative to current mouse position.
	//
	void   RescaleAroundMouse(double * pNewMin, double * pNewMax, int axIdx, double mouse_pos, double scale);
	void   EvaluateAt(at_type * pAt, GpValue * pVal);
	double EvalLinkFunction(const GpAxis * pAx, double raw_coord);
	void   MultiplotStart();
	void   MultiplotEnd();
	GpValue * FASTCALL PopOrConvertFromString(GpValue * v);
	GpValue * FASTCALL ConstExpress(GpValue * pVal);
	double RealExpression();
	float  FloatExpression();
	intgr_t IntExpression();
	void   IntError(int t_num, const char * pStr, ...);
	void   IntErrorCurToken(const char * pStr, ...);
	void   IntWarn(int t_num, const char * pStr, ...);
	void   IntWarnCurToken(const char * pStr, ...);
	void   Command();
	void   Define();
	void   WhileCommand();
	void   DoCommand();
	void   IfElseCommand(ifstate if_state);
	void   IfCommand();
	void   ElseCommand();
	void   ArrayCommand();
	void   PrintCommand();
	void   SetCommand();
	void   ResetCommand();
	void   UnsetCommand();
	void   FitCommand();
	void   ExitCommand();
	void   ShowCommand();
	void   LinkCommand();
	void   ImportCommand();
	void   VoxelCommand();
	void   RefreshCommand();
	void   VFillCommand();
	void   HistoryCommand();
	void   PauseCommand();
	void   PlotCommand();
	void   SPlotCommand();
	void   ReplotCommand();
	void   ClearCommand();
	void   SaveCommand();
	void   PrintLineWithError(int t_num);
	void   PlotOptionEvery();
	void   PlotOptionUsing(int max_using);
	void   PlotOptionIndex();
	void   PlotOptionBinary(bool setMatrix, bool setDefault);
	void   Plot3DRequest(termentry * pTerm);
	void   RefreshRequest();
	void   StatsRequest();
	int    LpParse(lp_style_type * lp, lp_class destination_class, bool allow_point);
	void   ArrowParse(arrow_style_type * arrow, bool allow_as);
	int    MakePalette(termentry * pTerm);
	double AxisLogValueChecked(AXIS_INDEX axis, double coord, const char * pWhat);
	void   AxisCheckedExtendEmptyRange(AXIS_INDEX axis, const char * mesg);
	void   CloneLinkedAxes(GpAxis * pAx1, GpAxis * pAx2);
	void   SetRgbColorVar(termentry * pTerm, uint rgbvalue);
	void   SetRgbColorConst(termentry * pTerm, uint rgbvalue);
	void   MapPositionR(const termentry * pTerm, GpPosition * pos, double * x, double * y, const char * what);
	void   MapPositionDouble(const termentry * pTerm, GpPosition * pos, double * x, double * y, const char * what);
	void   MapPosition(const termentry * pTerm, GpPosition * pos, int * x, int * y, const char * what);
	void   Map3D_XYZ(double x, double y, double z/* user coordinates */, GpVertex * pOut);
	void   Map3D_XY_double(double x, double y, double z, double * xt, double * yt);
	void   Map3D_XY(double x, double y, double z, int * xt, int * yt);
	int    Map3DGetPosition(GpPosition * pos, const char * what, double * xpos, double * ypos, double * zpos);
	void   Map3DPositionRDouble(GpPosition * pos, double * xx, double * yy, const char * what);
	void   Map3DPositionDouble(GpPosition * pos, double * x, double * y, const char * what);
	void   Map3DPosition(GpPosition * pos, int * x, int * y, const char * what);
	void   Map3DPositionR(GpPosition * pPos, int * x, int * y, const char * what);
	void   ClipVector(termentry * pTerm, int x, int y);
	coord_type PolarToXY(double theta, double r, double * x, double * y, bool update);
	double PolarRadius(double r);
	double Cb2Gray(double cb);
	void   Rgb1FromGray(double gray, rgb_color * pColor);
	double QuantizeGray(double gray);
	void   Rgb1MaxColorsFromGray(double gray, rgb_color * pColor);
	void   Rgb255MaxColorsFromGray(double gray, rgb255_color * rgb255);
	int    CalculateColorFromFormulae(double gray, rgb_color * pColor);
	int    DrawClipLine(termentry * pTerm, int x1, int y1, int x2, int y2);
	void   DrawClipArrow(termentry * pTerm, double dsx, double dsy, double dex, double dey, t_arrow_head head);
	void   Draw3DPointUnconditional(termentry * pTerm, const GpVertex * pV, lp_style_type * pLp);
	void   Draw3DLineUnconditional(termentry * pTerm, const GpVertex * pV1, const GpVertex * pV2, lp_style_type * lp, t_colorspec color);
	void   Draw3DLine(termentry * pTerm, GpVertex * v1, GpVertex * v2, lp_style_type * lp);
	void   DrawPolarCircle(termentry * pTerm, double place);
	void   GetPositionDefault(GpPosition * pos, enum position_type default_type, int ndim);
	void   GetPosition(GpPosition * pos);
	void   Store2DPoint(curve_points * pPlot, int i/* point number */, double x, double y, double xlow, double xhigh, double ylow, double yhigh, double width/* BOXES widths: -1 -> autocalc, 0 ->  use xlow/xhigh */);
	void   Edge3DIntersect(GpCoordinate * p1, GpCoordinate * p2, double * ex, double * ey, double * ez/* the point where it crosses an edge */);
	bool   TwoEdge3DIntersect(GpCoordinate * p0, GpCoordinate * p1, double * lx, double * ly, double * lz/* lx[2], ly[2], lz[2]: points where it crosses edges */);
	void   ProcessImage(termentry * pTerm, const void * plot, t_procimg_action action);
	void   ApplyZoom(t_zoom * z);
	void   ApplyPm3DColor(termentry * pTerm, t_colorspec * tc);
	void   TermApplyLpProperties(termentry * pTerm, const lp_style_type * lp);
	void   WriteLabel(termentry * pTerm, int x, int y, text_label * pLabel);
	void   ParseUnaryExpression();
	void   ParseMultiplicativeExpression();
	void   ParseAdditiveExpression();
	int    ParseAssignmentExpression();
	void   ParseLogicalOrExpression();
	void   ParseLogicalAndExpression();
	void   ParseInclusiveOrExpression();
	void   ParseExclusiveOrExpression();
	void   ParseAndExpression();
	void   ParseRelationalExpression();
	void   ParseEqualityExpression();
	int    ParseArrayAssignmentExpression();
	void   ParseConditionalExpression();
	void   ParseExpression();
	int    ParseRange(AXIS_INDEX axis);
	void   ParseColorSpec(t_colorspec * tc, int options);
	void   ParseLabelOptions(text_label * pLabel, int ndim);
	GpSizeUnits ParseTermSize(float * pXSize, float * pYSize, GpSizeUnits default_units);
	GpValue * ConstStringExpress(GpValue * pVal);
	char * TryToGetString();
	char * StringOrExpress(at_type ** ppAt);
	at_type * TempAt();
	void   AcceptMultiplicativeExpression();
	void   AcceptAdditiveExpression();
	void   AcceptBitshiftExpression();
	void   AcceptLogicalOrExpression();
	void   AcceptLogicalAndExpression();
	void   AcceptInclusiveOrExpression();
	void   AcceptExclusiveOrExpression();
	void   AcceptAndExpression();
	void   AcceptRelationalExpression();
	void   AcceptEqualityExpression();
	void   ResetPalette();
	void   SetColorSequence(int option);
	void   SetCntrParam();
	void   SetAutoscale();
	void   SaveSetAll(FILE * fp);
	void   UnsetPolar();
	void   UnsetStyle();
	void   TestTerminal(termentry * pTerm);
	char * DfGeneratePseudodata();
	int    IsBuiltinFunction(int t_num) const;
	int    IsFunction(int t_num) const;
	int    IsDefinition(int t_num) const;
	bool   MightBeNumeric(int t_num) const;
	void   FASTCALL _ExecuteAt2(at_type * pAt);
	void   UpdateGpvalVariables(int context);
	GpIterator * CheckForIteration();
	void   PrepareCall(int calltype);
	int    DfOpen(const char * pCmdFileName, int maxUsing, curve_points * pPlot);
	// C-callable versions of internal gnuplot functions word() and words() 
	char * Gp_Word(char * string, int i);
	int    Gp_Words(char * string);
	char * GetAnnotateString(char * s, double x, double y, int mode, char * fmt);
	udvt_entry * AddUdv(int t_num);
	udvt_entry * GetColorMap(int token);
	void   PixMapFromColorMap(t_pixmap * pPixmap);
	int    GetMultiplotCurrentPanel() const;
	void   OutputNumber(double coord, int axIdx, char * pBuffer);
	void   LoadMouseVariables(double x, double y, bool button, int c);
	void   MousePosToGraphPosReal(int xx, int yy, double * x, double * y, double * x2, double * y2);
	void   RecalcRulerPos();
	void   UpdateStatusLine();
	void   DoStringReplot(const char * pS);
	void   IlluminateOneQuadrangle(Quadrangle * q);
	int    ReportError(int ierr);
	void   LfPush(FILE * fp, char * name, char * cmdline);
	bool   LfPop();
	void   PrintfValue(char * pOutString, size_t count, const char * pFormat, double log10_base, const GpValue * pV);
	void   GPrintf(char * pOutString, size_t count, const char * pFormat, double log10_base, double x);
	void   TermReset(termentry * pTerm);
	void   TermSetOutput(termentry * pTerm, char * pDest);
	int    DfReadLine(double v[], int maxSize);
	
	GpStack EvStk;
	GpEval Ev;
	GpAxisSet AxS;
	GpProgram Pgm;
	t_sm_palette SmPltt;
	GpGadgets Gg;
	GpView V;
	MpLayout_ MpLo;// = MP_LAYOUT_DEFAULT;
	GpTabulate Tab;
	GpEnhancedText Enht;
	TermDumbBlock TDumbB;
	GpPostscriptBlock TPsB;
	double TermPointSize;
	bool   TermInitialised; // true if terminal has been initialized 
	bool   TermGraphics;   //= false; // true if terminal is in graphics mode 
	bool   TermSuspended;  //= false; // we have suspended the driver, in multiplot mode 
	bool   TermOpenedBinary; //= false; // true if? 
	bool   TermForceInit;  //= false; // true if require terminal to be initialized 
private:
	termentry * SetTerm();
	void   TermInitialise(termentry * pTerm);
	void   TermStartPlot(termentry * pTerm);
	void   TermEndPlot(termentry * pTerm);
	void   TermStartMultiplot(termentry * pTerm);
	void   TermEndMultiplot(termentry * pTerm);
	void   TermCheckMultiplotOkay(bool fInteractive);
	uint   RgbFromColorspec(t_colorspec * tc);
	void   ReplotRequest(termentry * pTerm);
	void   Boundary(termentry * pTerm, curve_points * plots, int count);
	void   Boundary3D(termentry * pTerm, const surface_points * plots, int count);
	void   DoKeyBounds(termentry * pTerm, legend_key * pKey);
	void   DoKeyLayout(termentry * pTerm, legend_key * pKey);
	void   BoxPlotRangeFiddling(curve_points * pPlot);
	void   PlotBorder(termentry * pTerm);
	void   PlotSteps(termentry * pTerm, curve_points * plot);
	void   PlotFSteps(termentry * pTerm, curve_points * pPlot);
	void   PlotHiSteps(termentry * pTerm, curve_points * pPlot);
	void   PlotLines(termentry * pTerm, curve_points * pPlot);
	void   PlotFilledCurves(termentry * pTerm, curve_points * pPlot);
	void   PlotCBars(termentry * pTerm, curve_points * pPlot);
	void   PlotFBars(termentry * pTerm, curve_points * pPlot);
	void   PlotBars(termentry * pTerm, curve_points * plot);
	void   PlotBetweenCurves(termentry * pTerm, curve_points * plot);
	void   PlotBoxes(termentry * pTerm, curve_points * plot, int xaxis_y);
	void   PlotPoints(termentry * pTerm, curve_points * plot);
	void   PlotVectors(termentry * pTerm, curve_points * plot);
	void   PlaceGrid(termentry * pTerm, int layer);
	void   PlaceLabels(termentry * pTerm, text_label * pListHead, int layer, bool clip);
	void   PlaceRAxis(termentry * pTerm);
	void   PlaceParallelAxes(termentry * pTerm, curve_points * pFirstPlot, int layer);
	void   PlaceArrows3D(termentry * pTerm, int layer);
	void   PlaceArrows(termentry * pTerm, int layer);
	void   PlaceHistogramTitles(termentry * pTerm);
	void   PlaceObjects(termentry * pTerm, GpObject * pListHead, int layer, int dimensions);
	void   Plot3DBoxErrorBars(termentry * pTerm, surface_points * plot);
	void   PlotOptionMultiValued(df_multivalue_type type, int arg);
	void   PlotOptionArray();
	void   PlotTicLabelUsing(int axis);
	void   PlotDots(termentry * pTerm, const curve_points * pPlot);
	void   PlotBoxPlot(termentry * pTerm, curve_points * pPlot, bool onlyAutoscale);
	void   PlotSpiderPlot(termentry * pTerm, curve_points * pPlot);
	void   Plot3DVectors(termentry * pTerm, surface_points * pPlot);
	void   Plot3DBoxes(surface_points * pPlot);
	void   PlacePixmaps(termentry * pTerm, int layer, int dimensions);
	void   PlaceTitle(termentry * pTerm, int titleX, int titleY);
	void   PlotEllipses(termentry * pTerm, curve_points * pPlot);
	void   PlotParallel(termentry * pTerm, curve_points * pPlot);
	void   PlotCircles(termentry * pTerm, curve_points * pPlot);
	void   DoArc(termentry * pTerm, int cx, int cy/* Center */, double radius, double arc_start, double arc_end/* Limits of arc in degrees */, int style, bool wedge);
	void   KeySampleFill(termentry * pTerm, int xl, int yl, struct surface_points * pPlot);
	void   KeySamplePointPm3D(termentry * pTerm, surface_points * pPlot, int xl, int yl, int pointtype);
	void   KeySampleLinePm3D(termentry * pTerm, surface_points * pPlot, int xl, int yl);
	void   KeySampleLine(termentry * pTerm, int xl, int yl);
	void   KeySamplePoint(termentry * pTerm, surface_points * pPlot, int xl, int yl, int pointtype);
	void   DrawVertex(termentry * pTerm, GpVertex * pV);
	void   DrawEdge(termentry * pTerm, GpEdge * e, GpVertex * v1, GpVertex * v2);
	void   JitterPoints(const termentry * pTerm, curve_points * pPlot);
	void   PlotImpulses(termentry * pTerm, curve_points * pPlot, int yaxis_x, int xaxis_y);
	void   CpImplode(curve_points * pCp);
	void   FilledPolygon(termentry * pTerm, gpdPoint * corners, int fillstyle, int nv);
	void   Pm3DDepthQueueFlush(termentry * pTerm);
	void   Pm3DPlot(termentry * pTerm, surface_points * pPlot, int at_which_z);
	void   Pm3DDrawOne(termentry * pTerm, surface_points * pPlot);
	void   Plot3DZErrorFill(termentry * pTerm, surface_points * pPlot);
	void   Plot3DPolygons(termentry * pTerm, surface_points * pPlot);
	void   Plot3DLines(termentry * pTerm, surface_points * pPlot);
	void   Plot3DImpulses(termentry * pTerm, surface_points * pPlot);
	void   Plot3DHidden(termentry * pTerm, surface_points * plots, int pcount);
	bool   CheckForVariableColor(termentry * pTerm, const curve_points * pPlot, const double * pColorValue);
	void   Check3DForVariableColor(termentry * pTerm, surface_points * pPlot, GpCoordinate * pPoint);
	void   Cntr3DLabels(termentry * pTerm, gnuplot_contours * cntr, char * pLevelText, text_label * pLabel);
	void   Сntr3DImpulses(termentry * pTerm, gnuplot_contours * cntr, lp_style_type * lp);
	void   Cntr3DPoints(termentry * pTerm, gnuplot_contours * cntr, lp_style_type * lp);
	void   Сntr3DLines(termentry * pTerm, gnuplot_contours * cntr, struct lp_style_type * lp);
	void   Polyline3DStart(termentry * pTerm, GpVertex * v1);
	void   Polyline3DNext(termentry * pTerm, GpVertex * v2, lp_style_type * lp);
	void   GetArrow(termentry * pTerm, arrow_def * pArrow, double * pSx, double * pSy, double * pEx, double * pEy);
	void   DrawKey(termentry * pTerm, legend_key * pKey, bool keyPass);
	void   DoKeySample(termentry * pTerm, const curve_points * pPlot, legend_key * pKey, char * title, coordval var_color);
	void   DoKeySamplePoint(termentry * pTerm, curve_points * pPlot, legend_key * pKey);
	void   DoRectangle(termentry * pTerm, int dimensions, t_object * pObject, const fill_style_type * pFillStyle);
	void   DoPolygon(termentry * pTerm, int dimensions, t_object * pObject, int style, int facing);
	void   DoEllipse(termentry * pTerm, int dimensions, t_ellipse * pEllipse, int style, bool doOwnMapping);
	void   Do3DCubic(iso_curve * pCurve, enum PLOT_SMOOTH smoothOption);
	void   DoKDensity(curve_points * cp, int firstPoint/* where to start in plot->points */, int num_points/* to determine end in plot->points */, GpCoordinate * dest/* where to put the interpolated data */);
	void   DoBezier(curve_points * cp, double * bc/* Bezier coefficient array */, int firstPoint/* where to start in plot->points */,
		int numPoints/* to determine end in plot->points */, GpCoordinate * pDest/* where to put the interpolated data */);
	void   GenInterp(curve_points * pPlot);
	void   Gen3DSplines(surface_points * pPlot);
	void   McsInterp(curve_points * pPlot);
	void   MakeBins(curve_points * pPlot, int nbins, double binlow, double binhigh, double binwidth);
	void   Draw3DPoint(termentry * pTerm, GpVertex * v, lp_style_type * lp);
	void   DrawLabelHidden(termentry * pTerm, GpVertex * v, lp_style_type * lp, int x, int y);
	void   DrawInsideColorBoxBitmapMixed(termentry * pTerm);
	void   DrawInsideColorBoxBitmapDiscrete(termentry * pTerm);
	void   DrawInsideColorBoxBitmapSmooth(termentry * pTerm);
	void   DrawInsideColorSmoothBoxPostScript();
	void   DrawPolarClipLine(termentry * pTerm, double xbeg, double ybeg, double xend, double yend);
	void   DrawLineHidden(termentry * pTerm, GpVertex * v1, GpVertex * v2/* pointers to the end vertices */, lp_style_type * lp/* line and point style to draw in */);
	void   FinishFilledCurve(termentry * pTerm, int points, gpiPoint * pCorners, curve_points * pPlot);
	void   AxisOutputTics(termentry * pTerm, AXIS_INDEX axis/* axis number we're dealing with */, int * ticlabel_position/* 'non-running' coordinate */,
		AXIS_INDEX zeroaxis_basis/* axis to base 'non-running' position of * zeroaxis on */, GpTicCallback callback/* tic-drawing callback function */);
	void   GenTics(GpAxis * pThis, GpTicCallback callback);
	void   AdjustOffsets();
	void   AdjustNonlinearOffset(GpAxis * pAxSecondary);
	void   CheckCornerHeight(GpCoordinate * p, double height[2][2], double depth[2][2]);
	int    Get3DData(termentry * pTerm, surface_points * pPlot);
	void   GridNonGridData(surface_points * pPlot);
	bool   GetArrow3D(termentry * pTerm, arrow_def * pArrow, double * pDsx, double * pDsy, double * pDex, double * pDey);
	void   CalculateSetOfIsoLines(AXIS_INDEX valueAxIdx, bool cross, iso_curve ** ppThisIso, AXIS_INDEX iso_axis,
		double isoMin, double isoStep, int numIsoToUse, AXIS_INDEX samAxIdx, double samMin, double samStep, int numSamToUse);
	void   Setup3DBoxCorners();
	void   PlaceSpiderPlotAxes(termentry * pTerm, curve_points * pFirstPlot, int layer);
	void   AutoscaleBoxPlot(termentry * pTerm, curve_points * pPlot);
	void   AxisDraw2DZeroAxis(termentry * pTerm, AXIS_INDEX axis, AXIS_INDEX crossaxis);
	void   ResetTextColor(termentry * pTerm, const t_colorspec * tc);
	void   SetLogScale();
	void   SetMargin(GpPosition * pMargin);
	void   SetPalette();
	void   SetColorMap();
	void   SetColorMapRange();
	void   SetObj(int tag, int objType);
	void   SetObject();
	void   SetStyle();
	void   SetKey();
	void   SetPaletteFunction();
	void   SetMouse();
	void   SetTicProp(GpAxis * pThisAxis);
	void   SetPAxis();
	void   SetTics();
	void   SetTicScale();
	void   SetTimeStamp();
	void   SetArrowStyle();
	void   SetDataFile();
	void   DfSetDataFileBinary();
	void   SetTimeData(GpAxis * pAx);
	void   SetRange(GpAxis * pAx);
	void   SetCbMinMax();
	void   SetOverflow();
	void   SetVGridRange();
	void   SaveWritebackAllAxes();
	void   SaveZeroAxis(FILE * fp, AXIS_INDEX axis);
	void   SetFit();
	void   SetCntrLabel();
	void   SetContour();
	void   SetBoxPlot();
	void   ResetLogScale(GpAxis * pAx);
	void   EventPlotDone(termentry * pTerm);
	void   UpdateSecondaryAxisRange(GpAxis * pAxPrimary);
	void   ReconcileLinkedAxes(GpAxis * pAxPrimary, GpAxis * pAxSecondary);
	double MapX3D(double x);
	double MapY3D(double y);
	double MapZ3D(double z);
	void   UpdateRuler(termentry * pTerm);
	int    GetData(curve_points * pPlot);
	void   RefreshBounds(curve_points * pFirstPlot, int nplots);
	void   SpiderPlotRangeFiddling(curve_points * plot);
	void   ParsePrimaryExpression();
	void   ParseSumExpression();
	void   ParseBitshiftExpression();
	void   ParseLightingOptions();
	int    ParseDashType(t_dashtype * dt);
	void   ParsePlotTitle(curve_points * pPlot, char * pXTitle, char * pYTitle, bool * pSetTitle);
	void   ParseFillStyle(fill_style_type * fs);
	void   ParseHistogramStyle(histogram_style * pHs, t_histogram_type defType, int defGap);
	void   ParseKDensityOptions(curve_points * pPlot);
	void   ParseLinkVia(udft_entry * pUdf);
	at_type * PermAt();
	udft_entry * AddUdf(int t_num);
	void   ResetIteration(GpIterator * iter);
	bool   NextIteration(GpIterator * iter);
	void   ReevaluateIterationLimits(GpIterator * iter);
	void   GetPositionType(enum position_type * type, AXIS_INDEX * axes);
	void   InitColor();
	void   SetTerminal();
	int    SetPaletteDefined();
	void   SetColorBox();
	void   SetPolar();
	void   SetPm3D();
	void   SetArrow();
	void   SetAllZeroAxis();
	void   SetZeroAxis(AXIS_INDEX axis);
	void   SetDashType();
	void   SetPaletteFile();
	void   SetPaletteColorMap();
	void   SetOffsets();
	void   SetAngles();
	void   SetView();
	void   SetFormat();
	void   SetGrid();
	void   SetHidden3D();
	void   SetHidden3DOptions();
	void   SetHistory();
	void   SetIsoSurface();
	void   SetTheta();
	void   SetMonochrome();
	void   SetDataFileCommentsChars();
	void   SetTable();
	void   SetSeparator(char ** ppSeparators);
	void   SetJitter();
	void   SetWall();
	void   SetStyleSpiderPlot();
	void   SetStyleParallel();
	void   SetLineStyle(linestyle_def ** ppHead, lp_class destinationClass);
	void   SetSize();
	bool   SetAutoscaleAxis(GpAxis * pThis);
	void   SetBars();
	void   SetPixMap();
	void   SetIsoSamples();
	void   SetSpiderPlot();
	void   SetSurface();
	void   SetTermOptions(termentry * pTerm);
	void   SetMapping();
	void   SetPlotWithPalette(int plotNum, int plotMode);
	void   SetClip();
	void   SetBorder();
	void   SetBoxDepth();
	void   SetBoxWidth();
	void   SetDGrid3D();
	void   SetDummy();
	void   SetVGrid();
	void   SetXYZLabel(text_label * pLabel);
	void   SetPrint();
	void   SetEncoding();
	void   SetLabel();
	void   SetMissing();
	void   SetOutput();
	void   LoadTics(GpAxis * pAx);
	void   LoadTicUser(GpAxis * pAx);
	bool   GridMatch(AXIS_INDEX axis, const char * pString);
	void   CheckPaletteGrayscale();
	void   UpdatePlotBounds(termentry * pTerm);
	void   UnsetAxisLabel(AXIS_INDEX axis);
	void   UnsetTimeData(AXIS_INDEX axis);
	void   UnsetLogScale();
	void   UnsetAutoScale();
	void   UnsetSize();
	void   UnsetClip();
	void   UnsetFit();
	void   UnsetTerminal();
	void   UnsetParametric();
	void   UnsetDummy();
	void   UnsetSpiderPlot();
	void   UnsetAllZeroAxes();
	void   UnsetZeroAxis(AXIS_INDEX axis);
	void   UnsetRange(AXIS_INDEX axis);
	void   UnsetArrow();
	void   UnsetMonochrome();
	void   UnsetLineType();
	void   UnsetLabel();
	void   UnsetTable();
	void   UnsetOutput();
	void   NewColorMap();
	void   RRangeToXY();
	void   ShowAll();
	void   ShowAt();
	void   ShowFormat();
	void   ShowPAxis();
	void   ShowDataFile();
	void   ShowPalette();
	void   ShowZeroAxis(AXIS_INDEX axis);
	void   ShowMargin();
	void   ShowStyle();
	void   ShowLogScale();
	void   ShowPalette_RgbFormulae();
	void   ShowPalette_Fit2RgbFormulae();
	void   ShowPalette_Palette();
	void   ShowPalette_Gradient();
	void   ShowLink();
	void   ShowGrid();
	void   ShowFit();
	void   ShowTable();
	void   InitializePlotStyle(curve_points * pPlot);
	void   DfDetermineMatrix_info(FILE * fin);
	void   DfSetKeyTitleColumnHead(const curve_points * pPlot);
	void   AdjustBinaryUseSpec(curve_points * pPlot);
	int    Token2Tuple(double * pTuple, int dimension);
	void   DrawTitles(termentry * pTerm);
	void   Draw3DGraphBox(termentry * pTerm, const surface_points * pPlot, int plotNum, WHICHGRID whichgrid, int currentLayer);
	void   DrawColorSmoothBox(termentry * pTerm, int plotMode);
	void   Plot3DLinesPm3D(termentry * pTerm, surface_points * pPlot);
	void   Plot3DPoints(termentry * pTerm, surface_points * pPlot);
	void   VFill(t_voxel * pGrid, bool gridCoordinates);
	void   UpdatePrimaryAxisRange(GpAxis * pAxSecondary);
	void   ExtendPrimaryTicRange(GpAxis * pAx);
	void   CheckGridRanges();
	void   RecheckRanges(curve_points * pPlot);
	void   LoadOneRange(GpAxis * pAx, double * pA, t_autoscale * pAutoscale, t_autoscale which);
	t_autoscale LoadRange(GpAxis * pAx, double * pA, double * pB, t_autoscale autoscale);
	//
	void   CmplxDivide(double a, double b, double c, double d, GpValue * result);
	bool   IsArrayAssignment();
	void   MultiplotNext();
	void   MultiplotPrevious();
	void   MultiplotReset();
	void   MpLayoutSizeAndOffset();
	void   MpLayoutMarginsAndSpacing();
	void   MpLayoutSetMarginOrSpacing(GpPosition * pMargin);
	double JDist(const GpCoordinate * pi, const GpCoordinate * pj) const;
	void   UpdateStatusLineWithMouseSetting(termentry * pTerm, mouse_setting_t * ms);
	void   SetExplicitRange(GpAxis * pAx, double newmin, double newmax);
	void   GetImageOptions(t_image * image);
	int    GetPm3DAtOption(char * pm3d_where);
	void   Pm3DOptionAtError();
	void   ParametricFixup(curve_points * pStartPlot, int * pPlotNum);
	void   BoxRangeFiddling(const curve_points * pPlot);
	void   ImpulseRangeFiddling(const curve_points * pPlot);
	void   HistogramRangeFiddling(curve_points * pPlot);
	void   ParallelRangeFiddling(const curve_points * pPlot);
	void   PolarRangeFiddling(const curve_points * pPlot);
	void   Refresh3DBounds(termentry * pTerm, surface_points * pFirstPlot, int nplots);
	int    InFront(termentry * pTerm, long edgenum/* number of the edge in elist */, long vnum1, long vnum2/* numbers of its endpoints */, long * firstpoly/* first plist index to consider */);
	gnuplot_contours * Contour(int numIsoLines, iso_curve * pIsoLines);
	void   FillGpValSysInfo();
	t_object * NewObject(int tag, int objectType, t_object * pNew);
	void   AttachTitleToPlot(termentry * pTerm, curve_points * pPlot, legend_key * pkey);
	//
	void   WidestTicCallback(GpAxis * this_axis, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels); // callback
	void   CbTickCallback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid/* linetype or -2 for no grid */, ticmark * userlabels);
	void   XTickCallback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid/* linetype or -2 for none */, ticmark * userlabels);
	void   YTickCallback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels);
	void   ZTickCallback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels);
	void   YTick2DCallback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid/* grid.l_type == LT_NODRAW means no grid */, ticmark * userlabels/* User-specified tic labels */);
	void   XTick2DCallback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid/* grid.l_type == LT_NODRAW means no grid */, ticmark * userlabels/* User-specified tic labels */);
	void   TTickCallback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid/* grid.l_type == LT_NODRAW means no grid */, ticmark * userlabels/* User-specified tic labels */);
	void   SpiderTickCallback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels);
	void   ClearOneVar(const char * pPrefix, const char * pBase);
	void   ClearStatsVariables(const char * pPrefix);
	void   CreateAndStoreVar(const GpValue * pData, const char * pPrefix, const char * pBase, const char * pSuffix);
	void   CreateAndSetVar(double val, const char * pPrefix, const char * pBase, const char * pSuffix);
	void   CreateAndSetIntVar(int ival, const char * pPrefix, const char * pBase, const char * pSuffix);
	void   FileVariables(GpFileStats s, const char * pPrefix);
	void   SglColumnVariables(SglColumnStats s, const char * pPrefix, const char * pSuffix);
	void   TwoColumnVariables(TwoColumnStats s, const char * pPrefix, long n);
	void   PrintTable(curve_points * pPlot, int plotNum);
	void   Print3DTable(int pcount);
	bool   TabulateOneLine(double v[MAXDATACOLS], GpValue str[MAXDATACOLS], int ncols);
	int    DfReadBinary(double v[], int maxSize);
	int    DfReadAscii(double v[], int maxSize);
	//
	void   F_Bool(union argument * x);
	void   F_Jump(union argument * x);
	void   F_Jumpz(union argument * x);
	void   F_Jumpnz(union argument * x);
	void   F_Jtern(union argument * x);
	void   F_Push(union argument *x);
	void   F_Pushc(union argument *x);
	void   F_Pushd1(union argument *x);
	void   F_Pushd2(union argument *x);
	void   F_Pushd(union argument *x);
	void   F_Pop(union argument *x);
	void   F_Assign(union argument * arg);
	void   F_Value(union argument * arg);
	void   F_Int(union argument * /*arg*/);
	void   F_Real(union argument * /*arg*/);
	void   F_Imag(union argument * /*arg*/);
	void   F_Arg(union argument * /*arg*/);
	void   F_Round(union argument * /*arg*/);
	void   F_Floor(union argument * /*arg*/);
	void   F_Ceil(union argument * /*arg*/);
	void   F_Dollars(union argument * x);
	void   F_UMinus(union argument * /*arg*/);
	void   F_Eq(union argument * /*arg*/);
	void   F_Ne(union argument * /*arg*/);
	void   F_Gt(union argument * /*arg*/);
	void   F_Lt(union argument * arg);
	void   F_Ge(union argument * /*arg*/);
	void   F_Le(union argument * arg);
	void   F_LeftShift(union argument * /*arg*/);
	void   F_RightShift(union argument * /*arg*/);
	void   F_Plus(union argument * /*arg*/);
	void   F_Minus(union argument * /*arg*/);
	void   F_Mult(union argument * arg);
	void   F_Div(union argument * arg);
	void   F_Mod(union argument * /*arg*/);
	void   F_Power(union argument * arg);
	void   F_Factorial(union argument * /*arg*/);
	void   F_Concatenate(union argument * /*arg*/);
	void   F_Eqs(union argument * /*arg*/);
	void   F_Nes(union argument * /*arg*/);
	void   F_Column(union argument * arg);
	void   F_Columnhead(union argument * /*arg*/);
	void   F_StringColumn(union argument * /*arg*/);
	void   F_Besi0(union argument *x);
	void   F_Besi1(union argument *x);
	void   F_Besj0(union argument *x);
	void   F_Besj1(union argument *x);
	void   F_Besjn(union argument *x);
	void   F_Besy0(union argument *x);
	void   F_Besy1(union argument *x);
	void   F_Besyn(union argument *x);
	void   F_Besin(union argument * arg);
	void   F_LNot(union argument *x);
	void   F_BNot(union argument *x);
	void   F_LOr(union argument *x);
	void   F_LAnd(union argument *x);
	void   F_BOr(union argument *x);
	void   F_XOr(union argument *x);
	void   F_BAnd(union argument *x);
	void   F_Sum(union argument * arg);
	void   F_Call(union argument *x);
	void   F_Calln(union argument *x);
	void   F_Strlen(union argument *x);
	void   F_Strstrt(union argument *x);
	void   F_Voxel(union argument * x);
	void   F_Abs(union argument * /*arg*/);
	void   F_Sgn(union argument * /*arg*/);
	void   F_Sqrt(union argument * /*arg*/);
	void   F_Exp(union argument * /*arg*/);
	void   F_Log10(union argument * /*arg*/);
	void   F_Log(union argument * /*arg*/);
	void   F_Conjg(union argument * /*arg*/);
	void   F_Sin(union argument * /*arg*/);
	void   F_Cos(union argument * /*arg*/);
	void   F_Tan(union argument * /*arg*/);
	void   F_ASin(union argument * /*arg*/);
	void   F_ACos(union argument * /*arg*/);
	void   F_ATan(union argument * /*arg*/);
	void   F_ATan2(union argument * /*arg*/);
	void   F_Sinh(union argument * /*arg*/);
	void   F_Cosh(union argument * /*arg*/);
	void   F_Tanh(union argument * /*arg*/);
	void   F_ASinh(union argument * arg);
	void   F_ACosh(union argument * arg);
	void   F_ATanh(union argument * arg);
	void   F_EllipFirst(union argument * arg);
	void   F_EllipSecond(union argument * /*arg*/);
	void   F_EllipThird(union argument * arg);
	void   F_TmWeek(union argument * /*arg*/);
	void   F_TmSec(union argument * /*arg*/);
	void   F_TmMin(union argument * /*arg*/);
	void   F_TmHour(union argument * /*arg*/);
	void   F_TmMDay(union argument * /*arg*/);
	void   F_TmMon(union argument * /*arg*/);
	void   F_TmYear(union argument * /*arg*/);
	void   F_TmWDay(union argument * /*arg*/);
	void   F_TmYDay(union argument * /*arg*/);
	void   F_Exists(union argument * /*arg*/);
	void   F_GPrintf(union argument *x);
	void   F_Range(union argument *x);
	void   F_Index(union argument *x);
	void   F_Cardinality(union argument *x);
	void   F_SPrintf(union argument *x);
	void   F_Word(union argument *x);
	void   F_Words(union argument *x);
	void   F_StrFTime(union argument *x);
	void   F_StrPTime(union argument *x);
	void   F_Time(union argument *x);	
	void   F_System(union argument * arg);
	void   F_Trim(union argument * arg);
	void   F_Erf(union argument * x);
	void   F_Erfc(union argument * x);
	void   F_IBeta(union argument * x);
	void   F_Gamma(union argument * x);
#ifndef HAVE_COMPLEX_FUNCS
	void   F_IGamma(union argument * x);
#endif
	void   F_LGamma(union argument * x);
	void   F_Rand(union argument * x);
	void   F_Normal(union argument * x);
	void   F_InverseIBeta(union argument * arg);
	void   F_InverseIGamma(union argument * /*arg*/);
	void   F_InverseNormal(union argument * /*arg*/);
	void   F_InverseErf(union argument * /*arg*/);
	void   F_SynchrotronF(union argument * x);
	void   F_Airy(union argument * arg);
	void   F_Lambertw(union argument * /*arg*/);
	void   F_ExpInt(union argument * /*arg*/);
	// 
	// Support for user-callable routines
	// 
	void   F_Hsv2Rgb(union argument *);
	void   F_RgbColor(union argument *);
	void   F_Palette(union argument *);
	void   F_TimeColumn(union argument * arg);
	void   F_Valid(union argument * arg);
	void   F_Voigt(union argument * /*arg*/);
};

extern GnuPlot GPO; // @global
//
// Terminal support
//
// 
// Unicode escape sequences (\U+hhhh) are handling by the enhanced text code.
// Various terminals check every string to see whether it needs enhanced text
// rocessing. This macro allows them to include a check for the presence of
// unicode escapes.
// 
#define contains_unicode(S) strstr(S, "\\U+") // moved from term.h
size_t FASTCALL strwidth_utf8(const char * s);

