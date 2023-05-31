/*------------------------------------------------------------------------
 *  Copyright 2007-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/
#ifndef _ZBAR_H_
#define _ZBAR_H_

#include <slib.h>
//#include "refcnt.h"

#if defined(_WIN32)
	//#include <windows.h>
	typedef LONG refcnt_t;
	static inline int _zbar_refcnt(refcnt_t * cnt, int delta)
	{
		int rc = -1;
		if(delta > 0)
			while(delta--)
				rc = InterlockedIncrement(cnt);
		else if(delta < 0)
			while(delta++)
				rc = InterlockedDecrement(cnt);
		assert(rc >= 0);
		return (rc);
	}
#elif defined(TARGET_OS_MAC)
	#include <libkern/OSAtomic.h>
	typedef int32_t refcnt_t;
	static inline int _zbar_refcnt(refcnt_t * cnt, int delta)
	{
		int rc = OSAtomicAdd32Barrier(delta, cnt);
		assert(rc >= 0);
		return (rc);
	}
#elif defined(HAVE_LIBPTHREAD)
	#include <pthread.h>
	typedef int refcnt_t;
	extern pthread_mutex_t _zbar_reflock;
	static inline int _zbar_refcnt(refcnt_t * cnt, int delta)
	{
		pthread_mutex_lock(&_zbar_reflock);
		int rc = (*cnt += delta);
		pthread_mutex_unlock(&_zbar_reflock);
		assert(rc >= 0);
		return (rc);
	}
#else
	typedef int refcnt_t;
	static inline int _zbar_refcnt(refcnt_t * cnt, int delta)
	{
		int rc = (*cnt += delta);
		assert(rc >= 0);
		return (rc);
	}
#endif

void _zbar_refcnt_init();

#define ZBAR_VERSION_MAJOR 0
#define ZBAR_VERSION_MINOR 10
#define ZBAR_VIDEO_IMAGES_MAX  4 // number of images to preallocate
#define ERRINFO_MAGIC          (0x5252457a) // "zERR" (LE)

#define ENABLE_EAN
#define ENABLE_I25
#define ENABLE_DATABAR
#define ENABLE_CODABAR
#define ENABLE_CODE39
#define ENABLE_CODE93
#define ENABLE_CODE128
#define ENABLE_PDF417
#define ENABLE_QRCODE

#define HAVE_SYS_STAT_H
#define HAVE_FCNTL_H
#define HAVE_LIBJPEG
//
// size of bar width history (implementation assumes power of two)
//
#ifndef DECODE_WINDOW
	#define DECODE_WINDOW  16
#endif
#ifndef PRIx32
	#define PRIx32        "lx"
#endif

struct zbar_decoder_t;
struct zbar_image_t;

/** @file
 * ZBar Barcode Reader C API definition
 */

/** @mainpage
 *
 * interface to the barcode reader is available at several levels.
 * most applications will want to use the high-level interfaces:
 *
 * @section high-level High-Level Interfaces
 *
 * these interfaces wrap all library functionality into an easy-to-use
 * package for a specific toolkit:
 * - the "GTK+ 2.x widget" may be used with GTK GUI applications.  a
 * Python wrapper is included for PyGtk
 * - the @ref zbar::QZBar "Qt4 widget" may be used with Qt GUI
 * applications
 * - the Processor interface (in @ref c-processor "C" or @ref
 * zbar::Processor "C++") adds a scanning window to an application
 * with no GUI.
 *
 * @section mid-level Intermediate Interfaces
 *
 * building blocks used to construct high-level interfaces:
 * - the ImageScanner (in @ref c-imagescanner "C" or @ref
 * zbar::ImageScanner "C++") looks for barcodes in a library defined
 * image object
 * - the Window abstraction (in @ref c-window "C" or @ref
 * zbar::Window "C++") sinks library images, displaying them on the
 * platform display
 * - the Video abstraction (in @ref c-video "C" or @ref zbar::Video
 * "C++") sources library images from a video device
 *
 * @section low-level Low-Level Interfaces
 *
 * direct interaction with barcode scanning and decoding:
 * - the Scanner (in @ref c-scanner "C" or @ref zbar::Scanner "C++")
 * looks for barcodes in a linear intensity sample stream
 * - the Decoder (in @ref c-decoder "C" or @ref zbar::Decoder "C++")
 * extracts barcodes from a stream of bar and space widths
 */

#ifdef __cplusplus

/** C++ namespace for library interfaces */
//namespace zbar {
// extern "C" {
#endif

/** @name Global library interfaces */
/*@{*/

/** "color" of element: bar or space. */
enum zbar_color_t {
	ZBAR_SPACE = 0, /**< light area or space between bars */
	ZBAR_BAR = 1,  /**< dark area or colored bar segment */
};
//
// decoded symbol type
//
enum zbar_symbol_type_t {
	ZBAR_NONE        =      0, /**< no symbol decoded */
	ZBAR_PARTIAL     =      1, /**< intermediate status */
	ZBAR_EAN2        =      2, /**< GS1 2-digit add-on */
	ZBAR_EAN5        =      5, /**< GS1 5-digit add-on */
	ZBAR_EAN8        =      8, /**< EAN-8 */
	ZBAR_UPCE        =      9, /**< UPC-E */
	ZBAR_ISBN10      =     10, /**< ISBN-10 (from EAN-13). @since 0.4 */
	ZBAR_UPCA        =     12, /**< UPC-A */
	ZBAR_EAN13       =     13, /**< EAN-13 */
	ZBAR_ISBN13      =     14, /**< ISBN-13 (from EAN-13). @since 0.4 */
	ZBAR_COMPOSITE   =     15, /**< EAN/UPC composite */
	ZBAR_I25 =     25, /**< Interleaved 2 of 5. @since 0.4 */
	ZBAR_DATABAR     =     34, /**< GS1 DataBar (RSS). @since 0.11 */
	ZBAR_DATABAR_EXP =     35, /**< GS1 DataBar Expanded. @since 0.11 */
	ZBAR_CODABAR     =     38, /**< Codabar. @since 0.11 */
	ZBAR_CODE39      =     39, /**< Code 39. @since 0.4 */
	ZBAR_PDF417      =     57, /**< PDF417. @since 0.6 */
	ZBAR_QRCODE      =     64, /**< QR Code. @since 0.10 */
	ZBAR_CODE93      =     93, /**< Code 93. @since 0.11 */
	ZBAR_CODE128     =    128, /**< Code 128 */

	/** mask for base symbol type.
	 * @deprecated in 0.11, remove this from existing code
	 */
	ZBAR_SYMBOL      = 0x00ff,
	/** 2-digit add-on flag.
	 * @deprecated in 0.11, a ::ZBAR_EAN2 component is used for
	 * 2-digit GS1 add-ons
	 */
	ZBAR_ADDON2      = 0x0200,
	/** 5-digit add-on flag.
	 * @deprecated in 0.11, a ::ZBAR_EAN5 component is used for
	 * 5-digit GS1 add-ons
	 */
	ZBAR_ADDON5      = 0x0500,
	/** add-on flag mask.
	 * @deprecated in 0.11, GS1 add-ons are represented using composite
	 * symbols of type ::ZBAR_COMPOSITE; add-on components use ::ZBAR_EAN2
	 * or ::ZBAR_EAN5
	 */
	ZBAR_ADDON       = 0x0700,
};
//
// decoded symbol coarse orientation.
// @since 0.11
//
enum zbar_orientation_t {
	ZBAR_ORIENT_UNKNOWN = -1, /**< unable to determine orientation */
	ZBAR_ORIENT_UP,         /**< upright, read left to right */
	ZBAR_ORIENT_RIGHT,      /**< sideways, read top to bottom */
	ZBAR_ORIENT_DOWN,       /**< upside-down, read right to left */
	ZBAR_ORIENT_LEFT,       /**< sideways, read bottom to top */
};
//
// error codes
//
enum zbar_error_t {
	ZBAR_OK = 0,            /**< no error */
	ZBAR_ERR_NOMEM,         /**< out of memory */
	ZBAR_ERR_INTERNAL,      /**< internal library error */
	ZBAR_ERR_UNSUPPORTED,   /**< unsupported request */
	ZBAR_ERR_INVALID,       /**< invalid request */
	ZBAR_ERR_SYSTEM,        /**< system error */
	ZBAR_ERR_LOCKING,       /**< locking error */
	ZBAR_ERR_BUSY,          /**< all resources busy */
	ZBAR_ERR_XDISPLAY,      /**< X11 display error */
	ZBAR_ERR_XPROTO,        /**< X11 protocol error */
	ZBAR_ERR_CLOSED,        /**< output window is closed */
	ZBAR_ERR_WINAPI,        /**< windows system error */
	ZBAR_ERR_NUM            /**< number of error codes */
};

/** decoder configuration options.
 * @since 0.4
 */
typedef enum zbar_config_e {
	ZBAR_CFG_ENABLE = 0,    /**< enable symbology/feature */
	ZBAR_CFG_ADD_CHECK,     /**< enable check digit when optional */
	ZBAR_CFG_EMIT_CHECK,    /**< return check digit when present */
	ZBAR_CFG_ASCII,         /**< enable full ASCII character set */
	ZBAR_CFG_NUM,           /**< number of boolean decoder configs */
	ZBAR_CFG_MIN_LEN = 0x20, /**< minimum data length for valid decode */
	ZBAR_CFG_MAX_LEN,       /**< maximum data length for valid decode */
	ZBAR_CFG_UNCERTAINTY = 0x40, /**< required video consistency frames */
	ZBAR_CFG_POSITION = 0x80, /**< enable scanner to collect position data */
	ZBAR_CFG_X_DENSITY = 0x100, /**< image scanner vertical scan density */
	ZBAR_CFG_Y_DENSITY,     /**< image scanner horizontal scan density */
} zbar_config_t;

/** decoder symbology modifier flags.
 * @since 0.11
 */
typedef enum zbar_modifier_e {
	/** barcode tagged as GS1 (EAN.UCC) reserved
	 * (eg, FNC1 before first data character).
	 * data may be parsed as a sequence of GS1 AIs
	 */
	ZBAR_MOD_GS1 = 0,
	ZBAR_MOD_AIM, // barcode tagged as AIM reserved (eg, FNC1 after first character or digit pair)
	ZBAR_MOD_NUM, // number of modifiers 
} zbar_modifier_t;
//
//
//
#if 0 // @sobolev {
	#ifndef DEBUG_LEVEL
		#ifdef __GNUC__
			/* older versions of gcc (< 2.95) require a named varargs parameter */
			#define dbprintf(args...) while(0)
		#else
			/* unfortunately named vararg parameter is a gcc-specific extension */
			#define dbprintf(...) while(0)
		#endif
	#else
		#ifdef __GNUC__
			#define dbprintf(level, args...) do { if((level) <= DEBUG_LEVEL) slfprintf_stderr(args); } while(0)
		#else
			#define dbprintf(level, ...) do { if((level) <= DEBUG_LEVEL) slfprintf_stderr(__VA_ARGS__); } while(0)
		#endif
	#endif /* DEBUG_LEVEL */
#endif // } @sobolev

void cdecl ZBarSetupDebugLog(int debugLevel, const char * pOutFileName);
void cdecl dbprintf(int level, const char * pFormat, ...);

/* spew warnings for non-fatal assertions.
 * returns specified error code if assertion fails.
 * NB check/return is still performed for NDEBUG
 * only the message is inhibited
 * FIXME don't we need varargs hacks here?
 */
#ifndef NDEBUG
	/*
	#define zassert__(condition, retval, format, ...) do {                   \
        if(!(condition)) {                                              \
            slfprintf_stderr("WARNING: %s:%d: %s: Assertion \"%s\" failed.\n\t" format, __FILE__, __LINE__, __FUNCTION__, #condition , ##__VA_ARGS__);                                     \
            return (retval);                                             \
        }                                                               \
    } while(0)
	*/
#else
	// #define zassert__(condition, retval, format, ...) do { if(!(condition)) return (retval); } while(0)
#endif

enum errsev_t {
	SEV_FATAL   = -2,       /* application must terminate */
	SEV_ERROR   = -1,       /* might be able to recover and continue */
	SEV_OK      =  0,
	SEV_WARNING =  1,       /* unexpected condition */
	SEV_NOTE    =  2,       /* fyi */
};

enum errmodule_t {
	ZBAR_MOD_PROCESSOR,
	ZBAR_MOD_VIDEO,
	ZBAR_MOD_WINDOW,
	ZBAR_MOD_IMAGE_SCANNER,
	ZBAR_MOD_UNKNOWN,
};

struct errinfo_t {
	uint32 magic; /* just in case */
	errmodule_t module; /* reporting module */
	char * buf; /* formatted and passed to application */
	int    errnum; /* errno for system errors */
	errsev_t sev;
	zbar_error_t type;
	const char * func; /* reporting function */
	const char * detail; /* description */
	char * arg_str; /* single string argument */
	int    arg_int; /* single integer argument */
};

#ifdef _WIN32
	#define ZFLUSH fflush(stderr);
#else
	#define ZFLUSH
#endif

extern int _zbar_verbosity;

void cdecl zprintf(int level, const char * pFormat, ...);
int err_copy(void * dst_c, void * src_c);
int err_capture(const void * container, errsev_t sev, zbar_error_t type, const char * func, const char * detail);
int err_capture_str(const void * container, errsev_t sev, zbar_error_t type, const char * func, const char * detail, const char * arg);
int err_capture_int(const void * container, errsev_t sev, zbar_error_t type, const char * func, const char * detail, int arg);
int err_capture_num(const void * container, errsev_t sev, zbar_error_t type, const char * func, const char * detail, int num);
void err_init(errinfo_t * err, errmodule_t module);
void err_cleanup(errinfo_t * err);
//
// simple platform mutex abstraction
//
#if defined(_WIN32)

#define DEBUG_LOCKS
#ifdef DEBUG_LOCKS

struct zbar_mutex_t {
	int count;
	CRITICAL_SECTION mutex;
};

static inline int _zbar_mutex_init(zbar_mutex_t * lock)
{
	lock->count = 1;
	InitializeCriticalSection(&lock->mutex);
	return 0;
}

static inline void _zbar_mutex_destroy(zbar_mutex_t * lock)
{
	DeleteCriticalSection(&lock->mutex);
}

static inline int _zbar_mutex_lock(zbar_mutex_t * lock)
{
	EnterCriticalSection(&lock->mutex);
	if(lock->count++ < 1)
		assert(0);
	return 0;
}

static inline int _zbar_mutex_unlock(zbar_mutex_t * lock)
{
	if(lock->count-- <= 1)
		assert(0);
	LeaveCriticalSection(&lock->mutex);
	return 0;
}

#else

typedef CRITICAL_SECTION zbar_mutex_t;

static inline int _zbar_mutex_init(zbar_mutex_t * lock)
{
	InitializeCriticalSection(lock);
	return 0;
}

static inline void _zbar_mutex_destroy(zbar_mutex_t * lock)
{
	DeleteCriticalSection(lock);
}

static inline int _zbar_mutex_lock(zbar_mutex_t * lock)
{
	EnterCriticalSection(lock);
	return 0;
}

static inline int _zbar_mutex_unlock(zbar_mutex_t * lock)
{
	LeaveCriticalSection(lock);
	return 0;
}

#endif

#elif defined(HAVE_LIBPTHREAD)

#include <pthread.h>

typedef pthread_mutex_t zbar_mutex_t;

static inline int _zbar_mutex_init(zbar_mutex_t * lock)
{
#ifdef DEBUG_LOCKS
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	int rc = pthread_mutex_init(lock, &attr);
	pthread_mutexattr_destroy(&attr);
	return (rc);
#else
	return (pthread_mutex_init(lock, NULL));
#endif
}

static inline void _zbar_mutex_destroy(zbar_mutex_t * lock)
{
	pthread_mutex_destroy(lock);
}

static inline int _zbar_mutex_lock(zbar_mutex_t * lock)
{
	int rc = pthread_mutex_lock(lock);
#ifdef DEBUG_LOCKS
	assert(!rc);
#endif
	/* FIXME save system code */
	/*rc = err_capture(proc, SEV_ERROR, ZBAR_ERR_LOCKING, __FUNCTION__, "unable to lock processor");*/
	return (rc);
}

static inline int _zbar_mutex_unlock(zbar_mutex_t * lock)
{
	int rc = pthread_mutex_unlock(lock);
#ifdef DEBUG_LOCKS
	assert(!rc);
#endif
	/* FIXME save system code */
	return (rc);
}

#else

typedef int zbar_mutex_t[0];

#define _zbar_mutex_init(l) -1
#define _zbar_mutex_destroy(l)
#define _zbar_mutex_lock(l) 0
#define _zbar_mutex_unlock(l) 0

#endif

/** retrieve runtime library version information.
 * @param major set to the running major version (unless NULL)
 * @param minor set to the running minor version (unless NULL)
 * @returns 0
 */
extern int zbar_version(uint * major, uint * minor);
/** set global library debug level.
 * @param verbosity desired debug level.  higher values create more spew
 */
extern void zbar_set_verbosity(int verbosity);
/** increase global library debug level.
 * eg, for -vvvv
 */
extern void zbar_increase_verbosity(void);
/** retrieve string name for symbol encoding.
 * @param sym symbol type encoding
 * @returns the static string name for the specified symbol type,
 * or "UNKNOWN" if the encoding is not recognized
 */
extern const char * zbar_get_symbol_name(zbar_symbol_type_t sym);

/** retrieve string name for addon encoding.
 * @param sym symbol type encoding
 * @returns static string name for any addon, or the empty string
 * if no addons were decoded
 * @deprecated in 0.11
 */
extern const char * zbar_get_addon_name(zbar_symbol_type_t sym);

/** retrieve string name for configuration setting.
 * @param config setting to name
 * @returns static string name for config,
 * or the empty string if value is not a known config
 */
extern const char * zbar_get_config_name(zbar_config_t config);

/** retrieve string name for modifier.
 * @param modifier flag to name
 * @returns static string name for modifier,
 * or the empty string if the value is not a known flag
 */
extern const char * zbar_get_modifier_name(zbar_modifier_t modifier);

/** retrieve string name for orientation.
 * @param orientation orientation encoding
 * @returns the static string name for the specified orientation,
 * or "UNKNOWN" if the orientation is not recognized
 * @since 0.11
 */
extern const char * zbar_get_orientation_name(zbar_orientation_t orientation);

/** parse a configuration string of the form "[symbology.]config[=value]".
 * the config must match one of the recognized names.
 * the symbology, if present, must match one of the recognized names.
 * if symbology is unspecified, it will be set to 0.
 * if value is unspecified it will be set to 1.
 * @returns 0 if the config is parsed successfully, 1 otherwise
 * @since 0.4
 */
extern int zbar_parse_config(const char * config_string, zbar_symbol_type_t * symbology, zbar_config_t * config, int * value);

/** consistently compute fourcc values across architectures
 * (adapted from v4l2 specification)
 * @since 0.11
 */
#define zbar_fourcc(a, b, c, d) ((ulong)(a) | ((ulong)(b) << 8) | ((ulong)(c) << 16) | ((ulong)(d) << 24))

/** parse a fourcc string into its encoded integer value.
 * @since 0.11
 */
static inline ulong zbar_fourcc_parse(const char * format)
{
	ulong fourcc = 0;
	if(format) {
		for(int i = 0; i < 4 && format[i]; i++)
			fourcc |= ((ulong)format[i]) << (i * 8);
	}
	return (fourcc);
}

/** @internal type unsafe error API (don't use) */
extern int _zbar_error_spew(const void * object, int verbosity);
extern const char * _zbar_error_string(const void * object, int verbosity);
extern zbar_error_t _zbar_get_error_code(const void * object);

/*@}*/

struct zbar_symbol_s;
typedef struct zbar_symbol_s zbar_symbol_t;
struct zbar_symbol_set_s;
typedef struct zbar_symbol_set_s zbar_symbol_set_t;

/*------------------------------------------------------------*/
/** @name Symbol interface
 * decoded barcode symbol result object.  stores type, data, and image
 * location of decoded symbol.  all memory is owned by the library
 */
/*@{*/

/** @typedef zbar_symbol_t
 * opaque decoded symbol object.
 */

/** symbol reference count manipulation.
 * increment the reference count when you store a new reference to the
 * symbol.  decrement when the reference is no longer used.  do not
 * refer to the symbol once the count is decremented and the
 * containing image has been recycled or destroyed.
 * @note the containing image holds a reference to the symbol, so you
 * only need to use this if you keep a symbol after the image has been
 * destroyed or reused.
 * @since 0.9
 */
extern void zbar_symbol_ref(const zbar_symbol_t * symbol, int refs);
/** retrieve type of decoded symbol.
 * @returns the symbol type
 */
extern zbar_symbol_type_t zbar_symbol_get_type(const zbar_symbol_t * symbol);
/** retrieve symbology boolean config settings.
 * @returns a bitmask indicating which configs were set for the detected
 * symbology during decoding.
 * @since 0.11
 */
extern uint zbar_symbol_get_configs(const zbar_symbol_t * symbol);
/** retrieve symbology modifier flag settings.
 * @returns a bitmask indicating which characteristics were detected
 * during decoding.
 * @since 0.11
 */
extern uint zbar_symbol_get_modifiers(const zbar_symbol_t * symbol);
/** retrieve data decoded from symbol.
 * @returns the data string
 */
extern const char * zbar_symbol_get_data(const zbar_symbol_t * symbol);

/** retrieve length of binary data.
 * @returns the length of the decoded data
 */
extern uint zbar_symbol_get_data_length(const zbar_symbol_t * symbol);

/** retrieve a symbol confidence metric.
 * @returns an unscaled, relative quantity: larger values are better
 * than smaller values, where "large" and "small" are application
 * dependent.
 * @note expect the exact definition of this quantity to change as the
 * metric is refined.  currently, only the ordered relationship
 * between two values is defined and will remain stable in the future
 * @since 0.9
 */
extern int zbar_symbol_get_quality(const zbar_symbol_t * symbol);

/** retrieve current cache count.  when the cache is enabled for the
 * image_scanner this provides inter-frame reliability and redundancy
 * information for video streams.
 * @returns < 0 if symbol is still uncertain.
 * @returns 0 if symbol is newly verified.
 * @returns > 0 for duplicate symbols
 */
extern int zbar_symbol_get_count(const zbar_symbol_t * symbol);

/** retrieve the number of points in the location polygon.  the
 * location polygon defines the image area that the symbol was
 * extracted from.
 * @returns the number of points in the location polygon
 * @note this is currently not a polygon, but the scan locations
 * where the symbol was decoded
 */
extern uint zbar_symbol_get_loc_size(const zbar_symbol_t * symbol);

/** retrieve location polygon x-coordinates.
 * points are specified by 0-based index.
 * @returns the x-coordinate for a point in the location polygon.
 * @returns -1 if index is out of range
 */
extern int zbar_symbol_get_loc_x(const zbar_symbol_t * symbol, uint index);
/** retrieve location polygon y-coordinates.
 * points are specified by 0-based index.
 * @returns the y-coordinate for a point in the location polygon.
 * @returns -1 if index is out of range
 */
extern int zbar_symbol_get_loc_y(const zbar_symbol_t * symbol, uint index);
/** retrieve general orientation of decoded symbol.
 * @returns a coarse, axis-aligned indication of symbol orientation or
 * ::ZBAR_ORIENT_UNKNOWN if unknown
 * @since 0.11
 */
extern zbar_orientation_t zbar_symbol_get_orientation(const zbar_symbol_t * symbol);
/** iterate the set to which this symbol belongs (there can be only one).
 * @returns the next symbol in the set, or
 * @returns NULL when no more results are available
 */
extern const zbar_symbol_t * zbar_symbol_next(const zbar_symbol_t * symbol);

/** retrieve components of a composite result.
 * @returns the symbol set containing the components
 * @returns NULL if the symbol is already a physical symbol
 * @since 0.10
 */
extern const zbar_symbol_set_t* zbar_symbol_get_components(const zbar_symbol_t * symbol);

/** iterate components of a composite result.
 * @returns the first physical component symbol of a composite result
 * @returns NULL if the symbol is already a physical symbol
 * @since 0.10
 */
extern const zbar_symbol_t* zbar_symbol_first_component(const zbar_symbol_t * symbol);

/** print XML symbol element representation to user result buffer.
 * @see http://zbar.sourceforge.net/2008/barcode.xsd for the schema.
 * @param symbol is the symbol to print
 * @param buffer is the inout result pointer, it will be reallocated
 * with a larger size if necessary.
 * @param buflen is inout length of the result buffer.
 * @returns the buffer pointer
 * @since 0.6
 */
extern char * zbar_symbol_xml(const zbar_symbol_t * symbol, char ** buffer, uint * buflen);

/*@}*/

/*------------------------------------------------------------*/
/** @name Symbol Set interface
 * container for decoded result symbols associated with an image
 * or a composite symbol.
 * @since 0.10
 */
/*@{*/

/** @typedef zbar_symbol_set_t
 * opaque symbol iterator object.
 * @since 0.10
 */

/** reference count manipulation.
 * increment the reference count when you store a new reference.
 * decrement when the reference is no longer used.  do not refer to
 * the object any longer once references have been released.
 * @since 0.10
 */
extern void zbar_symbol_set_ref(/*const*/zbar_symbol_set_t * symbols, int refs);
/** retrieve set size.
 * @returns the number of symbols in the set.
 * @since 0.10
 */
extern int zbar_symbol_set_get_size(const zbar_symbol_set_t * symbols);
/** set iterator.
 * @returns the first decoded symbol result in a set
 * @returns NULL if the set is empty
 * @since 0.10
 */
extern const zbar_symbol_t* zbar_symbol_set_first_symbol(const zbar_symbol_set_t * symbols);

/** raw result iterator.
 * @returns the first decoded symbol result in a set, *before* filtering
 * @returns NULL if the set is empty
 * @since 0.11
 */
extern const zbar_symbol_t* zbar_symbol_set_first_unfiltered(const zbar_symbol_set_t * symbols);

/*@}*/
//
//
//
enum video_interface_t {
	VIDEO_INVALID = 0,      /* uninitialized */
	VIDEO_V4L1,             /* v4l protocol version 1 */
	VIDEO_V4L2,             /* v4l protocol version 2 */
	VIDEO_VFW,              /* video for windows */
};

enum video_iomode_t {
	VIDEO_READWRITE = 1,    /* standard system calls */
	VIDEO_MMAP,             /* mmap interface */
	VIDEO_USERPTR,          /* userspace buffers */
};

typedef struct video_state_s video_state_t;

struct zbar_video_t {
	errinfo_t err; /* error reporting */
	int fd; /* open camera device */
	uint width, height; /* video frame size */
	video_interface_t intf; /* input interface type */
	video_iomode_t iomode; /* video data transfer mode */
	uint initialized : 1; /* format selected and images mapped */
	uint active      : 1; /* current streaming state */
	uint32 format; /* selected fourcc */
	uint palette; /* v4l1 format index corresponding to format */
	uint32 * formats; /* 0 terminated list of supported formats */
	ulong datalen; /* size of image data for selected format */
	ulong buflen; /* total size of image data buffer */
	void * buf; /* image data buffer */
	uint frame; /* frame count */
	zbar_mutex_t qlock; /* lock image queue */
	int num_images; /* number of allocated images */
	zbar_image_t ** images; /* indexed list of images */
	zbar_image_t * nq_image; /* last image enqueued */
	zbar_image_t * dq_image; /* first image to dequeue (when ordered) */
	zbar_image_t * shadow_image; /* special case internal double buffering */
	video_state_t * state; /* platform/interface specific state */
#ifdef HAVE_LIBJPEG
	struct jpeg_decompress_struct * jpeg; /* JPEG decompressor */
	zbar_image_t * jpeg_img; /* temporary image */
#endif
	/* interface dependent methods */
	int (* init)(zbar_video_t*, uint32);
	int (* cleanup)(zbar_video_t*);
	int (* start)(zbar_video_t*);
	int (* stop)(zbar_video_t*);
	int (* nq)(zbar_video_t*, zbar_image_t*);
	zbar_image_t* (*dq)(zbar_video_t*);
};
//
// video.next_image and video.recycle_image have to be thread safe wrt/other apis
//
int video_lock(zbar_video_t * vdo);
int video_unlock(zbar_video_t * vdo);
int video_nq_image(zbar_video_t * vdo, zbar_image_t * img);
zbar_image_t * video_dq_image(zbar_video_t * vdo);
//
// PAL interface
//
extern int _zbar_video_open(zbar_video_t*, const char*);

/*------------------------------------------------------------*/
/** @name Image interface
 * stores image data samples along with associated format and size
 * metadata
 */
/*@{*/

#define fourcc zbar_fourcc
//
// unpack size/location of component
//
#define RGB_SIZE(c)   ((c) >> 5)
#define RGB_OFFSET(c) ((c) & 0x1f)
//
// coarse image format categorization.
// to limit conversion variations
//
enum zbar_format_group_t {
	ZBAR_FMT_GRAY,
	ZBAR_FMT_YUV_PLANAR,
	ZBAR_FMT_YUV_PACKED,
	ZBAR_FMT_RGB_PACKED,
	ZBAR_FMT_YUV_NV,
	ZBAR_FMT_JPEG,

	ZBAR_FMT_NUM // enum size
};

//
// cleanup handler callback function.
// called to free sample data when an image is destroyed.
//
typedef void (*zbar_image_cleanup_handler_t)(zbar_image_t * image);

struct zbar_image_t {
	uint32 Format;          // fourcc image format code
	uint width, height; // image size
	const void * P_Data;    // image sample data
	ulong datalen;          // allocated/mapped size of data
	uint crop_x;        // crop rectangle
	uint crop_y;
	uint crop_w;
	uint crop_h;
	void * userdata; /* user specified data associated w/image */
	zbar_image_cleanup_handler_t cleanup; // cleanup handler
	refcnt_t refcnt; /* reference count */
	zbar_video_t * src; /* originator */
	int srcidx; /* index used by originator */
	zbar_image_t * next; /* internal image lists */
	uint seq; /* page/frame sequence number */
	zbar_symbol_set_t * syms; /* decoded result set */
};
//
// description of an image format
//
struct zbar_format_def_t {
	uint32 format; /* fourcc */
	zbar_format_group_t group; /* coarse categorization */
	union {
		uint8 gen[4]; /* raw bytes */
		struct {
			uint8 bpp; /* bits per pixel */
			uint8 red;
			uint8 green;
			uint8 blue; /* size/location a la RGB_BITS() */
		} rgb;
		struct {
			uint8 xsub2;
			uint8 ysub2; // chroma subsampling in each axis
			uint8 packorder; // channel ordering flags bit0: 0=UV, 1=VU; bit1: 0=Y/chroma, 1=chroma/Y
		} yuv;
		uint32 cmp; // quick compare equivalent formats
	} p;
};

extern int _zbar_best_format(uint32, uint32 *, const uint32 *);
extern const zbar_format_def_t * _zbar_format_lookup(uint32);
extern void _zbar_image_free(zbar_image_t*);

#ifdef DEBUG_SVG
	extern int zbar_image_write_png(const zbar_image_t*, const char*);
#else
	#define zbar_image_write_png(img, filename)
#endif

void FASTCALL _zbar_image_refcnt(zbar_image_t * img, int delta);
void FASTCALL _zbar_image_swap_symbols(zbar_image_t * a, zbar_image_t * b);
void FASTCALL _zbar_image_copy_size(zbar_image_t * dst, const zbar_image_t * src);

//struct zbar_image_s;

/** opaque image object. */
//typedef struct zbar_image_s zbar_image_t;

/** data handler callback function.
 * called when decoded symbol results are available for an image
 */
typedef void (zbar_image_data_handler_t)(zbar_image_t * image, const void * userdata);

/** new image constructor.
 * @returns a new image object with uninitialized data and format.
 * this image should be destroyed (using zbar_image_destroy()) as
 * soon as the application is finished with it
 */
extern zbar_image_t * zbar_image_create(void);

/** image destructor.  all images created by or returned to the
 * application should be destroyed using this function.  when an image
 * is destroyed, the associated data cleanup handler will be invoked
 * if available
 * @note make no assumptions about the image or the data buffer.
 * they may not be destroyed/cleaned immediately if the library
 * is still using them.  if necessary, use the cleanup handler hook
 * to keep track of image data buffers
 */
extern void zbar_image_destroy(zbar_image_t * image);

/** image reference count manipulation.
 * increment the reference count when you store a new reference to the
 * image.  decrement when the reference is no longer used.  do not
 * refer to the image any longer once the count is decremented.
 * zbar_image_ref(image, -1) is the same as zbar_image_destroy(image)
 * @since 0.5
 */
extern void zbar_image_ref(zbar_image_t * image, int refs);

/** image format conversion.  refer to the documentation for supported
 * image formats
 * @returns a @em new image with the sample data from the original image
 * converted to the requested format.  the original image is
 * unaffected.
 * @note the converted image size may be rounded (up) due to format
 * constraints
 */
extern zbar_image_t * zbar_image_convert(const zbar_image_t * image, ulong format);
/** image format conversion with crop/pad.
 * if the requested size is larger than the image, the last row/column
 * are duplicated to cover the difference.  if the requested size is
 * smaller than the image, the extra rows/columns are dropped from the
 * right/bottom.
 * @returns a @em new image with the sample data from the original
 * image converted to the requested format and size.
 * @note the image is @em not scaled
 * @see zbar_image_convert()
 * @since 0.4
 */
extern zbar_image_t * zbar_image_convert_resize(const zbar_image_t * image, ulong format, uint width, uint height);
/** retrieve the image format.
 * @returns the fourcc describing the format of the image sample data
 */
extern ulong zbar_image_get_format(const zbar_image_t * image);
/** retrieve a "sequence" (page/frame) number associated with this image.
 * @since 0.6
 */
extern uint zbar_image_get_sequence(const zbar_image_t * image);
/** retrieve the width of the image.
 * @returns the width in sample columns
 */
extern uint zbar_image_get_width(const zbar_image_t * image);

/** retrieve the height of the image.
 * @returns the height in sample rows
 */
extern uint zbar_image_get_height(const zbar_image_t * image);

/** retrieve both dimensions of the image.
 * fills in the width and height in samples
 */
extern void zbar_image_get_size(const zbar_image_t * image, uint * width, uint * height);
/** retrieve the crop rectangle.
 * fills in the image coordinates of the upper left corner and size
 * of an axis-aligned rectangular area of the image that will be scanned.
 * defaults to the full image
 * @since 0.11
 */
extern void zbar_image_get_crop(const zbar_image_t * image, uint * x, uint * y, uint * width, uint * height);
/** return the image sample data.  the returned data buffer is only
 * valid until zbar_image_destroy() is called
 */
extern const void * zbar_image_get_data(const zbar_image_t * image);
/** return the size of image data.
 * @since 0.6
 */
extern ulong zbar_image_get_data_length(const zbar_image_t * img);

/** retrieve the decoded results.
 * @returns the (possibly empty) set of decoded symbols
 * @returns NULL if the image has not been scanned
 * @since 0.10
 */
extern const zbar_symbol_set_t* zbar_image_get_symbols(const zbar_image_t * image);

/** associate the specified symbol set with the image, replacing any
 * existing results.  use NULL to release the current results from the
 * image.
 * @see zbar_image_scanner_recycle_image()
 * @since 0.10
 */
extern void zbar_image_set_symbols(zbar_image_t * image, /*const*/zbar_symbol_set_t * symbols);

/** image_scanner decode result iterator.
 * @returns the first decoded symbol result for an image
 * or NULL if no results are available
 */
extern const zbar_symbol_t* zbar_image_first_symbol(const zbar_image_t * image);

/** specify the fourcc image format code for image sample data.
 * refer to the documentation for supported formats.
 * @note this does not convert the data!
 * (see zbar_image_convert() for that)
 */
extern void zbar_image_set_format(zbar_image_t * image, ulong format);
/** associate a "sequence" (page/frame) number with this image.
 * @since 0.6
 */
extern void zbar_image_set_sequence(zbar_image_t * image, uint sequence_num);

/** specify the pixel size of the image.
 * @note this also resets the crop rectangle to the full image
 * (0, 0, width, height)
 * @note this does not affect the data!
 */
extern void zbar_image_set_size(zbar_image_t * image, uint width, uint height);

/** specify a rectangular region of the image to scan.
 * the rectangle will be clipped to the image boundaries.
 * defaults to the full image specified by zbar_image_set_size()
 */
extern void zbar_image_set_crop(zbar_image_t * image, uint x, uint y, uint width, uint height);

/** specify image sample data.  when image data is no longer needed by
 * the library the specific data cleanup handler will be called
 * (unless NULL)
 * @note application image data will not be modified by the library
 */
extern void zbar_image_set_data(zbar_image_t * image, const void * data, ulong data_byte_length, zbar_image_cleanup_handler_t cleanup_hndlr);
/** built-in cleanup handler.
 * passes the image data buffer to SAlloc::F()
 */
extern void zbar_image_free_data(zbar_image_t * image);

/** associate user specified data value with an image.
 * @since 0.5
 */
extern void zbar_image_set_userdata(zbar_image_t * image, void * userdata);

/** return user specified data value associated with the image.
 * @since 0.5
 */
extern void * zbar_image_get_userdata(const zbar_image_t * image);

/** dump raw image data to a file for debug.
 * the data will be prefixed with a 16 byte header consisting of:
 * - 4 bytes uint = 0x676d697a ("zimg")
 * - 4 bytes format fourcc
 * - 2 bytes width
 * - 2 bytes height
 * - 4 bytes size of following image data in bytes
 * this header can be dumped w/eg:
 * @verbatim
       od -Ax -tx1z -N16 -w4 [file]
   @endverbatim
 * for some formats the image can be displayed/converted using
 * ImageMagick, eg:
 * @verbatim
       display -size 640x480+16 [-depth ?] [-sampling-factor ?x?] \
           {GRAY,RGB,UYVY,YUV}:[file]
   @endverbatim
 *
 * @param image the image object to dump
 * @param filebase base filename, appended with ".XXXX.zimg" where
 * XXXX is the format fourcc
 * @returns 0 on success or a system error code on failure
 */
extern int zbar_image_write(const zbar_image_t * image, const char * filebase);
/** read back an image in the format written by zbar_image_write()
 * @note TBD
 */
extern zbar_image_t * zbar_image_read(char * filename);

/*@}*/

/*------------------------------------------------------------*/
/** @name Processor interface
 * @anchor c-processor
 * high-level self-contained image processor.
 * processes video and images for barcodes, optionally displaying
 * images to a library owned output window
 */
/*@{*/

struct zbar_processor_s;

/** opaque standalone processor object. */
typedef struct zbar_processor_s zbar_processor_t;

/** constructor.
 * if threaded is set and threading is available the processor
 * will spawn threads where appropriate to avoid blocking and
 * improve responsiveness
 */
extern zbar_processor_t * zbar_processor_create(int threaded);

/** destructor.  cleans up all resources associated with the processor
 */
extern void zbar_processor_destroy(zbar_processor_t * processor);

/** (re)initialization.
 * opens a video input device and/or prepares to display output
 */
extern int zbar_processor_init(zbar_processor_t * processor, const char * video_device, int enable_display);

/** request a preferred size for the video image from the device.
 * the request may be adjusted or completely ignored by the driver.
 * @note must be called before zbar_processor_init()
 * @since 0.6
 */
extern int zbar_processor_request_size(zbar_processor_t * processor, uint width, uint height);
/** request a preferred video driver interface version for
 * debug/testing.
 * @note must be called before zbar_processor_init()
 * @since 0.6
 */
extern int zbar_processor_request_interface(zbar_processor_t * processor, int version);

/** request a preferred video I/O mode for debug/testing.  You will
 * get errors if the driver does not support the specified mode.
 * @verbatim
    0 = auto-detect
    1 = force I/O using read()
    2 = force memory mapped I/O using mmap()
    3 = force USERPTR I/O (v4l2 only)
   @endverbatim
 * @note must be called before zbar_processor_init()
 * @since 0.7
 */
extern int zbar_processor_request_iomode(zbar_processor_t * video, int iomode);

/** force specific input and output formats for debug/testing.
 * @note must be called before zbar_processor_init()
 */
extern int zbar_processor_force_format(zbar_processor_t * processor, ulong input_format, ulong output_format);

/** setup result handler callback.
 * the specified function will be called by the processor whenever
 * new results are available from the video stream or a static image.
 * pass a NULL value to disable callbacks.
 * @param processor the object on which to set the handler.
 * @param handler the function to call when new results are available.
 * @param userdata is set as with zbar_processor_set_userdata().
 * @returns the previously registered handler
 */
extern zbar_image_data_handler_t* zbar_processor_set_data_handler(zbar_processor_t * processor, zbar_image_data_handler_t * handler, const void * userdata);
/** associate user specified data value with the processor.
 * @since 0.6
 */
extern void zbar_processor_set_userdata(zbar_processor_t * processor, void * userdata);
/** return user specified data value associated with the processor.
 * @since 0.6
 */
extern void * zbar_processor_get_userdata(const zbar_processor_t * processor);
/** set config for indicated symbology (0 for all) to specified value.
 * @returns 0 for success, non-0 for failure (config does not apply to
 * specified symbology, or value out of range)
 * @see zbar_decoder_set_config()
 * @since 0.4
 */
extern int zbar_processor_set_config(zbar_processor_t * processor, zbar_symbol_type_t symbology, zbar_config_t config, int value);
/** parse configuration string using zbar_parse_config()
 * and apply to processor using zbar_processor_set_config().
 * @returns 0 for success, non-0 for failure
 * @see zbar_parse_config()
 * @see zbar_processor_set_config()
 * @since 0.4
 */
static inline int zbar_processor_parse_config(zbar_processor_t * processor, const char * config_string)
{
	zbar_symbol_type_t sym;
	zbar_config_t cfg;
	int val;
	return (zbar_parse_config(config_string, &sym, &cfg, &val) || zbar_processor_set_config(processor, sym, cfg, val));
}

/** retrieve the current state of the ouput window.
 * @returns 1 if the output window is currently displayed, 0 if not.
 * @returns -1 if an error occurs
 */
extern int zbar_processor_is_visible(zbar_processor_t * processor);

/** show or hide the display window owned by the library.
 * the size will be adjusted to the input size
 */
extern int zbar_processor_set_visible(zbar_processor_t * processor, int visible);

/** control the processor in free running video mode.
 * only works if video input is initialized. if threading is in use,
 * scanning will occur in the background, otherwise this is only
 * useful wrapping calls to zbar_processor_user_wait(). if the
 * library output window is visible, video display will be enabled.
 */
extern int zbar_processor_set_active(zbar_processor_t * processor, int active);
/** retrieve decode results for last scanned image/frame.
 * @returns the symbol set result container or NULL if no results are
 * available
 * @note the returned symbol set has its reference count incremented;
 * ensure that the count is decremented after use
 * @since 0.10
 */
extern const zbar_symbol_set_t* zbar_processor_get_results(/*const*/zbar_processor_t * processor);
/** wait for input to the display window from the user
 * (via mouse or keyboard).
 * @returns >0 when input is received, 0 if timeout ms expired
 * with no input or -1 in case of an error
 */
extern int zbar_processor_user_wait(zbar_processor_t * processor, int timeout);
/** process from the video stream until a result is available,
 * or the timeout (in milliseconds) expires.
 * specify a timeout of -1 to scan indefinitely
 * (zbar_processor_set_active() may still be used to abort the scan
 * from another thread).
 * if the library window is visible, video display will be enabled.
 * @note that multiple results may still be returned (despite the
 * name).
 * @returns >0 if symbols were successfully decoded,
 * 0 if no symbols were found (ie, the timeout expired)
 * or -1 if an error occurs
 */
extern int zbar_process_one(zbar_processor_t * processor, int timeout);
/** process the provided image for barcodes.
 * if the library window is visible, the image will be displayed.
 * @returns >0 if symbols were successfully decoded,
 * 0 if no symbols were found or -1 if an error occurs
 */
extern int zbar_process_image(zbar_processor_t * processor, zbar_image_t * image);
/** display detail for last processor error to stderr.
 * @returns a non-zero value suitable for passing to exit()
 */
static inline int zbar_processor_error_spew(const zbar_processor_t * processor, int verbosity)
{
	return (_zbar_error_spew(processor, verbosity));
}

/** retrieve the detail string for the last processor error. */
static inline const char* zbar_processor_error_string(const zbar_processor_t * processor, int verbosity)
{
	return (_zbar_error_string(processor, verbosity));
}

/** retrieve the type code for the last processor error. */
static inline zbar_error_t zbar_processor_get_error_code(const zbar_processor_t * processor)
{
	return (_zbar_get_error_code(processor));
}

/*@}*/

/*------------------------------------------------------------*/
/** @name Video interface
 * @anchor c-video
 * mid-level video source abstraction.
 * captures images from a video device
 */
/*@{*/

//struct zbar_video_s;

/** opaque video object. */
//typedef struct zbar_video_s zbar_video_t;

extern zbar_video_t * zbar_video_create(void);
extern void zbar_video_destroy(zbar_video_t * video);

/** open and probe a video device.
 * the device specified by platform specific unique name
 * (v4l device node path in *nix eg "/dev/video",
 *  DirectShow DevicePath property in windows).
 * @returns 0 if successful or -1 if an error occurs
 */
extern int zbar_video_open(zbar_video_t * video, const char * device);

/** retrieve file descriptor associated with open *nix video device
 * useful for using select()/poll() to tell when new images are
 * available (NB v4l2 only!!).
 * @returns the file descriptor or -1 if the video device is not open
 * or the driver only supports v4l1
 */
extern int zbar_video_get_fd(const zbar_video_t * video);

/** request a preferred size for the video image from the device.
 * the request may be adjusted or completely ignored by the driver.
 * @returns 0 if successful or -1 if the video device is already
 * initialized
 * @since 0.6
 */
extern int zbar_video_request_size(zbar_video_t * video, uint width, uint height);

/** request a preferred driver interface version for debug/testing.
 * @note must be called before zbar_video_open()
 * @since 0.6
 */
extern int zbar_video_request_interface(zbar_video_t * video, int version);

/** request a preferred I/O mode for debug/testing.  You will get
 * errors if the driver does not support the specified mode.
 * @verbatim
    0 = auto-detect
    1 = force I/O using read()
    2 = force memory mapped I/O using mmap()
    3 = force USERPTR I/O (v4l2 only)
   @endverbatim
 * @note must be called before zbar_video_open()
 * @since 0.7
 */
extern int zbar_video_request_iomode(zbar_video_t * video, int iomode);

/** retrieve current output image width.
 * @returns the width or 0 if the video device is not open
 */
extern int zbar_video_get_width(const zbar_video_t * video);

/** retrieve current output image height.
 * @returns the height or 0 if the video device is not open
 */
extern int zbar_video_get_height(const zbar_video_t * video);

/** initialize video using a specific format for debug.
 * use zbar_negotiate_format() to automatically select and initialize
 * the best available format
 */
extern int zbar_video_init(zbar_video_t * video, ulong format);

/** start/stop video capture.
 * all buffered images are retired when capture is disabled.
 * @returns 0 if successful or -1 if an error occurs
 */
extern int zbar_video_enable(zbar_video_t * video, int enable);

/** retrieve next captured image.  blocks until an image is available.
 * @returns NULL if video is not enabled or an error occurs
 */
extern zbar_image_t * zbar_video_next_image(zbar_video_t * video);

/** display detail for last video error to stderr.
 * @returns a non-zero value suitable for passing to exit()
 */
static inline int zbar_video_error_spew(const zbar_video_t * video,
    int verbosity)
{
	return (_zbar_error_spew(video, verbosity));
}

/** retrieve the detail string for the last video error. */
static inline const char * zbar_video_error_string(const zbar_video_t * video,
    int verbosity)
{
	return (_zbar_error_string(video, verbosity));
}

/** retrieve the type code for the last video error. */
static inline zbar_error_t zbar_video_get_error_code(const zbar_video_t * video)
{
	return (_zbar_get_error_code(video));
}

/*@}*/

/*------------------------------------------------------------*/
/** @name Window interface
 * @anchor c-window
 * mid-level output window abstraction.
 * displays images to user-specified platform specific output window
 */
/*@{*/

struct zbar_window_s;

/** opaque window object. */
typedef struct zbar_window_s zbar_window_t;

extern zbar_window_t * zbar_window_create(void);
extern void zbar_window_destroy(zbar_window_t * window);

/** associate reader with an existing platform window.
 * This can be any "Drawable" for X Windows or a "HWND" for windows.
 * input images will be scaled into the output window.
 * pass NULL to detach from the resource, further input will be
 * ignored
 */
extern int zbar_window_attach(zbar_window_t * window, void * x11_display_w32_hwnd, ulong x11_drawable);

/** control content level of the reader overlay.
 * the overlay displays graphical data for informational or debug
 * purposes.  higher values increase the level of annotation (possibly
 * decreasing performance). @verbatim
    0 = disable overlay
    1 = outline decoded symbols (default)
    2 = also track and display input frame rate
   @endverbatim
 */
extern void zbar_window_set_overlay(zbar_window_t * window, int level);

/** retrieve current content level of reader overlay.
 * @see zbar_window_set_overlay()
 * @since 0.10
 */
extern int zbar_window_get_overlay(const zbar_window_t * window);

/** draw a new image into the output window. */
extern int zbar_window_draw(zbar_window_t * window, zbar_image_t * image);

/** redraw the last image (exposure handler). */
extern int zbar_window_redraw(zbar_window_t * window);

/** resize the image window (reconfigure handler).
 * this does @em not update the contents of the window
 * @since 0.3, changed in 0.4 to not redraw window
 */
extern int zbar_window_resize(zbar_window_t * window, uint width, uint height);

/** display detail for last window error to stderr.
 * @returns a non-zero value suitable for passing to exit()
 */
static inline int zbar_window_error_spew(const zbar_window_t * window, int verbosity)
{
	return (_zbar_error_spew(window, verbosity));
}

/** retrieve the detail string for the last window error. */
static inline const char* zbar_window_error_string(const zbar_window_t * window, int verbosity)
{
	return (_zbar_error_string(window, verbosity));
}

/** retrieve the type code for the last window error. */
static inline zbar_error_t zbar_window_get_error_code(const zbar_window_t * window)
{
	return (_zbar_get_error_code(window));
}

/** select a compatible format between video input and output window.
 * the selection algorithm attempts to use a format shared by
 * video input and window output which is also most useful for
 * barcode scanning.  if a format conversion is necessary, it will
 * heuristically attempt to minimize the cost of the conversion
 */
extern int zbar_negotiate_format(zbar_video_t * video, zbar_window_t * window);

/*@}*/

/*------------------------------------------------------------*/
/** @name Image Scanner interface
 * @anchor c-imagescanner
 * mid-level image scanner interface.
 * reads barcodes from 2-D images
 */
/*@{*/

struct zbar_image_scanner_s;

/** opaque image scanner object. */
typedef struct zbar_image_scanner_s zbar_image_scanner_t;

extern zbar_image_scanner_t * zbar_image_scanner_create(void);
extern void zbar_image_scanner_destroy(zbar_image_scanner_t * scanner);

/** setup result handler callback.
 * the specified function will be called by the scanner whenever
 * new results are available from a decoded image.
 * pass a NULL value to disable callbacks.
 * @returns the previously registered handler
 */
extern zbar_image_data_handler_t* zbar_image_scanner_set_data_handler(zbar_image_scanner_t * scanner, zbar_image_data_handler_t * handler, const void * userdata);

/** set config for indicated symbology (0 for all) to specified value.
 * @returns 0 for success, non-0 for failure (config does not apply to
 * specified symbology, or value out of range)
 * @see zbar_decoder_set_config()
 * @since 0.4
 */
extern int zbar_image_scanner_set_config(zbar_image_scanner_t * scanner,
    zbar_symbol_type_t symbology,
    zbar_config_t config,
    int value);

/** parse configuration string using zbar_parse_config()
 * and apply to image scanner using zbar_image_scanner_set_config().
 * @returns 0 for success, non-0 for failure
 * @see zbar_parse_config()
 * @see zbar_image_scanner_set_config()
 * @since 0.4
 */
static inline int zbar_image_scanner_parse_config(zbar_image_scanner_t * scanner,
    const char * config_string)
{
	zbar_symbol_type_t sym;
	zbar_config_t cfg;
	int val;
	return (zbar_parse_config(config_string, &sym, &cfg, &val) ||
	    zbar_image_scanner_set_config(scanner, sym, cfg, val));
}

/** enable or disable the inter-image result cache (default disabled).
 * mostly useful for scanning video frames, the cache filters
 * duplicate results from consecutive images, while adding some
 * consistency checking and hysteresis to the results.
 * this interface also clears the cache
 */
extern void zbar_image_scanner_enable_cache(zbar_image_scanner_t * scanner, int enable);

/** remove any previously decoded results from the image scanner and the
 * specified image.  somewhat more efficient version of
 * zbar_image_set_symbols(image, NULL) which may retain memory for
 * subsequent decodes
 * @since 0.10
 */
extern void FASTCALL zbar_image_scanner_recycle_image(zbar_image_scanner_t * scanner, zbar_image_t * image);

/** retrieve decode results for last scanned image.
 * @returns the symbol set result container or NULL if no results are
 * available
 * @note the symbol set does not have its reference count adjusted;
 * ensure that the count is incremented if the results may be kept
 * after the next image is scanned
 * @since 0.10
 */
extern /*const*/zbar_symbol_set_t * zbar_image_scanner_get_results(/*const*/zbar_image_scanner_t * scanner);

/** scan for symbols in provided image.  The image format must be
 * "Y800" or "GRAY".
 * @returns >0 if symbols were successfully decoded from the image,
 * 0 if no symbols were found or -1 if an error occurs
 * @see zbar_image_convert()
 * @since 0.9 - changed to only accept grayscale images
 */
extern int zbar_scan_image(zbar_image_scanner_t * scanner, zbar_image_t * image);

/*@}*/

/*------------------------------------------------------------*/
/** @name Decoder interface
 * @anchor c-decoder
 * low-level bar width stream decoder interface.
 * identifies symbols and extracts encoded data
 */
/*@{*/

#define NUM_CFGS (ZBAR_CFG_MAX_LEN - ZBAR_CFG_MIN_LEN + 1)
//
// Codabar specific decode state
//
struct codabar_decoder_t {
    uint direction : 1; /* scan direction: 0=fwd, 1=rev */
    uint element : 4; /* element offset 0-7 */
    int character : 12; /* character position in symbol */
    uint s7; /* current character width */
    uint width; /* last character width */
    uchar buf[6]; /* initial scan buffer */

    uint config;
    int configs[NUM_CFGS]; /* int valued configurations */
};
//
// state of each parallel decode attempt
//
typedef struct ean_pass_s {
    int8 state; /* module position of w[idx] in symbol */
#define STATE_REV   0x80        /* scan direction reversed */
#define STATE_ADDON 0x40        /* scanning add-on */
#define STATE_IDX   0x3f        /* element offset into symbol */
    uint width; /* width of last character */
    uchar raw[7]; /* decode in process */
} ean_pass_t;
//
// EAN/UPC specific decode state
//
struct ean_decoder_t {
    ean_pass_t pass[4]; /* state of each parallel decode attempt */
    zbar_symbol_type_t left; /* current holding buffer contents */
    zbar_symbol_type_t right;
    int direction; /* scan direction */
    uint s4, width; /* character width */
    int8 buf[18]; /* holding buffer */

    int8 enable;
    uint ean13_config;
    uint ean8_config;
    uint upca_config;
    uint upce_config;
    uint isbn10_config;
    uint isbn13_config;
    uint ean5_config;
    uint ean2_config;
};
//
// Code 128 specific decode state
//
struct code128_decoder_t {
    uint direction : 1; /* scan direction: 0=fwd/space, 1=rev/bar */
    uint element : 3; /* element offset 0-5 */
    int character : 12; /* character position in symbol */
    uchar start; /* start character */
    uint s6; /* character width */
    uint width; /* last character width */

    uint config;
    int configs[NUM_CFGS]; /* int valued configurations */
};
//
// Code 39 specific decode state
//
typedef struct code39_decoder_s {
	uint direction : 1; /* scan direction: 0=fwd, 1=rev */
	uint element : 4; /* element offset 0-8 */
	int character : 12; /* character position in symbol */
	uint s9; /* current character width */
	uint width; /* last character width */

	uint config;
	int configs[NUM_CFGS]; /* int valued configurations */
} code39_decoder_t;
//
// Code 93 specific decode state
//
typedef struct code93_decoder_s {
	uint direction : 1; /* scan direction: 0=fwd/space, 1=rev/bar */
	uint element : 3; /* element offset 0-5 */
	int character : 12; /* character position in symbol */
	uint width; /* last character width */
	uchar buf; /* first character */

	uint config;
	int configs[NUM_CFGS]; /* int valued configurations */
} code93_decoder_t;
#define DATABAR_MAX_SEGMENTS 32
//
// active DataBar (partial) segment entry
//
typedef struct databar_segment_s {
	signed finder : 5; /* finder pattern */
	uint exp : 1; /* DataBar expanded finder */
	uint color : 1; /* finder coloring */
	uint side : 1; /* data character side of finder */

	uint partial : 1; /* unpaired partial segment */
	uint count : 7; /* times encountered */
	uint epoch : 8; /* age, in characters scanned */
	uint check : 8; /* bar checksum */
	signed short data; /* decoded character data */
	ushort width; /* measured width of finder (14 modules) */
} databar_segment_t;
//
// DataBar specific decode state
//
typedef struct databar_decoder_s {
	uint config; /* decoder configuration flags */
	uint config_exp;

	uint csegs : 8; /* allocated segments */
	uint epoch : 8; /* current scan */

	databar_segment_t * segs; /* active segment list */
	int8 chars[16]; /* outstanding character indices */
} databar_decoder_t;
//
// interleaved 2 of 5 specific decode state
//
typedef struct i25_decoder_s {
	uint direction : 1; /* scan direction: 0=fwd/space, 1=rev/bar */
	uint element : 4; /* element offset 0-8 */
	int character : 12; /* character position in symbol */
	uint s10; /* current character width */
	uint width; /* last character width */
	uchar buf[4]; /* initial scan buffer */

	uint config;
	int configs[NUM_CFGS]; /* int valued configurations */
} i25_decoder_t;
//
// PDF417 specific decode state
//
typedef struct pdf417_decoder_s {
	uint direction : 1; /* scan direction: 0=fwd/space, 1=rev/bar */
	uint element : 3; /* element offset 0-7 */
	int character : 12; /* character position in symbol */
	uint s8; /* character width */

	uint config;
	int configs[NUM_CFGS]; /* int valued configurations */
} pdf417_decoder_t;
//
//
//
typedef struct qr_reader qr_reader;
typedef int qr_point[2];
typedef struct qr_finder_line qr_finder_line;

/*The number of bits of subpel precision to store image coordinates in.
   This helps when estimating positions in low-resolution images, which may have
   a module pitch only a pixel or two wide, making rounding errors matter a
   great deal.*/
#define QR_FINDER_SUBPREC (2)

/*A line crossing a finder pattern.
   Whether the line is horizontal or vertical is determined by context.
   The offsts to various parts of the finder pattern are as follows:
 |*****|     |*****|*****|*****|     |*****|
 |*****|     |*****|*****|*****|     |*****|
       ^        ^                 ^        ^
 |        |                 |        |
 |        |                 |       pos[v]+len+eoffs
 |        |                pos[v]+len
 |       pos[v]
      pos[v]-boffs
   Here v is 0 for horizontal and 1 for vertical lines.*/
struct qr_finder_line {
	/*The location of the upper/left endpoint of the line.
	   The left/upper edge of the center section is used, since other lines must
	   cross in this region.*/
	qr_point pos;
	/*The length of the center section.
	   This extends to the right/bottom of the center section, since other lines
	   must cross in this region.*/
	int len;
	/*The offset to the midpoint of the upper/left section (part of the outside
	   ring), or 0 if we couldn't identify the edge of the beginning section.
	   We use the midpoint instead of the edge because it can be located more
	   reliably.*/
	int boffs;
	/*The offset to the midpoint of the end section (part of the outside ring),
	   or 0 if we couldn't identify the edge of the end section.
	   We use the midpoint instead of the edge because it can be located more
	   reliably.*/
	int eoffs;
};

qr_reader * _zbar_qr_create(void);
void _zbar_qr_destroy(qr_reader * reader);
void _zbar_qr_reset(qr_reader * reader);
int _zbar_qr_found_line(qr_reader * reader, int direction, const qr_finder_line * line);
int _zbar_qr_decode(qr_reader * reader, zbar_image_scanner_t * iscn, const zbar_image_t * img);
//
// QR Code symbol finder state
//
typedef struct qr_finder_s {
	uint s5; /* finder pattern width */
	qr_finder_line line; /* position info needed by decoder */
	uint config;
} qr_finder_t;
//
// decoder data handler callback function.
// called by decoder when new data has just been decoded
//
typedef void (*zbar_decoder_handler_t)(zbar_decoder_t * decoder);
//
// symbology independent decoder state
//
struct zbar_decoder_t {
    uchar  idx; /* current width index */
    uint   w[DECODE_WINDOW]; /* window of last N bar widths */
    zbar_symbol_type_t type; /* type of last decoded data */
    zbar_symbol_type_t lock; /* buffer lock */
    uint   modifiers; /* symbology modifier */
    int    direction; /* direction of last decoded data */
    uint   s6; /* 6-element character width */
    /* everything above here is automatically reset */
    uint   buf_alloc; /* dynamic buffer allocation */
    uint   buflen;                    /* binary data length */
    uchar * buf; /* decoded characters */
    void * userdata; /* application data */
    zbar_decoder_handler_t handler; /* application callback */
    /* symbology specific state */
#ifdef ENABLE_EAN
    ean_decoder_t ean; /* EAN/UPC parallel decode attempts */
#endif
#ifdef ENABLE_I25
    i25_decoder_t i25; /* Interleaved 2 of 5 decode state */
#endif
#ifdef ENABLE_DATABAR
    databar_decoder_t databar; /* DataBar decode state */
#endif
#ifdef ENABLE_CODABAR
    codabar_decoder_t codabar; /* Codabar decode state */
#endif
#ifdef ENABLE_CODE39
    code39_decoder_t code39; /* Code 39 decode state */
#endif
#ifdef ENABLE_CODE93
    code93_decoder_t code93; /* Code 93 decode state */
#endif
#ifdef ENABLE_CODE128
    code128_decoder_t code128; /* Code 128 decode state */
#endif
#ifdef ENABLE_PDF417
    pdf417_decoder_t pdf417; /* PDF417 decode state */
#endif
#ifdef ENABLE_QRCODE
    qr_finder_t qrf;                    /* QR Code finder state */
#endif
};
//
// reset QR finder specific state
//
void qr_finder_reset(qr_finder_t * qrf);
//
// find QR Code symbols
//
zbar_symbol_type_t _zbar_find_qr(zbar_decoder_t * dcode);
//
// reset PDF417 specific state
//
void pdf417_reset(pdf417_decoder_t * pdf417);
//
// decode PDF417 symbols
//
zbar_symbol_type_t _zbar_decode_pdf417(zbar_decoder_t * dcode);
//
// reset interleaved 2 of 5 specific state
//
void i25_reset(i25_decoder_t * i25);
//
// decode interleaved 2 of 5 symbols
//
zbar_symbol_type_t _zbar_decode_i25(zbar_decoder_t * dcode);
//
// reset DataBar segment decode state
//
void databar_new_scan(databar_decoder_t * db);
//
// reset DataBar accumulated segments
//
void databar_reset(databar_decoder_t * db);
//
// decode DataBar symbols
//
zbar_symbol_type_t _zbar_decode_databar(zbar_decoder_t * dcode);
//
// reset Code 93 specific state
//
void code93_reset(code93_decoder_t * dcode93);
//
// decode Code 93 symbols
//
zbar_symbol_type_t _zbar_decode_code93(zbar_decoder_t * dcode);
//
// reset Code 39 specific state
//
void code39_reset(code39_decoder_t * dcode39);
//
// decode Code 39 symbols
//
zbar_symbol_type_t _zbar_decode_code39(zbar_decoder_t * dcode);
//
// reset Code 128 specific state
//
void code128_reset(code128_decoder_t *dcode128);
//
// decode Code 128 symbols
//
zbar_symbol_type_t _zbar_decode_code128(zbar_decoder_t *dcode);
//
// reset EAN/UPC pass specific state
//
void ean_new_scan(ean_decoder_t * ean);
//
// reset all EAN/UPC state
//
void ean_reset(ean_decoder_t * ean);
uint ean_get_config(ean_decoder_t * ean, zbar_symbol_type_t sym);
// decode EAN/UPC symbols
zbar_symbol_type_t _zbar_decode_ean(zbar_decoder_t * dcode);
//
// reset Codabar specific state
//
void codabar_reset(codabar_decoder_t *codabar);
//
// decode Codabar symbols
//
zbar_symbol_type_t _zbar_decode_codabar(zbar_decoder_t *dcode);

//struct zbar_decoder_s;
/** opaque decoder object. */
//typedef struct zbar_decoder_s zbar_decoder_t;

extern zbar_decoder_t * zbar_decoder_create(void);
extern void zbar_decoder_destroy(zbar_decoder_t * decoder);

/** set config for indicated symbology (0 for all) to specified value.
 * @returns 0 for success, non-0 for failure (config does not apply to
 * specified symbology, or value out of range)
 * @since 0.4
 */
extern int zbar_decoder_set_config(zbar_decoder_t * decoder, zbar_symbol_type_t symbology, zbar_config_t config, int value);

/** parse configuration string using zbar_parse_config()
 * and apply to decoder using zbar_decoder_set_config().
 * @returns 0 for success, non-0 for failure
 * @see zbar_parse_config()
 * @see zbar_decoder_set_config()
 * @since 0.4
 */
static inline int zbar_decoder_parse_config(zbar_decoder_t * decoder, const char * config_string)
{
	zbar_symbol_type_t sym;
	zbar_config_t cfg;
	int val;
	return (zbar_parse_config(config_string, &sym, &cfg, &val) || zbar_decoder_set_config(decoder, sym, cfg, val));
}

/** retrieve symbology boolean config settings.
 * @returns a bitmask indicating which configs are currently set for the
 * specified symbology.
 * @since 0.11
 */
extern uint zbar_decoder_get_configs(const zbar_decoder_t * decoder,
    zbar_symbol_type_t symbology);

/** clear all decoder state.
 * any partial symbols are flushed
 */
extern void zbar_decoder_reset(zbar_decoder_t * decoder);

/** mark start of a new scan pass.
 * clears any intra-symbol state and resets color to ::ZBAR_SPACE.
 * any partially decoded symbol state is retained
 */
extern void zbar_decoder_new_scan(zbar_decoder_t * decoder);

/** process next bar/space width from input stream.
 * the width is in arbitrary relative units.  first value of a scan
 * is ::ZBAR_SPACE width, alternating from there.
 * @returns appropriate symbol type if width completes
 * decode of a symbol (data is available for retrieval)
 * @returns ::ZBAR_PARTIAL as a hint if part of a symbol was decoded
 * @returns ::ZBAR_NONE (0) if no new symbol data is available
 */
extern zbar_symbol_type_t zbar_decode_width(zbar_decoder_t * decoder, uint width);

/** retrieve color of @em next element passed to
 * zbar_decode_width(). */
extern zbar_color_t zbar_decoder_get_color(const zbar_decoder_t * decoder);

/** retrieve last decoded data.
 * @returns the data string or NULL if no new data available.
 * the returned data buffer is owned by library, contents are only
 * valid between non-0 return from zbar_decode_width and next library
 * call
 */
extern const char * zbar_decoder_get_data(const zbar_decoder_t * decoder);

/** retrieve length of binary data.
 * @returns the length of the decoded data or 0 if no new data
 * available.
 */
extern uint zbar_decoder_get_data_length(const zbar_decoder_t * decoder);

/** retrieve last decoded symbol type.
 * @returns the type or ::ZBAR_NONE if no new data available
 */
extern zbar_symbol_type_t FASTCALL zbar_decoder_get_type(const zbar_decoder_t * decoder);

/** retrieve modifier flags for the last decoded symbol.
 * @returns a bitmask indicating which characteristics were detected
 * during decoding.
 * @since 0.11
 */
extern uint zbar_decoder_get_modifiers(const zbar_decoder_t * decoder);

/** retrieve last decode direction.
 * @returns 1 for forward and -1 for reverse
 * @returns 0 if the decode direction is unknown or does not apply
 * @since 0.11
 */
extern int zbar_decoder_get_direction(const zbar_decoder_t * decoder);
/** setup data handler callback.
 * the registered function will be called by the decoder
 * just before zbar_decode_width() returns a non-zero value.
 * pass a NULL value to disable callbacks.
 * @returns the previously registered handler
 */
extern zbar_decoder_handler_t zbar_decoder_set_handler(zbar_decoder_t * decoder, zbar_decoder_handler_t handler);
/** associate user specified data value with the decoder. */
extern void zbar_decoder_set_userdata(zbar_decoder_t * decoder, void * userdata);
/** return user specified data value associated with the decoder. */
extern void * zbar_decoder_get_userdata(const zbar_decoder_t * decoder);

/*@}*/

/*------------------------------------------------------------*/
/** @name Scanner interface
 * @anchor c-scanner
 * low-level linear intensity sample stream scanner interface.
 * identifies "bar" edges and measures width between them.
 * optionally passes to bar width decoder
 */
/*@{*/

struct zbar_scanner_s;

/** opaque scanner object. */
typedef struct zbar_scanner_s zbar_scanner_t;

/** constructor.
 * if decoder is non-NULL it will be attached to scanner
 * and called automatically at each new edge
 * current color is initialized to ::ZBAR_SPACE
 * (so an initial BAR->SPACE transition may be discarded)
 */
extern zbar_scanner_t * zbar_scanner_create(zbar_decoder_t * decoder);
extern void zbar_scanner_destroy(zbar_scanner_t * scanner);

/** clear all scanner state.
 * also resets an associated decoder
 */
extern zbar_symbol_type_t zbar_scanner_reset(zbar_scanner_t * scanner);

/** mark start of a new scan pass. resets color to ::ZBAR_SPACE.
 * also updates an associated decoder.
 * @returns any decode results flushed from the pipeline
 * @note when not using callback handlers, the return value should
 * be checked the same as zbar_scan_y()
 * @note call zbar_scanner_flush() at least twice before calling this
 * method to ensure no decode results are lost
 */
extern zbar_symbol_type_t zbar_scanner_new_scan(zbar_scanner_t * scanner);

/** flush scanner processing pipeline.
 * forces current scanner position to be a scan boundary.
 * call multiple times (max 3) to completely flush decoder.
 * @returns any decode/scan results flushed from the pipeline
 * @note when not using callback handlers, the return value should
 * be checked the same as zbar_scan_y()
 * @since 0.9
 */
extern zbar_symbol_type_t zbar_scanner_flush(zbar_scanner_t * scanner);

/** process next sample intensity value.
 * intensity (y) is in arbitrary relative units.
 * @returns result of zbar_decode_width() if a decoder is attached,
 * otherwise @returns (::ZBAR_PARTIAL) when new edge is detected
 * or 0 (::ZBAR_NONE) if no new edge is detected
 */
extern zbar_symbol_type_t zbar_scan_y(zbar_scanner_t * scanner, int y);

/** process next sample from RGB (or BGR) triple. */
static inline zbar_symbol_type_t zbar_scan_rgb24(zbar_scanner_t * scanner, uchar * rgb)
{
	return (zbar_scan_y(scanner, rgb[0] + rgb[1] + rgb[2]));
}

/** retrieve last scanned width. */
extern uint zbar_scanner_get_width(const zbar_scanner_t * scanner);
/** retrieve sample position of last edge.
 * @since 0.10
 */
extern uint zbar_scanner_get_edge(const zbar_scanner_t * scn, uint offset, int prec);
/** retrieve last scanned color. */
extern zbar_color_t zbar_scanner_get_color(const zbar_scanner_t * scanner);
//
// internal image scanner APIs for 2D readers
//
extern zbar_symbol_t *_zbar_image_scanner_alloc_sym(zbar_image_scanner_t*, zbar_symbol_type_t, int);
extern void _zbar_image_scanner_add_sym(zbar_image_scanner_t*, zbar_symbol_t*);
extern void _zbar_image_scanner_recycle_syms(zbar_image_scanner_t*, zbar_symbol_t*);

/*@}*/

#ifdef __cplusplus
//}
// } // extern "C"
/* @sobolev
	#include "zbar/Exception.h"
	#include "zbar/Decoder.h"
	#include "zbar/Scanner.h"
	#include "zbar/Symbol.h"
	#include "zbar/Image.h"
	#include "zbar/ImageScanner.h"
	#include "zbar/Video.h"
	#include "zbar/Window.h"
	#include "zbar/Processor.h"
*/
#endif
//#include "decoder.h"
#ifdef ENABLE_QRCODE
	//#include "decoder/qr_finder.h"
#endif
#ifndef BUFFER_MIN
	#define BUFFER_MIN   0x20 // initial data buffer allocation
#endif
#ifndef BUFFER_MAX
	#define BUFFER_MAX  0x100 // maximum data buffer allocation (longer symbols are rejected)
#endif
#ifndef BUFFER_INCR
	#define BUFFER_INCR  0x10 // buffer allocation increment 
#endif
#define CFG(dcode, cfg) ((dcode).configs[(cfg) - ZBAR_CFG_MIN_LEN])
#define TEST_CFG(config, cfg) (((config) >> (cfg)) & 1)
#define MOD(mod) (1 << (mod))
//
// return current element color 
//
char FASTCALL get_color(const zbar_decoder_t * dcode);
//
// retrieve i-th previous element width 
//
uint FASTCALL get_width(const zbar_decoder_t * dcode, uchar offset);
//
// retrieve bar+space pair width starting at offset i 
//
uint FASTCALL pair_width(const zbar_decoder_t * dcode, uchar offset);
// 
// calculate total character width "s"
//   - start of character identified by context sensitive offset (<= DECODE_WINDOW - n)
//   - size of character is n elements
// 
uint calc_s(const zbar_decoder_t * dcode, uchar offset, uchar n);
// 
// fixed character width decode assist
// bar+space width are compared as a fraction of the reference dimension "x"
//   - +/- 1/2 x tolerance
//   - measured total character width (s) compared to symbology baseline (n)
//     (n = 7 for EAN/UPC, 11 for Code 128)
//   - bar+space *pair width* "e" is used to factor out bad "exposures" ("blooming" or "swelling" of dark or light areas)
//     => using like-edge measurements avoids these issues
// - n should be > 3
// 
int FASTCALL decode_e(uint e, uint s, uint n);
// 
// sort three like-colored elements and return ordering
// 
uint FASTCALL decode_sort3(const zbar_decoder_t * dcode, int i0);
//
// sort N like-colored elements and return ordering
//
uint FASTCALL decode_sortn(const zbar_decoder_t * dcode, int n, int i0);
//
// acquire shared state lock 
//
char FASTCALL acquire_lock(zbar_decoder_t * dcode, zbar_symbol_type_t req);
//
// check and release shared state lock 
//
char FASTCALL release_lock(zbar_decoder_t * dcode, zbar_symbol_type_t req);
//
// ensure output buffer has sufficient allocation for request 
//
char FASTCALL size_buf(zbar_decoder_t * dcode, uint len);
extern const char * _zbar_decoder_buf_dump(uchar * buf, uint buflen);
//
//#include "error.h"

extern int _zbar_verbosity;

// FIXME don't we need varargs hacks here? 

#ifdef _WIN32
	#define ZFLUSH fflush(stderr);
#else
	#define ZFLUSH
#endif
void cdecl zprintf(int level, const char * pFormat, ...);
int err_copy(void * dst_c, void * src_c);
int err_capture(const void * container, errsev_t sev, zbar_error_t type, const char * func, const char * detail);
int err_capture_str(const void * container, errsev_t sev, zbar_error_t type, const char * func, const char * detail, const char * arg);
int err_capture_int(const void * container, errsev_t sev, zbar_error_t type, const char * func, const char * detail, int arg);
int err_capture_num(const void * container, errsev_t sev, zbar_error_t type, const char * func, const char * detail, int num);
void err_init(errinfo_t * err, errmodule_t module);
void err_cleanup(errinfo_t * err);
//
//#include "qrcode\util.h"
#define QR_MAXI(_a,_b)      ((_a)-((_a)-(_b)&-((_b)>(_a))))
#define QR_MINI(_a,_b)      ((_a)+((_b)-(_a)&-((_b)<(_a))))
#define QR_SIGNI(_x)        (((_x)>0)-((_x)<0))
#define QR_SIGNMASK(_x)     (-((_x)<0))
// Unlike copysign(), simply inverts the sign of _a if _b is negative.
#define QR_FLIPSIGNI(_a,_b) ((_a)+QR_SIGNMASK(_b)^QR_SIGNMASK(_b))
#define QR_COPYSIGNI(_a,_b) QR_FLIPSIGNI(abs(_a),_b)
// Divides a signed integer by a positive value with exact rounding.
#define QR_DIVROUND(_x,_y)  (((_x)+QR_FLIPSIGNI(_y>>1,_x))/(_y))
#define QR_CLAMPI(_a,_b,_c) (QR_MAXI(_a,QR_MINI(_b,_c)))
#define QR_CLAMP255(_x)     ((uchar)((((_x)<0)-1)&((_x)|-((_x)>255))))
#define QR_SWAP2I(_a,_b)   do { int t__=(_a); (_a)=(_b); (_b)=t__; } while(0)
/*Swaps two integers _a and _b if _a>_b.*/
#define QR_SORT2I(_a,_b)   do { int t__=QR_MINI(_a,_b)^(_a); (_a)^=t__; (_b)^=t__; } while(0)
#define QR_ILOG0(_v) (!!((_v)&0x2))
#define QR_ILOG1(_v) (((_v)&0xC)?2+QR_ILOG0((_v)>>2):QR_ILOG0(_v))
#define QR_ILOG2(_v) (((_v)&0xF0)?4+QR_ILOG1((_v)>>4):QR_ILOG1(_v))
#define QR_ILOG3(_v) (((_v)&0xFF00)?8+QR_ILOG2((_v)>>8):QR_ILOG2(_v))
#define QR_ILOG4(_v) (((_v)&0xFFFF0000)?16+QR_ILOG3((_v)>>16):QR_ILOG3(_v))
#define QR_ILOG(_v) ((int)QR_ILOG4((uint)(_v))) // Computes the integer logarithm of a (positive, 32-bit) constant.
// Multiplies 32-bit numbers _a and _b, adds (possibly 64-bit) number _r, and takes bits [_s,_s+31] of the result.
#define QR_FIXMUL(_a,_b,_r,_s) ((int)((_a) * (long long)((_b)+(_r)) >> (_s)))
// Multiplies 32-bit numbers _a and _b, adds (possibly 64-bit) number _r, and gives all 64 bits of the result.
#define QR_EXTMUL(_a,_b,_r)    ((_a)*(long long)(_b)+(_r))

uint   qr_isqrt(uint _val);
uint   qr_ihypot(int _x,int _y);
int    qr_ilog(uint _val);
//
//#include "symbol.h"
#define NUM_SYMS  20

/* @sobolev (replaced with SPoint2I) struct point_t {
	int    x;
	int    y;
};*/

struct zbar_symbol_set_s {
	refcnt_t refcnt;
	int    nsyms;         // number of filtered symbols
	zbar_symbol_t * head; // first of decoded symbol results 
	zbar_symbol_t * tail; // last of unfiltered symbol results 
};

struct zbar_symbol_s {
	zbar_symbol_type_t type;   // symbol type
	uint   configs;            // symbology boolean config bitmask 
	uint   modifiers;          // symbology modifier bitmask 
	uint   data_alloc;         // allocation size of data 
	uint   datalen;            // length of binary symbol data 
	char * P_Data_;            // symbol data 
	uint   pts_alloc;          // allocation size of pts 
	uint   npts;               // number of points in location polygon 
	SPoint2I * pts;            // list of points in location polygon 
	zbar_orientation_t orient; // coarse orientation 
	refcnt_t refcnt;           // reference count 
	zbar_symbol_t * next;      // linked list of results (or siblings) 
	zbar_symbol_set_t * syms;  // components of composite result 
	ulong  time;               // relative symbol capture time 
	int    cache_count;        // cache state 
	int    quality;            // relative symbol reliability metric 
};

extern int _zbar_get_symbol_hash(zbar_symbol_type_t);
extern void _zbar_symbol_free(zbar_symbol_t*);
extern zbar_symbol_set_t * _zbar_symbol_set_create(void);
extern void _zbar_symbol_set_free(zbar_symbol_set_t*);
void FASTCALL sym_add_point(zbar_symbol_t * sym, int x, int y);
/*static inline void sym_add_point(zbar_symbol_t * sym, int x, int y)
{
	int i = sym->npts;
	if(++sym->npts >= sym->pts_alloc)
		sym->pts = static_cast<SPoint2I *>(SAlloc::R(sym->pts, ++sym->pts_alloc * sizeof(SPoint2I)));
	sym->pts[i].x = x;
	sym->pts[i].y = y;
}*/

static inline void _zbar_symbol_refcnt(zbar_symbol_t * sym, int delta)
{
	if(!_zbar_refcnt(&sym->refcnt, delta) && delta <= 0)
		_zbar_symbol_free(sym);
}

static inline void _zbar_symbol_set_add(zbar_symbol_set_t * syms, zbar_symbol_t * sym)
{
	sym->next = syms->head;
	syms->head = sym;
	syms->nsyms++;
	_zbar_symbol_refcnt(sym, 1);
}
//
//#include "window.h"
typedef struct window_state_s window_state_t;

struct zbar_window_s {
	errinfo_t err; /* error reporting */
	zbar_image_t * image; /* last displayed image NB image access must be locked! */
	uint overlay; /* user set overlay level */
	uint32 format; /* output format */
	uint width, height; /* current output size */
	uint max_width, max_height;
	uint32 src_format; /* current input format */
	uint src_width; /* last displayed image size */
	uint src_height;
	uint dst_width; /* conversion target */
	uint dst_height;
	uint scale_num; /* output scaling */
	uint scale_den;
	SPoint2I scaled_offset; /* output position and size */
	SPoint2I scaled_size;
	uint32 * formats; /* supported formats (zero terminated) */
	zbar_mutex_t imglock; /* lock displayed image */
	void * display;
	ulong xwin;
	ulong time; /* last image display in milliseconds */
	ulong time_avg; /* average of inter-frame times */
	window_state_t * state; /* platform/interface specific state */
	/* interface dependent methods */
	int (* init)(zbar_window_t*, zbar_image_t*, int);
	int (* draw_image)(zbar_window_t*, zbar_image_t*);
	int (* cleanup)(zbar_window_t*);
};

/* window.draw has to be thread safe wrt/other apis
 * FIXME should be a semaphore
 */
static inline int window_lock(zbar_window_t * w)
{
	int rc = 0;
	if((rc = _zbar_mutex_lock(&w->imglock))) {
		err_capture(w, SEV_FATAL, ZBAR_ERR_LOCKING, __FUNCTION__, "unable to acquire lock");
		w->err.errnum = rc;
		return -1;
	}
	return 0;
}

static inline int window_unlock(zbar_window_t * w)
{
	int rc = 0;
	if((rc = _zbar_mutex_unlock(&w->imglock))) {
		err_capture(w, SEV_FATAL, ZBAR_ERR_LOCKING, __FUNCTION__, "unable to release lock");
		w->err.errnum = rc;
		return -1;
	}
	return 0;
}

static inline int _zbar_window_add_format(zbar_window_t * w, uint32 fmt)
{
	int i;
	for(i = 0; w->formats && w->formats[i]; i++)
		if(w->formats[i] == fmt)
			return (i);
	w->formats = static_cast<uint32 *>(SAlloc::R(w->formats, (i + 2) * sizeof(uint32)));
	w->formats[i] = fmt;
	w->formats[i+1] = 0;
	return (i);
}

static inline SPoint2I window_scale_pt(zbar_window_t * w, SPoint2I p)
{
	p.x = ((long)p.x * w->scale_num + w->scale_den - 1) / w->scale_den;
	p.y = ((long)p.y * w->scale_num + w->scale_den - 1) / w->scale_den;
	return (p);
}

/* PAL interface */
extern int _zbar_window_attach(zbar_window_t*, void*, ulong);
extern int _zbar_window_expose(zbar_window_t*, int, int, int, int);
extern int _zbar_window_resize(zbar_window_t*);
extern int _zbar_window_clear(zbar_window_t*);
extern int _zbar_window_begin(zbar_window_t*);
extern int _zbar_window_end(zbar_window_t*);
extern int _zbar_window_draw_marker(zbar_window_t*, uint32, SPoint2I);
extern int _zbar_window_draw_polygon(zbar_window_t*, uint32, const SPoint2I*, int);
extern int _zbar_window_draw_text(zbar_window_t*, uint32, SPoint2I, const char*);
extern int _zbar_window_fill_rect(zbar_window_t*, uint32, SPoint2I, SPoint2I);
extern int _zbar_window_draw_logo(zbar_window_t*);
//
#ifdef _WIN32
	//#include "window/win.h"
	struct window_state_s {
		HDC hdc;
		void * hdd;
		BITMAPINFOHEADER bih;
		/* pre-calculated logo geometries */
		int logo_scale;
		HRGN logo_zbars;
		HPEN logo_zpen, logo_zbpen;
		POINT logo_z[4];
		int font_height;
	};

	extern int _zbar_window_bih_init(zbar_window_t * w, zbar_image_t * img);
#endif
//
#endif
