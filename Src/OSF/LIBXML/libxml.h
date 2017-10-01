/*
 * libxml.h: internal header only used during the compilation of libxml
 *
 * See COPYRIGHT for the status of this software
 *
 * Author: breese@users.sourceforge.net
 */

#ifndef __XML_LIBXML_H__
#define __XML_LIBXML_H__

#ifndef NO_LARGEFILE_SOURCE
	#ifndef _LARGEFILE_SOURCE
		#define _LARGEFILE_SOURCE
	#endif
	#ifndef _FILE_OFFSET_BITS
		#define _FILE_OFFSET_BITS 64
	#endif
#endif
#include <slib.h> // @sobolev
//
// Descr: Macro to cast a string to an xmlChar * when one know its safe.
//
#define BAD_CAST (xmlChar*)

#if defined(macintosh)
	#include "config-mac.h"
#elif defined(_WIN32_WCE)
	// 
	// Windows CE compatibility definitions and functions
	// This is needed to compile libxml2 for Windows CE.
	// At least I tested it with WinCE 5.0 for Emulator and WinCE 4.2/SH4 target
	// 
	#include <win32config.h>
	#include <libxml/xmlversion.h>
#else
	// 
	// Currently supported platforms use either autoconf or
	// copy to config.h own "preset" configuration file.
	// As result ifdef HAVE_CONFIG_H is omited here.
	// 
	#include "config.h"
	#include <libxml/xmlversion.h>
#endif
#if defined(__Lynx__)
	#include <stdio.h> /* pull definition of size_t */
	#include <varargs.h>
	int snprintf(char *, size_t, const char *, ...);
	int vfprintf(FILE *, const char *, va_list);
#endif
#ifndef WITH_TRIO
	#include <stdio.h>
#else
/**
 * TRIO_REPLACE_STDIO:
 *
 * This macro is defined if teh trio string formatting functions are to
 * be used instead of the default stdio ones.
 */
#define TRIO_REPLACE_STDIO
	#include "trio.h"
#endif
/*
 * Internal variable indicating if a callback has been registered for
 * node creation/destruction. It avoids spending a lot of time in locking
 * function while checking if the callback exists.
 */
extern int __xmlRegisterCallbacks;
/*
 * internal error reporting routines, shared but not partof the API.
 */
void __xmlIOErr(int domain, int code, const char *extra);
void __xmlLoaderErr(void *ctx, const char *msg, const char *filename);
#ifdef LIBXML_HTML_ENABLED
	/*
	* internal function of HTML parser needed for xmlParseInNodeContext
	* but not part of the API
	*/
	void __htmlParseContent(void *ctx);
#endif
/*
 * internal global initialization critical section routines.
 */
void __xmlGlobalInitMutexLock(void);
void __xmlGlobalInitMutexUnlock(void);
void __xmlGlobalInitMutexDestroy(void);
int __xmlInitializeDict(void);
#if defined(HAVE_RAND) && defined(HAVE_SRAND) && defined(HAVE_TIME)
	/*
	* internal thread safe random function
	*/
	int __xmlRandom(void);
#endif
int xmlNop(void);
#ifdef IN_LIBXML
// @sobolev {
//#include <string.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <stdarg.h>
//#include <limits.h>
#ifdef HAVE_CTYPE_H
	#include <ctype.h>
#endif
#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif
#ifdef HAVE_TIME_H
	#include <time.h>
#endif
#if defined(_WIN32) && defined(_MSC_VER)
	#include <windows.h>
#endif
#include <libxml/xmlstring.h>
#include <libxml/globals.h>
#include <libxml/xmlversion.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlerror.h>
#include <libxml/tree.h>
//#include <libxml/uri.h>
#include <libxml/parser.h>
#include <libxml/list.h>
#include <libxml/valid.h>
#include <libxml/hash.h>
#include <libxml/encoding.h>
#include <libxml/xmlIO.h>
#include <libxml/parserInternals.h>
#include <libxml/entities.h>
#include <libxml/threads.h>
#include <libxml/SAX.h>
#include <libxml/SAX2.h>
#include <libxml/xpath.h>
#include <libxml/xpointer.h>
#include <libxml/xpathInternals.h>
#include <libxml/debugXML.h>

#ifdef __cplusplus
extern "C" {
#endif
//
//#include <libxml/dict.h>
//
// 
// The dictionnary.
// 
//typedef struct _xmlDict xmlDict;
//typedef xmlDict * xmlDictPtr;
// 
// Initializer
// 
XMLPUBFUN int XMLCALL xmlInitializeDict();
// 
// Constructor and destructor.
// 
XMLPUBFUN xmlDict * XMLCALL xmlDictCreate();
XMLPUBFUN size_t XMLCALL xmlDictSetLimit(xmlDict * dict, size_t limit);
XMLPUBFUN size_t XMLCALL xmlDictGetUsage(xmlDict * dict);
XMLPUBFUN xmlDict * XMLCALL xmlDictCreateSub(xmlDict * sub);
XMLPUBFUN int XMLCALL xmlDictReference(xmlDict * dict);
XMLPUBFUN void XMLCALL xmlDictFree(xmlDict * dict);
// 
// Lookup of entry in the dictionnary.
// 
XMLPUBFUN const xmlChar * /*XMLCALL*/FASTCALL xmlDictLookup(xmlDict * dict, const xmlChar *name, int len);
const xmlChar * FASTCALL xmlDictLookupSL(xmlDict * dict, const xmlChar * name);
XMLPUBFUN const xmlChar * XMLCALL xmlDictExists(xmlDict * dict, const xmlChar *name, int len);
XMLPUBFUN const xmlChar * XMLCALL xmlDictQLookup(xmlDict * dict, const xmlChar *prefix, const xmlChar *name);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlDictOwns(xmlDict * pDict, const xmlChar * pStr);
XMLPUBFUN int XMLCALL xmlDictSize(xmlDict * dict);
// 
// Cleanup function
//
XMLPUBFUN void XMLCALL xmlDictCleanup  ();
//
//#include <libxml/uri.h>
//
/**
 * xmlURI:
 *
 * A parsed URI reference. This is a struct containing the various fields
 * as described in RFC 2396 but separated for further processing.
 *
 * Note: query is a deprecated field which is incorrectly unescaped.
 * query_raw takes precedence over query if the former is set.
 * See: http://mail.gnome.org/archives/xml/2007-April/thread.html#00127
 */
//typedef struct _xmlURI xmlURI;

struct xmlURI {
	char * scheme;  /* the URI scheme */
	char * opaque;  /* opaque part */
	char * authority; /* the authority part */
	char * server;  /* the server part */
	char * user;    /* the user part */
	int port;       /* the port number */
	char * path;    /* the path string */
	char * query;   /* the query string (deprecated - use with caution) */
	char * fragment; /* the fragment identifier */
	int cleanup;    /* parsing potentially unclean URI */
	char * query_raw; /* the query string (as it appears in the URI) */
};

typedef xmlURI * xmlURIPtr;
// 
// This function is in tree.h:
// xmlChar * xmlNodeGetBase(xmlDocPtr doc, xmlNode * cur);
// 
XMLPUBFUN xmlURIPtr XMLCALL xmlCreateURI();
XMLPUBFUN xmlChar * XMLCALL xmlBuildURI(const xmlChar * URI, const xmlChar * base);
XMLPUBFUN xmlChar * XMLCALL xmlBuildRelativeURI(const xmlChar * URI, const xmlChar * base);
XMLPUBFUN xmlURIPtr XMLCALL xmlParseURI(const char * str);
XMLPUBFUN xmlURIPtr XMLCALL xmlParseURIRaw(const char * str, int raw);
XMLPUBFUN int XMLCALL xmlParseURIReference(xmlURIPtr uri, const char * str);
XMLPUBFUN xmlChar * XMLCALL xmlSaveUri(xmlURIPtr uri);
XMLPUBFUN void XMLCALL xmlPrintURI(FILE * stream, xmlURIPtr uri);
XMLPUBFUN xmlChar * XMLCALL xmlURIEscapeStr(const xmlChar * str, const xmlChar * list);
XMLPUBFUN char * XMLCALL xmlURIUnescapeString(const char * str, int len, char * target);
XMLPUBFUN int XMLCALL xmlNormalizeURIPath(char * path);
XMLPUBFUN xmlChar * XMLCALL xmlURIEscape(const xmlChar * str);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeURI(xmlURI * pUri);
XMLPUBFUN xmlChar* XMLCALL xmlCanonicPath(const xmlChar * path);
XMLPUBFUN xmlChar* XMLCALL xmlPathToURI(const xmlChar * path);
//
//#include "buf.h"
//
xmlBuf * xmlBufCreate();
xmlBuf * FASTCALL xmlBufCreateSize(size_t size);
xmlBuf * FASTCALL xmlBufCreateStatic(void * mem, size_t size);
int FASTCALL xmlBufSetAllocationScheme(xmlBuf * buf, xmlBufferAllocationScheme scheme);
int FASTCALL xmlBufGetAllocationScheme(xmlBuf * buf);
void FASTCALL xmlBufFree(xmlBuf * buf);
void FASTCALL xmlBufEmpty(xmlBuf * buf);
// size_t xmlBufShrink(xmlBufPtr buf, size_t len); 
int FASTCALL xmlBufGrow(xmlBuf * buf, int len);
int FASTCALL xmlBufInflate(xmlBuf * buf, size_t len);
int FASTCALL xmlBufResize(xmlBuf * buf, size_t len);
int FASTCALL xmlBufAdd(xmlBuf * buf, const xmlChar * str, int len);
int xmlBufAddHead(xmlBufPtr buf, const xmlChar * str, int len);
int FASTCALL xmlBufCat(xmlBuf * buf, const xmlChar * str);
int FASTCALL xmlBufCCat(xmlBuf * buf, const char * str);
int FASTCALL xmlBufWriteCHAR(xmlBuf * buf, const xmlChar * string);
int FASTCALL xmlBufWriteChar(xmlBuf * buf, const char * string);
int FASTCALL xmlBufWriteQuotedString(xmlBufPtr buf, const xmlChar * string);
size_t FASTCALL xmlBufAvail(xmlBuf * buf);
size_t FASTCALL xmlBufLength(xmlBuf * buf);
// size_t xmlBufUse(const xmlBufPtr buf); 
int FASTCALL xmlBufIsEmpty(xmlBuf * buf);
int FASTCALL xmlBufAddLen(xmlBuf * buf, size_t len);
int FASTCALL xmlBufErase(xmlBuf * buf, size_t len);
// const xmlChar * xmlBufContent(const xmlBuf *buf); 
// const xmlChar * xmlBufEnd(xmlBufPtr buf); 
xmlChar * FASTCALL xmlBufDetach(xmlBuf * pBuf);
size_t xmlBufDump(FILE * file, xmlBufPtr buf);
xmlBuf * xmlBufFromBuffer(xmlBuffer * buffer);
xmlBuffer * xmlBufBackToBuffer(xmlBuf * buf);
int xmlBufMergeBuffer(xmlBufPtr buf, xmlBufferPtr buffer);
int xmlBufResetInput(xmlBufPtr buf, xmlParserInputPtr input);
size_t xmlBufGetInputBase(xmlBufPtr buf, xmlParserInputPtr input);
int xmlBufSetInputBaseCur(xmlBufPtr buf, xmlParserInputPtr input, size_t base, size_t cur);
//
//#include "enc.h"
//
int xmlCharEncFirstLineInt(xmlCharEncodingHandler *handler, xmlBufferPtr out, xmlBufferPtr in, int len);
int xmlCharEncFirstLineInput(xmlParserInputBufferPtr input, int len);
int xmlCharEncInput(xmlParserInputBufferPtr input, int flush);
int FASTCALL xmlCharEncOutput(xmlOutputBuffer * output, int init);
//
#ifdef __cplusplus
}
#endif

// } @sobolev
#ifdef __GNUC__
#ifdef PIC
#ifdef linux
#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || (__GNUC__ > 3)
#include "elfgcchack.h"
#endif
#endif
#endif
#endif
#endif
/* @sobolev
#if !defined(PIC) && !defined(NOLIBTOOL)
	#define LIBXML_STATIC
#endif
*/
#endif /* ! __XML_LIBXML_H__ */
