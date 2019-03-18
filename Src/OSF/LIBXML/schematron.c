/*
 * schematron.c : implementation of the Schematron schema validity checking
 *
 * See Copyright for the status of this software.
 *
 * Daniel Veillard <daniel@veillard.com>
 */
/*
 * @todo 
 * + double check the semantic, especially
 *      - multiple rules applying in a single pattern/node
 *      - the semantic of libxml2 patterns vs. XSLT production referenced by the spec.
 * + export of results in SVRL
 * + full parsing and coverage of the spec, conformance of the input to the spec
 * + divergences between the draft and the ISO proposed standard :-(
 * + hook and test include
 * + try and compare with the XSLT version
 */
#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop

#ifdef LIBXML_SCHEMATRON_ENABLED
//#include <libxml/xpath.h>
//#include <libxml/xpathInternals.h>
//#include <libxml/pattern.h>
//#include <libxml/schematron.h>

#define SCHEMATRON_PARSE_OPTIONS XML_PARSE_NOENT
#define SCT_OLD_NS reinterpret_cast<const xmlChar *>("http://www.ascc.net/xml/schematron")
#define XML_SCHEMATRON_NS reinterpret_cast<const xmlChar *>("http://purl.oclc.org/dsdl/schematron")

static const xmlChar * xmlSchematronNs = XML_SCHEMATRON_NS;
static const xmlChar * xmlOldSchematronNs = SCT_OLD_NS;

#define IS_SCHEMATRON(P_Node, elem) ((P_Node) && (P_Node->type == XML_ELEMENT_NODE ) && (P_Node->ns) && sstreq(P_Node->name, elem) && \
	((sstreq(P_Node->ns->href, xmlSchematronNs)) || (sstreq(P_Node->ns->href, xmlOldSchematronNs))))

#define NEXT_SCHEMATRON(P_Node)						\
	while(P_Node) {						    \
		if((P_Node->type == XML_ELEMENT_NODE ) && (P_Node->ns) && ((sstreq(P_Node->ns->href, xmlSchematronNs)) || (sstreq(P_Node->ns->href, xmlOldSchematronNs)))) \
			break;							     \
		P_Node = P_Node->next;						 \
	}

/**
 * @todo 
 * macro to flag unimplemented blocks
 */
#define TODO xmlGenericError(0, "Unimplemented block at %s:%d\n", __FILE__, __LINE__);

typedef enum {
	XML_SCHEMATRON_ASSERT = 1,
	XML_SCHEMATRON_REPORT = 2
} xmlSchematronTestType;
/**
 * _xmlSchematronTest:
 *
 * A Schematrons test, either an assert or a report
 */
typedef struct _xmlSchematronTest xmlSchematronTest;
typedef xmlSchematronTest * xmlSchematronTestPtr;
struct _xmlSchematronTest {
	xmlSchematronTestPtr next; /* the next test in the list */
	xmlSchematronTestType type; /* the test type */
	xmlNode * P_Node;        /* the node in the tree */
	xmlChar * test;         /* the expression to test */
	xmlXPathCompExprPtr comp; /* the compiled expression */
	xmlChar * report;       /* the message to report */
};
/**
 * _xmlSchematronRule:
 *
 * A Schematrons rule
 */
typedef struct _xmlSchematronRule xmlSchematronRule;
typedef xmlSchematronRule * xmlSchematronRulePtr;
struct _xmlSchematronRule {
	xmlSchematronRulePtr next; /* the next rule in the list */
	xmlSchematronRulePtr patnext; /* the next rule in the pattern list */
	xmlNode * P_Node;        /* the node in the tree */
	xmlChar * context;      /* the context evaluation rule */
	xmlSchematronTestPtr tests; /* the list of tests */
	xmlPattern * pattern;  /* the compiled pattern associated */
	xmlChar * report;       /* the message to report */
};
/**
 * _xmlSchematronPattern:
 *
 * A Schematrons pattern
 */
typedef struct _xmlSchematronPattern xmlSchematronPattern;
typedef xmlSchematronPattern * xmlSchematronPatternPtr;
struct _xmlSchematronPattern {
	xmlSchematronPatternPtr next; /* the next pattern in the list */
	xmlSchematronRulePtr rules; /* the list of rules */
	xmlChar * name;         /* the name of the pattern */
};

/**
 * _xmlSchematron:
 *
 * A Schematrons definition
 */
struct _xmlSchematron {
	const xmlChar * name;   /* schema name */
	int preserve;           /* was the document passed by the user */
	xmlDoc * doc;          /* pointer to the parsed document */
	int flags;              /* specific to this schematron */
	void * _private;        /* unused by the library */
	xmlDict * dict;        /* the dictionnary used internally */
	const xmlChar * title;  /* the title if any */
	int nbNs;               /* the number of namespaces */
	int nbPattern;          /* the number of patterns */
	xmlSchematronPatternPtr patterns; /* the patterns found */
	xmlSchematronRulePtr rules; /* the rules gathered */
	int nbNamespaces;       /* number of namespaces in the array */
	int maxNamespaces;      /* size of the array */
	const xmlChar ** namespaces; /* the array of namespaces */
};
/**
 * xmlSchematronValidCtxt:
 *
 * A Schematrons validation context
 */
struct _xmlSchematronValidCtxt {
	int type;
	int flags;              /* an or of xmlSchematronValidOptions */
	xmlDict * dict;
	int nberrors;
	int err;
	xmlSchematronPtr schema;
	xmlXPathContextPtr xctxt;
	FILE * outputFile;      /* if using XML_SCHEMATRON_OUT_FILE */
	xmlBuffer * outputBuffer; /* if using XML_SCHEMATRON_OUT_BUFFER */
#ifdef LIBXML_OUTPUT_ENABLED
	xmlOutputWriteCallback iowrite; /* if using XML_SCHEMATRON_OUT_IO */
	xmlOutputCloseCallback ioclose;
#endif
	void * ioctx;
	/* error reporting data */
	void * userData;                 /* user specific data block */
	xmlSchematronValidityErrorFunc error; /* the callback in case of errors */
	xmlSchematronValidityWarningFunc warning; /* callback in case of warning */
	xmlStructuredErrorFunc serror;   /* the structured function */
};

struct _xmlSchematronParserCtxt {
	int type;
	const xmlChar * URL;
	xmlDoc * doc;
	int preserve;           /* Whether the doc should be freed  */
	const char * buffer;
	int size;
	xmlDict * dict;        /* dictionnary for interned string names */
	int nberrors;
	int err;
	xmlXPathContextPtr xctxt; /* the XPath context used for compilation */
	xmlSchematronPtr schema;
	int nbNamespaces;       /* number of namespaces in the array */
	int maxNamespaces;      /* size of the array */
	const xmlChar ** namespaces; /* the array of namespaces */
	int nbIncludes;         /* number of includes in the array */
	int maxIncludes;        /* size of the array */
	xmlNode ** includes;  /* the array of includes */
	/* error reporting data */
	void * userData;                 /* user specific data block */
	xmlSchematronValidityErrorFunc error; /* the callback in case of errors */
	xmlSchematronValidityWarningFunc warning; /* callback in case of warning */
	xmlStructuredErrorFunc serror;   /* the structured function */
};

#define XML_STRON_CTXT_PARSER 1
#define XML_STRON_CTXT_VALIDATOR 2

/************************************************************************
*									*
*			Error reporting					*
*									*
************************************************************************/
/**
 * xmlSchematronPErrMemory:
 * @node: a context node
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void FASTCALL xmlSchematronPErrMemory(xmlSchematronParserCtxtPtr ctxt, const char * extra, xmlNode * P_Node)
{
	if(ctxt)
		ctxt->nberrors++;
	__xmlSimpleError(XML_FROM_SCHEMASP, XML_ERR_NO_MEMORY, P_Node, NULL, extra);
}
/**
 * xmlSchematronPErr:
 * @ctxt: the parsing context
 * @node: the context node
 * @error: the error code
 * @msg: the error message
 * @str1: extra data
 * @str2: extra data
 *
 * Handle a parser error
 */
static void FASTCALL xmlSchematronPErr(xmlSchematronParserCtxtPtr ctxt, xmlNode * P_Node, int error, const char * msg, const xmlChar * str1, const xmlChar * str2)
{
	xmlGenericErrorFunc channel = NULL;
	xmlStructuredErrorFunc schannel = NULL;
	void * data = NULL;
	if(ctxt) {
		ctxt->nberrors++;
		channel = ctxt->error;
		data = ctxt->userData;
		schannel = ctxt->serror;
	}
	__xmlRaiseError(schannel, channel, data, ctxt, P_Node, XML_FROM_SCHEMASP, error, XML_ERR_ERROR, NULL, 0, 
		(const char *)str1, (const char *)str2, NULL, 0, 0, msg, str1, str2);
}
/**
 * xmlSchematronVTypeErrMemory:
 * @node: a context node
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void xmlSchematronVErrMemory(xmlSchematronValidCtxtPtr ctxt, const char * extra, xmlNode * P_Node)
{
	if(ctxt) {
		ctxt->nberrors++;
		ctxt->err = XML_SCHEMAV_INTERNAL;
	}
	__xmlSimpleError(XML_FROM_SCHEMASV, XML_ERR_NO_MEMORY, P_Node, NULL, extra);
}
/************************************************************************
*									*
*		Parsing and compilation of the Schematrontrons		*
*									*
************************************************************************/

/**
 * xmlSchematronAddTest:
 * @ctxt: the schema parsing context
 * @type:  the type of test
 * @rule:  the parent rule
 * @node:  the node hosting the test
 * @test: the associated test
 * @report: the associated report string
 *
 * Add a test to a schematron
 *
 * Returns the new pointer or NULL in case of error
 */
static xmlSchematronTestPtr xmlSchematronAddTest(xmlSchematronParserCtxtPtr ctxt,
    xmlSchematronTestType type, xmlSchematronRulePtr rule, xmlNode * P_Node, xmlChar * test, xmlChar * report)
{
	xmlSchematronTestPtr ret;
	xmlXPathCompExprPtr comp;
	if(!ctxt || !rule || !P_Node || !test)
		return 0;
	/*
	 * try first to compile the test expression
	 */
	comp = xmlXPathCtxtCompile(ctxt->xctxt, test);
	if(comp == NULL) {
		xmlSchematronPErr(ctxt, P_Node, XML_SCHEMAP_NOROOT, "Failed to compile test expression %s", test, 0);
		return 0;
	}
	ret = (xmlSchematronTestPtr)SAlloc::M(sizeof(xmlSchematronTest));
	if(!ret) {
		xmlSchematronPErrMemory(ctxt, "allocating schema test", P_Node);
		return 0;
	}
	memzero(ret, sizeof(xmlSchematronTest));
	ret->type = type;
	ret->P_Node = P_Node;
	ret->test = test;
	ret->comp = comp;
	ret->report = report;
	ret->next = NULL;
	if(rule->tests == NULL) {
		rule->tests = ret;
	}
	else {
		xmlSchematronTest * prev = rule->tests;
		while(prev->next)
			prev = prev->next;
		prev->next = ret;
	}
	return ret;
}
/**
 * xmlSchematronFreeTests:
 * @tests:  a list of tests
 *
 * Free a list of tests.
 */
static void xmlSchematronFreeTests(xmlSchematronTestPtr tests)
{
	xmlSchematronTestPtr next;
	while(tests != NULL) {
		next = tests->next;
		SAlloc::F(tests->test);
		xmlXPathFreeCompExpr(tests->comp);
		SAlloc::F(tests->report);
		SAlloc::F(tests);
		tests = next;
	}
}

/**
 * xmlSchematronAddRule:
 * @ctxt: the schema parsing context
 * @schema:  a schema structure
 * @node:  the node hosting the rule
 * @context: the associated context string
 * @report: the associated report string
 *
 * Add a rule to a schematron
 *
 * Returns the new pointer or NULL in case of error
 */
static xmlSchematronRulePtr xmlSchematronAddRule(xmlSchematronParserCtxtPtr ctxt, xmlSchematronPtr schema,
    xmlSchematronPatternPtr pat, xmlNode * P_Node, xmlChar * context, xmlChar * report)
{
	xmlSchematronRulePtr ret;
	xmlPattern * pattern;
	if(!ctxt || !schema || !P_Node || !context)
		return 0;
	/*
	 * Try first to compile the pattern
	 */
	pattern = xmlPatterncompile(context, ctxt->dict, XML_PATTERN_XPATH, ctxt->namespaces);
	if(pattern == NULL) {
		xmlSchematronPErr(ctxt, P_Node, XML_SCHEMAP_NOROOT, "Failed to compile context expression %s", context, 0);
	}
	ret = (xmlSchematronRulePtr)SAlloc::M(sizeof(xmlSchematronRule));
	if(!ret) {
		xmlSchematronPErrMemory(ctxt, "allocating schema rule", P_Node);
		return 0;
	}
	memzero(ret, sizeof(xmlSchematronRule));
	ret->P_Node = P_Node;
	ret->context = context;
	ret->pattern = pattern;
	ret->report = report;
	ret->next = NULL;
	if(schema->rules == NULL) {
		schema->rules = ret;
	}
	else {
		xmlSchematronRulePtr prev = schema->rules;
		while(prev->next)
			prev = prev->next;
		prev->next = ret;
	}
	ret->patnext = NULL;
	if(pat->rules == NULL) {
		pat->rules = ret;
	}
	else {
		xmlSchematronRulePtr prev = pat->rules;
		while(prev->patnext)
			prev = prev->patnext;
		prev->patnext = ret;
	}
	return ret;
}
/**
 * xmlSchematronFreeRules:
 * @rules:  a list of rules
 *
 * Free a list of rules.
 */
static void xmlSchematronFreeRules(xmlSchematronRulePtr rules)
{
	while(rules != NULL) {
		xmlSchematronRule * next = rules->next;
		if(rules->tests)
			xmlSchematronFreeTests(rules->tests);
		SAlloc::F(rules->context);
		xmlFreePattern(rules->pattern);
		SAlloc::F(rules->report);
		SAlloc::F(rules);
		rules = next;
	}
}
/**
 * xmlSchematronAddPattern:
 * @ctxt: the schema parsing context
 * @schema:  a schema structure
 * @node:  the node hosting the pattern
 * @id: the id or name of the pattern
 *
 * Add a pattern to a schematron
 *
 * Returns the new pointer or NULL in case of error
 */
static xmlSchematronPatternPtr xmlSchematronAddPattern(xmlSchematronParserCtxtPtr ctxt, xmlSchematronPtr schema, xmlNode * P_Node, xmlChar * name)
{
	xmlSchematronPatternPtr ret;
	if(!ctxt || !schema || !P_Node || !name)
		return 0;
	ret = (xmlSchematronPatternPtr)SAlloc::M(sizeof(xmlSchematronPattern));
	if(!ret) {
		xmlSchematronPErrMemory(ctxt, "allocating schema pattern", P_Node);
		return 0;
	}
	memzero(ret, sizeof(xmlSchematronPattern));
	ret->name = name;
	ret->next = NULL;
	if(schema->patterns == NULL) {
		schema->patterns = ret;
	}
	else {
		xmlSchematronPatternPtr prev = schema->patterns;
		while(prev->next)
			prev = prev->next;
		prev->next = ret;
	}
	return ret;
}

/**
 * xmlSchematronFreePatterns:
 * @patterns:  a list of patterns
 *
 * Free a list of patterns.
 */
static void xmlSchematronFreePatterns(xmlSchematronPatternPtr patterns) 
{
	while(patterns) {
		xmlSchematronPattern * next = patterns->next;
		SAlloc::F(patterns->name);
		SAlloc::F(patterns);
		patterns = next;
	}
}

/**
 * xmlSchematronNewSchematron:
 * @ctxt:  a schema validation context
 *
 * Allocate a new Schematron structure.
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static xmlSchematronPtr xmlSchematronNewSchematron(xmlSchematronParserCtxtPtr ctxt)
{
	xmlSchematronPtr ret = (xmlSchematronPtr)SAlloc::M(sizeof(xmlSchematron));
	if(!ret) {
		xmlSchematronPErrMemory(ctxt, "allocating schema", 0);
	}
	else {
		memzero(ret, sizeof(xmlSchematron));
		ret->dict = ctxt->dict;
		xmlDictReference(ret->dict);
	}
	return ret;
}

/**
 * xmlSchematronFree:
 * @schema:  a schema structure
 *
 * Deallocate a Schematron structure.
 */
void xmlSchematronFree(xmlSchematronPtr schema)
{
	if(schema) {
		if(schema->doc && !(schema->preserve))
			xmlFreeDoc(schema->doc);
		SAlloc::F((char**)schema->namespaces);
		xmlSchematronFreeRules(schema->rules);
		xmlSchematronFreePatterns(schema->patterns);
		xmlDictFree(schema->dict);
		SAlloc::F(schema);
	}
}

/**
 * xmlSchematronNewParserCtxt:
 * @URL:  the location of the schema
 *
 * Create an XML Schematrons parse context for that file/resource expected
 * to contain an XML Schematrons file.
 *
 * Returns the parser context or NULL in case of error
 */
xmlSchematronParserCtxtPtr xmlSchematronNewParserCtxt(const char * URL)
{
	xmlSchematronParserCtxtPtr ret;
	if(URL == NULL)
		return 0;
	ret = (xmlSchematronParserCtxtPtr)SAlloc::M(sizeof(xmlSchematronParserCtxt));
	if(!ret) {
		xmlSchematronPErrMemory(NULL, "allocating schema parser context", 0);
		return 0;
	}
	memzero(ret, sizeof(xmlSchematronParserCtxt));
	ret->type = XML_STRON_CTXT_PARSER;
	ret->dict = xmlDictCreate();
	ret->URL = xmlDictLookupSL(ret->dict, (const xmlChar *)URL);
	ret->includes = NULL;
	ret->xctxt = xmlXPathNewContext(NULL);
	if(ret->xctxt == NULL) {
		xmlSchematronPErrMemory(NULL, "allocating schema parser XPath context", 0);
		xmlSchematronFreeParserCtxt(ret);
		return 0;
	}
	ret->xctxt->flags = XML_XPATH_CHECKNS;
	return ret;
}

/**
 * xmlSchematronNewMemParserCtxt:
 * @buffer:  a pointer to a char array containing the schemas
 * @size:  the size of the array
 *
 * Create an XML Schematrons parse context for that memory buffer expected
 * to contain an XML Schematrons file.
 *
 * Returns the parser context or NULL in case of error
 */
xmlSchematronParserCtxtPtr xmlSchematronNewMemParserCtxt(const char * buffer, int size)
{
	xmlSchematronParserCtxtPtr ret = 0;
	if(buffer && size > 0) {
		ret = (xmlSchematronParserCtxtPtr)SAlloc::M(sizeof(xmlSchematronParserCtxt));
		if(!ret) {
			xmlSchematronPErrMemory(NULL, "allocating schema parser context", 0);
		}
		else {
			memzero(ret, sizeof(xmlSchematronParserCtxt));
			ret->buffer = buffer;
			ret->size = size;
			ret->dict = xmlDictCreate();
			ret->xctxt = xmlXPathNewContext(NULL);
			if(ret->xctxt == NULL) {
				xmlSchematronPErrMemory(NULL, "allocating schema parser XPath context", 0);
				xmlSchematronFreeParserCtxt(ret);
				ret = 0;
			}
		}
	}
	return ret;
}

/**
 * xmlSchematronNewDocParserCtxt:
 * @doc:  a preparsed document tree
 *
 * Create an XML Schematrons parse context for that document.
 * NB. The document may be modified during the parsing process.
 *
 * Returns the parser context or NULL in case of error
 */
xmlSchematronParserCtxtPtr xmlSchematronNewDocParserCtxt(xmlDoc * doc)
{
	xmlSchematronParserCtxtPtr ret = 0;
	if(doc) {
		ret = (xmlSchematronParserCtxtPtr)SAlloc::M(sizeof(xmlSchematronParserCtxt));
		if(!ret) {
			xmlSchematronPErrMemory(NULL, "allocating schema parser context", 0);
		}
		else {
			memzero(ret, sizeof(xmlSchematronParserCtxt));
			ret->doc = doc;
			ret->dict = xmlDictCreate();
			/* The application has responsibility for the document */
			ret->preserve = 1;
			ret->xctxt = xmlXPathNewContext(doc);
			if(ret->xctxt == NULL) {
				xmlSchematronPErrMemory(NULL, "allocating schema parser XPath context", 0);
				xmlSchematronFreeParserCtxt(ret);
				ret = 0;
			}
		}
	}
	return ret;
}

/**
 * xmlSchematronFreeParserCtxt:
 * @ctxt:  the schema parser context
 *
 * Free the resources associated to the schema parser context
 */
void xmlSchematronFreeParserCtxt(xmlSchematronParserCtxtPtr ctxt)
{
	if(ctxt) {
		if(ctxt->doc && !ctxt->preserve)
			xmlFreeDoc(ctxt->doc);
		xmlXPathFreeContext(ctxt->xctxt);
		SAlloc::F(ctxt->namespaces);
		xmlDictFree(ctxt->dict);
		SAlloc::F(ctxt);
	}
}

#if 0
/**
 * xmlSchematronPushInclude:
 * @ctxt:  the schema parser context
 * @doc:  the included document
 * @cur:  the current include node
 *
 * Add an included document
 */
static void xmlSchematronPushInclude(xmlSchematronParserCtxtPtr ctxt, xmlDoc * doc, xmlNode * cur)
{
	if(ctxt->includes == NULL) {
		ctxt->maxIncludes = 10;
		ctxt->includes = (xmlNodePtr*)SAlloc::M(ctxt->maxIncludes * 2 * sizeof(xmlNode *));
		if(ctxt->includes == NULL) {
			xmlSchematronPErrMemory(NULL, "allocating parser includes", 0);
			return;
		}
		ctxt->nbIncludes = 0;
	}
	else if(ctxt->nbIncludes + 2 >= ctxt->maxIncludes) {
		xmlNode ** tmp = (xmlNodePtr*)SAlloc::R(ctxt->includes, ctxt->maxIncludes * 4 * sizeof(xmlNode *));
		if(!tmp) {
			xmlSchematronPErrMemory(NULL, "allocating parser includes", 0);
			return;
		}
		ctxt->includes = tmp;
		ctxt->maxIncludes *= 2;
	}
	ctxt->includes[2 * ctxt->nbIncludes] = cur;
	ctxt->includes[2 * ctxt->nbIncludes + 1] = (xmlNode *)doc;
	ctxt->nbIncludes++;
}

/**
 * xmlSchematronPopInclude:
 * @ctxt:  the schema parser context
 *
 * Pop an include level. The included document is being freed
 *
 * Returns the node immediately following the include or NULL if the
 *       include list was empty.
 */
static xmlNode * xmlSchematronPopInclude(xmlSchematronParserCtxtPtr ctxt)
{
	xmlDoc * doc;
	xmlNode * ret;
	if(ctxt->nbIncludes <= 0)
		return 0;
	ctxt->nbIncludes--;
	doc = (xmlDoc *)ctxt->includes[2 * ctxt->nbIncludes + 1];
	ret = ctxt->includes[2 * ctxt->nbIncludes];
	xmlFreeDoc(doc);
	if(ret)
		ret = ret->next;
	if(!ret)
		return (xmlSchematronPopInclude(ctxt));
	return ret;
}

#endif

/**
 * xmlSchematronAddNamespace:
 * @ctxt:  the schema parser context
 * @prefix:  the namespace prefix
 * @ns:  the namespace name
 *
 * Add a namespace definition in the context
 */
static void xmlSchematronAddNamespace(xmlSchematronParserCtxtPtr ctxt, const xmlChar * prefix, const xmlChar * ns)
{
	if(ctxt->namespaces == NULL) {
		ctxt->maxNamespaces = 10;
		ctxt->namespaces = (const xmlChar**)SAlloc::M(ctxt->maxNamespaces * 2 * sizeof(const xmlChar *));
		if(ctxt->namespaces == NULL) {
			xmlSchematronPErrMemory(NULL, "allocating parser namespaces", 0);
			return;
		}
		ctxt->nbNamespaces = 0;
	}
	else if(ctxt->nbNamespaces + 2 >= ctxt->maxNamespaces) {
		const xmlChar ** tmp = (const xmlChar**)SAlloc::R((xmlChar**)ctxt->namespaces, ctxt->maxNamespaces * 4 * sizeof(const xmlChar *));
		if(!tmp) {
			xmlSchematronPErrMemory(NULL, "allocating parser namespaces", 0);
			return;
		}
		ctxt->namespaces = tmp;
		ctxt->maxNamespaces *= 2;
	}
	ctxt->namespaces[2 * ctxt->nbNamespaces] = xmlDictLookupSL(ctxt->dict, ns);
	ctxt->namespaces[2 * ctxt->nbNamespaces + 1] = xmlDictLookupSL(ctxt->dict, prefix);
	ctxt->nbNamespaces++;
	ctxt->namespaces[2 * ctxt->nbNamespaces] = NULL;
	ctxt->namespaces[2 * ctxt->nbNamespaces + 1] = NULL;
}

/**
 * xmlSchematronParseRule:
 * @ctxt:  a schema validation context
 * @rule:  the rule node
 *
 * parse a rule element
 */
static void xmlSchematronParseRule(xmlSchematronParserCtxtPtr ctxt, xmlSchematronPatternPtr pattern, xmlNode * rule)
{
	xmlNode * cur;
	int nbChecks = 0;
	xmlChar * test;
	xmlChar * context;
	xmlChar * report;
	xmlSchematronRulePtr ruleptr;
	xmlSchematronTestPtr testptr;
	if(!ctxt || (rule == NULL))
		return;
	context = xmlGetNoNsProp(rule, reinterpret_cast<const xmlChar *>("context"));
	if(!context) {
		xmlSchematronPErr(ctxt, rule, XML_SCHEMAP_NOROOT, "rule has no context attribute", 0, 0);
		return;
	}
	else if(context[0] == 0) {
		xmlSchematronPErr(ctxt, rule, XML_SCHEMAP_NOROOT, "rule has an empty context attribute", 0, 0);
		SAlloc::F(context);
		return;
	}
	else {
		ruleptr = xmlSchematronAddRule(ctxt, ctxt->schema, pattern, rule, context, 0);
		if(ruleptr == NULL) {
			SAlloc::F(context);
			return;
		}
	}
	cur = rule->children;
	NEXT_SCHEMATRON(cur);
	while(cur) {
		if(IS_SCHEMATRON(cur, "assert")) {
			nbChecks++;
			test = xmlGetNoNsProp(cur, reinterpret_cast<const xmlChar *>("test"));
			if(test == NULL) {
				xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "assert has no test attribute", 0, 0);
			}
			else if(test[0] == 0) {
				xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "assert has an empty test attribute", 0, 0);
				SAlloc::F(test);
			}
			else {
				/* @todo will need dynamic processing instead */
				report = xmlNodeGetContent(cur);
				testptr = xmlSchematronAddTest(ctxt, XML_SCHEMATRON_ASSERT, ruleptr, cur, test, report);
				if(testptr == NULL)
					SAlloc::F(test);
			}
		}
		else if(IS_SCHEMATRON(cur, "report")) {
			nbChecks++;
			test = xmlGetNoNsProp(cur, reinterpret_cast<const xmlChar *>("test"));
			if(test == NULL) {
				xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "assert has no test attribute", 0, 0);
			}
			else if(test[0] == 0) {
				xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "assert has an empty test attribute", 0, 0);
				SAlloc::F(test);
			}
			else {
				/* @todo will need dynamic processing instead */
				report = xmlNodeGetContent(cur);
				testptr = xmlSchematronAddTest(ctxt, XML_SCHEMATRON_REPORT, ruleptr, cur, test, report);
				if(testptr == NULL)
					SAlloc::F(test);
			}
		}
		else {
			xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "Expecting an assert or a report element instead of %s", cur->name, 0);
		}
		cur = cur->next;
		NEXT_SCHEMATRON(cur);
	}
	if(nbChecks == 0) {
		xmlSchematronPErr(ctxt, rule, XML_SCHEMAP_NOROOT, "rule has no assert nor report element", 0, 0);
	}
}

/**
 * xmlSchematronParsePattern:
 * @ctxt:  a schema validation context
 * @pat:  the pattern node
 *
 * parse a pattern element
 */
static void xmlSchematronParsePattern(xmlSchematronParserCtxtPtr ctxt, xmlNode * pat)
{
	xmlNode * cur;
	xmlSchematronPatternPtr pattern;
	int nbRules = 0;
	xmlChar * id;
	if(!ctxt || (pat == NULL))
		return;
	id = xmlGetNoNsProp(pat, reinterpret_cast<const xmlChar *>("id"));
	if(id == NULL) {
		id = xmlGetNoNsProp(pat, reinterpret_cast<const xmlChar *>("name"));
	}
	pattern = xmlSchematronAddPattern(ctxt, ctxt->schema, pat, id);
	if(pattern == NULL) {
		SAlloc::F(id);
		return;
	}
	cur = pat->children;
	NEXT_SCHEMATRON(cur);
	while(cur) {
		if(IS_SCHEMATRON(cur, "rule")) {
			xmlSchematronParseRule(ctxt, pattern, cur);
			nbRules++;
		}
		else {
			xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "Expecting a rule element instead of %s", cur->name, 0);
		}
		cur = cur->next;
		NEXT_SCHEMATRON(cur);
	}
	if(nbRules == 0) {
		xmlSchematronPErr(ctxt, pat, XML_SCHEMAP_NOROOT, "Pattern has no rule element", 0, 0);
	}
}

#if 0
/**
 * xmlSchematronLoadInclude:
 * @ctxt:  a schema validation context
 * @cur:  the include element
 *
 * Load the include document, Push the current pointer
 *
 * Returns the updated node pointer
 */
static xmlNode * xmlSchematronLoadInclude(xmlSchematronParserCtxtPtr ctxt, xmlNode * cur)
{
	xmlNode * ret = NULL;
	xmlDoc * doc = NULL;
	xmlChar * href = NULL;
	xmlChar * base = NULL;
	xmlChar * URI = NULL;
	if(!ctxt || (cur == NULL))
		return 0;
	href = xmlGetNoNsProp(cur, reinterpret_cast<const xmlChar *>("href"));
	if(href == NULL) {
		xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "Include has no href attribute", 0, 0);
		return (cur->next);
	}

	/* do the URI base composition, load and find the root */
	base = xmlNodeGetBase(cur->doc, cur);
	URI = xmlBuildURI(href, base);
	doc = xmlReadFile((const char *)URI, NULL, SCHEMATRON_PARSE_OPTIONS);
	if(!doc) {
		xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_FAILED_LOAD, "could not load include '%s'.\n", URI, 0);
		goto done;
	}
	ret = xmlDocGetRootElement(doc);
	if(!ret) {
		xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_FAILED_LOAD, "could not find root from include '%s'.\n", URI, 0);
		goto done;
	}

	/* Success, push the include for rollback on exit */
	xmlSchematronPushInclude(ctxt, doc, cur);

done:
	if(!ret) {
		xmlFreeDoc(doc);
	}
	SAlloc::F(href);
	SAlloc::F(base);
	SAlloc::F(URI);
	return ret;
}

#endif

/**
 * xmlSchematronParse:
 * @ctxt:  a schema validation context
 *
 * parse a schema definition resource and build an internal
 * XML Shema struture which can be used to validate instances.
 *
 * Returns the internal XML Schematron structure built from the resource or
 *       NULL in case of error
 */
xmlSchematronPtr xmlSchematronParse(xmlSchematronParserCtxtPtr ctxt)
{
	xmlSchematronPtr ret = NULL;
	xmlDoc * doc;
	xmlNode * root;
	xmlNode * cur;
	int preserve = 0;
	if(!ctxt)
		return 0;
	ctxt->nberrors = 0;
	/*
	 * First step is to parse the input document into an DOM/Infoset
	 */
	if(ctxt->URL != NULL) {
		doc = xmlReadFile((const char *)ctxt->URL, NULL, SCHEMATRON_PARSE_OPTIONS);
		if(!doc) {
			xmlSchematronPErr(ctxt, NULL, XML_SCHEMAP_FAILED_LOAD, "xmlSchematronParse: could not load '%s'.\n", ctxt->URL, 0);
			return 0;
		}
		ctxt->preserve = 0;
	}
	else if(ctxt->buffer != NULL) {
		doc = xmlReadMemory(ctxt->buffer, ctxt->size, NULL, NULL, SCHEMATRON_PARSE_OPTIONS);
		if(!doc) {
			xmlSchematronPErr(ctxt, NULL, XML_SCHEMAP_FAILED_PARSE, "xmlSchematronParse: could not parse.\n", NULL, 0);
			return 0;
		}
		doc->URL = sstrdup(reinterpret_cast<const xmlChar *>("in_memory_buffer"));
		ctxt->URL = xmlDictLookupSL(ctxt->dict, reinterpret_cast<const xmlChar *>("in_memory_buffer"));
		ctxt->preserve = 0;
	}
	else if(ctxt->doc) {
		doc = ctxt->doc;
		preserve = 1;
		ctxt->preserve = 1;
	}
	else {
		xmlSchematronPErr(ctxt, NULL, XML_SCHEMAP_NOTHING_TO_PARSE, "xmlSchematronParse: could not parse.\n", NULL, 0);
		return 0;
	}
	/*
	 * Then extract the root and Schematron parse it
	 */
	root = xmlDocGetRootElement(doc);
	if(root == NULL) {
		xmlSchematronPErr(ctxt, (xmlNode *)doc, XML_SCHEMAP_NOROOT, "The schema has no document element.\n", 0, 0);
		if(!preserve) {
			xmlFreeDoc(doc);
		}
		return 0;
	}
	if(!IS_SCHEMATRON(root, "schema")) {
		xmlSchematronPErr(ctxt, root, XML_SCHEMAP_NOROOT, "The XML document '%s' is not a XML schematron document", ctxt->URL, 0);
		goto exit;
	}
	ret = xmlSchematronNewSchematron(ctxt);
	if(!ret)
		goto exit;
	ctxt->schema = ret;
	/*
	 * scan the schema elements
	 */
	cur = root->children;
	NEXT_SCHEMATRON(cur);
	if(IS_SCHEMATRON(cur, "title")) {
		xmlChar * title = xmlNodeGetContent(cur);
		if(title) {
			ret->title = xmlDictLookupSL(ret->dict, title);
			SAlloc::F(title);
		}
		cur = cur->next;
		NEXT_SCHEMATRON(cur);
	}
	while(IS_SCHEMATRON(cur, "ns")) {
		xmlChar * prefix = xmlGetNoNsProp(cur, reinterpret_cast<const xmlChar *>("prefix"));
		xmlChar * uri = xmlGetNoNsProp(cur, reinterpret_cast<const xmlChar *>("uri"));
		if(isempty(uri)) {
			xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "ns element has no uri", 0, 0);
		}
		if(isempty(prefix)) {
			xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "ns element has no prefix", 0, 0);
		}
		if(prefix && uri) {
			xmlXPathRegisterNs(ctxt->xctxt, prefix, uri);
			xmlSchematronAddNamespace(ctxt, prefix, uri);
			ret->nbNs++;
		}
		SAlloc::F(uri);
		SAlloc::F(prefix);
		cur = cur->next;
		NEXT_SCHEMATRON(cur);
	}
	while(cur) {
		if(IS_SCHEMATRON(cur, "pattern")) {
			xmlSchematronParsePattern(ctxt, cur);
			ret->nbPattern++;
		}
		else {
			xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "Expecting a pattern element instead of %s", cur->name, 0);
		}
		cur = cur->next;
		NEXT_SCHEMATRON(cur);
	}
	if(ret->nbPattern == 0) {
		xmlSchematronPErr(ctxt, root, XML_SCHEMAP_NOROOT, "The schematron document '%s' has no pattern", ctxt->URL, 0);
		goto exit;
	}
	/* the original document must be kept for reporting */
	ret->doc = doc;
	if(preserve) {
		ret->preserve = 1;
	}
	preserve = 1;
exit:
	if(!preserve) {
		xmlFreeDoc(doc);
	}
	if(ret) {
		if(ctxt->nberrors != 0) {
			xmlSchematronFree(ret);
			ret = NULL;
		}
		else {
			ret->namespaces = ctxt->namespaces;
			ret->nbNamespaces = ctxt->nbNamespaces;
			ctxt->namespaces = NULL;
		}
	}
	return ret;
}

/************************************************************************
*									*
*		Schematrontron Reports handler				*
*									*
************************************************************************/

static xmlNode * xmlSchematronGetNode(xmlSchematronValidCtxtPtr ctxt, xmlNode * cur, const xmlChar * xpath)
{
	xmlNode * P_Node = NULL;
	if(ctxt && cur && xpath) {
		ctxt->xctxt->doc = cur->doc;
		ctxt->xctxt->P_Node = cur;
		xmlXPathObject * ret = xmlXPathEval(xpath, ctxt->xctxt);
		if(ret) {
			if((ret->type == XPATH_NODESET) && ret->nodesetval && (ret->nodesetval->nodeNr > 0))
				P_Node = ret->nodesetval->PP_NodeTab[0];
			xmlXPathFreeObject(ret);
		}
	}
	return P_Node;
}

/**
 * xmlSchematronReportOutput:
 * @ctxt: the validation context
 * @cur: the current node tested
 * @msg: the message output
 *
 * Output part of the report to whatever channel the user selected
 */
static void xmlSchematronReportOutput(xmlSchematronValidCtxtPtr ctxt ATTRIBUTE_UNUSED, xmlNode * cur ATTRIBUTE_UNUSED, const char * msg) 
{
	/* @todo */
	fprintf(stderr, "%s", msg);
}

/**
 * xmlSchematronFormatReport:
 * @ctxt:  the validation context
 * @test: the test node
 * @cur: the current node tested
 *
 * Build the string being reported to the user.
 *
 * Returns a report string or NULL in case of error. The string needs
 *       to be deallocated by teh caller
 */
static xmlChar * xmlSchematronFormatReport(xmlSchematronValidCtxtPtr ctxt, xmlNode * test, xmlNode * cur)
{
	xmlChar * ret = NULL;
	xmlNode * child;
	xmlNode * P_Node;
	if((test == NULL) || (cur == NULL))
		return ret;
	child = test->children;
	while(child != NULL) {
		if((child->type == XML_TEXT_NODE) || (child->type == XML_CDATA_SECTION_NODE))
			ret = xmlStrcat(ret, child->content);
		else if(IS_SCHEMATRON(child, "name")) {
			xmlChar * path = xmlGetNoNsProp(child, reinterpret_cast<const xmlChar *>("path"));
			P_Node = cur;
			if(path) {
				P_Node = xmlSchematronGetNode(ctxt, cur, path);
				SETIFZ(P_Node, cur);
				SAlloc::F(path);
			}
			if((P_Node->ns == NULL) || (P_Node->ns->prefix == NULL))
				ret = xmlStrcat(ret, P_Node->name);
			else {
				ret = xmlStrcat(ret, P_Node->ns->prefix);
				ret = xmlStrcat(ret, reinterpret_cast<const xmlChar *>(":"));
				ret = xmlStrcat(ret, P_Node->name);
			}
		}
		else {
			child = child->next;
			continue;
		}

		/*
		 * remove superfluous \n
		 */
		if(ret) {
			int len = sstrlen(ret);
			xmlChar c;
			if(len > 0) {
				c = ret[len-1];
				if((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t')) {
					while((c == ' ') || (c == '\n') ||
					    (c == '\r') || (c == '\t')) {
						len--;
						if(len == 0)
							break;
						c = ret[len-1];
					}
					ret[len] = ' ';
					ret[len + 1] = 0;
				}
			}
		}

		child = child->next;
	}
	return ret;
}

/**
 * xmlSchematronReportSuccess:
 * @ctxt:  the validation context
 * @test: the compiled test
 * @cur: the current node tested
 * @success: boolean value for the result
 *
 * called from the validation engine when an assert or report test have
 * been done.
 */
static void xmlSchematronReportSuccess(xmlSchematronValidCtxtPtr ctxt, xmlSchematronTestPtr test, xmlNode * cur, xmlSchematronPatternPtr pattern, int success)
{
	if(!ctxt || (cur == NULL) || (test == NULL))
		return;
	/* if quiet and not SVRL report only failures */
	if((ctxt->flags & XML_SCHEMATRON_OUT_QUIET) && ((ctxt->flags & XML_SCHEMATRON_OUT_XML) == 0) && (test->type == XML_SCHEMATRON_REPORT))
		return;
	if(ctxt->flags & XML_SCHEMATRON_OUT_XML) {
		TODO
	}
	else {
		xmlChar * path;
		char msg[1000];
		long line;
		const xmlChar * report = NULL;
		if(((test->type == XML_SCHEMATRON_REPORT) & (!success)) || ((test->type == XML_SCHEMATRON_ASSERT) & (success)))
			return;
		line = xmlGetLineNo(cur);
		path = xmlGetNodePath(cur);
		SETIFZ(path, (xmlChar *)cur->name);
#if 0
		if((test->report != NULL) && (test->report[0] != 0))
			report = test->report;
#endif
		if(test->P_Node)
			report = xmlSchematronFormatReport(ctxt, test->P_Node, cur);
		if(report == NULL) {
			if(test->type == XML_SCHEMATRON_ASSERT) {
				report = sstrdup((const xmlChar *)"node failed assert");
			}
			else {
				report = sstrdup((const xmlChar *)"node failed report");
			}
		}
		snprintf(msg, 999, "%s line %ld: %s\n", (const char *)path, line, (const char *)report);
		if(ctxt->flags & XML_SCHEMATRON_OUT_ERROR) {
			xmlStructuredErrorFunc schannel = NULL;
			xmlGenericErrorFunc channel = NULL;
			void * data = NULL;
			if(ctxt) {
				if(ctxt->serror != NULL)
					schannel = ctxt->serror;
				else
					channel = ctxt->error;
				data = ctxt->userData;
			}
			__xmlRaiseError(schannel, channel, data, NULL, cur, XML_FROM_SCHEMATRONV,
			    (test->type == XML_SCHEMATRON_ASSERT) ? XML_SCHEMATRONV_ASSERT : XML_SCHEMATRONV_REPORT,
			    XML_ERR_ERROR, NULL, line, (pattern == NULL) ? NULL : ((const char *)pattern->name),
			    (const char *)path, (const char *)report, 0, 0, "%s", msg);
		}
		else {
			xmlSchematronReportOutput(ctxt, cur, &msg[0]);
		}
		SAlloc::F((char *)report);
		if((path != NULL) && (path != (xmlChar *)cur->name))
			SAlloc::F(path);
	}
}
/**
 * xmlSchematronReportPattern:
 * @ctxt:  the validation context
 * @pattern: the current pattern
 *
 * called from the validation engine when starting to check a pattern
 */
static void xmlSchematronReportPattern(xmlSchematronValidCtxtPtr ctxt, xmlSchematronPatternPtr pattern) 
{
	if(!ctxt || (pattern == NULL))
		return;
	if((ctxt->flags & XML_SCHEMATRON_OUT_QUIET) || (ctxt->flags & XML_SCHEMATRON_OUT_ERROR)) // Error gives pattern name as part of error
		return;
	if(ctxt->flags & XML_SCHEMATRON_OUT_XML) {
		TODO
	}
	else if(pattern->name) {
		char msg[1000];
		snprintf(msg, 999, "Pattern: %s\n", (const char *)pattern->name);
		xmlSchematronReportOutput(ctxt, NULL, &msg[0]);
	}
}

/************************************************************************
*									*
*		Validation against a Schematrontron				*
*									*
************************************************************************/

/**
 * xmlSchematronSetValidStructuredErrors:
 * @ctxt:  a Schematron validation context
 * @serror:  the structured error function
 * @ctx: the functions context
 *
 * Set the structured error callback
 */
void xmlSchematronSetValidStructuredErrors(xmlSchematronValidCtxtPtr ctxt, xmlStructuredErrorFunc serror, void * ctx)
{
	if(ctxt) {
		ctxt->serror = serror;
		ctxt->error = NULL;
		ctxt->warning = NULL;
		ctxt->userData = ctx;
	}
}
/**
 * xmlSchematronNewValidCtxt:
 * @schema:  a precompiled XML Schematrons
 * @options: a set of xmlSchematronValidOptions
 *
 * Create an XML Schematrons validation context based on the given schema.
 *
 * Returns the validation context or NULL in case of error
 */
xmlSchematronValidCtxtPtr xmlSchematronNewValidCtxt(xmlSchematronPtr schema, int options)
{
	int i;
	xmlSchematronValidCtxt * ret = (xmlSchematronValidCtxt *)SAlloc::M(sizeof(xmlSchematronValidCtxt));
	if(!ret) {
		xmlSchematronVErrMemory(NULL, "allocating validation context", NULL);
		return 0;
	}
	memzero(ret, sizeof(xmlSchematronValidCtxt));
	ret->type = XML_STRON_CTXT_VALIDATOR;
	ret->schema = schema;
	ret->xctxt = xmlXPathNewContext(NULL);
	ret->flags = options;
	if(ret->xctxt == NULL) {
		xmlSchematronPErrMemory(NULL, "allocating schema parser XPath context", NULL);
		xmlSchematronFreeValidCtxt(ret);
		return 0;
	}
	for(i = 0; i < schema->nbNamespaces; i++) {
		if((schema->namespaces[2 * i] == NULL) || (schema->namespaces[2 * i + 1] == NULL))
			break;
		xmlXPathRegisterNs(ret->xctxt, schema->namespaces[2 * i + 1], schema->namespaces[2 * i]);
	}
	return ret;
}
/**
 * xmlSchematronFreeValidCtxt:
 * @ctxt:  the schema validation context
 *
 * Free the resources associated to the schema validation context
 */
void xmlSchematronFreeValidCtxt(xmlSchematronValidCtxtPtr ctxt)
{
	if(ctxt) {
		xmlXPathFreeContext(ctxt->xctxt);
		xmlDictFree(ctxt->dict);
		SAlloc::F(ctxt);
	}
}

static xmlNode * xmlSchematronNextNode(xmlNode * cur)
{
	if(cur->children) {
		/*
		 * Do not descend on entities declarations
		 */
		if(cur->children->type != XML_ENTITY_DECL) {
			cur = cur->children;
			/*
			 * Skip DTDs
			 */
			if(cur->type != XML_DTD_NODE)
				return cur;
		}
	}
	while(cur->next) {
		cur = cur->next;
		if((cur->type != XML_ENTITY_DECL) && (cur->type != XML_DTD_NODE))
			return cur;
	}
	do {
		cur = cur->parent;
		if(!cur) 
			break;
		if(cur->type == XML_DOCUMENT_NODE) 
			return 0;
		if(cur->next) {
			cur = cur->next;
			return cur;
		}
	} while(cur);
	return cur;
}

/**
 * xmlSchematronRunTest:
 * @ctxt:  the schema validation context
 * @test:  the current test
 * @instance:  the document instace tree
 * @cur:  the current node in the instance
 *
 * Validate a rule against a tree instance at a given position
 *
 * Returns 1 in case of success, 0 if error and -1 in case of internal error
 */
static int xmlSchematronRunTest(xmlSchematronValidCtxtPtr ctxt,
    xmlSchematronTestPtr test, xmlDoc * instance, xmlNode * cur, xmlSchematronPatternPtr pattern)
{
	xmlXPathObjectPtr ret;
	int failed = 0;
	ctxt->xctxt->doc = instance;
	ctxt->xctxt->P_Node = cur;
	ret = xmlXPathCompiledEval(test->comp, ctxt->xctxt);
	if(!ret) {
		failed = 1;
	}
	else {
		switch(ret->type) {
			case XPATH_XSLT_TREE:
			case XPATH_NODESET:
			    if((ret->nodesetval == NULL) || (ret->nodesetval->nodeNr == 0))
				    failed = 1;
			    break;
			case XPATH_BOOLEAN:
			    failed = !ret->boolval;
			    break;
			case XPATH_NUMBER:
			    if((xmlXPathIsNaN(ret->floatval)) || (ret->floatval == 0.0))
				    failed = 1;
			    break;
			case XPATH_STRING:
			    if(isempty(ret->stringval))
				    failed = 1;
			    break;
			case XPATH_UNDEFINED:
			case XPATH_POINT:
			case XPATH_RANGE:
			case XPATH_LOCATIONSET:
			case XPATH_USERS:
			    failed = 1;
			    break;
		}
		xmlXPathFreeObject(ret);
	}
	if((failed) && (test->type == XML_SCHEMATRON_ASSERT))
		ctxt->nberrors++;
	else if((!failed) && (test->type == XML_SCHEMATRON_REPORT))
		ctxt->nberrors++;
	xmlSchematronReportSuccess(ctxt, test, cur, pattern, !failed);
	return !failed;
}
/**
 * xmlSchematronValidateDoc:
 * @ctxt:  the schema validation context
 * @instance:  the document instace tree
 *
 * Validate a tree instance against the schematron
 *
 * Returns 0 in case of success, -1 in case of internal error
 *       and an error count otherwise.
 */
int xmlSchematronValidateDoc(xmlSchematronValidCtxtPtr ctxt, xmlDoc * instance)
{
	xmlNode * cur;
	xmlNode * root;
	xmlSchematronPatternPtr pattern;
	xmlSchematronRulePtr rule;
	xmlSchematronTestPtr test;
	if(!ctxt || (ctxt->schema == NULL) || (ctxt->schema->rules == NULL) || (instance == NULL))
		return -1;
	ctxt->nberrors = 0;
	root = xmlDocGetRootElement(instance);
	if(root == NULL) {
		TODO
		ctxt->nberrors++;
		return 1;
	}
	if((ctxt->flags & XML_SCHEMATRON_OUT_QUIET) || (ctxt->flags == 0)) {
		/*
		 * we are just trying to assert the validity of the document,
		 * speed primes over the output, run in a single pass
		 */
		cur = root;
		while(cur) {
			rule = ctxt->schema->rules;
			while(rule != NULL) {
				if(xmlPatternMatch(rule->pattern, cur) == 1) {
					test = rule->tests;
					while(test != NULL) {
						xmlSchematronRunTest(ctxt, test, instance, cur, (xmlSchematronPatternPtr)rule->pattern);
						test = test->next;
					}
				}
				rule = rule->next;
			}
			cur = xmlSchematronNextNode(cur);
		}
	}
	else {
		/*
		 * Process all contexts one at a time
		 */
		pattern = ctxt->schema->patterns;
		while(pattern != NULL) {
			xmlSchematronReportPattern(ctxt, pattern);
			/*
			 * @todo convert the pattern rule to a direct XPath and
			 * compute directly instead of using the pattern matching over the full document...
			 * Check the exact semantic
			 */
			for(cur = root; cur; cur = xmlSchematronNextNode(cur)) {
				for(rule = pattern->rules; rule; rule = rule->patnext) {
					if(xmlPatternMatch(rule->pattern, cur) == 1) {
						for(test = rule->tests; test; test = test->next)
							xmlSchematronRunTest(ctxt, test, instance, cur, pattern);
					}
				}
			}
			pattern = pattern->next;
		}
	}
	return (ctxt->nberrors);
}

#ifdef STANDALONE
int main()
{
	int ret;
	xmlDoc * instance;
	xmlSchematronValidCtxtPtr vctxt;
	xmlSchematronPtr schema = NULL;
	xmlSchematronParserCtxtPtr pctxt = xmlSchematronNewParserCtxt("tst.sct");
	if(pctxt == NULL) {
		fprintf(stderr, "failed to build schematron parser\n");
	}
	else {
		schema = xmlSchematronParse(pctxt);
		if(schema == NULL) {
			fprintf(stderr, "failed to compile schematron\n");
		}
		xmlSchematronFreeParserCtxt(pctxt);
	}
	instance = xmlReadFile("tst.sct", NULL, XML_PARSE_NOENT | XML_PARSE_NOCDATA);
	if(instance == NULL) {
		fprintf(stderr, "failed to parse instance\n");
	}
	if((schema != NULL) && (instance != NULL)) {
		vctxt = xmlSchematronNewValidCtxt(schema);
		if(vctxt == NULL) {
			fprintf(stderr, "failed to build schematron validator\n");
		}
		else {
			ret = xmlSchematronValidateDoc(vctxt, instance);
			xmlSchematronFreeValidCtxt(vctxt);
		}
	}
	xmlSchematronFree(schema);
	xmlFreeDoc(instance);
	xmlCleanupParser();
	xmlMemoryDump();
	return 0;
}

#endif
#define bottom_schematron
//#include "elfgcchack.h"
#endif /* LIBXML_SCHEMATRON_ENABLED */
