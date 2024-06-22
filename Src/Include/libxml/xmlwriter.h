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

//#ifdef __cplusplus
//extern "C" {
//#endif

struct xmlBuffer;
struct xmlTextWriter;
struct xmlDoc;
typedef xmlTextWriter * xmlTextWriterPtr;

xmlTextWriter * xmlNewTextWriter(xmlOutputBuffer * out);
xmlTextWriter * xmlNewTextWriterFilename(const char * uri, int compression);
xmlTextWriter * xmlNewTextWriterMemory(xmlBuffer * buf, int compression);
xmlTextWriter * xmlNewTextWriterPushParser(xmlParserCtxt * ctxt, int compression);
xmlTextWriter * xmlNewTextWriterDoc(xmlDoc ** doc, int compression);
xmlTextWriter * xmlNewTextWriterTree(xmlDoc * doc, xmlNode * pNode, int compression);
void FASTCALL xmlFreeTextWriter(xmlTextWriter * pWriter);
/*
 * Functions
 */
/*
 * Document
 */
int FASTCALL xmlTextWriterStartDocument(xmlTextWriter * writer, const char * version, const char * encoding, const char * standalone);
int FASTCALL xmlTextWriterEndDocument(xmlTextWriter * writer);
/*
 * Comments
 */
int xmlTextWriterStartComment(xmlTextWriter * writer);
int xmlTextWriterEndComment(xmlTextWriter * writer);
int xmlTextWriterWriteFormatComment(xmlTextWriter * writer, const char * format, ...) LIBXML_ATTR_FORMAT(2, 3);
int xmlTextWriterWriteVFormatComment(xmlTextWriter * writer, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(2, 0);
int xmlTextWriterWriteComment(xmlTextWriter * writer, const xmlChar * content);
/*
 * Elements
 */
int FASTCALL xmlTextWriterStartElement(xmlTextWriter * writer, const xmlChar * name);
int xmlTextWriterStartElementNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI);
int FASTCALL xmlTextWriterEndElement(xmlTextWriter * writer);
int xmlTextWriterFullEndElement(xmlTextWriter * writer);
/*
 * Elements conveniency functions
 */
int xmlTextWriterWriteFormatElement(xmlTextWriter * writer, const xmlChar * name, const char * format, ...) LIBXML_ATTR_FORMAT(3, 4);
int xmlTextWriterWriteVFormatElement(xmlTextWriter * writer, const xmlChar * name, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(3, 0);
int xmlTextWriterWriteElement(xmlTextWriter * writer, const xmlChar * name, const xmlChar * content);
int xmlTextWriterWriteFormatElementNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const char * format, ...) LIBXML_ATTR_FORMAT(5, 6);
int xmlTextWriterWriteVFormatElementNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(5, 0);
int xmlTextWriterWriteElementNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const xmlChar * content);
/*
 * Text
 */
int xmlTextWriterWriteFormatRaw(xmlTextWriter * writer, const char * format, ...) LIBXML_ATTR_FORMAT(2, 3);
int xmlTextWriterWriteVFormatRaw(xmlTextWriter * writer, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(2, 0);
int FASTCALL xmlTextWriterWriteRawLen(xmlTextWriter * writer, const xmlChar * content, int len);
int FASTCALL xmlTextWriterWriteRaw(xmlTextWriter * writer, const xmlChar * content);
int xmlTextWriterWriteFormatString(xmlTextWriter * writer, const char * format, ...) LIBXML_ATTR_FORMAT(2, 3);
int xmlTextWriterWriteVFormatString(xmlTextWriter * writer, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(2, 0);
int FASTCALL xmlTextWriterWriteString(xmlTextWriter * writer, const xmlChar * content);
int xmlTextWriterWriteBase64(xmlTextWriter * writer, const char * data, int start, int len);
int xmlTextWriterWriteBinHex(xmlTextWriter * writer, const char * data, int start, int len);
/*
 * Attributes
 */
int FASTCALL xmlTextWriterStartAttribute(xmlTextWriter * writer, const xmlChar * name);
int xmlTextWriterStartAttributeNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI);
int FASTCALL xmlTextWriterEndAttribute(xmlTextWriter * writer);
/*
 * Attributes conveniency functions
 */
int xmlTextWriterWriteFormatAttribute(xmlTextWriter * writer, const xmlChar * name, const char * format, ...) LIBXML_ATTR_FORMAT(3, 4);
int xmlTextWriterWriteVFormatAttribute(xmlTextWriter * writer, const xmlChar * name, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(3, 0);
int xmlTextWriterWriteAttribute(xmlTextWriter * writer, const xmlChar * name, const xmlChar * content);
int xmlTextWriterWriteFormatAttributeNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const char * format, ...) LIBXML_ATTR_FORMAT(5, 6);
int xmlTextWriterWriteVFormatAttributeNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(5, 0);
int xmlTextWriterWriteAttributeNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const xmlChar * content);
/*
 * PI's
 */
int xmlTextWriterStartPI(xmlTextWriter * writer, const xmlChar * target);
int xmlTextWriterEndPI(xmlTextWriter * writer);
/*
 * PI conveniency functions
 */
int xmlTextWriterWriteFormatPI(xmlTextWriter * writer, const xmlChar * target, const char * format, ...) LIBXML_ATTR_FORMAT(3, 4);
int xmlTextWriterWriteVFormatPI(xmlTextWriter * writer, const xmlChar * target, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(3, 0);
int xmlTextWriterWritePI(xmlTextWriter * writer, const xmlChar * target, const xmlChar * content);
/**
 * xmlTextWriterWriteProcessingInstruction:
 *
 * This macro maps to xmlTextWriterWritePI
 */
#define xmlTextWriterWriteProcessingInstruction xmlTextWriterWritePI
/*
 * CDATA
 */
int xmlTextWriterStartCDATA(xmlTextWriter * writer);
int xmlTextWriterEndCDATA(xmlTextWriter * writer);
/*
 * CDATA conveniency functions
 */
int xmlTextWriterWriteFormatCDATA(xmlTextWriter * writer, const char * format, ...) LIBXML_ATTR_FORMAT(2, 3);
int xmlTextWriterWriteVFormatCDATA(xmlTextWriter * writer, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(2, 0);
int xmlTextWriterWriteCDATA(xmlTextWriter * writer, const xmlChar * content);
/*
 * DTD
 */
int xmlTextWriterStartDTD(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid);
int xmlTextWriterEndDTD(xmlTextWriter * writer);
/*
 * DTD conveniency functions
 */
int xmlTextWriterWriteFormatDTD(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const char * format, ...) LIBXML_ATTR_FORMAT(5, 6);
int xmlTextWriterWriteVFormatDTD(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(5, 0);
int xmlTextWriterWriteDTD(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const xmlChar * subset);
/**
 * xmlTextWriterWriteDocType:
 *
 * this macro maps to xmlTextWriterWriteDTD
 */
#define xmlTextWriterWriteDocType xmlTextWriterWriteDTD
/*
 * DTD element definition
 */
int xmlTextWriterStartDTDElement(xmlTextWriter * writer, const xmlChar * name);
int xmlTextWriterEndDTDElement(xmlTextWriter * writer);
/*
 * DTD element definition conveniency functions
 */
int xmlTextWriterWriteFormatDTDElement(xmlTextWriter * writer, const xmlChar * name, const char * format, ...) LIBXML_ATTR_FORMAT(3, 4);
int xmlTextWriterWriteVFormatDTDElement(xmlTextWriter * writer, const xmlChar * name, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(3, 0);
int xmlTextWriterWriteDTDElement(xmlTextWriter * writer, const xmlChar * name, const xmlChar * content);
/*
 * DTD attribute list definition
 */
int xmlTextWriterStartDTDAttlist(xmlTextWriter * writer, const xmlChar * name);
int xmlTextWriterEndDTDAttlist(xmlTextWriter * writer);
/*
 * DTD attribute list definition conveniency functions
 */
int xmlTextWriterWriteFormatDTDAttlist(xmlTextWriter * writer, const xmlChar * name, const char * format, ...) LIBXML_ATTR_FORMAT(3, 4);
int xmlTextWriterWriteVFormatDTDAttlist(xmlTextWriter * writer, const xmlChar * name, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(3, 0);
int xmlTextWriterWriteDTDAttlist(xmlTextWriter * writer, const xmlChar * name, const xmlChar * content);
/*
 * DTD entity definition
 */
int xmlTextWriterStartDTDEntity(xmlTextWriter * writer, int pe, const xmlChar * name);
int xmlTextWriterEndDTDEntity(xmlTextWriter * writer);
/*
 * DTD entity definition conveniency functions
 */
int xmlTextWriterWriteFormatDTDInternalEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const char * format, ...) LIBXML_ATTR_FORMAT(4, 5);
int xmlTextWriterWriteVFormatDTDInternalEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const char * format, va_list argptr) LIBXML_ATTR_FORMAT(4, 0);
int xmlTextWriterWriteDTDInternalEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const xmlChar * content);
int xmlTextWriterWriteDTDExternalEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const xmlChar * ndataid);
int xmlTextWriterWriteDTDExternalEntityContents(xmlTextWriter * writer, const xmlChar * pubid, const xmlChar * sysid, const xmlChar * ndataid);
int xmlTextWriterWriteDTDEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const xmlChar * ndataid, const xmlChar * content);
/*
 * DTD notation definition
 */
int xmlTextWriterWriteDTDNotation(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid);
/*
 * Indentation
 */
int xmlTextWriterSetIndent(xmlTextWriter * writer, int indent);
int xmlTextWriterSetIndentString(xmlTextWriter * writer, const xmlChar * str);
//
// Descr: equivalent to xmlTextWriterSetIndentString(writer, "\t");
//
int FASTCALL xmlTextWriterSetIndentTab(xmlTextWriter * writer); // @sobolev
int xmlTextWriterSetQuoteChar(xmlTextWriter * writer, xmlChar quotechar);
/*
 * misc
 */
int FASTCALL xmlTextWriterFlush(xmlTextWriter * writer);

//#ifdef __cplusplus
//}
//#endif
#endif /* LIBXML_WRITER_ENABLED */
#endif /* __XML_XMLWRITER_H__ */
