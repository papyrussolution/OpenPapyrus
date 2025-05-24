/*
 * xpath.c: XML Path Language implementation
 *     XPath is a language for addressing parts of an XML document,
 *     designed to be used by both XSLT and XPointer
 **f
 * Reference: W3C Recommendation 16 November 1999 http://www.w3.org/TR/1999/REC-xpath-19991116
 * Public reference: http://www.w3.org/TR/xpath
 *
 * See Copyright for the status of this software
 * Author: daniel@veillard.com
 */
#include <slib-internal.h>
#pragma hdrstop
#ifdef LIBXML_PATTERN_ENABLED
	#define XPATH_STREAMING
#endif
#define TODO xmlGenericError(0, "Unimplemented block at %s:%d\n", __FILE__, __LINE__);
/**
 * WITH_TIM_SORT:
 *
 * Use the Timsort algorithm provided in timsort.h to sort
 * nodeset as this is a great improvement over the old Shell sort
 * used in xmlXPathNodeSetSort()
 */
#define WITH_TIM_SORT
/*
 * XP_OPTIMIZED_NON_ELEM_COMPARISON:
 * If defined, this will use xmlXPathCmpNodesExt() instead of
 * xmlXPathCmpNodes(). The new function is optimized comparison of
 * non-element nodes; actually it will speed up comparison only if
 * xmlXPathOrderDocElems() was called in order to index the elements of
 * a tree in document order; Libxslt does such an indexing, thus it will
 * benefit from this optimization.
 */
#define XP_OPTIMIZED_NON_ELEM_COMPARISON

/*
 * XP_OPTIMIZED_FILTER_FIRST:
 * If defined, this will optimize expressions like "key('foo', 'val')[b][1]"
 * in a way, that it stop evaluation at the first node.
 */
#define XP_OPTIMIZED_FILTER_FIRST

/*
 * XP_DEBUG_OBJ_USAGE:
 * Internal flag to enable tracking of how much XPath objects have been
 * created.
 */
/* #define XP_DEBUG_OBJ_USAGE */

/*
 * XPATH_MAX_STEPS:
 * when compiling an XPath expression we arbitrary limit the maximum
 * number of step operation in the compiled expression. 1000000 is
 * an insanely large value which should never be reached under normal
 * circumstances
 */
#define XPATH_MAX_STEPS 1000000

/*
 * XPATH_MAX_STACK_DEPTH:
 * when evaluating an XPath expression we arbitrary limit the maximum
 * number of object allowed to be pushed on the stack. 1000000 is
 * an insanely large value which should never be reached under normal
 * circumstances
 */
#define XPATH_MAX_STACK_DEPTH 1000000

/*
 * XPATH_MAX_NODESET_LENGTH:
 * when evaluating an XPath expression nodesets are created and we
 * arbitrary limit the maximum length of those node set. 10000000 is
 * an insanely large value which should never be reached under normal
 * circumstances, one would first need to construct an in memory tree
 * with more than 10 millions nodes.
 */
#define XPATH_MAX_NODESET_LENGTH 10000000

/*
 * @todo 
 * There are a few spots where some tests are done which depend upon ascii
 * data.  These should be enhanced for full UTF8 support (see particularly
 * any use of the macros IS_ASCII_CHARACTER and IS_ASCII_DIGIT)
 */

#ifdef XP_OPTIMIZED_NON_ELEM_COMPARISON
/**
 * xmlXPathCmpNodesExt:
 * @node1:  the first node
 * @node2:  the second node
 *
 * Compare two nodes w.r.t document order.
 * This one is optimized for handling of non-element nodes.
 *
 * Returns -2 in case of error 1 if first point < second point, 0 if
 *    it's the same node, -1 otherwise
 */
static int xmlXPathCmpNodesExt(xmlNode * node1, xmlNode * node2) 
{
	int depth1, depth2;
	int misc = 0, precedence1 = 0, precedence2 = 0;
	xmlNode * miscNode1 = NULL;
	xmlNode * miscNode2 = NULL;
	xmlNode * cur;
	xmlNode * root;
	long l1, l2;
	if(!node1 || !node2)
		return -2;
	if(node1 == node2)
		return 0;
	/*
	 * a couple of optimizations which will avoid computations in most cases
	 */
	switch(node1->type) {
		case XML_ELEMENT_NODE:
		    if(node2->type == XML_ELEMENT_NODE) {
			    if((0 > (long)node1->content) && /* @todo Would a != 0 suffice here? */ (0 > (long)node2->content) && (node1->doc == node2->doc)) {
				    l1 = -((long)node1->content);
				    l2 = -((long)node2->content);
				    if(l1 < l2)
					    return 1;
				    if(l1 > l2)
					    return -1;
			    }
			    else
				    goto turtle_comparison;
		    }
		    break;
		case XML_ATTRIBUTE_NODE:
		    precedence1 = 1; /* element is owner */
		    miscNode1 = node1;
		    node1 = node1->P_ParentNode;
		    misc = 1;
		    break;
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_COMMENT_NODE:
		case XML_PI_NODE: {
		    miscNode1 = node1;
		    /*
		 * Find nearest element node.
		     */
		    if(node1->prev) {
			    do {
				    node1 = node1->prev;
				    if(node1->type == XML_ELEMENT_NODE) {
					    precedence1 = 3; /* element in prev-sibl axis */
					    break;
				    }
				    if(node1->prev == NULL) {
					    precedence1 = 2; /* element is parent */
					    // URGENT TODO: Are there any cases, where the parent of such a node is not an element node? 
					    node1 = node1->P_ParentNode;
					    break;
				    }
			    } while(1);
		    }
		    else {
			    precedence1 = 2; // element is parent 
			    node1 = node1->P_ParentNode;
		    }
		    if((node1 == NULL) || (node1->type != XML_ELEMENT_NODE) || (0 <= (long)node1->content)) {
				// Fallback for whatever case.
			    node1 = miscNode1;
			    precedence1 = 0;
		    }
		    else
			    misc = 1;
	    }
	    break;
		case XML_NAMESPACE_DECL:
			// @todo why do we return 1 for namespace nodes?
		    return 1;
		default:
		    break;
	}
	switch(node2->type) {
		case XML_ELEMENT_NODE:
		    break;
		case XML_ATTRIBUTE_NODE:
		    precedence2 = 1; /* element is owner */
		    miscNode2 = node2;
		    node2 = node2->P_ParentNode;
		    misc = 1;
		    break;
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_COMMENT_NODE:
		case XML_PI_NODE: {
		    miscNode2 = node2;
		    if(node2->prev) {
			    do {
				    node2 = node2->prev;
				    if(node2->type == XML_ELEMENT_NODE) {
					    precedence2 = 3; /* element in prev-sibl axis */
					    break;
				    }
				    if(node2->prev == NULL) {
					    precedence2 = 2; // element is parent 
					    node2 = node2->P_ParentNode;
					    break;
				    }
			    } while(1);
		    }
		    else {
			    precedence2 = 2; /* element is parent */
			    node2 = node2->P_ParentNode;
		    }
		    if(!node2 || (node2->type != XML_ELEMENT_NODE) || (0 <= (long)node2->content)) {
			    node2 = miscNode2;
			    precedence2 = 0;
		    }
		    else
			    misc = 1;
	    }
	    break;
		case XML_NAMESPACE_DECL:
		    return 1;
		default:
		    break;
	}
	if(misc) {
		if(node1 == node2) {
			if(precedence1 == precedence2) {
				// The ugly case; but normally there aren't many adjacent non-element nodes around.
				for(cur = miscNode2->prev; cur; cur = cur->prev) {
					if(cur == miscNode1)
						return 1;
					if(cur->type == XML_ELEMENT_NODE)
						return -1;
				}
				return -1;
			}
			else {
				/*
				 * Evaluate based on higher precedence wrt to the element.
				 * @todo This assumes attributes are sorted before content.
				 * Is this 100% correct?
				 */
				if(precedence1 < precedence2)
					return 1;
				else
					return -1;
			}
		}
		/*
		 * Special case: One of the helper-elements is contained by the other.
		 * <foo>
		 * <node2>
		 *   <node1>Text-1(precedence1 == 2)</node1>
		 * </node2>
		 * Text-6(precedence2 == 3)
		 * </foo>
		 */
		if((precedence2 == 3) && (precedence1 > 1)) {
			for(cur = node1->P_ParentNode; cur; cur = cur->P_ParentNode)
				if(cur == node2)
					return 1;
		}
		if((precedence1 == 3) && (precedence2 > 1)) {
			for(cur = node2->P_ParentNode; cur; cur = cur->P_ParentNode)
				if(cur == node1)
					return -1;
		}
	}
	/*
	 * Speedup using document order if availble.
	 */
	if((node1->type == XML_ELEMENT_NODE) && (node2->type == XML_ELEMENT_NODE) && (0 > (long)node1->content) && (0 > (long)node2->content) && (node1->doc == node2->doc)) {
		l1 = -((long)node1->content);
		l2 = -((long)node2->content);
		if(l1 < l2)
			return 1;
		if(l1 > l2)
			return -1;
	}
turtle_comparison:
	if(node1 == node2->prev)
		return 1;
	if(node1 == node2->next)
		return -1;
	/*
	 * compute depth to root
	 */
	for(depth2 = 0, cur = node2; cur->P_ParentNode; cur = cur->P_ParentNode) {
		if(cur == node1)
			return 1;
		depth2++;
	}
	root = cur;
	for(depth1 = 0, cur = node1; cur->P_ParentNode; cur = cur->P_ParentNode) {
		if(cur == node2)
			return -1;
		depth1++;
	}
	/*
	 * Distinct document (or distinct entities :-( ) case.
	 */
	if(root != cur) {
		return -2;
	}
	/*
	 * get the nearest common ancestor.
	 */
	while(depth1 > depth2) {
		depth1--;
		node1 = node1->P_ParentNode;
	}
	while(depth2 > depth1) {
		depth2--;
		node2 = node2->P_ParentNode;
	}
	while(node1->P_ParentNode != node2->P_ParentNode) {
		node1 = node1->P_ParentNode;
		node2 = node2->P_ParentNode;
		/* should not happen but just in case ... */
		if((node1 == NULL) || (node2 == NULL))
			return -2;
	}
	/*
	 * Find who's first.
	 */
	if(node1 == node2->prev)
		return 1;
	if(node1 == node2->next)
		return -1;
	/*
	 * Speedup using document order if availble.
	 */
	if((node1->type == XML_ELEMENT_NODE) && (node2->type == XML_ELEMENT_NODE) && (0 > (long)node1->content) && (0 > (long)node2->content) && (node1->doc == node2->doc)) {
		l1 = -((long)node1->content);
		l2 = -((long)node2->content);
		if(l1 < l2)
			return 1;
		if(l1 > l2)
			return -1;
	}
	for(cur = node1->next; cur; cur = cur->next)
		if(cur == node2)
			return 1;
	return -1; /* assume there is no sibling list corruption */
}

#endif /* XP_OPTIMIZED_NON_ELEM_COMPARISON */
/*
 * Wrapper for the Timsort argorithm from timsort.h
 */
#ifdef WITH_TIM_SORT
#define SORT_NAME libxml_domnode
// @v10.5.8 #define SORT_TYPE xmlNodePtr
typedef xmlNode * SORT_TYPE; // @v10.5.8
/**
 * wrap_cmp:
 * @x: a node
 * @y: another node
 *
 * Comparison function for the Timsort implementation
 *
 * Returns -2 in case of error -1 if first point < second point, 0 if
 *    it's the same node, +1 otherwise
 */
static int wrap_cmp(xmlNode * x, xmlNode * y);
#ifdef XP_OPTIMIZED_NON_ELEM_COMPARISON
	static int wrap_cmp(xmlNode * x, xmlNode * y)
	{
		int res = xmlXPathCmpNodesExt(x, y);
		return res == -2 ? res : -res;
	}
#else
	static int wrap_cmp(xmlNode * x, xmlNode * y)
	{
		int res = xmlXPathCmpNodes(x, y);
		return res == -2 ? res : -res;
	}
#endif
	#define SORT_CMP(x, y)  (wrap_cmp(x, y))
	#include "timsort.h"
#endif /* WITH_TIM_SORT */

#if defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
//
// Floating point stuff
// 
#ifndef TRIO_REPLACE_STDIO
	#define TRIO_PUBLIC static
#endif
//#include "trionan.c"
// trio_nzero()
// trio_signbit
//
//
/*
 * The lack of portability of this section of the libc is annoying !
 */
double xmlXPathNAN = 0;
double xmlXPathPINF = 1;
double xmlXPathNINF = -1;
static double xmlXPathNZERO = 0; /* not exported from headers */
static int xmlXPathInitialized = 0;
//
// Descr: Initialize the XPath environment
//
void xmlXPathInit()
{
	if(!xmlXPathInitialized) {
		//xmlXPathPINF = trio_pinf();
		//xmlXPathNINF = trio_ninf();
		//xmlXPathNAN = trio_nan();
		xmlXPathPINF = fgetposinf();
		xmlXPathNINF = fgetneginf();
		xmlXPathNAN = fgetnan();
		xmlXPathNZERO = -0.0;
		xmlXPathInitialized = 1;
	}
}
/**
 * xmlXPathIsNaN:
 * @val:  a double value
 *
 * Provides a portable isnan() function to detect whether a double
 * is a NotaNumber. Based on trio code
 * http://sourceforge.net/projects/ctrio/
 *
 * Returns 1 if the value is a NaN, 0 otherwise
 */
// @v10.9.11 int xmlXPathIsNaN_Removed(double val) { return fisnan(val); } // @v10.8.1 trio_isnan-->fisnan
/**
 * xmlXPathIsInf:
 * @val:  a double value
 *
 * Provides a portable isinf() function to detect whether a double
 * is a +Infinite or -Infinite. Based on trio code
 * http://sourceforge.net/projects/ctrio/
 *
 * Returns 1 vi the value is +Infinite, -1 if -Infinite, 0 otherwise
 */
int FASTCALL xmlXPathIsInf(double val) { return fisinf(val); }

#endif /* SCHEMAS or XPATH */
#ifdef LIBXML_XPATH_ENABLED
/**
 * xmlXPathGetSign:
 * @val:  a double value
 *
 * Provides a portable function to detect the sign of a double
 * Modified from trio code
 * http://sourceforge.net/projects/ctrio/
 *
 * Returns 1 if the value is Negative, 0 if positive
 */
static int FASTCALL xmlXPathGetSign(double val) { return BIN(val < 0.0); }
/*
 * @todo when compatibility allows remove all "fake node libxslt" strings
 *  the test should just be name[0] = ' '
 */
#ifdef DEBUG_XPATH_EXPRESSION
	#define DEBUG_STEP
	#define DEBUG_EXPR
	#define DEBUG_EVAL_COUNTS
#endif

static xmlNs xmlXPathXMLNamespaceStruct = { NULL, XML_NAMESPACE_DECL, XML_XML_NAMESPACE, reinterpret_cast<const xmlChar *>("xml"), NULL, NULL };
static xmlNs * xmlXPathXMLNamespace = &xmlXPathXMLNamespaceStruct;
#ifndef LIBXML_THREAD_ENABLED
/*
 * Optimizer is disabled only when threaded apps are detected while
 * the library ain't compiled for thread safety.
 */
static int xmlXPathDisableOptimizer = 0;
#endif
// 
// Error handling routines
// 
/**
 * XP_ERRORNULL:
 * @X:  the error code
 *
 * Macro to raise an XPath error and return NULL.
 */
#define XP_ERRORNULL(X) { xmlXPathErr(ctxt, X); return 0; }

/*
 * The array xmlXPathErrorMessages corresponds to the enum xmlXPathError
 */
static const char * xmlXPathErrorMessages[] = {
	"Ok\n",
	"Number encoding\n",
	"Unfinished literal\n",
	"Start of literal\n",
	"Expected $ for variable reference\n",
	"Undefined variable\n",
	"Invalid predicate\n",
	"Invalid expression\n",
	"Missing closing curly brace\n",
	"Unregistered function\n",
	"Invalid operand\n",
	"Invalid type\n",
	"Invalid number of arguments\n",
	"Invalid context size\n",
	"Invalid context position\n",
	"Memory allocation error\n",
	"Syntax error\n",
	"Resource error\n",
	"Sub resource error\n",
	"Undefined namespace prefix\n",
	"Encoding error\n",
	"Char out of XML range\n",
	"Invalid or incomplete context\n",
	"Stack usage error\n",
	"Forbidden variable\n",
	"?? Unknown error ??\n" /* Must be last in the list! */
};
#define MAXERRNO ((int)(sizeof(xmlXPathErrorMessages) / sizeof(xmlXPathErrorMessages[0])) - 1)
/**
 * xmlXPathErrMemory:
 * @ctxt:  an XPath context
 * @extra:  extra informations
 *
 * Handle a redefinition of attribute error
 */
static void FASTCALL xmlXPathErrMemory(xmlXPathContext * ctxt, const char * extra)
{
	SString msg_buf("Memory allocation failed");
	if(extra)
		msg_buf.CatDiv(':', 1).Cat(extra);
	msg_buf.CR();
	if(ctxt) {
		ctxt->lastError.message = (char *)sstrdup(msg_buf.ucptr());
		ctxt->lastError.domain = XML_FROM_XPATH;
		ctxt->lastError.code = XML_ERR_NO_MEMORY;
		if(ctxt->error)
			ctxt->error(ctxt->userData, &ctxt->lastError);
	}
	else
		__xmlRaiseError(0, 0, 0, 0, 0, XML_FROM_XPATH, XML_ERR_NO_MEMORY, XML_ERR_FATAL, 0, 0, 0, 0, 0, 0, 0, msg_buf);
}
/**
 * xmlXPathPErrMemory:
 * @ctxt:  an XPath parser context
 * @extra:  extra informations
 *
 * Handle a redefinition of attribute error
 */
static void xmlXPathPErrMemory(xmlXPathParserContext * ctxt, const char * extra)
{
	if(!ctxt)
		xmlXPathErrMemory(NULL, extra);
	else {
		ctxt->error = XPATH_MEMORY_ERROR;
		xmlXPathErrMemory(ctxt->context, extra);
	}
}
/**
 * xmlXPathErr:
 * @ctxt:  a XPath parser context
 * @error:  the error code
 *
 * Handle an XPath error
 */
void FASTCALL xmlXPathErr(xmlXPathParserContext * ctxt, int error)
{
	if(error < 0 || error > MAXERRNO)
		error = MAXERRNO;
	if(!ctxt) {
		__xmlRaiseError(0, 0, 0, 0, 0, XML_FROM_XPATH, error + XML_XPATH_EXPRESSION_OK - XPATH_EXPRESSION_OK, XML_ERR_ERROR, NULL, 0,
		    NULL, NULL, NULL, 0, 0, "%s", xmlXPathErrorMessages[error]);
		return;
	}
	ctxt->error = error;
	if(ctxt->context == NULL) {
		__xmlRaiseError(0, 0, 0, 0, 0, XML_FROM_XPATH, error + XML_XPATH_EXPRESSION_OK - XPATH_EXPRESSION_OK, XML_ERR_ERROR, NULL, 0,
		    (const char *)ctxt->base, NULL, NULL, ctxt->cur - ctxt->base, 0, "%s", xmlXPathErrorMessages[error]);
		return;
	}
	// cleanup current last error 
	xmlResetError(&ctxt->context->lastError);
	ctxt->context->lastError.domain = XML_FROM_XPATH;
	ctxt->context->lastError.code = error + XML_XPATH_EXPRESSION_OK - XPATH_EXPRESSION_OK;
	ctxt->context->lastError.level = XML_ERR_ERROR;
	ctxt->context->lastError.str1 = (char *)sstrdup(ctxt->base);
	ctxt->context->lastError.int1 = ctxt->cur - ctxt->base;
	ctxt->context->lastError.P_Node = ctxt->context->debugNode;
	if(ctxt->context->error) {
		ctxt->context->error(ctxt->context->userData, &ctxt->context->lastError);
	}
	else {
		__xmlRaiseError(0, 0, 0, 0, ctxt->context->debugNode, XML_FROM_XPATH, error + XML_XPATH_EXPRESSION_OK - XPATH_EXPRESSION_OK,
		    XML_ERR_ERROR, NULL, 0, (const char *)ctxt->base, NULL, NULL, ctxt->cur - ctxt->base, 0, "%s", xmlXPathErrorMessages[error]);
	}
}
/**
 * xmlXPatherror:
 * @ctxt:  the XPath Parser context
 * @file:  the file name
 * @line:  the line number
 * @no:  the error number
 *
 * Formats an error message.
 */
void xmlXPatherror(xmlXPathParserContext * ctxt, /*const char * file ATTRIBUTE_UNUSED, int line ATTRIBUTE_UNUSED,*/int no)
{
	xmlXPathErr(ctxt, no);
}
// 
// Utilities
// 
/**
 * xsltPointerList:
 *
 * Pointer-list for various purposes.
 */
struct xmlPointerList {
	void ** items;
	int    number;
	int    size;
};

//typedef xmlPointerList * xmlPointerListPtr;
/*
 * @todo Since such a list-handling is used in xmlschemas.c and libxslt
 * and here, we should make the functions public.
 */
static int FASTCALL xmlPointerListAddSize(xmlPointerList * list, void * item, int initialSize)
{
	if(list->items == NULL) {
		if(initialSize <= 0)
			initialSize = 1;
		list->items = (void **)SAlloc::M(initialSize * sizeof(void *));
		if(list->items == NULL) {
			xmlXPathErrMemory(NULL, "xmlPointerListCreate: allocating item");
			return -1;
		}
		list->number = 0;
		list->size = initialSize;
	}
	else if(list->size <= list->number) {
		if(list->size > 50000000) {
			xmlXPathErrMemory(NULL, "xmlPointerListAddSize: re-allocating item");
			return -1;
		}
		list->size *= 2;
		list->items = (void **)SAlloc::R(list->items, list->size * sizeof(void *));
		if(list->items == NULL) {
			xmlXPathErrMemory(NULL, "xmlPointerListAddSize: re-allocating item");
			list->size = 0;
			return -1;
		}
	}
	list->items[list->number++] = item;
	return 0;
}
/**
 * xsltPointerListCreate:
 *
 * Creates an xsltPointerList structure.
 *
 * Returns a xsltPointerList structure or NULL in case of an error.
 */
static xmlPointerList * FASTCALL xmlPointerListCreate(int initialSize)
{
	xmlPointerList * ret = (xmlPointerList *)SAlloc::M(sizeof(xmlPointerList));
	if(!ret) {
		xmlXPathErrMemory(NULL, "xmlPointerListCreate: allocating item");
	}
	else {
		memzero(ret, sizeof(xmlPointerList));
		if(initialSize > 0) {
			xmlPointerListAddSize(ret, NULL, initialSize);
			ret->number = 0;
		}
	}
	return ret;
}
/**
 * xsltPointerListFree:
 *
 * Frees the xsltPointerList structure. This does not free
 * the content of the list.
 */
static void xmlPointerListFree(xmlPointerList * list)
{
	if(list) {
		SAlloc::F(list->items);
		SAlloc::F(list);
	}
}
// 
// Parser Types
// 
/*
 * Types are private:
 */
enum xmlXPathOp {
	XPATH_OP_END = 0,
	XPATH_OP_AND,
	XPATH_OP_OR,
	XPATH_OP_EQUAL,
	XPATH_OP_CMP,
	XPATH_OP_PLUS,
	XPATH_OP_MULT,
	XPATH_OP_UNION,
	XPATH_OP_ROOT,
	XPATH_OP_NODE,
	XPATH_OP_RESET, /* 10 */
	XPATH_OP_COLLECT,
	XPATH_OP_VALUE, /* 12 */
	XPATH_OP_VARIABLE,
	XPATH_OP_FUNCTION,
	XPATH_OP_ARG,
	XPATH_OP_PREDICATE,
	XPATH_OP_FILTER, /* 17 */
	XPATH_OP_SORT /* 18 */
#ifdef LIBXML_XPTR_ENABLED
	, XPATH_OP_RANGETO
#endif
};

enum xmlXPathAxisVal {
	AXIS_ANCESTOR = 1,
	AXIS_ANCESTOR_OR_SELF,
	AXIS_ATTRIBUTE,
	AXIS_CHILD,
	AXIS_DESCENDANT,
	AXIS_DESCENDANT_OR_SELF,
	AXIS_FOLLOWING,
	AXIS_FOLLOWING_SIBLING,
	AXIS_NAMESPACE,
	AXIS_PARENT,
	AXIS_PRECEDING,
	AXIS_PRECEDING_SIBLING,
	AXIS_SELF
};

enum xmlXPathTestVal {
	NODE_TEST_NONE = 0,
	NODE_TEST_TYPE = 1,
	NODE_TEST_PI = 2,
	NODE_TEST_ALL = 3,
	NODE_TEST_NS = 4,
	NODE_TEST_NAME = 5
};

enum xmlXPathTypeVal {
	NODE_TYPE_NODE = 0,
	NODE_TYPE_COMMENT = XML_COMMENT_NODE,
	NODE_TYPE_TEXT = XML_TEXT_NODE,
	NODE_TYPE_PI = XML_PI_NODE
};

struct xmlXPathStepOp {
	xmlXPathOp op; /* The identifier of the operation */
	int ch1; /* First child */
	int ch2; /* Second child */
	int value;
	int value2;
	int value3;
	void * value4;
	void * value5;
	void * cache;
	void * cacheURI;
};
//typedef xmlXPathStepOp * xmlXPathStepOpPtr;

struct _xmlXPathCompExpr {
	int nbStep; /* Number of steps in this expression */
	int maxStep; /* Maximum number of steps allocated */
	xmlXPathStepOp * steps; /* ops for computation of this expression */
	int last; /* index of last step in expression */
	xmlChar * expr; /* the expression being computed */
	xmlDict * dict; /* the dictionnary to use if any */
#ifdef DEBUG_EVAL_COUNTS
	int nb;
	xmlChar * string;
#endif
#ifdef XPATH_STREAMING
	xmlPattern * stream;
#endif
};
// 
// Forward declarations
// 
static void xmlXPathFreeValueTree(xmlNodeSet * obj);
static void FASTCALL xmlXPathReleaseObject(xmlXPathContext * ctxt, xmlXPathObject * obj);
static int xmlXPathCompOpEvalFirst(xmlXPathParserContext * ctxt, xmlXPathStepOp * op, xmlNode * * first);
static int xmlXPathCompOpEvalToBoolean(xmlXPathParserContext * ctxt, xmlXPathStepOp * op, int isPredicate);
// 
// Parser Type functions
// 
/**
 * xmlXPathNewCompExpr:
 *
 * Create a new Xpath component
 *
 * Returns the newly allocated xmlXPathCompExprPtr or NULL in case of error
 */
static xmlXPathCompExprPtr xmlXPathNewCompExpr()
{
	xmlXPathCompExprPtr cur = (xmlXPathCompExprPtr)SAlloc::M(sizeof(xmlXPathCompExpr));
	if(!cur) {
		xmlXPathErrMemory(NULL, "allocating component");
		return 0;
	}
	memzero(cur, sizeof(xmlXPathCompExpr));
	cur->maxStep = 10;
	cur->nbStep = 0;
	cur->steps = (xmlXPathStepOp*)SAlloc::M(cur->maxStep * sizeof(xmlXPathStepOp));
	if(cur->steps == NULL) {
		xmlXPathErrMemory(NULL, "allocating steps");
		SAlloc::F(cur);
		return 0;
	}
	memzero(cur->steps, cur->maxStep * sizeof(xmlXPathStepOp));
	cur->last = -1;
#ifdef DEBUG_EVAL_COUNTS
	cur->nb = 0;
#endif
	return cur;
}

/**
 * xmlXPathFreeCompExpr:
 * @comp:  an XPATH comp
 *
 * Free up the memory allocated by @comp
 */
void xmlXPathFreeCompExpr(xmlXPathCompExprPtr comp)
{
	if(comp) {
		xmlXPathStepOp * op;
		int i;
		if(comp->dict == NULL) {
			for(i = 0; i < comp->nbStep; i++) {
				op = &comp->steps[i];
				if(op->value4) {
					if(op->op == XPATH_OP_VALUE)
						xmlXPathFreeObject((xmlXPathObject *)op->value4);
					else
						SAlloc::F(op->value4);
				}
				SAlloc::F(op->value5);
			}
		}
		else {
			for(i = 0; i < comp->nbStep; i++) {
				op = &comp->steps[i];
				if(op->value4) {
					if(op->op == XPATH_OP_VALUE)
						xmlXPathFreeObject((xmlXPathObject *)op->value4);
				}
			}
			xmlDictFree(comp->dict);
		}
		SAlloc::F(comp->steps);
	#ifdef DEBUG_EVAL_COUNTS
		SAlloc::F(comp->string);
	#endif
	#ifdef XPATH_STREAMING
		xmlFreePatternList(comp->stream);
	#endif
		SAlloc::F(comp->expr);
		SAlloc::F(comp);
	}
}

/**
 * xmlXPathCompExprAdd:
 * @comp:  the compiled expression
 * @ch1: first child index
 * @ch2: second child index
 * @op:  an op
 * @value:  the first int value
 * @value2:  the second int value
 * @value3:  the third int value
 * @value4:  the first string value
 * @value5:  the second string value
 *
 * Add a step to an XPath Compiled Expression
 *
 * Returns -1 in case of failure, the index otherwise
 */
static int xmlXPathCompExprAdd(xmlXPathCompExprPtr comp, int ch1, int ch2,
    xmlXPathOp op, int value, int value2, int value3, void * value4, void * value5)
{
	if(comp->nbStep >= comp->maxStep) {
		xmlXPathStepOp * real;
		if(comp->maxStep >= XPATH_MAX_STEPS) {
			xmlXPathErrMemory(NULL, "adding step");
			return -1;
		}
		comp->maxStep *= 2;
		real = (xmlXPathStepOp*)SAlloc::R(comp->steps, comp->maxStep * sizeof(xmlXPathStepOp));
		if(real == NULL) {
			comp->maxStep /= 2;
			xmlXPathErrMemory(NULL, "adding step");
			return -1;
		}
		comp->steps = real;
	}
	comp->last = comp->nbStep;
	comp->steps[comp->nbStep].ch1 = ch1;
	comp->steps[comp->nbStep].ch2 = ch2;
	comp->steps[comp->nbStep].op = op;
	comp->steps[comp->nbStep].value = value;
	comp->steps[comp->nbStep].value2 = value2;
	comp->steps[comp->nbStep].value3 = value3;
	if(comp->dict && oneof3(op, XPATH_OP_FUNCTION, XPATH_OP_VARIABLE, XPATH_OP_COLLECT)) {
		if(value4) {
			comp->steps[comp->nbStep].value4 = (void *)(xmlDictLookupSL(comp->dict, (const xmlChar *)value4)); // @badcast
			SAlloc::F(value4);
		}
		else
			comp->steps[comp->nbStep].value4 = NULL;
		if(value5) {
			comp->steps[comp->nbStep].value5 = (void *)xmlDictLookupSL(comp->dict, (const xmlChar *)value5); // @badcast
			SAlloc::F(value5);
		}
		else
			comp->steps[comp->nbStep].value5 = NULL;
	}
	else {
		comp->steps[comp->nbStep].value4 = value4;
		comp->steps[comp->nbStep].value5 = value5;
	}
	comp->steps[comp->nbStep].cache = NULL;
	return (comp->nbStep++);
}

/**
 * xmlXPathCompSwap:
 * @comp:  the compiled expression
 * @op: operation index
 *
 * Swaps 2 operations in the compiled expression
 */
static void xmlXPathCompSwap(xmlXPathStepOp * op) 
{
#ifndef LIBXML_THREAD_ENABLED
	/*
	 * Since this manipulates possibly shared variables, this is
	 * disabled if one detects that the library is used in a multithreaded
	 * application
	 */
	if(xmlXPathDisableOptimizer)
		return;
#endif
	int tmp = op->ch1;
	op->ch1 = op->ch2;
	op->ch2 = tmp;
}

#define PUSH_FULL_EXPR(op, op1, op2, val, val2, val3, val4, val5) xmlXPathCompExprAdd(ctxt->comp, (op1), (op2), (op), (val), (val2), (val3), (val4), (val5))
#define PUSH_LONG_EXPR(op, val, val2, val3, val4, val5) xmlXPathCompExprAdd(ctxt->comp, ctxt->comp->last, -1, (op), (val), (val2), (val3), (val4), (val5))
#define PUSH_LEAVE_EXPR(op, val, val2)            xmlXPathCompExprAdd(ctxt->comp, -1, -1, (op), (val), (val2), 0, NULL, NULL)
#define PUSH_UNARY_EXPR(op, ch, val, val2)        xmlXPathCompExprAdd(ctxt->comp, (ch), -1, (op), (val), (val2), 0, NULL, NULL)
#define PUSH_BINARY_EXPR(op, ch1, ch2, val, val2) xmlXPathCompExprAdd(ctxt->comp, (ch1), (ch2), (op), (val), (val2), 0, NULL, NULL)
//
// XPath object cache structures
//
/* #define XP_DEFAULT_CACHE_ON */

#define XP_HAS_CACHE(c) (c && (c)->cache)

typedef struct _xmlXPathContextCache xmlXPathContextCache;
typedef xmlXPathContextCache * xmlXPathContextCachePtr;
struct _xmlXPathContextCache {
	xmlPointerList * nodesetObjs; /* contains xmlXPathObjectPtr */
	xmlPointerList * stringObjs; /* contains xmlXPathObjectPtr */
	xmlPointerList * booleanObjs; /* contains xmlXPathObjectPtr */
	xmlPointerList * numberObjs; /* contains xmlXPathObjectPtr */
	xmlPointerList * miscObjs; /* contains xmlXPathObjectPtr */
	int maxNodeset;
	int maxString;
	int maxBoolean;
	int maxNumber;
	int maxMisc;
#ifdef XP_DEBUG_OBJ_USAGE
	int dbgCachedAll;
	int dbgCachedNodeset;
	int dbgCachedString;
	int dbgCachedBool;
	int dbgCachedNumber;
	int dbgCachedPoint;
	int dbgCachedRange;
	int dbgCachedLocset;
	int dbgCachedUsers;
	int dbgCachedXSLTTree;
	int dbgCachedUndefined;

	int dbgReusedAll;
	int dbgReusedNodeset;
	int dbgReusedString;
	int dbgReusedBool;
	int dbgReusedNumber;
	int dbgReusedPoint;
	int dbgReusedRange;
	int dbgReusedLocset;
	int dbgReusedUsers;
	int dbgReusedXSLTTree;
	int dbgReusedUndefined;

#endif
};
//
// Debugging related functions
// 
#define STRANGE xmlGenericError(0, "Internal error at %s:%d\n", __FILE__, __LINE__);

#ifdef LIBXML_DEBUG_ENABLED
static void xmlXPathDebugDumpNode(FILE * output, xmlNode * cur, int depth) 
{
	int i;
	char shift[100];
	for(i = 0; ((i < depth) && (i < 25)); i++)
		shift[2 * i] = shift[2 * i + 1] = ' ';
	shift[2 * i] = shift[2 * i + 1] = 0;
	if(!cur) {
		fprintf(output, "%s", shift);
		fprintf(output, "Node is NULL!\n");
		return;
	}
	if(oneof2(cur->type, XML_DOCUMENT_NODE, XML_HTML_DOCUMENT_NODE)) {
		fprintf(output, "%s", shift);
		fprintf(output, " /\n");
	}
	else if(cur->type == XML_ATTRIBUTE_NODE)
		xmlDebugDumpAttr(output, reinterpret_cast<xmlAttr *>(cur), depth);
	else
		xmlDebugDumpOneNode(output, cur, depth);
}

static void xmlXPathDebugDumpNodeList(FILE * output, xmlNode * cur, int depth)
{
	xmlNode * tmp;
	int i;
	char shift[100];
	for(i = 0; ((i < depth) && (i < 25)); i++)
		shift[2 * i] = shift[2 * i + 1] = ' ';
	shift[2 * i] = shift[2 * i + 1] = 0;
	if(!cur) {
		fprintf(output, "%s", shift);
		fprintf(output, "Node is NULL!\n");
		return;
	}
	while(cur) {
		tmp = cur;
		cur = cur->next;
		xmlDebugDumpOneNode(output, tmp, depth);
	}
}

static void xmlXPathDebugDumpNodeSet(FILE * output, xmlNodeSet * cur, int depth) 
{
	int i;
	char shift[100];
	for(i = 0; ((i < depth) && (i < 25)); i++)
		shift[2 * i] = shift[2 * i + 1] = ' ';
	shift[2 * i] = shift[2 * i + 1] = 0;
	if(!cur) {
		fprintf(output, "%s", shift);
		fprintf(output, "NodeSet is NULL!\n");
		return;
	}
	if(cur) {
		fprintf(output, "Set contains %d nodes:\n", cur->nodeNr);
		for(i = 0; i < cur->nodeNr; i++) {
			fprintf(output, "%s", shift);
			fprintf(output, "%d", i + 1);
			xmlXPathDebugDumpNode(output, cur->PP_NodeTab[i], depth + 1);
		}
	}
}

static void xmlXPathDebugDumpValueTree(FILE * output, xmlNodeSet * cur, int depth)
{
	int i;
	char shift[100];
	for(i = 0; ((i < depth) && (i < 25)); i++)
		shift[2 * i] = shift[2 * i + 1] = ' ';
	shift[2 * i] = shift[2 * i + 1] = 0;
	if(!cur || (cur->nodeNr == 0) || (cur->PP_NodeTab[0] == NULL)) {
		fprintf(output, "%s", shift);
		fprintf(output, "Value Tree is NULL!\n");
		return;
	}
	fprintf(output, "%s", shift);
	fprintf(output, "%d", i + 1);
	xmlXPathDebugDumpNodeList(output, cur->PP_NodeTab[0]->children, depth + 1);
}

#if defined(LIBXML_XPTR_ENABLED)
static void xmlXPathDebugDumpLocationSet(FILE * output, xmlLocationSet * cur, int depth)
{
	int i;
	char shift[100];
	for(i = 0; ((i < depth) && (i < 25)); i++)
		shift[2 * i] = shift[2 * i + 1] = ' ';
	shift[2 * i] = shift[2 * i + 1] = 0;
	if(!cur) {
		fprintf(output, "%s", shift);
		fprintf(output, "LocationSet is NULL!\n");
		return;
	}
	for(i = 0; i < cur->locNr; i++) {
		fprintf(output, "%s", shift);
		fprintf(output, "%d : ", i + 1);
		xmlXPathDebugDumpObject(output, cur->locTab[i], depth + 1);
	}
}

#endif /* LIBXML_XPTR_ENABLED */

/**
 * xmlXPathDebugDumpObject:
 * @output:  the FILE * to dump the output
 * @cur:  the object to inspect
 * @depth:  indentation level
 *
 * Dump the content of the object for debugging purposes
 */
void xmlXPathDebugDumpObject(FILE * output, xmlXPathObject * cur, int depth)
{
	int  i;
	char shift[128];
	if(output) {
		for(i = 0; ((i < depth) && (i < 25)); i++)
			shift[2 * i] = shift[2 * i + 1] = ' ';
		shift[2 * i] = shift[2 * i + 1] = 0;
		fprintf(output, "%s", shift);
		if(!cur)
			fprintf(output, "Object is empty (NULL)\n");
		else {
			switch(cur->type) {
				case XPATH_UNDEFINED:
					fprintf(output, "Object is uninitialized\n");
					break;
				case XPATH_NODESET:
					fprintf(output, "Object is a Node Set :\n");
					xmlXPathDebugDumpNodeSet(output, cur->nodesetval, depth);
					break;
				case XPATH_XSLT_TREE:
					fprintf(output, "Object is an XSLT value tree :\n");
					xmlXPathDebugDumpValueTree(output, cur->nodesetval, depth);
					break;
				case XPATH_BOOLEAN:
					fprintf(output, "Object is a Boolean : ");
					if(cur->boolval) 
						fprintf(output, "true\n");
					else 
						fprintf(output, "false\n");
					break;
				case XPATH_NUMBER:
					switch(xmlXPathIsInf(cur->floatval)) {
						case 1:
							fprintf(output, "Object is a number : Infinity\n");
							break;
						case -1:
							fprintf(output, "Object is a number : -Infinity\n");
							break;
						default:
							if(fisnan(cur->floatval))
								fprintf(output, "Object is a number : NaN\n");
							else if(cur->floatval == 0 && xmlXPathGetSign(cur->floatval) != 0)
								fprintf(output, "Object is a number : 0\n");
							else
								fprintf(output, "Object is a number : %0g\n", cur->floatval);
					}
					break;
				case XPATH_STRING:
					fprintf(output, "Object is a string : ");
					xmlDebugDumpString(output, cur->stringval);
					fprintf(output, "\n");
					break;
				case XPATH_POINT:
					fprintf(output, "Object is a point : index %d in node", cur->index);
					xmlXPathDebugDumpNode(output, static_cast<xmlNode *>(cur->user), depth + 1);
					fprintf(output, "\n");
					break;
				case XPATH_RANGE:
					if((cur->user2 == NULL) || ((cur->user2 == cur->user) && (cur->index == cur->index2))) {
						fprintf(output, "Object is a collapsed range :\n");
						fprintf(output, "%s", shift);
						if(cur->index >= 0)
							fprintf(output, "index %d in ", cur->index);
						fprintf(output, "node\n");
						xmlXPathDebugDumpNode(output, (xmlNode *)cur->user,
						depth + 1);
					}
					else {
						fprintf(output, "Object is a range :\n");
						fprintf(output, "%s", shift);
						fprintf(output, "From ");
						if(cur->index >= 0)
							fprintf(output, "index %d in ", cur->index);
						fprintf(output, "node\n");
						xmlXPathDebugDumpNode(output, (xmlNode *)cur->user,
						depth + 1);
						fprintf(output, "%s", shift);
						fprintf(output, "To ");
						if(cur->index2 >= 0)
							fprintf(output, "index %d in ", cur->index2);
						fprintf(output, "node\n");
						xmlXPathDebugDumpNode(output, (xmlNode *)cur->user2,
						depth + 1);
						fprintf(output, "\n");
					}
					break;
				case XPATH_LOCATIONSET:
		#if defined(LIBXML_XPTR_ENABLED)
					fprintf(output, "Object is a Location Set:\n");
					xmlXPathDebugDumpLocationSet(output, (xmlLocationSet *)cur->user, depth);
		#endif
					break;
				case XPATH_USERS:
					fprintf(output, "Object is user defined\n");
					break;
			}
		}
	}
}

static void xmlXPathDebugDumpStepOp(FILE * output, xmlXPathCompExprPtr comp, xmlXPathStepOp * op, int depth)
{
	int i;
	char shift[100];
	for(i = 0; ((i < depth) && (i < 25)); i++)
		shift[2 * i] = shift[2 * i + 1] = ' ';
	shift[2 * i] = shift[2 * i + 1] = 0;
	fprintf(output, "%s", shift);
	if(!op) {
		fprintf(output, "Step is NULL\n");
		return;
	}
	switch(op->op) {
		case XPATH_OP_END: fprintf(output, "END"); break;
		case XPATH_OP_AND: fprintf(output, "AND"); break;
		case XPATH_OP_OR: fprintf(output, "OR"); break;
		case XPATH_OP_EQUAL:
		    fprintf(output, op->value ? "EQUAL =" : "EQUAL !=");
		    break;
		case XPATH_OP_CMP:
		    fprintf(output, op->value ? "CMP <" : "CMP >");
		    if(!op->value2)
			    fprintf(output, "=");
		    break;
		case XPATH_OP_PLUS:
		    if(op->value == 0)
			    fprintf(output, "PLUS -");
		    else if(op->value == 1)
			    fprintf(output, "PLUS +");
		    else if(op->value == 2)
			    fprintf(output, "PLUS unary -");
		    else if(op->value == 3)
			    fprintf(output, "PLUS unary - -");
		    break;
		case XPATH_OP_MULT:
		    if(op->value == 0)
			    fprintf(output, "MULT *");
		    else if(op->value == 1)
			    fprintf(output, "MULT div");
		    else
			    fprintf(output, "MULT mod");
		    break;
		case XPATH_OP_UNION: fprintf(output, "UNION"); break;
		case XPATH_OP_ROOT:  fprintf(output, "ROOT"); break;
		case XPATH_OP_NODE:  fprintf(output, "NODE"); break;
		case XPATH_OP_RESET: fprintf(output, "RESET"); break;
		case XPATH_OP_SORT:  fprintf(output, "SORT"); break;
		case XPATH_OP_COLLECT: {
		    xmlXPathAxisVal axis = (xmlXPathAxisVal)op->value;
		    xmlXPathTestVal test = (xmlXPathTestVal)op->value2;
		    xmlXPathTypeVal type = (xmlXPathTypeVal)op->value3;
		    const xmlChar * prefix = (const xmlChar *)op->value4;
		    const xmlChar * name = (const xmlChar *)op->value5;

		    fprintf(output, "COLLECT ");
		    switch(axis) {
			    case AXIS_ANCESTOR: fprintf(output, " 'ancestors' "); break;
			    case AXIS_ANCESTOR_OR_SELF: fprintf(output, " 'ancestors-or-self' "); break;
			    case AXIS_ATTRIBUTE: fprintf(output, " 'attributes' "); break;
			    case AXIS_CHILD: fprintf(output, " 'child' "); break;
			    case AXIS_DESCENDANT: fprintf(output, " 'descendant' "); break;
			    case AXIS_DESCENDANT_OR_SELF: fprintf(output, " 'descendant-or-self' "); break;
			    case AXIS_FOLLOWING: fprintf(output, " 'following' "); break;
			    case AXIS_FOLLOWING_SIBLING: fprintf(output, " 'following-siblings' "); break;
			    case AXIS_NAMESPACE: fprintf(output, " 'namespace' "); break;
			    case AXIS_PARENT: fprintf(output, " 'parent' "); break;
			    case AXIS_PRECEDING: fprintf(output, " 'preceding' "); break;
			    case AXIS_PRECEDING_SIBLING: fprintf(output, " 'preceding-sibling' "); break;
			    case AXIS_SELF: fprintf(output, " 'self' "); break;
		    }
		    switch(test) {
			    case NODE_TEST_NONE: fprintf(output, "'none' "); break;
			    case NODE_TEST_TYPE: fprintf(output, "'type' "); break;
			    case NODE_TEST_PI: fprintf(output, "'PI' "); break;
			    case NODE_TEST_ALL: fprintf(output, "'all' "); break;
			    case NODE_TEST_NS: fprintf(output, "'namespace' "); break;
			    case NODE_TEST_NAME: fprintf(output, "'name' "); break;
		    }
		    switch(type) {
			    case NODE_TYPE_NODE: fprintf(output, "'node' "); break;
			    case NODE_TYPE_COMMENT: fprintf(output, "'comment' "); break;
			    case NODE_TYPE_TEXT: fprintf(output, "'text' "); break;
			    case NODE_TYPE_PI: fprintf(output, "'PI' "); break;
		    }
		    if(prefix)
			    fprintf(output, "%s:", prefix);
		    if(name)
			    fprintf(output, "%s", (const char *)name);
		    break;
	    }
		case XPATH_OP_VALUE: {
		    xmlXPathObject * object = (xmlXPathObject *)op->value4;
		    fprintf(output, "ELEM ");
		    xmlXPathDebugDumpObject(output, object, 0);
		    goto finish;
	    }
		case XPATH_OP_VARIABLE: {
		    const xmlChar * prefix = (const xmlChar *)op->value5;
		    const xmlChar * name = (const xmlChar *)op->value4;
		    if(prefix)
			    fprintf(output, "VARIABLE %s:%s", prefix, name);
		    else
			    fprintf(output, "VARIABLE %s", name);
		    break;
	    }
		case XPATH_OP_FUNCTION: {
		    int nbargs = op->value;
		    const xmlChar * prefix = (const xmlChar *)op->value5;
		    const xmlChar * name = (const xmlChar *)op->value4;
		    if(prefix)
			    fprintf(output, "FUNCTION %s:%s(%d args)", prefix, name, nbargs);
		    else
			    fprintf(output, "FUNCTION %s(%d args)", name, nbargs);
		    break;
	    }
		case XPATH_OP_ARG: fprintf(output, "ARG"); break;
		case XPATH_OP_PREDICATE: fprintf(output, "PREDICATE"); break;
		case XPATH_OP_FILTER: fprintf(output, "FILTER"); break;
#ifdef LIBXML_XPTR_ENABLED
		case XPATH_OP_RANGETO: fprintf(output, "RANGETO"); break;
#endif
		default:
		    fprintf(output, "UNKNOWN %d\n", op->op); return;
	}
	fprintf(output, "\n");
finish:
	if(op->ch1 >= 0)
		xmlXPathDebugDumpStepOp(output, comp, &comp->steps[op->ch1], depth + 1);
	if(op->ch2 >= 0)
		xmlXPathDebugDumpStepOp(output, comp, &comp->steps[op->ch2], depth + 1);
}

/**
 * xmlXPathDebugDumpCompExpr:
 * @output:  the FILE * for the output
 * @comp:  the precompiled XPath expression
 * @depth:  the indentation level.
 *
 * Dumps the tree of the compiled XPath expression.
 */
void xmlXPathDebugDumpCompExpr(FILE * output, xmlXPathCompExprPtr comp, int depth) 
{
	if(output && comp) {
		int i;
		char shift[100];
		for(i = 0; ((i < depth) && (i < 25)); i++)
			shift[2 * i] = shift[2 * i + 1] = ' ';
		shift[2 * i] = shift[2 * i + 1] = 0;
		fprintf(output, "%s", shift);
		fprintf(output, "Compiled Expression : %d elements\n", comp->nbStep);
		i = comp->last;
		xmlXPathDebugDumpStepOp(output, comp, &comp->steps[i], depth + 1);
	}
}

#ifdef XP_DEBUG_OBJ_USAGE

/*
 * XPath object usage related debugging variables.
 */
static int xmlXPathDebugObjCounterUndefined = 0;
static int xmlXPathDebugObjCounterNodeset = 0;
static int xmlXPathDebugObjCounterBool = 0;
static int xmlXPathDebugObjCounterNumber = 0;
static int xmlXPathDebugObjCounterString = 0;
static int xmlXPathDebugObjCounterPoint = 0;
static int xmlXPathDebugObjCounterRange = 0;
static int xmlXPathDebugObjCounterLocset = 0;
static int xmlXPathDebugObjCounterUsers = 0;
static int xmlXPathDebugObjCounterXSLTTree = 0;
static int xmlXPathDebugObjCounterAll = 0;

static int xmlXPathDebugObjTotalUndefined = 0;
static int xmlXPathDebugObjTotalNodeset = 0;
static int xmlXPathDebugObjTotalBool = 0;
static int xmlXPathDebugObjTotalNumber = 0;
static int xmlXPathDebugObjTotalString = 0;
static int xmlXPathDebugObjTotalPoint = 0;
static int xmlXPathDebugObjTotalRange = 0;
static int xmlXPathDebugObjTotalLocset = 0;
static int xmlXPathDebugObjTotalUsers = 0;
static int xmlXPathDebugObjTotalXSLTTree = 0;
static int xmlXPathDebugObjTotalAll = 0;

static int xmlXPathDebugObjMaxUndefined = 0;
static int xmlXPathDebugObjMaxNodeset = 0;
static int xmlXPathDebugObjMaxBool = 0;
static int xmlXPathDebugObjMaxNumber = 0;
static int xmlXPathDebugObjMaxString = 0;
static int xmlXPathDebugObjMaxPoint = 0;
static int xmlXPathDebugObjMaxRange = 0;
static int xmlXPathDebugObjMaxLocset = 0;
static int xmlXPathDebugObjMaxUsers = 0;
static int xmlXPathDebugObjMaxXSLTTree = 0;
static int xmlXPathDebugObjMaxAll = 0;

/* REVISIT TODO: Make this static when committing */
static void xmlXPathDebugObjUsageReset(xmlXPathContext * ctxt)
{
	if(ctxt) {
		if(ctxt->cache) {
			xmlXPathContextCachePtr cache = (xmlXPathContextCachePtr)ctxt->cache;
			cache->dbgCachedAll = 0;
			cache->dbgCachedNodeset = 0;
			cache->dbgCachedString = 0;
			cache->dbgCachedBool = 0;
			cache->dbgCachedNumber = 0;
			cache->dbgCachedPoint = 0;
			cache->dbgCachedRange = 0;
			cache->dbgCachedLocset = 0;
			cache->dbgCachedUsers = 0;
			cache->dbgCachedXSLTTree = 0;
			cache->dbgCachedUndefined = 0;

			cache->dbgReusedAll = 0;
			cache->dbgReusedNodeset = 0;
			cache->dbgReusedString = 0;
			cache->dbgReusedBool = 0;
			cache->dbgReusedNumber = 0;
			cache->dbgReusedPoint = 0;
			cache->dbgReusedRange = 0;
			cache->dbgReusedLocset = 0;
			cache->dbgReusedUsers = 0;
			cache->dbgReusedXSLTTree = 0;
			cache->dbgReusedUndefined = 0;
		}
	}

	xmlXPathDebugObjCounterUndefined = 0;
	xmlXPathDebugObjCounterNodeset = 0;
	xmlXPathDebugObjCounterBool = 0;
	xmlXPathDebugObjCounterNumber = 0;
	xmlXPathDebugObjCounterString = 0;
	xmlXPathDebugObjCounterPoint = 0;
	xmlXPathDebugObjCounterRange = 0;
	xmlXPathDebugObjCounterLocset = 0;
	xmlXPathDebugObjCounterUsers = 0;
	xmlXPathDebugObjCounterXSLTTree = 0;
	xmlXPathDebugObjCounterAll = 0;

	xmlXPathDebugObjTotalUndefined = 0;
	xmlXPathDebugObjTotalNodeset = 0;
	xmlXPathDebugObjTotalBool = 0;
	xmlXPathDebugObjTotalNumber = 0;
	xmlXPathDebugObjTotalString = 0;
	xmlXPathDebugObjTotalPoint = 0;
	xmlXPathDebugObjTotalRange = 0;
	xmlXPathDebugObjTotalLocset = 0;
	xmlXPathDebugObjTotalUsers = 0;
	xmlXPathDebugObjTotalXSLTTree = 0;
	xmlXPathDebugObjTotalAll = 0;

	xmlXPathDebugObjMaxUndefined = 0;
	xmlXPathDebugObjMaxNodeset = 0;
	xmlXPathDebugObjMaxBool = 0;
	xmlXPathDebugObjMaxNumber = 0;
	xmlXPathDebugObjMaxString = 0;
	xmlXPathDebugObjMaxPoint = 0;
	xmlXPathDebugObjMaxRange = 0;
	xmlXPathDebugObjMaxLocset = 0;
	xmlXPathDebugObjMaxUsers = 0;
	xmlXPathDebugObjMaxXSLTTree = 0;
	xmlXPathDebugObjMaxAll = 0;
}

static void xmlXPathDebugObjUsageRequested(xmlXPathContext * ctxt, xmlXPathObjectType objType)
{
	int isCached = 0;
	if(ctxt) {
		if(ctxt->cache) {
			xmlXPathContextCachePtr cache = (xmlXPathContextCachePtr)ctxt->cache;
			isCached = 1;
			cache->dbgReusedAll++;
			switch(objType) {
				case XPATH_UNDEFINED: cache->dbgReusedUndefined++; break;
				case XPATH_NODESET: cache->dbgReusedNodeset++; break;
				case XPATH_BOOLEAN: cache->dbgReusedBool++; break;
				case XPATH_NUMBER: cache->dbgReusedNumber++; break;
				case XPATH_STRING: cache->dbgReusedString++; break;
				case XPATH_POINT: cache->dbgReusedPoint++; break;
				case XPATH_RANGE: cache->dbgReusedRange++; break;
				case XPATH_LOCATIONSET: cache->dbgReusedLocset++; break;
				case XPATH_USERS: cache->dbgReusedUsers++; break;
				case XPATH_XSLT_TREE: cache->dbgReusedXSLTTree++; break;
				default:
				    break;
			}
		}
	}
	switch(objType) {
		case XPATH_UNDEFINED:
		    if(!isCached)
			    xmlXPathDebugObjTotalUndefined++;
		    xmlXPathDebugObjCounterUndefined++;
		    if(xmlXPathDebugObjCounterUndefined >
		    xmlXPathDebugObjMaxUndefined)
			    xmlXPathDebugObjMaxUndefined =
			    xmlXPathDebugObjCounterUndefined;
		    break;
		case XPATH_NODESET:
		    if(!isCached)
			    xmlXPathDebugObjTotalNodeset++;
		    xmlXPathDebugObjCounterNodeset++;
		    if(xmlXPathDebugObjCounterNodeset > xmlXPathDebugObjMaxNodeset)
			    xmlXPathDebugObjMaxNodeset = xmlXPathDebugObjCounterNodeset;
		    break;
		case XPATH_BOOLEAN:
		    if(!isCached)
			    xmlXPathDebugObjTotalBool++;
		    xmlXPathDebugObjCounterBool++;
		    if(xmlXPathDebugObjCounterBool > xmlXPathDebugObjMaxBool)
			    xmlXPathDebugObjMaxBool = xmlXPathDebugObjCounterBool;
		    break;
		case XPATH_NUMBER:
		    if(!isCached)
			    xmlXPathDebugObjTotalNumber++;
		    xmlXPathDebugObjCounterNumber++;
		    if(xmlXPathDebugObjCounterNumber > xmlXPathDebugObjMaxNumber)
			    xmlXPathDebugObjMaxNumber = xmlXPathDebugObjCounterNumber;
		    break;
		case XPATH_STRING:
		    if(!isCached)
			    xmlXPathDebugObjTotalString++;
		    xmlXPathDebugObjCounterString++;
		    if(xmlXPathDebugObjCounterString > xmlXPathDebugObjMaxString)
			    xmlXPathDebugObjMaxString = xmlXPathDebugObjCounterString;
		    break;
		case XPATH_POINT:
		    if(!isCached)
			    xmlXPathDebugObjTotalPoint++;
		    xmlXPathDebugObjCounterPoint++;
		    if(xmlXPathDebugObjCounterPoint > xmlXPathDebugObjMaxPoint)
			    xmlXPathDebugObjMaxPoint = xmlXPathDebugObjCounterPoint;
		    break;
		case XPATH_RANGE:
		    if(!isCached)
			    xmlXPathDebugObjTotalRange++;
		    xmlXPathDebugObjCounterRange++;
		    if(xmlXPathDebugObjCounterRange > xmlXPathDebugObjMaxRange)
			    xmlXPathDebugObjMaxRange = xmlXPathDebugObjCounterRange;
		    break;
		case XPATH_LOCATIONSET:
		    if(!isCached)
			    xmlXPathDebugObjTotalLocset++;
		    xmlXPathDebugObjCounterLocset++;
		    if(xmlXPathDebugObjCounterLocset > xmlXPathDebugObjMaxLocset)
			    xmlXPathDebugObjMaxLocset = xmlXPathDebugObjCounterLocset;
		    break;
		case XPATH_USERS:
		    if(!isCached)
			    xmlXPathDebugObjTotalUsers++;
		    xmlXPathDebugObjCounterUsers++;
		    if(xmlXPathDebugObjCounterUsers > xmlXPathDebugObjMaxUsers)
			    xmlXPathDebugObjMaxUsers = xmlXPathDebugObjCounterUsers;
		    break;
		case XPATH_XSLT_TREE:
		    if(!isCached)
			    xmlXPathDebugObjTotalXSLTTree++;
		    xmlXPathDebugObjCounterXSLTTree++;
		    if(xmlXPathDebugObjCounterXSLTTree > xmlXPathDebugObjMaxXSLTTree)
			    xmlXPathDebugObjMaxXSLTTree = xmlXPathDebugObjCounterXSLTTree;
		    break;
		default:
		    break;
	}
	if(!isCached)
		xmlXPathDebugObjTotalAll++;
	xmlXPathDebugObjCounterAll++;
	if(xmlXPathDebugObjCounterAll > xmlXPathDebugObjMaxAll)
		xmlXPathDebugObjMaxAll = xmlXPathDebugObjCounterAll;
}

static void xmlXPathDebugObjUsageReleased(xmlXPathContext * ctxt, xmlXPathObjectType objType)
{
	int isCached = 0;
	if(ctxt) {
		if(ctxt->cache) {
			xmlXPathContextCachePtr cache = (xmlXPathContextCachePtr)ctxt->cache;
			isCached = 1;
			cache->dbgCachedAll++;
			switch(objType) {
				case XPATH_UNDEFINED: cache->dbgCachedUndefined++; break;
				case XPATH_NODESET: cache->dbgCachedNodeset++; break;
				case XPATH_BOOLEAN: cache->dbgCachedBool++; break;
				case XPATH_NUMBER: cache->dbgCachedNumber++; break;
				case XPATH_STRING: cache->dbgCachedString++; break;
				case XPATH_POINT: cache->dbgCachedPoint++; break;
				case XPATH_RANGE: cache->dbgCachedRange++; break;
				case XPATH_LOCATIONSET: cache->dbgCachedLocset++; break;
				case XPATH_USERS: cache->dbgCachedUsers++; break;
				case XPATH_XSLT_TREE: cache->dbgCachedXSLTTree++; break;
				default: break;
			}
		}
	}
	switch(objType) {
		case XPATH_UNDEFINED: xmlXPathDebugObjCounterUndefined--; break;
		case XPATH_NODESET: xmlXPathDebugObjCounterNodeset--; break;
		case XPATH_BOOLEAN: xmlXPathDebugObjCounterBool--; break;
		case XPATH_NUMBER: xmlXPathDebugObjCounterNumber--; break;
		case XPATH_STRING: xmlXPathDebugObjCounterString--; break;
		case XPATH_POINT: xmlXPathDebugObjCounterPoint--; break;
		case XPATH_RANGE: xmlXPathDebugObjCounterRange--; break;
		case XPATH_LOCATIONSET: xmlXPathDebugObjCounterLocset--; break;
		case XPATH_USERS: xmlXPathDebugObjCounterUsers--; break;
		case XPATH_XSLT_TREE: xmlXPathDebugObjCounterXSLTTree--; break;
		default: break;
	}
	xmlXPathDebugObjCounterAll--;
}

/* REVISIT TODO: Make this static when committing */
static void xmlXPathDebugObjUsageDisplay(xmlXPathContext * ctxt)
{
	int reqAll, reqNodeset, reqString, reqBool, reqNumber, reqXSLTTree, reqUndefined;
	int caAll = 0, caNodeset = 0, caString = 0, caBool = 0, caNumber = 0, caXSLTTree = 0, caUndefined = 0;
	int reAll = 0, reNodeset = 0, reString = 0, reBool = 0, reNumber = 0, reXSLTTree = 0, reUndefined = 0;
	int leftObjs = xmlXPathDebugObjCounterAll;
	reqAll = xmlXPathDebugObjTotalAll;
	reqNodeset = xmlXPathDebugObjTotalNodeset;
	reqString = xmlXPathDebugObjTotalString;
	reqBool = xmlXPathDebugObjTotalBool;
	reqNumber = xmlXPathDebugObjTotalNumber;
	reqXSLTTree = xmlXPathDebugObjTotalXSLTTree;
	reqUndefined = xmlXPathDebugObjTotalUndefined;
	printf("# XPath object usage:\n");
	if(ctxt) {
		if(ctxt->cache) {
			xmlXPathContextCachePtr cache = (xmlXPathContextCachePtr)ctxt->cache;
			reAll = cache->dbgReusedAll;
			reqAll += reAll;
			reNodeset = cache->dbgReusedNodeset;
			reqNodeset += reNodeset;
			reString = cache->dbgReusedString;
			reqString += reString;
			reBool = cache->dbgReusedBool;
			reqBool += reBool;
			reNumber = cache->dbgReusedNumber;
			reqNumber += reNumber;
			reXSLTTree = cache->dbgReusedXSLTTree;
			reqXSLTTree += reXSLTTree;
			reUndefined = cache->dbgReusedUndefined;
			reqUndefined += reUndefined;

			caAll = cache->dbgCachedAll;
			caBool = cache->dbgCachedBool;
			caNodeset = cache->dbgCachedNodeset;
			caString = cache->dbgCachedString;
			caNumber = cache->dbgCachedNumber;
			caXSLTTree = cache->dbgCachedXSLTTree;
			caUndefined = cache->dbgCachedUndefined;

			if(cache->nodesetObjs)
				leftObjs -= cache->nodesetObjs->number;
			if(cache->stringObjs)
				leftObjs -= cache->stringObjs->number;
			if(cache->booleanObjs)
				leftObjs -= cache->booleanObjs->number;
			if(cache->numberObjs)
				leftObjs -= cache->numberObjs->number;
			if(cache->miscObjs)
				leftObjs -= cache->miscObjs->number;
		}
	}

	printf("# all\n");
	printf("#   total  : %d\n", reqAll);
	printf("#   left  : %d\n", leftObjs);
	printf("#   created: %d\n", xmlXPathDebugObjTotalAll);
	printf("#   reused : %d\n", reAll);
	printf("#   max    : %d\n", xmlXPathDebugObjMaxAll);

	printf("# node-sets\n");
	printf("#   total  : %d\n", reqNodeset);
	printf("#   created: %d\n", xmlXPathDebugObjTotalNodeset);
	printf("#   reused : %d\n", reNodeset);
	printf("#   max    : %d\n", xmlXPathDebugObjMaxNodeset);

	printf("# strings\n");
	printf("#   total  : %d\n", reqString);
	printf("#   created: %d\n", xmlXPathDebugObjTotalString);
	printf("#   reused : %d\n", reString);
	printf("#   max    : %d\n", xmlXPathDebugObjMaxString);

	printf("# booleans\n");
	printf("#   total  : %d\n", reqBool);
	printf("#   created: %d\n", xmlXPathDebugObjTotalBool);
	printf("#   reused : %d\n", reBool);
	printf("#   max    : %d\n", xmlXPathDebugObjMaxBool);

	printf("# numbers\n");
	printf("#   total  : %d\n", reqNumber);
	printf("#   created: %d\n", xmlXPathDebugObjTotalNumber);
	printf("#   reused : %d\n", reNumber);
	printf("#   max    : %d\n", xmlXPathDebugObjMaxNumber);

	printf("# XSLT result tree fragments\n");
	printf("#   total  : %d\n", reqXSLTTree);
	printf("#   created: %d\n", xmlXPathDebugObjTotalXSLTTree);
	printf("#   reused : %d\n", reXSLTTree);
	printf("#   max    : %d\n", xmlXPathDebugObjMaxXSLTTree);

	printf("# undefined\n");
	printf("#   total  : %d\n", reqUndefined);
	printf("#   created: %d\n", xmlXPathDebugObjTotalUndefined);
	printf("#   reused : %d\n", reUndefined);
	printf("#   max    : %d\n", xmlXPathDebugObjMaxUndefined);
}

#endif /* XP_DEBUG_OBJ_USAGE */

#endif /* LIBXML_DEBUG_ENABLED */

/************************************************************************
*									*
*			XPath object caching				*
*									*
************************************************************************/

/**
 * xmlXPathNewCache:
 *
 * Create a new object cache
 *
 * Returns the xmlXPathCache just allocated.
 */
static xmlXPathContextCachePtr xmlXPathNewCache()
{
	xmlXPathContextCachePtr ret = (xmlXPathContextCachePtr)SAlloc::M(sizeof(xmlXPathContextCache));
	if(!ret) {
		xmlXPathErrMemory(NULL, "creating object cache");
	}
	else {
		memzero(ret, sizeof(xmlXPathContextCache));
		ret->maxNodeset = 100;
		ret->maxString = 100;
		ret->maxBoolean = 100;
		ret->maxNumber = 100;
		ret->maxMisc = 100;
	}
	return ret;
}

static void FASTCALL xmlXPathCacheFreeObjectList(xmlPointerList * list)
{
	if(list) {
		for(int i = 0; i < list->number; i++) {
			xmlXPathObject * obj = (xmlXPathObject *)list->items[i];
			/*
			* Note that it is already assured that we don't need to
			* look out for namespace nodes in the node-set.
			*/
			if(obj->nodesetval) {
				SAlloc::F(obj->nodesetval->PP_NodeTab);
				SAlloc::F(obj->nodesetval);
			}
			SAlloc::F(obj);
	#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjCounterAll--;
	#endif
		}
		xmlPointerListFree(list);
	}
}

static void xmlXPathFreeCache(xmlXPathContextCachePtr cache)
{
	if(cache) {
		xmlXPathCacheFreeObjectList(cache->nodesetObjs);
		xmlXPathCacheFreeObjectList(cache->stringObjs);
		xmlXPathCacheFreeObjectList(cache->booleanObjs);
		xmlXPathCacheFreeObjectList(cache->numberObjs);
		xmlXPathCacheFreeObjectList(cache->miscObjs);
		SAlloc::F(cache);
	}
}

/**
 * xmlXPathContextSetCache:
 *
 * @ctxt:  the XPath context
 * @active: enables/disables (creates/frees) the cache
 * @value: a value with semantics dependant on @options
 * @options: options (currently only the value 0 is used)
 *
 * Creates/frees an object cache on the XPath context.
 * If activates XPath objects (xmlXPathObject) will be cached internally
 * to be reused.
 * @options:
 * 0: This will set the XPath object caching:
 * @value:
 *   This will set the maximum number of XPath objects
 *   to be cached per slot
 *   There are 5 slots for: node-set, string, number, boolean, and
 *   misc objects. Use <0 for the default number (100).
 * Other values for @options have currently no effect.
 *
 * Returns 0 if the setting succeeded, and -1 on API or internal errors.
 */
int xmlXPathContextSetCache(xmlXPathContext * ctxt, int active, int value, int options)
{
	if(!ctxt)
		return -1;
	if(active) {
		xmlXPathContextCachePtr cache;
		if(ctxt->cache == NULL) {
			ctxt->cache = xmlXPathNewCache();
			if(ctxt->cache == NULL)
				return -1;
		}
		cache = (xmlXPathContextCachePtr)ctxt->cache;
		if(options == 0) {
			if(value < 0)
				value = 100;
			cache->maxNodeset = value;
			cache->maxString = value;
			cache->maxNumber = value;
			cache->maxBoolean = value;
			cache->maxMisc = value;
		}
	}
	else {
		xmlXPathFreeCache((xmlXPathContextCachePtr)ctxt->cache);
		ctxt->cache = NULL;
	}
	return 0;
}

/**
 * xmlXPathCacheWrapNodeSet:
 * @ctxt: the XPath context
 * @val:  the NodePtr value
 *
 * This is the cached version of xmlXPathWrapNodeSet().
 * Wrap the Nodeset @val in a new xmlXPathObjectPtr
 *
 * Returns the created or reused object.
 */
static xmlXPathObject * xmlXPathCacheWrapNodeSet(xmlXPathContext * ctxt, xmlNodeSet * val)
{
	if(ctxt && ctxt->cache) {
		xmlXPathContextCachePtr cache = (xmlXPathContextCachePtr)ctxt->cache;
		if(cache->miscObjs && (cache->miscObjs->number != 0)) {
			xmlXPathObject * ret = (xmlXPathObject *)cache->miscObjs->items[--cache->miscObjs->number];
			ret->type = XPATH_NODESET;
			ret->nodesetval = val;
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_NODESET);
#endif
			return ret;
		}
	}
	return (xmlXPathWrapNodeSet(val));
}

/**
 * xmlXPathCacheWrapString:
 * @ctxt: the XPath context
 * @val:  the xmlChar * value
 *
 * This is the cached version of xmlXPathWrapString().
 * Wraps the @val string into an XPath object.
 *
 * Returns the created or reused object.
 */
static xmlXPathObject * xmlXPathCacheWrapString(xmlXPathContext * ctxt, xmlChar * val)
{
	if(ctxt && ctxt->cache) {
		xmlXPathContextCachePtr cache = (xmlXPathContextCachePtr)ctxt->cache;
		if(cache->stringObjs && (cache->stringObjs->number != 0)) {
			xmlXPathObject * ret = (xmlXPathObject *)cache->stringObjs->items[--cache->stringObjs->number];
			ret->type = XPATH_STRING;
			ret->stringval = val;
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_STRING);
#endif
			return ret;
		}
		else if(cache->miscObjs && (cache->miscObjs->number != 0)) {
			/*
			 * Fallback to misc-cache.
			 */
			xmlXPathObject * ret = (xmlXPathObject *)cache->miscObjs->items[--cache->miscObjs->number];
			ret->type = XPATH_STRING;
			ret->stringval = val;
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_STRING);
#endif
			return ret;
		}
	}
	return (xmlXPathWrapString(val));
}

/**
 * xmlXPathCacheNewNodeSet:
 * @ctxt: the XPath context
 * @val:  the NodePtr value
 *
 * This is the cached version of xmlXPathNewNodeSet().
 * Acquire an xmlXPathObjectPtr of type NodeSet and initialize
 * it with the single Node @val
 *
 * Returns the created or reused object.
 */
static xmlXPathObject * xmlXPathCacheNewNodeSet(xmlXPathContext * ctxt, xmlNode * val)
{
	if(ctxt && ctxt->cache) {
		xmlXPathContextCachePtr cache = (xmlXPathContextCachePtr)ctxt->cache;
		if(cache->nodesetObjs && (cache->nodesetObjs->number != 0)) {
			// Use the nodset-cache.
			xmlXPathObject * ret = (xmlXPathObject *)cache->nodesetObjs->items[--cache->nodesetObjs->number];
			ret->type = XPATH_NODESET;
			ret->boolval = 0;
			if(val) {
				if((ret->nodesetval->nodeMax == 0) || (val->type == XML_NAMESPACE_DECL)) {
					xmlXPathNodeSetAddUnique(ret->nodesetval, val);
				}
				else {
					ret->nodesetval->PP_NodeTab[0] = val;
					ret->nodesetval->nodeNr = 1;
				}
			}
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_NODESET);
#endif
			return ret;
		}
		else if(cache->miscObjs && (cache->miscObjs->number != 0)) {
			// Fallback to misc-cache.
			xmlXPathObject * ret = (xmlXPathObject *)cache->miscObjs->items[--cache->miscObjs->number];
			ret->type = XPATH_NODESET;
			ret->boolval = 0;
			ret->nodesetval = xmlXPathNodeSetCreate(val);
			if(ret->nodesetval == NULL) {
				ctxt->lastError.domain = XML_FROM_XPATH;
				ctxt->lastError.code = XML_ERR_NO_MEMORY;
				return 0;
			}
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_NODESET);
#endif
			return ret;
		}
	}
	return (xmlXPathNewNodeSet(val));
}

/**
 * xmlXPathCacheNewCString:
 * @ctxt: the XPath context
 * @val:  the char * value
 *
 * This is the cached version of xmlXPathNewCString().
 * Acquire an xmlXPathObjectPtr of type string and of value @val
 *
 * Returns the created or reused object.
 */
static xmlXPathObject * xmlXPathCacheNewCString(xmlXPathContext * ctxt, const char * val)
{
	if(ctxt && ctxt->cache) {
		xmlXPathContextCachePtr cache = (xmlXPathContextCachePtr)ctxt->cache;
		if(cache->stringObjs && (cache->stringObjs->number != 0)) {
			xmlXPathObject * ret = (xmlXPathObject *)cache->stringObjs->items[--cache->stringObjs->number];
			ret->type = XPATH_STRING;
			ret->stringval = sstrdup(BAD_CAST val);
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_STRING);
#endif
			return ret;
		}
		else if(cache->miscObjs && (cache->miscObjs->number != 0)) {
			xmlXPathObject * ret = (xmlXPathObject *)cache->miscObjs->items[--cache->miscObjs->number];
			ret->type = XPATH_STRING;
			ret->stringval = sstrdup(BAD_CAST val);
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_STRING);
#endif
			return ret;
		}
	}
	return (xmlXPathNewCString(val));
}

/**
 * xmlXPathCacheNewString:
 * @ctxt: the XPath context
 * @val:  the xmlChar * value
 *
 * This is the cached version of xmlXPathNewString().
 * Acquire an xmlXPathObjectPtr of type string and of value @val
 *
 * Returns the created or reused object.
 */
static xmlXPathObject * xmlXPathCacheNewString(xmlXPathContext * ctxt, const xmlChar * val)
{
	if(ctxt && ctxt->cache) {
		xmlXPathContextCachePtr cache = (xmlXPathContextCachePtr)ctxt->cache;
		if(cache->stringObjs && (cache->stringObjs->number != 0)) {
			xmlXPathObject * ret = (xmlXPathObject *)cache->stringObjs->items[--cache->stringObjs->number];
			ret->type = XPATH_STRING;
			if(val)
				ret->stringval = sstrdup(val);
			else
				ret->stringval = reinterpret_cast<xmlChar *>(sstrdup(""));
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_STRING);
#endif
			return ret;
		}
		else if(cache->miscObjs && (cache->miscObjs->number != 0)) {
			xmlXPathObject * ret = (xmlXPathObject *)cache->miscObjs->items[--cache->miscObjs->number];
			ret->type = XPATH_STRING;
			if(val)
				ret->stringval = sstrdup(val);
			else
				ret->stringval = reinterpret_cast<xmlChar *>(sstrdup(""));
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_STRING);
#endif
			return ret;
		}
	}
	return (xmlXPathNewString(val));
}

/**
 * xmlXPathCacheNewBoolean:
 * @ctxt: the XPath context
 * @val:  the boolean value
 *
 * This is the cached version of xmlXPathNewBoolean().
 * Acquires an xmlXPathObjectPtr of type boolean and of value @val
 *
 * Returns the created or reused object.
 */
static xmlXPathObject * xmlXPathCacheNewBoolean(xmlXPathContext * ctxt, int val)
{
	if(ctxt && ctxt->cache) {
		xmlXPathContextCachePtr cache = (xmlXPathContextCachePtr)ctxt->cache;
		if(cache->booleanObjs && (cache->booleanObjs->number != 0)) {
			xmlXPathObject * ret = (xmlXPathObject *)cache->booleanObjs->items[--cache->booleanObjs->number];
			ret->type = XPATH_BOOLEAN;
			ret->boolval = (val != 0);
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_BOOLEAN);
#endif
			return ret;
		}
		else if(cache->miscObjs && (cache->miscObjs->number != 0)) {
			xmlXPathObject * ret = (xmlXPathObject *)cache->miscObjs->items[--cache->miscObjs->number];
			ret->type = XPATH_BOOLEAN;
			ret->boolval = (val != 0);
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_BOOLEAN);
#endif
			return ret;
		}
	}
	return (xmlXPathNewBoolean(val));
}
/**
 * xmlXPathCacheNewFloat:
 * @ctxt: the XPath context
 * @val:  the double value
 *
 * This is the cached version of xmlXPathNewFloat().
 * Acquires an xmlXPathObjectPtr of type double and of value @val
 *
 * Returns the created or reused object.
 */
static xmlXPathObject * xmlXPathCacheNewFloat(xmlXPathContext * ctxt, double val)
{
	if(ctxt && ctxt->cache) {
		xmlXPathContextCachePtr cache = (xmlXPathContextCachePtr)ctxt->cache;
		if(cache->numberObjs && (cache->numberObjs->number != 0)) {
			xmlXPathObject * ret = (xmlXPathObject *)cache->numberObjs->items[--cache->numberObjs->number];
			ret->type = XPATH_NUMBER;
			ret->floatval = val;
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_NUMBER);
#endif
			return ret;
		}
		else if(cache->miscObjs && (cache->miscObjs->number != 0)) {
			xmlXPathObject * ret = (xmlXPathObject *)cache->miscObjs->items[--cache->miscObjs->number];
			ret->type = XPATH_NUMBER;
			ret->floatval = val;
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageRequested(ctxt, XPATH_NUMBER);
#endif
			return ret;
		}
	}
	return (xmlXPathNewFloat(val));
}
/**
 * xmlXPathCacheConvertString:
 * @ctxt: the XPath context
 * @val:  an XPath object
 *
 * This is the cached version of xmlXPathConvertString().
 * Converts an existing object to its string() equivalent
 *
 * Returns a created or reused object, the old one is freed (cached)
 *    (or the operation is done directly on @val)
 */
static xmlXPathObject * xmlXPathCacheConvertString(xmlXPathContext * ctxt, xmlXPathObject * val)
{
	xmlChar * res = NULL;
	if(!val)
		return (xmlXPathCacheNewCString(ctxt, ""));
	switch(val->type) {
		case XPATH_UNDEFINED:
#ifdef DEBUG_EXPR
		    xmlGenericError(0, "STRING: undefined\n");
#endif
		    break;
		case XPATH_NODESET:
		case XPATH_XSLT_TREE:
		    res = xmlXPathCastNodeSetToString(val->nodesetval);
		    break;
		case XPATH_STRING:
		    return val;
		case XPATH_BOOLEAN:
		    res = xmlXPathCastBooleanToString(val->boolval);
		    break;
		case XPATH_NUMBER:
		    res = xmlXPathCastNumberToString(val->floatval);
		    break;
		case XPATH_USERS:
		case XPATH_POINT:
		case XPATH_RANGE:
		case XPATH_LOCATIONSET:
		    TODO;
		    break;
	}
	xmlXPathReleaseObject(ctxt, val);
	return res ? xmlXPathCacheWrapString(ctxt, res) : xmlXPathCacheNewCString(ctxt, "");
}
/**
 * xmlXPathCacheObjectCopy:
 * @ctxt: the XPath context
 * @val:  the original object
 *
 * This is the cached version of xmlXPathObjectCopy().
 * Acquire a copy of a given object
 *
 * Returns a created or reused created object.
 */
static xmlXPathObject * xmlXPathCacheObjectCopy(xmlXPathContext * ctxt, xmlXPathObject * val)
{
	if(!val)
		return 0;
	if(XP_HAS_CACHE(ctxt)) {
		switch(val->type) {
			case XPATH_NODESET: return (xmlXPathCacheWrapNodeSet(ctxt, xmlXPathNodeSetMerge(NULL, val->nodesetval)));
			case XPATH_STRING: return (xmlXPathCacheNewString(ctxt, val->stringval));
			case XPATH_BOOLEAN: return (xmlXPathCacheNewBoolean(ctxt, val->boolval));
			case XPATH_NUMBER: return (xmlXPathCacheNewFloat(ctxt, val->floatval));
			default: break;
		}
	}
	return xmlXPathObjectCopy(val);
}
/**
 * xmlXPathCacheConvertBoolean:
 * @ctxt: the XPath context
 * @val:  an XPath object
 *
 * This is the cached version of xmlXPathConvertBoolean().
 * Converts an existing object to its boolean() equivalent
 *
 * Returns a created or reused object, the old one is freed (or the operation
 *    is done directly on @val)
 */
static xmlXPathObject * xmlXPathCacheConvertBoolean(xmlXPathContext * ctxt, xmlXPathObject * val)
{
	xmlXPathObject * ret;
	if(!val)
		return xmlXPathCacheNewBoolean(ctxt, 0);
	if(val->type == XPATH_BOOLEAN)
		return val;
	ret = xmlXPathCacheNewBoolean(ctxt, xmlXPathCastToBoolean(val));
	xmlXPathReleaseObject(ctxt, val);
	return ret;
}

/**
 * xmlXPathCacheConvertNumber:
 * @ctxt: the XPath context
 * @val:  an XPath object
 *
 * This is the cached version of xmlXPathConvertNumber().
 * Converts an existing object to its number() equivalent
 *
 * Returns a created or reused object, the old one is freed (or the operation
 *    is done directly on @val)
 */
static xmlXPathObject * xmlXPathCacheConvertNumber(xmlXPathContext * ctxt, xmlXPathObject * val)
{
	xmlXPathObject * ret;
	if(!val)
		return xmlXPathCacheNewFloat(ctxt, 0.0);
	if(val->type == XPATH_NUMBER)
		return val;
	ret = xmlXPathCacheNewFloat(ctxt, xmlXPathCastToNumber(val));
	xmlXPathReleaseObject(ctxt, val);
	return ret;
}
// 
// Parser stacks related functions and macros
// 
/**
 * xmlXPathSetFrame:
 * @ctxt: an XPath parser context
 *
 * Set the callee evaluation frame
 *
 * Returns the previous frame value to be restored once done
 */
static int xmlXPathSetFrame(xmlXPathParserContext * ctxt) 
{
	int ret = 0;
	if(ctxt) {
		ret = ctxt->valueFrame;
		ctxt->valueFrame = ctxt->valueNr;
	}
	return ret;
}
/**
 * xmlXPathPopFrame:
 * @ctxt: an XPath parser context
 * @frame: the previous frame value
 *
 * Remove the callee evaluation frame
 */
static void xmlXPathPopFrame(xmlXPathParserContext * ctxt, int frame) 
{
	if(ctxt) {
		if(ctxt->valueNr < ctxt->valueFrame)
			xmlXPatherror(ctxt, /*__FILE__, __LINE__,*/XPATH_STACK_ERROR);
		ctxt->valueFrame = frame;
	}
}
/**
 * valuePop:
 * @ctxt: an XPath evaluation context
 *
 * Pops the top XPath object from the value stack
 *
 * Returns the XPath object just removed
 */
xmlXPathObject * FASTCALL valuePop(xmlXPathParserContext * pCtxt)
{
	xmlXPathObject * ret = 0;
	if(pCtxt && (pCtxt->valueNr > 0)) {
		if(pCtxt->valueNr <= pCtxt->valueFrame) {
			xmlXPatherror(pCtxt,/*__FILE__, __LINE__,*/XPATH_STACK_ERROR);
		}
		else {
			pCtxt->valueNr--;
			pCtxt->value = (pCtxt->valueNr > 0) ? pCtxt->valueTab[pCtxt->valueNr - 1] : NULL;
			ret = pCtxt->valueTab[pCtxt->valueNr];
			pCtxt->valueTab[pCtxt->valueNr] = NULL;
		}
	}
	return ret;
}
/**
 * valuePush:
 * @ctxt:  an XPath evaluation context
 * @value:  the XPath object
 *
 * Pushes a new XPath object on top of the value stack
 *
 * returns the number of items on the value stack
 */
int FASTCALL valuePush(xmlXPathParserContext * pCtxt, xmlXPathObject * value)
{
	if(!pCtxt || !value)
		return -1;
	else {
		if(pCtxt->valueNr >= pCtxt->valueMax) {
			if(pCtxt->valueMax >= XPATH_MAX_STACK_DEPTH) {
				xmlXPathErrMemory(NULL, "XPath stack depth limit reached");
				pCtxt->error = XPATH_MEMORY_ERROR;
				return 0;
			}
			else {
				xmlXPathObject ** tmp = (xmlXPathObject **)SAlloc::R(pCtxt->valueTab, 2 * pCtxt->valueMax * sizeof(pCtxt->valueTab[0]));
				if(!tmp) {
					xmlXPathErrMemory(NULL, "pushing value");
					pCtxt->error = XPATH_MEMORY_ERROR;
					return 0;
				}
				else {
					pCtxt->valueMax *= 2;
					pCtxt->valueTab = tmp;
				}
			}
		}
		pCtxt->valueTab[pCtxt->valueNr] = value;
		pCtxt->value = value;
		return (pCtxt->valueNr++);
	}
}
/**
 * xmlXPathPopBoolean:
 * @ctxt:  an XPath parser context
 *
 * Pops a boolean from the stack, handling conversion if needed.
 * Check error with #xmlXPathCheckError.
 *
 * Returns the boolean
 */
int xmlXPathPopBoolean(xmlXPathParserContext * ctxt)
{
	int ret = 0;
	xmlXPathObject * obj = valuePop(ctxt);
	if(obj == NULL) {
		xmlXPathSetError(ctxt, XPATH_INVALID_OPERAND);
	}
	else {
		ret = (obj->type != XPATH_BOOLEAN) ? xmlXPathCastToBoolean(obj) : obj->boolval;
		xmlXPathReleaseObject(ctxt->context, obj);
	}
	return ret;
}

/**
 * xmlXPathPopNumber:
 * @ctxt:  an XPath parser context
 *
 * Pops a number from the stack, handling conversion if needed.
 * Check error with #xmlXPathCheckError.
 *
 * Returns the number
 */
double xmlXPathPopNumber(xmlXPathParserContext * ctxt)
{
	double ret;
	xmlXPathObject * obj = valuePop(ctxt);
	if(obj == NULL) {
		xmlXPathSetError(ctxt, XPATH_INVALID_OPERAND);
		return 0;
	}
	ret = (obj->type != XPATH_NUMBER) ? xmlXPathCastToNumber(obj) : obj->floatval;
	xmlXPathReleaseObject(ctxt->context, obj);
	return ret;
}

/**
 * xmlXPathPopString:
 * @ctxt:  an XPath parser context
 *
 * Pops a string from the stack, handling conversion if needed.
 * Check error with #xmlXPathCheckError.
 *
 * Returns the string
 */
xmlChar * xmlXPathPopString(xmlXPathParserContext * ctxt)
{
	xmlChar * ret;
	xmlXPathObject * obj = valuePop(ctxt);
	if(obj == NULL) {
		xmlXPathSetError(ctxt, XPATH_INVALID_OPERAND);
		return 0;
	}
	ret = xmlXPathCastToString(obj); /* this does required strdup */
	/* @todo needs refactoring somewhere else */
	if(obj->stringval == ret)
		obj->stringval = NULL;
	xmlXPathReleaseObject(ctxt->context, obj);
	return ret;
}
/**
 * xmlXPathPopNodeSet:
 * @ctxt:  an XPath parser context
 *
 * Pops a node-set from the stack, handling conversion if needed.
 * Check error with #xmlXPathCheckError.
 *
 * Returns the node-set
 */
xmlNodeSet * xmlXPathPopNodeSet(xmlXPathParserContext * ctxt)
{
	xmlXPathObject * obj;
	xmlNodeSet * ret;
	if(!ctxt)
		return 0;
	if(ctxt->value == NULL) {
		xmlXPathSetError(ctxt, XPATH_INVALID_OPERAND);
		return 0;
	}
	if(!xmlXPathStackIsNodeSet(ctxt)) {
		xmlXPathSetTypeError(ctxt);
		return 0;
	}
	obj = valuePop(ctxt);
	ret = obj->nodesetval;
#if 0
	/* to fix memory leak of not clearing obj->user */
	if(obj->boolval && obj->user)
		xmlFreeNodeList((xmlNode *)obj->user);
#endif
	obj->nodesetval = NULL;
	xmlXPathReleaseObject(ctxt->context, obj);
	return ret;
}

/**
 * xmlXPathPopExternal:
 * @ctxt:  an XPath parser context
 *
 * Pops an external object from the stack, handling conversion if needed.
 * Check error with #xmlXPathCheckError.
 *
 * Returns the object
 */
void * xmlXPathPopExternal(xmlXPathParserContext * ctxt)
{
	xmlXPathObject * obj;
	void * ret;
	if(!ctxt || !ctxt->value) {
		xmlXPathSetError(ctxt, XPATH_INVALID_OPERAND);
		return 0;
	}
	if(ctxt->value->type != XPATH_USERS) {
		xmlXPathSetTypeError(ctxt);
		return 0;
	}
	obj = valuePop(ctxt);
	ret = obj->user;
	obj->user = NULL;
	xmlXPathReleaseObject(ctxt->context, obj);
	return ret;
}

/*
 * Macros for accessing the content. Those should be used only by the parser,
 * and not exported.
 *
 * Dirty macros, i.e. one need to make assumption on the context to use them
 *
 * CUR_PTR return the current pointer to the xmlChar to be parsed.
 * CUR     returns the current xmlChar value, i.e. a 8 bit value
 *      in ISO-Latin or UTF-8.
 *      This should be used internally by the parser
 *      only to compare to ASCII values otherwise it would break when
 *      running with UTF-8 encoding.
 * NXT(n)  returns the n'th next xmlChar. Same as CUR is should be used only
 *      to compare on ASCII based substring.
 * SKIP(n) Skip n xmlChar, and must also be used only to skip ASCII defined
 *      strings within the parser.
 * CURRENT Returns the current char value, with the full decoding of
 *      UTF-8 if we are using this mode. It returns an int.
 * NEXT    Skip to the next character, this does the proper decoding
 *      in UTF-8 mode. It also pop-up unfinished entities on the fly.
 *      It returns the pointer to the current xmlChar.
 */

#define CUR (*ctxt->cur)
#define SKIP(val) ctxt->cur += (val)
#define NXT(val) ctxt->cur[(val)]
#define CUR_PTR ctxt->cur
#define CUR_CHAR(l) xmlXPathCurrentChar(ctxt, &l)

#define COPY_BUF(l, b, i, v) if(l == 1) b[i++] = (xmlChar)v; else i += xmlCopyChar(l, &b[i], v)
#define NEXTL(l)  ctxt->cur += l
#define SKIP_BLANKS while(IS_BLANK_CH(*(ctxt->cur))) NEXT
#define CURRENT (*ctxt->cur)
#define NEXT ((*ctxt->cur) ? ctxt->cur++ : ctxt->cur)
#ifndef DBL_DIG
	#define DBL_DIG 16
#endif
#ifndef DBL_EPSILON
	#define DBL_EPSILON 1E-9
#endif
#define UPPER_DOUBLE 1E9
#define LOWER_DOUBLE 1E-5
#define LOWER_DOUBLE_EXP 5
#define INTEGER_DIGITS DBL_DIG
#define FRACTION_DIGITS (DBL_DIG + 1 + (LOWER_DOUBLE_EXP))
#define EXPONENT_DIGITS (3 + 2)

/**
 * xmlXPathFormatNumber:
 * @number:     number to format
 * @buffer:     output buffer
 * @buffersize: size of output buffer
 *
 * Convert the number into a string representation.
 */
static void xmlXPathFormatNumber(double number, char buffer[], int buffersize)
{
	switch(xmlXPathIsInf(number)) {
		case 1:
		    if(buffersize > (int)sizeof("Infinity"))
			    snprintf(buffer, buffersize, "Infinity");
		    break;
		case -1:
		    if(buffersize > (int)sizeof("-Infinity"))
			    snprintf(buffer, buffersize, "-Infinity");
		    break;
		default:
		    if(fisnan(number)) {
			    if(buffersize > (int)sizeof("NaN"))
				    snprintf(buffer, buffersize, "NaN");
		    }
		    else if(number == 0 && xmlXPathGetSign(number) != 0) {
			    snprintf(buffer, buffersize, "0");
		    }
		    else if(number == ((int)number)) {
			    char work[30];
			    char * cur;
			    int value = (int)number;
			    char * ptr = &buffer[0];
			    if(value == 0) {
				    *ptr++ = '0';
			    }
			    else {
				    snprintf(work, 29, "%d", value);
				    cur = &work[0];
				    while((*cur) && (ptr - buffer < buffersize)) {
					    *ptr++ = *cur++;
				    }
			    }
			    if(ptr - buffer < buffersize) {
				    *ptr = 0;
			    }
			    else if(buffersize > 0) {
				    ptr--;
				    *ptr = 0;
			    }
		    }
		    else {
			    /*
			       For the dimension of work,
			          DBL_DIG is number of significant digits
			          EXPONENT is only needed for "scientific notation"
			          3 is sign, decimal point, and terminating zero
			          LOWER_DOUBLE_EXP is max number of leading zeroes in fraction
			       Note that this dimension is slightly (a few characters)
			       larger than actually necessary.
			     */
			    char work[DBL_DIG + EXPONENT_DIGITS + 3 + LOWER_DOUBLE_EXP];
			    int integer_place, fraction_place;
			    char * ptr;
			    char * after_fraction;
			    int size;
			    double absolute_value = fabs(number);
			    /*
			 * First choose format - scientific or regular floating point.
			 * In either case, result is in work, and after_fraction points
			 * just past the fractional part.
			     */
			    if(((absolute_value > UPPER_DOUBLE) || (absolute_value < LOWER_DOUBLE)) && (absolute_value != 0.0)) {
				    /* Use scientific notation */
				    integer_place = DBL_DIG + EXPONENT_DIGITS + 1;
				    fraction_place = DBL_DIG - 1;
				    size = snprintf(work, sizeof(work), "%*.*e",
				    integer_place, fraction_place, number);
				    while((size > 0) && (work[size] != 'e')) size--;
			    }
			    else {
				    /* Use regular notation */
				    if(absolute_value > 0.0) {
					    integer_place = (int)log10(absolute_value);
					    if(integer_place > 0)
						    fraction_place = DBL_DIG - integer_place - 1;
					    else
						    fraction_place = DBL_DIG - integer_place;
				    }
				    else {
					    fraction_place = 1;
				    }
				    size = snprintf(work, sizeof(work), "%0.*f",
				    fraction_place, number);
			    }
			    /* Remove leading spaces sometimes inserted by snprintf */
			    while(work[0] == ' ') {
				    for(ptr = &work[0]; (ptr[0] = ptr[1]); ptr++)
						;
				    size--;
			    }
			    /* Remove fractional trailing zeroes */
			    after_fraction = work + size;
			    ptr = after_fraction;
			    while(*(--ptr) == '0')
				    ;
			    if(*ptr != '.')
				    ptr++;
			    while((*ptr++ = *after_fraction++) != 0) 
					;
			    // Finally copy result back to caller 
			    size = sstrlen(work) + 1;
			    if(size > buffersize) {
				    work[buffersize - 1] = 0;
				    size = buffersize;
			    }
			    memmove(buffer, work, size);
		    }
		    break;
	}
}
//
// Routines to handle NodeSets
//
/**
 * xmlXPathOrderDocElems:
 * @doc:  an input document
 *
 * Call this routine to speed up XPath computation on static documents.
 * This stamps all the element nodes with the document order
 * Like for line information, the order is kept in the element->content
 * field, the value stored is actually - the node number (starting at -1)
 * to be able to differentiate from line numbers.
 *
 * Returns the number of elements found in the document or -1 in case
 *  of error.
 */
long xmlXPathOrderDocElems(xmlDoc * doc)
{
	long count = 0;
	xmlNode * cur;
	if(!doc)
		return -1;
	cur = doc->children;
	while(cur) {
		if(cur->type == XML_ELEMENT_NODE) {
			cur->content = (xmlChar *)(-(++count));
			if(cur->children) {
				cur = cur->children;
				continue;
			}
		}
		if(cur->next) {
			cur = cur->next;
			continue;
		}
		do {
			cur = cur->P_ParentNode;
			if(!cur)
				break;
			if(cur == (xmlNode *)doc) {
				cur = NULL;
				break;
			}
			if(cur->next) {
				cur = cur->next;
				break;
			}
		} while(cur);
	}
	return (count);
}
/**
 * xmlXPathCmpNodes:
 * @node1:  the first node
 * @node2:  the second node
 *
 * Compare two nodes w.r.t document order
 *
 * Returns -2 in case of error 1 if first point < second point, 0 if
 *    it's the same node, -1 otherwise
 */
int xmlXPathCmpNodes(xmlNode * node1, xmlNode * node2) 
{
	int depth1, depth2;
	int attr1 = 0, attr2 = 0;
	xmlNode * attrNode1 = NULL;
	xmlNode * attrNode2 = NULL;
	xmlNode * cur;
	xmlNode * root;
	if((node1 == NULL) || (node2 == NULL))
		return -2;
	/*
	 * a couple of optimizations which will avoid computations in most cases
	 */
	if(node1 == node2)      /* trivial case */
		return 0;
	if(node1->type == XML_ATTRIBUTE_NODE) {
		attr1 = 1;
		attrNode1 = node1;
		node1 = node1->P_ParentNode;
	}
	if(node2->type == XML_ATTRIBUTE_NODE) {
		attr2 = 1;
		attrNode2 = node2;
		node2 = node2->P_ParentNode;
	}
	if(node1 == node2) {
		if(attr1 == attr2) {
			/* not required, but we keep attributes in order */
			if(attr1 != 0) {
				cur = attrNode2->prev;
				while(cur) {
					if(cur == attrNode1)
						return 1;
					cur = cur->prev;
				}
				return -1;
			}
			return 0;
		}
		if(attr2 == 1)
			return 1;
		return -1;
	}
	if(node1->type == XML_NAMESPACE_DECL || node2->type == XML_NAMESPACE_DECL)
		return 1;
	if(node1 == node2->prev)
		return 1;
	if(node1 == node2->next)
		return -1;
	/*
	 * Speedup using document order if availble.
	 */
	if((node1->type == XML_ELEMENT_NODE) && (node2->type == XML_ELEMENT_NODE) && (0 > (long)node1->content) && (0 > (long)node2->content) && (node1->doc == node2->doc)) {
		long l1 = -((long)node1->content);
		long l2 = -((long)node2->content);
		if(l1 < l2)
			return 1;
		if(l1 > l2)
			return -1;
	}
	/*
	 * compute depth to root
	 */
	for(depth2 = 0, cur = node2; cur->P_ParentNode; cur = cur->P_ParentNode) {
		if(cur == node1)
			return 1;
		depth2++;
	}
	root = cur;
	for(depth1 = 0, cur = node1; cur->P_ParentNode; cur = cur->P_ParentNode) {
		if(cur == node2)
			return -1;
		depth1++;
	}
	/*
	 * Distinct document (or distinct entities :-( ) case.
	 */
	if(root != cur) {
		return -2;
	}
	/*
	 * get the nearest common ancestor.
	 */
	while(depth1 > depth2) {
		depth1--;
		node1 = node1->P_ParentNode;
	}
	while(depth2 > depth1) {
		depth2--;
		node2 = node2->P_ParentNode;
	}
	while(node1->P_ParentNode != node2->P_ParentNode) {
		node1 = node1->P_ParentNode;
		node2 = node2->P_ParentNode;
		/* should not happen but just in case ... */
		if((node1 == NULL) || (node2 == NULL))
			return -2;
	}
	/*
	 * Find who's first.
	 */
	if(node1 == node2->prev)
		return 1;
	if(node1 == node2->next)
		return -1;
	/*
	 * Speedup using document order if availble.
	 */
	if((node1->type == XML_ELEMENT_NODE) && (node2->type == XML_ELEMENT_NODE) && (0 > (long)node1->content) && (0 > (long)node2->content) && (node1->doc == node2->doc)) {
		long l1, l2;
		l1 = -((long)node1->content);
		l2 = -((long)node2->content);
		if(l1 < l2)
			return 1;
		if(l1 > l2)
			return -1;
	}
	for(cur = node1->next; cur; cur = cur->next)
		if(cur == node2)
			return 1;
	return -1; /* assume there is no sibling list corruption */
}

/**
 * xmlXPathNodeSetSort:
 * @set:  the node set
 *
 * Sort the node set in document order
 */
void xmlXPathNodeSetSort(xmlNodeSet * set) {
#ifndef WITH_TIM_SORT
	int i, j, incr, len;
	xmlNode * tmp;
#endif
	if(set == NULL)
		return;

#ifndef WITH_TIM_SORT
	/*
	 * Use the old Shell's sort implementation to sort the node-set
	 * Timsort ought to be quite faster
	 */
	len = set->nodeNr;
	for(incr = len / 2; incr > 0; incr /= 2) {
		for(i = incr; i < len; i++) {
			j = i - incr;
			while(j >= 0) {
#ifdef XP_OPTIMIZED_NON_ELEM_COMPARISON
				if(xmlXPathCmpNodesExt(set->nodeTab[j],
					    set->nodeTab[j + incr]) == -1)
#else
				if(xmlXPathCmpNodes(set->nodeTab[j],
					    set->nodeTab[j + incr]) == -1)
#endif
				{
					tmp = set->nodeTab[j];
					set->nodeTab[j] = set->nodeTab[j + incr];
					set->nodeTab[j + incr] = tmp;
					j -= incr;
				}
				else
					break;
			}
		}
	}
#else /* WITH_TIM_SORT */
	libxml_domnode_tim_sort(set->PP_NodeTab, set->nodeNr);
#endif /* WITH_TIM_SORT */
}

#define XML_NODESET_DEFAULT     10
/**
 * xmlXPathNodeSetDupNs:
 * @node:  the parent node of the namespace XPath node
 * @ns:  the libxml namespace declaration node.
 *
 * Namespace node in libxml don't match the XPath semantic. In a node set
 * the namespace nodes are duplicated and the next pointer is set to the
 * parent node in the XPath semantic.
 *
 * Returns the newly created object.
 */
static xmlNode * xmlXPathNodeSetDupNs(xmlNode * pNode, xmlNs * ns)
{
	if(!ns || ns->type != XML_NAMESPACE_DECL)
		return 0;
	else if(!pNode || (pNode->type == XML_NAMESPACE_DECL))
		return reinterpret_cast<xmlNode *>(ns);
	else {
		// Allocate a new Namespace and fill the fields.
		xmlNs * cur = static_cast<xmlNs *>(SAlloc::M(sizeof(xmlNs)));
		if(!cur) {
			xmlXPathErrMemory(NULL, "duplicating namespace");
		}
		else {
			memzero(cur, sizeof(xmlNs));
			cur->type = XML_NAMESPACE_DECL;
			if(ns->href)
				cur->href = sstrdup(ns->href);
			if(ns->prefix)
				cur->prefix = sstrdup(ns->prefix);
			cur->next = reinterpret_cast<xmlNs *>(pNode);
		}
		return reinterpret_cast<xmlNode *>(cur);
	}
}
/**
 * xmlXPathNodeSetFreeNs:
 * @ns:  the XPath namespace node found in a nodeset.
 *
 * Namespace nodes in libxml don't match the XPath semantic. In a node set
 * the namespace nodes are duplicated and the next pointer is set to the
 * parent node in the XPath semantic. Check if such a node needs to be freed
 */
void FASTCALL xmlXPathNodeSetFreeNs(xmlNs * ns)
{
	if(ns && ns->type == XML_NAMESPACE_DECL) {
		if(ns->next && (ns->next->type != XML_NAMESPACE_DECL)) {
			SAlloc::F((xmlChar *)ns->href);
			SAlloc::F((xmlChar *)ns->prefix);
			SAlloc::F(ns);
		}
	}
}
/**
 * xmlXPathNodeSetCreate:
 * @val:  an initial xmlNodePtr, or NULL
 *
 * Create a new xmlNodeSetPtr of type double and of value @val
 *
 * Returns the newly created object.
 */
xmlNodeSet * xmlXPathNodeSetCreate(xmlNode * val)
{
	xmlNodeSet * ret = static_cast<xmlNodeSet *>(SAlloc::M(sizeof(xmlNodeSet)));
	if(!ret) {
		xmlXPathErrMemory(NULL, "creating nodeset");
		return 0;
	}
	memzero(ret, sizeof(xmlNodeSet));
	if(val) {
		ret->PP_NodeTab = static_cast<xmlNode **>(SAlloc::M(XML_NODESET_DEFAULT * sizeof(xmlNode *)));
		if(ret->PP_NodeTab == NULL) {
			xmlXPathErrMemory(NULL, "creating nodeset");
			SAlloc::F(ret);
			return 0;
		}
		memzero(ret->PP_NodeTab, XML_NODESET_DEFAULT * sizeof(xmlNode *));
		ret->nodeMax = XML_NODESET_DEFAULT;
		if(val->type == XML_NAMESPACE_DECL) {
			xmlNs * ns = reinterpret_cast<xmlNs *>(val);
			ret->PP_NodeTab[ret->nodeNr++] = xmlXPathNodeSetDupNs(reinterpret_cast<xmlNode *>(ns->next), ns);
		}
		else
			ret->PP_NodeTab[ret->nodeNr++] = val;
	}
	return ret;
}
/**
 * xmlXPathNodeSetCreateSize:
 * @size:  the initial size of the set
 *
 * Create a new xmlNodeSetPtr of type double and of value @val
 *
 * Returns the newly created object.
 */
static xmlNodeSet * xmlXPathNodeSetCreateSize(int size)
{
	xmlNodeSet * ret = static_cast<xmlNodeSet *>(SAlloc::M(sizeof(xmlNodeSet)));
	if(!ret) {
		xmlXPathErrMemory(NULL, "creating nodeset");
		return 0;
	}
	memzero(ret, sizeof(xmlNodeSet));
	if(size < XML_NODESET_DEFAULT)
		size = XML_NODESET_DEFAULT;
	ret->PP_NodeTab = static_cast<xmlNode **>(SAlloc::M(size * sizeof(xmlNode *)));
	if(ret->PP_NodeTab == NULL) {
		xmlXPathErrMemory(NULL, "creating nodeset");
		SAlloc::F(ret);
		return 0;
	}
	memzero(ret->PP_NodeTab, size * sizeof(xmlNode *));
	ret->nodeMax = size;
	return ret;
}
/**
 * xmlXPathNodeSetContains:
 * @cur:  the node-set
 * @val:  the node
 *
 * checks whether @cur contains @val
 *
 * Returns true (1) if @cur contains @val, false (0) otherwise
 */
int xmlXPathNodeSetContains(const xmlNodeSet * cur, const xmlNode * val)
{
	if(cur && val) {
		if(val->type == XML_NAMESPACE_DECL) {
			for(int i = 0; i < cur->nodeNr; i++) {
				if(cur->PP_NodeTab[i]->type == XML_NAMESPACE_DECL) {
					const xmlNs * ns1 = reinterpret_cast<const xmlNs *>(val);
					const xmlNs * ns2 = reinterpret_cast<const xmlNs *>(cur->PP_NodeTab[i]);
					if(ns1 == ns2)
						return 1;
					else if(ns1->next && (ns2->next == ns1->next) && (sstreq(ns1->prefix, ns2->prefix)))
						return 1;
				}
			}
		}
		else {
			for(int i = 0; i < cur->nodeNr; i++) {
				if(cur->PP_NodeTab[i] == val)
					return 1;
			}
		}
	}
	return 0;
}

/**
 * xmlXPathNodeSetAddNs:
 * @cur:  the initial node set
 * @node:  the hosting node
 * @ns:  a the namespace node
 *
 * add a new namespace node to an existing NodeSet
 *
 * Returns 0 in case of success and -1 in case of error
 */
int xmlXPathNodeSetAddNs(xmlNodeSet * cur, xmlNode * pNode, xmlNs * ns) 
{
	int i;
	if(!cur || !ns || !pNode || (ns->type != XML_NAMESPACE_DECL) || (pNode->type != XML_ELEMENT_NODE))
		return -1;
	/* @@ with_ns to check whether namespace nodes should be looked at @@ */
	/*
	 * prevent duplicates
	 */
	for(i = 0; i < cur->nodeNr; i++) {
		if(cur->PP_NodeTab[i] && (cur->PP_NodeTab[i]->type == XML_NAMESPACE_DECL) && (((xmlNs *)cur->PP_NodeTab[i])->next == (xmlNs *)pNode) && (sstreq(ns->prefix, ((xmlNs *)cur->PP_NodeTab[i])->prefix)))
			return 0;
	}
	/*
	 * grow the nodeTab if needed
	 */
	if(cur->nodeMax == 0) {
		cur->PP_NodeTab = static_cast<xmlNode **>(SAlloc::M(XML_NODESET_DEFAULT * sizeof(xmlNode *)));
		if(cur->PP_NodeTab == NULL) {
			xmlXPathErrMemory(NULL, "growing nodeset");
			return -1;
		}
		memzero(cur->PP_NodeTab, XML_NODESET_DEFAULT * sizeof(xmlNode *));
		cur->nodeMax = XML_NODESET_DEFAULT;
	}
	else if(cur->nodeNr == cur->nodeMax) {
		xmlNode ** temp;
		if(cur->nodeMax >= XPATH_MAX_NODESET_LENGTH) {
			xmlXPathErrMemory(NULL, "growing nodeset hit limit");
			return -1;
		}
		temp = static_cast<xmlNode **>(SAlloc::R(cur->PP_NodeTab, cur->nodeMax * 2 * sizeof(xmlNode *)));
		if(temp == NULL) {
			xmlXPathErrMemory(NULL, "growing nodeset");
			return -1;
		}
		cur->nodeMax *= 2;
		cur->PP_NodeTab = temp;
	}
	cur->PP_NodeTab[cur->nodeNr++] = xmlXPathNodeSetDupNs(pNode, ns);
	return 0;
}

/**
 * xmlXPathNodeSetAdd:
 * @cur:  the initial node set
 * @val:  a new xmlNodePtr
 *
 * add a new xmlNode * to an existing NodeSet
 *
 * Returns 0 in case of success, and -1 in case of error
 */
int xmlXPathNodeSetAdd(xmlNodeSet * cur, xmlNode * val) 
{
	int i;
	if(!cur || (val == NULL)) return -1;

	/* @@ with_ns to check whether namespace nodes should be looked at @@ */
	/*
	 * prevent duplcates
	 */
	for(i = 0; i < cur->nodeNr; i++)
		if(cur->PP_NodeTab[i] == val) return 0;

	/*
	 * grow the nodeTab if needed
	 */
	if(cur->nodeMax == 0) {
		cur->PP_NodeTab = static_cast<xmlNode **>(SAlloc::M(XML_NODESET_DEFAULT * sizeof(xmlNode *)));
		if(cur->PP_NodeTab == NULL) {
			xmlXPathErrMemory(NULL, "growing nodeset");
			return -1;
		}
		memzero(cur->PP_NodeTab, XML_NODESET_DEFAULT * sizeof(xmlNode *));
		cur->nodeMax = XML_NODESET_DEFAULT;
	}
	else if(cur->nodeNr == cur->nodeMax) {
		if(cur->nodeMax >= XPATH_MAX_NODESET_LENGTH) {
			xmlXPathErrMemory(NULL, "growing nodeset hit limit");
			return -1;
		}
		else {
			xmlNode ** temp = static_cast<xmlNode **>(SAlloc::R(cur->PP_NodeTab, cur->nodeMax * 2 * sizeof(xmlNode *)));
			if(temp == NULL) {
				xmlXPathErrMemory(NULL, "growing nodeset");
				return -1;
			}
			cur->nodeMax *= 2;
			cur->PP_NodeTab = temp;
		}
	}
	if(val->type == XML_NAMESPACE_DECL) {
		xmlNs * ns = reinterpret_cast<xmlNs *>(val);
		cur->PP_NodeTab[cur->nodeNr++] =  xmlXPathNodeSetDupNs((xmlNode *)ns->next, ns);
	}
	else
		cur->PP_NodeTab[cur->nodeNr++] = val;
	return 0;
}

/**
 * xmlXPathNodeSetAddUnique:
 * @cur:  the initial node set
 * @val:  a new xmlNodePtr
 *
 * add a new xmlNode * to an existing NodeSet, optimized version
 * when we are sure the node is not already in the set.
 *
 * Returns 0 in case of success and -1 in case of failure
 */
int xmlXPathNodeSetAddUnique(xmlNodeSet * cur, xmlNode * val) 
{
	if(!cur || (val == NULL)) 
		return -1;
	/* @@ with_ns to check whether namespace nodes should be looked at @@ */
	/*
	 * grow the nodeTab if needed
	 */
	if(cur->nodeMax == 0) {
		cur->PP_NodeTab = static_cast<xmlNode **>(SAlloc::M(XML_NODESET_DEFAULT * sizeof(xmlNode *)));
		if(cur->PP_NodeTab == NULL) {
			xmlXPathErrMemory(NULL, "growing nodeset");
			return -1;
		}
		memzero(cur->PP_NodeTab, XML_NODESET_DEFAULT * sizeof(xmlNode *));
		cur->nodeMax = XML_NODESET_DEFAULT;
	}
	else if(cur->nodeNr == cur->nodeMax) {
		if(cur->nodeMax >= XPATH_MAX_NODESET_LENGTH) {
			xmlXPathErrMemory(NULL, "growing nodeset hit limit");
			return -1;
		}
		else {
			xmlNode ** temp = static_cast<xmlNode **>(SAlloc::R(cur->PP_NodeTab, cur->nodeMax * 2 * sizeof(xmlNode *)));
			if(temp == NULL) {
				xmlXPathErrMemory(NULL, "growing nodeset");
				return -1;
			}
			else {
				cur->PP_NodeTab = temp;
				cur->nodeMax *= 2;
			}
		}
	}
	if(val->type == XML_NAMESPACE_DECL) {
		xmlNs * ns = reinterpret_cast<xmlNs *>(val);
		cur->PP_NodeTab[cur->nodeNr++] = xmlXPathNodeSetDupNs((xmlNode *)ns->next, ns);
	}
	else
		cur->PP_NodeTab[cur->nodeNr++] = val;
	return 0;
}
/**
 * xmlXPathNodeSetMerge:
 * @val1:  the first NodeSet or NULL
 * @val2:  the second NodeSet
 *
 * Merges two nodesets, all nodes from @val2 are added to @val1
 * if @val1 is NULL, a new set is created and copied from @val2
 *
 * Returns @val1 once extended or NULL in case of error.
 */
xmlNodeSet * xmlXPathNodeSetMerge(xmlNodeSet * val1, xmlNodeSet * val2) 
{
	int i, j, initNr, skip;
	xmlNode * n1;
	xmlNode * n2;
	if(val2 == NULL) 
		return (val1);
	if(val1 == NULL) {
		val1 = xmlXPathNodeSetCreate(NULL);
		if(val1 == NULL)
			return 0;
#if 0
		/*
		 * @todo The optimization won't work in every case, since
		 *  those nasty namespace nodes need to be added with
		 *  xmlXPathNodeSetDupNs() to the set; thus a pure
		 *  memcpy is not possible.
		 *  If there was a flag on the nodesetval, indicating that
		 *  some temporary nodes are in, that would be helpfull.
		 */
		/*
		 * Optimization: Create an equally sized node-set
		 * and memcpy the content.
		 */
		val1 = xmlXPathNodeSetCreateSize(val2->nodeNr);
		if(val1 == NULL)
			return 0;
		if(val2->nodeNr != 0) {
			if(val2->nodeNr == 1)
				*(val1->nodeTab) = *(val2->nodeTab);
			else {
				memcpy(val1->nodeTab, val2->nodeTab, val2->nodeNr * sizeof(xmlNode *));
			}
			val1->nodeNr = val2->nodeNr;
		}
		return (val1);
#endif
	}
	/* @@ with_ns to check whether namespace nodes should be looked at @@ */
	initNr = val1->nodeNr;
	for(i = 0; i < val2->nodeNr; i++) {
		n2 = val2->PP_NodeTab[i];
		/*
		 * check against duplicates
		 */
		skip = 0;
		for(j = 0; j < initNr; j++) {
			n1 = val1->PP_NodeTab[j];
			if(n1 == n2) {
				skip = 1;
				break;
			}
			else if((n1->type == XML_NAMESPACE_DECL) && (n2->type == XML_NAMESPACE_DECL)) {
				if((((xmlNs *)n1)->next == ((xmlNs *)n2)->next) && (sstreq(((xmlNs *)n1)->prefix, ((xmlNs *)n2)->prefix))) {
					skip = 1;
					break;
				}
			}
		}
		if(skip)
			continue;
		/*
		 * grow the nodeTab if needed
		 */
		if(val1->nodeMax == 0) {
			val1->PP_NodeTab = static_cast<xmlNode **>(SAlloc::M(XML_NODESET_DEFAULT * sizeof(xmlNode *)));
			if(val1->PP_NodeTab == NULL) {
				xmlXPathErrMemory(NULL, "merging nodeset");
				return 0;
			}
			memzero(val1->PP_NodeTab, XML_NODESET_DEFAULT * sizeof(xmlNode *));
			val1->nodeMax = XML_NODESET_DEFAULT;
		}
		else if(val1->nodeNr == val1->nodeMax) {
			xmlNode ** temp;
			if(val1->nodeMax >= XPATH_MAX_NODESET_LENGTH) {
				xmlXPathErrMemory(NULL, "merging nodeset hit limit");
				return 0;
			}
			temp = static_cast<xmlNode **>(SAlloc::R(val1->PP_NodeTab, val1->nodeMax * 2 * sizeof(xmlNode *)));
			if(temp == NULL) {
				xmlXPathErrMemory(NULL, "merging nodeset");
				return 0;
			}
			val1->PP_NodeTab = temp;
			val1->nodeMax *= 2;
		}
		if(n2->type == XML_NAMESPACE_DECL) {
			xmlNs * ns = (xmlNs *)n2;
			val1->PP_NodeTab[val1->nodeNr++] = xmlXPathNodeSetDupNs((xmlNode *)ns->next, ns);
		}
		else
			val1->PP_NodeTab[val1->nodeNr++] = n2;
	}
	return (val1);
}

/**
 * xmlXPathNodeSetMergeAndClear:
 * @set1:  the first NodeSet or NULL
 * @set2:  the second NodeSet
 * @hasSet2NsNodes: 1 if set2 contains namespaces nodes
 *
 * Merges two nodesets, all nodes from @set2 are added to @set1
 * if @set1 is NULL, a new set is created and copied from @set2.
 * Checks for duplicate nodes. Clears set2.
 *
 * Returns @set1 once extended or NULL in case of error.
 */
static xmlNodeSet * xmlXPathNodeSetMergeAndClear(xmlNodeSet * set1, xmlNodeSet * set2, int hasNullEntries)
{
	if(!set1 && !hasNullEntries) {
		/*
		 * Note that doing a memcpy of the list, namespace nodes are
		 * just assigned to set1, since set2 is cleared anyway.
		 */
		set1 = xmlXPathNodeSetCreateSize(set2->nodeNr);
		if(set1 == NULL)
			return 0;
		if(set2->nodeNr != 0) {
			memcpy(set1->PP_NodeTab, set2->PP_NodeTab, set2->nodeNr * sizeof(xmlNode *));
			set1->nodeNr = set2->nodeNr;
		}
	}
	else {
		int i, j, initNbSet1;
		xmlNode * n1;
		xmlNode * n2;
		if(set1 == NULL)
			set1 = xmlXPathNodeSetCreate(NULL);
		if(set1 == NULL)
			return 0;
		initNbSet1 = set1->nodeNr;
		for(i = 0; i < set2->nodeNr; i++) {
			n2 = set2->PP_NodeTab[i];
			/*
			 * Skip NULLed entries.
			 */
			if(n2 == NULL)
				continue;
			/*
			 * Skip duplicates.
			 */
			for(j = 0; j < initNbSet1; j++) {
				n1 = set1->PP_NodeTab[j];
				if(n1 == n2) {
					goto skip_node;
				}
				else if((n1->type == XML_NAMESPACE_DECL) && (n2->type == XML_NAMESPACE_DECL)) {
					if((((xmlNs *)n1)->next == ((xmlNs *)n2)->next) && (sstreq(((xmlNs *)n1)->prefix, ((xmlNs *)n2)->prefix))) {
						/*
						 * Free the namespace node.
						 */
						set2->PP_NodeTab[i] = NULL;
						xmlXPathNodeSetFreeNs((xmlNs *)n2);
						goto skip_node;
					}
				}
			}
			/*
			 * grow the nodeTab if needed
			 */
			if(set1->nodeMax == 0) {
				set1->PP_NodeTab = static_cast<xmlNode **>(SAlloc::M(XML_NODESET_DEFAULT * sizeof(xmlNode *)));
				if(set1->PP_NodeTab == NULL) {
					xmlXPathErrMemory(NULL, "merging nodeset");
					return 0;
				}
				memzero(set1->PP_NodeTab, XML_NODESET_DEFAULT * sizeof(xmlNode *));
				set1->nodeMax = XML_NODESET_DEFAULT;
			}
			else if(set1->nodeNr >= set1->nodeMax) {
				xmlNode ** temp;
				if(set1->nodeMax >= XPATH_MAX_NODESET_LENGTH) {
					xmlXPathErrMemory(NULL, "merging nodeset hit limit");
					return 0;
				}
				temp = static_cast<xmlNode **>(SAlloc::R(set1->PP_NodeTab, set1->nodeMax * 2 * sizeof(xmlNode *)));
				if(temp == NULL) {
					xmlXPathErrMemory(NULL, "merging nodeset");
					return 0;
				}
				set1->PP_NodeTab = temp;
				set1->nodeMax *= 2;
			}
			if(n2->type == XML_NAMESPACE_DECL) {
				xmlNs * ns = (xmlNs *)n2;
				set1->PP_NodeTab[set1->nodeNr++] = xmlXPathNodeSetDupNs((xmlNode *)ns->next, ns);
			}
			else
				set1->PP_NodeTab[set1->nodeNr++] = n2;
skip_node:
			{}
		}
	}
	set2->nodeNr = 0;
	return (set1);
}

/**
 * xmlXPathNodeSetMergeAndClearNoDupls:
 * @set1:  the first NodeSet or NULL
 * @set2:  the second NodeSet
 * @hasSet2NsNodes: 1 if set2 contains namespaces nodes
 *
 * Merges two nodesets, all nodes from @set2 are added to @set1
 * if @set1 is NULL, a new set is created and copied from @set2.
 * Doesn't chack for duplicate nodes. Clears set2.
 *
 * Returns @set1 once extended or NULL in case of error.
 */
static xmlNodeSet * xmlXPathNodeSetMergeAndClearNoDupls(xmlNodeSet * set1, xmlNodeSet * set2, int hasNullEntries)
{
	if(set2 == NULL)
		return (set1);
	if(!set1 && (hasNullEntries == 0)) {
		/*
		 * Note that doing a memcpy of the list, namespace nodes are
		 * just assigned to set1, since set2 is cleared anyway.
		 */
		set1 = xmlXPathNodeSetCreateSize(set2->nodeNr);
		if(set1 == NULL)
			return 0;
		if(set2->nodeNr != 0) {
			memcpy(set1->PP_NodeTab, set2->PP_NodeTab, set2->nodeNr * sizeof(xmlNode *));
			set1->nodeNr = set2->nodeNr;
		}
	}
	else {
		int i;
		xmlNode * n2;
		SETIFZ(set1, xmlXPathNodeSetCreate(NULL));
		if(set1 == NULL)
			return 0;
		for(i = 0; i < set2->nodeNr; i++) {
			n2 = set2->PP_NodeTab[i];
			/*
			 * Skip NULLed entries.
			 */
			if(n2 == NULL)
				continue;
			if(set1->nodeMax == 0) {
				set1->PP_NodeTab = static_cast<xmlNode **>(SAlloc::M(XML_NODESET_DEFAULT * sizeof(xmlNode *)));
				if(set1->PP_NodeTab == NULL) {
					xmlXPathErrMemory(NULL, "merging nodeset");
					return 0;
				}
				memzero(set1->PP_NodeTab, XML_NODESET_DEFAULT * sizeof(xmlNode *));
				set1->nodeMax = XML_NODESET_DEFAULT;
			}
			else if(set1->nodeNr >= set1->nodeMax) {
				xmlNode ** temp;
				if(set1->nodeMax >= XPATH_MAX_NODESET_LENGTH) {
					xmlXPathErrMemory(NULL, "merging nodeset hit limit");
					return 0;
				}
				temp = static_cast<xmlNode **>(SAlloc::R(set1->PP_NodeTab, set1->nodeMax * 2 * sizeof(xmlNode *)));
				if(temp == NULL) {
					xmlXPathErrMemory(NULL, "merging nodeset");
					return 0;
				}
				set1->PP_NodeTab = temp;
				set1->nodeMax *= 2;
			}
			set1->PP_NodeTab[set1->nodeNr++] = n2;
		}
	}
	set2->nodeNr = 0;
	return (set1);
}

/**
 * xmlXPathNodeSetDel:
 * @cur:  the initial node set
 * @val:  an xmlNodePtr
 *
 * Removes an xmlNode * from an existing NodeSet
 */
void xmlXPathNodeSetDel(xmlNodeSet * cur, xmlNode * val) 
{
	int i;
	if(!cur || !val) 
		return;
	// find node in nodeTab
	for(i = 0; i < cur->nodeNr; i++)
		if(cur->PP_NodeTab[i] == val) 
			break;
	if(i >= cur->nodeNr) {  /* not found */
#ifndef NDEBUG
		xmlGenericError(0, "xmlXPathNodeSetDel: Node %s wasn't found in NodeList\n", val->name);
#endif
		return;
	}
	if(cur->PP_NodeTab[i] && (cur->PP_NodeTab[i]->type == XML_NAMESPACE_DECL))
		xmlXPathNodeSetFreeNs((xmlNs *)cur->PP_NodeTab[i]);
	cur->nodeNr--;
	for(; i < cur->nodeNr; i++)
		cur->PP_NodeTab[i] = cur->PP_NodeTab[i+1];
	cur->PP_NodeTab[cur->nodeNr] = NULL;
}

/**
 * xmlXPathNodeSetRemove:
 * @cur:  the initial node set
 * @val:  the index to remove
 *
 * Removes an entry from an existing NodeSet list.
 */
void xmlXPathNodeSetRemove(xmlNodeSet * cur, int val)
{
	if(cur && val < cur->nodeNr) {
		if(cur->PP_NodeTab[val] && (cur->PP_NodeTab[val]->type == XML_NAMESPACE_DECL))
			xmlXPathNodeSetFreeNs((xmlNs *)cur->PP_NodeTab[val]);
		cur->nodeNr--;
		for(; val < cur->nodeNr; val++)
			cur->PP_NodeTab[val] = cur->PP_NodeTab[val + 1];
		cur->PP_NodeTab[cur->nodeNr] = NULL;
	}
}
/**
 * xmlXPathFreeNodeSet:
 * @obj:  the xmlNodeSetPtr to free
 *
 * Free the NodeSet compound (not the actual nodes !).
 */
void xmlXPathFreeNodeSet(xmlNodeSet * obj)
{
	if(obj) {
		if(obj->PP_NodeTab) {
			/* @@ with_ns to check whether namespace nodes should be looked at @@ */
			for(int i = 0; i < obj->nodeNr; i++)
				if(obj->PP_NodeTab[i] && (obj->PP_NodeTab[i]->type == XML_NAMESPACE_DECL))
					xmlXPathNodeSetFreeNs((xmlNs *)obj->PP_NodeTab[i]);
			SAlloc::F(obj->PP_NodeTab);
		}
		SAlloc::F(obj);
	}
}

/**
 * xmlXPathNodeSetClear:
 * @set:  the node set to clear
 *
 * Clears the list from all temporary XPath objects (e.g. namespace nodes
 * are feed), but does *not* free the list itself. Sets the length of the
 * list to 0.
 */
static void xmlXPathNodeSetClear(xmlNodeSet * set, int hasNsNodes)
{
	if(set && set->nodeNr > 0) {
		if(hasNsNodes) {
			for(int i = 0; i < set->nodeNr; i++) {
				xmlNode * p_node = set->PP_NodeTab[i];
				if(p_node && p_node->type == XML_NAMESPACE_DECL)
					xmlXPathNodeSetFreeNs((xmlNs *)p_node);
			}
		}
		set->nodeNr = 0;
	}
}

/**
 * xmlXPathNodeSetClearFromPos:
 * @set: the node set to be cleared
 * @pos: the start position to clear from
 *
 * Clears the list from temporary XPath objects (e.g. namespace nodes
 * are feed) starting with the entry at @pos, but does *not* free the list
 * itself. Sets the length of the list to @pos.
 */
static void xmlXPathNodeSetClearFromPos(xmlNodeSet * set, int pos, int hasNsNodes)
{
	if(set && set->nodeNr > 0 && pos < set->nodeNr) {
		if(hasNsNodes) {
			for(int i = pos; i < set->nodeNr; i++) {
				xmlNode * p_node = set->PP_NodeTab[i];
				if(p_node && p_node->type == XML_NAMESPACE_DECL)
					xmlXPathNodeSetFreeNs((xmlNs *)p_node);
			}
		}
		set->nodeNr = pos;
	}
}
/**
 * xmlXPathFreeValueTree:
 * @obj:  the xmlNodeSetPtr to free
 *
 * Free the NodeSet compound and the actual tree, this is different
 * from xmlXPathFreeNodeSet()
 */
static void xmlXPathFreeValueTree(xmlNodeSet * obj)
{
	if(obj) {
		if(obj->PP_NodeTab) {
			for(int i = 0; i < obj->nodeNr; i++) {
				if(obj->PP_NodeTab[i]) {
					if(obj->PP_NodeTab[i]->type == XML_NAMESPACE_DECL) {
						xmlXPathNodeSetFreeNs((xmlNs *)obj->PP_NodeTab[i]);
					}
					else {
						xmlFreeNodeList(obj->PP_NodeTab[i]);
					}
				}
			}
			SAlloc::F(obj->PP_NodeTab);
		}
		SAlloc::F(obj);
	}
}

#if defined(DEBUG) || defined(DEBUG_STEP)
/**
 * xmlGenericErrorContextNodeSet:
 * @output:  a FILE * for the output
 * @obj:  the xmlNodeSetPtr to display
 *
 * Quick display of a NodeSet
 */
void xmlGenericErrorContextNodeSet(FILE * output, xmlNodeSet * obj)
{
	SETIFZ(output, xmlGenericErrorContext);
	if(obj == NULL) {
		fprintf(output, "NodeSet == NULL !");
	}
	else if(obj->nodeNr == 0) {
		fprintf(output, "NodeSet is empty");
	}
	else if(obj->nodeTab == NULL) {
		fprintf(output, " nodeTab == NULL !");
	}
	else {
		for(int i = 0; i < obj->nodeNr; i++) {
			if(obj->nodeTab[i] == NULL) {
				fprintf(output, " NULL !");
				break;
			}
			else if(oneof2(obj->nodeTab[i]->type, XML_DOCUMENT_NODE, XML_HTML_DOCUMENT_NODE))
				fprintf(output, " /");
			else if(obj->nodeTab[i]->name == NULL)
				fprintf(output, " noname!");
			else
				fprintf(output, " %s", obj->nodeTab[i]->name);
		}
	}
	fprintf(output, "\n");
}

#endif

/**
 * xmlXPathNewNodeSet:
 * @val:  the NodePtr value
 *
 * Create a new xmlXPathObjectPtr of type NodeSet and initialize
 * it with the single Node @val
 *
 * Returns the newly created object.
 */
xmlXPathObject * xmlXPathNewNodeSet(xmlNode * val)
{
	xmlXPathObject * ret = static_cast<xmlXPathObject *>(SAlloc::M(sizeof(xmlXPathObject)));
	if(!ret)
		xmlXPathErrMemory(NULL, "creating nodeset");
	else {
		memzero(ret, sizeof(xmlXPathObject));
		ret->type = XPATH_NODESET;
		ret->boolval = 0;
		ret->nodesetval = xmlXPathNodeSetCreate(val);
		/* @@ with_ns to check whether namespace nodes should be looked at @@ */
#ifdef XP_DEBUG_OBJ_USAGE
		xmlXPathDebugObjUsageRequested(NULL, XPATH_NODESET);
#endif
	}
	return ret;
}
/**
 * xmlXPathNewValueTree:
 * @val:  the NodePtr value
 *
 * Create a new xmlXPathObjectPtr of type Value Tree (XSLT) and initialize
 * it with the tree root @val
 *
 * Returns the newly created object.
 */
xmlXPathObject * xmlXPathNewValueTree(xmlNode * val)
{
	xmlXPathObject * ret = static_cast<xmlXPathObject *>(SAlloc::M(sizeof(xmlXPathObject)));
	if(!ret)
		xmlXPathErrMemory(NULL, "creating result value tree");
	else {
		memzero(ret, sizeof(xmlXPathObject));
		ret->type = XPATH_XSLT_TREE;
		ret->boolval = 1;
		ret->user = (void *)val;
		ret->nodesetval = xmlXPathNodeSetCreate(val);
#ifdef XP_DEBUG_OBJ_USAGE
		xmlXPathDebugObjUsageRequested(NULL, XPATH_XSLT_TREE);
#endif
	}
	return ret;
}
/**
 * xmlXPathNewNodeSetList:
 * @val:  an existing NodeSet
 *
 * Create a new xmlXPathObjectPtr of type NodeSet and initialize
 * it with the Nodeset @val
 *
 * Returns the newly created object.
 */
xmlXPathObject * xmlXPathNewNodeSetList(xmlNodeSet * val)
{
	xmlXPathObject * ret = 0;
	if(val) {
		if(val->PP_NodeTab == NULL)
			ret = xmlXPathNewNodeSet(NULL);
		else {
			ret = xmlXPathNewNodeSet(val->PP_NodeTab[0]);
			if(ret) {
				for(int i = 1; i < val->nodeNr; ++i) {
					if(xmlXPathNodeSetAddUnique(ret->nodesetval, val->PP_NodeTab[i]) < 0)
						break;
				}
			}
		}
	}
	return ret;
}
/**
 * xmlXPathWrapNodeSet:
 * @val:  the NodePtr value
 *
 * Wrap the Nodeset @val in a new xmlXPathObjectPtr
 *
 * Returns the newly created object.
 */
xmlXPathObject * xmlXPathWrapNodeSet(xmlNodeSet * val)
{
	xmlXPathObject * ret = static_cast<xmlXPathObject *>(SAlloc::M(sizeof(xmlXPathObject)));
	if(!ret)
		xmlXPathErrMemory(NULL, "creating node set object");
	else {
		memzero(ret, sizeof(xmlXPathObject));
		ret->type = XPATH_NODESET;
		ret->nodesetval = val;
#ifdef XP_DEBUG_OBJ_USAGE
		xmlXPathDebugObjUsageRequested(NULL, XPATH_NODESET);
#endif
	}
	return ret;
}

/**
 * xmlXPathFreeNodeSetList:
 * @obj:  an existing NodeSetList object
 *
 * Free up the xmlXPathObjectPtr @obj but don't deallocate the objects in
 * the list contrary to xmlXPathFreeObject().
 */
void xmlXPathFreeNodeSetList(xmlXPathObject * obj)
{
	if(obj) {
#ifdef XP_DEBUG_OBJ_USAGE
		xmlXPathDebugObjUsageReleased(NULL, obj->type);
#endif
		SAlloc::F(obj);
	}
}
/**
 * xmlXPathDifference:
 * @nodes1:  a node-set
 * @nodes2:  a node-set
 *
 * Implements the EXSLT - Sets difference() function:
 *  node-set set:difference (node-set, node-set)
 *
 * Returns the difference between the two node sets, or nodes1 if
 *    nodes2 is empty
 */
xmlNodeSet * xmlXPathDifference(xmlNodeSet * nodes1, xmlNodeSet * nodes2)
{
	xmlNodeSet * ret;
	int i, l1;
	xmlNode * cur;
	if(xmlXPathNodeSetIsEmpty(nodes2))
		return (nodes1);
	ret = xmlXPathNodeSetCreate(NULL);
	if(xmlXPathNodeSetIsEmpty(nodes1))
		return ret;
	l1 = xmlXPathNodeSetGetLength(nodes1);
	for(i = 0; i < l1; i++) {
		cur = xmlXPathNodeSetItem(nodes1, i);
		if(!xmlXPathNodeSetContains(nodes2, cur)) {
			if(xmlXPathNodeSetAddUnique(ret, cur) < 0)
				break;
		}
	}
	return ret;
}
/**
 * xmlXPathIntersection:
 * @nodes1:  a node-set
 * @nodes2:  a node-set
 *
 * Implements the EXSLT - Sets intersection() function:
 *  node-set set:intersection (node-set, node-set)
 *
 * Returns a node set comprising the nodes that are within both the
 *    node sets passed as arguments
 */
xmlNodeSet * xmlXPathIntersection(xmlNodeSet * nodes1, xmlNodeSet * nodes2)
{
	xmlNodeSet * ret = xmlXPathNodeSetCreate(NULL);
	int i, l1;
	xmlNode * cur;
	if(!ret)
		return ret;
	if(xmlXPathNodeSetIsEmpty(nodes1))
		return ret;
	if(xmlXPathNodeSetIsEmpty(nodes2))
		return ret;
	l1 = xmlXPathNodeSetGetLength(nodes1);
	for(i = 0; i < l1; i++) {
		cur = xmlXPathNodeSetItem(nodes1, i);
		if(xmlXPathNodeSetContains(nodes2, cur)) {
			if(xmlXPathNodeSetAddUnique(ret, cur) < 0)
				break;
		}
	}
	return ret;
}
/**
 * xmlXPathDistinctSorted:
 * @nodes:  a node-set, sorted by document order
 *
 * Implements the EXSLT - Sets distinct() function:
 *  node-set set:distinct (node-set)
 *
 * Returns a subset of the nodes contained in @nodes, or @nodes if
 *    it is empty
 */
xmlNodeSet * xmlXPathDistinctSorted(xmlNodeSet * nodes)
{
	xmlNodeSet * ret;
	xmlHashTable * hash;
	int i, l;
	xmlChar * strval;
	xmlNode * cur;
	if(xmlXPathNodeSetIsEmpty(nodes))
		return nodes;
	ret = xmlXPathNodeSetCreate(NULL);
	if(!ret)
		return ret;
	l = xmlXPathNodeSetGetLength(nodes);
	hash = xmlHashCreate(l);
	for(i = 0; i < l; i++) {
		cur = xmlXPathNodeSetItem(nodes, i);
		strval = xmlXPathCastNodeToString(cur);
		if(xmlHashLookup(hash, strval) == NULL) {
			xmlHashAddEntry(hash, strval, strval);
			if(xmlXPathNodeSetAddUnique(ret, cur) < 0)
				break;
		}
		else {
			SAlloc::F(strval);
		}
	}
	xmlHashFree(hash, (xmlHashDeallocator)free);
	return ret;
}
/**
 * xmlXPathDistinct:
 * @nodes:  a node-set
 *
 * Implements the EXSLT - Sets distinct() function:
 *  node-set set:distinct (node-set)
 * @nodes is sorted by document order, then #exslSetsDistinctSorted
 * is called with the sorted node-set
 *
 * Returns a subset of the nodes contained in @nodes, or @nodes if it is empty
 */
xmlNodeSet * xmlXPathDistinct(xmlNodeSet * nodes)
{
	if(xmlXPathNodeSetIsEmpty(nodes))
		return nodes;
	xmlXPathNodeSetSort(nodes);
	return (xmlXPathDistinctSorted(nodes));
}
/**
 * xmlXPathHasSameNodes:
 * @nodes1:  a node-set
 * @nodes2:  a node-set
 *
 * Implements the EXSLT - Sets has-same-nodes function:
 *  boolean set:has-same-node(node-set, node-set)
 *
 * Returns true (1) if @nodes1 shares any node with @nodes2, false (0)
 *    otherwise
 */
int xmlXPathHasSameNodes(xmlNodeSet * nodes1, xmlNodeSet * nodes2)
{
	int i, l;
	if(xmlXPathNodeSetIsEmpty(nodes1) || xmlXPathNodeSetIsEmpty(nodes2))
		return 0;
	l = xmlXPathNodeSetGetLength(nodes1);
	for(i = 0; i < l; i++) {
		xmlNode * cur = xmlXPathNodeSetItem(nodes1, i);
		if(xmlXPathNodeSetContains(nodes2, cur))
			return 1;
	}
	return 0;
}
/**
 * xmlXPathNodeLeadingSorted:
 * @nodes: a node-set, sorted by document order
 * @node: a node
 *
 * Implements the EXSLT - Sets leading() function:
 *  node-set set:leading (node-set, node-set)
 *
 * Returns the nodes in @nodes that precede @node in document order,
 *    @nodes if @node is NULL or an empty node-set if @nodes
 *    doesn't contain @node
 */
xmlNodeSet * xmlXPathNodeLeadingSorted(xmlNodeSet * nodes, xmlNode * pNode)
{
	int i, l;
	xmlNode * cur;
	xmlNodeSet * ret;
	if(!pNode)
		return nodes;
	ret = xmlXPathNodeSetCreate(NULL);
	if(!ret)
		return ret;
	if(xmlXPathNodeSetIsEmpty(nodes) || (!xmlXPathNodeSetContains(nodes, pNode)))
		return ret;
	l = xmlXPathNodeSetGetLength(nodes);
	for(i = 0; i < l; i++) {
		cur = xmlXPathNodeSetItem(nodes, i);
		if(cur == pNode)
			break;
		if(xmlXPathNodeSetAddUnique(ret, cur) < 0)
			break;
	}
	return ret;
}
/**
 * xmlXPathNodeLeading:
 * @nodes:  a node-set
 * @node:  a node
 *
 * Implements the EXSLT - Sets leading() function:
 *  node-set set:leading (node-set, node-set)
 * @nodes is sorted by document order, then #exslSetsNodeLeadingSorted
 * is called.
 *
 * Returns the nodes in @nodes that precede @node in document order,
 *    @nodes if @node is NULL or an empty node-set if @nodes
 *    doesn't contain @node
 */
xmlNodeSet * xmlXPathNodeLeading(xmlNodeSet * nodes, xmlNode * pNode)
{
	xmlXPathNodeSetSort(nodes);
	return xmlXPathNodeLeadingSorted(nodes, pNode);
}
/**
 * xmlXPathLeadingSorted:
 * @nodes1:  a node-set, sorted by document order
 * @nodes2:  a node-set, sorted by document order
 *
 * Implements the EXSLT - Sets leading() function:
 *  node-set set:leading (node-set, node-set)
 *
 * Returns the nodes in @nodes1 that precede the first node in @nodes2
 *    in document order, @nodes1 if @nodes2 is NULL or empty or
 *    an empty node-set if @nodes1 doesn't contain @nodes2
 */
xmlNodeSet * xmlXPathLeadingSorted(xmlNodeSet * nodes1, xmlNodeSet * nodes2)
{
	if(xmlXPathNodeSetIsEmpty(nodes2))
		return (nodes1);
	return (xmlXPathNodeLeadingSorted(nodes1, xmlXPathNodeSetItem(nodes2, 1)));
}
/**
 * xmlXPathLeading:
 * @nodes1:  a node-set
 * @nodes2:  a node-set
 *
 * Implements the EXSLT - Sets leading() function:
 *  node-set set:leading (node-set, node-set)
 * @nodes1 and @nodes2 are sorted by document order, then
 * #exslSetsLeadingSorted is called.
 *
 * Returns the nodes in @nodes1 that precede the first node in @nodes2
 *    in document order, @nodes1 if @nodes2 is NULL or empty or
 *    an empty node-set if @nodes1 doesn't contain @nodes2
 */
xmlNodeSet * xmlXPathLeading(xmlNodeSet * nodes1, xmlNodeSet * nodes2)
{
	if(xmlXPathNodeSetIsEmpty(nodes2))
		return (nodes1);
	if(xmlXPathNodeSetIsEmpty(nodes1))
		return (xmlXPathNodeSetCreate(NULL));
	xmlXPathNodeSetSort(nodes1);
	xmlXPathNodeSetSort(nodes2);
	return (xmlXPathNodeLeadingSorted(nodes1, xmlXPathNodeSetItem(nodes2, 1)));
}
/**
 * xmlXPathNodeTrailingSorted:
 * @nodes: a node-set, sorted by document order
 * @node: a node
 *
 * Implements the EXSLT - Sets trailing() function:
 *  node-set set:trailing (node-set, node-set)
 *
 * Returns the nodes in @nodes that follow @node in document order,
 *    @nodes if @node is NULL or an empty node-set if @nodes
 *    doesn't contain @node
 */
xmlNodeSet * xmlXPathNodeTrailingSorted(xmlNodeSet * nodes, xmlNode * pNode)
{
	int i, l;
	xmlNodeSet * ret = 0;
	if(pNode) {
		ret = xmlXPathNodeSetCreate(NULL);
		if(ret) {
			if(xmlXPathNodeSetIsEmpty(nodes) || (!xmlXPathNodeSetContains(nodes, pNode)))
				return ret;
			l = xmlXPathNodeSetGetLength(nodes);
			for(i = l - 1; i >= 0; i--) {
				xmlNode * cur = xmlXPathNodeSetItem(nodes, i);
				if(cur == pNode)
					break;
				if(xmlXPathNodeSetAddUnique(ret, cur) < 0)
					break;
			}
			xmlXPathNodeSetSort(ret); /* bug 413451 */
		}
	}
	return ret;
}
/**
 * xmlXPathNodeTrailing:
 * @nodes:  a node-set
 * @node:  a node
 *
 * Implements the EXSLT - Sets trailing() function:
 *  node-set set:trailing (node-set, node-set)
 * @nodes is sorted by document order, then #xmlXPathNodeTrailingSorted
 * is called.
 *
 * Returns the nodes in @nodes that follow @node in document order,
 *    @nodes if @node is NULL or an empty node-set if @nodes
 *    doesn't contain @node
 */
xmlNodeSet * xmlXPathNodeTrailing(xmlNodeSet * nodes, xmlNode * P_Node)
{
	xmlXPathNodeSetSort(nodes);
	return (xmlXPathNodeTrailingSorted(nodes, P_Node));
}
/**
 * xmlXPathTrailingSorted:
 * @nodes1:  a node-set, sorted by document order
 * @nodes2:  a node-set, sorted by document order
 *
 * Implements the EXSLT - Sets trailing() function:
 *  node-set set:trailing (node-set, node-set)
 *
 * Returns the nodes in @nodes1 that follow the first node in @nodes2
 *    in document order, @nodes1 if @nodes2 is NULL or empty or
 *    an empty node-set if @nodes1 doesn't contain @nodes2
 */
xmlNodeSet * xmlXPathTrailingSorted(xmlNodeSet * nodes1, xmlNodeSet * nodes2)
{
	return xmlXPathNodeSetIsEmpty(nodes2) ? nodes1 : xmlXPathNodeTrailingSorted(nodes1, xmlXPathNodeSetItem(nodes2, 0));
}
/**
 * xmlXPathTrailing:
 * @nodes1:  a node-set
 * @nodes2:  a node-set
 *
 * Implements the EXSLT - Sets trailing() function:
 *  node-set set:trailing (node-set, node-set)
 * @nodes1 and @nodes2 are sorted by document order, then
 * #xmlXPathTrailingSorted is called.
 *
 * Returns the nodes in @nodes1 that follow the first node in @nodes2
 *    in document order, @nodes1 if @nodes2 is NULL or empty or
 *    an empty node-set if @nodes1 doesn't contain @nodes2
 */
xmlNodeSet * xmlXPathTrailing(xmlNodeSet * nodes1, xmlNodeSet * nodes2)
{
	if(xmlXPathNodeSetIsEmpty(nodes2))
		return (nodes1);
	if(xmlXPathNodeSetIsEmpty(nodes1))
		return (xmlXPathNodeSetCreate(NULL));
	xmlXPathNodeSetSort(nodes1);
	xmlXPathNodeSetSort(nodes2);
	return xmlXPathNodeTrailingSorted(nodes1, xmlXPathNodeSetItem(nodes2, 0));
}
//
// Routines to handle extra functions
//
/**
 * xmlXPathRegisterFunc:
 * @ctxt:  the XPath context
 * @name:  the function name
 * @f:  the function implementation or NULL
 *
 * Register a new function. If @f is NULL it unregisters the function
 *
 * Returns 0 in case of success, -1 in case of error
 */
int FASTCALL xmlXPathRegisterFunc(xmlXPathContext * ctxt, const xmlChar * name, xmlXPathFunction f)
{
	return xmlXPathRegisterFuncNS(ctxt, name, NULL, f);
}
/**
 * xmlXPathRegisterFuncNS:
 * @ctxt:  the XPath context
 * @name:  the function name
 * @ns_uri:  the function namespace URI
 * @f:  the function implementation or NULL
 *
 * Register a new function. If @f is NULL it unregisters the function
 *
 * Returns 0 in case of success, -1 in case of error
 */
int xmlXPathRegisterFuncNS(xmlXPathContext * ctxt, const xmlChar * name, const xmlChar * ns_uri, xmlXPathFunction f)
{
	if(!ctxt)
		return -1;
	if(!name)
		return -1;
	SETIFZ(ctxt->funcHash, xmlHashCreate(0));
	if(ctxt->funcHash == NULL)
		return -1;
	return f ? xmlHashAddEntry2(ctxt->funcHash, name, ns_uri, XML_CAST_FPTR(f)) : xmlHashRemoveEntry2(ctxt->funcHash, name, ns_uri, NULL);
}

/**
 * xmlXPathRegisterFuncLookup:
 * @ctxt:  the XPath context
 * @f:  the lookup function
 * @funcCtxt:  the lookup data
 *
 * Registers an external mechanism to do function lookup.
 */
void xmlXPathRegisterFuncLookup(xmlXPathContext * ctxt, xmlXPathFuncLookupFunc f, void * funcCtxt)
{
	if(ctxt) {
		ctxt->funcLookupFunc = f;
		ctxt->funcLookupData = funcCtxt;
	}
}

/**
 * xmlXPathFunctionLookup:
 * @ctxt:  the XPath context
 * @name:  the function name
 *
 * Search in the Function array of the context for the given
 * function.
 *
 * Returns the xmlXPathFunction or NULL if not found
 */
xmlXPathFunction xmlXPathFunctionLookup(xmlXPathContext * ctxt, const xmlChar * name)
{
	if(!ctxt)
		return 0;
	if(ctxt->funcLookupFunc) {
		xmlXPathFuncLookupFunc f = ctxt->funcLookupFunc;
		xmlXPathFunction ret = f(ctxt->funcLookupData, name, 0);
		if(ret)
			return ret;
	}
	return xmlXPathFunctionLookupNS(ctxt, name, 0);
}

/**
 * xmlXPathFunctionLookupNS:
 * @ctxt:  the XPath context
 * @name:  the function name
 * @ns_uri:  the function namespace URI
 *
 * Search in the Function array of the context for the given
 * function.
 *
 * Returns the xmlXPathFunction or NULL if not found
 */
xmlXPathFunction xmlXPathFunctionLookupNS(xmlXPathContext * ctxt, const xmlChar * name, const xmlChar * ns_uri)
{
	xmlXPathFunction ret;
	if(!ctxt)
		return 0;
	if(!name)
		return 0;
	if(ctxt->funcLookupFunc) {
		xmlXPathFuncLookupFunc f = ctxt->funcLookupFunc;
		ret = f(ctxt->funcLookupData, name, ns_uri);
		if(ret)
			return ret;
	}
	if(ctxt->funcHash == NULL)
		return 0;
	XML_CAST_FPTR(ret) = (xmlXPathFunction)xmlHashLookup2(ctxt->funcHash, name, ns_uri);
	return ret;
}

/**
 * xmlXPathRegisteredFuncsCleanup:
 * @ctxt:  the XPath context
 *
 * Cleanup the XPath context data associated to registered functions
 */
void xmlXPathRegisteredFuncsCleanup(xmlXPathContext * ctxt)
{
	if(ctxt) {
		xmlHashFree(ctxt->funcHash, 0);
		ctxt->funcHash = NULL;
	}
}

/************************************************************************
*									*
*			Routines to handle Variables			*
*									*
************************************************************************/
/**
 * xmlXPathRegisterVariable:
 * @ctxt:  the XPath context
 * @name:  the variable name
 * @value:  the variable value or NULL
 *
 * Register a new variable value. If @value is NULL it unregisters
 * the variable
 *
 * Returns 0 in case of success, -1 in case of error
 */
int xmlXPathRegisterVariable(xmlXPathContext * ctxt, const xmlChar * name, xmlXPathObject * value)
{
	return (xmlXPathRegisterVariableNS(ctxt, name, NULL, value));
}
/**
 * xmlXPathRegisterVariableNS:
 * @ctxt:  the XPath context
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 * @value:  the variable value or NULL
 *
 * Register a new variable value. If @value is NULL it unregisters
 * the variable
 *
 * Returns 0 in case of success, -1 in case of error
 */
int xmlXPathRegisterVariableNS(xmlXPathContext * ctxt, const xmlChar * name, const xmlChar * ns_uri, xmlXPathObject * value)
{
	if(!ctxt)
		return -1;
	if(!name)
		return -1;
	if(ctxt->varHash == NULL)
		ctxt->varHash = xmlHashCreate(0);
	if(ctxt->varHash == NULL)
		return -1;
	if(!value)
		return (xmlHashRemoveEntry2(ctxt->varHash, name, ns_uri, (xmlHashDeallocator)xmlXPathFreeObject));
	return (xmlHashUpdateEntry2(ctxt->varHash, name, ns_uri, (void *)value, (xmlHashDeallocator)xmlXPathFreeObject));
}

/**
 * xmlXPathRegisterVariableLookup:
 * @ctxt:  the XPath context
 * @f:  the lookup function
 * @data:  the lookup data
 *
 * register an external mechanism to do variable lookup
 */
void xmlXPathRegisterVariableLookup(xmlXPathContext * ctxt, xmlXPathVariableLookupFunc f, void * data)
{
	if(ctxt) {
		ctxt->varLookupFunc = f;
		ctxt->varLookupData = data;
	}
}
/**
 * xmlXPathVariableLookup:
 * @ctxt:  the XPath context
 * @name:  the variable name
 *
 * Search in the Variable array of the context for the given
 * variable value.
 *
 * Returns a copy of the value or NULL if not found
 */
xmlXPathObject * xmlXPathVariableLookup(xmlXPathContext * ctxt, const xmlChar * name)
{
	if(!ctxt)
		return 0;
	if(ctxt->varLookupFunc) {
		xmlXPathObject * ret = ((xmlXPathVariableLookupFunc)ctxt->varLookupFunc)(ctxt->varLookupData, name, 0);
		return ret;
	}
	return xmlXPathVariableLookupNS(ctxt, name, 0);
}
/**
 * xmlXPathVariableLookupNS:
 * @ctxt:  the XPath context
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 *
 * Search in the Variable array of the context for the given
 * variable value.
 *
 * Returns the a copy of the value or NULL if not found
 */
xmlXPathObject * xmlXPathVariableLookupNS(xmlXPathContext * ctxt, const xmlChar * name, const xmlChar * ns_uri)
{
	if(!ctxt)
		return 0;
	if(ctxt->varLookupFunc) {
		xmlXPathObject * ret = ((xmlXPathVariableLookupFunc)ctxt->varLookupFunc)(ctxt->varLookupData, name, ns_uri);
		if(ret)
			return ret;
	}
	if(ctxt->varHash == NULL)
		return 0;
	if(!name)
		return 0;
	return xmlXPathCacheObjectCopy(ctxt, (xmlXPathObject *)xmlHashLookup2(ctxt->varHash, name, ns_uri));
}

/**
 * xmlXPathRegisteredVariablesCleanup:
 * @ctxt:  the XPath context
 *
 * Cleanup the XPath context data associated to registered variables
 */
void xmlXPathRegisteredVariablesCleanup(xmlXPathContext * ctxt)
{
	if(ctxt) {
		xmlHashFree(ctxt->varHash, (xmlHashDeallocator)xmlXPathFreeObject);
		ctxt->varHash = NULL;
	}
}

/**
 * xmlXPathRegisterNs:
 * @ctxt:  the XPath context
 * @prefix:  the namespace prefix cannot be NULL or empty string
 * @ns_uri:  the namespace name
 *
 * Register a new namespace. If @ns_uri is NULL it unregisters
 * the namespace
 *
 * Returns 0 in case of success, -1 in case of error
 */
int xmlXPathRegisterNs(xmlXPathContext * ctxt, const xmlChar * prefix, const xmlChar * ns_uri)
{
	if(!ctxt)
		return -1;
	if(prefix == NULL)
		return -1;
	if(prefix[0] == 0)
		return -1;
	SETIFZ(ctxt->nsHash, xmlHashCreate(10));
	if(ctxt->nsHash == NULL)
		return -1;
	if(ns_uri == NULL)
		return xmlHashRemoveEntry(ctxt->nsHash, prefix, (xmlHashDeallocator)free);
	return xmlHashUpdateEntry(ctxt->nsHash, prefix, (void *)sstrdup(ns_uri), (xmlHashDeallocator)free);
}

/**
 * xmlXPathNsLookup:
 * @ctxt:  the XPath context
 * @prefix:  the namespace prefix value
 *
 * Search in the namespace declaration array of the context for the given
 * namespace name associated to the given prefix
 *
 * Returns the value or NULL if not found
 */
const xmlChar * xmlXPathNsLookup(xmlXPathContext * ctxt, const xmlChar * prefix)
{
	if(!ctxt)
		return 0;
	if(prefix == NULL)
		return 0;
#ifdef XML_XML_NAMESPACE
	if(sstreq(prefix, (const xmlChar *)"xml"))
		return (XML_XML_NAMESPACE);
#endif
	if(ctxt->namespaces) {
		for(int i = 0; i < ctxt->nsNr; i++) {
			if(ctxt->namespaces[i] && (sstreq(ctxt->namespaces[i]->prefix, prefix)))
				return (ctxt->namespaces[i]->href);
		}
	}
	return (const xmlChar *)xmlHashLookup(ctxt->nsHash, prefix);
}

/**
 * xmlXPathRegisteredNsCleanup:
 * @ctxt:  the XPath context
 *
 * Cleanup the XPath context data associated to registered variables
 */
void xmlXPathRegisteredNsCleanup(xmlXPathContext * ctxt)
{
	if(ctxt) {
		xmlHashFree(ctxt->nsHash, (xmlHashDeallocator)free);
		ctxt->nsHash = NULL;
	}
}
// 
// Routines to handle Values
// 
/* Allocations are terrible, one needs to optimize all this !!! */

/**
 * xmlXPathNewFloat:
 * @val:  the double value
 *
 * Create a new xmlXPathObjectPtr of type double and of value @val
 *
 * Returns the newly created object.
 */
xmlXPathObject * xmlXPathNewFloat(double val)
{
	xmlXPathObject * ret = static_cast<xmlXPathObject *>(SAlloc::M(sizeof(xmlXPathObject)));
	if(!ret) {
		xmlXPathErrMemory(NULL, "creating float object");
		return 0;
	}
	memzero(ret, sizeof(xmlXPathObject));
	ret->type = XPATH_NUMBER;
	ret->floatval = val;
#ifdef XP_DEBUG_OBJ_USAGE
	xmlXPathDebugObjUsageRequested(NULL, XPATH_NUMBER);
#endif
	return ret;
}
/**
 * xmlXPathNewBoolean:
 * @val:  the boolean value
 *
 * Create a new xmlXPathObjectPtr of type boolean and of value @val
 *
 * Returns the newly created object.
 */
xmlXPathObject * xmlXPathNewBoolean(int val)
{
	xmlXPathObject * ret = static_cast<xmlXPathObject *>(SAlloc::M(sizeof(xmlXPathObject)));
	if(!ret) {
		xmlXPathErrMemory(NULL, "creating boolean object");
		return 0;
	}
	memzero(ret, sizeof(xmlXPathObject));
	ret->type = XPATH_BOOLEAN;
	ret->boolval = (val != 0);
#ifdef XP_DEBUG_OBJ_USAGE
	xmlXPathDebugObjUsageRequested(NULL, XPATH_BOOLEAN);
#endif
	return ret;
}
/**
 * xmlXPathNewString:
 * @val:  the xmlChar * value
 *
 * Create a new xmlXPathObjectPtr of type string and of value @val
 *
 * Returns the newly created object.
 */
xmlXPathObject * xmlXPathNewString(const xmlChar * val)
{
	xmlXPathObject * ret = static_cast<xmlXPathObject *>(SAlloc::M(sizeof(xmlXPathObject)));
	if(!ret) {
		xmlXPathErrMemory(NULL, "creating string object");
		return 0;
	}
	memzero(ret, sizeof(xmlXPathObject));
	ret->type = XPATH_STRING;
	if(val)
		ret->stringval = sstrdup(val);
	else
		ret->stringval = reinterpret_cast<xmlChar *>(sstrdup(""));
#ifdef XP_DEBUG_OBJ_USAGE
	xmlXPathDebugObjUsageRequested(NULL, XPATH_STRING);
#endif
	return ret;
}
/**
 * xmlXPathWrapString:
 * @val:  the xmlChar * value
 *
 * Wraps the @val string into an XPath object.
 *
 * Returns the newly created object.
 */
xmlXPathObject * xmlXPathWrapString(xmlChar * val)
{
	xmlXPathObject * ret = static_cast<xmlXPathObject *>(SAlloc::M(sizeof(xmlXPathObject)));
	if(!ret) {
		xmlXPathErrMemory(NULL, "creating string object");
		return 0;
	}
	memzero(ret, sizeof(xmlXPathObject));
	ret->type = XPATH_STRING;
	ret->stringval = val;
#ifdef XP_DEBUG_OBJ_USAGE
	xmlXPathDebugObjUsageRequested(NULL, XPATH_STRING);
#endif
	return ret;
}
/**
 * xmlXPathNewCString:
 * @val:  the char * value
 *
 * Create a new xmlXPathObjectPtr of type string and of value @val
 *
 * Returns the newly created object.
 */
xmlXPathObject * xmlXPathNewCString(const char * val)
{
	xmlXPathObject * ret = static_cast<xmlXPathObject *>(SAlloc::M(sizeof(xmlXPathObject)));
	if(!ret) {
		xmlXPathErrMemory(NULL, "creating string object");
		return 0;
	}
	memzero(ret, sizeof(xmlXPathObject));
	ret->type = XPATH_STRING;
	ret->stringval = sstrdup(BAD_CAST val);
#ifdef XP_DEBUG_OBJ_USAGE
	xmlXPathDebugObjUsageRequested(NULL, XPATH_STRING);
#endif
	return ret;
}
/**
 * xmlXPathWrapCString:
 * @val:  the char * value
 *
 * Wraps a string into an XPath object.
 *
 * Returns the newly created object.
 */
xmlXPathObject * xmlXPathWrapCString(char * val) 
{
	return xmlXPathWrapString((xmlChar *)(val));
}
/**
 * xmlXPathWrapExternal:
 * @val:  the user data
 *
 * Wraps the @val data into an XPath object.
 *
 * Returns the newly created object.
 */
xmlXPathObject * xmlXPathWrapExternal(void * val)
{
	xmlXPathObject * ret = static_cast<xmlXPathObject *>(SAlloc::M(sizeof(xmlXPathObject)));
	if(!ret) {
		xmlXPathErrMemory(NULL, "creating user object");
		return 0;
	}
	memzero(ret, sizeof(xmlXPathObject));
	ret->type = XPATH_USERS;
	ret->user = val;
#ifdef XP_DEBUG_OBJ_USAGE
	xmlXPathDebugObjUsageRequested(NULL, XPATH_USERS);
#endif
	return ret;
}
/**
 * xmlXPathObjectCopy:
 * @val:  the original object
 *
 * allocate a new copy of a given object
 *
 * Returns the newly created object.
 */
xmlXPathObject * xmlXPathObjectCopy(xmlXPathObject * val)
{
	xmlXPathObject * ret;
	if(!val)
		return 0;
	ret = static_cast<xmlXPathObject *>(SAlloc::M(sizeof(xmlXPathObject)));
	if(!ret) {
		xmlXPathErrMemory(NULL, "copying object");
		return 0;
	}
	memcpy(ret, val, sizeof(xmlXPathObject));
#ifdef XP_DEBUG_OBJ_USAGE
	xmlXPathDebugObjUsageRequested(NULL, val->type);
#endif
	switch(val->type) {
		case XPATH_BOOLEAN:
		case XPATH_NUMBER:
		case XPATH_POINT:
		case XPATH_RANGE:
		    break;
		case XPATH_STRING:
		    ret->stringval = sstrdup(val->stringval);
		    break;
		case XPATH_XSLT_TREE:
#if 0
/*
   Removed 11 July 2004 - the current handling of xslt tmpRVT nodes means that
   this previous handling is no longer correct, and can cause some serious
   problems (ref. bug 145547)
 */
		    if(val->nodesetval && val->nodesetval->nodeTab) {
			    xmlNode * cur;
				xmlNode * tmp;
			    xmlDoc * top;
			    ret->boolval = 1;
			    top =  xmlNewDoc(NULL);
			    top->name = (char *)sstrdup(val->nodesetval->nodeTab[0]->name);
			    ret->user = top;
			    if(top) {
				    top->doc = top;
				    cur = val->nodesetval->nodeTab[0]->children;
				    while(cur) {
					    tmp = xmlDocCopyNode(cur, top, 1);
					    xmlAddChild((xmlNode *)top, tmp);
					    cur = cur->next;
				    }
			    }

			    ret->nodesetval = xmlXPathNodeSetCreate((xmlNode *)top);
		    }
		    else
			    ret->nodesetval = xmlXPathNodeSetCreate(NULL);
		    /* Deallocate the copied tree value */
		    break;
#endif
		case XPATH_NODESET:
		    ret->nodesetval = xmlXPathNodeSetMerge(NULL, val->nodesetval);
		    /* Do not deallocate the copied tree value */
		    ret->boolval = 0;
		    break;
		case XPATH_LOCATIONSET:
#ifdef LIBXML_XPTR_ENABLED
		    {
			    xmlLocationSet * loc = (xmlLocationSet *)val->user;
			    ret->user = (void *)xmlXPtrLocationSetMerge(NULL, loc);
			    break;
		    }
#endif
		case XPATH_USERS:
		    ret->user = val->user;
		    break;
		case XPATH_UNDEFINED:
		    xmlGenericError(0, "xmlXPathObjectCopy: unsupported type %d\n", val->type);
		    break;
	}
	return ret;
}
/**
 * xmlXPathFreeObject:
 * @obj:  the object to free
 *
 * Free up an xmlXPathObjectPtr object.
 */
void FASTCALL xmlXPathFreeObject(xmlXPathObject * obj)
{
	if(obj) {
		if(oneof2(obj->type, XPATH_NODESET, XPATH_XSLT_TREE)) {
			if(obj->boolval) {
#if 0
				if(obj->user) {
					xmlXPathFreeNodeSet(obj->nodesetval);
					xmlFreeNodeList((xmlNode *)obj->user);
				}
				else
#endif
				obj->type = XPATH_XSLT_TREE; /* @todo Just for debugging. */
				xmlXPathFreeValueTree(obj->nodesetval);
			}
			else {
				xmlXPathFreeNodeSet(obj->nodesetval);
			}
#ifdef LIBXML_XPTR_ENABLED
		}
		else if(obj->type == XPATH_LOCATIONSET) {
			xmlXPtrFreeLocationSet((xmlLocationSet *)obj->user);
#endif
		}
		else if(obj->type == XPATH_STRING) {
			SAlloc::F(obj->stringval);
		}
#ifdef XP_DEBUG_OBJ_USAGE
		xmlXPathDebugObjUsageReleased(NULL, obj->type);
#endif
		SAlloc::F(obj);
	}
}
/**
 * @obj:  the xmlXPathObjectPtr to free or to cache
 *
 * Depending on the state of the cache this frees the given
 * XPath object or stores it in the cache.
 */
static void FASTCALL xmlXPathReleaseObject(xmlXPathContext * ctxt, xmlXPathObject * pObj)
{
#define XP_CACHE_ADD(sl, o) if(!sl) { sl = xmlPointerListCreate(10); if(sl == NULL) goto free_obj; } if(xmlPointerListAddSize(sl, pObj, 0) == -1) goto free_obj;
#define XP_CACHE_WANTS(sl, n) ((sl == NULL) || ((sl)->number < n))
	if(pObj) {
		if(!ctxt || (ctxt->cache == NULL)) {
			xmlXPathFreeObject(pObj);
		}
		else {
			xmlXPathContextCachePtr cache = (xmlXPathContextCachePtr)ctxt->cache;
			switch(pObj->type) {
				case XPATH_NODESET:
				case XPATH_XSLT_TREE:
					if(pObj->nodesetval) {
						if(pObj->boolval) {
							/*
							 * It looks like the @boolval is used for
							 * evaluation if this an XSLT Result Tree Fragment.
							 * @todo Check if this assumption is correct.
							 */
							pObj->type = XPATH_XSLT_TREE; /* just for debugging */
							xmlXPathFreeValueTree(pObj->nodesetval);
							pObj->nodesetval = NULL;
						}
						else if((pObj->nodesetval->nodeMax <= 40) && (XP_CACHE_WANTS(cache->nodesetObjs, cache->maxNodeset))) {
							XP_CACHE_ADD(cache->nodesetObjs, pObj);
							goto obj_cached;
						}
						else {
							xmlXPathFreeNodeSet(pObj->nodesetval);
							pObj->nodesetval = NULL;
						}
					}
					break;
				case XPATH_STRING:
					SAlloc::F(pObj->stringval);
					if(XP_CACHE_WANTS(cache->stringObjs, cache->maxString)) {
						XP_CACHE_ADD(cache->stringObjs, pObj);
						goto obj_cached;
					}
					break;
				case XPATH_BOOLEAN:
					if(XP_CACHE_WANTS(cache->booleanObjs, cache->maxBoolean)) {
						XP_CACHE_ADD(cache->booleanObjs, pObj);
						goto obj_cached;
					}
					break;
				case XPATH_NUMBER:
					if(XP_CACHE_WANTS(cache->numberObjs, cache->maxNumber)) {
						XP_CACHE_ADD(cache->numberObjs, pObj);
						goto obj_cached;
					}
					break;
	#ifdef LIBXML_XPTR_ENABLED
				case XPATH_LOCATIONSET:
					xmlXPtrFreeLocationSet((xmlLocationSet *)pObj->user);
					goto free_obj;
	#endif
				default:
					goto free_obj;
			}
			//
			// Fallback to adding to the misc-objects slot.
			//
			if(XP_CACHE_WANTS(cache->miscObjs, cache->maxMisc)) {
				XP_CACHE_ADD(cache->miscObjs, pObj);
			}
			else
				goto free_obj;
	obj_cached:
	#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageReleased(ctxt, obj->type);
	#endif
			if(pObj->nodesetval) {
				xmlNodeSet * tmpset = pObj->nodesetval;
				/*
				 * @todo Due to those nasty ns-nodes, we need to traverse
				 *  the list and free the ns-nodes.
				 * URGENT TODO: Check if it's actually slowing things down.
				 *  Maybe we shouldn't try to preserve the list.
				 */
				if(tmpset->nodeNr > 1) {
					for(int i = 0; i < tmpset->nodeNr; i++) {
						xmlNode * p_node = tmpset->PP_NodeTab[i];
						if(p_node && p_node->type == XML_NAMESPACE_DECL)
							xmlXPathNodeSetFreeNs((xmlNs *)p_node);
					}
				}
				else if(tmpset->nodeNr == 1) {
					if(tmpset->PP_NodeTab[0] && (tmpset->PP_NodeTab[0]->type == XML_NAMESPACE_DECL))
						xmlXPathNodeSetFreeNs((xmlNs *)tmpset->PP_NodeTab[0]);
				}
				tmpset->nodeNr = 0;
				memzero(pObj, sizeof(xmlXPathObject));
				pObj->nodesetval = tmpset;
			}
			else
				memzero(pObj, sizeof(xmlXPathObject));
			return;
free_obj:
			// Cache is full; free the object.
			xmlXPathFreeNodeSet(pObj->nodesetval);
#ifdef XP_DEBUG_OBJ_USAGE
			xmlXPathDebugObjUsageReleased(NULL, obj->type);
#endif
			SAlloc::F(pObj);
		}
	}
}
// 
// Type Casting Routines
// 
/**
 * xmlXPathCastBooleanToString:
 * @val:  a boolean
 *
 * Converts a boolean to its string value.
 *
 * Returns a newly allocated string.
 */
xmlChar * xmlXPathCastBooleanToString(int val) 
{
	return sstrdup((const xmlChar *)(val ? "true" : "false"));
}
/**
 * xmlXPathCastNumberToString:
 * @val:  a number
 *
 * Converts a number to its string value.
 *
 * Returns a newly allocated string.
 */
xmlChar * xmlXPathCastNumberToString(double val) 
{
	xmlChar * ret;
	switch(xmlXPathIsInf(val)) {
		case 1: ret = sstrdup((const xmlChar *)"Infinity"); break;
		case -1: ret = sstrdup((const xmlChar *)"-Infinity"); break;
		default:
		    if(fisnan(val)) {
			    ret = sstrdup((const xmlChar *)"NaN");
		    }
		    else if(val == 0 && xmlXPathGetSign(val) != 0) {
			    ret = sstrdup((const xmlChar *)"0");
		    }
		    else {
			    /* could be improved */
			    char buf[100];
			    xmlXPathFormatNumber(val, buf, 99);
			    buf[99] = 0;
			    ret = sstrdup((const xmlChar *)buf);
		    }
	}
	return ret;
}
/**
 * xmlXPathCastNodeToString:
 * @node:  a node
 *
 * Converts a node to its string value.
 *
 * Returns a newly allocated string.
 */
xmlChar * xmlXPathCastNodeToString(xmlNode * pNode) 
{
	xmlChar * ret = xmlNodeGetContent(pNode);
	return NZOR(ret, reinterpret_cast<xmlChar *>(sstrdup("")));
	/*if(ret == NULL)
		ret = reinterpret_cast<xmlChar *>(sstrdup(""));
	return ret;*/
}
/**
 * xmlXPathCastNodeSetToString:
 * @ns:  a node-set
 *
 * Converts a node-set to its string value.
 *
 * Returns a newly allocated string.
 */
xmlChar * xmlXPathCastNodeSetToString(xmlNodeSet * ns) 
{
	if((ns == NULL) || (ns->nodeNr == 0) || (ns->PP_NodeTab == NULL))
		return reinterpret_cast<xmlChar *>(sstrdup(""));
	if(ns->nodeNr > 1)
		xmlXPathNodeSetSort(ns);
	return (xmlXPathCastNodeToString(ns->PP_NodeTab[0]));
}

/**
 * xmlXPathCastToString:
 * @val:  an XPath object
 *
 * Converts an existing object to its string() equivalent
 *
 * Returns the allocated string value of the object, NULL in case of error.
 *    It's up to the caller to free the string memory with SAlloc::F().
 */
xmlChar * xmlXPathCastToString(xmlXPathObject * val)
{
	xmlChar * ret = NULL;
	if(!val)
		return reinterpret_cast<xmlChar *>(sstrdup(""));
	switch(val->type) {
		case XPATH_UNDEFINED:
#ifdef DEBUG_EXPR
		    xmlGenericError(0, "String: undefined\n");
#endif
		    ret = reinterpret_cast<xmlChar *>(sstrdup(""));
		    break;
		case XPATH_NODESET:
		case XPATH_XSLT_TREE: ret = xmlXPathCastNodeSetToString(val->nodesetval); break;
		case XPATH_STRING: return sstrdup(val->stringval);
		case XPATH_BOOLEAN: ret = xmlXPathCastBooleanToString(val->boolval); break;
		case XPATH_NUMBER: ret = xmlXPathCastNumberToString(val->floatval); break;
		case XPATH_USERS:
		case XPATH_POINT:
		case XPATH_RANGE:
		case XPATH_LOCATIONSET:
		    TODO
		    ret = reinterpret_cast<xmlChar *>(sstrdup(""));
		    break;
	}
	return ret;
}
/**
 * xmlXPathConvertString:
 * @val:  an XPath object
 *
 * Converts an existing object to its string() equivalent
 *
 * Returns the new object, the old one is freed (or the operation
 *    is done directly on @val)
 */
xmlXPathObject * xmlXPathConvertString(xmlXPathObject * val) 
{
	xmlChar * res = NULL;
	if(!val)
		return xmlXPathNewCString("");
	else {
		switch(val->type) {
			case XPATH_UNDEFINED:
	#ifdef DEBUG_EXPR
				xmlGenericError(0, "STRING: undefined\n");
	#endif
				break;
			case XPATH_NODESET:
			case XPATH_XSLT_TREE: res = xmlXPathCastNodeSetToString(val->nodesetval); break;
			case XPATH_STRING: return val;
			case XPATH_BOOLEAN: res = xmlXPathCastBooleanToString(val->boolval); break;
			case XPATH_NUMBER: res = xmlXPathCastNumberToString(val->floatval); break;
			case XPATH_USERS:
			case XPATH_POINT:
			case XPATH_RANGE:
			case XPATH_LOCATIONSET:
				TODO;
				break;
		}
		xmlXPathFreeObject(val);
		return res ? xmlXPathWrapString(res) : xmlXPathNewCString("");
	}
}

/**
 * xmlXPathCastBooleanToNumber:
 * @val:  a boolean
 *
 * Converts a boolean to its number value
 *
 * Returns the number value
 */
double xmlXPathCastBooleanToNumber(int val) 
{
	return val ? 1.0 : 0.0;
}
/**
 * xmlXPathCastStringToNumber:
 * @val:  a string
 *
 * Converts a string to its number value
 *
 * Returns the number value
 */
double xmlXPathCastStringToNumber(const xmlChar * val)
{
	return xmlXPathStringEvalNumber(val);
}
/**
 * xmlXPathCastNodeToNumber:
 * @node:  a node
 *
 * Converts a node to its number value
 *
 * Returns the number value
 */
double xmlXPathCastNodeToNumber(xmlNode * P_Node)
{
	double ret = xmlXPathNAN;
	if(P_Node) {
		xmlChar * strval = xmlXPathCastNodeToString(P_Node);
		if(strval) {
			ret = xmlXPathCastStringToNumber(strval);
			SAlloc::F(strval);
		}
	}
	return ret;
}
/**
 * xmlXPathCastNodeSetToNumber:
 * @ns:  a node-set
 *
 * Converts a node-set to its number value
 *
 * Returns the number value
 */
double xmlXPathCastNodeSetToNumber(xmlNodeSet * ns)
{
	double ret = xmlXPathNAN;
	if(ns) {
		xmlChar * str = xmlXPathCastNodeSetToString(ns);
		ret = xmlXPathCastStringToNumber(str);
		SAlloc::F(str);
	}
	return ret;
}
/**
 * xmlXPathCastToNumber:
 * @val:  an XPath object
 *
 * Converts an XPath object to its number value
 *
 * Returns the number value
 */
double xmlXPathCastToNumber(xmlXPathObject * val)
{
	double ret = 0.0;
	if(!val)
		ret = xmlXPathNAN;
	else {
		switch(val->type) {
			case XPATH_UNDEFINED:
#ifdef DEGUB_EXPR
				xmlGenericError(0, "NUMBER: undefined\n");
#endif
				ret = xmlXPathNAN;
				break;
			case XPATH_NODESET:
			case XPATH_XSLT_TREE:
				ret = xmlXPathCastNodeSetToNumber(val->nodesetval);
				break;
			case XPATH_STRING:
				ret = xmlXPathCastStringToNumber(val->stringval);
				break;
			case XPATH_NUMBER:
				ret = val->floatval;
				break;
			case XPATH_BOOLEAN:
				ret = xmlXPathCastBooleanToNumber(val->boolval);
				break;
			case XPATH_USERS:
			case XPATH_POINT:
			case XPATH_RANGE:
			case XPATH_LOCATIONSET:
				TODO;
				ret = xmlXPathNAN;
				break;
		}
	}
	return ret;
}
/**
 * xmlXPathConvertNumber:
 * @val:  an XPath object
 *
 * Converts an existing object to its number() equivalent
 *
 * Returns the new object, the old one is freed (or the operation
 *    is done directly on @val)
 */
xmlXPathObject * xmlXPathConvertNumber(xmlXPathObject * val)
{
	xmlXPathObject * ret;
	if(!val)
		return (xmlXPathNewFloat(0.0));
	if(val->type == XPATH_NUMBER)
		return val;
	ret = xmlXPathNewFloat(xmlXPathCastToNumber(val));
	xmlXPathFreeObject(val);
	return ret;
}
/**
 * xmlXPathCastNumberToBoolean:
 * @val:  a number
 *
 * Converts a number to its boolean value
 *
 * Returns the boolean value
 */
int xmlXPathCastNumberToBoolean(double val)
{
	return (fisnan(val) || (val == 0.0)) ? 0 : 1;
}
/**
 * xmlXPathCastStringToBoolean:
 * @val:  a string
 *
 * Converts a string to its boolean value
 *
 * Returns the boolean value
 */
int xmlXPathCastStringToBoolean(const xmlChar * val)
{
	return (!val || !sstrlen(val)) ? 0 : 1;
}
/**
 * xmlXPathCastNodeSetToBoolean:
 * @ns:  a node-set
 *
 * Converts a node-set to its boolean value
 *
 * Returns the boolean value
 */
int xmlXPathCastNodeSetToBoolean(const xmlNodeSet * ns)
{
	return (!ns || !ns->nodeNr) ? 0 : 1;
}

/**
 * xmlXPathCastToBoolean:
 * @val:  an XPath object
 *
 * Converts an XPath object to its boolean value
 *
 * Returns the boolean value
 */
int xmlXPathCastToBoolean(xmlXPathObject * val)
{
	int ret = 0;
	if(!val)
		return 0;
	switch(val->type) {
		case XPATH_UNDEFINED:
#ifdef DEBUG_EXPR
		    xmlGenericError(0, "BOOLEAN: undefined\n");
#endif
		    ret = 0;
		    break;
		case XPATH_NODESET:
		case XPATH_XSLT_TREE:
		    ret = xmlXPathCastNodeSetToBoolean(val->nodesetval);
		    break;
		case XPATH_STRING:
		    ret = xmlXPathCastStringToBoolean(val->stringval);
		    break;
		case XPATH_NUMBER:
		    ret = xmlXPathCastNumberToBoolean(val->floatval);
		    break;
		case XPATH_BOOLEAN:
		    ret = val->boolval;
		    break;
		case XPATH_USERS:
		case XPATH_POINT:
		case XPATH_RANGE:
		case XPATH_LOCATIONSET:
		    TODO;
		    ret = 0;
		    break;
	}
	return ret;
}
/**
 * xmlXPathConvertBoolean:
 * @val:  an XPath object
 *
 * Converts an existing object to its boolean() equivalent
 *
 * Returns the new object, the old one is freed (or the operation
 *    is done directly on @val)
 */
xmlXPathObject * xmlXPathConvertBoolean(xmlXPathObject * val)
{
	if(!val)
		return xmlXPathNewBoolean(0);
	else if(val->type == XPATH_BOOLEAN)
		return val;
	else {
		xmlXPathObject * ret = xmlXPathNewBoolean(xmlXPathCastToBoolean(val));
		xmlXPathFreeObject(val);
		return ret;
	}
}
//
// Routines to handle XPath contexts
//
/**
 * xmlXPathNewContext:
 * @doc:  the XML document
 *
 * Create a new xmlXPathContext
 *
 * Returns the xmlXPathContext just allocated. The caller will need to free it.
 */
xmlXPathContext * xmlXPathNewContext(xmlDoc * doc)
{
	xmlXPathContext * ret = (xmlXPathContext *)SAlloc::M(sizeof(xmlXPathContext));
	if(!ret) {
		xmlXPathErrMemory(NULL, "creating context");
		return 0;
	}
	memzero(ret, sizeof(xmlXPathContext));
	ret->doc = doc;
	ret->P_Node = NULL;
	ret->varHash = NULL;
	ret->nb_types = 0;
	ret->max_types = 0;
	ret->types = NULL;
	ret->funcHash = xmlHashCreate(0);
	ret->nb_axis = 0;
	ret->max_axis = 0;
	ret->axis = NULL;
	ret->nsHash = NULL;
	ret->user = NULL;
	ret->contextSize = -1;
	ret->proximityPosition = -1;
#ifdef XP_DEFAULT_CACHE_ON
	if(xmlXPathContextSetCache(ret, 1, -1, 0) == -1) {
		xmlXPathFreeContext(ret);
		return 0;
	}
#endif
	xmlXPathRegisterAllFunctions(ret);
	return ret;
}

/**
 * xmlXPathFreeContext:
 * @ctxt:  the context to free
 *
 * Free up an xmlXPathContext
 */
void xmlXPathFreeContext(xmlXPathContext * ctxt)
{
	if(ctxt) {
		xmlXPathFreeCache((xmlXPathContextCachePtr)ctxt->cache);
		xmlXPathRegisteredNsCleanup(ctxt);
		xmlXPathRegisteredFuncsCleanup(ctxt);
		xmlXPathRegisteredVariablesCleanup(ctxt);
		xmlResetError(&ctxt->lastError);
		SAlloc::F(ctxt);
	}
}
//
// Routines to handle XPath parser contexts
//
#define CHECK_CTXT(ctxt) if(!ctxt) { \
		__xmlRaiseError(0, 0, 0, 0, 0, XML_FROM_XPATH, XML_ERR_INTERNAL_ERROR, XML_ERR_FATAL, __FILE__, __LINE__, NULL, NULL, NULL, 0, 0, "NULL context pointer\n"); \
		return 0; }

#define CHECK_CTXT_NEG(ctxt) if(!ctxt) { \
		__xmlRaiseError(0, 0, 0, 0, 0, XML_FROM_XPATH, XML_ERR_INTERNAL_ERROR, XML_ERR_FATAL, __FILE__, __LINE__, NULL, NULL, NULL, 0, 0, "NULL context pointer\n"); \
		return -1; }

#define CHECK_CONTEXT(ctxt) if(!ctxt || !ctxt->doc || !ctxt->doc->children) { xmlXPatherror(ctxt, __FILE__, __LINE__, XPATH_INVALID_CTXT); return 0; }

/**
 * xmlXPathNewParserContext:
 * @str:  the XPath expression
 * @ctxt:  the XPath context
 *
 * Create a new xmlXPathParserContext
 *
 * Returns the xmlXPathParserContext just allocated.
 */
xmlXPathParserContext * xmlXPathNewParserContext(const xmlChar * str, xmlXPathContext * ctxt)
{
	xmlXPathParserContext * ret = static_cast<xmlXPathParserContext *>(SAlloc::M(sizeof(xmlXPathParserContext)));
	if(!ret) {
		xmlXPathErrMemory(ctxt, "creating parser context");
		return 0;
	}
	memzero(ret, sizeof(xmlXPathParserContext));
	ret->cur = ret->base = str;
	ret->context = ctxt;
	ret->comp = xmlXPathNewCompExpr();
	if(ret->comp == NULL) {
		SAlloc::F(ret->valueTab);
		SAlloc::F(ret);
		return 0;
	}
	if(ctxt && ctxt->dict) {
		ret->comp->dict = ctxt->dict;
		xmlDictReference(ret->comp->dict);
	}
	return ret;
}
/**
 * xmlXPathCompParserContext:
 * @comp:  the XPath compiled expression
 * @ctxt:  the XPath context
 *
 * Create a new xmlXPathParserContext when processing a compiled expression
 *
 * Returns the xmlXPathParserContext just allocated.
 */
static xmlXPathParserContext * xmlXPathCompParserContext(xmlXPathCompExprPtr comp, xmlXPathContext * ctxt)
{
	xmlXPathParserContext * ret = static_cast<xmlXPathParserContext *>(SAlloc::M(sizeof(xmlXPathParserContext)));
	if(!ret)
		xmlXPathErrMemory(ctxt, "creating evaluation context");
	else {
		memzero(ret, sizeof(xmlXPathParserContext));
		// Allocate the value stack 
		ret->valueTab = static_cast<xmlXPathObject **>(SAlloc::M(10 * sizeof(xmlXPathObject *)));
		if(ret->valueTab == NULL) {
			ZFREE(ret);
			xmlXPathErrMemory(ctxt, "creating evaluation context");
		}
		else {
			ret->valueNr = 0;
			ret->valueMax = 10;
			ret->value = NULL;
			ret->valueFrame = 0;
			ret->context = ctxt;
			ret->comp = comp;
		}
	}
	return ret;
}
/**
 * xmlXPathFreeParserContext:
 * @ctxt:  the context to free
 *
 * Free up an xmlXPathParserContext
 */
void xmlXPathFreeParserContext(xmlXPathParserContext * ctxt)
{
	SAlloc::F(ctxt->valueTab);
	if(ctxt->comp) {
#ifdef XPATH_STREAMING
		xmlFreePatternList(ctxt->comp->stream);
		ctxt->comp->stream = 0;
#endif
		xmlXPathFreeCompExpr(ctxt->comp);
	}
	SAlloc::F(ctxt);
}
// 
// The implicit core function library
// 
/**
 * xmlXPathNodeValHash:
 * @node:  a node pointer
 *
 * Function computing the beginning of the string value of the node,
 * used to speed up comparisons
 *
 * Returns an int usable as a hash
 */
static uint xmlXPathNodeValHash(xmlNode * pNode)
{
	int len = 2;
	const xmlChar * string = NULL;
	xmlNode * tmp = NULL;
	uint ret = 0;
	if(!pNode)
		return 0;
	if(pNode->type == XML_DOCUMENT_NODE) {
		tmp = xmlDocGetRootElement(reinterpret_cast<xmlDoc *>(pNode));
		pNode = tmp ? tmp : pNode->children;
		if(!pNode)
			return 0;
	}
	switch(pNode->type) {
		case XML_COMMENT_NODE:
		case XML_PI_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_TEXT_NODE:
		    string = pNode->content;
		    if(string == NULL)
			    return 0;
		    if(string[0] == 0)
			    return 0;
		    return (((uint)string[0]) + (((uint)string[1]) << 8));
		case XML_NAMESPACE_DECL:
		    string = reinterpret_cast<xmlNs *>(pNode)->href;
		    if(string == NULL)
			    return 0;
		    if(string[0] == 0)
			    return 0;
		    return (((uint)string[0]) + (((uint)string[1]) << 8));
		case XML_ATTRIBUTE_NODE:
		    tmp = reinterpret_cast<xmlAttr *>(pNode)->children;
		    break;
		case XML_ELEMENT_NODE:
		    tmp = pNode->children;
		    break;
		default:
		    return 0;
	}
	while(tmp) {
		switch(tmp->type) {
			case XML_COMMENT_NODE:
			case XML_PI_NODE:
			case XML_CDATA_SECTION_NODE:
			case XML_TEXT_NODE:
			    string = tmp->content;
			    break;
			case XML_NAMESPACE_DECL:
			    string = reinterpret_cast<xmlNs *>(tmp)->href;
			    break;
			default:
			    break;
		}
		if(string && (string[0] != 0)) {
			if(len == 1) {
				return (ret + (((uint)string[0]) << 8));
			}
			if(string[1] == 0) {
				len = 1;
				ret = (uint)string[0];
			}
			else {
				return (((uint)string[0]) + (((uint)string[1]) << 8));
			}
		}
		/*
		 * Skip to next node
		 */
		if(tmp->children && (tmp->type != XML_DTD_NODE)) {
			if(tmp->children->type != XML_ENTITY_DECL) {
				tmp = tmp->children;
				continue;
			}
		}
		if(tmp == pNode)
			break;
		if(tmp->next) {
			tmp = tmp->next;
			continue;
		}
		do {
			tmp = tmp->P_ParentNode;
			if(!tmp)
				break;
			if(tmp == pNode) {
				tmp = NULL;
				break;
			}
			if(tmp->next) {
				tmp = tmp->next;
				break;
			}
		} while(tmp);
	}
	return ret;
}
/**
 * xmlXPathStringHash:
 * @string:  a string
 *
 * Function computing the beginning of the string value of the node,
 * used to speed up comparisons
 *
 * Returns an int usable as a hash
 */
static uint xmlXPathStringHash(const xmlChar * string)
{
	if(string == NULL)
		return 0U;
	if(string[0] == 0)
		return 0;
	return (((uint)string[0]) + (((uint)string[1]) << 8));
}
/**
 * xmlXPathCompareNodeSetFloat:
 * @ctxt:  the XPath Parser context
 * @inf:  less than (1) or greater than (0)
 * @strict:  is the comparison strict
 * @arg:  the node set
 * @f:  the value
 *
 * Implement the compare operation between a nodeset and a number
 *   @ns < @val    (1, 1, ...
 *   @ns <= @val   (1, 0, ...
 *   @ns > @val    (0, 1, ...
 *   @ns >= @val   (0, 0, ...
 *
 * If one object to be compared is a node-set and the other is a number,
 * then the comparison will be true if and only if there is a node in the
 * node-set such that the result of performing the comparison on the number
 * to be compared and on the result of converting the string-value of that
 * node to a number using the number function is true.
 *
 * Returns 0 or 1 depending on the results of the test.
 */
static int xmlXPathCompareNodeSetFloat(xmlXPathParserContext * ctxt, int inf, int strict, xmlXPathObject * arg, xmlXPathObject * f)
{
	int i, ret = 0;
	xmlNodeSet * ns;
	xmlChar * str2;
	if(!f || !arg || ((arg->type != XPATH_NODESET) && (arg->type != XPATH_XSLT_TREE))) {
		xmlXPathReleaseObject(ctxt->context, arg);
		xmlXPathReleaseObject(ctxt->context, f);
		return 0;
	}
	ns = arg->nodesetval;
	if(ns) {
		for(i = 0; i < ns->nodeNr; i++) {
			str2 = xmlXPathCastNodeToString(ns->PP_NodeTab[i]);
			if(str2) {
				valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, str2));
				SAlloc::F(str2);
				xmlXPathNumberFunction(ctxt, 1);
				valuePush(ctxt, xmlXPathCacheObjectCopy(ctxt->context, f));
				ret = xmlXPathCompareValues(ctxt, inf, strict);
				if(ret)
					break;
			}
		}
	}
	xmlXPathReleaseObject(ctxt->context, arg);
	xmlXPathReleaseObject(ctxt->context, f);
	return ret;
}
/**
 * xmlXPathCompareNodeSetString:
 * @ctxt:  the XPath Parser context
 * @inf:  less than (1) or greater than (0)
 * @strict:  is the comparison strict
 * @arg:  the node set
 * @s:  the value
 *
 * Implement the compare operation between a nodeset and a string
 *   @ns < @val    (1, 1, ...
 *   @ns <= @val   (1, 0, ...
 *   @ns > @val    (0, 1, ...
 *   @ns >= @val   (0, 0, ...
 *
 * If one object to be compared is a node-set and the other is a string,
 * then the comparison will be true if and only if there is a node in
 * the node-set such that the result of performing the comparison on the
 * string-value of the node and the other string is true.
 *
 * Returns 0 or 1 depending on the results of the test.
 */
static int xmlXPathCompareNodeSetString(xmlXPathParserContext * ctxt, int inf, int strict, xmlXPathObject * arg, xmlXPathObject * s)
{
	int i, ret = 0;
	xmlNodeSet * ns;
	xmlChar * str2;
	if(!s || !arg || ((arg->type != XPATH_NODESET) && (arg->type != XPATH_XSLT_TREE))) {
		xmlXPathReleaseObject(ctxt->context, arg);
		xmlXPathReleaseObject(ctxt->context, s);
		return 0;
	}
	ns = arg->nodesetval;
	if(ns) {
		for(i = 0; i < ns->nodeNr; i++) {
			str2 = xmlXPathCastNodeToString(ns->PP_NodeTab[i]);
			if(str2) {
				valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, str2));
				SAlloc::F(str2);
				valuePush(ctxt, xmlXPathCacheObjectCopy(ctxt->context, s));
				ret = xmlXPathCompareValues(ctxt, inf, strict);
				if(ret)
					break;
			}
		}
	}
	xmlXPathReleaseObject(ctxt->context, arg);
	xmlXPathReleaseObject(ctxt->context, s);
	return ret;
}

/**
 * xmlXPathCompareNodeSets:
 * @inf:  less than (1) or greater than (0)
 * @strict:  is the comparison strict
 * @arg1:  the first node set object
 * @arg2:  the second node set object
 *
 * Implement the compare operation on nodesets:
 *
 * If both objects to be compared are node-sets, then the comparison
 * will be true if and only if there is a node in the first node-set
 * and a node in the second node-set such that the result of performing
 * the comparison on the string-values of the two nodes is true.
 * ....
 * When neither object to be compared is a node-set and the operator
 * is <=, <, >= or >, then the objects are compared by converting both
 * objects to numbers and comparing the numbers according to IEEE 754.
 * ....
 * The number function converts its argument to a number as follows:
 *  - a string that consists of optional whitespace followed by an
 *  optional minus sign followed by a Number followed by whitespace
 *  is converted to the IEEE 754 number that is nearest (according
 *  to the IEEE 754 round-to-nearest rule) to the mathematical value
 *  represented by the string; any other string is converted to NaN
 *
 * Conclusion all nodes need to be converted first to their string value
 * and then the comparison must be done when possible
 */
static int xmlXPathCompareNodeSets(int inf, int strict, xmlXPathObject * arg1, xmlXPathObject * arg2)
{
	int i, j, init = 0;
	double val1;
	double * values2;
	int ret = 0;
	xmlNodeSet * ns1;
	xmlNodeSet * ns2;
	if(!arg1 || ((arg1->type != XPATH_NODESET) && (arg1->type != XPATH_XSLT_TREE))) {
		xmlXPathFreeObject(arg2);
		return 0;
	}
	if(!arg2 || ((arg2->type != XPATH_NODESET) && (arg2->type != XPATH_XSLT_TREE))) {
		xmlXPathFreeObject(arg1);
		xmlXPathFreeObject(arg2);
		return 0;
	}
	ns1 = arg1->nodesetval;
	ns2 = arg2->nodesetval;
	if(!ns1 || (ns1->nodeNr <= 0)) {
		xmlXPathFreeObject(arg1);
		xmlXPathFreeObject(arg2);
		return 0;
	}
	if(!ns2 || (ns2->nodeNr <= 0)) {
		xmlXPathFreeObject(arg1);
		xmlXPathFreeObject(arg2);
		return 0;
	}
	values2 = (double *)SAlloc::M(ns2->nodeNr * sizeof(double));
	if(!values2) {
		xmlXPathErrMemory(NULL, "comparing nodesets");
		xmlXPathFreeObject(arg1);
		xmlXPathFreeObject(arg2);
		return 0;
	}
	for(i = 0; i < ns1->nodeNr; i++) {
		val1 = xmlXPathCastNodeToNumber(ns1->PP_NodeTab[i]);
		if(!fisnan(val1)) {
			for(j = 0; j < ns2->nodeNr; j++) {
				if(init == 0) {
					values2[j] = xmlXPathCastNodeToNumber(ns2->PP_NodeTab[j]);
				}
				if(fisnan(values2[j]))
					continue;
				if(inf && strict)
					ret = (val1 < values2[j]);
				else if(inf && !strict)
					ret = (val1 <= values2[j]);
				else if(!inf && strict)
					ret = (val1 > values2[j]);
				else if(!inf && !strict)
					ret = (val1 >= values2[j]);
				if(ret)
					break;
			}
			if(ret)
				break;
			init = 1;
		}
	}
	SAlloc::F(values2);
	xmlXPathFreeObject(arg1);
	xmlXPathFreeObject(arg2);
	return ret;
}

/**
 * xmlXPathCompareNodeSetValue:
 * @ctxt:  the XPath Parser context
 * @inf:  less than (1) or greater than (0)
 * @strict:  is the comparison strict
 * @arg:  the node set
 * @val:  the value
 *
 * Implement the compare operation between a nodeset and a value
 *   @ns < @val    (1, 1, ...
 *   @ns <= @val   (1, 0, ...
 *   @ns > @val    (0, 1, ...
 *   @ns >= @val   (0, 0, ...
 *
 * If one object to be compared is a node-set and the other is a boolean,
 * then the comparison will be true if and only if the result of performing
 * the comparison on the boolean and on the result of converting
 * the node-set to a boolean using the boolean function is true.
 *
 * Returns 0 or 1 depending on the results of the test.
 */
static int xmlXPathCompareNodeSetValue(xmlXPathParserContext * ctxt, int inf, int strict, xmlXPathObject * arg, xmlXPathObject * val)
{
	if((val == NULL) || (arg == NULL) ||
	    ((arg->type != XPATH_NODESET) && (arg->type != XPATH_XSLT_TREE)))
		return 0;

	switch(val->type) {
		case XPATH_NUMBER:
		    return (xmlXPathCompareNodeSetFloat(ctxt, inf, strict, arg, val));
		case XPATH_NODESET:
		case XPATH_XSLT_TREE:
		    return (xmlXPathCompareNodeSets(inf, strict, arg, val));
		case XPATH_STRING:
		    return (xmlXPathCompareNodeSetString(ctxt, inf, strict, arg, val));
		case XPATH_BOOLEAN:
		    valuePush(ctxt, arg);
		    xmlXPathBooleanFunction(ctxt, 1);
		    valuePush(ctxt, val);
		    return (xmlXPathCompareValues(ctxt, inf, strict));
		default:
		    TODO
	}
	return 0;
}

/**
 * xmlXPathEqualNodeSetString:
 * @arg:  the nodeset object argument
 * @str:  the string to compare to.
 * @neq:  flag to show whether for '=' (0) or '!=' (1)
 *
 * Implement the equal operation on XPath objects content: @arg1 == @arg2
 * If one object to be compared is a node-set and the other is a string,
 * then the comparison will be true if and only if there is a node in
 * the node-set such that the result of performing the comparison on the
 * string-value of the node and the other string is true.
 *
 * Returns 0 or 1 depending on the results of the test.
 */
static int xmlXPathEqualNodeSetString(xmlXPathObject * arg, const xmlChar * str, int neq)
{
	int i;
	xmlNodeSet * ns;
	xmlChar * str2;
	uint hash;

	if((str == NULL) || (arg == NULL) ||
	    ((arg->type != XPATH_NODESET) && (arg->type != XPATH_XSLT_TREE)))
		return 0;
	ns = arg->nodesetval;
	/*
	 * A NULL nodeset compared with a string is always false
	 * (since there is no node equal, and no node not equal)
	 */
	if((ns == NULL) || (ns->nodeNr <= 0) )
		return 0;
	hash = xmlXPathStringHash(str);
	for(i = 0; i < ns->nodeNr; i++) {
		if(xmlXPathNodeValHash(ns->PP_NodeTab[i]) == hash) {
			str2 = xmlNodeGetContent(ns->PP_NodeTab[i]);
			if(str2 && (sstreq(str, str2))) {
				SAlloc::F(str2);
				if(neq)
					continue;
				return 1;
			}
			else if((str2 == NULL) && (sstreq(str, ""))) {
				if(neq)
					continue;
				return 1;
			}
			else if(neq) {
				SAlloc::F(str2);
				return 1;
			}
			SAlloc::F(str2);
		}
		else if(neq)
			return 1;
	}
	return 0;
}

/**
 * xmlXPathEqualNodeSetFloat:
 * @arg:  the nodeset object argument
 * @f:  the float to compare to
 * @neq:  flag to show whether to compare '=' (0) or '!=' (1)
 *
 * Implement the equal operation on XPath objects content: @arg1 == @arg2
 * If one object to be compared is a node-set and the other is a number,
 * then the comparison will be true if and only if there is a node in
 * the node-set such that the result of performing the comparison on the
 * number to be compared and on the result of converting the string-value
 * of that node to a number using the number function is true.
 *
 * Returns 0 or 1 depending on the results of the test.
 */
static int xmlXPathEqualNodeSetFloat(xmlXPathParserContext * ctxt, xmlXPathObject * arg, double f, int neq) 
{
	int i, ret = 0;
	xmlNodeSet * ns;
	xmlChar * str2;
	xmlXPathObject * val;
	double v;
	if((arg == NULL) ||((arg->type != XPATH_NODESET) && (arg->type != XPATH_XSLT_TREE)))
		return 0;
	ns = arg->nodesetval;
	if(ns) {
		for(i = 0; i<ns->nodeNr; i++) {
			str2 = xmlXPathCastNodeToString(ns->PP_NodeTab[i]);
			if(str2) {
				valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, str2));
				SAlloc::F(str2);
				xmlXPathNumberFunction(ctxt, 1);
				val = valuePop(ctxt);
				v = val->floatval;
				xmlXPathReleaseObject(ctxt->context, val);
				if(!fisnan(v)) {
					if((!neq) && (v==f)) {
						ret = 1;
						break;
					}
					else if((neq) && (v!=f)) {
						ret = 1;
						break;
					}
				}
				else { /* NaN is unequal to any value */
					if(neq)
						ret = 1;
				}
			}
		}
	}

	return ret;
}

/**
 * xmlXPathEqualNodeSets:
 * @arg1:  first nodeset object argument
 * @arg2:  second nodeset object argument
 * @neq:   flag to show whether to test '=' (0) or '!=' (1)
 *
 * Implement the equal / not equal operation on XPath nodesets:
 * @arg1 == @arg2  or  @arg1 != @arg2
 * If both objects to be compared are node-sets, then the comparison
 * will be true if and only if there is a node in the first node-set and
 * a node in the second node-set such that the result of performing the
 * comparison on the string-values of the two nodes is true.
 *
 * (needless to say, this is a costly operation)
 *
 * Returns 0 or 1 depending on the results of the test.
 */
static int xmlXPathEqualNodeSets(xmlXPathObject * arg1, xmlXPathObject * arg2, int neq)
{
	int i, j;
	uint * hashs1;
	uint * hashs2;
	xmlChar ** values1;
	xmlChar ** values2;
	int ret = 0;
	xmlNodeSet * ns1;
	xmlNodeSet * ns2;
	if(!arg1 || ((arg1->type != XPATH_NODESET) && (arg1->type != XPATH_XSLT_TREE)))
		return 0;
	if(!arg2 || ((arg2->type != XPATH_NODESET) && (arg2->type != XPATH_XSLT_TREE)))
		return 0;
	ns1 = arg1->nodesetval;
	ns2 = arg2->nodesetval;
	if(!ns1 || (ns1->nodeNr <= 0))
		return 0;
	if(!ns2 || (ns2->nodeNr <= 0))
		return 0;
	/*
	 * for equal, check if there is a node pertaining to both sets
	 */
	if(neq == 0)
		for(i = 0; i < ns1->nodeNr; i++)
			for(j = 0; j < ns2->nodeNr; j++)
				if(ns1->PP_NodeTab[i] == ns2->PP_NodeTab[j])
					return 1;

	values1 = static_cast<xmlChar **>(SAlloc::M(ns1->nodeNr * sizeof(xmlChar *)));
	if(values1 == NULL) {
		xmlXPathErrMemory(NULL, "comparing nodesets");
		return 0;
	}
	hashs1 = static_cast<uint *>(SAlloc::M(ns1->nodeNr * sizeof(uint)));
	if(hashs1 == NULL) {
		xmlXPathErrMemory(NULL, "comparing nodesets");
		SAlloc::F(values1);
		return 0;
	}
	memzero(values1, ns1->nodeNr * sizeof(xmlChar *));
	values2 = static_cast<xmlChar **>(SAlloc::M(ns2->nodeNr * sizeof(xmlChar *)));
	if(values2 == NULL) {
		xmlXPathErrMemory(NULL, "comparing nodesets");
		SAlloc::F(hashs1);
		SAlloc::F(values1);
		return 0;
	}
	hashs2 = static_cast<uint *>(SAlloc::M(ns2->nodeNr * sizeof(uint)));
	if(hashs2 == NULL) {
		xmlXPathErrMemory(NULL, "comparing nodesets");
		SAlloc::F(hashs1);
		SAlloc::F(values1);
		SAlloc::F(values2);
		return 0;
	}
	memzero(values2, ns2->nodeNr * sizeof(xmlChar *));
	for(i = 0; i < ns1->nodeNr; i++) {
		hashs1[i] = xmlXPathNodeValHash(ns1->PP_NodeTab[i]);
		for(j = 0; j < ns2->nodeNr; j++) {
			if(!i)
				hashs2[j] = xmlXPathNodeValHash(ns2->PP_NodeTab[j]);
			if(hashs1[i] != hashs2[j]) {
				if(neq) {
					ret = 1;
					break;
				}
			}
			else {
				SETIFZ(values1[i], xmlNodeGetContent(ns1->PP_NodeTab[i]));
				SETIFZ(values2[j], xmlNodeGetContent(ns2->PP_NodeTab[j]));
				ret = BIN(sstreq(values1[i], values2[j])) ^ neq;
				if(ret)
					break;
			}
		}
		if(ret)
			break;
	}
	for(i = 0; i < ns1->nodeNr; i++)
		SAlloc::F(values1[i]);
	for(j = 0; j < ns2->nodeNr; j++)
		SAlloc::F(values2[j]);
	SAlloc::F(values1);
	SAlloc::F(values2);
	SAlloc::F(hashs1);
	SAlloc::F(hashs2);
	return ret;
}

static int xmlXPathEqualValuesCommon(xmlXPathParserContext * ctxt, xmlXPathObject * arg1, xmlXPathObject * arg2)
{
	int ret = 0;
	// 
	// At this point we are assured neither arg1 nor arg2
	// is a nodeset, so we can just pick the appropriate routine.
	// 
	switch(arg1->type) {
		case XPATH_UNDEFINED:
#ifdef DEBUG_EXPR
		    xmlGenericError(0, "Equal: undefined\n");
#endif
		    break;
		case XPATH_BOOLEAN:
		    switch(arg2->type) {
			    case XPATH_UNDEFINED:
#ifdef DEBUG_EXPR
				xmlGenericError(0, "Equal: undefined\n");
#endif
				break;
			    case XPATH_BOOLEAN:
#ifdef DEBUG_EXPR
				xmlGenericError(0, "Equal: %d boolean %d \n", arg1->boolval, arg2->boolval);
#endif
				ret = (arg1->boolval == arg2->boolval);
				break;
			    case XPATH_NUMBER:
				ret = (arg1->boolval == xmlXPathCastNumberToBoolean(arg2->floatval));
				break;
			    case XPATH_STRING:
				ret = isempty(arg2->stringval) ? 0 : 1;
				ret = (arg1->boolval == ret);
				break;
			    case XPATH_USERS:
			    case XPATH_POINT:
			    case XPATH_RANGE:
			    case XPATH_LOCATIONSET:
				TODO
				break;
			    case XPATH_NODESET:
			    case XPATH_XSLT_TREE:
				break;
		    }
		    break;
		case XPATH_NUMBER:
		    switch(arg2->type) {
			    case XPATH_UNDEFINED:
#ifdef DEBUG_EXPR
				xmlGenericError(0, "Equal: undefined\n");
#endif
				break;
			    case XPATH_BOOLEAN:
				ret = (arg2->boolval == xmlXPathCastNumberToBoolean(arg1->floatval));
				break;
			    case XPATH_STRING:
				valuePush(ctxt, arg2);
				xmlXPathNumberFunction(ctxt, 1);
				arg2 = valuePop(ctxt);
			    /* no break on purpose */
			    case XPATH_NUMBER:
				/* Hand check NaN and Infinity equalities */
				if(fisnan(arg1->floatval) || fisnan(arg2->floatval)) {
					ret = 0;
				}
				else if(xmlXPathIsInf(arg1->floatval) == 1) {
					if(xmlXPathIsInf(arg2->floatval) == 1)
						ret = 1;
					else
						ret = 0;
				}
				else if(xmlXPathIsInf(arg1->floatval) == -1) {
					if(xmlXPathIsInf(arg2->floatval) == -1)
						ret = 1;
					else
						ret = 0;
				}
				else if(xmlXPathIsInf(arg2->floatval) == 1) {
					if(xmlXPathIsInf(arg1->floatval) == 1)
						ret = 1;
					else
						ret = 0;
				}
				else if(xmlXPathIsInf(arg2->floatval) == -1) {
					if(xmlXPathIsInf(arg1->floatval) == -1)
						ret = 1;
					else
						ret = 0;
				}
				else {
					ret = (arg1->floatval == arg2->floatval);
				}
				break;
			    case XPATH_USERS:
			    case XPATH_POINT:
			    case XPATH_RANGE:
			    case XPATH_LOCATIONSET:
				TODO
				break;
			    case XPATH_NODESET:
			    case XPATH_XSLT_TREE:
				break;
		    }
		    break;
		case XPATH_STRING:
		    switch(arg2->type) {
			    case XPATH_UNDEFINED:
#ifdef DEBUG_EXPR
				xmlGenericError(0, "Equal: undefined\n");
#endif
				break;
			    case XPATH_BOOLEAN:
				if((arg1->stringval == NULL) ||
			    (arg1->stringval[0] == 0)) ret = 0;
				else
					ret = 1;
				ret = (arg2->boolval == ret);
				break;
			    case XPATH_STRING:
				ret = sstreq(arg1->stringval, arg2->stringval);
				break;
			    case XPATH_NUMBER:
				valuePush(ctxt, arg1);
				xmlXPathNumberFunction(ctxt, 1);
				arg1 = valuePop(ctxt);
				/* Hand check NaN and Infinity equalities */
				if(fisnan(arg1->floatval) || fisnan(arg2->floatval)) {
					ret = 0;
				}
				else if(xmlXPathIsInf(arg1->floatval) == 1) {
					if(xmlXPathIsInf(arg2->floatval) == 1)
						ret = 1;
					else
						ret = 0;
				}
				else if(xmlXPathIsInf(arg1->floatval) == -1) {
					if(xmlXPathIsInf(arg2->floatval) == -1)
						ret = 1;
					else
						ret = 0;
				}
				else if(xmlXPathIsInf(arg2->floatval) == 1) {
					if(xmlXPathIsInf(arg1->floatval) == 1)
						ret = 1;
					else
						ret = 0;
				}
				else if(xmlXPathIsInf(arg2->floatval) == -1) {
					if(xmlXPathIsInf(arg1->floatval) == -1)
						ret = 1;
					else
						ret = 0;
				}
				else {
					ret = (arg1->floatval == arg2->floatval);
				}
				break;
			    case XPATH_USERS:
			    case XPATH_POINT:
			    case XPATH_RANGE:
			    case XPATH_LOCATIONSET:
				TODO
				break;
			    case XPATH_NODESET:
			    case XPATH_XSLT_TREE:
				break;
		    }
		    break;
		case XPATH_USERS:
		case XPATH_POINT:
		case XPATH_RANGE:
		case XPATH_LOCATIONSET:
		    TODO
		    break;
		case XPATH_NODESET:
		case XPATH_XSLT_TREE:
		    break;
	}
	xmlXPathReleaseObject(ctxt->context, arg1);
	xmlXPathReleaseObject(ctxt->context, arg2);
	return ret;
}

/**
 * xmlXPathEqualValues:
 * @ctxt:  the XPath Parser context
 *
 * Implement the equal operation on XPath objects content: @arg1 == @arg2
 *
 * Returns 0 or 1 depending on the results of the test.
 */
int xmlXPathEqualValues(xmlXPathParserContext * ctxt) 
{
	xmlXPathObject * arg1;
	xmlXPathObject * arg2;
	xmlXPathObject * argtmp;
	int ret = 0;
	if(!ctxt || !ctxt->context) 
		return 0;
	arg2 = valuePop(ctxt);
	arg1 = valuePop(ctxt);
	if((arg1 == NULL) || (arg2 == NULL)) {
		if(arg1)
			xmlXPathReleaseObject(ctxt->context, arg1);
		else
			xmlXPathReleaseObject(ctxt->context, arg2);
		XP_ERROR0(XPATH_INVALID_OPERAND);
	}
	if(arg1 == arg2) {
#ifdef DEBUG_EXPR
		xmlGenericError(0, "Equal: by pointer\n");
#endif
		xmlXPathFreeObject(arg1);
		return 1;
	}
	/*
	   *If either argument is a nodeset, it's a 'special case'
	 */
	if((arg2->type == XPATH_NODESET) || (arg2->type == XPATH_XSLT_TREE) || (arg1->type == XPATH_NODESET) || (arg1->type == XPATH_XSLT_TREE)) {
		/*
		   *Hack it to assure arg1 is the nodeset
		 */
		if((arg1->type != XPATH_NODESET) && (arg1->type != XPATH_XSLT_TREE)) {
			argtmp = arg2;
			arg2 = arg1;
			arg1 = argtmp;
		}
		switch(arg2->type) {
			case XPATH_UNDEFINED:
#ifdef DEBUG_EXPR
			    xmlGenericError(0, "Equal: undefined\n");
#endif
			    break;
			case XPATH_NODESET:
			case XPATH_XSLT_TREE:
			    ret = xmlXPathEqualNodeSets(arg1, arg2, 0);
			    break;
			case XPATH_BOOLEAN:
			    if((arg1->nodesetval == NULL) || (arg1->nodesetval->nodeNr == 0)) 
					ret = 0;
			    else
				    ret = 1;
			    ret = (ret == arg2->boolval);
			    break;
			case XPATH_NUMBER:
			    ret = xmlXPathEqualNodeSetFloat(ctxt, arg1, arg2->floatval, 0);
			    break;
			case XPATH_STRING:
			    ret = xmlXPathEqualNodeSetString(arg1, arg2->stringval, 0);
			    break;
			case XPATH_USERS:
			case XPATH_POINT:
			case XPATH_RANGE:
			case XPATH_LOCATIONSET:
			    TODO
			    break;
		}
		xmlXPathReleaseObject(ctxt->context, arg1);
		xmlXPathReleaseObject(ctxt->context, arg2);
		return ret;
	}
	return (xmlXPathEqualValuesCommon(ctxt, arg1, arg2));
}

/**
 * xmlXPathNotEqualValues:
 * @ctxt:  the XPath Parser context
 *
 * Implement the equal operation on XPath objects content: @arg1 == @arg2
 *
 * Returns 0 or 1 depending on the results of the test.
 */
int xmlXPathNotEqualValues(xmlXPathParserContext * ctxt) 
{
	xmlXPathObject * arg1;
	xmlXPathObject * arg2;
	xmlXPathObject * argtmp;
	int ret = 0;
	if(!ctxt || !ctxt->context) 
		return 0;
	arg2 = valuePop(ctxt);
	arg1 = valuePop(ctxt);
	if((arg1 == NULL) || (arg2 == NULL)) {
		if(arg1)
			xmlXPathReleaseObject(ctxt->context, arg1);
		else
			xmlXPathReleaseObject(ctxt->context, arg2);
		XP_ERROR0(XPATH_INVALID_OPERAND);
	}
	if(arg1 == arg2) {
#ifdef DEBUG_EXPR
		xmlGenericError(0, "NotEqual: by pointer\n");
#endif
		xmlXPathReleaseObject(ctxt->context, arg1);
		return 0;
	}
	/*
	   *If either argument is a nodeset, it's a 'special case'
	 */
	if((arg2->type == XPATH_NODESET) || (arg2->type == XPATH_XSLT_TREE) || (arg1->type == XPATH_NODESET) || (arg1->type == XPATH_XSLT_TREE)) {
		/*
		   *Hack it to assure arg1 is the nodeset
		 */
		if((arg1->type != XPATH_NODESET) && (arg1->type != XPATH_XSLT_TREE)) {
			argtmp = arg2;
			arg2 = arg1;
			arg1 = argtmp;
		}
		switch(arg2->type) {
			case XPATH_UNDEFINED:
#ifdef DEBUG_EXPR
			    xmlGenericError(0, "NotEqual: undefined\n");
#endif
			    break;
			case XPATH_NODESET:
			case XPATH_XSLT_TREE:
			    ret = xmlXPathEqualNodeSets(arg1, arg2, 1);
			    break;
			case XPATH_BOOLEAN:
			    ret = (!arg1->nodesetval || !arg1->nodesetval->nodeNr) ? 0 : 1;
			    ret = (ret != arg2->boolval);
			    break;
			case XPATH_NUMBER:
			    ret = xmlXPathEqualNodeSetFloat(ctxt, arg1, arg2->floatval, 1);
			    break;
			case XPATH_STRING:
			    ret = xmlXPathEqualNodeSetString(arg1, arg2->stringval, 1);
			    break;
			case XPATH_USERS:
			case XPATH_POINT:
			case XPATH_RANGE:
			case XPATH_LOCATIONSET:
			    TODO
			    break;
		}
		xmlXPathReleaseObject(ctxt->context, arg1);
		xmlXPathReleaseObject(ctxt->context, arg2);
		return ret;
	}
	return (!xmlXPathEqualValuesCommon(ctxt, arg1, arg2));
}

/**
 * xmlXPathCompareValues:
 * @ctxt:  the XPath Parser context
 * @inf:  less than (1) or greater than (0)
 * @strict:  is the comparison strict
 *
 * Implement the compare operation on XPath objects:
 *   @arg1 < @arg2    (1, 1, ...
 *   @arg1 <= @arg2   (1, 0, ...
 *   @arg1 > @arg2    (0, 1, ...
 *   @arg1 >= @arg2   (0, 0, ...
 *
 * When neither object to be compared is a node-set and the operator is
 * <=, <, >=, >, then the objects are compared by converted both objects
 * to numbers and comparing the numbers according to IEEE 754. The <
 * comparison will be true if and only if the first number is less than the
 * second number. The <= comparison will be true if and only if the first
 * number is less than or equal to the second number. The > comparison
 * will be true if and only if the first number is greater than the second
 * number. The >= comparison will be true if and only if the first number
 * is greater than or equal to the second number.
 *
 * Returns 1 if the comparison succeeded, 0 if it failed
 */
int xmlXPathCompareValues(xmlXPathParserContext * ctxt, int inf, int strict) 
{
	int ret = 0, arg1i = 0, arg2i = 0;
	xmlXPathObject * arg1;
	xmlXPathObject * arg2;
	if(!ctxt || !ctxt->context) 
		return 0;
	arg2 = valuePop(ctxt);
	arg1 = valuePop(ctxt);
	if((arg1 == NULL) || (arg2 == NULL)) {
		if(arg1)
			xmlXPathReleaseObject(ctxt->context, arg1);
		else
			xmlXPathReleaseObject(ctxt->context, arg2);
		XP_ERROR0(XPATH_INVALID_OPERAND);
	}

	if((arg2->type == XPATH_NODESET) || (arg2->type == XPATH_XSLT_TREE) || (arg1->type == XPATH_NODESET) || (arg1->type == XPATH_XSLT_TREE)) {
		/*
		 * If either argument is a XPATH_NODESET or XPATH_XSLT_TREE the two arguments
		 * are not freed from within this routine; they will be freed from the
		 * called routine, e.g. xmlXPathCompareNodeSets or xmlXPathCompareNodeSetValue
		 */
		if(((arg2->type == XPATH_NODESET) || (arg2->type == XPATH_XSLT_TREE)) && ((arg1->type == XPATH_NODESET) || (arg1->type == XPATH_XSLT_TREE))) {
			ret = xmlXPathCompareNodeSets(inf, strict, arg1, arg2);
		}
		else {
			if((arg1->type == XPATH_NODESET) || (arg1->type == XPATH_XSLT_TREE)) {
				ret = xmlXPathCompareNodeSetValue(ctxt, inf, strict, arg1, arg2);
			}
			else {
				ret = xmlXPathCompareNodeSetValue(ctxt, !inf, strict, arg2, arg1);
			}
		}
		return ret;
	}
	if(arg1->type != XPATH_NUMBER) {
		valuePush(ctxt, arg1);
		xmlXPathNumberFunction(ctxt, 1);
		arg1 = valuePop(ctxt);
	}
	if(arg1->type != XPATH_NUMBER) {
		xmlXPathFreeObject(arg1);
		xmlXPathFreeObject(arg2);
		XP_ERROR0(XPATH_INVALID_OPERAND);
	}
	if(arg2->type != XPATH_NUMBER) {
		valuePush(ctxt, arg2);
		xmlXPathNumberFunction(ctxt, 1);
		arg2 = valuePop(ctxt);
	}
	if(arg2->type != XPATH_NUMBER) {
		xmlXPathReleaseObject(ctxt->context, arg1);
		xmlXPathReleaseObject(ctxt->context, arg2);
		XP_ERROR0(XPATH_INVALID_OPERAND);
	}
	// 
	// Add tests for infinity and nan => feedback on 3.4 for Inf and NaN
	// 
	// Hand check NaN and Infinity comparisons 
	if(fisnan(arg1->floatval) || fisnan(arg2->floatval)) {
		ret = 0;
	}
	else {
		arg1i = xmlXPathIsInf(arg1->floatval);
		arg2i = xmlXPathIsInf(arg2->floatval);
		if(inf && strict) {
			if((arg1i == -1 && arg2i != -1) || (arg2i == 1 && arg1i != 1)) {
				ret = 1;
			}
			else if(arg1i == 0 && arg2i == 0) {
				ret = (arg1->floatval < arg2->floatval);
			}
			else {
				ret = 0;
			}
		}
		else if(inf && !strict) {
			if(arg1i == -1 || arg2i == 1) {
				ret = 1;
			}
			else if(arg1i == 0 && arg2i == 0) {
				ret = (arg1->floatval <= arg2->floatval);
			}
			else {
				ret = 0;
			}
		}
		else if(!inf && strict) {
			if((arg1i == 1 && arg2i != 1) || (arg2i == -1 && arg1i != -1)) {
				ret = 1;
			}
			else if(arg1i == 0 && arg2i == 0) {
				ret = (arg1->floatval > arg2->floatval);
			}
			else {
				ret = 0;
			}
		}
		else if(!inf && !strict) {
			if(arg1i == 1 || arg2i == -1) {
				ret = 1;
			}
			else if(arg1i == 0 && arg2i == 0) {
				ret = (arg1->floatval >= arg2->floatval);
			}
			else {
				ret = 0;
			}
		}
	}
	xmlXPathReleaseObject(ctxt->context, arg1);
	xmlXPathReleaseObject(ctxt->context, arg2);
	return ret;
}
/**
 * xmlXPathValueFlipSign:
 * @ctxt:  the XPath Parser context
 *
 * Implement the unary - operation on an XPath object
 * The numeric operators convert their operands to numbers as if
 * by calling the number function.
 */
void xmlXPathValueFlipSign(xmlXPathParserContext * ctxt) 
{
	if(ctxt && ctxt->context) {
		CAST_TO_NUMBER;
		CHECK_TYPE(XPATH_NUMBER);
		if(fisnan(ctxt->value->floatval))
			ctxt->value->floatval = xmlXPathNAN;
		else if(xmlXPathIsInf(ctxt->value->floatval) == 1)
			ctxt->value->floatval = xmlXPathNINF;
		else if(xmlXPathIsInf(ctxt->value->floatval) == -1)
			ctxt->value->floatval = xmlXPathPINF;
		else if(ctxt->value->floatval == 0) {
			if(xmlXPathGetSign(ctxt->value->floatval) == 0)
				ctxt->value->floatval = xmlXPathNZERO;
			else
				ctxt->value->floatval = 0;
		}
		else
			ctxt->value->floatval = -ctxt->value->floatval;
	}
}

/**
 * xmlXPathAddValues:
 * @ctxt:  the XPath Parser context
 *
 * Implement the add operation on XPath objects:
 * The numeric operators convert their operands to numbers as if
 * by calling the number function.
 */
void xmlXPathAddValues(xmlXPathParserContext * ctxt) 
{
	double val;
	xmlXPathObject * arg = valuePop(ctxt);
	if(!arg)
		XP_ERROR(XPATH_INVALID_OPERAND);
	val = xmlXPathCastToNumber(arg);
	xmlXPathReleaseObject(ctxt->context, arg);
	CAST_TO_NUMBER;
	CHECK_TYPE(XPATH_NUMBER);
	ctxt->value->floatval += val;
}

/**
 * xmlXPathSubValues:
 * @ctxt:  the XPath Parser context
 *
 * Implement the subtraction operation on XPath objects:
 * The numeric operators convert their operands to numbers as if
 * by calling the number function.
 */
void xmlXPathSubValues(xmlXPathParserContext * ctxt) 
{
	double val;
	xmlXPathObject * arg = valuePop(ctxt);
	if(!arg)
		XP_ERROR(XPATH_INVALID_OPERAND);
	val = xmlXPathCastToNumber(arg);
	xmlXPathReleaseObject(ctxt->context, arg);
	CAST_TO_NUMBER;
	CHECK_TYPE(XPATH_NUMBER);
	ctxt->value->floatval -= val;
}
/**
 * xmlXPathMultValues:
 * @ctxt:  the XPath Parser context
 *
 * Implement the multiply operation on XPath objects:
 * The numeric operators convert their operands to numbers as if
 * by calling the number function.
 */
void xmlXPathMultValues(xmlXPathParserContext * ctxt) 
{
	double val;
	xmlXPathObject * arg = valuePop(ctxt);
	if(!arg)
		XP_ERROR(XPATH_INVALID_OPERAND);
	val = xmlXPathCastToNumber(arg);
	xmlXPathReleaseObject(ctxt->context, arg);
	CAST_TO_NUMBER;
	CHECK_TYPE(XPATH_NUMBER);
	ctxt->value->floatval *= val;
}
/**
 * xmlXPathDivValues:
 * @ctxt:  the XPath Parser context
 *
 * Implement the div operation on XPath objects @arg1 / @arg2:
 * The numeric operators convert their operands to numbers as if
 * by calling the number function.
 */
void xmlXPathDivValues(xmlXPathParserContext * ctxt) 
{
	double val;
	xmlXPathObject * arg = valuePop(ctxt);
	if(!arg)
		XP_ERROR(XPATH_INVALID_OPERAND);
	val = xmlXPathCastToNumber(arg);
	xmlXPathReleaseObject(ctxt->context, arg);
	CAST_TO_NUMBER;
	CHECK_TYPE(XPATH_NUMBER);
	if(fisnan(val) || fisnan(ctxt->value->floatval))
		ctxt->value->floatval = xmlXPathNAN;
	else if(val == 0 && xmlXPathGetSign(val) != 0) {
		if(ctxt->value->floatval == 0)
			ctxt->value->floatval = xmlXPathNAN;
		else if(ctxt->value->floatval > 0)
			ctxt->value->floatval = xmlXPathNINF;
		else if(ctxt->value->floatval < 0)
			ctxt->value->floatval = xmlXPathPINF;
	}
	else if(val == 0) {
		if(ctxt->value->floatval == 0)
			ctxt->value->floatval = xmlXPathNAN;
		else if(ctxt->value->floatval > 0)
			ctxt->value->floatval = xmlXPathPINF;
		else if(ctxt->value->floatval < 0)
			ctxt->value->floatval = xmlXPathNINF;
	}
	else
		ctxt->value->floatval /= val;
}
/**
 * xmlXPathModValues:
 * @ctxt:  the XPath Parser context
 *
 * Implement the mod operation on XPath objects: @arg1 / @arg2
 * The numeric operators convert their operands to numbers as if
 * by calling the number function.
 */
void xmlXPathModValues(xmlXPathParserContext * ctxt) 
{
	double arg1, arg2;
	xmlXPathObject * arg = valuePop(ctxt);
	if(!arg)
		XP_ERROR(XPATH_INVALID_OPERAND);
	arg2 = xmlXPathCastToNumber(arg);
	xmlXPathReleaseObject(ctxt->context, arg);
	CAST_TO_NUMBER;
	CHECK_TYPE(XPATH_NUMBER);
	arg1 = ctxt->value->floatval;
	if(arg2 == 0)
		ctxt->value->floatval = xmlXPathNAN;
	else {
		ctxt->value->floatval = fmod(arg1, arg2);
	}
}
// 
// The traversal functions
// 
/*
 * A traversal function enumerates nodes along an axis.
 * Initially it must be called with NULL, and it indicates
 * termination on the axis by returning NULL.
 */
typedef xmlNode * (*xmlXPathTraversalFunction)(xmlXPathParserContext * ctxt, xmlNode * cur);
/*
 * xmlXPathTraversalFunctionExt:
 * A traversal function enumerates nodes along an axis.
 * Initially it must be called with NULL, and it indicates
 * termination on the axis by returning NULL.
 * The context node of the traversal is specified via @contextNode.
 */
typedef xmlNode * (*xmlXPathTraversalFunctionExt)(xmlNode * cur, xmlNode * contextNode);
/*
 * xmlXPathNodeSetMergeFunction:
 * Used for merging node sets in xmlXPathCollectAndTest().
 */
typedef xmlNodeSet * (*xmlXPathNodeSetMergeFunction)(xmlNodeSet *, xmlNodeSet *, int);
/**
 * xmlXPathNextSelf:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "self" direction
 * The self axis contains just the context node itself
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextSelf(xmlXPathParserContext * ctxt, xmlNode * cur) 
{
	if(!ctxt || !ctxt->context) 
		return 0;
	if(!cur)
		return ctxt->context->P_Node;
	return 0;
}
/**
 * xmlXPathNextChild:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "child" direction
 * The child axis contains the children of the context node in document order.
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextChild(xmlXPathParserContext * ctxt, xmlNode * cur) 
{
	if(!ctxt || !ctxt->context) 
		return 0;
	if(!cur) {
		if(ctxt->context->P_Node == NULL) 
			return 0;
		switch(ctxt->context->P_Node->type) {
			case XML_ELEMENT_NODE:
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			case XML_ENTITY_REF_NODE:
			case XML_ENTITY_NODE:
			case XML_PI_NODE:
			case XML_COMMENT_NODE:
			case XML_NOTATION_NODE:
			case XML_DTD_NODE:
			    return (ctxt->context->P_Node->children);
			case XML_DOCUMENT_NODE:
			case XML_DOCUMENT_TYPE_NODE:
			case XML_DOCUMENT_FRAG_NODE:
			case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
			case XML_DOCB_DOCUMENT_NODE:
#endif
			    return (((xmlDoc *)ctxt->context->P_Node)->children);
			case XML_ELEMENT_DECL:
			case XML_ATTRIBUTE_DECL:
			case XML_ENTITY_DECL:
			case XML_ATTRIBUTE_NODE:
			case XML_NAMESPACE_DECL:
			case XML_XINCLUDE_START:
			case XML_XINCLUDE_END:
			    return 0;
		}
		return 0;
	}
	if(oneof2(cur->type, XML_DOCUMENT_NODE, XML_HTML_DOCUMENT_NODE))
		return 0;
	return (cur->next);
}
/**
 * xmlXPathNextChildElement:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "child" direction and nodes of type element.
 * The child axis contains the children of the context node in document order.
 *
 * Returns the next element following that axis
 */
static xmlNode * xmlXPathNextChildElement(xmlXPathParserContext * ctxt, xmlNode * cur) 
{
	if(!ctxt || !ctxt->context) 
		return 0;
	if(!cur) {
		cur = ctxt->context->P_Node;
		if(!cur) 
			return 0;
		/*
		 * Get the first element child.
		 */
		switch(cur->type) {
			case XML_ELEMENT_NODE:
			case XML_DOCUMENT_FRAG_NODE:
			case XML_ENTITY_REF_NODE: /* URGENT TODO: entify-refs as well? */
			case XML_ENTITY_NODE:
			    cur = cur->children;
			    if(cur) {
				    if(cur->type == XML_ELEMENT_NODE)
					    return cur;
				    do {
					    cur = cur->next;
				    } while(cur && (cur->type != XML_ELEMENT_NODE));
				    return cur;
			    }
			    return 0;
			case XML_DOCUMENT_NODE:
			case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
			case XML_DOCB_DOCUMENT_NODE:
#endif
			    return (xmlDocGetRootElement((xmlDoc *)cur));
			default:
			    return 0;
		}
		return 0;
	}
	/*
	 * Get the next sibling element node.
	 */
	switch(cur->type) {
		case XML_ELEMENT_NODE:
		case XML_TEXT_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_ENTITY_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		case XML_XINCLUDE_END:
		    break;
		/* case XML_DTD_NODE: */ /* URGENT TODO: DTD-node as well? */
		default:
		    return 0;
	}
	if(cur->next) {
		if(cur->next->type == XML_ELEMENT_NODE)
			return (cur->next);
		cur = cur->next;
		do {
			cur = cur->next;
		} while(cur && (cur->type != XML_ELEMENT_NODE));
		return cur;
	}
	return 0;
}

#if 0
/**
 * xmlXPathNextDescendantOrSelfElemParent:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "descendant-or-self" axis.
 * Additionally it returns only nodes which can be parents of
 * element nodes.
 *
 *
 * Returns the next element following that axis
 */
static xmlNode * xmlXPathNextDescendantOrSelfElemParent(xmlNode * cur, xmlNode * contextNode)
{
	if(!cur) {
		if(contextNode == NULL)
			return 0;
		switch(contextNode->type) {
			case XML_ELEMENT_NODE:
			case XML_XINCLUDE_START:
			case XML_DOCUMENT_FRAG_NODE:
			case XML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
			case XML_DOCB_DOCUMENT_NODE:
#endif
			case XML_HTML_DOCUMENT_NODE:
			    return (contextNode);
			default:
			    return 0;
		}
		return 0;
	}
	else {
		xmlNode * start = cur;
		while(cur) {
			switch(cur->type) {
				case XML_ELEMENT_NODE:
				/* @todo OK to have XInclude here? */
				case XML_XINCLUDE_START:
				case XML_DOCUMENT_FRAG_NODE:
				    if(cur != start)
					    return cur;
				    if(cur->children) {
					    cur = cur->children;
					    continue;
				    }
				    break;
				/* Not sure if we need those here. */
				case XML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
				case XML_DOCB_DOCUMENT_NODE:
#endif
				case XML_HTML_DOCUMENT_NODE:
				    if(cur != start)
					    return cur;
				    return (xmlDocGetRootElement((xmlDoc *)cur));
				default:
				    break;
			}

next_sibling:
			if(!cur || (cur == contextNode))
				return 0;
			if(cur->next) {
				cur = cur->next;
			}
			else {
				cur = cur->parent;
				goto next_sibling;
			}
		}
	}
	return 0;
}

#endif

/**
 * xmlXPathNextDescendant:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "descendant" direction
 * the descendant axis contains the descendants of the context node in document
 * order; a descendant is a child or a child of a child and so on.
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextDescendant(xmlXPathParserContext * ctxt, xmlNode * cur)
{
	if(!ctxt || !ctxt->context) return 0;
	if(!cur) {
		if(ctxt->context->P_Node == NULL)
			return 0;
		if((ctxt->context->P_Node->type == XML_ATTRIBUTE_NODE) ||
		    (ctxt->context->P_Node->type == XML_NAMESPACE_DECL))
			return 0;

		if(ctxt->context->P_Node == (xmlNode *)ctxt->context->doc)
			return (ctxt->context->doc->children);
		return (ctxt->context->P_Node->children);
	}

	if(cur->type == XML_NAMESPACE_DECL)
		return 0;
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
	if(cur == ctxt->context->P_Node)
		return 0;
	while(cur->next) {
		cur = cur->next;
		if(!oneof2(cur->type, XML_ENTITY_DECL, XML_DTD_NODE))
			return cur;
	}
	do {
		cur = cur->P_ParentNode;
		if(!cur)
			break;
		if(cur == ctxt->context->P_Node)
			return 0;
		if(cur->next) {
			cur = cur->next;
			return cur;
		}
	} while(cur);
	return cur;
}

/**
 * xmlXPathNextDescendantOrSelf:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "descendant-or-self" direction
 * the descendant-or-self axis contains the context node and the descendants
 * of the context node in document order; thus the context node is the first
 * node on the axis, and the first child of the context node is the second node
 * on the axis
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextDescendantOrSelf(xmlXPathParserContext * ctxt, xmlNode * cur) 
{
	if(!ctxt || !ctxt->context) return 0;
	if(!cur) {
		if(ctxt->context->P_Node == NULL)
			return 0;
		if((ctxt->context->P_Node->type == XML_ATTRIBUTE_NODE) ||
		    (ctxt->context->P_Node->type == XML_NAMESPACE_DECL))
			return 0;
		return (ctxt->context->P_Node);
	}

	return (xmlXPathNextDescendant(ctxt, cur));
}

/**
 * xmlXPathNextParent:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "parent" direction
 * The parent axis contains the parent of the context node, if there is one.
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextParent(xmlXPathParserContext * ctxt, xmlNode * cur) 
{
	if(!ctxt || !ctxt->context) return 0;
	/*
	 * the parent of an attribute or namespace node is the element
	 * to which the attribute or namespace node is attached
	 * Namespace handling !!!
	 */
	if(!cur) {
		if(ctxt->context->P_Node == NULL) return 0;
		switch(ctxt->context->P_Node->type) {
			case XML_ELEMENT_NODE:
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			case XML_ENTITY_REF_NODE:
			case XML_ENTITY_NODE:
			case XML_PI_NODE:
			case XML_COMMENT_NODE:
			case XML_NOTATION_NODE:
			case XML_DTD_NODE:
			case XML_ELEMENT_DECL:
			case XML_ATTRIBUTE_DECL:
			case XML_XINCLUDE_START:
			case XML_XINCLUDE_END:
			case XML_ENTITY_DECL:
			    if(ctxt->context->P_Node->P_ParentNode == NULL)
				    return ((xmlNode *)ctxt->context->doc);
			    if((ctxt->context->P_Node->P_ParentNode->type == XML_ELEMENT_NODE) && ((ctxt->context->P_Node->P_ParentNode->name[0] == ' ') ||
				    (sstreq(ctxt->context->P_Node->P_ParentNode->name, "fake node libxslt"))))
				    return 0;
			    return (ctxt->context->P_Node->P_ParentNode);
			case XML_ATTRIBUTE_NODE: {
			    xmlAttr * att = (xmlAttr *)ctxt->context->P_Node;
			    return (att->parent);
		    }
			case XML_DOCUMENT_NODE:
			case XML_DOCUMENT_TYPE_NODE:
			case XML_DOCUMENT_FRAG_NODE:
			case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
			case XML_DOCB_DOCUMENT_NODE:
#endif
			    return 0;
			case XML_NAMESPACE_DECL: {
			    xmlNs * ns = (xmlNs *)ctxt->context->P_Node;
			    if(ns->next && (ns->next->type != XML_NAMESPACE_DECL))
				    return ((xmlNode *)ns->next);
			    return 0;
		    }
		}
	}
	return 0;
}

/**
 * xmlXPathNextAncestor:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "ancestor" direction
 * the ancestor axis contains the ancestors of the context node; the ancestors
 * of the context node consist of the parent of context node and the parent's
 * parent and so on; the nodes are ordered in reverse document order; thus the
 * parent is the first node on the axis, and the parent's parent is the second
 * node on the axis
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextAncestor(xmlXPathParserContext * ctxt, xmlNode * cur) 
{
	if(!ctxt || !ctxt->context) return 0;
	/*
	 * the parent of an attribute or namespace node is the element
	 * to which the attribute or namespace node is attached
	 * !!!!!!!!!!!!!
	 */
	if(!cur) {
		if(ctxt->context->P_Node == NULL) return 0;
		switch(ctxt->context->P_Node->type) {
			case XML_ELEMENT_NODE:
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			case XML_ENTITY_REF_NODE:
			case XML_ENTITY_NODE:
			case XML_PI_NODE:
			case XML_COMMENT_NODE:
			case XML_DTD_NODE:
			case XML_ELEMENT_DECL:
			case XML_ATTRIBUTE_DECL:
			case XML_ENTITY_DECL:
			case XML_NOTATION_NODE:
			case XML_XINCLUDE_START:
			case XML_XINCLUDE_END:
			    if(ctxt->context->P_Node->P_ParentNode == NULL)
				    return ((xmlNode *)ctxt->context->doc);
			    if((ctxt->context->P_Node->P_ParentNode->type == XML_ELEMENT_NODE) && ((ctxt->context->P_Node->P_ParentNode->name[0] == ' ') ||
				    (sstreq(ctxt->context->P_Node->P_ParentNode->name, "fake node libxslt"))))
				    return 0;
			    return (ctxt->context->P_Node->P_ParentNode);
			case XML_ATTRIBUTE_NODE: {
			    xmlAttr * tmp = (xmlAttr *)ctxt->context->P_Node;
			    return tmp->parent;
		    }
			case XML_DOCUMENT_NODE:
			case XML_DOCUMENT_TYPE_NODE:
			case XML_DOCUMENT_FRAG_NODE:
			case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
			case XML_DOCB_DOCUMENT_NODE:
#endif
			    return 0;
			case XML_NAMESPACE_DECL: {
			    xmlNs * ns = (xmlNs *)ctxt->context->P_Node;
			    if(ns->next && (ns->next->type != XML_NAMESPACE_DECL))
				    return ((xmlNode *)ns->next);
			    /* Bad, how did that namespace end up here ? */
			    return 0;
		    }
		}
		return 0;
	}
	if(cur == ctxt->context->doc->children)
		return ((xmlNode *)ctxt->context->doc);
	if(cur == (xmlNode *)ctxt->context->doc)
		return 0;
	switch(cur->type) {
		case XML_ELEMENT_NODE:
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_ENTITY_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		case XML_NOTATION_NODE:
		case XML_DTD_NODE:
		case XML_ELEMENT_DECL:
		case XML_ATTRIBUTE_DECL:
		case XML_ENTITY_DECL:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
		    if(!cur->P_ParentNode)
			    return 0;
		    if((cur->P_ParentNode->type == XML_ELEMENT_NODE) && ((cur->P_ParentNode->name[0] == ' ') || (sstreq(cur->P_ParentNode->name, "fake node libxslt"))))
			    return 0;
		    return cur->P_ParentNode;
		case XML_ATTRIBUTE_NODE: {
		    xmlAttr * att = (xmlAttr *)ctxt->context->P_Node;
		    return att->parent;
	    }
		case XML_NAMESPACE_DECL: {
		    xmlNs * ns = (xmlNs *)ctxt->context->P_Node;
		    if(ns->next && (ns->next->type != XML_NAMESPACE_DECL))
			    return ((xmlNode *)ns->next);
		    /* Bad, how did that namespace end up here ? */
		    return 0;
	    }
		case XML_DOCUMENT_NODE:
		case XML_DOCUMENT_TYPE_NODE:
		case XML_DOCUMENT_FRAG_NODE:
		case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
		case XML_DOCB_DOCUMENT_NODE:
#endif
		    return 0;
	}
	return 0;
}

/**
 * xmlXPathNextAncestorOrSelf:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "ancestor-or-self" direction
 * he ancestor-or-self axis contains the context node and ancestors of
 * the context node in reverse document order; thus the context node is
 * the first node on the axis, and the context node's parent the second;
 * parent here is defined the same as with the parent axis.
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextAncestorOrSelf(xmlXPathParserContext * ctxt, xmlNode * cur) 
{
	if(!ctxt || !ctxt->context) return 0;
	if(!cur)
		return (ctxt->context->P_Node);
	return (xmlXPathNextAncestor(ctxt, cur));
}

/**
 * xmlXPathNextFollowingSibling:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "following-sibling" direction
 * The following-sibling axis contains the following siblings of the context
 * node in document order.
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextFollowingSibling(xmlXPathParserContext * ctxt, xmlNode * cur) 
{
	if(!ctxt || !ctxt->context) return 0;
	if((ctxt->context->P_Node->type == XML_ATTRIBUTE_NODE) ||
	    (ctxt->context->P_Node->type == XML_NAMESPACE_DECL))
		return 0;
	if(cur == (xmlNode *)ctxt->context->doc)
		return 0;
	if(!cur)
		return (ctxt->context->P_Node->next);
	return (cur->next);
}

/**
 * xmlXPathNextPrecedingSibling:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "preceding-sibling" direction
 * The preceding-sibling axis contains the preceding siblings of the context
 * node in reverse document order; the first preceding sibling is first on the
 * axis; the sibling preceding that node is the second on the axis and so on.
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextPrecedingSibling(xmlXPathParserContext * ctxt, xmlNode * cur)
{
	if(!ctxt || !ctxt->context)
		return 0;
	if(oneof2(ctxt->context->P_Node->type, XML_ATTRIBUTE_NODE, XML_NAMESPACE_DECL))
		return 0;
	if(cur == (xmlNode *)ctxt->context->doc)
		return 0;
	if(!cur)
		return (ctxt->context->P_Node->prev);
	if(cur->prev && (cur->prev->type == XML_DTD_NODE)) {
		cur = cur->prev;
		if(!cur)
			return (ctxt->context->P_Node->prev);
	}
	return (cur->prev);
}

/**
 * xmlXPathNextFollowing:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "following" direction
 * The following axis contains all nodes in the same document as the context
 * node that are after the context node in document order, excluding any
 * descendants and excluding attribute nodes and namespace nodes; the nodes
 * are ordered in document order
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextFollowing(xmlXPathParserContext * ctxt, xmlNode * cur)
{
	if(!ctxt || !ctxt->context) 
		return 0;
	if(cur && (cur->type  != XML_ATTRIBUTE_NODE) && (cur->type != XML_NAMESPACE_DECL) && cur->children)
		return (cur->children);
	if(!cur) {
		cur = ctxt->context->P_Node;
		if(cur->type == XML_NAMESPACE_DECL)
			return 0;
		if(cur->type == XML_ATTRIBUTE_NODE)
			cur = cur->P_ParentNode;
	}
	if(!cur)
		return NULL; /* ERROR */
	if(cur->next)
		return cur->next;
	do {
		cur = cur->P_ParentNode;
		if(!cur) 
			break;
		else if(cur == (xmlNode *)ctxt->context->doc) 
			return 0;
		else if(cur->next) 
			return cur->next;
	} while(cur);
	return cur;
}

/*
 * xmlXPathIsAncestor:
 * @ancestor:  the ancestor node
 * @node:  the current node
 *
 * Check that @ancestor is a @node's ancestor
 *
 * returns 1 if @ancestor is a @node's ancestor, 0 otherwise.
 */
static int xmlXPathIsAncestor(xmlNode * ancestor, xmlNode * P_Node)
{
	if(ancestor && P_Node) {
		if(P_Node->type == XML_NAMESPACE_DECL)
			return 0;
		if(ancestor->type == XML_NAMESPACE_DECL)
			return 0;
		/* nodes need to be in the same document */
		if(ancestor->doc != P_Node->doc) 
			return 0;
		/* avoid searching if ancestor or node is the root node */
		if(ancestor == (xmlNode *)P_Node->doc) 
			return 1;
		if(P_Node == (xmlNode *)ancestor->doc) 
			return 0;
		while(P_Node->P_ParentNode) {
			if(P_Node->P_ParentNode == ancestor)
				return 1;
			P_Node = P_Node->P_ParentNode;
		}
	}
	return 0;
}

/**
 * xmlXPathNextPreceding:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "preceding" direction
 * the preceding axis contains all nodes in the same document as the context
 * node that are before the context node in document order, excluding any
 * ancestors and excluding attribute nodes and namespace nodes; the nodes are
 * ordered in reverse document order
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextPreceding(xmlXPathParserContext * ctxt, xmlNode * cur)
{
	if(!ctxt || !ctxt->context) 
		return 0;
	if(!cur) {
		cur = ctxt->context->P_Node;
		if(cur->type == XML_NAMESPACE_DECL)
			return 0;
		if(cur->type == XML_ATTRIBUTE_NODE)
			return cur->P_ParentNode;
	}
	if(!cur || (cur->type == XML_NAMESPACE_DECL))
		return 0;
	if(cur->prev && (cur->prev->type == XML_DTD_NODE))
		cur = cur->prev;
	do {
		if(cur->prev) {
			for(cur = cur->prev; cur->last; cur = cur->last);
				return (cur);
		}
		cur = cur->P_ParentNode;
		if(!cur)
			return 0;
		if(cur == ctxt->context->doc->children)
			return 0;
	} while(xmlXPathIsAncestor(cur, ctxt->context->P_Node));
	return (cur);
}

/**
 * xmlXPathNextPrecedingInternal:
 * @ctxt:  the XPath Parser context
 * @cur:  the current node in the traversal
 *
 * Traversal function for the "preceding" direction
 * the preceding axis contains all nodes in the same document as the context
 * node that are before the context node in document order, excluding any
 * ancestors and excluding attribute nodes and namespace nodes; the nodes are
 * ordered in reverse document order
 * This is a faster implementation but internal only since it requires a
 * state kept in the parser context: ctxt->ancestor.
 *
 * Returns the next element following that axis
 */
static xmlNode * xmlXPathNextPrecedingInternal(xmlXPathParserContext * ctxt, xmlNode * cur)
{
	if(!ctxt || !ctxt->context) 
		return 0;
	if(!cur) {
		cur = ctxt->context->P_Node;
		if(!cur)
			return 0;
		if(cur->type == XML_NAMESPACE_DECL)
			return 0;
		ctxt->ancestor = cur->P_ParentNode;
	}
	if(cur->type == XML_NAMESPACE_DECL)
		return 0;
	if(cur->prev && (cur->prev->type == XML_DTD_NODE))
		cur = cur->prev;
	while(cur->prev == NULL) {
		cur = cur->P_ParentNode;
		if(!cur)
			return 0;
		else if(cur == ctxt->context->doc->children)
			return 0;
		else if(cur != ctxt->ancestor)
			return (cur);
		else
			ctxt->ancestor = cur->P_ParentNode;
	}
	cur = cur->prev;
	while(cur->last)
		cur = cur->last;
	return (cur);
}

/**
 * xmlXPathNextNamespace:
 * @ctxt:  the XPath Parser context
 * @cur:  the current attribute in the traversal
 *
 * Traversal function for the "namespace" direction
 * the namespace axis contains the namespace nodes of the context node;
 * the order of nodes on this axis is implementation-defined; the axis will
 * be empty unless the context node is an element
 *
 * We keep the XML namespace node at the end of the list.
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextNamespace(xmlXPathParserContext * ctxt, xmlNode * cur)
{
	if(!ctxt || !ctxt->context) 
		return 0;
	if(ctxt->context->P_Node->type != XML_ELEMENT_NODE) 
		return 0;
	if(ctxt->context->tmpNsList == NULL && cur != (xmlNode *)xmlXPathXMLNamespace) {
		SAlloc::F(ctxt->context->tmpNsList);
		ctxt->context->tmpNsList = xmlGetNsList(ctxt->context->doc, ctxt->context->P_Node);
		ctxt->context->tmpNsNr = 0;
		if(ctxt->context->tmpNsList) {
			while(ctxt->context->tmpNsList[ctxt->context->tmpNsNr]) {
				ctxt->context->tmpNsNr++;
			}
		}
		return ((xmlNode *)xmlXPathXMLNamespace);
	}
	if(ctxt->context->tmpNsNr > 0) {
		return (xmlNode *)ctxt->context->tmpNsList[--ctxt->context->tmpNsNr];
	}
	else {
		SAlloc::F(ctxt->context->tmpNsList);
		ctxt->context->tmpNsList = NULL;
		return 0;
	}
}

/**
 * xmlXPathNextAttribute:
 * @ctxt:  the XPath Parser context
 * @cur:  the current attribute in the traversal
 *
 * Traversal function for the "attribute" direction
 * @todo support DTD inherited default attributes
 *
 * Returns the next element following that axis
 */
xmlNode * xmlXPathNextAttribute(xmlXPathParserContext * ctxt, xmlNode * cur) 
{
	if(!ctxt || !ctxt->context) 
		return 0;
	if(ctxt->context->P_Node == NULL)
		return 0;
	if(ctxt->context->P_Node->type != XML_ELEMENT_NODE)
		return 0;
	if(!cur) {
		if(ctxt->context->P_Node == (xmlNode *)ctxt->context->doc)
			return 0;
		return ((xmlNode *)ctxt->context->P_Node->properties);
	}
	return ((xmlNode *)cur->next);
}

/************************************************************************
*									*
*		NodeTest Functions					*
*									*
************************************************************************/

#define IS_FUNCTION                     200

/************************************************************************
*									*
*		Implicit tree core function library			*
*									*
************************************************************************/

/**
 * xmlXPathRoot:
 * @ctxt:  the XPath Parser context
 *
 * Initialize the context to the root of the document
 */
void xmlXPathRoot(xmlXPathParserContext * ctxt) 
{
	if(ctxt && ctxt->context) {
		ctxt->context->P_Node = (xmlNode *)ctxt->context->doc;
		valuePush(ctxt, xmlXPathCacheNewNodeSet(ctxt->context, ctxt->context->P_Node));
	}
}

/************************************************************************
*									*
*		The explicit core function library			*
**http://www.w3.org/Style/XSL/Group/1999/07/xpath-19990705.html#corelib	*
*									*
************************************************************************/

/**
 * xmlXPathLastFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the last() XPath function
 *  number last()
 * The last function returns the number of nodes in the context node list.
 */
void xmlXPathLastFunction(xmlXPathParserContext * ctxt, int nargs) {
	CHECK_ARITY(0);
	if(ctxt->context->contextSize >= 0) {
		valuePush(ctxt, xmlXPathCacheNewFloat(ctxt->context, (double)ctxt->context->contextSize));
#ifdef DEBUG_EXPR
		xmlGenericError(0, "last() : %d\n", ctxt->context->contextSize);
#endif
	}
	else {
		XP_ERROR(XPATH_INVALID_CTXT_SIZE);
	}
}

/**
 * xmlXPathPositionFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the position() XPath function
 *  number position()
 * The position function returns the position of the context node in the
 * context node list. The first position is 1, and so the last position
 * will be equal to last().
 */
void xmlXPathPositionFunction(xmlXPathParserContext * ctxt, int nargs) {
	CHECK_ARITY(0);
	if(ctxt->context->proximityPosition >= 0) {
		valuePush(ctxt, xmlXPathCacheNewFloat(ctxt->context, (double)ctxt->context->proximityPosition));
#ifdef DEBUG_EXPR
		xmlGenericError(0, "position() : %d\n", ctxt->context->proximityPosition);
#endif
	}
	else {
		XP_ERROR(XPATH_INVALID_CTXT_POSITION);
	}
}

/**
 * xmlXPathCountFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the count() XPath function
 *  number count(node-set)
 */
void xmlXPathCountFunction(xmlXPathParserContext * ctxt, int nargs)
{
	xmlXPathObject * cur;
	CHECK_ARITY(1);
	if(!ctxt->value || ((ctxt->value->type != XPATH_NODESET) && (ctxt->value->type != XPATH_XSLT_TREE)))
		XP_ERROR(XPATH_INVALID_TYPE);
	cur = valuePop(ctxt);
	if(!cur || !cur->nodesetval)
		valuePush(ctxt, xmlXPathCacheNewFloat(ctxt->context, (double)0));
	else if(oneof2(cur->type, XPATH_NODESET, XPATH_XSLT_TREE)) {
		valuePush(ctxt, xmlXPathCacheNewFloat(ctxt->context, (double)cur->nodesetval->nodeNr));
	}
	else {
		if((cur->nodesetval->nodeNr != 1) || (cur->nodesetval->PP_NodeTab == NULL)) {
			valuePush(ctxt, xmlXPathCacheNewFloat(ctxt->context, (double)0));
		}
		else {
			int i = 0;
			xmlNode * tmp = cur->nodesetval->PP_NodeTab[0];
			if(tmp && (tmp->type != XML_NAMESPACE_DECL)) {
				tmp = tmp->children;
				while(tmp) {
					tmp = tmp->next;
					i++;
				}
			}
			valuePush(ctxt, xmlXPathCacheNewFloat(ctxt->context, (double)i));
		}
	}
	xmlXPathReleaseObject(ctxt->context, cur);
}

/**
 * xmlXPathGetElementsByIds:
 * @doc:  the document
 * @ids:  a whitespace separated list of IDs
 *
 * Selects elements by their unique ID.
 *
 * Returns a node-set of selected elements.
 */
static xmlNodeSet * xmlXPathGetElementsByIds(xmlDoc * doc, const xmlChar * ids)
{
	xmlNodeSet * ret;
	const xmlChar * cur = ids;
	xmlChar * ID;
	xmlAttr * attr;
	xmlNode * elem = NULL;
	if(ids == NULL)
		return 0;
	ret = xmlXPathNodeSetCreate(NULL);
	if(!ret)
		return ret;
	while(IS_BLANK_CH(*cur)) 
		cur++;
	while(*cur) {
		while((!IS_BLANK_CH(*cur)) && (*cur != 0))
			cur++;
		ID = xmlStrndup(ids, cur - ids);
		if(ID) {
			/*
			 * We used to check the fact that the value passed
			 * was an NCName, but this generated much troubles for
			 * me and Aleksey Sanin, people blatantly violated that
			 * constaint, like Visa3D spec.
			 * if(xmlValidateNCName(ID, 1) == 0)
			 */
			attr = xmlGetID(doc, ID);
			if(attr) {
				if(attr->type == XML_ATTRIBUTE_NODE)
					elem = attr->parent;
				else if(attr->type == XML_ELEMENT_NODE)
					elem = (xmlNode *)attr;
				else
					elem = NULL;
				if(elem)
					xmlXPathNodeSetAdd(ret, elem);
			}
			SAlloc::F(ID);
		}

		while(IS_BLANK_CH(*cur)) cur++;
		ids = cur;
	}
	return ret;
}

/**
 * xmlXPathIdFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the id() XPath function
 *  node-set id(object)
 * The id function selects elements by their unique ID
 * (see [5.2.1 Unique IDs]). When the argument to id is of type node-set,
 * then the result is the union of the result of applying id to the
 * string value of each of the nodes in the argument node-set. When the
 * argument to id is of any other type, the argument is converted to a
 * string as if by a call to the string function; the string is split
 * into a whitespace-separated list of tokens (whitespace is any sequence
 * of characters matching the production S); the result is a node-set
 * containing the elements in the same document as the context node that
 * have a unique ID equal to any of the tokens in the list.
 */
void xmlXPathIdFunction(xmlXPathParserContext * ctxt, int nargs) {
	xmlChar * tokens;
	xmlNodeSet * ret;
	xmlXPathObject * obj;

	CHECK_ARITY(1);
	obj = valuePop(ctxt);
	if(obj == NULL) XP_ERROR(XPATH_INVALID_OPERAND);
	if((obj->type == XPATH_NODESET) || (obj->type == XPATH_XSLT_TREE)) {
		xmlNodeSet * ns;
		int i;
		ret = xmlXPathNodeSetCreate(NULL);
		/*
		 * FIXME -- in an out-of-memory condition this will behave badly.
		 * The solution is not clear -- we already popped an item from
		 * ctxt, so the object is in a corrupt state.
		 */

		if(obj->nodesetval) {
			for(i = 0; i < obj->nodesetval->nodeNr; i++) {
				tokens = xmlXPathCastNodeToString(obj->nodesetval->PP_NodeTab[i]);
				ns = xmlXPathGetElementsByIds(ctxt->context->doc, tokens);
				ret = xmlXPathNodeSetMerge(ret, ns);
				xmlXPathFreeNodeSet(ns);
				SAlloc::F(tokens);
			}
		}
		xmlXPathReleaseObject(ctxt->context, obj);
		valuePush(ctxt, xmlXPathCacheWrapNodeSet(ctxt->context, ret));
		return;
	}
	obj = xmlXPathCacheConvertString(ctxt->context, obj);
	ret = xmlXPathGetElementsByIds(ctxt->context->doc, obj->stringval);
	valuePush(ctxt, xmlXPathCacheWrapNodeSet(ctxt->context, ret));
	xmlXPathReleaseObject(ctxt->context, obj);
	return;
}

/**
 * xmlXPathLocalNameFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the local-name() XPath function
 *  string local-name(node-set?)
 * The local-name function returns a string containing the local part
 * of the name of the node in the argument node-set that is first in
 * document order. If the node-set is empty or the first node has no
 * name, an empty string is returned. If the argument is omitted it
 * defaults to the context node.
 */
void xmlXPathLocalNameFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	xmlXPathObject * cur;
	if(ctxt) {
		if(nargs == 0) {
			valuePush(ctxt, xmlXPathCacheNewNodeSet(ctxt->context, ctxt->context->P_Node));
			nargs = 1;
		}
		CHECK_ARITY(1);
		if(!ctxt->value || ((ctxt->value->type != XPATH_NODESET) && (ctxt->value->type != XPATH_XSLT_TREE)))
			XP_ERROR(XPATH_INVALID_TYPE);
		cur = valuePop(ctxt);
		if((cur->nodesetval == NULL) || (cur->nodesetval->nodeNr == 0)) {
			valuePush(ctxt, xmlXPathCacheNewCString(ctxt->context, ""));
		}
		else {
			int i = 0; /* Should be first in document order !!!!! */
			switch(cur->nodesetval->PP_NodeTab[i]->type) {
				case XML_ELEMENT_NODE:
				case XML_ATTRIBUTE_NODE:
				case XML_PI_NODE:
					if(cur->nodesetval->PP_NodeTab[i]->name[0] == ' ')
						valuePush(ctxt, xmlXPathCacheNewCString(ctxt->context, ""));
					else
						valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, cur->nodesetval->PP_NodeTab[i]->name));
					break;
				case XML_NAMESPACE_DECL:
					valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, ((xmlNs *)cur->nodesetval->PP_NodeTab[i])->prefix));
					break;
				default:
					valuePush(ctxt, xmlXPathCacheNewCString(ctxt->context, ""));
			}
		}
		xmlXPathReleaseObject(ctxt->context, cur);
	}
}

/**
 * xmlXPathNamespaceURIFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the namespace-uri() XPath function
 *  string namespace-uri(node-set?)
 * The namespace-uri function returns a string containing the
 * namespace URI of the expanded name of the node in the argument
 * node-set that is first in document order. If the node-set is empty,
 * the first node has no name, or the expanded name has no namespace
 * URI, an empty string is returned. If the argument is omitted it
 * defaults to the context node.
 */
void xmlXPathNamespaceURIFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	xmlXPathObject * cur;
	if(!ctxt) 
		return;
	if(nargs == 0) {
		valuePush(ctxt, xmlXPathCacheNewNodeSet(ctxt->context, ctxt->context->P_Node));
		nargs = 1;
	}
	CHECK_ARITY(1);
	if(!ctxt->value || ((ctxt->value->type != XPATH_NODESET) && (ctxt->value->type != XPATH_XSLT_TREE)))
		XP_ERROR(XPATH_INVALID_TYPE);
	cur = valuePop(ctxt);
	if((cur->nodesetval == NULL) || (cur->nodesetval->nodeNr == 0)) {
		valuePush(ctxt, xmlXPathCacheNewCString(ctxt->context, ""));
	}
	else {
		int i = 0; /* Should be first in document order !!!!! */
		switch(cur->nodesetval->PP_NodeTab[i]->type) {
			case XML_ELEMENT_NODE:
			case XML_ATTRIBUTE_NODE:
			    if(cur->nodesetval->PP_NodeTab[i]->ns == NULL)
				    valuePush(ctxt, xmlXPathCacheNewCString(ctxt->context, ""));
			    else
				    valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, cur->nodesetval->PP_NodeTab[i]->ns->href));
			    break;
			default:
			    valuePush(ctxt, xmlXPathCacheNewCString(ctxt->context, ""));
		}
	}
	xmlXPathReleaseObject(ctxt->context, cur);
}

/**
 * xmlXPathNameFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the name() XPath function
 *  string name(node-set?)
 * The name function returns a string containing a QName representing
 * the name of the node in the argument node-set that is first in document
 * order. The QName must represent the name with respect to the namespace
 * declarations in effect on the node whose name is being represented.
 * Typically, this will be the form in which the name occurred in the XML
 * source. This need not be the case if there are namespace declarations
 * in effect on the node that associate multiple prefixes with the same
 * namespace. However, an implementation may include information about
 * the original prefix in its representation of nodes; in this case, an
 * implementation can ensure that the returned string is always the same
 * as the QName used in the XML source. If the argument it omitted it
 * defaults to the context node.
 * Libxml keep the original prefix so the "real qualified name" used is
 * returned.
 */
static void xmlXPathNameFunction(xmlXPathParserContext * ctxt, int nargs)
{
	xmlXPathObject * cur;
	if(nargs == 0) {
		valuePush(ctxt, xmlXPathCacheNewNodeSet(ctxt->context, ctxt->context->P_Node));
		nargs = 1;
	}
	CHECK_ARITY(1);
	if(!ctxt->value || ((ctxt->value->type != XPATH_NODESET) && (ctxt->value->type != XPATH_XSLT_TREE)))
		XP_ERROR(XPATH_INVALID_TYPE);
	cur = valuePop(ctxt);
	if((cur->nodesetval == NULL) || (cur->nodesetval->nodeNr == 0)) {
		valuePush(ctxt, xmlXPathCacheNewCString(ctxt->context, ""));
	}
	else {
		int i = 0; /* Should be first in document order !!!!! */
		switch(cur->nodesetval->PP_NodeTab[i]->type) {
			case XML_ELEMENT_NODE:
			case XML_ATTRIBUTE_NODE:
			    if(cur->nodesetval->PP_NodeTab[i]->name[0] == ' ')
				    valuePush(ctxt, xmlXPathCacheNewCString(ctxt->context, ""));
			    else if((cur->nodesetval->PP_NodeTab[i]->ns == NULL) || (cur->nodesetval->PP_NodeTab[i]->ns->prefix == NULL)) {
				    valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, cur->nodesetval->PP_NodeTab[i]->name));
			    }
			    else {
				    xmlChar * fullname = xmlBuildQName(cur->nodesetval->PP_NodeTab[i]->name, cur->nodesetval->PP_NodeTab[i]->ns->prefix, NULL, 0);
				    if(fullname == cur->nodesetval->PP_NodeTab[i]->name)
					    fullname = sstrdup(cur->nodesetval->PP_NodeTab[i]->name);
				    if(fullname == NULL) {
					    XP_ERROR(XPATH_MEMORY_ERROR);
				    }
				    valuePush(ctxt, xmlXPathCacheWrapString(ctxt->context, fullname));
			    }
			    break;
			default:
			    valuePush(ctxt, xmlXPathCacheNewNodeSet(ctxt->context, cur->nodesetval->PP_NodeTab[i]));
			    xmlXPathLocalNameFunction(ctxt, 1);
		}
	}
	xmlXPathReleaseObject(ctxt->context, cur);
}

/**
 * xmlXPathStringFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the string() XPath function
 *  string string(object?)
 * The string function converts an object to a string as follows:
 *  - A node-set is converted to a string by returning the value of
 * the node in the node-set that is first in document order.
 * If the node-set is empty, an empty string is returned.
 *  - A number is converted to a string as follows
 * + NaN is converted to the string NaN
 * + positive zero is converted to the string 0
 * + negative zero is converted to the string 0
 * + positive infinity is converted to the string Infinity
 * + negative infinity is converted to the string -Infinity
 * + if the number is an integer, the number is represented in
 *   decimal form as a Number with no decimal point and no leading
 *   zeros, preceded by a minus sign (-) if the number is negative
 * + otherwise, the number is represented in decimal form as a
 *   Number including a decimal point with at least one digit
 *   before the decimal point and at least one digit after the
 *   decimal point, preceded by a minus sign (-) if the number
 *   is negative; there must be no leading zeros before the decimal
 *   point apart possibly from the one required digit immediately
 *   before the decimal point; beyond the one required digit
 *   after the decimal point there must be as many, but only as
 *   many, more digits as are needed to uniquely distinguish the
 *   number from all other IEEE 754 numeric values.
 *  - The boolean false value is converted to the string false.
 * The boolean true value is converted to the string true.
 *
 * If the argument is omitted, it defaults to a node-set with the
 * context node as its only member.
 */
void xmlXPathStringFunction(xmlXPathParserContext * ctxt, int nargs)
{
	xmlXPathObject * cur;
	if(ctxt) {
		if(nargs == 0) {
			valuePush(ctxt, xmlXPathCacheWrapString(ctxt->context, xmlXPathCastNodeToString(ctxt->context->P_Node)));
		}
		else {
			CHECK_ARITY(1);
			cur = valuePop(ctxt);
			if(!cur) 
				XP_ERROR(XPATH_INVALID_OPERAND);
			valuePush(ctxt, xmlXPathCacheConvertString(ctxt->context, cur));
		}
	}
}

/**
 * xmlXPathStringLengthFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the string-length() XPath function
 *  number string-length(string?)
 * The string-length returns the number of characters in the string
 * (see [3.6 Strings]). If the argument is omitted, it defaults to
 * the context node converted to a string, in other words the value
 * of the context node.
 */
void xmlXPathStringLengthFunction(xmlXPathParserContext * ctxt, int nargs)
{
	xmlXPathObject * cur;
	if(nargs == 0) {
		if(!ctxt || !ctxt->context)
			return;
		if(ctxt->context->P_Node == NULL) {
			valuePush(ctxt, xmlXPathCacheNewFloat(ctxt->context, 0));
		}
		else {
			xmlChar * content = xmlXPathCastNodeToString(ctxt->context->P_Node);
			valuePush(ctxt, xmlXPathCacheNewFloat(ctxt->context, xmlUTF8Strlen(content)));
			SAlloc::F(content);
		}
		return;
	}
	CHECK_ARITY(1);
	CAST_TO_STRING;
	CHECK_TYPE(XPATH_STRING);
	cur = valuePop(ctxt);
	valuePush(ctxt, xmlXPathCacheNewFloat(ctxt->context, xmlUTF8Strlen(cur->stringval)));
	xmlXPathReleaseObject(ctxt->context, cur);
}

/**
 * xmlXPathConcatFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the concat() XPath function
 *  string concat(string, string, string*)
 * The concat function returns the concatenation of its arguments.
 */
void xmlXPathConcatFunction(xmlXPathParserContext * ctxt, int nargs)
{
	xmlXPathObject * cur;
	xmlXPathObject * newobj;
	xmlChar * tmp;
	if(!ctxt)
		return;
	if(nargs < 2) {
		CHECK_ARITY(2);
	}
	CAST_TO_STRING;
	cur = valuePop(ctxt);
	if(!cur || (cur->type != XPATH_STRING)) {
		xmlXPathReleaseObject(ctxt->context, cur);
		return;
	}
	nargs--;

	while(nargs > 0) {
		CAST_TO_STRING;
		newobj = valuePop(ctxt);
		if((newobj == NULL) || (newobj->type != XPATH_STRING)) {
			xmlXPathReleaseObject(ctxt->context, newobj);
			xmlXPathReleaseObject(ctxt->context, cur);
			XP_ERROR(XPATH_INVALID_TYPE);
		}
		tmp = xmlStrcat(newobj->stringval, cur->stringval);
		newobj->stringval = cur->stringval;
		cur->stringval = tmp;
		xmlXPathReleaseObject(ctxt->context, newobj);
		nargs--;
	}
	valuePush(ctxt, cur);
}

/**
 * xmlXPathContainsFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the contains() XPath function
 *  boolean contains(string, string)
 * The contains function returns true if the first argument string
 * contains the second argument string, and otherwise returns false.
 */
void xmlXPathContainsFunction(xmlXPathParserContext * ctxt, int nargs)
{
	xmlXPathObject * hay;
	xmlXPathObject * needle;
	CHECK_ARITY(2);
	CAST_TO_STRING;
	CHECK_TYPE(XPATH_STRING);
	needle = valuePop(ctxt);
	CAST_TO_STRING;
	hay = valuePop(ctxt);
	if((hay == NULL) || (hay->type != XPATH_STRING)) {
		xmlXPathReleaseObject(ctxt->context, hay);
		xmlXPathReleaseObject(ctxt->context, needle);
		XP_ERROR(XPATH_INVALID_TYPE);
	}
	if(xmlStrstr(hay->stringval, needle->stringval))
		valuePush(ctxt, xmlXPathCacheNewBoolean(ctxt->context, 1));
	else
		valuePush(ctxt, xmlXPathCacheNewBoolean(ctxt->context, 0));
	xmlXPathReleaseObject(ctxt->context, hay);
	xmlXPathReleaseObject(ctxt->context, needle);
}

/**
 * xmlXPathStartsWithFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the starts-with() XPath function
 *  boolean starts-with(string, string)
 * The starts-with function returns true if the first argument string
 * starts with the second argument string, and otherwise returns false.
 */
void xmlXPathStartsWithFunction(xmlXPathParserContext * ctxt, int nargs)
{
	xmlXPathObject * hay;
	xmlXPathObject * needle;
	int n;
	CHECK_ARITY(2);
	CAST_TO_STRING;
	CHECK_TYPE(XPATH_STRING);
	needle = valuePop(ctxt);
	CAST_TO_STRING;
	hay = valuePop(ctxt);
	if((hay == NULL) || (hay->type != XPATH_STRING)) {
		xmlXPathReleaseObject(ctxt->context, hay);
		xmlXPathReleaseObject(ctxt->context, needle);
		XP_ERROR(XPATH_INVALID_TYPE);
	}
	n = sstrlen(needle->stringval);
	if(xmlStrncmp(hay->stringval, needle->stringval, n))
		valuePush(ctxt, xmlXPathCacheNewBoolean(ctxt->context, 0));
	else
		valuePush(ctxt, xmlXPathCacheNewBoolean(ctxt->context, 1));
	xmlXPathReleaseObject(ctxt->context, hay);
	xmlXPathReleaseObject(ctxt->context, needle);
}

/**
 * xmlXPathSubstringFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the substring() XPath function
 *  string substring(string, number, number?)
 * The substring function returns the substring of the first argument
 * starting at the position specified in the second argument with
 * length specified in the third argument. For example,
 * substring("12345",2,3) returns "234". If the third argument is not
 * specified, it returns the substring starting at the position specified
 * in the second argument and continuing to the end of the string. For
 * example, substring("12345",2) returns "2345".  More precisely, each
 * character in the string (see [3.6 Strings]) is considered to have a
 * numeric position: the position of the first character is 1, the position
 * of the second character is 2 and so on. The returned substring contains
 * those characters for which the position of the character is greater than
 * or equal to the second argument and, if the third argument is specified,
 * less than the sum of the second and third arguments; the comparisons
 * and addition used for the above follow the standard IEEE 754 rules. Thus:
 *  - substring("12345", 1.5, 2.6) returns "234"
 *  - substring("12345", 0, 3) returns "12"
 *  - substring("12345", 0 div 0, 3) returns ""
 *  - substring("12345", 1, 0 div 0) returns ""
 *  - substring("12345", -42, 1 div 0) returns "12345"
 *  - substring("12345", -1 div 0, 1 div 0) returns ""
 */
void xmlXPathSubstringFunction(xmlXPathParserContext * ctxt, int nargs)
{
	xmlXPathObject * str;
	xmlXPathObject * start;
	xmlXPathObject * len;
	double le = 0, in;
	int i, l, m;
	xmlChar * ret;
	if(nargs < 2) {
		CHECK_ARITY(2);
	}
	if(nargs > 3) {
		CHECK_ARITY(3);
	}
	/*
	 * take care of possible last (position) argument
	 */
	if(nargs == 3) {
		CAST_TO_NUMBER;
		CHECK_TYPE(XPATH_NUMBER);
		len = valuePop(ctxt);
		le = len->floatval;
		xmlXPathReleaseObject(ctxt->context, len);
	}
	CAST_TO_NUMBER;
	CHECK_TYPE(XPATH_NUMBER);
	start = valuePop(ctxt);
	in = start->floatval;
	xmlXPathReleaseObject(ctxt->context, start);
	CAST_TO_STRING;
	CHECK_TYPE(XPATH_STRING);
	str = valuePop(ctxt);
	m = xmlUTF8Strlen((const uchar *)str->stringval);
	/*
	 * If last pos not present, calculate last position
	 */
	if(nargs != 3) {
		le = (double)m;
		if(in < 1.0)
			in = 1.0;
	}
	/* Need to check for the special cases where either
	 * the index is NaN, the length is NaN, or both
	 * arguments are infinity (relying on Inf + -Inf = NaN)
	 */
	if(!xmlXPathIsInf(in) && !fisnan(in + le)) {
		/*
		 * To meet the requirements of the spec, the arguments
		 * must be converted to integer format before
		 * initial index calculations are done
		 *
		 * First we go to integer form, rounding up
		 * and checking for special cases
		 */
		i = (int)in;
		if(((double)i)+0.5 <= in) 
			i++;
		if(xmlXPathIsInf(le) == 1) {
			l = m;
			if(i < 1)
				i = 1;
		}
		else if(xmlXPathIsInf(le) == -1 || le < 0.0)
			l = 0;
		else {
			l = (int)le;
			if(((double)l)+0.5 <= le) l++;
		}

		/* Now we normalize inidices */
		i -= 1;
		l += i;
		if(i < 0)
			i = 0;
		if(l > m)
			l = m;

		/* number of chars to copy */
		l -= i;

		ret = xmlUTF8Strsub(str->stringval, i, l);
	}
	else {
		ret = NULL;
	}
	if(!ret)
		valuePush(ctxt, xmlXPathCacheNewCString(ctxt->context, ""));
	else {
		valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, ret));
		SAlloc::F(ret);
	}
	xmlXPathReleaseObject(ctxt->context, str);
}

/**
 * xmlXPathSubstringBeforeFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the substring-before() XPath function
 *  string substring-before(string, string)
 * The substring-before function returns the substring of the first
 * argument string that precedes the first occurrence of the second
 * argument string in the first argument string, or the empty string
 * if the first argument string does not contain the second argument
 * string. For example, substring-before("1999/04/01","/") returns 1999.
 */
void xmlXPathSubstringBeforeFunction(xmlXPathParserContext * ctxt, int nargs)
{
	xmlXPathObject * str;
	xmlXPathObject * find;
	xmlBuf * target;
	const xmlChar * point;
	int offset;
	CHECK_ARITY(2);
	CAST_TO_STRING;
	find = valuePop(ctxt);
	CAST_TO_STRING;
	str = valuePop(ctxt);
	target = xmlBufCreate();
	if(target) {
		point = xmlStrstr(str->stringval, find->stringval);
		if(point) {
			offset = (int)(point - str->stringval);
			xmlBufAdd(target, str->stringval, offset);
		}
		valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, xmlBufContent(target)));
		xmlBufFree(target);
	}
	xmlXPathReleaseObject(ctxt->context, str);
	xmlXPathReleaseObject(ctxt->context, find);
}

/**
 * xmlXPathSubstringAfterFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the substring-after() XPath function
 *  string substring-after(string, string)
 * The substring-after function returns the substring of the first
 * argument string that follows the first occurrence of the second
 * argument string in the first argument string, or the empty stringi
 * if the first argument string does not contain the second argument
 * string. For example, substring-after("1999/04/01","/") returns 04/01,
 * and substring-after("1999/04/01","19") returns 99/04/01.
 */
void xmlXPathSubstringAfterFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	xmlXPathObject * str;
	xmlXPathObject * find;
	xmlBuf * target;
	const xmlChar * point;
	int offset;
	CHECK_ARITY(2);
	CAST_TO_STRING;
	find = valuePop(ctxt);
	CAST_TO_STRING;
	str = valuePop(ctxt);
	target = xmlBufCreate();
	if(target) {
		point = xmlStrstr(str->stringval, find->stringval);
		if(point) {
			offset = (int)(point - str->stringval) + sstrlen(find->stringval);
			xmlBufAdd(target, &str->stringval[offset], sstrlen(str->stringval) - offset);
		}
		valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, xmlBufContent(target)));
		xmlBufFree(target);
	}
	xmlXPathReleaseObject(ctxt->context, str);
	xmlXPathReleaseObject(ctxt->context, find);
}
/**
 * xmlXPathNormalizeFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the normalize-space() XPath function
 *  string normalize-space(string?)
 * The normalize-space function returns the argument string with white
 * space normalized by stripping leading and trailing whitespace
 * and replacing sequences of whitespace characters by a single
 * space. Whitespace characters are the same allowed by the S production
 * in XML. If the argument is omitted, it defaults to the context
 * node converted to a string, in other words the value of the context node.
 */
void xmlXPathNormalizeFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	xmlXPathObject * obj = NULL;
	xmlChar * source = NULL;
	xmlBuf * target;
	xmlChar blank;
	if(ctxt) {
		if(nargs == 0) {
			/* Use current context node */
			valuePush(ctxt, xmlXPathCacheWrapString(ctxt->context, xmlXPathCastNodeToString(ctxt->context->P_Node)));
			nargs = 1;
		}
		CHECK_ARITY(1);
		CAST_TO_STRING;
		CHECK_TYPE(XPATH_STRING);
		obj = valuePop(ctxt);
		source = obj->stringval;
		target = xmlBufCreate();
		if(target && source) {
			/* Skip leading whitespaces */
			while(IS_BLANK_CH(*source))
				source++;
			/* Collapse intermediate whitespaces, and skip trailing whitespaces */
			blank = 0;
			while(*source) {
				if(IS_BLANK_CH(*source)) {
					blank = 0x20;
				}
				else {
					if(blank) {
						xmlBufAdd(target, &blank, 1);
						blank = 0;
					}
					xmlBufAdd(target, source, 1);
				}
				source++;
			}
			valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, xmlBufContent(target)));
			xmlBufFree(target);
		}
		xmlXPathReleaseObject(ctxt->context, obj);
	}
}

/**
 * xmlXPathTranslateFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the translate() XPath function
 *  string translate(string, string, string)
 * The translate function returns the first argument string with
 * occurrences of characters in the second argument string replaced
 * by the character at the corresponding position in the third argument
 * string. For example, translate("bar","abc","ABC") returns the string
 * BAr. If there is a character in the second argument string with no
 * character at a corresponding position in the third argument string
 * (because the second argument string is longer than the third argument
 * string), then occurrences of that character in the first argument
 * string are removed. For example, translate("--aaa--","abc-","ABC")
 * returns "AAA". If a character occurs more than once in second
 * argument string, then the first occurrence determines the replacement
 * character. If the third argument string is longer than the second
 * argument string, then excess characters are ignored.
 */
void xmlXPathTranslateFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	xmlXPathObject * str;
	xmlXPathObject * from;
	xmlXPathObject * to;
	xmlBuf * target;
	int offset, max;
	xmlChar ch;
	const xmlChar * point;
	xmlChar * cptr;
	CHECK_ARITY(3);
	CAST_TO_STRING;
	to = valuePop(ctxt);
	CAST_TO_STRING;
	from = valuePop(ctxt);
	CAST_TO_STRING;
	str = valuePop(ctxt);
	target = xmlBufCreate();
	if(target) {
		max = xmlUTF8Strlen(to->stringval);
		for(cptr = str->stringval; (ch = *cptr);) {
			offset = xmlUTF8Strloc(from->stringval, cptr);
			if(offset >= 0) {
				if(offset < max) {
					point = xmlUTF8Strpos(to->stringval, offset);
					if(point)
						xmlBufAdd(target, point, xmlUTF8Strsize(point, 1));
				}
			}
			else
				xmlBufAdd(target, cptr, xmlUTF8Strsize(cptr, 1));

			/* Step to next character in input */
			cptr++;
			if(ch & 0x80) {
				/* if not simple ascii, verify proper format */
				if((ch & 0xc0) != 0xc0) {
					xmlGenericError(0, "xmlXPathTranslateFunction: Invalid UTF8 string\n");
					/* not asserting an XPath error is probably better */
					break;
				}
				/* then skip over remaining bytes for this char */
				while((ch <<= 1) & 0x80)
					if((*cptr++ & 0xc0) != 0x80) {
						xmlGenericError(0, "xmlXPathTranslateFunction: Invalid UTF8 string\n");
						/* not asserting an XPath error is probably better */
						break;
					}
				if(ch & 0x80) /* must have had error encountered */
					break;
			}
		}
	}
	valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, xmlBufContent(target)));
	xmlBufFree(target);
	xmlXPathReleaseObject(ctxt->context, str);
	xmlXPathReleaseObject(ctxt->context, from);
	xmlXPathReleaseObject(ctxt->context, to);
}

/**
 * xmlXPathBooleanFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the boolean() XPath function
 *  boolean boolean(object)
 * The boolean function converts its argument to a boolean as follows:
 *  - a number is true if and only if it is neither positive or
 * negative zero nor NaN
 *  - a node-set is true if and only if it is non-empty
 *  - a string is true if and only if its length is non-zero
 */
void xmlXPathBooleanFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	xmlXPathObject * cur;
	CHECK_ARITY(1);
	cur = valuePop(ctxt);
	if(!cur) 
		XP_ERROR(XPATH_INVALID_OPERAND);
	cur = xmlXPathCacheConvertBoolean(ctxt->context, cur);
	valuePush(ctxt, cur);
}
/**
 * xmlXPathNotFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the not() XPath function
 *  boolean not(boolean)
 * The not function returns true if its argument is false,
 * and false otherwise.
 */
void xmlXPathNotFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	CHECK_ARITY(1);
	CAST_TO_BOOLEAN;
	CHECK_TYPE(XPATH_BOOLEAN);
	ctxt->value->boolval = !ctxt->value->boolval;
}

/**
 * xmlXPathTrueFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the true() XPath function
 *  boolean true()
 */
void xmlXPathTrueFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	CHECK_ARITY(0);
	valuePush(ctxt, xmlXPathCacheNewBoolean(ctxt->context, 1));
}

/**
 * xmlXPathFalseFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the false() XPath function
 *  boolean false()
 */
void xmlXPathFalseFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	CHECK_ARITY(0);
	valuePush(ctxt, xmlXPathCacheNewBoolean(ctxt->context, 0));
}

/**
 * xmlXPathLangFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the lang() XPath function
 *  boolean lang(string)
 * The lang function returns true or false depending on whether the
 * language of the context node as specified by xml:lang attributes
 * is the same as or is a sublanguage of the language specified by
 * the argument string. The language of the context node is determined
 * by the value of the xml:lang attribute on the context node, or, if
 * the context node has no xml:lang attribute, by the value of the
 * xml:lang attribute on the nearest ancestor of the context node that
 * has an xml:lang attribute. If there is no such attribute, then lang
 * returns false. If there is such an attribute, then lang returns
 * true if the attribute value is equal to the argument ignoring case,
 * or if there is some suffix starting with - such that the attribute
 * value is equal to the argument ignoring that suffix of the attribute
 * value and ignoring case.
 */
void xmlXPathLangFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	xmlXPathObject * val = NULL;
	const xmlChar * theLang = NULL;
	const xmlChar * lang;
	int ret = 0;
	int i;
	CHECK_ARITY(1);
	CAST_TO_STRING;
	CHECK_TYPE(XPATH_STRING);
	val = valuePop(ctxt);
	lang = val->stringval;
	theLang = xmlNodeGetLang(ctxt->context->P_Node);
	if(theLang && lang) {
		for(i = 0; lang[i] != 0; i++)
			if(toupper(lang[i]) != toupper(theLang[i]))
				goto not_equal;
		if((theLang[i] == 0) || (theLang[i] == '-'))
			ret = 1;
	}
not_equal:
	SAlloc::F((void *)theLang);
	xmlXPathReleaseObject(ctxt->context, val);
	valuePush(ctxt, xmlXPathCacheNewBoolean(ctxt->context, ret));
}

/**
 * xmlXPathNumberFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the number() XPath function
 *  number number(object?)
 */
void xmlXPathNumberFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	if(ctxt) {
		if(nargs == 0) {
			if(ctxt->context->P_Node == NULL) {
				valuePush(ctxt, xmlXPathCacheNewFloat(ctxt->context, 0.0));
			}
			else {
				xmlChar * content = xmlNodeGetContent(ctxt->context->P_Node);
				const double res = xmlXPathStringEvalNumber(content);
				valuePush(ctxt, xmlXPathCacheNewFloat(ctxt->context, res));
				SAlloc::F(content);
			}
		}
		else {
			CHECK_ARITY(1);
			xmlXPathObject * cur = valuePop(ctxt);
			valuePush(ctxt, xmlXPathCacheConvertNumber(ctxt->context, cur));
		}
	}
}
/**
 * xmlXPathSumFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the sum() XPath function
 *  number sum(node-set)
 * The sum function returns the sum of the values of the nodes in
 * the argument node-set.
 */
void xmlXPathSumFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	xmlXPathObject * cur;
	int i;
	double res = 0.0;
	CHECK_ARITY(1);
	if(!ctxt->value || ((ctxt->value->type != XPATH_NODESET) && (ctxt->value->type != XPATH_XSLT_TREE)))
		XP_ERROR(XPATH_INVALID_TYPE);
	cur = valuePop(ctxt);
	if(cur->nodesetval && (cur->nodesetval->nodeNr != 0)) {
		for(i = 0; i < cur->nodesetval->nodeNr; i++) {
			res += xmlXPathCastNodeToNumber(cur->nodesetval->PP_NodeTab[i]);
		}
	}
	valuePush(ctxt, xmlXPathCacheNewFloat(ctxt->context, res));
	xmlXPathReleaseObject(ctxt->context, cur);
}

/*
 * To assure working code on multiple platforms, we want to only depend
 * upon the characteristic truncation of converting a floating point value
 * to an integer.  Unfortunately, because of the different storage sizes
 * of our internal floating point value (double) and integer (int), we
 * can't directly convert (see bug 301162).  This macro is a messy
 * 'workaround'
 */
#define XTRUNC(f, v)		\
	f = fmod((v), INT_MAX);	    \
	f = (v) - (f) + (double)((int)(f));

/**
 * xmlXPathFloorFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the floor() XPath function
 *  number floor(number)
 * The floor function returns the largest (closest to positive infinity)
 * number that is not greater than the argument and that is an integer.
 */
void xmlXPathFloorFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	double f;
	CHECK_ARITY(1);
	CAST_TO_NUMBER;
	CHECK_TYPE(XPATH_NUMBER);
	XTRUNC(f, ctxt->value->floatval);
	if(f != ctxt->value->floatval) {
		if(ctxt->value->floatval > 0)
			ctxt->value->floatval = f;
		else
			ctxt->value->floatval = f - 1;
	}
}
// 
// xmlXPathCeilingFunction:
// @ctxt:  the XPath Parser context
// @nargs:  the number of arguments
// 
// Implement the ceiling() XPath function number ceiling(number)
// The ceiling function returns the smallest (closest to negative infinity)
// number that is not less than the argument and that is an integer.
// 
void xmlXPathCeilingFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	double f;
	CHECK_ARITY(1);
	CAST_TO_NUMBER;
	CHECK_TYPE(XPATH_NUMBER);
#if 0
	ctxt->value->floatval = ceil(ctxt->value->floatval);
#else
	XTRUNC(f, ctxt->value->floatval);
	if(f != ctxt->value->floatval) {
		if(ctxt->value->floatval > 0)
			ctxt->value->floatval = f + 1;
		else {
			if(ctxt->value->floatval < 0 && f == 0)
				ctxt->value->floatval = xmlXPathNZERO;
			else
				ctxt->value->floatval = f;
		}
	}
#endif
}
// 
// xmlXPathRoundFunction:
// @ctxt:  the XPath Parser context
// @nargs:  the number of arguments
// 
// Implement the round() XPath function number round(number)
// The round function returns the number that is closest to the
// argument and that is an integer. If there are two such numbers, then the one that is even is returned.
// 
void xmlXPathRoundFunction(xmlXPathParserContext * ctxt, int nargs) 
{
	double f;
	CHECK_ARITY(1);
	CAST_TO_NUMBER;
	CHECK_TYPE(XPATH_NUMBER);
	if((fisnan(ctxt->value->floatval)) || (xmlXPathIsInf(ctxt->value->floatval) == 1) || (xmlXPathIsInf(ctxt->value->floatval) == -1) || (ctxt->value->floatval == 0.0))
		return;
	XTRUNC(f, ctxt->value->floatval);
	if(ctxt->value->floatval < 0) {
		ctxt->value->floatval = (ctxt->value->floatval < f - 0.5) ? (f - 1) : f;
		if(ctxt->value->floatval == 0)
			ctxt->value->floatval = xmlXPathNZERO;
	}
	else
		ctxt->value->floatval = (ctxt->value->floatval < f + 0.5) ? f : (f + 1);
}
// 
// The Parser
// 
/*
 * a few forward declarations since we use a recursive call based
 * implementation.
 */
static void xmlXPathCompileExpr(xmlXPathParserContext * ctxt, int sort);
static void xmlXPathCompPredicate(xmlXPathParserContext * ctxt, int filter);
static void xmlXPathCompLocationPath(xmlXPathParserContext * ctxt);
static void xmlXPathCompRelativeLocationPath(xmlXPathParserContext * ctxt);
static xmlChar * xmlXPathParseNameComplex(xmlXPathParserContext * ctxt, int qualified);
/**
 * xmlXPathCurrentChar:
 * @ctxt:  the XPath parser context
 * @cur:  pointer to the beginning of the char
 * @len:  pointer to the length of the char read
 *
 * The current char value, if using UTF-8 this may actually span multiple
 * bytes in the input buffer.
 *
 * Returns the current char value and its length
 */
static int FASTCALL xmlXPathCurrentChar(xmlXPathParserContext * ctxt, int * len) 
{
	uchar c;
	uint val;
	const xmlChar * cur;
	if(!ctxt)
		return 0;
	cur = ctxt->cur;
	/*
	 * We are supposed to handle UTF8, check it's valid
	 * From rfc2044: encoding of the Unicode values on UTF-8:
	 *
	 * UCS-4 range (hex.)           UTF-8 octet sequence (binary)
	 * 0000 0000-0000 007F   0xxxxxxx
	 * 0000 0080-0000 07FF   110xxxxx 10xxxxxx
	 * 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx
	 *
	 * Check for the 0x110000 limit too
	 */
	c = *cur;
	if(c & 0x80) {
		if((cur[1] & 0xc0) != 0x80)
			goto encoding_error;
		if((c & 0xe0) == 0xe0) {
			if((cur[2] & 0xc0) != 0x80)
				goto encoding_error;
			if((c & 0xf0) == 0xf0) {
				if(((c & 0xf8) != 0xf0) || ((cur[3] & 0xc0) != 0x80))
					goto encoding_error;
				/* 4-byte code */
				*len = 4;
				val = (cur[0] & 0x7) << 18;
				val |= (cur[1] & 0x3f) << 12;
				val |= (cur[2] & 0x3f) << 6;
				val |= cur[3] & 0x3f;
			}
			else {
				/* 3-byte code */
				*len = 3;
				val = (cur[0] & 0xf) << 12;
				val |= (cur[1] & 0x3f) << 6;
				val |= cur[2] & 0x3f;
			}
		}
		else {
			/* 2-byte code */
			*len = 2;
			val = (cur[0] & 0x1f) << 6;
			val |= cur[1] & 0x3f;
		}
		if(!IS_CHAR(val)) {
			XP_ERROR0(XPATH_INVALID_CHAR_ERROR);
		}
		return val;
	}
	else {
		/* 1-byte code */
		*len = 1;
		return ((int)*cur);
	}
encoding_error:
	/*
	 * If we detect an UTF8 error that probably means that the
	 * input encoding didn't get properly advertised in the
	 * declaration header. Report the error and switch the encoding
	 * to ISO-Latin-1 (if you don't like this policy, just declare the
	 * encoding !)
	 */
	*len = 0;
	XP_ERROR0(XPATH_ENCODING_ERROR);
}
/**
 * xmlXPathParseNCName:
 * @ctxt:  the XPath Parser context
 *
 * parse an XML namespace non qualified name.
 *
 * [NS 3] NCName ::= (Letter | '_') (NCNameChar)*
 *
 * [NS 4] NCNameChar ::= Letter | Digit | '.' | '-' | '_' |
 *            CombiningChar | Extender
 *
 * Returns the namespace name or NULL
 */
xmlChar * xmlXPathParseNCName(xmlXPathParserContext * ctxt) 
{
	const xmlChar * in;
	xmlChar * ret;
	int count = 0;
	if(!ctxt || !ctxt->cur) 
		return 0;
	/*
	 * Accelerator for simple ASCII names
	 */
	in = ctxt->cur;
	if(((*in >= 0x61) && (*in <= 0x7A)) || ((*in >= 0x41) && (*in <= 0x5A)) || (*in == '_')) {
		in++;
		while(((*in >= 0x61) && (*in <= 0x7A)) || ((*in >= 0x41) && (*in <= 0x5A)) || ((*in >= 0x30) && (*in <= 0x39)) ||
		    (*in == '_') || (*in == '.') || (*in == '-'))
			in++;
		if((*in == ' ') || (*in == '>') || (*in == '/') || (*in == '[') || (*in == ']') || (*in == ':') || (*in == '@') || (*in == '*')) {
			count = in - ctxt->cur;
			if(!count)
				return 0;
			ret = xmlStrndup(ctxt->cur, count);
			ctxt->cur = in;
			return ret;
		}
	}
	return xmlXPathParseNameComplex(ctxt, 0);
}
/**
 * xmlXPathParseQName:
 * @ctxt:  the XPath Parser context
 * @prefix:  a xmlChar **
 *
 * parse an XML qualified name
 *
 * [NS 5] QName ::= (Prefix ':')? LocalPart
 *
 * [NS 6] Prefix ::= NCName
 *
 * [NS 7] LocalPart ::= NCName
 *
 * Returns the function returns the local part, and prefix is updated
 * to get the Prefix if any.
 */
static xmlChar * xmlXPathParseQName(xmlXPathParserContext * ctxt, xmlChar ** prefix) 
{
	*prefix = NULL;
	xmlChar * ret = xmlXPathParseNCName(ctxt);
	if(ret && CUR == ':') {
		*prefix = ret;
		NEXT;
		ret = xmlXPathParseNCName(ctxt);
	}
	return ret;
}
/**
 * xmlXPathParseName:
 * @ctxt:  the XPath Parser context
 *
 * parse an XML name
 *
 * [4] NameChar ::= Letter | Digit | '.' | '-' | '_' | ':' |
 *       CombiningChar | Extender
 *
 * [5] Name ::= (Letter | '_' | ':') (NameChar)*
 *
 * Returns the namespace name or NULL
 */
xmlChar * xmlXPathParseName(xmlXPathParserContext * ctxt) 
{
	const xmlChar * in;
	xmlChar * ret;
	size_t count = 0;
	if(!ctxt || !ctxt->cur) 
		return 0;
	/*
	 * Accelerator for simple ASCII names
	 */
	in = ctxt->cur;
	if(((*in >= 0x61) && (*in <= 0x7A)) || ((*in >= 0x41) && (*in <= 0x5A)) || (*in == '_') || (*in == ':')) {
		in++;
		while(((*in >= 0x61) && (*in <= 0x7A)) || ((*in >= 0x41) && (*in <= 0x5A)) || ((*in >= 0x30) && (*in <= 0x39)) ||
		    (*in == '_') || (*in == '-') || (*in == ':') || (*in == '.'))
			in++;
		if((*in > 0) && (*in < 0x80)) {
			count = in - ctxt->cur;
			if(count > XML_MAX_NAME_LENGTH) {
				ctxt->cur = in;
				XP_ERRORNULL(XPATH_EXPR_ERROR);
			}
			ret = xmlStrndup(ctxt->cur, count);
			ctxt->cur = in;
			return ret;
		}
	}
	return (xmlXPathParseNameComplex(ctxt, 1));
}

static xmlChar * xmlXPathParseNameComplex(xmlXPathParserContext * ctxt, int qualified) 
{
	xmlChar buf[XML_MAX_NAMELEN + 5];
	int len = 0, l;
	/*
	 * Handler for more complex cases
	 */
	int c = CUR_CHAR(l);
	if(oneof3(c, ' ', '>', '/') || /* accelerators */ oneof3(c, '[', ']', '@') || /* accelerators */ (c == '*') || /* accelerators */
	    (!IS_LETTER(c) && (c != '_') && ((qualified) && (c != ':')))) {
		return 0;
	}
	while(!oneof3(c, ' ', '>', '/') && /* test bigname.xml */
	    ((IS_LETTER(c)) || (isdec(c)) || (c == '.') || (c == '-') || (c == '_') || ((qualified) && (c == ':')) || (IS_COMBINING(c)) || (IS_EXTENDER(c)))) {
		COPY_BUF(l, buf, len, c);
		NEXTL(l);
		c = CUR_CHAR(l);
		if(len >= XML_MAX_NAMELEN) {
			/*
			 * Okay someone managed to make a huge name, so he's ready to pay
			 * for the processing speed.
			 */
			xmlChar * buffer;
			int max = len * 2;
			if(len > XML_MAX_NAME_LENGTH) {
				XP_ERRORNULL(XPATH_EXPR_ERROR);
			}
			buffer = static_cast<xmlChar *>(SAlloc::M(max * sizeof(xmlChar)));
			if(!buffer) {
				XP_ERRORNULL(XPATH_MEMORY_ERROR);
			}
			memcpy(buffer, buf, len);
			while((IS_LETTER(c)) || (isdec(c)) || /* test bigname.xml */ (c == '.') || (c == '-') || (c == '_') || ((qualified) && (c == ':')) ||
			    (IS_COMBINING(c)) || (IS_EXTENDER(c))) {
				if(len + 10 > max) {
					if(max > XML_MAX_NAME_LENGTH) {
						XP_ERRORNULL(XPATH_EXPR_ERROR);
					}
					max *= 2;
					buffer = static_cast<xmlChar *>(SAlloc::R(buffer, max * sizeof(xmlChar)));
					if(!buffer) {
						XP_ERRORNULL(XPATH_MEMORY_ERROR);
					}
				}
				COPY_BUF(l, buffer, len, c);
				NEXTL(l);
				c = CUR_CHAR(l);
			}
			buffer[len] = 0;
			return (buffer);
		}
	}
	return len ? xmlStrndup(buf, len) : 0;
}

#define MAX_FRAC 20
/*
 * These are used as divisors for the fractional part of a number.
 * Since the table includes 1.0 (representing '0' fractional digits),
 * it must be dimensioned at MAX_FRAC+1 (bug 133921)
 */
/* (replaced with fpow10i()) static double my_pow10[MAX_FRAC+1] = {
	1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0, 1000000.0, 10000000.0, 100000000.0, 1000000000.0,
	10000000000.0, 100000000000.0, 1000000000000.0, 10000000000000.0, 100000000000000.0,
	1000000000000000.0, 10000000000000000.0, 100000000000000000.0, 1000000000000000000.0, 10000000000000000000.0, 100000000000000000000.0
};*/
/**
 * xmlXPathStringEvalNumber:
 * @str:  A string to scan
 *
 *  [30a]  Float  ::= Number ('e' Digits?)?
 *
 *  [30]   Number ::=   Digits ('.' Digits?)? | '.' Digits
 *  [31]   Digits ::=   [0-9]+
 *
 * Compile a Number in the string
 * In complement of the Number expression, this function also handles
 * negative values : '-' Number.
 *
 * Returns the double value.
 */
double xmlXPathStringEvalNumber(const xmlChar * str) 
{
	const xmlChar * cur = str;
	double ret;
	int ok = 0;
	int isneg = 0;
	int exponent = 0;
	int is_exponent_negative = 0;
#ifdef __GNUC__
	unsigned long tmp = 0;
	double temp;
#endif
	if(!cur) 
		return 0;
	while(IS_BLANK_CH(*cur)) 
		cur++;
	if((*cur != '.') && !isdec(*cur) && (*cur != '-')) {
		return xmlXPathNAN;
	}
	if(*cur == '-') {
		isneg = 1;
		cur++;
	}
#ifdef __GNUC__
	// 
	// tmp/temp is a workaround against a gcc compiler bug http://veillard.com/gcc.bug
	// 
	ret = 0;
	while(isdec(*cur)) {
		ret = ret * 10;
		tmp = (*cur - '0');
		ok = 1;
		cur++;
		temp = (double)tmp;
		ret = ret + temp;
	}
#else
	ret = 0;
	while(isdec(*cur)) {
		ret = ret * 10 + (*cur - '0');
		ok = 1;
		cur++;
	}
#endif
	if(*cur == '.') {
		int v, frac = 0;
		double fraction = 0.0;
		cur++;
		if(!isdec(*cur) && !ok) {
			return xmlXPathNAN;
		}
		while(isdec(*cur) && frac < MAX_FRAC) {
			v = (*cur - '0');
			fraction = fraction * 10 + v;
			frac = frac + 1;
			cur++;
		}
		fraction /= fpow10i(frac)/*my_pow10[frac]*/;
		ret = ret + fraction;
		while(isdec(*cur))
			cur++;
	}
	if((*cur == 'e') || (*cur == 'E')) {
		cur++;
		if(*cur == '-') {
			is_exponent_negative = 1;
			cur++;
		}
		else if(*cur == '+') {
			cur++;
		}
		while(isdec(*cur)) {
			exponent = exponent * 10 + (*cur - '0');
			cur++;
		}
	}
	while(IS_BLANK_CH(*cur)) 
		cur++;
	if(*cur)
		return (xmlXPathNAN);
	if(isneg) 
		ret = -ret;
	if(is_exponent_negative) 
		exponent = -exponent;
	ret *= pow(10.0, (double)exponent);
	return ret;
}
/**
 * xmlXPathCompNumber:
 * @ctxt:  the XPath Parser context
 *
 *  [30]   Number ::=   Digits ('.' Digits?)? | '.' Digits
 *  [31]   Digits ::=   [0-9]+
 *
 * Compile a Number, then push it on the stack
 *
 */
static void xmlXPathCompNumber(xmlXPathParserContext * ctxt)
{
	double ret = 0.0;
	int ok = 0;
	int exponent = 0;
	int is_exponent_negative = 0;
#ifdef __GNUC__
	unsigned long tmp = 0;
	double temp;
#endif
	CHECK_ERROR;
	if((CUR != '.') && !isdec(CUR)) {
		XP_ERROR(XPATH_NUMBER_ERROR);
	}
#ifdef __GNUC__
	/*
	 * tmp/temp is a workaround against a gcc compiler bug
	 * http://veillard.com/gcc.bug
	 */
	ret = 0;
	while(isdec(CUR)) {
		ret = ret * 10;
		tmp = (CUR - '0');
		ok = 1;
		NEXT;
		temp = (double)tmp;
		ret = ret + temp;
	}
#else
	ret = 0;
	while(isdec(CUR)) {
		ret = ret * 10 + (CUR - '0');
		ok = 1;
		NEXT;
	}
#endif
	if(CUR == '.') {
		int v, frac = 0;
		double fraction = 0;
		NEXT;
		if(!isdec(CUR) && (!ok)) {
			XP_ERROR(XPATH_NUMBER_ERROR);
		}
		while(isdec(CUR) && (frac < MAX_FRAC)) {
			v = (CUR - '0');
			fraction = fraction * 10 + v;
			frac = frac + 1;
			NEXT;
		}
		fraction /= fpow10i(frac)/*my_pow10[frac]*/;
		ret = ret + fraction;
		while(isdec(CUR))
			NEXT;
	}
	if((CUR == 'e') || (CUR == 'E')) {
		NEXT;
		if(CUR == '-') {
			is_exponent_negative = 1;
			NEXT;
		}
		else if(CUR == '+') {
			NEXT;
		}
		while(isdec(CUR)) {
			exponent = exponent * 10 + (CUR - '0');
			NEXT;
		}
		if(is_exponent_negative)
			exponent = -exponent;
		ret *= pow(10.0, (double)exponent);
	}
	PUSH_LONG_EXPR(XPATH_OP_VALUE, XPATH_NUMBER, 0, 0, xmlXPathCacheNewFloat(ctxt->context, ret), 0);
}

/**
 * xmlXPathParseLiteral:
 * @ctxt:  the XPath Parser context
 *
 * Parse a Literal
 *
 *  [29]   Literal ::=   '"' [^"]* '"'
 *         | "'" [^']* "'"
 *
 * Returns the value found or NULL in case of error
 */
static xmlChar * xmlXPathParseLiteral(xmlXPathParserContext * ctxt)
{
	const xmlChar * q;
	xmlChar * ret = NULL;
	if(CUR == '"') {
		NEXT;
		q = CUR_PTR;
		while((IS_CHAR_CH(CUR)) && (CUR != '"'))
			NEXT;
		if(!IS_CHAR_CH(CUR)) {
			XP_ERRORNULL(XPATH_UNFINISHED_LITERAL_ERROR);
		}
		else {
			ret = xmlStrndup(q, CUR_PTR - q);
			NEXT;
		}
	}
	else if(CUR == '\'') {
		NEXT;
		q = CUR_PTR;
		while((IS_CHAR_CH(CUR)) && (CUR != '\''))
			NEXT;
		if(!IS_CHAR_CH(CUR)) {
			XP_ERRORNULL(XPATH_UNFINISHED_LITERAL_ERROR);
		}
		else {
			ret = xmlStrndup(q, CUR_PTR - q);
			NEXT;
		}
	}
	else {
		XP_ERRORNULL(XPATH_START_LITERAL_ERROR);
	}
	return ret;
}

/**
 * xmlXPathCompLiteral:
 * @ctxt:  the XPath Parser context
 *
 * Parse a Literal and push it on the stack.
 *
 *  [29]   Literal ::=   '"' [^"]* '"'
 *         | "'" [^']* "'"
 *
 * @todo xmlXPathCompLiteral memory allocation could be improved.
 */
static void xmlXPathCompLiteral(xmlXPathParserContext * ctxt) {
	const xmlChar * q;
	xmlChar * ret = NULL;

	if(CUR == '"') {
		NEXT;
		q = CUR_PTR;
		while((IS_CHAR_CH(CUR)) && (CUR != '"'))
			NEXT;
		if(!IS_CHAR_CH(CUR)) {
			XP_ERROR(XPATH_UNFINISHED_LITERAL_ERROR);
		}
		else {
			ret = xmlStrndup(q, CUR_PTR - q);
			NEXT;
		}
	}
	else if(CUR == '\'') {
		NEXT;
		q = CUR_PTR;
		while((IS_CHAR_CH(CUR)) && (CUR != '\''))
			NEXT;
		if(!IS_CHAR_CH(CUR)) {
			XP_ERROR(XPATH_UNFINISHED_LITERAL_ERROR);
		}
		else {
			ret = xmlStrndup(q, CUR_PTR - q);
			NEXT;
		}
	}
	else {
		XP_ERROR(XPATH_START_LITERAL_ERROR);
	}
	if(!ret) return;
	PUSH_LONG_EXPR(XPATH_OP_VALUE, XPATH_STRING, 0, 0, xmlXPathCacheNewString(ctxt->context, ret), 0);
	SAlloc::F(ret);
}

/**
 * xmlXPathCompVariableReference:
 * @ctxt:  the XPath Parser context
 *
 * Parse a VariableReference, evaluate it and push it on the stack.
 *
 * The variable bindings consist of a mapping from variable names
 * to variable values. The value of a variable is an object, which can be
 * of any of the types that are possible for the value of an expression,
 * and may also be of additional types not specified here.
 *
 * Early evaluation is possible since:
 * The variable bindings [...] used to evaluate a subexpression are
 * always the same as those used to evaluate the containing expression.
 *
 *  [36]   VariableReference ::=   '$' QName
 */
static void xmlXPathCompVariableReference(xmlXPathParserContext * ctxt) 
{
	xmlChar * name;
	xmlChar * prefix;
	SKIP_BLANKS;
	if(CUR != '$') {
		XP_ERROR(XPATH_VARIABLE_REF_ERROR);
	}
	NEXT;
	name = xmlXPathParseQName(ctxt, &prefix);
	if(!name) {
		XP_ERROR(XPATH_VARIABLE_REF_ERROR);
	}
	ctxt->comp->last = -1;
	PUSH_LONG_EXPR(XPATH_OP_VARIABLE, 0, 0, 0,
	    name, prefix);
	SKIP_BLANKS;
	if(ctxt->context && (ctxt->context->flags & XML_XPATH_NOVAR)) {
		XP_ERROR(XPATH_FORBID_VARIABLE_ERROR);
	}
}

/**
 * xmlXPathIsNodeType:
 * @name:  a name string
 *
 * Is the name given a NodeType one.
 *
 *  [38]   NodeType ::=   'comment'
 *         | 'text'
 *         | 'processing-instruction'
 *         | 'node'
 *
 * Returns 1 if true 0 otherwise
 */
int xmlXPathIsNodeType(const xmlChar * name) 
{
	if(!name)
		return 0;
	if(sstreq(name, "node"))
		return 1;
	if(sstreq(name, "text"))
		return 1;
	if(sstreq(name, "comment"))
		return 1;
	if(sstreq(name, "processing-instruction"))
		return 1;
	return 0;
}

/**
 * xmlXPathCompFunctionCall:
 * @ctxt:  the XPath Parser context
 *
 *  [16]   FunctionCall ::=   FunctionName '(' ( Argument ( ',' Argument)*)? ')'
 *  [17]   Argument ::=   Expr
 *
 * Compile a function call, the evaluation of all arguments are
 * pushed on the stack
 */
static void xmlXPathCompFunctionCall(xmlXPathParserContext * ctxt)
{
	xmlChar * prefix;
	int nbargs = 0;
	int sort = 1;
	xmlChar * name = xmlXPathParseQName(ctxt, &prefix);
	if(!name) {
		SAlloc::F(prefix);
		XP_ERROR(XPATH_EXPR_ERROR);
	}
	SKIP_BLANKS;
#ifdef DEBUG_EXPR
	if(prefix == NULL)
		xmlGenericError(0, "Calling function %s\n", name);
	else
		xmlGenericError(0, "Calling function %s:%s\n", prefix, name);
#endif
	if(CUR != '(') {
		XP_ERROR(XPATH_EXPR_ERROR);
	}
	NEXT;
	SKIP_BLANKS;
	/*
	 * Optimization for count(): we don't need the node-set to be sorted.
	 */
	if((prefix == NULL) && (name[0] == 'c') && sstreq(name, "count")) {
		sort = 0;
	}
	ctxt->comp->last = -1;
	if(CUR != ')') {
		while(CUR != 0) {
			int op1 = ctxt->comp->last;
			ctxt->comp->last = -1;
			xmlXPathCompileExpr(ctxt, sort);
			if(ctxt->error != XPATH_EXPRESSION_OK) {
				SAlloc::F(name);
				SAlloc::F(prefix);
				return;
			}
			PUSH_BINARY_EXPR(XPATH_OP_ARG, op1, ctxt->comp->last, 0, 0);
			nbargs++;
			if(CUR == ')')
				break;
			if(CUR != ',') {
				XP_ERROR(XPATH_EXPR_ERROR);
			}
			NEXT;
			SKIP_BLANKS;
		}
	}
	PUSH_LONG_EXPR(XPATH_OP_FUNCTION, nbargs, 0, 0, name, prefix);
	NEXT;
	SKIP_BLANKS;
}

/**
 * xmlXPathCompPrimaryExpr:
 * @ctxt:  the XPath Parser context
 *
 *  [15]   PrimaryExpr ::=   VariableReference
 *     | '(' Expr ')'
 *     | Literal
 *     | Number
 *     | FunctionCall
 *
 * Compile a primary expression.
 */
static void xmlXPathCompPrimaryExpr(xmlXPathParserContext * ctxt) 
{
	SKIP_BLANKS;
	if(CUR == '$') xmlXPathCompVariableReference(ctxt);
	else if(CUR == '(') {
		NEXT;
		SKIP_BLANKS;
		xmlXPathCompileExpr(ctxt, 1);
		CHECK_ERROR;
		if(CUR != ')') {
			XP_ERROR(XPATH_EXPR_ERROR);
		}
		NEXT;
		SKIP_BLANKS;
	}
	else if(IS_ASCII_DIGIT(CUR) || (CUR == '.' && IS_ASCII_DIGIT(NXT(1)))) {
		xmlXPathCompNumber(ctxt);
	}
	else if((CUR == '\'') || (CUR == '"')) {
		xmlXPathCompLiteral(ctxt);
	}
	else {
		xmlXPathCompFunctionCall(ctxt);
	}
	SKIP_BLANKS;
}

/**
 * xmlXPathCompFilterExpr:
 * @ctxt:  the XPath Parser context
 *
 *  [20]   FilterExpr ::=   PrimaryExpr
 *    | FilterExpr Predicate
 *
 * Compile a filter expression.
 * Square brackets are used to filter expressions in the same way that
 * they are used in location paths. It is an error if the expression to
 * be filtered does not evaluate to a node-set. The context node list
 * used for evaluating the expression in square brackets is the node-set
 * to be filtered listed in document order.
 */

static void xmlXPathCompFilterExpr(xmlXPathParserContext * ctxt) {
	xmlXPathCompPrimaryExpr(ctxt);
	CHECK_ERROR;
	SKIP_BLANKS;

	while(CUR == '[') {
		xmlXPathCompPredicate(ctxt, 1);
		SKIP_BLANKS;
	}
}

/**
 * xmlXPathScanName:
 * @ctxt:  the XPath Parser context
 *
 * Trickery: parse an XML name but without consuming the input flow
 * Needed to avoid insanity in the parser state.
 *
 * [4] NameChar ::= Letter | Digit | '.' | '-' | '_' | ':' |
 *       CombiningChar | Extender
 *
 * [5] Name ::= (Letter | '_' | ':') (NameChar)*
 *
 * [6] Names ::= Name (S Name)*
 *
 * Returns the Name parsed or NULL
 */

static xmlChar * xmlXPathScanName(xmlXPathParserContext * ctxt) {
	int len = 0, l;
	int c;
	const xmlChar * cur;
	xmlChar * ret;

	cur = ctxt->cur;

	c = CUR_CHAR(l);
	if((c == ' ') || (c == '>') || (c == '/') || /* accelerators */
	    (!IS_LETTER(c) && (c != '_') && (c != ':'))) {
		return 0;
	}
	while((c != ' ') && (c != '>') && (c != '/') && /* test bigname.xml */
	    ((IS_LETTER(c)) || (isdec(c)) || (c == '.') || (c == '-') || (c == '_') || (c == ':') || (IS_COMBINING(c)) || (IS_EXTENDER(c)))) {
		len += l;
		NEXTL(l);
		c = CUR_CHAR(l);
	}
	ret = xmlStrndup(cur, ctxt->cur - cur);
	ctxt->cur = cur;
	return ret;
}

/**
 * xmlXPathCompPathExpr:
 * @ctxt:  the XPath Parser context
 *
 *  [19]   PathExpr ::=   LocationPath
 *    | FilterExpr
 *    | FilterExpr '/' RelativeLocationPath
 *    | FilterExpr '//' RelativeLocationPath
 *
 * Compile a path expression.
 * The / operator and // operators combine an arbitrary expression
 * and a relative location path. It is an error if the expression
 * does not evaluate to a node-set.
 * The / operator does composition in the same way as when / is
 * used in a location path. As in location paths, // is short for
 * /descendant-or-self::node()/.
 */
static void xmlXPathCompPathExpr(xmlXPathParserContext * ctxt)
{
	int lc = 1; /* Should we branch to LocationPath ?         */
	xmlChar * name = NULL; /* we may have to preparse a name to find out */
	SKIP_BLANKS;
	if((CUR == '$') || (CUR == '(') || (IS_ASCII_DIGIT(CUR)) || (CUR == '\'') || (CUR == '"') || (CUR == '.' && IS_ASCII_DIGIT(NXT(1)))) {
		lc = 0;
	}
	else if(CUR == '*') {
		/* relative or absolute location path */
		lc = 1;
	}
	else if(CUR == '/') {
		/* relative or absolute location path */
		lc = 1;
	}
	else if(CUR == '@') {
		/* relative abbreviated attribute location path */
		lc = 1;
	}
	else if(CUR == '.') {
		/* relative abbreviated attribute location path */
		lc = 1;
	}
	else {
		/*
		 * Problem is finding if we have a name here whether it's:
		 * - a nodetype
		 * - a function call in which case it's followed by '('
		 * - an axis in which case it's followed by ':'
		 * - a element name
		 * We do an a priori analysis here rather than having to
		 * maintain parsed token content through the recursive function
		 * calls. This looks uglier but makes the code easier to
		 * read/write/debug.
		 */
		SKIP_BLANKS;
		name = xmlXPathScanName(ctxt);
		if(name && (xmlStrstr(name, (xmlChar *)"::") != NULL)) {
#ifdef DEBUG_STEP
			xmlGenericError(0, "PathExpr: Axis\n");
#endif
			lc = 1;
			SAlloc::F(name);
		}
		else if(name) {
			int len = sstrleni(name);
			while(NXT(len) != 0) {
				if(NXT(len) == '/') {
					/* element name */
#ifdef DEBUG_STEP
					xmlGenericError(0, "PathExpr: AbbrRelLocation\n");
#endif
					lc = 1;
					break;
				}
				else if(IS_BLANK_CH(NXT(len))) {
					/* ignore blanks */
					;
				}
				else if(NXT(len) == ':') {
#ifdef DEBUG_STEP
					xmlGenericError(0, "PathExpr: AbbrRelLocation\n");
#endif
					lc = 1;
					break;
				}
				else if((NXT(len) == '(')) {
					/* Note Type or Function */
					if(xmlXPathIsNodeType(name)) {
#ifdef DEBUG_STEP
						xmlGenericError(0, "PathExpr: Type search\n");
#endif
						lc = 1;
					}
					else {
#ifdef DEBUG_STEP
						xmlGenericError(0, "PathExpr: function call\n");
#endif
						lc = 0;
					}
					break;
				}
				else if((NXT(len) == '[')) {
					/* element name */
#ifdef DEBUG_STEP
					xmlGenericError(0, "PathExpr: AbbrRelLocation\n");
#endif
					lc = 1;
					break;
				}
				else if((NXT(len) == '<') || (NXT(len) == '>') ||
				    (NXT(len) == '=')) {
					lc = 1;
					break;
				}
				else {
					lc = 1;
					break;
				}
				len++;
			}
			if(NXT(len) == 0) {
#ifdef DEBUG_STEP
				xmlGenericError(0, "PathExpr: AbbrRelLocation\n");
#endif
				/* element name */
				lc = 1;
			}
			SAlloc::F(name);
		}
		else {
			/* make sure all cases are covered explicitly */
			XP_ERROR(XPATH_EXPR_ERROR);
		}
	}

	if(lc) {
		if(CUR == '/') {
			PUSH_LEAVE_EXPR(XPATH_OP_ROOT, 0, 0);
		}
		else {
			PUSH_LEAVE_EXPR(XPATH_OP_NODE, 0, 0);
		}
		xmlXPathCompLocationPath(ctxt);
	}
	else {
		xmlXPathCompFilterExpr(ctxt);
		CHECK_ERROR;
		if((CUR == '/') && (NXT(1) == '/')) {
			SKIP(2);
			SKIP_BLANKS;
			PUSH_LONG_EXPR(XPATH_OP_COLLECT, AXIS_DESCENDANT_OR_SELF, NODE_TEST_TYPE, NODE_TYPE_NODE, 0, 0);
			PUSH_UNARY_EXPR(XPATH_OP_RESET, ctxt->comp->last, 1, 0);
			xmlXPathCompRelativeLocationPath(ctxt);
		}
		else if(CUR == '/') {
			xmlXPathCompRelativeLocationPath(ctxt);
		}
	}
	SKIP_BLANKS;
}

/**
 * xmlXPathCompUnionExpr:
 * @ctxt:  the XPath Parser context
 *
 *  [18]   UnionExpr ::=   PathExpr
 *    | UnionExpr '|' PathExpr
 *
 * Compile an union expression.
 */

static void xmlXPathCompUnionExpr(xmlXPathParserContext * ctxt) 
{
	xmlXPathCompPathExpr(ctxt);
	CHECK_ERROR;
	SKIP_BLANKS;
	while(CUR == '|') {
		int op1 = ctxt->comp->last;
		PUSH_LEAVE_EXPR(XPATH_OP_NODE, 0, 0);
		NEXT;
		SKIP_BLANKS;
		xmlXPathCompPathExpr(ctxt);
		PUSH_BINARY_EXPR(XPATH_OP_UNION, op1, ctxt->comp->last, 0, 0);
		SKIP_BLANKS;
	}
}
/**
 * xmlXPathCompUnaryExpr:
 * @ctxt:  the XPath Parser context
 *
 *  [27]   UnaryExpr ::=   UnionExpr
 *        | '-' UnaryExpr
 *
 * Compile an unary expression.
 */

static void xmlXPathCompUnaryExpr(xmlXPathParserContext * ctxt) 
{
	int minus = 0;
	int found = 0;
	SKIP_BLANKS;
	while(CUR == '-') {
		minus = 1 - minus;
		found = 1;
		NEXT;
		SKIP_BLANKS;
	}
	xmlXPathCompUnionExpr(ctxt);
	CHECK_ERROR;
	if(found) {
		if(minus)
			PUSH_UNARY_EXPR(XPATH_OP_PLUS, ctxt->comp->last, 2, 0);
		else
			PUSH_UNARY_EXPR(XPATH_OP_PLUS, ctxt->comp->last, 3, 0);
	}
}
/**
 * xmlXPathCompMultiplicativeExpr:
 * @ctxt:  the XPath Parser context
 *
 *  [26]   MultiplicativeExpr ::=   UnaryExpr
 *        | MultiplicativeExpr MultiplyOperator UnaryExpr
 *        | MultiplicativeExpr 'div' UnaryExpr
 *        | MultiplicativeExpr 'mod' UnaryExpr
 *  [34]   MultiplyOperator ::=   '*'
 *
 * Compile an Additive expression.
 */
static void xmlXPathCompMultiplicativeExpr(xmlXPathParserContext * ctxt) 
{
	xmlXPathCompUnaryExpr(ctxt);
	CHECK_ERROR;
	SKIP_BLANKS;
	while((CUR == '*') || ((CUR == 'd') && (NXT(1) == 'i') && (NXT(2) == 'v')) || ((CUR == 'm') && (NXT(1) == 'o') && (NXT(2) == 'd'))) {
		int op = -1;
		int op1 = ctxt->comp->last;
		if(CUR == '*') {
			op = 0;
			NEXT;
		}
		else if(CUR == 'd') {
			op = 1;
			SKIP(3);
		}
		else if(CUR == 'm') {
			op = 2;
			SKIP(3);
		}
		SKIP_BLANKS;
		xmlXPathCompUnaryExpr(ctxt);
		CHECK_ERROR;
		PUSH_BINARY_EXPR(XPATH_OP_MULT, op1, ctxt->comp->last, op, 0);
		SKIP_BLANKS;
	}
}
/**
 * xmlXPathCompAdditiveExpr:
 * @ctxt:  the XPath Parser context
 *
 *  [25]   AdditiveExpr ::=   MultiplicativeExpr
 *        | AdditiveExpr '+' MultiplicativeExpr
 *        | AdditiveExpr '-' MultiplicativeExpr
 *
 * Compile an Additive expression.
 */
static void xmlXPathCompAdditiveExpr(xmlXPathParserContext * ctxt) 
{
	xmlXPathCompMultiplicativeExpr(ctxt);
	CHECK_ERROR;
	SKIP_BLANKS;
	while((CUR == '+') || (CUR == '-')) {
		int plus;
		int op1 = ctxt->comp->last;
		if(CUR == '+') 
			plus = 1;
		else 
			plus = 0;
		NEXT;
		SKIP_BLANKS;
		xmlXPathCompMultiplicativeExpr(ctxt);
		CHECK_ERROR;
		PUSH_BINARY_EXPR(XPATH_OP_PLUS, op1, ctxt->comp->last, plus, 0);
		SKIP_BLANKS;
	}
}
/**
 * xmlXPathCompRelationalExpr:
 * @ctxt:  the XPath Parser context
 *
 *  [24]   RelationalExpr ::=   AdditiveExpr
 *      | RelationalExpr '<' AdditiveExpr
 *      | RelationalExpr '>' AdditiveExpr
 *      | RelationalExpr '<=' AdditiveExpr
 *      | RelationalExpr '>=' AdditiveExpr
 *
 *  A <= B > C is allowed ? Answer from James, yes with
 *  (AdditiveExpr <= AdditiveExpr) > AdditiveExpr
 *  which is basically what got implemented.
 *
 * Compile a Relational expression, then push the result
 * on the stack
 */
static void xmlXPathCompRelationalExpr(xmlXPathParserContext * ctxt) 
{
	xmlXPathCompAdditiveExpr(ctxt);
	CHECK_ERROR;
	SKIP_BLANKS;
	while((CUR == '<') || (CUR == '>') || ((CUR == '<') && (NXT(1) == '=')) || ((CUR == '>') && (NXT(1) == '='))) {
		int inf, strict;
		int op1 = ctxt->comp->last;
		if(CUR == '<') 
			inf = 1;
		else 
			inf = 0;
		if(NXT(1) == '=') 
			strict = 0;
		else 
			strict = 1;
		NEXT;
		if(!strict) 
			NEXT;
		SKIP_BLANKS;
		xmlXPathCompAdditiveExpr(ctxt);
		CHECK_ERROR;
		PUSH_BINARY_EXPR(XPATH_OP_CMP, op1, ctxt->comp->last, inf, strict);
		SKIP_BLANKS;
	}
}
/**
 * xmlXPathCompEqualityExpr:
 * @ctxt:  the XPath Parser context
 *
 *  [23]   EqualityExpr ::=   RelationalExpr
 *      | EqualityExpr '=' RelationalExpr
 *      | EqualityExpr '!=' RelationalExpr
 *
 *  A != B != C is allowed ? Answer from James, yes with
 *  (RelationalExpr = RelationalExpr) = RelationalExpr
 *  (RelationalExpr != RelationalExpr) != RelationalExpr
 *  which is basically what got implemented.
 *
 * Compile an Equality expression.
 *
 */
static void xmlXPathCompEqualityExpr(xmlXPathParserContext * ctxt) 
{
	xmlXPathCompRelationalExpr(ctxt);
	CHECK_ERROR;
	SKIP_BLANKS;
	while((CUR == '=') || ((CUR == '!') && (NXT(1) == '='))) {
		int eq;
		int op1 = ctxt->comp->last;
		if(CUR == '=') 
			eq = 1;
		else 
			eq = 0;
		NEXT;
		if(!eq) 
			NEXT;
		SKIP_BLANKS;
		xmlXPathCompRelationalExpr(ctxt);
		CHECK_ERROR;
		PUSH_BINARY_EXPR(XPATH_OP_EQUAL, op1, ctxt->comp->last, eq, 0);
		SKIP_BLANKS;
	}
}
/**
 * xmlXPathCompAndExpr:
 * @ctxt:  the XPath Parser context
 *
 *  [22]   AndExpr ::=   EqualityExpr
 *      | AndExpr 'and' EqualityExpr
 *
 * Compile an AND expression.
 *
 */
static void xmlXPathCompAndExpr(xmlXPathParserContext * ctxt)
{
	xmlXPathCompEqualityExpr(ctxt);
	CHECK_ERROR;
	SKIP_BLANKS;
	while((CUR == 'a') && (NXT(1) == 'n') && (NXT(2) == 'd')) {
		int op1 = ctxt->comp->last;
		SKIP(3);
		SKIP_BLANKS;
		xmlXPathCompEqualityExpr(ctxt);
		CHECK_ERROR;
		PUSH_BINARY_EXPR(XPATH_OP_AND, op1, ctxt->comp->last, 0, 0);
		SKIP_BLANKS;
	}
}
/**
 * xmlXPathCompileExpr:
 * @ctxt:  the XPath Parser context
 *
 *  [14]   Expr ::=   OrExpr
 *  [21]   OrExpr ::=   AndExpr
 *      | OrExpr 'or' AndExpr
 *
 * Parse and compile an expression
 */
static void xmlXPathCompileExpr(xmlXPathParserContext * ctxt, int sort)
{
	xmlXPathCompAndExpr(ctxt);
	CHECK_ERROR;
	SKIP_BLANKS;
	while((CUR == 'o') && (NXT(1) == 'r')) {
		int op1 = ctxt->comp->last;
		SKIP(2);
		SKIP_BLANKS;
		xmlXPathCompAndExpr(ctxt);
		CHECK_ERROR;
		PUSH_BINARY_EXPR(XPATH_OP_OR, op1, ctxt->comp->last, 0, 0);
		SKIP_BLANKS;
	}
	if((sort) && (ctxt->comp->steps[ctxt->comp->last].op != XPATH_OP_VALUE)) {
		/* more ops could be optimized too */
		/*
		 * This is the main place to eliminate sorting for
		 * operations which don't require a sorted node-set.
		 * E.g. count().
		 */
		PUSH_UNARY_EXPR(XPATH_OP_SORT, ctxt->comp->last, 0, 0);
	}
}

/**
 * xmlXPathCompPredicate:
 * @ctxt:  the XPath Parser context
 * @filter:  act as a filter
 *
 *  [8]   Predicate ::=   '[' PredicateExpr ']'
 *  [9]   PredicateExpr ::=   Expr
 *
 * Compile a predicate expression
 */
static void xmlXPathCompPredicate(xmlXPathParserContext * ctxt, int filter) {
	int op1 = ctxt->comp->last;

	SKIP_BLANKS;
	if(CUR != '[') {
		XP_ERROR(XPATH_INVALID_PREDICATE_ERROR);
	}
	NEXT;
	SKIP_BLANKS;

	ctxt->comp->last = -1;
	/*
	 * This call to xmlXPathCompileExpr() will deactivate sorting
	 * of the predicate result.
	 * @todo Sorting is still activated for filters, since I'm not
	 *  sure if needed. Normally sorting should not be needed, since
	 *  a filter can only diminish the number of items in a sequence,
	 *  but won't change its order; so if the initial sequence is sorted,
	 *  subsequent sorting is not needed.
	 */
	if(!filter)
		xmlXPathCompileExpr(ctxt, 0);
	else
		xmlXPathCompileExpr(ctxt, 1);
	CHECK_ERROR;

	if(CUR != ']') {
		XP_ERROR(XPATH_INVALID_PREDICATE_ERROR);
	}

	if(filter)
		PUSH_BINARY_EXPR(XPATH_OP_FILTER, op1, ctxt->comp->last, 0, 0);
	else
		PUSH_BINARY_EXPR(XPATH_OP_PREDICATE, op1, ctxt->comp->last, 0, 0);

	NEXT;
	SKIP_BLANKS;
}

/**
 * xmlXPathCompNodeTest:
 * @ctxt:  the XPath Parser context
 * @test:  pointer to a xmlXPathTestVal
 * @type:  pointer to a xmlXPathTypeVal
 * @prefix:  placeholder for a possible name prefix
 *
 * [7] NodeTest ::=   NameTest
 *		    | NodeType '(' ')'
 *		    | 'processing-instruction' '(' Literal ')'
 *
 * [37] NameTest ::=  '*'
 *		    | NCName ':' '*'
 *		    | QName
 * [38] NodeType ::= 'comment'
 *		   | 'text'
 *		   | 'processing-instruction'
 *		   | 'node'
 *
 * Returns the name found and updates @test, @type and @prefix appropriately
 */
static xmlChar * xmlXPathCompNodeTest(xmlXPathParserContext * ctxt, xmlXPathTestVal * test,
    xmlXPathTypeVal * type, const xmlChar ** prefix,
    xmlChar * name) {
	int blanks;

	if((test == NULL) || (type == NULL) || (prefix == NULL)) {
		STRANGE;
		return 0;
	}
	*type = (xmlXPathTypeVal)0;
	*test = (xmlXPathTestVal)0;
	*prefix = NULL;
	SKIP_BLANKS;

	if(!name && (CUR == '*')) {
		/*
		 * All elements
		 */
		NEXT;
		*test = NODE_TEST_ALL;
		return 0;
	}
	if(!name)
		name = xmlXPathParseNCName(ctxt);
	if(!name) {
		XP_ERRORNULL(XPATH_EXPR_ERROR);
	}
	blanks = IS_BLANK_CH(CUR);
	SKIP_BLANKS;
	if(CUR == '(') {
		NEXT;
		/*
		 * NodeType or PI search
		 */
		if(sstreq(name, "comment"))
			*type = NODE_TYPE_COMMENT;
		else if(sstreq(name, "node"))
			*type = NODE_TYPE_NODE;
		else if(sstreq(name, "processing-instruction"))
			*type = NODE_TYPE_PI;
		else if(sstreq(name, "text"))
			*type = NODE_TYPE_TEXT;
		else {
			SAlloc::F(name);
			XP_ERRORNULL(XPATH_EXPR_ERROR);
		}
		*test = NODE_TEST_TYPE;
		SKIP_BLANKS;
		if(*type == NODE_TYPE_PI) {
			/*
			 * Specific case: search a PI by name.
			 */
			SAlloc::F(name);
			name = NULL;
			if(CUR != ')') {
				name = xmlXPathParseLiteral(ctxt);
				CHECK_ERROR NULL;
				*test = NODE_TEST_PI;
				SKIP_BLANKS;
			}
		}
		if(CUR != ')') {
			SAlloc::F(name);
			XP_ERRORNULL(XPATH_UNCLOSED_ERROR);
		}
		NEXT;
		return name;
	}
	*test = NODE_TEST_NAME;
	if((!blanks) && (CUR == ':')) {
		NEXT;

		/*
		 * Since currently the parser context don't have a
		 * namespace list associated:
		 * The namespace name for this prefix can be computed
		 * only at evaluation time. The compilation is done
		 * outside of any context.
		 */
#if 0
		*prefix = xmlXPathNsLookup(ctxt->context, name);
		SAlloc::F(name);
		if(*prefix == NULL) {
			XP_ERROR0(XPATH_UNDEF_PREFIX_ERROR);
		}
#else
		*prefix = name;
#endif

		if(CUR == '*') {
			/*
			 * All elements
			 */
			NEXT;
			*test = NODE_TEST_ALL;
			return 0;
		}

		name = xmlXPathParseNCName(ctxt);
		if(!name) {
			XP_ERRORNULL(XPATH_EXPR_ERROR);
		}
	}
	return name;
}

/**
 * xmlXPathIsAxisName:
 * @name:  a preparsed name token
 *
 * [6] AxisName ::=   'ancestor'
 *       | 'ancestor-or-self'
 *       | 'attribute'
 *       | 'child'
 *       | 'descendant'
 *       | 'descendant-or-self'
 *       | 'following'
 *       | 'following-sibling'
 *       | 'namespace'
 *       | 'parent'
 *       | 'preceding'
 *       | 'preceding-sibling'
 *       | 'self'
 *
 * Returns the axis or 0
 */
static xmlXPathAxisVal xmlXPathIsAxisName(const xmlChar * name) 
{
	xmlXPathAxisVal ret = (xmlXPathAxisVal)0;
	switch(name[0]) {
		case 'a':
		    if(sstreq(name, "ancestor"))
			    ret = AXIS_ANCESTOR;
		    if(sstreq(name, "ancestor-or-self"))
			    ret = AXIS_ANCESTOR_OR_SELF;
		    if(sstreq(name, "attribute"))
			    ret = AXIS_ATTRIBUTE;
		    break;
		case 'c':
		    if(sstreq(name, "child"))
			    ret = AXIS_CHILD;
		    break;
		case 'd':
		    if(sstreq(name, "descendant"))
			    ret = AXIS_DESCENDANT;
		    if(sstreq(name, "descendant-or-self"))
			    ret = AXIS_DESCENDANT_OR_SELF;
		    break;
		case 'f':
		    if(sstreq(name, "following"))
			    ret = AXIS_FOLLOWING;
		    if(sstreq(name, "following-sibling"))
			    ret = AXIS_FOLLOWING_SIBLING;
		    break;
		case 'n':
		    if(sstreq(name, "namespace"))
			    ret = AXIS_NAMESPACE;
		    break;
		case 'p':
		    if(sstreq(name, "parent"))
			    ret = AXIS_PARENT;
		    if(sstreq(name, "preceding"))
			    ret = AXIS_PRECEDING;
		    if(sstreq(name, "preceding-sibling"))
			    ret = AXIS_PRECEDING_SIBLING;
		    break;
		case 's':
		    if(sstreq(name, "self"))
			    ret = AXIS_SELF;
		    break;
	}
	return ret;
}
/**
 * xmlXPathCompStep:
 * @ctxt:  the XPath Parser context
 *
 * [4] Step ::=   AxisSpecifier NodeTest Predicate*
 *       | AbbreviatedStep
 *
 * [12] AbbreviatedStep ::=   '.' | '..'
 *
 * [5] AxisSpecifier ::= AxisName '::'
 *       | AbbreviatedAxisSpecifier
 *
 * [13] AbbreviatedAxisSpecifier ::= '@'?
 *
 * Modified for XPtr range support as:
 *
 *  [4xptr] Step ::= AxisSpecifier NodeTest Predicate*
 *          | AbbreviatedStep
 *          | 'range-to' '(' Expr ')' Predicate*
 *
 * Compile one step in a Location Path
 * A location step of . is short for self::node(). This is
 * particularly useful in conjunction with //. For example, the
 * location path .//para is short for
 * self::node()/descendant-or-self::node()/child::para
 * and so will select all para descendant elements of the context
 * node.
 * Similarly, a location step of .. is short for parent::node().
 * For example, ../title is short for parent::node()/child::title
 * and so will select the title children of the parent of the context
 * node.
 */
static void xmlXPathCompStep(xmlXPathParserContext * ctxt) 
{
#ifdef LIBXML_XPTR_ENABLED
	int rangeto = 0;
	int op2 = -1;
#endif
	SKIP_BLANKS;
	if((CUR == '.') && (NXT(1) == '.')) {
		SKIP(2);
		SKIP_BLANKS;
		PUSH_LONG_EXPR(XPATH_OP_COLLECT, AXIS_PARENT, NODE_TEST_TYPE, NODE_TYPE_NODE, 0, 0);
	}
	else if(CUR == '.') {
		NEXT;
		SKIP_BLANKS;
	}
	else {
		xmlChar * name = NULL;
		const xmlChar * prefix = NULL;
		xmlXPathTestVal test = (xmlXPathTestVal)0;
		xmlXPathAxisVal axis = (xmlXPathAxisVal)0;
		xmlXPathTypeVal type = (xmlXPathTypeVal)0;
		int op1;

		/*
		 * The modification needed for XPointer change to the production
		 */
#ifdef LIBXML_XPTR_ENABLED
		if(ctxt->xptr) {
			name = xmlXPathParseNCName(ctxt);
			if(name && (sstreq(name, "range-to"))) {
				op2 = ctxt->comp->last;
				SAlloc::F(name);
				SKIP_BLANKS;
				if(CUR != '(') {
					XP_ERROR(XPATH_EXPR_ERROR);
				}
				NEXT;
				SKIP_BLANKS;
				xmlXPathCompileExpr(ctxt, 1);
				/* PUSH_BINARY_EXPR(XPATH_OP_RANGETO, op2, ctxt->comp->last, 0, 0); */
				CHECK_ERROR;
				SKIP_BLANKS;
				if(CUR != ')') {
					XP_ERROR(XPATH_EXPR_ERROR);
				}
				NEXT;
				rangeto = 1;
				goto eval_predicates;
			}
		}
#endif
		if(CUR == '*') {
			axis = AXIS_CHILD;
		}
		else {
			SETIFZ(name, xmlXPathParseNCName(ctxt));
			if(name) {
				axis = xmlXPathIsAxisName(name);
				if(axis != 0) {
					SKIP_BLANKS;
					if((CUR == ':') && (NXT(1) == ':')) {
						SKIP(2);
						SAlloc::F(name);
						name = NULL;
					}
					else {
						axis = AXIS_CHILD; /* an element name can conflict with an axis one :-\ */
					}
				}
				else {
					axis = AXIS_CHILD;
				}
			}
			else if(CUR == '@') {
				NEXT;
				axis = AXIS_ATTRIBUTE;
			}
			else {
				axis = AXIS_CHILD;
			}
		}
		if(ctxt->error != XPATH_EXPRESSION_OK) {
			SAlloc::F(name);
			return;
		}
		name = xmlXPathCompNodeTest(ctxt, &test, &type, &prefix, name);
		if(test == 0)
			return;
		if(prefix && ctxt->context && (ctxt->context->flags & XML_XPATH_CHECKNS)) {
			if(xmlXPathNsLookup(ctxt->context, prefix) == NULL) {
				xmlXPathErr(ctxt, XPATH_UNDEF_PREFIX_ERROR);
			}
		}
#ifdef DEBUG_STEP
		xmlGenericError(0, "Basis : computing new set\n");
#endif

#ifdef DEBUG_STEP
		xmlGenericError(0, "Basis : ");
		if(ctxt->value == NULL)
			xmlGenericError(0, "no value\n");
		else if(ctxt->value->nodesetval == NULL)
			xmlGenericError(0, "Empty\n");
		else
			xmlGenericErrorContextNodeSet(stdout, ctxt->value->nodesetval);
#endif

#ifdef LIBXML_XPTR_ENABLED
eval_predicates:
#endif
		op1 = ctxt->comp->last;
		ctxt->comp->last = -1;

		SKIP_BLANKS;
		while(CUR == '[') {
			xmlXPathCompPredicate(ctxt, 0);
		}

#ifdef LIBXML_XPTR_ENABLED
		if(rangeto) {
			PUSH_BINARY_EXPR(XPATH_OP_RANGETO, op2, op1, 0, 0);
		}
		else
#endif
		PUSH_FULL_EXPR(XPATH_OP_COLLECT, op1, ctxt->comp->last, axis,
		    test, type, (void *)prefix, (void *)name);
	}
#ifdef DEBUG_STEP
	xmlGenericError(0, "Step : ");
	if(ctxt->value == NULL)
		xmlGenericError(0, "no value\n");
	else if(ctxt->value->nodesetval == NULL)
		xmlGenericError(0, "Empty\n");
	else
		xmlGenericErrorContextNodeSet(xmlGenericErrorContext, ctxt->value->nodesetval);
#endif
}

/**
 * xmlXPathCompRelativeLocationPath:
 * @ctxt:  the XPath Parser context
 *
 *  [3]   RelativeLocationPath ::=   Step
 *          | RelativeLocationPath '/' Step
 *          | AbbreviatedRelativeLocationPath
 *  [11]  AbbreviatedRelativeLocationPath ::=   RelativeLocationPath '//' Step
 *
 * Compile a relative location path.
 */
static void xmlXPathCompRelativeLocationPath(xmlXPathParserContext * ctxt) {
	SKIP_BLANKS;
	if((CUR == '/') && (NXT(1) == '/')) {
		SKIP(2);
		SKIP_BLANKS;
		PUSH_LONG_EXPR(XPATH_OP_COLLECT, AXIS_DESCENDANT_OR_SELF,
		    NODE_TEST_TYPE, NODE_TYPE_NODE, 0, 0);
	}
	else if(CUR == '/') {
		NEXT;
		SKIP_BLANKS;
	}
	xmlXPathCompStep(ctxt);
	CHECK_ERROR;
	SKIP_BLANKS;
	while(CUR == '/') {
		if((CUR == '/') && (NXT(1) == '/')) {
			SKIP(2);
			SKIP_BLANKS;
			PUSH_LONG_EXPR(XPATH_OP_COLLECT, AXIS_DESCENDANT_OR_SELF,
			    NODE_TEST_TYPE, NODE_TYPE_NODE, 0, 0);
			xmlXPathCompStep(ctxt);
		}
		else if(CUR == '/') {
			NEXT;
			SKIP_BLANKS;
			xmlXPathCompStep(ctxt);
		}
		SKIP_BLANKS;
	}
}

/**
 * xmlXPathCompLocationPath:
 * @ctxt:  the XPath Parser context
 *
 *  [1]   LocationPath ::=   RelativeLocationPath
 *          | AbsoluteLocationPath
 *  [2]   AbsoluteLocationPath ::=   '/' RelativeLocationPath?
 *          | AbbreviatedAbsoluteLocationPath
 *  [10]   AbbreviatedAbsoluteLocationPath ::=
 *                '//' RelativeLocationPath
 *
 * Compile a location path
 *
 * // is short for /descendant-or-self::node()/. For example,
 * //para is short for /descendant-or-self::node()/child::para and
 * so will select any para element in the document (even a para element
 * that is a document element will be selected by //para since the
 * document element node is a child of the root node); div//para is
 * short for div/descendant-or-self::node()/child::para and so will
 * select all para descendants of div children.
 */
static void xmlXPathCompLocationPath(xmlXPathParserContext * ctxt) 
{
	SKIP_BLANKS;
	if(CUR != '/') {
		xmlXPathCompRelativeLocationPath(ctxt);
	}
	else {
		while(CUR == '/') {
			if((CUR == '/') && (NXT(1) == '/')) {
				SKIP(2);
				SKIP_BLANKS;
				PUSH_LONG_EXPR(XPATH_OP_COLLECT, AXIS_DESCENDANT_OR_SELF,
				    NODE_TEST_TYPE, NODE_TYPE_NODE, 0, 0);
				xmlXPathCompRelativeLocationPath(ctxt);
			}
			else if(CUR == '/') {
				NEXT;
				SKIP_BLANKS;
				if((CUR != 0) &&
				    ((IS_ASCII_LETTER(CUR)) || (CUR == '_') || (CUR == '.') ||
					    (CUR == '@') || (CUR == '*')))
					xmlXPathCompRelativeLocationPath(ctxt);
			}
			CHECK_ERROR;
		}
	}
}
//
// XPath precompiled expression evaluation
//
static int FASTCALL xmlXPathCompOpEval(xmlXPathParserContext * ctxt, xmlXPathStepOp * op);

#ifdef DEBUG_STEP
static void xmlXPathDebugDumpStepAxis(xmlXPathStepOp * op, int nbNodes)
{
	xmlGenericError(0, "new step : ");
	switch(op->value) {
		case AXIS_ANCESTOR:
		    xmlGenericError(0, "axis 'ancestors' ");
		    break;
		case AXIS_ANCESTOR_OR_SELF:
		    xmlGenericError(0, "axis 'ancestors-or-self' ");
		    break;
		case AXIS_ATTRIBUTE:
		    xmlGenericError(0, "axis 'attributes' ");
		    break;
		case AXIS_CHILD:
		    xmlGenericError(0, "axis 'child' ");
		    break;
		case AXIS_DESCENDANT:
		    xmlGenericError(0, "axis 'descendant' ");
		    break;
		case AXIS_DESCENDANT_OR_SELF:
		    xmlGenericError(0, "axis 'descendant-or-self' ");
		    break;
		case AXIS_FOLLOWING:
		    xmlGenericError(0, "axis 'following' ");
		    break;
		case AXIS_FOLLOWING_SIBLING:
		    xmlGenericError(0, "axis 'following-siblings' ");
		    break;
		case AXIS_NAMESPACE:
		    xmlGenericError(0, "axis 'namespace' ");
		    break;
		case AXIS_PARENT:
		    xmlGenericError(0, "axis 'parent' ");
		    break;
		case AXIS_PRECEDING:
		    xmlGenericError(0, "axis 'preceding' ");
		    break;
		case AXIS_PRECEDING_SIBLING:
		    xmlGenericError(0, "axis 'preceding-sibling' ");
		    break;
		case AXIS_SELF:
		    xmlGenericError(0, "axis 'self' ");
		    break;
	}
	xmlGenericError(0, " context contains %d nodes\n", nbNodes);
	switch(op->value2) {
		case NODE_TEST_NONE:
		    xmlGenericError(0, "           searching for none !!!\n");
		    break;
		case NODE_TEST_TYPE:
		    xmlGenericError(0, "           searching for type %d\n", op->value3);
		    break;
		case NODE_TEST_PI:
		    xmlGenericError(0, "           searching for PI !!!\n");
		    break;
		case NODE_TEST_ALL:
		    xmlGenericError(0, "           searching for *\n");
		    break;
		case NODE_TEST_NS:
		    xmlGenericError(0, "           searching for namespace %s\n",
		    op->value5);
		    break;
		case NODE_TEST_NAME:
		    xmlGenericError(0, "           searching for name %s\n", op->value5);
		    if(op->value4)
			    xmlGenericError(0, "           with namespace %s\n", op->value4);
		    break;
	}
	xmlGenericError(0, "Testing : ");
}

#endif /* DEBUG_STEP */

static int xmlXPathCompOpEvalPredicate(xmlXPathParserContext * ctxt, xmlXPathStepOp * op, xmlNodeSet * set, int contextSize, int hasNsNodes)
{
	if(op->ch1 != -1) {
		xmlXPathCompExprPtr comp = ctxt->comp;
		/*
		 * Process inner predicates first.
		 */
		if(comp->steps[op->ch1].op != XPATH_OP_PREDICATE) {
			/*
			 * @todo raise an internal error.
			 */
		}
		contextSize = xmlXPathCompOpEvalPredicate(ctxt, &comp->steps[op->ch1], set, contextSize, hasNsNodes);
		CHECK_ERROR0;
		if(contextSize <= 0)
			return 0;
	}
	if(op->ch2 != -1) {
		xmlXPathContext * xpctxt = ctxt->context;
		xmlNode * contextNode;
		xmlNode * oldContextNode;
		xmlDoc * oldContextDoc;
		int i, res, contextPos = 0, newContextSize;
		xmlXPathStepOp * exprOp;
		xmlXPathObject * contextObj = NULL;
		xmlXPathObject * exprRes = NULL;
#ifdef LIBXML_XPTR_ENABLED
		/*
		 * URGENT TODO: Check the following:
		 *  We don't expect location sets if evaluating prediates, right?
		 *  Only filters should expect location sets, right?
		 */
#endif
		/*
		 * SPEC XPath 1.0:
		 *  "For each node in the node-set to be filtered, the
		 *  PredicateExpr is evaluated with that node as the
		 *  context node, with the number of nodes in the
		 *  node-set as the context size, and with the proximity
		 *  position of the node in the node-set with respect to
		 *  the axis as the context position;"
		 * @oldset is the node-set" to be filtered.
		 *
		 * SPEC XPath 1.0:
		 *  "only predicates change the context position and
		 *  context size (see [2.4 Predicates])."
		 * Example:
		 * node-set  context pos
		 *  nA         1
		 *  nB         2
		 *  nC         3
		 * After applying predicate [position() > 1] :
		 * node-set  context pos
		 *  nB         1
		 *  nC         2
		 */
		oldContextNode = xpctxt->P_Node;
		oldContextDoc = xpctxt->doc;
		/*
		 * Get the expression of this predicate.
		 */
		exprOp = &ctxt->comp->steps[op->ch2];
		newContextSize = 0;
		for(i = 0; i < set->nodeNr; i++) {
			if(set->PP_NodeTab[i] == NULL)
				continue;
			contextNode = set->PP_NodeTab[i];
			xpctxt->P_Node = contextNode;
			xpctxt->contextSize = contextSize;
			xpctxt->proximityPosition = ++contextPos;
			/*
			 * Also set the xpath document in case things like
			 * key() are evaluated in the predicate.
			 */
			if((contextNode->type != XML_NAMESPACE_DECL) && contextNode->doc)
				xpctxt->doc = contextNode->doc;
			/*
			 * Evaluate the predicate expression with 1 context node
			 * at a time; this node is packaged into a node set; this
			 * node set is handed over to the evaluation mechanism.
			 */
			if(contextObj == NULL)
				contextObj = xmlXPathCacheNewNodeSet(xpctxt, contextNode);
			else {
				if(xmlXPathNodeSetAddUnique(contextObj->nodesetval, contextNode) < 0) {
					ctxt->error = XPATH_MEMORY_ERROR;
					goto evaluation_exit;
				}
			}
			valuePush(ctxt, contextObj);
			res = xmlXPathCompOpEvalToBoolean(ctxt, exprOp, 1);
			if((ctxt->error != XPATH_EXPRESSION_OK) || (res == -1)) {
				xmlXPathNodeSetClear(set, hasNsNodes);
				newContextSize = 0;
				goto evaluation_exit;
			}
			if(res != 0) {
				newContextSize++;
			}
			else {
				/*
				 * Remove the entry from the initial node set.
				 */
				set->PP_NodeTab[i] = NULL;
				if(contextNode->type == XML_NAMESPACE_DECL)
					xmlXPathNodeSetFreeNs((xmlNs *)contextNode);
			}
			if(ctxt->value == contextObj) {
				/*
				 * Don't free the temporary XPath object holding the
				 * context node, in order to avoid massive recreation
				 * inside this loop.
				 */
				valuePop(ctxt);
				xmlXPathNodeSetClear(contextObj->nodesetval, hasNsNodes);
			}
			else {
				/*
				 * @todo The object was lost in the evaluation machinery.
				 *  Can this happen? Maybe in internal-error cases.
				 */
				contextObj = NULL;
			}
		}
		if(contextObj) {
			if(ctxt->value == contextObj)
				valuePop(ctxt);
			xmlXPathReleaseObject(xpctxt, contextObj);
		}
evaluation_exit:
		if(exprRes)
			xmlXPathReleaseObject(ctxt->context, exprRes);
		/*
		 * Reset/invalidate the context.
		 */
		xpctxt->P_Node = oldContextNode;
		xpctxt->doc = oldContextDoc;
		xpctxt->contextSize = -1;
		xpctxt->proximityPosition = -1;
		return (newContextSize);
	}
	return (contextSize);
}

static int xmlXPathCompOpEvalPositionalPredicate(xmlXPathParserContext * ctxt, xmlXPathStepOp * op, xmlNodeSet * set,
    int contextSize, int minPos, int maxPos, int hasNsNodes)
{
	if(op->ch1 != -1) {
		xmlXPathCompExprPtr comp = ctxt->comp;
		if(comp->steps[op->ch1].op != XPATH_OP_PREDICATE) {
			/*
			 * @todo raise an internal error.
			 */
		}
		contextSize = xmlXPathCompOpEvalPredicate(ctxt,
		    &comp->steps[op->ch1], set, contextSize, hasNsNodes);
		CHECK_ERROR0;
		if(contextSize <= 0)
			return 0;
	}
	/*
	 * Check if the node set contains a sufficient number of nodes for
	 * the requested range.
	 */
	if(contextSize < minPos) {
		xmlXPathNodeSetClear(set, hasNsNodes);
		return 0;
	}
	if(op->ch2 == -1) {
		/*
		 * @todo Can this ever happen?
		 */
		return (contextSize);
	}
	else {
		xmlDoc * oldContextDoc;
		int i, pos = 0, newContextSize = 0, contextPos = 0, res;
		xmlXPathStepOp * exprOp;
		xmlXPathObject * contextObj = NULL;
		xmlXPathObject * exprRes = NULL;
		xmlNode * oldContextNode;
		xmlNode * contextNode = NULL;
		xmlXPathContext * xpctxt = ctxt->context;
		int frame;
#ifdef LIBXML_XPTR_ENABLED
		/*
		 * URGENT TODO: Check the following:
		 *  We don't expect location sets if evaluating prediates, right?
		 *  Only filters should expect location sets, right?
		 */
#endif /* LIBXML_XPTR_ENABLED */

		/*
		 * Save old context.
		 */
		oldContextNode = xpctxt->P_Node;
		oldContextDoc = xpctxt->doc;
		/*
		 * Get the expression of this predicate.
		 */
		exprOp = &ctxt->comp->steps[op->ch2];
		for(i = 0; i < set->nodeNr; i++) {
			xmlXPathObject * tmp;
			if(set->PP_NodeTab[i] == NULL)
				continue;
			contextNode = set->PP_NodeTab[i];
			xpctxt->P_Node = contextNode;
			xpctxt->contextSize = contextSize;
			xpctxt->proximityPosition = ++contextPos;

			/*
			 * Initialize the new set.
			 * Also set the xpath document in case things like
			 * key() evaluation are attempted on the predicate
			 */
			if((contextNode->type != XML_NAMESPACE_DECL) && contextNode->doc)
				xpctxt->doc = contextNode->doc;
			/*
			 * Evaluate the predicate expression with 1 context node
			 * at a time; this node is packaged into a node set; this
			 * node set is handed over to the evaluation mechanism.
			 */
			if(contextObj == NULL)
				contextObj = xmlXPathCacheNewNodeSet(xpctxt, contextNode);
			else {
				if(xmlXPathNodeSetAddUnique(contextObj->nodesetval, contextNode) < 0) {
					ctxt->error = XPATH_MEMORY_ERROR;
					goto evaluation_exit;
				}
			}
			frame = xmlXPathSetFrame(ctxt);
			valuePush(ctxt, contextObj);
			res = xmlXPathCompOpEvalToBoolean(ctxt, exprOp, 1);
			tmp = valuePop(ctxt);
			xmlXPathPopFrame(ctxt, frame);

			if((ctxt->error != XPATH_EXPRESSION_OK) || (res == -1)) {
				while(tmp != contextObj) {
					/*
					 * Free up the result
					 * then pop off contextObj, which will be freed later
					 */
					xmlXPathReleaseObject(xpctxt, tmp);
					tmp = valuePop(ctxt);
				}
				goto evaluation_error;
			}
			/* push the result back onto the stack */
			valuePush(ctxt, tmp);

			if(res)
				pos++;

			if(res && (pos >= minPos) && (pos <= maxPos)) {
				/*
				 * Fits in the requested range.
				 */
				newContextSize++;
				if(minPos == maxPos) {
					/*
					 * Only 1 node was requested.
					 */
					if(contextNode->type == XML_NAMESPACE_DECL) {
						/*
						 * As always: take care of those nasty
						 * namespace nodes.
						 */
						set->PP_NodeTab[i] = NULL;
					}
					xmlXPathNodeSetClear(set, hasNsNodes);
					set->nodeNr = 1;
					set->PP_NodeTab[0] = contextNode;
					goto evaluation_exit;
				}
				if(pos == maxPos) {
					/*
					 * We are done.
					 */
					xmlXPathNodeSetClearFromPos(set, i +1, hasNsNodes);
					goto evaluation_exit;
				}
			}
			else {
				/*
				 * Remove the entry from the initial node set.
				 */
				set->PP_NodeTab[i] = NULL;
				if(contextNode->type == XML_NAMESPACE_DECL)
					xmlXPathNodeSetFreeNs((xmlNs *)contextNode);
			}
			if(exprRes) {
				xmlXPathReleaseObject(ctxt->context, exprRes);
				exprRes = NULL;
			}
			if(ctxt->value == contextObj) {
				/*
				 * Don't free the temporary XPath object holding the
				 * context node, in order to avoid massive recreation
				 * inside this loop.
				 */
				valuePop(ctxt);
				xmlXPathNodeSetClear(contextObj->nodesetval, hasNsNodes);
			}
			else {
				/*
				 * The object was lost in the evaluation machinery.
				 * Can this happen? Maybe in case of internal-errors.
				 */
				contextObj = NULL;
			}
		}
		goto evaluation_exit;

evaluation_error:
		xmlXPathNodeSetClear(set, hasNsNodes);
		newContextSize = 0;

evaluation_exit:
		if(contextObj) {
			if(ctxt->value == contextObj)
				valuePop(ctxt);
			xmlXPathReleaseObject(xpctxt, contextObj);
		}
		if(exprRes)
			xmlXPathReleaseObject(ctxt->context, exprRes);
		/*
		 * Reset/invalidate the context.
		 */
		xpctxt->P_Node = oldContextNode;
		xpctxt->doc = oldContextDoc;
		xpctxt->contextSize = -1;
		xpctxt->proximityPosition = -1;
		return (newContextSize);
	}
	return (contextSize);
}

static int xmlXPathIsPositionalPredicate(xmlXPathParserContext * ctxt, xmlXPathStepOp * op, int * maxPos)
{
	xmlXPathStepOp * exprOp;
	/*
	 * BIG NOTE: This is not intended for XPATH_OP_FILTER yet!
	 */

	/*
	 * If not -1, then ch1 will point to:
	 * 1) For predicates (XPATH_OP_PREDICATE):
	 *  - an inner predicate operator
	 * 2) For filters (XPATH_OP_FILTER):
	 *  - an inner filter operater OR
	 *  - an expression selecting the node set.
	 * E.g. "key('a', 'b')" or "(//foo | //bar)".
	 */
	if((op->op != XPATH_OP_PREDICATE) && (op->op != XPATH_OP_FILTER))
		return 0;
	if(op->ch2 != -1) {
		exprOp = &ctxt->comp->steps[op->ch2];
	}
	else
		return 0;
	if(exprOp && (exprOp->op == XPATH_OP_VALUE) && exprOp->value4 && (((xmlXPathObject *)exprOp->value4)->type == XPATH_NUMBER)) {
		/*
		 * We have a "[n]" predicate here.
		 * @todo Unfortunately this simplistic test here is not
		 * able to detect a position() predicate in compound
		 * expressions like "[@attr = 'a" and position() = 1],
		 * and even not the usage of position() in
		 * "[position() = 1]"; thus - obviously - a position-range,
		 * like it "[position() < 5]", is also not detected.
		 * Maybe we could rewrite the AST to ease the optimization.
		 */
		*maxPos = (int)((xmlXPathObject *)exprOp->value4)->floatval;
		if(((xmlXPathObject *)exprOp->value4)->floatval ==
		    (float)*maxPos) {
			return 1;
		}
	}
	return 0;
}

static int xmlXPathNodeCollectAndTest(xmlXPathParserContext * ctxt, xmlXPathStepOp * op, xmlNode ** first, xmlNode ** last, int toBool)
{
#define XP_TEST_HIT \
	if(hasAxisRange != 0) {	\
		if(++pos == maxPos) { \
			if(addNode(seq, cur) < 0) \
				ctxt->error = XPATH_MEMORY_ERROR; \
			goto axis_range_end; } \
	} else { \
		if(addNode(seq, cur) < 0) \
			ctxt->error = XPATH_MEMORY_ERROR; \
		if(breakOnFirstHit) goto first_hit; }

#define XP_TEST_HIT_NS \
	if(hasAxisRange != 0) {	\
		if(++pos == maxPos) { \
			hasNsNodes = 1;	\
			if(xmlXPathNodeSetAddNs(seq, xpctxt->P_Node, (xmlNs *)cur) < 0) \
				ctxt->error = XPATH_MEMORY_ERROR; \
			goto axis_range_end; } \
	} else { \
		hasNsNodes = 1;	\
		if(xmlXPathNodeSetAddNs(seq, xpctxt->P_Node, (xmlNs *)cur) < 0) \
			ctxt->error = XPATH_MEMORY_ERROR; \
		if(breakOnFirstHit) goto first_hit; }

	xmlXPathAxisVal axis = (xmlXPathAxisVal)op->value;
	xmlXPathTestVal test = (xmlXPathTestVal)op->value2;
	xmlXPathTypeVal type = (xmlXPathTypeVal)op->value3;
	const xmlChar * prefix = (const xmlChar *)op->value4;
	const xmlChar * name = (const xmlChar *)op->value5;
	const xmlChar * URI = NULL;

#ifdef DEBUG_STEP
	int nbMatches = 0, prevMatches = 0;
#endif
	int total = 0, hasNsNodes = 0;
	/* The popped object holding the context nodes */
	xmlXPathObject * obj;
	/* The set of context nodes for the node tests */
	xmlNodeSet * contextSeq;
	int contextIdx;
	xmlNode * contextNode;
	/* The final resulting node set wrt to all context nodes */
	xmlNodeSet * outSeq;
	/*
	 * The temporary resulting node set wrt 1 context node.
	 * Used to feed predicate evaluation.
	 */
	xmlNodeSet * seq;
	xmlNode * cur;
	/* First predicate operator */
	xmlXPathStepOp * predOp;
	int maxPos; /* The requested position() (when a "[n]" predicate) */
	int hasPredicateRange, hasAxisRange, pos, size, newSize;
	int breakOnFirstHit;
	xmlXPathTraversalFunction next = NULL;
	int (* addNode)(xmlNodeSet *, xmlNode *);
	xmlXPathNodeSetMergeFunction mergeAndClear;
	xmlNode * oldContextNode;
	xmlXPathContext * xpctxt = ctxt->context;
	CHECK_TYPE0(XPATH_NODESET);
	obj = valuePop(ctxt);
	/*
	 * Setup namespaces.
	 */
	if(prefix) {
		URI = xmlXPathNsLookup(xpctxt, prefix);
		if(!URI) {
			xmlXPathReleaseObject(xpctxt, obj);
			XP_ERROR0(XPATH_UNDEF_PREFIX_ERROR);
		}
	}
	/*
	 * Setup axis.
	 *
	 * MAYBE FUTURE TODO: merging optimizations:
	 * - If the nodes to be traversed wrt to the initial nodes and
	 * the current axis cannot overlap, then we could avoid searching
	 * for duplicates during the merge.
	 * But the question is how/when to evaluate if they cannot overlap.
	 * Example: if we know that for two initial nodes, the one is
	 * not in the ancestor-or-self axis of the other, then we could safely
	 * avoid a duplicate-aware merge, if the axis to be traversed is e.g.
	 * the descendant-or-self axis.
	 */
	mergeAndClear = xmlXPathNodeSetMergeAndClear;
	switch(axis) {
		case AXIS_ANCESTOR:
		    first = NULL;
		    next = xmlXPathNextAncestor;
		    break;
		case AXIS_ANCESTOR_OR_SELF:
		    first = NULL;
		    next = xmlXPathNextAncestorOrSelf;
		    break;
		case AXIS_ATTRIBUTE:
		    first = NULL;
		    last = NULL;
		    next = xmlXPathNextAttribute;
		    mergeAndClear = xmlXPathNodeSetMergeAndClearNoDupls;
		    break;
		case AXIS_CHILD:
		    last = NULL;
		    if(oneof2(test, NODE_TEST_NAME, NODE_TEST_ALL) && type == NODE_TYPE_NODE) {
				// Optimization if an element node type is 'element'.
			    next = xmlXPathNextChildElement;
		    }
		    else
			    next = xmlXPathNextChild;
		    mergeAndClear = xmlXPathNodeSetMergeAndClearNoDupls;
		    break;
		case AXIS_DESCENDANT:
		    last = NULL;
		    next = xmlXPathNextDescendant;
		    break;
		case AXIS_DESCENDANT_OR_SELF:
		    last = NULL;
		    next = xmlXPathNextDescendantOrSelf;
		    break;
		case AXIS_FOLLOWING:
		    last = NULL;
		    next = xmlXPathNextFollowing;
		    break;
		case AXIS_FOLLOWING_SIBLING:
		    last = NULL;
		    next = xmlXPathNextFollowingSibling;
		    break;
		case AXIS_NAMESPACE:
		    first = NULL;
		    last = NULL;
		    next = (xmlXPathTraversalFunction)xmlXPathNextNamespace;
		    mergeAndClear = xmlXPathNodeSetMergeAndClearNoDupls;
		    break;
		case AXIS_PARENT:
		    first = NULL;
		    next = xmlXPathNextParent;
		    break;
		case AXIS_PRECEDING:
		    first = NULL;
		    next = xmlXPathNextPrecedingInternal;
		    break;
		case AXIS_PRECEDING_SIBLING:
		    first = NULL;
		    next = xmlXPathNextPrecedingSibling;
		    break;
		case AXIS_SELF:
		    first = NULL;
		    last = NULL;
		    next = xmlXPathNextSelf;
		    mergeAndClear = xmlXPathNodeSetMergeAndClearNoDupls;
		    break;
	}
#ifdef DEBUG_STEP
	xmlXPathDebugDumpStepAxis(op, (obj->nodesetval ? obj->nodesetval->nodeNr : 0));
#endif
	if(!next) {
		xmlXPathReleaseObject(xpctxt, obj);
		return 0;
	}
	contextSeq = obj->nodesetval;
	if(!contextSeq || (contextSeq->nodeNr <= 0)) {
		xmlXPathReleaseObject(xpctxt, obj);
		valuePush(ctxt, xmlXPathCacheWrapNodeSet(xpctxt, NULL));
		return 0;
	}
	/*
	 * Predicate optimization ---------------------------------------------
	 * If this step has a last predicate, which contains a position(),
	 * then we'll optimize (although not exactly "position()", but only
	 * the  short-hand form, i.e., "[n]".
	 *
	 * Example - expression "/foo[parent::bar][1]":
	 *
	 * COLLECT 'child' 'name' 'node' foo    -- op (we are here)
	 * ROOT                               -- op->ch1
	 * PREDICATE                          -- op->ch2 (predOp)
	 *   PREDICATE                          -- predOp->ch1 = [parent::bar]
	 *  SORT
	 *    COLLECT  'parent' 'name' 'node' bar
	 *      NODE
	 *   ELEM Object is a number : 1        -- predOp->ch2 = [1]
	 *
	 */
	maxPos = 0;
	predOp = NULL;
	hasPredicateRange = 0;
	hasAxisRange = 0;
	if(op->ch2 != -1) {
		/*
		 * There's at least one predicate. 16 == XPATH_OP_PREDICATE
		 */
		predOp = &ctxt->comp->steps[op->ch2];
		if(xmlXPathIsPositionalPredicate(ctxt, predOp, &maxPos)) {
			if(predOp->ch1 != -1) {
				/*
				 * Use the next inner predicate operator.
				 */
				predOp = &ctxt->comp->steps[predOp->ch1];
				hasPredicateRange = 1;
			}
			else {
				/*
				 * There's no other predicate than the [n] predicate.
				 */
				predOp = NULL;
				hasAxisRange = 1;
			}
		}
	}
	breakOnFirstHit = ((toBool) && (predOp == NULL)) ? 1 : 0;
	/*
	 * Axis traversal -----------------------------------------------------
	 */
	/*
	 * 2.3 Node Tests
	 *  - For the attribute axis, the principal node type is attribute.
	 *  - For the namespace axis, the principal node type is namespace.
	 *  - For other axes, the principal node type is element.
	 *
	 * A node test * is true for any node of the
	 * principal node type. For example, child::* will
	 * select all element children of the context node
	 */
	oldContextNode = xpctxt->P_Node;
	addNode = xmlXPathNodeSetAddUnique;
	outSeq = NULL;
	seq = NULL;
	contextNode = NULL;
	contextIdx = 0;
	while(((contextIdx < contextSeq->nodeNr) || contextNode) && (ctxt->error == XPATH_EXPRESSION_OK)) {
		xpctxt->P_Node = contextSeq->PP_NodeTab[contextIdx++];
		if(seq == NULL) {
			seq = xmlXPathNodeSetCreate(NULL);
			if(seq == NULL) {
				total = 0;
				goto error;
			}
		}
		/*
		 * Traverse the axis and test the nodes.
		 */
		pos = 0;
		cur = NULL;
		hasNsNodes = 0;
		do {
			cur = next(ctxt, cur);
			if(!cur)
				break;
			/*
			 * QUESTION TODO: What does the "first" and "last" stuff do?
			 */
			if(first && *first) {
				if(*first == cur)
					break;
				if(((total % 256) == 0) &&
#ifdef XP_OPTIMIZED_NON_ELEM_COMPARISON
				    (xmlXPathCmpNodesExt(*first, cur) >= 0))
#else
				    (xmlXPathCmpNodes(*first, cur) >= 0))
#endif
				{
					break;
				}
			}
			if(last && *last) {
				if(*last == cur)
					break;
				if(((total % 256) == 0) &&
#ifdef XP_OPTIMIZED_NON_ELEM_COMPARISON
				    (xmlXPathCmpNodesExt(cur, *last) >= 0))
#else
				    (xmlXPathCmpNodes(cur, *last) >= 0))
#endif
				{
					break;
				}
			}
			total++;
#ifdef DEBUG_STEP
			xmlGenericError(0, " %s", cur->name);
#endif
			switch(test) {
				case NODE_TEST_NONE:
				    total = 0;
				    STRANGE
				    goto error;
				case NODE_TEST_TYPE:
				    /*
				 * @todo Don't we need to use
				 *  xmlXPathNodeSetAddNs() for namespace nodes here?
				 *  Surprisingly, some c14n tests fail, if we do this.
				     */
				    if(type == NODE_TYPE_NODE) {
					    switch(cur->type) {
						    case XML_DOCUMENT_NODE:
						    case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
						    case XML_DOCB_DOCUMENT_NODE:
#endif
						    case XML_ELEMENT_NODE:
						    case XML_ATTRIBUTE_NODE:
						    case XML_PI_NODE:
						    case XML_COMMENT_NODE:
						    case XML_CDATA_SECTION_NODE:
						    case XML_TEXT_NODE:
						    case XML_NAMESPACE_DECL:
							XP_TEST_HIT
							break;
						    default:
							break;
					    }
				    }
				    else if(cur->type == type) {
					    if(cur->type == XML_NAMESPACE_DECL)
						    XP_TEST_HIT_NS
					    else
						    XP_TEST_HIT
				    }
				    else if((type == NODE_TYPE_TEXT) && (cur->type == XML_CDATA_SECTION_NODE)) {
					    XP_TEST_HIT
				    }
				    break;
			    case NODE_TEST_PI:
					if((cur->type == XML_PI_NODE) && (!name || sstreq(name, cur->name))) {
						XP_TEST_HIT
					}
					break;
			    case NODE_TEST_ALL:
					if(axis == AXIS_ATTRIBUTE) {
						if(cur->type == XML_ATTRIBUTE_NODE) {
							if(prefix == NULL) {
								XP_TEST_HIT
							}
							else if(cur->ns && (sstreq(URI, cur->ns->href))) {
								XP_TEST_HIT
							}
						}
					}
					else if(axis == AXIS_NAMESPACE) {
						if(cur->type == XML_NAMESPACE_DECL) {
							XP_TEST_HIT_NS
						}
					}
					else {
						if(cur->type == XML_ELEMENT_NODE) {
							if(prefix == NULL) {
								XP_TEST_HIT
							}
							else if(cur->ns && (sstreq(URI, cur->ns->href))) {
								XP_TEST_HIT
							}
						}
					}
					break;
			    case NODE_TEST_NS: 
					{
						TODO;
						break;
					}
			    case NODE_TEST_NAME:
					if(axis == AXIS_ATTRIBUTE) {
						if(cur->type != XML_ATTRIBUTE_NODE)
							break;
					}
					else if(axis == AXIS_NAMESPACE) {
						if(cur->type != XML_NAMESPACE_DECL)
							break;
					}
					else {
						if(cur->type != XML_ELEMENT_NODE)
							break;
					}
					switch(cur->type) {
						case XML_ELEMENT_NODE:
						    if(sstreq(name, cur->name)) {
							    if(prefix == NULL) {
								    if(cur->ns == NULL) {
									    XP_TEST_HIT
								    }
							    }
							    else {
								    if(cur->ns && (sstreq(URI, cur->ns->href))) {
									    XP_TEST_HIT
								    }
							    }
						    }
						    break;
						case XML_ATTRIBUTE_NODE: {
						    xmlAttr * attr = (xmlAttr *)cur;
						    if(sstreq(name, attr->name)) {
							    if(prefix == NULL) {
								    if((attr->ns == NULL) || (attr->ns->prefix == NULL)) {
									    XP_TEST_HIT
								    }
							    }
							    else {
								    if(attr->ns && (sstreq(URI, attr->ns->href))) {
									    XP_TEST_HIT
								    }
							    }
						    }
						    break;
						    }
						case XML_NAMESPACE_DECL:
						    if(cur->type == XML_NAMESPACE_DECL) {
							    xmlNs * ns = (xmlNs *)cur;
							    if(ns->prefix && name && (sstreq(ns->prefix, name))) {
								    XP_TEST_HIT_NS
							    }
						    }
						    break;
						default:
						    break;
					}
					break;
			    } /* switch(test) */
			} while(cur && (ctxt->error == XPATH_EXPRESSION_OK)) ;
			goto apply_predicates;
axis_range_end: /* ----------------------------------------------------- */
			/*
			 * We have a "/foo[n]", and position() = n was reached.
			 * Note that we can have as well "/foo/::parent::foo[1]", so
			 * a duplicate-aware merge is still needed.
			 * Merge with the result.
			 */
			if(outSeq == NULL) {
				outSeq = seq;
				seq = NULL;
			}
			else
				outSeq = mergeAndClear(outSeq, seq, 0);
			/*
			 * Break if only a true/false result was requested.
			 */
			if(toBool)
				break;
			continue;

first_hit: /* ---------------------------------------------------------- */
			/*
			 * Break if only a true/false result was requested and
			 * no predicates existed and a node test succeeded.
			 */
			if(outSeq == NULL) {
				outSeq = seq;
				seq = NULL;
			}
			else
				outSeq = mergeAndClear(outSeq, seq, 0);
			break;

#ifdef DEBUG_STEP
			if(seq)
				nbMatches += seq->nodeNr;
#endif

apply_predicates: /* --------------------------------------------------- */
			if(ctxt->error != XPATH_EXPRESSION_OK)
				goto error;

			/*
			 * Apply predicates.
			 */
			if(predOp && (seq->nodeNr > 0)) {
				/*
				 * E.g. when we have a "/foo[some expression][n]".
				 */
				/*
				 * QUESTION TODO: The old predicate evaluation took into
				 *  account location-sets.
				 *  (E.g. ctxt->value->type == XPATH_LOCATIONSET)
				 *  Do we expect such a set here?
				 *  All what I learned now from the evaluation semantics
				 *  does not indicate that a location-set will be processed
				 *  here, so this looks OK.
				 */
				/*
				 * Iterate over all predicates, starting with the outermost
				 * predicate.
				 * @todo Problem: we cannot execute the inner predicates first
				 *  since we cannot go back *up* the operator tree!
				 *  Options we have:
				 *  1) Use of recursive functions (like is it currently done
				 *   via xmlXPathCompOpEval())
				 *  2) Add a predicate evaluation information stack to the
				 *   context struct
				 *  3) Change the way the operators are linked; we need a
				 *   "parent" field on xmlXPathStepOp
				 *
				 * For the moment, I'll try to solve this with a recursive
				 * function: xmlXPathCompOpEvalPredicate().
				 */
				size = seq->nodeNr;
				if(hasPredicateRange != 0)
					newSize = xmlXPathCompOpEvalPositionalPredicate(ctxt, predOp, seq, size, maxPos, maxPos, hasNsNodes);
				else
					newSize = xmlXPathCompOpEvalPredicate(ctxt, predOp, seq, size, hasNsNodes);
				if(ctxt->error != XPATH_EXPRESSION_OK) {
					total = 0;
					goto error;
				}
				/*
				 * Add the filtered set of nodes to the result node set.
				 */
				if(newSize == 0) {
					/*
					 * The predicates filtered all nodes out.
					 */
					xmlXPathNodeSetClear(seq, hasNsNodes);
				}
				else if(seq->nodeNr > 0) {
					/*
					 * Add to result set.
					 */
					if(outSeq == NULL) {
						if(size != newSize) {
							/*
							 * We need to merge and clear here, since
							 * the sequence will contained NULLed entries.
							 */
							outSeq = mergeAndClear(NULL, seq, 1);
						}
						else {
							outSeq = seq;
							seq = NULL;
						}
					}
					else
						outSeq = mergeAndClear(outSeq, seq,
						    (size != newSize) ? 1 : 0);
					/*
					 * Break if only a true/false result was requested.
					 */
					if(toBool)
						break;
				}
			}
			else if(seq->nodeNr > 0) {
				/*
				 * Add to result set.
				 */
				if(outSeq == NULL) {
					outSeq = seq;
					seq = NULL;
				}
				else {
					outSeq = mergeAndClear(outSeq, seq, 0);
				}
			}
		}

error:
		if((obj->boolval) && obj->user) {
			/*
			 * QUESTION TODO: What does this do and why?
			 * @todo Do we have to do this also for the "error"
			 * cleanup further down?
			 */
			ctxt->value->boolval = 1;
			ctxt->value->user = obj->user;
			obj->user = NULL;
			obj->boolval = 0;
		}
		xmlXPathReleaseObject(xpctxt, obj);

		/*
		 * Ensure we return at least an emtpy set.
		 */
		if(outSeq == NULL) {
			if(seq && (seq->nodeNr == 0))
				outSeq = seq;
			else
				outSeq = xmlXPathNodeSetCreate(NULL);
			/* XXX what if xmlXPathNodeSetCreate returned NULL here? */
		}
		if(seq && (seq != outSeq)) {
			xmlXPathFreeNodeSet(seq);
		}
		/*
		 * Hand over the result. Better to push the set also in
		 * case of errors.
		 */
		valuePush(ctxt, xmlXPathCacheWrapNodeSet(xpctxt, outSeq));
		/*
		 * Reset the context node.
		 */
		xpctxt->P_Node = oldContextNode;

#ifdef DEBUG_STEP
		xmlGenericError(0, "\nExamined %d nodes, found %d nodes at that step\n", total, nbMatches);
#endif

		return total;
	}
	static int xmlXPathCompOpEvalFilterFirst(xmlXPathParserContext * ctxt, xmlXPathStepOp * op, xmlNode ** first);

/**
 * xmlXPathCompOpEvalFirst:
 * @ctxt:  the XPath parser context with the compiled expression
 * @op:  an XPath compiled operation
 * @first:  the first elem found so far
 *
 * Evaluate the Precompiled XPath operation searching only the first
 * element in document order
 *
 * Returns the number of examined objects.
 */
	static int xmlXPathCompOpEvalFirst(xmlXPathParserContext * ctxt, xmlXPathStepOp * op, xmlNode ** first)
	{
		int total = 0, cur;
		xmlXPathCompExprPtr comp;
		xmlXPathObject * arg1;
		xmlXPathObject * arg2;
		CHECK_ERROR0;
		comp = ctxt->comp;
		switch(op->op) {
			case XPATH_OP_END:
			    return 0;
			case XPATH_OP_UNION:
			    total = xmlXPathCompOpEvalFirst(ctxt, &comp->steps[op->ch1], first);
			    CHECK_ERROR0;
			    if(ctxt->value && (ctxt->value->type == XPATH_NODESET) && ctxt->value->nodesetval && (ctxt->value->nodesetval->nodeNr >= 1)) {
				    /*
				 * limit tree traversing to first node in the result
				     */
				    /*
				 * OPTIMIZE TODO: This implicitely sorts
				 *  the result, even if not needed. E.g. if the argument
				 *  of the count() function, no sorting is needed.
				 * OPTIMIZE TODO: How do we know if the node-list wasn't
				 *  aready sorted?
				     */
				    if(ctxt->value->nodesetval->nodeNr > 1)
					    xmlXPathNodeSetSort(ctxt->value->nodesetval);
				    *first = ctxt->value->nodesetval->PP_NodeTab[0];
			    }
			    cur =
			    xmlXPathCompOpEvalFirst(ctxt, &comp->steps[op->ch2],
			    first);
			    CHECK_ERROR0;
			    CHECK_TYPE0(XPATH_NODESET);
			    arg2 = valuePop(ctxt);

			    CHECK_TYPE0(XPATH_NODESET);
			    arg1 = valuePop(ctxt);

			    arg1->nodesetval = xmlXPathNodeSetMerge(arg1->nodesetval,
			    arg2->nodesetval);
			    valuePush(ctxt, arg1);
			    xmlXPathReleaseObject(ctxt->context, arg2);
			    /* optimizer */
			    if(total > cur)
				    xmlXPathCompSwap(op);
			    return (total + cur);
			case XPATH_OP_ROOT:
			    xmlXPathRoot(ctxt);
			    return 0;
			case XPATH_OP_NODE:
			    if(op->ch1 != -1)
				    total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			    CHECK_ERROR0;
			    if(op->ch2 != -1)
				    total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			    CHECK_ERROR0;
			    valuePush(ctxt, xmlXPathCacheNewNodeSet(ctxt->context, ctxt->context->P_Node));
			    return total;
			case XPATH_OP_RESET:
			    if(op->ch1 != -1)
				    total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			    CHECK_ERROR0;
			    if(op->ch2 != -1)
				    total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			    CHECK_ERROR0;
			    ctxt->context->P_Node = NULL;
			    return total;
			case XPATH_OP_COLLECT: {
			    if(op->ch1 == -1)
				    return total;
			    total = xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			    CHECK_ERROR0;
			    total += xmlXPathNodeCollectAndTest(ctxt, op, first, NULL, 0);
			    return total;
		    }
			case XPATH_OP_VALUE:
			    valuePush(ctxt, xmlXPathCacheObjectCopy(ctxt->context, (xmlXPathObject *)op->value4));
			    return 0;
			case XPATH_OP_SORT:
			    if(op->ch1 != -1)
				    total += xmlXPathCompOpEvalFirst(ctxt, &comp->steps[op->ch1], first);
			    CHECK_ERROR0;
			    if(ctxt->value && (ctxt->value->type == XPATH_NODESET) && ctxt->value->nodesetval && (ctxt->value->nodesetval->nodeNr > 1))
				    xmlXPathNodeSetSort(ctxt->value->nodesetval);
			    return total;
#ifdef XP_OPTIMIZED_FILTER_FIRST
			case XPATH_OP_FILTER:
			    total += xmlXPathCompOpEvalFilterFirst(ctxt, op, first);
			    return total;
#endif
			default:
			    return (xmlXPathCompOpEval(ctxt, op));
		}
	}

/**
 * xmlXPathCompOpEvalLast:
 * @ctxt:  the XPath parser context with the compiled expression
 * @op:  an XPath compiled operation
 * @last:  the last elem found so far
 *
 * Evaluate the Precompiled XPath operation searching only the last
 * element in document order
 *
 * Returns the number of nodes traversed
 */
	static int xmlXPathCompOpEvalLast(xmlXPathParserContext * ctxt, xmlXPathStepOp * op, xmlNode ** last)
	{
		int total = 0, cur;
		xmlXPathCompExprPtr comp;
		xmlXPathObject * arg1;
		xmlXPathObject * arg2;
		xmlNode * bak;
		xmlDoc * bakd;
		int pp;
		int cs;
		CHECK_ERROR0;
		comp = ctxt->comp;
		switch(op->op) {
			case XPATH_OP_END:
			    return 0;
			case XPATH_OP_UNION:
			    bakd = ctxt->context->doc;
			    bak = ctxt->context->P_Node;
			    pp = ctxt->context->proximityPosition;
			    cs = ctxt->context->contextSize;
			    total = xmlXPathCompOpEvalLast(ctxt, &comp->steps[op->ch1], last);
			    CHECK_ERROR0;
			    if(ctxt->value && (ctxt->value->type == XPATH_NODESET) && ctxt->value->nodesetval && (ctxt->value->nodesetval->nodeNr >= 1)) {
				    /*
				 * limit tree traversing to first node in the result
				     */
				    if(ctxt->value->nodesetval->nodeNr > 1)
					    xmlXPathNodeSetSort(ctxt->value->nodesetval);
				    *last = ctxt->value->nodesetval->PP_NodeTab[ctxt->value->nodesetval->nodeNr - 1];
			    }
			    ctxt->context->doc = bakd;
			    ctxt->context->P_Node = bak;
			    ctxt->context->proximityPosition = pp;
			    ctxt->context->contextSize = cs;
			    cur = xmlXPathCompOpEvalLast(ctxt, &comp->steps[op->ch2], last);
			    CHECK_ERROR0;
			    if(ctxt->value && (ctxt->value->type == XPATH_NODESET) && ctxt->value->nodesetval && (ctxt->value->nodesetval->nodeNr >= 1)) { /* @todo NOP ? */
			    }
			    CHECK_TYPE0(XPATH_NODESET);
			    arg2 = valuePop(ctxt);
			    CHECK_TYPE0(XPATH_NODESET);
			    arg1 = valuePop(ctxt);
			    arg1->nodesetval = xmlXPathNodeSetMerge(arg1->nodesetval, arg2->nodesetval);
			    valuePush(ctxt, arg1);
			    xmlXPathReleaseObject(ctxt->context, arg2);
			    /* optimizer */
			    if(total > cur)
				    xmlXPathCompSwap(op);
			    return (total + cur);
			case XPATH_OP_ROOT:
			    xmlXPathRoot(ctxt);
			    return 0;
			case XPATH_OP_NODE:
			    if(op->ch1 != -1)
				    total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			    CHECK_ERROR0;
			    if(op->ch2 != -1)
				    total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			    CHECK_ERROR0;
			    valuePush(ctxt, xmlXPathCacheNewNodeSet(ctxt->context, ctxt->context->P_Node));
			    return total;
			case XPATH_OP_RESET:
			    if(op->ch1 != -1)
				    total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			    CHECK_ERROR0;
			    if(op->ch2 != -1)
				    total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			    CHECK_ERROR0;
			    ctxt->context->P_Node = NULL;
			    return total;
			case XPATH_OP_COLLECT: {
			    if(op->ch1 == -1)
				    return 0;
			    total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			    CHECK_ERROR0;
			    total += xmlXPathNodeCollectAndTest(ctxt, op, NULL, last, 0);
			    return total;
		    }
			case XPATH_OP_VALUE:
			    valuePush(ctxt, xmlXPathCacheObjectCopy(ctxt->context, (xmlXPathObject *)op->value4));
			    return 0;
			case XPATH_OP_SORT:
			    if(op->ch1 != -1)
				    total += xmlXPathCompOpEvalLast(ctxt, &comp->steps[op->ch1], last);
			    CHECK_ERROR0;
			    if(ctxt->value && (ctxt->value->type == XPATH_NODESET) && ctxt->value->nodesetval && (ctxt->value->nodesetval->nodeNr > 1))
				    xmlXPathNodeSetSort(ctxt->value->nodesetval);
			    return total;
			default:
			    return (xmlXPathCompOpEval(ctxt, op));
		}
	}
#ifdef XP_OPTIMIZED_FILTER_FIRST
	static int xmlXPathCompOpEvalFilterFirst(xmlXPathParserContext * ctxt, xmlXPathStepOp * op, xmlNode ** first)
	{
		int total = 0;
		xmlXPathCompExprPtr comp;
		xmlXPathObject * res;
		xmlXPathObject * obj;
		xmlNodeSet * oldset;
		xmlNode * oldnode;
		xmlDoc * oldDoc;
		int i;
		CHECK_ERROR0;
		comp = ctxt->comp;
		/*
		 * Optimization for ()[last()] selection i.e. the last elem
		 */
		if((op->ch1 != -1) && (op->ch2 != -1) && (comp->steps[op->ch1].op == XPATH_OP_SORT) && (comp->steps[op->ch2].op == XPATH_OP_SORT)) {
			int f = comp->steps[op->ch2].ch1;
			if((f != -1) && (comp->steps[f].op == XPATH_OP_FUNCTION) && (comp->steps[f].value5 == NULL) && (comp->steps[f].value == 0) && 
				comp->steps[f].value4 && (sstreq((const xmlChar *)comp->steps[f].value4, "last"))) {
				xmlNode * last = NULL;
				total += xmlXPathCompOpEvalLast(ctxt, &comp->steps[op->ch1], &last);
				CHECK_ERROR0;
				/*
				 * The nodeset should be in document order,
				 * Keep only the last value
				 */
				if(ctxt->value && (ctxt->value->type == XPATH_NODESET) &&
				    ctxt->value->nodesetval && ctxt->value->nodesetval->PP_NodeTab && (ctxt->value->nodesetval->nodeNr > 1)) {
					ctxt->value->nodesetval->PP_NodeTab[0] = ctxt->value->nodesetval->PP_NodeTab[ctxt->value->nodesetval->nodeNr - 1];
					ctxt->value->nodesetval->nodeNr = 1;
					*first = *(ctxt->value->nodesetval->PP_NodeTab);
				}
				return total;
			}
		}
		if(op->ch1 != -1)
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
		CHECK_ERROR0;
		if(op->ch2 == -1)
			return total;
		if(ctxt->value == NULL)
			return total;
#ifdef LIBXML_XPTR_ENABLED
		oldnode = ctxt->context->P_Node;
		/*
		 * Hum are we filtering the result of an XPointer expression
		 */
		if(ctxt->value->type == XPATH_LOCATIONSET) {
			xmlXPathObject * tmp = NULL;
			xmlLocationSet * newlocset = NULL;
			xmlLocationSet * oldlocset;
			/*
			 * Extract the old locset, and then evaluate the result of the
			 * expression for all the element in the locset. use it to grow
			 * up a new locset.
			 */
			CHECK_TYPE0(XPATH_LOCATIONSET);
			obj = valuePop(ctxt);
			oldlocset = (xmlLocationSet *)obj->user;
			ctxt->context->P_Node = NULL;
			if((oldlocset == NULL) || (oldlocset->locNr == 0)) {
				ctxt->context->contextSize = 0;
				ctxt->context->proximityPosition = 0;
				if(op->ch2 != -1)
					total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
				res = valuePop(ctxt);
				if(res) {
					xmlXPathReleaseObject(ctxt->context, res);
				}
				valuePush(ctxt, obj);
				CHECK_ERROR0;
				return total;
			}
			newlocset = xmlXPtrLocationSetCreate(NULL);

			for(i = 0; i < oldlocset->locNr; i++) {
				/*
				 * Run the evaluation with a node list made of a
				 * single item in the nodelocset.
				 */
				ctxt->context->P_Node = (xmlNode *)oldlocset->locTab[i]->user;
				ctxt->context->contextSize = oldlocset->locNr;
				ctxt->context->proximityPosition = i + 1;
				if(!tmp) {
					tmp = xmlXPathCacheNewNodeSet(ctxt->context,
					    ctxt->context->P_Node);
				}
				else {
					if(xmlXPathNodeSetAddUnique(tmp->nodesetval,
						    ctxt->context->P_Node) < 0) {
						ctxt->error = XPATH_MEMORY_ERROR;
					}
				}
				valuePush(ctxt, tmp);
				if(op->ch2 != -1)
					total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
				if(ctxt->error != XPATH_EXPRESSION_OK) {
					xmlXPathFreeObject(obj);
					return 0;
				}
				/*
				 * The result of the evaluation need to be tested to
				 * decided whether the filter succeeded or not
				 */
				res = valuePop(ctxt);
				if(xmlXPathEvaluatePredicateResult(ctxt, res)) {
					xmlXPtrLocationSetAdd(newlocset, xmlXPathCacheObjectCopy(ctxt->context, oldlocset->locTab[i]));
				}
				/*
				 * Cleanup
				 */
				if(res) {
					xmlXPathReleaseObject(ctxt->context, res);
				}
				if(ctxt->value == tmp) {
					valuePop(ctxt);
					xmlXPathNodeSetClear(tmp->nodesetval, 1);
					/*
					 * REVISIT TODO: Don't create a temporary nodeset
					 * for everly iteration.
					 */
					/* OLD: xmlXPathFreeObject(res); */
				}
				else
					tmp = NULL;
				ctxt->context->P_Node = NULL;
				/*
				 * Only put the first node in the result, then leave.
				 */
				if(newlocset->locNr > 0) {
					*first = (xmlNode *)oldlocset->locTab[i]->user;
					break;
				}
			}
			if(tmp) {
				xmlXPathReleaseObject(ctxt->context, tmp);
			}
			/*
			 * The result is used as the new evaluation locset.
			 */
			xmlXPathReleaseObject(ctxt->context, obj);
			ctxt->context->P_Node = NULL;
			ctxt->context->contextSize = -1;
			ctxt->context->proximityPosition = -1;
			valuePush(ctxt, xmlXPtrWrapLocationSet(newlocset));
			ctxt->context->P_Node = oldnode;
			return total;
		}
#endif /* LIBXML_XPTR_ENABLED */

		/*
		 * Extract the old set, and then evaluate the result of the
		 * expression for all the element in the set. use it to grow
		 * up a new set.
		 */
		CHECK_TYPE0(XPATH_NODESET);
		obj = valuePop(ctxt);
		oldset = obj->nodesetval;

		oldnode = ctxt->context->P_Node;
		oldDoc = ctxt->context->doc;
		ctxt->context->P_Node = NULL;

		if((oldset == NULL) || (oldset->nodeNr == 0)) {
			ctxt->context->contextSize = 0;
			ctxt->context->proximityPosition = 0;
			/* QUESTION TODO: Why was this code commented out?
			    if(op->ch2 != -1)
			        total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			    CHECK_ERROR0;
			    res = valuePop(ctxt);
			    if(res)
			        xmlXPathFreeObject(res);
			 */
			valuePush(ctxt, obj);
			ctxt->context->P_Node = oldnode;
			CHECK_ERROR0;
		}
		else {
			xmlXPathObject * tmp = NULL;
			/*
			 * Initialize the new set.
			 * Also set the xpath document in case things like
			 * key() evaluation are attempted on the predicate
			 */
			xmlNodeSet * newset = xmlXPathNodeSetCreate(NULL);
			/* XXX what if xmlXPathNodeSetCreate returned NULL? */
			for(i = 0; i < oldset->nodeNr; i++) {
				/*
				 * Run the evaluation with a node list made of
				 * a single item in the nodeset.
				 */
				ctxt->context->P_Node = oldset->PP_NodeTab[i];
				if((oldset->PP_NodeTab[i]->type != XML_NAMESPACE_DECL) && oldset->PP_NodeTab[i]->doc)
					ctxt->context->doc = oldset->PP_NodeTab[i]->doc;
				if(!tmp) {
					tmp = xmlXPathCacheNewNodeSet(ctxt->context, ctxt->context->P_Node);
				}
				else {
					if(xmlXPathNodeSetAddUnique(tmp->nodesetval, ctxt->context->P_Node) < 0) {
						ctxt->error = XPATH_MEMORY_ERROR;
					}
				}
				valuePush(ctxt, tmp);
				ctxt->context->contextSize = oldset->nodeNr;
				ctxt->context->proximityPosition = i + 1;
				if(op->ch2 != -1)
					total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
				if(ctxt->error != XPATH_EXPRESSION_OK) {
					xmlXPathFreeNodeSet(newset);
					xmlXPathFreeObject(obj);
					return 0;
				}
				/*
				 * The result of the evaluation needs to be tested to
				 * decide whether the filter succeeded or not
				 */
				res = valuePop(ctxt);
				if(xmlXPathEvaluatePredicateResult(ctxt, res)) {
					if(xmlXPathNodeSetAdd(newset, oldset->PP_NodeTab[i]) < 0)
						ctxt->error = XPATH_MEMORY_ERROR;
				}
				/*
				 * Cleanup
				 */
				if(res) {
					xmlXPathReleaseObject(ctxt->context, res);
				}
				if(ctxt->value == tmp) {
					valuePop(ctxt);
					/*
					 * Don't free the temporary nodeset
					 * in order to avoid massive recreation inside this
					 * loop.
					 */
					xmlXPathNodeSetClear(tmp->nodesetval, 1);
				}
				else
					tmp = NULL;
				ctxt->context->P_Node = NULL;
				/*
				 * Only put the first node in the result, then leave.
				 */
				if(newset->nodeNr > 0) {
					*first = *(newset->PP_NodeTab);
					break;
				}
			}
			if(tmp) {
				xmlXPathReleaseObject(ctxt->context, tmp);
			}
			/*
			 * The result is used as the new evaluation set.
			 */
			xmlXPathReleaseObject(ctxt->context, obj);
			ctxt->context->P_Node = NULL;
			ctxt->context->contextSize = -1;
			ctxt->context->proximityPosition = -1;
			/* may want to move this past the '}' later */
			ctxt->context->doc = oldDoc;
			valuePush(ctxt, xmlXPathCacheWrapNodeSet(ctxt->context, newset));
		}
		ctxt->context->P_Node = oldnode;
		return total;
	}

#endif /* XP_OPTIMIZED_FILTER_FIRST */
/**
 * xmlXPathCompOpEval:
 * @ctxt:  the XPath parser context with the compiled expression
 * @op:  an XPath compiled operation
 *
 * Evaluate the Precompiled XPath operation
 * Returns the number of nodes traversed
 */
static int FASTCALL xmlXPathCompOpEval(xmlXPathParserContext * ctxt, xmlXPathStepOp * op)
{
	int total = 0;
	int equal, ret;
	xmlXPathCompExprPtr comp;
	xmlXPathObject * arg1;
	xmlXPathObject * arg2;
	xmlNode * bak;
	xmlDoc * bakd;
	int pp;
	int cs;
	CHECK_ERROR0;
	comp = ctxt->comp;
	switch(op->op) {
		case XPATH_OP_END:
			return 0;
		case XPATH_OP_AND:
			bakd = ctxt->context->doc;
			bak = ctxt->context->P_Node;
			pp = ctxt->context->proximityPosition;
			cs = ctxt->context->contextSize;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			CHECK_ERROR0;
			xmlXPathBooleanFunction(ctxt, 1);
			if(!ctxt->value || (ctxt->value->boolval == 0))
				return total;
			arg2 = valuePop(ctxt);
			ctxt->context->doc = bakd;
			ctxt->context->P_Node = bak;
			ctxt->context->proximityPosition = pp;
			ctxt->context->contextSize = cs;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			if(ctxt->error) {
				xmlXPathFreeObject(arg2);
				return 0;
			}
			xmlXPathBooleanFunction(ctxt, 1);
			arg1 = valuePop(ctxt);
			arg1->boolval &= arg2->boolval;
			valuePush(ctxt, arg1);
			xmlXPathReleaseObject(ctxt->context, arg2);
			return total;
		case XPATH_OP_OR:
			bakd = ctxt->context->doc;
			bak = ctxt->context->P_Node;
			pp = ctxt->context->proximityPosition;
			cs = ctxt->context->contextSize;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			CHECK_ERROR0;
			xmlXPathBooleanFunction(ctxt, 1);
			if(!ctxt->value || (ctxt->value->boolval == 1))
				return total;
			arg2 = valuePop(ctxt);
			ctxt->context->doc = bakd;
			ctxt->context->P_Node = bak;
			ctxt->context->proximityPosition = pp;
			ctxt->context->contextSize = cs;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			if(ctxt->error) {
				xmlXPathFreeObject(arg2);
				return 0;
			}
			xmlXPathBooleanFunction(ctxt, 1);
			arg1 = valuePop(ctxt);
			arg1->boolval |= arg2->boolval;
			valuePush(ctxt, arg1);
			xmlXPathReleaseObject(ctxt->context, arg2);
			return total;
		case XPATH_OP_EQUAL:
			bakd = ctxt->context->doc;
			bak = ctxt->context->P_Node;
			pp = ctxt->context->proximityPosition;
			cs = ctxt->context->contextSize;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			CHECK_ERROR0;
			ctxt->context->doc = bakd;
			ctxt->context->P_Node = bak;
			ctxt->context->proximityPosition = pp;
			ctxt->context->contextSize = cs;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			CHECK_ERROR0;
			if(op->value)
				equal = xmlXPathEqualValues(ctxt);
			else
				equal = xmlXPathNotEqualValues(ctxt);
			valuePush(ctxt, xmlXPathCacheNewBoolean(ctxt->context, equal));
			return total;
		case XPATH_OP_CMP:
			bakd = ctxt->context->doc;
			bak = ctxt->context->P_Node;
			pp = ctxt->context->proximityPosition;
			cs = ctxt->context->contextSize;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			CHECK_ERROR0;
			ctxt->context->doc = bakd;
			ctxt->context->P_Node = bak;
			ctxt->context->proximityPosition = pp;
			ctxt->context->contextSize = cs;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			CHECK_ERROR0;
			ret = xmlXPathCompareValues(ctxt, op->value, op->value2);
			valuePush(ctxt, xmlXPathCacheNewBoolean(ctxt->context, ret));
			return total;
		case XPATH_OP_PLUS:
			bakd = ctxt->context->doc;
			bak = ctxt->context->P_Node;
			pp = ctxt->context->proximityPosition;
			cs = ctxt->context->contextSize;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			CHECK_ERROR0;
			if(op->ch2 != -1) {
				ctxt->context->doc = bakd;
				ctxt->context->P_Node = bak;
				ctxt->context->proximityPosition = pp;
				ctxt->context->contextSize = cs;
				total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			}
			CHECK_ERROR0;
			if(op->value == 0)
				xmlXPathSubValues(ctxt);
			else if(op->value == 1)
				xmlXPathAddValues(ctxt);
			else if(op->value == 2)
				xmlXPathValueFlipSign(ctxt);
			else if(op->value == 3) {
				CAST_TO_NUMBER;
				CHECK_TYPE0(XPATH_NUMBER);
			}
			return total;
		case XPATH_OP_MULT:
			bakd = ctxt->context->doc;
			bak = ctxt->context->P_Node;
			pp = ctxt->context->proximityPosition;
			cs = ctxt->context->contextSize;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			CHECK_ERROR0;
			ctxt->context->doc = bakd;
			ctxt->context->P_Node = bak;
			ctxt->context->proximityPosition = pp;
			ctxt->context->contextSize = cs;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			CHECK_ERROR0;
			if(op->value == 0)
				xmlXPathMultValues(ctxt);
			else if(op->value == 1)
				xmlXPathDivValues(ctxt);
			else if(op->value == 2)
				xmlXPathModValues(ctxt);
			return total;
		case XPATH_OP_UNION:
			bakd = ctxt->context->doc;
			bak = ctxt->context->P_Node;
			pp = ctxt->context->proximityPosition;
			cs = ctxt->context->contextSize;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			CHECK_ERROR0;
			ctxt->context->doc = bakd;
			ctxt->context->P_Node = bak;
			ctxt->context->proximityPosition = pp;
			ctxt->context->contextSize = cs;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			CHECK_ERROR0;
			CHECK_TYPE0(XPATH_NODESET);
			arg2 = valuePop(ctxt);
			CHECK_TYPE0(XPATH_NODESET);
			arg1 = valuePop(ctxt);
			if((arg1->nodesetval == NULL) || (arg2->nodesetval && (arg2->nodesetval->nodeNr != 0))) {
				arg1->nodesetval = xmlXPathNodeSetMerge(arg1->nodesetval,
				arg2->nodesetval);
			}
			valuePush(ctxt, arg1);
			xmlXPathReleaseObject(ctxt->context, arg2);
			return total;
		case XPATH_OP_ROOT:
			xmlXPathRoot(ctxt);
			return total;
		case XPATH_OP_NODE:
			if(op->ch1 != -1)
				total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			CHECK_ERROR0;
			if(op->ch2 != -1)
				total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			CHECK_ERROR0;
			valuePush(ctxt, xmlXPathCacheNewNodeSet(ctxt->context, ctxt->context->P_Node));
			return total;
		case XPATH_OP_RESET:
			if(op->ch1 != -1)
				total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			CHECK_ERROR0;
			if(op->ch2 != -1)
				total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
			CHECK_ERROR0;
			ctxt->context->P_Node = NULL;
			return total;
		case XPATH_OP_COLLECT: {
			if(op->ch1 == -1)
				return total;
			total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			CHECK_ERROR0;
			total += xmlXPathNodeCollectAndTest(ctxt, op, NULL, NULL, 0);
			return total;
		}
		case XPATH_OP_VALUE:
			valuePush(ctxt, xmlXPathCacheObjectCopy(ctxt->context, (xmlXPathObject *)op->value4));
			return total;
		case XPATH_OP_VARIABLE: {
			xmlXPathObject * val;
			if(op->ch1 != -1)
				total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			if(op->value5 == NULL) {
				val = xmlXPathVariableLookup(ctxt->context, (const xmlChar *)op->value4);
				if(!val) {
					ctxt->error = XPATH_UNDEF_VARIABLE_ERROR;
					return 0;
				}
				valuePush(ctxt, val);
			}
			else {
				const xmlChar * URI = xmlXPathNsLookup(ctxt->context, (const xmlChar *)op->value5);
				if(!URI) {
					xmlGenericError(0, "xmlXPathCompOpEval: variable %s bound to undefined prefix %s\n", (char *)op->value4, (char *)op->value5);
					ctxt->error = XPATH_UNDEF_PREFIX_ERROR;
					return total;
				}
				val = xmlXPathVariableLookupNS(ctxt->context, (const xmlChar *)op->value4, URI);
				if(!val) {
					ctxt->error = XPATH_UNDEF_VARIABLE_ERROR;
					return 0;
				}
				valuePush(ctxt, val);
			}
			return total;
		}
		case XPATH_OP_FUNCTION: {
			xmlXPathFunction func;
			const xmlChar * oldFunc, * oldFuncURI;
			int i;
			int frame = xmlXPathSetFrame(ctxt);
			if(op->ch1 != -1) {
				total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
				if(ctxt->error != XPATH_EXPRESSION_OK) {
					xmlXPathPopFrame(ctxt, frame);
					return total;
				}
			}
			if(ctxt->valueNr < ctxt->valueFrame + op->value) {
				xmlGenericError(0, "xmlXPathCompOpEval: parameter error\n");
				ctxt->error = XPATH_INVALID_OPERAND;
				xmlXPathPopFrame(ctxt, frame);
				return total;
			}
			for(i = 0; i < op->value; i++) {
				if(ctxt->valueTab[(ctxt->valueNr - 1) - i] == NULL) {
					xmlGenericError(0, "xmlXPathCompOpEval: parameter error\n");
					ctxt->error = XPATH_INVALID_OPERAND;
					xmlXPathPopFrame(ctxt, frame);
					return total;
				}
			}
			if(op->cache)
				XML_CAST_FPTR(func) = (xmlXPathFunction)op->cache;
			else {
				const xmlChar * URI = NULL;
				if(op->value5 == NULL)
					func = xmlXPathFunctionLookup(ctxt->context, (const xmlChar *)op->value4);
				else {
					URI = xmlXPathNsLookup(ctxt->context, (const xmlChar *)op->value5);
					if(!URI) {
						xmlGenericError(0, "xmlXPathCompOpEval: function %s bound to undefined prefix %s\n", (char *)op->value4, (char *)op->value5);
						xmlXPathPopFrame(ctxt, frame);
						ctxt->error = XPATH_UNDEF_PREFIX_ERROR;
						return total;
					}
					func = xmlXPathFunctionLookupNS(ctxt->context, (const xmlChar *)op->value4, URI);
				}
				if(func == NULL) {
					xmlGenericError(0, "xmlXPathCompOpEval: function %s not found\n", (char *)op->value4);
					XP_ERROR0(XPATH_UNKNOWN_FUNC_ERROR);
				}
				op->cache = XML_CAST_FPTR(func);
				op->cacheURI = (void *)URI;
			}
			oldFunc = ctxt->context->function;
			oldFuncURI = ctxt->context->functionURI;
			ctxt->context->function = (const xmlChar *)op->value4;
			ctxt->context->functionURI = (const xmlChar *)op->cacheURI;
			func(ctxt, op->value);
			ctxt->context->function = oldFunc;
			ctxt->context->functionURI = oldFuncURI;
			xmlXPathPopFrame(ctxt, frame);
			return total;
		}
		case XPATH_OP_ARG:
			bakd = ctxt->context->doc;
			bak = ctxt->context->P_Node;
			pp = ctxt->context->proximityPosition;
			cs = ctxt->context->contextSize;
			if(op->ch1 != -1) {
				total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
				ctxt->context->contextSize = cs;
				ctxt->context->proximityPosition = pp;
				ctxt->context->P_Node = bak;
				ctxt->context->doc = bakd;
				CHECK_ERROR0;
			}
			if(op->ch2 != -1) {
				total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
				ctxt->context->contextSize = cs;
				ctxt->context->proximityPosition = pp;
				ctxt->context->P_Node = bak;
				ctxt->context->doc = bakd;
				CHECK_ERROR0;
			}
			return total;
		case XPATH_OP_PREDICATE:
		case XPATH_OP_FILTER: {
			xmlXPathObject * res;
			xmlXPathObject * obj;
			xmlXPathObject * tmp;
			xmlNodeSet * newset = NULL;
			xmlNodeSet * oldset;
			xmlNode * oldnode;
			xmlDoc * oldDoc;
			int i;
			/*
			    * Optimization for ()[1] selection i.e. the first elem
			    */
			if((op->ch1 != -1) && (op->ch2 != -1) &&
#ifdef XP_OPTIMIZED_FILTER_FIRST
			        /*
			 * FILTER TODO: Can we assume that the inner processing
			 *  will result in an ordered list if we have an
			 *  XPATH_OP_FILTER?
			 *  What about an additional field or flag on
			 *  xmlXPathObject like @sorted ? This way we wouln'd need
			 *  to assume anything, so it would be more robust and
			 *  easier to optimize.
			            */
				((comp->steps[op->ch1].op == XPATH_OP_SORT) || /* 18 */ (comp->steps[op->ch1].op == XPATH_OP_FILTER)) && /* 17 */
#else
				(comp->steps[op->ch1].op == XPATH_OP_SORT) &&
#endif
				(comp->steps[op->ch2].op == XPATH_OP_VALUE)) { /* 12 */
				xmlXPathObject * val = (xmlXPathObject *)comp->steps[op->ch2].value4;
				if(val && (val->type == XPATH_NUMBER) && (val->floatval == 1.0)) {
					xmlNode * first = NULL;
					total += xmlXPathCompOpEvalFirst(ctxt, &comp->steps[op->ch1], &first);
					CHECK_ERROR0;
					/*
					    * The nodeset should be in document order,
					    * Keep only the first value
					    */
					if(ctxt->value && (ctxt->value->type == XPATH_NODESET) && ctxt->value->nodesetval && (ctxt->value->nodesetval->nodeNr > 1))
						ctxt->value->nodesetval->nodeNr = 1;
					return total;
				}
			}
			/*
			    * Optimization for ()[last()] selection i.e. the last elem
			    */
			if((op->ch1 != -1) && (op->ch2 != -1) && (comp->steps[op->ch1].op == XPATH_OP_SORT) && (comp->steps[op->ch2].op == XPATH_OP_SORT)) {
				int f = comp->steps[op->ch2].ch1;
				if((f != -1) && (comp->steps[f].op == XPATH_OP_FUNCTION) && (comp->steps[f].value5 == NULL) && (comp->steps[f].value == 0) &&
					comp->steps[f].value4 && (sstreq((const xmlChar *)comp->steps[f].value4, "last"))) {
					xmlNode * last = NULL;
					total += xmlXPathCompOpEvalLast(ctxt, &comp->steps[op->ch1], &last);
					CHECK_ERROR0;
					/*
					    * The nodeset should be in document order,
					    * Keep only the last value
					    */
					if(ctxt->value && (ctxt->value->type == XPATH_NODESET) && ctxt->value->nodesetval && ctxt->value->nodesetval->PP_NodeTab &&
						(ctxt->value->nodesetval->nodeNr > 1)) {
						ctxt->value->nodesetval->PP_NodeTab[0] = ctxt->value->nodesetval->PP_NodeTab[ctxt->value->nodesetval->nodeNr - 1];
						ctxt->value->nodesetval->nodeNr = 1;
					}
					return total;
				}
			}
			/*
			    * Process inner predicates first.
			    * Example "index[parent::book][1]":
			    * ...
			    * PREDICATE   <-- we are here "[1]"
			    *   PREDICATE <-- process "[parent::book]" first
			    *  SORT
			    *    COLLECT  'parent' 'name' 'node' book
			    *      NODE
			    *   ELEM Object is a number : 1
			    */
			if(op->ch1 != -1)
				total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			CHECK_ERROR0;
			if(op->ch2 == -1)
				return total;
			if(ctxt->value == NULL)
				return total;
			oldnode = ctxt->context->P_Node;
#ifdef LIBXML_XPTR_ENABLED
			/*
			    * Hum are we filtering the result of an XPointer expression
			    */
			if(ctxt->value->type == XPATH_LOCATIONSET) {
				xmlLocationSet * newlocset = NULL;
				xmlLocationSet * oldlocset;
				/*
				    * Extract the old locset, and then evaluate the result of the
				    * expression for all the element in the locset. use it to grow
				    * up a new locset.
				    */
				CHECK_TYPE0(XPATH_LOCATIONSET);
				obj = valuePop(ctxt);
				oldlocset = (xmlLocationSet *)obj->user;
				ctxt->context->P_Node = NULL;
				if(!oldlocset || !oldlocset->locNr) {
					ctxt->context->contextSize = 0;
					ctxt->context->proximityPosition = 0;
					if(op->ch2 != -1)
						total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
					res = valuePop(ctxt);
					if(res) {
						xmlXPathReleaseObject(ctxt->context, res);
					}
					valuePush(ctxt, obj);
					CHECK_ERROR0;
					return total;
				}
				newlocset = xmlXPtrLocationSetCreate(NULL);
				for(i = 0; i < oldlocset->locNr; i++) {
					// 
					// Run the evaluation with a node list made of a
					// single item in the nodelocset.
					// 
					ctxt->context->P_Node = (xmlNode *)oldlocset->locTab[i]->user;
					ctxt->context->contextSize = oldlocset->locNr;
					ctxt->context->proximityPosition = i + 1;
					tmp = xmlXPathCacheNewNodeSet(ctxt->context, ctxt->context->P_Node);
					valuePush(ctxt, tmp);
					if(op->ch2 != -1)
						total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
					if(ctxt->error != XPATH_EXPRESSION_OK) {
						xmlXPathFreeObject(obj);
						return 0;
					}
					/*
					    * The result of the evaluation need to be tested to
					    * decided whether the filter succeeded or not
					    */
					res = valuePop(ctxt);
					if(xmlXPathEvaluatePredicateResult(ctxt, res)) {
						xmlXPtrLocationSetAdd(newlocset, xmlXPathObjectCopy(oldlocset->locTab[i]));
					}
					/*
					    * Cleanup
					    */
					if(res) {
						xmlXPathReleaseObject(ctxt->context, res);
					}
					if(ctxt->value == tmp) {
						res = valuePop(ctxt);
						xmlXPathReleaseObject(ctxt->context, res);
					}
					ctxt->context->P_Node = NULL;
				}
				/*
				    * The result is used as the new evaluation locset.
				    */
				xmlXPathReleaseObject(ctxt->context, obj);
				ctxt->context->P_Node = NULL;
				ctxt->context->contextSize = -1;
				ctxt->context->proximityPosition = -1;
				valuePush(ctxt, xmlXPtrWrapLocationSet(newlocset));
				ctxt->context->P_Node = oldnode;
				return total;
			}
#endif /* LIBXML_XPTR_ENABLED */
			/*
			    * Extract the old set, and then evaluate the result of the
			    * expression for all the element in the set. use it to grow
			    * up a new set.
			    */
			CHECK_TYPE0(XPATH_NODESET);
			obj = valuePop(ctxt);
			oldset = obj->nodesetval;
			oldnode = ctxt->context->P_Node;
			oldDoc = ctxt->context->doc;
			ctxt->context->P_Node = NULL;
			if(!oldset || !oldset->nodeNr) {
				ctxt->context->contextSize = 0;
				ctxt->context->proximityPosition = 0;
/*
                if(op->ch2 != -1)
                    total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
                CHECK_ERROR0;
                res = valuePop(ctxt);
                if(res)
                    xmlXPathFreeObject(res);
*/
				valuePush(ctxt, obj);
				ctxt->context->P_Node = oldnode;
				CHECK_ERROR0;
			}
			else {
				tmp = NULL;
				/*
				    * Initialize the new set.
				    * Also set the xpath document in case things like
				    * key() evaluation are attempted on the predicate
				    */
				newset = xmlXPathNodeSetCreate(NULL);
				/*
				    * SPEC XPath 1.0:
				    *  "For each node in the node-set to be filtered, the
				    *  PredicateExpr is evaluated with that node as the
				    *  context node, with the number of nodes in the
				    *  node-set as the context size, and with the proximity
				    *  position of the node in the node-set with respect to
				    *  the axis as the context position;"
				    * @oldset is the node-set" to be filtered.
				    *
				    * SPEC XPath 1.0:
				    *  "only predicates change the context position and
				    *  context size (see [2.4 Predicates])."
				    * Example:
				    * node-set  context pos
				    *  nA         1
				    *  nB         2
				    *  nC         3
				    * After applying predicate [position() > 1] :
				    * node-set  context pos
				    *  nB         1
				    *  nC         2
				    *
				    * removed the first node in the node-set, then
				    * the context position of the
				    */
				for(i = 0; i < oldset->nodeNr; i++) {
					/*
					    * Run the evaluation with a node list made of
					    * a single item in the nodeset.
					    */
					ctxt->context->P_Node = oldset->PP_NodeTab[i];
					if((oldset->PP_NodeTab[i]->type != XML_NAMESPACE_DECL) && oldset->PP_NodeTab[i]->doc)
						ctxt->context->doc = oldset->PP_NodeTab[i]->doc;
					if(!tmp)
						tmp = xmlXPathCacheNewNodeSet(ctxt->context, ctxt->context->P_Node);
					else if(xmlXPathNodeSetAddUnique(tmp->nodesetval, ctxt->context->P_Node) < 0)
						ctxt->error = XPATH_MEMORY_ERROR;
					valuePush(ctxt, tmp);
					ctxt->context->contextSize = oldset->nodeNr;
					ctxt->context->proximityPosition = i + 1;
					/*
					    * Evaluate the predicate against the context node.
					    * Can/should we optimize position() predicates
					    * here (e.g. "[1]")?
					    */
					if(op->ch2 != -1)
						total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
					if(ctxt->error != XPATH_EXPRESSION_OK) {
						xmlXPathFreeNodeSet(newset);
						xmlXPathFreeObject(obj);
						return 0;
					}
					/*
					    * The result of the evaluation needs to be tested to
					    * decide whether the filter succeeded or not
					    */
					/*
					    * OPTIMIZE TODO: Can we use
					    * xmlXPathNodeSetAdd*Unique()* instead?
					    */
					res = valuePop(ctxt);
					if(xmlXPathEvaluatePredicateResult(ctxt, res)) {
						if(xmlXPathNodeSetAdd(newset, oldset->PP_NodeTab[i]) < 0)
							ctxt->error = XPATH_MEMORY_ERROR;
					}
					// Cleanup
					if(res) {
						xmlXPathReleaseObject(ctxt->context, res);
					}
					if(ctxt->value == tmp) {
						valuePop(ctxt);
						xmlXPathNodeSetClear(tmp->nodesetval, 1);
						// 
						// Don't free the temporary nodeset
						// in order to avoid massive recreation inside this loop.
						// 
					}
					else
						tmp = NULL;
					ctxt->context->P_Node = NULL;
				}
				if(tmp)
					xmlXPathReleaseObject(ctxt->context, tmp);
				/*
				    * The result is used as the new evaluation set.
				    */
				xmlXPathReleaseObject(ctxt->context, obj);
				ctxt->context->P_Node = NULL;
				ctxt->context->contextSize = -1;
				ctxt->context->proximityPosition = -1;
				/* may want to move this past the '}' later */
				ctxt->context->doc = oldDoc;
				valuePush(ctxt, xmlXPathCacheWrapNodeSet(ctxt->context, newset));
			}
			ctxt->context->P_Node = oldnode;
			return total;
		}
		case XPATH_OP_SORT:
			if(op->ch1 != -1)
				total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			CHECK_ERROR0;
			if(ctxt->value && (ctxt->value->type == XPATH_NODESET) && ctxt->value->nodesetval && (ctxt->value->nodesetval->nodeNr > 1)) {
				xmlXPathNodeSetSort(ctxt->value->nodesetval);
			}
			return total;
#ifdef LIBXML_XPTR_ENABLED
		case XPATH_OP_RANGETO: {
			xmlXPathObject * range;
			xmlXPathObject * res;
			xmlXPathObject * obj;
			xmlXPathObject * tmp;
			xmlLocationSet * newlocset = NULL;
			xmlLocationSet * oldlocset;
			xmlNodeSet * oldset;
			int i, j;
			if(op->ch1 != -1)
				total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch1]);
			if(op->ch2 == -1)
				return total;
			if(ctxt->value->type == XPATH_LOCATIONSET) {
				// 
				// Extract the old locset, and then evaluate the result of the
				// expression for all the element in the locset. use it to grow up a new locset.
				// 
				CHECK_TYPE0(XPATH_LOCATIONSET);
				obj = valuePop(ctxt);
				oldlocset = (xmlLocationSet *)obj->user;
				if(!oldlocset || (oldlocset->locNr == 0)) {
					ctxt->context->P_Node = NULL;
					ctxt->context->contextSize = 0;
					ctxt->context->proximityPosition = 0;
					total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
					res = valuePop(ctxt);
					if(res)
						xmlXPathReleaseObject(ctxt->context, res);
					valuePush(ctxt, obj);
					CHECK_ERROR0;
					return total;
				}
				newlocset = xmlXPtrLocationSetCreate(NULL);
				for(i = 0; i < oldlocset->locNr; i++) {
					// 
					// Run the evaluation with a node list made of a single item in the nodelocset.
					// 
					ctxt->context->P_Node = (xmlNode *)oldlocset->locTab[i]->user;
					ctxt->context->contextSize = oldlocset->locNr;
					ctxt->context->proximityPosition = i + 1;
					tmp = xmlXPathCacheNewNodeSet(ctxt->context, ctxt->context->P_Node);
					valuePush(ctxt, tmp);
					if(op->ch2 != -1)
						total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
					if(ctxt->error != XPATH_EXPRESSION_OK) {
						xmlXPathFreeObject(obj);
						return 0;
					}
					res = valuePop(ctxt);
					if(res->type == XPATH_LOCATIONSET) {
						xmlLocationSet * rloc = (xmlLocationSet *)res->user;
						for(j = 0; j<rloc->locNr; j++) {
							range = xmlXPtrNewRange((xmlNode *)oldlocset->locTab[i]->user,
								oldlocset->locTab[i]->index, (xmlNode *)rloc->locTab[j]->user2, rloc->locTab[j]->index2);
							if(range) {
								xmlXPtrLocationSetAdd(newlocset, range);
							}
						}
					}
					else {
						range = xmlXPtrNewRangeNodeObject((xmlNode *)oldlocset->locTab[i]->user, res);
						if(range)
							xmlXPtrLocationSetAdd(newlocset, range);
					}
					// Cleanup
					if(res) {
						xmlXPathReleaseObject(ctxt->context, res);
					}
					if(ctxt->value == tmp) {
						res = valuePop(ctxt);
						xmlXPathReleaseObject(ctxt->context, res);
					}
					ctxt->context->P_Node = NULL;
				}
			}
			else { /* Not a location set */
				CHECK_TYPE0(XPATH_NODESET);
				obj = valuePop(ctxt);
				oldset = obj->nodesetval;
				ctxt->context->P_Node = NULL;
				newlocset = xmlXPtrLocationSetCreate(NULL);
				if(oldset) {
					for(i = 0; i < oldset->nodeNr; i++) {
						//
						// Run the evaluation with a node list made of a single item in the nodeset.
						//			
						ctxt->context->P_Node = oldset->PP_NodeTab[i];
						// OPTIMIZE TODO: Avoid recreation for every iteration.
						tmp = xmlXPathCacheNewNodeSet(ctxt->context, ctxt->context->P_Node);
						valuePush(ctxt, tmp);
						if(op->ch2 != -1)
							total += xmlXPathCompOpEval(ctxt, &comp->steps[op->ch2]);
						if(ctxt->error != XPATH_EXPRESSION_OK) {
							xmlXPathFreeObject(obj);
							return 0;
						}
						res = valuePop(ctxt);
						range = xmlXPtrNewRangeNodeObject(oldset->PP_NodeTab[i], res);
						if(range)
							xmlXPtrLocationSetAdd(newlocset, range);
						// Cleanup
						if(res)
							xmlXPathReleaseObject(ctxt->context, res);
						if(ctxt->value == tmp) {
							res = valuePop(ctxt);
							xmlXPathReleaseObject(ctxt->context, res);
						}
						ctxt->context->P_Node = NULL;
					}
				}
			}

			/*
			    * The result is used as the new evaluation set.
			    */
			xmlXPathReleaseObject(ctxt->context, obj);
			ctxt->context->P_Node = NULL;
			ctxt->context->contextSize = -1;
			ctxt->context->proximityPosition = -1;
			valuePush(ctxt, xmlXPtrWrapLocationSet(newlocset));
			return total;
		}
#endif /* LIBXML_XPTR_ENABLED */
	}
	xmlGenericError(0, "XPath: unknown precompiled operation %d\n", op->op);
	ctxt->error = XPATH_INVALID_OPERAND;
	return total;
}
/**
 * xmlXPathCompOpEvalToBoolean:
 * @ctxt:  the XPath parser context
 *
 * Evaluates if the expression evaluates to true.
 *
 * Returns 1 if true, 0 if false and -1 on API or internal errors.
 */
static int xmlXPathCompOpEvalToBoolean(xmlXPathParserContext * ctxt, xmlXPathStepOp * op, int isPredicate)
{
	xmlXPathObject * resObj = NULL;
start:
	/* comp = ctxt->comp; */
	switch(op->op) {
		case XPATH_OP_END:
			return 0;
		case XPATH_OP_VALUE:
			resObj = (xmlXPathObject *)op->value4;
			return isPredicate ? xmlXPathEvaluatePredicateResult(ctxt, resObj) : xmlXPathCastToBoolean(resObj);
		case XPATH_OP_SORT:
			// We don't need sorting for boolean results. Skip this one.
			if(op->ch1 != -1) {
				op = &ctxt->comp->steps[op->ch1];
				goto start;
			}
			return 0;
		case XPATH_OP_COLLECT:
			if(op->ch1 == -1)
				return 0;
			xmlXPathCompOpEval(ctxt, &ctxt->comp->steps[op->ch1]);
			if(ctxt->error != XPATH_EXPRESSION_OK)
				return -1;
			xmlXPathNodeCollectAndTest(ctxt, op, NULL, NULL, 1);
			if(ctxt->error != XPATH_EXPRESSION_OK)
				return -1;
			resObj = valuePop(ctxt);
			if(resObj == NULL)
				return -1;
			break;
		default:
			// Fallback to call xmlXPathCompOpEval().
			xmlXPathCompOpEval(ctxt, op);
			if(ctxt->error != XPATH_EXPRESSION_OK)
				return -1;
			resObj = valuePop(ctxt);
			if(resObj == NULL)
				return -1;
			break;
	}
	if(resObj) {
		int res;
		if(resObj->type == XPATH_BOOLEAN) {
			res = resObj->boolval;
		}
		else if(isPredicate) {
			/*
				* For predicates a result of type "number" is handled
				* differently:
				* SPEC XPath 1.0:
				* "If the result is a number, the result will be converted
				*  to true if the number is equal to the context position
				*  and will be converted to false otherwise;"
				*/
			res = xmlXPathEvaluatePredicateResult(ctxt, resObj);
		}
		else {
			res = xmlXPathCastToBoolean(resObj);
		}
		xmlXPathReleaseObject(ctxt->context, resObj);
		return res;
	}
	return 0;
}

#ifdef XPATH_STREAMING
/**
 * xmlXPathRunStreamEval:
 * @ctxt:  the XPath parser context with the compiled expression
 *
 * Evaluate the Precompiled Streamable XPath expression in the given context.
 */
	static int xmlXPathRunStreamEval(xmlXPathContext * ctxt, xmlPattern * comp, xmlXPathObject ** resultSeq, int toBool)
	{
		int max_depth, min_depth;
		int from_root;
		int ret, depth;
		int eval_all_nodes;
		xmlNode * cur = NULL;
		xmlNode * limit = NULL;
		xmlStreamCtxtPtr patstream = NULL;
		int nb_nodes = 0;
		if(!ctxt || (comp == NULL))
			return -1;
		max_depth = xmlPatternMaxDepth(comp);
		if(max_depth == -1)
			return -1;
		if(max_depth == -2)
			max_depth = 10000;
		min_depth = xmlPatternMinDepth(comp);
		if(min_depth == -1)
			return -1;
		from_root = xmlPatternFromRoot(comp);
		if(from_root < 0)
			return -1;
#if 0
		printf("stream eval: depth %d from root %d\n", max_depth, from_root);
#endif
		if(!toBool) {
			if(resultSeq == NULL)
				return -1;
			*resultSeq = xmlXPathCacheNewNodeSet(ctxt, 0);
			if(*resultSeq == NULL)
				return -1;
		}
		/*
		 * handle the special cases of "/" amd "." being matched
		 */
		if(min_depth == 0) {
			if(from_root) {
				/* Select "/" */
				if(toBool)
					return 1;
				xmlXPathNodeSetAddUnique((*resultSeq)->nodesetval, (xmlNode *)ctxt->doc);
			}
			else {
				/* Select "self::node()" */
				if(toBool)
					return 1;
				xmlXPathNodeSetAddUnique((*resultSeq)->nodesetval, ctxt->P_Node);
			}
		}
		if(max_depth == 0) {
			return 0;
		}
		if(from_root) {
			cur = (xmlNode *)ctxt->doc;
		}
		else if(ctxt->P_Node) {
			switch(ctxt->P_Node->type) {
				case XML_ELEMENT_NODE:
				case XML_DOCUMENT_NODE:
				case XML_DOCUMENT_FRAG_NODE:
				case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
				case XML_DOCB_DOCUMENT_NODE:
#endif
				    cur = ctxt->P_Node;
				    break;
				case XML_ATTRIBUTE_NODE:
				case XML_TEXT_NODE:
				case XML_CDATA_SECTION_NODE:
				case XML_ENTITY_REF_NODE:
				case XML_ENTITY_NODE:
				case XML_PI_NODE:
				case XML_COMMENT_NODE:
				case XML_NOTATION_NODE:
				case XML_DTD_NODE:
				case XML_DOCUMENT_TYPE_NODE:
				case XML_ELEMENT_DECL:
				case XML_ATTRIBUTE_DECL:
				case XML_ENTITY_DECL:
				case XML_NAMESPACE_DECL:
				case XML_XINCLUDE_START:
				case XML_XINCLUDE_END:
				    break;
			}
			limit = cur;
		}
		if(!cur) {
			return 0;
		}
		patstream = xmlPatternGetStreamCtxt(comp);
		if(patstream == NULL) {
			/*
			 * QUESTION TODO: Is this an error?
			 */
			return 0;
		}
		eval_all_nodes = xmlStreamWantsAnyNode(patstream);
		if(from_root) {
			ret = xmlStreamPush(patstream, 0, 0);
			if(ret < 0) {
			}
			else if(ret == 1) {
				if(toBool)
					goto return_1;
				xmlXPathNodeSetAddUnique((*resultSeq)->nodesetval, cur);
			}
		}
		depth = 0;
		goto scan_children;
next_node:
		do {
			nb_nodes++;
			switch(cur->type) {
				case XML_ELEMENT_NODE:
				case XML_TEXT_NODE:
				case XML_CDATA_SECTION_NODE:
				case XML_COMMENT_NODE:
				case XML_PI_NODE:
				    if(cur->type == XML_ELEMENT_NODE) {
					    ret = xmlStreamPush(patstream, cur->name, (cur->ns ? cur->ns->href : NULL));
				    }
				    else if(eval_all_nodes)
					    ret = xmlStreamPushNode(patstream, NULL, NULL, cur->type);
				    else
					    break;

				    if(ret < 0) {
					    /* NOP. */
				    }
				    else if(ret == 1) {
					    if(toBool)
						    goto return_1;
					    if(xmlXPathNodeSetAddUnique((*resultSeq)->nodesetval, cur) < 0) {
						    ctxt->lastError.domain = XML_FROM_XPATH;
						    ctxt->lastError.code = XML_ERR_NO_MEMORY;
					    }
				    }
				    if((cur->children == NULL) || (depth >= max_depth)) {
					    ret = xmlStreamPop(patstream);
					    while(cur->next) {
						    cur = cur->next;
						    if((cur->type != XML_ENTITY_DECL) && (cur->type != XML_DTD_NODE))
							    goto next_node;
					    }
				    }
				default:
				    break;
			}
scan_children:
			if(cur->type == XML_NAMESPACE_DECL)
				break;
			if(cur->children && (depth < max_depth)) {
				/*
				 * Do not descend on entities declarations
				 */
				if(cur->children->type != XML_ENTITY_DECL) {
					cur = cur->children;
					depth++;
					/*
					 * Skip DTDs
					 */
					if(cur->type != XML_DTD_NODE)
						continue;
				}
			}
			if(cur == limit)
				break;
			while(cur->next) {
				cur = cur->next;
				if(!oneof2(cur->type, XML_ENTITY_DECL, XML_DTD_NODE))
					goto next_node;
			}
			do {
				cur = cur->P_ParentNode;
				depth--;
				if(!cur || (cur == limit))
					goto done;
				if(cur->type == XML_ELEMENT_NODE) {
					ret = xmlStreamPop(patstream);
				}
				else if(eval_all_nodes && oneof4(cur->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE, XML_COMMENT_NODE, XML_PI_NODE)) {
					ret = xmlStreamPop(patstream);
				}
				if(cur->next) {
					cur = cur->next;
					break;
				}
			} while(cur);
		} while(cur && depth >= 0);
done:
#if 0
		printf("stream eval: checked %d nodes selected %d\n", nb_nodes, retObj->nodesetval->nodeNr);
#endif
		xmlFreeStreamCtxt(patstream);
		return 0;
return_1:
		xmlFreeStreamCtxt(patstream);
		return 1;
	}

#endif /* XPATH_STREAMING */

/**
 * xmlXPathRunEval:
 * @ctxt:  the XPath parser context with the compiled expression
 * @toBool:  evaluate to a boolean result
 *
 * Evaluate the Precompiled XPath expression in the given context.
 */
	static int xmlXPathRunEval(xmlXPathParserContext * ctxt, int toBool)
	{
		xmlXPathCompExprPtr comp;
		if(!ctxt || (ctxt->comp == NULL))
			return -1;
		if(ctxt->valueTab == NULL) {
			/* Allocate the value stack */
			ctxt->valueTab = static_cast<xmlXPathObject **>(SAlloc::M(10 * sizeof(xmlXPathObject *)));
			if(ctxt->valueTab == NULL) {
				xmlXPathPErrMemory(ctxt, "creating evaluation context\n");
				SAlloc::F(ctxt);
			}
			ctxt->valueNr = 0;
			ctxt->valueMax = 10;
			ctxt->value = NULL;
			ctxt->valueFrame = 0;
		}
#ifdef XPATH_STREAMING
		if(ctxt->comp->stream) {
			int res;
			if(toBool) {
				/*
				 * Evaluation to boolean result.
				 */
				res = xmlXPathRunStreamEval(ctxt->context, ctxt->comp->stream, NULL, 1);
				if(res != -1)
					return res;
			}
			else {
				xmlXPathObject * resObj = NULL;
				/*
				 * Evaluation to a sequence.
				 */
				res = xmlXPathRunStreamEval(ctxt->context, ctxt->comp->stream, &resObj, 0);
				if((res != -1) && resObj) {
					valuePush(ctxt, resObj);
					return 0;
				}
				if(resObj)
					xmlXPathReleaseObject(ctxt->context, resObj);
			}
			/*
			 * QUESTION TODO: This falls back to normal XPath evaluation
			 * if res == -1. Is this intended?
			 */
		}
#endif
		comp = ctxt->comp;
		if(comp->last < 0) {
			xmlGenericError(0, "xmlXPathRunEval: last is less than zero\n");
			return -1;
		}
		if(toBool)
			return xmlXPathCompOpEvalToBoolean(ctxt, &comp->steps[comp->last], 0);
		else
			xmlXPathCompOpEval(ctxt, &comp->steps[comp->last]);
		return 0;
	}
// 
// Public interfaces
// 
/**
 * xmlXPathEvalPredicate:
 * @ctxt:  the XPath context
 * @res:  the Predicate Expression evaluation result
 *
 * Evaluate a predicate result for the current node.
 * A PredicateExpr is evaluated by evaluating the Expr and converting
 * the result to a boolean. If the result is a number, the result will
 * be converted to true if the number is equal to the position of the
 * context node in the context node list (as returned by the position
 * function) and will be converted to false otherwise; if the result
 * is not a number, then the result will be converted as if by a call
 * to the boolean function.
 *
 * Returns 1 if predicate is true, 0 otherwise
 */
int xmlXPathEvalPredicate(xmlXPathContext * ctxt, xmlXPathObject * res)
{
	if(ctxt && res) {
		switch(res->type) {
			case XPATH_BOOLEAN: return (res->boolval);
			case XPATH_NUMBER: return (res->floatval == ctxt->proximityPosition);
			case XPATH_NODESET:
			case XPATH_XSLT_TREE: return res->nodesetval ? (res->nodesetval->nodeNr != 0) : 0;
			case XPATH_STRING: return (res->stringval && (sstrlen(res->stringval) != 0));
			default: STRANGE
		}
	}
	return 0;
}
/**
 * xmlXPathEvaluatePredicateResult:
 * @ctxt:  the XPath Parser context
 * @res:  the Predicate Expression evaluation result
 *
 * Evaluate a predicate result for the current node.
 * A PredicateExpr is evaluated by evaluating the Expr and converting
 * the result to a boolean. If the result is a number, the result will
 * be converted to true if the number is equal to the position of the
 * context node in the context node list (as returned by the position
 * function) and will be converted to false otherwise; if the result
 * is not a number, then the result will be converted as if by a call
 * to the boolean function.
 *
 * Returns 1 if predicate is true, 0 otherwise
 */
int xmlXPathEvaluatePredicateResult(xmlXPathParserContext * ctxt, xmlXPathObject * res)
{
	if(!ctxt || !res)
		return 0;
	switch(res->type) {
		case XPATH_BOOLEAN:
			return (res->boolval);
		case XPATH_NUMBER:
#if defined(__BORLANDC__) || (defined(_MSC_VER) && (_MSC_VER == 1200))
			return ((res->floatval == ctxt->context->proximityPosition) && (!fisnan(res->floatval))); /* MSC pbm Mark Vakoc !*/
#else
			return (res->floatval == ctxt->context->proximityPosition);
#endif
		case XPATH_NODESET:
		case XPATH_XSLT_TREE:
			return res->nodesetval ? (res->nodesetval->nodeNr != 0) : 0;
		case XPATH_STRING:
			return (res->stringval && (res->stringval[0] != 0));
#ifdef LIBXML_XPTR_ENABLED
		case XPATH_LOCATIONSET: {
			xmlLocationSet * ptr = (xmlLocationSet *)res->user;
			return ptr ? (ptr->locNr != 0) : 0;
		}
#endif
		default:
			STRANGE
	}
	return 0;
}

#ifdef XPATH_STREAMING
/**
 * xmlXPathTryStreamCompile:
 * @ctxt: an XPath context
 * @str:  the XPath expression
 *
 * Try to compile the XPath expression as a streamable subset.
 *
 * Returns the compiled expression or NULL if failed to compile.
 */
static xmlXPathCompExprPtr xmlXPathTryStreamCompile(xmlXPathContext * ctxt, const xmlChar * str)
{
	// 
	// Optimization: use streaming patterns when the XPath expression can
	// be compiled to a stream lookup
	// 
	xmlPattern * stream;
	xmlXPathCompExprPtr comp;
	xmlDict * dict = NULL;
	const xmlChar ** namespaces = NULL;
	xmlNs * ns;
	int i, j;
	if((!xmlStrchr(str, '[')) && (!xmlStrchr(str, '(')) && (!xmlStrchr(str, '@'))) {
		/*
			* We don't try to handle expressions using the verbose axis
			* specifiers ("::"), just the simplied form at this point.
			* Additionally, if there is no list of namespaces available and
			*  there's a ":" in the expression, indicating a prefixed QName,
			*  then we won't try to compile either. xmlPatterncompile() needs
			*  to have a list of namespaces at compilation time in order to
			*  compile prefixed name tests.
			*/
		const xmlChar * tmp = xmlStrchr(str, ':');
		if(tmp && (!ctxt || (ctxt->nsNr == 0) || (tmp[1] == ':')))
			return 0;
		if(ctxt) {
			dict = ctxt->dict;
			if(ctxt->nsNr > 0) {
				namespaces = static_cast<const xmlChar **>(SAlloc::M(2 * (ctxt->nsNr + 1) * sizeof(xmlChar *)));
				if(!namespaces) {
					xmlXPathErrMemory(ctxt, "allocating namespaces array");
					return 0;
				}
				for(i = 0, j = 0; (j < ctxt->nsNr); j++) {
					ns = ctxt->namespaces[j];
					namespaces[i++] = ns->href;
					namespaces[i++] = ns->prefix;
				}
				namespaces[i++] = NULL;
				namespaces[i] = NULL;
			}
		}
		stream = xmlPatterncompile(str, dict, XML_PATTERN_XPATH, &namespaces[0]);
		SAlloc::F((xmlChar **)namespaces);
		if(stream && (xmlPatternStreamable(stream) == 1)) {
			comp = xmlXPathNewCompExpr();
			if(comp == NULL) {
				xmlXPathErrMemory(ctxt, "allocating streamable expression");
				return 0;
			}
			comp->stream = stream;
			comp->dict = dict;
			if(comp->dict)
				xmlDictReference(comp->dict);
			return (comp);
		}
		xmlFreePattern(stream);
	}
	return 0;
}

#endif /* XPATH_STREAMING */

static void xmlXPathOptimizeExpression(xmlXPathCompExprPtr comp, xmlXPathStepOp * op)
{
	// 
	// Try to rewrite "descendant-or-self::node()/foo" to an optimized internal representation.
	// 
	if((op->op == XPATH_OP_COLLECT /* 11 */) && (op->ch1 != -1) && (op->ch2 == -1 /* no predicate */)) {
		xmlXPathStepOp * prevop = &comp->steps[op->ch1];
		if((prevop->op == XPATH_OP_COLLECT /* 11 */) && ((xmlXPathAxisVal)prevop->value == AXIS_DESCENDANT_OR_SELF) &&
			(prevop->ch2 == -1) && ((xmlXPathTestVal)prevop->value2 == NODE_TEST_TYPE) && ((xmlXPathTypeVal)prevop->value3 == NODE_TYPE_NODE)) {
			// 
			// This is a "descendant-or-self::node()" without predicates.
			// Try to eliminate it.
			// 
			switch((xmlXPathAxisVal)op->value) {
				case AXIS_CHILD:
				case AXIS_DESCENDANT:
					/*
					    * Convert "descendant-or-self::node()/child::" or
					    * "descendant-or-self::node()/descendant::" to
					    * "descendant::"
					    */
					op->ch1   = prevop->ch1;
					op->value = AXIS_DESCENDANT;
					break;
				case AXIS_SELF:
				case AXIS_DESCENDANT_OR_SELF:
					/*
					    * Convert "descendant-or-self::node()/self::" or
					    * "descendant-or-self::node()/descendant-or-self::" to
					    * to "descendant-or-self::"
					    */
					op->ch1   = prevop->ch1;
					op->value = AXIS_DESCENDANT_OR_SELF;
					break;
				default:
					break;
			}
		}
	}

	/* Recurse */
	if(op->ch1 != -1)
		xmlXPathOptimizeExpression(comp, &comp->steps[op->ch1]);
	if(op->ch2 != -1)
		xmlXPathOptimizeExpression(comp, &comp->steps[op->ch2]);
}
/**
 * xmlXPathCtxtCompile:
 * @ctxt: an XPath context
 * @str:  the XPath expression
 *
 * Compile an XPath expression
 *
 * Returns the xmlXPathCompExprPtr resulting from the compilation or NULL.
 *    the caller has to free the object.
 */
xmlXPathCompExprPtr xmlXPathCtxtCompile(xmlXPathContext * ctxt, const xmlChar * str)
{
	xmlXPathParserContext * pctxt;
	xmlXPathCompExprPtr comp;
#ifdef XPATH_STREAMING
	comp = xmlXPathTryStreamCompile(ctxt, str);
	if(comp)
		return (comp);
#endif
	xmlXPathInit();
	pctxt = xmlXPathNewParserContext(str, ctxt);
	if(pctxt == NULL)
		return NULL;
	xmlXPathCompileExpr(pctxt, 1);
	if(pctxt->error != XPATH_EXPRESSION_OK) {
		xmlXPathFreeParserContext(pctxt);
		return 0;
	}
	if(*pctxt->cur != 0) {
		/*
			* aleksey: in some cases this line prints *second* error message
			* (see bug #78858) and probably this should be fixed.
			* However, we are not sure that all error messages are printed
			* out in other places. It's not critical so we leave it as-is for now
			*/
		xmlXPatherror(pctxt,/*__FILE__, __LINE__,*/XPATH_EXPR_ERROR);
		comp = NULL;
	}
	else {
		comp = pctxt->comp;
		pctxt->comp = NULL;
	}
	xmlXPathFreeParserContext(pctxt);
	if(comp) {
		comp->expr = sstrdup(str);
#ifdef DEBUG_EVAL_COUNTS
		comp->string = sstrdup(str);
		comp->nb = 0;
#endif
		if((comp->nbStep > 1) && (comp->last >= 0)) {
			xmlXPathOptimizeExpression(comp, &comp->steps[comp->last]);
		}
	}
	return (comp);
}
/**
 * xmlXPathCompile:
 * @str:  the XPath expression
 *
 * Compile an XPath expression
 *
 * Returns the xmlXPathCompExprPtr resulting from the compilation or NULL.
 *    the caller has to free the object.
 */
xmlXPathCompExprPtr xmlXPathCompile(const xmlChar * str)
{
	return (xmlXPathCtxtCompile(NULL, str));
}
/**
 * xmlXPathCompiledEvalInternal:
 * @comp:  the compiled XPath expression
 * @ctxt:  the XPath context
 * @resObj: the resulting XPath object or NULL
 * @toBool: 1 if only a boolean result is requested
 *
 * Evaluate the Precompiled XPath expression in the given context.
 * The caller has to free @resObj.
 *
 * Returns the xmlXPathObjectPtr resulting from the evaluation or NULL.
 *    the caller has to free the object.
 */
	static int xmlXPathCompiledEvalInternal(xmlXPathCompExprPtr comp, xmlXPathContext * ctxt, xmlXPathObject ** resObj, int toBool)
	{
		xmlXPathParserContext * pctxt;
#ifndef LIBXML_THREAD_ENABLED
		static int reentance = 0;
#endif
		int res;
		CHECK_CTXT_NEG(ctxt)
		if(comp == NULL)
			return -1;
		xmlXPathInit();
#ifndef LIBXML_THREAD_ENABLED
		reentance++;
		if(reentance > 1)
			xmlXPathDisableOptimizer = 1;
#endif
#ifdef DEBUG_EVAL_COUNTS
		comp->nb++;
		if(comp->string && (comp->nb > 100)) {
			slfprintf_stderr("100 x %s\n", comp->string);
			comp->nb = 0;
		}
#endif
		pctxt = xmlXPathCompParserContext(comp, ctxt);
		res = xmlXPathRunEval(pctxt, toBool);
		if(resObj) {
			if(pctxt->value == NULL) {
				xmlGenericError(0, "xmlXPathCompiledEval: evaluation failed\n");
				*resObj = NULL;
			}
			else
				*resObj = valuePop(pctxt);
		}
		/*
		 * Pop all remaining objects from the stack.
		 */
		if(pctxt->valueNr > 0) {
			xmlXPathObject * tmp;
			int stack = 0;
			do {
				tmp = valuePop(pctxt);
				if(tmp) {
					stack++;
					xmlXPathReleaseObject(ctxt, tmp);
				}
			} while(tmp);
			if((stack != 0) && ((toBool) || ((resObj) && (*resObj)))) {
				xmlGenericError(0, "xmlXPathCompiledEval: %d objects left on the stack.\n", stack);
			}
		}
		if((pctxt->error != XPATH_EXPRESSION_OK) && (resObj) && (*resObj)) {
			xmlXPathFreeObject(*resObj);
			*resObj = NULL;
		}
		pctxt->comp = NULL;
		xmlXPathFreeParserContext(pctxt);
#ifndef LIBXML_THREAD_ENABLED
		reentance--;
#endif
		return res;
	}

/**
 * xmlXPathCompiledEval:
 * @comp:  the compiled XPath expression
 * @ctx:  the XPath context
 *
 * Evaluate the Precompiled XPath expression in the given context.
 *
 * Returns the xmlXPathObjectPtr resulting from the evaluation or NULL.
 *    the caller has to free the object.
 */
xmlXPathObject * xmlXPathCompiledEval(xmlXPathCompExprPtr comp, xmlXPathContext * ctx)
{
	xmlXPathObject * res = NULL;
	xmlXPathCompiledEvalInternal(comp, ctx, &res, 0);
	return res;
}
/**
 * xmlXPathCompiledEvalToBoolean:
 * @comp:  the compiled XPath expression
 * @ctxt:  the XPath context
 *
 * Applies the XPath boolean() function on the result of the given
 * compiled expression.
 *
 * Returns 1 if the expression evaluated to true, 0 if to false and
 *    -1 in API and internal errors.
 */
int xmlXPathCompiledEvalToBoolean(xmlXPathCompExprPtr comp, xmlXPathContext * ctxt)
{
	return (xmlXPathCompiledEvalInternal(comp, ctxt, NULL, 1));
}
/**
 * xmlXPathEvalExpr:
 * @ctxt:  the XPath Parser context
 *
 * Parse and evaluate an XPath expression in the given context,
 * then push the result on the context stack
 */
void xmlXPathEvalExpr(xmlXPathParserContext * ctxt)
{
#ifdef XPATH_STREAMING
	xmlXPathCompExprPtr comp;
#endif
	if(!ctxt) 
		return;
#ifdef XPATH_STREAMING
	comp = xmlXPathTryStreamCompile(ctxt->context, ctxt->base);
	if(comp) {
		xmlXPathFreeCompExpr(ctxt->comp);
		ctxt->comp = comp;
		if(ctxt->cur)
			while(*ctxt->cur != 0)
				ctxt->cur++;
	}
	else
#endif
	{
		xmlXPathCompileExpr(ctxt, 1);
		if((ctxt->error == XPATH_EXPRESSION_OK) && ctxt->comp && (ctxt->comp->nbStep > 1) && (ctxt->comp->last >= 0)) {
			xmlXPathOptimizeExpression(ctxt->comp, &ctxt->comp->steps[ctxt->comp->last]);
		}
	}
	CHECK_ERROR;
	xmlXPathRunEval(ctxt, 0);
}
/**
 * xmlXPathEval:
 * @str:  the XPath expression
 * @ctx:  the XPath context
 *
 * Evaluate the XPath Location Path in the given context.
 *
 * Returns the xmlXPathObjectPtr resulting from the evaluation or NULL.
 *    the caller has to free the object.
 */
xmlXPathObject * xmlXPathEval(const xmlChar * str, xmlXPathContext * ctx)
{
	xmlXPathParserContext * ctxt;
	xmlXPathObject * res;
	xmlXPathObject * tmp;
	xmlXPathObject * init = NULL;
	int stack = 0;
	CHECK_CTXT(ctx)
	xmlXPathInit();
	ctxt = xmlXPathNewParserContext(str, ctx);
	if(!ctxt)
		return NULL;
	xmlXPathEvalExpr(ctxt);
	if(ctxt->value == NULL) {
		xmlGenericError(0, "xmlXPathEval: evaluation failed\n");
		res = NULL;
	}
	else if(*ctxt->cur && ctxt->comp
#ifdef XPATH_STREAMING
		&& (ctxt->comp->stream == NULL)
#endif
		) {
		xmlXPatherror(ctxt,/*__FILE__, __LINE__,*/XPATH_EXPR_ERROR);
		res = NULL;
	}
	else {
		res = valuePop(ctxt);
	}
	do {
		tmp = valuePop(ctxt);
		if(tmp) {
			if(tmp != init)
				stack++;
			xmlXPathReleaseObject(ctx, tmp);
		}
	} while(tmp);
	if(stack && res) {
		xmlGenericError(0, "xmlXPathEval: %d object left on the stack\n", stack);
	}
	if(ctxt->error != XPATH_EXPRESSION_OK) {
		xmlXPathFreeObject(res);
		res = NULL;
	}
	xmlXPathFreeParserContext(ctxt);
	return res;
}
/**
 * xmlXPathSetContextNode:
 * @node: the node to to use as the context node
 * @ctx:  the XPath context
 *
 * Sets 'node' as the context node. The node must be in the same
 * document as that associated with the context.
 *
 * Returns -1 in case of error or 0 if successful
 */
	int xmlXPathSetContextNode(xmlNode * pNode, xmlXPathContext * ctx)
	{
		if(pNode && ctx && pNode->doc == ctx->doc) {
			ctx->P_Node = pNode;
			return 0;
		}
		else
			return -1;
	}
/**
 * xmlXPathNodeEval:
 * @node: the node to to use as the context node
 * @str:  the XPath expression
 * @ctx:  the XPath context
 *
 * Evaluate the XPath Location Path in the given context. The node 'node'
 * is set as the context node. The context node is not restored.
 *
 * Returns the xmlXPathObjectPtr resulting from the evaluation or NULL.
 *    the caller has to free the object.
 */
	xmlXPathObject * xmlXPathNodeEval(xmlNode * pNode, const xmlChar * str, xmlXPathContext * ctx)
	{
		return (str && xmlXPathSetContextNode(pNode, ctx) >= 0) ? xmlXPathEval(str, ctx) : 0;
	}

/**
 * xmlXPathEvalExpression:
 * @str:  the XPath expression
 * @ctxt:  the XPath context
 *
 * Evaluate the XPath expression in the given context.
 *
 * Returns the xmlXPathObjectPtr resulting from the evaluation or NULL.
 *    the caller has to free the object.
 */
	xmlXPathObject * xmlXPathEvalExpression(const xmlChar * str, xmlXPathContext * ctxt)
	{
		xmlXPathParserContext * pctxt;
		xmlXPathObject * res = 0;
		xmlXPathObject * tmp;
		int stack = 0;
		CHECK_CTXT(ctxt)
		xmlXPathInit();
		pctxt = xmlXPathNewParserContext(str, ctxt);
		if(pctxt) {
			xmlXPathEvalExpr(pctxt);
			if(*pctxt->cur != 0 || pctxt->error != XPATH_EXPRESSION_OK) {
				xmlXPatherror(pctxt,/*__FILE__, __LINE__,*/XPATH_EXPR_ERROR);
				res = NULL;
			}
			else {
				res = valuePop(pctxt);
			}
			do {
				tmp = valuePop(pctxt);
				if(tmp) {
					xmlXPathReleaseObject(ctxt, tmp);
					stack++;
				}
			} while(tmp);
			if(stack && res) {
				xmlGenericError(0, "xmlXPathEvalExpression: %d object left on the stack\n", stack);
			}
			xmlXPathFreeParserContext(pctxt);
		}
		return res;
	}
// 
// Extra functions not pertaining to the XPath spec
// 
/**
 * xmlXPathEscapeUriFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the escape-uri() XPath function
 *  string escape-uri(string $str, bool $escape-reserved)
 *
 * This function applies the URI escaping rules defined in section 2 of [RFC
 * 2396] to the string supplied as $uri-part, which typically represents all
 * or part of a URI. The effect of the function is to replace any special
 * character in the string by an escape sequence of the form %xx%yy...,
 * where xxyy... is the hexadecimal representation of the octets used to
 * represent the character in UTF-8.
 *
 * The set of characters that are escaped depends on the setting of the
 * boolean argument $escape-reserved.
 *
 * If $escape-reserved is true, all characters are escaped other than lower
 * case letters a-z, upper case letters A-Z, digits 0-9, and the characters
 * referred to in [RFC 2396] as "marks": specifically, "-" | "_" | "." | "!"
 * | "~" | "*" | "'" | "(" | ")". The "%" character itself is escaped only
 * if it is not followed by two hexadecimal digits (that is, 0-9, a-f, and
 * A-F).
 *
 * If $escape-reserved is false, the behavior differs in that characters
 * referred to in [RFC 2396] as reserved characters are not escaped. These
 * characters are ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" | "$" | ",".
 *
 * [RFC 2396] does not define whether escaped URIs should use lower case or
 * upper case for hexadecimal digits. To ensure that escaped URIs can be
 * compared using string comparison functions, this function must always use
 * the upper-case letters A-F.
 *
 * Generally, $escape-reserved should be set to true when escaping a string
 * that is to form a single part of a URI, and to false when escaping an
 * entire URI or URI reference.
 *
 * In the case of non-ascii characters, the string is encoded according to
 * utf-8 and then converted according to RFC 2396.
 *
 * Examples
 *  xf:escape-uri ("gopher://spinaltap.micro.umn.edu/00/Weather/California/Los%20Angeles#ocean"), true())
 *  returns "gopher%3A%2F%2Fspinaltap.micro.umn.edu%2F00%2FWeather%2FCalifornia%2FLos%20Angeles%23ocean"
 *  xf:escape-uri ("gopher://spinaltap.micro.umn.edu/00/Weather/California/Los%20Angeles#ocean"), false())
 *  returns "gopher://spinaltap.micro.umn.edu/00/Weather/California/Los%20Angeles%23ocean"
 *
 */
	static void xmlXPathEscapeUriFunction(xmlXPathParserContext * ctxt, int nargs)
	{
		xmlXPathObject * str;
		int escape_reserved;
		xmlBuf * target;
		xmlChar * cptr;
		xmlChar escape[4];
		CHECK_ARITY(2);
		escape_reserved = xmlXPathPopBoolean(ctxt);
		CAST_TO_STRING;
		str = valuePop(ctxt);
		target = xmlBufCreate();
		escape[0] = '%';
		escape[3] = 0;
		if(target) {
			for(cptr = str->stringval; *cptr; cptr++) {
				const xmlChar c = *cptr;
				const xmlChar c1 = cptr[1];
				const xmlChar c2 = cptr[2];
				if(isasciialnum(c) || oneof9(c, '-', '_', '.', '!', '~', '*', '\'', '(', ')') || (c == '%' && ishex(c1) && ishex(c2)) ||
				    (!escape_reserved && oneof10(c, ';', '/', '?', ':', '@', '&', '=', '+', '$', ','))) {
					xmlBufAdd(target, cptr, 1);
				}
				else {
					if((c >> 4) < 10)
						escape[1] = '0' + (c >> 4);
					else
						escape[1] = 'A' - 10 + (c >> 4);
					if((c & 0xF) < 10)
						escape[2] = '0' + (c & 0xF);
					else
						escape[2] = 'A' - 10 + (c & 0xF);
					xmlBufAdd(target, &escape[0], 3);
				}
			}
		}
		valuePush(ctxt, xmlXPathCacheNewString(ctxt->context, xmlBufContent(target)));
		xmlBufFree(target);
		xmlXPathReleaseObject(ctxt->context, str);
	}

/**
 * xmlXPathRegisterAllFunctions:
 * @ctxt:  the XPath context
 *
 * Registers all default XPath functions in this context
 */
void xmlXPathRegisterAllFunctions(xmlXPathContext * ctxt)
{
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"boolean", xmlXPathBooleanFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"ceiling", xmlXPathCeilingFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"count", xmlXPathCountFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"concat", xmlXPathConcatFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"contains", xmlXPathContainsFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"id", xmlXPathIdFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"false", xmlXPathFalseFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"floor", xmlXPathFloorFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"last", xmlXPathLastFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"lang", xmlXPathLangFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"local-name", xmlXPathLocalNameFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"not", xmlXPathNotFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"name", xmlXPathNameFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"namespace-uri", xmlXPathNamespaceURIFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"normalize-space", xmlXPathNormalizeFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"number", xmlXPathNumberFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"position", xmlXPathPositionFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"round", xmlXPathRoundFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"string", xmlXPathStringFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"string-length", xmlXPathStringLengthFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"starts-with", xmlXPathStartsWithFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"substring", xmlXPathSubstringFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"substring-before", xmlXPathSubstringBeforeFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"substring-after", xmlXPathSubstringAfterFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"sum", xmlXPathSumFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"true", xmlXPathTrueFunction);
	xmlXPathRegisterFunc(ctxt, (const xmlChar *)"translate", xmlXPathTranslateFunction);
	xmlXPathRegisterFuncNS(ctxt, (const xmlChar *)"escape-uri", (const xmlChar *)"http://www.w3.org/2002/08/xquery-functions", xmlXPathEscapeUriFunction);
}

#endif /* LIBXML_XPATH_ENABLED */
#define bottom_xpath
