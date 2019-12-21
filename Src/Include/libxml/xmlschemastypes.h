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

	#ifdef __cplusplus
	extern "C" {
	#endif

	enum xmlSchemaWhitespaceValueType {
		XML_SCHEMA_WHITESPACE_UNKNOWN = 0,
		XML_SCHEMA_WHITESPACE_PRESERVE = 1,
		XML_SCHEMA_WHITESPACE_REPLACE = 2,
		XML_SCHEMA_WHITESPACE_COLLAPSE = 3
	};

	XMLPUBFUN void XMLCALL xmlSchemaInitTypes();
	XMLPUBFUN void XMLCALL xmlSchemaCleanupTypes();
	XMLPUBFUN xmlSchemaType * XMLCALL xmlSchemaGetPredefinedType(const xmlChar * name, const xmlChar * ns);
	XMLPUBFUN int XMLCALL xmlSchemaValidatePredefinedType(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val);
	XMLPUBFUN int XMLCALL xmlSchemaValPredefTypeNode(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val, xmlNode * P_Node);
	XMLPUBFUN int XMLCALL xmlSchemaValidateFacet(xmlSchemaType * base, xmlSchemaFacet * facet, const xmlChar * value, xmlSchemaVal * val);
	XMLPUBFUN int XMLCALL xmlSchemaValidateFacetWhtsp(xmlSchemaFacet * facet, xmlSchemaWhitespaceValueType fws, xmlSchemaValType valType, const xmlChar * value, xmlSchemaVal * val, xmlSchemaWhitespaceValueType ws);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlSchemaFreeValue(xmlSchemaVal * pVal);
	XMLPUBFUN xmlSchemaFacet * XMLCALL xmlSchemaNewFacet();
	XMLPUBFUN int XMLCALL xmlSchemaCheckFacet(xmlSchemaFacet * facet, xmlSchemaType * typeDecl, xmlSchemaParserCtxtPtr ctxt, const xmlChar * name);
	XMLPUBFUN void /*XMLCALL*/FASTCALL xmlSchemaFreeFacet(xmlSchemaFacet * facet);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlSchemaCompareValues(xmlSchemaVal * x, xmlSchemaVal * y);
	XMLPUBFUN xmlSchemaType * XMLCALL xmlSchemaGetBuiltInListSimpleTypeItemType(xmlSchemaType * type);
	XMLPUBFUN int XMLCALL xmlSchemaValidateListSimpleTypeFacet(xmlSchemaFacet * facet, const xmlChar * value, ulong actualLen, ulong * expectedLen);
	XMLPUBFUN xmlSchemaType * /*XMLCALL*/FASTCALL xmlSchemaGetBuiltInType(xmlSchemaValType type);
	XMLPUBFUN int XMLCALL xmlSchemaIsBuiltInTypeFacet(xmlSchemaType * type, int facetType);
	XMLPUBFUN xmlChar * XMLCALL xmlSchemaCollapseString(const xmlChar * value);
	XMLPUBFUN xmlChar * XMLCALL xmlSchemaWhiteSpaceReplace(const xmlChar * value);
	XMLPUBFUN ulong XMLCALL xmlSchemaGetFacetValueAsULong(xmlSchemaFacet * facet);
	XMLPUBFUN int XMLCALL xmlSchemaValidateLengthFacet(xmlSchemaType * type, xmlSchemaFacet * facet, const xmlChar * value, xmlSchemaVal * val, ulong * length);
	XMLPUBFUN int XMLCALL xmlSchemaValidateLengthFacetWhtsp(xmlSchemaFacet * facet, xmlSchemaValType valType, const xmlChar * value, xmlSchemaVal * val, ulong * length, xmlSchemaWhitespaceValueType ws);
	XMLPUBFUN int XMLCALL xmlSchemaValPredefTypeNodeNoNorm(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val, xmlNode * P_Node);
	XMLPUBFUN int XMLCALL xmlSchemaGetCanonValue(xmlSchemaVal * val, xmlChar ** retValue);
	XMLPUBFUN int XMLCALL xmlSchemaGetCanonValueWhtsp(xmlSchemaVal * val, xmlChar ** retValue, xmlSchemaWhitespaceValueType ws);
	XMLPUBFUN int XMLCALL xmlSchemaValueAppend(xmlSchemaVal * prev, xmlSchemaVal * cur);
	XMLPUBFUN xmlSchemaVal * XMLCALL xmlSchemaValueGetNext(xmlSchemaVal * cur);
	XMLPUBFUN const xmlChar * XMLCALL xmlSchemaValueGetAsString(xmlSchemaVal * val);
	XMLPUBFUN int XMLCALL xmlSchemaValueGetAsBoolean(xmlSchemaVal * val);
	XMLPUBFUN xmlSchemaVal * XMLCALL xmlSchemaNewStringValue(xmlSchemaValType type, const xmlChar * value);
	XMLPUBFUN xmlSchemaVal * XMLCALL xmlSchemaNewNOTATIONValue(const xmlChar * name, const xmlChar * ns);
	XMLPUBFUN xmlSchemaVal * XMLCALL xmlSchemaNewQNameValue(const xmlChar * namespaceName, const xmlChar * localName);
	XMLPUBFUN int XMLCALL xmlSchemaCompareValuesWhtsp(xmlSchemaVal * x, xmlSchemaWhitespaceValueType xws, xmlSchemaVal * y, xmlSchemaWhitespaceValueType yws);
	XMLPUBFUN xmlSchemaVal * XMLCALL xmlSchemaCopyValue(xmlSchemaVal * val);
	XMLPUBFUN xmlSchemaValType XMLCALL xmlSchemaGetValType(xmlSchemaVal * val);

	#ifdef __cplusplus
	}
	#endif
#endif
#endif
