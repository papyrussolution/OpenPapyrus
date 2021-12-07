/*
 * xinclude.c : Code to implement XInclude processing
 *
 * World Wide Web Consortium W3C Last Call Working Draft 10 November 2003
 * http://www.w3.org/TR/2003/WD-xinclude-20031110
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */
#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop

#ifdef LIBXML_XINCLUDE_ENABLED
//#include <libxml/xinclude.h>
#define XINCLUDE_MAX_DEPTH 40
/* #define DEBUG_XINCLUDE */
// 
// XInclude context handling
// 
/*
 * An XInclude context
 */
typedef xmlChar * xmlURL;

typedef struct _xmlXIncludeRef xmlXIncludeRef;
typedef xmlXIncludeRef * xmlXIncludeRefPtr;
struct _xmlXIncludeRef {
	xmlChar * URI; /* the fully resolved resource URL */
	xmlChar * fragment; /* the fragment in the URI */
	xmlDoc * doc; /* the parsed document */
	xmlNode * ref; /* the node making the reference in the source */
	xmlNode * inc; /* the included copy */
	int xml; /* xml or txt */
	int count; /* how many refs use that specific doc */
	xmlXPathObject * xptr; /* the xpointer if needed */
	int emptyFb; /* flag to show fallback empty */
};

struct _xmlXIncludeCtxt {
	xmlDoc * doc; /* the source document */
	int incBase; /* the first include for this document */
	int incNr; /* number of includes */
	int incMax; /* size of includes tab */
	xmlXIncludeRefPtr * incTab; /* array of included references */
	int txtNr; /* number of unparsed documents */
	int txtMax; /* size of unparsed documents tab */
	xmlNode ** txtTab; /* array of unparsed text nodes */
	xmlURL * txturlTab; /* array of unparsed text URLs */
	xmlChar * url; /* the current URL processed */
	int urlNr; /* number of URLs stacked */
	int urlMax; /* size of URL stack */
	xmlChar ** urlTab; /* URL stack */
	int nbErrors; /* the number of errors detected */
	int legacy; /* using XINCLUDE_OLD_NS */
	int parseFlags; /* the flags used for parsing XML documents */
	xmlChar * base; /* the current xml:base */
	void * _private; /* application data */
};

static int xmlXIncludeDoProcess(xmlXIncludeCtxtPtr ctxt, xmlDoc * doc, xmlNode * tree);
//
// XInclude error handler
//
/**
 * xmlXIncludeErrMemory:
 * @extra:  extra information
 *
 * Handle an out of memory condition
 */
static void xmlXIncludeErrMemory(xmlXIncludeCtxtPtr ctxt, xmlNode * P_Node, const char * extra)
{
	if(ctxt)
		ctxt->nbErrors++;
	__xmlRaiseError(0, 0, 0, ctxt, P_Node, XML_FROM_XINCLUDE, XML_ERR_NO_MEMORY, XML_ERR_ERROR, NULL, 0, extra, 0, 0, 0, 0, "Memory allocation failed : %s\n", extra);
}
/**
 * xmlXIncludeErr:
 * @ctxt: the XInclude context
 * @node: the context node
 * @msg:  the error message
 * @extra:  extra information
 *
 * Handle an XInclude error
 */
static void FASTCALL xmlXIncludeErr(xmlXIncludeCtxtPtr ctxt, xmlNode * P_Node, int error, const char * msg, const xmlChar * extra)
{
	if(ctxt)
		ctxt->nbErrors++;
	__xmlRaiseError(0, 0, 0, ctxt, P_Node, XML_FROM_XINCLUDE, error, XML_ERR_ERROR, NULL, 0, (const char *)extra, NULL, NULL, 0, 0, msg, (const char *)extra);
}
#if 0
/**
 * xmlXIncludeWarn:
 * @ctxt: the XInclude context
 * @node: the context node
 * @msg:  the error message
 * @extra:  extra information
 *
 * Emit an XInclude warning.
 */
static void xmlXIncludeWarn(xmlXIncludeCtxtPtr ctxt, xmlNode * node, int error, const char * msg, const xmlChar * extra)
{
	__xmlRaiseError(0, 0, 0, ctxt, node, XML_FROM_XINCLUDE, error, XML_ERR_WARNING, NULL, 0, (const char *)extra, NULL, NULL, 0, 0, msg, (const char *)extra);
}
#endif
/**
 * xmlXIncludeGetProp:
 * @ctxt:  the XInclude context
 * @cur:  the node
 * @name:  the attribute name
 *
 * Get an XInclude attribute
 *
 * Returns the value (to be freed) or NULL if not found
 */
static xmlChar * FASTCALL xmlXIncludeGetProp(xmlXIncludeCtxtPtr ctxt, xmlNode * cur, const xmlChar * name) 
{
	xmlChar * ret = xmlGetNsProp(cur, XINCLUDE_NS, name);
	if(ret)
		return ret;
	if(ctxt->legacy != 0) {
		ret = xmlGetNsProp(cur, XINCLUDE_OLD_NS, name);
		if(ret)
			return ret;
	}
	ret = xmlGetProp(cur, name);
	return ret;
}
/**
 * xmlXIncludeFreeRef:
 * @ref: the XInclude reference
 *
 * Free an XInclude reference
 */
static void xmlXIncludeFreeRef(xmlXIncludeRefPtr ref) 
{
	if(ref) {
#ifdef DEBUG_XINCLUDE
		xmlGenericError(0, "Freeing ref\n");
#endif
		if(ref->doc) {
#ifdef DEBUG_XINCLUDE
			xmlGenericError(0, "Freeing doc %s\n", ref->URI);
#endif
			xmlFreeDoc(ref->doc);
		}
		SAlloc::F(ref->URI);
		SAlloc::F(ref->fragment);
		xmlXPathFreeObject(ref->xptr);
		SAlloc::F(ref);
	}
}
/**
 * xmlXIncludeNewRef:
 * @ctxt: the XInclude context
 * @URI:  the resource URI
 *
 * Creates a new reference within an XInclude context
 *
 * Returns the new set
 */
static xmlXIncludeRefPtr xmlXIncludeNewRef(xmlXIncludeCtxtPtr ctxt, const xmlChar * URI, xmlNode * ref) 
{
	xmlXIncludeRefPtr ret;
#ifdef DEBUG_XINCLUDE
	xmlGenericError(0, "New ref %s\n", URI);
#endif
	ret = static_cast<xmlXIncludeRefPtr>(SAlloc::M(sizeof(xmlXIncludeRef)));
	if(!ret) {
		xmlXIncludeErrMemory(ctxt, ref, "growing XInclude context");
		return 0;
	}
	memzero(ret, sizeof(xmlXIncludeRef));
	ret->URI = sstrdup(URI);
	ret->fragment = NULL;
	ret->ref = ref;
	ret->doc = NULL;
	ret->count = 0;
	ret->xml = 0;
	ret->inc = NULL;
	if(ctxt->incMax == 0) {
		ctxt->incMax = 4;
		ctxt->incTab = static_cast<xmlXIncludeRefPtr *>(SAlloc::M(ctxt->incMax * sizeof(ctxt->incTab[0])));
		if(ctxt->incTab == NULL) {
			xmlXIncludeErrMemory(ctxt, ref, "growing XInclude context");
			xmlXIncludeFreeRef(ret);
			return 0;
		}
	}
	if(ctxt->incNr >= ctxt->incMax) {
		ctxt->incMax *= 2;
		ctxt->incTab = static_cast<xmlXIncludeRefPtr *>(SAlloc::R(ctxt->incTab, ctxt->incMax * sizeof(ctxt->incTab[0])));
		if(ctxt->incTab == NULL) {
			xmlXIncludeErrMemory(ctxt, ref, "growing XInclude context");
			xmlXIncludeFreeRef(ret);
			return 0;
		}
	}
	ctxt->incTab[ctxt->incNr++] = ret;
	return ret;
}
/**
 * xmlXIncludeNewContext:
 * @doc:  an XML Document
 *
 * Creates a new XInclude context
 *
 * Returns the new set
 */
xmlXIncludeCtxtPtr xmlXIncludeNewContext(xmlDoc * doc) 
{
	xmlXIncludeCtxtPtr ret = 0;
#ifdef DEBUG_XINCLUDE
	xmlGenericError(0, "New context\n");
#endif
	if(doc) {
		ret = static_cast<xmlXIncludeCtxtPtr>(SAlloc::M(sizeof(xmlXIncludeCtxt)));
		if(!ret) {
			xmlXIncludeErrMemory(NULL, (xmlNode *)doc, "creating XInclude context");
		}
		else {
			memzero(ret, sizeof(xmlXIncludeCtxt));
			ret->doc = doc;
			ret->incNr = 0;
			ret->incBase = 0;
			ret->incMax = 0;
			ret->incTab = NULL;
			ret->nbErrors = 0;
		}
	}
	return ret;
}
/**
 * xmlXIncludeURLPush:
 * @ctxt:  the parser context
 * @value:  the url
 *
 * Pushes a new url on top of the url stack
 *
 * Returns -1 in case of error, the index in the stack otherwise
 */
static int xmlXIncludeURLPush(xmlXIncludeCtxtPtr ctxt, const xmlChar * value)
{
	if(ctxt->urlNr > XINCLUDE_MAX_DEPTH) {
		xmlXIncludeErr(ctxt, NULL, XML_XINCLUDE_RECURSION, "detected a recursion in %s\n", value);
		return -1;
	}
	if(ctxt->urlTab == NULL) {
		ctxt->urlMax = 4;
		ctxt->urlNr = 0;
		ctxt->urlTab = static_cast<xmlChar **>(SAlloc::M(ctxt->urlMax * sizeof(ctxt->urlTab[0])));
		if(ctxt->urlTab == NULL) {
			xmlXIncludeErrMemory(ctxt, NULL, "adding URL");
			return -1;
		}
	}
	if(ctxt->urlNr >= ctxt->urlMax) {
		ctxt->urlMax *= 2;
		ctxt->urlTab = static_cast<xmlChar **>(SAlloc::R(ctxt->urlTab, ctxt->urlMax * sizeof(ctxt->urlTab[0])));
		if(ctxt->urlTab == NULL) {
			xmlXIncludeErrMemory(ctxt, NULL, "adding URL");
			return -1;
		}
	}
	ctxt->url = ctxt->urlTab[ctxt->urlNr] = sstrdup(value);
	return (ctxt->urlNr++);
}
/**
 * xmlXIncludeURLPop:
 * @ctxt: the parser context
 *
 * Pops the top URL from the URL stack
 */
static void xmlXIncludeURLPop(xmlXIncludeCtxtPtr ctxt)
{
	if(ctxt->urlNr > 0) {
		ctxt->urlNr--;
		ctxt->url = (ctxt->urlNr > 0) ? ctxt->urlTab[ctxt->urlNr-1] : NULL;
		xmlChar * ret = ctxt->urlTab[ctxt->urlNr];
		ctxt->urlTab[ctxt->urlNr] = NULL;
		SAlloc::F(ret);
	}
}
/**
 * xmlXIncludeFreeContext:
 * @ctxt: the XInclude context
 *
 * Free an XInclude context
 */
void FASTCALL xmlXIncludeFreeContext(xmlXIncludeCtxtPtr ctxt)
{
#ifdef DEBUG_XINCLUDE
	xmlGenericError(0, "Freeing context\n");
#endif
	if(ctxt) {
		int i;
		while(ctxt->urlNr > 0)
			xmlXIncludeURLPop(ctxt);
		SAlloc::F(ctxt->urlTab);
		for(i = 0; i < ctxt->incNr; i++)
			xmlXIncludeFreeRef(ctxt->incTab[i]);
		if(ctxt->txturlTab != NULL) {
			for(i = 0; i < ctxt->txtNr; i++) {
				SAlloc::F(ctxt->txturlTab[i]);
			}
		}
		SAlloc::F(ctxt->incTab);
		SAlloc::F(ctxt->txtTab);
		SAlloc::F(ctxt->txturlTab);
		SAlloc::F(ctxt->base);
		SAlloc::F(ctxt);
	}
}
/**
 * xmlXIncludeParseFile:
 * @ctxt:  the XInclude context
 * @URL:  the URL or file path
 *
 * parse a document for XInclude
 */
static xmlDoc * xmlXIncludeParseFile(xmlXIncludeCtxtPtr ctxt, const char * URL)
{
	xmlDoc * ret;
	xmlParserCtxt * pctxt;
	xmlParserInput * inputStream;
	xmlInitParser();
	pctxt = xmlNewParserCtxt();
	if(pctxt == NULL) {
		xmlXIncludeErrMemory(ctxt, NULL, "cannot allocate parser context");
		return 0;
	}
	/*
	 * pass in the application data to the parser context.
	 */
	pctxt->_private = ctxt->_private;
	/*
	 * try to ensure that new documents included are actually
	 * built with the same dictionary as the including document.
	 */
	if(ctxt->doc && ctxt->doc->dict) {
		xmlDictFree(pctxt->dict);
		pctxt->dict = ctxt->doc->dict;
		xmlDictReference(pctxt->dict);
	}
	xmlCtxtUseOptions(pctxt, ctxt->parseFlags | XML_PARSE_DTDLOAD);
	inputStream = xmlLoadExternalEntity(URL, NULL, pctxt);
	if(!inputStream) {
		xmlFreeParserCtxt(pctxt);
		return 0;
	}
	inputPush(pctxt, inputStream);
	if(pctxt->directory == NULL)
		pctxt->directory = xmlParserGetDirectory(URL);
	pctxt->loadsubset |= XML_DETECT_IDS;
	xmlParseDocument(pctxt);
	if(pctxt->wellFormed) {
		ret = pctxt->myDoc;
	}
	else {
		ret = NULL;
		if(pctxt->myDoc != NULL)
			xmlFreeDoc(pctxt->myDoc);
		pctxt->myDoc = NULL;
	}
	xmlFreeParserCtxt(pctxt);
	return ret;
}
/**
 * xmlXIncludeAddNode:
 * @ctxt:  the XInclude context
 * @cur:  the new node
 *
 * Add a new node to process to an XInclude context
 */
static int xmlXIncludeAddNode(xmlXIncludeCtxtPtr ctxt, xmlNode * cur)
{
	xmlXIncludeRefPtr ref;
	xmlURI * uri;
	xmlChar * URL;
	xmlChar * fragment = NULL;
	xmlChar * href;
	xmlChar * parse;
	xmlChar * base;
	xmlChar * URI;
	int xml = 1, i; /* default Issue 64 */
	int local = 0;
	if(!ctxt || !cur)
		return -1;
#ifdef DEBUG_XINCLUDE
	xmlGenericError(0, "Add node\n");
#endif
	// 
	// read the attributes
	// 
	href = xmlXIncludeGetProp(ctxt, cur, XINCLUDE_HREF);
	if(href == NULL) {
		href = sstrdup(reinterpret_cast<const xmlChar *>("")); /* @@@@ href is now optional */
		if(href == NULL)
			return -1;
	}
	if((href[0] == '#') || (href[0] == 0))
		local = 1;
	parse = xmlXIncludeGetProp(ctxt, cur, XINCLUDE_PARSE);
	if(parse != NULL) {
		if(sstreq(parse, XINCLUDE_PARSE_XML))
			xml = 1;
		else if(sstreq(parse, XINCLUDE_PARSE_TEXT))
			xml = 0;
		else {
			xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_PARSE_VALUE, "invalid value %s for 'parse'\n", parse);
			SAlloc::F(href);
			SAlloc::F(parse);
			return -1;
		}
	}
	/*
	 * compute the URI
	 */
	base = xmlNodeGetBase(ctxt->doc, cur);
	URI = (base == NULL) ? xmlBuildURI(href, ctxt->doc->URL) : xmlBuildURI(href, base);
	if(URI == NULL) {
		/*
		 * Some escaping may be needed
		 */
		xmlChar * escbase = xmlURIEscape(base);
		xmlChar * eschref = xmlURIEscape(href);
		URI = xmlBuildURI(eschref, escbase);
		SAlloc::F(escbase);
		SAlloc::F(eschref);
	}
	SAlloc::F(parse);
	SAlloc::F(href);
	SAlloc::F(base);
	if(URI == NULL) {
		xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_HREF_URI, "failed build URL\n", 0);
		return -1;
	}
	fragment = xmlXIncludeGetProp(ctxt, cur, XINCLUDE_PARSE_XPOINTER);
	/*
	 * Check the URL and remove any fragment identifier
	 */
	uri = xmlParseURI((const char *)URI);
	if(!uri) {
		xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_HREF_URI, "invalid value URI %s\n", URI);
		SAlloc::F(fragment);
		SAlloc::F(URI);
		return -1;
	}
	if(uri->fragment != NULL) {
		if(ctxt->legacy != 0) {
			if(fragment == NULL) {
				fragment = (xmlChar *)uri->fragment;
			}
			else {
				SAlloc::F(uri->fragment);
			}
		}
		else {
			xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_FRAGMENT_ID, "Invalid fragment identifier in URI %s use the xpointer attribute\n", URI);
			SAlloc::F(fragment);
			xmlFreeURI(uri);
			SAlloc::F(URI);
			return -1;
		}
		uri->fragment = NULL;
	}
	URL = xmlSaveUri(uri);
	xmlFreeURI(uri);
	SAlloc::F(URI);
	if(URL == NULL) {
		xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_HREF_URI, "invalid value URI %s\n", URI);
		SAlloc::F(fragment);
		return -1;
	}
	/*
	 * If local and xml then we need a fragment
	 */
	if((local == 1) && (xml == 1) && ((fragment == NULL) || (fragment[0] == 0))) {
		xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_RECURSION, "detected a local recursion with no xpointer in %s\n", URL);
		SAlloc::F(fragment);
		return -1;
	}
	/*
	 * Check the URL against the stack for recursions
	 */
	if((!local) && (xml == 1)) {
		for(i = 0; i < ctxt->urlNr; i++) {
			if(sstreq(URL, ctxt->urlTab[i])) {
				xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_RECURSION, "detected a recursion in %s\n", URL);
				return -1;
			}
		}
	}
	ref = xmlXIncludeNewRef(ctxt, URL, cur);
	if(ref == NULL) {
		return -1;
	}
	else {
		ref->fragment = fragment;
		ref->doc = NULL;
		ref->xml = xml;
		ref->count = 1;
		SAlloc::F(URL);
		return 0;
	}
}
/**
 * xmlXIncludeRecurseDoc:
 * @ctxt:  the XInclude context
 * @doc:  the new document
 * @url:  the associated URL
 *
 * The XInclude recursive nature is handled at this point.
 */
static void xmlXIncludeRecurseDoc(xmlXIncludeCtxtPtr ctxt, xmlDoc * doc, const xmlURL url ATTRIBUTE_UNUSED) 
{
	xmlXIncludeCtxtPtr newctxt;
	int i;
	/*
	 * Avoid recursion in already substitued resources
	   for (i = 0;i < ctxt->urlNr;i++) {
	    if(sstreq(doc->URL, ctxt->urlTab[i]))
	        return;
	   }
	 */
#ifdef DEBUG_XINCLUDE
	xmlGenericError(0, "Recursing in doc %s\n", doc->URL);
#endif
	/*
	 * Handle recursion here.
	 */
	newctxt = xmlXIncludeNewContext(doc);
	if(newctxt != NULL) {
		/*
		 * Copy the private user data
		 */
		newctxt->_private = ctxt->_private;
		/*
		 * Copy the existing document set
		 */
		newctxt->incMax = ctxt->incMax;
		newctxt->incNr = ctxt->incNr;
		newctxt->incTab = static_cast<xmlXIncludeRefPtr *>(SAlloc::M(newctxt->incMax * sizeof(newctxt->incTab[0])));
		if(newctxt->incTab == NULL) {
			xmlXIncludeErrMemory(ctxt, (xmlNode *)doc, "processing doc");
			SAlloc::F(newctxt);
			return;
		}
		/*
		 * copy the urlTab
		 */
		newctxt->urlMax = ctxt->urlMax;
		newctxt->urlNr = ctxt->urlNr;
		newctxt->urlTab = ctxt->urlTab;
		/*
		 * Inherit the existing base
		 */
		newctxt->base = sstrdup(ctxt->base);
		/*
		 * Inherit the documents already in use by other includes
		 */
		newctxt->incBase = ctxt->incNr;
		for(i = 0; i < ctxt->incNr; i++) {
			newctxt->incTab[i] = ctxt->incTab[i];
			newctxt->incTab[i]->count++; // prevent the recursion from freeing it 
		}
		//
		// The new context should also inherit the Parse Flags (bug 132597)
		//
		newctxt->parseFlags = ctxt->parseFlags;
		xmlXIncludeDoProcess(newctxt, doc, xmlDocGetRootElement(doc));
		for(i = 0; i < ctxt->incNr; i++) {
			newctxt->incTab[i]->count--;
			newctxt->incTab[i] = NULL;
		}
		/* urlTab may have been reallocated */
		ctxt->urlTab = newctxt->urlTab;
		ctxt->urlMax = newctxt->urlMax;
		newctxt->urlMax = 0;
		newctxt->urlNr = 0;
		newctxt->urlTab = NULL;
		xmlXIncludeFreeContext(newctxt);
	}
#ifdef DEBUG_XINCLUDE
	xmlGenericError(0, "Done recursing in doc %s\n", url);
#endif
}
/**
 * xmlXIncludeAddTxt:
 * @ctxt:  the XInclude context
 * @txt:  the new text node
 * @url:  the associated URL
 *
 * Add a new txtument to the list
 */
static void xmlXIncludeAddTxt(xmlXIncludeCtxtPtr ctxt, xmlNode * txt, const xmlURL url) 
{
#ifdef DEBUG_XINCLUDE
	xmlGenericError(0, "Adding text %s\n", url);
#endif
	if(ctxt->txtMax == 0) {
		ctxt->txtMax = 4;
		ctxt->txtTab = static_cast<xmlNode **>(SAlloc::M(ctxt->txtMax * sizeof(ctxt->txtTab[0])));
		if(ctxt->txtTab == NULL) {
			xmlXIncludeErrMemory(ctxt, NULL, "processing text");
			return;
		}
		ctxt->txturlTab = static_cast<xmlURL *>(SAlloc::M(ctxt->txtMax * sizeof(ctxt->txturlTab[0])));
		if(ctxt->txturlTab == NULL) {
			xmlXIncludeErrMemory(ctxt, NULL, "processing text");
			return;
		}
	}
	if(ctxt->txtNr >= ctxt->txtMax) {
		ctxt->txtMax *= 2;
		ctxt->txtTab = static_cast<xmlNode **>(SAlloc::R(ctxt->txtTab, ctxt->txtMax * sizeof(ctxt->txtTab[0])));
		if(ctxt->txtTab == NULL) {
			xmlXIncludeErrMemory(ctxt, NULL, "processing text");
			return;
		}
		ctxt->txturlTab = static_cast<xmlURL *>(SAlloc::R(ctxt->txturlTab, ctxt->txtMax * sizeof(ctxt->txturlTab[0])));
		if(ctxt->txturlTab == NULL) {
			xmlXIncludeErrMemory(ctxt, NULL, "processing text");
			return;
		}
	}
	ctxt->txtTab[ctxt->txtNr] = txt;
	ctxt->txturlTab[ctxt->txtNr] = sstrdup(url);
	ctxt->txtNr++;
}
//
// Node copy with specific semantic
//
static xmlNode * xmlXIncludeCopyNodeList(xmlXIncludeCtxtPtr ctxt, xmlDoc * target, xmlDoc * source, xmlNode * elem);
/**
 * xmlXIncludeCopyNode:
 * @ctxt:  the XInclude context
 * @target:  the document target
 * @source:  the document source
 * @elem:  the element
 *
 * Make a copy of the node while preserving the XInclude semantic
 * of the Infoset copy
 */
static xmlNode * xmlXIncludeCopyNode(xmlXIncludeCtxtPtr ctxt, xmlDoc * target, xmlDoc * source, xmlNode * elem) 
{
	xmlNode * result = NULL;
	if(!ctxt || (target == NULL) || (source == NULL) || (elem == NULL))
		return 0;
	if(elem->type == XML_DTD_NODE)
		return 0;
	if(elem->type == XML_DOCUMENT_NODE)
		result = xmlXIncludeCopyNodeList(ctxt, target, source, elem->children);
	else
		result = xmlDocCopyNode(elem, target, 1);
	return result;
}
/**
 * xmlXIncludeCopyNodeList:
 * @ctxt:  the XInclude context
 * @target:  the document target
 * @source:  the document source
 * @elem:  the element list
 *
 * Make a copy of the node list while preserving the XInclude semantic
 * of the Infoset copy
 */
static xmlNode * xmlXIncludeCopyNodeList(xmlXIncludeCtxtPtr ctxt, xmlDoc * target, xmlDoc * source, xmlNode * elem) 
{
	xmlNode * cur;
	xmlNode * res;
	xmlNode * result = NULL;
	xmlNode * last = NULL;
	if(!ctxt || (target == NULL) || (source == NULL) || (elem == NULL))
		return 0;
	cur = elem;
	while(cur) {
		res = xmlXIncludeCopyNode(ctxt, target, source, cur);
		if(res != NULL) {
			if(result == NULL) {
				result = last = res;
			}
			else {
				last->next = res;
				res->prev = last;
				last = res;
			}
		}
		cur = cur->next;
	}
	return result;
}
/**
 * xmlXIncludeGetNthChild:
 * @cur:  the node
 * @no:  the child number
 *
 * Returns the @n'th element child of @cur or NULL
 */
static xmlNode * xmlXIncludeGetNthChild(xmlNode * cur, int no) 
{
	int i;
	if(!cur || (cur->type == XML_NAMESPACE_DECL))
		return 0;
	cur = cur->children;
	for(i = 0; i <= no; cur = cur->next) {
		if(!cur)
			return cur;
		if((cur->type == XML_ELEMENT_NODE) || (cur->type == XML_DOCUMENT_NODE) || (cur->type == XML_HTML_DOCUMENT_NODE)) {
			i++;
			if(i == no)
				break;
		}
	}
	return cur;
}

xmlNode * xmlXPtrAdvanceNode(xmlNode * cur, int * level); /* in xpointer.c */
/**
 * xmlXIncludeCopyRange:
 * @ctxt:  the XInclude context
 * @target:  the document target
 * @source:  the document source
 * @obj:  the XPointer result from the evaluation.
 *
 * Build a node list tree copy of the XPointer result.
 *
 * Returns an xmlNode * list or NULL.
 *    The caller has to free the node tree.
 */
static xmlNode * xmlXIncludeCopyRange(xmlXIncludeCtxtPtr ctxt, xmlDoc * target, xmlDoc * source, xmlXPathObject * range)
{
	/* pointers to generated nodes */
	xmlNode * list = NULL;
	xmlNode * last = NULL;
	xmlNode * listParent = NULL;
	xmlNode * tmp;
	xmlNode * tmp2;
	/* pointers to traversal nodes */
	xmlNode * start;
	xmlNode * cur;
	xmlNode * end;
	int index1, index2;
	int level = 0, lastLevel = 0, endLevel = 0, endFlag = 0;
	if(!ctxt || (target == NULL) || (source == NULL) || (range == NULL))
		return 0;
	if(range->type != XPATH_RANGE)
		return 0;
	start = (xmlNode *)range->user;
	if((start == NULL) || (start->type == XML_NAMESPACE_DECL))
		return 0;
	end = (xmlNode *)range->user2;
	if(end == NULL)
		return (xmlDocCopyNode(start, target, 1));
	if(end->type == XML_NAMESPACE_DECL)
		return 0;

	cur = start;
	index1 = range->index;
	index2 = range->index2;
	/*
	 * level is depth of the current node under consideration
	 * list is the pointer to the root of the output tree
	 * listParent is a pointer to the parent of output tree (within
	   the included file) in case we need to add another level
	 * last is a pointer to the last node added to the output tree
	 * lastLevel is the depth of last (relative to the root)
	 */
	while(cur) {
		/*
		 * Check if our output tree needs a parent
		 */
		if(level < 0) {
			while(level < 0) {
				/* copy must include namespaces and properties */
				tmp2 = xmlDocCopyNode(listParent, target, 2);
				xmlAddChild(tmp2, list);
				list = tmp2;
				listParent = listParent->P_ParentNode;
				level++;
			}
			last = list;
			lastLevel = 0;
		}
		/*
		 * Check whether we need to change our insertion point
		 */
		while(level < lastLevel) {
			last = last->P_ParentNode;
			lastLevel--;
		}
		if(cur == end) { /* Are we at the end of the range? */
			if(cur->type == XML_TEXT_NODE) {
				const xmlChar * content = cur->content;
				int len;
				if(content == NULL) {
					tmp = xmlNewTextLen(NULL, 0);
				}
				else {
					len = index2;
					if((cur == start) && (index1 > 1)) {
						content += (index1 - 1);
						len -= (index1 - 1);
					}
					else {
						len = index2;
					}
					tmp = xmlNewTextLen(content, len);
				}
				/* single sub text node selection */
				if(list == NULL)
					return tmp;
				/* prune and return full set */
				if(level == lastLevel)
					xmlAddNextSibling(last, tmp);
				else
					xmlAddChild(last, tmp);
				return (list);
			}
			else { /* ending node not a text node */
				endLevel = level; /* remember the level of the end node */
				endFlag = 1;
				/* last node - need to take care of properties + namespaces */
				tmp = xmlDocCopyNode(cur, target, 2);
				if(!list) {
					list = tmp;
					listParent = cur->P_ParentNode;
				}
				else {
					if(level == lastLevel)
						xmlAddNextSibling(last, tmp);
					else {
						xmlAddChild(last, tmp);
						lastLevel = level;
					}
				}
				last = tmp;

				if(index2 > 1) {
					end = xmlXIncludeGetNthChild(cur, index2 - 1);
					index2 = 0;
				}
				if((cur == start) && (index1 > 1)) {
					cur = xmlXIncludeGetNthChild(cur, index1 - 1);
					index1 = 0;
				}
				else {
					cur = cur->children;
				}
				level++; /* increment level to show change */
				/*
				 * Now gather the remaining nodes from cur to end
				 */
				continue; /* while */
			}
		}
		else if(cur == start) { /* Not at the end, are we at start? */
			if((cur->type == XML_TEXT_NODE) ||
			    (cur->type == XML_CDATA_SECTION_NODE)) {
				const xmlChar * content = cur->content;
				if(!content) {
					tmp = xmlNewTextLen(NULL, 0);
				}
				else {
					if(index1 > 1) {
						content += (index1 - 1);
						index1 = 0;
					}
					tmp = xmlNewText(content);
				}
				last = list = tmp;
				listParent = cur->P_ParentNode;
			}
			else {  /* Not text node */
				/*
				 * start of the range - need to take care of
				 * properties and namespaces
				 */
				tmp = xmlDocCopyNode(cur, target, 2);
				list = last = tmp;
				listParent = cur->P_ParentNode;
				if(index1 > 1) { /* Do we need to position? */
					cur = xmlXIncludeGetNthChild(cur, index1 - 1);
					level = lastLevel = 1;
					index1 = 0;
					/*
					 * Now gather the remaining nodes from cur to end
					 */
					continue; /* while */
				}
			}
		}
		else {
			tmp = NULL;
			switch(cur->type) {
				case XML_DTD_NODE:
				case XML_ELEMENT_DECL:
				case XML_ATTRIBUTE_DECL:
				case XML_ENTITY_NODE:
				    /* Do not copy DTD informations */
				    break;
				case XML_ENTITY_DECL:
				    /* handle crossing entities -> stack needed */
				    break;
				case XML_XINCLUDE_START:
				case XML_XINCLUDE_END:
				    /* don't consider it part of the tree content */
				    break;
				case XML_ATTRIBUTE_NODE:
				    /* Humm, should not happen ! */
				    break;
				default:
				    /*
				 * Middle of the range - need to take care of
				 * properties and namespaces
				     */
				    tmp = xmlDocCopyNode(cur, target, 2);
				    break;
			}
			if(tmp) {
				if(level == lastLevel)
					xmlAddNextSibling(last, tmp);
				else {
					xmlAddChild(last, tmp);
					lastLevel = level;
				}
				last = tmp;
			}
		}
		/*
		 * Skip to next node in document order
		 */
		cur = xmlXPtrAdvanceNode(cur, &level);
		if(endFlag && (level >= endLevel))
			break;
	}
	return (list);
}

/**
 * xmlXIncludeBuildNodeList:
 * @ctxt:  the XInclude context
 * @target:  the document target
 * @source:  the document source
 * @obj:  the XPointer result from the evaluation.
 *
 * Build a node list tree copy of the XPointer result.
 * This will drop Attributes and Namespace declarations.
 *
 * Returns an xmlNode * list or NULL.
 *    the caller has to free the node tree.
 */
static xmlNode * xmlXIncludeCopyXPointer(xmlXIncludeCtxtPtr ctxt, xmlDoc * target, xmlDoc * source, xmlXPathObject * obj) 
{
	xmlNode * list = NULL;
	xmlNode * last = NULL;
	int i;
	if(source == NULL)
		source = ctxt->doc;
	if(!ctxt || (target == NULL) || (source == NULL) ||
	    (obj == NULL))
		return 0;
	switch(obj->type) {
		case XPATH_NODESET: {
		    xmlNodeSet * set = obj->nodesetval;
		    if(set == NULL)
			    return 0;
		    for(i = 0; i < set->nodeNr; i++) {
			    if(set->PP_NodeTab[i] == NULL)
				    continue;
			    switch(set->PP_NodeTab[i]->type) {
				    case XML_TEXT_NODE:
				    case XML_CDATA_SECTION_NODE:
				    case XML_ELEMENT_NODE:
				    case XML_ENTITY_REF_NODE:
				    case XML_ENTITY_NODE:
				    case XML_PI_NODE:
				    case XML_COMMENT_NODE:
				    case XML_DOCUMENT_NODE:
				    case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
				    case XML_DOCB_DOCUMENT_NODE:
#endif
				    case XML_XINCLUDE_END:
					break;
				    case XML_XINCLUDE_START: {
					xmlNode * tmp;
					xmlNode * cur = set->PP_NodeTab[i];
					cur = cur->next;
					while(cur) {
						switch(cur->type) {
							case XML_TEXT_NODE:
							case XML_CDATA_SECTION_NODE:
							case XML_ELEMENT_NODE:
							case XML_ENTITY_REF_NODE:
							case XML_ENTITY_NODE:
							case XML_PI_NODE:
							case XML_COMMENT_NODE:
							    tmp = xmlXIncludeCopyNode(ctxt, target, source, cur);
							    if(last == NULL) {
								    list = last = tmp;
							    }
							    else {
								    xmlAddNextSibling(last, tmp);
								    last = tmp;
							    }
							    cur = cur->next;
							    continue;
							default:
							    break;
						}
						break;
					}
					continue;
				}
				    case XML_ATTRIBUTE_NODE:
				    case XML_NAMESPACE_DECL:
				    case XML_DOCUMENT_TYPE_NODE:
				    case XML_DOCUMENT_FRAG_NODE:
				    case XML_NOTATION_NODE:
				    case XML_DTD_NODE:
				    case XML_ELEMENT_DECL:
				    case XML_ATTRIBUTE_DECL:
				    case XML_ENTITY_DECL:
					continue; /* for */
			    }
			    if(last == NULL)
				    list = last = xmlXIncludeCopyNode(ctxt, target, source, set->PP_NodeTab[i]);
			    else {
				    xmlAddNextSibling(last, xmlXIncludeCopyNode(ctxt, target, source, set->PP_NodeTab[i]));
				    if(last->next)
					    last = last->next;
			    }
		    }
		    break;
	    }
#ifdef LIBXML_XPTR_ENABLED
		case XPATH_LOCATIONSET: {
		    xmlLocationSet * set = (xmlLocationSet *)obj->user;
		    if(set == NULL)
			    return 0;
		    for(i = 0; i < set->locNr; i++) {
			    if(last == NULL)
				    list = last = xmlXIncludeCopyXPointer(ctxt, target, source, set->locTab[i]);
			    else
				    xmlAddNextSibling(last, xmlXIncludeCopyXPointer(ctxt, target, source, set->locTab[i]));
			    if(last) {
				    while(last->next)
					    last = last->next;
			    }
		    }
		    break;
	    }
		case XPATH_RANGE:
		    return (xmlXIncludeCopyRange(ctxt, target, source, obj));
#endif
		case XPATH_POINT:
		    /* points are ignored in XInclude */
		    break;
		default:
		    break;
	}
	return (list);
}
//
// XInclude I/O handling
//
//typedef struct _xmlXIncludeMergeData xmlXIncludeMergeData;
//typedef xmlXIncludeMergeData * xmlXIncludeMergeDataPtr;

struct xmlXIncludeMergeData {
	xmlXIncludeMergeData(xmlXIncludeCtxt * pCtx, xmlDoc * pDoc) : ctxt(pCtx), doc(pDoc)
	{
	}
	xmlDoc * doc;
	xmlXIncludeCtxt * ctxt;
};

/**
 * xmlXIncludeMergeOneEntity:
 * @ent: the entity
 * @doc:  the including doc
 * @nr: the entity name
 *
 * Inplements the merge of one entity
 */
static void xmlXIncludeMergeEntity(xmlEntity * ent, xmlXIncludeMergeData * data, xmlChar * name ATTRIBUTE_UNUSED) 
{
	xmlEntity * ret;
	xmlEntity * prev;
	xmlDoc * doc;
	xmlXIncludeCtxtPtr ctxt;
	if(!ent || !data)
		return;
	ctxt = data->ctxt;
	doc = data->doc;
	if(!ctxt || !doc)
		return;
	switch(ent->etype) {
		case XML_INTERNAL_PARAMETER_ENTITY:
		case XML_EXTERNAL_PARAMETER_ENTITY:
		case XML_INTERNAL_PREDEFINED_ENTITY:
		    return;
		case XML_INTERNAL_GENERAL_ENTITY:
		case XML_EXTERNAL_GENERAL_PARSED_ENTITY:
		case XML_EXTERNAL_GENERAL_UNPARSED_ENTITY:
		    break;
	}
	ret = xmlAddDocEntity(doc, ent->name, ent->etype, ent->ExternalID, ent->SystemID, ent->content);
	if(ret) {
		if(ent->URI != NULL)
			ret->URI = sstrdup(ent->URI);
	}
	else {
		prev = xmlGetDocEntity(doc, ent->name);
		if(prev) {
			if(ent->etype != prev->etype)
				goto error;
			if(ent->SystemID && prev->SystemID) {
				if(!sstreq(ent->SystemID, prev->SystemID))
					goto error;
			}
			else if(ent->ExternalID && prev->ExternalID) {
				if(!sstreq(ent->ExternalID, prev->ExternalID))
					goto error;
			}
			else if(ent->content && prev->content) {
				if(!sstreq(ent->content, prev->content))
					goto error;
			}
			else {
				goto error;
			}
		}
	}
	return;
error:
	switch(ent->etype) {
		case XML_INTERNAL_PARAMETER_ENTITY:
		case XML_EXTERNAL_PARAMETER_ENTITY:
		case XML_INTERNAL_PREDEFINED_ENTITY:
		case XML_INTERNAL_GENERAL_ENTITY:
		case XML_EXTERNAL_GENERAL_PARSED_ENTITY:
		    return;
		case XML_EXTERNAL_GENERAL_UNPARSED_ENTITY:
		    break;
	}
	xmlXIncludeErr(ctxt, (xmlNode *)ent, XML_XINCLUDE_ENTITY_DEF_MISMATCH, "mismatch in redefinition of entity %s\n", ent->name);
}
/**
 * xmlXIncludeMergeEntities:
 * @ctxt: an XInclude context
 * @doc:  the including doc
 * @from:  the included doc
 *
 * Inplements the entity merge
 *
 * Returns 0 if merge succeeded, -1 if some processing failed
 */
static int xmlXIncludeMergeEntities(xmlXIncludeCtxtPtr ctxt, xmlDoc * doc, xmlDoc * from) 
{
	xmlNode * cur;
	xmlDtd * target;
	xmlDtd * source;
	if(!ctxt)
		return -1;
	if((from == NULL) || (from->intSubset == NULL))
		return 0;
	target = doc->intSubset;
	if(target == NULL) {
		cur = xmlDocGetRootElement(doc);
		if(!cur)
			return -1;
		target = xmlCreateIntSubset(doc, cur->name, 0, 0);
		if(target == NULL)
			return -1;
	}
	source = from->intSubset;
	if(source && source->entities) {
		xmlXIncludeMergeData data(ctxt, doc);
		xmlHashScan((xmlHashTable *)source->entities, (xmlHashScanner)xmlXIncludeMergeEntity, &data);
	}
	source = from->extSubset;
	if(source && source->entities) {
		xmlXIncludeMergeData data(ctxt, doc);
		/*
		 * don't duplicate existing stuff when external subsets are the same
		 */
		if(!sstreq(target->ExternalID, source->ExternalID) && !sstreq(target->SystemID, source->SystemID)) {
			xmlHashScan((xmlHashTable *)source->entities, (xmlHashScanner)xmlXIncludeMergeEntity, &data);
		}
	}
	return 0;
}
/**
 * xmlXIncludeLoadDoc:
 * @ctxt:  the XInclude context
 * @url:  the associated URL
 * @nr:  the xinclude node number
 *
 * Load the document, and store the result in the XInclude context
 *
 * Returns 0 in case of success, -1 in case of failure
 */
static int xmlXIncludeLoadDoc(xmlXIncludeCtxtPtr ctxt, const xmlChar * url, int nr) 
{
	xmlDoc * doc;
	xmlURI * uri;
	xmlChar * URL;
	xmlChar * fragment = NULL;
	int i = 0;
#ifdef LIBXML_XPTR_ENABLED
	int saveFlags;
#endif
#ifdef DEBUG_XINCLUDE
	xmlGenericError(0, "Loading doc %s:%d\n", url, nr);
#endif
	/*
	 * Check the URL and remove any fragment identifier
	 */
	uri = xmlParseURI((const char *)url);
	if(!uri) {
		xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_HREF_URI, "invalid value URI %s\n", url);
		return -1;
	}
	if(uri->fragment) {
		fragment = (xmlChar *)uri->fragment;
		uri->fragment = NULL;
	}
	if(ctxt->incTab && ctxt->incTab[nr] && ctxt->incTab[nr]->fragment) {
		SAlloc::F(fragment);
		fragment = sstrdup(ctxt->incTab[nr]->fragment);
	}
	URL = xmlSaveUri(uri);
	xmlFreeURI(uri);
	if(URL == NULL) {
		if(ctxt->incTab)
			xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_HREF_URI, "invalid value URI %s\n", url);
		else
			xmlXIncludeErr(ctxt, NULL, XML_XINCLUDE_HREF_URI, "invalid value URI %s\n", url);
		SAlloc::F(fragment);
		return -1;
	}
	/*
	 * Handling of references to the local document are done
	 * directly through ctxt->doc.
	 */
	if((URL[0] == 0) || (URL[0] == '#') || ((ctxt->doc != NULL) && (sstreq(URL, ctxt->doc->URL)))) {
		doc = NULL;
		goto loaded;
	}
	/*
	 * Prevent reloading twice the document.
	 */
	for(i = 0; i < ctxt->incNr; i++) {
		if(sstreq(URL, ctxt->incTab[i]->URI) && ctxt->incTab[i]->doc) {
			doc = ctxt->incTab[i]->doc;
#ifdef DEBUG_XINCLUDE
			printf("Already loaded %s\n", URL);
#endif
			goto loaded;
		}
	}

	/*
	 * Load it.
	 */
#ifdef DEBUG_XINCLUDE
	printf("loading %s\n", URL);
#endif
#ifdef LIBXML_XPTR_ENABLED
	/*
	 * If this is an XPointer evaluation, we want to assure that
	 * all entities have been resolved prior to processing the
	 * referenced document
	 */
	saveFlags = ctxt->parseFlags;
	if(fragment != NULL) {  /* if this is an XPointer eval */
		ctxt->parseFlags |= XML_PARSE_NOENT;
	}
#endif
	doc = xmlXIncludeParseFile(ctxt, (const char *)URL);
#ifdef LIBXML_XPTR_ENABLED
	ctxt->parseFlags = saveFlags;
#endif
	if(!doc) {
		SAlloc::F(URL);
		SAlloc::F(fragment);
		return -1;
	}
	ctxt->incTab[nr]->doc = doc;
	/*
	 * It's possible that the requested URL has been mapped to a
	 * completely different location (e.g. through a catalog entry).
	 * To check for this, we compare the URL with that of the doc
	 * and change it if they disagree (bug 146988).
	 */
	if(!sstreq(URL, doc->URL)) {
		SAlloc::F(URL);
		URL = sstrdup(doc->URL);
	}
	for(i = nr + 1; i < ctxt->incNr; i++) {
		if(sstreq(URL, ctxt->incTab[i]->URI)) {
			ctxt->incTab[nr]->count++;
#ifdef DEBUG_XINCLUDE
			printf("Increasing %s count since reused\n", URL);
#endif
			break;
		}
	}

	/*
	 * Make sure we have all entities fixed up
	 */
	xmlXIncludeMergeEntities(ctxt, ctxt->doc, doc);
	/*
	 * We don't need the DTD anymore, free up space
	   if(doc->intSubset != NULL) {
	    xmlUnlinkNode((xmlNode *) doc->intSubset);
	    xmlFreeNode((xmlNode *) doc->intSubset);
	    doc->intSubset = NULL;
	   }
	   if(doc->extSubset != NULL) {
	    xmlUnlinkNode((xmlNode *) doc->extSubset);
	    xmlFreeNode((xmlNode *) doc->extSubset);
	    doc->extSubset = NULL;
	   }
	 */
	xmlXIncludeRecurseDoc(ctxt, doc, URL);
loaded:
	if(fragment == NULL) {
		/*
		 * Add the top children list as the replacement copy.
		 */
		if(!doc) {
			// Hopefully a DTD declaration won't be copied from the same document 
			ctxt->incTab[nr]->inc = xmlCopyNodeList(ctxt->doc->children);
		}
		else {
			ctxt->incTab[nr]->inc = xmlXIncludeCopyNodeList(ctxt, ctxt->doc, doc, doc->children);
		}
	}
#ifdef LIBXML_XPTR_ENABLED
	else {
		/*
		 * Computes the XPointer expression and make a copy used
		 * as the replacement copy.
		 */
		xmlXPathObject * xptr;
		xmlNodeSet * set;
		xmlXPathContext * xptrctxt = doc ? xmlXPtrNewContext(doc, 0, 0) : xmlXPtrNewContext(ctxt->doc, ctxt->incTab[nr]->ref, 0);
		if(!xptrctxt) {
			xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_XPTR_FAILED, "could not create XPointer context\n", 0);
			SAlloc::F(URL);
			SAlloc::F(fragment);
			return -1;
		}
		xptr = xmlXPtrEval(fragment, xptrctxt);
		if(!xptr) {
			xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_XPTR_FAILED, "XPointer evaluation failed: #%s\n", fragment);
			xmlXPathFreeContext(xptrctxt);
			SAlloc::F(URL);
			SAlloc::F(fragment);
			return -1;
		}
		switch(xptr->type) {
			case XPATH_UNDEFINED:
			case XPATH_BOOLEAN:
			case XPATH_NUMBER:
			case XPATH_STRING:
			case XPATH_POINT:
			case XPATH_USERS:
			case XPATH_XSLT_TREE:
			    xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_XPTR_RESULT, "XPointer is not a range: #%s\n", fragment);
			    xmlXPathFreeContext(xptrctxt);
			    SAlloc::F(URL);
			    SAlloc::F(fragment);
			    return -1;
			case XPATH_NODESET:
			    if((xptr->nodesetval == NULL) || (xptr->nodesetval->nodeNr <= 0)) {
				    xmlXPathFreeContext(xptrctxt);
				    SAlloc::F(URL);
				    SAlloc::F(fragment);
				    return -1;
			    }

			case XPATH_RANGE:
			case XPATH_LOCATIONSET:
			    break;
		}
		set = xptr->nodesetval;
		if(set != NULL) {
			for(i = 0; i < set->nodeNr; i++) {
				if(set->PP_NodeTab[i] == NULL)
					continue;
				switch(set->PP_NodeTab[i]->type) {
					case XML_ELEMENT_NODE:
					case XML_TEXT_NODE:
					case XML_CDATA_SECTION_NODE:
					case XML_ENTITY_REF_NODE:
					case XML_ENTITY_NODE:
					case XML_PI_NODE:
					case XML_COMMENT_NODE:
					case XML_DOCUMENT_NODE:
					case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
					case XML_DOCB_DOCUMENT_NODE:
#endif
					    continue;

					case XML_ATTRIBUTE_NODE:
					    xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_XPTR_RESULT, "XPointer selects an attribute: #%s\n", fragment);
					    set->PP_NodeTab[i] = NULL;
					    continue;
					case XML_NAMESPACE_DECL:
					    xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_XPTR_RESULT, "XPointer selects a namespace: #%s\n", fragment);
					    set->PP_NodeTab[i] = NULL;
					    continue;
					case XML_DOCUMENT_TYPE_NODE:
					case XML_DOCUMENT_FRAG_NODE:
					case XML_NOTATION_NODE:
					case XML_DTD_NODE:
					case XML_ELEMENT_DECL:
					case XML_ATTRIBUTE_DECL:
					case XML_ENTITY_DECL:
					case XML_XINCLUDE_START:
					case XML_XINCLUDE_END:
					    xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_XPTR_RESULT, "XPointer selects unexpected nodes: #%s\n", fragment);
					    set->PP_NodeTab[i] = NULL;
					    set->PP_NodeTab[i] = NULL;
					    continue; /* for */
				}
			}
		}
		if(!doc) {
			ctxt->incTab[nr]->xptr = xptr;
			ctxt->incTab[nr]->inc = NULL;
		}
		else {
			ctxt->incTab[nr]->inc = xmlXIncludeCopyXPointer(ctxt, ctxt->doc, doc, xptr);
			xmlXPathFreeObject(xptr);
		}
		xmlXPathFreeContext(xptrctxt);
		SAlloc::F(fragment);
	}
#endif
	/*
	 * Do the xml:base fixup if needed
	 */
	if(doc && URL && (!(ctxt->parseFlags & XML_PARSE_NOBASEFIX)) && (!(doc->parseFlags & XML_PARSE_NOBASEFIX))) {
		xmlChar * curBase;
		/*
		 * The base is only adjusted if "necessary", i.e. if the xinclude node
		 * has a base specified, or the URL is relative
		 */
		xmlChar * base = xmlGetNsProp(ctxt->incTab[nr]->ref, reinterpret_cast<const xmlChar *>("base"), XML_XML_NAMESPACE);
		if(!base) {
			/*
			 * No xml:base on the xinclude node, so we check whether the
			 * URI base is different than (relative to) the context base
			 */
			curBase = xmlBuildRelativeURI(URL, ctxt->base);
			if(!curBase) { /* Error return */
				xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_HREF_URI, "trying to build relative URI from %s\n", URL);
			}
			else {
				// If the URI doesn't contain a slash, it's not relative 
				if(!xmlStrchr(curBase, (xmlChar)'/'))
					SAlloc::F(curBase);
				else
					base = curBase;
			}
		}
		if(base) { // Adjustment may be needed 
			for(xmlNode * p_node = ctxt->incTab[nr]->inc; p_node; p_node = p_node->next) {
				// Only work on element nodes 
				if(p_node->type == XML_ELEMENT_NODE) {
					curBase = xmlNodeGetBase(p_node->doc, p_node);
					// If no current base, set it 
					if(!curBase) {
						xmlNodeSetBase(p_node, base);
					}
					else {
						/*
						 * If the current base is the same as the
						 * URL of the document, then reset it to be
						 * the specified xml:base or the relative URI
						 */
						if(sstreq(curBase, p_node->doc->URL)) {
							xmlNodeSetBase(p_node, base);
						}
						else {
							// If the element already has an xml:base set, then relativise it if necessary
							xmlChar * xmlBase = xmlGetNsProp(p_node, reinterpret_cast<const xmlChar *>("base"), XML_XML_NAMESPACE);
							if(xmlBase) {
								xmlChar * relBase = xmlBuildURI(xmlBase, base);
								if(!relBase) // error 
									xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_HREF_URI, "trying to rebuild base from %s\n", xmlBase);
								else {
									xmlNodeSetBase(p_node, relBase);
									SAlloc::F(relBase);
								}
								SAlloc::F(xmlBase);
							}
						}
						SAlloc::F(curBase);
					}
				}
			}
			SAlloc::F(base);
		}
	}
	if((nr < ctxt->incNr) && (ctxt->incTab[nr]->doc != NULL) && (ctxt->incTab[nr]->count <= 1)) {
#ifdef DEBUG_XINCLUDE
		printf("freeing %s\n", ctxt->incTab[nr]->doc->URL);
#endif
		xmlFreeDoc(ctxt->incTab[nr]->doc);
		ctxt->incTab[nr]->doc = NULL;
	}
	SAlloc::F(URL);
	return 0;
}

/**
 * xmlXIncludeLoadTxt:
 * @ctxt:  the XInclude context
 * @url:  the associated URL
 * @nr:  the xinclude node number
 *
 * Load the content, and store the result in the XInclude context
 *
 * Returns 0 in case of success, -1 in case of failure
 */
static int xmlXIncludeLoadTxt(xmlXIncludeCtxtPtr ctxt, const xmlChar * url, int nr) 
{
	xmlParserInputBuffer * buf;
	xmlNode * P_Node;
	xmlChar * URL;
	int i;
	xmlChar * encoding = NULL;
	xmlCharEncoding enc = (xmlCharEncoding)0;
	xmlParserCtxt * pctxt;
	xmlParserInput * inputStream;
	int xinclude_multibyte_fallback_used = 0;
	/*
	 * Check the URL and remove any fragment identifier
	 */
	xmlURI * uri = xmlParseURI((const char *)url);
	if(!uri) {
		xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_HREF_URI, "invalid value URI %s\n", url);
		return -1;
	}
	if(uri->fragment) {
		xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_TEXT_FRAGMENT, "fragment identifier forbidden for text: %s\n", (const xmlChar *)uri->fragment);
		xmlFreeURI(uri);
		return -1;
	}
	URL = xmlSaveUri(uri);
	xmlFreeURI(uri);
	if(URL == NULL) {
		xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_HREF_URI, "invalid value URI %s\n", url);
		return -1;
	}
	/*
	 * Handling of references to the local document are done directly through ctxt->doc.
	 */
	if(URL[0] == 0) {
		xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_TEXT_DOCUMENT, "text serialization of document not available\n", 0);
		SAlloc::F(URL);
		return -1;
	}
	/*
	 * Prevent reloading twice the document.
	 */
	for(i = 0; i < ctxt->txtNr; i++) {
		if(sstreq(URL, ctxt->txturlTab[i])) {
			P_Node = xmlCopyNode(ctxt->txtTab[i], 1);
			goto loaded;
		}
	}
	/*
	 * Try to get the encoding if available
	 */
	if(ctxt->incTab[nr] && ctxt->incTab[nr]->ref) {
		encoding = xmlGetProp(ctxt->incTab[nr]->ref, XINCLUDE_PARSE_ENCODING);
	}
	if(encoding) {
		/*
		 * @todo we should not have to remap to the xmlCharEncoding
		 *  predefined set, a better interface than
		 *  xmlParserInputBufferCreateFilename should allow any
		 *  encoding supported by iconv
		 */
		enc = xmlParseCharEncoding(reinterpret_cast<const char *>(encoding));
		if(enc == XML_CHAR_ENCODING_ERROR) {
			xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_UNKNOWN_ENCODING, "encoding %s not supported\n", encoding);
			SAlloc::F(encoding);
			SAlloc::F(URL);
			return -1;
		}
		SAlloc::F(encoding);
	}
	/*
	 * Load it.
	 */
	pctxt = xmlNewParserCtxt();
	inputStream = xmlLoadExternalEntity((const char *)URL, NULL, pctxt);
	if(!inputStream) {
		xmlFreeParserCtxt(pctxt);
		SAlloc::F(URL);
		return -1;
	}
	buf = inputStream->buf;
	if(!buf) {
		xmlFreeInputStream(inputStream);
		xmlFreeParserCtxt(pctxt);
		SAlloc::F(URL);
		return -1;
	}
	if(buf->encoder)
		xmlCharEncCloseFunc(buf->encoder);
	buf->encoder = xmlGetCharEncodingHandler(enc);
	P_Node = xmlNewText(NULL);

	/*
	 * Scan all chars from the resource and add the to the node
	 */
xinclude_multibyte_fallback:
	while(xmlParserInputBufferRead(buf, 128) > 0) {
		const xmlChar * content = xmlBufContent(buf->buffer);
		int len = xmlBufLength(buf->buffer);
		for(i = 0; i < len; ) {
			int l;
			int cur = xmlStringCurrentChar(NULL, &content[i], &l);
			if(!IS_CHAR(cur)) {
				/* Handle splitted multibyte char at buffer boundary */
				if(((len - i) < 4) && (!xinclude_multibyte_fallback_used)) {
					xinclude_multibyte_fallback_used = 1;
					xmlBufShrink(buf->buffer, i);
					goto xinclude_multibyte_fallback;
				}
				else {
					xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_INVALID_CHAR, "%s contains invalid char\n", URL);
					xmlFreeParserInputBuffer(buf);
					SAlloc::F(URL);
					return -1;
				}
			}
			else {
				xinclude_multibyte_fallback_used = 0;
				xmlNodeAddContentLen(P_Node, &content[i], l);
			}
			i += l;
		}
		xmlBufShrink(buf->buffer, len);
	}
	xmlFreeParserCtxt(pctxt);
	xmlXIncludeAddTxt(ctxt, P_Node, URL);
	xmlFreeInputStream(inputStream);

loaded:
	/*
	 * Add the element as the replacement copy.
	 */
	ctxt->incTab[nr]->inc = P_Node;
	SAlloc::F(URL);
	return 0;
}
/**
 * xmlXIncludeLoadFallback:
 * @ctxt:  the XInclude context
 * @fallback:  the fallback node
 * @nr:  the xinclude node number
 *
 * Load the content of the fallback node, and store the result
 * in the XInclude context
 *
 * Returns 0 in case of success, -1 in case of failure
 */
static int xmlXIncludeLoadFallback(xmlXIncludeCtxtPtr ctxt, xmlNode * fallback, int nr) 
{
	xmlXIncludeCtxtPtr newctxt;
	int ret = 0;
	if(!fallback || (fallback->type == XML_NAMESPACE_DECL) || !ctxt)
		return -1;
	if(fallback->children != NULL) {
		// 
		// It's possible that the fallback also has 'includes' (Bug 129969), so we re-process the fallback just in case
		// 
		newctxt = xmlXIncludeNewContext(ctxt->doc);
		if(newctxt == NULL)
			return -1;
		newctxt->_private = ctxt->_private;
		newctxt->base = sstrdup(ctxt->base); /* Inherit the base from the existing context */
		xmlXIncludeSetFlags(newctxt, ctxt->parseFlags);
		ret = xmlXIncludeDoProcess(newctxt, ctxt->doc, fallback->children);
		if(ctxt->nbErrors > 0)
			ret = -1;
		else if(ret > 0)
			ret = 0; /* xmlXIncludeDoProcess can return +ve number */
		xmlXIncludeFreeContext(newctxt);
		ctxt->incTab[nr]->inc = xmlDocCopyNodeList(ctxt->doc, fallback->children);
	}
	else {
		ctxt->incTab[nr]->inc = NULL;
		ctxt->incTab[nr]->emptyFb = 1; /* flag empty callback */
	}
	return ret;
}
// 
// XInclude Processing
// 
/**
 * xmlXIncludePreProcessNode:
 * @ctxt: an XInclude context
 * @node: an XInclude node
 *
 * Implement the XInclude preprocessing, currently just adding the element
 * for further processing.
 *
 */
static void FASTCALL xmlXIncludePreProcessNode(xmlXIncludeCtxtPtr ctxt, xmlNode * P_Node) 
{
	xmlXIncludeAddNode(ctxt, P_Node);
}
/**
 * xmlXIncludeLoadNode:
 * @ctxt: an XInclude context
 * @nr: the node number
 *
 * Find and load the infoset replacement for the given node.
 *
 * Returns 0 if substitution succeeded, -1 if some processing failed
 */
static int xmlXIncludeLoadNode(xmlXIncludeCtxtPtr ctxt, int nr) 
{
	xmlNode * cur;
	xmlChar * href;
	xmlChar * parse;
	xmlChar * base;
	xmlChar * oldBase;
	xmlChar * URI;
	int xml = 1; /* default Issue 64 */
	int ret;
	if(!ctxt)
		return -1;
	if((nr < 0) || (nr >= ctxt->incNr))
		return -1;
	cur = ctxt->incTab[nr]->ref;
	if(!cur)
		return -1;
	/*
	 * read the attributes
	 */
	href = xmlXIncludeGetProp(ctxt, cur, XINCLUDE_HREF);
	if(href == NULL) {
		href = sstrdup(reinterpret_cast<const xmlChar *>("")); /* @@@@ href is now optional */
		if(href == NULL)
			return -1;
	}
	parse = xmlXIncludeGetProp(ctxt, cur, XINCLUDE_PARSE);
	if(parse != NULL) {
		if(sstreq(parse, XINCLUDE_PARSE_XML))
			xml = 1;
		else if(sstreq(parse, XINCLUDE_PARSE_TEXT))
			xml = 0;
		else {
			xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_PARSE_VALUE, "invalid value %s for 'parse'\n", parse);
			SAlloc::F(href);
			SAlloc::F(parse);
			return -1;
		}
	}

	/*
	 * compute the URI
	 */
	base = xmlNodeGetBase(ctxt->doc, cur);
	URI = xmlBuildURI(href, base ? base : ctxt->doc->URL);
	if(URI == NULL) {
		/*
		 * Some escaping may be needed
		 */
		xmlChar * escbase = xmlURIEscape(base);
		xmlChar * eschref = xmlURIEscape(href);
		URI = xmlBuildURI(eschref, escbase);
		SAlloc::F(escbase);
		SAlloc::F(eschref);
	}
	if(URI == NULL) {
		xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_HREF_URI, "failed build URL\n", 0);
		SAlloc::F(parse);
		SAlloc::F(href);
		SAlloc::F(base);
		return -1;
	}
#ifdef DEBUG_XINCLUDE
	xmlGenericError(0, "parse: %s\n", xml ? "xml" : "text");
	xmlGenericError(0, "URI: %s\n", URI);
#endif
	/*
	 * Save the base for this include (saving the current one)
	 */
	oldBase = ctxt->base;
	ctxt->base = base;
	if(xml) {
		ret = xmlXIncludeLoadDoc(ctxt, URI, nr);
		/* xmlXIncludeGetFragment(ctxt, cur, URI); */
	}
	else {
		ret = xmlXIncludeLoadTxt(ctxt, URI, nr);
	}
	/*
	 * Restore the original base before checking for fallback
	 */
	ctxt->base = oldBase;
	if(ret < 0) {
		xmlNode * children;
		/*
		 * Time to try a fallback if availble
		 */
#ifdef DEBUG_XINCLUDE
		xmlGenericError(0, "error looking for fallback\n");
#endif
		children = cur->children;
		while(children) {
			if((children->type == XML_ELEMENT_NODE) && children->ns &&
			    (sstreq(children->name, XINCLUDE_FALLBACK)) && ((sstreq(children->ns->href, XINCLUDE_NS)) || (sstreq(children->ns->href, XINCLUDE_OLD_NS)))) {
				ret = xmlXIncludeLoadFallback(ctxt, children, nr);
				if(!ret)
					break;
			}
			children = children->next;
		}
	}
	if(ret < 0) {
		xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_NO_FALLBACK, "could not load %s, and no fallback was found\n", URI);
	}
	/*
	 * Cleanup
	 */
	SAlloc::F(URI);
	SAlloc::F(parse);
	SAlloc::F(href);
	SAlloc::F(base);
	return 0;
}

/**
 * xmlXIncludeIncludeNode:
 * @ctxt: an XInclude context
 * @nr: the node number
 *
 * Inplement the infoset replacement for the given node
 *
 * Returns 0 if substitution succeeded, -1 if some processing failed
 */
static int xmlXIncludeIncludeNode(xmlXIncludeCtxtPtr ctxt, int nr) 
{
	xmlNode * cur;
	xmlNode * end;
	xmlNode * list;
	xmlNode * tmp;
	if(!ctxt)
		return -1;
	if((nr < 0) || (nr >= ctxt->incNr))
		return -1;
	cur = ctxt->incTab[nr]->ref;
	if(!cur || (cur->type == XML_NAMESPACE_DECL))
		return -1;
	/*
	 * If we stored an XPointer a late computation may be needed
	 */
	if((ctxt->incTab[nr]->inc == NULL) && (ctxt->incTab[nr]->xptr != NULL)) {
		ctxt->incTab[nr]->inc = xmlXIncludeCopyXPointer(ctxt, ctxt->doc, ctxt->doc, ctxt->incTab[nr]->xptr);
		xmlXPathFreeObject(ctxt->incTab[nr]->xptr);
		ctxt->incTab[nr]->xptr = NULL;
	}
	list = ctxt->incTab[nr]->inc;
	ctxt->incTab[nr]->inc = NULL;
	/*
	 * Check against the risk of generating a multi-rooted document
	 */
	if(cur->P_ParentNode && cur->P_ParentNode->type != XML_ELEMENT_NODE) {
		int nb_elem = 0;
		tmp = list;
		while(tmp) {
			if(tmp->type == XML_ELEMENT_NODE)
				nb_elem++;
			tmp = tmp->next;
		}
		if(nb_elem > 1) {
			xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_MULTIPLE_ROOT, "XInclude error: would result in multiple root nodes\n", 0);
			return -1;
		}
	}
	if(ctxt->parseFlags & XML_PARSE_NOXINCNODE) {
		// 
		// Add the list of nodes
		// 
		while(list) {
			end = list;
			list = list->next;
			xmlAddPrevSibling(cur, end);
		}
		xmlUnlinkNode(cur);
		xmlFreeNode(cur);
	}
	else {
		// 
		// Change the current node as an XInclude start one, and add an XInclude end one
		// 
		cur->type = XML_XINCLUDE_START;
		end = xmlNewDocNode(cur->doc, cur->ns, cur->name, 0);
		if(end == NULL) {
			xmlXIncludeErr(ctxt, ctxt->incTab[nr]->ref, XML_XINCLUDE_BUILD_FAILED, "failed to build node\n", 0);
			return -1;
		}
		end->type = XML_XINCLUDE_END;
		xmlAddNextSibling(cur, end);
		/*
		 * Add the list of nodes
		 */
		while(list) {
			cur = list;
			list = list->next;
			xmlAddPrevSibling(end, cur);
		}
	}
	return 0;
}
/**
 * xmlXIncludeTestNode:
 * @ctxt: the XInclude processing context
 * @node: an XInclude node
 *
 * test if the node is an XInclude node
 *
 * Returns 1 true, 0 otherwise
 */
static int FASTCALL xmlXIncludeTestNode(xmlXIncludeCtxtPtr ctxt, xmlNode * P_Node) 
{
	if(!P_Node)
		return 0;
	if(P_Node->type != XML_ELEMENT_NODE)
		return 0;
	if(P_Node->ns == NULL)
		return 0;
	if((sstreq(P_Node->ns->href, XINCLUDE_NS)) || (sstreq(P_Node->ns->href, XINCLUDE_OLD_NS))) {
		if(sstreq(P_Node->ns->href, XINCLUDE_OLD_NS)) {
			if(ctxt->legacy == 0) {
#if 0 /* wait for the XML Core Working Group to get something stable ! */
				xmlXIncludeWarn(ctxt, node, XML_XINCLUDE_DEPRECATED_NS, "Deprecated XInclude namespace found, use %s", XINCLUDE_NS);
#endif
				ctxt->legacy = 1;
			}
		}
		if(sstreq(P_Node->name, XINCLUDE_NODE)) {
			xmlNode * child = P_Node->children;
			int nb_fallback = 0;
			while(child) {
				if((child->type == XML_ELEMENT_NODE) && child->ns && ((sstreq(child->ns->href, XINCLUDE_NS)) || (sstreq(child->ns->href, XINCLUDE_OLD_NS)))) {
					if(sstreq(child->name, XINCLUDE_NODE)) {
						xmlXIncludeErr(ctxt, P_Node, XML_XINCLUDE_INCLUDE_IN_INCLUDE, "%s has an 'include' child\n", XINCLUDE_NODE);
						return 0;
					}
					if(sstreq(child->name, XINCLUDE_FALLBACK)) {
						nb_fallback++;
					}
				}
				child = child->next;
			}
			if(nb_fallback > 1) {
				xmlXIncludeErr(ctxt, P_Node, XML_XINCLUDE_FALLBACKS_IN_INCLUDE, "%s has multiple fallback children\n", XINCLUDE_NODE);
				return 0;
			}
			return 1;
		}
		if(sstreq(P_Node->name, XINCLUDE_FALLBACK)) {
			if(!P_Node->P_ParentNode || (P_Node->P_ParentNode->type != XML_ELEMENT_NODE) ||
			    !P_Node->P_ParentNode->ns || ((!sstreq(P_Node->P_ParentNode->ns->href, XINCLUDE_NS)) &&
				    (!sstreq(P_Node->P_ParentNode->ns->href, XINCLUDE_OLD_NS))) ||
			    (!sstreq(P_Node->P_ParentNode->name, XINCLUDE_NODE))) {
				xmlXIncludeErr(ctxt, P_Node, XML_XINCLUDE_FALLBACK_NOT_IN_INCLUDE, "%s is not the child of an 'include'\n",XINCLUDE_FALLBACK);
			}
		}
	}
	return 0;
}
/**
 * xmlXIncludeDoProcess:
 * @ctxt: the XInclude processing context
 * @doc: an XML document
 * @tree: the top of the tree to process
 *
 * Implement the XInclude substitution on the XML document @doc
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *  or the number of substitutions done.
 */
static int xmlXIncludeDoProcess(xmlXIncludeCtxtPtr ctxt, xmlDoc * doc, xmlNode * tree)
{
	xmlNode * cur;
	int ret = 0;
	int i, start;
	if(!doc || (tree == NULL) || (tree->type == XML_NAMESPACE_DECL))
		return -1;
	if(!ctxt)
		return -1;
	if(doc->URL != NULL) {
		ret = xmlXIncludeURLPush(ctxt, doc->URL);
		if(ret < 0)
			return -1;
	}
	start = ctxt->incNr;
	/*
	 * First phase: lookup the elements in the document
	 */
	cur = tree;
	if(xmlXIncludeTestNode(ctxt, cur) == 1)
		xmlXIncludePreProcessNode(ctxt, cur);
	while(cur && cur != tree->P_ParentNode) {
		/* @todo need to work on entities -> stack */
		if(cur->children && !oneof3(cur->children->type, XML_ENTITY_DECL, XML_XINCLUDE_START, XML_XINCLUDE_END)) {
			cur = cur->children;
			if(xmlXIncludeTestNode(ctxt, cur))
				xmlXIncludePreProcessNode(ctxt, cur);
		}
		else if(cur->next) {
			cur = cur->next;
			if(xmlXIncludeTestNode(ctxt, cur))
				xmlXIncludePreProcessNode(ctxt, cur);
		}
		else {
			if(cur == tree)
				break;
			do {
				cur = cur->P_ParentNode;
				if(!cur || cur == tree->P_ParentNode)
					break; /* do */
				if(cur->next) {
					cur = cur->next;
					if(xmlXIncludeTestNode(ctxt, cur))
						xmlXIncludePreProcessNode(ctxt, cur);
					break; /* do */
				}
			} while(cur);
		}
	}
	/*
	 * Second Phase : collect the infosets fragments
	 */
	for(i = start; i < ctxt->incNr; i++) {
		xmlXIncludeLoadNode(ctxt, i);
		ret++;
	}
	/*
	 * Third phase: extend the original document infoset.
	 *
	 * Originally we bypassed the inclusion if there were any errors
	 * encountered on any of the XIncludes.  A bug was raised (bug
	 * 132588) requesting that we output the XIncludes without error,
	 * so the check for inc!=NULL || xptr!=NULL was put in.  This may
	 * give some other problems in the future, but for now it seems to
	 * work ok.
	 *
	 */
	for(i = ctxt->incBase; i < ctxt->incNr; i++) {
		if((ctxt->incTab[i]->inc != NULL) || (ctxt->incTab[i]->xptr != NULL) || (ctxt->incTab[i]->emptyFb != 0))    /* (empty fallback) */
			xmlXIncludeIncludeNode(ctxt, i);
	}
	if(doc->URL != NULL)
		xmlXIncludeURLPop(ctxt);
	return ret;
}
/**
 * xmlXIncludeSetFlags:
 * @ctxt:  an XInclude processing context
 * @flags: a set of xmlParserOption used for parsing XML includes
 *
 * Set the flags used for further processing of XML resources.
 *
 * Returns 0 in case of success and -1 in case of error.
 */
int xmlXIncludeSetFlags(xmlXIncludeCtxtPtr ctxt, int flags)
{
	if(!ctxt)
		return -1;
	else {
		ctxt->parseFlags = flags;
		return 0;
	}
}
/**
 * xmlXIncludeProcessTreeFlagsData:
 * @tree: an XML node
 * @flags: a set of xmlParserOption used for parsing XML includes
 * @data: application data that will be passed to the parser context
 *   in the _private field of the parser context(s)
 *
 * Implement the XInclude substitution on the XML node @tree
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *  or the number of substitutions done.
 */
int xmlXIncludeProcessTreeFlagsData(xmlNode * tree, int flags, void * data)
{
	int ret = 0;
	if(!tree || tree->type == XML_NAMESPACE_DECL || !tree->doc)
		ret = -1;
	else {
		xmlXIncludeCtxtPtr ctxt = xmlXIncludeNewContext(tree->doc);
		if(!ctxt)
			ret = -1;
		else {
			ctxt->_private = data;
			ctxt->base = sstrdup((xmlChar *)tree->doc->URL);
			xmlXIncludeSetFlags(ctxt, flags);
			ret = xmlXIncludeDoProcess(ctxt, tree->doc, tree);
			if((ret >= 0) && (ctxt->nbErrors > 0))
				ret = -1;
			xmlXIncludeFreeContext(ctxt);
		}
	}
	return ret;
}
/**
 * xmlXIncludeProcessFlagsData:
 * @doc: an XML document
 * @flags: a set of xmlParserOption used for parsing XML includes
 * @data: application data that will be passed to the parser context
 *   in the _private field of the parser context(s)
 *
 * Implement the XInclude substitution on the XML document @doc
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *  or the number of substitutions done.
 */
int xmlXIncludeProcessFlagsData(xmlDoc * doc, int flags, void * data)
{
	if(!doc)
		return -1;
	xmlNode * tree = xmlDocGetRootElement(doc);
	if(tree == NULL)
		return -1;
	return xmlXIncludeProcessTreeFlagsData(tree, flags, data);
}
/**
 * xmlXIncludeProcessFlags:
 * @doc: an XML document
 * @flags: a set of xmlParserOption used for parsing XML includes
 *
 * Implement the XInclude substitution on the XML document @doc
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *  or the number of substitutions done.
 */
int xmlXIncludeProcessFlags(xmlDoc * doc, int flags)
{
	return xmlXIncludeProcessFlagsData(doc, flags, 0);
}
/**
 * xmlXIncludeProcess:
 * @doc: an XML document
 *
 * Implement the XInclude substitution on the XML document @doc
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *  or the number of substitutions done.
 */
int xmlXIncludeProcess(xmlDoc * doc)
{
	return (xmlXIncludeProcessFlags(doc, 0));
}

/**
 * xmlXIncludeProcessTreeFlags:
 * @tree: a node in an XML document
 * @flags: a set of xmlParserOption used for parsing XML includes
 *
 * Implement the XInclude substitution for the given subtree
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *  or the number of substitutions done.
 */
int xmlXIncludeProcessTreeFlags(xmlNode * tree, int flags)
{
	xmlXIncludeCtxtPtr ctxt;
	int ret = 0;
	if((tree == NULL) || (tree->type == XML_NAMESPACE_DECL) || (tree->doc == NULL))
		return -1;
	ctxt = xmlXIncludeNewContext(tree->doc);
	if(!ctxt)
		return -1;
	ctxt->base = xmlNodeGetBase(tree->doc, tree);
	xmlXIncludeSetFlags(ctxt, flags);
	ret = xmlXIncludeDoProcess(ctxt, tree->doc, tree);
	if((ret >= 0) && (ctxt->nbErrors > 0))
		ret = -1;
	xmlXIncludeFreeContext(ctxt);
	return ret;
}
/**
 * xmlXIncludeProcessTree:
 * @tree: a node in an XML document
 *
 * Implement the XInclude substitution for the given subtree
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *  or the number of substitutions done.
 */
int xmlXIncludeProcessTree(xmlNode * tree)
{
	return (xmlXIncludeProcessTreeFlags(tree, 0));
}
/**
 * xmlXIncludeProcessNode:
 * @ctxt: an existing XInclude context
 * @node: a node in an XML document
 *
 * Implement the XInclude substitution for the given subtree reusing
 * the informations and data coming from the given context.
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *  or the number of substitutions done.
 */
int xmlXIncludeProcessNode(xmlXIncludeCtxtPtr ctxt, xmlNode * P_Node)
{
	int ret = 0;
	if(!P_Node || (P_Node->type == XML_NAMESPACE_DECL) || (P_Node->doc == NULL) || (ctxt == NULL))
		return -1;
	ret = xmlXIncludeDoProcess(ctxt, P_Node->doc, P_Node);
	if((ret >= 0) && (ctxt->nbErrors > 0))
		ret = -1;
	return ret;
}

#else /* !LIBXML_XINCLUDE_ENABLED */
#endif
#define bottom_xinclude
