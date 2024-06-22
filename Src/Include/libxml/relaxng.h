/*
 * Summary: implementation of the Relax-NG validation
 * Description: implementation of the Relax-NG validation
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_RELAX_NG__
#define __XML_RELAX_NG__

#ifdef LIBXML_SCHEMAS_ENABLED

//#ifdef __cplusplus
//extern "C" {
//#endif

typedef struct _xmlRelaxNG xmlRelaxNG;
typedef xmlRelaxNG * xmlRelaxNGPtr;

/**
 * xmlRelaxNGValidityErrorFunc:
 * @ctx: the validation context
 * @msg: the message
 * @...: extra arguments
 *
 * Signature of an error callback from a Relax-NG validation
 */
typedef void (XMLCDECL *xmlRelaxNGValidityErrorFunc)(void * ctx, const char * msg, ...) LIBXML_ATTR_FORMAT(2, 3);
/**
 * xmlRelaxNGValidityWarningFunc:
 * @ctx: the validation context
 * @msg: the message
 * @...: extra arguments
 *
 * Signature of a warning callback from a Relax-NG validation
 */
typedef void (XMLCDECL *xmlRelaxNGValidityWarningFunc)(void * ctx, const char * msg, ...) LIBXML_ATTR_FORMAT(2, 3);
/**
 * A schemas validation context
 */
typedef struct _xmlRelaxNGParserCtxt xmlRelaxNGParserCtxt;
//typedef xmlRelaxNGParserCtxt * xmlRelaxNGParserCtxtPtr;
typedef struct _xmlRelaxNGValidCtxt xmlRelaxNGValidCtxt;
typedef xmlRelaxNGValidCtxt * xmlRelaxNGValidCtxtPtr;
/*
 * xmlRelaxNGValidErr:
 *
 * List of possible Relax NG validation errors
 */
typedef enum {
	XML_RELAXNG_OK = 0,
	XML_RELAXNG_ERR_MEMORY,
	XML_RELAXNG_ERR_TYPE,
	XML_RELAXNG_ERR_TYPEVAL,
	XML_RELAXNG_ERR_DUPID,
	XML_RELAXNG_ERR_TYPECMP,
	XML_RELAXNG_ERR_NOSTATE,
	XML_RELAXNG_ERR_NODEFINE,
	XML_RELAXNG_ERR_LISTEXTRA,
	XML_RELAXNG_ERR_LISTEMPTY,
	XML_RELAXNG_ERR_INTERNODATA,
	XML_RELAXNG_ERR_INTERSEQ,
	XML_RELAXNG_ERR_INTEREXTRA,
	XML_RELAXNG_ERR_ELEMNAME,
	XML_RELAXNG_ERR_ATTRNAME,
	XML_RELAXNG_ERR_ELEMNONS,
	XML_RELAXNG_ERR_ATTRNONS,
	XML_RELAXNG_ERR_ELEMWRONGNS,
	XML_RELAXNG_ERR_ATTRWRONGNS,
	XML_RELAXNG_ERR_ELEMEXTRANS,
	XML_RELAXNG_ERR_ATTREXTRANS,
	XML_RELAXNG_ERR_ELEMNOTEMPTY,
	XML_RELAXNG_ERR_NOELEM,
	XML_RELAXNG_ERR_NOTELEM,
	XML_RELAXNG_ERR_ATTRVALID,
	XML_RELAXNG_ERR_CONTENTVALID,
	XML_RELAXNG_ERR_EXTRACONTENT,
	XML_RELAXNG_ERR_INVALIDATTR,
	XML_RELAXNG_ERR_DATAELEM,
	XML_RELAXNG_ERR_VALELEM,
	XML_RELAXNG_ERR_LISTELEM,
	XML_RELAXNG_ERR_DATATYPE,
	XML_RELAXNG_ERR_VALUE,
	XML_RELAXNG_ERR_LIST,
	XML_RELAXNG_ERR_NOGRAMMAR,
	XML_RELAXNG_ERR_EXTRADATA,
	XML_RELAXNG_ERR_LACKDATA,
	XML_RELAXNG_ERR_INTERNAL,
	XML_RELAXNG_ERR_ELEMWRONG,
	XML_RELAXNG_ERR_TEXTWRONG
} xmlRelaxNGValidErr;

/*
 * xmlRelaxNGParserFlags:
 *
 * List of possible Relax NG Parser flags
 */
typedef enum {
	XML_RELAXNGP_NONE = 0,
	XML_RELAXNGP_FREE_DOC = 1,
	XML_RELAXNGP_CRNG = 2
} xmlRelaxNGParserFlag;

int xmlRelaxNGInitTypes();
void xmlRelaxNGCleanupTypes();
/*
 * Interfaces for parsing.
 */
xmlRelaxNGParserCtxt * xmlRelaxNGNewParserCtxt(const char * URL);
xmlRelaxNGParserCtxt * xmlRelaxNGNewMemParserCtxt(const char * buffer, int size);
xmlRelaxNGParserCtxt * xmlRelaxNGNewDocParserCtxt(xmlDoc * doc);
int xmlRelaxParserSetFlag(xmlRelaxNGParserCtxt * ctxt, int flag);
void xmlRelaxNGFreeParserCtxt(xmlRelaxNGParserCtxt * ctxt);
void xmlRelaxNGSetParserErrors(xmlRelaxNGParserCtxt * ctxt, xmlRelaxNGValidityErrorFunc err, xmlRelaxNGValidityWarningFunc warn, void * ctx);
int xmlRelaxNGGetParserErrors(xmlRelaxNGParserCtxt * ctxt, xmlRelaxNGValidityErrorFunc * err, xmlRelaxNGValidityWarningFunc * warn, void ** ctx);
void xmlRelaxNGSetParserStructuredErrors(xmlRelaxNGParserCtxt * ctxt, xmlStructuredErrorFunc serror, void * ctx);
xmlRelaxNGPtr xmlRelaxNGParse(xmlRelaxNGParserCtxt * ctxt);
void xmlRelaxNGFree(xmlRelaxNG * schema);
#ifdef LIBXML_OUTPUT_ENABLED
	void xmlRelaxNGDump(FILE * output, xmlRelaxNGPtr schema);
	void xmlRelaxNGDumpTree(FILE * output, xmlRelaxNGPtr schema);
#endif /* LIBXML_OUTPUT_ENABLED */
/*
 * Interfaces for validating
 */
void xmlRelaxNGSetValidErrors(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGValidityErrorFunc err, xmlRelaxNGValidityWarningFunc warn, void * ctx);
int xmlRelaxNGGetValidErrors(xmlRelaxNGValidCtxtPtr ctxt, xmlRelaxNGValidityErrorFunc * err, xmlRelaxNGValidityWarningFunc * warn, void ** ctx);
void xmlRelaxNGSetValidStructuredErrors(xmlRelaxNGValidCtxtPtr ctxt, xmlStructuredErrorFunc serror, void * ctx);
xmlRelaxNGValidCtxtPtr xmlRelaxNGNewValidCtxt(xmlRelaxNGPtr schema);
void xmlRelaxNGFreeValidCtxt(xmlRelaxNGValidCtxtPtr ctxt);
int xmlRelaxNGValidateDoc(xmlRelaxNGValidCtxtPtr ctxt, xmlDoc * doc);
/*
 * Interfaces for progressive validation when possible
 */
int xmlRelaxNGValidatePushElement(xmlRelaxNGValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem);
int xmlRelaxNGValidatePushCData(xmlRelaxNGValidCtxtPtr ctxt, const xmlChar * data, int len);
int xmlRelaxNGValidatePopElement(xmlRelaxNGValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem);
int xmlRelaxNGValidateFullElement(xmlRelaxNGValidCtxtPtr ctxt, xmlDoc * doc, xmlNode * elem);

//#ifdef __cplusplus
//}
//#endif
#endif /* LIBXML_SCHEMAS_ENABLED */
#endif /* __XML_RELAX_NG__ */
