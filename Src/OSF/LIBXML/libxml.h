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
		#include "config.h"
	#endif
#endif
#if defined(__Lynx__)
	#include <stdio.h> /* pull definition of size_t */
	#include <varargs.h>
	int snprintf(char *, size_t, const char *, ...);
	int vfprintf(FILE *, const char *, va_list);
#endif
#ifndef WITH_TRIO
	//#include <stdio.h>
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
	// 
	// internal function of HTML parser needed for xmlParseInNodeContext but not part of the API
	// 
	void __htmlParseContent(void *ctx);
#endif
// 
// internal global initialization critical section routines.
// 
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
	//#ifdef HAVE_CTYPE_H
		//#include <ctype.h>
	//#endif
	//#ifdef HAVE_ERRNO_H
		//#include <errno.h>
	//#endif
	//#ifdef HAVE_TIME_H
		//#include <time.h>
	//#endif
	//#if defined(_WIN32) && defined(_MSC_VER)
		//#include <windows.h>
	//#endif
	#ifdef HAVE_UNISTD_H
		#include <unistd.h>
	#endif
	//
	//#include <libxml/xmlstring.h>
	//
	#ifdef __cplusplus
	extern "C" {
	#endif
	/*
	 * xmlChar handling
	 */
	//XMLPUBFUN xmlChar * XMLCALL xmlStrdup_Removed(const xmlChar * cur);
	XMLPUBFUN xmlChar * /*XMLCALL*/FASTCALL xmlStrndup(const xmlChar * cur, /*int*/SSIZE_T len);
	XMLPUBFUN xmlChar * XMLCALL xmlCharStrndup(const char * cur, int len);
	XMLPUBFUN xmlChar * XMLCALL xmlCharStrdup(const char * cur);
	XMLPUBFUN xmlChar * XMLCALL xmlStrsub(const xmlChar * str, int start, int len);
	XMLPUBFUN const xmlChar * /*XMLCALL*/FASTCALL xmlStrchr(const xmlChar * str, xmlChar val);
	XMLPUBFUN const xmlChar * /*XMLCALL*/FASTCALL xmlStrstr(const xmlChar * str, const xmlChar * val);
	XMLPUBFUN const xmlChar * XMLCALL xmlStrcasestr(const xmlChar * str, const xmlChar * val);
	XMLPUBFUN int XMLCALL xmlStrcmp(const xmlChar * str1, const xmlChar * str2);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlStrncmp(const xmlChar * str1, const xmlChar * str2, int len);
	XMLPUBFUN int XMLCALL xmlStrcasecmp(const xmlChar * str1, const xmlChar * str2);
	XMLPUBFUN int XMLCALL xmlStrncasecmp(const xmlChar * str1, const xmlChar * str2, int len);
	//XMLPUBFUN int XMLCALL xmlStrEqual_Removed(const xmlChar * str1, const xmlChar * str2);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlStrQEqual(const xmlChar * pref, const xmlChar * name, const xmlChar * str);
	//XMLPUBFUN int XMLCALL xmlStrlen_Removed(const xmlChar * str);
	XMLPUBFUN xmlChar * /*XMLCALL*/FASTCALL xmlStrcat(xmlChar * cur, const xmlChar * add);
	XMLPUBFUN xmlChar * XMLCALL xmlStrncat(xmlChar * cur, const xmlChar * add, int len);
	XMLPUBFUN xmlChar * XMLCALL xmlStrncatNew(const xmlChar * str1, const xmlChar * str2, int len);
	XMLPUBFUN int XMLCALL xmlStrPrintf(xmlChar * buf, int len, const xmlChar * msg,...);
	XMLPUBFUN int XMLCALL xmlStrVPrintf(xmlChar * buf, int len, const xmlChar * msg, va_list ap);
	XMLPUBFUN int XMLCALL xmlGetUTF8Char(const uchar * utf, int * len);
	XMLPUBFUN int XMLCALL xmlCheckUTF8(const uchar * utf);
	XMLPUBFUN int XMLCALL xmlUTF8Strsize(const xmlChar * utf, int len);
	XMLPUBFUN xmlChar * XMLCALL xmlUTF8Strndup(const xmlChar * utf, int len);
	XMLPUBFUN const xmlChar * XMLCALL xmlUTF8Strpos(const xmlChar * utf, int pos);
	XMLPUBFUN int XMLCALL xmlUTF8Strloc(const xmlChar * utf, const xmlChar * utfchar);
	XMLPUBFUN xmlChar * XMLCALL xmlUTF8Strsub(const xmlChar * utf, int start, int len);
	XMLPUBFUN int /*XMLCALL*/FASTCALL  xmlUTF8Strlen(const xmlChar * utf);
	XMLPUBFUN int XMLCALL xmlUTF8Size(const xmlChar * utf);
	XMLPUBFUN int XMLCALL xmlUTF8Charcmp(const xmlChar * utf1, const xmlChar * utf2);

	#ifdef __cplusplus
	}
	#endif

	struct xmlHashTable;
	struct xmlValidCtxt;
	//
	#include <libxml/xmlversion.h>
	#include <libxml/xmlerror.h>
	#include <libxml/tree.h>
	#include <libxml/list.h>
	#include <libxml/xmlautomata.h>
	#include <libxml/valid.h>
	#include <libxml/parser.h>
	#include <libxml/xmlmemory.h>
	#include <libxml/encoding.h>
	#include <libxml/globals.h>
	//#include <libxml/hash.h>
	#ifdef __cplusplus
	extern "C" {
	#endif
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
	// 
	// Constructor and destructor.
	// 
	XMLPUBFUN xmlHashTable * /*XMLCALL*/FASTCALL xmlHashCreate(int size);
	XMLPUBFUN xmlHashTable * /*XMLCALL*/FASTCALL xmlHashCreateDict(int size, xmlDict * dict);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlHashFree(xmlHashTable * pTable, xmlHashDeallocator f);
	// 
	// Add a new entry to the hash table.
	// 
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlHashAddEntry(xmlHashTable * table, const xmlChar *name, void *userdata);
	XMLPUBFUN int XMLCALL xmlHashUpdateEntry(xmlHashTable * table, const xmlChar *name, void *userdata, xmlHashDeallocator f);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlHashAddEntry2(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, void *userdata);
	XMLPUBFUN int XMLCALL xmlHashUpdateEntry2(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, void *userdata, xmlHashDeallocator f);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlHashAddEntry3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3, void *userdata);
	XMLPUBFUN int XMLCALL xmlHashUpdateEntry3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3, void *userdata, xmlHashDeallocator f);
	// 
	// Remove an entry from the hash table.
	// 
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlHashRemoveEntry(xmlHashTable * table, const xmlChar *name, xmlHashDeallocator f);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlHashRemoveEntry2(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, xmlHashDeallocator f);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlHashRemoveEntry3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3,xmlHashDeallocator f);
	// 
	// Retrieve the userdata.
	// 
	XMLPUBFUN void * /*XMLCALL*/FASTCALL xmlHashLookup(xmlHashTable * table, const xmlChar *name);
	XMLPUBFUN void * /*XMLCALL*/FASTCALL xmlHashLookup2(xmlHashTable * table, const xmlChar *name, const xmlChar *name2);
	XMLPUBFUN void * /*XMLCALL*/FASTCALL xmlHashLookup3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3);
	XMLPUBFUN void * XMLCALL xmlHashQLookup(xmlHashTable * table, const xmlChar *name, const xmlChar *prefix);
	XMLPUBFUN void * XMLCALL xmlHashQLookup2(xmlHashTable * table, const xmlChar *name, const xmlChar *prefix, const xmlChar *name2, const xmlChar *prefix2);
	XMLPUBFUN void * XMLCALL xmlHashQLookup3(xmlHashTable * table, const xmlChar *name, const xmlChar *prefix, const xmlChar *name2, const xmlChar *prefix2, const xmlChar *name3, const xmlChar *prefix3);
	// 
	// Helpers.
	// 
	XMLPUBFUN xmlHashTable * XMLCALL xmlHashCopy(const xmlHashTable * table, xmlHashCopier f);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlHashSize(xmlHashTable * table);
	XMLPUBFUN void XMLCALL xmlHashScan(xmlHashTable * table, xmlHashScanner f, void *data);
	XMLPUBFUN void XMLCALL xmlHashScan3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3, xmlHashScanner f, void *data);
	XMLPUBFUN void XMLCALL xmlHashScanFull(xmlHashTable * table, xmlHashScannerFull f, void *data);
	XMLPUBFUN void XMLCALL xmlHashScanFull3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3, xmlHashScannerFull f, void *data);
	#ifdef __cplusplus
	}
	#endif
	//
	//#include <libxml/xmlregexp.h>
	#ifdef LIBXML_REGEXP_ENABLED
		#ifdef __cplusplus
		extern "C" {
		#endif
		// 
		// Descr: A libxml regular expression, they can actually be far more complex
		//   thank the POSIX regex expressions.
		// 
		struct xmlRegexp;
		typedef xmlRegexp * xmlRegexpPtr;
		// 
		// A libxml progressive regular expression evaluation context
		// 
		typedef struct _xmlRegExecCtxt xmlRegExecCtxt;
		typedef xmlRegExecCtxt *xmlRegExecCtxtPtr;
		// 
		// The POSIX like API
		// 
		XMLPUBFUN xmlRegexp * XMLCALL xmlRegexpCompile(const xmlChar *regexp);
		XMLPUBFUN void XMLCALL xmlRegFreeRegexp(xmlRegexp * regexp);
		XMLPUBFUN int XMLCALL xmlRegexpExec(xmlRegexpPtr comp, const xmlChar *value);
		XMLPUBFUN void XMLCALL xmlRegexpPrint(FILE *output, xmlRegexpPtr regexp);
		XMLPUBFUN int XMLCALL xmlRegexpIsDeterminist(xmlRegexpPtr comp);
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
		XMLPUBFUN xmlRegExecCtxtPtr XMLCALL xmlRegNewExecCtxt(xmlRegexpPtr comp, xmlRegExecCallbacks callback, void *data);
		XMLPUBFUN void XMLCALL xmlRegFreeExecCtxt(xmlRegExecCtxtPtr exec); 
		XMLPUBFUN int XMLCALL xmlRegExecPushString(xmlRegExecCtxtPtr exec, const xmlChar *value, void *data);
		XMLPUBFUN int XMLCALL xmlRegExecPushString2(xmlRegExecCtxtPtr exec, const xmlChar *value, const xmlChar *value2, void *data);
		XMLPUBFUN int XMLCALL xmlRegExecNextValues(xmlRegExecCtxtPtr exec, int *nbval, int *nbneg, xmlChar **values, int *terminal);
		XMLPUBFUN int XMLCALL xmlRegExecErrInfo(xmlRegExecCtxtPtr exec, const xmlChar **string, int *nbval, int *nbneg, xmlChar **values, int *terminal);
		#ifdef LIBXML_EXPR_ENABLED
			// 
			// Formal regular expression handling
			// Its goal is to do some formal work on content models
			// 
			// expressions are used within a context 
			typedef struct _xmlExpCtxt xmlExpCtxt;
			typedef xmlExpCtxt *xmlExpCtxtPtr;

			XMLPUBFUN void XMLCALL xmlExpFreeCtxt(xmlExpCtxtPtr ctxt);
			XMLPUBFUN xmlExpCtxtPtr XMLCALL xmlExpNewCtxt(int maxNodes, xmlDict * dict);
			XMLPUBFUN int XMLCALL xmlExpCtxtNbNodes(xmlExpCtxtPtr ctxt);
			XMLPUBFUN int XMLCALL xmlExpCtxtNbCons(xmlExpCtxtPtr ctxt);

			// Expressions are trees but the tree is opaque 
			typedef struct _xmlExpNode xmlExpNode;
			typedef xmlExpNode *xmlExpNodePtr;

			typedef enum {
				XML_EXP_EMPTY = 0,
				XML_EXP_FORBID = 1,
				XML_EXP_ATOM = 2,
				XML_EXP_SEQ = 3,
				XML_EXP_OR = 4,
				XML_EXP_COUNT = 5
			} xmlExpNodeType;
			/*
			 * 2 core expressions shared by all for the empty language set
			 * and for the set with just the empty token
			 */
			XMLPUBVAR xmlExpNodePtr forbiddenExp;
			XMLPUBVAR xmlExpNodePtr emptyExp;
			/*
			 * Expressions are reference counted internally
			 */
			XMLPUBFUN void /*XMLCALL*/FASTCALL xmlExpFree(xmlExpCtxt * pCtxt, xmlExpNode * pExpr);
			XMLPUBFUN void XMLCALL xmlExpRef(xmlExpNodePtr expr);
			/*
			 * constructors can be either manual or from a string
			 */
			XMLPUBFUN xmlExpNodePtr XMLCALL xmlExpParse(xmlExpCtxtPtr ctxt, const char *expr);
			XMLPUBFUN xmlExpNodePtr XMLCALL xmlExpNewAtom(xmlExpCtxtPtr ctxt, const xmlChar *name, int len);
			XMLPUBFUN xmlExpNodePtr XMLCALL xmlExpNewOr(xmlExpCtxtPtr ctxt, xmlExpNodePtr left, xmlExpNodePtr right);
			XMLPUBFUN xmlExpNodePtr XMLCALL xmlExpNewSeq(xmlExpCtxtPtr ctxt, xmlExpNodePtr left, xmlExpNodePtr right);
			XMLPUBFUN xmlExpNodePtr XMLCALL xmlExpNewRange(xmlExpCtxtPtr ctxt, xmlExpNodePtr subset, int min, int max);
			/*
			 * The really interesting APIs
			 */
			XMLPUBFUN int XMLCALL xmlExpIsNillable(xmlExpNodePtr expr);
			XMLPUBFUN int XMLCALL xmlExpMaxToken(xmlExpNodePtr expr);
			XMLPUBFUN int XMLCALL xmlExpGetLanguage(xmlExpCtxtPtr ctxt, xmlExpNodePtr expr, const xmlChar**langList, int len);
			XMLPUBFUN int XMLCALL xmlExpGetStart(xmlExpCtxtPtr ctxt, xmlExpNodePtr expr, const xmlChar**tokList, int len);
			XMLPUBFUN xmlExpNodePtr XMLCALL xmlExpStringDerive(xmlExpCtxtPtr ctxt, xmlExpNodePtr expr, const xmlChar *str, int len);
			XMLPUBFUN xmlExpNodePtr XMLCALL xmlExpExpDerive	(xmlExpCtxtPtr ctxt, xmlExpNodePtr expr, xmlExpNodePtr sub);
			XMLPUBFUN int XMLCALL xmlExpSubsume(xmlExpCtxtPtr ctxt, xmlExpNodePtr expr, xmlExpNodePtr sub);
			XMLPUBFUN void XMLCALL xmlExpDump(xmlBuffer * buf, xmlExpNodePtr expr);
		#endif // LIBXML_EXPR_ENABLED 
		#ifdef __cplusplus
		}
		#endif
	#endif // LIBXML_REGEXP_ENABLED 
	//
	//#include <libxml/tree.h>
	//#include <libxml/uri.h>
	//#include <libxml/encoding.h>
	#include <libxml/xmlIO.h>
	//#include <libxml/list.h>
	//#include <libxml/xmlautomata.h>
	//#include <libxml/valid.h>
	//#include <libxml/entities.h>
	#ifdef __cplusplus
	extern "C" {
	#endif
	// 
	// The different valid entity types.
	// 
	typedef enum {
		XML_INTERNAL_GENERAL_ENTITY = 1,
		XML_EXTERNAL_GENERAL_PARSED_ENTITY = 2,
		XML_EXTERNAL_GENERAL_UNPARSED_ENTITY = 3,
		XML_INTERNAL_PARAMETER_ENTITY = 4,
		XML_EXTERNAL_PARAMETER_ENTITY = 5,
		XML_INTERNAL_PREDEFINED_ENTITY = 6
	} xmlEntityType;
	// 
	// An unit of storage for an entity, contains the string, the value
	// and the linkind data needed for the linking in the hash table.
	// 
	struct xmlEntity {
		void * _private; // application data 
		xmlElementType type;  // XML_ENTITY_DECL, must be second ! 
		const xmlChar * name; // Entity name 
		xmlNode * children;   // First child link 
		xmlNode * last;  /* Last child link */
		xmlDtd  * parent;  /* -> DTD */
		xmlNode * next;  /* next sibling link  */
		xmlNode * prev;  /* previous sibling link  */
		xmlDoc  * doc;  /* the containing document */
		const xmlChar * orig;  /* content without ref substitution */
		const xmlChar * content;  /* content or ndata if unparsed */
		int    length;                     /* the content length */
		xmlEntityType etype;            /* The entity type */
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
		XMLPUBFUN void XMLCALL xmlInitializePredefinedEntities();
	#endif /* LIBXML_LEGACY_ENABLED */
	XMLPUBFUN xmlEntity * XMLCALL xmlNewEntity(xmlDoc * doc, const xmlChar * name, int type, const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content);
	XMLPUBFUN xmlEntity * XMLCALL xmlAddDocEntity(xmlDoc * doc, const xmlChar * name, int type, const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content);
	XMLPUBFUN xmlEntity * XMLCALL xmlAddDtdEntity(xmlDoc * doc, const xmlChar * name, int type, const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content);
	XMLPUBFUN xmlEntity * /*XMLCALL*/FASTCALL  xmlGetPredefinedEntity(const xmlChar * name);
	XMLPUBFUN xmlEntity * /*XMLCALL*/FASTCALL xmlGetDocEntity(const xmlDoc * doc, const xmlChar * name);
	XMLPUBFUN xmlEntity * XMLCALL xmlGetDtdEntity(xmlDoc * doc, const xmlChar * name);
	XMLPUBFUN xmlEntity * XMLCALL xmlGetParameterEntity(xmlDoc * doc, const xmlChar * name);
	#ifdef LIBXML_LEGACY_ENABLED
		XMLPUBFUN const xmlChar * XMLCALL xmlEncodeEntities(xmlDoc * doc, const xmlChar * input);
	#endif /* LIBXML_LEGACY_ENABLED */
	XMLPUBFUN xmlChar * XMLCALL xmlEncodeEntitiesReentrant(xmlDoc * doc, const xmlChar * input);
	XMLPUBFUN xmlChar * XMLCALL xmlEncodeSpecialChars(const xmlDoc * doc, const xmlChar * input);
	XMLPUBFUN xmlEntitiesTablePtr XMLCALL xmlCreateEntitiesTable();
	#ifdef LIBXML_TREE_ENABLED
		XMLPUBFUN xmlEntitiesTablePtr XMLCALL xmlCopyEntitiesTable(const xmlEntitiesTablePtr table);
	#endif /* LIBXML_TREE_ENABLED */
	XMLPUBFUN void XMLCALL  xmlFreeEntitiesTable(xmlEntitiesTablePtr table);
	#ifdef LIBXML_OUTPUT_ENABLED
		XMLPUBFUN void XMLCALL  xmlDumpEntitiesTable(xmlBuffer * buf, xmlEntitiesTablePtr table);
		XMLPUBFUN void XMLCALL  xmlDumpEntityDecl(xmlBuffer * buf, xmlEntity * ent);
	#endif /* LIBXML_OUTPUT_ENABLED */
	#ifdef LIBXML_LEGACY_ENABLED
		XMLPUBFUN void XMLCALL  xmlCleanupPredefinedEntities();
	#endif /* LIBXML_LEGACY_ENABLED */
	#ifdef __cplusplus
	}
	#endif
	//
	//#include <libxml/parser.h>
	#include <libxml/parserInternals.h>
	//
	//#include <libxml/threads.h>
	//
	#ifdef __cplusplus
	extern "C" {
	#endif

	struct xmlMutex;  // xmlMutex are a simple mutual exception locks.
	struct xmlRMutex; // xmlRMutex are reentrant mutual exception locks.

	XMLPUBFUN xmlMutex * XMLCALL xmlNewMutex();
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlMutexLock(xmlMutex * tok);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlMutexUnlock(xmlMutex * tok);
	XMLPUBFUN void XMLCALL xmlFreeMutex(xmlMutex * tok);
	XMLPUBFUN xmlRMutex * XMLCALL xmlNewRMutex();
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlRMutexLock(xmlRMutex * tok);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlRMutexUnlock(xmlRMutex * tok);
	XMLPUBFUN void XMLCALL xmlFreeRMutex(xmlRMutex * tok);
	// 
	// Library wide APIs.
	// 
	XMLPUBFUN void XMLCALL xmlInitThreads();
	XMLPUBFUN void XMLCALL xmlLockLibrary();
	XMLPUBFUN void XMLCALL xmlUnlockLibrary();
	XMLPUBFUN int XMLCALL xmlGetThreadId();
	XMLPUBFUN int XMLCALL xmlIsMainThread();
	XMLPUBFUN void XMLCALL xmlCleanupThreads();
	XMLPUBFUN xmlGlobalState * XMLCALL xmlGetGlobalState();
	#if defined(HAVE_WIN32_THREADS) && !defined(HAVE_COMPILER_TLS) && defined(LIBXML_STATIC_FOR_DLL)
		int XMLCALL xmlDllMain(void *hinstDLL, unsigned long fdwReason, void *lpvReserved);
	#endif
	//
	//#include <libxml/xlink.h>
	//
	#ifdef LIBXML_XPTR_ENABLED
		/**
		 * Various defines for the various Link properties.
		 *
		 * NOTE: the link detection layer will try to resolve QName expansion
		 *     of namespaces. If "foo" is the prefix for "http://foo.com/"
		 *     then the link detection layer will expand role="foo:myrole"
		 *     to "http://foo.com/:myrole".
		 * NOTE: the link detection layer will expand URI-Refences found on
		 *     href attributes by using the base mechanism if found.
		 */
		typedef xmlChar * xlinkHRef;
		typedef xmlChar * xlinkRole;
		typedef xmlChar * xlinkTitle;

		typedef enum {
			XLINK_TYPE_NONE = 0,
			XLINK_TYPE_SIMPLE,
			XLINK_TYPE_EXTENDED,
			XLINK_TYPE_EXTENDED_SET
		} xlinkType;

		typedef enum {
			XLINK_SHOW_NONE = 0,
			XLINK_SHOW_NEW,
			XLINK_SHOW_EMBED,
			XLINK_SHOW_REPLACE
		} xlinkShow;

		typedef enum {
			XLINK_ACTUATE_NONE = 0,
			XLINK_ACTUATE_AUTO,
			XLINK_ACTUATE_ONREQUEST
		} xlinkActuate;
		/**
		 * xlinkNodeDetectFunc:
		 * @ctx:  user data pointer
		 * @node:  the node to check
		 *
		 * This is the prototype for the link detection routine.
		 * It calls the default link detection callbacks upon link detection.
		 */
		typedef void (*xlinkNodeDetectFunc)(void * ctx, xmlNode * P_Node);
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
		typedef void (*xlinkSimpleLinkFunk)(void * ctx, xmlNode * P_Node, const xlinkHRef href, const xlinkRole role, const xlinkTitle title);
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
		typedef void (*xlinkExtendedLinkFunk)(void * ctx, xmlNode * P_Node, int nbLocators, const xlinkHRef * hrefs, const xlinkRole * roles,
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
		typedef void (*xlinkExtendedLinkSetFunk)(void * ctx, xmlNode * P_Node, int nbLocators, const xlinkHRef * hrefs,
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
		XMLPUBFUN xlinkNodeDetectFunc XMLCALL xlinkGetDefaultDetect();
		XMLPUBFUN void XMLCALL xlinkSetDefaultDetect(xlinkNodeDetectFunc func);
		// 
		// Routines to set/get the default handlers.
		// 
		XMLPUBFUN xlinkHandlerPtr XMLCALL xlinkGetDefaultHandler();
		XMLPUBFUN void XMLCALL xlinkSetDefaultHandler(xlinkHandlerPtr handler);
		// 
		// Link detection module itself.
		// 
		XMLPUBFUN xlinkType XMLCALL xlinkIsLink(xmlDoc * doc, xmlNode * P_Node);
	#endif

	#ifdef __cplusplus
	}
	#endif
	//
	#include <libxml/SAX.h>
	#include <libxml/SAX2.h>
	#include <libxml/xpath.h>
	#include <libxml/xpointer.h>
	#include <libxml/xpathInternals.h>
	#include <libxml/c14n.h>
	#include <libxml/pattern.h>
	#include <libxml/schematron.h>
	#include <libxml/debugXML.h>
	#include <libxml/chvalid.h>
	#include <libxml/HTMLparser.h>
	#include <libxml/HTMLtree.h>

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
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlDictReference(xmlDict * dict);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlDictFree(xmlDict * dict);
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
	// Descr: Free a string if it is not owned by the "dict" dictionnary in the current scope
	//
	void FASTCALL XmlDestroyStringWithDict(xmlDict * pDict, xmlChar * pStr);
	// 
	// Cleanup function
	//
	XMLPUBFUN void XMLCALL xmlDictCleanup();
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
		int    port;   // the port number 
		char * path;    /* the path string */
		char * query;   /* the query string (deprecated - use with caution) */
		char * fragment; /* the fragment identifier */
		int    cleanup;    /* parsing potentially unclean URI */
		char * query_raw; /* the query string (as it appears in the URI) */
	};

	typedef xmlURI * xmlURIPtr;
	// 
	// This function is in tree.h:
	// xmlChar * xmlNodeGetBase(xmlDoc * doc, xmlNode * cur);
	// 
	XMLPUBFUN xmlURIPtr XMLCALL xmlCreateURI();
	XMLPUBFUN xmlChar * XMLCALL xmlBuildURI(const xmlChar * URI, const xmlChar * base);
	XMLPUBFUN xmlChar * XMLCALL xmlBuildRelativeURI(const xmlChar * URI, const xmlChar * base);
	XMLPUBFUN xmlURIPtr XMLCALL xmlParseURI(const char * str);
	XMLPUBFUN xmlURIPtr XMLCALL xmlParseURIRaw(const char * str, int raw);
	XMLPUBFUN int XMLCALL xmlParseURIReference(xmlURIPtr uri, const char * str);
	XMLPUBFUN xmlChar * XMLCALL xmlSaveUri(xmlURIPtr uri);
	XMLPUBFUN void XMLCALL xmlPrintURI(FILE * stream, xmlURIPtr uri);
	XMLPUBFUN xmlChar * /*XMLCALL*/FASTCALL xmlURIEscapeStr(const xmlChar * str, const xmlChar * list);
	XMLPUBFUN char * /*XMLCALL*/FASTCALL xmlURIUnescapeString(const char * str, int len, char * target);
	XMLPUBFUN int XMLCALL xmlNormalizeURIPath(char * path);
	XMLPUBFUN xmlChar * /*XMLCALL*/FASTCALL  xmlURIEscape(const xmlChar * str);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeURI(xmlURI * pUri);
	XMLPUBFUN xmlChar* XMLCALL xmlCanonicPath(const xmlChar * path);
	XMLPUBFUN xmlChar* XMLCALL xmlPathToURI(const xmlChar * path);
	//
	//#include "buf.h"
	//
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
	int    FASTCALL xmlBufAdd(xmlBuf * buf, const xmlChar * str, int len);
	int    xmlBufAddHead(xmlBufPtr buf, const xmlChar * str, int len);
	int    FASTCALL xmlBufCat(xmlBuf * buf, const xmlChar * str);
	int    FASTCALL xmlBufCCat(xmlBuf * buf, const char * str);
	int    FASTCALL xmlBufWriteCHAR(xmlBuf * buf, const xmlChar * string);
	int    FASTCALL xmlBufWriteChar(xmlBuf * buf, const char * string);
	int    FASTCALL xmlBufWriteQuotedString(xmlBufPtr buf, const xmlChar * string);
	size_t FASTCALL xmlBufAvail(xmlBuf * buf);
	size_t FASTCALL xmlBufLength(xmlBuf * buf);
	// size_t xmlBufUse(const xmlBufPtr buf); 
	int    FASTCALL xmlBufIsEmpty(xmlBuf * buf);
	int    FASTCALL xmlBufAddLen(xmlBuf * buf, size_t len);
	int    FASTCALL xmlBufErase(xmlBuf * buf, size_t len);
	// const xmlChar * xmlBufContent(const xmlBuf *buf); 
	// const xmlChar * xmlBufEnd(xmlBufPtr buf); 
	xmlChar * FASTCALL xmlBufDetach(xmlBuf * pBuf);
	size_t xmlBufDump(FILE * file, xmlBufPtr buf);
	xmlBuf * FASTCALL xmlBufFromBuffer(xmlBuffer * buffer);
	xmlBuffer * FASTCALL xmlBufBackToBuffer(xmlBuf * buf);
	int FASTCALL xmlBufMergeBuffer(xmlBuf * buf, xmlBuffer * buffer);
	int FASTCALL xmlBufResetInput(xmlBuf * buf, xmlParserInput * input);
	size_t FASTCALL xmlBufGetInputBase(xmlBuf * buf, const xmlParserInput * input);
	int FASTCALL xmlBufSetInputBaseCur(xmlBufPtr buf, xmlParserInput * input, size_t base, size_t cur);
	//
	//#include "enc.h"
	//
	int xmlCharEncFirstLineInt(xmlCharEncodingHandler *handler, xmlBuffer * out, xmlBuffer * in, int len);
	int xmlCharEncFirstLineInput(xmlParserInputBuffer * input, int len);
	int xmlCharEncInput(xmlParserInputBuffer * input, int flush);
	int FASTCALL xmlCharEncOutput(xmlOutputBuffer * output, int init);
	//
	#ifdef LIBXML_CATALOG_ENABLED
		//
		//#include <libxml/catalog.h>
		//
		#define XML_CATALOGS_NAMESPACE (const xmlChar *)"urn:oasis:names:tc:entity:xmlns:xml:catalog" // The namespace for the XML Catalogs elements
		#define XML_CATALOG_PI         (const xmlChar *)"oasis-xml-catalog" // The specific XML Catalog Processing Instuction name.
		// 
		// The API is voluntarily limited to general cataloging.
		// 
		typedef enum {
			XML_CATA_PREFER_NONE = 0,
			XML_CATA_PREFER_PUBLIC = 1,
			XML_CATA_PREFER_SYSTEM
		} xmlCatalogPrefer;

		typedef enum {
			XML_CATA_ALLOW_NONE = 0,
			XML_CATA_ALLOW_GLOBAL = 1,
			XML_CATA_ALLOW_DOCUMENT = 2,
			XML_CATA_ALLOW_ALL = 3
		} xmlCatalogAllow;

		typedef struct _xmlCatalog xmlCatalog;
		typedef xmlCatalog * xmlCatalogPtr;
		// 
		// Operations on a given catalog.
		// 
		XMLPUBFUN xmlCatalogPtr XMLCALL xmlNewCatalog(int sgml);
		XMLPUBFUN xmlCatalogPtr XMLCALL xmlLoadACatalog(const char * filename);
		XMLPUBFUN xmlCatalogPtr XMLCALL xmlLoadSGMLSuperCatalog(const char * filename);
		XMLPUBFUN int XMLCALL xmlConvertSGMLCatalog(xmlCatalogPtr catal);
		XMLPUBFUN int XMLCALL xmlACatalogAdd(xmlCatalogPtr catal, const xmlChar * type, const xmlChar * orig, const xmlChar * replace);
		XMLPUBFUN int XMLCALL xmlACatalogRemove(xmlCatalogPtr catal, const xmlChar * value);
		XMLPUBFUN xmlChar * XMLCALL xmlACatalogResolve(xmlCatalogPtr catal, const xmlChar * pubID, const xmlChar * sysID);
		XMLPUBFUN xmlChar * XMLCALL xmlACatalogResolveSystem(xmlCatalogPtr catal, const xmlChar * sysID);
		XMLPUBFUN xmlChar * XMLCALL xmlACatalogResolvePublic(xmlCatalogPtr catal, const xmlChar * pubID);
		XMLPUBFUN xmlChar * XMLCALL xmlACatalogResolveURI(xmlCatalogPtr catal, const xmlChar * URI);
		#ifdef LIBXML_OUTPUT_ENABLED
			XMLPUBFUN void XMLCALL xmlACatalogDump(xmlCatalogPtr catal, FILE * out);
		#endif
		XMLPUBFUN void XMLCALL xmlFreeCatalog(xmlCatalogPtr catal);
		XMLPUBFUN int XMLCALL xmlCatalogIsEmpty(xmlCatalogPtr catal);
		// 
		// Global operations.
		// 
		XMLPUBFUN void XMLCALL xmlInitializeCatalog();
		XMLPUBFUN int XMLCALL xmlLoadCatalog(const char * filename);
		XMLPUBFUN void XMLCALL xmlLoadCatalogs(const char * paths);
		XMLPUBFUN void XMLCALL xmlCatalogCleanup();
		#ifdef LIBXML_OUTPUT_ENABLED
			XMLPUBFUN void XMLCALL xmlCatalogDump(FILE * out);
		#endif
		XMLPUBFUN xmlChar * XMLCALL xmlCatalogResolve(const xmlChar * pubID, const xmlChar * sysID);
		XMLPUBFUN xmlChar * XMLCALL xmlCatalogResolveSystem(const xmlChar * sysID);
		XMLPUBFUN xmlChar * XMLCALL xmlCatalogResolvePublic(const xmlChar * pubID);
		XMLPUBFUN xmlChar * XMLCALL xmlCatalogResolveURI(const xmlChar * URI);
		XMLPUBFUN int XMLCALL xmlCatalogAdd(const xmlChar * type, const xmlChar * orig, const xmlChar * replace);
		XMLPUBFUN int XMLCALL xmlCatalogRemove(const xmlChar * value);
		XMLPUBFUN xmlDoc * XMLCALL xmlParseCatalogFile(const char * filename);
		XMLPUBFUN int XMLCALL xmlCatalogConvert();
		// 
		// Strictly minimal interfaces for per-document catalogs used by the parser.
		// 
		XMLPUBFUN void XMLCALL xmlCatalogFreeLocal(void * catalogs);
		XMLPUBFUN void * XMLCALL xmlCatalogAddLocal(void * catalogs, const xmlChar * URL);
		XMLPUBFUN xmlChar * XMLCALL xmlCatalogLocalResolve(void * catalogs, const xmlChar * pubID, const xmlChar * sysID);
		XMLPUBFUN xmlChar * XMLCALL xmlCatalogLocalResolveURI(void * catalogs, const xmlChar * URI);
		// 
		// Preference settings.
		// 
		XMLPUBFUN int XMLCALL xmlCatalogSetDebug(int level);
		XMLPUBFUN xmlCatalogPrefer XMLCALL xmlCatalogSetDefaultPrefer(xmlCatalogPrefer prefer);
		XMLPUBFUN void XMLCALL xmlCatalogSetDefaults(xmlCatalogAllow allow);
		XMLPUBFUN xmlCatalogAllow XMLCALL xmlCatalogGetDefaults();
		// 
		// DEPRECATED interfaces 
		// 
		XMLPUBFUN const xmlChar * XMLCALL xmlCatalogGetSystem(const xmlChar * sysID);
		XMLPUBFUN const xmlChar * XMLCALL xmlCatalogGetPublic(const xmlChar * pubID);
	#endif
	#ifdef __cplusplus
	}
	#endif
	// } @sobolev
	#ifdef __GNUC__
		#ifdef PIC
			#ifdef linux
				#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || (__GNUC__ > 3)
					//#include "elfgcchack.h"
				#endif
			#endif
		#endif
	#endif
	#ifdef HAVE_ZLIB_H
		#include <zlib.h>
	#endif
	#ifdef HAVE_LZMA_H
		#include <lzma.h>
	#endif
#endif // IN_LIBXML
/* @sobolev
#if !defined(PIC) && !defined(NOLIBTOOL)
	#define LIBXML_STATIC
#endif
*/
#endif /* ! __XML_LIBXML_H__ */
