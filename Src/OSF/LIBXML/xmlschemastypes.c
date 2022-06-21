/*
 * schemastypes.c : implementation of the XML Schema Datatypes definition and validity checking
 * See Copyright for the status of this software.
 * Daniel Veillard <veillard@redhat.com>
 */
#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop

#ifdef LIBXML_SCHEMAS_ENABLED

#include <libxml/xmlschemas.h>
#include <libxml/schemasInternals.h>
#include <libxml/xmlschemastypes.h>
//#ifdef HAVE_MATH_H
	//#include <math.h>
//#endif
//#ifdef HAVE_FLOAT_H
	//#include <float.h>
//#endif
#define DEBUG
#ifndef LIBXML_XPATH_ENABLED
	extern double xmlXPathNAN;
	extern double xmlXPathPINF;
	extern double xmlXPathNINF;
#endif
#define TODO xmlGenericError(0, "Unimplemented block at %s:%d\n", __FILE__, __LINE__);
#define XML_SCHEMAS_NAMESPACE_NAME (const xmlChar *)"http://www.w3.org/2001/XMLSchema"
#define IS_WSP_REPLACE_CH(c)    ((((c) == 0x9) || ((c) == 0xa)) || ((c) == 0xd))
#define IS_WSP_SPACE_CH(c)      ((c) == 0x20)
#define IS_WSP_BLANK_CH(c) IS_BLANK_CH(c)

/* Date value */
typedef struct _xmlSchemaValDate xmlSchemaValDate;
typedef xmlSchemaValDate * xmlSchemaValDatePtr;
struct _xmlSchemaValDate {
	long year;
	uint mon     : 4; /* 1 <=  mon    <= 12   */
	uint day     : 5; /* 1 <=  day    <= 31   */
	uint hour    : 5; /* 0 <=  hour   <= 23   */
	uint min     : 6; /* 0 <=  min    <= 59	*/
	double sec;
	uint tz_flag : 1; /* is tzo explicitely set? */
	signed int tzo     : 12; /* -1440 <= tzo <= 1440;
	                                   currently only -840 to +840 are needed */
};

/* Duration value */
typedef struct _xmlSchemaValDuration xmlSchemaValDuration;
typedef xmlSchemaValDuration * xmlSchemaValDurationPtr;
struct _xmlSchemaValDuration {
	long mon; /* mon stores years also */
	long day;
	double sec; /* sec stores min and hour also */
};

typedef struct _xmlSchemaValDecimal xmlSchemaValDecimal;
typedef xmlSchemaValDecimal * xmlSchemaValDecimalPtr;
struct _xmlSchemaValDecimal {
	/* would use long long but not portable */
	ulong lo;
	ulong mi;
	ulong hi;
	uint extra;
	uint sign : 1;
	uint frac : 7;
	uint total : 8;
};

typedef struct _xmlSchemaValQName xmlSchemaValQName;
typedef xmlSchemaValQName * xmlSchemaValQNamePtr;
struct _xmlSchemaValQName {
	xmlChar * name;
	xmlChar * uri;
};

typedef struct _xmlSchemaValHex xmlSchemaValHex;
typedef xmlSchemaValHex * xmlSchemaValHexPtr;
struct _xmlSchemaValHex {
	xmlChar * str;
	uint total;
};

typedef struct _xmlSchemaValBase64 xmlSchemaValBase64;
typedef xmlSchemaValBase64 * xmlSchemaValBase64Ptr;
struct _xmlSchemaValBase64 {
	xmlChar * str;
	uint total;
};

struct xmlSchemaVal {
	xmlSchemaValType type;
	xmlSchemaVal * next;
	union {
		xmlSchemaValDecimal decimal;
		xmlSchemaValDate date;
		xmlSchemaValDuration dur;
		xmlSchemaValQName qname;
		xmlSchemaValHex hex;
		xmlSchemaValBase64 base64;
		float f;
		double d;
		int b;
		xmlChar * str;
	} value;
};

static int xmlSchemaTypesInitialized = 0;
static xmlHashTable * xmlSchemaTypesBank = NULL;
/*
 * Basic types
 */
static xmlSchemaType * xmlSchemaTypeStringDef = NULL;
static xmlSchemaType * xmlSchemaTypeAnyTypeDef = NULL;
static xmlSchemaType * xmlSchemaTypeAnySimpleTypeDef = NULL;
static xmlSchemaType * xmlSchemaTypeDecimalDef = NULL;
static xmlSchemaType * xmlSchemaTypeDatetimeDef = NULL;
static xmlSchemaType * xmlSchemaTypeDateDef = NULL;
static xmlSchemaType * xmlSchemaTypeTimeDef = NULL;
static xmlSchemaType * xmlSchemaTypeGYearDef = NULL;
static xmlSchemaType * xmlSchemaTypeGYearMonthDef = NULL;
static xmlSchemaType * xmlSchemaTypeGDayDef = NULL;
static xmlSchemaType * xmlSchemaTypeGMonthDayDef = NULL;
static xmlSchemaType * xmlSchemaTypeGMonthDef = NULL;
static xmlSchemaType * xmlSchemaTypeDurationDef = NULL;
static xmlSchemaType * xmlSchemaTypeFloatDef = NULL;
static xmlSchemaType * xmlSchemaTypeBooleanDef = NULL;
static xmlSchemaType * xmlSchemaTypeDoubleDef = NULL;
static xmlSchemaType * xmlSchemaTypeHexBinaryDef = NULL;
static xmlSchemaType * xmlSchemaTypeBase64BinaryDef = NULL;
static xmlSchemaType * xmlSchemaTypeAnyURIDef = NULL;
/*
 * Derived types
 */
static xmlSchemaType * xmlSchemaTypePositiveIntegerDef = NULL;
static xmlSchemaType * xmlSchemaTypeNonPositiveIntegerDef = NULL;
static xmlSchemaType * xmlSchemaTypeNegativeIntegerDef = NULL;
static xmlSchemaType * xmlSchemaTypeNonNegativeIntegerDef = NULL;
static xmlSchemaType * xmlSchemaTypeIntegerDef = NULL;
static xmlSchemaType * xmlSchemaTypeLongDef = NULL;
static xmlSchemaType * xmlSchemaTypeIntDef = NULL;
static xmlSchemaType * xmlSchemaTypeShortDef = NULL;
static xmlSchemaType * xmlSchemaTypeByteDef = NULL;
static xmlSchemaType * xmlSchemaTypeUnsignedLongDef = NULL;
static xmlSchemaType * xmlSchemaTypeUnsignedIntDef = NULL;
static xmlSchemaType * xmlSchemaTypeUnsignedShortDef = NULL;
static xmlSchemaType * xmlSchemaTypeUnsignedByteDef = NULL;
static xmlSchemaType * xmlSchemaTypeNormStringDef = NULL;
static xmlSchemaType * xmlSchemaTypeTokenDef = NULL;
static xmlSchemaType * xmlSchemaTypeLanguageDef = NULL;
static xmlSchemaType * xmlSchemaTypeNameDef = NULL;
static xmlSchemaType * xmlSchemaTypeQNameDef = NULL;
static xmlSchemaType * xmlSchemaTypeNCNameDef = NULL;
static xmlSchemaType * xmlSchemaTypeIdDef = NULL;
static xmlSchemaType * xmlSchemaTypeIdrefDef = NULL;
static xmlSchemaType * xmlSchemaTypeIdrefsDef = NULL;
static xmlSchemaType * xmlSchemaTypeEntityDef = NULL;
static xmlSchemaType * xmlSchemaTypeEntitiesDef = NULL;
static xmlSchemaType * xmlSchemaTypeNotationDef = NULL;
static xmlSchemaType * xmlSchemaTypeNmtokenDef = NULL;
static xmlSchemaType * xmlSchemaTypeNmtokensDef = NULL;
//
// Datatype error handlers
//
/**
 * xmlSchemaTypeErrMemory:
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void FASTCALL xmlSchemaTypeErrMemory(xmlNode * P_Node, const char * extra)
{
	__xmlSimpleError(XML_FROM_DATATYPE, XML_ERR_NO_MEMORY, P_Node, NULL, extra);
}
//
// Base types support
//
/**
 * xmlSchemaNewValue:
 * @type:  the value type
 *
 * Allocate a new simple type value
 *
 * Returns a pointer to the new value or NULL in case of error
 */
static xmlSchemaVal * FASTCALL xmlSchemaNewValue(xmlSchemaValType type)
{
	xmlSchemaVal * value = (xmlSchemaVal *)SAlloc::M(sizeof(xmlSchemaVal));
	if(value) {
		memzero(value, sizeof(xmlSchemaVal));
		value->type = type;
	}
	return value;
}

static xmlSchemaFacet * xmlSchemaNewMinLengthFacet(int value)
{
	xmlSchemaFacet * ret = xmlSchemaNewFacet();
	if(ret) {
		ret->type = XML_SCHEMA_FACET_MINLENGTH;
		ret->val = xmlSchemaNewValue(XML_SCHEMAS_NNINTEGER);
		if(ret->val == NULL) {
			SAlloc::F(ret);
			return 0;
		}
		ret->val->value.decimal.lo = value;
	}
	return ret;
}
/*
 * xmlSchemaInitBasicType:
 * @name:  the type name
 * @type:  the value type associated
 *
 * Initialize one primitive built-in type
 */
static xmlSchemaType * FASTCALL xmlSchemaInitBasicType(const char * name, xmlSchemaValType type, xmlSchemaType * baseType)
{
	xmlSchemaType * ret = (xmlSchemaType *)SAlloc::M(sizeof(xmlSchemaType));
	if(!ret)
		xmlSchemaTypeErrMemory(NULL, "could not initialize basic types");
	else {
		memzero(ret, sizeof(xmlSchemaType));
		ret->name = (const xmlChar *)name;
		ret->targetNamespace = XML_SCHEMAS_NAMESPACE_NAME;
		ret->type = XML_SCHEMA_TYPE_BASIC;
		ret->baseType = baseType;
		ret->contentType = XML_SCHEMA_CONTENT_BASIC;
		/*
		 * Primitive types.
		 */
		switch(type) {
			case XML_SCHEMAS_STRING:
			case XML_SCHEMAS_DECIMAL:
			case XML_SCHEMAS_DATE:
			case XML_SCHEMAS_DATETIME:
			case XML_SCHEMAS_TIME:
			case XML_SCHEMAS_GYEAR:
			case XML_SCHEMAS_GYEARMONTH:
			case XML_SCHEMAS_GMONTH:
			case XML_SCHEMAS_GMONTHDAY:
			case XML_SCHEMAS_GDAY:
			case XML_SCHEMAS_DURATION:
			case XML_SCHEMAS_FLOAT:
			case XML_SCHEMAS_DOUBLE:
			case XML_SCHEMAS_BOOLEAN:
			case XML_SCHEMAS_ANYURI:
			case XML_SCHEMAS_HEXBINARY:
			case XML_SCHEMAS_BASE64BINARY:
			case XML_SCHEMAS_QNAME:
			case XML_SCHEMAS_NOTATION:
				ret->flags |= XML_SCHEMAS_TYPE_BUILTIN_PRIMITIVE;
				break;
			default:
				break;
		}
		/*
		 * Set variety.
		 */
		switch(type) {
			case XML_SCHEMAS_ANYTYPE:
			case XML_SCHEMAS_ANYSIMPLETYPE:
				break;
			case XML_SCHEMAS_IDREFS:
			case XML_SCHEMAS_NMTOKENS:
			case XML_SCHEMAS_ENTITIES:
				ret->flags |= XML_SCHEMAS_TYPE_VARIETY_LIST;
				ret->facets = xmlSchemaNewMinLengthFacet(1);
				ret->flags |= XML_SCHEMAS_TYPE_HAS_FACETS;
				break;
			default:
				ret->flags |= XML_SCHEMAS_TYPE_VARIETY_ATOMIC;
				break;
		}
		xmlHashAddEntry2(xmlSchemaTypesBank, ret->name, XML_SCHEMAS_NAMESPACE_NAME, ret);
		ret->builtInType = type;
	}
	return ret;
}

/*
 * WARNING: Those type reside normally in xmlschemas.c but are
 * redefined here locally in oder of being able to use them for xs:anyType-
 * @todo Remove those definition if we move the types to a header file.
 * @todo Always keep those structs up-to-date with the originals.
 */
#define UNBOUNDED (1 << 30)

typedef struct _xmlSchemaTreeItem xmlSchemaTreeItem;
typedef xmlSchemaTreeItem * xmlSchemaTreeItemPtr;
struct _xmlSchemaTreeItem {
	xmlSchemaTypeType type;
	xmlSchemaAnnotPtr annot;
	xmlSchemaTreeItemPtr next;
	xmlSchemaTreeItemPtr children;
};

//typedef struct _xmlSchemaParticle xmlSchemaParticle;
struct xmlSchemaParticle {
	xmlSchemaTypeType type;
	xmlSchemaAnnotPtr annot;
	xmlSchemaTreeItemPtr next;
	xmlSchemaTreeItemPtr children;
	int minOccurs;
	int maxOccurs;
	xmlNode * P_Node;
};

typedef xmlSchemaParticle * xmlSchemaParticlePtr;

//typedef struct _xmlSchemaModelGroup xmlSchemaModelGroup;
struct xmlSchemaModelGroup {
	xmlSchemaTypeType type;
	xmlSchemaAnnotPtr annot;
	xmlSchemaTreeItemPtr next;
	xmlSchemaTreeItemPtr children;
	xmlNode * P_Node;
};

typedef xmlSchemaModelGroup * xmlSchemaModelGroupPtr;

static xmlSchemaParticlePtr xmlSchemaAddParticle()
{
	xmlSchemaParticlePtr ret = (xmlSchemaParticlePtr)SAlloc::M(sizeof(xmlSchemaParticle));
	if(!ret) {
		xmlSchemaTypeErrMemory(NULL, "allocating particle component");
	}
	else {
		memzero(ret, sizeof(xmlSchemaParticle));
		ret->type = XML_SCHEMA_TYPE_PARTICLE;
		ret->minOccurs = 1;
		ret->maxOccurs = 1;
	}
	return ret;
}
/*
 * xmlSchemaInitTypes:
 *
 * Initialize the default XML Schemas type library
 */
void xmlSchemaInitTypes()
{
	if(xmlSchemaTypesInitialized != 0)
		return;
	xmlSchemaTypesBank = xmlHashCreate(40);
	/*
	 * 3.4.7 Built-in Complex Type Definition
	 */
	xmlSchemaTypeAnyTypeDef = xmlSchemaInitBasicType("anyType", XML_SCHEMAS_ANYTYPE, 0);
	xmlSchemaTypeAnyTypeDef->baseType = xmlSchemaTypeAnyTypeDef;
	xmlSchemaTypeAnyTypeDef->contentType = XML_SCHEMA_CONTENT_MIXED;
	/*
	 * Init the content type.
	 */
	xmlSchemaTypeAnyTypeDef->contentType = XML_SCHEMA_CONTENT_MIXED;
	{
		xmlSchemaParticlePtr particle;
		xmlSchemaModelGroupPtr sequence;
		xmlSchemaWildcardPtr wild;
		/* First particle. */
		particle = xmlSchemaAddParticle();
		if(particle == NULL)
			return;
		xmlSchemaTypeAnyTypeDef->subtypes = (xmlSchemaType *)particle;
		/* Sequence model group. */
		sequence = (xmlSchemaModelGroupPtr)SAlloc::M(sizeof(xmlSchemaModelGroup));
		if(sequence == NULL) {
			xmlSchemaTypeErrMemory(NULL, "allocating model group component");
			return;
		}
		memzero(sequence, sizeof(xmlSchemaModelGroup));
		sequence->type = XML_SCHEMA_TYPE_SEQUENCE;
		particle->children = (xmlSchemaTreeItemPtr)sequence;
		/* Second particle. */
		particle = xmlSchemaAddParticle();
		if(particle == NULL)
			return;
		particle->minOccurs = 0;
		particle->maxOccurs = UNBOUNDED;
		sequence->children = (xmlSchemaTreeItemPtr)particle;
		/* The wildcard */
		wild = (xmlSchemaWildcard *)SAlloc::M(sizeof(xmlSchemaWildcard));
		if(wild == NULL) {
			xmlSchemaTypeErrMemory(NULL, "allocating wildcard component");
			return;
		}
		memzero(wild, sizeof(xmlSchemaWildcard));
		wild->type = XML_SCHEMA_TYPE_ANY;
		wild->any = 1;
		wild->processContents = XML_SCHEMAS_ANY_LAX;
		particle->children = (xmlSchemaTreeItemPtr)wild;
		/*
		 * Create the attribute wildcard.
		 */
		wild = (xmlSchemaWildcard *)SAlloc::M(sizeof(xmlSchemaWildcard));
		if(wild == NULL) {
			xmlSchemaTypeErrMemory(NULL, "could not create an attribute wildcard on anyType");
			return;
		}
		memzero(wild, sizeof(xmlSchemaWildcard));
		wild->any = 1;
		wild->processContents = XML_SCHEMAS_ANY_LAX;
		xmlSchemaTypeAnyTypeDef->attributeWildcard = wild;
	}
	xmlSchemaTypeAnySimpleTypeDef = xmlSchemaInitBasicType("anySimpleType", XML_SCHEMAS_ANYSIMPLETYPE, xmlSchemaTypeAnyTypeDef);
	/*
	 * primitive datatypes
	 */
	xmlSchemaTypeStringDef = xmlSchemaInitBasicType("string", XML_SCHEMAS_STRING, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeDecimalDef = xmlSchemaInitBasicType("decimal", XML_SCHEMAS_DECIMAL, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeDateDef = xmlSchemaInitBasicType("date", XML_SCHEMAS_DATE, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeDatetimeDef = xmlSchemaInitBasicType("dateTime", XML_SCHEMAS_DATETIME, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeTimeDef = xmlSchemaInitBasicType("time", XML_SCHEMAS_TIME, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeGYearDef = xmlSchemaInitBasicType("gYear", XML_SCHEMAS_GYEAR, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeGYearMonthDef = xmlSchemaInitBasicType("gYearMonth", XML_SCHEMAS_GYEARMONTH, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeGMonthDef = xmlSchemaInitBasicType("gMonth", XML_SCHEMAS_GMONTH, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeGMonthDayDef = xmlSchemaInitBasicType("gMonthDay", XML_SCHEMAS_GMONTHDAY, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeGDayDef = xmlSchemaInitBasicType("gDay", XML_SCHEMAS_GDAY, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeDurationDef = xmlSchemaInitBasicType("duration", XML_SCHEMAS_DURATION, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeFloatDef = xmlSchemaInitBasicType("float", XML_SCHEMAS_FLOAT, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeDoubleDef = xmlSchemaInitBasicType("double", XML_SCHEMAS_DOUBLE, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeBooleanDef = xmlSchemaInitBasicType("boolean", XML_SCHEMAS_BOOLEAN, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeAnyURIDef = xmlSchemaInitBasicType("anyURI", XML_SCHEMAS_ANYURI, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeHexBinaryDef = xmlSchemaInitBasicType("hexBinary", XML_SCHEMAS_HEXBINARY, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeBase64BinaryDef = xmlSchemaInitBasicType("base64Binary", XML_SCHEMAS_BASE64BINARY, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeNotationDef = xmlSchemaInitBasicType("NOTATION", XML_SCHEMAS_NOTATION, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeQNameDef = xmlSchemaInitBasicType("QName", XML_SCHEMAS_QNAME, xmlSchemaTypeAnySimpleTypeDef);
	/*
	 * derived datatypes
	 */
	xmlSchemaTypeIntegerDef = xmlSchemaInitBasicType("integer", XML_SCHEMAS_INTEGER, xmlSchemaTypeDecimalDef);
	xmlSchemaTypeNonPositiveIntegerDef = xmlSchemaInitBasicType("nonPositiveInteger", XML_SCHEMAS_NPINTEGER, xmlSchemaTypeIntegerDef);
	xmlSchemaTypeNegativeIntegerDef = xmlSchemaInitBasicType("negativeInteger", XML_SCHEMAS_NINTEGER, xmlSchemaTypeNonPositiveIntegerDef);
	xmlSchemaTypeLongDef = xmlSchemaInitBasicType("long", XML_SCHEMAS_LONG, xmlSchemaTypeIntegerDef);
	xmlSchemaTypeIntDef = xmlSchemaInitBasicType("int", XML_SCHEMAS_INT, xmlSchemaTypeLongDef);
	xmlSchemaTypeShortDef = xmlSchemaInitBasicType("short", XML_SCHEMAS_SHORT, xmlSchemaTypeIntDef);
	xmlSchemaTypeByteDef = xmlSchemaInitBasicType("byte", XML_SCHEMAS_BYTE, xmlSchemaTypeShortDef);
	xmlSchemaTypeNonNegativeIntegerDef = xmlSchemaInitBasicType("nonNegativeInteger", XML_SCHEMAS_NNINTEGER, xmlSchemaTypeIntegerDef);
	xmlSchemaTypeUnsignedLongDef = xmlSchemaInitBasicType("unsignedLong", XML_SCHEMAS_ULONG, xmlSchemaTypeNonNegativeIntegerDef);
	xmlSchemaTypeUnsignedIntDef = xmlSchemaInitBasicType("unsignedInt", XML_SCHEMAS_UINT, xmlSchemaTypeUnsignedLongDef);
	xmlSchemaTypeUnsignedShortDef = xmlSchemaInitBasicType("unsignedShort", XML_SCHEMAS_USHORT, xmlSchemaTypeUnsignedIntDef);
	xmlSchemaTypeUnsignedByteDef = xmlSchemaInitBasicType("unsignedByte", XML_SCHEMAS_UBYTE, xmlSchemaTypeUnsignedShortDef);
	xmlSchemaTypePositiveIntegerDef = xmlSchemaInitBasicType("positiveInteger", XML_SCHEMAS_PINTEGER, xmlSchemaTypeNonNegativeIntegerDef);
	xmlSchemaTypeNormStringDef = xmlSchemaInitBasicType("normalizedString", XML_SCHEMAS_NORMSTRING, xmlSchemaTypeStringDef);
	xmlSchemaTypeTokenDef = xmlSchemaInitBasicType("token", XML_SCHEMAS_TOKEN, xmlSchemaTypeNormStringDef);
	xmlSchemaTypeLanguageDef = xmlSchemaInitBasicType("language", XML_SCHEMAS_LANGUAGE, xmlSchemaTypeTokenDef);
	xmlSchemaTypeNameDef = xmlSchemaInitBasicType("Name", XML_SCHEMAS_NAME, xmlSchemaTypeTokenDef);
	xmlSchemaTypeNmtokenDef = xmlSchemaInitBasicType("NMTOKEN", XML_SCHEMAS_NMTOKEN, xmlSchemaTypeTokenDef);
	xmlSchemaTypeNCNameDef = xmlSchemaInitBasicType("NCName", XML_SCHEMAS_NCNAME, xmlSchemaTypeNameDef);
	xmlSchemaTypeIdDef = xmlSchemaInitBasicType("ID", XML_SCHEMAS_ID, xmlSchemaTypeNCNameDef);
	xmlSchemaTypeIdrefDef = xmlSchemaInitBasicType("IDREF", XML_SCHEMAS_IDREF, xmlSchemaTypeNCNameDef);
	xmlSchemaTypeEntityDef = xmlSchemaInitBasicType("ENTITY", XML_SCHEMAS_ENTITY, xmlSchemaTypeNCNameDef);
	/*
	 * Derived list types.
	 */
	/* ENTITIES */
	xmlSchemaTypeEntitiesDef = xmlSchemaInitBasicType("ENTITIES", XML_SCHEMAS_ENTITIES, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeEntitiesDef->subtypes = xmlSchemaTypeEntityDef;
	/* IDREFS */
	xmlSchemaTypeIdrefsDef = xmlSchemaInitBasicType("IDREFS", XML_SCHEMAS_IDREFS, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeIdrefsDef->subtypes = xmlSchemaTypeIdrefDef;
	/* NMTOKENS */
	xmlSchemaTypeNmtokensDef = xmlSchemaInitBasicType("NMTOKENS", XML_SCHEMAS_NMTOKENS, xmlSchemaTypeAnySimpleTypeDef);
	xmlSchemaTypeNmtokensDef->subtypes = xmlSchemaTypeNmtokenDef;
	xmlSchemaTypesInitialized = 1;
}
/**
 * xmlSchemaCleanupTypes:
 *
 * Cleanup the default XML Schemas type library
 */
void xmlSchemaCleanupTypes()
{
	if(xmlSchemaTypesInitialized) {
		/*
		 * Free xs:anyType.
		 */
		{
			xmlSchemaParticlePtr particle;
			/* Attribute wildcard. */
			xmlSchemaFreeWildcard(xmlSchemaTypeAnyTypeDef->attributeWildcard);
			/* Content type. */
			particle = (xmlSchemaParticlePtr)xmlSchemaTypeAnyTypeDef->subtypes;
			/* Wildcard. */
			xmlSchemaFreeWildcard((xmlSchemaWildcard *)particle->children->children->children);
			SAlloc::F((xmlSchemaParticlePtr)particle->children->children);
			/* Sequence model group. */
			SAlloc::F((xmlSchemaModelGroupPtr)particle->children);
			SAlloc::F((xmlSchemaParticlePtr)particle);
			xmlSchemaTypeAnyTypeDef->subtypes = NULL;
		}
		xmlHashFree(xmlSchemaTypesBank, (xmlHashDeallocator)xmlSchemaFreeType);
		xmlSchemaTypesInitialized = 0;
	}
}
/**
 * xmlSchemaIsBuiltInTypeFacet:
 * @type: the built-in type
 * @facetType:  the facet type
 *
 * Evaluates if a specific facet can be
 * used in conjunction with a type.
 *
 * Returns 1 if the facet can be used with the given built-in type,
 * 0 otherwise and -1 in case the type is not a built-in type.
 */
int xmlSchemaIsBuiltInTypeFacet(xmlSchemaType * type, int facetType)
{
	if(type == NULL)
		return -1;
	if(type->type != XML_SCHEMA_TYPE_BASIC)
		return -1;
	switch(type->builtInType) {
		case XML_SCHEMAS_BOOLEAN:
			return oneof2(facetType, XML_SCHEMA_FACET_PATTERN, XML_SCHEMA_FACET_WHITESPACE) ? 1 : 0;
		case XML_SCHEMAS_STRING:
		case XML_SCHEMAS_NOTATION:
		case XML_SCHEMAS_QNAME:
		case XML_SCHEMAS_ANYURI:
		case XML_SCHEMAS_BASE64BINARY:
		case XML_SCHEMAS_HEXBINARY:
		    if((facetType == XML_SCHEMA_FACET_LENGTH) ||
		    (facetType == XML_SCHEMA_FACET_MINLENGTH) ||
		    (facetType == XML_SCHEMA_FACET_MAXLENGTH) ||
		    (facetType == XML_SCHEMA_FACET_PATTERN) ||
		    (facetType == XML_SCHEMA_FACET_ENUMERATION) ||
		    (facetType == XML_SCHEMA_FACET_WHITESPACE))
			    return 1;
		    else
			    return 0;
		case XML_SCHEMAS_DECIMAL:
		    if(oneof9(facetType, XML_SCHEMA_FACET_TOTALDIGITS, XML_SCHEMA_FACET_FRACTIONDIGITS,
				XML_SCHEMA_FACET_PATTERN, XML_SCHEMA_FACET_WHITESPACE, XML_SCHEMA_FACET_ENUMERATION,
				XML_SCHEMA_FACET_MAXINCLUSIVE, XML_SCHEMA_FACET_MAXEXCLUSIVE, XML_SCHEMA_FACET_MININCLUSIVE,
				XML_SCHEMA_FACET_MINEXCLUSIVE))
			    return 1;
		    else
			    return 0;
		case XML_SCHEMAS_TIME:
		case XML_SCHEMAS_GDAY:
		case XML_SCHEMAS_GMONTH:
		case XML_SCHEMAS_GMONTHDAY:
		case XML_SCHEMAS_GYEAR:
		case XML_SCHEMAS_GYEARMONTH:
		case XML_SCHEMAS_DATE:
		case XML_SCHEMAS_DATETIME:
		case XML_SCHEMAS_DURATION:
		case XML_SCHEMAS_FLOAT:
		case XML_SCHEMAS_DOUBLE:
		    if(oneof7(facetType, XML_SCHEMA_FACET_PATTERN, XML_SCHEMA_FACET_ENUMERATION,
				XML_SCHEMA_FACET_WHITESPACE, XML_SCHEMA_FACET_MAXINCLUSIVE, XML_SCHEMA_FACET_MAXEXCLUSIVE,
				XML_SCHEMA_FACET_MININCLUSIVE, XML_SCHEMA_FACET_MINEXCLUSIVE))
			    return 1;
		    else
			    return 0;
		default:
		    break;
	}
	return 0;
}
/**
 * xmlSchemaGetBuiltInType:
 * @type:  the type of the built in type
 *
 * Gives you the type struct for a built-in
 * type by its type id.
 *
 * Returns the type if found, NULL otherwise.
 */
xmlSchemaType * FASTCALL xmlSchemaGetBuiltInType(xmlSchemaValType type)
{
	if(xmlSchemaTypesInitialized == 0)
		xmlSchemaInitTypes();
	switch(type) {
		case XML_SCHEMAS_ANYSIMPLETYPE: return (xmlSchemaTypeAnySimpleTypeDef);
		case XML_SCHEMAS_STRING: return (xmlSchemaTypeStringDef);
		case XML_SCHEMAS_NORMSTRING: return (xmlSchemaTypeNormStringDef);
		case XML_SCHEMAS_DECIMAL: return (xmlSchemaTypeDecimalDef);
		case XML_SCHEMAS_TIME: return (xmlSchemaTypeTimeDef);
		case XML_SCHEMAS_GDAY: return (xmlSchemaTypeGDayDef);
		case XML_SCHEMAS_GMONTH: return (xmlSchemaTypeGMonthDef);
		case XML_SCHEMAS_GMONTHDAY: return (xmlSchemaTypeGMonthDayDef);
		case XML_SCHEMAS_GYEAR: return (xmlSchemaTypeGYearDef);
		case XML_SCHEMAS_GYEARMONTH: return (xmlSchemaTypeGYearMonthDef);
		case XML_SCHEMAS_DATE: return (xmlSchemaTypeDateDef);
		case XML_SCHEMAS_DATETIME: return (xmlSchemaTypeDatetimeDef);
		case XML_SCHEMAS_DURATION: return (xmlSchemaTypeDurationDef);
		case XML_SCHEMAS_FLOAT: return (xmlSchemaTypeFloatDef);
		case XML_SCHEMAS_DOUBLE: return (xmlSchemaTypeDoubleDef);
		case XML_SCHEMAS_BOOLEAN: return (xmlSchemaTypeBooleanDef);
		case XML_SCHEMAS_TOKEN: return (xmlSchemaTypeTokenDef);
		case XML_SCHEMAS_LANGUAGE: return (xmlSchemaTypeLanguageDef);
		case XML_SCHEMAS_NMTOKEN: return (xmlSchemaTypeNmtokenDef);
		case XML_SCHEMAS_NMTOKENS: return (xmlSchemaTypeNmtokensDef);
		case XML_SCHEMAS_NAME: return (xmlSchemaTypeNameDef);
		case XML_SCHEMAS_QNAME: return (xmlSchemaTypeQNameDef);
		case XML_SCHEMAS_NCNAME: return (xmlSchemaTypeNCNameDef);
		case XML_SCHEMAS_ID: return (xmlSchemaTypeIdDef);
		case XML_SCHEMAS_IDREF: return (xmlSchemaTypeIdrefDef);
		case XML_SCHEMAS_IDREFS: return (xmlSchemaTypeIdrefsDef);
		case XML_SCHEMAS_ENTITY: return (xmlSchemaTypeEntityDef);
		case XML_SCHEMAS_ENTITIES: return (xmlSchemaTypeEntitiesDef);
		case XML_SCHEMAS_NOTATION: return (xmlSchemaTypeNotationDef);
		case XML_SCHEMAS_ANYURI: return (xmlSchemaTypeAnyURIDef);
		case XML_SCHEMAS_INTEGER: return (xmlSchemaTypeIntegerDef);
		case XML_SCHEMAS_NPINTEGER: return (xmlSchemaTypeNonPositiveIntegerDef);
		case XML_SCHEMAS_NINTEGER: return (xmlSchemaTypeNegativeIntegerDef);
		case XML_SCHEMAS_NNINTEGER: return (xmlSchemaTypeNonNegativeIntegerDef);
		case XML_SCHEMAS_PINTEGER: return (xmlSchemaTypePositiveIntegerDef);
		case XML_SCHEMAS_INT: return (xmlSchemaTypeIntDef);
		case XML_SCHEMAS_UINT: return (xmlSchemaTypeUnsignedIntDef);
		case XML_SCHEMAS_LONG: return (xmlSchemaTypeLongDef);
		case XML_SCHEMAS_ULONG: return (xmlSchemaTypeUnsignedLongDef);
		case XML_SCHEMAS_SHORT: return (xmlSchemaTypeShortDef);
		case XML_SCHEMAS_USHORT: return (xmlSchemaTypeUnsignedShortDef);
		case XML_SCHEMAS_BYTE: return (xmlSchemaTypeByteDef);
		case XML_SCHEMAS_UBYTE: return (xmlSchemaTypeUnsignedByteDef);
		case XML_SCHEMAS_HEXBINARY: return (xmlSchemaTypeHexBinaryDef);
		case XML_SCHEMAS_BASE64BINARY: return (xmlSchemaTypeBase64BinaryDef);
		case XML_SCHEMAS_ANYTYPE: return (xmlSchemaTypeAnyTypeDef);
		default: return 0;
	}
}
/**
 * xmlSchemaValueAppend:
 * @prev: the value
 * @cur: the value to be appended
 *
 * Appends a next sibling to a list of computed values.
 *
 * Returns 0 if succeeded and -1 on API errors.
 */
int xmlSchemaValueAppend(xmlSchemaVal * prev, xmlSchemaVal * cur)
{
	if(prev && cur) {
		prev->next = cur;
		return 0;
	}
	else
		return -1;
}
/**
 * xmlSchemaValueGetNext:
 * @cur: the value
 *
 * Accessor for the next sibling of a list of computed values.
 *
 * Returns the next value or NULL if there was none, or on API errors.
 */
xmlSchemaVal * xmlSchemaValueGetNext(xmlSchemaVal * cur)
{
	return cur ? cur->next : 0;
}
/**
 * xmlSchemaValueGetAsString:
 * @val: the value
 *
 * Accessor for the string value of a computed value.
 *
 * Returns the string value or NULL if there was none, or on
 *    API errors.
 */
const xmlChar * xmlSchemaValueGetAsString(xmlSchemaVal * val)
{
	if(val) {
		switch(val->type) {
			case XML_SCHEMAS_STRING:
			case XML_SCHEMAS_NORMSTRING:
			case XML_SCHEMAS_ANYSIMPLETYPE:
			case XML_SCHEMAS_TOKEN:
			case XML_SCHEMAS_LANGUAGE:
			case XML_SCHEMAS_NMTOKEN:
			case XML_SCHEMAS_NAME:
			case XML_SCHEMAS_NCNAME:
			case XML_SCHEMAS_ID:
			case XML_SCHEMAS_IDREF:
			case XML_SCHEMAS_ENTITY:
			case XML_SCHEMAS_ANYURI:
				return (BAD_CAST val->value.str);
			default:
				break;
		}
	}
	return 0;
}

/**
 * xmlSchemaValueGetAsBoolean:
 * @val: the value
 *
 * Accessor for the boolean value of a computed value.
 *
 * Returns 1 if true and 0 if false, or in case of an error. Hmm.
 */
int xmlSchemaValueGetAsBoolean(xmlSchemaVal * val)
{
	return (val && val->type == XML_SCHEMAS_BOOLEAN) ? val->value.b : 0;
}

/**
 * xmlSchemaNewStringValue:
 * @type:  the value type
 * @value:  the value
 *
 * Allocate a new simple type value. The type can be
 * of XML_SCHEMAS_STRING.
 * WARNING: This one is intended to be expanded for other
 * string based types. We need this for anySimpleType as well.
 * The given value is consumed and freed with the struct.
 *
 * Returns a pointer to the new value or NULL in case of error
 */
xmlSchemaVal * xmlSchemaNewStringValue(xmlSchemaValType type, const xmlChar * value)
{
	xmlSchemaVal * val;
	if(type != XML_SCHEMAS_STRING)
		return 0;
	val = (xmlSchemaVal *)SAlloc::M(sizeof(xmlSchemaVal));
	if(!val) {
		return 0;
	}
	memzero(val, sizeof(xmlSchemaVal));
	val->type = type;
	val->value.str = (xmlChar *)value;
	return val;
}

/**
 * xmlSchemaNewNOTATIONValue:
 * @name:  the notation name
 * @ns: the notation namespace name or NULL
 *
 * Allocate a new NOTATION value.
 * The given values are consumed and freed with the struct.
 *
 * Returns a pointer to the new value or NULL in case of error
 */
xmlSchemaVal * xmlSchemaNewNOTATIONValue(const xmlChar * name, const xmlChar * ns)
{
	xmlSchemaVal * val = xmlSchemaNewValue(XML_SCHEMAS_NOTATION);
	if(val) {
		val->value.qname.name = (xmlChar *)name;
		if(ns)
			val->value.qname.uri = (xmlChar *)ns;
	}
	return val;
}

/**
 * xmlSchemaNewQNameValue:
 * @namespaceName: the namespace name
 * @localName: the local name
 *
 * Allocate a new QName value.
 * The given values are consumed and freed with the struct.
 *
 * Returns a pointer to the new value or NULL in case of an error.
 */
xmlSchemaVal * xmlSchemaNewQNameValue(const xmlChar * namespaceName, const xmlChar * localName)
{
	xmlSchemaVal * val = xmlSchemaNewValue(XML_SCHEMAS_QNAME);
	if(val) {
		val->value.qname.name = (xmlChar *)localName;
		val->value.qname.uri = (xmlChar *)namespaceName;
	}
	return val;
}
/**
 * @value:  the value to free
 *
 * Cleanup the default XML Schemas type library
 */
void FASTCALL xmlSchemaFreeValue(xmlSchemaVal * pValue)
{
	while(pValue) {
		switch(pValue->type) {
			case XML_SCHEMAS_STRING:
			case XML_SCHEMAS_NORMSTRING:
			case XML_SCHEMAS_TOKEN:
			case XML_SCHEMAS_LANGUAGE:
			case XML_SCHEMAS_NMTOKEN:
			case XML_SCHEMAS_NMTOKENS:
			case XML_SCHEMAS_NAME:
			case XML_SCHEMAS_NCNAME:
			case XML_SCHEMAS_ID:
			case XML_SCHEMAS_IDREF:
			case XML_SCHEMAS_IDREFS:
			case XML_SCHEMAS_ENTITY:
			case XML_SCHEMAS_ENTITIES:
			case XML_SCHEMAS_ANYURI:
			case XML_SCHEMAS_ANYSIMPLETYPE:
			    SAlloc::F(pValue->value.str);
			    break;
			case XML_SCHEMAS_NOTATION:
			case XML_SCHEMAS_QNAME:
			    SAlloc::F(pValue->value.qname.uri);
			    SAlloc::F(pValue->value.qname.name);
			    break;
			case XML_SCHEMAS_HEXBINARY:
			    SAlloc::F(pValue->value.hex.str);
			    break;
			case XML_SCHEMAS_BASE64BINARY:
			    SAlloc::F(pValue->value.base64.str);
			    break;
			default:
			    break;
		}
		xmlSchemaVal * p_prev = pValue;
		pValue = pValue->next;
		SAlloc::F(p_prev);
	}
}

/**
 * xmlSchemaGetPredefinedType:
 * @name: the type name
 * @ns:  the URI of the namespace usually "http://www.w3.org/2001/XMLSchema"
 *
 * Lookup a type in the default XML Schemas type library
 *
 * Returns the type if found, NULL otherwise
 */
xmlSchemaType * xmlSchemaGetPredefinedType(const xmlChar * name, const xmlChar * ns)
{
	if(xmlSchemaTypesInitialized == 0)
		xmlSchemaInitTypes();
	return name ? (xmlSchemaType *)xmlHashLookup2(xmlSchemaTypesBank, name, ns) : 0;
}
/**
 * xmlSchemaGetBuiltInListSimpleTypeItemType:
 * @type: the built-in simple type.
 *
 * Lookup function
 *
 * Returns the item type of @type as defined by the built-in datatype
 * hierarchy of XML Schema Part 2: Datatypes, or NULL in case of an error.
 */
xmlSchemaType * xmlSchemaGetBuiltInListSimpleTypeItemType(const xmlSchemaType * type)
{
	if(!type || type->type != XML_SCHEMA_TYPE_BASIC)
		return 0;
	switch(type->builtInType) {
		case XML_SCHEMAS_NMTOKENS: return (xmlSchemaTypeNmtokenDef);
		case XML_SCHEMAS_IDREFS: return (xmlSchemaTypeIdrefDef);
		case XML_SCHEMAS_ENTITIES: return (xmlSchemaTypeEntityDef);
		default: return 0;
	}
}
//
// Convenience macros and functions
//
#define IS_TZO_CHAR(c)          ((c == 0) || (c == 'Z') || (c == '+') || (c == '-'))
#define VALID_YEAR(yr)          (yr != 0)
#define VALID_MONTH(mon)        ((mon >= 1) && (mon <= 12))
/* VALID_DAY should only be used when month is unknown */
#define VALID_DAY(day)          ((day >= 1) && (day <= 31))
#define VALID_HOUR(hr)          ((hr >= 0) && (hr <= 23))
#define VALID_MIN(min)          ((min >= 0) && (min <= 59))
#define VALID_SEC(sec)          ((sec >= 0) && (sec < 60))
#define VALID_TZO(tzo)          ((tzo > -840) && (tzo < 840))
//#define IS_LEAP(y)              (((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0))

static const uint daysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const uint daysInMonthLeap[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#define MAX_DAYINMONTH(yr, mon)      (IsLeapYear_Gregorian(yr) ? daysInMonthLeap[mon - 1] : daysInMonth[mon - 1])
#define VALID_MDAY(dt)   (IsLeapYear_Gregorian(dt->year) ? (dt->day <= daysInMonthLeap[dt->mon - 1]) : (dt->day <= daysInMonth[dt->mon - 1]))
#define VALID_DATE(dt)   (VALID_YEAR(dt->year) && VALID_MONTH(dt->mon) && VALID_MDAY(dt))
#define VALID_TIME(dt)   (VALID_HOUR(dt->hour) && VALID_MIN(dt->min) && VALID_SEC(dt->sec) && VALID_TZO(dt->tzo))
#define VALID_DATETIME(dt) (VALID_DATE(dt) && VALID_TIME(dt))
#define SECS_PER_MIN            (60)
#define SECS_PER_HOUR           (60 * SECS_PER_MIN)
#define SECS_PER_DAY            (24 * SECS_PER_HOUR)

static const long dayInYearByMonth[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
static const long dayInLeapYearByMonth[12] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

#define DAY_IN_YEAR(day, month, year) ((IsLeapYear_Gregorian(year) ? dayInLeapYearByMonth[month - 1] : dayInYearByMonth[month - 1]) + day)

#ifndef NDEBUG
#define DEBUG_DATE(dt)							\
	xmlGenericError(0, "type=%o %04ld-%02u-%02uT%02u:%02u:%03f",			    \
	    dt->type, dt->value.date.year, dt->value.date.mon, dt->value.date.day, dt->value.date.hour, dt->value.date.min, dt->value.date.sec); \
	if(dt->value.date.tz_flag)					   \
		if(dt->value.date.tzo != 0)				       \
			xmlGenericError(0, "%+05d\n", dt->value.date.tzo); \
		else								\
			xmlGenericError(0, "Z\n");		    \
	else								    \
		xmlGenericError(0, "\n")
#else
#define DEBUG_DATE(dt)
#endif

/**
 * _xmlSchemaParseGYear:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a xs:gYear without time zone and fills in the appropriate
 * field of the @dt structure. @str is updated to point just after the
 * xs:gYear. It is supposed that @dt->year is big enough to contain
 * the year.
 *
 * Returns 0 or the error code
 */
static int _xmlSchemaParseGYear(xmlSchemaValDatePtr dt, const xmlChar ** str)
{
	const xmlChar * cur = *str, * firstChar;
	int isneg = 0, digcnt = 0;
	if(((*cur < '0') || (*cur > '9')) && (*cur != '-') && (*cur != '+'))
		return -1;
	if(*cur == '-') {
		isneg = 1;
		cur++;
	}
	firstChar = cur;
	while(isdec(*cur)) {
		dt->year = dt->year * 10 + (*cur - '0');
		cur++;
		digcnt++;
	}
	/* year must be at least 4 digits (CCYY); over 4
	 * digits cannot have a leading zero. */
	if((digcnt < 4) || ((digcnt > 4) && (*firstChar == '0')))
		return 1;
	if(isneg)
		dt->year = -dt->year;
	if(!VALID_YEAR(dt->year))
		return 2;
	*str = cur;
	return 0;
}

/**
 * PARSE_2_DIGITS:
 * @num:  the integer to fill in
 * @cur:  an #xmlChar *
 * @invalid: an integer
 *
 * Parses a 2-digits integer and updates @num with the value. @cur is
 * updated to point just after the integer.
 * In case of error, @invalid is set to %TRUE, values of @num and
 * @cur are undefined.
 */
#define PARSE_2_DIGITS(num, cur, invalid)			\
	if((cur[0] < '0') || (cur[0] > '9') || (cur[1] < '0') || (cur[1] > '9')) \
		invalid = 1;					    \
	else							\
		num = (cur[0] - '0') * 10 + (cur[1] - '0');	    \
	cur += 2;

/**
 * PARSE_FLOAT:
 * @num:  the double to fill in
 * @cur:  an #xmlChar *
 * @invalid: an integer
 *
 * Parses a float and updates @num with the value. @cur is
 * updated to point just after the float. The float must have a
 * 2-digits integer part and may or may not have a decimal part.
 * In case of error, @invalid is set to %TRUE, values of @num and
 * @cur are undefined.
 */
#define PARSE_FLOAT(num, cur, invalid)				\
	PARSE_2_DIGITS(num, cur, invalid);			\
	if(!invalid && (*cur == '.')) {			       \
		double mult = 1;				    \
		cur++;						    \
		if((*cur < '0') || (*cur > '9'))		   \
			invalid = 1;					\
		while((*cur >= '0') && (*cur <= '9')) {		   \
			mult /= 10;					\
			num += (*cur - '0') * mult;			\
			cur++;						\
		}						    \
	}

/**
 * _xmlSchemaParseGMonth:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a xs:gMonth without time zone and fills in the appropriate
 * field of the @dt structure. @str is updated to point just after the
 * xs:gMonth.
 *
 * Returns 0 or the error code
 */
static int _xmlSchemaParseGMonth(xmlSchemaValDatePtr dt, const xmlChar ** str)
{
	const xmlChar * cur = *str;
	int ret = 0;
	uint value = 0;
	PARSE_2_DIGITS(value, cur, ret);
	if(ret)
		return ret;
	if(!VALID_MONTH(value))
		return 2;
	dt->mon = value;
	*str = cur;
	return 0;
}
/**
 * _xmlSchemaParseGDay:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a xs:gDay without time zone and fills in the appropriate
 * field of the @dt structure. @str is updated to point just after the
 * xs:gDay.
 *
 * Returns 0 or the error code
 */
static int _xmlSchemaParseGDay(xmlSchemaValDatePtr dt, const xmlChar ** str) 
{
	const xmlChar * cur = *str;
	int ret = 0;
	uint value = 0;
	PARSE_2_DIGITS(value, cur, ret);
	if(ret)
		return ret;
	if(!VALID_DAY(value))
		return 2;
	dt->day = value;
	*str = cur;
	return 0;
}

/**
 * _xmlSchemaParseTime:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a xs:time without time zone and fills in the appropriate
 * fields of the @dt structure. @str is updated to point just after the
 * xs:time.
 * In case of error, values of @dt fields are undefined.
 *
 * Returns 0 or the error code
 */
static int _xmlSchemaParseTime(xmlSchemaValDatePtr dt, const xmlChar ** str) 
{
	const xmlChar * cur = *str;
	int ret = 0;
	int value = 0;
	PARSE_2_DIGITS(value, cur, ret);
	if(ret)
		return ret;
	if(*cur != ':')
		return 1;
	if(!VALID_HOUR(value))
		return 2;
	cur++;

	/* the ':' insures this string is xs:time */
	dt->hour = value;

	PARSE_2_DIGITS(value, cur, ret);
	if(ret)
		return ret;
	if(!VALID_MIN(value))
		return 2;
	dt->min = value;

	if(*cur != ':')
		return 1;
	cur++;

	PARSE_FLOAT(dt->sec, cur, ret);
	if(ret)
		return ret;

	if((!VALID_SEC(dt->sec)) || (!VALID_TZO(dt->tzo)))
		return 2;

	*str = cur;
	return 0;
}

/**
 * _xmlSchemaParseTimeZone:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a time zone without time zone and fills in the appropriate
 * field of the @dt structure. @str is updated to point just after the
 * time zone.
 *
 * Returns 0 or the error code
 */
static int _xmlSchemaParseTimeZone(xmlSchemaValDatePtr dt, const xmlChar ** str) 
{
	const xmlChar * cur;
	int ret = 0;
	if(!str)
		return -1;
	cur = *str;
	switch(*cur) {
		case 0:
		    dt->tz_flag = 0;
		    dt->tzo = 0;
		    break;

		case 'Z':
		    dt->tz_flag = 1;
		    dt->tzo = 0;
		    cur++;
		    break;

		case '+':
		case '-': {
		    int isneg = 0, tmp = 0;
		    isneg = (*cur == '-');

		    cur++;

		    PARSE_2_DIGITS(tmp, cur, ret);
		    if(ret)
			    return ret;
		    if(!VALID_HOUR(tmp))
			    return 2;

		    if(*cur != ':')
			    return 1;
		    cur++;

		    dt->tzo = tmp * 60;

		    PARSE_2_DIGITS(tmp, cur, ret);
		    if(ret)
			    return ret;
		    if(!VALID_MIN(tmp))
			    return 2;

		    dt->tzo += tmp;
		    if(isneg)
			    dt->tzo = -dt->tzo;

		    if(!VALID_TZO(dt->tzo))
			    return 2;

		    dt->tz_flag = 1;
		    break;
	    }
		default:
		    return 1;
	}

	*str = cur;
	return 0;
}

/**
 * _xmlSchemaBase64Decode:
 * @ch: a character
 *
 * Converts a base64 encoded character to its base 64 value.
 *
 * Returns 0-63 (value), 64 (pad), or -1 (not recognized)
 */
static int FASTCALL _xmlSchemaBase64Decode(const xmlChar ch) 
{
	if(('A' <= ch) && (ch <= 'Z')) return ch - 'A';
	if(('a' <= ch) && (ch <= 'z')) return ch - 'a' + 26;
	if(('0' <= ch) && (ch <= '9')) return ch - '0' + 52;
	if('+' == ch) return 62;
	if('/' == ch) return 63;
	if('=' == ch) return 64;
	return -1;
}

/****************************************************************
*								*
*	XML Schema Dates/Times Datatypes Handling		*
*								*
****************************************************************/

/**
 * PARSE_DIGITS:
 * @num:  the integer to fill in
 * @cur:  an #xmlChar *
 * @num_type: an integer flag
 *
 * Parses a digits integer and updates @num with the value. @cur is
 * updated to point just after the integer.
 * In case of error, @num_type is set to -1, values of @num and
 * @cur are undefined.
 */
#define PARSE_DIGITS(num, cur, num_type)			\
	if(!isdec(*cur))		       \
		num_type = -1;					    \
	else							\
		while(isdec(*cur)) {		   \
			num = num * 10 + (*cur - '0');			\
			cur++;						\
		}

/**
 * PARSE_NUM:
 * @num:  the double to fill in
 * @cur:  an #xmlChar *
 * @num_type: an integer flag
 *
 * Parses a float or integer and updates @num with the value. @cur is
 * updated to point just after the number. If the number is a float,
 * then it must have an integer part and a decimal part; @num_type will
 * be set to 1. If there is no decimal part, @num_type is set to zero.
 * In case of error, @num_type is set to -1, values of @num and
 * @cur are undefined.
 */
#define PARSE_NUM(num, cur, num_type)				\
	num = 0;						\
	PARSE_DIGITS(num, cur, num_type);			\
	if(!num_type && (*cur == '.')) {		       \
		double mult = 1;				    \
		cur++;						    \
		if(!isdec(*cur))		   \
			num_type = -1;					\
		else						    \
			num_type = 1;					\
		while(isdec(*cur)) {		   \
			mult /= 10;					\
			num += (*cur - '0') * mult;			\
			cur++;						\
		}						    \
	}

/**
 * xmlSchemaValidateDates:
 * @type: the expected type or XML_SCHEMAS_UNKNOWN
 * @dateTime:  string to analyze
 * @val:  the return computed value
 *
 * Check that @dateTime conforms to the lexical space of one of the date types.
 * if true a value is computed and returned in @val.
 *
 * Returns 0 if this validates, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
static int xmlSchemaValidateDates(xmlSchemaValType type, const xmlChar * dateTime, xmlSchemaVal ** val, int collapse) 
{
	xmlSchemaVal * dt;
	int ret;
	const xmlChar * cur = dateTime;
#define RETURN_TYPE_IF_VALID(t)					\
	if(IS_TZO_CHAR(*cur)) {					   \
		ret = _xmlSchemaParseTimeZone(&(dt->value.date), &cur);	\
		if(!ret) {					       \
			if(*cur)					   \
				goto error;					\
			dt->type = t;					    \
			goto done;					    \
		}							\
	}
	if(dateTime == NULL)
		return -1;
	if(collapse)
		while IS_WSP_BLANK_CH(*cur) cur++;
	if((*cur != '-') && !isdec(*cur)) // @sobolev (it sees as an error) ((*cur < '0') && (*cur > '9'))-->!isdec(*cur)
		return 1;
	dt = xmlSchemaNewValue(XML_SCHEMAS_UNKNOWN);
	if(dt == NULL)
		return -1;
	if((cur[0] == '-') && (cur[1] == '-')) {
		// It's an incomplete date (xs:gMonthDay, xs:gMonth or xs:gDay)
		cur += 2;
		/* is it an xs:gDay? */
		if(*cur == '-') {
			if(type == XML_SCHEMAS_GMONTH)
				goto error;
			++cur;
			ret = _xmlSchemaParseGDay(&(dt->value.date), &cur);
			if(ret)
				goto error;
			RETURN_TYPE_IF_VALID(XML_SCHEMAS_GDAY);
			goto error;
		}
		/*
		 * it should be an xs:gMonthDay or xs:gMonth
		 */
		ret = _xmlSchemaParseGMonth(&(dt->value.date), &cur);
		if(ret)
			goto error;
		/*
		 * a '-' char could indicate this type is xs:gMonthDay or
		 * a negative time zone offset. Check for xs:gMonthDay first.
		 * Also the first three char's of a negative tzo (-MM:SS) can
		 * appear to be a valid day; so even if the day portion
		 * of the xs:gMonthDay verifies, we must insure it was not
		 * a tzo.
		 */
		if(*cur == '-') {
			const xmlChar * rewnd = cur;
			cur++;
			ret = _xmlSchemaParseGDay(&(dt->value.date), &cur);
			if((ret == 0) && ((*cur == 0) || (*cur != ':'))) {
				/*
				 * we can use the VALID_MDAY macro to validate the month
				 * and day because the leap year test will flag year zero
				 * as a leap year (even though zero is an invalid year).
				 * FUTURE TODO: Zero will become valid in XML Schema 1.1
				 * probably.
				 */
				if(VALID_MDAY((&(dt->value.date)))) {
					RETURN_TYPE_IF_VALID(XML_SCHEMAS_GMONTHDAY);
					goto error;
				}
			}
			/*
			 * not xs:gMonthDay so rewind and check if just xs:gMonth
			 * with an optional time zone.
			 */
			cur = rewnd;
		}
		RETURN_TYPE_IF_VALID(XML_SCHEMAS_GMONTH);
		goto error;
	}
	/*
	 * It's a right-truncated date or an xs:time.
	 * Try to parse an xs:time then fallback on right-truncated dates.
	 */
	if(isdec(*cur)) {
		ret = _xmlSchemaParseTime(&(dt->value.date), &cur);
		if(!ret) {
			/* it's an xs:time */
			RETURN_TYPE_IF_VALID(XML_SCHEMAS_TIME);
		}
	}
	/* fallback on date parsing */
	cur = dateTime;
	ret = _xmlSchemaParseGYear(&(dt->value.date), &cur);
	if(ret)
		goto error;
	/* is it an xs:gYear? */
	RETURN_TYPE_IF_VALID(XML_SCHEMAS_GYEAR);
	if(*cur != '-')
		goto error;
	cur++;
	ret = _xmlSchemaParseGMonth(&(dt->value.date), &cur);
	if(ret)
		goto error;
	/* is it an xs:gYearMonth? */
	RETURN_TYPE_IF_VALID(XML_SCHEMAS_GYEARMONTH);
	if(*cur != '-')
		goto error;
	cur++;
	ret = _xmlSchemaParseGDay(&(dt->value.date), &cur);
	if((ret != 0) || !VALID_DATE((&(dt->value.date))))
		goto error;
	/* is it an xs:date? */
	RETURN_TYPE_IF_VALID(XML_SCHEMAS_DATE);
	if(*cur != 'T')
		goto error;
	cur++;
	/* it should be an xs:dateTime */
	ret = _xmlSchemaParseTime(&(dt->value.date), &cur);
	if(ret)
		goto error;
	ret = _xmlSchemaParseTimeZone(&(dt->value.date), &cur);
	if(collapse)
		while IS_WSP_BLANK_CH(*cur) cur++;
	if((ret != 0) || (*cur != 0) || (!(VALID_DATETIME((&(dt->value.date))))))
		goto error;
	dt->type = XML_SCHEMAS_DATETIME;
done:
#if 1
	if((type != XML_SCHEMAS_UNKNOWN) && (type != dt->type))
		goto error;
#else
	/*
	 * insure the parsed type is equal to or less significant (right
	 * truncated) than the desired type.
	 */
	if((type != XML_SCHEMAS_UNKNOWN) && (type != dt->type)) {
		/* time only matches time */
		if((type == XML_SCHEMAS_TIME) && (dt->type == XML_SCHEMAS_TIME))
			goto error;

		if((type == XML_SCHEMAS_DATETIME) && ((dt->type != XML_SCHEMAS_DATE) || (dt->type != XML_SCHEMAS_GYEARMONTH) || (dt->type != XML_SCHEMAS_GYEAR)))
			goto error;

		if((type == XML_SCHEMAS_DATE) && ((dt->type != XML_SCHEMAS_GYEAR) || (dt->type != XML_SCHEMAS_GYEARMONTH)))
			goto error;
		if((type == XML_SCHEMAS_GYEARMONTH) && (dt->type != XML_SCHEMAS_GYEAR))
			goto error;
		if((type == XML_SCHEMAS_GMONTHDAY) && (dt->type != XML_SCHEMAS_GMONTH))
			goto error;
	}
#endif
	if(val)
		*val = dt;
	else
		xmlSchemaFreeValue(dt);
	return 0;
error:
	if(dt != NULL)
		xmlSchemaFreeValue(dt);
	return 1;
}
/**
 * xmlSchemaValidateDuration:
 * @type: the predefined type
 * @duration:  string to analyze
 * @val:  the return computed value
 *
 * Check that @duration conforms to the lexical space of the duration type.
 * if true a value is computed and returned in @val.
 *
 * Returns 0 if this validates, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
static int xmlSchemaValidateDuration(xmlSchemaType * type ATTRIBUTE_UNUSED, const xmlChar * duration, xmlSchemaVal ** val, int collapse) 
{
	const xmlChar  * cur = duration;
	xmlSchemaVal * dur;
	int isneg = 0;
	uint seq = 0;
	double num;
	int num_type = 0; /* -1 = invalid, 0 = int, 1 = floating */
	const xmlChar desig[]  = {'Y', 'M', 'D', 'H', 'M', 'S'};
	const double multi[]  = { 0.0, 0.0, 86400.0, 3600.0, 60.0, 1.0, 0.0};
	if(duration == NULL)
		return -1;
	if(collapse)
		while IS_WSP_BLANK_CH(*cur) cur++;
	if(*cur == '-') {
		isneg = 1;
		cur++;
	}
	/* duration must start with 'P' (after sign) */
	if(*cur++ != 'P')
		return 1;
	if(*cur == 0)
		return 1;
	dur = xmlSchemaNewValue(XML_SCHEMAS_DURATION);
	if(dur == NULL)
		return -1;
	while(*cur) {
		/* input string should be empty or invalid date/time item */
		if(seq >= sizeof(desig))
			goto error;

		/* T designator must be present for time items */
		if(*cur == 'T') {
			if(seq <= 3) {
				seq = 3;
				cur++;
			}
			else
				return 1;
		}
		else if(seq == 3)
			goto error;

		/* parse the number portion of the item */
		PARSE_NUM(num, cur, num_type);

		if((num_type == -1) || (*cur == 0))
			goto error;

		/* update duration based on item type */
		while(seq < sizeof(desig)) {
			if(*cur == desig[seq]) {
				/* verify numeric type; only seconds can be float */
				if((num_type != 0) && (seq < (sizeof(desig)-1)))
					goto error;

				switch(seq) {
					case 0:
					    dur->value.dur.mon = (long)num * 12;
					    break;
					case 1:
					    dur->value.dur.mon += (long)num;
					    break;
					default:
					    /* convert to seconds using multiplier */
					    dur->value.dur.sec += num * multi[seq];
					    seq++;
					    break;
				}
				break; /* exit loop */
			}
			/* no date designators found? */
			if((++seq == 3) || (seq == 6))
				goto error;
		}
		cur++;
		if(collapse)
			while IS_WSP_BLANK_CH(*cur) cur++;
	}

	if(isneg) {
		dur->value.dur.mon = -dur->value.dur.mon;
		dur->value.dur.day = -dur->value.dur.day;
		dur->value.dur.sec = -dur->value.dur.sec;
	}

	if(val)
		*val = dur;
	else
		xmlSchemaFreeValue(dur);

	return 0;

error:
	if(dur != NULL)
		xmlSchemaFreeValue(dur);
	return 1;
}

/**
 * xmlSchemaStrip:
 * @value: a value
 *
 * Removes the leading and ending spaces of a string
 *
 * Returns the new string or NULL if no change was required.
 */
static xmlChar * xmlSchemaStrip(const xmlChar * value) {
	const xmlChar * start = value, * end, * f;

	if(!value) return 0;
	while((*start != 0) && (IS_BLANK_CH(*start))) start++;
	end = start;
	while(*end != 0) end++;
	f = end;
	end--;
	while((end > start) && (IS_BLANK_CH(*end))) end--;
	end++;
	if((start == value) && (f == end)) return 0;
	return (xmlStrndup(start, end - start));
}

/**
 * xmlSchemaWhiteSpaceReplace:
 * @value: a value
 *
 * Replaces 0xd, 0x9 and 0xa with a space.
 *
 * Returns the new string or NULL if no change was required.
 */
xmlChar * xmlSchemaWhiteSpaceReplace(const xmlChar * value) 
{
	const xmlChar * cur = value;
	xmlChar * ret = NULL, * mcur;
	if(!value)
		return 0;
	while((*cur != 0) && (((*cur) != 0xd) && ((*cur) != 0x9) && ((*cur) != 0xa))) {
		cur++;
	}
	if(*cur == 0)
		return 0;
	ret = sstrdup(value);
	/* @todo FIXME: I guess gcc will bark at this. */
	mcur = (xmlChar *)(ret + (cur - value));
	do {
		if(((*mcur) == 0xd) || ((*mcur) == 0x9) || ((*mcur) == 0xa) )
			*mcur = ' ';
		mcur++;
	} while(*mcur != 0);
	return ret;
}

/**
 * xmlSchemaCollapseString:
 * @value: a value
 *
 * Removes and normalize white spaces in the string
 *
 * Returns the new string or NULL if no change was required.
 */
xmlChar * xmlSchemaCollapseString(const xmlChar * value) {
	const xmlChar * start = value, * end, * f;
	xmlChar * g;
	int col = 0;

	if(!value) return 0;
	while((*start != 0) && (IS_BLANK_CH(*start))) start++;
	end = start;
	while(*end != 0) {
		if((*end == ' ') && (IS_BLANK_CH(end[1]))) {
			col = end - start;
			break;
		}
		else if((*end == 0xa) || (*end == 0x9) || (*end == 0xd)) {
			col = end - start;
			break;
		}
		end++;
	}
	if(col == 0) {
		f = end;
		end--;
		while((end > start) && (IS_BLANK_CH(*end))) 
			end--;
		end++;
		if((start == value) && (f == end)) return 0;
		return xmlStrndup(start, end - start);
	}
	start = sstrdup(start);
	if(start == NULL) 
		return 0;
	g = (xmlChar *)(start + col);
	end = g;
	while(*end != 0) {
		if(IS_BLANK_CH(*end)) {
			end++;
			while(IS_BLANK_CH(*end)) end++;
			if(*end != 0)
				*g++ = ' ';
		}
		else
			*g++ = *end++;
	}
	*g = 0;
	return ((xmlChar *)start);
}

/**
 * xmlSchemaValAtomicListNode:
 * @type: the predefined atomic type for a token in the list
 * @value: the list value to check
 * @ret:  the return computed value
 * @node:  the node containing the value
 *
 * Check that a value conforms to the lexical space of the predefined
 * list type. if true a value is computed and returned in @ret.
 *
 * Returns the number of items if this validates, a negative error code
 *    number otherwise
 */
static int xmlSchemaValAtomicListNode(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** ret, xmlNode * pNode) 
{
	xmlChar * val, * cur, * endval;
	int nb_values = 0;
	int tmp = 0;
	if(!value) {
		return -1;
	}
	val = sstrdup(value);
	if(!val) {
		return -1;
	}
	if(ret) {
		*ret = NULL;
	}
	cur = val;
	/*
	 * Split the list
	 */
	while(IS_BLANK_CH(*cur)) *cur++ = 0;
	while(*cur) {
		if(IS_BLANK_CH(*cur)) {
			*cur = 0;
			cur++;
			while(IS_BLANK_CH(*cur)) *cur++ = 0;
		}
		else {
			nb_values++;
			cur++;
			while((*cur != 0) && (!IS_BLANK_CH(*cur))) cur++;
		}
	}
	if(nb_values == 0) {
		SAlloc::F(val);
		return (nb_values);
	}
	endval = cur;
	cur = val;
	while((*cur == 0) && (cur != endval)) cur++;
	while(cur != endval) {
		tmp = xmlSchemaValPredefTypeNode(type, cur, NULL, pNode);
		if(tmp != 0)
			break;
		while(*cur) cur++;
		while((*cur == 0) && (cur != endval)) cur++;
	}
	/* @todo what return value ? c.f. bug #158628
	   if(ret) {
	    TODO
	   } */
	SAlloc::F(val);
	if(tmp == 0)
		return (nb_values);
	return -1;
}

/**
 * xmlSchemaParseUInt:
 * @str: pointer to the string R/W
 * @llo: pointer to the low result
 * @lmi: pointer to the mid result
 * @lhi: pointer to the high result
 *
 * Parse an unsigned long into 3 fields.
 *
 * Returns the number of significant digits in the number or
 * -1 if overflow of the capacity and -2 if it's not a number.
 */
static int xmlSchemaParseUInt(const xmlChar ** str, ulong * llo, ulong * lmi, ulong * lhi) 
{
	ulong lo = 0, mi = 0, hi = 0;
	const xmlChar * tmp, * cur = *str;
	int ret = 0, i = 0;
	if(!((*cur >= '0') && (*cur <= '9')))
		return -2;
	while(*cur == '0') { /* ignore leading zeroes */
		cur++;
	}
	tmp = cur;
	while((*tmp != 0) && (*tmp >= '0') && (*tmp <= '9')) {
		i++; tmp++; ret++;
	}
	if(i > 24) {
		*str = tmp;
		return -1;
	}
	while(i > 16) {
		hi = hi * 10 + (*cur++ - '0');
		i--;
	}
	while(i > 8) {
		mi = mi * 10 + (*cur++ - '0');
		i--;
	}
	while(i > 0) {
		lo = lo * 10 + (*cur++ - '0');
		i--;
	}

	*str = cur;
	*llo = lo;
	*lmi = mi;
	*lhi = hi;
	return ret;
}

/**
 * xmlSchemaValAtomicType:
 * @type: the predefined type
 * @value: the value to check
 * @val:  the return computed value
 * @node:  the node containing the value
 * flags:  flags to control the vlidation
 *
 * Check that a value conforms to the lexical space of the atomic type.
 * if true a value is computed and returned in @val.
 * This checks the value space for list types as well (IDREFS, NMTOKENS).
 *
 * Returns 0 if this validates, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
static int xmlSchemaValAtomicType(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val, xmlNode * pNode, int flags,
    xmlSchemaWhitespaceValueType ws, int normOnTheFly, int applyNorm, int createStringValue)
{
	xmlSchemaVal * v;
	xmlChar * norm = NULL;
	int ret = 0;
	if(xmlSchemaTypesInitialized == 0)
		xmlSchemaInitTypes();
	if(type == NULL)
		return -1;
	/*
	 * validating a non existant text node is similar to validating
	 * an empty one.
	 */
	if(!value)
		value = reinterpret_cast<const xmlChar *>("");

	if(val)
		*val = NULL;
	if((flags == 0) && (value != NULL)) {
		if((type->builtInType != XML_SCHEMAS_STRING) &&
		    (type->builtInType != XML_SCHEMAS_ANYTYPE) &&
		    (type->builtInType != XML_SCHEMAS_ANYSIMPLETYPE)) {
			if(type->builtInType == XML_SCHEMAS_NORMSTRING)
				norm = xmlSchemaWhiteSpaceReplace(value);
			else
				norm = xmlSchemaCollapseString(value);
			if(norm)
				value = norm;
		}
	}

	switch(type->builtInType) {
		case XML_SCHEMAS_UNKNOWN:
		    goto error;
		case XML_SCHEMAS_ANYTYPE:
		case XML_SCHEMAS_ANYSIMPLETYPE:
		    if((createStringValue) && val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_ANYSIMPLETYPE);
			    if(v) {
				    v->value.str = sstrdup(value);
				    *val = v;
			    }
			    else {
				    goto error;
			    }
		    }
		    goto return0;
		case XML_SCHEMAS_STRING:
		    if(!normOnTheFly) {
			    const xmlChar * cur = value;

			    if(ws == XML_SCHEMA_WHITESPACE_REPLACE) {
				    while(*cur) {
					    if((*cur == 0xd) || (*cur == 0xa) || (*cur == 0x9)) {
						    goto return1;
					    }
					    else {
						    cur++;
					    }
				    }
			    }
			    else if(ws == XML_SCHEMA_WHITESPACE_COLLAPSE) {
				    while(*cur) {
					    if((*cur == 0xd) || (*cur == 0xa) || (*cur == 0x9)) {
						    goto return1;
					    }
					    else if IS_WSP_SPACE_CH(*cur) {
						    cur++;
						    if IS_WSP_SPACE_CH(*cur)
						    goto return1;
					    }
					    else {
						    cur++;
					    }
				    }
			    }
		    }
		    if(createStringValue && val) {
			    if(applyNorm) {
				    if(ws == XML_SCHEMA_WHITESPACE_COLLAPSE)
					    norm = xmlSchemaCollapseString(value);
				    else if(ws == XML_SCHEMA_WHITESPACE_REPLACE)
					    norm = xmlSchemaWhiteSpaceReplace(value);
				    if(norm)
					    value = norm;
			    }
			    v = xmlSchemaNewValue(XML_SCHEMAS_STRING);
			    if(v) {
				    v->value.str = sstrdup(value);
				    *val = v;
			    }
			    else {
				    goto error;
			    }
		    }
		    goto return0;
		case XML_SCHEMAS_NORMSTRING: {
		    if(normOnTheFly) {
			    if(applyNorm) {
				    norm = (ws == XML_SCHEMA_WHITESPACE_COLLAPSE) ? xmlSchemaCollapseString(value) : xmlSchemaWhiteSpaceReplace(value);
				    if(norm)
					    value = norm;
			    }
		    }
		    else {
			    const xmlChar * cur = value;
			    while(*cur) {
				    if((*cur == 0xd) || (*cur == 0xa) || (*cur == 0x9)) {
					    goto return1;
				    }
				    else {
					    cur++;
				    }
			    }
		    }
		    if(val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_NORMSTRING);
			    if(v) {
				    v->value.str = sstrdup(value);
				    *val = v;
			    }
			    else {
				    goto error;
			    }
		    }
		    goto return0;
	    }
		case XML_SCHEMAS_DECIMAL: {
		    const xmlChar * cur = value;
		    uint len, neg, integ, hasLeadingZeroes;
		    xmlChar cval[25];
		    xmlChar * cptr = cval;
		    if(!cur || (*cur == 0))
			    goto return1;
			// xs:decimal has a whitespace-facet value of 'collapse'.
		    if(normOnTheFly)
			    while IS_WSP_BLANK_CH(*cur) cur++;
			// First we handle an optional sign.
		    neg = 0;
		    if(*cur == '-') {
			    neg = 1;
			    cur++;
		    }
		    else if(*cur == '+')
			    cur++;
			// Disallow: "", "-", "- "
		    if(*cur == 0)
			    goto return1;
		    /*
		 * Next we "pre-parse" the number, in preparation for calling
		 * the common routine xmlSchemaParseUInt.  We get rid of any
		 * leading zeroes (because we have reserved only 25 chars),
		 * and note the position of a decimal point.
		     */
		    len = 0;
		    integ = ~0u;
		    hasLeadingZeroes = 0;
		    /*
		 * Skip leading zeroes.
		     */
		    while(*cur == '0') {
			    cur++;
			    hasLeadingZeroes = 1;
		    }
		    if(*cur) {
			    do {
				    if((*cur >= '0') && (*cur <= '9')) {
					    *cptr++ = *cur++;
					    len++;
				    }
				    else if(*cur == '.') {
					    cur++;
					    integ = len;
					    do {
						    if((*cur >= '0') && (*cur <= '9')) {
							    *cptr++ = *cur++;
							    len++;
						    }
						    else
							    break;
					    } while(len < 24);
					    /*
					 * Disallow "." but allow "00."
					     */
					    if((len == 0) && (!hasLeadingZeroes))
						    goto return1;
					    break;
				    }
				    else
					    break;
			    } while(len < 24);
		    }
		    if(normOnTheFly)
			    while IS_WSP_BLANK_CH(*cur) cur++;
		    if(*cur)
			    goto return1; /* error if any extraneous chars */
		    if(val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_DECIMAL);
			    if(v) {
				    /*
				 * Now evaluate the significant digits of the number
				     */
				    if(len != 0) {
					    if(integ != ~0u) {
						    /*
						 * Get rid of trailing zeroes in the
						 * fractional part.
						     */
						    while((len != integ) && (*(cptr-1) == '0')) {
							    cptr--;
							    len--;
						    }
					    }
					    /*
					 * Terminate the (preparsed) string.
					     */
					    if(len != 0) {
						    *cptr = 0;
						    cptr = cval;

						    xmlSchemaParseUInt((const xmlChar **)&cptr,
							    &v->value.decimal.lo,
							    &v->value.decimal.mi,
							    &v->value.decimal.hi);
					    }
				    }
				    /*
				 * Set the total digits to 1 if a zero value.
				     */
				    v->value.decimal.sign = neg;
				    if(!len) {
					    /* Speedup for zero values. */
					    v->value.decimal.total = 1;
				    }
				    else {
					    v->value.decimal.total = len;
					    if(integ == ~0u)
						    v->value.decimal.frac = 0;
					    else
						    v->value.decimal.frac = len - integ;
				    }
				    *val = v;
			    }
		    }
		    goto return0;
	    }
		case XML_SCHEMAS_TIME:
		case XML_SCHEMAS_GDAY:
		case XML_SCHEMAS_GMONTH:
		case XML_SCHEMAS_GMONTHDAY:
		case XML_SCHEMAS_GYEAR:
		case XML_SCHEMAS_GYEARMONTH:
		case XML_SCHEMAS_DATE:
		case XML_SCHEMAS_DATETIME:
		    ret = xmlSchemaValidateDates(type->builtInType, value, val,
		    normOnTheFly);
		    break;
		case XML_SCHEMAS_DURATION:
		    ret = xmlSchemaValidateDuration(type, value, val,
		    normOnTheFly);
		    break;
		case XML_SCHEMAS_FLOAT:
		case XML_SCHEMAS_DOUBLE: {
		    const xmlChar * cur = value;
		    int neg = 0;
		    int digits_before = 0;
		    int digits_after = 0;
		    if(normOnTheFly)
			    while IS_WSP_BLANK_CH(*cur) cur++;

		    if((cur[0] == 'N') && (cur[1] == 'a') && (cur[2] == 'N')) {
			    cur += 3;
			    if(*cur)
				    goto return1;
			    if(val) {
				    if(type == xmlSchemaTypeFloatDef) {
					    v = xmlSchemaNewValue(XML_SCHEMAS_FLOAT);
					    if(v) {
						    v->value.f = (float)xmlXPathNAN;
					    }
					    else {
						    xmlSchemaFreeValue(v);
						    goto error;
					    }
				    }
				    else {
					    v = xmlSchemaNewValue(XML_SCHEMAS_DOUBLE);
					    if(v) {
						    v->value.d = xmlXPathNAN;
					    }
					    else {
						    xmlSchemaFreeValue(v);
						    goto error;
					    }
				    }
				    *val = v;
			    }
			    goto return0;
		    }
		    if(*cur == '-') {
			    neg = 1;
			    cur++;
		    }
		    if((cur[0] == 'I') && (cur[1] == 'N') && (cur[2] == 'F')) {
			    cur += 3;
			    if(*cur)
				    goto return1;
			    if(val) {
				    if(type == xmlSchemaTypeFloatDef) {
					    v = xmlSchemaNewValue(XML_SCHEMAS_FLOAT);
					    if(v) {
						    v->value.f = neg ? (float)xmlXPathNINF : (float)xmlXPathPINF;
					    }
					    else {
						    xmlSchemaFreeValue(v);
						    goto error;
					    }
				    }
				    else {
					    v = xmlSchemaNewValue(XML_SCHEMAS_DOUBLE);
					    if(v) {
						    v->value.d = neg ? xmlXPathNINF : xmlXPathPINF;
					    }
					    else {
						    xmlSchemaFreeValue(v);
						    goto error;
					    }
				    }
				    *val = v;
			    }
			    goto return0;
		    }
		    if((neg == 0) && (*cur == '+'))
			    cur++;
		    if(oneof3(cur[0], 0, '+', '-'))
			    goto return1;
		    while((*cur >= '0') && (*cur <= '9')) {
			    cur++;
			    digits_before++;
		    }
		    if(*cur == '.') {
			    cur++;
			    while((*cur >= '0') && (*cur <= '9')) {
				    cur++;
				    digits_after++;
			    }
		    }
		    if((digits_before == 0) && (digits_after == 0))
			    goto return1;
		    if((*cur == 'e') || (*cur == 'E')) {
			    cur++;
			    if((*cur == '-') || (*cur == '+'))
				    cur++;
			    while((*cur >= '0') && (*cur <= '9'))
				    cur++;
		    }
		    if(normOnTheFly)
			    while IS_WSP_BLANK_CH(*cur) cur++;

		    if(*cur)
			    goto return1;
		    if(val) {
			    if(type == xmlSchemaTypeFloatDef) {
				    v = xmlSchemaNewValue(XML_SCHEMAS_FLOAT);
				    if(v) {
					    /*
					 * @todo sscanf seems not to give the correct
					 * value for extremely high/low values.
					 * E.g. "1E-149" results in zero.
					     */
					    if(sscanf((const char *)value, "%f",
							    &(v->value.f)) == 1) {
						    *val = v;
					    }
					    else {
						    xmlSchemaFreeValue(v);
						    goto return1;
					    }
				    }
				    else {
					    goto error;
				    }
			    }
			    else {
				    v = xmlSchemaNewValue(XML_SCHEMAS_DOUBLE);
				    if(v) {
					    /*
					 * @todo sscanf seems not to give the correct
					 * value for extremely high/low values.
					     */
					    if(sscanf((const char *)value, "%lf",
							    &(v->value.d)) == 1) {
						    *val = v;
					    }
					    else {
						    xmlSchemaFreeValue(v);
						    goto return1;
					    }
				    }
				    else {
					    goto error;
				    }
			    }
		    }
		    goto return0;
	    }
		case XML_SCHEMAS_BOOLEAN: {
		    const xmlChar * cur = value;
		    if(normOnTheFly) {
			    while IS_WSP_BLANK_CH(*cur) cur++;
			    if(*cur == '0') {
				    ret = 0;
				    cur++;
			    }
			    else if(*cur == '1') {
				    ret = 1;
				    cur++;
			    }
			    else if(*cur == 't') {
				    cur++;
				    if((*cur++ == 'r') && (*cur++ == 'u') && (*cur++ == 'e')) {
					    ret = 1;
				    }
				    else
					    goto return1;
			    }
			    else if(*cur == 'f') {
				    cur++;
				    if((*cur++ == 'a') && (*cur++ == 'l') && (*cur++ == 's') && (*cur++ == 'e')) {
					    ret = 0;
				    }
				    else
					    goto return1;
			    }
			    else
				    goto return1;
			    if(*cur) {
				    while IS_WSP_BLANK_CH(*cur) cur++;
				    if(*cur)
					    goto return1;
			    }
		    }
		    else {
			    if((cur[0] == '0') && (cur[1] == 0))
				    ret = 0;
			    else if((cur[0] == '1') && (cur[1] == 0))
				    ret = 1;
			    else if((cur[0] == 't') && (cur[1] == 'r') && (cur[2] == 'u') && (cur[3] == 'e') && (cur[4] == 0))
				    ret = 1;
			    else if((cur[0] == 'f') && (cur[1] == 'a') && (cur[2] == 'l') && (cur[3] == 's') && (cur[4] == 'e') && (cur[5] == 0))
				    ret = 0;
			    else
				    goto return1;
		    }
		    if(val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_BOOLEAN);
			    if(v) {
				    v->value.b = ret;
				    *val = v;
			    }
			    else {
				    goto error;
			    }
		    }
		    goto return0;
	    }
		case XML_SCHEMAS_TOKEN: {
		    const xmlChar * cur = value;
		    if(!normOnTheFly) {
			    while(*cur) {
				    if((*cur == 0xd) || (*cur == 0xa) || (*cur == 0x9)) {
					    goto return1;
				    }
				    else if(*cur == ' ') {
					    cur++;
					    if(*cur == 0)
						    goto return1;
					    if(*cur == ' ')
						    goto return1;
				    }
				    else {
					    cur++;
				    }
			    }
		    }
		    if(val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_TOKEN);
			    if(v) {
				    v->value.str = sstrdup(value);
				    *val = v;
			    }
			    else {
				    goto error;
			    }
		    }
		    goto return0;
	    }
		case XML_SCHEMAS_LANGUAGE:
		    if(normOnTheFly) {
			    norm = xmlSchemaCollapseString(value);
			    if(norm)
				    value = norm;
		    }
		    if(xmlCheckLanguageID(value) == 1) {
			    if(val) {
				    v = xmlSchemaNewValue(XML_SCHEMAS_LANGUAGE);
				    if(v) {
					    v->value.str = sstrdup(value);
					    *val = v;
				    }
				    else {
					    goto error;
				    }
			    }
			    goto return0;
		    }
		    goto return1;
		case XML_SCHEMAS_NMTOKEN:
		    if(xmlValidateNMToken(value, 1) == 0) {
			    if(val) {
				    v = xmlSchemaNewValue(XML_SCHEMAS_NMTOKEN);
				    if(v) {
					    v->value.str = sstrdup(value);
					    *val = v;
				    }
				    else {
					    goto error;
				    }
			    }
			    goto return0;
		    }
		    goto return1;
		case XML_SCHEMAS_NMTOKENS:
		    ret = xmlSchemaValAtomicListNode(xmlSchemaTypeNmtokenDef, value, val, pNode);
		    if(ret > 0)
			    ret = 0;
		    else
			    ret = 1;
		    goto done;
		case XML_SCHEMAS_NAME:
		    ret = xmlValidateName(value, 1);
		    if((ret == 0) && val && value) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_NAME);
			    if(v) {
				    const xmlChar * start = value, * end;
				    while(IS_BLANK_CH(*start)) 
						start++;
				    end = start;
				    while((*end != 0) && (!IS_BLANK_CH(*end))) 
						end++;
				    v->value.str = xmlStrndup(start, end - start);
				    *val = v;
			    }
			    else {
				    goto error;
			    }
		    }
		    goto done;
		case XML_SCHEMAS_QNAME: {
		    const xmlChar * uri = NULL;
		    xmlChar * local = NULL;
		    ret = xmlValidateQName(value, 1);
		    if(ret)
			    goto done;
		    if(pNode) {
			    xmlChar * prefix;
			    xmlNs * ns;
			    local = xmlSplitQName2(value, &prefix);
			    ns = xmlSearchNs(pNode->doc, pNode, prefix);
			    if(!ns && prefix) {
				    SAlloc::F(prefix);
				    SAlloc::F(local);
				    goto return1;
			    }
			    if(ns)
				    uri = ns->href;
			    SAlloc::F(prefix);
		    }
		    if(val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_QNAME);
			    if(!v) {
				    SAlloc::F(local);
				    goto error;
			    }
			    if(local != NULL)
				    v->value.qname.name = local;
			    else
				    v->value.qname.name = sstrdup(value);
			    if(uri)
				    v->value.qname.uri = sstrdup(uri);
			    *val = v;
		    }
		    else
			    SAlloc::F(local);
		    goto done;
	    }
		case XML_SCHEMAS_NCNAME:
		    ret = xmlValidateNCName(value, 1);
		    if((ret == 0) && val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_NCNAME);
			    if(v) {
				    v->value.str = sstrdup(value);
				    *val = v;
			    }
			    else {
				    goto error;
			    }
		    }
		    goto done;
		case XML_SCHEMAS_ID:
		    ret = xmlValidateNCName(value, 1);
		    if((ret == 0) && val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_ID);
			    if(v) {
				    v->value.str = sstrdup(value);
				    *val = v;
			    }
			    else {
				    goto error;
			    }
		    }
		    if((ret == 0) && pNode && (pNode->type == XML_ATTRIBUTE_NODE)) {
			    xmlAttr * attr = reinterpret_cast<xmlAttr *>(pNode);
				// NOTE: the IDness might have already be declared in the DTD
			    if(attr->atype != XML_ATTRIBUTE_ID) {
				    xmlID * res;
				    xmlChar * strip = xmlSchemaStrip(value);
				    if(strip) {
					    res = xmlAddID(NULL, pNode->doc, strip, attr);
					    SAlloc::F(strip);
				    }
				    else
					    res = xmlAddID(NULL, pNode->doc, value, attr);
				    if(!res)
					    ret = 2;
				    else
					    attr->atype = XML_ATTRIBUTE_ID;
			    }
		    }
		    goto done;
		case XML_SCHEMAS_IDREF:
		    ret = xmlValidateNCName(value, 1);
		    if((ret == 0) && val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_IDREF);
			    if(!v)
				    goto error;
			    v->value.str = sstrdup(value);
			    *val = v;
		    }
		    if((ret == 0) && pNode && (pNode->type == XML_ATTRIBUTE_NODE)) {
			    xmlAttr * attr = reinterpret_cast<xmlAttr *>(pNode);
			    xmlChar * strip = xmlSchemaStrip(value);
			    if(strip) {
				    xmlAddRef(NULL, pNode->doc, strip, attr);
				    SAlloc::F(strip);
			    }
			    else
				    xmlAddRef(NULL, pNode->doc, value, attr);
			    attr->atype = XML_ATTRIBUTE_IDREF;
		    }
		    goto done;
		case XML_SCHEMAS_IDREFS:
		    ret = xmlSchemaValAtomicListNode(xmlSchemaTypeIdrefDef, value, val, pNode);
		    if(ret < 0)
			    ret = 2;
		    else
			    ret = 0;
		    if((ret == 0) && pNode && (pNode->type == XML_ATTRIBUTE_NODE)) {
			    xmlAttr * attr = reinterpret_cast<xmlAttr *>(pNode);
			    attr->atype = XML_ATTRIBUTE_IDREFS;
		    }
		    goto done;
		case XML_SCHEMAS_ENTITY: {
		    xmlChar * strip;
		    ret = xmlValidateNCName(value, 1);
		    if(!pNode || (pNode->doc == NULL))
			    ret = 3;
		    if(!ret) {
			    xmlEntity * ent;
			    strip = xmlSchemaStrip(value);
			    if(strip) {
				    ent = xmlGetDocEntity(pNode->doc, strip);
				    SAlloc::F(strip);
			    }
			    else
				    ent = xmlGetDocEntity(pNode->doc, value);
			    if(!ent || (ent->etype != XML_EXTERNAL_GENERAL_UNPARSED_ENTITY))
				    ret = 4;
		    }
		    if((ret == 0) && val) {
			    TODO;
		    }
		    if(ret == 0 && pNode && pNode->type == XML_ATTRIBUTE_NODE) {
			    xmlAttr * attr = reinterpret_cast<xmlAttr *>(pNode);
			    attr->atype = XML_ATTRIBUTE_ENTITY;
		    }
		    goto done;
	    }
		case XML_SCHEMAS_ENTITIES:
		    if(!pNode || !pNode->doc)
			    goto return3;
		    ret = xmlSchemaValAtomicListNode(xmlSchemaTypeEntityDef, value, val, pNode);
		    if(ret <= 0)
			    ret = 1;
		    else
			    ret = 0;
		    if(ret == 0 && pNode && pNode->type == XML_ATTRIBUTE_NODE) {
			    xmlAttr * attr = reinterpret_cast<xmlAttr *>(pNode);
			    attr->atype = XML_ATTRIBUTE_ENTITIES;
		    }
		    goto done;
		case XML_SCHEMAS_NOTATION: {
		    xmlChar * uri = NULL;
		    xmlChar * local = NULL;
		    ret = xmlValidateQName(value, 1);
		    if((ret == 0) && pNode) {
			    xmlChar * prefix;
			    local = xmlSplitQName2(value, &prefix);
			    if(prefix) {
				    xmlNs * ns = xmlSearchNs(pNode->doc, pNode, prefix);
				    if(!ns)
					    ret = 1;
				    else if(val)
					    uri = sstrdup(ns->href);
			    }
			    if(local && ((val == NULL) || (ret != 0)))
				    SAlloc::F(local);
			    SAlloc::F(prefix);
		    }
		    if(!pNode || (pNode->doc == NULL))
			    ret = 3;
		    if(!ret) {
			    ret = xmlValidateNotationUse(NULL, pNode->doc, value);
			    ret = (ret == 1) ? 0 : 1;
		    }
		    if((ret == 0) && val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_NOTATION);
			    if(v) {
				    v->value.qname.name = local ? local : sstrdup(value);
				    if(uri)
					    v->value.qname.uri = uri;
				    *val = v;
			    }
			    else {
				    SAlloc::F(local);
				    SAlloc::F(uri);
				    goto error;
			    }
		    }
		    goto done;
	    }
		case XML_SCHEMAS_ANYURI: {
		    if(*value != 0) {
			    xmlURI * uri;
			    xmlChar * tmpval, * cur;
			    if(normOnTheFly) {
				    norm = xmlSchemaCollapseString(value);
				    if(norm)
					    value = norm;
			    }
			    tmpval = sstrdup(value);
			    for(cur = tmpval; *cur; ++cur) {
				    if(*cur < 32 || *cur >= 127 || *cur == ' ' || *cur == '<' || *cur == '>' || *cur == '"' ||
					    *cur == '{' || *cur == '}' || *cur == '|' || *cur == '\\' || *cur == '^' || *cur == '`' || *cur == '\'')
					    *cur = '_';
			    }
			    uri = xmlParseURI((const char *)tmpval);
			    SAlloc::F(tmpval);
			    if(!uri)
				    goto return1;
			    xmlFreeURI(uri);
		    }
		    if(val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_ANYURI);
			    if(!v)
				    goto error;
			    v->value.str = sstrdup(value);
			    *val = v;
		    }
		    goto return0;
	    }
		case XML_SCHEMAS_HEXBINARY: {
		    const xmlChar * cur = value, * start;
		    xmlChar * base;
		    int total, i = 0;
		    if(!cur)
			    goto return1;
		    if(normOnTheFly)
			    while IS_WSP_BLANK_CH(*cur) 
					cur++;
		    start = cur;
		    while(((*cur >= '0') && (*cur <= '9')) || ((*cur >= 'A') && (*cur <= 'F')) || ((*cur >= 'a') && (*cur <= 'f'))) {
			    i++;
			    cur++;
		    }
		    if(normOnTheFly)
			    while IS_WSP_BLANK_CH(*cur) 
					cur++;
		    if(*cur)
			    goto return1;
		    if((i % 2) != 0)
			    goto return1;
		    if(val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_HEXBINARY);
			    if(!v)
				    goto error;
			    /*
			 * Copy only the normalized piece.
			 * CRITICAL TODO: Check this.
			     */
			    cur = xmlStrndup(start, i);
			    if(!cur) {
				    xmlSchemaTypeErrMemory(pNode, "allocating hexbin data");
				    SAlloc::F(v);
				    goto return1;
			    }
			    total = i / 2; /* number of octets */
			    base = (xmlChar *)cur;
			    while(i-- > 0) {
				    if(*base >= 'a')
					    *base = *base - ('a' - 'A');
				    base++;
			    }
			    v->value.hex.str = (xmlChar *)cur;
			    v->value.hex.total = total;
			    *val = v;
		    }
		    goto return0;
	    }
		case XML_SCHEMAS_BASE64BINARY: {
		    /* ISSUE:
		     *
		 * Ignore all stray characters? (yes, currently)
		 * Worry about long lines? (no, currently)
		     *
		 * rfc2045.txt:
		     *
		 * "The encoded output stream must be represented in lines of
		 * no more than 76 characters each.  All line breaks or other
		 * characters not found in Table 1 must be ignored by decoding
		 * software.  In base64 data, characters other than those in
		 * Table 1, line breaks, and other white space probably
		 * indicate a transmission error, about which a warning
		 * message or even a message rejection might be appropriate
		 * under some circumstances." */
		    const xmlChar * cur = value;
		    xmlChar * base;
		    int total, i = 0, pad = 0;
		    if(!cur)
			    goto return1;
		    for(; *cur; ++cur) {
			    int decc = _xmlSchemaBase64Decode(*cur);
			    if(decc < 0) 
					;
			    else if(decc < 64)
				    i++;
			    else
				    break;
		    }
		    for(; *cur; ++cur) {
			    int decc = _xmlSchemaBase64Decode(*cur);
			    if(decc < 0) 
					;
			    else if(decc < 64)
				    goto return1;
			    if(decc == 64)
				    pad++;
		    }
		    // rfc2045.txt: "Special processing is performed if fewer than 24 bits are available at the end of the data being encoded.
		    // A full encoding quantum is always completed at the end of a body.  When fewer than 24 input bits are available in an
		    // input group, zero bits are added (on the right) to form an integral number of 6-bit groups.  Padding at the end of the
		    // data is performed using the "=" character.  Since all base64 input is an integral number of octets, only the
		    // following cases can arise: (1) the final quantum of encoding input is an integral multiple of 24 bits; here,
		    // the final unit of encoded output will be an integral multiple ofindent: Standard input:701: Warning:old style
		    // assignment ambiguity in "=*".  Assuming "= *" 4 characters with no "=" padding, (2) the final
		    // quantum of encoding input is exactly 8 bits; here, the final unit of encoded output will be two characters
		    // followed by two "=" padding characters, or (3) the final quantum of encoding input is exactly 16 bits; here, the
		    // final unit of encoded output will be three characters followed by one "=" padding character."
		    // 
		    total = 3 * (i / 4);
		    if(pad == 0) {
			    if(i % 4 != 0)
				    goto return1;
		    }
		    else if(pad == 1) {
			    int decc;
			    if(i % 4 != 3)
				    goto return1;
			    for(decc = _xmlSchemaBase64Decode(*cur); (decc < 0) || (decc > 63); decc = _xmlSchemaBase64Decode(*cur))
				    --cur;
			    /* 16bits in 24bits means 2 pad bits: nnnnnn nnmmmm mmmm00*/
			    /* 00111100 -> 0x3c */
			    if(decc & ~0x3c)
				    goto return1;
			    total += 2;
		    }
		    else if(pad == 2) {
			    int decc;
			    if(i % 4 != 2)
				    goto return1;
			    for(decc = _xmlSchemaBase64Decode(*cur); (decc < 0) || (decc > 63); decc = _xmlSchemaBase64Decode(*cur))
				    --cur;
			    /* 8bits in 12bits means 4 pad bits: nnnnnn nn0000 */
			    /* 00110000 -> 0x30 */
			    if(decc & ~0x30)
				    goto return1;
			    total += 1;
		    }
		    else
			    goto return1;

		    if(val) {
			    v = xmlSchemaNewValue(XML_SCHEMAS_BASE64BINARY);
			    if(!v)
				    goto error;
			    base = static_cast<xmlChar *>(SAlloc::M((i + pad + 1) * sizeof(xmlChar)));
			    if(base == NULL) {
				    xmlSchemaTypeErrMemory(pNode, "allocating base64 data");
				    SAlloc::F(v);
				    goto return1;
			    }
			    v->value.base64.str = base;
			    for(cur = value; *cur; ++cur)
				    if(_xmlSchemaBase64Decode(*cur) >= 0) {
					    *base = *cur;
					    ++base;
				    }
			    *base = 0;
			    v->value.base64.total = total;
			    *val = v;
		    }
		    goto return0;
	    }
		case XML_SCHEMAS_INTEGER:
		case XML_SCHEMAS_PINTEGER:
		case XML_SCHEMAS_NPINTEGER:
		case XML_SCHEMAS_NINTEGER:
		case XML_SCHEMAS_NNINTEGER: {
		    const xmlChar * cur = value;
		    ulong lo, mi, hi;
		    int sign = 0;
		    if(!cur)
			    goto return1;
		    if(normOnTheFly)
			    while IS_WSP_BLANK_CH(*cur) cur++;
		    if(*cur == '-') {
			    sign = 1;
			    cur++;
		    }
		    else if(*cur == '+')
			    cur++;
		    ret = xmlSchemaParseUInt(&cur, &lo, &mi, &hi);
		    if(ret < 0)
			    goto return1;
		    if(normOnTheFly)
			    while IS_WSP_BLANK_CH(*cur) cur++;
		    if(*cur)
			    goto return1;
		    if(type->builtInType == XML_SCHEMAS_NPINTEGER) {
			    if((sign == 0) && ((hi != 0) || (mi != 0) || (lo != 0)))
				    goto return1;
		    }
		    else if(type->builtInType == XML_SCHEMAS_PINTEGER) {
			    if(sign == 1)
				    goto return1;
			    if((hi == 0) && (mi == 0) && (lo == 0))
				    goto return1;
		    }
		    else if(type->builtInType == XML_SCHEMAS_NINTEGER) {
			    if(sign == 0)
				    goto return1;
			    if((hi == 0) && (mi == 0) && (lo == 0))
				    goto return1;
		    }
		    else if(type->builtInType == XML_SCHEMAS_NNINTEGER) {
			    if((sign == 1) && ((hi != 0) || (mi != 0) || (lo != 0)))
				    goto return1;
		    }
		    if(val) {
			    v = xmlSchemaNewValue(type->builtInType);
			    if(v) {
				    if(!ret)
					    ret++;
				    v->value.decimal.lo = lo;
				    v->value.decimal.mi = mi;
				    v->value.decimal.hi = hi;
				    v->value.decimal.sign = sign;
				    v->value.decimal.frac = 0;
				    v->value.decimal.total = ret;
				    *val = v;
			    }
		    }
		    goto return0;
	    }
		case XML_SCHEMAS_LONG:
		case XML_SCHEMAS_BYTE:
		case XML_SCHEMAS_SHORT:
		case XML_SCHEMAS_INT: {
		    const xmlChar * cur = value;
		    ulong lo, mi, hi;
		    int sign = 0;
		    if(!cur)
			    goto return1;
		    if(*cur == '-') {
			    sign = 1;
			    cur++;
		    }
		    else if(*cur == '+')
			    cur++;
		    ret = xmlSchemaParseUInt(&cur, &lo, &mi, &hi);
		    if(ret < 0)
			    goto return1;
		    if(*cur)
			    goto return1;
		    if(type->builtInType == XML_SCHEMAS_LONG) {
			    if(hi >= 922) {
				    if(hi > 922)
					    goto return1;
				    if(mi >= 33720368) {
					    if(mi > 33720368)
						    goto return1;
					    if((sign == 0) && (lo > 54775807))
						    goto return1;
					    if((sign == 1) && (lo > 54775808))
						    goto return1;
				    }
			    }
		    }
		    else if(type->builtInType == XML_SCHEMAS_INT) {
			    if(hi != 0)
				    goto return1;
			    if(mi >= 21) {
				    if(mi > 21)
					    goto return1;
				    if((sign == 0) && (lo > 47483647))
					    goto return1;
				    if((sign == 1) && (lo > 47483648))
					    goto return1;
			    }
		    }
		    else if(type->builtInType == XML_SCHEMAS_SHORT) {
			    if((mi != 0) || (hi != 0))
				    goto return1;
			    if((sign == 1) && (lo > 32768))
				    goto return1;
			    if((sign == 0) && (lo > 32767))
				    goto return1;
		    }
		    else if(type->builtInType == XML_SCHEMAS_BYTE) {
			    if((mi != 0) || (hi != 0))
				    goto return1;
			    if((sign == 1) && (lo > 128))
				    goto return1;
			    if((sign == 0) && (lo > 127))
				    goto return1;
		    }
		    if(val) {
			    v = xmlSchemaNewValue(type->builtInType);
			    if(v) {
				    v->value.decimal.lo = lo;
				    v->value.decimal.mi = mi;
				    v->value.decimal.hi = hi;
				    v->value.decimal.sign = sign;
				    v->value.decimal.frac = 0;
				    v->value.decimal.total = ret;
				    *val = v;
			    }
		    }
		    goto return0;
	    }
		case XML_SCHEMAS_UINT:
		case XML_SCHEMAS_ULONG:
		case XML_SCHEMAS_USHORT:
		case XML_SCHEMAS_UBYTE: {
		    const xmlChar * cur = value;
		    ulong lo, mi, hi;
		    if(!cur)
			    goto return1;
		    ret = xmlSchemaParseUInt(&cur, &lo, &mi, &hi);
		    if(ret < 0)
			    goto return1;
		    if(*cur)
			    goto return1;
		    if(type->builtInType == XML_SCHEMAS_ULONG) {
			    if(hi >= 1844) {
				    if(hi > 1844)
					    goto return1;
				    if(mi >= 67440737) {
					    if(mi > 67440737)
						    goto return1;
					    if(lo > 9551615)
						    goto return1;
				    }
			    }
		    }
		    else if(type->builtInType == XML_SCHEMAS_UINT) {
			    if(hi != 0)
				    goto return1;
			    if(mi >= 42) {
				    if(mi > 42)
					    goto return1;
				    if(lo > 94967295)
					    goto return1;
			    }
		    }
		    else if(type->builtInType == XML_SCHEMAS_USHORT) {
			    if((mi != 0) || (hi != 0))
				    goto return1;
			    if(lo > 65535)
				    goto return1;
		    }
		    else if(type->builtInType == XML_SCHEMAS_UBYTE) {
			    if((mi != 0) || (hi != 0))
				    goto return1;
			    if(lo > 255)
				    goto return1;
		    }
		    if(val) {
			    v = xmlSchemaNewValue(type->builtInType);
			    if(v) {
				    v->value.decimal.lo = lo;
				    v->value.decimal.mi = mi;
				    v->value.decimal.hi = hi;
				    v->value.decimal.sign = 0;
				    v->value.decimal.frac = 0;
				    v->value.decimal.total = ret;
				    *val = v;
			    }
		    }
		    goto return0;
	    }
	}
done:
	SAlloc::F(norm);
	return ret;
return3:
	SAlloc::F(norm);
	return (3);
return1:
	SAlloc::F(norm);
	return 1;
return0:
	SAlloc::F(norm);
	return 0;
error:
	SAlloc::F(norm);
	return -1;
}
/**
 * xmlSchemaValPredefTypeNode:
 * @type: the predefined type
 * @value: the value to check
 * @val:  the return computed value
 * @node:  the node containing the value
 *
 * Check that a value conforms to the lexical space of the predefined type.
 * if true a value is computed and returned in @val.
 *
 * Returns 0 if this validates, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int xmlSchemaValPredefTypeNode(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val, xmlNode * pNode) 
{
	return (xmlSchemaValAtomicType(type, value, val, pNode, 0, XML_SCHEMA_WHITESPACE_UNKNOWN, 1, 1, 0));
}
/**
 * xmlSchemaValPredefTypeNodeNoNorm:
 * @type: the predefined type
 * @value: the value to check
 * @val:  the return computed value
 * @node:  the node containing the value
 *
 * Check that a value conforms to the lexical space of the predefined type.
 * if true a value is computed and returned in @val.
 * This one does apply any normalization to the value.
 *
 * Returns 0 if this validates, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int xmlSchemaValPredefTypeNodeNoNorm(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val, xmlNode * pNode) 
{
	return (xmlSchemaValAtomicType(type, value, val, pNode, 1, XML_SCHEMA_WHITESPACE_UNKNOWN, 1, 0, 1));
}
/**
 * xmlSchemaValidatePredefinedType:
 * @type: the predefined type
 * @value: the value to check
 * @val:  the return computed value
 *
 * Check that a value conforms to the lexical space of the predefined type.
 * if true a value is computed and returned in @val.
 *
 * Returns 0 if this validates, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int xmlSchemaValidatePredefinedType(xmlSchemaType * type, const xmlChar * value, xmlSchemaVal ** val) 
{
	return (xmlSchemaValPredefTypeNode(type, value, val, NULL));
}
/**
 * xmlSchemaCompareDecimals:
 * @x:  a first decimal value
 * @y:  a second decimal value
 *
 * Compare 2 decimals
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y and -2 in case of error
 */
static int xmlSchemaCompareDecimals(xmlSchemaVal * x, xmlSchemaVal * y)
{
	xmlSchemaVal * swp;
	int order = 1, integx, integy, dlen;
	ulong hi, mi, lo;
	/*
	 * First test: If x is -ve and not zero
	 */
	if((x->value.decimal.sign) &&
	    ((x->value.decimal.lo != 0) ||
		    (x->value.decimal.mi != 0) ||
		    (x->value.decimal.hi != 0))) {
		/*
		 * Then if y is -ve and not zero reverse the compare
		 */
		if((y->value.decimal.sign) &&
		    ((y->value.decimal.lo != 0) ||
			    (y->value.decimal.mi != 0) ||
			    (y->value.decimal.hi != 0)))
			order = -1;
		/*
		 * Otherwise (y >= 0) we have the answer
		 */
		else
			return -1;
		/*
		 * If x is not -ve and y is -ve we have the answer
		 */
	}
	else if((y->value.decimal.sign) &&
	    ((y->value.decimal.lo != 0) ||
		    (y->value.decimal.mi != 0) ||
		    (y->value.decimal.hi != 0))) {
		return 1;
	}
	/*
	 * If it's not simply determined by a difference in sign,
	 * then we need to compare the actual values of the two nums.
	 * To do this, we start by looking at the integral parts.
	 * If the number of integral digits differ, then we have our
	 * answer.
	 */
	integx = x->value.decimal.total - x->value.decimal.frac;
	integy = y->value.decimal.total - y->value.decimal.frac;
	/*
	 * NOTE: We changed the "total" for values like "0.1"
	 * (or "-0.1" or ".1") to be 1, which was 2 previously.
	 * Therefore the special case, when such values are
	 * compared with 0, needs to be handled separately;
	 * otherwise a zero would be recognized incorrectly as
	 * greater than those values. This has the nice side effect
	 * that we gain an overall optimized comparison with zeroes.
	 * Note that a "0" has a "total" of 1 already.
	 */
	if(integx == 1) {
		if(x->value.decimal.lo == 0) {
			if(integy != 1)
				return -order;
			else if(y->value.decimal.lo != 0)
				return -order;
			else
				return 0;
		}
	}
	if(integy == 1) {
		if(y->value.decimal.lo == 0) {
			if(integx != 1)
				return order;
			else if(x->value.decimal.lo != 0)
				return order;
			else
				return 0;
		}
	}

	if(integx > integy)
		return order;
	else if(integy > integx)
		return -order;

	/*
	 * If the number of integral digits is the same for both numbers,
	 * then things get a little more complicated.  We need to "normalize"
	 * the numbers in order to properly compare them.  To do this, we
	 * look at the total length of each number (length => number of
	 * significant digits), and divide the "shorter" by 10 (decreasing
	 * the length) until they are of equal length.
	 */
	dlen = x->value.decimal.total - y->value.decimal.total;
	if(dlen < 0) {  /* y has more digits than x */
		swp = x;
		hi = y->value.decimal.hi;
		mi = y->value.decimal.mi;
		lo = y->value.decimal.lo;
		dlen = -dlen;
		order = -order;
	}
	else {          /* x has more digits than y */
		swp = y;
		hi = x->value.decimal.hi;
		mi = x->value.decimal.mi;
		lo = x->value.decimal.lo;
	}
	while(dlen > 8) { /* in effect, right shift by 10**8 */
		lo = mi;
		mi = hi;
		hi = 0;
		dlen -= 8;
	}
	while(dlen > 0) {
		ulong rem1, rem2;
		rem1 = (hi % 10) * 100000000L;
		hi = hi / 10;
		rem2 = (mi % 10) * 100000000L;
		mi = (mi + rem1) / 10;
		lo = (lo + rem2) / 10;
		dlen--;
	}
	if(hi > swp->value.decimal.hi) {
		return order;
	}
	else if(hi == swp->value.decimal.hi) {
		if(mi > swp->value.decimal.mi) {
			return order;
		}
		else if(mi == swp->value.decimal.mi) {
			if(lo > swp->value.decimal.lo) {
				return order;
			}
			else if(lo == swp->value.decimal.lo) {
				if(x->value.decimal.total == y->value.decimal.total) {
					return 0;
				}
				else {
					return order;
				}
			}
		}
	}
	return -order;
}

/**
 * xmlSchemaCompareDurations:
 * @x:  a first duration value
 * @y:  a second duration value
 *
 * Compare 2 durations
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y, 2 if x <> y, and -2 in
 * case of error
 */
static int xmlSchemaCompareDurations(xmlSchemaVal * x, xmlSchemaVal * y)
{
	long carry, mon, day;
	double sec;
	int invert = 1;
	long xmon, xday, myear, minday, maxday;
	static const long dayRange [2][12] = {
		{ 0, 28, 59, 89, 120, 150, 181, 212, 242, 273, 303, 334, },
		{ 0, 31, 62, 92, 123, 153, 184, 215, 245, 276, 306, 337}
	};
	if(!x || !y)
		return -2;
	/* months */
	mon = x->value.dur.mon - y->value.dur.mon;
	/* seconds */
	sec = x->value.dur.sec - y->value.dur.sec;
	carry = (long)(sec / SECS_PER_DAY);
	sec -= ((double)carry) * SECS_PER_DAY;
	/* days */
	day = x->value.dur.day - y->value.dur.day + carry;
	/* easy test */
	if(mon == 0) {
		if(day == 0)
			if(sec == 0.0)
				return 0;
			else if(sec < 0.0)
				return -1;
			else
				return 1;
		else if(day < 0)
			return -1;
		else
			return 1;
	}
	if(mon > 0) {
		if((day >= 0) && (sec >= 0.0))
			return 1;
		else {
			xmon = mon;
			xday = -day;
		}
	}
	else if((day <= 0) && (sec <= 0.0)) {
		return -1;
	}
	else {
		invert = -1;
		xmon = -mon;
		xday = day;
	}
	myear = xmon / 12;
	if(myear == 0) {
		minday = 0;
		maxday = 0;
	}
	else {
		maxday = 366 * ((myear + 3) / 4) + 365 * ((myear - 1) % 4);
		minday = maxday - 1;
	}
	xmon = xmon % 12;
	minday += dayRange[0][xmon];
	maxday += dayRange[1][xmon];
	if((maxday == minday) && (maxday == xday))
		return 0; /* can this really happen ? */
	if(maxday < xday)
		return (-invert);
	if(minday > xday)
		return (invert);
	/* indeterminate */
	return 2;
}
/*
 * macros for adding date/times and durations
 */
#define FQUOTIENT(a, b)                  (floor(((double)a/(double)b)))
#define MODULO(a, b)                     (a - FQUOTIENT(a, b) * b)
#define FQUOTIENT_RANGE(a, low, high)     (FQUOTIENT((a-low), (high-low)))
#define MODULO_RANGE(a, low, high)        ((MODULO((a-low), (high-low)))+low)
/**
 * xmlSchemaDupVal:
 * @v: the #xmlSchemaValPtr value to duplicate
 *
 * Makes a copy of @v. The calling program is responsible for freeing
 * the returned value.
 *
 * returns a pointer to a duplicated #xmlSchemaValPtr or NULL if error.
 */
static xmlSchemaVal * FASTCALL xmlSchemaDupVal(xmlSchemaVal * v)
{
	xmlSchemaVal * ret = xmlSchemaNewValue(v->type);
	if(ret) {
		memcpy(ret, v, sizeof(xmlSchemaVal));
		ret->next = NULL;
	}
	return ret;
}
/**
 * xmlSchemaCopyValue:
 * @val:  the precomputed value to be copied
 *
 * Copies the precomputed value. This duplicates any string within.
 *
 * Returns the copy or NULL if a copy for a data-type is not implemented.
 */
xmlSchemaVal * xmlSchemaCopyValue(xmlSchemaVal * val)
{
	xmlSchemaVal * ret = NULL;
	xmlSchemaVal * prev = NULL;
	xmlSchemaVal * cur;
	// 
	// Copy the string values.
	// 
	while(val) {
		switch(val->type) {
			case XML_SCHEMAS_ANYTYPE:
			case XML_SCHEMAS_IDREFS:
			case XML_SCHEMAS_ENTITIES:
			case XML_SCHEMAS_NMTOKENS:
			    xmlSchemaFreeValue(ret);
			    return 0;
			case XML_SCHEMAS_ANYSIMPLETYPE:
			case XML_SCHEMAS_STRING:
			case XML_SCHEMAS_NORMSTRING:
			case XML_SCHEMAS_TOKEN:
			case XML_SCHEMAS_LANGUAGE:
			case XML_SCHEMAS_NAME:
			case XML_SCHEMAS_NCNAME:
			case XML_SCHEMAS_ID:
			case XML_SCHEMAS_IDREF:
			case XML_SCHEMAS_ENTITY:
			case XML_SCHEMAS_NMTOKEN:
			case XML_SCHEMAS_ANYURI:
			    cur = xmlSchemaDupVal(val);
			    if(val->value.str)
				    cur->value.str = sstrdup(BAD_CAST val->value.str);
			    break;
			case XML_SCHEMAS_QNAME:
			case XML_SCHEMAS_NOTATION:
			    cur = xmlSchemaDupVal(val);
			    if(val->value.qname.name)
				    cur->value.qname.name = sstrdup(BAD_CAST val->value.qname.name);
			    if(val->value.qname.uri)
				    cur->value.qname.uri = sstrdup(BAD_CAST val->value.qname.uri);
			    break;
			case XML_SCHEMAS_HEXBINARY:
			    cur = xmlSchemaDupVal(val);
			    if(val->value.hex.str)
				    cur->value.hex.str = sstrdup(BAD_CAST val->value.hex.str);
			    break;
			case XML_SCHEMAS_BASE64BINARY:
			    cur = xmlSchemaDupVal(val);
			    if(val->value.base64.str)
				    cur->value.base64.str = sstrdup(BAD_CAST val->value.base64.str);
			    break;
			default:
			    cur = xmlSchemaDupVal(val);
			    break;
		}
		if(!ret)
			ret = cur;
		else
			prev->next = cur;
		prev = cur;
		val = val->next;
	}
	return ret;
}
/**
 * _xmlSchemaDateAdd:
 * @dt: an #xmlSchemaValPtr
 * @dur: an #xmlSchemaValPtr of type #XS_DURATION
 *
 * Compute a new date/time from @dt and @dur. This function assumes @dt
 * is either #XML_SCHEMAS_DATETIME, #XML_SCHEMAS_DATE, #XML_SCHEMAS_GYEARMONTH,
 * or #XML_SCHEMAS_GYEAR. The returned #xmlSchemaVal is the same type as
 * @dt. The calling program is responsible for freeing the returned value.
 *
 * Returns a pointer to a new #xmlSchemaVal or NULL if error.
 */
static xmlSchemaVal * _xmlSchemaDateAdd(xmlSchemaVal * dt, xmlSchemaVal * dur)
{
	xmlSchemaVal * ret;
	xmlSchemaVal * tmp;
	long carry, tempdays, temp;
	xmlSchemaValDatePtr r, d;
	xmlSchemaValDurationPtr u;

	if((dt == NULL) || (dur == NULL))
		return NULL;

	ret = xmlSchemaNewValue(dt->type);
	if(!ret)
		return NULL;

	/* make a copy so we don't alter the original value */
	tmp = xmlSchemaDupVal(dt);
	if(!tmp) {
		xmlSchemaFreeValue(ret);
		return NULL;
	}

	r = &(ret->value.date);
	d = &(tmp->value.date);
	u = &(dur->value.dur);

	/* normalization */
	if(d->mon == 0)
		d->mon = 1;

	/* normalize for time zone offset */
	u->sec -= (d->tzo * 60);
	d->tzo = 0;

	/* normalization */
	if(d->day == 0)
		d->day = 1;

	/* month */
	carry  = d->mon + u->mon;
	r->mon = (uint)MODULO_RANGE(carry, 1, 13);
	carry  = (long)FQUOTIENT_RANGE(carry, 1, 13);

	/* year (may be modified later) */
	r->year = d->year + carry;
	if(r->year == 0) {
		if(d->year > 0)
			r->year--;
		else
			r->year++;
	}

	/* time zone */
	r->tzo     = d->tzo;
	r->tz_flag = d->tz_flag;

	/* seconds */
	r->sec = d->sec + u->sec;
	carry  = (long)FQUOTIENT((long)r->sec, 60);
	if(r->sec != 0.0) {
		r->sec = MODULO(r->sec, 60.0);
	}
	/* minute */
	carry += d->min;
	r->min = (uint)MODULO(carry, 60);
	carry  = (long)FQUOTIENT(carry, 60);
	/* hours */
	carry  += d->hour;
	r->hour = (uint)MODULO(carry, 24);
	carry   = (long)FQUOTIENT(carry, 24);
	/*
	 * days
	 * Note we use tempdays because the temporary values may need more
	 * than 5 bits
	 */
	if((VALID_YEAR(r->year)) && (VALID_MONTH(r->mon)) && (d->day > MAX_DAYINMONTH(r->year, r->mon)))
		tempdays = MAX_DAYINMONTH(r->year, r->mon);
	else if(d->day < 1)
		tempdays = 1;
	else
		tempdays = d->day;

	tempdays += u->day + carry;

	while(1) {
		if(tempdays < 1) {
			long tmon = (long)MODULO_RANGE((int)r->mon-1, 1, 13);
			long tyr  = r->year + (long)FQUOTIENT_RANGE((int)r->mon-1, 1, 13);
			if(tyr == 0)
				tyr--;
			/*
			 * Coverity detected an overrun in daysInMonth
			 * of size 12 at position 12 with index variable "((r)->mon - 1)"
			 */
			if(tmon < 1)
				tmon = 1;
			if(tmon > 12)
				tmon = 12;
			tempdays += MAX_DAYINMONTH(tyr, tmon);
			carry = -1;
		}
		else if(VALID_YEAR(r->year) && VALID_MONTH(r->mon) &&
		    tempdays > (long)MAX_DAYINMONTH(r->year, r->mon)) {
			tempdays = tempdays - MAX_DAYINMONTH(r->year, r->mon);
			carry = 1;
		}
		else
			break;
		temp = r->mon + carry;
		r->mon = (uint)MODULO_RANGE(temp, 1, 13);
		r->year = r->year + (uint)FQUOTIENT_RANGE(temp, 1, 13);
		if(r->year == 0) {
			if(temp < 1)
				r->year--;
			else
				r->year++;
		}
	}
	r->day = tempdays;
	/*
	 * adjust the date/time type to the date values
	 */
	if(ret->type != XML_SCHEMAS_DATETIME) {
		if((r->hour) || (r->min) || (r->sec))
			ret->type = XML_SCHEMAS_DATETIME;
		else if(ret->type != XML_SCHEMAS_DATE) {
			if((r->mon != 1) && (r->day != 1))
				ret->type = XML_SCHEMAS_DATE;
			else if((ret->type != XML_SCHEMAS_GYEARMONTH) && (r->mon != 1))
				ret->type = XML_SCHEMAS_GYEARMONTH;
		}
	}
	xmlSchemaFreeValue(tmp);
	return ret;
}
/**
 * xmlSchemaDateNormalize:
 * @dt: an #xmlSchemaValPtr of a date/time type value.
 * @offset: number of seconds to adjust @dt by.
 *
 * Normalize @dt to GMT time. The @offset parameter is subtracted from
 * the return value is a time-zone offset is present on @dt.
 *
 * Returns a normalized copy of @dt or NULL if error.
 */
static xmlSchemaVal * FASTCALL xmlSchemaDateNormalize(xmlSchemaVal * dt, double offset)
{
	xmlSchemaVal * ret = 0;
	xmlSchemaVal * dur;
	if(dt == NULL)
		return NULL;
	if(((dt->type != XML_SCHEMAS_TIME) && (dt->type != XML_SCHEMAS_DATETIME) && (dt->type != XML_SCHEMAS_DATE)) || (dt->value.date.tzo == 0))
		return xmlSchemaDupVal(dt);
	dur = xmlSchemaNewValue(XML_SCHEMAS_DURATION);
	if(dur == NULL)
		return NULL;
	dur->value.date.sec -= offset;
	ret = _xmlSchemaDateAdd(dt, dur);
	if(!ret)
		return NULL;
	xmlSchemaFreeValue(dur);
	// ret->value.date.tzo = 0; 
	return ret;
}
/**
 * _xmlSchemaDateCastYMToDays:
 * @dt: an #xmlSchemaValPtr
 *
 * Convert mon and year of @dt to total number of days. Take the
 * number of years since (or before) 1 AD and add the number of leap
 * years. This is a function  because negative
 * years must be handled a little differently and there is no zero year.
 *
 * Returns number of days.
 */
static long _xmlSchemaDateCastYMToDays(const xmlSchemaVal * dt)
{
	long ret;
	int mon = dt->value.date.mon;
	if(mon <= 0) 
		mon = 1; /* normalization */
	if(dt->value.date.year <= 0)
		ret = (dt->value.date.year * 365) + (((dt->value.date.year+1)/4)-((dt->value.date.year+1)/100) + ((dt->value.date.year+1)/400)) + DAY_IN_YEAR(0, mon, dt->value.date.year);
	else
		ret = ((dt->value.date.year-1) * 365) + (((dt->value.date.year-1)/4)-((dt->value.date.year-1)/100) + ((dt->value.date.year-1)/400)) + DAY_IN_YEAR(0, mon, dt->value.date.year);
	return ret;
}

/**
 * TIME_TO_NUMBER:
 * @dt:  an #xmlSchemaValPtr
 *
 * Calculates the number of seconds in the time portion of @dt.
 *
 * Returns seconds.
 */
#define TIME_TO_NUMBER(dt) (static_cast<double>((dt->value.date.hour * SECS_PER_HOUR) + (dt->value.date.min * SECS_PER_MIN) + (dt->value.date.tzo * SECS_PER_MIN)) + dt->value.date.sec)

/**
 * xmlSchemaCompareDates:
 * @x:  a first date/time value
 * @y:  a second date/time value
 *
 * Compare 2 date/times
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y, 2 if x <> y, and -2 in
 * case of error
 */
static int xmlSchemaCompareDates(xmlSchemaVal * x, xmlSchemaVal * y)
{
	uchar xmask, ymask, xor_mask, and_mask;
	xmlSchemaVal * p1;
	xmlSchemaVal * p2;
	xmlSchemaVal * q1;
	xmlSchemaVal * q2;
	long p1d, p2d, q1d, q2d;
	if(!x || !y)
		return -2;
	if(x->value.date.tz_flag) {
		if(!y->value.date.tz_flag) {
			p1 = xmlSchemaDateNormalize(x, 0);
			p1d = _xmlSchemaDateCastYMToDays(p1) + p1->value.date.day;
			/* normalize y + 14:00 */
			q1 = xmlSchemaDateNormalize(y, (14 * SECS_PER_HOUR));
			q1d = _xmlSchemaDateCastYMToDays(q1) + q1->value.date.day;
			if(p1d < q1d) {
				xmlSchemaFreeValue(p1);
				xmlSchemaFreeValue(q1);
				return -1;
			}
			else if(p1d == q1d) {
				double sec = TIME_TO_NUMBER(p1) - TIME_TO_NUMBER(q1);
				if(sec < 0.0) {
					xmlSchemaFreeValue(p1);
					xmlSchemaFreeValue(q1);
					return -1;
				}
				else {
					int ret = 0;
					/* normalize y - 14:00 */
					q2 = xmlSchemaDateNormalize(y, -(14 * SECS_PER_HOUR));
					q2d = _xmlSchemaDateCastYMToDays(q2) + q2->value.date.day;
					if(p1d > q2d)
						ret = 1;
					else if(p1d == q2d) {
						sec = TIME_TO_NUMBER(p1) - TIME_TO_NUMBER(q2);
						if(sec > 0.0)
							ret = 1;
						else
							ret = 2; /* indeterminate */
					}
					xmlSchemaFreeValue(p1);
					xmlSchemaFreeValue(q1);
					xmlSchemaFreeValue(q2);
					if(ret)
						return ret;
				}
			}
			else {
				xmlSchemaFreeValue(p1);
				xmlSchemaFreeValue(q1);
			}
		}
	}
	else if(y->value.date.tz_flag) {
		q1 = xmlSchemaDateNormalize(y, 0);
		q1d = _xmlSchemaDateCastYMToDays(q1) + q1->value.date.day;
		/* normalize x - 14:00 */
		p1 = xmlSchemaDateNormalize(x, -(14 * SECS_PER_HOUR));
		p1d = _xmlSchemaDateCastYMToDays(p1) + p1->value.date.day;

		if(p1d < q1d) {
			xmlSchemaFreeValue(p1);
			xmlSchemaFreeValue(q1);
			return -1;
		}
		else if(p1d == q1d) {
			double sec = TIME_TO_NUMBER(p1) - TIME_TO_NUMBER(q1);
			if(sec < 0.0) {
				xmlSchemaFreeValue(p1);
				xmlSchemaFreeValue(q1);
				return -1;
			}
			else {
				int ret = 0;
				/* normalize x + 14:00 */
				p2 = xmlSchemaDateNormalize(x, (14 * SECS_PER_HOUR));
				p2d = _xmlSchemaDateCastYMToDays(p2) + p2->value.date.day;
				if(p2d > q1d) {
					ret = 1;
				}
				else if(p2d == q1d) {
					sec = TIME_TO_NUMBER(p2) - TIME_TO_NUMBER(q1);
					if(sec > 0.0)
						ret = 1;
					else
						ret = 2; /* indeterminate */
				}
				xmlSchemaFreeValue(p1);
				xmlSchemaFreeValue(q1);
				xmlSchemaFreeValue(p2);
				if(ret)
					return ret;
			}
		}
		else {
			xmlSchemaFreeValue(p1);
			xmlSchemaFreeValue(q1);
		}
	}
	/*
	 * if the same type then calculate the difference
	 */
	if(x->type == y->type) {
		int ret = 0;
		q1 = xmlSchemaDateNormalize(y, 0);
		q1d = _xmlSchemaDateCastYMToDays(q1) + q1->value.date.day;
		p1 = xmlSchemaDateNormalize(x, 0);
		p1d = _xmlSchemaDateCastYMToDays(p1) + p1->value.date.day;
		if(p1d < q1d) {
			ret = -1;
		}
		else if(p1d > q1d) {
			ret = 1;
		}
		else {
			double sec = TIME_TO_NUMBER(p1) - TIME_TO_NUMBER(q1);
			if(sec < 0.0)
				ret = -1;
			else if(sec > 0.0)
				ret = 1;
		}
		xmlSchemaFreeValue(p1);
		xmlSchemaFreeValue(q1);
		return ret;
	}
	switch(x->type) {
		case XML_SCHEMAS_DATETIME: xmask = 0xf; break;
		case XML_SCHEMAS_DATE: xmask = 0x7; break;
		case XML_SCHEMAS_GYEAR: xmask = 0x1; break;
		case XML_SCHEMAS_GMONTH: xmask = 0x2; break;
		case XML_SCHEMAS_GDAY: xmask = 0x3; break;
		case XML_SCHEMAS_GYEARMONTH: xmask = 0x3; break;
		case XML_SCHEMAS_GMONTHDAY: xmask = 0x6; break;
		case XML_SCHEMAS_TIME: xmask = 0x8; break;
		default: xmask = 0; break;
	}
	switch(y->type) {
		case XML_SCHEMAS_DATETIME: ymask = 0xf; break;
		case XML_SCHEMAS_DATE: ymask = 0x7; break;
		case XML_SCHEMAS_GYEAR: ymask = 0x1; break;
		case XML_SCHEMAS_GMONTH: ymask = 0x2; break;
		case XML_SCHEMAS_GDAY: ymask = 0x3; break;
		case XML_SCHEMAS_GYEARMONTH: ymask = 0x3; break;
		case XML_SCHEMAS_GMONTHDAY: ymask = 0x6; break;
		case XML_SCHEMAS_TIME: ymask = 0x8; break;
		default: ymask = 0; break;
	}
	xor_mask = xmask ^ ymask; /* mark type differences */
	and_mask = xmask & ymask; /* mark field specification */
	/* year */
	if(xor_mask & 1)
		return 2; /* indeterminate */
	else if(and_mask & 1) {
		if(x->value.date.year < y->value.date.year)
			return -1;
		else if(x->value.date.year > y->value.date.year)
			return 1;
	}

	/* month */
	if(xor_mask & 2)
		return 2; /* indeterminate */
	else if(and_mask & 2) {
		if(x->value.date.mon < y->value.date.mon)
			return -1;
		else if(x->value.date.mon > y->value.date.mon)
			return 1;
	}

	/* day */
	if(xor_mask & 4)
		return 2; /* indeterminate */
	else if(and_mask & 4) {
		if(x->value.date.day < y->value.date.day)
			return -1;
		else if(x->value.date.day > y->value.date.day)
			return 1;
	}

	/* time */
	if(xor_mask & 8)
		return 2; /* indeterminate */
	else if(and_mask & 8) {
		if(x->value.date.hour < y->value.date.hour)
			return -1;
		else if(x->value.date.hour > y->value.date.hour)
			return 1;
		else if(x->value.date.min < y->value.date.min)
			return -1;
		else if(x->value.date.min > y->value.date.min)
			return 1;
		else if(x->value.date.sec < y->value.date.sec)
			return -1;
		else if(x->value.date.sec > y->value.date.sec)
			return 1;
	}

	return 0;
}
/**
 * xmlSchemaComparePreserveReplaceStrings:
 * @x:  a first string value
 * @y:  a second string value
 * @invert: inverts the result if x < y or x > y.
 *
 * Compare 2 string for their normalized values.
 * @x is a string with whitespace of "preserve", @y is
 * a string with a whitespace of "replace". I.e. @x could
 * be an "xsd:string" and @y an "xsd:normalizedString".
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y, and -2 in
 * case of error
 */
static int xmlSchemaComparePreserveReplaceStrings(const xmlChar * x, const xmlChar * y, int invert)
{
	while((*x != 0) && (*y != 0)) {
		if(IS_WSP_REPLACE_CH(*y)) {
			if(!IS_WSP_SPACE_CH(*x)) {
				if((*x - 0x20) < 0) {
					return invert ? 1 : -1;
				}
				else {
					return invert ? -1 : 1;
				}
			}
		}
		else {
			const int tmp = *x - *y;
			if(tmp < 0) {
				return invert ? 1 : -1;
			}
			if(tmp > 0) {
				return invert ? -1 : 1;
			}
		}
		x++;
		y++;
	}
	if(*x != 0) {
		return invert ? -1 : 1;
	}
	if(*y != 0) {
		return invert ? 1 : -1;
	}
	return 0;
}
/**
 * xmlSchemaComparePreserveCollapseStrings:
 * @x:  a first string value
 * @y:  a second string value
 *
 * Compare 2 string for their normalized values.
 * @x is a string with whitespace of "preserve", @y is
 * a string with a whitespace of "collapse". I.e. @x could
 * be an "xsd:string" and @y an "xsd:normalizedString".
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y, and -2 in
 * case of error
 */
static int xmlSchemaComparePreserveCollapseStrings(const xmlChar * x, const xmlChar * y, int invert)
{
	/*
	 * Skip leading blank chars of the collapsed string.
	 */
	while(IS_WSP_BLANK_CH(*y))
		y++;
	while((*x != 0) && (*y != 0)) {
		if IS_WSP_BLANK_CH(*y) {
			if(!IS_WSP_SPACE_CH(*x)) {
				/*
				 * The yv character would have been replaced to 0x20.
				 */
				if((*x - 0x20) < 0) {
					return invert ? 1 : -1;
				}
				else {
					return invert ? -1 : 1;
				}
			}
			x++;
			y++;
			/*
			 * Skip contiguous blank chars of the collapsed string.
			 */
			while(IS_WSP_BLANK_CH(*y))
				y++;
		}
		else {
			const int tmp = *x++ - *y++;
			if(tmp < 0) {
				return invert ? 1 : -1;
			}
			if(tmp > 0) {
				return invert ? -1 : 1;
			}
		}
	}
	if(*x != 0) {
		return invert ? -1 : 1;
	}
	if(*y != 0) {
		/*
		 * Skip trailing blank chars of the collapsed string.
		 */
		while(IS_WSP_BLANK_CH(*y))
			y++;
		if(*y != 0) {
			return invert ? 1 : -1;
		}
	}
	return 0;
}
/**
 * xmlSchemaComparePreserveCollapseStrings:
 * @x:  a first string value
 * @y:  a second string value
 *
 * Compare 2 string for their normalized values.
 * @x is a string with whitespace of "preserve", @y is
 * a string with a whitespace of "collapse". I.e. @x could
 * be an "xsd:string" and @y an "xsd:normalizedString".
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y, and -2 in
 * case of error
 */
static int xmlSchemaCompareReplaceCollapseStrings(const xmlChar * x, const xmlChar * y, int invert)
{
	/*
	 * Skip leading blank chars of the collapsed string.
	 */
	while(IS_WSP_BLANK_CH(*y))
		y++;
	while((*x != 0) && (*y != 0)) {
		if IS_WSP_BLANK_CH(*y) {
			if(!IS_WSP_BLANK_CH(*x)) {
				/*
				 * The yv character would have been replaced to 0x20.
				 */
				if((*x - 0x20) < 0) {
					return invert ? 1 : -1;
				}
				else {
					return invert ? -1 : 1;
				}
			}
			x++;
			y++;
			/*
			 * Skip contiguous blank chars of the collapsed string.
			 */
			while(IS_WSP_BLANK_CH(*y))
				y++;
		}
		else {
			if(IS_WSP_BLANK_CH(*x)) {
				/*
				 * The xv character would have been replaced to 0x20.
				 */
				if((0x20 - *y) < 0) {
					return invert ? 1 : -1;
				}
				else {
					return invert ? -1 : 1;
				}
			}
			int tmp = *x++ - *y++;
			if(tmp < 0)
				return -1;
			if(tmp > 0)
				return 1;
		}
	}
	if(*x != 0) {
		return invert ? -1 : 1;
	}
	if(*y != 0) {
		/*
		 * Skip trailing blank chars of the collapsed string.
		 */
		while(IS_WSP_BLANK_CH(*y))
			y++;
		if(*y != 0) {
			return invert ? 1 : -1;
		}
	}
	return 0;
}
/**
 * xmlSchemaCompareReplacedStrings:
 * @x:  a first string value
 * @y:  a second string value
 *
 * Compare 2 string for their normalized values.
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y, and -2 in
 * case of error
 */
static int xmlSchemaCompareReplacedStrings(const xmlChar * x, const xmlChar * y)
{
	int tmp;
	while((*x != 0) && (*y != 0)) {
		if IS_WSP_BLANK_CH(*y) {
			if(!IS_WSP_BLANK_CH(*x)) {
				if((*x - 0x20) < 0)
					return -1;
				else
					return 1;
			}
		}
		else {
			if IS_WSP_BLANK_CH(*x) {
				if((0x20 - *y) < 0)
					return -1;
				else
					return 1;
			}
			tmp = *x - *y;
			if(tmp < 0)
				return -1;
			if(tmp > 0)
				return 1;
		}
		x++;
		y++;
	}
	if(*x != 0)
		return 1;
	if(*y != 0)
		return -1;
	return 0;
}

/**
 * xmlSchemaCompareNormStrings:
 * @x:  a first string value
 * @y:  a second string value
 *
 * Compare 2 string for their normalized values.
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y, and -2 in
 * case of error
 */
static int xmlSchemaCompareNormStrings(const xmlChar * x, const xmlChar * y) 
{
	int tmp;
	while(IS_BLANK_CH(*x)) x++;
	while(IS_BLANK_CH(*y)) y++;
	while((*x != 0) && (*y != 0)) {
		if(IS_BLANK_CH(*x)) {
			if(!IS_BLANK_CH(*y)) {
				tmp = *x - *y;
				return tmp;
			}
			while(IS_BLANK_CH(*x)) x++;
			while(IS_BLANK_CH(*y)) y++;
		}
		else {
			tmp = *x++ - *y++;
			if(tmp < 0)
				return -1;
			if(tmp > 0)
				return 1;
		}
	}
	if(*x != 0) {
		while(IS_BLANK_CH(*x)) x++;
		if(*x != 0)
			return 1;
	}
	if(*y != 0) {
		while(IS_BLANK_CH(*y)) y++;
		if(*y != 0)
			return -1;
	}
	return 0;
}
/**
 * xmlSchemaCompareFloats:
 * @x:  a first float or double value
 * @y:  a second float or double value
 *
 * Compare 2 values
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y, 2 if x <> y, and -2 in
 * case of error
 */
static int xmlSchemaCompareFloats(xmlSchemaVal * x, xmlSchemaVal * y) 
{
	double d1, d2;
	if(!x || !y)
		return -2;
	/*
	 * Cast everything to doubles.
	 */
	if(x->type == XML_SCHEMAS_DOUBLE)
		d1 = x->value.d;
	else if(x->type == XML_SCHEMAS_FLOAT)
		d1 = x->value.f;
	else
		return -2;
	if(y->type == XML_SCHEMAS_DOUBLE)
		d2 = y->value.d;
	else if(y->type == XML_SCHEMAS_FLOAT)
		d2 = y->value.f;
	else
		return -2;
	/*
	 * Check for special cases.
	 */
	if(fisnan(d1)) {
		if(fisnan(d2))
			return 0;
		return 1;
	}
	if(fisnan(d2))
		return -1;
	if(d1 == xmlXPathPINF) {
		return (d2 == xmlXPathPINF) ? 0 : 1;
	}
	if(d2 == xmlXPathPINF)
		return -1;
	if(d1 == xmlXPathNINF) {
		return (d2 == xmlXPathNINF) ? 0 : -1;
	}
	if(d2 == xmlXPathNINF)
		return 1;
	/*
	 * basic tests, the last one we should have equality, but
	 * portability is more important than speed and handling
	 * NaN or Inf in a portable way is always a challenge, so ...
	 */
	if(d1 < d2)
		return -1;
	if(d1 > d2)
		return 1;
	if(d1 == d2)
		return 0;
	return (2);
}
/**
 * xmlSchemaCompareValues:
 * @x:  a first value
 * @xvalue: the first value as a string (optional)
 * @xwtsp: the whitespace type
 * @y:  a second value
 * @xvalue: the second value as a string (optional)
 * @ywtsp: the whitespace type
 *
 * Compare 2 values
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y, 2 if x <> y, 3 if not
 * comparable and -2 in case of error
 */
static int xmlSchemaCompareValuesInternal(xmlSchemaValType xtype, xmlSchemaVal * x, const xmlChar * xvalue,
    xmlSchemaWhitespaceValueType xws, xmlSchemaValType ytype, xmlSchemaVal * y, const xmlChar * yvalue, xmlSchemaWhitespaceValueType yws)
{
	switch(xtype) {
		case XML_SCHEMAS_UNKNOWN:
		case XML_SCHEMAS_ANYTYPE:
		    return -2;
		case XML_SCHEMAS_INTEGER:
		case XML_SCHEMAS_NPINTEGER:
		case XML_SCHEMAS_NINTEGER:
		case XML_SCHEMAS_NNINTEGER:
		case XML_SCHEMAS_PINTEGER:
		case XML_SCHEMAS_INT:
		case XML_SCHEMAS_UINT:
		case XML_SCHEMAS_LONG:
		case XML_SCHEMAS_ULONG:
		case XML_SCHEMAS_SHORT:
		case XML_SCHEMAS_USHORT:
		case XML_SCHEMAS_BYTE:
		case XML_SCHEMAS_UBYTE:
		case XML_SCHEMAS_DECIMAL:
		    if(!x || !y)
			    return -2;
		    if(ytype == xtype)
			    return (xmlSchemaCompareDecimals(x, y));
		    if(oneof14(ytype, XML_SCHEMAS_DECIMAL, XML_SCHEMAS_INTEGER, XML_SCHEMAS_NPINTEGER, XML_SCHEMAS_NINTEGER, XML_SCHEMAS_NNINTEGER,
				XML_SCHEMAS_PINTEGER, XML_SCHEMAS_INT, XML_SCHEMAS_UINT, XML_SCHEMAS_LONG, XML_SCHEMAS_ULONG, XML_SCHEMAS_SHORT, XML_SCHEMAS_USHORT,
				XML_SCHEMAS_BYTE, XML_SCHEMAS_UBYTE))
			    return (xmlSchemaCompareDecimals(x, y));
		    return -2;
		case XML_SCHEMAS_DURATION:
		    if(!x || !y)
			    return -2;
		    if(ytype == XML_SCHEMAS_DURATION)
			    return (xmlSchemaCompareDurations(x, y));
		    return -2;
		case XML_SCHEMAS_TIME:
		case XML_SCHEMAS_GDAY:
		case XML_SCHEMAS_GMONTH:
		case XML_SCHEMAS_GMONTHDAY:
		case XML_SCHEMAS_GYEAR:
		case XML_SCHEMAS_GYEARMONTH:
		case XML_SCHEMAS_DATE:
		case XML_SCHEMAS_DATETIME:
		    if(!x || !y)
			    return -2;
		    if(oneof8(ytype, XML_SCHEMAS_DATETIME, XML_SCHEMAS_TIME, XML_SCHEMAS_GDAY, XML_SCHEMAS_GMONTH, 
				XML_SCHEMAS_GMONTHDAY, XML_SCHEMAS_GYEAR, XML_SCHEMAS_DATE, XML_SCHEMAS_GYEARMONTH))
			    return (xmlSchemaCompareDates(x, y));
		    return (-2);
		/*
		 * Note that we will support comparison of string types against
		 * anySimpleType as well.
		 */
		case XML_SCHEMAS_ANYSIMPLETYPE:
		case XML_SCHEMAS_STRING:
		case XML_SCHEMAS_NORMSTRING:
		case XML_SCHEMAS_TOKEN:
		case XML_SCHEMAS_LANGUAGE:
		case XML_SCHEMAS_NMTOKEN:
		case XML_SCHEMAS_NAME:
		case XML_SCHEMAS_NCNAME:
		case XML_SCHEMAS_ID:
		case XML_SCHEMAS_IDREF:
		case XML_SCHEMAS_ENTITY:
		case XML_SCHEMAS_ANYURI:
	    {
		    const xmlChar * xv = x ? x->value.str : xvalue;
		    const xmlChar * yv = y ? y->value.str : yvalue;
			// 
			// @todo Compare those against QName.
			// 
		    if(ytype == XML_SCHEMAS_QNAME) {
			    TODO
			    if(y == NULL)
				    return -2;
			    return (-2);
		    }
		    if(oneof12(ytype, XML_SCHEMAS_ANYSIMPLETYPE, XML_SCHEMAS_STRING, XML_SCHEMAS_NORMSTRING, XML_SCHEMAS_TOKEN, XML_SCHEMAS_LANGUAGE,
			    XML_SCHEMAS_NMTOKEN, XML_SCHEMAS_NAME, XML_SCHEMAS_NCNAME, XML_SCHEMAS_ID, XML_SCHEMAS_IDREF, XML_SCHEMAS_ENTITY, XML_SCHEMAS_ANYURI)) {
			    if(xws == XML_SCHEMA_WHITESPACE_PRESERVE) {
				    if(yws == XML_SCHEMA_WHITESPACE_PRESERVE) {
					    /* @todo What about x < y or x > y. */
					    if(sstreq(xv, yv))
						    return 0;
					    else
						    return (2);
				    }
				    else if(yws == XML_SCHEMA_WHITESPACE_REPLACE)
					    return (xmlSchemaComparePreserveReplaceStrings(xv, yv, 0));
				    else if(yws == XML_SCHEMA_WHITESPACE_COLLAPSE)
					    return (xmlSchemaComparePreserveCollapseStrings(xv, yv, 0));
			    }
			    else if(xws == XML_SCHEMA_WHITESPACE_REPLACE) {
				    if(yws == XML_SCHEMA_WHITESPACE_PRESERVE)
					    return (xmlSchemaComparePreserveReplaceStrings(yv, xv, 1));
				    if(yws == XML_SCHEMA_WHITESPACE_REPLACE)
					    return (xmlSchemaCompareReplacedStrings(xv, yv));
				    if(yws == XML_SCHEMA_WHITESPACE_COLLAPSE)
					    return (xmlSchemaCompareReplaceCollapseStrings(xv, yv, 0));
			    }
			    else if(xws == XML_SCHEMA_WHITESPACE_COLLAPSE) {
				    if(yws == XML_SCHEMA_WHITESPACE_PRESERVE)
					    return (xmlSchemaComparePreserveCollapseStrings(yv, xv, 1));
				    if(yws == XML_SCHEMA_WHITESPACE_REPLACE)
					    return (xmlSchemaCompareReplaceCollapseStrings(yv, xv, 1));
				    if(yws == XML_SCHEMA_WHITESPACE_COLLAPSE)
					    return (xmlSchemaCompareNormStrings(xv, yv));
			    }
			    else
				    return (-2);
		    }
		    return (-2);
	    }
		case XML_SCHEMAS_QNAME:
		case XML_SCHEMAS_NOTATION:
		    if(!x || !y)
			    return -2;
		    if(oneof2(ytype, XML_SCHEMAS_QNAME, XML_SCHEMAS_NOTATION)) {
			    if((sstreq(x->value.qname.name, y->value.qname.name)) && (sstreq(x->value.qname.uri, y->value.qname.uri)))
				    return 0;
			    return (2);
		    }
		    return (-2);
		case XML_SCHEMAS_FLOAT:
		case XML_SCHEMAS_DOUBLE:
		    if(!x || !y)
			    return -2;
		    if((ytype == XML_SCHEMAS_FLOAT) || (ytype == XML_SCHEMAS_DOUBLE))
			    return (xmlSchemaCompareFloats(x, y));
		    return (-2);
		case XML_SCHEMAS_BOOLEAN:
		    if(!x || !y)
			    return -2;
		    if(ytype == XML_SCHEMAS_BOOLEAN) {
			    if(x->value.b == y->value.b)
				    return 0;
			    if(x->value.b == 0)
				    return -1;
			    return 1;
		    }
		    return (-2);
		case XML_SCHEMAS_HEXBINARY:
		    if(!x || !y)
			    return -2;
		    if(ytype == XML_SCHEMAS_HEXBINARY) {
			    if(x->value.hex.total == y->value.hex.total) {
				    int ret = xmlStrcmp(x->value.hex.str, y->value.hex.str);
				    if(ret > 0)
					    return 1;
				    else if(!ret)
					    return 0;
			    }
			    else if(x->value.hex.total > y->value.hex.total)
				    return 1;

			    return -1;
		    }
		    return (-2);
		case XML_SCHEMAS_BASE64BINARY:
		    if(!x || !y)
			    return -2;
		    if(ytype == XML_SCHEMAS_BASE64BINARY) {
			    if(x->value.base64.total == y->value.base64.total) {
				    int ret = xmlStrcmp(x->value.base64.str,
				    y->value.base64.str);
				    if(ret > 0)
					    return 1;
				    else if(!ret)
					    return 0;
				    else
					    return -1;
			    }
			    else if(x->value.base64.total > y->value.base64.total)
				    return 1;
			    else
				    return -1;
		    }
		    return (-2);
		case XML_SCHEMAS_IDREFS:
		case XML_SCHEMAS_ENTITIES:
		case XML_SCHEMAS_NMTOKENS:
		    TODO
		    break;
	}
	return -2;
}
/**
 * xmlSchemaCompareValues:
 * @x:  a first value
 * @y:  a second value
 *
 * Compare 2 values
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y, 2 if x <> y, and -2 in
 * case of error
 */
int FASTCALL xmlSchemaCompareValues(xmlSchemaVal * x, xmlSchemaVal * y) 
{
	if(!x || !y)
		return -2;
	xmlSchemaWhitespaceValueType xws = (x->type == XML_SCHEMAS_STRING) ? XML_SCHEMA_WHITESPACE_PRESERVE : ((x->type == XML_SCHEMAS_NORMSTRING) ? XML_SCHEMA_WHITESPACE_REPLACE : XML_SCHEMA_WHITESPACE_COLLAPSE);
	xmlSchemaWhitespaceValueType yws = (y->type == XML_SCHEMAS_STRING) ? XML_SCHEMA_WHITESPACE_PRESERVE : ((y->type == XML_SCHEMAS_NORMSTRING) ? XML_SCHEMA_WHITESPACE_REPLACE : XML_SCHEMA_WHITESPACE_COLLAPSE);
	return xmlSchemaCompareValuesInternal(x->type, x, NULL, xws, y->type, y, NULL, yws);
}
/**
 * xmlSchemaCompareValuesWhtsp:
 * @x:  a first value
 * @xws: the whitespace value of x
 * @y:  a second value
 * @yws: the whitespace value of y
 *
 * Compare 2 values
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y, 2 if x <> y, and -2 in case of error
 */
int xmlSchemaCompareValuesWhtsp(xmlSchemaVal * x, xmlSchemaWhitespaceValueType xws, xmlSchemaVal * y, xmlSchemaWhitespaceValueType yws)
{
	return (x && y) ? xmlSchemaCompareValuesInternal(x->type, x, NULL, xws, y->type, y, NULL, yws) : -2;
}
/**
 * xmlSchemaCompareValuesWhtspExt:
 * @x:  a first value
 * @xws: the whitespace value of x
 * @y:  a second value
 * @yws: the whitespace value of y
 *
 * Compare 2 values
 *
 * Returns -1 if x < y, 0 if x == y, 1 if x > y, 2 if x <> y, and -2 in
 * case of error
 */
static int xmlSchemaCompareValuesWhtspExt(xmlSchemaValType xtype, xmlSchemaVal * x, const xmlChar * xvalue,
    xmlSchemaWhitespaceValueType xws, xmlSchemaValType ytype, xmlSchemaVal * y, const xmlChar * yvalue, xmlSchemaWhitespaceValueType yws)
{
	return xmlSchemaCompareValuesInternal(xtype, x, xvalue, xws, ytype, y, yvalue, yws);
}
/**
 * xmlSchemaNormLen:
 * @value:  a string
 *
 * Computes the UTF8 length of the normalized value of the string
 *
 * Returns the length or -1 in case of error.
 */
static int FASTCALL xmlSchemaNormLen(const xmlChar * value) 
{
	const xmlChar * utf;
	int ret = 0;
	if(!value)
		return -1;
	utf = value;
	while(IS_BLANK_CH(*utf)) 
		utf++;
	while(*utf != 0) {
		if(utf[0] & 0x80) {
			if((utf[1] & 0xc0) != 0x80)
				return -1;
			if((utf[0] & 0xe0) == 0xe0) {
				if((utf[2] & 0xc0) != 0x80)
					return -1;
				if((utf[0] & 0xf0) == 0xf0) {
					if((utf[0] & 0xf8) != 0xf0 || (utf[3] & 0xc0) != 0x80)
						return -1;
					utf += 4;
				}
				else {
					utf += 3;
				}
			}
			else {
				utf += 2;
			}
		}
		else if(IS_BLANK_CH(*utf)) {
			while(IS_BLANK_CH(*utf)) utf++;
			if(*utf == 0)
				break;
		}
		else {
			utf++;
		}
		ret++;
	}
	return ret;
}
/**
 * xmlSchemaGetFacetValueAsULong:
 * @facet: an schemas type facet
 *
 * Extract the value of a facet
 *
 * Returns the value as a long
 */
ulong xmlSchemaGetFacetValueAsULong(xmlSchemaFacet * facet)
{
	// @todo Check if this is a decimal.
	return facet ? ((ulong)facet->val->value.decimal.lo) : 0;
}

/**
 * xmlSchemaValidateListSimpleTypeFacet:
 * @facet:  the facet to check
 * @value:  the lexical repr of the value to validate
 * @actualLen:  the number of list items
 * @expectedLen: the resulting expected number of list items
 *
 * Checks the value of a list simple type against a facet.
 *
 * Returns 0 if the value is valid, a positive error code
 * number otherwise and -1 in case of an internal error.
 */
int xmlSchemaValidateListSimpleTypeFacet(xmlSchemaFacet * facet, const xmlChar * value, ulong actualLen, ulong * expectedLen)
{
	if(facet == NULL)
		return -1;
	/*
	 * @todo Check if this will work with large numbers.
	 * (compare value.decimal.mi and value.decimal.hi as well?).
	 */
	if(facet->type == XML_SCHEMA_FACET_LENGTH) {
		if(actualLen != facet->val->value.decimal.lo) {
			ASSIGN_PTR(expectedLen, facet->val->value.decimal.lo);
			return (XML_SCHEMAV_CVC_LENGTH_VALID);
		}
	}
	else if(facet->type == XML_SCHEMA_FACET_MINLENGTH) {
		if(actualLen < facet->val->value.decimal.lo) {
			ASSIGN_PTR(expectedLen, facet->val->value.decimal.lo);
			return (XML_SCHEMAV_CVC_MINLENGTH_VALID);
		}
	}
	else if(facet->type == XML_SCHEMA_FACET_MAXLENGTH) {
		if(actualLen > facet->val->value.decimal.lo) {
			ASSIGN_PTR(expectedLen, facet->val->value.decimal.lo);
			return (XML_SCHEMAV_CVC_MAXLENGTH_VALID);
		}
	}
	else
		/*
		 * NOTE: That we can pass NULL as xmlSchemaValPtr to
		 * xmlSchemaValidateFacet, since the remaining facet types
		 * are: XML_SCHEMA_FACET_PATTERN, XML_SCHEMA_FACET_ENUMERATION.
		 */
		return (xmlSchemaValidateFacet(NULL, facet, value, NULL));
	return 0;
}

/**
 * xmlSchemaValidateLengthFacet:
 * @type:  the built-in type
 * @facet:  the facet to check
 * @value:  the lexical repr. of the value to be validated
 * @val:  the precomputed value
 * @ws: the whitespace type of the value
 * @length: the actual length of the value
 *
 * Checka a value against a "length", "minLength" and "maxLength"
 * facet; sets @length to the computed length of @value.
 *
 * Returns 0 if the value is valid, a positive error code
 * otherwise and -1 in case of an internal or API error.
 */
static int xmlSchemaValidateLengthFacetInternal(xmlSchemaFacet * facet,
    xmlSchemaValType valType, const xmlChar * value, xmlSchemaVal * val, ulong * length, xmlSchemaWhitespaceValueType ws)
{
	uint len = 0;
	if(!length || !facet)
		return -1;
	*length = 0;
	if(!oneof3(facet->type, XML_SCHEMA_FACET_LENGTH, XML_SCHEMA_FACET_MAXLENGTH, XML_SCHEMA_FACET_MINLENGTH))
		return -1;
	// 
	// @todo length, maxLength and minLength must be of type
	// nonNegativeInteger only. Check if decimal is used somehow.
	// 
	if(!facet->val || !oneof2(facet->val->type, XML_SCHEMAS_DECIMAL, XML_SCHEMAS_NNINTEGER) || (facet->val->value.decimal.frac != 0)) {
		return -1;
	}
	if(val && (val->type == XML_SCHEMAS_HEXBINARY))
		len = val->value.hex.total;
	else if(val && (val->type == XML_SCHEMAS_BASE64BINARY))
		len = val->value.base64.total;
	else {
		switch(valType) {
			case XML_SCHEMAS_STRING:
			case XML_SCHEMAS_NORMSTRING:
			    if(ws == XML_SCHEMA_WHITESPACE_UNKNOWN) {
				    // 
				    // This is to ensure API compatibility with the old
				    // xmlSchemaValidateLengthFacet(). Anyway, this was and
				    // is not the correct handling.
				    // @todo Get rid of this case somehow.
				    // 
				    if(valType == XML_SCHEMAS_STRING)
					    len = xmlUTF8Strlen(value);
				    else
					    len = xmlSchemaNormLen(value);
			    }
			    else if(value) {
				    if(ws == XML_SCHEMA_WHITESPACE_COLLAPSE)
					    len = xmlSchemaNormLen(value);
				    else
					    len = xmlUTF8Strlen(value); // Should be OK for "preserve" as well.
			    }
			    break;
			case XML_SCHEMAS_IDREF:
			case XML_SCHEMAS_TOKEN:
			case XML_SCHEMAS_LANGUAGE:
			case XML_SCHEMAS_NMTOKEN:
			case XML_SCHEMAS_NAME:
			case XML_SCHEMAS_NCNAME:
			case XML_SCHEMAS_ID:
			// FIXME: What exactly to do with anyURI?
			case XML_SCHEMAS_ANYURI:
			    if(value)
				    len = xmlSchemaNormLen(value);
			    break;
			case XML_SCHEMAS_QNAME:
			case XML_SCHEMAS_NOTATION:
			    // 
			    // For QName and NOTATION, those facets are deprecated and should be ignored.
			    // 
			    return 0;
			default:
			    TODO
		}
	}
	*length = (ulong)len;
	/*
	 * @todo Return the whole expected value, i.e. "lo", "mi" and "hi".
	 */
	if(facet->type == XML_SCHEMA_FACET_LENGTH) {
		if(len != facet->val->value.decimal.lo)
			return (XML_SCHEMAV_CVC_LENGTH_VALID);
	}
	else if(facet->type == XML_SCHEMA_FACET_MINLENGTH) {
		if(len < facet->val->value.decimal.lo)
			return (XML_SCHEMAV_CVC_MINLENGTH_VALID);
	}
	else {
		if(len > facet->val->value.decimal.lo)
			return (XML_SCHEMAV_CVC_MAXLENGTH_VALID);
	}

	return 0;
}

/**
 * xmlSchemaValidateLengthFacet:
 * @type:  the built-in type
 * @facet:  the facet to check
 * @value:  the lexical repr. of the value to be validated
 * @val:  the precomputed value
 * @length: the actual length of the value
 *
 * Checka a value against a "length", "minLength" and "maxLength"
 * facet; sets @length to the computed length of @value.
 *
 * Returns 0 if the value is valid, a positive error code
 * otherwise and -1 in case of an internal or API error.
 */
int xmlSchemaValidateLengthFacet(xmlSchemaType * type, xmlSchemaFacet * facet, const xmlChar * value, xmlSchemaVal * val, ulong * length)
{
	if(type == NULL)
		return -1;
	return (xmlSchemaValidateLengthFacetInternal(facet, type->builtInType, value, val, length, XML_SCHEMA_WHITESPACE_UNKNOWN));
}

/**
 * xmlSchemaValidateLengthFacetWhtsp:
 * @facet:  the facet to check
 * @valType:  the built-in type
 * @value:  the lexical repr. of the value to be validated
 * @val:  the precomputed value
 * @ws: the whitespace type of the value
 * @length: the actual length of the value
 *
 * Checka a value against a "length", "minLength" and "maxLength"
 * facet; sets @length to the computed length of @value.
 *
 * Returns 0 if the value is valid, a positive error code
 * otherwise and -1 in case of an internal or API error.
 */
int xmlSchemaValidateLengthFacetWhtsp(xmlSchemaFacet * facet, xmlSchemaValType valType, const xmlChar * value,
    xmlSchemaVal * val, ulong * length, xmlSchemaWhitespaceValueType ws)
{
	return (xmlSchemaValidateLengthFacetInternal(facet, valType, value, val, length, ws));
}
/**
 * xmlSchemaValidateFacetInternal:
 * @facet:  the facet to check
 * @fws: the whitespace type of the facet's value
 * @valType: the built-in type of the value
 * @value:  the lexical repr of the value to validate
 * @val:  the precomputed value
 * @ws: the whitespace type of the value
 *
 * Check a value against a facet condition
 *
 * Returns 0 if the element is schemas valid, a positive error code
 *   number otherwise and -1 in case of internal or API error.
 */
static int xmlSchemaValidateFacetInternal(xmlSchemaFacet * facet, xmlSchemaWhitespaceValueType fws, xmlSchemaValType valType,
    const xmlChar * value, xmlSchemaVal * val, xmlSchemaWhitespaceValueType ws)
{
	int ret;
	if(facet == NULL)
		return -1;
	switch(facet->type) {
		case XML_SCHEMA_FACET_PATTERN:
		    /*
		 * NOTE that for patterns, the @value needs to be the normalized
		 * value, *not* the lexical initial value or the canonical value.
		     */
		    if(!value)
			    return -1;
		    ret = xmlRegexpExec(facet->regexp, value);
		    if(ret == 1)
			    return 0;
		    if(!ret)
			    return (XML_SCHEMAV_CVC_PATTERN_VALID);
		    return ret;
		case XML_SCHEMA_FACET_MAXEXCLUSIVE:
		    ret = xmlSchemaCompareValues(val, facet->val);
		    if(ret == -2)
			    return -1;
		    if(ret == -1)
			    return 0;
		    return (XML_SCHEMAV_CVC_MAXEXCLUSIVE_VALID);
		case XML_SCHEMA_FACET_MAXINCLUSIVE:
		    ret = xmlSchemaCompareValues(val, facet->val);
		    if(ret == -2)
			    return -1;
		    if((ret == -1) || (ret == 0))
			    return 0;
		    return (XML_SCHEMAV_CVC_MAXINCLUSIVE_VALID);
		case XML_SCHEMA_FACET_MINEXCLUSIVE:
		    ret = xmlSchemaCompareValues(val, facet->val);
		    if(ret == -2)
			    return -1;
		    if(ret == 1)
			    return 0;
		    return (XML_SCHEMAV_CVC_MINEXCLUSIVE_VALID);
		case XML_SCHEMA_FACET_MININCLUSIVE:
		    ret = xmlSchemaCompareValues(val, facet->val);
		    if(ret == -2)
			    return -1;
		    if((ret == 1) || (ret == 0))
			    return 0;
		    return (XML_SCHEMAV_CVC_MININCLUSIVE_VALID);
		case XML_SCHEMA_FACET_WHITESPACE:
		    /* @todo whitespaces */
		    /*
		 * NOTE: Whitespace should be handled to normalize
		 * the value to be validated against a the facets;
		 * not to normalize the value in-between.
		     */
		    return 0;
		case  XML_SCHEMA_FACET_ENUMERATION:
		    if(ws == XML_SCHEMA_WHITESPACE_UNKNOWN) {
			    /*
			 * This is to ensure API compatibility with the old
			 * xmlSchemaValidateFacet().
			 * @todo Get rid of this case.
			     */
			    if(facet->value && sstreq(facet->value, value))
				    return 0;
		    }
		    else {
			    ret = xmlSchemaCompareValuesWhtspExt(facet->val->type, facet->val, facet->value, fws, valType, val, value, ws);
			    if(ret == -2)
				    return -1;
			    if(!ret)
				    return 0;
		    }
		    return (XML_SCHEMAV_CVC_ENUMERATION_VALID);
		case XML_SCHEMA_FACET_LENGTH:
		    /*
		 * SPEC (1.3) "if {primitive type definition} is QName or NOTATION,
		 * then any {value} is facet-valid."
		     */
		    if(oneof2(valType, XML_SCHEMAS_QNAME, XML_SCHEMAS_NOTATION))
			    return 0;
		/* No break on purpose. */
		case XML_SCHEMA_FACET_MAXLENGTH:
		case XML_SCHEMA_FACET_MINLENGTH: {
		    uint len = 0;
		    if(oneof2(valType, XML_SCHEMAS_QNAME, XML_SCHEMAS_NOTATION))
			    return 0;
		    /*
		 * @todo length, maxLength and minLength must be of type
		 * nonNegativeInteger only. Check if decimal is used somehow.
		     */
		    if(!facet->val || !oneof2(facet->val->type, XML_SCHEMAS_DECIMAL, XML_SCHEMAS_NNINTEGER) || (facet->val->value.decimal.frac != 0)) {
			    return -1;
		    }
		    if(val && (val->type == XML_SCHEMAS_HEXBINARY))
			    len = val->value.hex.total;
		    else if(val && (val->type == XML_SCHEMAS_BASE64BINARY))
			    len = val->value.base64.total;
		    else {
			    switch(valType) {
				    case XML_SCHEMAS_STRING:
				    case XML_SCHEMAS_NORMSTRING:
					if(ws == XML_SCHEMA_WHITESPACE_UNKNOWN) {
						/*
						 * This is to ensure API compatibility with the old
						 * xmlSchemaValidateFacet(). Anyway, this was and
						 * is not the correct handling.
						 * @todo Get rid of this case somehow.
						 */
						if(valType == XML_SCHEMAS_STRING)
							len = xmlUTF8Strlen(value);
						else
							len = xmlSchemaNormLen(value);
					}
					else if(value) {
						if(ws == XML_SCHEMA_WHITESPACE_COLLAPSE)
							len = xmlSchemaNormLen(value);
						else
							/*
							 * Should be OK for "preserve" as well.
							 */
							len = xmlUTF8Strlen(value);
					}
					break;
				    case XML_SCHEMAS_IDREF:
				    case XML_SCHEMAS_TOKEN:
				    case XML_SCHEMAS_LANGUAGE:
				    case XML_SCHEMAS_NMTOKEN:
				    case XML_SCHEMAS_NAME:
				    case XML_SCHEMAS_NCNAME:
				    case XML_SCHEMAS_ID:
				    case XML_SCHEMAS_ANYURI:
					if(value)
						len = xmlSchemaNormLen(value);
					break;
				    default:
					TODO
			    }
		    }
		    if(facet->type == XML_SCHEMA_FACET_LENGTH) {
			    if(len != facet->val->value.decimal.lo)
				    return (XML_SCHEMAV_CVC_LENGTH_VALID);
		    }
		    else if(facet->type == XML_SCHEMA_FACET_MINLENGTH) {
			    if(len < facet->val->value.decimal.lo)
				    return (XML_SCHEMAV_CVC_MINLENGTH_VALID);
		    }
		    else {
			    if(len > facet->val->value.decimal.lo)
				    return (XML_SCHEMAV_CVC_MAXLENGTH_VALID);
		    }
		    break;
	    }
		case XML_SCHEMA_FACET_TOTALDIGITS:
		case XML_SCHEMA_FACET_FRACTIONDIGITS:
		    if(!facet->val || ((facet->val->type != XML_SCHEMAS_PINTEGER) && (facet->val->type != XML_SCHEMAS_NNINTEGER)) || (facet->val->value.decimal.frac != 0)) {
			    return -1;
		    }
		    if(!val || !oneof14(val->type, XML_SCHEMAS_DECIMAL, XML_SCHEMAS_INTEGER, XML_SCHEMAS_NPINTEGER, XML_SCHEMAS_NINTEGER, 
				XML_SCHEMAS_NNINTEGER, XML_SCHEMAS_PINTEGER, XML_SCHEMAS_INT, XML_SCHEMAS_UINT, XML_SCHEMAS_LONG, 
				XML_SCHEMAS_ULONG, XML_SCHEMAS_SHORT, XML_SCHEMAS_USHORT, XML_SCHEMAS_BYTE, XML_SCHEMAS_UBYTE)) {
			    return -1;
		    }
		    if(facet->type == XML_SCHEMA_FACET_TOTALDIGITS) {
			    if(val->value.decimal.total > facet->val->value.decimal.lo)
				    return (XML_SCHEMAV_CVC_TOTALDIGITS_VALID);
		    }
		    else if(facet->type == XML_SCHEMA_FACET_FRACTIONDIGITS) {
			    if(val->value.decimal.frac > facet->val->value.decimal.lo)
				    return (XML_SCHEMAV_CVC_FRACTIONDIGITS_VALID);
		    }
		    break;
		default:
		    TODO
	}
	return 0;
}

/**
 * xmlSchemaValidateFacet:
 * @base:  the base type
 * @facet:  the facet to check
 * @value:  the lexical repr of the value to validate
 * @val:  the precomputed value
 *
 * Check a value against a facet condition
 *
 * Returns 0 if the element is schemas valid, a positive error code
 *   number otherwise and -1 in case of internal or API error.
 */
int xmlSchemaValidateFacet(xmlSchemaType * base, xmlSchemaFacet * facet, const xmlChar * value, xmlSchemaVal * val)
{
	/*
	 * This tries to ensure API compatibility regarding the old
	 * xmlSchemaValidateFacet() and the new xmlSchemaValidateFacetInternal() and
	 * xmlSchemaValidateFacetWhtsp().
	 */
	if(val)
		return xmlSchemaValidateFacetInternal(facet, XML_SCHEMA_WHITESPACE_UNKNOWN, val->type, value, val, XML_SCHEMA_WHITESPACE_UNKNOWN);
	else if(base)
		return xmlSchemaValidateFacetInternal(facet, XML_SCHEMA_WHITESPACE_UNKNOWN, base->builtInType, value, val, XML_SCHEMA_WHITESPACE_UNKNOWN);
	return -1;
}

/**
 * xmlSchemaValidateFacetWhtsp:
 * @facet:  the facet to check
 * @fws: the whitespace type of the facet's value
 * @valType: the built-in type of the value
 * @value:  the lexical (or normalized for pattern) repr of the value to validate
 * @val:  the precomputed value
 * @ws: the whitespace type of the value
 *
 * Check a value against a facet condition. This takes value normalization
 * according to the specified whitespace types into account.
 * Note that @value needs to be the *normalized* value if the facet
 * is of type "pattern".
 *
 * Returns 0 if the element is schemas valid, a positive error code
 *   number otherwise and -1 in case of internal or API error.
 */
int xmlSchemaValidateFacetWhtsp(xmlSchemaFacet * facet, xmlSchemaWhitespaceValueType fws, xmlSchemaValType valType,
    const xmlChar * value, xmlSchemaVal * val, xmlSchemaWhitespaceValueType ws)
{
	return xmlSchemaValidateFacetInternal(facet, fws, valType, value, val, ws);
}
#if 0
#ifndef DBL_DIG
#define DBL_DIG 16
#endif
#ifndef DBL_EPSILON
#define DBL_EPSILON 1E-9
#endif

#define INTEGER_DIGITS DBL_DIG
#define FRACTION_DIGITS (DBL_DIG + 1)
#define EXPONENT_DIGITS (3 + 2)

/**
 * xmlXPathFormatNumber:
 * @number:     number to format
 * @buffer:     output buffer
 * @buffersize: size of output buffer
 *
 * Convert the number into a string representation.
 */
static void xmlSchemaFormatFloat(double number, char buffer[], int buffersize)
{
	switch(xmlXPathIsInf(number)) {
		case 1:
		    if(buffersize > (int)sizeof("INF"))
			    snprintf(buffer, buffersize, "INF");
		    break;
		case -1:
		    if(buffersize > (int)sizeof("-INF"))
			    snprintf(buffer, buffersize, "-INF");
		    break;
		default:
		    if(fisnan(number)) {
			    if(buffersize > (int)sizeof("NaN"))
				    snprintf(buffer, buffersize, "NaN");
		    }
		    else if(number == 0) {
			    snprintf(buffer, buffersize, "0.0E0");
		    }
		    else {
			    /* 3 is sign, decimal point, and terminating zero */
			    char work[DBL_DIG + EXPONENT_DIGITS + 3];
			    char * ptr;
			    char * after_fraction;
			    int size;
			    double absolute_value = fabs(number);
			    /*
			 * Result is in work, and after_fraction points
			 * just past the fractional part.
			 * Use scientific notation
			     */
			    int integer_place = DBL_DIG + EXPONENT_DIGITS + 1;
			    int fraction_place = DBL_DIG - 1;
			    snprintf(work, sizeof(work), "%*.*e", integer_place, fraction_place, number);
			    after_fraction = sstrchr(work + DBL_DIG, 'e');
			    /* Remove fractional trailing zeroes */
			    ptr = after_fraction;
			    while(*(--ptr) == '0')
				    ;
			    if(*ptr != '.')
				    ptr++;
			    while((*ptr++ = *after_fraction++) != 0) ;

			    /* Finally copy result back to caller */
			    size = strlen(work) + 1;
			    if(size > buffersize) {
				    work[buffersize - 1] = 0;
				    size = buffersize;
			    }
			    memmove(buffer, work, size);
		    }
		    break;
	}
}

#endif
/**
 * xmlSchemaGetCanonValue:
 * @val: the precomputed value
 * @retValue: the returned value
 *
 * Get the canonical lexical representation of the value.
 * The caller has to FREE the returned retValue.
 *
 * WARNING: Some value types are not supported yet, resulting
 * in a @retValue of "???".
 *
 * @todo XML Schema 1.0 does not define canonical representations
 * for: duration, gYearMonth, gYear, gMonthDay, gMonth, gDay,
 * anyURI, QName, NOTATION. This will be fixed in XML Schema 1.1.
 *
 *
 * Returns 0 if the value could be built, 1 if the value type is
 * not supported yet and -1 in case of API errors.
 */
int xmlSchemaGetCanonValue(xmlSchemaVal * val, xmlChar ** retValue)
{
	if(!retValue || !val)
		return -1;
	*retValue = NULL;
	switch(val->type) {
		case XML_SCHEMAS_STRING:
		    if(val->value.str == NULL)
			    *retValue = sstrdup(reinterpret_cast<const xmlChar *>(""));
		    else
			    *retValue = sstrdup((const xmlChar *)val->value.str);
		    break;
		case XML_SCHEMAS_NORMSTRING:
		    if(val->value.str == NULL)
			    *retValue = sstrdup(reinterpret_cast<const xmlChar *>(""));
		    else {
			    *retValue = xmlSchemaWhiteSpaceReplace(
			    (const xmlChar *)val->value.str);
			    if((*retValue) == NULL)
				    *retValue = sstrdup((const xmlChar *)val->value.str);
		    }
		    break;
		case XML_SCHEMAS_TOKEN:
		case XML_SCHEMAS_LANGUAGE:
		case XML_SCHEMAS_NMTOKEN:
		case XML_SCHEMAS_NAME:
		case XML_SCHEMAS_NCNAME:
		case XML_SCHEMAS_ID:
		case XML_SCHEMAS_IDREF:
		case XML_SCHEMAS_ENTITY:
		case XML_SCHEMAS_NOTATION: /* Unclear */
		case XML_SCHEMAS_ANYURI: /* Unclear */
		    if(val->value.str == NULL)
			    return -1;
		    *retValue = BAD_CAST xmlSchemaCollapseString(BAD_CAST val->value.str);
		    if(*retValue == NULL)
			    *retValue = sstrdup((const xmlChar *)val->value.str);
		    break;
		case XML_SCHEMAS_QNAME:
		    /* @todo Unclear in XML Schema 1.0. */
		    if(val->value.qname.uri == NULL) {
			    *retValue = sstrdup(BAD_CAST val->value.qname.name);
			    return 0;
		    }
		    else {
			    *retValue = sstrdup(reinterpret_cast<const xmlChar *>("{"));
			    *retValue = BAD_CAST xmlStrcat((xmlChar *)(*retValue), BAD_CAST val->value.qname.uri);
			    *retValue = BAD_CAST xmlStrcat((xmlChar *)(*retValue), reinterpret_cast<const xmlChar *>("}"));
			    *retValue = BAD_CAST xmlStrcat((xmlChar *)(*retValue), BAD_CAST val->value.qname.uri);
		    }
		    break;
		case XML_SCHEMAS_DECIMAL:
		    /*
		 * @todo Lookout for a more simple implementation.
		     */
		    if((val->value.decimal.total == 1) && (val->value.decimal.lo == 0)) {
			    *retValue = sstrdup(reinterpret_cast<const xmlChar *>("0.0"));
		    }
		    else {
			    xmlSchemaValDecimal dec = val->value.decimal;
			    char * buf = NULL, * offs;
			    /* Add room for the decimal point as well. */
			    int bufsize = dec.total + 2;
			    if(dec.sign)
				    bufsize++;
			    /* Add room for leading/trailing zero. */
			    if((dec.frac == 0) || (dec.frac == dec.total))
				    bufsize++;
			    buf = static_cast<char *>(SAlloc::M(bufsize));
			    if(!buf)
				    return -1;
			    offs = buf;
			    if(dec.sign)
				    *offs++ = '-';
			    if(dec.frac == dec.total) {
				    *offs++ = '0';
				    *offs++ = '.';
			    }
			    if(dec.hi != 0)
				    snprintf(offs, bufsize - (offs - buf), "%lu%lu%lu", dec.hi, dec.mi, dec.lo);
			    else if(dec.mi != 0)
				    snprintf(offs, bufsize - (offs - buf), "%lu%lu", dec.mi, dec.lo);
			    else
				    snprintf(offs, bufsize - (offs - buf), "%lu", dec.lo);
			    if(dec.frac != 0) {
				    if(dec.frac != dec.total) {
					    int diff = dec.total - dec.frac;
						// Insert the decimal point.
					    memmove(offs + diff + 1, offs + diff, dec.frac +1);
					    offs[diff] = '.';
				    }
				    else {
					    uint i = 0;
						// Insert missing zeroes behind the decimal point.
					    while(*(offs + i) != 0)
						    i++;
					    if(i < dec.total) {
						    memmove(offs + (dec.total - i), offs, i +1);
						    memset(offs, '0', dec.total - i);
					    }
				    }
			    }
			    else {
					// Append decimal point and zero.
				    offs = buf + bufsize - 1;
				    *offs-- = 0;
				    *offs-- = '0';
				    *offs-- = '.';
			    }
			    *retValue = BAD_CAST buf;
		    }
		    break;
		case XML_SCHEMAS_INTEGER:
		case XML_SCHEMAS_PINTEGER:
		case XML_SCHEMAS_NPINTEGER:
		case XML_SCHEMAS_NINTEGER:
		case XML_SCHEMAS_NNINTEGER:
		case XML_SCHEMAS_LONG:
		case XML_SCHEMAS_BYTE:
		case XML_SCHEMAS_SHORT:
		case XML_SCHEMAS_INT:
		case XML_SCHEMAS_UINT:
		case XML_SCHEMAS_ULONG:
		case XML_SCHEMAS_USHORT:
		case XML_SCHEMAS_UBYTE:
		    if((val->value.decimal.total == 1) && (val->value.decimal.lo == 0))
			    *retValue = sstrdup(reinterpret_cast<const xmlChar *>("0"));
		    else {
			    xmlSchemaValDecimal dec = val->value.decimal;
			    int bufsize = dec.total + 1;
			    /* Add room for the decimal point as well. */
			    if(dec.sign)
				    bufsize++;
			    *retValue = static_cast<xmlChar *>(SAlloc::M(bufsize));
			    if(*retValue == NULL)
				    return -1;
			    if(dec.hi != 0) {
				    if(dec.sign)
					    snprintf((char *)*retValue, bufsize, "-%lu%lu%lu", dec.hi, dec.mi, dec.lo);
				    else
					    snprintf((char *)*retValue, bufsize, "%lu%lu%lu", dec.hi, dec.mi, dec.lo);
			    }
			    else if(dec.mi != 0) {
				    if(dec.sign)
					    snprintf((char *)*retValue, bufsize, "-%lu%lu", dec.mi, dec.lo);
				    else
					    snprintf((char *)*retValue, bufsize, "%lu%lu", dec.mi, dec.lo);
			    }
			    else {
				    if(dec.sign)
					    snprintf((char *)*retValue, bufsize, "-%lu", dec.lo);
				    else
					    snprintf((char *)*retValue, bufsize, "%lu", dec.lo);
			    }
		    }
		    break;
		case XML_SCHEMAS_BOOLEAN:
		    *retValue = val->value.b ? sstrdup(reinterpret_cast<const xmlChar *>("true")) : sstrdup(reinterpret_cast<const xmlChar *>("false"));
		    break;
		case XML_SCHEMAS_DURATION: {
		    char buf[100];
		    ulong year;
		    ulong mon, day, hour = 0, min = 0;
		    double sec = 0, left;
		    /* @todo Unclear in XML Schema 1.0 */
		    /*
		 * @todo This results in a normalized output of the value
		 * - which is NOT conformant to the spec -
		 * since the exact values of each property are not
		 * recoverable. Think about extending the structure to
		 * provide a field for every property.
		     */
		    year = (ulong)FQUOTIENT(labs(val->value.dur.mon), 12);
		    mon = labs(val->value.dur.mon) - 12 * year;
		    day = (ulong)FQUOTIENT(fabs(val->value.dur.sec), 86400);
		    left = fabs(val->value.dur.sec) - day * 86400;
		    if(left > 0) {
			    hour = (ulong)FQUOTIENT(left, 3600);
			    left = left - (hour * 3600);
			    if(left > 0) {
				    min = (ulong)FQUOTIENT(left, 60);
				    sec = left - (min * 60);
			    }
		    }
		    if((val->value.dur.mon < 0) || (val->value.dur.sec < 0))
			    snprintf(buf, 100, "P%luY%luM%luDT%luH%luM%.14gS", year, mon, day, hour, min, sec);
		    else
			    snprintf(buf, 100, "-P%luY%luM%luDT%luH%luM%.14gS", year, mon, day, hour, min, sec);
		    *retValue = sstrdup(BAD_CAST buf);
	    }
	    break;
		case XML_SCHEMAS_GYEAR: {
		    char buf[30];
		    /* @todo Unclear in XML Schema 1.0 */
		    /* @todo What to do with the timezone? */
		    snprintf(buf, 30, "%04ld", val->value.date.year);
		    *retValue = sstrdup(BAD_CAST buf);
	    }
	    break;
		case XML_SCHEMAS_GMONTH: {
		    /* @todo Unclear in XML Schema 1.0 */
		    /* @todo What to do with the timezone? */
		    *retValue = static_cast<xmlChar *>(SAlloc::M(6));
		    if(*retValue == NULL)
			    return -1;
		    snprintf((char *)*retValue, 6, "--%02u", val->value.date.mon);
	    }
	    break;
		case XML_SCHEMAS_GDAY: {
		    /* @todo Unclear in XML Schema 1.0 */
		    /* @todo What to do with the timezone? */
		    *retValue = static_cast<xmlChar *>(SAlloc::M(6));
		    if(*retValue == NULL)
			    return -1;
		    snprintf((char *)*retValue, 6, "---%02u", val->value.date.day);
	    }
	    break;
		case XML_SCHEMAS_GMONTHDAY: {
		    /* @todo Unclear in XML Schema 1.0 */
		    /* @todo What to do with the timezone? */
		    *retValue = static_cast<xmlChar *>(SAlloc::M(8));
		    if(*retValue == NULL)
			    return -1;
		    snprintf((char *)*retValue, 8, "--%02u-%02u", val->value.date.mon, val->value.date.day);
	    }
	    break;
		case XML_SCHEMAS_GYEARMONTH: {
		    char buf[35];
		    /* @todo Unclear in XML Schema 1.0 */
		    /* @todo What to do with the timezone? */
		    if(val->value.date.year < 0)
			    snprintf(buf, 35, "-%04ld-%02u", labs(val->value.date.year), val->value.date.mon);
		    else
			    snprintf(buf, 35, "%04ld-%02u", val->value.date.year, val->value.date.mon);
		    *retValue = sstrdup(BAD_CAST buf);
	    }
	    break;
		case XML_SCHEMAS_TIME:
	    {
		    char buf[30];
		    if(val->value.date.tz_flag) {
			    xmlSchemaVal * norm = xmlSchemaDateNormalize(val, 0);
			    if(norm == NULL)
				    return -1;
			    /*
			 * @todo Check if "%.14g" is portable.
			     */
			    snprintf(buf, 30, "%02u:%02u:%02.14gZ", norm->value.date.hour, norm->value.date.min, norm->value.date.sec);
			    xmlSchemaFreeValue(norm);
		    }
		    else {
			    snprintf(buf, 30, "%02u:%02u:%02.14g", val->value.date.hour, val->value.date.min, val->value.date.sec);
		    }
		    *retValue = sstrdup(BAD_CAST buf);
	    }
	    break;
		case XML_SCHEMAS_DATE:
	    {
		    char buf[30];
		    if(val->value.date.tz_flag) {
			    xmlSchemaVal * norm = xmlSchemaDateNormalize(val, 0);
			    if(norm == NULL)
				    return -1;
			    /*
			 * @todo Append the canonical value of the
			 * recoverable timezone and not "Z".
			     */
			    snprintf(buf, 30, "%04ld:%02u:%02uZ", norm->value.date.year, norm->value.date.mon, norm->value.date.day);
			    xmlSchemaFreeValue(norm);
		    }
		    else {
			    snprintf(buf, 30, "%04ld:%02u:%02u", val->value.date.year, val->value.date.mon, val->value.date.day);
		    }
		    *retValue = sstrdup(BAD_CAST buf);
	    }
	    break;
		case XML_SCHEMAS_DATETIME:
	    {
		    char buf[50];
		    if(val->value.date.tz_flag) {
			    xmlSchemaVal * norm = xmlSchemaDateNormalize(val, 0);
			    if(norm == NULL)
				    return -1;
				// @todo Check if "%.14g" is portable.
			    snprintf(buf, 50, "%04ld:%02u:%02uT%02u:%02u:%02.14gZ",
				    norm->value.date.year, norm->value.date.mon, norm->value.date.day, norm->value.date.hour, norm->value.date.min, norm->value.date.sec);
			    xmlSchemaFreeValue(norm);
		    }
		    else {
			    snprintf(buf, 50, "%04ld:%02u:%02uT%02u:%02u:%02.14g",
				    val->value.date.year, val->value.date.mon, val->value.date.day, val->value.date.hour, val->value.date.min, val->value.date.sec);
		    }
		    *retValue = sstrdup(BAD_CAST buf);
	    }
	    break;
		case XML_SCHEMAS_HEXBINARY:
		    *retValue = sstrdup(BAD_CAST val->value.hex.str);
		    break;
		case XML_SCHEMAS_BASE64BINARY:
		    /*
		 * @todo Is the following spec piece implemented?:
		 * SPEC: "Note: For some values the canonical form defined
		 * above does not conform to [RFC 2045], which requires breaking
		 * with linefeeds at appropriate intervals."
		     */
		    *retValue = sstrdup(BAD_CAST val->value.base64.str);
		    break;
		case XML_SCHEMAS_FLOAT: {
		    char buf[30];
		    /*
		 * |m| < 16777216, -149 <= e <= 104.
		 * @todo Handle, NaN, INF, -INF. The format is not
		 * yet conformant. The c type float does not cover
		 * the whole range.
		     */
		    snprintf(buf, 30, "%01.14e", val->value.f);
		    *retValue = sstrdup(BAD_CAST buf);
	    }
	    break;
		case XML_SCHEMAS_DOUBLE: {
		    char buf[40];
		    /* |m| < 9007199254740992, -1075 <= e <= 970 */
		    /*
		 * @todo Handle, NaN, INF, -INF. The format is not
		 * yet conformant. The c type float does not cover
		 * the whole range.
		     */
		    snprintf(buf, 40, "%01.14e", val->value.d);
		    *retValue = sstrdup(BAD_CAST buf);
	    }
	    break;
		default:
		    *retValue = sstrdup(reinterpret_cast<const xmlChar *>("???"));
		    return 1;
	}
	if(*retValue == NULL)
		return -1;
	return 0;
}
/**
 * xmlSchemaGetCanonValueWhtsp:
 * @val: the precomputed value
 * @retValue: the returned value
 * @ws: the whitespace type of the value
 *
 * Get the canonical representation of the value.
 * The caller has to free the returned @retValue.
 *
 * Returns 0 if the value could be built, 1 if the value type is
 * not supported yet and -1 in case of API errors.
 */
int xmlSchemaGetCanonValueWhtsp(xmlSchemaVal * val, xmlChar ** retValue, xmlSchemaWhitespaceValueType ws)
{
	if((retValue == NULL) || (val == NULL))
		return -1;
	if((ws == XML_SCHEMA_WHITESPACE_UNKNOWN) || (ws > XML_SCHEMA_WHITESPACE_COLLAPSE))
		return -1;
	*retValue = NULL;
	switch(val->type) {
		case XML_SCHEMAS_STRING:
		    if(val->value.str == NULL)
			    *retValue = sstrdup(reinterpret_cast<const xmlChar *>(""));
		    else if(ws == XML_SCHEMA_WHITESPACE_COLLAPSE)
			    *retValue = xmlSchemaCollapseString(val->value.str);
		    else if(ws == XML_SCHEMA_WHITESPACE_REPLACE)
			    *retValue = xmlSchemaWhiteSpaceReplace(val->value.str);
		    if((*retValue) == NULL)
			    *retValue = sstrdup(val->value.str);
		    break;
		case XML_SCHEMAS_NORMSTRING:
		    if(val->value.str == NULL)
			    *retValue = sstrdup(reinterpret_cast<const xmlChar *>(""));
		    else {
			    if(ws == XML_SCHEMA_WHITESPACE_COLLAPSE)
				    *retValue = xmlSchemaCollapseString(val->value.str);
			    else
				    *retValue = xmlSchemaWhiteSpaceReplace(val->value.str);
			    if((*retValue) == NULL)
				    *retValue = sstrdup(val->value.str);
		    }
		    break;
		default:
		    return (xmlSchemaGetCanonValue(val, retValue));
	}
	return 0;
}
/**
 * xmlSchemaGetValType:
 * @val: a schemas value
 *
 * Accessor for the type of a value
 *
 * Returns the xmlSchemaValType of the value
 */
xmlSchemaValType xmlSchemaGetValType(xmlSchemaVal * val)
{
	return val ? val->type : XML_SCHEMAS_UNKNOWN;
}

#define bottom_xmlschemastypes
#endif /* LIBXML_SCHEMAS_ENABLED */
