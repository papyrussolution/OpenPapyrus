/*
 * relaxng.c : implementation of the Relax-NG handling and validity checking
 *
 * See Copyright for the status of this software.
 *
 * Daniel Veillard <veillard@redhat.com>
 */

/**
 * @todo 
 * - add support for DTD compatibility spec
 *   http://www.oasis-open.org/committees/relax-ng/compatibility-20011203.html
 * - report better mem allocations pbms at runtime and abort immediately.
 */

#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop

#ifdef LIBXML_SCHEMAS_ENABLED
#include <libxml/relaxng.h>
#include <libxml/xmlschemastypes.h>
//#include <libxml/xmlautomata.h>
//#include <libxml/xmlregexp.h>
#include <libxml/xmlschemastypes.h>
/*
 * The Relax-NG namespace
 */
static const xmlChar * xmlRelaxNGNs = (const xmlChar*)"http://relaxng.org/ns/structure/1.0";

#define IS_RELAXNG(P_Node, typ) (P_Node && P_Node->ns && (P_Node->type == XML_ELEMENT_NODE) && (sstreq(P_Node->name, typ)) && (sstreq(P_Node->ns->href, xmlRelaxNGNs)))

#if 0
	#define DEBUG 1
	#define DEBUG_GRAMMAR 1
	#define DEBUG_CONTENT 1
	#define DEBUG_TYPE 1
	#define DEBUG_VALID 1
	#define DEBUG_INTERLEAVE 1
	#define DEBUG_LIST 1
	#define DEBUG_INCLUDE 1
	#define DEBUG_ERROR 1
	#define DEBUG_COMPILE 1
	#define DEBUG_PROGRESSIVE 1
#endif

#define MAX_ERROR 5

#define TODO xmlGenericError(0, "Unimplemented block at %s:%d\n", __FILE__, __LINE__);

typedef struct _xmlRelaxNGSchema xmlRelaxNGSchema;
typedef xmlRelaxNGSchema * xmlRelaxNGSchemaPtr;

typedef struct _xmlRelaxNGDefine xmlRelaxNGDefine;
typedef xmlRelaxNGDefine * xmlRelaxNGDefinePtr;

typedef struct _xmlRelaxNGDocument xmlRelaxNGDocument;
typedef xmlRelaxNGDocument * xmlRelaxNGDocumentPtr;

typedef struct _xmlRelaxNGInclude xmlRelaxNGInclude;
typedef xmlRelaxNGInclude * xmlRelaxNGIncludePtr;

typedef enum {
	XML_RELAXNG_COMBINE_UNDEFINED = 0, /* undefined */
	XML_RELAXNG_COMBINE_CHOICE, /* choice */
	XML_RELAXNG_COMBINE_INTERLEAVE  /* interleave */
} xmlRelaxNGCombine;

typedef enum {
	XML_RELAXNG_CONTENT_ERROR = -1,
	XML_RELAXNG_CONTENT_EMPTY = 0,
	XML_RELAXNG_CONTENT_SIMPLE,
	XML_RELAXNG_CONTENT_COMPLEX
} xmlRelaxNGContentType;

typedef struct _xmlRelaxNGGrammar xmlRelaxNGGrammar;
typedef xmlRelaxNGGrammar * xmlRelaxNGGrammarPtr;

struct _xmlRelaxNGGrammar {
	xmlRelaxNGGrammarPtr parent;    /* the parent grammar if any */
	xmlRelaxNGGrammarPtr children;  /* the children grammar if any */
	xmlRelaxNGGrammarPtr next; /* the next grammar if any */
	xmlRelaxNGDefinePtr start; /* <start> content */
	xmlRelaxNGCombine combine; /* the default combine value */
	xmlRelaxNGDefinePtr startList;  /* list of <start> definitions */
	xmlHashTable * defs;   /* define* */
	xmlHashTable * refs;   /* references */
};

typedef enum {
	XML_RELAXNG_NOOP = -1,  /* a no operation from simplification  */
	XML_RELAXNG_EMPTY = 0,  /* an empty pattern */
	XML_RELAXNG_NOT_ALLOWED, /* not allowed top */
	XML_RELAXNG_EXCEPT,     /* except present in nameclass defs */
	XML_RELAXNG_TEXT,       /* textual content */
	XML_RELAXNG_ELEMENT,    /* an element */
	XML_RELAXNG_DATATYPE,   /* extenal data type definition */
	XML_RELAXNG_PARAM,      /* extenal data type parameter */
	XML_RELAXNG_VALUE,      /* value from an extenal data type definition */
	XML_RELAXNG_LIST,       /* a list of patterns */
	XML_RELAXNG_ATTRIBUTE,  /* an attrbute following a pattern */
	XML_RELAXNG_DEF,        /* a definition */
	XML_RELAXNG_REF,        /* reference to a definition */
	XML_RELAXNG_EXTERNALREF, /* reference to an external def */
	XML_RELAXNG_PARENTREF,  /* reference to a def in the parent grammar */
	XML_RELAXNG_OPTIONAL,   /* optional patterns */
	XML_RELAXNG_ZEROORMORE, /* zero or more non empty patterns */
	XML_RELAXNG_ONEORMORE,  /* one or more non empty patterns */
	XML_RELAXNG_CHOICE,     /* a choice between non empty patterns */
	XML_RELAXNG_GROUP,      /* a pair/group of non empty patterns */
	XML_RELAXNG_INTERLEAVE, /* interleaving choice of non-empty patterns */
	XML_RELAXNG_START       /* Used to keep track of starts on grammars */
} xmlRelaxNGType;

#define IS_NULLABLE             (1 << 0)
#define IS_NOT_NULLABLE         (1 << 1)
#define IS_INDETERMINIST        (1 << 2)
#define IS_MIXED                (1 << 3)
#define IS_TRIABLE              (1 << 4)
#define IS_PROCESSED            (1 << 5)
#define IS_COMPILABLE           (1 << 6)
#define IS_NOT_COMPILABLE       (1 << 7)
#define IS_EXTERNAL_REF         (1 << 8)

struct _xmlRelaxNGDefine {
	xmlRelaxNGType type;    /* the type of definition */
	xmlNode * P_Node;        /* the node in the source */
	xmlChar * name;         /* the element local name if present */
	xmlChar * ns;           /* the namespace local name if present */
	xmlChar * value;        /* value when available */
	void * data;            /* data lib or specific pointer */
	xmlRelaxNGDefinePtr content;    /* the expected content */
	xmlRelaxNGDefinePtr parent; /* the parent definition, if any */
	xmlRelaxNGDefinePtr next; /* list within grouping sequences */
	xmlRelaxNGDefinePtr attrs; /* list of attributes for elements */
	xmlRelaxNGDefinePtr nameClass;  /* the nameClass definition if any */
	xmlRelaxNGDefinePtr nextHash;   /* next define in defs/refs hash tables */
	short depth;            /* used for the cycle detection */
	short dflags;           /* define related flags */
	xmlRegexpPtr contModel; /* a compiled content model if available */
};
/**
 * _xmlRelaxNG:
 *
 * A RelaxNGs definition
 */
struct _xmlRelaxNG {
	void * _private;        /* unused by the library for users or bindings */
	xmlRelaxNGGrammarPtr topgrammar;
	xmlDoc * doc;
	int idref;              /* requires idref checking */
	xmlHashTable * defs;   /* define */
	xmlHashTable * refs;   /* references */
	xmlRelaxNGDocumentPtr documents; /* all the documents loaded */
	xmlRelaxNGIncludePtr includes;  /* all the includes loaded */
	int defNr;              /* number of defines used */
	xmlRelaxNGDefinePtr * defTab;   /* pointer to the allocated definitions */
};

#define XML_RELAXNG_IN_ATTRIBUTE        (1 << 0)
#define XML_RELAXNG_IN_ONEORMORE        (1 << 1)
#define XML_RELAXNG_IN_LIST             (1 << 2)
#define XML_RELAXNG_IN_DATAEXCEPT       (1 << 3)
#define XML_RELAXNG_IN_START            (1 << 4)
#define XML_RELAXNG_IN_OOMGROUP         (1 << 5)
#define XML_RELAXNG_IN_OOMINTERLEAVE    (1 << 6)
#define XML_RELAXNG_IN_EXTERNALREF      (1 << 7)
#define XML_RELAXNG_IN_ANYEXCEPT        (1 << 8)
#define XML_RELAXNG_IN_NSEXCEPT         (1 << 9)

struct _xmlRelaxNGParserCtxt {
	void * userData;        /* user specific data block */
	xmlRelaxNGValidityErrorFunc error; /* the callback in case of errors */
	xmlRelaxNGValidityWarningFunc warning;  /* the callback in case of warning */
	xmlStructuredErrorFunc serror;
	xmlRelaxNGValidErr err;

	xmlRelaxNGPtr schema;   /* The schema in use */
	xmlRelaxNGGrammarPtr grammar;   /* the current grammar */
	xmlRelaxNGGrammarPtr parentgrammar; /* the parent grammar */
	int flags;              /* parser flags */
	int nbErrors;           /* number of errors at parse time */
	int nbWarnings;         /* number of warnings at parse time */
	const xmlChar * define; /* the current define scope */
	xmlRelaxNGDefinePtr def; /* the current define */

	int nbInterleaves;
	xmlHashTable * interleaves;    /* keep track of all the interleaves */
	xmlRelaxNGDocumentPtr documents; /* all the documents loaded */
	xmlRelaxNGIncludePtr includes;  /* all the includes loaded */
	xmlChar * URL;
	xmlDoc * document;
	int defNr;              /* number of defines used */
	int defMax;             /* number of defines aloocated */
	xmlRelaxNGDefinePtr * defTab;   /* pointer to the allocated definitions */
	const char * buffer;
	int size;
	/* the document stack */
	xmlRelaxNGDocumentPtr doc; /* Current parsed external ref */
	int docNr;              /* Depth of the parsing stack */
	int docMax;             /* Max depth of the parsing stack */
	xmlRelaxNGDocumentPtr * docTab; /* array of docs */

	/* the include stack */
	xmlRelaxNGIncludePtr inc; /* Current parsed include */
	int incNr;              /* Depth of the include parsing stack */
	int incMax;             /* Max depth of the parsing stack */
	xmlRelaxNGIncludePtr * incTab;  /* array of incs */

	int idref;              /* requires idref checking */

	/* used to compile content models */
	xmlAutomataPtr am;      /* the automata */
	xmlAutomataStatePtr state; /* used to build the automata */

	int crng;               /* compact syntax and other flags */
	int freedoc;            /* need to free the document */
};

#define FLAGS_IGNORABLE         1
#define FLAGS_NEGATIVE          2
#define FLAGS_MIXED_CONTENT     4
#define FLAGS_NOERROR           8

/**
 * xmlRelaxNGInterleaveGroup:
 *
 * A RelaxNGs partition set associated to lists of definitions
 */
typedef struct _xmlRelaxNGInterleaveGroup xmlRelaxNGInterleaveGroup;
typedef xmlRelaxNGInterleaveGroup * xmlRelaxNGInterleaveGroupPtr;
struct _xmlRelaxNGInterleaveGroup {
	xmlRelaxNGDefinePtr rule; /* the rule to satisfy */
	xmlRelaxNGDefinePtr * defs; /* the array of element definitions */
	xmlRelaxNGDefinePtr * attrs; /* the array of attributes definitions */
};

#define IS_DETERMINIST          1
#define IS_NEEDCHECK            2

/**
 * xmlRelaxNGPartitions:
 *
 * A RelaxNGs partition associated to an interleave group
 */
typedef struct _xmlRelaxNGPartition xmlRelaxNGPartition;
typedef xmlRelaxNGPartition * xmlRelaxNGPartitionPtr;
struct _xmlRelaxNGPartition {
	int nbgroups;           /* number of groups in the partitions */
	xmlHashTable * triage; // hash table used to direct nodes to the right group when possible 
	int flags;              /* determinist ? */
	xmlRelaxNGInterleaveGroupPtr * groups;
};

/**
 * xmlRelaxNGValidState:
 *
 * A RelaxNGs validation state
 */
#define MAX_ATTR 20
typedef struct _xmlRelaxNGValidState xmlRelaxNGValidState;
typedef xmlRelaxNGValidState * xmlRelaxNGValidStatePtr;
struct _xmlRelaxNGValidState {
	xmlNode * P_Node;        /* the current node */
	xmlNode * seq;         /* the sequence of children left to validate */
	int nbAttrs;            /* the number of attributes */
	int maxAttrs;           /* the size of attrs */
	int nbAttrLeft;         /* the number of attributes left to validate */
	xmlChar * value;        /* the value when operating on string */
	xmlChar * endvalue;     /* the end value when operating on string */
	xmlAttrPtr * attrs;     /* the array of attributes */
};

/**
 * xmlRelaxNGStates:
 *
 * A RelaxNGs container for validation state
 */
typedef struct _xmlRelaxNGStates xmlRelaxNGStates;
typedef xmlRelaxNGStates * xmlRelaxNGStatesPtr;
struct _xmlRelaxNGStates {
	int nbState;            /* the number of states */
	int maxState;           /* the size of the array */
	xmlRelaxNGValidStatePtr * tabState;
};

#define ERROR_IS_DUP    1

/**
 * xmlRelaxNGValidError:
 *
 * A RelaxNGs validation error
 */
typedef struct _xmlRelaxNGValidError xmlRelaxNGValidError;
typedef xmlRelaxNGValidError * xmlRelaxNGValidErrorPtr;
struct _xmlRelaxNGValidError {
	xmlRelaxNGValidErr err; /* the error number */
	int flags;              /* flags */
	xmlNode * P_Node;        /* the current node */
	xmlNode * seq;         /* the current child */
	const xmlChar * arg1;   /* first arg */
	const xmlChar * arg2;   /* second arg */
};

/**
 * xmlRelaxNGValidCtxt:
 *
 * A RelaxNGs validation context
 */

struct _xmlRelaxNGValidCtxt {
	void * userData;        /* user specific data block */
	xmlRelaxNGValidityErrorFunc error; /* the callback in case of errors */
	xmlRelaxNGValidityWarningFunc warning;  /* the callback in case of warning */
	xmlStructuredErrorFunc serror;
	int nbErrors;           /* number of errors in validation */

	xmlRelaxNGPtr schema;   /* The schema in use */
	xmlDoc * doc;          /* the document being validated */
	int flags;              /* validation flags */
	int depth;              /* validation depth */
	int idref;              /* requires idref checking */
	int errNo;              /* the first error found */

	/*
	 * Errors accumulated in branches may have to be stacked to be
	 * provided back when it's sure they affect validation.
	 */
	xmlRelaxNGValidErrorPtr err;    /* Last error */
	int errNr;              /* Depth of the error stack */
	int errMax;             /* Max depth of the error stack */
	xmlRelaxNGValidErrorPtr errTab; /* stack of errors */

	xmlRelaxNGValidStatePtr state;  /* the current validation state */
	xmlRelaxNGStatesPtr states; /* the accumulated state list */

	xmlRelaxNGStatesPtr freeState;  /* the pool of free valid states */
	int freeStatesNr;
	int freeStatesMax;
	xmlRelaxNGStatesPtr * freeStates; /* the pool of free state groups */

	/*
	 * This is used for "progressive" validation
	 */
	xmlRegExecCtxtPtr elem; /* the current element regexp */
	int elemNr;             /* the number of element validated */
	int elemMax;            /* the max depth of elements */
	xmlRegExecCtxtPtr * elemTab; /* the stack of regexp runtime */
	int pstate;             /* progressive state */
	xmlNode * pnode;       /* the current node */
	xmlRelaxNGDefinePtr pdef; /* the non-streamable definition */
	int perr;               /* signal error in content model
	                         * outside the regexp */
};

/**
 * xmlRelaxNGInclude:
 *
 * Structure associated to a RelaxNGs document element
 */
struct _xmlRelaxNGInclude {
	xmlRelaxNGIncludePtr next; /* keep a chain of includes */
	xmlChar * href;         /* the normalized href value */
	xmlDoc * doc;          /* the associated XML document */
	xmlRelaxNGDefinePtr content;    /* the definitions */
	xmlRelaxNGPtr schema;   /* the schema */
};

/**
 * xmlRelaxNGDocument:
 *
 * Structure associated to a RelaxNGs document element
 */
struct _xmlRelaxNGDocument {
	xmlRelaxNGDocumentPtr next; /* keep a chain of documents */
	xmlChar * href;         /* the normalized href value */
	xmlDoc * doc;          /* the associated XML document */
	xmlRelaxNGDefinePtr content;    /* the definitions */
	xmlRelaxNGPtr schema;   /* the schema */
	int externalRef;        /* 1 if an external ref */
};

/************************************************************************
*									*
*		Some factorized error routines				*
*									*
************************************************************************/

/**
 * xmlRngPErrMemory:
 * @ctxt:  an Relax-NG parser context
 * @extra:  extra informations
 *
 * Handle a redefinition of attribute error
 */
static void FASTCALL xmlRngPErrMemory(xmlRelaxNGParserCtxt * ctxt, const char * extra)
{
	xmlStructuredErrorFunc schannel = NULL;
	xmlGenericErrorFunc channel = NULL;
	void * data = NULL;
	if(ctxt) {
		if(ctxt->serror)
			schannel = ctxt->serror;
		else
			channel = ctxt->error;
		data = ctxt->userData;
		ctxt->nbErrors++;
	}
	if(extra)
		__xmlRaiseError(schannel, channel, data, NULL, NULL, XML_FROM_RELAXNGP, XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0, extra,
		    NULL, NULL, 0, 0, "Memory allocation failed : %s\n", extra);
	else
		__xmlRaiseError(schannel, channel, data, NULL, NULL, XML_FROM_RELAXNGP, XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0, NULL,
		    NULL, NULL, 0, 0, "Memory allocation failed\n");
}
/**
 * xmlRngVErrMemory:
 * @ctxt:  a Relax-NG validation context
 * @extra:  extra informations
 *
 * Handle a redefinition of attribute error
 */
static void FASTCALL xmlRngVErrMemory(xmlRelaxNGValidCtxt * ctxt, const char * extra)
{
	xmlStructuredErrorFunc schannel = NULL;
	xmlGenericErrorFunc channel = NULL;
	void * data = NULL;
	if(ctxt) {
		if(ctxt->serror)
			schannel = ctxt->serror;
		else
			channel = ctxt->error;
		data = ctxt->userData;
		ctxt->nbErrors++;
	}
	if(extra)
		__xmlRaiseError(schannel, channel, data, NULL, NULL, XML_FROM_RELAXNGV, XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0, extra,
		    NULL, NULL, 0, 0, "Memory allocation failed : %s\n", extra);
	else
		__xmlRaiseError(schannel, channel, data, NULL, NULL, XML_FROM_RELAXNGV, XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0, NULL,
		    NULL, NULL, 0, 0, "Memory allocation failed\n");
}
/**
 * xmlRngPErr:
 * @ctxt:  a Relax-NG parser context
 * @node:  the node raising the error
 * @error:  the error code
 * @msg:  message
 * @str1:  extra info
 * @str2:  extra info
 *
 * Handle a Relax NG Parsing error
 */
static void FASTCALL xmlRngPErr(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node, int error, const char * msg, const xmlChar * str1, const xmlChar * str2)
{
	xmlStructuredErrorFunc schannel = NULL;
	xmlGenericErrorFunc channel = NULL;
	void * data = NULL;
	if(ctxt) {
		if(ctxt->serror)
			schannel = ctxt->serror;
		else
			channel = ctxt->error;
		data = ctxt->userData;
		ctxt->nbErrors++;
	}
	__xmlRaiseError(schannel, channel, data, NULL, P_Node, XML_FROM_RELAXNGP, error, XML_ERR_ERROR, NULL, 0, (const char*)str1, (const char*)str2, NULL, 0, 0, msg, str1, str2);
}
/**
 * xmlRngVErr:
 * @ctxt:  a Relax-NG validation context
 * @node:  the node raising the error
 * @error:  the error code
 * @msg:  message
 * @str1:  extra info
 * @str2:  extra info
 *
 * Handle a Relax NG Validation error
 */
static void xmlRngVErr(xmlRelaxNGValidCtxtPtr ctxt, xmlNode * P_Node, int error, const char * msg, const xmlChar * str1, const xmlChar * str2)
{
	xmlStructuredErrorFunc schannel = NULL;
	xmlGenericErrorFunc channel = NULL;
	void * data = NULL;
	if(ctxt) {
		if(ctxt->serror)
			schannel = ctxt->serror;
		else
			channel = ctxt->error;
		data = ctxt->userData;
		ctxt->nbErrors++;
	}
	__xmlRaiseError(schannel, channel, data, NULL, P_Node, XML_FROM_RELAXNGV, error, XML_ERR_ERROR, NULL, 0,
	    (const char*)str1, (const char*)str2, NULL, 0, 0, msg, str1, str2);
}
/************************************************************************
*									*
*		Preliminary type checking interfaces			*
*									*
************************************************************************/

/**
 * xmlRelaxNGTypeHave:
 * @data:  data needed for the library
 * @type:  the type name
 * @value:  the value to check
 *
 * Function provided by a type library to check if a type is exported
 *
 * Returns 1 if yes, 0 if no and -1 in case of error.
 */
typedef int (*xmlRelaxNGTypeHave)(void * data, const xmlChar * type);

/**
 * xmlRelaxNGTypeCheck:
 * @data:  data needed for the library
 * @type:  the type name
 * @value:  the value to check
 * @result:  place to store the result if needed
 *
 * Function provided by a type library to check if a value match a type
 *
 * Returns 1 if yes, 0 if no and -1 in case of error.
 */
typedef int (*xmlRelaxNGTypeCheck)(void * data, const xmlChar * type, const xmlChar * value, void ** result, xmlNode * P_Node);
/**
 * xmlRelaxNGFacetCheck:
 * @data:  data needed for the library
 * @type:  the type name
 * @facet:  the facet name
 * @val:  the facet value
 * @strval:  the string value
 * @value:  the value to check
 *
 * Function provided by a type library to check a value facet
 *
 * Returns 1 if yes, 0 if no and -1 in case of error.
 */
typedef int (*xmlRelaxNGFacetCheck)(void * data, const xmlChar * type, const xmlChar * facet, const xmlChar * val, const xmlChar * strval, void * value);
/**
 * xmlRelaxNGTypeFree:
 * @data:  data needed for the library
 * @result:  the value to free
 *
 * Function provided by a type library to free a returned result
 */
typedef void (*xmlRelaxNGTypeFree)(void * data, void * result);
/**
 * xmlRelaxNGTypeCompare:
 * @data:  data needed for the library
 * @type:  the type name
 * @value1:  the first value
 * @value2:  the second value
 *
 * Function provided by a type library to compare two values accordingly
 * to a type.
 *
 * Returns 1 if yes, 0 if no and -1 in case of error.
 */
typedef int (*xmlRelaxNGTypeCompare)(void * data, const xmlChar * type, const xmlChar * value1, xmlNode * ctxt1, void * comp1, const xmlChar * value2, xmlNode * ctxt2);
typedef struct _xmlRelaxNGTypeLibrary xmlRelaxNGTypeLibrary;
typedef xmlRelaxNGTypeLibrary * xmlRelaxNGTypeLibraryPtr;

struct _xmlRelaxNGTypeLibrary {
	const xmlChar * P_Namespace; /* the datatypeLibrary value */
	void * data;            /* data needed for the library */
	xmlRelaxNGTypeHave have; /* the export function */
	xmlRelaxNGTypeCheck check; /* the checking function */
	xmlRelaxNGTypeCompare comp; /* the compare function */
	xmlRelaxNGFacetCheck facet; /* the facet check function */
	xmlRelaxNGTypeFree freef; /* the freeing function */
};

/************************************************************************
*									*
*			Allocation functions				*
*									*
************************************************************************/
static void xmlRelaxNGFreeGrammar(xmlRelaxNGGrammarPtr grammar);
static void xmlRelaxNGFreeDefine(xmlRelaxNGDefinePtr define);
static void xmlRelaxNGNormExtSpace(xmlChar * value);
static void xmlRelaxNGFreeInnerSchema(xmlRelaxNG * schema);
static int xmlRelaxNGEqualValidState(xmlRelaxNGValidCtxtPtr ctxt ATTRIBUTE_UNUSED, xmlRelaxNGValidStatePtr state1, xmlRelaxNGValidStatePtr state2);
static void FASTCALL xmlRelaxNGFreeValidState(xmlRelaxNGValidCtxt * ctxt, xmlRelaxNGValidState * state);
/**
 * xmlRelaxNGFreeDocument:
 * @docu:  a document structure
 *
 * Deallocate a RelaxNG document structure.
 */
static void xmlRelaxNGFreeDocument(xmlRelaxNGDocument * docu)
{
	if(docu) {
		SAlloc::F(docu->href);
		xmlFreeDoc(docu->doc);
		xmlRelaxNGFreeInnerSchema(docu->schema);
		SAlloc::F(docu);
	}
}
/**
 * xmlRelaxNGFreeDocumentList:
 * @docu:  a list of  document structure
 *
 * Deallocate a RelaxNG document structures.
 */
static void xmlRelaxNGFreeDocumentList(xmlRelaxNGDocumentPtr docu)
{
	while(docu) {
		xmlRelaxNGDocument * next = docu->next;
		xmlRelaxNGFreeDocument(docu);
		docu = next;
	}
}
/**
 * xmlRelaxNGFreeInclude:
 * @incl:  a include structure
 *
 * Deallocate a RelaxNG include structure.
 */
static void xmlRelaxNGFreeInclude(xmlRelaxNGIncludePtr incl)
{
	if(incl) {
		SAlloc::F(incl->href);
		if(incl->doc)
			xmlFreeDoc(incl->doc);
		xmlRelaxNGFree(incl->schema);
		SAlloc::F(incl);
	}
}
/**
 * xmlRelaxNGFreeIncludeList:
 * @incl:  a include structure list
 *
 * Deallocate a RelaxNG include structure.
 */
static void xmlRelaxNGFreeIncludeList(xmlRelaxNGIncludePtr incl)
{
	while(incl) {
		xmlRelaxNGIncludePtr next = incl->next;
		xmlRelaxNGFreeInclude(incl);
		incl = next;
	}
}
/**
 * xmlRelaxNGNewRelaxNG:
 * @ctxt:  a Relax-NG validation context (optional)
 *
 * Allocate a new RelaxNG structure.
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static xmlRelaxNGPtr xmlRelaxNGNewRelaxNG(xmlRelaxNGParserCtxtPtr ctxt)
{
	xmlRelaxNGPtr ret = (xmlRelaxNGPtr)SAlloc::M(sizeof(xmlRelaxNG));
	if(!ret)
		xmlRngPErrMemory(ctxt, 0);
	else
		memzero(ret, sizeof(xmlRelaxNG));
	return ret;
}
/**
 * xmlRelaxNGFreeInnerSchema:
 * @schema:  a schema structure
 *
 * Deallocate a RelaxNG schema structure.
 */
static void xmlRelaxNGFreeInnerSchema(xmlRelaxNG * schema)
{
	if(schema) {
		xmlFreeDoc(schema->doc);
		if(schema->defTab) {
			for(int i = 0; i < schema->defNr; i++)
				xmlRelaxNGFreeDefine(schema->defTab[i]);
			SAlloc::F(schema->defTab);
		}
		SAlloc::F(schema);
	}
}
/**
 * xmlRelaxNGFree:
 * @schema:  a schema structure
 *
 * Deallocate a RelaxNG structure.
 */
void xmlRelaxNGFree(xmlRelaxNG * schema)
{
	if(schema) {
		xmlRelaxNGFreeGrammar(schema->topgrammar);
		xmlFreeDoc(schema->doc);
		xmlRelaxNGFreeDocumentList(schema->documents);
		xmlRelaxNGFreeIncludeList(schema->includes);
		if(schema->defTab) {
			for(int i = 0; i < schema->defNr; i++)
				xmlRelaxNGFreeDefine(schema->defTab[i]);
			SAlloc::F(schema->defTab);
		}
		SAlloc::F(schema);
	}
}
/**
 * xmlRelaxNGNewGrammar:
 * @ctxt:  a Relax-NG validation context (optional)
 *
 * Allocate a new RelaxNG grammar.
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static xmlRelaxNGGrammarPtr xmlRelaxNGNewGrammar(xmlRelaxNGParserCtxtPtr ctxt)
{
	xmlRelaxNGGrammarPtr ret = (xmlRelaxNGGrammarPtr)SAlloc::M(sizeof(xmlRelaxNGGrammar));
	if(!ret)
		xmlRngPErrMemory(ctxt, 0);
	else
		memzero(ret, sizeof(xmlRelaxNGGrammar));
	return ret;
}
/**
 * xmlRelaxNGFreeGrammar:
 * @grammar:  a grammar structure
 *
 * Deallocate a RelaxNG grammar structure.
 */
static void xmlRelaxNGFreeGrammar(xmlRelaxNGGrammarPtr grammar)
{
	if(grammar) {
		xmlRelaxNGFreeGrammar(grammar->children); // @recursion
		xmlRelaxNGFreeGrammar(grammar->next); // @recursion
		xmlHashFree(grammar->refs, 0);
		xmlHashFree(grammar->defs, 0);
		SAlloc::F(grammar);
	}
}
/**
 * xmlRelaxNGNewDefine:
 * @ctxt:  a Relax-NG validation context
 * @node:  the node in the input document.
 *
 * Allocate a new RelaxNG define.
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static xmlRelaxNGDefine * FASTCALL xmlRelaxNGNewDefine(xmlRelaxNGParserCtxt * ctxt, xmlNode * P_Node)
{
	xmlRelaxNGDefine * ret;
	if(ctxt->defMax == 0) {
		ctxt->defMax = 16;
		ctxt->defNr = 0;
		ctxt->defTab = (xmlRelaxNGDefinePtr*)SAlloc::M(ctxt->defMax * sizeof(xmlRelaxNGDefinePtr));
		if(ctxt->defTab == NULL) {
			xmlRngPErrMemory(ctxt, "allocating define\n");
			return 0;
		}
	}
	else if(ctxt->defMax <= ctxt->defNr) {
		xmlRelaxNGDefine ** tmp;
		ctxt->defMax *= 2;
		tmp = (xmlRelaxNGDefine **)SAlloc::R(ctxt->defTab, ctxt->defMax * sizeof(xmlRelaxNGDefine *));
		if(!tmp) {
			xmlRngPErrMemory(ctxt, "allocating define\n");
			return 0;
		}
		ctxt->defTab = tmp;
	}
	ret = (xmlRelaxNGDefine *)SAlloc::M(sizeof(xmlRelaxNGDefine));
	if(!ret) {
		xmlRngPErrMemory(ctxt, "allocating define\n");
	}
	else {
		memzero(ret, sizeof(xmlRelaxNGDefine));
		ctxt->defTab[ctxt->defNr++] = ret;
		ret->P_Node = P_Node;
		ret->depth = -1;
	}
	return ret;
}
/**
 * xmlRelaxNGFreePartition:
 * @partitions:  a partition set structure
 *
 * Deallocate RelaxNG partition set structures.
 */
static void xmlRelaxNGFreePartition(xmlRelaxNGPartitionPtr partitions)
{
	if(partitions) {
		if(partitions->groups) {
			for(int j = 0; j < partitions->nbgroups; j++) {
				xmlRelaxNGInterleaveGroup * group = partitions->groups[j];
				if(group) {
					SAlloc::F(group->defs);
					SAlloc::F(group->attrs);
					SAlloc::F(group);
				}
			}
			SAlloc::F(partitions->groups);
		}
		xmlHashFree(partitions->triage, 0);
		SAlloc::F(partitions);
	}
}
/**
 * xmlRelaxNGFreeDefine:
 * @define:  a define structure
 *
 * Deallocate a RelaxNG define structure.
 */
static void xmlRelaxNGFreeDefine(xmlRelaxNGDefinePtr define)
{
	if(define) {
		if(define->type == XML_RELAXNG_VALUE && define->attrs) {
			xmlRelaxNGTypeLibraryPtr lib = (xmlRelaxNGTypeLibraryPtr)define->data;
			if(lib && lib->freef)
				lib->freef(lib->data, (void*)define->attrs);
		}
		if(define->data && define->type == XML_RELAXNG_INTERLEAVE)
			xmlRelaxNGFreePartition((xmlRelaxNGPartitionPtr)define->data);
		if(define->data && define->type == XML_RELAXNG_CHOICE)
			xmlHashFree((xmlHashTable *)define->data, 0);
		SAlloc::F(define->name);
		SAlloc::F(define->ns);
		SAlloc::F(define->value);
		xmlRegFreeRegexp(define->contModel);
		SAlloc::F(define);
	}
}

/**
 * xmlRelaxNGNewStates:
 * @ctxt:  a Relax-NG validation context
 * @size:  the default size for the container
 *
 * Allocate a new RelaxNG validation state container
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static xmlRelaxNGStatesPtr xmlRelaxNGNewStates(xmlRelaxNGValidCtxtPtr ctxt, int size)
{
	xmlRelaxNGStates * ret = 0;
	if(ctxt && ctxt->freeStates && (ctxt->freeStatesNr > 0)) {
		ctxt->freeStatesNr--;
		ret = ctxt->freeStates[ctxt->freeStatesNr];
		ret->nbState = 0;
	}
	else {
		SETMAX(size, 16);
		ret = (xmlRelaxNGStatesPtr)SAlloc::M(sizeof(xmlRelaxNGStates) + (size - 1) * sizeof(xmlRelaxNGValidStatePtr));
		if(!ret)
			xmlRngVErrMemory(ctxt, "allocating states\n");
		else {
			ret->nbState = 0;
			ret->maxState = size;
			ret->tabState = (xmlRelaxNGValidStatePtr*)SAlloc::M((size) * sizeof(xmlRelaxNGValidStatePtr));
			if(ret->tabState == NULL) {
				xmlRngVErrMemory(ctxt, "allocating states\n");
				SAlloc::F(ret);
				return 0;
			}
		}
	}
	return ret;
}

/**
 * xmlRelaxNGAddStateUniq:
 * @ctxt:  a Relax-NG validation context
 * @states:  the states container
 * @state:  the validation state
 *
 * Add a RelaxNG validation state to the container without checking
 * for unicity.
 *
 * Return 1 in case of success and 0 if this is a duplicate and -1 on error
 */
static int xmlRelaxNGAddStatesUniq(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGStatesPtr states, xmlRelaxNGValidStatePtr state)
{
	if(state == NULL) {
		return -1;
	}
	else {
		if(states->nbState >= states->maxState) {
			int size = states->maxState * 2;
			xmlRelaxNGValidStatePtr * tmp = (xmlRelaxNGValidStatePtr*)SAlloc::R(states->tabState, (size) * sizeof(xmlRelaxNGValidStatePtr));
			if(!tmp) {
				xmlRngVErrMemory(ctxt, "adding states\n");
				return -1;
			}
			else {
				states->tabState = tmp;
				states->maxState = size;
			}
		}
		states->tabState[states->nbState++] = state;
		return 1;
	}
}
/**
 * xmlRelaxNGAddState:
 * @ctxt:  a Relax-NG validation context
 * @states:  the states container
 * @state:  the validation state
 *
 * Add a RelaxNG validation state to the container
 *
 * Return 1 in case of success and 0 if this is a duplicate and -1 on error
 */
static int xmlRelaxNGAddStates(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGStatesPtr states, xmlRelaxNGValidStatePtr state)
{
	if(state == NULL || states == NULL) {
		return -1;
	}
	else {
		if(states->nbState >= states->maxState) {
			int size = states->maxState * 2;
			xmlRelaxNGValidStatePtr * tmp = (xmlRelaxNGValidStatePtr*)SAlloc::R(states->tabState, (size) * sizeof(xmlRelaxNGValidStatePtr));
			if(!tmp) {
				xmlRngVErrMemory(ctxt, "adding states\n");
				return -1;
			}
			else {
				states->tabState = tmp;
				states->maxState = size;
			}
		}
		for(int i = 0; i < states->nbState; i++) {
			if(xmlRelaxNGEqualValidState(ctxt, state, states->tabState[i])) {
				xmlRelaxNGFreeValidState(ctxt, state);
				return 0;
			}
		}
		states->tabState[states->nbState++] = state;
		return 1;
	}
}
/**
 * xmlRelaxNGFreeStates:
 * @ctxt:  a Relax-NG validation context
 * @states:  teh container
 *
 * Free a RelaxNG validation state container
 */
static void FASTCALL xmlRelaxNGFreeStates(xmlRelaxNGValidCtxt * ctxt, xmlRelaxNGStates * states)
{
	if(states) {
		if(ctxt && (ctxt->freeStates == NULL)) {
			ctxt->freeStatesMax = 40;
			ctxt->freeStatesNr = 0;
			ctxt->freeStates = (xmlRelaxNGStatesPtr*)SAlloc::M(ctxt->freeStatesMax * sizeof(xmlRelaxNGStatesPtr));
			if(ctxt->freeStates == NULL) {
				xmlRngVErrMemory(ctxt, "storing states\n");
			}
		}
		else if((ctxt) && (ctxt->freeStatesNr >= ctxt->freeStatesMax)) {
			xmlRelaxNGStatesPtr * tmp;
			tmp = (xmlRelaxNGStatesPtr*)SAlloc::R(ctxt->freeStates, 2 * ctxt->freeStatesMax * sizeof(xmlRelaxNGStatesPtr));
			if(!tmp) {
				xmlRngVErrMemory(ctxt, "storing states\n");
				SAlloc::F(states->tabState);
				SAlloc::F(states);
				return;
			}
			ctxt->freeStates = tmp;
			ctxt->freeStatesMax *= 2;
		}
		if(!ctxt || (ctxt->freeStates == NULL)) {
			SAlloc::F(states->tabState);
			SAlloc::F(states);
		}
		else {
			ctxt->freeStates[ctxt->freeStatesNr++] = states;
		}
	}
}

/**
 * xmlRelaxNGNewValidState:
 * @ctxt:  a Relax-NG validation context
 * @node:  the current node or NULL for the document
 *
 * Allocate a new RelaxNG validation state
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static xmlRelaxNGValidStatePtr xmlRelaxNGNewValidState(xmlRelaxNGValidCtxtPtr ctxt, xmlNode * P_Node)
{
	xmlRelaxNGValidStatePtr ret;
	xmlAttr * attr;
	xmlAttrPtr attrs[MAX_ATTR];
	int nbAttrs = 0;
	xmlNode * root = NULL;

	if(!P_Node) {
		root = xmlDocGetRootElement(ctxt->doc);
		if(root == NULL)
			return 0;
	}
	else {
		attr = P_Node->properties;
		while(attr) {
			if(nbAttrs < MAX_ATTR)
				attrs[nbAttrs++] = attr;
			else
				nbAttrs++;
			attr = attr->next;
		}
	}
	if(ctxt->freeState && (ctxt->freeState->nbState > 0)) {
		ctxt->freeState->nbState--;
		ret = ctxt->freeState->tabState[ctxt->freeState->nbState];
	}
	else {
		ret = (xmlRelaxNGValidStatePtr)SAlloc::M(sizeof(xmlRelaxNGValidState));
		if(!ret) {
			xmlRngVErrMemory(ctxt, "allocating states\n");
			return 0;
		}
		memzero(ret, sizeof(xmlRelaxNGValidState));
	}
	ret->value = NULL;
	ret->endvalue = NULL;
	if(!P_Node) {
		ret->P_Node = (xmlNode *)ctxt->doc;
		ret->seq = root;
	}
	else {
		ret->P_Node = P_Node;
		ret->seq = P_Node->children;
	}
	ret->nbAttrs = 0;
	if(nbAttrs > 0) {
		if(ret->attrs == NULL) {
			if(nbAttrs < 4)
				ret->maxAttrs = 4;
			else
				ret->maxAttrs = nbAttrs;
			ret->attrs = (xmlAttrPtr*)SAlloc::M(ret->maxAttrs *
			    sizeof(xmlAttr *));
			if(ret->attrs == NULL) {
				xmlRngVErrMemory(ctxt, "allocating states\n");
				return ret;
			}
		}
		else if(ret->maxAttrs < nbAttrs) {
			xmlAttrPtr * tmp = (xmlAttrPtr*)SAlloc::R(ret->attrs, nbAttrs * sizeof(xmlAttr *));
			if(!tmp) {
				xmlRngVErrMemory(ctxt, "allocating states\n");
				return ret;
			}
			ret->attrs = tmp;
			ret->maxAttrs = nbAttrs;
		}
		ret->nbAttrs = nbAttrs;
		if(nbAttrs < MAX_ATTR) {
			memcpy(ret->attrs, attrs, sizeof(xmlAttr *) * nbAttrs);
		}
		else {
			attr = P_Node->properties;
			nbAttrs = 0;
			while(attr) {
				ret->attrs[nbAttrs++] = attr;
				attr = attr->next;
			}
		}
	}
	ret->nbAttrLeft = ret->nbAttrs;
	return ret;
}

/**
 * xmlRelaxNGCopyValidState:
 * @ctxt:  a Relax-NG validation context
 * @state:  a validation state
 *
 * Copy the validation state
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static xmlRelaxNGValidStatePtr xmlRelaxNGCopyValidState(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGValidStatePtr state)
{
	xmlRelaxNGValidStatePtr ret;
	uint maxAttrs;
	xmlAttrPtr * attrs;
	if(state == NULL)
		return 0;
	if(ctxt->freeState && (ctxt->freeState->nbState > 0)) {
		ctxt->freeState->nbState--;
		ret = ctxt->freeState->tabState[ctxt->freeState->nbState];
	}
	else {
		ret = (xmlRelaxNGValidStatePtr)SAlloc::M(sizeof(xmlRelaxNGValidState));
		if(!ret) {
			xmlRngVErrMemory(ctxt, "allocating states\n");
			return 0;
		}
		memzero(ret, sizeof(xmlRelaxNGValidState));
	}
	attrs = ret->attrs;
	maxAttrs = ret->maxAttrs;
	memcpy(ret, state, sizeof(xmlRelaxNGValidState));
	ret->attrs = attrs;
	ret->maxAttrs = maxAttrs;
	if(state->nbAttrs > 0) {
		if(ret->attrs == NULL) {
			ret->maxAttrs = state->maxAttrs;
			ret->attrs = (xmlAttrPtr*)SAlloc::M(ret->maxAttrs *
			    sizeof(xmlAttr *));
			if(ret->attrs == NULL) {
				xmlRngVErrMemory(ctxt, "allocating states\n");
				ret->nbAttrs = 0;
				return ret;
			}
		}
		else if(ret->maxAttrs < state->nbAttrs) {
			xmlAttrPtr * tmp;

			tmp = (xmlAttrPtr*)SAlloc::R(ret->attrs, state->maxAttrs *
			    sizeof(xmlAttr *));
			if(!tmp) {
				xmlRngVErrMemory(ctxt, "allocating states\n");
				ret->nbAttrs = 0;
				return ret;
			}
			ret->maxAttrs = state->maxAttrs;
			ret->attrs = tmp;
		}
		memcpy(ret->attrs, state->attrs,
		    state->nbAttrs * sizeof(xmlAttr *));
	}
	return ret;
}

/**
 * xmlRelaxNGEqualValidState:
 * @ctxt:  a Relax-NG validation context
 * @state1:  a validation state
 * @state2:  a validation state
 *
 * Compare the validation states for equality
 *
 * Returns 1 if equald, 0 otherwise
 */
static int xmlRelaxNGEqualValidState(xmlRelaxNGValidCtxtPtr ctxt ATTRIBUTE_UNUSED, xmlRelaxNGValidStatePtr state1, xmlRelaxNGValidStatePtr state2)
{
	int i;
	if((state1 == NULL) || (state2 == NULL))
		return 0;
	if(state1 == state2)
		return 1;
	if(state1->P_Node != state2->P_Node)
		return 0;
	if(state1->seq != state2->seq)
		return 0;
	if(state1->nbAttrLeft != state2->nbAttrLeft)
		return 0;
	if(state1->nbAttrs != state2->nbAttrs)
		return 0;
	if(state1->endvalue != state2->endvalue)
		return 0;
	if(state1->value != state2->value && (!sstreq(state1->value, state2->value)))
		return 0;
	for(i = 0; i < state1->nbAttrs; i++) {
		if(state1->attrs[i] != state2->attrs[i])
			return 0;
	}
	return 1;
}
/**
 * xmlRelaxNGFreeValidState:
 * @state:  a validation state structure
 *
 * Deallocate a RelaxNG validation state structure.
 */
static void FASTCALL xmlRelaxNGFreeValidState(xmlRelaxNGValidCtxt * ctxt, xmlRelaxNGValidState * state)
{
	if(state) {
		if(ctxt && (ctxt->freeState == NULL)) {
			ctxt->freeState = xmlRelaxNGNewStates(ctxt, 40);
		}
		if(!ctxt || (ctxt->freeState == NULL)) {
			SAlloc::F(state->attrs);
			SAlloc::F(state);
		}
		else {
			xmlRelaxNGAddStatesUniq(ctxt, ctxt->freeState, state);
		}
	}
}

/************************************************************************
*									*
*			Semi internal functions				*
*									*
************************************************************************/

/**
 * xmlRelaxParserSetFlag:
 * @ctxt: a RelaxNG parser context
 * @flags: a set of flags values
 *
 * Semi private function used to pass informations to a parser context
 * which are a combination of xmlRelaxNGParserFlag .
 *
 * Returns 0 if success and -1 in case of error
 */
int xmlRelaxParserSetFlag(xmlRelaxNGParserCtxtPtr ctxt, int flags)
{
	if(!ctxt) 
		return -1;
	if(flags & XML_RELAXNGP_FREE_DOC) {
		ctxt->crng |= XML_RELAXNGP_FREE_DOC;
		flags -= XML_RELAXNGP_FREE_DOC;
	}
	if(flags & XML_RELAXNGP_CRNG) {
		ctxt->crng |= XML_RELAXNGP_CRNG;
		flags -= XML_RELAXNGP_CRNG;
	}
	return (flags != 0) ? -1 : 0;
}

/************************************************************************
*									*
*			Document functions				*
*									*
************************************************************************/
static xmlDoc * xmlRelaxNGCleanupDoc(xmlRelaxNGParserCtxtPtr ctxt, xmlDoc * doc);
/**
 * xmlRelaxNGIncludePush:
 * @ctxt:  the parser context
 * @value:  the element doc
 *
 * Pushes a new include on top of the include stack
 *
 * Returns 0 in case of error, the index in the stack otherwise
 */
static int xmlRelaxNGIncludePush(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGIncludePtr value)
{
	if(ctxt->incTab == NULL) {
		ctxt->incMax = 4;
		ctxt->incNr = 0;
		ctxt->incTab = (xmlRelaxNGIncludePtr*)SAlloc::M(ctxt->incMax * sizeof(ctxt->incTab[0]));
		if(ctxt->incTab == NULL) {
			xmlRngPErrMemory(ctxt, "allocating include\n");
			return 0;
		}
	}
	if(ctxt->incNr >= ctxt->incMax) {
		ctxt->incMax *= 2;
		ctxt->incTab = (xmlRelaxNGIncludePtr*)SAlloc::R(ctxt->incTab, ctxt->incMax * sizeof(ctxt->incTab[0]));
		if(ctxt->incTab == NULL) {
			xmlRngPErrMemory(ctxt, "allocating include\n");
			return 0;
		}
	}
	ctxt->incTab[ctxt->incNr] = value;
	ctxt->inc = value;
	return (ctxt->incNr++);
}
/**
 * xmlRelaxNGIncludePop:
 * @ctxt: the parser context
 *
 * Pops the top include from the include stack
 *
 * Returns the include just removed
 */
static xmlRelaxNGInclude * xmlRelaxNGIncludePop(xmlRelaxNGParserCtxt * ctxt)
{
	xmlRelaxNGInclude * ret = 0;
	if(ctxt->incNr > 0) {
		ctxt->incNr--;
		ctxt->inc = (ctxt->incNr > 0) ? ctxt->incTab[ctxt->incNr - 1] : NULL;
		ret = ctxt->incTab[ctxt->incNr];
		ctxt->incTab[ctxt->incNr] = NULL;
	}
	return ret;
}
/**
 * xmlRelaxNGRemoveRedefine:
 * @ctxt: the parser context
 * @URL:  the normalized URL
 * @target:  the included target
 * @name:  the define name to eliminate
 *
 * Applies the elimination algorithm of 4.7
 *
 * Returns 0 in case of error, 1 in case of success.
 */
static int xmlRelaxNGRemoveRedefine(xmlRelaxNGParserCtxtPtr ctxt, const xmlChar * URL ATTRIBUTE_UNUSED, xmlNode * target, const xmlChar * name)
{
	int found = 0;
	xmlNode * tmp;
	xmlNode * tmp2;
	xmlChar * name2;
#ifdef DEBUG_INCLUDE
	if(!name)
		xmlGenericError(0, "Elimination of <include> start from %s\n", URL);
	else
		xmlGenericError(0, "Elimination of <include> define %s from %s\n", name, URL);
#endif
	tmp = target;
	while(tmp) {
		tmp2 = tmp->next;
		if((name == NULL) && (IS_RELAXNG(tmp, "start"))) {
			found = 1;
			xmlUnlinkNode(tmp);
			xmlFreeNode(tmp);
		}
		else if(name && (IS_RELAXNG(tmp, "define"))) {
			name2 = xmlGetProp(tmp, BAD_CAST "name");
			xmlRelaxNGNormExtSpace(name2);
			if(name2) {
				if(sstreq(name, name2)) {
					found = 1;
					xmlUnlinkNode(tmp);
					xmlFreeNode(tmp);
				}
				SAlloc::F(name2);
			}
		}
		else if(IS_RELAXNG(tmp, "include")) {
			xmlChar * href = NULL;
			xmlRelaxNGDocumentPtr inc = (xmlRelaxNGDocumentPtr)tmp->psvi;
			if(inc && inc->doc && inc->doc->children) {
				if(sstreq(inc->doc->children->name, "grammar")) {
#ifdef DEBUG_INCLUDE
					href = xmlGetProp(tmp, BAD_CAST "href");
#endif
					if(xmlRelaxNGRemoveRedefine(ctxt, href, xmlDocGetRootElement(inc->doc)->children, name) == 1) {
						found = 1;
					}
#ifdef DEBUG_INCLUDE
					SAlloc::F(href);
#endif
				}
			}
		}
		tmp = tmp2;
	}
	return (found);
}

/**
 * xmlRelaxNGLoadInclude:
 * @ctxt: the parser context
 * @URL:  the normalized URL
 * @node: the include node.
 * @ns:  the namespace passed from the context.
 *
 * First lookup if the document is already loaded into the parser context,
 * check against recursion. If not found the resource is loaded and
 * the content is preprocessed before being returned back to the caller.
 *
 * Returns the xmlRelaxNGIncludePtr or NULL in case of error
 */
static xmlRelaxNGIncludePtr xmlRelaxNGLoadInclude(xmlRelaxNGParserCtxtPtr ctxt, const xmlChar * URL, xmlNode * P_Node, const xmlChar * ns)
{
	xmlRelaxNGIncludePtr ret = NULL;
	xmlDoc * doc;
	int i;
	xmlNode * root;
	xmlNode * cur;
#ifdef DEBUG_INCLUDE
	xmlGenericError(0, "xmlRelaxNGLoadInclude(%s)\n", URL);
#endif
	/*
	 * check against recursion in the stack
	 */
	for(i = 0; i < ctxt->incNr; i++) {
		if(sstreq(ctxt->incTab[i]->href, URL)) {
			xmlRngPErr(ctxt, NULL, XML_RNGP_INCLUDE_RECURSE, "Detected an Include recursion for %s\n", URL, 0);
			return 0;
		}
	}
	/*
	 * load the document
	 */
	doc = xmlReadFile((const char*)URL, NULL, 0);
	if(!doc) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_PARSE_ERROR, "xmlRelaxNG: could not load %s\n", URL, 0);
		return 0;
	}
#ifdef DEBUG_INCLUDE
	xmlGenericError(0, "Parsed %s Okay\n", URL);
#endif
	/*
	 * Allocate the document structures and register it first.
	 */
	ret = (xmlRelaxNGIncludePtr)SAlloc::M(sizeof(xmlRelaxNGInclude));
	if(!ret) {
		xmlRngPErrMemory(ctxt, "allocating include\n");
		xmlFreeDoc(doc);
		return 0;
	}
	memzero(ret, sizeof(xmlRelaxNGInclude));
	ret->doc = doc;
	ret->href = sstrdup(URL);
	ret->next = ctxt->includes;
	ctxt->includes = ret;
	/*
	 * transmit the ns if needed
	 */
	if(ns) {
		root = xmlDocGetRootElement(doc);
		if(root) {
			if(xmlHasProp(root, BAD_CAST "ns") == NULL) {
				xmlSetProp(root, BAD_CAST "ns", ns);
			}
		}
	}
	/*
	 * push it on the stack
	 */
	xmlRelaxNGIncludePush(ctxt, ret);

	/*
	 * Some preprocessing of the document content, this include recursing
	 * in the include stack.
	 */
#ifdef DEBUG_INCLUDE
	xmlGenericError(0, "cleanup of %s\n", URL);
#endif
	doc = xmlRelaxNGCleanupDoc(ctxt, doc);
	if(!doc) {
		ctxt->inc = NULL;
		return 0;
	}
	/*
	 * Pop up the include from the stack
	 */
	xmlRelaxNGIncludePop(ctxt);
#ifdef DEBUG_INCLUDE
	xmlGenericError(0, "Checking of %s\n", URL);
#endif
	/*
	 * Check that the top element is a grammar
	 */
	root = xmlDocGetRootElement(doc);
	if(root == NULL) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_EMPTY, "xmlRelaxNG: included document is empty %s\n", URL, 0);
		return 0;
	}
	if(!IS_RELAXNG(root, "grammar")) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_GRAMMAR_MISSING, "xmlRelaxNG: included document %s root is not a grammar\n", URL, 0);
		return 0;
	}
	/*
	 * Elimination of redefined rules in the include.
	 */
	cur = P_Node->children;
	while(cur) {
		if(IS_RELAXNG(cur, "start")) {
			int found = xmlRelaxNGRemoveRedefine(ctxt, URL, root->children, 0);
			if(!found) {
				xmlRngPErr(ctxt, P_Node, XML_RNGP_START_MISSING, "xmlRelaxNG: include %s has a start but not the included grammar\n", URL, 0);
			}
		}
		else if(IS_RELAXNG(cur, "define")) {
			xmlChar * name = xmlGetProp(cur, BAD_CAST "name");
			if(!name) {
				xmlRngPErr(ctxt, P_Node, XML_RNGP_NAME_MISSING, "xmlRelaxNG: include %s has define without name\n", URL, 0);
			}
			else {
				int found;
				xmlRelaxNGNormExtSpace(name);
				found = xmlRelaxNGRemoveRedefine(ctxt, URL, root->children, name);
				if(!found) {
					xmlRngPErr(ctxt, P_Node, XML_RNGP_DEFINE_MISSING, "xmlRelaxNG: include %s has a define %s but not the included grammar\n", URL, name);
				}
				SAlloc::F(name);
			}
		}
		cur = cur->next;
	}
	return ret;
}
/**
 * xmlRelaxNGValidErrorPush:
 * @ctxt:  the validation context
 * @err:  the error code
 * @arg1:  the first string argument
 * @arg2:  the second string argument
 * @dup:  arg need to be duplicated
 *
 * Pushes a new error on top of the error stack
 *
 * Returns 0 in case of error, the index in the stack otherwise
 */
static int xmlRelaxNGValidErrorPush(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGValidErr err, const xmlChar * arg1, const xmlChar * arg2, int dup)
{
	xmlRelaxNGValidErrorPtr cur;
#ifdef DEBUG_ERROR
	xmlGenericError(0, "Pushing error %d at %d on stack\n", err, ctxt->errNr);
#endif
	if(ctxt->errTab == NULL) {
		ctxt->errMax = 8;
		ctxt->errNr = 0;
		ctxt->errTab = (xmlRelaxNGValidErrorPtr)SAlloc::M(ctxt->errMax * sizeof(xmlRelaxNGValidError));
		if(ctxt->errTab == NULL) {
			xmlRngVErrMemory(ctxt, "pushing error\n");
			return 0;
		}
		ctxt->err = NULL;
	}
	if(ctxt->errNr >= ctxt->errMax) {
		ctxt->errMax *= 2;
		ctxt->errTab = (xmlRelaxNGValidErrorPtr)SAlloc::R(ctxt->errTab, ctxt->errMax * sizeof(xmlRelaxNGValidError));
		if(ctxt->errTab == NULL) {
			xmlRngVErrMemory(ctxt, "pushing error\n");
			return 0;
		}
		ctxt->err = &ctxt->errTab[ctxt->errNr - 1];
	}
	if(ctxt->err && ctxt->state && (ctxt->err->P_Node == ctxt->state->P_Node) && (ctxt->err->err == err))
		return (ctxt->errNr);
	cur = &ctxt->errTab[ctxt->errNr];
	cur->err = err;
	if(dup) {
		cur->arg1 = sstrdup(arg1);
		cur->arg2 = sstrdup(arg2);
		cur->flags = ERROR_IS_DUP;
	}
	else {
		cur->arg1 = arg1;
		cur->arg2 = arg2;
		cur->flags = 0;
	}
	if(ctxt->state) {
		cur->P_Node = ctxt->state->P_Node;
		cur->seq = ctxt->state->seq;
	}
	else {
		cur->P_Node = NULL;
		cur->seq = NULL;
	}
	ctxt->err = cur;
	return (ctxt->errNr++);
}
/**
 * xmlRelaxNGValidErrorPop:
 * @ctxt: the validation context
 *
 * Pops the top error from the error stack
 */
static void xmlRelaxNGValidErrorPop(xmlRelaxNGValidCtxtPtr ctxt)
{
	xmlRelaxNGValidErrorPtr cur;
	if(ctxt->errNr <= 0) {
		ctxt->err = NULL;
	}
	else {
		ctxt->errNr--;
		ctxt->err = (ctxt->errNr > 0) ? &ctxt->errTab[ctxt->errNr-1] : 0;
		cur = &ctxt->errTab[ctxt->errNr];
		if(cur->flags & ERROR_IS_DUP) {
			SAlloc::F((xmlChar*)cur->arg1);
			cur->arg1 = NULL;
			SAlloc::F((xmlChar*)cur->arg2);
			cur->arg2 = NULL;
			cur->flags = 0;
		}
	}
}

/**
 * xmlRelaxNGDocumentPush:
 * @ctxt:  the parser context
 * @value:  the element doc
 *
 * Pushes a new doc on top of the doc stack
 *
 * Returns 0 in case of error, the index in the stack otherwise
 */
static int xmlRelaxNGDocumentPush(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGDocumentPtr value)
{
	if(ctxt->docTab == NULL) {
		ctxt->docMax = 4;
		ctxt->docNr = 0;
		ctxt->docTab = (xmlRelaxNGDocumentPtr*)SAlloc::M(ctxt->docMax * sizeof(ctxt->docTab[0]));
		if(ctxt->docTab == NULL) {
			xmlRngPErrMemory(ctxt, "adding document\n");
			return 0;
		}
	}
	if(ctxt->docNr >= ctxt->docMax) {
		ctxt->docMax *= 2;
		ctxt->docTab = (xmlRelaxNGDocumentPtr*)SAlloc::R(ctxt->docTab, ctxt->docMax * sizeof(ctxt->docTab[0]));
		if(ctxt->docTab == NULL) {
			xmlRngPErrMemory(ctxt, "adding document\n");
			return 0;
		}
	}
	ctxt->docTab[ctxt->docNr] = value;
	ctxt->doc = value;
	return (ctxt->docNr++);
}
/**
 * xmlRelaxNGDocumentPop:
 * @ctxt: the parser context
 *
 * Pops the top doc from the doc stack
 *
 * Returns the doc just removed
 */
static xmlRelaxNGDocument * xmlRelaxNGDocumentPop(xmlRelaxNGParserCtxt * ctxt)
{
	xmlRelaxNGDocument * ret = 0;
	if(ctxt->docNr > 0) {
		ctxt->docNr--;
		ctxt->doc = (ctxt->docNr > 0) ? ctxt->docTab[ctxt->docNr - 1] : NULL;
		ret = ctxt->docTab[ctxt->docNr];
		ctxt->docTab[ctxt->docNr] = NULL;
	}
	return ret;
}
/**
 * xmlRelaxNGLoadExternalRef:
 * @ctxt: the parser context
 * @URL:  the normalized URL
 * @ns:  the inherited ns if any
 *
 * First lookup if the document is already loaded into the parser context,
 * check against recursion. If not found the resource is loaded and
 * the content is preprocessed before being returned back to the caller.
 *
 * Returns the xmlRelaxNGDocumentPtr or NULL in case of error
 */
static xmlRelaxNGDocumentPtr xmlRelaxNGLoadExternalRef(xmlRelaxNGParserCtxtPtr ctxt, const xmlChar * URL, const xmlChar * ns)
{
	xmlRelaxNGDocumentPtr ret = NULL;
	xmlDoc * doc;
	xmlNode * root;
	int i;
	/*
	 * check against recursion in the stack
	 */
	for(i = 0; i < ctxt->docNr; i++) {
		if(sstreq(ctxt->docTab[i]->href, URL)) {
			xmlRngPErr(ctxt, NULL, XML_RNGP_EXTERNALREF_RECURSE, "Detected an externalRef recursion for %s\n", URL, 0);
			return 0;
		}
	}
	/*
	 * load the document
	 */
	doc = xmlReadFile((const char*)URL, NULL, 0);
	if(!doc) {
		xmlRngPErr(ctxt, NULL, XML_RNGP_PARSE_ERROR, "xmlRelaxNG: could not load %s\n", URL, 0);
		return 0;
	}
	/*
	 * Allocate the document structures and register it first.
	 */
	ret = (xmlRelaxNGDocumentPtr)SAlloc::M(sizeof(xmlRelaxNGDocument));
	if(!ret) {
		xmlRngPErr(ctxt, (xmlNode *)doc, XML_ERR_NO_MEMORY, "xmlRelaxNG: allocate memory for doc %s\n", URL, 0);
		xmlFreeDoc(doc);
		return 0;
	}
	memzero(ret, sizeof(xmlRelaxNGDocument));
	ret->doc = doc;
	ret->href = sstrdup(URL);
	ret->next = ctxt->documents;
	ret->externalRef = 1;
	ctxt->documents = ret;
	/*
	 * transmit the ns if needed
	 */
	if(ns) {
		root = xmlDocGetRootElement(doc);
		if(root) {
			if(xmlHasProp(root, BAD_CAST "ns") == NULL) {
				xmlSetProp(root, BAD_CAST "ns", ns);
			}
		}
	}
	/*
	 * push it on the stack and register it in the hash table
	 */
	xmlRelaxNGDocumentPush(ctxt, ret);
	/*
	 * Some preprocessing of the document content
	 */
	doc = xmlRelaxNGCleanupDoc(ctxt, doc);
	if(!doc) {
		ctxt->doc = NULL;
		return 0;
	}
	xmlRelaxNGDocumentPop(ctxt);
	return ret;
}

/************************************************************************
*									*
*			Error functions					*
*									*
************************************************************************/

#define VALID_ERR(a) xmlRelaxNGAddValidError(ctxt, a, NULL, NULL, 0);
#define VALID_ERR2(a, b) xmlRelaxNGAddValidError(ctxt, a, b, NULL, 0);
#define VALID_ERR3(a, b, c) xmlRelaxNGAddValidError(ctxt, a, b, c, 0);
#define VALID_ERR2P(a, b) xmlRelaxNGAddValidError(ctxt, a, b, NULL, 1);
#define VALID_ERR3P(a, b, c) xmlRelaxNGAddValidError(ctxt, a, b, c, 1);

static const char * FASTCALL xmlRelaxNGDefName(xmlRelaxNGDefine * def)
{
	if(def == NULL)
		return ("none");
	switch(def->type) {
		case XML_RELAXNG_EMPTY: return ("empty");
		case XML_RELAXNG_NOT_ALLOWED: return ("notAllowed");
		case XML_RELAXNG_EXCEPT: return ("except");
		case XML_RELAXNG_TEXT: return ("text");
		case XML_RELAXNG_ELEMENT: return ("element");
		case XML_RELAXNG_DATATYPE: return ("datatype");
		case XML_RELAXNG_VALUE: return ("value");
		case XML_RELAXNG_LIST: return ("list");
		case XML_RELAXNG_ATTRIBUTE: return ("attribute");
		case XML_RELAXNG_DEF: return ("def");
		case XML_RELAXNG_REF: return ("ref");
		case XML_RELAXNG_EXTERNALREF: return ("externalRef");
		case XML_RELAXNG_PARENTREF: return ("parentRef");
		case XML_RELAXNG_OPTIONAL: return ("optional");
		case XML_RELAXNG_ZEROORMORE: return ("zeroOrMore");
		case XML_RELAXNG_ONEORMORE: return ("oneOrMore");
		case XML_RELAXNG_CHOICE: return ("choice");
		case XML_RELAXNG_GROUP: return ("group");
		case XML_RELAXNG_INTERLEAVE: return ("interleave");
		case XML_RELAXNG_START: return ("start");
		case XML_RELAXNG_NOOP: return ("noop");
		case XML_RELAXNG_PARAM: return ("param");
	}
	return ("unknown");
}
/**
 * xmlRelaxNGGetErrorString:
 * @err:  the error code
 * @arg1:  the first string argument
 * @arg2:  the second string argument
 *
 * computes a formatted error string for the given error code and args
 *
 * Returns the error string, it must be deallocated by the caller
 */
static xmlChar * xmlRelaxNGGetErrorString(xmlRelaxNGValidErr err, const xmlChar * arg1, const xmlChar * arg2)
{
	char msg[1000];
	SETIFZ(arg1, BAD_CAST "");
	SETIFZ(arg2, BAD_CAST "");
	msg[0] = 0;
	switch(err) {
		case XML_RELAXNG_OK: return 0;
		case XML_RELAXNG_ERR_MEMORY: return (xmlCharStrdup("out of memory\n"));
		case XML_RELAXNG_ERR_TYPE: snprintf(msg, 1000, "failed to validate type %s\n", arg1); break;
		case XML_RELAXNG_ERR_TYPEVAL: snprintf(msg, 1000, "Type %s doesn't allow value '%s'\n", arg1, arg2); break;
		case XML_RELAXNG_ERR_DUPID: snprintf(msg, 1000, "ID %s redefined\n", arg1); break;
		case XML_RELAXNG_ERR_TYPECMP: snprintf(msg, 1000, "failed to compare type %s\n", arg1); break;
		case XML_RELAXNG_ERR_NOSTATE: return (xmlCharStrdup("Internal error: no state\n")); 
		case XML_RELAXNG_ERR_NODEFINE: return (xmlCharStrdup("Internal error: no define\n"));
		case XML_RELAXNG_ERR_INTERNAL: snprintf(msg, 1000, "Internal error: %s\n", arg1); break;
		case XML_RELAXNG_ERR_LISTEXTRA: snprintf(msg, 1000, "Extra data in list: %s\n", arg1); break;
		case XML_RELAXNG_ERR_INTERNODATA: return (xmlCharStrdup("Internal: interleave block has no data\n"));
		case XML_RELAXNG_ERR_INTERSEQ: return (xmlCharStrdup("Invalid sequence in interleave\n"));
		case XML_RELAXNG_ERR_INTEREXTRA: snprintf(msg, 1000, "Extra element %s in interleave\n", arg1); break;
		case XML_RELAXNG_ERR_ELEMNAME: snprintf(msg, 1000, "Expecting element %s, got %s\n", arg1, arg2); break;
		case XML_RELAXNG_ERR_ELEMNONS: snprintf(msg, 1000, "Expecting a namespace for element %s\n", arg1); break;
		case XML_RELAXNG_ERR_ELEMWRONGNS: snprintf(msg, 1000, "Element %s has wrong namespace: expecting %s\n", arg1, arg2); break;
		case XML_RELAXNG_ERR_ELEMWRONG: snprintf(msg, 1000, "Did not expect element %s there\n", arg1); break;
		case XML_RELAXNG_ERR_TEXTWRONG: snprintf(msg, 1000, "Did not expect text in element %s content\n", arg1); break;
		case XML_RELAXNG_ERR_ELEMEXTRANS: snprintf(msg, 1000, "Expecting no namespace for element %s\n", arg1); break;
		case XML_RELAXNG_ERR_ELEMNOTEMPTY: snprintf(msg, 1000, "Expecting element %s to be empty\n", arg1); break;
		case XML_RELAXNG_ERR_NOELEM: snprintf(msg, 1000, "Expecting an element %s, got nothing\n", arg1); break;
		case XML_RELAXNG_ERR_NOTELEM: return (xmlCharStrdup("Expecting an element got text\n")); 
		case XML_RELAXNG_ERR_ATTRVALID: snprintf(msg, 1000, "Element %s failed to validate attributes\n", arg1); break;
		case XML_RELAXNG_ERR_CONTENTVALID: snprintf(msg, 1000, "Element %s failed to validate content\n", arg1); break;
		case XML_RELAXNG_ERR_EXTRACONTENT: snprintf(msg, 1000, "Element %s has extra content: %s\n", arg1, arg2); break;
		case XML_RELAXNG_ERR_INVALIDATTR: snprintf(msg, 1000, "Invalid attribute %s for element %s\n", arg1, arg2); break;
		case XML_RELAXNG_ERR_LACKDATA: snprintf(msg, 1000, "Datatype element %s contains no data\n", arg1); break;
		case XML_RELAXNG_ERR_DATAELEM: snprintf(msg, 1000, "Datatype element %s has child elements\n", arg1); break;
		case XML_RELAXNG_ERR_VALELEM: snprintf(msg, 1000, "Value element %s has child elements\n", arg1); break;
		case XML_RELAXNG_ERR_LISTELEM: snprintf(msg, 1000, "List element %s has child elements\n", arg1); break;
		case XML_RELAXNG_ERR_DATATYPE: snprintf(msg, 1000, "Error validating datatype %s\n", arg1); break;
		case XML_RELAXNG_ERR_VALUE: snprintf(msg, 1000, "Error validating value %s\n", arg1); break;
		case XML_RELAXNG_ERR_LIST: return (xmlCharStrdup("Error validating list\n"));
		case XML_RELAXNG_ERR_NOGRAMMAR: return (xmlCharStrdup("No top grammar defined\n"));
		case XML_RELAXNG_ERR_EXTRADATA: return (xmlCharStrdup("Extra data in the document\n"));
		default: return (xmlCharStrdup("Unknown error !\n"));
	}
	if(msg[0] == 0) {
		snprintf(msg, 1000, "Unknown error code %d\n", err);
	}
	msg[1000 - 1] = 0;
	return sstrdup((xmlChar*)msg);
}
/**
 * xmlRelaxNGShowValidError:
 * @ctxt:  the validation context
 * @err:  the error number
 * @node:  the node
 * @child:  the node child generating the problem.
 * @arg1:  the first argument
 * @arg2:  the second argument
 *
 * Show a validation error.
 */
static void xmlRelaxNGShowValidError(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGValidErr err, xmlNode * P_Node, xmlNode * child, const xmlChar * arg1, const xmlChar * arg2)
{
	xmlChar * msg;
	if(ctxt->flags & FLAGS_NOERROR)
		return;
#ifdef DEBUG_ERROR
	xmlGenericError(0, "Show error %d\n", err);
#endif
	msg = xmlRelaxNGGetErrorString(err, arg1, arg2);
	if(msg) {
		if(ctxt->errNo == XML_RELAXNG_OK)
			ctxt->errNo = err;
		xmlRngVErr(ctxt, (child == NULL ? P_Node : child), err, (const char*)msg, arg1, arg2);
		SAlloc::F(msg);
	}
}
/**
 * xmlRelaxNGPopErrors:
 * @ctxt:  the validation context
 * @level:  the error level in the stack
 *
 * pop and discard all errors until the given level is reached
 */
static void FASTCALL xmlRelaxNGPopErrors(xmlRelaxNGValidCtxt * ctxt, int level)
{
	int i;
	xmlRelaxNGValidError * err;
#ifdef DEBUG_ERROR
	xmlGenericError(0, "Pop errors till level %d\n", level);
#endif
	for(i = level; i < ctxt->errNr; i++) {
		err = &ctxt->errTab[i];
		if(err->flags & ERROR_IS_DUP) {
			SAlloc::F((xmlChar*)err->arg1);
			err->arg1 = NULL;
			SAlloc::F((xmlChar*)err->arg2);
			err->arg2 = NULL;
			err->flags = 0;
		}
	}
	ctxt->errNr = level;
	if(ctxt->errNr <= 0)
		ctxt->err = NULL;
}
/**
 * xmlRelaxNGDumpValidError:
 * @ctxt:  the validation context
 *
 * Show all validation error over a given index.
 */
static void FASTCALL xmlRelaxNGDumpValidError(xmlRelaxNGValidCtxt * ctxt)
{
	int i, j, k;
	xmlRelaxNGValidError * err;
#ifdef DEBUG_ERROR
	xmlGenericError(0, "Dumping error stack %d errors\n", ctxt->errNr);
#endif
	for(i = 0, k = 0; i < ctxt->errNr; i++) {
		err = &ctxt->errTab[i];
		if(k < MAX_ERROR) {
			for(j = 0; j < i; j++) {
				xmlRelaxNGValidError * dup = &ctxt->errTab[j];
				if((err->err == dup->err) && (err->P_Node == dup->P_Node) && sstreq(err->arg1, dup->arg1) && sstreq(err->arg2, dup->arg2)) {
					goto skip;
				}
			}
			xmlRelaxNGShowValidError(ctxt, err->err, err->P_Node, err->seq, err->arg1, err->arg2);
			k++;
		}
skip:
		if(err->flags & ERROR_IS_DUP) {
			SAlloc::F((xmlChar*)err->arg1);
			err->arg1 = NULL;
			SAlloc::F((xmlChar*)err->arg2);
			err->arg2 = NULL;
			err->flags = 0;
		}
	}
	ctxt->errNr = 0;
}

/**
 * xmlRelaxNGAddValidError:
 * @ctxt:  the validation context
 * @err:  the error number
 * @arg1:  the first argument
 * @arg2:  the second argument
 * @dup:  need to dup the args
 *
 * Register a validation error, either generating it if it's sure
 * or stacking it for later handling if unsure.
 */
static void xmlRelaxNGAddValidError(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGValidErr err, const xmlChar * arg1, const xmlChar * arg2, int dup)
{
	if(!ctxt)
		return;
	if(ctxt->flags & FLAGS_NOERROR)
		return;
#ifdef DEBUG_ERROR
	xmlGenericError(0, "Adding error %d\n", err);
#endif
	/*
	 * generate the error directly
	 */
	if(((ctxt->flags & FLAGS_IGNORABLE) == 0) || (ctxt->flags & FLAGS_NEGATIVE)) {
		xmlNode * P_Node;
		xmlNode * seq;
		/*
		 * Flush first any stacked error which might be the
		 * real cause of the problem.
		 */
		if(ctxt->errNr != 0)
			xmlRelaxNGDumpValidError(ctxt);
		if(ctxt->state) {
			P_Node = ctxt->state->P_Node;
			seq = ctxt->state->seq;
		}
		else {
			P_Node = seq = NULL;
		}
		if(!P_Node && !seq) {
			P_Node = ctxt->pnode;
		}
		xmlRelaxNGShowValidError(ctxt, err, P_Node, seq, arg1, arg2);
	}
	/*
	 * Stack the error for later processing if needed
	 */
	else {
		xmlRelaxNGValidErrorPush(ctxt, err, arg1, arg2, dup);
	}
}

/************************************************************************
*									*
*			Type library hooks				*
*									*
************************************************************************/
static xmlChar * xmlRelaxNGNormalize(xmlRelaxNGValidCtxtPtr ctxt,
    const xmlChar * str);

/**
 * xmlRelaxNGSchemaTypeHave:
 * @data:  data needed for the library
 * @type:  the type name
 *
 * Check if the given type is provided by
 * the W3C XMLSchema Datatype library.
 *
 * Returns 1 if yes, 0 if no and -1 in case of error.
 */
static int xmlRelaxNGSchemaTypeHave(void * data ATTRIBUTE_UNUSED, const xmlChar * type)
{
	if(type == NULL)
		return -1;
	else {
		xmlSchemaType * typ = xmlSchemaGetPredefinedType(type, BAD_CAST "http://www.w3.org/2001/XMLSchema");
		return (typ == NULL) ? 0 : 1;
	}
}
/**
 * xmlRelaxNGSchemaTypeCheck:
 * @data:  data needed for the library
 * @type:  the type name
 * @value:  the value to check
 * @node:  the node
 *
 * Check if the given type and value are validated by
 * the W3C XMLSchema Datatype library.
 *
 * Returns 1 if yes, 0 if no and -1 in case of error.
 */
static int xmlRelaxNGSchemaTypeCheck(void * data ATTRIBUTE_UNUSED, const xmlChar * type, const xmlChar * value, void ** result, xmlNode * P_Node)
{
	xmlSchemaTypePtr typ;
	int ret;
	if((type == NULL) || (value == NULL))
		return -1;
	typ = xmlSchemaGetPredefinedType(type, BAD_CAST "http://www.w3.org/2001/XMLSchema");
	if(typ == NULL)
		return -1;
	ret = xmlSchemaValPredefTypeNode(typ, value, (xmlSchemaValPtr*)result, P_Node);
	if(ret == 2)            /* special ID error code */
		return (2);
	if(ret == 0)
		return 1;
	if(ret > 0)
		return 0;
	return -1;
}

/**
 * xmlRelaxNGSchemaFacetCheck:
 * @data:  data needed for the library
 * @type:  the type name
 * @facet:  the facet name
 * @val:  the facet value
 * @strval:  the string value
 * @value:  the value to check
 *
 * Function provided by a type library to check a value facet
 *
 * Returns 1 if yes, 0 if no and -1 in case of error.
 */
static int xmlRelaxNGSchemaFacetCheck(void * data ATTRIBUTE_UNUSED,
    const xmlChar * type, const xmlChar * facetname, const xmlChar * val, const xmlChar * strval, void * value)
{
	xmlSchemaFacetPtr facet;
	xmlSchemaTypePtr typ;
	int ret;
	if((type == NULL) || (strval == NULL))
		return -1;
	typ = xmlSchemaGetPredefinedType(type, BAD_CAST "http://www.w3.org/2001/XMLSchema");
	if(typ == NULL)
		return -1;
	facet = xmlSchemaNewFacet();
	if(facet == NULL)
		return -1;
	if(sstreq(facetname, "minInclusive")) {
		facet->type = XML_SCHEMA_FACET_MININCLUSIVE;
	}
	else if(sstreq(facetname, "minExclusive")) {
		facet->type = XML_SCHEMA_FACET_MINEXCLUSIVE;
	}
	else if(sstreq(facetname, "maxInclusive")) {
		facet->type = XML_SCHEMA_FACET_MAXINCLUSIVE;
	}
	else if(sstreq(facetname, "maxExclusive")) {
		facet->type = XML_SCHEMA_FACET_MAXEXCLUSIVE;
	}
	else if(sstreq(facetname, "totalDigits")) {
		facet->type = XML_SCHEMA_FACET_TOTALDIGITS;
	}
	else if(sstreq(facetname, "fractionDigits")) {
		facet->type = XML_SCHEMA_FACET_FRACTIONDIGITS;
	}
	else if(sstreq(facetname, "pattern")) {
		facet->type = XML_SCHEMA_FACET_PATTERN;
	}
	else if(sstreq(facetname, "enumeration")) {
		facet->type = XML_SCHEMA_FACET_ENUMERATION;
	}
	else if(sstreq(facetname, "whiteSpace")) {
		facet->type = XML_SCHEMA_FACET_WHITESPACE;
	}
	else if(sstreq(facetname, "length")) {
		facet->type = XML_SCHEMA_FACET_LENGTH;
	}
	else if(sstreq(facetname, "maxLength")) {
		facet->type = XML_SCHEMA_FACET_MAXLENGTH;
	}
	else if(sstreq(facetname, "minLength")) {
		facet->type = XML_SCHEMA_FACET_MINLENGTH;
	}
	else {
		xmlSchemaFreeFacet(facet);
		return -1;
	}
	facet->value = val;
	ret = xmlSchemaCheckFacet(facet, typ, NULL, type);
	if(ret != 0) {
		xmlSchemaFreeFacet(facet);
		return -1;
	}
	ret = xmlSchemaValidateFacet(typ, facet, strval, (xmlSchemaValPtr)value);
	xmlSchemaFreeFacet(facet);
	if(ret != 0)
		return -1;
	return 0;
}
/**
 * xmlRelaxNGSchemaFreeValue:
 * @data:  data needed for the library
 * @value:  the value to free
 *
 * Function provided by a type library to free a Schemas value
 *
 * Returns 1 if yes, 0 if no and -1 in case of error.
 */
static void xmlRelaxNGSchemaFreeValue(void * data ATTRIBUTE_UNUSED, void * value)
{
	xmlSchemaFreeValue((xmlSchemaValPtr)value);
}

/**
 * xmlRelaxNGSchemaTypeCompare:
 * @data:  data needed for the library
 * @type:  the type name
 * @value1:  the first value
 * @value2:  the second value
 *
 * Compare two values for equality accordingly a type from the W3C XMLSchema
 * Datatype library.
 *
 * Returns 1 if equal, 0 if no and -1 in case of error.
 */
static int xmlRelaxNGSchemaTypeCompare(void * data ATTRIBUTE_UNUSED, const xmlChar * type, const xmlChar * value1,
    xmlNode * ctxt1, void * comp1, const xmlChar * value2, xmlNode * ctxt2)
{
	int ret;
	xmlSchemaTypePtr typ;
	xmlSchemaValPtr res1 = NULL, res2 = NULL;
	if((type == NULL) || (value1 == NULL) || (value2 == NULL))
		return -1;
	typ = xmlSchemaGetPredefinedType(type, BAD_CAST "http://www.w3.org/2001/XMLSchema");
	if(typ == NULL)
		return -1;
	if(comp1 == NULL) {
		ret = xmlSchemaValPredefTypeNode(typ, value1, &res1, ctxt1);
		if(ret != 0)
			return -1;
		if(res1 == NULL)
			return -1;
	}
	else {
		res1 = (xmlSchemaValPtr)comp1;
	}
	ret = xmlSchemaValPredefTypeNode(typ, value2, &res2, ctxt2);
	if(ret != 0) {
		if(res1 != (xmlSchemaValPtr)comp1)
			xmlSchemaFreeValue(res1);
		return -1;
	}
	ret = xmlSchemaCompareValues(res1, res2);
	if(res1 != (xmlSchemaValPtr)comp1)
		xmlSchemaFreeValue(res1);
	xmlSchemaFreeValue(res2);
	return (ret == -2) ? -1 : ((ret == 0) ? 1 : 0);
}
/**
 * xmlRelaxNGDefaultTypeHave:
 * @data:  data needed for the library
 * @type:  the type name
 *
 * Check if the given type is provided by
 * the default datatype library.
 *
 * Returns 1 if yes, 0 if no and -1 in case of error.
 */
static int xmlRelaxNGDefaultTypeHave(void * data ATTRIBUTE_UNUSED, const xmlChar * type)
{
	if(type == NULL)
		return -1;
	else if(sstreq(type, "string"))
		return 1;
	else if(sstreq(type, "token"))
		return 1;
	else
		return 0;
}
/**
 * xmlRelaxNGDefaultTypeCheck:
 * @data:  data needed for the library
 * @type:  the type name
 * @value:  the value to check
 * @node:  the node
 *
 * Check if the given type and value are validated by
 * the default datatype library.
 *
 * Returns 1 if yes, 0 if no and -1 in case of error.
 */
static int xmlRelaxNGDefaultTypeCheck(void * data ATTRIBUTE_UNUSED, const xmlChar * type ATTRIBUTE_UNUSED,
    const xmlChar * value ATTRIBUTE_UNUSED, void ** result ATTRIBUTE_UNUSED, xmlNode * P_Node ATTRIBUTE_UNUSED)
{
	if(!value)
		return -1;
	else if(sstreq(type, "string"))
		return 1;
	else if(sstreq(type, "token"))
		return 1;
	else
		return 0;
}

/**
 * xmlRelaxNGDefaultTypeCompare:
 * @data:  data needed for the library
 * @type:  the type name
 * @value1:  the first value
 * @value2:  the second value
 *
 * Compare two values accordingly a type from the default
 * datatype library.
 *
 * Returns 1 if yes, 0 if no and -1 in case of error.
 */
static int xmlRelaxNGDefaultTypeCompare(void * data ATTRIBUTE_UNUSED, const xmlChar * type,
    const xmlChar * value1, xmlNode * ctxt1 ATTRIBUTE_UNUSED, void * comp1 ATTRIBUTE_UNUSED, const xmlChar * value2, xmlNode * ctxt2 ATTRIBUTE_UNUSED)
{
	int ret = -1;
	if(sstreq(type, "string")) {
		ret = sstreq(value1, value2);
	}
	else if(sstreq(type, "token")) {
		if(!sstreq(value1, value2)) {
			/*
			 * @todo trivial optimizations are possible by computing at compile-time
			 */
			xmlChar * nval = xmlRelaxNGNormalize(NULL, value1);
			xmlChar * nvalue = xmlRelaxNGNormalize(NULL, value2);
			if((nval == NULL) || (nvalue == NULL))
				ret = -1;
			else if(sstreq(nval, nvalue))
				ret = 1;
			else
				ret = 0;
			SAlloc::F(nval);
			SAlloc::F(nvalue);
		}
		else
			ret = 1;
	}
	return ret;
}

static int xmlRelaxNGTypeInitialized = 0;
static xmlHashTable * xmlRelaxNGRegisteredTypes = NULL;
/**
 * xmlRelaxNGFreeTypeLibrary:
 * @lib:  the type library structure
 * @namespace:  the URI bound to the library
 *
 * Free the structure associated to the type library
 */
static void xmlRelaxNGFreeTypeLibrary(xmlRelaxNGTypeLibraryPtr lib, const xmlChar * pNamespace ATTRIBUTE_UNUSED)
{
	if(lib) {
		SAlloc::F((xmlChar *)lib->P_Namespace);
		SAlloc::F(lib);
	}
}
/**
 * xmlRelaxNGRegisterTypeLibrary:
 * @namespace:  the URI bound to the library
 * @data:  data associated to the library
 * @have:  the provide function
 * @check:  the checking function
 * @comp:  the comparison function
 *
 * Register a new type library
 *
 * Returns 0 in case of success and -1 in case of error.
 */
static int xmlRelaxNGRegisterTypeLibrary(const xmlChar * pNamespace, void * data, xmlRelaxNGTypeHave have,
    xmlRelaxNGTypeCheck check, xmlRelaxNGTypeCompare comp, xmlRelaxNGFacetCheck facet, xmlRelaxNGTypeFree freef)
{
	xmlRelaxNGTypeLibraryPtr lib;
	int ret;
	if(!xmlRelaxNGRegisteredTypes || !pNamespace || !check || !comp)
		return -1;
	if(xmlHashLookup(xmlRelaxNGRegisteredTypes, pNamespace)) {
		xmlGenericError(0, "Relax-NG types library '%s' already registered\n", pNamespace);
		return -1;
	}
	lib = (xmlRelaxNGTypeLibraryPtr)SAlloc::M(sizeof(xmlRelaxNGTypeLibrary));
	if(lib == NULL) {
		xmlRngVErrMemory(NULL, "adding types library\n");
		return -1;
	}
	memzero(lib, sizeof(xmlRelaxNGTypeLibrary));
	lib->P_Namespace = sstrdup(pNamespace);
	lib->data = data;
	lib->have = have;
	lib->comp = comp;
	lib->check = check;
	lib->facet = facet;
	lib->freef = freef;
	ret = xmlHashAddEntry(xmlRelaxNGRegisteredTypes, pNamespace, lib);
	if(ret < 0) {
		xmlGenericError(0, "Relax-NG types library failed to register '%s'\n", pNamespace);
		xmlRelaxNGFreeTypeLibrary(lib, pNamespace);
		return -1;
	}
	return 0;
}
/**
 * xmlRelaxNGInitTypes:
 *
 * Initilize the default type libraries.
 *
 * Returns 0 in case of success and -1 in case of error.
 */
int xmlRelaxNGInitTypes()
{
	if(xmlRelaxNGTypeInitialized) {
		xmlRelaxNGRegisteredTypes = xmlHashCreate(10);
		if(xmlRelaxNGRegisteredTypes == NULL) {
			xmlGenericError(0, "Failed to allocate sh table for Relax-NG types\n");
			return -1;
		}
		xmlRelaxNGRegisterTypeLibrary(BAD_CAST "http://www.w3.org/2001/XMLSchema-datatypes", NULL, xmlRelaxNGSchemaTypeHave, 
			xmlRelaxNGSchemaTypeCheck, xmlRelaxNGSchemaTypeCompare, xmlRelaxNGSchemaFacetCheck, xmlRelaxNGSchemaFreeValue);
		xmlRelaxNGRegisterTypeLibrary(xmlRelaxNGNs, NULL, xmlRelaxNGDefaultTypeHave, xmlRelaxNGDefaultTypeCheck,
			xmlRelaxNGDefaultTypeCompare, 0, 0);
		xmlRelaxNGTypeInitialized = 1;
	}
	return 0;
}

/**
 * xmlRelaxNGCleanupTypes:
 *
 * Cleanup the default Schemas type library associated to RelaxNG
 */
void xmlRelaxNGCleanupTypes()
{
	xmlSchemaCleanupTypes();
	if(xmlRelaxNGTypeInitialized) {
		xmlHashFree(xmlRelaxNGRegisteredTypes, (xmlHashDeallocator)xmlRelaxNGFreeTypeLibrary);
		xmlRelaxNGTypeInitialized = 0;
	}
}
/************************************************************************
*									*
*		Compiling element content into regexp			*
*									*
* Sometime the element content can be compiled into a pure regexp,	*
* This allows a faster execution and streamability at that level	*
*									*
************************************************************************/

/* from automata.c but not exported */
void xmlAutomataSetFlags(xmlAutomataPtr am, int flags);

static int xmlRelaxNGTryCompile(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGDefinePtr def);
/**
 * xmlRelaxNGIsCompileable:
 * @define:  the definition to check
 *
 * Check if a definition is nullable.
 *
 * Returns 1 if yes, 0 if no and -1 in case of error
 */
static int FASTCALL xmlRelaxNGIsCompileable(xmlRelaxNGDefine * def)
{
	int ret = -1;
	if(def == NULL) {
		return -1;
	}
	if((def->type != XML_RELAXNG_ELEMENT) && (def->dflags & IS_COMPILABLE))
		return 1;
	if((def->type != XML_RELAXNG_ELEMENT) && (def->dflags & IS_NOT_COMPILABLE))
		return 0;
	switch(def->type) {
		case XML_RELAXNG_NOOP:
		    ret = xmlRelaxNGIsCompileable(def->content); // @recursion
		    break;
		case XML_RELAXNG_TEXT:
		case XML_RELAXNG_EMPTY:
		    ret = 1;
		    break;
		case XML_RELAXNG_ELEMENT:
		    /*
		     * Check if the element content is compileable
		     */
		    if(((def->dflags & IS_NOT_COMPILABLE) == 0) && ((def->dflags & IS_COMPILABLE) == 0)) {
			    for(xmlRelaxNGDefine * list = def->content; list; list = list->next) {
				    ret = xmlRelaxNGIsCompileable(list); // @recursion
				    if(ret != 1)
					    break;
			    }
			    /*
			     * Because the routine is recursive, we must guard against
			     * discovering both COMPILABLE and NOT_COMPILABLE
			     */
			    if(ret == 0) {
				    def->dflags &= ~IS_COMPILABLE;
				    def->dflags |= IS_NOT_COMPILABLE;
			    }
			    if((ret == 1) && !(def->dflags &= IS_NOT_COMPILABLE))
				    def->dflags |= IS_COMPILABLE;
#ifdef DEBUG_COMPILE
			    if(ret == 1) {
				    xmlGenericError(0, "element content for %s is compilable\n", def->name);
			    }
			    else if(ret == 0) {
				    xmlGenericError(0, "element content for %s is not compilable\n", def->name);
			    }
			    else {
				    xmlGenericError(0, "Problem in RelaxNGIsCompileable for element %s\n", def->name);
			    }
#endif
		    }
		    /*
		     * All elements return a compileable status unless they are generic like anyName
		     */
		    ret = (def->nameClass || (def->name == NULL)) ? 0 : 1;
		    return ret;
		case XML_RELAXNG_REF:
		case XML_RELAXNG_EXTERNALREF:
		case XML_RELAXNG_PARENTREF:
		    if(def->depth == -20) {
			    return 1;
		    }
		    else {
			    def->depth = -20;
			    for(xmlRelaxNGDefine * list = def->content; list; list = list->next) {
				    ret = xmlRelaxNGIsCompileable(list); // @recursion
				    if(ret != 1)
					    break;
			    }
		    }
		    break;
		case XML_RELAXNG_START:
		case XML_RELAXNG_OPTIONAL:
		case XML_RELAXNG_ZEROORMORE:
		case XML_RELAXNG_ONEORMORE:
		case XML_RELAXNG_CHOICE:
		case XML_RELAXNG_GROUP:
		case XML_RELAXNG_DEF: 
			{
				for(xmlRelaxNGDefine * list = def->content; list; list = list->next) {
					ret = xmlRelaxNGIsCompileable(list); // @recursion
					if(ret != 1)
						break;
				}
			}
			break;
		case XML_RELAXNG_EXCEPT:
		case XML_RELAXNG_ATTRIBUTE:
		case XML_RELAXNG_INTERLEAVE:
		case XML_RELAXNG_DATATYPE:
		case XML_RELAXNG_LIST:
		case XML_RELAXNG_PARAM:
		case XML_RELAXNG_VALUE:
		case XML_RELAXNG_NOT_ALLOWED:
		    ret = 0;
		    break;
	}
	if(ret == 0)
		def->dflags |= IS_NOT_COMPILABLE;
	if(ret == 1)
		def->dflags |= IS_COMPILABLE;
#ifdef DEBUG_COMPILE
	if(ret == 1) {
		xmlGenericError(0, "RelaxNGIsCompileable %s : true\n", xmlRelaxNGDefName(def));
	}
	else if(ret == 0) {
		xmlGenericError(0, "RelaxNGIsCompileable %s : false\n", xmlRelaxNGDefName(def));
	}
	else {
		xmlGenericError(0, "Problem in RelaxNGIsCompileable %s\n", xmlRelaxNGDefName(def));
	}
#endif
	return ret;
}
/**
 * xmlRelaxNGCompile:
 * ctxt:  the RelaxNG parser context
 * @define:  the definition tree to compile
 *
 * Compile the set of definitions, it works recursively, till the
 * element boundaries, where it tries to compile the content if possible
 *
 * Returns 0 if success and -1 in case of error
 */
static int FASTCALL xmlRelaxNGCompile(xmlRelaxNGParserCtxt * ctxt, xmlRelaxNGDefine * def)
{
	int ret = 0;
	xmlRelaxNGDefine * list;
	if(!ctxt || !def)
		return -1;
	switch(def->type) {
		case XML_RELAXNG_START:
		    if((xmlRelaxNGIsCompileable(def) == 1) && (def->depth != -25)) {
			    xmlAutomataPtr oldam = ctxt->am;
			    xmlAutomataStatePtr oldstate = ctxt->state;
			    def->depth = -25;
			    list = def->content;
			    ctxt->am = xmlNewAutomata();
			    if(ctxt->am == NULL)
				    return -1;
			    /*
			     * assume identical strings but not same pointer are different
			     * atoms, needed for non-determinism detection
			     * That way if 2 elements with the same name are in a choice
			     * branch the automata is found non-deterministic and
			     * we fallback to the normal validation which does the right
			     * thing of exploring both choices.
			     */
			    xmlAutomataSetFlags(ctxt->am, 1);
			    ctxt->state = xmlAutomataGetInitState(ctxt->am);
			    while(list) {
				    xmlRelaxNGCompile(ctxt, list); // @recursion
				    list = list->next;
			    }
			    xmlAutomataSetFinalState(ctxt->am, ctxt->state);
			    if(xmlAutomataIsDeterminist(ctxt->am))
				    def->contModel = xmlAutomataCompile(ctxt->am);
			    xmlFreeAutomata(ctxt->am);
			    ctxt->state = oldstate;
			    ctxt->am = oldam;
		    }
		    break;
		case XML_RELAXNG_ELEMENT:
		    if(ctxt->am && def->name) {
			    ctxt->state = xmlAutomataNewTransition2(ctxt->am, ctxt->state, NULL, def->name, def->ns, def);
		    }
		    if((def->dflags & IS_COMPILABLE) && (def->depth != -25)) {
			    xmlAutomataPtr oldam = ctxt->am;
			    xmlAutomataStatePtr oldstate = ctxt->state;
			    def->depth = -25;
			    list = def->content;
			    ctxt->am = xmlNewAutomata();
			    if(ctxt->am == NULL)
				    return -1;
			    xmlAutomataSetFlags(ctxt->am, 1);
			    ctxt->state = xmlAutomataGetInitState(ctxt->am);
			    while(list) {
				    xmlRelaxNGCompile(ctxt, list); // @recursion
				    list = list->next;
			    }
			    xmlAutomataSetFinalState(ctxt->am, ctxt->state);
			    def->contModel = xmlAutomataCompile(ctxt->am);
			    if(!xmlRegexpIsDeterminist(def->contModel)) {
#ifdef DEBUG_COMPILE
				    xmlGenericError(0, "Content model not determinist %s\n", def->name);
#endif
				    /*
				     * we can only use the automata if it is determinist
				     */
				    xmlRegFreeRegexp(def->contModel);
				    def->contModel = NULL;
			    }
			    xmlFreeAutomata(ctxt->am);
			    ctxt->state = oldstate;
			    ctxt->am = oldam;
		    }
		    else {
			    xmlAutomataPtr oldam = ctxt->am;
			    /*
			     * we can't build the content model for this element content
			     * but it still might be possible to build it for some of its children, recurse.
			     */
			    ret = xmlRelaxNGTryCompile(ctxt, def);
			    ctxt->am = oldam;
		    }
		    break;
		case XML_RELAXNG_NOOP:
		    ret = xmlRelaxNGCompile(ctxt, def->content); // @recursion
		    break;
		case XML_RELAXNG_OPTIONAL: 
			{
				xmlAutomataStatePtr oldstate = ctxt->state;
				for(list = def->content; list; list = list->next)
					xmlRelaxNGCompile(ctxt, list); // @recursion
				xmlAutomataNewEpsilon(ctxt->am, oldstate, ctxt->state);
			}
			break;
		case XML_RELAXNG_ZEROORMORE: 
			{
				ctxt->state = xmlAutomataNewEpsilon(ctxt->am, ctxt->state, 0);
				xmlAutomataState * oldstate = ctxt->state;
				for(list = def->content; list; list = list->next)
					xmlRelaxNGCompile(ctxt, list); // @recursion
				xmlAutomataNewEpsilon(ctxt->am, ctxt->state, oldstate);
				ctxt->state = xmlAutomataNewEpsilon(ctxt->am, oldstate, 0);
			}
			break;
		case XML_RELAXNG_ONEORMORE: 
			{
				for(list = def->content; list; list = list->next)
					xmlRelaxNGCompile(ctxt, list); // @recursion
				xmlAutomataState * oldstate = ctxt->state;
				for(list = def->content; list; list = list->next)
					xmlRelaxNGCompile(ctxt, list); // @recursion
				xmlAutomataNewEpsilon(ctxt->am, ctxt->state, oldstate);
				ctxt->state = xmlAutomataNewEpsilon(ctxt->am, oldstate, 0);
			}
			break;
		case XML_RELAXNG_CHOICE: 
			{
				xmlAutomataStatePtr target = NULL;
				xmlAutomataStatePtr oldstate = ctxt->state;
				for(list = def->content; list; list = list->next) {
					ctxt->state = oldstate;
					ret = xmlRelaxNGCompile(ctxt, list); // @recursion
					if(ret != 0)
						break;
					if(target == NULL)
						target = ctxt->state;
					else
						xmlAutomataNewEpsilon(ctxt->am, ctxt->state, target);
				}
				ctxt->state = target;
			}
			break;
		case XML_RELAXNG_REF:
		case XML_RELAXNG_EXTERNALREF:
		case XML_RELAXNG_PARENTREF:
		case XML_RELAXNG_GROUP:
		case XML_RELAXNG_DEF:
		    for(list = def->content; list; list = list->next) {
			    ret = xmlRelaxNGCompile(ctxt, list); // @recursion
			    if(ret != 0)
				    break;
		    }
		    break;
		case XML_RELAXNG_TEXT: 
			{
				xmlAutomataStatePtr oldstate;
				ctxt->state = xmlAutomataNewEpsilon(ctxt->am, ctxt->state, 0);
				oldstate = ctxt->state;
				xmlRelaxNGCompile(ctxt, def->content); // @recursion
				xmlAutomataNewTransition(ctxt->am, ctxt->state, ctxt->state, BAD_CAST "#text", 0);
				ctxt->state = xmlAutomataNewEpsilon(ctxt->am, oldstate, 0);
			}
			break;
		case XML_RELAXNG_EMPTY:
		    ctxt->state = xmlAutomataNewEpsilon(ctxt->am, ctxt->state, 0);
		    break;
		case XML_RELAXNG_EXCEPT:
		case XML_RELAXNG_ATTRIBUTE:
		case XML_RELAXNG_INTERLEAVE:
		case XML_RELAXNG_NOT_ALLOWED:
		case XML_RELAXNG_DATATYPE:
		case XML_RELAXNG_LIST:
		case XML_RELAXNG_PARAM:
		case XML_RELAXNG_VALUE:
		    // This should not happen and generate an internal error 
		    fprintf(stderr, "RNG internal error trying to compile %s\n", xmlRelaxNGDefName(def));
		    break;
	}
	return ret;
}
/**
 * xmlRelaxNGTryCompile:
 * ctxt:  the RelaxNG parser context
 * @define:  the definition tree to compile
 *
 * Try to compile the set of definitions, it works recursively,
 * possibly ignoring parts which cannot be compiled.
 *
 * Returns 0 if success and -1 in case of error
 */
static int xmlRelaxNGTryCompile(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGDefinePtr def)
{
	int ret = 0;
	xmlRelaxNGDefinePtr list;
	if(!ctxt || (def == NULL))
		return -1;
	if(oneof2(def->type, XML_RELAXNG_START, XML_RELAXNG_ELEMENT)) {
		ret = xmlRelaxNGIsCompileable(def);
		if((def->dflags & IS_COMPILABLE) && (def->depth != -25)) {
			ctxt->am = NULL;
			ret = xmlRelaxNGCompile(ctxt, def);
#ifdef DEBUG_PROGRESSIVE
			if(ret == 0) {
				if(def->type == XML_RELAXNG_START)
					xmlGenericError(0, "compiled the start\n");
				else
					xmlGenericError(0, "compiled element %s\n", def->name);
			}
			else {
				if(def->type == XML_RELAXNG_START)
					xmlGenericError(0, "failed to compile the start\n");
				else
					xmlGenericError(0, "failed to compile element %s\n", def->name);
			}
#endif
			return ret;
		}
	}
	switch(def->type) {
		case XML_RELAXNG_NOOP:
		    ret = xmlRelaxNGTryCompile(ctxt, def->content);
		    break;
		case XML_RELAXNG_TEXT:
		case XML_RELAXNG_DATATYPE:
		case XML_RELAXNG_LIST:
		case XML_RELAXNG_PARAM:
		case XML_RELAXNG_VALUE:
		case XML_RELAXNG_EMPTY:
		case XML_RELAXNG_ELEMENT:
		    ret = 0;
		    break;
		case XML_RELAXNG_OPTIONAL:
		case XML_RELAXNG_ZEROORMORE:
		case XML_RELAXNG_ONEORMORE:
		case XML_RELAXNG_CHOICE:
		case XML_RELAXNG_GROUP:
		case XML_RELAXNG_DEF:
		case XML_RELAXNG_START:
		case XML_RELAXNG_REF:
		case XML_RELAXNG_EXTERNALREF:
		case XML_RELAXNG_PARENTREF:
		    list = def->content;
		    while(list) {
			    ret = xmlRelaxNGTryCompile(ctxt, list);
			    if(ret != 0)
				    break;
			    list = list->next;
		    }
		    break;
		case XML_RELAXNG_EXCEPT:
		case XML_RELAXNG_ATTRIBUTE:
		case XML_RELAXNG_INTERLEAVE:
		case XML_RELAXNG_NOT_ALLOWED:
		    ret = 0;
		    break;
	}
	return ret;
}

/************************************************************************
*									*
*			Parsing functions				*
*									*
************************************************************************/

static xmlRelaxNGDefinePtr xmlRelaxNGParseAttribute(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node);
static xmlRelaxNGDefinePtr xmlRelaxNGParseElement(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node);
static xmlRelaxNGDefinePtr xmlRelaxNGParsePatterns(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * nodes, int group);
static xmlRelaxNGDefinePtr xmlRelaxNGParsePattern(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node);
static xmlRelaxNGPtr xmlRelaxNGParseDocument(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node);
static int xmlRelaxNGParseGrammarContent(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * nodes);
static xmlRelaxNGDefinePtr xmlRelaxNGParseNameClass(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node, xmlRelaxNGDefinePtr def);
static xmlRelaxNGGrammarPtr xmlRelaxNGParseGrammar(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * nodes);
static int xmlRelaxNGElementMatch(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr define, xmlNode * elem);

#define IS_BLANK_NODE(n) (xmlRelaxNGIsBlank((n)->content))

/**
 * xmlRelaxNGIsNullable:
 * @define:  the definition to verify
 *
 * Check if a definition is nullable.
 *
 * Returns 1 if yes, 0 if no and -1 in case of error
 */
static int xmlRelaxNGIsNullable(xmlRelaxNGDefinePtr define)
{
	int ret;

	if(define == NULL)
		return -1;

	if(define->dflags & IS_NULLABLE)
		return 1;
	if(define->dflags & IS_NOT_NULLABLE)
		return 0;
	switch(define->type) {
		case XML_RELAXNG_EMPTY:
		case XML_RELAXNG_TEXT:
		    ret = 1;
		    break;
		case XML_RELAXNG_NOOP:
		case XML_RELAXNG_DEF:
		case XML_RELAXNG_REF:
		case XML_RELAXNG_EXTERNALREF:
		case XML_RELAXNG_PARENTREF:
		case XML_RELAXNG_ONEORMORE:
		    ret = xmlRelaxNGIsNullable(define->content);
		    break;
		case XML_RELAXNG_EXCEPT:
		case XML_RELAXNG_NOT_ALLOWED:
		case XML_RELAXNG_ELEMENT:
		case XML_RELAXNG_DATATYPE:
		case XML_RELAXNG_PARAM:
		case XML_RELAXNG_VALUE:
		case XML_RELAXNG_LIST:
		case XML_RELAXNG_ATTRIBUTE:
		    ret = 0;
		    break;
		case XML_RELAXNG_CHOICE: {
		    xmlRelaxNGDefinePtr list = define->content;

		    while(list) {
			    ret = xmlRelaxNGIsNullable(list);
			    if(ret != 0)
				    goto done;
			    list = list->next;
		    }
		    ret = 0;
		    break;
	    }
		case XML_RELAXNG_START:
		case XML_RELAXNG_INTERLEAVE:
		case XML_RELAXNG_GROUP: {
		    xmlRelaxNGDefinePtr list = define->content;

		    while(list) {
			    ret = xmlRelaxNGIsNullable(list);
			    if(ret != 1)
				    goto done;
			    list = list->next;
		    }
		    return 1;
	    }
		default:
		    return -1;
	}
done:
	if(ret == 0)
		define->dflags |= IS_NOT_NULLABLE;
	if(ret == 1)
		define->dflags |= IS_NULLABLE;
	return ret;
}

/**
 * xmlRelaxNGIsBlank:
 * @str:  a string
 *
 * Check if a string is ignorable c.f. 4.2. Whitespace
 *
 * Returns 1 if the string is NULL or made of blanks chars, 0 otherwise
 */
static int xmlRelaxNGIsBlank(xmlChar * str)
{
	if(!str)
		return 1;
	while(*str != 0) {
		if(!(IS_BLANK_CH(*str)))
			return 0;
		str++;
	}
	return 1;
}

/**
 * xmlRelaxNGGetDataTypeLibrary:
 * @ctxt:  a Relax-NG parser context
 * @node:  the current data or value element
 *
 * Applies algorithm from 4.3. datatypeLibrary attribute
 *
 * Returns the datatypeLibary value or NULL if not found
 */
static xmlChar * xmlRelaxNGGetDataTypeLibrary(xmlRelaxNGParserCtxtPtr ctxt ATTRIBUTE_UNUSED, xmlNode * P_Node)
{
	xmlChar * ret, * escape;
	if(!P_Node)
		return 0;
	if((IS_RELAXNG(P_Node, "data")) || (IS_RELAXNG(P_Node, "value"))) {
		ret = xmlGetProp(P_Node, BAD_CAST "datatypeLibrary");
		if(ret) {
			if(ret[0] == 0) {
				SAlloc::F(ret);
				return 0;
			}
			escape = xmlURIEscapeStr(ret, BAD_CAST ":/#?");
			if(escape == NULL) {
				return ret;
			}
			SAlloc::F(ret);
			return (escape);
		}
	}
	P_Node = P_Node->parent;
	while(P_Node && (P_Node->type == XML_ELEMENT_NODE)) {
		ret = xmlGetProp(P_Node, BAD_CAST "datatypeLibrary");
		if(ret) {
			if(ret[0] == 0) {
				SAlloc::F(ret);
				return 0;
			}
			escape = xmlURIEscapeStr(ret, BAD_CAST ":/#?");
			if(escape == NULL) {
				return ret;
			}
			SAlloc::F(ret);
			return (escape);
		}
		P_Node = P_Node->parent;
	}
	return 0;
}

/**
 * xmlRelaxNGParseValue:
 * @ctxt:  a Relax-NG parser context
 * @node:  the data node.
 *
 * parse the content of a RelaxNG value node.
 *
 * Returns the definition pointer or NULL in case of error
 */
static xmlRelaxNGDefinePtr xmlRelaxNGParseValue(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node)
{
	xmlRelaxNGTypeLibraryPtr lib = NULL;
	xmlChar * type;
	xmlChar * library;
	int success = 0;
	xmlRelaxNGDefine * def = xmlRelaxNGNewDefine(ctxt, P_Node);
	if(def == NULL)
		return 0;
	def->type = XML_RELAXNG_VALUE;
	type = xmlGetProp(P_Node, BAD_CAST "type");
	if(type) {
		xmlRelaxNGNormExtSpace(type);
		if(xmlValidateNCName(type, 0)) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_TYPE_VALUE, "value type '%s' is not an NCName\n", type, 0);
		}
		library = xmlRelaxNGGetDataTypeLibrary(ctxt, P_Node);
		SETIFZ(library, sstrdup(BAD_CAST "http://relaxng.org/ns/structure/1.0"));
		def->name = type;
		def->ns = library;
		lib = (xmlRelaxNGTypeLibraryPtr)xmlHashLookup(xmlRelaxNGRegisteredTypes, library);
		if(lib == NULL) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_UNKNOWN_TYPE_LIB, "Use of unregistered type library '%s'\n", library, 0);
			def->data = NULL;
		}
		else {
			def->data = lib;
			if(lib->have == NULL) {
				xmlRngPErr(ctxt, P_Node, XML_RNGP_ERROR_TYPE_LIB, "Internal error with type library '%s': no 'have'\n", library, 0);
			}
			else {
				success = lib->have(lib->data, def->name);
				if(success != 1) {
					xmlRngPErr(ctxt, P_Node, XML_RNGP_TYPE_NOT_FOUND, "Error type '%s' is not exported by type library '%s'\n", def->name, library);
				}
			}
		}
	}
	if(P_Node->children == NULL) {
		def->value = sstrdup(BAD_CAST "");
	}
	else if(((P_Node->children->type != XML_TEXT_NODE) && (P_Node->children->type != XML_CDATA_SECTION_NODE)) || P_Node->children->next) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_TEXT_EXPECTED, "Expecting a single text value for <value>content\n", 0, 0);
	}
	else if(def) {
		def->value = xmlNodeGetContent(P_Node);
		if(def->value == NULL) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_VALUE_NO_CONTENT, "Element <value> has no content\n", 0, 0);
		}
		else if(lib && lib->check && (success == 1)) {
			void * val = NULL;
			success = lib->check(lib->data, def->name, def->value, &val, P_Node);
			if(success != 1) {
				xmlRngPErr(ctxt, P_Node, XML_RNGP_INVALID_VALUE, "Value '%s' is not acceptable for type '%s'\n", def->value, def->name);
			}
			else {
				if(val)
					def->attrs = (xmlRelaxNGDefinePtr)val;
			}
		}
	}
	return (def);
}

/**
 * xmlRelaxNGParseData:
 * @ctxt:  a Relax-NG parser context
 * @node:  the data node.
 *
 * parse the content of a RelaxNG data node.
 *
 * Returns the definition pointer or NULL in case of error
 */
static xmlRelaxNGDefinePtr xmlRelaxNGParseData(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node)
{
	xmlRelaxNGDefinePtr def = NULL, except;
	xmlRelaxNGDefinePtr param, lastparam = NULL;
	xmlRelaxNGTypeLibraryPtr lib;
	xmlChar * library;
	xmlNode * content;
	int tmp;
	xmlChar * type = xmlGetProp(P_Node, BAD_CAST "type");
	if(type == NULL) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_TYPE_MISSING, "data has no type\n", 0, 0);
		return 0;
	}
	xmlRelaxNGNormExtSpace(type);
	if(xmlValidateNCName(type, 0)) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_TYPE_VALUE, "data type '%s' is not an NCName\n", type, 0);
	}
	library = xmlRelaxNGGetDataTypeLibrary(ctxt, P_Node);
	SETIFZ(library, sstrdup(BAD_CAST "http://relaxng.org/ns/structure/1.0"));
	def = xmlRelaxNGNewDefine(ctxt, P_Node);
	if(def == NULL) {
		SAlloc::F(type);
		return 0;
	}
	def->type = XML_RELAXNG_DATATYPE;
	def->name = type;
	def->ns = library;
	lib = (xmlRelaxNGTypeLibraryPtr)xmlHashLookup(xmlRelaxNGRegisteredTypes, library);
	if(lib == NULL) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_UNKNOWN_TYPE_LIB, "Use of unregistered type library '%s'\n", library, 0);
		def->data = NULL;
	}
	else {
		def->data = lib;
		if(lib->have == NULL) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_ERROR_TYPE_LIB, "Internal error with type library '%s': no 'have'\n", library, 0);
		}
		else {
			tmp = lib->have(lib->data, def->name);
			if(tmp != 1) {
				xmlRngPErr(ctxt, P_Node, XML_RNGP_TYPE_NOT_FOUND, "Error type '%s' is not exported by type library '%s'\n", def->name, library);
			}
			else if((sstreq(library, "http://www.w3.org/2001/XMLSchema-datatypes")) && ((sstreq(def->name, "IDREF")) || (sstreq(def->name, "IDREFS")))) {
				ctxt->idref = 1;
			}
		}
	}
	content = P_Node->children;
	/*
	 * Handle optional params
	 */
	while(content) {
		if(!sstreq(content->name, "param"))
			break;
		if(sstreq(library, "http://relaxng.org/ns/structure/1.0")) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_PARAM_FORBIDDEN, "Type library '%s' does not allow type parameters\n", library, 0);
			content = content->next;
			while(content && sstreq(content->name, "param"))
				content = content->next;
		}
		else {
			param = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(param) {
				param->type = XML_RELAXNG_PARAM;
				param->name = xmlGetProp(content, BAD_CAST "name");
				if(param->name == NULL) {
					xmlRngPErr(ctxt, P_Node, XML_RNGP_PARAM_NAME_MISSING, "param has no name\n", 0, 0);
				}
				param->value = xmlNodeGetContent(content);
				if(lastparam == NULL) {
					def->attrs = lastparam = param;
				}
				else {
					lastparam->next = param;
					lastparam = param;
				}
				if(lib) {
				}
			}
			content = content->next;
		}
	}
	/*
	 * Handle optional except
	 */
	if(content && sstreq(content->name, "except")) {
		xmlNode * child;
		xmlRelaxNGDefinePtr tmp2, last = NULL;
		except = xmlRelaxNGNewDefine(ctxt, P_Node);
		if(except == NULL) {
			return (def);
		}
		except->type = XML_RELAXNG_EXCEPT;
		child = content->children;
		def->content = except;
		if(child == NULL) {
			xmlRngPErr(ctxt, content, XML_RNGP_EXCEPT_NO_CONTENT, "except has no content\n", 0, 0);
		}
		while(child) {
			tmp2 = xmlRelaxNGParsePattern(ctxt, child);
			if(tmp2) {
				if(last == NULL) {
					except->content = last = tmp2;
				}
				else {
					last->next = tmp2;
					last = tmp2;
				}
			}
			child = child->next;
		}
		content = content->next;
	}
	//
	// Check there is no unhandled data
	//
	if(content) {
		xmlRngPErr(ctxt, content, XML_RNGP_DATA_CONTENT, "Element data has unexpected content %s\n", content->name, 0);
	}
	return (def);
}

static const xmlChar * invalidName = BAD_CAST "\1";

/**
 * xmlRelaxNGCompareNameClasses:
 * @defs1:  the first element/attribute defs
 * @defs2:  the second element/attribute defs
 * @name:  the restriction on the name
 * @ns:  the restriction on the namespace
 *
 * Compare the 2 lists of element definitions. The comparison is
 * that if both lists do not accept the same QNames, it returns 1
 * If the 2 lists can accept the same QName the comparison returns 0
 *
 * Returns 1 disttinct, 0 if equal
 */
static int xmlRelaxNGCompareNameClasses(xmlRelaxNGDefinePtr def1, xmlRelaxNGDefinePtr def2)
{
	int ret = 1;
	xmlNode P_Node;
	xmlNs ns;
	xmlRelaxNGValidCtxt ctxt;
	MEMSZERO(ctxt);
	ctxt.flags = FLAGS_IGNORABLE | FLAGS_NOERROR;
	if((def1->type == XML_RELAXNG_ELEMENT) || (def1->type == XML_RELAXNG_ATTRIBUTE)) {
		if(def2->type == XML_RELAXNG_TEXT)
			return 1;
		P_Node.name = def1->name ? def1->name : invalidName;
		if(def1->ns) {
			if(def1->ns[0] == 0) {
				P_Node.ns = NULL;
			}
			else {
				P_Node.ns = &ns;
				ns.href = def1->ns;
			}
		}
		else {
			P_Node.ns = NULL;
		}
		if(xmlRelaxNGElementMatch(&ctxt, def2, &P_Node))
			ret = def1->nameClass ? xmlRelaxNGCompareNameClasses(def1->nameClass, def2) : 0;
		else
			ret = 1;
	}
	else if(def1->type == XML_RELAXNG_TEXT) {
		if(def2->type == XML_RELAXNG_TEXT)
			return 0;
		return 1;
	}
	else if(def1->type == XML_RELAXNG_EXCEPT) {
		TODO ret = 0;
	}
	else {
		TODO ret = 0;
	}
	if(ret == 0)
		return ret;
	if(oneof2(def2->type, XML_RELAXNG_ELEMENT, XML_RELAXNG_ATTRIBUTE)) {
		P_Node.name = def2->name ? def2->name : invalidName;
		P_Node.ns = &ns;
		if(def2->ns) {
			if(def2->ns[0] == 0) {
				P_Node.ns = NULL;
			}
			else {
				ns.href = def2->ns;
			}
		}
		else {
			ns.href = invalidName;
		}
		if(xmlRelaxNGElementMatch(&ctxt, def1, &P_Node)) {
			ret = def2->nameClass ? xmlRelaxNGCompareNameClasses(def2->nameClass, def1) : 0;
		}
		else {
			ret = 1;
		}
	}
	else {
		TODO ret = 0;
	}
	return ret;
}
/**
 * xmlRelaxNGCompareElemDefLists:
 * @ctxt:  a Relax-NG parser context
 * @defs1:  the first list of element/attribute defs
 * @defs2:  the second list of element/attribute defs
 *
 * Compare the 2 lists of element or attribute definitions. The comparison
 * is that if both lists do not accept the same QNames, it returns 1
 * If the 2 lists can accept the same QName the comparison returns 0
 *
 * Returns 1 disttinct, 0 if equal
 */
static int xmlRelaxNGCompareElemDefLists(xmlRelaxNGParserCtxtPtr ctxt ATTRIBUTE_UNUSED, xmlRelaxNGDefinePtr * def1, xmlRelaxNGDefinePtr * def2)
{
	xmlRelaxNGDefinePtr * basedef2 = def2;
	if((def1 == NULL) || (def2 == NULL))
		return 1;
	if((*def1 == NULL) || (*def2 == NULL))
		return 1;
	while(*def1) {
		while(*def2) {
			if(xmlRelaxNGCompareNameClasses(*def1, *def2) == 0)
				return 0;
			def2++;
		}
		def2 = basedef2;
		def1++;
	}
	return 1;
}

/**
 * xmlRelaxNGGenerateAttributes:
 * @ctxt:  a Relax-NG parser context
 * @def:  the definition definition
 *
 * Check if the definition can only generate attributes
 *
 * Returns 1 if yes, 0 if no and -1 in case of error.
 */
static int xmlRelaxNGGenerateAttributes(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGDefinePtr def)
{
	//
	// Don't run that check in case of error. Infinite recursion becomes possible.
	//
	if(ctxt->nbErrors)
		return -1;
	else {
		xmlRelaxNGDefine * parent = NULL;
		xmlRelaxNGDefine * cur = def;
		while(cur) {
			if(oneof7(cur->type, XML_RELAXNG_ELEMENT, XML_RELAXNG_TEXT, XML_RELAXNG_DATATYPE, XML_RELAXNG_PARAM, XML_RELAXNG_LIST, XML_RELAXNG_VALUE, XML_RELAXNG_EMPTY))
				return 0;
			else {
				if(oneof10(cur->type, XML_RELAXNG_CHOICE, XML_RELAXNG_INTERLEAVE, XML_RELAXNG_GROUP, XML_RELAXNG_ONEORMORE, XML_RELAXNG_ZEROORMORE, XML_RELAXNG_OPTIONAL,
					XML_RELAXNG_PARENTREF, XML_RELAXNG_EXTERNALREF, XML_RELAXNG_REF, XML_RELAXNG_DEF)) {
					if(cur->content) {
						parent = cur;
						cur = cur->content;
						for(xmlRelaxNGDefine * tmp = cur; tmp; tmp = tmp->next)
							tmp->parent = parent;
						continue;
					}
				}
				if(cur == def)
					break;
				else if(cur->next) {
					cur = cur->next;
					continue;
				}
				else {
					do {
						cur = cur->parent;
						if(!cur)
							break;
						if(cur == def)
							return 1;
						if(cur->next) {
							cur = cur->next;
							break;
						}
					} while(cur);
				}
			}
		}
		return 1;
	}
}

/**
 * xmlRelaxNGGetElements:
 * @ctxt:  a Relax-NG parser context
 * @def:  the definition definition
 * @eora:  gather elements (0) or attributes (1)
 *
 * Compute the list of top elements a definition can generate
 *
 * Returns a list of elements or NULL if none was found.
 */
static xmlRelaxNGDefinePtr * xmlRelaxNGGetElements(xmlRelaxNGParserCtxtPtr ctxt,
    xmlRelaxNGDefinePtr def, int eora)
{
	xmlRelaxNGDefinePtr * ret = NULL, parent, cur, tmp;
	int len = 0;
	int max = 0;

	/*
	 * Don't run that check in case of error. Infinite recursion
	 * becomes possible.
	 */
	if(ctxt->nbErrors != 0)
		return 0;

	parent = NULL;
	cur = def;
	while(cur) {
		if(((eora == 0) && ((cur->type == XML_RELAXNG_ELEMENT) || (cur->type == XML_RELAXNG_TEXT))) || ((eora == 1) && (cur->type == XML_RELAXNG_ATTRIBUTE))) {
			if(!ret) {
				max = 10;
				ret = (xmlRelaxNGDefinePtr*)SAlloc::M((max + 1) * sizeof(xmlRelaxNGDefinePtr));
				if(!ret) {
					xmlRngPErrMemory(ctxt, "getting element list\n");
					return 0;
				}
			}
			else if(max <= len) {
				xmlRelaxNGDefinePtr * temp;
				max *= 2;
				temp = (xmlRelaxNGDefinePtr *)SAlloc::R(ret, (max + 1) * sizeof(xmlRelaxNGDefinePtr));
				if(temp == NULL) {
					xmlRngPErrMemory(ctxt, "getting element list\n");
					SAlloc::F(ret);
					return 0;
				}
				ret = temp;
			}
			ret[len++] = cur;
			ret[len] = NULL;
		}
		else if(oneof10(cur->type, XML_RELAXNG_CHOICE, XML_RELAXNG_INTERLEAVE, XML_RELAXNG_GROUP,
		    XML_RELAXNG_ONEORMORE, XML_RELAXNG_ZEROORMORE, XML_RELAXNG_OPTIONAL, XML_RELAXNG_PARENTREF,
		    XML_RELAXNG_REF, XML_RELAXNG_DEF, XML_RELAXNG_EXTERNALREF)) {
			/*
			 * Don't go within elements or attributes or string values.
			 * Just gather the element top list
			 */
			if(cur->content) {
				parent = cur;
				cur = cur->content;
				tmp = cur;
				while(tmp) {
					tmp->parent = parent;
					tmp = tmp->next;
				}
				continue;
			}
		}
		if(cur == def)
			break;
		if(cur->next) {
			cur = cur->next;
			continue;
		}
		do {
			cur = cur->parent;
			if(!cur)
				break;
			if(cur == def)
				return ret;
			if(cur->next) {
				cur = cur->next;
				break;
			}
		} while(cur);
	}
	return ret;
}

/**
 * xmlRelaxNGCheckChoiceDeterminism:
 * @ctxt:  a Relax-NG parser context
 * @def:  the choice definition
 *
 * Also used to find indeterministic pattern in choice
 */
static void xmlRelaxNGCheckChoiceDeterminism(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGDefinePtr def)
{
	xmlRelaxNGDefinePtr ** list;
	int nbchild = 0, i, j, ret;
	int is_indeterminist = 0;
	xmlHashTable * triage = NULL;
	int is_triable = 1;
	if(!def || def->type != XML_RELAXNG_CHOICE || def->dflags & IS_PROCESSED)
		return;
	if(!ctxt->nbErrors) { // Don't run that check in case of error. Infinite recursion becomes possible.
		int is_nullable = xmlRelaxNGIsNullable(def);
		xmlRelaxNGDefine * cur = def->content;
		while(cur) {
			nbchild++;
			cur = cur->next;
		}
		list = (xmlRelaxNGDefinePtr**)SAlloc::M(nbchild * sizeof(xmlRelaxNGDefinePtr*));
		if(list == NULL) {
			xmlRngPErrMemory(ctxt, "building choice\n");
			return;
		}
		i = 0;
		/*
		 * a bit strong but safe
		 */
		if(is_nullable == 0) {
			triage = xmlHashCreate(10);
		}
		else {
			is_triable = 0;
		}
		cur = def->content;
		while(cur) {
			list[i] = xmlRelaxNGGetElements(ctxt, cur, 0);
			if((list[i] == NULL) || (list[i][0] == NULL)) {
				is_triable = 0;
			}
			else if(is_triable == 1) {
				int res;
				xmlRelaxNGDefinePtr * tmp = list[i];
				while(*tmp && (is_triable == 1)) {
					if((*tmp)->type == XML_RELAXNG_TEXT) {
						res = xmlHashAddEntry2(triage, BAD_CAST "#text", NULL, (void*)cur);
						if(res != 0)
							is_triable = -1;
					}
					else if(((*tmp)->type == XML_RELAXNG_ELEMENT) && (*tmp)->name) {
						if(((*tmp)->ns == NULL) || ((*tmp)->ns[0] == 0))
							res = xmlHashAddEntry2(triage, (*tmp)->name, NULL, (void*)cur);
						else
							res = xmlHashAddEntry2(triage, (*tmp)->name, (*tmp)->ns, (void*)cur);
						if(res != 0)
							is_triable = -1;
					}
					else if((*tmp)->type == XML_RELAXNG_ELEMENT) {
						if(((*tmp)->ns == NULL) || ((*tmp)->ns[0] == 0))
							res = xmlHashAddEntry2(triage, BAD_CAST "#any", NULL, (void*)cur);
						else
							res = xmlHashAddEntry2(triage, BAD_CAST "#any", (*tmp)->ns, (void*)cur);
						if(res != 0)
							is_triable = -1;
					}
					else {
						is_triable = -1;
					}
					tmp++;
				}
			}
			i++;
			cur = cur->next;
		}
		for(i = 0; i < nbchild; i++) {
			if(list[i] == NULL)
				continue;
			for(j = 0; j < i; j++) {
				if(list[j] == NULL)
					continue;
				ret = xmlRelaxNGCompareElemDefLists(ctxt, list[i], list[j]);
				if(ret == 0) {
					is_indeterminist = 1;
				}
			}
		}
		for(i = 0; i < nbchild; i++) {
			SAlloc::F(list[i]);
		}
		SAlloc::F(list);
		if(is_indeterminist) {
			def->dflags |= IS_INDETERMINIST;
		}
		if(is_triable == 1) {
			def->dflags |= IS_TRIABLE;
			def->data = triage;
		}
		else
			xmlHashFree(triage, 0);
		def->dflags |= IS_PROCESSED;
	}
}
/**
 * xmlRelaxNGCheckGroupAttrs:
 * @ctxt:  a Relax-NG parser context
 * @def:  the group definition
 *
 * Detects violations of rule 7.3
 */
static void xmlRelaxNGCheckGroupAttrs(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGDefinePtr def)
{
	xmlRelaxNGDefinePtr ** list;
	xmlRelaxNGDefinePtr cur;
	int nbchild = 0, i, j, ret;
	if((def == NULL) || ((def->type != XML_RELAXNG_GROUP) && (def->type != XML_RELAXNG_ELEMENT)))
		return;
	if(def->dflags & IS_PROCESSED)
		return;
	/*
	 * Don't run that check in case of error. Infinite recursion
	 * becomes possible.
	 */
	if(ctxt->nbErrors != 0)
		return;
	cur = def->attrs;
	while(cur) {
		nbchild++;
		cur = cur->next;
	}
	cur = def->content;
	while(cur) {
		nbchild++;
		cur = cur->next;
	}
	list = (xmlRelaxNGDefinePtr**)SAlloc::M(nbchild * sizeof(xmlRelaxNGDefinePtr*));
	if(list == NULL) {
		xmlRngPErrMemory(ctxt, "building group\n");
		return;
	}
	i = 0;
	cur = def->attrs;
	while(cur) {
		list[i] = xmlRelaxNGGetElements(ctxt, cur, 1);
		i++;
		cur = cur->next;
	}
	cur = def->content;
	while(cur) {
		list[i] = xmlRelaxNGGetElements(ctxt, cur, 1);
		i++;
		cur = cur->next;
	}
	for(i = 0; i < nbchild; i++) {
		if(list[i] == NULL)
			continue;
		for(j = 0; j < i; j++) {
			if(list[j] == NULL)
				continue;
			ret = xmlRelaxNGCompareElemDefLists(ctxt, list[i], list[j]);
			if(ret == 0) {
				xmlRngPErr(ctxt, def->P_Node, XML_RNGP_GROUP_ATTR_CONFLICT, "Attributes conflicts in group\n", 0, 0);
			}
		}
	}
	for(i = 0; i < nbchild; i++) {
		SAlloc::F(list[i]);
	}
	SAlloc::F(list);
	def->dflags |= IS_PROCESSED;
}

/**
 * xmlRelaxNGComputeInterleaves:
 * @def:  the interleave definition
 * @ctxt:  a Relax-NG parser context
 * @name:  the definition name
 *
 * A lot of work for preprocessing interleave definitions
 * is potentially needed to get a decent execution speed at runtime
 *   - trying to get a total order on the element nodes generated
 *     by the interleaves, order the list of interleave definitions
 *     following that order.
 *   - if <text/> is used to handle mixed content, it is better to
 *     flag this in the define and simplify the runtime checking
 *     algorithm
 */
static void xmlRelaxNGComputeInterleaves(xmlRelaxNGDefinePtr def, xmlRelaxNGParserCtxtPtr ctxt, xmlChar * name ATTRIBUTE_UNUSED)
{
	xmlRelaxNGDefinePtr cur, * tmp;
	xmlRelaxNGPartitionPtr partitions = NULL;
	xmlRelaxNGInterleaveGroupPtr * groups = NULL;
	xmlRelaxNGInterleaveGroupPtr group;
	int i, j, ret, res;
	int nbgroups = 0;
	int nbchild = 0;
	int is_mixed = 0;
	int is_determinist = 1;
	/*
	 * Don't run that check in case of error. Infinite recursion
	 * becomes possible.
	 */
	if(ctxt->nbErrors != 0)
		return;
#ifdef DEBUG_INTERLEAVE
	xmlGenericError(0, "xmlRelaxNGComputeInterleaves(%s)\n", name);
#endif
	cur = def->content;
	while(cur) {
		nbchild++;
		cur = cur->next;
	}
#ifdef DEBUG_INTERLEAVE
	xmlGenericError(0, "  %d child\n", nbchild);
#endif
	groups = (xmlRelaxNGInterleaveGroupPtr*)SAlloc::M(nbchild * sizeof(xmlRelaxNGInterleaveGroupPtr));
	if(groups == NULL)
		goto error;
	cur = def->content;
	while(cur) {
		groups[nbgroups] = (xmlRelaxNGInterleaveGroupPtr)SAlloc::M(sizeof(xmlRelaxNGInterleaveGroup));
		if(groups[nbgroups] == NULL)
			goto error;
		if(cur->type == XML_RELAXNG_TEXT)
			is_mixed++;
		groups[nbgroups]->rule = cur;
		groups[nbgroups]->defs = xmlRelaxNGGetElements(ctxt, cur, 0);
		groups[nbgroups]->attrs = xmlRelaxNGGetElements(ctxt, cur, 1);
		nbgroups++;
		cur = cur->next;
	}
#ifdef DEBUG_INTERLEAVE
	xmlGenericError(0, "  %d groups\n", nbgroups);
#endif
	/*
	 * Let's check that all rules makes a partitions according to 7.4
	 */
	partitions = (xmlRelaxNGPartitionPtr)SAlloc::M(sizeof(xmlRelaxNGPartition));
	if(partitions == NULL)
		goto error;
	memzero(partitions, sizeof(xmlRelaxNGPartition));
	partitions->nbgroups = nbgroups;
	partitions->triage = xmlHashCreate(nbgroups);
	for(i = 0; i < nbgroups; i++) {
		group = groups[i];
		for(j = i + 1; j < nbgroups; j++) {
			if(groups[j] == NULL)
				continue;
			ret = xmlRelaxNGCompareElemDefLists(ctxt, group->defs, groups[j]->defs);
			if(ret == 0) {
				xmlRngPErr(ctxt, def->P_Node, XML_RNGP_ELEM_TEXT_CONFLICT, "Element or text conflicts in interleave\n", 0, 0);
			}
			ret = xmlRelaxNGCompareElemDefLists(ctxt, group->attrs, groups[j]->attrs);
			if(ret == 0) {
				xmlRngPErr(ctxt, def->P_Node, XML_RNGP_ATTR_CONFLICT, "Attributes conflicts in interleave\n", 0, 0);
			}
		}
		tmp = group->defs;
		if(tmp && *tmp) {
			while(*tmp) {
				if((*tmp)->type == XML_RELAXNG_TEXT) {
					res = xmlHashAddEntry2(partitions->triage, BAD_CAST "#text", NULL, (void*)(long)(i + 1));
					if(res != 0)
						is_determinist = -1;
				}
				else if(((*tmp)->type == XML_RELAXNG_ELEMENT) && (*tmp)->name) {
					if(((*tmp)->ns == NULL) || ((*tmp)->ns[0] == 0))
						res = xmlHashAddEntry2(partitions->triage, (*tmp)->name, NULL, (void*)(long)(i + 1));
					else
						res = xmlHashAddEntry2(partitions->triage, (*tmp)->name, (*tmp)->ns, (void*)(long)(i + 1));
					if(res != 0)
						is_determinist = -1;
				}
				else if((*tmp)->type == XML_RELAXNG_ELEMENT) {
					if(((*tmp)->ns == NULL) || ((*tmp)->ns[0] == 0))
						res = xmlHashAddEntry2(partitions->triage, BAD_CAST "#any", NULL, (void*)(long)(i + 1));
					else
						res = xmlHashAddEntry2(partitions->triage, BAD_CAST "#any", (*tmp)->ns, (void*)(long)(i + 1));
					if((*tmp)->nameClass)
						is_determinist = 2;
					if(res != 0)
						is_determinist = -1;
				}
				else {
					is_determinist = -1;
				}
				tmp++;
			}
		}
		else {
			is_determinist = 0;
		}
	}
	partitions->groups = groups;
	/*
	 * and save the partition list back in the def
	 */
	def->data = partitions;
	if(is_mixed != 0)
		def->dflags |= IS_MIXED;
	if(is_determinist == 1)
		partitions->flags = IS_DETERMINIST;
	if(is_determinist == 2)
		partitions->flags = IS_DETERMINIST | IS_NEEDCHECK;
	return;
error:
	xmlRngPErrMemory(ctxt, "in interleave computation\n");
	if(groups) {
		for(i = 0; i < nbgroups; i++)
			if(groups[i]) {
				SAlloc::F(groups[i]->defs);
				SAlloc::F(groups[i]);
			}
		SAlloc::F(groups);
	}
	xmlRelaxNGFreePartition(partitions);
}

/**
 * xmlRelaxNGParseInterleave:
 * @ctxt:  a Relax-NG parser context
 * @node:  the data node.
 *
 * parse the content of a RelaxNG interleave node.
 *
 * Returns the definition pointer or NULL in case of error
 */
static xmlRelaxNGDefinePtr xmlRelaxNGParseInterleave(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node)
{
	xmlRelaxNGDefinePtr def = NULL;
	xmlRelaxNGDefinePtr last = NULL, cur;
	xmlNode * child;
	def = xmlRelaxNGNewDefine(ctxt, P_Node);
	if(def == NULL) {
		return 0;
	}
	def->type = XML_RELAXNG_INTERLEAVE;
	SETIFZ(ctxt->interleaves, xmlHashCreate(10));
	if(ctxt->interleaves == NULL) {
		xmlRngPErrMemory(ctxt, "create interleaves\n");
	}
	else {
		char name[32];
		snprintf(name, 32, "interleave%d", ctxt->nbInterleaves++);
		if(xmlHashAddEntry(ctxt->interleaves, BAD_CAST name, def) < 0) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_INTERLEAVE_ADD, "Failed to add %s to hash table\n", (const xmlChar*)name, 0);
		}
	}
	child = P_Node->children;
	if(child == NULL) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_INTERLEAVE_NO_CONTENT, "Element interleave is empty\n", 0, 0);
	}
	while(child) {
		if(IS_RELAXNG(child, "element")) {
			cur = xmlRelaxNGParseElement(ctxt, child);
		}
		else {
			cur = xmlRelaxNGParsePattern(ctxt, child);
		}
		if(cur) {
			cur->parent = def;
			if(last == NULL) {
				def->content = last = cur;
			}
			else {
				last->next = cur;
				last = cur;
			}
		}
		child = child->next;
	}
	return (def);
}

/**
 * xmlRelaxNGParseInclude:
 * @ctxt:  a Relax-NG parser context
 * @node:  the include node
 *
 * Integrate the content of an include node in the current grammar
 *
 * Returns 0 in case of success or -1 in case of error
 */
static int xmlRelaxNGParseInclude(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node)
{
	xmlNode * root;
	int ret = 0, tmp;
	xmlRelaxNGIncludePtr incl = (xmlRelaxNGIncludePtr)P_Node->psvi;
	if(incl == NULL) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_INCLUDE_EMPTY, "Include node has no data\n", 0, 0);
		return -1;
	}
	root = xmlDocGetRootElement(incl->doc);
	if(root == NULL) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_EMPTY, "Include document is empty\n", 0, 0);
		return -1;
	}
	if(!sstreq(root->name, "grammar")) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_GRAMMAR_MISSING, "Include document root is not a grammar\n", 0, 0);
		return -1;
	}
	/*
	 * Merge the definition from both the include and the internal list
	 */
	if(root->children) {
		tmp = xmlRelaxNGParseGrammarContent(ctxt, root->children);
		if(tmp != 0)
			ret = -1;
	}
	if(P_Node->children) {
		tmp = xmlRelaxNGParseGrammarContent(ctxt, P_Node->children);
		if(tmp != 0)
			ret = -1;
	}
	return ret;
}
/**
 * xmlRelaxNGParseDefine:
 * @ctxt:  a Relax-NG parser context
 * @node:  the define node
 *
 * parse the content of a RelaxNG define element node.
 *
 * Returns 0 in case of success or -1 in case of error
 */
static int xmlRelaxNGParseDefine(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node)
{
	int ret = 0, tmp;
	xmlRelaxNGDefinePtr def;
	const xmlChar * olddefine;
	xmlChar * name = xmlGetProp(P_Node, BAD_CAST "name");
	if(!name) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_DEFINE_NAME_MISSING, "define has no name\n", 0, 0);
	}
	else {
		xmlRelaxNGNormExtSpace(name);
		if(xmlValidateNCName(name, 0)) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_INVALID_DEFINE_NAME, "define name '%s' is not an NCName\n", name, 0);
		}
		def = xmlRelaxNGNewDefine(ctxt, P_Node);
		if(def == NULL) {
			SAlloc::F(name);
			return -1;
		}
		def->type = XML_RELAXNG_DEF;
		def->name = name;
		if(P_Node->children == NULL) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_DEFINE_EMPTY, "define has no children\n", 0, 0);
		}
		else {
			olddefine = ctxt->define;
			ctxt->define = name;
			def->content =
			    xmlRelaxNGParsePatterns(ctxt, P_Node->children, 0);
			ctxt->define = olddefine;
		}
		if(ctxt->grammar->defs == NULL)
			ctxt->grammar->defs = xmlHashCreate(10);
		if(ctxt->grammar->defs == NULL) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_DEFINE_CREATE_FAILED, "Could not create definition hash\n", 0, 0);
			ret = -1;
		}
		else {
			tmp = xmlHashAddEntry(ctxt->grammar->defs, name, def);
			if(tmp < 0) {
				xmlRelaxNGDefinePtr prev;
				prev = (xmlRelaxNGDefinePtr)xmlHashLookup(ctxt->grammar->defs, name);
				if(prev == NULL) {
					xmlRngPErr(ctxt, P_Node, XML_RNGP_DEFINE_CREATE_FAILED, "Internal error on define aggregation of %s\n", name, 0);
					ret = -1;
				}
				else {
					while(prev->nextHash)
						prev = prev->nextHash;
					prev->nextHash = def;
				}
			}
		}
	}
	return ret;
}

/**
 * xmlRelaxNGParseImportRef:
 * @payload: the parser context
 * @data: the current grammar
 * @name: the reference name
 *
 * Import import one references into the current grammar
 */
static void xmlRelaxNGParseImportRef(void * payload, void * data, xmlChar * name) 
{
	xmlRelaxNGParserCtxtPtr ctxt = (xmlRelaxNGParserCtxtPtr)data;
	xmlRelaxNGDefinePtr def = (xmlRelaxNGDefinePtr)payload;
	int tmp;
	def->dflags |= IS_EXTERNAL_REF;
	tmp = xmlHashAddEntry(ctxt->grammar->refs, name, def);
	if(tmp < 0) {
		xmlRelaxNGDefinePtr prev = (xmlRelaxNGDefinePtr)xmlHashLookup(ctxt->grammar->refs, def->name);
		if(prev == NULL) {
			if(def->name)
				xmlRngPErr(ctxt, NULL, XML_RNGP_REF_CREATE_FAILED, "Error refs definitions '%s'\n", def->name, 0);
			else
				xmlRngPErr(ctxt, NULL, XML_RNGP_REF_CREATE_FAILED, "Error refs definitions\n", 0, 0);
		}
		else {
			def->nextHash = prev->nextHash;
			prev->nextHash = def;
		}
	}
}

/**
 * xmlRelaxNGParseImportRefs:
 * @ctxt: the parser context
 * @grammar: the sub grammar
 *
 * Import references from the subgrammar into the current grammar
 *
 * Returns 0 in case of success, -1 in case of failure
 */
static int xmlRelaxNGParseImportRefs(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGGrammarPtr grammar) 
{
	if(!ctxt || (grammar == NULL) || (ctxt->grammar == NULL))
		return -1;
	if(grammar->refs == NULL)
		return 0;
	if(ctxt->grammar->refs == NULL)
		ctxt->grammar->refs = xmlHashCreate(10);
	if(ctxt->grammar->refs == NULL) {
		xmlRngPErr(ctxt, NULL, XML_RNGP_REF_CREATE_FAILED, "Could not create references hash\n", 0, 0);
		return -1;
	}
	xmlHashScan(grammar->refs, xmlRelaxNGParseImportRef, ctxt);
	return 0;
}

/**
 * xmlRelaxNGProcessExternalRef:
 * @ctxt: the parser context
 * @node:  the externlRef node
 *
 * Process and compile an externlRef node
 *
 * Returns the xmlRelaxNGDefinePtr or NULL in case of error
 */
static xmlRelaxNGDefinePtr xmlRelaxNGProcessExternalRef(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node)
{
	xmlRelaxNGDefinePtr def = 0;
	xmlNode * root;
	xmlNode * tmp;
	xmlChar * ns;
	int newNs = 0, oldflags;
	xmlRelaxNGDocumentPtr docu = (xmlRelaxNGDocumentPtr)P_Node->psvi;
	if(docu) {
		def = xmlRelaxNGNewDefine(ctxt, P_Node);
		if(def) {
			def->type = XML_RELAXNG_EXTERNALREF;
			if(docu->content == NULL) {
				// Then do the parsing for good
				root = xmlDocGetRootElement(docu->doc);
				if(root == NULL) {
					xmlRngPErr(ctxt, P_Node, XML_RNGP_EXTERNALREF_EMTPY, "xmlRelaxNGParse: %s is empty\n", ctxt->URL, 0);
					return 0;
				}
				// ns transmission rules
				ns = xmlGetProp(root, BAD_CAST "ns");
				if(ns == NULL) {
					tmp = P_Node;
					while(tmp && (tmp->type == XML_ELEMENT_NODE)) {
						ns = xmlGetProp(tmp, BAD_CAST "ns");
						if(ns) {
							break;
						}
						tmp = tmp->parent;
					}
					if(ns) {
						xmlSetProp(root, BAD_CAST "ns", ns);
						newNs = 1;
						SAlloc::F(ns);
					}
				}
				else {
					SAlloc::F(ns);
				}
				//
				// Parsing to get a precompiled schemas.
				//
				oldflags = ctxt->flags;
				ctxt->flags |= XML_RELAXNG_IN_EXTERNALREF;
				docu->schema = xmlRelaxNGParseDocument(ctxt, root);
				ctxt->flags = oldflags;
				if(docu->schema && docu->schema->topgrammar) {
					docu->content = docu->schema->topgrammar->start;
					if(docu->schema->topgrammar->refs)
						xmlRelaxNGParseImportRefs(ctxt, docu->schema->topgrammar);
				}
				/*
				* the externalRef may be reused in a different ns context
				*/
				if(newNs == 1)
					xmlUnsetProp(root, BAD_CAST "ns");
			}
			def->content = docu->content;
		}
	}
	return def;
}

/**
 * xmlRelaxNGParsePattern:
 * @ctxt:  a Relax-NG parser context
 * @node:  the pattern node.
 *
 * parse the content of a RelaxNG pattern node.
 *
 * Returns the definition pointer or NULL in case of error or if no
 *     pattern is generated.
 */
static xmlRelaxNGDefinePtr xmlRelaxNGParsePattern(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node)
{
	xmlRelaxNGDefine * def = NULL;
	if(P_Node) {
		if(IS_RELAXNG(P_Node, "element")) {
			def = xmlRelaxNGParseElement(ctxt, P_Node);
		}
		else if(IS_RELAXNG(P_Node, "attribute")) {
			def = xmlRelaxNGParseAttribute(ctxt, P_Node);
		}
		else if(IS_RELAXNG(P_Node, "empty")) {
			def = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(def == NULL)
				return 0;
			def->type = XML_RELAXNG_EMPTY;
			if(P_Node->children)
				xmlRngPErr(ctxt, P_Node, XML_RNGP_EMPTY_NOT_EMPTY, "empty: had a child node\n", 0, 0);
		}
		else if(IS_RELAXNG(P_Node, "text")) {
			def = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(def == NULL)
				return 0;
			def->type = XML_RELAXNG_TEXT;
			if(P_Node->children)
				xmlRngPErr(ctxt, P_Node, XML_RNGP_TEXT_HAS_CHILD, "text: had a child node\n", 0, 0);
		}
		else if(IS_RELAXNG(P_Node, "zeroOrMore")) {
			def = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(def == NULL)
				return 0;
			def->type = XML_RELAXNG_ZEROORMORE;
			if(P_Node->children == NULL)
				xmlRngPErr(ctxt, P_Node, XML_RNGP_EMPTY_CONSTRUCT, "Element %s is empty\n", P_Node->name, 0);
			else
				def->content = xmlRelaxNGParsePatterns(ctxt, P_Node->children, 1);
		}
		else if(IS_RELAXNG(P_Node, "oneOrMore")) {
			def = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(def == NULL)
				return 0;
			def->type = XML_RELAXNG_ONEORMORE;
			if(P_Node->children == NULL)
				xmlRngPErr(ctxt, P_Node, XML_RNGP_EMPTY_CONSTRUCT, "Element %s is empty\n", P_Node->name, 0);
			else
				def->content = xmlRelaxNGParsePatterns(ctxt, P_Node->children, 1);
		}
		else if(IS_RELAXNG(P_Node, "optional")) {
			def = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(def == NULL)
				return 0;
			def->type = XML_RELAXNG_OPTIONAL;
			if(P_Node->children == NULL)
				xmlRngPErr(ctxt, P_Node, XML_RNGP_EMPTY_CONSTRUCT, "Element %s is empty\n", P_Node->name, 0);
			else
				def->content = xmlRelaxNGParsePatterns(ctxt, P_Node->children, 1);
		}
		else if(IS_RELAXNG(P_Node, "choice")) {
			def = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(def == NULL)
				return 0;
			def->type = XML_RELAXNG_CHOICE;
			if(P_Node->children == NULL)
				xmlRngPErr(ctxt, P_Node, XML_RNGP_EMPTY_CONSTRUCT, "Element %s is empty\n", P_Node->name, 0);
			else
				def->content = xmlRelaxNGParsePatterns(ctxt, P_Node->children, 0);
		}
		else if(IS_RELAXNG(P_Node, "group")) {
			def = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(def == NULL)
				return 0;
			def->type = XML_RELAXNG_GROUP;
			if(P_Node->children == NULL)
				xmlRngPErr(ctxt, P_Node, XML_RNGP_EMPTY_CONSTRUCT, "Element %s is empty\n", P_Node->name, 0);
			else
				def->content = xmlRelaxNGParsePatterns(ctxt, P_Node->children, 0);
		}
		else if(IS_RELAXNG(P_Node, "ref")) {
			def = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(def == NULL)
				return 0;
			def->type = XML_RELAXNG_REF;
			def->name = xmlGetProp(P_Node, BAD_CAST "name");
			if(def->name == NULL)
				xmlRngPErr(ctxt, P_Node, XML_RNGP_REF_NO_NAME, "ref has no name\n", 0, 0);
			else {
				xmlRelaxNGNormExtSpace(def->name);
				if(xmlValidateNCName(def->name, 0)) {
					xmlRngPErr(ctxt, P_Node, XML_RNGP_REF_NAME_INVALID, "ref name '%s' is not an NCName\n", def->name, 0);
				}
			}
			if(P_Node->children) {
				xmlRngPErr(ctxt, P_Node, XML_RNGP_REF_NOT_EMPTY, "ref is not empty\n", 0, 0);
			}
			SETIFZ(ctxt->grammar->refs, xmlHashCreate(10));
			if(ctxt->grammar->refs == NULL) {
				xmlRngPErr(ctxt, P_Node, XML_RNGP_REF_CREATE_FAILED, "Could not create references hash\n", 0, 0);
				def = NULL;
			}
			else {
				int tmp = xmlHashAddEntry(ctxt->grammar->refs, def->name, def);
				if(tmp < 0) {
					xmlRelaxNGDefinePtr prev = (xmlRelaxNGDefinePtr)xmlHashLookup(ctxt->grammar->refs, def->name);
					if(prev == NULL) {
						if(def->name)
							xmlRngPErr(ctxt, P_Node, XML_RNGP_REF_CREATE_FAILED, "Error refs definitions '%s'\n", def->name, 0);
						else
							xmlRngPErr(ctxt, P_Node, XML_RNGP_REF_CREATE_FAILED, "Error refs definitions\n", 0, 0);
						def = NULL;
					}
					else {
						def->nextHash = prev->nextHash;
						prev->nextHash = def;
					}
				}
			}
		}
		else if(IS_RELAXNG(P_Node, "data"))
			def = xmlRelaxNGParseData(ctxt, P_Node);
		else if(IS_RELAXNG(P_Node, "value"))
			def = xmlRelaxNGParseValue(ctxt, P_Node);
		else if(IS_RELAXNG(P_Node, "list")) {
			def = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(def == NULL)
				return 0;
			def->type = XML_RELAXNG_LIST;
			if(P_Node->children == NULL)
				xmlRngPErr(ctxt, P_Node, XML_RNGP_EMPTY_CONSTRUCT, "Element %s is empty\n", P_Node->name, 0);
			else
				def->content = xmlRelaxNGParsePatterns(ctxt, P_Node->children, 0);
		}
		else if(IS_RELAXNG(P_Node, "interleave"))
			def = xmlRelaxNGParseInterleave(ctxt, P_Node);
		else if(IS_RELAXNG(P_Node, "externalRef"))
			def = xmlRelaxNGProcessExternalRef(ctxt, P_Node);
		else if(IS_RELAXNG(P_Node, "notAllowed")) {
			def = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(def == NULL)
				return 0;
			def->type = XML_RELAXNG_NOT_ALLOWED;
			if(P_Node->children)
				xmlRngPErr(ctxt, P_Node, XML_RNGP_NOTALLOWED_NOT_EMPTY, "xmlRelaxNGParse: notAllowed element is not empty\n", 0, 0);
		}
		else if(IS_RELAXNG(P_Node, "grammar")) {
			xmlRelaxNGGrammarPtr grammar, old;
			xmlRelaxNGGrammarPtr oldparent;
#ifdef DEBUG_GRAMMAR
			xmlGenericError(0, "Found <grammar> pattern\n");
#endif
			oldparent = ctxt->parentgrammar;
			old = ctxt->grammar;
			ctxt->parentgrammar = old;
			grammar = xmlRelaxNGParseGrammar(ctxt, P_Node->children);
			if(old) {
				ctxt->grammar = old;
				ctxt->parentgrammar = oldparent;
#if 0
				if(grammar) {
					grammar->next = old->next;
					old->next = grammar;
				}
#endif
			}
			def = grammar ? grammar->start : NULL;
		}
		else if(IS_RELAXNG(P_Node, "parentRef")) {
			if(ctxt->parentgrammar == NULL) {
				xmlRngPErr(ctxt, P_Node, XML_RNGP_PARENTREF_NO_PARENT, "Use of parentRef without a parent grammar\n", 0, 0);
				return 0;
			}
			def = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(def == NULL)
				return 0;
			def->type = XML_RELAXNG_PARENTREF;
			def->name = xmlGetProp(P_Node, BAD_CAST "name");
			if(def->name == NULL)
				xmlRngPErr(ctxt, P_Node, XML_RNGP_PARENTREF_NO_NAME, "parentRef has no name\n", 0, 0);
			else {
				xmlRelaxNGNormExtSpace(def->name);
				if(xmlValidateNCName(def->name, 0))
					xmlRngPErr(ctxt, P_Node, XML_RNGP_PARENTREF_NAME_INVALID, "parentRef name '%s' is not an NCName\n", def->name, 0);
			}
			if(P_Node->children) {
				xmlRngPErr(ctxt, P_Node, XML_RNGP_PARENTREF_NOT_EMPTY, "parentRef is not empty\n", 0, 0);
			}
			if(ctxt->parentgrammar->refs == NULL)
				ctxt->parentgrammar->refs = xmlHashCreate(10);
			if(ctxt->parentgrammar->refs == NULL) {
				xmlRngPErr(ctxt, P_Node, XML_RNGP_PARENTREF_CREATE_FAILED, "Could not create references hash\n", 0, 0);
				def = NULL;
			}
			else if(def->name) {
				int tmp = xmlHashAddEntry(ctxt->parentgrammar->refs, def->name, def);
				if(tmp < 0) {
					xmlRelaxNGDefinePtr prev = (xmlRelaxNGDefinePtr)xmlHashLookup(ctxt->parentgrammar->refs, def->name);
					if(prev == NULL) {
						xmlRngPErr(ctxt, P_Node, XML_RNGP_PARENTREF_CREATE_FAILED, "Internal error parentRef definitions '%s'\n", def->name, 0);
						def = NULL;
					}
					else {
						def->nextHash = prev->nextHash;
						prev->nextHash = def;
					}
				}
			}
		}
		else if(IS_RELAXNG(P_Node, "mixed")) {
			if(P_Node->children == NULL) {
				xmlRngPErr(ctxt, P_Node, XML_RNGP_EMPTY_CONSTRUCT, "Mixed is empty\n", 0, 0);
				def = NULL;
			}
			else {
				def = xmlRelaxNGParseInterleave(ctxt, P_Node);
				if(def) {
					xmlRelaxNGDefinePtr tmp;
					if(def->content && def->content->next) {
						tmp = xmlRelaxNGNewDefine(ctxt, P_Node);
						if(tmp) {
							tmp->type = XML_RELAXNG_GROUP;
							tmp->content = def->content;
							def->content = tmp;
						}
					}
					tmp = xmlRelaxNGNewDefine(ctxt, P_Node);
					if(!tmp)
						return (def);
					tmp->type = XML_RELAXNG_TEXT;
					tmp->next = def->content;
					def->content = tmp;
				}
			}
		}
		else {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_UNKNOWN_CONSTRUCT, "Unexpected node %s is not a pattern\n", P_Node->name, 0);
			def = NULL;
		}
	}
	return def;
}

/**
 * xmlRelaxNGParseAttribute:
 * @ctxt:  a Relax-NG parser context
 * @node:  the element node
 *
 * parse the content of a RelaxNG attribute node.
 *
 * Returns the definition pointer or NULL in case of error.
 */
static xmlRelaxNGDefinePtr xmlRelaxNGParseAttribute(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node)
{
	xmlRelaxNGDefinePtr cur;
	xmlNode * child;
	int old_flags;
	xmlRelaxNGDefinePtr ret = xmlRelaxNGNewDefine(ctxt, P_Node);
	if(!ret)
		return 0;
	ret->type = XML_RELAXNG_ATTRIBUTE;
	ret->parent = ctxt->def;
	child = P_Node->children;
	if(child == NULL) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_ATTRIBUTE_EMPTY, "xmlRelaxNGParseattribute: attribute has no children\n", 0, 0);
		return ret;
	}
	old_flags = ctxt->flags;
	ctxt->flags |= XML_RELAXNG_IN_ATTRIBUTE;
	cur = xmlRelaxNGParseNameClass(ctxt, child, ret);
	if(cur)
		child = child->next;

	if(child) {
		cur = xmlRelaxNGParsePattern(ctxt, child);
		if(cur) {
			switch(cur->type) {
				case XML_RELAXNG_EMPTY:
				case XML_RELAXNG_NOT_ALLOWED:
				case XML_RELAXNG_TEXT:
				case XML_RELAXNG_ELEMENT:
				case XML_RELAXNG_DATATYPE:
				case XML_RELAXNG_VALUE:
				case XML_RELAXNG_LIST:
				case XML_RELAXNG_REF:
				case XML_RELAXNG_PARENTREF:
				case XML_RELAXNG_EXTERNALREF:
				case XML_RELAXNG_DEF:
				case XML_RELAXNG_ONEORMORE:
				case XML_RELAXNG_ZEROORMORE:
				case XML_RELAXNG_OPTIONAL:
				case XML_RELAXNG_CHOICE:
				case XML_RELAXNG_GROUP:
				case XML_RELAXNG_INTERLEAVE:
				case XML_RELAXNG_ATTRIBUTE:
				    ret->content = cur;
				    cur->parent = ret;
				    break;
				case XML_RELAXNG_START:
				case XML_RELAXNG_PARAM:
				case XML_RELAXNG_EXCEPT:
				    xmlRngPErr(ctxt, P_Node, XML_RNGP_ATTRIBUTE_CONTENT, "attribute has invalid content\n", 0, 0);
				    break;
				case XML_RELAXNG_NOOP:
				    xmlRngPErr(ctxt, P_Node, XML_RNGP_ATTRIBUTE_NOOP, "RNG Internal error, noop found in attribute\n", 0, 0);
				    break;
			}
		}
		child = child->next;
	}
	if(child) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_ATTRIBUTE_CHILDREN, "attribute has multiple children\n", 0, 0);
	}
	ctxt->flags = old_flags;
	return ret;
}

/**
 * xmlRelaxNGParseExceptNameClass:
 * @ctxt:  a Relax-NG parser context
 * @node:  the except node
 * @attr:  1 if within an attribute, 0 if within an element
 *
 * parse the content of a RelaxNG nameClass node.
 *
 * Returns the definition pointer or NULL in case of error.
 */
static xmlRelaxNGDefinePtr xmlRelaxNGParseExceptNameClass(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node, int attr)
{
	xmlRelaxNGDefinePtr ret, cur, last = NULL;
	xmlNode * child;
	if(!IS_RELAXNG(P_Node, "except")) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_EXCEPT_MISSING, "Expecting an except node\n", 0, 0);
		return 0;
	}
	if(P_Node->next) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_EXCEPT_MULTIPLE, "exceptNameClass allows only a single except node\n", 0, 0);
	}
	if(P_Node->children == NULL) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_EXCEPT_EMPTY, "except has no content\n", 0, 0);
		return 0;
	}
	ret = xmlRelaxNGNewDefine(ctxt, P_Node);
	if(!ret)
		return 0;
	ret->type = XML_RELAXNG_EXCEPT;
	child = P_Node->children;
	while(child) {
		cur = xmlRelaxNGNewDefine(ctxt, child);
		if(!cur)
			break;
		cur->type = attr ? XML_RELAXNG_ATTRIBUTE : XML_RELAXNG_ELEMENT;
		if(xmlRelaxNGParseNameClass(ctxt, child, cur)) {
			if(last == NULL) {
				ret->content = cur;
			}
			else {
				last->next = cur;
			}
			last = cur;
		}
		child = child->next;
	}
	return ret;
}
/**
 * xmlRelaxNGParseNameClass:
 * @ctxt:  a Relax-NG parser context
 * @node:  the nameClass node
 * @def:  the current definition
 *
 * parse the content of a RelaxNG nameClass node.
 *
 * Returns the definition pointer or NULL in case of error.
 */
static xmlRelaxNGDefinePtr xmlRelaxNGParseNameClass(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node, xmlRelaxNGDefinePtr def)
{
	xmlRelaxNGDefinePtr tmp;
	xmlChar * val;
	xmlRelaxNGDefinePtr ret = def;
	if((IS_RELAXNG(P_Node, "name")) || (IS_RELAXNG(P_Node, "anyName")) || (IS_RELAXNG(P_Node, "nsName"))) {
		if((def->type != XML_RELAXNG_ELEMENT) && (def->type != XML_RELAXNG_ATTRIBUTE)) {
			ret = xmlRelaxNGNewDefine(ctxt, P_Node);
			if(!ret)
				return 0;
			ret->parent = def;
			if(ctxt->flags & XML_RELAXNG_IN_ATTRIBUTE)
				ret->type = XML_RELAXNG_ATTRIBUTE;
			else
				ret->type = XML_RELAXNG_ELEMENT;
		}
	}
	if(IS_RELAXNG(P_Node, "name")) {
		val = xmlNodeGetContent(P_Node);
		xmlRelaxNGNormExtSpace(val);
		if(xmlValidateNCName(val, 0)) {
			if(P_Node->parent)
				xmlRngPErr(ctxt, P_Node, XML_RNGP_ELEMENT_NAME, "Element %s name '%s' is not an NCName\n", P_Node->parent->name, val);
			else
				xmlRngPErr(ctxt, P_Node, XML_RNGP_ELEMENT_NAME, "name '%s' is not an NCName\n", val, 0);
		}
		ret->name = val;
		val = xmlGetProp(P_Node, BAD_CAST "ns");
		ret->ns = val;
		if((ctxt->flags & XML_RELAXNG_IN_ATTRIBUTE) && val && sstreq(val, "http://www.w3.org/2000/xmlns")) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_XML_NS, "Attribute with namespace '%s' is not allowed\n", val, 0);
		}
		if((ctxt->flags & XML_RELAXNG_IN_ATTRIBUTE) && val && (val[0] == 0) && sstreq(ret->name, "xmlns")) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_XMLNS_NAME, "Attribute with QName 'xmlns' is not allowed\n", val, 0);
		}
	}
	else if(IS_RELAXNG(P_Node, "anyName")) {
		ret->name = NULL;
		ret->ns = NULL;
		if(P_Node->children) {
			ret->nameClass = xmlRelaxNGParseExceptNameClass(ctxt, P_Node->children, (def->type == XML_RELAXNG_ATTRIBUTE));
		}
	}
	else if(IS_RELAXNG(P_Node, "nsName")) {
		ret->name = NULL;
		ret->ns = xmlGetProp(P_Node, BAD_CAST "ns");
		if(ret->ns == NULL) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_NSNAME_NO_NS, "nsName has no ns attribute\n", 0, 0);
		}
		if((ctxt->flags & XML_RELAXNG_IN_ATTRIBUTE) && ret->ns && sstreq(ret->ns, "http://www.w3.org/2000/xmlns")) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_XML_NS, "Attribute with namespace '%s' is not allowed\n", ret->ns, 0);
		}
		if(P_Node->children) {
			ret->nameClass = xmlRelaxNGParseExceptNameClass(ctxt, P_Node->children, (def->type == XML_RELAXNG_ATTRIBUTE));
		}
	}
	else if(IS_RELAXNG(P_Node, "choice")) {
		xmlNode * child;
		xmlRelaxNGDefinePtr last = NULL;
		ret = xmlRelaxNGNewDefine(ctxt, P_Node);
		if(!ret)
			return 0;
		ret->parent = def;
		ret->type = XML_RELAXNG_CHOICE;
		if(P_Node->children == NULL) {
			xmlRngPErr(ctxt, P_Node, XML_RNGP_CHOICE_EMPTY, "Element choice is empty\n", 0, 0);
		}
		else {
			child = P_Node->children;
			while(child) {
				tmp = xmlRelaxNGParseNameClass(ctxt, child, ret);
				if(tmp) {
					if(last == NULL) {
						last = ret->nameClass = tmp;
					}
					else {
						last->next = tmp;
						last = tmp;
					}
				}
				child = child->next;
			}
		}
	}
	else {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_CHOICE_CONTENT, "expecting name, anyName, nsName or choice : got %s\n", (P_Node == NULL ? (const xmlChar*)"nothing" : P_Node->name), 0);
		return 0;
	}
	if(ret != def) {
		if(def->nameClass == NULL) {
			def->nameClass = ret;
		}
		else {
			tmp = def->nameClass;
			while(tmp->next) {
				tmp = tmp->next;
			}
			tmp->next = ret;
		}
	}
	return ret;
}
/**
 * xmlRelaxNGParseElement:
 * @ctxt:  a Relax-NG parser context
 * @node:  the element node
 *
 * parse the content of a RelaxNG element node.
 *
 * Returns the definition pointer or NULL in case of error.
 */
static xmlRelaxNGDefinePtr xmlRelaxNGParseElement(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node)
{
	xmlRelaxNGDefinePtr ret, cur, last;
	xmlNode * child;
	const xmlChar * olddefine;

	ret = xmlRelaxNGNewDefine(ctxt, P_Node);
	if(!ret)
		return 0;
	ret->type = XML_RELAXNG_ELEMENT;
	ret->parent = ctxt->def;
	child = P_Node->children;
	if(child == NULL) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_ELEMENT_EMPTY, "xmlRelaxNGParseElement: element has no children\n", 0, 0);
		return ret;
	}
	cur = xmlRelaxNGParseNameClass(ctxt, child, ret);
	if(cur)
		child = child->next;

	if(child == NULL) {
		xmlRngPErr(ctxt, P_Node, XML_RNGP_ELEMENT_NO_CONTENT, "xmlRelaxNGParseElement: element has no content\n", 0, 0);
		return ret;
	}
	olddefine = ctxt->define;
	ctxt->define = NULL;
	last = NULL;
	while(child) {
		cur = xmlRelaxNGParsePattern(ctxt, child);
		if(cur) {
			cur->parent = ret;
			switch(cur->type) {
				case XML_RELAXNG_EMPTY:
				case XML_RELAXNG_NOT_ALLOWED:
				case XML_RELAXNG_TEXT:
				case XML_RELAXNG_ELEMENT:
				case XML_RELAXNG_DATATYPE:
				case XML_RELAXNG_VALUE:
				case XML_RELAXNG_LIST:
				case XML_RELAXNG_REF:
				case XML_RELAXNG_PARENTREF:
				case XML_RELAXNG_EXTERNALREF:
				case XML_RELAXNG_DEF:
				case XML_RELAXNG_ZEROORMORE:
				case XML_RELAXNG_ONEORMORE:
				case XML_RELAXNG_OPTIONAL:
				case XML_RELAXNG_CHOICE:
				case XML_RELAXNG_GROUP:
				case XML_RELAXNG_INTERLEAVE:
				    if(last == NULL) {
					    ret->content = last = cur;
				    }
				    else {
					    if((last->type == XML_RELAXNG_ELEMENT) && (ret->content == last)) {
						    ret->content = xmlRelaxNGNewDefine(ctxt, P_Node);
						    if(ret->content) {
							    ret->content->type = XML_RELAXNG_GROUP;
							    ret->content->content = last;
						    }
						    else {
							    ret->content = last;
						    }
					    }
					    last->next = cur;
					    last = cur;
				    }
				    break;
				case XML_RELAXNG_ATTRIBUTE:
				    cur->next = ret->attrs;
				    ret->attrs = cur;
				    break;
				case XML_RELAXNG_START:
				    xmlRngPErr(ctxt, P_Node, XML_RNGP_ELEMENT_CONTENT, "RNG Internal error, start found in element\n", 0, 0);
				    break;
				case XML_RELAXNG_PARAM:
				    xmlRngPErr(ctxt, P_Node, XML_RNGP_ELEMENT_CONTENT, "RNG Internal error, param found in element\n", 0, 0);
				    break;
				case XML_RELAXNG_EXCEPT:
				    xmlRngPErr(ctxt, P_Node, XML_RNGP_ELEMENT_CONTENT, "RNG Internal error, except found in element\n", 0, 0);
				    break;
				case XML_RELAXNG_NOOP:
				    xmlRngPErr(ctxt, P_Node, XML_RNGP_ELEMENT_CONTENT, "RNG Internal error, noop found in element\n", 0, 0);
				    break;
			}
		}
		child = child->next;
	}
	ctxt->define = olddefine;
	return ret;
}

/**
 * xmlRelaxNGParsePatterns:
 * @ctxt:  a Relax-NG parser context
 * @nodes:  list of nodes
 * @group:  use an implicit <group> for elements
 *
 * parse the content of a RelaxNG start node.
 *
 * Returns the definition pointer or NULL in case of error.
 */
static xmlRelaxNGDefinePtr xmlRelaxNGParsePatterns(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * nodes, int group)
{
	xmlRelaxNGDefine * def = NULL;
	xmlRelaxNGDefine * last = NULL;
	xmlRelaxNGDefine * parent = ctxt->def;
	while(nodes) {
		if(IS_RELAXNG(nodes, "element")) {
			xmlRelaxNGDefine * cur = xmlRelaxNGParseElement(ctxt, nodes);
			if(def == NULL) {
				def = last = cur;
			}
			else {
				if((group == 1) && (def->type == XML_RELAXNG_ELEMENT) && (def == last)) {
					def = xmlRelaxNGNewDefine(ctxt, nodes);
					def->type = XML_RELAXNG_GROUP;
					def->content = last;
				}
				last->next = cur;
				last = cur;
			}
			cur->parent = parent;
		}
		else {
			xmlRelaxNGDefine * cur = xmlRelaxNGParsePattern(ctxt, nodes);
			if(cur) {
				if(def == NULL) {
					def = last = cur;
				}
				else {
					last->next = cur;
					last = cur;
				}
			}
		}
		nodes = nodes->next;
	}
	return (def);
}

/**
 * xmlRelaxNGParseStart:
 * @ctxt:  a Relax-NG parser context
 * @nodes:  start children nodes
 *
 * parse the content of a RelaxNG start node.
 *
 * Returns 0 in case of success, -1 in case of error
 */
static int xmlRelaxNGParseStart(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * nodes)
{
	int ret = 0;
	xmlRelaxNGDefinePtr def = NULL, last;
	if(nodes == NULL) {
		xmlRngPErr(ctxt, nodes, XML_RNGP_START_EMPTY, "start has no children\n", 0, 0);
		return -1;
	}
	if(IS_RELAXNG(nodes, "empty")) {
		def = xmlRelaxNGNewDefine(ctxt, nodes);
		if(def == NULL)
			return -1;
		def->type = XML_RELAXNG_EMPTY;
		if(nodes->children)
			xmlRngPErr(ctxt, nodes, XML_RNGP_EMPTY_CONTENT, "element empty is not empty\n", 0, 0);
	}
	else if(IS_RELAXNG(nodes, "notAllowed")) {
		def = xmlRelaxNGNewDefine(ctxt, nodes);
		if(def == NULL)
			return -1;
		def->type = XML_RELAXNG_NOT_ALLOWED;
		if(nodes->children)
			xmlRngPErr(ctxt, nodes, XML_RNGP_NOTALLOWED_NOT_EMPTY, "element notAllowed is not empty\n", 0, 0);
	}
	else {
		def = xmlRelaxNGParsePatterns(ctxt, nodes, 1);
	}
	if(ctxt->grammar->start) {
		last = ctxt->grammar->start;
		while(last->next)
			last = last->next;
		last->next = def;
	}
	else {
		ctxt->grammar->start = def;
	}
	nodes = nodes->next;
	if(nodes) {
		xmlRngPErr(ctxt, nodes, XML_RNGP_START_CONTENT, "start more than one children\n", 0, 0);
		return -1;
	}
	else
		return ret;
}

/**
 * xmlRelaxNGParseGrammarContent:
 * @ctxt:  a Relax-NG parser context
 * @nodes:  grammar children nodes
 *
 * parse the content of a RelaxNG grammar node.
 *
 * Returns 0 in case of success, -1 in case of error
 */
static int xmlRelaxNGParseGrammarContent(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * nodes)
{
	int ret = 0;
	if(nodes == NULL) {
		xmlRngPErr(ctxt, nodes, XML_RNGP_GRAMMAR_EMPTY, "grammar has no children\n", 0, 0);
		return -1;
	}
	else {
		while(nodes) {
			if(IS_RELAXNG(nodes, "start")) {
				if(nodes->children == NULL) {
					xmlRngPErr(ctxt, nodes, XML_RNGP_START_EMPTY, "start has no children\n", 0, 0);
				}
				else {
					int tmp = xmlRelaxNGParseStart(ctxt, nodes->children);
					if(tmp != 0)
						ret = -1;
				}
			}
			else if(IS_RELAXNG(nodes, "define")) {
				int tmp = xmlRelaxNGParseDefine(ctxt, nodes);
				if(tmp != 0)
					ret = -1;
			}
			else if(IS_RELAXNG(nodes, "include")) {
				int tmp = xmlRelaxNGParseInclude(ctxt, nodes);
				if(tmp != 0)
					ret = -1;
			}
			else {
				xmlRngPErr(ctxt, nodes, XML_RNGP_GRAMMAR_CONTENT, "grammar has unexpected child %s\n", nodes->name, 0);
				ret = -1;
			}
			nodes = nodes->next;
		}
		return ret;
	}
}

/**
 * xmlRelaxNGCheckReference:
 * @ref:  the ref
 * @ctxt:  a Relax-NG parser context
 * @name:  the name associated to the defines
 *
 * Applies the 4.17. combine attribute rule for all the define
 * element of a given grammar using the same name.
 */
static void xmlRelaxNGCheckReference(xmlRelaxNGDefinePtr ref, xmlRelaxNGParserCtxtPtr ctxt, const xmlChar * name)
{
	/*
	 * Those rules don't apply to imported ref from xmlRelaxNGParseImportRef
	 */
	if(!(ref->dflags & IS_EXTERNAL_REF)) {
		xmlRelaxNGGrammar * grammar = ctxt->grammar;
		if(grammar == NULL) {
			xmlRngPErr(ctxt, ref->P_Node, XML_ERR_INTERNAL_ERROR, "Internal error: no grammar in CheckReference %s\n", name, 0);
		}
		else if(ref->content) {
			xmlRngPErr(ctxt, ref->P_Node, XML_ERR_INTERNAL_ERROR, "Internal error: reference has content in CheckReference %s\n", name, 0);
		}
		else if(grammar->defs) {
			xmlRelaxNGDefine * def = (xmlRelaxNGDefine *)xmlHashLookup(grammar->defs, name);
			if(def) {
				for(xmlRelaxNGDefine * cur = ref; cur; cur = cur->nextHash)
					cur->content = def;
			}
			else {
				xmlRngPErr(ctxt, ref->P_Node, XML_RNGP_REF_NO_DEF, "Reference %s has no matching definition\n", name, 0);
			}
		}
		else {
			xmlRngPErr(ctxt, ref->P_Node, XML_RNGP_REF_NO_DEF, "Reference %s has no matching definition\n", name, 0);
		}
	}
}
/**
 * xmlRelaxNGCheckCombine:
 * @define:  the define(s) list
 * @ctxt:  a Relax-NG parser context
 * @name:  the name associated to the defines
 *
 * Applies the 4.17. combine attribute rule for all the define
 * element of a given grammar using the same name.
 */
static void xmlRelaxNGCheckCombine(xmlRelaxNGDefinePtr define, xmlRelaxNGParserCtxtPtr ctxt, const xmlChar * name)
{
	int choiceOrInterleave = -1;
	int missing = 0;
	xmlRelaxNGDefine * last;
	xmlRelaxNGDefine * tmp;
	xmlRelaxNGDefine * tmp2;
	if(define->nextHash) {
		xmlRelaxNGDefine * cur = define;
		while(cur) {
			xmlChar * combine = xmlGetProp(cur->P_Node, BAD_CAST "combine");
			if(combine) {
				if(sstreq(combine, "choice")) {
					if(choiceOrInterleave == -1)
						choiceOrInterleave = 1;
					else if(choiceOrInterleave == 0)
						xmlRngPErr(ctxt, define->P_Node, XML_RNGP_DEF_CHOICE_AND_INTERLEAVE, "Defines for %s use both 'choice' and 'interleave'\n", name, 0);
				}
				else if(sstreq(combine, "interleave")) {
					if(choiceOrInterleave == -1)
						choiceOrInterleave = 0;
					else if(choiceOrInterleave == 1)
						xmlRngPErr(ctxt, define->P_Node, XML_RNGP_DEF_CHOICE_AND_INTERLEAVE, "Defines for %s use both 'choice' and 'interleave'\n", name, 0);
				}
				else
					xmlRngPErr(ctxt, define->P_Node, XML_RNGP_UNKNOWN_COMBINE, "Defines for %s use unknown combine value '%s''\n", name, combine);
				SAlloc::F(combine);
			}
			else {
				if(missing == 0)
					missing = 1;
				else
					xmlRngPErr(ctxt, define->P_Node, XML_RNGP_NEED_COMBINE, "Some defines for %s needs the combine attribute\n", name, 0);
			}
			cur = cur->nextHash;
		}
#ifdef DEBUG
		xmlGenericError(0, "xmlRelaxNGCheckCombine(): merging %s defines: %d\n", name, choiceOrInterleave);
#endif
		if(choiceOrInterleave == -1)
			choiceOrInterleave = 0;
		cur = xmlRelaxNGNewDefine(ctxt, define->P_Node);
		if(cur) {
			cur->type = (choiceOrInterleave == 0) ? XML_RELAXNG_INTERLEAVE : XML_RELAXNG_CHOICE;
			tmp = define;
			last = NULL;
			while(tmp) {
				if(tmp->content) {
					if(tmp->content->next) {
						/*
						 * we need first to create a wrapper.
						 */
						tmp2 = xmlRelaxNGNewDefine(ctxt, tmp->content->P_Node);
						if(tmp2 == NULL)
							break;
						tmp2->type = XML_RELAXNG_GROUP;
						tmp2->content = tmp->content;
					}
					else {
						tmp2 = tmp->content;
					}
					if(last == NULL) {
						cur->content = tmp2;
					}
					else {
						last->next = tmp2;
					}
					last = tmp2;
				}
				tmp->content = cur;
				tmp = tmp->nextHash;
			}
			define->content = cur;
			if(choiceOrInterleave == 0) {
				SETIFZ(ctxt->interleaves, xmlHashCreate(10));
				if(ctxt->interleaves == NULL) {
					xmlRngPErr(ctxt, define->P_Node, XML_RNGP_INTERLEAVE_CREATE_FAILED, "Failed to create interleaves hash table\n", 0, 0);
				}
				else {
					char tmpname[32];
					snprintf(tmpname, 32, "interleave%d", ctxt->nbInterleaves++);
					if(xmlHashAddEntry(ctxt->interleaves, BAD_CAST tmpname, cur) < 0)
						xmlRngPErr(ctxt, define->P_Node, XML_RNGP_INTERLEAVE_CREATE_FAILED, "Failed to add %s to hash table\n", (const xmlChar*)tmpname, 0);
				}
			}
		}
	}
}
/**
 * xmlRelaxNGCombineStart:
 * @ctxt:  a Relax-NG parser context
 * @grammar:  the grammar
 *
 * Applies the 4.17. combine rule for all the start
 * element of a given grammar.
 */
static void xmlRelaxNGCombineStart(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGGrammarPtr grammar)
{
	xmlChar * combine;
	int choiceOrInterleave = -1;
	int missing = 0;
	xmlRelaxNGDefinePtr cur;
	xmlRelaxNGDefinePtr starts = grammar->start;
	if((starts == NULL) || (starts->next == NULL))
		return;
	cur = starts;
	while(cur) {
		if(!cur->P_Node || !cur->P_Node->parent || !sstreq(cur->P_Node->parent->name, "start")) {
			combine = NULL;
			xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_START_MISSING, "Internal error: start element not found\n", 0, 0);
		}
		else {
			combine = xmlGetProp(cur->P_Node->parent, BAD_CAST "combine");
		}
		if(combine) {
			if(sstreq(combine, "choice")) {
				if(choiceOrInterleave == -1)
					choiceOrInterleave = 1;
				else if(choiceOrInterleave == 0) {
					xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_START_CHOICE_AND_INTERLEAVE, "<start> use both 'choice' and 'interleave'\n", 0, 0);
				}
			}
			else if(sstreq(combine, "interleave")) {
				if(choiceOrInterleave == -1)
					choiceOrInterleave = 0;
				else if(choiceOrInterleave == 1) {
					xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_START_CHOICE_AND_INTERLEAVE, "<start> use both 'choice' and 'interleave'\n", 0, 0);
				}
			}
			else
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_UNKNOWN_COMBINE, "<start> uses unknown combine value '%s''\n", combine, 0);
			SAlloc::F(combine);
		}
		else {
			if(missing == 0)
				missing = 1;
			else
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_NEED_COMBINE, "Some <start> element miss the combine attribute\n", 0, 0);
		}
		cur = cur->next;
	}
#ifdef DEBUG
	xmlGenericError(0, "xmlRelaxNGCombineStart(): merging <start>: %d\n", choiceOrInterleave);
#endif
	if(choiceOrInterleave == -1)
		choiceOrInterleave = 0;
	cur = xmlRelaxNGNewDefine(ctxt, starts->P_Node);
	if(!cur)
		return;
	cur->type = (choiceOrInterleave == 0) ? XML_RELAXNG_INTERLEAVE : XML_RELAXNG_CHOICE;
	cur->content = grammar->start;
	grammar->start = cur;
	if(choiceOrInterleave == 0) {
		SETIFZ(ctxt->interleaves, xmlHashCreate(10));
		if(ctxt->interleaves == NULL) {
			xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_INTERLEAVE_CREATE_FAILED, "Failed to create interleaves hash table\n", 0, 0);
		}
		else {
			char tmpname[32];
			snprintf(tmpname, 32, "interleave%d", ctxt->nbInterleaves++);
			if(xmlHashAddEntry(ctxt->interleaves, BAD_CAST tmpname, cur) < 0) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_INTERLEAVE_CREATE_FAILED, "Failed to add %s to hash table\n", (const xmlChar*)tmpname, 0);
			}
		}
	}
}

/**
 * xmlRelaxNGCheckCycles:
 * @ctxt:  a Relax-NG parser context
 * @nodes:  grammar children nodes
 * @depth:  the counter
 *
 * Check for cycles.
 *
 * Returns 0 if check passed, and -1 in case of error
 */
static int xmlRelaxNGCheckCycles(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGDefinePtr cur, int depth)
{
	int ret = 0;
	while((ret == 0) && cur) {
		if((cur->type == XML_RELAXNG_REF) || (cur->type == XML_RELAXNG_PARENTREF)) {
			if(cur->depth == -1) {
				cur->depth = depth;
				ret = xmlRelaxNGCheckCycles(ctxt, cur->content, depth);
				cur->depth = -2;
			}
			else if(depth == cur->depth) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_REF_CYCLE, "Detected a cycle in %s references\n", cur->name, 0);
				return -1;
			}
		}
		else if(cur->type == XML_RELAXNG_ELEMENT) {
			ret = xmlRelaxNGCheckCycles(ctxt, cur->content, depth + 1);
		}
		else {
			ret = xmlRelaxNGCheckCycles(ctxt, cur->content, depth);
		}
		cur = cur->next;
	}
	return ret;
}

/**
 * xmlRelaxNGTryUnlink:
 * @ctxt:  a Relax-NG parser context
 * @cur:  the definition to unlink
 * @parent:  the parent definition
 * @prev:  the previous sibling definition
 *
 * Try to unlink a definition. If not possble make it a NOOP
 *
 * Returns the new prev definition
 */
static xmlRelaxNGDefinePtr xmlRelaxNGTryUnlink(xmlRelaxNGParserCtxtPtr ctxt ATTRIBUTE_UNUSED, xmlRelaxNGDefinePtr cur, xmlRelaxNGDefinePtr parent, xmlRelaxNGDefinePtr prev)
{
	if(prev) {
		prev->next = cur->next;
	}
	else {
		if(parent) {
			if(parent->content == cur)
				parent->content = cur->next;
			else if(parent->attrs == cur)
				parent->attrs = cur->next;
			else if(parent->nameClass == cur)
				parent->nameClass = cur->next;
		}
		else {
			cur->type = XML_RELAXNG_NOOP;
			prev = cur;
		}
	}
	return (prev);
}

/**
 * xmlRelaxNGSimplify:
 * @ctxt:  a Relax-NG parser context
 * @nodes:  grammar children nodes
 *
 * Check for simplification of empty and notAllowed
 */
static void xmlRelaxNGSimplify(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGDefinePtr cur, xmlRelaxNGDefinePtr parent)
{
	xmlRelaxNGDefine * prev = NULL;
	while(cur) {
		if(oneof2(cur->type, XML_RELAXNG_REF, XML_RELAXNG_PARENTREF)) {
			if(cur->depth != -3) {
				cur->depth = -3;
				xmlRelaxNGSimplify(ctxt, cur->content, cur);
			}
		}
		else if(cur->type == XML_RELAXNG_NOT_ALLOWED) {
			cur->parent = parent;
			if(parent && oneof6(parent->type, XML_RELAXNG_ATTRIBUTE, XML_RELAXNG_LIST, XML_RELAXNG_GROUP, XML_RELAXNG_INTERLEAVE, XML_RELAXNG_ONEORMORE, XML_RELAXNG_ZEROORMORE)) {
				parent->type = XML_RELAXNG_NOT_ALLOWED;
				break;
			}
			if(parent && parent->type == XML_RELAXNG_CHOICE) {
				prev = xmlRelaxNGTryUnlink(ctxt, cur, parent, prev);
			}
			else
				prev = cur;
		}
		else if(cur->type == XML_RELAXNG_EMPTY) {
			cur->parent = parent;
			if(parent && ((parent->type == XML_RELAXNG_ONEORMORE) || (parent->type == XML_RELAXNG_ZEROORMORE))) {
				parent->type = XML_RELAXNG_EMPTY;
				break;
			}
			if(parent && ((parent->type == XML_RELAXNG_GROUP) || (parent->type == XML_RELAXNG_INTERLEAVE))) {
				prev = xmlRelaxNGTryUnlink(ctxt, cur, parent, prev);
			}
			else
				prev = cur;
		}
		else {
			cur->parent = parent;
			if(cur->content)
				xmlRelaxNGSimplify(ctxt, cur->content, cur);
			if((cur->type != XML_RELAXNG_VALUE) && cur->attrs)
				xmlRelaxNGSimplify(ctxt, cur->attrs, cur);
			if(cur->nameClass)
				xmlRelaxNGSimplify(ctxt, cur->nameClass, cur);
			/*
			 * On Elements, try to move attribute only generating rules on
			 * the attrs rules.
			 */
			if(cur->type == XML_RELAXNG_ELEMENT) {
				int attronly;
				xmlRelaxNGDefinePtr tmp, pre;
				while(cur->content) {
					attronly = xmlRelaxNGGenerateAttributes(ctxt, cur->content);
					if(attronly == 1) {
						/*
						 * migrate cur->content to attrs
						 */
						tmp = cur->content;
						cur->content = tmp->next;
						tmp->next = cur->attrs;
						cur->attrs = tmp;
					}
					else {
						/*
						 * cur->content can generate elements or text
						 */
						break;
					}
				}
				pre = cur->content;
				while(pre && pre->next) {
					tmp = pre->next;
					attronly = xmlRelaxNGGenerateAttributes(ctxt, tmp);
					if(attronly == 1) {
						/*
						 * migrate tmp to attrs
						 */
						pre->next = tmp->next;
						tmp->next = cur->attrs;
						cur->attrs = tmp;
					}
					else {
						pre = tmp;
					}
				}
			}
			/*
			 * This may result in a simplification
			 */
			if(oneof2(cur->type, XML_RELAXNG_GROUP, XML_RELAXNG_INTERLEAVE)) {
				if(cur->content == NULL)
					cur->type = XML_RELAXNG_EMPTY;
				else if(cur->content->next == NULL) {
					if((parent == NULL) && (prev == NULL)) {
						cur->type = XML_RELAXNG_NOOP;
					}
					else if(prev == NULL) {
						parent->content = cur->content;
						cur->content->next = cur->next;
						cur = cur->content;
					}
					else {
						cur->content->next = cur->next;
						prev->next = cur->content;
						cur = cur->content;
					}
				}
			}
			/*
			 * the current node may have been transformed back
			 */
			if((cur->type == XML_RELAXNG_EXCEPT) && cur->content && (cur->content->type == XML_RELAXNG_NOT_ALLOWED)) {
				prev = xmlRelaxNGTryUnlink(ctxt, cur, parent, prev);
			}
			else if(cur->type == XML_RELAXNG_NOT_ALLOWED) {
				if(parent && oneof6(parent->type, XML_RELAXNG_ATTRIBUTE, XML_RELAXNG_LIST, XML_RELAXNG_GROUP, XML_RELAXNG_INTERLEAVE, XML_RELAXNG_ONEORMORE, XML_RELAXNG_ZEROORMORE)) {
					parent->type = XML_RELAXNG_NOT_ALLOWED;
					break;
				}
				if(parent && parent->type == XML_RELAXNG_CHOICE) {
					prev = xmlRelaxNGTryUnlink(ctxt, cur, parent, prev);
				}
				else
					prev = cur;
			}
			else if(cur->type == XML_RELAXNG_EMPTY) {
				if(parent && oneof2(parent->type, XML_RELAXNG_ONEORMORE, XML_RELAXNG_ZEROORMORE)) {
					parent->type = XML_RELAXNG_EMPTY;
					break;
				}
				if(parent && oneof3(parent->type, XML_RELAXNG_GROUP, XML_RELAXNG_INTERLEAVE, XML_RELAXNG_CHOICE)) {
					prev = xmlRelaxNGTryUnlink(ctxt, cur, parent, prev);
				}
				else
					prev = cur;
			}
			else {
				prev = cur;
			}
		}
		cur = cur->next;
	}
}

/**
 * xmlRelaxNGGroupContentType:
 * @ct1:  the first content type
 * @ct2:  the second content type
 *
 * Try to group 2 content types
 *
 * Returns the content type
 */
static xmlRelaxNGContentType xmlRelaxNGGroupContentType(xmlRelaxNGContentType ct1, xmlRelaxNGContentType ct2)
{
	if((ct1 == XML_RELAXNG_CONTENT_ERROR) || (ct2 == XML_RELAXNG_CONTENT_ERROR))
		return (XML_RELAXNG_CONTENT_ERROR);
	if(ct1 == XML_RELAXNG_CONTENT_EMPTY)
		return (ct2);
	if(ct2 == XML_RELAXNG_CONTENT_EMPTY)
		return (ct1);
	if((ct1 == XML_RELAXNG_CONTENT_COMPLEX) && (ct2 == XML_RELAXNG_CONTENT_COMPLEX))
		return (XML_RELAXNG_CONTENT_COMPLEX);
	return (XML_RELAXNG_CONTENT_ERROR);
}

/**
 * xmlRelaxNGMaxContentType:
 * @ct1:  the first content type
 * @ct2:  the second content type
 *
 * Compute the max content-type
 *
 * Returns the content type
 */
static xmlRelaxNGContentType xmlRelaxNGMaxContentType(xmlRelaxNGContentType ct1,
    xmlRelaxNGContentType ct2)
{
	if((ct1 == XML_RELAXNG_CONTENT_ERROR) ||
	    (ct2 == XML_RELAXNG_CONTENT_ERROR))
		return (XML_RELAXNG_CONTENT_ERROR);
	if((ct1 == XML_RELAXNG_CONTENT_SIMPLE) ||
	    (ct2 == XML_RELAXNG_CONTENT_SIMPLE))
		return (XML_RELAXNG_CONTENT_SIMPLE);
	if((ct1 == XML_RELAXNG_CONTENT_COMPLEX) ||
	    (ct2 == XML_RELAXNG_CONTENT_COMPLEX))
		return (XML_RELAXNG_CONTENT_COMPLEX);
	return (XML_RELAXNG_CONTENT_EMPTY);
}

/**
 * xmlRelaxNGCheckRules:
 * @ctxt:  a Relax-NG parser context
 * @cur:  the current definition
 * @flags:  some accumulated flags
 * @ptype:  the parent type
 *
 * Check for rules in section 7.1 and 7.2
 *
 * Returns the content type of @cur
 */
static xmlRelaxNGContentType xmlRelaxNGCheckRules(xmlRelaxNGParserCtxtPtr ctxt,
    xmlRelaxNGDefinePtr cur, int flags,
    xmlRelaxNGType ptype)
{
	int nflags;
	xmlRelaxNGContentType ret, tmp, val = XML_RELAXNG_CONTENT_EMPTY;

	while(cur) {
		ret = XML_RELAXNG_CONTENT_EMPTY;
		if((cur->type == XML_RELAXNG_REF) ||
		    (cur->type == XML_RELAXNG_PARENTREF)) {
			/*
			 * This should actually be caught by list//element(ref) at the
			 * element boundaries, c.f. Bug #159968 local refs are dropped
			 * in step 4.19.
			 */
#if 0
			if(flags & XML_RELAXNG_IN_LIST) {
				xmlRngPErr(ctxt, cur->node, XML_RNGP_PAT_LIST_REF, "Found forbidden pattern list//ref\n", 0, 0);
			}
#endif
			if(flags & XML_RELAXNG_IN_DATAEXCEPT) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_DATA_EXCEPT_REF, "Found forbidden pattern data/except//ref\n", 0, 0);
			}
			if(cur->content == NULL) {
				if(cur->type == XML_RELAXNG_PARENTREF)
					xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_REF_NO_DEF, "Internal found no define for parent refs\n", 0, 0);
				else
					xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_REF_NO_DEF, "Internal found no define for ref %s\n", (cur->name ? cur->name : BAD_CAST "null"), 0);
			}
			if(cur->depth > -4) {
				cur->depth = -4;
				ret = xmlRelaxNGCheckRules(ctxt, cur->content,
				    flags, cur->type);
				cur->depth = ret - 15;
			}
			else if(cur->depth == -4) {
				ret = XML_RELAXNG_CONTENT_COMPLEX;
			}
			else {
				ret = (xmlRelaxNGContentType)(cur->depth + 15);
			}
		}
		else if(cur->type == XML_RELAXNG_ELEMENT) {
			/*
			 * The 7.3 Attribute derivation rule for groups is plugged there
			 */
			xmlRelaxNGCheckGroupAttrs(ctxt, cur);
			if(flags & XML_RELAXNG_IN_DATAEXCEPT) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_DATA_EXCEPT_ELEM, "Found forbidden pattern data/except//element(ref)\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_LIST) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_LIST_ELEM, "Found forbidden pattern list//element(ref)\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_ATTRIBUTE) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_ATTR_ELEM, "Found forbidden pattern attribute//element(ref)\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_ATTRIBUTE) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_ATTR_ELEM, "Found forbidden pattern attribute//element(ref)\n", 0, 0);
			}
			/*
			 * reset since in the simple form elements are only child
			 * of grammar/define
			 */
			nflags = 0;
			ret =
			    xmlRelaxNGCheckRules(ctxt, cur->attrs, nflags, cur->type);
			if(ret != XML_RELAXNG_CONTENT_EMPTY) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_ELEM_CONTENT_EMPTY, "Element %s attributes have a content type error\n", cur->name, 0);
			}
			ret = xmlRelaxNGCheckRules(ctxt, cur->content, nflags, cur->type);
			if(ret == XML_RELAXNG_CONTENT_ERROR) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_ELEM_CONTENT_ERROR, "Element %s has a content type error\n", cur->name, 0);
			}
			else {
				ret = XML_RELAXNG_CONTENT_COMPLEX;
			}
		}
		else if(cur->type == XML_RELAXNG_ATTRIBUTE) {
			if(flags & XML_RELAXNG_IN_ATTRIBUTE) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_ATTR_ATTR, "Found forbidden pattern attribute//attribute\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_LIST) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_LIST_ATTR, "Found forbidden pattern list//attribute\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_OOMGROUP) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_ONEMORE_GROUP_ATTR, "Found forbidden pattern oneOrMore//group//attribute\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_OOMINTERLEAVE) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_ONEMORE_INTERLEAVE_ATTR, "Found forbidden pattern oneOrMore//interleave//attribute\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_DATAEXCEPT) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_DATA_EXCEPT_ATTR, "Found forbidden pattern data/except//attribute\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_START) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_START_ATTR, "Found forbidden pattern start//attribute\n", 0, 0);
			}
			if((!(flags & XML_RELAXNG_IN_ONEORMORE))
			    && (cur->name == NULL)) {
				if(cur->ns == NULL) {
					xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_ANYNAME_ATTR_ANCESTOR, "Found anyName attribute without oneOrMore ancestor\n", 0, 0);
				}
				else {
					xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_NSNAME_ATTR_ANCESTOR, "Found nsName attribute without oneOrMore ancestor\n", 0, 0);
				}
			}
			nflags = flags | XML_RELAXNG_IN_ATTRIBUTE;
			xmlRelaxNGCheckRules(ctxt, cur->content, nflags, cur->type);
			ret = XML_RELAXNG_CONTENT_EMPTY;
		}
		else if((cur->type == XML_RELAXNG_ONEORMORE) || (cur->type == XML_RELAXNG_ZEROORMORE)) {
			if(flags & XML_RELAXNG_IN_DATAEXCEPT) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_DATA_EXCEPT_ONEMORE, "Found forbidden pattern data/except//oneOrMore\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_START) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_START_ONEMORE, "Found forbidden pattern start//oneOrMore\n", 0, 0);
			}
			nflags = flags | XML_RELAXNG_IN_ONEORMORE;
			ret = xmlRelaxNGCheckRules(ctxt, cur->content, nflags, cur->type);
			ret = xmlRelaxNGGroupContentType(ret, ret);
		}
		else if(cur->type == XML_RELAXNG_LIST) {
			if(flags & XML_RELAXNG_IN_LIST) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_LIST_LIST, "Found forbidden pattern list//list\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_DATAEXCEPT) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_DATA_EXCEPT_LIST, "Found forbidden pattern data/except//list\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_START) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_START_LIST, "Found forbidden pattern start//list\n", 0, 0);
			}
			nflags = flags | XML_RELAXNG_IN_LIST;
			ret = xmlRelaxNGCheckRules(ctxt, cur->content, nflags, cur->type);
		}
		else if(cur->type == XML_RELAXNG_GROUP) {
			if(flags & XML_RELAXNG_IN_DATAEXCEPT) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_DATA_EXCEPT_GROUP, "Found forbidden pattern data/except//group\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_START) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_START_GROUP, "Found forbidden pattern start//group\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_ONEORMORE)
				nflags = flags | XML_RELAXNG_IN_OOMGROUP;
			else
				nflags = flags;
			ret =
			    xmlRelaxNGCheckRules(ctxt, cur->content, nflags,
			    cur->type);
			/*
			 * The 7.3 Attribute derivation rule for groups is plugged there
			 */
			xmlRelaxNGCheckGroupAttrs(ctxt, cur);
		}
		else if(cur->type == XML_RELAXNG_INTERLEAVE) {
			if(flags & XML_RELAXNG_IN_LIST) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_LIST_INTERLEAVE, "Found forbidden pattern list//interleave\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_DATAEXCEPT) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_DATA_EXCEPT_INTERLEAVE, "Found forbidden pattern data/except//interleave\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_START) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_DATA_EXCEPT_INTERLEAVE, "Found forbidden pattern start//interleave\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_ONEORMORE)
				nflags = flags | XML_RELAXNG_IN_OOMINTERLEAVE;
			else
				nflags = flags;
			ret = xmlRelaxNGCheckRules(ctxt, cur->content, nflags, cur->type);
		}
		else if(cur->type == XML_RELAXNG_EXCEPT) {
			if(cur->parent && (cur->parent->type == XML_RELAXNG_DATATYPE))
				nflags = flags | XML_RELAXNG_IN_DATAEXCEPT;
			else
				nflags = flags;
			ret = xmlRelaxNGCheckRules(ctxt, cur->content, nflags, cur->type);
		}
		else if(cur->type == XML_RELAXNG_DATATYPE) {
			if(flags & XML_RELAXNG_IN_START) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_START_DATA, "Found forbidden pattern start//data\n", 0, 0);
			}
			xmlRelaxNGCheckRules(ctxt, cur->content, flags, cur->type);
			ret = XML_RELAXNG_CONTENT_SIMPLE;
		}
		else if(cur->type == XML_RELAXNG_VALUE) {
			if(flags & XML_RELAXNG_IN_START) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_START_VALUE, "Found forbidden pattern start//value\n", 0, 0);
			}
			xmlRelaxNGCheckRules(ctxt, cur->content, flags, cur->type);
			ret = XML_RELAXNG_CONTENT_SIMPLE;
		}
		else if(cur->type == XML_RELAXNG_TEXT) {
			if(flags & XML_RELAXNG_IN_LIST) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_LIST_TEXT, "Found forbidden pattern list//text\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_DATAEXCEPT) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_DATA_EXCEPT_TEXT, "Found forbidden pattern data/except//text\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_START) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_START_TEXT, "Found forbidden pattern start//text\n", 0, 0);
			}
			ret = XML_RELAXNG_CONTENT_COMPLEX;
		}
		else if(cur->type == XML_RELAXNG_EMPTY) {
			if(flags & XML_RELAXNG_IN_DATAEXCEPT) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_DATA_EXCEPT_EMPTY, "Found forbidden pattern data/except//empty\n", 0, 0);
			}
			if(flags & XML_RELAXNG_IN_START) {
				xmlRngPErr(ctxt, cur->P_Node, XML_RNGP_PAT_START_EMPTY, "Found forbidden pattern start//empty\n", 0, 0);
			}
			ret = XML_RELAXNG_CONTENT_EMPTY;
		}
		else if(cur->type == XML_RELAXNG_CHOICE) {
			xmlRelaxNGCheckChoiceDeterminism(ctxt, cur);
			ret =
			    xmlRelaxNGCheckRules(ctxt, cur->content, flags, cur->type);
		}
		else {
			ret =
			    xmlRelaxNGCheckRules(ctxt, cur->content, flags, cur->type);
		}
		cur = cur->next;
		if(ptype == XML_RELAXNG_GROUP) {
			val = xmlRelaxNGGroupContentType(val, ret);
		}
		else if(ptype == XML_RELAXNG_INTERLEAVE) {
			/*
			 * @todo scan complain that tmp is never used, seems on purpose
			 *       need double-checking
			 */
			tmp = xmlRelaxNGGroupContentType(val, ret);
			if(tmp != XML_RELAXNG_CONTENT_ERROR)
				tmp = xmlRelaxNGMaxContentType(val, ret);
		}
		else if(ptype == XML_RELAXNG_CHOICE) {
			val = xmlRelaxNGMaxContentType(val, ret);
		}
		else if(ptype == XML_RELAXNG_LIST) {
			val = XML_RELAXNG_CONTENT_SIMPLE;
		}
		else if(ptype == XML_RELAXNG_EXCEPT) {
			if(ret == XML_RELAXNG_CONTENT_ERROR)
				val = XML_RELAXNG_CONTENT_ERROR;
			else
				val = XML_RELAXNG_CONTENT_SIMPLE;
		}
		else {
			val = xmlRelaxNGGroupContentType(val, ret);
		}
	}
	return (val);
}

/**
 * xmlRelaxNGParseGrammar:
 * @ctxt:  a Relax-NG parser context
 * @nodes:  grammar children nodes
 *
 * parse a Relax-NG <grammar> node
 *
 * Returns the internal xmlRelaxNGGrammarPtr built or
 *         NULL in case of error
 */
static xmlRelaxNGGrammarPtr xmlRelaxNGParseGrammar(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * nodes)
{
	xmlRelaxNGGrammarPtr ret, tmp, old;
#ifdef DEBUG_GRAMMAR
	xmlGenericError(0, "Parsing a new grammar\n");
#endif
	ret = xmlRelaxNGNewGrammar(ctxt);
	if(!ret)
		return 0;
	/*
	 * Link the new grammar in the tree
	 */
	ret->parent = ctxt->grammar;
	if(ctxt->grammar) {
		tmp = ctxt->grammar->children;
		if(!tmp) {
			ctxt->grammar->children = ret;
		}
		else {
			while(tmp->next)
				tmp = tmp->next;
			tmp->next = ret;
		}
	}
	old = ctxt->grammar;
	ctxt->grammar = ret;
	xmlRelaxNGParseGrammarContent(ctxt, nodes);
	ctxt->grammar = ret;
	if(ctxt->grammar == NULL) {
		xmlRngPErr(ctxt, nodes, XML_RNGP_GRAMMAR_CONTENT, "Failed to parse <grammar> content\n", 0, 0);
	}
	else if(ctxt->grammar->start == NULL) {
		xmlRngPErr(ctxt, nodes, XML_RNGP_GRAMMAR_NO_START, "Element <grammar> has no <start>\n", 0, 0);
	}
	/*
	 * Apply 4.17 merging rules to defines and starts
	 */
	xmlRelaxNGCombineStart(ctxt, ret);
	if(ret->defs)
		xmlHashScan(ret->defs, (xmlHashScanner)xmlRelaxNGCheckCombine, ctxt);
	/*
	 * link together defines and refs in this grammar
	 */
	if(ret->refs)
		xmlHashScan(ret->refs, (xmlHashScanner)xmlRelaxNGCheckReference, ctxt);
	/* @@@@ */
	ctxt->grammar = old;
	return ret;
}
/**
 * xmlRelaxNGParseDocument:
 * @ctxt:  a Relax-NG parser context
 * @node:  the root node of the RelaxNG schema
 *
 * parse a Relax-NG definition resource and build an internal
 * xmlRelaxNG struture which can be used to validate instances.
 *
 * Returns the internal XML RelaxNG structure built or
 *         NULL in case of error
 */
static xmlRelaxNGPtr xmlRelaxNGParseDocument(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node)
{
	xmlRelaxNGPtr schema = NULL;
	const xmlChar * olddefine;
	xmlRelaxNGGrammarPtr old;
	if(!ctxt || !P_Node)
		return 0;
	schema = xmlRelaxNGNewRelaxNG(ctxt);
	if(schema == NULL)
		return 0;
	olddefine = ctxt->define;
	ctxt->define = NULL;
	if(IS_RELAXNG(P_Node, "grammar")) {
		schema->topgrammar = xmlRelaxNGParseGrammar(ctxt, P_Node->children);
		if(schema->topgrammar == NULL) {
			xmlRelaxNGFree(schema);
			return 0;
		}
	}
	else {
		xmlRelaxNGGrammarPtr tmp, ret;
		schema->topgrammar = ret = xmlRelaxNGNewGrammar(ctxt);
		if(schema->topgrammar == NULL) {
			xmlRelaxNGFree(schema);
			return 0;
		}
		/*
		 * Link the new grammar in the tree
		 */
		ret->parent = ctxt->grammar;
		if(ctxt->grammar) {
			tmp = ctxt->grammar->children;
			if(!tmp) {
				ctxt->grammar->children = ret;
			}
			else {
				while(tmp->next)
					tmp = tmp->next;
				tmp->next = ret;
			}
		}
		old = ctxt->grammar;
		ctxt->grammar = ret;
		xmlRelaxNGParseStart(ctxt, P_Node);
		if(old)
			ctxt->grammar = old;
	}
	ctxt->define = olddefine;
	if(schema->topgrammar->start) {
		xmlRelaxNGCheckCycles(ctxt, schema->topgrammar->start, 0);
		if((ctxt->flags & XML_RELAXNG_IN_EXTERNALREF) == 0) {
			xmlRelaxNGSimplify(ctxt, schema->topgrammar->start, 0);
			while(schema->topgrammar->start && (schema->topgrammar->start->type == XML_RELAXNG_NOOP) && schema->topgrammar->start->next)
				schema->topgrammar->start = schema->topgrammar->start->content;
			xmlRelaxNGCheckRules(ctxt, schema->topgrammar->start, XML_RELAXNG_IN_START, XML_RELAXNG_NOOP);
		}
	}
#ifdef DEBUG
	if(schema == NULL)
		xmlGenericError(0, "xmlRelaxNGParseDocument() failed\n");
#endif
	return (schema);
}

/************************************************************************
*									*
*			Reading RelaxNGs				*
*									*
************************************************************************/

/**
 * xmlRelaxNGNewParserCtxt:
 * @URL:  the location of the schema
 *
 * Create an XML RelaxNGs parse context for that file/resource expected
 * to contain an XML RelaxNGs file.
 *
 * Returns the parser context or NULL in case of error
 */
xmlRelaxNGParserCtxtPtr xmlRelaxNGNewParserCtxt(const char * URL)
{
	xmlRelaxNGParserCtxt * ret = 0;
	if(URL) {
		ret = (xmlRelaxNGParserCtxtPtr)SAlloc::M(sizeof(xmlRelaxNGParserCtxt));
		if(!ret) {
			xmlRngPErrMemory(NULL, "building parser\n");
		}
		else {
			memzero(ret, sizeof(xmlRelaxNGParserCtxt));
			ret->URL = sstrdup((const xmlChar*)URL);
			ret->error = xmlGenericError;
			ret->userData = xmlGenericErrorContext;
		}
	}
	return ret;
}

/**
 * xmlRelaxNGNewMemParserCtxt:
 * @buffer:  a pointer to a char array containing the schemas
 * @size:  the size of the array
 *
 * Create an XML RelaxNGs parse context for that memory buffer expected
 * to contain an XML RelaxNGs file.
 *
 * Returns the parser context or NULL in case of error
 */
xmlRelaxNGParserCtxtPtr xmlRelaxNGNewMemParserCtxt(const char * buffer, int size)
{
	xmlRelaxNGParserCtxt * ret = 0;
	if(buffer && size > 0) {
		ret = (xmlRelaxNGParserCtxt *)SAlloc::M(sizeof(xmlRelaxNGParserCtxt));
		if(!ret)
			xmlRngPErrMemory(NULL, "building parser\n");
		else {
			memzero(ret, sizeof(xmlRelaxNGParserCtxt));
			ret->buffer = buffer;
			ret->size = size;
			ret->error = xmlGenericError;
			ret->userData = xmlGenericErrorContext;
		}
	}
	return ret;
}
/**
 * xmlRelaxNGNewDocParserCtxt:
 * @doc:  a preparsed document tree
 *
 * Create an XML RelaxNGs parser context for that document.
 * Note: since the process of compiling a RelaxNG schemas modifies the
 *       document, the @doc parameter is duplicated internally.
 *
 * Returns the parser context or NULL in case of error
 */
xmlRelaxNGParserCtxtPtr xmlRelaxNGNewDocParserCtxt(xmlDoc * doc)
{
	xmlRelaxNGParserCtxt * ret = 0;
	if(doc) {
		xmlDoc * copy = xmlCopyDoc(doc, 1);
		if(copy) {
			ret = (xmlRelaxNGParserCtxtPtr)SAlloc::M(sizeof(xmlRelaxNGParserCtxt));
			if(!ret)
				xmlRngPErrMemory(NULL, "building parser\n");
			else {
				memzero(ret, sizeof(xmlRelaxNGParserCtxt));
				ret->document = copy;
				ret->freedoc = 1;
				ret->userData = xmlGenericErrorContext;
			}
		}
	}
	return ret;
}
/**
 * xmlRelaxNGFreeParserCtxt:
 * @ctxt:  the schema parser context
 *
 * Free the resources associated to the schema parser context
 */
void xmlRelaxNGFreeParserCtxt(xmlRelaxNGParserCtxtPtr ctxt)
{
	if(ctxt) {
		SAlloc::F(ctxt->URL);
		xmlRelaxNGFreeDocument(ctxt->doc);
		xmlHashFree(ctxt->interleaves, 0);
		xmlRelaxNGFreeDocumentList(ctxt->documents);
		xmlRelaxNGFreeIncludeList(ctxt->includes);
		SAlloc::F(ctxt->docTab);
		SAlloc::F(ctxt->incTab);
		if(ctxt->defTab) {
			for(int i = 0; i < ctxt->defNr; i++)
				xmlRelaxNGFreeDefine(ctxt->defTab[i]);
			SAlloc::F(ctxt->defTab);
		}
		if(ctxt->document && ctxt->freedoc)
			xmlFreeDoc(ctxt->document);
		SAlloc::F(ctxt);
	}
}
/**
 * xmlRelaxNGNormExtSpace:
 * @value:  a value
 *
 * Removes the leading and ending spaces of the value
 * The string is modified "in situ"
 */
static void xmlRelaxNGNormExtSpace(xmlChar * value)
{
	xmlChar * start = value;
	xmlChar * cur = value;
	if(value) {
		while(IS_BLANK_CH(*cur))
			cur++;
		if(cur == start) {
			do {
				while((*cur != 0) && (!IS_BLANK_CH(*cur)))
					cur++;
				if(*cur == 0)
					return;
				start = cur;
				while(IS_BLANK_CH(*cur))
					cur++;
				if(*cur == 0) {
					*start = 0;
					return;
				}
			} while(1);
		}
		else {
			do {
				while((*cur != 0) && (!IS_BLANK_CH(*cur)))
					*start++ = *cur++;
				if(*cur == 0) {
					*start = 0;
					return;
				}
				/* don't try to normalize the inner spaces */
				while(IS_BLANK_CH(*cur))
					cur++;
				if(*cur == 0) {
					*start = 0;
					return;
				}
				*start++ = *cur++;
			} while(1);
		}
	}
}
/**
 * xmlRelaxNGCleanupAttributes:
 * @ctxt:  a Relax-NG parser context
 * @node:  a Relax-NG node
 *
 * Check all the attributes on the given node
 */
static void xmlRelaxNGCleanupAttributes(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * P_Node)
{
	for(xmlAttr * cur = P_Node->properties; cur;) {
		xmlAttr * next = cur->next;
		if(!cur->ns || sstreq(cur->ns->href, xmlRelaxNGNs)) {
			if(sstreq(cur->name, "name")) {
				if((!sstreq(P_Node->name, "element")) && (!sstreq(P_Node->name, "attribute")) && (!sstreq(P_Node->name, "ref")) &&
				    (!sstreq(P_Node->name, "parentRef")) && (!sstreq(P_Node->name, "param")) && (!sstreq(P_Node->name, "define"))) {
					xmlRngPErr(ctxt, P_Node, XML_RNGP_FORBIDDEN_ATTRIBUTE, "Attribute %s is not allowed on %s\n", cur->name, P_Node->name);
				}
			}
			else if(sstreq(cur->name, "type")) {
				if((!sstreq(P_Node->name, "value")) && (!sstreq(P_Node->name, "data"))) {
					xmlRngPErr(ctxt, P_Node, XML_RNGP_FORBIDDEN_ATTRIBUTE, "Attribute %s is not allowed on %s\n", cur->name, P_Node->name);
				}
			}
			else if(sstreq(cur->name, "href")) {
				if((!sstreq(P_Node->name, "externalRef")) && (!sstreq(P_Node->name, "include"))) {
					xmlRngPErr(ctxt, P_Node, XML_RNGP_FORBIDDEN_ATTRIBUTE, "Attribute %s is not allowed on %s\n", cur->name, P_Node->name);
				}
			}
			else if(sstreq(cur->name, "combine")) {
				if((!sstreq(P_Node->name, "start")) && (!sstreq(P_Node->name, "define"))) {
					xmlRngPErr(ctxt, P_Node, XML_RNGP_FORBIDDEN_ATTRIBUTE, "Attribute %s is not allowed on %s\n", cur->name, P_Node->name);
				}
			}
			else if(sstreq(cur->name, "datatypeLibrary")) {
				xmlChar * val = xmlNodeListGetString(P_Node->doc, cur->children, 1);
				if(val) {
					if(val[0] != 0) {
						xmlURI * uri = xmlParseURI((const char*)val);
						if(uri == NULL) {
							xmlRngPErr(ctxt, P_Node, XML_RNGP_INVALID_URI, "Attribute %s contains invalid URI %s\n", cur->name, val);
						}
						else {
							if(uri->scheme == NULL) {
								xmlRngPErr(ctxt, P_Node, XML_RNGP_URI_NOT_ABSOLUTE, "Attribute %s URI %s is not absolute\n", cur->name, val);
							}
							if(uri->fragment) {
								xmlRngPErr(ctxt, P_Node, XML_RNGP_URI_FRAGMENT, "Attribute %s URI %s has a fragment ID\n", cur->name, val);
							}
							xmlFreeURI(uri);
						}
					}
					SAlloc::F(val);
				}
			}
			else if(!sstreq(cur->name, "ns")) {
				xmlRngPErr(ctxt, P_Node, XML_RNGP_UNKNOWN_ATTRIBUTE, "Unknown attribute %s on %s\n", cur->name, P_Node->name);
			}
		}
		cur = next;
	}
}

/**
 * xmlRelaxNGCleanupTree:
 * @ctxt:  a Relax-NG parser context
 * @root:  an xmlNode * subtree
 *
 * Cleanup the subtree from unwanted nodes for parsing, resolve
 * Include and externalRef lookups.
 */
static void xmlRelaxNGCleanupTree(xmlRelaxNGParserCtxtPtr ctxt, xmlNode * root)
{
	xmlNode * p_delete = NULL;
	xmlNode * cur = root;
	while(cur) {
		if(p_delete) {
			xmlUnlinkNode(p_delete);
			xmlFreeNode(p_delete);
			p_delete = NULL;
		}
		if(cur->type == XML_ELEMENT_NODE) {
			/*
			 * Simplification 4.1. Annotations
			 */
			if(!cur->ns || !sstreq(cur->ns->href, xmlRelaxNGNs)) {
				if(cur->parent && (cur->parent->type == XML_ELEMENT_NODE) &&
				    (sstreq(cur->parent->name, "name") || sstreq(cur->parent->name, "value") || sstreq(cur->parent->name, "param"))) {
					xmlRngPErr(ctxt, cur, XML_RNGP_FOREIGN_ELEMENT, "element %s doesn't allow foreign elements\n", cur->parent->name, 0);
				}
				p_delete = cur;
				goto skip_children;
			}
			else {
				xmlRelaxNGCleanupAttributes(ctxt, cur);
				if(sstreq(cur->name, "externalRef")) {
					xmlChar * href, * base, * URL;
					xmlRelaxNGDocumentPtr docu;
					xmlNode * tmp;
					xmlURIPtr uri;
					xmlChar * ns = xmlGetProp(cur, BAD_CAST "ns");
					if(ns == NULL) {
						tmp = cur->parent;
						while(tmp && tmp->type == XML_ELEMENT_NODE) {
							ns = xmlGetProp(tmp, BAD_CAST "ns");
							if(ns)
								break;
							tmp = tmp->parent;
						}
					}
					href = xmlGetProp(cur, BAD_CAST "href");
					if(href == NULL) {
						xmlRngPErr(ctxt, cur, XML_RNGP_MISSING_HREF, "xmlRelaxNGParse: externalRef has no href attribute\n", 0, 0);
						SAlloc::F(ns);
						p_delete = cur;
						goto skip_children;
					}
					uri = xmlParseURI((const char*)href);
					if(uri == NULL) {
						xmlRngPErr(ctxt, cur, XML_RNGP_HREF_ERROR, "Incorrect URI for externalRef %s\n", href, 0);
						SAlloc::F(ns);
						SAlloc::F(href);
						p_delete = cur;
						goto skip_children;
					}
					if(uri->fragment) {
						xmlRngPErr(ctxt, cur, XML_RNGP_HREF_ERROR, "Fragment forbidden in URI for externalRef %s\n", href, 0);
						SAlloc::F(ns);
						xmlFreeURI(uri);
						SAlloc::F(href);
						p_delete = cur;
						goto skip_children;
					}
					xmlFreeURI(uri);
					base = xmlNodeGetBase(cur->doc, cur);
					URL = xmlBuildURI(href, base);
					if(URL == NULL) {
						xmlRngPErr(ctxt, cur, XML_RNGP_HREF_ERROR, "Failed to compute URL for externalRef %s\n", href, 0);
						SAlloc::F(ns);
						SAlloc::F(href);
						SAlloc::F(base);
						p_delete = cur;
						goto skip_children;
					}
					SAlloc::F(href);
					SAlloc::F(base);
					docu = xmlRelaxNGLoadExternalRef(ctxt, URL, ns);
					if(docu == NULL) {
						xmlRngPErr(ctxt, cur, XML_RNGP_EXTERNAL_REF_FAILURE, "Failed to load externalRef %s\n", URL, 0);
						SAlloc::F(ns);
						SAlloc::F(URL);
						p_delete = cur;
						goto skip_children;
					}
					SAlloc::F(ns);
					SAlloc::F(URL);
					cur->psvi = docu;
				}
				else if(sstreq(cur->name, "include")) {
					xmlChar * ns, * base, * URL;
					xmlRelaxNGIncludePtr incl;
					xmlNode * tmp;
					xmlChar * href = xmlGetProp(cur, BAD_CAST "href");
					if(href == NULL) {
						xmlRngPErr(ctxt, cur, XML_RNGP_MISSING_HREF, "xmlRelaxNGParse: include has no href attribute\n", 0, 0);
						p_delete = cur;
						goto skip_children;
					}
					base = xmlNodeGetBase(cur->doc, cur);
					URL = xmlBuildURI(href, base);
					if(URL == NULL) {
						xmlRngPErr(ctxt, cur, XML_RNGP_HREF_ERROR, "Failed to compute URL for include %s\n", href, 0);
						SAlloc::F(href);
						SAlloc::F(base);
						p_delete = cur;
						goto skip_children;
					}
					SAlloc::F(href);
					SAlloc::F(base);
					ns = xmlGetProp(cur, BAD_CAST "ns");
					if(ns == NULL) {
						tmp = cur->parent;
						while(tmp && tmp->type == XML_ELEMENT_NODE) {
							ns = xmlGetProp(tmp, BAD_CAST "ns");
							if(ns)
								break;
							tmp = tmp->parent;
						}
					}
					incl = xmlRelaxNGLoadInclude(ctxt, URL, cur, ns);
					SAlloc::F(ns);
					if(incl == NULL) {
						xmlRngPErr(ctxt, cur, XML_RNGP_INCLUDE_FAILURE, "Failed to load include %s\n", URL, 0);
						SAlloc::F(URL);
						p_delete = cur;
						goto skip_children;
					}
					SAlloc::F(URL);
					cur->psvi = incl;
				}
				else if((sstreq(cur->name, "element")) || (sstreq(cur->name, "attribute"))) {
					xmlChar * ns;
					xmlNode * text = NULL;
					/*
					 * Simplification 4.8. name attribute of element
					 * and attribute elements
					 */
					xmlChar * name = xmlGetProp(cur, BAD_CAST "name");
					if(name) {
						if(cur->children == NULL) {
							text = xmlNewChild(cur, cur->ns, BAD_CAST "name", name);
						}
						else {
							xmlNode * P_Node = xmlNewDocNode(cur->doc, cur->ns, BAD_CAST "name", 0);
							if(P_Node) {
								xmlAddPrevSibling(cur->children, P_Node);
								text = xmlNewText(name);
								xmlAddChild(P_Node, text);
								text = P_Node;
							}
						}
						if(text == NULL) {
							xmlRngPErr(ctxt, cur, XML_RNGP_CREATE_FAILURE, "Failed to create a name %s element\n", name, 0);
						}
						xmlUnsetProp(cur, BAD_CAST "name");
						SAlloc::F(name);
						ns = xmlGetProp(cur, BAD_CAST "ns");
						if(ns) {
							if(text) {
								xmlSetProp(text, BAD_CAST "ns", ns);
								/* xmlUnsetProp(cur, BAD_CAST "ns"); */
							}
							SAlloc::F(ns);
						}
						else if(sstreq(cur->name, BAD_CAST "attribute")) {
							xmlSetProp(text, BAD_CAST "ns", BAD_CAST "");
						}
					}
				}
				else if(sstreq(cur->name, "name") || sstreq(cur->name, "nsName") || sstreq(cur->name, "value")) {
					/*
					 * Simplification 4.8. name attribute of element
					 * and attribute elements
					 */
					if(xmlHasProp(cur, BAD_CAST "ns") == NULL) {
						xmlChar * ns = NULL;
						xmlNode * P_Node = cur->parent;
						while(P_Node && (P_Node->type == XML_ELEMENT_NODE)) {
							ns = xmlGetProp(P_Node, BAD_CAST "ns");
							if(ns) {
								break;
							}
							P_Node = P_Node->parent;
						}
						if(ns == NULL) {
							xmlSetProp(cur, BAD_CAST "ns", BAD_CAST "");
						}
						else {
							xmlSetProp(cur, BAD_CAST "ns", ns);
							SAlloc::F(ns);
						}
					}
					if(sstreq(cur->name, "name")) {
						xmlChar * local, * prefix;
						/*
						 * Simplification: 4.10. QNames
						 */
						xmlChar * name = xmlNodeGetContent(cur);
						if(name) {
							local = xmlSplitQName2(name, &prefix);
							if(local) {
								xmlNs * ns = xmlSearchNs(cur->doc, cur, prefix);
								if(ns == NULL) {
									xmlRngPErr(ctxt, cur, XML_RNGP_PREFIX_UNDEFINED, "xmlRelaxNGParse: no namespace for prefix %s\n", prefix, 0);
								}
								else {
									xmlSetProp(cur, BAD_CAST "ns", ns->href);
									xmlNodeSetContent(cur, local);
								}
								SAlloc::F(local);
								SAlloc::F(prefix);
							}
							SAlloc::F(name);
						}
					}
					/*
					 * 4.16
					 */
					if(sstreq(cur->name, "nsName")) {
						if(ctxt->flags & XML_RELAXNG_IN_NSEXCEPT) {
							xmlRngPErr(ctxt, cur, XML_RNGP_PAT_NSNAME_EXCEPT_NSNAME, "Found nsName/except//nsName forbidden construct\n", 0, 0);
						}
					}
				}
				else if(sstreq(cur->name, "except") && cur != root) {
					int oldflags = ctxt->flags;
					/*
					 * 4.16
					 */
					if(cur->parent && sstreq(cur->parent->name, "anyName")) {
						ctxt->flags |= XML_RELAXNG_IN_ANYEXCEPT;
						xmlRelaxNGCleanupTree(ctxt, cur);
						ctxt->flags = oldflags;
						goto skip_children;
					}
					else if(cur->parent && sstreq(cur->parent->name, "nsName")) {
						ctxt->flags |= XML_RELAXNG_IN_NSEXCEPT;
						xmlRelaxNGCleanupTree(ctxt, cur);
						ctxt->flags = oldflags;
						goto skip_children;
					}
				}
				else if(sstreq(cur->name, "anyName")) {
					/*
					 * 4.16
					 */
					if(ctxt->flags & XML_RELAXNG_IN_ANYEXCEPT) {
						xmlRngPErr(ctxt, cur, XML_RNGP_PAT_ANYNAME_EXCEPT_ANYNAME, "Found anyName/except//anyName forbidden construct\n", 0, 0);
					}
					else if(ctxt->flags & XML_RELAXNG_IN_NSEXCEPT) {
						xmlRngPErr(ctxt, cur, XML_RNGP_PAT_NSNAME_EXCEPT_ANYNAME, "Found nsName/except//anyName forbidden construct\n", 0, 0);
					}
				}
				/*
				 * This is not an else since "include" is transformed
				 * into a div
				 */
				if(sstreq(cur->name, "div")) {
					xmlNode * tmp;
					/*
					 * implements rule 4.11
					 */
					xmlChar * ns = xmlGetProp(cur, BAD_CAST "ns");
					xmlNode * child = cur->children;
					xmlNode * ins = cur;
					while(child) {
						if(ns) {
							if(!xmlHasProp(child, BAD_CAST "ns")) {
								xmlSetProp(child, BAD_CAST "ns", ns);
							}
						}
						tmp = child->next;
						xmlUnlinkNode(child);
						ins = xmlAddNextSibling(ins, child);
						child = tmp;
					}
					SAlloc::F(ns);
					/*
					 * Since we are about to p_delete cur, if its nsDef is non-NULL we
					 * need to preserve it (it contains the ns definitions for the
					 * children we just moved).  We'll just stick it on to the end
					 * of cur->parent's list, since it's never going to be re-serialized
					 * (bug 143738).
					 */
					if(cur->nsDef && cur->parent) {
						xmlNs * parDef = (xmlNs *)&cur->parent->nsDef;
						while(parDef->next)
							parDef = parDef->next;
						parDef->next = cur->nsDef;
						cur->nsDef = NULL;
					}
					p_delete = cur;
					goto skip_children;
				}
			}
		}
		/*
		 * Simplification 4.2 whitespaces
		 */
		else if(oneof2(cur->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE)) {
			if(IS_BLANK_NODE(cur)) {
				if(cur->parent && cur->parent->type == XML_ELEMENT_NODE) {
					if((!sstreq(cur->parent->name, "value")) && (!sstreq (cur->parent->name, "param")))
						p_delete = cur;
				}
				else {
					p_delete = cur;
					goto skip_children;
				}
			}
		}
		else {
			p_delete = cur;
			goto skip_children;
		}

		/*
		 * Skip to next node
		 */
		if(cur->children) {
			if(!oneof3(cur->children->type, XML_ENTITY_DECL, XML_ENTITY_REF_NODE, XML_ENTITY_NODE)) {
				cur = cur->children;
				continue;
			}
		}
skip_children:
		if(cur->next) {
			cur = cur->next;
			continue;
		}
		do {
			cur = cur->parent;
			if(!cur)
				break;
			if(cur == root) {
				cur = NULL;
				break;
			}
			if(cur->next) {
				cur = cur->next;
				break;
			}
		} while(cur);
	}
	if(p_delete) {
		xmlUnlinkNode(p_delete);
		xmlFreeNode(p_delete);
		p_delete = NULL;
	}
}
/**
 * xmlRelaxNGCleanupDoc:
 * @ctxt:  a Relax-NG parser context
 * @doc:  an xmldocPtr document pointer
 *
 * Cleanup the document from unwanted nodes for parsing, resolve
 * Include and externalRef lookups.
 *
 * Returns the cleaned up document or NULL in case of error
 */
static xmlDoc * xmlRelaxNGCleanupDoc(xmlRelaxNGParserCtxtPtr ctxt, xmlDoc * doc)
{
	/*
	 * Extract the root
	 */
	xmlNode * root = xmlDocGetRootElement(doc);
	if(root == NULL) {
		xmlRngPErr(ctxt, (xmlNode *)doc, XML_RNGP_EMPTY, "xmlRelaxNGParse: %s is empty\n", ctxt->URL, 0);
		return 0;
	}
	else {
		xmlRelaxNGCleanupTree(ctxt, root);
		return doc;
	}
}
/**
 * xmlRelaxNGParse:
 * @ctxt:  a Relax-NG parser context
 *
 * parse a schema definition resource and build an internal
 * XML Shema struture which can be used to validate instances.
 *
 * Returns the internal XML RelaxNG structure built from the resource or
 *         NULL in case of error
 */
xmlRelaxNGPtr xmlRelaxNGParse(xmlRelaxNGParserCtxtPtr ctxt)
{
	xmlRelaxNGPtr ret = NULL;
	xmlDoc * doc;
	xmlNode * root;
	xmlRelaxNGInitTypes();
	if(!ctxt)
		return 0;
	/*
	 * First step is to parse the input document into an DOM/Infoset
	 */
	if(ctxt->URL) {
		doc = xmlReadFile((const char*)ctxt->URL, NULL, 0);
		if(!doc) {
			xmlRngPErr(ctxt, NULL, XML_RNGP_PARSE_ERROR, "xmlRelaxNGParse: could not load %s\n", ctxt->URL, 0);
			return 0;
		}
	}
	else if(ctxt->buffer) {
		doc = xmlReadMemory(ctxt->buffer, ctxt->size, NULL, NULL, 0);
		if(!doc) {
			xmlRngPErr(ctxt, NULL, XML_RNGP_PARSE_ERROR, "xmlRelaxNGParse: could not parse schemas\n", 0, 0);
			return 0;
		}
		doc->URL = sstrdup(BAD_CAST "in_memory_buffer");
		ctxt->URL = sstrdup(BAD_CAST "in_memory_buffer");
	}
	else if(ctxt->document) {
		doc = ctxt->document;
	}
	else {
		xmlRngPErr(ctxt, NULL, XML_RNGP_EMPTY, "xmlRelaxNGParse: nothing to parse\n", 0, 0);
		return 0;
	}
	ctxt->document = doc;

	/*
	 * Some preprocessing of the document content
	 */
	doc = xmlRelaxNGCleanupDoc(ctxt, doc);
	if(!doc) {
		xmlFreeDoc(ctxt->document);
		ctxt->document = NULL;
		return 0;
	}
	/*
	 * Then do the parsing for good
	 */
	root = xmlDocGetRootElement(doc);
	if(root == NULL) {
		xmlRngPErr(ctxt, (xmlNode *)doc, XML_RNGP_EMPTY, "xmlRelaxNGParse: %s is empty\n", (ctxt->URL ? ctxt->URL : BAD_CAST "schemas"), 0);
		xmlFreeDoc(ctxt->document);
		ctxt->document = NULL;
		return 0;
	}
	ret = xmlRelaxNGParseDocument(ctxt, root);
	if(!ret) {
		xmlFreeDoc(ctxt->document);
		ctxt->document = NULL;
		return 0;
	}
	/*
	 * Check the ref/defines links
	 */
	/*
	 * try to preprocess interleaves
	 */
	if(ctxt->interleaves) {
		xmlHashScan(ctxt->interleaves, (xmlHashScanner)xmlRelaxNGComputeInterleaves, ctxt);
	}
	/*
	 * if there was a parsing error return NULL
	 */
	if(ctxt->nbErrors > 0) {
		xmlRelaxNGFree(ret);
		ctxt->document = NULL;
		xmlFreeDoc(doc);
		return 0;
	}
	/*
	 * try to compile (parts of) the schemas
	 */
	if(ret->topgrammar && ret->topgrammar->start) {
		if(ret->topgrammar->start->type != XML_RELAXNG_START) {
			xmlRelaxNGDefine * def = xmlRelaxNGNewDefine(ctxt, 0);
			if(def) {
				def->type = XML_RELAXNG_START;
				def->content = ret->topgrammar->start;
				ret->topgrammar->start = def;
			}
		}
		xmlRelaxNGTryCompile(ctxt, ret->topgrammar->start);
	}
	/*
	 * Transfer the pointer for cleanup at the schema level.
	 */
	ret->doc = doc;
	ctxt->document = NULL;
	ret->documents = ctxt->documents;
	ctxt->documents = NULL;
	ret->includes = ctxt->includes;
	ctxt->includes = NULL;
	ret->defNr = ctxt->defNr;
	ret->defTab = ctxt->defTab;
	ctxt->defTab = NULL;
	if(ctxt->idref == 1)
		ret->idref = 1;
	return ret;
}
/**
 * xmlRelaxNGSetParserErrors:
 * @ctxt:  a Relax-NG validation context
 * @err:  the error callback
 * @warn:  the warning callback
 * @ctx:  contextual data for the callbacks
 *
 * Set the callback functions used to handle errors for a validation context
 */
void xmlRelaxNGSetParserErrors(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGValidityErrorFunc err, xmlRelaxNGValidityWarningFunc warn, void * ctx)
{
	if(ctxt) {
		ctxt->error = err;
		ctxt->warning = warn;
		ctxt->serror = NULL;
		ctxt->userData = ctx;
	}
}
/**
 * xmlRelaxNGGetParserErrors:
 * @ctxt:  a Relax-NG validation context
 * @err:  the error callback result
 * @warn:  the warning callback result
 * @ctx:  contextual data for the callbacks result
 *
 * Get the callback information used to handle errors for a validation context
 *
 * Returns -1 in case of failure, 0 otherwise.
 */
int xmlRelaxNGGetParserErrors(xmlRelaxNGParserCtxtPtr ctxt, xmlRelaxNGValidityErrorFunc * err, xmlRelaxNGValidityWarningFunc * warn, void ** ctx)
{
	if(!ctxt)
		return -1;
	else {
		ASSIGN_PTR(err, ctxt->error);
		ASSIGN_PTR(warn, ctxt->warning);
		ASSIGN_PTR(ctx, ctxt->userData);
		return 0;
	}
}

/**
 * xmlRelaxNGSetParserStructuredErrors:
 * @ctxt:  a Relax-NG parser context
 * @serror:  the error callback
 * @ctx:  contextual data for the callbacks
 *
 * Set the callback functions used to handle errors for a parsing context
 */
void xmlRelaxNGSetParserStructuredErrors(xmlRelaxNGParserCtxtPtr ctxt, xmlStructuredErrorFunc serror, void * ctx)
{
	if(ctxt) {
		ctxt->serror = serror;
		ctxt->error = NULL;
		ctxt->warning = NULL;
		ctxt->userData = ctx;
	}
}

#ifdef LIBXML_OUTPUT_ENABLED

/************************************************************************
*									*
*			Dump back a compiled form			*
*									*
************************************************************************/
static void xmlRelaxNGDumpDefine(FILE * output, xmlRelaxNGDefinePtr define);
/**
 * xmlRelaxNGDumpDefines:
 * @output:  the file output
 * @defines:  a list of define structures
 *
 * Dump a RelaxNG structure back
 */
static void FASTCALL xmlRelaxNGDumpDefines(FILE * output, xmlRelaxNGDefinePtr defines)
{
	while(defines) {
		xmlRelaxNGDumpDefine(output, defines);
		defines = defines->next;
	}
}
/**
 * xmlRelaxNGDumpDefine:
 * @output:  the file output
 * @define:  a define structure
 *
 * Dump a RelaxNG structure back
 */
static void xmlRelaxNGDumpDefine(FILE * output, xmlRelaxNGDefinePtr define)
{
	if(define) {
		switch(define->type) {
			case XML_RELAXNG_EMPTY:
				fprintf(output, "<empty/>\n");
				break;
			case XML_RELAXNG_NOT_ALLOWED:
				fprintf(output, "<notAllowed/>\n");
				break;
			case XML_RELAXNG_TEXT:
				fprintf(output, "<text/>\n");
				break;
			case XML_RELAXNG_ELEMENT:
				fprintf(output, "<element>\n");
				if(define->name) {
					fprintf(output, "<name");
					if(define->ns)
						fprintf(output, " ns=\"%s\"", define->ns);
					fprintf(output, ">%s</name>\n", define->name);
				}
				xmlRelaxNGDumpDefines(output, define->attrs);
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</element>\n");
				break;
			case XML_RELAXNG_LIST:
				fprintf(output, "<list>\n");
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</list>\n");
				break;
			case XML_RELAXNG_ONEORMORE:
				fprintf(output, "<oneOrMore>\n");
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</oneOrMore>\n");
				break;
			case XML_RELAXNG_ZEROORMORE:
				fprintf(output, "<zeroOrMore>\n");
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</zeroOrMore>\n");
				break;
			case XML_RELAXNG_CHOICE:
				fprintf(output, "<choice>\n");
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</choice>\n");
				break;
			case XML_RELAXNG_GROUP:
				fprintf(output, "<group>\n");
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</group>\n");
				break;
			case XML_RELAXNG_INTERLEAVE:
				fprintf(output, "<interleave>\n");
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</interleave>\n");
				break;
			case XML_RELAXNG_OPTIONAL:
				fprintf(output, "<optional>\n");
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</optional>\n");
				break;
			case XML_RELAXNG_ATTRIBUTE:
				fprintf(output, "<attribute>\n");
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</attribute>\n");
				break;
			case XML_RELAXNG_DEF:
				fprintf(output, "<define");
				if(define->name)
					fprintf(output, " name=\"%s\"", define->name);
				fprintf(output, ">\n");
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</define>\n");
				break;
			case XML_RELAXNG_REF:
				fprintf(output, "<ref");
				if(define->name)
					fprintf(output, " name=\"%s\"", define->name);
				fprintf(output, ">\n");
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</ref>\n");
				break;
			case XML_RELAXNG_PARENTREF:
				fprintf(output, "<parentRef");
				if(define->name)
					fprintf(output, " name=\"%s\"", define->name);
				fprintf(output, ">\n");
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</parentRef>\n");
				break;
			case XML_RELAXNG_EXTERNALREF:
				fprintf(output, "<externalRef>");
				xmlRelaxNGDumpDefines(output, define->content);
				fprintf(output, "</externalRef>\n");
				break;
			case XML_RELAXNG_DATATYPE:
			case XML_RELAXNG_VALUE:
				TODO break;
			case XML_RELAXNG_START:
			case XML_RELAXNG_EXCEPT:
			case XML_RELAXNG_PARAM:
				TODO break;
			case XML_RELAXNG_NOOP:
				xmlRelaxNGDumpDefines(output, define->content);
				break;
		}
	}
}

/**
 * xmlRelaxNGDumpGrammar:
 * @output:  the file output
 * @grammar:  a grammar structure
 * @top:  is this a top grammar
 *
 * Dump a RelaxNG structure back
 */
static void xmlRelaxNGDumpGrammar(FILE * output, xmlRelaxNGGrammarPtr grammar, int top)
{
	if(grammar == NULL)
		return;

	fprintf(output, "<grammar");
	if(top)
		fprintf(output, " xmlns=\"http://relaxng.org/ns/structure/1.0\"");
	switch(grammar->combine) {
		case XML_RELAXNG_COMBINE_UNDEFINED:
		    break;
		case XML_RELAXNG_COMBINE_CHOICE:
		    fprintf(output, " combine=\"choice\"");
		    break;
		case XML_RELAXNG_COMBINE_INTERLEAVE:
		    fprintf(output, " combine=\"interleave\"");
		    break;
		default:
		    fprintf(output, " <!-- invalid combine value -->");
	}
	fprintf(output, ">\n");
	if(grammar->start == NULL) {
		fprintf(output, " <!-- grammar had no start -->");
	}
	else {
		fprintf(output, "<start>\n");
		xmlRelaxNGDumpDefine(output, grammar->start);
		fprintf(output, "</start>\n");
	}
	/* @todo ? Dump the defines ? */
	fprintf(output, "</grammar>\n");
}

/**
 * xmlRelaxNGDump:
 * @output:  the file output
 * @schema:  a schema structure
 *
 * Dump a RelaxNG structure back
 */
void xmlRelaxNGDump(FILE * output, xmlRelaxNGPtr schema)
{
	if(output) {
		if(!schema)
			fprintf(output, "RelaxNG empty or failed to compile\n");
		else {
			fprintf(output, "RelaxNG: ");
			if(schema->doc == NULL)
				fprintf(output, "no document\n");
			else if(schema->doc->URL)
				fprintf(output, "%s\n", schema->doc->URL);
			else
				fprintf(output, "\n");
			if(!schema->topgrammar)
				fprintf(output, "RelaxNG has no top grammar\n");
			else
				xmlRelaxNGDumpGrammar(output, schema->topgrammar, 1);
		}
	}
}
/**
 * xmlRelaxNGDumpTree:
 * @output:  the file output
 * @schema:  a schema structure
 *
 * Dump the transformed RelaxNG tree.
 */
void xmlRelaxNGDumpTree(FILE * output, xmlRelaxNGPtr schema)
{
	if(output) {
		if(schema == NULL) {
			fprintf(output, "RelaxNG empty or failed to compile\n");
		}
		else {
			if(schema->doc == NULL) {
				fprintf(output, "no document\n");
			}
			else {
				xmlDocDump(output, schema->doc);
			}
		}
	}
}

#endif /* LIBXML_OUTPUT_ENABLED */

/************************************************************************
*									*
*		Validation of compiled content				*
*									*
************************************************************************/
static int xmlRelaxNGValidateDefinition(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr define);
/**
 * xmlRelaxNGValidateCompiledCallback:
 * @exec:  the regular expression instance
 * @token:  the token which matched
 * @transdata:  callback data, the define for the subelement if available
   @ @inputdata:  callback data, the Relax NG validation context
 *
 * Handle the callback and if needed validate the element children.
 */
static void xmlRelaxNGValidateCompiledCallback(xmlRegExecCtxtPtr exec ATTRIBUTE_UNUSED, const xmlChar * token, void * transdata, void * inputdata)
{
	xmlRelaxNGValidCtxtPtr ctxt = (xmlRelaxNGValidCtxtPtr)inputdata;
	xmlRelaxNGDefinePtr define = (xmlRelaxNGDefinePtr)transdata;
	int ret;
#ifdef DEBUG_COMPILE
	xmlGenericError(0, "Compiled callback for: '%s'\n", token);
#endif
	if(!ctxt) {
		fprintf(stderr, "callback on %s missing context\n", token);
		return;
	}
	if(define == NULL) {
		if(token[0] == '#')
			return;
		fprintf(stderr, "callback on %s missing define\n", token);
		if(ctxt && ctxt->errNo == XML_RELAXNG_OK)
			ctxt->errNo = XML_RELAXNG_ERR_INTERNAL;
		return;
	}
	if(!ctxt || (define == NULL)) {
		fprintf(stderr, "callback on %s missing info\n", token);
		if(ctxt && ctxt->errNo == XML_RELAXNG_OK)
			ctxt->errNo = XML_RELAXNG_ERR_INTERNAL;
		return;
	}
	else if(define->type != XML_RELAXNG_ELEMENT) {
		fprintf(stderr, "callback on %s define is not element\n", token);
		if(ctxt->errNo == XML_RELAXNG_OK)
			ctxt->errNo = XML_RELAXNG_ERR_INTERNAL;
		return;
	}
	ret = xmlRelaxNGValidateDefinition(ctxt, define);
	if(ret != 0)
		ctxt->perr = ret;
}

/**
 * xmlRelaxNGValidateCompiledContent:
 * @ctxt:  the RelaxNG validation context
 * @regexp:  the regular expression as compiled
 * @content:  list of children to test against the regexp
 *
 * Validate the content model of an element or start using the regexp
 *
 * Returns 0 in case of success, -1 in case of error.
 */
static int xmlRelaxNGValidateCompiledContent(xmlRelaxNGValidCtxtPtr ctxt, xmlRegexpPtr regexp, xmlNode * content)
{
	xmlRegExecCtxtPtr exec;
	xmlNode * cur;
	int ret = 0;
	int oldperr;
	if(!ctxt || (regexp == NULL))
		return -1;
	oldperr = ctxt->perr;
	exec = xmlRegNewExecCtxt(regexp, xmlRelaxNGValidateCompiledCallback, ctxt);
	ctxt->perr = 0;
	cur = content;
	while(cur) {
		ctxt->state->seq = cur;
		switch(cur->type) {
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			    if(xmlIsBlankNode(cur))
				    break;
			    ret = xmlRegExecPushString(exec, BAD_CAST "#text", ctxt);
			    if(ret < 0) {
				    VALID_ERR2(XML_RELAXNG_ERR_TEXTWRONG, cur->parent->name);
			    }
			    break;
			case XML_ELEMENT_NODE:
			    if(cur->ns) {
				    ret = xmlRegExecPushString2(exec, cur->name, cur->ns->href, ctxt);
			    }
			    else {
				    ret = xmlRegExecPushString(exec, cur->name, ctxt);
			    }
			    if(ret < 0) {
				    VALID_ERR2(XML_RELAXNG_ERR_ELEMWRONG, cur->name);
			    }
			    break;
			default:
			    break;
		}
		if(ret < 0)
			break;
		/*
		 * Switch to next element
		 */
		cur = cur->next;
	}
	ret = xmlRegExecPushString(exec, 0, 0);
	if(ret == 1) {
		ret = 0;
		ctxt->state->seq = NULL;
	}
	else if(ret == 0) {
		/*
		 * @todo get some of the names needed to exit the current state of exec
		 */
		VALID_ERR2(XML_RELAXNG_ERR_NOELEM, BAD_CAST "");
		ret = -1;
		if((ctxt->flags & FLAGS_IGNORABLE) == 0)
			xmlRelaxNGDumpValidError(ctxt);
	}
	else {
		ret = -1;
	}
	xmlRegFreeExecCtxt(exec);
	/*
	 * There might be content model errors outside of the pure
	 * regexp validation, e.g. for attribute values.
	 */
	if((ret == 0) && (ctxt->perr != 0)) {
		ret = ctxt->perr;
	}
	ctxt->perr = oldperr;
	return ret;
}

/************************************************************************
*									*
*		Progressive validation of when possible			*
*									*
************************************************************************/
static int xmlRelaxNGValidateAttributeList(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr defines);
static int xmlRelaxNGValidateElementEnd(xmlRelaxNGValidCtxtPtr ctxt, int dolog);
static void xmlRelaxNGLogBestError(xmlRelaxNGValidCtxtPtr ctxt);

/**
 * xmlRelaxNGElemPush:
 * @ctxt:  the validation context
 * @exec:  the regexp runtime for the new content model
 *
 * Push a new regexp for the current node content model on the stack
 *
 * Returns 0 in case of success and -1 in case of error.
 */
static int xmlRelaxNGElemPush(xmlRelaxNGValidCtxtPtr ctxt, xmlRegExecCtxtPtr exec)
{
	if(ctxt->elemTab == NULL) {
		ctxt->elemMax = 10;
		ctxt->elemTab = (xmlRegExecCtxtPtr*)SAlloc::M(ctxt->elemMax * sizeof(xmlRegExecCtxtPtr));
		if(ctxt->elemTab == NULL) {
			xmlRngVErrMemory(ctxt, "validating\n");
			return -1;
		}
	}
	if(ctxt->elemNr >= ctxt->elemMax) {
		ctxt->elemMax *= 2;
		ctxt->elemTab = (xmlRegExecCtxtPtr*)SAlloc::R(ctxt->elemTab, ctxt->elemMax * sizeof(xmlRegExecCtxtPtr));
		if(ctxt->elemTab == NULL) {
			xmlRngVErrMemory(ctxt, "validating\n");
			return -1;
		}
	}
	ctxt->elemTab[ctxt->elemNr++] = exec;
	ctxt->elem = exec;
	return 0;
}
/**
 * xmlRelaxNGElemPop:
 * @ctxt:  the validation context
 *
 * Pop the regexp of the current node content model from the stack
 *
 * Returns the exec or NULL if empty
 */
static xmlRegExecCtxt * FASTCALL xmlRelaxNGElemPop(xmlRelaxNGValidCtxt * ctxt)
{
	xmlRegExecCtxt * ret = 0;
	if(ctxt->elemNr > 0) {
		ctxt->elemNr--;
		ret = ctxt->elemTab[ctxt->elemNr];
		ctxt->elemTab[ctxt->elemNr] = NULL;
		ctxt->elem = (ctxt->elemNr > 0) ? ctxt->elemTab[ctxt->elemNr - 1] : 0;
	}
	return ret;
}
/**
 * xmlRelaxNGValidateProgressiveCallback:
 * @exec:  the regular expression instance
 * @token:  the token which matched
 * @transdata:  callback data, the define for the subelement if available
   @ @inputdata:  callback data, the Relax NG validation context
 *
 * Handle the callback and if needed validate the element children.
 * some of the in/out informations are passed via the context in @inputdata.
 */
static void xmlRelaxNGValidateProgressiveCallback(xmlRegExecCtxtPtr exec ATTRIBUTE_UNUSED, const xmlChar * token, void * transdata, void * inputdata)
{
	xmlRelaxNGValidCtxtPtr ctxt = (xmlRelaxNGValidCtxtPtr)inputdata;
	xmlRelaxNGDefinePtr define = (xmlRelaxNGDefinePtr)transdata;
	xmlRelaxNGValidStatePtr state, oldstate;
	xmlNode * P_Node;
	int ret = 0, oldflags;
#ifdef DEBUG_PROGRESSIVE
	xmlGenericError(0, "Progressive callback for: '%s'\n", token);
#endif
	if(!ctxt) {
		fprintf(stderr, "callback on %s missing context\n", token);
		return;
	}
	P_Node = ctxt->pnode;
	ctxt->pstate = 1;
	if(define == NULL) {
		if(token[0] == '#')
			return;
		fprintf(stderr, "callback on %s missing define\n", token);
		if(ctxt && ctxt->errNo == XML_RELAXNG_OK)
			ctxt->errNo = XML_RELAXNG_ERR_INTERNAL;
		ctxt->pstate = -1;
		return;
	}
	if(!ctxt || (define == NULL)) {
		fprintf(stderr, "callback on %s missing info\n", token);
		if(ctxt && ctxt->errNo == XML_RELAXNG_OK)
			ctxt->errNo = XML_RELAXNG_ERR_INTERNAL;
		ctxt->pstate = -1;
		return;
	}
	else if(define->type != XML_RELAXNG_ELEMENT) {
		fprintf(stderr, "callback on %s define is not element\n", token);
		if(ctxt->errNo == XML_RELAXNG_OK)
			ctxt->errNo = XML_RELAXNG_ERR_INTERNAL;
		ctxt->pstate = -1;
		return;
	}
	if(P_Node->type != XML_ELEMENT_NODE) {
		VALID_ERR(XML_RELAXNG_ERR_NOTELEM);
		if((ctxt->flags & FLAGS_IGNORABLE) == 0)
			xmlRelaxNGDumpValidError(ctxt);
		ctxt->pstate = -1;
		return;
	}
	if(define->contModel == NULL) {
		/*
		 * this node cannot be validated in a streamable fashion
		 */
#ifdef DEBUG_PROGRESSIVE
		xmlGenericError(0, "Element '%s' validation is not streamable\n", token);
#endif
		ctxt->pstate = 0;
		ctxt->pdef = define;
		return;
	}
	exec = xmlRegNewExecCtxt(define->contModel, xmlRelaxNGValidateProgressiveCallback, ctxt);
	if(exec == NULL) {
		ctxt->pstate = -1;
		return;
	}
	xmlRelaxNGElemPush(ctxt, exec);
	/*
	 * Validate the attributes part of the content.
	 */
	state = xmlRelaxNGNewValidState(ctxt, P_Node);
	if(state == NULL) {
		ctxt->pstate = -1;
		return;
	}
	oldstate = ctxt->state;
	ctxt->state = state;
	if(define->attrs) {
		ret = xmlRelaxNGValidateAttributeList(ctxt, define->attrs);
		if(ret != 0) {
			ctxt->pstate = -1;
			VALID_ERR2(XML_RELAXNG_ERR_ATTRVALID, P_Node->name);
		}
	}
	if(ctxt->state) {
		ctxt->state->seq = NULL;
		ret = xmlRelaxNGValidateElementEnd(ctxt, 1);
		if(ret != 0) {
			ctxt->pstate = -1;
		}
		xmlRelaxNGFreeValidState(ctxt, ctxt->state);
	}
	else if(ctxt->states) {
		int tmp = -1, i;
		oldflags = ctxt->flags;
		for(i = 0; i < ctxt->states->nbState; i++) {
			state = ctxt->states->tabState[i];
			ctxt->state = state;
			ctxt->state->seq = NULL;
			if(xmlRelaxNGValidateElementEnd(ctxt, 0) == 0) {
				tmp = 0;
				break;
			}
		}
		if(tmp != 0) {
			/*
			 * validation error, log the message for the "best" one
			 */
			ctxt->flags |= FLAGS_IGNORABLE;
			xmlRelaxNGLogBestError(ctxt);
		}
		for(i = 0; i < ctxt->states->nbState; i++) {
			xmlRelaxNGFreeValidState(ctxt, ctxt->states->tabState[i]);
		}
		xmlRelaxNGFreeStates(ctxt, ctxt->states);
		ctxt->states = NULL;
		if((ret == 0) && (tmp == -1))
			ctxt->pstate = -1;
		ctxt->flags = oldflags;
	}
	if(ctxt->pstate == -1) {
		if((ctxt->flags & FLAGS_IGNORABLE) == 0) {
			xmlRelaxNGDumpValidError(ctxt);
		}
	}
	ctxt->state = oldstate;
}

/**
 * xmlRelaxNGValidatePushElement:
 * @ctxt:  the validation context
 * @doc:  a document instance
 * @elem:  an element instance
 *
 * Push a new element start on the RelaxNG validation stack.
 *
 * returns 1 if no validation problem was found or 0 if validating the
 *         element requires a full node, and -1 in case of error.
 */
int xmlRelaxNGValidatePushElement(xmlRelaxNGValidCtxtPtr ctxt, xmlDoc * doc ATTRIBUTE_UNUSED, xmlNode * elem)
{
	int ret = 1;
	if(!ctxt || (elem == NULL))
		return -1;
#ifdef DEBUG_PROGRESSIVE
	xmlGenericError(0, "PushElem %s\n", elem->name);
#endif
	if(ctxt->elem == 0) {
		xmlRelaxNGGrammarPtr grammar;
		xmlRegExecCtxtPtr exec;
		xmlRelaxNGDefinePtr define;
		xmlRelaxNG * schema = ctxt->schema;
		if(schema == NULL) {
			VALID_ERR(XML_RELAXNG_ERR_NOGRAMMAR);
			return -1;
		}
		grammar = schema->topgrammar;
		if((grammar == NULL) || (grammar->start == NULL)) {
			VALID_ERR(XML_RELAXNG_ERR_NOGRAMMAR);
			return -1;
		}
		define = grammar->start;
		if(define->contModel == NULL) {
			ctxt->pdef = define;
			return 0;
		}
		exec = xmlRegNewExecCtxt(define->contModel, xmlRelaxNGValidateProgressiveCallback, ctxt);
		if(exec == NULL) {
			return -1;
		}
		xmlRelaxNGElemPush(ctxt, exec);
	}
	ctxt->pnode = elem;
	ctxt->pstate = 0;
	if(elem->ns) {
		ret = xmlRegExecPushString2(ctxt->elem, elem->name, elem->ns->href, ctxt);
	}
	else {
		ret = xmlRegExecPushString(ctxt->elem, elem->name, ctxt);
	}
	if(ret < 0) {
		VALID_ERR2(XML_RELAXNG_ERR_ELEMWRONG, elem->name);
	}
	else {
		if(ctxt->pstate == 0)
			ret = 0;
		else if(ctxt->pstate < 0)
			ret = -1;
		else
			ret = 1;
	}
#ifdef DEBUG_PROGRESSIVE
	if(ret < 0)
		xmlGenericError(0, "PushElem %s failed\n", elem->name);
#endif
	return ret;
}
/**
 * xmlRelaxNGValidatePushCData:
 * @ctxt:  the RelaxNG validation context
 * @data:  some character data read
 * @len:  the length of the data
 *
 * check the CData parsed for validation in the current stack
 *
 * returns 1 if no validation problem was found or -1 otherwise
 */
int xmlRelaxNGValidatePushCData(xmlRelaxNGValidCtxtPtr ctxt, const xmlChar * data, int len ATTRIBUTE_UNUSED)
{
	int ret = 1;
	if(!ctxt || (ctxt->elem == NULL) || (data == NULL))
		return -1;
#ifdef DEBUG_PROGRESSIVE
	xmlGenericError(0, "CDATA %s %d\n", data, len);
#endif
	while(*data != 0) {
		if(!IS_BLANK_CH(*data))
			break;
		data++;
	}
	if(*data == 0)
		return 1;
	ret = xmlRegExecPushString(ctxt->elem, BAD_CAST "#text", ctxt);
	if(ret < 0) {
		VALID_ERR2(XML_RELAXNG_ERR_TEXTWRONG, BAD_CAST " TODO ");
#ifdef DEBUG_PROGRESSIVE
		xmlGenericError(0, "CDATA failed\n");
#endif
		return -1;
	}
	return 1;
}
/**
 * xmlRelaxNGValidatePopElement:
 * @ctxt:  the RelaxNG validation context
 * @doc:  a document instance
 * @elem:  an element instance
 *
 * Pop the element end from the RelaxNG validation stack.
 *
 * returns 1 if no validation problem was found or 0 otherwise
 */
int xmlRelaxNGValidatePopElement(xmlRelaxNGValidCtxtPtr ctxt, xmlDoc * doc ATTRIBUTE_UNUSED, xmlNode * elem)
{
	int ret;
	xmlRegExecCtxtPtr exec;

	if(!ctxt || (ctxt->elem == NULL) || (elem == NULL))
		return -1;
#ifdef DEBUG_PROGRESSIVE
	xmlGenericError(0, "PopElem %s\n", elem->name);
#endif
	/*
	 * verify that we reached a terminal state of the content model.
	 */
	exec = xmlRelaxNGElemPop(ctxt);
	ret = xmlRegExecPushString(exec, 0, 0);
	if(ret == 0) {
		/*
		 * @todo get some of the names needed to exit the current state of exec
		 */
		VALID_ERR2(XML_RELAXNG_ERR_NOELEM, BAD_CAST "");
		ret = -1;
	}
	else if(ret < 0) {
		ret = -1;
	}
	else {
		ret = 1;
	}
	xmlRegFreeExecCtxt(exec);
#ifdef DEBUG_PROGRESSIVE
	if(ret < 0)
		xmlGenericError(0, "PopElem %s failed\n", elem->name);
#endif
	return ret;
}

/**
 * xmlRelaxNGValidateFullElement:
 * @ctxt:  the validation context
 * @doc:  a document instance
 * @elem:  an element instance
 *
 * Validate a full subtree when xmlRelaxNGValidatePushElement() returned
 * 0 and the content of the node has been expanded.
 *
 * returns 1 if no validation problem was found or -1 in case of error.
 */
int xmlRelaxNGValidateFullElement(xmlRelaxNGValidCtxtPtr ctxt, xmlDoc * doc ATTRIBUTE_UNUSED, xmlNode * elem)
{
	int ret;
	xmlRelaxNGValidStatePtr state;
	if(!ctxt || !ctxt->pdef || !elem)
		return -1;
#ifdef DEBUG_PROGRESSIVE
	xmlGenericError(0, "FullElem %s\n", elem->name);
#endif
	state = xmlRelaxNGNewValidState(ctxt, elem->parent);
	if(state == NULL) {
		return -1;
	}
	state->seq = elem;
	ctxt->state = state;
	ctxt->errNo = XML_RELAXNG_OK;
	ret = xmlRelaxNGValidateDefinition(ctxt, ctxt->pdef);
	if((ret != 0) || (ctxt->errNo != XML_RELAXNG_OK))
		ret = -1;
	else
		ret = 1;
	xmlRelaxNGFreeValidState(ctxt, ctxt->state);
	ctxt->state = NULL;
#ifdef DEBUG_PROGRESSIVE
	if(ret < 0)
		xmlGenericError(0, "FullElem %s failed\n", elem->name);
#endif
	return ret;
}

/************************************************************************
*									*
*		Generic interpreted validation implementation		*
*									*
************************************************************************/
static int xmlRelaxNGValidateValue(xmlRelaxNGValidCtxtPtr ctxt,
    xmlRelaxNGDefinePtr define);

/**
 * xmlRelaxNGSkipIgnored:
 * @ctxt:  a schema validation context
 * @node:  the top node.
 *
 * Skip ignorable nodes in that context
 *
 * Returns the new sibling or NULL in case of error.
 */
static xmlNode * xmlRelaxNGSkipIgnored(xmlRelaxNGValidCtxtPtr ctxt ATTRIBUTE_UNUSED, xmlNode * P_Node)
{
	/*
	 * @todo complete and handle entities
	 */
	while(P_Node && (oneof4(P_Node->type, XML_COMMENT_NODE, XML_PI_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END) ||
		(oneof2(P_Node->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE) && ((ctxt->flags & FLAGS_MIXED_CONTENT) || (IS_BLANK_NODE(P_Node)))))) {
		P_Node = P_Node->next;
	}
	return (P_Node);
}
/**
 * xmlRelaxNGNormalize:
 * @ctxt:  a schema validation context
 * @str:  the string to normalize
 *
 * Implements the  normalizeWhiteSpace( s ) function from
 * section 6.2.9 of the spec
 *
 * Returns the new string or NULL in case of error.
 */
static xmlChar * xmlRelaxNGNormalize(xmlRelaxNGValidCtxtPtr ctxt, const xmlChar * str)
{
	xmlChar * ret = 0;
	xmlChar * p;
	int len;
	if(str) {
		const xmlChar * tmp = str;
		while(*tmp != 0)
			tmp++;
		len = tmp - str;
		ret = (xmlChar*)SAlloc::M((len + 1) * sizeof(xmlChar));
		if(!ret) {
			xmlRngVErrMemory(ctxt, "validating\n");
			return 0;
		}
		p = ret;
		while(IS_BLANK_CH(*str))
			str++;
		while(*str != 0) {
			if(IS_BLANK_CH(*str)) {
				while(IS_BLANK_CH(*str))
					str++;
				if(*str == 0)
					break;
				*p++ = ' ';
			}
			else
				*p++ = *str++;
		}
		*p = 0;
	}
	return ret;
}
/**
 * xmlRelaxNGValidateDatatype:
 * @ctxt:  a Relax-NG validation context
 * @value:  the string value
 * @type:  the datatype definition
 * @node:  the node
 *
 * Validate the given value against the dataype
 *
 * Returns 0 if the validation succeeded or an error code.
 */
static int xmlRelaxNGValidateDatatype(xmlRelaxNGValidCtxtPtr ctxt, const xmlChar * value, xmlRelaxNGDefinePtr define, xmlNode * P_Node)
{
	int ret, tmp;
	xmlRelaxNGTypeLibraryPtr lib;
	void * result = NULL;
	xmlRelaxNGDefinePtr cur;
	if((define == NULL) || (define->data == NULL)) {
		return -1;
	}
	lib = (xmlRelaxNGTypeLibraryPtr)define->data;
	if(lib->check) {
		if(define->attrs && (define->attrs->type == XML_RELAXNG_PARAM)) {
			ret = lib->check(lib->data, define->name, value, &result, P_Node);
		}
		else {
			ret = lib->check(lib->data, define->name, value, NULL, P_Node);
		}
	}
	else
		ret = -1;
	if(ret < 0) {
		VALID_ERR2(XML_RELAXNG_ERR_TYPE, define->name);
		if(result && lib && lib->freef)
			lib->freef(lib->data, result);
		return -1;
	}
	else if(ret == 1) {
		ret = 0;
	}
	else if(ret == 2) {
		VALID_ERR2P(XML_RELAXNG_ERR_DUPID, value);
	}
	else {
		VALID_ERR3P(XML_RELAXNG_ERR_TYPEVAL, define->name, value);
		ret = -1;
	}
	cur = define->attrs;
	while((ret == 0) && cur && (cur->type == XML_RELAXNG_PARAM)) {
		if(lib->facet) {
			tmp = lib->facet(lib->data, define->name, cur->name, cur->value, value, result);
			if(tmp != 0)
				ret = -1;
		}
		cur = cur->next;
	}
	if((ret == 0) && define->content) {
		const xmlChar * oldvalue = ctxt->state->value;
		const xmlChar * oldendvalue = ctxt->state->endvalue;
		ctxt->state->value = (xmlChar*)value;
		ctxt->state->endvalue = NULL;
		ret = xmlRelaxNGValidateValue(ctxt, define->content);
		ctxt->state->value = (xmlChar*)oldvalue;
		ctxt->state->endvalue = (xmlChar*)oldendvalue;
	}
	if(result && lib && lib->freef)
		lib->freef(lib->data, result);
	return ret;
}
/**
 * xmlRelaxNGNextValue:
 * @ctxt:  a Relax-NG validation context
 *
 * Skip to the next value when validating within a list
 *
 * Returns 0 if the operation succeeded or an error code.
 */
static int xmlRelaxNGNextValue(xmlRelaxNGValidCtxtPtr ctxt)
{
	xmlChar * cur = ctxt->state->value;
	if(!cur || (ctxt->state->endvalue == NULL)) {
		ctxt->state->value = NULL;
		ctxt->state->endvalue = NULL;
		return 0;
	}
	while(*cur != 0)
		cur++;
	while((cur != ctxt->state->endvalue) && (*cur == 0))
		cur++;
	ctxt->state->value = (cur == ctxt->state->endvalue) ? NULL : cur;
	return 0;
}
/**
 * xmlRelaxNGValidateValueList:
 * @ctxt:  a Relax-NG validation context
 * @defines:  the list of definitions to verify
 *
 * Validate the given set of definitions for the current value
 *
 * Returns 0 if the validation succeeded or an error code.
 */
static int xmlRelaxNGValidateValueList(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr defines)
{
	int ret = 0;
	for(; !ret && defines; defines = defines->next)
		ret = xmlRelaxNGValidateValue(ctxt, defines);
	return ret;
}
/**
 * xmlRelaxNGValidateValue:
 * @ctxt:  a Relax-NG validation context
 * @define:  the definition to verify
 *
 * Validate the given definition for the current value
 *
 * Returns 0 if the validation succeeded or an error code.
 */
static int xmlRelaxNGValidateValue(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr define)
{
	int ret = 0, oldflags;
	xmlChar * value = ctxt->state->value;
	switch(define->type) {
		case XML_RELAXNG_EMPTY: {
		    if(value && (value[0] != 0)) {
			    int idx = 0;
			    while(IS_BLANK_CH(value[idx]))
				    idx++;
			    if(value[idx] != 0)
				    ret = -1;
		    }
		    break;
	    }
		case XML_RELAXNG_TEXT:
		    break;
		case XML_RELAXNG_VALUE: {
		    if(!sstreq(value, define->value)) {
			    if(define->name) {
				    xmlRelaxNGTypeLibraryPtr lib = (xmlRelaxNGTypeLibraryPtr)define->data;
				    if(lib && lib->comp) {
					    ret = lib->comp(lib->data, define->name, define->value, define->P_Node, (void*)define->attrs, value, ctxt->state->P_Node);
				    }
				    else
					    ret = -1;
				    if(ret < 0) {
					    VALID_ERR2(XML_RELAXNG_ERR_TYPECMP, define->name);
					    return -1;
				    }
				    else if(ret == 1) {
					    ret = 0;
				    }
				    else {
					    ret = -1;
				    }
			    }
			    else {
				    /*
				     * @todo trivial optimizations are possible by computing at compile-time
				     */
				    xmlChar * nval = xmlRelaxNGNormalize(ctxt, define->value);
				    xmlChar * nvalue = xmlRelaxNGNormalize(ctxt, value);
				    if((nval == NULL) || (nvalue == NULL) || (!sstreq(nval, nvalue)))
					    ret = -1;
				    SAlloc::F(nval);
				    SAlloc::F(nvalue);
			    }
		    }
		    if(ret == 0)
			    xmlRelaxNGNextValue(ctxt);
		    break;
	    }
		case XML_RELAXNG_DATATYPE: {
		    ret = xmlRelaxNGValidateDatatype(ctxt, value, define, ctxt->state->seq);
		    if(ret == 0)
			    xmlRelaxNGNextValue(ctxt);
		    break;
	    }
		case XML_RELAXNG_CHOICE: {
		    xmlRelaxNGDefinePtr list = define->content;
		    xmlChar * oldvalue;
		    oldflags = ctxt->flags;
		    ctxt->flags |= FLAGS_IGNORABLE;
		    oldvalue = ctxt->state->value;
		    while(list) {
			    ret = xmlRelaxNGValidateValue(ctxt, list);
			    if(ret == 0) {
				    break;
			    }
			    ctxt->state->value = oldvalue;
			    list = list->next;
		    }
		    ctxt->flags = oldflags;
		    if(ret != 0) {
			    if((ctxt->flags & FLAGS_IGNORABLE) == 0)
				    xmlRelaxNGDumpValidError(ctxt);
		    }
		    else {
			    if(ctxt->errNr > 0)
				    xmlRelaxNGPopErrors(ctxt, 0);
		    }
		    break;
	    }
		case XML_RELAXNG_LIST: {
		    xmlRelaxNGDefinePtr list = define->content;
		    xmlChar * oldvalue, * oldend, * val, * cur;
#ifdef DEBUG_LIST
		    int nb_values = 0;
#endif
		    oldvalue = ctxt->state->value;
		    oldend = ctxt->state->endvalue;
		    val = sstrdup(oldvalue);
			SETIFZ(val, sstrdup(BAD_CAST ""));
		    if(!val) {
			    VALID_ERR(XML_RELAXNG_ERR_NOSTATE);
			    return -1;
		    }
		    cur = val;
		    while(*cur != 0) {
			    if(IS_BLANK_CH(*cur)) {
				    *cur = 0;
				    cur++;
#ifdef DEBUG_LIST
				    nb_values++;
#endif
				    while(IS_BLANK_CH(*cur))
					    *cur++ = 0;
			    }
			    else
				    cur++;
		    }
#ifdef DEBUG_LIST
		    xmlGenericError(0, "list value: '%s' found %d items\n", oldvalue, nb_values);
		    nb_values = 0;
#endif
		    ctxt->state->endvalue = cur;
		    cur = val;
		    while((*cur == 0) && (cur != ctxt->state->endvalue))
			    cur++;

		    ctxt->state->value = cur;
		    while(list) {
			    if(ctxt->state->value == ctxt->state->endvalue)
				    ctxt->state->value = NULL;
			    ret = xmlRelaxNGValidateValue(ctxt, list);
			    if(ret != 0) {
#ifdef DEBUG_LIST
				    xmlGenericError(0, "Failed to validate value: '%s' with %d rule\n", ctxt->state->value, nb_values);
#endif
				    break;
			    }
#ifdef DEBUG_LIST
			    nb_values++;
#endif
			    list = list->next;
		    }
		    if(!ret && ctxt->state->value && (ctxt->state->value != ctxt->state->endvalue)) {
			    VALID_ERR2(XML_RELAXNG_ERR_LISTEXTRA, ctxt->state->value);
			    ret = -1;
		    }
		    SAlloc::F(val);
		    ctxt->state->value = oldvalue;
		    ctxt->state->endvalue = oldend;
		    break;
	    }
		case XML_RELAXNG_ONEORMORE:
		    ret = xmlRelaxNGValidateValueList(ctxt, define->content);
		    if(ret != 0) {
			    break;
		    }
		/* no break on purpose */
		case XML_RELAXNG_ZEROORMORE: {
		    xmlChar * cur, * temp;
		    if((ctxt->state->value == NULL) || (*ctxt->state->value == 0)) {
			    ret = 0;
			    break;
		    }
		    oldflags = ctxt->flags;
		    ctxt->flags |= FLAGS_IGNORABLE;
		    cur = ctxt->state->value;
		    temp = NULL;
		    while(cur && (cur != ctxt->state->endvalue) && (temp != cur)) {
			    temp = cur;
			    ret = xmlRelaxNGValidateValueList(ctxt, define->content);
			    if(ret != 0) {
				    ctxt->state->value = temp;
				    ret = 0;
				    break;
			    }
			    cur = ctxt->state->value;
		    }
		    ctxt->flags = oldflags;
		    if(ctxt->errNr > 0)
			    xmlRelaxNGPopErrors(ctxt, 0);
		    break;
	    }
		case XML_RELAXNG_OPTIONAL: {
		    xmlChar * temp;
		    if((ctxt->state->value == NULL) || (*ctxt->state->value == 0)) {
			    ret = 0;
			    break;
		    }
		    oldflags = ctxt->flags;
		    ctxt->flags |= FLAGS_IGNORABLE;
		    temp = ctxt->state->value;
		    ret = xmlRelaxNGValidateValue(ctxt, define->content);
		    ctxt->flags = oldflags;
		    if(ret != 0) {
			    ctxt->state->value = temp;
			    if(ctxt->errNr > 0)
				    xmlRelaxNGPopErrors(ctxt, 0);
			    ret = 0;
			    break;
		    }
		    if(ctxt->errNr > 0)
			    xmlRelaxNGPopErrors(ctxt, 0);
		    break;
	    }
		case XML_RELAXNG_EXCEPT: {
		    xmlRelaxNGDefine * list = define->content;
		    while(list) {
			    ret = xmlRelaxNGValidateValue(ctxt, list);
			    if(ret == 0) {
				    ret = -1;
				    break;
			    }
			    else
				    ret = 0;
			    list = list->next;
		    }
		    break;
	    }
		case XML_RELAXNG_DEF:
		case XML_RELAXNG_GROUP: {
		    xmlRelaxNGDefine * list = define->content;
		    while(list) {
			    ret = xmlRelaxNGValidateValue(ctxt, list);
			    if(ret != 0) {
				    ret = -1;
				    break;
			    }
			    else
				    ret = 0;
			    list = list->next;
		    }
		    break;
	    }
		case XML_RELAXNG_REF:
		case XML_RELAXNG_PARENTREF:
		    if(define->content == NULL) {
			    VALID_ERR(XML_RELAXNG_ERR_NODEFINE);
			    ret = -1;
		    }
		    else {
			    ret = xmlRelaxNGValidateValue(ctxt, define->content);
		    }
		    break;
		default:
		    TODO ret = -1;
	}
	return ret;
}
/**
 * xmlRelaxNGValidateValueContent:
 * @ctxt:  a Relax-NG validation context
 * @defines:  the list of definitions to verify
 *
 * Validate the given definitions for the current value
 *
 * Returns 0 if the validation succeeded or an error code.
 */
static int xmlRelaxNGValidateValueContent(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr defines)
{
	int ret = 0;
	for(; !ret && defines; defines = defines->next)
		ret = xmlRelaxNGValidateValue(ctxt, defines);
	return ret;
}
/**
 * xmlRelaxNGAttributeMatch:
 * @ctxt:  a Relax-NG validation context
 * @define:  the definition to check
 * @prop:  the attribute
 *
 * Check if the attribute matches the definition nameClass
 *
 * Returns 1 if the attribute matches, 0 if no, or -1 in case of error
 */
static int xmlRelaxNGAttributeMatch(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr define, xmlAttrPtr prop)
{
	int ret;
	if(define->name) {
		if(!sstreq(define->name, prop->name))
			return 0;
	}
	if(define->ns) {
		if(define->ns[0] == 0) {
			if(prop->ns)
				return 0;
		}
		else {
			if((prop->ns == NULL) || (!sstreq(define->ns, prop->ns->href)))
				return 0;
		}
	}
	if(define->nameClass == NULL)
		return 1;
	define = define->nameClass;
	if(define->type == XML_RELAXNG_EXCEPT) {
		xmlRelaxNGDefinePtr list = define->content;
		while(list) {
			ret = xmlRelaxNGAttributeMatch(ctxt, list, prop);
			if(ret == 1)
				return 0;
			if(ret < 0)
				return ret;
			list = list->next;
		}
	}
	else if(define->type == XML_RELAXNG_CHOICE) {
		xmlRelaxNGDefinePtr list = define->nameClass;
		while(list) {
			ret = xmlRelaxNGAttributeMatch(ctxt, list, prop);
			if(ret == 1)
				return 1;
			if(ret < 0)
				return ret;
			list = list->next;
		}
		return 0;
	}
	else {
		TODO
	}
	return 1;
}
/**
 * xmlRelaxNGValidateAttribute:
 * @ctxt:  a Relax-NG validation context
 * @define:  the definition to verify
 *
 * Validate the given attribute definition for that node
 *
 * Returns 0 if the validation succeeded or an error code.
 */
static int xmlRelaxNGValidateAttribute(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr define)
{
	int ret = 0, i;
	xmlChar * value, * oldvalue;
	xmlAttrPtr prop = NULL, tmp;
	xmlNode * oldseq;
	if(ctxt->state->nbAttrLeft <= 0)
		return -1;
	if(define->name) {
		for(i = 0; i < ctxt->state->nbAttrs; i++) {
			tmp = ctxt->state->attrs[i];
			if(tmp && sstreq(define->name, tmp->name)) {
				if((((define->ns == NULL) || (define->ns[0] == 0)) && (tmp->ns == NULL)) || (tmp->ns && sstreq(define->ns, tmp->ns->href))) {
					prop = tmp;
					break;
				}
			}
		}
		if(prop) {
			value = xmlNodeListGetString(prop->doc, prop->children, 1);
			oldvalue = ctxt->state->value;
			oldseq = ctxt->state->seq;
			ctxt->state->seq = (xmlNode *)prop;
			ctxt->state->value = value;
			ctxt->state->endvalue = NULL;
			ret = xmlRelaxNGValidateValueContent(ctxt, define->content);
			if(ctxt->state->value)
				value = ctxt->state->value;
			SAlloc::F(value);
			ctxt->state->value = oldvalue;
			ctxt->state->seq = oldseq;
			if(ret == 0) {
				/*
				 * flag the attribute as processed
				 */
				ctxt->state->attrs[i] = NULL;
				ctxt->state->nbAttrLeft--;
			}
		}
		else {
			ret = -1;
		}
#ifdef DEBUG
		xmlGenericError(0, "xmlRelaxNGValidateAttribute(%s): %d\n", define->name, ret);
#endif
	}
	else {
		for(i = 0; i < ctxt->state->nbAttrs; i++) {
			tmp = ctxt->state->attrs[i];
			if(tmp && (xmlRelaxNGAttributeMatch(ctxt, define, tmp) == 1)) {
				prop = tmp;
				break;
			}
		}
		if(prop) {
			value = xmlNodeListGetString(prop->doc, prop->children, 1);
			oldvalue = ctxt->state->value;
			oldseq = ctxt->state->seq;
			ctxt->state->seq = (xmlNode *)prop;
			ctxt->state->value = value;
			ret = xmlRelaxNGValidateValueContent(ctxt, define->content);
			if(ctxt->state->value)
				value = ctxt->state->value;
			SAlloc::F(value);
			ctxt->state->value = oldvalue;
			ctxt->state->seq = oldseq;
			if(ret == 0) {
				/*
				 * flag the attribute as processed
				 */
				ctxt->state->attrs[i] = NULL;
				ctxt->state->nbAttrLeft--;
			}
		}
		else {
			ret = -1;
		}
#ifdef DEBUG
		if(define->ns) {
			xmlGenericError(0, "xmlRelaxNGValidateAttribute(nsName ns = %s): %d\n", define->ns, ret);
		}
		else {
			xmlGenericError(0, "xmlRelaxNGValidateAttribute(anyName): %d\n", ret);
		}
#endif
	}
	return ret;
}
/**
 * xmlRelaxNGValidateAttributeList:
 * @ctxt:  a Relax-NG validation context
 * @define:  the list of definition to verify
 *
 * Validate the given node against the list of attribute definitions
 *
 * Returns 0 if the validation succeeded or an error code.
 */
static int xmlRelaxNGValidateAttributeList(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr defines)
{
	int ret = 0, res;
	int needmore = 0;
	xmlRelaxNGDefine * cur = defines;
	while(cur) {
		if(cur->type == XML_RELAXNG_ATTRIBUTE) {
			if(xmlRelaxNGValidateAttribute(ctxt, cur) != 0)
				ret = -1;
		}
		else
			needmore = 1;
		cur = cur->next;
	}
	if(!needmore)
		return ret;
	cur = defines;
	while(cur) {
		if(cur->type != XML_RELAXNG_ATTRIBUTE) {
			if(ctxt->state || ctxt->states) {
				res = xmlRelaxNGValidateDefinition(ctxt, cur);
				if(res < 0)
					ret = -1;
			}
			else {
				VALID_ERR(XML_RELAXNG_ERR_NOSTATE);
				return -1;
			}
			if(res == -1) /* continues on -2 */
				break;
		}
		cur = cur->next;
	}
	return ret;
}
/**
 * xmlRelaxNGNodeMatchesList:
 * @node:  the node
 * @list:  a NULL terminated array of definitions
 *
 * Check if a node can be matched by one of the definitions
 *
 * Returns 1 if matches 0 otherwise
 */
static int xmlRelaxNGNodeMatchesList(xmlNode * P_Node, xmlRelaxNGDefinePtr * list)
{
	if(P_Node && list) {
		int i = 0;
		for(xmlRelaxNGDefine * cur = list[i++]; cur;) {
			if((P_Node->type == XML_ELEMENT_NODE) && (cur->type == XML_RELAXNG_ELEMENT)) {
				int tmp = xmlRelaxNGElementMatch(NULL, cur, P_Node);
				if(tmp == 1)
					return 1;
			}
			else if(((P_Node->type == XML_TEXT_NODE) || (P_Node->type == XML_CDATA_SECTION_NODE)) && (cur->type == XML_RELAXNG_TEXT)) {
				return 1;
			}
			cur = list[i++];
		}
	}
	return 0;
}

/**
 * xmlRelaxNGValidateInterleave:
 * @ctxt:  a Relax-NG validation context
 * @define:  the definition to verify
 *
 * Validate an interleave definition for a node.
 *
 * Returns 0 if the validation succeeded or an error code.
 */
static int xmlRelaxNGValidateInterleave(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr define)
{
	int ret = 0, i, nbgroups;
	int errNr = ctxt->errNr;
	int oldflags;
	xmlRelaxNGValidStatePtr oldstate;
	xmlRelaxNGPartitionPtr partitions;
	xmlRelaxNGInterleaveGroupPtr group = NULL;
	xmlNode * cur;
	xmlNode * start;
	xmlNode * last = NULL;
	xmlNode * lastchg = NULL;
	xmlNode * lastelem;
	xmlNode ** list = NULL;
	xmlNode ** lasts = NULL;
	if(define->data) {
		partitions = (xmlRelaxNGPartitionPtr)define->data;
		nbgroups = partitions->nbgroups;
	}
	else {
		VALID_ERR(XML_RELAXNG_ERR_INTERNODATA);
		return -1;
	}
	/*
	 * Optimizations for MIXED
	 */
	oldflags = ctxt->flags;
	if(define->dflags & IS_MIXED) {
		ctxt->flags |= FLAGS_MIXED_CONTENT;
		if(nbgroups == 2) {
			/*
			 * this is a pure <mixed> case
			 */
			if(ctxt->state)
				ctxt->state->seq = xmlRelaxNGSkipIgnored(ctxt, ctxt->state->seq);
			if(partitions->groups[0]->rule->type == XML_RELAXNG_TEXT)
				ret = xmlRelaxNGValidateDefinition(ctxt, partitions->groups[1]->rule);
			else
				ret = xmlRelaxNGValidateDefinition(ctxt, partitions->groups[0]->rule);
			if(ret == 0) {
				if(ctxt->state)
					ctxt->state->seq = xmlRelaxNGSkipIgnored(ctxt, ctxt->state->seq);
			}
			ctxt->flags = oldflags;
			return ret;
		}
	}
	/*
	 * Build arrays to store the first and last node of the chain
	 * pertaining to each group
	 */
	list = (xmlNode **)SAlloc::M(nbgroups * sizeof(xmlNode *));
	if(list == NULL) {
		xmlRngVErrMemory(ctxt, "validating\n");
		return -1;
	}
	memzero(list, nbgroups * sizeof(xmlNode *));
	lasts = (xmlNode **)SAlloc::M(nbgroups * sizeof(xmlNode *));
	if(lasts == NULL) {
		xmlRngVErrMemory(ctxt, "validating\n");
		return -1;
	}
	memzero(lasts, nbgroups * sizeof(xmlNode *));
	/*
	 * Walk the sequence of children finding the right group and
	 * sorting them in sequences.
	 */
	cur = ctxt->state->seq;
	cur = xmlRelaxNGSkipIgnored(ctxt, cur);
	start = cur;
	while(cur) {
		ctxt->state->seq = cur;
		if(partitions->triage && (partitions->flags & IS_DETERMINIST)) {
			void * tmp = NULL;
			if((cur->type == XML_TEXT_NODE) || (cur->type == XML_CDATA_SECTION_NODE)) {
				tmp = xmlHashLookup2(partitions->triage, BAD_CAST "#text", NULL);
			}
			else if(cur->type == XML_ELEMENT_NODE) {
				if(cur->ns) {
					tmp = xmlHashLookup2(partitions->triage, cur->name, cur->ns->href);
					SETIFZ(tmp, xmlHashLookup2(partitions->triage, BAD_CAST "#any", cur->ns->href));
				}
				else
					tmp = xmlHashLookup2(partitions->triage, cur->name, NULL);
				SETIFZ(tmp, xmlHashLookup2(partitions->triage, BAD_CAST "#any", NULL));
			}
			if(!tmp) {
				i = nbgroups;
			}
			else {
				i = ((long)tmp) - 1;
				if(partitions->flags & IS_NEEDCHECK) {
					group = partitions->groups[i];
					if(!xmlRelaxNGNodeMatchesList(cur, group->defs))
						i = nbgroups;
				}
			}
		}
		else {
			for(i = 0; i < nbgroups; i++) {
				group = partitions->groups[i];
				if(group) {
					if(xmlRelaxNGNodeMatchesList(cur, group->defs))
						break;
				}
			}
		}
		/*
		 * We break as soon as an element not matched is found
		 */
		if(i >= nbgroups) {
			break;
		}
		if(lasts[i]) {
			lasts[i]->next = cur;
			lasts[i] = cur;
		}
		else {
			list[i] = cur;
			lasts[i] = cur;
		}
		lastchg = cur->next ? cur->next : cur;
		cur = xmlRelaxNGSkipIgnored(ctxt, cur->next);
	}
	if(ret != 0) {
		VALID_ERR(XML_RELAXNG_ERR_INTERSEQ);
		ret = -1;
		goto done;
	}
	lastelem = cur;
	oldstate = ctxt->state;
	for(i = 0; i < nbgroups; i++) {
		ctxt->state = xmlRelaxNGCopyValidState(ctxt, oldstate);
		if(ctxt->state == NULL) {
			ret = -1;
			break;
		}
		group = partitions->groups[i];
		if(lasts[i]) {
			last = lasts[i]->next;
			lasts[i]->next = NULL;
		}
		ctxt->state->seq = list[i];
		ret = xmlRelaxNGValidateDefinition(ctxt, group->rule);
		if(ret != 0)
			break;
		if(ctxt->state) {
			cur = ctxt->state->seq;
			cur = xmlRelaxNGSkipIgnored(ctxt, cur);
			xmlRelaxNGFreeValidState(ctxt, oldstate);
			oldstate = ctxt->state;
			ctxt->state = NULL;
			if(cur) {
				VALID_ERR2(XML_RELAXNG_ERR_INTEREXTRA, cur->name);
				ret = -1;
				ctxt->state = oldstate;
				goto done;
			}
		}
		else if(ctxt->states) {
			int j;
			int found = 0;
			int best = -1;
			int lowattr = -1;
			/*
			 * PBM: what happen if there is attributes checks in the interleaves
			 */
			for(j = 0; j < ctxt->states->nbState; j++) {
				cur = ctxt->states->tabState[j]->seq;
				cur = xmlRelaxNGSkipIgnored(ctxt, cur);
				if(!cur) {
					if(found == 0) {
						lowattr = ctxt->states->tabState[j]->nbAttrLeft;
						best = j;
					}
					found = 1;
					if(ctxt->states->tabState[j]->nbAttrLeft <= lowattr) {
						/* try  to keep the latest one to mach old heuristic */
						lowattr = ctxt->states->tabState[j]->nbAttrLeft;
						best = j;
					}
					if(lowattr == 0)
						break;
				}
				else if(found == 0) {
					if(lowattr == -1) {
						lowattr = ctxt->states->tabState[j]->nbAttrLeft;
						best = j;
					}
					else if(ctxt->states->tabState[j]->nbAttrLeft <= lowattr) {
						/* try  to keep the latest one to mach old heuristic */
						lowattr = ctxt->states->tabState[j]->nbAttrLeft;
						best = j;
					}
				}
			}
			/*
			 * BIG PBM: here we pick only one restarting point :-(
			 */
			if(ctxt->states->nbState > 0) {
				xmlRelaxNGFreeValidState(ctxt, oldstate);
				if(best != -1) {
					oldstate = ctxt->states->tabState[best];
					ctxt->states->tabState[best] = NULL;
				}
				else {
					oldstate = ctxt->states->tabState[ctxt->states->nbState - 1];
					ctxt->states->tabState[ctxt->states->nbState - 1] = NULL;
					ctxt->states->nbState--;
				}
			}
			for(j = 0; j < ctxt->states->nbState; j++) {
				xmlRelaxNGFreeValidState(ctxt, ctxt->states->tabState[j]);
			}
			xmlRelaxNGFreeStates(ctxt, ctxt->states);
			ctxt->states = NULL;
			if(found == 0) {
				if(!cur) {
					VALID_ERR2(XML_RELAXNG_ERR_INTEREXTRA, (const xmlChar*)"noname");
				}
				else {
					VALID_ERR2(XML_RELAXNG_ERR_INTEREXTRA, cur->name);
				}
				ret = -1;
				ctxt->state = oldstate;
				goto done;
			}
		}
		else {
			ret = -1;
			break;
		}
		if(lasts[i])
			lasts[i]->next = last;
	}
	xmlRelaxNGFreeValidState(ctxt, ctxt->state);
	ctxt->state = oldstate;
	ctxt->state->seq = lastelem;
	if(ret) {
		VALID_ERR(XML_RELAXNG_ERR_INTERSEQ);
		ret = -1;
		goto done;
	}

done:
	ctxt->flags = oldflags;
	/*
	 * builds the next links chain from the prev one
	 */
	cur = lastchg;
	while(cur) {
		if((cur == start) || (cur->prev == NULL))
			break;
		cur->prev->next = cur;
		cur = cur->prev;
	}
	if(ret == 0) {
		if(ctxt->errNr > errNr)
			xmlRelaxNGPopErrors(ctxt, errNr);
	}
	SAlloc::F(list);
	SAlloc::F(lasts);
	return ret;
}
/**
 * xmlRelaxNGValidateDefinitionList:
 * @ctxt:  a Relax-NG validation context
 * @define:  the list of definition to verify
 *
 * Validate the given node content against the (list) of definitions
 *
 * Returns 0 if the validation succeeded or an error code.
 */
static int xmlRelaxNGValidateDefinitionList(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr defines)
{
	int ret = 0, res;
	if(defines == NULL) {
		VALID_ERR2(XML_RELAXNG_ERR_INTERNAL, BAD_CAST "NULL definition list");
		return -1;
	}
	while(defines) {
		if(ctxt->state || ctxt->states) {
			res = xmlRelaxNGValidateDefinition(ctxt, defines);
			if(res < 0)
				ret = -1;
		}
		else {
			VALID_ERR(XML_RELAXNG_ERR_NOSTATE);
			return -1;
		}
		if(res == -1)   /* continues on -2 */
			break;
		defines = defines->next;
	}
	return ret;
}
/**
 * xmlRelaxNGElementMatch:
 * @ctxt:  a Relax-NG validation context
 * @define:  the definition to check
 * @elem:  the element
 *
 * Check if the element matches the definition nameClass
 *
 * Returns 1 if the element matches, 0 if no, or -1 in case of error
 */
static int xmlRelaxNGElementMatch(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr define, xmlNode * elem)
{
	int ret = 0, oldflags = 0;
	if(define->name) {
		if(!sstreq(elem->name, define->name)) {
			VALID_ERR3(XML_RELAXNG_ERR_ELEMNAME, define->name, elem->name);
			return 0;
		}
	}
	if(define->ns && (define->ns[0] != 0)) {
		if(elem->ns == NULL) {
			VALID_ERR2(XML_RELAXNG_ERR_ELEMNONS, elem->name);
			return 0;
		}
		else if(!sstreq(elem->ns->href, define->ns)) {
			VALID_ERR3(XML_RELAXNG_ERR_ELEMWRONGNS, elem->name, define->ns);
			return 0;
		}
	}
	else if(elem->ns && define->ns && (define->name == NULL)) {
		VALID_ERR2(XML_RELAXNG_ERR_ELEMEXTRANS, elem->name);
		return 0;
	}
	else if(elem->ns && define->name) {
		VALID_ERR2(XML_RELAXNG_ERR_ELEMEXTRANS, define->name);
		return 0;
	}
	if(define->nameClass == NULL)
		return 1;
	define = define->nameClass;
	if(define->type == XML_RELAXNG_EXCEPT) {
		xmlRelaxNGDefinePtr list;
		if(ctxt) {
			oldflags = ctxt->flags;
			ctxt->flags |= FLAGS_IGNORABLE;
		}
		list = define->content;
		while(list) {
			ret = xmlRelaxNGElementMatch(ctxt, list, elem);
			if(ret == 1) {
				if(ctxt)
					ctxt->flags = oldflags;
				return 0;
			}
			if(ret < 0) {
				if(ctxt)
					ctxt->flags = oldflags;
				return ret;
			}
			list = list->next;
		}
		ret = 1;
		if(ctxt) {
			ctxt->flags = oldflags;
		}
	}
	else if(define->type == XML_RELAXNG_CHOICE) {
		xmlRelaxNGDefinePtr list;
		if(ctxt) {
			oldflags = ctxt->flags;
			ctxt->flags |= FLAGS_IGNORABLE;
		}
		list = define->nameClass;
		while(list) {
			ret = xmlRelaxNGElementMatch(ctxt, list, elem);
			if(ret == 1) {
				if(ctxt)
					ctxt->flags = oldflags;
				return 1;
			}
			if(ret < 0) {
				if(ctxt)
					ctxt->flags = oldflags;
				return ret;
			}
			list = list->next;
		}
		if(ctxt) {
			if(ret != 0) {
				if((ctxt->flags & FLAGS_IGNORABLE) == 0)
					xmlRelaxNGDumpValidError(ctxt);
			}
			else {
				if(ctxt->errNr > 0)
					xmlRelaxNGPopErrors(ctxt, 0);
			}
		}
		ret = 0;
		if(ctxt)
			ctxt->flags = oldflags;
	}
	else {
		TODO ret = -1;
	}
	return ret;
}

/**
 * xmlRelaxNGBestState:
 * @ctxt:  a Relax-NG validation context
 *
 * Find the "best" state in the ctxt->states list of states to report
 * errors about. I.e. a state with no element left in the child list
 * or the one with the less attributes left.
 * This is called only if a falidation error was detected
 *
 * Returns the index of the "best" state or -1 in case of error
 */
static int xmlRelaxNGBestState(xmlRelaxNGValidCtxtPtr ctxt)
{
	int best = -1;
	if(ctxt && ctxt->states && ctxt->states->nbState > 0) {
		int value = 1000000;
		for(int i = 0; i < ctxt->states->nbState; i++) {
			xmlRelaxNGValidState * state = ctxt->states->tabState[i];
			if(state) {
				if(state->seq) {
					if((best == -1) || (value > 100000)) {
						value = 100000;
						best = i;
					}
				}
				else {
					int tmp = state->nbAttrLeft;
					if((best == -1) || (value > tmp)) {
						value = tmp;
						best = i;
					}
				}
			}
		}
	}
	return (best);
}
/**
 * xmlRelaxNGLogBestError:
 * @ctxt:  a Relax-NG validation context
 *
 * Find the "best" state in the ctxt->states list of states to report
 * errors about and log it.
 */
static void xmlRelaxNGLogBestError(xmlRelaxNGValidCtxtPtr ctxt)
{
	int best;
	if(!ctxt || (ctxt->states == NULL) || (ctxt->states->nbState <= 0))
		return;
	best = xmlRelaxNGBestState(ctxt);
	if((best >= 0) && (best < ctxt->states->nbState)) {
		ctxt->state = ctxt->states->tabState[best];
		xmlRelaxNGValidateElementEnd(ctxt, 1);
	}
}
/**
 * xmlRelaxNGValidateElementEnd:
 * @ctxt:  a Relax-NG validation context
 * @dolog:  indicate that error logging should be done
 *
 * Validate the end of the element, implements check that
 * there is nothing left not consumed in the element content
 * or in the attribute list.
 *
 * Returns 0 if the validation succeeded or an error code.
 */
static int xmlRelaxNGValidateElementEnd(xmlRelaxNGValidCtxtPtr ctxt, int dolog)
{
	int i;
	xmlRelaxNGValidState * state = ctxt->state;
	if(state->seq) {
		state->seq = xmlRelaxNGSkipIgnored(ctxt, state->seq);
		if(state->seq) {
			if(dolog) {
				VALID_ERR3(XML_RELAXNG_ERR_EXTRACONTENT, state->P_Node->name, state->seq->name);
			}
			return -1;
		}
	}
	for(i = 0; i < state->nbAttrs; i++) {
		if(state->attrs[i]) {
			if(dolog) {
				VALID_ERR3(XML_RELAXNG_ERR_INVALIDATTR, state->attrs[i]->name, state->P_Node->name);
			}
			return (-1 - i);
		}
	}
	return 0;
}
/**
 * xmlRelaxNGValidateState:
 * @ctxt:  a Relax-NG validation context
 * @define:  the definition to verify
 *
 * Validate the current state against the definition
 *
 * Returns 0 if the validation succeeded or an error code.
 */
static int xmlRelaxNGValidateState(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr define)
{
	xmlNode * P_Node;
	int ret = 0, i, tmp, oldflags, errNr;
	xmlRelaxNGValidStatePtr oldstate = NULL, state;
	if(define == NULL) {
		VALID_ERR(XML_RELAXNG_ERR_NODEFINE);
		return -1;
	}
	P_Node = ctxt->state ? ctxt->state->seq : NULL;
#ifdef DEBUG
	for(i = 0; i < ctxt->depth; i++)
		xmlGenericError(0, " ");
	xmlGenericError(0, "Start validating %s ", xmlRelaxNGDefName(define));
	if(define->name)
		xmlGenericError(0, "%s ", define->name);
	if(node && node->name)
		xmlGenericError(0, "on %s\n", node->name);
	else
		xmlGenericError(0, "\n");
#endif
	ctxt->depth++;
	switch(define->type) {
		case XML_RELAXNG_EMPTY:
		    P_Node = xmlRelaxNGSkipIgnored(ctxt, P_Node);
		    ret = 0;
		    break;
		case XML_RELAXNG_NOT_ALLOWED:
		    ret = -1;
		    break;
		case XML_RELAXNG_TEXT:
		    while(P_Node && oneof4(P_Node->type, XML_TEXT_NODE, XML_COMMENT_NODE, XML_PI_NODE, XML_CDATA_SECTION_NODE))
			    P_Node = P_Node->next;
		    ctxt->state->seq = P_Node;
		    break;
		case XML_RELAXNG_ELEMENT:
		    errNr = ctxt->errNr;
		    P_Node = xmlRelaxNGSkipIgnored(ctxt, P_Node);
		    if(!P_Node) {
			    VALID_ERR2(XML_RELAXNG_ERR_NOELEM, define->name);
			    ret = -1;
			    if((ctxt->flags & FLAGS_IGNORABLE) == 0)
				    xmlRelaxNGDumpValidError(ctxt);
			    break;
		    }
		    if(P_Node->type != XML_ELEMENT_NODE) {
			    VALID_ERR(XML_RELAXNG_ERR_NOTELEM);
			    ret = -1;
			    if((ctxt->flags & FLAGS_IGNORABLE) == 0)
				    xmlRelaxNGDumpValidError(ctxt);
			    break;
		    }
		    /*
		     * This node was already validated successfully against
		     * this definition.
		     */
		    if(P_Node->psvi == define) {
			    ctxt->state->seq = xmlRelaxNGSkipIgnored(ctxt, P_Node->next);
			    if(ctxt->errNr > errNr)
				    xmlRelaxNGPopErrors(ctxt, errNr);
			    if(ctxt->errNr != 0) {
				    while(ctxt->err && (((ctxt->err->err == XML_RELAXNG_ERR_ELEMNAME) && (sstreq(ctxt->err->arg2, P_Node->name)))
					    || ((ctxt->err->err == XML_RELAXNG_ERR_ELEMEXTRANS) && (sstreq(ctxt->err->arg1, P_Node->name)))
					    || (ctxt->err->err == XML_RELAXNG_ERR_NOELEM) || (ctxt->err->err == XML_RELAXNG_ERR_NOTELEM)))
					    xmlRelaxNGValidErrorPop(ctxt);
			    }
			    break;
		    }
		    ret = xmlRelaxNGElementMatch(ctxt, define, P_Node);
		    if(ret <= 0) {
			    ret = -1;
			    if((ctxt->flags & FLAGS_IGNORABLE) == 0)
				    xmlRelaxNGDumpValidError(ctxt);
			    break;
		    }
		    ret = 0;
		    if(ctxt->errNr != 0) {
			    if(ctxt->errNr > errNr)
				    xmlRelaxNGPopErrors(ctxt, errNr);
			    while(ctxt->err && ((ctxt->err->err == XML_RELAXNG_ERR_ELEMNAME && sstreq(ctxt->err->arg2, P_Node->name)) ||
				    (ctxt->err->err == XML_RELAXNG_ERR_ELEMEXTRANS && sstreq(ctxt->err->arg1, P_Node->name)) ||
				    oneof2(ctxt->err->err, XML_RELAXNG_ERR_NOELEM, XML_RELAXNG_ERR_NOTELEM)))
				    xmlRelaxNGValidErrorPop(ctxt);
		    }
		    errNr = ctxt->errNr;
		    oldflags = ctxt->flags;
		    if(ctxt->flags & FLAGS_MIXED_CONTENT) {
			    ctxt->flags -= FLAGS_MIXED_CONTENT;
		    }
		    state = xmlRelaxNGNewValidState(ctxt, P_Node);
		    if(state == NULL) {
			    ret = -1;
			    if((ctxt->flags & FLAGS_IGNORABLE) == 0)
				    xmlRelaxNGDumpValidError(ctxt);
			    break;
		    }
		    oldstate = ctxt->state;
		    ctxt->state = state;
		    if(define->attrs) {
			    tmp = xmlRelaxNGValidateAttributeList(ctxt, define->attrs);
			    if(tmp != 0) {
				    ret = -1;
				    VALID_ERR2(XML_RELAXNG_ERR_ATTRVALID, P_Node->name);
			    }
		    }
		    if(define->contModel) {
			    xmlRelaxNGValidState * tmpstate = ctxt->state;
			    xmlRelaxNGStates * tmpstates = ctxt->states;
			    xmlNode * nseq;
			    xmlRelaxNGValidState * nstate = xmlRelaxNGNewValidState(ctxt, P_Node);
			    ctxt->state = nstate;
			    ctxt->states = NULL;
			    tmp = xmlRelaxNGValidateCompiledContent(ctxt, define->contModel, ctxt->state->seq);
			    nseq = ctxt->state->seq;
			    ctxt->state = tmpstate;
			    ctxt->states = tmpstates;
			    xmlRelaxNGFreeValidState(ctxt, nstate);

#ifdef DEBUG_COMPILE
			    xmlGenericError(0, "Validating content of '%s' : %d\n", define->name, tmp);
#endif
			    if(tmp != 0)
				    ret = -1;
			    if(ctxt->states) {
				    tmp = -1;
				    for(i = 0; i < ctxt->states->nbState; i++) {
					    state = ctxt->states->tabState[i];
					    ctxt->state = state;
					    ctxt->state->seq = nseq;
					    if(xmlRelaxNGValidateElementEnd(ctxt, 0) == 0) {
						    tmp = 0;
						    break;
					    }
				    }
				    if(tmp != 0) {
					    /*
					     * validation error, log the message for the "best" one
					     */
					    ctxt->flags |= FLAGS_IGNORABLE;
					    xmlRelaxNGLogBestError(ctxt);
				    }
				    for(i = 0; i < ctxt->states->nbState; i++) {
					    xmlRelaxNGFreeValidState(ctxt, ctxt->states->tabState[i]);
				    }
				    xmlRelaxNGFreeStates(ctxt, ctxt->states);
				    ctxt->flags = oldflags;
				    ctxt->states = NULL;
				    if((ret == 0) && (tmp == -1))
					    ret = -1;
			    }
			    else {
				    state = ctxt->state;
				    if(ctxt->state)
					    ctxt->state->seq = nseq;
					SETIFZ(ret, xmlRelaxNGValidateElementEnd(ctxt, 1));
				    xmlRelaxNGFreeValidState(ctxt, state);
			    }
		    }
		    else {
			    if(define->content) {
				    tmp = xmlRelaxNGValidateDefinitionList(ctxt, define->content);
				    if(tmp) {
					    ret = -1;
					    if(ctxt->state == NULL) {
						    ctxt->state = oldstate;
						    VALID_ERR2(XML_RELAXNG_ERR_CONTENTVALID, P_Node->name);
						    ctxt->state = NULL;
					    }
					    else {
						    VALID_ERR2(XML_RELAXNG_ERR_CONTENTVALID, P_Node->name);
					    }
				    }
			    }
			    if(ctxt->states) {
				    tmp = -1;
				    for(i = 0; i < ctxt->states->nbState; i++) {
					    state = ctxt->states->tabState[i];
					    ctxt->state = state;
					    if(xmlRelaxNGValidateElementEnd(ctxt, 0) == 0) {
						    tmp = 0;
						    break;
					    }
				    }
				    if(tmp != 0) {
					    /*
					     * validation error, log the message for the "best" one
					     */
					    ctxt->flags |= FLAGS_IGNORABLE;
					    xmlRelaxNGLogBestError(ctxt);
				    }
				    for(i = 0; i < ctxt->states->nbState; i++) {
					    xmlRelaxNGFreeValidState(ctxt, ctxt->states->tabState[i]);
					    ctxt->states->tabState[i] = NULL;
				    }
				    xmlRelaxNGFreeStates(ctxt, ctxt->states);
				    ctxt->flags = oldflags;
				    ctxt->states = NULL;
				    if((ret == 0) && (tmp == -1))
					    ret = -1;
			    }
			    else {
				    state = ctxt->state;
					SETIFZ(ret, xmlRelaxNGValidateElementEnd(ctxt, 1));
					xmlRelaxNGFreeValidState(ctxt, state);
			    }
		    }
		    if(ret == 0) {
			    P_Node->psvi = define;
		    }
		    ctxt->flags = oldflags;
		    ctxt->state = oldstate;
		    if(oldstate)
			    oldstate->seq = xmlRelaxNGSkipIgnored(ctxt, P_Node->next);
		    if(ret != 0) {
			    if((ctxt->flags & FLAGS_IGNORABLE) == 0) {
				    xmlRelaxNGDumpValidError(ctxt);
				    ret = 0;
#if 0
			    }
			    else {
				    ret = -2;
#endif
			    }
		    }
		    else {
			    if(ctxt->errNr > errNr)
				    xmlRelaxNGPopErrors(ctxt, errNr);
		    }

#ifdef DEBUG
		    xmlGenericError(0, "xmlRelaxNGValidateDefinition(): validated %s : %d", node->name, ret);
		    if(oldstate == NULL)
			    xmlGenericError(0, ": no state\n");
		    else if(oldstate->seq == NULL)
			    xmlGenericError(0, ": done\n");
		    else if(oldstate->seq->type == XML_ELEMENT_NODE)
			    xmlGenericError(0, ": next elem %s\n", oldstate->seq->name);
		    else
			    xmlGenericError(0, ": next %s %d\n", oldstate->seq->name, oldstate->seq->type);
#endif
		    break;
		case XML_RELAXNG_OPTIONAL: {
		    errNr = ctxt->errNr;
		    oldflags = ctxt->flags;
		    ctxt->flags |= FLAGS_IGNORABLE;
		    oldstate = xmlRelaxNGCopyValidState(ctxt, ctxt->state);
		    ret = xmlRelaxNGValidateDefinitionList(ctxt, define->content);
		    if(ret != 0) {
			    xmlRelaxNGFreeValidState(ctxt, ctxt->state);
			    ctxt->state = oldstate;
			    ctxt->flags = oldflags;
			    ret = 0;
			    if(ctxt->errNr > errNr)
				    xmlRelaxNGPopErrors(ctxt, errNr);
			    break;
		    }
		    if(ctxt->states) {
			    xmlRelaxNGAddStates(ctxt, ctxt->states, oldstate);
		    }
		    else {
			    ctxt->states = xmlRelaxNGNewStates(ctxt, 1);
			    if(ctxt->states == NULL) {
				    xmlRelaxNGFreeValidState(ctxt, oldstate);
				    ctxt->flags = oldflags;
				    ret = -1;
				    if(ctxt->errNr > errNr)
					    xmlRelaxNGPopErrors(ctxt, errNr);
				    break;
			    }
			    xmlRelaxNGAddStates(ctxt, ctxt->states, oldstate);
			    xmlRelaxNGAddStates(ctxt, ctxt->states, ctxt->state);
			    ctxt->state = NULL;
		    }
		    ctxt->flags = oldflags;
		    ret = 0;
		    if(ctxt->errNr > errNr)
			    xmlRelaxNGPopErrors(ctxt, errNr);
		    break;
	    }
		case XML_RELAXNG_ONEORMORE:
		    errNr = ctxt->errNr;
		    ret = xmlRelaxNGValidateDefinitionList(ctxt, define->content);
		    if(ret != 0) {
			    break;
		    }
		    if(ctxt->errNr > errNr)
			    xmlRelaxNGPopErrors(ctxt, errNr);
		/* no break on purpose */
		case XML_RELAXNG_ZEROORMORE: {
		    int progress;
		    xmlRelaxNGStatesPtr states = NULL, res = NULL;
		    int base, j;
		    errNr = ctxt->errNr;
		    res = xmlRelaxNGNewStates(ctxt, 1);
		    if(res == NULL) {
			    ret = -1;
			    break;
		    }
		    /*
		     * All the input states are also exit states
		     */
		    if(ctxt->state) {
			    xmlRelaxNGAddStates(ctxt, res, xmlRelaxNGCopyValidState(ctxt, ctxt->state));
		    }
		    else {
			    for(j = 0; j < ctxt->states->nbState; j++) {
				    xmlRelaxNGAddStates(ctxt, res, xmlRelaxNGCopyValidState(ctxt, ctxt->states->tabState[j]));
			    }
		    }
		    oldflags = ctxt->flags;
		    ctxt->flags |= FLAGS_IGNORABLE;
		    do {
			    progress = 0;
			    base = res->nbState;
			    if(ctxt->states) {
				    states = ctxt->states;
				    for(i = 0; i < states->nbState; i++) {
					    ctxt->state = states->tabState[i];
					    ctxt->states = NULL;
					    ret = xmlRelaxNGValidateDefinitionList(ctxt, define->content);
					    if(ret == 0) {
						    if(ctxt->state) {
							    tmp = xmlRelaxNGAddStates(ctxt, res, ctxt->state);
							    ctxt->state = NULL;
							    if(tmp == 1)
								    progress = 1;
						    }
						    else if(ctxt->states) {
							    for(j = 0; j < ctxt->states->nbState; j++) {
								    tmp = xmlRelaxNGAddStates(ctxt, res, ctxt->states->tabState[j]);
								    if(tmp == 1)
									    progress = 1;
							    }
							    xmlRelaxNGFreeStates(ctxt, ctxt->states);
							    ctxt->states = NULL;
						    }
					    }
					    else {
						    if(ctxt->state) {
							    xmlRelaxNGFreeValidState(ctxt, ctxt->state);
							    ctxt->state = NULL;
						    }
					    }
				    }
			    }
			    else {
				    ret = xmlRelaxNGValidateDefinitionList(ctxt, define->content);
				    if(ret != 0) {
					    xmlRelaxNGFreeValidState(ctxt, ctxt->state);
					    ctxt->state = NULL;
				    }
				    else {
					    base = res->nbState;
					    if(ctxt->state) {
						    tmp = xmlRelaxNGAddStates(ctxt, res, ctxt->state);
						    ctxt->state = NULL;
						    if(tmp == 1)
							    progress = 1;
					    }
					    else if(ctxt->states) {
						    for(j = 0; j < ctxt->states->nbState; j++) {
							    tmp = xmlRelaxNGAddStates(ctxt, res, ctxt->states->tabState[j]);
							    if(tmp == 1)
								    progress = 1;
						    }
						    if(states == NULL) {
							    states = ctxt->states;
						    }
						    else {
							    xmlRelaxNGFreeStates(ctxt, ctxt->states);
						    }
						    ctxt->states = NULL;
					    }
				    }
			    }
			    if(progress) {
				    /*
				     * Collect all the new nodes added at that step
				     * and make them the new node set
				     */
				    if(res->nbState - base == 1) {
					    ctxt->state = xmlRelaxNGCopyValidState(ctxt, res->tabState[base]);
				    }
				    else {
					    if(states == NULL) {
						    xmlRelaxNGNewStates(ctxt, res->nbState - base);
						    states = ctxt->states;
						    if(states == NULL) {
							    progress = 0;
							    break;
						    }
					    }
					    states->nbState = 0;
					    for(i = base; i < res->nbState; i++)
						    xmlRelaxNGAddStates(ctxt, states, xmlRelaxNGCopyValidState(ctxt, res->tabState[i]));
					    ctxt->states = states;
				    }
			    }
		    } while(progress == 1);
		    xmlRelaxNGFreeStates(ctxt, states);
		    ctxt->states = res;
		    ctxt->flags = oldflags;
#if 0
		    /*
		     * errors may have to be propagated back...
		     */
		    if(ctxt->errNr > errNr)
			    xmlRelaxNGPopErrors(ctxt, errNr);
#endif
		    ret = 0;
		    break;
	    }
		case XML_RELAXNG_CHOICE: {
		    xmlRelaxNGDefinePtr list = NULL;
		    xmlRelaxNGStatesPtr states = NULL;
		    P_Node = xmlRelaxNGSkipIgnored(ctxt, P_Node);
		    errNr = ctxt->errNr;
		    if((define->dflags & IS_TRIABLE) && define->data && P_Node) {
			    /*
			     * node == NULL can't be optimized since IS_TRIABLE
			     * doesn't account for choice which may lead to
			     * only attributes.
			     */
			    xmlHashTable * triage = (xmlHashTable *)define->data;
			    /*
			     * Something we can optimize cleanly there is only one
			     * possble branch out !
			     */
			    if((P_Node->type == XML_TEXT_NODE) || (P_Node->type == XML_CDATA_SECTION_NODE)) {
				    list = (xmlRelaxNGDefinePtr)xmlHashLookup2(triage, BAD_CAST "#text", 0);
			    }
			    else if(P_Node->type == XML_ELEMENT_NODE) {
				    if(P_Node->ns) {
					    list = (xmlRelaxNGDefinePtr)xmlHashLookup2(triage, P_Node->name, P_Node->ns->href);
						SETIFZ(list, (xmlRelaxNGDefinePtr)xmlHashLookup2(triage, BAD_CAST "#any", P_Node->ns->href));
				    }
				    else
					    list = (xmlRelaxNGDefinePtr)xmlHashLookup2(triage, P_Node->name, 0);
					SETIFZ(list, (xmlRelaxNGDefinePtr)xmlHashLookup2(triage, BAD_CAST "#any", 0));
			    }
			    if(list == NULL) {
				    ret = -1;
				    VALID_ERR2(XML_RELAXNG_ERR_ELEMWRONG, P_Node->name);
				    break;
			    }
			    ret = xmlRelaxNGValidateDefinition(ctxt, list);
			    if(ret == 0) {
			    }
			    break;
		    }

		    list = define->content;
		    oldflags = ctxt->flags;
		    ctxt->flags |= FLAGS_IGNORABLE;
		    while(list) {
			    oldstate = xmlRelaxNGCopyValidState(ctxt, ctxt->state);
			    ret = xmlRelaxNGValidateDefinition(ctxt, list);
			    if(ret == 0) {
					SETIFZ(states, xmlRelaxNGNewStates(ctxt, 1));
				    if(ctxt->state) {
					    xmlRelaxNGAddStates(ctxt, states, ctxt->state);
				    }
				    else if(ctxt->states) {
					    for(i = 0; i < ctxt->states->nbState; i++) {
						    xmlRelaxNGAddStates(ctxt, states, ctxt->states->tabState[i]);
					    }
					    xmlRelaxNGFreeStates(ctxt, ctxt->states);
					    ctxt->states = NULL;
				    }
			    }
			    else {
				    xmlRelaxNGFreeValidState(ctxt, ctxt->state);
			    }
			    ctxt->state = oldstate;
			    list = list->next;
		    }
		    if(states) {
			    xmlRelaxNGFreeValidState(ctxt, oldstate);
			    ctxt->states = states;
			    ctxt->state = NULL;
			    ret = 0;
		    }
		    else {
			    ctxt->states = NULL;
		    }
		    ctxt->flags = oldflags;
		    if(ret != 0) {
			    if((ctxt->flags & FLAGS_IGNORABLE) == 0) {
				    xmlRelaxNGDumpValidError(ctxt);
			    }
		    }
		    else {
			    if(ctxt->errNr > errNr)
				    xmlRelaxNGPopErrors(ctxt, errNr);
		    }
		    break;
	    }
		case XML_RELAXNG_DEF:
		case XML_RELAXNG_GROUP:
		    ret = xmlRelaxNGValidateDefinitionList(ctxt, define->content);
		    break;
		case XML_RELAXNG_INTERLEAVE:
		    ret = xmlRelaxNGValidateInterleave(ctxt, define);
		    break;
		case XML_RELAXNG_ATTRIBUTE:
		    ret = xmlRelaxNGValidateAttribute(ctxt, define);
		    break;
		case XML_RELAXNG_START:
		case XML_RELAXNG_NOOP:
		case XML_RELAXNG_REF:
		case XML_RELAXNG_EXTERNALREF:
		case XML_RELAXNG_PARENTREF:
		    ret = xmlRelaxNGValidateDefinition(ctxt, define->content);
		    break;
		case XML_RELAXNG_DATATYPE: {
		    xmlNode * child;
		    xmlChar * content = NULL;
		    child = P_Node;
		    while(child) {
			    if(child->type == XML_ELEMENT_NODE) {
				    VALID_ERR2(XML_RELAXNG_ERR_DATAELEM, P_Node->parent->name);
				    ret = -1;
				    break;
			    }
			    else if(oneof2(child->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE)) {
				    content = xmlStrcat(content, child->content);
			    }
			    /* @todo handle entities ... */
			    child = child->next;
		    }
		    if(ret == -1) {
			    SAlloc::F(content);
			    break;
		    }
		    if(content == NULL) {
			    content = sstrdup(BAD_CAST "");
			    if(content == NULL) {
				    xmlRngVErrMemory(ctxt, "validating\n");
				    ret = -1;
				    break;
			    }
		    }
		    ret = xmlRelaxNGValidateDatatype(ctxt, content, define, ctxt->state->seq);
		    if(ret == -1) {
			    VALID_ERR2(XML_RELAXNG_ERR_DATATYPE, define->name);
		    }
		    else if(ret == 0) {
			    ctxt->state->seq = NULL;
		    }
		    SAlloc::F(content);
		    break;
	    }
		case XML_RELAXNG_VALUE: {
		    xmlChar * content = NULL;
		    xmlChar * oldvalue;
		    xmlNode * child = P_Node;
		    while(child) {
			    if(child->type == XML_ELEMENT_NODE) {
				    VALID_ERR2(XML_RELAXNG_ERR_VALELEM, P_Node->parent->name);
				    ret = -1;
				    break;
			    }
			    else if(oneof2(child->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE)) {
				    content = xmlStrcat(content, child->content);
			    }
			    /* @todo handle entities ... */
			    child = child->next;
		    }
		    if(ret == -1) {
			    SAlloc::F(content);
			    break;
		    }
		    if(content == NULL) {
			    content = sstrdup(BAD_CAST "");
			    if(content == NULL) {
				    xmlRngVErrMemory(ctxt, "validating\n");
				    ret = -1;
				    break;
			    }
		    }
		    oldvalue = ctxt->state->value;
		    ctxt->state->value = content;
		    ret = xmlRelaxNGValidateValue(ctxt, define);
		    ctxt->state->value = oldvalue;
		    if(ret == -1) {
			    VALID_ERR2(XML_RELAXNG_ERR_VALUE, define->name);
		    }
		    else if(ret == 0) {
			    ctxt->state->seq = NULL;
		    }
		    SAlloc::F(content);
		    break;
	    }
		case XML_RELAXNG_LIST: {
		    xmlChar * oldvalue, * oldendvalue;
		    int len;
		    /*
		     * Make sure it's only text nodes
		     */
		    xmlChar * content = NULL;
		    xmlNode * child = P_Node;
		    while(child) {
			    if(child->type == XML_ELEMENT_NODE) {
				    VALID_ERR2(XML_RELAXNG_ERR_LISTELEM, P_Node->parent->name);
				    ret = -1;
				    break;
			    }
			    else if(oneof2(child->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE)) {
				    content = xmlStrcat(content, child->content);
			    }
			    /* @todo handle entities ... */
			    child = child->next;
		    }
		    if(ret == -1) {
			    SAlloc::F(content);
			    break;
		    }
		    if(content == NULL) {
			    content = sstrdup(BAD_CAST "");
			    if(content == NULL) {
				    xmlRngVErrMemory(ctxt, "validating\n");
				    ret = -1;
				    break;
			    }
		    }
		    len = sstrlen(content);
		    oldvalue = ctxt->state->value;
		    oldendvalue = ctxt->state->endvalue;
		    ctxt->state->value = content;
		    ctxt->state->endvalue = content + len;
		    ret = xmlRelaxNGValidateValue(ctxt, define);
		    ctxt->state->value = oldvalue;
		    ctxt->state->endvalue = oldendvalue;
		    if(ret == -1) {
			    VALID_ERR(XML_RELAXNG_ERR_LIST);
		    }
		    else if((ret == 0) && P_Node) {
			    ctxt->state->seq = P_Node->next;
		    }
		    SAlloc::F(content);
		    break;
	    }
		case XML_RELAXNG_EXCEPT:
		case XML_RELAXNG_PARAM:
		    TODO ret = -1;
		    break;
	}
	ctxt->depth--;
#ifdef DEBUG
	for(i = 0; i < ctxt->depth; i++)
		xmlGenericError(0, " ");
	xmlGenericError(0, "Validating %s ", xmlRelaxNGDefName(define));
	if(define->name)
		xmlGenericError(0, "%s ", define->name);
	if(ret == 0)
		xmlGenericError(0, "suceeded\n");
	else
		xmlGenericError(0, "failed\n");
#endif
	return ret;
}
/**
 * xmlRelaxNGValidateDefinition:
 * @ctxt:  a Relax-NG validation context
 * @define:  the definition to verify
 *
 * Validate the current node lists against the definition
 *
 * Returns 0 if the validation succeeded or an error code.
 */
static int xmlRelaxNGValidateDefinition(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGDefinePtr define)
{
	xmlRelaxNGStatesPtr states, res;
	int i, j, k, ret, oldflags;
	/*
	 * We should NOT have both ctxt->state and ctxt->states
	 */
	if(ctxt->state && ctxt->states) {
		TODO xmlRelaxNGFreeValidState(ctxt, ctxt->state);
		ctxt->state = NULL;
	}
	if((ctxt->states == NULL) || (ctxt->states->nbState == 1)) {
		if(ctxt->states) {
			ctxt->state = ctxt->states->tabState[0];
			xmlRelaxNGFreeStates(ctxt, ctxt->states);
			ctxt->states = NULL;
		}
		ret = xmlRelaxNGValidateState(ctxt, define);
		if(ctxt->state && ctxt->states) {
			TODO xmlRelaxNGFreeValidState(ctxt, ctxt->state);
			ctxt->state = NULL;
		}
		if(ctxt->states && (ctxt->states->nbState == 1)) {
			ctxt->state = ctxt->states->tabState[0];
			xmlRelaxNGFreeStates(ctxt, ctxt->states);
			ctxt->states = NULL;
		}
		return ret;
	}
	states = ctxt->states;
	ctxt->states = NULL;
	res = NULL;
	j = 0;
	oldflags = ctxt->flags;
	ctxt->flags |= FLAGS_IGNORABLE;
	for(i = 0; i < states->nbState; i++) {
		ctxt->state = states->tabState[i];
		ctxt->states = NULL;
		ret = xmlRelaxNGValidateState(ctxt, define);
		/*
		 * We should NOT have both ctxt->state and ctxt->states
		 */
		if(ctxt->state && ctxt->states) {
			TODO xmlRelaxNGFreeValidState(ctxt, ctxt->state);
			ctxt->state = NULL;
		}
		if(ret == 0) {
			if(ctxt->states == NULL) {
				if(res) {
					/* add the state to the container */
					xmlRelaxNGAddStates(ctxt, res, ctxt->state);
					ctxt->state = NULL;
				}
				else {
					/* add the state directly in states */
					states->tabState[j++] = ctxt->state;
					ctxt->state = NULL;
				}
			}
			else {
				if(res == NULL) {
					/* make it the new container and copy other results */
					res = ctxt->states;
					ctxt->states = NULL;
					for(k = 0; k < j; k++)
						xmlRelaxNGAddStates(ctxt, res, states->tabState[k]);
				}
				else {
					/* add all the new results to res and reff the container */
					for(k = 0; k < ctxt->states->nbState; k++)
						xmlRelaxNGAddStates(ctxt, res, ctxt->states->tabState[k]);
					xmlRelaxNGFreeStates(ctxt, ctxt->states);
					ctxt->states = NULL;
				}
			}
		}
		else {
			if(ctxt->state) {
				xmlRelaxNGFreeValidState(ctxt, ctxt->state);
				ctxt->state = NULL;
			}
			else if(ctxt->states) {
				for(k = 0; k < ctxt->states->nbState; k++)
					xmlRelaxNGFreeValidState(ctxt, ctxt->states->tabState[k]);
				xmlRelaxNGFreeStates(ctxt, ctxt->states);
				ctxt->states = NULL;
			}
		}
	}
	ctxt->flags = oldflags;
	if(res) {
		xmlRelaxNGFreeStates(ctxt, states);
		ctxt->states = res;
		ret = 0;
	}
	else if(j > 1) {
		states->nbState = j;
		ctxt->states = states;
		ret = 0;
	}
	else if(j == 1) {
		ctxt->state = states->tabState[0];
		xmlRelaxNGFreeStates(ctxt, states);
		ret = 0;
	}
	else {
		ret = -1;
		xmlRelaxNGFreeStates(ctxt, states);
		if(ctxt->states) {
			xmlRelaxNGFreeStates(ctxt, ctxt->states);
			ctxt->states = NULL;
		}
	}
	if(ctxt->state && ctxt->states) {
		TODO xmlRelaxNGFreeValidState(ctxt, ctxt->state);
		ctxt->state = NULL;
	}
	return ret;
}
/**
 * xmlRelaxNGValidateDocument:
 * @ctxt:  a Relax-NG validation context
 * @doc:  the document
 *
 * Validate the given document
 *
 * Returns 0 if the validation succeeded or an error code.
 */
static int xmlRelaxNGValidateDocument(xmlRelaxNGValidCtxtPtr ctxt, xmlDoc * doc)
{
	int ret;
	xmlRelaxNGPtr schema;
	xmlRelaxNGGrammarPtr grammar;
	xmlRelaxNGValidStatePtr state;
	xmlNode * P_Node;
	if(!ctxt || (ctxt->schema == NULL) || (doc == NULL))
		return -1;
	ctxt->errNo = XML_RELAXNG_OK;
	schema = ctxt->schema;
	grammar = schema->topgrammar;
	if(grammar == NULL) {
		VALID_ERR(XML_RELAXNG_ERR_NOGRAMMAR);
		return -1;
	}
	state = xmlRelaxNGNewValidState(ctxt, 0);
	ctxt->state = state;
	ret = xmlRelaxNGValidateDefinition(ctxt, grammar->start);
	if(ctxt->state && state->seq) {
		state = ctxt->state;
		P_Node = state->seq;
		P_Node = xmlRelaxNGSkipIgnored(ctxt, P_Node);
		if(P_Node) {
			if(ret != -1) {
				VALID_ERR(XML_RELAXNG_ERR_EXTRADATA);
				ret = -1;
			}
		}
	}
	else if(ctxt->states) {
		int i;
		int tmp = -1;
		for(i = 0; i < ctxt->states->nbState; i++) {
			state = ctxt->states->tabState[i];
			P_Node = state->seq;
			P_Node = xmlRelaxNGSkipIgnored(ctxt, P_Node);
			if(!P_Node)
				tmp = 0;
			xmlRelaxNGFreeValidState(ctxt, state);
		}
		if(tmp == -1) {
			if(ret != -1) {
				VALID_ERR(XML_RELAXNG_ERR_EXTRADATA);
				ret = -1;
			}
		}
	}
	if(ctxt->state) {
		xmlRelaxNGFreeValidState(ctxt, ctxt->state);
		ctxt->state = NULL;
	}
	if(ret != 0)
		xmlRelaxNGDumpValidError(ctxt);
#ifdef DEBUG
	else if(ctxt->errNr != 0) {
		ctxt->error(ctxt->userData, "%d Extra error messages left on stack !\n", ctxt->errNr);
		xmlRelaxNGDumpValidError(ctxt);
	}
#endif
#ifdef LIBXML_VALID_ENABLED
	if(ctxt->idref == 1) {
		xmlValidCtxt vctxt;
		MEMSZERO(vctxt);
		vctxt.valid = 1;
		vctxt.error = ctxt->error;
		vctxt.warning = ctxt->warning;
		vctxt.userData = ctxt->userData;
		if(xmlValidateDocumentFinal(&vctxt, doc) != 1)
			ret = -1;
	}
#endif /* LIBXML_VALID_ENABLED */
	if((ret == 0) && (ctxt->errNo != XML_RELAXNG_OK))
		ret = -1;
	return ret;
}

/**
 * xmlRelaxNGCleanPSVI:
 * @node:  an input element or document
 *
 * Call this routine to speed up XPath computation on static documents.
 * This stamps all the element nodes with the document order
 * Like for line information, the order is kept in the element->content
 * field, the value stored is actually - the node number (starting at -1)
 * to be able to differentiate from line numbers.
 *
 * Returns the number of elements found in the document or -1 in case
 *    of error.
 */
static void xmlRelaxNGCleanPSVI(xmlNode * P_Node) 
{
	if(P_Node && oneof3(P_Node->type, XML_ELEMENT_NODE, XML_DOCUMENT_NODE, XML_HTML_DOCUMENT_NODE)) {
		if(P_Node->type == XML_ELEMENT_NODE)
			P_Node->psvi = NULL;
		for(xmlNode * cur = P_Node->children; cur;) {
			if(cur->type == XML_ELEMENT_NODE) {
				cur->psvi = NULL;
				if(cur->children) {
					cur = cur->children;
					continue;
				}
			}
			if(cur->next) {
				cur = cur->next;
			}
			else {
				do {
					cur = cur->parent;
					if(!cur)
						break;
					if(cur == P_Node) {
						cur = NULL;
						break;
					}
					if(cur->next) {
						cur = cur->next;
						break;
					}
				} while(cur);
			}
		}
	}
}
/************************************************************************
*									*
*			Validation interfaces				*
*									*
************************************************************************/

/**
 * xmlRelaxNGNewValidCtxt:
 * @schema:  a precompiled XML RelaxNGs
 *
 * Create an XML RelaxNGs validation context based on the given schema
 *
 * Returns the validation context or NULL in case of error
 */
xmlRelaxNGValidCtxtPtr xmlRelaxNGNewValidCtxt(xmlRelaxNGPtr schema)
{
	xmlRelaxNGValidCtxtPtr ret = (xmlRelaxNGValidCtxtPtr)SAlloc::M(sizeof(xmlRelaxNGValidCtxt));
	if(!ret) {
		xmlRngVErrMemory(NULL, "building context\n");
	}
	else {
		memzero(ret, sizeof(xmlRelaxNGValidCtxt));
		ret->schema = schema;
		ret->error = xmlGenericError;
		ret->userData = xmlGenericErrorContext;
		ret->errNr = 0;
		ret->errMax = 0;
		ret->err = NULL;
		ret->errTab = NULL;
		if(schema)
			ret->idref = schema->idref;
		ret->states = NULL;
		ret->freeState = NULL;
		ret->freeStates = NULL;
		ret->errNo = XML_RELAXNG_OK;
	}
	return ret;
}
/**
 * xmlRelaxNGFreeValidCtxt:
 * @ctxt:  the schema validation context
 *
 * Free the resources associated to the schema validation context
 */
void xmlRelaxNGFreeValidCtxt(xmlRelaxNGValidCtxtPtr ctxt)
{
	if(ctxt) {
		xmlRelaxNGFreeStates(NULL, ctxt->states);
		if(ctxt->freeState) {
			for(int k = 0; k < ctxt->freeState->nbState; k++) {
				xmlRelaxNGFreeValidState(NULL, ctxt->freeState->tabState[k]);
			}
			xmlRelaxNGFreeStates(NULL, ctxt->freeState);
		}
		if(ctxt->freeStates) {
			for(int k = 0; k < ctxt->freeStatesNr; k++)
				xmlRelaxNGFreeStates(NULL, ctxt->freeStates[k]);
			SAlloc::F(ctxt->freeStates);
		}
		SAlloc::F(ctxt->errTab);
		if(ctxt->elemTab) {
			for(xmlRegExecCtxt * exec = xmlRelaxNGElemPop(ctxt); exec;) {
				xmlRegFreeExecCtxt(exec);
				exec = xmlRelaxNGElemPop(ctxt);
			}
			SAlloc::F(ctxt->elemTab);
		}
		SAlloc::F(ctxt);
	}
}
/**
 * xmlRelaxNGSetValidErrors:
 * @ctxt:  a Relax-NG validation context
 * @err:  the error function
 * @warn: the warning function
 * @ctx: the functions context
 *
 * Set the error and warning callback informations
 */
void xmlRelaxNGSetValidErrors(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGValidityErrorFunc err, xmlRelaxNGValidityWarningFunc warn, void * ctx)
{
	if(ctxt) {
		ctxt->error = err;
		ctxt->warning = warn;
		ctxt->userData = ctx;
		ctxt->serror = NULL;
	}
}
/**
 * xmlRelaxNGSetValidStructuredErrors:
 * @ctxt:  a Relax-NG validation context
 * @serror:  the structured error function
 * @ctx: the functions context
 *
 * Set the structured error callback
 */
void xmlRelaxNGSetValidStructuredErrors(xmlRelaxNGValidCtxtPtr ctxt, xmlStructuredErrorFunc serror, void * ctx)
{
	if(ctxt) {
		ctxt->serror = serror;
		ctxt->error = NULL;
		ctxt->warning = NULL;
		ctxt->userData = ctx;
	}
}
/**
 * xmlRelaxNGGetValidErrors:
 * @ctxt:  a Relax-NG validation context
 * @err:  the error function result
 * @warn: the warning function result
 * @ctx: the functions context result
 *
 * Get the error and warning callback informations
 *
 * Returns -1 in case of error and 0 otherwise
 */
int xmlRelaxNGGetValidErrors(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGValidityErrorFunc * err, xmlRelaxNGValidityWarningFunc * warn, void ** ctx)
{
	if(!ctxt)
		return -1;
	ASSIGN_PTR(err, ctxt->error);
	ASSIGN_PTR(warn, ctxt->warning);
	ASSIGN_PTR(ctx, ctxt->userData);
	return 0;
}
/**
 * xmlRelaxNGValidateDoc:
 * @ctxt:  a Relax-NG validation context
 * @doc:  a parsed document tree
 *
 * Validate a document tree in memory.
 *
 * Returns 0 if the document is valid, a positive error code
 *     number otherwise and -1 in case of internal or API error.
 */
int xmlRelaxNGValidateDoc(xmlRelaxNGValidCtxtPtr ctxt, xmlDoc * doc)
{
	int ret;
	if(!ctxt || (doc == NULL))
		return -1;
	ctxt->doc = doc;
	ret = xmlRelaxNGValidateDocument(ctxt, doc);
	/*
	 * Remove all left PSVI
	 */
	xmlRelaxNGCleanPSVI((xmlNode *)doc);
	/*
	 * @todo build error codes
	 */
	return (ret == -1) ? 1 : ret;
}

#define bottom_relaxng
#include "elfgcchack.h"
#endif /* LIBXML_SCHEMAS_ENABLED */
