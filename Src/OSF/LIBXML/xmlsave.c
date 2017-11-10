/*
 * xmlsave.c: Implemetation of the document serializer
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */
#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop
#include <libxml/xmlsave.h>

#define MAX_INDENT 60

#include <libxml/HTMLtree.h>
#include "save.h"

/************************************************************************
*									*
*			XHTML detection					*
*									*
************************************************************************/
#define XHTML_STRICT_PUBLIC_ID BAD_CAST	"-//W3C//DTD XHTML 1.0 Strict//EN"
#define XHTML_STRICT_SYSTEM_ID BAD_CAST	"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"
#define XHTML_FRAME_PUBLIC_ID BAD_CAST "-//W3C//DTD XHTML 1.0 Frameset//EN"
#define XHTML_FRAME_SYSTEM_ID BAD_CAST "http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd"
#define XHTML_TRANS_PUBLIC_ID BAD_CAST "-//W3C//DTD XHTML 1.0 Transitional//EN"
#define XHTML_TRANS_SYSTEM_ID BAD_CAST "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"
#define XHTML_NS_NAME BAD_CAST "http://www.w3.org/1999/xhtml"
/**
 * xmlIsXHTML:
 * @systemID:  the system identifier
 * @publicID:  the public identifier
 *
 * Try to find if the document correspond to an XHTML DTD
 *
 * Returns 1 if true, 0 if not and -1 in case of error
 */
int xmlIsXHTML(const xmlChar * systemID, const xmlChar * publicID) 
{
	if((systemID == NULL) && (publicID == NULL))
		return -1;
	if(publicID != NULL) {
		if(sstreq(publicID, XHTML_STRICT_PUBLIC_ID)) return 1;
		if(sstreq(publicID, XHTML_FRAME_PUBLIC_ID)) return 1;
		if(sstreq(publicID, XHTML_TRANS_PUBLIC_ID)) return 1;
	}
	if(systemID != NULL) {
		if(sstreq(systemID, XHTML_STRICT_SYSTEM_ID)) return 1;
		if(sstreq(systemID, XHTML_FRAME_SYSTEM_ID)) return 1;
		if(sstreq(systemID, XHTML_TRANS_SYSTEM_ID)) return 1;
	}
	return 0;
}

#ifdef LIBXML_OUTPUT_ENABLED

#define TODO xmlGenericError(0, "Unimplemented block at %s:%d\n", __FILE__, __LINE__);

struct _xmlSaveCtxt {
	void * _private;
	int    type;
	int    fd;
	const  xmlChar * filename;
	const  xmlChar * encoding;
	xmlCharEncodingHandler * handler;
	xmlOutputBuffer * buf;
	xmlDoc * doc;
	int    options;
	int    level;
	int    format;
	char   indent[MAX_INDENT + 1];    /* array for indenting output */
	int    indent_nr;
	int    indent_size;
	xmlCharEncodingOutputFunc escape; /* used for element content */
	xmlCharEncodingOutputFunc escapeAttr; /* used for attribute content */
};

/************************************************************************
*									*
*			Output error handlers				*
*									*
************************************************************************/
/**
 * xmlSaveErrMemory:
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void FASTCALL xmlSaveErrMemory(const char * extra)
{
	__xmlSimpleError(XML_FROM_OUTPUT, XML_ERR_NO_MEMORY, NULL, NULL, extra);
}
/**
 * xmlSaveErr:
 * @code:  the error number
 * @node:  the location of the error.
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void FASTCALL xmlSaveErr(int code, xmlNodePtr P_Node, const char * extra)
{
	const char * msg = NULL;
	switch(code) {
		case XML_SAVE_NOT_UTF8: msg = "string is not in UTF-8\n"; break;
		case XML_SAVE_CHAR_INVALID: msg = "invalid character value\n"; break;
		case XML_SAVE_UNKNOWN_ENCODING: msg = "unknown encoding %s\n"; break;
		case XML_SAVE_NO_DOCTYPE: msg = "document has no DOCTYPE\n"; break;
		default: msg = "unexpected error number\n";
	}
	__xmlSimpleError(XML_FROM_OUTPUT, code, P_Node, msg, extra);
}

/************************************************************************
*									*
*			Special escaping routines			*
*									*
************************************************************************/
static uchar * FASTCALL xmlSerializeHexCharRef(uchar * out, int val)
{
	uchar * ptr;
	*out++ = '&';
	*out++ = '#';
	*out++ = 'x';
	if(val < 0x10)
		ptr = out;
	else if(val < 0x100) ptr = out + 1;
	else if(val < 0x1000) ptr = out + 2;
	else if(val < 0x10000) ptr = out + 3;
	else if(val < 0x100000) ptr = out + 4;
	else ptr = out + 5;
	out = ptr + 1;
	while(val > 0) {
		switch(val & 0xF) {
			case 0: *ptr-- = '0'; break;
			case 1: *ptr-- = '1'; break;
			case 2: *ptr-- = '2'; break;
			case 3: *ptr-- = '3'; break;
			case 4: *ptr-- = '4'; break;
			case 5: *ptr-- = '5'; break;
			case 6: *ptr-- = '6'; break;
			case 7: *ptr-- = '7'; break;
			case 8: *ptr-- = '8'; break;
			case 9: *ptr-- = '9'; break;
			case 0xA: *ptr-- = 'A'; break;
			case 0xB: *ptr-- = 'B'; break;
			case 0xC: *ptr-- = 'C'; break;
			case 0xD: *ptr-- = 'D'; break;
			case 0xE: *ptr-- = 'E'; break;
			case 0xF: *ptr-- = 'F'; break;
			default: *ptr-- = '0'; break;
		}
		val >>= 4;
	}
	*out++ = ';';
	*out = 0;
	return(out);
}

/**
 * xmlEscapeEntities:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @in:  a pointer to an array of unescaped UTF-8 bytes
 * @inlen:  the length of @in
 *
 * Take a block of UTF-8 chars in and escape them. Used when there is no
 * encoding specified.
 *
 * Returns 0 if success, or -1 otherwise
 * The value of @inlen after return is the number of octets consumed
 *     if the return value is positive, else unpredictable.
 * The value of @outlen after return is the number of octets consumed.
 */
static int xmlEscapeEntities(uchar* out, int * outlen, const xmlChar* in, int * inlen)
{
	uchar* outstart = out;
	const uchar* base = in;
	uchar* outend = out + *outlen;
	int val;
	const uchar * inend = in + (*inlen);
	while((in < inend) && (out < outend)) {
		if(*in == '<') {
			if(outend - out < 4)
				break;
			*out++ = '&';
			*out++ = 'l';
			*out++ = 't';
			*out++ = ';';
			in++;
			continue;
		}
		else if(*in == '>') {
			if(outend - out < 4)
				break;
			*out++ = '&';
			*out++ = 'g';
			*out++ = 't';
			*out++ = ';';
			in++;
			continue;
		}
		else if(*in == '&') {
			if(outend - out < 5) break;
			*out++ = '&';
			*out++ = 'a';
			*out++ = 'm';
			*out++ = 'p';
			*out++ = ';';
			in++;
			continue;
		}
		else if(((*in >= 0x20) && (*in < 0x80)) || (*in == '\n') || (*in == '\t')) {
			/*
			 * default case, just copy !
			 */
			*out++ = *in++;
			continue;
		}
		else if(*in >= 0x80) {
			/*
			 * We assume we have UTF-8 input.
			 */
			if(outend - out < 11)
				break;
			if(*in < 0xC0) {
				xmlSaveErr(XML_SAVE_NOT_UTF8, 0, 0);
				in++;
				goto error;
			}
			else if(*in < 0xE0) {
				if(inend - in < 2)
					break;
				val = (in[0]) & 0x1F;
				val <<= 6;
				val |= (in[1]) & 0x3F;
				in += 2;
			}
			else if(*in < 0xF0) {
				if(inend - in < 3)
					break;
				val = (in[0]) & 0x0F;
				val <<= 6;
				val |= (in[1]) & 0x3F;
				val <<= 6;
				val |= (in[2]) & 0x3F;
				in += 3;
			}
			else if(*in < 0xF8) {
				if(inend - in < 4)
					break;
				val = (in[0]) & 0x07;
				val <<= 6;
				val |= (in[1]) & 0x3F;
				val <<= 6;
				val |= (in[2]) & 0x3F;
				val <<= 6;
				val |= (in[3]) & 0x3F;
				in += 4;
			}
			else {
				xmlSaveErr(XML_SAVE_CHAR_INVALID, 0, 0);
				in++;
				goto error;
			}
			if(!IS_CHAR(val)) {
				xmlSaveErr(XML_SAVE_CHAR_INVALID, 0, 0);
				in++;
				goto error;
			}

			/*
			 * We could do multiple things here. Just save as a char ref
			 */
			out = xmlSerializeHexCharRef(out, val);
		}
		else if(IS_BYTE_CHAR(*in)) {
			if(outend - out < 6) break;
			out = xmlSerializeHexCharRef(out, *in++);
		}
		else {
			xmlGenericError(0, "xmlEscapeEntities : char out of range\n");
			in++;
			goto error;
		}
	}
	*outlen = out - outstart;
	*inlen = in - base;
	return 0;
error:
	*outlen = out - outstart;
	*inlen = in - base;
	return -1;
}

/************************************************************************
*									*
*			Allocation and deallocation			*
*									*
************************************************************************/
/**
 * xmlSaveCtxtInit:
 * @ctxt: the saving context
 *
 * Initialize a saving context
 */
static void xmlSaveCtxtInit(xmlSaveCtxtPtr ctxt)
{
	int i;
	int len;
	if(!ctxt) return;
	if((ctxt->encoding == NULL) && (ctxt->escape == NULL))
		ctxt->escape = xmlEscapeEntities;
	len = sstrlen((xmlChar*)xmlTreeIndentString);
	if((xmlTreeIndentString == NULL) || (len == 0)) {
		memzero(&ctxt->indent[0], MAX_INDENT + 1);
	}
	else {
		ctxt->indent_size = len;
		ctxt->indent_nr = MAX_INDENT / ctxt->indent_size;
		for(i = 0; i < ctxt->indent_nr; i++)
			memcpy(&ctxt->indent[i * ctxt->indent_size], xmlTreeIndentString,
			    ctxt->indent_size);
		ctxt->indent[ctxt->indent_nr * ctxt->indent_size] = 0;
	}
	if(xmlSaveNoEmptyTags) {
		ctxt->options |= XML_SAVE_NO_EMPTY;
	}
}

/**
 * xmlFreeSaveCtxt:
 *
 * Free a saving context, destroying the ouptut in any remaining buffer
 */
static void xmlFreeSaveCtxt(xmlSaveCtxtPtr ctxt)
{
	if(!ctxt) return;
	if(ctxt->encoding != NULL)
		SAlloc::F((char*)ctxt->encoding);
	if(ctxt->buf != NULL)
		xmlOutputBufferClose(ctxt->buf);
	SAlloc::F(ctxt);
}

/**
 * xmlNewSaveCtxt:
 *
 * Create a new saving context
 *
 * Returns the new structure or NULL in case of error
 */
static xmlSaveCtxt * FASTCALL xmlNewSaveCtxt(const char * encoding, int options)
{
	xmlSaveCtxt * ret = (xmlSaveCtxt *)SAlloc::M(sizeof(xmlSaveCtxt));
	if(!ret) {
		xmlSaveErrMemory("creating saving context");
		return ( NULL );
	}
	memzero(ret, sizeof(xmlSaveCtxt));
	if(encoding) {
		ret->handler = xmlFindCharEncodingHandler(encoding);
		if(ret->handler == NULL) {
			xmlSaveErr(XML_SAVE_UNKNOWN_ENCODING, NULL, encoding);
			xmlFreeSaveCtxt(ret);
			return 0;
		}
		ret->encoding = sstrdup((const xmlChar*)encoding);
		ret->escape = NULL;
	}
	xmlSaveCtxtInit(ret);

	/*
	 * Use the options
	 */
	/* Re-check this option as it may already have been set */
	if((ret->options & XML_SAVE_NO_EMPTY) && !(options & XML_SAVE_NO_EMPTY)) {
		options |= XML_SAVE_NO_EMPTY;
	}
	ret->options = options;
	if(options & XML_SAVE_FORMAT)
		ret->format = 1;
	else if(options & XML_SAVE_WSNONSIG)
		ret->format = 2;
	return ret;
}

/************************************************************************
*									*
*		Dumping XML tree content to a simple buffer		*
*									*
************************************************************************/
/**
 * xmlAttrSerializeContent:
 * @buf:  the XML buffer output
 * @doc:  the document
 * @attr:  the attribute pointer
 *
 * Serialize the attribute in the buffer
 */
static void xmlAttrSerializeContent(xmlOutputBufferPtr buf, xmlAttrPtr attr)
{
	for(xmlNode * children = attr->children; children; children = children->next) {
		switch(children->type) {
			case XML_TEXT_NODE:
			    xmlBufAttrSerializeTxtContent(buf->buffer, attr->doc, attr, children->content);
			    break;
			case XML_ENTITY_REF_NODE:
			    xmlBufAdd(buf->buffer, BAD_CAST "&", 1);
			    xmlBufAdd(buf->buffer, children->name, sstrlen(children->name));
			    xmlBufAdd(buf->buffer, BAD_CAST ";", 1);
			    break;
			default: // should not happen unless we have a badly built tree 
			    break;
		}
	}
}
/**
 * xmlBufDumpNotationTable:
 * @buf:  an xmlBufPtr output
 * @table:  A notation table
 *
 * This will dump the content of the notation table as an XML DTD definition
 */
void xmlBufDumpNotationTable(xmlBufPtr buf, xmlNotationTablePtr table) 
{
	xmlBuffer * buffer = xmlBufferCreate();
	if(!buffer) {
		return; // @todo set the error in buf
	}
	xmlDumpNotationTable(buffer, table);
	xmlBufMergeBuffer(buf, buffer);
}
/**
 * xmlBufDumpElementDecl:
 * @buf:  an xmlBufPtr output
 * @elem:  An element table
 *
 * This will dump the content of the element declaration as an XML
 * DTD definition
 */
void xmlBufDumpElementDecl(xmlBufPtr buf, xmlElementPtr elem) 
{
	xmlBuffer * buffer = xmlBufferCreate();
	if(!buffer) {
		return; // @todo set the error in buf
	}
	xmlDumpElementDecl(buffer, elem);
	xmlBufMergeBuffer(buf, buffer);
}
/**
 * xmlBufDumpAttributeDecl:
 * @buf:  an xmlBufPtr output
 * @attr:  An attribute declaration
 *
 * This will dump the content of the attribute declaration as an XML
 * DTD definition
 */
void xmlBufDumpAttributeDecl(xmlBufPtr buf, xmlAttribute * attr) 
{
	xmlBuffer * buffer = xmlBufferCreate();
	if(!buffer) {
		return; // @todo set the error in buf
	}
	xmlDumpAttributeDecl(buffer, attr);
	xmlBufMergeBuffer(buf, buffer);
}
/**
 * xmlBufDumpEntityDecl:
 * @buf:  an xmlBufPtr output
 * @ent:  An entity table
 *
 * This will dump the content of the entity table as an XML DTD definition
 */
void xmlBufDumpEntityDecl(xmlBufPtr buf, xmlEntityPtr ent) 
{
	xmlBuffer * buffer = xmlBufferCreate();
	if(!buffer) {
		return; // @todo set the error in buf
	}
	xmlDumpEntityDecl(buffer, ent);
	xmlBufMergeBuffer(buf, buffer);
}
/************************************************************************
*									*
*		Dumping XML tree content to an I/O output buffer	*
*									*
************************************************************************/

static int FASTCALL xmlSaveSwitchEncoding(xmlSaveCtxtPtr ctxt, const char * encoding)
{
	xmlOutputBufferPtr buf = ctxt->buf;
	if(encoding && !buf->encoder && !buf->conv) {
		buf->encoder = xmlFindCharEncodingHandler((const char*)encoding);
		if(buf->encoder == NULL) {
			xmlSaveErr(XML_SAVE_UNKNOWN_ENCODING, NULL, (const char*)encoding);
			return -1;
		}
		buf->conv = xmlBufCreate();
		if(buf->conv == NULL) {
			xmlCharEncCloseFunc(buf->encoder);
			xmlSaveErrMemory("creating encoding buffer");
			return -1;
		}
		// initialize the state, e.g. if outputting a BOM
		xmlCharEncOutput(buf, 1);
	}
	return 0;
}

static int FASTCALL xmlSaveClearEncoding(xmlSaveCtxtPtr ctxt)
{
	xmlOutputBufferPtr buf = ctxt->buf;
	xmlOutputBufferFlush(buf);
	xmlCharEncCloseFunc(buf->encoder);
	xmlBufFree(buf->conv);
	buf->encoder = NULL;
	buf->conv = NULL;
	return 0;
}

#ifdef LIBXML_HTML_ENABLED
static void xhtmlNodeDumpOutput(xmlSaveCtxtPtr ctxt, xmlNode * cur);
#endif
static void xmlNodeListDumpOutput(xmlSaveCtxtPtr ctxt, xmlNode * cur);
static void xmlNodeDumpOutputInternal(xmlSaveCtxtPtr ctxt, xmlNode * cur);
void xmlNsListDumpOutput(xmlOutputBufferPtr buf, xmlNs * cur);
static int xmlDocContentDumpOutput(xmlSaveCtxtPtr ctxt, xmlDocPtr cur);

/**
 * xmlOutputBufferWriteWSNonSig:
 * @ctxt:  The save context
 * @extra: Number of extra indents to apply to ctxt->level
 *
 * Write out formatting for non-significant whitespace output.
 */
static void FASTCALL xmlOutputBufferWriteWSNonSig(xmlSaveCtxtPtr ctxt, int extra)
{
	if(ctxt && ctxt->buf) {
		xmlOutputBufferWrite(ctxt->buf, 1, "\n");
		for(int i = 0; i < (ctxt->level + extra); i += ctxt->indent_nr) {
			xmlOutputBufferWrite(ctxt->buf, ctxt->indent_size * ((ctxt->level + extra - i) > ctxt->indent_nr ?
				ctxt->indent_nr : (ctxt->level + extra - i)), ctxt->indent);
		}
	}
}
/**
 * xmlNsDumpOutput:
 * @buf:  the XML buffer output
 * @cur:  a namespace
 * @ctxt: the output save context. Optional.
 *
 * Dump a local Namespace definition.
 * Should be called in the context of attributes dumps.
 * If @ctxt is supplied, @buf should be its buffer.
 */
static void xmlNsDumpOutput(xmlOutputBufferPtr buf, xmlNs * cur, xmlSaveCtxtPtr ctxt)
{
	if(cur && buf && (cur->type == XML_LOCAL_NAMESPACE) && cur->href) {
		if(sstreq(cur->prefix, BAD_CAST "xml") == 0) {
			if(ctxt && ctxt->format == 2)
				xmlOutputBufferWriteWSNonSig(ctxt, 2);
			else
				xmlOutputBufferWrite(buf, 1, " ");
			// Within the context of an element attributes
			if(cur->prefix != NULL) {
				xmlOutputBufferWrite(buf, 6, "xmlns:");
				xmlOutputBufferWriteString(buf, (const char*)cur->prefix);
			}
			else
				xmlOutputBufferWrite(buf, 5, "xmlns");
			xmlOutputBufferWrite(buf, 1, "=");
			xmlBufWriteQuotedString(buf->buffer, cur->href);
		}
	}
}

/**
 * xmlNsDumpOutputCtxt
 * @ctxt: the save context
 * @cur:  a namespace
 *
 * Dump a local Namespace definition to a save context.
 * Should be called in the context of attribute dumps.
 */
static void xmlNsDumpOutputCtxt(xmlSaveCtxtPtr ctxt, xmlNs * cur)
{
	xmlNsDumpOutput(ctxt->buf, cur, ctxt);
}

/**
 * xmlNsListDumpOutputCtxt
 * @ctxt: the save context
 * @cur:  the first namespace
 *
 * Dump a list of local namespace definitions to a save context.
 * Should be called in the context of attribute dumps.
 */
static void FASTCALL xmlNsListDumpOutputCtxt(xmlSaveCtxtPtr ctxt, xmlNs * cur)
{
	while(cur) {
		xmlNsDumpOutput(ctxt->buf, cur, ctxt);
		cur = cur->next;
	}
}
/**
 * xmlNsListDumpOutput:
 * @buf:  the XML buffer output
 * @cur:  the first namespace
 *
 * Dump a list of local Namespace definitions.
 * Should be called in the context of attributes dumps.
 */
void xmlNsListDumpOutput(xmlOutputBufferPtr buf, xmlNs * cur) 
{
	while(cur) {
		xmlNsDumpOutput(buf, cur, 0);
		cur = cur->next;
	}
}
/**
 * xmlDtdDumpOutput:
 * @buf:  the XML buffer output
 * @dtd:  the pointer to the DTD
 *
 * Dump the XML document DTD, if any.
 */
static void xmlDtdDumpOutput(xmlSaveCtxtPtr ctxt, xmlDtdPtr dtd)
{
	xmlOutputBufferPtr buf;
	int format, level;
	xmlDocPtr doc;
	if(dtd == NULL)
		return;
	if(!ctxt || (ctxt->buf == NULL))
		return;
	buf = ctxt->buf;
	xmlOutputBufferWrite(buf, 10, "<!DOCTYPE ");
	xmlOutputBufferWriteString(buf, (const char*)dtd->name);
	if(dtd->ExternalID != NULL) {
		xmlOutputBufferWrite(buf, 8, " PUBLIC ");
		xmlBufWriteQuotedString(buf->buffer, dtd->ExternalID);
		xmlOutputBufferWrite(buf, 1, " ");
		xmlBufWriteQuotedString(buf->buffer, dtd->SystemID);
	}
	else if(dtd->SystemID != NULL) {
		xmlOutputBufferWrite(buf, 8, " SYSTEM ");
		xmlBufWriteQuotedString(buf->buffer, dtd->SystemID);
	}
	if(!dtd->entities && !dtd->elements && !dtd->attributes && !dtd->notations && !dtd->pentities) {
		xmlOutputBufferWrite(buf, 1, ">");
		return;
	}
	xmlOutputBufferWrite(buf, 3, " [\n");
	/*
	 * Dump the notations first they are not in the DTD children list
	 * Do this only on a standalone DTD or on the internal subset though.
	 */
	if((dtd->notations != NULL) && ((dtd->doc == NULL) ||
		    (dtd->doc->intSubset == dtd))) {
		xmlBufDumpNotationTable(buf->buffer,
		    (xmlNotationTablePtr)dtd->notations);
	}
	format = ctxt->format;
	level = ctxt->level;
	doc = ctxt->doc;
	ctxt->format = 0;
	ctxt->level = -1;
	ctxt->doc = dtd->doc;
	xmlNodeListDumpOutput(ctxt, dtd->children);
	ctxt->format = format;
	ctxt->level = level;
	ctxt->doc = doc;
	xmlOutputBufferWrite(buf, 2, "]>");
}

/**
 * xmlAttrDumpOutput:
 * @buf:  the XML buffer output
 * @cur:  the attribute pointer
 *
 * Dump an XML attribute
 */
static void xmlAttrDumpOutput(xmlSaveCtxtPtr ctxt, xmlAttrPtr cur) {
	xmlOutputBufferPtr buf;

	if(!cur) return;
	buf = ctxt->buf;
	if(!buf) return;
	if(ctxt->format == 2)
		xmlOutputBufferWriteWSNonSig(ctxt, 2);
	else
		xmlOutputBufferWrite(buf, 1, " ");
	if(cur->ns && (cur->ns->prefix != NULL)) {
		xmlOutputBufferWriteString(buf, (const char*)cur->ns->prefix);
		xmlOutputBufferWrite(buf, 1, ":");
	}
	xmlOutputBufferWriteString(buf, (const char*)cur->name);
	xmlOutputBufferWrite(buf, 2, "=\"");
	xmlAttrSerializeContent(buf, cur);
	xmlOutputBufferWrite(buf, 1, "\"");
}

/**
 * xmlAttrListDumpOutput:
 * @buf:  the XML buffer output
 * @doc:  the document
 * @cur:  the first attribute pointer
 * @encoding:  an optional encoding string
 *
 * Dump a list of XML attributes
 */
static void xmlAttrListDumpOutput(xmlSaveCtxtPtr ctxt, xmlAttrPtr cur) {
	if(!cur) return;
	while(cur) {
		xmlAttrDumpOutput(ctxt, cur);
		cur = cur->next;
	}
}

/**
 * xmlNodeListDumpOutput:
 * @cur:  the first node
 *
 * Dump an XML node list, recursive behaviour, children are printed too.
 */
static void xmlNodeListDumpOutput(xmlSaveCtxtPtr ctxt, xmlNode * cur) {
	xmlOutputBufferPtr buf;

	if(!cur) return;
	buf = ctxt->buf;
	while(cur) {
		if((ctxt->format == 1) && (xmlIndentTreeOutput) &&
		    ((cur->type == XML_ELEMENT_NODE) ||
			    (cur->type == XML_COMMENT_NODE) ||
			    (cur->type == XML_PI_NODE)))
			xmlOutputBufferWrite(buf, ctxt->indent_size *
			    (ctxt->level > ctxt->indent_nr ?
				    ctxt->indent_nr : ctxt->level),
			    ctxt->indent);
		xmlNodeDumpOutputInternal(ctxt, cur);
		if(ctxt->format == 1) {
			xmlOutputBufferWrite(buf, 1, "\n");
		}
		cur = cur->next;
	}
}

#ifdef LIBXML_HTML_ENABLED
/**
 * xmlNodeDumpOutputInternal:
 * @cur:  the current node
 *
 * Dump an HTML node, recursive behaviour, children are printed too.
 */
static int htmlNodeDumpOutputInternal(xmlSaveCtxtPtr ctxt, xmlNode * cur)
{
	const xmlChar * oldenc = NULL;
	const xmlChar * oldctxtenc = ctxt->encoding;
	const xmlChar * encoding = ctxt->encoding;
	xmlOutputBufferPtr buf = ctxt->buf;
	int switched_encoding = 0;
	xmlDocPtr doc;
	xmlInitParser();
	doc = cur->doc;
	if(doc) {
		oldenc = doc->encoding;
		if(ctxt->encoding != NULL) {
			doc->encoding = BAD_CAST ctxt->encoding;
		}
		else if(doc->encoding != NULL) {
			encoding = doc->encoding;
		}
	}
	if((encoding != NULL) && doc)
		htmlSetMetaEncoding(doc, (const xmlChar*)encoding);
	if((encoding == NULL) && doc)
		encoding = htmlGetMetaEncoding(doc);
	SETIFZ(encoding, BAD_CAST "HTML");
	if((encoding != NULL) && (oldctxtenc == NULL) && (buf->encoder == NULL) && (buf->conv == NULL)) {
		if(xmlSaveSwitchEncoding(ctxt, (const char*)encoding) < 0) {
			doc->encoding = oldenc;
			return -1;
		}
		switched_encoding = 1;
	}
	if(ctxt->options & XML_SAVE_FORMAT)
		htmlNodeDumpFormatOutput(buf, doc, cur, (const char*)encoding, 1);
	else
		htmlNodeDumpFormatOutput(buf, doc, cur, (const char*)encoding, 0);
	/*
	 * Restore the state of the saving context at the end of the document
	 */
	if((switched_encoding) && (oldctxtenc == NULL)) {
		xmlSaveClearEncoding(ctxt);
	}
	if(doc)
		doc->encoding = oldenc;
	return 0;
}
#endif

/**
 * xmlNodeDumpOutputInternal:
 * @cur:  the current node
 *
 * Dump an XML node, recursive behaviour, children are printed too.
 */
static void xmlNodeDumpOutputInternal(xmlSaveCtxtPtr ctxt, xmlNode * cur)
{
	xmlChar * start, * end;
	if(cur && !oneof2(cur->type, XML_XINCLUDE_START, XML_XINCLUDE_END)) {
		xmlOutputBuffer * p_buf = ctxt->buf;
		if(oneof2(cur->type, XML_DOCUMENT_NODE, XML_HTML_DOCUMENT_NODE)) {
			xmlDocContentDumpOutput(ctxt, (xmlDocPtr)cur);
		}
#ifdef LIBXML_HTML_ENABLED
		else if(ctxt->options & XML_SAVE_XHTML) {
			xhtmlNodeDumpOutput(ctxt, cur);
		}
		else if(((cur->type != XML_NAMESPACE_DECL) && cur->doc && (cur->doc->type == XML_HTML_DOCUMENT_NODE) && ((ctxt->options & XML_SAVE_AS_XML) == 0)) || (ctxt->options & XML_SAVE_AS_HTML)) {
			htmlNodeDumpOutputInternal(ctxt, cur);
		}
#endif
		else if(cur->type == XML_DTD_NODE)
			xmlDtdDumpOutput(ctxt, (xmlDtd *)cur);
		else if(cur->type == XML_DOCUMENT_FRAG_NODE)
			xmlNodeListDumpOutput(ctxt, cur->children);
		else if(cur->type == XML_ELEMENT_DECL)
			xmlBufDumpElementDecl(p_buf->buffer, (xmlElementPtr)cur);
		else if(cur->type == XML_ATTRIBUTE_DECL)
			xmlBufDumpAttributeDecl(p_buf->buffer, (xmlAttribute *)cur);
		else if(cur->type == XML_ENTITY_DECL)
			xmlBufDumpEntityDecl(p_buf->buffer, (xmlEntityPtr)cur);
		else if(cur->type == XML_TEXT_NODE) {
			if(cur->content) {
				if(cur->name != xmlStringTextNoenc) {
					xmlOutputBufferWriteEscape(p_buf, cur->content, ctxt->escape);
				}
				else {
					// Disable escaping, needed for XSLT
					xmlOutputBufferWriteString(p_buf, (const char*)cur->content);
				}
			}
		}
		else if(cur->type == XML_PI_NODE) {
			if(cur->content) {
				xmlOutputBufferWrite(p_buf, 2, "<?");
				xmlOutputBufferWriteString(p_buf, (const char*)cur->name);
				if(cur->content) {
					if(ctxt->format == 2)
						xmlOutputBufferWriteWSNonSig(ctxt, 0);
					else
						xmlOutputBufferWrite(p_buf, 1, " ");
					xmlOutputBufferWriteString(p_buf, (const char*)cur->content);
				}
				xmlOutputBufferWrite(p_buf, 2, "?>");
			}
			else {
				xmlOutputBufferWrite(p_buf, 2, "<?");
				xmlOutputBufferWriteString(p_buf, (const char*)cur->name);
				if(ctxt->format == 2)
					xmlOutputBufferWriteWSNonSig(ctxt, 0);
				xmlOutputBufferWrite(p_buf, 2, "?>");
			}
		}
		else if(cur->type == XML_COMMENT_NODE) {
			if(cur->content) {
				xmlOutputBufferWrite(p_buf, 4, "<!--");
				xmlOutputBufferWriteString(p_buf, (const char*)cur->content);
				xmlOutputBufferWrite(p_buf, 3, "-->");
			}
		}
		else if(cur->type == XML_ENTITY_REF_NODE) {
			xmlOutputBufferWrite(p_buf, 1, "&");
			xmlOutputBufferWriteString(p_buf, (const char*)cur->name);
			xmlOutputBufferWrite(p_buf, 1, ";");
		}
		else if(cur->type == XML_CDATA_SECTION_NODE) {
			if(cur->content == NULL || *cur->content == '\0')
				xmlOutputBufferWrite(p_buf, 12, "<![CDATA[]]>");
			else {
				start = end = cur->content;
				while(*end != '\0') {
					if((*end == ']') && (*(end + 1) == ']') && (*(end + 2) == '>')) {
						end = end + 2;
						xmlOutputBufferWrite(p_buf, 9, "<![CDATA[");
						xmlOutputBufferWrite(p_buf, end - start, (const char*)start);
						xmlOutputBufferWrite(p_buf, 3, "]]>");
						start = end;
					}
					end++;
				}
				if(start != end) {
					xmlOutputBufferWrite(p_buf, 9, "<![CDATA[");
					xmlOutputBufferWriteString(p_buf, (const char*)start);
					xmlOutputBufferWrite(p_buf, 3, "]]>");
				}
			}
		}
		else if(cur->type == XML_ATTRIBUTE_NODE) {
			xmlAttrDumpOutput(ctxt, (xmlAttr *)cur);
		}
		else if(cur->type == XML_NAMESPACE_DECL) {
			xmlNsDumpOutputCtxt(ctxt, (xmlNs *)cur);
		}
		else {
			int format = ctxt->format;
			if(format == 1) {
				xmlNode * p_tmp = cur->children;
				while(p_tmp) {
					if((p_tmp->type == XML_TEXT_NODE) || (p_tmp->type == XML_CDATA_SECTION_NODE) || (p_tmp->type == XML_ENTITY_REF_NODE)) {
						ctxt->format = 0;
						break;
					}
					p_tmp = p_tmp->next;
				}
			}
			xmlOutputBufferWrite(p_buf, 1, "<");
			if(cur->ns && (cur->ns->prefix != NULL)) {
				xmlOutputBufferWriteString(p_buf, (const char*)cur->ns->prefix);
				xmlOutputBufferWrite(p_buf, 1, ":");
			}
			xmlOutputBufferWriteString(p_buf, (const char*)cur->name);
			xmlNsListDumpOutputCtxt(ctxt, cur->nsDef);
			if(cur->properties != NULL)
				xmlAttrListDumpOutput(ctxt, cur->properties);
			if(((cur->type == XML_ELEMENT_NODE) || (cur->content == NULL)) && (cur->children == NULL) && ((ctxt->options & XML_SAVE_NO_EMPTY) == 0)) {
				if(ctxt->format == 2)
					xmlOutputBufferWriteWSNonSig(ctxt, 0);
				xmlOutputBufferWrite(p_buf, 2, "/>");
				ctxt->format = format;
			}
			else {
				if(ctxt->format == 2)
					xmlOutputBufferWriteWSNonSig(ctxt, 1);
				xmlOutputBufferWrite(p_buf, 1, ">");
				if((cur->type != XML_ELEMENT_NODE) && (cur->content != NULL)) {
					xmlOutputBufferWriteEscape(p_buf, cur->content, ctxt->escape);
				}
				if(cur->children) {
					if(ctxt->format == 1)
						xmlOutputBufferWrite(p_buf, 1, "\n");
					if(ctxt->level >= 0)
						ctxt->level++;
					xmlNodeListDumpOutput(ctxt, cur->children);
					if(ctxt->level > 0)
						ctxt->level--;
					if((xmlIndentTreeOutput) && (ctxt->format == 1))
						xmlOutputBufferWrite(p_buf, ctxt->indent_size * (ctxt->level > ctxt->indent_nr ? ctxt->indent_nr : ctxt->level), ctxt->indent);
				}
				xmlOutputBufferWrite(p_buf, 2, "</");
				if(cur->ns && (cur->ns->prefix != NULL)) {
					xmlOutputBufferWriteString(p_buf, (const char*)cur->ns->prefix);
					xmlOutputBufferWrite(p_buf, 1, ":");
				}
				xmlOutputBufferWriteString(p_buf, (const char*)cur->name);
				if(ctxt->format == 2)
					xmlOutputBufferWriteWSNonSig(ctxt, 0);
				xmlOutputBufferWrite(p_buf, 1, ">");
				ctxt->format = format;
			}
		}
	}
}

/**
 * xmlDocContentDumpOutput:
 * @cur:  the document
 *
 * Dump an XML document.
 */
static int xmlDocContentDumpOutput(xmlSaveCtxtPtr ctxt, xmlDocPtr cur)
{
#ifdef LIBXML_HTML_ENABLED
	xmlDtd * dtd;
	int is_xhtml = 0;
#endif
	const xmlChar * oldenc = cur->encoding;
	const xmlChar * oldctxtenc = ctxt->encoding;
	const xmlChar * encoding = ctxt->encoding;
	xmlCharEncodingOutputFunc oldescape = ctxt->escape;
	xmlCharEncodingOutputFunc oldescapeAttr = ctxt->escapeAttr;
	xmlOutputBufferPtr buf = ctxt->buf;
	xmlCharEncoding enc;
	int switched_encoding = 0;
	xmlInitParser();
	if((cur->type != XML_HTML_DOCUMENT_NODE) && (cur->type != XML_DOCUMENT_NODE))
		return -1;
	if(ctxt->encoding) {
		cur->encoding = BAD_CAST ctxt->encoding;
	}
	else if(cur->encoding != NULL) {
		encoding = cur->encoding;
	}
	else if(cur->charset != XML_CHAR_ENCODING_UTF8) {
		encoding = (const xmlChar*)xmlGetCharEncodingName((xmlCharEncoding)cur->charset);
	}
	if(((cur->type == XML_HTML_DOCUMENT_NODE) && !(ctxt->options & XML_SAVE_AS_XML) && !(ctxt->options & XML_SAVE_XHTML)) || (ctxt->options & XML_SAVE_AS_HTML)) {
#ifdef LIBXML_HTML_ENABLED
		if(encoding)
			htmlSetMetaEncoding(cur, (const xmlChar*)encoding);
		SETIFZ(encoding, htmlGetMetaEncoding(cur));
		SETIFZ(encoding, BAD_CAST "HTML");
		if(encoding && !oldctxtenc && !buf->encoder && !buf->conv) {
			if(xmlSaveSwitchEncoding(ctxt, (const char*)encoding) < 0) {
				cur->encoding = oldenc;
				return -1;
			}
		}
		if(ctxt->options & XML_SAVE_FORMAT)
			htmlDocContentDumpFormatOutput(buf, cur, (const char*)encoding, 1);
		else
			htmlDocContentDumpFormatOutput(buf, cur, (const char*)encoding, 0);
		if(ctxt->encoding)
			cur->encoding = oldenc;
		return 0;
#else
		return -1;
#endif
	}
	else if((cur->type == XML_DOCUMENT_NODE) || (ctxt->options & XML_SAVE_AS_XML) || (ctxt->options & XML_SAVE_XHTML)) {
		enc = xmlParseCharEncoding((const char*)encoding);
		if((encoding != NULL) && (oldctxtenc == NULL) && (buf->encoder == NULL) && (buf->conv == NULL) && ((ctxt->options & XML_SAVE_NO_DECL) == 0)) {
			if((enc != XML_CHAR_ENCODING_UTF8) && (enc != XML_CHAR_ENCODING_NONE) && (enc != XML_CHAR_ENCODING_ASCII)) {
				/*
				 * we need to switch to this encoding but just for this
				 * document since we output the XMLDecl the conversion
				 * must be done to not generate not well formed documents.
				 */
				if(xmlSaveSwitchEncoding(ctxt, (const char*)encoding) < 0) {
					cur->encoding = oldenc;
					return -1;
				}
				switched_encoding = 1;
			}
			if(ctxt->escape == xmlEscapeEntities)
				ctxt->escape = NULL;
			if(ctxt->escapeAttr == xmlEscapeEntities)
				ctxt->escapeAttr = NULL;
		}

		/*
		 * Save the XML declaration
		 */
		if(!(ctxt->options & XML_SAVE_NO_DECL)) {
			xmlOutputBufferWrite(buf, 14, "<?xml version=");
			if(cur->version != NULL)
				xmlBufWriteQuotedString(buf->buffer, cur->version);
			else
				xmlOutputBufferWrite(buf, 5, "\"1.0\"");
			if(encoding) {
				xmlOutputBufferWrite(buf, 10, " encoding=");
				xmlBufWriteQuotedString(buf->buffer, (xmlChar*)encoding);
			}
			switch(cur->standalone) {
				case 0:
				    xmlOutputBufferWrite(buf, 16, " standalone=\"no\"");
				    break;
				case 1:
				    xmlOutputBufferWrite(buf, 17, " standalone=\"yes\"");
				    break;
			}
			xmlOutputBufferWrite(buf, 3, "?>\n");
		}

#ifdef LIBXML_HTML_ENABLED
		if(ctxt->options & XML_SAVE_XHTML)
			is_xhtml = 1;
		if(!(ctxt->options & XML_SAVE_NO_XHTML)) {
			dtd = xmlGetIntSubset(cur);
			if(dtd) {
				is_xhtml = xmlIsXHTML(dtd->SystemID, dtd->ExternalID);
				if(is_xhtml < 0)
					is_xhtml = 0;
			}
		}
#endif
		if(cur->children) {
			xmlNodePtr child = cur->children;
			while(child != NULL) {
				ctxt->level = 0;
#ifdef LIBXML_HTML_ENABLED
				if(is_xhtml)
					xhtmlNodeDumpOutput(ctxt, child);
				else
#endif
				xmlNodeDumpOutputInternal(ctxt, child);
				xmlOutputBufferWrite(buf, 1, "\n");
				child = child->next;
			}
		}
	}

	/*
	 * Restore the state of the saving context at the end of the document
	 */
	if((switched_encoding) && (oldctxtenc == NULL)) {
		xmlSaveClearEncoding(ctxt);
		ctxt->escape = oldescape;
		ctxt->escapeAttr = oldescapeAttr;
	}
	cur->encoding = oldenc;
	return 0;
}

#ifdef LIBXML_HTML_ENABLED
/************************************************************************
*									*
*		Functions specific to XHTML serialization		*
*									*
************************************************************************/

/**
 * xhtmlIsEmpty:
 * @node:  the node
 *
 * Check if a node is an empty xhtml node
 *
 * Returns 1 if the node is an empty node, 0 if not and -1 in case of error
 */
static int xhtmlIsEmpty(xmlNodePtr P_Node)
{
	if(!P_Node)
		return -1;
	if(P_Node->type != XML_ELEMENT_NODE)
		return 0;
	if((P_Node->ns != NULL) && (!sstreq(P_Node->ns->href, XHTML_NS_NAME)))
		return 0;
	if(P_Node->children != NULL)
		return 0;
	switch(P_Node->name[0]) {
		case 'a':
		    if(sstreq(P_Node->name, "area"))
			    return 1;
		    return 0;
		case 'b':
		    if(sstreq(P_Node->name, "br"))
			    return 1;
		    if(sstreq(P_Node->name, "base"))
			    return 1;
		    if(sstreq(P_Node->name, "basefont"))
			    return 1;
		    return 0;
		case 'c':
		    if(sstreq(P_Node->name, "col"))
			    return 1;
		    return 0;
		case 'f':
		    if(sstreq(P_Node->name, "frame"))
			    return 1;
		    return 0;
		case 'h':
		    if(sstreq(P_Node->name, "hr"))
			    return 1;
		    return 0;
		case 'i':
		    if(sstreq(P_Node->name, "img"))
			    return 1;
		    if(sstreq(P_Node->name, "input"))
			    return 1;
		    if(sstreq(P_Node->name, "isindex"))
			    return 1;
		    return 0;
		case 'l':
		    if(sstreq(P_Node->name, "link"))
			    return 1;
		    return 0;
		case 'm':
		    if(sstreq(P_Node->name, "meta"))
			    return 1;
		    return 0;
		case 'p':
		    if(sstreq(P_Node->name, "param"))
			    return 1;
		    return 0;
	}
	return 0;
}

/**
 * xhtmlAttrListDumpOutput:
 * @cur:  the first attribute pointer
 *
 * Dump a list of XML attributes
 */
static void xhtmlAttrListDumpOutput(xmlSaveCtxtPtr ctxt, xmlAttrPtr cur) {
	xmlAttrPtr xml_lang = NULL;
	xmlAttrPtr lang = NULL;
	xmlAttrPtr name = NULL;
	xmlAttrPtr id = NULL;
	xmlNodePtr parent;
	xmlOutputBufferPtr buf;

	if(!cur) return;
	buf = ctxt->buf;
	parent = cur->parent;
	while(cur) {
		if((cur->ns == NULL) && (sstreq(cur->name, "id")))
			id = cur;
		else if((cur->ns == NULL) && (sstreq(cur->name, "name")))
			name = cur;
		else if((cur->ns == NULL) && (sstreq(cur->name, "lang")))
			lang = cur;
		else if(cur->ns && (sstreq(cur->name, "lang")) && (sstreq(cur->ns->prefix, "xml")))
			xml_lang = cur;
		else if((cur->ns == NULL) &&
		    ((cur->children == NULL) ||
			    (cur->children->content == NULL) ||
			    (cur->children->content[0] == 0)) &&
		    (htmlIsBooleanAttr(cur->name))) {
			if(cur->children)
				xmlFreeNode(cur->children);
			cur->children = xmlNewText(cur->name);
			if(cur->children)
				cur->children->parent = (xmlNode *)cur;
		}
		xmlAttrDumpOutput(ctxt, cur);
		cur = cur->next;
	}
	/*
	 * C.8
	 */
	if((name != NULL) && (id == NULL)) {
		if((parent != NULL) && (parent->name != NULL) && ((sstreq(parent->name, "a")) ||
			    (sstreq(parent->name, "p")) || (sstreq(parent->name, "div")) || (sstreq(parent->name, "img")) ||
			    (sstreq(parent->name, "map")) || (sstreq(parent->name, "applet")) || (sstreq(parent->name, "form")) ||
			    (sstreq(parent->name, "frame")) || (sstreq(parent->name, "iframe")))) {
			xmlOutputBufferWrite(buf, 5, " id=\"");
			xmlAttrSerializeContent(buf, name);
			xmlOutputBufferWrite(buf, 1, "\"");
		}
	}
	/*
	 * C.7.
	 */
	if((lang != NULL) && (xml_lang == NULL)) {
		xmlOutputBufferWrite(buf, 11, " xml:lang=\"");
		xmlAttrSerializeContent(buf, lang);
		xmlOutputBufferWrite(buf, 1, "\"");
	}
	else if((xml_lang != NULL) && (lang == NULL))        {
		xmlOutputBufferWrite(buf, 7, " lang=\"");
		xmlAttrSerializeContent(buf, xml_lang);
		xmlOutputBufferWrite(buf, 1, "\"");
	}
}

/**
 * xhtmlNodeListDumpOutput:
 * @buf:  the XML buffer output
 * @doc:  the XHTML document
 * @cur:  the first node
 * @level: the imbrication level for indenting
 * @format: is formatting allowed
 * @encoding:  an optional encoding string
 *
 * Dump an XML node list, recursive behaviour, children are printed too.
 * Note that @format = 1 provide node indenting only if xmlIndentTreeOutput = 1
 * or xmlKeepBlanksDefault(0) was called
 */
static void xhtmlNodeListDumpOutput(xmlSaveCtxtPtr ctxt, xmlNode * cur) {
	xmlOutputBufferPtr buf;

	if(!cur) return;
	buf = ctxt->buf;
	while(cur) {
		if((ctxt->format == 1) && (xmlIndentTreeOutput) &&
		    (cur->type == XML_ELEMENT_NODE))
			xmlOutputBufferWrite(buf, ctxt->indent_size *
			    (ctxt->level > ctxt->indent_nr ?
				    ctxt->indent_nr : ctxt->level),
			    ctxt->indent);
		xhtmlNodeDumpOutput(ctxt, cur);
		if(ctxt->format == 1) {
			xmlOutputBufferWrite(buf, 1, "\n");
		}
		cur = cur->next;
	}
}

/**
 * xhtmlNodeDumpOutput:
 * @buf:  the XML buffer output
 * @doc:  the XHTML document
 * @cur:  the current node
 * @level: the imbrication level for indenting
 * @format: is formatting allowed
 * @encoding:  an optional encoding string
 *
 * Dump an XHTML node, recursive behaviour, children are printed too.
 */
static void xhtmlNodeDumpOutput(xmlSaveCtxtPtr ctxt, xmlNode * cur) 
{
	int format, addmeta = 0;
	xmlNodePtr tmp;
	xmlChar * start, * end;
	xmlOutputBufferPtr buf;
	if(!cur) 
		return;
	if(oneof2(cur->type, XML_DOCUMENT_NODE, XML_HTML_DOCUMENT_NODE)) {
		xmlDocContentDumpOutput(ctxt, (xmlDocPtr)cur);
		return;
	}
	if(cur->type == XML_XINCLUDE_START)
		return;
	if(cur->type == XML_XINCLUDE_END)
		return;
	if(cur->type == XML_NAMESPACE_DECL) {
		xmlNsDumpOutputCtxt(ctxt, (xmlNs *)cur);
		return;
	}
	if(cur->type == XML_DTD_NODE) {
		xmlDtdDumpOutput(ctxt, (xmlDtd *)cur);
		return;
	}
	if(cur->type == XML_DOCUMENT_FRAG_NODE) {
		xhtmlNodeListDumpOutput(ctxt, cur->children);
		return;
	}
	buf = ctxt->buf;
	if(cur->type == XML_ELEMENT_DECL) {
		xmlBufDumpElementDecl(buf->buffer, (xmlElementPtr)cur);
		return;
	}
	if(cur->type == XML_ATTRIBUTE_DECL) {
		xmlBufDumpAttributeDecl(buf->buffer, (xmlAttribute *)cur);
		return;
	}
	if(cur->type == XML_ENTITY_DECL) {
		xmlBufDumpEntityDecl(buf->buffer, (xmlEntityPtr)cur);
		return;
	}
	if(cur->type == XML_TEXT_NODE) {
		if(cur->content) {
			if((cur->name == xmlStringText) || (cur->name != xmlStringTextNoenc)) {
				xmlOutputBufferWriteEscape(buf, cur->content, ctxt->escape);
			}
			else {
				/*
				 * Disable escaping, needed for XSLT
				 */
				xmlOutputBufferWriteString(buf, (const char*)cur->content);
			}
		}

		return;
	}
	if(cur->type == XML_PI_NODE) {
		if(cur->content) {
			xmlOutputBufferWrite(buf, 2, "<?");
			xmlOutputBufferWriteString(buf, (const char*)cur->name);
			if(cur->content) {
				xmlOutputBufferWrite(buf, 1, " ");
				xmlOutputBufferWriteString(buf, (const char*)cur->content);
			}
			xmlOutputBufferWrite(buf, 2, "?>");
		}
		else {
			xmlOutputBufferWrite(buf, 2, "<?");
			xmlOutputBufferWriteString(buf, (const char*)cur->name);
			xmlOutputBufferWrite(buf, 2, "?>");
		}
		return;
	}
	if(cur->type == XML_COMMENT_NODE) {
		if(cur->content) {
			xmlOutputBufferWrite(buf, 4, "<!--");
			xmlOutputBufferWriteString(buf, (const char*)cur->content);
			xmlOutputBufferWrite(buf, 3, "-->");
		}
		return;
	}
	if(cur->type == XML_ENTITY_REF_NODE) {
		xmlOutputBufferWrite(buf, 1, "&");
		xmlOutputBufferWriteString(buf, (const char*)cur->name);
		xmlOutputBufferWrite(buf, 1, ";");
		return;
	}
	if(cur->type == XML_CDATA_SECTION_NODE) {
		if(cur->content == NULL || *cur->content == '\0') {
			xmlOutputBufferWrite(buf, 12, "<![CDATA[]]>");
		}
		else {
			start = end = cur->content;
			while(*end != '\0') {
				if(*end == ']' && *(end + 1) == ']' && *(end + 2) == '>') {
					end = end + 2;
					xmlOutputBufferWrite(buf, 9, "<![CDATA[");
					xmlOutputBufferWrite(buf, end - start, (const char*)start);
					xmlOutputBufferWrite(buf, 3, "]]>");
					start = end;
				}
				end++;
			}
			if(start != end) {
				xmlOutputBufferWrite(buf, 9, "<![CDATA[");
				xmlOutputBufferWriteString(buf, (const char*)start);
				xmlOutputBufferWrite(buf, 3, "]]>");
			}
		}
		return;
	}
	if(cur->type == XML_ATTRIBUTE_NODE) {
		xmlAttrDumpOutput(ctxt, (xmlAttr *)cur);
		return;
	}
	format = ctxt->format;
	if(format == 1) {
		tmp = cur->children;
		while(tmp) {
			if(oneof2(tmp->type, XML_TEXT_NODE, XML_ENTITY_REF_NODE)) {
				format = 0;
				break;
			}
			tmp = tmp->next;
		}
	}
	xmlOutputBufferWrite(buf, 1, "<");
	if(cur->ns && cur->ns->prefix) {
		xmlOutputBufferWriteString(buf, (const char*)cur->ns->prefix);
		xmlOutputBufferWrite(buf, 1, ":");
	}
	xmlOutputBufferWriteString(buf, (const char*)cur->name);
	xmlNsListDumpOutputCtxt(ctxt, cur->nsDef);
	if(sstreq(cur->name, "html") && !cur->ns && !cur->nsDef) {
		/*
		 * 3.1.1. Strictly Conforming Documents A.3.1.1 3/
		 */
		xmlOutputBufferWriteString(buf, " xmlns=\"http://www.w3.org/1999/xhtml\"");
	}
	if(cur->properties)
		xhtmlAttrListDumpOutput(ctxt, cur->properties);
	if((cur->type == XML_ELEMENT_NODE) && cur->parent && (cur->parent->parent == (xmlNode *)cur->doc) &&
	    sstreq(cur->name, "head") && sstreq(cur->parent->name, "html")) {
		tmp = cur->children;
		while(tmp) {
			if(sstreq(tmp->name, "meta")) {
				xmlChar * httpequiv = xmlGetProp(tmp, BAD_CAST "http-equiv");
				if(httpequiv) {
					if(xmlStrcasecmp(httpequiv, BAD_CAST "Content-Type") == 0) {
						SAlloc::F(httpequiv);
						break;
					}
					SAlloc::F(httpequiv);
				}
			}
			tmp = tmp->next;
		}
		if(!tmp)
			addmeta = 1;
	}
	if((cur->type == XML_ELEMENT_NODE) && (cur->children == NULL)) {
		if(((cur->ns == NULL) || (cur->ns->prefix == NULL)) && ((xhtmlIsEmpty(cur) == 1) && (addmeta == 0))) {
			/*
			 * C.2. Empty Elements
			 */
			xmlOutputBufferWrite(buf, 3, " />");
		}
		else {
			if(addmeta == 1) {
				xmlOutputBufferWrite(buf, 1, ">");
				if(ctxt->format == 1) {
					xmlOutputBufferWrite(buf, 1, "\n");
					if(xmlIndentTreeOutput)
						xmlOutputBufferWrite(buf, ctxt->indent_size * (ctxt->level + 1 > ctxt->indent_nr ? ctxt->indent_nr : ctxt->level + 1), ctxt->indent);
				}
				xmlOutputBufferWriteString(buf, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=");
				if(ctxt->encoding) {
					xmlOutputBufferWriteString(buf, (const char*)ctxt->encoding);
				}
				else {
					xmlOutputBufferWrite(buf, 5, "UTF-8");
				}
				xmlOutputBufferWrite(buf, 4, "\" />");
				if(ctxt->format == 1)
					xmlOutputBufferWrite(buf, 1, "\n");
			}
			else {
				xmlOutputBufferWrite(buf, 1, ">");
			}
			/*
			 * C.3. Element Minimization and Empty Element Content
			 */
			xmlOutputBufferWrite(buf, 2, "</");
			if(cur->ns && (cur->ns->prefix != NULL)) {
				xmlOutputBufferWriteString(buf, (const char*)cur->ns->prefix);
				xmlOutputBufferWrite(buf, 1, ":");
			}
			xmlOutputBufferWriteString(buf, (const char*)cur->name);
			xmlOutputBufferWrite(buf, 1, ">");
		}
		return;
	}
	xmlOutputBufferWrite(buf, 1, ">");
	if(addmeta == 1) {
		if(ctxt->format == 1) {
			xmlOutputBufferWrite(buf, 1, "\n");
			if(xmlIndentTreeOutput)
				xmlOutputBufferWrite(buf, ctxt->indent_size * (ctxt->level + 1 > ctxt->indent_nr ? ctxt->indent_nr : ctxt->level + 1), ctxt->indent);
		}
		xmlOutputBufferWriteString(buf, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=");
		if(ctxt->encoding) {
			xmlOutputBufferWriteString(buf, (const char*)ctxt->encoding);
		}
		else {
			xmlOutputBufferWrite(buf, 5, "UTF-8");
		}
		xmlOutputBufferWrite(buf, 4, "\" />");
	}
	if((cur->type != XML_ELEMENT_NODE) && cur->content) {
		xmlOutputBufferWriteEscape(buf, cur->content, ctxt->escape);
	}
#if 0
	/*
	 * This was removed due to problems with HTML processors.
	 * See bug #345147.
	 */
	/*
	 * 4.8. Script and Style elements
	 */
	if((cur->type == XML_ELEMENT_NODE) && ((sstreq(cur->name, "script")) || (sstreq(cur->name, "style"))) &&
	    ((cur->ns == NULL) || (sstreq(cur->ns->href, XHTML_NS_NAME)))) {
		xmlNodePtr child = cur->children;
		while(child != NULL) {
			if(child->type == XML_TEXT_NODE) {
				if(!xmlStrchr(child->content, '<') && !xmlStrchr(child->content, '&') && !xmlStrstr(child->content, BAD_CAST "]]>")) {
					/* Nothing to escape, so just output as is... */
					/* FIXME: Should we do something about "--" also? */
					int level = ctxt->level;
					int indent = ctxt->format;

					ctxt->level = 0;
					ctxt->format = 0;
					xmlOutputBufferWriteString(buf, (const char*)child->content);
					/* (We cannot use xhtmlNodeDumpOutput() here because
					 * we wish to leave '>' unescaped!) */
					ctxt->level = level;
					ctxt->format = indent;
				}
				else {
					/* We must use a CDATA section.  Unfortunately,
					 * this will break CSS and JavaScript when read by
					 * a browser in HTML4-compliant mode. :-( */
					start = end = child->content;
					while(*end != '\0') {
						if(*end == ']' && *(end + 1) == ']' && *(end + 2) == '>') {
							end = end + 2;
							xmlOutputBufferWrite(buf, 9, "<![CDATA[");
							xmlOutputBufferWrite(buf, end - start, (const char*)start);
							xmlOutputBufferWrite(buf, 3, "]]>");
							start = end;
						}
						end++;
					}
					if(start != end) {
						xmlOutputBufferWrite(buf, 9, "<![CDATA[");
						xmlOutputBufferWrite(buf, end - start, (const char*)start);
						xmlOutputBufferWrite(buf, 3, "]]>");
					}
				}
			}
			else {
				int level = ctxt->level;
				int indent = ctxt->format;
				ctxt->level = 0;
				ctxt->format = 0;
				xhtmlNodeDumpOutput(ctxt, child);
				ctxt->level = level;
				ctxt->format = indent;
			}
			child = child->next;
		}
	}
#endif
	if(cur->children) {
		int indent = ctxt->format;
		if(format == 1) 
			xmlOutputBufferWrite(buf, 1, "\n");
		if(ctxt->level >= 0) 
			ctxt->level++;
		ctxt->format = format;
		xhtmlNodeListDumpOutput(ctxt, cur->children);
		if(ctxt->level > 0) 
			ctxt->level--;
		ctxt->format = indent;
		if((xmlIndentTreeOutput) && (format == 1))
			xmlOutputBufferWrite(buf, ctxt->indent_size * (ctxt->level > ctxt->indent_nr ? ctxt->indent_nr : ctxt->level), ctxt->indent);
	}
	xmlOutputBufferWrite(buf, 2, "</");
	if(cur->ns && (cur->ns->prefix != NULL)) {
		xmlOutputBufferWriteString(buf, (const char*)cur->ns->prefix);
		xmlOutputBufferWrite(buf, 1, ":");
	}
	xmlOutputBufferWriteString(buf, (const char*)cur->name);
	xmlOutputBufferWrite(buf, 1, ">");
}

#endif

/************************************************************************
*									*
*			Public entry points				*
*									*
************************************************************************/

/**
 * xmlSaveToFd:
 * @fd:  a file descriptor number
 * @encoding:  the encoding name to use or NULL
 * @options:  a set of xmlSaveOptions
 *
 * Create a document saving context serializing to a file descriptor
 * with the encoding and the options given.
 *
 * Returns a new serialization context or NULL in case of error.
 */
xmlSaveCtxtPtr xmlSaveToFd(int fd, const char * encoding, int options)
{
	xmlSaveCtxtPtr ret;

	ret = xmlNewSaveCtxt(encoding, options);
	if(!ret) return 0;
	ret->buf = xmlOutputBufferCreateFd(fd, ret->handler);
	if(ret->buf == NULL) {
		xmlFreeSaveCtxt(ret);
		return 0;
	}
	return ret;
}

/**
 * xmlSaveToFilename:
 * @filename:  a file name or an URL
 * @encoding:  the encoding name to use or NULL
 * @options:  a set of xmlSaveOptions
 *
 * Create a document saving context serializing to a filename or possibly
 * to an URL (but this is less reliable) with the encoding and the options
 * given.
 *
 * Returns a new serialization context or NULL in case of error.
 */
xmlSaveCtxtPtr xmlSaveToFilename(const char * filename, const char * encoding, int options)
{
	int compression = 0; /* @todo handle compression option */
	xmlSaveCtxt * ret = xmlNewSaveCtxt(encoding, options);
	if(ret) {
		ret->buf = xmlOutputBufferCreateFilename(filename, ret->handler, compression);
		if(ret->buf == NULL) {
			xmlFreeSaveCtxt(ret);
			return 0;
		}
	}
	return ret;
}
/**
 * xmlSaveToBuffer:
 * @buffer:  a buffer
 * @encoding:  the encoding name to use or NULL
 * @options:  a set of xmlSaveOptions
 *
 * Create a document saving context serializing to a buffer
 * with the encoding and the options given
 *
 * Returns a new serialization context or NULL in case of error.
 */
xmlSaveCtxtPtr xmlSaveToBuffer(xmlBufferPtr buffer, const char * encoding, int options)
{
	xmlSaveCtxt * ret = xmlNewSaveCtxt(encoding, options);
	if(ret) {
		xmlCharEncodingHandler * handler;
		if(encoding) {
			handler = xmlFindCharEncodingHandler(encoding);
			if(handler == NULL) {
				SAlloc::F(ret);
				return 0;
			}
		}
		else
			handler = NULL;
		{
			xmlOutputBuffer * out_buff = xmlOutputBufferCreateBuffer(buffer, handler);
			if(out_buff == NULL) {
				SAlloc::F(ret);
				if(handler)
					xmlCharEncCloseFunc(handler);
				return 0;
			}
			ret->buf = out_buff;
		}
	}
	return ret;
}

/**
 * xmlSaveToIO:
 * @iowrite:  an I/O write function
 * @ioclose:  an I/O close function
 * @ioctx:  an I/O handler
 * @encoding:  the encoding name to use or NULL
 * @options:  a set of xmlSaveOptions
 *
 * Create a document saving context serializing to a file descriptor
 * with the encoding and the options given
 *
 * Returns a new serialization context or NULL in case of error.
 */
xmlSaveCtxtPtr xmlSaveToIO(xmlOutputWriteCallback iowrite, xmlOutputCloseCallback ioclose, void * ioctx, const char * encoding, int options)
{
	xmlSaveCtxt * ret = xmlNewSaveCtxt(encoding, options);
	if(ret) {
		ret->buf = xmlOutputBufferCreateIO(iowrite, ioclose, ioctx, ret->handler);
		if(ret->buf == NULL) {
			xmlFreeSaveCtxt(ret);
			return 0;
		}
	}
	return ret;
}
/**
 * xmlSaveDoc:
 * @ctxt:  a document saving context
 * @doc:  a document
 *
 * Save a full document to a saving context
 * @todo The function is not fully implemented yet as it does not return the
 * byte count but 0 instead
 *
 * Returns the number of byte written or -1 in case of error
 */
long xmlSaveDoc(xmlSaveCtxtPtr ctxt, xmlDocPtr doc)
{
	return (ctxt && doc && xmlDocContentDumpOutput(ctxt, doc) >= 0) ? 0 : -1;
}
/**
 * xmlSaveTree:
 * @ctxt:  a document saving context
 * @node:  the top node of the subtree to save
 *
 * Save a subtree starting at the node parameter to a saving context
 * @todo The function is not fully implemented yet as it does not return the
 * byte count but 0 instead
 *
 * Returns the number of byte written or -1 in case of error
 */
long xmlSaveTree(xmlSaveCtxtPtr ctxt, xmlNodePtr P_Node)
{
	if(!ctxt || !P_Node) 
		return -1;
	else {
		xmlNodeDumpOutputInternal(ctxt, P_Node);
		return 0;
	}
}
/**
 * xmlSaveFlush:
 * @ctxt:  a document saving context
 *
 * Flush a document saving context, i.e. make sure that all bytes have
 * been output.
 *
 * Returns the number of byte written or -1 in case of error.
 */
int xmlSaveFlush(xmlSaveCtxtPtr ctxt)
{
	return (ctxt && ctxt->buf) ? xmlOutputBufferFlush(ctxt->buf) : -1;
}
/**
 * xmlSaveClose:
 * @ctxt:  a document saving context
 *
 * Close a document saving context, i.e. make sure that all bytes have
 * been output and free the associated data.
 *
 * Returns the number of byte written or -1 in case of error.
 */
int xmlSaveClose(xmlSaveCtxtPtr ctxt)
{
	int ret;
	if(!ctxt) 
		return -1;
	ret = xmlSaveFlush(ctxt);
	xmlFreeSaveCtxt(ctxt);
	return ret;
}
/**
 * xmlSaveSetEscape:
 * @ctxt:  a document saving context
 * @escape:  the escaping function
 *
 * Set a custom escaping function to be used for text in element content
 *
 * Returns 0 if successful or -1 in case of error.
 */
int xmlSaveSetEscape(xmlSaveCtxtPtr ctxt, xmlCharEncodingOutputFunc escape)
{
	if(!ctxt) return -1;
	ctxt->escape = escape;
	return 0;
}

/**
 * xmlSaveSetAttrEscape:
 * @ctxt:  a document saving context
 * @escape:  the escaping function
 *
 * Set a custom escaping function to be used for text in attribute content
 *
 * Returns 0 if successful or -1 in case of error.
 */
int xmlSaveSetAttrEscape(xmlSaveCtxtPtr ctxt, xmlCharEncodingOutputFunc escape)
{
	if(!ctxt) 
		return -1;
	ctxt->escapeAttr = escape;
	return 0;
}

/************************************************************************
*									*
*		Public entry points based on buffers			*
*									*
************************************************************************/

/**
 * xmlBufAttrSerializeTxtContent:
 * @buf:  and xmlBufPtr output
 * @doc:  the document
 * @attr: the attribute node
 * @string: the text content
 *
 * Serialize text attribute values to an xmlBufPtr
 */
void xmlBufAttrSerializeTxtContent(xmlBufPtr buf, xmlDocPtr doc, xmlAttrPtr attr, const xmlChar * string)
{
	xmlChar * base, * cur;
	if(string) {
		base = cur = (xmlChar*)string;
		while(*cur != 0) {
			if(*cur == '\n') {
				if(base != cur)
					xmlBufAdd(buf, base, cur - base);
				xmlBufAdd(buf, BAD_CAST "&#10;", 5);
				cur++;
				base = cur;
			}
			else if(*cur == '\r') {
				if(base != cur)
					xmlBufAdd(buf, base, cur - base);
				xmlBufAdd(buf, BAD_CAST "&#13;", 5);
				cur++;
				base = cur;
			}
			else if(*cur == '\t') {
				if(base != cur)
					xmlBufAdd(buf, base, cur - base);
				xmlBufAdd(buf, BAD_CAST "&#9;", 4);
				cur++;
				base = cur;
			}
			else if(*cur == '"') {
				if(base != cur)
					xmlBufAdd(buf, base, cur - base);
				xmlBufAdd(buf, BAD_CAST "&quot;", 6);
				cur++;
				base = cur;
			}
			else if(*cur == '<') {
				if(base != cur)
					xmlBufAdd(buf, base, cur - base);
				xmlBufAdd(buf, BAD_CAST "&lt;", 4);
				cur++;
				base = cur;
			}
			else if(*cur == '>') {
				if(base != cur)
					xmlBufAdd(buf, base, cur - base);
				xmlBufAdd(buf, BAD_CAST "&gt;", 4);
				cur++;
				base = cur;
			}
			else if(*cur == '&') {
				if(base != cur)
					xmlBufAdd(buf, base, cur - base);
				xmlBufAdd(buf, BAD_CAST "&amp;", 5);
				cur++;
				base = cur;
			}
			else if((*cur >= 0x80) && ((doc == NULL) ||
					(doc->encoding == NULL))) {
				/*
				* We assume we have UTF-8 content.
				*/
				uchar tmp[12];
				int val = 0, l = 1;

				if(base != cur)
					xmlBufAdd(buf, base, cur - base);
				if(*cur < 0xC0) {
					xmlSaveErr(XML_SAVE_NOT_UTF8, (xmlNode *)attr, 0);
					if(doc)
						doc->encoding = sstrdup(BAD_CAST "ISO-8859-1");
					xmlSerializeHexCharRef(tmp, *cur);
					xmlBufAdd(buf, (xmlChar*)tmp, -1);
					cur++;
					base = cur;
					continue;
				}
				else if(*cur < 0xE0) {
					val = (cur[0]) & 0x1F;
					val <<= 6;
					val |= (cur[1]) & 0x3F;
					l = 2;
				}
				else if(*cur < 0xF0) {
					val = (cur[0]) & 0x0F;
					val <<= 6;
					val |= (cur[1]) & 0x3F;
					val <<= 6;
					val |= (cur[2]) & 0x3F;
					l = 3;
				}
				else if(*cur < 0xF8) {
					val = (cur[0]) & 0x07;
					val <<= 6;
					val |= (cur[1]) & 0x3F;
					val <<= 6;
					val |= (cur[2]) & 0x3F;
					val <<= 6;
					val |= (cur[3]) & 0x3F;
					l = 4;
				}
				if((l == 1) || (!IS_CHAR(val))) {
					xmlSaveErr(XML_SAVE_CHAR_INVALID, (xmlNode *)attr, 0);
					if(doc)
						doc->encoding = sstrdup(BAD_CAST "ISO-8859-1");
					xmlSerializeHexCharRef(tmp, *cur);
					xmlBufAdd(buf, (xmlChar*)tmp, -1);
					cur++;
					base = cur;
					continue;
				}
				/*
				* We could do multiple things here. Just save
				* as a char ref
				*/
				xmlSerializeHexCharRef(tmp, val);
				xmlBufAdd(buf, (xmlChar*)tmp, -1);
				cur += l;
				base = cur;
			}
			else {
				cur++;
			}
		}
		if(base != cur)
			xmlBufAdd(buf, base, cur - base);
	}
}

/**
 * xmlAttrSerializeTxtContent:
 * @buf:  the XML buffer output
 * @doc:  the document
 * @attr: the attribute node
 * @string: the text content
 *
 * Serialize text attribute values to an xml simple buffer
 */
void xmlAttrSerializeTxtContent(xmlBufferPtr buf, xmlDocPtr doc, xmlAttrPtr attr, const xmlChar * string)
{
	if(buf && string) {
		xmlBufPtr buffer = xmlBufFromBuffer(buf);
		if(buffer) {
			xmlBufAttrSerializeTxtContent(buffer, doc, attr, string);
			xmlBufBackToBuffer(buffer);
		}
	}
}

/**
 * xmlNodeDump:
 * @buf:  the XML buffer output
 * @doc:  the document
 * @cur:  the current node
 * @level: the imbrication level for indenting
 * @format: is formatting allowed
 *
 * Dump an XML node, recursive behaviour,children are printed too.
 * Note that @format = 1 provide node indenting only if xmlIndentTreeOutput = 1
 * or xmlKeepBlanksDefault(0) was called
 * Since this is using xmlBuffer structures it is limited to 2GB and somehow
 * deprecated, use xmlBufNodeDump() instead.
 *
 * Returns the number of bytes written to the buffer or -1 in case of error
 */
int xmlNodeDump(xmlBufferPtr buf, xmlDocPtr doc, xmlNodePtr cur, int level, int format)
{
	int ret = -1;
	if(buf && cur) {
		xmlBufPtr buffer = xmlBufFromBuffer(buf);
		if(buffer) {
			ret = xmlBufNodeDump(buffer, doc, cur, level, format);
			xmlBufBackToBuffer(buffer);
			if(ret > INT_MAX)
				ret = -1;
		}
	}
	return ret;
}
/**
 * xmlBufNodeDump:
 * @buf:  the XML buffer output
 * @doc:  the document
 * @cur:  the current node
 * @level: the imbrication level for indenting
 * @format: is formatting allowed
 *
 * Dump an XML node, recursive behaviour,children are printed too.
 * Note that @format = 1 provide node indenting only if xmlIndentTreeOutput = 1
 * or xmlKeepBlanksDefault(0) was called
 *
 * Returns the number of bytes written to the buffer, in case of error 0
 *     is returned or @buf stores the error
 */
size_t xmlBufNodeDump(xmlBufPtr buf, xmlDocPtr doc, xmlNodePtr cur, int level, int format)
{
	int    ret = -1;
	size_t use;
	xmlOutputBufferPtr outbuf;
	int oldalloc;
	xmlInitParser();
	if(!cur) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNodeDump : node == NULL\n");
#endif
	}
	else if(!buf) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNodeDump : buf == NULL\n");
#endif
	}
	else {
		outbuf = (xmlOutputBufferPtr)SAlloc::M(sizeof(xmlOutputBuffer));
		if(outbuf == NULL) {
			xmlSaveErrMemory("creating buffer");
		}
		else {
			memzero(outbuf, (size_t)sizeof(xmlOutputBuffer));
			outbuf->buffer = buf;
			outbuf->encoder = NULL;
			outbuf->writecallback = NULL;
			outbuf->closecallback = NULL;
			outbuf->context = NULL;
			outbuf->written = 0;
			use = xmlBufUse(buf);
			oldalloc = xmlBufGetAllocationScheme(buf);
			xmlBufSetAllocationScheme(buf, XML_BUFFER_ALLOC_DOUBLEIT);
			xmlNodeDumpOutput(outbuf, doc, cur, level, format, 0);
			xmlBufSetAllocationScheme(buf, (xmlBufferAllocationScheme)oldalloc);
			SAlloc::F(outbuf);
			ret = xmlBufUse(buf) - use;
		}
	}
	return ret;
}
/**
 * xmlElemDump:
 * @f:  the FILE * for the output
 * @doc:  the document
 * @cur:  the current node
 *
 * Dump an XML/HTML node, recursive behaviour, children are printed too.
 */
void xmlElemDump(FILE * f, xmlDocPtr doc, xmlNode * cur)
{
	xmlInitParser();
	if(!cur) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlElemDump : cur == NULL\n");
#endif
	}
	else {
#ifdef DEBUG_TREE
		if(!doc) {
			xmlGenericError(0, "xmlElemDump : doc == NULL\n");
		}
#endif
		xmlOutputBufferPtr outbuf = xmlOutputBufferCreateFile(f, 0);
		if(outbuf) {
			if(doc && (doc->type == XML_HTML_DOCUMENT_NODE)) {
#ifdef LIBXML_HTML_ENABLED
				htmlNodeDumpOutput(outbuf, doc, cur, 0);
#else
				xmlSaveErr(XML_ERR_INTERNAL_ERROR, cur, "HTML support not compiled in\n");
#endif
			}
			else
				xmlNodeDumpOutput(outbuf, doc, cur, 0, 1, 0);
			xmlOutputBufferClose(outbuf);
		}
	}
}

/************************************************************************
*									*
*		Saving functions front-ends				*
*									*
************************************************************************/

/**
 * xmlNodeDumpOutput:
 * @buf:  the XML buffer output
 * @doc:  the document
 * @cur:  the current node
 * @level: the imbrication level for indenting
 * @format: is formatting allowed
 * @encoding:  an optional encoding string
 *
 * Dump an XML node, recursive behaviour, children are printed too.
 * Note that @format = 1 provide node indenting only if xmlIndentTreeOutput = 1
 * or xmlKeepBlanksDefault(0) was called
 */
void xmlNodeDumpOutput(xmlOutputBufferPtr buf, xmlDocPtr doc, xmlNodePtr cur, int level, int format, const char * encoding)
{
	xmlSaveCtxt ctxt;
#ifdef LIBXML_HTML_ENABLED
	xmlDtd * dtd;
	int is_xhtml = 0;
#endif
	xmlInitParser();
	if(buf && cur) {
		SETIFZ(encoding, "UTF-8");
		MEMSZERO(ctxt);
		ctxt.doc = doc;
		ctxt.buf = buf;
		ctxt.level = level;
		ctxt.format = BIN(format);
		ctxt.encoding = (const xmlChar*)encoding;
		xmlSaveCtxtInit(&ctxt);
		ctxt.options |= XML_SAVE_AS_XML;
#ifdef LIBXML_HTML_ENABLED
		dtd = xmlGetIntSubset(doc);
		if(dtd != NULL) {
			is_xhtml = xmlIsXHTML(dtd->SystemID, dtd->ExternalID);
			if(is_xhtml < 0)
				is_xhtml = 0;
		}
		if(is_xhtml)
			xhtmlNodeDumpOutput(&ctxt, cur);
		else
#endif
		xmlNodeDumpOutputInternal(&ctxt, cur);
	}
}

/**
 * xmlDocDumpFormatMemoryEnc:
 * @out_doc:  Document to generate XML text from
 * @doc_txt_ptr:  Memory pointer for allocated XML text
 * @doc_txt_len:  Length of the generated XML text
 * @txt_encoding:  Character encoding to use when generating XML text
 * @format:  should formatting spaces been added
 *
 * Dump the current DOM tree into memory using the character encoding specified
 * by the caller.  Note it is up to the caller of this function to free the
 * allocated memory with SAlloc::F().
 * Note that @format = 1 provide node indenting only if xmlIndentTreeOutput = 1
 * or xmlKeepBlanksDefault(0) was called
 */
void xmlDocDumpFormatMemoryEnc(xmlDocPtr out_doc, xmlChar ** doc_txt_ptr, int * doc_txt_len, const char * txt_encoding, int format)
{
	xmlSaveCtxt ctxt;
	int dummy = 0;
	xmlOutputBuffer * out_buff = NULL;
	xmlCharEncodingHandler * conv_hdlr = NULL;
	SETIFZ(doc_txt_len, &dummy); /*  Continue, caller just won't get length */
	if(doc_txt_ptr == NULL) {
		*doc_txt_len = 0;
	}
	else {
		*doc_txt_ptr = NULL;
		*doc_txt_len = 0;
		if(out_doc) { /*  No document, no output  */
			/*
			*  Validate the encoding value, if provided.
			*  This logic is copied from xmlSaveFileEnc.
			*/
			SETIFZ(txt_encoding, (const char*)out_doc->encoding);
			if(txt_encoding != NULL) {
				conv_hdlr = xmlFindCharEncodingHandler(txt_encoding);
				if(conv_hdlr == NULL) {
					xmlSaveErr(XML_SAVE_UNKNOWN_ENCODING, (xmlNode *)out_doc, txt_encoding);
					return;
				}
			}
			if((out_buff = xmlAllocOutputBuffer(conv_hdlr)) == NULL) {
				xmlSaveErrMemory("creating buffer");
			}
			else {
				MEMSZERO(ctxt);
				ctxt.doc = out_doc;
				ctxt.buf = out_buff;
				ctxt.level = 0;
				ctxt.format = format ? 1 : 0;
				ctxt.encoding = (const xmlChar*)txt_encoding;
				xmlSaveCtxtInit(&ctxt);
				ctxt.options |= XML_SAVE_AS_XML;
				xmlDocContentDumpOutput(&ctxt, out_doc);
				xmlOutputBufferFlush(out_buff);
				if(out_buff->conv != NULL) {
					*doc_txt_len = xmlBufUse(out_buff->conv);
					*doc_txt_ptr = xmlStrndup(xmlBufContent(out_buff->conv), *doc_txt_len);
				}
				else {
					*doc_txt_len = xmlBufUse(out_buff->buffer);
					*doc_txt_ptr = xmlStrndup(xmlBufContent(out_buff->buffer), *doc_txt_len);
				}
				xmlOutputBufferClose(out_buff);
				if((*doc_txt_ptr == NULL) && (*doc_txt_len > 0)) {
					*doc_txt_len = 0;
					xmlSaveErrMemory("creating output");
				}
			}
		}
	}
}
/**
 * xmlDocDumpMemory:
 * @cur:  the document
 * @mem:  OUT: the memory pointer
 * @size:  OUT: the memory length
 *
 * Dump an XML document in memory and return the #xmlChar * and it's size
 * in bytes. It's up to the caller to free the memory with SAlloc::F().
 * The resulting byte array is zero terminated, though the last 0 is not
 * included in the returned size.
 */
void xmlDocDumpMemory(xmlDocPtr cur, xmlChar** mem, int * size)
{
	xmlDocDumpFormatMemoryEnc(cur, mem, size, NULL, 0);
}
/**
 * xmlDocDumpFormatMemory:
 * @cur:  the document
 * @mem:  OUT: the memory pointer
 * @size:  OUT: the memory length
 * @format:  should formatting spaces been added
 *
 *
 * Dump an XML document in memory and return the #xmlChar * and it's size.
 * It's up to the caller to free the memory with SAlloc::F().
 * Note that @format = 1 provide node indenting only if xmlIndentTreeOutput = 1
 * or xmlKeepBlanksDefault(0) was called
 */
void xmlDocDumpFormatMemory(xmlDocPtr cur, xmlChar** mem, int * size, int format)
{
	xmlDocDumpFormatMemoryEnc(cur, mem, size, NULL, format);
}
/**
 * xmlDocDumpMemoryEnc:
 * @out_doc:  Document to generate XML text from
 * @doc_txt_ptr:  Memory pointer for allocated XML text
 * @doc_txt_len:  Length of the generated XML text
 * @txt_encoding:  Character encoding to use when generating XML text
 *
 * Dump the current DOM tree into memory using the character encoding specified
 * by the caller.  Note it is up to the caller of this function to free the
 * allocated memory with SAlloc::F().
 */
void xmlDocDumpMemoryEnc(xmlDocPtr out_doc, xmlChar ** doc_txt_ptr, int * doc_txt_len, const char * txt_encoding)
{
	xmlDocDumpFormatMemoryEnc(out_doc, doc_txt_ptr, doc_txt_len, txt_encoding, 0);
}

/**
 * xmlDocFormatDump:
 * @f:  the FILE*
 * @cur:  the document
 * @format: should formatting spaces been added
 *
 * Dump an XML document to an open FILE.
 *
 * returns: the number of bytes written or -1 in case of failure.
 * Note that @format = 1 provide node indenting only if xmlIndentTreeOutput = 1
 * or xmlKeepBlanksDefault(0) was called
 */
int xmlDocFormatDump(FILE * f, xmlDocPtr cur, int format)
{
	xmlSaveCtxt ctxt;
	xmlOutputBufferPtr buf;
	const char * encoding;
	xmlCharEncodingHandler * handler = NULL;
	int ret;
	if(!cur) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlDocDump : document == NULL\n");
#endif
		return -1;
	}
	encoding = (const char*)cur->encoding;
	if(encoding) {
		handler = xmlFindCharEncodingHandler(encoding);
		if(handler == NULL) {
			SAlloc::F((char*)cur->encoding);
			cur->encoding = NULL;
			encoding = NULL;
		}
	}
	buf = xmlOutputBufferCreateFile(f, handler);
	if(!buf)
		return -1;
	MEMSZERO(ctxt);
	ctxt.doc = cur;
	ctxt.buf = buf;
	ctxt.level = 0;
	ctxt.format = format ? 1 : 0;
	ctxt.encoding = (const xmlChar*)encoding;
	xmlSaveCtxtInit(&ctxt);
	ctxt.options |= XML_SAVE_AS_XML;
	xmlDocContentDumpOutput(&ctxt, cur);
	ret = xmlOutputBufferClose(buf);
	return ret;
}
/**
 * xmlDocDump:
 * @f:  the FILE*
 * @cur:  the document
 *
 * Dump an XML document to an open FILE.
 *
 * returns: the number of bytes written or -1 in case of failure.
 */
int xmlDocDump(FILE * f, xmlDocPtr cur) 
{
	return xmlDocFormatDump(f, cur, 0);
}
/**
 * xmlSaveFileTo:
 * @buf:  an output I/O buffer
 * @cur:  the document
 * @encoding:  the encoding if any assuming the I/O layer handles the trancoding
 *
 * Dump an XML document to an I/O buffer.
 * Warning ! This call xmlOutputBufferClose() on buf which is not available
 * after this call.
 *
 * returns: the number of bytes written or -1 in case of failure.
 */
int xmlSaveFileTo(xmlOutputBufferPtr buf, xmlDocPtr cur, const char * encoding)
{
	int ret = -1;
	if(buf) {
		if(!cur) {
			xmlOutputBufferClose(buf);
		}
		else {
			xmlSaveCtxt ctxt;
			MEMSZERO(ctxt);
			ctxt.doc = cur;
			ctxt.buf = buf;
			ctxt.level = 0;
			ctxt.format = 0;
			ctxt.encoding = (const xmlChar*)encoding;
			xmlSaveCtxtInit(&ctxt);
			ctxt.options |= XML_SAVE_AS_XML;
			xmlDocContentDumpOutput(&ctxt, cur);
			ret = xmlOutputBufferClose(buf);
		}
	}
	return ret;
}
/**
 * xmlSaveFormatFileTo:
 * @buf:  an output I/O buffer
 * @cur:  the document
 * @encoding:  the encoding if any assuming the I/O layer handles the trancoding
 * @format: should formatting spaces been added
 *
 * Dump an XML document to an I/O buffer.
 * Warning ! This call xmlOutputBufferClose() on buf which is not available
 * after this call.
 *
 * returns: the number of bytes written or -1 in case of failure.
 */
int xmlSaveFormatFileTo(xmlOutputBufferPtr buf, xmlDocPtr cur, const char * encoding, int format)
{
	int    ret = -1;
	if(buf)  {
		if(!cur || !oneof2(cur->type, XML_DOCUMENT_NODE, XML_HTML_DOCUMENT_NODE)) {
			xmlOutputBufferClose(buf);
		}
		else {
			xmlSaveCtxt ctxt;
			MEMSZERO(ctxt);
			ctxt.doc = cur;
			ctxt.buf = buf;
			ctxt.level = 0;
			ctxt.format = BIN(format);
			ctxt.encoding = (const xmlChar*)encoding;
			xmlSaveCtxtInit(&ctxt);
			ctxt.options |= XML_SAVE_AS_XML;
			xmlDocContentDumpOutput(&ctxt, cur);
			ret = xmlOutputBufferClose(buf);
		}
	}
	return ret;
}
/**
 * xmlSaveFormatFileEnc:
 * @filename:  the filename or URL to output
 * @cur:  the document being saved
 * @encoding:  the name of the encoding to use or NULL.
 * @format:  should formatting spaces be added.
 *
 * Dump an XML document to a file or an URL.
 *
 * Returns the number of bytes written or -1 in case of error.
 * Note that @format = 1 provide node indenting only if xmlIndentTreeOutput = 1
 * or xmlKeepBlanksDefault(0) was called
 */
int xmlSaveFormatFileEnc(const char * filename, xmlDocPtr cur, const char * encoding, int format)
{
	int    ret = -1;
	xmlSaveCtxt ctxt;
	xmlOutputBufferPtr buf;
	xmlCharEncodingHandler * handler = NULL;
	if(cur) {
		SETIFZ(encoding, (const char *)cur->encoding);
		if(encoding) {
			handler = xmlFindCharEncodingHandler(encoding);
			if(handler == NULL)
				return -1;
		}
#ifdef HAVE_ZLIB_H
		if(cur->compression < 0) cur->compression = xmlGetCompressMode();
#endif
		// save the content to a temp buffer.
		buf = xmlOutputBufferCreateFilename(filename, handler, cur->compression);
		if(buf) {
			MEMSZERO(ctxt);
			ctxt.doc = cur;
			ctxt.buf = buf;
			ctxt.level = 0;
			ctxt.format = BIN(format);
			ctxt.encoding = (const xmlChar*)encoding;
			xmlSaveCtxtInit(&ctxt);
			ctxt.options |= XML_SAVE_AS_XML;
			xmlDocContentDumpOutput(&ctxt, cur);
			ret = xmlOutputBufferClose(buf);
		}
	}
	return ret;
}

/**
 * xmlSaveFileEnc:
 * @filename:  the filename (or URL)
 * @cur:  the document
 * @encoding:  the name of an encoding (or NULL)
 *
 * Dump an XML document, converting it to the given encoding
 *
 * returns: the number of bytes written or -1 in case of failure.
 */
int xmlSaveFileEnc(const char * filename, xmlDocPtr cur, const char * encoding)
{
	return (xmlSaveFormatFileEnc(filename, cur, encoding, 0));
}

/**
 * xmlSaveFormatFile:
 * @filename:  the filename (or URL)
 * @cur:  the document
 * @format:  should formatting spaces been added
 *
 * Dump an XML document to a file. Will use compression if
 * compiled in and enabled. If @filename is "-" the stdout file is
 * used. If @format is set then the document will be indented on output.
 * Note that @format = 1 provide node indenting only if xmlIndentTreeOutput = 1
 * or xmlKeepBlanksDefault(0) was called
 *
 * returns: the number of bytes written or -1 in case of failure.
 */
int xmlSaveFormatFile(const char * filename, xmlDocPtr cur, int format)
{
	return xmlSaveFormatFileEnc(filename, cur, NULL, format);
}
/**
 * xmlSaveFile:
 * @filename:  the filename (or URL)
 * @cur:  the document
 *
 * Dump an XML document to a file. Will use compression if
 * compiled in and enabled. If @filename is "-" the stdout file is
 * used.
 * returns: the number of bytes written or -1 in case of failure.
 */
int xmlSaveFile(const char * filename, xmlDocPtr cur)
{
	return(xmlSaveFormatFileEnc(filename, cur, NULL, 0));
}

#endif /* LIBXML_OUTPUT_ENABLED */

#define bottom_xmlsave
#include "elfgcchack.h"
