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

	void xmlSchemaInitTypes();
	void xmlSchemaCleanupTypes();
	xmlSchemaType * xmlSchemaGetPredefinedType(const xmlChar * name, const xmlChar * ns);
	int xmlSchemaValidatePredefinedType(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val);
	int xmlSchemaValPredefTypeNode(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val, xmlNode * P_Node);
	int xmlSchemaValidateFacet(xmlSchemaType * base, xmlSchemaFacet * facet, const xmlChar * value, xmlSchemaVal * val);
	int xmlSchemaValidateFacetWhtsp(xmlSchemaFacet * facet, xmlSchemaWhitespaceValueType fws, xmlSchemaValType valType, const xmlChar * value, xmlSchemaVal * val, xmlSchemaWhitespaceValueType ws);
	void FASTCALL xmlSchemaFreeValue(xmlSchemaVal * pVal);
	xmlSchemaFacet * xmlSchemaNewFacet();
	int xmlSchemaCheckFacet(xmlSchemaFacet * facet, xmlSchemaType * typeDecl, xmlSchemaParserCtxtPtr ctxt, const xmlChar * name);
	void FASTCALL xmlSchemaFreeFacet(xmlSchemaFacet * facet);
	int FASTCALL xmlSchemaCompareValues(xmlSchemaVal * x, xmlSchemaVal * y);
	xmlSchemaType * xmlSchemaGetBuiltInListSimpleTypeItemType(const xmlSchemaType * type);
	int xmlSchemaValidateListSimpleTypeFacet(xmlSchemaFacet * facet, const xmlChar * value, ulong actualLen, ulong * expectedLen);
	xmlSchemaType * FASTCALL xmlSchemaGetBuiltInType(xmlSchemaValType type);
	int xmlSchemaIsBuiltInTypeFacet(const xmlSchemaType * type, int facetType);
	xmlChar * xmlSchemaCollapseString(const xmlChar * value);
	xmlChar * xmlSchemaWhiteSpaceReplace(const xmlChar * value);
	ulong xmlSchemaGetFacetValueAsULong(xmlSchemaFacet * facet);
	int xmlSchemaValidateLengthFacet(xmlSchemaType * type, xmlSchemaFacet * facet, const xmlChar * value, xmlSchemaVal * val, ulong * length);
	int xmlSchemaValidateLengthFacetWhtsp(xmlSchemaFacet * facet, xmlSchemaValType valType, const xmlChar * value, xmlSchemaVal * val, ulong * length, xmlSchemaWhitespaceValueType ws);
	int xmlSchemaValPredefTypeNodeNoNorm(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val, xmlNode * P_Node);
	int xmlSchemaGetCanonValue(xmlSchemaVal * val, xmlChar ** retValue);
	int xmlSchemaGetCanonValueWhtsp(xmlSchemaVal * val, xmlChar ** retValue, xmlSchemaWhitespaceValueType ws);
	int xmlSchemaValueAppend(xmlSchemaVal * prev, xmlSchemaVal * cur);
	xmlSchemaVal * xmlSchemaValueGetNext(xmlSchemaVal * cur);
	const xmlChar * xmlSchemaValueGetAsString(xmlSchemaVal * val);
	int xmlSchemaValueGetAsBoolean(xmlSchemaVal * val);
	xmlSchemaVal * xmlSchemaNewStringValue(xmlSchemaValType type, const xmlChar * value);
	xmlSchemaVal * xmlSchemaNewNOTATIONValue(const xmlChar * name, const xmlChar * ns);
	xmlSchemaVal * xmlSchemaNewQNameValue(const xmlChar * namespaceName, const xmlChar * localName);
	int xmlSchemaCompareValuesWhtsp(xmlSchemaVal * x, xmlSchemaWhitespaceValueType xws, xmlSchemaVal * y, xmlSchemaWhitespaceValueType yws);
	xmlSchemaVal * xmlSchemaCopyValue(xmlSchemaVal * val);
	xmlSchemaValType xmlSchemaGetValType(const xmlSchemaVal * val);

	//#ifdef __cplusplus
	//}
	//#endif
#endif
#endif
