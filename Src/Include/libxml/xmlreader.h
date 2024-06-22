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
xmlTextReader * xmlNewTextReader(xmlParserInputBuffer * input, const char * URI);
xmlTextReader * xmlNewTextReaderFilename(const char * URI);
void  xmlFreeTextReader(xmlTextReader * reader);
int xmlTextReaderSetup(xmlTextReader * reader, xmlParserInputBuffer * input, const char * URL, const char * encoding, int options);
// 
// Iterators
// 
int FASTCALL xmlTextReaderRead(xmlTextReader * reader);
#ifdef LIBXML_WRITER_ENABLED
	xmlChar * xmlTextReaderReadInnerXml(xmlTextReader * reader);
	xmlChar * xmlTextReaderReadOuterXml(xmlTextReader * reader);
#endif
xmlChar * xmlTextReaderReadString(xmlTextReader * reader);
int xmlTextReaderReadAttributeValue(xmlTextReader * reader);
// 
// Attributes of the node
// 
int xmlTextReaderAttributeCount(xmlTextReader * reader);
int xmlTextReaderDepth(xmlTextReader * reader);
int xmlTextReaderHasAttributes(xmlTextReader * reader);
int xmlTextReaderHasValue(xmlTextReader * reader);
int xmlTextReaderIsDefault(const xmlTextReader * reader);
int xmlTextReaderIsEmptyElement(xmlTextReader * reader);
int xmlTextReaderNodeType(xmlTextReader * reader);
int xmlTextReaderQuoteChar(const xmlTextReader * reader);
int xmlTextReaderReadState(xmlTextReader * reader);
int xmlTextReaderIsNamespaceDecl(xmlTextReader * reader);

const xmlChar * xmlTextReaderConstBaseUri(xmlTextReader * reader);
const xmlChar * xmlTextReaderConstLocalName(xmlTextReader * reader);
const xmlChar * xmlTextReaderConstName(xmlTextReader * reader);
const xmlChar * xmlTextReaderConstNamespaceUri(xmlTextReader * reader);
const xmlChar * xmlTextReaderConstPrefix(xmlTextReader * reader);
const xmlChar * xmlTextReaderConstXmlLang(xmlTextReader * reader);
const xmlChar * xmlTextReaderConstString(xmlTextReader * reader, const xmlChar * str);
const xmlChar * xmlTextReaderConstValue(xmlTextReader * reader);
// 
// use the Const version of the routine for better performance and simpler code
// 
xmlChar * xmlTextReaderBaseUri(xmlTextReader * reader);
xmlChar * xmlTextReaderLocalName(xmlTextReader * reader);
xmlChar * xmlTextReaderName(xmlTextReader * reader);
xmlChar * xmlTextReaderNamespaceUri(xmlTextReader * reader);
xmlChar * xmlTextReaderPrefix(xmlTextReader * reader);
xmlChar * xmlTextReaderXmlLang(xmlTextReader * reader);
xmlChar * xmlTextReaderValue(xmlTextReader * reader);
// 
// Methods of the XmlTextReader
// 
int xmlTextReaderClose(xmlTextReader * reader);
xmlChar * xmlTextReaderGetAttributeNo(xmlTextReader * reader, int no);
xmlChar * xmlTextReaderGetAttribute(xmlTextReader * reader, const xmlChar * name);
xmlChar * xmlTextReaderGetAttributeNs(xmlTextReader * reader, const xmlChar * localName, const xmlChar * namespaceURI);
xmlParserInputBuffer * xmlTextReaderGetRemainder(xmlTextReader * reader);
xmlChar * xmlTextReaderLookupNamespace(xmlTextReader * reader, const xmlChar * prefix);
int xmlTextReaderMoveToAttributeNo(xmlTextReader * reader, int no);
int xmlTextReaderMoveToAttribute(xmlTextReader * reader, const xmlChar * name);
int xmlTextReaderMoveToAttributeNs(xmlTextReader * reader, const xmlChar * localName, const xmlChar * namespaceURI);
int xmlTextReaderMoveToFirstAttribute(xmlTextReader * reader);
int xmlTextReaderMoveToNextAttribute(xmlTextReader * reader);
int xmlTextReaderMoveToElement(xmlTextReader * reader);
int xmlTextReaderNormalization(xmlTextReader * reader);
const xmlChar * xmlTextReaderConstEncoding(xmlTextReader * reader);
// 
// Extensions
// 
int xmlTextReaderSetParserProp(xmlTextReader * reader, int prop, int value);
int xmlTextReaderGetParserProp(xmlTextReader * reader, int prop);
xmlNode * xmlTextReaderCurrentNode(xmlTextReader * reader);
int xmlTextReaderGetParserLineNumber(xmlTextReader * reader);
int xmlTextReaderGetParserColumnNumber(xmlTextReader * reader);
xmlNode * xmlTextReaderPreserve(xmlTextReader * reader);
#ifdef LIBXML_PATTERN_ENABLED
	int xmlTextReaderPreservePattern(xmlTextReader * reader, const xmlChar * pattern, const xmlChar ** namespaces);
#endif
xmlDoc  * xmlTextReaderCurrentDoc(xmlTextReader * reader);
xmlNode * xmlTextReaderExpand(xmlTextReader * reader);
int xmlTextReaderNext(xmlTextReader * reader);
int xmlTextReaderNextSibling(xmlTextReader * reader);
int xmlTextReaderIsValid(xmlTextReader * reader);
#ifdef LIBXML_SCHEMAS_ENABLED
	int xmlTextReaderRelaxNGValidate(xmlTextReader * reader, const char * rng);
	int xmlTextReaderRelaxNGValidateCtxt(xmlTextReader * reader, xmlRelaxNGValidCtxtPtr ctxt, int options);
	int xmlTextReaderRelaxNGSetSchema(xmlTextReader * reader, xmlRelaxNGPtr schema);
	int xmlTextReaderSchemaValidate(xmlTextReader * reader, const char * xsd);
	int xmlTextReaderSchemaValidateCtxt(xmlTextReader * reader, xmlSchemaValidCtxt * ctxt, int options);
	int xmlTextReaderSetSchema(xmlTextReader * reader, xmlSchemaPtr schema);
#endif
const xmlChar * xmlTextReaderConstXmlVersion(xmlTextReader * reader);
int xmlTextReaderStandalone(xmlTextReader * reader);
// 
// Index lookup
// 
long xmlTextReaderByteConsumed(xmlTextReader * reader);
// 
// New more complete APIs for simpler creation and reuse of readers
// 
xmlTextReader * xmlReaderWalker(xmlDoc * doc);
xmlTextReader * xmlReaderForDoc(const xmlChar * cur, const char * URL, const char * encoding, int options);
xmlTextReader * xmlReaderForFile(const char * filename, const char * encoding, int options);
xmlTextReader * xmlReaderForMemory(const char * buffer, int size, const char * URL, const char * encoding, int options);
xmlTextReader * xmlReaderForFd(int fd, const char * URL, const char * encoding, int options);
xmlTextReader * xmlReaderForIO(xmlInputReadCallback ioread, xmlInputCloseCallback ioclose, void * ioctx, const char * URL, const char * encoding, int options);
int xmlReaderNewWalker(xmlTextReader * reader, xmlDoc * doc);
int xmlReaderNewDoc(xmlTextReader * reader, const xmlChar * cur, const char * URL, const char * encoding, int options);
int xmlReaderNewFile(xmlTextReader * reader, const char * filename, const char * encoding, int options);
int xmlReaderNewMemory(xmlTextReader * reader, const char * buffer, int size, const char * URL, const char * encoding, int options);
int xmlReaderNewFd(xmlTextReader * reader, int fd, const char * URL, const char * encoding, int options);
int xmlReaderNewIO(xmlTextReader * reader, xmlInputReadCallback ioread, xmlInputCloseCallback ioclose, void * ioctx, const char * URL, const char * encoding, int options);
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
int xmlTextReaderLocatorLineNumber(xmlTextReaderLocatorPtr locator);
xmlChar * xmlTextReaderLocatorBaseURI(xmlTextReaderLocatorPtr locator);
void xmlTextReaderSetErrorHandler(xmlTextReader * reader, xmlTextReaderErrorFunc f, void * arg);
void xmlTextReaderSetStructuredErrorHandler(xmlTextReader * reader, xmlStructuredErrorFunc f, void * arg);
void xmlTextReaderGetErrorHandler(xmlTextReader * reader, xmlTextReaderErrorFunc * f, void ** arg);

#endif /* LIBXML_READER_ENABLED */

//#ifdef __cplusplus
//}
//#endif

#endif /* __XML_XMLREADER_H__ */

