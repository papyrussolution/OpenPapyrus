// PARSER.H
// Summary: the core parser module
// Description: Interfaces, constants and types related to the XML parser
// Copy: See Copyright for the status of this software.
// Author: Daniel Veillard
// 
#ifndef __XML_PARSER_H__
#define __XML_PARSER_H__

struct xmlHashTable;
struct xmlValidCtxt;

#include <libxml/valid.h>
#include <libxml/encoding.h>
#include <libxml/xmlIO.h>

//#ifdef __cplusplus
//extern "C" {
//#endif
/**
 * XML_DEFAULT_VERSION:
 *
 * The default version of XML used: 1.0
 */
#define XML_DEFAULT_VERSION     "1.0"
/**
 * xmlParserInput:
 *
 * An xmlParserInput is an input flow for the XML processor.
 * Each entity parsed is associated an xmlParserInput (except the
 * few predefined ones). This is the case both for internal entities
 * - in which case the flow is already completely in memory - or
 * external entities - in which case we use the buf structure for
 * progressive reading and I18N conversions to the internal UTF-8 format.
 */

/**
 * xmlParserInputDeallocate:
 * @str:  the string to deallocate
 *
 * Callback for freeing some parser input allocations.
 */
typedef void (*xmlParserInputDeallocate)(xmlChar * str);

struct xmlParserInput {
	// Input buffer 
	const  xmlChar * cur;          // Current char being parsed 
	xmlParserInputBuffer * buf;   // UTF-8 encoded buffer 
	const  char * filename; // The file analyzed, if any 
	const  char * directory; // the directory/base of the file 
	const  xmlChar * base; // Base of the array to parse 
	const  xmlChar * end; // end of the array to parse 
	int    length; // length if known 
	int    line;   // Current line 
	int    col;    // Current column 
	// 
	// NOTE: consumed is only tested for equality in the parser code,
	//   so even if there is an overflow this should not give troubles for parsing very large instances.
	// 
	ulong consumed; // How many xmlChars already consumed 
	xmlParserInputDeallocate FnFree; // function to deallocate the base 
	const xmlChar * encoding; // the encoding string for entity
	const xmlChar * version; // the version string for entity
	int    standalone; // Was that entity marked standalone
	int    id; // an unique identifier for the entity 
};
/**
 * xmlParserNodeInfo:
 *
 * The parser can be asked to collect Node informations, i.e. at what
 * place in the file they were detected.
 * NOTE: This is off by default and not very well tested.
 */
typedef struct _xmlParserNodeInfo xmlParserNodeInfo;
typedef xmlParserNodeInfo * xmlParserNodeInfoPtr;

struct _xmlParserNodeInfo {
	const xmlNode * P_Node;
	// Position & line # that text that created the node begins & ends on 
	ulong  begin_pos;
	ulong  begin_line;
	ulong  end_pos;
	ulong  end_line;
};

typedef struct _xmlParserNodeInfoSeq xmlParserNodeInfoSeq;
typedef xmlParserNodeInfoSeq * xmlParserNodeInfoSeqPtr;

struct _xmlParserNodeInfoSeq {
	ulong  maximum;
	ulong  length;
	xmlParserNodeInfo * buffer;
};
/**
 * xmlParserInputState:
 *
 * The parser is now working also as a state based parser.
 * The recursive one use the state info for entities processing.
 */
enum xmlParserInputState {
	XML_PARSER_EOF = -1,    /* nothing is to be parsed */
	XML_PARSER_START = 0,   /* nothing has been parsed */
	XML_PARSER_MISC,        /* Misc* before int subset */
	XML_PARSER_PI,          /* Within a processing instruction */
	XML_PARSER_DTD,         /* within some DTD content */
	XML_PARSER_PROLOG,      /* Misc* after internal subset */
	XML_PARSER_COMMENT,     /* within a comment */
	XML_PARSER_START_TAG,   /* within a start tag */
	XML_PARSER_CONTENT,     /* within the content */
	XML_PARSER_CDATA_SECTION, /* within a CDATA section */
	XML_PARSER_END_TAG,     /* within a closing tag */
	XML_PARSER_ENTITY_DECL, /* within an entity declaration */
	XML_PARSER_ENTITY_VALUE, /* within an entity value in a decl */
	XML_PARSER_ATTRIBUTE_VALUE, /* within an attribute value */
	XML_PARSER_SYSTEM_LITERAL, /* within a SYSTEM value */
	XML_PARSER_EPILOG,      /* the Misc* after the last end tag */
	XML_PARSER_IGNORE,      /* within an IGNORED section */
	XML_PARSER_PUBLIC_LITERAL /* within a PUBLIC value */
};
/**
 * XML_DETECT_IDS:
 *
 * Bit in the loadsubset context field to tell to do ID/REFs lookups.
 * Use it to initialize xmlLoadExtDtdDefaultValue.
 */
#define XML_DETECT_IDS          2
/**
 * XML_COMPLETE_ATTRS:
 *
 * Bit in the loadsubset context field to tell to do complete the
 * elements attributes lists with the ones defaulted from the DTDs.
 * Use it to initialize xmlLoadExtDtdDefaultValue.
 */
#define XML_COMPLETE_ATTRS      4
/**
 * XML_SKIP_IDS:
 *
 * Bit in the loadsubset context field to tell to not do ID/REFs registration.
 * Used to initialize xmlLoadExtDtdDefaultValue in some special cases.
 */
#define XML_SKIP_IDS            8
/**
 * xmlParserMode:
 *
 * A parser can operate in various modes
 */
enum xmlParserMode {
	XML_PARSE_UNKNOWN = 0,
	XML_PARSE_DOM = 1,
	XML_PARSE_SAX = 2,
	XML_PARSE_PUSH_DOM = 3,
	XML_PARSE_PUSH_SAX = 4,
	XML_PARSE_READER = 5
};
// 
// Descr: The parser context.
// NOTE This doesn't completely define the parser state, the (current ?)
//   design of the parser uses recursive function calls since this allow
//   and easy mapping from the production rules of the specification
//   to the actual code. The drawback is that the actual function call
//   also reflect the parser state. However most of the parsing routines
//   takes as the only argument the parser context pointer, so migrating
//   to a state based parser for progressive parsing shouldn't be too hard.
//   
struct xmlParserCtxt {
	static bool FASTCALL IsEofInNonSaxMode(const xmlParserCtxt * pCtx) { return (pCtx && pCtx->disableSAX && pCtx->IsEof()); }
	bool   IsEof() const { return instate == XML_PARSER_EOF; }
	bool   IsSaxInUse() const { return (sax && !disableSAX); }
	xmlParserInput * input;          // Current input stream
	struct xmlSAXHandler * sax;      // The SAX handler
	void * userData;                 // For SAX interface only, used by DOM build
	xmlDoc * myDoc;                  // the document being built
	int    wellFormed;               // is the document well formed
	int    replaceEntities;          // shall we replace entities ?
	const  xmlChar * version;        // the XML version string
	const  xmlChar * encoding;       // the declared encoding, if any
	int    standalone;               // standalone document
	int    html;                     // an HTML(1)/Docbook(2) document 3 is HTML after <head> 10 is HTML after <body>
	// Input stream stack
	int    inputNr;                  // Number of current input streams
	int    inputMax;                 // Max number of input streams
	xmlParserInput ** inputTab;      // stack of inputs
	// Node analysis stack only used for DOM building
	xmlNode * P_Node;                // Current parsed Node
	int    nodeNr;                   // Depth of the parsing stack
	int    nodeMax;                  // Max depth of the parsing stack
	xmlNode ** PP_NodeTab;           // array of nodes
	int    record_info;              // Whether node info should be kept
	xmlParserNodeInfoSeq node_seq;   // info about each node parsed
	int    errNo;                    // error code
	int    hasExternalSubset;        // reference and external subset
	int    hasPErefs;                // the internal subset has PE refs
	int    external;                 // are we parsing an external entity
	int    valid;                    // is the document valid
	int    validate;                 // shall we try to validate ?
	xmlValidCtxt vctxt;              // The validity context
	xmlParserInputState instate;     // current type of input
	int    token;                    // next char look-ahead
	char * directory;   // the data directory
	// Node name stack
	const  xmlChar * name;           // Current parsed Node
	int    nameNr;                   // Depth of the parsing stack
	int    nameMax;                  // Max depth of the parsing stack
	const  xmlChar ** nameTab;       // array of nodes
	long   nbChars;                  // number of xmlChar processed
	long   CheckIndex;               // used by progressive parsing lookup
	int    keepBlanks;               // (bool) ugly but ...
	int    disableSAX;               // (bool) SAX callbacks are disabled
	int    pedantic;                 // (bool) signal pedantic warnings
	int    recovery;                 // (bool) run in recovery mode
	int    docdict;                  // (bool) use strings from dict to build tree
	int    nsWellFormed;             // (bool) is the document XML Nanespace okay
	int    inSubset;                 // Parsing is in int 1/ext 2 subset
	const xmlChar * intSubName; // name of subset
	xmlChar * extSubURI; // URI of external subset
	xmlChar * extSubSystem; // SYSTEM ID of external subset
	// xml:space values
	int  * space;                    // Should the parser preserve spaces
	int    spaceNr;                  // Depth of the parsing stack
	int    spaceMax;                 // Max depth of the parsing stack
	int  * spaceTab;                 // array of space infos
	int    depth;                    // to prevent entity substitution loops
	xmlParserInput * entity;         // used to check entities boundaries
	int    charset;                  // encoding of the in-memory content actually an xmlCharEncoding
	int    nodelen;                  // Those two fields are there to
	int    nodemem;                  // Speed up large node parsing
	void * _private;                 // For user data, libxml won't touch it
	int    loadsubset;               // should the external subset be loaded
	int    linenumbers;              // set line number in element content
	void * catalogs;                 // document's own catalog
	int    progressive;              // is this a progressive parsing
	xmlDict * dict;                  // dictionnary for the parser
	const xmlChar ** atts;           // array for the attributes callbacks
	int    maxatts;                  // the size of the array
	//
	// pre-interned strings
	//
	const  xmlChar * str_xml;
	const  xmlChar * str_xmlns;
	const  xmlChar * str_xml_ns;
	//
	// Everything below is used only by the new SAX mode
	//
	int    sax2;                  // operating in the new SAX mode
	int    nsNr;                  // the number of inherited namespaces
	int    nsMax;                 // the size of the arrays
	const  xmlChar ** nsTab;       // the array of prefix/namespace name
	int  * attallocs;             // which attribute were allocated
	void ** pushTab;              // array of data for push
	xmlHashTable * attsDefault;  // defaulted attributes if any
	xmlHashTable * attsSpecial;  // non-CDATA attributes if any
	int    options;                  // Extra options
	//
	// Those fields are needed only for treaming parsing so far
	//
	int    dictNames;              // Use dictionary names for the tree
	int    freeElemsNr;            // number of freed element nodes
	xmlNode * freeElems;       // List of freed element nodes
	int    freeAttrsNr;            // number of freed attributes nodes
	xmlAttr * freeAttrs;       // List of freed attributes nodes
	//
	// the complete error informations for the last error.
	//
	xmlError lastError;
	xmlParserMode parseMode;    // the parser mode
	ulong  nbentities;   // number of entities references
	ulong  sizeentities; // size of parsed entities
	// for use by HTML non-recursive parser
	xmlParserNodeInfo * nodeInfo; // Current NodeInfo
	int    nodeInfoNr;               // Depth of the parsing stack
	int    nodeInfoMax;              // Max depth of the parsing stack
	xmlParserNodeInfo * nodeInfoTab; // array of nodeInfos
	int    input_id;                 // we need to label inputs
	ulong  sizeentcopy;    // volume of entity copy
};

typedef xmlParserCtxt htmlParserCtxt;
//
// Descr: A SAX Locator
//
struct xmlSAXLocator {
	const xmlChar *(*getPublicId)(void * ctx);
	const xmlChar *(*getSystemId)(void * ctx);
	int (* getLineNumber)(void * ctx);
	int (* getColumnNumber)(void * ctx);
};
/**
 * xmlSAXHandler:
 *
 * A SAX handler is bunch of callbacks called by the parser when processing
 * of the input generate data or structure informations.
 */
/**
 * resolveEntitySAXFunc:
 * @ctx:  the user data (XML parser context)
 * @publicId: The public ID of the entity
 * @systemId: The system ID of the entity
 *
 * Callback:
 * The entity loader, to control the loading of external entities,
 * the application can either:
 *  - override this resolveEntity() callback in the SAX block
 *  - or better use the xmlSetExternalEntityLoader() function to
 * set up it's own entity resolution routine
 *
 * Returns the xmlParserInputPtr if inlined or NULL for DOM behaviour.
 */
typedef xmlParserInput * (*resolveEntitySAXFunc)(void * ctx, const xmlChar * publicId, const xmlChar * systemId);
/**
 * internalSubsetSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @name:  the root element name
 * @ExternalID:  the external ID
 * @SystemID:  the SYSTEM ID (e.g. filename or URL)
 *
 * Callback on internal subset declaration.
 */
typedef void (*internalSubsetSAXFunc)(void * ctx, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID);
/**
 * externalSubsetSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @name:  the root element name
 * @ExternalID:  the external ID
 * @SystemID:  the SYSTEM ID (e.g. filename or URL)
 *
 * Callback on external subset declaration.
 */
typedef void (*externalSubsetSAXFunc)(void * ctx, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID);
/**
 * getEntitySAXFunc:
 * @ctx:  the user data (XML parser context)
 * @name: The entity name
 *
 * Get an entity by name.
 *
 * Returns the xmlEntityPtr if found.
 */
typedef xmlEntity * (*getEntitySAXFunc)(void * ctx, const xmlChar * name);
/**
 * getParameterEntitySAXFunc:
 * @ctx:  the user data (XML parser context)
 * @name: The entity name
 *
 * Get a parameter entity by name.
 *
 * Returns the xmlEntityPtr if found.
 */
typedef xmlEntity * (*getParameterEntitySAXFunc)(void * ctx, const xmlChar * name);
/**
 * entityDeclSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @name:  the entity name
 * @type:  the entity type
 * @publicId: The public ID of the entity
 * @systemId: The system ID of the entity
 * @content: the entity value (without processing).
 *
 * An entity definition has been parsed.
 */
typedef void (*entityDeclSAXFunc)(void * ctx, const xmlChar * name, int type, const xmlChar * publicId, const xmlChar * systemId, xmlChar * content);
/**
 * notationDeclSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @name: The name of the notation
 * @publicId: The public ID of the entity
 * @systemId: The system ID of the entity
 *
 * What to do when a notation declaration has been parsed.
 */
typedef void (*notationDeclSAXFunc)(void * ctx, const xmlChar * name, const xmlChar * publicId, const xmlChar * systemId);
/**
 * attributeDeclSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @elem:  the name of the element
 * @fullname:  the attribute name
 * @type:  the attribute type
 * @def:  the type of default value
 * @defaultValue: the attribute default value
 * @tree:  the tree of enumerated value set
 *
 * An attribute definition has been parsed.
 */
typedef void (*attributeDeclSAXFunc)(void * ctx, const xmlChar * elem, const xmlChar * fullname, int type, int def, const xmlChar * defaultValue, xmlEnumeration * tree);
/**
 * elementDeclSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @name:  the element name
 * @type:  the element type
 * @content: the element value tree
 *
 * An element definition has been parsed.
 */
typedef void (*elementDeclSAXFunc)(void * ctx, const xmlChar * name, int type, xmlElementContent * content);
/**
 * unparsedEntityDeclSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @name: The name of the entity
 * @publicId: The public ID of the entity
 * @systemId: The system ID of the entity
 * @notationName: the name of the notation
 *
 * What to do when an unparsed entity declaration is parsed.
 */
typedef void (*unparsedEntityDeclSAXFunc)(void * ctx, const xmlChar * name, const xmlChar * publicId, const xmlChar * systemId, const xmlChar * notationName);
/**
 * setDocumentLocatorSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @loc: A SAX Locator
 *
 * Receive the document locator at startup, actually xmlDefaultSAXLocator.
 * Everything is available on the context, so this is useless in our case.
 */
typedef void (*setDocumentLocatorSAXFunc)(void * ctx, xmlSAXLocator * loc);
/**
 * startDocumentSAXFunc:
 * @ctx:  the user data (XML parser context)
 *
 * Called when the document start being processed.
 */
typedef void (*startDocumentSAXFunc)(void * ctx);
/**
 * endDocumentSAXFunc:
 * @ctx:  the user data (XML parser context)
 *
 * Called when the document end has been detected.
 */
typedef void (*endDocumentSAXFunc)(void * ctx);
/**
 * startElementSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @name:  The element name, including namespace prefix
 * @atts:  An array of name/value attributes pairs, NULL terminated
 *
 * Called when an opening tag has been processed.
 */
typedef void (*startElementSAXFunc)(void * ctx, const xmlChar * name, const xmlChar ** atts);
/**
 * endElementSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @name:  The element name
 *
 * Called when the end of an element has been detected.
 */
typedef void (*endElementSAXFunc)(void * ctx, const xmlChar * name);
/**
 * attributeSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @name:  The attribute name, including namespace prefix
 * @value:  The attribute value
 *
 * Handle an attribute that has been read by the parser.
 * The default handling is to convert the attribute into an
 * DOM subtree and past it in a new xmlAttr element added to
 * the element.
 */
typedef void (*attributeSAXFunc)(void * ctx, const xmlChar * name, const xmlChar * value);
/**
 * referenceSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @name:  The entity name
 *
 * Called when an entity reference is detected.
 */
typedef void (*referenceSAXFunc)(void * ctx, const xmlChar * name);
/**
 * charactersSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @ch:  a xmlChar string
 * @len: the number of xmlChar
 *
 * Receiving some chars from the parser.
 */
typedef void (*charactersSAXFunc)(void * ctx, const xmlChar * ch, int len);
/**
 * ignorableWhitespaceSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @ch:  a xmlChar string
 * @len: the number of xmlChar
 *
 * Receiving some ignorable whitespaces from the parser.
 * UNUSED: by default the DOM building will use characters.
 */
typedef void (*ignorableWhitespaceSAXFunc)(void * ctx, const xmlChar * ch, int len);
/**
 * processingInstructionSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @target:  the target name
 * @data: the PI data's
 *
 * A processing instruction has been parsed.
 */
typedef void (*processingInstructionSAXFunc)(void * ctx, const xmlChar * target, const xmlChar * data);
/**
 * commentSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @value:  the comment content
 *
 * A comment has been parsed.
 */
typedef void (*commentSAXFunc)(void * ctx, const xmlChar * value);
/**
 * cdataBlockSAXFunc:
 * @ctx:  the user data (XML parser context)
 * @value:  The pcdata content
 * @len:  the block length
 *
 * Called when a pcdata block has been parsed.
 */
typedef void (*cdataBlockSAXFunc)(void * ctx, const xmlChar * value, int len);
/**
 * warningSAXFunc:
 * @ctx:  an XML parser context
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 *
 * Display and format a warning messages, callback.
 */
typedef void (XMLCDECL *warningSAXFunc)(void * ctx, const char * msg, ...) LIBXML_ATTR_FORMAT(2, 3);
/**
 * errorSAXFunc:
 * @ctx:  an XML parser context
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 *
 * Display and format an error messages, callback.
 */
typedef void (XMLCDECL *errorSAXFunc)(void * ctx, const char * msg, ...) LIBXML_ATTR_FORMAT(2, 3);
/**
 * fatalErrorSAXFunc:
 * @ctx:  an XML parser context
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 *
 * Display and format fatal error messages, callback.
 * Note: so far fatalError() SAX callbacks are not used, error()
 *  get all the callbacks for errors.
 */
typedef void (XMLCDECL *fatalErrorSAXFunc)(void * ctx, const char * msg, ...) LIBXML_ATTR_FORMAT(2, 3);
/**
 * isStandaloneSAXFunc:
 * @ctx:  the user data (XML parser context)
 *
 * Is this document tagged standalone?
 *
 * Returns 1 if true
 */
typedef int (*isStandaloneSAXFunc)(void * ctx);
/**
 * hasInternalSubsetSAXFunc:
 * @ctx:  the user data (XML parser context)
 *
 * Does this document has an internal subset.
 *
 * Returns 1 if true
 */
typedef int (*hasInternalSubsetSAXFunc)(void * ctx);
/**
 * hasExternalSubsetSAXFunc:
 * @ctx:  the user data (XML parser context)
 *
 * Does this document has an external subset?
 *
 * Returns 1 if true
 */
typedef int (*hasExternalSubsetSAXFunc)(void * ctx);
// 
// The SAX version 2 API extensions
// 
/**
 * XML_SAX2_MAGIC:
 *
 * Special constant found in SAX2 blocks initialized fields
 */
#define XML_SAX2_MAGIC 0xDEEDBEAF

/**
 * startElementNsSAX2Func:
 * @ctx:  the user data (XML parser context)
 * @localname:  the local name of the element
 * @prefix:  the element namespace prefix if available
 * @URI:  the element namespace name if available
 * @nb_namespaces:  number of namespace definitions on that node
 * @namespaces:  pointer to the array of prefix/URI pairs namespace definitions
 * @nb_attributes:  the number of attributes on that node
 * @nb_defaulted:  the number of defaulted attributes. The defaulted
 *       ones are at the end of the array
 * @attributes:  pointer to the array of (localname/prefix/URI/value/end)
 *    attribute values.
 *
 * SAX2 callback when an element start has been detected by the parser.
 * It provides the namespace informations for the element, as well as
 * the new namespace declarations on the element.
 */
typedef void (*startElementNsSAX2Func)(void * ctx, const xmlChar * localname, const xmlChar * prefix, const xmlChar * URI,
    int nb_namespaces, const xmlChar ** namespaces, int nb_attributes, int nb_defaulted, const xmlChar ** attributes);
/**
 * endElementNsSAX2Func:
 * @ctx:  the user data (XML parser context)
 * @localname:  the local name of the element
 * @prefix:  the element namespace prefix if available
 * @URI:  the element namespace name if available
 *
 * SAX2 callback when an element end has been detected by the parser.
 * It provides the namespace informations for the element.
 */

typedef void (*endElementNsSAX2Func)(void * ctx, const xmlChar * localname, const xmlChar * prefix, const xmlChar * URI);

struct xmlSAXHandler {
	xmlSAXHandler();
	internalSubsetSAXFunc internalSubset;
	isStandaloneSAXFunc isStandalone;
	hasInternalSubsetSAXFunc hasInternalSubset;
	hasExternalSubsetSAXFunc hasExternalSubset;
	resolveEntitySAXFunc resolveEntity;
	getEntitySAXFunc getEntity;
	entityDeclSAXFunc entityDecl;
	notationDeclSAXFunc notationDecl;
	attributeDeclSAXFunc attributeDecl;
	elementDeclSAXFunc elementDecl;
	unparsedEntityDeclSAXFunc unparsedEntityDecl;
	setDocumentLocatorSAXFunc setDocumentLocator;
	startDocumentSAXFunc startDocument;
	endDocumentSAXFunc endDocument;
	startElementSAXFunc startElement;
	endElementSAXFunc endElement;
	referenceSAXFunc reference;
	charactersSAXFunc characters;
	ignorableWhitespaceSAXFunc ignorableWhitespace;
	processingInstructionSAXFunc processingInstruction;
	commentSAXFunc comment;
	warningSAXFunc warning;
	errorSAXFunc error;
	fatalErrorSAXFunc fatalError; // unused error() get all the errors 
	getParameterEntitySAXFunc getParameterEntity;
	cdataBlockSAXFunc cdataBlock;
	externalSubsetSAXFunc externalSubset;
	uint initialized;
	// The following fields are extensions available only on version 2 
	void * _private;
	startElementNsSAX2Func startElementNs;
	endElementNsSAX2Func endElementNs;
	xmlStructuredErrorFunc serror;
};
/*
 * SAX Version 1
 */
//typedef struct _xmlSAXHandlerV1 xmlSAXHandlerV1;

struct xmlSAXHandlerV1 {
	internalSubsetSAXFunc internalSubset;
	isStandaloneSAXFunc isStandalone;
	hasInternalSubsetSAXFunc hasInternalSubset;
	hasExternalSubsetSAXFunc hasExternalSubset;
	resolveEntitySAXFunc resolveEntity;
	getEntitySAXFunc getEntity;
	entityDeclSAXFunc entityDecl;
	notationDeclSAXFunc notationDecl;
	attributeDeclSAXFunc attributeDecl;
	elementDeclSAXFunc elementDecl;
	unparsedEntityDeclSAXFunc unparsedEntityDecl;
	setDocumentLocatorSAXFunc setDocumentLocator;
	startDocumentSAXFunc startDocument;
	endDocumentSAXFunc endDocument;
	startElementSAXFunc startElement;
	endElementSAXFunc endElement;
	referenceSAXFunc reference;
	charactersSAXFunc characters;
	ignorableWhitespaceSAXFunc ignorableWhitespace;
	processingInstructionSAXFunc processingInstruction;
	commentSAXFunc comment;
	warningSAXFunc warning;
	errorSAXFunc error;
	fatalErrorSAXFunc fatalError; /* unused error() get all the errors */
	getParameterEntitySAXFunc getParameterEntity;
	cdataBlockSAXFunc cdataBlock;
	externalSubsetSAXFunc externalSubset;
	uint initialized;
};
//typedef xmlSAXHandlerV1 * xmlSAXHandlerV1Ptr;
/**
 * xmlExternalEntityLoader:
 * @URL: The System ID of the resource requested
 * @ID: The Public ID of the resource requested
 * @context: the XML parser context
 *
 * External entity loaders types.
 *
 * Returns the entity input parser.
 */
typedef xmlParserInput * (*xmlExternalEntityLoader)(const char * URL, const char * ID, xmlParserCtxt * context);
/*
 * Init/Cleanup
 */
void xmlInitParser();
void xmlCleanupParser();
/*
 * Input functions
 */
int xmlParserInputRead(xmlParserInput * in, int len);
int FASTCALL xmlParserInputGrow(xmlParserInput * in, int len);
/*
 * Basic parsing Interfaces
 */
#ifdef LIBXML_SAX1_ENABLED
	xmlDoc * xmlParseDoc(const xmlChar * cur);
	xmlDoc * xmlParseFile(const char * filename);
	xmlDoc * xmlParseMemory(const char * buffer, int size);
#endif /* LIBXML_SAX1_ENABLED */
int xmlSubstituteEntitiesDefault(int val);
int xmlKeepBlanksDefault(int val);
void xmlStopParser(xmlParserCtxt * ctxt);
int xmlPedanticParserDefault(int val);
int xmlLineNumbersDefault(int val);
#ifdef LIBXML_SAX1_ENABLED
	/*
	 * Recovery mode
	 */
	xmlDoc * xmlRecoverDoc(const xmlChar * cur);
	xmlDoc * xmlRecoverMemory(const char * buffer, int size);
	xmlDoc * xmlRecoverFile(const char * filename);
#endif /* LIBXML_SAX1_ENABLED */
/*
 * Less common routines and SAX interfaces
 */
int xmlParseDocument(xmlParserCtxt * ctxt);
int xmlParseExtParsedEnt(xmlParserCtxt * ctxt);
#ifdef LIBXML_SAX1_ENABLED
	int xmlSAXUserParseFile(xmlSAXHandler * sax, void * user_data, const char * filename);
	int xmlSAXUserParseMemory(xmlSAXHandler * sax, void * user_data, const char * buffer, int size);
	xmlDoc * xmlSAXParseDoc(xmlSAXHandler * sax, const xmlChar * cur, int recovery);
	xmlDoc * xmlSAXParseMemory(xmlSAXHandler * sax, const char * buffer, int size, int recovery);
	xmlDoc * xmlSAXParseMemoryWithData(xmlSAXHandler * sax, const char * buffer, int size, int recovery, void * data);
	xmlDoc * xmlSAXParseFile(xmlSAXHandler * sax, const char * filename, int recovery);
	xmlDoc * xmlSAXParseFileWithData(xmlSAXHandler * sax, const char * filename, int recovery, void * data);
	xmlDoc * xmlSAXParseEntity(xmlSAXHandler * sax, const char * filename);
	xmlDoc * xmlParseEntity(const char * filename);
#endif /* LIBXML_SAX1_ENABLED */
#ifdef LIBXML_VALID_ENABLED
	xmlDtd * xmlSAXParseDTD(xmlSAXHandler * sax, const xmlChar * ExternalID, const xmlChar * SystemID);
	xmlDtd * xmlParseDTD(const xmlChar * ExternalID, const xmlChar * SystemID);
	xmlDtd * xmlIOParseDTD(xmlSAXHandler * sax, xmlParserInputBuffer * input, xmlCharEncoding enc);
#endif /* LIBXML_VALID_ENABLE */
#ifdef LIBXML_SAX1_ENABLED
	int xmlParseBalancedChunkMemory(xmlDoc * doc, xmlSAXHandler * sax, void * user_data, int depth, const xmlChar * string, xmlNode ** lst);
#endif /* LIBXML_SAX1_ENABLED */
xmlParserErrors xmlParseInNodeContext(xmlNode * P_Node, const char * data, int datalen, int options, xmlNode ** lst);
#ifdef LIBXML_SAX1_ENABLED
	int xmlParseBalancedChunkMemoryRecover(xmlDoc * doc, xmlSAXHandler * sax, void * user_data, int depth, const xmlChar * string, xmlNode ** lst, int recover);
	int xmlParseExternalEntity(xmlDoc * doc, xmlSAXHandler * sax, void * user_data, int depth, const xmlChar * URL, const xmlChar * ID, xmlNode ** lst);
#endif /* LIBXML_SAX1_ENABLED */
int xmlParseCtxtExternalEntity(xmlParserCtxt * ctx, const xmlChar * URL, const xmlChar * ID, xmlNode ** lst);
/*
 * Parser contexts handling.
 */
xmlParserCtxt * xmlNewParserCtxt();
int xmlInitParserCtxt(xmlParserCtxt * ctxt);
void xmlClearParserCtxt(xmlParserCtxt * ctxt);
void FASTCALL xmlFreeParserCtxt(xmlParserCtxt * ctxt);
#ifdef LIBXML_SAX1_ENABLED
	void xmlSetupParserForBuffer(xmlParserCtxt * ctxt, const xmlChar* buffer, const char * filename);
#endif /* LIBXML_SAX1_ENABLED */
xmlParserCtxt * xmlCreateDocParserCtxt(const xmlChar * cur);
#ifdef LIBXML_LEGACY_ENABLED
	/*
	 * Reading/setting optional parsing features.
	 */
	int xmlGetFeaturesList(int * len, const char ** result);
	int xmlGetFeature(xmlParserCtxt * ctxt, const char * name, void * result);
	int xmlSetFeature(xmlParserCtxt * ctxt, const char * name, void * value);
#endif /* LIBXML_LEGACY_ENABLED */
#ifdef LIBXML_PUSH_ENABLED
	/*
	 * Interfaces for the Push mode.
	 */
	xmlParserCtxt * xmlCreatePushParserCtxt(xmlSAXHandler * sax, void * user_data, const char * chunk, int size, const char * filename);
	int FASTCALL xmlParseChunk(xmlParserCtxt * ctxt, const char * chunk, int size, int terminate);
#endif /* LIBXML_PUSH_ENABLED */
// 
// Special I/O mode.
// 
typedef int (* xmlInputReadCallback)(void * context, char * buffer, int len); // @sobolev dup of xmlIO.h declaration
typedef int (* xmlInputCloseCallback)(void * context); // @sobolev dup of xmlIO.h declaration

xmlParserCtxt * xmlCreateIOParserCtxt(xmlSAXHandler * sax, void * user_data, xmlInputReadCallback ioread,
    xmlInputCloseCallback ioclose, void * ioctx, xmlCharEncoding enc);
xmlParserInput * xmlNewIOInputStream(xmlParserCtxt * ctxt, xmlParserInputBuffer * input, xmlCharEncoding enc);
/*
 * Node infos.
 */
const xmlParserNodeInfo* xmlParserFindNodeInfo(const xmlParserCtxt * ctxt, const xmlNode * P_Node);
void xmlInitNodeInfoSeq(xmlParserNodeInfoSeqPtr seq);
void xmlClearNodeInfoSeq(xmlParserNodeInfoSeqPtr seq);
ulong xmlParserFindNodeInfoIndex(const xmlParserNodeInfoSeq * seq, const xmlNode * P_Node);
void xmlParserAddNodeInfo(xmlParserCtxt * ctxt, const xmlParserNodeInfoPtr info);
/*
 * External entities handling actually implemented in xmlIO.
 */
void xmlSetExternalEntityLoader(xmlExternalEntityLoader f);
xmlExternalEntityLoader xmlGetExternalEntityLoader();
xmlParserInput * xmlLoadExternalEntity(const char * URL, const char * ID, xmlParserCtxt * ctxt);
/*
 * Index lookup, actually implemented in the encoding module
 */
long xmlByteConsumed(xmlParserCtxt * ctxt);
/*
 * New set of simpler/more flexible APIs
 */
/**
 * xmlParserOption:
 *
 * This is the set of XML parser options that can be passed down
 * to the xmlReadDoc() and similar calls.
 */
typedef enum {
	XML_PARSE_RECOVER   = 1<<0, /* recover on errors */
	XML_PARSE_NOENT     = 1<<1, /* substitute entities */
	XML_PARSE_DTDLOAD   = 1<<2, /* load the external subset */
	XML_PARSE_DTDATTR   = 1<<3, /* default DTD attributes */
	XML_PARSE_DTDVALID  = 1<<4, /* validate with the DTD */
	XML_PARSE_NOERROR   = 1<<5, /* suppress error reports */
	XML_PARSE_NOWARNING = 1<<6, /* suppress warning reports */
	XML_PARSE_PEDANTIC  = 1<<7, /* pedantic error reporting */
	XML_PARSE_NOBLANKS  = 1<<8, /* remove blank nodes */
	XML_PARSE_SAX1      = 1<<9, /* use the SAX1 interface internally */
	XML_PARSE_XINCLUDE  = 1<<10, /* Implement XInclude substitition  */
	XML_PARSE_NONET     = 1<<11, /* Forbid network access */
	XML_PARSE_NODICT    = 1<<12, /* Do not reuse the context dictionnary */
	XML_PARSE_NSCLEAN   = 1<<13, /* remove redundant namespaces declarations */
	XML_PARSE_NOCDATA   = 1<<14, /* merge CDATA as text nodes */
	XML_PARSE_NOXINCNODE = 1<<15, /* do not generate XINCLUDE START/END nodes */
	XML_PARSE_COMPACT   = 1<<16, /* compact small text nodes; no modification of the tree allowed afterwards (will possibly crash if you try to modify the tree) */
	XML_PARSE_OLD10     = 1<<17, /* parse using XML-1.0 before update 5 */
	XML_PARSE_NOBASEFIX = 1<<18, /* do not fixup XINCLUDE xml:base uris */
	XML_PARSE_HUGE      = 1<<19, /* relax any hardcoded limit from the parser */
	XML_PARSE_OLDSAX    = 1<<20, /* parse using SAX2 interface before 2.7.0 */
	XML_PARSE_IGNORE_ENC = 1<<21, /* ignore internal document encoding hint */
	XML_PARSE_BIG_LINES = 1<<22 /* Store big lines numbers in text PSVI field */
} xmlParserOption;

void xmlCtxtReset(xmlParserCtxt * ctxt);
int xmlCtxtResetPush(xmlParserCtxt * ctxt, const char * chunk, int size, const char * filename, const char * encoding);
int xmlCtxtUseOptions(xmlParserCtxt * ctxt, int options);
xmlDoc * xmlReadDoc(const xmlChar * cur, const char * URL, const char * encoding, int options);
xmlDoc * xmlReadFile(const char * URL, const char * encoding, int options);
xmlDoc * xmlReadMemory(const char * buffer, int size, const char * URL, const char * encoding, int options);
xmlDoc * xmlReadFd(int fd, const char * URL, const char * encoding, int options);
xmlDoc * xmlReadIO(xmlInputReadCallback ioread, xmlInputCloseCallback ioclose, void * ioctx, const char * URL, const char * encoding, int options);
xmlDoc * xmlCtxtReadDoc(xmlParserCtxt * ctxt, const xmlChar * cur, const char * URL, const char * encoding, int options);
xmlDoc * xmlCtxtReadFile(xmlParserCtxt * ctxt, const char * filename, const char * encoding, int options);
xmlDoc * xmlCtxtReadMemory(xmlParserCtxt * ctxt, const char * buffer, int size, const char * URL, const char * encoding, int options);
xmlDoc * xmlCtxtReadFd(xmlParserCtxt * ctxt, int fd, const char * URL, const char * encoding, int options);
xmlDoc * xmlCtxtReadIO(xmlParserCtxt * ctxt, xmlInputReadCallback ioread, xmlInputCloseCallback ioclose, void * ioctx, const char * URL, const char * encoding, int options);
/*
 * Library wide options
 */
/**
 * xmlFeature:
 *
 * Used to examine the existance of features that can be enabled
 * or disabled at compile-time.
 * They used to be called XML_FEATURE_xxx but this clashed with Expat
 */
typedef enum {
	XML_WITH_THREAD = 1,
	XML_WITH_TREE = 2,
	XML_WITH_OUTPUT = 3,
	XML_WITH_PUSH = 4,
	XML_WITH_READER = 5,
	XML_WITH_PATTERN = 6,
	XML_WITH_WRITER = 7,
	XML_WITH_SAX1 = 8,
	XML_WITH_FTP = 9,
	XML_WITH_HTTP = 10,
	XML_WITH_VALID = 11,
	XML_WITH_HTML = 12,
	XML_WITH_LEGACY = 13,
	XML_WITH_C14N = 14,
	XML_WITH_CATALOG = 15,
	XML_WITH_XPATH = 16,
	XML_WITH_XPTR = 17,
	XML_WITH_XINCLUDE = 18,
	XML_WITH_ICONV = 19,
	XML_WITH_ISO8859X = 20,
	XML_WITH_UNICODE = 21,
	XML_WITH_REGEXP = 22,
	XML_WITH_AUTOMATA = 23,
	XML_WITH_EXPR = 24,
	XML_WITH_SCHEMAS = 25,
	XML_WITH_SCHEMATRON = 26,
	XML_WITH_MODULES = 27,
	XML_WITH_DEBUG = 28,
	XML_WITH_DEBUG_MEM = 29,
	XML_WITH_DEBUG_RUN = 30,
	XML_WITH_ZLIB = 31,
	XML_WITH_ICU = 32,
	XML_WITH_LZMA = 33,
	XML_WITH_NONE = 99999 /* just to be sure of allocation size */
} xmlFeature;

int xmlHasFeature(xmlFeature feature);

//#ifdef __cplusplus
//}
//#endif
#endif /* __XML_PARSER_H__ */
