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
#include <snet.h> // @v10.6.5
#define HAVE_LZMA_H   1 // @v10.9.6
#define HAVE_SIGNAL_H 1 // @v11.2.12
//
// Descr: Macro to cast a string to an xmlChar * when one know its safe.
//
#define BAD_CAST (xmlChar *)

#if defined(macintosh)
	#include "config-mac.h"
#else
	#include <libxml/xmlversion.h>
	#if defined(_WIN32_WCE)
		// 
		// Windows CE compatibility definitions and functions
		// This is needed to compile libxml2 for Windows CE.
		// At least I tested it with WinCE 5.0 for Emulator and WinCE 4.2/SH4 target
		// 
		#include <win32config.h>
	#else
		// 
		// Currently supported platforms use either autoconf or
		// copy to config.h own "preset" configuration file.
		// As result ifdef HAVE_CONFIG_H is omited here.
		// 
		//#include "libxml-config.h"
		#if defined(_WIN32_WCE)
			#undef HAVE_ERRNO_H
			#include <windows.h>
			#include "wincecompat.h"
		#else
			#include <direct.h>
		#endif
		#ifndef ICONV_CONST
			#define ICONV_CONST const
		#endif
		#ifdef NEED_SOCKETS
			#include <wsockcompat.h>
		#endif
		// 
		// Windows platforms may define except 
		// 
		#undef except

		#define HAVE_ISINF
		#define HAVE_ISNAN
		#if defined(_MSC_VER) || defined(__BORLANDC__)
			//
			// MS C-runtime has functions which can be used in order to determine if
			// a given floating-point variable contains NaN, (+-)INF. These are 
			// preferred, because floating-point technology is considered propriatary
			// by MS and we can assume that their functions know more about their oddities than we do. 
			// 
			// Bjorn Reese figured a quite nice construct for isinf() using the _fpclass function. 
			#ifndef isinf
				#define isinf(d) ((_fpclass(d) == _FPCLASS_PINF) ? 1 : ((_fpclass(d) == _FPCLASS_NINF) ? -1 : 0))
			#endif
			/* _isnan(x) returns nonzero if(x == NaN) and zero otherwise. */
			#ifndef isnan
				#define isnan(d) (_isnan(d))
			#endif
		#else
			#ifndef isinf
				static int isinf(double d) 
				{
					int    expon = 0;
					double val = frexp(d, &expon);
					if(expon == 1025) {
						if(val == 0.5)
							return 1;
						else if(val == -0.5)
							return -1;
						else
							return 0;
					} 
					else
						return 0;
				}
			#endif
			#ifndef isnan
				static int isnan(double d) 
				{
					int    expon = 0;
					double val = frexp(d, &expon);
					if(expon == 1025) {
						if(val == 0.5)
							return 0;
						else if(val == -0.5)
							return 0;
						else
							return 1;
					} 
					else
						return 0;
				}
			#endif
		#endif
		#if defined(_MSC_VER)
			#define mkdir(p,m) _mkdir(p)
			#define snprintf _snprintf
			#if _MSC_VER < 1500
				#define vsnprintf(b,c,f,a) _vsnprintf(b,c,f,a)
			#endif
		#elif defined(__MINGW32__)
			#define mkdir(p,m) _mkdir(p)
		#endif
		//
		// Threading API to use should be specified here for compatibility reasons.
		// This is however best specified on the compiler's command-line. 
		//
		#if defined(LIBXML_THREAD_ENABLED)
			#if !defined(HAVE_PTHREAD_H) && !defined(HAVE_WIN32_THREADS) && !defined(_WIN32_WCE)
				#define HAVE_WIN32_THREADS
			#endif
		#endif
		//
	#endif
#endif
#if defined(__Lynx__)
	#include <stdio.h> /* pull definition of size_t */
	#include <varargs.h>
	int snprintf(char *, size_t, const char *, ...);
	int vfprintf(FILE *, const char *, va_list);
#endif

struct xmlDoc;
struct xmlNs;
struct xmlNode;
struct xmlHashTable;
struct xmlValidCtxt;
struct xmlMutex;  // xmlMutex are a simple mutual exception locks.
struct xmlRMutex; // xmlRMutex are reentrant mutual exception locks.
//
#include <libxml/xmlversion.h>
#include <libxml/xmlerror.h>
#include <libxml/tree.h>
#include <libxml/list.h>
#include <libxml/xmlautomata.h>
#include <libxml/valid.h>
#include <libxml/xmlmemory.h>
#include <libxml/encoding.h>
#include <libxml/globals.h>
#include <libxml/parser.h>
#include <libxml/xmlIO.h>
#ifdef HAVE_ZLIB_H
	#include <zlib.h>
#endif
#ifdef HAVE_LZMA_H
	#include <..\osf\liblzma\api\lzma.h>
#endif
// 
// Internal variable indicating if a callback has been registered for
// node creation/destruction. It avoids spending a lot of time in locking
// function while checking if the callback exists.
// 
extern int __xmlRegisterCallbacks;
// 
// internal error reporting routines, shared but not partof the API.
// 
void __xmlIOErr(int domain, int code, const char *extra);
void __xmlLoaderErr(void *ctx, const char *msg, const char *filename);
#ifdef LIBXML_HTML_ENABLED
	// 
	// internal function of HTML parser needed for xmlParseInNodeContext but not part of the API
	// 
	void __htmlParseContent(void *ctx);
#endif
// 
// internal global initialization critical section routines.
// 
void __xmlGlobalInitMutexLock();
void __xmlGlobalInitMutexUnlock();
void __xmlGlobalInitMutexDestroy();
int __xmlInitializeDict();
#if defined(HAVE_RAND) && defined(HAVE_SRAND) && defined(HAVE_TIME)
	//
	// Descr: internal thread safe random function
	//
	int __xmlRandom(void);
#endif
int xmlNop(void);
#ifdef IN_LIBXML
	// @sobolev {
	#ifdef HAVE_UNISTD_H
		#include <unistd.h>
	#endif
	//#ifdef __cplusplus
	//extern "C" {
	//#endif
	/*
	 * xmlChar handling
	 */
	//XMLPUBFUN xmlChar * xmlStrdup_Removed(const xmlChar * cur);
	XMLPUBFUN xmlChar * FASTCALL xmlStrndup(const xmlChar * cur, /*int*/SSIZE_T len);
	XMLPUBFUN xmlChar * xmlCharStrndup(const char * cur, int len);
	XMLPUBFUN xmlChar * xmlCharStrdup(const char * cur);
	XMLPUBFUN xmlChar * xmlStrsub(const xmlChar * str, int start, int len);
	XMLPUBFUN const xmlChar * FASTCALL xmlStrchr(const xmlChar * str, xmlChar val);
	XMLPUBFUN const xmlChar * FASTCALL xmlStrstr(const xmlChar * str, const xmlChar * val);
	XMLPUBFUN const xmlChar * xmlStrcasestr(const xmlChar * str, const xmlChar * val);
	XMLPUBFUN int xmlStrcmp(const xmlChar * str1, const xmlChar * str2);
	XMLPUBFUN int FASTCALL xmlStrncmp(const xmlChar * str1, const xmlChar * str2, int len);
	XMLPUBFUN int xmlStrcasecmp(const xmlChar * str1, const xmlChar * str2);
	XMLPUBFUN int xmlStrncasecmp(const xmlChar * str1, const xmlChar * str2, int len);
	//XMLPUBFUN int xmlStrEqual_Removed(const xmlChar * str1, const xmlChar * str2);
	XMLPUBFUN int FASTCALL xmlStrQEqual(const xmlChar * pref, const xmlChar * name, const xmlChar * str);
	//XMLPUBFUN int xmlStrlen_Removed(const xmlChar * str);
	XMLPUBFUN xmlChar * FASTCALL xmlStrcat(xmlChar * cur, const xmlChar * add);
	XMLPUBFUN xmlChar * xmlStrncat(xmlChar * cur, const xmlChar * add, int len);
	XMLPUBFUN xmlChar * xmlStrncatNew(const xmlChar * str1, const xmlChar * str2, int len);
	XMLPUBFUN int xmlStrPrintf(xmlChar * buf, int len, const xmlChar * msg,...);
	XMLPUBFUN int xmlStrVPrintf(xmlChar * buf, int len, const xmlChar * msg, va_list ap);
	XMLPUBFUN int FASTCALL xmlGetUTF8Char(const uchar * utf, int * len);
	XMLPUBFUN int xmlCheckUTF8(const uchar * utf);
	XMLPUBFUN int xmlUTF8Strsize(const xmlChar * utf, int len);
	XMLPUBFUN xmlChar * xmlUTF8Strndup(const xmlChar * utf, int len);
	XMLPUBFUN const xmlChar * xmlUTF8Strpos(const xmlChar * utf, int pos);
	XMLPUBFUN int xmlUTF8Strloc(const xmlChar * utf, const xmlChar * utfchar);
	XMLPUBFUN xmlChar * xmlUTF8Strsub(const xmlChar * utf, int start, int len);
	XMLPUBFUN int FASTCALL  xmlUTF8Strlen(const xmlChar * utf);
	XMLPUBFUN int xmlUTF8Size(const xmlChar * utf);
	XMLPUBFUN int xmlUTF8Charcmp(const xmlChar * utf1, const xmlChar * utf2);
	/*
	 * Recent version of gcc produce a warning when a function pointer is assigned
	 * to an object pointer, or vice versa.  The following macro is a dirty hack
	 * to allow suppression of the warning.  If your architecture has function
	 * pointers which are a different size than a void pointer, there may be some
	 * serious trouble within the library.
	 */
	/**
	 * XML_CAST_FPTR:
	 * @fptr:  pointer to a function
	 *
	 * Macro to do a casting from an object pointer to a
	 * function pointer without encountering a warning from
	 * gcc
	 *
	 * #define XML_CAST_FPTR(fptr) (*(void **)(&fptr))
	 * This macro violated ISO C aliasing rules (gcc4 on s390 broke) so it is disabled now
	 */
	#define XML_CAST_FPTR(fptr) fptr
	// 
	// function types:
	// 
	/**
	 * xmlHashDeallocator:
	 * @payload:  the data in the hash
	 * @name:  the name associated
	 *
	 * Callback to free data from a hash.
	 */
	typedef void (*xmlHashDeallocator)(void *payload, xmlChar *name);
	/**
	 * xmlHashCopier:
	 * @payload:  the data in the hash
	 * @name:  the name associated
	 *
	 * Callback to copy data from a hash.
	 *
	 * Returns a copy of the data or NULL in case of error.
	 */
	typedef void *(*xmlHashCopier)(const void *payload, xmlChar *name);
	/**
	 * xmlHashScanner:
	 * @payload:  the data in the hash
	 * @data:  extra scannner data
	 * @name:  the name associated
	 *
	 * Callback when scanning data in a hash with the simple scanner.
	 */
	typedef void (*xmlHashScanner)(void *payload, void *data, xmlChar *name);
	// 
	// xmlHashScannerFull:
	// @payload:  the data in the hash
	// @data:  extra scannner data
	// @name:  the name associated
	// @name2:  the second name associated
	// @name3:  the third name associated
	// 
	// Callback when scanning data in a hash with the full scanner.
	// 
	typedef void (*xmlHashScannerFull)(void *payload, void *data, const xmlChar *name, const xmlChar *name2, const xmlChar *name3);
	XMLPUBFUN xmlHashTable * FASTCALL xmlHashCreate(int size);
	XMLPUBFUN xmlHashTable * FASTCALL xmlHashCreateDict(int size, xmlDict * dict);
	XMLPUBFUN void FASTCALL xmlHashFree(xmlHashTable * pTable, xmlHashDeallocator f);
	// 
	// Add a new entry to the hash table.
	// 
	XMLPUBFUN int FASTCALL xmlHashAddEntry(xmlHashTable * table, const xmlChar *name, void *userdata);
	XMLPUBFUN int xmlHashUpdateEntry(xmlHashTable * table, const xmlChar *name, void *userdata, xmlHashDeallocator f);
	XMLPUBFUN int FASTCALL xmlHashAddEntry2(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, void *userdata);
	XMLPUBFUN int xmlHashUpdateEntry2(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, void *userdata, xmlHashDeallocator f);
	XMLPUBFUN int FASTCALL xmlHashAddEntry3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3, void *userdata);
	XMLPUBFUN int xmlHashUpdateEntry3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3, void *userdata, xmlHashDeallocator f);
	// 
	// Remove an entry from the hash table.
	// 
	XMLPUBFUN int FASTCALL xmlHashRemoveEntry(xmlHashTable * table, const xmlChar *name, xmlHashDeallocator f);
	XMLPUBFUN int FASTCALL xmlHashRemoveEntry2(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, xmlHashDeallocator f);
	XMLPUBFUN int FASTCALL xmlHashRemoveEntry3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3,xmlHashDeallocator f);
	// 
	// Retrieve the userdata.
	// 
	XMLPUBFUN void * FASTCALL xmlHashLookup(xmlHashTable * table, const xmlChar *name);
	XMLPUBFUN void * FASTCALL xmlHashLookup2(xmlHashTable * table, const xmlChar *name, const xmlChar *name2);
	XMLPUBFUN void * FASTCALL xmlHashLookup3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3);
	XMLPUBFUN void * xmlHashQLookup(xmlHashTable * table, const xmlChar *name, const xmlChar *prefix);
	XMLPUBFUN void * xmlHashQLookup2(xmlHashTable * table, const xmlChar *name, const xmlChar *prefix, const xmlChar *name2, const xmlChar *prefix2);
	XMLPUBFUN void * xmlHashQLookup3(xmlHashTable * table, const xmlChar *name, const xmlChar *prefix, const xmlChar *name2, const xmlChar *prefix2, const xmlChar *name3, const xmlChar *prefix3);
	// 
	// Helpers.
	// 
	XMLPUBFUN xmlHashTable * xmlHashCopy(const xmlHashTable * table, xmlHashCopier f);
	XMLPUBFUN int FASTCALL xmlHashSize(xmlHashTable * table);
	XMLPUBFUN void xmlHashScan(xmlHashTable * table, xmlHashScanner f, void *data);
	XMLPUBFUN void xmlHashScan3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3, xmlHashScanner f, void *data);
	XMLPUBFUN void xmlHashScanFull(xmlHashTable * table, xmlHashScannerFull f, void *data);
	XMLPUBFUN void xmlHashScanFull3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3, xmlHashScannerFull f, void *data);
	//#ifdef __cplusplus
	//}
	//#endif
	//
	//#include <libxml/xmlregexp.h>
	#ifdef LIBXML_REGEXP_ENABLED
		//#ifdef __cplusplus
		//extern "C" {
		//#endif
		// 
		// Descr: A libxml regular expression, they can actually be far more complex
		//   thank the POSIX regex expressions.
		// 
		struct xmlRegexp;
		//typedef xmlRegexp * xmlRegexpPtr;
		// 
		// A libxml progressive regular expression evaluation context
		// 
		typedef struct _xmlRegExecCtxt xmlRegExecCtxt;
		typedef xmlRegExecCtxt *xmlRegExecCtxtPtr;
		// 
		// The POSIX like API
		// 
		XMLPUBFUN xmlRegexp * xmlRegexpCompile(const xmlChar *regexp);
		XMLPUBFUN void FASTCALL xmlRegFreeRegexp(xmlRegexp * regexp);
		XMLPUBFUN int  xmlRegexpExec(xmlRegexp * comp, const xmlChar *value);
		XMLPUBFUN void xmlRegexpPrint(FILE *output, xmlRegexp * regexp);
		XMLPUBFUN int  xmlRegexpIsDeterminist(xmlRegexp * comp);
		// 
		// Descr: Callback function when doing a transition in the automata
		// @exec: the regular expression context
		// @token: the current token string
		// @transdata: transition data
		// @inputdata: input data
		// 
		typedef void (*xmlRegExecCallbacks) (xmlRegExecCtxtPtr exec, const xmlChar *token, void *transdata, void *inputdata);
		// 
		// The progressive API
		// 
		XMLPUBFUN xmlRegExecCtxtPtr xmlRegNewExecCtxt(xmlRegexp * comp, xmlRegExecCallbacks callback, void *data);
		XMLPUBFUN void xmlRegFreeExecCtxt(xmlRegExecCtxtPtr exec); 
		XMLPUBFUN int  xmlRegExecPushString(xmlRegExecCtxtPtr exec, const xmlChar *value, void *data);
		XMLPUBFUN int  xmlRegExecPushString2(xmlRegExecCtxtPtr exec, const xmlChar *value, const xmlChar *value2, void *data);
		XMLPUBFUN int  xmlRegExecNextValues(xmlRegExecCtxtPtr exec, int *nbval, int *nbneg, xmlChar **values, int *terminal);
		XMLPUBFUN int  xmlRegExecErrInfo(xmlRegExecCtxtPtr exec, const xmlChar **string, int *nbval, int *nbneg, xmlChar **values, int *terminal);
		#ifdef LIBXML_EXPR_ENABLED
			// 
			// Formal regular expression handling
			// Its goal is to do some formal work on content models
			// 
			// expressions are used within a context 
			typedef struct _xmlExpCtxt xmlExpCtxt;
			typedef xmlExpCtxt *xmlExpCtxtPtr;

			XMLPUBFUN void xmlExpFreeCtxt(xmlExpCtxtPtr ctxt);
			XMLPUBFUN xmlExpCtxtPtr xmlExpNewCtxt(int maxNodes, xmlDict * dict);
			XMLPUBFUN int xmlExpCtxtNbNodes(xmlExpCtxtPtr ctxt);
			XMLPUBFUN int xmlExpCtxtNbCons(xmlExpCtxtPtr ctxt);

			// Expressions are trees but the tree is opaque 
			typedef struct _xmlExpNode xmlExpNode;
			typedef xmlExpNode *xmlExpNodePtr;

			enum xmlExpNodeType {
				XML_EXP_EMPTY = 0,
				XML_EXP_FORBID = 1,
				XML_EXP_ATOM = 2,
				XML_EXP_SEQ = 3,
				XML_EXP_OR = 4,
				XML_EXP_COUNT = 5
			};
			/*
			 * 2 core expressions shared by all for the empty language set
			 * and for the set with just the empty token
			 */
			XMLPUBVAR xmlExpNodePtr forbiddenExp;
			XMLPUBVAR xmlExpNodePtr emptyExp;
			/*
			 * Expressions are reference counted internally
			 */
			XMLPUBFUN void FASTCALL xmlExpFree(xmlExpCtxt * pCtxt, xmlExpNode * pExpr);
			XMLPUBFUN void xmlExpRef(xmlExpNodePtr expr);
			/*
			 * constructors can be either manual or from a string
			 */
			XMLPUBFUN xmlExpNodePtr xmlExpParse(xmlExpCtxtPtr ctxt, const char *expr);
			XMLPUBFUN xmlExpNodePtr xmlExpNewAtom(xmlExpCtxtPtr ctxt, const xmlChar *name, int len);
			XMLPUBFUN xmlExpNodePtr xmlExpNewOr(xmlExpCtxtPtr ctxt, xmlExpNodePtr left, xmlExpNodePtr right);
			XMLPUBFUN xmlExpNodePtr xmlExpNewSeq(xmlExpCtxtPtr ctxt, xmlExpNodePtr left, xmlExpNodePtr right);
			XMLPUBFUN xmlExpNodePtr xmlExpNewRange(xmlExpCtxtPtr ctxt, xmlExpNodePtr subset, int min, int max);
			/*
			 * The really interesting APIs
			 */
			XMLPUBFUN int xmlExpIsNillable(xmlExpNodePtr expr);
			XMLPUBFUN int xmlExpMaxToken(xmlExpNodePtr expr);
			XMLPUBFUN int xmlExpGetLanguage(xmlExpCtxtPtr ctxt, xmlExpNodePtr expr, const xmlChar**langList, int len);
			XMLPUBFUN int xmlExpGetStart(xmlExpCtxtPtr ctxt, xmlExpNodePtr expr, const xmlChar**tokList, int len);
			XMLPUBFUN xmlExpNodePtr xmlExpStringDerive(xmlExpCtxtPtr ctxt, xmlExpNodePtr expr, const xmlChar *str, int len);
			XMLPUBFUN xmlExpNodePtr xmlExpExpDerive	(xmlExpCtxtPtr ctxt, xmlExpNodePtr expr, xmlExpNodePtr sub);
			XMLPUBFUN int xmlExpSubsume(xmlExpCtxtPtr ctxt, xmlExpNodePtr expr, xmlExpNodePtr sub);
			XMLPUBFUN void xmlExpDump(xmlBuffer * buf, xmlExpNodePtr expr);
		#endif // LIBXML_EXPR_ENABLED 
		//#ifdef __cplusplus
		//}
		//#endif
	#endif // LIBXML_REGEXP_ENABLED 
	//
	//#ifdef __cplusplus
	//extern "C" {
	//#endif
	// 
	// The different valid entity types.
	// 
	enum xmlEntityType {
		XML_INTERNAL_GENERAL_ENTITY = 1,
		XML_EXTERNAL_GENERAL_PARSED_ENTITY = 2,
		XML_EXTERNAL_GENERAL_UNPARSED_ENTITY = 3,
		XML_INTERNAL_PARAMETER_ENTITY = 4,
		XML_EXTERNAL_PARAMETER_ENTITY = 5,
		XML_INTERNAL_PREDEFINED_ENTITY = 6
	};
	// 
	// An unit of storage for an entity, contains the string, the value
	// and the linkind data needed for the linking in the hash table.
	// 
	struct xmlEntity {
		void * _private; // application data 
		xmlElementType type;  // XML_ENTITY_DECL, must be second ! 
		const xmlChar * name; // Entity name 
		xmlNode * children;   // First child link 
		xmlNode * last; /* Last child link */
		xmlDtd  * parent; /* -> DTD */
		xmlNode * next; /* next sibling link  */
		xmlNode * prev; /* previous sibling link  */
		xmlDoc  * doc; /* the containing document */
		const xmlChar * orig; /* content without ref substitution */
		const xmlChar * content; /* content or ndata if unparsed */
		int    length; /* the content length */
		xmlEntityType etype; /* The entity type */
		const xmlChar * ExternalID; // External identifier for PUBLIC 
		const xmlChar * SystemID;   // URI for a SYSTEM or PUBLIC Entity 
		struct xmlEntity * nexte;   // unused 
		const  xmlChar * URI; // the full URI as computed 
		int    owner;   // does the entity own the childrens 
		int    checked; // was the entity content checked  this is also used to count entities references done from that entity and if it contains '<'
	};
	// 
	// All entities are stored in an hash table.
	// There is 2 separate hash tables for global and parameter entities.
	// 
	typedef xmlHashTable xmlEntitiesTable;
	typedef xmlEntitiesTable * xmlEntitiesTablePtr;
	// 
	// External functions:
	// 
	#ifdef LIBXML_LEGACY_ENABLED
		XMLPUBFUN void xmlInitializePredefinedEntities();
	#endif /* LIBXML_LEGACY_ENABLED */
	XMLPUBFUN xmlEntity * xmlNewEntity(xmlDoc * doc, const xmlChar * name, int type, const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content);
	XMLPUBFUN xmlEntity * xmlAddDocEntity(xmlDoc * doc, const xmlChar * name, int type, const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content);
	XMLPUBFUN xmlEntity * xmlAddDtdEntity(xmlDoc * doc, const xmlChar * name, int type, const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content);
	XMLPUBFUN xmlEntity * FASTCALL  xmlGetPredefinedEntity(const xmlChar * name);
	XMLPUBFUN xmlEntity * FASTCALL xmlGetDocEntity(const xmlDoc * doc, const xmlChar * name);
	XMLPUBFUN xmlEntity * xmlGetDtdEntity(xmlDoc * doc, const xmlChar * name);
	XMLPUBFUN xmlEntity * xmlGetParameterEntity(xmlDoc * doc, const xmlChar * name);
	#ifdef LIBXML_LEGACY_ENABLED
		XMLPUBFUN const xmlChar * xmlEncodeEntities(xmlDoc * doc, const xmlChar * input);
	#endif /* LIBXML_LEGACY_ENABLED */
	XMLPUBFUN xmlChar * xmlEncodeEntitiesReentrant(xmlDoc * doc, const xmlChar * input);
	XMLPUBFUN xmlChar * xmlEncodeSpecialChars(const xmlDoc * doc, const xmlChar * input);
	XMLPUBFUN xmlEntitiesTablePtr xmlCreateEntitiesTable();
	#ifdef LIBXML_TREE_ENABLED
		XMLPUBFUN xmlEntitiesTablePtr xmlCopyEntitiesTable(const xmlEntitiesTablePtr table);
	#endif /* LIBXML_TREE_ENABLED */
	XMLPUBFUN void  xmlFreeEntitiesTable(xmlEntitiesTablePtr table);
	#ifdef LIBXML_OUTPUT_ENABLED
		XMLPUBFUN void  xmlDumpEntitiesTable(xmlBuffer * buf, xmlEntitiesTablePtr table);
		XMLPUBFUN void  xmlDumpEntityDecl(xmlBuffer * buf, xmlEntity * ent);
	#endif /* LIBXML_OUTPUT_ENABLED */
	#ifdef LIBXML_LEGACY_ENABLED
		XMLPUBFUN void  xmlCleanupPredefinedEntities();
	#endif /* LIBXML_LEGACY_ENABLED */
	//
	//#include <libxml/parserInternals.h>
	// 
	// arbitrary depth limit for the XML documents that we allow to
	// process. This is not a limitation of the parser but a safety
	// boundary feature, use XML_PARSE_HUGE option to override it.
	// 
	XMLPUBVAR uint xmlParserMaxDepth;
	// 
	// Maximum size allowed for a single text node when building a tree.
	// This is not a limitation of the parser but a safety boundary feature,
	// use XML_PARSE_HUGE option to override it.
	// Introduced in 2.9.0
	// 
	#define XML_MAX_TEXT_LENGTH 10000000
	// 
	// Descr: Maximum size allowed for a markup identitier
	//   This is not a limitation of the parser but a safety boundary feature,
	//   use XML_PARSE_HUGE option to override it.
	//   Note that with the use of parsing dictionaries overriding the limit
	//   may result in more runtime memory usage in face of "unfriendly' content
	//   Introduced in 2.9.0
	// 
	#define XML_MAX_NAME_LENGTH 50000
	// 
	// Descr: Maximum size allowed by the parser for a dictionary by default
	//   This is not a limitation of the parser but a safety boundary feature,
	//   use XML_PARSE_HUGE option to override it.
	//   Introduced in 2.9.0
	// 
	#define XML_MAX_DICTIONARY_LIMIT 10000000
	// 
	// Descr: Maximum size allowed by the parser for ahead lookup
	//   This is an upper boundary enforced by the parser to avoid bad
	//   behaviour on "unfriendly' content
	//   Introduced in 2.9.0
	// 
	#define XML_MAX_LOOKUP_LIMIT 10000000
	#define XML_MAX_NAMELEN 100 // Identifiers can be longer, but this will be more costly at runtime.
	// 
	// Descr: The parser tries to always have that amount of input ready.
	//   One of the point is providing context when reporting errors.
	// 
	#define INPUT_CHUNK     250
	// 
	// UNICODE version of the macros.
	// 
	/**
	 * IS_BYTE_CHAR:
	 * @c:  an byte value (int)
	 *
	 * Macro to check the following production in the XML spec:
	 *
	 * [2] Char ::= #x9 | #xA | #xD | [#x20...]
	 * any byte character in the accepted range
	 */
	#define IS_BYTE_CHAR(c)  xmlIsChar_ch(c)
	/**
	 * IS_CHAR:
	 * @c:  an UNICODE value (int)
	 *
	 * Macro to check the following production in the XML spec:
	 *
	 * [2] Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD]
	 *       | [#x10000-#x10FFFF]
	 * any Unicode character, excluding the surrogate blocks, FFFE, and FFFF.
	 */
	#define IS_CHAR(c)   xmlIsCharQ(c)
	// 
	// @c: an xmlChar (usually an uchar)
	// Behaves like IS_CHAR on single-byte value
	// 
	#define IS_CHAR_CH(c)  xmlIsChar_ch(c)
	/**
	 * IS_BLANK:
	 * @c:  an UNICODE value (int)
	 *
	 * Macro to check the following production in the XML spec:
	 *
	 * [3] S ::= (#x20 | #x9 | #xD | #xA)+
	 */
	#define IS_BLANK(c)  xmlIsBlankQ(c)
	/**
	 * IS_BLANK_CH:
	 * @c:  an xmlChar value (normally uchar)
	 *
	 * Behaviour same as IS_BLANK
	 */
	#define IS_BLANK_CH(c)  xmlIsBlank_ch(c)
	/**
	 * IS_BASECHAR:
	 * @c:  an UNICODE value (int)
	 *
	 * Macro to check the following production in the XML spec:
	 *
	 * [85] BaseChar ::= ... long list see REC ...
	 */
	#define IS_BASECHAR(c) xmlIsBaseCharQ(c)
	/**
	 * IS_DIGIT:
	 * @c:  an UNICODE value (int)
	 *
	 * Macro to check the following production in the XML spec:
	 *
	 * [88] Digit ::= ... long list see REC ...
	 */
	#define IS_DIGIT(c) xmlIsDigitQ(c)
	/**
	 * IS_DIGIT_CH:
	 * @c:  an xmlChar value (usually an uchar)
	 *
	 * Behaves like IS_DIGIT but with a single byte argument
	 */
	//#define IS_DIGIT_CH(c)  xmlIsDigit_ch(c)
	/**
	 * @c:  an UNICODE value (int)
	 * Macro to check the following production in the XML spec:
	 * [87] CombiningChar ::= ... long list see REC ...
	 */
	#define IS_COMBINING(c) xmlIsCombiningQ(c)
	/**
	 * @c:  an xmlChar (usually an uchar)
	 * Always false (all combining chars > 0xff)
	 */
	#define IS_COMBINING_CH(c) 0
	/**
	 * @c:  an UNICODE value (int)
	 * Macro to check the following production in the XML spec:
	 * [89] Extender ::= #x00B7 | #x02D0 | #x02D1 | #x0387 | #x0640 | #x0E46 | #x0EC6 | #x3005 | [#x3031-#x3035] | [#x309D-#x309E] | [#x30FC-#x30FE]
	 */
	#define IS_EXTENDER(c) xmlIsExtenderQ(c)
	/**
	 * @c:  an xmlChar value (usually an uchar)
	 * Behaves like IS_EXTENDER but with a single-byte argument
	 */
	#define IS_EXTENDER_CH(c)  xmlIsExtender_ch(c)
	/**
	 * @c:  an UNICODE value (int)
	 * Macro to check the following production in the XML spec:
	 * [86] Ideographic ::= [#x4E00-#x9FA5] | #x3007 | [#x3021-#x3029]
	 */
	#define IS_IDEOGRAPHIC(c) xmlIsIdeographicQ(c)
	/**
	 * @c:  an UNICODE value (int)
	 * Macro to check the following production in the XML spec:
	 * [84] Letter ::= BaseChar | Ideographic
	 */
	#define IS_LETTER(c) (IS_BASECHAR(c) || IS_IDEOGRAPHIC(c))
	/**
	 * @c:  an xmlChar value (normally uchar)
	 * Macro behaves like IS_LETTER, but only check base chars
	 */
	#define IS_LETTER_CH(c) xmlIsBaseChar_ch(c)
	/**
	 * @c: an xmlChar value
	 * Macro to check [a-zA-Z]
	 */
	#define IS_ASCII_LETTER(c) (((0x41 <= (c)) && ((c) <= 0x5a)) || ((0x61 <= (c)) && ((c) <= 0x7a)))
	/**
	 * @c: an xmlChar value
	 * Macro to check [0-9]
	 */
	#define IS_ASCII_DIGIT(c)  ((0x30 <= (c)) && ((c) <= 0x39))
	/**
	 * @c:  an UNICODE value (int)
	 * Macro to check the following production in the XML spec:
	 * [13] PubidChar ::= #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%]
	 */
	#define IS_PUBIDCHAR(c) xmlIsPubidCharQ(c)
	/**
	 * @c:  an xmlChar value (normally uchar)
	 * Same as IS_PUBIDCHAR but for single-byte value
	 */
	#define IS_PUBIDCHAR_CH(c) xmlIsPubidChar_ch(c)
	/**
	 * @p:  and UTF8 string pointer
	 * Skips the end of line chars.
	 */
	#define SKIP_EOL(p) if(*(p) == 0x13) { p++; if(*(p) == 0x10) p++; } if(*(p) == 0x10) { p++; if(*(p) == 0x13) p++; }
	// 
	// @p:  and UTF8 string pointer
	// Skips to the next '>' char.
	// 
	#define MOVETO_ENDTAG(p) while((*p) && (*(p) != '>')) (p)++
	// 
	// @p:  and UTF8 string pointer
	// Skips to the next '<' char.
	// 
	#define MOVETO_STARTTAG(p) while((*p) && (*(p) != '<')) (p)++
	// 
	// Global variables used for predefined strings.
	// 
	XMLPUBVAR const xmlChar xmlStringText[];
	XMLPUBVAR const xmlChar xmlStringTextNoenc[];
	XMLPUBVAR const xmlChar xmlStringComment[];
	// 
	// Function to finish the work of the macros where needed.
	// 
	XMLPUBFUN int xmlIsLetter(int c);
	// 
	// Parser context.
	// 
	XMLPUBFUN xmlParserCtxt * xmlCreateFileParserCtxt(const char * filename);
	XMLPUBFUN xmlParserCtxt * xmlCreateURLParserCtxt(const char * filename, int options);
	XMLPUBFUN xmlParserCtxt * xmlCreateMemoryParserCtxt(const char * buffer, int size);
	XMLPUBFUN xmlParserCtxt * xmlCreateEntityParserCtxt(const xmlChar * URL, const xmlChar * ID, const xmlChar * base);
	XMLPUBFUN int xmlSwitchEncoding(xmlParserCtxt * ctxt, xmlCharEncoding enc);
	XMLPUBFUN int xmlSwitchToEncoding(xmlParserCtxt * ctxt, xmlCharEncodingHandler * handler);
	XMLPUBFUN int xmlSwitchInputEncoding(xmlParserCtxt * ctxt, xmlParserInput * input, xmlCharEncodingHandler * handler);
	#ifdef IN_LIBXML
		// internal error reporting 
		XMLPUBFUN void FASTCALL __xmlErrEncoding(xmlParserCtxt * ctxt, xmlParserErrors xmlerr, const char * msg, const xmlChar * str1, const xmlChar * str2);
	#endif
	// 
	// Input Streams.
	// 
	XMLPUBFUN xmlParserInput * xmlNewStringInputStream(xmlParserCtxt * ctxt, const xmlChar * buffer);
	XMLPUBFUN xmlParserInput * xmlNewEntityInputStream(xmlParserCtxt * ctxt, xmlEntity * entity);
	XMLPUBFUN int FASTCALL xmlPushInput(xmlParserCtxt * ctxt, xmlParserInput * input);
	XMLPUBFUN xmlChar FASTCALL xmlPopInput(xmlParserCtxt * ctxt);
	XMLPUBFUN void FASTCALL xmlFreeInputStream(xmlParserInput * input);
	XMLPUBFUN xmlParserInput * xmlNewInputFromFile(xmlParserCtxt * ctxt, const char * filename);
	XMLPUBFUN xmlParserInput * xmlNewInputStream(xmlParserCtxt * ctxt);
	// 
	// Namespaces.
	// 
	XMLPUBFUN xmlChar * xmlSplitQName(xmlParserCtxt * ctxt, const xmlChar * name, xmlChar ** prefix);
	// 
	// Generic production rules.
	// 
	XMLPUBFUN const xmlChar * FASTCALL xmlParseName(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * xmlParseNmtoken(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * xmlParseEntityValue(xmlParserCtxt * ctxt, xmlChar ** orig);
	XMLPUBFUN xmlChar * FASTCALL xmlParseAttValue(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * xmlParseSystemLiteral(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * xmlParsePubidLiteral(xmlParserCtxt * ctxt);
	XMLPUBFUN void FASTCALL xmlParseCharData(xmlParserCtxt * ctxt, int cdata);
	XMLPUBFUN xmlChar * xmlParseExternalID(xmlParserCtxt * ctxt, xmlChar ** publicID, int strict);
	XMLPUBFUN void xmlParseComment(xmlParserCtxt * ctxt);
	XMLPUBFUN const xmlChar * xmlParsePITarget(xmlParserCtxt * ctxt);
	XMLPUBFUN void xmlParsePI(xmlParserCtxt * ctxt);
	XMLPUBFUN void xmlParseNotationDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN void xmlParseEntityDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN int xmlParseDefaultDecl(xmlParserCtxt * ctxt, xmlChar ** value);
	XMLPUBFUN xmlEnumeration * xmlParseNotationType(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlEnumeration * xmlParseEnumerationType(xmlParserCtxt * ctxt);
	XMLPUBFUN int xmlParseEnumeratedType(xmlParserCtxt * ctxt, xmlEnumeration ** tree);
	XMLPUBFUN int xmlParseAttributeType(xmlParserCtxt * ctxt, xmlEnumeration ** tree);
	XMLPUBFUN void xmlParseAttributeListDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlElementContent * xmlParseElementMixedContentDecl(xmlParserCtxt * ctxt, int inputchk);
	XMLPUBFUN xmlElementContent * xmlParseElementChildrenContentDecl(xmlParserCtxt * ctxt, int inputchk);
	XMLPUBFUN int xmlParseElementContentDecl(xmlParserCtxt * ctxt, const xmlChar * name, xmlElementContent ** result);
	XMLPUBFUN int xmlParseElementDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN void xmlParseMarkupDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN int xmlParseCharRef(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlEntity * xmlParseEntityRef(xmlParserCtxt * ctxt);
	XMLPUBFUN void xmlParseReference(xmlParserCtxt * ctxt);
	XMLPUBFUN void xmlParsePEReference(xmlParserCtxt * ctxt);
	XMLPUBFUN void xmlParseDocTypeDecl(xmlParserCtxt * ctxt);
	#ifdef LIBXML_SAX1_ENABLED
		XMLPUBFUN const xmlChar * FASTCALL xmlParseAttribute(xmlParserCtxt * ctxt, xmlChar ** value);
		XMLPUBFUN const xmlChar * xmlParseStartTag(xmlParserCtxt * ctxt);
		XMLPUBFUN void xmlParseEndTag(xmlParserCtxt * ctxt);
	#endif /* LIBXML_SAX1_ENABLED */
	XMLPUBFUN void xmlParseCDSect(xmlParserCtxt * ctxt);
	XMLPUBFUN void FASTCALL xmlParseContent(xmlParserCtxt * ctxt);
	XMLPUBFUN void FASTCALL xmlParseElement(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * xmlParseVersionNum(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * xmlParseVersionInfo(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * xmlParseEncName(xmlParserCtxt * ctxt);
	XMLPUBFUN const xmlChar * xmlParseEncodingDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN int xmlParseSDDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN void xmlParseXMLDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN void xmlParseTextDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN void xmlParseMisc(xmlParserCtxt * ctxt);
	XMLPUBFUN void xmlParseExternalSubset(xmlParserCtxt * ctxt, const xmlChar * ExternalID, const xmlChar * SystemID);

	#define XML_SUBSTITUTE_NONE     0 // If no entities need to be substituted.
	#define XML_SUBSTITUTE_REF      1 // Whether general entities need to be substituted.
	#define XML_SUBSTITUTE_PEREF    2 // Whether parameter entities need to be substituted.
	#define XML_SUBSTITUTE_BOTH     3 // Both general and parameter entities need to be substituted.

	XMLPUBFUN xmlChar * FASTCALL xmlStringDecodeEntities(xmlParserCtxt * ctxt, const xmlChar * str, int what, xmlChar end, xmlChar end2, xmlChar end3);
	XMLPUBFUN xmlChar * xmlStringLenDecodeEntities(xmlParserCtxt * ctxt, const xmlChar * str, int len, int what, xmlChar end, xmlChar end2, xmlChar end3);
	// 
	// Generated by MACROS on top of parser.c c.f. PUSH_AND_POP.
	// 
	XMLPUBFUN int nodePush(xmlParserCtxt * ctxt, xmlNode * value);
	XMLPUBFUN xmlNode * nodePop(xmlParserCtxt * ctxt);
	XMLPUBFUN int FASTCALL inputPush(xmlParserCtxt * ctxt, xmlParserInput * value);
	XMLPUBFUN xmlParserInput * inputPop(xmlParserCtxt * ctxt);
	XMLPUBFUN const xmlChar * namePop(xmlParserCtxt * ctxt);
	XMLPUBFUN int namePush(xmlParserCtxt * ctxt, const xmlChar * value);
	// 
	// other commodities shared between parser.c and parserInternals.
	// 
	XMLPUBFUN int FASTCALL xmlSkipBlankChars(xmlParserCtxt * ctxt);
	XMLPUBFUN int FASTCALL xmlStringCurrentChar(xmlParserCtxt * ctxt, const xmlChar * cur, int * len);
	XMLPUBFUN void FASTCALL xmlParserHandlePEReference(xmlParserCtxt * ctxt);
	XMLPUBFUN int xmlCheckLanguageID(const xmlChar * lang);
	// 
	// Really core function shared with HTML parser.
	// 
	XMLPUBFUN int FASTCALL xmlCurrentChar(xmlParserCtxt * ctxt, int * len);
	XMLPUBFUN int FASTCALL xmlCopyCharMultiByte(xmlChar * out, int val);
	XMLPUBFUN int FASTCALL xmlCopyChar(int len, xmlChar * out, int val);
	XMLPUBFUN void FASTCALL xmlNextChar(xmlParserCtxt * ctxt);
	XMLPUBFUN void FASTCALL xmlParserInputShrink(xmlParserInput * in);
	#ifdef LIBXML_HTML_ENABLED
		// 
		// Actually comes from the HTML parser but launched from the init stuff.
		// 
		XMLPUBFUN void htmlInitAutoClose();
		XMLPUBFUN htmlParserCtxt * htmlCreateFileParserCtxt(const char * filename, const char * encoding);
	#endif
	// 
	// Specific function to keep track of entities references and used by the XSLT debugger.
	// 
	#ifdef LIBXML_LEGACY_ENABLED
		// 
		// @ent: the entity
		// @firstNode:  the fist node in the chunk
		// @lastNode:  the last nod in the chunk
		// 
		// Callback function used when one needs to be able to track back the
		// provenance of a chunk of nodes inherited from an entity replacement.
		// 
		typedef void (*xmlEntityReferenceFunc)(xmlEntity * ent, xmlNode * firstNode, xmlNode * lastNode);
		XMLPUBFUN void xmlSetEntityReferenceFunc(xmlEntityReferenceFunc func);
		XMLPUBFUN xmlChar * xmlParseQuotedString(xmlParserCtxt * ctxt);
		XMLPUBFUN void xmlParseNamespace(xmlParserCtxt * ctxt);
		XMLPUBFUN xmlChar * xmlNamespaceParseNSDef(xmlParserCtxt * ctxt);
		XMLPUBFUN xmlChar * xmlScanName(xmlParserCtxt * ctxt);
		XMLPUBFUN xmlChar * xmlNamespaceParseNCName(xmlParserCtxt * ctxt);
		XMLPUBFUN void xmlParserHandleReference(xmlParserCtxt * ctxt);
		XMLPUBFUN xmlChar * xmlNamespaceParseQName(xmlParserCtxt * ctxt, xmlChar ** prefix);
		// 
		// Entities
		// 
		XMLPUBFUN xmlChar * xmlDecodeEntities(xmlParserCtxt * ctxt, int len, int what, xmlChar end, xmlChar end2, xmlChar end3);
		XMLPUBFUN void xmlHandleEntity(xmlParserCtxt * ctxt, xmlEntity * entity);
	#endif /* LIBXML_LEGACY_ENABLED */
	#ifdef IN_LIBXML
		// 
		// internal only
		// 
		XMLPUBFUN void FASTCALL xmlErrMemory(xmlParserCtxt * ctxt, const char * extra);
	#endif
	//
	XMLPUBFUN xmlMutex * xmlNewMutex();
	XMLPUBFUN void FASTCALL xmlMutexLock(xmlMutex * tok);
	XMLPUBFUN void FASTCALL xmlMutexUnlock(xmlMutex * tok);
	XMLPUBFUN void xmlFreeMutex(xmlMutex * tok);
	XMLPUBFUN xmlRMutex * xmlNewRMutex();
	XMLPUBFUN void FASTCALL xmlRMutexLock(xmlRMutex * tok);
	XMLPUBFUN void FASTCALL xmlRMutexUnlock(xmlRMutex * tok);
	XMLPUBFUN void xmlFreeRMutex(xmlRMutex * tok);
	// 
	// Library wide APIs.
	// 
	XMLPUBFUN void xmlInitThreads();
	XMLPUBFUN void xmlLockLibrary();
	XMLPUBFUN void xmlUnlockLibrary();
	XMLPUBFUN int xmlGetThreadId();
	XMLPUBFUN int xmlIsMainThread();
	XMLPUBFUN void xmlCleanupThreads();
	XMLPUBFUN xmlGlobalState * xmlGetGlobalState();
	#if defined(HAVE_WIN32_THREADS) && !defined(HAVE_COMPILER_TLS) && defined(LIBXML_STATIC_FOR_DLL)
		int xmlDllMain(void *hinstDLL, unsigned long fdwReason, void *lpvReserved);
	#endif
	#ifdef LIBXML_XPTR_ENABLED
		/**
		 * Various defines for the various Link properties.
		 *
		 * NOTE: the link detection layer will try to resolve QName expansion
		 *  of namespaces. If "foo" is the prefix for "http://foo.com/"
		 *  then the link detection layer will expand role="foo:myrole"
		 *  to "http://foo.com/:myrole".
		 * NOTE: the link detection layer will expand URI-Refences found on
		 *  href attributes by using the base mechanism if found.
		 */
		typedef xmlChar * xlinkHRef;
		typedef xmlChar * xlinkRole;
		typedef xmlChar * xlinkTitle;

		enum xlinkType {
			XLINK_TYPE_NONE = 0,
			XLINK_TYPE_SIMPLE,
			XLINK_TYPE_EXTENDED,
			XLINK_TYPE_EXTENDED_SET
		};

		enum xlinkShow {
			XLINK_SHOW_NONE = 0,
			XLINK_SHOW_NEW,
			XLINK_SHOW_EMBED,
			XLINK_SHOW_REPLACE
		};

		enum xlinkActuate {
			XLINK_ACTUATE_NONE = 0,
			XLINK_ACTUATE_AUTO,
			XLINK_ACTUATE_ONREQUEST
		};
		/**
		 * xlinkNodeDetectFunc:
		 * @ctx:  user data pointer
		 * @node:  the node to check
		 *
		 * This is the prototype for the link detection routine.
		 * It calls the default link detection callbacks upon link detection.
		 */
		typedef void (*xlinkNodeDetectFunc)(void * ctx, xmlNode * pNode);
		// 
		// The link detection module interact with the upper layers using
		// a set of callback registered at parsing time.
		//
		/**
		 * xlinkSimpleLinkFunk:
		 * @ctx:  user data pointer
		 * @node:  the node carrying the link
		 * @href:  the target of the link
		 * @role:  the role string
		 * @title:  the link title
		 *
		 * This is the prototype for a simple link detection callback.
		 */
		typedef void (*xlinkSimpleLinkFunk)(void * ctx, xmlNode * pNode, const xlinkHRef href, const xlinkRole role, const xlinkTitle title);
		/**
		 * xlinkExtendedLinkFunk:
		 * @ctx:  user data pointer
		 * @node:  the node carrying the link
		 * @nbLocators: the number of locators detected on the link
		 * @hrefs:  pointer to the array of locator hrefs
		 * @roles:  pointer to the array of locator roles
		 * @nbArcs: the number of arcs detected on the link
		 * @from:  pointer to the array of source roles found on the arcs
		 * @to:  pointer to the array of target roles found on the arcs
		 * @show:  array of values for the show attributes found on the arcs
		 * @actuate:  array of values for the actuate attributes found on the arcs
		 * @nbTitles: the number of titles detected on the link
		 * @title:  array of titles detected on the link
		 * @langs:  array of xml:lang values for the titles
		 *
		 * This is the prototype for a extended link detection callback.
		 */
		typedef void (*xlinkExtendedLinkFunk)(void * ctx, xmlNode * pNode, int nbLocators, const xlinkHRef * hrefs, const xlinkRole * roles,
			int nbArcs, const xlinkRole * from, const xlinkRole * to, xlinkShow * show, xlinkActuate * actuate, int nbTitles, const xlinkTitle * titles, const xmlChar ** langs);
		/**
		 * xlinkExtendedLinkSetFunk:
		 * @ctx:  user data pointer
		 * @node:  the node carrying the link
		 * @nbLocators: the number of locators detected on the link
		 * @hrefs:  pointer to the array of locator hrefs
		 * @roles:  pointer to the array of locator roles
		 * @nbTitles: the number of titles detected on the link
		 * @title:  array of titles detected on the link
		 * @langs:  array of xml:lang values for the titles
		 *
		 * This is the prototype for a extended link set detection callback.
		 */
		typedef void (*xlinkExtendedLinkSetFunk)(void * ctx, xmlNode * pNode, int nbLocators, const xlinkHRef * hrefs,
			const xlinkRole * roles, int nbTitles, const xlinkTitle * titles, const xmlChar ** langs);
		// 
		// This is the structure containing a set of Links detection callbacks.
		// 
		// There is no default xlink callbacks, if one want to get link
		// recognition activated, those call backs must be provided before parsing.
		// 
		typedef struct _xlinkHandler xlinkHandler;
		typedef xlinkHandler * xlinkHandlerPtr;

		struct _xlinkHandler {
			xlinkSimpleLinkFunk simple;
			xlinkExtendedLinkFunk extended;
			xlinkExtendedLinkSetFunk set;
		};
		// 
		// The default detection routine, can be overridden, they call the default detection callbacks.
		// 
		XMLPUBFUN xlinkNodeDetectFunc xlinkGetDefaultDetect();
		XMLPUBFUN void xlinkSetDefaultDetect(xlinkNodeDetectFunc func);
		// 
		// Routines to set/get the default handlers.
		// 
		XMLPUBFUN xlinkHandlerPtr xlinkGetDefaultHandler();
		XMLPUBFUN void xlinkSetDefaultHandler(xlinkHandlerPtr handler);
		// 
		// Link detection module itself.
		// 
		XMLPUBFUN xlinkType xlinkIsLink(xmlDoc * doc, xmlNode * pNode);
	#endif

	//#ifdef __cplusplus
	//}
	//#endif
	//
	//#include <libxml/SAX.h>
	#ifdef LIBXML_LEGACY_ENABLED
		//#ifdef __cplusplus
		//extern "C" {
		//#endif
			XMLPUBFUN const xmlChar * getPublicId(void *ctx);
			XMLPUBFUN const xmlChar * getSystemId(void *ctx);
			XMLPUBFUN void setDocumentLocator(void *ctx, xmlSAXLocator * loc);
			XMLPUBFUN int getLineNumber(void *ctx);
			XMLPUBFUN int getColumnNumber(void *ctx);
			XMLPUBFUN int isStandalone(void *ctx);
			XMLPUBFUN int hasInternalSubset(void *ctx);
			XMLPUBFUN int hasExternalSubset(void *ctx);
			XMLPUBFUN void internalSubset(void *ctx, const xmlChar *name, const xmlChar *ExternalID, const xmlChar *SystemID);
			XMLPUBFUN void externalSubset(void *ctx, const xmlChar *name, const xmlChar *ExternalID, const xmlChar *SystemID);
			XMLPUBFUN xmlEntity * getEntity(void *ctx, const xmlChar *name);
			XMLPUBFUN xmlEntity * getParameterEntity(void *ctx, const xmlChar *name);
			XMLPUBFUN xmlParserInput * resolveEntity(void *ctx, const xmlChar *publicId, const xmlChar *systemId);
			XMLPUBFUN void entityDecl(void *ctx, const xmlChar *name, int type, const xmlChar *publicId, const xmlChar *systemId, xmlChar *content);
			XMLPUBFUN void attributeDecl(void *ctx, const xmlChar *elem, const xmlChar *fullname, int type, int def, const xmlChar *defaultValue, xmlEnumeration * tree);
			XMLPUBFUN void elementDecl(void *ctx, const xmlChar *name, int type, xmlElementContent * content);
			XMLPUBFUN void notationDecl(void *ctx, const xmlChar *name, const xmlChar *publicId, const xmlChar *systemId);
			XMLPUBFUN void unparsedEntityDecl(void *ctx, const xmlChar *name, const xmlChar *publicId, const xmlChar *systemId, const xmlChar *notationName);
			XMLPUBFUN void startDocument(void *ctx);
			XMLPUBFUN void endDocument(void *ctx);
			XMLPUBFUN void attribute(void *ctx, const xmlChar *fullname, const xmlChar *value);
			XMLPUBFUN void startElement(void *ctx, const xmlChar *fullname, const xmlChar **atts);
			XMLPUBFUN void endElement(void *ctx, const xmlChar *name);
			XMLPUBFUN void reference(void *ctx, const xmlChar *name);
			XMLPUBFUN void characters(void *ctx, const xmlChar *ch, int len);
			XMLPUBFUN void ignorableWhitespace(void *ctx, const xmlChar *ch, int len);
			XMLPUBFUN void processingInstruction(void *ctx, const xmlChar *target, const xmlChar *data);
			XMLPUBFUN void globalNamespace(void *ctx, const xmlChar *href, const xmlChar *prefix);
			XMLPUBFUN void setNamespace(void *ctx, const xmlChar *name);
			XMLPUBFUN xmlNs * getNamespace(void *ctx);
			XMLPUBFUN int checkNamespace(void *ctx, xmlChar *nameSpace);
			XMLPUBFUN void namespaceDecl(void *ctx, const xmlChar *href, const xmlChar *prefix);
			XMLPUBFUN void comment(void *ctx, const xmlChar *value);
			XMLPUBFUN void cdataBlock(void *ctx, const xmlChar *value, int len);
			#ifdef LIBXML_SAX1_ENABLED
				XMLPUBFUN void initxmlDefaultSAXHandler(xmlSAXHandlerV1 *hdlr, int warning);
				#ifdef LIBXML_HTML_ENABLED 
					XMLPUBFUN void inithtmlDefaultSAXHandler(xmlSAXHandlerV1 *hdlr);
				#endif
				#ifdef LIBXML_DOCB_ENABLED
					XMLPUBFUN void initdocbDefaultSAXHandler(xmlSAXHandlerV1 *hdlr);
				#endif
			#endif /* LIBXML_SAX1_ENABLED */
		//#ifdef __cplusplus
		//}
		//#endif
	#endif /* LIBXML_LEGACY_ENABLED */
	//#include <libxml/SAX2.h>
	//#ifdef __cplusplus
	//extern "C" {
	//#endif
		XMLPUBFUN const xmlChar * xmlSAX2GetPublicId(void * ctx);
		XMLPUBFUN const xmlChar * xmlSAX2GetSystemId(void * ctx);
		XMLPUBFUN void xmlSAX2SetDocumentLocator(void * ctx, xmlSAXLocator * loc);
		XMLPUBFUN int xmlSAX2GetLineNumber(void * ctx);
		XMLPUBFUN int xmlSAX2GetColumnNumber(void * ctx);
		XMLPUBFUN int xmlSAX2IsStandalone(void * ctx);
		XMLPUBFUN int xmlSAX2HasInternalSubset(void * ctx);
		XMLPUBFUN int xmlSAX2HasExternalSubset(void * ctx);
		XMLPUBFUN void xmlSAX2InternalSubset(void * ctx, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID);
		XMLPUBFUN void xmlSAX2ExternalSubset(void * ctx, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID);
		XMLPUBFUN xmlEntity * xmlSAX2GetEntity(void * ctx, const xmlChar * name);
		XMLPUBFUN xmlEntity * xmlSAX2GetParameterEntity(void * ctx, const xmlChar * name);
		XMLPUBFUN xmlParserInput * xmlSAX2ResolveEntity(void * ctx, const xmlChar * publicId, const xmlChar * systemId);
		XMLPUBFUN void xmlSAX2EntityDecl(void * ctx, const xmlChar * name, int type, const xmlChar * publicId, const xmlChar * systemId, xmlChar * content);
		XMLPUBFUN void xmlSAX2AttributeDecl(void * ctx, const xmlChar * elem, const xmlChar * fullname, int type, int def, const xmlChar * defaultValue, xmlEnumeration * tree);
		XMLPUBFUN void xmlSAX2ElementDecl(void * ctx, const xmlChar * name, int type, xmlElementContent * content);
		XMLPUBFUN void xmlSAX2NotationDecl(void * ctx, const xmlChar * name, const xmlChar * publicId, const xmlChar * systemId);
		XMLPUBFUN void xmlSAX2UnparsedEntityDecl(void * ctx, const xmlChar * name, const xmlChar * publicId, const xmlChar * systemId, const xmlChar * notationName);
		XMLPUBFUN void xmlSAX2StartDocument(void * ctx);
		XMLPUBFUN void xmlSAX2EndDocument(void * ctx);
		#if defined(LIBXML_SAX1_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_WRITER_ENABLED) || defined(LIBXML_DOCB_ENABLED) || defined(LIBXML_LEGACY_ENABLED)
			XMLPUBFUN void xmlSAX2StartElement(void * ctx, const xmlChar * fullname, const xmlChar ** atts);
			XMLPUBFUN void xmlSAX2EndElement(void * ctx, const xmlChar * name);
		#endif
		XMLPUBFUN void xmlSAX2StartElementNs(void * ctx, const xmlChar * localname, const xmlChar * prefix,
			const xmlChar * URI, int nb_namespaces, const xmlChar ** namespaces, int nb_attributes, int nb_defaulted, const xmlChar ** attributes);
		XMLPUBFUN void xmlSAX2EndElementNs(void * ctx, const xmlChar * localname, const xmlChar * prefix, const xmlChar * URI);
		XMLPUBFUN void xmlSAX2Reference(void * ctx, const xmlChar * name);
		XMLPUBFUN void xmlSAX2Characters(void * ctx, const xmlChar * ch, int len);
		XMLPUBFUN void xmlSAX2IgnorableWhitespace(void * ctx, const xmlChar * ch, int len);
		XMLPUBFUN void xmlSAX2ProcessingInstruction(void * ctx, const xmlChar * target, const xmlChar * data);
		XMLPUBFUN void xmlSAX2Comment(void * ctx, const xmlChar * value);
		XMLPUBFUN void xmlSAX2CDataBlock(void * ctx, const xmlChar * value, int len);
		#ifdef LIBXML_SAX1_ENABLED
			XMLPUBFUN int xmlSAXDefaultVersion(int version);
		#endif
		XMLPUBFUN int xmlSAXVersion(xmlSAXHandler * hdlr, int version);
		XMLPUBFUN void xmlSAX2InitDefaultSAXHandler(xmlSAXHandler * hdlr, int warning);
		#ifdef LIBXML_HTML_ENABLED
			XMLPUBFUN void xmlSAX2InitHtmlDefaultSAXHandler(xmlSAXHandler * hdlr);
			XMLPUBFUN void htmlDefaultSAXHandlerInit();
		#endif
		#ifdef LIBXML_DOCB_ENABLED
			XMLPUBFUN void xmlSAX2InitDocbDefaultSAXHandler(xmlSAXHandler * hdlr);
			XMLPUBFUN void docbDefaultSAXHandlerInit();
		#endif
		XMLPUBFUN void xmlDefaultSAXHandlerInit();
	//#ifdef __cplusplus
	//}
	//#endif
#endif // } IN_LIBXML
	//#include <libxml/xpath.h>
	#if defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
		//#ifdef __cplusplus
		//extern "C" {
		//#endif
	#endif
	#ifdef LIBXML_XPATH_ENABLED
		struct xmlXPathContext;
		struct xmlXPathParserContext;
		//typedef xmlXPathContext * xmlXPathContextPtr;
		//typedef xmlXPathParserContext * xmlXPathParserContextPtr;
		// 
		// Descr: The set of XPath error codes.
		// 
		enum xmlXPathError {
			XPATH_EXPRESSION_OK = 0,
			XPATH_NUMBER_ERROR,
			XPATH_UNFINISHED_LITERAL_ERROR,
			XPATH_START_LITERAL_ERROR,
			XPATH_VARIABLE_REF_ERROR,
			XPATH_UNDEF_VARIABLE_ERROR,
			XPATH_INVALID_PREDICATE_ERROR,
			XPATH_EXPR_ERROR,
			XPATH_UNCLOSED_ERROR,
			XPATH_UNKNOWN_FUNC_ERROR,
			XPATH_INVALID_OPERAND,
			XPATH_INVALID_TYPE,
			XPATH_INVALID_ARITY,
			XPATH_INVALID_CTXT_SIZE,
			XPATH_INVALID_CTXT_POSITION,
			XPATH_MEMORY_ERROR,
			XPTR_SYNTAX_ERROR,
			XPTR_RESOURCE_ERROR,
			XPTR_SUB_RESOURCE_ERROR,
			XPATH_UNDEF_PREFIX_ERROR,
			XPATH_ENCODING_ERROR,
			XPATH_INVALID_CHAR_ERROR,
			XPATH_INVALID_CTXT,
			XPATH_STACK_ERROR,
			XPATH_FORBID_VARIABLE_ERROR
		};
		/*
		 * A node-set (an unordered collection of nodes without duplicates).
		 */
		//typedef struct _xmlNodeSet xmlNodeSet;
		struct xmlNodeSet {
			int nodeNr;			/* number of nodes in the set */
			int nodeMax;		/* size of the array as allocated */
			xmlNode ** PP_NodeTab; // array of nodes in no particular order 
			/* @@ with_ns to check wether namespace nodes should be looked at @@ */
		};
		//typedef xmlNodeSet * xmlNodeSetPtr;
		/*
		 * An expression is evaluated to yield an object, which
		 * has one of the following four basic types:
		 * - node-set
		 * - boolean
		 * - number
		 * - string
		 *
		 * @@ XPointer will add more types !
		 */
		enum xmlXPathObjectType {
			XPATH_UNDEFINED = 0,
			XPATH_NODESET = 1,
			XPATH_BOOLEAN = 2,
			XPATH_NUMBER = 3,
			XPATH_STRING = 4,
			XPATH_POINT = 5,
			XPATH_RANGE = 6,
			XPATH_LOCATIONSET = 7,
			XPATH_USERS = 8,
			XPATH_XSLT_TREE = 9  /* An XSLT value tree, non modifiable */
		};

		struct xmlXPathObject {
			xmlXPathObjectType type;
			xmlNodeSet * nodesetval;
			int    boolval;
			double floatval;
			xmlChar * stringval;
			void * user;
			int    index;
			void * user2;
			int    index2;
		};

		//typedef xmlXPathObject * xmlXPathObjectPtr;
		/**
		 * xmlXPathConvertFunc:
		 * @obj:  an XPath object
		 * @type:  the number of the target type
		 *
		 * A conversion function is associated to a type and used to cast
		 * the new type to primitive values.
		 *
		 * Returns -1 in case of error, 0 otherwise
		 */
		typedef int (*xmlXPathConvertFunc)(xmlXPathObject * obj, int type);
		/*
		 * Extra type: a name and a conversion function.
		 */
		//typedef struct _xmlXPathType xmlXPathType;
		struct xmlXPathType {
			const xmlChar * name; /* the type name */
			xmlXPathConvertFunc func; /* the conversion function */
		};
		//typedef xmlXPathType * xmlXPathTypePtr;
		/*
		 * Extra variable: a name and a value.
		 */
		//typedef struct _xmlXPathVariable xmlXPathVariable;
		struct xmlXPathVariable {
			const xmlChar * name; // the variable name 
			xmlXPathObject * value; // the value 
		};
		//typedef xmlXPathVariable *xmlXPathVariablePtr;
		/**
		 * xmlXPathEvalFunc:
		 * @ctxt: an XPath parser context
		 * @nargs: the number of arguments passed to the function
		 *
		 * An XPath evaluation function, the parameters are on the XPath context stack.
		 */
		typedef void (*xmlXPathEvalFunc)(xmlXPathParserContext * ctxt, int nargs);
		/*
		 * Extra function: a name and a evaluation function.
		 */
		struct xmlXPathFunct {
			const xmlChar * name; // the function name 
			xmlXPathEvalFunc func; // the evaluation function 
		};

		typedef xmlXPathFunct *xmlXPathFuncPtr;
		/**
		 * xmlXPathAxisFunc:
		 * @ctxt:  the XPath interpreter context
		 * @cur:  the previous node being explored on that axis
		 *
		 * An axis traversal function. To traverse an axis, the engine calls
		 * the first time with cur == NULL and repeat until the function returns
		 * NULL indicating the end of the axis traversal.
		 *
		 * Returns the next node in that axis or NULL if at the end of the axis.
		 */
		typedef xmlXPathObject * (*xmlXPathAxisFunc)(xmlXPathParserContext * ctxt, xmlXPathObject * cur);
		/*
		 * Extra axis: a name and an axis function.
		 */
		//typedef struct _xmlXPathAxis xmlXPathAxis;
		struct xmlXPathAxis {
			const xmlChar * name;  // the axis name 
			xmlXPathAxisFunc func; // the search function 
		};
		//typedef xmlXPathAxis * xmlXPathAxisPtr;
		/**
		 * xmlXPathFunction:
		 * @ctxt:  the XPath interprestation context
		 * @nargs:  the number of arguments
		 *
		 * An XPath function.
		 * The arguments (if any) are popped out from the context stack
		 * and the result is pushed on the stack.
		 */
		typedef void (*xmlXPathFunction)(xmlXPathParserContext * ctxt, int nargs);
		/*
		 * Function and Variable Lookup.
		 */
		/**
		 * xmlXPathVariableLookupFunc:
		 * @ctxt:  an XPath context
		 * @name:  name of the variable
		 * @ns_uri:  the namespace name hosting this variable
		 *
		 * Prototype for callbacks used to plug variable lookup in the XPath
		 * engine.
		 *
		 * Returns the XPath object value or NULL if not found.
		 */
		typedef xmlXPathObject * (*xmlXPathVariableLookupFunc)(void *ctxt, const xmlChar *name, const xmlChar *ns_uri);
		/**
		 * xmlXPathFuncLookupFunc:
		 * @ctxt:  an XPath context
		 * @name:  name of the function
		 * @ns_uri:  the namespace name hosting this function
		 *
		 * Prototype for callbacks used to plug function lookup in the XPath
		 * engine.
		 *
		 * Returns the XPath function or NULL if not found.
		 */
		typedef xmlXPathFunction (*xmlXPathFuncLookupFunc)(void *ctxt, const xmlChar *name, const xmlChar *ns_uri);
		/**
		 * xmlXPathFlags:
		 * Flags for XPath engine compilation and runtime
		 */
		#define XML_XPATH_CHECKNS (1<<0) // check namespaces at compilation
		#define XML_XPATH_NOVAR	  (1<<1) // forbid variables in expression
		/**
		 * xmlXPathContext:
		 *
		 * Expression evaluation occurs with respect to a context.
		 * he context consists of:
		 *  - a node (the context node)
		 *  - a node list (the context node list)
		 *  - a set of variable bindings
		 *  - a function library
		 *  - the set of namespace declarations in scope for the expression
		 * Following the switch to hash tables, this need to be trimmed up at
		 * the next binary incompatible release.
		 * The node may be modified when the context is passed to libxml2
		 * for an XPath evaluation so you may need to initialize it again
		 * before the next call.
		 */
		struct xmlXPathContext {
			xmlDoc  * doc;  // The current document 
			xmlNode * P_Node; // The current node 
			int nb_variables_unused;		/* unused (hash table) */
			int max_variables_unused;		/* unused (hash table) */
			xmlHashTable * varHash;		/* Hash table of defined variables */
			int nb_types;			/* number of defined types */
			int max_types;			/* max number of types */
			xmlXPathType * types; // Array of defined types 
			int nb_funcs_unused;		/* unused (hash table) */
			int max_funcs_unused;		/* unused (hash table) */
			xmlHashTable * funcHash;		/* Hash table of defined funcs */
			int nb_axis;			/* number of defined axis */
			int max_axis;			/* max number of axis */
			xmlXPathAxis * axis; // Array of defined axis 
			// the namespace nodes of the context node 
			xmlNs ** namespaces; // Array of namespaces 
			int nsNr;				/* number of namespace in scope */
			void *user;				/* function to free */
			//
			// extra variables 
			//
			int contextSize;			/* the context size */
			int proximityPosition;		/* the proximity position */
			//
			// extra stuff for XPointer 
			//
			int xptr;				/* is this an XPointer context? */
			xmlNode * here;			/* for here() */
			xmlNode * origin;			/* for origin() */
			/* the set of namespace declarations in scope for the expression */
			xmlHashTable * nsHash;		/* The namespaces hash table */
			xmlXPathVariableLookupFunc varLookupFunc; /* variable lookup func */
			void *varLookupData;		/* variable lookup data */
			/* Possibility to link in an extra item */
			void *extra; /* needed for XSLT */
			/* The function name and URI when calling a function */
			const xmlChar *function;
			const xmlChar *functionURI;
			/* function lookup function and data */
			xmlXPathFuncLookupFunc funcLookupFunc; /* function lookup func */
			void *funcLookupData;		/* function lookup data */
			/* temporary namespace lists kept for walking the namespace axis */
			xmlNs ** tmpNsList; // Array of namespaces 
			int tmpNsNr;			/* number of namespaces in scope */
			/* error reporting mechanism */
			void *userData; /* user specific data block */
			xmlStructuredErrorFunc error; /* the callback in case of errors */
			xmlError lastError;			/* the last error */
			xmlNode * debugNode;		/* the source node XSLT */
			/* dictionary */
			xmlDict * dict;			/* dictionary if any */
			int flags;				/* flags to control compilation */
			void * cache; // Cache for reusal of XPath objects 
		};
		/*
		 * The structure of a compiled expression form is not public.
		 */
		typedef struct _xmlXPathCompExpr xmlXPathCompExpr;
		typedef xmlXPathCompExpr *xmlXPathCompExprPtr;
		// 
		// Descr: An XPath parser context. It contains pure parsing informations,
		//   an xmlXPathContext, and the stack of objects.
		// 
		struct xmlXPathParserContext {
			const xmlChar * cur;  // the current char being parsed 
			const xmlChar * base; // the full expression 
			int    error;               // error code 
			xmlXPathContext * context; // the evaluation context
			xmlXPathObject * value;   // the current value 
			int    valueNr;             // number of values stacked 
			int    valueMax;            // max number of values stacked 
			xmlXPathObject ** valueTab; // stack of values 
			xmlXPathCompExprPtr comp;     // the precompiled expression 
			int    xptr;                  // it this an XPointer expression 
			xmlNode * ancestor;           // used for walking preceding axis 
			int    valueFrame;            // used to limit Pop on the stack 
		};
		// 
		// Public API
		// 
		/**
		 * Objects and Nodesets handling
		 */
		XMLPUBVAR double xmlXPathNAN;
		XMLPUBVAR double xmlXPathPINF;
		XMLPUBVAR double xmlXPathNINF;
		/* These macros may later turn into functions */
		/**
		 * xmlXPathNodeSetGetLength:
		 * @ns:  a node-set
		 *
		 * Implement a functionality similar to the DOM NodeList.length.
		 *
		 * Returns the number of nodes in the node-set.
		 */
		#define xmlXPathNodeSetGetLength(ns) ((ns) ? (ns)->nodeNr : 0)
		/**
		 * xmlXPathNodeSetItem:
		 * @ns:  a node-set
		 * @index:  index of a node in the set
		 *
		 * Implements a functionality similar to the DOM NodeList.item().
		 *
		 * Returns the xmlNodePtr at the given @index in @ns or NULL if
		 *    @index is out of range (0 to length-1)
		 */
		#define xmlXPathNodeSetItem(ns, index) ((((ns) != NULL) && ((index) >= 0) && ((index) < (ns)->nodeNr)) ? (ns)->PP_NodeTab[(index)] : NULL)
		/**
		 * xmlXPathNodeSetIsEmpty:
		 * @ns: a node-set
		 *
		 * Checks whether @ns is empty or not.
		 *
		 * Returns %TRUE if @ns is an empty node-set.
		 */
		#define xmlXPathNodeSetIsEmpty(ns) (((ns) == NULL) || ((ns)->nodeNr == 0) || ((ns)->PP_NodeTab == NULL))

		XMLPUBFUN void FASTCALL xmlXPathFreeObject(xmlXPathObject * obj);
		XMLPUBFUN xmlNodeSet * xmlXPathNodeSetCreate(xmlNode * val);
		XMLPUBFUN void xmlXPathFreeNodeSetList(xmlXPathObject * obj);
		XMLPUBFUN void xmlXPathFreeNodeSet(xmlNodeSet * obj);
		XMLPUBFUN xmlXPathObject * xmlXPathObjectCopy(xmlXPathObject * val);
		XMLPUBFUN int xmlXPathCmpNodes(xmlNode * node1, xmlNode * node2);
		// 
		// Conversion functions to basic types.
		// 
		XMLPUBFUN int xmlXPathCastNumberToBoolean(double val);
		XMLPUBFUN int xmlXPathCastStringToBoolean(const xmlChar * val);
		XMLPUBFUN int xmlXPathCastNodeSetToBoolean(const xmlNodeSet * ns);
		XMLPUBFUN int xmlXPathCastToBoolean	(xmlXPathObject * val);
		XMLPUBFUN double xmlXPathCastBooleanToNumber(int val);
		XMLPUBFUN double xmlXPathCastStringToNumber(const xmlChar * val);
		XMLPUBFUN double xmlXPathCastNodeToNumber(xmlNode * pNode);
		XMLPUBFUN double xmlXPathCastNodeSetToNumber(xmlNodeSet * ns);
		XMLPUBFUN double xmlXPathCastToNumber(xmlXPathObject * val);
		XMLPUBFUN xmlChar * xmlXPathCastBooleanToString(int val);
		XMLPUBFUN xmlChar * xmlXPathCastNumberToString(double val);
		XMLPUBFUN xmlChar * xmlXPathCastNodeToString(xmlNode * pNode);
		XMLPUBFUN xmlChar * xmlXPathCastNodeSetToString(xmlNodeSet * ns);
		XMLPUBFUN xmlChar * xmlXPathCastToString(xmlXPathObject * val);
		XMLPUBFUN xmlXPathObject * xmlXPathConvertBoolean(xmlXPathObject * val);
		XMLPUBFUN xmlXPathObject * xmlXPathConvertNumber(xmlXPathObject * val);
		XMLPUBFUN xmlXPathObject * xmlXPathConvertString(xmlXPathObject * val);
		// 
		// Context handling.
		// 
		XMLPUBFUN xmlXPathContext * xmlXPathNewContext(xmlDoc * doc);
		XMLPUBFUN void xmlXPathFreeContext(xmlXPathContext * ctxt);
		XMLPUBFUN int xmlXPathContextSetCache(xmlXPathContext * ctxt, int active, int value, int options);
		// 
		// Evaluation functions.
		// 
		XMLPUBFUN long xmlXPathOrderDocElems(xmlDoc * doc);
		XMLPUBFUN int xmlXPathSetContextNode(xmlNode * pNode, xmlXPathContext * ctx);
		XMLPUBFUN xmlXPathObject * xmlXPathNodeEval(xmlNode * pNode, const xmlChar * str, xmlXPathContext * ctx);
		XMLPUBFUN xmlXPathObject * xmlXPathEval(const xmlChar *str, xmlXPathContext * ctx);
		XMLPUBFUN xmlXPathObject * xmlXPathEvalExpression(const xmlChar *str, xmlXPathContext * ctxt);
		XMLPUBFUN int xmlXPathEvalPredicate(xmlXPathContext * ctxt, xmlXPathObject * res);
		/**
		 * Separate compilation/evaluation entry points.
		 */
		XMLPUBFUN xmlXPathCompExprPtr xmlXPathCompile(const xmlChar *str);
		XMLPUBFUN xmlXPathCompExprPtr xmlXPathCtxtCompile(xmlXPathContext * ctxt, const xmlChar *str);
		XMLPUBFUN xmlXPathObject * xmlXPathCompiledEval(xmlXPathCompExprPtr comp, xmlXPathContext * ctx);
		XMLPUBFUN int xmlXPathCompiledEvalToBoolean(xmlXPathCompExprPtr comp, xmlXPathContext * ctxt);
		XMLPUBFUN void xmlXPathFreeCompExpr(xmlXPathCompExprPtr comp);
	#endif /* LIBXML_XPATH_ENABLED */
	#if defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
		XMLPUBFUN void xmlXPathInit();
		// @v10.9.11 XMLPUBFUN int xmlXPathIsNaN_Removed(double val);
		XMLPUBFUN int FASTCALL xmlXPathIsInf(double val);
		//#ifdef __cplusplus
		//}
		//#endif
	#endif
	//#include <libxml/xpointer.h>
	#ifdef LIBXML_XPTR_ENABLED
		//#ifdef __cplusplus
		//extern "C" {
		//#endif
		// 
		// A Location Set
		// 
		struct xmlLocationSet {
			int    locNr;  // number of locations in the set 
			int    locMax; // size of the array as allocated 
			xmlXPathObject ** locTab; // array of locations 
		};

		//typedef xmlLocationSet * xmlLocationSetPtr;
		// 
		// Handling of location sets.
		// 
		XMLPUBFUN xmlLocationSet * FASTCALL xmlXPtrLocationSetCreate(xmlXPathObject * val);
		XMLPUBFUN void xmlXPtrFreeLocationSet(xmlLocationSet * obj);
		XMLPUBFUN xmlLocationSet * xmlXPtrLocationSetMerge(xmlLocationSet * val1, xmlLocationSet * val2);
		XMLPUBFUN xmlXPathObject * xmlXPtrNewRange(xmlNode * start, int startindex, xmlNode * end, int endindex);
		XMLPUBFUN xmlXPathObject * xmlXPtrNewRangePoints(xmlXPathObject * start, xmlXPathObject * end);
		XMLPUBFUN xmlXPathObject * xmlXPtrNewRangeNodePoint(xmlNode * start, xmlXPathObject * end);
		XMLPUBFUN xmlXPathObject * xmlXPtrNewRangePointNode(xmlXPathObject * start, xmlNode * end);
		XMLPUBFUN xmlXPathObject * xmlXPtrNewRangeNodes(xmlNode * start, xmlNode * end);
		XMLPUBFUN xmlXPathObject * xmlXPtrNewLocationSetNodes(xmlNode * start, xmlNode * end);
		XMLPUBFUN xmlXPathObject * xmlXPtrNewLocationSetNodeSet(xmlNodeSet * set);
		XMLPUBFUN xmlXPathObject * xmlXPtrNewRangeNodeObject(xmlNode * start, xmlXPathObject * end);
		XMLPUBFUN xmlXPathObject * xmlXPtrNewCollapsedRange(xmlNode * start);
		XMLPUBFUN void FASTCALL xmlXPtrLocationSetAdd(xmlLocationSet * cur, xmlXPathObject * val);
		XMLPUBFUN xmlXPathObject * xmlXPtrWrapLocationSet(xmlLocationSet * val);
		XMLPUBFUN void xmlXPtrLocationSetDel(xmlLocationSet * cur, xmlXPathObject * val);
		XMLPUBFUN void xmlXPtrLocationSetRemove(xmlLocationSet * cur, int val);
		XMLPUBFUN xmlXPathContext * xmlXPtrNewContext(xmlDoc * doc, xmlNode * here, xmlNode * origin);
		XMLPUBFUN xmlXPathObject * xmlXPtrEval(const xmlChar * str, xmlXPathContext * ctx);
		XMLPUBFUN void xmlXPtrRangeToFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN xmlNode * xmlXPtrBuildNodeList(xmlXPathObject * obj);
		XMLPUBFUN void xmlXPtrEvalRangePredicate(xmlXPathParserContext * ctxt);
		//#ifdef __cplusplus
		//}
		//#endif
	#endif

	//#include <libxml/xpathInternals.h>
	#ifdef LIBXML_XPATH_ENABLED
		//#ifdef __cplusplus
		//extern "C" {
		//#endif
		// 
		// Helpers
		// 
		/*
		 * Many of these macros may later turn into functions. They
		 * shouldn't be used in #ifdef's preprocessor instructions.
		 */
		/**
		 * xmlXPathSetError:
		 * @ctxt:  an XPath parser context
		 * @err:  an xmlXPathError code
		 *
		 * Raises an error.
		 */
		#define xmlXPathSetError(ctxt, err) { xmlXPatherror((ctxt),/*__FILE__, __LINE__,*/(err)); if((ctxt) != NULL) (ctxt)->error = (err); }
		/**
		 * xmlXPathSetArityError:
		 * @ctxt:  an XPath parser context
		 *
		 * Raises an XPATH_INVALID_ARITY error.
		 */
		#define xmlXPathSetArityError(ctxt) xmlXPathSetError((ctxt), XPATH_INVALID_ARITY)
		/**
		 * xmlXPathSetTypeError:
		 * @ctxt:  an XPath parser context
		 * Raises an XPATH_INVALID_TYPE error.
		 */
		#define xmlXPathSetTypeError(ctxt) xmlXPathSetError((ctxt), XPATH_INVALID_TYPE)
		/**
		 * xmlXPathGetError:
		 * @ctxt:  an XPath parser context
		 * Get the error code of an XPath context.
		 * Returns the context error.
		 */
		#define xmlXPathGetError(ctxt)    ((ctxt)->error)
		/**
		 * xmlXPathCheckError:
		 * @ctxt:  an XPath parser context
		 * Check if an XPath error was raised.
		 * Returns true if an error has been raised, false otherwise.
		 */
		#define xmlXPathCheckError(ctxt)  ((ctxt)->error != XPATH_EXPRESSION_OK)
		/**
		 * xmlXPathGetDocument:
		 * @ctxt:  an XPath parser context
		 * Get the document of an XPath context.
		 * Returns the context document.
		 */
		#define xmlXPathGetDocument(ctxt)       ((ctxt)->context->doc)
		/**
		 * xmlXPathGetContextNode:
		 * @ctxt: an XPath parser context
		 * Get the context node of an XPath context.
		 * Returns the context node.
		 */
		#define xmlXPathGetContextNode(ctxt)    ((ctxt)->context->P_Node)

		XMLPUBFUN int xmlXPathPopBoolean(xmlXPathParserContext * ctxt);
		XMLPUBFUN double xmlXPathPopNumber(xmlXPathParserContext * ctxt);
		XMLPUBFUN xmlChar * xmlXPathPopString(xmlXPathParserContext * ctxt);
		XMLPUBFUN xmlNodeSet * xmlXPathPopNodeSet(xmlXPathParserContext * ctxt);
		XMLPUBFUN void * xmlXPathPopExternal(xmlXPathParserContext * ctxt);
		/**
		 * xmlXPathReturnBoolean:
		 * @ctxt:  an XPath parser context
		 * @val:  a boolean
		 *
		 * Pushes the boolean @val on the context stack.
		 */
		#define xmlXPathReturnBoolean(ctxt, val) valuePush((ctxt), xmlXPathNewBoolean(val))
		/**
		 * xmlXPathReturnTrue:
		 * @ctxt:  an XPath parser context
		 *
		 * Pushes true on the context stack.
		 */
		#define xmlXPathReturnTrue(ctxt)   xmlXPathReturnBoolean((ctxt), 1)
		/**
		 * xmlXPathReturnFalse:
		 * @ctxt:  an XPath parser context
		 *
		 * Pushes false on the context stack.
		 */
		#define xmlXPathReturnFalse(ctxt)  xmlXPathReturnBoolean((ctxt), 0)
		/**
		 * xmlXPathReturnNumber:
		 * @ctxt:  an XPath parser context
		 * @val:  a double
		 *
		 * Pushes the double @val on the context stack.
		 */
		#define xmlXPathReturnNumber(ctxt, val) valuePush((ctxt), xmlXPathNewFloat(val))
		/**
		 * xmlXPathReturnString:
		 * @ctxt:  an XPath parser context
		 * @str:  a string
		 *
		 * Pushes the string @str on the context stack.
		 */
		#define xmlXPathReturnString(ctxt, str) valuePush((ctxt), xmlXPathWrapString(str))
		/**
		 * xmlXPathReturnEmptyString:
		 * @ctxt:  an XPath parser context
		 *
		 * Pushes an empty string on the stack.
		 */
		#define xmlXPathReturnEmptyString(ctxt) valuePush((ctxt), xmlXPathNewCString(""))
		/**
		 * xmlXPathReturnNodeSet:
		 * @ctxt:  an XPath parser context
		 * @ns:  a node-set
		 *
		 * Pushes the node-set @ns on the context stack.
		 */
		#define xmlXPathReturnNodeSet(ctxt, ns) valuePush((ctxt), xmlXPathWrapNodeSet(ns))
		/**
		 * xmlXPathReturnEmptyNodeSet:
		 * @ctxt:  an XPath parser context
		 *
		 * Pushes an empty node-set on the context stack.
		 */
		#define xmlXPathReturnEmptyNodeSet(ctxt) valuePush((ctxt), xmlXPathNewNodeSet(NULL))
		/**
		 * xmlXPathReturnExternal:
		 * @ctxt:  an XPath parser context
		 * @val:  user data
		 *
		 * Pushes user data on the context stack.
		 */
		#define xmlXPathReturnExternal(ctxt, val) valuePush((ctxt), xmlXPathWrapExternal(val))
		/**
		 * xmlXPathStackIsNodeSet:
		 * @ctxt: an XPath parser context
		 *
		 * Check if the current value on the XPath stack is a node set or an XSLT value tree.
		 *
		 * Returns true if the current object on the stack is a node-set.
		 */
		#define xmlXPathStackIsNodeSet(ctxt) (((ctxt)->value) && (((ctxt)->value->type == XPATH_NODESET) || ((ctxt)->value->type == XPATH_XSLT_TREE)))
		/**
		 * xmlXPathStackIsExternal:
		 * @ctxt: an XPath parser context
		 *
		 * Checks if the current value on the XPath stack is an external object.
		 *
		 * Returns true if the current object on the stack is an external object.
		 */
		#define xmlXPathStackIsExternal(ctxt) ((ctxt->value) && (ctxt->value->type == XPATH_USERS))
		/**
		 * xmlXPathEmptyNodeSet:
		 * @ns:  a node-set
		 *
		 * Empties a node-set.
		 */
		#define xmlXPathEmptyNodeSet(ns) { while((ns)->nodeNr > 0) (ns)->PP_NodeTab[(ns)->nodeNr--] = NULL; }
		// Macro to return from the function if an XPath error was detected.
		#define CHECK_ERROR if(ctxt->error != XPATH_EXPRESSION_OK) return
		// Macro to return 0 from the function if an XPath error was detected.
		#define CHECK_ERROR0 if(ctxt->error != XPATH_EXPRESSION_OK) return (0)
		/**
		 * Macro to raise an XPath error and return.
		 * @X:  the error code
		 */
		#define XP_ERROR(X) { xmlXPathErr(ctxt, X); return; }
		// 
		// Macro to raise an XPath error and return 0.
		// @X:  the error code
		// 
		#define XP_ERROR0(X) { xmlXPathErr(ctxt, X); return 0; }
		// 
		// Macro to check that the value on top of the XPath stack is of a given type.
		// @typeval:  the XPath type
		// 
		#define CHECK_TYPE(typeval) if(!ctxt->value || (ctxt->value->type != typeval)) XP_ERROR(XPATH_INVALID_TYPE)
		// 
		// Macro to check that the value on top of the XPath stack is of a given type. 
		// @typeval:  the XPath type
		// Return(0) in case of failure
		// 
		#define CHECK_TYPE0(typeval) if(!ctxt->value || (ctxt->value->type != typeval)) XP_ERROR0(XPATH_INVALID_TYPE)
		/**
		 * CHECK_ARITY:
		 * @x:  the number of expected args
		 *
		 * Macro to check that the number of args passed to an XPath function matches.
		 */
		#define CHECK_ARITY(x) if(ctxt == NULL) return; if(nargs != (x)) XP_ERROR(XPATH_INVALID_ARITY); if(ctxt->valueNr < ctxt->valueFrame + (x)) XP_ERROR(XPATH_STACK_ERROR);
		// 
		// Descr: Macro to try to cast the value on the top of the XPath stack to a string.
		// 
		#define CAST_TO_STRING if(ctxt->value && (ctxt->value->type != XPATH_STRING)) xmlXPathStringFunction(ctxt, 1);
		// 
		// Descr: Macro to try to cast the value on the top of the XPath stack to a number.
		// 
		#define CAST_TO_NUMBER if(ctxt->value && (ctxt->value->type != XPATH_NUMBER)) xmlXPathNumberFunction(ctxt, 1);
		// 
		// Descr: Macro to try to cast the value on the top of the XPath stack to a boolean.
		// 
		#define CAST_TO_BOOLEAN if(ctxt->value && (ctxt->value->type != XPATH_BOOLEAN)) xmlXPathBooleanFunction(ctxt, 1);
		/*
		 * Variable Lookup forwarding.
		 */
		XMLPUBFUN void xmlXPathRegisterVariableLookup(xmlXPathContext * ctxt, xmlXPathVariableLookupFunc f, void * data);
		/*
		 * Function Lookup forwarding.
		 */
		XMLPUBFUN void xmlXPathRegisterFuncLookup(xmlXPathContext * ctxt, xmlXPathFuncLookupFunc f, void * funcCtxt);
		// 
		// Error reporting.
		// 
		XMLPUBFUN void xmlXPatherror(xmlXPathParserContext * ctxt, /*const char * file, int line,*/int no);
		XMLPUBFUN void FASTCALL xmlXPathErr(xmlXPathParserContext * ctxt, int error);
		#ifdef LIBXML_DEBUG_ENABLED
			XMLPUBFUN void xmlXPathDebugDumpObject(FILE * output, xmlXPathObject * cur, int depth);
			XMLPUBFUN void xmlXPathDebugDumpCompExpr(FILE * output, xmlXPathCompExprPtr comp, int depth);
		#endif
		// 
		// NodeSet handling.
		// 
		XMLPUBFUN int xmlXPathNodeSetContains(const xmlNodeSet * cur, const xmlNode * val);
		XMLPUBFUN xmlNodeSet * xmlXPathDifference(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		XMLPUBFUN xmlNodeSet * xmlXPathIntersection(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		XMLPUBFUN xmlNodeSet * xmlXPathDistinctSorted(xmlNodeSet * nodes);
		XMLPUBFUN xmlNodeSet * xmlXPathDistinct(xmlNodeSet * nodes);
		XMLPUBFUN int xmlXPathHasSameNodes(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		XMLPUBFUN xmlNodeSet * xmlXPathNodeLeadingSorted(xmlNodeSet * nodes, xmlNode * pNode);
		XMLPUBFUN xmlNodeSet * xmlXPathLeadingSorted(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		XMLPUBFUN xmlNodeSet * xmlXPathNodeLeading(xmlNodeSet * nodes, xmlNode * pNode);
		XMLPUBFUN xmlNodeSet * xmlXPathLeading(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		XMLPUBFUN xmlNodeSet * xmlXPathNodeTrailingSorted(xmlNodeSet * nodes, xmlNode * pNode);
		XMLPUBFUN xmlNodeSet * xmlXPathTrailingSorted(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		XMLPUBFUN xmlNodeSet * xmlXPathNodeTrailing(xmlNodeSet * nodes, xmlNode * pNode);
		XMLPUBFUN xmlNodeSet * xmlXPathTrailing(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		// 
		// Extending a context.
		// 
		XMLPUBFUN int xmlXPathRegisterNs(xmlXPathContext * ctxt, const xmlChar * prefix, const xmlChar * ns_uri);
		XMLPUBFUN const xmlChar * xmlXPathNsLookup(xmlXPathContext * ctxt, const xmlChar * prefix);
		XMLPUBFUN void xmlXPathRegisteredNsCleanup(xmlXPathContext * ctxt);
		XMLPUBFUN int FASTCALL xmlXPathRegisterFunc(xmlXPathContext * ctxt, const xmlChar * name, xmlXPathFunction f);
		XMLPUBFUN int xmlXPathRegisterFuncNS(xmlXPathContext * ctxt, const xmlChar * name, const xmlChar * ns_uri, xmlXPathFunction f);
		XMLPUBFUN int xmlXPathRegisterVariable(xmlXPathContext * ctxt, const xmlChar * name, xmlXPathObject * value);
		XMLPUBFUN int xmlXPathRegisterVariableNS(xmlXPathContext * ctxt, const xmlChar * name, const xmlChar * ns_uri, xmlXPathObject * value);
		XMLPUBFUN xmlXPathFunction xmlXPathFunctionLookup(xmlXPathContext * ctxt, const xmlChar * name);
		XMLPUBFUN xmlXPathFunction xmlXPathFunctionLookupNS(xmlXPathContext * ctxt, const xmlChar * name, const xmlChar * ns_uri);
		XMLPUBFUN void xmlXPathRegisteredFuncsCleanup(xmlXPathContext * ctxt);
		XMLPUBFUN xmlXPathObject * xmlXPathVariableLookup(xmlXPathContext * ctxt, const xmlChar * name);
		XMLPUBFUN xmlXPathObject * xmlXPathVariableLookupNS(xmlXPathContext * ctxt, const xmlChar * name, const xmlChar * ns_uri);
		XMLPUBFUN void xmlXPathRegisteredVariablesCleanup(xmlXPathContext * ctxt);
		// 
		// Utilities to extend XPath.
		// 
		XMLPUBFUN xmlXPathParserContext * xmlXPathNewParserContext(const xmlChar * str, xmlXPathContext * ctxt);
		XMLPUBFUN void xmlXPathFreeParserContext(xmlXPathParserContext * ctxt);
		/* @todo remap to xmlXPathValuePop and Push. */
		XMLPUBFUN xmlXPathObject * FASTCALL valuePop(xmlXPathParserContext * pCtxt);
		XMLPUBFUN int FASTCALL valuePush(xmlXPathParserContext * pCtxt, xmlXPathObject * pValue);
		XMLPUBFUN xmlXPathObject * xmlXPathNewString(const xmlChar * val);
		XMLPUBFUN xmlXPathObject * xmlXPathNewCString(const char * val);
		XMLPUBFUN xmlXPathObject * xmlXPathWrapString(xmlChar * val);
		XMLPUBFUN xmlXPathObject * xmlXPathWrapCString(char * val);
		XMLPUBFUN xmlXPathObject * xmlXPathNewFloat(double val);
		XMLPUBFUN xmlXPathObject * xmlXPathNewBoolean(int val);
		XMLPUBFUN xmlXPathObject * xmlXPathNewNodeSet(xmlNode * val);
		XMLPUBFUN xmlXPathObject * xmlXPathNewValueTree(xmlNode * val);
		XMLPUBFUN int xmlXPathNodeSetAdd(xmlNodeSet * cur, xmlNode * val);
		XMLPUBFUN int xmlXPathNodeSetAddUnique(xmlNodeSet * cur, xmlNode * val);
		XMLPUBFUN int xmlXPathNodeSetAddNs(xmlNodeSet * cur, xmlNode * pNode, xmlNs * ns);
		XMLPUBFUN void xmlXPathNodeSetSort(xmlNodeSet * set);
		XMLPUBFUN void xmlXPathRoot(xmlXPathParserContext * ctxt);
		XMLPUBFUN void xmlXPathEvalExpr(xmlXPathParserContext * ctxt);
		XMLPUBFUN xmlChar * xmlXPathParseName(xmlXPathParserContext * ctxt);
		XMLPUBFUN xmlChar * xmlXPathParseNCName(xmlXPathParserContext * ctxt);
		// 
		// Existing functions.
		// 
		XMLPUBFUN double xmlXPathStringEvalNumber(const xmlChar * str);
		XMLPUBFUN int    xmlXPathEvaluatePredicateResult(xmlXPathParserContext * ctxt, xmlXPathObject * res);
		XMLPUBFUN void   xmlXPathRegisterAllFunctions(xmlXPathContext * ctxt);
		XMLPUBFUN xmlNodeSet * xmlXPathNodeSetMerge(xmlNodeSet * val1, xmlNodeSet * val2);
		XMLPUBFUN void   xmlXPathNodeSetDel(xmlNodeSet * cur, xmlNode * val);
		XMLPUBFUN void   xmlXPathNodeSetRemove(xmlNodeSet * cur, int val);
		XMLPUBFUN xmlXPathObject * xmlXPathNewNodeSetList(xmlNodeSet * val);
		XMLPUBFUN xmlXPathObject * xmlXPathWrapNodeSet(xmlNodeSet * val);
		XMLPUBFUN xmlXPathObject * xmlXPathWrapExternal(void * val);
		XMLPUBFUN int    xmlXPathEqualValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN int    xmlXPathNotEqualValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN int    xmlXPathCompareValues(xmlXPathParserContext * ctxt, int inf, int strict);
		XMLPUBFUN void   xmlXPathValueFlipSign(xmlXPathParserContext * ctxt);
		XMLPUBFUN void   xmlXPathAddValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN void   xmlXPathSubValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN void   xmlXPathMultValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN void   xmlXPathDivValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN void   xmlXPathModValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN int    xmlXPathIsNodeType(const xmlChar * name);
		// 
		// Some of the axis navigation routines.
		// 
		XMLPUBFUN xmlNode * xmlXPathNextSelf(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * xmlXPathNextChild(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * xmlXPathNextDescendant(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * xmlXPathNextDescendantOrSelf(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * xmlXPathNextParent(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * xmlXPathNextAncestorOrSelf(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * xmlXPathNextFollowingSibling(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * xmlXPathNextFollowing(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * xmlXPathNextNamespace(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * xmlXPathNextAttribute(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * xmlXPathNextPreceding(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * xmlXPathNextAncestor(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * xmlXPathNextPrecedingSibling(xmlXPathParserContext * ctxt, xmlNode * cur);
		// 
		// The official core of XPath functions.
		// 
		XMLPUBFUN void xmlXPathLastFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathPositionFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathCountFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathIdFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathLocalNameFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathNamespaceURIFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathStringFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathStringLengthFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathConcatFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathContainsFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathStartsWithFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathSubstringFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathSubstringBeforeFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathSubstringAfterFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathNormalizeFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathTranslateFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathNotFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathTrueFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathFalseFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathLangFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathNumberFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathSumFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathFloorFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathCeilingFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathRoundFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void xmlXPathBooleanFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void FASTCALL xmlXPathNodeSetFreeNs(xmlNs * ns); // Really internal functions

		//#ifdef __cplusplus
		//}
		//#endif
	#endif
#ifdef IN_LIBXML
	//#include <libxml/c14n.h>
	#ifdef LIBXML_C14N_ENABLED
		#ifdef LIBXML_OUTPUT_ENABLED
			//#ifdef __cplusplus
			//extern "C" {
			//#endif /* __cplusplus */
			/*
			 * XML Canonicazation http://www.w3.org/TR/xml-c14n
			 * Exclusive XML Canonicazation http://www.w3.org/TR/xml-exc-c14n
			 *
			 * Canonical form of an XML document could be created if and only if
			 *  a) default attributes (if any) are added to all nodes
			 *  b) all character and parsed entity references are resolved
			 * In order to achive this in libxml2 the document MUST be loaded with
			 * following global setings:
			 *
			 *  xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;
			 *  xmlSubstituteEntitiesDefault(1);
			 *
			 * or corresponding parser context setting:
			 *  xmlParserCtxt * ctxt;
			 *
			 *  ...
			 *  ctxt->loadsubset = XML_DETECT_IDS | XML_COMPLETE_ATTRS;
			 *  ctxt->replaceEntities = 1;
			 *  ...
			 */
			//
			// Descr: Predefined values for C14N modes
			//
			enum xmlC14NMode {
				XML_C14N_1_0    = 0, // Origianal C14N 1.0 spec 
				XML_C14N_EXCLUSIVE_1_0  = 1, // Exclusive C14N 1.0 spec 
				XML_C14N_1_1    = 2  // C14N 1.1 spec 
			};

			XMLPUBFUN int xmlC14NDocSaveTo(xmlDoc * doc, xmlNodeSet * nodes, int mode/* a xmlC14NMode */, xmlChar ** inclusive_ns_prefixes, int with_comments, xmlOutputBuffer * buf);
			XMLPUBFUN int xmlC14NDocDumpMemory(xmlDoc * doc, xmlNodeSet * nodes, int mode/* a xmlC14NMode */, xmlChar ** inclusive_ns_prefixes, int with_comments, xmlChar ** doc_txt_ptr);
			XMLPUBFUN int xmlC14NDocSave(xmlDoc * doc, xmlNodeSet * nodes, int mode/* a xmlC14NMode */, xmlChar ** inclusive_ns_prefixes, int with_comments, const char * filename, int compression);
			/**
			 * This is the core C14N function
			 */
			/**
			 * xmlC14NIsVisibleCallback:
			 * @user_data: user data
			 * @node: the curent node
			 * @parent: the parent node
			 *
			 * Signature for a C14N callback on visible nodes
			 *
			 * Returns 1 if the node should be included
			 */
			typedef int (*xmlC14NIsVisibleCallback)(void * user_data, xmlNode * node, xmlNode * parent);

			XMLPUBFUN int xmlC14NExecute(xmlDoc * doc, xmlC14NIsVisibleCallback is_visible_callback, void * user_data, 
				int mode/* a xmlC14NMode */, xmlChar ** inclusive_ns_prefixes, int with_comments, xmlOutputBuffer * buf);

			//#ifdef __cplusplus
			//}
			//#endif /* __cplusplus */
		#endif /* LIBXML_OUTPUT_ENABLED */
	#endif
	//#include <libxml/pattern.h>
	#ifdef LIBXML_PATTERN_ENABLED
		//#ifdef __cplusplus
		//extern "C" {
		//#endif
		//
		// Descr: A compiled (XPath based) pattern to select nodes
		//
		struct xmlPattern;
		//typedef xmlPattern * xmlPatternPtr;
		//
		// Descr: This is the set of options affecting the behaviour of pattern matching with this module
		//
		enum xmlPatternFlags {
			XML_PATTERN_DEFAULT		= 0,	/* simple pattern match */
			XML_PATTERN_XPATH		= 1<<0,	/* standard XPath pattern */
			XML_PATTERN_XSSEL		= 1<<1,	/* XPath subset for schema selector */
			XML_PATTERN_XSFIELD		= 1<<2	/* XPath subset for schema field */
		};

		XMLPUBFUN void xmlFreePattern(xmlPattern * comp);
		XMLPUBFUN void xmlFreePatternList(xmlPattern * comp);
		XMLPUBFUN xmlPattern * xmlPatterncompile(const xmlChar *pattern, xmlDict *dict, int flags, const xmlChar **namespaces);
		XMLPUBFUN int xmlPatternMatch(xmlPattern * comp, xmlNode * pNode);

		/* streaming interfaces */
		typedef struct _xmlStreamCtxt xmlStreamCtxt;
		typedef xmlStreamCtxt * xmlStreamCtxtPtr;

		XMLPUBFUN int xmlPatternStreamable(xmlPattern * comp);
		XMLPUBFUN int xmlPatternMaxDepth(xmlPattern * comp);
		XMLPUBFUN int xmlPatternMinDepth(xmlPattern * comp);
		XMLPUBFUN int xmlPatternFromRoot(xmlPattern * comp);
		XMLPUBFUN xmlStreamCtxtPtr xmlPatternGetStreamCtxt(xmlPattern * comp);
		XMLPUBFUN void xmlFreeStreamCtxt(xmlStreamCtxtPtr stream);
		XMLPUBFUN int xmlStreamPushNode(xmlStreamCtxtPtr stream, const xmlChar *name, const xmlChar *ns, int nodeType);
		XMLPUBFUN int xmlStreamPush(xmlStreamCtxtPtr stream, const xmlChar *name, const xmlChar *ns);
		XMLPUBFUN int xmlStreamPushAttr(xmlStreamCtxtPtr stream, const xmlChar *name, const xmlChar *ns);
		XMLPUBFUN int xmlStreamPop(xmlStreamCtxtPtr stream);
		XMLPUBFUN int xmlStreamWantsAnyNode	(xmlStreamCtxtPtr stream);
		//#ifdef __cplusplus
		//}
		//#endif
	#endif /* LIBXML_PATTERN_ENABLED */
	//#include <libxml/schematron.h>
	#ifdef LIBXML_SCHEMATRON_ENABLED
		//#ifdef __cplusplus
		//extern "C" {
		//#endif

		enum xmlSchematronValidOptions {
			XML_SCHEMATRON_OUT_QUIET = 1 << 0,	/* quiet no report */
			XML_SCHEMATRON_OUT_TEXT = 1 << 1,	/* build a textual report */
			XML_SCHEMATRON_OUT_XML = 1 << 2,	/* output SVRL */
			XML_SCHEMATRON_OUT_ERROR = 1 << 3,  /* output via xmlStructuredErrorFunc */
			XML_SCHEMATRON_OUT_FILE = 1 << 8,	/* output to a file descriptor */
			XML_SCHEMATRON_OUT_BUFFER = 1 << 9,	/* output to a buffer */
			XML_SCHEMATRON_OUT_IO = 1 << 10	/* output to I/O mechanism */
		};
		/**
		 * The schemas related types are kept internal
		 */
		typedef struct _xmlSchematron xmlSchematron;
		// @v10.6.0 typedef xmlSchematron * xmlSchematronPtr;
		/**
		 * xmlSchematronValidityErrorFunc:
		 * @ctx: the validation context
		 * @msg: the message
		 * @...: extra arguments
		 *
		 * Signature of an error callback from a Schematron validation
		 */
		typedef void (*xmlSchematronValidityErrorFunc)(void *ctx, const char *msg, ...);
		/**
		 * xmlSchematronValidityWarningFunc:
		 * @ctx: the validation context
		 * @msg: the message
		 * @...: extra arguments
		 *
		 * Signature of a warning callback from a Schematron validation
		 */
		typedef void (*xmlSchematronValidityWarningFunc)(void *ctx, const char *msg, ...);
		/**
		 * A schemas validation context
		 */
		typedef struct _xmlSchematronParserCtxt xmlSchematronParserCtxt;
		//typedef xmlSchematronParserCtxt *xmlSchematronParserCtxtPtr;
		typedef struct _xmlSchematronValidCtxt xmlSchematronValidCtxt;
		//typedef xmlSchematronValidCtxt * xmlSchematronValidCtxtPtr;
		/*
		 * Interfaces for parsing.
		 */
		XMLPUBFUN xmlSchematronParserCtxt * xmlSchematronNewParserCtxt(const char *URL);
		XMLPUBFUN xmlSchematronParserCtxt * xmlSchematronNewMemParserCtxt(const char *buffer, int size);
		XMLPUBFUN xmlSchematronParserCtxt * xmlSchematronNewDocParserCtxt(xmlDoc * doc);
		XMLPUBFUN void xmlSchematronFreeParserCtxt(xmlSchematronParserCtxt * ctxt);
		// XMLPUBFUN void xmlSchematronSetParserErrors(xmlSchematronParserCtxt * ctxt, xmlSchematronValidityErrorFunc err, xmlSchematronValidityWarningFunc warn, void *ctx);
		// XMLPUBFUN int xmlSchematronGetParserErrors(xmlSchematronParserCtxt * ctxt, xmlSchematronValidityErrorFunc * err, xmlSchematronValidityWarningFunc * warn, void **ctx);
		// XMLPUBFUN int xmlSchematronIsValid(xmlSchematronValidCtxt * ctxt);
		XMLPUBFUN xmlSchematron * xmlSchematronParse(xmlSchematronParserCtxt * ctxt);
		XMLPUBFUN void xmlSchematronFree(xmlSchematron * schema);
		/*
		 * Interfaces for validating
		 */
		XMLPUBFUN void xmlSchematronSetValidStructuredErrors(xmlSchematronValidCtxt * ctxt, xmlStructuredErrorFunc serror, void *ctx);
		// XMLPUBFUN void xmlSchematronSetValidErrors(xmlSchematronValidCtxt * ctxt, xmlSchematronValidityErrorFunc err, xmlSchematronValidityWarningFunc warn, void *ctx);
		// XMLPUBFUN int xmlSchematronGetValidErrors(xmlSchematronValidCtxt * ctxt, xmlSchematronValidityErrorFunc *err, xmlSchematronValidityWarningFunc *warn, void **ctx);
		// XMLPUBFUN int xmlSchematronSetValidOptions(xmlSchematronValidCtxt * ctxt, int options);
		// XMLPUBFUN int xmlSchematronValidCtxtGetOptions(xmlSchematronValidCtxt * ctxt);
		// XMLPUBFUN int xmlSchematronValidateOneElement(xmlSchematronValidCtxt * ctxt, xmlNode * elem);
		XMLPUBFUN xmlSchematronValidCtxt * xmlSchematronNewValidCtxt(xmlSchematron * schema, int options);
		XMLPUBFUN void xmlSchematronFreeValidCtxt(xmlSchematronValidCtxt * ctxt);
		XMLPUBFUN int xmlSchematronValidateDoc(xmlSchematronValidCtxt * ctxt, xmlDoc * instance);

		//#ifdef __cplusplus
		//}
		//#endif
	#endif
	//#include <libxml/debugXML.h>
	#ifdef LIBXML_DEBUG_ENABLED
		//#ifdef __cplusplus
		//extern "C" {
		//#endif
		/*
		 * The standard Dump routines.
		 */
		XMLPUBFUN void xmlDebugDumpString(FILE * output, const xmlChar * str);
		XMLPUBFUN void xmlDebugDumpAttr(FILE * output, xmlAttr * attr, int depth);
		XMLPUBFUN void xmlDebugDumpAttrList(FILE * output, xmlAttr * attr, int depth);
		XMLPUBFUN void xmlDebugDumpOneNode(FILE * output, xmlNode * pNode, int depth);
		XMLPUBFUN void xmlDebugDumpNode(FILE * output, xmlNode * pNode, int depth);
		XMLPUBFUN void xmlDebugDumpNodeList(FILE * output, xmlNode * pNode, int depth);
		XMLPUBFUN void xmlDebugDumpDocumentHead(FILE * output, xmlDoc * doc);
		XMLPUBFUN void xmlDebugDumpDocument(FILE * output, xmlDoc * doc);
		XMLPUBFUN void xmlDebugDumpDTD(FILE * output, xmlDtd * dtd);
		XMLPUBFUN void xmlDebugDumpEntities(FILE * output, xmlDoc * doc);
		// 
		// Checking routines
		// 
		XMLPUBFUN int xmlDebugCheckDocument(FILE * output, xmlDoc * doc);
		// 
		// XML shell helpers
		// 
		XMLPUBFUN void FASTCALL xmlLsOneNode(FILE * output, xmlNode * pNode);
		XMLPUBFUN int xmlLsCountNode(xmlNode * pNode);
		XMLPUBFUN const char * xmlBoolToText(int boolval);
		// 
		// The XML shell related structures and functions
		// 
		#ifdef LIBXML_XPATH_ENABLED
			/**
			 * xmlShellReadlineFunc:
			 * @prompt:  a string prompt
			 *
			 * This is a generic signature for the XML shell input function.
			 *
			 * Returns a string which will be freed by the Shell.
			 */
			typedef char * (*xmlShellReadlineFunc)(char * prompt);
			/**
			 * xmlShellCtxt:
			 *
			 * A debugging shell context.
			 * @todo add the defined function tables.
			 */
			struct xmlShellCtxt {
				char * filename;
				xmlDoc * doc;
				xmlNode * P_Node;
				xmlXPathContext * pctxt;
				int loaded;
				FILE * output;
				xmlShellReadlineFunc input;
			};
			typedef xmlShellCtxt * xmlShellCtxtPtr;
			/**
			 * xmlShellCmd:
			 * @ctxt:  a shell context
			 * @arg:  a string argument
			 * @node:  a first node
			 * @node2:  a second node
			 *
			 * This is a generic signature for the XML shell functions.
			 *
			 * Returns an int, negative returns indicating errors.
			 */
			typedef int (*xmlShellCmd)(xmlShellCtxtPtr ctxt, char * arg, xmlNode * pNode, xmlNode * node2);

			XMLPUBFUN void xmlShellPrintXPathError(int errorType, const char * arg);
			XMLPUBFUN void xmlShellPrintXPathResult(xmlXPathObject * list);
			XMLPUBFUN int xmlShellList(xmlShellCtxtPtr ctxt, char * arg, xmlNode * pNode, xmlNode * node2);
			XMLPUBFUN int xmlShellBase(xmlShellCtxtPtr ctxt, char * arg, xmlNode * pNode, xmlNode * node2);
			XMLPUBFUN int xmlShellDir(xmlShellCtxtPtr ctxt, char * arg, xmlNode * pNode, xmlNode * node2);
			XMLPUBFUN int xmlShellLoad(xmlShellCtxtPtr ctxt, char * filename, xmlNode * pNode, xmlNode * node2);
			#ifdef LIBXML_OUTPUT_ENABLED
				XMLPUBFUN void xmlShellPrintNode(xmlNode * pNode);
				XMLPUBFUN int xmlShellCat(xmlShellCtxtPtr ctxt, char * arg, xmlNode * pNode, xmlNode * node2);
				XMLPUBFUN int xmlShellWrite(xmlShellCtxtPtr ctxt, char * filename, xmlNode * pNode, xmlNode * node2);
				XMLPUBFUN int xmlShellSave(xmlShellCtxtPtr ctxt, char * filename, xmlNode * pNode, xmlNode * node2);
			#endif
			#ifdef LIBXML_VALID_ENABLED
				// @v10.9.0 XMLPUBFUN int xmlShellValidate(xmlShellCtxtPtr ctxt, char * dtd, xmlNode * pNode, xmlNode * node2);
			#endif
			// @v10.9.0 XMLPUBFUN int xmlShellDu(xmlShellCtxtPtr ctxt, char * arg, xmlNode * tree, xmlNode * node2);
			// @v10.9.0 XMLPUBFUN int xmlShellPwd(xmlShellCtxtPtr ctxt, char * buffer, xmlNode * pNode, xmlNode * node2);
			/*
			 * The Shell interface.
			 */
			// @v10.9.0 XMLPUBFUN void xmlShell(xmlDoc * doc, char * filename, xmlShellReadlineFunc input, FILE * output);
		#endif
		//#ifdef __cplusplus
		//}
		//#endif
	#endif
	//#include <libxml/chvalid.h>
	//#ifdef __cplusplus
	//extern "C" {
	//#endif
	// 
	// Descr: Define our typedefs and structures
	// 
	struct xmlChSRange {
		ushort low;
		ushort high;
	};

	struct xmlChLRange {
		uint low;
		uint high;
	};

	struct xmlChRangeGroup {
		int nbShortRange;
		int nbLongRange;
		const xmlChSRange * shortRange; /* points to an array of ranges */
		const xmlChLRange * longRange;
	};

	// @v10.6.0 typedef xmlChSRange * xmlChSRangePtr;
	// @v10.6.0 typedef xmlChLRange * xmlChLRangePtr;
	// @v10.6.0 typedef xmlChRangeGroup * xmlChRangeGroupPtr;
	// 
	// Descr: Range checking routine
	// 
	XMLPUBFUN int FASTCALL xmlCharInRange(uint val, const xmlChRangeGroup * group);
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsBaseChar_ch(c) (((0x41 <= (c)) && ((c) <= 0x5a)) || ((0x61 <= (c)) && ((c) <= 0x7a)) || ((0xc0 <= (c)) && ((c) <= 0xd6)) || ((0xd8 <= (c)) && ((c) <= 0xf6)) || (0xf8 <= (c)))
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsBaseCharQ(c) (((c) < 0x100) ? xmlIsBaseChar_ch((c)) : xmlCharInRange((c), &xmlIsBaseCharGroup))

	XMLPUBVAR const xmlChRangeGroup xmlIsBaseCharGroup;
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsBlank_ch(c) (((c) == 0x20) || ((0x9 <= (c)) && ((c) <= 0xa)) || ((c) == 0xd))
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsBlankQ(c) (((c) < 0x100) ? xmlIsBlank_ch((c)) : 0)
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsChar_ch(c) (((0x9 <= (c)) && ((c) <= 0xa)) || ((c) == 0xd) || (0x20 <= (c)))
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsCharQ(c) (((c) < 0x100) ? xmlIsChar_ch((c)) : (((0x100 <= (c)) && ((c) <= 0xd7ff)) || ((0xe000 <= (c)) && ((c) <= 0xfffd)) || ((0x10000 <= (c)) && ((c) <= 0x10ffff))))

	XMLPUBVAR const xmlChRangeGroup xmlIsCharGroup;
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsCombiningQ(c) (((c) < 0x100) ? 0 : xmlCharInRange((c), &xmlIsCombiningGroup))

	XMLPUBVAR const xmlChRangeGroup xmlIsCombiningGroup;
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	//#define xmlIsDigit_ch(c)        (((0x30 <= (c)) && ((c) <= 0x39)))
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsDigitQ(c) (((c) < 0x100) ? isdec(c) : xmlCharInRange((c), &xmlIsDigitGroup))

	XMLPUBVAR const xmlChRangeGroup xmlIsDigitGroup;
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsExtender_ch(c)     (((c) == 0xb7))
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsExtenderQ(c) (((c) < 0x100) ? xmlIsExtender_ch((c)) : xmlCharInRange((c), &xmlIsExtenderGroup))

	XMLPUBVAR const xmlChRangeGroup xmlIsExtenderGroup;
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsIdeographicQ(c) (((c) < 0x100) ? 0 : (((0x4e00 <= (c)) && ((c) <= 0x9fa5)) || ((c) == 0x3007) || ((0x3021 <= (c)) && ((c) <= 0x3029))))

	XMLPUBVAR const xmlChRangeGroup xmlIsIdeographicGroup;
	XMLPUBVAR const uchar xmlIsPubidChar_tab[256];
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsPubidChar_ch(c)    (xmlIsPubidChar_tab[(c)])
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsPubidCharQ(c) (((c) < 0x100) ? xmlIsPubidChar_ch((c)) : 0)

	XMLPUBFUN int xmlIsBaseChar(uint ch);
	XMLPUBFUN int xmlIsBlank(uint ch);
	XMLPUBFUN int xmlIsChar(uint ch);
	XMLPUBFUN int xmlIsCombining(uint ch);
	// @v10.9.11 XMLPUBFUN int xmlIsDigit(uint ch);
	XMLPUBFUN int xmlIsExtender(uint ch);
	XMLPUBFUN int xmlIsIdeographic(uint ch);
	XMLPUBFUN int xmlIsPubidChar(uint ch);
	//#ifdef __cplusplus
	//}
	//#endif
	//#include <libxml/xmlunicode.h>
	#ifdef LIBXML_UNICODE_ENABLED
		//#ifdef __cplusplus
		//extern "C" {
		//#endif
		XMLPUBFUN int xmlUCSIsAegeanNumbers(int code);
		XMLPUBFUN int xmlUCSIsAlphabeticPresentationForms(int code);
		XMLPUBFUN int xmlUCSIsArabic(int code);
		XMLPUBFUN int xmlUCSIsArabicPresentationFormsA(int code);
		XMLPUBFUN int xmlUCSIsArabicPresentationFormsB(int code);
		XMLPUBFUN int xmlUCSIsArmenian(int code);
		XMLPUBFUN int xmlUCSIsArrows(int code);
		XMLPUBFUN int xmlUCSIsBasicLatin(int code);
		XMLPUBFUN int xmlUCSIsBengali(int code);
		XMLPUBFUN int xmlUCSIsBlockElements(int code);
		XMLPUBFUN int xmlUCSIsBopomofo(int code);
		XMLPUBFUN int xmlUCSIsBopomofoExtended(int code);
		XMLPUBFUN int xmlUCSIsBoxDrawing(int code);
		XMLPUBFUN int xmlUCSIsBraillePatterns(int code);
		XMLPUBFUN int xmlUCSIsBuhid	(int code);
		XMLPUBFUN int xmlUCSIsByzantineMusicalSymbols(int code);
		XMLPUBFUN int xmlUCSIsCJKCompatibility(int code);
		XMLPUBFUN int xmlUCSIsCJKCompatibilityForms(int code);
		XMLPUBFUN int xmlUCSIsCJKCompatibilityIdeographs(int code);
		XMLPUBFUN int xmlUCSIsCJKCompatibilityIdeographsSupplement(int code);
		XMLPUBFUN int xmlUCSIsCJKRadicalsSupplement(int code);
		XMLPUBFUN int xmlUCSIsCJKSymbolsandPunctuation(int code);
		XMLPUBFUN int xmlUCSIsCJKUnifiedIdeographs(int code);
		XMLPUBFUN int xmlUCSIsCJKUnifiedIdeographsExtensionA(int code);
		XMLPUBFUN int xmlUCSIsCJKUnifiedIdeographsExtensionB(int code);
		XMLPUBFUN int xmlUCSIsCherokee(int code);
		XMLPUBFUN int xmlUCSIsCombiningDiacriticalMarks(int code);
		XMLPUBFUN int xmlUCSIsCombiningDiacriticalMarksforSymbols(int code);
		XMLPUBFUN int xmlUCSIsCombiningHalfMarks(int code);
		XMLPUBFUN int xmlUCSIsCombiningMarksforSymbols(int code);
		XMLPUBFUN int xmlUCSIsControlPictures(int code);
		XMLPUBFUN int xmlUCSIsCurrencySymbols(int code);
		XMLPUBFUN int xmlUCSIsCypriotSyllabary(int code);
		XMLPUBFUN int xmlUCSIsCyrillic(int code);
		XMLPUBFUN int xmlUCSIsCyrillicSupplement(int code);
		XMLPUBFUN int xmlUCSIsDeseret(int code);
		XMLPUBFUN int xmlUCSIsDevanagari(int code);
		XMLPUBFUN int xmlUCSIsDingbats(int code);
		XMLPUBFUN int xmlUCSIsEnclosedAlphanumerics(int code);
		XMLPUBFUN int xmlUCSIsEnclosedCJKLettersandMonths(int code);
		XMLPUBFUN int xmlUCSIsEthiopic(int code);
		XMLPUBFUN int xmlUCSIsGeneralPunctuation(int code);
		XMLPUBFUN int xmlUCSIsGeometricShapes(int code);
		XMLPUBFUN int xmlUCSIsGeorgian(int code);
		XMLPUBFUN int xmlUCSIsGothic(int code);
		XMLPUBFUN int xmlUCSIsGreek(int code);
		XMLPUBFUN int xmlUCSIsGreekExtended(int code);
		XMLPUBFUN int xmlUCSIsGreekandCoptic(int code);
		XMLPUBFUN int xmlUCSIsGujarati(int code);
		XMLPUBFUN int xmlUCSIsGurmukhi(int code);
		XMLPUBFUN int xmlUCSIsHalfwidthandFullwidthForms(int code);
		XMLPUBFUN int xmlUCSIsHangulCompatibilityJamo(int code);
		XMLPUBFUN int xmlUCSIsHangulJamo(int code);
		XMLPUBFUN int xmlUCSIsHangulSyllables(int code);
		XMLPUBFUN int xmlUCSIsHanunoo(int code);
		XMLPUBFUN int xmlUCSIsHebrew(int code);
		XMLPUBFUN int xmlUCSIsHighPrivateUseSurrogates(int code);
		XMLPUBFUN int xmlUCSIsHighSurrogates(int code);
		XMLPUBFUN int xmlUCSIsHiragana	(int code);
		XMLPUBFUN int xmlUCSIsIPAExtensions	(int code);
		XMLPUBFUN int xmlUCSIsIdeographicDescriptionCharacters	(int code);
		XMLPUBFUN int xmlUCSIsKanbun	(int code);
		XMLPUBFUN int xmlUCSIsKangxiRadicals	(int code);
		XMLPUBFUN int xmlUCSIsKannada	(int code);
		XMLPUBFUN int xmlUCSIsKatakana	(int code);
		XMLPUBFUN int xmlUCSIsKatakanaPhoneticExtensions	(int code);
		XMLPUBFUN int xmlUCSIsKhmer	(int code);
		XMLPUBFUN int xmlUCSIsKhmerSymbols	(int code);
		XMLPUBFUN int xmlUCSIsLao	(int code);
		XMLPUBFUN int xmlUCSIsLatin1Supplement(int code);
		XMLPUBFUN int xmlUCSIsLatinExtendedA(int code);
		XMLPUBFUN int xmlUCSIsLatinExtendedB(int code);
		XMLPUBFUN int xmlUCSIsLatinExtendedAdditional(int code);
		XMLPUBFUN int xmlUCSIsLetterlikeSymbols(int code);
		XMLPUBFUN int xmlUCSIsLimbu	(int code);
		XMLPUBFUN int xmlUCSIsLinearBIdeograms(int code);
		XMLPUBFUN int xmlUCSIsLinearBSyllabary(int code);
		XMLPUBFUN int xmlUCSIsLowSurrogates	(int code);
		XMLPUBFUN int xmlUCSIsMalayalam(int code);
		XMLPUBFUN int xmlUCSIsMathematicalAlphanumericSymbols(int code);
		XMLPUBFUN int xmlUCSIsMathematicalOperators(int code);
		XMLPUBFUN int xmlUCSIsMiscellaneousMathematicalSymbolsA(int code);
		XMLPUBFUN int xmlUCSIsMiscellaneousMathematicalSymbolsB(int code);
		XMLPUBFUN int xmlUCSIsMiscellaneousSymbols(int code);
		XMLPUBFUN int xmlUCSIsMiscellaneousSymbolsandArrows(int code);
		XMLPUBFUN int xmlUCSIsMiscellaneousTechnical(int code);
		XMLPUBFUN int xmlUCSIsMongolian(int code);
		XMLPUBFUN int xmlUCSIsMusicalSymbols(int code);
		XMLPUBFUN int xmlUCSIsMyanmar(int code);
		XMLPUBFUN int xmlUCSIsNumberForms(int code);
		XMLPUBFUN int xmlUCSIsOgham(int code);
		XMLPUBFUN int xmlUCSIsOldItalic(int code);
		XMLPUBFUN int xmlUCSIsOpticalCharacterRecognition(int code);
		XMLPUBFUN int xmlUCSIsOriya(int code);
		XMLPUBFUN int xmlUCSIsOsmanya(int code);
		XMLPUBFUN int xmlUCSIsPhoneticExtensions(int code);
		XMLPUBFUN int xmlUCSIsPrivateUse(int code);
		XMLPUBFUN int xmlUCSIsPrivateUseArea(int code);
		XMLPUBFUN int xmlUCSIsRunic(int code);
		XMLPUBFUN int xmlUCSIsShavian(int code);
		XMLPUBFUN int xmlUCSIsSinhala(int code);
		XMLPUBFUN int xmlUCSIsSmallFormVariants(int code);
		XMLPUBFUN int xmlUCSIsSpacingModifierLetters(int code);
		XMLPUBFUN int xmlUCSIsSpecials(int code);
		XMLPUBFUN int xmlUCSIsSuperscriptsandSubscripts(int code);
		XMLPUBFUN int xmlUCSIsSupplementalArrowsA(int code);
		XMLPUBFUN int xmlUCSIsSupplementalArrowsB(int code);
		XMLPUBFUN int xmlUCSIsSupplementalMathematicalOperators(int code);
		XMLPUBFUN int xmlUCSIsSupplementaryPrivateUseAreaA(int code);
		XMLPUBFUN int xmlUCSIsSupplementaryPrivateUseAreaB(int code);
		XMLPUBFUN int xmlUCSIsSyriac(int code);
		XMLPUBFUN int xmlUCSIsTagalog(int code);
		XMLPUBFUN int xmlUCSIsTagbanwa(int code);
		XMLPUBFUN int xmlUCSIsTags(int code);
		XMLPUBFUN int xmlUCSIsTaiLe(int code);
		XMLPUBFUN int xmlUCSIsTaiXuanJingSymbols(int code);
		XMLPUBFUN int xmlUCSIsTamil(int code);
		XMLPUBFUN int xmlUCSIsTelugu(int code);
		XMLPUBFUN int xmlUCSIsThaana(int code);
		XMLPUBFUN int xmlUCSIsThai(int code);
		XMLPUBFUN int xmlUCSIsTibetan(int code);
		XMLPUBFUN int xmlUCSIsUgaritic(int code);
		XMLPUBFUN int xmlUCSIsUnifiedCanadianAboriginalSyllabics(int code);
		XMLPUBFUN int xmlUCSIsVariationSelectors(int code);
		XMLPUBFUN int xmlUCSIsVariationSelectorsSupplement(int code);
		XMLPUBFUN int xmlUCSIsYiRadicals(int code);
		XMLPUBFUN int xmlUCSIsYiSyllables(int code);
		XMLPUBFUN int xmlUCSIsYijingHexagramSymbols(int code);
		XMLPUBFUN int xmlUCSIsBlock(int code, const char *block);
		XMLPUBFUN int xmlUCSIsCatC(int code);
		XMLPUBFUN int xmlUCSIsCatCc(int code);
		XMLPUBFUN int xmlUCSIsCatCf(int code);
		XMLPUBFUN int xmlUCSIsCatCo(int code);
		XMLPUBFUN int xmlUCSIsCatCs(int code);
		XMLPUBFUN int xmlUCSIsCatL(int code);
		XMLPUBFUN int xmlUCSIsCatLl(int code);
		XMLPUBFUN int xmlUCSIsCatLm(int code);
		XMLPUBFUN int xmlUCSIsCatLo(int code);
		XMLPUBFUN int xmlUCSIsCatLt(int code);
		XMLPUBFUN int xmlUCSIsCatLu(int code);
		XMLPUBFUN int xmlUCSIsCatM(int code);
		XMLPUBFUN int xmlUCSIsCatMc(int code);
		XMLPUBFUN int xmlUCSIsCatMe(int code);
		XMLPUBFUN int xmlUCSIsCatMn(int code);
		XMLPUBFUN int xmlUCSIsCatN(int code);
		XMLPUBFUN int xmlUCSIsCatNd(int code);
		XMLPUBFUN int xmlUCSIsCatNl(int code);
		XMLPUBFUN int xmlUCSIsCatNo(int code);
		XMLPUBFUN int xmlUCSIsCatP(int code);
		XMLPUBFUN int xmlUCSIsCatPc(int code);
		XMLPUBFUN int xmlUCSIsCatPd(int code);
		XMLPUBFUN int xmlUCSIsCatPe(int code);
		XMLPUBFUN int xmlUCSIsCatPf(int code);
		XMLPUBFUN int xmlUCSIsCatPi(int code);
		XMLPUBFUN int xmlUCSIsCatPo(int code);
		XMLPUBFUN int xmlUCSIsCatPs(int code);
		XMLPUBFUN int xmlUCSIsCatS(int code);
		XMLPUBFUN int xmlUCSIsCatSc(int code);
		XMLPUBFUN int xmlUCSIsCatSk(int code);
		XMLPUBFUN int xmlUCSIsCatSm(int code);
		XMLPUBFUN int xmlUCSIsCatSo(int code);
		XMLPUBFUN int xmlUCSIsCatZ(int code);
		XMLPUBFUN int xmlUCSIsCatZl(int code);
		XMLPUBFUN int xmlUCSIsCatZp(int code);
		XMLPUBFUN int xmlUCSIsCatZs(int code);
		XMLPUBFUN int xmlUCSIsCat(int code, const char *cat);
		//#ifdef __cplusplus
		//}
		//#endif
	#endif
	// @v10.6.0 #include <libxml/HTMLparser.h>
	// @v10.6.0 #include <libxml/HTMLtree.h>
	#include <libxml/HTMLparser.h> // @v11.0.0
	#include <libxml/HTMLtree.h> // @v11.0.0
	//#ifdef __cplusplus
	//extern "C" {
	//#endif
	// 
	// The dictionnary.
	// 
	//typedef struct _xmlDict xmlDict;
	//typedef xmlDict * xmlDictPtr;
	// 
	// Initializer
	// 
	// @v10.9.0 (depricated) XMLPUBFUN int xmlInitializeDict();
	XMLPUBFUN xmlDict * xmlDictCreate();
	XMLPUBFUN size_t xmlDictSetLimit(xmlDict * dict, size_t limit);
	XMLPUBFUN size_t xmlDictGetUsage(const xmlDict * dict);
	XMLPUBFUN xmlDict * xmlDictCreateSub(xmlDict * sub);
	XMLPUBFUN int FASTCALL xmlDictReference(xmlDict * dict);
	XMLPUBFUN void FASTCALL xmlDictFree(xmlDict * dict);
	// 
	// Lookup of entry in the dictionnary.
	// 
	XMLPUBFUN const xmlChar * FASTCALL xmlDictLookup(xmlDict * dict, const xmlChar *name, int len);
	const xmlChar * FASTCALL xmlDictLookupSL(xmlDict * dict, const xmlChar * name);
	XMLPUBFUN const xmlChar * xmlDictExists(xmlDict * dict, const xmlChar *name, int len);
	XMLPUBFUN const xmlChar * xmlDictQLookup(xmlDict * dict, const xmlChar *prefix, const xmlChar *name);
	XMLPUBFUN int FASTCALL xmlDictOwns(const xmlDict * pDict, const xmlChar * pStr);
	XMLPUBFUN int xmlDictSize(const xmlDict * dict);
	//
	// Descr: Free a string if it is not owned by the "dict" dictionnary in the current scope
	//
	void FASTCALL XmlDestroyStringWithDict(xmlDict * pDict, xmlChar * pStr);
	// 
	// Cleanup function
	//
	XMLPUBFUN void xmlDictCleanup();
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
		char * scheme; /* the URI scheme */
		char * opaque; /* opaque part */
		char * authority; /* the authority part */
		char * server; /* the server part */
		char * user; /* the user part */
		int    port;   // the port number 
		char * path; /* the path string */
		char * query; /* the query string (deprecated - use with caution) */
		char * fragment; /* the fragment identifier */
		int    cleanup; /* parsing potentially unclean URI */
		char * query_raw; /* the query string (as it appears in the URI) */
	};
	//typedef xmlURI * xmlURIPtr;
	// 
	// This function is in tree.h:
	// xmlChar * xmlNodeGetBase(xmlDoc * doc, xmlNode * cur);
	// 
	XMLPUBFUN xmlURI * xmlCreateURI();
	XMLPUBFUN xmlChar * FASTCALL xmlBuildURI(const xmlChar * URI, const xmlChar * base);
	XMLPUBFUN xmlChar * xmlBuildRelativeURI(const xmlChar * URI, const xmlChar * base);
	XMLPUBFUN xmlURI * xmlParseURI(const char * str);
	XMLPUBFUN xmlURI * xmlParseURIRaw(const char * str, int raw);
	XMLPUBFUN int FASTCALL xmlParseURIReference(xmlURI * uri, const char * str);
	XMLPUBFUN xmlChar * xmlSaveUri(xmlURI * uri);
	XMLPUBFUN void xmlPrintURI(FILE * stream, xmlURI * uri);
	XMLPUBFUN xmlChar * FASTCALL xmlURIEscapeStr(const xmlChar * str, const xmlChar * list);
	XMLPUBFUN char * FASTCALL xmlURIUnescapeString(const char * str, int len, char * target);
	XMLPUBFUN int xmlNormalizeURIPath(char * path);
	XMLPUBFUN xmlChar * FASTCALL  xmlURIEscape(const xmlChar * str);
	XMLPUBFUN void FASTCALL xmlFreeURI(xmlURI * pUri);
	XMLPUBFUN xmlChar* xmlCanonicPath(const xmlChar * path);
	XMLPUBFUN xmlChar* xmlPathToURI(const xmlChar * path);
	xmlBuf * xmlBufCreate();
	xmlBuf * FASTCALL xmlBufCreateSize(size_t size);
	xmlBuf * FASTCALL xmlBufCreateStatic(void * mem, size_t size);
	int    FASTCALL xmlBufSetAllocationScheme(xmlBuf * buf, xmlBufferAllocationScheme scheme);
	int    FASTCALL xmlBufGetAllocationScheme(xmlBuf * buf);
	void   FASTCALL xmlBufFree(xmlBuf * buf);
	void   FASTCALL xmlBufEmpty(xmlBuf * buf);
	// size_t xmlBufShrink(xmlBufPtr buf, size_t len); 
	int    FASTCALL xmlBufGrow(xmlBuf * buf, int len);
	int    FASTCALL xmlBufInflate(xmlBuf * buf, size_t len);
	int    FASTCALL xmlBufResize(xmlBuf * buf, size_t len);
	int    STDCALL  xmlBufAdd(xmlBuf * buf, const xmlChar * str, int len);
	int    xmlBufAddHead(xmlBuf * buf, const xmlChar * str, int len);
	int    FASTCALL xmlBufCat(xmlBuf * buf, const xmlChar * str);
	int    FASTCALL xmlBufCCat(xmlBuf * buf, const char * str);
	int    FASTCALL xmlBufWriteCHAR(xmlBuf * buf, const xmlChar * string);
	int    FASTCALL xmlBufWriteChar(xmlBuf * buf, const char * string);
	int    FASTCALL xmlBufWriteQuotedString(xmlBuf * buf, const xmlChar * string);
	size_t FASTCALL xmlBufAvail(xmlBuf * buf);
	size_t FASTCALL xmlBufLength(xmlBuf * buf);
	// size_t xmlBufUse(const xmlBufPtr buf); 
	int    FASTCALL xmlBufIsEmpty(xmlBuf * buf);
	int    FASTCALL xmlBufAddLen(xmlBuf * buf, size_t len);
	int    FASTCALL xmlBufErase(xmlBuf * buf, size_t len);
	// const xmlChar * xmlBufContent(const xmlBuf *buf); 
	// const xmlChar * xmlBufEnd(xmlBufPtr buf); 
	xmlChar * FASTCALL xmlBufDetach(xmlBuf * pBuf);
	size_t xmlBufDump(FILE * file, xmlBuf * buf);
	xmlBuf * FASTCALL xmlBufFromBuffer(xmlBuffer * buffer);
	xmlBuffer * FASTCALL xmlBufBackToBuffer(xmlBuf * buf);
	int FASTCALL xmlBufMergeBuffer(xmlBuf * buf, xmlBuffer * buffer);
	int FASTCALL xmlBufResetInput(xmlBuf * buf, xmlParserInput * input);
	size_t FASTCALL xmlBufGetInputBase(xmlBuf * buf, const xmlParserInput * input);
	int FASTCALL xmlBufSetInputBaseCur(xmlBuf * buf, xmlParserInput * input, size_t base, size_t cur);
	int xmlCharEncFirstLineInt(xmlCharEncodingHandler *handler, xmlBuffer * out, xmlBuffer * in, int len);
	int xmlCharEncFirstLineInput(xmlParserInputBuffer * input, int len);
	int xmlCharEncInput(xmlParserInputBuffer * input, int flush);
	int FASTCALL xmlCharEncOutput(xmlOutputBuffer * output, int init);
	//
	#ifdef LIBXML_CATALOG_ENABLED
		#define XML_CATALOGS_NAMESPACE (const xmlChar *)"urn:oasis:names:tc:entity:xmlns:xml:catalog" // The namespace for the XML Catalogs elements
		#define XML_CATALOG_PI         (const xmlChar *)"oasis-xml-catalog" // The specific XML Catalog Processing Instuction name.
		// 
		// The API is voluntarily limited to general cataloging.
		// 
		enum xmlCatalogPrefer {
			XML_CATA_PREFER_NONE = 0,
			XML_CATA_PREFER_PUBLIC = 1,
			XML_CATA_PREFER_SYSTEM
		};

		enum xmlCatalogAllow {
			XML_CATA_ALLOW_NONE = 0,
			XML_CATA_ALLOW_GLOBAL = 1,
			XML_CATA_ALLOW_DOCUMENT = 2,
			XML_CATA_ALLOW_ALL = 3
		};

		typedef struct _xmlCatalog xmlCatalog;
		typedef xmlCatalog * xmlCatalogPtr;
		// 
		// Operations on a given catalog.
		// 
		XMLPUBFUN xmlCatalogPtr xmlNewCatalog(int sgml);
		XMLPUBFUN xmlCatalogPtr xmlLoadACatalog(const char * filename);
		XMLPUBFUN xmlCatalogPtr xmlLoadSGMLSuperCatalog(const char * filename);
		XMLPUBFUN int xmlConvertSGMLCatalog(xmlCatalogPtr catal);
		XMLPUBFUN int xmlACatalogAdd(xmlCatalogPtr catal, const xmlChar * type, const xmlChar * orig, const xmlChar * replace);
		XMLPUBFUN int xmlACatalogRemove(xmlCatalogPtr catal, const xmlChar * value);
		XMLPUBFUN xmlChar * xmlACatalogResolve(xmlCatalogPtr catal, const xmlChar * pubID, const xmlChar * sysID);
		XMLPUBFUN xmlChar * xmlACatalogResolveSystem(xmlCatalogPtr catal, const xmlChar * sysID);
		XMLPUBFUN xmlChar * xmlACatalogResolvePublic(xmlCatalogPtr catal, const xmlChar * pubID);
		XMLPUBFUN xmlChar * xmlACatalogResolveURI(xmlCatalogPtr catal, const xmlChar * URI);
		#ifdef LIBXML_OUTPUT_ENABLED
			XMLPUBFUN void xmlACatalogDump(xmlCatalogPtr catal, FILE * out);
		#endif
		XMLPUBFUN void xmlFreeCatalog(xmlCatalogPtr catal);
		XMLPUBFUN int xmlCatalogIsEmpty(xmlCatalogPtr catal);
		// 
		// Global operations.
		// 
		XMLPUBFUN void xmlInitializeCatalog();
		XMLPUBFUN int xmlLoadCatalog(const char * filename);
		XMLPUBFUN void xmlLoadCatalogs(const char * paths);
		XMLPUBFUN void xmlCatalogCleanup();
		#ifdef LIBXML_OUTPUT_ENABLED
			XMLPUBFUN void xmlCatalogDump(FILE * out);
		#endif
		XMLPUBFUN xmlChar * xmlCatalogResolve(const xmlChar * pubID, const xmlChar * sysID);
		XMLPUBFUN xmlChar * xmlCatalogResolveSystem(const xmlChar * sysID);
		XMLPUBFUN xmlChar * xmlCatalogResolvePublic(const xmlChar * pubID);
		XMLPUBFUN xmlChar * xmlCatalogResolveURI(const xmlChar * URI);
		XMLPUBFUN int xmlCatalogAdd(const xmlChar * type, const xmlChar * orig, const xmlChar * replace);
		XMLPUBFUN int xmlCatalogRemove(const xmlChar * value);
		XMLPUBFUN xmlDoc * xmlParseCatalogFile(const char * filename);
		XMLPUBFUN int xmlCatalogConvert();
		// 
		// Strictly minimal interfaces for per-document catalogs used by the parser.
		// 
		XMLPUBFUN void xmlCatalogFreeLocal(void * catalogs);
		XMLPUBFUN void * xmlCatalogAddLocal(void * catalogs, const xmlChar * URL);
		XMLPUBFUN xmlChar * xmlCatalogLocalResolve(void * catalogs, const xmlChar * pubID, const xmlChar * sysID);
		XMLPUBFUN xmlChar * xmlCatalogLocalResolveURI(void * catalogs, const xmlChar * URI);
		// 
		// Preference settings.
		// 
		XMLPUBFUN int xmlCatalogSetDebug(int level);
		XMLPUBFUN xmlCatalogPrefer xmlCatalogSetDefaultPrefer(xmlCatalogPrefer prefer);
		XMLPUBFUN void xmlCatalogSetDefaults(xmlCatalogAllow allow);
		XMLPUBFUN xmlCatalogAllow xmlCatalogGetDefaults();
		// 
		// DEPRECATED interfaces 
		// 
		XMLPUBFUN const xmlChar * xmlCatalogGetSystem(const xmlChar * sysID);
		XMLPUBFUN const xmlChar * xmlCatalogGetPublic(const xmlChar * pubID);
	#endif
	// save.h {
	#ifdef LIBXML_OUTPUT_ENABLED
		void xmlBufAttrSerializeTxtContent(xmlBuf * buf, xmlDoc * doc, xmlAttr * attr, const xmlChar * string);
		void xmlBufDumpNotationTable(xmlBuf * buf, xmlNotationTablePtr table);
		void xmlBufDumpElementDecl(xmlBuf * buf, xmlElement * elem);
		void xmlBufDumpAttributeDecl(xmlBuf * buf, xmlAttribute * attr);
		void xmlBufDumpEntityDecl(xmlBuf * buf, xmlEntity * ent);
		xmlChar *xmlEncodeAttributeEntities(xmlDoc * doc, const xmlChar *input);
	#endif
	//#ifdef __cplusplus
	//}
	//#endif
	// } save.h
	// } @sobolev
	//#ifdef LIBXML_XINCLUDE_ENABLED
		//#include <libxml/xinclude.h>
	//#endif
	//
	// Summary: implementation of XInclude
	// Description: API to handle XInclude processing, implements the
	// World Wide Web Consortium Last Call Working Draft 10 November 2003 http://www.w3.org/TR/2003/WD-xinclude-20031110
	// 
	// Copy: See Copyright for the status of this software.
	// Author: Daniel Veillard
	// 
	#ifndef __XML_XINCLUDE_H__
		#define __XML_XINCLUDE_H__
		#ifdef LIBXML_XINCLUDE_ENABLED
			//#ifdef __cplusplus
			//extern "C" {
			//#endif

			#define XINCLUDE_NS (const xmlChar *)"http://www.w3.org/2003/XInclude"
			#define XINCLUDE_OLD_NS (const xmlChar *)"http://www.w3.org/2001/XInclude"
			#define XINCLUDE_NODE (const xmlChar *)"include"
			#define XINCLUDE_FALLBACK (const xmlChar *)"fallback"
			#define XINCLUDE_HREF (const xmlChar *)"href"
			#define XINCLUDE_PARSE (const xmlChar *)"parse"
			#define XINCLUDE_PARSE_XML (const xmlChar *)"xml"
			#define XINCLUDE_PARSE_TEXT (const xmlChar *)"text"
			#define XINCLUDE_PARSE_ENCODING (const xmlChar *)"encoding"
			#define XINCLUDE_PARSE_XPOINTER (const xmlChar *)"xpointer"

			typedef struct _xmlXIncludeCtxt xmlXIncludeCtxt;
			typedef xmlXIncludeCtxt * xmlXIncludeCtxtPtr;
			// 
			// standalone processing
			// 
			XMLPUBFUN int xmlXIncludeProcess(xmlDoc * doc);
			XMLPUBFUN int xmlXIncludeProcessFlags(xmlDoc * doc, int flags);
			XMLPUBFUN int xmlXIncludeProcessFlagsData(xmlDoc * doc, int flags, void * data);
			XMLPUBFUN int xmlXIncludeProcessTreeFlagsData(xmlNode * tree, int flags, void * data);
			XMLPUBFUN int xmlXIncludeProcessTree(xmlNode * tree);
			XMLPUBFUN int xmlXIncludeProcessTreeFlags(xmlNode * tree, int flags);
			// 
			// contextual processing
			// 
			XMLPUBFUN xmlXIncludeCtxtPtr xmlXIncludeNewContext(xmlDoc * doc);
			XMLPUBFUN int xmlXIncludeSetFlags(xmlXIncludeCtxtPtr ctxt, int flags);
			XMLPUBFUN void FASTCALL xmlXIncludeFreeContext(xmlXIncludeCtxtPtr ctxt);
			XMLPUBFUN int xmlXIncludeProcessNode(xmlXIncludeCtxtPtr ctxt, xmlNode * tree);
			//#ifdef __cplusplus
			//}
			//#endif
		#endif
	#endif
	//
	#ifdef __GNUC__
		#ifdef PIC
			#ifdef linux
				#if(__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || (__GNUC__ > 3)
					//#include "elfgcchack.h"
				#endif
			#endif
		#endif
	#endif
#endif // IN_LIBXML
/* @sobolev
#if !defined(PIC) && !defined(NOLIBTOOL)
	#define LIBXML_STATIC
#endif
*/
#ifdef HAVE_LZMA_H
	//#include "xzlib.h"
	// 
	// xzlib.h: header for the front end for the transparent suport of lzma compression at the I/O layer
	// See Copyright for the status of this software.
	// Anders F Bjorklund <afb@users.sourceforge.net>
	// 
	typedef void * xzFile; // opaque lzma file descriptor 

	xzFile __libxml2_xzopen(const char *path, const char *mode);
	xzFile __libxml2_xzdopen(int fd, const char *mode);
	int __libxml2_xzread(xzFile file, void *buf, unsigned len);
	int __libxml2_xzclose(xzFile file);
	int __libxml2_xzcompressed(xzFile f);
#endif
#endif /* ! __XML_LIBXML_H__ */
