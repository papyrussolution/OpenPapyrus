/*
 * Summary: internal interfaces for XML Schemas
 * Description: internal interfaces for the XML Schemas handling
 *   and schema validity checking
 *		The Schemas development is a Work In Progress.
 *   Some of those interfaces are not garanteed to be API or ABI stable !
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */
#ifndef __XML_SCHEMA_INTERNALS_H__
#define __XML_SCHEMA_INTERNALS_H__

#ifdef LIBXML_SCHEMAS_ENABLED

#ifdef __cplusplus
extern "C" {
#endif

enum xmlSchemaValType {
    XML_SCHEMAS_UNKNOWN = 0,
    XML_SCHEMAS_STRING,
    XML_SCHEMAS_NORMSTRING,
    XML_SCHEMAS_DECIMAL,
    XML_SCHEMAS_TIME,
    XML_SCHEMAS_GDAY,
    XML_SCHEMAS_GMONTH,
    XML_SCHEMAS_GMONTHDAY,
    XML_SCHEMAS_GYEAR,
    XML_SCHEMAS_GYEARMONTH,
    XML_SCHEMAS_DATE,
    XML_SCHEMAS_DATETIME,
    XML_SCHEMAS_DURATION,
    XML_SCHEMAS_FLOAT,
    XML_SCHEMAS_DOUBLE,
    XML_SCHEMAS_BOOLEAN,
    XML_SCHEMAS_TOKEN,
    XML_SCHEMAS_LANGUAGE,
    XML_SCHEMAS_NMTOKEN,
    XML_SCHEMAS_NMTOKENS,
    XML_SCHEMAS_NAME,
    XML_SCHEMAS_QNAME,
    XML_SCHEMAS_NCNAME,
    XML_SCHEMAS_ID,
    XML_SCHEMAS_IDREF,
    XML_SCHEMAS_IDREFS,
    XML_SCHEMAS_ENTITY,
    XML_SCHEMAS_ENTITIES,
    XML_SCHEMAS_NOTATION,
    XML_SCHEMAS_ANYURI,
    XML_SCHEMAS_INTEGER,
    XML_SCHEMAS_NPINTEGER,
    XML_SCHEMAS_NINTEGER,
    XML_SCHEMAS_NNINTEGER,
    XML_SCHEMAS_PINTEGER,
    XML_SCHEMAS_INT,
    XML_SCHEMAS_UINT,
    XML_SCHEMAS_LONG,
    XML_SCHEMAS_ULONG,
    XML_SCHEMAS_SHORT,
    XML_SCHEMAS_USHORT,
    XML_SCHEMAS_BYTE,
    XML_SCHEMAS_UBYTE,
    XML_SCHEMAS_HEXBINARY,
    XML_SCHEMAS_BASE64BINARY,
    XML_SCHEMAS_ANYTYPE,
    XML_SCHEMAS_ANYSIMPLETYPE
};
/*
 * XML Schemas defines multiple type of types.
 */
enum xmlSchemaTypeType {
    XML_SCHEMA_TYPE_BASIC = 1, /* A built-in datatype */
    XML_SCHEMA_TYPE_ANY,
    XML_SCHEMA_TYPE_FACET,
    XML_SCHEMA_TYPE_SIMPLE,
    XML_SCHEMA_TYPE_COMPLEX,
    XML_SCHEMA_TYPE_SEQUENCE = 6,
    XML_SCHEMA_TYPE_CHOICE,
    XML_SCHEMA_TYPE_ALL,
    XML_SCHEMA_TYPE_SIMPLE_CONTENT,
    XML_SCHEMA_TYPE_COMPLEX_CONTENT,
    XML_SCHEMA_TYPE_UR,
    XML_SCHEMA_TYPE_RESTRICTION,
    XML_SCHEMA_TYPE_EXTENSION,
    XML_SCHEMA_TYPE_ELEMENT,
    XML_SCHEMA_TYPE_ATTRIBUTE,
    XML_SCHEMA_TYPE_ATTRIBUTEGROUP,
    XML_SCHEMA_TYPE_GROUP,
    XML_SCHEMA_TYPE_NOTATION,
    XML_SCHEMA_TYPE_LIST,
    XML_SCHEMA_TYPE_UNION,
    XML_SCHEMA_TYPE_ANY_ATTRIBUTE,
    XML_SCHEMA_TYPE_IDC_UNIQUE,
    XML_SCHEMA_TYPE_IDC_KEY,
    XML_SCHEMA_TYPE_IDC_KEYREF,
    XML_SCHEMA_TYPE_PARTICLE = 25,
    XML_SCHEMA_TYPE_ATTRIBUTE_USE,
    XML_SCHEMA_FACET_MININCLUSIVE = 1000,
    XML_SCHEMA_FACET_MINEXCLUSIVE,
    XML_SCHEMA_FACET_MAXINCLUSIVE,
    XML_SCHEMA_FACET_MAXEXCLUSIVE,
    XML_SCHEMA_FACET_TOTALDIGITS,
    XML_SCHEMA_FACET_FRACTIONDIGITS,
    XML_SCHEMA_FACET_PATTERN,
    XML_SCHEMA_FACET_ENUMERATION,
    XML_SCHEMA_FACET_WHITESPACE,
    XML_SCHEMA_FACET_LENGTH,
    XML_SCHEMA_FACET_MAXLENGTH,
    XML_SCHEMA_FACET_MINLENGTH,
    XML_SCHEMA_EXTRA_QNAMEREF = 2000,
    XML_SCHEMA_EXTRA_ATTR_USE_PROHIB
};

enum xmlSchemaContentType {
    XML_SCHEMA_CONTENT_UNKNOWN = 0,
    XML_SCHEMA_CONTENT_EMPTY = 1,
    XML_SCHEMA_CONTENT_ELEMENTS,
    XML_SCHEMA_CONTENT_MIXED,
    XML_SCHEMA_CONTENT_SIMPLE,
    XML_SCHEMA_CONTENT_MIXED_OR_ELEMENTS, /* Obsolete */
    XML_SCHEMA_CONTENT_BASIC,
    XML_SCHEMA_CONTENT_ANY
};

struct xmlSchemaVal;
typedef struct _xmlSchemaType xmlSchemaType;
typedef struct _xmlSchemaFacet xmlSchemaFacet;
//typedef xmlSchemaFacet * xmlSchemaFacetPtr;
//typedef xmlSchemaVal * xmlSchemaValPtr;
//typedef xmlSchemaType * xmlSchemaTypePtr;
/**
 * Annotation
 */
//typedef struct _xmlSchemaAnnot xmlSchemaAnnot;
struct xmlSchemaAnnot {
    xmlSchemaAnnot * next;
    xmlNode * content; // the annotation 
};

typedef xmlSchemaAnnot * xmlSchemaAnnotPtr;

#define XML_SCHEMAS_ANYATTR_SKIP            1 // Skip unknown attribute from validation. Obsolete, not used anymore.
#define XML_SCHEMAS_ANYATTR_LAX             2 // Ignore validation non definition on attributes. Obsolete, not used anymore.
#define XML_SCHEMAS_ANYATTR_STRICT          3 // Apply strict validation rules on attributes. Obsolete, not used anymore.
#define XML_SCHEMAS_ANY_SKIP                1 // Skip unknown attribute from validation
#define XML_SCHEMAS_ANY_LAX                 2 // Used by wildcards. Validate if type found, don't worry if not found
#define XML_SCHEMAS_ANY_STRICT              3 // Used by wildcards. Apply strict validation rules
#define XML_SCHEMAS_ATTR_USE_PROHIBITED     0 // Used by wildcards. The attribute is prohibited.
#define XML_SCHEMAS_ATTR_USE_REQUIRED       1 // The attribute is required.
#define XML_SCHEMAS_ATTR_USE_OPTIONAL       2 // The attribute is optional.
#define XML_SCHEMAS_ATTR_GLOBAL             (1 << 0) // allow elements in no namespace
#define XML_SCHEMAS_ATTR_NSDEFAULT          (1 << 7) // allow elements in no namespace
#define XML_SCHEMAS_ATTR_INTERNAL_RESOLVED  (1 << 8) // this is set when the "type" and "ref" references have been resolved.
#define XML_SCHEMAS_ATTR_FIXED              (1 << 9) // the attribute has a fixed value
/**
 * xmlSchemaAttribute:
 * An attribute definition.
 */
struct xmlSchemaAttribute {
    xmlSchemaTypeType type;
    xmlSchemaAttribute * next; /* the next attribute (not used?) */
    const xmlChar *name; /* the name of the declaration */
    const xmlChar *id; /* Deprecated; not used */
    const xmlChar *ref; /* Deprecated; not used */
    const xmlChar *refNs; /* Deprecated; not used */
    const xmlChar *typeName; /* the local name of the type definition */
    const xmlChar *typeNs; /* the ns URI of the type definition */
    xmlSchemaAnnotPtr annot;
    xmlSchemaType * base; /* Deprecated; not used */
    int occurs; /* Deprecated; not used */
    const xmlChar *defValue; /* The initial value of the value constraint */
    xmlSchemaType * subtypes; /* the type definition */
    xmlNode * P_Node;
    const xmlChar *targetNamespace;
    int flags;
    const xmlChar *refPrefix; /* Deprecated; not used */
    xmlSchemaVal * defVal; /* The compiled value constraint */
    xmlSchemaAttribute * refDecl; /* Deprecated; not used */
};
//typedef struct _xmlSchemaAttribute xmlSchemaAttribute;
//typedef xmlSchemaAttribute * xmlSchemaAttributePtr;
/**
 * xmlSchemaAttributeLink:
 * Used to build a list of attribute uses on complexType definitions.
 * WARNING: Deprecated; not used.
 */
struct xmlSchemaAttributeLink {
    xmlSchemaAttributeLink * next; /* the next attribute link ... */
    xmlSchemaAttribute * attr; /* the linked attribute */
};

//typedef struct _xmlSchemaAttributeLink xmlSchemaAttributeLink;
//typedef xmlSchemaAttributeLink *xmlSchemaAttributeLinkPtr;
/**
 * XML_SCHEMAS_WILDCARD_COMPLETE:
 *
 * If the wildcard is complete.
 */
#define XML_SCHEMAS_WILDCARD_COMPLETE 1 << 0
/**
 * xmlSchemaCharValueLink:
 * Used to build a list of namespaces on wildcards.
 */
//typedef struct _xmlSchemaWildcardNs xmlSchemaWildcardNs;
//typedef xmlSchemaWildcardNs * xmlSchemaWildcardNsPtr;
struct xmlSchemaWildcardNs {
    struct xmlSchemaWildcardNs * next; // the next constraint link ... 
    const xmlChar * value; // the value 
};
/**
 * xmlSchemaWildcard.
 * A wildcard.
 */
typedef struct _xmlSchemaWildcard xmlSchemaWildcard;
typedef xmlSchemaWildcard * xmlSchemaWildcardPtr;
struct _xmlSchemaWildcard {
    xmlSchemaTypeType type; /* The kind of type */
    const xmlChar *id; /* Deprecated; not used */
    xmlSchemaAnnotPtr annot;
    xmlNode * P_Node;
    int minOccurs; /* Deprecated; not used */
    int maxOccurs; /* Deprecated; not used */
    int processContents;
    int any; /* Indicates if the ns constraint is of ##any */
    xmlSchemaWildcardNs * nsSet; /* The list of allowed namespaces */
    xmlSchemaWildcardNs * negNsSet; /* The negated namespace */
    int flags;
};

#define XML_SCHEMAS_ATTRGROUP_WILDCARD_BUILDED (1 << 0) // The attribute wildcard has been already builded.
#define XML_SCHEMAS_ATTRGROUP_GLOBAL (1 << 1) // The attribute wildcard has been already builded.
#define XML_SCHEMAS_ATTRGROUP_MARKED 1 << 2 // Marks the attr group as marked; used for circular checks.
#define XML_SCHEMAS_ATTRGROUP_REDEFINED (1 << 3) // The attr group was redefined.
#define XML_SCHEMAS_ATTRGROUP_HAS_REFS (1 << 4) // Whether this attr. group contains attr. group references.
/**
 * An attribute group definition.
 *
 * xmlSchemaAttribute and xmlSchemaAttributeGroup start of structures
 * must be kept similar
 */
//typedef struct _xmlSchemaAttributeGroup xmlSchemaAttributeGroup;
//typedef xmlSchemaAttributeGroup * xmlSchemaAttributeGroupPtr;
struct xmlSchemaAttributeGroup {
    xmlSchemaTypeType type;    // The kind of type 
    xmlSchemaAttribute * next; // the next attribute if in a group ... 
    const xmlChar *name;
    const xmlChar *id;
    const xmlChar *ref;   // Deprecated; not used
    const xmlChar *refNs; // Deprecated; not used 
    xmlSchemaAnnotPtr annot;
    xmlSchemaAttribute * attributes; // Deprecated; not used 
    xmlNode * P_Node;
    int flags;
    xmlSchemaWildcardPtr attributeWildcard;
    const xmlChar *refPrefix; // Deprecated; not used 
    xmlSchemaAttributeGroup * refItem; // Deprecated; not used 
    const xmlChar *targetNamespace;
    void *attrUses;
};

/**
 * xmlSchemaTypeLink:
 * Used to build a list of types (e.g. member types of
 * simpleType with variety "union").
 */
//typedef struct _xmlSchemaTypeLink xmlSchemaTypeLink;
//typedef xmlSchemaTypeLink *xmlSchemaTypeLinkPtr;
struct xmlSchemaTypeLink {
    xmlSchemaTypeLink * next; // the next type link ... 
    xmlSchemaType * type; // the linked type 
};
/**
 * xmlSchemaFacetLink:
 * Used to build a list of facets.
 */
//typedef struct _xmlSchemaFacetLink xmlSchemaFacetLink;
//typedef xmlSchemaFacetLink * xmlSchemaFacetLinkPtr;
struct xmlSchemaFacetLink {
    xmlSchemaFacetLink * next; // the next facet link ... 
    xmlSchemaFacet * facet; // the linked facet 
};

#define XML_SCHEMAS_TYPE_MIXED                (1 << 0) // the element content type is mixed
#define XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION                (1 << 1) // the simple or complex type has a derivation method of "extension".
#define XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION                (1 << 2) // the simple or complex type has a derivation method of "restriction".
#define XML_SCHEMAS_TYPE_GLOBAL                (1 << 3) // the type is global
#define XML_SCHEMAS_TYPE_OWNED_ATTR_WILDCARD    (1 << 4) // Obsolete. // the complexType owns an attribute wildcard, i.e. it can be freed by the complexType
/**
 * XML_SCHEMAS_TYPE_VARIETY_ABSENT:
 *
 * the simpleType has a variety of "absent".
 * @todo Actually not necessary :-/, since if
 * none of the variety flags occur then it's
 * automatically absent.
 */
#define XML_SCHEMAS_TYPE_VARIETY_ABSENT    (1 << 5)
#define XML_SCHEMAS_TYPE_VARIETY_LIST    (1 << 6) // the simpleType has a variety of "list".
#define XML_SCHEMAS_TYPE_VARIETY_UNION    (1 << 7) // the simpleType has a variety of "union".
#define XML_SCHEMAS_TYPE_VARIETY_ATOMIC    (1 << 8) // the simpleType has a variety of "union".
#define XML_SCHEMAS_TYPE_FINAL_EXTENSION    (1 << 9) // the complexType has a final of "extension".
#define XML_SCHEMAS_TYPE_FINAL_RESTRICTION    (1 << 10) // the simpleType/complexType has a final of "restriction".
#define XML_SCHEMAS_TYPE_FINAL_LIST    (1 << 11) // the simpleType has a final of "list".
#define XML_SCHEMAS_TYPE_FINAL_UNION    (1 << 12) // the simpleType has a final of "union".
#define XML_SCHEMAS_TYPE_FINAL_DEFAULT    (1 << 13) // the simpleType has a final of "default".
#define XML_SCHEMAS_TYPE_BUILTIN_PRIMITIVE    (1 << 14) // Marks the item as a builtin primitive.
#define XML_SCHEMAS_TYPE_MARKED        (1 << 16) // Marks the item as marked; used for circular checks.
#define XML_SCHEMAS_TYPE_BLOCK_DEFAULT    (1 << 17) // the complexType did not specify 'block' so use the default of the <schema> item.
#define XML_SCHEMAS_TYPE_BLOCK_EXTENSION    (1 << 18) // the complexType has a 'block' of "extension".
#define XML_SCHEMAS_TYPE_BLOCK_RESTRICTION    (1 << 19) // the complexType has a 'block' of "restriction".
#define XML_SCHEMAS_TYPE_ABSTRACT    (1 << 20) // the simple/complexType is abstract.
#define XML_SCHEMAS_TYPE_FACETSNEEDVALUE    (1 << 21) // indicates if the facets need a computed value
#define XML_SCHEMAS_TYPE_INTERNAL_RESOLVED    (1 << 22) // indicates that the type was typefixed
#define XML_SCHEMAS_TYPE_INTERNAL_INVALID    (1 << 23) // indicates that the type is invalid
#define XML_SCHEMAS_TYPE_WHITESPACE_PRESERVE    (1 << 24) // a whitespace-facet value of "preserve"
#define XML_SCHEMAS_TYPE_WHITESPACE_REPLACE    (1 << 25) // a whitespace-facet value of "replace"
#define XML_SCHEMAS_TYPE_WHITESPACE_COLLAPSE    (1 << 26) // a whitespace-facet value of "collapse"
#define XML_SCHEMAS_TYPE_HAS_FACETS    (1 << 27) // has facets
#define XML_SCHEMAS_TYPE_NORMVALUENEEDED    (1 << 28) // indicates if the facets (pattern) need a normalized value
#define XML_SCHEMAS_TYPE_FIXUP_1    (1 << 29) // First stage of fixup was done.
#define XML_SCHEMAS_TYPE_REDEFINED    (1 << 30) // The type was redefined.
/**
 * XML_SCHEMAS_TYPE_REDEFINING:
 *
 * The type redefines an other type.
 */
/* #define XML_SCHEMAS_TYPE_REDEFINING    (1 << 31) */

/**
 * _xmlSchemaType:
 *
 * Schemas type definition.
 */
struct _xmlSchemaType {
    xmlSchemaTypeType type; /* The kind of type */
    struct _xmlSchemaType *next; /* the next type if in a sequence ... */
    const xmlChar *name;
    const xmlChar *id ; /* Deprecated; not used */
    const xmlChar *ref; /* Deprecated; not used */
    const xmlChar *refNs; /* Deprecated; not used */
    xmlSchemaAnnotPtr annot;
    xmlSchemaType * subtypes;
    xmlSchemaAttribute * attributes; // Deprecated; not used 
    xmlNode * P_Node;
    int minOccurs; /* Deprecated; not used */
    int maxOccurs; /* Deprecated; not used */

    int flags;
    xmlSchemaContentType contentType;
    const xmlChar *base; /* Base type's local name */
    const xmlChar *baseNs; /* Base type's target namespace */
    xmlSchemaType * baseType; /* The base type component */
    xmlSchemaFacet * facets; /* Local facets */
    struct _xmlSchemaType *redef; /* Deprecated; not used */
    int recurse; /* Obsolete */
    xmlSchemaAttributeLink ** attributeUses; // Deprecated; not used 
    xmlSchemaWildcardPtr attributeWildcard;
    /*int*/xmlSchemaValType builtInType; // Type of built-in types. // @sobolev int-->xmlSchemaValType
    xmlSchemaTypeLink * memberTypes; /* member-types if a union type. */
    xmlSchemaFacetLink * facetSet; /* All facets (incl. inherited) */
    const xmlChar *refPrefix; /* Deprecated; not used */
    xmlSchemaType * contentTypeDef; // Used for the simple content of complex types. Could we use @subtypes for this? 
    xmlRegexp * contModel; /* Holds the automaton of the content model */
    const xmlChar *targetNamespace;
    void * attrUses;
};
/*
 * xmlSchemaElement:
 * An element definition.
 *
 * xmlSchemaType, xmlSchemaFacet and xmlSchemaElement start of
 * structures must be kept similar
 */
#define XML_SCHEMAS_ELEM_NILLABLE           1 << 0 // the element is nillable
#define XML_SCHEMAS_ELEM_GLOBAL             1 << 1 // the element is global
#define XML_SCHEMAS_ELEM_DEFAULT            1 << 2 // the element has a default value
#define XML_SCHEMAS_ELEM_FIXED              1 << 3 // the element has a fixed value
#define XML_SCHEMAS_ELEM_ABSTRACT           1 << 4 // the element is abstract
#define XML_SCHEMAS_ELEM_TOPLEVEL           1 << 5 // the element is top level. obsolete: use XML_SCHEMAS_ELEM_GLOBAL instead
#define XML_SCHEMAS_ELEM_REF                1 << 6 // the element is a reference to a type
#define XML_SCHEMAS_ELEM_NSDEFAULT          1 << 7 // allow elements in no namespace. Obsolete, not used anymore.
#define XML_SCHEMAS_ELEM_INTERNAL_RESOLVED  1 << 8 // this is set when "type", "ref", "substitutionGroup" references have been resolved.
#define XML_SCHEMAS_ELEM_CIRCULAR           1 << 9 // a helper flag for the search of circular references.
#define XML_SCHEMAS_ELEM_BLOCK_ABSENT       1 << 10 // the "block" attribute is absent
#define XML_SCHEMAS_ELEM_BLOCK_EXTENSION    1 << 11 // disallowed substitutions are absent
#define XML_SCHEMAS_ELEM_BLOCK_RESTRICTION  1 << 12 // disallowed substitutions: "restriction"
#define XML_SCHEMAS_ELEM_BLOCK_SUBSTITUTION 1 << 13 // disallowed substitutions: "substituion"
#define XML_SCHEMAS_ELEM_FINAL_ABSENT       1 << 14 // substitution group exclusions are absent
#define XML_SCHEMAS_ELEM_FINAL_EXTENSION    1 << 15 // substitution group exclusions: "extension"
#define XML_SCHEMAS_ELEM_FINAL_RESTRICTION  1 << 16 // substitution group exclusions: "restriction"
#define XML_SCHEMAS_ELEM_SUBST_GROUP_HEAD   1 << 17 // the declaration is a substitution group head
#define XML_SCHEMAS_ELEM_INTERNAL_CHECKED   1 << 18 // this is set when the elem decl has been checked against all constraints

typedef struct _xmlSchemaElement xmlSchemaElement;
typedef xmlSchemaElement *xmlSchemaElementPtr;

struct _xmlSchemaElement {
    xmlSchemaTypeType type; /* The kind of type */
    struct _xmlSchemaType *next; /* Not used? */
    const xmlChar *name;
    const xmlChar *id; /* Deprecated; not used */
    const xmlChar *ref; /* Deprecated; not used */
    const xmlChar *refNs; /* Deprecated; not used */
    xmlSchemaAnnot * annot;
    xmlSchemaType * subtypes; /* the type definition */
    xmlSchemaAttribute * attributes;
    xmlNode * P_Node;
    int minOccurs; /* Deprecated; not used */
    int maxOccurs; /* Deprecated; not used */
    int flags;
    const xmlChar *targetNamespace;
    const xmlChar *namedType;
    const xmlChar *namedTypeNs;
    const xmlChar *substGroup;
    const xmlChar *substGroupNs;
    const xmlChar *scope;
    const xmlChar *value; /* The original value of the value constraint. */
    struct _xmlSchemaElement *refDecl; // This will now be used for the substitution group affiliation 
    xmlRegexp * contModel; /* Obsolete for WXS, maybe used for RelaxNG */
    xmlSchemaContentType contentType;
    const xmlChar *refPrefix; /* Deprecated; not used */
    xmlSchemaVal * defVal; /* The compiled value contraint. */
    void *idcs; /* The identity-constraint defs */
};

#define XML_SCHEMAS_FACET_UNKNOWN        0 // unknown facet handling
#define XML_SCHEMAS_FACET_PRESERVE        1 // preserve the type of the facet
#define XML_SCHEMAS_FACET_REPLACE        2 // replace the type of the facet
#define XML_SCHEMAS_FACET_COLLAPSE        3 // collapse the types of the facet
/**
 * A facet definition.
 */
struct _xmlSchemaFacet {
    xmlSchemaTypeType type; /* The kind of type */
    struct _xmlSchemaFacet *next; /* the next type if in a sequence ... */
    const xmlChar * value; /* The original value */
    const xmlChar * id; /* Obsolete */
    xmlSchemaAnnotPtr annot;
    xmlNode * P_Node;
    int fixed; /* XML_SCHEMAS_FACET_PRESERVE, etc. */
    int whitespace;
    xmlSchemaVal * val; /* The compiled value */
    xmlRegexp * regexp; /* The regex for patterns */
};
/**
 * A notation definition.
 */
typedef struct _xmlSchemaNotation xmlSchemaNotation;
typedef xmlSchemaNotation *xmlSchemaNotationPtr;
struct _xmlSchemaNotation {
    xmlSchemaTypeType type; /* The kind of type */
    const xmlChar *name;
    xmlSchemaAnnotPtr annot;
    const xmlChar *identifier;
    const xmlChar *targetNamespace;
};
/*
* @todo Actually all those flags used for the schema should sit
* on the schema parser context, since they are used only
* during parsing an XML schema document, and not available
* on the component level as per spec.
*/
#define XML_SCHEMAS_QUALIF_ELEM                1 << 0 // Reflects elementFormDefault == qualified in an XML schema document.
#define XML_SCHEMAS_QUALIF_ATTR                1 << 1 // Reflects attributeFormDefault == qualified in an XML schema document.
#define XML_SCHEMAS_FINAL_DEFAULT_EXTENSION    1 << 2 // the schema has "extension" in the set of finalDefault.
#define XML_SCHEMAS_FINAL_DEFAULT_RESTRICTION  1 << 3 // the schema has "restriction" in the set of finalDefault.
#define XML_SCHEMAS_FINAL_DEFAULT_LIST         1 << 4 // the cshema has "list" in the set of finalDefault.
#define XML_SCHEMAS_FINAL_DEFAULT_UNION        1 << 5 // the schema has "union" in the set of finalDefault.
#define XML_SCHEMAS_BLOCK_DEFAULT_EXTENSION    1 << 6 // the schema has "extension" in the set of blockDefault.
#define XML_SCHEMAS_BLOCK_DEFAULT_RESTRICTION  1 << 7 // the schema has "restriction" in the set of blockDefault.
#define XML_SCHEMAS_BLOCK_DEFAULT_SUBSTITUTION 1 << 8 // the schema has "substitution" in the set of blockDefault.
#define XML_SCHEMAS_INCLUDING_CONVERT_NS       1 << 9 // the schema is currently including an other schema with no target namespace.
/**
 * _xmlSchema:
 *
 * A Schemas definition
 */
struct _xmlSchema {
    const xmlChar * name; /* schema name */
    const xmlChar * targetNamespace; /* the target namespace */
    const xmlChar * version;
    const xmlChar * id; /* Obsolete */
    xmlDoc * doc;
    xmlSchemaAnnot * annot;
    int    flags;
    xmlHashTable * typeDecl;
    xmlHashTable * attrDecl;
    xmlHashTable * attrgrpDecl;
    xmlHashTable * elemDecl;
    xmlHashTable * notaDecl;
    xmlHashTable * schemasImports;
    void * _private; /* unused by the library for users or bindings */
    xmlHashTable * groupDecl;
    xmlDict * dict;
    void * includes;  // the includes, this is opaque for now 
    bool   Preserve_; // whether to free the document // @sobolev preserve-->Preserve_
    int    counter;   // used to give ononymous components unique names 
    xmlHashTable * idcDef; /* All identity-constraint defs. */
    void * volatiles; /* Obsolete */
};

XMLPUBFUN void xmlSchemaFreeType(xmlSchemaType * type);
XMLPUBFUN void xmlSchemaFreeWildcard(xmlSchemaWildcardPtr wildcard);

#ifdef __cplusplus
}
#endif

#endif /* LIBXML_SCHEMAS_ENABLED */
#endif /* __XML_SCHEMA_INTERNALS_H__ */
