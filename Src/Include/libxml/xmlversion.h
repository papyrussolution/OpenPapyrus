/*
 * Summary: compile-time version informations
 * Description: compile-time version informations for the XML library
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */
#ifndef __XML_VERSION_H__
#define __XML_VERSION_H__
//
// XMLPUBFUN, XMLPUBVAR, XMLCALL
// 
// Macros which declare an exportable function, an exportable variable and
// the calling convention used for functions.
// 
// Please use an extra block for every platform/compiler combination when
// modifying this, rather than overlong #ifdef lines. This helps
// readability as well as the fact that different compilers on the same
// platform might need different definitions.
// 
#define XMLPUBFUN        // Macros which declare an exportable function
#define XMLPUBVAR extern // Macros which declare an exportable variable
#define XMLCALL          // Macros which declare the called convention for exported functions
#define XMLCDECL         // Macro which declares the calling convention for exported functions that use '...'.
//
// DOC_DISABLE 
//
// Windows platform with MS compiler 
#if defined(_WIN32) && defined(_MSC_VER)
	#undef XMLPUBFUN
	#undef XMLPUBVAR
	#undef XMLCALL
	#undef XMLCDECL
	#if defined(IN_LIBXML) && !defined(LIBXML_STATIC)
		#define XMLPUBFUN __declspec(dllexport)
		#define XMLPUBVAR __declspec(dllexport)
	#else
		#define XMLPUBFUN
		#if !defined(LIBXML_STATIC)
			#define XMLPUBVAR __declspec(dllimport) extern
		#else
			#define XMLPUBVAR extern
		#endif
	#endif
	#if defined(LIBXML_FASTCALL)
		#define XMLCALL __fastcall
	#else
		#define XMLCALL __cdecl
	#endif
	#define XMLCDECL __cdecl
	#if !defined _REENTRANT
		#define _REENTRANT
	#endif
#endif
// Windows platform with Borland compiler 
#if defined(_WIN32) && defined(__BORLANDC__)
	#undef XMLPUBFUN
	#undef XMLPUBVAR
	#undef XMLCALL
	#undef XMLCDECL
	#if defined(IN_LIBXML) && !defined(LIBXML_STATIC)
		#define XMLPUBFUN __declspec(dllexport)
		#define XMLPUBVAR __declspec(dllexport) extern
	#else
		#define XMLPUBFUN
		#if !defined(LIBXML_STATIC)
			#define XMLPUBVAR __declspec(dllimport) extern
		#else
			#define XMLPUBVAR extern
		#endif
	#endif
	#define XMLCALL __cdecl
	#define XMLCDECL __cdecl
	#if !defined _REENTRANT
		#define _REENTRANT
	#endif
#endif
// Windows platform with GNU compiler (Mingw) 
#if defined(_WIN32) && defined(__MINGW32__)
	#undef XMLPUBFUN
	#undef XMLPUBVAR
	#undef XMLCALL
	#undef XMLCDECL
	// 
	// if defined(IN_LIBXML) this raises problems on mingw with msys
	// _imp__xmlFree listed as missing. Try to workaround the problem
	// by also making that declaration when compiling client code.
	// 
	#if defined(IN_LIBXML) && !defined(LIBXML_STATIC)
		#define XMLPUBFUN __declspec(dllexport)
		#define XMLPUBVAR __declspec(dllexport) extern
	#else
		#define XMLPUBFUN
		#if !defined(LIBXML_STATIC)
			#define XMLPUBVAR __declspec(dllimport) extern
		#else
			#define XMLPUBVAR extern
		#endif
	#endif
	#define XMLCALL __cdecl
	#define XMLCDECL __cdecl
	#if !defined _REENTRANT
		#define _REENTRANT
	#endif
#endif
// Cygwin platform, GNU compiler 
#if defined(_WIN32) && defined(__CYGWIN__)
	#undef XMLPUBFUN
	#undef XMLPUBVAR
	#undef XMLCALL
	#undef XMLCDECL
	#if defined(IN_LIBXML) && !defined(LIBXML_STATIC)
		#define XMLPUBFUN __declspec(dllexport)
		#define XMLPUBVAR __declspec(dllexport)
	#else
		#define XMLPUBFUN
		#if !defined(LIBXML_STATIC)
			#define XMLPUBVAR __declspec(dllimport) extern
		#else
			#define XMLPUBVAR
		#endif
	#endif
	#define XMLCALL __cdecl
	#define XMLCDECL __cdecl
#endif
// Compatibility 
#if !defined(LIBXML_DLL_IMPORT)
	#define LIBXML_DLL_IMPORT XMLPUBVAR
#endif
//
#ifdef __cplusplus
extern "C" {
#endif
//
// use those to be sure nothing nasty will happen if your library and includes mismatch
//
#ifndef LIBXML2_COMPILING_MSCCDEF
	XMLPUBFUN void XMLCALL xmlCheckVersion(int version);
#endif
#define LIBXML_DOTTED_VERSION "2.9.2" // the version string like "1.2.3"
#define LIBXML_VERSION 20902 // the version number: 1.2.3 value is 10203
#define LIBXML_VERSION_STRING "20902" // the version number string, 1.2.3 value is "10203"
#define LIBXML_VERSION_EXTRA "-GITv2.9.2-rc2-10-gbe2a7ed" // extra version information, used to show a CVS compilation
/**
 * LIBXML_TEST_VERSION:
 *
 * Macro to check that the libxml version in use is compatible with
 * the version the software has been compiled against
 */
#define LIBXML_TEST_VERSION xmlCheckVersion(20902);

#ifndef VMS
	#if 0
		#define WITH_TRIO // defined if the trio support need to be configured in
	#else
		#define WITHOUT_TRIO // defined if the trio support should not be configured in
	#endif
#else
	#define WITH_TRIO 1 // defined if the trio support need to be configured in
#endif
#if 1
	#if defined(_REENTRANT) || defined(__MT__) || (defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE - 0 >= 199506L))
		#define LIBXML_THREAD_ENABLED // Whether the thread support is configured in
	#endif
#endif
#if 0
	#define LIBXML_THREAD_ALLOC_ENABLED // Whether the allocation hooks are per-thread
#endif
#if 1
	#define LIBXML_TREE_ENABLED // Whether the DOM like tree manipulation API support is configured in
#endif
#if 1
	#define LIBXML_OUTPUT_ENABLED // Whether the serialization/saving support is configured in
#endif
#if 1
	#define LIBXML_PUSH_ENABLED // Whether the push parsing interfaces are configured in
#endif
#if 1
	#define LIBXML_READER_ENABLED // Whether the xmlReader parsing interface is configured in
#endif
#if 1
#define LIBXML_PATTERN_ENABLED // Whether the xmlPattern node selection interface is configured in
#endif
#if 1
	#define LIBXML_WRITER_ENABLED // Whether the xmlWriter saving interface is configured in
#endif
#if 1
	#define LIBXML_SAX1_ENABLED // Whether the older SAX1 interface is configured in
#endif
#if 0 // @sobolev 1-->0
	#define LIBXML_FTP_ENABLED // Whether the FTP support is configured in
#endif
#if 0 // @sobolev 1-->0
	#define LIBXML_HTTP_ENABLED // Whether the HTTP support is configured in
#endif
#if 1
	#define LIBXML_VALID_ENABLED // Whether the DTD validation support is configured in
#endif
#if 0 // @v10.5.6 1-->0
	#define LIBXML_HTML_ENABLED // Whether the HTML support is configured in
#endif
#if 1
	#define LIBXML_LEGACY_ENABLED // Whether the deprecated APIs are compiled in for compatibility
#endif
#if 1
	#define LIBXML_C14N_ENABLED // Whether the Canonicalization support is configured in
#endif
#if 1
	#define LIBXML_CATALOG_ENABLED // Whether the Catalog support is configured in
#endif
#if 1
	// @v10.9.6 #define LIBXML_DOCB_ENABLED // Whether the SGML Docbook support is configured in
#endif
#if 1
	#define LIBXML_XPATH_ENABLED // Whether XPath is configured in
#endif
#if 1
	#define LIBXML_XPTR_ENABLED // Whether XPointer is configured in
#endif
#if 1
	#define LIBXML_XINCLUDE_ENABLED // Whether XInclude is configured in
#endif
#if 0 // @sobolev 1-->0
#define LIBXML_ICONV_ENABLED // Whether iconv support is available
#endif
#if 0
	#define LIBXML_ICU_ENABLED // Whether icu support is available
#endif
#if 1
	#define LIBXML_ISO8859X_ENABLED // Whether ISO-8859-* support is made available in case iconv is not
#endif
#if 1
	#define LIBXML_DEBUG_ENABLED // Whether Debugging module is configured in
#endif
#if 0
	#define DEBUG_MEMORY_LOCATION // Whether the memory debugging is configured in
#endif
#if 0
	#define LIBXML_DEBUG_RUNTIME // Whether the runtime debugging is configured in
#endif
#if 1
	#define LIBXML_UNICODE_ENABLED // Whether the Unicode related interfaces are compiled in
#endif
#if 1
	#define LIBXML_REGEXP_ENABLED // Whether the regular expressions interfaces are compiled in
#endif
#if 1
	#define LIBXML_AUTOMATA_ENABLED // Whether the automata interfaces are compiled in
#endif
#if 1
	#define LIBXML_EXPR_ENABLED // Whether the formal expressions interfaces are compiled in
#endif
#if 1
	#define LIBXML_SCHEMAS_ENABLED // Whether the Schemas validation interfaces are compiled in
#endif
#if 1
	#define LIBXML_SCHEMATRON_ENABLED // Whether the Schematron validation interfaces are compiled in
#endif
#if 1
	#define LIBXML_MODULES_ENABLED        // Whether the module interfaces are compiled in 
	#define LIBXML_MODULE_EXTENSION ".so" // the string suffix used by dynamic modules (usually shared libraries)
#endif
#if 1
	#define LIBXML_ZLIB_ENABLED // Whether the Zlib support is compiled in
#endif
#if 1
	#define LIBXML_LZMA_ENABLED // Whether the Lzma support is compiled in
#endif
#ifdef __GNUC__
#ifdef HAVE_ANSIDECL_H
	#include <ansidecl.h>
#endif
/**
 * ATTRIBUTE_UNUSED:
 *
 * Macro used to signal to GCC unused function parameters
 */
#ifndef ATTRIBUTE_UNUSED
	#if ((__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 7)))
		#define ATTRIBUTE_UNUSED __attribute__((unused))
	#else
		#define ATTRIBUTE_UNUSED
	#endif
#endif
/**
 * LIBXML_ATTR_ALLOC_SIZE:
 *
 * Macro used to indicate to GCC this is an allocator function
 */
#ifndef LIBXML_ATTR_ALLOC_SIZE
	#if ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3)))
		#define LIBXML_ATTR_ALLOC_SIZE(x) __attribute__((alloc_size(x)))
	#else
		#define LIBXML_ATTR_ALLOC_SIZE(x)
	#endif
#else
	#define LIBXML_ATTR_ALLOC_SIZE(x)
#endif
/**
 * LIBXML_ATTR_FORMAT:
 *
 * Macro used to indicate to GCC the parameter are printf like
 */
#ifndef LIBXML_ATTR_FORMAT
	#if ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)))
		#define LIBXML_ATTR_FORMAT(fmt,args) __attribute__((__format__(__printf__,fmt,args)))
	#else
		#define LIBXML_ATTR_FORMAT(fmt,args)
	#endif
#else
	#define LIBXML_ATTR_FORMAT(fmt,args)
#endif
#else /* ! __GNUC__ */
	#define ATTRIBUTE_UNUSED // Macro used to signal to GCC unused function parameters
	#define LIBXML_ATTR_ALLOC_SIZE(x) //  Macro used to indicate to GCC this is an allocator function
	#define LIBXML_ATTR_FORMAT(fmt,args) // Macro used to indicate to GCC the parameter are printf like
#endif /* __GNUC__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
