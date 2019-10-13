/*
 * Summary: text writing API for XML
 * Description: text writing API for XML
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Alfred Mickautsch <alfred@mickautsch.de>
 */
#ifndef __XML_XMLWRITER_H__
#define __XML_XMLWRITER_H__

#include <libxml/xmlversion.h>

#ifdef LIBXML_WRITER_ENABLED

#include <libxml/globals.h>
#include <libxml/xmlIO.h>

#ifdef __cplusplus
extern "C" {
#endif

struct xmlBuffer;
struct xmlTextWriter;
struct xmlDoc;
typedef xmlTextWriter * xmlTextWriterPtr;
/*
 * Constructors & Destructor
 */
XMLPUBFUN xmlTextWriter * XMLCALL xmlNewTextWriter(xmlOutputBuffer * out);
XMLPUBFUN xmlTextWriter * XMLCALL xmlNewTextWriterFilename(const char * uri, int compression);
XMLPUBFUN xmlTextWriter * XMLCALL xmlNewTextWriterMemory(xmlBuffer * buf, int compression);
XMLPUBFUN xmlTextWriter * XMLCALL xmlNewTextWriterPushParser(xmlParserCtxt * ctxt, int compression);
XMLPUBFUN xmlTextWriter * XMLCALL xmlNewTextWriterDoc(xmlDoc ** doc, int compression);
XMLPUBFUN xmlTextWriter * XMLCALL xmlNewTextWriterTree(xmlDoc * doc, xmlNode * pNode, int compression);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeTextWriter(xmlTextWriter * pWriter);
/*
 * Functions
 */
/*
 * Document
 */
XMLPUBFUN int XMLCALL xmlTextWriterStartDocument(xmlTextWriter * writer, const char * version, const char * encoding, const char * standalone);
XMLPUBFUN int XMLCALL xmlTextWriterEndDocument(xmlTextWriter * writer);
/*
 * Comments
 */
XMLPUBFUN int XMLCALL xmlTextWriterStartComment(xmlTextWriter * writer);
XMLPUBFUN int XMLCALL xmlTextWriterEndComment(xmlTextWriter * writer);
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatComment(xmlTextWriter * writer, const char * format, ...) LIBXML_ATTR_FORMAT(2, 3);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatComment(xmlTextWriter * writer, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(2, 0);
XMLPUBFUN int XMLCALL xmlTextWriterWriteComment(xmlTextWriter * writer, const xmlChar * content);
/*
 * Elements
 */
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlTextWriterStartElement(xmlTextWriter * writer, const xmlChar * name);
XMLPUBFUN int XMLCALL xmlTextWriterStartElementNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlTextWriterEndElement(xmlTextWriter * writer);
XMLPUBFUN int XMLCALL xmlTextWriterFullEndElement(xmlTextWriter * writer);
/*
 * Elements conveniency functions
 */
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatElement(xmlTextWriter * writer, const xmlChar * name, const char * format, ...) LIBXML_ATTR_FORMAT(3, 4);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatElement(xmlTextWriter * writer, const xmlChar * name, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(3, 0);
XMLPUBFUN int XMLCALL xmlTextWriterWriteElement(xmlTextWriter * writer, const xmlChar * name, const xmlChar * content);
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatElementNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const char * format, ...) LIBXML_ATTR_FORMAT(5, 6);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatElementNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(5, 0);
XMLPUBFUN int XMLCALL xmlTextWriterWriteElementNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const xmlChar * content);
/*
 * Text
 */
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatRaw(xmlTextWriter * writer, const char * format, ...) LIBXML_ATTR_FORMAT(2, 3);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatRaw(xmlTextWriter * writer, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(2, 0);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlTextWriterWriteRawLen(xmlTextWriter * writer, const xmlChar * content, int len);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlTextWriterWriteRaw(xmlTextWriter * writer, const xmlChar * content);
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatString(xmlTextWriter * writer, const char * format, ...) LIBXML_ATTR_FORMAT(2, 3);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatString(xmlTextWriter * writer, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(2, 0);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlTextWriterWriteString(xmlTextWriter * writer, const xmlChar * content);
XMLPUBFUN int XMLCALL xmlTextWriterWriteBase64(xmlTextWriter * writer, const char * data, int start, int len);
XMLPUBFUN int XMLCALL xmlTextWriterWriteBinHex(xmlTextWriter * writer, const char * data, int start, int len);
/*
 * Attributes
 */
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlTextWriterStartAttribute(xmlTextWriter * writer, const xmlChar * name);
XMLPUBFUN int XMLCALL xmlTextWriterStartAttributeNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlTextWriterEndAttribute(xmlTextWriter * writer);
/*
 * Attributes conveniency functions
 */
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatAttribute(xmlTextWriter * writer, const xmlChar * name, const char * format, ...) LIBXML_ATTR_FORMAT(3, 4);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatAttribute(xmlTextWriter * writer, const xmlChar * name, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(3, 0);
XMLPUBFUN int XMLCALL xmlTextWriterWriteAttribute(xmlTextWriter * writer, const xmlChar * name, const xmlChar * content);
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatAttributeNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const char * format, ...) LIBXML_ATTR_FORMAT(5, 6);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatAttributeNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(5, 0);
XMLPUBFUN int XMLCALL xmlTextWriterWriteAttributeNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const xmlChar * content);
/*
 * PI's
 */
XMLPUBFUN int XMLCALL xmlTextWriterStartPI(xmlTextWriter * writer, const xmlChar * target);
XMLPUBFUN int XMLCALL xmlTextWriterEndPI(xmlTextWriter * writer);
/*
 * PI conveniency functions
 */
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatPI(xmlTextWriter * writer, const xmlChar * target, const char * format, ...) LIBXML_ATTR_FORMAT(3, 4);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatPI(xmlTextWriter * writer, const xmlChar * target, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(3, 0);
XMLPUBFUN int XMLCALL xmlTextWriterWritePI(xmlTextWriter * writer, const xmlChar * target, const xmlChar * content);
/**
 * xmlTextWriterWriteProcessingInstruction:
 *
 * This macro maps to xmlTextWriterWritePI
 */
#define xmlTextWriterWriteProcessingInstruction xmlTextWriterWritePI
/*
 * CDATA
 */
XMLPUBFUN int XMLCALL xmlTextWriterStartCDATA(xmlTextWriter * writer);
XMLPUBFUN int XMLCALL xmlTextWriterEndCDATA(xmlTextWriter * writer);
/*
 * CDATA conveniency functions
 */
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatCDATA(xmlTextWriter * writer, const char * format, ...) LIBXML_ATTR_FORMAT(2, 3);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatCDATA(xmlTextWriter * writer, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(2, 0);
XMLPUBFUN int XMLCALL xmlTextWriterWriteCDATA(xmlTextWriter * writer, const xmlChar * content);
/*
 * DTD
 */
XMLPUBFUN int XMLCALL xmlTextWriterStartDTD(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid);
XMLPUBFUN int XMLCALL xmlTextWriterEndDTD(xmlTextWriter * writer);
/*
 * DTD conveniency functions
 */
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatDTD(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const char * format, ...) LIBXML_ATTR_FORMAT(5, 6);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatDTD(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(5, 0);
XMLPUBFUN int XMLCALL xmlTextWriterWriteDTD(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const xmlChar * subset);
/**
 * xmlTextWriterWriteDocType:
 *
 * this macro maps to xmlTextWriterWriteDTD
 */
#define xmlTextWriterWriteDocType xmlTextWriterWriteDTD
/*
 * DTD element definition
 */
XMLPUBFUN int XMLCALL xmlTextWriterStartDTDElement(xmlTextWriter * writer, const xmlChar * name);
XMLPUBFUN int XMLCALL xmlTextWriterEndDTDElement(xmlTextWriter * writer);
/*
 * DTD element definition conveniency functions
 */
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatDTDElement(xmlTextWriter * writer, const xmlChar * name, const char * format, ...) LIBXML_ATTR_FORMAT(3, 4);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatDTDElement(xmlTextWriter * writer, const xmlChar * name, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(3, 0);
XMLPUBFUN int XMLCALL xmlTextWriterWriteDTDElement(xmlTextWriter * writer, const xmlChar * name, const xmlChar * content);
/*
 * DTD attribute list definition
 */
XMLPUBFUN int XMLCALL xmlTextWriterStartDTDAttlist(xmlTextWriter * writer, const xmlChar * name);
XMLPUBFUN int XMLCALL xmlTextWriterEndDTDAttlist(xmlTextWriter * writer);
/*
 * DTD attribute list definition conveniency functions
 */
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatDTDAttlist(xmlTextWriter * writer, const xmlChar * name, const char * format, ...) LIBXML_ATTR_FORMAT(3, 4);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatDTDAttlist(xmlTextWriter * writer, const xmlChar * name, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(3, 0);
XMLPUBFUN int XMLCALL xmlTextWriterWriteDTDAttlist(xmlTextWriter * writer, const xmlChar * name, const xmlChar * content);
/*
 * DTD entity definition
 */
XMLPUBFUN int XMLCALL xmlTextWriterStartDTDEntity(xmlTextWriter * writer, int pe, const xmlChar * name);
XMLPUBFUN int XMLCALL xmlTextWriterEndDTDEntity(xmlTextWriter * writer);
/*
 * DTD entity definition conveniency functions
 */
XMLPUBFUN int XMLCALL xmlTextWriterWriteFormatDTDInternalEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const char * format, ...) LIBXML_ATTR_FORMAT(4, 5);
XMLPUBFUN int XMLCALL xmlTextWriterWriteVFormatDTDInternalEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(4, 0);
XMLPUBFUN int XMLCALL xmlTextWriterWriteDTDInternalEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const xmlChar * content);
XMLPUBFUN int XMLCALL xmlTextWriterWriteDTDExternalEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const xmlChar * ndataid);
XMLPUBFUN int XMLCALL xmlTextWriterWriteDTDExternalEntityContents(xmlTextWriter * writer, const xmlChar * pubid, const xmlChar * sysid, const xmlChar * ndataid);
XMLPUBFUN int XMLCALL xmlTextWriterWriteDTDEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const xmlChar * ndataid, const xmlChar * content);
/*
 * DTD notation definition
 */
XMLPUBFUN int XMLCALL xmlTextWriterWriteDTDNotation(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid);
/*
 * Indentation
 */
XMLPUBFUN int XMLCALL xmlTextWriterSetIndent(xmlTextWriter * writer, int indent);
XMLPUBFUN int XMLCALL xmlTextWriterSetIndentString(xmlTextWriter * writer, const xmlChar * str);
XMLPUBFUN int XMLCALL xmlTextWriterSetQuoteChar(xmlTextWriter * writer, xmlChar quotechar);
/*
 * misc
 */
XMLPUBFUN int XMLCALL xmlTextWriterFlush(xmlTextWriter * writer);

#ifdef __cplusplus
}
#endif

#endif /* LIBXML_WRITER_ENABLED */

#endif                          /* __XML_XMLWRITER_H__ */
