/*
 * Summary: The DTD validation
 * Description: API for the DTD handling and the validity checking
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

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
/*
 * Validation state added for non-determinist content model.
 */
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
	void * userData;                /* user specific data block */
	xmlValidityErrorFunc error;     /* the callback in case of errors */
	xmlValidityWarningFunc warning; /* the callback in case of warning */
	/* Node analysis stack used when validating within entities */
	xmlNode * P_Node;              /* Current parsed Node */
	int nodeNr;                   /* Depth of the parsing stack */
	int nodeMax;                  /* Max depth of the parsing stack */
	xmlNode ** PP_NodeTab; // array of nodes 
	uint finishDtd;       /* finished validating the Dtd ? */
	xmlDoc * doc;                /* the document */
	int valid;                    /* temporary validity check result */
	/* state state used for non-determinist content validation */
	xmlValidState * vstate;   /* current state */
	int vstateNr;                 /* Depth of the validation stack */
	int vstateMax;                /* Max depth of the validation stack */
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
/*
 * ALL element declarations are stored in a table.
 * There is one table per DTD.
 */
typedef xmlHashTable xmlElementTable;
//typedef xmlElementTable * xmlElementTablePtr;
/*
 * ALL attribute declarations are stored in a table.
 * There is one table per DTD.
 */
typedef xmlHashTable xmlAttributeTable;
//typedef xmlAttributeTable * xmlAttributeTablePtr;
/*
 * ALL IDs attributes are stored in a table.
 * There is one table per document.
 */
typedef xmlHashTable xmlIDTable;
// typedef xmlIDTable * xmlIDTablePtr;
/*
 * ALL Refs attributes are stored in a table.
 * There is one table per document.
 */
typedef xmlHashTable xmlRefTable;
//typedef xmlRefTable * xmlRefTablePtr;

/* Notation */
XMLPUBFUN xmlNotation * XMLCALL xmlAddNotationDecl(xmlValidCtxtPtr ctxt, xmlDtd * dtd, const xmlChar * name, const xmlChar * PublicID, const xmlChar * SystemID);
#ifdef LIBXML_TREE_ENABLED
	XMLPUBFUN xmlNotationTablePtr XMLCALL xmlCopyNotationTable(xmlNotationTablePtr table);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void XMLCALL xmlFreeNotationTable(xmlNotationTablePtr table);
#ifdef LIBXML_OUTPUT_ENABLED
	XMLPUBFUN void XMLCALL xmlDumpNotationDecl(xmlBuffer * buf, xmlNotation * nota);
	XMLPUBFUN void XMLCALL xmlDumpNotationTable(xmlBuffer * buf, xmlNotationTablePtr table);
#endif /* LIBXML_OUTPUT_ENABLED */

/* Element Content */
/* the non Doc version are being deprecated */
XMLPUBFUN xmlElementContent * XMLCALL xmlNewElementContent(const xmlChar * name, xmlElementContentType type);
XMLPUBFUN xmlElementContent * XMLCALL xmlCopyElementContent(xmlElementContent * content);
XMLPUBFUN void XMLCALL xmlFreeElementContent(xmlElementContent * cur);
/* the new versions with doc argument */
XMLPUBFUN xmlElementContent * XMLCALL xmlNewDocElementContent(xmlDoc * doc, const xmlChar * name, xmlElementContentType type);
XMLPUBFUN xmlElementContent * XMLCALL xmlCopyDocElementContent(xmlDoc * doc, xmlElementContent * content);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeDocElementContent(xmlDoc * pDoc, xmlElementContent * pCur);
XMLPUBFUN void XMLCALL xmlSnprintfElementContent(char * buf, int size, xmlElementContent * content, int englob);
#ifdef LIBXML_OUTPUT_ENABLED
	/* DEPRECATED */
	XMLPUBFUN void XMLCALL xmlSprintfElementContent(char * buf, xmlElementContent * content, int englob);
#endif /* LIBXML_OUTPUT_ENABLED */
/* DEPRECATED */

/* Element */
XMLPUBFUN xmlElement * XMLCALL xmlAddElementDecl(xmlValidCtxtPtr ctxt, xmlDtd * dtd, const xmlChar * name, xmlElementTypeVal type, xmlElementContent * content);
#ifdef LIBXML_TREE_ENABLED
	XMLPUBFUN xmlElementTable * XMLCALL xmlCopyElementTable(xmlElementTable * table);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void XMLCALL xmlFreeElementTable(xmlElementTable * table);
#ifdef LIBXML_OUTPUT_ENABLED
	XMLPUBFUN void XMLCALL xmlDumpElementTable(xmlBuffer * buf, xmlElementTable * table);
	XMLPUBFUN void XMLCALL xmlDumpElementDecl(xmlBuffer * buf, xmlElement * elem);
#endif /* LIBXML_OUTPUT_ENABLED */

/* Enumeration */
XMLPUBFUN xmlEnumeration * XMLCALL xmlCreateEnumeration(const xmlChar * name);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeEnumeration(xmlEnumeration * cur);
#ifdef LIBXML_TREE_ENABLED
	XMLPUBFUN xmlEnumeration * XMLCALL xmlCopyEnumeration(xmlEnumeration * cur);
#endif /* LIBXML_TREE_ENABLED */

/* Attribute */
XMLPUBFUN xmlAttribute * XMLCALL xmlAddAttributeDecl(xmlValidCtxtPtr ctxt, xmlDtd * dtd, const xmlChar * elem,
    const xmlChar * name, const xmlChar * ns, xmlAttributeType type, xmlAttributeDefault def, const xmlChar * defaultValue, xmlEnumeration * tree);
#ifdef LIBXML_TREE_ENABLED
	XMLPUBFUN xmlAttributeTable * XMLCALL xmlCopyAttributeTable(xmlAttributeTable * table);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void XMLCALL xmlFreeAttributeTable(xmlAttributeTable * table);
#ifdef LIBXML_OUTPUT_ENABLED
	XMLPUBFUN void XMLCALL xmlDumpAttributeTable(xmlBuffer * buf, xmlAttributeTable * table);
	XMLPUBFUN void XMLCALL xmlDumpAttributeDecl(xmlBuffer * buf, xmlAttribute * attr);
#endif /* LIBXML_OUTPUT_ENABLED */

/* IDs */
XMLPUBFUN xmlID * XMLCALL xmlAddID(xmlValidCtxtPtr ctxt, xmlDoc * doc, const xmlChar * value, xmlAttr * attr);
XMLPUBFUN void XMLCALL xmlFreeIDTable(xmlIDTable * table);
XMLPUBFUN xmlAttr * XMLCALL xmlGetID(xmlDoc * doc, const xmlChar * ID);
XMLPUBFUN int XMLCALL xmlIsID(xmlDoc * doc, xmlNode * elem, xmlAttr * attr);
XMLPUBFUN int XMLCALL xmlRemoveID(xmlDoc * doc, xmlAttr * attr);

/* IDREFs */
XMLPUBFUN xmlRef * XMLCALL xmlAddRef(xmlValidCtxtPtr ctxt, xmlDoc * doc, const xmlChar * value, xmlAttr * attr);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeRefTable(xmlRefTable * table);
XMLPUBFUN int XMLCALL xmlIsRef(xmlDoc * doc, xmlNode * elem, xmlAttr * attr);
XMLPUBFUN int XMLCALL xmlRemoveRef(xmlDoc * doc, xmlAttr * attr);
XMLPUBFUN xmlList * XMLCALL xmlGetRefs(xmlDoc * pDoc, const xmlChar * pID);
/**
 * The public function calls related to validity checking.
 */
#ifdef LIBXML_VALID_ENABLED
	/* Allocate/Release Validation Contexts */
	XMLPUBFUN xmlValidCtxtPtr XMLCALL xmlNewValidCtxt();
	XMLPUBFUN void XMLCALL xmlFreeValidCtxt(xmlValidCtxtPtr);
	XMLPUBFUN int XMLCALL xmlValidateRoot(xmlValidCtxtPtr ctxt, xmlDoc * doc);
	XMLPUBFUN int XMLCALL xmlValidateElementDecl(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlElement * elem);
	XMLPUBFUN xmlChar * XMLCALL xmlValidNormalizeAttributeValue(xmlDoc * doc, xmlNode * elem, const xmlChar * name, const xmlChar * value);
	XMLPUBFUN xmlChar * XMLCALL xmlValidCtxtNormalizeAttributeValue(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, const xmlChar * name, const xmlChar * value);
	XMLPUBFUN int XMLCALL xmlValidateAttributeDecl(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlAttribute * attr);
	XMLPUBFUN int XMLCALL xmlValidateAttributeValue(xmlAttributeType type, const xmlChar * value);
	XMLPUBFUN int XMLCALL xmlValidateNotationDecl(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNotation * nota);
	XMLPUBFUN int XMLCALL xmlValidateDtd(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlDtd * dtd);
	XMLPUBFUN int XMLCALL xmlValidateDtdFinal(xmlValidCtxtPtr ctxt, xmlDoc * doc);
	XMLPUBFUN int XMLCALL xmlValidateDocument(xmlValidCtxtPtr ctxt, xmlDoc * doc);
	XMLPUBFUN int XMLCALL xmlValidateElement(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem);
	XMLPUBFUN int XMLCALL xmlValidateOneElement(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem);
	XMLPUBFUN int XMLCALL xmlValidateOneAttribute(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, xmlAttr * attr, const xmlChar * value);
	XMLPUBFUN int XMLCALL xmlValidateOneNamespace(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, const xmlChar * prefix, xmlNs * ns, const xmlChar * value);
	XMLPUBFUN int XMLCALL xmlValidateDocumentFinal(xmlValidCtxtPtr ctxt, xmlDoc * doc);
#endif /* LIBXML_VALID_ENABLED */
#if defined(LIBXML_VALID_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
	XMLPUBFUN int XMLCALL xmlValidateNotationUse(xmlValidCtxtPtr ctxt, xmlDoc * doc, const xmlChar * notationName);
#endif /* LIBXML_VALID_ENABLED or LIBXML_SCHEMAS_ENABLED */

XMLPUBFUN int /*XMLCALL*/FASTCALL xmlIsMixedElement(xmlDoc * doc, const xmlChar * name);
XMLPUBFUN xmlAttribute * XMLCALL xmlGetDtdAttrDesc(xmlDtd * dtd, const xmlChar * elem, const xmlChar * name);
XMLPUBFUN xmlAttribute * XMLCALL xmlGetDtdQAttrDesc(xmlDtd * dtd, const xmlChar * elem, const xmlChar * name, const xmlChar * prefix);
XMLPUBFUN xmlNotation * XMLCALL xmlGetDtdNotationDesc(xmlDtd * dtd, const xmlChar * name);
XMLPUBFUN xmlElement * XMLCALL xmlGetDtdQElementDesc(xmlDtd * dtd, const xmlChar * name, const xmlChar * prefix);
XMLPUBFUN xmlElement * /*XMLCALL*/FASTCALL xmlGetDtdElementDesc(xmlDtd * dtd, const xmlChar * name);
#ifdef LIBXML_VALID_ENABLED
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlValidGetPotentialChildren(xmlElementContent * ctree, const xmlChar ** names, int * len, int max);
	XMLPUBFUN int XMLCALL xmlValidGetValidElements(xmlNode * prev, xmlNode * next, const xmlChar ** names, int max);
	XMLPUBFUN int XMLCALL xmlValidateNameValue(const xmlChar * value);
	XMLPUBFUN int XMLCALL xmlValidateNamesValue(const xmlChar * value);
	XMLPUBFUN int XMLCALL xmlValidateNmtokenValue(const xmlChar * value);
	XMLPUBFUN int XMLCALL xmlValidateNmtokensValue(const xmlChar * value);

	#ifdef LIBXML_REGEXP_ENABLED
		/*
		 * Validation based on the regexp support
		 */
		XMLPUBFUN int XMLCALL xmlValidBuildContentModel(xmlValidCtxtPtr ctxt, xmlElement * elem);
		XMLPUBFUN int XMLCALL xmlValidatePushElement(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, const xmlChar * qname);
		XMLPUBFUN int XMLCALL xmlValidatePushCData(xmlValidCtxtPtr ctxt, const xmlChar * data, int len);
		XMLPUBFUN int XMLCALL xmlValidatePopElement(xmlValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem, const xmlChar * qname);
	#endif /* LIBXML_REGEXP_ENABLED */
#endif /* LIBXML_VALID_ENABLED */
#ifdef __cplusplus
}
#endif
#endif /* __XML_VALID_H__ */
