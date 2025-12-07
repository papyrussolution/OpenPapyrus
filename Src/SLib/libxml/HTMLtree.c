/*
 * HTMLtree.c : implementation of access function for an HTML tree.
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */
#include <slib-internal.h>
#pragma hdrstop
#ifdef LIBXML_HTML_ENABLED
// 
// Getting/Setting encoding meta tags
// 
/**
 * htmlGetMetaEncoding:
 * @doc:  the document
 *
 * Encoding definition lookup in the Meta tags
 *
 * Returns the current encoding as flagged in the HTML source
 */
const xmlChar * htmlGetMetaEncoding(htmlDocPtr doc) 
{
	const xmlChar * encoding = 0;
	const xmlChar * content;
	if(doc) {
		htmlNodePtr cur = doc->children;
		// 
		// Search the html
		// 
		while(cur) {
			if((cur->type == XML_ELEMENT_NODE) && cur->name) {
				if(sstreq(cur->name, "html"))
					break;
				if(sstreq(cur->name, "head"))
					goto found_head;
				if(sstreq(cur->name, "meta"))
					goto found_meta;
			}
			cur = cur->next;
		}
		if(!cur)
			return NULL;
		cur = cur->children;
		// 
		// Search the head
		// 
		while(cur) {
			if((cur->type == XML_ELEMENT_NODE) && cur->name) {
				if(sstreq(cur->name, "head"))
					break;
				if(sstreq(cur->name, "meta"))
					goto found_meta;
			}
			cur = cur->next;
		}
		if(!cur)
			return NULL;
found_head:
		cur = cur->children;
		//
		// Search the meta elements
		//
found_meta:
		while(cur) {
			if((cur->type == XML_ELEMENT_NODE) && cur->name) {
				if(sstreq(cur->name, "meta")) {
					xmlAttr * attr = cur->properties;
					int http = 0;
					const xmlChar * value;
					content = NULL;
					while(attr) {
						if(attr->children && (attr->children->type == XML_TEXT_NODE) && (attr->children->next == NULL)) {
							value = attr->children->content;
							if(sstreqi_ascii(attr->name, reinterpret_cast<const xmlChar *>("http-equiv")) && sstreqi_ascii(value, reinterpret_cast<const xmlChar *>("Content-Type")))
								http = 1;
							else if(value && sstreqi_ascii(attr->name, reinterpret_cast<const xmlChar *>("content")))
								content = value;
							if(http && content)
								goto found_content;
						}
						attr = attr->next;
					}
				}
			}
			cur = cur->next;
		}
		return NULL;
found_content:
		encoding = xmlStrstr(content, reinterpret_cast<const xmlChar *>("charset="));
		SETIFZ(encoding, xmlStrstr(content, reinterpret_cast<const xmlChar *>("Charset=")));
		SETIFZ(encoding, xmlStrstr(content, reinterpret_cast<const xmlChar *>("CHARSET=")));
		if(encoding) {
			encoding += 8;
		}
		else {
			encoding = xmlStrstr(content, reinterpret_cast<const xmlChar *>("charset ="));
			SETIFZ(encoding, xmlStrstr(content, reinterpret_cast<const xmlChar *>("Charset =")));
			SETIFZ(encoding, xmlStrstr(content, reinterpret_cast<const xmlChar *>("CHARSET =")));
			if(encoding)
				encoding += 9;
		}
		if(encoding) {
			while(oneof2(*encoding, ' ', '\t')) 
				encoding++;
		}
	}
	return encoding;
}
/**
 * htmlSetMetaEncoding:
 * @doc:  the document
 * @encoding:  the encoding string
 *
 * Sets the current encoding in the Meta tags
 * NOTE: this will not change the document content encoding, just
 * the META flag associated.
 *
 * Returns 0 in case of success and -1 in case of error
 */
int htmlSetMetaEncoding(htmlDocPtr doc, const xmlChar * encoding) 
{
	htmlNodePtr cur, meta = NULL, head = NULL;
	const xmlChar * content = NULL;
	char newcontent[100];
	newcontent[0] = 0;
	if(!doc)
		return -1;
	// html isn't a real encoding it's just libxml2 way to get entities 
	if(sstreqi_ascii(encoding, reinterpret_cast<const xmlChar *>("html")))
		return -1;
	if(encoding) {
		snprintf(newcontent, sizeof(newcontent), "text/html; charset=%s", (char *)encoding);
		newcontent[sizeof(newcontent) - 1] = 0;
	}
	cur = doc->children;
	/*
	 * Search the html
	 */
	while(cur) {
		if((cur->type == XML_ELEMENT_NODE) && cur->name) {
			if(sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("html")))
				break;
			if(sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("head")))
				goto found_head;
			if(sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("meta")))
				goto found_meta;
		}
		cur = cur->next;
	}
	if(!cur)
		return -1;
	cur = cur->children;
	/*
	 * Search the head
	 */
	while(cur) {
		if((cur->type == XML_ELEMENT_NODE) && cur->name) {
			if(sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("head")))
				break;
			if(sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("meta"))) {
				head = cur->P_ParentNode;
				goto found_meta;
			}
		}
		cur = cur->next;
	}
	if(!cur)
		return -1;
found_head:
	head = cur;
	if(cur->children == NULL)
		goto create;
	cur = cur->children;
found_meta:
	// 
	// Search and update all the remaining the meta elements carrying encoding informations
	// 
	while(cur) {
		if((cur->type == XML_ELEMENT_NODE) && cur->name) {
			if(sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("meta"))) {
				xmlAttr * attr = cur->properties;
				int http = 0;
				const xmlChar * value;
				content = NULL;
				while(attr) {
					if(attr->children && (attr->children->type == XML_TEXT_NODE) && !attr->children->next) {
						value = attr->children->content;
						if(sstreqi_ascii(attr->name, reinterpret_cast<const xmlChar *>("http-equiv")) && sstreqi_ascii(value, reinterpret_cast<const xmlChar *>("Content-Type")))
							http = 1;
						else {
							if(value && sstreqi_ascii(attr->name, reinterpret_cast<const xmlChar *>("content")))
								content = value;
						}
						if(http && content)
							break;
					}
					attr = attr->next;
				}
				if(http && content) {
					meta = cur;
					break;
				}
			}
		}
		cur = cur->next;
	}
create:
	if(!meta) {
		if(encoding && head) {
			// 
			// Create a new Meta element with the right attributes
			// 
			meta = xmlNewDocNode(doc, NULL, reinterpret_cast<const xmlChar *>("meta"), 0);
			if(head->children == NULL)
				xmlAddChild(head, meta);
			else
				xmlAddPrevSibling(head->children, meta);
			xmlNewProp(meta, reinterpret_cast<const xmlChar *>("http-equiv"), reinterpret_cast<const xmlChar *>("Content-Type"));
			xmlNewProp(meta, reinterpret_cast<const xmlChar *>("content"), BAD_CAST newcontent);
		}
	}
	else {
		// remove the meta tag if NULL is passed 
		if(encoding == NULL) {
			xmlUnlinkNode(meta);
			xmlFreeNode(meta);
		}
		// change the document only if there is a real encoding change
		else if(xmlStrcasestr(content, encoding) == NULL) {
			xmlSetProp(meta, reinterpret_cast<const xmlChar *>("content"), BAD_CAST newcontent);
		}
	}
	return 0;
}
/**
 * booleanHTMLAttrs:
 *
 * These are the HTML attributes which will be output
 * in minimized form, i.e. <option selected="selected"> will be
 * output as <option selected>, as per XSLT 1.0 16.2 "HTML Output Method"
 *
 */
static const char * htmlBooleanAttrs[] = 
	{ "checked", "compact", "declare", "defer", "disabled", "ismap", "multiple", "nohref", "noresize", "noshade", "nowrap", "readonly", "selected", NULL };
/**
 * htmlIsBooleanAttr:
 * @name:  the name of the attribute to check
 *
 * Determine if a given attribute is a boolean attribute.
 *
 * returns: false if the attribute is not boolean, true otherwise.
 */
int htmlIsBooleanAttr(const xmlChar * name)
{
	for(size_t i = 0; htmlBooleanAttrs[i]; i++) {
		if(sstreqi_ascii((const xmlChar *)htmlBooleanAttrs[i], name))
			return 1;
	}
	return 0;
}

#ifdef LIBXML_OUTPUT_ENABLED
/*
 * private routine exported from xmlIO.c
 */
xmlOutputBuffer * xmlAllocOutputBufferInternal(xmlCharEncodingHandler * encoder);
// 
// Output error handlers
// 
/**
 * htmlSaveErrMemory:
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void htmlSaveErrMemory(const char * extra) { __xmlSimpleError(XML_FROM_OUTPUT, XML_ERR_NO_MEMORY, NULL, NULL, extra); }
/**
 * htmlSaveErr:
 * @code:  the error number
 * @node:  the location of the error.
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void FASTCALL htmlSaveErr(int code, xmlNode * pNode, const char * extra)
{
	const char * msg = NULL;
	switch(code) {
		case XML_SAVE_NOT_UTF8: msg = "string is not in UTF-8\n"; break;
		case XML_SAVE_CHAR_INVALID: msg = "invalid character value\n"; break;
		case XML_SAVE_UNKNOWN_ENCODING: msg = "unknown encoding %s\n"; break;
		case XML_SAVE_NO_DOCTYPE: msg = "HTML has no DOCTYPE\n"; break;
		default: msg = "unexpected error number\n";
	}
	__xmlSimpleError(XML_FROM_OUTPUT, code, pNode, msg, extra);
}
// 
// Dumping HTML tree content to a simple buffer
// 
/**
 * htmlBufNodeDumpFormat:
 * @buf:  the xmlBufPtr output
 * @doc:  the document
 * @cur:  the current node
 * @format:  should formatting spaces been added
 *
 * Dump an HTML node, recursive behaviour,children are printed too.
 *
 * Returns the number of byte written or -1 in case of error
 */
static size_t htmlBufNodeDumpFormat(xmlBuf * buf, xmlDoc * doc, xmlNode * cur, int format) 
{
	int ret = -1;
	if(cur && buf) {
		xmlOutputBuffer * outbuf = static_cast<xmlOutputBuffer *>(SAlloc::M(sizeof(xmlOutputBuffer)));
		if(!outbuf)
			htmlSaveErrMemory("allocating HTML output buffer");
		else {
			memzero(outbuf, sizeof(xmlOutputBuffer));
			outbuf->buffer = buf;
			outbuf->encoder = NULL;
			outbuf->writecallback = NULL;
			outbuf->closecallback = NULL;
			outbuf->context = NULL;
			outbuf->written = 0;
			size_t use = xmlBufUse(buf);
			htmlNodeDumpFormatOutput(outbuf, doc, cur, NULL, format);
			SAlloc::F(outbuf);
			ret = static_cast<int>(xmlBufUse(buf) - use);
		}
	}
	return ret;
}
/**
 * htmlNodeDump:
 * @buf:  the HTML buffer output
 * @doc:  the document
 * @cur:  the current node
 *
 * Dump an HTML node, recursive behaviour,children are printed too,
 * and formatting returns are added.
 *
 * Returns the number of byte written or -1 in case of error
 */
int htmlNodeDump(xmlBuffer * buf, xmlDoc * doc, xmlNode * cur) 
{
	xmlBuf * buffer;
	size_t ret;
	if(!buf || !cur)
		return -1;
	xmlInitParser();
	buffer = xmlBufFromBuffer(buf);
	if(!buffer)
		return -1;
	ret = htmlBufNodeDumpFormat(buffer, doc, cur, 1);
	xmlBufBackToBuffer(buffer);
	return (ret > INT_MAX) ? -1 : static_cast<int>(ret);
}
/**
 * htmlNodeDumpFileFormat:
 * @out:  the FILE pointer
 * @doc:  the document
 * @cur:  the current node
 * @encoding: the document encoding
 * @format:  should formatting spaces been added
 *
 * Dump an HTML node, recursive behaviour,children are printed too.
 *
 * @todo if encoding == NULL try to save in the doc encoding
 *
 * returns: the number of byte written or -1 in case of failure.
 */
int htmlNodeDumpFileFormat(FILE * out, xmlDoc * doc, xmlNode * cur, const char * encoding, int format) 
{
	xmlOutputBuffer * buf;
	xmlCharEncodingHandler * handler = NULL;
	xmlInitParser();
	if(encoding) {
		xmlCharEncoding enc = xmlParseCharEncoding(encoding);
		if(enc != XML_CHAR_ENCODING_UTF8) {
			handler = xmlFindCharEncodingHandler(encoding);
			if(handler == NULL)
				htmlSaveErr(XML_SAVE_UNKNOWN_ENCODING, NULL, encoding);
		}
	}
	/*
	 * Fallback to HTML or ASCII when the encoding is unspecified
	 */
	SETIFZ(handler, xmlFindCharEncodingHandler("HTML"));
	SETIFZ(handler, xmlFindCharEncodingHandler("ascii"));
	/*
	 * save the content to a temp buffer.
	 */
	buf = xmlOutputBufferCreateFile(out, handler);
	if(!buf) 
		return 0;
	htmlNodeDumpFormatOutput(buf, doc, cur, encoding, format);
	return xmlOutputBufferClose(buf);
}
/**
 * htmlNodeDumpFile:
 * @out:  the FILE pointer
 * @doc:  the document
 * @cur:  the current node
 *
 * Dump an HTML node, recursive behaviour,children are printed too,
 * and formatting returns are added.
 */
void htmlNodeDumpFile(FILE * out, xmlDoc * doc, xmlNode * cur) 
{
	htmlNodeDumpFileFormat(out, doc, cur, NULL, 1);
}
/**
 * htmlDocDumpMemoryFormat:
 * @cur:  the document
 * @mem:  OUT: the memory pointer
 * @size:  OUT: the memory length
 * @format:  should formatting spaces been added
 *
 * Dump an HTML document in memory and return the xmlChar * and it's size.
 * It's up to the caller to free the memory.
 */
void htmlDocDumpMemoryFormat(xmlDoc * cur, xmlChar** mem, int * size, int format) 
{
	xmlCharEncodingHandler * handler = NULL;
	xmlInitParser();
	if(mem && size) {
		if(!cur) {
			*mem = NULL;
			*size = 0;
		}
		else {
			const char * encoding = (const char *)htmlGetMetaEncoding(cur);
			if(encoding) {
				xmlCharEncoding enc = xmlParseCharEncoding(encoding);
				if(enc != cur->charset) {
					if(cur->charset != XML_CHAR_ENCODING_UTF8) {
						// Not supported yet
						*mem = NULL;
						*size = 0;
						return;
					}
					handler = xmlFindCharEncodingHandler(encoding);
					if(!handler)
						htmlSaveErr(XML_SAVE_UNKNOWN_ENCODING, NULL, encoding);
				}
				else
					handler = xmlFindCharEncodingHandler(encoding);
			}
			// Fallback to HTML or ASCII when the encoding is unspecified
			SETIFZ(handler, xmlFindCharEncodingHandler("HTML"));
			SETIFZ(handler, xmlFindCharEncodingHandler("ascii"));
			{
				xmlOutputBuffer * buf = xmlAllocOutputBufferInternal(handler);
				if(!buf) {
					*mem = NULL;
					*size = 0;
				}
				else {
					htmlDocContentDumpFormatOutput(buf, cur, NULL, format);
					xmlOutputBufferFlush(buf);
					if(buf->conv) {
						*size = static_cast<int>(xmlBufUse(buf->conv));
						*mem = xmlStrndup(xmlBufContent(buf->conv), *size);
					}
					else {
						*size = static_cast<int>(xmlBufUse(buf->buffer));
						*mem = xmlStrndup(xmlBufContent(buf->buffer), *size);
					}
					xmlOutputBufferClose(buf);
				}
			}
		}
	}
}
/**
 * htmlDocDumpMemory:
 * @cur:  the document
 * @mem:  OUT: the memory pointer
 * @size:  OUT: the memory length
 *
 * Dump an HTML document in memory and return the xmlChar * and it's size.
 * It's up to the caller to free the memory.
 */
void htmlDocDumpMemory(xmlDoc * cur, xmlChar** mem, int * size) 
{
	htmlDocDumpMemoryFormat(cur, mem, size, 1);
}
// 
// Dumping HTML tree content to an I/O output buffer
// 
void xmlNsListDumpOutput(xmlOutputBuffer * buf, xmlNs * cur);
/**
 * htmlDtdDumpOutput:
 * @buf:  the HTML buffer output
 * @doc:  the document
 * @encoding:  the encoding string
 *
 * @todo check whether encoding is needed
 *
 * Dump the HTML document DTD, if any.
 */
static void htmlDtdDumpOutput(xmlOutputBuffer * buf, xmlDoc * doc, const char * encoding ATTRIBUTE_UNUSED) 
{
	xmlDtd * cur = doc->intSubset;
	if(!cur) {
		htmlSaveErr(XML_SAVE_NO_DOCTYPE, (xmlNode *)doc, 0);
	}
	else {
		xmlOutputBufferWriteString(buf, "<!DOCTYPE ");
		xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->name));
		if(cur->ExternalID) {
			xmlOutputBufferWriteString(buf, " PUBLIC ");
			xmlBufWriteQuotedString(buf->buffer, cur->ExternalID);
			if(cur->SystemID != NULL) {
				xmlOutputBufferWriteString(buf, " ");
				xmlBufWriteQuotedString(buf->buffer, cur->SystemID);
			}
		}
		else if(cur->SystemID) {
			xmlOutputBufferWriteString(buf, " SYSTEM ");
			xmlBufWriteQuotedString(buf->buffer, cur->SystemID);
		}
		xmlOutputBufferWriteString(buf, ">\n");
	}
}

/**
 * htmlAttrDumpOutput:
 * @buf:  the HTML buffer output
 * @doc:  the document
 * @cur:  the attribute pointer
 * @encoding:  the encoding string
 *
 * Dump an HTML attribute
 */
static void htmlAttrDumpOutput(xmlOutputBuffer * buf, xmlDoc * doc, xmlAttr * cur, const char * encoding ATTRIBUTE_UNUSED) 
{
	xmlChar * value;
	/*
	 * The html output method should not escape a & character
	 * occurring in an attribute value immediately followed by
	 * a { character (see Section B.7.1 of the HTML 4.0 Recommendation).
	 * This is implemented in xmlEncodeEntitiesReentrant
	 */
	if(!cur) {
		return;
	}
	xmlOutputBufferWriteString(buf, " ");
	if(cur->ns && (cur->ns->prefix != NULL)) {
		xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->ns->prefix));
		xmlOutputBufferWriteString(buf, ":");
	}
	xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->name));
	if(cur->children && (!htmlIsBooleanAttr(cur->name))) {
		value = xmlNodeListGetString(doc, cur->children, 0);
		if(value) {
			xmlOutputBufferWriteString(buf, "=");
			if(!cur->ns && cur->parent && !cur->parent->ns && (sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("href")) || sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("action")) ||
				sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("src")) || (sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("name")) && sstreqi_ascii(cur->parent->name, reinterpret_cast<const xmlChar *>("a"))))) {
				xmlChar * tmp = value;
				/* xmlURIEscapeStr() escapes '"' so it can be safely used. */
				xmlBufCCat(buf->buffer, "\"");
				while(IS_BLANK_CH(*tmp)) 
					tmp++;
				/* URI Escape everything, except server side includes. */
				for(;;) {
					xmlChar * escaped;
					xmlChar endChar;
					xmlChar * end = NULL;
					xmlChar * start = (xmlChar *)xmlStrstr(tmp, reinterpret_cast<const xmlChar *>("<!--"));
					if(start) {
						end = (xmlChar *)xmlStrstr(tmp, reinterpret_cast<const xmlChar *>("-->"));
						if(end)
							*start = '\0';
					}
					/* Escape the whole string, or until start (set to '\0'). */
					escaped = xmlURIEscapeStr(tmp, reinterpret_cast<const xmlChar *>("@/:=?;#%&,+"));
					if(escaped) {
						xmlBufCat(buf->buffer, escaped);
						SAlloc::F(escaped);
					}
					else
						xmlBufCat(buf->buffer, tmp);
					if(end == NULL) { /* Everything has been written. */
						break;
					}
					/* Do not escape anything within server side includes. */
					*start = '<'; /* Restore the first character of "<!--". */
					end += 3; /* strlen("-->") */
					endChar = *end;
					*end = '\0';
					xmlBufCat(buf->buffer, start);
					*end = endChar;
					tmp = end;
				}
				xmlBufCCat(buf->buffer, "\"");
			}
			else {
				xmlBufWriteQuotedString(buf->buffer, value);
			}
			SAlloc::F(value);
		}
		else
			xmlOutputBufferWriteString(buf, "=\"\"");
	}
}
/**
 * htmlAttrListDumpOutput:
 * @buf:  the HTML buffer output
 * @doc:  the document
 * @cur:  the first attribute pointer
 * @encoding:  the encoding string
 *
 * Dump a list of HTML attributes
 */
static void htmlAttrListDumpOutput(xmlOutputBuffer * buf, xmlDoc * doc, xmlAttr * cur, const char * encoding) 
{
	for(; cur; cur = cur->next)
		htmlAttrDumpOutput(buf, doc, cur, encoding);
}
/**
 * htmlNodeListDumpOutput:
 * @buf:  the HTML buffer output
 * @doc:  the document
 * @cur:  the first node
 * @encoding:  the encoding string
 * @format:  should formatting spaces been added
 *
 * Dump an HTML node list, recursive behaviour,children are printed too.
 */
static void htmlNodeListDumpOutput(xmlOutputBuffer * buf, xmlDoc * doc, xmlNode * cur, const char * encoding, int format) 
{
	for(; cur; cur = cur->next)
		htmlNodeDumpFormatOutput(buf, doc, cur, encoding, format);
}
/**
 * htmlNodeDumpFormatOutput:
 * @buf:  the HTML buffer output
 * @doc:  the document
 * @cur:  the current node
 * @encoding:  the encoding string
 * @format:  should formatting spaces been added
 *
 * Dump an HTML node, recursive behaviour,children are printed too.
 */
void htmlNodeDumpFormatOutput(xmlOutputBuffer * buf, xmlDoc * doc, xmlNode * cur, const char * encoding, int format) 
{
	xmlInitParser();
	if(cur && buf) {
		//
		// Special cases.
		//
		if(cur->type == XML_DTD_NODE) {
		}
		else if(oneof2(cur->type, XML_HTML_DOCUMENT_NODE, XML_DOCUMENT_NODE))
			htmlDocContentDumpOutput(buf, (xmlDoc *)cur, encoding);
		else if(cur->type == XML_ATTRIBUTE_NODE)
			htmlAttrDumpOutput(buf, doc, (xmlAttr *)cur, encoding);
		else if(cur->type == HTML_TEXT_NODE) {
			if(cur->content) {
				if((cur->name == (const xmlChar *)xmlStringText || cur->name != (const xmlChar *)xmlStringTextNoenc) &&
					(!cur->P_ParentNode || (!sstreqi_ascii(cur->P_ParentNode->name, reinterpret_cast<const xmlChar *>("script")) && !sstreqi_ascii(cur->P_ParentNode->name, reinterpret_cast<const xmlChar *>("style"))))) {
					xmlChar * buffer = xmlEncodeEntitiesReentrant(doc, cur->content);
					if(buffer) {
						xmlOutputBufferWriteString(buf, (const char *)buffer);
						SAlloc::F(buffer);
					}
				}
				else {
					xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->content));
				}
			}
		}
		else if(cur->type == HTML_COMMENT_NODE) {
			if(cur->content) {
				xmlOutputBufferWriteString(buf, "<!--");
				xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->content));
				xmlOutputBufferWriteString(buf, "-->");
			}
		}
		else if(cur->type == HTML_PI_NODE) {
			if(cur->name) {
				xmlOutputBufferWriteString(buf, "<?");
				xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->name));
				if(cur->content) {
					xmlOutputBufferWriteString(buf, " ");
					xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->content));
				}
				xmlOutputBufferWriteString(buf, ">");
			}
		}
		else if(cur->type == HTML_ENTITY_REF_NODE) {
			xmlOutputBufferWriteString(buf, "&");
			xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->name));
			xmlOutputBufferWriteString(buf, ";");
		}
		else if(cur->type == HTML_PRESERVE_NODE) {
			xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->content));
		}
		else {
			const htmlElemDesc * info = cur->ns ? 0 : htmlTagLookup(cur->name); // Get specific HTML info for that node.
			xmlOutputBufferWriteString(buf, "<");
			if(cur->ns && cur->ns->prefix) {
				xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->ns->prefix));
				xmlOutputBufferWriteString(buf, ":");
			}
			xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->name));
			xmlNsListDumpOutput(buf, cur->nsDef);
			htmlAttrListDumpOutput(buf, doc, cur->properties, encoding);
			if(info && info->empty) {
				xmlOutputBufferWriteString(buf, ">");
				if(format && (!info->isinline) && cur->next) {
					if(!oneof2(cur->next->type, HTML_TEXT_NODE, HTML_ENTITY_REF_NODE) && cur->P_ParentNode && cur->P_ParentNode->name && (cur->P_ParentNode->name[0] != 'p')) /* p, pre, param */
						xmlOutputBufferWriteString(buf, "\n");
				}
			}
			else if(((cur->type == XML_ELEMENT_NODE) || !cur->content) && !cur->children) {
				if(info && info->saveEndTag && !sstreq(info->name, "html") && !sstreq(info->name, "body")) {
					xmlOutputBufferWriteString(buf, ">");
				}
				else {
					xmlOutputBufferWriteString(buf, "></");
					if(cur->ns && cur->ns->prefix) {
						xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->ns->prefix));
						xmlOutputBufferWriteString(buf, ":");
					}
					xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->name));
					xmlOutputBufferWriteString(buf, ">");
				}
				if(format && cur->next && info && !info->isinline) {
					if(!oneof2(cur->next->type, HTML_TEXT_NODE, HTML_ENTITY_REF_NODE) && cur->P_ParentNode && cur->P_ParentNode->name && (cur->P_ParentNode->name[0] != 'p')) /* p, pre, param */
						xmlOutputBufferWriteString(buf, "\n");
				}
			}
			else {
				xmlOutputBufferWriteString(buf, ">");
				if((cur->type != XML_ELEMENT_NODE) && cur->content) {
					// Uses the OutputBuffer property to automatically convert invalids to charrefs
					xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->content));
				}
				if(cur->children) {
					if(format && info && !info->isinline && !oneof2(cur->children->type, HTML_TEXT_NODE, HTML_ENTITY_REF_NODE) && 
						(cur->children != cur->last) && cur->name && (cur->name[0] != 'p')) /* p, pre, param */
						xmlOutputBufferWriteString(buf, "\n");
					htmlNodeListDumpOutput(buf, doc, cur->children, encoding, format);
					if(format && info && !info->isinline && !oneof2(cur->last->type, HTML_TEXT_NODE, HTML_ENTITY_REF_NODE) && 
						(cur->children != cur->last) && cur->name && (cur->name[0] != 'p')) /* p, pre, param */
						xmlOutputBufferWriteString(buf, "\n");
				}
				xmlOutputBufferWriteString(buf, "</");
				if(cur->ns && cur->ns->prefix) {
					xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->ns->prefix));
					xmlOutputBufferWriteString(buf, ":");
				}
				xmlOutputBufferWriteString(buf, reinterpret_cast<const char *>(cur->name));
				xmlOutputBufferWriteString(buf, ">");
				if(format && info && !info->isinline && cur->next) {
					if(!oneof2(cur->next->type, HTML_TEXT_NODE, HTML_ENTITY_REF_NODE) && cur->P_ParentNode && cur->P_ParentNode->name && (cur->P_ParentNode->name[0] != 'p')) /* p, pre, param */
						xmlOutputBufferWriteString(buf, "\n");
				}
			}
		}
	}
}

/**
 * htmlNodeDumpOutput:
 * @buf:  the HTML buffer output
 * @doc:  the document
 * @cur:  the current node
 * @encoding:  the encoding string
 *
 * Dump an HTML node, recursive behaviour,children are printed too,
 * and formatting returns/spaces are added.
 */
void htmlNodeDumpOutput(xmlOutputBuffer * buf, xmlDoc * doc, xmlNode * cur, const char * encoding) 
{
	htmlNodeDumpFormatOutput(buf, doc, cur, encoding, 1);
}
/**
 * htmlDocContentDumpFormatOutput:
 * @buf:  the HTML buffer output
 * @cur:  the document
 * @encoding:  the encoding string
 * @format:  should formatting spaces been added
 *
 * Dump an HTML document.
 */
void htmlDocContentDumpFormatOutput(xmlOutputBuffer * buf, xmlDoc * cur, const char * encoding, int format) 
{
	xmlInitParser();
	if(buf && cur) {
		// 
		// force to output the stuff as HTML, especially for entities
		// 
		int type = cur->type;
		cur->type = XML_HTML_DOCUMENT_NODE;
		if(cur->intSubset)
			htmlDtdDumpOutput(buf, cur, 0);
		if(cur->children)
			htmlNodeListDumpOutput(buf, cur, cur->children, encoding, format);
		xmlOutputBufferWriteString(buf, "\n");
		cur->type = (xmlElementType)type;
	}
}
/**
 * htmlDocContentDumpOutput:
 * @buf:  the HTML buffer output
 * @cur:  the document
 * @encoding:  the encoding string
 *
 * Dump an HTML document. Formating return/spaces are added.
 */
void htmlDocContentDumpOutput(xmlOutputBuffer * buf, xmlDoc * cur, const char * encoding) 
{
	htmlDocContentDumpFormatOutput(buf, cur, encoding, 1);
}
//
// Saving functions front-ends
//
/**
 * htmlDocDump:
 * @f:  the FILE*
 * @cur:  the document
 *
 * Dump an HTML document to an open FILE.
 *
 * returns: the number of byte written or -1 in case of failure.
 */
int htmlDocDump(FILE * f, xmlDoc * cur) 
{
	xmlOutputBuffer * buf;
	xmlCharEncodingHandler * handler = NULL;
	int ret = -1;
	xmlInitParser();
	if(cur && f) {
		const char * encoding = (const char *)htmlGetMetaEncoding(cur);
		if(encoding) {
			xmlCharEncoding enc = xmlParseCharEncoding(encoding);
			if(enc != cur->charset) {
				if(cur->charset != XML_CHAR_ENCODING_UTF8) {
					return -1; // Not supported yet
				}
				handler = xmlFindCharEncodingHandler(encoding);
				if(handler == NULL)
					htmlSaveErr(XML_SAVE_UNKNOWN_ENCODING, NULL, encoding);
			}
			else {
				handler = xmlFindCharEncodingHandler(encoding);
			}
		}
		/*
		* Fallback to HTML or ASCII when the encoding is unspecified
		*/
		SETIFZ(handler, xmlFindCharEncodingHandler("HTML"));
		SETIFZ(handler, xmlFindCharEncodingHandler("ascii"));
		buf = xmlOutputBufferCreateFile(f, handler);
		if(!buf) 
			return -1;
		htmlDocContentDumpOutput(buf, cur, 0);
		ret = xmlOutputBufferClose(buf);
	}
	return ret;
}

/**
 * htmlSaveFile:
 * @filename:  the filename (or URL)
 * @cur:  the document
 *
 * Dump an HTML document to a file. If @filename is "-" the stdout file is
 * used.
 * returns: the number of byte written or -1 in case of failure.
 */
int htmlSaveFile(const char * filename, xmlDoc * cur) 
{
	xmlOutputBuffer * buf;
	xmlCharEncodingHandler * handler = NULL;
	const char * encoding;
	if(!cur || (filename == NULL))
		return -1;
	xmlInitParser();
	encoding = (const char *)htmlGetMetaEncoding(cur);
	if(encoding) {
		xmlCharEncoding enc = xmlParseCharEncoding(encoding);
		if(enc != cur->charset) {
			if(cur->charset != XML_CHAR_ENCODING_UTF8) {
				// Not supported yet
				return -1;
			}
			handler = xmlFindCharEncodingHandler(encoding);
			if(!handler)
				htmlSaveErr(XML_SAVE_UNKNOWN_ENCODING, NULL, encoding);
		}
	}
	/*
	 * Fallback to HTML or ASCII when the encoding is unspecified
	 */
	SETIFZ(handler, xmlFindCharEncodingHandler("HTML"));
	SETIFZ(handler, xmlFindCharEncodingHandler("ascii"));
	/*
	 * save the content to a temp buffer.
	 */
	buf = xmlOutputBufferCreateFilename(filename, handler, cur->compression);
	if(!buf) 
		return 0;
	htmlDocContentDumpOutput(buf, cur, 0);
	return xmlOutputBufferClose(buf);
}
/**
 * htmlSaveFileFormat:
 * @filename:  the filename
 * @cur:  the document
 * @format:  should formatting spaces been added
 * @encoding: the document encoding
 *
 * Dump an HTML document to a file using a given encoding.
 *
 * returns: the number of byte written or -1 in case of failure.
 */
int htmlSaveFileFormat(const char * filename, xmlDoc * cur, const char * encoding, int format) 
{
	xmlOutputBuffer * buf;
	xmlCharEncodingHandler * handler = NULL;
	if(!cur || !filename)
		return -1;
	xmlInitParser();
	if(encoding) {
		xmlCharEncoding enc = xmlParseCharEncoding(encoding);
		if(enc != cur->charset) {
			if(cur->charset != XML_CHAR_ENCODING_UTF8) {
				return -1; // Not supported yet
			}
			handler = xmlFindCharEncodingHandler(encoding);
			if(handler == NULL)
				htmlSaveErr(XML_SAVE_UNKNOWN_ENCODING, NULL, encoding);
		}
		htmlSetMetaEncoding(cur, (const xmlChar *)encoding);
	}
	else {
		htmlSetMetaEncoding(cur, (const xmlChar *)"UTF-8");
	}
	/*
	 * Fallback to HTML or ASCII when the encoding is unspecified
	 */
	SETIFZ(handler, xmlFindCharEncodingHandler("HTML"));
	SETIFZ(handler, xmlFindCharEncodingHandler("ascii"));
	/*
	 * save the content to a temp buffer.
	 */
	buf = xmlOutputBufferCreateFilename(filename, handler, 0);
	if(!buf) 
		return 0;
	htmlDocContentDumpFormatOutput(buf, cur, encoding, format);
	return xmlOutputBufferClose(buf);
}
/**
 * htmlSaveFileEnc:
 * @filename:  the filename
 * @cur:  the document
 * @encoding: the document encoding
 *
 * Dump an HTML document to a file using a given encoding
 * and formatting returns/spaces are added.
 *
 * returns: the number of byte written or -1 in case of failure.
 */
int htmlSaveFileEnc(const char * filename, xmlDoc * cur, const char * encoding) 
{
	return (htmlSaveFileFormat(filename, cur, encoding, 1));
}

#endif /* LIBXML_OUTPUT_ENABLED */

#define bottom_HTMLtree
#endif /* LIBXML_HTML_ENABLED */
