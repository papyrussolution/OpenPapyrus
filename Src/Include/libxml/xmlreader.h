/*
 * Summary: the XMLReader implementation
 * Description: API of the XML streaming API based on C# interfaces.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_XMLREADER_H__
#define __XML_XMLREADER_H__

#include <libxml/xmlversion.h>
#include <libxml/tree.h>
//#include <libxml/xmlIO.h>
#ifdef LIBXML_SCHEMAS_ENABLED
#include <libxml/relaxng.h>
#include <libxml/xmlschemas.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * xmlParserSeverities:
 *
 * How severe an error callback is when the per-reader error callback API
 * is used.
 */
typedef enum {
	XML_PARSER_SEVERITY_VALIDITY_WARNING = 1,
	XML_PARSER_SEVERITY_VALIDITY_ERROR = 2,
	XML_PARSER_SEVERITY_WARNING = 3,
	XML_PARSER_SEVERITY_ERROR = 4
} xmlParserSeverities;

#ifdef LIBXML_READER_ENABLED

/**
 * xmlTextReaderMode:
 *
 * Internal state values for the reader.
 */
typedef enum {
	XML_TEXTREADER_MODE_INITIAL = 0,
	XML_TEXTREADER_MODE_INTERACTIVE = 1,
	XML_TEXTREADER_MODE_ERROR = 2,
	XML_TEXTREADER_MODE_EOF = 3,
	XML_TEXTREADER_MODE_CLOSED = 4,
	XML_TEXTREADER_MODE_READING = 5
} xmlTextReaderMode;

/**
 * xmlParserProperties:
 *
 * Some common options to use with xmlTextReaderSetParserProp, but it
 * is better to use xmlParserOption and the xmlReaderNewxxx and
 * xmlReaderForxxx APIs now.
 */
typedef enum {
	XML_PARSER_LOADDTD = 1,
	XML_PARSER_DEFAULTATTRS = 2,
	XML_PARSER_VALIDATE = 3,
	XML_PARSER_SUBST_ENTITIES = 4
} xmlParserProperties;

/**
 * xmlReaderTypes:
 *
 * Predefined constants for the different types of nodes.
 */
typedef enum {
	XML_READER_TYPE_NONE = 0,
	XML_READER_TYPE_ELEMENT = 1,
	XML_READER_TYPE_ATTRIBUTE = 2,
	XML_READER_TYPE_TEXT = 3,
	XML_READER_TYPE_CDATA = 4,
	XML_READER_TYPE_ENTITY_REFERENCE = 5,
	XML_READER_TYPE_ENTITY = 6,
	XML_READER_TYPE_PROCESSING_INSTRUCTION = 7,
	XML_READER_TYPE_COMMENT = 8,
	XML_READER_TYPE_DOCUMENT = 9,
	XML_READER_TYPE_DOCUMENT_TYPE = 10,
	XML_READER_TYPE_DOCUMENT_FRAGMENT = 11,
	XML_READER_TYPE_NOTATION = 12,
	XML_READER_TYPE_WHITESPACE = 13,
	XML_READER_TYPE_SIGNIFICANT_WHITESPACE = 14,
	XML_READER_TYPE_END_ELEMENT = 15,
	XML_READER_TYPE_END_ENTITY = 16,
	XML_READER_TYPE_XML_DECLARATION = 17
} xmlReaderTypes;

/**
 * xmlTextReader:
 *
 * Structure for an xmlReader context.
 */
//typedef struct _xmlTextReader xmlTextReader;
struct xmlTextReader;
/**
 * xmlTextReaderPtr:
 *
 * Pointer to an xmlReader context.
 */
typedef xmlTextReader * xmlTextReaderPtr;
/*
 * Constructors & Destructor
 */
XMLPUBFUN xmlTextReaderPtr XMLCALL xmlNewTextReader(xmlParserInputBufferPtr input, const char * URI);
XMLPUBFUN xmlTextReaderPtr XMLCALL xmlNewTextReaderFilename(const char * URI);
XMLPUBFUN void XMLCALL  xmlFreeTextReader(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderSetup(xmlTextReader * reader, xmlParserInputBufferPtr input, const char * URL, const char * encoding, int options);

/*
 * Iterators
 */
XMLPUBFUN int XMLCALL xmlTextReaderRead(xmlTextReader * reader);

#ifdef LIBXML_WRITER_ENABLED
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderReadInnerXml(xmlTextReader * reader);
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderReadOuterXml(xmlTextReader * reader);
#endif

XMLPUBFUN xmlChar * XMLCALL xmlTextReaderReadString(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderReadAttributeValue(xmlTextReader * reader);

/*
 * Attributes of the node
 */
XMLPUBFUN int XMLCALL xmlTextReaderAttributeCount(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderDepth(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderHasAttributes(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderHasValue(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderIsDefault(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderIsEmptyElement(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderNodeType(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderQuoteChar(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderReadState(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderIsNamespaceDecl(xmlTextReader * reader);

XMLPUBFUN const xmlChar * XMLCALL xmlTextReaderConstBaseUri(xmlTextReader * reader);
XMLPUBFUN const xmlChar * XMLCALL xmlTextReaderConstLocalName(xmlTextReader * reader);
XMLPUBFUN const xmlChar * XMLCALL xmlTextReaderConstName(xmlTextReader * reader);
XMLPUBFUN const xmlChar * XMLCALL xmlTextReaderConstNamespaceUri(xmlTextReader * reader);
XMLPUBFUN const xmlChar * XMLCALL xmlTextReaderConstPrefix(xmlTextReader * reader);
XMLPUBFUN const xmlChar * XMLCALL xmlTextReaderConstXmlLang(xmlTextReader * reader);
XMLPUBFUN const xmlChar * XMLCALL xmlTextReaderConstString(xmlTextReader * reader, const xmlChar * str);
XMLPUBFUN const xmlChar * XMLCALL xmlTextReaderConstValue(xmlTextReader * reader);

/*
 * use the Const version of the routine for
 * better performance and simpler code
 */
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderBaseUri(xmlTextReader * reader);
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderLocalName(xmlTextReader * reader);
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderName(xmlTextReader * reader);
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderNamespaceUri(xmlTextReader * reader);
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderPrefix(xmlTextReader * reader);
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderXmlLang(xmlTextReader * reader);
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderValue(xmlTextReader * reader);
/*
 * Methods of the XmlTextReader
 */
XMLPUBFUN int XMLCALL xmlTextReaderClose(xmlTextReader * reader);
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderGetAttributeNo(xmlTextReader * reader, int no);
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderGetAttribute(xmlTextReader * reader, const xmlChar * name);
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderGetAttributeNs(xmlTextReader * reader, const xmlChar * localName, const xmlChar * namespaceURI);
XMLPUBFUN xmlParserInputBufferPtr XMLCALL xmlTextReaderGetRemainder(xmlTextReader * reader);
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderLookupNamespace(xmlTextReader * reader, const xmlChar * prefix);
XMLPUBFUN int XMLCALL xmlTextReaderMoveToAttributeNo(xmlTextReader * reader, int no);
XMLPUBFUN int XMLCALL xmlTextReaderMoveToAttribute(xmlTextReader * reader, const xmlChar * name);
XMLPUBFUN int XMLCALL xmlTextReaderMoveToAttributeNs(xmlTextReader * reader, const xmlChar * localName, const xmlChar * namespaceURI);
XMLPUBFUN int XMLCALL xmlTextReaderMoveToFirstAttribute(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderMoveToNextAttribute(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderMoveToElement(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderNormalization(xmlTextReader * reader);
XMLPUBFUN const xmlChar * XMLCALL xmlTextReaderConstEncoding(xmlTextReader * reader);

/*
 * Extensions
 */
XMLPUBFUN int XMLCALL xmlTextReaderSetParserProp(xmlTextReader * reader, int prop, int value);
XMLPUBFUN int XMLCALL xmlTextReaderGetParserProp(xmlTextReader * reader, int prop);
XMLPUBFUN xmlNode * XMLCALL xmlTextReaderCurrentNode(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderGetParserLineNumber(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderGetParserColumnNumber(xmlTextReader * reader);
XMLPUBFUN xmlNode * XMLCALL xmlTextReaderPreserve(xmlTextReader * reader);
#ifdef LIBXML_PATTERN_ENABLED
XMLPUBFUN int XMLCALL xmlTextReaderPreservePattern(xmlTextReader * reader, const xmlChar * pattern, const xmlChar ** namespaces);
#endif /* LIBXML_PATTERN_ENABLED */
XMLPUBFUN xmlDocPtr XMLCALL xmlTextReaderCurrentDoc(xmlTextReader * reader);
XMLPUBFUN xmlNode * XMLCALL xmlTextReaderExpand(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderNext(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderNextSibling(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderIsValid(xmlTextReader * reader);
#ifdef LIBXML_SCHEMAS_ENABLED
XMLPUBFUN int XMLCALL xmlTextReaderRelaxNGValidate(xmlTextReader * reader, const char * rng);
XMLPUBFUN int XMLCALL xmlTextReaderRelaxNGValidateCtxt(xmlTextReader * reader, xmlRelaxNGValidCtxtPtr ctxt, int options);
XMLPUBFUN int XMLCALL xmlTextReaderRelaxNGSetSchema(xmlTextReader * reader, xmlRelaxNGPtr schema);
XMLPUBFUN int XMLCALL xmlTextReaderSchemaValidate(xmlTextReader * reader, const char * xsd);
XMLPUBFUN int XMLCALL xmlTextReaderSchemaValidateCtxt(xmlTextReader * reader, xmlSchemaValidCtxtPtr ctxt, int options);
XMLPUBFUN int XMLCALL xmlTextReaderSetSchema(xmlTextReader * reader, xmlSchemaPtr schema);
#endif
XMLPUBFUN const xmlChar * XMLCALL xmlTextReaderConstXmlVersion(xmlTextReader * reader);
XMLPUBFUN int XMLCALL xmlTextReaderStandalone(xmlTextReader * reader);

/*
 * Index lookup
 */
XMLPUBFUN long XMLCALL xmlTextReaderByteConsumed(xmlTextReader * reader);

/*
 * New more complete APIs for simpler creation and reuse of readers
 */
XMLPUBFUN xmlTextReaderPtr XMLCALL xmlReaderWalker(xmlDocPtr doc);
XMLPUBFUN xmlTextReaderPtr XMLCALL xmlReaderForDoc(const xmlChar * cur, const char * URL, const char * encoding, int options);
XMLPUBFUN xmlTextReaderPtr XMLCALL xmlReaderForFile(const char * filename, const char * encoding, int options);
XMLPUBFUN xmlTextReaderPtr XMLCALL xmlReaderForMemory(const char * buffer, int size, const char * URL, const char * encoding, int options);
XMLPUBFUN xmlTextReaderPtr XMLCALL xmlReaderForFd(int fd, const char * URL, const char * encoding, int options);
XMLPUBFUN xmlTextReaderPtr XMLCALL xmlReaderForIO(xmlInputReadCallback ioread, xmlInputCloseCallback ioclose,
    void * ioctx, const char * URL, const char * encoding, int options);
XMLPUBFUN int XMLCALL xmlReaderNewWalker(xmlTextReader * reader, xmlDocPtr doc);
XMLPUBFUN int XMLCALL xmlReaderNewDoc(xmlTextReader * reader, const xmlChar * cur, const char * URL, const char * encoding, int options);
XMLPUBFUN int XMLCALL xmlReaderNewFile(xmlTextReader * reader, const char * filename, const char * encoding, int options);
XMLPUBFUN int XMLCALL xmlReaderNewMemory(xmlTextReader * reader, const char * buffer, int size, const char * URL,
    const char * encoding, int options);
XMLPUBFUN int XMLCALL xmlReaderNewFd(xmlTextReader * reader, int fd, const char * URL, const char * encoding, int options);
XMLPUBFUN int XMLCALL xmlReaderNewIO(xmlTextReader * reader, xmlInputReadCallback ioread, xmlInputCloseCallback ioclose, void * ioctx,
    const char * URL, const char * encoding, int options);
/*
 * Error handling extensions
 */
typedef void *  xmlTextReaderLocatorPtr;

/**
 * xmlTextReaderErrorFunc:
 * @arg: the user argument
 * @msg: the message
 * @severity: the severity of the error
 * @locator: a locator indicating where the error occured
 *
 * Signature of an error callback from a reader parser
 */
typedef void (XMLCALL *xmlTextReaderErrorFunc)(void * arg, const char * msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator);
XMLPUBFUN int XMLCALL xmlTextReaderLocatorLineNumber(xmlTextReaderLocatorPtr locator);
XMLPUBFUN xmlChar * XMLCALL xmlTextReaderLocatorBaseURI(xmlTextReaderLocatorPtr locator);
XMLPUBFUN void XMLCALL xmlTextReaderSetErrorHandler(xmlTextReader * reader, xmlTextReaderErrorFunc f, void * arg);
XMLPUBFUN void XMLCALL xmlTextReaderSetStructuredErrorHandler(xmlTextReader * reader, xmlStructuredErrorFunc f, void * arg);
XMLPUBFUN void XMLCALL xmlTextReaderGetErrorHandler(xmlTextReader * reader, xmlTextReaderErrorFunc * f, void ** arg);

#endif /* LIBXML_READER_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* __XML_XMLREADER_H__ */

