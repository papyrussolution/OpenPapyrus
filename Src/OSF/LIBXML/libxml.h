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
		#include "config.h"
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
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlGetUTF8Char(const uchar * utf, int * len);
	XMLPUBFUN int XMLCALL xmlCheckUTF8(const uchar * utf);
	XMLPUBFUN int XMLCALL xmlUTF8Strsize(const xmlChar * utf, int len);
	XMLPUBFUN xmlChar * XMLCALL xmlUTF8Strndup(const xmlChar * utf, int len);
	XMLPUBFUN const xmlChar * XMLCALL xmlUTF8Strpos(const xmlChar * utf, int pos);
	XMLPUBFUN int XMLCALL xmlUTF8Strloc(const xmlChar * utf, const xmlChar * utfchar);
	XMLPUBFUN xmlChar * XMLCALL xmlUTF8Strsub(const xmlChar * utf, int start, int len);
	XMLPUBFUN int /*XMLCALL*/FASTCALL  xmlUTF8Strlen(const xmlChar * utf);
	XMLPUBFUN int XMLCALL xmlUTF8Size(const xmlChar * utf);
	XMLPUBFUN int XMLCALL xmlUTF8Charcmp(const xmlChar * utf1, const xmlChar * utf2);
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
		//typedef xmlRegexp * xmlRegexpPtr;
		// 
		// A libxml progressive regular expression evaluation context
		// 
		typedef struct _xmlRegExecCtxt xmlRegExecCtxt;
		typedef xmlRegExecCtxt *xmlRegExecCtxtPtr;
		// 
		// The POSIX like API
		// 
		XMLPUBFUN xmlRegexp * XMLCALL xmlRegexpCompile(const xmlChar *regexp);
		XMLPUBFUN void /*XMLCALL*/FASTCALL xmlRegFreeRegexp(xmlRegexp * regexp);
		XMLPUBFUN int  XMLCALL xmlRegexpExec(xmlRegexp * comp, const xmlChar *value);
		XMLPUBFUN void XMLCALL xmlRegexpPrint(FILE *output, xmlRegexp * regexp);
		XMLPUBFUN int  XMLCALL xmlRegexpIsDeterminist(xmlRegexp * comp);
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
		XMLPUBFUN xmlRegExecCtxtPtr XMLCALL xmlRegNewExecCtxt(xmlRegexp * comp, xmlRegExecCallbacks callback, void *data);
		XMLPUBFUN void XMLCALL xmlRegFreeExecCtxt(xmlRegExecCtxtPtr exec); 
		XMLPUBFUN int  XMLCALL xmlRegExecPushString(xmlRegExecCtxtPtr exec, const xmlChar *value, void *data);
		XMLPUBFUN int  XMLCALL xmlRegExecPushString2(xmlRegExecCtxtPtr exec, const xmlChar *value, const xmlChar *value2, void *data);
		XMLPUBFUN int  XMLCALL xmlRegExecNextValues(xmlRegExecCtxtPtr exec, int *nbval, int *nbneg, xmlChar **values, int *terminal);
		XMLPUBFUN int  XMLCALL xmlRegExecErrInfo(xmlRegExecCtxtPtr exec, const xmlChar **string, int *nbval, int *nbneg, xmlChar **values, int *terminal);
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
	#ifdef __cplusplus
	extern "C" {
	#endif
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
	#define IS_DIGIT_CH(c)  xmlIsDigit_ch(c)
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
	XMLPUBFUN int XMLCALL xmlIsLetter(int c);
	// 
	// Parser context.
	// 
	XMLPUBFUN xmlParserCtxt * XMLCALL xmlCreateFileParserCtxt(const char * filename);
	XMLPUBFUN xmlParserCtxt * XMLCALL xmlCreateURLParserCtxt(const char * filename, int options);
	XMLPUBFUN xmlParserCtxt * XMLCALL xmlCreateMemoryParserCtxt(const char * buffer, int size);
	XMLPUBFUN xmlParserCtxt * XMLCALL xmlCreateEntityParserCtxt(const xmlChar * URL, const xmlChar * ID, const xmlChar * base);
	XMLPUBFUN int XMLCALL xmlSwitchEncoding(xmlParserCtxt * ctxt, xmlCharEncoding enc);
	XMLPUBFUN int XMLCALL xmlSwitchToEncoding(xmlParserCtxt * ctxt, xmlCharEncodingHandler * handler);
	XMLPUBFUN int XMLCALL xmlSwitchInputEncoding(xmlParserCtxt * ctxt, xmlParserInput * input, xmlCharEncodingHandler * handler);
	#ifdef IN_LIBXML
		// internal error reporting 
		XMLPUBFUN void /*XMLCALL*/FASTCALL __xmlErrEncoding(xmlParserCtxt * ctxt, xmlParserErrors xmlerr, const char * msg, const xmlChar * str1, const xmlChar * str2);
	#endif
	// 
	// Input Streams.
	// 
	XMLPUBFUN xmlParserInput * XMLCALL xmlNewStringInputStream(xmlParserCtxt * ctxt, const xmlChar * buffer);
	XMLPUBFUN xmlParserInput * XMLCALL xmlNewEntityInputStream(xmlParserCtxt * ctxt, xmlEntity * entity);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlPushInput(xmlParserCtxt * ctxt, xmlParserInput * input);
	XMLPUBFUN xmlChar /*XMLCALL*/FASTCALL xmlPopInput(xmlParserCtxt * ctxt);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeInputStream(xmlParserInput * input);
	XMLPUBFUN xmlParserInput * XMLCALL xmlNewInputFromFile(xmlParserCtxt * ctxt, const char * filename);
	XMLPUBFUN xmlParserInput * XMLCALL xmlNewInputStream(xmlParserCtxt * ctxt);
	// 
	// Namespaces.
	// 
	XMLPUBFUN xmlChar * XMLCALL xmlSplitQName(xmlParserCtxt * ctxt, const xmlChar * name, xmlChar ** prefix);
	// 
	// Generic production rules.
	// 
	XMLPUBFUN const xmlChar * /*XMLCALL*/FASTCALL xmlParseName(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * XMLCALL xmlParseNmtoken(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * XMLCALL xmlParseEntityValue(xmlParserCtxt * ctxt, xmlChar ** orig);
	XMLPUBFUN xmlChar * /*XMLCALL*/FASTCALL xmlParseAttValue(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * XMLCALL xmlParseSystemLiteral(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * XMLCALL xmlParsePubidLiteral(xmlParserCtxt * ctxt);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlParseCharData(xmlParserCtxt * ctxt, int cdata);
	XMLPUBFUN xmlChar * XMLCALL xmlParseExternalID(xmlParserCtxt * ctxt, xmlChar ** publicID, int strict);
	XMLPUBFUN void XMLCALL xmlParseComment(xmlParserCtxt * ctxt);
	XMLPUBFUN const xmlChar * XMLCALL xmlParsePITarget(xmlParserCtxt * ctxt);
	XMLPUBFUN void XMLCALL xmlParsePI(xmlParserCtxt * ctxt);
	XMLPUBFUN void XMLCALL xmlParseNotationDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN void XMLCALL xmlParseEntityDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN int XMLCALL xmlParseDefaultDecl(xmlParserCtxt * ctxt, xmlChar ** value);
	XMLPUBFUN xmlEnumeration * XMLCALL xmlParseNotationType(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlEnumeration * XMLCALL xmlParseEnumerationType(xmlParserCtxt * ctxt);
	XMLPUBFUN int XMLCALL xmlParseEnumeratedType(xmlParserCtxt * ctxt, xmlEnumeration ** tree);
	XMLPUBFUN int XMLCALL xmlParseAttributeType(xmlParserCtxt * ctxt, xmlEnumeration ** tree);
	XMLPUBFUN void XMLCALL xmlParseAttributeListDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlElementContent * XMLCALL xmlParseElementMixedContentDecl(xmlParserCtxt * ctxt, int inputchk);
	XMLPUBFUN xmlElementContent * XMLCALL xmlParseElementChildrenContentDecl(xmlParserCtxt * ctxt, int inputchk);
	XMLPUBFUN int XMLCALL xmlParseElementContentDecl(xmlParserCtxt * ctxt, const xmlChar * name, xmlElementContent ** result);
	XMLPUBFUN int XMLCALL xmlParseElementDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN void XMLCALL xmlParseMarkupDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN int XMLCALL xmlParseCharRef(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlEntity * XMLCALL xmlParseEntityRef(xmlParserCtxt * ctxt);
	XMLPUBFUN void XMLCALL xmlParseReference(xmlParserCtxt * ctxt);
	XMLPUBFUN void XMLCALL xmlParsePEReference(xmlParserCtxt * ctxt);
	XMLPUBFUN void XMLCALL xmlParseDocTypeDecl(xmlParserCtxt * ctxt);
	#ifdef LIBXML_SAX1_ENABLED
		XMLPUBFUN const xmlChar * /*XMLCALL*/FASTCALL xmlParseAttribute(xmlParserCtxt * ctxt, xmlChar ** value);
		XMLPUBFUN const xmlChar * XMLCALL xmlParseStartTag(xmlParserCtxt * ctxt);
		XMLPUBFUN void XMLCALL xmlParseEndTag(xmlParserCtxt * ctxt);
	#endif /* LIBXML_SAX1_ENABLED */
	XMLPUBFUN void XMLCALL xmlParseCDSect(xmlParserCtxt * ctxt);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlParseContent(xmlParserCtxt * ctxt);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlParseElement(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * XMLCALL xmlParseVersionNum(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * XMLCALL xmlParseVersionInfo(xmlParserCtxt * ctxt);
	XMLPUBFUN xmlChar * XMLCALL xmlParseEncName(xmlParserCtxt * ctxt);
	XMLPUBFUN const xmlChar * XMLCALL xmlParseEncodingDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN int XMLCALL xmlParseSDDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN void XMLCALL xmlParseXMLDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN void XMLCALL xmlParseTextDecl(xmlParserCtxt * ctxt);
	XMLPUBFUN void XMLCALL xmlParseMisc(xmlParserCtxt * ctxt);
	XMLPUBFUN void XMLCALL xmlParseExternalSubset(xmlParserCtxt * ctxt, const xmlChar * ExternalID, const xmlChar * SystemID);

	#define XML_SUBSTITUTE_NONE     0 // If no entities need to be substituted.
	#define XML_SUBSTITUTE_REF      1 // Whether general entities need to be substituted.
	#define XML_SUBSTITUTE_PEREF    2 // Whether parameter entities need to be substituted.
	#define XML_SUBSTITUTE_BOTH     3 // Both general and parameter entities need to be substituted.

	XMLPUBFUN xmlChar * /*XMLCALL*/FASTCALL xmlStringDecodeEntities(xmlParserCtxt * ctxt, const xmlChar * str, int what, xmlChar end, xmlChar end2, xmlChar end3);
	XMLPUBFUN xmlChar * XMLCALL xmlStringLenDecodeEntities(xmlParserCtxt * ctxt, const xmlChar * str, int len, int what, xmlChar end, xmlChar end2, xmlChar end3);
	// 
	// Generated by MACROS on top of parser.c c.f. PUSH_AND_POP.
	// 
	XMLPUBFUN int XMLCALL nodePush(xmlParserCtxt * ctxt, xmlNode * value);
	XMLPUBFUN xmlNode * XMLCALL nodePop(xmlParserCtxt * ctxt);
	XMLPUBFUN int /*XMLCALL*/FASTCALL inputPush(xmlParserCtxt * ctxt, xmlParserInput * value);
	XMLPUBFUN xmlParserInput * XMLCALL inputPop(xmlParserCtxt * ctxt);
	XMLPUBFUN const xmlChar * XMLCALL namePop(xmlParserCtxt * ctxt);
	XMLPUBFUN int XMLCALL namePush(xmlParserCtxt * ctxt, const xmlChar * value);
	// 
	// other commodities shared between parser.c and parserInternals.
	// 
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlSkipBlankChars(xmlParserCtxt * ctxt);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlStringCurrentChar(xmlParserCtxt * ctxt, const xmlChar * cur, int * len);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlParserHandlePEReference(xmlParserCtxt * ctxt);
	XMLPUBFUN int XMLCALL xmlCheckLanguageID(const xmlChar * lang);
	// 
	// Really core function shared with HTML parser.
	// 
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlCurrentChar(xmlParserCtxt * ctxt, int * len);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlCopyCharMultiByte(xmlChar * out, int val);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlCopyChar(int len, xmlChar * out, int val);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlNextChar(xmlParserCtxt * ctxt);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlParserInputShrink(xmlParserInput * in);
	#ifdef LIBXML_HTML_ENABLED
		// 
		// Actually comes from the HTML parser but launched from the init stuff.
		// 
		XMLPUBFUN void XMLCALL htmlInitAutoClose();
		XMLPUBFUN htmlParserCtxt * XMLCALL htmlCreateFileParserCtxt(const char * filename, const char * encoding);
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
		XMLPUBFUN void XMLCALL xmlSetEntityReferenceFunc(xmlEntityReferenceFunc func);
		XMLPUBFUN xmlChar * XMLCALL xmlParseQuotedString(xmlParserCtxt * ctxt);
		XMLPUBFUN void XMLCALL xmlParseNamespace(xmlParserCtxt * ctxt);
		XMLPUBFUN xmlChar * XMLCALL xmlNamespaceParseNSDef(xmlParserCtxt * ctxt);
		XMLPUBFUN xmlChar * XMLCALL xmlScanName(xmlParserCtxt * ctxt);
		XMLPUBFUN xmlChar * XMLCALL xmlNamespaceParseNCName(xmlParserCtxt * ctxt);
		XMLPUBFUN void XMLCALL xmlParserHandleReference(xmlParserCtxt * ctxt);
		XMLPUBFUN xmlChar * XMLCALL xmlNamespaceParseQName(xmlParserCtxt * ctxt, xmlChar ** prefix);
		// 
		// Entities
		// 
		XMLPUBFUN xmlChar * XMLCALL xmlDecodeEntities(xmlParserCtxt * ctxt, int len, int what, xmlChar end, xmlChar end2, xmlChar end3);
		XMLPUBFUN void XMLCALL xmlHandleEntity(xmlParserCtxt * ctxt, xmlEntity * entity);
	#endif /* LIBXML_LEGACY_ENABLED */
	#ifdef IN_LIBXML
		// 
		// internal only
		// 
		XMLPUBFUN void /*XMLCALL*/FASTCALL xmlErrMemory(xmlParserCtxt * ctxt, const char * extra);
	#endif
	//
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
		XMLPUBFUN xlinkType XMLCALL xlinkIsLink(xmlDoc * doc, xmlNode * pNode);
	#endif

	#ifdef __cplusplus
	}
	#endif
	//
	//#include <libxml/SAX.h>
	#ifdef LIBXML_LEGACY_ENABLED
		#ifdef __cplusplus
		extern "C" {
		#endif
			XMLPUBFUN const xmlChar * XMLCALL getPublicId(void *ctx);
			XMLPUBFUN const xmlChar * XMLCALL getSystemId(void *ctx);
			XMLPUBFUN void XMLCALL setDocumentLocator(void *ctx, xmlSAXLocator * loc);
			XMLPUBFUN int XMLCALL getLineNumber(void *ctx);
			XMLPUBFUN int XMLCALL getColumnNumber(void *ctx);
			XMLPUBFUN int XMLCALL isStandalone(void *ctx);
			XMLPUBFUN int XMLCALL hasInternalSubset(void *ctx);
			XMLPUBFUN int XMLCALL hasExternalSubset(void *ctx);
			XMLPUBFUN void XMLCALL internalSubset(void *ctx, const xmlChar *name, const xmlChar *ExternalID, const xmlChar *SystemID);
			XMLPUBFUN void XMLCALL externalSubset(void *ctx, const xmlChar *name, const xmlChar *ExternalID, const xmlChar *SystemID);
			XMLPUBFUN xmlEntity * XMLCALL getEntity(void *ctx, const xmlChar *name);
			XMLPUBFUN xmlEntity * XMLCALL getParameterEntity(void *ctx, const xmlChar *name);
			XMLPUBFUN xmlParserInput * XMLCALL resolveEntity(void *ctx, const xmlChar *publicId, const xmlChar *systemId);
			XMLPUBFUN void XMLCALL entityDecl(void *ctx, const xmlChar *name, int type, const xmlChar *publicId, const xmlChar *systemId, xmlChar *content);
			XMLPUBFUN void XMLCALL attributeDecl(void *ctx, const xmlChar *elem, const xmlChar *fullname, int type, int def, const xmlChar *defaultValue, xmlEnumeration * tree);
			XMLPUBFUN void XMLCALL elementDecl(void *ctx, const xmlChar *name, int type, xmlElementContent * content);
			XMLPUBFUN void XMLCALL notationDecl(void *ctx, const xmlChar *name, const xmlChar *publicId, const xmlChar *systemId);
			XMLPUBFUN void XMLCALL unparsedEntityDecl(void *ctx, const xmlChar *name, const xmlChar *publicId, const xmlChar *systemId, const xmlChar *notationName);
			XMLPUBFUN void XMLCALL startDocument(void *ctx);
			XMLPUBFUN void XMLCALL endDocument(void *ctx);
			XMLPUBFUN void XMLCALL attribute(void *ctx, const xmlChar *fullname, const xmlChar *value);
			XMLPUBFUN void XMLCALL startElement(void *ctx, const xmlChar *fullname, const xmlChar **atts);
			XMLPUBFUN void XMLCALL endElement(void *ctx, const xmlChar *name);
			XMLPUBFUN void XMLCALL reference(void *ctx, const xmlChar *name);
			XMLPUBFUN void XMLCALL characters(void *ctx, const xmlChar *ch, int len);
			XMLPUBFUN void XMLCALL ignorableWhitespace(void *ctx, const xmlChar *ch, int len);
			XMLPUBFUN void XMLCALL processingInstruction(void *ctx, const xmlChar *target, const xmlChar *data);
			XMLPUBFUN void XMLCALL globalNamespace(void *ctx, const xmlChar *href, const xmlChar *prefix);
			XMLPUBFUN void XMLCALL setNamespace(void *ctx, const xmlChar *name);
			XMLPUBFUN xmlNs * XMLCALL getNamespace(void *ctx);
			XMLPUBFUN int XMLCALL checkNamespace(void *ctx, xmlChar *nameSpace);
			XMLPUBFUN void XMLCALL namespaceDecl(void *ctx, const xmlChar *href, const xmlChar *prefix);
			XMLPUBFUN void XMLCALL comment(void *ctx, const xmlChar *value);
			XMLPUBFUN void XMLCALL cdataBlock(void *ctx, const xmlChar *value, int len);
			#ifdef LIBXML_SAX1_ENABLED
				XMLPUBFUN void XMLCALL initxmlDefaultSAXHandler(xmlSAXHandlerV1 *hdlr, int warning);
				#ifdef LIBXML_HTML_ENABLED 
					XMLPUBFUN void XMLCALL inithtmlDefaultSAXHandler(xmlSAXHandlerV1 *hdlr);
				#endif
				#ifdef LIBXML_DOCB_ENABLED
					XMLPUBFUN void XMLCALL initdocbDefaultSAXHandler(xmlSAXHandlerV1 *hdlr);
				#endif
			#endif /* LIBXML_SAX1_ENABLED */
		#ifdef __cplusplus
		}
		#endif
	#endif /* LIBXML_LEGACY_ENABLED */
	//#include <libxml/SAX2.h>
	#ifdef __cplusplus
	extern "C" {
	#endif
		XMLPUBFUN const xmlChar * XMLCALL xmlSAX2GetPublicId(void * ctx);
		XMLPUBFUN const xmlChar * XMLCALL xmlSAX2GetSystemId(void * ctx);
		XMLPUBFUN void XMLCALL xmlSAX2SetDocumentLocator(void * ctx, xmlSAXLocator * loc);
		XMLPUBFUN int XMLCALL xmlSAX2GetLineNumber(void * ctx);
		XMLPUBFUN int XMLCALL xmlSAX2GetColumnNumber(void * ctx);
		XMLPUBFUN int XMLCALL xmlSAX2IsStandalone(void * ctx);
		XMLPUBFUN int XMLCALL xmlSAX2HasInternalSubset(void * ctx);
		XMLPUBFUN int XMLCALL xmlSAX2HasExternalSubset(void * ctx);
		XMLPUBFUN void XMLCALL xmlSAX2InternalSubset(void * ctx, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID);
		XMLPUBFUN void XMLCALL xmlSAX2ExternalSubset(void * ctx, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID);
		XMLPUBFUN xmlEntity * XMLCALL xmlSAX2GetEntity(void * ctx, const xmlChar * name);
		XMLPUBFUN xmlEntity * XMLCALL xmlSAX2GetParameterEntity(void * ctx, const xmlChar * name);
		XMLPUBFUN xmlParserInput * XMLCALL xmlSAX2ResolveEntity(void * ctx, const xmlChar * publicId, const xmlChar * systemId);
		XMLPUBFUN void XMLCALL xmlSAX2EntityDecl(void * ctx, const xmlChar * name, int type, const xmlChar * publicId, const xmlChar * systemId, xmlChar * content);
		XMLPUBFUN void XMLCALL xmlSAX2AttributeDecl(void * ctx, const xmlChar * elem, const xmlChar * fullname, int type, int def, const xmlChar * defaultValue, xmlEnumeration * tree);
		XMLPUBFUN void XMLCALL xmlSAX2ElementDecl(void * ctx, const xmlChar * name, int type, xmlElementContent * content);
		XMLPUBFUN void XMLCALL xmlSAX2NotationDecl(void * ctx, const xmlChar * name, const xmlChar * publicId, const xmlChar * systemId);
		XMLPUBFUN void XMLCALL xmlSAX2UnparsedEntityDecl(void * ctx, const xmlChar * name, const xmlChar * publicId, const xmlChar * systemId, const xmlChar * notationName);
		XMLPUBFUN void XMLCALL xmlSAX2StartDocument(void * ctx);
		XMLPUBFUN void XMLCALL xmlSAX2EndDocument(void * ctx);
		#if defined(LIBXML_SAX1_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_WRITER_ENABLED) || defined(LIBXML_DOCB_ENABLED) || defined(LIBXML_LEGACY_ENABLED)
			XMLPUBFUN void XMLCALL xmlSAX2StartElement(void * ctx, const xmlChar * fullname, const xmlChar ** atts);
			XMLPUBFUN void XMLCALL xmlSAX2EndElement(void * ctx, const xmlChar * name);
		#endif
		XMLPUBFUN void XMLCALL xmlSAX2StartElementNs(void * ctx, const xmlChar * localname, const xmlChar * prefix,
			const xmlChar * URI, int nb_namespaces, const xmlChar ** namespaces, int nb_attributes, int nb_defaulted, const xmlChar ** attributes);
		XMLPUBFUN void XMLCALL xmlSAX2EndElementNs(void * ctx, const xmlChar * localname, const xmlChar * prefix, const xmlChar * URI);
		XMLPUBFUN void XMLCALL xmlSAX2Reference(void * ctx, const xmlChar * name);
		XMLPUBFUN void XMLCALL xmlSAX2Characters(void * ctx, const xmlChar * ch, int len);
		XMLPUBFUN void XMLCALL xmlSAX2IgnorableWhitespace(void * ctx, const xmlChar * ch, int len);
		XMLPUBFUN void XMLCALL xmlSAX2ProcessingInstruction(void * ctx, const xmlChar * target, const xmlChar * data);
		XMLPUBFUN void XMLCALL xmlSAX2Comment(void * ctx, const xmlChar * value);
		XMLPUBFUN void XMLCALL xmlSAX2CDataBlock(void * ctx, const xmlChar * value, int len);
		#ifdef LIBXML_SAX1_ENABLED
			XMLPUBFUN int XMLCALL xmlSAXDefaultVersion(int version);
		#endif
		XMLPUBFUN int XMLCALL xmlSAXVersion(xmlSAXHandler * hdlr, int version);
		XMLPUBFUN void XMLCALL xmlSAX2InitDefaultSAXHandler(xmlSAXHandler * hdlr, int warning);
		#ifdef LIBXML_HTML_ENABLED
			XMLPUBFUN void XMLCALL xmlSAX2InitHtmlDefaultSAXHandler(xmlSAXHandler * hdlr);
			XMLPUBFUN void XMLCALL htmlDefaultSAXHandlerInit();
		#endif
		#ifdef LIBXML_DOCB_ENABLED
			XMLPUBFUN void XMLCALL xmlSAX2InitDocbDefaultSAXHandler(xmlSAXHandler * hdlr);
			XMLPUBFUN void XMLCALL docbDefaultSAXHandlerInit();
		#endif
		XMLPUBFUN void XMLCALL xmlDefaultSAXHandlerInit();
	#ifdef __cplusplus
	}
	#endif
#endif // } IN_LIBXML
	//#include <libxml/xpath.h>
	#if defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
		#ifdef __cplusplus
		extern "C" {
		#endif
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

		XMLPUBFUN void /*XMLCALL*/FASTCALL xmlXPathFreeObject(xmlXPathObject * obj);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathNodeSetCreate(xmlNode * val);
		XMLPUBFUN void XMLCALL xmlXPathFreeNodeSetList(xmlXPathObject * obj);
		XMLPUBFUN void XMLCALL xmlXPathFreeNodeSet(xmlNodeSet * obj);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathObjectCopy(xmlXPathObject * val);
		XMLPUBFUN int XMLCALL xmlXPathCmpNodes(xmlNode * node1, xmlNode * node2);
		// 
		// Conversion functions to basic types.
		// 
		XMLPUBFUN int XMLCALL xmlXPathCastNumberToBoolean(double val);
		XMLPUBFUN int XMLCALL xmlXPathCastStringToBoolean(const xmlChar * val);
		XMLPUBFUN int XMLCALL xmlXPathCastNodeSetToBoolean(const xmlNodeSet * ns);
		XMLPUBFUN int XMLCALL xmlXPathCastToBoolean	(xmlXPathObject * val);
		XMLPUBFUN double XMLCALL xmlXPathCastBooleanToNumber(int val);
		XMLPUBFUN double XMLCALL xmlXPathCastStringToNumber(const xmlChar * val);
		XMLPUBFUN double XMLCALL xmlXPathCastNodeToNumber(xmlNode * pNode);
		XMLPUBFUN double XMLCALL xmlXPathCastNodeSetToNumber(xmlNodeSet * ns);
		XMLPUBFUN double XMLCALL xmlXPathCastToNumber(xmlXPathObject * val);
		XMLPUBFUN xmlChar * XMLCALL xmlXPathCastBooleanToString(int val);
		XMLPUBFUN xmlChar * XMLCALL xmlXPathCastNumberToString(double val);
		XMLPUBFUN xmlChar * XMLCALL xmlXPathCastNodeToString(xmlNode * pNode);
		XMLPUBFUN xmlChar * XMLCALL xmlXPathCastNodeSetToString(xmlNodeSet * ns);
		XMLPUBFUN xmlChar * XMLCALL xmlXPathCastToString(xmlXPathObject * val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathConvertBoolean(xmlXPathObject * val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathConvertNumber(xmlXPathObject * val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathConvertString(xmlXPathObject * val);
		// 
		// Context handling.
		// 
		XMLPUBFUN xmlXPathContext * XMLCALL xmlXPathNewContext(xmlDoc * doc);
		XMLPUBFUN void XMLCALL xmlXPathFreeContext(xmlXPathContext * ctxt);
		XMLPUBFUN int XMLCALL xmlXPathContextSetCache(xmlXPathContext * ctxt, int active, int value, int options);
		// 
		// Evaluation functions.
		// 
		XMLPUBFUN long XMLCALL xmlXPathOrderDocElems(xmlDoc * doc);
		XMLPUBFUN int XMLCALL xmlXPathSetContextNode(xmlNode * pNode, xmlXPathContext * ctx);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathNodeEval(xmlNode * pNode, const xmlChar * str, xmlXPathContext * ctx);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathEval(const xmlChar *str, xmlXPathContext * ctx);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathEvalExpression(const xmlChar *str, xmlXPathContext * ctxt);
		XMLPUBFUN int XMLCALL xmlXPathEvalPredicate(xmlXPathContext * ctxt, xmlXPathObject * res);
		/**
		 * Separate compilation/evaluation entry points.
		 */
		XMLPUBFUN xmlXPathCompExprPtr XMLCALL xmlXPathCompile(const xmlChar *str);
		XMLPUBFUN xmlXPathCompExprPtr XMLCALL xmlXPathCtxtCompile(xmlXPathContext * ctxt, const xmlChar *str);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathCompiledEval(xmlXPathCompExprPtr comp, xmlXPathContext * ctx);
		XMLPUBFUN int XMLCALL xmlXPathCompiledEvalToBoolean(xmlXPathCompExprPtr comp, xmlXPathContext * ctxt);
		XMLPUBFUN void XMLCALL xmlXPathFreeCompExpr(xmlXPathCompExprPtr comp);
	#endif /* LIBXML_XPATH_ENABLED */
	#if defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
		XMLPUBFUN void XMLCALL xmlXPathInit();
		// @v10.9.11 XMLPUBFUN int XMLCALL xmlXPathIsNaN_Removed(double val);
		XMLPUBFUN int /*XMLCALL*/FASTCALL xmlXPathIsInf(double val);
		#ifdef __cplusplus
		}
		#endif
	#endif
	//#include <libxml/xpointer.h>
	#ifdef LIBXML_XPTR_ENABLED
		#ifdef __cplusplus
		extern "C" {
		#endif
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
		XMLPUBFUN xmlLocationSet * /*XMLCALL*/FASTCALL xmlXPtrLocationSetCreate(xmlXPathObject * val);
		XMLPUBFUN void XMLCALL xmlXPtrFreeLocationSet(xmlLocationSet * obj);
		XMLPUBFUN xmlLocationSet * XMLCALL xmlXPtrLocationSetMerge(xmlLocationSet * val1, xmlLocationSet * val2);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPtrNewRange(xmlNode * start, int startindex, xmlNode * end, int endindex);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPtrNewRangePoints(xmlXPathObject * start, xmlXPathObject * end);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPtrNewRangeNodePoint(xmlNode * start, xmlXPathObject * end);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPtrNewRangePointNode(xmlXPathObject * start, xmlNode * end);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPtrNewRangeNodes(xmlNode * start, xmlNode * end);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPtrNewLocationSetNodes(xmlNode * start, xmlNode * end);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPtrNewLocationSetNodeSet(xmlNodeSet * set);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPtrNewRangeNodeObject(xmlNode * start, xmlXPathObject * end);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPtrNewCollapsedRange(xmlNode * start);
		XMLPUBFUN void /*XMLCALL*/FASTCALL xmlXPtrLocationSetAdd(xmlLocationSet * cur, xmlXPathObject * val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPtrWrapLocationSet(xmlLocationSet * val);
		XMLPUBFUN void XMLCALL xmlXPtrLocationSetDel(xmlLocationSet * cur, xmlXPathObject * val);
		XMLPUBFUN void XMLCALL xmlXPtrLocationSetRemove(xmlLocationSet * cur, int val);
		XMLPUBFUN xmlXPathContext * XMLCALL xmlXPtrNewContext(xmlDoc * doc, xmlNode * here, xmlNode * origin);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPtrEval(const xmlChar * str, xmlXPathContext * ctx);
		XMLPUBFUN void XMLCALL xmlXPtrRangeToFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN xmlNode * XMLCALL xmlXPtrBuildNodeList(xmlXPathObject * obj);
		XMLPUBFUN void XMLCALL xmlXPtrEvalRangePredicate(xmlXPathParserContext * ctxt);
		#ifdef __cplusplus
		}
		#endif
	#endif

	//#include <libxml/xpathInternals.h>
	#ifdef LIBXML_XPATH_ENABLED
		#ifdef __cplusplus
		extern "C" {
		#endif
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

		XMLPUBFUN int XMLCALL xmlXPathPopBoolean(xmlXPathParserContext * ctxt);
		XMLPUBFUN double XMLCALL xmlXPathPopNumber(xmlXPathParserContext * ctxt);
		XMLPUBFUN xmlChar * XMLCALL xmlXPathPopString(xmlXPathParserContext * ctxt);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathPopNodeSet(xmlXPathParserContext * ctxt);
		XMLPUBFUN void * XMLCALL xmlXPathPopExternal(xmlXPathParserContext * ctxt);
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
		XMLPUBFUN void XMLCALL xmlXPathRegisterVariableLookup(xmlXPathContext * ctxt, xmlXPathVariableLookupFunc f, void * data);
		/*
		 * Function Lookup forwarding.
		 */
		XMLPUBFUN void XMLCALL xmlXPathRegisterFuncLookup(xmlXPathContext * ctxt, xmlXPathFuncLookupFunc f, void * funcCtxt);
		// 
		// Error reporting.
		// 
		XMLPUBFUN void XMLCALL xmlXPatherror(xmlXPathParserContext * ctxt, /*const char * file, int line,*/int no);
		XMLPUBFUN void /*XMLCALL*/FASTCALL xmlXPathErr(xmlXPathParserContext * ctxt, int error);
		#ifdef LIBXML_DEBUG_ENABLED
			XMLPUBFUN void XMLCALL xmlXPathDebugDumpObject(FILE * output, xmlXPathObject * cur, int depth);
			XMLPUBFUN void XMLCALL xmlXPathDebugDumpCompExpr(FILE * output, xmlXPathCompExprPtr comp, int depth);
		#endif
		// 
		// NodeSet handling.
		// 
		XMLPUBFUN int XMLCALL xmlXPathNodeSetContains(const xmlNodeSet * cur, const xmlNode * val);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathDifference(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathIntersection(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathDistinctSorted(xmlNodeSet * nodes);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathDistinct(xmlNodeSet * nodes);
		XMLPUBFUN int XMLCALL xmlXPathHasSameNodes(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathNodeLeadingSorted(xmlNodeSet * nodes, xmlNode * pNode);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathLeadingSorted(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathNodeLeading(xmlNodeSet * nodes, xmlNode * pNode);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathLeading(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathNodeTrailingSorted(xmlNodeSet * nodes, xmlNode * pNode);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathTrailingSorted(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathNodeTrailing(xmlNodeSet * nodes, xmlNode * pNode);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathTrailing(xmlNodeSet * nodes1, xmlNodeSet * nodes2);
		// 
		// Extending a context.
		// 
		XMLPUBFUN int XMLCALL xmlXPathRegisterNs(xmlXPathContext * ctxt, const xmlChar * prefix, const xmlChar * ns_uri);
		XMLPUBFUN const xmlChar * XMLCALL xmlXPathNsLookup(xmlXPathContext * ctxt, const xmlChar * prefix);
		XMLPUBFUN void XMLCALL xmlXPathRegisteredNsCleanup(xmlXPathContext * ctxt);
		XMLPUBFUN int /*XMLCALL*/FASTCALL xmlXPathRegisterFunc(xmlXPathContext * ctxt, const xmlChar * name, xmlXPathFunction f);
		XMLPUBFUN int XMLCALL xmlXPathRegisterFuncNS(xmlXPathContext * ctxt, const xmlChar * name, const xmlChar * ns_uri, xmlXPathFunction f);
		XMLPUBFUN int XMLCALL xmlXPathRegisterVariable(xmlXPathContext * ctxt, const xmlChar * name, xmlXPathObject * value);
		XMLPUBFUN int XMLCALL xmlXPathRegisterVariableNS(xmlXPathContext * ctxt, const xmlChar * name, const xmlChar * ns_uri, xmlXPathObject * value);
		XMLPUBFUN xmlXPathFunction XMLCALL xmlXPathFunctionLookup(xmlXPathContext * ctxt, const xmlChar * name);
		XMLPUBFUN xmlXPathFunction XMLCALL xmlXPathFunctionLookupNS(xmlXPathContext * ctxt, const xmlChar * name, const xmlChar * ns_uri);
		XMLPUBFUN void XMLCALL xmlXPathRegisteredFuncsCleanup(xmlXPathContext * ctxt);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathVariableLookup(xmlXPathContext * ctxt, const xmlChar * name);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathVariableLookupNS(xmlXPathContext * ctxt, const xmlChar * name, const xmlChar * ns_uri);
		XMLPUBFUN void XMLCALL xmlXPathRegisteredVariablesCleanup(xmlXPathContext * ctxt);
		// 
		// Utilities to extend XPath.
		// 
		XMLPUBFUN xmlXPathParserContext * XMLCALL xmlXPathNewParserContext(const xmlChar * str, xmlXPathContext * ctxt);
		XMLPUBFUN void XMLCALL xmlXPathFreeParserContext(xmlXPathParserContext * ctxt);
		/* @todo remap to xmlXPathValuePop and Push. */
		XMLPUBFUN xmlXPathObject * /*XMLCALL*/FASTCALL valuePop(xmlXPathParserContext * pCtxt);
		XMLPUBFUN int /*XMLCALL*/FASTCALL valuePush(xmlXPathParserContext * pCtxt, xmlXPathObject * pValue);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathNewString(const xmlChar * val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathNewCString(const char * val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathWrapString(xmlChar * val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathWrapCString(char * val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathNewFloat(double val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathNewBoolean(int val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathNewNodeSet(xmlNode * val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathNewValueTree(xmlNode * val);
		XMLPUBFUN int XMLCALL xmlXPathNodeSetAdd(xmlNodeSet * cur, xmlNode * val);
		XMLPUBFUN int XMLCALL xmlXPathNodeSetAddUnique(xmlNodeSet * cur, xmlNode * val);
		XMLPUBFUN int XMLCALL xmlXPathNodeSetAddNs(xmlNodeSet * cur, xmlNode * pNode, xmlNs * ns);
		XMLPUBFUN void XMLCALL xmlXPathNodeSetSort(xmlNodeSet * set);
		XMLPUBFUN void XMLCALL xmlXPathRoot(xmlXPathParserContext * ctxt);
		XMLPUBFUN void XMLCALL xmlXPathEvalExpr(xmlXPathParserContext * ctxt);
		XMLPUBFUN xmlChar * XMLCALL xmlXPathParseName(xmlXPathParserContext * ctxt);
		XMLPUBFUN xmlChar * XMLCALL xmlXPathParseNCName(xmlXPathParserContext * ctxt);
		// 
		// Existing functions.
		// 
		XMLPUBFUN double XMLCALL xmlXPathStringEvalNumber(const xmlChar * str);
		XMLPUBFUN int    XMLCALL xmlXPathEvaluatePredicateResult(xmlXPathParserContext * ctxt, xmlXPathObject * res);
		XMLPUBFUN void   XMLCALL xmlXPathRegisterAllFunctions(xmlXPathContext * ctxt);
		XMLPUBFUN xmlNodeSet * XMLCALL xmlXPathNodeSetMerge(xmlNodeSet * val1, xmlNodeSet * val2);
		XMLPUBFUN void   XMLCALL xmlXPathNodeSetDel(xmlNodeSet * cur, xmlNode * val);
		XMLPUBFUN void   XMLCALL xmlXPathNodeSetRemove(xmlNodeSet * cur, int val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathNewNodeSetList(xmlNodeSet * val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathWrapNodeSet(xmlNodeSet * val);
		XMLPUBFUN xmlXPathObject * XMLCALL xmlXPathWrapExternal(void * val);
		XMLPUBFUN int    XMLCALL xmlXPathEqualValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN int    XMLCALL xmlXPathNotEqualValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN int    XMLCALL xmlXPathCompareValues(xmlXPathParserContext * ctxt, int inf, int strict);
		XMLPUBFUN void   XMLCALL xmlXPathValueFlipSign(xmlXPathParserContext * ctxt);
		XMLPUBFUN void   XMLCALL xmlXPathAddValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN void   XMLCALL xmlXPathSubValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN void   XMLCALL xmlXPathMultValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN void   XMLCALL xmlXPathDivValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN void   XMLCALL xmlXPathModValues(xmlXPathParserContext * ctxt);
		XMLPUBFUN int    XMLCALL xmlXPathIsNodeType(const xmlChar * name);
		// 
		// Some of the axis navigation routines.
		// 
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextSelf(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextChild(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextDescendant(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextDescendantOrSelf(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextParent(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextAncestorOrSelf(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextFollowingSibling(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextFollowing(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextNamespace(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextAttribute(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextPreceding(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextAncestor(xmlXPathParserContext * ctxt, xmlNode * cur);
		XMLPUBFUN xmlNode * XMLCALL xmlXPathNextPrecedingSibling(xmlXPathParserContext * ctxt, xmlNode * cur);
		// 
		// The official core of XPath functions.
		// 
		XMLPUBFUN void XMLCALL xmlXPathLastFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathPositionFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathCountFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathIdFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathLocalNameFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathNamespaceURIFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathStringFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathStringLengthFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathConcatFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathContainsFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathStartsWithFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathSubstringFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathSubstringBeforeFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathSubstringAfterFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathNormalizeFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathTranslateFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathNotFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathTrueFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathFalseFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathLangFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathNumberFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathSumFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathFloorFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathCeilingFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathRoundFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void XMLCALL xmlXPathBooleanFunction(xmlXPathParserContext * ctxt, int nargs);
		XMLPUBFUN void /*XMLCALL*/FASTCALL xmlXPathNodeSetFreeNs(xmlNs * ns); // Really internal functions

		#ifdef __cplusplus
		}
		#endif
	#endif
#ifdef IN_LIBXML
	//#include <libxml/c14n.h>
	#ifdef LIBXML_C14N_ENABLED
		#ifdef LIBXML_OUTPUT_ENABLED
			#ifdef __cplusplus
			extern "C" {
			#endif /* __cplusplus */
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

			XMLPUBFUN int XMLCALL xmlC14NDocSaveTo(xmlDoc * doc, xmlNodeSet * nodes, int mode/* a xmlC14NMode */, xmlChar ** inclusive_ns_prefixes, int with_comments, xmlOutputBuffer * buf);
			XMLPUBFUN int XMLCALL xmlC14NDocDumpMemory(xmlDoc * doc, xmlNodeSet * nodes, int mode/* a xmlC14NMode */, xmlChar ** inclusive_ns_prefixes, int with_comments, xmlChar ** doc_txt_ptr);
			XMLPUBFUN int XMLCALL xmlC14NDocSave(xmlDoc * doc, xmlNodeSet * nodes, int mode/* a xmlC14NMode */, xmlChar ** inclusive_ns_prefixes, int with_comments, const char * filename, int compression);
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

			XMLPUBFUN int XMLCALL xmlC14NExecute(xmlDoc * doc, xmlC14NIsVisibleCallback is_visible_callback, void * user_data, 
				int mode/* a xmlC14NMode */, xmlChar ** inclusive_ns_prefixes, int with_comments, xmlOutputBuffer * buf);

			#ifdef __cplusplus
			}
			#endif /* __cplusplus */
		#endif /* LIBXML_OUTPUT_ENABLED */
	#endif
	//#include <libxml/pattern.h>
	#ifdef LIBXML_PATTERN_ENABLED
		#ifdef __cplusplus
		extern "C" {
		#endif
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

		XMLPUBFUN void XMLCALL xmlFreePattern(xmlPattern * comp);
		XMLPUBFUN void XMLCALL xmlFreePatternList(xmlPattern * comp);
		XMLPUBFUN xmlPattern * XMLCALL xmlPatterncompile(const xmlChar *pattern, xmlDict *dict, int flags, const xmlChar **namespaces);
		XMLPUBFUN int XMLCALL xmlPatternMatch(xmlPattern * comp, xmlNode * pNode);

		/* streaming interfaces */
		typedef struct _xmlStreamCtxt xmlStreamCtxt;
		typedef xmlStreamCtxt * xmlStreamCtxtPtr;

		XMLPUBFUN int XMLCALL xmlPatternStreamable(xmlPattern * comp);
		XMLPUBFUN int XMLCALL xmlPatternMaxDepth(xmlPattern * comp);
		XMLPUBFUN int XMLCALL xmlPatternMinDepth(xmlPattern * comp);
		XMLPUBFUN int XMLCALL xmlPatternFromRoot(xmlPattern * comp);
		XMLPUBFUN xmlStreamCtxtPtr XMLCALL xmlPatternGetStreamCtxt(xmlPattern * comp);
		XMLPUBFUN void XMLCALL xmlFreeStreamCtxt(xmlStreamCtxtPtr stream);
		XMLPUBFUN int XMLCALL xmlStreamPushNode(xmlStreamCtxtPtr stream, const xmlChar *name, const xmlChar *ns, int nodeType);
		XMLPUBFUN int XMLCALL xmlStreamPush(xmlStreamCtxtPtr stream, const xmlChar *name, const xmlChar *ns);
		XMLPUBFUN int XMLCALL xmlStreamPushAttr(xmlStreamCtxtPtr stream, const xmlChar *name, const xmlChar *ns);
		XMLPUBFUN int XMLCALL xmlStreamPop(xmlStreamCtxtPtr stream);
		XMLPUBFUN int XMLCALL xmlStreamWantsAnyNode	(xmlStreamCtxtPtr stream);
		#ifdef __cplusplus
		}
		#endif
	#endif /* LIBXML_PATTERN_ENABLED */
	//#include <libxml/schematron.h>
	#ifdef LIBXML_SCHEMATRON_ENABLED
		#ifdef __cplusplus
		extern "C" {
		#endif

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
		XMLPUBFUN xmlSchematronParserCtxt * XMLCALL xmlSchematronNewParserCtxt(const char *URL);
		XMLPUBFUN xmlSchematronParserCtxt * XMLCALL xmlSchematronNewMemParserCtxt(const char *buffer, int size);
		XMLPUBFUN xmlSchematronParserCtxt * XMLCALL xmlSchematronNewDocParserCtxt(xmlDoc * doc);
		XMLPUBFUN void XMLCALL xmlSchematronFreeParserCtxt(xmlSchematronParserCtxt * ctxt);
		// XMLPUBFUN void XMLCALL xmlSchematronSetParserErrors(xmlSchematronParserCtxt * ctxt, xmlSchematronValidityErrorFunc err, xmlSchematronValidityWarningFunc warn, void *ctx);
		// XMLPUBFUN int XMLCALL xmlSchematronGetParserErrors(xmlSchematronParserCtxt * ctxt, xmlSchematronValidityErrorFunc * err, xmlSchematronValidityWarningFunc * warn, void **ctx);
		// XMLPUBFUN int XMLCALL xmlSchematronIsValid(xmlSchematronValidCtxt * ctxt);
		XMLPUBFUN xmlSchematron * XMLCALL xmlSchematronParse(xmlSchematronParserCtxt * ctxt);
		XMLPUBFUN void XMLCALL xmlSchematronFree(xmlSchematron * schema);
		/*
		 * Interfaces for validating
		 */
		XMLPUBFUN void XMLCALL xmlSchematronSetValidStructuredErrors(xmlSchematronValidCtxt * ctxt, xmlStructuredErrorFunc serror, void *ctx);
		// XMLPUBFUN void XMLCALL xmlSchematronSetValidErrors(xmlSchematronValidCtxt * ctxt, xmlSchematronValidityErrorFunc err, xmlSchematronValidityWarningFunc warn, void *ctx);
		// XMLPUBFUN int XMLCALL xmlSchematronGetValidErrors(xmlSchematronValidCtxt * ctxt, xmlSchematronValidityErrorFunc *err, xmlSchematronValidityWarningFunc *warn, void **ctx);
		// XMLPUBFUN int XMLCALL xmlSchematronSetValidOptions(xmlSchematronValidCtxt * ctxt, int options);
		// XMLPUBFUN int XMLCALL xmlSchematronValidCtxtGetOptions(xmlSchematronValidCtxt * ctxt);
		// XMLPUBFUN int XMLCALL xmlSchematronValidateOneElement(xmlSchematronValidCtxt * ctxt, xmlNode * elem);
		XMLPUBFUN xmlSchematronValidCtxt * XMLCALL xmlSchematronNewValidCtxt(xmlSchematron * schema, int options);
		XMLPUBFUN void XMLCALL xmlSchematronFreeValidCtxt(xmlSchematronValidCtxt * ctxt);
		XMLPUBFUN int XMLCALL xmlSchematronValidateDoc(xmlSchematronValidCtxt * ctxt, xmlDoc * instance);

		#ifdef __cplusplus
		}
		#endif
	#endif
	//#include <libxml/debugXML.h>
	#ifdef LIBXML_DEBUG_ENABLED
		#ifdef __cplusplus
		extern "C" {
		#endif
		/*
		 * The standard Dump routines.
		 */
		XMLPUBFUN void XMLCALL xmlDebugDumpString(FILE * output, const xmlChar * str);
		XMLPUBFUN void XMLCALL xmlDebugDumpAttr(FILE * output, xmlAttr * attr, int depth);
		XMLPUBFUN void XMLCALL xmlDebugDumpAttrList(FILE * output, xmlAttr * attr, int depth);
		XMLPUBFUN void XMLCALL xmlDebugDumpOneNode(FILE * output, xmlNode * pNode, int depth);
		XMLPUBFUN void XMLCALL xmlDebugDumpNode(FILE * output, xmlNode * pNode, int depth);
		XMLPUBFUN void XMLCALL xmlDebugDumpNodeList(FILE * output, xmlNode * pNode, int depth);
		XMLPUBFUN void XMLCALL xmlDebugDumpDocumentHead(FILE * output, xmlDoc * doc);
		XMLPUBFUN void XMLCALL xmlDebugDumpDocument(FILE * output, xmlDoc * doc);
		XMLPUBFUN void XMLCALL xmlDebugDumpDTD(FILE * output, xmlDtd * dtd);
		XMLPUBFUN void XMLCALL xmlDebugDumpEntities(FILE * output, xmlDoc * doc);
		// 
		// Checking routines
		// 
		XMLPUBFUN int XMLCALL xmlDebugCheckDocument(FILE * output, xmlDoc * doc);
		// 
		// XML shell helpers
		// 
		XMLPUBFUN void FASTCALL xmlLsOneNode(FILE * output, xmlNode * pNode);
		XMLPUBFUN int XMLCALL xmlLsCountNode(xmlNode * pNode);
		XMLPUBFUN const char * XMLCALL xmlBoolToText(int boolval);
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

			XMLPUBFUN void XMLCALL xmlShellPrintXPathError(int errorType, const char * arg);
			XMLPUBFUN void XMLCALL xmlShellPrintXPathResult(xmlXPathObject * list);
			XMLPUBFUN int XMLCALL xmlShellList(xmlShellCtxtPtr ctxt, char * arg, xmlNode * pNode, xmlNode * node2);
			XMLPUBFUN int XMLCALL xmlShellBase(xmlShellCtxtPtr ctxt, char * arg, xmlNode * pNode, xmlNode * node2);
			XMLPUBFUN int XMLCALL xmlShellDir(xmlShellCtxtPtr ctxt, char * arg, xmlNode * pNode, xmlNode * node2);
			XMLPUBFUN int XMLCALL xmlShellLoad(xmlShellCtxtPtr ctxt, char * filename, xmlNode * pNode, xmlNode * node2);
			#ifdef LIBXML_OUTPUT_ENABLED
				XMLPUBFUN void XMLCALL xmlShellPrintNode(xmlNode * pNode);
				XMLPUBFUN int XMLCALL xmlShellCat(xmlShellCtxtPtr ctxt, char * arg, xmlNode * pNode, xmlNode * node2);
				XMLPUBFUN int XMLCALL xmlShellWrite(xmlShellCtxtPtr ctxt, char * filename, xmlNode * pNode, xmlNode * node2);
				XMLPUBFUN int XMLCALL xmlShellSave(xmlShellCtxtPtr ctxt, char * filename, xmlNode * pNode, xmlNode * node2);
			#endif
			#ifdef LIBXML_VALID_ENABLED
				// @v10.9.0 XMLPUBFUN int XMLCALL xmlShellValidate(xmlShellCtxtPtr ctxt, char * dtd, xmlNode * pNode, xmlNode * node2);
			#endif
			// @v10.9.0 XMLPUBFUN int XMLCALL xmlShellDu(xmlShellCtxtPtr ctxt, char * arg, xmlNode * tree, xmlNode * node2);
			// @v10.9.0 XMLPUBFUN int XMLCALL xmlShellPwd(xmlShellCtxtPtr ctxt, char * buffer, xmlNode * pNode, xmlNode * node2);
			/*
			 * The Shell interface.
			 */
			// @v10.9.0 XMLPUBFUN void XMLCALL xmlShell(xmlDoc * doc, char * filename, xmlShellReadlineFunc input, FILE * output);
		#endif
		#ifdef __cplusplus
		}
		#endif
	#endif
	//#include <libxml/chvalid.h>
	#ifdef __cplusplus
	extern "C" {
	#endif
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
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlCharInRange(uint val, const xmlChRangeGroup * group);
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
	#define xmlIsDigit_ch(c)        (((0x30 <= (c)) && ((c) <= 0x39)))
	// 
	// @c: char to validate
	// Automatically generated by genChRanges.py
	// 
	#define xmlIsDigitQ(c) (((c) < 0x100) ? xmlIsDigit_ch((c)) : xmlCharInRange((c), &xmlIsDigitGroup))

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

	XMLPUBFUN int XMLCALL xmlIsBaseChar(uint ch);
	XMLPUBFUN int XMLCALL xmlIsBlank(uint ch);
	XMLPUBFUN int XMLCALL xmlIsChar(uint ch);
	XMLPUBFUN int XMLCALL xmlIsCombining(uint ch);
	// @v10.9.11 XMLPUBFUN int XMLCALL xmlIsDigit(uint ch);
	XMLPUBFUN int XMLCALL xmlIsExtender(uint ch);
	XMLPUBFUN int XMLCALL xmlIsIdeographic(uint ch);
	XMLPUBFUN int XMLCALL xmlIsPubidChar(uint ch);
	#ifdef __cplusplus
	}
	#endif
	//#include <libxml/xmlunicode.h>
	#ifdef LIBXML_UNICODE_ENABLED
		#ifdef __cplusplus
		extern "C" {
		#endif
		XMLPUBFUN int XMLCALL xmlUCSIsAegeanNumbers(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsAlphabeticPresentationForms(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsArabic(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsArabicPresentationFormsA(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsArabicPresentationFormsB(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsArmenian(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsArrows(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsBasicLatin(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsBengali(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsBlockElements(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsBopomofo(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsBopomofoExtended(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsBoxDrawing(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsBraillePatterns(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsBuhid	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsByzantineMusicalSymbols(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCJKCompatibility(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCJKCompatibilityForms(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCJKCompatibilityIdeographs(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCJKCompatibilityIdeographsSupplement(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCJKRadicalsSupplement(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCJKSymbolsandPunctuation(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCJKUnifiedIdeographs(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCJKUnifiedIdeographsExtensionA(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCJKUnifiedIdeographsExtensionB(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCherokee(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCombiningDiacriticalMarks(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCombiningDiacriticalMarksforSymbols(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCombiningHalfMarks(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCombiningMarksforSymbols(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsControlPictures(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCurrencySymbols(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCypriotSyllabary(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCyrillic(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCyrillicSupplement(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsDeseret(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsDevanagari(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsDingbats(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsEnclosedAlphanumerics(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsEnclosedCJKLettersandMonths(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsEthiopic(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsGeneralPunctuation(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsGeometricShapes(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsGeorgian(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsGothic(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsGreek(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsGreekExtended(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsGreekandCoptic(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsGujarati(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsGurmukhi(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsHalfwidthandFullwidthForms(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsHangulCompatibilityJamo(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsHangulJamo(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsHangulSyllables(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsHanunoo(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsHebrew(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsHighPrivateUseSurrogates(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsHighSurrogates(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsHiragana	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsIPAExtensions	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsIdeographicDescriptionCharacters	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsKanbun	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsKangxiRadicals	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsKannada	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsKatakana	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsKatakanaPhoneticExtensions	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsKhmer	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsKhmerSymbols	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsLao	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsLatin1Supplement(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsLatinExtendedA(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsLatinExtendedB(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsLatinExtendedAdditional(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsLetterlikeSymbols(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsLimbu	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsLinearBIdeograms(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsLinearBSyllabary(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsLowSurrogates	(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsMalayalam(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsMathematicalAlphanumericSymbols(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsMathematicalOperators(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsMiscellaneousMathematicalSymbolsA(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsMiscellaneousMathematicalSymbolsB(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsMiscellaneousSymbols(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsMiscellaneousSymbolsandArrows(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsMiscellaneousTechnical(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsMongolian(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsMusicalSymbols(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsMyanmar(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsNumberForms(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsOgham(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsOldItalic(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsOpticalCharacterRecognition(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsOriya(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsOsmanya(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsPhoneticExtensions(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsPrivateUse(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsPrivateUseArea(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsRunic(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsShavian(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsSinhala(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsSmallFormVariants(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsSpacingModifierLetters(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsSpecials(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsSuperscriptsandSubscripts(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsSupplementalArrowsA(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsSupplementalArrowsB(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsSupplementalMathematicalOperators(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsSupplementaryPrivateUseAreaA(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsSupplementaryPrivateUseAreaB(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsSyriac(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsTagalog(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsTagbanwa(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsTags(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsTaiLe(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsTaiXuanJingSymbols(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsTamil(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsTelugu(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsThaana(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsThai(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsTibetan(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsUgaritic(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsUnifiedCanadianAboriginalSyllabics(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsVariationSelectors(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsVariationSelectorsSupplement(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsYiRadicals(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsYiSyllables(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsYijingHexagramSymbols(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsBlock(int code, const char *block);
		XMLPUBFUN int XMLCALL xmlUCSIsCatC(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatCc(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatCf(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatCo(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatCs(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatL(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatLl(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatLm(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatLo(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatLt(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatLu(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatM(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatMc(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatMe(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatMn(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatN(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatNd(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatNl(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatNo(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatP(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatPc(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatPd(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatPe(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatPf(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatPi(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatPo(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatPs(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatS(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatSc(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatSk(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatSm(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatSo(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatZ(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatZl(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatZp(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCatZs(int code);
		XMLPUBFUN int XMLCALL xmlUCSIsCat(int code, const char *cat);
		#ifdef __cplusplus
		}
		#endif
	#endif
	// @v10.6.0 #include <libxml/HTMLparser.h>
	// @v10.6.0 #include <libxml/HTMLtree.h>
	#include <libxml/HTMLparser.h> // @v11.0.0
	#include <libxml/HTMLtree.h> // @v11.0.0
	#ifdef __cplusplus
	extern "C" {
	#endif
	// 
	// The dictionnary.
	// 
	//typedef struct _xmlDict xmlDict;
	//typedef xmlDict * xmlDictPtr;
	// 
	// Initializer
	// 
	// @v10.9.0 (depricated) XMLPUBFUN int XMLCALL xmlInitializeDict();
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
	XMLPUBFUN xmlURI * XMLCALL xmlCreateURI();
	XMLPUBFUN xmlChar * /*XMLCALL*/FASTCALL xmlBuildURI(const xmlChar * URI, const xmlChar * base);
	XMLPUBFUN xmlChar * XMLCALL xmlBuildRelativeURI(const xmlChar * URI, const xmlChar * base);
	XMLPUBFUN xmlURI * XMLCALL xmlParseURI(const char * str);
	XMLPUBFUN xmlURI * XMLCALL xmlParseURIRaw(const char * str, int raw);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlParseURIReference(xmlURI * uri, const char * str);
	XMLPUBFUN xmlChar * XMLCALL xmlSaveUri(xmlURI * uri);
	XMLPUBFUN void XMLCALL xmlPrintURI(FILE * stream, xmlURI * uri);
	XMLPUBFUN xmlChar * /*XMLCALL*/FASTCALL xmlURIEscapeStr(const xmlChar * str, const xmlChar * list);
	XMLPUBFUN char * /*XMLCALL*/FASTCALL xmlURIUnescapeString(const char * str, int len, char * target);
	XMLPUBFUN int XMLCALL xmlNormalizeURIPath(char * path);
	XMLPUBFUN xmlChar * /*XMLCALL*/FASTCALL  xmlURIEscape(const xmlChar * str);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeURI(xmlURI * pUri);
	XMLPUBFUN xmlChar* XMLCALL xmlCanonicPath(const xmlChar * path);
	XMLPUBFUN xmlChar* XMLCALL xmlPathToURI(const xmlChar * path);
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
	// save.h {
	#ifdef LIBXML_OUTPUT_ENABLED
		void xmlBufAttrSerializeTxtContent(xmlBufPtr buf, xmlDoc * doc, xmlAttr * attr, const xmlChar * string);
		void xmlBufDumpNotationTable(xmlBufPtr buf, xmlNotationTablePtr table);
		void xmlBufDumpElementDecl(xmlBufPtr buf, xmlElement * elem);
		void xmlBufDumpAttributeDecl(xmlBufPtr buf, xmlAttribute * attr);
		void xmlBufDumpEntityDecl(xmlBufPtr buf, xmlEntity * ent);
		xmlChar *xmlEncodeAttributeEntities(xmlDoc * doc, const xmlChar *input);
	#endif
	#ifdef __cplusplus
	}
	#endif
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
		//#include <libxml/xmlversion.h>
		//#include <libxml/tree.h>
		#ifdef LIBXML_XINCLUDE_ENABLED
			#ifdef __cplusplus
			extern "C" {
			#endif

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
			XMLPUBFUN int XMLCALL xmlXIncludeProcess(xmlDoc * doc);
			XMLPUBFUN int XMLCALL xmlXIncludeProcessFlags(xmlDoc * doc, int flags);
			XMLPUBFUN int XMLCALL xmlXIncludeProcessFlagsData(xmlDoc * doc, int flags, void * data);
			XMLPUBFUN int XMLCALL xmlXIncludeProcessTreeFlagsData(xmlNode * tree, int flags, void * data);
			XMLPUBFUN int XMLCALL xmlXIncludeProcessTree(xmlNode * tree);
			XMLPUBFUN int XMLCALL xmlXIncludeProcessTreeFlags(xmlNode * tree, int flags);
			// 
			// contextual processing
			// 
			XMLPUBFUN xmlXIncludeCtxtPtr XMLCALL xmlXIncludeNewContext(xmlDoc * doc);
			XMLPUBFUN int XMLCALL xmlXIncludeSetFlags(xmlXIncludeCtxtPtr ctxt, int flags);
			XMLPUBFUN void /*XMLCALL*/FASTCALL xmlXIncludeFreeContext(xmlXIncludeCtxtPtr ctxt);
			XMLPUBFUN int XMLCALL xmlXIncludeProcessNode(xmlXIncludeCtxtPtr ctxt, xmlNode * tree);
			#ifdef __cplusplus
			}
			#endif
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
