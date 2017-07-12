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
#if defined(macintosh)
	#include "config-mac.h"
#elif defined(_WIN32_WCE)
	/*
	* Windows CE compatibility definitions and functions
	* This is needed to compile libxml2 for Windows CE.
	* At least I tested it with WinCE 5.0 for Emulator and WinCE 4.2/SH4 target
	*/
	#include <win32config.h>
	#include <libxml/xmlversion.h>
#else
	/*
	* Currently supported platforms use either autoconf or
	* copy to config.h own "preset" configuration file.
	* As result ifdef HAVE_CONFIG_H is omited here.
	*/
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
#include <libxml/uri.h>
#include <libxml/parser.h>
#include <libxml/list.h>
#include <libxml/valid.h>
#include <libxml/hash.h>
#include <libxml/encoding.h>
#include <libxml/xmlIO.h>

#ifdef __cplusplus
extern "C" {
#endif

//#include "buf.h"
xmlBufPtr xmlBufCreate(void);
xmlBufPtr xmlBufCreateSize(size_t size);
xmlBufPtr xmlBufCreateStatic(void * mem, size_t size);
int xmlBufSetAllocationScheme(xmlBufPtr buf, xmlBufferAllocationScheme scheme);
int xmlBufGetAllocationScheme(xmlBufPtr buf);
void xmlBufFree(xmlBuf * buf);
void xmlBufEmpty(xmlBuf * buf);
// size_t xmlBufShrink(xmlBufPtr buf, size_t len); 
int FASTCALL xmlBufGrow(xmlBuf * buf, int len);
int FASTCALL xmlBufInflate(xmlBuf * buf, size_t len);
int xmlBufResize(xmlBufPtr buf, size_t len);
int xmlBufAdd(xmlBufPtr buf, const xmlChar * str, int len);
int xmlBufAddHead(xmlBufPtr buf, const xmlChar * str, int len);
int xmlBufCat(xmlBufPtr buf, const xmlChar * str);
int xmlBufCCat(xmlBufPtr buf, const char * str);
int xmlBufWriteCHAR(xmlBufPtr buf, const xmlChar * string);
int xmlBufWriteChar(xmlBufPtr buf, const char * string);
int xmlBufWriteQuotedString(xmlBufPtr buf, const xmlChar * string);
size_t FASTCALL xmlBufAvail(xmlBuf * buf);
size_t FASTCALL xmlBufLength(xmlBuf * buf);
// size_t xmlBufUse(const xmlBufPtr buf); 
int FASTCALL xmlBufIsEmpty(xmlBuf * buf);
int FASTCALL xmlBufAddLen(xmlBuf * buf, size_t len);
int FASTCALL xmlBufErase(xmlBuf * buf, size_t len);
// const xmlChar * xmlBufContent(const xmlBuf *buf); 
// const xmlChar * xmlBufEnd(xmlBufPtr buf); 
xmlChar * xmlBufDetach(xmlBufPtr buf);
size_t xmlBufDump(FILE * file, xmlBufPtr buf);
xmlBuf * xmlBufFromBuffer(xmlBuffer * buffer);
xmlBuffer * xmlBufBackToBuffer(xmlBuf * buf);
int xmlBufMergeBuffer(xmlBufPtr buf, xmlBufferPtr buffer);
int xmlBufResetInput(xmlBufPtr buf, xmlParserInputPtr input);
size_t xmlBufGetInputBase(xmlBufPtr buf, xmlParserInputPtr input);
int xmlBufSetInputBaseCur(xmlBufPtr buf, xmlParserInputPtr input, size_t base, size_t cur);

//#include "enc.h"
int xmlCharEncFirstLineInt(xmlCharEncodingHandler *handler, xmlBufferPtr out, xmlBufferPtr in, int len);
int xmlCharEncFirstLineInput(xmlParserInputBufferPtr input, int len);
int xmlCharEncInput(xmlParserInputBufferPtr input, int flush);
int xmlCharEncOutput(xmlOutputBufferPtr output, int init);

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
