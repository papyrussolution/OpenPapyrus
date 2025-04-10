// pattern.c
// Implemetation of selectors for nodes
// Reference: http://www.w3.org/TR/2001/REC-xmlschema-1-20010502/
// to some extent http://www.w3.org/TR/1999/REC-xml-19991116
// See Copyright for the status of this software.
// daniel@veillard.com
// 
// @todo 
//   - compilation flags to check for specific syntaxes
//     using flags of xmlPatterncompile()
//   - making clear how pattern starting with / or . need to be handled,
//     currently push(NULL, NULL) means a reset of the streaming context
//     and indicating we are on / (the document node), probably need
//     something similar for .
//   - get rid of the "compile" starting with lowercase
//   - DONE (2006-05-16): get rid of the Strdup/Strndup in case of dictionary
// 
#include <slib-internal.h>
#pragma hdrstop

#ifdef LIBXML_PATTERN_ENABLED

/* #define DEBUG_STREAMING */
#ifdef ERROR
	#undef ERROR
#endif
#define ERROR(a, b, c, d)
#define ERROR5(a, b, c, d, e)

#define XML_STREAM_STEP_DESC    1
#define XML_STREAM_STEP_FINAL   2
#define XML_STREAM_STEP_ROOT    4
#define XML_STREAM_STEP_ATTR    8
#define XML_STREAM_STEP_NODE    16
#define XML_STREAM_STEP_IN_SET  32

/*
 * NOTE: Those private flags (XML_STREAM_xxx) are used
 * in _xmlStreamCtxt->flag. They extend the public
 * xmlPatternFlags, so be carefull not to interfere with the
 * reserved values for xmlPatternFlags.
 */
#define XML_STREAM_FINAL_IS_ANY_NODE 1<<14
#define XML_STREAM_FROM_ROOT 1<<15
#define XML_STREAM_DESC 1<<16
/*
 * XML_STREAM_ANY_NODE is used for comparison against
 * xmlElementType enums, to indicate a node of any type.
 */
#define XML_STREAM_ANY_NODE 100

#define XML_PATTERN_NOTPATTERN  (XML_PATTERN_XPATH|XML_PATTERN_XSSEL|XML_PATTERN_XSFIELD)
#define XML_STREAM_XS_IDC(c) ((c)->flags & (XML_PATTERN_XSSEL | XML_PATTERN_XSFIELD))
#define XML_STREAM_XS_IDC_SEL(c) ((c)->flags & XML_PATTERN_XSSEL)
#define XML_STREAM_XS_IDC_FIELD(c) ((c)->flags & XML_PATTERN_XSFIELD)
#define XML_PAT_COPY_NSNAME(c, r, nsname) r = ((c)->comp->dict) ? const_cast<xmlChar *>(xmlDictLookupSL((c)->comp->dict, reinterpret_cast<const xmlChar *>(nsname))) : sstrdup(reinterpret_cast<const xmlChar *>(nsname));
#define XML_PAT_FREE_STRING(c, r) if((c)->comp->dict == NULL) SAlloc::F(r);

struct xmlStreamStep {
	int flags; /* properties of that step */
	const xmlChar * name; /* first string value if NULL accept all */
	const xmlChar * ns; /* second string value */
	int nodeType; /* type of node */
};

//typedef struct _xmlStreamStep xmlStreamStep;
//typedef xmlStreamStep * xmlStreamStepPtr;

struct xmlStreamComp {
	xmlDict * dict; /* the dictionary if any */
	int nbStep; /* number of steps in the automata */
	int maxStep; /* allocated number of steps */
	xmlStreamStep * steps; /* the array of steps */
	int flags;
};

//typedef struct _xmlStreamComp xmlStreamComp;
typedef xmlStreamComp * xmlStreamCompPtr;

struct _xmlStreamCtxt {
	struct _xmlStreamCtxt * next; /* link to next sub pattern if | */
	xmlStreamCompPtr comp; /* the compiled stream */
	int nbState; /* number of states in the automata */
	int maxState; /* allocated number of states */
	int level; /* how deep are we ? */
	int * states; /* the array of step indexes */
	int flags; /* validation options */
	int blockLevel;
};
/*
 * Types are private:
 */
typedef enum {
	XML_OP_END = 0,
	XML_OP_ROOT,
	XML_OP_ELEM,
	XML_OP_CHILD,
	XML_OP_ATTR,
	XML_OP_PARENT,
	XML_OP_ANCESTOR,
	XML_OP_NS,
	XML_OP_ALL
} xmlPatOp;

struct xmlStepState {
	int step;
	xmlNode * P_Node;
};

//typedef struct _xmlStepState xmlStepState;
typedef xmlStepState * xmlStepStatePtr;

struct xmlStepStates {
	int nbstates;
	int maxstates;
	xmlStepState * states;
};

//typedef struct _xmlStepStates xmlStepStates;
typedef xmlStepStates * xmlStepStatesPtr;

struct xmlStepOp {
	xmlPatOp op;
	const xmlChar * value;
	const xmlChar * value2; /* The namespace name */
};

//typedef struct _xmlStepOp xmlStepOp;
//typedef xmlStepOp * xmlStepOpPtr;

#define PAT_FROM_ROOT   (1<<8)
#define PAT_FROM_CUR    (1<<9)

struct xmlPattern {
	void * data; /* the associated template */
	xmlDict * dict; /* the optional dictionary */
	xmlPattern * next; /* next pattern if | is used */
	const xmlChar * pattern; /* the pattern */
	int flags; /* flags */
	int nbStep;
	int maxStep;
	xmlStepOp * steps; /* ops for computation */
	xmlStreamComp * stream; /* the streaming data if any */
};

struct xmlPatParserContext {
	const xmlChar * cur; /* the current char being parsed */
	const xmlChar * base; /* the full expression */
	int error; /* error code */
	xmlDict * dict; /* the dictionary if any */
	xmlPattern * comp; /* the result */
	xmlNode * elem; /* the current node if any */
	const xmlChar ** namespaces; /* the namespaces definitions */
	int nb_namespaces; /* the number of namespaces */
};

//typedef struct _xmlPatParserContext xmlPatParserContext;
typedef xmlPatParserContext * xmlPatParserContextPtr;
// 
// Type functions
// 
/**
 * xmlNewPattern:
 *
 * Create a new XSLT Pattern
 *
 * Returns the newly allocated xmlPatternPtr or NULL in case of error
 */
static xmlPattern * xmlNewPattern()
{
	xmlPattern * cur = static_cast<xmlPattern *>(SAlloc::M(sizeof(xmlPattern)));
	if(!cur) {
		ERROR(0, 0, 0, __FUNCTION__ ": malloc failed\n");
	}
	else {
		memzero(cur, sizeof(xmlPattern));
		cur->maxStep = 10;
		cur->steps = static_cast<xmlStepOp *>(SAlloc::M(cur->maxStep * sizeof(xmlStepOp)));
		if(cur->steps == NULL) {
			SAlloc::F(cur);
			ERROR(0, 0, 0, __FUNCTION__ ": malloc failed\n");
			cur = 0;
		}
	}
	return cur;
}
/**
 * @comp: the compiled pattern for streaming
 *
 * Free the compiled pattern for streaming
 */
static void FASTCALL xmlFreeStreamComp(xmlStreamComp * comp)
{
	if(comp) {
		SAlloc::F(comp->steps);
		xmlDictFree(comp->dict);
		SAlloc::F(comp);
	}
}
/**
 * xmlFreePattern:
 * @comp:  an XSLT comp
 *
 * Free up the memory allocated by @comp
 */
void xmlFreePattern(xmlPattern * comp)
{
	if(comp) {
		xmlFreePattern(comp->next); // @recursion
		xmlFreeStreamComp(comp->stream);
		SAlloc::F((xmlChar *)comp->pattern);
		if(comp->steps) {
			if(comp->dict == NULL) {
				for(int i = 0; i < comp->nbStep; i++) {
					xmlStepOp * op = &comp->steps[i];
					SAlloc::F((xmlChar *)op->value);
					SAlloc::F((xmlChar *)op->value2);
				}
			}
			SAlloc::F(comp->steps);
		}
		xmlDictFree(comp->dict);
		memset(comp, -1, sizeof(xmlPattern));
		SAlloc::F(comp);
	}
}
/**
 * xmlFreePatternList:
 * @comp:  an XSLT comp list
 *
 * Free up the memory allocated by all the elements of @comp
 */
void xmlFreePatternList(xmlPattern * comp)
{
	while(comp) {
		xmlPattern * cur = comp;
		comp = comp->next;
		cur->next = NULL;
		xmlFreePattern(cur);
	}
}
/**
 * xmlNewPatParserContext:
 * @pattern:  the pattern context
 * @dict:  the inherited dictionary or NULL
 * @namespaces: the prefix definitions, array of [URI, prefix] terminated
 *   with [NULL, NULL] or NULL if no namespace is used
 *
 * Create a new XML pattern parser context
 *
 * Returns the newly allocated xmlPatParserContextPtr or NULL in case of error
 */
static xmlPatParserContextPtr xmlNewPatParserContext(const xmlChar * pattern, xmlDict * dict, const xmlChar ** namespaces)
{
	xmlPatParserContext * cur;
	if(pattern == NULL)
		return 0;
	cur = (xmlPatParserContextPtr)SAlloc::M(sizeof(xmlPatParserContext));
	if(!cur) {
		ERROR(0, 0, 0, __FUNCTION__ ": malloc failed\n");
		return 0;
	}
	memzero(cur, sizeof(xmlPatParserContext));
	cur->dict = dict;
	cur->cur = pattern;
	cur->base = pattern;
	if(namespaces) {
		int i;
		for(i = 0; namespaces[2 * i]; i++)
			;
		cur->nb_namespaces = i;
	}
	else {
		cur->nb_namespaces = 0;
	}
	cur->namespaces = namespaces;
	return cur;
}

/**
 * xmlFreePatParserContext:
 * @ctxt:  an XSLT parser context
 *
 * Free up the memory allocated by @ctxt
 */
static void xmlFreePatParserContext(xmlPatParserContextPtr ctxt)
{
	if(ctxt) {
		memset(ctxt, -1, sizeof(xmlPatParserContext));
		SAlloc::F(ctxt);
	}
}

/**
 * xmlPatternAdd:
 * @comp:  the compiled match expression
 * @op:  an op
 * @value:  the first value
 * @value2:  the second value
 *
 * Add a step to an XSLT Compiled Match
 *
 * Returns -1 in case of failure, 0 otherwise.
 */
static int FASTCALL xmlPatternAdd(xmlPatParserContextPtr ctxt ATTRIBUTE_UNUSED, xmlPattern * comp, xmlPatOp op, const xmlChar * value, const xmlChar * value2)
{
	if(comp->nbStep >= comp->maxStep) {
		xmlStepOp * temp = static_cast<xmlStepOp *>(SAlloc::R(comp->steps, comp->maxStep * 2 * sizeof(xmlStepOp)));
		if(temp == NULL) {
			ERROR(ctxt, NULL, NULL, "xmlPatternAdd: realloc failed\n");
			return -1;
		}
		comp->steps = temp;
		comp->maxStep *= 2;
	}
	comp->steps[comp->nbStep].op = op;
	comp->steps[comp->nbStep].value = value;
	comp->steps[comp->nbStep].value2 = value2;
	comp->nbStep++;
	return 0;
}

#if 0
/**
 * xsltSwapTopPattern:
 * @comp:  the compiled match expression
 *
 * reverse the two top steps.
 */
static void xsltSwapTopPattern(xmlPattern * comp)
{
	int j = comp->nbStep - 1;
	if(j > 0) {
		xmlPatOp op;
		int i = j - 1;
		const xmlChar * tmp = comp->steps[i].value;
		comp->steps[i].value = comp->steps[j].value;
		comp->steps[j].value = tmp;
		tmp = comp->steps[i].value2;
		comp->steps[i].value2 = comp->steps[j].value2;
		comp->steps[j].value2 = tmp;
		op = comp->steps[i].op;
		comp->steps[i].op = comp->steps[j].op;
		comp->steps[j].op = op;
	}
}

#endif

/**
 * xmlReversePattern:
 * @comp:  the compiled match expression
 *
 * reverse all the stack of expressions
 *
 * returns 0 in case of success and -1 in case of error.
 */
static int xmlReversePattern(xmlPattern * comp)
{
	int i, j;
	/*
	 * remove the leading // for //a or .//a
	 */
	if((comp->nbStep > 0) && (comp->steps[0].op == XML_OP_ANCESTOR)) {
		for(i = 0, j = 1; j < comp->nbStep; i++, j++) {
			comp->steps[i].value = comp->steps[j].value;
			comp->steps[i].value2 = comp->steps[j].value2;
			comp->steps[i].op = comp->steps[j].op;
		}
		comp->nbStep--;
	}
	if(comp->nbStep >= comp->maxStep) {
		xmlStepOp * temp = static_cast<xmlStepOp *>(SAlloc::R(comp->steps, comp->maxStep * 2 * sizeof(xmlStepOp)));
		if(temp == NULL) {
			ERROR(ctxt, NULL, NULL, "xmlReversePattern: realloc failed\n");
			return -1;
		}
		comp->steps = temp;
		comp->maxStep *= 2;
	}
	i = 0;
	j = comp->nbStep - 1;
	while(j > i) {
		xmlPatOp op;
		const xmlChar * tmp = comp->steps[i].value;
		comp->steps[i].value = comp->steps[j].value;
		comp->steps[j].value = tmp;
		tmp = comp->steps[i].value2;
		comp->steps[i].value2 = comp->steps[j].value2;
		comp->steps[j].value2 = tmp;
		op = comp->steps[i].op;
		comp->steps[i].op = comp->steps[j].op;
		comp->steps[j].op = op;
		j--;
		i++;
	}
	comp->steps[comp->nbStep].value = NULL;
	comp->steps[comp->nbStep].value2 = NULL;
	comp->steps[comp->nbStep++].op = XML_OP_END;
	return 0;
}
//
// The interpreter for the precompiled patterns
//
static int xmlPatPushState(xmlStepStates * states, int step, xmlNode * P_Node)
{
	if((states->states == NULL) || (states->maxstates <= 0)) {
		states->maxstates = 4;
		states->nbstates = 0;
		states->states = static_cast<xmlStepState *>(SAlloc::M(4 * sizeof(xmlStepState)));
	}
	else if(states->maxstates <= states->nbstates) {
		xmlStepState * tmp = static_cast<xmlStepState *>(SAlloc::R(states->states, 2 * states->maxstates * sizeof(xmlStepState)));
		if(!tmp)
			return -1;
		states->states = tmp;
		states->maxstates *= 2;
	}
	states->states[states->nbstates].step = step;
	states->states[states->nbstates++].P_Node = P_Node;
#if 0
	slfprintf_stderr("Push: %d, %s\n", step, node->name);
#endif
	return 0;
}
/**
 * xmlPatMatch:
 * @comp: the precompiled pattern
 * @node: a node
 *
 * Test whether the node matches the pattern
 *
 * Returns 1 if it matches, 0 if it doesn't and -1 in case of failure
 */
static int xmlPatMatch(xmlPattern * comp, xmlNode * pNode)
{
	int i;
	xmlStepOp * step;
	xmlStepStates states = {0, 0, NULL}; /* // may require backtrack */
	if(!comp || !pNode)
		return -1;
	i = 0;
restart:
	for(; i < comp->nbStep; i++) {
		step = &comp->steps[i];
		switch(step->op) {
			case XML_OP_END:
			    goto found;
			case XML_OP_ROOT:
			    if(pNode->type == XML_NAMESPACE_DECL)
				    goto rollback;
			    pNode = pNode->P_ParentNode;
			    if((pNode->type == XML_DOCUMENT_NODE) ||
#ifdef LIBXML_DOCB_ENABLED
			    (pNode->type == XML_DOCB_DOCUMENT_NODE) ||
#endif
			    (pNode->type == XML_HTML_DOCUMENT_NODE))
				    continue;
			    goto rollback;
			case XML_OP_ELEM:
			    if(pNode->type != XML_ELEMENT_NODE)
				    goto rollback;
			    if(!step->value)
				    continue;
			    if(step->value[0] != pNode->name[0])
				    goto rollback;
			    if(!sstreq(step->value, pNode->name))
				    goto rollback;
			    /* Namespace test */
			    if(!pNode->ns) {
				    if(step->value2)
					    goto rollback;
			    }
			    else if(pNode->ns->href) {
				    if(!step->value2)
					    goto rollback;
				    if(!sstreq(step->value2, pNode->ns->href))
					    goto rollback;
			    }
			    continue;
			case XML_OP_CHILD: {
			    xmlNode * lst;
			    if((pNode->type != XML_ELEMENT_NODE) && (pNode->type != XML_DOCUMENT_NODE) &&
#ifdef LIBXML_DOCB_ENABLED
				    (pNode->type != XML_DOCB_DOCUMENT_NODE) &&
#endif
				    (pNode->type != XML_HTML_DOCUMENT_NODE))
				    goto rollback;
			    lst = pNode->children;
			    if(step->value) {
				    while(lst) {
					    if((lst->type == XML_ELEMENT_NODE) && (step->value[0] == lst->name[0]) && (sstreq(step->value, lst->name)))
						    break;
					    lst = lst->next;
				    }
				    if(lst)
					    continue;
			    }
			    goto rollback;
		    }
			case XML_OP_ATTR:
			    if(pNode->type != XML_ATTRIBUTE_NODE)
				    goto rollback;
			    if(step->value) {
				    if(step->value[0] != pNode->name[0])
					    goto rollback;
				    if(!sstreq(step->value, pNode->name))
					    goto rollback;
			    }
			    /* Namespace test */
			    if(!pNode->ns) {
				    if(step->value2)
					    goto rollback;
			    }
			    else if(step->value2) {
				    if(!sstreq(step->value2, pNode->ns->href))
					    goto rollback;
			    }
			    continue;
			case XML_OP_PARENT:
			    if((pNode->type == XML_DOCUMENT_NODE) || (pNode->type == XML_HTML_DOCUMENT_NODE) ||
#ifdef LIBXML_DOCB_ENABLED
			    (pNode->type == XML_DOCB_DOCUMENT_NODE) ||
#endif
			    (pNode->type == XML_NAMESPACE_DECL))
				    goto rollback;
			    pNode = pNode->P_ParentNode;
			    if(!pNode)
				    goto rollback;
			    if(!step->value)
				    continue;
			    if(step->value[0] != pNode->name[0])
				    goto rollback;
			    if(!sstreq(step->value, pNode->name))
				    goto rollback;
			    /* Namespace test */
			    if(!pNode->ns) {
				    if(step->value2)
					    goto rollback;
			    }
			    else if(pNode->ns->href) {
				    if(!step->value2)
					    goto rollback;
				    if(!sstreq(step->value2, pNode->ns->href))
					    goto rollback;
			    }
			    continue;
			case XML_OP_ANCESTOR:
			    /* @todo implement coalescing of ANCESTOR/NODE ops */
			    if(!step->value) {
				    i++;
				    step = &comp->steps[i];
				    if(step->op == XML_OP_ROOT)
					    goto found;
				    if(step->op != XML_OP_ELEM)
					    goto rollback;
				    if(!step->value)
					    return -1;
			    }
			    if(!pNode)
				    goto rollback;
			    if((pNode->type == XML_DOCUMENT_NODE) || (pNode->type == XML_HTML_DOCUMENT_NODE) ||
#ifdef LIBXML_DOCB_ENABLED
			    (pNode->type == XML_DOCB_DOCUMENT_NODE) ||
#endif
			    (pNode->type == XML_NAMESPACE_DECL))
				    goto rollback;
			    pNode = pNode->P_ParentNode;
			    while(pNode) {
				    if((pNode->type == XML_ELEMENT_NODE) && (step->value[0] == pNode->name[0]) && (sstreq(step->value, pNode->name))) {
					    /* Namespace test */
					    if(!pNode->ns) {
						    if(!step->value2)
							    break;
					    }
					    else if(pNode->ns->href) {
						    if(step->value2 && sstreq(step->value2, pNode->ns->href))
							    break;
					    }
				    }
				    pNode = pNode->P_ParentNode;
			    }
			    if(!pNode)
				    goto rollback;
			    /*
			 * prepare a potential rollback from here for ancestors of that node.
			     */
			    if(step->op == XML_OP_ANCESTOR)
				    xmlPatPushState(&states, i, pNode);
			    else
				    xmlPatPushState(&states, i-1, pNode);
			    continue;
			case XML_OP_NS:
			    if(pNode->type != XML_ELEMENT_NODE)
				    goto rollback;
			    if(!pNode->ns) {
				    if(step->value)
					    goto rollback;
			    }
			    else if(pNode->ns->href) {
				    if(!step->value)
					    goto rollback;
				    if(!sstreq(step->value, pNode->ns->href))
					    goto rollback;
			    }
			    break;
			case XML_OP_ALL:
			    if(pNode->type != XML_ELEMENT_NODE)
				    goto rollback;
			    break;
		}
	}
found:
	SAlloc::F(states.states); /* Free the rollback states */
	return 1;
rollback:
	/* got an error try to rollback */
	if(states.states == NULL)
		return 0;
	if(states.nbstates <= 0) {
		SAlloc::F(states.states);
		return 0;
	}
	states.nbstates--;
	i = states.states[states.nbstates].step;
	pNode = states.states[states.nbstates].P_Node;
#if 0
	slfprintf_stderr("Pop: %d, %s\n", i, node->name);
#endif
	goto restart;
}
// 
// Dedicated parser for templates
// 
#define TODO xmlGenericError(0, "Unimplemented block at %s:%d\n", __FILE__, __LINE__);
#define CUR  (*ctxt->cur)
#define SKIP(val) ctxt->cur += (val)
#define NXT(val) ctxt->cur[(val)]
#define PEEKPREV(val) ctxt->cur[-(val)]
#define CUR_PTR ctxt->cur
#define SKIP_BLANKS while(IS_BLANK_CH(CUR)) NEXT
#define CURRENT (*ctxt->cur)
#define NEXT ((*ctxt->cur) ? ctxt->cur++ : ctxt->cur)
#define PUSH(op, val, val2) if(xmlPatternAdd(ctxt, ctxt->comp, (op), (val), (val2))) goto error;
#define XSLT_ERROR(X) { xsltError(ctxt, __FILE__, __LINE__, X); ctxt->error = (X); return; }
#define XSLT_ERROR0(X) { xsltError(ctxt, __FILE__, __LINE__, X); ctxt->error = (X); return 0; }

#if 0
/**
 * xmlPatScanLiteral:
 * @ctxt:  the XPath Parser context
 *
 * Parse an XPath Litteral:
 *
 * [29] Literal ::= '"' [^"]* '"'
 *     | "'" [^']* "'"
 *
 * Returns the Literal parsed or NULL
 */

static xmlChar * xmlPatScanLiteral(xmlPatParserContextPtr ctxt)
{
	const xmlChar * q, * cur;
	xmlChar * ret = NULL;
	int val, len;
	SKIP_BLANKS;
	if(CUR == '"') {
		NEXT;
		cur = q = CUR_PTR;
		val = xmlStringCurrentChar(NULL, cur, &len);
		while((IS_CHAR(val)) && (val != '"')) {
			cur += len;
			val = xmlStringCurrentChar(NULL, cur, &len);
		}
		if(!IS_CHAR(val)) {
			ctxt->error = 1;
			return 0;
		}
		else {
			if(ctxt->dict)
				ret = (xmlChar *)xmlDictLookup(ctxt->dict, q, cur - q);
			else
				ret = xmlStrndup(q, cur - q);
		}
		cur += len;
		CUR_PTR = cur;
	}
	else if(CUR == '\'') {
		NEXT;
		cur = q = CUR_PTR;
		val = xmlStringCurrentChar(NULL, cur, &len);
		while((IS_CHAR(val)) && (val != '\'')) {
			cur += len;
			val = xmlStringCurrentChar(NULL, cur, &len);
		}
		if(!IS_CHAR(val)) {
			ctxt->error = 1;
			return 0;
		}
		else {
			if(ctxt->dict)
				ret = (xmlChar *)xmlDictLookup(ctxt->dict, q, cur - q);
			else
				ret = xmlStrndup(q, cur - q);
		}
		cur += len;
		CUR_PTR = cur;
	}
	else {
		/* XP_ERROR(XPATH_START_LITERAL_ERROR); */
		ctxt->error = 1;
		return 0;
	}
	return ret;
}

#endif

/**
 * xmlPatScanName:
 * @ctxt:  the XPath Parser context
 *
 * [4] NameChar ::= Letter | Digit | '.' | '-' | '_' |
 *       CombiningChar | Extender
 *
 * [5] Name ::= (Letter | '_' | ':') (NameChar)*
 *
 * [6] Names ::= Name (S Name)*
 *
 * Returns the Name parsed or NULL
 */
static xmlChar * xmlPatScanName(xmlPatParserContextPtr ctxt)
{
	const xmlChar * q, * cur;
	xmlChar * ret = NULL;
	int val, len;
	SKIP_BLANKS;
	cur = q = CUR_PTR;
	val = xmlStringCurrentChar(NULL, cur, &len);
	if(!IS_LETTER(val) && (val != '_') && (val != ':'))
		return 0;
	while((IS_LETTER(val)) || (isdec(val)) || (val == '.') || (val == '-') || (val == '_') || (IS_COMBINING(val)) || (IS_EXTENDER(val))) {
		cur += len;
		val = xmlStringCurrentChar(NULL, cur, &len);
	}
	ret = (ctxt->dict) ? (xmlChar *)(xmlDictLookup(ctxt->dict, q, cur - q)) : xmlStrndup(q, cur - q);
	CUR_PTR = cur;
	return ret;
}
/**
 * xmlPatScanNCName:
 * @ctxt:  the XPath Parser context
 *
 * Parses a non qualified name
 *
 * Returns the Name parsed or NULL
 */
static xmlChar * xmlPatScanNCName(xmlPatParserContextPtr ctxt)
{
	const xmlChar * q, * cur;
	xmlChar * ret = NULL;
	int val, len;
	SKIP_BLANKS;
	cur = q = CUR_PTR;
	val = xmlStringCurrentChar(NULL, cur, &len);
	if(!IS_LETTER(val) && (val != '_'))
		return 0;
	while((IS_LETTER(val)) || (isdec(val)) || oneof3(val, '.', '-', '_') || (IS_COMBINING(val)) || (IS_EXTENDER(val))) {
		cur += len;
		val = xmlStringCurrentChar(NULL, cur, &len);
	}
	ret = (ctxt->dict) ? (xmlChar *)xmlDictLookup(ctxt->dict, q, cur - q) : xmlStrndup(q, cur - q);
	CUR_PTR = cur;
	return ret;
}

#if 0
/**
 * xmlPatScanQName:
 * @ctxt:  the XPath Parser context
 * @prefix:  the place to store the prefix
 *
 * Parse a qualified name
 *
 * Returns the Name parsed or NULL
 */

static xmlChar * xmlPatScanQName(xmlPatParserContextPtr ctxt, xmlChar ** prefix)
{
	*prefix = NULL;
	xmlChar * ret = xmlPatScanNCName(ctxt);
	if(CUR == ':') {
		*prefix = ret;
		NEXT;
		ret = xmlPatScanNCName(ctxt);
	}
	return ret;
}

#endif
/**
 * xmlCompileAttributeTest:
 * @ctxt:  the compilation context
 *
 * Compile an attribute test.
 */
static void xmlCompileAttributeTest(xmlPatParserContextPtr ctxt)
{
	xmlChar * token = NULL;
	xmlChar * name = NULL;
	xmlChar * URL = NULL;
	SKIP_BLANKS;
	name = xmlPatScanNCName(ctxt);
	if(!name) {
		if(CUR == '*') {
			PUSH(XML_OP_ATTR, 0, 0);
			NEXT;
		}
		else {
			ERROR(0, 0, 0, "xmlCompileAttributeTest : Name expected\n");
			ctxt->error = 1;
		}
		return;
	}
	if(CUR == ':') {
		int i;
		xmlChar * prefix = name;
		NEXT;
		if(IS_BLANK_CH(CUR)) {
			ERROR5(NULL, NULL, NULL, "Invalid QName.\n", 0);
			XML_PAT_FREE_STRING(ctxt, prefix);
			ctxt->error = 1;
			goto error;
		}
		/*
		 * This is a namespace match
		 */
		token = xmlPatScanName(ctxt);
		if((prefix[0] == 'x') && (prefix[1] == 'm') && (prefix[2] == 'l') && (prefix[3] == 0)) {
			XML_PAT_COPY_NSNAME(ctxt, URL, XML_XML_NAMESPACE);
		}
		else {
			for(i = 0; i < ctxt->nb_namespaces; i++) {
				if(sstreq(ctxt->namespaces[2 * i + 1], prefix)) {
					XML_PAT_COPY_NSNAME(ctxt, URL, ctxt->namespaces[2 * i])
					break;
				}
			}
			if(i >= ctxt->nb_namespaces) {
				ERROR5(NULL, NULL, NULL, "xmlCompileAttributeTest : no namespace bound to prefix %s\n", prefix);
				ctxt->error = 1;
				goto error;
			}
		}
		XML_PAT_FREE_STRING(ctxt, prefix);
		if(token == NULL) {
			if(CUR == '*') {
				NEXT;
				PUSH(XML_OP_ATTR, NULL, URL);
			}
			else {
				ERROR(0, 0, 0, "xmlCompileAttributeTest : Name expected\n");
				ctxt->error = 1;
				goto error;
			}
		}
		else {
			PUSH(XML_OP_ATTR, token, URL);
		}
	}
	else {
		PUSH(XML_OP_ATTR, name, 0);
	}
	return;
error:
	if(URL)
		XML_PAT_FREE_STRING(ctxt, URL)
		if(token)
			XML_PAT_FREE_STRING(ctxt, token);
}

/**
 * xmlCompileStepPattern:
 * @ctxt:  the compilation context
 *
 * Compile the Step Pattern and generates a precompiled
 * form suitable for fast matching.
 *
 * [3]    Step    ::=    '.' | NameTest
 * [4]    NameTest    ::=    QName | '*' | NCName ':' '*'
 */

static void xmlCompileStepPattern(xmlPatParserContextPtr ctxt)
{
	xmlChar * token = NULL;
	xmlChar * name = NULL;
	xmlChar * URL = NULL;
	int hasBlanks = 0;
	SKIP_BLANKS;
	if(CUR == '.') {
		/*
		 * Context node.
		 */
		NEXT;
		PUSH(XML_OP_ELEM, 0, 0);
		return;
	}
	if(CUR == '@') {
		/*
		 * Attribute test.
		 */
		if(XML_STREAM_XS_IDC_SEL(ctxt->comp)) {
			ERROR5(NULL, NULL, NULL, "Unexpected attribute axis in '%s'.\n", ctxt->base);
			ctxt->error = 1;
			return;
		}
		NEXT;
		xmlCompileAttributeTest(ctxt);
		if(ctxt->error)
			goto error;
		return;
	}
	name = xmlPatScanNCName(ctxt);
	if(!name) {
		if(CUR == '*') {
			NEXT;
			PUSH(XML_OP_ALL, 0, 0);
			return;
		}
		else {
			ERROR(0, 0, 0, "xmlCompileStepPattern : Name expected\n");
			ctxt->error = 1;
			return;
		}
	}
	if(IS_BLANK_CH(CUR)) {
		hasBlanks = 1;
		SKIP_BLANKS;
	}
	if(CUR == ':') {
		NEXT;
		if(CUR != ':') {
			xmlChar * prefix = name;
			int i;
			if(hasBlanks || IS_BLANK_CH(CUR)) {
				ERROR5(NULL, NULL, NULL, "Invalid QName.\n", 0);
				ctxt->error = 1;
				goto error;
			}
			/*
			 * This is a namespace match
			 */
			token = xmlPatScanName(ctxt);
			if((prefix[0] == 'x') && (prefix[1] == 'm') && (prefix[2] == 'l') && (prefix[3] == 0)) {
				XML_PAT_COPY_NSNAME(ctxt, URL, XML_XML_NAMESPACE)
			}
			else {
				for(i = 0; i < ctxt->nb_namespaces; i++) {
					if(sstreq(ctxt->namespaces[2 * i + 1], prefix)) {
						XML_PAT_COPY_NSNAME(ctxt, URL, ctxt->namespaces[2 * i])
						break;
					}
				}
				if(i >= ctxt->nb_namespaces) {
					ERROR5(NULL, NULL, NULL, "xmlCompileStepPattern : no namespace bound to prefix %s\n", prefix);
					ctxt->error = 1;
					goto error;
				}
			}
			XML_PAT_FREE_STRING(ctxt, prefix);
			name = NULL;
			if(token == NULL) {
				if(CUR == '*') {
					NEXT;
					PUSH(XML_OP_NS, URL, 0);
				}
				else {
					ERROR(0, 0, 0, "xmlCompileStepPattern : Name expected\n");
					ctxt->error = 1;
					goto error;
				}
			}
			else {
				PUSH(XML_OP_ELEM, token, URL);
			}
		}
		else {
			NEXT;
			if(sstreq(name, (const xmlChar *)"child")) {
				XML_PAT_FREE_STRING(ctxt, name);
				name = xmlPatScanName(ctxt);
				if(!name) {
					if(CUR == '*') {
						NEXT;
						PUSH(XML_OP_ALL, 0, 0);
						return;
					}
					else {
						ERROR(0, 0, 0, "xmlCompileStepPattern : QName expected\n");
						ctxt->error = 1;
						goto error;
					}
				}
				if(CUR == ':') {
					xmlChar * prefix = name;
					int i;
					NEXT;
					if(IS_BLANK_CH(CUR)) {
						ERROR5(NULL, NULL, NULL, "Invalid QName.\n", 0);
						ctxt->error = 1;
						goto error;
					}
					/*
					 * This is a namespace match
					 */
					token = xmlPatScanName(ctxt);
					if((prefix[0] == 'x') && (prefix[1] == 'm') && (prefix[2] == 'l') && (prefix[3] == 0)) {
						XML_PAT_COPY_NSNAME(ctxt, URL, XML_XML_NAMESPACE)
					}
					else {
						for(i = 0; i < ctxt->nb_namespaces; i++) {
							if(sstreq(ctxt->namespaces[2 * i + 1], prefix)) {
								XML_PAT_COPY_NSNAME(ctxt, URL, ctxt->namespaces[2 * i])
								break;
							}
						}
						if(i >= ctxt->nb_namespaces) {
							ERROR5(NULL, NULL, NULL, "xmlCompileStepPattern : no namespace bound to prefix %s\n", prefix);
							ctxt->error = 1;
							goto error;
						}
					}
					XML_PAT_FREE_STRING(ctxt, prefix);
					name = NULL;
					if(token == NULL) {
						if(CUR == '*') {
							NEXT;
							PUSH(XML_OP_NS, URL, 0);
						}
						else {
							ERROR(0, 0, 0, "xmlCompileStepPattern : Name expected\n");
							ctxt->error = 1;
							goto error;
						}
					}
					else {
						PUSH(XML_OP_CHILD, token, URL);
					}
				}
				else
					PUSH(XML_OP_CHILD, name, 0);
				return;
			}
			else if(sstreq(name, (const xmlChar *)"attribute")) {
				XML_PAT_FREE_STRING(ctxt, name)
				name = NULL;
				if(XML_STREAM_XS_IDC_SEL(ctxt->comp)) {
					ERROR5(NULL, NULL, NULL, "Unexpected attribute axis in '%s'.\n", ctxt->base);
					ctxt->error = 1;
					goto error;
				}
				xmlCompileAttributeTest(ctxt);
				if(ctxt->error)
					goto error;
				return;
			}
			else {
				ERROR5(NULL, NULL, NULL, "The 'element' or 'attribute' axis is expected.\n", 0);
				ctxt->error = 1;
				goto error;
			}
		}
	}
	else if(CUR == '*') {
		if(name) {
			ctxt->error = 1;
			goto error;
		}
		NEXT;
		PUSH(XML_OP_ALL, token, 0);
	}
	else {
		PUSH(XML_OP_ELEM, name, 0);
	}
	return;
error:
	if(URL)
		XML_PAT_FREE_STRING(ctxt, URL)
		if(token)
			XML_PAT_FREE_STRING(ctxt, token)
			if(name)
				XML_PAT_FREE_STRING(ctxt, name)
}

/**
 * xmlCompilePathPattern:
 * @ctxt:  the compilation context
 *
 * Compile the Path Pattern and generates a precompiled
 * form suitable for fast matching.
 *
 * [5]    Path    ::=    ('.//')? ( Step '/' )* ( Step | '@' NameTest )
 */
static void xmlCompilePathPattern(xmlPatParserContextPtr ctxt)
{
	SKIP_BLANKS;
	if(CUR == '/') {
		ctxt->comp->flags |= PAT_FROM_ROOT;
	}
	else if((CUR == '.') || (ctxt->comp->flags & XML_PATTERN_NOTPATTERN)) {
		ctxt->comp->flags |= PAT_FROM_CUR;
	}
	if((CUR == '/') && (NXT(1) == '/')) {
		PUSH(XML_OP_ANCESTOR, 0, 0);
		NEXT;
		NEXT;
	}
	else if((CUR == '.') && (NXT(1) == '/') && (NXT(2) == '/')) {
		PUSH(XML_OP_ANCESTOR, 0, 0);
		NEXT;
		NEXT;
		NEXT;
		/* Check for incompleteness. */
		SKIP_BLANKS;
		if(CUR == 0) {
			ERROR5(NULL, NULL, NULL, "Incomplete expression '%s'.\n", ctxt->base);
			ctxt->error = 1;
			goto error;
		}
	}
	if(CUR == '@') {
		NEXT;
		xmlCompileAttributeTest(ctxt);
		SKIP_BLANKS;
		/* @todo check for incompleteness */
		if(CUR != 0) {
			xmlCompileStepPattern(ctxt);
			if(ctxt->error)
				goto error;
		}
	}
	else {
		if(CUR == '/') {
			PUSH(XML_OP_ROOT, 0, 0);
			NEXT;
			/* Check for incompleteness. */
			SKIP_BLANKS;
			if(CUR == 0) {
				ERROR5(NULL, NULL, NULL, "Incomplete expression '%s'.\n", ctxt->base);
				ctxt->error = 1;
				goto error;
			}
		}
		xmlCompileStepPattern(ctxt);
		if(ctxt->error)
			goto error;
		SKIP_BLANKS;
		while(CUR == '/') {
			if(NXT(1) == '/') {
				PUSH(XML_OP_ANCESTOR, 0, 0);
				NEXT;
				NEXT;
				SKIP_BLANKS;
				xmlCompileStepPattern(ctxt);
				if(ctxt->error)
					goto error;
			}
			else {
				PUSH(XML_OP_PARENT, 0, 0);
				NEXT;
				SKIP_BLANKS;
				if(CUR == 0) {
					ERROR5(NULL, NULL, NULL, "Incomplete expression '%s'.\n", ctxt->base);
					ctxt->error = 1;
					goto error;
				}
				xmlCompileStepPattern(ctxt);
				if(ctxt->error)
					goto error;
			}
		}
	}
	if(CUR != 0) {
		ERROR5(NULL, NULL, NULL, "Failed to compile pattern %s\n", ctxt->base);
		ctxt->error = 1;
	}
error:
	return;
}

/**
 * xmlCompileIDCXPathPath:
 * @ctxt:  the compilation context
 *
 * Compile the Path Pattern and generates a precompiled
 * form suitable for fast matching.
 *
 * [5]    Path    ::=    ('.//')? ( Step '/' )* ( Step | '@' NameTest )
 */
				static void xmlCompileIDCXPathPath(xmlPatParserContextPtr ctxt) {
					SKIP_BLANKS;
					if(CUR == '/') {
						ERROR5(NULL, NULL, NULL, "Unexpected selection of the document root in '%s'.\n", ctxt->base);
						goto error;
					}
					ctxt->comp->flags |= PAT_FROM_CUR;

					if(CUR == '.') {
						/* "." - "self::node()" */
						NEXT;
						SKIP_BLANKS;
						if(CUR == 0) {
							/*
							 * Selection of the context node.
							 */
							PUSH(XML_OP_ELEM, 0, 0);
							return;
						}
						if(CUR != '/') {
							/* @todo A more meaningful error message. */
							ERROR5(NULL, NULL, NULL, "Unexpected token after '.' in '%s'.\n", ctxt->base);
							goto error;
						}
						/* "./" - "self::node()/" */
						NEXT;
						SKIP_BLANKS;
						if(CUR == '/') {
							if(IS_BLANK_CH(PEEKPREV(1))) {
								/*
								 * Disallow "./ /"
								 */
								ERROR5(NULL, NULL, NULL, "Unexpected '/' token in '%s'.\n", ctxt->base);
								goto error;
							}
							/* ".//" - "self:node()/descendant-or-self::node()/" */
							PUSH(XML_OP_ANCESTOR, 0, 0);
							NEXT;
							SKIP_BLANKS;
						}
						if(CUR == 0)
							goto error_unfinished;
					}
					/*
					 * Process steps.
					 */
					do {
						xmlCompileStepPattern(ctxt);
						if(ctxt->error)
							goto error;
						SKIP_BLANKS;
						if(CUR != '/')
							break;
						PUSH(XML_OP_PARENT, 0, 0);
						NEXT;
						SKIP_BLANKS;
						if(CUR == '/') {
							/*
							 * Disallow subsequent '//'.
							 */
							ERROR5(NULL, NULL, NULL, "Unexpected subsequent '//' in '%s'.\n", ctxt->base);
							goto error;
						}
						if(CUR == 0)
							goto error_unfinished;
					} while(CUR != 0);

					if(CUR != 0) {
						ERROR5(NULL, NULL, NULL, "Failed to compile expression '%s'.\n", ctxt->base);
						ctxt->error = 1;
					}
					return;
error:
					ctxt->error = 1;
					return;

error_unfinished:
					ctxt->error = 1;
					ERROR5(NULL, NULL, NULL, "Unfinished expression '%s'.\n", ctxt->base);
					return;
				}

// 
// The streaming code
// 
#ifdef DEBUG_STREAMING
				static void xmlDebugStreamComp(xmlStreamCompPtr stream) 
				{
					if(stream == NULL) {
						printf("Stream: NULL\n");
					}
					else {
						printf("Stream: %d steps\n", stream->nbStep);
						for(int i = 0; i < stream->nbStep; i++) {
							if(stream->steps[i].ns) {
								printf("{%s}", stream->steps[i].ns);
							}
							if(stream->steps[i].name == NULL) {
								printf("* ");
							}
							else {
								printf("%s ", stream->steps[i].name);
							}
							if(stream->steps[i].flags & XML_STREAM_STEP_ROOT)
								printf("root ");
							if(stream->steps[i].flags & XML_STREAM_STEP_DESC)
								printf("// ");
							if(stream->steps[i].flags & XML_STREAM_STEP_FINAL)
								printf("final ");
							printf("\n");
						}
					}
				}
				static void xmlDebugStreamCtxt(xmlStreamCtxtPtr ctxt, int match) 
				{
					if(!ctxt) {
						printf("Stream: NULL\n");
					}
					else {
						printf("Stream: level %d, %d states: ", ctxt->level, ctxt->nbState);
						if(match)
							printf("matches\n");
						else
							printf("\n");
						for(int i = 0; i < ctxt->nbState; i++) {
							if(ctxt->states[2 * i] < 0)
								printf(" %d: free\n", i);
							else {
								printf(" %d: step %d, level %d", i, ctxt->states[2 * i], ctxt->states[(2 * i) + 1]);
								if(ctxt->comp->steps[ctxt->states[2 * i]].flags & XML_STREAM_STEP_DESC) 
									printf(" //\n");
								else
									printf("\n");
							}
						}
					}
				}

#endif
/**
 * xmlNewStreamComp:
 * @size: the number of expected steps
 *
 * build a new compiled pattern for streaming
 *
 * Returns the new structure or NULL in case of error.
 */
static xmlStreamCompPtr xmlNewStreamComp(int size)
{
	SETMAX(size, 4);
	xmlStreamCompPtr cur = static_cast<xmlStreamComp *>(SAlloc::M(sizeof(xmlStreamComp)));
	if(!cur) {
		ERROR(0, 0, 0, __FUNCTION__ ": malloc failed\n");
		return 0;
	}
	memzero(cur, sizeof(xmlStreamComp));
	cur->steps = static_cast<xmlStreamStep *>(SAlloc::M(size * sizeof(xmlStreamStep)));
	if(cur->steps == NULL) {
		SAlloc::F(cur);
		ERROR(0, 0, 0, __FUNCTION__ ": malloc failed\n");
		return 0;
	}
	cur->nbStep = 0;
	cur->maxStep = size;
	return cur;
}
/**
 * xmlStreamCompAddStep:
 * @comp: the compiled pattern for streaming
 * @name: the first string, the name, or NULL for *
 * @ns: the second step, the namespace name
 * @flags: the flags for that step
 *
 * Add a new step to the compiled pattern
 *
 * Returns -1 in case of error or the step index if successful
 */
static int FASTCALL xmlStreamCompAddStep(xmlStreamCompPtr comp, const xmlChar * name, const xmlChar * ns, int nodeType, int flags)
{
	xmlStreamStep * cur;
	if(comp->nbStep >= comp->maxStep) {
		cur = static_cast<xmlStreamStep *>(SAlloc::R(comp->steps, comp->maxStep * 2 * sizeof(xmlStreamStep)));
		if(!cur) {
			ERROR(0, 0, 0, __FUNCTION__ ": malloc failed\n");
			return -1;
		}
		comp->steps = cur;
		comp->maxStep *= 2;
	}
	cur = &comp->steps[comp->nbStep++];
	cur->flags = flags;
	cur->name = name;
	cur->ns = ns;
	cur->nodeType = nodeType;
	return (comp->nbStep - 1);
}
/**
 * xmlStreamCompile:
 * @comp: the precompiled pattern
 *
 * Tries to stream compile a pattern
 *
 * Returns -1 in case of failure and 0 in case of success.
 */
static int xmlStreamCompile(xmlPattern * comp)
{
	xmlStreamCompPtr stream;
	int i, s = 0, root = 0, flags = 0, prevs = -1;
	xmlStepOp step;
	if((comp == NULL) || (comp->steps == NULL))
		return -1;
	/*
		* special case for .
		*/
	if((comp->nbStep == 1) && (comp->steps[0].op == XML_OP_ELEM) && (comp->steps[0].value == NULL) && (comp->steps[0].value2 == NULL)) {
		stream = xmlNewStreamComp(0);
		if(stream == NULL)
			return -1;
		/* Note that the stream will have no steps in this case. */
		stream->flags |= XML_STREAM_FINAL_IS_ANY_NODE;
		comp->stream = stream;
		return 0;
	}
	stream = xmlNewStreamComp((comp->nbStep / 2) + 1);
	if(stream == NULL)
		return -1;
	if(comp->dict) {
		stream->dict = comp->dict;
		xmlDictReference(stream->dict);
	}
	i = 0;
	if(comp->flags & PAT_FROM_ROOT)
		stream->flags |= XML_STREAM_FROM_ROOT;
	for(; i < comp->nbStep; i++) {
		step = comp->steps[i];
		switch(step.op) {
			case XML_OP_END:
				break;
			case XML_OP_ROOT:
				if(i != 0)
					goto error;
				root = 1;
				break;
			case XML_OP_NS:
				s = xmlStreamCompAddStep(stream, NULL, step.value, XML_ELEMENT_NODE, flags);
				if(s < 0)
					goto error;
				prevs = s;
				flags = 0;
				break;
			case XML_OP_ATTR:
				flags |= XML_STREAM_STEP_ATTR;
				prevs = -1;
				s = xmlStreamCompAddStep(stream, step.value, step.value2, XML_ATTRIBUTE_NODE, flags);
				flags = 0;
				if(s < 0)
					goto error;
				break;
			case XML_OP_ELEM:
				if((step.value == NULL) && (step.value2 == NULL)) {
					/*
						* We have a "." or "self::node()" here.
						* Eliminate redundant self::node() tests like in
						*"/./."
						* or "//./"
						* The only case we won't eliminate is "//.", i.e.
						*if
						* self::node() is the last node test and we had
						* continuation somewhere beforehand.
						*/
					if((comp->nbStep == i + 1) && (flags & XML_STREAM_STEP_DESC)) {
						/* Mark the special case where the expression resolves to any type of node. */
						if(comp->nbStep == i + 1) {
							stream->flags |= XML_STREAM_FINAL_IS_ANY_NODE;
						}
						flags |= XML_STREAM_STEP_NODE;
						s = xmlStreamCompAddStep(stream, NULL, NULL, XML_STREAM_ANY_NODE, flags);
						if(s < 0)
							goto error;
						flags = 0;
						/*
							* If there was a previous step, mark it to
							*be added to
							* the result node-set; this is needed since
							*only
							* the last step will be marked as "final"
							*and only
							* "final" nodes are added to the resulting
							*set.
							*/
						if(prevs != -1) {
							stream->steps[prevs].flags |= XML_STREAM_STEP_IN_SET;
							prevs = -1;
						}
						break;
					}
					else {
						/* Just skip this one. */
						continue;
					}
				}
				/* An element node. */
				s = xmlStreamCompAddStep(stream, step.value, step.value2, XML_ELEMENT_NODE, flags);
				if(s < 0)
					goto error;
				prevs = s;
				flags = 0;
				break;
			case XML_OP_CHILD:
				/* An element node child. */
				s = xmlStreamCompAddStep(stream, step.value, step.value2, XML_ELEMENT_NODE, flags);
				if(s < 0)
					goto error;
				prevs = s;
				flags = 0;
				break;
			case XML_OP_ALL:
				s = xmlStreamCompAddStep(stream, NULL, NULL, XML_ELEMENT_NODE, flags);
				if(s < 0)
					goto error;
				prevs = s;
				flags = 0;
				break;
			case XML_OP_PARENT:
				break;
			case XML_OP_ANCESTOR:
				/* Skip redundant continuations. */
				if(flags & XML_STREAM_STEP_DESC)
					break;
				flags |= XML_STREAM_STEP_DESC;
				/*
					* Mark the expression as having "//".
					*/
				if((stream->flags & XML_STREAM_DESC) == 0)
					stream->flags |= XML_STREAM_DESC;
				break;
		}
	}
	if((!root) && (comp->flags & XML_PATTERN_NOTPATTERN) == 0) {
		/*
			* If this should behave like a real pattern, we will mark
			* the first step as having "//", to be reentrant on every
			* tree level.
			*/
		if((stream->flags & XML_STREAM_DESC) == 0)
			stream->flags |= XML_STREAM_DESC;

		if(stream->nbStep > 0) {
			if((stream->steps[0].flags & XML_STREAM_STEP_DESC) == 0)
				stream->steps[0].flags |= XML_STREAM_STEP_DESC;
		}
	}
	if(stream->nbStep <= s)
		goto error;
	stream->steps[s].flags |= XML_STREAM_STEP_FINAL;
	if(root)
		stream->steps[0].flags |= XML_STREAM_STEP_ROOT;
#ifdef DEBUG_STREAMING
	xmlDebugStreamComp(stream);
#endif
	comp->stream = stream;
	return 0;
error:
	xmlFreeStreamComp(stream);
	return 0;
}

/**
 * xmlNewStreamCtxt:
 * @size: the number of expected states
 *
 * build a new stream context
 *
 * Returns the new structure or NULL in case of error.
 */
static xmlStreamCtxtPtr xmlNewStreamCtxt(xmlStreamCompPtr stream)
{
	xmlStreamCtxtPtr cur = (xmlStreamCtxt *)SAlloc::M(sizeof(xmlStreamCtxt));
	if(!cur) {
		ERROR(0, 0, 0, __FUNCTION__ ": malloc failed\n");
	}
	else {
		memzero(cur, sizeof(xmlStreamCtxt));
		cur->states = (int *)SAlloc::M(4 * 2 * sizeof(int));
		if(cur->states == NULL) {
			SAlloc::F(cur);
			ERROR(0, 0, 0, __FUNCTION__ ": malloc failed\n");
			cur = 0;
		}
		else {
			cur->nbState = 0;
			cur->maxState = 4;
			cur->level = 0;
			cur->comp = stream;
			cur->blockLevel = -1;
		}
	}
	return cur;
}
/**
 * xmlFreeStreamCtxt:
 * @stream: the stream context
 *
 * Free the stream context
 */
void xmlFreeStreamCtxt(xmlStreamCtxtPtr stream)
{
	while(stream) {
		xmlStreamCtxtPtr next = stream->next;
		SAlloc::F(stream->states);
		SAlloc::F(stream);
		stream = next;
	}
}
/**
 * xmlStreamCtxtAddState:
 * @comp: the stream context
 * @idx: the step index for that streaming state
 *
 * Add a new state to the stream context
 *
 * Returns -1 in case of error or the state index if successful
 */
static int xmlStreamCtxtAddState(xmlStreamCtxtPtr comp, int idx, int level)
{
	int i;
	for(i = 0; i < comp->nbState; i++) {
		if(comp->states[2 * i] < 0) {
			comp->states[2 * i] = idx;
			comp->states[2 * i + 1] = level;
			return (i);
		}
	}
	if(comp->nbState >= comp->maxState) {
		int * cur = (int *)SAlloc::R(comp->states, comp->maxState * 4 * sizeof(int));
		if(!cur) {
			ERROR(0, 0, 0, __FUNCTION__ ": malloc failed\n");
			return -1;
		}
		comp->states = cur;
		comp->maxState *= 2;
	}
	comp->states[2 * comp->nbState] = idx;
	comp->states[2 * comp->nbState++ + 1] = level;
	return (comp->nbState - 1);
}
/**
 * xmlStreamPushInternal:
 * @stream: the stream context
 * @name: the current name
 * @ns: the namespace name
 * @nodeType: the type of the node
 *
 * Push new data onto the stream. NOTE: if the call xmlPatterncompile()
 * indicated a dictionary, then strings for name and ns will be expected
 * to come from the dictionary.
 * Both @name and @ns being NULL means the / i.e. the root of the document.
 * This can also act as a reset.
 *
 * Returns: -1 in case of error, 1 if the current state in the stream is a
 *  match and 0 otherwise.
 */
static int xmlStreamPushInternal(xmlStreamCtxtPtr stream, const xmlChar * name, const xmlChar * ns, int nodeType)
{
	int ret = 0, err = 0, final = 0, tmp, i, m, match, stepNr, desc;
	xmlStreamCompPtr comp;
	xmlStreamStep step;
#ifdef DEBUG_STREAMING
	xmlStreamCtxtPtr orig = stream;
#endif
	if((stream == NULL) || (stream->nbState < 0))
		return -1;
	while(stream) {
		comp = stream->comp;
		if((nodeType == XML_ELEMENT_NODE) && !name && (ns == NULL)) {
			/* We have a document node here (or a reset). */
			stream->nbState = 0;
			stream->level = 0;
			stream->blockLevel = -1;
			if(comp->flags & XML_STREAM_FROM_ROOT) {
				if(comp->nbStep == 0) {
					/* @todo We have a "/." here? */
					ret = 1;
				}
				else {
					if((comp->nbStep == 1) && (comp->steps[0].nodeType == XML_STREAM_ANY_NODE) && (comp->steps[0].flags & XML_STREAM_STEP_DESC)) {
						/*
							* In the case of "//." the document
							*node will match
							* as well.
							*/
						ret = 1;
					}
					else if(comp->steps[0].flags & XML_STREAM_STEP_ROOT) {
						/* @todo Do we need this ? */
						tmp = xmlStreamCtxtAddState(stream, 0, 0);
						if(tmp < 0)
							err++;
					}
				}
			}
			stream = stream->next;
			continue; /* while */
		}

		/*
			* Fast check for ".".
			*/
		if(comp->nbStep == 0) {
			/*
				* / and . are handled at the XPath node set creation
				* level by checking min depth
				*/
			if(stream->flags & XML_PATTERN_XPATH) {
				stream = stream->next;
				continue; /* while */
			}
			/*
				* For non-pattern like evaluation like XML Schema IDCs
				* or traditional XPath expressions, this will match if
				* we are at the first level only, otherwise on every level.
				*/
			if((nodeType != XML_ATTRIBUTE_NODE) && (((stream->flags & XML_PATTERN_NOTPATTERN) == 0) || (stream->level == 0))) {
				ret = 1;
			}
			stream->level++;
			goto stream_next;
		}
		if(stream->blockLevel != -1) {
			//
			// Skip blocked expressions.
			//
			stream->level++;
			goto stream_next;
		}

		if((nodeType != XML_ELEMENT_NODE) && (nodeType != XML_ATTRIBUTE_NODE) && ((comp->flags & XML_STREAM_FINAL_IS_ANY_NODE) == 0)) {
			/*
				* No need to process nodes of other types if we don't
				* resolve to those types.
				* @todo Do we need to block the context here?
				*/
			stream->level++;
			goto stream_next;
		}
		/*
			* Check evolution of existing states
			*/
		i = 0;
		m = stream->nbState;
		while(i < m) {
			if((comp->flags & XML_STREAM_DESC) == 0) {
				// 
				// If there is no "//", then only the last added state is of interest.
				// 
				stepNr = stream->states[2 * (stream->nbState -1)];
				// 
				// @todo Security check, should not happen, remove it.
				// 
				if(stream->states[(2 * (stream->nbState -1)) + 1] <
					stream->level) {
					return -1;
				}
				desc = 0;
				// loop-stopper 
				i = m;
			}
			else {
				// 
				// If there are "//", then we need to process every "//"
				// occuring in the states, plus any other state for this level.
				// 
				stepNr = stream->states[2 * i];
				/* @todo should not happen anymore: dead states */
				if(stepNr < 0)
					goto next_state;
				tmp = stream->states[(2 * i) + 1];
				/* skip new states just added */
				if(tmp > stream->level)
					goto next_state;
				/* skip states at ancestor levels, except if "//" */
				desc = comp->steps[stepNr].flags & XML_STREAM_STEP_DESC;
				if((tmp < stream->level) && (!desc))
					goto next_state;
			}
			/*
				* Check for correct node-type.
				*/
			step = comp->steps[stepNr];
			if(step.nodeType != nodeType) {
				if(step.nodeType == XML_ATTRIBUTE_NODE) {
					// 
					// Block this expression for deeper evaluation.
					// 
					if((comp->flags & XML_STREAM_DESC) == 0)
						stream->blockLevel = stream->level +1;
					goto next_state;
				}
				else if(step.nodeType != XML_STREAM_ANY_NODE)
					goto next_state;
			}
			//
			// Compare local/namespace-name.
			//
			match = 0;
			if(step.nodeType == XML_STREAM_ANY_NODE) {
				match = 1;
			}
			else if(step.name == NULL) {
				if(step.ns == NULL) {
					match = 1; // This lets through all elements/attributes.
				}
				else if(ns)
					match = sstreq(step.ns, ns);
			}
			else if(((step.ns != NULL) == (ns != NULL)) && name && (step.name[0] == name[0]) && sstreq(step.name, name) && ((step.ns == ns) || sstreq(step.ns, ns))) {
				match = 1;
			}
#if 0
/*
* @todo Pointer comparison won't work, since not guaranteed that the given
*  values are in the same dict; especially if it's the namespace name,
*  normally coming from ns->href. We need a namespace dict mechanism !
*/
		}
		else if(comp->dict) {
			if(step.name == NULL) {
				if(step.ns == NULL)
					match = 1;
				else
					match = (step.ns == ns);
			}
			else {
				match = ((step.name == name) && (step.ns == ns));
			}
#endif /* if 0 ------------------------------------------------------- */
			if(match) {
				final = step.flags & XML_STREAM_STEP_FINAL;
				if(desc) {
					if(final) {
						ret = 1;
					}
					else {
						// descending match create a new state
						xmlStreamCtxtAddState(stream, stepNr + 1, stream->level + 1);
					}
				}
				else {
					if(final) {
						ret = 1;
					}
					else {
						xmlStreamCtxtAddState(stream, stepNr + 1, stream->level + 1);
					}
				}
				if((ret != 1) && (step.flags & XML_STREAM_STEP_IN_SET)) {
					/*
						* Check if we have a special case like
						*"foo/bar//.", where
						* "foo" is selected as well.
						*/
					ret = 1;
				}
			}
			if(((comp->flags & XML_STREAM_DESC) == 0) && ((!match) || final)) {
				/*
					* Mark this expression as blocked for any evaluation at
					* deeper levels. Note that this includes "/foo"
					* expressions if the *pattern* behaviour is used.
					*/
				stream->blockLevel = stream->level +1;
			}
next_state:
			i++;
		}
		stream->level++;
		// 
		// Re/enter the expression.
		// Don't reenter if it's an absolute expression like "/foo", except "//foo".
		// 
		step = comp->steps[0];
		if(step.flags & XML_STREAM_STEP_ROOT)
			goto stream_next;
		desc = step.flags & XML_STREAM_STEP_DESC;
		if(stream->flags & XML_PATTERN_NOTPATTERN) {
			// 
			// Re/enter the expression if it is a "descendant" one,
			// or if we are at the 1st level of evaluation.
			// 
			if(stream->level == 1) {
				if(XML_STREAM_XS_IDC(stream)) {
					// 
					// XS-IDC: The missing "self::node()" will always
					// match the first given node.
					// 
					goto stream_next;
				}
				else
					goto compare;
			}
			/*
				* A "//" is always reentrant.
				*/
			if(desc)
				goto compare;

			/*
				* XS-IDC: Process the 2nd level, since the missing
				* "self::node()" is responsible for the 2nd level being
				* the real start level.
				*/
			if((stream->level == 2) && XML_STREAM_XS_IDC(stream))
				goto compare;

			goto stream_next;
		}
compare:
		// Check expected node-type.
		if(step.nodeType != nodeType) {
			if(nodeType == XML_ATTRIBUTE_NODE)
				goto stream_next;
			else if(step.nodeType != XML_STREAM_ANY_NODE)
				goto stream_next;
		}
		// Compare local/namespace-name.
		match = 0;
		if(step.nodeType == XML_STREAM_ANY_NODE) {
			match = 1;
		}
		else if(step.name == NULL) {
			if(step.ns == NULL) {
				match = 1; // This lets through all elements/attributes.
			}
			else if(ns)
				match = sstreq(step.ns, ns);
		}
		else if(((step.ns != NULL) == (ns != NULL)) && name && (step.name[0] == name[0]) && sstreq(step.name, name) && ((step.ns == ns) || sstreq(step.ns, ns))) {
			match = 1;
		}
		final = step.flags & XML_STREAM_STEP_FINAL;
		if(match) {
			if(final)
				ret = 1;
			else
				xmlStreamCtxtAddState(stream, 1, stream->level);
			if((ret != 1) && (step.flags & XML_STREAM_STEP_IN_SET)) {
				ret = 1; // Check if we have a special case like "foo//.", where "foo" is selected as well.
			}
		}
		if(((comp->flags & XML_STREAM_DESC) == 0) && ((!match) || final)) {
			stream->blockLevel = stream->level; // Mark this expression as blocked for any evaluation at deeper levels.
		}

stream_next:
		stream = stream->next;
	} /* while stream != NULL */
	if(err > 0)
		ret = -1;
#ifdef DEBUG_STREAMING
	xmlDebugStreamCtxt(orig, ret);
#endif
	return ret;
}
/**
 * xmlStreamPush:
 * @stream: the stream context
 * @name: the current name
 * @ns: the namespace name
 *
 * Push new data onto the stream. NOTE: if the call xmlPatterncompile()
 * indicated a dictionary, then strings for name and ns will be expected
 * to come from the dictionary.
 * Both @name and @ns being NULL means the / i.e. the root of the document.
 * This can also act as a reset.
 * Otherwise the function will act as if it has been given an element-node.
 *
 * Returns: -1 in case of error, 1 if the current state in the stream is a
 *  match and 0 otherwise.
 */
int xmlStreamPush(xmlStreamCtxtPtr stream, const xmlChar * name, const xmlChar * ns)
{
	return xmlStreamPushInternal(stream, name, ns, (int)XML_ELEMENT_NODE);
}
/**
 * xmlStreamPushNode:
 * @stream: the stream context
 * @name: the current name
 * @ns: the namespace name
 * @nodeType: the type of the node being pushed
 *
 * Push new data onto the stream. NOTE: if the call xmlPatterncompile()
 * indicated a dictionary, then strings for name and ns will be expected
 * to come from the dictionary.
 * Both @name and @ns being NULL means the / i.e. the root of the document.
 * This can also act as a reset.
 * Different from xmlStreamPush() this function can be fed with nodes of type:
 * element-, attribute-, text-, cdata-section-, comment- and
 * processing-instruction-node.
 *
 * Returns: -1 in case of error, 1 if the current state in the stream is a
 *  match and 0 otherwise.
 */
int xmlStreamPushNode(xmlStreamCtxtPtr stream, const xmlChar * name, const xmlChar * ns, int nodeType)
{
	return xmlStreamPushInternal(stream, name, ns, nodeType);
}
/**
 * xmlStreamPushAttr:
 * @stream: the stream context
 * @name: the current name
 * @ns: the namespace name
 *
 * Push new attribute data onto the stream. NOTE: if the call xmlPatterncompile()
 * indicated a dictionary, then strings for name and ns will be expected
 * to come from the dictionary.
 * Both @name and @ns being NULL means the / i.e. the root of the document.
 * This can also act as a reset.
 * Otherwise the function will act as if it has been given an attribute-node.
 *
 * Returns: -1 in case of error, 1 if the current state in the stream is a
 *  match and 0 otherwise.
 */
int xmlStreamPushAttr(xmlStreamCtxtPtr stream, const xmlChar * name, const xmlChar * ns)
{
	return (xmlStreamPushInternal(stream, name, ns, (int)XML_ATTRIBUTE_NODE));
}
/**
 * xmlStreamPop:
 * @stream: the stream context
 *
 * push one level from the stream.
 *
 * Returns: -1 in case of error, 0 otherwise.
 */
int xmlStreamPop(xmlStreamCtxtPtr stream)
{
	int i, lev;
	if(stream == NULL)
		return -1;
	while(stream) {
		/*
			* Reset block-level.
			*/
		if(stream->blockLevel == stream->level)
			stream->blockLevel = -1;
		/*
			*  stream->level can be zero when XML_FINAL_IS_ANY_NODE is set
			*  (see the thread at
			*  http://mail.gnome.org/archives/xslt/2008-July/msg00027.html)
			*/
		if(stream->level)
			stream->level--;
		/*
			* Check evolution of existing states
			*/
		for(i = stream->nbState -1; i >= 0; i--) {
			/* discard obsoleted states */
			lev = stream->states[(2 * i) + 1];
			if(lev > stream->level)
				stream->nbState--;
			if(lev <= stream->level)
				break;
		}
		stream = stream->next;
	}
	return 0;
}
/**
 * xmlStreamWantsAnyNode:
 * @streamCtxt: the stream context
 *
 * Query if the streaming pattern additionally needs to be fed with
 * text-, cdata-section-, comment- and processing-instruction-nodes.
 * If the result is 0 then only element-nodes and attribute-nodes
 * need to be pushed.
 *
 * Returns: 1 in case of need of nodes of the above described types,
 *     0 otherwise. -1 on API errors.
 */
int xmlStreamWantsAnyNode(xmlStreamCtxtPtr streamCtxt)
{
	if(streamCtxt == NULL)
		return -1;
	while(streamCtxt) {
		if(streamCtxt->comp->flags & XML_STREAM_FINAL_IS_ANY_NODE)
			return 1;
		streamCtxt = streamCtxt->next;
	}
	return 0;
}
// 
// The public interfaces
// 
/**
 * xmlPatterncompile:
 * @pattern: the pattern to compile
 * @dict: an optional dictionary for interned strings
 * @flags: compilation flags, see xmlPatternFlags
 * @namespaces: the prefix definitions, array of [URI, prefix] or NULL
 *
 * Compile a pattern.
 *
 * Returns the compiled form of the pattern or NULL in case of error
 */
xmlPattern * xmlPatterncompile(const xmlChar * pattern, xmlDict * dict, int flags, const xmlChar ** namespaces)
{
	xmlPattern * ret = NULL;
	xmlPattern * cur;
	xmlPatParserContextPtr ctxt = NULL;
	const xmlChar * p_or;
	const xmlChar * start;
	xmlChar * tmp = NULL;
	int type = 0;
	int streamable = 1;
	if(pattern == NULL)
		return 0;
	start = pattern;
	p_or = start;
	while(*p_or != 0) {
		tmp = NULL;
		while((*p_or != 0) && (*p_or != '|')) 
			p_or++;
		if(*p_or == 0)
			ctxt = xmlNewPatParserContext(start, dict, namespaces);
		else {
			tmp = xmlStrndup(start, p_or - start);
			if(tmp)
				ctxt = xmlNewPatParserContext(tmp, dict, namespaces);
			p_or++;
		}
		if(!ctxt)
			goto error;
		cur = xmlNewPattern();
		if(!cur)
			goto error;
		// 
		// Assign string dict.
		// 
		if(dict) {
			cur->dict = dict;
			xmlDictReference(dict);
		}
		if(!ret)
			ret = cur;
		else {
			cur->next = ret->next;
			ret->next = cur;
		}
		cur->flags = flags;
		ctxt->comp = cur;
		if(XML_STREAM_XS_IDC(cur))
			xmlCompileIDCXPathPath(ctxt);
		else
			xmlCompilePathPattern(ctxt);
		if(ctxt->error)
			goto error;
		xmlFreePatParserContext(ctxt);
		ctxt = NULL;
		if(streamable) {
			if(type == 0) {
				type = cur->flags & (PAT_FROM_ROOT | PAT_FROM_CUR);
			}
			else if(type == PAT_FROM_ROOT) {
				if(cur->flags & PAT_FROM_CUR)
					streamable = 0;
			}
			else if(type == PAT_FROM_CUR) {
				if(cur->flags & PAT_FROM_ROOT)
					streamable = 0;
			}
		}
		if(streamable)
			xmlStreamCompile(cur);
		if(xmlReversePattern(cur) < 0)
			goto error;
		ZFREE(tmp);
		start = p_or;
	}
	if(streamable == 0) {
		for(cur = ret; cur; cur = cur->next) {
			xmlFreeStreamComp(cur->stream);
			cur->stream = NULL;
		}
	}
	return ret;
error:
	xmlFreePatParserContext(ctxt);
	xmlFreePattern(ret);
	SAlloc::F(tmp);
	return 0;
}

/**
 * xmlPatternMatch:
 * @comp: the precompiled pattern
 * @node: a node
 *
 * Test whether the node matches the pattern
 *
 * Returns 1 if it matches, 0 if it doesn't and -1 in case of failure
 */
int xmlPatternMatch(xmlPattern * comp, xmlNode * P_Node)
{
	int ret = 0;
	if(!comp || !P_Node)
		return -1;
	else {
		while(comp) {
			ret = xmlPatMatch(comp, P_Node);
			if(ret)
				return ret;
			comp = comp->next;
		}
		return ret;
	}
}
/**
 * xmlPatternGetStreamCtxt:
 * @comp: the precompiled pattern
 *
 * Get a streaming context for that pattern
 * Use xmlFreeStreamCtxt to free the context.
 *
 * Returns a pointer to the context or NULL in case of failure
 */
xmlStreamCtxtPtr xmlPatternGetStreamCtxt(xmlPattern * comp)
{
	xmlStreamCtxtPtr ret = NULL, cur;
	if((comp == NULL) || (comp->stream == NULL))
		return 0;
	while(comp) {
		if(comp->stream == NULL)
			goto failed;
		cur = xmlNewStreamCtxt(comp->stream);
		if(!cur)
			goto failed;
		if(!ret)
			ret = cur;
		else {
			cur->next = ret->next;
			ret->next = cur;
		}
		cur->flags = comp->flags;
		comp = comp->next;
	}
	return ret;
failed:
	xmlFreeStreamCtxt(ret);
	return 0;
}

/**
 * xmlPatternStreamable:
 * @comp: the precompiled pattern
 *
 * Check if the pattern is streamable i.e. xmlPatternGetStreamCtxt()
 * should work.
 *
 * Returns 1 if streamable, 0 if not and -1 in case of error.
 */
int xmlPatternStreamable(xmlPattern * comp)
{
	if(comp == NULL)
		return -1;
	else {
		while(comp) {
			if(comp->stream == NULL)
				return 0;
			else
				comp = comp->next;
		}
		return 1;
	}
}
/**
 * xmlPatternMaxDepth:
 * @comp: the precompiled pattern
 *
 * Check the maximum depth reachable by a pattern
 *
 * Returns -2 if no limit (using //), otherwise the depth,
 *    and -1 in case of error
 */
int xmlPatternMaxDepth(xmlPattern * comp)
{
	int ret = 0, i;
	if(comp == NULL)
		return -1;
	while(comp) {
		if(comp->stream == NULL)
			return -1;
		for(i = 0; i < comp->stream->nbStep; i++)
			if(comp->stream->steps[i].flags & XML_STREAM_STEP_DESC)
				return -2;
		if(comp->stream->nbStep > ret)
			ret = comp->stream->nbStep;
		comp = comp->next;
	}
	return ret;
}
/**
 * xmlPatternMinDepth:
 * @comp: the precompiled pattern
 *
 * Check the minimum depth reachable by a pattern, 0 mean the / or . are
 * part of the set.
 *
 * Returns -1 in case of error otherwise the depth,
 *
 */
int xmlPatternMinDepth(xmlPattern * comp)
{
	int ret = 12345678;
	if(comp == NULL)
		return -1;
	while(comp) {
		if(comp->stream == NULL)
			return -1;
		if(comp->stream->nbStep < ret)
			ret = comp->stream->nbStep;
		if(!ret)
			return 0;
		comp = comp->next;
	}
	return ret;
}
/**
 * xmlPatternFromRoot:
 * @comp: the precompiled pattern
 *
 * Check if the pattern must be looked at from the root.
 *
 * Returns 1 if true, 0 if false and -1 in case of error
 */
int xmlPatternFromRoot(xmlPattern * comp)
{
	if(comp == NULL)
		return -1;
	else {
		while(comp) {
			if(comp->stream == NULL)
				return -1;
			else if(comp->flags & PAT_FROM_ROOT)
				return 1;
			else
				comp = comp->next;
		}
		return 0;
	}
}

#define bottom_pattern
#endif /* LIBXML_PATTERN_ENABLED */
