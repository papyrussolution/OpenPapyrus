// VALID.H
// Summary: The DTD validation
// Description: API for the DTD handling and the validity checking
// Copy: See Copyright for the status of this software.
// Author: Daniel Veillard
// 
#ifndef __XML_VALID_H__
#define __XML_VALID_H__

#include <libxml/xmlversion.h>
#include <libxml/xmlerror.h>
#include <libxml/tree.h>
#include <libxml/list.h>
#include <libxml/xmlautomata.h>

#ifdef __cplusplus
extern "C" {
#endif
// 
// Validation state added for non-determinist content model.
// 
typedef struct _xmlValidState xmlValidState;
typedef xmlValidState * xmlValidStatePtr;
/**
 * xmlValidityErrorFunc:
 * @ctx:  usually an xmlValidCtxtPtr to a validity error context,
 *   but comes from ctxt->userData (which normally contains such
 *   a pointer); ctxt->userData can be changed by the user.
 * @msg:  the string to format *printf like vararg
 * @...:  remaining arguments to the format
 *
 * Callback called when a validity error is found. This is a message
 * oriented function similar to an *printf function.
 */
typedef void (XMLCDECL *xmlValidityErrorFunc)(void * ctx, const char * msg, ...) LIBXML_ATTR_FORMAT(2, 3);

/**
 * xmlValidityWarningFunc:
 * @ctx:  usually an xmlValidCtxtPtr to a validity error context,
 *   but comes from ctxt->userData (which normally contains such
 *   a pointer); ctxt->userData can be changed by the user.
 * @msg:  the string to format *printf like vararg
 * @...:  remaining arguments to the format
 *
 * Callback called when a validity warning is found. This is a message
 * oriented function similar to an *printf function.
 */
typedef void (XMLCDECL *xmlValidityWarningFunc)(void * ctx, const char * msg, ...) LIBXML_ATTR_FORMAT(2, 3);

#ifdef IN_LIBXML
/**
 * XML_CTXT_FINISH_DTD_0:
 *
 * Special value for finishDtd field when embedded in an xmlParserCtxt
 */
#define XML_CTXT_FINISH_DTD_0 0xabcd1234
	#define XML_CTXT_FINISH_DTD_1 0xabcd1235 // Special value for finishDtd field when embedded in an xmlParserCtxt
#endif
// 
// Descr: An xmlValidCtxt is used for error reporting when validating.
// 
struct xmlValidCtxt {
	void * userData; /* user specific data block */
	xmlValidityErrorFunc error; /* the callback in case of errors */
	xmlValidityWarningFunc warning; /* the callback in case of warning */
	/* Node analysis stack used when validating within entities */
	xmlNode * P_Node; /* Current parsed Node */
	int    nodeNr;  /* Depth of the parsing stack */
	int    nodeMax; /* Max depth of the parsing stack */
	xmlNode ** PP_NodeTab; // array of nodes 
	uint finishDtd; /* finished validating the Dtd ? */
	xmlDoc * doc; /* the document */
	int valid;                    /* temporary validity check result */
	/* state state used for non-determinist content validation */
	xmlValidState * vstate; /* current state */
	int vstateNr; /* Depth of the validation stack */
	int vstateMax; /* Max depth of the validation stack */
	xmlValidState * vstateTab; /* array of validation states */
#ifdef LIBXML_REGEXP_ENABLED
	xmlAutomata * am;          // the automata 
	xmlAutomataState * state; // used to build the automata
#else
	void * am;
	void * state;
#endif
};

typedef xmlValidCtxt * xmlValidCtxtPtr;
// 
// ALL notation declarations are stored in a table.
// There is one table per DTD.
// 
typedef xmlHashTable xmlNotationTable;
typedef xmlNotationTable * xmlNotationTablePtr;
// 
// ALL element declarations are stored in a table.
// There is one table per DTD.
// 
typedef xmlHashTable xmlElementTable;
//typedef xmlElementTable * xmlElementTablePtr;
// 
// ALL attribute declarations are stored in a table.
// There is one table per DTD.
// 
typedef xmlHashTable xmlAttributeTable;
//typedef xmlAttributeTable * xmlAttributeTablePtr;
// 
// ALL IDs attributes are stored in a table.
// There is one table per document.
// 
typedef xmlHashTable xmlIDTable;
// typedef xmlIDTable * xmlIDTablePtr;
// 
// ALL Refs attributes are stored in a table.
// There is one table per document.
// 
typedef xmlHashTable xmlRefTable;
//typedef xmlRefTable * xmlRefTablePtr;

/* Notation */
XMLPUBFUN xmlNotation * xmlAddNotationDecl(xmlValidCtxtPtr ctxt, xmlDtd * dtd, const xmlChar * name, const xmlChar * PublicID, const xmlChar * SystemID);
#ifdef LIBXML_TREE_ENABLED
	XMLPUBFUN xmlNotationTablePtr xmlCopyNotationTable(xmlNotationTablePtr table);
#endif
XMLPUBFUN void xmlFreeNotationTable(xmlNotationTablePtr table);
#ifdef LIBXML_OUTPUT_ENABLED
	XMLPUBFUN void xmlDumpNotationDecl(xmlBuffer * buf, xmlNotation * nota);
	XMLPUBFUN void xmlDumpNotationTable(xmlBuffer * buf, xmlNotationTablePtr table);
#endif

/* Element Content */
/* the non Doc version are being deprecated */
XMLPUBFUN xmlElementContent * xmlNewElementContent(const xmlChar * name, xmlElementContentType type);
XMLPUBFUN xmlElementContent * xmlCopyElementContent(xmlElementContent * content);
XMLPUBFUN void xmlFreeElementContent(xmlElementContent * cur);
/* the new versions with doc argument */
XMLPUBFUN xmlElementContent * xmlNewDocElementContent(xmlDoc * doc, const xmlChar * name, xmlElementContentType type);
XMLPUBFUN xmlElementContent * xmlCopyDocElementContent(xmlDoc * doc, xmlElementContent * content);
XMLPUBFUN void FASTCALL xmlFreeDocElementContent(xmlDoc * pDoc, xmlElementContent * pCur);
XMLPUBFUN void xmlSnprintfElementContent(char * buf, int size, xmlElementContent * content, int englob);
#ifdef LIBXML_OUTPUT_ENABLED
	XMLPUBFUN void xmlSprintfElementContent(char * buf, xmlElementContent * content, int englob); // DEPRECATED 
#endif
/* DEPRECATED */

/* Element */
XMLPUBFUN xmlElement * xmlAddElementDecl(xmlValidCtxtPtr ctxt, xmlDtd * dtd, const xmlChar * name, xmlElementTypeVal type, xmlElementContent * content);
#ifdef LIBXML_TREE_ENABLED
	XMLPUBFUN xmlElementTable * xmlCopyElementTable(xmlElementTable * table);
#endif
XMLPUBFUN void xmlFreeElementTable(xmlElementTable * table);
#ifdef LIBXML_OUTPUT_ENABLED
	XMLPUBFUN void xmlDumpElementTable(xmlBuffer * buf, xmlElementTable * table);
	XMLPUBFUN void xmlDumpElementDecl(xmlBuffer * buf, xmlElement * elem);
#endif

/* Enumeration */
XMLPUBFUN xmlEnumeration * xmlCreateEnumeration(const xmlChar * name);
XMLPUBFUN void FASTCALL xmlFreeEnumeration(xmlEnumeration * cur);
#ifdef LIBXML_TREE_ENABLED
	XMLPUBFUN xmlEnumeration * xmlCopyEnumeration(xmlEnumeration * cur);
#endif

/* Attribute */
XMLPUBFUN xmlAttribute * xmlAddAttributeDecl(xmlValidCtxtPtr ctxt, xmlDtd * dtd, const xmlChar * elem,
    const xmlChar * name, const xmlChar * ns, xmlAttributeType type, xmlAttributeDefault def, const xmlChar * defaultValue, xmlEnumeration * tree);
#ifdef LIBXML_TREE_ENABLED
	XMLPUBFUN xmlAttributeTable * xmlCopyAttributeTable(xmlAttributeTable * table);
#endif
XMLPUBFUN void xmlFreeAttributeTable(xmlAttributeTable * table);
#ifdef LIBXML_OUTPUT_ENABLED
	XMLPUBFUN void xmlDumpAttributeTable(xmlBuffer * buf, xmlAttributeTable * table);
	XMLPUBFUN void xmlDumpAttributeDecl(xmlBuffer * buf, xmlAttribute * attr);
#endif

/* IDs */
XMLPUBFUN xmlID * xmlAddID(xmlValidCtxtPtr ctxt, xmlDoc * doc, const xmlChar * value, xmlAttr * attr);
XMLPUBFUN void xmlFreeIDTable(xmlIDTable * table);
XMLPUBFUN xmlAttr * xmlGetID(xmlDoc * doc, const xmlChar * ID);
XMLPUBFUN int xmlIsID(xmlDoc * doc, xmlNode * elem, xmlAttr * attr);
XMLPUBFUN int xmlRemoveID(xmlDoc * doc, xmlAttr * attr);

/* IDREFs */
XMLPUBFUN xmlRef * xmlAddRef(xmlValidCtxtPtr ctxt, xmlDoc * doc, const xmlChar * value, xmlAttr * attr);
XMLPUBFUN void FASTCALL xmlFreeRefTable(xmlRefTable * table);
XMLPUBFUN int xmlIsRef(xmlDoc * doc, xmlNode * elem, xmlAttr * attr);
XMLPUBFUN int xmlRemoveRef(xmlDoc * doc, xmlAttr * attr);
XMLPUBFUN xmlList * xmlGetRefs(xmlDoc * pDoc, const xmlChar * pID);
/**
 * The public function calls related to validity checking.
 */
#ifdef LIBXML_VALID_ENABLED
	/* Allocate/Release Validation Contexts */
	XMLPUBFUN xmlValidCtxtPtr xmlNewValidCtxt();
	XMLPUBFUN void xmlFreeValidCtxt(xmlValidCtxtPtr);
	XMLPUBFUN int xmlValidateRoot(xmlValidCtxtPtr ctxt, xmlDoc * doc);
	XMLPUBFUN int xmlValidateElementDecl(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlElement * elem);
	XMLPUBFUN xmlChar * xmlValidNormalizeAttributeValue(xmlDoc * doc, xmlNode * elem, const xmlChar * name, const xmlChar * value);
	XMLPUBFUN xmlChar * xmlValidCtxtNormalizeAttributeValue(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, const xmlChar * name, const xmlChar * value);
	XMLPUBFUN int xmlValidateAttributeDecl(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlAttribute * attr);
	XMLPUBFUN int xmlValidateAttributeValue(xmlAttributeType type, const xmlChar * value);
	XMLPUBFUN int xmlValidateNotationDecl(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNotation * nota);
	XMLPUBFUN int xmlValidateDtd(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlDtd * dtd);
	XMLPUBFUN int xmlValidateDtdFinal(xmlValidCtxtPtr ctxt, xmlDoc * doc);
	XMLPUBFUN int xmlValidateDocument(xmlValidCtxtPtr ctxt, xmlDoc * doc);
	XMLPUBFUN int xmlValidateElement(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem);
	XMLPUBFUN int xmlValidateOneElement(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem);
	XMLPUBFUN int xmlValidateOneAttribute(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, xmlAttr * attr, const xmlChar * value);
	XMLPUBFUN int xmlValidateOneNamespace(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, const xmlChar * prefix, xmlNs * ns, const xmlChar * value);
	XMLPUBFUN int xmlValidateDocumentFinal(xmlValidCtxtPtr ctxt, xmlDoc * doc);
#endif
#if defined(LIBXML_VALID_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
	XMLPUBFUN int xmlValidateNotationUse(xmlValidCtxtPtr ctxt, xmlDoc * doc, const xmlChar * notationName);
#endif

XMLPUBFUN int FASTCALL xmlIsMixedElement(xmlDoc * doc, const xmlChar * name);
XMLPUBFUN xmlAttribute * xmlGetDtdAttrDesc(xmlDtd * dtd, const xmlChar * elem, const xmlChar * name);
XMLPUBFUN xmlAttribute * xmlGetDtdQAttrDesc(xmlDtd * dtd, const xmlChar * elem, const xmlChar * name, const xmlChar * prefix);
XMLPUBFUN xmlNotation * xmlGetDtdNotationDesc(xmlDtd * dtd, const xmlChar * name);
XMLPUBFUN xmlElement * xmlGetDtdQElementDesc(xmlDtd * dtd, const xmlChar * name, const xmlChar * prefix);
XMLPUBFUN xmlElement * FASTCALL xmlGetDtdElementDesc(xmlDtd * dtd, const xmlChar * name);
#ifdef LIBXML_VALID_ENABLED
	XMLPUBFUN int FASTCALL xmlValidGetPotentialChildren(xmlElementContent * ctree, const xmlChar ** names, int * len, int max);
	XMLPUBFUN int xmlValidGetValidElements(xmlNode * prev, xmlNode * next, const xmlChar ** names, int max);
	XMLPUBFUN int xmlValidateNameValue(const xmlChar * value);
	XMLPUBFUN int xmlValidateNamesValue(const xmlChar * value);
	XMLPUBFUN int xmlValidateNmtokenValue(const xmlChar * value);
	XMLPUBFUN int xmlValidateNmtokensValue(const xmlChar * value);

	#ifdef LIBXML_REGEXP_ENABLED
		/*
		 * Validation based on the regexp support
		 */
		XMLPUBFUN int xmlValidBuildContentModel(xmlValidCtxtPtr ctxt, xmlElement * elem);
		XMLPUBFUN int xmlValidatePushElement(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, const xmlChar * qname);
		XMLPUBFUN int xmlValidatePushCData(xmlValidCtxtPtr ctxt, const xmlChar * data, int len);
		XMLPUBFUN int xmlValidatePopElement(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, const xmlChar * qname);
	#endif
#endif
#ifdef __cplusplus
}
#endif
#endif /* __XML_VALID_H__ */
