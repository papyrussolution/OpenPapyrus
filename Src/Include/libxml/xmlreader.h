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
#include <libxml/globals.h>
#include <libxml/xmlIO.h>
#ifdef LIBXML_SCHEMAS_ENABLED
	#include <libxml/relaxng.h>
	#include <libxml/xmlschemas.h>
#endif

//#ifdef __cplusplus
//extern "C" {
//#endif
//
// Descr: How severe an error callback is when the per-reader error callback API is used.
//
enum xmlParserSeverities {
	XML_PARSER_SEVERITY_VALIDITY_WARNING = 1,
	XML_PARSER_SEVERITY_VALIDITY_ERROR = 2,
	XML_PARSER_SEVERITY_WARNING = 3,
	XML_PARSER_SEVERITY_ERROR = 4
};

#ifdef LIBXML_READER_ENABLED
//
// Descr: Internal state values for the reader.
//
enum xmlTextReaderMode {
	XML_TEXTREADER_MODE_INITIAL = 0,
	XML_TEXTREADER_MODE_INTERACTIVE = 1,
	XML_TEXTREADER_MODE_ERROR = 2,
	XML_TEXTREADER_MODE_EOF = 3,
	XML_TEXTREADER_MODE_CLOSED = 4,
	XML_TEXTREADER_MODE_READING = 5
};
//
// Descr: Some common options to use with xmlTextReaderSetParserProp, but it
//   is better to use xmlParserOption and the xmlReaderNewxxx and xmlReaderForxxx APIs now.
//
enum xmlParserProperties {
	XML_PARSER_LOADDTD = 1,
	XML_PARSER_DEFAULTATTRS = 2,
	XML_PARSER_VALIDATE = 3,
	XML_PARSER_SUBST_ENTITIES = 4
};
//
// Descr: Predefined constants for the different types of nodes.
//
enum xmlReaderTypes {
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
};
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
//typedef xmlTextReader * xmlTextReaderPtr;
XMLPUBFUN xmlTextReader * xmlNewTextReader(xmlParserInputBuffer * input, const char * URI);
XMLPUBFUN xmlTextReader * xmlNewTextReaderFilename(const char * URI);
XMLPUBFUN void  xmlFreeTextReader(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderSetup(xmlTextReader * reader, xmlParserInputBuffer * input, const char * URL, const char * encoding, int options);
// 
// Iterators
// 
XMLPUBFUN int FASTCALL xmlTextReaderRead(xmlTextReader * reader);
#ifdef LIBXML_WRITER_ENABLED
	XMLPUBFUN xmlChar * xmlTextReaderReadInnerXml(xmlTextReader * reader);
	XMLPUBFUN xmlChar * xmlTextReaderReadOuterXml(xmlTextReader * reader);
#endif
XMLPUBFUN xmlChar * xmlTextReaderReadString(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderReadAttributeValue(xmlTextReader * reader);
// 
// Attributes of the node
// 
XMLPUBFUN int xmlTextReaderAttributeCount(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderDepth(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderHasAttributes(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderHasValue(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderIsDefault(const xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderIsEmptyElement(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderNodeType(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderQuoteChar(const xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderReadState(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderIsNamespaceDecl(xmlTextReader * reader);

XMLPUBFUN const xmlChar * xmlTextReaderConstBaseUri(xmlTextReader * reader);
XMLPUBFUN const xmlChar * xmlTextReaderConstLocalName(xmlTextReader * reader);
XMLPUBFUN const xmlChar * xmlTextReaderConstName(xmlTextReader * reader);
XMLPUBFUN const xmlChar * xmlTextReaderConstNamespaceUri(xmlTextReader * reader);
XMLPUBFUN const xmlChar * xmlTextReaderConstPrefix(xmlTextReader * reader);
XMLPUBFUN const xmlChar * xmlTextReaderConstXmlLang(xmlTextReader * reader);
XMLPUBFUN const xmlChar * xmlTextReaderConstString(xmlTextReader * reader, const xmlChar * str);
XMLPUBFUN const xmlChar * xmlTextReaderConstValue(xmlTextReader * reader);
// 
// use the Const version of the routine for better performance and simpler code
// 
XMLPUBFUN xmlChar * xmlTextReaderBaseUri(xmlTextReader * reader);
XMLPUBFUN xmlChar * xmlTextReaderLocalName(xmlTextReader * reader);
XMLPUBFUN xmlChar * xmlTextReaderName(xmlTextReader * reader);
XMLPUBFUN xmlChar * xmlTextReaderNamespaceUri(xmlTextReader * reader);
XMLPUBFUN xmlChar * xmlTextReaderPrefix(xmlTextReader * reader);
XMLPUBFUN xmlChar * xmlTextReaderXmlLang(xmlTextReader * reader);
XMLPUBFUN xmlChar * xmlTextReaderValue(xmlTextReader * reader);
// 
// Methods of the XmlTextReader
// 
XMLPUBFUN int xmlTextReaderClose(xmlTextReader * reader);
XMLPUBFUN xmlChar * xmlTextReaderGetAttributeNo(xmlTextReader * reader, int no);
XMLPUBFUN xmlChar * xmlTextReaderGetAttribute(xmlTextReader * reader, const xmlChar * name);
XMLPUBFUN xmlChar * xmlTextReaderGetAttributeNs(xmlTextReader * reader, const xmlChar * localName, const xmlChar * namespaceURI);
XMLPUBFUN xmlParserInputBuffer * xmlTextReaderGetRemainder(xmlTextReader * reader);
XMLPUBFUN xmlChar * xmlTextReaderLookupNamespace(xmlTextReader * reader, const xmlChar * prefix);
XMLPUBFUN int xmlTextReaderMoveToAttributeNo(xmlTextReader * reader, int no);
XMLPUBFUN int xmlTextReaderMoveToAttribute(xmlTextReader * reader, const xmlChar * name);
XMLPUBFUN int xmlTextReaderMoveToAttributeNs(xmlTextReader * reader, const xmlChar * localName, const xmlChar * namespaceURI);
XMLPUBFUN int xmlTextReaderMoveToFirstAttribute(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderMoveToNextAttribute(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderMoveToElement(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderNormalization(xmlTextReader * reader);
XMLPUBFUN const xmlChar * xmlTextReaderConstEncoding(xmlTextReader * reader);
// 
// Extensions
// 
XMLPUBFUN int xmlTextReaderSetParserProp(xmlTextReader * reader, int prop, int value);
XMLPUBFUN int xmlTextReaderGetParserProp(xmlTextReader * reader, int prop);
XMLPUBFUN xmlNode * xmlTextReaderCurrentNode(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderGetParserLineNumber(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderGetParserColumnNumber(xmlTextReader * reader);
XMLPUBFUN xmlNode * xmlTextReaderPreserve(xmlTextReader * reader);
#ifdef LIBXML_PATTERN_ENABLED
	XMLPUBFUN int xmlTextReaderPreservePattern(xmlTextReader * reader, const xmlChar * pattern, const xmlChar ** namespaces);
#endif
XMLPUBFUN xmlDoc * xmlTextReaderCurrentDoc(xmlTextReader * reader);
XMLPUBFUN xmlNode * xmlTextReaderExpand(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderNext(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderNextSibling(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderIsValid(xmlTextReader * reader);
#ifdef LIBXML_SCHEMAS_ENABLED
	XMLPUBFUN int xmlTextReaderRelaxNGValidate(xmlTextReader * reader, const char * rng);
	XMLPUBFUN int xmlTextReaderRelaxNGValidateCtxt(xmlTextReader * reader, xmlRelaxNGValidCtxtPtr ctxt, int options);
	XMLPUBFUN int xmlTextReaderRelaxNGSetSchema(xmlTextReader * reader, xmlRelaxNGPtr schema);
	XMLPUBFUN int xmlTextReaderSchemaValidate(xmlTextReader * reader, const char * xsd);
	XMLPUBFUN int xmlTextReaderSchemaValidateCtxt(xmlTextReader * reader, xmlSchemaValidCtxt * ctxt, int options);
	XMLPUBFUN int xmlTextReaderSetSchema(xmlTextReader * reader, xmlSchemaPtr schema);
#endif
XMLPUBFUN const xmlChar * xmlTextReaderConstXmlVersion(xmlTextReader * reader);
XMLPUBFUN int xmlTextReaderStandalone(xmlTextReader * reader);
// 
// Index lookup
// 
XMLPUBFUN long xmlTextReaderByteConsumed(xmlTextReader * reader);
// 
// New more complete APIs for simpler creation and reuse of readers
// 
XMLPUBFUN xmlTextReader * xmlReaderWalker(xmlDoc * doc);
XMLPUBFUN xmlTextReader * xmlReaderForDoc(const xmlChar * cur, const char * URL, const char * encoding, int options);
XMLPUBFUN xmlTextReader * xmlReaderForFile(const char * filename, const char * encoding, int options);
XMLPUBFUN xmlTextReader * xmlReaderForMemory(const char * buffer, int size, const char * URL, const char * encoding, int options);
XMLPUBFUN xmlTextReader * xmlReaderForFd(int fd, const char * URL, const char * encoding, int options);
XMLPUBFUN xmlTextReader * xmlReaderForIO(xmlInputReadCallback ioread, xmlInputCloseCallback ioclose, void * ioctx, const char * URL, const char * encoding, int options);
XMLPUBFUN int xmlReaderNewWalker(xmlTextReader * reader, xmlDoc * doc);
XMLPUBFUN int xmlReaderNewDoc(xmlTextReader * reader, const xmlChar * cur, const char * URL, const char * encoding, int options);
XMLPUBFUN int xmlReaderNewFile(xmlTextReader * reader, const char * filename, const char * encoding, int options);
XMLPUBFUN int xmlReaderNewMemory(xmlTextReader * reader, const char * buffer, int size, const char * URL, const char * encoding, int options);
XMLPUBFUN int xmlReaderNewFd(xmlTextReader * reader, int fd, const char * URL, const char * encoding, int options);
XMLPUBFUN int xmlReaderNewIO(xmlTextReader * reader, xmlInputReadCallback ioread, xmlInputCloseCallback ioclose, void * ioctx, const char * URL, const char * encoding, int options);
// 
// Error handling extensions
// 
typedef void * xmlTextReaderLocatorPtr;
// 
// xmlTextReaderErrorFunc:
// @arg: the user argument
// @msg: the message
// @severity: the severity of the error
// @locator: a locator indicating where the error occured
// 
// Signature of an error callback from a reader parser
// 
typedef void (* xmlTextReaderErrorFunc)(void * arg, const char * msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator);
XMLPUBFUN int xmlTextReaderLocatorLineNumber(xmlTextReaderLocatorPtr locator);
XMLPUBFUN xmlChar * xmlTextReaderLocatorBaseURI(xmlTextReaderLocatorPtr locator);
XMLPUBFUN void xmlTextReaderSetErrorHandler(xmlTextReader * reader, xmlTextReaderErrorFunc f, void * arg);
XMLPUBFUN void xmlTextReaderSetStructuredErrorHandler(xmlTextReader * reader, xmlStructuredErrorFunc f, void * arg);
XMLPUBFUN void xmlTextReaderGetErrorHandler(xmlTextReader * reader, xmlTextReaderErrorFunc * f, void ** arg);

#endif /* LIBXML_READER_ENABLED */

//#ifdef __cplusplus
//}
//#endif

#endif /* __XML_XMLREADER_H__ */

