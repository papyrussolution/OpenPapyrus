/*
 * Summary: API to handle XML Pointers
 * Description: API to handle XML Pointers
 * Base implementation was made accordingly to
 * W3C Candidate Recommendation 7 June 2000
 * http://www.w3.org/TR/2000/CR-xptr-20000607
 *
 * Added support for the element() scheme described in:
 * W3C Proposed Recommendation 13 November 2002
 * http://www.w3.org/TR/2002/PR-xptr-element-20021113/
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_XPTR_H__
#define __XML_XPTR_H__

//#include <libxml/xmlversion.h>

#ifdef LIBXML_XPTR_ENABLED

//#include <libxml/tree.h>
//#include <libxml/xpath.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A Location Set
 */
typedef struct _xmlLocationSet xmlLocationSet;
typedef xmlLocationSet * xmlLocationSetPtr;
struct _xmlLocationSet {
	int locNr;            /* number of locations in the set */
	int locMax;           /* size of the array as allocated */
	xmlXPathObjectPtr * locTab; /* array of locations */
};

/*
 * Handling of location sets.
 */

XMLPUBFUN xmlLocationSetPtr XMLCALL xmlXPtrLocationSetCreate(xmlXPathObjectPtr val);
XMLPUBFUN void XMLCALL xmlXPtrFreeLocationSet(xmlLocationSetPtr obj);
XMLPUBFUN xmlLocationSetPtr XMLCALL xmlXPtrLocationSetMerge(xmlLocationSetPtr val1, xmlLocationSetPtr val2);
XMLPUBFUN xmlXPathObjectPtr XMLCALL xmlXPtrNewRange(xmlNode * start, int startindex, xmlNode * end, int endindex);
XMLPUBFUN xmlXPathObjectPtr XMLCALL xmlXPtrNewRangePoints(xmlXPathObjectPtr start, xmlXPathObjectPtr end);
XMLPUBFUN xmlXPathObjectPtr XMLCALL xmlXPtrNewRangeNodePoint(xmlNode * start, xmlXPathObjectPtr end);
XMLPUBFUN xmlXPathObjectPtr XMLCALL xmlXPtrNewRangePointNode(xmlXPathObjectPtr start, xmlNode * end);
XMLPUBFUN xmlXPathObjectPtr XMLCALL xmlXPtrNewRangeNodes(xmlNode * start, xmlNode * end);
XMLPUBFUN xmlXPathObjectPtr XMLCALL xmlXPtrNewLocationSetNodes(xmlNode * start, xmlNode * end);
XMLPUBFUN xmlXPathObjectPtr XMLCALL xmlXPtrNewLocationSetNodeSet(xmlNodeSetPtr set);
XMLPUBFUN xmlXPathObjectPtr XMLCALL xmlXPtrNewRangeNodeObject(xmlNode * start, xmlXPathObjectPtr end);
XMLPUBFUN xmlXPathObjectPtr XMLCALL xmlXPtrNewCollapsedRange(xmlNode * start);
XMLPUBFUN void XMLCALL xmlXPtrLocationSetAdd(xmlLocationSetPtr cur, xmlXPathObjectPtr val);
XMLPUBFUN xmlXPathObjectPtr XMLCALL xmlXPtrWrapLocationSet(xmlLocationSetPtr val);
XMLPUBFUN void XMLCALL xmlXPtrLocationSetDel(xmlLocationSetPtr cur, xmlXPathObjectPtr val);
XMLPUBFUN void XMLCALL xmlXPtrLocationSetRemove(xmlLocationSetPtr cur, int val);
/*
 * Functions.
 */
XMLPUBFUN xmlXPathContextPtr XMLCALL xmlXPtrNewContext(xmlDoc * doc, xmlNode * here, xmlNode * origin);
XMLPUBFUN xmlXPathObjectPtr XMLCALL xmlXPtrEval(const xmlChar * str, xmlXPathContextPtr ctx);
XMLPUBFUN void XMLCALL xmlXPtrRangeToFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN xmlNode * XMLCALL xmlXPtrBuildNodeList(xmlXPathObjectPtr obj);
XMLPUBFUN void XMLCALL xmlXPtrEvalRangePredicate(xmlXPathParserContextPtr ctxt);
#ifdef __cplusplus
}
#endif

#endif /* LIBXML_XPTR_ENABLED */
#endif /* __XML_XPTR_H__ */
