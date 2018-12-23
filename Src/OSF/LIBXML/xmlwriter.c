/*
 * xmlwriter.c: XML text writer implementation
 *
 * For license and disclaimer see the license and disclaimer of
 * libxml2.
 *
 * alfred@mickautsch.de
 */

#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop
//#include <libxml/HTMLtree.h>

#ifdef LIBXML_WRITER_ENABLED
#include <libxml/xmlwriter.h>
#include "save.h"

#define B64LINELEN 72
#define B64CRLF "\r\n"
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
				#define VA_COPY(dest, src) memcpy((char*)(dest), (char*)(src), sizeof(va_list))
			#endif
		#endif
	#endif
#endif
/*
 * Types are kept private
 */
typedef enum {
	XML_TEXTWRITER_NONE = 0,
	XML_TEXTWRITER_NAME,
	XML_TEXTWRITER_ATTRIBUTE,
	XML_TEXTWRITER_TEXT,
	XML_TEXTWRITER_PI,
	XML_TEXTWRITER_PI_TEXT,
	XML_TEXTWRITER_CDATA,
	XML_TEXTWRITER_DTD,
	XML_TEXTWRITER_DTD_TEXT,
	XML_TEXTWRITER_DTD_ELEM,
	XML_TEXTWRITER_DTD_ELEM_TEXT,
	XML_TEXTWRITER_DTD_ATTL,
	XML_TEXTWRITER_DTD_ATTL_TEXT,
	XML_TEXTWRITER_DTD_ENTY, /* entity */
	XML_TEXTWRITER_DTD_ENTY_TEXT,
	XML_TEXTWRITER_DTD_PENT, /* parameter entity */
	XML_TEXTWRITER_COMMENT
} xmlTextWriterState;

typedef struct _xmlTextWriterStackEntry xmlTextWriterStackEntry;

struct _xmlTextWriterStackEntry {
	xmlChar * name;
	xmlTextWriterState state;
};

typedef struct _xmlTextWriterNsStackEntry xmlTextWriterNsStackEntry;
struct _xmlTextWriterNsStackEntry {
	xmlChar * prefix;
	xmlChar * uri;
	xmlLink * elem;
};

struct xmlTextWriter {
	xmlOutputBuffer * out; /* output buffer */
	xmlList * nodes;       /* element name stack */
	xmlList * nsstack;     /* name spaces stack */
	int level;
	int indent;             /* enable indent */
	int doindent;           /* internal indent flag */
	xmlChar * ichar;        /* indent character */
	char qchar;             /* character used for quoting attribute values */
	xmlParserCtxt * ctxt;
	int no_doc_free;
	xmlDoc * doc;
};

static void xmlFreeTextWriterStackEntry(xmlLink * lk);
static int xmlCmpTextWriterStackEntry(const void * data0, const void * data1);
static int xmlTextWriterOutputNSDecl(xmlTextWriter * writer);
static void FASTCALL xmlFreeTextWriterNsStackEntry(xmlLink * pLk);
static int xmlCmpTextWriterNsStackEntry(const void * data0, const void * data1);
static int xmlTextWriterWriteDocCallback(void * context, const xmlChar * str, int len);
static int xmlTextWriterCloseDocCallback(void * context);
static xmlChar * xmlTextWriterVSprintf(const char * format, va_list argptr);
static int xmlOutputBufferWriteBase64(xmlOutputBuffer * out, int len, const uchar * data);
static void xmlTextWriterStartDocumentCallback(void * ctx);
static int xmlTextWriterWriteIndent(xmlTextWriter * writer);
static int xmlTextWriterHandleStateDependencies(xmlTextWriter * writer, xmlTextWriterStackEntry * p);
/**
 * xmlWriterErrMsg:
 * @ctxt:  a writer context
 * @error:  the error number
 * @msg:  the error message
 *
 * Handle a writer error
 */
static void FASTCALL xmlWriterErrMsg(xmlTextWriter * ctxt, xmlParserErrors error, const char * pFunc, const char * pMsg)
{
	SString msg_buf;
	if(!isempty(pFunc)) {
		(msg_buf = pFunc).CatDiv(':', 2).Cat(pMsg);
	}
	else
		msg_buf = pMsg;
	msg_buf.CR();
	if(ctxt)
		__xmlRaiseError(0, 0, 0, ctxt->ctxt, NULL, XML_FROM_WRITER, error, XML_ERR_FATAL, 0, 0, 0, 0, 0, 0, 0, "%s", msg_buf.cptr());
	else
		__xmlRaiseError(0, 0, 0, 0, 0, XML_FROM_WRITER, error, XML_ERR_FATAL, 0, 0, 0, 0, 0, 0, 0, "%s", msg_buf.cptr());
}
/**
 * xmlWriterErrMsgInt:
 * @ctxt:  a writer context
 * @error:  the error number
 * @msg:  the error message
 * @val:  an int
 *
 * Handle a writer error
 */
static void xmlWriterErrMsgInt(xmlTextWriter * ctxt, xmlParserErrors error, const char * msg, int val)
{
	if(ctxt)
		__xmlRaiseError(0, 0, 0, ctxt->ctxt, NULL, XML_FROM_WRITER, error, XML_ERR_FATAL, 0, 0, 0, 0, 0, val, 0, msg, val);
	else
		__xmlRaiseError(0, 0, 0, 0, 0, XML_FROM_WRITER, error, XML_ERR_FATAL, 0, 0, 0, 0, 0, val, 0, msg, val);
}
/**
 * xmlNewTextWriter:
 * @out:  an xmlOutputBufferPtr
 *
 * Create a new xmlNewTextWriter structure using an xmlOutputBufferPtr
 * NOTE: the @out parameter will be deallocated when the writer is closed
 *       (if the call succeed.)
 *
 * Returns the new xmlTextWriterPtr or NULL in case of error
 */
xmlTextWriter * xmlNewTextWriter(xmlOutputBuffer * out)
{
	const char * _p_func_name = "xmlNewTextWriter";
	xmlTextWriter * ret = (xmlTextWriterPtr)SAlloc::M(sizeof(xmlTextWriter));
	if(!ret) {
		xmlWriterErrMsg(NULL, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		return NULL;
	}
	memzero(ret, (size_t)sizeof(xmlTextWriter));
	ret->nodes = xmlListCreate((xmlListDeallocator)xmlFreeTextWriterStackEntry, (xmlListDataCompare)xmlCmpTextWriterStackEntry);
	if(ret->nodes == NULL) {
		xmlWriterErrMsg(NULL, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		SAlloc::F(ret);
		return NULL;
	}
	ret->nsstack = xmlListCreate((xmlListDeallocator)xmlFreeTextWriterNsStackEntry, (xmlListDataCompare)xmlCmpTextWriterNsStackEntry);
	if(ret->nsstack == NULL) {
		xmlWriterErrMsg(NULL, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		xmlListDelete(ret->nodes);
		SAlloc::F(ret);
		return NULL;
	}
	ret->out = out;
	ret->ichar = sstrdup(BAD_CAST " ");
	ret->qchar = '"';
	if(!ret->ichar) {
		xmlListDelete(ret->nodes);
		xmlListDelete(ret->nsstack);
		SAlloc::F(ret);
		xmlWriterErrMsg(NULL, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		return NULL;
	}
	ret->doc = xmlNewDoc(NULL);
	ret->no_doc_free = 0;
	return ret;
}
// 
// Descr: Create a new xmlNewTextWriter structure with @uri as output
// @uri:  the URI of the resource for the output
// @compression:  compress the output?
// Returns the new xmlTextWriterPtr or NULL in case of error
// 
xmlTextWriter * xmlNewTextWriterFilename(const char * uri, int compression)
{
	const char * _p_func_name = "xmlNewTextWriterFilename";
	xmlTextWriter * ret = 0;
	xmlOutputBuffer * out = xmlOutputBufferCreateFilename(uri, NULL, compression);
	if(out == NULL) {
		xmlWriterErrMsg(NULL, XML_IO_EIO, _p_func_name, "cannot open uri");
	}
	else {
		ret = xmlNewTextWriter(out);
		if(!ret) {
			xmlWriterErrMsg(NULL, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
			xmlOutputBufferClose(out);
		}
		else {
			ret->indent = 0;
			ret->doindent = 0;
		}
	}
	return ret;
}
// 
// Descr: Create a new xmlNewTextWriter structure with @buf as output
// @buf:  xmlBufferPtr
// @compression:  compress the output?
// @todo handle compression
// Returns the new xmlTextWriterPtr or NULL in case of error
// 
xmlTextWriter * xmlNewTextWriterMemory(xmlBuffer * buf, int compression ATTRIBUTE_UNUSED)
{
	const char * _p_func_name = "xmlNewTextWriterMemory";
	xmlTextWriter * ret = 0;
	//::todo handle compression 
	xmlOutputBuffer * out = xmlOutputBufferCreateBuffer(buf, 0);
	if(out == NULL) {
		xmlWriterErrMsg(NULL, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
	}
	else {
		ret = xmlNewTextWriter(out);
		if(!ret) {
			xmlWriterErrMsg(NULL, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
			xmlOutputBufferClose(out);
		}
	}
	return ret;
}
/**
 * xmlNewTextWriterPushParser:
 * @ctxt: xmlParserCtxtPtr to hold the new XML document tree
 * @compression:  compress the output?
 *
 * Create a new xmlNewTextWriter structure with @ctxt as output
 * NOTE: the @ctxt context will be freed with the resulting writer (if the call succeeds).
 * @todo handle compression
 *
 * Returns the new xmlTextWriterPtr or NULL in case of error
 */
xmlTextWriter * xmlNewTextWriterPushParser(xmlParserCtxt * ctxt, int compression ATTRIBUTE_UNUSED)
{
	const char * _p_func_name = "xmlNewTextWriterPushParser";
	xmlTextWriter * ret;
	xmlOutputBuffer * out;
	if(!ctxt) {
		xmlWriterErrMsg(NULL, XML_ERR_INTERNAL_ERROR, _p_func_name, "invalid context!");
		return NULL;
	}
	out = xmlOutputBufferCreateIO((xmlOutputWriteCallback)xmlTextWriterWriteDocCallback, (xmlOutputCloseCallback)xmlTextWriterCloseDocCallback, (void*)ctxt, 0);
	if(out == NULL) {
		xmlWriterErrMsg(NULL, XML_ERR_INTERNAL_ERROR, _p_func_name, "error at xmlOutputBufferCreateIO!");
		return NULL;
	}
	ret = xmlNewTextWriter(out);
	if(!ret) {
		xmlWriterErrMsg(NULL, XML_ERR_INTERNAL_ERROR, _p_func_name, "error at xmlNewTextWriter!");
		xmlOutputBufferClose(out);
		return NULL;
	}
	ret->ctxt = ctxt;
	return ret;
}
/**
 * xmlNewTextWriterDoc:
 * @doc: address of a xmlDocPtr to hold the new XML document tree
 * @compression:  compress the output?
 *
 * Create a new xmlNewTextWriter structure with @*doc as output
 *
 * Returns the new xmlTextWriterPtr or NULL in case of error
 */
xmlTextWriter * xmlNewTextWriterDoc(xmlDocPtr * doc, int compression)
{
	const char * _p_func_name = "xmlNewTextWriterDoc";
	xmlTextWriter * ret;
	xmlSAXHandler saxHandler;
	xmlParserCtxt * ctxt;
	MEMSZERO(saxHandler);
	xmlSAX2InitDefaultSAXHandler(&saxHandler, 1);
	saxHandler.startDocument = xmlTextWriterStartDocumentCallback;
	saxHandler.startElement = xmlSAX2StartElement;
	saxHandler.endElement = xmlSAX2EndElement;
	ctxt = xmlCreatePushParserCtxt(&saxHandler, NULL, NULL, 0, 0);
	if(!ctxt) {
		xmlWriterErrMsg(NULL, XML_ERR_INTERNAL_ERROR, _p_func_name, "error at xmlCreatePushParserCtxt!");
		return NULL;
	}
	//
	// For some reason this seems to completely break if node names are interned.
	//
	ctxt->dictNames = 0;
	ctxt->myDoc = xmlNewDoc(BAD_CAST XML_DEFAULT_VERSION);
	if(ctxt->myDoc == NULL) {
		xmlFreeParserCtxt(ctxt);
		xmlWriterErrMsg(NULL, XML_ERR_INTERNAL_ERROR, _p_func_name, "error at xmlNewDoc!");
		return NULL;
	}
	ret = xmlNewTextWriterPushParser(ctxt, compression);
	if(!ret) {
		xmlFreeDoc(ctxt->myDoc);
		xmlFreeParserCtxt(ctxt);
		xmlWriterErrMsg(NULL, XML_ERR_INTERNAL_ERROR, _p_func_name, "error at xmlNewTextWriterPushParser!");
		return NULL;
	}
	xmlSetDocCompressMode(ctxt->myDoc, compression);
	if(doc) {
		*doc = ctxt->myDoc;
		ret->no_doc_free = 1;
	}
	return ret;
}
/**
 * xmlNewTextWriterTree:
 * @doc: xmlDocPtr
 * @node: xmlNodePtr or NULL for doc->children
 * @compression:  compress the output?
 *
 * Create a new xmlNewTextWriter structure with @doc as output
 * starting at @node
 *
 * Returns the new xmlTextWriter * or NULL in case of error
 */
xmlTextWriter * xmlNewTextWriterTree(xmlDoc * doc, xmlNodePtr P_Node, int compression)
{
	const char * _p_func_name = "xmlNewTextWriterTree";
	xmlTextWriter * ret = 0;
	xmlSAXHandler saxHandler;
	if(!doc) {
		xmlWriterErrMsg(NULL, XML_ERR_INTERNAL_ERROR, _p_func_name, "invalid document tree!");
	}
	else {
		MEMSZERO(saxHandler);
		xmlSAX2InitDefaultSAXHandler(&saxHandler, 1);
		saxHandler.startDocument = xmlTextWriterStartDocumentCallback;
		saxHandler.startElement = xmlSAX2StartElement;
		saxHandler.endElement = xmlSAX2EndElement;
		xmlParserCtxt * ctxt = xmlCreatePushParserCtxt(&saxHandler, NULL, NULL, 0, 0);
		if(!ctxt) {
			xmlWriterErrMsg(NULL, XML_ERR_INTERNAL_ERROR, _p_func_name, "error at xmlCreatePushParserCtxt!");
		}
		else {
			//
			// For some reason this seems to completely break if node names are interned.
			//
			ctxt->dictNames = 0;
			ret = xmlNewTextWriterPushParser(ctxt, compression);
			if(!ret) {
				xmlFreeParserCtxt(ctxt);
				xmlWriterErrMsg(NULL, XML_ERR_INTERNAL_ERROR, _p_func_name, "error at xmlNewTextWriterPushParser!");
			}
			else {
				ctxt->myDoc = doc;
				ctxt->P_Node = P_Node;
				ret->no_doc_free = 1;
				xmlSetDocCompressMode(doc, compression);
			}
		}
	}
	return ret;
}
/**
 * @writer:  the xmlTextWriterPtr
 *
 * Deallocate all the resources associated to the writer
 */
void FASTCALL xmlFreeTextWriter(xmlTextWriter * pWriter)
{
	if(pWriter) {
		xmlOutputBufferClose(pWriter->out);
		xmlListDelete(pWriter->nodes);
		xmlListDelete(pWriter->nsstack);
		if(pWriter->ctxt) {
			if(pWriter->ctxt->myDoc && (pWriter->no_doc_free == 0)) {
				xmlFreeDoc(pWriter->ctxt->myDoc);
				pWriter->ctxt->myDoc = NULL;
			}
			xmlFreeParserCtxt(pWriter->ctxt);
		}
		if(pWriter->doc)
			xmlFreeDoc(pWriter->doc);
		SAlloc::F(pWriter->ichar);
		SAlloc::F(pWriter);
	}
}
/**
 * xmlTextWriterStartDocument:
 * @writer:  the xmlTextWriterPtr
 * @version:  the xml version ("1.0") or NULL for default ("1.0")
 * @encoding:  the encoding or NULL for default
 * @standalone: "yes" or "no" or NULL for default
 *
 * Start a new xml document
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterStartDocument(xmlTextWriter * writer, const char * version, const char * encoding, const char * standalone)
{
	const char * _p_func_name = "xmlTextWriterStartDocument";
	int    sum = 0;
	int    count;
	int    _cp1251 = 0;
	xmlLink * lk;
	xmlCharEncodingHandler * encoder = 0;
	if(!writer || !writer->out) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "invalid writer!");
		CALLEXCEPT();
	}
	lk = xmlListFront(writer->nodes);
	if(lk && xmlLinkGetData(lk)) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "not allowed in this context!");
		CALLEXCEPT();
	}
	// AHTOXA {
	if(stricmp(encoding, "windows-1251") == 0) {
		_cp1251 = 1;
		writer->doc->encoding = sstrdup((xmlChar *)"windows-1251"); // @sobolev @v7.6.8
	}
	else { // } AHTOXA
		if(encoding) {
			encoder = xmlFindCharEncodingHandler(encoding);
			if(encoder == NULL) {
				xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
				CALLEXCEPT();
			}
		}
	}
	writer->out->encoder = encoder;
	if(encoder) {
		SETIFZ(writer->out->conv, xmlBufCreateSize(4000));
		xmlCharEncOutput(writer->out, 1);
		if(writer->doc && !writer->doc->encoding)
			writer->doc->encoding = sstrdup((xmlChar*)writer->out->encoder->name);
	}
	else
		writer->out->conv = NULL;
	count = xmlOutputBufferWriteString(writer->out, "<?xml version=");
	THROW(count >= 0);
	sum += count;
	count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
	THROW(count >= 0);
	sum += count;
	count = xmlOutputBufferWriteString(writer->out, version ? version : "1.0");
	THROW(count >= 0);
	sum += count;
	count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
	THROW(count >= 0);
	sum += count;
	// AHTOXA {
	if(_cp1251) {
		count = xmlOutputBufferWriteString(writer->out, " encoding=");
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWriteString(writer->out, encoding);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
	}
	else { // } AHTOXA
		if(writer->out->encoder != 0) {
			count = xmlOutputBufferWriteString(writer->out, " encoding=");
			THROW(count >= 0);
			sum += count;
			count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
			THROW(count >= 0);
			sum += count;
			count = xmlOutputBufferWriteString(writer->out, writer->out->encoder->name);
			THROW(count >= 0);
			sum += count;
			count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
			THROW(count >= 0);
			sum += count;
		}
	}
	if(standalone != 0) {
		count = xmlOutputBufferWriteString(writer->out, " standalone=");
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWriteString(writer->out, standalone);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
	}
	count = xmlOutputBufferWriteString(writer->out, "?>\n");
	THROW(count >= 0);
	sum += count;
	CATCH
		sum = -1;
	ENDCATCH
	return sum;
}
/**
 * xmlTextWriterEndDocument:
 * @writer:  the xmlTextWriterPtr
 *
 * End an xml document. All open elements are closed, and
 * the content is flushed to the output.
 *
 * Returns the bytes written or -1 in case of error
 */
int xmlTextWriterEndDocument(xmlTextWriter * writer)
{
	const char * _p_func_name = "xmlTextWriterEndDocument";
	int count;
	int sum;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "invalid writer!");
		return -1;
	}
	sum = 0;
	while((lk = xmlListFront(writer->nodes)) != NULL) {
		p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
		if(p == 0)
			break;
		switch(p->state) {
			case XML_TEXTWRITER_NAME:
			case XML_TEXTWRITER_ATTRIBUTE:
			case XML_TEXTWRITER_TEXT:
			    count = xmlTextWriterEndElement(writer);
			    if(count < 0)
				    return -1;
			    sum += count;
			    break;
			case XML_TEXTWRITER_PI:
			case XML_TEXTWRITER_PI_TEXT:
			    count = xmlTextWriterEndPI(writer);
			    if(count < 0)
				    return -1;
			    sum += count;
			    break;
			case XML_TEXTWRITER_CDATA:
			    count = xmlTextWriterEndCDATA(writer);
			    if(count < 0)
				    return -1;
			    sum += count;
			    break;
			case XML_TEXTWRITER_DTD:
			case XML_TEXTWRITER_DTD_TEXT:
			case XML_TEXTWRITER_DTD_ELEM:
			case XML_TEXTWRITER_DTD_ELEM_TEXT:
			case XML_TEXTWRITER_DTD_ATTL:
			case XML_TEXTWRITER_DTD_ATTL_TEXT:
			case XML_TEXTWRITER_DTD_ENTY:
			case XML_TEXTWRITER_DTD_ENTY_TEXT:
			case XML_TEXTWRITER_DTD_PENT:
			    count = xmlTextWriterEndDTD(writer);
			    if(count < 0)
				    return -1;
			    sum += count;
			    break;
			case XML_TEXTWRITER_COMMENT:
			    count = xmlTextWriterEndComment(writer);
			    if(count < 0)
				    return -1;
			    sum += count;
			    break;
			default:
			    break;
		}
	}
	if(!writer->indent) {
		count = xmlOutputBufferWriteString(writer->out, "\n");
		if(count < 0)
			return -1;
		sum += count;
	}
	sum += xmlTextWriterFlush(writer);
	return sum;
}
/**
 * xmlTextWriterStartComment:
 * @writer:  the xmlTextWriterPtr
 *
 * Start an xml comment.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterStartComment(xmlTextWriter * writer)
{
	const char * _p_func_name = "xmlTextWriterStartComment";
	int count;
	int sum;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "invalid writer!");
		return -1;
	}
	sum = 0;
	lk = xmlListFront(writer->nodes);
	if(lk != 0) {
		p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
		if(p != 0) {
			switch(p->state) {
				case XML_TEXTWRITER_TEXT:
				case XML_TEXTWRITER_NONE:
				    break;
				case XML_TEXTWRITER_NAME:
				    /* Output namespace declarations */
				    count = xmlTextWriterOutputNSDecl(writer);
				    if(count < 0)
					    return -1;
				    sum += count;
				    count = xmlOutputBufferWriteString(writer->out, ">");
				    if(count < 0)
					    return -1;
				    sum += count;
				    if(writer->indent) {
					    count =
					    xmlOutputBufferWriteString(writer->out, "\n");
					    if(count < 0)
						    return -1;
					    sum += count;
				    }
				    p->state = XML_TEXTWRITER_TEXT;
				    break;
				default:
				    return -1;
			}
		}
	}
	p = (xmlTextWriterStackEntry*)SAlloc::M(sizeof(xmlTextWriterStackEntry));
	if(p == 0) {
		xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		return -1;
	}
	p->name = NULL;
	p->state = XML_TEXTWRITER_COMMENT;
	xmlListPushFront(writer->nodes, p);
	if(writer->indent) {
		count = xmlTextWriterWriteIndent(writer);
		if(count < 0)
			return -1;
		sum += count;
	}
	count = xmlOutputBufferWriteString(writer->out, "<!--");
	if(count < 0)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterEndComment:
 * @writer:  the xmlTextWriterPtr
 *
 * End the current xml coment.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterEndComment(xmlTextWriter * writer)
{
	const char * _p_func_name = "xmlTextWriterEndComment";
	int count;
	int sum;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "invalid writer!");
		return -1;
	}
	lk = xmlListFront(writer->nodes);
	if(lk == 0) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "not allowed in this context!");
		return -1;
	}
	p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p == 0)
		return -1;
	sum = 0;
	switch(p->state) {
		case XML_TEXTWRITER_COMMENT:
		    count = xmlOutputBufferWriteString(writer->out, "-->");
		    if(count < 0)
			    return -1;
		    sum += count;
		    break;
		default:
		    return -1;
	}
	if(writer->indent) {
		count = xmlOutputBufferWriteString(writer->out, "\n");
		if(count < 0)
			return -1;
		sum += count;
	}
	xmlListPopFront(writer->nodes);
	return sum;
}
/**
 * xmlTextWriterWriteFormatComment:
 * @writer:  the xmlTextWriterPtr
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write an xml comment.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatComment(xmlTextWriter * writer, const char * format, ...)
{
	int rc;
	va_list ap;
	va_start(ap, format);
	rc = xmlTextWriterWriteVFormatComment(writer, format, ap);
	va_end(ap);
	return rc;
}
/**
 * xmlTextWriterWriteVFormatComment:
 * @writer:  the xmlTextWriterPtr
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write an xml comment.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatComment(xmlTextWriter * writer, const char * format, va_list argptr)
{
	const char * _p_func_name = "xmlTextWriterWriteVFormatComment";
	int rc;
	xmlChar * buf;
	if(writer == NULL) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "invalid writer!");
		return -1;
	}
	buf = xmlTextWriterVSprintf(format, argptr);
	if(!buf)
		return -1;
	rc = xmlTextWriterWriteComment(writer, buf);
	SAlloc::F(buf);
	return rc;
}
/**
 * xmlTextWriterWriteComment:
 * @writer:  the xmlTextWriterPtr
 * @content:  comment string
 *
 * Write an xml comment.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteComment(xmlTextWriter * writer, const xmlChar * content)
{
	int sum = 0;
	int count = xmlTextWriterStartComment(writer);
	if(count < 0)
		return -1;
	sum += count;
	count = xmlTextWriterWriteString(writer, content);
	if(count < 0)
		return -1;
	sum += count;
	count = xmlTextWriterEndComment(writer);
	if(count < 0)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterStartElement:
 * @writer:  the xmlTextWriterPtr
 * @name:  element name
 *
 * Start an xml element.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterStartElement(xmlTextWriter * writer, const xmlChar * name)
{
	const char * _p_func_name = "xmlTextWriterStartElement";
	int sum = 0;
	int count;
	xmlTextWriterStackEntry * p;
	if(!writer || !name || name[0] == '\0')
		sum = -1;
	else {
		xmlLink * lk = xmlListFront(writer->nodes);
		if(lk != 0) {
			p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
			if(p != 0) {
				switch(p->state) {
					case XML_TEXTWRITER_PI:
					case XML_TEXTWRITER_PI_TEXT:
						return -1;
					case XML_TEXTWRITER_NONE:
						break;
					case XML_TEXTWRITER_ATTRIBUTE:
						count = xmlTextWriterEndAttribute(writer);
						if(count < 0)
							return -1;
						sum += count;
					// @fallthrough
					case XML_TEXTWRITER_NAME:
						/* Output namespace declarations */
						count = xmlTextWriterOutputNSDecl(writer);
						if(count < 0)
							return -1;
						sum += count;
						count = xmlOutputBufferWriteString(writer->out, ">");
						if(count < 0)
							return -1;
						sum += count;
						if(writer->indent)
							count = xmlOutputBufferWriteString(writer->out, "\n");
						p->state = XML_TEXTWRITER_TEXT;
						break;
					default:
						break;
				}
			}
		}
		p = (xmlTextWriterStackEntry*)SAlloc::M(sizeof(xmlTextWriterStackEntry));
		if(p == 0) {
			xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
			return -1;
		}
		p->name = sstrdup(name);
		if(p->name == 0) {
			xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
			SAlloc::F(p);
			return -1;
		}
		p->state = XML_TEXTWRITER_NAME;
		xmlListPushFront(writer->nodes, p);
		if(writer->indent) {
			count = xmlTextWriterWriteIndent(writer);
			sum += count;
		}
		count = xmlOutputBufferWriteString(writer->out, "<");
		if(count < 0)
			return -1;
		sum += count;
		count = xmlOutputBufferWriteString(writer->out, (const char*)p->name);
		if(count < 0)
			return -1;
		sum += count;
	}
	return sum;
}
/**
 * xmlTextWriterStartElementNS:
 * @writer:  the xmlTextWriterPtr
 * @prefix:  namespace prefix or NULL
 * @name:  element local name
 * @namespaceURI:  namespace URI or NULL
 *
 * Start an xml element with namespace support.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterStartElementNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI)
{
	const char * _p_func_name = "xmlTextWriterStartElementNS";
	int count;
	int sum = 0;
	xmlChar * buf;
	if((writer == NULL) || (name == NULL) || (*name == '\0'))
		return -1;
	buf = NULL;
	if(prefix != 0) {
		buf = sstrdup(prefix);
		buf = xmlStrcat(buf, BAD_CAST ":");
	}
	buf = xmlStrcat(buf, name);
	count = xmlTextWriterStartElement(writer, buf);
	SAlloc::F(buf);
	if(count < 0)
		return -1;
	sum += count;
	if(namespaceURI != 0) {
		xmlTextWriterNsStackEntry * p = (xmlTextWriterNsStackEntry*)SAlloc::M(sizeof(xmlTextWriterNsStackEntry));
		if(p == 0) {
			xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
			return -1;
		}
		buf = sstrdup(BAD_CAST "xmlns");
		if(prefix != 0) {
			buf = xmlStrcat(buf, BAD_CAST ":");
			buf = xmlStrcat(buf, prefix);
		}
		p->prefix = buf;
		p->uri = sstrdup(namespaceURI);
		if(p->uri == 0) {
			xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
			SAlloc::F(p);
			return -1;
		}
		p->elem = xmlListFront(writer->nodes);
		xmlListPushFront(writer->nsstack, p);
	}
	return sum;
}
/**
 * xmlTextWriterEndElement:
 * @writer:  the xmlTextWriterPtr
 *
 * End the current xml element.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterEndElement(xmlTextWriter * writer)
{
	int count;
	int sum = 0;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL)
		return -1;
	lk = xmlListFront(writer->nodes);
	if(lk == 0) {
		xmlListDelete(writer->nsstack);
		writer->nsstack = NULL;
		return -1;
	}
	p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p == 0) {
		xmlListDelete(writer->nsstack);
		writer->nsstack = NULL;
		return -1;
	}
	switch(p->state) {
		case XML_TEXTWRITER_ATTRIBUTE:
		    count = xmlTextWriterEndAttribute(writer);
		    if(count < 0) {
			    xmlListDelete(writer->nsstack);
			    writer->nsstack = NULL;
			    return -1;
		    }
		    sum += count;
		// @fallthrough
		case XML_TEXTWRITER_NAME:
		    /* Output namespace declarations */
		    count = xmlTextWriterOutputNSDecl(writer);
		    if(count < 0)
			    return -1;
		    sum += count;
		    if(writer->indent) /* next element needs indent */
			    writer->doindent = 1;
		    count = xmlOutputBufferWriteString(writer->out, "/>");
		    if(count < 0)
			    return -1;
		    sum += count;
		    break;
		case XML_TEXTWRITER_TEXT:
		    if(writer->indent && writer->doindent) {
			    count = xmlTextWriterWriteIndent(writer);
			    sum += count;
			    writer->doindent = 1;
		    }
		    else
			    writer->doindent = 1;
		    count = xmlOutputBufferWriteString(writer->out, "</");
		    if(count < 0)
			    return -1;
		    sum += count;
		    count = xmlOutputBufferWriteString(writer->out, (const char*)p->name);
		    if(count < 0)
			    return -1;
		    sum += count;
		    count = xmlOutputBufferWriteString(writer->out, ">");
		    if(count < 0)
			    return -1;
		    sum += count;
		    break;
		default:
		    return -1;
	}
	if(writer->indent) {
		count = xmlOutputBufferWriteString(writer->out, "\n");
		sum += count;
	}
	xmlListPopFront(writer->nodes);
	return sum;
}

/**
 * xmlTextWriterFullEndElement:
 * @writer:  the xmlTextWriterPtr
 *
 * End the current xml element. Writes an end tag even if the element is empty
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterFullEndElement(xmlTextWriter * writer)
{
	int count;
	int sum;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL)
		return -1;
	lk = xmlListFront(writer->nodes);
	if(lk == 0)
		return -1;
	p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p == 0)
		return -1;
	sum = 0;
	switch(p->state) {
		case XML_TEXTWRITER_ATTRIBUTE:
		    count = xmlTextWriterEndAttribute(writer);
		    if(count < 0)
			    return -1;
		    sum += count;
		// @fallthrough
		case XML_TEXTWRITER_NAME:
		    /* Output namespace declarations */
		    count = xmlTextWriterOutputNSDecl(writer);
		    if(count < 0)
			    return -1;
		    sum += count;
		    count = xmlOutputBufferWriteString(writer->out, ">");
		    if(count < 0)
			    return -1;
		    sum += count;
		    if(writer->indent)
			    writer->doindent = 0;
		// @fallthrough
		case XML_TEXTWRITER_TEXT:
		    if((writer->indent) && (writer->doindent)) {
			    count = xmlTextWriterWriteIndent(writer);
			    sum += count;
			    writer->doindent = 1;
		    }
		    else
			    writer->doindent = 1;
		    count = xmlOutputBufferWriteString(writer->out, "</");
		    if(count < 0)
			    return -1;
		    sum += count;
		    count = xmlOutputBufferWriteString(writer->out, (const char*)p->name);
		    if(count < 0)
			    return -1;
		    sum += count;
		    count = xmlOutputBufferWriteString(writer->out, ">");
		    if(count < 0)
			    return -1;
		    sum += count;
		    break;
		default:
		    return -1;
	}
	if(writer->indent) {
		count = xmlOutputBufferWriteString(writer->out, "\n");
		sum += count;
	}
	xmlListPopFront(writer->nodes);
	return sum;
}

/**
 * xmlTextWriterWriteFormatRaw:
 * @writer:  the xmlTextWriterPtr
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write a formatted raw xml text.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatRaw(xmlTextWriter * writer, const char * format, ...)
{
	int rc;
	va_list ap;
	va_start(ap, format);
	rc = xmlTextWriterWriteVFormatRaw(writer, format, ap);
	va_end(ap);
	return rc;
}

/**
 * xmlTextWriterWriteVFormatRaw:
 * @writer:  the xmlTextWriterPtr
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write a formatted raw xml text.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatRaw(xmlTextWriter * writer, const char * format, va_list argptr)
{
	int rc = -1;
	if(writer) {
		xmlChar * buf = xmlTextWriterVSprintf(format, argptr);
		if(buf) {
			rc = xmlTextWriterWriteRaw(writer, buf);
			SAlloc::F(buf);
		}
	}
	return rc;
}
/**
 * xmlTextWriterWriteRawLen:
 * @writer:  the xmlTextWriterPtr
 * @content:  text string
 * @len:  length of the text string
 *
 * Write an xml text.
 * @todo what about entities and special chars??
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int FASTCALL xmlTextWriterWriteRawLen(xmlTextWriter * writer, const xmlChar * content, int len)
{
	const char * _p_func_name = "xmlTextWriterWriteRawLen";
	int    sum  = 0;
	if(!writer) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "invalid writer!");
		sum = -1;
	}
	else if(!content || len < 0) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "invalid content!");
		sum = -1;
	}
	else {
		int    count;
		xmlLink * lk = xmlListFront(writer->nodes);
		if(lk != 0) {
			xmlTextWriterStackEntry * p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
			count = xmlTextWriterHandleStateDependencies(writer, p);
			if(count < 0)
				return -1;
			sum += count;
		}
		if(writer->indent)
			writer->doindent = 0;
		if(content) {
			count = xmlOutputBufferWrite(writer->out, len, (const char*)content);
			if(count < 0)
				return -1;
			sum += count;
		}
	}
	return sum;
}
/**
 * xmlTextWriterWriteRaw:
 * @writer:  the xmlTextWriterPtr
 * @content:  text string
 *
 * Write a raw xml text.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int FASTCALL xmlTextWriterWriteRaw(xmlTextWriter * writer, const xmlChar * content)
{
	return xmlTextWriterWriteRawLen(writer, content, sstrlen(content));
}
/**
 * xmlTextWriterWriteFormatString:
 * @writer:  the xmlTextWriterPtr
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write a formatted xml text.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatString(xmlTextWriter * writer, const char * format, ...)
{
	int rc;
	va_list ap;
	if((writer == NULL) || (format == NULL))
		return -1;
	va_start(ap, format);
	rc = xmlTextWriterWriteVFormatString(writer, format, ap);
	va_end(ap);
	return rc;
}
/**
 * xmlTextWriterWriteVFormatString:
 * @writer:  the xmlTextWriterPtr
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write a formatted xml text.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatString(xmlTextWriter * writer, const char * format, va_list argptr)
{
	int rc;
	xmlChar * buf;
	if((writer == NULL) || (format == NULL))
		return -1;
	buf = xmlTextWriterVSprintf(format, argptr);
	if(!buf)
		return -1;
	rc = xmlTextWriterWriteString(writer, buf);
	SAlloc::F(buf);
	return rc;
}
/**
 * xmlTextWriterWriteString:
 * @writer:  the xmlTextWriterPtr
 * @content:  text string
 *
 * Write an xml text.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int FASTCALL xmlTextWriterWriteString(xmlTextWriter * writer, const xmlChar * content)
{
	int count;
	int sum = 0;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	xmlChar * buf;
	if(!writer || !content)
		return -1;
	buf = (xmlChar*)content;
	lk = xmlListFront(writer->nodes);
	if(lk != 0) {
		p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
		if(p) {
			switch(p->state) {
				case XML_TEXTWRITER_NAME:
				case XML_TEXTWRITER_TEXT:
#if 0
				    buf = NULL;
				    xmlOutputBufferWriteEscape(writer->out, content, 0);
#endif
				    // AHTOXA buf = xmlEncodeSpecialChars(NULL, content);
				    break;
				case XML_TEXTWRITER_ATTRIBUTE:
				    buf = NULL;
				    xmlBufAttrSerializeTxtContent(writer->out->buffer, writer->doc, NULL, content);
				    break;
				default:
				    break;
			}
		}
	}
	if(buf) {
		count = xmlTextWriterWriteRaw(writer, buf);
		if(buf != content) // buf was allocated by us, so free it 
			SAlloc::F(buf);
		if(count < 0)
			return -1;
		sum += count;
	}
	return sum;
}
/**
 * xmlOutputBufferWriteBase64:
 * @out: the xmlOutputBufferPtr
 * @data:   binary data
 * @len:  the number of bytes to encode
 *
 * Write base64 encoded data to an xmlOutputBuffer.
 * Adapted from John Walker's base64.c (http://www.fourmilab.ch/).
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
static int xmlOutputBufferWriteBase64(xmlOutputBuffer * out, int len, const uchar * data)
{
	/*static uchar dtable[64] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};*/
	const char * p_basis = STextConst::Get(STextConst::cBasis64, 0);
	int i = 0;
	int linelen = 0;
	int count;
	int sum = 0;
	if(!out || (len < 0) || !data)
		return -1;
	while(1) {
		uchar igroup[3];
		uchar ogroup[4];
		int c;
		int n;
		igroup[0] = igroup[1] = igroup[2] = 0;
		for(n = 0; n < 3 && i < len; n++, i++) {
			c = data[i];
			igroup[n] = (uchar)c;
		}
		if(n > 0) {
			ogroup[0] = p_basis[igroup[0] >> 2];
			ogroup[1] = p_basis[((igroup[0] & 3) << 4) | (igroup[1] >> 4)];
			ogroup[2] = p_basis[((igroup[1] & 0xF) << 2) | (igroup[2] >> 6)];
			ogroup[3] = p_basis[igroup[2] & 0x3F];
			if(n < 3) {
				ogroup[3] = '=';
				if(n < 2) {
					ogroup[2] = '=';
				}
			}
			if(linelen >= B64LINELEN) {
				count = xmlOutputBufferWrite(out, 2, B64CRLF);
				if(count == -1)
					return -1;
				sum += count;
				linelen = 0;
			}
			count = xmlOutputBufferWrite(out, 4, (const char*)ogroup);
			if(count == -1)
				return -1;
			sum += count;
			linelen += 4;
		}
		if(i >= len)
			break;
	}
	return sum;
}
/**
 * xmlTextWriterWriteBase64:
 * @writer: the xmlTextWriterPtr
 * @data:   binary data
 * @start:  the position within the data of the first byte to encode
 * @len:  the number of bytes to encode
 *
 * Write an base64 encoded xml text.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteBase64(xmlTextWriter * writer, const char * data, int start, int len)
{
	int count;
	int sum;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(!writer || !data || (start < 0) || (len < 0))
		return -1;
	sum = 0;
	lk = xmlListFront(writer->nodes);
	if(lk != 0) {
		p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
		if(p != 0) {
			count = xmlTextWriterHandleStateDependencies(writer, p);
			if(count < 0)
				return -1;
			sum += count;
		}
	}
	if(writer->indent)
		writer->doindent = 0;
	count = xmlOutputBufferWriteBase64(writer->out, len, (uchar*)data + start);
	if(count < 0)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlOutputBufferWriteBinHex:
 * @out: the xmlOutputBufferPtr
 * @data:   binary data
 * @len:  the number of bytes to encode
 *
 * Write hqx encoded data to an xmlOutputBuffer.
 * ::todo
 *
 * Returns the bytes written (may be 0 because of buffering)
 * or -1 in case of error
 */
static int xmlOutputBufferWriteBinHex(xmlOutputBuffer * out, int len, const uchar * data)
{
	int count;
	int sum = 0;
	static char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	int i;
	if(!out || !data || (len < 0)) {
		return -1;
	}
	for(i = 0; i < len; i++) {
		count = xmlOutputBufferWrite(out, 1, (const char*)&hex[data[i] >> 4]);
		if(count == -1)
			return -1;
		sum += count;
		count = xmlOutputBufferWrite(out, 1, (const char*)&hex[data[i] & 0xF]);
		if(count == -1)
			return -1;
		sum += count;
	}
	return sum;
}
/**
 * xmlTextWriterWriteBinHex:
 * @writer: the xmlTextWriterPtr
 * @data:   binary data
 * @start:  the position within the data of the first byte to encode
 * @len:  the number of bytes to encode
 *
 * Write a BinHex encoded xml text.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteBinHex(xmlTextWriter * writer, const char * data, int start, int len)
{
	int sum = 0;
	if(!writer || !data || (start < 0) || (len < 0))
		sum = -1;
	else {
		xmlLink * lk = xmlListFront(writer->nodes);
		int count;
		if(lk != 0) {
			xmlTextWriterStackEntry * p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
			if(p != 0) {
				count = xmlTextWriterHandleStateDependencies(writer, p);
				if(count < 0)
					return -1;
				sum += count;
			}
		}
		if(writer->indent)
			writer->doindent = 0;
		count = xmlOutputBufferWriteBinHex(writer->out, len, (uchar*)data + start);
		if(count < 0)
			return -1;
		sum += count;
	}
	return sum;
}
/**
 * xmlTextWriterStartAttribute:
 * @writer:  the xmlTextWriterPtr
 * @name:  element name
 *
 * Start an xml attribute.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterStartAttribute(xmlTextWriter * writer, const xmlChar * name)
{
	int count;
	int sum = 0;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if((writer == NULL) || (name == NULL) || (*name == '\0'))
		return -1;
	lk = xmlListFront(writer->nodes);
	if(lk == 0)
		return -1;
	p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p == 0)
		return -1;
	switch(p->state) {
		case XML_TEXTWRITER_ATTRIBUTE:
		    count = xmlTextWriterEndAttribute(writer);
		    if(count < 0)
			    return -1;
		    sum += count;
		// @fallthrough
		case XML_TEXTWRITER_NAME:
		    count = xmlOutputBufferWriteString(writer->out, " ");
		    if(count < 0)
			    return -1;
		    sum += count;
		    count =
		    xmlOutputBufferWriteString(writer->out, (const char*)name);
		    if(count < 0)
			    return -1;
		    sum += count;
		    count = xmlOutputBufferWriteString(writer->out, "=");
		    if(count < 0)
			    return -1;
		    sum += count;
		    count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		    if(count < 0)
			    return -1;
		    sum += count;
		    p->state = XML_TEXTWRITER_ATTRIBUTE;
		    break;
		default:
		    return -1;
	}
	return sum;
}
/**
 * xmlTextWriterStartAttributeNS:
 * @writer:  the xmlTextWriterPtr
 * @prefix:  namespace prefix or NULL
 * @name:  element local name
 * @namespaceURI:  namespace URI or NULL
 *
 * Start an xml attribute with namespace support.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterStartAttributeNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI)
{
	const char * _p_func_name = "xmlTextWriterStartAttributeNS";
	int count;
	int sum;
	xmlChar * buf;
	xmlTextWriterNsStackEntry * p;
	if(!writer || isempty(name))
		return -1;
	// Handle namespace first in case of error 
	if(namespaceURI) {
		xmlTextWriterNsStackEntry nsentry, * curns;
		buf = sstrdup(BAD_CAST "xmlns");
		if(prefix != 0) {
			buf = xmlStrcat(buf, BAD_CAST ":");
			buf = xmlStrcat(buf, prefix);
		}
		nsentry.prefix = buf;
		nsentry.uri = (xmlChar*)namespaceURI;
		nsentry.elem = xmlListFront(writer->nodes);
		curns = (xmlTextWriterNsStackEntry*)xmlListSearch(writer->nsstack, (void*)&nsentry);
		if(curns) {
			SAlloc::F(buf);
			if(sstreq(curns->uri, namespaceURI))
				buf = NULL; // Namespace already defined on element skip 
			else
				return -1; // Prefix mismatch so error out 
		}
		// Do not add namespace decl to list - it is already there 
		if(buf) {
			p = (xmlTextWriterNsStackEntry*)SAlloc::M(sizeof(xmlTextWriterNsStackEntry));
			if(p == 0) {
				xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
				return -1;
			}
			p->prefix = buf;
			p->uri = sstrdup(namespaceURI);
			if(p->uri == 0) {
				xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
				SAlloc::F(p);
				return -1;
			}
			p->elem = xmlListFront(writer->nodes);
			xmlListPushFront(writer->nsstack, p);
		}
	}
	buf = NULL;
	if(prefix != 0) {
		buf = sstrdup(prefix);
		buf = xmlStrcat(buf, BAD_CAST ":");
	}
	buf = xmlStrcat(buf, name);
	sum = 0;
	count = xmlTextWriterStartAttribute(writer, buf);
	SAlloc::F(buf);
	if(count < 0)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterEndAttribute:
 * @writer:  the xmlTextWriterPtr
 *
 * End the current xml element.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterEndAttribute(xmlTextWriter * writer)
{
	int count;
	int sum = 0;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL)
		return -1;
	lk = xmlListFront(writer->nodes);
	if(lk == 0) {
		return -1;
	}
	p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p == 0) {
		return -1;
	}
	switch(p->state) {
		case XML_TEXTWRITER_ATTRIBUTE:
		    p->state = XML_TEXTWRITER_NAME;
		    count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		    if(count < 0) {
			    return -1;
		    }
		    sum += count;
		    break;
		default:
		    return -1;
	}
	return sum;
}
/**
 * xmlTextWriterWriteFormatAttribute:
 * @writer:  the xmlTextWriterPtr
 * @name:  attribute name
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write a formatted xml attribute.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatAttribute(xmlTextWriter * writer, const xmlChar * name, const char * format, ...)
{
	int rc;
	va_list ap;
	va_start(ap, format);
	rc = xmlTextWriterWriteVFormatAttribute(writer, name, format, ap);
	va_end(ap);
	return rc;
}
/**
 * xmlTextWriterWriteVFormatAttribute:
 * @writer:  the xmlTextWriterPtr
 * @name:  attribute name
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write a formatted xml attribute.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatAttribute(xmlTextWriter * writer, const xmlChar * name, const char * format, va_list argptr)
{
	int rc;
	xmlChar * buf;
	if(writer == NULL)
		return -1;
	buf = xmlTextWriterVSprintf(format, argptr);
	if(!buf)
		return -1;
	rc = xmlTextWriterWriteAttribute(writer, name, buf);
	SAlloc::F(buf);
	return rc;
}
/**
 * xmlTextWriterWriteAttribute:
 * @writer:  the xmlTextWriterPtr
 * @name:  attribute name
 * @content:  attribute content
 *
 * Write an xml attribute.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteAttribute(xmlTextWriter * writer, const xmlChar * name, const xmlChar * content)
{
	int sum = 0;
	int count = xmlTextWriterStartAttribute(writer, name);
	if(count < 0)
		return -1;
	sum += count;
	count = xmlTextWriterWriteString(writer, content);
	if(count < 0)
		return -1;
	sum += count;
	count = xmlTextWriterEndAttribute(writer);
	if(count < 0)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterWriteFormatAttributeNS:
 * @writer:  the xmlTextWriterPtr
 * @prefix:  namespace prefix
 * @name:  attribute local name
 * @namespaceURI:  namespace URI
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write a formatted xml attribute.with namespace support
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatAttributeNS(xmlTextWriter * writer,
    const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const char * format, ...)
{
	int rc;
	va_list ap;
	va_start(ap, format);
	rc = xmlTextWriterWriteVFormatAttributeNS(writer, prefix, name, namespaceURI, format, ap);
	va_end(ap);
	return rc;
}
/**
 * xmlTextWriterWriteVFormatAttributeNS:
 * @writer:  the xmlTextWriterPtr
 * @prefix:  namespace prefix
 * @name:  attribute local name
 * @namespaceURI:  namespace URI
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write a formatted xml attribute.with namespace support
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatAttributeNS(xmlTextWriter * writer, const xmlChar * prefix,
    const xmlChar * name, const xmlChar * namespaceURI, const char * format, va_list argptr)
{
	int rc = -1;
	if(writer) {
		xmlChar * buf = xmlTextWriterVSprintf(format, argptr);
		if(buf) {
			rc = xmlTextWriterWriteAttributeNS(writer, prefix, name, namespaceURI, buf);
			SAlloc::F(buf);
		}
	}
	return rc;
}
/**
 * xmlTextWriterWriteAttributeNS:
 * @writer:  the xmlTextWriterPtr
 * @prefix:  namespace prefix
 * @name:  attribute local name
 * @namespaceURI:  namespace URI
 * @content:  attribute content
 *
 * Write an xml attribute.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteAttributeNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name,
    const xmlChar * namespaceURI, const xmlChar * content)
{
	int count;
	int sum = 0;
	if(!writer || !name || *name == '\0')
		return -1;
	count = xmlTextWriterStartAttributeNS(writer, prefix, name, namespaceURI);
	if(count < 0)
		return -1;
	sum += count;
	count = xmlTextWriterWriteString(writer, content);
	if(count < 0)
		return -1;
	sum += count;
	count = xmlTextWriterEndAttribute(writer);
	if(count < 0)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterWriteFormatElement:
 * @writer:  the xmlTextWriterPtr
 * @name:  element name
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write a formatted xml element.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatElement(xmlTextWriter * writer, const xmlChar * name, const char * format, ...)
{
	int rc;
	va_list ap;
	va_start(ap, format);
	rc = xmlTextWriterWriteVFormatElement(writer, name, format, ap);
	va_end(ap);
	return rc;
}
/**
 * xmlTextWriterWriteVFormatElement:
 * @writer:  the xmlTextWriterPtr
 * @name:  element name
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write a formatted xml element.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatElement(xmlTextWriter * writer, const xmlChar * name, const char * format, va_list argptr)
{
	int rc = -1;
	if(writer) {
		xmlChar * buf = xmlTextWriterVSprintf(format, argptr);
		if(buf) {
			rc = xmlTextWriterWriteElement(writer, name, buf);
			SAlloc::F(buf);
		}
	}
	return rc;
}
/**
 * xmlTextWriterWriteElement:
 * @writer:  the xmlTextWriterPtr
 * @name:  element name
 * @content:  element content
 *
 * Write an xml element.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteElement(xmlTextWriter * writer, const xmlChar * name, const xmlChar * content)
{
	int sum = 0;
	int count = xmlTextWriterStartElement(writer, name);
	if(count == -1)
		return -1;
	sum += count;
	if(content) {
		count = xmlTextWriterWriteString(writer, content);
		if(count == -1)
			return -1;
		sum += count;
	}
	count = xmlTextWriterEndElement(writer);
	if(count == -1)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterWriteFormatElementNS:
 * @writer:  the xmlTextWriterPtr
 * @prefix:  namespace prefix
 * @name:  element local name
 * @namespaceURI:  namespace URI
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write a formatted xml element with namespace support.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatElementNS(xmlTextWriter * writer, const xmlChar * prefix,
    const xmlChar * name, const xmlChar * namespaceURI, const char * format, ...)
{
	int rc;
	va_list ap;
	va_start(ap, format);
	rc = xmlTextWriterWriteVFormatElementNS(writer, prefix, name, namespaceURI, format, ap);
	va_end(ap);
	return rc;
}
/**
 * xmlTextWriterWriteVFormatElementNS:
 * @writer:  the xmlTextWriterPtr
 * @prefix:  namespace prefix
 * @name:  element local name
 * @namespaceURI:  namespace URI
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write a formatted xml element with namespace support.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatElementNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name,
    const xmlChar * namespaceURI, const char * format, va_list argptr)
{
	int rc = -1;
	if(writer) {
		xmlChar * buf = xmlTextWriterVSprintf(format, argptr);
		if(buf) {
			rc = xmlTextWriterWriteElementNS(writer, prefix, name, namespaceURI, buf);
			SAlloc::F(buf);
		}
	}
	return rc;
}
/**
 * xmlTextWriterWriteElementNS:
 * @writer:  the xmlTextWriterPtr
 * @prefix:  namespace prefix
 * @name:  element local name
 * @namespaceURI:  namespace URI
 * @content:  element content
 *
 * Write an xml element with namespace support.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteElementNS(xmlTextWriter * writer, const xmlChar * prefix, const xmlChar * name, const xmlChar * namespaceURI, const xmlChar * content)
{
	int count;
	int sum;
	if(!writer || isempty(name))
		return -1;
	sum = 0;
	count = xmlTextWriterStartElementNS(writer, prefix, name, namespaceURI);
	if(count < 0)
		return -1;
	sum += count;
	count = xmlTextWriterWriteString(writer, content);
	if(count == -1)
		return -1;
	sum += count;
	count = xmlTextWriterEndElement(writer);
	if(count == -1)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterStartPI:
 * @writer:  the xmlTextWriterPtr
 * @target:  PI target
 *
 * Start an xml PI.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterStartPI(xmlTextWriter * writer, const xmlChar * target)
{
	const char * _p_func_name = "xmlTextWriterStartPI";
	int count;
	int sum;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(!writer || !target || *target == '\0')
		return -1;
	if(sstreqi_ascii(target, (const xmlChar*)"xml")) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "target name [Xx][Mm][Ll] is reserved for xml standardization!");
		return -1;
	}
	sum = 0;
	lk = xmlListFront(writer->nodes);
	if(lk != 0) {
		p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
		if(p != 0) {
			switch(p->state) {
				case XML_TEXTWRITER_ATTRIBUTE:
				    count = xmlTextWriterEndAttribute(writer);
				    if(count < 0)
					    return -1;
				    sum += count;
				// @fallthrough
				case XML_TEXTWRITER_NAME:
				    /* Output namespace declarations */
				    count = xmlTextWriterOutputNSDecl(writer);
				    if(count < 0)
					    return -1;
				    sum += count;
				    count = xmlOutputBufferWriteString(writer->out, ">");
				    if(count < 0)
					    return -1;
				    sum += count;
				    p->state = XML_TEXTWRITER_TEXT;
				    break;
				case XML_TEXTWRITER_NONE:
				case XML_TEXTWRITER_TEXT:
				case XML_TEXTWRITER_DTD:
				    break;
				case XML_TEXTWRITER_PI:
				case XML_TEXTWRITER_PI_TEXT:
				    xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "nested PI!");
				    return -1;
				default:
				    return -1;
			}
		}
	}
	p = (xmlTextWriterStackEntry*)SAlloc::M(sizeof(xmlTextWriterStackEntry));
	if(p == 0) {
		xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		return -1;
	}
	p->name = sstrdup(target);
	if(p->name == 0) {
		xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		SAlloc::F(p);
		return -1;
	}
	p->state = XML_TEXTWRITER_PI;
	xmlListPushFront(writer->nodes, p);
	count = xmlOutputBufferWriteString(writer->out, "<?");
	if(count < 0)
		return -1;
	sum += count;
	count = xmlOutputBufferWriteString(writer->out, (const char*)p->name);
	if(count < 0)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterEndPI:
 * @writer:  the xmlTextWriterPtr
 *
 * End the current xml PI.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterEndPI(xmlTextWriter * writer)
{
	int count;
	int sum;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL)
		return -1;
	lk = xmlListFront(writer->nodes);
	if(lk == 0)
		return 0;
	p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p == 0)
		return 0;
	sum = 0;
	switch(p->state) {
		case XML_TEXTWRITER_PI:
		case XML_TEXTWRITER_PI_TEXT:
		    count = xmlOutputBufferWriteString(writer->out, "?>");
		    if(count < 0)
			    return -1;
		    sum += count;
		    break;
		default:
		    return -1;
	}
	if(writer->indent) {
		count = xmlOutputBufferWriteString(writer->out, "\n");
		if(count < 0)
			return -1;
		sum += count;
	}
	xmlListPopFront(writer->nodes);
	return sum;
}
/**
 * xmlTextWriterWriteFormatPI:
 * @writer:  the xmlTextWriterPtr
 * @target:  PI target
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write a formatted PI.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatPI(xmlTextWriter * writer, const xmlChar * target, const char * format, ...)
{
	int rc;
	va_list ap;
	va_start(ap, format);
	rc = xmlTextWriterWriteVFormatPI(writer, target, format, ap);
	va_end(ap);
	return rc;
}
/**
 * xmlTextWriterWriteVFormatPI:
 * @writer:  the xmlTextWriterPtr
 * @target:  PI target
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write a formatted xml PI.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatPI(xmlTextWriter * writer, const xmlChar * target, const char * format, va_list argptr)
{
	int rc;
	xmlChar * buf;
	if(writer == NULL)
		return -1;
	buf = xmlTextWriterVSprintf(format, argptr);
	if(!buf)
		return -1;
	rc = xmlTextWriterWritePI(writer, target, buf);
	SAlloc::F(buf);
	return rc;
}
/**
 * xmlTextWriterWritePI:
 * @writer:  the xmlTextWriterPtr
 * @target:  PI target
 * @content:  PI content
 *
 * Write an xml PI.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWritePI(xmlTextWriter * writer, const xmlChar * target, const xmlChar * content)
{
	int sum = 0;
	int count = xmlTextWriterStartPI(writer, target);
	if(count == -1)
		return -1;
	sum += count;
	if(content != 0) {
		count = xmlTextWriterWriteString(writer, content);
		if(count == -1)
			return -1;
		sum += count;
	}
	count = xmlTextWriterEndPI(writer);
	if(count == -1)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterStartCDATA:
 * @writer:  the xmlTextWriterPtr
 *
 * Start an xml CDATA section.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterStartCDATA(xmlTextWriter * writer)
{
	const char * _p_func_name = "xmlTextWriterStartCDATA";
	int count;
	int sum;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL)
		return -1;
	sum = 0;
	lk = xmlListFront(writer->nodes);
	if(lk != 0) {
		p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
		if(p != 0) {
			switch(p->state) {
				case XML_TEXTWRITER_NONE:
				case XML_TEXTWRITER_TEXT:
				case XML_TEXTWRITER_PI:
				case XML_TEXTWRITER_PI_TEXT:
				    break;
				case XML_TEXTWRITER_ATTRIBUTE:
				    count = xmlTextWriterEndAttribute(writer);
				    if(count < 0)
					    return -1;
				    sum += count;
				// @fallthrough
				case XML_TEXTWRITER_NAME:
				    /* Output namespace declarations */
				    count = xmlTextWriterOutputNSDecl(writer);
				    if(count < 0)
					    return -1;
				    sum += count;
				    count = xmlOutputBufferWriteString(writer->out, ">");
				    if(count < 0)
					    return -1;
				    sum += count;
				    p->state = XML_TEXTWRITER_TEXT;
				    break;
				case XML_TEXTWRITER_CDATA:
				    xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "CDATA not allowed in this context!");
				    return -1;
				default:
				    return -1;
			}
		}
	}
	p = (xmlTextWriterStackEntry*)SAlloc::M(sizeof(xmlTextWriterStackEntry));
	if(p == 0) {
		xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		return -1;
	}
	p->name = NULL;
	p->state = XML_TEXTWRITER_CDATA;
	xmlListPushFront(writer->nodes, p);
	count = xmlOutputBufferWriteString(writer->out, "<![CDATA[");
	if(count < 0)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterEndCDATA:
 * @writer:  the xmlTextWriterPtr
 *
 * End an xml CDATA section.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterEndCDATA(xmlTextWriter * writer)
{
	int count;
	int sum;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL)
		return -1;
	lk = xmlListFront(writer->nodes);
	if(lk == 0)
		return -1;
	p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p == 0)
		return -1;
	sum = 0;
	switch(p->state) {
		case XML_TEXTWRITER_CDATA:
		    count = xmlOutputBufferWriteString(writer->out, "]]>");
		    if(count < 0)
			    return -1;
		    sum += count;
		    break;
		default:
		    return -1;
	}
	xmlListPopFront(writer->nodes);
	return sum;
}
/**
 * xmlTextWriterWriteFormatCDATA:
 * @writer:  the xmlTextWriterPtr
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write a formatted xml CDATA.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatCDATA(xmlTextWriter * writer, const char * format, ...)
{
	va_list ap;
	va_start(ap, format);
	int rc = xmlTextWriterWriteVFormatCDATA(writer, format, ap);
	va_end(ap);
	return rc;
}
/**
 * xmlTextWriterWriteVFormatCDATA:
 * @writer:  the xmlTextWriterPtr
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write a formatted xml CDATA.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatCDATA(xmlTextWriter * writer, const char * format, va_list argptr)
{
	int rc;
	xmlChar * buf;
	if(writer == NULL)
		return -1;
	buf = xmlTextWriterVSprintf(format, argptr);
	if(!buf)
		return -1;
	rc = xmlTextWriterWriteCDATA(writer, buf);
	SAlloc::F(buf);
	return rc;
}

/**
 * xmlTextWriterWriteCDATA:
 * @writer:  the xmlTextWriterPtr
 * @content:  CDATA content
 *
 * Write an xml CDATA.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteCDATA(xmlTextWriter * writer, const xmlChar * content)
{
	int sum = 0;
	int count = xmlTextWriterStartCDATA(writer);
	if(count == -1)
		return -1;
	sum += count;
	if(content != 0) {
		count = xmlTextWriterWriteString(writer, content);
		if(count == -1)
			return -1;
		sum += count;
	}
	count = xmlTextWriterEndCDATA(writer);
	if(count == -1)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterStartDTD:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the DTD
 * @pubid:  the public identifier, which is an alternative to the system identifier
 * @sysid:  the system identifier, which is the URI of the DTD
 *
 * Start an xml DTD.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterStartDTD(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid)
{
	const char * _p_func_name = "xmlTextWriterStartDTD";
	int    count = 0;
	int    sum = 0;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	THROW(writer && !isempty(name));
	lk = xmlListFront(writer->nodes);
	if(lk && xmlLinkGetData(lk)) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "DTD allowed only in prolog!");
		CALLEXCEPT();
	}
	p = (xmlTextWriterStackEntry*)SAlloc::M(sizeof(xmlTextWriterStackEntry));
	if(p == 0) {
		xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		CALLEXCEPT();
	}
	p->name = sstrdup(name);
	if(p->name == 0) {
		xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		SAlloc::F(p);
		CALLEXCEPT();
	}
	p->state = XML_TEXTWRITER_DTD;
	xmlListPushFront(writer->nodes, p);
	count = xmlOutputBufferWriteString(writer->out, "<!DOCTYPE ");
	THROW(count >= 0);
	sum += count;
	count = xmlOutputBufferWriteString(writer->out, (const char*)name);
	THROW(count >= 0);
	sum += count;
	if(pubid != 0) {
		if(sysid == 0) {
			xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "system identifier needed!");
			CALLEXCEPT();
		}
		count = xmlOutputBufferWrite(writer->out, 1, writer->indent ? "\n" : " ");
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWriteString(writer->out, "PUBLIC ");
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWriteString(writer->out, (const char*)pubid);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
	}
	if(sysid != 0) {
		if(pubid == 0) {
			count = xmlOutputBufferWrite(writer->out, 1, writer->indent ? "\n" : " ");
			THROW(count >= 0);
			sum += count;
			count = xmlOutputBufferWriteString(writer->out, "SYSTEM ");
			THROW(count >= 0);
			sum += count;
		}
		else {
			count = xmlOutputBufferWriteString(writer->out, writer->indent ? "\n       " : " ");
			THROW(count >= 0);
			sum += count;
		}
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWriteString(writer->out, (const char*)sysid);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
	}
	CATCH
		sum = -1;
	ENDCATCH
	return sum;
}
/**
 * xmlTextWriterEndDTD:
 * @writer:  the xmlTextWriterPtr
 *
 * End an xml DTD.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterEndDTD(xmlTextWriter * writer)
{
	int loop;
	int count;
	int sum;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL)
		return -1;
	sum = 0;
	loop = 1;
	while(loop) {
		lk = xmlListFront(writer->nodes);
		if(lk == NULL)
			break;
		p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
		if(p == 0)
			break;
		switch(p->state) {
			case XML_TEXTWRITER_DTD_TEXT:
			    count = xmlOutputBufferWriteString(writer->out, "]");
			    if(count < 0)
				    return -1;
			    sum += count;
			// @fallthrough
			case XML_TEXTWRITER_DTD:
			    count = xmlOutputBufferWriteString(writer->out, ">");
			    if(writer->indent) {
				    if(count < 0)
					    return -1;
				    sum += count;
				    count = xmlOutputBufferWriteString(writer->out, "\n");
			    }
			    xmlListPopFront(writer->nodes);
			    break;
			case XML_TEXTWRITER_DTD_ELEM:
			case XML_TEXTWRITER_DTD_ELEM_TEXT:
			    count = xmlTextWriterEndDTDElement(writer);
			    break;
			case XML_TEXTWRITER_DTD_ATTL:
			case XML_TEXTWRITER_DTD_ATTL_TEXT:
			    count = xmlTextWriterEndDTDAttlist(writer);
			    break;
			case XML_TEXTWRITER_DTD_ENTY:
			case XML_TEXTWRITER_DTD_PENT:
			case XML_TEXTWRITER_DTD_ENTY_TEXT:
			    count = xmlTextWriterEndDTDEntity(writer);
			    break;
			case XML_TEXTWRITER_COMMENT:
			    count = xmlTextWriterEndComment(writer);
			    break;
			default:
			    loop = 0;
			    continue;
		}
		if(count < 0)
			return -1;
		sum += count;
	}
	return sum;
}
/**
 * xmlTextWriterWriteFormatDTD:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the DTD
 * @pubid:  the public identifier, which is an alternative to the system identifier
 * @sysid:  the system identifier, which is the URI of the DTD
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write a DTD with a formatted markup declarations part.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatDTD(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const char * format, ...)
{
	int rc;
	va_list ap;
	va_start(ap, format);
	rc = xmlTextWriterWriteVFormatDTD(writer, name, pubid, sysid, format, ap);
	va_end(ap);
	return rc;
}
/**
 * xmlTextWriterWriteVFormatDTD:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the DTD
 * @pubid:  the public identifier, which is an alternative to the system identifier
 * @sysid:  the system identifier, which is the URI of the DTD
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write a DTD with a formatted markup declarations part.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatDTD(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const char * format, va_list argptr)
{
	int rc = -1;
	if(writer) {
		xmlChar * buf = xmlTextWriterVSprintf(format, argptr);
		if(buf) {
			rc = xmlTextWriterWriteDTD(writer, name, pubid, sysid, buf);
			SAlloc::F(buf);
		}
	}
	return rc;
}

/**
 * xmlTextWriterWriteDTD:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the DTD
 * @pubid:  the public identifier, which is an alternative to the system identifier
 * @sysid:  the system identifier, which is the URI of the DTD
 * @subset:  string content of the DTD
 *
 * Write a DTD.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteDTD(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const xmlChar * subset)
{
	int sum = 0;
	int count = xmlTextWriterStartDTD(writer, name, pubid, sysid);
	if(count == -1)
		return -1;
	sum += count;
	if(subset != 0) {
		count = xmlTextWriterWriteString(writer, subset);
		if(count == -1)
			return -1;
		sum += count;
	}
	count = xmlTextWriterEndDTD(writer);
	if(count == -1)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterStartDTDElement:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the DTD element
 *
 * Start an xml DTD element.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterStartDTDElement(xmlTextWriter * writer, const xmlChar * name)
{
	const char * _p_func_name = "xmlTextWriterStartDTDElement";
	int count;
	int sum = 0;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(!writer || isempty(name))
		return -1;
	lk = xmlListFront(writer->nodes);
	if(lk == 0) {
		return -1;
	}
	p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p != 0) {
		switch(p->state) {
			case XML_TEXTWRITER_DTD:
			    count = xmlOutputBufferWriteString(writer->out, " [");
			    if(count < 0)
				    return -1;
			    sum += count;
			    if(writer->indent) {
				    count = xmlOutputBufferWriteString(writer->out, "\n");
				    if(count < 0)
					    return -1;
				    sum += count;
			    }
			    p->state = XML_TEXTWRITER_DTD_TEXT;
			// @fallthrough
			case XML_TEXTWRITER_DTD_TEXT:
			case XML_TEXTWRITER_NONE:
			    break;
			default:
			    return -1;
		}
	}
	p = (xmlTextWriterStackEntry*)SAlloc::M(sizeof(xmlTextWriterStackEntry));
	if(p == 0) {
		xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		return -1;
	}
	p->name = sstrdup(name);
	if(p->name == 0) {
		xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		SAlloc::F(p);
		return -1;
	}
	p->state = XML_TEXTWRITER_DTD_ELEM;
	xmlListPushFront(writer->nodes, p);
	if(writer->indent) {
		count = xmlTextWriterWriteIndent(writer);
		if(count < 0)
			return -1;
		sum += count;
	}
	count = xmlOutputBufferWriteString(writer->out, "<!ELEMENT ");
	if(count < 0)
		return -1;
	sum += count;
	count = xmlOutputBufferWriteString(writer->out, (const char*)name);
	if(count < 0)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterEndDTDElement:
 * @writer:  the xmlTextWriterPtr
 *
 * End an xml DTD element.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterEndDTDElement(xmlTextWriter * writer)
{
	int count;
	int sum = 0;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL)
		return -1;
	lk = xmlListFront(writer->nodes);
	if(lk == 0)
		return -1;
	p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p == 0)
		return -1;
	switch(p->state) {
		case XML_TEXTWRITER_DTD_ELEM:
		case XML_TEXTWRITER_DTD_ELEM_TEXT:
		    count = xmlOutputBufferWriteString(writer->out, ">");
		    if(count < 0)
			    return -1;
		    sum += count;
		    break;
		default:
		    return -1;
	}

	if(writer->indent) {
		count = xmlOutputBufferWriteString(writer->out, "\n");
		if(count < 0)
			return -1;
		sum += count;
	}
	xmlListPopFront(writer->nodes);
	return sum;
}
/**
 * xmlTextWriterWriteFormatDTDElement:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the DTD element
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write a formatted DTD element.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatDTDElement(xmlTextWriter * writer, const xmlChar * name, const char * format, ...)
{
	int rc;
	va_list ap;
	va_start(ap, format);
	rc = xmlTextWriterWriteVFormatDTDElement(writer, name, format, ap);
	va_end(ap);
	return rc;
}
/**
 * xmlTextWriterWriteVFormatDTDElement:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the DTD element
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write a formatted DTD element.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatDTDElement(xmlTextWriter * writer, const xmlChar * name, const char * format, va_list argptr)
{
	int rc = -1;
	if(writer) {
		xmlChar * buf = xmlTextWriterVSprintf(format, argptr);
		if(buf) {
			rc = xmlTextWriterWriteDTDElement(writer, name, buf);
			SAlloc::F(buf);
		}
	}
	return rc;
}

/**
 * xmlTextWriterWriteDTDElement:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the DTD element
 * @content:  content of the element
 *
 * Write a DTD element.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteDTDElement(xmlTextWriter * writer, const xmlChar * name, const xmlChar * content)
{
	int count;
	int sum = 0;
	if(content == NULL)
		return -1;
	count = xmlTextWriterStartDTDElement(writer, name);
	if(count == -1)
		return -1;
	sum += count;
	count = xmlTextWriterWriteString(writer, content);
	if(count == -1)
		return -1;
	sum += count;
	count = xmlTextWriterEndDTDElement(writer);
	if(count == -1)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterStartDTDAttlist:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the DTD ATTLIST
 *
 * Start an xml DTD ATTLIST.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterStartDTDAttlist(xmlTextWriter * writer, const xmlChar * name)
{
	const char * _p_func_name = "xmlTextWriterStartDTDAttlist";
	int sum = 0;
	int count;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL || name == NULL || *name == '\0')
		return -1;
	lk = xmlListFront(writer->nodes);
	if(lk == 0) {
		return -1;
	}
	p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p != 0) {
		switch(p->state) {
			case XML_TEXTWRITER_DTD:
			    count = xmlOutputBufferWriteString(writer->out, " [");
			    if(count < 0)
				    return -1;
			    sum += count;
			    if(writer->indent) {
				    count = xmlOutputBufferWriteString(writer->out, "\n");
				    if(count < 0)
					    return -1;
				    sum += count;
			    }
			    p->state = XML_TEXTWRITER_DTD_TEXT;
			// @fallthrough
			case XML_TEXTWRITER_DTD_TEXT:
			case XML_TEXTWRITER_NONE:
			    break;
			default:
			    return -1;
		}
	}
	p = (xmlTextWriterStackEntry*)SAlloc::M(sizeof(xmlTextWriterStackEntry));
	if(p == 0) {
		xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		return -1;
	}
	p->name = sstrdup(name);
	if(p->name == 0) {
		xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		SAlloc::F(p);
		return -1;
	}
	p->state = XML_TEXTWRITER_DTD_ATTL;
	xmlListPushFront(writer->nodes, p);
	if(writer->indent) {
		count = xmlTextWriterWriteIndent(writer);
		if(count < 0)
			return -1;
		sum += count;
	}
	count = xmlOutputBufferWriteString(writer->out, "<!ATTLIST ");
	if(count < 0)
		return -1;
	sum += count;
	count = xmlOutputBufferWriteString(writer->out, (const char*)name);
	if(count < 0)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterEndDTDAttlist:
 * @writer:  the xmlTextWriterPtr
 *
 * End an xml DTD attribute list.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterEndDTDAttlist(xmlTextWriter * writer)
{
	int sum = 0;
	int count;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL)
		return -1;
	lk = xmlListFront(writer->nodes);
	if(lk == 0)
		return -1;
	p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p == 0)
		return -1;
	switch(p->state) {
		case XML_TEXTWRITER_DTD_ATTL:
		case XML_TEXTWRITER_DTD_ATTL_TEXT:
		    count = xmlOutputBufferWriteString(writer->out, ">");
		    if(count < 0)
			    return -1;
		    sum += count;
		    break;
		default:
		    return -1;
	}
	if(writer->indent) {
		count = xmlOutputBufferWriteString(writer->out, "\n");
		if(count < 0)
			return -1;
		sum += count;
	}
	xmlListPopFront(writer->nodes);
	return sum;
}
/**
 * xmlTextWriterWriteFormatDTDAttlist:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the DTD ATTLIST
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write a formatted DTD ATTLIST.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatDTDAttlist(xmlTextWriter * writer, const xmlChar * name, const char * format, ...)
{
	int rc;
	va_list ap;
	va_start(ap, format);
	rc = xmlTextWriterWriteVFormatDTDAttlist(writer, name, format, ap);
	va_end(ap);
	return rc;
}
/**
 * xmlTextWriterWriteVFormatDTDAttlist:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the DTD ATTLIST
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write a formatted DTD ATTLIST.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatDTDAttlist(xmlTextWriter * writer, const xmlChar * name, const char * format, va_list argptr)
{
	int rc = -1;
	if(writer) {
		xmlChar * buf = xmlTextWriterVSprintf(format, argptr);
		if(buf) {
			rc = xmlTextWriterWriteDTDAttlist(writer, name, buf);
			SAlloc::F(buf);
		}
	}
	return rc;
}
/**
 * xmlTextWriterWriteDTDAttlist:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the DTD ATTLIST
 * @content:  content of the ATTLIST
 *
 * Write a DTD ATTLIST.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteDTDAttlist(xmlTextWriter * writer, const xmlChar * name, const xmlChar * content)
{
	int sum = 0;
	int count;
	if(content == NULL)
		return -1;
	count = xmlTextWriterStartDTDAttlist(writer, name);
	if(count == -1)
		return -1;
	sum += count;
	count = xmlTextWriterWriteString(writer, content);
	if(count == -1)
		return -1;
	sum += count;
	count = xmlTextWriterEndDTDAttlist(writer);
	if(count == -1)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterStartDTDEntity:
 * @writer:  the xmlTextWriterPtr
 * @pe:  TRUE if this is a parameter entity, FALSE if not
 * @name:  the name of the DTD ATTLIST
 *
 * Start an xml DTD ATTLIST.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterStartDTDEntity(xmlTextWriter * writer, int pe, const xmlChar * name)
{
	const char * _p_func_name = "xmlTextWriterStartDTDEntity";
	int count;
	int sum = 0;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(!writer || isempty(name))
		return -1;
	lk = xmlListFront(writer->nodes);
	if(lk != 0) {
		p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
		if(p != 0) {
			switch(p->state) {
				case XML_TEXTWRITER_DTD:
				    count = xmlOutputBufferWriteString(writer->out, " [");
				    if(count < 0)
					    return -1;
				    sum += count;
				    if(writer->indent) {
					    count = xmlOutputBufferWriteString(writer->out, "\n");
					    if(count < 0)
						    return -1;
					    sum += count;
				    }
				    p->state = XML_TEXTWRITER_DTD_TEXT;
				// @fallthrough
				case XML_TEXTWRITER_DTD_TEXT:
				case XML_TEXTWRITER_NONE:
				    break;
				default:
				    return -1;
			}
		}
	}
	p = (xmlTextWriterStackEntry*)SAlloc::M(sizeof(xmlTextWriterStackEntry));
	if(p == 0) {
		xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		return -1;
	}
	p->name = sstrdup(name);
	if(p->name == 0) {
		xmlWriterErrMsg(writer, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
		SAlloc::F(p);
		return -1;
	}
	p->state = pe ? XML_TEXTWRITER_DTD_PENT : XML_TEXTWRITER_DTD_ENTY;
	xmlListPushFront(writer->nodes, p);
	if(writer->indent) {
		count = xmlTextWriterWriteIndent(writer);
		if(count < 0)
			return -1;
		sum += count;
	}
	count = xmlOutputBufferWriteString(writer->out, "<!ENTITY ");
	if(count < 0)
		return -1;
	sum += count;
	if(pe != 0) {
		count = xmlOutputBufferWriteString(writer->out, "% ");
		if(count < 0)
			return -1;
		sum += count;
	}
	count = xmlOutputBufferWriteString(writer->out, (const char*)name);
	if(count < 0)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterEndDTDEntity:
 * @writer:  the xmlTextWriterPtr
 *
 * End an xml DTD entity.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterEndDTDEntity(xmlTextWriter * writer)
{
	int count;
	int sum = 0;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(writer == NULL)
		return -1;
	lk = xmlListFront(writer->nodes);
	if(lk == 0)
		return -1;
	p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p == 0)
		return -1;
	switch(p->state) {
		case XML_TEXTWRITER_DTD_ENTY_TEXT:
		    count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		    if(count < 0)
			    return -1;
		    sum += count;
		case XML_TEXTWRITER_DTD_ENTY:
		case XML_TEXTWRITER_DTD_PENT:
		    count = xmlOutputBufferWriteString(writer->out, ">");
		    if(count < 0)
			    return -1;
		    sum += count;
		    break;
		default:
		    return -1;
	}

	if(writer->indent) {
		count = xmlOutputBufferWriteString(writer->out, "\n");
		if(count < 0)
			return -1;
		sum += count;
	}
	xmlListPopFront(writer->nodes);
	return sum;
}
/**
 * xmlTextWriterWriteFormatDTDInternalEntity:
 * @writer:  the xmlTextWriterPtr
 * @pe:  TRUE if this is a parameter entity, FALSE if not
 * @name:  the name of the DTD entity
 * @format:  format string (see printf)
 * @...:  extra parameters for the format
 *
 * Write a formatted DTD internal entity.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int XMLCDECL xmlTextWriterWriteFormatDTDInternalEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const char * format, ...)
{
	int rc;
	va_list ap;
	va_start(ap, format);
	rc = xmlTextWriterWriteVFormatDTDInternalEntity(writer, pe, name, format, ap);
	va_end(ap);
	return rc;
}
/**
 * xmlTextWriterWriteVFormatDTDInternalEntity:
 * @writer:  the xmlTextWriterPtr
 * @pe:  TRUE if this is a parameter entity, FALSE if not
 * @name:  the name of the DTD entity
 * @format:  format string (see printf)
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Write a formatted DTD internal entity.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteVFormatDTDInternalEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const char * format, va_list argptr)
{
	int rc;
	xmlChar * buf;
	if(writer == NULL)
		return -1;
	buf = xmlTextWriterVSprintf(format, argptr);
	if(!buf)
		return -1;
	rc = xmlTextWriterWriteDTDInternalEntity(writer, pe, name, buf);
	SAlloc::F(buf);
	return rc;
}
/**
 * xmlTextWriterWriteDTDEntity:
 * @writer:  the xmlTextWriterPtr
 * @pe:  TRUE if this is a parameter entity, FALSE if not
 * @name:  the name of the DTD entity
 * @pubid:  the public identifier, which is an alternative to the system identifier
 * @sysid:  the system identifier, which is the URI of the DTD
 * @ndataid:  the xml notation name.
 * @content:  content of the entity
 *
 * Write a DTD entity.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteDTDEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const xmlChar * ndataid, const xmlChar * content)
{
	if(!content && !pubid && !sysid)
		return -1;
	if(pe && ndataid)
		return -1;
	return (!pubid && !sysid) ? xmlTextWriterWriteDTDInternalEntity(writer, pe, name, content) : xmlTextWriterWriteDTDExternalEntity(writer, pe, name, pubid, sysid, ndataid);
}
/**
 * xmlTextWriterWriteDTDInternalEntity:
 * @writer:  the xmlTextWriterPtr
 * @pe:  TRUE if this is a parameter entity, FALSE if not
 * @name:  the name of the DTD entity
 * @content:  content of the entity
 *
 * Write a DTD internal entity.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteDTDInternalEntity(xmlTextWriter * writer, int pe, const xmlChar * name, const xmlChar * content)
{
	int count;
	int sum = 0;
	if(isempty(name) || !content)
		return -1;
	count = xmlTextWriterStartDTDEntity(writer, pe, name);
	if(count == -1)
		return -1;
	sum += count;
	count = xmlTextWriterWriteString(writer, content);
	if(count == -1)
		return -1;
	sum += count;
	count = xmlTextWriterEndDTDEntity(writer);
	if(count == -1)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterWriteDTDExternalEntity:
 * @writer:  the xmlTextWriterPtr
 * @pe:  TRUE if this is a parameter entity, FALSE if not
 * @name:  the name of the DTD entity
 * @pubid:  the public identifier, which is an alternative to the system identifier
 * @sysid:  the system identifier, which is the URI of the DTD
 * @ndataid:  the xml notation name.
 *
 * Write a DTD external entity. The entity must have been started with xmlTextWriterStartDTDEntity
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteDTDExternalEntity(xmlTextWriter * writer, int pe,
    const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid, const xmlChar * ndataid)
{
	int count;
	int sum = 0;
	if(!pubid && !sysid)
		return -1;
	if(pe && ndataid)
		return -1;
	count = xmlTextWriterStartDTDEntity(writer, pe, name);
	if(count == -1)
		return -1;
	sum += count;
	count = xmlTextWriterWriteDTDExternalEntityContents(writer, pubid, sysid, ndataid);
	if(count < 0)
		return -1;
	sum += count;
	count = xmlTextWriterEndDTDEntity(writer);
	if(count == -1)
		return -1;
	sum += count;
	return sum;
}
/**
 * xmlTextWriterWriteDTDExternalEntityContents:
 * @writer:  the xmlTextWriterPtr
 * @pubid:  the public identifier, which is an alternative to the system identifier
 * @sysid:  the system identifier, which is the URI of the DTD
 * @ndataid:  the xml notation name.
 *
 * Write the contents of a DTD external entity.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteDTDExternalEntityContents(xmlTextWriter * writer, const xmlChar * pubid, const xmlChar * sysid, const xmlChar * ndataid)
{
	const char * _p_func_name = "xmlTextWriterWriteDTDExternalEntityContents";
	int count;
	int sum = 0;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	if(!writer) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "xmlTextWriterPtr invalid!");
		CALLEXCEPT();
	}
	lk = xmlListFront(writer->nodes);
	if(!lk) {
		xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "you must call xmlTextWriterStartDTDEntity before the call to this function!");
		CALLEXCEPT();
	}
	THROW(p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk));
	switch(p->state) {
		case XML_TEXTWRITER_DTD_ENTY:
		    break;
		case XML_TEXTWRITER_DTD_PENT:
		    if(ndataid) {
			    xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "notation not allowed with parameter entities!");
				CALLEXCEPT();
		    }
		    break;
		default:
		    xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "you must call xmlTextWriterStartDTDEntity before the call to this function!");
			CALLEXCEPT();
	}
	if(pubid) {
		if(sysid == 0) {
			xmlWriterErrMsg(writer, XML_ERR_INTERNAL_ERROR, _p_func_name, "system identifier needed!");
			CALLEXCEPT();
		}
		count = xmlOutputBufferWriteString(writer->out, " PUBLIC ");
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWriteString(writer->out, (const char*)pubid);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
	}
	if(sysid) {
		if(pubid == 0) {
			count = xmlOutputBufferWriteString(writer->out, " SYSTEM");
			THROW(count >= 0);
			sum += count;
		}
		count = xmlOutputBufferWriteString(writer->out, " ");
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWriteString(writer->out, (const char*)sysid);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
	}
	if(ndataid) {
		count = xmlOutputBufferWriteString(writer->out, " NDATA ");
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWriteString(writer->out, (const char*)ndataid);
		THROW(count >= 0);
		sum += count;
	}
	CATCH
		sum = -1;
	ENDCATCH
	return sum;
}
/**
 * xmlTextWriterWriteDTDNotation:
 * @writer:  the xmlTextWriterPtr
 * @name:  the name of the xml notation
 * @pubid:  the public identifier, which is an alternative to the system identifier
 * @sysid:  the system identifier, which is the URI of the DTD
 *
 * Write a DTD entity.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterWriteDTDNotation(xmlTextWriter * writer, const xmlChar * name, const xmlChar * pubid, const xmlChar * sysid)
{
	int sum = 0;
	int count;
	xmlLink * lk;
	xmlTextWriterStackEntry * p;
	THROW(writer && !isempty(name));
	THROW(lk = xmlListFront(writer->nodes));
	p = (xmlTextWriterStackEntry *)xmlLinkGetData(lk);
	if(p != 0) {
		switch(p->state) {
			case XML_TEXTWRITER_DTD:
			    count = xmlOutputBufferWriteString(writer->out, " [");
				THROW(count >= 0);
			    sum += count;
			    if(writer->indent) {
				    count = xmlOutputBufferWriteString(writer->out, "\n");
					THROW(count >= 0);
				    sum += count;
			    }
			    p->state = XML_TEXTWRITER_DTD_TEXT;
			// @fallthrough
			case XML_TEXTWRITER_DTD_TEXT:
			    break;
			default:
			    return -1;
		}
	}
	if(writer->indent) {
		count = xmlTextWriterWriteIndent(writer);
		THROW(count >= 0);
		sum += count;
	}
	count = xmlOutputBufferWriteString(writer->out, "<!NOTATION ");
	THROW(count >= 0);
	sum += count;
	count = xmlOutputBufferWriteString(writer->out, (const char*)name);
	THROW(count >= 0);
	sum += count;
	if(pubid != 0) {
		count = xmlOutputBufferWriteString(writer->out, " PUBLIC ");
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWriteString(writer->out, (const char*)pubid);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
	}
	if(sysid != 0) {
		if(pubid == 0) {
			count = xmlOutputBufferWriteString(writer->out, " SYSTEM");
			THROW(count >= 0);
			sum += count;
		}
		count = xmlOutputBufferWriteString(writer->out, " ");
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWriteString(writer->out, (const char*)sysid);
		THROW(count >= 0);
		sum += count;
		count = xmlOutputBufferWrite(writer->out, 1, &writer->qchar);
		THROW(count >= 0);
		sum += count;
	}
	count = xmlOutputBufferWriteString(writer->out, ">");
	THROW(count >= 0);
	sum += count;
	CATCH
		sum = -1;
	ENDCATCH
	return sum;
}
/**
 * xmlTextWriterFlush:
 * @writer:  the xmlTextWriterPtr
 *
 * Flush the output buffer.
 *
 * Returns the bytes written (may be 0 because of buffering) or -1 in case of error
 */
int xmlTextWriterFlush(xmlTextWriter * writer)
{
	int count;
	if(writer == NULL)
		count = -1;
	else if(writer->out == NULL)
		count = 0;
	else
		count = xmlOutputBufferFlush(writer->out);
	return count;
}
/**
 * misc
 */
/**
 * xmlFreeTextWriterStackEntry:
 * @lk:  the xmlLinkPtr
 *
 * Free callback for the xmlList.
 */
static void xmlFreeTextWriterStackEntry(xmlLink * lk)
{
	xmlTextWriterStackEntry * p = (xmlTextWriterStackEntry*)xmlLinkGetData(lk);
	if(p) {
		SAlloc::F(p->name);
		SAlloc::F(p);
	}
}
/**
 * xmlCmpTextWriterStackEntry:
 * @data0:  the first data
 * @data1:  the second data
 *
 * Compare callback for the xmlList.
 *
 * Returns -1, 0, 1
 */
static int xmlCmpTextWriterStackEntry(const void * data0, const void * data1)
{
	if(data0 == data1)
		return 0;
	else if(data0 == 0)
		return -1;
	else if(data1 == 0)
		return 1;
	else
		return xmlStrcmp(((xmlTextWriterStackEntry*)data0)->name, ((xmlTextWriterStackEntry*)data1)->name);
}
/**
 * misc
 */
/**
 * xmlTextWriterOutputNSDecl:
 * @writer:  the xmlTextWriterPtr
 *
 * Output the current namespace declarations.
 */
static int xmlTextWriterOutputNSDecl(xmlTextWriter * writer)
{
	int sum = 0;
	while(!xmlListEmpty(writer->nsstack)) {
		xmlChar * namespaceURI = NULL;
		xmlChar * prefix = NULL;
		xmlLink * lk = xmlListFront(writer->nsstack);
		xmlTextWriterNsStackEntry * np = (xmlTextWriterNsStackEntry*)xmlLinkGetData(lk);
		if(np != 0) {
			namespaceURI = sstrdup(np->uri);
			prefix = sstrdup(np->prefix);
		}
		xmlListPopFront(writer->nsstack);
		if(np != 0) {
			int count = xmlTextWriterWriteAttribute(writer, prefix, namespaceURI);
			SAlloc::F(namespaceURI);
			SAlloc::F(prefix);
			if(count < 0) {
				xmlListDelete(writer->nsstack);
				writer->nsstack = NULL;
				return -1;
			}
			sum += count;
		}
	}
	return sum;
}
/**
 * xmlFreeTextWriterNsStackEntry:
 * @lk:  the xmlLinkPtr
 *
 * Free callback for the xmlList.
 */
static void FASTCALL xmlFreeTextWriterNsStackEntry(xmlLink * pLk)
{
	xmlTextWriterNsStackEntry * p = (xmlTextWriterNsStackEntry*)xmlLinkGetData(pLk);
	if(p) {
		SAlloc::F(p->prefix);
		SAlloc::F(p->uri);
		SAlloc::F(p);
	}
}
/**
 * xmlCmpTextWriterNsStackEntry:
 * @data0:  the first data
 * @data1:  the second data
 *
 * Compare callback for the xmlList.
 *
 * Returns -1, 0, 1
 */
static int xmlCmpTextWriterNsStackEntry(const void * data0, const void * data1)
{
	int rc;
	if(data0 == data1)
		return 0;
	else if(data0 == 0)
		return -1;
	else if(data1 == 0)
		return 1;
	else {
		const xmlTextWriterNsStackEntry * p0 = (const xmlTextWriterNsStackEntry *)data0;
		const xmlTextWriterNsStackEntry * p1 = (const xmlTextWriterNsStackEntry *)data1;
		rc = xmlStrcmp(p0->prefix, p1->prefix);
		if(rc || (p0->elem != p1->elem))
			rc = -1;
		return rc;
	}
}
/**
 * xmlTextWriterWriteDocCallback:
 * @context:  the xmlBufferPtr
 * @str:  the data to write
 * @len:  the length of the data
 *
 * Write callback for the xmlOutputBuffer with target xmlBuffer
 *
 * Returns -1, 0, 1
 */
static int xmlTextWriterWriteDocCallback(void * context, const xmlChar * str, int len)
{
	xmlParserCtxt * ctxt = (xmlParserCtxt *)context;
	int rc;
	if((rc = xmlParseChunk(ctxt, (const char*)str, len, 0)) != 0) {
		xmlWriterErrMsgInt(NULL, XML_ERR_INTERNAL_ERROR, "xmlTextWriterWriteDocCallback : XML error %d !\n", rc);
		return -1;
	}
	return len;
}
/**
 * xmlTextWriterCloseDocCallback:
 * @context:  the xmlBufferPtr
 *
 * Close callback for the xmlOutputBuffer with target xmlBuffer
 *
 * Returns -1, 0, 1
 */
static int xmlTextWriterCloseDocCallback(void * context)
{
	xmlParserCtxt * ctxt = (xmlParserCtxt *)context;
	int rc = xmlParseChunk(ctxt, NULL, 0, 1);
	if(rc) {
		xmlWriterErrMsgInt(NULL, XML_ERR_INTERNAL_ERROR, "xmlTextWriterWriteDocCallback : XML error %d !\n", rc);
		return -1;
	}
	else
		return 0;
}
/**
 * xmlTextWriterVSprintf:
 * @format:  see printf
 * @argptr:  pointer to the first member of the variable argument list.
 *
 * Utility function for formatted output
 *
 * Returns a new xmlChar buffer with the data or NULL on error. This buffer must be freed.
 */
static xmlChar * xmlTextWriterVSprintf(const char * format, va_list argptr)
{
	const char * _p_func_name = "xmlTextWriterVSprintf";
	int size = BUFSIZ;
	xmlChar * buf = (xmlChar*)SAlloc::M(size);
	if(!buf) {
		xmlWriterErrMsg(NULL, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
	}
	else {
		int count;
		va_list locarg;
		VA_COPY(locarg, argptr);
		while(((count = vsnprintf((char*)buf, size, format, locarg)) < 0) || (count == size - 1) || (count == size) || (count > size)) {
			va_end(locarg);
			SAlloc::F(buf);
			size += BUFSIZ;
			buf = (xmlChar *)SAlloc::M(size);
			if(!buf) {
				xmlWriterErrMsg(NULL, XML_ERR_NO_MEMORY, _p_func_name, "out of memory!");
				return NULL;
			}
			VA_COPY(locarg, argptr);
		}
		va_end(locarg);
	}
	return buf;
}
/**
 * xmlTextWriterStartDocumentCallback:
 * @ctx: the user data (XML parser context)
 *
 * called at the start of document processing.
 */
static void xmlTextWriterStartDocumentCallback(void * ctx)
{
	xmlParserCtxt * ctxt = (xmlParserCtxt *)ctx;
	xmlDocPtr doc;
	if(ctxt->html) {
#ifdef LIBXML_HTML_ENABLED
		SETIFZ(ctxt->myDoc, htmlNewDocNoDtD(NULL, NULL));
		if(ctxt->myDoc == NULL) {
			if(ctxt->sax && ctxt->sax->error)
				ctxt->sax->error(ctxt->userData, "SAX.startDocument(): out of memory\n");
			ctxt->errNo = XML_ERR_NO_MEMORY;
			ctxt->instate = XML_PARSER_EOF;
			ctxt->disableSAX = 1;
			return;
		}
#else
		xmlWriterErrMsg(NULL, XML_ERR_INTERNAL_ERROR, "libxml2 built without HTML support\n");
		ctxt->errNo = XML_ERR_INTERNAL_ERROR;
		ctxt->instate = XML_PARSER_EOF;
		ctxt->disableSAX = 1;
		return;
#endif
	}
	else {
		doc = ctxt->myDoc;
		if(!doc)
			doc = ctxt->myDoc = xmlNewDoc(ctxt->version);
		if(doc) {
			if(doc->children == NULL) {
				doc->encoding = ctxt->encoding ? sstrdup(ctxt->encoding) : 0;
				doc->standalone = ctxt->standalone;
			}
		}
		else {
			if(ctxt->sax && ctxt->sax->error)
				ctxt->sax->error(ctxt->userData, "SAX.startDocument(): out of memory\n");
			ctxt->errNo = XML_ERR_NO_MEMORY;
			ctxt->instate = XML_PARSER_EOF;
			ctxt->disableSAX = 1;
			return;
		}
	}
	if(ctxt->myDoc && (ctxt->myDoc->URL == NULL) && ctxt->input && (ctxt->input->filename != NULL)) {
		ctxt->myDoc->URL = xmlCanonicPath((const xmlChar*)ctxt->input->filename);
		SETIFZ(ctxt->myDoc->URL, sstrdup((const xmlChar*)ctxt->input->filename));
	}
}
/**
 * xmlTextWriterSetIndent:
 * @writer:  the xmlTextWriterPtr
 * @indent:  do indentation?
 *
 * Set indentation output. indent = 0 do not indentation. indent > 0 do indentation.
 *
 * Returns -1 on error or 0 otherwise.
 */
int xmlTextWriterSetIndent(xmlTextWriter * writer, int indent)
{
	if(!writer || indent < 0)
		return -1;
	else {
		writer->indent = indent;
		writer->doindent = 1;
		return 0;
	}
}
/**
 * xmlTextWriterSetIndentString:
 * @writer:  the xmlTextWriterPtr
 * @str:  the xmlChar string
 *
 * Set string indentation.
 *
 * Returns -1 on error or 0 otherwise.
 */
int xmlTextWriterSetIndentString(xmlTextWriter * writer, const xmlChar * str)
{
	if(!writer || !str)
		return -1;
	else {
		SAlloc::F(writer->ichar);
		writer->ichar = sstrdup(str);
		return writer->ichar ? 0 : -1;
	}
}
/**
 * xmlTextWriterSetQuoteChar:
 * @writer:  the xmlTextWriterPtr
 * @quotechar:  the quote character
 *
 * Set the character used for quoting attributes.
 *
 * Returns -1 on error or 0 otherwise.
 */
int xmlTextWriterSetQuoteChar(xmlTextWriter * writer, xmlChar quotechar)
{
	if(!writer || ((quotechar != '\'') && (quotechar != '"')))
		return -1;
	else {
		writer->qchar = quotechar;
		return 0;
	}
}
/**
 * xmlTextWriterWriteIndent:
 * @writer:  the xmlTextWriterPtr
 *
 * Write indent string.
 *
 * Returns -1 on error or the number of strings written.
 */
static int xmlTextWriterWriteIndent(xmlTextWriter * writer)
{
	int i;
	int ret;
	int lksize = xmlListSize(writer->nodes);
	if(lksize < 1)
		return -1;    /* list is empty */
	for(i = 0; i < (lksize - 1); i++) {
		ret = xmlOutputBufferWriteString(writer->out, (const char*)writer->ichar);
		if(ret == -1)
			return -1;
	}
	return (lksize - 1);
}
/**
 * xmlTextWriterHandleStateDependencies:
 * @writer:  the xmlTextWriterPtr
 * @p:  the xmlTextWriterStackEntry
 *
 * Write state dependent strings.
 *
 * Returns -1 on error or the number of characters written.
 */
static int xmlTextWriterHandleStateDependencies(xmlTextWriter * writer, xmlTextWriterStackEntry * p)
{
	int    count;
	int    sum = 0;
	char   extra[3];
	if(writer == NULL)
		return -1;
	if(!p)
		return 0;
	extra[0] = extra[1] = extra[2] = '\0';
	if(p != 0) {
		sum = 0;
		switch(p->state) {
			case XML_TEXTWRITER_NAME:
			    /* Output namespace declarations */
			    count = xmlTextWriterOutputNSDecl(writer);
			    if(count < 0)
				    return -1;
			    sum += count;
			    extra[0] = '>';
			    p->state = XML_TEXTWRITER_TEXT;
			    break;
			case XML_TEXTWRITER_PI:
			    extra[0] = ' ';
			    p->state = XML_TEXTWRITER_PI_TEXT;
			    break;
			case XML_TEXTWRITER_DTD:
			    extra[0] = ' ';
			    extra[1] = '[';
			    p->state = XML_TEXTWRITER_DTD_TEXT;
			    break;
			case XML_TEXTWRITER_DTD_ELEM:
			    extra[0] = ' ';
			    p->state = XML_TEXTWRITER_DTD_ELEM_TEXT;
			    break;
			case XML_TEXTWRITER_DTD_ATTL:
			    extra[0] = ' ';
			    p->state = XML_TEXTWRITER_DTD_ATTL_TEXT;
			    break;
			case XML_TEXTWRITER_DTD_ENTY:
			case XML_TEXTWRITER_DTD_PENT:
			    extra[0] = ' ';
			    extra[1] = writer->qchar;
			    p->state = XML_TEXTWRITER_DTD_ENTY_TEXT;
			    break;
			default:
			    break;
		}
	}
	if(*extra != '\0') {
		count = xmlOutputBufferWriteString(writer->out, extra);
		if(count < 0)
			return -1;
		sum += count;
	}
	return sum;
}

#define bottom_xmlwriter
//#include "elfgcchack.h"
#endif
