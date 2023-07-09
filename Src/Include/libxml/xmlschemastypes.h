/*
 * Summary: implementation of XML Schema Datatypes
 * Description: module providing the XML Schema Datatypes implementation both definition and validity checking
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */
#ifndef __XML_SCHEMA_TYPES_H__
#define __XML_SCHEMA_TYPES_H__

#include <libxml/xmlversion.h>

#ifdef LIBXML_SCHEMAS_ENABLED
	#include <libxml/schemasInternals.h>
	#include <libxml/xmlschemas.h>

	//#ifdef __cplusplus
	//extern "C" {
	//#endif

	enum xmlSchemaWhitespaceValueType {
		XML_SCHEMA_WHITESPACE_UNKNOWN = 0,
		XML_SCHEMA_WHITESPACE_PRESERVE = 1,
		XML_SCHEMA_WHITESPACE_REPLACE = 2,
		XML_SCHEMA_WHITESPACE_COLLAPSE = 3
	};

	XMLPUBFUN void xmlSchemaInitTypes();
	XMLPUBFUN void xmlSchemaCleanupTypes();
	XMLPUBFUN xmlSchemaType * xmlSchemaGetPredefinedType(const xmlChar * name, const xmlChar * ns);
	XMLPUBFUN int xmlSchemaValidatePredefinedType(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val);
	XMLPUBFUN int xmlSchemaValPredefTypeNode(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val, xmlNode * P_Node);
	XMLPUBFUN int xmlSchemaValidateFacet(xmlSchemaType * base, xmlSchemaFacet * facet, const xmlChar * value, xmlSchemaVal * val);
	XMLPUBFUN int xmlSchemaValidateFacetWhtsp(xmlSchemaFacet * facet, xmlSchemaWhitespaceValueType fws, xmlSchemaValType valType, const xmlChar * value, xmlSchemaVal * val, xmlSchemaWhitespaceValueType ws);
	XMLPUBFUN void FASTCALL xmlSchemaFreeValue(xmlSchemaVal * pVal);
	XMLPUBFUN xmlSchemaFacet * xmlSchemaNewFacet();
	XMLPUBFUN int xmlSchemaCheckFacet(xmlSchemaFacet * facet, xmlSchemaType * typeDecl, xmlSchemaParserCtxtPtr ctxt, const xmlChar * name);
	XMLPUBFUN void FASTCALL xmlSchemaFreeFacet(xmlSchemaFacet * facet);
	XMLPUBFUN int FASTCALL xmlSchemaCompareValues(xmlSchemaVal * x, xmlSchemaVal * y);
	XMLPUBFUN xmlSchemaType * xmlSchemaGetBuiltInListSimpleTypeItemType(const xmlSchemaType * type);
	XMLPUBFUN int xmlSchemaValidateListSimpleTypeFacet(xmlSchemaFacet * facet, const xmlChar * value, ulong actualLen, ulong * expectedLen);
	XMLPUBFUN xmlSchemaType * FASTCALL xmlSchemaGetBuiltInType(xmlSchemaValType type);
	XMLPUBFUN int xmlSchemaIsBuiltInTypeFacet(xmlSchemaType * type, int facetType);
	XMLPUBFUN xmlChar * xmlSchemaCollapseString(const xmlChar * value);
	XMLPUBFUN xmlChar * xmlSchemaWhiteSpaceReplace(const xmlChar * value);
	XMLPUBFUN ulong xmlSchemaGetFacetValueAsULong(xmlSchemaFacet * facet);
	XMLPUBFUN int xmlSchemaValidateLengthFacet(xmlSchemaType * type, xmlSchemaFacet * facet, const xmlChar * value, xmlSchemaVal * val, ulong * length);
	XMLPUBFUN int xmlSchemaValidateLengthFacetWhtsp(xmlSchemaFacet * facet, xmlSchemaValType valType, const xmlChar * value, xmlSchemaVal * val, ulong * length, xmlSchemaWhitespaceValueType ws);
	XMLPUBFUN int xmlSchemaValPredefTypeNodeNoNorm(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val, xmlNode * P_Node);
	XMLPUBFUN int xmlSchemaGetCanonValue(xmlSchemaVal * val, xmlChar ** retValue);
	XMLPUBFUN int xmlSchemaGetCanonValueWhtsp(xmlSchemaVal * val, xmlChar ** retValue, xmlSchemaWhitespaceValueType ws);
	XMLPUBFUN int xmlSchemaValueAppend(xmlSchemaVal * prev, xmlSchemaVal * cur);
	XMLPUBFUN xmlSchemaVal * xmlSchemaValueGetNext(xmlSchemaVal * cur);
	XMLPUBFUN const xmlChar * xmlSchemaValueGetAsString(xmlSchemaVal * val);
	XMLPUBFUN int xmlSchemaValueGetAsBoolean(xmlSchemaVal * val);
	XMLPUBFUN xmlSchemaVal * xmlSchemaNewStringValue(xmlSchemaValType type, const xmlChar * value);
	XMLPUBFUN xmlSchemaVal * xmlSchemaNewNOTATIONValue(const xmlChar * name, const xmlChar * ns);
	XMLPUBFUN xmlSchemaVal * xmlSchemaNewQNameValue(const xmlChar * namespaceName, const xmlChar * localName);
	XMLPUBFUN int xmlSchemaCompareValuesWhtsp(xmlSchemaVal * x, xmlSchemaWhitespaceValueType xws, xmlSchemaVal * y, xmlSchemaWhitespaceValueType yws);
	XMLPUBFUN xmlSchemaVal * xmlSchemaCopyValue(xmlSchemaVal * val);
	XMLPUBFUN xmlSchemaValType xmlSchemaGetValType(xmlSchemaVal * val);

	//#ifdef __cplusplus
	//}
	//#endif
#endif
#endif
