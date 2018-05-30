// GNUPLOT.H
//
#ifndef __GNUPLOT_H
#define __GNUPLOT_H

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#define _WIN32_IE 0x0501

#include <slib.h>
#include <sys\types.h>
#include "national.h"

#ifdef _PAPYRUS
	#define NO_BITMAP_SUPPORT // Используют глобальные BITMAP'ы - не нужны (pbm.trm, epson.trm, hp500c.trm, hpljii.trm, hppj.trm)
	#define HAVE_STRING_H
	#define HAVE_MALLOC_H
	#define HAVE_MEMCPY
	#define HAVE_GETCWD
	#define HAVE_VFPRINTF
	#define HAVE_STPCPY
	#define HAVE_STRCHR   // @v9.2.1
	#define HAVE_STRSTR   // @v9.2.1
	#define HAVE_STRERROR // @v9.2.1
	#if _MSC_VER >= 1900
		#define HAVE_STRNLEN  // @v9.2.1
	#endif
	#define HAVE_TIME_T_IN_TIME_H // @v9.2.1
	#define HAVE_SYS_STAT_H
	//#define HAVE_GD_PNG
	#define EAM_DATASTRINGS
	#define EAM_HISTOGRAMS
	#define EAM_OBJECTS
	#define READLINE
	#define GNUPLOT_HISTORY
	#define WITH_IMAGE
	#define BINARY_DATA_FILE
	#define USE_MOUSE
	#define PROTOTYPES
	#define NO_GIH
	#define STDC_HEADERS
	#define WIN_IPC
	#define GP_STRING_VARS 2
	#define HAVE_EXTERNAL_FUNCTIONS
	#ifdef _MSC_VER
		#define __MSC__
	#endif
	#define _Windows
	#include <time.h>
	#include <float.h>
	#include <limits.h>
	#include <math.h>

	#define ICON_PAPYRUS 101
#else
	#error "_PAPYRUS must be defined"

	#define FASTCALL
#endif
#ifndef __MSC__
	#error "__MSC__ must be defined"
#endif
//
//
//
struct UdftEntry;
struct UdvtEntry;
union GpArgument;
struct GpAxis;
struct ticmark;
struct t_colorspec;
struct t_value;
struct GpPosition;
struct LexicalUnit;
struct fill_style_type;
struct CurvePoints;
class  GpGadgets;
class  GpCommand;
struct GpEvent;
//
// GP_TYPES.H
//
//
// SYSCFG.H
//
#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif
#define HAVE_STDLIB_H
#define HAVE_STRING_H
#define HAVE_MALLOC_H
#define HAVE_MEMCPY
/*
 * Define operating system dependent constants [default value]:
 *
 * OS:       [""] Name of OS; only required if system has no uname(2) call
 * HELPFILE: ["docs/gnuplot.gih"] Location of helpfile - overridden by Makefile
 * HOME:     ["HOME"] Name of environment variable which points to
 *           the directory where gnuplot's config file is found.
 * PLOTRC:   [".gnuplot"] Name of the gnuplot startup file.
 * SHELL:    ["/bin/sh"] Name, and in some cases, full path to the shell
 *           that is used to run external commands.
 * DIRSEP1:  ['/'] Primary character which separates path components.
 * DIRSEP2:  ['\0'] Secondary character which separates path components.
 * PATHSEP:  [':'] Character which separates path names
 *
 */
#if defined(vms) || defined(VMS)
	#define OS "VMS"
	#ifndef VMS
		#define VMS
	#endif
	#define HOME   "sys$login"
	#define PLOTRC "gnuplot.ini"
	#ifdef NO_GIH
		#define HELPFILE "GNUPLOT$HELP" // for show version long 
	#endif
	#if !defined(VAXCRTL) && !defined(DECCRTL)
		#define VAXCRTL VAXCRTL_AND_DECCRTL_UNDEFINED
		#define DECCRTL VAXCRTL_AND_DECCRTL_UNDEFINED
	#endif
	// avoid some IMPLICITFUNC warnings
	#ifdef __DECC
		#include <starlet.h>
	#endif  /* __DECC */
#endif /* VMS */
#if defined(_WINDOWS) || defined(_Windows) || defined(WIN32) || defined(_WIN32)
	#ifndef _Windows
		#define _Windows
	#endif
	#ifndef WIN32
		#define WIN32
	#endif
	#ifndef _WIN32
		#define _WIN32
	#endif
	#ifdef _WIN64
		#define OS "MS-Windows 64 bit"
	#else
		#define OS "MS-Windows 32 bit"
	#endif
	// introduced by Pedro Mendes, prm@aber.ac.uk
	#define far
	// Fix for broken compiler headers See stdfn.h
	#define S_IFIFO  _S_IFIFO
	#define HOME    "GNUPLOT"
	#define PLOTRC  "gnuplot.ini"
	#define SHELL   "\\command.com"
	#define DIRSEP1 '\\'
	#define DIRSEP2 '/'
	#define PATHSEP ';'
	#define GNUPLOT_HISTORY_FILE "~\\gnuplot_history"
	// Flags for windows.h: Minimal required platform is Windows XP
	#ifndef WINVER
		#define WINVER 0x0501
	#endif
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
	#ifndef _WIN32_IE
		#define _WIN32_IE 0x0501
	#endif
#endif // _WINDOWS
//
// End OS dependent constants; fall-through defaults
// for the constants defined above are following.
//
#ifndef OS
	#define OS "non-recognized OS"
#endif
#ifndef HELPFILE
#ifndef VMS
	#define HELPFILE "docs/gnuplot.gih"
#else
	#define HELPFILE "sys$login:gnuplot.gih"
#endif
#endif
#ifndef HOME
	#define HOME "HOME"
#endif
#ifndef PLOTRC
	#define PLOTRC ".gnuplot"
#endif
#ifndef SHELL
	#define SHELL "/bin/sh"    /* used if SHELL env variable not set */
#endif
#ifndef DIRSEP1
	#define DIRSEP1 '/'
#endif
#ifndef DIRSEP2
	#define DIRSEP2 NUL
#endif
#ifndef PATHSEP
	#define PATHSEP ':'
#endif
#ifndef FAQ_LOCATION
	#define FAQ_LOCATION "http://www.gnuplot.info/faq/"
#endif
#ifndef CONTACT
	#define CONTACT "gnuplot-bugs@lists.sourceforge.net"
#endif
#ifndef HELPMAIL
	#define HELPMAIL "gnuplot-info@lists.sourceforge.net"
#endif
// End fall-through defaults
//
// Need this before any headers are incldued
#ifdef PROTOTYPES
	#define __PROTO(proto) proto
#else
	#define __PROTO(proto) ()
#endif
#ifdef __WATCOMC__
	#include <direct.h>
	#include <dos.h>
	#define HAVE_GETCWD 1
	#define GP_EXCEPTION_NAME _exception
#endif
#ifdef __MSC__
	#include <direct.h> /* for getcwd() */
#endif
#if defined(alliant)
	#undef HAVE_LIMITS_H
#endif
#ifdef sequent
	#undef HAVE_LIMITS_H
	#undef HAVE_STRCHR
#endif
/* HBB 20000416: stuff moved from plot.h to here. It's system-dependent,
 * so it belongs here, IMHO */

/* BM 20110904: remnant of huge memory model support */
#define GPHUGE /* nothing */
#define GPFAR /* nothing */

// LFS support 
#if !defined(HAVE_FSEEKO) || !defined(HAVE_OFF_T)
	#if defined(HAVE_SYS_TYPES_H)
		#include <sys/types.h>
	#endif
	#if defined(_MSC_VER)
		// @sobolev #define off_t __int64
	#elif defined(__MINGW32__)
		#define off_t off64_t
	#elif !defined(HAVE_OFF_T)
		#define off_t long
	#endif
#endif
#ifndef MAXINT // should there be one already defined ? 
	#define MAXINT INT_MAX // from <limits.h> 
#endif
//
// This is the maximum number of arguments in a user-defined function.
// Note: This could be increased further, but in this case it would be good to
// make  GpGg.Gp__C.P.CDummyVar[][] and GpGg.Gp__C.P.SetDummyVar[][] into pointer arrays rather than
// fixed-size storage for long variable name strings that will never be used.
//
#define MAX_NUM_VAR	12
#define ISO_SAMPLES 10		// default number of isolines per splot 

#ifdef VMS
	#define DEFAULT_COMMENTS_CHARS "#!"
	#define is_system(c) ((c) == '$')
	// maybe configure could check this? 
	#define BACKUP_FILESYSTEM 1
#else /* not VMS */
	#define DEFAULT_COMMENTS_CHARS "#"
	#define is_system(c) ((c) == '!')
#endif /* not VMS */
// HBB NOTE 2014-12-16: no longer defined by autoconf; hardwired here instead 
#ifndef RETSIGTYPE
	#define RETSIGTYPE void
#endif
#ifndef SIGFUNC_NO_INT_ARG
	typedef RETSIGTYPE (*sigfunc)(int);
#else
	typedef RETSIGTYPE (*sigfunc)();
#endif
#ifdef HAVE_SIGSETJMP
	#define SETJMP(env, save_signals) sigsetjmp(env, save_signals)
	#define LONGJMP(env, retval) siglongjmp(env, retval)
	#define JMP_BUF sigjmp_buf
#else
	#define SETJMP(env, save_signals) setjmp(env)
	#define LONGJMP(env, retval) longjmp(env, retval)
	#define JMP_BUF jmp_buf
#endif
//
// generic pointer type. For old compilers this has to be changed to char *,
// but I don't know if there are any CC's that support void and not void *
//
// #define generic void
//
// Macros for string concatenation
//
#ifdef HAVE_STRINGIZE
	// ANSI version
	#define CONCAT(x,y) x##y
	#define CONCAT3(x,y,z) x##y##z
#else
// K&R version
	#define CONCAT(x,y) x/**/y
	#define CONCAT3(x,y,z) x/**/y/**/z
#endif
// Windows needs to redefine stdin/stdout functions
#if defined(_Windows) && !defined(WINDOWS_NO_GUI)
	#include "win/wtext.h"
#endif
#ifndef GP_EXCEPTION_NAME
	#define GP_EXCEPTION_NAME exception
#endif
#ifndef GP_MATHERR
	#define GP_MATHERR matherr
#endif
#ifdef HAVE_STRUCT_EXCEPTION_IN_MATH_H
	#define STRUCT_EXCEPTION_P_X struct GP_EXCEPTION_NAME *x
#else
	#define STRUCT_EXCEPTION_P_X /* nothing */
#endif
//
// if GP_INLINE has not yet been defined, set to __inline__ for gcc,
// nothing. I'd prefer that any other compilers have the defn in
// the makefile, rather than having a huge list of compilers here.
// But gcc is sufficiently ubiquitous that I'll allow it here !!!
//
#ifndef GP_INLINE
	#ifdef __GNUC__
		#define GP_INLINE __inline__
	#else
		#define GP_INLINE /*nothing*/
	#endif
#endif
#include "stdfn.h"
//
// precision of calculations in normalized space. Coordinates closer to
// each other than an absolute difference of EPSILON are considered
// equal, by some of the routines in this module
const double GpEpsilon = 1e-5;
//
// UTIL.H
//
#define HISTORY_SIZE 500
#define NO_CARET (-1) // special token number meaning 'do not draw the "caret"', for int_error and friends:
#define DATAFILE (-2) // token number meaning 'the error was in the datafile, not the command line'

//extern bool screen_ok; // true if command just typed; becomes false whenever we send some other output to screen.  
	// If false, the command line will be echoed to the screen before the ^ error message.
extern char * decimalsign;       // decimal sign 
extern char * numeric_locale;    // LC_NUMERIC 
extern char * current_locale;	 // LC_TIME 
extern char   degree_sign[8];    // degree sign 
extern const  char * current_prompt; // needed by is_error() and friends

// Command parsing helpers: 
//size_t token_len(int);
void   parse_esc(char *);
//int    type_udv(int);
char * gp_stradd(const char *, const char *);
//#define isstringvalue(_tok) (GpGg.Gp__C.IsString(_tok) || GpGg.Gp__C.TypeDdv(_tok)==STRING)
// HBB 20010726: IMHO this one belongs into alloc.c: 
char * gp_strdup(const char *);
// HBB 20020405: moved this here, from axis.[ch] 
void   gprintf(char * pBuffer, size_t bufLen, const char * pFmt, double, double);

// Error message handling 
#if defined(__GNUC__)
	void os_error(int, const char *, ...)) __attribute__((noreturn);
	//void int_error(int, const char *, ...)) __attribute__((noreturn);
	//void common_error_exit() __attribute__((noreturn);
#elif defined(_MSC_VER)
	__declspec(noreturn) void os_error(int, const char *, ...);
	//__declspec(noreturn) void int_error(int, const char *, ...);
	//__declspec(noreturn) void common_error_exit();
#else
	void os_error(int, const char *, ...);
	//void int_error(int, const char *, ...);
	//void common_error_exit();
#endif
//void IntWarn(int, const char *, ...);
// To disallow 8-bit characters in variable names, set this to
// #define ALLOWED_8BITVAR(c) false 
#define ALLOWED_8BITVAR(c) ((c)&0x80)
//
//
//
#define MAX_ID_LEN 50		/* max length of an identifier */
#define MAX_LINE_LEN 1024	/* maximum number of chars allowed on line */
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
	NOTDEFINED,     // exists, but value is currently undefined
	INVALID_VALUE,  // used only for error return by external functions
	INVALID_NAME    // used only to trap errors in linked axis function definition
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
	NODATA
};
//
// we explicitly assign values to the types, such that we can
// perform bit tests to see if the style involves points and/or lines
// bit 0 (val 1) = line, bit 1 (val 2) = point, bit 2 (val 4)= error
// This allows rapid decisions about the sample drawn into the key, for example.
//
// HBB 20010610: new enum, to make mnemonic names for these flags
// accessible everywhere
enum PLOT_STYLE_FLAGS {
    PLOT_STYLE_HAS_LINE      = (1<<0),
    PLOT_STYLE_HAS_POINT     = (1<<1),
    PLOT_STYLE_HAS_ERRORBAR  = (1<<2),
    PLOT_STYLE_HAS_FILL      = (1<<3),
    PLOT_STYLE_BITS          = (1<<4)
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
    BOXES        =  9*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_LINE | PLOT_STYLE_HAS_FILL),
    BOXERROR     = 10*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_LINE | PLOT_STYLE_HAS_FILL),
    STEPS        = 11*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
    FILLSTEPS    = 11*PLOT_STYLE_BITS + PLOT_STYLE_HAS_FILL,
    FSTEPS       = 12*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
    HISTEPS      = 13*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
    VECTOR       = 14*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
    CANDLESTICKS = 15*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_ERRORBAR | PLOT_STYLE_HAS_FILL),
    FINANCEBARS  = 16*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE,
    XERRORLINES  = 17*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_LINE | PLOT_STYLE_HAS_POINT | PLOT_STYLE_HAS_ERRORBAR),
    YERRORLINES  = 18*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_LINE | PLOT_STYLE_HAS_POINT | PLOT_STYLE_HAS_ERRORBAR),
    XYERRORLINES = 19*PLOT_STYLE_BITS + (PLOT_STYLE_HAS_LINE | PLOT_STYLE_HAS_POINT | PLOT_STYLE_HAS_ERRORBAR),
    FILLEDCURVES = 21*PLOT_STYLE_BITS + PLOT_STYLE_HAS_LINE + PLOT_STYLE_HAS_FILL,
    PM3DSURFACE  = 22*PLOT_STYLE_BITS + 0,
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
    SMOOTH_BINS
};
//
// FIXME HBB 20000521: 't_value' and its part, 'cmplx', should go
// into one of scanner/internal/standard/util .h, but I've yet to decide which of them
//
struct cmplx {
	double real;
	double imag;
};

struct t_value {
	void   Init(DATA_TYPES typ)
	{
		type = typ;
		memzero(&v, sizeof(v));
	}
	t_value * SetInt(int i);
	t_value * SetComplex(double realpart, double imagpart);
	double Real() const;

    DATA_TYPES type;
    union {
		int int_val;
		struct cmplx cmplx_val;
		char *string_val;
		char **data_array;
		t_value * value_array;
    } v;
};
//
// Defines the type of a GpCoordinate
// INRANGE and OUTRANGE points have an x,y point associated with them
//
enum coord_type {
    INRANGE,			/* inside plot boundary */
    OUTRANGE,			/* outside plot boundary, but defined */
    UNDEFINED,			/* not defined at all */
    EXCLUDEDRANGE		/* would be inside plot, but excluded for other reasons */
				/* e.g. in polar mode and outside of trange[tmin:tmax] */
};
//
// These fields of 'GpCoordinate' hold extra properties of 3D data points
// Used by splot styles RGBIMAGE and RGBA_IMAGE
//
#define CRD_R      yhigh
#define CRD_G      xlow
#define CRD_B      xhigh
#define CRD_A      ylow
#define CRD_COLOR  yhigh // Used by all splot style with variable line/point color 
#define CRD_PTSIZE xlow  // Used by splot styles POINTSTYLE and LINESPOINTS with variable point size 
#define CRD_PTTYPE xhigh // Used by splot styles POINTSTYLE and LINESPOINTS with variable point type 

struct GpCoordinate : public RPoint3 {
	GpCoordinate()
	{
		THISZERO();
	}
	GpCoordinate(coord_type t, double _x, double _y, double _z, double _ylo, double _yhi, double _xlo, double _xhi)
	{
		type = t;
		RPoint3::Set(_x, _y, _z);
		ylow  = _ylo;
		yhigh = _yhi;
		xlow  = _xlo;
		xhigh = _xhi;
	}
    coord_type type; // see above
    //double x, y, z;
    double ylow, yhigh; // ignored in 3d
    double xlow, xhigh; // also ignored in 3d
	// @construction RealRange Yr;
	// @construction RealRange Xr;
};

enum lp_class {
	LP_TYPE   = 0,	/* lp_style_type defined by 'set linetype' */
	LP_STYLE  = 1,	/* lp_style_type defined by 'set style line' */
	LP_ADHOC  = 2,	/* lp_style_type used for single purpose */
	LP_NOFILL = 3	/* special treatment of fillcolor */
};
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

inline t_autoscale operator ~ (t_autoscale a1)
	{ return (t_autoscale)~((int)a1); }
inline t_autoscale operator | (t_autoscale a1, t_autoscale a2)
	{ return (t_autoscale)((int)a1 | (int)a2); }
inline t_autoscale operator &= (t_autoscale & rA1, t_autoscale a2)
	{ return (rA1 = (t_autoscale)((int)rA1 & (int)a2)); }
inline t_autoscale operator |= (t_autoscale & rA1, t_autoscale a2)
	{ return (rA1 = (t_autoscale)((int)rA1 | (int)a2)); }

enum t_constraint {
    CONSTRAINT_NONE  = 0,
    CONSTRAINT_LOWER = 1<<0,
    CONSTRAINT_UPPER = 1<<1,
    CONSTRAINT_BOTH  = (1<<0 | 1<<1)
};

inline t_constraint operator ~ (t_constraint a1)
	{ return (t_constraint)~((int)a1); }
inline t_constraint operator &= (t_constraint & rA1, t_constraint a2)
	{ return (rA1 = (t_constraint)((int)rA1 & (int)a2)); }
inline t_constraint operator |= (t_constraint & rA1, t_constraint a2)
	{ return (rA1 = (t_constraint)((int)rA1 | (int)a2)); }
//
// The unit the tics of a given time/date axis are to interpreted in
// HBB 20040318: start at one, to avoid undershoot
//
enum t_timelevel {
	TIMELEVEL_UNDEF = 0, // @sobolev
    TIMELEVEL_SECONDS = 1,
	TIMELEVEL_MINUTES,
	TIMELEVEL_HOURS,
    TIMELEVEL_DAYS,
	TIMELEVEL_WEEKS,
	TIMELEVEL_MONTHS,
    TIMELEVEL_YEARS
};
//
//
//
#ifdef EXTENDED_COLOR_SPECS
struct colorspec_t {
	double gray;
	// to be extended
};
#endif
//
// EAM July 2004 - Disentangle polygon support and PM3D support
// a point (with integer coordinates) for use in polygon drawing
//
struct gpiPoint {
	int    x;
	int    y;
#ifdef EXTENDED_COLOR_SPECS
	double z;
	colorspec_t spec;
#endif
	int    style;
};
//
// TABLES.H
//
typedef void (*parsefuncp_t)();

struct GpGenFTable {
    const char * key;
    parsefuncp_t value;
};
//
// The basic structure 
//
struct GenTable {
    const  char * key;
    int    value;
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
enum save_id { 
	SAVE_INVALID, 
	SAVE_FUNCS, 
	SAVE_TERMINAL, 
	SAVE_SET, 
	SAVE_VARS 
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
    S_BOXWIDTH, 
	S_CLABEL, 
	S_CLIP, 
	S_CNTRPARAM, 
	S_CNTRLABEL, 
	S_CONTOUR, 
    S_COLOR, 
	S_COLORSEQUENCE, 
	S_DASHTYPE, 
	S_DATA, 
	S_DATAFILE,
    S_FUNCTIONS, 
	S_DGRID3D, 
	S_DUMMY, 
	S_ENCODING, 
	S_DECIMALSIGN, 
	S_FIT,
    S_FONTPATH, 
	S_FORMAT,
    S_GRID, 
	S_HIDDEN3D, 
	S_HISTORY, 
	S_HISTORYSIZE, 
	S_ISOSAMPLES, 
	S_JITTER, 
	S_KEY,
    S_LABEL, 
	S_LINK, 
	S_NONLINEAR,
    S_LINESTYLE, 
	S_LINETYPE, 
	S_LOADPATH, 
	S_LOCALE, 
	S_LOGSCALE, 
	S_MACROS,
    S_MAPPING, 
	S_MARGIN, 
	S_LMARGIN, 
	S_RMARGIN, 
	S_TMARGIN, 
	S_BMARGIN, 
	S_MISSING,
#ifdef USE_MOUSE
    S_MOUSE,
#endif
    S_MONOCHROME, 
	S_MULTIPLOT, 
	S_MX2TICS, 
	S_NOMX2TICS, 
	S_MXTICS, 
	S_NOMXTICS,
    S_MY2TICS, 
	S_NOMY2TICS, 
	S_MYTICS, 
	S_NOMYTICS,
    S_MZTICS, 
	S_NOMZTICS, 
	S_MRTICS, 
	S_NOMRTICS,
    S_OFFSETS, 
	S_ORIGIN, 
	SET_OUTPUT, 
	S_PARAMETRIC,
    S_PALETTE, 
	S_PM3D, 
	S_COLORBOX, 
	S_COLORNAMES,
    S_CBLABEL, 
	S_CBRANGE, 
	S_CBTICS, 
	S_NOCBTICS, 
	S_MCBTICS, 
	S_NOMCBTICS,
    S_CBDATA, 
	S_CBDTICS, 
	S_NOCBDTICS, 
	S_CBMTICS, 
	S_NOCBMTICS, 
	S_OBJECT,
    S_PLOT, 
	S_POINTINTERVALBOX, 
	S_POINTSIZE, 
	S_POLAR, 
	S_PRINT, 
	S_PSDIR,
    S_SAMPLES, 
	S_SIZE, 
	S_SURFACE, 
	S_STYLE, 
    S_TABLE, 
	S_TERMINAL, 
	S_TERMOPTIONS,
    S_TICS, 
	S_TICSCALE, 
	S_TICSLEVEL, 
	S_TIMEFMT, 
	S_TIMESTAMP, 
	S_TITLE,
    S_TRANGE, 
	S_URANGE, 
	S_VARIABLES, 
	S_VERSION, 
	S_VIEW, 
	S_VRANGE,

    S_X2DATA, 
	S_X2DTICS, 
	S_NOX2DTICS, 
	S_X2LABEL, 
	S_X2MTICS, 
	S_NOX2MTICS,
    S_X2RANGE, 
	S_X2TICS, 
	S_NOX2TICS,
    S_XDATA, 
	S_XDTICS, 
	S_NOXDTICS, 
	S_XLABEL, 
	S_XMTICS, 
	S_NOXMTICS, 
	S_XRANGE,
    S_XTICS, 
	S_NOXTICS, 
	S_XYPLANE,

    S_Y2DATA, 
	S_Y2DTICS, 
	S_NOY2DTICS, 
	S_Y2LABEL, 
	S_Y2MTICS, 
	S_NOY2MTICS,
    S_Y2RANGE, 
	S_Y2TICS, 
	S_NOY2TICS,
    S_YDATA, 
	S_YDTICS, 
	S_NOYDTICS, 
	S_YLABEL, 
	S_YMTICS, 
	S_NOYMTICS, 
	S_YRANGE,
    S_YTICS, 
	S_NOYTICS,

    S_ZDATA, 
	S_ZDTICS, 
	S_NOZDTICS, 
	S_ZLABEL, 
	S_ZMTICS, 
	S_NOZMTICS, 
	S_ZRANGE,
    S_ZTICS, 
	S_NOZTICS,

    S_RTICS, 
	S_NORTICS, 
	S_RRANGE, 
	S_RAXIS, 
	S_PAXIS,

    S_ZERO, 
	S_ZEROAXIS, 
	S_XZEROAXIS, 
	S_X2ZEROAXIS, 
	S_YZEROAXIS, 
	S_Y2ZEROAXIS,
    S_ZZEROAXIS
};

enum set_hidden3d_id {
    S_HI_INVALID,
    S_HI_DEFAULTS, 
	S_HI_OFFSET, 
	S_HI_NOOFFSET, 
	S_HI_TRIANGLEPATTERN,
    S_HI_UNDEFINED, 
	S_HI_NOUNDEFINED, 
	S_HI_ALTDIAGONAL, 
	S_HI_NOALTDIAGONAL,
    S_HI_BENTOVER, 
	S_HI_NOBENTOVER,
    S_HI_FRONT, 
	S_HI_BACK
};

enum set_key_id {
    S_KEY_INVALID,
    S_KEY_TOP, 
	S_KEY_BOTTOM, 
	S_KEY_LEFT, 
	S_KEY_RIGHT, 
	S_KEY_CENTER,
    S_KEY_VERTICAL, 
	S_KEY_HORIZONTAL, 
	S_KEY_OVER, 
	S_KEY_UNDER, 
	S_KEY_MANUAL,
    S_KEY_INSIDE, 
	S_KEY_OUTSIDE, 
	S_KEY_ABOVE, 
	S_KEY_BELOW,
    S_KEY_TMARGIN, 
	S_KEY_BMARGIN, 
	S_KEY_LMARGIN, 
	S_KEY_RMARGIN,
    S_KEY_LLEFT, 
	S_KEY_RRIGHT, 
	S_KEY_REVERSE, 
	S_KEY_NOREVERSE,
    S_KEY_INVERT, 
	S_KEY_NOINVERT,
    S_KEY_ENHANCED, 
	S_KEY_NOENHANCED,
    S_KEY_BOX, 
	S_KEY_NOBOX, 
	S_KEY_SAMPLEN, 
	S_KEY_SPACING, 
	S_KEY_WIDTH,
    S_KEY_HEIGHT, 
	S_KEY_TITLE, 
	S_KEY_NOTITLE,
    S_KEY_FONT, 
	S_KEY_TEXTCOLOR,
    S_KEY_AUTOTITLES, 
	S_KEY_NOAUTOTITLES,
    S_KEY_DEFAULT, 
	S_KEY_ON, 
	S_KEY_OFF,
    S_KEY_MAXCOLS, 
	S_KEY_MAXROWS,
    S_KEY_FRONT, 
	S_KEY_NOFRONT
};

enum set_colorbox_id {
    S_COLORBOX_INVALID,
    S_COLORBOX_VERTICAL, 
	S_COLORBOX_HORIZONTAL,
    S_COLORBOX_DEFAULT, 
	S_COLORBOX_USER,
    S_COLORBOX_BORDER, 
	S_COLORBOX_BDEFAULT, 
	S_COLORBOX_NOBORDER,
    S_COLORBOX_ORIGIN, 
	S_COLORBOX_SIZE,
    S_COLORBOX_FRONT, 
	S_COLORBOX_BACK
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
    S_PM3D_CLIP_1IN, 
	S_PM3D_CLIP_4IN,
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
    SHOW_STYLE_DATA, 
	SHOW_STYLE_FUNCTION, 
	SHOW_STYLE_LINE,
    SHOW_STYLE_FILLING, 
	SHOW_STYLE_ARROW, 
    SHOW_STYLE_CIRCLE, 
	SHOW_STYLE_ELLIPSE, 
	SHOW_STYLE_RECTANGLE,
    SHOW_STYLE_INCREMENT, 
	SHOW_STYLE_HISTOGRAM, 
	SHOW_STYLE_BOXPLOT,
    SHOW_STYLE_PARALLEL
#ifdef EAM_BOXED_TEXT
    , SHOW_STYLE_TEXTBOX
#endif
};

enum filledcurves_opts_id {
    FILLEDCURVES_CLOSED,
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
    FILLEDCURVES_BETWEEN
};

//int lookup_table(const GenTable *, int);
parsefuncp_t lookup_ftable(const GpGenFTable *, int);
int lookup_table_entry(const GenTable *tbl, const char *search_str);
int lookup_table_nth(const GenTable *tbl, const char *search_str);
int lookup_table_nth_reverse(const GenTable *tbl, int table_len, const char *search_str);
const char * reverse_table_lookup(const GenTable *tbl, int entry);
//
//
//
struct dynarray {
	dynarray()
	{
		size = 0;
		end = 0;
		increment = 0;
		entry_size = 0;
		v = 0;
	}
	void   Init(size_t entrySize, long sz, long incr);
	void   Destroy()
	{
		ZFREE(v); // should work, even if malloc failed
		end = size = 0;
	}
	// Set the size of the dynamical array to a new, fixed value
	void   Resize(long newsize);
	void * GetNext();

    long   size;       // alloced size of the array
    long   end;        // index of first unused entry
    long   increment;  // amount to increment size by, on realloc
    size_t entry_size; // size of the entries in this array
    void * v;          // the vector itself
};
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
    ASSIGN,
    /* only jump operators go between jump and sf_start, for is_jump() */
    JUMP,
	JUMPZ,
	JUMPNZ,
	JTERN,
	SF_START,
    /* External function call */
#ifdef HAVE_EXTERNAL_FUNCTIONS
    CALLE,
#endif
    // functions specific to using spec
    COLUMN, 
	STRINGCOLUMN
};

#define is_jump(op) ((op) >=(int)JUMP && (op) <(int)SF_START)
#define STACK_DEPTH 250		/* maximum size of the execution stack */
#define MAX_AT_LEN 150		/* max number of entries in action table */
//
// p-code argument
//
union GpArgument {
	int    j_arg;   // offset for jump
	t_value v_arg;  // constant value
	UdvtEntry * udv_arg; // pointer to dummy variable
	UdftEntry * udf_arg; // pointer to udf to execute
#ifdef HAVE_EXTERNAL_FUNCTIONS
	struct exft_entry *exf_arg; /* pointer to external function */
#endif
};
//
// action table entry
//
struct AtEntry {
    enum operators Index;	/* index of p-code function */
    GpArgument arg;
};

struct AtType {
	static void Destroy(AtType * at_ptr);
	AtType()
	{
		a_count = 0;
		memzero(actions, sizeof(actions));
	}
    int    a_count; // count of entries in .actions[] 
	AtEntry actions[MAX_AT_LEN]; // will usually be less than MAX_AT_LEN is malloc()'d copy
};
//
// user-defined function table entry
//
struct UdftEntry {
	UdftEntry()
	{
		THISZERO();
	}
    UdftEntry * next_udf; // pointer to next udf in linked list
    char * udf_name;   // name of this function entry
    AtType * at;       // pointer to action table to execute
    char * definition; // definition of function as typed
    int    dummy_num;  // required number of input variables
    t_value dummy_values[MAX_NUM_VAR]; // current value of dummy variables
};
//
// PARSE.H
//
struct t_iterator {
	t_iterator * next;  // doubly linked list
	t_iterator * prev;
	UdvtEntry *iteration_udv;
	char * iteration_string;
	int    iteration_start;
	int    iteration_end;
	int    iteration_increment;
	int    iteration_current;
	int    iteration;
	bool   done;
	bool   really_done;
	bool   empty_iteration;
};

class GpParser {
public:
	GpParser()
	{
		for(uint i = 0; i < MAX_NUM_VAR; i++) {
			SetDummyVar[i][0] = 0;
		}
		STRNSCPY(SetDummyVar[0], "x");
		STRNSCPY(SetDummyVar[1], "y");
		MEMSZERO(FitDummyVar);
		MEMSZERO(CDummyVar);
		AtHighestColumnUsed = 0;
		P_DfArray = 0;
		P_PlotIterator = 0;
		P_SetIterator = 0;
		IsScanningRangeInProgress = false;
		Parse1stRowAsHeaders = false;
		IsStringResultOnly = false;

		parse_recursion_level = 0;
		P_At = NULL;
		AtSize = 0;
	}
	t_value * ConstExpress(GpCommand & rC, t_value * valptr);
	t_value * ConstStringExpress(t_value * valptr);
	char * StringOrExpress(GpCommand & rC, AtType ** atptr);
	AtType * PermAt();
	AtType * TempAt();
	void   ExtendAt();
	GpArgument * AddAction(enum operators sf_index);

	void   CleanupSetIterator();
	void   CleanupPlotIterator();

	void   ParseExpression(GpCommand & rC);
	void   ParsePrimaryExpression(GpCommand & rC);
	void   ParseUnaryExpression(GpCommand & rC);
	void   ParseMultiplicativeExpression(GpCommand & rC);
	int    ParseAssignmentExpression(GpCommand & rC);
	int    ParseArrayAssignmentExpression(GpCommand & rC);
	void   ParseAdditiveExpression(GpCommand & rC);
	void   ParseBitshiftExpression(GpCommand & rC);
	void   ParseRelationalExpression(GpCommand & rC);
	void   ParseEqualityExpression(GpCommand & rC);
	void   ParseConditionalExpression(GpCommand & rC);
	void   ParseLogicalOrExpression(GpCommand & rC);
	void   ParseLogicalAndExpression(GpCommand & rC);
	void   ParseExclusiveOrExpression(GpCommand & rC);
	void   ParseInclusiveOrExpression(GpCommand & rC);
	void   ParseAndExpression(GpCommand & rC);
	void   ParseSumExpression(GpCommand & rC);

	void   ParseResetAfterError();

	void   AcceptMultiplicativeExpression(GpCommand & rC);
	void   AcceptAdditiveExpression(GpCommand & rC);
	void   AcceptBitshiftExpression(GpCommand & rC);
	void   AcceptRelationalExpression(GpCommand & rC);
	void   AcceptEqualityExpression(GpCommand & rC);
	void   AcceptLogicalOrExpression(GpCommand & rC);
	void   AcceptLogicalAndExpression(GpCommand & rC);
	void   AcceptAndExpression(GpCommand & rC);
	void   AcceptExclusiveOrExpression(GpCommand & rC);
	void   AcceptInclusiveOrExpression(GpCommand & rC);

	void   SetupColumnheaderParsing(const AtEntry & rPrevious);
	//
	char   SetDummyVar[MAX_NUM_VAR][MAX_ID_LEN+1]; // The choice of dummy variables, as set by 'set dummy',
		// 'set polar' and 'set parametric'
	int    FitDummyVar[MAX_NUM_VAR]; // Dummy variables referenced by name in a fit command
		// Sep 2014 (DEBUG) used to deduce how many independent variables
	// the currently used 'dummy' variables. Usually a copy of
	// GpGg.Gp__C.P.SetDummyVar, but may be changed by the '(s)plot' command
	// containing an explicit range (--> 'plot [phi=0..pi]')
	char   CDummyVar[MAX_NUM_VAR][MAX_ID_LEN+1];
	int    AtHighestColumnUsed; // This is used by plot_option_using()
	UdvtEntry * P_DfArray; // This is used by df_open() and df_readascii()
	t_iterator * P_PlotIterator; // Used for plot and splot
	t_iterator * P_SetIterator;  // Used by set/unset commands
	bool IsScanningRangeInProgress;
	bool Parse1stRowAsHeaders; // This is checked by df_readascii()
	bool IsStringResultOnly;
private:
	int    parse_recursion_level;
	AtType * P_At;
	int    AtSize;
};
//
// Prototypes of exported functions in parse.c
//
AtType * create_call_column_at(char *);
//UdvtEntry * add_udv(int t_num);
//UdftEntry * add_udf(int t_num);
void   cleanup_udvlist();
//
// These are used by the iteration code
//
//t_iterator * check_for_iteration();
bool next_iteration(t_iterator *);
bool empty_iteration(t_iterator *);
bool forever_iteration(t_iterator *);
t_iterator * cleanup_iteration(t_iterator *);
//
// COMMAND.H
//
struct LexicalUnit {   // produced by scanner 
    bool is_token;  // true if token, false if a value 
    t_value l_val;
    int    start_index; // index of first char in token 
    int    length;      // length of token in chars 
};

//
// Used by terminals and by shared routine parse_term_size() 
//
enum size_units {
	PIXELS,
	INCHES,
	CM
};
//
// custom dash pattern definition modeled after SVG terminal,
// but string specifications like "--.. " are also allowed and stored
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

struct GpHistEntry {
    char * line;
    GpHistEntry * prev;
    GpHistEntry * next;
};

class GpHistory {
public:
	GpHistory()
	{
		history = NULL;
		cur_entry = NULL;
		gnuplot_history_size = HISTORY_SIZE;
		history_quiet = false;
		history_full = false;
	}
	void   AddHistory(char * line);
	void   WriteHistory(char * filename);
	void   WriteHistoryN(const int n, const char * filename, const char * mode);
	void   WriteHistoryList(const int num, const char * const filename, const char * mode);
	void   ReadHistory(char * filename);
	const char * HistoryFindByNumber(int n);
	const char * HistoryFind(char * cmd);
	int    HistoryFindAll(char * cmd);

	GpHistEntry * history;
	GpHistEntry * cur_entry;
	int    gnuplot_history_size;
	bool   history_quiet;
	bool   history_full;
};

class GpCommand {
public:
	void   BreakCommand();
	void   ContinueCommand();
	void   DoCommand();
	void   WhileCommand();
	void   IfCommand();
	void   ElseCommand();
	void   ExitCommand();
	void   BeginClause();
	void   EndClause();
	void   ArrayCommand();
	void   CallCommand();

	GpCommand(GpGadgets & rGg) : R_Gg(rGg)
	{
		P_InputLine = 0;
		InputLineLen = 0;
		InlineNum = 0;
		IfDepth = 0;
		IfOpenForElse = false;
		IfCondition = false;
		IsReplotDisabled = false;
		P_Token = 0;
		TokenTableSize = 0;
		PlotToken = 0;
		P_ReplotLine = 0;

		F_PrintOut = 0;
		P_PrintOutVar = 0;
		P_PrintOutName = 0;
		P_DummyFunc = 0;
		NumTokens = 0;
		CToken = 0;
		//
		clause_depth = 0;
		iteration_depth = 0;
		requested_break = false;
		requested_continue = false;
		command_exit_status = 0;
		//
		splot_map_active = 0;
		splot_map_surface_rot_x = 0.0f;
		splot_map_surface_rot_z = 0.0f;
		splot_map_surface_scale = 0.0f;
	}
	char * ReadLine(const char * prompt);
	char * ReadLine_ipc(const char* prompt);
	char * RlGets(char * s, size_t n, const char * prompt);
	int    LookupTable(const GenTable * pTbl, int findToken);
	int    Scanner(char ** expressionp, size_t * expressionlenp);
	t_value * ConstExpress(t_value * valptr)
	{
		return P.ConstExpress(*this, valptr);
	}
	double RealExpression();
	int    IntExpression();
	bool   IterationEarlyExit() const
	{
		return (requested_continue || requested_break);
	}
	int    ReadLine(const char * prompt, int start);
	//
	// length of token string 
	//
	size_t TokenLen(int t_num) const
	{
		return (size_t)(P_Token[t_num].length);
	}
	int    DoLine(); // do_line()
	void   Command();
	void   Define();
	void   Convert(t_value * pValue, int t_num);
	void   ExpandCallArgs();
	int    IsDefinition(/*int t_num*/) const;
	bool   IsArrayAssignment();
	int    IsString(int t_num) const;
	int    IsLetter(int t_num) const;
	int    IsANumber(int t_num) const;
	int    TypeDdv(int t_num);
	//#define isstringvalue(_tok) (GpGg.Gp__C.IsString(_tok) || GpGg.Gp__C.TypeDdv(_tok)==STRING)
	int    IsStringValue(int t_num)
	{
		return (IsString(t_num) || TypeDdv(t_num) == STRING);
	}
	void   CopyStr(char * str, int t_num, int max);
	char * TryToGetString();
	int    TryToGetString(SString & rBuf);
	void   Capture(char * str, int start, int end, int max);
	void   MCapture(char ** str, int start, int end);
	void   MQuoteCapture(char ** str, int start, int end);
	int    EndOfCommand() const
	{
		return (CToken >= NumTokens || Eq(";"));
	}
	void   InitMemory();
	void   ExtendInputLine();
	void   ExtendTokenTable();
	//
	// AlmostEq() compares string value of token number CToken with str[], and returns true if 
	// they are identical up to the first $ in str[].
	//
	int AlmostEq(const char * str) const
	{
		return AlmostEq(CToken, str);
	}
	int AlmostEq(int t_num, const char * str) const
	{
		if(!str || t_num < 0 || t_num >= NumTokens || !P_Token[t_num].is_token)
			return false;
		else {
			const  int length = P_Token[t_num].length;
			int    start = P_Token[t_num].start_index;
			int    after = 0;
			int    i;
			for(i = 0; i < length + after; i++) {
				if(str[i] != P_InputLine[start + i]) {
					if(str[i] != '$')
						return (false);
					else {
						after = 1;
						start--; // back up token ptr
					}
				}
			}
			// i now beyond end of token string
			return (after || str[i] == '$' || str[i] == NUL);
		}
	}
	//
	// GpGg.Gp__C.Eq() compares string value of token number t_num with str[], and returns true if they are identical.
	//
	int    Eq(const char * str) const;
	int    Eq(int t_num, const char * str) const;
	void   EnsureOutput();
	void   ParseColorSpec(t_colorspec * tc, int options);
	void   ParseFillStyle(fill_style_type * fs, int def_style, int def_density, int def_pattern, t_colorspec def_bordertype);
	int    ParseDashType(t_dashtype * pDashType);
	void   ParseSkipRange();
	char * ParseDataBlockName();
	void   ClauseResetAfterError();
	void   DoString(const char * s);
	void   DoStringAndFree(char * cmdline);
	void   PrintSetOutput(char * name, bool datablock, bool append_p);
	char * PrintShowOutput();
	void   ShowPrint();
	void   RaiseLowerCommand(int lower);
	void   ImportCommand();
	size_units ParseTermSize(float * pXSize, float * pYSize, size_units defaultUnits);
	void   ParseLinkVia(UdftEntry * pUdf);
	t_iterator * CheckForIteration();
	int    GetNum(char str[]);
	void   Substitute(char ** ppStr, size_t * pStrLen, int curChrP);
	int    Expand1LevelMacros();
	void   StringExpandMacros();
	int    Token2Tuple(double * tuple, int dimension);
	AtType * ExternalAt(const char * pFuncName);
	void   ShowHistory();
	void   ShowDummy();

	void   PrintMessageToStderr();
	void   PrintSpacesUpToToken(int tokNum);

	char * P_InputLine;
	size_t InputLineLen;
	int    InlineNum;
	int    IfDepth;         // old if/else syntax only
	bool   IfOpenForElse;   // new if/else syntax only 
	bool   IfCondition;     // used by both old and new syntax
	bool   IsReplotDisabled; // flag to disable `replot` when some data are sent through stdin; used by mouse/hotkey capable terminals 
	LexicalUnit * P_Token;
	int    TokenTableSize;
	int    PlotToken;
	char * P_ReplotLine;
	FILE * F_PrintOut; // output file for the print command 
	UdvtEntry * P_PrintOutVar;
	char * P_PrintOutName;
	UdftEntry * P_DummyFunc;
	// input data, parsing variables 
	int    NumTokens;
	int    CToken;
	//
	int    clause_depth;
	// support for 'break' and 'continue' commands
	int    iteration_depth;
	bool   requested_break;
	bool   requested_continue;
	int    command_exit_status;
	//
	//
	// Is 'set view map' currently working inside 'splot' or not? Calculation of
	// mouse coordinates and the corresponding routines must know it, because
	// 'splot' can be either true 3D plot or a 2D map.
	// This flag is set when entering splot command and 'set view map', i.e. by
	// splot_map_activate(), and reset when calling splot_map_deactivate().
	// 
	int    splot_map_active;
	// 
	// Store values reset by 'set view map' during splot, used by those two routines below.
	// 
	float  splot_map_surface_rot_x;
	float  splot_map_surface_rot_z;
	float  splot_map_surface_scale;
	//
	//
	//
	GpParser P;
	GpHistory H;
private:
	int    FindClause(int * pClauseStart, int * pClauseEnd);

	GpGadgets & R_Gg;
};

//extern GpCommand GpGg.Gp__C;

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
#ifndef STDOUT
	#define STDOUT 1
#endif
#ifdef _Windows
	#define SET_CURSOR_WAIT SetCursor(LoadCursor((HINSTANCE) NULL, IDC_WAIT))
	#define SET_CURSOR_ARROW SetCursor(LoadCursor((HINSTANCE) NULL, IDC_ARROW))
#else
	#define SET_CURSOR_WAIT        /* nought, zilch */
	#define SET_CURSOR_ARROW       /* nought, zilch */
#endif
// Include code to support deprecated "call" syntax. 
#ifdef BACKWARDS_COMPATIBLE
	#define OLD_STYLE_CALL_ARGS
#endif
#ifdef X11
	extern void x11_raise_terminal_window(int);
	extern void x11_raise_terminal_group();
	extern void x11_lower_terminal_window(int);
	extern void x11_lower_terminal_group();
#endif
#ifdef _Windows
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
//extern void string_expand_macros();
#ifdef USE_MOUSE
	//void bind_command();
	void restore_prompt();
#else
	//#define bind_command()
#endif
//void changedir_command();
void help_command(GpCommand & rC);
void history_command(GpCommand & rC);
void invalid_command();
void null_command();
void print_command(GpCommand & rC);
void pwd_command();
//void refresh_command();
void reread_command();
//void save_command(GpCommand & rC);
void screendump_command();
void stats_command();
void system_command();
void do_shell();
void undefine_command(GpCommand & rC);
//
// Prototypes for functions exported by command.c 
//
//int com_line();
#ifdef USE_MOUSE
	void toggle_display_of_ipc_commands();
	int display_ipc_commands();
#endif
#ifdef VMS                     /* HBB 990829: used only on VMS */
	void done(int status);
#endif

//void define();
//void replotrequest(); /* used in command.c & mouse.c */
//void print_set_output(char *, bool, bool); /* set print output file */
//char *print_show_output(); /* show print output file */
/* Activate/deactive effects of 'set view map' before 'splot'/'plot',
 * respectively. Required for proper mousing during 'set view map';
 * actually it won't be necessary if gnuplot keeps a copy of all variables for
 * the current plot and don't share them with 'set' commands.
 *   These routines need to be executed before each plotrequest() and
 * plot3drequest().
 */
//void splot_map_activate();
//void splot_map_deactivate();
int do_system_func(const char *cmd, char **output);
//
// Constants that are interpreted by terminal driver routines
//
// Default line type is LT_BLACK; reset to this after changing colors
//
#define LT_AXIS       (-1)
#define LT_BLACK      (-2)		/* Base line type */
#define LT_SOLID      (-2)		/* Synonym for base line type */
#define LT_NODRAW     (-3)
#define LT_BACKGROUND (-4)
#define LT_UNDEFINED  (-5)
#define LT_COLORFROMCOLUMN  (-6)	/* Used by hidden3d code */
#define LT_DEFAULT    (-7)
//
// Pre-defined dash types
//
#define DASHTYPE_CUSTOM (-3)
#define DASHTYPE_AXIS   (-2)
#define DASHTYPE_SOLID  (-1)
// more...?

#define PT_CHARACTER  (-9) // magic point type that indicates a character rather than a predefined symbol
#define PT_VARIABLE   (-8) // magic point type that indicates true point type comes from a data column
//
// Constant value passed to (term->text_angle)(ang) to generate vertical
// text corresponding to old keyword "rotate", which produced the equivalent
// of "rotate by 90 right-justified".
//
#define TEXT_VERTICAL (-270)
//
// Type definitions
//
// this order means we can use  x-(just*strlen(text)*t->HChr)/2 if term cannot justify
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

enum t_fillstyle {
	FS_EMPTY,
	FS_SOLID,
	FS_PATTERN,
	FS_DEFAULT,
	FS_TRANSPARENT_SOLID,
	FS_TRANSPARENT_PATTERN
};

#define TC_DEFAULT      0       /* Use default color, set separately */
#define TC_LT           1       /* Use the color of linetype <n> */
#define TC_LINESTYLE    2       /* Use the color of line style <n> */
#define TC_RGB          3       /* Explicit RGB triple provided by user */
#define TC_CB           4       /* "palette cb <value>" */
#define TC_FRAC         5       /* "palette frac <value> */
#define TC_Z            6       /* "palette z" */
#define TC_VARIABLE     7       /* only used for "tc", never "lc" */

#define DEFAULT_DASHPATTERN {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0,0,0,0,0,0,0,0}}
//
// Default point size is taken from the global "pointsize" variable
//
#define PTSZ_DEFAULT    (-2)
#define PTSZ_VARIABLE   (-3)
#define AS_VARIABLE	(-3)
//
// Generalized pm3d-compatible color specifier
// Supplements basic linetype choice
//
struct t_colorspec {
	t_colorspec()
	{
		SetDefault();
	}
	t_colorspec(int csType, int csLt, double csValue)
	{
		type = csType;
		lt = csLt;
		value = csValue;
	}
	void   Set(int csType, int csLt, double csValue)
	{
		type = csType;
		lt = csLt;
		value = csValue;
	}
	// #define DEFAULT_COLORSPEC {TC_DEFAULT, 0, 0.0}
	// #define BLACK_COLORSPEC {TC_LT, LT_BLACK, 0.0}
	// #define BACKGROUND_COLORSPEC {TC_LT, LT_BACKGROUND, 0.0}
	void   SetDefault()
	{
		type = TC_DEFAULT;
		lt = 0;
		value = 0.0;
	}
	void   SetBlack()
	{
		type = TC_LT;
		lt = LT_BLACK;
		value = 0.0;
	}
	void   SetBackground()
	{
		type = TC_LT;
		lt = LT_BACKGROUND;
		value = 0.0;
	}
	//#define TC_USES_PALETTE(tctype) oneof3(tctype, TC_Z, TC_CB, TC_FRAC)
	int    UsesPalette() const
	{
		return oneof3(type, TC_Z, TC_CB, TC_FRAC);
	}
	int    type;  // TC_<type> definitions below
	int    lt;    // used for TC_LT, TC_LINESTYLE and TC_RGB
	double value; // used for TC_CB and TC_FRAC
};

struct lp_style_type {	/* contains all Line and Point properties */
	enum {
		defCommon = 0,
		defBkg,
		defZeroAxis,
		defGrid,
		defParallelAxis,
		defBorder
	};
	lp_style_type(int def = defCommon)
	{
		if(def == defBkg)
			SetDefault();
		else if(def == defZeroAxis) {
			// #define DEFAULT_AXIS_ZEROAXIS {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}
			flags = 0;
			l_type = LT_AXIS;
			p_type = 0;
			d_type = DASHTYPE_AXIS;
			p_interval = 0;
			l_width = 1.0;
			p_size = PTSZ_DEFAULT;
			MEMSZERO(p_char);
			pm3d_color.SetBlack();
			custom_dash_pattern.SetDefault();
		}
		else if(def == defParallelAxis) {
			//#define DEFAULT_PARALLEL_AXIS_STYLE {{0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 2.0, 0.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, LAYER_FRONT }
			flags = 0;
			l_type = LT_BLACK;
			p_type = 0;
			d_type = DASHTYPE_SOLID;
			p_interval = 0;
			l_width = 2.0;
			p_size = 0.0;
			MEMSZERO(p_char);
			pm3d_color.SetBlack();
			custom_dash_pattern.SetDefault();
		}
		else if(def == defGrid) {
			//#define DEFAULT_GRID_LP {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 0.5, 0.0, DEFAULT_P_CHAR, {TC_LT, LT_AXIS, 0.0}, DEFAULT_DASHPATTERN}
			flags = 0;
			l_type = LT_AXIS;
			p_type = 0;
			d_type = DASHTYPE_AXIS;
			p_interval = 0;
			l_width = 0.5;
			p_size = 0.0;
			MEMSZERO(p_char);
			pm3d_color.Set(TC_LT, LT_AXIS, 0.0);
			custom_dash_pattern.SetDefault();
		}
		else if(def == defBorder) {
			//#define DEFAULT_BORDER_LP { 0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 1.0, 1.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN }
			flags = 0;
			l_type = LT_BLACK;
			p_type = 0;
			d_type = DASHTYPE_SOLID;
			p_interval = 0;
			l_width = 1.0;
			p_size = 1.0;
			MEMSZERO(p_char);
			pm3d_color.SetBlack();
			custom_dash_pattern.SetDefault();
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
		l_width = 1.0;
		p_size = 0.0;
		MEMSZERO(p_char);
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
		l_width = 1.0;
		p_size = PTSZ_DEFAULT;
		MEMSZERO(p_char);
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
		l_width = 1.0;
		p_size = PTSZ_DEFAULT;
		MEMSZERO(p_char);
		pm3d_color.SetBlack();
		custom_dash_pattern.SetDefault();
	}
    int     flags;		/* e.g. LP_SHOW_POINTS */
    int     l_type;
    int	    p_type;
    int     d_type;		/* Dashtype */
    int     p_interval;		/* Every Nth point in style LINESPOINTS */
    double  l_width;
    double  p_size;
    char    p_char[8];		/* string holding UTF-8 char used if p_type = PT_CHARACTER */
    t_colorspec pm3d_color;
    t_dashtype custom_dash_pattern;	/* per-line, user defined dashtype */
    /* ... more to come ? */
};

// #define DEFAULT_P_CHAR {0,0,0,0,0,0,0,0}
// #define DEFAULT_LP_STYLE_TYPE {0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, DEFAULT_COLORSPEC, DEFAULT_DASHPATTERN}
//
// Bit definitions for lp_style_type.flags
//
#define LP_SHOW_POINTS     (0x1) // if not set, ignore the point properties of this line style
#define LP_NOT_INITIALIZED (0x2) // internal flag used in set.c:parse_label_options
#define LP_EXPLICIT_COLOR  (0x4) // set by lp_parse if the user provided a color spec
#define LP_ERRORBAR_SET    (0x8) // set by "set errorbars <lineprops>

#define DEFAULT_COLOR_SEQUENCE { 0x9400d3, 0x009e73, 0x56b4e9, 0xe69f00, 0xf0e442, 0x0072b2, 0xe51e10, 0x000000 }
#define PODO_COLOR_SEQUENCE { 0x000000, 0xe69f00, 0x56b4e9, 0x009e73, 0xf0e442, 0x0072b2, 0xd55e00, 0xcc79a7 }

#define DEFAULT_MONO_LINETYPES { \
	{0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 1.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
	{0, LT_BLACK, 0, 1 /* dt 2 */, 0, 1.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
	{0, LT_BLACK, 0, 2 /* dt 3 */, 0, 1.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
	{0, LT_BLACK, 0, 3 /* dt 4 */, 0, 1.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
	{0, LT_BLACK, 0, 0 /* dt 1 */, 0, 2.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
	{0, LT_BLACK, 0, DASHTYPE_CUSTOM, 0, 1.2 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, {{16.,8.,2.,5.,2.,5.,2.,8.},{0,0,0,0,0,0,0,0}}} \
}

enum t_arrow_head {
	NOHEAD = 0,
	END_HEAD = 1,
	BACKHEAD = 2,
	BOTH_HEADS = 3
};

extern const char * arrow_head_names[4];

enum arrowheadfill {
	AS_NOFILL = 0,
	AS_EMPTY,
	AS_FILLED,
	AS_NOBORDER
};

struct arrow_style_type {    /* contains all Arrow properties */
    int    tag;   // -1 (local), AS_VARIABLE, or style index
    int    layer; // 0 = back, 1 = front
    lp_style_type lp_properties;
	//
    // head options
	//
    t_arrow_head head;               /* arrow head choice */
    /* GpPosition headsize; */  /* x = length, y = angle [deg] */
    double head_length;              /* length of head, 0 = default */
    int    head_lengthunit;             /* unit (x1, x2, screen, graph) */
    double head_angle;               /* front angle / deg */
    double head_backangle;           /* back angle / deg */
    arrowheadfill headfill;	     /* AS_FILLED etc */
    bool head_fixedsize;         /* Adapt the head size for short arrow shafts? */
    /* ... more to come ? */
};
//
// Operations used by the terminal entry point term->layer()
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
	TERM_LAYER_BEGIN_KEYSAMPLE,
	TERM_LAYER_END_KEYSAMPLE,
	TERM_LAYER_RESET_PLOTNO,
	TERM_LAYER_BEFORE_ZOOM,
	TERM_LAYER_BEGIN_PM3D_MAP,
	TERM_LAYER_END_PM3D_MAP,
	TERM_LAYER_BEGIN_IMAGE,
	TERM_LAYER_END_IMAGE,
	TERM_LAYER_3DPLOT
};
//
// Options used by the terminal entry point term->waitforinput()
//
#define TERM_ONLY_CHECK_MOUSING	1
#define TERM_EVENT_POLL_TIMEOUT 0 // select() timeout in usec
//
// Options used by the terminal entry point term->hypertext().
//
#define TERM_HYPERTEXT_TOOLTIP 0
#define TERM_HYPERTEXT_TITLE   1

struct fill_style_type {
	fill_style_type(int style, int density, int pattern, t_colorspec borderCs)
	{
		fillstyle = style;
		filldensity = density;
		fillpattern = pattern;
		border_color = borderCs;
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

#ifdef EAM_BOXED_TEXT
/* Options used by the terminal entry point term->boxed_text() */
enum t_textbox_options {
	TEXTBOX_INIT = 0,
	TEXTBOX_OUTLINE,
	TEXTBOX_BACKGROUNDFILL,
	TEXTBOX_MARGINS,
	TEXTBOX_FINISH,
	TEXTBOX_GREY
};
#endif

#define FS_OPAQUE (FS_SOLID + (100<<4))
//
// Color construction for an image, palette lookup or rgb components
//
enum t_imagecolor {
	IC_PALETTE,
	IC_RGB,
	IC_RGBA
};
//
// Contains a colour in RGB scheme.
// Values of  r, g and b  are all in range [0;1]
//
struct rgb_color {
	rgb_color & Set(double _r, double _g, double _b)
	{
		R = _r;
		G = _g;
		B = _b;
		return *this;
	}
	rgb_color & SetGray(double _v)
	{
		R = _v;
		G = _v;
		B = _v;
		return *this;
	}
	int    IsEqual(const rgb_color & rS) const
	{
		return BIN(R == rS.R && G == rS.G && B == rS.B);
	}
	rgb_color & Constrain()
	{
		RealRange c;
		c.Set(0.0, 1.0);
		R = c.Clip(R);
		G = c.Clip(G);
		B = c.Clip(B);
		return *this;
	}
	double R;
	double G;
	double B;
};
//
// Contains a colour in RGB scheme.
// Values of  r, g and b  are uchars in range [0;255]
//
struct rgb255_color {
	uchar r;
	uchar g;
	uchar b;
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
// Declaration of smooth palette, i.e. palette for smooth colours
//
struct t_sm_palette {
	/** Constants: **/

	/* (Fixed) number of formulae implemented for gray index to RGB
	 * mapping in color.c.  Usage: somewhere in `set' command to check
	 * that each of the below-given formula R,G,B are lower than this
	 * value. */
	int    colorFormulae;
	/** Values that can be changed by `set' and shown by `show' commands: **/
	palette_color_mode colorMode; /* can be SMPAL_COLOR_MODE_GRAY or SMPAL_COLOR_MODE_RGB */
	int    formulaR, formulaG, formulaB; /* mapping formulae for SMPAL_COLOR_MODE_RGB */
	char   positive;          /* positive or negative figure */
	/* Now the variables that contain the discrete approximation of the
	 * desired palette of smooth colours as created by make_palette in
	 * pm3d.c.  This is then passed into terminal's make_palette, who
	 * transforms this [0;1] into whatever it supports.  */

	/* Only this number of colour positions will be used even though
	 * there are some more available in the discrete palette of the
	 * terminal.  Useful for multiplot.  Max. number of colours is taken
	 * if this value GpGg.Gp__C.Eq 0.  Unused by: PostScript */
	int    use_maxcolors;
	/* Number of colours used for the discrete palette. Equals to the
	 * result from term->make_palette(NULL), or restricted by
	 * use_maxcolor.  Used by: pm, gif. Unused by: PostScript */
	int    colors;
	/* Table of RGB triplets resulted from applying the formulae. Used
	 * in the 2nd call to term->make_palette for a terminal with
	 * discrete colours. Unused by PostScript which calculates them
	 * analytically. */
	rgb_color * color;
	// Variables used by some terminals 
	//
	// Option unique for output to PostScript file.  By default,
	// ps_allcF=0 and only the 3 selected rgb color formulae are written
	// into the header preceding pm3d map in the file.  If ps_allcF is
	// non-zero, then print there all color formulae, so that it is easy
	// to play with choosing manually any color scheme in the PS file
	// (see the definition of "/g"). Like that you can get the
	// Rosenbrock multiplot figure on my gnuplot.html#pm3d demo page.
	// Note: this option is used by all terminals of the postscript
	// family, i.e. postscript, pslatex, epslatex, so it will not be
	// comfortable to move it to the particular .trm files. 
	//
	bool ps_allcF;

	/* These variables are used to define interpolated color palettes:
	 * gradient is an array if(gray,color) pairs.  This array is
	 * gradient_num entries big.
	 * Interpolated tables are used if colorMode==SMPAL_COLOR_MODE_GRADIENT */
	int    gradient_num;
	gradient_struct * gradient;
	/* Smallest nonzero gradient[i+1] - gradient[i].  If this is < (1/colors)
	 * Then a truncated gray value may miss the gradient it belongs in. */
	double smallest_gradient_interval;
	int    cmodel; /* the used color model: RGB, HSV, XYZ, etc. */
	//
	// Three mapping function for gray->RGB/HSV/XYZ/etc. mapping
	// used if colorMode == SMPAL_COLOR_MODE_FUNCTIONS
	//
	UdftEntry Afunc; /* R for RGB, H for HSV, C for CMY, ... */
	UdftEntry Bfunc; /* G for RGB, S for HSV, M for CMY, ... */
	UdftEntry Cfunc; /* B for RGB, V for HSV, Y for CMY, ... */
	double gamma; /* gamma for gray scale and cubehelix palettes only */
	// control parameters for the cubehelix palette scheme
	double cubehelix_start; /* offset (radians) from colorwheel 0 */
	double cubehelix_cycles; /* number of times round the colorwheel */
	double cubehelix_saturation; /* color saturation */
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
#define TERM_CAN_MULTIPLOT    (1<<0)	/* tested if stdout not redirected */
#define TERM_CANNOT_MULTIPLOT (1<<1)	/* tested if stdout is redirected  */
#define TERM_BINARY           (1<<2)	/* open output file with "b"       */
#define TERM_INIT_ON_REPLOT   (1<<3)	/* call term->init() on replot     */
#define TERM_IS_POSTSCRIPT    (1<<4)	/* post, next, pslatex, etc        */
#define TERM_ENHANCED_TEXT    (1<<5)	/* enhanced text mode is enabled   */
#define TERM_NO_OUTPUTFILE    (1<<6)	/* terminal doesnt write to a file */
#define TERM_CAN_CLIP         (1<<7)	/* terminal does its own clipping  */
#define TERM_CAN_DASH         (1<<8)	/* terminal supports dashed lines  */
#define TERM_ALPHA_CHANNEL    (1<<9)	/* alpha channel transparency      */
#define TERM_MONOCHROME      (1<<10)	/* term is running in mono mode    */
#define TERM_LINEWIDTH       (1<<11)	/* support for set term linewidth  */
#define TERM_FONTSCALE       (1<<12)	/* terminal supports fontscale     */
#define TERM_IS_LATEX        (1<<13)	/* text uses TeX markup            */
#define TERM_EXTENDED_COLOR  (1<<14)	/* uses EXTENDED_COLOR_SPECS       */
#define TERM_NULL_SET_COLOR  (1<<15)	/* no support for RGB color        */
#define TERM_POLYGON_PIXELS  (1<<16)	/* filledpolygon rather than fillbox */

/* The terminal interface structure --- heart of the terminal layer.
 *
 * It should go without saying that additional entries may be made
 * only at the end of this structure. Any fields added must be
 * optional - a value of 0 (or NULL pointer) means an older driver
 * does not support that feature - gnuplot must still be able to
 * function without that terminal feature
 */
struct GpTermEntry {
#if 0 // @construction {	
    virtual void Options(GpCommand & rC)
	{
	}
    virtual void Init()
	{
	}
    virtual void Reset()
	{
	}
    virtual void Text()
	{
	}
    virtual int  Scale(double, double)
	{
		return 0;
	}
    virtual void Graphics()
	{
	}
    virtual void Move(uint, uint)
	{
	}
    virtual void Vector(uint, uint)
	{
	}
	virtual void LineType(int)
	{
	}
    virtual void PutText(uint, uint, const char * pText)
	{
	}
	//
    // the following are optional. set term ensures they are not NULL
	//
    virtual int  TextAngle(int)
	{
		return 0;
	}
    virtual int  JustifyText(enum JUSTIFY)
	{
		return 0;
	}
    virtual void Point(uint, uint, int)
	{
	}
    virtual void Arrow(uint, uint, uint, uint, int)
	{
	}
    virtual int  SetFont(const char *font)
	{
		return 0;
	}
    virtual void PointSize(double) // change pointsize 
	{
	}
    virtual void Suspend() // called after one plot of multiplot 
	{
	}
    virtual void Resume() // called before plots of multiplot 
	{
	}
    virtual void FillBox(int, uint, uint, uint, uint) // clear in multiplot mode 
	{
	}
    virtual void LineWidth(double linewidth)
	{
	}
	virtual int  MakePalette(t_sm_palette * pPalette)
	{
		return 0;
	}
	//
	// 1. if palette==NULL, then return nice/suitable
	// maximal number of colours supported by this terminal.
	// Returns 0 if it can make colours without palette (like postscript).
	// 2. if palette!=NULL, then allocate its own palette return value is undefined
	// 3. available: some negative values of max_colors for whatever can be useful
	//
	virtual void PreviousPalette()
	{
	}
	//
	// release the palette that the above routine allocated and get
	// back the palette that was active before.
	// Some terminals, like displays, may draw parts of the figure
	// using their own palette. Those terminals that possess only
	// one palette for the whole plot don't need this routine.
	//
    virtual void SetColor(t_colorspec * pColor)
	{
	}
	//
	// EAM November 2004 - revised to take a pointer to struct rgb_color,
	// so that a palette gray value is not the only option for specifying color.
	//
    virtual void FilledPolygon(int points, gpiPoint *corners)
	{
	}
    virtual void Image(uint, uint, double *, gpiPoint *, t_imagecolor)
	{
	}
	// Enhanced text mode driver call-backs
    virtual void   EnhancedOpen(char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint)
	{
	}
    virtual void   EnhancedFlush()
	{
	}
    virtual void   EnhancedWritec(int c)
	{
	}
	//
	// Driver-specific synchronization or other layering commands.
	// Introduced as an alternative to the ugly sight of
	// driver-specific code strewn about in the core routines.
	// As of this point (July 2005) used only by pslatex.trm
	//
    virtual void   Layer(t_termlayer)
	{
	}
	//
	// Begin/End path control.
	// Needed by PostScript-like devices in order to join the endpoints of a polygon cleanly.
	//
    virtual void   Path(int p)
	{
	}
	//
	// Scale factor for converting terminal coordinates to output
	// pixel coordinates.  Used to provide data for external mousing code.
	//
	// Pass hypertext for inclusion in the output plot
    virtual void   HyperText(int type, const char *text)
	{
	}
    virtual void   ModifyPlots(uint operations, int plotno)
	{
	}
    virtual void   DashType(int type, t_dashtype *custom_dash_pattern)
	{
	}
#ifdef USE_MOUSE
    virtual int  WaitForInput(int) // used for mouse and hotkey input 
	{
		return 0;
	}
    virtual void PutTmpText(int, const char []) // draws temporary text; int determines where: 0=statusline, 1,2: at corners of zoom box, with \r separating text above and below the point 
	{
	}
    virtual void SetRuler(int, int) // set ruler location; x<0 switches ruler off 
	{
	}
    virtual void SetCursor(int, int, int) // set cursor style and corner of rubber band 
	{
	}
    virtual void SetClipboard(const char[]) // write text into cut&paste buffer (clipboard) 
	{
	}
#endif
#ifdef EAM_BOXED_TEXT
    virtual void   BoxedText(uint, uint, int)
	{
	}
#endif
	GpTermEntry()
	{
		name = 0;
		description = 0;
		xmax = 0;
		ymax = 0;
		VChr = 0;
		HChr = 0;
		VTic = 0;
		HTic = 0;
		flags = 0;
		tscale = 1.0;
	}
	GpTermEntry(
			const char * pName, 
			const char * pDescr, 
			uint _xmax, 
			uint _ymax, 
			uint _vchr, 
			uint _hchr, 
			uint _vtic, 
			uint _htic, 
			void (*fOptions)(GpCommand & rC),
			void (*fInit)(),
			void (*fReset)(),
			void (*fText)(),
			int  (*fScale)(double, double),
			void (*fGraphics)(),
			void (*fMove)(uint, uint),
			void (*fVector)(uint, uint),
			void (*fLinetype)(int),
			void (*fPut_text)(uint, uint, const char*),
			int  (*fText_angle)(int),
			int  (*fJustify_text)(enum JUSTIFY),
			void (*fPoint)(uint, uint, int),
			void (*fArrow)(uint, uint, uint, uint, int),
			int  (*fSet_font)(const char *font),
			void (*fPointsize)(double),
			int  _flags,
			void (*fSuspend)(),
			void (*fResume) (),
			void (*fFillbox)(int, uint, uint, uint, uint),
			void (*fLinewidth)(double linewidth),
		#ifdef USE_MOUSE
			int  (*fWaitforinput)(int),
			void (*fPut_tmptext)(int, const char []),
			void (*fSet_ruler)(int, int),
			void (*fSet_cursor)(int, int, int),
			void (*fSet_clipboard)(const char[]),
		#endif
			int (*fMake_palette)(t_sm_palette *palette),
			void (*fPrevious_palette)(),
			void (*fSet_color)(t_colorspec *),
			void (*fFilled_polygon)(int points, gpiPoint *corners),
			void (*fImage)(uint, uint, double *, gpiPoint *, t_imagecolor),
			void   (*fEnhanced_open)(char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint),
			void   (*fEnhanced_flush)(),
			void   (*fEnhanced_writec)(int),
			void   (*fLayer)(t_termlayer),
			void   (*fPath)(int),
			double _tscale,
			void   (*fHypertext)(int type, const char *text),
#ifdef EAM_BOXED_TEXT
			void   (*fBoxedText)(uint, uint, int),
#endif
			void   (*fModify_plots)(uint operations, int plotno),
			void   (*fDashtype)(int type, t_dashtype *custom_dash_pattern)
		)
	{
		name = pName;
		description = pDescr;
		xmax = _xmax;
		ymax = _ymax;
		VChr = _vchr;
		HChr = _hchr;
		VTic = _vtic;
		HTic = _htic;
		options = fOptions;
		init = fInit;
		reset = fReset;
		text = fText;
		scale = fScale;
		graphics = fGraphics;
		move = fMove;
		vector = fVector;
		linetype = fLinetype;
		put_text = fPut_text;
		text_angle = fText_angle;
		justify_text = fJustify_text;
		point = fPoint;
		arrow = fArrow;
		set_font = fSet_font;
		pointsize = fPointsize;
		flags = _flags;
		suspend = fSuspend;
		resume = fResume;
		fillbox = fFillbox;
		linewidth = fLinewidth;
	#ifdef USE_MOUSE
		waitforinput = fWaitforinput;
		put_tmptext = fPut_tmptext;
		set_ruler = fSet_ruler;
		set_cursor = fSet_cursor;
		set_clipboard = fSet_clipboard;
	#endif
		make_palette = fMake_palette;
		previous_palette = fPrevious_palette;
		set_color = fSet_color;
		filled_polygon = fFilled_polygon;
		image = fImage;
		enhanced_open = fEnhanced_open;
		enhanced_flush = fEnhanced_flush;
		enhanced_writec = fEnhanced_writec;
		layer = fLayer;
		path = fPath;
		tscale = _tscale;
		hypertext = fHypertext;
#ifdef EAM_BOXED_TEXT
		boxed_text = fBoxedText;
#endif
		modify_plots = fModify_plots;
		dashtype = fDashtype;
	}
#endif // } @construction
	//
	//
	//
    const  char * name;
    const  char * description;
    uint   xmax;
	uint   ymax;
	uint   VChr;
	uint   HChr;
	uint   VTic;
	uint   HTic;

    void (*options)(GpCommand & rC);
    void (*init)();
    void (*reset)();
    void (*text)();
    int  (*scale)(double, double);
    void (*graphics)();
    void (*move)(uint, uint);
    void (*vector)(uint, uint);
    void (*linetype)(int);
    void (*put_text)(uint, uint, const char *);
	//
    // the following are optional. set term ensures they are not NULL
	//
    int  (*text_angle)(int);
    int  (*justify_text)(enum JUSTIFY);
    void (*point)(uint, uint, int);
    void (*arrow)(uint, uint, uint, uint, int);
    int  (*set_font)(const char *font);
    void (*pointsize)(double); /* change pointsize */
    int  flags;
    void (*suspend)(); /* called after one plot of multiplot */
    void (*resume) (); /* called before plots of multiplot */
    void (*fillbox)(int, uint, uint, uint, uint); /* clear in multiplot mode */
    void (*linewidth)(double linewidth);
#ifdef USE_MOUSE
    int  (*waitforinput)(int);     /* used for mouse and hotkey input */
    void (*put_tmptext)(int, const char []);   /* draws temporary text; int determines where: 0=statusline, 1,2: at corners of zoom box, with \r separating text above and below the point */
    void (*set_ruler)(int, int);    /* set ruler location; x<0 switches ruler off */
    void (*set_cursor)(int, int, int);   /* set cursor style and corner of rubber band */
    void (*set_clipboard)(const char[]);  /* write text into cut&paste buffer (clipboard) */
#endif
    int (*make_palette)(t_sm_palette *palette);
	//
	// 1. if palette==NULL, then return nice/suitable
	// maximal number of colours supported by this terminal.
	// Returns 0 if it can make colours without palette (like postscript).
	// 2. if palette!=NULL, then allocate its own palette return value is undefined
	// 3. available: some negative values of max_colors for whatever can be useful
	//
    void (*previous_palette)();
	//
	// release the palette that the above routine allocated and get
	// back the palette that was active before.
	// Some terminals, like displays, may draw parts of the figure
	// using their own palette. Those terminals that possess only
	// one palette for the whole plot don't need this routine.
	//
    void (*set_color)(t_colorspec *);
	//
	// EAM November 2004 - revised to take a pointer to struct rgb_color,
	// so that a palette gray value is not the only option for specifying color.
	//
    void (*filled_polygon)(int points, gpiPoint *corners);
    void (*image)(uint, uint, double *, gpiPoint *, t_imagecolor);
	// Enhanced text mode driver call-backs
    void   (*enhanced_open)(char * fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
    void   (*enhanced_flush)();
    void   (*enhanced_writec)(int c);
	//
	// Driver-specific synchronization or other layering commands.
	// Introduced as an alternative to the ugly sight of
	// driver-specific code strewn about in the core routines.
	// As of this point (July 2005) used only by pslatex.trm
	//
    void   (*layer)(t_termlayer);
	//
	// Begin/End path control.
	// Needed by PostScript-like devices in order to join the endpoints of a polygon cleanly.
	//
    void   (*path)(int p);
	//
	// Scale factor for converting terminal coordinates to output
	// pixel coordinates.  Used to provide data for external mousing code.
	//
    double tscale;
	// Pass hypertext for inclusion in the output plot
    void   (*hypertext)(int type, const char *text);
#ifdef EAM_BOXED_TEXT
    void   (*boxed_text)(uint, uint, int);
#endif
    void   (*modify_plots)(uint operations, int plotno);
    void   (*dashtype)(int type, t_dashtype *custom_dash_pattern);
	//
	//
	//
	void SetColor(double gray);
	void DrawMultiline(uint x, uint y, char * pText, JUSTIFY hor, VERT_JUSTIFY vert, int angle, const char * pFont);
	//
	void _Move(uint x, uint y)
	{
		(*move)(x, y);
	}
	void _Vector(uint x, uint y)
	{
		(*vector)(x, y);
	}
	void _LineType(int _lt)
	{
		(*linetype)(_lt);
	}
	void _Layer(t_termlayer _l)
	{
		(*layer)(_l);
	}
	void _PutText(uint x, uint y, const char * pText)
	{
		(*put_text)(x, y, pText);
	}
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

/* HBB 20020225: this stuff used to be in a separate header, ipc.h,
 * but I strongly disliked the way that was done */

/*
 * There are the following types of interprocess communication from
 * (gnupmdrv, gnuplot_x11) => gnuplot:
 *	OS2_IPC  ... the OS/2 shared memory + event semaphores approach
 *	PIPE_IPC ... communication by using bidirectional pipe
 */


/*
 * OS2_IPC: gnuplot's terminals communicate with it by shared memory + event
 * semaphores => the code in gpexecute.c is used, and nothing more from here.
 */

/* options handling */
enum {
	UNSET = -1,
	no = 0,
	yes = 1
};

/* Variables of term.c needed by other modules: */
extern GpTermEntry * term; /* the terminal info structure, being the heart of the whole module */
extern char term_options[]; /* Options string of the currently used terminal driver */
extern int curr_arrow_headlength; /* access head length + angle without changing API */
extern double curr_arrow_headangle; /* angle in degrees */
extern double curr_arrow_headbackangle;
extern arrowheadfill curr_arrow_headfilled;
extern bool curr_arrow_headfixedsize;
extern int linetype_recycle_count; /* Recycle count for user-defined linetypes */
extern int mono_recycle_count;
extern char *outstr; /* Current 'output' file: name and open filehandle */
extern FILE *gpoutfile;
/*
	Output file where postscript terminal output goes to.
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
extern enum set_encoding_id encoding; /* 'set encoding' support: index of current encoding ... */
extern const char *encoding_names[]; /* ... in table of encoding names: */
extern const GenTable set_encoding_tbl[]; /* parsing table for encodings */
// extern bool term_initialised; /* mouse module needs this */
// The qt and wxt terminals cannot be used in the same session.
// Whichever one is used first to plot, this locks out the other.
extern void *term_interlock;
// Support for enhanced text mode.
extern char  enhanced_text[];
extern char *enhanced_cur_text;
extern double enhanced_fontscale;
//
// give array size to allow the use of sizeof
//
extern char enhanced_escape_format[16];
extern bool ignore_enhanced_text;
//
// Prototypes of functions exported by term.c
//
//void term_set_output(char *);
//void term_initialise();
void term_reset();
//void term_check_multiplot_okay(bool);
void init_monochrome();
GpTermEntry *change_term(const char *name, int length);
void DrawAxisLabel(uint x, uint y, const GpAxis & rAx, JUSTIFY hor, VERT_JUSTIFY vert, bool dontRotate);
int estimate_strlen(char *);
char *estimate_plaintext(char *);
void list_terms();
char* get_terminals_names();
GpTermEntry * set_term(GpCommand & rC);
void init_terminal();
//
// Support for enhanced text mode
//
const char *enhanced_recursion(const char *p, bool brace, char *fontname, double fontsize, double base, bool widthflag, bool showflag, int overprint);
void enh_err_check(const char *str);
//
// note: c is char, but must be declared int due to K&R compatibility.
//
void do_enh_writec(int c);
// flag: don't use enhanced output methods --- for output of
// filenames, which usually looks bad using subscripts
void ignore_enhanced(bool flag);
// Simple-minded test that point is with drawable area
bool on_page(int x, int y);
// Convert a fill style into a backwards compatible packed form
int style_from_fill(fill_style_type *);

#ifdef EAM_OBJECTS
// Terminal-independent routine to draw a circle or arc
void do_arc( uint cx, uint cy, double radius, double arc_start, double arc_end, int style, bool wedge);
#endif
#ifdef LINUXVGA
	void LINUX_setup();
#endif
#ifdef PC
	void PC_setup();
#endif
#ifdef VMS
	void vms_reset();
#endif

int    load_dashtype(t_dashtype *dt, int tag);
void   lp_use_properties(lp_style_type *lp, int tag);
void   load_linetype(lp_style_type *lp, int tag);
//
// Wrappers for term->path()
//
void   newpath(GpTermEntry * pT);
void   closepath(GpTermEntry * pT);
//
// Generic wrapper to check for mouse events or hotkeys during
// non-interactive input (e.g. "load")
//
void   check_for_mouse_events();

//#define DEFAULT_COLORSPEC {TC_DEFAULT, 0, 0.0}
//#define BLACK_COLORSPEC {TC_LT, LT_BLACK, 0.0}
//#define BACKGROUND_COLORSPEC {TC_LT, LT_BACKGROUND, 0.0}
//
// a point (with double coordinates) for use in polygon drawing
// the "c" field is used only inside the routine pm3d_plot()
//
struct gpdPoint : public RPoint3 {
	double c;
};
//
// inverting the colour for negative picture (default is positive picture) (for pm3d.positive)
//
#define SMPAL_NEGATIVE  'n'
#define SMPAL_POSITIVE  'p'
//
// GLOBAL VARIABLES
//
//extern t_sm_palette sm_palette;
#ifdef EXTENDED_COLOR_SPECS
	extern int supply_extended_color_specs;
#endif
//
// ROUTINES
//
//void init_color(); // call once to initialize variables
//
// Make the colour palette. Return 0 on success
// Put number of allocated colours into sm_palette.colors
//
//int make_palette();
void invalidate_palette();
//
// The routine above for 4 points explicitly
//
//#ifdef EXTENDED_COLOR_SPECS
//	void filled_quadrangle(gpdPoint *corners, gpiPoint* icorners);
//#else
//	void filled_quadrangle(gpdPoint *corners);
//#endif
//
// Makes mapping from real 3D coordinates, passed as coords array,
// to 2D terminal coordinates, then draws filled polygon
//
//void filled_polygon_3dcoords(int points, const GpCoordinate & rCoords);
//
// Makes mapping from real 3D coordinates, passed as coords array, but at z GpCoordinate
// fixed (base_z, for instance) to 2D terminal coordinates, then draws filled polygon
//
//void filled_polygon_3dcoords_zfixed(int points, const GpCoordinate & rCoords, double z);
//
// Draw colour smooth box
//
//
// VARIABLE.H
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
char * loadpath_handler(int, char *);

#define init_loadpath()    loadpath_handler(ACTION_INIT,NULL)
#define set_var_loadpath(path) loadpath_handler(ACTION_SET,(path))
#define get_loadpath()     loadpath_handler(ACTION_GET,NULL)
#define save_loadpath()    loadpath_handler(ACTION_SAVE,NULL)
#define clear_loadpath()   loadpath_handler(ACTION_CLEAR,NULL)
//
// Fontpath related
//
char * fontpath_handler(int, char *);

#define init_fontpath()    fontpath_handler(ACTION_INIT,NULL)
#define set_var_fontpath(path) fontpath_handler(ACTION_SET,(path))
#define get_fontpath()     fontpath_handler(ACTION_GET,NULL)
#define save_fontpath()    fontpath_handler(ACTION_SAVE,NULL)
#define clear_fontpath()   fontpath_handler(ACTION_CLEAR,NULL)
//
// Locale related
//
char * locale_handler(int, char *);

#define INITIAL_LOCALE ("C")

#define init_locale()      locale_handler(ACTION_INIT,NULL)
#define set_var_locale(path)   locale_handler(ACTION_SET,(path))
#define get_time_locale()       locale_handler(ACTION_GET,NULL)

#ifdef HAVE_LOCALE_H
	#define set_numeric_locale()   do {if(numeric_locale && strcmp(numeric_locale,"C")) setlocale(LC_NUMERIC,numeric_locale);} while (0)
	#define reset_numeric_locale() do {if(numeric_locale && strcmp(numeric_locale,"C")) setlocale(LC_NUMERIC,"C");} while (0)
	#define get_decimal_locale()  (localeconv()->decimal_point)
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
// EVAL.H
//
// user-defined variable table entry
//
struct UdvtEntry {
	void   Init(UdvtEntry * pNext, char * pName, DATA_TYPES typ)
	{
		next_udv = pNext;
		udv_name = pName;
		udv_value.Init(typ);
	}
    UdvtEntry *next_udv; // pointer to next value in linked list
    char * udv_name;   // name of this value entry */
    t_value udv_value; // value it has
};
//
// Descr: Идентификторы функций
//
enum {
	gpfuncNull = 0,	
	gpfunc_StringColumn,
	gpfunc_abs,
	gpfunc_acos,
	gpfunc_acosh,
	gpfunc_airy,   
	gpfunc_arg,
	gpfunc_asin,
	gpfunc_asinh,
	gpfunc_assign,
	gpfunc_atan,
	gpfunc_atan2,
	gpfunc_atanh,
	gpfunc_band,
	gpfunc_besj0,
	gpfunc_besj1,
	gpfunc_besy0,
	gpfunc_besy1,
	gpfunc_bnot,
	gpfunc_bool,
	gpfunc_bor,
	gpfunc_call,
	gpfunc_calle,
	gpfunc_calln,
	gpfunc_cdawson,
	gpfunc_ceil,
	gpfunc_cerf,   
	gpfunc_column,
	gpfunc_columnhead,
	gpfunc_concatenate,
	gpfunc_conjg,
	gpfunc_cos,
	gpfunc_cosh,
	gpfunc_div,
	gpfunc_dollars,
	gpfunc_ellip_first,
	gpfunc_ellip_second,
	gpfunc_ellip_third,
	gpfunc_eq,
	gpfunc_eqs,
	gpfunc_erf,
	gpfunc_erfc,
	gpfunc_erfi,   
	gpfunc_exists,
	gpfunc_exp,
	gpfunc_expint, 
	gpfunc_factorial,
	gpfunc_faddeeva,
	gpfunc_floor,
	gpfunc_gamma,
	gpfunc_ge,
	gpfunc_gprintf, 
	gpfunc_gt,
	gpfunc_hsv2rgb,
	gpfunc_ibeta,
	gpfunc_igamma,
	gpfunc_imag,
	gpfunc_index,
	gpfunc_int,
	gpfunc_inverse_erf, 
	gpfunc_inverse_normal,
	gpfunc_jtern,
	gpfunc_jump,
	gpfunc_jumpnz,
	gpfunc_jumpz,
	gpfunc_lambertw,
	gpfunc_land,
	gpfunc_le,
	gpfunc_leftshift,
	gpfunc_lgamma,
	gpfunc_lnot,
	gpfunc_log,
	gpfunc_log10,
	gpfunc_lor,
	gpfunc_lt,
	gpfunc_minus,
	gpfunc_mod,
	gpfunc_mult,
	gpfunc_ne,
	gpfunc_nes,
	gpfunc_normal,
	gpfunc_plus,
	gpfunc_pop,
	gpfunc_power,
	gpfunc_push,	
	gpfunc_pushc,
	gpfunc_pushd,
	gpfunc_pushd1,
	gpfunc_pushd2,
	gpfunc_rand,
	gpfunc_range,
	gpfunc_real,
	gpfunc_rightshift,
	gpfunc_sgn,
	gpfunc_sin,
	gpfunc_sinh,
	gpfunc_sprintf,
	gpfunc_sqrt,
	gpfunc_strftime,
	gpfunc_strlen, 
	gpfunc_strptime,
	gpfunc_strstrt,
	gpfunc_sum,
	gpfunc_system,
	gpfunc_tan,
	gpfunc_tanh,
	gpfunc_time,
	gpfunc_timecolumn,
	gpfunc_tmhour,
	gpfunc_tmmday,
	gpfunc_tmmin, 
	gpfunc_tmmon, 
	gpfunc_tmsec, 
	gpfunc_tmwday,
	gpfunc_tmyday,
	gpfunc_tmyear,
	gpfunc_uminus,
	gpfunc_valid,
	gpfunc_value,
	gpfunc_voigt,
	gpfunc_voigtp, 
	gpfunc_word,    
	gpfunc_words,   
	gpfunc_xor
};
//
//
//
struct GpCallFuncBlock {
	GpCallFuncBlock(const GpGadgets & rGg, GpArgument * pArg);

	GpArgument * P_Arg;
	double AngToRad;
};
//
// This type definition has to come after GpArgument has been declared.
//
// standard/internal function table entry
//
struct ft_entry {
    const char * f_name; // pointer to name of this function 
    // FUNC_PTR Proc_; 
	//void (* Proc_)(GpArgument * pArg); // (func) address of function to call 
	int    FuncId;
};

class GpEval {
public:
	GpEval(GpGadgets & rGg);
	void   ClearUdfList();
	int    IsBuiltinFunction(GpCommand & rC, int t_num) const;
	UdvtEntry * AddUdvByName(const char * key);
	UdvtEntry * AddUdv(GpCommand & rC, int t_num);
	UdvtEntry * GetUdvByName(const char * key) const;
	UdftEntry * AddUdf(GpCommand & rC, int t_num);
	void   DelUdvByName(const char * key, bool wildcard);
	static RETSIGTYPE Fpe(int an_int);
	void   EvaluateAt(AtType * at_ptr, t_value * val_ptr);
	void   FillGpValString(char * var, const char * stringvalue);
	void   FillGpValInteger(char * var, int value);
	void   FillGpValFloat(char * var, double value);
	void   FillGpValComplex(char * var, double areal, double aimag);
	void   UpdateGpValVariables(int context);
	void   FillGpValSysInfo();
	int    GpWords(char * string);

	void   CheckStack();
	void   ResetStack();
	t_value & TopOfStack() 
	{
		return Stack[Sp];
	}
	void   FASTCALL Push(t_value * x);
	t_value & FASTCALL Pop(t_value & rValue);
	t_value & FASTCALL PopOrConvertFromString(t_value & rValue);
	void   ExecuteAt(AtType * pAt);

	void   ShowVariables(GpCommand & rC);

	void   SaveFunctions__sub(FILE * fp);
	void   SaveVariables__sub(FILE * fp);
	//
	//
	//
	void   f_push(GpArgument * pArg);
	void   f_pushc(GpArgument * pArg);
	void   f_pushd1(GpArgument * pArg);
	void   f_pushd2(GpArgument * pArg);
	void   f_pushd(GpArgument * pArg);
	void   f_pop(GpArgument * pArg);
	void   f_call(GpArgument * pArg);
	void   f_calln(GpArgument * pArg);
	void   f_sum(GpArgument * pArg);
	void   f_lnot(GpArgument * pArg);
	void   f_bnot(GpArgument * pArg);
	void   f_lor(GpArgument * pArg);
	void   f_land(GpArgument * pArg);
	void   f_bor(GpArgument * pArg);
	void   f_xor(GpArgument * pArg);
	void   f_band(GpArgument * pArg);
	void   f_uminus(GpArgument * pArg);
	void   f_eq(GpArgument * pArg);
	void   f_ne(GpArgument * pArg);
	void   f_gt(GpArgument * pArg);
	void   f_lt(GpArgument * pArg);
	void   f_ge(GpArgument * pArg);
	void   f_le(GpArgument * pArg);
	void   f_leftshift(GpArgument * pArg);
	void   f_rightshift(GpArgument * pArg);
	void   f_plus(GpArgument * pArg);
	void   f_minus(GpArgument * pArg);
	void   f_mult(GpArgument * pArg);
	void   f_div(GpArgument * pArg);
	void   f_mod(GpArgument * pArg);
	void   f_power(GpArgument * pArg);
	void   f_factorial(GpArgument * pArg);
	void   f_concatenate(GpArgument * pArg);
	void   f_eqs(GpArgument * pArg);
	void   f_nes(GpArgument * pArg);
	void   f_gprintf(GpArgument * pArg);
	void   f_range(GpArgument * pArg);
	void   f_index(GpArgument * pArg);
	void   f_sprintf(GpArgument * pArg);
	void   f_strlen(GpArgument * pArg);
	void   f_strstrt(GpArgument * pArg);
	void   f_system(GpArgument * pArg);
	void   f_word(GpArgument * pArg);
	void   f_words(GpArgument * pArg);
	void   f_strftime(GpArgument * pArg);
	void   f_strptime(GpArgument * pArg);
	void   f_time(GpArgument * pArg);
	void   f_assign(GpArgument * pArg);
	void   f_value(GpArgument * pArg);
	void   f_bool(GpArgument * pArg);
	void   f_jump(GpArgument * pArg);
	void   f_jumpz(GpArgument * pArg);
	void   f_jumpnz(GpArgument * pArg);
	void   f_jtern(GpArgument * pArg);
	void   f_hsv2rgb(GpArgument *);
	//
	void   f_dollars(GpArgument * pArg);
	void   f_column(GpArgument * pArg);
	void   f_columnhead(GpArgument * pArg);
	void   f_valid(GpArgument * pArg);
	void   f_timecolumn(GpArgument * pArg);
	//
	void   f_real(GpArgument * pArg);
	void   f_imag(GpArgument * pArg);
	void   f_int(GpArgument * pArg);
	void   f_arg(GpArgument * pArg);
	void   f_conjg(GpArgument * pArg);
	void   f_sin(GpArgument * pArg);
	void   f_cos(GpArgument * pArg);
	void   f_tan(GpArgument * pArg);
	void   f_asin(GpArgument * pArg);
	void   f_acos(GpArgument * pArg);
	void   f_atan(GpArgument * pArg);
	void   f_atan2(GpArgument * pArg);
	void   f_sinh(GpArgument * pArg);
	void   f_cosh(GpArgument * pArg);
	void   f_tanh(GpArgument * pArg);
	void   f_asinh(GpArgument * pArg);
	void   f_acosh(GpArgument * pArg);
	void   f_atanh(GpArgument * pArg);
	void   f_ellip_first(GpArgument * pArg);
	void   f_ellip_second(GpArgument * pArg);
	void   f_ellip_third(GpArgument * pArg);
	void   f_void(GpArgument * pArg);
	void   f_abs(GpArgument * pArg);
	void   f_sgn(GpArgument * pArg);
	void   f_sqrt(GpArgument * pArg);
	void   f_exp(GpArgument * pArg);
	void   f_log10(GpArgument * pArg);
	void   f_log(GpArgument * pArg);
	void   f_floor(GpArgument * pArg);
	void   f_ceil(GpArgument * pArg);
	void   f_besj0(GpArgument * pArg);
	void   f_besj1(GpArgument * pArg);
	void   f_besy0(GpArgument * pArg);
	void   f_besy1(GpArgument * pArg);
	void   f_exists(GpArgument * pArg);   /* exists("foo") */
	void   f_tmsec(GpArgument * pArg);
	void   f_tmmin(GpArgument * pArg);
	void   f_tmhour(GpArgument * pArg);
	void   f_tmmday(GpArgument * pArg);
	void   f_tmmon(GpArgument * pArg);
	void   f_tmyear(GpArgument * pArg);
	void   f_tmwday(GpArgument * pArg);
	void   f_tmyday(GpArgument * pArg);
	//
	void   f_erf(GpArgument * pArg);
	void   f_erfc(GpArgument * pArg);
	void   f_ibeta(GpArgument * pArg);
	void   f_igamma(GpArgument * pArg);
	void   f_gamma(GpArgument * pArg);
	void   f_lgamma(GpArgument * pArg);
	void   f_rand(GpArgument * pArg);
	void   f_normal(GpArgument * pArg);
	void   f_inverse_normal(GpArgument * pArg);
	void   f_inverse_erf(GpArgument * pArg);
	void   f_lambertw(GpArgument * pArg);
	void   f_airy(GpArgument * pArg);
	void   f_expint(GpArgument * pArg);
	#ifndef HAVE_LIBCERF
		void   f_voigt(GpArgument * pArg);
	#endif
	#ifdef HAVE_LIBCERF
		void f_cerf(GpArgument *z);
		void f_cdawson(GpArgument *z);
		void f_faddeeva(GpArgument *z);
		void f_voigtp(GpArgument *z);
		void f_voigt(GpArgument *z);
		void f_erfi(GpArgument *z);
	#endif
	//
	//
	//
	static const ft_entry ft[]; /* The table of builtin functions */
	UdftEntry *first_udf; /* user-def'd functions */
	UdvtEntry *first_udv; /* user-def'd variables */
	UdvtEntry udv_pi; /* 'pi' variable */
	UdvtEntry *udv_NaN; /* 'NaN' variable */
	UdvtEntry **udv_user_head; /* first udv that can be deleted */
	bool   undefined;
	t_value Stack[STACK_DEPTH]; //
	int    Sp;                  // (s_p) stack pointer 
	int    JumpOffset;          // (jump_offset) to be modified by 'jump' operators 
private:
	void   UpdatePlotBounds();
	double AngToRad; // copy of GpGadgets::Ang2Rad Инициализируется в ExecuteAt перед вызовом функций
	GpGadgets & R_Gg;
};

double gp_exp(double x);
// HBB 20010726: Moved these here, from util.h
//double real(const t_value *);
int    real_int(const t_value *val);
double imag(const t_value *);
double magnitude(const t_value *);
double angle(const t_value *);
//t_value * Gcomplex(t_value *, double, double);
//t_value * _Ginteger(t_value *, int);
t_value * Gstring(t_value *, char *);
//t_value * pop_or_convert_from_string(t_value *);
t_value * gpfree_string(t_value *a);
void gpfree_array(t_value *a);
//void reset_stack();
//void check_stack();
bool more_on_stack();
//t_value * pop(t_value *x);
//void push(t_value *x);
void int_check(t_value & rValue);
// C-callable versions of internal gnuplot functions word() and words()
char * gp_word(char *string, int i);
//int gp_words(char *string);
//
// FIT.H
//
#define DEF_FIT_LIMIT 1e-5
#define LAMBDA_UP_FACTOR 10
#define LAMBDA_DOWN_FACTOR 10

// error interrupt for fitting routines 
#define Eex(a)       { ErrorEx(NO_CARET, (a)); }
#define Eex2(a,b)    { ErrorEx(NO_CARET, (a), (b)); }
#define Eex3(a,b,c)  { ErrorEx(NO_CARET, (a), (b), (c)); }
#define Eexc(c,a)    { ErrorEx((c), (a)); }
#define Eexc2(c,a,b) { ErrorEx((c), (a), (b)); }

/* Type definitions */

enum verbosity_level {
    QUIET, 
	RESULTS, 
	BRIEF, 
	VERBOSE
};

typedef char fixstr[MAX_ID_LEN+1];

class GpFit {
public:
	GpFit()
	{
		fitlogfile = NULL;
		fit_script = NULL;
		fit_wrap = 0;
		fit_verbosity = BRIEF;
		fit_suppress_log = false;
		fit_errorvariables = true;
		fit_covarvariables = false;
		fit_errorscaling = true;
		fit_prescale = true;
		fit_v4compatible = false;
		FITLIMIT = "FIT_LIMIT";
		FITSTARTLAMBDA = "FIT_START_LAMBDA";
		FITLAMBDAFACTOR = "FIT_LAMBDA_FACTOR";
		FITMAXITER = "FIT_MAXITER";
		epsilon_abs = 0.0; // default to zero non-relative limit
		maxiter = 0;
		//
		epsilon = DEF_FIT_LIMIT;
		startup_lambda = 0.0;
		lambda_down_factor = LAMBDA_DOWN_FACTOR;
		lambda_up_factor = LAMBDA_UP_FACTOR;
		fitlogfile_default = "fit.log";
		GNUFITLOG = "FIT_LOG";
		log_f = NULL;
		fit_show_lambda = true;
		GP_FIXED = "# FIXED";
		FITSCRIPT = "FIT_SCRIPT";
		DEFAULT_CMD = "replot";
		num_data = 0;
		num_params = 0;
		num_indep = 0;
		num_errors = 0;
		memzero(err_cols, sizeof(err_cols));
		columns = 0;
		fit_x = 0;
		fit_z = 0;
		err_data = 0;
		P_A = 0;
		regress_C = 0;
		regress_cleanup = NULL;
		user_stop = false;
		scale_params = 0;
		MEMSZERO(func);
		par_name = 0;
		last_par_name = NULL;
		last_num_params = 0;
		memzero(last_dummy_var, sizeof(last_dummy_var));
		last_fit_command = NULL;
		memzero(fit_dummy_udvs, sizeof(fit_dummy_udvs));
	}
	
	static void   InternalCleanup();

	enum marq_res_t {
		OK, 
		ML_ERROR, 
		BETTER, 
		WORSE
	};
	void   Init();
	void   FitCommand(GpCommand & rC);
	void   Update(char * pfile, char * npfile);
	marq_res_t Marquardt(double a[], double ** C, double * chisq, double * lambda);
	void   Calculate(double * zfunc, double ** dzda, double a[]);
	void   Analyze(double a[], double ** C, double d[], double * chisq, double ** deriv);
	void   CallGnuplot(const double * par, double * data);
	bool   Regress(double a[]);
	bool   RegressCheckStop(int iter, double chisq, double last_chisq, double lambda);
	void   RegressFinalize(int iter, double chisq, double last_chisq, double lambda, double ** covar);
	void   FitShow(int i, double chisq, double last_chisq, double* a, double lambda, FILE * device);
	void   FitShowBrief(int iter, double chisq, double last_chisq, double* parms, double lambda, FILE * device);
	void   FitProgress(int i, double chisq, double last_chisq, double* a, double lambda, FILE * device);
	void   ShowResults(double chisq, double last_chisq, double * a, double * dpar, double ** corel);
	char * GetFitScript();
	char * GetLogfile();
	bool   FitInterrupt();
	void   ErrorEx(int t_num, const char * str, ...);
	void   Dblfn(const char * fmt, ...);
	size_t WriToFilLastFitCmd(FILE * fp);

	const char * FITLIMIT;
	const char * FITSTARTLAMBDA;
	const char * FITLAMBDAFACTOR;
	const char * FITMAXITER;
	char * fitlogfile;
	char * fit_script;
	int    maxiter;
	int    fit_wrap;
	double epsilon_abs;  /* absolute convergence criterion */
	verbosity_level fit_verbosity;
	bool   fit_errorscaling;
	bool   fit_prescale;
	bool   fit_suppress_log;
	bool   fit_errorvariables;
	bool   fit_covarvariables;
	bool   fit_v4compatible;

	double ** regress_C;  // global copy of C matrix in regress 
private:
	void   Implement_FitCommand(GpCommand & rC);
	double EffectiveError(double ** deriv, int i);
	void   CalcEerivatives(const double * par, double * data, double ** deriv);
	void   RegressInit();
	void   BackupFile(char * tofile, const char * fromfile);

	double epsilon; // relative convergence limit
	double startup_lambda;
	double lambda_down_factor;
	double lambda_up_factor;
	const char * fitlogfile_default;
	const char * GNUFITLOG;
	FILE * log_f;
	bool fit_show_lambda;
	const char * GP_FIXED;
	const char * FITSCRIPT;
	const char * DEFAULT_CMD;      /* if no fitscript spec. */
	int num_data;
	int num_params;
	int num_indep;    /* # independent variables in fit function */
	int num_errors;   /* # error columns */
	bool err_cols[MAX_NUM_VAR+1];    /* true if variable has an associated error */
	int columns;      /* # values read from data file for each point */
	double * fit_x;       // all independent variable values, e.g. value of the ith variable from the jth data point is in fit_x[j*num_indep+i] 
	double * fit_z;       /* dependent data values */
	double * err_data;    /* standard deviations of indep. and dependent data */
	double * P_A;         // array of fitting parameters 
	void (* regress_cleanup)(); // memory cleanup function callback 
	bool user_stop;
	double * scale_params; // scaling values for parameters 
	UdftEntry func;
	fixstr * par_name;
	fixstr * last_par_name;
	int last_num_params;
	char * last_dummy_var[MAX_NUM_VAR];
	char * last_fit_command;;
	//
	// Mar 2014 - the single hottest call path in fit was looking up the
	// dummy parameters by name (4 billion times in fit.dem).
	// A total waste, since they don't change.  Look up once and store here.
	//
	UdvtEntry * fit_dummy_udvs[MAX_NUM_VAR];
};
//
// READLINE.h
//
#if defined(HAVE_LIBREADLINE)
	#include <readline/readline.h>
	#include <readline/history.h>
#endif
#if defined(HAVE_LIBEDITLINE)
	#include <editline/readline.h>
#endif
#if defined(HAVE_LIBEDITLINE)
	int getc_wrapper(FILE* fp);
#endif
#if defined(READLINE) && !defined(HAVE_LIBREADLINE) && !defined(HAVE_LIBEDITLINE)
	//char * readline(const char *);
#endif
//
// The following 'readline_ipc' routine is usual 'readline' for OS2_IPC,
// and a special one for IPC communication.
//
//char * readline_ipc(const char*);
//
// SAVE.H
//
// Variables of save.c needed by other modules
extern const char *coord_msg[];
//
// SCANNER.H
//
// Variables of scanner.c needed by other modules:
extern int curly_brace_count;

bool legal_identifier(char *p);
//int scanner(char **expression, size_t *line_lengthp);
//
// PLOT.H
//
//
// Variables of plot.c needed by other modules
//
//
// Prototypes of functions exported by plot.c
//
#if defined(__GNUC__)
	void bail_to_command_line() __attribute__((noreturn));
#else
	void bail_to_command_line();
#endif
//void init_constants();
//void init_session();
#if defined(_Windows)
	int gnu_main(int argc, char **argv);
#endif
void interrupt_setup();
void gp_expand_tilde(char **);
void get_user_env();
#ifdef LINUXVGA
	void drop_privilege();
	void take_privilege();
#endif /* LINUXVGA */
void restrict_popen();
#ifdef GNUPLOT_HISTORY
	void cancel_history();
#else
	#define cancel_history() {}
#endif
//
// PLOT2D.H
//
//
// EXPERIMENTAL configuration option
//
#define SMOOTH_BINS_OPTION 1
//
// This allows a natural interpretation of providing only a single column in 'using'
//
#define default_smooth_weight(option) oneof3(option, SMOOTH_BINS, SMOOTH_KDENSITY, SMOOTH_FREQUENCY)
//
// Variables of plot2d.c needed by other modules:
//
//extern CurvePoints * P_FirstPlot; // first_plot
//extern double boxwidth;
//extern bool boxwidth_is_absolute;
//
// PLOT3D.H
//
enum t_data_mapping {
    MAP3D_CARTESIAN,
    MAP3D_SPHERICAL,
    MAP3D_CYLINDRICAL
};
//
// GADGETS.H
//
/* Types and variables concerning graphical plot elements that are not
 * *terminal-specific, are used by both* 2D and 3D plots, and are not
 * *assignable to any particular * axis. I.e. they belong to neither
 * *term_api, graphics, graph3d, nor * axis .h files.
 */
// Coordinate system specifications: x1/y1, x2/y2, graph-box relative
// or screen relative GpCoordinate systems 
enum position_type {
	first_axes,
	second_axes,
	graph,
	screen,
	character
};
//
// This is the default state for the axis, timestamp, and plot title labels indicated by tag = -2
//
#define NONROTATABLE_LABEL_TAG -2
#define ROTATE_IN_3D_LABEL_TAG -3
//
// A full 3D position, with all 3 coordinates of possible using different axes.
// Used for 'set label', 'set arrow' positions and various offsets.
//
struct GpPosition : public RPoint3 {
	void   Set(position_type sx, position_type sy, position_type sz, double _x, double _y, double _z)
	{
		scalex = sx;
		scaley = sy;
		scalez = sz;
		RPoint3::Set(_x, _y, _z);
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
	void   SetMargin(GpCommand & rC);
	void   SetDefaultKeyPosition()
	{
		//#define DEFAULT_KEY_POSITION { graph, graph, graph, 0.9, 0.9, 0. }
		scalex = graph;
		scaley = graph;
		scalez = graph;
		RPoint3::Set(0.9, 0.9, 0.0);
	}
	position_type scalex;
	position_type scaley;
	position_type scalez;
};
//
// Linked list of structures storing 'set label' information
//
struct GpTextLabel {
	GpTextLabel()
	{
		SetEmpty();
	}
	//void free_labels(GpTextLabel * label)
	static void Destroy(GpTextLabel * pLabel)
	{
		if(pLabel) {
			char * p_master_font = pLabel->font;
			// Labels generated by 'plot with labels' all use the same font
			free(p_master_font);
			while(pLabel) {
				free(pLabel->text);
				if(pLabel->font && pLabel->font != p_master_font)
					free(pLabel->font);
				GpTextLabel * temp = pLabel->next;
				free(pLabel);
				pLabel = temp;
			}
		}
	}
	void    SetEmpty()
	{
		/* #define EMPTY_LABELSTRUCT \
		                {NULL, NONROTATABLE_LABEL_TAG, \
		                {character, character, character, 0.0, 0.0, 0.0}, CENTRE, 0, 0, \
		                0, \
		                NULL, NULL, {TC_LT, -2, 0.0}, DEFAULT_LP_STYLE_TYPE, \
		                {character, character, character, 0.0, 0.0, 0.0}, false, \
		                false}
		 */
		next = 0;
		tag = NONROTATABLE_LABEL_TAG;
		place.Set(character, character, character, 0.0, 0.0, 0.0);
		pos = CENTRE;
		rotate = 0;
		layer = 0;
		boxed = 0;
		text = 0;
		font = 0;
		textcolor.type = TC_LT;
		textcolor.lt = -2;
		textcolor.value = 0.0;
		lp_properties.SetDefault();
		offset.Set(character, character, character, 0.0, 0.0, 0.0);
		noenhanced = false;
		hypertext = false;
	}
	//
	// process 'unset [xyz]{2}label command
	//
	//static void unset_axislabel_or_title(GpTextLabel * label)
	void UnsetAxisLabelOrTitle()
	{
		GpPosition default_offset;
		default_offset.Set(character, character, character, 0., 0., 0.);
		ZFREE(text);
		ZFREE(font);
		offset = default_offset;
		textcolor.type = TC_DEFAULT;
	}
	GpTextLabel * next; // pointer to next label in linked list
	int    tag; // identifies the label
	GpPosition place;
	enum JUSTIFY pos; // left/center/right horizontal justification

	int    rotate;
	int    layer;
	int    boxed; // EAM_BOXED_TEXT
	char * text;
	char * font; // Entry font added by DJL
	t_colorspec textcolor;
	lp_style_type lp_properties;
	GpPosition offset;
	bool   noenhanced;
	bool   hypertext;
};
//
// Datastructure for implementing 'set arrow' 
//
enum arrow_type {
	arrow_end_absolute,
	arrow_end_relative,
	arrow_end_oriented
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

#ifdef EAM_OBJECTS
/* The object types supported so far are OBJ_RECTANGLE, OBJ_CIRCLE, and OBJ_ELLIPSE */
struct t_rectangle {
	void   SetDefault()
	{
		THISZERO();
	}

	int type;      // 0 = corners;  1 = center + size
	GpPosition center; // center
	GpPosition extent; // width and height
	GpPosition bl; // bottom left
	GpPosition tr; // top right
};

#define DEFAULT_RADIUS (-1.0)
#define DEFAULT_ELLIPSE (-2.0)

struct t_circle {
	void   SetDefault()
	{
		// {.circle = {1, {0,0,0,0.,0.,0.}, {graph,0,0,0.02,0.,0.}, 0., 360., true }} }
		type = 1;
		MEMSZERO(center);
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
	bool wedge;         /* true = connect arc ends to center */
};

#define ELLIPSEAXES_XY (0)
#define ELLIPSEAXES_XX (1)
#define ELLIPSEAXES_YY (2)

struct t_ellipse {
	void   SetDefault()
	{
		// {.ellipse = {ELLIPSEAXES_XY, {0,0,0,0.,0.,0.}, {graph,graph,0,0.05,0.03,0.}, 0. }} }
		type = ELLIPSEAXES_XY;
		MEMSZERO(center);
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
	void   SetDefault()
	{
		// {.polygon = {0, NULL} } }
		type = 0;
		vertex = 0;
	}
	int    type;          // Number of vertices
	GpPosition * vertex;  // Array of vertices
};

enum t_clip_object {
	OBJ_CLIP,       // Clip to graph unless GpCoordinate type is screen 
	OBJ_NOCLIP,     // Clip to canvas, never to graph 
	OBJ_ALWAYS_CLIP // Not yet implemented 
};

#define OBJ_RECTANGLE (1)
#define OBJ_CIRCLE (2)
#define OBJ_ELLIPSE (3)
#define OBJ_POLYGON (4)
//
// Datastructure for 'set object'
//
struct t_object {
	enum {
		defUnkn = 0,
		defRectangle = 1,
		defCircle,
		defEllipse,
		defPolygon
	};

	t_object(int def)
	{
		if(def == defRectangle)
			SetDefaultRectangleStyle();
		else if(def == defCircle)
			SetDefaultCircleStyle();
		else if(def == defEllipse)
			SetDefaultEllipseStyle();
		else if(def == defPolygon)
			SetDefaultPolygonStyle();
	}
	void SetDefaultRectangleStyle()
	{
		/*
		   #define DEFAULT_RECTANGLE_STYLE
		        {
		                NULL,
		                -1,
		                0,
		                OBJ_RECTANGLE,
		                OBJ_CLIP,
		                { FS_SOLID, 100, 0, BLACK_COLORSPEC},
		                {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC,
		                   DEFAULT_DASHPATTERN},
		                {.rectangle = {0, {0,0,0,0.,0.,0.}, {0,0,0,0.,0.,0.}, {0,0,0,0.,0.,0.},
		                   {0,0,0,0.,0.,0.}}}
		        }
		 */
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
		/*
		   #define DEFAULT_CIRCLE_STYLE { NULL, -1, 0, OBJ_CIRCLE, OBJ_CLIP, \
		                {FS_SOLID, 100, 0, BLACK_COLORSPEC},                    \
		                {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC,
		                   DEFAULT_DASHPATTERN}, \
		                {.circle = {1, {0,0,0,0.,0.,0.}, {graph,0,0,0.02,0.,0.}, 0., 360., true }} }
		 */
		next = 0;
		tag = -1;
		layer = 0;
		object_type = OBJ_CIRCLE;
		clip = OBJ_CLIP;
		fillstyle.SetDefault();
		lp_properties.SetDefault();
		o.circle.SetDefault();
	}

	void SetRAxisCircleStyle()
	{
		next = 0;
		tag = 1;
		layer = 1;
		object_type = OBJ_CIRCLE;
		clip = OBJ_CLIP;
		fillstyle.SetDefault();
		lp_properties.SetDefault();
		lp_properties.l_width = 0.2;
		o.circle.SetDefault();
		/*
		        t_object raxis_circle = {
		                NULL, 1, 1, OBJ_CIRCLE, OBJ_CLIP, // link, tag, layer (front), object_type, clip //
		                {FS_SOLID, 100, 0, BLACK_COLORSPEC},
		                {0, LT_BACKGROUND, 0, DASHTYPE_AXIS, 0, 0.2, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC,
		                   DEFAULT_DASHPATTERN},
		                {.circle = {1, {0, 0, 0, 0., 0., 0.}, {graph, 0, 0, 0.02, 0., 0.}, 0., 360. }}
		        };
		 */
	}

	void SetDefaultEllipseStyle()
	{
		/*
		   #define DEFAULT_ELLIPSE_STYLE { NULL, -1, 0, OBJ_ELLIPSE, OBJ_CLIP, \
		                {FS_SOLID, 100, 0, BLACK_COLORSPEC},                    \
		                {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC,
		                   DEFAULT_DASHPATTERN}, \
		                {.ellipse = {ELLIPSEAXES_XY, {0,0,0,0.,0.,0.}, {graph,graph,0,0.05,0.03,0.}, 0. }} }
		 */
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
		/*
		   #define DEFAULT_POLYGON_STYLE { NULL, -1, 0, OBJ_POLYGON, OBJ_CLIP, \
		                {FS_SOLID, 100, 0, BLACK_COLORSPEC},                    \
		                {0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 1.0, 0.0, DEFAULT_P_CHAR, BLACK_COLORSPEC,
		                   DEFAULT_DASHPATTERN}, \
		                {.polygon = {0, NULL} } }
		 */
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

	t_object * next;
	int tag;
	int layer;              /* behind or back or front */
	int object_type;        /* OBJ_RECTANGLE */
	t_clip_object clip;
	fill_style_type fillstyle;
	lp_style_type lp_properties;
	union {
		t_rectangle rectangle;
		t_circle circle;
		t_ellipse ellipse;
		t_polygon polygon;
	} o;
};

#endif
//
// Datastructure implementing 'set dashtype' 
//
struct custom_dashtype_def {
	custom_dashtype_def * next; // pointer to next dashtype in linked list
	int    tag;          // identifies the dashtype
	int    d_type;       // for DASHTYPE_SOLID or CUSTOM
	t_dashtype dashtype;
};
//
// Datastructure implementing 'set style line'
//
struct linestyle_def {
	linestyle_def * next;   /* pointer to next linestyle in linked list */
	int tag;                /* identifies the linestyle */
	lp_style_type lp_properties;
};

/* Datastructure implementing 'set style arrow' */
struct arrowstyle_def {
	arrowstyle_def * next; /* pointer to next arrowstyle in linked list */
	int tag;                /* identifies the arrowstyle */
	arrow_style_type arrow_properties;
};

//
// Plot layer definitions are collected here.
// Someday they might actually be used.
//
#define LAYER_BEHIND     -1
#define LAYER_BACK        0
#define LAYER_FRONT       1
#define LAYER_FOREGROUND  2     /* not currently used */
#define LAYER_PLOTLABELS 99
//
// For 'set style parallelaxis'
//
struct pa_style {
	pa_style() : lp_properties(lp_style_type::defParallelAxis)
	{
		layer = LAYER_FRONT;
	}
	lp_style_type lp_properties; /* used to draw the axes themselves */
	int layer;              /* front/back */
};
/*
#define DEFAULT_PARALLEL_AXIS_STYLE {{0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 2.0, 0.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, \
	DEFAULT_DASHPATTERN}, LAYER_FRONT }
*/
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

/* If exterior, there are 12 possible auto placements.  Since
   left/right/center with top/bottom/center can only define 9
   locations, further subdivide the exterior region into four
   subregions for which left/right/center (TMARGIN/BMARGIN)
   and top/bottom/center (LMARGIN/RMARGIN) creates 12 locations. */
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
	filledcurves_opts()
	{
		THISZERO();
	}

	int opt_given; /* option given / not given (otherwise default) */
	int closeto; /* from list FILLEDCURVES_CLOSED, ... */
	double at; /* value for FILLEDCURVES_AT... */
	double aty; /* the other value for FILLEDCURVES_ATXY */
	int oneside; /* -1 if fill below bound only; +1 if fill above bound only */
};

//#define EMPTY_FILLEDCURVES_OPTS { 0, 0, 0.0, 0.0, 0 }

enum t_histogram_type /*histogram_type*/ {
	HT_NONE,
	HT_STACKED_IN_LAYERS,
	HT_STACKED_IN_TOWERS,
	HT_CLUSTERED,
	HT_ERRORBARS
};

struct histogram_style {
	histogram_style()
	{
		//#define DEFAULT_HISTOGRAM_STYLE { HT_CLUSTERED, 2, 1, 0.0, 0.0, LT_UNDEFINED, LT_UNDEFINED, 0, NULL,
		// EMPTY_LABELSTRUCT }
		type = HT_CLUSTERED;
		gap = 2;
		clustersize = 1;
		start = 0.0;
		end = 0.0;
		startcolor = LT_UNDEFINED;
		startpattern = LT_UNDEFINED;
		bar_lw = 0.0;
		next = 0;
		title.SetEmpty();
	}
	int    type;        // enum t_histogram_type
	int    gap;         // set style hist gap <n> (space between clusters)
	int    clustersize; // number of datasets in this histogram
	double start;    // X-coord of first histogram entry
	double end;      // X-coord of last histogram entry
	int    startcolor;  // LT_UNDEFINED or explicit color for first entry
	int    startpattern; // LT_UNDEFINED or explicit pattern for first entry
	double bar_lw;   // linewidth for error bars
	histogram_style * next;
	GpTextLabel title;
};

//#define DEFAULT_HISTOGRAM_STYLE { HT_CLUSTERED, 2, 1, 0.0, 0.0, LT_UNDEFINED, LT_UNDEFINED, 0, NULL, EMPTY_LABELSTRUCT }

enum t_boxplot_factor_labels {
	BOXPLOT_FACTOR_LABELS_OFF,
	BOXPLOT_FACTOR_LABELS_AUTO,
	BOXPLOT_FACTOR_LABELS_X,
	BOXPLOT_FACTOR_LABELS_X2
};

#define DEFAULT_BOXPLOT_FACTOR -1

struct boxplot_style {
	boxplot_style()
	{
		SetDefault();
	}
	void SetDefault()
	{
		//#define DEFAULT_BOXPLOT_STYLE { 0, 1.5, true, 6, CANDLESTICKS, 1.0, BOXPLOT_FACTOR_LABELS_AUTO, false }
		limit_type = 0;
		limit_value = 0.0;
		outliers = true;
		pointtype = 6;
		plotstyle = CANDLESTICKS;
		separation = 1.0;
		labels = BOXPLOT_FACTOR_LABELS_AUTO;
		sort_factors = false;  
	}
	void   Show();

	int    limit_type; // 0 = multiple of interquartile 1 = fraction of points
	double limit_value;
	bool   outliers;
	int    pointtype;
	int    plotstyle;  // CANDLESTICKS or FINANCEBARS 
	double separation; // of boxplots if there are more than one factors 
	t_boxplot_factor_labels labels; // Which axis to put the tic labels if there are factors 
	bool   sort_factors; // Sort factors in alphabetical order? 
};

//#define DEFAULT_BOXPLOT_STYLE { 0, 1.5, true, 6, CANDLESTICKS, 1.0, BOXPLOT_FACTOR_LABELS_AUTO, false }

#ifdef EAM_BOXED_TEXT
	struct textbox_style {
		textbox_style()
		{
			SetDefault();
		}
		void   SetDefault()
		{
			//#define DEFAULT_TEXTBOX_STYLE { false, false, 1.0, 1.0 }
			opaque = false;
			noborder = false;
			xmargin = 1.0;
			ymargin = 1.0;
		}
		bool opaque; // True if the box is background-filled before writing into it 
		bool noborder; // True if you want fill only, no lines 
		double xmargin; // fraction of default margin to use 
		double ymargin; // fraction of default margin to use 
	};
#endif
//
// bounding box position, in terminal coordinates
//
struct BoundingBox {
	void   SetZero()
	{
		THISZERO();
	}
	int    xleft;
	int    xright;
	int    ybot;
	int    ytop;
};
//
// EAM Feb 2003 - Move all global variables related to key into a
// single structure. Eventually this will allow multiple keys.  
//
enum keytitle_type {
	NOAUTO_KEYTITLES,
	FILENAME_KEYTITLES,
	COLUMNHEAD_KEYTITLES
};

struct legend_key {
	legend_key()
	{
		SetDefault();
	}

	void   SetDefault()
	{
		/*
		   #define DEFAULT_KEY_PROPS \
		                        { true, \
		                        GPKEY_AUTO_INTERIOR_LRTBC, GPKEY_RMARGIN, \
		                        DEFAULT_KEY_POSITION, \
		                        JUST_TOP, RIGHT, \
		                        GPKEY_RIGHT, GPKEY_VERTICAL, \
		                        4.0, 1.0, 0.0, 0.0, \
		                        FILENAME_KEYTITLES, \
		                        false, false, false, true, \
		                        DEFAULT_KEYBOX_LP, \
		                        NULL, {TC_LT, LT_BLACK, 0.0}, \
		                        {0,0,0,0}, 0, 0, \
		                        EMPTY_LABELSTRUCT}
		 */
		visible = true;
		region = GPKEY_AUTO_INTERIOR_LRTBC;
		margin = GPKEY_RMARGIN;
		user_pos.SetDefaultKeyPosition();
		vpos = JUST_TOP;
		hpos = RIGHT;
		just = GPKEY_RIGHT;
		stack_dir = GPKEY_VERTICAL;
		swidth = 4.0;
		vert_factor = 1.0;
		width_fix = 0.0;
		height_fix = 0.0;
		auto_titles = FILENAME_KEYTITLES;
		front = false;
		reverse = false;
		invert = false;
		enhanced = true;
		box.SetDefaultKeybox();
		font = 0;
		{
			textcolor.type = TC_LT;
			textcolor.lt = LT_BLACK;
			textcolor.value = 0.0;
		}
		MEMSZERO(bounds);
		maxcols = 0;
		maxrows = 0;
		title.SetEmpty();
	}

	bool visible;       // Do we show this key at all?
	t_key_region region;    // if so: where?
	t_key_ext_region margin; // if exterior: where outside?
	GpPosition user_pos;    // if user specified position, this is it
	VERT_JUSTIFY vpos;      // otherwise these guide auto-positioning
	JUSTIFY hpos;
	t_key_sample_positioning just;
	t_key_stack_direction stack_dir;
	double swidth;          // 'width' of the linestyle sample line in the key
	double vert_factor;     // user specified vertical spacing multiplier
	double width_fix;       // user specified additional (+/-) width of key titles
	double height_fix;
	keytitle_type auto_titles; // auto title curves unless plotted 'with notitle'
	bool front;         // draw key in a second pass after the rest of the graph
	bool reverse;       // key back to front
	bool invert;        // key top to bottom
	bool enhanced;      // enable/disable enhanced text of key titles
	lp_style_type box; // linetype of box around key:
	char * font;                    // Will be used for both key title and plot titles
	t_colorspec textcolor;  // Will be used for both key title and plot titles
	BoundingBox bounds;
	int maxcols;            // maximum no of columns for horizontal keys
	int maxrows;            // maximum no of rows for vertical keys
	GpTextLabel title;       // holds title line for the key as a whole
};

//extern legend_key keyT;
//
// EAM Jan 2006 - Move colorbox structure definition to here from color.h
// in order to be able to use GpPosition
//
#define SMCOLOR_BOX_NO      'n'
#define SMCOLOR_BOX_DEFAULT 'd'
#define SMCOLOR_BOX_USER    'u'

struct color_box_struct {
	void   SetDefault()
	{
		where = SMCOLOR_BOX_DEFAULT;
		rotation = 'v';
		border = 1;
		border_lt_tag = LT_BLACK;
		layer = LAYER_FRONT;
		xoffset = 0;
		origin.Set(screen, screen, screen, 0.90, 0.2, 0.0);
		size.Set(screen, screen, screen, 0.05, 0.6, 0.0);
		bounds.SetZero();
	}
	char   where;
	// where
	// SMCOLOR_BOX_NO .. do not draw the colour box
	// SMCOLOR_BOX_DEFAULT .. draw it at default position and size
	// SMCOLOR_BOX_USER .. draw it at the position given by user
	//
	char   rotation; /* 'v' or 'h' vertical or horizontal box */
	char   border; /* if non-null, a border will be drawn around the box (default) */
	int    border_lt_tag;
	int    layer; /* front or back */
	int    xoffset; /* To adjust left or right, e.g. for y2tics */
	GpPosition origin;
	GpPosition size;
	BoundingBox bounds;
};

#define DEFAULT_MARGIN_POSITION {character, character, character, -1, -1, -1}
#define ZERO 1e-8 // default for 'zero' set option
//
// A macro to check whether 2D functionality is allowed in the last plot:
// either the plot is a 2D plot, or it is a suitably oriented 3D plot (e.g. map).
//
#define ALMOST2D (!GpGg.Is3DPlot || GpGg.splot_map || (fabs(fmod(GpGg.surface_rot_z, 90.0f))<0.1 && fabs(fmod(GpGg.surface_rot_x, 180.0f))<0.1))
//#define SET_REFRESH_OK(ok, nplots) do { GpGg.RefreshOk = (ok); GpGg.RefreshNPlots = (nplots); } while(0)
#define SAMPLES 100             /* default number of samples for a plot */
#ifndef DEFAULT_TIMESTAMP_FORMAT
	#define DEFAULT_TIMESTAMP_FORMAT "%a %b %d %H:%M:%S %Y" // asctime() format
#endif
#define SOUTH           1 /* 0th bit */
#define WEST            2 /* 1th bit */
#define NORTH           4 /* 2th bit */
#define EAST            8 /* 3th bit */

enum TRefresh_Allowed {
	E_REFRESH_NOT_OK = 0,
	E_REFRESH_OK_2D = 2,
	E_REFRESH_OK_3D = 3
};
//
// Holder for various image properties
//
struct t_image {
	t_imagecolor type; /* See above */
	bool fallback; /* true == don't use terminal-specific code */
	// image dimensions
	uint ncols;
	uint nrows;
};

extern pa_style parallel_axis_style;

// Prefer line styles over plain line types
//#if 1 || defined(BACKWARDS_COMPATIBLE)
//	extern bool prefer_line_styles;
//#else
//	#define prefer_line_styles false
//#endif
//extern histogram_style histogram_opts;
#ifdef EAM_BOXED_TEXT
	//extern textbox_style textbox_opts;
#endif
#ifdef EAM_OBJECTS
	// Warning: C89 does not like the union initializers 
	//extern t_object default_rectangle;
	//extern t_object default_circle;
	//extern t_object default_ellipse;
#endif
//
// AXIS.H
//
#ifndef DISABLE_NONLINEAR_AXES
	#define NONLINEAR_AXES
#endif

/* typedefs / #defines */

/* give some names to some array elements used in command.c and grap*.c
 * maybe one day the relevant items in setshow will also be stored
 * in arrays.
 *
 * Always keep the following conditions alive:
 * SECOND_X_AXIS = FIRST_X_AXIS + SECOND_AXES
 * FIRST_X_AXIS & SECOND_AXES == 0
 */
#ifndef MAX_PARALLEL_AXES
	#define MAX_PARALLEL_AXES MAXDATACOLS-1
#endif

enum AXIS_INDEX {
    NO_AXIS = -2,
    ALL_AXES = -1,
    FIRST_Z_AXIS = 0,
#define FIRST_AXES FIRST_Z_AXIS
    FIRST_Y_AXIS,
    FIRST_X_AXIS,
    COLOR_AXIS,			/* fill gap */
    SECOND_Z_AXIS,		/* not used, yet */
#define SECOND_AXES SECOND_Z_AXIS
    SAMPLE_AXIS=SECOND_Z_AXIS,
    SECOND_Y_AXIS,
    SECOND_X_AXIS,
    POLAR_AXIS,
#define NUMBER_OF_MAIN_VISIBLE_AXES (POLAR_AXIS + 1)
    T_AXIS,
    U_AXIS,
    V_AXIS,		// Last index into GpGg[] 
    PARALLEL_AXES, // Parallel axis data is allocated dynamically 

    AXIS_ARRAY_SIZE = PARALLEL_AXES
};
//
// HBB NOTE 2015-01-28: SECOND_Z_AXIS is not actually used 
//
#define AXIS_IS_SECOND(ax) (((ax) >= SECOND_Y_AXIS) && ((ax) <= SECOND_X_AXIS))
#define AXIS_IS_FIRST(ax)  (((ax) >=  FIRST_Z_AXIS) && ((ax) <=  FIRST_X_AXIS))
#define AXIS_MAP_FROM_FIRST_TO_SECOND(ax) (SECOND_AXES + ((ax) - FIRST_AXES))
#define AXIS_MAP_FROM_SECOND_TO_FIRST(ax) (FIRST_AXES + ((ax) - SECOND_AXES))

#define SAMPLE_INTERVAL mtic_freq // sample axis doesn't need mtics, so use the slot to hold sample interval 
//
// What kind of ticmarking is wanted?
//
enum t_ticseries_type {
    TIC_COMPUTED=1, 		/* default; gnuplot figures them */
    TIC_SERIES,			/* user-defined series */
    TIC_USER,			/* user-defined points */
    TIC_MONTH,   		/* print out month names ((mo-1)%12)+1 */
    TIC_DAY      		/* print out day of week */
};

enum td_type {
    DT_NORMAL=0,		/* default; treat values as pure numeric */
    DT_TIMEDATE,		/* old datatype */
    DT_DMS,			/* degrees minutes seconds */
    DT_UNINITIALIZED
};
//
// Defines one ticmark for TIC_USER style.
// If label==NULL, the value is printed with the usual format string.
// else, it is used as the format string (note that it may be a constant string, like "high" or "low").
//
struct ticmark {
	static void DestroyList(ticmark * pList)
	{
		while(pList) {
			ticmark * p_freeable = pList;
			pList = pList->next;
			free(p_freeable->label);
			free(p_freeable);
		}
	}
	ticmark()
	{
		position = 0.0;
		label = 0;
		level = 0;
		next = 0;
	}
    double position; // where on axis is this
    char * label;    // optional (format) string label
    int    level;    // 0=major tic, 1=minor tic
    ticmark * next;  // linked list
};

/* Tic-mark labelling definition; see set xtics */
struct t_ticdef /*ticdef*/ {
	t_ticdef()
	{
		SetDefault();
	}
	void   SetDefault()
	{
		//#define DEFAULT_AXIS_TICDEF {TIC_COMPUTED, NULL, {TC_DEFAULT, 0, 0.0}, {NULL, {0.,0.,0.}, false},  { character, character, character, 0., 0., 0. }, false, true, false }
		type = TIC_COMPUTED;
		font = 0;
		textcolor.Set(TC_DEFAULT, 0, 0.0);
		def.user = 0;
		def.series.start = 0.0;
		def.series.incr = 0.0;
		def.series.end = 0.0;
		def.mix = false;
		offset.Set(character, character, character, 0.0, 0.0, 0.0);
		rangelimited = false;
		enhanced = true;
		logscaling = false;
	}
    t_ticseries_type type;
    char * font;
    t_colorspec textcolor;
    struct {
	   ticmark * user; // for TIC_USER
	   struct { // for TIC_SERIES
		  double start;
		  double incr;
		  double end;		/* ymax, if GPVL */
	   } series;
	   bool mix;		/* true to use both the above */
    } def;
    GpPosition offset;
    bool rangelimited;		/* Limit tics to data range */
    bool enhanced;			/* Use enhanced text mode or labels */
    bool logscaling;		/* place tics suitably for logscaled axis */
};

/* we want two auto modes for minitics - default where minitics are
 * auto for log/time and off for linear, and auto where auto for all
 * graphs I've done them in this order so that logscale-mode can
 * simply test bit 0 to see if it must do the minitics automatically.
 * similarly, conventional plot can test bit 1 to see if minitics are
 * required */
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

#if 0 /* HBB 20010806 --- move GRID flags into axis struct */
/* Need to allow user to choose grid at first and/or second axes tics.
 * Also want to let user choose circles at x or y tics for polar grid.
 * Also want to allow user rectangular grid for polar plot or polar
 * grid for parametric plot. So just go for full configurability.
 * These are bitmasks
 */
#define GRID_OFF    0
#define GRID_X      (1<<0)
#define GRID_Y      (1<<1)
#define GRID_Z      (1<<2)
#define GRID_X2     (1<<3)
#define GRID_Y2     (1<<4)
#define GRID_MX     (1<<5)
#define GRID_MY     (1<<6)
#define GRID_MZ     (1<<7)
#define GRID_MX2    (1<<8)
#define GRID_MY2    (1<<9)
#define GRID_CB     (1<<10)
#define GRID_MCB    (1<<11)
#endif /* 0 */

#define RANGE_WRITEBACK   1
#define RANGE_SAMPLED     2
#define RANGE_IS_REVERSED 4

#define DEFAULT_AXIS_TICDEF {TIC_COMPUTED, NULL, {TC_DEFAULT, 0, 0.0}, {NULL, {0.,0.,0.}, false},  { character, character, character, 0., 0., 0. }, false, true, false }
#define DEFAULT_AXIS_ZEROAXIS {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}

struct GpAxis {
	static const lp_style_type DefaultAxisZeroAxis; // zeroaxis linetype (flag type==-3 if none wanted)

	GpAxis()
	{
		SetDefault();
	}
	//
	// Free dynamic fields in an axis structure so that it can be safely deleted
	// or reinitialized.  Doesn't free the axis structure itself.
	//
	void Destroy()
	{
		ZFREE(formatstring);
		ZFREE(ticfmt);
		if(link_udf) {
			ZFREE(link_udf->at);
			ZFREE(link_udf->definition);
			ZFREE(link_udf);
		}
		ticmark::DestroyList(ticdef.def.user);
		ticdef.def.user = 0;
		ZFREE(ticdef.font);
		label.UnsetAxisLabelOrTitle();
		if(zeroaxis != &DefaultAxisZeroAxis)
			ZFREE(zeroaxis);
	}
	void SetDefault()
	{
		AutoScale = AUTOSCALE_BOTH;
		SetAutoScale = AUTOSCALE_BOTH;
		range_flags = 0;
		Range.Set(-10.0, 10.0);
		SetRange.Set(-10.0, 10.0);
		//writeback_min = -10.0;
		//writeback_max = 10.0;
		//data_min = 0.0;
		//data_max = 0.0;
		WritebackRange.Set(-10.0, 10.0);
		DataRange.SetVal(0.0);

		min_constraint = CONSTRAINT_NONE;
		max_constraint = CONSTRAINT_NONE;
		Lb.SetVal(0.0);
		Ub.SetVal(0.0);
		//min_lb = 0.0;
		//min_ub = 0.0;
		//max_lb = 0.0;
		//max_ub = 0.0;

		//term_lower = 0;
		//term_upper = 0;
		TermBounds.Set(0);
		term_scale = 0.0;
		term_zero = 0;

		base = 0.0;
		log_base = 0.0;

		P_LinkToPrmr = 0;
		P_LinkToScnd = 0;
		link_udf = 0;

		ticmode = NO_TICS;
		ticdef.SetDefault();
		tic_rotate = 0;

		//IsLog = false;
		//gridmajor = false;
		//gridminor = false;
		//tic_in = true;
		//manual_justify = false;
		Flags = fTicIn;

		minitics = MINI_DEFAULT;
		mtic_freq = 10.0;
		ticscale = 1.0;
		miniticscale = 0.5;
		ticstep = 0.0;

		datatype = DT_NORMAL;
		tictype = DT_NORMAL;
		formatstring = 0;
		ticfmt = 0;
		timelevel = TIMELEVEL_UNDEF;
		Index = 0;
		label.SetEmpty();
		zeroaxis = 0;
	}
	void   Init(bool infinite)
	{
		AutoScale = SetAutoScale;
		Range.Set(((infinite && (SetAutoScale & AUTOSCALE_MIN)) ?  GPVL : SetRange.low), 
			((infinite && (SetAutoScale & AUTOSCALE_MAX)) ? -GPVL : SetRange.upp));
		DataRange.Set(GPVL, -GPVL);
		//data_min = GPVL;
		//data_max = -GPVL;
	}
	void   InitRefresh2D(bool infinite)
	{
		AutoScale = SetAutoScale;
		Range.low = (infinite && (SetAutoScale & AUTOSCALE_MIN)) ?  GPVL : LogValue(SetRange.low); 
		Range.upp = (infinite && (SetAutoScale & AUTOSCALE_MAX)) ? -GPVL : LogValue(SetRange.upp);
		log_base = (Flags & fLog) ? ::log(base) : 0.0;
	}
	void   UpdateRefresh2D()
	{
		if((SetAutoScale & AUTOSCALE_MIN) == 0)
			Range.low = LogValue(SetRange.low);
		if((SetAutoScale & AUTOSCALE_MAX) == 0)
			Range.upp = LogValue(SetRange.upp);
	}
	bool   ValidateValue(double v) const
	{
		// These are flag bits, not constants!!!
		if((AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_BOTH)
			return true;
		else if(((AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_MIN) && (v <= Range.upp))
			return true;
		else if(((AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_MAX) && (v >= Range.low))
			return true;
		else if(((AutoScale & AUTOSCALE_BOTH) == AUTOSCALE_NONE) && Range.Check(v))
			return(true);
		else
			return(false);
	}
	bool   TicsOn() const
	{
		return ((ticmode & TICS_MASK) != NO_TICS);
	}
	bool   IsRangeUndef() const
	{
		return (Range.upp == -GPVL || Range.low == GPVL);
	}
	bool   InRange(double val) const
	{
		return Range.CheckX(val) ? true : false;
	}
	int    Map(double value) const
	{
		return (int)(TermBounds.low + (value - Range.low) * term_scale + 0.5);
	}
	double MapBack(int pos) const
	{
		return (((double)(pos) - TermBounds.low) / term_scale + Range.low);
	}
	double DoLog(double value) const
	{
		return (::log(value) / log_base);
	}
	double UndoLog(double value) const
	{
		return ::exp(value * log_base);
	}
	double LogValue(double value) const
	{
		return (Flags & fLog) ? DoLog(value) : value;
	}
	double DeLogValue(double value) const
	{
		return (Flags & fLog) ? UndoLog(value) : value;
	}
	//
	// this is used in a few places all over the code: undo logscaling of
	// a given range if necessary. If checkrange is true, will int_error() if
	// range is invalid
	//
	void UnlogInterval(RealRange & rR, bool checkrange);
	void InvertIfRequested()
	{
		if((range_flags & RANGE_IS_REVERSED) && AutoScale && (Range.upp > Range.low))
			Exchange(&Range.low, &Range.upp);
	}
	void AdjustAutoscale(double value)
	{
		if(SetAutoScale & AUTOSCALE_MIN)
			SETMIN(Range.low, value);
		if(SetAutoScale & AUTOSCALE_MAX)
			SETMAX(Range.upp, value);
	}
	int SetAutoscaleMin(double setMin)
	{
		if(AutoScale & AUTOSCALE_MIN) {
			Range.low = setMin;
			return 1;
		}
		else
			return 0;
	}
	int SetAutoscaleMax(double setMax)
	{
		if(AutoScale & AUTOSCALE_MAX) {
			Range.upp = setMax;
			return 1;
		}
		else
			return 0;
	}
	int SetAutoscale(double low, double upp)
	{
		int    result = 0;
		if(AutoScale & AUTOSCALE_MIN) {
			Range.low = low;
			result |= 0x01;
		}
		if(AutoScale & AUTOSCALE_MAX) {
			Range.upp = upp;
			result |= 0x02;
		}
		return result;
	}
	double GetRange() const
	{
		return Range.GetDistance();
	}
	void   SetScaleAndRange(uint lo, uint up)
	{
		term_scale = (up - lo) / Range.GetDistance();
		TermBounds.Set(lo, up);
		//term_lower = lo;
		//term_upper = up;
	#ifdef NONLINEAR_AXES
		if(P_LinkToPrmr && P_LinkToPrmr->Index <= 0) {
			GpAxis * p_link = P_LinkToPrmr;
			p_link->term_scale = (up - lo) / p_link->Range.GetDistance();
			//p_link->term_lower = lo;
			//p_link->term_upper = up;
			p_link->TermBounds.Set(lo, up);
		}
	#endif
	}
	void   SetGrid(int _minor /* 0 - major, !0 - minor */, bool value)
	{
		if(_minor) {
			//gridminor = value;
			SETFLAG(Flags, fGridMinor, value);
		}
		else {
			//gridmajor = value;
			SETFLAG(Flags, fGridMajor, value);
		}
	}
	void   SetupTics(int aMax);
	//
	// process 'unset mxtics' command
	//
	void   UnsetMiniTics()
	{
		minitics = MINI_OFF;
		mtic_freq = 10.0;
	}
	void   UnsetTics()
	{
		ticmode = NO_TICS;
		ZFREE(ticdef.font);
		ticdef.textcolor.type = TC_DEFAULT;
		ticdef.textcolor.lt = 0;
		ticdef.textcolor.value = 0;
		ticdef.offset.Set(character, character, character, 0., 0., 0.);
		ticdef.rangelimited = false;
		ticdef.enhanced = true;
		tic_rotate = 0;
		ticscale = 1.0;
		miniticscale = 0.5;
		//tic_in = true;
		//manual_justify = false;
		Flags |= fTicIn;
		Flags &= ~fManualJustify;
		ticmark::DestroyList(ticdef.def.user);
		ticdef.def.user = NULL;
		if(Index >= PARALLEL_AXES)
			ticdef.rangelimited = true;
	}
	double EvalLinkFunction(double raw_coord) const;
	void   LoadOneRange(GpCommand & rC, double * pA, t_autoscale * pAutoscale, t_autoscale which);
	t_autoscale LoadRange(GpCommand & rC, /*double * pA, double * pB*/RealRange & rRange, t_autoscale autoscale);
	void   SetTimeData(GpCommand & rC);
	//
	// range of this axis
	//
    t_autoscale AutoScale;     // Which end(s) are autoscaled?
    t_autoscale SetAutoScale;  // what does 'set' think AutoScale to be?
    int    range_flags;        // flag bits about AutoScale/writeback:
	//
    // write auto-ed ranges back to variables for AutoScale
	//
	RealRange Range; // 'transient' axis extremal values
    //double set_min;       // set/show 'permanent' values
    //double set_max;
	RealRange SetRange; // set/show 'permanent' values
    //double writeback_min; // ULIG's writeback implementation
    //double writeback_max;
	RealRange WritebackRange; // ULIG's writeback implementation
    //double data_min;      // Not necessarily the same as axis min
    //double data_max;
	RealRange DataRange; // Not necessarily the same as axis min
	//
	// range constraints
	//
    t_constraint min_constraint;
    t_constraint max_constraint;
	RealRange Lb; // lower-bound
	RealRange Ub; // upper-bound
    //double min_lb;
	//double min_ub;     // min lower- and upper-bound
    //double max_lb;
	//double max_ub;     // min lower- and upper-bound
    //
	// output-related quantities
	//
    //int    term_lower; // low and high end of the axis on output,
    //int    term_upper; // ... (in terminal coordinates)
	IntRange TermBounds;
    double term_scale; // scale factor: plot --> term coords
    uint   term_zero;  // position of zero axis
	//
	// log axis control
	//
    double base;     // logarithm base value
    double log_base; // ln(base), for easier computations
	//
	// linked axis information (used only by x2, y2)
	// If axes are linked, the primary axis info will be cloned into the
	// secondary axis only up to this point in the structure.
	//
	GpAxis * P_LinkToPrmr; //Set only in a secondary axis
    GpAxis * P_LinkToScnd; // Set only in a primary axis
    UdftEntry * link_udf;
	//
	// ticmark control variables
	//
    int    ticmode;       // tics on border/axis? mirrored?
    t_ticdef ticdef;      // tic series definition
    int    tic_rotate;       // ticmarks rotated by this angle
    t_minitics_status minitics; // minor tic mode (none/auto/user)?
    double mtic_freq;    // minitic stepsize
    double ticscale;     // scale factor for tic marks (was (0..1])
    double miniticscale; // and for minitics
    double ticstep;      // increment used to generate tic placement

	enum {
		fLog           = 0x0001, // log axis stuff: flag "islog?"
		fTicIn         = 0x0002, // tics to be drawn inward?
		fGridMajor     = 0x0004, // Grid lines wanted on major tics?
		fGridMinor     = 0x0008, // Grid lines for minor tics?
		fManualJustify = 0x0010  // override automatic justification
	};
	long   Flags;
    //bool   IsLog;          // log axis stuff: flag "islog?"
	//bool   tic_in;         // tics to be drawn inward?
    //bool   gridmajor;      // Grid lines wanted on major tics?
    //bool   gridminor;      // Grid lines for minor tics?
    //bool   manual_justify; // override automatic justification
	//
	// time/date axis control
	//
    td_type datatype; // {DT_NORMAL|DT_TIMEDATE} controls _input_
    td_type tictype;  // {DT_NORMAL|DT_TIMEDATE|DT_DMS} controls _output_
    char * formatstring; // the format string for output
    char * ticfmt;       // autogenerated alternative to formatstring (needed??)
    t_timelevel timelevel; // minimum time unit used to quantize ticks
	//
	// other miscellaneous fields
	//
    int    Index; // if this is a permanent axis, this indexes GpGg[]
		// (index >= PARALLEL_AXES) indexes parallel axes
		// (index < 0) indicates a dynamically allocated structure
    GpTextLabel label;           // label string and position offsets
    lp_style_type *zeroaxis;	// usually points to GpAxis::DefaultAxisZeroAxis
};

// Function pointer type for callback functions to generate ticmarks

struct GpTicCallbackParam {
	GpTicCallbackParam(GpAxis * pAx, double place, char * pText, int ticLevel, lp_style_type style, ticmark * pM)
	{
		P_Ax = pAx;
		Place = place;
		P_Text = pText;
		TicLevel = ticLevel;
		Style = style;
		P_M = pM;
	}
	GpAxis * P_Ax;
	double Place;
	char * P_Text;
	int    TicLevel;
	lp_style_type Style;
	ticmark * P_M;
};

typedef void (*tic_callback)(GpAxis * pAx, double, char *, int, lp_style_type, ticmark *);
typedef void (GpGadgets:: *TicCallback)(GpTermEntry * pT, GpTicCallbackParam * pTcbP);

#define DEF_FORMAT       "% h"            // default format for tic mark labels
#define DEF_FORMAT_LATEX "$%h$"
#define TIMEFMT          "%d/%m/%y,%H:%M" // default parse timedata string
//
// This much of the axis structure is cloned by the "set x2range link" command
//
#define AXIS_CLONE_SIZE offsetof(GpAxis, P_LinkToPrmr)
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
// Tic levels 0 and 1 are maintained in the axis structure.
// Tic levels 2 - MAX_TICLEVEL have only one property - scale.
//
#define MAX_TICLEVEL 5
//
// macros to reduce code clutter caused by the array notation, mainly in graphics.c and fit.c
//
class GpAxisBlock {
public:
	GpAxisBlock();
	GpAxis & operator [] (size_t idx);
	const  char * GetAxisName(int axis);
	double GetTicScale(const GpTicCallbackParam * pP /*int ticLevel, const GpAxis * pAx*/) const;
	//
	// If we encounter a parallel axis index higher than any used so far,
	// extend parallel_axis[] to hold the corresponding data.
	// Returns pointer to the new axis.
	//
	GpAxis * ExtendParallelAxis(uint paxis);
	//
	void * CreateAxesCopy();
	void   RestoreAxesCopy(const void * pCopy);
	void   DestroyAxesCopy(void * pCopy);
	void   SetTicScale(GpCommand & rC);
	void   SetAutoscale(GpCommand & rC);
	bool   SomeGridSelected() const
	{
		for(/*AXIS_INDEX*/int i = FIRST_AXES; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++) {
			//if(AxA[i].gridmajor || AxA[i].gridminor)
			if(AxA[i].Flags & (GpAxis::fGridMajor|GpAxis::fGridMinor))
				return true;
		}
		return false;
	}
	void   SaveAutoscaledRanges(const GpAxis * pAxX, const GpAxis * pAxY);
	void   RestoreAutoscaledRanges(GpAxis * pXAx, GpAxis * pYAx);
	void   UnsetAxisLabel(AXIS_INDEX axis);
	int    ParseRange(AXIS_INDEX axis, GpCommand & rC);
	void   PolarRangeFiddling(const CurvePoints * plot);

	double TicScale[MAX_TICLEVEL];
	int    TicStart;
	int    TicDirection;
	int    TicMirror;
	int    TicText;
	int    RotateTics;
	int    TicHJust;
	int    TicVJust;
	//
	// axes being used by the current plot
	// These are mainly convenience variables, replacing separate copies of
	// such variables originally found in the 2D and 3D plotting code
	//
	AXIS_INDEX XAxis; //= FIRST_X_AXIS;
	AXIS_INDEX YAxis; //= FIRST_Y_AXIS;
	AXIS_INDEX ZAxis; // = FIRST_Z_AXIS;

	uint   NumParallelAxes;
	GpAxis * P_ParallelAxis;
	//
	//
	//
	const  t_ticdef default_axis_ticdef; // = DEFAULT_AXIS_TICDEF;
	const  GpTextLabel default_axis_label; // = EMPTY_LABELSTRUCT; // axis labels
	//
	// These are declare volatile in order to fool the compiler into not
	// optimizing out intermediate values, thus hiding loss of precision
	//
	volatile double vol_this_tic;
	volatile double vol_previous_tic;
	char * P_TimeFormat; // global default time format
	int    grid_layer;
	double polar_grid_angle; // nonzero means a polar grid 
	int    widest_tic_strlen; // Length of the longest tics label, set by GpGadgets::WidestTicCallback():
	bool   grid_tics_in_front;
	bool   raxis;
	bool   inside_zoom; // flag to indicate that in-line axis ranges should be ignored 
protected:
	//
	// Fill in the starting values for a just-allocated  parallel axis structure
	//
	//void init_parallel_axis(GpAxis * this_axis, AXIS_INDEX index)
	void InitParallelAxis(GpAxis * pAx, AXIS_INDEX index);

	GpAxis AxA[AXIS_ARRAY_SIZE];
	SString AxNameBuf;
	//
	// Only accessed by save_autoscaled_ranges() and restore_autoscaled_ranges()
	//
	RealRange SaveAutoScaleX;
	RealRange SaveAutoScaleY;
};
//
// GRAPHICS.H
//
struct CurvePoints {
    CurvePoints * P_Next; // pointer to next plot in linked list
    int    Token; // last token used, for second parsing pass
    enum   PLOT_TYPE  plot_type;   // DATA2D? DATA3D? FUNC2D FUNC3D? NODATA? 
    enum   PLOT_STYLE plot_style; // style set by "with" or by default
    char * title; // plot title, a.k.a. key entry 
    GpPosition * title_position; // title at {beginning|end|<xpos>,<ypos>}
    bool   title_no_enhanced; // don't typeset title in enhanced mode
    bool   title_is_filename; // true if title was auto-generated from filename
    bool   title_is_suppressed; // true if 'notitle' was specified 
    bool   noautoscale;         // ignore data from this plot during autoscaling 
    lp_style_type lp_properties;
    arrow_style_type arrow_properties;
    fill_style_type fill_properties;
    GpTextLabel * labels; // Only used if plot_style == LABELPOINTS 
    t_image image_properties; // only used if plot_style is IMAGE or RGB_IMAGE 
    UdvtEntry *sample_var; // Only used if plot has private sampling range 
    // 2D and 3D plot structure fields overlay only to this point 
    filledcurves_opts filledcurves_options;
    int    base_linetype; // before any calls to load_linetype(), lc variable 
		// analogous to hidden3d_top_linetype in graph3d.h 
    int    ellipseaxes_units; // Only used if plot_style == ELLIPSES
    histogram_style *histogram;	/* Only used if plot_style == HISTOGRAM */
    int    histogram_sequence;	/* Ordering of this dataset within the histogram */
    enum PLOT_SMOOTH plot_smooth; /* which "smooth" method to be used? */
    double smooth_parameter;	/* e.g. optional bandwidth for smooth kdensity */
    int    boxplot_factors;	/* Only used if plot_style == BOXPLOT */
    int    p_max;			/* how many points are allocated */
    int    p_count;		/* count of points in points */
    int    x_axis;			/* FIRST_X_AXIS or SECOND_X_AXIS */
    int    y_axis;			/* FIRST_Y_AXIS or SECOND_Y_AXIS */
    int    z_axis;			/* same as either x_axis or y_axis, for 5-column plot types */
    int    n_par_axes;		/* Only used for parallel axis plots */
    double **z_n;		/* Only used for parallel axis plots */
    double * varcolor;		/* Only used if plot has variable color */
    GpCoordinate * points;
};

enum t_procimg_action {
    IMG_PLOT,
    IMG_UPDATE_AXES,
    IMG_UPDATE_CORNERS
};
//
// Probably some of these could be made static 
//
class GpBoundary {
public:
	GpBoundary()
	{
		KeyEntryHeight = 0;
		key_point_offset = 0;
		key_col_wth = 0;
		yl_ref = 0;
		YLabelX = 0;
		Y2LabelX = 0;
		XLabelY = 0;
		X2LabelY = 0;
		YLabelY = 0;
		Y2LabelY = 0;
		xtic_y = 0;
		x2tic_y = 0;
		ytic_x = 0;
		y2tic_x = 0;
		KeyCols = 0;
		key_rows = 0;
		//
		key_sample_width = 0;    // width of line sample 
		key_sample_left = 0;     // offset from x for left of line sample 
		key_sample_right = 0;    // offset from x for right of line sample 
		key_text_left = 0;       // offset from x for left-justified text 
		key_text_right = 0;      // offset from x for right-justified text 
		key_size_left = 0;       // size of left bit of key (text or sample, depends on key->reverse) 
		key_size_right = 0;      // size of right part of key (including padding) 
		key_xleft = 0;           // Amount of space on the left required by the key 
		max_ptitl_len = 0;       // max length of plot-titles (keys) 
		ptitl_cnt = 0;           // count keys with len > 0  
		key_width = 0;           // calculate once, then everyone uses it 
		key_height = 0;          // ditto 
		key_title_height = 0;    // nominal number of lines * character height 
		key_title_extra = 0;     // allow room for subscript/superscript 
		time_y = 0;
		time_x = 0;
		title_y = 0;
		//
		xlablin = 0;
		x2lablin = 0;
		ylablin = 0;
		y2lablin = 0;
		titlelin = 0;
		xticlin = 0;
		x2ticlin = 0;
	}
	void   Boundary(GpTermEntry * pT, GpGadgets & rGg, const CurvePoints * pPlots, int count);
	void   DoKeyBounds(GpTermEntry * pT, GpGadgets & rGg, legend_key * key);
	void   DoKeyLayout(GpTermEntry * pT, GpGadgets & rGg, legend_key * pKey);
	void   DoKeySample(GpTermEntry * pT, GpGadgets & rGg, CurvePoints * pPlot, legend_key * key, char * pTitle, int xl, int yl);
	void   DoKeySamplePoint(GpTermEntry * pT, GpGadgets & rGg, CurvePoints * pPlot, legend_key * pKey, int xl, int yl);
	void   DrawTitles(GpTermEntry * pT, GpGadgets & rGg);
	void   DrawKey(GpTermEntry * pT, GpGadgets & rGg, legend_key * pKey, bool key_pass, int * xinkey, int * yinkey);

	int    KeyEntryHeight; // bigger of t->VChr, t->VTic 
	int    key_col_wth;
	int    key_rows;
	int    yl_ref;
	int    xtic_y;
	int    ytic_x;
	int    x2tic_y;
	int    y2tic_x;
	int    XLabelY;
private:
	int    key_point_offset; // offset from x for point sample 
	int    YLabelX;
	int    Y2LabelX;
	int    X2LabelY;
	int    YLabelY;
	int    Y2LabelY;
	int    KeyCols;
	//
	// privates for Boundary
	//
	int    key_sample_width;    // width of line sample 
	int    key_sample_left;     // offset from x for left of line sample 
	int    key_sample_right;    // offset from x for right of line sample 
	int    key_text_left;       // offset from x for left-justified text 
	int    key_text_right;      // offset from x for right-justified text 
	int    key_size_left;       // size of left bit of key (text or sample, depends on key->reverse) 
	int    key_size_right;      // size of right part of key (including padding) 
	int    key_xleft;           // Amount of space on the left required by the key 
	int    max_ptitl_len;       // max length of plot-titles (keys) 
	int    ptitl_cnt;           // count keys with len > 0  
	int    key_width;           // calculate once, then everyone uses it 
	int    key_height;          // ditto 
	int    key_title_height;    // nominal number of lines * character height 
	int    key_title_extra;     // allow room for subscript/superscript 
	int    time_y;
	int    time_x;
	int    title_y;
	//
	int    xlablin;
	int    x2lablin;
	int    ylablin;
	int    y2lablin;
	int    titlelin;
	int    xticlin;
	int    x2ticlin;
};
//
// UTIL3D.H
//
// HBB 990828: moved all those variable decl's and #defines to new
// file "graph3d.h", as the definitions are in graph3d.c, not in
// util3d.c. Include that file from here, to ensure everything is known
//
// All the necessary information about one vertex
//
struct GpVertex : RPoint3 {
	GpVertex()
	{
		Set(0.0, 0.0, 0.0);
		lp_style = 0;
		real_z = 0;
		label = 0;
#ifdef HIDDEN3D_VAR_PTSIZE
		original = 0
#endif
	}
	int    IsEqual(const GpVertex & rS) const
	{
		return feqeps(fabs(x - rS.x) + fabs(y - rS.y) + fabs(z - rS.z), 0.0, GpEpsilon);
	}
	int    IsUndefined() const
	{
		return (z == -2.0);
	}
	void   SetAsUndefined()
	{
		z = -2.0;
	}
    lp_style_type * lp_style;	// where to find point symbol type (if any) 
    double real_z;
    GpTextLabel * label;
#ifdef HIDDEN3D_VAR_PTSIZE // Needed for variable pointsize, but takes space 
    GpCoordinate * original;
#endif
};
//
// GRAPH3D.H
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

enum  t_contour_placement {
    // Where to place contour maps if at all
    CONTOUR_NONE,
    CONTOUR_BASE,
    CONTOUR_SRF,
    CONTOUR_BOTH
};

typedef double transform_matrix[4][4]; /* HBB 990826: added */

struct gnuplot_contours {
    gnuplot_contours *next;
    GpCoordinate * coords;
    char   isNewLevel;
    char   label[32];
    int    num_pts;
    double z;
};

struct iso_curve {
    iso_curve * next;
    int    p_max;   // how many points are allocated 
    int    p_count; // count of points in points 
    GpCoordinate * points;
};

struct SurfacePoints {
    SurfacePoints * next_sp; // pointer to next plot in linked list 
    int    token;            // last token used, for second parsing pass 
    PLOT_TYPE  plot_type;    // DATA2D? DATA3D? FUNC2D FUNC3D? NODATA? 
    PLOT_STYLE plot_style;   // style set by "with" or by default 
    char * title;            // plot title, a.k.a. key entry 
    GpPosition *title_position;	// title at {beginning|end|<xpos>,<ypos>} 
    lp_style_type lp_properties;
    arrow_style_type arrow_properties;
    fill_style_type fill_properties;	// FIXME: ignored in 3D 
    GpTextLabel *labels;	// Only used if plot_style == LABELPOINTS 
    t_image image_properties;	// only used if plot_style is IMAGE, RGBIMAGE or RGBA_IMAGE 
    UdvtEntry *sample_var;	// Only used if plot has private sampling range 
    // 2D and 3D plot structure fields overlay only to this point 
    bool   opt_out_of_hidden3d; // set by "nohidden" option to splot command 
    bool   opt_out_of_contours; // set by "nocontours" option to splot command 
    bool   opt_out_of_surface;  // set by "nosurface" option to splot command 
    bool   pm3d_color_from_column;
    bool   title_no_enhanced;	// don't typeset title in enhanced mode 
    bool   title_is_filename;	// not used in 3D 
    bool   title_is_suppressed;// true if 'notitle' was specified 
    bool   noautoscale;	// ignore data from this plot during autoscaling 

    int    hidden3d_top_linetype;	// before any calls to load_linetype() 
    int    has_grid_topology;
    int    iteration;		// needed for tracking iteration 
    // Data files only - num of isolines read from file. For functions,  
    // num_iso_read is the number of 'primary' isolines (in x direction) 
    int    num_iso_read;
    gnuplot_contours *contours; // NULL if not doing contours. 
    iso_curve *iso_crvs;	// the actual data 
    char pm3d_where[7];		// explicitly given base, top, surface 
};

enum WHICHGRID { 
	ALLGRID, 
	FRONTGRID, 
	BACKGRID, 
	BORDERONLY 
};

class GpGraphics {
public:
	GpGraphics()
	{
		loff.Set(first_axes, first_axes, first_axes, 0.0, 0.0, 0.0);
		roff.Set(first_axes, first_axes, first_axes, 0.0, 0.0, 0.0);
		toff.Set(first_axes, first_axes, first_axes, 0.0, 0.0, 0.0);
		boff.Set(first_axes, first_axes, first_axes, 0.0, 0.0, 0.0);
		BarSize = 1.0;
		BarLayer = LAYER_FRONT;
		//
		draw_contour = CONTOUR_NONE;
		clabel_interval = 20;
		clabel_start = 5; 
		P_ClabelFont = NULL;
		clabel_onecolor = false;
		draw_surface = true;
		implicit_surface = true;
		hidden3d = false;
		hidden3d_layer = LAYER_BACK;
		xmiddle = 0;
		ymiddle = 0;
		xscaler = 0;
		yscaler = 0;
		surface_rot_z = 30.0;
		surface_rot_x = 60.0;
		surface_scale = 1.0;
		surface_zscale = 1.0;
		surface_lscale = 0.0;
		mapview_scale = 1.0;
		splot_map = 0 /*false*/; // Set by 'set view map': 
		xyplane.Set(0.5, false); // position of the base plane, as given by 'set ticslevel' or 'set xyplane' 
		iso_samples_1 = ISO_SAMPLES;
		iso_samples_2 = ISO_SAMPLES;
		Scale3d.Set(0.0, 0.0, 0.0);
		Center3d.Set(0.0, 0.0, 0.0);
		floor_z = 0.0;
		ceiling_z = 0.0;
		base_z = 0.0; // made exportable for PM3D
		memzero(trans_mat, sizeof(trans_mat));
		axis3d_o_x = 0;
		axis3d_o_y = 0;
		axis3d_x_dx = 0;
		axis3d_x_dy = 0;
		axis3d_y_dx = 0;
		axis3d_y_dy = 0;
	}
	void   PlaceHistogramTitles(GpTermEntry * pT);
	void   PlotDots(GpTermEntry * pT, CurvePoints * plot);
	void   SetBars(GpCommand & rC);
	//
	//
	//
	int    FindMaxlKeys3D(SurfacePoints * plots, int count, int * kcnt);
	void   KeyText(int xl, int yl, char * text);

	void   UnsetCntrLabel();
	void   UnsetView();
	void   ResetBars();
	//
	//
	// 'set offset' status variables
	GpPosition loff;
	GpPosition roff;
	GpPosition toff;
	GpPosition boff;
	// 'set bar' status
	double BarSize;
	int    BarLayer;
	lp_style_type BarLp;
	//
	GpBoundary B;
	//
	//
	//
	int    xmiddle;
	int    ymiddle;
	int    xscaler;
	int    yscaler;
	double floor_z;
	double ceiling_z;
	double base_z; // made exportable for PM3D 
	transform_matrix trans_mat;
	RPoint3 Scale3d;
	RPoint3 Center3d;
	t_contour_placement draw_contour;
	int    clabel_start;
	int    clabel_interval;
	char * P_ClabelFont;
	bool   clabel_onecolor;
	bool   draw_surface;
	bool   implicit_surface;
	bool   hidden3d; // is hidden3d display wanted? 
	int    hidden3d_layer; // LAYER_FRONT or LAYER_BACK
	float  surface_rot_z;
	float  surface_rot_x;
	float  surface_scale;
	float  surface_zscale;
	float  surface_lscale;
	float  mapview_scale;
	int    splot_map;

	struct t_xyplane { 
		void   Set(double z, bool isAbs)
		{
			Z = z;
			IsAbsolute = isAbs;
		}
		double Z; 
		bool   IsAbsolute;
	};
	t_xyplane xyplane;
	int    iso_samples_1;
	int    iso_samples_2;
#ifdef USE_MOUSE
	int    axis3d_o_x;
	int    axis3d_o_y;
	int    axis3d_x_dx;
	int    axis3d_x_dy;
	int    axis3d_y_dx;
	int    axis3d_y_dy;
#endif
};
//
// One edge of the mesh. The edges are (currently) organized into a
// linked list as a method of traversing them back-to-front
//
struct GpEdge {
	long   v1;
	long   v2; // the vertices at either end
	int    style;  // linetype index
	lp_style_type * lp; // line/point style attributes
	long   next;   // index of next edge in z-sorted list
};
//
// Zoom queue
//
struct GpZoom {
	RealRange xRange;
	RealRange yRange;
	RealRange x2Range;
	RealRange y2Range;
	GpZoom * prev;
	GpZoom * next;
};

struct MpLayout {
	MpLayout()
	{
		auto_layout = false;
		current_panel = 0;
		num_rows = 0;
		num_cols = 0;
		row_major = false;
		downwards = true;
		act_row = 0;
		act_col = 0;
		Scale.Set(1.0);
		Offs.Set(0.0);
		auto_layout_margins = false;
		LMrg.Set(screen, screen, screen, 0.1, -1, -1);
		RMrg.Set(screen, screen, screen, 0.9, -1, -1);
		BMrg.Set(screen, screen, screen, 0.1, -1, -1);
		TMrg.Set(screen, screen, screen, 0.9, -1, -1);
		xspacing.Set(screen, screen, screen, 0.05, -1, -1);
		yspacing.Set(screen, screen, screen, 0.05, -1, -1);
		PrevSz.Set(0.0);
		PrefOffs.Set(0.0);
		prev_LMrg.SetDefaultMargin();
		prev_RMrg.SetDefaultMargin();
		prev_TMrg.SetDefaultMargin();
		prev_BMrg.SetDefaultMargin();
		title.SetEmpty();
		title_height = 0.0;
	}
	bool auto_layout; // automatic layout if true
	int current_panel; // initialized to 0, incremented after each plot
	int num_rows;      // number of rows in layout
	int num_cols;      // number of columns in layout
	bool row_major; // row major mode if true, column major else
	bool downwards; // prefer downwards or upwards direction
	int    act_row;     // actual row in layout
	int    act_col;     // actual column in layout
	//double xscale;      // factor for horizontal scaling
	//double yscale;      // factor for vertical scaling
	RPoint Scale;
	//double xoffset;     // horizontal shift
	//double yoffset;     // horizontal shift
	RPoint Offs;
	bool auto_layout_margins;
	GpPosition LMrg;
	GpPosition RMrg;
	GpPosition BMrg;
	GpPosition TMrg;
	GpPosition xspacing;
	GpPosition yspacing;
	RPoint PrevSz;
	RPoint PrefOffs;
	GpPosition prev_LMrg;
	GpPosition prev_RMrg;
	GpPosition prev_TMrg;
	GpPosition prev_BMrg;
	//
	// values before 'set multiplot layout'
	//
	GpTextLabel title; // goes above complete set of plots
	double title_height; // fractional height reserved for title
};
//
// PM3D.H
//
//#ifndef TERM_HELP
//
// Global options for pm3d algorithm (to be accessed by set / show)
//
//
// where to plot pm3d: base or top (color map) or surface (color surface)
// The string pm3d.where can be any combination of the #defines below.
// For instance, "b" plot at botton only, "st" plots firstly surface, then top, etc.
//
#define PM3D_AT_BASE	'b'
#define PM3D_AT_TOP	't'
#define PM3D_AT_SURFACE	's'
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
#define PM3D_SCANS_AUTOMATIC  'a'
#define PM3D_SCANS_FORWARD    'f'
#define PM3D_SCANS_BACKWARD   'b'
#define PM3D_DEPTH            'd'
//
// clipping method:
// PM3D_CLIP_1IN: all 4 points of the quadrangle must be defined and at least
//		   1 point of the quadrangle must be in the x and y ranges
// PM3D_CLIP_4IN: all 4 points of the quadrangle must be in the x and y ranges
//
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
    PM3D_WHICHCORNER_C1 = 0, 	/* corner 1: first scan, first point   */
    PM3D_WHICHCORNER_C2 = 1, 	/* corner 2: first scan, second point  */
    PM3D_WHICHCORNER_C3 = 2, 	/* corner 3: second scan, first point  */
    PM3D_WHICHCORNER_C4 = 3,	/* corner 4: second scan, second point */
    /* the rest can be in any order */
    PM3D_WHICHCORNER_MEAN    = 4, /* average z-value from all 4 corners */
    PM3D_WHICHCORNER_GEOMEAN = 5, /* geometrical mean of 4 corners */
    PM3D_WHICHCORNER_HARMEAN = 6, /* harmonic mean of 4 corners */
    PM3D_WHICHCORNER_MEDIAN  = 7, /* median of 4 corners */
    PM3D_WHICHCORNER_RMS     = 8, /* root mean square of 4 corners*/
    PM3D_WHICHCORNER_MIN     = 9, /* minimum of 4 corners */
    PM3D_WHICHCORNER_MAX     = 10,/* maximum of 4 corners */
    PM3D_COLOR_BY_NORMAL     = 11 /* derive color from surface normal (not currently used) */
};
//
// structure defining all properties of pm3d plotting mode
// (except for the properties of the smooth color box, see GpGg.ColorBox instead)
//
struct pm3d_struct {
	pm3d_struct()
	{
		/*
			pm3d_struct pm3d = {
				"s",                    // where[6] 
				PM3D_FLUSH_BEGIN,       // flush 
				0,                      // no flushing triangles 
				PM3D_SCANS_AUTOMATIC,   // scans direction is determined automatically 
				PM3D_CLIP_4IN,          // clipping: all 4 points in ranges 
				PM3D_EXPLICIT,          // implicit 
				PM3D_WHICHCORNER_MEAN,  // color from which corner(s) 
				1,                      // interpolate along scanline 
				1,                      // interpolate between scanlines 
				DEFAULT_LP_STYLE_TYPE   // for the border 
			};
		*/
		STRNSCPY(where, "s");
		flush = PM3D_FLUSH_BEGIN;
		ftriangles = 0;
		direction = PM3D_SCANS_AUTOMATIC;
		clip = PM3D_CLIP_4IN;
		implicit = PM3D_EXPLICIT;
		which_corner_color = PM3D_WHICHCORNER_MEAN;
		interp_i = 1;
		interp_j = 1;
		border.SetDefault2();
	}
	char where[7];	/* base, top, surface */
	char flush;   	/* left, right, center */
	char ftriangles;   	/* 0/1 (don't) draw flushing triangles */
	char direction;	/* forward, backward */
	char clip;		/* 1in, 4in */
	PM3D_IMPL_MODE implicit; // 1: [default] draw ALL surfaces with pm3d; 0: only surfaces specified with 'with pm3d'
	PM3D_WHICH_CORNERS2COLOR which_corner_color; // default: average color from all 4 points
	int interp_i;		/* # of interpolation steps along scanline */
	int interp_j;		/* # of interpolation steps between scanlines */
	lp_style_type border;	/* LT_NODRAW to disable.  From `set pm3d border <linespec> */
};

struct lighting_model {
  double strength;	/* 0 = no lighting model; 1 = full shading */
  double spec;		/* specular component 0-1 */
  double ambient;	/* ambient component 0-1 */
  double Phong;		/* Phong exponent */
  int    rot_z;		/* illumination angle */
  int    rot_x;		/* illumination angle */
  bool   fixed;	/* true means the light does not rotate */
};

// extern lp_style_type default_pm3d_border; // Used to initialize `set pm3d border`
//
// Structure for the GpGg.Mse.Ruler: on/off, position,...
//
struct GpMouseRuler {
	GpMouseRuler()
	{
		on = false;
		P.Set(0.0);
		P2.Set(0.0);
		px = 0;
		py = 0;
	}
	bool   on;
	RPoint P;
	RPoint P2; // GpGg.Mse.Ruler position in real units of the graph
	long   px;
	long   py; // GpGg.Mse.Ruler position in the viewport units 
};
//
// MOUSE.H
//
#include "mousecmn.h"

extern char mouse_fmt_default[]; // @global @really const
extern long mouse_mode;
extern char * mouse_alt_string;

enum {
    MOUSE_COORDINATES_REAL = 0,
    MOUSE_COORDINATES_REAL1, /* w/o brackets */
    MOUSE_COORDINATES_FRACTIONAL,
#if 0
    MOUSE_COORDINATES_PIXELS,
    MOUSE_COORDINATES_SCREEN,
#endif
    MOUSE_COORDINATES_TIMEFMT,
    MOUSE_COORDINATES_XDATE,
    MOUSE_COORDINATES_XTIME,
    MOUSE_COORDINATES_XDATETIME,
    MOUSE_COORDINATES_ALT // alternative format as specified by the user 
};

#define DEFAULT_MOUSE_MODE    1 // start with mouse on by default 

class GpMouse {
public:
	struct Settings {
		Settings()
		{
			SetDefault();
		}
		void   SetDefault()
		{
			on = DEFAULT_MOUSE_MODE;
			doubleclick = 300; // ms
			annotate_zoom_box = 1;
			label = 0;
			polardistance = 0;
			verbose = 0;
			warp_pointer = 0;
			xmzoom_factor = 1.0;
			ymzoom_factor = 1.0;
			fmt = mouse_fmt_default;
			labelopts = 0;
		}
		int    on;                // ...                                         
		int    doubleclick;       // Button1 double / single click resolution    
		int    annotate_zoom_box; // draw coordinates at zoom box                
		int    label;             // draw real gnuplot labels on Button 2        
		int    polardistance;     // display dist. to ruler in polar coordinates 
		int    verbose;           // display ipc commands                        
		int    warp_pointer;      // warp pointer after starting a zoom box      
		double xmzoom_factor;  // scale factor for +/- zoom on x		  
		double ymzoom_factor;  // scale factor for +/- zoom on y		  
		char * fmt;            // fprintf format for printing numbers         
		char * labelopts;      // label options                               
	};
	struct Bind {
		int    key;
		char   modifier;
		char * command;
		bool   allwindows;
		Bind * prev;
		Bind * next;
		char * (*builtin)(GpEvent * pGe);
	};
	GpMouse() : NO_KEY(-1)
	{
		P_Bindings = 0;
		TrapRelease = false;
		setting_zoom_region = false;
		//setting_zoom_x = 0;
		//setting_zoom_y = 0;
		SettingZoomP = (int)0;

		//mouse_x = -1;
		//mouse_y = -1; // the coordinates of the mouse cursor in gnuplot's internal GpCoordinate system
		MP = -1;
		/*
		real_x = 0.0;
		real_y = 0.0;
		real_x2 = 0.0;
		real_y2 = 0.0; // the "real" coordinates of the mouse cursor, i.e., in the user's GpCoordinate system(s)
		*/
		RealP.Set(0.0);
		RealP2.Set(0.0);
		button = 0; // status of buttons; button i corresponds to bit (1<<i) of this variable
		needreplot = false; // did we already postpone a replot because allowmotion was false ? 
		//start_x = 0;
		//start_y = 0; // mouse position when dragging started 
		SP = (int)0;
		motion = 0; // ButtonPress sets this to 0, ButtonMotion to 1 
		zero_rot_x = 0.0f;
		zero_rot_z = 0.0f; // values for rot_x and rot_z corresponding to zero position of mouse 
		modifier_mask = 0; // the status of the shift, ctrl and alt keys
	}
	void   BindRemove(GpMouse::Bind * b);
	void   BindAppend(char * lhs, char * rhs, char *(*builtin)(GpEvent* ge));
	void   BindInstallDefaultBindings();
	void   BindAll(char * lhs);
	void   BindRemoveAll();
	void   BindProcess(char * lhs, char * rhs, bool allwindows);

	GpMouseRuler Ruler;
	GpMouse::Settings Cfg;
	//
	// variables for setting the zoom region:
	//
	bool   setting_zoom_region; // flag, true while user is outlining the zoom region 
	// coordinates of the first corner of the zoom region, in the internal GpCoordinate system 
	TPoint SettingZoomP;
	//
	// bind related stuff 
	//
	GpMouse::Bind * P_Bindings;
	const  int NO_KEY;
	bool   TrapRelease;
	bool   needreplot; // did we already postpone a replot because allowmotion was false ? 

	TPoint MP; // the coordinates of the mouse cursor in gnuplot's internal GpCoordinate system
	TPoint SP; // mouse position when dragging started 
	RPoint RealP;
	RPoint RealP2; // the "real" coordinates of the mouse cursor, i.e., in the user's GpCoordinate system(s)
	int    button; // status of buttons; button i corresponds to bit (1<<i) of this variable
	int    motion; // ButtonPress sets this to 0, ButtonMotion to 1 
	int    modifier_mask; // the status of the shift, ctrl and alt keys
	float  zero_rot_x;
	float  zero_rot_z; // values for rot_x and rot_z corresponding to zero position of mouse 
};

#define spline_coeff_size 4
typedef double spline_coeff[spline_coeff_size];

class GpGadgets : public GpAxisBlock, public GpGraphics {
public:
	void   XTickCallback(GpTermEntry * pT, GpTicCallbackParam * pP/*GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels*/);
	void   YTickCallback(GpTermEntry * pT, GpTicCallbackParam * pP/*GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels*/);
	void   ZTickCallback(GpTermEntry * pT, GpTicCallbackParam * pP/*GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels*/);
	void   YTick2D_Callback(GpTermEntry * pT, GpTicCallbackParam * pP);
	void   XTick2D_Callback(GpTermEntry * pT, GpTicCallbackParam * pP);
	void   WidestTicCallback(GpTermEntry * pT, GpTicCallbackParam * pP);
	void   CbTickCallback(GpTermEntry * pT, GpTicCallbackParam * pP);

	GpGadgets() :
		DefaultBorderLp(lp_style_type::defBorder),
		BackgroundLp(lp_style_type::defBkg),
		BorderLp(lp_style_type::defBorder),
		DefaultFillStyle(FS_EMPTY, 100, 0, t_colorspec(TC_DEFAULT, 0, 0.0)),
		DefaultRectangle(t_object::defRectangle),
		DefaultCircle(t_object::defCircle),
		DefaultEllipse(t_object::defEllipse),
		Ev(*this),
		Gp__C(*this)
	{
		State_ = 0;
		P_Clip = &PlotBounds;
		XSz = 1.0f;              // scale factor for size 
		YSz = 1.0f;              // scale factor for size 
		ZSz = 1.0f;              // scale factor for size 
		XOffs = 0.0f;            // x origin 
		YOffs = 0.0f;            // y origin 
		AspectRatio = 0.0f;       // don't attempt to force it 
		AspectRatio3D = 0;       // 2 will put x and y on same scale, 3 for z also 
		// EAM Augest 2006 - redefine margin as GpPosition so that absolute placement is possible
		LMrg.SetDefaultMargin(); // space between left edge and GpGg.PlotBounds.xleft in chars (-1: computed) 
		BMrg.SetDefaultMargin(); // space between bottom and GpGg.PlotBounds.ybot in chars (-1: computed) 
		RMrg.SetDefaultMargin(); // space between right egde and GpGg.PlotBounds.xright in chars (-1: computed) 
		TMrg.SetDefaultMargin(); // space between top egde and GpGg.PlotBounds.ytop in chars (-1: computed) 

		first_custom_dashtype = 0;
		first_arrow = 0;
		first_label = 0;
		first_linestyle = 0;
		first_perm_linestyle = 0;
		first_mono_linestyle = 0;
		first_arrowstyle = 0;
#ifdef EAM_OBJECTS
		first_object = 0;
#endif
		timelabel_rotate = false;
		timelabel_bottom = true;
		Zero = ZERO; // zero threshold, may _not_ be 0! 
		PtSz = 1.0;
		PtIntervalBox = 1.0;

		// set border
		DrawBorder = 31;   // The current settings
		UserBorder = 31;   // What the user last set explicitly
		BorderLayer = LAYER_FRONT;
		// set samples 
		Samples1 = SAMPLES;
		Samples2 = SAMPLES;
		RefreshNPlots = 0; // FIXME: do_plot should be able to figure this out on its own! 
		CurrentX11WindowId = 0; // WINDOWID to be filled by terminals running on X11 (x11, wxt, qt, ...) 
		// set angles 
		Ang2Rad = 1.0;         /* 1 or pi/180, tracking angles_format */
		DataStyle = POINTSTYLE;
		FuncStyle = LINES;
		RefreshOk = E_REFRESH_NOT_OK; /* Flag to signal that the existing data is valid for a quick refresh */

		IsPolar = false;
		ClipLines1 = false;
		ClipLines2 = false;
		ClipPoints = false;
		IsParametric = false;
		InParametric = false;
		Is3DPlot = false; // If last plot was a 3d one
		IsVolatileData = false;
		//
		IsInteractive = true;
		noinputfiles = true;
		persist_cl = false;
		reading_from_dash = false;
		skip_gnuplotrc = false;
		ctrlc_flag = false;
		CallFromRexx = false;
		user_shell = 0;
		//
		P_FirstPlot = NULL;
		boxwidth = -1.0; // box width (automatic)
		boxwidth_is_absolute = true;  // whether box width is absolute (default) or relative
		histogram_rightmost = 0.0;    // Highest x-coord of histogram so far 
		StackCount = 0;               // counter for stackheight 
		StackHeight = NULL;           // Scratch space for y autoscale 
		//
		LargestPolarCircle = 0.0;
		//stackheight = NULL;
		P_PrevRowStackHeight = NULL;
		//stack_count = 0;
		PrevRowStackCount = 0;
		BoxPlotFactorSortRequired = false;
		//
		mapping3d = MAP3D_CARTESIAN;
		dgrid3d_row_fineness = 10;
		dgrid3d_col_fineness = 10;
		dgrid3d_norm_value = 1;
		dgrid3d_mode = DGRID3D_QNORM;
		dgrid3d_x_scale = 1.0;
		dgrid3d_y_scale = 1.0;
		dgrid3d = false;
		dgrid3d_kdensity = false;
		prefer_line_styles = false;
		IsMonochrome = false; // true if "set monochrome" 
		IsMultiPlot = false;  // true if in multiplot mode 
		screen_ok = false;
		//
		P_First3DPlot = NULL;
		plot3d_num = 0;
		//
		term_initialised = false; // mouse module needs this 
		term_graphics = false; // true if terminal is in graphics mode
		term_suspended = false; // we have suspended the driver, in multiplot mode 
		opened_binary = false;  // true if? 
		term_force_init = false; // true if require terminal to be initialized 
		term_pointsize = 1.0;    // internal pointsize for do_point 
	}
	//
	// Descr: Головная процедура. Вызывается из main()
	//
	int    Run(int argc, char ** argv);

#if defined(__GNUC__)
	void   CommonErrorExit() __attribute__((noreturn);
#elif defined(_MSC_VER)
	__declspec(noreturn) void CommonErrorExit();
#else
	void   CommonErrorExit();
#endif
	//
	void   IntError(int t_num, const char * pStr, ...);
	void   IntErrorCurToken(const char * pStr, ...);
	void   IntErrorNoCaret(const char * pStr, ...);
	void   IntWarn(int t_num, const char * str, ...);

	void   InitSession(GpCommand & rC);
	void   InitConstants();
	void   InitColor();
	void   ResetPalette();

	void   TermInitialise();
	void   TermSetOutput(GpTermEntry * pT, char * pDest);
	int    ComLine(GpCommand & rC);

	GpAxis & GetX() 
	{
		return AxA[XAxis];
	}
	GpAxis & GetY() 	
	{
		return AxA[YAxis];
	}
	GpAxis & GetZ() 
	{
		return AxA[ZAxis];
	}
	GpAxis & GetR() 
	{
		return AxA[POLAR_AXIS];
	}
	GpAxis & GetCB() 
	{
		return AxA[COLOR_AXIS];
	}
	void   InitAxis(AXIS_INDEX axIdx, bool infinite)
	{
		AxA[axIdx].Init(infinite);
	}
	bool   ValidateData(double v, int axIdx) const
	{
		return AxA[axIdx].ValidateValue(v);
	}
	//
	void   RevertRange(AXIS_INDEX axis)
	{
		AxA[axis].InvertIfRequested();
	}
	void   RevertAndUnlogRange(AXIS_INDEX axis)
	{
		GpAxis & r_ax = AxA[axis];
		r_ax.InvertIfRequested();
		r_ax.UnlogInterval(r_ax.Range, true);
	}
	void   SetTicMode(int axIdx, int setFlag, int resetFlag)
	{
		if(axIdx < 0) {
			for(size_t i = 0; i < SIZEOFARRAY(AxA); i++) {
				GpAxis & r_ax = AxA[i];
				r_ax.ticmode |= setFlag;
				r_ax.ticmode &= ~resetFlag;
			}
		}
		else if(axIdx < SIZEOFARRAY(AxA)) {
			GpAxis & r_ax = AxA[axIdx];
			r_ax.ticmode |= setFlag;
			r_ax.ticmode &= ~resetFlag;
		}
	}
	void   SetTics(GpCommand & rC);
	void   LoadTicUser(GpCommand & rC, GpAxis * pAx);
	void   LoadTicSeries(GpCommand & rC, GpAxis * pAx);
	int    Map(AXIS_INDEX axis, double value) const
	{
		return AxA[axis].Map(value);
	}
	double MapBack(AXIS_INDEX axis, int pos) const
	{
		return AxA[axis].MapBack(pos);
	}
	double DoLog(AXIS_INDEX axIdx, double value) const
	{
		return AxA[axIdx].DoLog(value);
	}
	double UndoLog(AXIS_INDEX axIdx, double value) const
	{
		return AxA[axIdx].UndoLog(value);
	}
	double LogValue(AXIS_INDEX axIdx, double value) const
	{
		const GpAxis & r_ax = AxA[axIdx];
		return (r_ax.Flags & GpAxis::fLog) ? r_ax.DoLog(value) : value;
	}
	double LogValueChecked(AXIS_INDEX axis, double coord, const char * pWhat);
	double DelogValue(AXIS_INDEX axIdx, double coord) const
	{
		const GpAxis & r_ax = AxA[axIdx];
		return (r_ax.Flags & GpAxis::fLog) ? r_ax.UndoLog(coord) : coord;
	}
	void   AxisCheckedExtendEmptyRange(AXIS_INDEX axis, const char * mesg);
	void   FillGpValAxis(AXIS_INDEX axis);
	double Rescale(int axIdx, double w1, double w2) const;
	void   RescaleAroundMouse(double * newmin, double * newmax, int axIdx, double mouse_pos, double scale) const;
	void   ZoomRescaleXYX2Y2(GpTermEntry * pT, double a0, double a1, double a2, double a3, double a4, double a5, double a6,
		double a7, double a8, double a9, double a10, double a11, double a12, double a13, double a14, double a15, char msg[]);
	void   ZoomInX(GpTermEntry * pT, int zoom_key);
	void   LoadMouseVariables(double x, double y, bool button, int c);
	void   GetViewPortX(RealRange & rVpR)
	{
		const GpAxis & r_ax = GetX();
		rVpR.low = (r_ax.SetAutoScale & AUTOSCALE_MIN) ? r_ax.Range.low : r_ax.SetRange.low;
		rVpR.upp = (r_ax.SetAutoScale & AUTOSCALE_MAX) ? r_ax.Range.upp : r_ax.SetRange.upp;
	}
	void   GetViewPortY(RealRange & rVpR)
	{
		const GpAxis & r_ax = GetY();
		rVpR.low = (r_ax.SetAutoScale & AUTOSCALE_MIN) ? r_ax.Range.low : r_ax.SetRange.low;
		rVpR.upp = (r_ax.SetAutoScale & AUTOSCALE_MAX) ? r_ax.Range.upp : r_ax.SetRange.upp;
	}
	void   GetViewPortZ(RealRange & rVpR)
	{
		const GpAxis & r_ax = GetZ();
		rVpR.low = (r_ax.SetAutoScale & AUTOSCALE_MIN) ? r_ax.Range.low : r_ax.SetRange.low;
		rVpR.upp = (r_ax.SetAutoScale & AUTOSCALE_MAX) ? r_ax.Range.upp : r_ax.SetRange.upp;
	}
	int    AssignLabelTag();
	void   MousePosToGraphPosReal(int xx, int yy, double * x, double * y, double * x2, double * y2);
	void   RecalcRulerPos();
	void   RecalcStatusLine();
	void   ChangeView(GpTermEntry * pT, GpCommand & rC, int x, int z);
	void   UpdateRuler(GpTermEntry * pT);
	void   CheckAxisReversed(AXIS_INDEX axis);
	bool   InAxisRange(double val, int axIdx) const;
	bool   InAxisRange2(double val1, int axIdx1, double val2, int axIdx2) const;
	bool   InAxisRange3(double val1, int axIdx1, double val2, int axIdx2, double val3, int axIdx3) const;
	void   InitSampleRange(const GpAxis * pAx);
	void   RRangeToXY();
	void   GenTics(GpTermEntry * pT, GpAxis & rAx, /*tic_callback*/TicCallback callback);
	void   SetCbMinMax();
	void   AxisOutputTics(GpTermEntry * pT, AXIS_INDEX axIdx, int * pTicLabelPosition, AXIS_INDEX zeroAxisBasis, /*tic_callback*/TicCallback callback);
	bool   PositionZeroAxis(AXIS_INDEX axis);
	void   Edge3DIntersect(GpCoordinate * p1, GpCoordinate * p2, double * ex, double * ey, double * ez);

	void   LoadRcFile(GpCommand & rC, int where);

	void   TermStartPlot(GpTermEntry * pT);
	void   TermEndPlot(GpTermEntry * pT);
	void   TermStartMultiplot(GpTermEntry * pT, GpCommand & rC);
	void   TermEndMultiplot(GpTermEntry * pT);

	void   ClipVector(GpTermEntry * pT, uint x, uint y);
	bool   TwoEdgeIntersect(GpCoordinate * pPoints, int i, double * pLx, double * pLy);

	double MapX3D(double v);
	double MapY3D(double v);
	double MapZ3D(double v);
	void   Map3DXYZ(double x, double y, double z /* user coordinates */, GpVertex * out);
	void   Map3DXY(double x, double y, double z, double * xt, double * yt);
	void   Map3DXY(double x, double y, double z, int * xt, int * yt);
	int    Map3DGetPosition(GpPosition * pos, const char * what, double * xpos, double * ypos, double * zpos);
	void   Map3DPositionR(GpPosition & rPos, int * pX, int * pY, const char * pWhat);
	void   Map3DPositionDouble(GpPosition & rPos, double * pX, double * pY, const char * pWhat);
	void   Map3DPosition(GpPosition & rPos, int * pX, int * pY, const char * pWhat);
	int    NearestLabelTag(GpTermEntry * pT, int xref, int yref);
	void   RemoveLabel(GpTermEntry * pT, GpCommand & rC, int x, int y);

	void   ParseLightingOptions(GpCommand & rC);

	bool   NeedFillBorder(GpTermEntry * pT, fill_style_type * pFillStyle);
	void   ApplyHeadProperties(const arrow_style_type & rArrowProperties);
	void   ApplyLpProperties(GpTermEntry * pT, lp_style_type * pLp);
	void   ApplyPm3DColor(GpTermEntry * pT, t_colorspec * pTc);
	int    ApplyLightingModel(GpCoordinate * pV0, GpCoordinate * pV1, GpCoordinate * pV2, GpCoordinate * pV3, double gray);
	int    MakePalette(GpTermEntry * pT);
	void   GetArrow(arrow_def * pArrow, int* pSx, int* pSy, int* pEx, int* pEy);

	void   Boundary3D(SurfacePoints * plots, int count);
	void   Setup3DBoxCorners();
	bool   GetArrow3D(arrow_def* arrow, int* sx, int* sy, int* ex, int* ey);
	void   AdjustOffsets();
	void   LoadFile(FILE * pFp, char * pFileName, int calltype);
	void   Eval3DPlots(GpCommand & rC);
	void   MultiplotStart(GpTermEntry * pT, GpCommand & rC);
	void   MultiplotEnd();
	void   DoEvent(GpTermEntry * pT, GpEvent * pGe);
	void   DoPlot(CurvePoints * plots, int pcount);
	void   DoSave3DPlot(GpCommand & rC, SurfacePoints * plots, int pcount, int quick);
	void   DoStringReplot(GpCommand & rC, const char * pStr);
	void   DoEllipse(GpTermEntry * pT, int dimensions, t_ellipse * pEllipse, int style, bool doOwnMapping);
	void   DoRectangle(GpTermEntry * pT, int dimensions, t_object * pObj, fill_style_type * pFillStyle);
	void   DoPolygon(GpTermEntry * pT, int dimensions, t_polygon * pPolygon, int style, t_clip_object clip);
	void   ReplotRequest(GpCommand & rC);
	void   RefreshRequest();
	void   Refresh3DBounds(SurfacePoints * pFirstPlot, int nPlots);
	void   ProcessImage(GpTermEntry * pT, void * pPlot, t_procimg_action action);
	void   PlotSteps(GpTermEntry * pT, CurvePoints * pPlot);
	void   PlotFSteps(GpTermEntry * pT, CurvePoints * pPlot);
	void   PlotHiSteps(GpTermEntry * pT, CurvePoints * pPlot);
	void   PlotCBars(GpTermEntry * pT, CurvePoints * plot);
	void   PlotFBars(GpTermEntry * pT, CurvePoints * plot);
	void   PlotBoxPlot(GpTermEntry * pT, CurvePoints * pPlot);
	void   PlotVectors(GpTermEntry * pT, CurvePoints * plot);
	void   PlotPoints(GpTermEntry * pT, CurvePoints * pPlot);
	void   PlotLines(GpTermEntry * pT, CurvePoints * plot);
	void   PlotImpulses(GpTermEntry * pT, CurvePoints * plot, int yaxis_x, int xaxis_y);
	void   PlotBoxes(GpTermEntry *pT, CurvePoints * pPlot, int xAxisY);
	void   PlotBars(GpTermEntry *pT, CurvePoints * plot);
	void   PlotBetweenCurves(GpTermEntry * pT, CurvePoints * pPlot);
	void   PlotBorder(GpTermEntry * pT);
	void   PlotFilledCurves(GpTermEntry * pT, CurvePoints * pPlot);
	void   PlotEllipses(GpTermEntry * pT, CurvePoints * plot);
	void   FillBetween(double x1, double xu1, double yl1, double yu1, double x2, double xu2, double yl2, double yu2, CurvePoints * pPlot);
	void   FinishFilledCurve(GpTermEntry * pT, int points, gpiPoint * corners, CurvePoints * pPlot);
	void   Do3DPlot(GpTermEntry * pT, SurfacePoints * pPlots, int pcount/* count of plots in linked list */, int quick/* !=0 means plot only axes etc., for quick rotation */);
	void   Plot3DImpulses(SurfacePoints * plot);
	void   Plot3DLines(SurfacePoints * plot);
	void   Plot3DLinesPm3D(SurfacePoints * plot);
	void   Plot3DPoints(GpTermEntry * pT, SurfacePoints * plot);
	void   PolyLine3DStart(GpTermEntry * pT, GpVertex & rV1);
	void   PolyLine3DNext(GpTermEntry * pT, GpVertex & rV2, lp_style_type * pLp);
	void   Cntr3DLabels(gnuplot_contours * cntr, char * level_text, GpTextLabel * label);
	void   Cntr3DImpulses(gnuplot_contours * cntr, lp_style_type * lp);
	void   Cntr3DLines(gnuplot_contours * cntr, lp_style_type * lp);
	void   Cntr3DPoints(gnuplot_contours * cntr, lp_style_type * lp);
	void   Pm3DRearRangePart(iso_curve * src, const int len, iso_curve *** dest, int * invert);
	void   Pm3DDepthQueueFlush(GpTermEntry * pT);
	void   KeySampleLinePm3D(GpTermEntry * pT, SurfacePoints * plot, int xl, int yl);
	void   KeySamplePointPm3D(GpTermEntry * pT, SurfacePoints * plot, int xl, int yl, int pointtype);
	void   BoxPlotRangeFiddling(CurvePoints * pPlot);
	void   Store2DPoint(CurvePoints * pPlot, int i /* point number */, double x, double y, double xlow, double xhigh,
		double ylow, double yhigh, double width /* BOXES widths: -1 -> autocalc, 0 ->  use xlow/xhigh */);
	int    GetData(CurvePoints * pPlot);
	void   HistogramRangeFiddling(CurvePoints * pPlot);
	void   FilledPolygonCommon(GpTermEntry * pT, int points, const GpCoordinate & rCoords, bool fixed, double z);
	void   FilledPolygon3DCoords(GpTermEntry * pT, int points, const GpCoordinate & rCoords);
	void   FilledPolygon3DCoords_ZFixed(GpTermEntry * pT, int points, const GpCoordinate & rCoords, double z);
	void   ParametricFixup(CurvePoints * pStartPlot, int * pPlotNum);
	void   RefreshBounds(GpTermEntry * pT, CurvePoints * pFirstPlot, int nplots);
	void   MpLayoutSizeAndOffset();
	void   MpLayoutMarginsAndSpacing(GpTermEntry * pT);
	void   MultiplotNext(GpTermEntry * pT);
	void   MultiplotPrevious(GpTermEntry * pT);
	int    MultiplotCurrentPanel() const;
	void   CpImplode(CurvePoints * pCp);
	spline_coeff * CpTriDiag(CurvePoints * pPlot, int firstPoint, int numPoints);
	void   McsInterp(CurvePoints * pPlot);
	void   DoFreq(CurvePoints * pPlot, int firstPoint, int numPoints);
	void   DoKDensity(CurvePoints * cp, int first_point, int num_points, GpCoordinate * dest);
	void   GenInterp(CurvePoints * pPlot);
	
	char * BuiltInNearestLog(GpCommand & rC, GpEvent * ge);
	char * BuiltInToggleLog(GpCommand & rC, GpEvent * ge);
	void   EventMotion(GpCommand & rC, GpEvent * ge);
	void   GetRulerString(char * p, double x, double y);
	void   UpdateStatuslineWithMouseSetting(GpTermEntry * pT, GpMouse::Settings * ms);
	void   UpdateStatusline(GpTermEntry * pT);

	void   PlaceLabels(GpTermEntry * pT, GpTextLabel * listhead, int layer, bool clip);
	void   PlaceLabels3D(GpTextLabel * listhead, int layer);
	void   PlaceArrows3D(int layer);
	void   PlaceArrows(GpTermEntry * pT, int layer);
	void   PlaceGrid(GpTermEntry * pT, int layer);
	void   PlaceObjects(GpTermEntry * pT, t_object * pListHead, int layer, int dimensions);
	void   PlaceRAxis(GpTermEntry * pT);
	void   PlaceParallelAxes(GpTermEntry * pT, const CurvePoints * pFirstPlot, int pcount, int layer);
	void   AttachTitleToPlot(GpTermEntry * pT, CurvePoints * pPlot, legend_key * pKey);
	void   GetOffsets(GpTextLabel * pLabel, int * pHTic, int * pVTic);
	char * GetAnnotateString(char * s, double x, double y, int mode, char * fmt);

	void   SetPlotWithPalette(int plot_num, int plot_mode);
	void   SetRgbColorVar(GpTermEntry * pT, uint rgbvalue);
	void   SetRgbColorConst(GpTermEntry * pT, uint rgbvalue);
	double QuantizeGray(double gray);
	void   RGB1FromGray(double gray, rgb_color * pColor);
	void   RGB1MaxColorsFromGray(double gray, rgb_color * pColor);
	int    InterpolateColorFromGray(double gray, rgb_color * pColor);
	void   ColorComponentsFromGray(double gray, rgb_color * pColor);
	int    CalculateColorFromFormulae(double gray, rgb_color * pColor);
	void   CalculateSetOfIsolines(AXIS_INDEX value_axis,
		bool cross, iso_curve ** this_iso, AXIS_INDEX iso_axis,
		double iso_min, double iso_step, int num_iso_to_use, AXIS_INDEX sam_axis,
		double sam_min, double sam_step, int num_sam_to_use);
	gnuplot_contours * Contour(int numIsoLines, iso_curve * pIsoLines);
	void   IFilledQuadrangle(GpTermEntry * pT, gpiPoint * pICorners);
#ifdef EXTENDED_COLOR_SPECS
	void   FilledQuadrangle(GpTermEntry * pT, gpdPoint * corners, gpiPoint * icorners);
#else
	void   FilledQuadrangle(GpTermEntry * pT, gpdPoint * corners);
#endif
	void   DrawClipLine(GpTermEntry * pT, int x1, int y1, int x2, int y2);
	void   DrawClipPolygon(GpTermEntry * pT, int points, gpiPoint * p);
	void   DrawClipArrow(GpTermEntry * pT, int sx, int sy, int ex, int ey, int head);
	void   Draw2DZeroAxis(GpTermEntry * pT, AXIS_INDEX axIdx, AXIS_INDEX crossaxis);
	void   Draw3DGraphBox(GpTermEntry * pT, SurfacePoints * plot, int plot_num, WHICHGRID whichgrid, int current_layer);
	void   Draw3DPoint(GpTermEntry * pT, GpVertex & rV, lp_style_type * pLp);
	void   DrawColorSmoothBox(GpTermEntry * pT, int plot_mode);
	void   DrawInsideColorSmoothBoxBitmap(GpTermEntry * pT);
	void   DrawInsideColorSmoothBoxPostscript(GpTermEntry * pT);
	void   DrawVertex(GpTermEntry * pT, GpVertex & rV);
	void   Draw3DPointUnconditional(GpTermEntry * pT, const GpVertex * pV, lp_style_type * pLp);
	void   Draw3DLineUnconditional(GpTermEntry * pT, const GpVertex * pV1, const GpVertex * pV2, lp_style_type * pLp, t_colorspec cs);
	void   Draw3DLine(GpTermEntry * pT, GpVertex * pV1, GpVertex * pV2, lp_style_type * pLp);
	void   WriteLabel(GpTermEntry * pT, uint x, uint y, GpTextLabel * pLabel);
	void   FilledColorContourPlot(GpTermEntry * pT, SurfacePoints * pPlot, int contoursWhere);
	void   Pm3DPlot(GpTermEntry * pT, SurfacePoints * pPlot, int at_which_z);
	void   Pm3DDrawOne(GpTermEntry * pT, SurfacePoints * pPlot);
	void   DrawLineHidden(GpTermEntry * pT, GpVertex * v1, GpVertex * v2/* pointers to the end vertices */, lp_style_type * lp/* line and point style to draw in */);
	void   DrawEdge(GpTermEntry * pT, GpEdge * pE, GpVertex * pV1, GpVertex * pV2);

	void   SaveWritebackAllAxes();
	void   SaveZeroAxis(FILE * fp, AXIS_INDEX axis);
	bool   TwoEdge3DIntersect(GpCoordinate * p0, GpCoordinate * p1, double * lx, double * ly, double * lz);
	int    EdgeIntersect(GpCoordinate * pPoints, int i, double * pEx, double * pEy);
	void   ClipPolygon(gpiPoint * in, gpiPoint * out, int in_length, int * out_length);

	int    MapX(double value);
	int    MapY(double value);
	void   MapPositionR(const GpPosition & rPos, double * pX, double * pY, const char * pWhat);
	void   MapPosition(GpTermEntry * pT, GpPosition * pPos, int * pX, int * pY, const char * pWhat);
	double JDist(const GpCoordinate * pi, const GpCoordinate * pj);

	void   SplotMapActivate(GpCommand & rC);
	void   SplotMapDeactivate(GpCommand & rC);

	void   DoBezier(CurvePoints * pCp, double * pBc, int firstPoint, int numPoints, GpCoordinate * pDest);
	void   DoCubic(CurvePoints * pPlot, spline_coeff * pSc, int firstPoint, int numPoints, GpCoordinate * pDest);

	void   ApplyZoom(GpTermEntry * pT, GpZoom * pZ);
	void   DoZoom(GpTermEntry * pT, double xmin, double ymin, double x2min, double y2min, double xmax, double ymax, double x2max, double y2max);
	void   ZoomAroundMouse(GpTermEntry * pT, int zoom_key);

	void   SetTerminal(GpCommand & rC);
	void   SetContour(GpCommand & rC);
	int    SetTicProp(GpAxis & rAx, GpCommand & rC);
	void   SetRefreshOk(TRefresh_Allowed ok, int nplots)
	{
		RefreshOk = ok;
		RefreshNPlots = nplots;
	}
	void SetDGrid3D(GpCommand & rC);
	void SetMapping(GpCommand & rC);
	void SetView(GpCommand & rC);
	void SetPaletteFunction(GpCommand & rC);
	void SetPalette(GpCommand & rC);
	void SetTimestamp(GpCommand & rC);
	void SetLineStyle(GpCommand & rC, linestyle_def ** ppHead, lp_class destinationClass);
	void SetZeroAxis(GpCommand & rC, AXIS_INDEX axIdx);
	void SetXYZLabel(GpCommand & rC, GpTextLabel * pLabel);
	void SetSamples(GpCommand & rC);
	void SetLogScale(GpCommand & rC);
	void SetSurface(GpCommand & rC);
	void SetFormat(GpCommand & rC);
	void SetDashType(GpCommand & rC);
	void SetCntrLabel(GpCommand & rC);
	void SetPointIntervalBox(GpCommand & rC);
	void SetHidden3DOptions(GpCommand & rC);
	void SetIsoSamples(GpCommand & rC);
	void SetColorSequence(GpCommand & rC, int option);
	void SetParametric(GpCommand & rC);
	void SetBorder(GpCommand & rC);
	void SetTicsLevel(GpCommand & rC);
	void SetXYPlane(GpCommand & rC);
	void SetPolar(GpCommand & rC);
	void SetPointSize(GpCommand & rC);
	void SetBoxWidth(GpCommand & rC);
	void SetMonochrome(GpCommand & rC);
	void SetJitter(GpCommand & rC);
	void SetMouse(GpCommand & rC);
	void SetMouseRuler(bool on, int mx, int my);
	void SetArrow(GpCommand & rC);
	void SetAngles(GpCommand & rC);
	void SetFit(GpCommand & rC, GpFit & rF);
	void SetRange(GpCommand & rC, GpAxis * pAx);

	void UnsetFillStyle()
	{
		DefaultFillStyle.fillstyle = FS_EMPTY;
		DefaultFillStyle.filldensity = 100;
		DefaultFillStyle.fillpattern = 0;
		DefaultFillStyle.border_color.type = TC_DEFAULT;
	}
	//
	// process 'unset zero' command
	//
	//static void unset_zero()
	void UnsetZero()
	{
		Zero = ZERO;
	}
	//
	// process 'unset {x|y|z|x2|y2}data' command
	//
	//static void unset_timedata(AXIS_INDEX axis)
	void UnsetTimedata(AXIS_INDEX axis)
	{
		AxA[axis].datatype = DT_NORMAL;
		AxA[axis].tictype = DT_NORMAL;
	}
	//
	// process 'unset {x|y|z|x2|y2|t|u|v|r}range' command
	//
	void UnsetRange(AXIS_INDEX axis);
	//
	// process 'unset {x|y|x2|y2|z}zeroaxis' command 
	//
	//static void unset_zeroaxis(AXIS_INDEX axis)
	void UnsetZeroAxis(AXIS_INDEX axis)
	{
		if(AxA[axis].zeroaxis != &GpAxis::DefaultAxisZeroAxis)
			free(AxA[axis].zeroaxis);
		AxA[axis].zeroaxis = NULL;
	}
	//
	// process 'unset zeroaxis' command 
	//
	//static void unset_all_zeroaxes()
	void UnsetAllZeroAxes()
	{
		for(int ax_idx = FIRST_AXES; ax_idx < NUMBER_OF_MAIN_VISIBLE_AXES; ax_idx++)
			UnsetZeroAxis((AXIS_INDEX)ax_idx);
	}
	//
	// process 'unset {x|y|x2|y2|z}tics' command
	//
	//static void unset_all_tics()
	void UnsetAllTics()
	{
		for(int i = 0; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++)
			AxA[i].UnsetTics();
	}
	//static void unset_month_day_tics(AXIS_INDEX axis)
	void UnsetMonthDayTics(AXIS_INDEX axis)
	{
		AxA[axis].ticdef.type = TIC_COMPUTED;
	}
	void UnsetPolar(GpCommand & rC);
	void UnsetParametric(GpCommand & rC);
	void UnsetOrigin();
	void UnsetTimestamp();
	void UnsetGrid();
	void UnsetLogscale(GpCommand & rC);
	void UnsetOffsets();
	void UnsetClip(GpCommand & rC);
	void UnsetDGrid3D();
	void UnsetMapping();
	void UnsetBoxWidth();
	void UnsetStyle(GpCommand & rC);
	void UnsetDummy(GpCommand & rC);
	void UnsetSamples();
	void UnsetIsoSamples();
	void UnsetLabel(GpCommand & rC);
	void UnsetArrow(GpCommand & rC);
	void UnsetArrowStyles();
	void UnsetAutoScale(GpCommand & rC);
	void UnsetDashType(GpCommand & rC);
	void UnsetLineStyle(GpCommand & rC, linestyle_def ** ppHead);
	void UnsetLineType(GpCommand & rC);
	void UnsetMonochrome(GpCommand & rC);
	void UnsetHistogram();
	void UnsetFit();
	void UnsetTerminal();
	void UnsetJitter();
	void ResetKey();
	void ResetLogScale(GpAxis * pAx);

	void SetCommand(GpCommand & rC);
	void UnsetCommand(GpCommand & rC);
	void ResetCommand(GpCommand & rC);
	void ShowCommand(GpCommand & rC);
	void PlotCommand(GpCommand & rC);
	void ReplotCommand(GpTermEntry * pT, GpCommand & rC);
	void LoadCommand(GpCommand & rC);
	void TestCommand(GpCommand & rC);
	void TestPaletteSubcommand(GpCommand & rC);
	//
	// delete arrow from linked list started by GpGg.first_arrow.
	// called with pointers to the previous arrow (prev) and the arrow to delete (this).
	// If there is no previous arrow (the arrow to delete is GpGg.first_arrow) then call with prev = NULL.
	//
	void DeleteArrow(arrow_def * pPrev, arrow_def * pArr)
	{
		if(pArr) { // there really is something to delete
			if(pPrev) // there is a previous arrow 
				pPrev->next = pArr->next;
			else // pArr = GpGg.first_arrow so change GpGg.first_arrow
				first_arrow = pArr->next;
			free(pArr);
		}
	}
	void DestroyArrows()
	{
		while(first_arrow)
			DeleteArrow(0, first_arrow);
	}
	//
	// delete label from linked list started by GpGg.first_label.
	// called with pointers to the previous label (prev) and the label to delete (this).
	// If there is no previous label (the label to delete is GpGg.first_label) then call with prev = NULL.
	//
	void DeleteLabel(GpTextLabel * pPrev, GpTextLabel * pLab)
	{
		if(pLab) { // there really is something to delete 
			if(pPrev) // there is a previous label 
				pPrev->next = pLab->next;
			else            // pLab = GpGg.first_label so change GpGg.first_label 
				first_label = pLab->next;
			free(pLab->text);
			free(pLab->font);
			free(pLab);
		}
	}
	void DestroyLabeles()
	{
		while(first_label)
			DeleteLabel(0, first_label);
	}
	//
	// delete object from linked list started by GpGg.first_object.
	// called with pointers to the previous object (prev) and the
	// object to delete (this).
	// If there is no previous object (the object to delete is
	// GpGg.first_object) then call with prev = NULL.
	//
	//static void delete_object(t_object * prev, t_object * pThis)
	void DeleteObject(t_object * pPrev, t_object * pObj)
	{
		if(pObj) { // there really is something to delete 
			if(pPrev) // there is a previous rectangle 
				pPrev->next = pObj->next;
			else // pObj = GpGg.first_object so change GpGg.first_object */
				first_object = pObj->next;
			// NOTE:  Must free contents as well
			if(pObj->object_type == OBJ_POLYGON)
				free(pObj->o.polygon.vertex);
			free(pObj);
		}
	}
	void DestroyObjects()
	{
		while(first_object)
			DeleteObject(0, first_object);
	}
	#ifdef EAM_OBJECTS
	//static void unset_object(GpCommand & rC)
	void UnsetObject(GpCommand & rC)
	{
		if(rC.EndOfCommand()) {
			DestroyObjects();
		}
		else {
			const int tag = rC.IntExpression();
			if(!rC.EndOfCommand())
				IntErrorCurToken("extraneous arguments to unset rectangle");
			for(t_object * p_obj = first_object, * p_prev_obj = NULL; p_obj; p_prev_obj = p_obj, p_obj = p_obj->next) {
				if(p_obj->tag == tag) {
					DeleteObject(p_prev_obj, p_obj);
					return; // exit, our job is done
				}
			}
		}
	}
	#endif

	bool   FEqEps(double a, double b) const
	{
		return (fabs(a - b) < Zero);
	}
	bool   IsZero(double v) const
	{
		return (fabs(v) <= Zero);
	}
	//
	// Maps from normalized space to terminal coordinates 
	//
	void   TermCoord(const GpVertex & rV, uint & rX, uint & rY) const
	{
		rX = ((int)(rV.x * xscaler)) + xmiddle;
		rY = ((int)(rV.y * yscaler)) + ymiddle;
	}
	double Z2CB(double z);
	double CB2Gray(double cb);
	int    ClipPoint(uint x, uint y) const;
	int    ClipLine(int * x1, int * y1, int * x2, int * y2) const;
	void   MapPositionDouble(GpTermEntry * pT, GpPosition * pPos, double * pX, double * pY, const char * pWhat);
	void   GetPositionDefault(GpCommand & rC, GpPosition * pos, enum position_type default_type, int ndim);
	void   GetPosition(GpCommand & rC, GpPosition * pPos);
	void   CheckCornerHeight(GpCoordinate * p, double height[2][2], double depth[2][2]);
	//
	// parse a position of the form
	//  [coords] x, [coords] y {,[coords] z}
	// where coords is one of first,second.graph,screen,character
	// if first or second, we need to take axis.datatype into account
	// FIXME: Cannot handle parallel axes
	//
	double GetNumberOrTime(GpCommand & rC, AXIS_INDEX baseAxIdx, AXIS_INDEX axIdx);
	int    Get3DData(SurfacePoints * pPlot);
	void   GridNongridData(SurfacePoints * pPlot);
	double GetNumOrTime(GpCommand & rC, GpAxis * pAx);
	int    LpParse(GpCommand & rC, lp_style_type & rLp, lp_class destinationClass, bool allow_point);
	void   ParseLabelOptions(GpCommand & rC, GpTextLabel * pLabel, int nDim);
	void   ParsePlotTitle(GpCommand & rC, CurvePoints * pPlot, char * pXTitle, char * pYTitle, bool * pSetTitle);

	void   PlotRequest(/*GpCommand & rC*/);
	void   Plot3DRequest(GpCommand & rC);
	void   StatsRequest(GpCommand & rC);
	void   EvalPlots(/*GpCommand & rC*/);

	void   CheckPaletteGrayscale(GpCommand & rC);
	void   SetPaletteFile(GpCommand & rC);
	int    SetPaletteDefined(GpCommand & rC);
	void   SetStyle(GpCommand & rC);
	void   SetArrowStyle(GpCommand & rC);
	void   SetColorbox(GpCommand & rC);
	void   SetObj(GpCommand & rC, int tag, int obj_type);
	void   SetLabel(GpCommand & rC);
	void   SetKey(GpCommand & rC);
	void   SetOrigin(GpCommand & rC);
	void   SetOffsets(GpCommand & rC);
	void   SetPm3D(GpCommand & rC);
	void   SetClip(GpCommand & rC);
	void   SetBoxPlot(GpCommand & rC);
	void   SetObject(GpCommand & rC);
	void   SetSize(GpCommand & rC);
	void   SetGrid(GpCommand & rC);

	void   ShowPaletteGradient(GpCommand & rC);
	void   ShowPalette(GpCommand & rC);
	void   ShowMapping();
	void   ShowDGrid3D();
	void   ShowView();
	void   ShowZeroAxis(AXIS_INDEX axis);
	void   ShowSurface();
	void   ShowAutoscale();
	void   ShowBoxWidth();
	void   ShowBorder();
	void   ShowGrid();
	void   ShowStyleRectangle();
	void   ShowFormat();
	void   ShowStyle(GpCommand & rC);
	void   ShowFillStyle();
	void   ShowMargin();
	void   ShowTicDef(AXIS_INDEX axIdx);
	void   ShowTicDefP(GpAxis & rAx);
	void   ShowTics(bool showx, bool showy, bool showz, bool showx2, bool showy2, bool showcb);
	void   ShowColorBox(GpCommand & rC);
	void   ShowTimestamp();
	void   ShowMouse();
	void   ShowClip();
	void   ShowFit();
	void   ShowLabel(GpCommand & rC, int tag);
	void   ShowArrow(GpCommand & rC, int tag);
	void   ShowPm3D(GpCommand & rC);
	void   ShowTextBox();
	void   ShowSamples();
	void   ShowAngles();
	void   ShowIsoSamples();
	void   ShowContour();
	void   ShowJitter();
	void   ShowAll(GpCommand & rC);
	void   PrintFileAndLine(GpCommand & rC);

	void   SaveSetAll(GpCommand & rC, FILE * fp);
	void   SaveTics(FILE * fp, AXIS_INDEX axIdx);
	void   SavePTics(FILE * fp, GpAxis & rAx);
	void   SaveOffsets(FILE * fp, char * lead);
	void   SaveFunctions(GpCommand & rC, FILE * fp);
	void   SaveVariables(GpCommand & rC, FILE * fp);
	void   SaveSet(GpCommand & rC, FILE * fp);
	void   SaveBars(FILE * fp);
	void   SaveJitter(FILE * fp);
	void   SaveAll(GpCommand & rC, FILE * fp);
	void   SaveCommand(GpCommand & rC);
	void   ClearCommand(GpCommand & rC);
	void   LinkCommand(GpCommand & rC);
	void   PauseCommand(GpCommand & rC);
	void   ChangeDirCommand(GpCommand & rC);
	void   ToggleCommand(GpTermEntry * pT, GpCommand & rC);
	void   BindCommand(GpCommand & rC);
	void   EventReset(GpTermEntry * pT, GpEvent * pEv);

	void   EventButtonPress(GpTermEntry * pT, GpCommand & rC, GpEvent * ge);
	void   EventButtonRelease(GpTermEntry * pT, GpEvent * pGe);
	void   EventKeyPress(GpTermEntry * pT, GpCommand & rC, GpEvent * pGe, bool current);

	void   TestTerm(GpTermEntry * pT, GpCommand & rC);

	color_box_struct ColorBox;
	//color_box_struct DefColorBox;
	BoundingBox PlotBounds; // Plot Boundary
	BoundingBox Canvas;      // Writable area on terminal
	BoundingBox * P_Clip; // Current clipping box

	float  XSz;             /* x scale factor for size */
	float  YSz;             /* y scale factor for size */
	float  ZSz;             /* z scale factor for size */
	float  XOffs;           /* x origin setting */
	float  YOffs;           /* y origin setting */
	float  AspectRatio;      /* 1.0 for square */
	int    AspectRatio3D;     /* 2 for equal scaling of x and y; 3 for z also */
	// plot border autosizing overrides, in characters (-1: autosize)
	GpPosition LMrg;
	GpPosition BMrg;
	GpPosition RMrg;
	GpPosition TMrg; 

	custom_dashtype_def * first_custom_dashtype;
	arrow_def * first_arrow;
	GpTextLabel * first_label;
	linestyle_def * first_linestyle;
	linestyle_def * first_perm_linestyle;
	linestyle_def * first_mono_linestyle;
	arrowstyle_def * first_arrowstyle;
#ifdef EAM_OBJECTS
	t_object * first_object; // Pointer to first object instance in linked list
	// Default rectangle style - background fill, black border
	t_object DefaultRectangle;
	t_object DefaultCircle;
	t_object DefaultEllipse;
#endif
	histogram_style histogram_opts; // = DEFAULT_HISTOGRAM_STYLE;
	boxplot_style boxplot_opts; // = DEFAULT_BOXPLOT_STYLE;
#ifdef EAM_BOXED_TEXT
	textbox_style textbox_opts; // = DEFAULT_TEXTBOX_STYLE;
#endif
	GpTextLabel title;
	GpTextLabel timelabel;
	int    timelabel_rotate;
	int    timelabel_bottom;
	double Zero;             // zero threshold, not 0! 
	double PtSz;
	double PtIntervalBox;

	const  lp_style_type BackgroundLp;
	const  lp_style_type DefaultBorderLp;
	lp_style_type BorderLp;
	int    DrawBorder;
	int    UserBorder;
	int    BorderLayer;
	int    Samples1;
	int    Samples2;
	int    RefreshNPlots;
	//int    current_x11_windowid; // WINDOWID to be filled by terminals running on X11 (x11, wxt, qt, ...)
	int    CurrentX11WindowId; // WINDOWID to be filled by terminals running on X11 (x11, wxt, qt, ...)
	double Ang2Rad; // 1 or pi/180
	enum PLOT_STYLE DataStyle;
	enum PLOT_STYLE FuncStyle;
	TRefresh_Allowed RefreshOk;
	fill_style_type DefaultFillStyle;
	filledcurves_opts FilledcurvesOptsData; // filledcurves style options set by 'set style [data|func] filledcurves opts'
	filledcurves_opts FilledcurvesOptsFunc;
	t_sm_palette SmPalette; // initialized in plot.c on program entry
	legend_key keyT; // = DEFAULT_KEY_PROPS;

	enum {
		stIsPolar                   = 0x00000001,
		stClipLines1                = 0x00000002,
		stClipLines2                = 0x00000004,
		stClipPoints                = 0x00000008,
		stIsParametric              = 0x00000010,
		stInParametric              = 0x00000020,
		stIs3DPlot                  = 0x00000040, // If last plot was a 3d one
		stIsVolatileData            = 0x00000080,
		stIsMonochrome              = 0x00000100,
		stIsMultiPlot               = 0x00000200, // multiplot;
		stScreen_ok                 = 0x00000400, // true if command just typed; becomes false whenever we send some other output to screen.  
			// If false, the command line will be echoed to the screen before the ^ error message.
		stIsInteractive             = 0x00000800, // false if stdin not a terminal 
		stNoinputfiles              = 0x00001000, // false if there are script files 
		stPersist_cl                = 0x00002000, // true if -persist is parsed in the command line 
		stReading_from_dash         = 0x00004000, // True if processing "-" as an input file 
		stSkip_gnuplotrc            = 0x00008000, // skip system gnuplotrc and ~/.gnuplot 
		stCtrlc_flag                = 0x00010000, // Flag for asynchronous handling of Ctrl-C. Used by fit.c and Windows 
		stPrefer_line_styles        = 0x00020000, // Prefer line styles over plain line types
		stCallFromRexx              = 0x00040000, // OS2 only
		stBoxwidth_is_absolute      = 0x00080000, // whether box width is absolute (default) or relative
		stBoxPlotFactorSortRequired = 0x00100000, // used by compare_ypoints via q_sort from filter_boxplot
		stDgrid3d                   = 0x00200000,
		stDgrid3d_kdensity          = 0x00400000,
		stTerm_initialised          = 0x00800000, // mouse module needs this 
		stTerm_graphics             = 0x01000000, // true if terminal is in graphics mode
		stTerm_suspended            = 0x02000000, // we have suspended the driver, in multiplot mode 
		stOpened_binary             = 0x04000000, // true if? 
		stTerm_force_init           = 0x08000000, // true if require terminal to be initialized 
	};
	long   State_;

	bool   IsPolar;
	bool   ClipLines1;
	bool   ClipLines2;
	bool   ClipPoints;
	bool   IsParametric;
	bool   InParametric;
	bool   Is3DPlot; // If last plot was a 3d one
	bool   IsVolatileData;
	bool   IsMonochrome;
	bool   IsMultiPlot; // multiplot;
	bool   screen_ok; // true if command just typed; becomes false whenever we send some other output to screen.  
		// If false, the command line will be echoed to the screen before the ^ error message.
	//
	bool   IsInteractive;      // false if stdin not a terminal 
	bool   noinputfiles;       // false if there are script files 
	bool   persist_cl;         // true if -persist is parsed in the command line 
	bool   reading_from_dash;  // True if processing "-" as an input file 
	bool   skip_gnuplotrc;     // skip system gnuplotrc and ~/.gnuplot 
	bool   ctrlc_flag;         // Flag for asynchronous handling of Ctrl-C. Used by fit.c and Windows 
	bool   prefer_line_styles; // Prefer line styles over plain line types
	bool   CallFromRexx;       // OS2 only
	bool   boxwidth_is_absolute; // whether box width is absolute (default) or relative
	bool   BoxPlotFactorSortRequired; // used by compare_ypoints via q_sort from filter_boxplot
	bool   dgrid3d;
	bool   dgrid3d_kdensity;
	bool   term_initialised; // mouse module needs this 
	bool   term_graphics;   // true if terminal is in graphics mode
	bool   term_suspended;  // we have suspended the driver, in multiplot mode 
	bool   opened_binary;   // true if? 
	bool   term_force_init; // true if require terminal to be initialized 

	const  char * user_shell; // user shell 
	//
	//
	//
	CurvePoints * P_FirstPlot;
	UdftEntry plot_func;
	double boxwidth; // box width (automatic)
	double histogram_rightmost;    // Highest x-coord of histogram so far 
	GpTextLabel histogram_title;          // Subtitle for this histogram 
	int    StackCount;                 // counter for stackheight 
	GpCoordinate * StackHeight; // Scratch space for y autoscale 
	//
	// key placement is calculated in boundary, so we need file-wide variables
	// To simplify adjustments to the key, we set all these once [depends on
	// key->reverse] and use them throughout.
	//
	double LargestPolarCircle; // set by tic_callback - how large to draw polar radii
	// Status information for stacked histogram plots
	GpCoordinate * P_PrevRowStackHeight;    // top of previous row
	int    PrevRowStackCount; // points actually used
	//
	t_data_mapping mapping3d;
	int    dgrid3d_row_fineness;
	int    dgrid3d_col_fineness;
	int    dgrid3d_norm_value;
	int    dgrid3d_mode;
	double dgrid3d_x_scale;
	double dgrid3d_y_scale;
	//
	// the curves/surfaces of the plot
	SurfacePoints * P_First3DPlot;
	UdftEntry plot3D_func;
	int    plot3d_num;
	GpCommand Gp__C;
	MpLayout MpL; // = MP_LAYOUT_DEFAULT;
	GpMouse  Mse;
	GpEval   Ev;
	//
	// Used by routine filled_quadrangle() in color.c 
	lp_style_type Pm3DBorderLp; // pm3d_border_lp;	/* FIXME: Needed anymore? */
	pm3d_struct Pm3D; // pm3d;
	lighting_model Pm3DShade; // pm3d_shade;
	//
	double term_pointsize;  // internal pointsize for do_point 
private:
	void   CheckTransform(GpCommand & rC, int & rTransformDefined);
	bool   GridMatch(GpCommand & rC, int axIdx, const char * pString);
	void   Plot3dVectors(GpTermEntry * pT, SurfacePoints * pPlot);
	void   KeySamplePoint(GpTermEntry * pT, int xl, int yl, int pointtype);
	void   KeySampleLine(GpTermEntry * pT, int xl, int yl);
	void   Do3DkeyLayout(GpTermEntry * pT, legend_key * pKey, int * pXinkey, int * pYinkey);

	GpVertex PolyLine3DPreviousVertex; // Previous points 3D position 
	//
	void   JitterPoints(CurvePoints * pPlot);
	//
	// JITTER
	//
	enum jitterstyle {
		JITTER_DEFAULT = 0,
		JITTER_SWARM,
		JITTER_SQUARE
	};
	struct t_jitter {
		t_jitter()
		{
			overlap.Set(first_axes, first_axes, first_axes, 0.0, 0.0, 0.0);
			spread = 0.0;
			limit = 0.0;
			style = JITTER_DEFAULT;
		}
		GpPosition overlap;
		double spread;
		double limit;
		enum jitterstyle style;
	};

	t_jitter jitter;
};

/* store VALUE or log(VALUE) in STORE, set TYPE as appropriate
 * Do OUT_ACTION or UNDEF_ACTION as appropriate
 * adjust range provided type is INRANGE (ie dont adjust y if x is outrange
 * VALUE must not be same as STORE
 * NOAUTOSCALE is per-plot property, whereas AUTOSCALE_XXX is per-axis.
 * Note: see the particular implementation for COLOR GpAxis below.
 */
#define ACTUAL_STORE_WITH_LOG_AND_UPDATE_RANGE(STORE, VALUE, TYPE, ax, NOAUTOSCALE, OUT_ACTION, UNDEF_ACTION, is_cb_axis)  \
do {									  \
    GpAxis *axis = ax; \
    double curval = (VALUE);						  \
    /* Version 5: OK to store infinities or NaN */			  \
    STORE = curval;							  \
    if(!(curval > -GPVL && curval < GPVL)) {		  \
		TYPE = UNDEFINED;						  \
		UNDEF_ACTION;							  \
		break;								  \
    }									  \
    if(axis->Flags & GpAxis::fLog) {							  \
		if(curval < 0.0) {						  \
			STORE = not_a_number();					  \
			TYPE = UNDEFINED;						  \
			UNDEF_ACTION;						  \
			break;							  \
		} \
		else if(curval == 0.0) {					  \
			STORE = -GPVL;						  \
			TYPE = OUTRANGE;						  \
			OUT_ACTION;							  \
			break;							  \
		} \
		else {							  \
			STORE = log(curval) / axis->log_base; /* AXIS_DO_LOG() */	  \
		}								  \
    }									  \
    if(NOAUTOSCALE)							  \
		break;  /* this plot is not being used for autoscaling */	  \
    if(TYPE != INRANGE)						  \
		break;  /* don't set y range if x is outrange, for example */	  \
    if((!is_cb_axis) && axis->P_LinkToPrmr) {	  		  \
		axis = axis->P_LinkToPrmr; \
		if(axis->link_udf->at) \
			curval = axis->EvalLinkFunction(curval);			  \
	} 									  \
	SETMIN(axis->DataRange.low, curval); \
	if((curval < axis->Range.low) && ((curval <= axis->Range.upp) || (axis->Range.upp == -GPVL))) { \
		if(axis->AutoScale & AUTOSCALE_MIN)	{ \
			if(axis->min_constraint & CONSTRAINT_LOWER) {		  \
				if(axis->Lb.low <= curval) {				  \
					axis->Range.low = curval;					  \
				} else {						  \
					axis->Range.low = axis->Lb.low;				  \
					TYPE = OUTRANGE;					  \
					OUT_ACTION;						  \
					break;						  \
				}							  \
			} else {							  \
				axis->Range.low = curval;					  \
			}								  \
		} else if(curval != axis->Range.upp) {				  \
			TYPE = OUTRANGE;						  \
			OUT_ACTION;							  \
			break;							  \
		}								  \
	}									  \
	SETMAX(axis->DataRange.upp, curval); \
	if(curval > axis->Range.upp && (curval >= axis->Range.low || axis->Range.low == GPVL)) { \
		if(axis->AutoScale & AUTOSCALE_MAX)	{			  \
			if(axis->max_constraint & CONSTRAINT_UPPER) {		  \
				if(axis->Ub.upp >= curval) {		 		  \
					axis->Range.upp = curval;					  \
				} else {						  \
					axis->Range.upp = axis->Ub.upp;				  \
					TYPE = OUTRANGE;					  \
					OUT_ACTION;						  \
					break;						  \
				}							  \
			} else {							  \
				axis->Range.upp = curval;					  \
			}								  \
		} else if(curval != axis->Range.low) {				  \
			TYPE = OUTRANGE;						  \
			OUT_ACTION;							  \
		}								  \
	}									  \
} while(0)

/* normal calls go though this macro, marked as not being a color axis */
#define STORE_WITH_LOG_AND_UPDATE_RANGE(STORE, VALUE, TYPE, ax, NOAUTOSCALE, OUT_ACTION, UNDEF_ACTION)	 \
	if(ax != NO_AXIS) ACTUAL_STORE_WITH_LOG_AND_UPDATE_RANGE(STORE, VALUE, TYPE, (&GpGg[ax]), NOAUTOSCALE, OUT_ACTION, UNDEF_ACTION, 0)

/* Implementation of the above for the color axis. It should not change
 * the type of the point (out-of-range color is plotted with the color
 * of the min or max color value).
 */
#define COLOR_STORE_WITH_LOG_AND_UPDATE_RANGE(STORE, VALUE, TYPE, ax, NOAUTOSCALE, OUT_ACTION, UNDEF_ACTION) \
{									  \
    coord_type c_type_tmp = TYPE;					  \
    ACTUAL_STORE_WITH_LOG_AND_UPDATE_RANGE(STORE, VALUE, c_type_tmp, &GpGg[ax], NOAUTOSCALE, OUT_ACTION, UNDEF_ACTION, 1); \
}

/* #define NOOP (0) caused many warnings from gcc 3.2 */
#define NOOP ((void)0)

/* HBB 20000506: new macro to automatically build initializer lists
 * for arrays of AXIS_ARRAY_SIZE=11 equal elements */
#define AXIS_ARRAY_INITIALIZER(value) { value, value, value, value, value, value, value, value, value, value, value }

/* 'roundoff' check tolerance: less than one hundredth of a tic mark */
#define SIGNIF (0.01)
/* (DFK) Watch for cancellation error near zero on axes labels */
/* FIXME HBB 20000521: these seem not to be used much, anywhere... */
#define CheckZero(x,tic) (fabs(x) < ((tic) * SIGNIF) ? 0.0 : (x))
//
// COUNTUR.H
//
#define DEFAULT_CONTOUR_LEVELS 5
#define DEFAULT_NUM_APPROX_PTS 5
#define DEFAULT_CONTOUR_ORDER  4
#define MAX_BSPLINE_ORDER      10
//
// Method of drawing the contour lines found 
//
enum t_contour_kind {
    CONTOUR_KIND_LINEAR,
    CONTOUR_KIND_CUBIC_SPL,
    CONTOUR_KIND_BSPLINE
};
//
// How contour levels are set 
//
enum t_contour_levels_kind {
    LEVELS_AUTO,        // automatically selected
    LEVELS_INCREMENTAL, // user specified start & incremnet
    LEVELS_DISCRETE     // user specified discrete levels
};

// Used to allocate the tri-diag matrix.
typedef double tri_diag[3];
//
// Variables of contour.c needed by other modules:
//
extern char contour_format[32];
extern t_contour_kind contour_kind;
extern t_contour_levels_kind contour_levels_kind;
extern int contour_levels;
extern int contour_order;
extern int contour_pts;
extern dynarray dyn_contour_levels_list; // storage for z levels to draw contours at 

#define contour_levels_list ((double *)dyn_contour_levels_list.v)
//
// BITMAP.H
//
// allow up to 16 bit width for character array 
typedef uint char_row;
typedef char_row const GPFAR * GPFAR char_box;

#define FNT_CHARS   96		/* Number of characters in the font set */

#define FNT5X9 0
#define FNT5X9_VCHAR 11		/* vertical spacing between characters */
#define FNT5X9_VBITS 9		/* actual number of rows of bits per char */
#define FNT5X9_HCHAR 7		/* horizontal spacing between characters */
#define FNT5X9_HBITS 5		/* actual number of bits per row per char */

#define FNT9X17 1
#define FNT9X17_VCHAR 21	/* vertical spacing between characters */
#define FNT9X17_VBITS 17	/* actual number of rows of bits per char */
#define FNT9X17_HCHAR 13	/* horizontal spacing between characters */
#define FNT9X17_HBITS 9		/* actual number of bits per row per char */

#define FNT13X25 2
#define FNT13X25_VCHAR 31	/* vertical spacing between characters */
#define FNT13X25_VBITS 25	/* actual number of rows of bits per char */
#define FNT13X25_HCHAR 19	/* horizontal spacing between characters */
#define FNT13X25_HBITS 13	/* actual number of bits per row per char */

extern const char_row GPFAR fnt5x9[FNT_CHARS][FNT5X9_VBITS];
extern const char_row GPFAR fnt9x17[FNT_CHARS][FNT9X17_VBITS];
extern const char_row GPFAR fnt13x25[FNT_CHARS][FNT13X25_VBITS];

typedef uchar pixels;  // the type of one set of 8 pixels in bitmap 
typedef pixels *bitmap[];	// the bitmap 

class GpBitmapBlock {
public:
	GpBitmapBlock()
	{
		b_p = 0;
		b_xsize = 0;
		b_ysize = 0;
		b_planes = 0;
		b_psize = 0;
		b_rastermode = 0;
		b_linemask = 0;
		b_angle = 0;
		b_maskcount = 0;
		//
		b_value = 1;
		b_currx = 0;
		b_curry = 0;
		b_hchar = 0;
		b_hbits = 0;
		b_vchar = 0;
		b_vbits = 0;
		memzero(b_font, sizeof(b_font));
		b_lastx = 0;
		b_lasty = 0;
	}
	void   Destroy();
	void   Create(uint x, uint y, uint planes);
	void   Move(uint x, uint y);
	void   Vector(uint x, uint y);
	void   PutText(uint x, uint y, const char * str);
	int    TextAngle(int ang);
	void   CharSize(uint size);
	void   PutC(uint x, uint y, int c, uint c_angle);
	void   SetMaskPixel(uint x, uint y, uint value);
	void   Line(uint x1, uint y1, uint x2, uint y2);
	void   SetLneType(int linetype);
	void   SetPixel(uint x, uint y, uint value);
	uint   GetPixel(uint x, uint y);
	void   SetValue(uint value);
	void   BoxFill(int style, uint x, uint y, uint w, uint h);

	bitmap * b_p; // global pointer to bitmap 
	uint   b_xsize; // the size of the bitmap 
	uint   b_ysize; // the size of the bitmap 
	uint   b_planes;	// number of color planes 
	uint   b_psize;	// size of each plane 
	uint   b_rastermode; // raster mode rotates -90deg 
	uint   b_linemask;	// 16 bit mask for dotted lines 
	uint   b_angle;	// rotation of text 
	int    b_maskcount;
	//
	uint   b_value;    // colour of lines 
	uint   b_currx;
	uint   b_curry;    // the current coordinates 
	uint   b_hchar;    // width of characters 
	uint   b_hbits;    // actual bits in char horizontally 
	uint   b_vchar;    // height of characters 
	uint   b_vbits;    // actual bits in char vertically 
	char_box b_font[FNT_CHARS]; // the current font 
	uint   b_lastx;
	uint   b_lasty;    // last pixel set - used by b_line 
};

extern GpBitmapBlock GpBmB; // @global
//
// GETCOLOR.H
//
enum color_models_id {
    C_MODEL_RGB = 'r',
    C_MODEL_HSV = 'h',
    C_MODEL_CMY = 'c',
    C_MODEL_YIQ = 'y',
    C_MODEL_XYZ = 'x'
};
//
// DATAFILE.H
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
    DF_COLUMN_HEADERS = -9
};

// large file support (offsets potentially > 2GB) 
#if defined(HAVE_FSEEKO) && defined(HAVE_OFF_T)
	#define fseek(stream,pos,whence) fseeko(stream,pos,whence)
	#define ftell(stream) ftello(stream)
#elif defined(_MSC_VER)
	// @sobolev #define fseek(stream,pos,whence) _fseeki64(stream,pos,whence)
	// @sobolev #define ftell(stream) _ftelli64(stream)
#elif defined(__MINGW32__)
	#define fseek(stream,pos,whence) fseeko64(stream,pos,whence)
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
//
#define MAXDATACOLS (MAX_NUM_VAR+2)
#define NO_COLUMN_HEADER (-99)  // some value that can never be a real column 

struct UseSpecS {
	UseSpecS()
	{
		column = 0;
		expected_type = 0;
		at = 0;
	}
    int    column;
    int    expected_type;
    AtType * at;
};
//
// Details about the records contained in a binary data file
//
enum df_translation_type {
    DF_TRANSLATE_DEFAULT,     /* Gnuplot will position in first quadrant at origin. */
    DF_TRANSLATE_VIA_ORIGIN,
    DF_TRANSLATE_VIA_CENTER
};

enum df_sample_scan_type {
    DF_SCAN_POINT = -3,  /* fastest */
    DF_SCAN_LINE  = -4,
    DF_SCAN_PLANE = -5   /* slowest */
};
//
// To generate a swap, take the bit-wise complement of the lowest two bits
//
enum df_endianess_type {
    DF_LITTLE_ENDIAN,
    DF_PDP_ENDIAN,
    DF_DPD_ENDIAN,
    DF_BIG_ENDIAN,
    DF_ENDIAN_TYPE_LENGTH  /* Must be last */
};

const long long_0x2468 = 0x2468;
#define TEST_BIG_PDP         ( (((char*)&long_0x2468)[0] < 3) ? DF_BIG_ENDIAN : DF_PDP_ENDIAN )
#define THIS_COMPILER_ENDIAN ( (((char*)&long_0x2468)[0] < 5) ? TEST_BIG_PDP : DF_LITTLE_ENDIAN )
#define DF_BIN_FILE_ENDIANESS_RESET THIS_COMPILER_ENDIAN
//
// The various types of numerical types that can be read from a data file
//
enum df_data_type {
	DF_UNDEF = -1, // @sobolev
    DF_CHAR = 0, 
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
//
// Some macros for making the compiler figure out what function
// the "machine independent" names should execute to read the
// appropriately sized variable from a data file.
// 
#define SIGNED_TEST(val) ((val)==sizeof(long) ? DF_LONG : ((val)==sizeof(int64) ? DF_LONGLONG : \
			 ((val)==sizeof(int) ? DF_INT : ((val)==sizeof(short) ? DF_SHORT : ((val)==sizeof(char) ? DF_CHAR : DF_BAD_TYPE)))))
#define UNSIGNED_TEST(val) ((val)==sizeof(ulong) ? DF_ULONG : \
			   ((val)==sizeof(uint64) ? DF_ULONGLONG : ((val)==sizeof(uint) ? DF_UINT : \
			   ((val)==sizeof(ushort) ? DF_USHORT : ((val)==sizeof(uchar) ? DF_UCHAR : DF_BAD_TYPE)))))
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
 * "cart" means Cartesian, i.e., the (x,y,z) [or (r,t,z)] GpCoordinate
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
    int cart_dim[3];                  // dimension array size, x/y/z 
    int cart_dir[3];                  // 1 scan in positive direction, -1 negative, x/y/z 
    double cart_delta[3];             // spacing between array points, x/y/z 
    df_translation_type cart_trans;   // translate via origin, center or default 
    double cart_cen_or_ori[3];        // vector representing center or origin, x/y/z 
    double cart_alpha;                // 2D rotation angle (rotate) 
    double cart_p[3];                 // 3D rotation normal vector (perpendicular) 

    df_sample_scan_type cart_scan[3]; // how to assign the dimensions read from file when generating coordinates 
    bool scan_generate_coord;     // whether or not Gnuplot should generate coordinates. 
    off_t scan_skip[3];               // skip bytes before the record, line, plane 
	//
    // Not controllable by the user, only by file type functions.
    // These are all points/lines/planes format.
    //
    int scan_dim[3];                  // number of points, lines, planes 
    int scan_dir[3];                  // 1 scan in positive direction wrt Cartesian GpCoordinate system, -1 negative 
    double scan_delta[3];             // sample period along points, lines, planes 
    df_translation_type scan_trans;   // translate via origin, center or default 
    double scan_cen_or_ori[3];        // vector representing center or origin, x/y/z 
    // *** Do not modify outside of datafile.c!!! *** 
    char GPFAR *memory_data;
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
//
// rather than three arrays which all grow dynamically, make one dynamic array of this structure
//
struct df_column_struct {
	double datum;
	DF_STATUS good;
	char * position; /* points to start of this field in current line */
	char * header;  /* points to copy of the header for this column */
};

class GpDatafile {
public:
	static void AvsFiletypeFunction();
	static void F_StringColumn(GpArgument * pArg);

	GpDatafile() : blank_data_line(UNDEFINED, -999.0, -999.0, -999.0, -999.0, -999.0, -999.0, -999.0)
	{
		MEMSZERO(df_tokens);
		MEMSZERO(df_matrix_corner);
		df_commentschars = 0;
		missing_val = 0;
		df_separators = 0;
		df_bin_record = 0;
		df_num_bin_records = 0;
		df_bin_record_count = 0;
		df_max_num_bin_records = 0;
		df_max_num_bin_records_default = 0;
		df_num_bin_records_default = 0;
		df_bin_filetype = 0;
		df_bin_filetype_default = 0;
		df_bin_file_endianess_default = DF_BIN_FILE_ENDIANESS_RESET;
		df_bin_file_endianess = DF_BIN_FILE_ENDIANESS_RESET;

		df_no_use_specs = 0;
		df_datum = 0;
		df_line_number = 0;
		df_last_col = 0;
		df_no_bin_cols = 0;
		df_pixeldata = 0;

		df_matrix = false;
		df_nofpe_trap = false;
		df_fortran_constants = false;
		plotted_data_from_stdin = false;
		evaluate_inside_using = false;
		df_warn_on_missing_columnheader = false;
		//
		//
		//
		df_line = NULL;
		max_line_len = 0;
		data_fp = NULL;
#if defined(PIPES)
		df_pipe_open = false;
#endif
#if defined(HAVE_FDOPEN)
		data_fd = -2;        // only used for file redirection 
#endif
		mixed_data_fp = false; // inline data 
		df_filename = NULL;      // name of data file 
		df_eof = 0;
		df_no_tic_specs = 0;
		blank_count = 0;     // how many blank lines recently 
		df_lower_index = 0;  // first mesh required 
		df_upper_index = MAXINT;
		df_index_step = 1;   // 'every' for indices 
		df_current_index = 0;    // current mesh 
		indexname = NULL;
		index_found = false;
		df_longest_columnhead = 0;
		everypoint = 1;
		firstpoint = 0;
		lastpoint = MAXINT;
		everyline = 1;
		firstline = 0;
		lastline = MAXINT;
		point_count = -1;     // point counter - preincrement and test 0
		line_count = 0;       // line counter
		df_skip_at_front = 0; // for ascii file "skip" lines at head of file 
		df_pseudodata = 0;
		df_pseudorecord = 0;
		df_pseudospan = 0;
		df_pseudovalue_0 = 0;
		df_pseudovalue_1 = 0;
		df_datablock = false;
		df_datablock_line = NULL;
		df_array_index = 0;
		df_xpixels = 0;
		df_ypixels = 0;
		df_transpose = false;
		df_format = NULL;
		df_binary_format = NULL;

		df_column = NULL;
		df_max_cols = 0;
		df_no_cols = 0;
		fast_columns = 0;
		//
		MEMSZERO(df_stringexpression);
		df_current_plot = 0;    // used to process histogram labels + key entries 
		column_for_key_title = NO_COLUMN_HEADER;
		df_already_got_headers = false;
		df_key_title = NULL;     // filled in from column header if requested 
		df_read_binary = false;
		df_nonuniform_matrix = false;
		df_matrix_columnheaders = false;
		df_matrix_rowheaders = false;
		df_plot_mode = 0;
		//
		df_column_bininfo = NULL;      /* allocate space as needed */
		df_max_bininfo_cols = 0;     /* space allocated */
		matrix_general_binary_conflict_msg = "Conflict between some matrix binary and general binary keywords";
	}
	void   InitializeUseSpec();
	void   InitializeBinaryVars();
	void   DfInit();
	int    DfOpen(GpCommand & rC, const char * cmd_filename, int max_using, CurvePoints * plot);
	void   DfClose();
	int    DfReadLine(double v[], int max);
	int    DfReadAscii(GpCommand & rC, double v[], int max);
	int    DfReadBinary(double v[], int max);
	float * DfReadMatrix(int * rows, int * cols);
	char * DfParseStringField(char * field);
	void   DfDetermineMatrixInfo(FILE * fin);
	void   DfSetDatafileBinary(GpCommand & rC);
	void   DfUnsetDatafileBinary();
	void   DfShowData();
	void   DfShowBinary(FILE * fp);
	int    DfTokenise(char * s);
	void   PlotOptionUsing(GpCommand & rC, int max_using);
	void   PlotOptionArray(GpCommand & rC);
	void   PlotOptionMultiValued(GpCommand & rC, df_multivalue_type type, int arg);
	void   PlotOptionBinary(GpCommand & rC, bool set_matrix, bool set_default);
	void   PlotOptionBinaryFormat(char * format_string);
	void   PlotOptionIndex(GpCommand & rC);
	void   PlotOptionEvery(GpCommand & rC);
	void   PlotTicLabelUsing(GpCommand & rC, int axis);
	void   AdjustBinaryUseSpec(CurvePoints * plot);
	void   DfAddBinaryRecords(int num_records_to_add, df_records_type records_type);
	void   DfInsertScannedUseSpec(int uspec);
	void   ClearBinaryRecords(df_records_type records_type);
	void   ClearDfColumnHeaders();
	void   DfSetKeyTitleColumnHead(GpCommand & rC, CurvePoints * plot);
	void   DfSetKeyTitle(CurvePoints * plot);
	void   DfSetPlotMode(int mode);
	int    ExpectString(const char column);
	int    CheckMissing(char * s);
	char * DfGets(GpCommand & rC);
	char * DfFGets(GpCommand & rC, FILE * fin);
	int    DfSkipBytes(off_t nbytes);
	char * DfGenerateAsciiArrayEntry();
	char * DfGeneratePseudoData();
	void   ExpandDfColumn(int new_max);
	void   AddKeyEntry(char * temp_string, int df_datum);
	void   DfSetSkipBefore(int col, int bytes);
	void   DfSetReadType(int col, df_data_type type);
	df_data_type DfGetReadType(int col);
	int    DfGetReadSize(int col);
	void   DfExtendBinaryColumns(int no_cols);
	void   SetSeparator(GpCommand & rC);
    
	const GpCoordinate blank_data_line;

	int    df_no_use_specs; // how many using columns were specified in the current command 
	int    df_datum; // suggested x value if none given 
	char * df_filename;
	int    df_line_number;
	AXIS_INDEX df_axis[MAXDATACOLS];
#ifdef BACKWARDS_COMPATIBLE
	UdftEntry ydata_func; // deprecated "thru" function 
#endif
	char * df_tokens[MAXDATACOLS]; // Returned to caller by df_readline() 
	int    df_last_col; // number of columns in first row of data return to user in STATS_columns 
	char * missing_val; // string representing missing values, ascii datafiles 
	char * df_separators; // input field separators, NULL if whitespace is the separator 
	char * df_commentschars; // comments chars 
	int    df_no_bin_cols; // cols to read 
	int    df_num_bin_records;
	int    df_max_num_bin_records;
	int    df_bin_record_count;
	int    df_max_num_bin_records_default;
	int    df_num_bin_records_default;
	int    df_bin_filetype; // Initially set to default and then possibly altered by command line
	int    df_bin_filetype_default;
	void * df_pixeldata;

	df_binary_file_record_struct * df_bin_record;
	df_endianess_type df_bin_file_endianess_default;
	df_endianess_type df_bin_file_endianess;
	double df_matrix_corner[2][2]; // First argument is corner, second argument is x (0) or y(1)

	UseSpecS use_spec[MAXDATACOLS];
	//
	// Bookkeeping for df_fgets() and df_gets(). Must be initialized before any calls to either function.
	//
	char * df_line;
	size_t max_line_len;
	FILE * data_fp;
#if defined(HAVE_FDOPEN)
	int    data_fd;        // only used for file redirection 
#endif
	int    df_eof;
	int    df_no_tic_specs;     // ticlabel columns not counted in df_no_use_specs 
	// stuff for implementing index 
	int    blank_count;     // how many blank lines recently 
	int    df_lower_index;  // first mesh required 
	int    df_upper_index;
	int    df_index_step;   // 'every' for indices 
	int    df_current_index;    // current mesh 
	// stuff for named index support 
	char * indexname;
	int    df_longest_columnhead;
	// stuff for every point:line 
	int    everypoint;
	int    firstpoint;
	int    lastpoint;
	int    everyline;
	int    firstline;
	int    lastline;
	int    point_count;     // point counter - preincrement and test 0
	int    line_count;       // line counter
	int    df_skip_at_front; // for ascii file "skip" lines at head of file 
	//
	// for pseudo-data (1 if filename = '+'; 2 if filename = '++') 
	//
	int    df_pseudodata;
	int    df_pseudorecord;
	int    df_pseudospan;
	double df_pseudovalue_0;
	double df_pseudovalue_1;
	// for datablocks 
	char ** df_datablock_line;
	// for arrays
	int    df_array_index;
	// track dimensions of input matrix/array/image 
	uint   df_xpixels;
	uint   df_ypixels;
	// parsing stuff 
	char * df_format;
	char * df_binary_format;
	//
	df_column_struct * df_column;      // we'll allocate space as needed 
	int    df_max_cols;     // space allocated 
	int    df_no_cols;          // cols read 
	int    fast_columns;        // corey@cac optimization 
	//
	char * df_stringexpression[MAXDATACOLS]; // filled in after evaluate_at()
	CurvePoints * df_current_plot; // used to process histogram labels + key entries
	int    column_for_key_title;
	char * df_key_title; // filled in from column header if requested
	//
	// Binary *read* variables used by df_readbinary().
	// There is a confusing difference between the ascii and binary "matrix" keywords.
	// Ascii matrix data by default is interpreted as having an implicit uniform grid
	// of x and y coords that are not actually present in the data file.
	// The equivalent binary data format is called "binary general".
	// In both of these cases the internal flag df_nonuniform_matrix is false;
	// Binary matrix data contains explicit y values in the first row, and explicit x
	// values in the first column. This is signalled by "binary matrix".
	// In this case the internal flag df_nonuniform_matrix is true.
	//
	// EAM May 2011 - Add a keyword "nonuniform matrix" to indicate ascii matrix data
	// in the same format as "binary matrix", i.e. with explicit x and y coordinates.
	// EAM Jul 2014 - Add keywords "columnheaders" and "rowheaders" to indicate ascii
	// matrix data in the uniform grid format containing labels in row 1 and column 1.
	//
	bool   df_matrix; // is this a matrix splot? 
	bool   df_binary; // is this a binary file? 
	bool   df_matrix_file;
	bool   df_binary_file;
	bool   plotted_data_from_stdin; // flag if any 'inline' data are in use, for the current plot 
	// Setting this allows the parser to recognize Fortran D or Q   
	// format constants in the input file. But it slows things down 
	bool   df_fortran_constants;
	// Setting this disables initialization of the floating point exception 
	// handler before every expression evaluation in a using specifier.   	 
	// This can speed data input significantly, but assumes valid input.    
	bool   df_nofpe_trap;
	bool   evaluate_inside_using;
	bool   df_warn_on_missing_columnheader;
	bool   mixed_data_fp; // inline data 
	bool   index_found;
	bool   df_datablock;
	bool   df_transpose;
	bool   df_already_got_headers;
	bool   df_read_binary;
	bool   df_nonuniform_matrix;
	bool   df_matrix_columnheaders;
	bool   df_matrix_rowheaders;
#if defined(PIPES)
	bool   df_pipe_open;
#endif
	int    df_plot_mode;
	//
	// Information about binary data structure, to be determined by the
	// using and format options.  This should be one greater than df_no_bin_cols.
	//
	df_column_bininfo_struct * df_column_bininfo;      /* allocate space as needed */
	int    df_max_bininfo_cols;     /* space allocated */
	const  char * matrix_general_binary_conflict_msg;
};
//
// State information for load_file(), to recover from errors
// and properly handle recursive load_file calls
//
struct LFS {
    // new recursion level: 
    FILE * fp;            // file pointer for load file 
    char * name;          // name of file 
    char * cmdline;       // content of command string for do_string() 
    // last recursion level: 
    int    inline_num;    // inline_num on entry 
    int    depth;         // recursion depth 
    int    if_depth;      // used by _old_ if/else syntax 
    char * input_line;    // Input line text to restore 
    LexicalUnit * P_Ttokens; // Input line tokens to restore 
    int    NumTokens;     // How big is the above ? 
    int    CToken;        // Which one were we on ? 
    LFS  * prev;          // defines a stack
    int    call_argc;     // This saves the _caller's_ argc 
    char * call_args[10]; // args when file is 'call'ed instead of 'load'ed 
    bool   interactive; // value of interactive flag on entry 
    bool   if_open_for_else; // used by _new_ if/else syntax 
    bool   if_condition;	   // used by both old and new if/else syntax 
};
//
// HIDDEN3D.H
//
#define PT_ARROWHEAD -10
#define PT_BACKARROW -11
//
// HELP.H
//
//
// Exit status returned by help() 
//
#define	H_FOUND		0	/* found the keyword */
#define	H_NOTFOUND	1	/* didn't find the keyword */
#define	H_ERROR		(-1)	/* didn't find the help file */

int  help(char *keyword, char *path, bool *subtopics);
void FreeHelp();
void StartOutput();
void OutLine(const char *line);
void EndOutput();
//
// EXTERNAL.H
//
#ifdef HAVE_EXTERNAL_FUNCTIONS
	void f_calle(GpArgument * pArg);
	void external_free(AtType *);
	#if defined(WIN32)
		typedef void * gp_dll_t;

		#define DLL_PATHSEP "\\"
		#define DLL_EXT  ".dll"
		#define DLL_OPEN(f) ((void *)LoadLibrary((f)));
		#define DLL_CLOSE(dl) (FreeLibrary((HINSTANCE)dl))
		#define DLL_SYM(dl, sym) ((void *)GetProcAddress((HINSTANCE)dl, (sym)))
		#define DLL_ERROR(dl) "dynamic library error"
	#elif defined(HAVE_DLFCN_H)
		#include <dlfcn.h>
		typedef void *gp_dll_t;
		#define DLL_PATHSEP "/"
		#define DLL_EXT  ".so"
		#define DLL_OPEN(f) dlopen((f), RTLD_NOW);
		#define DLL_CLOSE(dl) dlclose(dl)
		#define DLL_SYM(dl, sym) dlsym((dl),(sym))
		#define DLL_ERROR(dl) dlerror()
	#elif defined(HAVE_DL_H)
		#include <dl.h>
		typedef shl_t gp_dll_t;
		#define DLL_PATHSEP "/"
		#define DLL_EXT  ".so"
		#define DLL_OPEN(f) shl_load((f), BIND_IMMEDIATE, 0);
		#define DLL_CLOSE(dl) shl_unload(dl)
		__inline__ static DLL_SYM(gp_dll_t dl, const char *sym)
		{
			void * a = 0;
			return shl_findsym(&dl, sym, TYPE_PROCEDURE, &a) ? a : 0x0;
		}
		#define DLL_ERROR(dl) strerror(errno)
	#else
		#error "HAVE_EXTERNAL_FUNCTIONS requires a DLL lib"
	#endif
#endif
//
// INTERNAL.H
//
void eval_reset_after_error();
//
// GP_TIME.H
//
//
// Define the zero point for internal storage of time+date as some number of seconds */
// Through gnuplot version 4.6 this was taken as 1-jan-2000, i.e. 30 years off from  */
// the conventional unix epoch date 1-jan-1970. This caused problems when converting */
// internal <-> external dates given in seconds, so now we change it to agree with   */
// the rest of the unix world.							     */
#if(0)
	#define ZERO_YEAR	2000
	#define JAN_FIRST_WDAY 6	/* 1 jan 2000 was a Saturday (cal 1 2000 on unix) */
	#define SEC_OFFS_SYS	946684800.0	/*  zero gnuplot (2000) - zero system (1970) */
#else
	#define ZERO_YEAR	1970
	#define JAN_FIRST_WDAY 4	/* 1 jan 1970 was a Thursday (cal 1 1970 on unix) */
	#define SEC_OFFS_SYS	0.0	/* difference between internal and external epochs */
#endif
// defines used for timeseries, seconds */
#define YEAR_SEC	31557600.0	/* avg, incl. leap year */
#define MON_SEC		2629800.0	/* YEAR_SEC / 12 */
#define WEEK_SEC	604800.0
#define DAY_SEC		86400.0

// string to *tm 
char * gstrptime(char *, char *, struct tm *, double *);
// seconds to string 
size_t gstrftime(char *, size_t, const char *, double);
// *tm to seconds 
double gtimegm(struct tm *);
// seconds to *tm 
int ggmtime(struct tm *, double);
//
// GLOBALS
//
extern const char * ps_math_color_formulae[];

extern GpGadgets    GpGg;  // @global
extern GpDatafile   GpDf;  // @global
extern GpFit        GpF;   // @global
//extern GpEval       GpGg.Ev;   // @global

#define df_set_skip_after(col,bytes) GpDf.DfSetSkipBefore(col+1,bytes) // Number of bytes to skip after a binary column

extern const GenTable command_tbl[];
extern const GenTable plot_axes_tbl[];
extern const GenTable plot_smooth_tbl[];
extern const GenTable dgrid3d_mode_tbl[];
extern const GenTable save_tbl[];
extern const GenTable set_tbl[];
extern const GenTable test_tbl[];
extern const GenTable set_key_tbl[];
extern const GenTable set_colorbox_tbl[];
extern const GenTable set_palette_tbl[];
extern const GenTable set_pm3d_tbl[];
extern const GenTable color_model_tbl[];
extern const GenTable set_hidden3d_tbl[];
extern const GenTable show_style_tbl[];
extern const GenTable plotstyle_tbl[];
//
// EAM Nov 2008 - this is now dynamic, so we can add colors on the fly 
//
extern       GenTable *user_color_names_tbl;
extern       GenTable *pm3d_color_names_tbl;
extern const int num_predefined_colors;
extern int num_userdefined_colors;
//extern const GpGenFTable command_ftbl[];
extern const GenTable filledcurves_opts_tbl[];
//
// global variables in axis.c
//
extern const  AXIS_DEFAULTS axis_defaults[AXIS_ARRAY_SIZE];
extern GpAxis * shadow_axis_array;
// EAM DEBUG - Dynamic allocation of parallel axes.
extern const  GenTable axisname_tbl[]; // A parsing table for mapping axis names into axis indices. For use by the set/show machinery, mainly
extern const  lp_style_type default_grid_lp; // default grid linetype, to be used by 'unset grid' and 'reset'
//extern boxplot_style boxplot_opts;
//
// global variables for communication with the tic callback functions
//
// The remaining ones are for grid drawing; controlled by 'set grid':
// extern int grid_selection; --- comm'ed out, HBB 20010806
extern lp_style_type grid_lp; // linestyle for major grid lines
extern lp_style_type mgrid_lp; // linestyle for minor grid lines
// extern AXIS_INDEX x_axis, y_axis, z_axis; // axes being used by the current plot

//extern struct GpHistEntry * history;
//extern struct GpHistEntry * cur_entry;
//extern int gnuplot_history_size;
//extern bool history_quiet;
//extern bool history_full;
// these two are global so that plot.c can load them on program entry 
extern char * call_args[10];
extern int    call_argc;
extern LFS  * lf_head;
extern FILE * table_outfile;
extern UdvtEntry * table_var;
extern bool table_mode;
extern int  enable_reset_palette;
extern bool disable_mouse_z;
//extern t_jitter jitter;
//
// Standalone functions
//
void * unused_gp_alloc(size_t size, const char * message);
void * gp_realloc(void * p, size_t size, const char * message);
//void * nextfrom_dynarray(dynarray * array);
void   droplast_dynarray(dynarray * array);
double  *vec(int n);
int     *ivec(int n);
double  **matr(int r, int c);
void    free_matr(double **m);
double  *redim_vec(double **v, int n);
void    solve(double **a, int n, double **b, int m);
void    Givens(double **C, double *d, double *x, int N, int n);
void    Invert_RtR(double **R, double **I, int n);
// Functions for use by THIN_PLATE_SPLINES_GRID method
void    lu_decomp(double **, int, int *, double *);
void    lu_backsubst(double **, int n, int *, double *);
double  enorm_vec(int n, const double *x);
double  sumsq_vec(int n, const double *x);

//void   save_functions(FILE *fp);
//void   save_variables(FILE *fp);
//void   save_set(GpCommand & rC, FILE *fp);
void   save_term(FILE *fp);
//void   save_all(GpCommand & rC, FILE *fp);
void   save_position(FILE *, GpPosition *, int, bool);
void   save_prange(FILE *, GpAxis *);
void   save_link(FILE *, GpAxis *);
void   save_nonlinear(FILE *, GpAxis *);
void   save_textcolor(FILE *, const t_colorspec *);
void   save_pm3dcolor(FILE *, const t_colorspec *);
void   save_fillstyle(FILE *, const fill_style_type *);
//void   save_offsets(FILE *, char *);
void   save_histogram_opts(FILE *fp);
#ifdef EAM_OBJECTS
	void   save_object(FILE *, int);
#endif
void   save_style_parallel(FILE *);
void   save_data_func_style(FILE *, const char *, enum PLOT_STYLE);
void   save_linetype(FILE *, lp_style_type *, bool);
void   save_dashtype(FILE *, int, const t_dashtype *);
void   save_num_or_time_input(FILE *, double x, GpAxis *);
//void   save_bars(FILE *);
void   save_array_content(FILE *, t_value *);

void   squash_spaces(char *);
bool   existdir(const char *);
bool   existfile(const char *);
char * getusername();
bool   contains8bit(const char *s);
bool   utf8toulong(ulong * wch, const char ** str);
size_t strlen_utf8(const char *s);
size_t gp_strlen(const char *s);
char * gp_strchrn(const char *s, int N);
bool   streq(const char *a, const char *b);
size_t strappend(char **dest, size_t *size, size_t len, const char *src);
char * num_to_str(double r);
char * value_to_str(t_value *val, bool need_quotes);

void   clip_put_text(uint, uint, char *);
void   clip_move(uint x, uint y);
void   reset_textcolor(const t_colorspec * tc);
void   default_arrow_style(arrow_style_type * arrow);
//void   apply_head_properties(arrow_style_type * arrow_properties);
int    label_width(const char *, int *);

char * copy_or_invent_formatstring(GpAxis *);
double quantize_normal_tics(double, int);
void add_tic_user(GpAxis *, char *, double, int);
/* set widest_tic_label: length of the longest tics label */
//void widest_tic_callback(GpTermEntry * pT, GpTicCallbackParam * pP /*GpAxis *, double place, char *text, int ticlevel, lp_style_type grid, ticmark * */);
void gstrdms(char *label, char *format, double value);
void clone_linked_axes(GpAxis *axis1, GpAxis *axis2);
GpAxis *get_shadow_axis(GpAxis *axis);
//double get_num_or_time(GpAxis *);
void   init_histogram(histogram_style *hist, GpTextLabel *title);
void   free_histlist(histogram_style *hist);
//void   process_image(void *plot, t_procimg_action action);
bool   check_for_variable_color(CurvePoints *plot, double *colorvalue);

int filter_boxplot(CurvePoints *);
int find_maxl_keys(const CurvePoints *plots, int count, int *kcnt);
void   mat_scale(double sx, double sy, double sz, double mat[4][4]);
void   mat_rot_x(double teta, double mat[4][4]);
void   mat_rot_z(double teta, double mat[4][4]);
void   mat_mult(double mat_res[4][4], double mat1[4][4], double mat2[4][4]);

//gnuplot_contours * contour(int num_isolines, iso_curve *iso_lines);
int    solve_tri_diag(tri_diag m[], double r[], double x[], int n);

// main gray --> rgb color mapping 
//void   rgb1_from_gray(double gray, rgb_color *color);
void   rgb255_from_rgb1(rgb_color rgb1, rgb255_color *rgb255);
// main gray --> rgb color mapping as above, with take care of palette maxcolors 
//void   rgb1maxcolors_from_gray(double gray, rgb_color *color);
void   rgb255maxcolors_from_gray(double gray, rgb255_color *rgb255);
//double quantize_gray(double gray);
// HSV --> RGB user-visible function hsv2rgb(h,s,v) 
uint hsv2rgb(rgb_color *color);
// used to (de-)serialize color/gradient information 
char *gradient_entry_to_str(gradient_struct *gs );
void   str_to_gradient_entry(char *s, gradient_struct *gs );
// check if two palettes p1 and p2 differ 
int palettes_differ( t_sm_palette *p1, t_sm_palette *p2);
// construct minimal gradient to approximate palette 
gradient_struct *approximate_palette(t_sm_palette *palette, int maxsamples, double allowed_deviation, int *gradient_num );
double GetColorValueFromFormula(int formula, double x);

//void   statsrequest();

// The set and show commands, in setshow.c 
//void   show_command();
// and some accessible support functions 
void   show_version(GpCommand & rC, FILE *fp);
char * conv_text(const char *s);
void   delete_linestyle(linestyle_def **, linestyle_def *, linestyle_def *);
void   delete_dashtype(custom_dashtype_def *, custom_dashtype_def *);
extern GpTextLabel * new_text_label(int tag);
extern void disp_value(FILE *, t_value *, bool);
extern ticmark * prune_dataticks(ticmark *list);

void   edf_filetype_function();
void   png_filetype_function();
void   gif_filetype_function();
void   jpeg_filetype_function();
int    df_libgd_get_pixel(int i, int j, int component);

int    df_2dbinary(CurvePoints *);
int    df_3dmatrix(SurfacePoints *, int);
void   df_reset_after_error();

void   df_show_datasizes(FILE *fp);
void   df_show_filetypes(FILE *fp);
void   datablock_command(GpCommand & rC);
char ** get_datablock(char *name);
void   gpfree_datablock(t_value *datablock_value);
void   append_to_datablock(t_value *datablock_value, const char * line);

iso_curve * iso_alloc(int num);
void   iso_extend(iso_curve *ip, int num);
void   iso_free(iso_curve *ip);
const char *expand_call_arg(int c);
FILE * lf_top();
bool   lf_pop(GpCommand & rC);
void   lf_push(FILE *fp, char *name, char *cmdline);
void   load_file_error();
FILE * loadpath_fopen(const char *, const char *);
char * fontpath_fullname(const char *);
void   push_terminal(int is_interactive);
void   pop_terminal();
enum PLOT_STYLE get_style(GpCommand & rC);
void   get_filledcurves_style_options(GpCommand & rC, filledcurves_opts *);
void   filledcurves_options_tofile(filledcurves_opts *, FILE *);
void   arrow_parse(GpCommand & rC, arrow_style_type *, bool);
void   arrow_use_properties(arrow_style_type *arrow, int tag);
long   parse_color_name();
void   get_image_options(t_image *image);
void   print_table(CurvePoints * pFirstPlot, int plot_num);
void   print_3dtable(int pcount);
//void   jitter_points(CurvePoints *plot);
//void   show_jitter();
//void   unset_jitter();
//void   save_jitter(FILE *);
void   show_hidden3doptions();
void   reset_hidden3doptions();
void   save_hidden3doptions(FILE *fp);
void   init_hidden_line_removal();
void   reset_hidden_line_removal();
void   term_hidden_line_removal();
void   plot3d_hidden(SurfacePoints *plots, int pcount);
void   draw_label_hidden(GpVertex *, lp_style_type *, int, int);
//void   gen_interp(CurvePoints *plot);
void   gen_interp_unwrap(CurvePoints *plot);
void   gen_interp_frequency(CurvePoints *plot);
//void   mcs_interp(CurvePoints *plot);
void   sort_points(CurvePoints *plot);
//void   cp_implode(CurvePoints *cp);
void   make_bins(CurvePoints *plot, int nbins, double binlow, double binhigh);

void   cp_free(CurvePoints *cp);
void   cp_extend(CurvePoints *cp, int num);
GpTextLabel *store_label(GpTextLabel *, GpCoordinate *, int i, char * string, double colorval);
void   sp_free(SurfacePoints *sp);

int    get_pm3d_at_option(GpCommand & rC, char *pm3d_where);
void   pm3d_depth_queue_clear();
void   pm3d_reset();
void   pm3d_rearrange_scan_array(SurfacePoints* this_plot, iso_curve*** first_ptr, int* first_n, int* first_invert, iso_curve*** second_ptr, int* second_n, int* second_invert);
bool   is_plot_with_palette();
bool   is_plot_with_colorbox();

void   event_plotdone();
//void   UpdateStatusline();
int    plot_mode(int mode);
//void   event_reset(GpEvent *ge);
//
//
//
double chisq_cdf(int dof, double chisqr);
//
#endif // __GNUPLOT_H
