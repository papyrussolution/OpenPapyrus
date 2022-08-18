// GNUPLOT.H
//
#include <slib.h>

//#define DEBUG
#define BITMAPDEBUG

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
#define HAVE_LUA
//#define HAVE_WEBP // @v11.3.7
//#define HAVE_CAIROPDF
//#define HAVE_CAIROEPS
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
//#define PIPES

//#include <signal.h>
//#include <setjmp.h>
#include <memory.h>
//#ifdef HAVE_WCHAR_H
	//#include <wchar.h>
//#endif
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

int gpfputc(int ch, FILE * pF); // @construction
int gpputs(const char * pStr); // @construction
int gpfputs(const char * pStr, FILE * pF); // @construction
int gpprintf(FILE * pF, const char * pFmt, ...); // @construction

const int GpResolution = 72; // resolution in dpi for converting pixels to size units 

//struct TERMENTRY_Removed;
//#define termentry_Removed TERMENTRY_Removed
struct GpTermEntry;
class  GnuPlot;
struct GpSurfacePoints;
struct GpMeshTriangle;
struct at_type;

extern const char gnuplot_version[];
extern const char gnuplot_patchlevel[];
extern const char gnuplot_date[];
extern const char gnuplot_copyright[];
extern const char faq_location[];
extern const char bug_email[];
extern const char help_email[];
extern char * compile_options;

//#include <gp_types.h>
#define MAX_ID_LEN 50     // max length of an identifier 
#define MAX_LINE_LEN 1024 // maximum number of chars allowed on line 
//#define DEG2RAD_Removed (SMathConst::Pi / 180.0)
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
	PLOT_STYLE_BITS  = (1<<6)
};

enum PLOT_STYLE {
	LINES        =  0*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
	POINTSTYLE   =  1*PLOT_STYLE_BITS + PLOT_STYLE_HAS_POINT,
	IMPULSES     =  2*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
	LINESPOINTS  =  3*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_POINT | PLOT_STYLE_HAS_LINE),
	DOTS =  4*PLOT_STYLE_BITS + 0,
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

typedef float t_voxel;

struct VGrid {
	int    size; // size x size x size array
	double vxmin;
	double vxmax;
	double vxdelta;
	double vymin;
	double vymax;
	double vydelta;
	double vzmin;
	double vzmax;
	double vzdelta;
	double min_value; // min non-zero voxel value
	double max_value; // max voxel value
	double mean_value; // mean non-zero voxel value
	double stddev; // esd over non-zero voxels
	double sum;
	int    nzero;
	t_voxel * vdata; // points to 3D array of voxels
};

struct GpValue {
	GpValue & Init(DATA_TYPES dtyp)
	{
		Type = dtyp;
		memzero(&v, sizeof(v));
		return *this;
	}
	void   Destroy();
	int    IntCheck() const;
	void   SetNotDefined() { Type = NOTDEFINED; }
	//
	// Descr: count number of lines in a datablock 
	//
	int    GetDatablockSize() const;
	enum DATA_TYPES Type;
	union {
		intgr_t int_val;
		cmplx  cmplx_val;
		char * string_val;
		char ** data_array;
		GpValue * value_array;
		VGrid * vgrid;
	} v;
};
//
// Defines the type of a coordinate 
// INRANGE and OUTRANGE points have an x,y point associated with them 
//
enum coord_type {
	INRANGE, // inside plot boundary 
	OUTRANGE, // outside plot boundary, but defined 
	UNDEFINED, // not defined at all 
	EXCLUDEDRANGE // would be inside plot, but excluded for other reasons e.g. in polar mode and outside of trange[tmin:tmax] 
};
// 
// These are aliases of fields in 'GpCoordinate' used to hold
// extra properties of 3D data points (i.e. anything other than x/y/z)
// or of specific plot types.
// The aliases are needed because the total number of data slots is limited
// to 7: x y z xlow ylow xhigh yhigh
// At some point we may need to expand GpCoordinate.
// 
#define CRD_R yhigh        // Used by splot styles RGBIMAGE and RGBA_IMAGE
#define CRD_G xlow         // Used by splot styles RGBIMAGE and RGBA_IMAGE
#define CRD_B xhigh        // Used by splot styles RGBIMAGE and RGBA_IMAGE
#define CRD_A ylow         // Used by splot styles RGBIMAGE and RGBA_IMAGE
#define CRD_COLOR yhigh    // Used by all splot styles with variable color
#define CRD_ROTATE ylow    // Used by "with labels"
#define CRD_PTSIZE xlow    // Used by "with points|linespoints|labels"
#define CRD_PTTYPE xhigh   // Used by "with points|linespoints|labels"
#define CRD_PTCHAR ylow    // Used by "with points pt variable
#define CRD_ZLOW xlow      // Used by splot style ZERRORFILL
#define CRD_ZHIGH xhigh    // Used by splot style ZERRORFILL
#define CRD_XJITTER xlow   // Used to hold jitter offset on x
#define CRD_YJITTER yhigh  // Used to hold jitter offset on y
#define CRD_PATH xhigh     // Used by 3D spline code to hold path coordinate
#define PATHCOORD 6        // must match sequence order of field CRD_PATH

struct GpCoordinate {
	enum CtrBlank {
		ctrBlank = 1
	};
	GpCoordinate() : type(UNDEFINED), ylow(0.0), yhigh(0.0), xlow(0.0), xhigh(0.0)
	{
	}
	GpCoordinate(CtrBlank) : type(UNDEFINED), Pt(-999.0), ylow(-999.0), yhigh(-999.0), xlow(-999.0), xhigh(-999.0)
	{
	}
	enum coord_type type; // see above 
	SPoint3R Pt;
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
	DT_NORMAL = 0,          // default; treat values as pure numeric
	DT_TIMEDATE,            // old datatype
	DT_DMS,                 // degrees minutes seconds
	DT_UNINITIALIZED,
	DT_BAD                  // something went wrong (e.g. in gstrptime)
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
	#define SEC_OFFS_SYS	946684800.0	/* zero gnuplot (2000) - zero system (1970) */
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
// (replaced with SSECSPERDAYr) #define DAY_SEC 86400.0
//
// Prototypes of functions exported by time.c 
//
double gtimegm(struct tm *); /* *tm to seconds */
//
//#include <gp_hist.h>
#define HISTORY_SIZE 500
// 
// Variables of history.c needed by other modules:
// 
typedef void * histdata_t;

struct HIST_ENTRY {
	char * line;
	histdata_t data;
	HIST_ENTRY * prev;
	HIST_ENTRY * next;
};
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
	void using_history();
	histdata_t free_history_entry(HIST_ENTRY * histent);
#endif
//
//#include <eval.h>

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
	udft_entry * next_udf; // pointer to next udf in linked list
	char * udf_name; // name of this function entry
	at_type * at; // pointer to action table to execute
	char * definition; // definition of function as typed
	int    dummy_num; // required number of input variables
	GpValue dummy_values[MAX_NUM_VAR]; // current value of dummy variables
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
	enum operators index; /* index of p-code function */
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
		OverflowHandling(INT64_OVERFLOW_TO_FLOAT), IsUndefined_(false), RecursionDepth(0)
	{
	}
	udvt_entry * AddUdvByName(const char * pKey);
	udvt_entry * GetUdvByName(const char * pKey);
	//void   DelUdvByName(const char * pKey, bool wildcard);
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
	int    RecursionDepth;
	bool   IsUndefined_;
};
//
// Prototypes of functions exported by eval.c 
//
double gp_exp(double x);
GpValue * Gcomplex(GpValue *, double, double);
GpValue * FASTCALL Ginteger(GpValue *, intgr_t);
GpValue * FASTCALL Gstring(GpValue *, char *);
void FASTCALL gpfree_string(GpValue * a);
void gpfree_array(GpValue * a);
void FASTCALL real_free_at(at_type * at_ptr);
// Wrap real_free_at in a macro 
#define free_at(at_ptr) do { real_free_at(at_ptr); at_ptr = NULL; } while(0)
//
//#include <parse.h>
at_type * create_call_column_at(char *);
at_type * create_call_columnhead();
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
//
// These are used by the iteration code 
//
bool empty_iteration(GpIterator *);
bool forever_iteration(GpIterator *);
GpIterator * cleanup_iteration(GpIterator *);
//
//#include <util.h>
#define NO_CARET (-1) // special token number meaning 'do not draw the "caret"', for int_error and friends: 
#define DATAFILE (-2) // token number meaning 'the error was in the datafile, not the command line' 
char * gp_stradd(const char *, const char *);
// Error message handling 
#if defined(__GNUC__)
	//void os_error(int, const char *, ...) __attribute__((noreturn));
	//void common_error_exit(void) __attribute__((noreturn));
#elif defined(_MSC_VER)
	//__declspec(noreturn) void os_error(int, const char *, ...);
	//__declspec(noreturn) void common_error_exit();
#else
	//void os_error(int, const char *, ...);
	//void common_error_exit();
#endif
void   squash_spaces(char *s, int remain);
bool   existdir(const char *);
bool   existfile(const char *);
char * getusername();
size_t gp_strlen(const char *s);
char * gp_strchrn(const char *s, int N);
bool   streq(const char *a, const char *b);
size_t strappend(char **dest, size_t *size, size_t len, const char *src);
char * texify_title(const char * pTitle, int plot_type);
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
	TC_DEFAULT      = 0,    // Use default color, set separately
	TC_LT   = 1,    // Use the color of linetype <n>
	TC_LINESTYLE    = 2,    // Use the color of line style <n>
	TC_RGB  = 3,    // Explicit RGB triple provided by user
	TC_CB   = 4,    // "palette cb <value>"
	TC_FRAC = 5,    // "palette frac <value>
	TC_Z    = 6,    // "palette z"
	TC_VARIABLE     = 7,    // only used for "tc", never "lc"
	TC_COLORMAP     = 8     // "palette colormap"
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
struct gpiPoint : public SPoint2I {
	//int    x;
	//int    y;
	gpiPoint() : SPoint2I(), style(0)
	{
	}
	gpiPoint(int _x, int _y) : SPoint2I(_x, _y), style(0)
	{
	}
	int    style;
};
//
// color modes
//
enum palette_color_mode {
	SMPAL_COLOR_MODE_NONE = '0',
	SMPAL_COLOR_MODE_GRAY = 'g',  // grayscale only
	SMPAL_COLOR_MODE_RGB = 'r',   // one of several fixed transforms
	SMPAL_COLOR_MODE_FUNCTIONS = 'f', // user defined transforms
	SMPAL_COLOR_MODE_GRADIENT = 'd', // interpolated table: explicitly defined or read from file
	SMPAL_COLOR_MODE_CUBEHELIX = 'c'
};
// 
// color gradient type
// 
enum palette_gradient_type {
	SMPAL_GRADIENT_TYPE_NONE     = 0,
	SMPAL_GRADIENT_TYPE_SMOOTH   = 1,// smooth palette
	SMPAL_GRADIENT_TYPE_DISCRETE = 2, // (full) discrete palette
	SMPAL_GRADIENT_TYPE_MIXED    = 3,// partially discrete palette
};
// 
// Contains a colour in RGB scheme.
// Values of  r, g and b  are all in range [0;1] 
// 
struct rgb_color {
	void   Set(double _r, double _g, double _b)
	{
		r = _r;
		g = _g;
		b = _b;
	}
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
struct gpdPoint : public SPoint3R {
	gpdPoint() : SPoint3R(), c(0.0)
	{
	}
	gpdPoint & operator = (const SPoint3R & rPt)
	{
		static_cast<SPoint3R>(*this) = rPt;
		return *this;
	}
	gpdPoint & operator = (const SPoint2R & rPt)
	{
		x = rPt.x;
		y = rPt.y;
		return *this;
	}
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
	//void   ColorComponentsFromGray(double gray, rgb_color * pColor) const;
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
	double SmallestGradientInterval;
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
	double HSV_offset; /* offset (radians) from colorwheel 0 */
};
// 
// Send current colour to the terminal
// 
void   set_color(GpTermEntry * pTerm, double gray);
// 
// Support for colormaps (named palettes)
// 
uint   rgb_from_colormap(double gray, const udvt_entry * pColorMap);
void   get_colormap_range(const udvt_entry * pColorMap, double * cm_min, double * cm_max);
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
	S_PALETTE_POSITIVE, 
	S_PALETTE_NEGATIVE,
	S_PALETTE_GRAY, 
	S_PALETTE_COLOR, 
	S_PALETTE_RGBFORMULAE,
	S_PALETTE_NOPS_ALLCF, 
	S_PALETTE_PS_ALLCF, 
	S_PALETTE_MAXCOLORS,
	S_PALETTE_COLORMAP, 
	S_PALETTE_DEFINED, 
	S_PALETTE_FILE, 
	S_PALETTE_FUNCTIONS,
	S_PALETTE_MODEL, 
	S_PALETTE_GAMMA, 
	S_PALETTE_CUBEHELIX
};

enum set_pm3d_id {
	S_PM3D_INVALID,
	S_PM3D_AT,
	S_PM3D_INTERPOLATE,
	S_PM3D_SCANSFORWARD, 
	S_PM3D_SCANSBACKWARD, 
	S_PM3D_SCANS_AUTOMATIC,
	S_PM3D_DEPTH,
	S_PM3D_FLUSH, 
	S_PM3D_FTRIANGLES, 
	S_PM3D_NOFTRIANGLES,
	S_PM3D_CLIP_Z, 
	S_PM3D_CLIP_1IN, 
	S_PM3D_CLIP_4IN, 
	S_PM3D_CLIPCB, 
	S_PM3D_NOCLIPCB,
	S_PM3D_MAP, 
	S_PM3D_BORDER, 
	S_PM3D_NOBORDER, 
	S_PM3D_HIDDEN, 
	S_PM3D_NOHIDDEN,
	S_PM3D_SOLID, 
	S_PM3D_NOTRANSPARENT, 
	S_PM3D_NOSOLID, 
	S_PM3D_TRANSPARENT,
	S_PM3D_IMPLICIT, 
	S_PM3D_NOEXPLICIT, 
	S_PM3D_NOIMPLICIT, 
	S_PM3D_EXPLICIT,
	S_PM3D_WHICH_CORNER,
	S_PM3D_LIGHTING_MODEL, 
	S_PM3D_NOLIGHTING_MODEL
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
extern const struct gen_table * user_color_names_tbl;
extern const struct gen_table * pm3d_color_names_tbl;
extern const int num_predefined_colors;
extern int num_userdefined_colors;

extern const struct gen_table filledcurves_opts_tbl[];
//
// Function prototypes 
//
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
	lp_style_type(int def = defCommon);
	void   SetDefault();
	void   SetDefault2(); // DEFAULT_LP_STYLE_TYPE
	void   SetDefaultKeybox();
	int    flags;      // e.g. LP_SHOW_POINTS 
	int    l_type;
	int    PtType;
	int    d_type;     // Dashtype 
	int    p_interval; // Every Nth point in style LINESPOINTS 
	int    p_number;   // specify number of points in style LINESPOINTS 
	double l_width;
	double PtSize;
	char   p_char[8]; // string holding UTF-8 char used if p_type = PT_CHARACTER 
	t_colorspec pm3d_color;
	t_dashtype CustomDashPattern; /* per-line, user defined dashtype */
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
	int tag; /* -1 (local), AS_VARIABLE, or style index */
	int layer;                   /* 0 = back, 1 = front */
	lp_style_type lp_properties;
	/* head options */
	t_arrow_head head; /* arrow head choice */
	/* struct GpPosition headsize; */  /* x = length, y = angle [deg] */
	double head_length;     // length of head, 0 = default
	int head_lengthunit;    // unit (x1, x2, screen, graph)
	double head_angle;      // front angle / deg 
	double head_backangle;  // back angle / deg
	arrowheadfill headfill; // AS_FILLED etc
	bool head_fixedsize; /* Adapt the head size for short arrow shafts? */
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
	int    fillstyle;
	int    filldensity;
	int    fillpattern;
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
//
// Operations possible with term->modify_plots() 
//
#define MODPLOTS_SET_VISIBLE         (1<<0)
#define MODPLOTS_SET_INVISIBLE       (1<<1)
#define MODPLOTS_INVERT_VISIBILITIES (MODPLOTS_SET_VISIBLE|MODPLOTS_SET_INVISIBLE)
// 
// Values for the flags field of GpTermEntry
// 
#define TERM_CAN_MULTIPLOT    (1<<0) // tested if stdout not redirected
#define TERM_CANNOT_MULTIPLOT (1<<1) // tested if stdout is redirected 
#define TERM_BINARY           (1<<2) // open output file with "b"      
#define TERM_INIT_ON_REPLOT   (1<<3) // call term->init() on replot    
#define TERM_IS_POSTSCRIPT    (1<<4) // post, next, pslatex, etc       
#define TERM_ENHANCED_TEXT    (1<<5) // enhanced text mode is enabled  
#define TERM_NO_OUTPUTFILE    (1<<6) // terminal doesn't write to a file
#define TERM_CAN_CLIP         (1<<7) // terminal does its own clipping 
#define TERM_CAN_DASH         (1<<8) // terminal supports dashed lines 
#define TERM_ALPHA_CHANNEL    (1<<9) // alpha channel transparency     
#define TERM_MONOCHROME      (1<<10) // term is running in mono mode   
#define TERM_LINEWIDTH       (1<<11) // support for set term linewidth 
#define TERM_FONTSCALE       (1<<12) // terminal supports fontscale    
#define TERM_POINTSCALE      (1<<13) // terminal supports fontscale    
#define TERM_IS_LATEX        (1<<14) // text uses TeX markup           
#define TERM_EXTENDED_COLOR  (1<<15) // uses EXTENDED_COLOR_SPECS      
#define TERM_NULL_SET_COLOR  (1<<16) // no support for RGB color       
#define TERM_POLYGON_PIXELS  (1<<17) // filledpolygon rather than fillbox

//#define GP_VIRTUAL_TERMENTRY
#ifdef GP_VIRTUAL_TERMENTRY
	#define GpTerminalBase GpTermEntry
#else
	struct GpTermEntry;
	typedef GpTermEntry GpTermEntry_Static;
#endif
// 
// The terminal interface structure --- heart of the terminal layer.
// 
// It should go without saying that additional entries may be made
// only at the end of this structure. Any fields added must be
// optional - a value of 0 (or NULL pointer) means an older driver
// does not support that feature - gnuplot must still be able to
// function without that terminal feature
// 
#ifdef GP_VIRTUAL_TERMENTRY
struct GpTermEntry_Static {
#else
struct GpTermEntry {
	void   Mov_(uint x, uint y) { this->move(this, x, y); }
	void   Vec_(uint x, uint y) { this->vector(this, x, y); }
	void   Pnt_(uint x, uint y, int s) { this->point(this, x, y, s); }
	void   Layer_(t_termlayer _layer) { this->layer(this, _layer); }
	void   FillBox_(int style, uint x, uint y, uint width, uint height) { this->fillbox(this, style, x, y, width, height); }
	void   FilledPolygon_(int points, gpiPoint * pCorners) { if(this->filled_polygon) this->filled_polygon(this, points, pCorners); }
	void   SetLineType_(int lt) { this->linetype(this, lt); }
	int    SetTextAngle_(int a) { return this->text_angle(this, a); }
	const char * GetName() const { return name; }
#endif
	const char * name;
	const char * description;
	uint  MaxX;
	uint  MaxY;
	uint  ChrV;
	uint  ChrH;
	uint  TicV;
	uint  TicH;
	void  (*options)(GpTermEntry_Static * pThis, GnuPlot * pGp);
	void  (*init)(GpTermEntry_Static * pThis);
	void  (*reset)(GpTermEntry_Static * pThis);
	void  (*text)(GpTermEntry_Static * pThis);
	int   (*scale)(GpTermEntry_Static * pThis, double, double);
	void  (*graphics)(GpTermEntry_Static * pThis);
	void  (*move)(GpTermEntry_Static * pThis, uint, uint);
	void  (*vector)(GpTermEntry_Static * pThis, uint, uint);
	void  (*linetype)(GpTermEntry_Static * pThis, int);
	void  (*put_text)(GpTermEntry_Static * pThis, uint, uint, const char*);
	// the following are optional. set term ensures they are not NULL 
	int   (*text_angle)(GpTermEntry_Static * pThis, int);
	int   (*justify_text)(GpTermEntry_Static * pThis, enum JUSTIFY);
	void  (*point)(GpTermEntry_Static * pThis, uint, uint, int);
	void  (*arrow)(GpTermEntry_Static * pThis, uint, uint, uint, uint, int headstyle);
	int   (*set_font)(GpTermEntry_Static * pThis, const char * font);
	void  (*pointsize)(GpTermEntry_Static * pThis, double); // change pointsize 
	int   flags;
	void  (*suspend)(GpTermEntry_Static * pThis); // called after one plot of multiplot 
	void  (*resume)(GpTermEntry_Static * pThis);  // called before plots of multiplot 
	void  (*fillbox)(GpTermEntry_Static * pThis, int style, uint x, uint y, uint width, uint height); // clear in multiplot mode 
	void  (*linewidth)(GpTermEntry_Static * pThis, double linewidth);
#ifdef USE_MOUSE
	int   (*waitforinput)(GpTermEntry_Static * pThis, int); // used for mouse and hotkey input 
	void  (*put_tmptext)(GpTermEntry_Static * pThis, int, const char []); // draws temporary text; int determines where: 0=statusline, 1,2: at corners of zoom box, with \r separating text above and below the point 
	void  (*set_ruler)(GpTermEntry_Static * pThis, int, int); // set ruler location; x<0 switches ruler off 
	void  (*set_cursor)(GpTermEntry_Static * pThis, int, int, int); // set cursor style and corner of rubber band 
	void  (*set_clipboard)(GpTermEntry_Static * pThis, const char[]); // write text into cut&paste buffer (clipboard) 
#endif
	int   (*make_palette)(GpTermEntry_Static * pThis, t_sm_palette * pPalette);
	// 
	// 1. if palette==NULL, then return nice/suitable
	// maximal number of colours supported by this terminal.
	// Returns 0 if it can make colours without palette (like postscript).
	// 2. if palette!=NULL, then allocate its own palette return value is undefined
	// 3. available: some negative values of max_colors for whatever can be useful
	// 
	void  (*previous_palette)(GpTermEntry_Static * pThis);
	// 
	// release the palette that the above routine allocated and get
	// back the palette that was active before.
	// Some terminals, like displays, may draw parts of the figure
	// using their own palette. Those terminals that possess only
	// one palette for the whole plot don't need this routine.
	// 
	void  (*set_color)(GpTermEntry_Static * pThis, const t_colorspec *);
	// EAM November 2004 - revised to take a pointer to struct rgb_color,
	// so that a palette gray value is not the only option for specifying color.
	void  (*filled_polygon)(GpTermEntry_Static * pThis, int points, gpiPoint *corners);
	void  (*image)(GpTermEntry_Static * pThis, uint, uint, coordval *, const gpiPoint * pCorners, t_imagecolor);
	// Enhanced text mode driver call-backs 
	void  (*enhanced_open)(GpTermEntry_Static * pThis, const char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
	void  (*enhanced_flush)(GpTermEntry_Static * pThis);
	void  (*enhanced_writec)(GpTermEntry_Static * pThis, int c);
	// Driver-specific synchronization or other layering commands.
	// Introduced as an alternative to the ugly sight of
	// driver-specific code strewn about in the core routines.
	// As of this point (July 2005) used only by pslatex.trm
	void (*layer)(GpTermEntry_Static * pThis, t_termlayer);
	// Begin/End path control.
	// Needed by PostScript-like devices in order to join the endpoints of a polygon cleanly.
	void (*path)(GpTermEntry_Static * pThis, int p);
	// Scale factor for converting terminal coordinates to output
	// pixel coordinates.  Used to provide data for external mousing code.
	double tscale;
	// Pass hypertext for inclusion in the output plot 
	void (*hypertext)(GpTermEntry_Static * pThis, int type, const char * text);
	void (*boxed_text)(GpTermEntry_Static * pThis, uint, uint, int);
	void (*modify_plots)(GpTermEntry_Static * pThis, uint operations, int plotno);
	void (*dashtype)(GpTermEntry_Static * pThis, int type, t_dashtype *custom_dash_pattern);
	//
	uint & CH() { return ChrH; }
	uint & CV() { return ChrV; }
	uint   CH_C() const { return ChrH; }
	uint   CV_C() const { return ChrV; }
	double MulMaxX(double v) const { return (v * MaxX); }
	float  MulMaxX(float v) const { return (v * MaxX); }
	double MulMaxY(double v) const { return (v * MaxY); }
	float  MulMaxY(float v) const { return (v * MaxY); }
	int    MulTicH(int v) const { return (v * TicH); }
	int    MulTicV(int v) const { return (v * TicV); }
	double MulTicH(double v) const { return (v * TicH); }
	double MulTicV(double v) const { return (v * TicV); }
	void   SetCharSize(uint h, uint v)
	{
		ChrH = h;
		ChrV = v;
	}
	void   SetMax(uint x, uint y)
	{
		MaxX = x;
		MaxY = y;
	}
	void   SetTic(uint h, uint v)
	{
		TicH = h;
		TicV = v;
	}
	void   SetTic(uint hv)
	{
		TicH = hv;
		TicV = hv;
	}
	void   SetFlag(int f) { flags |= f; }
	void   ResetFlag(int f) { flags &= ~f; }
	bool   CheckFlag(int f) const { return ((flags & f) != 0); }
	GnuPlot * P_Gp;
};

struct GpTerminalBase { // @construction
public:
	GpTerminalBase();
	virtual ~GpTerminalBase()
	{
	}
	operator GpTermEntry_Static * () { return P_Oit; }
	uint & CH() { return P_Oit->ChrH; }
	uint & CV() { return P_Oit->ChrV; }
	uint   CH_C() const { return P_Oit->ChrH; }
	uint   CV_C() const { return P_Oit->ChrV; }
	double MulMaxX(double v) const { return (v * P_Oit->MaxX); }
	float  MulMaxX(float v) const { return (v * P_Oit->MaxX); }
	double MulMaxY(double v) const { return (v * P_Oit->MaxY); }
	float  MulMaxY(float v) const { return (v * P_Oit->MaxY); }
	int    MulTicH(int v) const { return (v * P_Oit->TicH); }
	int    MulTicV(int v) const { return (v * P_Oit->TicV); }
	double MulTicH(double v) const { return (v * P_Oit->TicH); }
	double MulTicV(double v) const { return (v * P_Oit->TicV); }
	void   SetCharSize(uint h, uint v)
	{
		P_Oit->ChrH = h;
		P_Oit->ChrV = v;
	}
	void   SetMax(uint x, uint y)
	{
		MaxX = x;
		MaxY = y;
	}
	void   SetTic(uint h, uint v)
	{
		TicH = h;
		TicV = v;
	}
	void   SetTic(uint hv)
	{
		TicH = hv;
		TicV = hv;
	}
	void   SetFlag(int f) { P_Oit->flags |= f; }
	void   ResetFlag(int f) { P_Oit->flags &= ~f; }
	bool   CheckFlag(int f) const { return ((P_Oit->flags & f) != 0); }
	void   Mov_(uint x, uint y) { Move(x, y); }
	void   Vec_(uint x, uint y) { Vector(x, y); }
	void   Pnt_(uint x, uint y, int s) { Point(x, y, s); }
	void   Layer_(t_termlayer _layer) { Layer(_layer); }
	void   FillBox_(int style, uint x, uint y, uint width, uint height) { FillBox(style, x, y, width, height); }
	void   FilledPolygon_(int points, gpiPoint * pCorners) { FilledPolygon(points, pCorners); }
	void   SetLineType_(int lt) { LineType(lt); }
	int    SetTextAngle_(int a) { return TextAngle(a); }

	const char * GetName() const { return Name.cptr(); }
	//
	virtual void Options(GnuPlot * pGp);
	virtual void Init();
	virtual void Reset();
	virtual void Text();
	virtual int  Scale(double, double);
	virtual void Graphics();
	virtual void Move(uint, uint);
	virtual void Vector(uint, uint);
	virtual void LineType(int);
	virtual void PutText(uint, uint, const char *);
	// the following are optional. set term ensures they are not NULL 
	virtual int  TextAngle(int);
	virtual int  JustifyText(enum JUSTIFY);
	virtual void Point(uint, uint, int);
	virtual void Arrow(uint, uint, uint, uint, int headstyle);
	virtual int  SetFont(const char * font);
	virtual void PointSize(double); // change pointsize 
	virtual void Suspend(); // called after one plot of multiplot 
	virtual void Resume();  // called before plots of multiplot 
	virtual void FillBox(int style, uint x, uint y, uint width, uint height); // clear in multiplot mode 
	virtual void LineWidth(double linewidth);
#ifdef USE_MOUSE
	virtual int  WaitForInput(int); // used for mouse and hotkey input 
	virtual void PutTmpText(int, const char []); // draws temporary text; int determines where: 0=statusline, 1,2: at corners of zoom box, with \r separating text above and below the point 
	virtual void SetRuler(int, int); // set ruler location; x<0 switches ruler off 
	virtual void SetCursor(int, int, int); // set cursor style and corner of rubber band 
	virtual void SetClipboard(const char[]); // write text into cut&paste buffer (clipboard) 
#endif
	virtual int  MakePalette(t_sm_palette * pPalette);
	// 
	// 1. if palette==NULL, then return nice/suitable
	// maximal number of colours supported by this terminal.
	// Returns 0 if it can make colours without palette (like postscript).
	// 2. if palette!=NULL, then allocate its own palette return value is undefined
	// 3. available: some negative values of max_colors for whatever can be useful
	// 
	virtual void  PreviousPalette();
	// 
	// release the palette that the above routine allocated and get
	// back the palette that was active before.
	// Some terminals, like displays, may draw parts of the figure
	// using their own palette. Those terminals that possess only
	// one palette for the whole plot don't need this routine.
	// 
	virtual void  SetColor(const t_colorspec *);
	// EAM November 2004 - revised to take a pointer to struct rgb_color,
	// so that a palette gray value is not the only option for specifying color.
	virtual void  FilledPolygon(int points, gpiPoint *corners);
	virtual void  Image(uint, uint, coordval *, const gpiPoint * pCorners, t_imagecolor);
	// Enhanced text mode driver call-backs 
	virtual void  EnhancedOpen(char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
	virtual void  EnhancedFlush();
	virtual void  EnhancedWriteC(int c);
	// Driver-specific synchronization or other layering commands.
	// Introduced as an alternative to the ugly sight of
	// driver-specific code strewn about in the core routines.
	// As of this point (July 2005) used only by pslatex.trm
	virtual void  Layer(t_termlayer);
	// Begin/End path control.
	// Needed by PostScript-like devices in order to join the endpoints of a polygon cleanly.
	virtual void  Path(int p);
	// Pass hypertext for inclusion in the output plot 
	virtual void Hypertext(int type, const char * text);
	virtual void BoxedText(uint, uint, int);
	virtual void ModifyPlots(uint operations, int plotno);
	virtual void DashType(int type, t_dashtype * custom_dash_pattern);

	GnuPlot * P_Gp;
protected:
	uint  MaxX;
	uint  MaxY;
	uint  ChrV;
	uint  ChrH;
	uint  TicV;
	uint  TicH;
	int   Flags;
	double TScale; // Scale factor for converting terminal coordinates to output pixel coordinates.  Used to provide data for external mousing code.
	SString Name;
	SString Descr;
	GpTermEntry_Static * P_Oit; // Old implementation terminal pointer 
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
//
// options handling 
//
enum { 
	UNSET = -1, 
	no = 0, 
	yes = 1 
};
//
//
//
struct GpArrow {
	int    HeadLength;        // access head length + angle without changing API
	double HeadAngle;         // angle in degrees 
	double HeadBackAngle;     // angle in degrees 
	arrowheadfill HeadFilled; // arrow head filled or not 
	bool   HeadFixedSize;     // Adapt the head size for short arrows or not 
};

class GpTerminalBlock {
public:
	GpTerminalBlock() : P_Term(0), P_OutStr(0), P_GpOutFile(0), P_GpPsFile(0), P_PS_PsDir(0), P_PS_FontPath(0), P_TermInterlock(0),
		Flags(0), MultiplotCount(0), _Encoding(S_ENC_DEFAULT), LinetypeRecycleCount(0), MonoRecycleCount(0)
	{
		//memzero(TermOptions, sizeof(TermOptions));
	}
	int OutPrintf(); // @construction
	int PsPrintf(); // @construction
	struct GpTermEntry * P_Term;  // unknown 
	//char   TermOptions[MAX_LINE_LEN+1]; // ... and its options string 
	SString _TermOptions;
	// the 'output' file name and handle 
	char * P_OutStr; // means "STDOUT" 
	FILE * P_GpOutFile;
	// Output file where the PostScript output goes to. See term_api.h for more details. 
	FILE * P_GpPsFile;
	char * P_PS_PsDir;
	char * P_PS_FontPath;
	//bool term_initialised; // true if terminal has been initialized 
	// The qt and wxt terminals cannot be used in the same session. 
	// Whichever one is used first to plot, this locks out the other. 
	void * P_TermInterlock;
	enum {
		fTermInitialised = 0x0001, // true if terminal has been initialized 
		fMonochrome      = 0x0002, // true if "set monochrome"
		fMultiplot       = 0x0004  // // true if in multiplot mode 
	};
	uint   Flags;
	//bool monochrome; // true if "set monochrome"
	//bool multiplot;  // true if in multiplot mode 
	int    MultiplotCount;
	enum   set_encoding_id _Encoding; // text output encoding, for terminals that support it 
	//
	// Support for enhanced text mode. Declared extern in term_api.h 
	//
	int    LinetypeRecycleCount; // Recycle count for user-defined linetypes 
	int    MonoRecycleCount;
	GpArrow CArw;
};

extern GpTerminalBlock GPT; // @global
//
// Variables of term.c needed by other modules: 
//
//extern GpTermEntry * term; // the terminal info structure, being the heart of the whole module 
// extern char term_options[MAX_LINE_LEN+1]; // Options string of the currently used terminal driver 
//extern int curr_arrow_headlength; // access head length + angle without changing API 
//extern double curr_arrow_headangle; // angle in degrees 
//extern double curr_arrow_headbackangle; // angle in degrees 
//extern arrowheadfill curr_arrow_headfilled;
//extern bool curr_arrow_headfixedsize;
//extern int linetype_recycle_count; // Recycle count for user-defined linetypes 
//extern int mono_recycle_count;
// Current 'output' file: name and open filehandle 
//extern char * outstr;
//extern FILE * gpoutfile;
// 
// Output file where postscript terminal output goes to.
// In particular:
//   gppsfile == gpoutfile       - for 'set term': postscript, pstex
//   gppsfile == PSLATEX_auxfile - for 'set term': pslatex, cairolatex
//   gppsfile == 0               - for all other terminals
// 
//extern FILE * gppsfile;
//extern char * PS_psdir;
//extern char * PS_fontpath; // just a directory name 
//extern bool monochrome;
//extern bool multiplot;
//extern int multiplot_count;
//extern enum set_encoding_id encoding; // 'set encoding' support: index of current encoding ... 
extern const char * encoding_names[]; // ... in table of encoding names: 
extern const gen_table set_encoding_tbl[]; /* parsing table for encodings */
// The qt and wxt terminals cannot be used in the same session. 
// Whichever one is used first to plot, this locks out the other. 
//extern void * term_interlock;
//void init_monochrome();
//void write_multiline(GpTermEntry * pTerm, int, int, char *, JUSTIFY, VERT_JUSTIFY, int, const char *);
//int estimate_strlen(const char * length, double * estimated_fontheight);
//char * estimate_plaintext(char *);
//void list_terms();
char * get_terminals_names();
// Support for enhanced text mode. 
const char * enhanced_recursion(GpTermEntry * pTerm, const char * p, bool brace, const char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
//void enh_err_check(const char * str);
// note: c is char, but must be declared int due to K&R compatibility. 
void do_enh_writec(GpTermEntry_Static * pThis, int c);
// flag: don't use enhanced output methods --- for output of
// filenames, which usually looks bad using subscripts 
//void ignore_enhanced(bool flag);
// Simple-minded test that point is with drawable area 
bool on_page(GpTermEntry * pTerm, int x, int y);
// Convert a fill style into a backwards compatible packed form 
int style_from_fill(const fill_style_type *);
//
// Terminal-independent routine to draw a circle or arc 
//
//int  load_dashtype(t_dashtype * dt, int tag);
// Wrappers for term->path() 
void FASTCALL newpath(GpTermEntry * pTerm);
void FASTCALL closepath(GpTermEntry * pTerm);
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
	// @noctr
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
	int tag; /* identifies the label */
	GpPosition place;
	enum JUSTIFY pos; /* left/center/right horizontal justification */
	int rotate;
	int layer;
	int boxed; /* 0 no box;  -1 default box props;  >0 boxstyle */
	char * text;
	char * font; /* Entry font added by DJL */
	t_colorspec textcolor;
	lp_style_type lp_properties;
	GpPosition offset;
	bool noenhanced;
	bool hypertext;
};

//#define EMPTY_LABELSTRUCT \
	//{NULL, NONROTATING_LABEL_TAG, {character, character, character, 0.0, 0.0, 0.0}, CENTRE, 0, 0, 0, \
	//NULL, NULL, {TC_LT, -2, 0.0}, DEFAULT_LP_STYLE_TYPE, {character, character, character, 0.0, 0.0, 0.0}, FALSE, FALSE}
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
	int tag; /* identifies the arrow */
	arrow_type type; /* how to interpret GpPosition end */
	GpPosition start;
	GpPosition end;
	double angle; /* angle in degrees if type arrow_end_oriented */
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
	int type; /* 0 = corners;  1 = center + size */
	GpPosition center; /* center */
	GpPosition extent; /* width and height */
	GpPosition bl; /* bottom left */
	GpPosition tr; /* top right */
};

#define DEFAULT_RADIUS (-1.0)
#define DEFAULT_ELLIPSE (-2.0)

#define ELLIPSEAXES_XY (0)
#define ELLIPSEAXES_XX (1)
#define ELLIPSEAXES_YY (2)

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
//#define DEFAULT_PARALLEL_AXIS_STYLE {{0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 0, 2.0, 0.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, LAYER_FRONT}
//#define DEFAULT_SPIDERPLOT_STYLE { DEFAULT_LP_STYLE_TYPE, {FS_EMPTY, 100, 0, DEFAULT_COLORSPEC} }
//#define DEFAULT_HISTOGRAM_STYLE { HT_CLUSTERED, 2, 1, 0.0, 0.0, LT_UNDEFINED, LT_UNDEFINED, 0, NULL, EMPTY_LABELSTRUCT }
//#define DEFAULT_KEY_PROPS { TRUE, GPKEY_AUTO_INTERIOR_LRTBC, GPKEY_RMARGIN, DEFAULT_KEY_POSITION, \
		//JUST_TOP, RIGHT, TRUE, GPKEY_RIGHT, GPKEY_VERTICAL, 4.0, 1.0, 0.0, 0.0, \
		//FILENAME_KEYTITLES, FALSE, FALSE, FALSE, TRUE, DEFAULT_KEYBOX_LP, \
		//NULL, {TC_LT, LT_BLACK, 0.0}, BACKGROUND_COLORSPEC, {0, 0, 0, 0}, 0, 0, EMPTY_LABELSTRUCT}
#define DEFAULT_BOXPLOT_FACTOR -1
#define DEFAULT_BOXPLOT_STYLE { 0, 1.5, TRUE, 6, CANDLESTICKS, -1.0, 1.0, BOXPLOT_FACTOR_LABELS_AUTO, FALSE }
#define DEFAULT_KEYBOX_LP {0, LT_NODRAW, 0, DASHTYPE_SOLID, 0, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}
#define DEFAULT_KEY_POSITION { graph, graph, graph, 0.9, 0.9, 0. }
#define DEFAULT_TEXTBOX_STYLE { FALSE, FALSE, 1.0, 1.0, 1.0, BLACK_COLORSPEC, BACKGROUND_COLORSPEC }
// 
// EAM Jan 2006 - Move colorbox structure definition to here from color.h
// in order to be able to use struct GpPosition
// 
#define SMCOLOR_BOX_NO      'n'
#define SMCOLOR_BOX_DEFAULT 'd'
#define SMCOLOR_BOX_USER    'u'

struct t_circle {
	void   SetDefault()
	{
		// {.circle = {1, {0,0,0,0.,0.,0.}, {graph,0,0,0.02,0.,0.}, 0., 360., true }} }
		type = 1;
		memzero(&center, sizeof(center));
		extent.Set(graph, first_axes, first_axes, 0.02, 0.0, 0.0);
		ArcR.Set(0.0, 360.0);
		wedge = true;
	}
	int type; // not used 
	GpPosition center; // center 
	GpPosition extent; // radius 
	RealRange ArcR;
	bool wedge; // TRUE = connect arc ends to center 
};

struct t_ellipse {
	void   SetDefault()
	{
		// {.ellipse = {ELLIPSEAXES_XY, {0,0,0,0.,0.,0.}, {graph,graph,0,0.05,0.03,0.}, 0. }} }
		type = ELLIPSEAXES_XY;
		memzero(&center, sizeof(center));
		extent.Set(graph, graph, first_axes, 0.05, 0.03, 0.0);
		orientation = 0.0;
	}
	int type; /* mapping of axes: ELLIPSEAXES_XY, ELLIPSEAXES_XX or ELLIPSEAXES_YY */
	GpPosition center; /* center */
	GpPosition extent; /* major and minor axes */
	double orientation; /* angle of first axis to horizontal */
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
	int type; /* Number of vertices */
	GpPosition * vertex; /* Array of vertices */
};

enum t_clip_object {
	OBJ_CLIP,       /* Clip to graph unless coordinate type is screen */
	OBJ_NOCLIP, /* Clip to canvas, never to graph */
	OBJ_ALWAYS_CLIP /* Not yet implemented */
};
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
	int layer; /* behind or back or front */
	int object_type; /* e.g. OBJ_RECTANGLE */
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
	int tag; /* identifies the dashtype */
	int d_type; /* for DASHTYPE_SOLID or CUSTOM */
	t_dashtype dashtype;
};
//
// Datastructure implementing 'set style line' 
//
struct linestyle_def {
	linestyle_def * next; /* pointer to next linestyle in linked list */
	int tag; /* identifies the linestyle */
	lp_style_type lp_properties;
};
//
// Datastructure implementing 'set style arrow' 
//
struct arrowstyle_def {
	arrowstyle_def * next; /* pointer to next arrowstyle in linked list */
	int tag; /* identifies the arrowstyle */
	arrow_style_type arrow_properties;
};
//
// Datastructure for 'set pixmap 
//
struct t_pixmap {
	int tag; /* index referring to this pixmap */
	t_pixmap * next; /* pointer to next pixmap in the linked list */
	uint ncols; /* image size */
	uint nrows; 
	GpPosition pin; /* where it goes */
	GpPosition extent; /* width dx;  dy implicitly dx*aspect_ratio */
	int    layer; /* front/back/behind */
	bool   center; /* position is center rather than lower left */
	char * filename; /* where to read the pixmap pixmap */
	char * colormapname; /* the colormap this was taken from */
	coordval * image_data; /* pixel array RGBARGBA... */
};
//
// Used by 'set style parallelaxis' and 'set style spiderplot' 
//
struct pa_style {
	pa_style() : lp_properties(lp_style_type::defParallelAxis), layer(LAYER_FRONT)
	{
	}
	lp_style_type lp_properties; /* used to draw the axes themselves */
	int layer; /* front/back */
};

struct spider_web {
	//#define DEFAULT_SPIDERPLOT_STYLE { DEFAULT_LP_STYLE_TYPE, {FS_EMPTY, 100, 0, DEFAULT_COLORSPEC} }
	spider_web() : lp_properties(lp_style_type::defCommon), fillstyle(FS_EMPTY, 100, 0)
	{
	}
	lp_style_type lp_properties;
	fill_style_type fillstyle;
};
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
	filledcurves_opts() : closeto(FILLEDCURVES_DEFAULT), at(0.0), aty(0.0), oneside(0)
	{
	}
	enum filledcurves_opts_id closeto; // from list FILLEDCURVES_CLOSED, ...
	double at; // value for FILLEDCURVES_AT... 
	double aty; // the other value for FILLEDCURVES_ATXY 
	int    oneside; // -1 if fill below bound only; +1 if fill above bound only 
};

//#define EMPTY_FILLEDCURVES_OPTS { FILLEDCURVES_DEFAULT, 0.0, 0.0, 0 }

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
	int    type; /* enum t_histogram_type */
	int    gap; /* set style hist gap <n> (space between clusters) */
	int    clustersize; /* number of datasets in this histogram */
	double start; /* X-coord of first histogram entry */
	double end; /* X-coord of last histogram entry */
	int    startcolor; /* LT_UNDEFINED or explicit color for first entry */
	int    startpattern; /* LT_UNDEFINED or explicit pattern for first entry */
	double bar_lw; /* linewidth for error bars */
	histogram_style * next;
	text_label title;
};

enum t_boxplot_factor_labels {
	BOXPLOT_FACTOR_LABELS_OFF,
	BOXPLOT_FACTOR_LABELS_AUTO,
	BOXPLOT_FACTOR_LABELS_X,
	BOXPLOT_FACTOR_LABELS_X2
};

struct boxplot_style {
	int limit_type; /* 0 = multiple of interquartile 1 = fraction of points */
	double limit_value;
	bool outliers;
	int pointtype;
	int plotstyle; /* CANDLESTICKS or FINANCEBARS */
	double median_linewidth;
	double separation; /* of boxplots if there are more than one factors */
	t_boxplot_factor_labels labels; /* Which axis to put the tic labels if there are factors */
	bool sort_factors; /* Sort factors in alphabetical order? */
};

struct textbox_style {
	bool opaque;    // True if the box is background-filled before writing into it 
	bool noborder;  // True if you want fill only, no lines 
	//double xmargin; // fraction of default margin to use 
	//double ymargin; // fraction of default margin to use 
	SPoint2R Margin;  // fraction of default margin to use 
	double linewidth; // applied to border 
	t_colorspec border_color; // TC_LT + LT_NODRAW is "noborder" 
	t_colorspec fillcolor;  // only used if opaque is TRUE 
};
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
// EAM Feb 2003 - Move all global variables related to key into a 
// single structure. Eventually this will allow multiple keys.    
enum keytitle_type {
	NOAUTO_KEYTITLES, 
	FILENAME_KEYTITLES, 
	COLUMNHEAD_KEYTITLES
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
	bool visible; /* Do we show this key at all? */
	t_key_region region; /* if so: where? */
	t_key_ext_region margin; /* if exterior: where outside? */
	GpPosition user_pos; /* if user specified position, this is it */
	VERT_JUSTIFY vpos; /* otherwise these guide auto-positioning */
	JUSTIFY hpos;
	bool fixed; /* prevents key in 3D plot from rotating/scaling with plot */
	t_key_sample_positioning just;
	t_key_stack_direction stack_dir;
	double swidth; /* 'width' of the linestyle sample line in the key */
	double vert_factor; /* user specified vertical spacing multiplier */
	double width_fix; /* user specified additional (+/-) width of key titles */
	double height_fix;
	keytitle_type auto_titles; /* auto title curves unless plotted 'with notitle' */
	bool front; /* draw key in a second pass after the rest of the graph */
	bool reverse; /* key back to front */
	bool invert; /* key top to bottom */
	bool enhanced; /* enable/disable enhanced text of key titles */
	lp_style_type box; /* linetype of box around key:  */
	char * font; /* Will be used for both key title and plot titles */
	t_colorspec textcolor; /* Will be used for both key title and plot titles */
	t_colorspec fillcolor; /* only used if "set key front" */
	BoundingBox bounds;
	int maxcols; /* maximum no of columns for horizontal keys */
	int maxrows; /* maximum no of rows for vertical keys */
	text_label title; /* holds title line for the key as a whole */
};

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

extern const color_box_struct default_color_box;
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
	SPoint3F Size; // scale factor for size 
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
#ifndef DEFAULT_TIMESTAMP_FORMAT
	#define DEFAULT_TIMESTAMP_FORMAT "%a %b %d %H:%M:%S %Y" // asctime() format 
#endif
#define ZERO 1e-8               /* default for 'zero' set option */
extern const t_colorspec background_fill;

#define SOUTH           1 /* 0th bit */
#define WEST            2 /* 1th bit */
#define NORTH           4 /* 2th bit */
#define EAST            8 /* 3th bit */
#define border_east     (Gg.draw_border & EAST)
#define border_west     (Gg.draw_border & WEST)
#define border_south    (Gg.draw_border & SOUTH)
#define border_north    (Gg.draw_border & NORTH)
extern const lp_style_type background_lp;
extern const lp_style_type default_border_lp;
		
#define SAMPLES 100             /* default number of samples for a plot */
// 
// A macro to check whether 2D functionality is allowed in the last plot:
// either the plot is a 2D plot, or it is a suitably oriented 3D plot (e.g. map).
// 
//#define ALMOST2D (!GPO.Gg.Is3DPlot || GPO._3DBlk.splot_map || (fabs(fmod(GPO._3DBlk.SurfaceRotZ, 90.0)) < 0.1 && fabs(fmod(GPO._3DBlk.SurfaceRotX, 180.0))<0.1))

enum TRefresh_Allowed {
	E_REFRESH_NOT_OK = 0,
	E_REFRESH_OK_2D = 2,
	E_REFRESH_OK_3D = 3
};

#define SET_REFRESH_OK(ok, nplots) do { Gg.refresh_ok = (ok); Gg.refresh_nplots = (nplots); } while(0)
#define WALL_Y0_TAG 0
#define WALL_X0_TAG 1
#define WALL_Y1_TAG 2
#define WALL_X1_TAG 3
#define WALL_Z0_TAG 4
#define WALL_Y0_CORNERS { {graph, graph, graph, 0, 0, 0}, {graph, graph, graph, 0, 0, 1}, {graph, graph, graph, 1, 0, 1}, {graph, graph, graph, 1, 0, 0}, {graph, graph, graph, 0, 0, 0} }
#define WALL_X0_CORNERS { {graph, graph, graph, 0, 0, 0}, {graph, graph, graph, 0, 1, 0}, {graph, graph, graph, 0, 1, 1}, {graph, graph, graph, 0, 0, 1}, {graph, graph, graph, 0, 0, 0} }
#define WALL_Y1_CORNERS { {graph, graph, graph, 0, 1, 0}, {graph, graph, graph, 1, 1, 0}, {graph, graph, graph, 1, 1, 1}, {graph, graph, graph, 0, 1, 1}, {graph, graph, graph, 0, 1, 0} }
#define WALL_X1_CORNERS { {graph, graph, graph, 1, 0, 0}, {graph, graph, graph, 1, 0, 1}, {graph, graph, graph, 1, 1, 1}, {graph, graph, graph, 1, 1, 0}, {graph, graph, graph, 1, 0, 0} }
#define WALL_Z0_CORNERS { {graph, graph, graph, 0, 0, 0}, {graph, graph, graph, 1, 0, 0}, {graph, graph, graph, 1, 1, 0}, {graph, graph, graph, 0, 1, 0}, {graph, graph, graph, 0, 0, 0} }
#define WALL_Y_COLOR 0xcdb79e
#define WALL_X_COLOR 0x228b22
#define WALL_Z_COLOR 0xa0b6cd
// TODO: linked list rather than fixed size array 
#define NUM_TEXTBOX_STYLES 4
void   default_arrow_style(arrow_style_type * arrow);
void   free_labels(text_label * tl);
//
//#include <axis.h>
// Aug 2017 - unconditional support for nonlinear axes 
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
	TIC_USER, /* user-defined points */
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
	double position; /* where on axis is this */
	char * label; /* optional (format) string label */
	int level; /* 0=major tic, 1=minor tic */
	ticmark * next; /* linked list */
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
		MinConstraint(CONSTRAINT_NONE), MaxConstraint(CONSTRAINT_NONE), min_lb(0.0), min_ub(0.0), max_lb(0.0), max_ub(0.0),
		term_lower(0), term_upper(0), term_scale(0.0), term_zero(0), log(false), base(0.0), log_base(0.0),
		linked_to_primary(0), linked_to_secondary(0), link_udf(0), ticmode(NO_TICS), /*ticdef @ctr*/tic_rotate(0), tic_pos(CENTRE),
		gridmajor(false), gridminor(false), minitics(MINI_DEFAULT), mtic_freq(10.0),
		ticscale(1.0), miniticscale(0.5), ticstep(0.0), TicIn(true), datatype(DT_NORMAL), tictype(DT_NORMAL),
		formatstring(0), ticfmt(0), timelevel((t_timelevel)(0)), index(0), manual_justify(false), zeroaxis(0), paxis_x(0.0)
	{
	}
	void   Init(bool resetAutoscale);
	//#define inrange(z, min, max) (((min)<(max)) ? (((z)>=(min)) && ((z)<=(max))) : (((z)>=(max)) && ((z)<=(min))))
	bool   InRange(double v) const { return ((min<max) ? ((v>=min) && (v<=max)) : ((v>=max) && (v<=min))); }
	double GetActualMin() const { return MIN(max, min); }
	double GetActualMax() const { return MAX(max, min); }
	double GetRange() const { return (max - min); }
	double ClipToRange(double v) const { return sclampx(v, min, max); }
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
	double MapBack(int var) const { return (((double)(var) - term_lower)/term_scale + min); }
	void   FlipProjection();
	// range of this axis 
	t_autoscale autoscale;     // Which end(s) are autoscaled? */
	t_autoscale set_autoscale; // what does 'set' think autoscale to be? 
	int range_flags; /* flag bits about autoscale/writeback: */
	// write auto-ed ranges back to variables for autoscale 
#define RANGE_WRITEBACK   1
#define RANGE_SAMPLED     2
#define RANGE_IS_REVERSED 4
	double min; /* 'transient' axis extremal values */
	double max;
	double set_min; /* set/show 'permanent' values */
	double set_max;
	double writeback_min; /* ULIG's writeback implementation */
	double writeback_max;
	double data_min; // Not necessarily the same as axis min 
	double data_max;
	// range constraints 
	t_constraint MinConstraint;
	t_constraint MaxConstraint;
	double min_lb; // min lower- and upper-bound 
	double min_ub; 
	double max_lb; // min lower- and upper-bound
	double max_ub; 
	// output-related quantities 
	int    term_lower; // low and high end of the axis on output,
	int    term_upper; // ... (in terminal coordinates)
	double term_scale; // scale factor: plot --> term coords
	uint   term_zero;  // position of zero axis 
	// log axis control 
	bool   log;   // log axis stuff: flag "islog?" 
	double base;  // logarithm base value 
	double log_base; // ln(base), for easier computations 
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
	enum JUSTIFY tic_pos; /* left/center/right tic label justification */
	bool   gridmajor; /* Grid lines wanted on major tics? */
	bool   gridminor; /* Grid lines for minor tics? */
	t_minitics_status minitics; /* minor tic mode (none/auto/user)? */
	double mtic_freq; /* minitic stepsize */
	double ticscale; /* scale factor for tic marks (was (0..1])*/
	double miniticscale; /* and for minitics */
	double ticstep; /* increment used to generate tic placement */
	bool   TicIn; /* tics to be drawn inward?  */
	// time/date axis control 
	td_type datatype; /* {DT_NORMAL|DT_TIMEDATE} controls _input_ */
	td_type tictype; /* {DT_NORMAL|DT_TIMEDATE|DT_DMS} controls _output_ */
	char * formatstring; /* the format string for output */
	char * ticfmt; /* autogenerated alternative to formatstring (needed??) */
	t_timelevel timelevel; /* minimum time unit used to quantize ticks */
	// other miscellaneous fields 
	int    index; // if this is a permanent axis, this indexes GPO.AxS[] 
		// (index >= PARALLEL_AXES) indexes parallel axes; (index < 0) indicates a dynamically allocated structure 
	text_label label; /* label string and position offsets */
	bool   manual_justify; /* override automatic justification */
	lp_style_type * zeroaxis; /* usually points to default_axis_zeroaxis */
	double paxis_x; /* x coordinate of parallel axis */
};

#if 0 // {
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
#endif // } 0

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
extern const AXIS_DEFAULTS axis_defaults[AXIS_ARRAY_SIZE];
extern const GpAxis default_axis_state;

// A parsing table for mapping axis names into axis indices. For use by the set/show machinery, mainly 
extern const gen_table axisname_tbl[];
extern const t_ticdef default_axis_ticdef;

// default format for tic mark labels 
#define DEF_FORMAT "% h"
#define DEF_FORMAT_LATEX "$%h$"
#define TIMEFMT "%d/%m/%y,%H:%M" // default parse timedata string 

extern const text_label default_axis_label; // axis labels 
extern const lp_style_type default_axis_zeroaxis; // zeroaxis linetype (flag type==-3 if none wanted) 
extern const lp_style_type default_grid_lp; // default grid linetype, to be used by 'unset grid' and 'reset' 

class GpAxisSet {
public:
	GpAxisSet() : P_ShadowAxArray(0), P_ParallelAxArray(0), NumParallelAxes(0), Idx_X(FIRST_X_AXIS), Idx_Y(FIRST_Y_AXIS), Idx_Z(FIRST_Z_AXIS),
		tic_start(0), tic_direction(0), tic_text(0), tic_mirror(0), rotate_tics(0), tic_hjust(LEFT), tic_vjust(JUST_TOP),
		vol_this_tic(0.0), vol_previous_tic(0.0), P_TimeFormat(0), grid_lp(lp_style_type::defGrid), mgrid_lp(lp_style_type::defGrid),
		grid_layer(LAYER_BEHIND), polar_grid_angle(0.0), ThetaOrigin(0.0), ThetaDirection(1.0), WidestTicLen(0), grid_tics_in_front(false),
		grid_vertical_lines(false), grid_spiderweb(false), raxis(false), inside_zoom(false),
		ticscale{1.0, 0.5, 1.0, 1.0, 1.0}
	{
	}
	GpAxis & operator [](size_t idx) { return AxArray[idx]; }
	GpAxis & Theta() { return ThetaAx; }
	GpAxis & Parallel(int idx) { return P_ParallelAxArray[idx]; }
	GpAxis & __X() { return AxArray[Idx_X]; }
	GpAxis & __Y() { return AxArray[Idx_Y]; }
	GpAxis & __Z() { return AxArray[Idx_Z]; }
	GpAxis & __R() { return AxArray[POLAR_AXIS]; }
	GpAxis & __CB() { return AxArray[COLOR_AXIS]; }

	bool   HasParallel() const { return LOGIC(P_ParallelAxArray); }
	int    GetParallelAxisCount() const { return NumParallelAxes; }
	void   ExtendParallelAxis(int);
	//GpAxis * GetShadowAxis(GpAxis * pAxis);
	void   DestroyShadowAxes();
	void   DestroyParallelAxes();
	void   FASTCALL InitSampleRange(const GpAxis * pAxis, enum PLOT_TYPE plot_type);
	//void   FASTCALL CheckRange(AXIS_INDEX idx);
	void   SaveAxisFormat(FILE * fp, AXIS_INDEX axis) const;
	bool   ValidateData(double v, AXIS_INDEX ax) const;
	void   SaveAutoscaledRanges(const GpAxis * pAxX, const GpAxis * pAxY);
	void   RestoreAutoscaledRanges(GpAxis * pAxX, GpAxis * pAxY) const;

	AXIS_INDEX Idx_X; // axes being used by the current plot 
	AXIS_INDEX Idx_Y; // axes being used by the current plot 
	AXIS_INDEX Idx_Z; // axes being used by the current plot 
	//
	// penalty for doing tics by callback in gen_tics is need for global
	// variables to communicate with the tic routines. Dont need to be arrays for this 
	// HBB 20000416: they may not need to be array[]ed, but it'd sure
	// make coding easier, in some places... 
	// HBB 20000416: for the testing, these are global... 
	int    tic_start;
	int    tic_direction;
	int    tic_text;
	int    tic_mirror;
	int    rotate_tics;
	JUSTIFY tic_hjust;
	VERT_JUSTIFY tic_vjust;
	//
	// These are declare volatile in order to fool the compiler into not 
	// optimizing out intermediate values, thus hiding loss of precision.
	volatile double vol_this_tic;
	volatile double vol_previous_tic;
	double ticscale[MAX_TICLEVEL]; // Tic scale for tics with level > 1.  0 means 'inherit minitics scale' 
	char * P_TimeFormat;    // global default time format 
	lp_style_type grid_lp;  // = DEFAULT_GRID_LP;
	lp_style_type mgrid_lp; // = DEFAULT_GRID_LP;
	int    grid_layer;
	double polar_grid_angle; // nonzero means a polar grid 
	double ThetaOrigin;  // default origin at right side
	double ThetaDirection; // counterclockwise from origin 
	int    WidestTicLen; // Length of the longest tics label, set by widest_tic_callback()
	bool   grid_tics_in_front;
	bool   grid_vertical_lines;
	bool   grid_spiderweb;
	bool   raxis;
	bool   inside_zoom; // flag to indicate that in-line axis ranges should be ignored 
//private:
	GpAxis AxArray[AXIS_ARRAY_SIZE]; // axis_array
	GpAxis ThetaAx;           //
	GpAxis * P_ShadowAxArray; // shadow_axis_array
	GpAxis * P_ParallelAxArray; // parallel_axis_array
	int    NumParallelAxes;     // num_parallel_axes
	RealRange SaveAutoscaledRangeX;
	RealRange SaveAutoscaledRangeY;
};
//
// -------- macros using these variables:
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
			if(store_and_update_range(&(STORE), VALUE, &(TYPE), (&AxS[AXIS]), NOAUTOSCALE) == UNDEFINED) { \
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
void   axis_invert_if_requested(GpAxis *);
double quantize_normal_tics(double, int);
void   axis_set_scale_and_range(GpAxis * axis, int lower, int upper);
// set widest_tic_label: length of the longest tics label 
void   widest_tic_callback(GpAxis *, double place, char * text, int ticlevel, struct lp_style_type grid, struct ticmark *);
const  char * FASTCALL axis_name(AXIS_INDEX);
void   init_parallel_axis(GpAxis *, AXIS_INDEX);
void   dump_axis_range(GpAxis * axis); // For debugging 

// macro for tic scale, used in all tic_callback functions 
#define tic_scale(ticlevel, axis) (ticlevel <= 0 ? axis->ticscale : ticlevel == 1 ? axis->miniticscale : ticlevel < MAX_TICLEVEL ? AxS.ticscale[ticlevel] : 0)
// convenience macro to make sure min < max 
#define reorder_if_necessary(min, max) do { if(max < min) { double temp = min; min = max; max = temp; } } while(0)
//
//#include <command.h>
struct GpLexicalUnit { // produced by scanner 
	bool   IsToken;   // true if token, false if a value 
	GpValue LVal;
	int    StartIdx; // index of first char in token 
	int    Len;      // length of token in chars 
};

#ifdef USE_MOUSE
	extern int paused_for_mouse; // Flag the end condition we are paused until
	#define PAUSE_BUTTON1   001  // Mouse button 1
	#define PAUSE_BUTTON2   002  // Mouse button 2
	#define PAUSE_BUTTON3   004  // Mouse button 3
	#define PAUSE_CLICK     007  // Any button click
	#define PAUSE_KEYSTROKE 010  // Any keystroke
	#define PAUSE_WINCLOSE	020  // Window close event
	#define PAUSE_ANY       077  // Terminate on any of the above
#endif

#ifndef STDOUT
	#define STDOUT 1
#endif
#ifdef _WIN32
	#define SET_CURSOR_WAIT ::SetCursor(::LoadCursor(0, IDC_WAIT))
	#define SET_CURSOR_ARROW ::SetCursor(::LoadCursor(0, IDC_ARROW))
#else
	#define SET_CURSOR_WAIT        /* nought, zilch */
	#define SET_CURSOR_ARROW       /* nought, zilch */
#endif

enum ifstate {
	IF_INITIAL = 1, 
	IF_TRUE, 
	IF_FALSE
};

class GpProgram {
public:
	GpProgram() : CToken(0), NumTokens(0), P_Token(0), TokenTableSize(0), __TNum(0), CurlyBraceCount(0),
		P_InputLine(0), InputLineLen(0), inline_num(0), dummy_func(0), replot_line(0), plot_token(0),
		print_out(0), print_out_var(0), print_out_name(0), clause_depth(0), iteration_depth(0), command_exit_requested(0),
		replot_disabled(false), if_open_for_else(false), requested_break(false), requested_continue(false)
	{
	}
	size_t TokenLen(int t_num) const { return (size_t)(P_Token[t_num].Len); }
	size_t CurTokenLen() const { return (size_t)(P_Token[CToken].Len); }
	int    GetCurTokenLength() const { return P_Token[CToken].Len; }
	int    GetCurTokenStartIndex() const { return P_Token[CToken].StartIdx; }
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
	int    FASTCALL IsANumber(int tokN) const { return (!P_Token[tokN].IsToken); }
	int    FASTCALL IsString(int t_num) const;
	int    FASTCALL LookupTableForCurrentToken(const struct gen_table * tbl) const;
	int    AlmostEquals(int t_num, const char * pStr) const;
	int    AlmostEqualsCur(const char * pStr) const { return AlmostEquals(CToken, pStr); }
	void   SetTokenIdx(int tokN) { CToken = tokN; }
	void   ParseSkipRange();
	char * ParseDatablockName();
	void   BreakCommand();
	void   ContinueCommand();
	int    IsLetter(int t_num) const;
	void   ExtendTokenTable();
	void   CopyStr(char * pStr, int tokNum, int maxCount) const;
	void   Capture(char * pStr, int start, int end, int max) const;
	void   MCapture(char ** ppStr, int start, int end);
	char * TokenToString(int tokN) const;
	int    FindClause(int * pClauseStart, int * pClauseEnd);
	int    Shift() { return CToken++; }
	int    Rollback() { return CToken--; }
	GpLexicalUnit & Tok() { return P_Token[__TNum]; }
	GpLexicalUnit & СTok() { return P_Token[CToken]; }

	int    CToken;
	int    NumTokens;
	int    TokenTableSize;
	GpLexicalUnit * P_Token;
	int    __TNum; // Number of token I'm working on 
	int    CurlyBraceCount;
	//
	char * P_InputLine;  //gp_input_line;
	size_t InputLineLen; //gp_input_line_len;
	int    inline_num; // input line number 
	udft_entry * dummy_func;
	char * replot_line; // support for replot command 
	int    plot_token; // start of 'plot' command 
	FILE * print_out; // output file for the print command 
	udvt_entry * print_out_var;
	char * print_out_name;
	int    clause_depth;
	int    iteration_depth;
	int    command_exit_requested; // set when an "exit" command is encountered 
	bool   replot_disabled; // flag to disable `replot` when some data are sent through stdin; used by mouse/hotkey capable terminals 
	bool   if_open_for_else;
	bool   requested_break;
	bool   requested_continue;
};

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
void null_command();
//
// Prototypes for functions exported by command.c 
//
#ifdef USE_MOUSE
	void toggle_display_of_ipc_commands();
	int  display_ipc_commands();
#endif
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
#define INITIAL_LOCALE ("C")

#ifdef HAVE_LOCALE_H
	#define set_numeric_locale() do { if(GpU.numeric_locale && strcmp(GpU.numeric_locale,"C")) setlocale(LC_NUMERIC, GpU.numeric_locale);} while(0)
	#define reset_numeric_locale() do { if(GpU.numeric_locale && strcmp(GpU.numeric_locale,"C")) setlocale(LC_NUMERIC,"C");} while(0)
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
typedef uchar pixels; /* the type of one set of 8 pixels in bitmap */
typedef pixels * bitmap[]; /* the bitmap */

void b_setvalue(GpTermEntry_Static * pThis, uint);
void b_setlinetype(GpTermEntry_Static * pThis, int);
void b_linewidth(GpTermEntry_Static * pThis, double linewidth);
void b_move(GpTermEntry_Static * pThis, uint, uint);
void b_vector(GpTermEntry_Static * pThis, uint, uint);
void b_put_text(GpTermEntry_Static * pThis, uint, uint, const char *);
int  b_text_angle(GpTermEntry_Static * pThis, int);
void b_boxfill(GpTermEntry_Static * pThis, int, uint, uint, uint, uint);
void b_filled_polygon(GpTermEntry_Static * pThis, int points, gpiPoint * corners);
//
//#include <graphics.h>
//
// types defined for 2D plotting 
//
//
struct curve_points {
	curve_points * next;         // pointer to next plot in linked list 
	int    token;                // last token used, for second parsing pass 
	enum PLOT_TYPE  plot_type;   // DATA2D? DATA3D? FUNC2D FUNC3D? NODATA? 
	enum PLOT_STYLE plot_style;  // style set by "with" or by default 
	char * title;                // plot title, a.k.a. key entry 
	GpPosition * title_position; // title at {beginning|end|<xpos>,<ypos>} 
	bool   title_no_enhanced;    // don't typeset title in enhanced mode 
	bool   title_is_automated;   // TRUE if title was auto-generated 
	bool   title_is_suppressed;  // TRUE if 'notitle' was specified 
	bool   noautoscale;          // ignore data from this plot during autoscaling 
	lp_style_type lp_properties;
	arrow_style_type arrow_properties;
	fill_style_type fill_properties;
	text_label * labels; // Only used if plot_style == LABELPOINTS 
	t_image image_properties; // only used if plot_style is IMAGE or RGB_IMAGE 
	udvt_entry * sample_var;  // used by '+' if plot has private sampling range 
	udvt_entry * sample_var2; // used by '++'if plot has private sampling range 
	// 2D and 3D plot structure fields overlay only to this point 
	filledcurves_opts filledcurves_options;
	int    base_linetype; // before any calls to load_linetype(), lc variable analogous to hidden3d_top_linetype in graph3d.h
	int    ellipseaxes_units;  // Only used if plot_style == ELLIPSES 
	histogram_style *histogram; // Only used if plot_style == HISTOGRAM 
	int    histogram_sequence;  // Ordering of this dataset within the histogram 
	enum   PLOT_SMOOTH plot_smooth; // which "smooth" method to be used? 
	double smooth_parameter; // e.g. optional bandwidth for smooth kdensity 
	double smooth_period;    // e.g. 2pi for a circular function 
	int    boxplot_factors;  // Only used if plot_style == BOXPLOT 
	int    p_max;            // how many points are allocated 
	int    p_count;          // count of points in points 
	AXIS_INDEX AxIdx_X/*x_axis*/; // FIRST_X_AXIS or SECOND_X_AXIS 
	AXIS_INDEX AxIdx_Y/*y_axis*/; // FIRST_Y_AXIS or SECOND_Y_AXIS 
	AXIS_INDEX AxIdx_Z/*z_axis*/; // same as either x_axis or y_axis, for 5-column plot types 
	int    current_plotno; // Only used by "pn" option of linespoints 
	AXIS_INDEX AxIdx_P; /*p_axis*/;  // Only used for parallel axis plots 
	double * varcolor;  // Only used if plot has variable color 
	GpCoordinate * points;
};

//void free_histlist(histogram_style * hist);

enum t_procimg_action {
	IMG_PLOT,
	IMG_UPDATE_AXES,
	IMG_UPDATE_CORNERS
};
//int    filter_boxplot(curve_points *);
//
//#include <breaders.h>
//
// Prototypes of functions exported by breaders.c
//
int  df_libgd_get_pixel(int i, int j, int component);
//
//#include <getcolor.h>
enum color_models_id {
	C_MODEL_RGB = 'r',
	C_MODEL_HSV = 'h',
	C_MODEL_CMY = 'c',
	C_MODEL_XYZ = 'x'
};

void rgb255_from_rgb1(rgb_color rgb1, rgb255_color * rgb255);
// HSV --> RGB user-visible function hsv2rgb(h,s,v) 
//uint hsv2rgb(rgb_color * color);
// used to (de-)serialize color/gradient information 
char * gradient_entry_to_str(gradient_struct * gs);
void str_to_gradient_entry(char * s, gradient_struct * gs);
// check if two palettes p1 and p2 differ 
int palettes_differ(t_sm_palette * p1, t_sm_palette * p2);
double GetColorValueFromFormula(int formula, double x);
extern const char * ps_math_color_formulae[];
//
//#include <plot.h>
//
// Prototypes of functions exported by plot.c 
//
#if defined(__GNUC__)
	//void bail_to_command_line(void) __attribute__((noreturn));
#else
	//void bail_to_command_line();
#endif
void interrupt_setup();
#ifdef GNUPLOT_HISTORY
	void cancel_history();
#else
	#define cancel_history() {}
#endif
//
//#include <plot2d.h>
// This allows a natural interpretation of providing only a single column in 'using' 
#define default_smooth_weight(option) (oneof3(option, SMOOTH_BINS, SMOOTH_KDENSITY, SMOOTH_FREQUENCY))
void cp_extend(curve_points *cp, int num);
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

void sp_free(GpSurfacePoints *sp);
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

struct GpSurfacePoints {
	GpSurfacePoints * next_sp; // pointer to next plot in linked list
	int    token; // last token used, for second parsing pass
	enum   PLOT_TYPE plot_type; // DATA2D? DATA3D? FUNC2D FUNC3D? NODATA?
	enum   PLOT_STYLE plot_style; // style set by "with" or by default
	char * title; // plot title, a.k.a. key entry
	GpPosition * title_position; // title at {beginning|end|<xpos>,<ypos>}
	bool   title_no_enhanced; // don't typeset title in enhanced mode
	bool   title_is_automated; // TRUE if title was auto-generated
	bool   title_is_suppressed; // TRUE if 'notitle' was specified
	bool   noautoscale; // ignore data from this plot during autoscaling
	lp_style_type lp_properties;
	arrow_style_type arrow_properties;
	fill_style_type fill_properties;
	text_label * labels; // Only used if plot_style == LABELPOINTS
	t_image image_properties; // only used if plot_style is IMAGE, RGBIMAGE or RGBA_IMAGE
	udvt_entry * sample_var; // used by '+' if plot has private sampling range
	udvt_entry * sample_var2; // used by '++' if plot has private sampling range
	//
	// 2D and 3D plot structure fields overlay only to this point 
	//
	enum PLOT_SMOOTH plot_smooth; // EXPERIMENTAL: smooth lines in 3D
	bool   opt_out_of_hidden3d; // set by "nohidden" option to splot command
	bool   opt_out_of_contours; // set by "nocontours" option to splot command
	bool   opt_out_of_surface; // set by "nosurface" option to splot command
	bool   pm3d_color_from_column;
	bool   has_grid_topology;
	int    hidden3d_top_linetype; // before any calls to load_linetype()
	int    iteration; // needed for tracking iteration
	VGrid * vgrid; // used only for voxel plots
	double iso_level; // used only for voxel plots
	// Data files only - num of isolines read from file. For functions, 
	// num_iso_read is the number of 'primary' isolines (in x direction)
	int    num_iso_read;
	gnuplot_contours * contours; // NULL if not doing contours.
	iso_curve * iso_crvs; // the actual data
	char   pm3d_where[7]; // explicitly given base, top, surface
};

struct t_xyplane {
	t_xyplane(double _z, bool isAbs) : z(_z), absolute(isAbs)
	{
	}
	double z;
	bool   absolute;
};
	
#define ISO_SAMPLES 10          /* default number of isolines per splot */

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
		// keep the following order of PM3D_WHICHCORNER_C1 .. _C4 
		PM3D_WHICHCORNER_C1 = 0, /* corner 1: first scan, first point   */
		PM3D_WHICHCORNER_C2 = 1, /* corner 2: first scan, second point  */
		PM3D_WHICHCORNER_C3 = 2, /* corner 3: second scan, first point  */
		PM3D_WHICHCORNER_C4 = 3, /* corner 4: second scan, second point */
		// the rest can be in any order 
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
		pm3d_struct() : flush(0), ftriangles(0), clip(0), no_clipcb(false), direction(PM3D_SCANS_AUTOMATIC),
			base_sort(false), zmean_sort(false), implicit(PM3D_EXPLICIT), which_corner_color(PM3D_WHICHCORNER_C1),
			interp_i(0), interp_j(0)
		{
			memzero(where, sizeof(where));
		}
		char   where[7];   // base, top, surface 
		char   flush;      // left, right, center 
		char   ftriangles; // 0/1 (don't) draw flushing triangles 
		char   clip;       // 1in, 4in 
		bool   no_clipcb;  // FALSE: out-of-range cb treated as 0  TRUE: treated as NaN 
		pm3d_scandir direction;
		// If direction is "depth" sort by max z of 4 corners unless ... 
		bool   base_sort;  // use z values of projection to baseplane 
		bool   zmean_sort; // sort on mean z rather than max z 
		PM3D_IMPL_MODE implicit;
		// 1: [default] draw ALL surfaces with pm3d
		// 0: only surfaces specified with 'with pm3d' 
		PM3D_WHICH_CORNERS2COLOR which_corner_color;
		// default: average color from all 4 points 
		int    interp_i; // # of interpolation steps along scanline 
		int    interp_j; // # of interpolation steps between scanlines 
		lp_style_type border; // LT_NODRAW to disable.  From `set pm3d border <linespec> 
	};

	struct lighting_model {
		lighting_model() : strength(0.0), spec(0.0), ambient(0.0), Phong(0.0), rot_z(0), rot_x(0), fixed(false), spec2(0.0)
		{
		}
		double strength; // 0 = no lighting model; 1 = full shading 
		double spec;     // specular component 0-1 
		double ambient;  // ambient component 0-1 
		double Phong;    // Phong exponent 
		int    rot_z;    // illumination angle 
		int    rot_x;    // illumination angle 
		bool   fixed;    // TRUE means the light does not rotate 
		double spec2;    // 2nd specular contribution from red spotlight on opposite side 
	};
	extern const lp_style_type default_pm3d_border; // Used to initialize `set pm3d border` 
#endif /* TERM_HELP */
//
//#include <misc.h>
//
// Variables of misc.c needed by other modules:
//
extern char * loadpath_fontname; /* Used by postscript terminal if a font file is found by GnuPlot::LoadPath_fopen() */
/* these two are global so that plot.c can load them on program entry */
extern char * call_args[10];
extern int call_argc;
//
// Prototypes from file "misc.c" 
//
const char * expand_call_arg(int c);
void filledcurves_options_tofile(filledcurves_opts *, FILE *);
long lookup_color_name(char * string);
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
	GpLexicalUnit * P_Tokens/*tokens*/;   // Input line tokens to restore 
	int    _NumTokens; /*num_tokens*/;       // How big is the above ? 
	int    _CToken/*c_token*/;          // Which one were we on ? 
	LFS  * prev;             // defines a stack 
	int    call_argc;        // This saves the _caller's_ argc 
	char * call_args[10];    // ARG0 through ARG9 from "call" command 
	GpValue argv[10];        // content of global ARGV[] array 
};
//
//#include <util3d.h>
// 
// HBB 990828: moved all those variable decl's and #defines to new
// file "graph3d.h", as the definitions are in graph3d.c, not in
// util3d.c. Include that file from here, to ensure everything is known 
//
// All the necessary information about one vertex.
//
struct GpVertex : public SPoint3R {
	//coordval x, y, z;         // vertex coordinates 
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
		xvar = (((v)->x * _3DBlk.Scaler.x)) + _3DBlk.Middle.x;      \
		yvar = (((v)->y * _3DBlk.Scaler.y)) + _3DBlk.Middle.y;      \
	}
#define TERMCOORD(v, xvar, yvar)                  \
	{                                               \
		xvar = ((int)((v)->x * _3DBlk.Scaler.x)) + _3DBlk.Middle.x; \
		yvar = ((int)((v)->y * _3DBlk.Scaler.y)) + _3DBlk.Middle.y; \
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
// Maximum number of columns returned to caller by df_readline
// Various data structures are dimensioned to hold this many entries.
// As of June 2013, plot commands never ask for more than 7 columns of
// data, but fit commands can use more. "fit" is also limited by
// the number of parameters that can be passed	to a user function, so
// let's try setting MAXDATACOLS to match.
// At present this bumps it from 7 to 14.
#define MAXDATACOLS (MAX_NUM_VAR+2)
//
// Prototypes of functions exported by datafile.c 
//
int    df_2dbinary(curve_points *);
int    df_3dmatrix(GpSurfacePoints *, int);

struct use_spec_s {
	int column;
	int expected_type;
	at_type * at;
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
#define SIGNED_TEST(val) ((val)==sizeof(long) ? DF_LONG : ((val)==sizeof(int64) ? DF_LONGLONG : \
	((val)==sizeof(int) ? DF_INT : ((val)==sizeof(short) ? DF_SHORT : ((val)==sizeof(char) ? DF_CHAR : DF_BAD_TYPE)))))
#define UNSIGNED_TEST(val) ((val)==sizeof(ulong) ? DF_ULONG : ((val)==sizeof(uint64) ? DF_ULONGLONG : \
	((val)==sizeof(uint) ? DF_UINT : ((val)==sizeof(ushort) ? DF_USHORT : ((val)==sizeof(uchar) ? DF_UCHAR : DF_BAD_TYPE)))))
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
// 
// NOTE TO THOSE WRITING FILE TYPE FUNCTIONS
// 
// "cart" means Cartesian, i.e., the (x,y,z) [or (r,t,z)] coordinate
// system of the plot.  "scan" refers to the scanning method of the
// file in question, i.e., first points, then lines, then planes.
// The important variables for a file type function to fill in are
// those beginning with "scan".  There is a tricky set of rules
// related to the "scan_cart" mapping, the file-specified variables,
// the default variables, and the command-line variables.  Basically,
// command line overrides data file which overrides default.  (Yes,
// like a confusing version of rock, paper, scissors.) So, from the
// file type function perspective, it is better to leave those
// variables which are not specifically known from file data or
// otherwise (e.g., sample periods "scan_delta") unaltered in case
// the user has issued "set datafile" to define defaults.
// 
struct df_binary_file_record_struct {
	int    cart_dim[3]; // dimension array size, x/y/z 
	int    cart_dir[3]; // 1 scan in positive direction, -1 negative, x/y/z 
	double cart_delta[3]; // spacing between array points, x/y/z 
	df_translation_type cart_trans; /* translate via origin, center or default */
	double cart_cen_or_ori[3]; /* vector representing center or origin, x/y/z */
	double cart_alpha; /* 2D rotation angle (rotate) */
	double cart_p[3]; /* 3D rotation normal vector (perpendicular) */
	df_sample_scan_type cart_scan[3]; /* how to assign the dimensions read from file when generating coordinates */
	bool   scan_generate_coord; /* whether or not Gnuplot should generate coordinates. */
	off_t  scan_skip[3]; /* skip bytes before the record, line, plane */
	// 
	// Not controllable by the user, only by file type functions.
	// These are all points/lines/planes format.
	// 
	int    scan_dim[3]; // number of points, lines, planes 
	int    scan_dir[3]; // 1 scan in positive direction wrt Cartesian coordinate system, -1 negative 
	double scan_delta[3]; // sample period along points, lines, planes 
	df_translation_type scan_trans; // translate via origin, center or default 
	double scan_cen_or_ori[3]; // vector representing center or origin, x/y/z 
	// 
	// `matrix every ::lowx:lowy:` can select a submatrix.
	// This is its size.
	// 
	int    submatrix_ncols;
	int    submatrix_nrows;
	char * memory_data; // Do not modify outside of datafile.c!!! 
};

extern const GpCoordinate blank_data_line;
//
// Prototypes of functions exported by datafile.c 
//
void df_show_datasizes(FILE * fp);
void df_show_filetypes(FILE * fp);
#define df_set_skip_after(col, bytes) DfSetSkipBefore(col+1, bytes) // Number of bytes to skip after a binary column
//
//#include <datablock.h>
void gpfree_datablock(GpValue *datablock_value);
void append_to_datablock(GpValue *datablock_value, const char * line);
void append_multiline_to_datablock(GpValue *datablock_value, const char * lines);
//
//#include <fit.h>
// defaults 
#define DEF_FIT_LIMIT 1e-5

// error interrupt for fitting routines 
#define Eex(a) { ErrorEx(NO_CARET, (a)); }
#define Eex2(a, b)    { ErrorEx(NO_CARET, (a), (b)); }
#define Eex3(a, b, c) { ErrorEx(NO_CARET, (a), (b), (c)); }
#define Eexc(c, a)    { ErrorEx((c), (a)); }
#define Eexc2(c, a, b) { ErrorEx((c), (a), (b)); }
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
	//char * readline(const char *);
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
//void save_prange(FILE *, GpAxis *);
void save_link(FILE *, GpAxis *);
void save_nonlinear(FILE *, GpAxis *);
void save_textcolor(FILE *, const t_colorspec *);
void save_pm3dcolor(FILE *, const t_colorspec *);
void save_fillstyle(FILE *, const fill_style_type *);
void save_linetype(FILE *, lp_style_type *, bool);
void save_dashtype(FILE *, int, const t_dashtype *);
//void save_num_or_time_input(FILE *, double x, const GpAxis *);
//
//#include <scanner.h>
bool legal_identifier(char *p);
//
//#include <setshow.h>
//void show_version(FILE *fp);
const char * FASTCALL conv_text(const char * s);
void delete_linestyle(struct linestyle_def **, struct linestyle_def *, struct linestyle_def *);
//void delete_dashtype(struct custom_dashtype_def *, struct custom_dashtype_def *);
void free_marklist(struct ticmark * list);
//extern int enable_reset_palette;
extern text_label * new_text_label(int tag);
extern ticmark * prune_dataticks(struct ticmark *list);
//
//#include <external.h>
#ifdef HAVE_EXTERNAL_FUNCTIONS
void external_free(struct at_type *);

#if defined(_WIN32)
	typedef void * gp_dll_t;

	#define DLL_PATHSEP "\\"
	#define DLL_EXT  ".dll"
	#define DLL_OPEN(f) dll_open_w(f);
	#define DLL_CLOSE(dl) ((void)FreeLibrary((HINSTANCE)dl))
	#define DLL_SYM(dl, sym) ((void *)GetProcAddress((HINSTANCE)dl, (sym)))
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
//#include <voxelgrid.h>
#define VOXEL_GRID_SUPPORT 1

struct isosurface_opt {
	int tessellation; /* 0 = mixed  1 = triangles only */
	int inside_offset; /* difference between front/back linetypes */
};

void   vgrid_stats(VGrid * vgrid);
//
//#include <dynarray.h>
struct dynarray {
	dynarray() : v(0), size(0), end(0), increment(0), entry_size(0)
	{
	}
	long size; /* alloced size of the array */
	long end; /* index of first unused entry */
	long increment; /* amount to increment size by, on realloc */
	size_t entry_size; /* size of the entries in this array */
	void * v; /* the vector itself */
};
//
// Prototypes 
//
void   init_dynarray(dynarray * array, size_t element, long size, long increment);
void   free_dynarray(dynarray * array);
//
//#include <complexfun.h>
#ifdef HAVE_COMPLEX_FUNCS
	//
	// Prototypes for complex functions
	//
	//void f_Igamma(union argument *arg);
	//void f_LambertW(union argument *arg);
	//void f_lnGamma(union argument *arg);
	//void f_Sign(union argument *arg);
	double igamma(double a, double z);
#endif
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
//#include <matrix.h>
//
// public functions
//
double * vec(int n);
int  * ivec(int n);
double ** matr(int r, int c);
void   free_matr(double ** m);
double * redim_vec(double ** v, int n);
void    solve(double ** a, int n, double ** b, int m);
//void    Givens(double ** C, double * d, double * x, int N, int n);
//void    Invert_RtR(double ** R, double ** I, int n);
//
// Functions for use by THIN_PLATE_SPLINES_GRID method
//
//void    lu_decomp(double **, int, int *, double *);
void   lu_backsubst(double **, int n, int *, double *);
double enorm_vec(int n, const double * x);
double sumsq_vec(int n, const double * x);
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
#define contour_levels_list ((double *)_Cntr.dyn_contour_levels_list.v)
//
//#include <hidden3d.h>
#define PT_ARROWHEAD -10
#define PT_BACKARROW -11
#define PT_BOTHHEADS -12

extern bool disable_mouse_z;
//
//#include <encoding.h>
enum set_encoding_id encoding_from_locale();
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
//int  help(char * keyword, char * path, bool * subtopics);
void FreeHelp();
//void StartOutput();
void OutLine(const char *line);
void EndOutput();
//
//#include <interpol.h>
//
// Prototypes of functions exported by interpol.c 
//
void sort_points(curve_points *plot);
void zsort_points(curve_points *plot);
//
//#include <mousecmn.h>
// 
// Definitions that are used by both gnuplot core and standalone terminals.
// 
// 
// Descr: Structure for reporting mouse events to the main program
// 
struct GpEvent {
	int    type;  // see below 
	int    mx;    // current mouse coordinates 
	int    my;
	int    par1;  // other parameters, depending on the event type 
	int    par2;
	int    winid; // ID of window in which the event occurred 
};
// 
// event types:
// 
#define GE_EVT_LIST(_) \
	_(GE_motion)        /* mouse has moved */ \
	_(GE_buttonpress)   /* mouse button has been pressed; par1 = number of the button (1, 2, 3...) */ \
	_(GE_buttonrelease) /* mouse button has been released; par1 = number of the button (1, 2, 3...); par2 = time (ms) since previous button release */ \
	_(GE_keypress)      /* keypress; par1 = keycode (either ASCII, or one of the GP_ enums defined below); par2 = (|1 .. don't pass through bindings )*/ \
	_(GE_buttonpress_old) /* same as GE_buttonpress but triggered from inactive window */ \
	_(GE_buttonrelease_old) /* same as GE_buttonrelease but triggered from inactive window */ \
	_(GE_keypress_old)  /* same as GE_keypress but triggered from inactive window */ \
	_(GE_modifier)      /* shift/ctrl/alt key pressed or released; par1 = is new mask, see Mod_ enums below */ \
	_(GE_plotdone)      /* acknowledgement of plot completion (for synchronization) */ \
	_(GE_replot)        /* used only by ggi.trm */ \
	_(GE_reset)         /* reset to a well-defined state (e.g.  after an X11 error occured) */ \
	_(GE_fontprops)     /* par1 = hchar par2 = vchar */ \
	_(GE_pending)       /* signal gp_exec_event() to send pending events */ \
	_(GE_raise)         /* raise console window */ \

#define GE_EVT_DEFINE_ENUM(name) name,
enum {
	GE_EVT_LIST(GE_EVT_DEFINE_ENUM)
	GE_EVT_NUM
};
// 
// the status of the shift, ctrl and alt keys
// Mod_Opt is used by the "bind" mechanism to indicate that the
// Ctrl or Alt key is allowed but not required
// 
enum { 
	Mod_Shift = (1), 
	Mod_Ctrl = (1 << 1), 
	Mod_Alt = (1 << 2), 
	Mod_Opt = (1 << 3) 
};
// 
// the below depends on the ascii character set lying in the
// range from 0 to 255 (below 1000) 
// 
enum { /* special keys with "usual well-known" keycodes */
	GP_BackSpace = 0x08,
	GP_Tab = 0x09,
	GP_KP_Enter = 0x0A,
	GP_Return = 0x0D,
	GP_Escape = 0x1B,
	GP_Delete = 127
};

enum { /* other special keys */
	GP_FIRST_KEY = 1000,
	GP_Linefeed,
	GP_Clear,
	GP_Pause,
	GP_Scroll_Lock,
	GP_Sys_Req,
	GP_Insert,
	GP_Home,
	GP_Left,
	GP_Up,
	GP_Right,
	GP_Down,
	GP_PageUp,
	GP_PageDown,
	GP_End,
	GP_Begin,
	GP_KP_Space,
	GP_KP_Tab,
	GP_KP_F1,
	GP_KP_F2,
	GP_KP_F3,
	GP_KP_F4,

	GP_KP_Insert, /* ~ KP_0 */
	GP_KP_End,   /* ~ KP_1 */
	GP_KP_Down,  /* ~ KP_2 */
	GP_KP_Page_Down, /* ~ KP_3 */
	GP_KP_Left,  /* ~ KP_4 */
	GP_KP_Begin, /* ~ KP_5 */
	GP_KP_Right, /* ~ KP_6 */
	GP_KP_Home,  /* ~ KP_7 */
	GP_KP_Up,    /* ~ KP_8 */
	GP_KP_Page_Up, /* ~ KP_9 */

	GP_KP_Delete,
	GP_KP_Equal,
	GP_KP_Multiply,
	GP_KP_Add,
	GP_KP_Separator,
	GP_KP_Subtract,
	GP_KP_Decimal,
	GP_KP_Divide,
	GP_KP_0,
	GP_KP_1,
	GP_KP_2,
	GP_KP_3,
	GP_KP_4,
	GP_KP_5,
	GP_KP_6,
	GP_KP_7,
	GP_KP_8,
	GP_KP_9,
	GP_F1,
	GP_F2,
	GP_F3,
	GP_F4,
	GP_F5,
	GP_F6,
	GP_F7,
	GP_F8,
	GP_F9,
	GP_F10,
	GP_F11,
	GP_F12,
	GP_Cancel,
	GP_Button1, /* Buttons must come last */
	GP_Button2,
	GP_Button3,
	GP_LAST_KEY
};
//
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
		t_zoom * prev;
		t_zoom * next;
	};

	struct mouse_setting_t {
		int on; // ...                                        
		int doubleclick; // Button1 double / single click resolution   
		int annotate_zoom_box; // draw coordinates at zoom box               
		int label; // draw real gnuplot labels on Button 2       
		int polardistance; // display dist. to ruler in polar coordinates
		int verbose; // display ipc commands                       
		int warp_pointer; // warp pointer after starting a zoom box     
		double xmzoom_factor; // scale factor for +/- zoom on x		 
		double ymzoom_factor; // scale factor for +/- zoom on y		 
		char * fmt; // fprintf format for printing numbers        
		char * labelopts; // label options                              
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

	bool exec_event(char type, int mx, int my, int par1, int par2, int winid); /* wrapper for do_event() */
//
#endif
//#include "xdg.h"
#ifdef __unix__ // {
	//
	// List of possible XDG variables 
	//
	enum XDGVarType {
		kXDGNone = -1,
		kXDGConfigHome,  // XDG_CONFIG_HOME
		kXDGDataHome,    // XDG_DATA_HOME
		kXDGCacheHome,   // XDG_CACHE_HOME
		kXDGRuntimeDir,  // XDG_RUNTIME_DIR
		kXDGConfigDirs,  // XDG_CONFIG_DIRS
		kXDGDataDirs,    // XDG_DATA_DIRS
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
		GpEvent ge;
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
	//void FASTCALL Push(GpValue * x);
	//GpValue * FASTCALL Pop(GpValue * x);
	void SetJumpOffset(int a)
	{
		JumpOffset = a;
	}
	int  GetJumpOffset() const { return JumpOffset; }

	GpValue St[STACK_DEPTH];
	int    Sp; // stack pointer 
	int    JumpOffset; // to be modified by 'jump' operators 
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
		xscale(1.0), yscale(1.0), auto_layout_margins(false), title_height(0.0)
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
	SPoint2F Offset; // shift
	bool   auto_layout_margins;
	GpPosition lmargin;
	GpPosition rmargin;
	GpPosition bmargin;
	GpPosition tmargin;
	GpPosition xspacing;
	GpPosition yspacing;
	SPoint2F   PrevSize;
	SPoint2F   PrevOffset;
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
	TermDumbBlock() : P_Matrix(0), P_Colors(0), PtMax(DUMB_XMAX, DUMB_YMAX), /*XMax(DUMB_XMAX), YMax(DUMB_YMAX),*/ Feed(true),
		ColorMode(0), Pen(0)/*, X(0), Y(0)*/
	{
	}
	//#define DUMB_PIXEL(x, y) GPO.TDumbB.P_Matrix[GPO.TDumbB.XMax*(y)+(x)]
	charcell & Pixel(uint x, uint y) { return P_Matrix[PtMax.x * (y) + (x)]; }
	charcell * P_Matrix; // matrix of characters 
	void   DumbRgbColor(rgb255_color rgb255, char * pColorString);
#ifndef NO_DUMB_COLOR_SUPPORT
	// matrix of colors 
	t_colorspec * P_Colors;
	t_colorspec Color;
	t_colorspec PrevColor;
#endif
	char Pen; // current character used to draw 
	//int  X;   // current X position 
	//int  Y;   // current Y position 
	//int  XMax;
	//int  YMax;
	SPoint2I Pt;     // current position 
	SPoint2I PtMax;
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
	bool   duplex_option; // one of duplex or simplex specified?
	bool   duplex_state;
	bool   rounded; // rounded linecaps and linejoins
	bool   clipped; // path clipped to BoundingBox?
	ps_fontfile_def * first_fontfile;
	char   font[MAX_ID_LEN+1]; // name of font
	float  fontsize; // size of font in pts
	float  fontscale; // multiplier for nominal font size
	bool   useauxfile; // only necessary for ps(la)tex
	bool   rotate; // only necessary for ps(la)tex
	int    palfunc_samples; // setable via "palf$uncparam"
	double palfunc_deviation; // terminal option
	bool   oldstyle;
	bool   epslatex_standalone;
	bool   adobeglyphnames; // Choice of output names for UTF8
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
	int    Page; /* page count */
	int    PathCount; /* count of lines in path */
	int    Ang; /* text angle */
	enum   JUSTIFY Justify; /* text is flush left */
	ps_params_t * P_Params;// = &post_params;
	char * PsLatexAuxname; // name of auxiliary file 
};

enum marq_res {
	OK, 
	ML_ERROR, 
	BETTER, 
	WORSE
};

typedef enum marq_res marq_res_t;

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
	double ssd; /* sample standard deviation */
	double skewness;
	double kurtosis;
	double mean_err;
	double stddev_err;
	double skewness_err;
	double kurtosis_err;
	double sum; /* sum x    */
	double sum_sq; /* sum x**2 */
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
	double slope; /* linear regression */
	double intercept;
	double slope_err;
	double intercept_err;
	double correlation;
	double pos_min_y; // x coordinate of min y 
	double pos_max_y; // x coordinate of max y 
};

struct GpGadgets {
	GpGadgets() : P_FirstCustomDashtype(0), P_FirstLabel(0), P_FirstLineStyle(0), P_FirstPermLineStyle(0), P_FirstMonoLineStyle(0),
		P_FirstArrowStyle(0), P_PixmapListHead(0), P_FirstArrow(0), P_FirstObject(0), TimeLabelBottom(TRUE),
		CornerPoles(true), ClipLines1(true), ClipLines2(false), ClipPoints(false), ClipRadial(false),
		Polar(false), InvertedRaxis(false), SpiderPlot(false), Parametric(false), InParametric(false), Is3DPlot(false), VolatileData(false),
		PreferLineStyles(false), Zero(ZERO), PointSize(1.0), PointIntervalBox(1.0), Samples1(SAMPLES), Samples2(SAMPLES),
		IsoSamples1(ISO_SAMPLES), IsoSamples2(ISO_SAMPLES),
		//
		draw_border(31), user_border(31), border_layer(LAYER_FRONT), refresh_nplots(0), current_x11_windowid(0), ang2rad(1.0),
		border_lp(lp_style_type::defBorder), data_style(POINTSTYLE), func_style(LINES), refresh_ok(E_REFRESH_NOT_OK),
		default_fillstyle(FS_EMPTY, 100, 0), default_rectangle(t_object::defRectangle), default_circle(t_object::defCircle), default_ellipse(t_object::defEllipse),
		// @ctr filledcurves_opts_data(EMPTY_FILLEDCURVES_OPTS), 
		// @ctr filledcurves_opts_func(EMPTY_FILLEDCURVES_OPTS), 
		histogram_opts(), boxplot_opts(DEFAULT_BOXPLOT_STYLE)
	{
		memzero(textbox_opts, sizeof(textbox_opts));
	}
	void   DeleteArrow(arrow_def * pPrev, arrow_def * pThis);
	custom_dashtype_def * P_FirstCustomDashtype; // Pointer to first 'set dashtype' definition in linked list 
	text_label * P_FirstLabel; // Pointer to the start of the linked list of 'set label' definitions 
	linestyle_def * P_FirstLineStyle;
	linestyle_def * P_FirstPermLineStyle;
	linestyle_def * P_FirstMonoLineStyle;
	arrowstyle_def * P_FirstArrowStyle; // Pointer to first 'set style arrow' definition in linked list 
	t_pixmap * P_PixmapListHead; // Listhead for pixmaps 
	arrow_def * P_FirstArrow; // set arrow 
	GpObject  * P_FirstObject; // Pointer to first object instance in linked list 
	text_label LblTitle; // = EMPTY_LABELSTRUCT; /* 'set title' status */
	// 'set timelabel' status 
	text_label LblTime; // = EMPTY_LABELSTRUCT;
	pa_style   ParallelAxisStyle; // = DEFAULT_PARALLEL_AXIS_STYLE; // Holds the properties from 'set style parallelaxis' 
	spider_web SpiderPlotStyle; // = DEFAULT_SPIDERPLOT_STYLE; // Holds properties for 'set style spiderplot' 
	int    TimeLabelBottom;
	// set samples 
	int    Samples1; // = SAMPLES;
	int    Samples2; // = SAMPLES;
	int    IsoSamples1; // = ISO_SAMPLES;
	int    IsoSamples2; // = ISO_SAMPLES;
	double Zero; // zero threshold, may _not_ be 0! 
	// Status of 'set pointsize' and 'set pointintervalbox' commands 
	double PointSize; // Status of 'set pointsize' command
	double PointIntervalBox; // Status of 'set pointintervalbox' command
	bool   CornerPoles;
	bool   ClipLines1;
	bool   ClipLines2;
	bool   ClipPoints;
	bool   ClipRadial;
	bool   Polar;
	bool   InvertedRaxis;
	bool   SpiderPlot;           // toggle spiderplot mode on/off 
	bool   Parametric;
	bool   InParametric;
	bool   Is3DPlot;             // If last plot was a 3d one. 
	bool   VolatileData;         // Flag to show that volatile input data is present 
	bool   PreferLineStyles;     // Prefer line styles over plain line types 
	color_box_struct ColorBox;   // initialized in init_color() 
	GpObject GridWall[5];
	legend_key KeyT;             // = DEFAULT_KEY_PROPS;
	SPoint2I MovPos;             // support of ClipMove and ClipVector
	//
	int    draw_border;          // The current settings 
	int    user_border;          // What the user last set explicitly 
	int    border_layer;         // 
	int    refresh_nplots;       // FIXME: do_plot should be able to figure this out on its own! 
	int    current_x11_windowid; // WINDOWID to be filled by terminals running on X11 (x11, wxt, qt, ...) 
	double ang2rad;              // 1 or pi/180, tracking angles_format 
	lp_style_type border_lp;     // = DEFAULT_BORDER_LP;
	enum PLOT_STYLE data_style;
	enum PLOT_STYLE func_style;
	TRefresh_Allowed refresh_ok; // Flag to signal that the existing data is valid for a quick refresh 
	fill_style_type default_fillstyle; // = { FS_EMPTY, 100, 0, DEFAULT_COLORSPEC };
	// Default rectangle style - background fill, black border 
	GpObject default_rectangle;//= DEFAULT_RECTANGLE_STYLE;
	GpObject default_circle;// = DEFAULT_CIRCLE_STYLE;
	GpObject default_ellipse;// = DEFAULT_ELLIPSE_STYLE;
	// filledcurves style options 
	filledcurves_opts filledcurves_opts_data;
	filledcurves_opts filledcurves_opts_func;
	histogram_style histogram_opts; // = DEFAULT_HISTOGRAM_STYLE;
	boxplot_style boxplot_opts;
	textbox_style textbox_opts[NUM_TEXTBOX_STYLES];
};

struct GpGraphics {
	GpGraphics() : RetainOffsets(false), BarSize(1.0), BarLayer(LAYER_FRONT), BarLp(), RgbMax(255.0), LargestPolarCircle(0.0),
		P_StackHeight(0), StackCount(0)/*, BoxplotFactorSortRequired(false)*/
	{
		LOff = {first_axes, first_axes, first_axes, 0.0, 0.0, 0.0};
		ROff = {first_axes, first_axes, first_axes, 0.0, 0.0, 0.0};
		TOff = {first_axes, first_axes, first_axes, 0.0, 0.0, 0.0};
		BOff = {first_axes, first_axes, first_axes, 0.0, 0.0, 0.0};
	}
	double RgbScale(double component) const;
	int    FilterBoxplot(curve_points * pPlot);
	// 
	// 'set offset' --- artificial buffer zone between coordinate axes and
	// the area actually covered by the data.
	// The retain_offsets flag is an interlock to prevent repeated application
	// of the offsets when a plot is refreshed or scrolled.
	// 
	GpPosition LOff;
	GpPosition ROff;
	GpPosition TOff;
	GpPosition BOff;
	bool   RetainOffsets;
	double BarSize;
	int    BarLayer;
	lp_style_type BarLp;
	double RgbMax; // 'set rgbmax {0|255}' 
	// End points and tickmark offsets for radial axes in spiderplots 
	SPoint2R Spoke0;
	SPoint2R Spoke1;
	SPoint2R SpokeD;
	// NB: x-axis coordinates, not polar. updated by xtick2d_callback. 
	double LargestPolarCircle;
	// Status information for stacked histogram plots 
	GpCoordinate * P_StackHeight; // top of previous row 
	int    StackCount; // points actually used 
private:
	//bool   BoxplotFactorSortRequired; // used by compare_ypoints via q_sort from filter_boxplot 
};

struct GpBoundary {
	GpBoundary() :
		xlablin(0),
		x2lablin(0),
		ylablin(0),
		y2lablin(0),
		titlelin(0),
		xticlin(0),
		x2ticlin(0),
		key_sample_left(0),
		key_sample_right(0),
		key_text_left(0),
		key_text_right(0),
		key_size_left(0),
		key_size_right(0),
		key_xleft(0),
		max_ptitl_len(0),
		ptitl_cnt(0),
		key_title_height(0),
		key_title_extra(0),
		key_title_ypos(0),
		key_col_wth(0),
		yl_ref(0),
		key_entry_height(0),
		key_point_offset(0),
		ylabel_x(0),
		xlabel_y(0),
		y2label_x(0),
		x2label_y(0),
		x2label_yoffset(0),
		ylabel_y(0),
		y2label_y(0),
		xtic_y(0),
		x2tic_y(0),
		ytic_x(0),
		y2tic_x(0),
		key_rows(0),
		key_cols(0),
		key_count(0)
	{
	}
	//
	// stupid test used in only one place but it refers to our local variables 
	//
	bool   AtLeftOfKey() const { return (L.y == yl_ref); }
	int    xlablin;
	int    x2lablin;
	int    ylablin;
	int    y2lablin;
	int    titlelin;
	int    xticlin;
	int    x2ticlin;
	SPoint2I KeySampleS;        // Size of line sample. KeySampleS.y is sample itself; does not scale with "set key spacing" 
	int    key_sample_left;     // offset from x for left of line sample 
	int    key_sample_right;    // offset from x for right of line sample 
	int    key_text_left;       // offset from x for left-justified text 
	int    key_text_right;      // offset from x for right-justified text 
	int    key_size_left;       // size of left bit of key (text or sample, depends on key->reverse) 
	int    key_size_right;      // size of right part of key (including padding) 
	int    key_xleft;           // Amount of space on the left required by the key 
	int    max_ptitl_len;       // max length of plot-titles (keys) 
	int    ptitl_cnt;           // count keys with len > 0  
	//int    key_width;           // calculate once, then everyone uses it 
	//int    key_height;          // ditto 
	SPoint2I KeyS;              // Calculate once, then everyone uses it    
	int    key_title_height;    // nominal number of lines * character height 
	int    key_title_extra;     // allow room for subscript/superscript 
	int    key_title_ypos;      // offset from key->bounds.ytop 
	SPoint2I TmP; // 
	int    key_col_wth;
	int    yl_ref;
	SPoint2I L;
	SPoint2I TitlePos;
	//
	// These quantities are needed in do_plot() e.g. for histogtram title layout
	//
	int    key_entry_height; // bigger of t->ChrV, t->TicV 
	int    key_point_offset; // offset from x for point sample 
	int    ylabel_x;
	int    xlabel_y;
	int    y2label_x;
	int    x2label_y;
	int    x2label_yoffset;
	int    ylabel_y;
	int    y2label_y;
	int    xtic_y;
	int    x2tic_y;
	int    ytic_x;
	int    y2tic_x;
	int    key_rows;
	int    key_cols;
	int    key_count;
};

struct GpGraph3DBlock {
	GpGraph3DBlock();
	int    KeyEntryHeight; // bigger of t->ChrV, pointsize*t->v_tick 
	int    KeyTitleHeight;
	int    KeyTitleExtra;  // allow room for subscript/superscript 
	int    KeyTitleWidth;
	// 
	// we precalculate features of the key, to save lots of nested
	// ifs in code - x,y = user supplied or computed position of key
	// taken to be inner edge of a line sample
	// 
	int    KeySampleLeft;  // offset from x for left of line sample 
	int    KeySampleRight; // offset from x for right of line sample 
	int    KeyPointOffset; // offset from x for point sample 
	int    KeyTextLeft;    // offset from x for left-justified text 
	int    KeyTextRight;   // offset from x for right-justified text 
	int    KeySizeLeft;    // distance from x to left edge of box 
	int    KeySizeRight;   // distance from x to right edge of box 
	int    SPlotMapActive;
	float  SPlotMapSurfaceRotX;
	float  SPlotMapSurfaceRotZ;
	float  SPlotMapSurfaceScale;
	//
	// x and y input range endpoints where the three axes are to be
	// displayed (left, front-left, and front-right edges of the cube) 
	//
	double XAxisY;
	double YAxisX;
	double ZAxisX;
	double ZAxisY;
	// 
	// ... and the same for the back, right, and front corners 
	// 
	SPoint2R Back;
	SPoint2R Right;
	SPoint2R Front;
	SPoint3R TicUnit; // unit vector (terminal coords) 
	//
	int    PTitlCnt;
	int    MaxPTitlLen;
	int    TitleLin;
	int    KeySampleWidth;
	int    KeyRows;
	int    KeyCols;
	int    KeyColWth;
	int    YlRef;
	double KTitleLines;
	double CeilingZ1;
	double BaseZ1;
	SPoint3R Scale3D;
	SPoint3R Center3D;
	// 
	// Rotation and scale of the 3d view, as controlled by 'set view': 
	// 
	float  SurfaceRotZ;
	float  SurfaceRotX;
	float  SurfaceScale;
	float  SurfaceZScale;
	float  SurfaceLScale;
	float  MapviewScale;
	float  Azimuth;
	// 
	// Define the boundary of the plot
	// These are computed at each call to do_plot, and are constant over
	// the period of one do_plot. They actually only change when the term
	// type changes and when the 'set size' factors change.
	// 
	SPoint2I Middle;
	SPoint2I Scaler;
	double xyscaler;
	double radius_scaler;
	//
	t_contour_placement draw_contour; // is contouring wanted?
	int    clabel_interval; // label every 20th contour segment 
	int    clabel_start; // starting with the 5th 
	int    hidden3d_layer; // LAYER_FRONT or LAYER_BACK 
	char * clabel_font; // default to current font 
	t_xyplane xyplane; // position of the base plane, as given by 'set ticslevel' or 'set xyplane' 
	// 
	// The global flags splot_map, xz_projection, and yz_projection are specific views.
	// These flag the more general case of looking down the x or y axis
	// 
	bool   XzPlane;
	bool   YzPlane;
	bool   CanPm3D;
	bool   clabel_onecolor; // use same linetype for all contours 
	bool   draw_surface; // Draw the surface at all? (FALSE if only contours are wanted) 
	bool   implicit_surface; // Always create a gridded surface when lines are read from a data file 
	bool   hidden3d; // Was hidden3d display selected by user? 
	// 
	// These flags indicate projection onto the xy, xz or yz plane
	// as requested by 'set view map' or 'set view projection'.
	// in_3d_polygon disables conversion of graph coordinates from x/y/z to
	// hor/ver in projection; i.e. polygon vertices are always orthogonal x/y/z.
	// 
	bool   splot_map;
	bool   xz_projection;
	bool   yz_projection;
	bool   in_3d_polygon;
	//
	// Boundary and scale factors, in user coordinates 
	//
	// These positions assume a single linear scale encompassing the
	// zrange plus extra space below for the baseplane.  This was messy but
	// correct before the introduction of nonlinear axes. Now - not so much.
	//
	// ceiling_z is the highest z in use
	// floor_z   is the lowest z in use
	// base_z    is the z of the base
	// min3d_z   is the lowest z of the graph area
	// max3d_z   is the highest z of the graph area
	//
	// ceiling_z is either max3d_z or base_z, and similarly for floor_z
	// There should be no part of graph drawn outside
	// min3d_z:max3d_z  - apart from arrows, perhaps
	//
	double floor_z;   // is the lowest z in use
	double ceiling_z; // is the highest z in use
	double base_z;    // is the z of the base
	double floor_z1;  // To handle a non-linear z axis we need to calculate these values on the other end of the linked linear:nonlinear axis pair.
	transform_matrix trans_mat;
#ifdef USE_MOUSE
	int    axis3d_o_x;
	int    axis3d_o_y;
	int    axis3d_x_dx;
	int    axis3d_x_dy;
	int    axis3d_y_dx;
	int    axis3d_y_dy;
#endif
};

#define NO_COLUMN_HEADER (-99) // some value that can never be a real column 
// 
// rather than three arrays which all grow dynamically, make one dynamic array of this structure
// 
struct df_column_struct {
	double datum;
	enum DF_STATUS good;
	char * position; // points to start of this field in current line 
	char * header;   // points to copy of the header for this column 
};

struct GpParseBlock {
	GpParseBlock() : P_At(0), AtSize(0), RecursionLevel(0),
		string_result_only(false), scanning_range_in_progress(false), parse_1st_row_as_headers(false),
		at_highest_column_used(-1), df_array(0), plot_iterator(0), set_iterator(0)
	{
		memzero(c_dummy_var, sizeof(c_dummy_var));
		memzero(fit_dummy_var, sizeof(fit_dummy_var));
		memzero(set_dummy_var, sizeof(set_dummy_var));
		STRNSCPY(set_dummy_var[0], "x");
		STRNSCPY(set_dummy_var[1], "y");
	}
	at_type * P_At;
	int    AtSize;
	int    RecursionLevel;
	//
	char   c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
	char   set_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1]; // = { "x", "y" };
	int    fit_dummy_var[MAX_NUM_VAR];
	// 
	// Protection mechanism for trying to parse a string followed by a + or - sign.
	// Also suppresses an undefined variable message if an unrecognized token
	// is encountered during try_to_get_string().
	// 
	bool   string_result_only;
	bool   scanning_range_in_progress;
	bool   parse_1st_row_as_headers; // This is checked by df_readascii() 
	int    at_highest_column_used; // This is used by plot_option_using() 
	udvt_entry * df_array; // This is used by df_open() and df_readascii() 
	// Iteration structures used for bookkeeping 
	GpIterator * plot_iterator;
	GpIterator * set_iterator;
};
//
//
//
struct GpReadLineBlock {
	GpReadLineBlock() : P_CurLine(0), LineLen(0), CurPos(0), MaxPos(0), SearchMode(false),
		P_SearchPrompt("search '"), P_SearchPrompt2("': "), P_SearchResult(0), SearchResultWidth(0)
	{
	}
	char * P_CurLine; // current contents of the line 
	size_t LineLen;
	size_t CurPos; // current position of the cursor 
	size_t MaxPos; // maximum character position 
	bool   SearchMode;
	const char * P_SearchPrompt;
	const char * P_SearchPrompt2;
	HIST_ENTRY * P_SearchResult;
	int    SearchResultWidth; // on-screen width of the search result 
};

#define MAX_POINTS_PER_CNTR     100
// If you don't want the color-distinction between the
// 'top' and 'bottom' sides of the surface, like I do, then just compile
// with -DBACKSIDE_LINETYPE_OFFSET = 0. 
#ifndef BACKSIDE_LINETYPE_OFFSET
	#define BACKSIDE_LINETYPE_OFFSET 1
#endif
// This #define lets you choose if the diagonals that
// divide each original quadrangle in two triangles will be drawn
// visible or not: do draw them, define it to be 7L, otherwise let be 3L 
#ifndef TRIANGLE_LINESDRAWN_PATTERN
	#define TRIANGLE_LINESDRAWN_PATTERN 3L
#endif
// Handle out-of-range or undefined points. Compares the maximum
// marking (0=inrange, 1=outrange, 2=undefined) of the coordinates of
// a vertex to the value #defined here. If not less, the vertex is
// rejected, and all edges that hit it, as well. NB: if this is set to
// anything above 1, gnuplot may crash with a floating point exception
// in hidden3d. You get what you asked for ... 
#ifndef HANDLE_UNDEFINED_POINTS
	#define HANDLE_UNDEFINED_POINTS 1
#endif
#define UNHANDLED (UNDEFINED+1) // Symbolic value for 'do not handle Undefined Points specially'

// If both subtriangles of a quad were cancelled, try if using the
// other diagonal is better. This only makes a difference if exactly
// one vertex of the quad is unusable, and that one is on the 'usual'
// tried diagonal. In such a case, the border of the hole in the
// surface will be less rough than with the previous method, as the
// border follows the undefined region as close as it can. 
#ifndef SHOW_ALTERNATIVE_DIAGONAL
	#define SHOW_ALTERNATIVE_DIAGONAL 1
#endif
// If the two triangles in a quad are both drawn, and they show
// different sides to the user (the quad is 'bent over'), then it's
// preferable to force the diagonal being visible to avoid other
// parts of the scene being obscured by a line the user can't
// see. This avoids unnecessary user surprises. 
#ifndef HANDLE_BENTOVER_QUADRANGLES
	#define HANDLE_BENTOVER_QUADRANGLES 1
#endif
// 
// in order to support multiple axes, and to simplify ranging in
// parametric plots, we use arrays to store some things. For 2d plots,
// elements are z=0,y1=1,x1=2,z2=4,y2=5,x2=6 these are given symbolic
// names in plot.h
// 
#define GP_SPLINE_COEFF_SIZE  4
#define LAMBDA_UP_FACTOR     10
#define LAMBDA_DOWN_FACTOR   10
typedef double GpSplineCoeff[GP_SPLINE_COEFF_SIZE];
typedef double GpFiveDiag[5];
//
// Function pointer type for callback functions to generate ticmarks 
//
typedef void (GnuPlot::*GpTicCallback)(GpTermEntry * pTerm, GpAxis *, double, const char *, int, const lp_style_type & rGrid, ticmark *);
typedef char * (GnuPlot::*BuiltinEventHandler)(GpEvent * ge, GpTermEntry * pTerm);
typedef void (GnuPlot::*RegressCleanUpProc)();

typedef coordval t_plane[4]; // A plane equation, stored as a four-element vector. The equation itself is: x*p[0]+y*p[1]+z*p[2]+1*p[3]=0 

class GnuPlot {
public:
	enum FileType {
		filtypUndef = -1,
		filtypAvs = 1,
		filtypBin,
		filtypEdf, // edf
		filtypEhf, // ehf
		filtypGif,
		filtypGpBin,
		filtypJpeg, // jpeg, jpg
		filtypPng,
		filtypRaw,
		filtypRgb,
		filtypAuto,
	};
	struct GpDataFile {
		GpDataFile() : BlankCount(0), df_lower_index(0), df_upper_index(MAXINT), df_index_step(1), df_current_index(0), df_last_index_read(0),
			P_IndexName(NULL), LongestColumnHead(0), EveryPoint(1), FirstPoint(0), LastPoint(MAXINT), everyline(1), firstline(0), lastline(MAXINT),
			PointCount(-1), LineCount(0), df_skip_at_front(0), df_pseudodata(0), df_pseudorecord(0), df_pseudospan(0), df_pseudovalue_0(0.0),
			df_pseudovalue_1(0.0), df_datablock_line(0), df_array_index(0), df_arrayname(0), df_xpixels(0), df_ypixels(0), df_format(0), df_binary_format(0),
			df_line(0), MaxLineLen(0), df_eof(0), df_no_tic_specs(0), df_column(0), df_max_cols(0), df_no_cols(0), fast_columns(0), df_current_plot(0),
			data_fp(0), df_M_count(0), df_N_count(0), df_O_count(0), df_column_bininfo(0), df_max_bininfo_cols(0), df_plot_mode(0),
			BinFileType(filtypAvs), BinFileTypeDefault(filtypAvs), df_bin_file_endianess_default(DF_LITTLE_ENDIAN), 
			BinFileEndianess(DF_LITTLE_ENDIAN), BinFileTypeReset(filtypUndef),
			ColumnForKeyTitle(NO_COLUMN_HEADER), 
			IndexFound(false), SetEvery(false), df_datablock(false), df_transpose(false), mixed_data_fp(false), df_tabulate_strings(false),
			df_read_binary(false), df_nonuniform_matrix(false), df_matrix_columnheaders(false), df_matrix_rowheaders(false), df_already_got_headers(false),
			df_columnheaders(false), df_matrix(false), df_binary(false), df_binary_file(false), df_matrix_file(false), df_voxelgrid(false), plotted_data_from_stdin(false),
			df_fortran_constants(false), df_nofpe_trap(false), evaluate_inside_using(false), df_warn_on_missing_columnheader(false),
			df_no_use_specs(0), df_line_number(0), df_datum(0), df_last_col(0), df_bad_matrix_values(0), df_pixeldata(0), missing_val(0),
			df_separators(0), df_commentschars(0),
			df_max_num_bin_records(0),
			NumBinRecords(0),
			df_bin_record_count(0),
			df_max_num_bin_records_default(0),
			df_num_bin_records_default(0),
			df_no_bin_cols(0),
			df_bin_record(0),
			df_bin_record_default(0),
			df_filename(0),
			df_key_title(0),
			df_plot_title_at(0),
			#if defined(PIPES)
				_Df.df_pipe_open(false)
			#endif
			#if defined(HAVE_FDOPEN)
				data_fd(-2)
			#endif
		{
			memzero(df_stringexpression, sizeof(df_stringexpression));
			memzero(df_axis, sizeof(df_axis));
			memzero(UseSpec, sizeof(UseSpec));
			memzero(df_tokens, sizeof(df_tokens));
			memzero(df_strings, sizeof(df_strings));
		}
		//
		// Allocate space for more data columns as needed 
		//
		void ExpandColumn(int newMax)
		{
			df_column = (df_column_struct *)SAlloc::R(df_column, newMax * sizeof(df_column_struct));
			for(; df_max_cols < newMax; df_max_cols++) {
				df_column[df_max_cols].datum = 0;
				df_column[df_max_cols].header = NULL;
				df_column[df_max_cols].position = NULL;
			}
		}
		//
		// Clear column headers stored for previous plot 
		//
		void ClearColumnHeaders()
		{
			for(int i = 0; i < df_max_cols; i++) {
				ZFREE(df_column[i].header);
			}
			LongestColumnHead = 0;
		}
		// stuff for implementing index 
		int    BlankCount; // how many blank lines recently 
		int    df_lower_index; // first mesh required 
		int    df_upper_index;
		int    df_index_step; // 'every' for indices 
		int    df_current_index;  // current mesh 
		int    df_last_index_read; // last mesh we actually read data from 
		// stuff for named index support 
		char * P_IndexName;
		int    LongestColumnHead;
		// stuff for every point:line 
		int    EveryPoint;
		int    FirstPoint;
		int    LastPoint;
		int    everyline;
		int    firstline;
		int    lastline;
		int    PointCount; // point counter - preincrement and test 0 
		int    LineCount;  // line counter 
		int    df_skip_at_front; // for ascii file "skip" lines at head of file 
		// for pseudo-data (1 if filename = '+'; 2 if filename = '++') 
		int    df_pseudodata;
		int    df_pseudorecord;
		int    df_pseudospan;
		double df_pseudovalue_0;
		double df_pseudovalue_1;
		char ** df_datablock_line;
		// for arrays 
		int    df_array_index;
		const  char * df_arrayname;
		// track dimensions of input matrix/array/image 
		uint   df_xpixels;
		uint   df_ypixels;
		char * df_format;
		char * df_binary_format;
		// Bookkeeping for df_fgets() and df_gets(). Must be initialized before any calls to either function.
		char * df_line;
		size_t MaxLineLen;
		int    df_eof;
		int    df_no_tic_specs; // ticlabel columns not counted in df_no_use_specs 
		df_column_struct * df_column; // we'll allocate space as needed 
		int    df_max_cols; // space allocated 
		int    df_no_cols;   // total number of columns found in input lines 
		int    fast_columns; // corey@cac optimization 
		curve_points * df_current_plot; // used to process histogram labels + key entries 
		FILE * data_fp;
		//
		// Information about binary data structure, to be determined by the
		// using and format options.  This should be one greater than df_no_bin_cols.
		//
		df_column_bininfo_struct * df_column_bininfo; // allocate space as needed 
		int    df_max_bininfo_cols; // space allocated 
		int    df_plot_mode;
		int    df_M_count;
		int    df_N_count;
		int    df_O_count;
		FileType BinFileType; // Initially set to default and then possibly altered by command line. 
		// Default setting
		FileType BinFileTypeDefault; 
		FileType BinFileTypeReset; // Setting that is transferred to default upon reset. 
		df_endianess_type df_bin_file_endianess_default;
		df_endianess_type BinFileEndianess; // This one is needed by breaders.c 
		int    ColumnForKeyTitle;
	#if defined(PIPES)
		bool   df_pipe_open;
	#endif
	#if defined(HAVE_FDOPEN)
		int    data_fd; // only used for file redirection
	#endif
		int    df_no_use_specs;  // how many using columns were specified in the current command 
		int    df_line_number;
		int    df_datum;         // suggested x value if none given 
		int    df_last_col;      // number of columns in first row of data return to user in STATS_columns (visible to user via STATS_columns)
		int    df_bad_matrix_values; // number of matrix elements entered as missing or NaN 
		AXIS_INDEX df_axis[MAXDATACOLS];
		char * df_stringexpression[MAXDATACOLS]; // filled in after evaluate_at() 
		char * df_tokens[MAXDATACOLS];   // filled in by df_tokenise 
		use_spec_s UseSpec[MAXDATACOLS];
		GpValue df_strings[MAXDATACOLS]; // used only by TABLESTYLE 
		void * df_pixeldata;     // pixel data from an external library (e.g. libgd) 
		char * missing_val;      // string representing missing values in ascii datafiles 
		char * df_separators;    // input field separators, NULL if whitespace is the separator 
		char * df_commentschars; // comments chars 
		// 
		// Binary *read* variables used by df_readbinary().
		// There is a confusing difference between the ascii and binary "matrix" keywords.
		// Ascii matrix data by default is interpreted as having an implicit uniform grid
		// of x and y coords that are not actually present in the data file.
		// The equivalent binary data format is called "binary general".
		// In both of these cases the internal flag df_nonuniform_matrix is FALSE;
		// Binary matrix data contains explicit y values in the first row, and explicit x
		// values in the first column. This is signalled by "binary matrix".
		// In this case the internal flag df_nonuniform_matrix is TRUE.
		// 
		// EAM May 2011 - Add a keyword "nonuniform matrix" to indicate ascii matrix data
		// in the same format as "binary matrix", i.e. with explicit x and y coordinates.
		// EAM Jul 2014 - Add keywords "columnheaders" and "rowheaders" to indicate ascii
		// matrix data in the uniform grid format containing labels in row 1 and column 1.
		// 
		bool   df_read_binary;
		bool   df_nonuniform_matrix;
		bool   df_matrix_columnheaders;
		bool   df_matrix_rowheaders;
		bool   df_datablock;
		bool   SetEvery;
		bool   IndexFound;
		bool   df_transpose;
		bool   mixed_data_fp; // inline data 
		bool   df_tabulate_strings; // used only by TABLESTYLE 
		bool   df_already_got_headers;
		bool   df_columnheaders; // First row of data is known to contain headers rather than data 
		bool   df_matrix; // is this a matrix splot? 
		bool   df_binary; // is this a binary file? 
		// Logical variables indicating information about data file. 
		bool   df_binary_file;
		bool   df_matrix_file;
		bool   df_voxelgrid; // was df_open called on something that turned out to be a voxel grid? 
		bool   plotted_data_from_stdin; // flag if any 'inline' data are in use, for the current plot 
		bool   df_fortran_constants; // Setting this allows the parser to recognize Fortran D or Q format constants in the input file. But it slows things down 
		// Setting this disables initialization of the floating point exception 
		// handler before every expression evaluation in a using specifier.      
		// This can speed data input significantly, but assumes valid input.    
		bool   df_nofpe_trap;
		bool   evaluate_inside_using;
		bool   df_warn_on_missing_columnheader;
		//
		int    df_max_num_bin_records;
		int    NumBinRecords;
		int    df_bin_record_count;
		int    df_max_num_bin_records_default;
		int    df_num_bin_records_default;
		int    df_no_bin_cols; // binary columns to read 
		df_binary_file_record_struct * df_bin_record; // Initially set to default and then possibly altered by command line. 
		df_binary_file_record_struct * df_bin_record_default; // Default settings. 
		char * df_filename;         // name of data file 
		char * df_key_title;        // filled in from column header if requested 
		at_type * df_plot_title_at; // used for deferred evaluation of plot title 
	};
	struct GpPlot {
		GpPlot() : interactive(true), noinputfiles(true), reading_from_dash(false),
			skip_gnuplotrc(false), persist_cl(false), slow_font_startup(false), successful_initialization(false), ctrlc_flag(false),
			terminate_flag(false), user_homedir(0), user_shell(0), exit_status(EXIT_SUCCESS),
			P_FirstPlot(0), histogram_rightmost(0.0), stack_count(0), stackheight(0), paxis_start(-1), paxis_end(-1), paxis_current(-1), 
			Plot2D_NComplexValues(0), 
			mapping3d(MAP3D_CARTESIAN), dgrid3d_row_fineness(10), dgrid3d_col_fineness(10), dgrid3d_norm_value(1), dgrid3d_mode(DGRID3D_QNORM),
			dgrid3d_x_scale(1.0), dgrid3d_y_scale(1.0), dgrid3d(false), dgrid3d_kdensity(false), boxdepth(0.0), first_3dplot(0),
			plot3d_num(0), last_iteration_in_first_pass(INT_MAX), Plot3D_NComplexValues(0),
			hiddenBacksideLinetypeOffset(BACKSIDE_LINETYPE_OFFSET),
			hiddenTriangleLinesdrawnPattern(TRIANGLE_LINESDRAWN_PATTERN),
			hiddenHandleUndefinedPoints(HANDLE_UNDEFINED_POINTS),
			hiddenShowAlternativeDiagonal(SHOW_ALTERNATIVE_DIAGONAL),
			hiddenHandleBentoverQuadrangles(HANDLE_BENTOVER_QUADRANGLES)
		{
			memzero(command_line_env, sizeof(command_line_env));
			MEMSZERO(Plot2D_Func);
			MEMSZERO(Plot3D_Func);
		}
		bool   interactive; // FALSE if stdin not a terminal 
		bool   noinputfiles; // FALSE if there are script files 
		bool   reading_from_dash; // True if processing "-" as an input file 
		bool   skip_gnuplotrc; // skip system gnuplotrc and ~/.gnuplot 
		bool   persist_cl; // --persist command line option 
		bool   slow_font_startup; // --slow command line option 
		bool   successful_initialization; // not static because unset.c refers to it when in debugging mode 
		bool   ctrlc_flag; // Flag for asynchronous handling of Ctrl-C. Used by fit.c and Windows 
		bool   terminate_flag; // Flag for (asynchronous) term signal on Windows. 
		const  char * user_homedir; // user home directory 
		const  char * user_shell; // user shell 
		// patch to get home dir, see command.c 
		JMP_BUF command_line_env; // a longjmp buffer to get back to the command line 
		int    exit_status;
		//
		curve_points * P_FirstPlot; // the curves/surfaces of the plot 
		udft_entry Plot2D_Func;
		double histogram_rightmost; // Highest x-coord of histogram so far 
		text_label histogram_title;       // Subtitle for this histogram 
		GpCoordinate * stackheight; // Scratch space for y autoscale 
		int    stack_count; // counter for stackheight 
		int    paxis_start;   // PARALLELPLOT bookkeeping 
		int    paxis_end;     // PARALLELPLOT bookkeeping 
		int    paxis_current; // PARALLELPLOT bookkeeping 
		// If the user tries to plot a complex-valued function without reducing it to
		// some derived value like abs(f(z)), it generates a non-obvious error message
		// "all points y value undefined". Try to detect this case by counting complex
		// values as they are encountered so that a better error message is possible.
		int    Plot2D_NComplexValues;
		//
		t_data_mapping mapping3d;
		int    dgrid3d_row_fineness;
		int    dgrid3d_col_fineness;
		int    dgrid3d_norm_value;
		int    dgrid3d_mode;
		double dgrid3d_x_scale;
		double dgrid3d_y_scale;
		bool   dgrid3d;
		bool   dgrid3d_kdensity;
		double boxdepth;
		GpSurfacePoints * first_3dplot; // the curves/surfaces of the plot 
		udft_entry Plot3D_Func;
		int    plot3d_num;
		// FIXME:
		// Because this is global, it gets clobbered if there is more than
		// one unbounded iteration in the splot command, e.g. splot for [i=0:*] A index i, for [j=0:*] B index j
		// Moving it into (GpSurfacePoints) would be nice but would require
		// extra bookkeeping to track which plot header it is stored in.
		int    last_iteration_in_first_pass;
		// It is a common mistake to try to plot a complex-valued function
		// without reducing it to some derived real value like abs(f(z)).
		// When this happens the user gets a not entirely obvious error message "All point z value undefined".  
		// Try to distinguish this by counting complex values as we go.
		int    Plot3D_NComplexValues;
		//
		// The actual configuration is stored in these variables, modifiable at runtime through 'set hidden3d' options 
		int    hiddenBacksideLinetypeOffset = BACKSIDE_LINETYPE_OFFSET;
		long   hiddenTriangleLinesdrawnPattern = TRIANGLE_LINESDRAWN_PATTERN;
		int    hiddenHandleUndefinedPoints = HANDLE_UNDEFINED_POINTS;
		int    hiddenShowAlternativeDiagonal = SHOW_ALTERNATIVE_DIAGONAL;
		int    hiddenHandleBentoverQuadrangles = HANDLE_BENTOVER_QUADRANGLES;

		dynarray HiddenVertices;
		dynarray HiddenEdges;
		dynarray HiddenPolygons;
	};
	struct GpVPlot {
		GpVPlot()
		{
			THISZERO();
		}
		int    ScaledOffset[8][3]; // local copy of vertex offsets from voxel corner, scaled by downsampling 
		double Intersection[12][3]; // the fractional index intersection along each of the cubes's 12 edges 
		t_voxel CornerValue[8]; // working copy of the corner values for the current cube 
	};
	GnuPlot();
	~GnuPlot();
	int    ImplementMain(int argc_orig, char ** argv);
	void   Call(const double * par, double * data);
	void   PlotRequest(GpTermEntry * pTerm);
	static void DoArrow(GpTermEntry_Static * pThis, uint usx, uint usy/* start point */, uint uex, uint uey/* end point (point of arrowhead) */, int headstyle);
	static void DoPoint(GpTermEntry_Static * pThis, uint x, uint y, int number);
	static void LineAndPoint(GpTermEntry_Static * pTerm, uint x, uint y, int number);
	static void DoPointSize(GpTermEntry_Static * pThis, double size);
	static void NullSetColor(GpTermEntry_Static * pTerm, const t_colorspec * pColorSpec);
	static int  NullJustifyText(GpTermEntry_Static * pThis, enum JUSTIFY just);
	static int  NullTextAngle(GpTermEntry_Static * pThis, int ang);
	static void OptionsNull(GpTermEntry_Static * pThis, GnuPlot * pGp);
	static char * _StyleFont(const char * pFontName, bool isBold, bool isItalic);
	//
	// Deprecated terminal function (pre-version 3)
	//
	static int NullScale(GpTermEntry_Static * pThis, double x, double y);
	const  char * AnsiColorString(const t_colorspec * pColor, const t_colorspec * pPrevColor);
	// 
	// A macro to check whether 2D functionality is allowed in the last plot:
	// either the plot is a 2D plot, or it is a suitably oriented 3D plot (e.g. map).
	// 
	bool   IsAlmost2D() const
	{
		return (!Gg.Is3DPlot || _3DBlk.splot_map || (fabs(fmod(_3DBlk.SurfaceRotZ, 90.0)) < 0.1 && fabs(fmod(_3DBlk.SurfaceRotX, 180.0)) < 0.1));
	}

	char * ValueToStr(const GpValue * pVal, bool needQuotes);
	void   DispValue(FILE * fp, const GpValue * pVal, bool needQuotes);
	void   InitSession();
	void   InitTerminal();
	void   InitMonochrome();
	void   InitGadgets();
	void   InitFit();
	void   CommonErrorExit();
	void   EvalPlots(GpTermEntry * pTerm);
	void   Eval3DPlots(GpTermEntry * pTerm);
	int    ComLine();
	int    DoLine();
	void   DoString(const char * s);
	void   DoPlot(GpTermEntry * pTerm, curve_points * plots, int pcount);
	void   Do3DPlot(GpTermEntry * pTerm, GpSurfacePoints * plots, int pcount/* count of plots in linked list */, REPLOT_TYPE replot_mode/* replot/refresh/axes-only/quick-refresh */);
	void   DoSave3DPlot(GpTermEntry * pTerm, GpSurfacePoints * pPlots, int pcount, REPLOT_TYPE quick);
	void   DoStringAndFree(char * cmdline);
	void   DoZoom(double xmin, double ymin, double x2min, double y2min, double xmax, double ymax, double x2max, double y2max);
	void   ZoomRescale_XYX2Y2(double a0, double a1, double a2, double a3, double a4, double a5, double a6,
		double a7, double a8, double a9, double a10, double a11, double a12, double a13, double a14, double a15, const char * pMsg);
	void   ZoomInX(int zoomKey);
	void   ZoomAroundMouse(int zoom_key);
	//void   BindAppend(char * lhs, char * rhs, char * (*builtin)(GpEvent * ge));
	void   BindAppend(const char * lhs, char * rhs, BuiltinEventHandler handlerFunc);
	void   BindInstallDefaultBindings();
	void   DoEvent(GpTermEntry * pTerm, GpEvent * pGe);
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
	void   MultiplotStart(GpTermEntry * pTerm);
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
	void   OsError(int t_num, const char * str, ...);
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
	void   RefreshCommand(GpTermEntry * pTerm);
	void   VFillCommand();
	void   HistoryCommand();
	void   PauseCommand(GpTermEntry * pTerm);
	void   PlotCommand(GpTermEntry * pTerm);
	void   SPlotCommand(GpTermEntry * pTerm);
	void   ReplotCommand(GpTermEntry * pTerm);
	void   ClearCommand(GpTermEntry * pTerm);
	void   ToggleCommand(GpTermEntry * pTerm);
	void   SaveCommand();
	void   ChangeDirCommand(GpTermEntry * pTerm);
	void   CallCommand();
	void   TestCommand();
	void   UndefineCommand();
	void   BindCommand();
	void   VClearCommand();
	void   LoadCommand();
	void   InvalidCommand();
	void   OldIfCommand(at_type * expr);
	void   RaiseCommand();
	void   LowerCommand();
	void   RaiseLowerCommand(int lower);
	void   EvalCommand();
	void   DatablockCommand();
	void   PrintErrCommand();
	void   SystemCommand();
	void   StatsCommand();
	void   HelpCommand();
	void   RereadCommand();
	void   PwdCommand();
	void   UpdateCommand();
	void   ScreenDumpCommand(GpTermEntry * pTerm);
	void   PrintLineWithError(int t_num);
	void   PlotOptionEvery();
	void   PlotOptionUsing(int max_using);
	void   PlotOptionIndex();
	void   PlotOptionBinary(bool setMatrix, bool setDefault);
	void   Plot3DRequest(GpTermEntry * pTerm);
	void   RefreshRequest(GpTermEntry * pTerm);
	void   StatsRequest();
	int    LpParse(GpTermEntry * pTerm, lp_style_type * lp, lp_class destination_class, bool allow_point);
	void   ArrowParse(arrow_style_type * arrow, bool allow_as);
	int    MakePalette(GpTermEntry * pTerm);
	double AxisLogValueChecked(AXIS_INDEX axis, double coord, const char * pWhat);
	void   AxisCheckedExtendEmptyRange(AXIS_INDEX axis, const char * mesg);
	void   CloneLinkedAxes(GpAxis * pAx1, GpAxis * pAx2);
	void   SetRgbColorVar(GpTermEntry * pTerm, uint rgbvalue);
	void   SetRgbColorConst(GpTermEntry * pTerm, uint rgbvalue);
	void   MapPositionR(const GpTermEntry * pTerm, GpPosition * pos, double * x, double * y, const char * what);
	void   MapPositionDouble(const GpTermEntry * pTerm, GpPosition * pos, double * x, double * y, const char * what);
	void   MapPosition(const GpTermEntry * pTerm, GpPosition * pos, int * x, int * y, const char * what);
	void   Map3D_XYZ(double x, double y, double z/* user coordinates */, GpVertex * pOut);
	void   Map3D_XYZ(const SPoint2R & rPt, double z/* user coordinates */, GpVertex * pOut);
	void   Map3D_XYZ(const SPoint3R & rPt/* user coordinates */, GpVertex * pOut);
	void   Map3D_XY_double(double x, double y, double z, double * xt, double * yt);
	void   Map3D_XY_double(const SPoint2R & rPt, double z, double * xt, double * yt);
	void   Map3D_XY_double(const SPoint3R & rPt, double * xt, double * yt);
	void   Map3D_XY(double x, double y, double z, int * xt, int * yt);
	void   Map3D_XY(const SPoint2R & rPt, double z, int * xt, int * yt);
	void   Map3D_XY(const SPoint3R & rPt, int * xt, int * yt);
	SPoint2I Map3D_XY(double x, double y, double z);
	int    Map3DGetPosition(const GpTermEntry * pTerm, GpPosition * pos, const char * what, double * xpos, double * ypos, double * zpos);
	void   Map3DPositionRDouble(const GpTermEntry * pTerm, GpPosition * pos, double * xx, double * yy, const char * what);
	void   Map3DPositionDouble(const GpTermEntry * pTerm, GpPosition * pos, double * x, double * y, const char * what);
	void   Map3DPosition(const GpTermEntry * pTerm, GpPosition * pos, int * x, int * y, const char * what);
	void   Map3DPositionR(const GpTermEntry * pTerm, GpPosition * pPos, int * x, int * y, const char * what);
	void   ClipMove(int x, int y);
	void   ClipVector(GpTermEntry * pTerm, int x, int y);
	coord_type PolarToXY(double theta, double r, double * x, double * y, bool update);
	double PolarRadius(double r);
	double Cb2Gray(double cb);
	double Map2Gray(double z, const udvt_entry * pColorMap);
	void   Rgb1FromGray(double gray, rgb_color * pColor);
	double QuantizeGray(double gray);
	void   Rgb1MaxColorsFromGray(double gray, rgb_color * pColor);
	void   Rgb255MaxColorsFromGray(double gray, rgb255_color * rgb255);
	int    CalculateColorFromFormulae(double gray, rgb_color * pColor);
	int    DrawClipLine(GpTermEntry * pTerm, int x1, int y1, int x2, int y2);
	void   DrawClipArrow(GpTermEntry * pTerm, double dsx, double dsy, double dex, double dey, t_arrow_head head);
	void   Draw3DPointUnconditional(GpTermEntry * pTerm, const GpVertex * pV, const lp_style_type * pLp);
	void   Draw3DLineUnconditional(GpTermEntry * pTerm, const GpVertex * pV1, const GpVertex * pV2, const lp_style_type * lp, t_colorspec color);
	void   Draw3DLine(GpTermEntry * pTerm, GpVertex * v1, GpVertex * v2, lp_style_type * lp);
	void   DrawPolarCircle(GpTermEntry * pTerm, double place);
	void   GetPositionDefault(GpPosition * pos, enum position_type default_type, int ndim);
	void   GetPosition(GpPosition * pos);
	void   Store2DPoint(curve_points * pPlot, int i/* point number */, double x, double y, double xlow, double xhigh, double ylow, double yhigh, double width/* BOXES widths: -1 -> autocalc, 0 ->  use xlow/xhigh */);
	void   Edge3DIntersect(const GpCoordinate * p1, const GpCoordinate * p2, SPoint3R & rE/* the point where it crosses an edge */);
	bool   TwoEdge3DIntersect(const GpCoordinate * p0, const GpCoordinate * p1, double * lx, double * ly, double * lz/* lx[2], ly[2], lz[2]: points where it crosses edges */);
	void   ProcessImage(GpTermEntry * pTerm, const void * plot, t_procimg_action action);
	void   ApplyZoom(GpTermEntry * pTerm, t_zoom * z);
	void   ApplyPm3DColor(GpTermEntry * pTerm, const t_colorspec * tc);
	void   TermApplyLpProperties(GpTermEntry * pTerm, const lp_style_type * lp);
	void   WriteLabel(GpTermEntry * pTerm, int x, int y, text_label * pLabel);
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
	long   ParseColorName();
	GpValue * ConstStringExpress(GpValue * pVal);
	char * TryToGetString();
	const  char * StringOrExpress(at_type ** ppAt);
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
	void   TestTerminal(GpTermEntry * pTerm);
	char * DfGeneratePseudodata();
	int    IsBuiltinFunction(int t_num) const;
	int    IsFunction(int t_num) const;
	int    IsDefinition(int t_num) const;
	bool   IsPlotWithPalette() const;
	bool   MightBeNumeric(int t_num) const;
	void   FASTCALL _ExecuteAt2(at_type * pAt);
	void   UpdateGpvalVariables(GpTermEntry * pTerm, int context);
	GpIterator * CheckForIteration();
	void   PrepareCall(int calltype);
	int    DfOpen(const char * pCmdFileName, int maxUsing, curve_points * pPlot);
	double DfReadAFloat(FILE * fin);
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
	//void   MousePosToGraphPosReal(int xx, int yy, double * x, double * y, double * x2, double * y2);
	void   MousePosToGraphPosReal(SPoint2I pt, double * x, double * y, double * x2, double * y2);
	void   RecalcRulerPos();
	void   UpdateStatusLine();
	void   DoStringReplot(GpTermEntry * pTerm, const char * pS);
	void   IlluminateOneQuadrangle(Quadrangle * q);
	int    ReportError(int ierr);
	void   LfPush(FILE * fp, char * name, char * cmdline);
	bool   LfPop();
	void   PrintfValue(char * pOutString, size_t count, const char * pFormat, double log10_base, const GpValue * pV);
	void   GPrintf(char * pOutString, size_t count, const char * pFormat, double log10_base, double x);
	void   TermReset(GpTermEntry * pTerm);
	void   TermSetOutput(GpTermEntry * pTerm, char * pDest);
	int    DfReadLine(double v[], int maxSize);
	void   RemoveLabel(GpTermEntry * pTerm, int x, int y);
	gradient_struct * ApproximatePalette(t_sm_palette * pPalette, int samples, double allowedDeviation, int * pGradientNum);
	double MapX(double value);
	double MapY(double value);
	int    MapiX(double value);
	int    MapiY(double value);
	char * DfGets(FILE * fin);
	void   DfSetSkipBefore(int col, int bytes);
	void   DfSetReadType(int col, df_data_type type);
	void   DfExtendBinaryColumns(int no_cols);
	void   DfSetPlotMode(int mode);
	void   Givens(double ** ppC, double * pD, double * pX, int N, int n);
	void   ErrorEx(int t_num, const char * str, ...);
	char * ReadLine(const char * pPrompt);
	void   CheckForMouseEvents(GpTermEntry * pTerm);
	void   RestrictPOpen();
	int    EstimateStrlen(const char * pText, double * pHeight);
	void   FASTCALL Push(GpValue * x);
	GpValue * FASTCALL Pop(GpValue * x);
	double FASTCALL Real(const GpValue *);
	double FASTCALL Imag(const GpValue *);
	double FASTCALL Magnitude(const GpValue * pVal);
	double FASTCALL Angle(const GpValue * pVal);
	void   EnhErrCheck(const char * pStr);
	//
	//
	//
	struct GpFit {
		//
		// is_empty: check for valid string entries
		//
		static bool IsEmptyString(const char * s)
		{
			while(*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
				s++;
			return (bool)(*s == '#' || *s == '\0');
		}
		GpFit() :
			epsilon(DEF_FIT_LIMIT),
			startup_lambda(0.0),
			lambda_down_factor(LAMBDA_DOWN_FACTOR),
			lambda_up_factor(LAMBDA_UP_FACTOR),
			log_f(0),
			via_f(0),
			num_data(0),
			num_params(0),
			num_indep(0),
			num_errors(0),
			fit_show_lambda(true),
			user_stop(false),
			columns(0),
			fit_x(0),
			fit_z(0),
			err_data(0),
			a(0),
			regress_C(0),
			CleanUpProc(0),
			scale_params(0),
			par_name(0),
			par_udv(0),
			last_par_name(0),
			last_num_params(0),
			last_fit_command(0),
			fitlogfile(0),
			fit_verbosity(BRIEF),
			fit_script(0),
			fit_wrap(0),
			maxiter(0),
			epsilon_abs(0.0),
			fit_suppress_log(false),
			fit_errorvariables(true),
			fit_covarvariables(false),
			fit_errorscaling(true),
			fit_prescale(true),
			fit_v4compatible(false)
		{
			memzero(err_cols, sizeof(err_cols));
			memzero(last_dummy_var, sizeof(last_dummy_var));
			memzero(fit_dummy_udvs, sizeof(fit_dummy_udvs));
			memzero(&func, sizeof(func));
		}
		// 
		// Small function to write the last fit command into a file
		// Arg: Pointer to the file; if NULL, nothing is written,
		//   but only the size of the string is returned.
		// 
		size_t WriToFilLastFitCmd(FILE * fp)
		{
			if(!last_fit_command)
				return 0;
			else if(!fp)
				return strlen(last_fit_command);
			else
				return (size_t)fputs(last_fit_command, fp);
		}
		void   Dblfn(const char * fmt, ...);
		void   ShowResults(double chisq, double last_chisq, double * pA, double * pDPar, double ** ppCorel);
		double epsilon;  // relative convergence limit 
		double startup_lambda;
		double lambda_down_factor;
		double lambda_up_factor;
		FILE * log_f;
		FILE * via_f;
		int    num_data;
		int    num_params;
		int    num_indep; // # independent variables in fit function 
		int    num_errors; // #error columns 
		bool   fit_show_lambda;
		bool   user_stop;
		bool   err_cols[MAX_NUM_VAR+1]; // TRUE if variable has an associated error 
		int    columns; // # values read from data file for each point 
		double * fit_x; // all independent variable values, e.g. value of the ith variable from the jth data point is in fit_x[j*num_indep+i] 
		double * fit_z; // dependent data values 
		double * err_data; // standard deviations of indep. and dependent data
		double * a; // array of fitting parameters 
		double ** regress_C; // global copy of C matrix in regress 
		//void (* regress_cleanup)(); // memory cleanup function callback 
		RegressCleanUpProc CleanUpProc; // memory cleanup function callback 
		double * scale_params; // scaling values for parameters 
		udft_entry func;
		fixstr * par_name;
		GpValue ** par_udv; // array of pointers to the "via" variables 
		fixstr * last_par_name;
		int    last_num_params;
		char * last_dummy_var[MAX_NUM_VAR];
		char * last_fit_command;
		// Mar 2014 - the single hottest call path in fit was looking up the
		// dummy parameters by name (4 billion times in fit.dem).
		// A total waste, since they don't change.  Look up once and store here.
		udvt_entry * fit_dummy_udvs[MAX_NUM_VAR];
		//
		char * fitlogfile;
		verbosity_level fit_verbosity;
		char * fit_script;
		int    fit_wrap;
		int    maxiter;
		double epsilon_abs; // // absolute convergence criterion. default to zero non-relative limit 
		bool   fit_suppress_log;
		bool   fit_errorvariables;
		bool   fit_covarvariables;
		bool   fit_errorscaling;
		bool   fit_prescale;
		bool   fit_v4compatible;
	};
	//
	//
	//
	struct bind_t {
		bind_t * prev;
		int    key;
		char   modifier;
		char * command;
		//char *(*builtin)(GpEvent * ge);
		BuiltinEventHandler HandlerFunc;
		bool   allwindows;
		bind_t * next;
	};
	struct GpMouse {
		GpMouse() : Pos(-1, -1), Button(0), ModifierMask(0), SettingZoomRegion(false), NeedReplot(false), TrapRelease(false), P_Bindings(0),
			Motion(0), ZeroRotX(0.0f), ZeroRotZ(0.0f)
		{
			RealPos.Set(0.0);
			RealPos2.Set(0.0);
		}
		SPoint2I Pos;       // the coordinates of the mouse cursor in gnuplot's internal coordinate system
		SPoint2R RealPos;   // the "real" coordinates of the mouse cursor, i.e., in the user's coordinate system(s)
		SPoint2R RealPos2;
		SPoint2I SettingZoom; // coordinates of the first corner of the zoom region, in the internal coordinate system 
		SPoint2I Start;       // mouse position when dragging started 
		int    Button; // status of buttons; button i corresponds to bit (1<<i) of this variable
		int    ModifierMask; // the status of the shift, ctrl and alt keys
		bool   SettingZoomRegion; // flag, TRUE while user is outlining the zoom region 
		bool   NeedReplot; // did we already postpone a replot because allowmotion was FALSE ? 
		bool   TrapRelease;
		int    Motion; // ButtonPress sets this to 0, ButtonMotion to 1 
		float  ZeroRotX; // values for rot_x and rot_z corresponding to zero position of mouse 
		float  ZeroRotZ; 

		GnuPlot::bind_t * P_Bindings;
		struct RulerBlock {
			RulerBlock() : on(false)
			{
				RealPos.Set(0.0);
				RealPos2.Set(0.0);
			}
			SPoint2R RealPos; // ruler position in real units of the graph 
			SPoint2R RealPos2;
			SPoint2I Pos; // ruler position in the viewport units 
			bool on;
		} Ruler; // = { false, 0.0, 0.0, 0.0, 0.0, 0, 0 };
	};
	//
	// Contours are saved using this struct list. 
	//
	struct ContourNode : public SPoint2R {
		//double X, Y; // The coordinates of this vertex. 
		ContourNode * next; // To chain lists. 
	};
	struct PolyNode;

	struct EdgeNode {
		//
		// position of edge in mesh 
		//
		enum t_edge_position {
			INNER_MESH = 1,
			BOUNDARY,
			DIAGONAL
		};
		PolyNode * poly[2];    // Each edge belongs to up to 2 polygons 
		GpCoordinate * vertex[2]; // The two extreme points of this edge. 
		EdgeNode * next; // To chain lists 
		bool is_active;     // is edge is 'active' at certain Z level? 
		t_edge_position position; // position of edge in mesh 
	};

	struct PolyNode {
		EdgeNode * edge[3]; // As we do triangulation here... 
		PolyNode * next;    // To chain lists. 
	};
	struct GpUtil {
		GpUtil() : screen_ok(false), decimalsign(0), numeric_locale(0), time_locale(0), micro(0), minus_sign(0), use_micro(false), use_minus_sign(false),
			current_prompt(0), debug(0)
		{
			STRNSCPY(degree_sign, "°");
		}
		// 
		// TRUE if command just typed; becomes FALSE whenever we
		// send some other output to screen.  If FALSE, the command line
		// will be echoed to the screen before the ^ error message.
		// 
		bool   screen_ok;
		char * decimalsign; // decimal sign 
		char * numeric_locale; // LC_NUMERIC 
		char * time_locale; // LC_TIME 
		char   degree_sign[8]; // // degree sign.  Defaults to UTF-8 but will be changed to match encoding encoding-specific characters used by gprintf() 
		const  char * micro;
		const  char * minus_sign;
		bool   use_micro;
		bool   use_minus_sign;
		const  char * current_prompt; // needed by is_error() and friends 
		int    debug;
	};

	void   AddCntrPoint(double x, double y);
	void   EndCrntCntr();
	int    FuzzyEqual(const ContourNode * pCntr1, const ContourNode * pCntr2) const;
	int    GenCubicSpline(int num_pts/* Number of points (num_pts>=3), input */, ContourNode * p_cntr/* List of points (x(t_i),y(t_i)), input */,
		double d2x[], double d2y[]/* Second derivatives (x''(t_i),y''(t_i)), output */,
		double delta_t[]/* List of interval lengths t_{i+1}-t_{i}, output */,
		bool contr_isclosed/* Closed or open contour?, input  */, double unit_x, double unit_y/* Unit length in x and y (norm=1), input */);
	void   PutContourCubic(ContourNode * p_cntr, double xx_min, double xx_max, double yy_min, double yy_max, bool contr_isclosed);
	void   PutContour(ContourNode * p_cntr/* contour structure input */, double xx_min, double xx_max, double yy_min, double yy_max/* minimum/maximum values input */,
		bool contr_isclosed/* contour line closed? (input) */);
	void   GenContours(EdgeNode * p_edges, double z_level, double xx_min, double xx_max, double yy_min, double yy_max);
	void   GenTriangle(int num_isolines/* number of iso-lines input */, iso_curve * iso_lines/* iso-lines input */, PolyNode ** p_polys/* list of polygons output */,
		EdgeNode ** p_edges/* list of edges output */);
	void   GenBSplineApprox(ContourNode * p_cntr, int num_of_points, int order, bool contr_isclosed);
	int    UpdateAllEdges(EdgeNode * p_edges, double z_level);
	ContourNode * GenOneContour(EdgeNode * p_edges/* list of edges input */,
		double z_level/* Z level of contour input */, bool * contr_isclosed/* open or closed contour, in/out */, int * num_active/* number of active edges in/out */);
	ContourNode * TraceContour(EdgeNode * pe_start/* edge to start contour input */,
		double z_level/* Z level of contour input */, int * num_active/* number of active edges in/out */, bool contr_isclosed/* open or closed contour line (input) */);
	ContourNode * UpdateCntrPt(EdgeNode * p_edge, double z_level);
	void   FreeContour(ContourNode * pCntr);
	void   PutContourNothing(ContourNode * pCntr);
	void   PutContourBSpline(ContourNode * p_cntr, bool contr_isclosed);
	int    ChkContourKind(ContourNode * p_cntr, bool contr_isclosed);
	PolyNode * AddPoly(EdgeNode * edge0, EdgeNode * edge1, EdgeNode * edge2/* 3 edges input */, PolyNode ** p_poly,
		PolyNode ** pp_tail/* pointers to polygon list in/out */);
	EdgeNode * AddEdge(GpCoordinate * point0,  /* 2 vertices input */ GpCoordinate * point1, EdgeNode ** p_edge/* pointers to edge list in/out */, EdgeNode ** pe_tail);
	void   EvalBSpline(double t, ContourNode * p_cntr, int num_of_points, int order, int j, bool contr_isclosed, double * x, double * y);
	void   IntpCubicSpline(int n, ContourNode * p_cntr, double d2x[], double d2y[], double delta_t[], int n_intpol);
	int    CountContour(const ContourNode * pCntr) const;
	void   BmpCharSize(uint size);
	void   BmpMakeBitmap(uint x, uint y, uint planes);
	void   BmpFreeBitmap();
	marq_res_t Marquardt(double a[], double ** C, double * chisq, double * lambda);
	void   InternalRegressCleanUp();
	void   GetUserEnv();
	void   GpExpandTilde(char ** ppPath);
	FILE * LoadPath_fopen(const char *, const char *);
	bool   FASTCALL IsStringValue(int tokN) const { return (Pgm.IsString(tokN) || TypeUdv(tokN) == STRING); }
	void   BailToCommandLine();
	const char * LatexInputEncoding(enum set_encoding_id encoding);

	class GpHistory {
	public:
		GpHistory() : HistorySize(HISTORY_SIZE), HistoryLength(0), HistoryBase(1), HistoryQuiet(false), HistoryFull(false),
			P_History(0), P_CurEntry(0)
		{
		}
		int  HistorySize;
		int  HistoryLength;
		int  HistoryBase;
		bool HistoryQuiet;
		bool HistoryFull;
		HIST_ENTRY * P_History; // last entry in the history list, no history yet 
		HIST_ENTRY * P_CurEntry;
	};

	void   AddHistory(const char * pLine);
	void   ClearHistory();
	int    WhereHistory();
	int    HistoryFindAll(char * pCmd);
	int    HistorySetPos(int offset);
	HIST_ENTRY * HistoryGet(int offset);
	const char * HistoryFindByNumber(int);
	int    HistorySearch(const char * pString, int direction);
	HIST_ENTRY * CurrentHistory();
	HIST_ENTRY * PreviousHistory();
	HIST_ENTRY * NextHistory();
	HIST_ENTRY * RemoveHistory(int which);
	void   WriteHistoryList(const int num, const char * const filename, const char * mode);
	void   WriteHistoryN(const int, const char *, const char *);
	int    WriteHistory(const char * pFileName);
	void   ReadHistory(const char * pFileName);
	int    Implement_ReadHistory(const char * pFileName);
	int    HistorySearchPrefix(const char * string, int direction);
	const char * HistoryFind(char * pCmd);
	HIST_ENTRY * ReplaceHistoryEntry(int which, const char * line, histdata_t data);

	enum jitterstyle {
		JITTER_DEFAULT = 0,
		JITTER_SWARM,
		JITTER_SQUARE,
		JITTER_ON_Y
	};

	struct t_jitter {
		t_jitter() : spread(0.0), limit(0.0), style(JITTER_DEFAULT)
		{
			overlap.Set(first_axes, first_axes, first_axes, 0.0, 0.0, 0.0);
		}
		GpPosition overlap;
		double spread;
		double limit;
		enum jitterstyle style;
	};
	struct GpVoxelGrid {
		GpVoxelGrid() : P_CurrentVGrid(0), P_UdvVoxelDistance(0), P_UdvGridDistance(0), P_DensityFunction(0), NVoxelsModified(0)
		{
		}
		void   FreeGrid(udvt_entry * pGrid);
		t_voxel Voxel(double vx, double vy, double vz) const;
		VGrid * P_CurrentVGrid; // active voxel grid 
		udvt_entry * P_UdvVoxelDistance; // reserved user variable 
		udvt_entry * P_UdvGridDistance;  // reserved user variable 
		int    NVoxelsModified;
		at_type * P_DensityFunction;
		isosurface_opt IsoSurfaceOptions;
	};
	struct GpBitmap {
		GpBitmap() : b_p(0), b_xsize(0), b_ysize(0), b_planes(0), b_psize(0), b_rastermode(0), b_linemask(0xffff), b_angle(0), b_maskcount(0),
			b_value(1), b_lw(1.0), b_currx(0), b_curry(0), b_hchar(0), b_hbits(0), b_vchar(0), b_vbits(0), b_lastx(0), b_lasty(0)
		{
			memzero(b_font, sizeof(b_font));
		}
		void   SetMaskPixel(uint x, uint y, uint value);
		uint   GetPixel(uint x, uint y);
		void   SetPixel(uint x, uint y, uint value);
		void   Line(uint x1, uint y1, uint x2, uint y2);
		void   WLine(uint x1, uint y1, uint x2, uint y2);
		void   Putc(uint x, uint y, int c, uint c_angle);
		bitmap * b_p;        // global pointer to bitmap 
		uint   b_xsize;      // the size of the bitmap 
		uint   b_ysize;
		uint   b_planes;     // number of color planes 
		uint   b_psize;      // size of each plane 
		uint   b_rastermode; // raster mode rotates -90deg 
		uint   b_linemask;   // 16 bit mask for dotted lines 
		uint   b_angle;      // rotation of text 
		int    b_maskcount;
		//
		uint   b_value = 1; // colour of lines */
		double b_lw = 1.0;  // line width 
		uint   b_currx; // the current coordinates 
		uint   b_curry;
		uint   b_hchar; // width of characters 
		uint   b_hbits; // actual bits in char horizontally 
		uint   b_vchar; // height of characters 
		uint   b_vbits; // actual bits in char vertically 
		char_box b_font[FNT_CHARS]; // the current font 
		uint   b_lastx;
		uint   b_lasty; // last pixel set - used by b_line 
	};
	struct GpPm3DBlock {
		GpPm3DBlock() : allocated_quadrangles(0), current_quadrangle(0), P_Quadrangles(0), P_PolygonList(0),
			next_polygon(0), current_polygon(0), polygonlistsize(0), pm3d_plot_at(0), color_from_rgbvar(false), plot_has_palette(false),
			track_pm3d_quadrangles(false)
		{
			MEMSZERO(light);
		}
		void   InitLightingModel();
		gpdPoint * GetPolygon(int size);
		void   FreePolygonList();
		void   ResetAfterError()
		{
			pm3d_plot_at = 0;
			FreePolygonList();
		}
		int    allocated_quadrangles;
		int    current_quadrangle;
		Quadrangle * P_Quadrangles;
		gpdPoint   * P_PolygonList; // holds polygons with >4 vertices 
		int    next_polygon;    // index of next slot in the list 
		int    current_polygon; // index of the current polygon 
		int    polygonlistsize;
		int    pm3d_plot_at; // flag so that top/base polygons are not clipped against z 
		bool   color_from_rgbvar;
		bool   plot_has_palette;
		bool   track_pm3d_quadrangles; // Set by plot styles that use pm3d quadrangles even in non-pm3d mode 
		double light[3];
		pm3d_struct pm3d; // Initialized via init_session->reset_command->reset_pm3d 
		lighting_model pm3d_shade;
	};
	struct GpContour {
		GpContour() : InterpKind(CONTOUR_KIND_LINEAR), P_ContourList(0), CrntCntrPtIndex(0), ContourLevel(0.0),
			ContourKind(CONTOUR_KIND_LINEAR),
			ContourLevelsKind(LEVELS_AUTO),
			ContourLevels(DEFAULT_CONTOUR_LEVELS),
			ContourOrder(DEFAULT_CONTOUR_ORDER),
			ContourPts(DEFAULT_NUM_APPROX_PTS),
			ContourFirstLineType(-1),
			ContourSortLevels(false)
		{
			MEMSZERO(dyn_contour_levels_list);
			memzero(CrntCntr, sizeof(CrntCntr));
			STRNSCPY(ContourFormat, "%8.3g");
		}
		SPoint3R Min; // Minimum values of x, y, and z 
		SPoint3R Max; // Maximum values of x, y, and z 
		t_contour_kind InterpKind; // Linear, Cubic interp., Bspline
		gnuplot_contours * P_ContourList;
		double CrntCntr[MAX_POINTS_PER_CNTR * 2];
		int    CrntCntrPtIndex;
		double ContourLevel;
		//
		char   ContourFormat[32]; // format for contour key entries 
		t_contour_kind ContourKind;
		t_contour_levels_kind ContourLevelsKind;
		int    ContourLevels;
		int    ContourOrder;
		int    ContourPts;
		int    ContourFirstLineType;
		bool   ContourSortLevels;
		dynarray dyn_contour_levels_list; // storage for z levels to draw contours at 
	};

	#define QUADTREE_GRANULARITY 30

	class GpHidden3D {
	public:
		GpHidden3D() : PFirst(0), EFirst(0)
		{
			memzero(Quadtree, sizeof(Quadtree));
		}
		long Quadtree[QUADTREE_GRANULARITY][QUADTREE_GRANULARITY];
		long PFirst; // first polygon in zsorted chain
		long EFirst; // first edges in zsorted chain 
		dynarray QTree; // the dynarray to actually store all that stuff in
	};
	GpEval Ev;
	GpAxisSet AxS;
	GpProgram Pgm;
	t_sm_palette SmPltt;
	GpView V;
	GpGadgets Gg;
	GpHistory Hist;
private:
	bool   CheckTagGtZero(int * pTag, const char ** ppErrMessage);
	GpStack EvStk;
	//
	// Copy of palette previously in use.
	// Exported so that change_term() can invalidate contents
	//
	t_sm_palette PrevPltt; // = { -1, (palette_color_mode)-1, -1, -1, -1, -1, -1, -1, (rgb_color*)0, -1 };
	MpLayout_ MpLo;// = MP_LAYOUT_DEFAULT;
	t_jitter Jitter;
	GpTabulate Tab;
	GpVoxelGrid _VG;
	GpGraphics Gr;
	GpVPlot _VP;
	GpPm3DBlock _Pm3D;
	GpContour _Cntr;
	GpFit _Fit;
	GpUtil  GpU;
	GpReadLineBlock RlB_;
	GpParseBlock _Pb;
	GpDataFile _Df;
	GpHidden3D Hid3D;

	palette_color_mode Pm3dLastSetPaletteMode/*= SMPAL_COLOR_MODE_NONE*/;
	int    EnableResetPalette/*= 1*/; // is resetting palette enabled? note: reset_palette() is disabled within 'test palette'
	GpVertex Polyline3dPreviousVertex; // Previous points 3D position 
public:
	GpPlot  _Plt;
	GpGraph3DBlock _3DBlk;
	GpEnhancedText Enht;
	TermDumbBlock TDumbB;
	GpPostscriptBlock TPsB;
	GpBoundary _Bry;
	GpBitmap _Bmp;
	GpMouse _Mse;
	double TermPointSize;
	bool   TermInitialised; // true if terminal has been initialized 
	bool   TermGraphics;   //= false; // true if terminal is in graphics mode 
	bool   TermSuspended;  //= false; // we have suspended the driver, in multiplot mode 
	bool   TermOpenedBinary; //= false; // true if? 
	bool   TermForceInit;  //= false; // true if require terminal to be initialized 
	LFS  * P_LfHead;
private:
	curve_points * CpAlloc(int num);
	// 
	// Descr: releases any memory which was previously malloc()'d to hold curve points (and recursively down the linked list).
	// 
	void   CpFree(curve_points * cp);
	GpTermEntry * SetTerm();
	void   TermInitialise(GpTermEntry * pTerm);
	void   TermStartPlot(GpTermEntry * pTerm);
	void   TermEndPlot(GpTermEntry * pTerm);
	void   TermStartMultiplot(GpTermEntry * pTerm);
	void   TermEndMultiplot(GpTermEntry * pTerm);
	void   TermCheckMultiplotOkay(bool fInteractive);
	void   TermSuspend(GpTermEntry * pTerm);
	void   PushTerminal(int isInteractive);
	void   PopTerminal();
	GpTermEntry * ChangeTerm(const char * pOrigName, int length);
	void   TermCloseOutput(GpTermEntry * pTerm);
	void   ClosePrinter(GpTermEntry * pTerm, FILE * outfile);
	void   TestPaletteSubcommand();
	uint   RgbFromColorspec(t_colorspec * tc);
	void   ReplotRequest(GpTermEntry * pTerm);
	void   Boundary(GpTermEntry * pTerm, const curve_points * pPlots, int count);
	void   Boundary3D(GpTermEntry * pTerm, const GpSurfacePoints * plots, int count);
	void   InitHistogram(histogram_style * pHistogram, text_label * pTitle);
	void   DoKeyBounds(GpTermEntry * pTerm, legend_key * pKey);
	void   DoKeyLayout(GpTermEntry * pTerm, legend_key * pKey);
	void   Do3DKeyLayout(GpTermEntry * pTerm, legend_key * pKey, int * xinkey, int * yinkey);
	void   DoTimeLabel(GpTermEntry * pTerm, SPoint2I pt);
	void   SPlotMapActivate();
	void   SPlotMapDeactivate();
	void   KeyText(GpTermEntry * pTerm, int xl, int yl, const char * pText);
	void   BoxPlotRangeFiddling(GpTermEntry * pTerm, curve_points * pPlot);
	void   PlotBorder(GpTermEntry * pTerm);
	void   PlotSteps(GpTermEntry * pTerm, curve_points * plot);
	void   PlotFSteps(GpTermEntry * pTerm, curve_points * pPlot);
	void   PlotHiSteps(GpTermEntry * pTerm, curve_points * pPlot);
	void   PlotLines(GpTermEntry * pTerm, curve_points * pPlot);
	void   PlotFilledCurves(GpTermEntry * pTerm, curve_points * pPlot);
	void   PlotCBars(GpTermEntry * pTerm, curve_points * pPlot);
	void   PlotFBars(GpTermEntry * pTerm, curve_points * pPlot);
	void   PlotBars(GpTermEntry * pTerm, curve_points * plot);
	void   PlotBetweenCurves(GpTermEntry * pTerm, curve_points * plot);
	void   PlotBoxes(GpTermEntry * pTerm, curve_points * plot, int xaxis_y);
	void   PlotPoints(GpTermEntry * pTerm, curve_points * plot);
	void   PlotVectors(GpTermEntry * pTerm, curve_points * plot);
	void   PlaceGrid(GpTermEntry * pTerm, int layer);
	void   PlaceLabels(GpTermEntry * pTerm, text_label * pListHead, int layer, bool clip);
	void   PlaceRAxis(GpTermEntry * pTerm);
	void   PlaceParallelAxes(GpTermEntry * pTerm, const curve_points * pFirstPlot, int layer);
	void   PlaceArrows3D(GpTermEntry * pTerm, int layer);
	void   PlaceArrows(GpTermEntry * pTerm, int layer);
	void   PlaceHistogramTitles(GpTermEntry * pTerm);
	void   PlaceObjects(GpTermEntry * pTerm, GpObject * pListHead, int layer, int dimensions);
	void   PlaceLabels3D(GpTermEntry * pTerm, text_label * pListHead, int layer);
	void   Plot3DBoxErrorBars(GpTermEntry * pTerm, GpSurfacePoints * plot);
	void   PlotOptionMultiValued(df_multivalue_type type, int arg);
	void   PlotOptionArray();
	void   PlotTicLabelUsing(int axis);
	void   PlotDots(GpTermEntry * pTerm, const curve_points * pPlot);
	void   PlotBoxPlot(GpTermEntry * pTerm, curve_points * pPlot, bool onlyAutoscale);
	void   PlotSpiderPlot(GpTermEntry * pTerm, curve_points * pPlot);
	void   Plot3DVectors(GpTermEntry * pTerm, GpSurfacePoints * pPlot);
	void   Plot3DBoxes(GpTermEntry * pTerm, GpSurfacePoints * pPlot);
	void   PlacePixmaps(GpTermEntry * pTerm, int layer, int dimensions);
	void   PlaceTitle(GpTermEntry * pTerm, int titleX, int titleY);
	void   PlotEllipses(GpTermEntry * pTerm, curve_points * pPlot);
	void   PlotParallel(GpTermEntry * pTerm, curve_points * pPlot);
	void   PlotCircles(GpTermEntry * pTerm, curve_points * pPlot);
	void   VPlotPoints(GpTermEntry * pTerm, GpSurfacePoints * pPlot, double level);
	void   VPlotIsoSurface(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int downsample);
	void   TessellateOneCube(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int ix, int iy, int iz);
	void   Pm3DAddQuadrangle(GpTermEntry * pTerm, GpSurfacePoints * pPlot, gpdPoint corners[4]);
	void   Pm3DAddPolygon(GpTermEntry * pTerm, GpSurfacePoints * pPlot, gpdPoint corners[4], int vertices);
	void   DoArc(GpTermEntry * pTerm, int cx, int cy/* Center */, double radius, double arc_start, double arc_end/* Limits of arc in degrees */, int style, bool wedge);
	void   KeySampleFill(GpTermEntry * pTerm, int xl, int yl, GpSurfacePoints * pPlot);
	void   KeySamplePointPm3D(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int xl, int yl, int pointtype);
	void   KeySampleLinePm3D(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int xl, int yl);
	void   KeySampleLine(GpTermEntry * pTerm, int xl, int yl);
	void   KeySamplePoint(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int xl, int yl, int pointtype);
	void   DrawVertex(GpTermEntry * pTerm, GpVertex * pV);
	void   DrawEdge(GpTermEntry * pTerm, GpEdge * e, GpVertex * v1, GpVertex * v2);
	void   JitterPoints(const GpTermEntry * pTerm, curve_points * pPlot);
	void   PlotImpulses(GpTermEntry * pTerm, curve_points * pPlot, int yaxis_x, int xaxis_y);
	void   CpImplode(curve_points * pCp);
	void   FilledPolygon(GpTermEntry * pTerm, gpdPoint * corners, int fillstyle, int nv);
	void   Pm3DDepthQueueFlush(GpTermEntry * pTerm);
	void   Pm3DPlot(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int at_which_z);
	void   Pm3DDrawOne(GpTermEntry * pTerm, GpSurfacePoints * pPlot);
	int    Pm3DSide(const GpCoordinate * p0, const GpCoordinate * p1, const GpCoordinate * p2);
	void   Pm3DDepthQueueClear();
	void   Pm3DInitLightingModel();
	void   Plot3DZErrorFill(GpTermEntry * pTerm, GpSurfacePoints * pPlot);
	void   Plot3DPolygons(GpTermEntry * pTerm, GpSurfacePoints * pPlot);
	void   Plot3DLines(GpTermEntry * pTerm, GpSurfacePoints * pPlot);
	void   Plot3DImpulses(GpTermEntry * pTerm, GpSurfacePoints * pPlot);
	void   Plot3DHidden(GpTermEntry * pTerm, GpSurfacePoints * plots, int pcount);
	bool   CheckForVariableColor(GpTermEntry * pTerm, const curve_points * pPlot, const double * pColorValue);
	void   Check3DForVariableColor(GpTermEntry * pTerm, GpSurfacePoints * pPlot, const GpCoordinate * pPoint);
	void   Cntr3DLabels(GpTermEntry * pTerm, const gnuplot_contours * cntr, char * pLevelText, text_label * pLabel);
	void   Cntr3DImpulses(GpTermEntry * pTerm, const gnuplot_contours * cntr, lp_style_type * lp);
	void   Cntr3DPoints(GpTermEntry * pTerm, const gnuplot_contours * cntr, lp_style_type * lp);
	void   Cntr3DLines(GpTermEntry * pTerm, const gnuplot_contours * cntr, struct lp_style_type * lp);
	void   Polyline3DStart(GpTermEntry * pTerm, GpVertex * v1);
	void   Polyline3DNext(GpTermEntry * pTerm, GpVertex * v2, lp_style_type * lp);
	void   GetArrow(GpTermEntry * pTerm, arrow_def * pArrow, double * pSx, double * pSy, double * pEx, double * pEy);
	void   DrawKey(GpTermEntry * pTerm, legend_key * pKey, bool keyPass);
	void   DoSystem(const char * cmd);
	void   DoKeySample(GpTermEntry * pTerm, const curve_points * pPlot, legend_key * pKey, char * title, coordval var_color);
	void   DoKeySamplePoint(GpTermEntry * pTerm, curve_points * pPlot, legend_key * pKey);
	void   DoRectangle(GpTermEntry * pTerm, int dimensions, t_object * pObject, const fill_style_type * pFillStyle);
	void   DoPolygon(GpTermEntry * pTerm, int dimensions, t_object * pObject, int style, int facing);
	void   DoEllipse(GpTermEntry * pTerm, int dimensions, t_ellipse * pEllipse, int style, bool doOwnMapping);
	void   Do3DCubic(iso_curve * pCurve, enum PLOT_SMOOTH smoothOption);
	void   DoKDensity(curve_points * cp, int firstPoint/* where to start in plot->points */, int num_points/* to determine end in plot->points */, GpCoordinate * dest/* where to put the interpolated data */);
	void   DoBezier(curve_points * cp, double * bc/* Bezier coefficient array */, int firstPoint/* where to start in plot->points */,
		int numPoints/* to determine end in plot->points */, GpCoordinate * pDest/* where to put the interpolated data */);
	void   DoCubic(curve_points * pPlot/* still contains old plot->points */,
		GpSplineCoeff * sc/* generated by cp_tridiag */, GpSplineCoeff * sc2/* optional spline for yhigh */,
		int first_point/* where to start in plot->points */, int num_points/* to determine end in plot->points */,
		GpCoordinate * dest/* where to put the interpolated data */);
	void   GenInterp(curve_points * pPlot);
	void   Gen3DSplines(GpSurfacePoints * pPlot);
	void   Gen2DPathSplines(curve_points * pPlot);
	void   McsInterp(curve_points * pPlot);
	void   MakeBins(curve_points * pPlot, int nbins, double binlow, double binhigh, double binwidth);
	void   Draw3DPoint(GpTermEntry * pTerm, GpVertex * v, lp_style_type * lp);
	void   DrawLabelHidden(GpTermEntry * pTerm, GpVertex * v, lp_style_type * lp, int x, int y);
	void   DrawInsideColorBoxBitmapMixed(GpTermEntry * pTerm);
	void   DrawInsideColorBoxBitmapDiscrete(GpTermEntry * pTerm);
	void   DrawInsideColorBoxBitmapSmooth(GpTermEntry * pTerm);
	void   DrawInsideColorSmoothBoxPostScript();
	void   DrawPolarClipLine(GpTermEntry * pTerm, double xbeg, double ybeg, double xend, double yend);
	void   DrawLineHidden(GpTermEntry * pTerm, GpVertex * v1, GpVertex * v2/* pointers to the end vertices */, lp_style_type * lp/* line and point style to draw in */);
	void   DrawClipPolygon(GpTermEntry * pTerm, int points, gpiPoint * p);
	void   FinishFilledCurve(GpTermEntry * pTerm, int points, gpiPoint * pCorners, curve_points * pPlot);
	void   AxisOutputTics(GpTermEntry * pTerm, AXIS_INDEX axis/* axis number we're dealing with */, int * ticlabel_position/* 'non-running' coordinate */,
		AXIS_INDEX zeroaxis_basis/* axis to base 'non-running' position of * zeroaxis on */, GpTicCallback callback/* tic-drawing callback function */);
	void   GenTics(GpTermEntry * pTerm, GpAxis * pThis, GpTicCallback callback);
	void   AdjustOffsets();
	void   AdjustNonlinearOffset(GpAxis * pAxSecondary);
	void   CheckCornerHeight(GpCoordinate * p, double height[2][2], double depth[2][2]);
	int    Get3DData(GpTermEntry * pTerm, GpSurfacePoints * pPlot);
	void   GridNonGridData(GpSurfacePoints * pPlot);
	bool   GetArrow3D(GpTermEntry * pTerm, arrow_def * pArrow, double * pDsx, double * pDsy, double * pDex, double * pDey);
	void   CalculateSetOfIsoLines(AXIS_INDEX valueAxIdx, bool cross, iso_curve ** ppThisIso, AXIS_INDEX iso_axis,
		double isoMin, double isoStep, int numIsoToUse, AXIS_INDEX samAxIdx, double samMin, double samStep, int numSamToUse);
	void   Setup3DBoxCorners();
	void   PlaceSpiderPlotAxes(GpTermEntry * pTerm, const curve_points * pFirstPlot, int layer);
	void   AutoscaleBoxPlot(GpTermEntry * pTerm, curve_points * pPlot);
	void   AxisDraw2DZeroAxis(GpTermEntry * pTerm, AXIS_INDEX axis, AXIS_INDEX crossaxis);
	void   ResetTextColor(GpTermEntry * pTerm, const t_colorspec * tc);
	void   SetLogScale();
	void   Convert(GpValue * pVal, int t_num) const;
	void   GetFilledCurvesStyleOptions(filledcurves_opts * fco);
	void   SetMargin(GpPosition * pMargin);
	void   SetPalette();
	void   SetColorMap();
	void   SetColorMapRange();
	void   SetObj(int tag, int objType);
	void   SetObject();
	void   SetStyle();
	void   SetKey();
	void   SetPaletteFunction();
	void   SetMouse(GpTermEntry * pTerm);
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
	void   CheckAxisReversed(AXIS_INDEX axIdx);
	void   SaveZeroAxis(FILE * fp, AXIS_INDEX axis);
	void   SetFit();
	void   SetCntrLabel();
	void   SetContour();
	void   SetBoxPlot();
	void   ResetLogScale(GpAxis * pAx);
	void   EventPlotDone(GpTermEntry * pTerm);
	void   UpdateSecondaryAxisRange(GpAxis * pAxPrimary);
	void   ReconcileLinkedAxes(GpAxis * pAxPrimary, GpAxis * pAxSecondary);
	double MapX3D(double x);
	double MapY3D(double y);
	double MapZ3D(double z);
	void   UpdateRuler(GpTermEntry * pTerm);
	int    GetData(curve_points * pPlot);
	void   RefreshBounds(GpTermEntry * pTerm, curve_points * pFirstPlot, int nplots);
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
	void   GetOffsets(GpTermEntry * pTerm, text_label * pLabel, int * pHTic, int * pVTic);
	int    NearestLabelTag(GpTermEntry * pTerm, int xref, int yref);
	at_type * PermAt();
	at_type * ExternalAt(const char * func_name);
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
	void   SetGrid(GpTermEntry * pTerm);
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
	void   SetTermOptions(GpTermEntry * pTerm);
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
	void   SetFontPath();
	void   SetParametric();
	void   SetPointIntervalBox();
	void   SetPointSize();
	void   SetSamples();
	void   SetRgbMax();
	void   SetLoadPath();
	void   SetLocale();
	void   SetXyPlane();
	void   SetTicsLevel();
	void   SetTimeFmt();
	void   SetZero();
	void   SetCornerPoles();
	void   SetMtTics(GpAxis * pAx);
	void   SetDecimalSign();
	void   SetPsDir();
	void   SetMicro();
	void   SetMinusSign();
	void   SetRaxis();
	void   SetDegreeSign(char * locale);
	int    AssignArrowStyleTag();
	void   LoadTics(GpAxis * pAx);
	void   LoadTicUser(GpAxis * pAx);
	void   LoadTicSeries(GpAxis * pAx);
	bool   GridMatch(AXIS_INDEX axis, const char * pString);
	void   CheckPaletteGrayscale();
	void   UpdatePlotBounds(GpTermEntry * pTerm);
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
	void   UnsetPixmap(int i);
	void   UnsetPixmaps();
	void   UnsetArrowStyles();
	void   UnsetDashType();
	void   UnsetObject();
	void   UnsetTimeStamp();
	void   UnsetMonthDayTics(AXIS_INDEX axis);
	void   UnsetAllTics();
	void   UnsetVGrid();
	void   UnsetPointIntervalBox();
	void   UnsetPointSize();
	void   UnsetColorBox();
	void   UnsetSamples();
	void   ResetColorBox();
	void   UnsetPalette();
	void   UnsetOrigin();
	void   UnsetOffsets();
	void   UnsetBars();
	void   ResetBars();
	void   UnsetView();
	void   ResetKey();
	void   UnsetIsoSamples();
	void   UnsetGrid();
	void   UnsetCntrParam();
	void   UnsetCntrLabel();
	void   UnsetStyleSpiderPlot();
	void   UnsetZero();
	void   UnsetStyleParallel();
	void   UnsetWall(int which);
	void   UnsetBoxWidth();
	void   UnsetSurface();
	void   UnsetFillStyle();
	void   UnsetHistogram();
	void   UnsetStyleRectangle();
	void   UnsetStyleCircle();
	void   UnsetStyleEllipse();
	void   UnsetBorder();
	void   UnsetBoxPlot();
	void   UnsetPm3D();
	void   UnsetTicsLevel();
	void   UnsetTextboxStyle();
	void   UnsetPrint();
	void   UnsetAngles();
	void   UnsetContour();
	void   UnsetHidden3D();
	void   UnsetKey();
	void   UnsetLineStyle(linestyle_def ** ppHead);
	void   UnsetMissing();
	void   UnsetMouse();
	void   UnsetDGrid3D();
	void   UnsetBoxDepth();
	void   UnsetMapping();
	void   UnsetTimeFmt();
	void   UnsetLocale();
	void   UnsetDecimalSign();
	void   UnsetMicro();
	void   UnsetMinusSign();
	void   UnsetJitter();
	void   DeleteDashType(custom_dashtype_def * prev, custom_dashtype_def * pThis);
	void   Pm3DReset();
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
	void   ShowLineType(linestyle_def * pListHead, int tag);
	void   ShowArrowStyle(int tag);
	void   ShowClip();
	void   ShowVariables();
	void   ShowOffsets();
	void   ShowColorBox();
	void   ShowRange(AXIS_INDEX axis);
	void   ShowTics(bool showx, bool showy, bool showz, bool showx2, bool showy2, bool showcb);
	void   ShowTicdef(AXIS_INDEX axis);
	void   ShowSize();
	void   ShowPosition(const GpPosition * pPos, int ndim);
	void   ShowHistogram();
	void   ShowStyleCircle();
	void   ShowStyleEllipse();
	void   ShowLabel(int tag);
	void   ShowArrow(int tag);
	void   ShowKey();
	void   ShowXyzLabel(const char * pName, const char * pSuffix, text_label * pLabel);
	void   ShowTicDefp(const GpAxis * pAx);
	void   ShowTimeStamp();
	void   ShowTitle();
	void   ShowAxisLabel(AXIS_INDEX axIdx);
	void   ShowView();
	void   ShowBoxWidth();
	void   ShowDashType(int tag);
	void   ShowIsoSurface();
	void   ShowIsoSamples();
	void   ShowSamples();
	void   ShowVGrid();
	void   ShowOverflow();
	void   ShowContour();
	void   ShowAutoScale();
	void   ShowBorder();
	void   ShowBoxPlot();
	void   ShowFillStyle();
	void   ShowStyleRectangle();
	void   ShowDGrid3D();
	void   ShowPm3D();
	void   ShowDataIsTimeDate(AXIS_INDEX axIdx);
	void   ShowSurface();
	void   ShowPrint();
	void   ShowParametric();
	void   ShowColorNames(const gen_table * tbl);
	void   ShowPaletteColorNames();
	void   ShowPointSize();
	void   ShowPointIntervalBox();
	void   ShowRgbMax();
	void   ShowStyles(const char * pName, enum PLOT_STYLE style);
	void   ShowFunctions();
	void   ShowHidden3D();
	void   ShowIncrement();
	void   ShowLineStyle(int tag);
	void   ShowHidden3DOptions();
	void   ShowAngles();
	void   ShowPolar();
	void   ShowTextBox();
	void   ShowVersion(FILE * fp);
	void   ShowDummy();
	void   ShowOrigin();
	void   ShowKeyTitle();
	void   ShowZero();
	void   ShowMapping();
	void   ShowRAxis();
	void   ShowTimeFmt();
	void   ShowMTics(const GpAxis * pAx);
	void   ShowNonLinear();
	void   ShowMouse();
	void   ShowPlot();
	void   ShowOutput();
	void   ShowPsDir();
	void   ShowEncoding();
	void   ShowDecimalSign();
	void   ShowMinusSign();
	void   ShowTerm(GpTermEntry * pTerm);
	void   ShowLocale();
	void   ShowLoadPath();
	void   ShowFontPath();
	void   ShowHistory();
	void   ShowMicro();
	void   ShowAllNl();
	void   ShowJitter();
	void   SaveStyleTextBox(FILE * fp);
	void   SaveFit(FILE * fp);
	void   SaveAll(FILE * fp);
	void   SaveOffsets(FILE * fp, const char * lead);
	void   SavePosition(FILE * fp, const GpPosition * pPos, int ndim, bool offset);
	void   SaveAxisLabelOrTitle(FILE * fp, const char * pName, const char * pSuffix, text_label * pLabel, bool savejust);
	void   SaveTics(FILE * fp, const GpAxis * pAx);
	void   SaveHistogramOpts(FILE * fp);
	void   SavePixmaps(FILE * fp);
	void   SaveObject(FILE * fp, int tag);
	void   SaveBars(FILE * fp);
	void   SaveStyleParallel(FILE * fp);
	void   SaveStyleSpider(FILE * fp);
	void   SaveHidden3DOptions(FILE * fp);
	void   SaveDataFuncStyle(FILE * fp, const char * which, enum PLOT_STYLE style);
	void   SaveVariables(FILE * fp);
	void   Save_Variables__Sub(FILE * fp);
	void   SaveFunctions(FILE * fp);
	void   Save_Functions__Sub(FILE * fp);
	void   SaveSet(FILE * fp);
	void   SaveArrayContent(FILE * fp, GpValue * pArray);
	void   SaveColorMaps(FILE * fp);
	void   SaveWalls(FILE * fp);
	void   SaveDatablocks(FILE * fp);
	void   SaveTerm(GpTermEntry * pTerm, FILE * fp);
	void   SaveNumOrTimeInput(FILE * fp, double x, const GpAxis * pAx);
	void   SavePRange(FILE * fp, const GpAxis & rAx);
	void   SaveJitter(FILE * fp);
	void   InitializePlotStyle(curve_points * pPlot);
	void   DfDetermineMatrix_info(FILE * fin);
	void   DfSetKeyTitleColumnHead(const curve_points * pPlot);
	void   AdjustBinaryUseSpec(curve_points * pPlot);
	int    Token2Tuple(double * pTuple, int dimension);
	void   DrawTitles(GpTermEntry * pTerm);
	void   Draw3DGraphBox(GpTermEntry * pTerm, const GpSurfacePoints * pPlot, int plotNum, WHICHGRID whichgrid, int currentLayer);
	void   DrawColorSmoothBox(GpTermEntry * pTerm, int plotMode);
	void   Plot3DLinesPm3D(GpTermEntry * pTerm, GpSurfacePoints * pPlot);
	void   Plot3DPoints(GpTermEntry * pTerm, GpSurfacePoints * pPlot);
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
	void   MpLayoutMarginsAndSpacing(GpTermEntry * pTerm);
	void   MpLayoutSetMarginOrSpacing(GpPosition * pMargin);
	double JDist(const GpCoordinate * pi, const GpCoordinate * pj);
	void   UpdateStatusLineWithMouseSetting(GpTermEntry * pTerm, mouse_setting_t * ms);
	void   SetExplicitRange(GpAxis * pAx, double newmin, double newmax);
	void   GetImageOptions(t_image * image);
	int    GetPm3DAtOption(char * pm3d_where);
	char ** GetDatablock(const char * pName);
	void   Pm3DOptionAtError();
	void   ParametricFixup(curve_points * pStartPlot, int * pPlotNum);
	void   BoxRangeFiddling(const curve_points * pPlot);
	void   ImpulseRangeFiddling(const curve_points * pPlot);
	void   HistogramRangeFiddling(curve_points * pPlot);
	void   ParallelRangeFiddling(const curve_points * pPlot);
	void   PolarRangeFiddling(const curve_points * pPlot);
	void   Refresh3DBounds(GpTermEntry * pTerm, GpSurfacePoints * pFirstPlot, int nplots);
	int    InFront(GpTermEntry * pTerm, long edgenum/* number of the edge in elist */, long vnum1, long vnum2/* numbers of its endpoints */, long * firstpoly/* first plist index to consider */);
	bool   NeedFillBorder(GpTermEntry * pTerm, const fill_style_type * pFillStyle);
	gnuplot_contours * Contour(int numIsoLines, iso_curve * pIsoLines);
	void   FillGpValSysInfo();
	t_object * NewObject(int tag, int objectType, t_object * pNew);
	void   AttachTitleToPlot(GpTermEntry * pTerm, curve_points * pPlot, const legend_key * pkey);
	int    ApplyLightingModel(GpCoordinate * v0, GpCoordinate * v1, GpCoordinate * v2, GpCoordinate * v3, double gray, bool grayIsRgb);
	void   ApplyHeadProperties(GpTermEntry * pTerm, const arrow_style_type * pArrowProperties);
	//
	void   WidestTicCallback(GpTermEntry * pTerm, GpAxis * this_axis, double place, const char * text, int ticlevel, const lp_style_type & rGrid, ticmark * userlabels); // callback
	void   CbTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, const char * text, int ticlevel, const lp_style_type & rGrid/* linetype or -2 for no grid */, ticmark * userlabels);
	void   XTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, const char * text, int ticlevel, const lp_style_type & rGrid/* linetype or -2 for none */, ticmark * userlabels);
	void   YTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, const char * text, int ticlevel, const lp_style_type & rGrid, ticmark * userlabels);
	void   ZTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, const char * text, int ticlevel, const lp_style_type & rGrid, ticmark * userlabels);
	void   YTick2DCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, const char * text, int ticlevel, const lp_style_type & rGrid/* grid.l_type == LT_NODRAW means no grid */, ticmark * userlabels/* User-specified tic labels */);
	void   XTick2DCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, const char * text, int ticlevel, const lp_style_type & rGrid/* grid.l_type == LT_NODRAW means no grid */, ticmark * userlabels/* User-specified tic labels */);
	void   TTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, const char * text, int ticlevel, const lp_style_type & rGrid/* grid.l_type == LT_NODRAW means no grid */, ticmark * userlabels/* User-specified tic labels */);
	void   SpiderTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, const char * text, int ticlevel, const lp_style_type & rGrid, ticmark * userlabels);
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
	double * DfReadMatrix(int * pRows, int * pCols);
	bool   Regress(double a[]);
	void   RegressFinalize(int iter, double chisq, double lastChisq, double lambda, double ** ppCovar);
	void   GenInterpFrequency(curve_points * pPlot);
	void   GenInterpUnwrap(curve_points * pPlot);
	void   DoFreq(curve_points * pPlot/* still contains old plot->points */, int first_point/* where to start in plot->points */, int num_points/* to determine end in plot->points */);
	void   BindProcess(char * lhs, char * rhs, bool allwindows);
	void   InitVoxelSupport();
	void   BeginClause();
	void   EndClause();
	double FASTCALL GetNumOrTime(const GpAxis * pAx);
	td_type GStrPTime(char * s, char * fmt, struct tm * tm, double * usec, double * reltime);
	void   LoadFile(FILE * fp, char * pName, int calltype);
	void   LoadRcFile(int where);
	void   LoadLineType(GpTermEntry * pTerm, lp_style_type * pLp, int tag);
	void   LpUseProperties(GpTermEntry * pTerm, lp_style_type * pLp, int tag);
	text_label * StoreLabel(GpTermEntry * pTerm, text_label * pListHead, GpCoordinate * cp, int i/* point number */, const char * string/* start of label string */, 
		double colorval/* used if text color derived from palette */);
	enum PLOT_STYLE GetStyle();
	void   VertexInterp(int edgeNo, int start, int end, t_voxel isolevel);
	void   SetRuler(GpTermEntry * pTerm, bool on, int mx, int my);
	void   EventReset(GpEvent * ge, GpTermEntry * pTerm);
	void   EventModifier(GpEvent * ge, GpTermEntry * pTerm);
	int    PlotMode(GpTermEntry * pTerm, int set);
	void   TurnRulerOff(GpTermEntry * pTerm);
	void   PutLabel(GpTermEntry * pTerm, const char * pLabel, double x, double y);
	void   DeleteObject(GpObject * pPrev, GpObject * pThis);
	void   DeleteLabel(text_label * pPrev, struct text_label * pThis);
	void   AdvanceKey(bool onlyInvert);
	void   BuildNetworks(GpSurfacePoints * pPlots, int pcount);
	long   StoreVertex(GpCoordinate * pPoint, lp_style_type * pLpStyle, bool colorFromColumn);
	void   ReevaluatePlotTitle(curve_points * pPlot);
	bool   IsPlotWithColorbox() const;
	void   DfInit();
	void   DfClose();
	char * DfGenerateAsciiArrayEntry();
	char * DfGets();
	int    DfTokenise(char * s);
	void   DfShowData();
	char * DfRetrieveColumnHead(int column);
	void   DfUnsetDatafileBinary();
	void   DfShowBinary(FILE * fp);
	int    DfSkipBytes(off_t nbytes);
	void   DfSetKeyTitle(curve_points * pPlot);
	bool   DfReadPixmap(t_pixmap * pixmap);
	void   InitializeBinaryVars();
	int    ExpectString(const char column);
	void   AddKeyEntry(char * pTempString, int dfDatum);
	void   PlotOptionBinaryFormat(char * pFormatString);
	void   LogAxisRestriction(FILE * log_f, int param, double min, double max, int autoscale, const char * pName);
	void   DrawArrow(GpTermEntry_Static * pThis, uint usx, uint usy/* start point */, uint uex, uint uey/* end point (point of arrowhead) */, int headstyle);
	int    DoSearch(int dir);
	void   PrintSearchResult(const HIST_ENTRY * result);
	void   SwitchPrompt(const char * pOldPrompt, const char * pNewPrompt);
	void   ClearLine(const char * pPrompt);
	void   ClearEoline(const char * pPrompt);
	void   CopyLine(const char * pLine);
	void   RedrawLine(const char * pPrompt);
	void   DeletePreviousWord();
	void   DeleteForward();
	void   DeleteBackward();
	void   StepForward();
	void   ExtendCurLine();
	void   FixLine();
	void   TabCompletion(bool forward);
	int    CharSeqLen();
	int    BackSpace();
	int    FASTCALL GetNum(char pStr[]);
	//
	// direction into which the polygon is facing (the corner with the
	// right angle, inside the mesh, that is). The reference identifiying
	// the whole cell is always the lower right, i.e. southeast one. 
	//
	enum polygon_direction {
		pdir_NE, 
		pdir_SE, 
		pdir_SW, 
		pdir_NW
	};
	long   StorePolygon(long vnum1, polygon_direction direction, long crvlen);
	void   SortPolysByZ();
	int    CoordToTreeCell(coordval x) const;
	void   ChangeAzimuth(GpTermEntry * pTerm, int x);
	void   ChangeView(GpTermEntry * pTerm, int x, int z);
	void   Pm3DRearrangeScanArray(GpSurfacePoints * pPlot, iso_curve *** pppFirstPtr, int * pFirstN, int * pFirstInvert,
		iso_curve *** pppSecondPtr, int * pSecondN, int * pSecondInvert);
	void   Pm3DRearrangePart(iso_curve * pSrc, const int len, struct iso_curve *** pppDest, int * invert);
	int    ClipFilledPolygon(const gpdPoint * pInpts, gpdPoint * pOutpts, int nv);
	void   ModifyVoxels(t_voxel * pGrid, double x, double y, double z, double radius, at_type * pDensityFunction, bool gridcoordinates);
	udvt_entry * GetVGridByName(const char * name);
	void   Invert_RtR(double ** ppR, double ** ppI, int n);
	void   EvalResetAfterError();
	int    CheckOrAddBoxplotFactor(curve_points * pPlot, const char * pString, double x);
	void   AddTicsBoxplotFactors(curve_points * pPlot);
	void   AddTicUser(GpAxis * pAx, const char * pLabel, double position, int level);
	void   FASTCALL AxisCheckEmptyNonLinear(const GpAxis * pAx);
	void   ZoomNext(GpTermEntry * pTerm);
	void   ZoomPrevious(GpTermEntry * pTerm);
	void   ZoomUnzoom(GpTermEntry * pTerm);
	void   IncrMouseMode(const int amount);
	bind_t * GetBinding(GpEvent * ge, bool current);
	void   BindAll(char * lhs);
	void   BindClear(bind_t * b);
	void   BindRemove(bind_t * b);
	void   BindRemoveAll();
	void   BindDisplay(char * lhs);
	void   BindDisplayOne(bind_t * ptr);
	int    BindMatches(const bind_t * a, const bind_t * b);
	char * BindFmtLhs(const bind_t * in);
	int    BindScanLhs(bind_t * out, const char * in);
	void   DoZoomInAroundBouse();
	void   DoZoomOutAroundMouse();
	void   DoZoomScrollLeft();
	void   DoZoomScrollRight();
	void   DoZoomScrollUp();
	void   DoZoomScrollDown();
	void   DoZoomInX();
	void   DoZoomOutX();
	void   RecalcStatusLine();
	void   LoadContourLabelOptions(GpTermEntry * pTerm, text_label * pContourLabel);
	void   TimedPause(GpTermEntry * pTerm, double sleepTime);
	char * MkStr(char * sp, double x, AXIS_INDEX axis);
	int    IsMouseOutsidePlot();
	void   GetRulerString(char * p, double x, double y);
	int    FindMaxlKeys3D(const GpSurfacePoints * pPlots, int count, int * pKCnt);
	int    FindMaxlCntr(const gnuplot_contours * pContours, int * pCount);
	char * FnCompletion(size_t anchor_pos, int direction);
	void   PrintLine(const char * pStr);
	union argument * AddAction(enum operators sf_index);
	void   ExtendAt();
	void   ParseResetAfterError();
	void   SetUpColumnHeaderParsing(const at_entry * previous);
	bool   AxisPositionZeroAxis(AXIS_INDEX axis);
	intgr_t GetIVar(const char * pVarName);
	double GetDVar(const char * pVarName);
	void   RegressInit();
	GpFileStats AnalyzeFile(long n, int outofrange, int invalid, int blank, int dblblank, int headers);
	void   EnsureOutput();
	void   FileOutput(GpFileStats s);
	void   TwoColumnOutput(SglColumnStats x, SglColumnStats y, TwoColumnStats xy, long n);
	void   SglColumnOutputNonFormat(SglColumnStats s, const char * x);
	void   SglColumnOoutput(SglColumnStats s, long n);
	void   PrintSetOutput(char * pName, bool datablock, bool append_p);
	const  char * PrintShowOutput();
	char * NewClause(int clauseStart, int clauseEnd);
	void   ClauseResetAfterError();
	bool   SomeGridSelected();
	void   ExtendInputLine();
	int    Expand1LevelMacros();
	void   StringExpandMacros();
	void   Substitute(char ** ppStr, size_t * pStrLen, int current);
	int    Scanner(char ** ppExpression, size_t * pExpressionLen);
	int    ReadLine(const char * pPrompt, int start);
	void   InitMemory();
	void   Analyze(double pA[], double ** ppC, double pD[], double * pChisq, double ** ppDeriv);
	void   Calculate(double * pZFunc, double ** ppDzda, double pA[]);
	void   CalcDerivatives(const double * pPar, double * pData, double ** ppDeriv);
	double EffectiveError(double ** ppDeriv, int i);
	bool   RegressCheckStop(int iter, double chisq, double last_chisq, double lambda);
	bool   FitInterrupt();
	void   FitProgress(int i, double chisq, double last_chisq, double * pA, double lambda, FILE * pDevice);
	void   FitShow(int i, double chisq, double last_chisq, double * pA, double lambda, FILE * pDevice);
	void   FitShowBrief(int iter, double chisq, double last_chisq, double * pParms, double lambda, FILE * pDevice);
	int    AssignArrowTag();
	int    AssignLabelTag();
	bool   IterationEarlyExit();
	parsefuncp_t LookupFTable(const gen_ftable * pFTbl, int findToken); // @unused
	void   InitializeUseSpec();
	void   ResetHidden3DOptions();
	GpSplineCoeff * CpApproxSpline(const GpCoordinate * pPoints, int numPoints, int pathDim, int splineDim, int wDim);
	GpSplineCoeff * CpTriDiag(const GpCoordinate * pPoints, int numPoints, int pathDim, int splineDim);
	void   ThinPlateSplinesSetup(const iso_curve * pOldIsoCrvs, double ** ppXX, int * pNumPoints);
	void   LuDecomp(double ** ppA, int n, int * pIndx, double * pD);
	void   SetVarErr(const char * pVarName, double value);
	void   SetVarCovar(const char * pVarName1, const char * pVarName2, double value);
	void   SetVar(const char * pVarName, double data);
	double CreateDVar(const char * pVarName, double value);
	void   LoadFileError();
	FILE * LfTop();
	bool   Pm3DObjects();
	double Ranf(const GpValue * pInit);
	double InverseIncompleteGamma(double a, double p);
	char * XDateTimeFormat(double x, char * pB, int mode);
	void   SetupTics(GpAxis * pAx, int max);
	char * CopyOrInventFormatString(GpAxis * pAx);
	const  char * GetFitScript();
	char * GetFitLogFile();
	void   ScreenDump(GpTermEntry * pTerm);
	uint   Hsv2Rgb(rgb_color * pColor);
	GpSurfacePoints * SpAlloc(int num_samp_1, int num_iso_1, int num_samp_2, int num_iso_2);
	void   DfInsertScannedUseSpec(int uspec);
	char * GpGetString(char * buffer, size_t len, const char * pPrompt);
	char * RlGets(char * s, size_t n, const char * pPrompt);
	char * FGetsIpc(GpTermEntry * pTerm, char * dest/* string to fill */, int len/* size of it */);
	void   ArrowUseProperties(arrow_style_type * pArrow, int tag);
	int    CheckMissing(const char * s);
	char * DfParseStringField(const char * field);
	void   DfResetAfterError();
	void   RequireValue(const char column);
	void   FileTypeFunction_GpBin();
	void   FileTypeFunction_Raw();
	void   FileTypeFunction_Avs();
	void   FileTypeFunction_Edf();
	void   Implement_FileTypeFunction_Gd(int type);
	void   FileTypeFunction_Png();
	void   FileTypeFunction_Gif();
	void   FileTypeFunction_Jpeg();
	void   DfAddBinaryRecords(int num_records_to_add, df_records_type records_type);
	void   ClearBinaryRecords(df_records_type records_type);
	int    AxColForTicLabel(enum COLUMN_TYPE type, int * axis);
	void   DoShell();
	int    DoSystemFunc(const char * cmd, char ** output);
	void   Parametric3DFixUp(GpSurfacePoints * pStartPlot, int * pPlotNum);
	void   RestorePrompt();
	int    FindMaxlKeys(const curve_points * pPlots, int count, int * kcnt);
	int    LabelWidth(const char * pStr, int * pLines);
	void   FreeHistogramList(histogram_style * pHist);
	void   ParseEsc(char * pInStr);
	void   MQuoteCapture(char ** ppStr, int start, int end);
	int    MsDosGetch(GpTermEntry * pTerm);
	int    WinGetch(GpTermEntry * pTerm);
	void   FASTCALL CheckAxisRange(AXIS_INDEX idx);
	GpAxis * GetShadowAxis(GpAxis * pAx);
	void   CheckAxisLogLimits(const GpAxis * pAx, double min, double max);
	void   DispAt(const at_type * pCurrAt, int level);
	double MakeTics(GpAxis * pAx, int guide);
	void   GStrDMS(char * pLabel, char * pFormat, double value);
	void   FillGpValAxis(AXIS_INDEX axIdx);
	void   SetGpValAxisSthDouble(const char * pPrefix, AXIS_INDEX axIdx, const char * pSuffix, double value);
	void   InvalidatePalette();
	int    GGmTime(struct tm * pTm, double l_clock);
	double RoundOutward(const GpAxis * pAx/* Axis to work on */, bool upwards/* extend upwards or downwards? */, double input/* the current endpoint */);
	double TimeTicJust(t_timelevel level, double ticplace);
	int    TmWeek(double time);
	size_t GStrFTime(char * pS, size_t bsz, const char * pFmt, double l_clock);
	size_t XStrFTime(char * pStr/* output buffer */, int bsz/* remaining space available in buffer */, const char * pFmt, struct tm * pTm, double usec, double fulltime);
	void   InitHiddenLineRemoval();
	void   ResetHiddenLineRemoval();
	void   TermHiddenLineRemoval();
	void   IgnoreEnhanced(bool flag);
	void   ColorComponentsFromGray(double gray, rgb_color * pColor);
	int    IsDoubleWidth(size_t pos);
	void   DelUdvByName(const char * pKey, bool wildcard);
	void * NextFromDynArray(dynarray * pArray);
	void   ExtendDynArray(dynarray * pArray, long increment);
	void   ResizeDynArray(dynarray * pArray, long newsize);
	void   DropLastDynArray(dynarray * pArray);
	// 
	// Enumeration of possible types of line, for use with the
	// store_edge() function. Influences the position in the grid the
	// second vertex will be put to, relative to the one that is passed
	// in, as another argument to that function. edir_none is for
	// single-pixel 'blind' edges, which exist only to facilitate output
	// of 'points' style splots.
	// 
	// Directions are interpreted in a pseudo-geographical coordinate
	// system of the data grid: within the isoline, we count from left to
	// right (west to east), and the isolines themselves are counted from
	// top to bottom, described as north and south. 
	// 
	enum edge_direction {
		edir_west, 
		edir_north,
		edir_NW, 
		edir_NE,
		edir_impulse, 
		edir_point,
		edir_vector
	};
	long   StoreEdge(long vnum1, edge_direction direction, long crvlen, lp_style_type * lp, int style);
	long   MakeEdge(long vnum1, long vnum2, lp_style_type * lp, int style, int next);
	int    CoverPointPoly(const GpVertex * v1, const GpVertex * v2, double u, GpMeshTriangle * poly);
	void   ColorEdges(long new_edge/* index of 'new', conflictless edge */,
		long old_edge/* index of 'old' edge, may conflict */, long new_poly/* index of current polygon */,
		long old_poly/* index of poly sharing old_edge */, int above/* style number for front of polygons */, int below/* style number for backside of polys */);
	bool   GetPlane(GpMeshTriangle * poly, t_plane plane);
	void   SortEdgesByZ();
	long   SplitLineAtRatio(long vnum1, long vnum2/* vertex indices of line to split */, double w/* where to split it */);
	void   WriteMultiline(GpTermEntry * pTerm, int x, int y, const char * pText, JUSTIFY hor/* horizontal ... */,
		VERT_JUSTIFY vert/* ... and vertical just - text in hor direction despite angle */, int angle/* assume term has already been set for this */,
		const char * pFont/* NULL or "" means use default */);
	char * LocaleHandler(int action, char * newlocale);
	char * EstimatePlainText(char * pEnhancedText);
	void   InitEncoding();
	void   InitSpecialChars();
	void   MantExp(double log10_base, double x, bool scientific/* round to power of 3 */, double * m/* results */, int * p, const char * format/* format string for fixup */);
	int    Help(char * pKeyword/* on this topic */, const char * pPath/* from this file */, bool * pSubTopics/* (in) - subtopics only? */);
	int    FASTCALL TypeUdv(int t_num) const;
	int    LoadDashType(t_dashtype * dt, int tag);
	void   ListTerms();
	void   PrintHelp(void/*KEY*/ * pKey, bool * pSubTopics/* (in) - subtopics only? (out) - are there subtopics? */);
	void   StartOutput();
	enum DATA_TYPES SPrintfSpecifier(const char * pFormat);

	static int ComparePolysByZMax(void * pCtx, SORTFUNC_ARGS p1, SORTFUNC_ARGS p2);
	static int CompareEdgesByZMin(void * pCtx, SORTFUNC_ARGS p1, SORTFUNC_ARGS p2);
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
#ifdef HAVE_COMPLEX_FUNCS
	_Dcomplex __Igamma(_Dcomplex a, _Dcomplex z);
	_Dcomplex __Igamma_negative_z(double a, _Dcomplex z);
	#ifdef IGAMMA_POINCARE
		_Dcomplex __Igamma_Poincare(double a, _Dcomplex z);
	#endif
	_Dcomplex _LambertW(_Dcomplex z, int k);
	void   F_LambertW(union argument * arg);
	void   F_lnGamma(union argument * arg);
	void   F_Sign(union argument * arg);
#endif
	void   F_IGamma(union argument * x);
	void   F_amos_Ai(union argument * arg);
	void   F_amos_Bi(union argument * arg);
	void   F_amos_BesselI(union argument * arg);
	void   F_amos_BesselJ(union argument * arg);
	void   F_amos_BesselK(union argument * arg);
	void   F_amos_BesselY(union argument * arg);
	void   F_amos_Hankel(int k, union argument * arg);
	void   F_amos_cexint(union argument * arg); // CEXINT is in libamos but not in libopenspecfun 
	void   F_Hankel1(union argument * arg);
	void   F_Hankel2(union argument * arg);

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
	void   F_Calle(union argument * x);

	void   F_Cerf(union argument * arg);
	void   F_CDawson(union argument * arg);
	void   F_Erfi(union argument * arg);
	void   F_Voigtp(union argument * arg);
	void   F_Faddeeva(union argument * arg);
	void   F_VP_Fwhm(union argument * arg);
	void   F_FresnelC(union argument * arg);
	void   F_FresnelS(union argument * arg);
	// 
	// Support for user-callable routines
	// 
	void   F_Hsv2Rgb(union argument *);
	void   F_RgbColor(union argument *);
	void   F_Palette(union argument *);
	void   F_TimeColumn(union argument * arg);
	void   F_Valid(union argument * arg);
	void   F_Voigt(union argument * /*arg*/);
	//
	void   EventButtonPress(GpEvent * pGe, GpTermEntry * pTerm);
	void   EventButtonRelease(GpEvent * pGe, GpTermEntry * pTerm);
	void   EventMotion(GpEvent * pGe, GpTermEntry * pTerm);
	void   EventKeyPress(GpEvent * ge, GpTermEntry * pTerm, bool current);
	//
	char * BuiltinAutoscale(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinToggleBorder(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinReplot(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinToggleGrid(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinHelp(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinInvertPlotVisibilities(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinToggleLog(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinNearestLog(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinToggleMouse(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinToggleRuler(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinSetPlotsInvisible(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinSetPlotsVisible(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinDecrementMouseMode(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinIncrementMouseMode(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinTogglePolarDistance(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinToggleVerbose(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinToggleRatio(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinZoomNext(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinZoomPrevious(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinUnzoom(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinZoomInAroundMouse(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinZoomOutAroundMouse(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinRotateRight(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinRotateUp(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinRotateLeft(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinRotateDown(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinAzimuthLeft(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinAzimuthRight(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinCancelZoom(GpEvent * ge, GpTermEntry * pTerm);
#if (0) // Not currently used 
	char * BuiltinZoomScrollLeft(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinZoomScrollRight(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinZoomScrollUp(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinZoomScrollDown(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinZoomInX(GpEvent * ge, GpTermEntry * pTerm);
	char * BuiltinZoomOutX(GpEvent * ge, GpTermEntry * pTerm);
#endif
	const char * EncodingMicro();
	const char * EncodingMinus();

	GpTermEntry _ENHest;
	char * P_PushTermName;
	char * P_PushTermOpts;
	bool   VarShowAll;
};

extern GnuPlot GPO; // @global

class GnuPlotHolder {
public:
	GnuPlotHolder()
	{
	}
};
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
