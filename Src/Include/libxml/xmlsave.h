// XMLSAVE.H
// Part of libxml
//
#ifndef __XMLSAVE_H
#define __XMLSAVE_H
// 
// Descr: xmlSaveOption
//   This is the set of XML save options that can be passed down
//   to the xmlSaveToFd() and similar calls.
// 
enum xmlSaveOption {
	XML_SAVE_FORMAT     = 1<<0, /* format save output */
	XML_SAVE_NO_DECL    = 1<<1, /* drop the xml declaration */
	XML_SAVE_NO_EMPTY   = 1<<2, /* no empty tags */
	XML_SAVE_NO_XHTML   = 1<<3, /* disable XHTML1 specific rules */
	XML_SAVE_XHTML      = 1<<4, /* force XHTML1 specific rules */
	XML_SAVE_AS_XML     = 1<<5, /* force XML serialization on HTML doc */
	XML_SAVE_AS_HTML    = 1<<6, /* force HTML serialization on XML doc */
	XML_SAVE_WSNONSIG   = 1<<7 /* format with non-significant whitespace */
};

struct xmlSaveCtxt;
//typedef xmlSaveCtxt * xmlSaveCtxtPtr;

xmlSaveCtxt * xmlSaveToFd(int fd, const char * encoding, int options);
xmlSaveCtxt * xmlSaveToFilename(const char * filename, const char * encoding, int options);
xmlSaveCtxt * xmlSaveToBuffer(xmlBuffer * buffer, const char * encoding, int options);
xmlSaveCtxt * xmlSaveToIO(xmlOutputWriteCallback iowrite, xmlOutputCloseCallback ioclose, void * ioctx, const char * encoding, int options);
long xmlSaveDoc(xmlSaveCtxt * ctxt, xmlDoc * doc);
long xmlSaveTree(xmlSaveCtxt * ctxt, xmlNode * pNode);
int xmlSaveFlush(xmlSaveCtxt * ctxt);
int xmlSaveClose(xmlSaveCtxt * ctxt);
int xmlSaveSetEscape(xmlSaveCtxt * ctxt, xmlCharEncodingOutputFunc escape);
int xmlSaveSetAttrEscape(xmlSaveCtxt * ctxt, xmlCharEncodingOutputFunc escape);
//#ifdef __cplusplus
//}
//#endif

#endif // __XMLSAVE_H
