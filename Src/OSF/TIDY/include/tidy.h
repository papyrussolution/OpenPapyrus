#ifndef __TIDY_H__
#define __TIDY_H__

/** @file tidy.h - Defines HTML Tidy API implemented by tidy library.

   Public interface is const-correct and doesn't explicitly depend
   on any globals.  Thus, thread-safety may be introduced w/out
   changing the interface.

   Looking ahead to a C++ wrapper, C functions always pass
   this-equivalent as 1st arg.


   Copyright (c) 1998-2008 World Wide Web Consortium
   (Massachusetts Institute of Technology, European Research
   Consortium for Informatics and Mathematics, Keio University).
   All Rights Reserved.

   CVS Info :

    $Author: arnaud02 $
    $Date: 2008/04/22 11:00:42 $
    $Revision: 1.22 $

   Contributing Author(s):

     Dave Raggett <dsr@w3.org>

   The contributing author(s) would like to thank all those who
   helped with testing, bug fixes and suggestions for improvements.
   This wouldn't have been possible without your help.

   COPYRIGHT NOTICE:

   This software and documentation is provided "as is," and
   the copyright holders and contributing author(s) make false
   representations or warranties, express or implied, including
   but not limited to, warranties of merchantability or fitness
   for any particular purpose or that the use of the software or
   documentation will not infringe any third party patents,
   copyrights, trademarks or other rights.

   The copyright holders and contributing author(s) will not be held
   liable for any direct, indirect, special or consequential damages
   arising out of any use of the software or documentation, even if
   advised of the possibility of such damage.

   Permission is hereby granted to use, copy, modify, and distribute
   this source code, or portions hereof, documentation and executables,
   for any purpose, without fee, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented.
   2. Altered versions must be plainly marked as such and must
     not be misrepresented as being the original source.
   3. This Copyright notice may not be removed or altered from any
     source or altered source distribution.

   The copyright holders and contributing author(s) specifically
   permit, without fee, and encourage the use of this source code
   as a component for supporting the Hypertext Markup Language in
   commercial products. If you use this source code in a product,
   acknowledgment is not required but would be appreciated.


   Created 2001-05-20 by Charles Reitzel
   Updated 2002-07-01 by Charles Reitzel - 1st Implementation

*/
// @sobolev #include "platform.h"
// PLATFORM {
#ifdef __cplusplus
extern "C" {
#endif
/*
   Uncomment and edit one of the following #defines if you
   want to specify the config file at compile-time.
*/
/* #define TIDY_CONFIG_FILE "/etc/tidy_config.txt" */ /* original */
/* #define TIDY_CONFIG_FILE "/etc/tidyrc" */
/* #define TIDY_CONFIG_FILE "/etc/tidy.conf" */
/*
   Uncomment the following #define if you are on a system
   supporting the HOME environment variable.
   It enables tidy to find config files named ~/.tidyrc if
   the HTML_TIDY environment variable is not set.
*/
/* #define TIDY_USER_CONFIG_FILE "~/.tidyrc" */

/*
   Uncomment the following #define if your
   system supports the call getpwnam().
   E.g. Unix and Linux.

   It enables tidy to find files named
   ~your/foo for use in the HTML_TIDY environment
   variable or CONFIG_FILE or USER_CONFIGFILE or
   on the command line: -config ~joebob/tidy.cfg

   Contributed by Todd Lewis.
 */

/* #define SUPPORT_GETPWNAM */
#ifndef SUPPORT_ASIAN_ENCODINGS
	#define SUPPORT_ASIAN_ENCODINGS      1 // Enable/disable support for Big5 and Shift_JIS character encodings
#endif
#ifndef SUPPORT_UTF16_ENCODINGS
	#define SUPPORT_UTF16_ENCODINGS      1 // Enable/disable support for UTF-16 character encodings
#endif
#ifndef SUPPORT_ACCESSIBILITY_CHECKS
	#define SUPPORT_ACCESSIBILITY_CHECKS 1 // Enable/disable support for additional accessibility checks
#endif
//
// Convenience defines for Mac platforms
//
#if defined(macintosh)
	/* Mac OS 6.x/7.x/8.x/9.x, with or without CarbonLib - MPW or Metrowerks 68K/PPC compilers */
	#define MAC_OS_CLASSIC
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "Mac OS"
	#endif
	/* needed for access() */
	#if !defined(_POSIX) && !defined(NO_ACCESS_SUPPORT)
		#define NO_ACCESS_SUPPORT
	#endif
	#ifdef SUPPORT_GETPWNAM
		#undef SUPPORT_GETPWNAM
	#endif
	#elif defined(__APPLE__) && defined(__MACH__)
		/* Mac OS X (client) 10.x (or server 1.x/10.x) - gcc or Metrowerks MachO compilers */
		#define MAC_OS_X
		#ifndef PLATFORM_NAME
			#define PLATFORM_NAME "Mac OS X"
		#endif
	#endif
	#if defined(MAC_OS_CLASSIC) || defined(MAC_OS_X)
		/* Any OS on Mac platform */
		#define MAC_OS
		#define FILENAMES_CASE_SENSITIVE 0
		#define strcasecmp strcmp
		#ifndef DFLT_REPL_CHARENC
		#define DFLT_REPL_CHARENC MACROMAN
	#endif
#endif
//
// Convenience defines for BSD like platforms
//
#if defined(__FreeBSD__)
	#define BSD_BASED_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "FreeBSD"
	#endif
	#elif defined(__NetBSD__)
		#define BSD_BASED_OS
		#ifndef PLATFORM_NAME
			#define PLATFORM_NAME "NetBSD"
		#endif
	#elif defined(__OpenBSD__)
		#define BSD_BASED_OS
		#ifndef PLATFORM_NAME
			#define PLATFORM_NAME "OpenBSD"
		#endif
	#elif defined(__DragonFly__)
		#define BSD_BASED_OS
		#ifndef PLATFORM_NAME
			#define PLATFORM_NAME "DragonFly"
		#endif
	#elif defined(__MINT__)
		#define BSD_BASED_OS
		#ifndef PLATFORM_NAME
			#define PLATFORM_NAME "FreeMiNT"
		#endif
	#elif defined(__bsdi__)
		#define BSD_BASED_OS
		#ifndef PLATFORM_NAME
			#define PLATFORM_NAME "BSD/OS"
		#endif
#endif
//
// Convenience defines for Windows platforms
//
#if defined(WINDOWS) || defined(_WIN32)
	#define WINDOWS_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "Windows"
	#endif
	#if defined(__MWERKS__) || defined(__MSL__)
	/* not available with Metrowerks Standard Library */
		#ifdef SUPPORT_GETPWNAM
			#undef SUPPORT_GETPWNAM
		#endif
		/* needed for setmode() */
		#if !defined(NO_SETMODE_SUPPORT)
			#define NO_SETMODE_SUPPORT
		#endif
		#define strcasecmp _stricmp
	#endif
	#if defined(__BORLANDC__)
		#define strcasecmp stricmp
	#endif
	#define FILENAMES_CASE_SENSITIVE 0
	#define SUPPORT_POSIX_MAPPED_FILES 0
#endif
//
// Convenience defines for Linux platforms
//
#if defined(linux) && defined(__alpha__)
	/* Linux on Alpha - gcc compiler */
	#define LINUX_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "Linux/Alpha"
	#endif
#elif defined(linux) && defined(__sparc__)
	/* Linux on Sparc - gcc compiler */
	#define LINUX_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "Linux/Sparc"
	#endif
#elif defined(linux) && (defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__))
	/* Linux on x86 - gcc compiler */
	#define LINUX_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "Linux/x86"
	#endif
#elif defined(linux) && defined(__powerpc__)
/* Linux on PPC - gcc compiler */
#define LINUX_OS

#if defined(__linux__) && defined(__powerpc__)
	/* #if #system(linux) */
	/* MkLinux on PPC  - gcc (egcs) compiler */
	/* #define MAC_OS_MKLINUX */
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "MkLinux"
	#endif
	#else
		#ifndef PLATFORM_NAME
			#define PLATFORM_NAME "Linux/PPC"
		#endif
	#endif
#elif defined(linux) || defined(__linux__)
	/* generic Linux */
	#define LINUX_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "Linux"
	#endif
#endif
//
// Convenience defines for Solaris platforms
//
#if defined(sun)
	#define SOLARIS_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "Solaris"
	#endif
#endif
//
// Convenience defines for HPUX + gcc platforms
//
#if defined(__hpux)
	#define HPUX_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "HPUX"
	#endif
#endif
//
// Convenience defines for RISCOS + gcc platforms
//
#if defined(__riscos__)
	#define RISC_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "RISC OS"
	#endif
#endif
//
// Convenience defines for OS/2 + icc/gcc platforms
//
#if defined(__OS2__) || defined(__EMX__)
	#define OS2_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "OS/2"
	#endif
	#define FILENAMES_CASE_SENSITIVE 0
	#define strcasecmp stricmp
#endif
//
// Convenience defines for IRIX
//
#if defined(__sgi)
	#define IRIX_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "SGI IRIX"
	#endif
#endif
//
// Convenience defines for AIX
//
#if defined(_AIX)
	#define AIX_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "IBM AIX"
	#endif
#endif
//
// Convenience defines for BeOS platforms
//
#if defined(__BEOS__)
	#define BE_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "BeOS"
	#endif
#endif
//
// Convenience defines for Cygwin platforms
//
#if defined(__CYGWIN__)
	#define CYGWIN_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "Cygwin"
	#endif
	#define FILENAMES_CASE_SENSITIVE 0
#endif
//
// Convenience defines for OpenVMS
//
#if defined(__VMS)
	#define OPENVMS_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "OpenVMS"
	#endif
	#define FILENAMES_CASE_SENSITIVE 0
#endif
//
// Convenience defines for DEC Alpha OSF + gcc platforms
//
#if defined(__osf__)
	#define OSF_OS
	#ifndef PLATFORM_NAME
		#define PLATFORM_NAME "DEC Alpha OSF"
	#endif
#endif
//
// Convenience defines for ARM platforms
//
#if defined(__arm)
	#define ARM_OS
	#if defined(forARM) && defined(__NEWTON_H)
		/* Using Newton C++ Tools ARMCpp compiler */
		#define NEWTON_OS
		#ifndef PLATFORM_NAME
			#define PLATFORM_NAME "Newton"
		#endif
	#else
		#ifndef PLATFORM_NAME
			#define PLATFORM_NAME "ARM"
		#endif
	#endif
#endif
#include <ctype.h>
#include <stdio.h>
#include <setjmp.h>  /* for longjmp on error exit */
#include <stdlib.h>
#include <stdarg.h>  /* may need <varargs.h> for Unix V */
#include <string.h>
#include <assert.h>
#ifdef NEEDS_MALLOC_H
	#include <SAlloc::M.h>
#endif
#ifdef SUPPORT_GETPWNAM
	#include <pwd.h>
#endif
#ifdef NEEDS_UNISTD_H
	#include <unistd.h>  /* needed for unlink on some Unix systems */
#endif
//
// This can be set at compile time.  Usually Windows,
// except for Macintosh builds.
//
#ifndef DFLT_REPL_CHARENC
	#define DFLT_REPL_CHARENC WIN1252
#endif
//
// By default, use case-sensitive filename comparison.
//
#ifndef FILENAMES_CASE_SENSITIVE
	#define FILENAMES_CASE_SENSITIVE 1
#endif
/*
   Tidy preserves the last modified time for the files it
   cleans up.
 */

/*
   If your platform doesn't support <utime.h> and the
   utime() function, or <sys/futime> and the futime()
   function then set PRESERVE_FILE_TIMES to 0.

   If your platform doesn't support <sys/utime.h> and the
   futime() function, then set HAS_FUTIME to 0.

   If your platform supports <utime.h> and the
   utime() function requires the file to be
   closed first, then set UTIME_NEEDS_CLOSED_FILE to 1.
 */
//
// Keep old PRESERVEFILETIMES define for compatibility
//
#ifdef PRESERVEFILETIMES
	#undef PRESERVE_FILE_TIMES
	#define PRESERVE_FILE_TIMES PRESERVEFILETIMES
#endif
#ifndef PRESERVE_FILE_TIMES
	#if defined(RISC_OS) || defined(OPENVMS_OS) || defined(OSF_OS)
		#define PRESERVE_FILE_TIMES 0
	#else
		#define PRESERVE_FILE_TIMES 1
	#endif
#endif
#if PRESERVE_FILE_TIMES
	#ifndef HAS_FUTIME
		#if defined(CYGWIN_OS) || defined(BE_OS) || defined(OS2_OS) || defined(HPUX_OS) || defined(SOLARIS_OS) || defined(LINUX_OS) || \
			defined(BSD_BASED_OS) || defined(MAC_OS) || defined(__MSL__) || defined(IRIX_OS) || defined(AIX_OS) || defined(__BORLANDC__)
			#define HAS_FUTIME 0
		#else
			#define HAS_FUTIME 1
		#endif
	#endif
	#ifndef UTIME_NEEDS_CLOSED_FILE
		#if defined(SOLARIS_OS) || defined(BSD_BASED_OS) || defined(MAC_OS) || defined(__MSL__) || defined(LINUX_OS)
			#define UTIME_NEEDS_CLOSED_FILE 1
		#else
			#define UTIME_NEEDS_CLOSED_FILE 0
		#endif
	#endif
	#if defined(MAC_OS_X) || (!defined(MAC_OS_CLASSIC) && !defined(__MSL__))
		#include <sys/types.h>
		#include <sys/stat.h>
	#else
		#include <stat.h>
	#endif
	#if HAS_FUTIME
		#include <sys/utime.h>
	#else
		#include <utime.h>
	#endif /* HASFUTIME */
	/*
	   MS Windows needs _ prefix for Unix file functions.
	   Not required by Metrowerks Standard Library (MSL).

	   Tidy uses following for preserving the last modified time.

	   WINDOWS automatically set by Win16 compilers.
	   _WIN32 automatically set by Win32 compilers.
	 */
	#if defined(_WIN32) && !defined(__MSL__) && !defined(__BORLANDC__)
		#define futime _futime
		#define fstat _fstat
		#define utimbuf _utimbuf /* Windows seems to want utimbuf */
		#define stat _stat
		#define utime _utime
		#define vsnprintf _vsnprintf
	#endif /* _WIN32 */
#endif /* PRESERVE_FILE_TIMES */
/*
   MS Windows needs _ prefix for Unix file functions.
   Not required by Metrowerks Standard Library (MSL).

   WINDOWS automatically set by Win16 compilers.
   _WIN32 automatically set by Win32 compilers.
*/
#if defined(_WIN32) && !defined(__MSL__) && !defined(__BORLANDC__)
	#ifndef __WATCOMC__
		#define fileno _fileno
		#define setmode _setmode
	#endif
	#define access _access
	#define strcasecmp _stricmp
	#if _MSC_VER > 1000
		#pragma warning( disable : 4189 ) /* local variable is initialized but not referenced */
		#pragma warning( disable : 4100 ) /* unreferenced formal parameter */
		#pragma warning( disable : 4706 ) /* assignment within conditional expression */
	#endif
	#if _MSC_VER > 1300
		#pragma warning( disable : 4996 ) /* disable depreciation warning */
	#endif
#endif /* _WIN32 */
#if defined(_WIN32)
	#if (defined(_USRDLL) || defined(_WINDLL)) && !defined(TIDY_EXPORT)
		#define TIDY_EXPORT __declspec(dllexport)
	#endif
	#ifndef TIDY_CALL
		#ifdef _WIN64
			#define TIDY_CALL __fastcall
		#else
			#define TIDY_CALL __stdcall
		#endif
	#endif
#endif /* _WIN32 */

/* hack for gnu sys/types.h file which defines uint and ulong */

#if defined(BE_OS) || defined(SOLARIS_OS) || defined(BSD_BASED_OS) || defined(OSF_OS) || defined(IRIX_OS) || defined(AIX_OS)
	#include <sys/types.h>
#endif
#if !defined(HPUX_OS) && !defined(CYGWIN_OS) && !defined(MAC_OS_X) && !defined(BE_OS) && !defined(SOLARIS_OS) && !defined(BSD_BASED_OS) && \
	!defined(OSF_OS) && !defined(IRIX_OS) && !defined(AIX_OS) && !defined(LINUX_OS)
	#undef uint
	typedef unsigned int uint;
#endif
#if defined(HPUX_OS) || defined(CYGWIN_OS) || defined(MAC_OS) || defined(BSD_BASED_OS) || defined(_WIN32)
	#undef ulong
	typedef unsigned long ulong;
#endif
/*
   With GCC 4,  __attribute__ ((visibility("default"))) can be used along compiling with tidylib
   with "-fvisibility=hidden". See http://gcc.gnu.org/wiki/Visibility and build/gmake/Makefile.
 */
/*
   #if defined(__GNUC__) && __GNUC__ >= 4
   #define TIDY_EXPORT __attribute__ ((visibility("default")))
   #endif
*/
#ifndef TIDY_EXPORT /* Define it away for most builds */
	#define TIDY_EXPORT
#endif
#ifndef TIDY_STRUCT
	#define TIDY_STRUCT
#endif
typedef unsigned char byte;
typedef uint tchar;         /* single, full character */
typedef char tmbchar;       /* single, possibly partial character */
#ifndef TMBSTR_DEFINED
	typedef tmbchar* tmbstr;    /* pointer to buffer of possibly partial chars */
	typedef const tmbchar* ctmbstr; /* Ditto, but const */
	#define NULLSTR (tmbstr)""
	#define TMBSTR_DEFINED
#endif
#ifndef TIDY_CALL
	#define TIDY_CALL
#endif
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
	#define ARG_UNUSED(x) x __attribute__((unused))
#else
	#define ARG_UNUSED(x) x
#endif
//
// HAS_VSNPRINTF triggers the use of "vsnprintf", which is safe related to
// buffer overflow. Therefore, we make it the default unless HAS_VSNPRINTF has been defined.
//
#ifndef HAS_VSNPRINTF
	#define HAS_VSNPRINTF 1
#endif
#ifndef SUPPORT_POSIX_MAPPED_FILES
	#define SUPPORT_POSIX_MAPPED_FILES 1
#endif
/*
   bool is a reserved word in some but
   not all C++ compilers depending on age
   work around is to avoid bool altogether
   by introducing a new enum called bool
*/
/* We could use the C99 definition where supported
   typedef _Bool bool;
   #define false (_Bool)0
   #define true (_Bool)1
*/
/*
typedef enum {
	_no = 0,
	_yes = 1
} _Bool;
*/

/* for NULL pointers
   #define null ((const void*)0)
   extern void* null;
*/
#if defined(DMALLOC)
	#include "dmalloc.h"
#endif

/* Opaque data structure.
 *  Cast to implementation type struct within lib.
 *  This will reduce inter-dependencies/conflicts w/ application code.
 */
#if 1
#define opaque_type(typenam) \
	struct _ ## typenam { int _opaque; }; \
	typedef struct _ ## typenam const * typenam
#else
#define opaque_type(typenam) typedef const void* typenam
#endif

/* Opaque data structure used to pass back
** and forth to keep current position in a
** list or other collection.
*/
opaque_type(TidyIterator);

#ifdef __cplusplus
} /* extern "C" */
#endif
// } PLATFORM
//#include "tidyenum.h"
// TIDYENUM {
//
// Enumerate configuration options
//
//
// Categories of Tidy configuration options
//
typedef enum {
	TidyMarkup,    /**< Markup options: (X)HTML version, etc */
	TidyDiagnostics, /**< Diagnostics */
	TidyPrettyPrint, /**< Output layout */
	TidyEncoding,  /**< Character encodings */
	TidyMiscellaneous /**< File handling, message format, etc. */
} TidyConfigCategory;

/** Option IDs Used to get/set option values.
 */
typedef enum {
	TidyUnknownOption, /**< Unknown option! */
	TidyIndentSpaces, /**< Indentation n spaces */
	TidyWrapLen,   /**< Wrap margin */
	TidyTabSize,   /**< Expand tabs to n spaces */
	TidyCharEncoding, /**< In/out character encoding */
	TidyInCharEncoding, /**< Input character encoding (if different) */
	TidyOutCharEncoding, /**< Output character encoding (if different) */
	TidyNewline,   /**< Output line ending (default to platform) */
	TidyDoctypeMode, /**< See doctype property */
	TidyDoctype,   /**< User specified doctype */
	TidyDuplicateAttrs, /**< Keep first or last duplicate attribute */
	TidyAltText,   /**< Default text for alt attribute */
	/* obsolete */
	TidySlideStyle, /**< Style sheet for slides: not used for anything yet */
	TidyErrFile,   /**< File name to write errors to */
	TidyOutFile,   /**< File name to write markup to */
	TidyWriteBack, /**< If true then output tidied markup */
	TidyShowMarkup, /**< If false, normal output is suppressed */
	TidyShowWarnings, /**< However errors are always shown */
	TidyQuiet,     /**< No 'Parsing X', guessed DTD or summary */
	TidyIndentContent, /**< Indent content of appropriate tags < "auto" does text/block level content indentation */
	TidyHideEndTags, /**< Suppress optional end tags */
	TidyXmlTags,   /**< Treat input as XML */
	TidyXmlOut,    /**< Create output as XML */
	TidyXhtmlOut,  /**< Output extensible HTML */
	TidyHtmlOut,   /**< Output plain HTML, even for XHTML input. Yes means set explicitly. */
	TidyXmlDecl,   /**< Add <?xml?> for XML docs */
	TidyUpperCaseTags, /**< Output tags in upper not lower case */
	TidyUpperCaseAttrs, /**< Output attributes in upper not lower case */
	TidyMakeBare,  /**< Make bare HTML: remove Microsoft cruft */
	TidyMakeClean, /**< Replace presentational clutter by style rules */
	TidyLogicalEmphasis, /**< Replace i by em and b by strong */
	TidyDropPropAttrs, /**< Discard proprietary attributes */
	TidyDropFontTags, /**< Discard presentation tags */
	TidyDropEmptyParas, /**< Discard empty p elements */
	TidyFixComments, /**< Fix comments with adjacent hyphens */
	TidyBreakBeforeBR, /**< Output newline before <br> or not? */

	/* obsolete */
	TidyBurstSlides, /**< Create slides on each h2 element */

	TidyNumEntities, /**< Use numeric entities */
	TidyQuoteMarks, /**< Output " marks as &quot; */
	TidyQuoteNbsp, /**< Output non-breaking space as entity */
	TidyQuoteAmpersand, /**< Output naked ampersand as &amp; */
	TidyWrapAttVals, /**< Wrap within attribute values */
	TidyWrapScriptlets, /**< Wrap within JavaScript string literals */
	TidyWrapSection, /**< Wrap within <![ ... ]> section tags */
	TidyWrapAsp,   /**< Wrap within ASP pseudo elements */
	TidyWrapJste,  /**< Wrap within JSTE pseudo elements */
	TidyWrapPhp,   /**< Wrap within PHP pseudo elements */
	TidyFixBackslash, /**< Fix URLs by replacing \ with / */
	TidyIndentAttributes, /**< Newline+indent before each attribute */
	TidyXmlPIs,    /**< If set to true PIs must end with ?> */
	TidyXmlSpace,  /**< If set to true adds xml:space attr as needed */
	TidyEncloseBodyText, /**< If true text at body is wrapped in P's */
	TidyEncloseBlockText, /**< If true text in blocks is wrapped in P's */
	TidyKeepFileTimes, /**< If true last modied time is preserved */
	TidyWord2000,  /**< Draconian cleaning for Word2000 */
	TidyMark,      /**< Add meta element indicating tidied doc */
	TidyEmacs,     /**< If true format error output for GNU Emacs */
	TidyEmacsFile, /**< Name of current Emacs file */
	TidyLiteralAttribs, /**< If true attributes may use newlines */
	TidyBodyOnly,  /**< Output BODY content only */
	TidyFixUri,    /**< Applies URI encoding if necessary */
	TidyLowerLiterals, /**< Folds known attribute values to lower case */
	TidyHideComments, /**< Hides all (real) comments in output */
	TidyIndentCdata, /**< Indent <!CDATA[ ... ]]> section */
	TidyForceOutput, /**< Output document even if errors were found */
	TidyShowErrors, /**< Number of errors to put out */
	TidyAsciiChars, /**< Convert quotes and dashes to nearest ASCII char */
	TidyJoinClasses, /**< Join multiple class attributes */
	TidyJoinStyles, /**< Join multiple style attributes */
	TidyEscapeCdata, /**< Replace <![CDATA[]]> sections with escaped text */
#if SUPPORT_ASIAN_ENCODINGS
	TidyLanguage,  /**< Language property: not used for anything yet */
	TidyNCR,       /**< Allow numeric character references */
#else
	TidyLanguageNotUsed,
	TidyNCRNotUsed,
#endif
#if SUPPORT_UTF16_ENCODINGS
	TidyOutputBOM, /**< Output a Byte Order Mark (BOM) for UTF-16 encodings  < auto: if input stream has BOM, we output a BOM */
#else
	TidyOutputBOMNotUsed,
#endif
	TidyReplaceColor, /**< Replace hex color attribute values with names */
	TidyCSSPrefix, /**< CSS class naming for -clean option */
	TidyInlineTags, /**< Declared inline tags */
	TidyBlockTags, /**< Declared block tags */
	TidyEmptyTags, /**< Declared empty tags */
	TidyPreTags,   /**< Declared pre tags */
	TidyAccessibilityCheckLevel, /**< Accessibility check level 0 (old style), or 1, 2, 3 */
	TidyVertSpace, /**< degree to which markup is spread out vertically */
#if SUPPORT_ASIAN_ENCODINGS
	TidyPunctWrap, /**< consider punctuation and breaking spaces for wrapping */
#else
	TidyPunctWrapNotUsed,
#endif
	TidyMergeDivs, /**< Merge multiple DIVs */
	TidyDecorateInferredUL, /**< Mark inferred UL elements with false indent CSS */
	TidyPreserveEntities, /**< Preserve entities */
	TidySortAttributes, /**< Sort attributes */
	TidyMergeSpans, /**< Merge multiple SPANs */
	TidyAnchorAsName, /**< Define anchors as name attributes */
	N_TIDY_OPTIONS /**< Must be last */
} TidyOptionId;
/*
	Option data types
*/
typedef enum {
	TidyString,    /**< String */
	TidyInteger,   /**< Integer or enumeration */
	TidyBoolean    /**< Boolean flag */
} TidyOptionType;
/*
	AutoBool values used by ParseBool, ParseTriState, ParseIndent, ParseBOM
*/
typedef enum {
	TidyNoState, /**< maps to 'false' */
	TidyYesState, /**< maps to 'true' */
	TidyAutoState /**< Automatic */
} TidyTriState;
/*
	TidyNewline option values to control output line endings.
*/
typedef enum {
	TidyLF,     /**< Use Unix style: LF */
	TidyCRLF,   /**< Use DOS/Windows style: CR+LF */
	TidyCR      /**< Use Macintosh style: CR */
} TidyLineEnding;
/*
	Mode controlling treatment of doctype
*/
typedef enum {
	TidyDoctypeOmit, /**< Omit DOCTYPE altogether */
	TidyDoctypeAuto, /**< Keep DOCTYPE in input.  Set version to content */
	TidyDoctypeStrict, /**< Convert document to HTML 4 strict content model */
	TidyDoctypeLoose, /**< Convert document to HTML 4 transitional content model */
	TidyDoctypeUser /**< Set DOCTYPE FPI explicitly */
} TidyDoctypeModes;
//
// Mode controlling treatment of duplicate Attributes
//
typedef enum {
	TidyKeepFirst,
	TidyKeepLast
} TidyDupAttrModes;
//
// Mode controlling treatment of sorting attributes
//
typedef enum {
	TidySortAttrNone,
	TidySortAttrAlpha
} TidyAttrSortStrategy;
//
//	I/O and Message handling interface
//
//	By default, Tidy will define, create and use
//	instances of input and output handlers for
//	standard C buffered I/O (i.e. FILE* stdin,
//	FILE* stdout and FILE* stderr for content
//	input, content output and diagnostic output,
//	respectively.  A FILE* cfgFile input handler
//	will be used for config files.  Command line
//	options will just be set directly.
//
//
// Message severity level
//
typedef enum {
	TidyInfo,       /**< Information about markup usage */
	TidyWarning,    /**< Warning message */
	TidyConfig,     /**< Configuration error */
	TidyAccess,     /**< Accessibility message */
	TidyError,      /**< Error message - output suppressed */
	TidyBadDocument, /**< I/O or file system error */
	TidyFatal       /**< Crash! */
} TidyReportLevel;
//
// Document tree traversal functions
//
//Node types
//
typedef enum {
	TidyNode_Root,  /**< Root */
	TidyNode_DocType, /**< DOCTYPE */
	TidyNode_Comment, /**< Comment */
	TidyNode_ProcIns, /**< Processing Instruction */
	TidyNode_Text,  /**< Text */
	TidyNode_Start, /**< Start Tag */
	TidyNode_End,   /**< End Tag */
	TidyNode_StartEnd, /**< Start/End (empty) Tag */
	TidyNode_CDATA, /**< Unparsed Text */
	TidyNode_Section, /**< XML Section */
	TidyNode_Asp,   /**< ASP Source */
	TidyNode_Jste,  /**< JSTE Source */
	TidyNode_Php,   /**< PHP Source */
	TidyNode_XmlDecl /**< XML Declaration */
} TidyNodeType;
//
// Known HTML element types
//
typedef enum {
	TidyTag_UNKNOWN, /**< Unknown tag! */
	TidyTag_A,  /**< A */
	TidyTag_ABBR, /**< ABBR */
	TidyTag_ACRONYM, /**< ACRONYM */
	TidyTag_ADDRESS, /**< ADDRESS */
	TidyTag_ALIGN, /**< ALIGN */
	TidyTag_APPLET, /**< APPLET */
	TidyTag_AREA, /**< AREA */
	TidyTag_B,  /**< B */
	TidyTag_BASE, /**< BASE */
	TidyTag_BASEFONT, /**< BASEFONT */
	TidyTag_BDO, /**< BDO */
	TidyTag_BGSOUND, /**< BGSOUND */
	TidyTag_BIG, /**< BIG */
	TidyTag_BLINK, /**< BLINK */
	TidyTag_BLOCKQUOTE, /**< BLOCKQUOTE */
	TidyTag_BODY, /**< BODY */
	TidyTag_BR, /**< BR */
	TidyTag_BUTTON, /**< BUTTON */
	TidyTag_CAPTION, /**< CAPTION */
	TidyTag_CENTER, /**< CENTER */
	TidyTag_CITE, /**< CITE */
	TidyTag_CODE, /**< CODE */
	TidyTag_COL, /**< COL */
	TidyTag_COLGROUP, /**< COLGROUP */
	TidyTag_COMMENT, /**< COMMENT */
	TidyTag_DD, /**< DD */
	TidyTag_DEL, /**< DEL */
	TidyTag_DFN, /**< DFN */
	TidyTag_DIR, /**< DIR */
	TidyTag_DIV, /**< DIF */
	TidyTag_DL, /**< DL */
	TidyTag_DT, /**< DT */
	TidyTag_EM, /**< EM */
	TidyTag_EMBED, /**< EMBED */
	TidyTag_FIELDSET, /**< FIELDSET */
	TidyTag_FONT, /**< FONT */
	TidyTag_FORM, /**< FORM */
	TidyTag_FRAME, /**< FRAME */
	TidyTag_FRAMESET, /**< FRAMESET */
	TidyTag_H1, /**< H1 */
	TidyTag_H2, /**< H2 */
	TidyTag_H3, /**< H3 */
	TidyTag_H4, /**< H4 */
	TidyTag_H5, /**< H5 */
	TidyTag_H6, /**< H6 */
	TidyTag_HEAD, /**< HEAD */
	TidyTag_HR, /**< HR */
	TidyTag_HTML, /**< HTML */
	TidyTag_I,  /**< I */
	TidyTag_IFRAME, /**< IFRAME */
	TidyTag_ILAYER, /**< ILAYER */
	TidyTag_IMG, /**< IMG */
	TidyTag_INPUT, /**< INPUT */
	TidyTag_INS, /**< INS */
	TidyTag_ISINDEX, /**< ISINDEX */
	TidyTag_KBD, /**< KBD */
	TidyTag_KEYGEN, /**< KEYGEN */
	TidyTag_LABEL, /**< LABEL */
	TidyTag_LAYER, /**< LAYER */
	TidyTag_LEGEND, /**< LEGEND */
	TidyTag_LI, /**< LI */
	TidyTag_LINK, /**< LINK */
	TidyTag_LISTING, /**< LISTING */
	TidyTag_MAP, /**< MAP */
	TidyTag_MARQUEE, /**< MARQUEE */
	TidyTag_MENU, /**< MENU */
	TidyTag_META, /**< META */
	TidyTag_MULTICOL, /**< MULTICOL */
	TidyTag_NOBR, /**< NOBR */
	TidyTag_NOEMBED, /**< NOEMBED */
	TidyTag_NOFRAMES, /**< NOFRAMES */
	TidyTag_NOLAYER, /**< NOLAYER */
	TidyTag_NOSAVE, /**< NOSAVE */
	TidyTag_NOSCRIPT, /**< NOSCRIPT */
	TidyTag_OBJECT, /**< OBJECT */
	TidyTag_OL, /**< OL */
	TidyTag_OPTGROUP, /**< OPTGROUP */
	TidyTag_OPTION, /**< OPTION */
	TidyTag_P,  /**< P */
	TidyTag_PARAM, /**< PARAM */
	TidyTag_PLAINTEXT, /**< PLAINTEXT */
	TidyTag_PRE, /**< PRE */
	TidyTag_Q,  /**< Q */
	TidyTag_RB, /**< RB */
	TidyTag_RBC, /**< RBC */
	TidyTag_RP, /**< RP */
	TidyTag_RT, /**< RT */
	TidyTag_RTC, /**< RTC */
	TidyTag_RUBY, /**< RUBY */
	TidyTag_S,  /**< S */
	TidyTag_SAMP, /**< SAMP */
	TidyTag_SCRIPT, /**< SCRIPT */
	TidyTag_SELECT, /**< SELECT */
	TidyTag_SERVER, /**< SERVER */
	TidyTag_SERVLET, /**< SERVLET */
	TidyTag_SMALL, /**< SMALL */
	TidyTag_SPACER, /**< SPACER */
	TidyTag_SPAN, /**< SPAN */
	TidyTag_STRIKE, /**< STRIKE */
	TidyTag_STRONG, /**< STRONG */
	TidyTag_STYLE, /**< STYLE */
	TidyTag_SUB, /**< SUB */
	TidyTag_SUP, /**< SUP */
	TidyTag_TABLE, /**< TABLE */
	TidyTag_TBODY, /**< TBODY */
	TidyTag_TD, /**< TD */
	TidyTag_TEXTAREA, /**< TEXTAREA */
	TidyTag_TFOOT, /**< TFOOT */
	TidyTag_TH, /**< TH */
	TidyTag_THEAD, /**< THEAD */
	TidyTag_TITLE, /**< TITLE */
	TidyTag_TR, /**< TR */
	TidyTag_TT, /**< TT */
	TidyTag_U,  /**< U */
	TidyTag_UL, /**< UL */
	TidyTag_VAR, /**< VAR */
	TidyTag_WBR, /**< WBR */
	TidyTag_XMP, /**< XMP */
	TidyTag_NEXTID, /**< NEXTID */

	N_TIDY_TAGS /**< Must be last */
} TidyTagId;
//
// Attribute interrogation
//
// Known HTML attributes
//
typedef enum {
	TidyAttr_UNKNOWN,     /**< UNKNOWN= */
	TidyAttr_ABBR,        /**< ABBR= */
	TidyAttr_ACCEPT,      /**< ACCEPT= */
	TidyAttr_ACCEPT_CHARSET, /**< ACCEPT_CHARSET= */
	TidyAttr_ACCESSKEY,   /**< ACCESSKEY= */
	TidyAttr_ACTION,      /**< ACTION= */
	TidyAttr_ADD_DATE,    /**< ADD_DATE= */
	TidyAttr_ALIGN,       /**< ALIGN= */
	TidyAttr_ALINK,       /**< ALINK= */
	TidyAttr_ALT,         /**< ALT= */
	TidyAttr_ARCHIVE,     /**< ARCHIVE= */
	TidyAttr_AXIS,        /**< AXIS= */
	TidyAttr_BACKGROUND,  /**< BACKGROUND= */
	TidyAttr_BGCOLOR,     /**< BGCOLOR= */
	TidyAttr_BGPROPERTIES, /**< BGPROPERTIES= */
	TidyAttr_BORDER,      /**< BORDER= */
	TidyAttr_BORDERCOLOR, /**< BORDERCOLOR= */
	TidyAttr_BOTTOMMARGIN, /**< BOTTOMMARGIN= */
	TidyAttr_CELLPADDING, /**< CELLPADDING= */
	TidyAttr_CELLSPACING, /**< CELLSPACING= */
	TidyAttr_CHAR,        /**< CHAR= */
	TidyAttr_CHAROFF,     /**< CHAROFF= */
	TidyAttr_CHARSET,     /**< CHARSET= */
	TidyAttr_CHECKED,     /**< CHECKED= */
	TidyAttr_CITE,        /**< CITE= */
	TidyAttr_CLASS,       /**< CLASS= */
	TidyAttr_CLASSID,     /**< CLASSID= */
	TidyAttr_CLEAR,       /**< CLEAR= */
	TidyAttr_CODE,        /**< CODE= */
	TidyAttr_CODEBASE,    /**< CODEBASE= */
	TidyAttr_CODETYPE,    /**< CODETYPE= */
	TidyAttr_COLOR,       /**< COLOR= */
	TidyAttr_COLS,        /**< COLS= */
	TidyAttr_COLSPAN,     /**< COLSPAN= */
	TidyAttr_COMPACT,     /**< COMPACT= */
	TidyAttr_CONTENT,     /**< CONTENT= */
	TidyAttr_COORDS,      /**< COORDS= */
	TidyAttr_DATA,        /**< DATA= */
	TidyAttr_DATAFLD,     /**< DATAFLD= */
	TidyAttr_DATAFORMATAS, /**< DATAFORMATAS= */
	TidyAttr_DATAPAGESIZE, /**< DATAPAGESIZE= */
	TidyAttr_DATASRC,     /**< DATASRC= */
	TidyAttr_DATETIME,    /**< DATETIME= */
	TidyAttr_DECLARE,     /**< DECLARE= */
	TidyAttr_DEFER,       /**< DEFER= */
	TidyAttr_DIR,         /**< DIR= */
	TidyAttr_DISABLED,    /**< DISABLED= */
	TidyAttr_ENCODING,    /**< ENCODING= */
	TidyAttr_ENCTYPE,     /**< ENCTYPE= */
	TidyAttr_FACE,        /**< FACE= */
	TidyAttr_FOR,         /**< FOR= */
	TidyAttr_FRAME,       /**< FRAME= */
	TidyAttr_FRAMEBORDER, /**< FRAMEBORDER= */
	TidyAttr_FRAMESPACING, /**< FRAMESPACING= */
	TidyAttr_GRIDX,       /**< GRIDX= */
	TidyAttr_GRIDY,       /**< GRIDY= */
	TidyAttr_HEADERS,     /**< HEADERS= */
	TidyAttr_HEIGHT,      /**< HEIGHT= */
	TidyAttr_HREF,        /**< HREF= */
	TidyAttr_HREFLANG,    /**< HREFLANG= */
	TidyAttr_HSPACE,      /**< HSPACE= */
	TidyAttr_HTTP_EQUIV,  /**< HTTP_EQUIV= */
	TidyAttr_ID,          /**< ID= */
	TidyAttr_ISMAP,       /**< ISMAP= */
	TidyAttr_LABEL,       /**< LABEL= */
	TidyAttr_LANG,        /**< LANG= */
	TidyAttr_LANGUAGE,    /**< LANGUAGE= */
	TidyAttr_LAST_MODIFIED, /**< LAST_MODIFIED= */
	TidyAttr_LAST_VISIT,  /**< LAST_VISIT= */
	TidyAttr_LEFTMARGIN,  /**< LEFTMARGIN= */
	TidyAttr_LINK,        /**< LINK= */
	TidyAttr_LONGDESC,    /**< LONGDESC= */
	TidyAttr_LOWSRC,      /**< LOWSRC= */
	TidyAttr_MARGINHEIGHT, /**< MARGINHEIGHT= */
	TidyAttr_MARGINWIDTH, /**< MARGINWIDTH= */
	TidyAttr_MAXLENGTH,   /**< MAXLENGTH= */
	TidyAttr_MEDIA,       /**< MEDIA= */
	TidyAttr_METHOD,      /**< METHOD= */
	TidyAttr_MULTIPLE,    /**< MULTIPLE= */
	TidyAttr_NAME,        /**< NAME= */
	TidyAttr_NOHREF,      /**< NOHREF= */
	TidyAttr_NORESIZE,    /**< NORESIZE= */
	TidyAttr_NOSHADE,     /**< NOSHADE= */
	TidyAttr_NOWRAP,      /**< NOWRAP= */
	TidyAttr_OBJECT,      /**< OBJECT= */
	TidyAttr_OnAFTERUPDATE, /**< OnAFTERUPDATE= */
	TidyAttr_OnBEFOREUNLOAD, /**< OnBEFOREUNLOAD= */
	TidyAttr_OnBEFOREUPDATE, /**< OnBEFOREUPDATE= */
	TidyAttr_OnBLUR,      /**< OnBLUR= */
	TidyAttr_OnCHANGE,    /**< OnCHANGE= */
	TidyAttr_OnCLICK,     /**< OnCLICK= */
	TidyAttr_OnDATAAVAILABLE, /**< OnDATAAVAILABLE= */
	TidyAttr_OnDATASETCHANGED, /**< OnDATASETCHANGED= */
	TidyAttr_OnDATASETCOMPLETE, /**< OnDATASETCOMPLETE= */
	TidyAttr_OnDBLCLICK,  /**< OnDBLCLICK= */
	TidyAttr_OnERRORUPDATE, /**< OnERRORUPDATE= */
	TidyAttr_OnFOCUS,     /**< OnFOCUS= */
	TidyAttr_OnKEYDOWN,   /**< OnKEYDOWN= */
	TidyAttr_OnKEYPRESS,  /**< OnKEYPRESS= */
	TidyAttr_OnKEYUP,     /**< OnKEYUP= */
	TidyAttr_OnLOAD,      /**< OnLOAD= */
	TidyAttr_OnMOUSEDOWN, /**< OnMOUSEDOWN= */
	TidyAttr_OnMOUSEMOVE, /**< OnMOUSEMOVE= */
	TidyAttr_OnMOUSEOUT,  /**< OnMOUSEOUT= */
	TidyAttr_OnMOUSEOVER, /**< OnMOUSEOVER= */
	TidyAttr_OnMOUSEUP,   /**< OnMOUSEUP= */
	TidyAttr_OnRESET,     /**< OnRESET= */
	TidyAttr_OnROWENTER,  /**< OnROWENTER= */
	TidyAttr_OnROWEXIT,   /**< OnROWEXIT= */
	TidyAttr_OnSELECT,    /**< OnSELECT= */
	TidyAttr_OnSUBMIT,    /**< OnSUBMIT= */
	TidyAttr_OnUNLOAD,    /**< OnUNLOAD= */
	TidyAttr_PROFILE,     /**< PROFILE= */
	TidyAttr_PROMPT,      /**< PROMPT= */
	TidyAttr_RBSPAN,      /**< RBSPAN= */
	TidyAttr_READONLY,    /**< READONLY= */
	TidyAttr_REL,         /**< REL= */
	TidyAttr_REV,         /**< REV= */
	TidyAttr_RIGHTMARGIN, /**< RIGHTMARGIN= */
	TidyAttr_ROWS,        /**< ROWS= */
	TidyAttr_ROWSPAN,     /**< ROWSPAN= */
	TidyAttr_RULES,       /**< RULES= */
	TidyAttr_SCHEME,      /**< SCHEME= */
	TidyAttr_SCOPE,       /**< SCOPE= */
	TidyAttr_SCROLLING,   /**< SCROLLING= */
	TidyAttr_SELECTED,    /**< SELECTED= */
	TidyAttr_SHAPE,       /**< SHAPE= */
	TidyAttr_SHOWGRID,    /**< SHOWGRID= */
	TidyAttr_SHOWGRIDX,   /**< SHOWGRIDX= */
	TidyAttr_SHOWGRIDY,   /**< SHOWGRIDY= */
	TidyAttr_SIZE,        /**< SIZE= */
	TidyAttr_SPAN,        /**< SPAN= */
	TidyAttr_SRC,         /**< SRC= */
	TidyAttr_STANDBY,     /**< STANDBY= */
	TidyAttr_START,       /**< START= */
	TidyAttr_STYLE,       /**< STYLE= */
	TidyAttr_SUMMARY,     /**< SUMMARY= */
	TidyAttr_TABINDEX,    /**< TABINDEX= */
	TidyAttr_TARGET,      /**< TARGET= */
	TidyAttr_TEXT,        /**< TEXT= */
	TidyAttr_TITLE,       /**< TITLE= */
	TidyAttr_TOPMARGIN,   /**< TOPMARGIN= */
	TidyAttr_TYPE,        /**< TYPE= */
	TidyAttr_USEMAP,      /**< USEMAP= */
	TidyAttr_VALIGN,      /**< VALIGN= */
	TidyAttr_VALUE,       /**< VALUE= */
	TidyAttr_VALUETYPE,   /**< VALUETYPE= */
	TidyAttr_VERSION,     /**< VERSION= */
	TidyAttr_VLINK,       /**< VLINK= */
	TidyAttr_VSPACE,      /**< VSPACE= */
	TidyAttr_WIDTH,       /**< WIDTH= */
	TidyAttr_WRAP,        /**< WRAP= */
	TidyAttr_XML_LANG,    /**< XML_LANG= */
	TidyAttr_XML_SPACE,   /**< XML_SPACE= */
	TidyAttr_XMLNS,       /**< XMLNS= */

	TidyAttr_EVENT,       /**< EVENT= */
	TidyAttr_METHODS,     /**< METHODS= */
	TidyAttr_N,           /**< N= */
	TidyAttr_SDAFORM,     /**< SDAFORM= */
	TidyAttr_SDAPREF,     /**< SDAPREF= */
	TidyAttr_SDASUFF,     /**< SDASUFF= */
	TidyAttr_URN,         /**< URN= */

	N_TIDY_ATTRIBS        /**< Must be last */
} TidyAttrId;
//
// } TIDYENUM

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup Opaque Opaque Types
**
** Cast to implementation types within lib.
** Reduces inter-dependencies/conflicts w/ application code.
** @{
*/

/** @struct TidyDoc
**  Opaque document datatype
*/
opaque_type(TidyDoc);

/** @struct TidyOption
**  Opaque option datatype
*/
opaque_type(TidyOption);
//
// Opaque node datatype
//
opaque_type(TidyNode);
//
// Opaque attribute datatype
//
opaque_type(TidyAttr);

/** @} end Opaque group */

TIDY_STRUCT struct _TidyBuffer;

typedef struct _TidyBuffer TidyBuffer;

/** @defgroup Memory  Memory Allocation
**
** Tidy uses a user provided allocator for all
** memory allocations.  If this allocator is
** not provided, then a default allocator is
** used which simply wraps standard C SAlloc::M/free
** calls.  These wrappers call the panic function
** upon any failure.  The default panic function
** prints an out of memory message to stderr, and
** calls exit(2).
**
** For applications in which it is unacceptable to
** abort in the case of memory allocation, then the
** panic function can be replaced with one which
** longjmps() out of the tidy code.  For this to
** clean up completely, you should be careful not
** to use any tidy methods that open files as these
** will not be closed before panic() is called.
**
** @todo associate file handles with tidyDoc and
** ensure that tidyDocRelease() can close them all.
**
** Calling the withAllocator() family (
** tidyCreateWithAllocator, tidyBufInitWithAllocator,
** tidyBufAllocWithAllocator) allow settings custom
** allocators).
**
** All parts of the document use the same allocator.
** Calls that require a user provided buffer can
** optionally use a different allocator.
**
** For reference in designing a plug-in allocator,
** most allocations made by tidy are less than 100
** bytes, corresponding to attribute names/values, etc.
**
** There is also an additional class of much larger
** allocations which are where most of the data from
** the lexer is stored.  (It is not currently possible
** to use a separate allocator for the lexer, this
** would be a useful extension).
**
** In general, approximately 1/3rd of the memory
** used by tidy is freed during the parse, so if
** memory usage is an issue then an allocator that
** can reuse this memory is a good idea.
**
** @{
*/

/** Prototype for the allocator's function table */
struct _TidyAllocatorVtbl;

/** The allocators function table */
typedef struct _TidyAllocatorVtbl TidyAllocatorVtbl;

/** Prototype for the allocator */
struct _TidyAllocator;

/** The allocator **/
typedef struct _TidyAllocator TidyAllocator;

/** An allocator's function table.  All functions here must
    be provided.
 */
struct _TidyAllocatorVtbl {
	/** Called to allocate a block of nBytes of memory */
	void* (TIDY_CALL *alloc)( TidyAllocator *self, size_t nBytes );
	/** Called to resize (grow, in general) a block of memory.
	    Must support being called with NULL.
	 */
	void* (TIDY_CALL *realloc)( TidyAllocator *self, void * block, size_t nBytes );
	/** Called to free a previously allocated block of memory */
	void (TIDY_CALL * free)(TidyAllocator * self, void * block);
	/** Called when a panic condition is detected.  Must support
	    block == NULL.  This function is not called if either alloc
	    or realloc fails; it is up to the allocator to do this.
	    Currently this function can only be called if an error is
	    detected in the tree integrity via the internal function
	    CheckNodeIntegrity().  This is a situation that can
	    only arise in the case of a programming error in tidylib.
	    You can turn off node integrity checking by defining
	    the constant NO_NODE_INTEGRITY_CHECK during the build.
	 **/
	void (TIDY_CALL * panic)(TidyAllocator * self, ctmbstr msg);
};

/** An allocator.  To create your own allocator, do something like
    the following:

    typedef struct _MyAllocator {
       TidyAllocator base;
       ...other custom allocator state...
    } MyAllocator;

    void* MyAllocator_alloc(TidyAllocator *base, void *block, size_t nBytes)
    {
        MyAllocator *self = (MyAllocator*)base;
        ...
    }
    (etc)

    static const TidyAllocatorVtbl MyAllocatorVtbl = {
        MyAllocator_alloc,
        MyAllocator_realloc,
        MyAllocator_free,
        MyAllocator_panic
    };

    myAllocator allocator;
    TidyDoc doc;

    allocator.base.vtbl = &amp;MyAllocatorVtbl;
    ...initialise allocator specific state...
    doc = tidyCreateWithAllocator(&allocator);
    ...

    Although this looks slightly long winded, the advantage is that to create
    a custom allocator you simply need to set the vtbl pointer correctly.
    The vtbl itself can reside in static/global data, and hence does not
    need to be initialised each time an allocator is created, and furthermore
    the memory is shared amongst all created allocators.
 */
struct _TidyAllocator {
	const TidyAllocatorVtbl * vtbl;
};

/** Callback for "SAlloc::M" replacement */
typedef void* (TIDY_CALL *TidyMalloc)(size_t len);
/** Callback for "realloc" replacement */
typedef void* (TIDY_CALL *TidyRealloc)(void* buf, size_t len);
/** Callback for "free" replacement */
typedef void (TIDY_CALL *TidyFree)(void* buf);
/** Callback for "out of memory" panic state */
typedef void (TIDY_CALL *TidyPanic)(ctmbstr mssg);

/** Give Tidy a SAlloc::M() replacement */
TIDY_EXPORT bool TIDY_CALL tidySetMallocCall(TidyMalloc fmalloc);
/** Give Tidy a realloc() replacement */
TIDY_EXPORT bool TIDY_CALL tidySetReallocCall(TidyRealloc frealloc);
/** Give Tidy a free() replacement */
TIDY_EXPORT bool TIDY_CALL tidySetFreeCall(TidyFree ffree);
/** Give Tidy an "out of memory" handler */
TIDY_EXPORT bool TIDY_CALL tidySetPanicCall(TidyPanic fpanic);

/** @} end Memory group */

/** @defgroup Basic Basic Operations
**
** Tidy public interface
**
** Several functions return an integer document status:
**
** <pre>
** 0    -> SUCCESS
** >0   -> 1 == TIDY WARNING, 2 == TIDY ERROR
** <0   -> SEVERE ERROR
** </pre>
**
   The following is a short example program.

   <pre>
**#include <tidy.h>
**#include <buffio.h>
**#include <stdio.h>
**#include <errno.h>


   int main(int argc, char **argv )
   {
   const char* input = "<title>Foo</title><p>Foo!";
   TidyBuffer output;
   TidyBuffer errbuf;
   int rc = -1;
   bool ok;

   TidyDoc tdoc = tidyCreate();                     // Initialize "document"
   tidyBufInit( &output );
   tidyBufInit( &errbuf );
   printf( "Tidying:\t\%s\\n", input );

   ok = tidyOptSetBool( tdoc, TidyXhtmlOut, true );  // Convert to XHTML
   if ( ok )
    rc = tidySetErrorBuffer( tdoc, &errbuf );      // Capture diagnostics
   if ( rc >= 0 )
    rc = tidyParseString( tdoc, input );           // Parse the input
   if ( rc >= 0 )
    rc = tidyCleanAndRepair( tdoc );               // Tidy it up!
   if ( rc >= 0 )
    rc = tidyRunDiagnostics( tdoc );               // Kvetch
   if ( rc > 1 )                                    // If error, force output.
    rc = ( tidyOptSetBool(tdoc, TidyForceOutput, true) ? rc : -1 );
   if ( rc >= 0 )
    rc = tidySaveBuffer( tdoc, &output );          // Pretty Print

   if ( rc >= 0 ) {
    if ( rc > 0 )
      printf( "\\nDiagnostics:\\n\\n\%s", errbuf.bp );
    printf( "\\nAnd here is the result:\\n\\n\%s", output.bp );
   }
   else
    printf( "A severe error (\%d) occurred.\\n", rc );

   tidyBufFree( &output );
   tidyBufFree( &errbuf );
   tidyRelease( tdoc );
   return rc;
   }
   </pre>
** @{
*/

TIDY_EXPORT TidyDoc TIDY_CALL     tidyCreate(void);
TIDY_EXPORT TidyDoc TIDY_CALL     tidyCreateWithAllocator(TidyAllocator * allocator);
TIDY_EXPORT void TIDY_CALL        tidyRelease(TidyDoc tdoc);
//
// Let application store a chunk of data w/ each Tidy instance.
// Useful for callbacks.
//
TIDY_EXPORT void TIDY_CALL        tidySetAppData(TidyDoc tdoc, void* appData);
//
// Get application data set previously
//
TIDY_EXPORT void* TIDY_CALL tidyGetAppData(TidyDoc tdoc);
//
// Get release date (version) for current library
//
TIDY_EXPORT ctmbstr TIDY_CALL tidyReleaseDate(void);
//
// Diagnostics and Repair
//
// Get status of current document.
//
TIDY_EXPORT int TIDY_CALL tidyStatus(TidyDoc tdoc);
//
// Detected HTML version: 0, 2, 3 or 4
//
TIDY_EXPORT int TIDY_CALL tidyDetectedHtmlVersion(TidyDoc tdoc);
//
// Input is XHTML?
//
TIDY_EXPORT bool TIDY_CALL tidyDetectedXhtml(TidyDoc tdoc);

/** Input is generic XML (not HTML or XHTML)? */
TIDY_EXPORT bool TIDY_CALL tidyDetectedGenericXml(TidyDoc tdoc);

/** Number of Tidy errors encountered.  If > 0, output is suppressed
**  unless TidyForceOutput is set.
*/
TIDY_EXPORT uint TIDY_CALL tidyErrorCount(TidyDoc tdoc);

/** Number of Tidy warnings encountered. */
TIDY_EXPORT uint TIDY_CALL tidyWarningCount(TidyDoc tdoc);

/** Number of Tidy accessibility warnings encountered. */
TIDY_EXPORT uint TIDY_CALL        tidyAccessWarningCount(TidyDoc tdoc);

/** Number of Tidy configuration errors encountered. */
TIDY_EXPORT uint TIDY_CALL        tidyConfigErrorCount(TidyDoc tdoc);

/* Get/Set configuration options
 */
/** Load an ASCII Tidy configuration file */
TIDY_EXPORT int TIDY_CALL         tidyLoadConfig(TidyDoc tdoc, ctmbstr configFile);

/** Load a Tidy configuration file with the specified character encoding */
TIDY_EXPORT int TIDY_CALL         tidyLoadConfigEnc(TidyDoc tdoc, ctmbstr configFile,
    ctmbstr charenc);

TIDY_EXPORT bool TIDY_CALL        tidyFileExists(TidyDoc tdoc, ctmbstr filename);

/** Set the input/output character encoding for parsing markup.
**  Values include: ascii, latin1, raw, utf8, iso2022, mac,
**  win1252, utf16le, utf16be, utf16, big5 and shiftjis.  Case in-sensitive.
*/
TIDY_EXPORT int TIDY_CALL         tidySetCharEncoding(TidyDoc tdoc, ctmbstr encnam);

/** Set the input encoding for parsing markup.
** As for tidySetCharEncoding but only affects the input encoding
**/
TIDY_EXPORT int TIDY_CALL         tidySetInCharEncoding(TidyDoc tdoc, ctmbstr encnam);

/** Set the output encoding.
**/
TIDY_EXPORT int TIDY_CALL         tidySetOutCharEncoding(TidyDoc tdoc, ctmbstr encnam);

/** @} end Basic group */

/** @defgroup Configuration Configuration Options
**
** Functions for getting and setting Tidy configuration options.
** @{
*/

/** Applications using TidyLib may want to augment command-line and
**  configuration file options.  Setting this callback allows an application
**  developer to examine command-line and configuration file options after
**  TidyLib has examined them and failed to recognize them.
**/

typedef bool (TIDY_CALL *TidyOptCallback)(ctmbstr option, ctmbstr value);

TIDY_EXPORT bool TIDY_CALL          tidySetOptionCallback(TidyDoc tdoc, TidyOptCallback pOptCallback);

/** Get option ID by name */
TIDY_EXPORT TidyOptionId TIDY_CALL  tidyOptGetIdForName(ctmbstr optnam);

/** Get iterator for list of option */
/**
   Example:
   <pre>
   TidyIterator itOpt = tidyGetOptionList( tdoc );
   while ( itOpt )
   {
   TidyOption opt = tidyGetNextOption( tdoc, &itOpt );
   .. get/set option values ..
   }
   </pre>
 */

TIDY_EXPORT TidyIterator TIDY_CALL  tidyGetOptionList(TidyDoc tdoc);
/** Get next Option */
TIDY_EXPORT TidyOption TIDY_CALL    tidyGetNextOption(TidyDoc tdoc, TidyIterator* pos);
/** Lookup option by ID */
TIDY_EXPORT TidyOption TIDY_CALL    tidyGetOption(TidyDoc tdoc, TidyOptionId optId);
/** Lookup option by name */
TIDY_EXPORT TidyOption TIDY_CALL    tidyGetOptionByName(TidyDoc tdoc, ctmbstr optnam);
/** Get ID of given Option */
TIDY_EXPORT TidyOptionId TIDY_CALL  tidyOptGetId(TidyOption opt);
/** Get name of given Option */
TIDY_EXPORT ctmbstr TIDY_CALL       tidyOptGetName(TidyOption opt);
/** Get datatype of given Option */
TIDY_EXPORT TidyOptionType TIDY_CALL tidyOptGetType(TidyOption opt);
/** Is Option read-only? */
TIDY_EXPORT bool TIDY_CALL          tidyOptIsReadOnly(TidyOption opt);
/** Get category of given Option */
TIDY_EXPORT TidyConfigCategory TIDY_CALL tidyOptGetCategory(TidyOption opt);
/** Get default value of given Option as a string */
TIDY_EXPORT ctmbstr TIDY_CALL       tidyOptGetDefault(TidyOption opt);
/** Get default value of given Option as an unsigned integer */
TIDY_EXPORT ulong TIDY_CALL         tidyOptGetDefaultInt(TidyOption opt);
/** Get default value of given Option as a Boolean value */
TIDY_EXPORT bool TIDY_CALL          tidyOptGetDefaultBool(TidyOption opt);
/** Iterate over Option "pick list" */
TIDY_EXPORT TidyIterator TIDY_CALL  tidyOptGetPickList(TidyOption opt);
/** Get next string value of Option "pick list" */
TIDY_EXPORT ctmbstr TIDY_CALL       tidyOptGetNextPick(TidyOption opt, TidyIterator* pos);
/** Get current Option value as a string */
TIDY_EXPORT ctmbstr TIDY_CALL       tidyOptGetValue(TidyDoc tdoc, TidyOptionId optId);
/** Set Option value as a string */
TIDY_EXPORT bool TIDY_CALL          tidyOptSetValue(TidyDoc tdoc, TidyOptionId optId, ctmbstr val);
/** Set named Option value as a string.  Good if not sure of type. */
TIDY_EXPORT bool TIDY_CALL          tidyOptParseValue(TidyDoc tdoc, ctmbstr optnam, ctmbstr val);
/** Get current Option value as an integer */
TIDY_EXPORT ulong TIDY_CALL         tidyOptGetInt(TidyDoc tdoc, TidyOptionId optId);
/** Set Option value as an integer */
TIDY_EXPORT bool TIDY_CALL          tidyOptSetInt(TidyDoc tdoc, TidyOptionId optId, ulong val);
/** Get current Option value as a Boolean flag */
TIDY_EXPORT bool TIDY_CALL          tidyOptGetBool(TidyDoc tdoc, TidyOptionId optId);
/** Set Option value as a Boolean flag */
TIDY_EXPORT bool TIDY_CALL          tidyOptSetBool(TidyDoc tdoc, TidyOptionId optId, bool val);
/** Reset option to default value by ID */
TIDY_EXPORT bool TIDY_CALL          tidyOptResetToDefault(TidyDoc tdoc, TidyOptionId opt);
/** Reset all options to their default values */
TIDY_EXPORT bool TIDY_CALL          tidyOptResetAllToDefault(TidyDoc tdoc);
/** Take a snapshot of current config settings */
TIDY_EXPORT bool TIDY_CALL          tidyOptSnapshot(TidyDoc tdoc);
/** Reset config settings to snapshot (after document processing) */
TIDY_EXPORT bool TIDY_CALL          tidyOptResetToSnapshot(TidyDoc tdoc);
/** Any settings different than default? */
TIDY_EXPORT bool TIDY_CALL          tidyOptDiffThanDefault(TidyDoc tdoc);
/** Any settings different than snapshot? */
TIDY_EXPORT bool TIDY_CALL          tidyOptDiffThanSnapshot(TidyDoc tdoc);
/** Copy current configuration settings from one document to another */
TIDY_EXPORT bool TIDY_CALL          tidyOptCopyConfig(TidyDoc tdocTo, TidyDoc tdocFrom);
/** Get character encoding name.  Used with TidyCharEncoding,
**  TidyOutCharEncoding, TidyInCharEncoding */
TIDY_EXPORT ctmbstr TIDY_CALL       tidyOptGetEncName(TidyDoc tdoc, TidyOptionId optId);
/** Get current pick list value for option by ID.  Useful for enum types. */
TIDY_EXPORT ctmbstr TIDY_CALL       tidyOptGetCurrPick(TidyDoc tdoc, TidyOptionId optId);
/** Iterate over user declared tags */
TIDY_EXPORT TidyIterator TIDY_CALL  tidyOptGetDeclTagList(TidyDoc tdoc);
/** Get next declared tag of specified type: TidyInlineTags, TidyBlockTags,
**  TidyEmptyTags, TidyPreTags */
TIDY_EXPORT ctmbstr TIDY_CALL       tidyOptGetNextDeclTag(TidyDoc tdoc, TidyOptionId optId, TidyIterator* iter);
/** Get option description */
TIDY_EXPORT ctmbstr TIDY_CALL       tidyOptGetDoc(TidyDoc tdoc, TidyOption opt);
/** Iterate over a list of related options */
TIDY_EXPORT TidyIterator TIDY_CALL  tidyOptGetDocLinksList(TidyDoc tdoc, TidyOption opt);
/** Get next related option */
TIDY_EXPORT TidyOption TIDY_CALL    tidyOptGetNextDocLinks(TidyDoc tdoc, TidyIterator* pos);

/** @} end Configuration group */

/** @defgroup IO  I/O and Messages
**
** By default, Tidy will define, create and use
** instances of input and output handlers for
** standard C buffered I/O (i.e. FILE* stdin,
** FILE* stdout and FILE* stderr for content
** input, content output and diagnostic output,
** respectively.  A FILE* cfgFile input handler
** will be used for config files.  Command line
** options will just be set directly.
**
** @{
*/

/*****************
   Input Source
*****************/
/** Input Callback: get next byte of input */
typedef int (TIDY_CALL *TidyGetByteFunc)(void* sourceData);

/** Input Callback: unget a byte of input */
typedef void (TIDY_CALL *TidyUngetByteFunc)(void* sourceData, byte bt);

/** Input Callback: is end of input? */
typedef bool (TIDY_CALL *TidyEOFFunc)(void* sourceData);

/** End of input "character" */
#define EndOfStream (~0u)

/** TidyInputSource - Delivers raw bytes of input
 */
TIDY_STRUCT typedef struct _TidyInputSource {
	/* Instance data */
	void * sourceData; /**< Input context.  Passed to callbacks */
	/* Methods */
	TidyGetByteFunc getByte;   /**< Pointer to "get byte" callback */
	TidyUngetByteFunc ungetByte; /**< Pointer to "unget" callback */
	TidyEOFFunc eof;           /**< Pointer to "eof" callback */
} TidyInputSource;

/** Facilitates user defined source by providing
**  an entry point to marshal pointers-to-functions.
**  Needed by .NET and possibly other language bindings.
*/
TIDY_EXPORT bool TIDY_CALL tidyInitSource(TidyInputSource*  source,
    void*             srcData,
    TidyGetByteFunc gbFunc,
    TidyUngetByteFunc ugbFunc,
    TidyEOFFunc endFunc);

/** Helper: get next byte from input source */
TIDY_EXPORT uint TIDY_CALL tidyGetByte(TidyInputSource* source);
/** Helper: unget byte back to input source */
TIDY_EXPORT void TIDY_CALL tidyUngetByte(TidyInputSource* source, uint byteValue);
/** Helper: check if input source at end */
TIDY_EXPORT bool TIDY_CALL tidyIsEOF(TidyInputSource* source);

/****************
   Output Sink
****************/
/** Output callback: send a byte to output */
typedef void (TIDY_CALL *TidyPutByteFunc)(void* sinkData, byte bt);

/** TidyOutputSink - accepts raw bytes of output
 */
TIDY_STRUCT typedef struct _TidyOutputSink {
	/* Instance data */
	void * sinkData; /**< Output context.  Passed to callbacks */
	/* Methods */
	TidyPutByteFunc putByte; /**< Pointer to "put byte" callback */
} TidyOutputSink;

/** Facilitates user defined sinks by providing
**  an entry point to marshal pointers-to-functions.
**  Needed by .NET and possibly other language bindings.
*/
TIDY_EXPORT bool TIDY_CALL tidyInitSink(TidyOutputSink* sink, void * snkData, TidyPutByteFunc pbFunc);

/** Helper: send a byte to output */
TIDY_EXPORT void TIDY_CALL tidyPutByte(TidyOutputSink* sink, uint byteValue);

/** Callback to filter messages by diagnostic level:
**  info, warning, etc.  Just set diagnostic output
**  handler to redirect all diagnostics output.  Return true
**  to proceed with output, false to cancel.
*/
typedef bool (TIDY_CALL *TidyReportFilter)(TidyDoc tdoc, TidyReportLevel lvl, uint line, uint col, ctmbstr mssg);

/** Give Tidy a filter callback to use */
TIDY_EXPORT bool TIDY_CALL    tidySetReportFilter(TidyDoc tdoc, TidyReportFilter filtCallback);
/** Set error sink to named file */
TIDY_EXPORT FILE* TIDY_CALL   tidySetErrorFile(TidyDoc tdoc, ctmbstr errfilnam);
/** Set error sink to given buffer */
TIDY_EXPORT int TIDY_CALL     tidySetErrorBuffer(TidyDoc tdoc, TidyBuffer* errbuf);
/** Set error sink to given generic sink */
TIDY_EXPORT int TIDY_CALL     tidySetErrorSink(TidyDoc tdoc, TidyOutputSink* sink);

/** @} end IO group */

/* @todo Catalog all messages for easy translation
   TIDY_EXPORT ctmbstr     tidyLookupMessage( int errorNo );
 */

/** @defgroup Parse Document Parse
**
** Parse markup from a given input source.  String and filename
** functions added for convenience.  HTML/XHTML version determined
** from input.
** @{
*/

/** Parse markup in named file */
TIDY_EXPORT int TIDY_CALL tidyParseFile(TidyDoc tdoc, ctmbstr filename);
/** Parse markup from the standard input */
TIDY_EXPORT int TIDY_CALL tidyParseStdin(TidyDoc tdoc);
/** Parse markup in given string */
TIDY_EXPORT int TIDY_CALL tidyParseString(TidyDoc tdoc, ctmbstr content);
/** Parse markup in given buffer */
TIDY_EXPORT int TIDY_CALL tidyParseBuffer(TidyDoc tdoc, TidyBuffer* buf);
/** Parse markup in given generic input source */
TIDY_EXPORT int TIDY_CALL tidyParseSource(TidyDoc tdoc, TidyInputSource* source);

/** @} End Parse group */

/** @defgroup Clean Diagnostics and Repair
**
** @{
*/
/** Execute configured cleanup and repair operations on parsed markup */
TIDY_EXPORT int TIDY_CALL tidyCleanAndRepair(TidyDoc tdoc);
/** Run configured diagnostics on parsed and repaired markup.
**  Must call tidyCleanAndRepair() first.
*/
TIDY_EXPORT int TIDY_CALL tidyRunDiagnostics(TidyDoc tdoc);
/** @} end Clean group */

/** @defgroup Save Document Save Functions
**
** Save currently parsed document to the given output sink.  File name
** and string/buffer functions provided for convenience.
** @{
*/

/** Save to named file */
TIDY_EXPORT int TIDY_CALL tidySaveFile(TidyDoc tdoc, ctmbstr filename);
/** Save to standard output (FILE *) */
TIDY_EXPORT int TIDY_CALL tidySaveStdout(TidyDoc tdoc);
/** Save to given TidyBuffer object */
TIDY_EXPORT int TIDY_CALL tidySaveBuffer(TidyDoc tdoc, TidyBuffer* buf);

/** Save document to application buffer.  If buffer is not big enough,
**  ENOMEM will be returned and the necessary buffer size will be placed
**  in *buflen.
*/
TIDY_EXPORT int TIDY_CALL tidySaveString(TidyDoc tdoc, tmbstr buffer, uint* buflen);
/** Save to given generic output sink */
TIDY_EXPORT int TIDY_CALL tidySaveSink(TidyDoc tdoc, TidyOutputSink* sink);

/** @} end Save group */

/** @addtogroup Basic
** @{
*/
/** Save current settings to named file.
    Only non-default values are written. */
TIDY_EXPORT int TIDY_CALL         tidyOptSaveFile(TidyDoc tdoc, ctmbstr cfgfil);
/** Save current settings to given output sink.
    Only non-default values are written. */
TIDY_EXPORT int TIDY_CALL         tidyOptSaveSink(TidyDoc tdoc, TidyOutputSink* sink);

/* Error reporting functions
 */

/** Write more complete information about errors to current error sink. */
TIDY_EXPORT void TIDY_CALL        tidyErrorSummary(TidyDoc tdoc);

/** Write more general information about markup to current error sink. */
TIDY_EXPORT void TIDY_CALL        tidyGeneralInfo(TidyDoc tdoc);

/** @} end Basic group (again) */

/** @defgroup Tree Document Tree
**
** A parsed and, optionally, repaired document is
** represented by Tidy as a Tree, much like a W3C DOM.
** This tree may be traversed using these functions.
** The following snippet gives a basic idea how these
** functions can be used.
**
   <pre>
   void dumpNode( TidyNode tnod, int indent )
   {
   TidyNode child;

   for ( child = tidyGetChild(tnod); child; child = tidyGetNext(child) )
   {
    ctmbstr name;
    switch ( tidyNodeGetType(child) )
    {
    case TidyNode_Root:       name = "Root";                    break;
    case TidyNode_DocType:    name = "DOCTYPE";                 break;
    case TidyNode_Comment:    name = "Comment";                 break;
    case TidyNode_ProcIns:    name = "Processing Instruction";  break;
    case TidyNode_Text:       name = "Text";                    break;
    case TidyNode_CDATA:      name = "CDATA";                   break;
    case TidyNode_Section:    name = "XML Section";             break;
    case TidyNode_Asp:        name = "ASP";                     break;
    case TidyNode_Jste:       name = "JSTE";                    break;
    case TidyNode_Php:        name = "PHP";                     break;
    case TidyNode_XmlDecl:    name = "XML Declaration";         break;

    case TidyNode_Start:
    case TidyNode_End:
    case TidyNode_StartEnd:
    default:
      name = tidyNodeGetName( child );
      break;
    }
    assert( name != NULL );
    printf( "\%*.*sNode: \%s\\n", indent, indent, " ", name );
    dumpNode( child, indent + 4 );
   }
   }

   void dumpDoc( TidyDoc tdoc )
   {
   dumpNode( tidyGetRoot(tdoc), 0 );
   }

   void dumpBody( TidyDoc tdoc )
   {
   dumpNode( tidyGetBody(tdoc), 0 );
   }
   </pre>

   @{

*/

TIDY_EXPORT TidyNode TIDY_CALL    tidyGetRoot(TidyDoc tdoc);
TIDY_EXPORT TidyNode TIDY_CALL    tidyGetHtml(TidyDoc tdoc);
TIDY_EXPORT TidyNode TIDY_CALL    tidyGetHead(TidyDoc tdoc);
TIDY_EXPORT TidyNode TIDY_CALL    tidyGetBody(TidyDoc tdoc);

/* parent / child */
TIDY_EXPORT TidyNode TIDY_CALL    tidyGetParent(TidyNode tnod);
TIDY_EXPORT TidyNode TIDY_CALL    tidyGetChild(TidyNode tnod);

/* siblings */
TIDY_EXPORT TidyNode TIDY_CALL    tidyGetNext(TidyNode tnod);
TIDY_EXPORT TidyNode TIDY_CALL    tidyGetPrev(TidyNode tnod);

/* Null for non-element nodes and all pure HTML
   TIDY_EXPORT ctmbstr     tidyNodeNsLocal( TidyNode tnod );
   TIDY_EXPORT ctmbstr     tidyNodeNsPrefix( TidyNode tnod );
   TIDY_EXPORT ctmbstr     tidyNodeNsUri( TidyNode tnod );
 */

/* Iterate over attribute values */
TIDY_EXPORT TidyAttr TIDY_CALL    tidyAttrFirst(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL    tidyAttrNext(TidyAttr tattr);

TIDY_EXPORT ctmbstr TIDY_CALL     tidyAttrName(TidyAttr tattr);
TIDY_EXPORT ctmbstr TIDY_CALL     tidyAttrValue(TidyAttr tattr);

/* Null for pure HTML
   TIDY_EXPORT ctmbstr     tidyAttrNsLocal( TidyAttr tattr );
   TIDY_EXPORT ctmbstr     tidyAttrNsPrefix( TidyAttr tattr );
   TIDY_EXPORT ctmbstr     tidyAttrNsUri( TidyAttr tattr );
 */

/** @} end Tree group */

/** @defgroup NodeAsk Node Interrogation
**
** Get information about any givent node.
** @{
*/

/* Node info */
TIDY_EXPORT TidyNodeType TIDY_CALL tidyNodeGetType(TidyNode tnod);
TIDY_EXPORT ctmbstr TIDY_CALL     tidyNodeGetName(TidyNode tnod);

TIDY_EXPORT bool TIDY_CALL tidyNodeIsText(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsProp(TidyDoc tdoc, TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsHeader(TidyNode tnod);   /* h1, h2, ... */

TIDY_EXPORT bool TIDY_CALL tidyNodeHasText(TidyDoc tdoc, TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeGetText(TidyDoc tdoc, TidyNode tnod, TidyBuffer* buf);

/* Copy the unescaped value of this node into the given TidyBuffer as UTF-8 */
TIDY_EXPORT bool TIDY_CALL tidyNodeGetValue(TidyDoc tdoc, TidyNode tnod, TidyBuffer* buf);

TIDY_EXPORT TidyTagId TIDY_CALL tidyNodeGetId(TidyNode tnod);

TIDY_EXPORT uint TIDY_CALL tidyNodeLine(TidyNode tnod);
TIDY_EXPORT uint TIDY_CALL tidyNodeColumn(TidyNode tnod);

/** @defgroup NodeIsElementName Deprecated node interrogation per TagId
**
** @deprecated The functions tidyNodeIs{ElementName} are deprecated and
** should be replaced by tidyNodeGetId.
** @{
*/
TIDY_EXPORT bool TIDY_CALL tidyNodeIsHTML(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsHEAD(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsTITLE(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsBASE(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsMETA(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsBODY(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsFRAMESET(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsFRAME(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsIFRAME(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsNOFRAMES(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsHR(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsH1(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsH2(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsPRE(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsLISTING(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsP(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsUL(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsOL(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsDL(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsDIR(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsLI(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsDT(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsDD(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsTABLE(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsCAPTION(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsTD(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsTH(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsTR(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsCOL(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsCOLGROUP(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsBR(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsA(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsLINK(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsB(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsI(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsSTRONG(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsEM(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsBIG(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsSMALL(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsPARAM(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsOPTION(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsOPTGROUP(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsIMG(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsMAP(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsAREA(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsNOBR(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsWBR(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsFONT(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsLAYER(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsSPACER(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsCENTER(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsSTYLE(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsSCRIPT(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsNOSCRIPT(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsFORM(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsTEXTAREA(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsBLOCKQUOTE(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsAPPLET(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsOBJECT(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsDIV(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsSPAN(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsINPUT(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsQ(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsLABEL(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsH3(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsH4(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsH5(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsH6(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsADDRESS(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsXMP(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsSELECT(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsBLINK(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsMARQUEE(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsEMBED(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsBASEFONT(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsISINDEX(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsS(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsSTRIKE(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsU(TidyNode tnod);
TIDY_EXPORT bool TIDY_CALL tidyNodeIsMENU(TidyNode tnod);

/** @} End NodeIsElementName group */

/** @} End NodeAsk group */

/** @defgroup Attribute Attribute Interrogation
**
** Get information about any given attribute.
** @{
*/

TIDY_EXPORT TidyAttrId TIDY_CALL tidyAttrGetId(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsEvent(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsProp(TidyAttr tattr);

/** @defgroup AttrIsAttributeName Deprecated attribute interrogation per AttrId
**
** @deprecated The functions  tidyAttrIs{AttributeName} are deprecated and
** should be replaced by tidyAttrGetId.
** @{
*/
TIDY_EXPORT bool TIDY_CALL tidyAttrIsHREF(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsSRC(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsID(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsNAME(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsSUMMARY(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsALT(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsLONGDESC(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsUSEMAP(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsISMAP(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsLANGUAGE(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsTYPE(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsVALUE(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsCONTENT(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsTITLE(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsXMLNS(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsDATAFLD(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsWIDTH(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsHEIGHT(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsFOR(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsSELECTED(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsCHECKED(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsLANG(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsTARGET(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsHTTP_EQUIV(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsREL(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsOnMOUSEMOVE(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsOnMOUSEDOWN(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsOnMOUSEUP(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsOnCLICK(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsOnMOUSEOVER(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsOnMOUSEOUT(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsOnKEYDOWN(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsOnKEYUP(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsOnKEYPRESS(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsOnFOCUS(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsOnBLUR(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsBGCOLOR(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsLINK(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsALINK(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsVLINK(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsTEXT(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsSTYLE(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsABBR(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsCOLSPAN(TidyAttr tattr);
TIDY_EXPORT bool TIDY_CALL tidyAttrIsROWSPAN(TidyAttr tattr);

/** @} End AttrIsAttributeName group */

/** @} end AttrAsk group */

/** @defgroup AttrGet Attribute Retrieval
**
** Lookup an attribute from a given node
** @{
*/

TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetById(TidyNode tnod, TidyAttrId attId);

/** @defgroup AttrGetAttributeName Deprecated attribute retrieval per AttrId
**
** @deprecated The functions tidyAttrGet{AttributeName} are deprecated and
** should be replaced by tidyAttrGetById.
** For instance, tidyAttrGetID( TidyNode tnod ) can be replaced by
** tidyAttrGetById( TidyNode tnod, TidyAttr_ID ). This avoids a potential
** name clash with tidyAttrGetId for case-insensitive languages.
** @{
*/
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetHREF(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetSRC(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetID(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetNAME(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetSUMMARY(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetALT(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetLONGDESC(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetUSEMAP(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetISMAP(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetLANGUAGE(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetTYPE(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetVALUE(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetCONTENT(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetTITLE(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetXMLNS(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetDATAFLD(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetWIDTH(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetHEIGHT(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetFOR(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetSELECTED(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetCHECKED(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetLANG(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetTARGET(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetHTTP_EQUIV(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetREL(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetOnMOUSEMOVE(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetOnMOUSEDOWN(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetOnMOUSEUP(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetOnCLICK(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetOnMOUSEOVER(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetOnMOUSEOUT(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetOnKEYDOWN(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetOnKEYUP(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetOnKEYPRESS(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetOnFOCUS(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetOnBLUR(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetBGCOLOR(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetLINK(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetALINK(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetVLINK(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetTEXT(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetSTYLE(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetABBR(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetCOLSPAN(TidyNode tnod);
TIDY_EXPORT TidyAttr TIDY_CALL tidyAttrGetROWSPAN(TidyNode tnod);

/** @} End AttrGetAttributeName group */

/** @} end AttrGet group */

// BUFFIO {
//
// TidyBuffer - A chunk of memory
//
TIDY_STRUCT struct _TidyBuffer {
	TidyAllocator* allocator; /**< Memory allocator */
	byte* bp;       /**< Pointer to bytes */
	uint size;      /**< # bytes currently in use */
	uint allocated; /**< # bytes allocated */
	uint next;      /**< Offset of current input position */
};
//
// Initialize data structure using the default allocator
//
TIDY_EXPORT void TIDY_CALL tidyBufInit(TidyBuffer* buf);
//
// Initialize data structure using the given custom allocator
//
TIDY_EXPORT void TIDY_CALL tidyBufInitWithAllocator(TidyBuffer* buf, TidyAllocator* allocator);
//
// Free current buffer, allocate given amount, reset input pointer, use the default allocator
//
TIDY_EXPORT void TIDY_CALL tidyBufAlloc(TidyBuffer* buf, uint allocSize);
//
// Free current buffer, allocate given amount, reset input pointer,
// use the given custom allocator
//
TIDY_EXPORT void TIDY_CALL tidyBufAllocWithAllocator(TidyBuffer* buf, TidyAllocator* allocator, uint allocSize);
//
// Expand buffer to given size.
// Chunk size is minimum growth. Pass 0 for default of 256 bytes.
//
TIDY_EXPORT void TIDY_CALL tidyBufCheckAlloc(TidyBuffer* buf, uint allocSize, uint chunkSize);
//
// Free current contents and zero out
//
TIDY_EXPORT void TIDY_CALL tidyBufFree(TidyBuffer* buf);
//
// Set buffer bytes to 0
//
TIDY_EXPORT void TIDY_CALL tidyBufClear(TidyBuffer* buf);
//
// Attach to existing buffer
//
TIDY_EXPORT void TIDY_CALL tidyBufAttach(TidyBuffer* buf, byte* bp, uint size);
//
// Detach from buffer.  Caller must free.
//
TIDY_EXPORT void TIDY_CALL tidyBufDetach(TidyBuffer* buf);
//
// Append bytes to buffer.  Expand if necessary.
//
TIDY_EXPORT void TIDY_CALL tidyBufAppend(TidyBuffer* buf, void* vp, uint size);
//
// Append one byte to buffer.  Expand if necessary.
//
TIDY_EXPORT void TIDY_CALL tidyBufPutByte(TidyBuffer* buf, byte bv);
//
// Get byte from end of buffer
//
TIDY_EXPORT int TIDY_CALL  tidyBufPopByte(TidyBuffer* buf);
//
// Get byte from front of buffer.  Increment input offset.
//
TIDY_EXPORT int TIDY_CALL  tidyBufGetByte(TidyBuffer* buf);
//
// At end of buffer?
//
TIDY_EXPORT bool TIDY_CALL tidyBufEndOfInput(TidyBuffer* buf);
//
// Put a byte back into the buffer.  Decrement input offset.
//
TIDY_EXPORT void TIDY_CALL tidyBufUngetByte(TidyBuffer* buf, byte bv);

/**************
   TIDY
**************/
//
// Forward declarations
//
// Initialize a buffer input source
//
TIDY_EXPORT void TIDY_CALL tidyInitInputBuffer(TidyInputSource* inp, TidyBuffer* buf);
//
// Initialize a buffer output sink
//
TIDY_EXPORT void TIDY_CALL tidyInitOutputBuffer(TidyOutputSink* outp, TidyBuffer* buf);

// } BUFFIO

#ifdef __cplusplus
}  /* extern "C" */
#endif
#endif /* __TIDY_H__ */

/*
 * local variables:
 * mode: c
 * indent-tabs-mode: nil
 * c-basic-offset: 4
 * eval: (c-set-offset 'substatement-open 0)
 * end:
 */
