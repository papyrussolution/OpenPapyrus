/*
 * xmlreader.c: implements the xmlTextReader streaming node API
 *
 * NOTE:
 * XmlTextReader.Normalization Property won't be supported, since
 *   it makes the parser non compliant to the XML recommendation
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

/*
 * TODOs:
 * - XML Schemas validation
 */
#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop

#ifdef LIBXML_READER_ENABLED
#include <libxml/xmlreader.h>
#ifdef LIBXML_SCHEMAS_ENABLED
	#include <libxml/relaxng.h>
	#include <libxml/xmlschemas.h>
#endif
#ifdef LIBXML_XINCLUDE_ENABLED
	#include <libxml/xinclude.h>
#endif
#define MAX_ERR_MSG_SIZE 64000
/*
 * The following VA_COPY was coded following an example in
 * the Samba project.  It may not be sufficient for some
 * esoteric implementations of va_list but (hopefully) will
 * be sufficient for libxml2.
 */
#ifndef VA_COPY
  #ifdef HAVE_VA_COPY
    #define VA_COPY(dest, src) va_copy(dest, src)
  #else
    #ifdef HAVE___VA_COPY
      #define VA_COPY(dest, src) __va_copy(dest, src)
    #else
      #ifndef VA_LIST_IS_ARRAY
	#define VA_COPY(dest, src) (dest) = (src)
      #else
	#include <string.h>
	#define VA_COPY(dest, src) memcpy((char *)(dest), (char *)(src), sizeof(va_list))
      #endif
    #endif
  #endif
#endif

/* #define DEBUG_CALLBACKS */
/* #define DEBUG_READER */

/**
 * @todo 
 *
 * macro to flag unimplemented blocks
 */
#define TODO xmlGenericError(0, "Unimplemented block at %s:%d\n", __FILE__, __LINE__);
#ifdef DEBUG_READER
	#define DUMP_READER xmlTextReaderDebug(reader);
#else
	#define DUMP_READER
#endif
#define CHUNK_SIZE 512
/************************************************************************
*									*
*	The parser: maps the Text Reader API on top of the existing	*
*		parsing routines building a tree			*
*									*
************************************************************************/

#define XML_TEXTREADER_INPUT    1
#define XML_TEXTREADER_CTXT     2

typedef enum {
	XML_TEXTREADER_NONE = -1,
	XML_TEXTREADER_START = 0,
	XML_TEXTREADER_ELEMENT = 1,
	XML_TEXTREADER_END = 2,
	XML_TEXTREADER_EMPTY = 3,
	XML_TEXTREADER_BACKTRACK = 4,
	XML_TEXTREADER_DONE = 5,
	XML_TEXTREADER_ERROR = 6
} xmlTextReaderState;

typedef enum {
	XML_TEXTREADER_NOT_VALIDATE = 0,
	XML_TEXTREADER_VALIDATE_DTD = 1,
	XML_TEXTREADER_VALIDATE_RNG = 2,
	XML_TEXTREADER_VALIDATE_XSD = 4
} xmlTextReaderValidate;

struct xmlTextReader {
	xmlNode * P_Node;   // @firstmember current node 
	int    mode;        // the parsing mode 
	int    allocs;      // what structure were deallocated 
	int    depth;       // depth of the current node 
	int    preserve;    // preserve the resulting document 
	int    entNr;       // Depth of the entities stack 
	int    entMax;      // Max depth of the entities stack 
	int    preserves;   // level of preserves 
	int    parserFlags; // the set of options set 
	uint   base;        // base of the segment in the input 
	uint   cur;         // current position in the input 
	xmlDoc * doc;       // when walking an existing doc 
	xmlTextReaderValidate validate;  /* is there any validation */
	xmlTextReaderState state;
	xmlParserCtxt * ctxt;          /* the parser context */
	xmlSAXHandler * sax;           /* the parser SAX callbacks */
	xmlParserInputBuffer * input;  /* the input */
	startElementSAXFunc startElement;    /* initial SAX callbacks */
	endElementSAXFunc endElement;        /* idem */
	startElementNsSAX2Func startElementNs; /* idem */
	endElementNsSAX2Func endElementNs;     /* idem */
	charactersSAXFunc characters;
	cdataBlockSAXFunc cdataBlock;
	xmlNode * curnode; // current attribute node 
	xmlNode * faketext; // fake xmlNs chld 
	xmlBuf  * buffer; // used to return const xmlChar * 
	xmlDict * dict;   // the context dictionnary 
	//
	// entity stack when traversing entities content 
	//
	xmlNode * ent;     // Current Entity Ref Node 
	xmlNode ** entTab; // array of entities 
	// error handling 
	xmlTextReaderErrorFunc errorFunc; // callback function 
	void * errorFuncArg; /* callback function user argument */
	// Structured error handling 
	xmlStructuredErrorFunc sErrorFunc; // callback function 
#ifdef LIBXML_SCHEMAS_ENABLED
	/* Handling of RelaxNG validation */
	xmlRelaxNGPtr rngSchemas;       /* The Relax NG schemas */
	xmlRelaxNGValidCtxtPtr rngValidCtxt; /* The Relax NG validation context */
	int    rngPreserveCtxt;                /* 1 if the context was provided by the user */
	int    rngValidErrors;               /* The number of errors detected */
	xmlNode * rngFullNode;         /* the node if RNG not progressive */
	/* Handling of Schemas validation */
	xmlSchema * xsdSchemas; // The Schemas schemas 
	xmlSchemaValidCtxt * xsdValidCtxt; // The Schemas validation context 
	int    xsdPreserveCtxt;               /* 1 if the context was provided by the user */
	int    xsdValidErrors;              /* The number of errors detected */
	xmlSchemaSAXPlugPtr xsdPlug;    /* the schemas plug in SAX pipeline */
#endif
#ifdef LIBXML_XINCLUDE_ENABLED
	/* Handling of XInclude processing */
	int    xinclude;                   /* is xinclude asked for */
	const  xmlChar * xinclude_name; /* the xinclude name from dict */
	xmlXIncludeCtxt * xincctxt;    /* the xinclude context */
	int    in_xinclude;                /* counts for xinclude */
#endif
#ifdef LIBXML_PATTERN_ENABLED
	int    patternNr;                  /* number of preserve patterns */
	int    patternMax;                 /* max preserve patterns */
	xmlPattern ** patternTab; /* array of preserve patterns */
#endif
};

#define NODE_IS_EMPTY           0x1
#define NODE_IS_PRESERVED       0x2
#define NODE_IS_SPRESERVED      0x4

/**
 * CONSTSTR:
 *
 * Macro used to return an interned string
 */
#define CONSTSTR(str) xmlDictLookupSL(reader->dict, (str))
#define CONSTQSTR(p, str) xmlDictQLookup(reader->dict, (p), (str))

static int xmlTextReaderReadTree(xmlTextReader * reader);
static int xmlTextReaderNextTree(xmlTextReader * reader);
//
// Our own version of the freeing routines as we recycle nodes
//
/**
 * @str:  a string
 *
 * Free a string if it is not owned by the "dict" dictionnary in the current scope
 */
//#define DICT_FREE(str) if((str) && ((!dict) || (xmlDictOwns(dict, (const xmlChar *)(str)) == 0))) SAlloc::F((char *)(str));

static void xmlTextReaderFreeNode(xmlTextReader * reader, xmlNode * cur);
static void FASTCALL xmlTextReaderFreeNodeList(xmlTextReader * reader, xmlNode * cur);

/**
 * xmlFreeID:
 * @not:  A id
 *
 * Deallocate the memory used by an id definition
 */
static void xmlFreeID(xmlID * id) 
{
	if(id) {
		xmlDict * dict = id->doc ? id->doc->dict : 0;
		XmlDestroyStringWithDict(dict, (xmlChar *)id->value); // @badcast
		SAlloc::F(id);
	}
}
/**
 * xmlTextReaderRemoveID:
 * @doc:  the document
 * @attr:  the attribute
 *
 * Remove the given attribute from the ID table maintained internally.
 *
 * Returns -1 if the lookup failed and 0 otherwise
 */
static int xmlTextReaderRemoveID(xmlDoc * doc, xmlAttr * attr)
{
	xmlIDTablePtr table;
	xmlID * id;
	xmlChar * ID;
	if(!doc || !attr)
		return -1;
	table = static_cast<xmlIDTable *>(doc->ids);
	if(!table)
		return -1;
	ID = xmlNodeListGetString(doc, attr->children, 1);
	if(!ID)
		return -1;
	id = static_cast<xmlID *>(xmlHashLookup(table, ID));
	SAlloc::F(ID);
	if(id == NULL || id->attr != attr) {
		return -1;
	}
	id->name = attr->name;
	id->attr = NULL;
	return 0;
}
/**
 * xmlTextReaderFreeProp:
 * @reader:  the (xmlTextReader *) used
 * @cur:  the node
 *
 * Free a node.
 */
static void xmlTextReaderFreeProp(xmlTextReader * reader, xmlAttr * cur)
{
	if(cur) {
		xmlDict * dict = (reader && reader->ctxt) ? reader->ctxt->dict : NULL;
		if((__xmlRegisterCallbacks) && (xmlDeregisterNodeDefaultValue))
			xmlDeregisterNodeDefaultValue((xmlNode *)cur);
		// Check for ID removal -> leading to invalid references ! 
		if(cur->parent && cur->parent->doc && (cur->parent->doc->intSubset || cur->parent->doc->extSubset)) {
			if(xmlIsID(cur->parent->doc, cur->parent, cur))
				xmlTextReaderRemoveID(cur->parent->doc, cur);
		}
		xmlTextReaderFreeNodeList(reader, cur->children);
		XmlDestroyStringWithDict(dict, (xmlChar *)cur->name); // @badcast
		if(reader && reader->ctxt && (reader->ctxt->freeAttrsNr < 100)) {
			cur->next = reader->ctxt->freeAttrs;
			reader->ctxt->freeAttrs = cur;
			reader->ctxt->freeAttrsNr++;
		}
		else
			SAlloc::F(cur);
	}
}
/**
 * xmlTextReaderFreePropList:
 * @reader:  the (xmlTextReader *) used
 * @cur:  the first property in the list
 *
 * Free a property and all its siblings, all the children are freed too.
 */
static void xmlTextReaderFreePropList(xmlTextReader * reader, xmlAttr * cur) 
{
	while(cur) {
		xmlAttr * next = cur->next;
		xmlTextReaderFreeProp(reader, cur);
		cur = next;
	}
}
/**
 * @reader:  the (xmlTextReader *) used
 * @cur:  the first node in the list
 *
 * Free a node and all its siblings, this is a recursive behaviour, all
 * the children are freed too.
 */
static void FASTCALL xmlTextReaderFreeNodeList(xmlTextReader * reader, xmlNode * cur) 
{
	if(cur) {
		if(cur->type == XML_NAMESPACE_DECL)
			xmlFreeNsList((xmlNs *)cur);
		else if(oneof2(cur->type, XML_DOCUMENT_NODE, XML_HTML_DOCUMENT_NODE))
			xmlFreeDoc((xmlDoc *)cur);
		else {
			xmlDict * dict = (reader && reader->ctxt) ? reader->ctxt->dict : NULL;
			while(cur) {
				xmlNode * next = cur->next;
				/* unroll to speed up freeing the document */
				if(cur->type != XML_DTD_NODE) {
					if(cur->children && (cur->type != XML_ENTITY_REF_NODE)) {
						if(cur->children->P_ParentNode == cur)
							xmlTextReaderFreeNodeList(reader, cur->children); // @recursion
						cur->children = NULL;
					}
					if((__xmlRegisterCallbacks) && (xmlDeregisterNodeDefaultValue))
						xmlDeregisterNodeDefaultValue(cur);
					if(oneof3(cur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END) && cur->properties)
						xmlTextReaderFreePropList(reader, cur->properties);
					if((cur->content != (xmlChar *)&(cur->properties)) && !oneof4(cur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END, XML_ENTITY_REF_NODE)) {
						XmlDestroyStringWithDict(dict, cur->content);
					}
					if(oneof3(cur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END) && cur->nsDef)
						xmlFreeNsList(cur->nsDef);
					/*
					 * we don't free element names here they are interned now
					 */
					if(!oneof2(cur->type, XML_TEXT_NODE, XML_COMMENT_NODE))
						XmlDestroyStringWithDict(dict, (xmlChar *)cur->name); // @badcast
					if(oneof2(cur->type, XML_ELEMENT_NODE, XML_TEXT_NODE) && reader && reader->ctxt && (reader->ctxt->freeElemsNr < 100)) {
						cur->next = reader->ctxt->freeElems;
						reader->ctxt->freeElems = cur;
						reader->ctxt->freeElemsNr++;
					}
					else {
						SAlloc::F(cur);
					}
				}
				cur = next;
			}
		}
	}
}
/**
 * xmlTextReaderFreeNode:
 * @reader:  the (xmlTextReader *) used
 * @cur:  the node
 *
 * Free a node, this is a recursive behaviour, all the children are freed too.
 * This doesn't unlink the child from the list, use xmlUnlinkNode() first.
 */
static void xmlTextReaderFreeNode(xmlTextReader * reader, xmlNode * cur)
{
	xmlDict * dict = (reader && reader->ctxt) ? reader->ctxt->dict : NULL;
	if(cur->type == XML_DTD_NODE) {
		xmlFreeDtd((xmlDtd *)cur);
		return;
	}
	if(cur->type == XML_NAMESPACE_DECL) {
		xmlFreeNs((xmlNs *)cur);
		return;
	}
	if(cur->type == XML_ATTRIBUTE_NODE) {
		xmlTextReaderFreeProp(reader, (xmlAttr *)cur);
		return;
	}
	if(cur->children && (cur->type != XML_ENTITY_REF_NODE)) {
		if(cur->children->P_ParentNode == cur)
			xmlTextReaderFreeNodeList(reader, cur->children);
		cur->children = NULL;
	}
	if((__xmlRegisterCallbacks) && (xmlDeregisterNodeDefaultValue))
		xmlDeregisterNodeDefaultValue(cur);
	if(oneof3(cur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END) && cur->properties)
		xmlTextReaderFreePropList(reader, cur->properties);
	if((cur->content != (xmlChar *)&(cur->properties)) && !oneof4(cur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END, XML_ENTITY_REF_NODE)) {
		XmlDestroyStringWithDict(dict, cur->content);
	}
	if(oneof3(cur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END) && cur->nsDef)
		xmlFreeNsList(cur->nsDef);
	//
	// we don't free names here they are interned now
	//
	if(!oneof2(cur->type, XML_TEXT_NODE, XML_COMMENT_NODE))
		XmlDestroyStringWithDict(dict, (xmlChar *)cur->name); // @badcast
	if(oneof2(cur->type, XML_ELEMENT_NODE, XML_TEXT_NODE) && reader && reader->ctxt && (reader->ctxt->freeElemsNr < 100)) {
		cur->next = reader->ctxt->freeElems;
		reader->ctxt->freeElems = cur;
		reader->ctxt->freeElemsNr++;
	}
	else
		SAlloc::F(cur);
}
/**
 * xmlTextReaderFreeIDTable:
 * @table:  An id table
 *
 * Deallocate the memory used by an ID hash table.
 */
static void xmlTextReaderFreeIDTable(xmlIDTablePtr table)
{
	xmlHashFree(table, (xmlHashDeallocator)xmlFreeID);
}
/**
 * xmlTextReaderFreeDoc:
 * @reader:  the (xmlTextReader *) used
 * @cur:  pointer to the document
 *
 * Free up all the structures used by a document, tree included.
 */
static void xmlTextReaderFreeDoc(xmlTextReader * reader, xmlDoc * cur)
{
	xmlDtd * extSubset;
	xmlDtd * intSubset;
	if(cur) {
		if((__xmlRegisterCallbacks) && (xmlDeregisterNodeDefaultValue))
			xmlDeregisterNodeDefaultValue((xmlNode *)cur);
		/*
		 * Do this before freeing the children list to avoid ID lookups
		 */
		xmlTextReaderFreeIDTable((xmlIDTable *)cur->ids);
		cur->ids = NULL;
		xmlFreeRefTable((xmlRefTable *)cur->refs);
		cur->refs = NULL;
		extSubset = cur->extSubset;
		intSubset = cur->intSubset;
		if(intSubset == extSubset)
			extSubset = NULL;
		if(extSubset) {
			xmlUnlinkNode((xmlNode *)cur->extSubset);
			cur->extSubset = NULL;
			xmlFreeDtd(extSubset);
		}
		if(intSubset) {
			xmlUnlinkNode((xmlNode *)cur->intSubset);
			cur->intSubset = NULL;
			xmlFreeDtd(intSubset);
		}
		xmlTextReaderFreeNodeList(reader, cur->children);
		SAlloc::F((char *)cur->version);
		SAlloc::F((char *)cur->name);
		SAlloc::F((char *)cur->encoding);
		if(cur->oldNs)
			xmlFreeNsList(cur->oldNs);
		SAlloc::F((char *)cur->URL);
		xmlDictFree(cur->dict);
		SAlloc::F(cur);
	}
}

/************************************************************************
*									*
*			The reader core parser				*
*									*
************************************************************************/
#ifdef DEBUG_READER
static void xmlTextReaderDebug(xmlTextReader * reader)
{
	if(!reader || !reader->ctxt) {
		fprintf(stderr, "xmlTextReader NULL\n");
		return;
	}
	fprintf(stderr, "xmlTextReader: state %d depth %d ", reader->state, reader->depth);
	if(reader->node == NULL) {
		fprintf(stderr, "node = NULL\n");
	}
	else {
		fprintf(stderr, "node %s\n", reader->node->name);
	}
	fprintf(stderr, "  input: base %d, cur %d, depth %d: ", reader->base, reader->cur, reader->ctxt->nodeNr);
	if(reader->input->buffer == NULL) {
		fprintf(stderr, "buffer is NULL\n");
	}
	else {
#ifdef LIBXML_DEBUG_ENABLED
		xmlDebugDumpString(stderr, &reader->input->buffer->content[reader->cur]);
#endif
		fprintf(stderr, "\n");
	}
}

#endif

/**
 * xmlTextReaderEntPush:
 * @reader:  the (xmlTextReader *) used
 * @value:  the entity reference node
 *
 * Pushes a new entity reference node on top of the entities stack
 *
 * Returns 0 in case of error, the index in the stack otherwise
 */
static int FASTCALL xmlTextReaderEntPush(xmlTextReader * reader, xmlNode * value)
{
	if(reader->entMax <= 0) {
		reader->entMax = 10;
		reader->entTab = (xmlNode **)SAlloc::M(reader->entMax * sizeof(reader->entTab[0]));
		if(reader->entTab == NULL) {
			xmlGenericError(0, "xmlMalloc failed !\n");
			return 0;
		}
	}
	if(reader->entNr >= reader->entMax) {
		reader->entMax *= 2;
		reader->entTab = (xmlNode **)SAlloc::R(reader->entTab, reader->entMax * sizeof(reader->entTab[0]));
		if(reader->entTab == NULL) {
			xmlGenericError(0, "xmlRealloc failed !\n");
			return 0;
		}
	}
	reader->entTab[reader->entNr] = value;
	reader->ent = value;
	return (reader->entNr++);
}
/**
 * xmlTextReaderEntPop:
 * @reader:  the (xmlTextReader *) used
 *
 * Pops the top element entity from the entities stack
 *
 * Returns the entity just removed
 */
static xmlNode * FASTCALL xmlTextReaderEntPop(xmlTextReader * reader)
{
	xmlNode * ret = 0;
	if(reader->entNr > 0) {
		reader->entNr--;
		reader->ent = (reader->entNr > 0) ? reader->entTab[reader->entNr - 1] : NULL;
		ret = reader->entTab[reader->entNr];
		reader->entTab[reader->entNr] = NULL;
	}
	return ret;
}
/**
 * xmlTextReaderStartElement:
 * @ctx: the user data (XML parser context)
 * @fullname:  The element name, including namespace prefix
 * @atts:  An array of name/value attributes pairs, NULL terminated
 *
 * called when an opening tag has been processed.
 */
static void xmlTextReaderStartElement(void * ctx, const xmlChar * fullname, const xmlChar ** atts)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlTextReader * reader = (xmlTextReader *)ctxt->_private;
#ifdef DEBUG_CALLBACKS
	printf("xmlTextReaderStartElement(%s)\n", fullname);
#endif
	if(reader && reader->startElement) {
		reader->startElement(ctx, fullname, atts);
		if(ctxt->P_Node && ctxt->input && ctxt->input->cur && (ctxt->input->cur[0] == '/') && (ctxt->input->cur[1] == '>'))
			ctxt->P_Node->extra = NODE_IS_EMPTY;
	}
	if(reader)
		reader->state = XML_TEXTREADER_ELEMENT;
}

/**
 * xmlTextReaderEndElement:
 * @ctx: the user data (XML parser context)
 * @fullname:  The element name, including namespace prefix
 *
 * called when an ending tag has been processed.
 */
static void xmlTextReaderEndElement(void * ctx, const xmlChar * fullname)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlTextReader * reader = (xmlTextReader *)ctxt->_private;
#ifdef DEBUG_CALLBACKS
	printf("xmlTextReaderEndElement(%s)\n", fullname);
#endif
	if(reader && reader->endElement) {
		reader->endElement(ctx, fullname);
	}
}

/**
 * xmlTextReaderStartElementNs:
 * @ctx: the user data (XML parser context)
 * @localname:  the local name of the element
 * @prefix:  the element namespace prefix if available
 * @URI:  the element namespace name if available
 * @nb_namespaces:  number of namespace definitions on that node
 * @namespaces:  pointer to the array of prefix/URI pairs namespace definitions
 * @nb_attributes:  the number of attributes on that node
 * nb_defaulted:  the number of defaulted attributes.
 * @attributes:  pointer to the array of (localname/prefix/URI/value/end)
 *          attribute values.
 *
 * called when an opening tag has been processed.
 */
static void xmlTextReaderStartElementNs(void * ctx, const xmlChar * localname,
    const xmlChar * prefix, const xmlChar * URI, int nb_namespaces,
    const xmlChar ** namespaces, int nb_attributes, int nb_defaulted, const xmlChar ** attributes)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlTextReader * reader = (xmlTextReader *)ctxt->_private;
#ifdef DEBUG_CALLBACKS
	printf("xmlTextReaderStartElementNs(%s)\n", localname);
#endif
	if(reader && reader->startElementNs) {
		reader->startElementNs(ctx, localname, prefix, URI, nb_namespaces, namespaces, nb_attributes, nb_defaulted, attributes);
		if(ctxt->P_Node && ctxt->input && ctxt->input->cur && (ctxt->input->cur[0] == '/') && (ctxt->input->cur[1] == '>'))
			ctxt->P_Node->extra = NODE_IS_EMPTY;
	}
	if(reader)
		reader->state = XML_TEXTREADER_ELEMENT;
}

/**
 * xmlTextReaderEndElementNs:
 * @ctx: the user data (XML parser context)
 * @localname:  the local name of the element
 * @prefix:  the element namespace prefix if available
 * @URI:  the element namespace name if available
 *
 * called when an ending tag has been processed.
 */
static void xmlTextReaderEndElementNs(void * ctx, const xmlChar * localname, const xmlChar * prefix, const xmlChar * URI)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlTextReader * reader = (xmlTextReader *)ctxt->_private;
#ifdef DEBUG_CALLBACKS
	printf("xmlTextReaderEndElementNs(%s)\n", localname);
#endif
	if(reader && reader->endElementNs) {
		reader->endElementNs(ctx, localname, prefix, URI);
	}
}

/**
 * xmlTextReaderCharacters:
 * @ctx: the user data (XML parser context)
 * @ch:  a xmlChar string
 * @len: the number of xmlChar
 *
 * receiving some chars from the parser.
 */
static void xmlTextReaderCharacters(void * ctx, const xmlChar * ch, int len)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlTextReader * reader = (xmlTextReader *)ctxt->_private;
#ifdef DEBUG_CALLBACKS
	printf("xmlTextReaderCharacters()\n");
#endif
	if(reader && reader->characters) {
		reader->characters(ctx, ch, len);
	}
}

/**
 * xmlTextReaderCDataBlock:
 * @ctx: the user data (XML parser context)
 * @value:  The pcdata content
 * @len:  the block length
 *
 * called when a pcdata block has been parsed
 */
static void xmlTextReaderCDataBlock(void * ctx, const xmlChar * ch, int len)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlTextReader * reader = (xmlTextReader *)ctxt->_private;
#ifdef DEBUG_CALLBACKS
	printf("xmlTextReaderCDataBlock()\n");
#endif
	if(reader && reader->cdataBlock) {
		reader->cdataBlock(ctx, ch, len);
	}
}
/**
 * xmlTextReaderPushData:
 * @reader:  the (xmlTextReader *) used
 *
 * Push data down the progressive parser until a significant callback
 * got raised.
 *
 * Returns -1 in case of failure, 0 otherwise
 */
static int FASTCALL xmlTextReaderPushData(xmlTextReader * reader)
{
	xmlBufPtr inbuf;
	int val, s;
	xmlTextReaderState oldstate;
	int alloc;
	if(!reader->input || !reader->input->buffer)
		return -1;
	oldstate = reader->state;
	reader->state = XML_TEXTREADER_NONE;
	inbuf = reader->input->buffer;
	alloc = xmlBufGetAllocationScheme(inbuf);
	while(reader->state == XML_TEXTREADER_NONE) {
		if(xmlBufUse(inbuf) < reader->cur + CHUNK_SIZE) {
			/*
			 * Refill the buffer unless we are at the end of the stream
			 */
			if(reader->mode != XML_TEXTREADER_MODE_EOF) {
				val = xmlParserInputBufferRead(reader->input, 4096);
				if(!val && alloc == XML_BUFFER_ALLOC_IMMUTABLE) {
					if(xmlBufUse(inbuf) == reader->cur) {
						reader->mode = XML_TEXTREADER_MODE_EOF;
						reader->state = oldstate;
					}
				}
				else if(val < 0) {
					reader->mode = XML_TEXTREADER_MODE_EOF;
					reader->state = oldstate;
					if((oldstate != XML_TEXTREADER_START) || reader->ctxt->myDoc)
						return val;
				}
				else if(val == 0) {
					/* mark the end of the stream and process the remains */
					reader->mode = XML_TEXTREADER_MODE_EOF;
					break;
				}
			}
			else
				break;
		}
		//
		// parse by block of CHUNK_SIZE bytes, various tests show that
		// it's the best tradeoff at least on a 1.2GH Duron
		//
		if(xmlBufUse(inbuf) >= reader->cur + CHUNK_SIZE) {
			val = xmlParseChunk(reader->ctxt, (const char *)xmlBufContent(inbuf) + reader->cur, CHUNK_SIZE, 0);
			reader->cur += CHUNK_SIZE;
			if(val)
				reader->ctxt->wellFormed = 0;
			if(reader->ctxt->wellFormed == 0)
				break;
		}
		else {
			s = xmlBufUse(inbuf) - reader->cur;
			val = xmlParseChunk(reader->ctxt, (const char *)xmlBufContent(inbuf) + reader->cur, s, 0);
			reader->cur += s;
			if(val)
				reader->ctxt->wellFormed = 0;
			break;
		}
	}
	//
	// Discard the consumed input when needed and possible
	//
	if(reader->mode == XML_TEXTREADER_MODE_INTERACTIVE) {
		if(alloc != XML_BUFFER_ALLOC_IMMUTABLE) {
			if((reader->cur >= 4096) && (xmlBufUse(inbuf) - reader->cur <= CHUNK_SIZE)) {
				val = xmlBufShrink(inbuf, reader->cur);
				if(val >= 0)
					reader->cur -= val;
			}
		}
	}
	//
	// At the end of the stream signal that the work is done to the Push parser.
	//
	else if(reader->mode == XML_TEXTREADER_MODE_EOF) {
		if(reader->state != XML_TEXTREADER_DONE) {
			s = xmlBufUse(inbuf) - reader->cur;
			val = xmlParseChunk(reader->ctxt, (const char *)xmlBufContent(inbuf) + reader->cur, s, 1);
			reader->cur = xmlBufUse(inbuf);
			reader->state  = XML_TEXTREADER_DONE;
			if(val) {
				if(reader->ctxt->wellFormed)
					reader->ctxt->wellFormed = 0;
				else
					return -1;
			}
		}
	}
	reader->state = oldstate;
	if(reader->ctxt->wellFormed == 0) {
		reader->mode = XML_TEXTREADER_MODE_EOF;
		return -1;
	}
	return 0;
}

#ifdef LIBXML_REGEXP_ENABLED
/**
 * xmlTextReaderValidatePush:
 * @reader:  the (xmlTextReader *) used
 *
 * Push the current node for validation
 */
static void FASTCALL xmlTextReaderValidatePush(xmlTextReader * reader ATTRIBUTE_UNUSED)
{
	xmlNode * P_Node = reader->P_Node;
#ifdef LIBXML_VALID_ENABLED
	if((reader->validate == XML_TEXTREADER_VALIDATE_DTD) && reader->ctxt && (reader->ctxt->validate == 1)) {
		if((P_Node->ns == NULL) || (P_Node->ns->prefix == NULL)) {
			reader->ctxt->valid &= xmlValidatePushElement(&reader->ctxt->vctxt, reader->ctxt->myDoc, P_Node, P_Node->name);
		}
		else {
			/* @todo use the BuildQName interface */
			xmlChar * qname = sstrdup(P_Node->ns->prefix);
			qname = xmlStrcat(qname, reinterpret_cast<const xmlChar *>(":"));
			qname = xmlStrcat(qname, P_Node->name);
			reader->ctxt->valid &= xmlValidatePushElement(&reader->ctxt->vctxt, reader->ctxt->myDoc, P_Node, qname);
			SAlloc::F(qname);
		}
	}
#endif /* LIBXML_VALID_ENABLED */
#ifdef LIBXML_SCHEMAS_ENABLED
	if((reader->validate == XML_TEXTREADER_VALIDATE_RNG) && reader->rngValidCtxt) {
		int ret;
		if(reader->rngFullNode)
			return;
		ret = xmlRelaxNGValidatePushElement(reader->rngValidCtxt, reader->ctxt->myDoc, P_Node);
		if(!ret) {
			/*
			 * this element requires a full tree
			 */
			P_Node = xmlTextReaderExpand(reader);
			if(!P_Node) {
				printf("Expand failed !\n");
				ret = -1;
			}
			else {
				ret = xmlRelaxNGValidateFullElement(reader->rngValidCtxt, reader->ctxt->myDoc, P_Node);
				reader->rngFullNode = P_Node;
			}
		}
		if(ret != 1)
			reader->rngValidErrors++;
	}
#endif
}

/**
 * xmlTextReaderValidateCData:
 * @reader:  the (xmlTextReader *) used
 * @data:  pointer to the CData
 * @len:  length of the CData block in bytes.
 *
 * Push some CData for validation
 */
static void xmlTextReaderValidateCData(xmlTextReader * reader, const xmlChar * data)
{
	const int len = sstrlen(data);
#ifdef LIBXML_VALID_ENABLED
	if((reader->validate == XML_TEXTREADER_VALIDATE_DTD) && reader->ctxt && (reader->ctxt->validate == 1)) {
		reader->ctxt->valid &= xmlValidatePushCData(&reader->ctxt->vctxt, data, len);
	}
#endif /* LIBXML_VALID_ENABLED */
#ifdef LIBXML_SCHEMAS_ENABLED
	if((reader->validate == XML_TEXTREADER_VALIDATE_RNG) && reader->rngValidCtxt) {
		if(!reader->rngFullNode) {
			int ret = xmlRelaxNGValidatePushCData(reader->rngValidCtxt, data, len);
			if(ret != 1)
				reader->rngValidErrors++;
		}
	}
#endif
}
/**
 * xmlTextReaderValidatePop:
 * @reader:  the (xmlTextReader *) used
 *
 * Pop the current node from validation
 */
static void FASTCALL xmlTextReaderValidatePop(xmlTextReader * reader)
{
	xmlNode * p_node = reader->P_Node;
#ifdef LIBXML_VALID_ENABLED
	if(reader->validate == XML_TEXTREADER_VALIDATE_DTD && reader->ctxt && (reader->ctxt->validate == 1)) {
		if(!p_node->ns || !p_node->ns->prefix) {
			reader->ctxt->valid &= xmlValidatePopElement(&reader->ctxt->vctxt, reader->ctxt->myDoc, p_node, p_node->name);
		}
		else {
			// @todo use the BuildQName interface 
			SString qname_buf;
			(qname_buf = (const char *)p_node->ns->prefix).CatChar(':').Cat((const char *)p_node->name);
			//xmlChar * qname = sstrdup(p_node->ns->prefix);
			//qname = xmlStrcat(qname, reinterpret_cast<const xmlChar *>(":"));
			//qname = xmlStrcat(qname, p_node->name);
			reader->ctxt->valid &= xmlValidatePopElement(&reader->ctxt->vctxt, reader->ctxt->myDoc, p_node, /*qname*/qname_buf.ucptr());
			//SAlloc::F(qname);
		}
	}
#endif /* LIBXML_VALID_ENABLED */
#ifdef LIBXML_SCHEMAS_ENABLED
	if(reader->validate == XML_TEXTREADER_VALIDATE_RNG && reader->rngValidCtxt) {
		if(reader->rngFullNode) {
			if(p_node == reader->rngFullNode)
				reader->rngFullNode = NULL;
		}
		else {
			int ret = xmlRelaxNGValidatePopElement(reader->rngValidCtxt, reader->ctxt->myDoc, p_node);
			if(ret != 1)
				reader->rngValidErrors++;
		}
	}
#endif
}

/**
 * xmlTextReaderValidateEntity:
 * @reader:  the (xmlTextReader *) used
 *
 * Handle the validation when an entity reference is encountered and
 * entity substitution is not activated. As a result the parser interface
 * must walk through the entity and do the validation calls
 */
static void FASTCALL xmlTextReaderValidateEntity(xmlTextReader * reader)
{
	xmlNode * oldnode = reader->P_Node;
	xmlNode * p_node = reader->P_Node;
	xmlParserCtxt * ctxt = reader->ctxt;
	do {
		if(p_node->type == XML_ENTITY_REF_NODE) {
			/*
			 * Case where the underlying tree is not availble, lookup the entity
			 * and walk it.
			 */
			if(!p_node->children && ctxt->sax && ctxt->sax->getEntity) {
				p_node->children = (xmlNode *)ctxt->sax->getEntity(ctxt, p_node->name);
			}
			if(p_node->children && (p_node->children->type == XML_ENTITY_DECL) && p_node->children->children) {
				xmlTextReaderEntPush(reader, p_node);
				p_node = p_node->children->children;
				continue;
			}
			else {
				/*
				 * The error has probably be raised already.
				 */
				if(p_node == oldnode)
					break;
				p_node = p_node->next;
			}
#ifdef LIBXML_REGEXP_ENABLED
		}
		else if(p_node->type == XML_ELEMENT_NODE) {
			reader->P_Node = p_node;
			xmlTextReaderValidatePush(reader);
		}
		else if(oneof2(p_node->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE)) {
			xmlTextReaderValidateCData(reader, p_node->content);
#endif
		}
		/*
		 * go to next node
		 */
		if(p_node->children) {
			p_node = p_node->children;
			continue;
		}
		else if(p_node->type == XML_ELEMENT_NODE) {
			xmlTextReaderValidatePop(reader);
		}
		if(p_node->next) {
			p_node = p_node->next;
			continue;
		}
		do {
			p_node = p_node->P_ParentNode;
			if(p_node->type == XML_ELEMENT_NODE) {
				xmlNode * tmp;
				if(reader->entNr == 0) {
					while((tmp = p_node->last) != NULL) {
						if((tmp->extra & NODE_IS_PRESERVED) == 0) {
							xmlUnlinkNode(tmp);
							xmlTextReaderFreeNode(reader, tmp);
						}
						else
							break;
					}
				}
				reader->P_Node = p_node;
				xmlTextReaderValidatePop(reader);
			}
			if((p_node->type == XML_ENTITY_DECL) && reader->ent && (reader->ent->children == p_node)) {
				p_node = xmlTextReaderEntPop(reader);
			}
			if(p_node == oldnode)
				break;
			if(p_node->next) {
				p_node = p_node->next;
				break;
			}
		} while(p_node && (p_node != oldnode));
	} while(p_node && (p_node != oldnode));
	reader->P_Node = oldnode;
}

#endif /* LIBXML_REGEXP_ENABLED */

/**
 * xmlTextReaderGetSuccessor:
 * @cur:  the current node
 *
 * Get the successor of a node if available.
 *
 * Returns the successor node or NULL
 */
static xmlNode * FASTCALL xmlTextReaderGetSuccessor(xmlNode * cur)
{
	if(cur) {
		if(cur->next)
			return cur->next;
		else {
			do {
				cur = cur->P_ParentNode;
				if(!cur)
					break;
				if(cur->next)
					return cur->next;
			} while(cur);
		}
	}
	return cur;
}
/**
 * xmlTextReaderDoExpand:
 * @reader:  the (xmlTextReader *) used
 *
 * Makes sure that the current node is fully read as well as all its
 * descendant. It means the full DOM subtree must be available at the
 * end of the call.
 *
 * Returns 1 if the node was expanded successfully, 0 if there is no more
 *     nodes to read, or -1 in case of error
 */
static int FASTCALL xmlTextReaderDoExpand(xmlTextReader * reader)
{
	if(!reader || !reader->P_Node || !reader->ctxt)
		return -1;
	else {
		do {
			if(reader->ctxt->instate == XML_PARSER_EOF)
				return 1;
			else if(xmlTextReaderGetSuccessor(reader->P_Node))
				return 1;
			else if(reader->ctxt->nodeNr < reader->depth)
				return 1;
			else if(reader->mode == XML_TEXTREADER_MODE_EOF)
				return 1;
			else {
				int val = xmlTextReaderPushData(reader);
				if(val < 0) {
					reader->mode = XML_TEXTREADER_MODE_ERROR;
					return -1;
				}
			}
		} while(reader->mode != XML_TEXTREADER_MODE_EOF);
		return 1;
	}
}

/**
 * xmlTextReaderCollectSiblings:
 * @node:    the first child
 *
 *  Traverse depth-first through all sibling nodes and their children
 *  nodes and concatenate their content. This is an auxiliary function
 *  to xmlTextReaderReadString.
 *
 *  Returns a string containing the content, or NULL in case of error.
 */
static xmlChar * xmlTextReaderCollectSiblings(xmlNode * P_Node)
{
	xmlBuffer * buffer;
	xmlChar * ret;
	if(!P_Node || (P_Node->type == XML_NAMESPACE_DECL))
		return 0;
	buffer = xmlBufferCreate();
	if(!buffer)
		return NULL;
	for(; P_Node; P_Node = P_Node->next) {
		switch(P_Node->type) {
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			    xmlBufferCat(buffer, P_Node->content);
			    break;
			case XML_ELEMENT_NODE: {
			    xmlChar * tmp = xmlTextReaderCollectSiblings(P_Node->children);
			    xmlBufferCat(buffer, tmp);
			    SAlloc::F(tmp);
			    break;
		    }
			default:
			    break;
		}
	}
	ret = buffer->content;
	buffer->content = NULL;
	xmlBufferFree(buffer);
	return ret;
}

/**
 * xmlTextReaderRead:
 * @reader:  the (xmlTextReader *) used
 *
 *  Moves the position of the current instance to the next node in
 *  the stream, exposing its properties.
 *
 *  Returns 1 if the node was read successfully, 0 if there is no more
 *     nodes to read, or -1 in case of error
 */
int FASTCALL xmlTextReaderRead(xmlTextReader * reader)
{
	int val, olddepth = 0;
	xmlTextReaderState oldstate = XML_TEXTREADER_START;
	xmlNode * oldnode = NULL;
	if(!reader)
		return -1;
	reader->curnode = NULL;
	if(reader->doc)
		return xmlTextReaderReadTree(reader);
	if(!reader->ctxt)
		return -1;
#ifdef DEBUG_READER
	fprintf(stderr, "\nREAD ");
	DUMP_READER
#endif
	if(reader->mode == XML_TEXTREADER_MODE_INITIAL) {
		reader->mode = XML_TEXTREADER_MODE_INTERACTIVE;
		/*
		 * Initial state
		 */
		do {
			val = xmlTextReaderPushData(reader);
			if(val < 0) {
				reader->mode = XML_TEXTREADER_MODE_ERROR;
				reader->state = XML_TEXTREADER_ERROR;
				return -1;
			}
		} while(!reader->ctxt->P_Node && ((reader->mode != XML_TEXTREADER_MODE_EOF) && (reader->state != XML_TEXTREADER_DONE)));
		if(!reader->ctxt->P_Node) {
			if(reader->ctxt->myDoc)
				reader->P_Node = reader->ctxt->myDoc->children;
			if(!reader->P_Node) {
				reader->mode = XML_TEXTREADER_MODE_ERROR;
				reader->state = XML_TEXTREADER_ERROR;
				return -1;
			}
			reader->state = XML_TEXTREADER_ELEMENT;
		}
		else {
			if(reader->ctxt->myDoc)
				reader->P_Node = reader->ctxt->myDoc->children;
			SETIFZ(reader->P_Node, reader->ctxt->PP_NodeTab[0]);
			reader->state = XML_TEXTREADER_ELEMENT;
		}
		reader->depth = 0;
		reader->ctxt->parseMode = XML_PARSE_READER;
		goto node_found;
	}
	oldstate = reader->state;
	olddepth = reader->ctxt->nodeNr;
	oldnode = reader->P_Node;
get_next_node:
	if(!reader->P_Node)
		return (reader->mode == XML_TEXTREADER_MODE_EOF) ? 0 : -1;
	/*
	 * If we are not backtracking on ancestors or examined nodes,
	 * that the parser didn't finished or that we arent at the end
	 * of stream, continue processing.
	 */
	while(reader->P_Node && !reader->P_Node->next && (reader->ctxt->nodeNr == olddepth) &&
	    ((oldstate == XML_TEXTREADER_BACKTRACK) || !reader->P_Node->children || (reader->P_Node->type == XML_ENTITY_REF_NODE) ||
		    (reader->P_Node->children && (reader->P_Node->children->type == XML_TEXT_NODE) && !reader->P_Node->children->next) ||
		    oneof3(reader->P_Node->type, XML_DTD_NODE, XML_DOCUMENT_NODE, XML_HTML_DOCUMENT_NODE)) && (!reader->ctxt->P_Node ||
		    (reader->ctxt->P_Node == reader->P_Node) || (reader->ctxt->P_Node == reader->P_Node->P_ParentNode)) && (reader->ctxt->instate != XML_PARSER_EOF)) {
		val = xmlTextReaderPushData(reader);
		if(val < 0) {
			reader->mode = XML_TEXTREADER_MODE_ERROR;
			reader->state = XML_TEXTREADER_ERROR;
			return -1;
		}
		if(!reader->P_Node)
			goto node_end;
	}
	if(oldstate != XML_TEXTREADER_BACKTRACK) {
		if(reader->P_Node->children && !oneof3(reader->P_Node->type, XML_ENTITY_REF_NODE, XML_XINCLUDE_START, XML_DTD_NODE)) {
			reader->P_Node = reader->P_Node->children;
			reader->depth++;
			reader->state = XML_TEXTREADER_ELEMENT;
			goto node_found;
		}
	}
	if(reader->P_Node->next) {
		if((oldstate == XML_TEXTREADER_ELEMENT) && (reader->P_Node->type == XML_ELEMENT_NODE) &&
		    !reader->P_Node->children && ((reader->P_Node->extra & NODE_IS_EMPTY) == 0)
#ifdef LIBXML_XINCLUDE_ENABLED
		    && (reader->in_xinclude <= 0)
#endif
		    ) {
			reader->state = XML_TEXTREADER_END;
			goto node_found;
		}
#ifdef LIBXML_REGEXP_ENABLED
		if(reader->validate && reader->P_Node->type == XML_ELEMENT_NODE)
			xmlTextReaderValidatePop(reader);
#endif /* LIBXML_REGEXP_ENABLED */
		if((reader->preserves > 0) && (reader->P_Node->extra & NODE_IS_SPRESERVED))
			reader->preserves--;
		reader->P_Node = reader->P_Node->next;
		reader->state = XML_TEXTREADER_ELEMENT;
		/*
		 * Cleanup of the old node
		 */
		if((reader->preserves == 0) &&
#ifdef LIBXML_XINCLUDE_ENABLED
		    (reader->in_xinclude == 0) &&
#endif
		    (reader->entNr == 0) && reader->P_Node->prev && (reader->P_Node->prev->type != XML_DTD_NODE)) {
			xmlNode * tmp = reader->P_Node->prev;
			if((tmp->extra & NODE_IS_PRESERVED) == 0) {
				xmlUnlinkNode(tmp);
				xmlTextReaderFreeNode(reader, tmp);
			}
		}
		goto node_found;
	}
	if((oldstate == XML_TEXTREADER_ELEMENT) && (reader->P_Node->type == XML_ELEMENT_NODE) &&
	    !reader->P_Node->children && ((reader->P_Node->extra & NODE_IS_EMPTY) == 0)) {
		;
		reader->state = XML_TEXTREADER_END;
		goto node_found;
	}
#ifdef LIBXML_REGEXP_ENABLED
	if((reader->validate != XML_TEXTREADER_NOT_VALIDATE) && (reader->P_Node->type == XML_ELEMENT_NODE))
		xmlTextReaderValidatePop(reader);
#endif /* LIBXML_REGEXP_ENABLED */
	if(reader->preserves > 0 && (reader->P_Node->extra & NODE_IS_SPRESERVED))
		reader->preserves--;
	reader->P_Node = reader->P_Node->P_ParentNode;
	if(!reader->P_Node || (reader->P_Node->type == XML_DOCUMENT_NODE) ||
#ifdef LIBXML_DOCB_ENABLED
	    (reader->P_Node->type == XML_DOCB_DOCUMENT_NODE) ||
#endif
	    (reader->P_Node->type == XML_HTML_DOCUMENT_NODE)) {
		if(reader->mode != XML_TEXTREADER_MODE_EOF) {
			val = xmlParseChunk(reader->ctxt, "", 0, 1);
			reader->state = XML_TEXTREADER_DONE;
			if(val)
				return -1;
		}
		reader->P_Node = NULL;
		reader->depth = -1;
		/*
		 * Cleanup of the old node
		 */
		if(oldnode && (reader->preserves == 0) &&
#ifdef LIBXML_XINCLUDE_ENABLED
		    (reader->in_xinclude == 0) &&
#endif
		    (reader->entNr == 0) && (oldnode->type != XML_DTD_NODE) && ((oldnode->extra & NODE_IS_PRESERVED) == 0)) {
			xmlUnlinkNode(oldnode);
			xmlTextReaderFreeNode(reader, oldnode);
		}

		goto node_end;
	}
	if((reader->preserves == 0) &&
#ifdef LIBXML_XINCLUDE_ENABLED
	    (reader->in_xinclude == 0) &&
#endif
	    (reader->entNr == 0) && reader->P_Node->last && ((reader->P_Node->last->extra & NODE_IS_PRESERVED) == 0)) {
		xmlNode * tmp = reader->P_Node->last;
		xmlUnlinkNode(tmp);
		xmlTextReaderFreeNode(reader, tmp);
	}
	reader->depth--;
	reader->state = XML_TEXTREADER_BACKTRACK;
node_found:
	DUMP_READER
	/*
	 * If we are in the middle of a piece of CDATA make sure it's finished
	 */
	if(reader->P_Node && !reader->P_Node->next && ((reader->P_Node->type == XML_TEXT_NODE) || (reader->P_Node->type == XML_CDATA_SECTION_NODE))) {
		if(!xmlTextReaderExpand(reader))
			return -1;
	}
#ifdef LIBXML_XINCLUDE_ENABLED
	/*
	 * Handle XInclude if asked for
	 */
	if(reader->xinclude && reader->P_Node && (reader->P_Node->type == XML_ELEMENT_NODE) && reader->P_Node->ns &&
	    (sstreq(reader->P_Node->ns->href, XINCLUDE_NS) || sstreq(reader->P_Node->ns->href, XINCLUDE_OLD_NS))) {
		if(!reader->xincctxt) {
			reader->xincctxt = xmlXIncludeNewContext(reader->ctxt->myDoc);
			xmlXIncludeSetFlags(reader->xincctxt, reader->parserFlags & (~XML_PARSE_NOXINCNODE));
		}
		/*
		 * expand that node and process it
		 */
		if(!xmlTextReaderExpand(reader))
			return -1;
		xmlXIncludeProcessNode(reader->xincctxt, reader->P_Node);
	}
	if(reader->P_Node && reader->P_Node->type == XML_XINCLUDE_START) {
		reader->in_xinclude++;
		goto get_next_node;
	}
	if(reader->P_Node && reader->P_Node->type == XML_XINCLUDE_END) {
		reader->in_xinclude--;
		goto get_next_node;
	}
#endif
	/*
	 * Handle entities enter and exit when in entity replacement mode
	 */
	if(reader->P_Node && (reader->P_Node->type == XML_ENTITY_REF_NODE) && reader->ctxt && (reader->ctxt->replaceEntities == 1)) {
		// 
		// Case where the underlying tree is not availble, lookup the entity and walk it.
		// 
		if(!reader->P_Node->children && reader->ctxt->sax && reader->ctxt->sax->getEntity) {
			reader->P_Node->children = (xmlNode *)reader->ctxt->sax->getEntity(reader->ctxt, reader->P_Node->name);
		}
		if(reader->P_Node->children && (reader->P_Node->children->type == XML_ENTITY_DECL) && reader->P_Node->children->children) {
			xmlTextReaderEntPush(reader, reader->P_Node);
			reader->P_Node = reader->P_Node->children->children;
		}
#ifdef LIBXML_REGEXP_ENABLED
	}
	else if(reader->P_Node && (reader->P_Node->type == XML_ENTITY_REF_NODE) && reader->ctxt && (reader->validate)) {
		xmlTextReaderValidateEntity(reader);
#endif /* LIBXML_REGEXP_ENABLED */
	}
	if(reader->P_Node && (reader->P_Node->type == XML_ENTITY_DECL) && reader->ent && (reader->ent->children == reader->P_Node)) {
		reader->P_Node = xmlTextReaderEntPop(reader);
		reader->depth++;
		goto get_next_node;
	}
#ifdef LIBXML_REGEXP_ENABLED
	if((reader->validate != XML_TEXTREADER_NOT_VALIDATE) && reader->P_Node) {
		xmlNode * p_node = reader->P_Node;
		if(p_node->type == XML_ELEMENT_NODE && !oneof2(reader->state, XML_TEXTREADER_END, XML_TEXTREADER_BACKTRACK)) {
			xmlTextReaderValidatePush(reader);
		}
		else if(oneof2(p_node->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE)) {
			xmlTextReaderValidateCData(reader, p_node->content);
		}
	}
#endif /* LIBXML_REGEXP_ENABLED */
#ifdef LIBXML_PATTERN_ENABLED
	if((reader->patternNr > 0) && (reader->state != XML_TEXTREADER_END) && (reader->state != XML_TEXTREADER_BACKTRACK)) {
		for(int i = 0; i < reader->patternNr; i++) {
			if(xmlPatternMatch(reader->patternTab[i], reader->P_Node) == 1) {
				xmlTextReaderPreserve(reader);
				break;
			}
		}
	}
#endif /* LIBXML_PATTERN_ENABLED */
#ifdef LIBXML_SCHEMAS_ENABLED
	if((reader->validate == XML_TEXTREADER_VALIDATE_XSD) && (reader->xsdValidErrors == 0) && reader->xsdValidCtxt) {
		reader->xsdValidErrors = !xmlSchemaIsValid(reader->xsdValidCtxt);
	}
#endif /* LIBXML_PATTERN_ENABLED */
	return 1;
node_end:
	reader->state = XML_TEXTREADER_DONE;
	return 0;
}
/**
 * xmlTextReaderReadState:
 * @reader:  the (xmlTextReader *) used
 *
 * Gets the read state of the reader.
 *
 * Returns the state value, or -1 in case of error
 */
int xmlTextReaderReadState(xmlTextReader * reader)
{
	return reader ? reader->mode : -1;
}
/**
 * xmlTextReaderExpand:
 * @reader:  the (xmlTextReader *) used
 *
 * Reads the contents of the current node and the full subtree. It then makes
 * the subtree available until the next xmlTextReaderRead() call
 *
 * Returns a node pointer valid until the next xmlTextReaderRead() call
 *    or NULL in case of error.
 */
xmlNode * xmlTextReaderExpand(xmlTextReader * reader) 
{
	if(!reader || !reader->P_Node)
		return 0;
	if(reader->doc)
		return (reader->P_Node);
	if(!reader->ctxt)
		return 0;
	if(xmlTextReaderDoExpand(reader) < 0)
		return 0;
	return (reader->P_Node);
}

/**
 * xmlTextReaderNext:
 * @reader:  the (xmlTextReader *) used
 *
 * Skip to the node following the current one in document order while
 * avoiding the subtree if any.
 *
 * Returns 1 if the node was read successfully, 0 if there is no more
 *     nodes to read, or -1 in case of error
 */
int xmlTextReaderNext(xmlTextReader * reader) 
{
	int ret;
	xmlNode * cur;
	if(!reader)
		return -1;
	if(reader->doc)
		return xmlTextReaderNextTree(reader);
	cur = reader->P_Node;
	if(!cur || (cur->type != XML_ELEMENT_NODE))
		return xmlTextReaderRead(reader);
	if(oneof2(reader->state, XML_TEXTREADER_END, XML_TEXTREADER_BACKTRACK))
		return xmlTextReaderRead(reader);
	if(cur->extra & NODE_IS_EMPTY)
		return xmlTextReaderRead(reader);
	do {
		ret = xmlTextReaderRead(reader);
		if(ret != 1)
			return ret;
	} while(reader->P_Node != cur);
	return xmlTextReaderRead(reader);
}

#ifdef LIBXML_WRITER_ENABLED
/**
 * xmlTextReaderReadInnerXml:
 * @reader:  the (xmlTextReader *) used
 *
 * Reads the contents of the current node, including child nodes and markup.
 *
 * Returns a string containing the XML content, or NULL if the current node
 *    is neither an element nor attribute, or has no child nodes. The
 *    string must be deallocated by the caller.
 */
xmlChar * xmlTextReaderReadInnerXml(xmlTextReader * reader ATTRIBUTE_UNUSED)
{
	xmlChar * resbuf = 0;
	if(xmlTextReaderExpand(reader)) {
		xmlDoc * doc = reader->doc;
		xmlBuffer * buff = xmlBufferCreate();
		for(xmlNode * cur_node = reader->P_Node->children; cur_node; cur_node = cur_node->next) {
			xmlNode * p_node = xmlDocCopyNode(cur_node, doc, 1);
			xmlBuffer * buff2 = xmlBufferCreate();
			if(xmlNodeDump(buff2, doc, p_node, 0, 0) == -1) {
				xmlFreeNode(p_node);
				xmlBufferFree(buff2);
				xmlBufferFree(buff);
				return NULL;
			}
			xmlBufferCat(buff, buff2->content);
			xmlFreeNode(p_node);
			xmlBufferFree(buff2);
		}
		resbuf = buff->content;
		buff->content = NULL;
		xmlBufferFree(buff);
	}
	return resbuf;
}

#endif

#ifdef LIBXML_WRITER_ENABLED
/**
 * xmlTextReaderReadOuterXml:
 * @reader:  the (xmlTextReader *) used
 *
 * Reads the contents of the current node, including child nodes and markup.
 *
 * Returns a string containing the node and any XML content, or NULL if the
 *    current node cannot be serialized. The string must be deallocated by the caller.
 */
xmlChar * xmlTextReaderReadOuterXml(xmlTextReader * reader ATTRIBUTE_UNUSED)
{
	xmlChar * resbuf = 0;
	if(xmlTextReaderExpand(reader)) {
		xmlDoc * doc = reader->doc;
		xmlNode * p_node = reader->P_Node;
		p_node = (p_node->type == XML_DTD_NODE) ? (xmlNode *)xmlCopyDtd((xmlDtd *)p_node) : xmlDocCopyNode(p_node, doc, 1);
		{
			xmlBuffer * buff = xmlBufferCreate();
			if(xmlNodeDump(buff, doc, p_node, 0, 0) == -1) {
				xmlFreeNode(p_node);
				xmlBufferFree(buff);
				return NULL;
			}
			resbuf = buff->content;
			buff->content = NULL;
			xmlFreeNode(p_node);
			xmlBufferFree(buff);
		}
	}
	return resbuf;
}

#endif

/**
 * xmlTextReaderReadString:
 * @reader:  the (xmlTextReader *) used
 *
 * Reads the contents of an element or a text node as a string.
 *
 * Returns a string containing the contents of the Element or Text node,
 *    or NULL if the reader is positioned on any other type of node.
 *    The string must be deallocated by the caller.
 */
xmlChar * xmlTextReaderReadString(xmlTextReader * reader)
{
	if(reader && reader->P_Node) {
		xmlNode * P_Node = reader->curnode ? reader->curnode : reader->P_Node;
		switch(P_Node->type) {
			case XML_TEXT_NODE:
				return sstrdup(P_Node->content);
				break;
			case XML_ELEMENT_NODE:
				if(xmlTextReaderDoExpand(reader) != -1)
					return xmlTextReaderCollectSiblings(P_Node->children);
				break;
			case XML_ATTRIBUTE_NODE:
				TODO
				break;
			default:
				break;
		}
	}
	return 0;
}

#if 0
/**
 * xmlTextReaderReadBase64:
 * @reader:  the (xmlTextReader *) used
 * @array:  a byte array to store the content.
 * @offset:  the zero-based index into array where the method should
 *      begin to write.
 * @len:  the number of bytes to write.
 *
 * Reads and decodes the Base64 encoded contents of an element and
 * stores the result in a byte buffer.
 *
 * Returns the number of bytes written to array, or zero if the current
 *    instance is not positioned on an element or -1 in case of error.
 */
int xmlTextReaderReadBase64(xmlTextReader * reader, uchar * array ATTRIBUTE_UNUSED, int offset ATTRIBUTE_UNUSED, int len ATTRIBUTE_UNUSED) 
{
	if(!reader || !reader->ctxt)
		return -1;
	if(reader->ctxt->wellFormed != 1)
		return -1;
	if((reader->node == NULL) || (reader->node->type == XML_ELEMENT_NODE))
		return 0;
	TODO
	return 0;
}

/**
 * xmlTextReaderReadBinHex:
 * @reader:  the (xmlTextReader *) used
 * @array:  a byte array to store the content.
 * @offset:  the zero-based index into array where the method should
 *      begin to write.
 * @len:  the number of bytes to write.
 *
 * Reads and decodes the BinHex encoded contents of an element and
 * stores the result in a byte buffer.
 *
 * Returns the number of bytes written to array, or zero if the current
 *    instance is not positioned on an element or -1 in case of error.
 */
int xmlTextReaderReadBinHex(xmlTextReader * reader,
    uchar * array ATTRIBUTE_UNUSED,
    int offset ATTRIBUTE_UNUSED,
    int len ATTRIBUTE_UNUSED) {
	if(!reader || !reader->ctxt)
		return -1;
	if(reader->ctxt->wellFormed != 1)
		return -1;

	if((reader->node == NULL) || (reader->node->type == XML_ELEMENT_NODE))
		return 0;
	TODO
	return 0;
}

#endif

/************************************************************************
*									*
*			Operating on a preparsed tree			*
*									*
************************************************************************/
static int xmlTextReaderNextTree(xmlTextReader * reader)
{
	if(!reader)
		return -1;
	if(reader->state == XML_TEXTREADER_END)
		return 0;
	if(!reader->P_Node) {
		if(reader->doc->children == NULL) {
			reader->state = XML_TEXTREADER_END;
			return 0;
		}
		else {
			reader->P_Node = reader->doc->children;
			reader->state = XML_TEXTREADER_START;
			return 1;
		}
	}
	if(reader->state != XML_TEXTREADER_BACKTRACK) {
		/* Here removed traversal to child, because we want to skip the subtree,
		   replace with traversal to sibling to skip subtree */
		if(reader->P_Node->next != 0) {
			/* Move to sibling if present,skipping sub-tree */
			reader->P_Node = reader->P_Node->next;
			reader->state = XML_TEXTREADER_START;
			return 1;
		}
		/* if reader->node->next is NULL mean no subtree for current node,
		   so need to move to sibling of parent node if present */
		if(oneof2(reader->P_Node->type, XML_ELEMENT_NODE, XML_ATTRIBUTE_NODE)) {
			reader->state = XML_TEXTREADER_BACKTRACK;
			/* This will move to parent if present */
			xmlTextReaderRead(reader);
		}
	}
	if(reader->P_Node->next != 0) {
		reader->P_Node = reader->P_Node->next;
		reader->state = XML_TEXTREADER_START;
		return 1;
	}
	if(reader->P_Node->P_ParentNode != 0) {
		if(reader->P_Node->P_ParentNode->type == XML_DOCUMENT_NODE) {
			reader->state = XML_TEXTREADER_END;
			return 0;
		}
		reader->P_Node = reader->P_Node->P_ParentNode;
		reader->depth--;
		reader->state = XML_TEXTREADER_BACKTRACK;
		/* Repeat process to move to sibling of parent node if present */
		xmlTextReaderNextTree(reader);
	}
	reader->state = XML_TEXTREADER_END;
	return 1;
}
/**
 * xmlTextReaderReadTree:
 * @reader:  the (xmlTextReader *) used
 *
 *  Moves the position of the current instance to the next node in
 *  the stream, exposing its properties.
 *
 *  Returns 1 if the node was read successfully, 0 if there is no more
 *     nodes to read, or -1 in case of error
 */
static int xmlTextReaderReadTree(xmlTextReader * reader) 
{
	if(reader->state == XML_TEXTREADER_END)
		return 0;
next_node:
	if(!reader->P_Node) {
		if(reader->doc->children == NULL) {
			reader->state = XML_TEXTREADER_END;
			return 0;
		}
		else {
			reader->P_Node = reader->doc->children;
			reader->state = XML_TEXTREADER_START;
			goto found_node;
		}
	}
	if((reader->state != XML_TEXTREADER_BACKTRACK) && !oneof3(reader->P_Node->type, XML_DTD_NODE, XML_XINCLUDE_START, XML_ENTITY_REF_NODE)) {
		if(reader->P_Node->children) {
			reader->P_Node = reader->P_Node->children;
			reader->depth++;
			reader->state = XML_TEXTREADER_START;
			goto found_node;
		}
		if(reader->P_Node->type == XML_ATTRIBUTE_NODE) {
			reader->state = XML_TEXTREADER_BACKTRACK;
			goto found_node;
		}
	}
	if(reader->P_Node->next) {
		reader->P_Node = reader->P_Node->next;
		reader->state = XML_TEXTREADER_START;
		goto found_node;
	}

	if(reader->P_Node->P_ParentNode) {
		if((reader->P_Node->P_ParentNode->type == XML_DOCUMENT_NODE) || (reader->P_Node->P_ParentNode->type == XML_HTML_DOCUMENT_NODE)) {
			reader->state = XML_TEXTREADER_END;
			return 0;
		}
		else {
			reader->P_Node = reader->P_Node->P_ParentNode;
			reader->depth--;
			reader->state = XML_TEXTREADER_BACKTRACK;
			goto found_node;
		}
	}
	reader->state = XML_TEXTREADER_END;
found_node:
	if(oneof2(reader->P_Node->type, XML_XINCLUDE_START, XML_XINCLUDE_END))
		goto next_node;
	return 1;
}

/**
 * xmlTextReaderNextSibling:
 * @reader:  the (xmlTextReader *) used
 *
 * Skip to the node following the current one in document order while
 * avoiding the subtree if any.
 * Currently implemented only for Readers built on a document
 *
 * Returns 1 if the node was read successfully, 0 if there is no more
 *     nodes to read, or -1 in case of error
 */
int xmlTextReaderNextSibling(xmlTextReader * reader)
{
	if(!reader)
		return -1;
	else if(!reader->doc) {
		/* @todo */
		return -1;
	}
	else if(reader->state == XML_TEXTREADER_END)
		return 0;
	else if(!reader->P_Node)
		return xmlTextReaderNextTree(reader);
	else if(reader->P_Node->next) {
		reader->P_Node = reader->P_Node->next;
		reader->state = XML_TEXTREADER_START;
		return 1;
	}
	else
		return 0;
}

/************************************************************************
*									*
*			Constructor and destructors			*
*									*
************************************************************************/
/**
 * xmlNewTextReader:
 * @input: the xmlParserInputBufferPtr used to read data
 * @URI: the URI information for the source if available
 *
 * Create an xmlTextReader structure fed with @input
 *
 * Returns the new xmlTextReaderPtr or NULL in case of error
 */
xmlTextReader * xmlNewTextReader(xmlParserInputBuffer * input, const char * URI)
{
	if(!input)
		return 0;
	xmlTextReader * ret = (xmlTextReader *)SAlloc::M(sizeof(xmlTextReader));
	if(!ret) {
		xmlGenericError(0, "xmlNewTextReader : malloc failed\n");
		return 0;
	}
	memzero(ret, sizeof(xmlTextReader));
	ret->doc = NULL;
	ret->entTab = NULL;
	ret->entMax = 0;
	ret->entNr = 0;
	ret->input = input;
	ret->buffer = xmlBufCreateSize(100);
	if(!ret->buffer) {
		SAlloc::F(ret);
		xmlGenericError(0, "xmlNewTextReader : malloc failed\n");
		return 0;
	}
	ret->sax = (xmlSAXHandler*)SAlloc::M(sizeof(xmlSAXHandler));
	if(!ret->sax) {
		xmlBufFree(ret->buffer);
		SAlloc::F(ret);
		xmlGenericError(0, "xmlNewTextReader : malloc failed\n");
		return 0;
	}
	xmlSAXVersion(ret->sax, 2);
	ret->startElement = ret->sax->startElement;
	ret->sax->startElement = xmlTextReaderStartElement;
	ret->endElement = ret->sax->endElement;
	ret->sax->endElement = xmlTextReaderEndElement;
#ifdef LIBXML_SAX1_ENABLED
	if(ret->sax->initialized == XML_SAX2_MAGIC) {
#endif /* LIBXML_SAX1_ENABLED */
		ret->startElementNs = ret->sax->startElementNs;
		ret->sax->startElementNs = xmlTextReaderStartElementNs;
		ret->endElementNs = ret->sax->endElementNs;
		ret->sax->endElementNs = xmlTextReaderEndElementNs;
#ifdef LIBXML_SAX1_ENABLED
	}
	else {
		ret->startElementNs = NULL;
		ret->endElementNs = NULL;
	}
#endif /* LIBXML_SAX1_ENABLED */
	ret->characters = ret->sax->characters;
	ret->sax->characters = xmlTextReaderCharacters;
	ret->sax->ignorableWhitespace = xmlTextReaderCharacters;
	ret->cdataBlock = ret->sax->cdataBlock;
	ret->sax->cdataBlock = xmlTextReaderCDataBlock;

	ret->mode = XML_TEXTREADER_MODE_INITIAL;
	ret->P_Node = NULL;
	ret->curnode = NULL;
	if(xmlBufUse(ret->input->buffer) < 4) {
		xmlParserInputBufferRead(input, 4);
	}
	if(xmlBufUse(ret->input->buffer) >= 4) {
		ret->ctxt = xmlCreatePushParserCtxt(ret->sax, NULL, (const char *)xmlBufContent(ret->input->buffer),
		    4, URI);
		ret->base = 0;
		ret->cur = 4;
	}
	else {
		ret->ctxt = xmlCreatePushParserCtxt(ret->sax, NULL, NULL, 0, URI);
		ret->base = 0;
		ret->cur = 0;
	}
	if(ret->ctxt == NULL) {
		xmlGenericError(0, "xmlNewTextReader : malloc failed\n");
		xmlBufFree(ret->buffer);
		SAlloc::F(ret->sax);
		SAlloc::F(ret);
		return 0;
	}
	ret->ctxt->parseMode = XML_PARSE_READER;
	ret->ctxt->_private = ret;
	ret->ctxt->linenumbers = 1;
	ret->ctxt->dictNames = 1;
	ret->allocs = XML_TEXTREADER_CTXT;
	/*
	 * use the parser dictionnary to allocate all elements and attributes names
	 */
	ret->ctxt->docdict = 1;
	ret->dict = ret->ctxt->dict;
#ifdef LIBXML_XINCLUDE_ENABLED
	ret->xinclude = 0;
#endif
#ifdef LIBXML_PATTERN_ENABLED
	ret->patternMax = 0;
	ret->patternTab = NULL;
#endif
	return ret;
}
/**
 * xmlNewTextReaderFilename:
 * @URI: the URI of the resource to process
 *
 * Create an xmlTextReader structure fed with the resource at @URI
 *
 * Returns the new xmlTextReaderPtr or NULL in case of error
 */
xmlTextReader * xmlNewTextReaderFilename(const char * URI) 
{
	xmlTextReader * ret = 0;
	xmlParserInputBuffer * input = xmlParserInputBufferCreateFilename(URI, XML_CHAR_ENCODING_NONE);
	if(input) {
		ret = xmlNewTextReader(input, URI);
		if(!ret)
			xmlFreeParserInputBuffer(input);
		else {
			char * directory = NULL;
			ret->allocs |= XML_TEXTREADER_INPUT;
			if(ret->ctxt->directory == NULL)
				directory = xmlParserGetDirectory(URI);
			if(!ret->ctxt->directory && directory)
				ret->ctxt->directory = sstrdup(directory);
			SAlloc::F(directory);
		}
	}
	return ret;
}
/**
 * xmlFreeTextReader:
 * @reader:  the xmlTextReaderPtr
 *
 * Deallocate all the resources associated to the reader
 */
void xmlFreeTextReader(xmlTextReader * reader)
{
	if(reader) {
#ifdef LIBXML_SCHEMAS_ENABLED
		xmlRelaxNGFree(reader->rngSchemas);
		reader->rngSchemas = NULL;
		if(reader->rngValidCtxt) {
			if(!reader->rngPreserveCtxt)
				xmlRelaxNGFreeValidCtxt(reader->rngValidCtxt);
			reader->rngValidCtxt = NULL;
		}
		xmlSchemaSAXUnplug(reader->xsdPlug);
		reader->xsdPlug = NULL;
		if(reader->xsdValidCtxt) {
			if(!reader->xsdPreserveCtxt)
				xmlSchemaFreeValidCtxt(reader->xsdValidCtxt);
			reader->xsdValidCtxt = NULL;
		}
		if(reader->xsdSchemas) {
			xmlSchemaFree(reader->xsdSchemas);
			reader->xsdSchemas = NULL;
		}
#endif
#ifdef LIBXML_XINCLUDE_ENABLED
		xmlXIncludeFreeContext(reader->xincctxt);
#endif
#ifdef LIBXML_PATTERN_ENABLED
		if(reader->patternTab) {
			for(int i = 0; i < reader->patternNr; i++) {
				xmlFreePattern(reader->patternTab[i]);
			}
			SAlloc::F(reader->patternTab);
		}
#endif
		xmlFreeNode(reader->faketext);
		if(reader->ctxt) {
			if(reader->dict == reader->ctxt->dict)
				reader->dict = NULL;
			if(reader->ctxt->myDoc) {
				if(reader->preserve == 0)
					xmlTextReaderFreeDoc(reader, reader->ctxt->myDoc);
				reader->ctxt->myDoc = NULL;
			}
			if(reader->ctxt->vctxt.vstateTab && reader->ctxt->vctxt.vstateMax > 0) {
				SAlloc::F(reader->ctxt->vctxt.vstateTab);
				reader->ctxt->vctxt.vstateTab = NULL;
				reader->ctxt->vctxt.vstateMax = 0;
			}
			if(reader->allocs & XML_TEXTREADER_CTXT)
				xmlFreeParserCtxt(reader->ctxt);
		}
		SAlloc::F(reader->sax);
		if(reader->input  && (reader->allocs & XML_TEXTREADER_INPUT))
			xmlFreeParserInputBuffer(reader->input);
		if(reader->buffer)
			xmlBufFree(reader->buffer);
		SAlloc::F(reader->entTab);
		xmlDictFree(reader->dict);
		SAlloc::F(reader);
	}
}

/************************************************************************
*									*
*			Methods for XmlTextReader			*
*									*
************************************************************************/
/**
 * xmlTextReaderClose:
 * @reader:  the (xmlTextReader *) used
 *
 * This method releases any resources allocated by the current instance
 * changes the state to Closed and close any underlying input.
 *
 * Returns 0 or -1 in case of error
 */
int xmlTextReaderClose(xmlTextReader * reader)
{
	if(!reader)
		return -1;
	else {
		reader->P_Node = NULL;
		reader->curnode = NULL;
		reader->mode = XML_TEXTREADER_MODE_CLOSED;
		if(reader->ctxt) {
			xmlStopParser(reader->ctxt);
			if(reader->ctxt->myDoc) {
				if(reader->preserve == 0)
					xmlTextReaderFreeDoc(reader, reader->ctxt->myDoc);
				reader->ctxt->myDoc = NULL;
			}
		}
		if(reader->input && (reader->allocs & XML_TEXTREADER_INPUT)) {
			xmlFreeParserInputBuffer(reader->input);
			reader->allocs -= XML_TEXTREADER_INPUT;
		}
		return 0;
	}
}

/**
 * xmlTextReaderGetAttributeNo:
 * @reader:  the (xmlTextReader *) used
 * @no: the zero-based index of the attribute relative to the containing element
 *
 * Provides the value of the attribute with the specified index relative
 * to the containing element.
 *
 * Returns a string containing the value of the specified attribute, or NULL
 *  in case of error. The string must be deallocated by the caller.
 */
xmlChar * xmlTextReaderGetAttributeNo(xmlTextReader * reader, int no) 
{
	xmlChar * ret;
	int i;
	xmlAttr * cur;
	xmlNs * ns;
	if(!reader || !reader->P_Node)
		return 0;
	if(reader->curnode)
		return 0;
	/* @todo handle the xmlDecl */
	if(reader->P_Node->type != XML_ELEMENT_NODE)
		return 0;
	ns = reader->P_Node->nsDef;
	for(i = 0; (i < no) && ns; i++) {
		ns = ns->next;
	}
	if(ns)
		return sstrdup(ns->href);
	cur = reader->P_Node->properties;
	if(!cur)
		return 0;
	for(; i < no; i++) {
		cur = cur->next;
		if(!cur)
			return 0;
	}
	/* @todo walk the DTD if present */
	ret = xmlNodeListGetString(reader->P_Node->doc, cur->children, 1);
	return NZOR(ret, sstrdup((xmlChar *)""));
}

/**
 * xmlTextReaderGetAttribute:
 * @reader:  the (xmlTextReader *) used
 * @name: the qualified name of the attribute.
 *
 * Provides the value of the attribute with the specified qualified name.
 *
 * Returns a string containing the value of the specified attribute, or NULL
 *  in case of error. The string must be deallocated by the caller.
 */
xmlChar * xmlTextReaderGetAttribute(xmlTextReader * reader, const xmlChar * name) 
{
	xmlChar * prefix = NULL;
	xmlChar * localname;
	xmlNs * ns;
	xmlChar * ret = NULL;
	if(!reader || !name || !reader->P_Node)
		return 0;
	if(reader->curnode)
		return 0;
	// @todo handle the xmlDecl 
	if(reader->P_Node->type != XML_ELEMENT_NODE)
		return 0;
	localname = xmlSplitQName2(name, &prefix);
	if(!localname) {
		/*
		 * Namespace default decl
		 */
		if(sstreq(name, "xmlns")) {
			for(ns = reader->P_Node->nsDef; ns; ns = ns->next) {
				if(!ns->prefix)
					return sstrdup(ns->href);
			}
			return NULL;
		}
		else
			return xmlGetNoNsProp(reader->P_Node, name);
	}
	/*
	 * Namespace default decl
	 */
	if(sstreq(prefix, "xmlns")) {
		for(ns = reader->P_Node->nsDef; ns; ns = ns->next) {
			if(ns->prefix && sstreq(ns->prefix, localname)) {
				ret = sstrdup(ns->href);
				break;
			}
		}
	}
	else {
		ns = xmlSearchNs(reader->P_Node->doc, reader->P_Node, prefix);
		if(ns)
			ret = xmlGetNsProp(reader->P_Node, localname, ns->href);
	}
	SAlloc::F(localname);
	SAlloc::F(prefix);
	return ret;
}

/**
 * xmlTextReaderGetAttributeNs:
 * @reader:  the (xmlTextReader *) used
 * @localName: the local name of the attribute.
 * @namespaceURI: the namespace URI of the attribute.
 *
 * Provides the value of the specified attribute
 *
 * Returns a string containing the value of the specified attribute, or NULL
 *  in case of error. The string must be deallocated by the caller.
 */
xmlChar * xmlTextReaderGetAttributeNs(xmlTextReader * reader, const xmlChar * localName, const xmlChar * namespaceURI) 
{
	xmlChar * prefix = NULL;
	xmlNs * ns;
	if(!reader || !localName || !reader->P_Node)
		return 0;
	if(reader->curnode)
		return 0;
	// @todo handle the xmlDecl 
	if(reader->P_Node->type != XML_ELEMENT_NODE)
		return 0;
	if(sstreq(namespaceURI, "http://www.w3.org/2000/xmlns/")) {
		if(!sstreq(localName, "xmlns")) {
			prefix = BAD_CAST localName;
		}
		for(ns = reader->P_Node->nsDef; ns; ns = ns->next) {
			if(!prefix && !ns->prefix || (ns->prefix && sstreq(ns->prefix, localName)))
				return sstrdup(ns->href);
		}
		return NULL;
	}
	return xmlGetNsProp(reader->P_Node, localName, namespaceURI);
}

/**
 * xmlTextReaderGetRemainder:
 * @reader:  the (xmlTextReader *) used
 *
 * Method to get the remainder of the buffered XML. this method stops the
 * parser, set its state to End Of File and return the input stream with
 * what is left that the parser did not use.
 *
 * The implementation is not good, the parser certainly procgressed past
 * what's left in reader->input, and there is an allocation problem. Best
 * would be to rewrite it differently.
 *
 * Returns the xmlParserInputBufferPtr attached to the XML or NULL
 *  in case of error.
 */
xmlParserInputBuffer * xmlTextReaderGetRemainder(xmlTextReader * reader) 
{
	xmlParserInputBuffer * ret = NULL;
	if(reader && reader->P_Node) {
		reader->P_Node = NULL;
		reader->curnode = NULL;
		reader->mode = XML_TEXTREADER_MODE_EOF;
		if(reader->ctxt) {
			xmlStopParser(reader->ctxt);
			if(reader->ctxt->myDoc) {
				if(reader->preserve == 0)
					xmlTextReaderFreeDoc(reader, reader->ctxt->myDoc);
				reader->ctxt->myDoc = NULL;
			}
		}
		if(reader->allocs & XML_TEXTREADER_INPUT) {
			ret = reader->input;
			reader->input = NULL;
			reader->allocs -= XML_TEXTREADER_INPUT;
		}
		else {
			/*
			 * Hum, one may need to duplicate the data structure because
			 * without reference counting the input may be freed twice:
			 * - by the layer which allocated it.
			 * - by the layer to which would have been returned to.
			 */
			TODO
			return 0;
		}
	}
	return ret;
}

/**
 * xmlTextReaderLookupNamespace:
 * @reader:  the (xmlTextReader *) used
 * @prefix: the prefix whose namespace URI is to be resolved. To return
 *     the default namespace, specify NULL
 *
 * Resolves a namespace prefix in the scope of the current element.
 *
 * Returns a string containing the namespace URI to which the prefix maps
 *  or NULL in case of error. The string must be deallocated by the caller.
 */
xmlChar * xmlTextReaderLookupNamespace(xmlTextReader * reader, const xmlChar * prefix) 
{
	xmlNs * ns;
	if(!reader || !reader->P_Node)
		return 0;
	ns = xmlSearchNs(reader->P_Node->doc, reader->P_Node, prefix);
	if(ns == NULL)
		return 0;
	return sstrdup(ns->href);
}
/**
 * xmlTextReaderMoveToAttributeNo:
 * @reader:  the (xmlTextReader *) used
 * @no: the zero-based index of the attribute relative to the containing
 * element.
 *
 * Moves the position of the current instance to the attribute with
 * the specified index relative to the containing element.
 *
 * Returns 1 in case of success, -1 in case of error, 0 if not found
 */
int xmlTextReaderMoveToAttributeNo(xmlTextReader * reader, int no) 
{
	int i;
	xmlAttr * cur;
	xmlNs * ns;
	if(!reader || !reader->P_Node)
		return -1;
	// @todo handle the xmlDecl 
	if(reader->P_Node->type != XML_ELEMENT_NODE)
		return -1;
	reader->curnode = NULL;
	ns = reader->P_Node->nsDef;
	for(i = 0; (i < no) && ns; i++) {
		ns = ns->next;
	}
	if(ns) {
		reader->curnode = (xmlNode *)ns;
		return 1;
	}
	cur = reader->P_Node->properties;
	if(!cur)
		return 0;
	for(; i < no; i++) {
		cur = cur->next;
		if(!cur)
			return 0;
	}
	/* @todo walk the DTD if present */
	reader->curnode = (xmlNode *)cur;
	return 1;
}

/**
 * xmlTextReaderMoveToAttribute:
 * @reader:  the (xmlTextReader *) used
 * @name: the qualified name of the attribute.
 *
 * Moves the position of the current instance to the attribute with
 * the specified qualified name.
 *
 * Returns 1 in case of success, -1 in case of error, 0 if not found
 */
int xmlTextReaderMoveToAttribute(xmlTextReader * reader, const xmlChar * name) 
{
	xmlChar * prefix = NULL;
	xmlChar * localname = 0;
	xmlAttr * prop;
	if(!reader || !name || !reader->P_Node)
		return -1;
	// @todo handle the xmlDecl 
	if(reader->P_Node->type != XML_ELEMENT_NODE)
		return 0;
	localname = xmlSplitQName2(name, &prefix);
	if(localname == NULL) {
		/*
		 * Namespace default decl
		 */
		if(sstreq(name, "xmlns")) {
			for(xmlNs * ns = reader->P_Node->nsDef; ns; ns = ns->next) {
				if(ns->prefix == NULL) {
					reader->curnode = (xmlNode *)ns;
					return 1;
				}
			}
			return 0;
		}
		prop = reader->P_Node->properties;
		while(prop) {
			/*
			 * One need to have
			 * - same attribute names
			 * - and the attribute carrying that namespace
			 */
			if(sstreq(prop->name, name) && (!prop->ns || !prop->ns->prefix)) {
				reader->curnode = (xmlNode *)prop;
				return 1;
			}
			prop = prop->next;
		}
		return 0;
	}
	/*
	 * Namespace default decl
	 */
	if(sstreq(prefix, "xmlns")) {
		for(xmlNs * ns = reader->P_Node->nsDef; ns; ns = ns->next) {
			if(ns->prefix && sstreq(ns->prefix, localname)) {
				reader->curnode = (xmlNode *)ns;
				goto found;
			}
		}
		goto not_found;
	}
	prop = reader->P_Node->properties;
	while(prop) {
		/*
		 * One need to have
		 * - same attribute names
		 * - and the attribute carrying that namespace
		 */
		if(sstreq(prop->name, localname) && prop->ns && sstreq(prop->ns->prefix, prefix)) {
			reader->curnode = (xmlNode *)prop;
			goto found;
		}
		prop = prop->next;
	}
not_found:
	SAlloc::F(localname);
	SAlloc::F(prefix);
	return 0;
found:
	SAlloc::F(localname);
	SAlloc::F(prefix);
	return 1;
}

/**
 * xmlTextReaderMoveToAttributeNs:
 * @reader:  the (xmlTextReader *) used
 * @localName:  the local name of the attribute.
 * @namespaceURI:  the namespace URI of the attribute.
 *
 * Moves the position of the current instance to the attribute with the
 * specified local name and namespace URI.
 *
 * Returns 1 in case of success, -1 in case of error, 0 if not found
 */
int xmlTextReaderMoveToAttributeNs(xmlTextReader * reader, const xmlChar * localName, const xmlChar * namespaceURI) 
{
	xmlAttr * prop;
	xmlNode * P_Node;
	xmlNs * ns;
	xmlChar * prefix = NULL;
	if(!reader || !localName || !namespaceURI || !reader->P_Node)
		return -1;
	if(reader->P_Node->type != XML_ELEMENT_NODE)
		return 0;
	P_Node = reader->P_Node;
	if(sstreq(namespaceURI, "http://www.w3.org/2000/xmlns/")) {
		if(!sstreq(localName, "xmlns")) {
			prefix = BAD_CAST localName;
		}
		ns = reader->P_Node->nsDef;
		while(ns) {
			if((prefix == NULL && ns->prefix == NULL) || (ns->prefix && sstreq(ns->prefix, localName))) {
				reader->curnode = (xmlNode *)ns;
				return 1;
			}
			ns = ns->next;
		}
		return 0;
	}
	prop = P_Node->properties;
	while(prop) {
		/*
		 * One need to have
		 * - same attribute names
		 * - and the attribute carrying that namespace
		 */
		if(sstreq(prop->name, localName) && (prop->ns && (sstreq(prop->ns->href, namespaceURI)))) {
			reader->curnode = (xmlNode *)prop;
			return 1;
		}
		prop = prop->next;
	}
	return 0;
}

/**
 * xmlTextReaderMoveToFirstAttribute:
 * @reader:  the (xmlTextReader *) used
 *
 * Moves the position of the current instance to the first attribute
 * associated with the current node.
 *
 * Returns 1 in case of success, -1 in case of error, 0 if not found
 */
int xmlTextReaderMoveToFirstAttribute(xmlTextReader * reader) 
{
	if(!reader || !reader->P_Node)
		return -1;
	if(reader->P_Node->type != XML_ELEMENT_NODE)
		return 0;
	if(reader->P_Node->nsDef) {
		reader->curnode = (xmlNode *)reader->P_Node->nsDef;
		return 1;
	}
	if(reader->P_Node->properties) {
		reader->curnode = (xmlNode *)reader->P_Node->properties;
		return 1;
	}
	return 0;
}

/**
 * xmlTextReaderMoveToNextAttribute:
 * @reader:  the (xmlTextReader *) used
 *
 * Moves the position of the current instance to the next attribute
 * associated with the current node.
 *
 * Returns 1 in case of success, -1 in case of error, 0 if not found
 */
int xmlTextReaderMoveToNextAttribute(xmlTextReader * reader) 
{
	if(!reader || !reader->P_Node)
		return -1;
	else if(reader->P_Node->type != XML_ELEMENT_NODE)
		return 0;
	else if(!reader->curnode)
		return xmlTextReaderMoveToFirstAttribute(reader);
	else if(reader->curnode->type == XML_NAMESPACE_DECL) {
		xmlNs * ns = (xmlNs *)reader->curnode;
		if(ns->next) {
			reader->curnode = (xmlNode *)ns->next;
			return 1;
		}
		else if(reader->P_Node->properties) {
			reader->curnode = (xmlNode *)reader->P_Node->properties;
			return 1;
		}
		else 
			return 0;
	}
	else if((reader->curnode->type == XML_ATTRIBUTE_NODE) && reader->curnode->next) {
		reader->curnode = reader->curnode->next;
		return 1;
	}
	else
		return 0;
}
/**
 * xmlTextReaderMoveToElement:
 * @reader:  the (xmlTextReader *) used
 *
 * Moves the position of the current instance to the node that
 * contains the current Attribute  node.
 *
 * Returns 1 in case of success, -1 in case of error, 0 if not moved
 */
int xmlTextReaderMoveToElement(xmlTextReader * reader) 
{
	if(!reader || !reader->P_Node)
		return -1;
	if(reader->P_Node->type != XML_ELEMENT_NODE)
		return 0;
	if(reader->curnode) {
		reader->curnode = NULL;
		return 1;
	}
	return 0;
}

/**
 * xmlTextReaderReadAttributeValue:
 * @reader:  the (xmlTextReader *) used
 *
 * Parses an attribute value into one or more Text and EntityReference nodes.
 *
 * Returns 1 in case of success, 0 if the reader was not positionned on an
 *    ttribute node or all the attribute values have been read, or -1
 *    in case of error.
 */
int xmlTextReaderReadAttributeValue(xmlTextReader * reader) 
{
	if(!reader || !reader->P_Node)
		return -1;
	if(!reader->curnode)
		return 0;
	if(reader->curnode->type == XML_ATTRIBUTE_NODE) {
		if(reader->curnode->children == NULL)
			return 0;
		reader->curnode = reader->curnode->children;
	}
	else if(reader->curnode->type == XML_NAMESPACE_DECL) {
		xmlNs * ns = (xmlNs *)reader->curnode;
		if(reader->faketext == NULL) {
			reader->faketext = xmlNewDocText(reader->P_Node->doc, ns->href);
		}
		else {
			if(reader->faketext->content && (reader->faketext->content != (xmlChar *)&(reader->faketext->properties)))
				SAlloc::F(reader->faketext->content);
			reader->faketext->content = sstrdup(ns->href);
		}
		reader->curnode = reader->faketext;
	}
	else {
		if(reader->curnode->next == NULL)
			return 0;
		reader->curnode = reader->curnode->next;
	}
	return 1;
}

/**
 * xmlTextReaderConstEncoding:
 * @reader:  the (xmlTextReader *) used
 *
 * Determine the encoding of the document being read.
 *
 * Returns a string containing the encoding of the document or NULL in
 * case of error.  The string is deallocated with the reader.
 */
const xmlChar * xmlTextReaderConstEncoding(xmlTextReader * reader) 
{
	xmlDoc * doc = NULL;
	if(!reader)
		return 0;
	if(reader->doc)
		doc = reader->doc;
	else if(reader->ctxt)
		doc = reader->ctxt->myDoc;
	return (doc && doc->encoding) ? CONSTSTR(doc->encoding) : 0;
}

/************************************************************************
*									*
*			Acces API to the current node			*
*									*
************************************************************************/
/**
 * xmlTextReaderAttributeCount:
 * @reader:  the (xmlTextReader *) used
 *
 * Provides the number of attributes of the current node
 *
 * Returns 0 i no attributes, -1 in case of error or the attribute count
 */
int xmlTextReaderAttributeCount(xmlTextReader * reader) 
{
	int ret = 0;
	if(reader) {
		if(reader->P_Node) {
			const xmlNode * P_Node = reader->curnode ? reader->curnode : reader->P_Node;
			if(P_Node->type == XML_ELEMENT_NODE && !oneof2(reader->state, XML_TEXTREADER_END, XML_TEXTREADER_BACKTRACK)) {
				for(const xmlAttr * attr = P_Node->properties; attr; attr = attr->next)
					ret++;
				for(const xmlNs * ns = P_Node->nsDef; ns; ns = ns->next)
					ret++;
			}
		}
	}
	else
		ret = -1;
	return ret;
}
/**
 * xmlTextReaderNodeType:
 * @reader:  the (xmlTextReader *) used
 *
 * Get the node type of the current node
 * Reference:
 * http://www.gnu.org/software/dotgnu/pnetlib-doc/System/Xml/XmlNodeType.html
 *
 * Returns the xmlNodeType of the current node or -1 in case of error
 */
int xmlTextReaderNodeType(xmlTextReader * reader) 
{
	xmlNode * P_Node;
	if(!reader)
		return -1;
	if(!reader->P_Node)
		return XML_READER_TYPE_NONE;
	P_Node = reader->curnode ? reader->curnode : reader->P_Node;
	switch(P_Node->type) {
		case XML_ELEMENT_NODE:
		    return oneof2(reader->state, XML_TEXTREADER_END, XML_TEXTREADER_BACKTRACK) ? XML_READER_TYPE_END_ELEMENT : XML_READER_TYPE_ELEMENT;
		case XML_NAMESPACE_DECL:
		case XML_ATTRIBUTE_NODE: return XML_READER_TYPE_ATTRIBUTE;
		case XML_TEXT_NODE:
		    if(xmlIsBlankNode(reader->P_Node)) {
			    if(xmlNodeGetSpacePreserve(reader->P_Node))
				    return (XML_READER_TYPE_SIGNIFICANT_WHITESPACE);
			    else
				    return (XML_READER_TYPE_WHITESPACE);
		    }
		    else {
			    return (XML_READER_TYPE_TEXT);
		    }
		case XML_CDATA_SECTION_NODE: return (XML_READER_TYPE_CDATA);
		case XML_ENTITY_REF_NODE: return (XML_READER_TYPE_ENTITY_REFERENCE);
		case XML_ENTITY_NODE: return (XML_READER_TYPE_ENTITY);
		case XML_PI_NODE: return (XML_READER_TYPE_PROCESSING_INSTRUCTION);
		case XML_COMMENT_NODE: return (XML_READER_TYPE_COMMENT);
		case XML_DOCUMENT_NODE:
		case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
		case XML_DOCB_DOCUMENT_NODE:
#endif
		    return (XML_READER_TYPE_DOCUMENT);
		case XML_DOCUMENT_FRAG_NODE: return (XML_READER_TYPE_DOCUMENT_FRAGMENT);
		case XML_NOTATION_NODE: return (XML_READER_TYPE_NOTATION);
		case XML_DOCUMENT_TYPE_NODE:
		case XML_DTD_NODE: return (XML_READER_TYPE_DOCUMENT_TYPE);
		case XML_ELEMENT_DECL:
		case XML_ATTRIBUTE_DECL:
		case XML_ENTITY_DECL:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END: return (XML_READER_TYPE_NONE);
	}
	return -1;
}

/**
 * xmlTextReaderIsEmptyElement:
 * @reader:  the (xmlTextReader *) used
 *
 * Check if the current node is empty
 *
 * Returns 1 if empty, 0 if not and -1 in case of error
 */
int xmlTextReaderIsEmptyElement(xmlTextReader * reader) 
{
	if(!reader || !reader->P_Node)
		return -1;
	if(reader->P_Node->type != XML_ELEMENT_NODE)
		return 0;
	if(reader->curnode)
		return 0;
	if(reader->P_Node->children)
		return 0;
	if(reader->state == XML_TEXTREADER_END)
		return 0;
	if(reader->doc)
		return 1;
#ifdef LIBXML_XINCLUDE_ENABLED
	if(reader->in_xinclude > 0)
		return 1;
#endif
	return ((reader->P_Node->extra & NODE_IS_EMPTY) != 0);
}

/**
 * xmlTextReaderLocalName:
 * @reader:  the (xmlTextReader *) used
 *
 * The local name of the node.
 *
 * Returns the local name or NULL if not available,
 * if non NULL it need to be freed by the caller.
 */
xmlChar * xmlTextReaderLocalName(xmlTextReader * reader) 
{
	xmlNode * P_Node;
	if(!reader || !reader->P_Node)
		return 0;
	P_Node = reader->curnode ? reader->curnode : reader->P_Node;
	if(P_Node->type == XML_NAMESPACE_DECL) {
		xmlNs * ns = (xmlNs *)P_Node;
		return sstrdup(ns->prefix ? ns->prefix : reinterpret_cast<const xmlChar *>("xmlns"));
	}
	if((P_Node->type != XML_ELEMENT_NODE) && (P_Node->type != XML_ATTRIBUTE_NODE))
		return xmlTextReaderName(reader);
	return sstrdup(P_Node->name);
}
/**
 * xmlTextReaderConstLocalName:
 * @reader:  the (xmlTextReader *) used
 *
 * The local name of the node.
 *
 * Returns the local name or NULL if not available, the
 *    string will be deallocated with the reader.
 */
const xmlChar * xmlTextReaderConstLocalName(xmlTextReader * reader) 
{
	xmlNode * P_Node;
	if(!reader || !reader->P_Node)
		return 0;
	P_Node = reader->curnode ? reader->curnode : reader->P_Node;
	if(P_Node->type == XML_NAMESPACE_DECL) {
		xmlNs * ns = (xmlNs *)P_Node;
		if(ns->prefix == NULL)
			return CONSTSTR(reinterpret_cast<const xmlChar *>("xmlns"));
		else
			return ns->prefix;
	}
	if((P_Node->type != XML_ELEMENT_NODE) && (P_Node->type != XML_ATTRIBUTE_NODE))
		return xmlTextReaderConstName(reader);
	return P_Node->name;
}

/**
 * xmlTextReaderName:
 * @reader:  the (xmlTextReader *) used
 *
 * The qualified name of the node, equal to Prefix :LocalName.
 *
 * Returns the local name or NULL if not available,
 * if non NULL it need to be freed by the caller.
 */
xmlChar * xmlTextReaderName(xmlTextReader * reader) 
{
	xmlNode * P_Node;
	xmlChar * ret;
	if(!reader || !reader->P_Node)
		return 0;
	P_Node = reader->curnode ? reader->curnode : reader->P_Node;
	switch(P_Node->type) {
		case XML_ELEMENT_NODE:
		case XML_ATTRIBUTE_NODE:
		    if((P_Node->ns == NULL) || (P_Node->ns->prefix == NULL))
			    return sstrdup(P_Node->name);
		    ret = sstrdup(P_Node->ns->prefix);
		    ret = xmlStrcat(ret, reinterpret_cast<const xmlChar *>(":"));
		    ret = xmlStrcat(ret, P_Node->name);
		    return ret;
		case XML_TEXT_NODE:
		    return sstrdup(reinterpret_cast<const xmlChar *>("#text"));
		case XML_CDATA_SECTION_NODE:
		    return sstrdup(reinterpret_cast<const xmlChar *>("#cdata-section"));
		case XML_ENTITY_NODE:
		case XML_ENTITY_REF_NODE:
		    return sstrdup(P_Node->name);
		case XML_PI_NODE:
		    return sstrdup(P_Node->name);
		case XML_COMMENT_NODE:
		    return sstrdup(reinterpret_cast<const xmlChar *>("#comment"));
		case XML_DOCUMENT_NODE:
		case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
		case XML_DOCB_DOCUMENT_NODE:
#endif
		    return sstrdup(reinterpret_cast<const xmlChar *>("#document"));
		case XML_DOCUMENT_FRAG_NODE:
		    return sstrdup(reinterpret_cast<const xmlChar *>("#document-fragment"));
		case XML_NOTATION_NODE:
		    return sstrdup(P_Node->name);
		case XML_DOCUMENT_TYPE_NODE:
		case XML_DTD_NODE:
		    return sstrdup(P_Node->name);
		case XML_NAMESPACE_DECL: {
		    xmlNs * ns = (xmlNs *)P_Node;
		    ret = sstrdup(reinterpret_cast<const xmlChar *>("xmlns"));
		    if(ns->prefix == NULL)
			    return ret;
		    ret = xmlStrcat(ret, reinterpret_cast<const xmlChar *>(":"));
		    ret = xmlStrcat(ret, ns->prefix);
		    return ret;
	    }
		case XML_ELEMENT_DECL:
		case XML_ATTRIBUTE_DECL:
		case XML_ENTITY_DECL:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
		    return 0;
	}
	return 0;
}

/**
 * xmlTextReaderConstName:
 * @reader:  the (xmlTextReader *) used
 *
 * The qualified name of the node, equal to Prefix :LocalName.
 *
 * Returns the local name or NULL if not available, the string is
 *    deallocated with the reader.
 */
const xmlChar * xmlTextReaderConstName(xmlTextReader * reader) 
{
	xmlNode * p_node;
	if(!reader || !reader->P_Node)
		return 0;
	p_node = reader->curnode ? reader->curnode : reader->P_Node;
	switch(p_node->type) {
		case XML_ELEMENT_NODE:
		case XML_ATTRIBUTE_NODE:
		    if(!p_node->ns || !p_node->ns->prefix)
			    return (p_node->name);
		    return (CONSTQSTR(p_node->ns->prefix, p_node->name));
		case XML_TEXT_NODE: return CONSTSTR(reinterpret_cast<const xmlChar *>("#text"));
		case XML_CDATA_SECTION_NODE: return CONSTSTR(reinterpret_cast<const xmlChar *>("#cdata-section"));
		case XML_ENTITY_NODE:
		case XML_ENTITY_REF_NODE: return CONSTSTR(p_node->name);
		case XML_PI_NODE: return CONSTSTR(p_node->name);
		case XML_COMMENT_NODE: return CONSTSTR(reinterpret_cast<const xmlChar *>("#comment"));
		case XML_DOCUMENT_NODE:
		case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
		case XML_DOCB_DOCUMENT_NODE:
#endif
		    return (CONSTSTR(reinterpret_cast<const xmlChar *>("#document")));
		case XML_DOCUMENT_FRAG_NODE: return CONSTSTR(reinterpret_cast<const xmlChar *>("#document-fragment"));
		case XML_NOTATION_NODE: return (CONSTSTR(p_node->name));
		case XML_DOCUMENT_TYPE_NODE:
		case XML_DTD_NODE: return (CONSTSTR(p_node->name));
		case XML_NAMESPACE_DECL: 
			{
				xmlNs * ns = reinterpret_cast<xmlNs *>(p_node);
				if(ns->prefix == NULL)
					return (CONSTSTR(reinterpret_cast<const xmlChar *>("xmlns")));
				return (CONSTQSTR(reinterpret_cast<const xmlChar *>("xmlns"), ns->prefix));
			}
		case XML_ELEMENT_DECL:
		case XML_ATTRIBUTE_DECL:
		case XML_ENTITY_DECL:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
		    return 0;
	}
	return 0;
}
/**
 * xmlTextReaderPrefix:
 * @reader:  the (xmlTextReader *) used
 *
 * A shorthand reference to the namespace associated with the node.
 *
 * Returns the prefix or NULL if not available,
 *  if non NULL it need to be freed by the caller.
 */
xmlChar * xmlTextReaderPrefix(xmlTextReader * reader) 
{
	if(reader && reader->P_Node) {
		xmlNode * p_node = reader->curnode ? reader->curnode : reader->P_Node;
		if(p_node->type == XML_NAMESPACE_DECL) {
			xmlNs * ns = reinterpret_cast<xmlNs *>(p_node);
			return ns->prefix ? sstrdup(reinterpret_cast<const xmlChar *>("xmlns")) : 0;
		}
		else if(!oneof2(p_node->type, XML_ELEMENT_NODE, XML_ATTRIBUTE_NODE))
			return 0;
		else if(p_node->ns && p_node->ns->prefix)
			return sstrdup(p_node->ns->prefix);
	}
	return 0;
}
/**
 * xmlTextReaderConstPrefix:
 * @reader:  the (xmlTextReader *) used
 *
 * A shorthand reference to the namespace associated with the node.
 *
 * Returns the prefix or NULL if not available, the string is deallocated with the reader.
 */
const xmlChar * xmlTextReaderConstPrefix(xmlTextReader * reader) 
{
	xmlNode * P_Node;
	if(!reader || !reader->P_Node)
		return 0;
	P_Node = reader->curnode ? reader->curnode : reader->P_Node;
	if(P_Node->type == XML_NAMESPACE_DECL) {
		xmlNs * ns = (xmlNs *)P_Node;
		return ns->prefix ? CONSTSTR(reinterpret_cast<const xmlChar *>("xmlns")) : 0;
	}
	if(!oneof2(P_Node->type, XML_ELEMENT_NODE, XML_ATTRIBUTE_NODE))
		return 0;
	return (P_Node->ns && P_Node->ns->prefix) ? CONSTSTR(P_Node->ns->prefix) : 0;
}
/**
 * xmlTextReaderNamespaceUri:
 * @reader:  the (xmlTextReader *) used
 *
 * The URI defining the namespace associated with the node.
 *
 * Returns the namespace URI or NULL if not available, if non NULL it need to be freed by the caller.
 */
xmlChar * xmlTextReaderNamespaceUri(xmlTextReader * reader) 
{
	xmlNode * P_Node;
	if(!reader || !reader->P_Node)
		return 0;
	P_Node = reader->curnode ? reader->curnode : reader->P_Node;
	if(P_Node->type == XML_NAMESPACE_DECL)
		return sstrdup(reinterpret_cast<const xmlChar *>("http://www.w3.org/2000/xmlns/"));
	if(!oneof2(P_Node->type, XML_ELEMENT_NODE, XML_ATTRIBUTE_NODE))
		return 0;
	return P_Node->ns ? sstrdup(P_Node->ns->href) : 0;
}
/**
 * xmlTextReaderConstNamespaceUri:
 * @reader:  the (xmlTextReader *) used
 *
 * The URI defining the namespace associated with the node.
 *
 * Returns the namespace URI or NULL if not available, the string will be deallocated with the reader
 */
const xmlChar * xmlTextReaderConstNamespaceUri(xmlTextReader * reader) 
{
	if(!reader || !reader->P_Node)
		return 0;
	else {
		xmlNode * p_node = reader->curnode ? reader->curnode : reader->P_Node;
		if(p_node->type == XML_NAMESPACE_DECL)
			return CONSTSTR(reinterpret_cast<const xmlChar *>("http://www.w3.org/2000/xmlns/"));
		if(!oneof2(p_node->type, XML_ELEMENT_NODE, XML_ATTRIBUTE_NODE))
			return 0;
		return p_node->ns ? CONSTSTR(p_node->ns->href) : 0;
	}
}
/**
 * xmlTextReaderBaseUri:
 * @reader:  the (xmlTextReader *) used
 *
 * The base URI of the node.
 *
 * Returns the base URI or NULL if not available, if non NULL it need to be freed by the caller.
 */
xmlChar * xmlTextReaderBaseUri(xmlTextReader * reader) 
{
	return (reader && reader->P_Node) ? xmlNodeGetBase(NULL, reader->P_Node) : 0;
}
/**
 * xmlTextReaderConstBaseUri:
 * @reader:  the (xmlTextReader *) used
 *
 * The base URI of the node.
 *
 * Returns the base URI or NULL if not available, the string will be deallocated with the reader
 */
const xmlChar * xmlTextReaderConstBaseUri(xmlTextReader * reader) 
{
	const xmlChar * ret;
	if(!reader || !reader->P_Node)
		return 0;
	xmlChar * tmp = xmlNodeGetBase(NULL, reader->P_Node);
	if(!tmp)
		return 0;
	ret = CONSTSTR(tmp);
	SAlloc::F(tmp);
	return ret;
}
/**
 * xmlTextReaderDepth:
 * @reader:  the (xmlTextReader *) used
 *
 * The depth of the node in the tree.
 *
 * Returns the depth or -1 in case of error
 */
int xmlTextReaderDepth(xmlTextReader * reader) 
{
	if(!reader)
		return -1;
	else if(!reader->P_Node)
		return 0;
	else if(reader->curnode)
		return oneof2(reader->curnode->type, XML_ATTRIBUTE_NODE, XML_NAMESPACE_DECL) ? (reader->depth + 1) : (reader->depth + 2);
	else
		return reader->depth;
}
/**
 * xmlTextReaderHasAttributes:
 * @reader:  the (xmlTextReader *) used
 *
 * Whether the node has attributes.
 *
 * Returns 1 if true, 0 if false, and -1 in case or error
 */
int xmlTextReaderHasAttributes(xmlTextReader * reader) 
{
	if(!reader)
		return -1;
	if(!reader->P_Node)
		return 0;
	xmlNode * p_node = reader->curnode ? reader->curnode : reader->P_Node;
	if((p_node->type == XML_ELEMENT_NODE) && (p_node->properties || p_node->nsDef))
		return 1;
	/* @todo handle the xmlDecl */
	return 0;
}
/**
 * xmlTextReaderHasValue:
 * @reader:  the (xmlTextReader *) used
 *
 * Whether the node can have a text value.
 *
 * Returns 1 if true, 0 if false, and -1 in case or error
 */
int xmlTextReaderHasValue(xmlTextReader * reader) 
{
	if(!reader)
		return -1;
	if(!reader->P_Node)
		return 0;
	xmlNode * p_node = reader->curnode ? reader->curnode : reader->P_Node;
	switch(p_node->type) {
		case XML_ATTRIBUTE_NODE:
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		case XML_NAMESPACE_DECL:
		    return 1;
		default:
		    break;
	}
	return 0;
}
/**
 * xmlTextReaderValue:
 * @reader:  the (xmlTextReader *) used
 *
 * Provides the text value of the node if present
 *
 * Returns the string or NULL if not available. The result must be deallocated
 *   with SAlloc::F()
 */
xmlChar * xmlTextReaderValue(xmlTextReader * reader) 
{
	if(reader && reader->P_Node) {
		xmlNode * p_node = reader->curnode ? reader->curnode : reader->P_Node;
		switch(p_node->type) {
			case XML_NAMESPACE_DECL:
				return sstrdup(reinterpret_cast<xmlNs *>(p_node)->href);
			case XML_ATTRIBUTE_NODE: 
				{
					xmlAttr * attr = reinterpret_cast<xmlAttr *>(p_node);
					return xmlNodeListGetString(attr->parent ? attr->parent->doc : 0, attr->children, 1);
				}
				break;
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			case XML_PI_NODE:
			case XML_COMMENT_NODE:
				return sstrdup(p_node->content);
			default:
				return 0;
				break;
		}
	}
	else
		return 0;
}
/**
 * xmlTextReaderConstValue:
 * @reader:  the (xmlTextReader *) used
 *
 * Provides the text value of the node if present
 *
 * Returns the string or NULL if not available. The result will be
 *   deallocated on the next Read() operation.
 */
const xmlChar * xmlTextReaderConstValue(xmlTextReader * reader)
{
	if(!reader || !reader->P_Node)
		return 0;
	xmlNode * p_node = reader->curnode ? reader->curnode : reader->P_Node;
	switch(p_node->type) {
		case XML_NAMESPACE_DECL:
		    return reinterpret_cast<xmlNs *>(p_node)->href;
		case XML_ATTRIBUTE_NODE: {
		    xmlAttr * attr = reinterpret_cast<xmlAttr *>(p_node);
		    if(attr->children && (attr->children->type == XML_TEXT_NODE) && (attr->children->next == NULL))
			    return (attr->children->content);
		    else {
			    if(reader->buffer == NULL) {
				    reader->buffer = xmlBufCreateSize(100);
				    if(reader->buffer == NULL) {
					    xmlGenericError(0, "xmlTextReaderSetup : malloc failed\n");
					    return 0;
				    }
			    }
			    else
				    xmlBufEmpty(reader->buffer);
			    xmlBufGetNodeContent(reader->buffer, p_node);
			    return xmlBufContent(reader->buffer);
		    }
		    break;
	    }
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		    return p_node->content;
		default:
		    break;
	}
	return 0;
}
/**
 * xmlTextReaderIsDefault:
 * @reader:  the (xmlTextReader *) used
 *
 * Whether an Attribute  node was generated from the default value
 * defined in the DTD or schema.
 *
 * Returns 0 if not defaulted, 1 if defaulted, and -1 in case of error
 */
int xmlTextReaderIsDefault(const xmlTextReader * reader)
{
	return reader ? 0 : -1;
}
/**
 * xmlTextReaderQuoteChar:
 * @reader:  the (xmlTextReader *) used
 *
 * The quotation mark character used to enclose the value of an attribute.
 *
 * Returns " or ' and -1 in case of error
 */
int xmlTextReaderQuoteChar(const xmlTextReader * reader)
{
	// @todo maybe lookup the attribute value for " first
	return reader ? (int)'"' : -1;
}
/**
 * xmlTextReaderXmlLang:
 * @reader:  the (xmlTextReader *) used
 *
 * The xml:lang scope within which the node resides.
 *
 * Returns the xml:lang value or NULL if none exists., if non NULL it need to be freed by the caller.
 */
xmlChar * xmlTextReaderXmlLang(xmlTextReader * reader)
{
	return (reader && reader->P_Node) ? xmlNodeGetLang(reader->P_Node) : 0;
}
/**
 * xmlTextReaderConstXmlLang:
 * @reader:  the (xmlTextReader *) used
 *
 * The xml:lang scope within which the node resides.
 *
 * Returns the xml:lang value or NULL if none exists.
 */
const xmlChar * xmlTextReaderConstXmlLang(xmlTextReader * reader)
{
	const xmlChar * ret = 0;
	if(reader && reader->P_Node) {
		xmlChar * tmp = xmlNodeGetLang(reader->P_Node);
		if(tmp) {
			ret = CONSTSTR(tmp);
			SAlloc::F(tmp);
		}
	}
	return ret;
}
/**
 * xmlTextReaderConstString:
 * @reader:  the (xmlTextReader *) used
 * @str:  the string to intern.
 *
 * Get an interned string from the reader, allows for example to speedup string name comparisons
 *
 * Returns an interned copy of the string or NULL in case of error. The string will be deallocated with the reader.
 */
const xmlChar * xmlTextReaderConstString(xmlTextReader * reader, const xmlChar * str)
{
	return reader ? CONSTSTR(str) : 0;
}
/**
 * xmlTextReaderNormalization:
 * @reader:  the (xmlTextReader *) used
 *
 * The value indicating whether to normalize white space and attribute values.
 * Since attribute value and end of line normalizations are a MUST in the XML
 * specification only the value true is accepted. The broken bahaviour of
 * accepting out of range character entities like &#0; is of course not
 * supported either.
 *
 * Returns 1 or -1 in case of error.
 */
int xmlTextReaderNormalization(xmlTextReader * reader)
{
	return reader ? 1 : -1;
}
// 
// Extensions to the base APIs
//
/**
 * xmlTextReaderSetParserProp:
 * @reader:  the (xmlTextReader *) used
 * @prop:  the xmlParserProperties to set
 * @value:  usually 0 or 1 to (de)activate it
 *
 * Change the parser processing behaviour by changing some of its internal
 * properties. Note that some properties can only be changed before any
 * read has been done.
 *
 * Returns 0 if the call was successful, or -1 in case of error
 */
int xmlTextReaderSetParserProp(xmlTextReader * reader, int prop, int value)
{
	if(reader && reader->ctxt) {
		xmlParserCtxt * ctxt = reader->ctxt;
		const xmlParserProperties p = static_cast<xmlParserProperties>(prop);
		switch(p) {
			case XML_PARSER_LOADDTD:
				if(value != 0) {
					if(ctxt->loadsubset == 0) {
						if(reader->mode != XML_TEXTREADER_MODE_INITIAL)
							return -1;
						else
							ctxt->loadsubset = XML_DETECT_IDS;
					}
				}
				else {
					ctxt->loadsubset = 0;
				}
				return 0;
			case XML_PARSER_DEFAULTATTRS:
				if(value)
					ctxt->loadsubset |= XML_COMPLETE_ATTRS;
				else if(ctxt->loadsubset & XML_COMPLETE_ATTRS)
					ctxt->loadsubset -= XML_COMPLETE_ATTRS;
				return 0;
			case XML_PARSER_VALIDATE:
				if(value != 0) {
					ctxt->validate = 1;
					reader->validate = XML_TEXTREADER_VALIDATE_DTD;
				}
				else
					ctxt->validate = 0;
				return 0;
			case XML_PARSER_SUBST_ENTITIES:
				ctxt->replaceEntities = value ? 1 : 0;
				return 0;
		}
	}
	return -1;
}

/**
 * xmlTextReaderGetParserProp:
 * @reader:  the (xmlTextReader *) used
 * @prop:  the xmlParserProperties to get
 *
 * Read the parser internal property.
 *
 * Returns the value, usually 0 or 1, or -1 in case of error.
 */
int xmlTextReaderGetParserProp(xmlTextReader * reader, int prop)
{
	int    ret = -1;
	if(reader && reader->ctxt) {
		xmlParserCtxt * ctxt = reader->ctxt;
		xmlParserProperties p = (xmlParserProperties)prop;
		switch(p) {
			case XML_PARSER_LOADDTD: ret = (ctxt->loadsubset || ctxt->validate) ? 1 : 0; break;
			case XML_PARSER_DEFAULTATTRS: ret = (ctxt->loadsubset & XML_COMPLETE_ATTRS) ? 1 : 0; break;
			case XML_PARSER_VALIDATE: ret = reader->validate; break;
			case XML_PARSER_SUBST_ENTITIES: ret = ctxt->replaceEntities; break;
		}
	}
	return ret;
}
/**
 * xmlTextReaderGetParserLineNumber:
 * @reader: the user data (XML reader context)
 *
 * Provide the line number of the current parsing point.
 *
 * Returns an int or 0 if not available
 */
int xmlTextReaderGetParserLineNumber(xmlTextReader * reader)
{
	return (reader && reader->ctxt && reader->ctxt->input) ? (reader->ctxt->input->line) : 0;
}
/**
 * xmlTextReaderGetParserColumnNumber:
 * @reader: the user data (XML reader context)
 *
 * Provide the column number of the current parsing point.
 *
 * Returns an int or 0 if not available
 */
int xmlTextReaderGetParserColumnNumber(xmlTextReader * reader)
{
	return (reader && reader->ctxt && reader->ctxt->input) ? (reader->ctxt->input->col) : 0;
}

/**
 * xmlTextReaderCurrentNode:
 * @reader:  the (xmlTextReader *) used
 *
 * Hacking interface allowing to get the xmlNode * correponding to the
 * current node being accessed by the xmlTextReader. This is dangerous
 * because the underlying node may be destroyed on the next Reads.
 *
 * Returns the xmlNode * or NULL in case of error.
 */
xmlNode * xmlTextReaderCurrentNode(xmlTextReader * reader)
{
	return reader ? (reader->curnode ? reader->curnode : reader->P_Node) : 0;
}

/**
 * xmlTextReaderPreserve:
 * @reader:  the (xmlTextReader *) used
 *
 * This tells the XML Reader to preserve the current node.
 * The caller must also use xmlTextReaderCurrentDoc() to
 * keep an handle on the resulting document once parsing has finished
 *
 * Returns the xmlNode * or NULL in case of error.
 */
xmlNode * xmlTextReaderPreserve(xmlTextReader * reader)
{
	xmlNode * cur = 0;
	if(reader) {
		cur = reader->curnode ? reader->curnode : reader->P_Node;
		if(cur) {
			if(!oneof2(cur->type, XML_DOCUMENT_NODE, XML_DTD_NODE)) {
				cur->extra |= NODE_IS_PRESERVED;
				cur->extra |= NODE_IS_SPRESERVED;
			}
			reader->preserves++;
			for(xmlNode * parent = cur->P_ParentNode; parent; parent = parent->P_ParentNode) {
				if(parent->type == XML_ELEMENT_NODE)
					parent->extra |= NODE_IS_PRESERVED;
			}
		}
	}
	return cur;
}

#ifdef LIBXML_PATTERN_ENABLED
/**
 * xmlTextReaderPreservePattern:
 * @reader:  the (xmlTextReader *) used
 * @pattern:  an XPath subset pattern
 * @namespaces: the prefix definitions, array of [URI, prefix] or NULL
 *
 * This tells the XML Reader to preserve all nodes matched by the
 * pattern. The caller must also use xmlTextReaderCurrentDoc() to
 * keep an handle on the resulting document once parsing has finished
 *
 * Returns a positive number in case of success and -1 in case of error
 */
int xmlTextReaderPreservePattern(xmlTextReader * reader, const xmlChar * pattern, const xmlChar ** namespaces)
{
	int    result = -1;
	if(reader && pattern) {
		xmlPattern * comp = xmlPatterncompile(pattern, reader->dict, 0, namespaces);
		if(comp) {
			if(reader->patternMax <= 0) {
				reader->patternMax = 4;
				reader->patternTab = (xmlPattern **)SAlloc::M(reader->patternMax * sizeof(reader->patternTab[0]));
				if(reader->patternTab == NULL) {
					xmlGenericError(0, "xmlMalloc failed !\n");
					return -1;
				}
			}
			if(reader->patternNr >= reader->patternMax) {
				reader->patternMax *= 2;
				xmlPattern ** tmp = (xmlPattern **)SAlloc::R(reader->patternTab, reader->patternMax * sizeof(reader->patternTab[0]));
				if(!tmp) {
					xmlGenericError(0, "xmlRealloc failed !\n");
					reader->patternMax /= 2;
					return -1;
				}
				else
					reader->patternTab = tmp;
			}
			reader->patternTab[reader->patternNr] = comp;
			result = (reader->patternNr++);
		}
	}
	return result;
}

#endif
/**
 * xmlTextReaderCurrentDoc:
 * @reader:  the (xmlTextReader *) used
 *
 * Hacking interface allowing to get the xmlDocPtr correponding to the
 * current document being accessed by the xmlTextReader.
 * NOTE: as a result of this call, the reader will not destroy the
 *  associated XML document and calling xmlFreeDoc() on the result
 *  is needed once the reader parsing has finished.
 *
 * Returns the xmlDocPtr or NULL in case of error.
 */
xmlDoc * xmlTextReaderCurrentDoc(xmlTextReader * reader)
{
	if(!reader)
		return 0;
	else if(reader->doc)
		return reader->doc;
	else if(!reader->ctxt || !reader->ctxt->myDoc)
		return 0;
	else {
		reader->preserve = 1;
		return reader->ctxt->myDoc;
	}
}

#ifdef LIBXML_SCHEMAS_ENABLED
static char * xmlTextReaderBuildMessage(const char * msg, va_list ap);
static void XMLCDECL xmlTextReaderValidityError(void * ctxt, const char * msg, ...);
static void XMLCDECL xmlTextReaderValidityWarning(void * ctxt, const char * msg, ...);

static void XMLCDECL xmlTextReaderValidityErrorRelay(void * ctx, const char * msg, ...)
{
	xmlTextReader * reader = static_cast<xmlTextReader *>(ctx);
	char * str;
	va_list ap;
	va_start(ap, msg);
	str = xmlTextReaderBuildMessage(msg, ap);
	if(!reader->errorFunc) {
		xmlTextReaderValidityError(ctx, "%s", str);
	}
	else {
		reader->errorFunc(reader->errorFuncArg, str, XML_PARSER_SEVERITY_VALIDITY_ERROR, NULL /* locator */);
	}
	SAlloc::F(str);
	va_end(ap);
}

static void XMLCDECL xmlTextReaderValidityWarningRelay(void * ctx, const char * msg, ...)
{
	xmlTextReader * reader = static_cast<xmlTextReader *>(ctx);
	char * str;
	va_list ap;
	va_start(ap, msg);
	str = xmlTextReaderBuildMessage(msg, ap);
	if(!reader->errorFunc) {
		xmlTextReaderValidityWarning(ctx, "%s", str);
	}
	else {
		reader->errorFunc(reader->errorFuncArg, str, XML_PARSER_SEVERITY_VALIDITY_WARNING, NULL /* locator */);
	}
	SAlloc::F(str);
	va_end(ap);
}

static void xmlTextReaderStructuredError(void * ctxt, xmlError * error);

static void xmlTextReaderValidityStructuredRelay(void * userData, xmlError * error)
{
	xmlTextReader * reader = static_cast<xmlTextReader *>(userData);
	if(reader->sErrorFunc)
		reader->sErrorFunc(reader->errorFuncArg, error);
	else
		xmlTextReaderStructuredError(reader, error);
}
/**
 * xmlTextReaderRelaxNGSetSchema:
 * @reader:  the (xmlTextReader *) used
 * @schema:  a precompiled RelaxNG schema
 *
 * Use RelaxNG to validate the document as it is processed.
 * Activation is only possible before the first Read().
 * if @schema is NULL, then RelaxNG validation is desactivated.
   @ The @schema should not be freed until the reader is deallocated
 * or its use has been deactivated.
 *
 * Returns 0 in case the RelaxNG validation could be (des)activated and
 *    -1 in case of error.
 */
int xmlTextReaderRelaxNGSetSchema(xmlTextReader * reader, xmlRelaxNGPtr schema)
{
	if(!reader)
		return -1;
	if(schema == NULL) {
		xmlRelaxNGFree(reader->rngSchemas);
		reader->rngSchemas = NULL;
		if(reader->rngValidCtxt != NULL) {
			if(!reader->rngPreserveCtxt)
				xmlRelaxNGFreeValidCtxt(reader->rngValidCtxt);
			reader->rngValidCtxt = NULL;
		}
		reader->rngPreserveCtxt = 0;
		return 0;
	}
	if(reader->mode != XML_TEXTREADER_MODE_INITIAL)
		return -1;
	xmlRelaxNGFree(reader->rngSchemas);
	reader->rngSchemas = NULL;
	if(reader->rngValidCtxt != NULL) {
		if(!reader->rngPreserveCtxt)
			xmlRelaxNGFreeValidCtxt(reader->rngValidCtxt);
		reader->rngValidCtxt = NULL;
	}
	reader->rngPreserveCtxt = 0;
	reader->rngValidCtxt = xmlRelaxNGNewValidCtxt(schema);
	if(reader->rngValidCtxt == NULL)
		return -1;
	if(reader->errorFunc != NULL) {
		xmlRelaxNGSetValidErrors(reader->rngValidCtxt, xmlTextReaderValidityErrorRelay, xmlTextReaderValidityWarningRelay, reader);
	}
	if(reader->sErrorFunc != NULL) {
		xmlRelaxNGSetValidStructuredErrors(reader->rngValidCtxt, xmlTextReaderValidityStructuredRelay, reader);
	}
	reader->rngValidErrors = 0;
	reader->rngFullNode = NULL;
	reader->validate = XML_TEXTREADER_VALIDATE_RNG;
	return 0;
}
/**
 * xmlTextReaderLocator:
 * @ctx: the xmlTextReaderPtr used
 * @file: returned file information
 * @line: returned line information
 *
 * Internal locator function for the readers
 *
 * Returns 0 in case the Schema validation could be (des)activated and
 *    -1 in case of error.
 */
static int xmlTextReaderLocator(void * ctx, const char ** file, unsigned long * line)
{
	xmlTextReader * reader;
	if(!ctx || ((file == NULL) && (line == NULL)))
		return -1;
	ASSIGN_PTR(file, 0);
	ASSIGN_PTR(line, 0);
	reader = static_cast<xmlTextReader *>(ctx);
	if(reader->ctxt && reader->ctxt->input) {
		ASSIGN_PTR(file, reader->ctxt->input->filename);
		ASSIGN_PTR(line, reader->ctxt->input->line);
		return 0;
	}
	if(reader->P_Node) {
		long res;
		int ret = 0;
		if(line != NULL) {
			res = xmlGetLineNo(reader->P_Node);
			if(res > 0)
				*line = (ulong)res;
			else
				ret = -1;
		}
		if(file != NULL) {
			xmlDoc * doc = reader->P_Node->doc;
			if(doc && doc->URL)
				*file = (const char *)doc->URL;
			else
				ret = -1;
		}
		return ret;
	}
	return -1;
}
/**
 * xmlTextReaderSetSchema:
 * @reader:  the (xmlTextReader *) used
 * @schema:  a precompiled Schema schema
 *
 * Use XSD Schema to validate the document as it is processed.
 * Activation is only possible before the first Read().
 * if @schema is NULL, then Schema validation is desactivated.
   @ The @schema should not be freed until the reader is deallocated
 * or its use has been deactivated.
 *
 * Returns 0 in case the Schema validation could be (des)activated and
 *    -1 in case of error.
 */
int xmlTextReaderSetSchema(xmlTextReader * reader, xmlSchemaPtr schema)
{
	if(!reader)
		return -1;
	else if(schema == NULL) {
		xmlSchemaSAXUnplug(reader->xsdPlug);
		reader->xsdPlug = NULL;
		if(reader->xsdValidCtxt != NULL) {
			if(!reader->xsdPreserveCtxt)
				xmlSchemaFreeValidCtxt(reader->xsdValidCtxt);
			reader->xsdValidCtxt = NULL;
		}
		reader->xsdPreserveCtxt = 0;
		if(reader->xsdSchemas != NULL) {
			xmlSchemaFree(reader->xsdSchemas);
			reader->xsdSchemas = NULL;
		}
		return 0;
	}
	else if(reader->mode != XML_TEXTREADER_MODE_INITIAL)
		return -1;
	else {
		xmlSchemaSAXUnplug(reader->xsdPlug);
		reader->xsdPlug = NULL;
		if(reader->xsdValidCtxt) {
			if(!reader->xsdPreserveCtxt)
				xmlSchemaFreeValidCtxt(reader->xsdValidCtxt);
			reader->xsdValidCtxt = NULL;
		}
		reader->xsdPreserveCtxt = 0;
		xmlSchemaFree(reader->xsdSchemas);
		reader->xsdSchemas = NULL;
		reader->xsdValidCtxt = xmlSchemaNewValidCtxt(schema);
		if(reader->xsdValidCtxt == NULL) {
			xmlSchemaFree(reader->xsdSchemas);
			reader->xsdSchemas = NULL;
			return -1;
		}
		else {
			reader->xsdPlug = xmlSchemaSAXPlug(reader->xsdValidCtxt, &(reader->ctxt->sax), &(reader->ctxt->userData));
			if(reader->xsdPlug == NULL) {
				xmlSchemaFree(reader->xsdSchemas);
				reader->xsdSchemas = NULL;
				xmlSchemaFreeValidCtxt(reader->xsdValidCtxt);
				reader->xsdValidCtxt = NULL;
				return -1;
			}
			else {
				xmlSchemaValidateSetLocator(reader->xsdValidCtxt, xmlTextReaderLocator, (void *)reader);
				if(reader->errorFunc)
					xmlSchemaSetValidErrors(reader->xsdValidCtxt, xmlTextReaderValidityErrorRelay, xmlTextReaderValidityWarningRelay, reader);
				if(reader->sErrorFunc)
					xmlSchemaSetValidStructuredErrors(reader->xsdValidCtxt, xmlTextReaderValidityStructuredRelay, reader);
				reader->xsdValidErrors = 0;
				reader->validate = XML_TEXTREADER_VALIDATE_XSD;
				return 0;
			}
		}
	}
}
/**
 * xmlTextReaderRelaxNGValidateInternal:
 * @reader:  the (xmlTextReader *) used
 * @rng:  the path to a RelaxNG schema or NULL
 * @ctxt: the RelaxNG schema validation context or NULL
 * @options: options (not yet used)
 *
 * Use RelaxNG to validate the document as it is processed.
 * Activation is only possible before the first Read().
 * If both @rng and @ctxt are NULL, then RelaxNG validation is deactivated.
 *
 * Returns 0 in case the RelaxNG validation could be (de)activated and
 *	   -1 in case of error.
 */
static int xmlTextReaderRelaxNGValidateInternal(xmlTextReader * reader, const char * rng, xmlRelaxNGValidCtxtPtr ctxt, int options ATTRIBUTE_UNUSED)
{
	if(!reader)
		return -1;
	if(rng && ctxt)
		return -1;
	if((rng || ctxt) && ((reader->mode != XML_TEXTREADER_MODE_INITIAL) || (reader->ctxt == NULL)))
		return -1;
	/* Cleanup previous validation stuff. */
	if(reader->rngValidCtxt != NULL) {
		if(!reader->rngPreserveCtxt)
			xmlRelaxNGFreeValidCtxt(reader->rngValidCtxt);
		reader->rngValidCtxt = NULL;
	}
	reader->rngPreserveCtxt = 0;
	xmlRelaxNGFree(reader->rngSchemas);
	reader->rngSchemas = NULL;
	if(!rng && !ctxt) {
		return 0; // We just want to deactivate the validation, so get out
	}
	if(rng) {
		// Parse the schema and create validation environment. 
		xmlRelaxNGParserCtxt * pctxt = xmlRelaxNGNewParserCtxt(rng);
		if(reader->errorFunc)
			xmlRelaxNGSetParserErrors(pctxt, xmlTextReaderValidityErrorRelay, xmlTextReaderValidityWarningRelay, reader);
		if(reader->sErrorFunc)
			xmlRelaxNGSetValidStructuredErrors(reader->rngValidCtxt, xmlTextReaderValidityStructuredRelay, reader);
		reader->rngSchemas = xmlRelaxNGParse(pctxt);
		xmlRelaxNGFreeParserCtxt(pctxt);
		if(reader->rngSchemas == NULL)
			return -1;
		else {
			reader->rngValidCtxt = xmlRelaxNGNewValidCtxt(reader->rngSchemas);
			if(reader->rngValidCtxt == NULL) {
				xmlRelaxNGFree(reader->rngSchemas);
				reader->rngSchemas = NULL;
				return -1;
			}
		}
	}
	else {
		/* Use the given validation context. */
		reader->rngValidCtxt = ctxt;
		reader->rngPreserveCtxt = 1;
	}
	/*
	 * Redirect the validation context's error channels to use
	 * the reader channels.
	 * @todo In case the user provides the validation context we
	 *	could make this redirection optional.
	 */
	if(reader->errorFunc != NULL) {
		xmlRelaxNGSetValidErrors(reader->rngValidCtxt, xmlTextReaderValidityErrorRelay, xmlTextReaderValidityWarningRelay, reader);
	}
	if(reader->sErrorFunc != NULL) {
		xmlRelaxNGSetValidStructuredErrors(reader->rngValidCtxt, xmlTextReaderValidityStructuredRelay, reader);
	}
	reader->rngValidErrors = 0;
	reader->rngFullNode = NULL;
	reader->validate = XML_TEXTREADER_VALIDATE_RNG;
	return 0;
}

/**
 * xmlTextReaderSchemaValidateInternal:
 * @reader:  the (xmlTextReader *) used
 * @xsd:  the path to a W3C XSD schema or NULL
 * @ctxt: the XML Schema validation context or NULL
 * @options: options (not used yet)
 *
 * Validate the document as it is processed using XML Schema.
 * Activation is only possible before the first Read().
 * If both @xsd and @ctxt are NULL then XML Schema validation is deactivated.
 *
 * Returns 0 in case the schemas validation could be (de)activated and
 *    -1 in case of error.
 */
static int xmlTextReaderSchemaValidateInternal(xmlTextReader * reader, const char * xsd, xmlSchemaValidCtxt * ctxt, int options ATTRIBUTE_UNUSED)
{
	if(!reader)
		return -1;
	if(xsd && ctxt)
		return -1;
	if(((xsd != NULL) || (ctxt)) && ((reader->mode != XML_TEXTREADER_MODE_INITIAL) || (reader->ctxt == NULL)))
		return -1;
	// Cleanup previous validation stuff
	xmlSchemaSAXUnplug(reader->xsdPlug);
	reader->xsdPlug = NULL;
	if(reader->xsdValidCtxt != NULL) {
		if(!reader->xsdPreserveCtxt)
			xmlSchemaFreeValidCtxt(reader->xsdValidCtxt);
		reader->xsdValidCtxt = NULL;
	}
	reader->xsdPreserveCtxt = 0;
	if(reader->xsdSchemas != NULL) {
		xmlSchemaFree(reader->xsdSchemas);
		reader->xsdSchemas = NULL;
	}
	if((xsd == NULL) && (ctxt == NULL)) {
		return 0; // We just want to deactivate the validation, so get out
	}
	if(xsd != NULL) {
		xmlSchemaParserCtxtPtr pctxt;
		/* Parse the schema and create validation environment. */
		pctxt = xmlSchemaNewParserCtxt(xsd);
		if(reader->errorFunc != NULL) {
			xmlSchemaSetParserErrors(pctxt, xmlTextReaderValidityErrorRelay, xmlTextReaderValidityWarningRelay, reader);
		}
		reader->xsdSchemas = xmlSchemaParse(pctxt);
		xmlSchemaFreeParserCtxt(pctxt);
		if(reader->xsdSchemas == NULL)
			return -1;
		reader->xsdValidCtxt = xmlSchemaNewValidCtxt(reader->xsdSchemas);
		if(reader->xsdValidCtxt == NULL) {
			xmlSchemaFree(reader->xsdSchemas);
			reader->xsdSchemas = NULL;
			return -1;
		}
		reader->xsdPlug = xmlSchemaSAXPlug(reader->xsdValidCtxt, &(reader->ctxt->sax), &(reader->ctxt->userData));
		if(reader->xsdPlug == NULL) {
			xmlSchemaFree(reader->xsdSchemas);
			reader->xsdSchemas = NULL;
			xmlSchemaFreeValidCtxt(reader->xsdValidCtxt);
			reader->xsdValidCtxt = NULL;
			return -1;
		}
	}
	else {
		/* Use the given validation context. */
		reader->xsdValidCtxt = ctxt;
		reader->xsdPreserveCtxt = 1;
		reader->xsdPlug = xmlSchemaSAXPlug(reader->xsdValidCtxt, &(reader->ctxt->sax), &(reader->ctxt->userData));
		if(reader->xsdPlug == NULL) {
			reader->xsdValidCtxt = NULL;
			reader->xsdPreserveCtxt = 0;
			return -1;
		}
	}
	xmlSchemaValidateSetLocator(reader->xsdValidCtxt, xmlTextReaderLocator, (void *)reader);
	/*
	 * Redirect the validation context's error channels to use
	 * the reader channels.
	 * @todo In case the user provides the validation context we
	 * could make this redirection optional.
	 */
	if(reader->errorFunc)
		xmlSchemaSetValidErrors(reader->xsdValidCtxt, xmlTextReaderValidityErrorRelay, xmlTextReaderValidityWarningRelay, reader);
	if(reader->sErrorFunc)
		xmlSchemaSetValidStructuredErrors(reader->xsdValidCtxt, xmlTextReaderValidityStructuredRelay, reader);
	reader->xsdValidErrors = 0;
	reader->validate = XML_TEXTREADER_VALIDATE_XSD;
	return 0;
}

/**
 * xmlTextReaderSchemaValidateCtxt:
 * @reader:  the (xmlTextReader *) used
 * @ctxt: the XML Schema validation context or NULL
 * @options: options (not used yet)
 *
 * Use W3C XSD schema context to validate the document as it is processed.
 * Activation is only possible before the first Read().
 * If @ctxt is NULL, then XML Schema validation is deactivated.
 *
 * Returns 0 in case the schemas validation could be (de)activated and
 *    -1 in case of error.
 */
int xmlTextReaderSchemaValidateCtxt(xmlTextReader * reader, xmlSchemaValidCtxt * ctxt, int options)
{
	return xmlTextReaderSchemaValidateInternal(reader, NULL, ctxt, options);
}

/**
 * xmlTextReaderSchemaValidate:
 * @reader:  the (xmlTextReader *) used
 * @xsd:  the path to a W3C XSD schema or NULL
 *
 * Use W3C XSD schema to validate the document as it is processed.
 * Activation is only possible before the first Read().
 * If @xsd is NULL, then XML Schema validation is deactivated.
 *
 * Returns 0 in case the schemas validation could be (de)activated and
 *    -1 in case of error.
 */
int xmlTextReaderSchemaValidate(xmlTextReader * reader, const char * xsd)
{
	return (xmlTextReaderSchemaValidateInternal(reader, xsd, NULL, 0));
}

/**
 * xmlTextReaderRelaxNGValidateCtxt:
 * @reader:  the (xmlTextReader *) used
 * @ctxt: the RelaxNG schema validation context or NULL
 * @options: options (not used yet)
 *
 * Use RelaxNG schema context to validate the document as it is processed.
 * Activation is only possible before the first Read().
 * If @ctxt is NULL, then RelaxNG schema validation is deactivated.
 *
 * Returns 0 in case the schemas validation could be (de)activated and
 *    -1 in case of error.
 */
int xmlTextReaderRelaxNGValidateCtxt(xmlTextReader * reader, xmlRelaxNGValidCtxtPtr ctxt, int options)
{
	return xmlTextReaderRelaxNGValidateInternal(reader, NULL, ctxt, options);
}

/**
 * xmlTextReaderRelaxNGValidate:
 * @reader:  the (xmlTextReader *) used
 * @rng:  the path to a RelaxNG schema or NULL
 *
 * Use RelaxNG schema to validate the document as it is processed.
 * Activation is only possible before the first Read().
 * If @rng is NULL, then RelaxNG schema validation is deactivated.
 *
 * Returns 0 in case the schemas validation could be (de)activated and
 *    -1 in case of error.
 */
int xmlTextReaderRelaxNGValidate(xmlTextReader * reader, const char * rng)
{
	return (xmlTextReaderRelaxNGValidateInternal(reader, rng, NULL, 0));
}

#endif

/**
 * xmlTextReaderIsNamespaceDecl:
 * @reader: the xmlTextReaderPtr used
 *
 * Determine whether the current node is a namespace declaration
 * rather than a regular attribute.
 *
 * Returns 1 if the current node is a namespace declaration, 0 if it
 * is a regular attribute or other type of node, or -1 in case of
 * error.
 */
int xmlTextReaderIsNamespaceDecl(xmlTextReader * reader) 
{
	if(!reader || !reader->P_Node)
		return -1;
	else {
		xmlNode * P_Node = NZOR(reader->curnode, reader->P_Node);
		return (XML_NAMESPACE_DECL == P_Node->type) ? 1 : 0;
	}
}

/**
 * xmlTextReaderConstXmlVersion:
 * @reader:  the (xmlTextReader *) used
 *
 * Determine the XML version of the document being read.
 *
 * Returns a string containing the XML version of the document or NULL
 * in case of error.  The string is deallocated with the reader.
 */
const xmlChar * xmlTextReaderConstXmlVersion(xmlTextReader * reader) 
{
	xmlDoc * doc = NULL;
	if(!reader)
		return 0;
	if(reader->doc)
		doc = reader->doc;
	else if(reader->ctxt)
		doc = reader->ctxt->myDoc;
	return (doc && doc->version) ? CONSTSTR(doc->version) : 0;
}

/**
 * xmlTextReaderStandalone:
 * @reader:  the (xmlTextReader *) used
 *
 * Determine the standalone status of the document being read.
 *
 * Returns 1 if the document was declared to be standalone, 0 if it
 * was declared to be not standalone, or -1 if the document did not
 * specify its standalone status or in case of error.
 */
int xmlTextReaderStandalone(xmlTextReader * reader) 
{
	xmlDoc * doc = NULL;
	if(!reader)
		return -1;
	if(reader->doc)
		doc = reader->doc;
	else if(reader->ctxt)
		doc = reader->ctxt->myDoc;
	if(!doc)
		return -1;
	return doc->standalone;
}
//
// Error Handling Extensions
//
/* helper to build a xmlMalloc'ed string from a format and va_list */
static char * xmlTextReaderBuildMessage(const char * msg, va_list ap)
{
	int size = 0;
	int chars;
	char * larger = 0;
	char * str = NULL;
	va_list aq;
	while(1) {
		VA_COPY(aq, ap);
		chars = vsnprintf(str, size, msg, aq);
		va_end(aq);
		if(chars < 0) {
			xmlGenericError(0, "vsnprintf failed !\n");
			SAlloc::F(str);
			return NULL;
		}
		if((chars < size) || (size == MAX_ERR_MSG_SIZE))
			break;
		size = (chars < MAX_ERR_MSG_SIZE) ? (chars + 1) : MAX_ERR_MSG_SIZE;
		larger = static_cast<char *>(SAlloc::R(str, size));
		if(larger == NULL) {
			xmlGenericError(0, "xmlRealloc failed !\n");
			SAlloc::F(str);
			return NULL;
		}
		str = larger;
	}
	return str;
}
/**
 * xmlTextReaderLocatorLineNumber:
 * @locator: the xmlTextReaderLocatorPtr used
 *
 * Obtain the line number for the given locator.
 *
 * Returns the line number or -1 in case of error.
 */
int xmlTextReaderLocatorLineNumber(xmlTextReaderLocatorPtr locator) 
{
	int ret = -1;
	if(locator) {
		// we know that locator is a xmlParserCtxtPtr 
		xmlParserCtxt * ctx = (xmlParserCtxt *)locator;
		if(ctx->P_Node)
			ret = xmlGetLineNo(ctx->P_Node);
		else {
			// inspired from error.c 
			xmlParserInput * input = ctx->input;
			if(!input->filename && ctx->inputNr > 1)
				input = ctx->inputTab[ctx->inputNr-2];
			ret = input ? input->line : -1;
		}
	}
	return ret;
}
/**
 * xmlTextReaderLocatorBaseURI:
 * @locator: the xmlTextReaderLocatorPtr used
 *
 * Obtain the base URI for the given locator.
 *
 * Returns the base URI or NULL in case of error,
 *  if non NULL it need to be freed by the caller.
 */
xmlChar * xmlTextReaderLocatorBaseURI(xmlTextReaderLocatorPtr locator)
{
	xmlChar * ret = NULL;
	if(locator) {
		// we know that locator is a xmlParserCtxtPtr 
		xmlParserCtxt * ctx = (xmlParserCtxt *)locator;
		if(ctx->P_Node) {
			ret = xmlNodeGetBase(NULL, ctx->P_Node);
		}
		else {
			/* inspired from error.c */
			xmlParserInput * input = ctx->input;
			if((input->filename == NULL) && (ctx->inputNr > 1))
				input = ctx->inputTab[ctx->inputNr - 2];
			ret = input ? sstrdup(BAD_CAST input->filename) : 0;
		}
	}
	return ret;
}

static void xmlTextReaderGenericError(void * ctxt, xmlParserSeverities severity, char * str)
{
	xmlParserCtxt * ctx = (xmlParserCtxt *)ctxt;
	xmlTextReader * reader = (xmlTextReader *)ctx->_private;
	if(str) {
		if(reader->errorFunc)
			reader->errorFunc(reader->errorFuncArg, str, severity, (xmlTextReaderLocatorPtr)ctx);
		SAlloc::F(str);
	}
}

static void xmlTextReaderStructuredError(void * ctxt, xmlError * error)
{
	xmlParserCtxt * ctx = (xmlParserCtxt *)ctxt;
	xmlTextReader * reader = (xmlTextReader *)ctx->_private;
	if(error && reader->sErrorFunc) {
		reader->sErrorFunc(reader->errorFuncArg, (xmlError *)error);
	}
}

static void XMLCDECL xmlTextReaderError(void * ctxt, const char * msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	xmlTextReaderGenericError(ctxt, XML_PARSER_SEVERITY_ERROR, xmlTextReaderBuildMessage(msg, ap));
	va_end(ap);
}

static void XMLCDECL xmlTextReaderWarning(void * ctxt, const char * msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	xmlTextReaderGenericError(ctxt, XML_PARSER_SEVERITY_WARNING, xmlTextReaderBuildMessage(msg, ap));
	va_end(ap);
}

static void XMLCDECL xmlTextReaderValidityError(void * ctxt, const char * msg, ...)
{
	va_list ap;
	int len = sstrlen(msg);
	if((len > 1) && (msg[len - 2] != ':')) {
		// some callbacks only report locator information: skip them (mimicking behaviour in error.c)
		va_start(ap, msg);
		xmlTextReaderGenericError(ctxt, XML_PARSER_SEVERITY_VALIDITY_ERROR, xmlTextReaderBuildMessage(msg, ap));
		va_end(ap);
	}
}

static void XMLCDECL xmlTextReaderValidityWarning(void * ctxt, const char * msg, ...)
{
	va_list ap;
	int len = sstrlen(msg);
	if(len && (msg[len-1] != ':')) {
		// some callbacks only report locator information: skip them (mimicking behaviour in error.c)
		va_start(ap, msg);
		xmlTextReaderGenericError(ctxt, XML_PARSER_SEVERITY_VALIDITY_WARNING, xmlTextReaderBuildMessage(msg, ap));
		va_end(ap);
	}
}

/**
 * xmlTextReaderSetErrorHandler:
 * @reader:  the (xmlTextReader *) used
 * @f:	the callback function to call on error and warnings
 * @arg:    a user argument to pass to the callback function
 *
 * Register a callback function that will be called on error and warnings.
 *
 * If @f is NULL, the default error and warning handlers are restored.
 */
void xmlTextReaderSetErrorHandler(xmlTextReader * reader, xmlTextReaderErrorFunc f, void * arg)
{
	if(f) {
		reader->ctxt->sax->error = xmlTextReaderError;
		reader->ctxt->sax->serror = NULL;
		reader->ctxt->vctxt.error = xmlTextReaderValidityError;
		reader->ctxt->sax->warning = xmlTextReaderWarning;
		reader->ctxt->vctxt.warning = xmlTextReaderValidityWarning;
		reader->errorFunc = f;
		reader->sErrorFunc = NULL;
		reader->errorFuncArg = arg;
#ifdef LIBXML_SCHEMAS_ENABLED
		if(reader->rngValidCtxt) {
			xmlRelaxNGSetValidErrors(reader->rngValidCtxt, xmlTextReaderValidityErrorRelay, xmlTextReaderValidityWarningRelay, reader);
			xmlRelaxNGSetValidStructuredErrors(reader->rngValidCtxt, NULL, reader);
		}
		if(reader->xsdValidCtxt) {
			xmlSchemaSetValidErrors(reader->xsdValidCtxt, xmlTextReaderValidityErrorRelay, xmlTextReaderValidityWarningRelay, reader);
			xmlSchemaSetValidStructuredErrors(reader->xsdValidCtxt, NULL, reader);
		}
#endif
	}
	else {
		/* restore defaults */
		reader->ctxt->sax->error = xmlParserError;
		reader->ctxt->vctxt.error = xmlParserValidityError;
		reader->ctxt->sax->warning = xmlParserWarning;
		reader->ctxt->vctxt.warning = xmlParserValidityWarning;
		reader->errorFunc = NULL;
		reader->sErrorFunc = NULL;
		reader->errorFuncArg = NULL;
#ifdef LIBXML_SCHEMAS_ENABLED
		if(reader->rngValidCtxt) {
			xmlRelaxNGSetValidErrors(reader->rngValidCtxt, NULL, NULL, reader);
			xmlRelaxNGSetValidStructuredErrors(reader->rngValidCtxt, NULL, reader);
		}
		if(reader->xsdValidCtxt) {
			xmlSchemaSetValidErrors(reader->xsdValidCtxt, NULL, NULL, reader);
			xmlSchemaSetValidStructuredErrors(reader->xsdValidCtxt, NULL, reader);
		}
#endif
	}
}

/**
 * xmlTextReaderSetStructuredErrorHandler:
 * @reader:  the (xmlTextReader *) used
 * @f:	the callback function to call on error and warnings
 * @arg:    a user argument to pass to the callback function
 *
 * Register a callback function that will be called on error and warnings.
 *
 * If @f is NULL, the default error and warning handlers are restored.
 */
void xmlTextReaderSetStructuredErrorHandler(xmlTextReader * reader, xmlStructuredErrorFunc f, void * arg)
{
	if(f) {
		reader->ctxt->sax->error = NULL;
		reader->ctxt->sax->serror = xmlTextReaderStructuredError;
		reader->ctxt->vctxt.error = xmlTextReaderValidityError;
		reader->ctxt->sax->warning = xmlTextReaderWarning;
		reader->ctxt->vctxt.warning = xmlTextReaderValidityWarning;
		reader->sErrorFunc = f;
		reader->errorFunc = NULL;
		reader->errorFuncArg = arg;
#ifdef LIBXML_SCHEMAS_ENABLED
		if(reader->rngValidCtxt) {
			xmlRelaxNGSetValidErrors(reader->rngValidCtxt, NULL, NULL, reader);
			xmlRelaxNGSetValidStructuredErrors(reader->rngValidCtxt, xmlTextReaderValidityStructuredRelay, reader);
		}
		if(reader->xsdValidCtxt) {
			xmlSchemaSetValidErrors(reader->xsdValidCtxt, NULL, NULL, reader);
			xmlSchemaSetValidStructuredErrors(reader->xsdValidCtxt, xmlTextReaderValidityStructuredRelay, reader);
		}
#endif
	}
	else {
		/* restore defaults */
		reader->ctxt->sax->error = xmlParserError;
		reader->ctxt->sax->serror = NULL;
		reader->ctxt->vctxt.error = xmlParserValidityError;
		reader->ctxt->sax->warning = xmlParserWarning;
		reader->ctxt->vctxt.warning = xmlParserValidityWarning;
		reader->errorFunc = NULL;
		reader->sErrorFunc = NULL;
		reader->errorFuncArg = NULL;
#ifdef LIBXML_SCHEMAS_ENABLED
		if(reader->rngValidCtxt) {
			xmlRelaxNGSetValidErrors(reader->rngValidCtxt, NULL, NULL, reader);
			xmlRelaxNGSetValidStructuredErrors(reader->rngValidCtxt, NULL, reader);
		}
		if(reader->xsdValidCtxt) {
			xmlSchemaSetValidErrors(reader->xsdValidCtxt, NULL, NULL, reader);
			xmlSchemaSetValidStructuredErrors(reader->xsdValidCtxt, NULL, reader);
		}
#endif
	}
}

/**
 * xmlTextReaderIsValid:
 * @reader:  the (xmlTextReader *) used
 *
 * Retrieve the validity status from the parser context
 *
 * Returns the flag value 1 if valid, 0 if no, and -1 in case of error
 */
int xmlTextReaderIsValid(xmlTextReader * reader)
{
	if(!reader)
		return -1;
#ifdef LIBXML_SCHEMAS_ENABLED
	if(reader->validate == XML_TEXTREADER_VALIDATE_RNG)
		return (reader->rngValidErrors == 0);
	if(reader->validate == XML_TEXTREADER_VALIDATE_XSD)
		return (reader->xsdValidErrors == 0);
#endif
	return (reader->ctxt && (reader->ctxt->validate == 1)) ? (reader->ctxt->valid) : 0;
}

/**
 * xmlTextReaderGetErrorHandler:
 * @reader:  the (xmlTextReader *) used
 * @f:	the callback function or NULL is no callback has been registered
 * @arg:    a user argument
 *
 * Retrieve the error callback function and user argument.
 */
void xmlTextReaderGetErrorHandler(xmlTextReader * reader, xmlTextReaderErrorFunc * f, void ** arg)
{
	ASSIGN_PTR(f, reader->errorFunc);
	ASSIGN_PTR(arg, reader->errorFuncArg);
}

/************************************************************************
*									*
*	New set (2.6.0) of simpler and more flexible APIs		*
*									*
************************************************************************/

/**
 * xmlTextReaderSetup:
 * @reader:  an XML reader
 * @input: xmlParserInputBufferPtr used to feed the reader, will
 *    be destroyed with it.
 * @URL:  the base URL to use for the document
 * @encoding:  the document encoding, or NULL
 * @options:  a combination of xmlParserOption
 *
 * Setup an XML reader with new options
 *
 * Returns 0 in case of success and -1 in case of error.
 */
int xmlTextReaderSetup(xmlTextReader * reader, xmlParserInputBuffer * input, const char * URL, const char * encoding, int options)
{
	if(!reader) {
		xmlFreeParserInputBuffer(input);
		return -1;
	}
	/*
	 * we force the generation of compact text nodes on the reader
	 * since usr applications should never modify the tree
	 */
	options |= XML_PARSE_COMPACT;
	reader->doc = NULL;
	reader->entNr = 0;
	reader->parserFlags = options;
	reader->validate = XML_TEXTREADER_NOT_VALIDATE;
	if(input && reader->input && (reader->allocs & XML_TEXTREADER_INPUT)) {
		xmlFreeParserInputBuffer(reader->input);
		reader->input = NULL;
		reader->allocs -= XML_TEXTREADER_INPUT;
	}
	if(input) {
		reader->input = input;
		reader->allocs |= XML_TEXTREADER_INPUT;
	}
	SETIFZ(reader->buffer, xmlBufCreateSize(100));
	if(reader->buffer == NULL) {
		xmlGenericError(0, "xmlTextReaderSetup : malloc failed\n");
		return -1;
	}
	SETIFZ(reader->sax, (xmlSAXHandler*)SAlloc::M(sizeof(xmlSAXHandler)));
	if(reader->sax == NULL) {
		xmlGenericError(0, "xmlTextReaderSetup : malloc failed\n");
		return -1;
	}
	xmlSAXVersion(reader->sax, 2);
	reader->startElement = reader->sax->startElement;
	reader->sax->startElement = xmlTextReaderStartElement;
	reader->endElement = reader->sax->endElement;
	reader->sax->endElement = xmlTextReaderEndElement;
#ifdef LIBXML_SAX1_ENABLED
	if(reader->sax->initialized == XML_SAX2_MAGIC) {
#endif /* LIBXML_SAX1_ENABLED */
	reader->startElementNs = reader->sax->startElementNs;
	reader->sax->startElementNs = xmlTextReaderStartElementNs;
	reader->endElementNs = reader->sax->endElementNs;
	reader->sax->endElementNs = xmlTextReaderEndElementNs;
#ifdef LIBXML_SAX1_ENABLED
}

else {
	reader->startElementNs = NULL;
	reader->endElementNs = NULL;
}
#endif /* LIBXML_SAX1_ENABLED */
	reader->characters = reader->sax->characters;
	reader->sax->characters = xmlTextReaderCharacters;
	reader->sax->ignorableWhitespace = xmlTextReaderCharacters;
	reader->cdataBlock = reader->sax->cdataBlock;
	reader->sax->cdataBlock = xmlTextReaderCDataBlock;
	reader->mode = XML_TEXTREADER_MODE_INITIAL;
	reader->P_Node = NULL;
	reader->curnode = NULL;
	if(input != NULL) {
		if(xmlBufUse(reader->input->buffer) < 4) {
			xmlParserInputBufferRead(input, 4);
		}
		if(!reader->ctxt) {
			if(xmlBufUse(reader->input->buffer) >= 4) {
				reader->ctxt = xmlCreatePushParserCtxt(reader->sax, NULL, (const char *)xmlBufContent(reader->input->buffer), 4, URL);
				reader->base = 0;
				reader->cur = 4;
			}
			else {
				reader->ctxt = xmlCreatePushParserCtxt(reader->sax, NULL, NULL, 0, URL);
				reader->base = 0;
				reader->cur = 0;
			}
		}
		else {
			xmlParserInput * inputStream;
			xmlParserInputBuffer * buf;
			xmlCharEncoding enc = XML_CHAR_ENCODING_NONE;
			xmlCtxtReset(reader->ctxt);
			buf = xmlAllocParserInputBuffer(enc);
			if(!buf) 
				return -1;
			inputStream = xmlNewInputStream(reader->ctxt);
			if(!inputStream) {
				xmlFreeParserInputBuffer(buf);
				return -1;
			}
			inputStream->filename = URL ? (char *)xmlCanonicPath((const xmlChar *)URL) : 0;
			inputStream->buf = buf;
			xmlBufResetInput(buf->buffer, inputStream);
			inputPush(reader->ctxt, inputStream);
			reader->cur = 0;
		}
		if(!reader->ctxt) {
			xmlGenericError(0, "xmlTextReaderSetup : malloc failed\n");
			return -1;
		}
	}
	if(reader->dict) {
		if(reader->ctxt->dict) {
			if(reader->dict != reader->ctxt->dict) {
				xmlDictFree(reader->dict);
				reader->dict = reader->ctxt->dict;
			}
		}
		else {
			reader->ctxt->dict = reader->dict;
		}
	}
	else {
		SETIFZ(reader->ctxt->dict, xmlDictCreate());
		reader->dict = reader->ctxt->dict;
	}
	reader->ctxt->_private = reader;
	reader->ctxt->linenumbers = 1;
	reader->ctxt->dictNames = 1;
	/*
	 * use the parser dictionnary to allocate all elements and attributes names
	 */
	reader->ctxt->docdict = 1;
	reader->ctxt->parseMode = XML_PARSE_READER;
#ifdef LIBXML_XINCLUDE_ENABLED
	xmlXIncludeFreeContext(reader->xincctxt);
	reader->xincctxt = NULL;
	if(options & XML_PARSE_XINCLUDE) {
		reader->xinclude = 1;
		reader->xinclude_name = xmlDictLookupSL(reader->dict, XINCLUDE_NODE);
		options -= XML_PARSE_XINCLUDE;
	}
	else
		reader->xinclude = 0;
	reader->in_xinclude = 0;
#endif
#ifdef LIBXML_PATTERN_ENABLED
	if(reader->patternTab == NULL) {
		reader->patternNr = 0;
		reader->patternMax = 0;
	}
	while(reader->patternNr > 0) {
		reader->patternNr--;
		if(reader->patternTab[reader->patternNr] != NULL) {
			xmlFreePattern(reader->patternTab[reader->patternNr]);
			reader->patternTab[reader->patternNr] = NULL;
		}
	}
#endif
	if(options & XML_PARSE_DTDVALID)
		reader->validate = XML_TEXTREADER_VALIDATE_DTD;
	xmlCtxtUseOptions(reader->ctxt, options);
	if(encoding) {
		xmlCharEncodingHandler * hdlr = xmlFindCharEncodingHandler(encoding);
		if(hdlr)
			xmlSwitchToEncoding(reader->ctxt, hdlr);
	}
	if(URL && reader->ctxt->input && (reader->ctxt->input->filename == NULL))
		reader->ctxt->input->filename = sstrdup(URL);
	reader->doc = NULL;
	return 0;
}

/**
 * xmlTextReaderByteConsumed:
 * @reader: an XML reader
 *
 * This function provides the current index of the parser used
 * by the reader, relative to the start of the current entity.
 * This function actually just wraps a call to xmlBytesConsumed()
 * for the parser context associated with the reader.
 * See xmlBytesConsumed() for more information.
 *
 * Returns the index in bytes from the beginning of the entity or -1
 *    in case the index could not be computed.
 */
long xmlTextReaderByteConsumed(xmlTextReader * reader)
{
	return (reader && reader->ctxt) ? xmlByteConsumed(reader->ctxt) : -1;
}
/**
 * xmlReaderWalker:
 * @doc:  a preparsed document
 *
 * Create an xmltextReader for a preparsed document.
 *
 * Returns the new reader or NULL in case of error.
 */
xmlTextReader * xmlReaderWalker(xmlDoc * doc)
{
	xmlTextReader * ret;
	if(!doc)
		return 0;
	ret = (xmlTextReader *)SAlloc::M(sizeof(xmlTextReader));
	if(!ret) {
		xmlGenericError(0, "xmlNewTextReader : malloc failed\n");
		return 0;
	}
	memzero(ret, sizeof(xmlTextReader));
	ret->entNr = 0;
	ret->input = NULL;
	ret->mode = XML_TEXTREADER_MODE_INITIAL;
	ret->P_Node = NULL;
	ret->curnode = NULL;
	ret->base = 0;
	ret->cur = 0;
	ret->allocs = XML_TEXTREADER_CTXT;
	ret->doc = doc;
	ret->state = XML_TEXTREADER_START;
	ret->dict = xmlDictCreate();
	return ret;
}
/**
 * xmlReaderForDoc:
 * @cur:  a pointer to a zero terminated string
 * @URL:  the base URL to use for the document
 * @encoding:  the document encoding, or NULL
 * @options:  a combination of xmlParserOption
 *
 * Create an xmltextReader for an XML in-memory document.
 * The parsing flags @options are a combination of xmlParserOption.
 *
 * Returns the new reader or NULL in case of error.
 */
xmlTextReader * xmlReaderForDoc(const xmlChar * cur, const char * URL, const char * encoding, int options)
{
	if(!cur)
		return 0;
	int len = sstrlen(cur);
	return xmlReaderForMemory((const char *)cur, len, URL, encoding, options);
}
/**
 * xmlReaderForFile:
 * @filename:  a file or URL
 * @encoding:  the document encoding, or NULL
 * @options:  a combination of xmlParserOption
 *
 * parse an XML file from the filesystem or the network.
 * The parsing flags @options are a combination of xmlParserOption.
 *
 * Returns the new reader or NULL in case of error.
 */
xmlTextReader * xmlReaderForFile(const char * filename, const char * encoding, int options)
{
	xmlTextReader * reader = xmlNewTextReaderFilename(filename);
	if(!reader)
		return 0;
	xmlTextReaderSetup(reader, NULL, NULL, encoding, options);
	return reader;
}
/**
 * xmlReaderForMemory:
 * @buffer:  a pointer to a char array
 * @size:  the size of the array
 * @URL:  the base URL to use for the document
 * @encoding:  the document encoding, or NULL
 * @options:  a combination of xmlParserOption
 *
 * Create an xmltextReader for an XML in-memory document.
 * The parsing flags @options are a combination of xmlParserOption.
 *
 * Returns the new reader or NULL in case of error.
 */
xmlTextReader * xmlReaderForMemory(const char * buffer, int size, const char * URL, const char * encoding, int options)
{
	xmlTextReader * reader = 0;
	xmlParserInputBuffer * buf = xmlParserInputBufferCreateStatic(buffer, size, XML_CHAR_ENCODING_NONE);
	if(buf) {
		reader = xmlNewTextReader(buf, URL);
		if(!reader) {
			xmlFreeParserInputBuffer(buf);
		}
		else {
			reader->allocs |= XML_TEXTREADER_INPUT;
			xmlTextReaderSetup(reader, NULL, URL, encoding, options);
		}
	}
	return (reader);
}
/**
 * xmlReaderForFd:
 * @fd:  an open file descriptor
 * @URL:  the base URL to use for the document
 * @encoding:  the document encoding, or NULL
 * @options:  a combination of xmlParserOption
 *
 * Create an xmltextReader for an XML from a file descriptor.
 * The parsing flags @options are a combination of xmlParserOption.
 * NOTE that the file descriptor will not be closed when the
 * reader is closed or reset.
 *
 * Returns the new reader or NULL in case of error.
 */
xmlTextReader * xmlReaderForFd(int fd, const char * URL, const char * encoding, int options)
{
	xmlTextReader * reader = 0;
	if(fd >= 0) {
		xmlParserInputBuffer * input = xmlParserInputBufferCreateFd(fd, XML_CHAR_ENCODING_NONE);
		if(input) {
			input->closecallback = NULL;
			reader = xmlNewTextReader(input, URL);
			if(!reader)
				xmlFreeParserInputBuffer(input);
			else {
				reader->allocs |= XML_TEXTREADER_INPUT;
				xmlTextReaderSetup(reader, NULL, URL, encoding, options);
			}
		}
	}
	return reader;
}
/**
 * xmlReaderForIO:
 * @ioread:  an I/O read function
 * @ioclose:  an I/O close function
 * @ioctx:  an I/O handler
 * @URL:  the base URL to use for the document
 * @encoding:  the document encoding, or NULL
 * @options:  a combination of xmlParserOption
 *
 * Create an xmltextReader for an XML document from I/O functions and source.
 * The parsing flags @options are a combination of xmlParserOption.
 *
 * Returns the new reader or NULL in case of error.
 */
xmlTextReader * xmlReaderForIO(xmlInputReadCallback ioread, xmlInputCloseCallback ioclose, void * ioctx, const char * URL, const char * encoding, int options)
{
	xmlTextReader * reader = 0;
	if(ioread) {
		xmlParserInputBuffer * input = xmlParserInputBufferCreateIO(ioread, ioclose, ioctx, XML_CHAR_ENCODING_NONE);
		if(!input) {
			if(ioclose)
				ioclose(ioctx);
		}
		else {
			reader = xmlNewTextReader(input, URL);
			if(!reader) {
				xmlFreeParserInputBuffer(input);
			}
			else {
				reader->allocs |= XML_TEXTREADER_INPUT;
				xmlTextReaderSetup(reader, NULL, URL, encoding, options);
			}
		}
	}
	return reader;
}
/**
 * xmlReaderNewWalker:
 * @reader:  an XML reader
 * @doc:  a preparsed document
 *
 * Setup an xmltextReader to parse a preparsed XML document.
 * This reuses the existing @reader xmlTextReader.
 *
 * Returns 0 in case of success and -1 in case of error
 */
int xmlReaderNewWalker(xmlTextReader * reader, xmlDoc * doc)
{
	if(!doc || !reader)
		return -1;
	else {
		xmlFreeParserInputBuffer(reader->input);
		xmlCtxtReset(reader->ctxt);
		reader->entNr = 0;
		reader->input = NULL;
		reader->mode = XML_TEXTREADER_MODE_INITIAL;
		reader->P_Node = NULL;
		reader->curnode = NULL;
		reader->base = 0;
		reader->cur = 0;
		reader->allocs = XML_TEXTREADER_CTXT;
		reader->doc = doc;
		reader->state = XML_TEXTREADER_START;
		SETIFZ(reader->dict, (reader->ctxt && reader->ctxt->dict) ? reader->ctxt->dict : xmlDictCreate());
		return 0;
	}
}
/**
 * xmlReaderNewDoc:
 * @reader:  an XML reader
 * @cur:  a pointer to a zero terminated string
 * @URL:  the base URL to use for the document
 * @encoding:  the document encoding, or NULL
 * @options:  a combination of xmlParserOption
 *
 * Setup an xmltextReader to parse an XML in-memory document.
 * The parsing flags @options are a combination of xmlParserOption.
 * This reuses the existing @reader xmlTextReader.
 *
 * Returns 0 in case of success and -1 in case of error
 */
int xmlReaderNewDoc(xmlTextReader * reader, const xmlChar * cur, const char * URL, const char * encoding, int options)
{
	if(!cur || !reader)
		return -1;
	int len = sstrlen(cur);
	return xmlReaderNewMemory(reader, (const char *)cur, len, URL, encoding, options);
}
/**
 * xmlReaderNewFile:
 * @reader:  an XML reader
 * @filename:  a file or URL
 * @encoding:  the document encoding, or NULL
 * @options:  a combination of xmlParserOption
 *
 * parse an XML file from the filesystem or the network.
 * The parsing flags @options are a combination of xmlParserOption.
 * This reuses the existing @reader xmlTextReader.
 *
 * Returns 0 in case of success and -1 in case of error
 */
int xmlReaderNewFile(xmlTextReader * reader, const char * filename, const char * encoding, int options)
{
	xmlParserInputBuffer * input = (filename && reader) ? xmlParserInputBufferCreateFilename(filename, XML_CHAR_ENCODING_NONE) : 0;
	return input ? xmlTextReaderSetup(reader, input, filename, encoding, options) : -1;
}
/**
 * xmlReaderNewMemory:
 * @reader:  an XML reader
 * @buffer:  a pointer to a char array
 * @size:  the size of the array
 * @URL:  the base URL to use for the document
 * @encoding:  the document encoding, or NULL
 * @options:  a combination of xmlParserOption
 *
 * Setup an xmltextReader to parse an XML in-memory document.
 * The parsing flags @options are a combination of xmlParserOption.
 * This reuses the existing @reader xmlTextReader.
 *
 * Returns 0 in case of success and -1 in case of error
 */
int xmlReaderNewMemory(xmlTextReader * reader, const char * buffer, int size, const char * URL, const char * encoding, int options)
{
	xmlParserInputBuffer * input = (reader && buffer) ? xmlParserInputBufferCreateStatic(buffer, size, XML_CHAR_ENCODING_NONE) : 0;
	return input ? xmlTextReaderSetup(reader, input, URL, encoding, options) : -1;
}
/**
 * xmlReaderNewFd:
 * @reader:  an XML reader
 * @fd:  an open file descriptor
 * @URL:  the base URL to use for the document
 * @encoding:  the document encoding, or NULL
 * @options:  a combination of xmlParserOption
 *
 * Setup an xmltextReader to parse an XML from a file descriptor.
 * NOTE that the file descriptor will not be closed when the
 * reader is closed or reset.
 * The parsing flags @options are a combination of xmlParserOption.
 * This reuses the existing @reader xmlTextReader.
 *
 * Returns 0 in case of success and -1 in case of error
 */
int xmlReaderNewFd(xmlTextReader * reader, int fd, const char * URL, const char * encoding, int options)
{
	xmlParserInputBuffer * input;
	if(fd < 0)
		return -1;
	if(!reader)
		return -1;
	input = xmlParserInputBufferCreateFd(fd, XML_CHAR_ENCODING_NONE);
	if(!input)
		return -1;
	input->closecallback = NULL;
	return xmlTextReaderSetup(reader, input, URL, encoding, options);
}
/**
 * xmlReaderNewIO:
 * @reader:  an XML reader
 * @ioread:  an I/O read function
 * @ioclose:  an I/O close function
 * @ioctx:  an I/O handler
 * @URL:  the base URL to use for the document
 * @encoding:  the document encoding, or NULL
 * @options:  a combination of xmlParserOption
 *
 * Setup an xmltextReader to parse an XML document from I/O functions
 * and source.
 * The parsing flags @options are a combination of xmlParserOption.
 * This reuses the existing @reader xmlTextReader.
 *
 * Returns 0 in case of success and -1 in case of error
 */
int xmlReaderNewIO(xmlTextReader * reader, xmlInputReadCallback ioread,
    xmlInputCloseCallback ioclose, void * ioctx, const char * URL, const char * encoding, int options)
{
	xmlParserInputBuffer * input;
	if(ioread == NULL)
		return -1;
	if(!reader)
		return -1;
	input = xmlParserInputBufferCreateIO(ioread, ioclose, ioctx, XML_CHAR_ENCODING_NONE);
	if(!input) {
		if(ioclose)
			ioclose(ioctx);
		return -1;
	}
	return (xmlTextReaderSetup(reader, input, URL, encoding, options));
}
// 
// Utilities
// 
#ifdef NOT_USED_YET

/**
 * xmlBase64Decode:
 * @in:  the input buffer
 * @inlen:  the size of the input (in), the size read from it (out)
 * @to:  the output buffer
 * @tolen:  the size of the output (in), the size written to (out)
 *
 * Base64 decoder, reads from @in and save in @to
 * @todo tell jody when this is actually exported
 *
 * Returns 0 if all the input was consumer, 1 if the Base64 end was reached,
 *    2 if there wasn't enough space on the output or -1 in case of error.
 */
static int xmlBase64Decode(const uchar * in, unsigned long * inlen, uchar * to, unsigned long * tolen)
{
	ulong  incur;    /* current index in in[] */
	ulong  inblk;    /* last block index in in[] */
	ulong  outcur;   /* current index in out[] */
	ulong  inmax;    /* size of in[] */
	ulong  outmax;   /* size of out[] */
	uchar  cur;      /* the current value read from in[] */
	uchar  intmp[4], outtmp[4]; /* temporary buffers for the convert */
	int    nbintmp;            /* number of byte in intmp[] */
	int    is_ignore;          /* cur should be ignored */
	int    is_end = 0;         /* the end of the base64 was found */
	int    retval = 1;
	int    i;
	if((in == NULL) || (inlen == NULL) || (to == NULL) || (tolen == NULL))
		return -1;
	incur = 0;
	inblk = 0;
	outcur = 0;
	inmax = *inlen;
	outmax = *tolen;
	nbintmp = 0;

	while(1) {
		if(incur >= inmax)
			break;
		cur = in[incur++];
		is_ignore = 0;
		if((cur >= 'A') && (cur <= 'Z'))
			cur = cur - 'A';
		else if((cur >= 'a') && (cur <= 'z'))
			cur = cur - 'a' + 26;
		else if((cur >= '0') && (cur <= '9'))
			cur = cur - '0' + 52;
		else if(cur == '+')
			cur = 62;
		else if(cur == '/')
			cur = 63;
		else if(cur == '.')
			cur = 0;
		else if(cur == '=') /*no op , end of the base64 stream */
			is_end = 1;
		else {
			is_ignore = 1;
			if(nbintmp == 0)
				inblk = incur;
		}
		if(!is_ignore) {
			int nbouttmp = 3;
			int is_break = 0;
			if(is_end) {
				if(nbintmp == 0)
					break;
				if((nbintmp == 1) || (nbintmp == 2))
					nbouttmp = 1;
				else
					nbouttmp = 2;
				nbintmp = 3;
				is_break = 1;
			}
			intmp[nbintmp++] = cur;
			/*
			 * if intmp is full, push the 4byte sequence as a 3 byte
			 * sequence out
			 */
			if(nbintmp == 4) {
				nbintmp = 0;
				outtmp[0] = (intmp[0] << 2) | ((intmp[1] & 0x30) >> 4);
				outtmp[1] =
				    ((intmp[1] & 0x0F) << 4) | ((intmp[2] & 0x3C) >> 2);
				outtmp[2] = ((intmp[2] & 0x03) << 6) | (intmp[3] & 0x3F);
				if(outcur + 3 >= outmax) {
					retval = 2;
					break;
				}
				for(i = 0; i < nbouttmp; i++)
					to[outcur++] = outtmp[i];
				inblk = incur;
			}
			if(is_break) {
				retval = 0;
				break;
			}
		}
	}
	*tolen = outcur;
	*inlen = inblk;
	return (retval);
}

/*
 * Test routine for the xmlBase64Decode function
 */
#if 0
int main(int argc, char ** argv)
{
	char * input = "  VW4 gcGV0        \n      aXQgdGVzdCAuCg== ";
	char output[100];
	char output2[100];
	char output3[100];
	unsigned long inlen = strlen(input);
	unsigned long outlen = 100;
	int ret;
	unsigned long cons, tmp, tmp2, prod;
	/*
	 * Direct
	 */
	ret = xmlBase64Decode(input, &inlen, output, &outlen);
	output[outlen] = 0;
	printf("ret: %d, inlen: %ld , outlen: %ld, output: '%s'\n", ret, inlen,
	    outlen, output) indent : Standard input : 179 : Error : Unmatched # endif
	;

	/*
	 * output chunking
	 */
	cons = 0;
	prod = 0;
	while(cons < inlen) {
		tmp = 5;
		tmp2 = inlen - cons;
		printf("%ld %ld\n", cons, prod);
		ret = xmlBase64Decode(&input[cons], &tmp2, &output2[prod], &tmp);
		cons += tmp2;
		prod += tmp;
		printf("%ld %ld\n", cons, prod);
	}
	output2[outlen] = 0;
	printf("ret: %d, cons: %ld , prod: %ld, output: '%s'\n", ret, cons, prod, output2);

	/*
	 * input chunking
	 */
	cons = 0;
	prod = 0;
	while(cons < inlen) {
		tmp = 100 - prod;
		tmp2 = inlen - cons;
		if(tmp2 > 5)
			tmp2 = 5;
		printf("%ld %ld\n", cons, prod);
		ret = xmlBase64Decode(&input[cons], &tmp2, &output3[prod], &tmp);
		cons += tmp2;
		prod += tmp;
		printf("%ld %ld\n", cons, prod);
	}
	output3[outlen] = 0;
	printf("ret: %d, cons: %ld , prod: %ld, output: '%s'\n", ret, cons,
	    prod, output3);
	return 0;
}

#endif
#endif /* NOT_USED_YET */
#define bottom_xmlreader
//#include "elfgcchack.h"
#endif /* LIBXML_READER_ENABLED */
