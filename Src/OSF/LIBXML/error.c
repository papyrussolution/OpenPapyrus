/*
 * error.c: module displaying/handling XML parser errors
 *
 * See Copyright for the status of this software.
 *
 * Daniel Veillard <daniel@veillard.com>
 */

#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop

void XMLCDECL xmlGenericErrorDefaultFunc(void * ctx ATTRIBUTE_UNUSED, const char * msg, ...);

#define XML_GET_VAR_STR(msg, str) {				\
		int    prev_size = -1;			      \
		int    chars;					      \
		va_list ap;						  \
		str = (char*)SAlloc::M(150); \
		if(str) {					   \
			int size = 150;						    \
			while(size < 64000) {					   \
				va_start(ap, msg);					\
				chars = vsnprintf(str, size, msg, ap);			\
				va_end(ap);						\
				if((chars > -1) && (chars < size)) {		       \
					if(prev_size == chars)			   \
						break;						\
					else					    \
						prev_size = chars;				\
				}							\
				size += ((chars > -1) ? (chars + 1) : 100); \
				char * larger = (char*)SAlloc::R(str, size); \
				if(!larger) \
					break;	\
				str = larger; \
			}} \
}

/************************************************************************
*									*
*			Handling of out of context errors		*
*									*
************************************************************************/

/**
 * xmlGenericErrorDefaultFunc:
 * @ctx:  an error context
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 *
 * Default handler for out of context error messages.
 */
void XMLCDECL xmlGenericErrorDefaultFunc(void * ctx ATTRIBUTE_UNUSED, const char * msg, ...)
{
	va_list args;
	SETIFZ(xmlGenericErrorContext, (void*)stderr);
	va_start(args, msg);
	vfprintf((FILE*)xmlGenericErrorContext, msg, args);
	va_end(args);
}
/**
 * initGenericErrorDefaultFunc:
 * @handler:  the handler
 *
 * Set or reset (if NULL) the default handler for generic errors
 * to the builtin error function.
 */
void initGenericErrorDefaultFunc(xmlGenericErrorFunc * handler)
{
	xmlGenericError = handler ? *handler : xmlGenericErrorDefaultFunc;
}
/**
 * xmlSetGenericErrorFunc:
 * @ctx:  the new error handling context
 * @handler:  the new handler function
 *
 * Function to reset the handler and the error context for out of
 * context error messages.
 * This simply means that @handler will be called for subsequent
 * error messages while not parsing nor validating. And @ctx will
 * be passed as first argument to @handler
 * One can simply force messages to be emitted to another FILE * than
 * stderr by setting @ctx to this file handle and @handler to NULL.
 * For multi-threaded applications, this must be set separately for each thread.
 */
void xmlSetGenericErrorFunc(void * ctx, xmlGenericErrorFunc handler)
{
	xmlGenericErrorContext = ctx;
	xmlGenericError = NZOR(handler, xmlGenericErrorDefaultFunc);
}
/**
 * xmlSetStructuredErrorFunc:
 * @ctx:  the new error handling context
 * @handler:  the new handler function
 *
 * Function to reset the handler and the error context for out of
 * context structured error messages.
 * This simply means that @handler will be called for subsequent
 * error messages while not parsing nor validating. And @ctx will
 * be passed as first argument to @handler
 * For multi-threaded applications, this must be set separately for each thread.
 */
void xmlSetStructuredErrorFunc(void * ctx, xmlStructuredErrorFunc handler)
{
	xmlStructuredErrorContext = ctx;
	xmlStructuredError = handler;
}

/************************************************************************
*									*
*			Handling of parsing errors			*
*									*
************************************************************************/
/**
 * xmlParserPrintFileInfo:
 * @input:  an xmlParserInputPtr input
 *
 * Displays the associated file and line informations for the current input
 */
void xmlParserPrintFileInfo(xmlParserInput * input)
{
	if(input) {
		if(input->filename)
			xmlGenericError(0, "%s:%d: ", input->filename, input->line);
		else
			xmlGenericError(0, "Entity: line %d: ", input->line);
	}
}
/**
 * xmlParserPrintFileContext:
 * @input:  an xmlParserInputPtr input
 *
 * Displays current context within the input content for error tracking
 */
static void xmlParserPrintFileContextInternal(xmlParserInputPtr input, xmlGenericErrorFunc channel, void * data)
{
	uint n, col;    /* GCC warns if signed, because compared with sizeof() */
	xmlChar content[81]; /* space for 80 chars + line terminator */
	xmlChar * ctnt;
	if(input) {
		const xmlChar * cur = input->cur;
		const xmlChar * base = input->base;
		/* skip backwards over any end-of-lines */
		while((cur > base) && ((*(cur) == '\n') || (*(cur) == '\r'))) {
			cur--;
		}
		n = 0;
		/* search backwards for beginning-of-line (to max buff size) */
		while((n++ < (sizeof(content)-1)) && (cur > base) && (*(cur) != '\n') && (*(cur) != '\r'))
			cur--;
		if((*(cur) == '\n') || (*(cur) == '\r')) 
			cur++;
		/* calculate the error position in terms of the current position */
		col = input->cur - cur;
		/* search forward for end-of-line (to max buff size) */
		n = 0;
		ctnt = content;
		/* copy selected text to our buffer */
		while((*cur != 0) && (*(cur) != '\n') && (*(cur) != '\r') && (n < sizeof(content)-1)) {
			*ctnt++ = *cur++;
			n++;
		}
		*ctnt = 0;
		/* print out the selected text */
		channel(data, "%s\n", content);
		/* create blank line with problem pointer */
		n = 0;
		ctnt = content;
		/* (leave buffer space for pointer + line terminator) */
		while((n<col) && (n++ < sizeof(content)-2) && (*ctnt != 0)) {
			if(*(ctnt) != '\t')
				*(ctnt) = ' ';
			ctnt++;
		}
		*ctnt++ = '^';
		*ctnt = 0;
		channel(data, "%s\n", content);
	}
}
/**
 * @input:  an xmlParserInputPtr input
 *
 * Displays current context within the input content for error tracking
 */
void xmlParserPrintFileContext(xmlParserInput * input)
{
	xmlParserPrintFileContextInternal(input, xmlGenericError, xmlGenericErrorContext);
}
/**
 * xmlReportError:
 * @err: the error
 * @ctx: the parser context or NULL
 * @str: the formatted error message
 *
 * Report an erro with its context, replace the 4 old error/warning
 * routines.
 */
static void xmlReportError(xmlErrorPtr err, xmlParserCtxtPtr ctxt, const char * str, xmlGenericErrorFunc channel, void * data)
{
	char * file = NULL;
	int line = 0;
	int code = -1;
	int domain;
	SString temp_buf;
	const xmlChar * name = NULL;
	xmlNode * P_Node;
	xmlErrorLevel level;
	xmlParserInputPtr input = NULL;
	xmlParserInputPtr cur = NULL;
	if(err) {
		if(channel == NULL) {
			channel = xmlGenericError;
			data = xmlGenericErrorContext;
		}
		file = err->file;
		line = err->line;
		code = err->code;
		domain = err->domain;
		level = err->level;
		P_Node = (xmlNode *)err->P_Node;
		if(code == XML_ERR_OK)
			return;
		if(P_Node && P_Node->type == XML_ELEMENT_NODE)
			name = P_Node->name;
		/*
		 * Maintain the compatibility with the legacy error handling
		 */
		if(ctxt) {
			input = ctxt->input;
			if(input && !input->filename && (ctxt->inputNr > 1)) {
				cur = input;
				input = ctxt->inputTab[ctxt->inputNr - 2];
			}
			if(input != NULL) {
				if(input->filename)
					channel(data, "%s:%d: ", input->filename, input->line);
				else if((line != 0) && (domain == XML_FROM_PARSER))
					channel(data, "Entity: line %d: ", input->line);
			}
		}
		else {
			if(file)
				channel(data, "%s:%d: ", file, line);
			else if(line && oneof6(domain, XML_FROM_PARSER, XML_FROM_SCHEMASV, XML_FROM_SCHEMASP, XML_FROM_DTD, XML_FROM_RELAXNGP, XML_FROM_RELAXNGV))
				channel(data, "Entity: line %d: ", line);
		}
		if(name) {
			channel(data, "element %s: ", name);
		}
		{
			const char * p_domain_text = 0;
			switch(domain) {
				case XML_FROM_PARSER: p_domain_text = "parser"; break;
				case XML_FROM_NAMESPACE: p_domain_text = "namespace"; break;
				case XML_FROM_DTD:
				case XML_FROM_VALID: p_domain_text = "validity"; break;
				case XML_FROM_HTML: p_domain_text = "HTML parser"; break;
				case XML_FROM_MEMORY: p_domain_text = "memory"; break;
				case XML_FROM_OUTPUT: p_domain_text = "output"; break;
				case XML_FROM_IO: p_domain_text = "I/O"; break;
				case XML_FROM_XINCLUDE: p_domain_text = "XInclude"; break;
				case XML_FROM_XPATH: p_domain_text = "XPath"; break;
				case XML_FROM_XPOINTER: p_domain_text = "parser"; break;
				case XML_FROM_REGEXP: p_domain_text = "regexp"; break;
				case XML_FROM_MODULE: p_domain_text = "module"; break;
				case XML_FROM_SCHEMASV: p_domain_text = "Schemas validity"; break;
				case XML_FROM_SCHEMASP: p_domain_text = "Schemas parser"; break;
				case XML_FROM_RELAXNGP: p_domain_text = "Relax-NG parser"; break;
				case XML_FROM_RELAXNGV: p_domain_text = "Relax-NG validity"; break;
				case XML_FROM_CATALOG: p_domain_text = "Catalog"; break;
				case XML_FROM_C14N: p_domain_text = "C14N"; break;
				case XML_FROM_XSLT: p_domain_text = "XSLT"; break;
				case XML_FROM_I18N: p_domain_text = "encoding"; break;
				case XML_FROM_SCHEMATRONV: p_domain_text = "schematron"; break;
				case XML_FROM_BUFFER: p_domain_text = "internal buffer"; break;
				case XML_FROM_URI: p_domain_text = "URI"; break;
				default: break;
			}
			if(p_domain_text) {
				(temp_buf = p_domain_text).Space();
				channel(data, temp_buf.cptr());
			}
		}
		switch(level) {
			case XML_ERR_NONE: channel(data, ": "); break;
			case XML_ERR_WARNING: channel(data, "warning : "); break;
			case XML_ERR_ERROR: channel(data, "error : "); break;
			case XML_ERR_FATAL: channel(data, "error : "); break;
		}
		if(str) {
			int len = sstrlen(str);
			if((len > 0) && (str[len-1] != '\n'))
				channel(data, "%s\n", str);
			else
				channel(data, "%s", str);
		}
		else {
			channel(data, "%s\n", "out of memory error");
		}
		if(ctxt) {
			xmlParserPrintFileContextInternal(input, channel, data);
			if(cur) {
				if(cur->filename)
					channel(data, "%s:%d: \n", cur->filename, cur->line);
				else if(line && (domain == XML_FROM_PARSER))
					channel(data, "Entity: line %d: \n", cur->line);
				xmlParserPrintFileContextInternal(cur, channel, data);
			}
		}
		if((domain == XML_FROM_XPATH) && err->str1 && (err->int1 < 100) && (err->int1 < (int)sstrlen(err->str1))) {
			xmlChar buf[150];
			int i;
			channel(data, "%s\n", err->str1);
			for(i = 0; i < err->int1; i++)
				buf[i] = ' ';
			buf[i++] = '^';
			buf[i] = 0;
			channel(data, "%s\n", buf);
		}
	}
}

/**
 * __xmlRaiseError:
 * @schannel: the structured callback channel
 * @channel: the old callback channel
 * @data: the callback data
 * @ctx: the parser context or NULL
 * @ctx: the parser context or NULL
 * @domain: the domain for the error
 * @code: the code for the error
 * @level: the xmlErrorLevel for the error
 * @file: the file source of the error (or NULL)
 * @line: the line of the error or 0 if N/A
 * @str1: extra string info
 * @str2: extra string info
 * @str3: extra string info
 * @int1: extra int info
 * @col: column number of the error or 0 if N/A
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 *
 * Update the appropriate global or contextual error structure,
 * then forward the error message down the parser or generic
 * error callback handler
 */
void XMLCDECL __xmlRaiseError(xmlStructuredErrorFunc schannel, xmlGenericErrorFunc channel, void * data, void * ctx,
    void * nod, int domain, int code, xmlErrorLevel level, const char * file, int line, const char * str1,
    const char * str2, const char * str3, int int1, int col, const char * msg, ...)
{
	xmlParserCtxtPtr ctxt = NULL;
	xmlNode * P_Node = (xmlNode *)nod;
	char * str = NULL;
	xmlParserInputPtr input = NULL;
	xmlErrorPtr to = &xmlLastError;
	xmlNode * baseptr = NULL;
	if(code == XML_ERR_OK)
		return;
	if((xmlGetWarningsDefaultValue == 0) && (level == XML_ERR_WARNING))
		return;
	if(oneof6(domain, XML_FROM_PARSER, XML_FROM_HTML, XML_FROM_DTD, XML_FROM_NAMESPACE, XML_FROM_IO, XML_FROM_VALID)) {
		ctxt = (xmlParserCtxtPtr)ctx;
		if(!schannel && ctxt && ctxt->sax && (ctxt->sax->initialized == XML_SAX2_MAGIC) && ctxt->sax->serror) {
			schannel = ctxt->sax->serror;
			data = ctxt->userData;
		}
	}
	/*
	 * Check if structured error handler set
	 */
	if(schannel == NULL) {
		schannel = xmlStructuredError;
		/*
		 * if user has defined handler, change data ptr to user's choice
		 */
		if(schannel)
			data = xmlStructuredErrorContext;
	}
	/*
	 * Formatting the message
	 */
	if(msg == NULL) {
		str = sstrdup("No error message provided");
	}
	else {
		XML_GET_VAR_STR(msg, str);
	}
	/*
	 * specific processing if a parser context is provided
	 */
	if(ctxt) {
		if(file == NULL) {
			input = ctxt->input;
			if(input && !input->filename && (ctxt->inputNr > 1)) {
				input = ctxt->inputTab[ctxt->inputNr - 2];
			}
			if(input) {
				file = input->filename;
				line = input->line;
				col = input->col;
			}
		}
		to = &ctxt->lastError;
	}
	else if(P_Node && !file) {
		int i;
		if(P_Node->doc && P_Node->doc->URL) {
			baseptr = P_Node;
/*	    file = (const char *) node->doc->URL; */
		}
		for(i = 0; ((i < 10) && P_Node && (P_Node->type != XML_ELEMENT_NODE)); i++)
			P_Node = P_Node->parent;
		if(!baseptr && P_Node && P_Node->doc && P_Node->doc->URL)
			baseptr = P_Node;
		if(P_Node && P_Node->type == XML_ELEMENT_NODE)
			line = P_Node->line;
		if(!line || line == 65535)
			line = xmlGetLineNo(P_Node);
	}
	/*
	 * Save the information about the error
	 */
	xmlResetError(to);
	to->domain = domain;
	to->code = code;
	to->message = str;
	to->level = level;
	if(file)
		to->file = sstrdup(file);
	else if(baseptr) {
#ifdef LIBXML_XINCLUDE_ENABLED
		// 
		// We check if the error is within an XInclude section and,
		// if so, attempt to print out the href of the XInclude instead
		// of the usual "base" (doc->URL) for the node (bug 152623).
		// 
		xmlNode * prev = baseptr;
		int inclcount = 0;
		while(prev) {
			if(prev->prev == NULL)
				prev = prev->parent;
			else {
				prev = prev->prev;
				if(prev->type == XML_XINCLUDE_START) {
					if(--inclcount < 0)
						break;
				}
				else if(prev->type == XML_XINCLUDE_END)
					inclcount++;
			}
		}
		if(prev) {
			if(prev->type == XML_XINCLUDE_START) {
				prev->type = XML_ELEMENT_NODE;
				to->file = (char*)xmlGetProp(prev, BAD_CAST "href");
				prev->type = XML_XINCLUDE_START;
			}
			else {
				to->file = (char*)xmlGetProp(prev, BAD_CAST "href");
			}
		}
		else
#endif
		to->file = (char *)sstrdup(baseptr->doc->URL);
		if(!to->file && P_Node && P_Node->doc)
			to->file = (char*)sstrdup(P_Node->doc->URL);
	}
	to->line = line;
	to->str1 = sstrdup(str1);
	to->str2 = sstrdup(str2);
	to->str3 = sstrdup(str3);
	to->int1 = int1;
	to->int2 = col;
	to->P_Node = P_Node;
	to->ctxt = ctx;
	if(to != &xmlLastError)
		xmlCopyError(to, &xmlLastError);
	if(schannel) {
		schannel(data, to);
	}
	else {
		/*
		* Find the callback channel if channel param is NULL
		*/
		if(ctxt && !channel && !xmlStructuredError && ctxt->sax) {
			channel = (level == XML_ERR_WARNING) ? ctxt->sax->warning : ctxt->sax->error;
			data = ctxt->userData;
		}
		else if(!channel) {
			channel = xmlGenericError;
			data = NZOR(ctxt, xmlGenericErrorContext);
		}
		if(channel) {
			if(oneof4(channel, xmlParserError, xmlParserWarning, xmlParserValidityError, xmlParserValidityWarning))
				xmlReportError(to, ctxt, str, 0, 0);
			else if((channel == (xmlGenericErrorFunc)fprintf) || (channel == xmlGenericErrorDefaultFunc))
				xmlReportError(to, ctxt, str, channel, data);
			else
				channel(data, "%s", str);
		}
	}
}
/**
 * __xmlSimpleError:
 * @domain: where the error comes from
 * @code: the error code
 * @node: the context node
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
void FASTCALL __xmlSimpleError(int domain, int code, xmlNode * P_Node, const char * msg, const char * extra)
{
	if(code == XML_ERR_NO_MEMORY) {
		if(extra)
			__xmlRaiseError(0, 0, 0, 0, P_Node, domain, XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0, extra, 0, 0, 0, 0, "Memory allocation failed : %s\n", extra);
		else
			__xmlRaiseError(0, 0, 0, 0, P_Node, domain, XML_ERR_NO_MEMORY, XML_ERR_FATAL, 0, 0, 0, 0, 0, 0, 0, "Memory allocation failed\n");
	}
	else {
		__xmlRaiseError(0, 0, 0, 0, P_Node, domain, code, XML_ERR_ERROR, NULL, 0, extra, 0, 0, 0, 0, msg, extra);
	}
}
/**
 * xmlParserError:
 * @ctx:  an XML parser context
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 *
 * Display and format an error messages, gives file, line, position and
 * extra parameters.
 */
void XMLCDECL xmlParserError(void * ctx, const char * msg, ...)
{
	xmlParserCtxt * ctxt = (xmlParserCtxt *)ctx;
	xmlParserInputPtr input = NULL;
	xmlParserInputPtr cur = NULL;
	char * str;
	if(ctxt) {
		input = ctxt->input;
		if(input && (input->filename == NULL) && (ctxt->inputNr > 1)) {
			cur = input;
			input = ctxt->inputTab[ctxt->inputNr - 2];
		}
		xmlParserPrintFileInfo(input);
	}
	xmlGenericError(0, "error: ");
	XML_GET_VAR_STR(msg, str);
	xmlGenericError(0, "%s", str);
	SAlloc::F(str);
	if(ctxt) {
		xmlParserPrintFileContext(input);
		if(cur) {
			xmlParserPrintFileInfo(cur);
			xmlGenericError(0, "\n");
			xmlParserPrintFileContext(cur);
		}
	}
}

/**
 * xmlParserWarning:
 * @ctx:  an XML parser context
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 *
 * Display and format a warning messages, gives file, line, position and
 * extra parameters.
 */
void XMLCDECL xmlParserWarning(void * ctx, const char * msg, ...)
{
	xmlParserCtxt * ctxt = (xmlParserCtxt *)ctx;
	xmlParserInputPtr input = NULL;
	xmlParserInputPtr cur = NULL;
	char * str;
	if(ctxt) {
		input = ctxt->input;
		if(input && (input->filename == NULL) && (ctxt->inputNr > 1)) {
			cur = input;
			input = ctxt->inputTab[ctxt->inputNr - 2];
		}
		xmlParserPrintFileInfo(input);
	}
	xmlGenericError(0, "warning: ");
	XML_GET_VAR_STR(msg, str);
	xmlGenericError(0, "%s", str);
	SAlloc::F(str);
	if(ctxt) {
		xmlParserPrintFileContext(input);
		if(cur) {
			xmlParserPrintFileInfo(cur);
			xmlGenericError(0, "\n");
			xmlParserPrintFileContext(cur);
		}
	}
}

/************************************************************************
*									*
*			Handling of validation errors			*
*									*
************************************************************************/

/**
 * xmlParserValidityError:
 * @ctx:  an XML parser context
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 *
 * Display and format an validity error messages, gives file,
 * line, position and extra parameters.
 */
void XMLCDECL xmlParserValidityError(void * ctx, const char * msg, ...)
{
	xmlParserCtxt * ctxt = (xmlParserCtxt *)ctx;
	xmlParserInputPtr input = NULL;
	char * str;
	int len = sstrlen((const xmlChar*)msg);
	static int had_info = 0;

	if((len > 1) && (msg[len - 2] != ':')) {
		if(ctxt) {
			input = ctxt->input;
			if((input->filename == NULL) && (ctxt->inputNr > 1))
				input = ctxt->inputTab[ctxt->inputNr - 2];

			if(had_info == 0) {
				xmlParserPrintFileInfo(input);
			}
		}
		xmlGenericError(0, "validity error: ");
		had_info = 0;
	}
	else {
		had_info = 1;
	}
	XML_GET_VAR_STR(msg, str);
	xmlGenericError(0, "%s", str);
	SAlloc::F(str);
	if(ctxt && input) {
		xmlParserPrintFileContext(input);
	}
}

/**
 * xmlParserValidityWarning:
 * @ctx:  an XML parser context
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 *
 * Display and format a validity warning messages, gives file, line,
 * position and extra parameters.
 */
void XMLCDECL xmlParserValidityWarning(void * ctx, const char * msg, ...)
{
	xmlParserCtxt * ctxt = (xmlParserCtxt *)ctx;
	xmlParserInputPtr input = NULL;
	char * str;
	int len = sstrlen((const xmlChar*)msg);
	if(ctxt && (len != 0) && (msg[len - 1] != ':')) {
		input = ctxt->input;
		if((input->filename == NULL) && (ctxt->inputNr > 1))
			input = ctxt->inputTab[ctxt->inputNr - 2];
		xmlParserPrintFileInfo(input);
	}
	xmlGenericError(0, "validity warning: ");
	XML_GET_VAR_STR(msg, str);
	xmlGenericError(0, "%s", str);
	SAlloc::F(str);
	if(ctxt) {
		xmlParserPrintFileContext(input);
	}
}

/************************************************************************
*									*
*			Extended Error Handling				*
*									*
************************************************************************/

/**
 * xmlGetLastError:
 *
 * Get the last global error registered. This is per thread if compiled
 * with thread support.
 *
 * Returns NULL if no error occured or a pointer to the error
 */
xmlErrorPtr xmlGetLastError()
{
	if(xmlLastError.code == XML_ERR_OK)
		return 0;
	return (&xmlLastError);
}

/**
 * xmlResetError:
 * @err: pointer to the error.
 *
 * Cleanup the error.
 */
void xmlResetError(xmlErrorPtr err)
{
	if(err == NULL)
		return;
	if(err->code == XML_ERR_OK)
		return;
	SAlloc::F(err->message);
	SAlloc::F(err->file);
	SAlloc::F(err->str1);
	SAlloc::F(err->str2);
	SAlloc::F(err->str3);
	memzero(err, sizeof(xmlError));
	err->code = XML_ERR_OK;
}

/**
 * xmlResetLastError:
 *
 * Cleanup the last global error registered. For parsing error
 * this does not change the well-formedness result.
 */
void xmlResetLastError()
{
	if(xmlLastError.code == XML_ERR_OK)
		return;
	xmlResetError(&xmlLastError);
}

/**
 * xmlCtxtGetLastError:
 * @ctx:  an XML parser context
 *
 * Get the last parsing error registered.
 *
 * Returns NULL if no error occured or a pointer to the error
 */
xmlErrorPtr xmlCtxtGetLastError(void * ctx)
{
	xmlParserCtxt * ctxt = (xmlParserCtxt *)ctx;

	if(!ctxt)
		return 0;
	if(ctxt->lastError.code == XML_ERR_OK)
		return 0;
	return (&ctxt->lastError);
}

/**
 * xmlCtxtResetLastError:
 * @ctx:  an XML parser context
 *
 * Cleanup the last global error registered. For parsing error
 * this does not change the well-formedness result.
 */
void xmlCtxtResetLastError(void * ctx)
{
	xmlParserCtxt * ctxt = (xmlParserCtxt *)ctx;

	if(!ctxt)
		return;
	ctxt->errNo = XML_ERR_OK;
	if(ctxt->lastError.code == XML_ERR_OK)
		return;
	xmlResetError(&ctxt->lastError);
}

/**
 * xmlCopyError:
 * @from:  a source error
 * @to:  a target error
 *
 * Save the original error to the new place.
 *
 * Returns 0 in case of success and -1 in case of error.
 */
int xmlCopyError(xmlErrorPtr from, xmlErrorPtr to) 
{
	if(!from || !to)
		return -1;
	else {
		char * message = sstrdup(from->message);
		char * file = sstrdup(from->file);
		char * str1 = sstrdup(from->str1);
		char * str2 = sstrdup(from->str2);
		char * str3 = sstrdup(from->str3);
		SAlloc::F(to->message);
		SAlloc::F(to->file);
		SAlloc::F(to->str1);
		SAlloc::F(to->str2);
		SAlloc::F(to->str3);
		to->domain = from->domain;
		to->code = from->code;
		to->level = from->level;
		to->line = from->line;
		to->P_Node = from->P_Node;
		to->int1 = from->int1;
		to->int2 = from->int2;
		to->P_Node = from->P_Node;
		to->ctxt = from->ctxt;
		to->message = message;
		to->file = file;
		to->str1 = str1;
		to->str2 = str2;
		to->str3 = str3;
		return 0;
	}
}

#define bottom_error
#include "elfgcchack.h"
