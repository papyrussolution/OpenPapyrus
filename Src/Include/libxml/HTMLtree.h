/*
 * Summary: specific APIs to process HTML tree, especially serialization
 * Description: this module implements a few function needed to process tree in an HTML specific way.
 * Copy: See Copyright for the status of this software.
 * Author: Daniel Veillard
 */
#ifndef __HTML_TREE_H__
#define __HTML_TREE_H__

#ifdef LIBXML_HTML_ENABLED
//#ifdef __cplusplus
//extern "C" {
//#endif
// 
// Macro. A text node in a HTML document is really implemented
// the same way as a text node in an XML document.
// 
#define HTML_TEXT_NODE          XML_TEXT_NODE
// 
// Macro. An entity reference in a HTML document is really implemented
// the same way as an entity reference in an XML document.
// 
#define HTML_ENTITY_REF_NODE    XML_ENTITY_REF_NODE
// 
// Macro. A comment in a HTML document is really implemented
// the same way as a comment in an XML document.
// 
#define HTML_COMMENT_NODE       XML_COMMENT_NODE
// 
// Macro. A preserved node in a HTML document is really implemented
// the same way as a CDATA section in an XML document.
// 
#define HTML_PRESERVE_NODE      XML_CDATA_SECTION_NODE
// 
// Macro. A processing instruction in a HTML document is really implemented
// the same way as a processing instruction in an XML document.
// 
#define HTML_PI_NODE            XML_PI_NODE

htmlDocPtr htmlNewDoc(const xmlChar * URI, const xmlChar * ExternalID);
htmlDocPtr htmlNewDocNoDtD(const xmlChar * URI, const xmlChar * ExternalID);
const xmlChar * htmlGetMetaEncoding(htmlDocPtr doc);
int htmlSetMetaEncoding(htmlDocPtr doc, const xmlChar * encoding);
#ifdef LIBXML_OUTPUT_ENABLED
	void htmlDocDumpMemory(xmlDoc * cur, xmlChar ** mem, int * size);
	void htmlDocDumpMemoryFormat(xmlDoc * cur, xmlChar ** mem, int * size, int format);
	int htmlDocDump(FILE * f, xmlDoc * cur);
	int htmlSaveFile(const char * filename, xmlDoc * cur);
	int htmlNodeDump(xmlBuffer * buf, xmlDoc * doc, xmlNode * cur);
	void htmlNodeDumpFile(FILE * out, xmlDoc * doc, xmlNode * cur);
	int htmlNodeDumpFileFormat(FILE * out, xmlDoc * doc, xmlNode * cur, const char * encoding, int format);
	int htmlSaveFileEnc(const char * filename, xmlDoc * cur, const char * encoding);
	int htmlSaveFileFormat(const char * filename, xmlDoc * cur, const char * encoding, int format);
	void htmlNodeDumpFormatOutput(xmlOutputBuffer * buf, xmlDoc * doc, xmlNode * cur, const char * encoding, int format);
	void htmlDocContentDumpOutput(xmlOutputBuffer * buf, xmlDoc * cur, const char * encoding);
	void htmlDocContentDumpFormatOutput(xmlOutputBuffer * buf, xmlDoc * cur, const char * encoding, int format);
	void htmlNodeDumpOutput(xmlOutputBuffer * buf, xmlDoc * doc, xmlNode * cur, const char * encoding);
#endif /* LIBXML_OUTPUT_ENABLED */
int htmlIsBooleanAttr(const xmlChar * name);

//#ifdef __cplusplus
//}
//#endif
#endif /* LIBXML_HTML_ENABLED */
#endif /* __HTML_TREE_H__ */

