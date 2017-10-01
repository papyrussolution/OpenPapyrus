/*
 * Summary: interfaces for tree manipulation
 * Description: this module describes the structures found in an tree resulting
 *              from an XML or HTML parsing, as well as the API provided for
 *              various processing on that tree
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_TREE_H__
#define __XML_TREE_H__

struct xmlDict;
// 
// Some of the basic types pointer to structures:
// 
struct xmlDict;
//typedef struct _xmlParserInputBuffer xmlParserInputBuffer;
struct xmlParserInputBuffer;
typedef xmlParserInputBuffer * xmlParserInputBufferPtr;
//typedef struct _xmlOutputBuffer xmlOutputBuffer;
struct xmlOutputBuffer;
typedef xmlOutputBuffer * xmlOutputBufferPtr;
//typedef struct _xmlParserInput xmlParserInput;
struct xmlParserInput;
typedef xmlParserInput * xmlParserInputPtr;
//typedef struct _xmlParserCtxt xmlParserCtxt;
struct xmlParserCtxt;
typedef xmlParserCtxt * xmlParserCtxtPtr;
//typedef struct _xmlSAXLocator xmlSAXLocator;
struct xmlSAXLocator;
typedef xmlSAXLocator * xmlSAXLocatorPtr;
//typedef struct _xmlSAXHandler xmlSAXHandler;
struct xmlSAXHandler;
typedef xmlSAXHandler * xmlSAXHandlerPtr;
//typedef struct _xmlEntity xmlEntity;
struct xmlEntity;
typedef xmlEntity * xmlEntityPtr;

#include <libxml/xmlversion.h>
#include <libxml/xmlstring.h>

#ifdef __cplusplus
extern "C" {
#endif
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
 * A buffer structure, this old construct is limited to 2GB and
 * is being deprecated, use API with xmlBuf instead
 */
//typedef struct _xmlBuffer xmlBuffer;
struct xmlBuffer;
typedef xmlBuffer * xmlBufferPtr;

struct xmlBuffer {
	xmlChar * content;      /* The buffer content UTF8 */
	uint   use;       /* The buffer size used */
	uint   size;      /* The buffer size */
	xmlBufferAllocationScheme alloc; /* The realloc method */
	xmlChar * contentIO;    /* in IO mode we may have a different base */
};
/**
 * xmlBuf:
 *
 * A buffer structure, new one, the actual structure internals are not public
 */
typedef struct _xmlBuf xmlBuf;
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
XMLPUBFUN xmlChar * /*XMLCALL*/FASTCALL xmlBufContent(const xmlBuf * buf);
XMLPUBFUN xmlChar * /*XMLCALL*/FASTCALL xmlBufEnd(xmlBuf * buf);
XMLPUBFUN size_t /*XMLCALL*/FASTCALL xmlBufUse(xmlBuf * buf);
XMLPUBFUN size_t /*XMLCALL*/FASTCALL xmlBufShrink(xmlBuf * buf, size_t len);

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
#define XML_XML_NAMESPACE (const xmlChar*)"http://www.w3.org/XML/1998/namespace"
#define XML_XML_ID (const xmlChar*)"xml:id" // This is the name for the special xml:id attribute
/*
 * The different element types carried by an XML tree.
 *
 * NOTE: This is synchronized with DOM Level1 values
 *       See http://www.w3.org/TR/REC-DOM-Level-1/
 *
 * Actually this had diverged a bit, and now XML_DOCUMENT_TYPE_NODE should
 * be deprecated to use an XML_DTD_NODE.
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

/**
 * xmlNotation:
 *
 * A DTD Notation definition.
 */

typedef struct _xmlNotation xmlNotation;
typedef xmlNotation * xmlNotationPtr;
struct _xmlNotation {
	const xmlChar * name;     /* Notation name */
	const xmlChar * PublicID; /* Public identifier, if any */
	const xmlChar * SystemID; /* System identifier, if any */
};
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

/**
 * xmlAttributeDefault:
 *
 * A DTD Attribute default definition.
 */

typedef enum {
	XML_ATTRIBUTE_NONE = 1,
	XML_ATTRIBUTE_REQUIRED,
	XML_ATTRIBUTE_IMPLIED,
	XML_ATTRIBUTE_FIXED
} xmlAttributeDefault;

/**
 * xmlEnumeration:
 *
 * List structure used when there is an enumeration in DTDs.
 */
typedef struct _xmlEnumeration xmlEnumeration;
typedef xmlEnumeration * xmlEnumerationPtr;
struct _xmlEnumeration {
	struct _xmlEnumeration * next; /* next one */
	const xmlChar * name; /* Enumeration name */
};
/**
 * xmlAttribute:
 *
 * An Attribute declaration in a DTD.
 */

typedef struct _xmlAttribute xmlAttribute;
typedef xmlAttribute * xmlAttributePtr;
struct _xmlAttribute {
	void           * _private;      /* application data */
	xmlElementType type;            /* XML_ATTRIBUTE_DECL, must be second ! */
	const xmlChar          * name;  /* Attribute name */
	struct _xmlNode    * children;  /* NULL */
	struct _xmlNode        * last;  /* NULL */
	struct _xmlDtd       * parent;  /* -> DTD */
	struct _xmlNode        * next;  /* next sibling link  */
	struct _xmlNode        * prev;  /* previous sibling link  */
	struct _xmlDoc          * doc;  /* the containing document */
	struct _xmlAttribute  * nexth;  /* next in hash table */
	xmlAttributeType atype;         /* The attribute type */
	xmlAttributeDefault def;        /* the default */
	const xmlChar  * defaultValue;  /* or the default value */
	xmlEnumerationPtr tree;         /* or the enumeration tree if any */
	const xmlChar        * prefix;  /* the namespace prefix if any */
	const xmlChar          * elem;  /* Element holding the attribute */
};
/**
 * xmlElementContentType:
 *
 * Possible definitions of element content types.
 */
typedef enum {
	XML_ELEMENT_CONTENT_PCDATA = 1,
	XML_ELEMENT_CONTENT_ELEMENT,
	XML_ELEMENT_CONTENT_SEQ,
	XML_ELEMENT_CONTENT_OR
} xmlElementContentType;

/**
 * xmlElementContentOccur:
 *
 * Possible definitions of element content occurrences.
 */
typedef enum {
	XML_ELEMENT_CONTENT_ONCE = 1,
	XML_ELEMENT_CONTENT_OPT,
	XML_ELEMENT_CONTENT_MULT,
	XML_ELEMENT_CONTENT_PLUS
} xmlElementContentOccur;

/**
 * xmlElementContent:
 *
 * An XML Element content as stored after parsing an element definition
 * in a DTD.
 */
typedef struct _xmlElementContent xmlElementContent;
typedef xmlElementContent * xmlElementContentPtr;

struct _xmlElementContent {
	xmlElementContentType type;     /* PCDATA, ELEMENT, SEQ or OR */
	xmlElementContentOccur ocur;    /* ONCE, OPT, MULT or PLUS */
	const xmlChar             * name; /* Element name */
	struct _xmlElementContent * c1; /* first child */
	struct _xmlElementContent * c2; /* second child */
	struct _xmlElementContent * parent; /* parent */
	const xmlChar             * prefix; /* Namespace prefix */
};
/**
 * xmlElementTypeVal:
 *
 * The different possibilities for an element content type.
 */

typedef enum {
	XML_ELEMENT_TYPE_UNDEFINED = 0,
	XML_ELEMENT_TYPE_EMPTY = 1,
	XML_ELEMENT_TYPE_ANY,
	XML_ELEMENT_TYPE_MIXED,
	XML_ELEMENT_TYPE_ELEMENT
} xmlElementTypeVal;

#ifdef __cplusplus
}
#endif
#include <libxml/xmlregexp.h>
#ifdef __cplusplus
extern "C" {
#endif
/**
 * xmlElement:
 *
 * An XML Element declaration from a DTD.
 */
typedef struct _xmlElement xmlElement;
typedef xmlElement * xmlElementPtr;
struct _xmlElement {
	void * _private;      /* application data */
	xmlElementType type;            /* XML_ELEMENT_DECL, must be second ! */
	const xmlChar * name;  /* Element name */
	struct _xmlNode * children;  /* NULL */
	struct _xmlNode * last;  /* NULL */
	struct _xmlDtd * parent;  /* -> DTD */
	struct _xmlNode * next;  /* next sibling link  */
	struct _xmlNode * prev;  /* previous sibling link  */
	struct _xmlDoc * doc;  /* the containing document */
	xmlElementTypeVal etype;        /* The type */
	xmlElementContentPtr content;   /* the allowed element content */
	xmlAttributePtr attributes;     /* List of the declared attributes */
	const xmlChar * prefix;  /* the namespace prefix if any */
#ifdef LIBXML_REGEXP_ENABLED
	xmlRegexpPtr contModel;         /* the validating regexp */
#else
	void              * contModel;
#endif
};
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
struct xmlNs;
//typedef xmlNs * xmlNsPtr;

struct /*_xmlNs*/xmlNs {
	xmlNs * next;  /* next Ns link for this node  */
	xmlNsType type;         /* global or local */
	const xmlChar * href;   /* URL for the namespace */
	const xmlChar * prefix; /* prefix for the namespace */
	void * _private; /* application data */
	struct _xmlDoc * context;       /* normally an xmlDoc */
};
/**
 * xmlDtd:
 *
 * An XML DTD, as defined by <!DOCTYPE ... There is actually one for
 * the internal subset and for the external subset.
 */
typedef struct _xmlDtd xmlDtd;
typedef xmlDtd * xmlDtdPtr;
struct _xmlDtd {
	void * _private; /* application data */
	xmlElementType type;    /* XML_DTD_NODE, must be second ! */
	const xmlChar * name;   /* Name of the DTD */
	struct _xmlNode * children; /* the value of the property link */
	struct _xmlNode * last; /* last child link */
	struct _xmlDoc  * parent; /* child->parent link */
	struct _xmlNode * next; /* next sibling link  */
	struct _xmlNode * prev; /* previous sibling link  */
	struct _xmlDoc  * doc;  /* the containing document */
	/* End of common part */
	void * notations; /* Hash table for notations if any */
	void * elements; /* Hash table for elements if any */
	void * attributes; /* Hash table for attributes if any */
	void * entities; /* Hash table for entities if any */
	const xmlChar * ExternalID; /* External identifier for PUBLIC DTD */
	const xmlChar * SystemID; /* URI for a SYSTEM or PUBLIC DTD */
	void * pentities; /* Hash table for param entities if any */
};

/**
 * xmlAttr:
 *
 * An attribute on an XML node.
 */
typedef struct _xmlAttr xmlAttr;
typedef xmlAttr * xmlAttrPtr;
struct _xmlAttr {
	void           * _private; /* application data */
	xmlElementType type;    /* XML_ATTRIBUTE_NODE, must be second ! */
	const xmlChar   * name; /* the name of the property */
	struct _xmlNode * children; /* the value of the property */
	struct _xmlNode * last; /* NULL */
	struct _xmlNode * parent; /* child->parent link */
	struct _xmlAttr * next; /* next sibling link  */
	struct _xmlAttr * prev; /* previous sibling link  */
	struct _xmlDoc  * doc;  /* the containing document */
	xmlNs           * ns;   /* pointer to the associated namespace */
	xmlAttributeType atype; /* the attribute type if validating */
	void            * psvi; /* for type/PSVI informations */
};
/**
 * xmlID:
 *
 * An XML ID instance.
 */
typedef struct _xmlID xmlID;
typedef xmlID * xmlIDPtr;

struct _xmlID {
	struct _xmlID * next; /* next ID */
	const xmlChar * value; /* The ID name */
	xmlAttr * attr;        /* The attribute holding it */
	const xmlChar * name; /* The attribute if attr is not available */
	int lineno;             /* The line number if attr is not available */
	struct _xmlDoc * doc; /* The document holding the ID */
};
/**
 * xmlRef:
 *
 * An XML IDREF instance.
 */
typedef struct _xmlRef xmlRef;
typedef xmlRef * xmlRefPtr;

struct _xmlRef {
	struct _xmlRef * next; /* next Ref */
	const xmlChar  * value; /* The Ref name */
	xmlAttr * attr;        /* The attribute holding it */
	const xmlChar * name; /* The attribute if attr is not available */
	int lineno;             /* The line number if attr is not available */
};
/**
 * xmlNode:
 *
 * A node in an XML tree.
 */
typedef struct _xmlNode xmlNode;
typedef xmlNode * xmlNodePtr;

struct _xmlNode {
	void * _private; // application data 
	xmlElementType type; // type number, must be second ! 
	const xmlChar   * name; /* the name of the node, or the entity */
	struct _xmlNode * children; /* parent->childs link */
	struct _xmlNode * last; /* last child link */
	struct _xmlNode * parent; /* child->parent link */
	struct _xmlNode * next; /* next sibling link  */
	struct _xmlNode * prev; /* previous sibling link  */
	struct _xmlDoc  * doc;  /* the containing document */
	// End of common part 
	xmlNs   * ns;   /* pointer to the associated namespace */
	xmlChar * content; /* the content */
	struct _xmlAttr * properties; /* properties list */
	xmlNs * nsDef; /* namespace definitions on this node */
	void  * psvi; /* for type/PSVI informations */
	ushort line;    /* line number */
	ushort extra;   /* extra data for XPath/XSLT */
};
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
	XML_DOC_WELLFORMED          = 1<<0, /* document is XML well formed */
	XML_DOC_NSVALID             = 1<<1, /* document is Namespace valid */
	XML_DOC_OLD10               = 1<<2, /* parsed with old XML-1.0 parser */
	XML_DOC_DTDVALID            = 1<<3, /* DTD validation was successful */
	XML_DOC_XINCLUDE            = 1<<4, /* XInclude substitution was done */
	XML_DOC_USERBUILT           = 1<<5, /* Document was built using the API and not by parsing an instance */
	XML_DOC_INTERNAL            = 1<<6, /* built for internal processing */
	XML_DOC_HTML                = 1<<7 /* parsed or built HTML document */
} xmlDocProperties;
/**
 * xmlDoc:
 *
 * An XML document.
 */
typedef struct _xmlDoc xmlDoc;
typedef xmlDoc * xmlDocPtr;

struct _xmlDoc {
	void           * _private; /* application data */
	xmlElementType type;    /* XML_DOCUMENT_NODE, must be second ! */
	char           * name;  /* name/filename/URI of the document */
	struct _xmlNode * children; /* the document tree */
	struct _xmlNode * last; /* last child link */
	struct _xmlNode * parent; /* child->parent link */
	struct _xmlNode * next; /* next sibling link  */
	struct _xmlNode * prev; /* previous sibling link  */
	struct _xmlDoc  * doc;  /* autoreference to itself */
	/* End of common part */
	int compression;        /* level of zlib compression */
	int standalone; // standalone document (no external refs) 1 if standalone="yes", 0 if standalone="no", 
		// -1 if there is no XML declaration, -2 if there is an XML declaration, but no standalone attribute was specified 
	struct _xmlDtd  * intSubset; /* the document internal subset */
	struct _xmlDtd  * extSubset; /* the document external subset */
	xmlNs * oldNs; /* Global namespace, the old way */
	const xmlChar  * version; /* the XML version string */
	const xmlChar  * encoding; /* external initial encoding, if any */
	void           * ids;   /* Hash table for ID attributes if any */
	void           * refs;  /* Hash table for IDREFs attributes if any */
	const xmlChar  * URL;   /* The URI for that document */
	int charset;            /* encoding of the in-memory content actually an xmlCharEncoding */
	xmlDict * dict; // dict used to allocate names or NULL 
	void * psvi;  /* for type/PSVI informations */
	int parseFlags; // set of xmlParserOption used to parse the document 
	int properties; // set of xmlDocProperties for this document set at the end of parsing 
};

typedef struct _xmlDOMWrapCtxt xmlDOMWrapCtxt;
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
typedef xmlNs * (*xmlDOMWrapAcquireNsFunction)(xmlDOMWrapCtxtPtr ctxt, xmlNode * node, const xmlChar * nsName, const xmlChar * nsPrefix);
/**
 * xmlDOMWrapCtxt:
 *
 * Context for DOM wrapper-operations.
 */
struct _xmlDOMWrapCtxt {
	void * _private;
	int type; // The type of this context, just in case we need specialized contexts in the future.
	void * namespaceMap; // Internal namespace map used for various operations.
	xmlDOMWrapAcquireNsFunction getNsForNodeFunc; // Use this one to acquire an xmlNsPtr intended for node->ns. (Note that this is not intended for elem->nsDef).
};
/**
 * xmlChildrenNode:
 *
 * Macro for compatibility naming layer with libxml1. Maps
 * to "children."
 */
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
XMLPUBFUN int XMLCALL xmlValidateNCName(const xmlChar * value, int space);
#endif
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
	XMLPUBFUN int XMLCALL xmlValidateQName(const xmlChar * value, int space);
	XMLPUBFUN int XMLCALL xmlValidateName(const xmlChar * value, int space);
	XMLPUBFUN int XMLCALL xmlValidateNMToken(const xmlChar * value, int space);
#endif
XMLPUBFUN xmlChar * XMLCALL xmlBuildQName(const xmlChar * ncname, const xmlChar * prefix, xmlChar * memory, int len);
XMLPUBFUN xmlChar * XMLCALL xmlSplitQName2(const xmlChar * name, xmlChar ** prefix);
XMLPUBFUN const xmlChar * XMLCALL xmlSplitQName3(const xmlChar * name, int * len);
/*
 * Handling Buffers, the old ones see @xmlBuf for the new ones.
 */
XMLPUBFUN void XMLCALL xmlSetBufferAllocationScheme(xmlBufferAllocationScheme scheme);
XMLPUBFUN xmlBufferAllocationScheme XMLCALL xmlGetBufferAllocationScheme();
XMLPUBFUN xmlBufferPtr XMLCALL xmlBufferCreate();
XMLPUBFUN xmlBufferPtr XMLCALL xmlBufferCreateSize(size_t size);
XMLPUBFUN xmlBufferPtr XMLCALL xmlBufferCreateStatic(void * mem, size_t size);
XMLPUBFUN int XMLCALL xmlBufferResize(xmlBufferPtr buf, uint size);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlBufferFree(xmlBuffer * pBuf);
XMLPUBFUN int XMLCALL xmlBufferDump(FILE * file, xmlBufferPtr buf);
XMLPUBFUN int XMLCALL xmlBufferAdd(xmlBufferPtr buf, const xmlChar * str, int len);
XMLPUBFUN int XMLCALL xmlBufferAddHead(xmlBufferPtr buf, const xmlChar * str, int len);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlBufferCat(xmlBuffer * pBuf, const xmlChar * pStr);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlBufferCCat(xmlBuffer * pBuf, const char * pStr);
XMLPUBFUN int XMLCALL xmlBufferShrink(xmlBufferPtr buf, uint len);
XMLPUBFUN int XMLCALL xmlBufferGrow(xmlBufferPtr buf, uint len);
XMLPUBFUN void XMLCALL xmlBufferEmpty(xmlBufferPtr buf);
XMLPUBFUN const xmlChar* XMLCALL xmlBufferContent(const xmlBuffer * buf);
XMLPUBFUN xmlChar* XMLCALL xmlBufferDetach(xmlBufferPtr buf);
XMLPUBFUN void XMLCALL xmlBufferSetAllocationScheme(xmlBufferPtr buf, xmlBufferAllocationScheme scheme);
XMLPUBFUN int XMLCALL xmlBufferLength(const xmlBuffer * buf);
/*
 * Creating/freeing new structures.
 */
XMLPUBFUN xmlDtdPtr XMLCALL xmlCreateIntSubset(xmlDocPtr doc, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID);
XMLPUBFUN xmlDtdPtr XMLCALL xmlNewDtd(xmlDocPtr doc, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID);
XMLPUBFUN xmlDtdPtr XMLCALL xmlGetIntSubset(const xmlDoc * doc);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeDtd(xmlDtd * pCur);
#ifdef LIBXML_LEGACY_ENABLED
XMLPUBFUN xmlNs * XMLCALL xmlNewGlobalNs(xmlDocPtr doc, const xmlChar * href, const xmlChar * prefix);
#endif /* LIBXML_LEGACY_ENABLED */
XMLPUBFUN xmlNs * XMLCALL xmlNewNs(xmlNode * node, const xmlChar * href, const xmlChar * prefix);
XMLPUBFUN void XMLCALL xmlFreeNs(xmlNs * cur);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeNsList(xmlNs * cur);
XMLPUBFUN xmlDocPtr XMLCALL xmlNewDoc(const xmlChar * version);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeDoc(xmlDoc * pDoc);
XMLPUBFUN xmlAttrPtr XMLCALL xmlNewDocProp(xmlDocPtr doc, const xmlChar * name, const xmlChar * value);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN xmlAttrPtr XMLCALL xmlNewProp(xmlNode * node, const xmlChar * name, const xmlChar * value);
#endif
XMLPUBFUN xmlAttrPtr XMLCALL xmlNewNsProp(xmlNode * node, xmlNs * ns, const xmlChar * name, const xmlChar * value);
XMLPUBFUN xmlAttrPtr XMLCALL xmlNewNsPropEatName(xmlNode * node, xmlNs * ns, xmlChar * name, const xmlChar * value);
XMLPUBFUN void XMLCALL xmlFreePropList(xmlAttrPtr cur);
XMLPUBFUN void XMLCALL xmlFreeProp(xmlAttrPtr cur);
XMLPUBFUN xmlAttrPtr XMLCALL xmlCopyProp(xmlNode * target, xmlAttrPtr cur);
XMLPUBFUN xmlAttrPtr XMLCALL xmlCopyPropList(xmlNode * target, xmlAttrPtr cur);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlDtdPtr XMLCALL xmlCopyDtd(xmlDtdPtr dtd);
#endif /* LIBXML_TREE_ENABLED */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN xmlDocPtr XMLCALL xmlCopyDoc(xmlDocPtr doc, int recursive);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
/*
 * Creating new nodes.
 */
XMLPUBFUN xmlNode * XMLCALL xmlNewDocNode(xmlDocPtr doc, xmlNs * ns, const xmlChar * name, const xmlChar * content);
XMLPUBFUN xmlNode * XMLCALL xmlNewDocNodeEatName(xmlDocPtr doc, xmlNs * ns, xmlChar * name, const xmlChar * content);
XMLPUBFUN xmlNode * XMLCALL xmlNewNode(xmlNs * ns, const xmlChar * name);
XMLPUBFUN xmlNode * XMLCALL xmlNewNodeEatName(xmlNs * ns, xmlChar * name);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN xmlNode * XMLCALL xmlNewChild(xmlNode * parent, xmlNs * ns, const xmlChar * name, const xmlChar * content);
#endif
XMLPUBFUN xmlNode * XMLCALL xmlNewDocText(const xmlDoc * doc, const xmlChar * content);
XMLPUBFUN xmlNode * XMLCALL xmlNewText(const xmlChar * content);
XMLPUBFUN xmlNode * XMLCALL xmlNewDocPI(xmlDocPtr doc, const xmlChar * name, const xmlChar * content);
XMLPUBFUN xmlNode * XMLCALL xmlNewPI(const xmlChar * name, const xmlChar * content);
XMLPUBFUN xmlNode * XMLCALL xmlNewDocTextLen(xmlDocPtr doc, const xmlChar * content, int len);
XMLPUBFUN xmlNode * XMLCALL xmlNewTextLen(const xmlChar * content, int len);
XMLPUBFUN xmlNode * XMLCALL xmlNewDocComment(xmlDocPtr doc, const xmlChar * content);
XMLPUBFUN xmlNode * XMLCALL xmlNewComment(const xmlChar * content);
XMLPUBFUN xmlNode * XMLCALL xmlNewCDataBlock(xmlDocPtr doc, const xmlChar * content, int len);
XMLPUBFUN xmlNode * XMLCALL xmlNewCharRef(xmlDocPtr doc, const xmlChar * name);
XMLPUBFUN xmlNode * XMLCALL xmlNewReference(const xmlDoc * doc, const xmlChar * name);
XMLPUBFUN xmlNode * XMLCALL xmlCopyNode(xmlNode * node, int recursive);
XMLPUBFUN xmlNode * XMLCALL xmlDocCopyNode(xmlNode * node, xmlDocPtr doc, int recursive);
XMLPUBFUN xmlNode * XMLCALL xmlDocCopyNodeList(xmlDocPtr doc, xmlNode * node);
XMLPUBFUN xmlNode * XMLCALL xmlCopyNodeList(xmlNode * node);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlNode * XMLCALL xmlNewTextChild(xmlNode * parent, xmlNs * ns, const xmlChar * name, const xmlChar * content);
XMLPUBFUN xmlNode * XMLCALL xmlNewDocRawNode(xmlDocPtr doc, xmlNs * ns, const xmlChar * name, const xmlChar * content);
XMLPUBFUN xmlNode * XMLCALL xmlNewDocFragment(xmlDocPtr doc);
#endif /* LIBXML_TREE_ENABLED */

/*
 * Navigating.
 */
XMLPUBFUN long XMLCALL xmlGetLineNo(const xmlNode * node);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_DEBUG_ENABLED)
XMLPUBFUN xmlChar * XMLCALL xmlGetNodePath(const xmlNode * node);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_DEBUG_ENABLED) */
XMLPUBFUN xmlNode * XMLCALL xmlDocGetRootElement(const xmlDoc * doc);
XMLPUBFUN xmlNode * XMLCALL xmlGetLastChild(const xmlNode * parent);
XMLPUBFUN int XMLCALL xmlNodeIsText(const xmlNode * node);
XMLPUBFUN int XMLCALL xmlIsBlankNode(const xmlNode * node);

/*
 * Changing the structure.
 */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED)
XMLPUBFUN xmlNode * XMLCALL xmlDocSetRootElement(xmlDocPtr doc, xmlNode * root);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED) */
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN void XMLCALL xmlNodeSetName(xmlNode * cur, const xmlChar * name);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN xmlNode * /*XMLCALL*/FASTCALL xmlAddChild(xmlNode * parent, xmlNode * cur);
XMLPUBFUN xmlNode * XMLCALL xmlAddChildList(xmlNode * parent, xmlNode * cur);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED)
XMLPUBFUN xmlNode * XMLCALL xmlReplaceNode(xmlNode * old, xmlNode * cur);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED)
XMLPUBFUN xmlNode * XMLCALL xmlAddPrevSibling(xmlNode * cur, xmlNode * elem);
#endif /* LIBXML_TREE_ENABLED || LIBXML_HTML_ENABLED || LIBXML_SCHEMAS_ENABLED */
XMLPUBFUN xmlNode * XMLCALL xmlAddSibling(xmlNode * cur, xmlNode * elem);
XMLPUBFUN xmlNode * XMLCALL xmlAddNextSibling(xmlNode * cur, xmlNode * elem);
XMLPUBFUN void XMLCALL xmlUnlinkNode(xmlNode * cur);
XMLPUBFUN xmlNode * XMLCALL xmlTextMerge(xmlNode * first, xmlNode * second);
XMLPUBFUN int XMLCALL xmlTextConcat(xmlNode * node, const xmlChar * content, int len);
XMLPUBFUN void XMLCALL xmlFreeNodeList(xmlNode * cur);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeNode(xmlNode * pCur);
XMLPUBFUN void XMLCALL xmlSetTreeDoc(xmlNode * tree, xmlDocPtr doc);
XMLPUBFUN void XMLCALL xmlSetListDoc(xmlNode * list, xmlDocPtr doc);
/*
 * Namespaces.
 */
XMLPUBFUN xmlNs * XMLCALL xmlSearchNs(xmlDocPtr doc, xmlNode * node, const xmlChar * nameSpace);
XMLPUBFUN xmlNs * XMLCALL xmlSearchNsByHref(xmlDocPtr doc, xmlNode * node, const xmlChar * href);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN xmlNs ** XMLCALL xmlGetNsList(const xmlDoc * doc, const xmlNode * node);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) */

XMLPUBFUN void XMLCALL xmlSetNs(xmlNode * node, xmlNs * ns);
XMLPUBFUN xmlNs * XMLCALL xmlCopyNamespace(xmlNs * cur);
XMLPUBFUN xmlNs * XMLCALL xmlCopyNamespaceList(xmlNs * cur);

/*
 * Changing the content.
 */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) ||	defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_HTML_ENABLED)
XMLPUBFUN xmlAttrPtr XMLCALL xmlSetProp(xmlNode * node, const xmlChar * name, const xmlChar * value);
XMLPUBFUN xmlAttrPtr XMLCALL xmlSetNsProp(xmlNode * node, xmlNs * ns, const xmlChar * name, const xmlChar * value);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_HTML_ENABLED) */
XMLPUBFUN xmlChar * XMLCALL xmlGetNoNsProp(const xmlNode * node, const xmlChar * name);
XMLPUBFUN xmlChar * /*XMLCALL*/FASTCALL xmlGetProp(const xmlNode * node, const xmlChar * name);
XMLPUBFUN xmlAttrPtr /*XMLCALL*/FASTCALL xmlHasProp(const xmlNode * node, const xmlChar * name);
XMLPUBFUN xmlAttrPtr XMLCALL xmlHasNsProp(const xmlNode * node, const xmlChar * name, const xmlChar * nameSpace);
XMLPUBFUN xmlChar * XMLCALL xmlGetNsProp(const xmlNode * node, const xmlChar * name, const xmlChar * nameSpace);
XMLPUBFUN xmlNode * XMLCALL xmlStringGetNodeList(const xmlDoc * doc, const xmlChar * value);
XMLPUBFUN xmlNode * XMLCALL xmlStringLenGetNodeList(const xmlDoc * doc, const xmlChar * value, int len);
XMLPUBFUN xmlChar * XMLCALL xmlNodeListGetString(xmlDocPtr doc, const xmlNode * list, int inLine);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlChar * XMLCALL xmlNodeListGetRawString(const xmlDoc * doc, const xmlNode * list, int inLine);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void XMLCALL xmlNodeSetContent(xmlNode * cur, const xmlChar * content);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN void XMLCALL xmlNodeSetContentLen(xmlNode * cur, const xmlChar * content, int len);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlNodeAddContent(xmlNode * cur, const xmlChar * content);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlNodeAddContentLen(xmlNode * cur, const xmlChar * content, int len);
XMLPUBFUN xmlChar * XMLCALL xmlNodeGetContent(const xmlNode * cur);
XMLPUBFUN int XMLCALL xmlNodeBufGetContent(xmlBufferPtr buffer, const xmlNode * cur);
XMLPUBFUN int XMLCALL xmlBufGetNodeContent(xmlBufPtr buf, const xmlNode * cur);
XMLPUBFUN xmlChar * XMLCALL xmlNodeGetLang(const xmlNode * cur);
XMLPUBFUN int XMLCALL xmlNodeGetSpacePreserve(const xmlNode * cur);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN void XMLCALL xmlNodeSetLang(xmlNode * cur, const xmlChar * lang);
XMLPUBFUN void XMLCALL xmlNodeSetSpacePreserve(xmlNode * cur, int val);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN xmlChar * XMLCALL xmlNodeGetBase(const xmlDoc * doc, const xmlNode * cur);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED)
XMLPUBFUN void XMLCALL xmlNodeSetBase(xmlNode * cur, const xmlChar * uri);
#endif
/*
 * Removing content.
 */
XMLPUBFUN int XMLCALL xmlRemoveProp(xmlAttrPtr cur);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN int XMLCALL xmlUnsetNsProp(xmlNode * node, xmlNs * ns, const xmlChar * name);
XMLPUBFUN int XMLCALL xmlUnsetProp(xmlNode * node, const xmlChar * name);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
/*
 * Internal, don't use.
 */
XMLPUBFUN void XMLCALL xmlBufferWriteCHAR(xmlBufferPtr buf, const xmlChar * string);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlBufferWriteChar(xmlBuffer * pBuf, const char * pString);
XMLPUBFUN void XMLCALL xmlBufferWriteQuotedString(xmlBufferPtr buf, const xmlChar * string);

#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void xmlAttrSerializeTxtContent(xmlBufferPtr buf, xmlDocPtr doc, xmlAttrPtr attr, const xmlChar * string);
#endif /* LIBXML_OUTPUT_ENABLED */

#ifdef LIBXML_TREE_ENABLED
/*
 * Namespace handling.
 */
XMLPUBFUN int XMLCALL xmlReconciliateNs(xmlDocPtr doc, xmlNode * tree);
#endif

#ifdef LIBXML_OUTPUT_ENABLED
/*
 * Saving.
 */
XMLPUBFUN void XMLCALL xmlDocDumpFormatMemory(xmlDocPtr cur, xmlChar ** mem, int * size, int format);
XMLPUBFUN void XMLCALL xmlDocDumpMemory(xmlDocPtr cur, xmlChar ** mem, int * size);
XMLPUBFUN void XMLCALL xmlDocDumpMemoryEnc(xmlDocPtr out_doc, xmlChar ** doc_txt_ptr, int * doc_txt_len, const char * txt_encoding);
XMLPUBFUN void XMLCALL xmlDocDumpFormatMemoryEnc(xmlDocPtr out_doc, xmlChar ** doc_txt_ptr, int * doc_txt_len, const char * txt_encoding, int format);
XMLPUBFUN int XMLCALL xmlDocFormatDump(FILE * f, xmlDocPtr cur, int format);
XMLPUBFUN int XMLCALL xmlDocDump(FILE * f, xmlDocPtr cur);
XMLPUBFUN void XMLCALL xmlElemDump(FILE * f, xmlDocPtr doc, xmlNode * cur);
XMLPUBFUN int XMLCALL xmlSaveFile(const char * filename, xmlDocPtr cur);
XMLPUBFUN int XMLCALL xmlSaveFormatFile(const char * filename, xmlDocPtr cur, int format);
XMLPUBFUN size_t XMLCALL xmlBufNodeDump(xmlBufPtr buf, xmlDocPtr doc, xmlNode * cur, int level, int format);
XMLPUBFUN int XMLCALL xmlNodeDump(xmlBufferPtr buf, xmlDocPtr doc, xmlNode * cur, int level, int format);
XMLPUBFUN int XMLCALL xmlSaveFileTo(xmlOutputBufferPtr buf, xmlDocPtr cur, const char * encoding);
XMLPUBFUN int XMLCALL xmlSaveFormatFileTo(xmlOutputBufferPtr buf, xmlDocPtr cur, const char * encoding, int format);
XMLPUBFUN void XMLCALL xmlNodeDumpOutput(xmlOutputBufferPtr buf, xmlDocPtr doc, xmlNode * cur, int level, int format, const char * encoding);
XMLPUBFUN int XMLCALL xmlSaveFormatFileEnc(const char * filename, xmlDocPtr cur, const char * encoding, int format);
XMLPUBFUN int XMLCALL xmlSaveFileEnc(const char * filename, xmlDocPtr cur, const char * encoding);

#endif /* LIBXML_OUTPUT_ENABLED */
/*
 * XHTML
 */
XMLPUBFUN int XMLCALL xmlIsXHTML(const xmlChar * systemID, const xmlChar * publicID);
/*
 * Compression.
 */
XMLPUBFUN int XMLCALL xmlGetDocCompressMode(const xmlDoc * doc);
XMLPUBFUN void XMLCALL xmlSetDocCompressMode(xmlDocPtr doc, int mode);
XMLPUBFUN int XMLCALL xmlGetCompressMode();
XMLPUBFUN void XMLCALL xmlSetCompressMode(int mode);
/*
 * DOM-wrapper helper functions.
 */
XMLPUBFUN xmlDOMWrapCtxtPtr XMLCALL xmlDOMWrapNewCtxt();
XMLPUBFUN void XMLCALL xmlDOMWrapFreeCtxt(xmlDOMWrapCtxtPtr ctxt);
XMLPUBFUN int XMLCALL xmlDOMWrapReconcileNamespaces(xmlDOMWrapCtxtPtr ctxt, xmlNode * elem, int options);
XMLPUBFUN int XMLCALL xmlDOMWrapAdoptNode(xmlDOMWrapCtxtPtr ctxt, xmlDocPtr sourceDoc, xmlNode * node, xmlDocPtr destDoc, xmlNode * destParent, int options);
XMLPUBFUN int XMLCALL xmlDOMWrapRemoveNode(xmlDOMWrapCtxtPtr ctxt, xmlDoc * doc, xmlNode * node, int options);
XMLPUBFUN int XMLCALL xmlDOMWrapCloneNode(xmlDOMWrapCtxtPtr ctxt, xmlDocPtr sourceDoc, xmlNode * node, xmlNode ** clonedNode, xmlDocPtr destDoc, xmlNode * destParent, int deep, int options);

#ifdef LIBXML_TREE_ENABLED
/*
 * 5 interfaces from DOM ElementTraversal, but different in entities
 * traversal.
 */
XMLPUBFUN unsigned long XMLCALL xmlChildElementCount(xmlNode * parent);
XMLPUBFUN xmlNode * XMLCALL xmlNextElementSibling(xmlNode * node);
XMLPUBFUN xmlNode * XMLCALL xmlFirstElementChild(xmlNode * parent);
XMLPUBFUN xmlNode * XMLCALL xmlLastElementChild(xmlNode * parent);
XMLPUBFUN xmlNode * XMLCALL xmlPreviousElementSibling(xmlNode * node);
#endif
#ifdef __cplusplus
}
#endif
#ifndef __XML_PARSER_H__
#include <libxml/xmlmemory.h>
#endif

#endif /* __XML_TREE_H__ */

