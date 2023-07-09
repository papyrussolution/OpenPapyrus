/*
 * Summary: interfaces for tree manipulation
 * Description: this module describes the structures found in an tree resulting
 *   from an XML or HTML parsing, as well as the API provided for
 *   various processing on that tree
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */
#ifndef __XML_TREE_H__
#define __XML_TREE_H__

struct xmlDict;
struct xmlDoc;
struct xmlNode;
struct xmlDtd;
struct xmlRegexp;
struct xmlParserInputBuffer;
struct xmlOutputBuffer;
struct xmlParserInput;
struct xmlParserCtxt;
struct xmlSAXLocator;
struct xmlSAXHandler;
struct xmlEntity;

//#ifdef __cplusplus
//extern "C" {
//#endif
// 
// BASE_BUFFER_SIZE:
// 
// default buffer size 4000.
// 
#define BASE_BUFFER_SIZE 4096
/**
 * LIBXML_NAMESPACE_DICT:
 *
 * Defines experimental behaviour:
 * 1) xmlNs gets an additional field @context (a xmlDoc)
 * 2) when creating a tree, xmlNs->href is stored in the dict of xmlDoc.
 */
/* #define LIBXML_NAMESPACE_DICT */

/**
 * xmlBufferAllocationScheme:
 *
 * A buffer allocation scheme can be defined to either match exactly the
 * need or double it's allocated size each time it is found too small.
 */
typedef enum {
	XML_BUFFER_ALLOC_DOUBLEIT, /* double each time one need to grow */
	XML_BUFFER_ALLOC_EXACT, /* grow only to the minimal size */
	XML_BUFFER_ALLOC_IMMUTABLE, /* immutable buffer */
	XML_BUFFER_ALLOC_IO,    /* special allocation scheme used for I/O */
	XML_BUFFER_ALLOC_HYBRID /* exact up to a threshold, and doubleit thereafter */
} xmlBufferAllocationScheme;

/**
 * xmlBuffer:
 *
 * A buffer structure, this old construct is limited to 2GB and is being deprecated, use API with xmlBuf instead
 */
//typedef struct _xmlBuffer xmlBuffer;
//struct xmlBuffer;
//typedef xmlBuffer * xmlBufferPtr;

struct xmlBuffer {
	xmlChar * content; // The buffer content UTF8 
	uint   use;       // The buffer size used 
	uint   size;      // The buffer size 
	xmlBufferAllocationScheme alloc; // The realloc method 
	xmlChar * contentIO; // in IO mode we may have a different base 
};
/**
 * xmlBuf:
 *
 * A buffer structure, new one, the actual structure internals are not public
 */
struct xmlBuf;
/**
 * xmlBufPtr:
 *
 * A pointer to a buffer structure, the actual structure internals are not
 * public
 */
typedef xmlBuf * xmlBufPtr;
/*
 * A few public routines for xmlBuf. As those are expected to be used
 * mostly internally the bulk of the routines are internal in buf.h
 */
XMLPUBFUN xmlChar * FASTCALL xmlBufContent(const xmlBuf * buf);
XMLPUBFUN xmlChar * FASTCALL xmlBufEnd(xmlBuf * buf);
XMLPUBFUN size_t FASTCALL xmlBufUse(xmlBuf * buf);
XMLPUBFUN size_t FASTCALL xmlBufShrink(xmlBuf * buf, size_t len);
/*
 * LIBXML2_NEW_BUFFER:
 *
 * Macro used to express that the API use the new buffers for
 * xmlParserInputBuffer and xmlOutputBuffer. The change was
 * introduced in 2.9.0.
 */
#define LIBXML2_NEW_BUFFER
/**
 * XML_XML_NAMESPACE:
 *
 * This is the namespace for the special xml: prefix predefined in the XML Namespace specification.
 */
#define XML_XML_NAMESPACE (const xmlChar *)"http://www.w3.org/XML/1998/namespace"
#define XML_XML_ID (const xmlChar *)"xml:id" // This is the name for the special xml:id attribute
/*
 * The different element types carried by an XML tree.
 *
 * NOTE: This is synchronized with DOM Level1 values
 *  See http://www.w3.org/TR/REC-DOM-Level-1/
 *
 * Actually this had diverged a bit, and now XML_DOCUMENT_TYPE_NODE should be deprecated to use an XML_DTD_NODE.
 */
typedef enum {
	XML_ELEMENT_NODE =           1,
	XML_ATTRIBUTE_NODE =         2,
	XML_TEXT_NODE =              3,
	XML_CDATA_SECTION_NODE =     4,
	XML_ENTITY_REF_NODE =        5,
	XML_ENTITY_NODE =            6,
	XML_PI_NODE =                7,
	XML_COMMENT_NODE =           8,
	XML_DOCUMENT_NODE =          9,
	XML_DOCUMENT_TYPE_NODE =     10,
	XML_DOCUMENT_FRAG_NODE =     11,
	XML_NOTATION_NODE =          12,
	XML_HTML_DOCUMENT_NODE =     13,
	XML_DTD_NODE =               14,
	XML_ELEMENT_DECL =           15,
	XML_ATTRIBUTE_DECL =         16,
	XML_ENTITY_DECL =            17,
	XML_NAMESPACE_DECL =         18,
	XML_XINCLUDE_START =         19,
	XML_XINCLUDE_END =           20
#ifdef LIBXML_DOCB_ENABLED
	, XML_DOCB_DOCUMENT_NODE =     21
#endif
} xmlElementType;
//
// Descr: A DTD Notation definition.
//
struct xmlNotation {
	const xmlChar * name; /* Notation name */
	const xmlChar * PublicID; /* Public identifier, if any */
	const xmlChar * SystemID; /* System identifier, if any */
};
//typedef xmlNotation * xmlNotationPtr;
/**
 * xmlAttributeType:
 *
 * A DTD Attribute type definition.
 */
typedef enum {
	XML_ATTRIBUTE_UNDEF = 0, // @sobolev
	XML_ATTRIBUTE_CDATA = 1,
	XML_ATTRIBUTE_ID,
	XML_ATTRIBUTE_IDREF,
	XML_ATTRIBUTE_IDREFS,
	XML_ATTRIBUTE_ENTITY,
	XML_ATTRIBUTE_ENTITIES,
	XML_ATTRIBUTE_NMTOKEN,
	XML_ATTRIBUTE_NMTOKENS,
	XML_ATTRIBUTE_ENUMERATION,
	XML_ATTRIBUTE_NOTATION
} xmlAttributeType;
// 
// Descr: A DTD Attribute default definition.
// 
typedef enum {
	XML_ATTRIBUTE_NONE = 1,
	XML_ATTRIBUTE_REQUIRED,
	XML_ATTRIBUTE_IMPLIED,
	XML_ATTRIBUTE_FIXED
} xmlAttributeDefault;
// 
// Descr: List structure used when there is an enumeration in DTDs.
// 
struct xmlEnumeration {
	struct xmlEnumeration * next; /* next one */
	const xmlChar * name; /* Enumeration name */
};
//typedef xmlEnumeration * xmlEnumerationPtr;

struct XmlNodeBase { // @construction
	void * _private;      // application data 
	xmlElementType type;  // XML_DTD_NODE, must be second!
	const xmlChar * name; // Name of the DTD 
	xmlNode * children;   // the value of the property link 
	xmlNode * last;       // last child link 
	xmlNode * P_ParentNode; // child->parent link 
	xmlNode * next;       // next sibling link 
	xmlNode * prev;       // previous sibling link 
	xmlDoc  * doc;        // the containing document
};
// 
// Descr: An XML DTD, as defined by <!DOCTYPE ... There is actually one for
//   the internal subset and for the external subset.
// 
//typedef struct _xmlDtd xmlDtd;
struct xmlDtd : public XmlNodeBase {
	//void * _private;      // application data
	//xmlElementType type;  // XML_DTD_NODE, must be second !
	//const xmlChar * name; // Name of the DTD 
	//xmlNode * children; // the value of the property link
	//xmlNode * last;   // last child link
	//xmlDoc  * parent; // child->parent link // @sobolev @note здесь (xmlDoc * parent) в результате наслендования от XmlNodeBase превращается в (xmlNode * parent)
	//xmlNode * next; // next sibling link 
	//xmlNode * prev; // previous sibling link 
	//xmlDoc  * doc;  // the containing document
	// End of common part 
	void * notations; /* Hash table for notations if any */
	void * elements; /* Hash table for elements if any */
	void * attributes; /* Hash table for attributes if any */
	void * entities; /* Hash table for entities if any */
	const xmlChar * ExternalID; /* External identifier for PUBLIC DTD */
	const xmlChar * SystemID; /* URI for a SYSTEM or PUBLIC DTD */
	void * pentities; /* Hash table for param entities if any */
};

//typedef xmlDtd * xmlDtdPtr;
// 
// Descr: An Attribute declaration in a DTD.
// 
struct xmlAttribute {
	void * _private; /* application data */
	xmlElementType type; /* XML_ATTRIBUTE_DECL, must be second ! */
	const xmlChar * name; /* Attribute name */
	xmlNode * children; /* NULL */
	xmlNode * last; /* NULL */
	xmlDtd  * parent; /* -> DTD */
	xmlNode * next; /* next sibling link  */
	xmlNode * prev; /* previous sibling link  */
	xmlDoc  * doc; /* the containing document */
	struct xmlAttribute * nexth; /* next in hash table */
	xmlAttributeType atype; /* The attribute type */
	xmlAttributeDefault def; /* the default */
	const xmlChar  * defaultValue; /* or the default value */
	xmlEnumeration * tree; /* or the enumeration tree if any */
	const xmlChar * prefix; /* the namespace prefix if any */
	const xmlChar * elem; /* Element holding the attribute */
};
// 
// Descr: Possible definitions of element content types.
// 
typedef enum {
	XML_ELEMENT_CONTENT_PCDATA = 1,
	XML_ELEMENT_CONTENT_ELEMENT,
	XML_ELEMENT_CONTENT_SEQ,
	XML_ELEMENT_CONTENT_OR
} xmlElementContentType;
// 
// Descr: Possible definitions of element content occurrences.
// 
typedef enum {
	XML_ELEMENT_CONTENT_ONCE = 1,
	XML_ELEMENT_CONTENT_OPT,
	XML_ELEMENT_CONTENT_MULT,
	XML_ELEMENT_CONTENT_PLUS
} xmlElementContentOccur;
// 
// Descr: An XML Element content as stored after parsing an element definition in a DTD.
// 
struct xmlElementContent {
	xmlElementContentType type; /* PCDATA, ELEMENT, SEQ or OR */
	xmlElementContentOccur ocur; /* ONCE, OPT, MULT or PLUS */
	const xmlChar * name; /* Element name */
	xmlElementContent * c1; /* first child */
	xmlElementContent * c2; /* second child */
	xmlElementContent * parent; /* parent */
	const xmlChar * prefix; /* Namespace prefix */
};
//typedef xmlElementContent * xmlElementContentPtr;
//
// Descr: The different possibilities for an element content type.
//
typedef enum {
	XML_ELEMENT_TYPE_UNDEFINED = 0,
	XML_ELEMENT_TYPE_EMPTY = 1,
	XML_ELEMENT_TYPE_ANY,
	XML_ELEMENT_TYPE_MIXED,
	XML_ELEMENT_TYPE_ELEMENT
} xmlElementTypeVal;

//#ifdef __cplusplus
//}
//#endif
//#ifdef __cplusplus
//extern "C" {
//#endif
/**
 * xmlElement:
 *
 * An XML Element declaration from a DTD.
 */
//typedef struct _xmlElement xmlElement;
struct xmlElement {
	void * _private; /* application data */
	xmlElementType type; /* XML_ELEMENT_DECL, must be second ! */
	const xmlChar * name; /* Element name */
	xmlNode * children; /* NULL */
	xmlNode * last; /* NULL */
	xmlDtd  * parent; /* -> DTD */
	xmlNode * next; /* next sibling link  */
	xmlNode * prev; /* previous sibling link  */
	xmlDoc  * doc; /* the containing document */
	xmlElementTypeVal etype; /* The type */
	xmlElementContent * content; /* the allowed element content */
	xmlAttribute  * attributes; /* List of the declared attributes */
	const xmlChar * prefix; /* the namespace prefix if any */
#ifdef LIBXML_REGEXP_ENABLED
	xmlRegexp * contModel; /* the validating regexp */
#else
	void * contModel;
#endif
};
//typedef xmlElement * xmlElementPtr;
/**
 * XML_LOCAL_NAMESPACE:
 *
 * A namespace declaration node.
 */
#define XML_LOCAL_NAMESPACE XML_NAMESPACE_DECL
typedef xmlElementType xmlNsType;
/**
 * xmlNs:
 *
 * An XML namespace.
 * Note that prefix == NULL is valid, it defines the default namespace
 * within the subtree (until overridden).
 *
 * xmlNsType is unified with xmlElementType.
 */
//typedef struct _xmlNs xmlNs;
//struct xmlNs;
//typedef xmlNs * xmlNsPtr;

struct xmlNs {
	xmlNs * next; /* next Ns link for this node  */
	xmlNsType type; /* global or local */
	const xmlChar * href; /* URL for the namespace */
	const xmlChar * prefix; /* prefix for the namespace */
	void * _private; /* application data */
	xmlDoc * context; /* normally an xmlDoc */
};
//
// Descr: An attribute on an XML node.
// 
//typedef struct _xmlAttr xmlAttr;
struct xmlAttr {
	void * _private; /* application data */
	xmlElementType type; /* XML_ATTRIBUTE_NODE, must be second ! */
	const xmlChar * name; /* the name of the property */
	xmlNode * children; /* the value of the property */
	xmlNode * last; /* NULL */
	xmlNode * parent; /* child->parent link */
	xmlAttr * next; /* next sibling link  */
	xmlAttr * prev; /* previous sibling link  */
	xmlDoc  * doc; /* the containing document */
	xmlNs * ns; /* pointer to the associated namespace */
	xmlAttributeType atype; /* the attribute type if validating */
	void * psvi; /* for type/PSVI informations */
};
//typedef xmlAttr * xmlAttrPtr;
// 
// Descr: An XML ID instance.
// 
struct xmlID {
	xmlID * next; /* next ID */
	const  xmlChar * value; /* The ID name */
	xmlAttr * attr; /* The attribute holding it */
	const xmlChar * name; /* The attribute if attr is not available */
	int    lineno; /* The line number if attr is not available */
	xmlDoc * doc; /* The document holding the ID */
};

//typedef xmlID * xmlIDPtr;
// 
// Descr: An XML IDREF instance.
// 
struct xmlRef {
	xmlRef * next; /* next Ref */
	const xmlChar * value; /* The Ref name */
	xmlAttr * attr; /* The attribute holding it */
	const xmlChar * name; /* The attribute if attr is not available */
	int lineno; /* The line number if attr is not available */
};

//typedef xmlRef * xmlRefPtr;
// 
// Descr: A node in an XML tree.
// 
struct xmlNode : public XmlNodeBase {
	//void * _private; // application data 
	//xmlElementType type; // type number, must be second ! 
	//const xmlChar * name; // the name of the node, or the entity 
	//xmlNode * children; // parent->childs link 
	//xmlNode * last;   // last child link 
	//xmlNode * parent; // child->parent link 
	//xmlNode * next; // next sibling link  
	//xmlNode * prev; // previous sibling link  
	//xmlDoc  * doc;  // the containing document 
	// End of common part 
	xmlNs   * ns;      // pointer to the associated namespace 
	xmlChar * content; // the content 
	xmlAttr * properties; // properties list 
	xmlNs * nsDef; /* namespace definitions on this node */
	void * psvi; /* for type/PSVI informations */
	ushort line; /* line number */
	ushort extra; /* extra data for XPath/XSLT */
};

// @v10.5.8 typedef xmlNode * xmlNodePtr;
/**
 * XML_GET_CONTENT:
 *
 * Macro to extract the content pointer of a node.
 */
#define XML_GET_CONTENT(n) ((n)->type == XML_ELEMENT_NODE ? NULL : (n)->content)
/**
 * XML_GET_LINE:
 *
 * Macro to extract the line number of an element node.
 */
#define XML_GET_LINE(n) (xmlGetLineNo(n))
/**
 * xmlDocProperty
 *
 * Set of properties of the document as found by the parser
 * Some of them are linked to similary named xmlParserOption
 */
typedef enum {
	XML_DOC_WELLFORMED  = 1<<0, /* document is XML well formed */
	XML_DOC_NSVALID     = 1<<1, /* document is Namespace valid */
	XML_DOC_OLD10       = 1<<2, /* parsed with old XML-1.0 parser */
	XML_DOC_DTDVALID    = 1<<3, /* DTD validation was successful */
	XML_DOC_XINCLUDE    = 1<<4, /* XInclude substitution was done */
	XML_DOC_USERBUILT   = 1<<5, /* Document was built using the API and not by parsing an instance */
	XML_DOC_INTERNAL    = 1<<6, /* built for internal processing */
	XML_DOC_HTML        = 1<<7 /* parsed or built HTML document */
} xmlDocProperties;
// 
// Descr: An XML document
// 
struct xmlDoc : public XmlNodeBase {
	//void * _private;     // application data 
	//xmlElementType type; // XML_DOCUMENT_NODE, must be second!
	//char * name;         // name/filename/URI of the document
	//xmlNode * children;  // the document tree 
	//xmlNode * last;      // last child link
	//xmlNode * parent;    // child->parent link
	//xmlNode * next;      // next sibling link
	//xmlNode * prev;      // previous sibling link
	//xmlDoc  * doc;       // autoreference to itself
	// End of common part 
	int compression; /* level of zlib compression */
	int standalone; // standalone document (no external refs) 1 if standalone="yes", 0 if standalone="no", 
		// -1 if there is no XML declaration, -2 if there is an XML declaration, but no standalone attribute was specified 
	xmlDtd * intSubset; /* the document internal subset */
	xmlDtd * extSubset; /* the document external subset */
	xmlNs  * oldNs; /* Global namespace, the old way */
	const  xmlChar  * version; /* the XML version string */
	const  xmlChar  * encoding; /* external initial encoding, if any */
	void * ids; /* Hash table for ID attributes if any */
	void * refs; /* Hash table for IDREFs attributes if any */
	const  xmlChar  * URL; /* The URI for that document */
	int    charset; /* encoding of the in-memory content actually an xmlCharEncoding */
	xmlDict * dict; // dict used to allocate names or NULL 
	void * psvi; /* for type/PSVI informations */
	int    parseFlags; // set of xmlParserOption used to parse the document 
	int    properties; // set of xmlDocProperties for this document set at the end of parsing 
};

typedef xmlDoc * xmlDocPtr;
struct xmlDOMWrapCtxt;
typedef xmlDOMWrapCtxt * xmlDOMWrapCtxtPtr;
/**
 * xmlDOMWrapAcquireNsFunction:
 * @ctxt:  a DOM wrapper context
 * @node:  the context node (element or attribute)
 * @nsName:  the requested namespace name
 * @nsPrefix:  the requested namespace prefix
 *
 * A function called to acquire namespaces (xmlNs) from the wrapper.
 *
 * Returns an xmlNsPtr or NULL in case of an error.
 */
typedef xmlNs * (*xmlDOMWrapAcquireNsFunction)(xmlDOMWrapCtxtPtr ctxt, xmlNode * pNode, const xmlChar * nsName, const xmlChar * nsPrefix);
// 
// Descr: Context for DOM wrapper-operations.
// 
struct xmlDOMWrapCtxt {
	void * _private;
	int type; // The type of this context, just in case we need specialized contexts in the future.
	void * namespaceMap; // Internal namespace map used for various operations.
	xmlDOMWrapAcquireNsFunction getNsForNodeFunc; // Use this one to acquire an xmlNsPtr intended for node->ns. (Note that this is not intended for elem->nsDef).
};
// 
// Descr: Macro for compatibility naming layer with libxml1. Maps to "children."
// 
#ifndef xmlChildrenNode
    #define xmlChildrenNode children
#endif
/**
 * xmlRootNode:
 *
 * Macro for compatibility naming layer with libxml1. Maps
 * to "children".
 */
#ifndef xmlRootNode
    #define xmlRootNode children
#endif
/*
 * Variables.
 */
/*
 * Some helper functions
 */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_DEBUG_ENABLED) || \
	defined (LIBXML_HTML_ENABLED) || defined(LIBXML_SAX1_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_WRITER_ENABLED) || \
	defined(LIBXML_DOCB_ENABLED) || defined(LIBXML_LEGACY_ENABLED)
XMLPUBFUN int xmlValidateNCName(const xmlChar * value, int space);
#endif
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
	XMLPUBFUN int xmlValidateQName(const xmlChar * value, int space);
	XMLPUBFUN int xmlValidateName(const xmlChar * value, int space);
	XMLPUBFUN int xmlValidateNMToken(const xmlChar * value, int space);
#endif
XMLPUBFUN xmlChar * FASTCALL xmlBuildQName(const xmlChar * ncname, const xmlChar * prefix, xmlChar * memory, int len);
XMLPUBFUN xmlChar * FASTCALL xmlSplitQName2(const xmlChar * name, xmlChar ** prefix);
XMLPUBFUN const xmlChar * xmlSplitQName3(const xmlChar * name, int * len);
/*
 * Handling Buffers, the old ones see @xmlBuf for the new ones.
 */
XMLPUBFUN void xmlSetBufferAllocationScheme(xmlBufferAllocationScheme scheme);
XMLPUBFUN xmlBufferAllocationScheme xmlGetBufferAllocationScheme();
XMLPUBFUN xmlBuffer * xmlBufferCreate();
XMLPUBFUN xmlBuffer * xmlBufferCreateSize(size_t size);
XMLPUBFUN xmlBuffer * xmlBufferCreateStatic(void * mem, size_t size);
XMLPUBFUN int xmlBufferResize(xmlBuffer * buf, uint size);
XMLPUBFUN void FASTCALL xmlBufferFree(xmlBuffer * pBuf);
XMLPUBFUN int xmlBufferDump(FILE * file, xmlBuffer * buf);
XMLPUBFUN int FASTCALL xmlBufferAdd(xmlBuffer * buf, const xmlChar * str, int len);
XMLPUBFUN int xmlBufferAddHead(xmlBuffer * buf, const xmlChar * str, int len);
XMLPUBFUN int FASTCALL xmlBufferCat(xmlBuffer * pBuf, const xmlChar * pStr);
XMLPUBFUN int FASTCALL xmlBufferCCat(xmlBuffer * pBuf, const char * pStr);
XMLPUBFUN int xmlBufferShrink(xmlBuffer * buf, uint len);
XMLPUBFUN int xmlBufferGrow(xmlBuffer * buf, uint len);
XMLPUBFUN void xmlBufferEmpty(xmlBuffer * buf);
XMLPUBFUN const xmlChar* xmlBufferContent(const xmlBuffer * buf);
XMLPUBFUN xmlChar* xmlBufferDetach(xmlBuffer * buf);
XMLPUBFUN void xmlBufferSetAllocationScheme(xmlBuffer * buf, xmlBufferAllocationScheme scheme);
XMLPUBFUN int xmlBufferLength(const xmlBuffer * buf);
/*
 * Creating/freeing new structures.
 */
XMLPUBFUN xmlDtd * xmlCreateIntSubset(xmlDoc * doc, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID);
XMLPUBFUN xmlDtd * xmlNewDtd(xmlDoc * doc, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID);
XMLPUBFUN xmlDtd * FASTCALL xmlGetIntSubset(const xmlDoc * doc);
XMLPUBFUN void FASTCALL xmlFreeDtd(xmlDtd * pCur);
#ifdef LIBXML_LEGACY_ENABLED
XMLPUBFUN xmlNs * xmlNewGlobalNs(xmlDoc * doc, const xmlChar * href, const xmlChar * prefix);
#endif /* LIBXML_LEGACY_ENABLED */
XMLPUBFUN xmlNs * FASTCALL xmlNewNs(xmlNode * pNode, const xmlChar * href, const xmlChar * prefix);
XMLPUBFUN void FASTCALL xmlFreeNs(xmlNs * cur);
XMLPUBFUN void FASTCALL xmlFreeNsList(xmlNs * cur);
XMLPUBFUN xmlDoc * xmlNewDoc(const xmlChar * version);
XMLPUBFUN void FASTCALL xmlFreeDoc(xmlDoc * pDoc);
XMLPUBFUN xmlAttr * FASTCALL xmlNewDocProp(xmlDoc * doc, const xmlChar * name, const xmlChar * value);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
	XMLPUBFUN xmlAttr * xmlNewProp(xmlNode * pNode, const xmlChar * name, const xmlChar * value);
#endif
XMLPUBFUN xmlAttr * xmlNewNsProp(xmlNode * pNode, xmlNs * ns, const xmlChar * name, const xmlChar * value);
XMLPUBFUN xmlAttr * xmlNewNsPropEatName(xmlNode * pNode, xmlNs * ns, xmlChar * name, const xmlChar * value);
XMLPUBFUN void xmlFreePropList(xmlAttr * cur);
XMLPUBFUN void FASTCALL xmlFreeProp(xmlAttr * cur);
XMLPUBFUN xmlAttr * xmlCopyProp(xmlNode * target, xmlAttr * cur);
XMLPUBFUN xmlAttr * xmlCopyPropList(xmlNode * target, xmlAttr * cur);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlDtd * xmlCopyDtd(xmlDtd * dtd);
#endif /* LIBXML_TREE_ENABLED */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN xmlDoc * xmlCopyDoc(xmlDoc * doc, int recursive);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
/*
 * Creating new nodes.
 */
XMLPUBFUN xmlNode * FASTCALL xmlNewDocNode(xmlDoc * doc, xmlNs * ns, const xmlChar * name, const xmlChar * content);
XMLPUBFUN xmlNode * xmlNewDocNodeEatName(xmlDoc * doc, xmlNs * ns, xmlChar * name, const xmlChar * content);
XMLPUBFUN xmlNode * FASTCALL xmlNewNode(xmlNs * ns, const xmlChar * name);
XMLPUBFUN xmlNode * xmlNewNodeEatName(xmlNs * ns, xmlChar * name);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN xmlNode * xmlNewChild(xmlNode * parent, xmlNs * ns, const xmlChar * name, const xmlChar * content);
#endif
XMLPUBFUN xmlNode * xmlNewDocText(const xmlDoc * doc, const xmlChar * content);
XMLPUBFUN xmlNode * xmlNewText(const xmlChar * content);
XMLPUBFUN xmlNode * xmlNewDocPI(xmlDoc * doc, const xmlChar * name, const xmlChar * content);
XMLPUBFUN xmlNode * xmlNewPI(const xmlChar * name, const xmlChar * content);
XMLPUBFUN xmlNode * xmlNewDocTextLen(xmlDoc * doc, const xmlChar * content, int len);
XMLPUBFUN xmlNode * FASTCALL xmlNewTextLen(const xmlChar * content, int len);
XMLPUBFUN xmlNode * xmlNewDocComment(xmlDoc * doc, const xmlChar * content);
XMLPUBFUN xmlNode * xmlNewComment(const xmlChar * content);
XMLPUBFUN xmlNode * xmlNewCDataBlock(xmlDoc * doc, const xmlChar * content, int len);
XMLPUBFUN xmlNode * xmlNewCharRef(xmlDoc * doc, const xmlChar * name);
XMLPUBFUN xmlNode * xmlNewReference(const xmlDoc * doc, const xmlChar * name);
XMLPUBFUN xmlNode * xmlCopyNode(xmlNode * pNode, int recursive);
XMLPUBFUN xmlNode * xmlDocCopyNode(xmlNode * pNode, xmlDoc * doc, int recursive);
XMLPUBFUN xmlNode * xmlDocCopyNodeList(xmlDoc * doc, xmlNode * pNode);
XMLPUBFUN xmlNode * xmlCopyNodeList(xmlNode * pNode);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlNode * xmlNewTextChild(xmlNode * parent, xmlNs * ns, const xmlChar * name, const xmlChar * content);
XMLPUBFUN xmlNode * xmlNewDocRawNode(xmlDoc * doc, xmlNs * ns, const xmlChar * name, const xmlChar * content);
XMLPUBFUN xmlNode * xmlNewDocFragment(xmlDoc * doc);
#endif /* LIBXML_TREE_ENABLED */
// 
// Navigating.
// 
XMLPUBFUN long xmlGetLineNo(const xmlNode * pNode);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_DEBUG_ENABLED)
XMLPUBFUN xmlChar * xmlGetNodePath(const xmlNode * pNode);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_DEBUG_ENABLED) */
XMLPUBFUN xmlNode * FASTCALL xmlDocGetRootElement(const xmlDoc * doc);
XMLPUBFUN xmlNode * FASTCALL xmlGetLastChild(const xmlNode * parent);
XMLPUBFUN int FASTCALL xmlNodeIsText(const xmlNode * pNode);
XMLPUBFUN int FASTCALL xmlIsBlankNode(const xmlNode * pNode);
// 
// Changing the structure.
// 
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED)
	XMLPUBFUN xmlNode * xmlDocSetRootElement(xmlDoc * doc, xmlNode * root);
#endif
#ifdef LIBXML_TREE_ENABLED
	XMLPUBFUN void xmlNodeSetName(xmlNode * cur, const xmlChar * name);
#endif
XMLPUBFUN xmlNode * FASTCALL xmlAddChild(xmlNode * parent, xmlNode * cur);
XMLPUBFUN xmlNode * xmlAddChildList(xmlNode * parent, xmlNode * cur);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED)
	XMLPUBFUN xmlNode * xmlReplaceNode(xmlNode * old, xmlNode * cur);
#endif
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED)
	XMLPUBFUN xmlNode * FASTCALL xmlAddPrevSibling(xmlNode * cur, xmlNode * elem);
#endif
XMLPUBFUN xmlNode * xmlAddSibling(xmlNode * cur, xmlNode * elem);
XMLPUBFUN xmlNode * FASTCALL xmlAddNextSibling(xmlNode * cur, xmlNode * elem);
XMLPUBFUN void FASTCALL xmlUnlinkNode(xmlNode * cur);
XMLPUBFUN xmlNode * xmlTextMerge(xmlNode * first, xmlNode * second);
XMLPUBFUN int xmlTextConcat(xmlNode * pNode, const xmlChar * content, int len);
XMLPUBFUN void FASTCALL xmlFreeNodeList(xmlNode * cur);
XMLPUBFUN void FASTCALL xmlFreeNode(xmlNode * pCur);
XMLPUBFUN void FASTCALL xmlSetTreeDoc(xmlNode * tree, xmlDoc * doc);
XMLPUBFUN void FASTCALL xmlSetListDoc(xmlNode * list, xmlDoc * doc);
// 
// Namespaces.
// 
XMLPUBFUN xmlNs * FASTCALL xmlSearchNs(xmlDoc * doc, xmlNode * pNode, const xmlChar * nameSpace);
XMLPUBFUN xmlNs * xmlSearchNsByHref(xmlDoc * doc, xmlNode * pNode, const xmlChar * href);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
	XMLPUBFUN xmlNs ** xmlGetNsList(const xmlDoc * doc, const xmlNode * pNode);
#endif
XMLPUBFUN void xmlSetNs(xmlNode * pNode, xmlNs * ns);
XMLPUBFUN xmlNs * xmlCopyNamespace(xmlNs * cur);
XMLPUBFUN xmlNs * xmlCopyNamespaceList(xmlNs * cur);
// 
// Changing the content.
// 
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) ||	defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_HTML_ENABLED)
	XMLPUBFUN xmlAttr * FASTCALL xmlSetProp(xmlNode * pNode, const xmlChar * name, const xmlChar * value);
	XMLPUBFUN xmlAttr * FASTCALL xmlSetNsProp(xmlNode * pNode, xmlNs * ns, const xmlChar * name, const xmlChar * value);
#endif
XMLPUBFUN xmlChar * FASTCALL xmlGetNoNsProp(const xmlNode * pNode, const xmlChar * name);
XMLPUBFUN xmlChar * FASTCALL xmlGetProp(const xmlNode * pNode, const xmlChar * name);
XMLPUBFUN xmlAttr * FASTCALL xmlHasProp(const xmlNode * pNode, const xmlChar * name);
XMLPUBFUN xmlAttr * xmlHasNsProp(const xmlNode * pNode, const xmlChar * name, const xmlChar * nameSpace);
XMLPUBFUN xmlChar * FASTCALL xmlGetNsProp(const xmlNode * pNode, const xmlChar * name, const xmlChar * nameSpace);
XMLPUBFUN xmlNode * xmlStringGetNodeList(const xmlDoc * doc, const xmlChar * value);
XMLPUBFUN xmlNode * xmlStringLenGetNodeList(const xmlDoc * doc, const xmlChar * value, int len);
XMLPUBFUN xmlChar * FASTCALL xmlNodeListGetString(xmlDoc * doc, const xmlNode * list, int inLine);
#ifdef LIBXML_TREE_ENABLED
	XMLPUBFUN xmlChar * xmlNodeListGetRawString(const xmlDoc * doc, const xmlNode * list, int inLine);
#endif
XMLPUBFUN void xmlNodeSetContent(xmlNode * cur, const xmlChar * content);
#ifdef LIBXML_TREE_ENABLED
	XMLPUBFUN void xmlNodeSetContentLen(xmlNode * cur, const xmlChar * content, int len);
#endif
XMLPUBFUN void FASTCALL xmlNodeAddContent(xmlNode * cur, const xmlChar * content);
XMLPUBFUN void FASTCALL xmlNodeAddContentLen(xmlNode * cur, const xmlChar * content, int len);
XMLPUBFUN xmlChar * FASTCALL xmlNodeGetContent(const xmlNode * cur);
XMLPUBFUN int xmlNodeBufGetContent(xmlBuffer * buffer, const xmlNode * cur);
XMLPUBFUN int FASTCALL xmlBufGetNodeContent(xmlBuf * buf, const xmlNode * cur);
XMLPUBFUN xmlChar * xmlNodeGetLang(const xmlNode * cur);
XMLPUBFUN int xmlNodeGetSpacePreserve(const xmlNode * cur);
#ifdef LIBXML_TREE_ENABLED
	XMLPUBFUN void xmlNodeSetLang(xmlNode * cur, const xmlChar * lang);
	XMLPUBFUN void xmlNodeSetSpacePreserve(xmlNode * cur, int val);
#endif
XMLPUBFUN xmlChar * FASTCALL xmlNodeGetBase(const xmlDoc * doc, const xmlNode * cur);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED)
	XMLPUBFUN void xmlNodeSetBase(xmlNode * cur, const xmlChar * uri);
#endif
// 
// Removing content.
// 
XMLPUBFUN int xmlRemoveProp(xmlAttr * cur);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
	XMLPUBFUN int xmlUnsetNsProp(xmlNode * pNode, xmlNs * ns, const xmlChar * name);
	XMLPUBFUN int xmlUnsetProp(xmlNode * pNode, const xmlChar * name);
#endif
// 
// Internal, don't use.
// 
XMLPUBFUN void FASTCALL xmlBufferWriteCHAR(xmlBuffer * buf, const xmlChar * string);
XMLPUBFUN void FASTCALL xmlBufferWriteChar(xmlBuffer * pBuf, const char * pString);
XMLPUBFUN void FASTCALL xmlBufferWriteQuotedString(xmlBuffer * buf, const xmlChar * string);
#ifdef LIBXML_OUTPUT_ENABLED
	XMLPUBFUN void xmlAttrSerializeTxtContent(xmlBuffer * buf, xmlDoc * doc, xmlAttr * attr, const xmlChar * string);
#endif
#ifdef LIBXML_TREE_ENABLED
	// 
	// Namespace handling.
	// 
	XMLPUBFUN int xmlReconciliateNs(xmlDoc * doc, xmlNode * tree);
#endif
#ifdef LIBXML_OUTPUT_ENABLED
// 
// Saving.
// 
XMLPUBFUN void xmlDocDumpFormatMemory(xmlDoc * cur, xmlChar ** mem, int * size, int format);
XMLPUBFUN void xmlDocDumpMemory(xmlDoc * cur, xmlChar ** mem, int * size);
XMLPUBFUN void xmlDocDumpMemoryEnc(xmlDoc * out_doc, xmlChar ** doc_txt_ptr, int * doc_txt_len, const char * txt_encoding);
XMLPUBFUN void xmlDocDumpFormatMemoryEnc(xmlDoc * out_doc, xmlChar ** doc_txt_ptr, int * doc_txt_len, const char * txt_encoding, int format);
XMLPUBFUN int xmlDocFormatDump(FILE * f, xmlDoc * cur, int format);
XMLPUBFUN int xmlDocDump(FILE * f, xmlDoc * cur);
XMLPUBFUN void xmlElemDump(FILE * f, xmlDoc * doc, xmlNode * cur);
XMLPUBFUN int xmlSaveFile(const char * filename, xmlDoc * cur);
XMLPUBFUN int xmlSaveFormatFile(const char * filename, xmlDoc * cur, int format);
XMLPUBFUN size_t xmlBufNodeDump(xmlBufPtr buf, xmlDoc * doc, xmlNode * cur, int level, int format);
XMLPUBFUN int xmlNodeDump(xmlBuffer * buf, xmlDoc * doc, xmlNode * cur, int level, int format);
XMLPUBFUN int xmlSaveFileTo(xmlOutputBuffer * buf, xmlDoc * cur, const char * encoding);
XMLPUBFUN int xmlSaveFormatFileTo(xmlOutputBuffer * buf, xmlDoc * cur, const char * encoding, int format);
XMLPUBFUN void xmlNodeDumpOutput(xmlOutputBuffer * buf, xmlDoc * doc, xmlNode * cur, int level, int format, const char * encoding);
XMLPUBFUN int xmlSaveFormatFileEnc(const char * filename, xmlDoc * cur, const char * encoding, int format);
XMLPUBFUN int xmlSaveFileEnc(const char * filename, xmlDoc * cur, const char * encoding);

#endif /* LIBXML_OUTPUT_ENABLED */
/*
 * XHTML
 */
XMLPUBFUN int xmlIsXHTML(const xmlChar * systemID, const xmlChar * publicID);
/*
 * Compression.
 */
XMLPUBFUN int xmlGetDocCompressMode(const xmlDoc * doc);
XMLPUBFUN void xmlSetDocCompressMode(xmlDoc * doc, int mode);
XMLPUBFUN int xmlGetCompressMode();
XMLPUBFUN void xmlSetCompressMode(int mode);
/*
 * DOM-wrapper helper functions.
 */
XMLPUBFUN xmlDOMWrapCtxtPtr xmlDOMWrapNewCtxt();
XMLPUBFUN void xmlDOMWrapFreeCtxt(xmlDOMWrapCtxtPtr ctxt);
XMLPUBFUN int xmlDOMWrapReconcileNamespaces(xmlDOMWrapCtxtPtr ctxt, xmlNode * elem, int options);
XMLPUBFUN int xmlDOMWrapAdoptNode(xmlDOMWrapCtxtPtr ctxt, xmlDoc * sourceDoc, xmlNode * pNode, xmlDoc * destDoc, xmlNode * destParent, int options);
XMLPUBFUN int xmlDOMWrapRemoveNode(xmlDOMWrapCtxtPtr ctxt, xmlDoc * doc, xmlNode * pNode, int options);
XMLPUBFUN int xmlDOMWrapCloneNode(xmlDOMWrapCtxtPtr ctxt, xmlDoc * sourceDoc, xmlNode * pNode, xmlNode ** clonedNode, xmlDoc * destDoc, xmlNode * destParent, int deep, int options);

#ifdef LIBXML_TREE_ENABLED
/*
 * 5 interfaces from DOM ElementTraversal, but different in entities
 * traversal.
 */
XMLPUBFUN unsigned long xmlChildElementCount(xmlNode * parent);
XMLPUBFUN xmlNode * xmlNextElementSibling(xmlNode * pNode);
XMLPUBFUN xmlNode * xmlFirstElementChild(xmlNode * parent);
XMLPUBFUN xmlNode * xmlLastElementChild(xmlNode * parent);
XMLPUBFUN xmlNode * xmlPreviousElementSibling(xmlNode * pNode);
#endif
//#ifdef __cplusplus
//}
//#endif
#ifndef __XML_PARSER_H__
	#include <libxml/xmlmemory.h>
#endif

#endif /* __XML_TREE_H__ */

