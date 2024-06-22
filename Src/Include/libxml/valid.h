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

//#ifdef __cplusplus
//extern "C" {
//#endif
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
xmlNotation * xmlAddNotationDecl(xmlValidCtxtPtr ctxt, xmlDtd * dtd, const xmlChar * name, const xmlChar * PublicID, const xmlChar * SystemID);
#ifdef LIBXML_TREE_ENABLED
	xmlNotationTablePtr xmlCopyNotationTable(xmlNotationTablePtr table);
#endif
void xmlFreeNotationTable(xmlNotationTablePtr table);
#ifdef LIBXML_OUTPUT_ENABLED
	void xmlDumpNotationDecl(xmlBuffer * buf, xmlNotation * nota);
	void xmlDumpNotationTable(xmlBuffer * buf, xmlNotationTablePtr table);
#endif

/* Element Content */
/* the non Doc version are being deprecated */
xmlElementContent * xmlNewElementContent(const xmlChar * name, xmlElementContentType type);
xmlElementContent * xmlCopyElementContent(xmlElementContent * content);
void xmlFreeElementContent(xmlElementContent * cur);
/* the new versions with doc argument */
xmlElementContent * xmlNewDocElementContent(xmlDoc * doc, const xmlChar * name, xmlElementContentType type);
xmlElementContent * xmlCopyDocElementContent(xmlDoc * doc, xmlElementContent * content);
void FASTCALL xmlFreeDocElementContent(xmlDoc * pDoc, xmlElementContent * pCur);
void xmlSnprintfElementContent(char * buf, int size, xmlElementContent * content, int englob);
#ifdef LIBXML_OUTPUT_ENABLED
	void xmlSprintfElementContent(char * buf, xmlElementContent * content, int englob); // DEPRECATED 
#endif
/* DEPRECATED */

/* Element */
xmlElement * xmlAddElementDecl(xmlValidCtxtPtr ctxt, xmlDtd * dtd, const xmlChar * name, xmlElementTypeVal type, xmlElementContent * content);
#ifdef LIBXML_TREE_ENABLED
	xmlElementTable * xmlCopyElementTable(xmlElementTable * table);
#endif
void xmlFreeElementTable(xmlElementTable * table);
#ifdef LIBXML_OUTPUT_ENABLED
	void xmlDumpElementTable(xmlBuffer * buf, xmlElementTable * table);
	void xmlDumpElementDecl(xmlBuffer * buf, xmlElement * elem);
#endif

/* Enumeration */
xmlEnumeration * xmlCreateEnumeration(const xmlChar * name);
void FASTCALL xmlFreeEnumeration(xmlEnumeration * cur);
#ifdef LIBXML_TREE_ENABLED
	xmlEnumeration * xmlCopyEnumeration(xmlEnumeration * cur);
#endif

/* Attribute */
xmlAttribute * xmlAddAttributeDecl(xmlValidCtxtPtr ctxt, xmlDtd * dtd, const xmlChar * elem,
    const xmlChar * name, const xmlChar * ns, xmlAttributeType type, xmlAttributeDefault def, const xmlChar * defaultValue, xmlEnumeration * tree);
#ifdef LIBXML_TREE_ENABLED
	xmlAttributeTable * xmlCopyAttributeTable(xmlAttributeTable * table);
#endif
void xmlFreeAttributeTable(xmlAttributeTable * table);
#ifdef LIBXML_OUTPUT_ENABLED
	void xmlDumpAttributeTable(xmlBuffer * buf, xmlAttributeTable * table);
	void xmlDumpAttributeDecl(xmlBuffer * buf, xmlAttribute * attr);
#endif

/* IDs */
xmlID * xmlAddID(xmlValidCtxtPtr ctxt, xmlDoc * doc, const xmlChar * value, xmlAttr * attr);
void xmlFreeIDTable(xmlIDTable * table);
xmlAttr * xmlGetID(xmlDoc * doc, const xmlChar * ID);
int xmlIsID(xmlDoc * doc, xmlNode * elem, xmlAttr * attr);
int xmlRemoveID(xmlDoc * doc, xmlAttr * attr);

/* IDREFs */
xmlRef * xmlAddRef(xmlValidCtxtPtr ctxt, xmlDoc * doc, const xmlChar * value, xmlAttr * attr);
void FASTCALL xmlFreeRefTable(xmlRefTable * table);
int xmlIsRef(xmlDoc * doc, xmlNode * elem, xmlAttr * attr);
int xmlRemoveRef(xmlDoc * doc, xmlAttr * attr);
xmlList * xmlGetRefs(xmlDoc * pDoc, const xmlChar * pID);
/**
 * The public function calls related to validity checking.
 */
#ifdef LIBXML_VALID_ENABLED
	/* Allocate/Release Validation Contexts */
	xmlValidCtxtPtr xmlNewValidCtxt();
	void xmlFreeValidCtxt(xmlValidCtxtPtr);
	int xmlValidateRoot(xmlValidCtxtPtr ctxt, xmlDoc * doc);
	int xmlValidateElementDecl(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlElement * elem);
	xmlChar * xmlValidNormalizeAttributeValue(xmlDoc * doc, xmlNode * elem, const xmlChar * name, const xmlChar * value);
	xmlChar * xmlValidCtxtNormalizeAttributeValue(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, const xmlChar * name, const xmlChar * value);
	int xmlValidateAttributeDecl(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlAttribute * attr);
	int xmlValidateAttributeValue(xmlAttributeType type, const xmlChar * value);
	int xmlValidateNotationDecl(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNotation * nota);
	int xmlValidateDtd(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlDtd * dtd);
	int xmlValidateDtdFinal(xmlValidCtxtPtr ctxt, xmlDoc * doc);
	int xmlValidateDocument(xmlValidCtxtPtr ctxt, xmlDoc * doc);
	int xmlValidateElement(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem);
	int xmlValidateOneElement(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem);
	int xmlValidateOneAttribute(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, xmlAttr * attr, const xmlChar * value);
	int xmlValidateOneNamespace(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, const xmlChar * prefix, xmlNs * ns, const xmlChar * value);
	int xmlValidateDocumentFinal(xmlValidCtxtPtr ctxt, xmlDoc * doc);
#endif
#if defined(LIBXML_VALID_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
	int xmlValidateNotationUse(xmlValidCtxtPtr ctxt, xmlDoc * doc, const xmlChar * notationName);
#endif

int FASTCALL xmlIsMixedElement(xmlDoc * doc, const xmlChar * name);
xmlAttribute * xmlGetDtdAttrDesc(xmlDtd * dtd, const xmlChar * elem, const xmlChar * name);
xmlAttribute * xmlGetDtdQAttrDesc(xmlDtd * dtd, const xmlChar * elem, const xmlChar * name, const xmlChar * prefix);
xmlNotation * xmlGetDtdNotationDesc(xmlDtd * dtd, const xmlChar * name);
xmlElement * xmlGetDtdQElementDesc(xmlDtd * dtd, const xmlChar * name, const xmlChar * prefix);
xmlElement * FASTCALL xmlGetDtdElementDesc(xmlDtd * dtd, const xmlChar * name);
#ifdef LIBXML_VALID_ENABLED
	int FASTCALL xmlValidGetPotentialChildren(xmlElementContent * ctree, const xmlChar ** names, int * len, int max);
	int xmlValidGetValidElements(xmlNode * prev, xmlNode * next, const xmlChar ** names, int max);
	int xmlValidateNameValue(const xmlChar * value);
	int xmlValidateNamesValue(const xmlChar * value);
	int xmlValidateNmtokenValue(const xmlChar * value);
	int xmlValidateNmtokensValue(const xmlChar * value);

	#ifdef LIBXML_REGEXP_ENABLED
		/*
		 * Validation based on the regexp support
		 */
		int xmlValidBuildContentModel(xmlValidCtxtPtr ctxt, xmlElement * elem);
		int xmlValidatePushElement(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, const xmlChar * qname);
		int xmlValidatePushCData(xmlValidCtxtPtr ctxt, const xmlChar * data, int len);
		int xmlValidatePopElement(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, const xmlChar * qname);
	#endif
#endif
//#ifdef __cplusplus
//}
//#endif
#endif /* __XML_VALID_H__ */
