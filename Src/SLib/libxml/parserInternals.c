// PARSERINTERNALS.C
// Internal routines (and obsolete ones) needed for the XML and HTML parsers.
// See Copyright for the status of this software.
// daniel@veillard.com
// 
#include <slib-internal.h>
#pragma hdrstop
#if defined(WIN32) && !defined (__CYGWIN__)
	#define XML_DIR_SEP '\\'
#else
	#define XML_DIR_SEP '/'
#endif
/*
 * Various global defaults for parsing
 */
/**
 * xmlCheckVersion:
 * @version: the include version number
 *
 * check the compiled lib version against the include one.
 * This can warn or immediately kill the application
 */
void xmlCheckVersion(int version) 
{
	const int myversion = (int)LIBXML_VERSION;
	xmlInitParser();
	if((myversion / 10000) != (version / 10000)) {
		xmlGenericError(0, "Fatal: program compiled against libxml %d using libxml %d\n", (version / 10000), (myversion / 10000));
		slfprintf_stderr("Fatal: program compiled against libxml %d using libxml %d\n", (version / 10000), (myversion / 10000));
	}
	if((myversion / 100) < (version / 100)) {
		xmlGenericError(0, "Warning: program compiled against libxml %d using older %d\n", (version / 100), (myversion / 100));
	}
}
// 
// Some factorized error routines
// 
/**
 * xmlErrMemory:
 * @ctxt:  an XML parser context
 * @extra:  extra informations
 *
 * Handle a redefinition of attribute error
 */
void FASTCALL xmlErrMemory(xmlParserCtxt * ctxt, const char * extra)
{
	if(!xmlParserCtxt::IsEofInNonSaxMode(ctxt)) {
		if(ctxt) {
			ctxt->errNo = XML_ERR_NO_MEMORY;
			ctxt->instate = XML_PARSER_EOF;
			ctxt->disableSAX = 1;
		}
		if(extra)
			__xmlRaiseError(0, 0, 0, ctxt, 0, XML_FROM_PARSER, XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0, extra, NULL, NULL, 0, 0, "Memory allocation failed : %s\n", extra);
		else
			__xmlRaiseError(0, 0, 0, ctxt, 0, XML_FROM_PARSER, XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0, NULL, NULL, NULL, 0, 0, "Memory allocation failed\n");
	}
}
/**
 * __xmlErrEncoding:
 * @ctxt:  an XML parser context
 * @xmlerr:  the error number
 * @msg:  the error message
 * @str1:  an string info
 * @str2:  an string info
 *
 * Handle an encoding error
 */
void FASTCALL __xmlErrEncoding(xmlParserCtxt * ctxt, xmlParserErrors xmlerr, const char * msg, const xmlChar * str1, const xmlChar * str2)
{
	if(!xmlParserCtxt::IsEofInNonSaxMode(ctxt)) {
		if(ctxt)
			ctxt->errNo = xmlerr;
		__xmlRaiseError(0, 0, 0, ctxt, 0, XML_FROM_PARSER, xmlerr, XML_ERR_FATAL, NULL, 0, PTRCHRC_(str1), PTRCHRC_(str2), NULL, 0, 0, msg, str1, str2);
		if(ctxt) {
			ctxt->wellFormed = 0;
			if(ctxt->recovery == 0)
				ctxt->disableSAX = 1;
		}
	}
}

static void FASTCALL __xmlErrEncodingNotSupported(xmlParserCtxt * ctxt, const xmlChar * pEnc)
{
	__xmlErrEncoding(ctxt, XML_ERR_UNSUPPORTED_ENCODING, "encoding not supported %s\n", pEnc, 0);
}
/**
 * xmlErrInternal:
 * @ctxt:  an XML parser context
 * @msg:  the error message
 * @str:  error informations
 *
 * Handle an internal error
 */
static void FASTCALL xmlErrInternal(xmlParserCtxt * ctxt, const char * msg, const xmlChar * str)
{
	if(!xmlParserCtxt::IsEofInNonSaxMode(ctxt)) {
		if(ctxt)
			ctxt->errNo = XML_ERR_INTERNAL_ERROR;
		__xmlRaiseError(0, 0, 0, ctxt, 0, XML_FROM_PARSER, XML_ERR_INTERNAL_ERROR, XML_ERR_FATAL, NULL, 0, (const char *)str, NULL, NULL, 0, 0, msg, str);
		if(ctxt) {
			ctxt->wellFormed = 0;
			if(ctxt->recovery == 0)
				ctxt->disableSAX = 1;
		}
	}
}
/**
 * xmlErrEncodingInt:
 * @ctxt:  an XML parser context
 * @error:  the error number
 * @msg:  the error message
 * @val:  an integer value
 *
 * n encoding error
 */
static void FASTCALL xmlErrEncodingInt(xmlParserCtxt * ctxt, xmlParserErrors error, const char * msg, int val)
{
	if(!xmlParserCtxt::IsEofInNonSaxMode(ctxt)) {
		if(ctxt)
			ctxt->errNo = error;
		__xmlRaiseError(0, 0, 0, ctxt, 0, XML_FROM_PARSER, error, XML_ERR_FATAL, 0, 0, 0, 0, 0, val, 0, msg, val);
		if(ctxt) {
			ctxt->wellFormed = 0;
			if(ctxt->recovery == 0)
				ctxt->disableSAX = 1;
		}
	}
}
/**
 * xmlIsLetter:
 * @c:  an unicode character (int)
 *
 * Check whether the character is allowed by the production
 * [84] Letter ::= BaseChar | Ideographic
 *
 * Returns 0 if not, non-zero otherwise
 */
int xmlIsLetter(int c) 
{
	return (IS_BASECHAR(c) || IS_IDEOGRAPHIC(c));
}
// 
// Input handling functions for progressive parsing
// 
/* #define DEBUG_INPUT */
/* #define DEBUG_STACK */
/* #define DEBUG_PUSH */

/* we need to keep enough input to show errors in context */
#define LINE_LEN        80
#ifdef DEBUG_INPUT
	#define CHECK_BUFFER(in) check_buffer(in)

	static void check_buffer(xmlParserInput * in) 
	{
		if(in->base != xmlBufContent(in->buf->buffer)) {
			xmlGenericError(0, "xmlParserInput: base mismatch problem\n");
		}
		if(in->cur < in->base) {
			xmlGenericError(0, "xmlParserInput: cur < base problem\n");
		}
		if(in->cur > in->base + xmlBufUse(in->buf->buffer)) {
			xmlGenericError(0, "xmlParserInput: cur > base + use problem\n");
		}
		xmlGenericError(0, "buffer %x : content %x, cur %d, use %d\n",
			(int)in, (int)xmlBufContent(in->buf->buffer), in->cur - in->base, xmlBufUse(in->buf->buffer));
	}
#else
	#define CHECK_BUFFER(in)
#endif
/**
 * xmlParserInputRead:
 * @in:  an XML parser input
 * @len:  an indicative size for the lookahead
 *
 * This function was internal and is deprecated.
 *
 * Returns -1 as this is an error to use it.
 */
int xmlParserInputRead(xmlParserInput * in ATTRIBUTE_UNUSED, int len ATTRIBUTE_UNUSED) 
{
	return -1;
}
/**
 * xmlParserInputGrow:
 * @in:  an XML parser input
 * @len:  an indicative size for the lookahead
 *
 * This function increase the input for the parser. It tries to
 * preserve pointers to the input buffer, and keep already read data
 *
 * Returns the amount of char read, or -1 in case of error, 0 indicate the
 * end of this entity
 */
int FASTCALL xmlParserInputGrow(xmlParserInput * in, int len) 
{
	size_t ret;
	size_t indx;
	const xmlChar * content;
	if(!in || len < 0) 
		return -1;
#ifdef DEBUG_INPUT
	xmlGenericError(0, "Grow\n");
#endif
	if(!in->buf || !in->base || !in->cur || !in->buf->buffer)
		return -1;
	CHECK_BUFFER(in);
	indx = in->cur - in->base;
	if(xmlBufUse(in->buf->buffer) > (uint)indx + INPUT_CHUNK) {
		CHECK_BUFFER(in);
		return 0;
	}
	if(in->buf->readcallback)
		ret = xmlParserInputBufferGrow(in->buf, len);
	else
		return 0;
	/*
	 * NOTE : in->base may be a "dangling" i.e. freed pointer in this
	 *   block, but we use it really as an integer to do some
	 *   pointer arithmetic. Insure will raise it as a bug but in
	 *   that specific case, that's not !
	 */
	content = xmlBufContent(in->buf->buffer);
	if(in->base != content) {
		// the buffer has been reallocated
		indx = in->cur - in->base;
		in->base = content;
		in->cur = &content[indx];
	}
	in->end = xmlBufEnd(in->buf->buffer);
	CHECK_BUFFER(in);
	return ret;
}
/**
 * xmlParserInputShrink:
 * @in:  an XML parser input
 *
 * This function removes used input for the parser.
 */
void FASTCALL xmlParserInputShrink(xmlParserInput * in)
{
#ifdef DEBUG_INPUT
	xmlGenericError(0, "Shrink\n");
#endif
	if(in && in->buf && in->base && in->cur && in->buf->buffer) {
		CHECK_BUFFER(in);
		const size_t used = in->cur - xmlBufContent(in->buf->buffer);
		//
		// Do not shrink on large buffers whose only a tiny fraction was consumed
		//
		if(used > INPUT_CHUNK) {
			const size_t ret = xmlBufShrink(in->buf->buffer, used - LINE_LEN);
			if(ret > 0) {
				in->cur -= ret;
				in->consumed += ret;
			}
			in->end = xmlBufEnd(in->buf->buffer);
		}
		CHECK_BUFFER(in);
		if(xmlBufUse(in->buf->buffer) <= INPUT_CHUNK) {
			xmlParserInputBufferRead(in->buf, 2 * INPUT_CHUNK);
			const xmlChar * content = xmlBufContent(in->buf->buffer);
			if(in->base != content) {
				// the buffer has been reallocated
				const size_t indx = in->cur - in->base;
				in->base = content;
				in->cur = &content[indx];
			}
			in->end = xmlBufEnd(in->buf->buffer);
			CHECK_BUFFER(in);
		}
	}
}
// 
// UTF8 character input and related functions
// 
/**
 * xmlNextChar:
 * @ctxt:  the XML parser context
 *
 * Skip to the next char input char.
 */
void FASTCALL xmlNextChar(xmlParserCtxt * ctxt)
{
	if(!ctxt || ctxt->IsEof() || !ctxt->input)
		return;
	if(ctxt->charset == XML_CHAR_ENCODING_UTF8) {
		if((*ctxt->input->cur == 0) && (xmlParserInputGrow(ctxt->input, INPUT_CHUNK) <= 0) && (ctxt->instate != XML_PARSER_COMMENT)) {
			/*
			 * If we are at the end of the current entity and
			 * the context allows it, we pop consumed entities automatically.
			 * the auto closing should be blocked in other cases
			 */
			xmlPopInput(ctxt);
		}
		else {
			const uchar * cur;
			uchar c;
			/*
			 * 2.11 End-of-Line Handling
			 * the literal two-character sequence "#xD#xA" or a standalone
			 * literal #xD, an XML processor must pass to the application the single character #xA.
			 */
			if(*(ctxt->input->cur) == '\n') {
				ctxt->input->line++; 
				ctxt->input->col = 1;
			}
			else
				ctxt->input->col++;
			/*
			 * We are supposed to handle UTF8, check it's valid
			 * From rfc2044: encoding of the Unicode values on UTF-8:
			 *
			 * UCS-4 range (hex.)           UTF-8 octet sequence (binary)
			 * 0000 0000-0000 007F   0xxxxxxx
			 * 0000 0080-0000 07FF   110xxxxx 10xxxxxx
			 * 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx
			 *
			 * Check for the 0x110000 limit too
			 */
			cur = ctxt->input->cur;
			c = *cur;
			if(c & 0x80) {
				if(c == 0xC0)
					goto encoding_error;
				if(cur[1] == 0) {
					xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
					cur = ctxt->input->cur;
				}
				if((cur[1] & 0xc0) != 0x80)
					goto encoding_error;
				if((c & 0xe0) == 0xe0) {
					uint val;
					if(cur[2] == 0) {
						xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
						cur = ctxt->input->cur;
					}
					if((cur[2] & 0xc0) != 0x80)
						goto encoding_error;
					if((c & 0xf0) == 0xf0) {
						if(cur[3] == 0) {
							xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
							cur = ctxt->input->cur;
						}
						if(((c & 0xf8) != 0xf0) || ((cur[3] & 0xc0) != 0x80))
							goto encoding_error;
						/* 4-byte code */
						ctxt->input->cur += 4;
						val = (cur[0] & 0x7) << 18;
						val |= (cur[1] & 0x3f) << 12;
						val |= (cur[2] & 0x3f) << 6;
						val |= cur[3] & 0x3f;
					}
					else {
						/* 3-byte code */
						ctxt->input->cur += 3;
						val = (cur[0] & 0xf) << 12;
						val |= (cur[1] & 0x3f) << 6;
						val |= cur[2] & 0x3f;
					}
					if(((val > 0xd7ff) && (val < 0xe000)) || ((val > 0xfffd) && (val < 0x10000)) || (val >= 0x110000)) {
						xmlErrEncodingInt(ctxt, XML_ERR_INVALID_CHAR, "Char 0x%X out of allowed range\n", val);
					}
				}
				else
					ctxt->input->cur += 2; // 2-byte code 
			}
			else
				ctxt->input->cur++; // 1-byte code 
			ctxt->nbChars++;
			if(*ctxt->input->cur == 0)
				xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
		}
	}
	else {
		/*
		 * Assume it's a fixed length encoding (1) with a compatible encoding for the ASCII set, since
		 * XML constructs only use < 128 chars
		 */
		if(*(ctxt->input->cur) == '\n') {
			ctxt->input->line++; 
			ctxt->input->col = 1;
		}
		else
			ctxt->input->col++;
		ctxt->input->cur++;
		ctxt->nbChars++;
		if(*ctxt->input->cur == 0)
			xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
	}
	if((*ctxt->input->cur == '%') && (!ctxt->html))
		xmlParserHandlePEReference(ctxt);
	if((*ctxt->input->cur == 0) && (xmlParserInputGrow(ctxt->input, INPUT_CHUNK) <= 0))
		xmlPopInput(ctxt);
	return;
encoding_error:
	/*
	 * If we detect an UTF8 error that probably mean that the
	 * input encoding didn't get properly advertised in the
	 * declaration header. Report the error and switch the encoding
	 * to ISO-Latin-1 (if you don't like this policy, just declare the
	 * encoding !)
	 */
	if(!ctxt || !ctxt->input || (ctxt->input->end - ctxt->input->cur < 4)) {
		__xmlErrEncoding(ctxt, XML_ERR_INVALID_CHAR, "Input is not proper UTF-8, indicate encoding !\n", 0, 0);
	}
	else {
		char buffer[150];
		snprintf(buffer, 149, "Bytes: 0x%02X 0x%02X 0x%02X 0x%02X\n", ctxt->input->cur[0], ctxt->input->cur[1], ctxt->input->cur[2], ctxt->input->cur[3]);
		__xmlErrEncoding(ctxt, XML_ERR_INVALID_CHAR, "Input is not proper UTF-8, indicate encoding !\n%s", BAD_CAST buffer, 0);
	}
	ctxt->charset = XML_CHAR_ENCODING_8859_1;
	ctxt->input->cur++;
	return;
}
/**
 * xmlCurrentChar:
 * @ctxt:  the XML parser context
 * @len:  pointer to the length of the char read
 *
 * The current char value, if using UTF-8 this may actually span multiple
 * bytes in the input buffer. Implement the end of line normalization:
 * 2.11 End-of-Line Handling
 * Wherever an external parsed entity or the literal entity value
 * of an internal parsed entity contains either the literal two-character
 * sequence "#xD#xA" or a standalone literal #xD, an XML processor
 * must pass to the application the single character #xA.
 * This behavior can conveniently be produced by normalizing all
 * line breaks to #xA on input, before parsing.)
 *
 * Returns the current char value and its length
 */
int FASTCALL xmlCurrentChar(xmlParserCtxt * ctxt, int * len) 
{
	if(!ctxt || !len || !ctxt->input) 
		return 0;
	if(ctxt->IsEof())
		return 0;
	if((*ctxt->input->cur >= 0x20) && (*ctxt->input->cur <= 0x7F)) {
		*len = 1;
		return static_cast<int>(*ctxt->input->cur);
	}
	if(ctxt->charset == XML_CHAR_ENCODING_UTF8) {
		/*
		 * We are supposed to handle UTF8, check it's valid
		 * From rfc2044: encoding of the Unicode values on UTF-8:
		 *
		 * UCS-4 range (hex.)           UTF-8 octet sequence (binary)
		 * 0000 0000-0000 007F   0xxxxxxx
		 * 0000 0080-0000 07FF   110xxxxx 10xxxxxx
		 * 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx
		 *
		 * Check for the 0x110000 limit too
		 */
		const uchar * cur = ctxt->input->cur;
		uint val;
		uchar c = *cur;
		if(c & 0x80) {
			if(((c & 0x40) == 0) || (c == 0xC0))
				goto encoding_error;
			if(cur[1] == 0) {
				xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
				cur = ctxt->input->cur;
			}
			if((cur[1] & 0xc0) != 0x80)
				goto encoding_error;
			if((c & 0xe0) == 0xe0) {
				if(cur[2] == 0) {
					xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
					cur = ctxt->input->cur;
				}
				if((cur[2] & 0xc0) != 0x80)
					goto encoding_error;
				if((c & 0xf0) == 0xf0) {
					if(cur[3] == 0) {
						xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
						cur = ctxt->input->cur;
					}
					if(((c & 0xf8) != 0xf0) || ((cur[3] & 0xc0) != 0x80))
						goto encoding_error;
					/* 4-byte code */
					*len = 4;
					val = (cur[0] & 0x7) << 18;
					val |= (cur[1] & 0x3f) << 12;
					val |= (cur[2] & 0x3f) << 6;
					val |= cur[3] & 0x3f;
					if(val < 0x10000)
						goto encoding_error;
				}
				else {
					/* 3-byte code */
					*len = 3;
					val = (cur[0] & 0xf) << 12;
					val |= (cur[1] & 0x3f) << 6;
					val |= cur[2] & 0x3f;
					if(val < 0x800)
						goto encoding_error;
				}
			}
			else {
				/* 2-byte code */
				*len = 2;
				val = (cur[0] & 0x1f) << 6;
				val |= cur[1] & 0x3f;
				if(val < 0x80)
					goto encoding_error;
			}
			if(!IS_CHAR(val)) {
				xmlErrEncodingInt(ctxt, XML_ERR_INVALID_CHAR, "Char 0x%X out of allowed range\n", val);
			}
			return val;
		}
		else {
			// 1-byte code 
			*len = 1;
			if(*ctxt->input->cur == 0)
				xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
			if(*ctxt->input->cur == 0 && ctxt->input->end > ctxt->input->cur) {
				xmlErrEncodingInt(ctxt, XML_ERR_INVALID_CHAR, "Char 0x0 out of allowed range\n", 0);
			}
			if(*ctxt->input->cur == 0xD) {
				if(ctxt->input->cur[1] == 0xA) {
					ctxt->nbChars++;
					ctxt->input->cur++;
				}
				return (0xA);
			}
			return static_cast<int>(*ctxt->input->cur);
		}
	}
	// 
	// Assume it's a fixed length encoding (1) with
	// a compatible encoding for the ASCII set, since
	// XML constructs only use < 128 chars
	// 
	*len = 1;
	if(*ctxt->input->cur == 0xD) {
		if(ctxt->input->cur[1] == 0xA) {
			ctxt->nbChars++;
			ctxt->input->cur++;
		}
		return (0xA);
	}
	return static_cast<int>(*ctxt->input->cur);
encoding_error:
	// 
	// An encoding problem may arise from a truncated input buffer
	// splitting a character in the middle. In that case do not raise
	// an error but return 0 to endicate an end of stream problem
	// 
	if(ctxt->input->end - ctxt->input->cur < 4) {
		*len = 0;
		return 0;
	}
	// 
	// If we detect an UTF8 error that probably mean that the
	// input encoding didn't get properly advertised in the
	// declaration header. Report the error and switch the encoding
	// to ISO-Latin-1 (if you don't like this policy, just declare the encoding !)
	// 
	{
		char buffer[150];
		snprintf(&buffer[0], 149, "Bytes: 0x%02X 0x%02X 0x%02X 0x%02X\n",
		    ctxt->input->cur[0], ctxt->input->cur[1], ctxt->input->cur[2], ctxt->input->cur[3]);
		__xmlErrEncoding(ctxt, XML_ERR_INVALID_CHAR, "Input is not proper UTF-8, indicate encoding !\n%s", BAD_CAST buffer, 0);
	}
	ctxt->charset = XML_CHAR_ENCODING_8859_1;
	*len = 1;
	return static_cast<int>(*ctxt->input->cur);
}
/**
 * xmlStringCurrentChar:
 * @ctxt:  the XML parser context
 * @cur:  pointer to the beginning of the char
 * @len:  pointer to the length of the char read
 *
 * The current char value, if using UTF-8 this may actually span multiple bytes in the input buffer.
 *
 * Returns the current char value and its length
 */
int FASTCALL xmlStringCurrentChar(xmlParserCtxt * ctxt, const xmlChar * cur, int * len)
{
	if(!len || !cur) 
		return 0;
	if(!ctxt || (ctxt->charset == XML_CHAR_ENCODING_UTF8)) {
		/*
		 * We are supposed to handle UTF8, check it's valid
		 * From rfc2044: encoding of the Unicode values on UTF-8:
		 *
		 * UCS-4 range (hex.)           UTF-8 octet sequence (binary)
		 * 0000 0000-0000 007F   0xxxxxxx
		 * 0000 0080-0000 07FF   110xxxxx 10xxxxxx
		 * 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx
		 *
		 * Check for the 0x110000 limit too
		 */
		uint val;
		uchar c = *cur;
		if(c & 0x80) {
			if((cur[1] & 0xc0) != 0x80)
				goto encoding_error;
			if((c & 0xe0) == 0xe0) {
				if((cur[2] & 0xc0) != 0x80)
					goto encoding_error;
				if((c & 0xf0) == 0xf0) {
					if(((c & 0xf8) != 0xf0) || ((cur[3] & 0xc0) != 0x80))
						goto encoding_error;
					/* 4-byte code */
					*len = 4;
					val = (cur[0] & 0x7) << 18;
					val |= (cur[1] & 0x3f) << 12;
					val |= (cur[2] & 0x3f) << 6;
					val |= cur[3] & 0x3f;
				}
				else {
					/* 3-byte code */
					*len = 3;
					val = (cur[0] & 0xf) << 12;
					val |= (cur[1] & 0x3f) << 6;
					val |= cur[2] & 0x3f;
				}
			}
			else {
				/* 2-byte code */
				*len = 2;
				val = (cur[0] & 0x1f) << 6;
				val |= cur[1] & 0x3f;
			}
			if(!IS_CHAR(val)) {
				xmlErrEncodingInt(ctxt, XML_ERR_INVALID_CHAR, "Char 0x%X out of allowed range\n", val);
			}
			return (val);
		}
		else {
			/* 1-byte code */
			*len = 1;
			return ((int)*cur);
		}
	}
	/*
	 * Assume it's a fixed length encoding (1) with
	 * a compatible encoding for the ASCII set, since
	 * XML constructs only use < 128 chars
	 */
	*len = 1;
	return ((int)*cur);
encoding_error:
	/*
	 * An encoding problem may arise from a truncated input buffer
	 * splitting a character in the middle. In that case do not raise
	 * an error but return 0 to endicate an end of stream problem
	 */
	if(!ctxt || !ctxt->input || (ctxt->input->end - ctxt->input->cur < 4)) {
		*len = 0;
		return 0;
	}
	/*
	 * If we detect an UTF8 error that probably mean that the
	 * input encoding didn't get properly advertised in the
	 * declaration header. Report the error and switch the encoding
	 * to ISO-Latin-1 (if you don't like this policy, just declare the
	 * encoding !)
	 */
	{
		char buffer[150];
		snprintf(buffer, 149, "Bytes: 0x%02X 0x%02X 0x%02X 0x%02X\n", ctxt->input->cur[0], ctxt->input->cur[1], ctxt->input->cur[2], ctxt->input->cur[3]);
		__xmlErrEncoding(ctxt, XML_ERR_INVALID_CHAR, "Input is not proper UTF-8, indicate encoding !\n%s", BAD_CAST buffer, 0);
	}
	*len = 1;
	return ((int)*cur);
}
/**
 * xmlCopyCharMultiByte:
 * @out:  pointer to an array of xmlChar
 * @val:  the char value
 *
 * append the char value in the array
 *
 * Returns the number of xmlChar written
 */
int FASTCALL xmlCopyCharMultiByte(xmlChar * out, int val) 
{
	if(!out) 
		return 0;
	/*
	 * We are supposed to handle UTF8, check it's valid
	 * From rfc2044: encoding of the Unicode values on UTF-8:
	 *
	 * UCS-4 range (hex.)           UTF-8 octet sequence (binary)
	 * 0000 0000-0000 007F   0xxxxxxx
	 * 0000 0080-0000 07FF   110xxxxx 10xxxxxx
	 * 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx
	 */
	if(val >= 0x80) {
		xmlChar * savedout = out;
		int bits;
		if(val <   0x800) {
			*out++ = (val >>  6) | 0xC0;  
			bits =  0;
		}
		else if(val < 0x10000) {
			*out++ = (val >> 12) | 0xE0;  
			bits =  6;
		}
		else if(val < 0x110000) {
			*out++ = (val >> 18) | 0xF0;  
			bits =  12;
		}
		else {
			xmlErrEncodingInt(NULL, XML_ERR_INVALID_CHAR, "Internal error, xmlCopyCharMultiByte 0x%X out of bound\n", val);
			return 0;
		}
		for(; bits >= 0; bits -= 6)
			*out++ = ((val >> bits) & 0x3F) | 0x80;
		return (out - savedout);
	}
	*out = static_cast<xmlChar>(val);
	return 1;
}
/**
 * xmlCopyChar:
 * @len:  Ignored, compatibility
 * @out:  pointer to an array of xmlChar
 * @val:  the char value
 *
 * append the char value in the array
 *
 * Returns the number of xmlChar written
 */
int FASTCALL xmlCopyChar(int len ATTRIBUTE_UNUSED, xmlChar * out, int val) 
{
	// the len parameter is ignored 
	if(!out) 
		return 0;
	else if(val >= 0x80)
		return xmlCopyCharMultiByte(out, val);
	else {
		*out = static_cast<xmlChar>(val);
		return 1;
	}
}
//
// Commodity functions to switch encodings
//
static int xmlSwitchToEncodingInt(xmlParserCtxt * ctxt, xmlCharEncodingHandler * handler, int len);
static int xmlSwitchInputEncodingInt(xmlParserCtxt * ctxt, xmlParserInput * input, xmlCharEncodingHandler * handler, int len);
/**
 * xmlSwitchEncoding:
 * @ctxt:  the parser context
 * @enc:  the encoding value (number)
 *
 * change the input functions when discovering the character encoding
 * of a given entity.
 *
 * Returns 0 in case of success, -1 otherwise
 */
int xmlSwitchEncoding(xmlParserCtxt * ctxt, xmlCharEncoding enc)
{
	xmlCharEncodingHandler * handler;
	int len = -1;
	if(!ctxt) 
		return -1;
	switch(enc) {
		case XML_CHAR_ENCODING_ERROR:
		    __xmlErrEncoding(ctxt, XML_ERR_UNKNOWN_ENCODING, "encoding unknown\n", 0, 0);
		    return -1;
		case XML_CHAR_ENCODING_NONE:
		    // let's assume it's UTF-8 without the XML decl 
		    ctxt->charset = XML_CHAR_ENCODING_UTF8;
		    return 0;
		case XML_CHAR_ENCODING_UTF8:
		    // default encoding, no conversion should be needed 
		    ctxt->charset = XML_CHAR_ENCODING_UTF8;
		    /*
			Errata on XML-1.0 June 20 2001
			Specific handling of the Byte Order Mark for UTF-8
		     */
		    if(ctxt->input && (ctxt->input->cur[0] == 0xEF) && (ctxt->input->cur[1] == 0xBB) && (ctxt->input->cur[2] == 0xBF)) {
			    ctxt->input->cur += 3;
		    }
		    return 0;
		case XML_CHAR_ENCODING_UTF16LE:
		case XML_CHAR_ENCODING_UTF16BE:
		    /*The raw input characters are encoded
		       *in UTF-16. As we expect this function
		       *to be called after xmlCharEncInFunc, we expect
		       *ctxt->input->cur to contain UTF-8 encoded characters.
		       *So the raw UTF16 Byte Order Mark
		       *has also been converted into
		       *an UTF-8 BOM. Let's skip that BOM.
		     */
		    if(ctxt->input && ctxt->input->cur && (ctxt->input->cur[0] == 0xEF) && (ctxt->input->cur[1] == 0xBB) && (ctxt->input->cur[2] == 0xBF)) {
			    ctxt->input->cur += 3;
		    }
		    len = 90;
		    break;
		case XML_CHAR_ENCODING_UCS2: len = 90; break;
		case XML_CHAR_ENCODING_UCS4BE:
		case XML_CHAR_ENCODING_UCS4LE:
		case XML_CHAR_ENCODING_UCS4_2143:
		case XML_CHAR_ENCODING_UCS4_3412: len = 180; break;
		case XML_CHAR_ENCODING_EBCDIC:
		case XML_CHAR_ENCODING_8859_1:
		case XML_CHAR_ENCODING_8859_2:
		case XML_CHAR_ENCODING_8859_3:
		case XML_CHAR_ENCODING_8859_4:
		case XML_CHAR_ENCODING_8859_5:
		case XML_CHAR_ENCODING_8859_6:
		case XML_CHAR_ENCODING_8859_7:
		case XML_CHAR_ENCODING_8859_8:
		case XML_CHAR_ENCODING_8859_9:
		case XML_CHAR_ENCODING_ASCII:
		case XML_CHAR_ENCODING_2022_JP:
		case XML_CHAR_ENCODING_SHIFT_JIS:
		case XML_CHAR_ENCODING_EUC_JP: len = 45; break;
	}
	handler = xmlGetCharEncodingHandler(enc);
	if(handler == NULL) {
		/*
		 * Default handlers.
		 */
		switch(enc) {
			case XML_CHAR_ENCODING_ASCII:
			    /* default encoding, no conversion should be needed */
			    ctxt->charset = XML_CHAR_ENCODING_UTF8;
			    return 0;
			case XML_CHAR_ENCODING_UTF16LE:
			    break;
			case XML_CHAR_ENCODING_UTF16BE:
			    break;
			case XML_CHAR_ENCODING_UCS4LE: __xmlErrEncodingNotSupported(ctxt, reinterpret_cast<const xmlChar *>("USC4 little endian")); break;
			case XML_CHAR_ENCODING_UCS4BE: __xmlErrEncodingNotSupported(ctxt, reinterpret_cast<const xmlChar *>("USC4 big endian")); break;
			case XML_CHAR_ENCODING_EBCDIC: __xmlErrEncodingNotSupported(ctxt, reinterpret_cast<const xmlChar *>("EBCDIC")); break;
			case XML_CHAR_ENCODING_UCS4_2143: __xmlErrEncodingNotSupported(ctxt, reinterpret_cast<const xmlChar *>("UCS4 2143")); break;
			case XML_CHAR_ENCODING_UCS4_3412: __xmlErrEncodingNotSupported(ctxt, reinterpret_cast<const xmlChar *>("UCS4 3412")); break;
			case XML_CHAR_ENCODING_UCS2: __xmlErrEncodingNotSupported(ctxt, reinterpret_cast<const xmlChar *>("UCS2")); break;
			case XML_CHAR_ENCODING_8859_1:
			case XML_CHAR_ENCODING_8859_2:
			case XML_CHAR_ENCODING_8859_3:
			case XML_CHAR_ENCODING_8859_4:
			case XML_CHAR_ENCODING_8859_5:
			case XML_CHAR_ENCODING_8859_6:
			case XML_CHAR_ENCODING_8859_7:
			case XML_CHAR_ENCODING_8859_8:
			case XML_CHAR_ENCODING_8859_9:
			    /*
			 * We used to keep the internal content in the
			 * document encoding however this turns being unmaintainable
			 * So xmlGetCharEncodingHandler() will return non-null
			 * values for this now.
			     */
			    if((ctxt->inputNr == 1) && !ctxt->encoding && ctxt->input && ctxt->input->encoding) {
				    ctxt->encoding = sstrdup(ctxt->input->encoding);
			    }
			    ctxt->charset = enc;
			    return 0;
			case XML_CHAR_ENCODING_2022_JP: __xmlErrEncodingNotSupported(ctxt, reinterpret_cast<const xmlChar *>("ISO-2022-JP")); break;
			case XML_CHAR_ENCODING_SHIFT_JIS: __xmlErrEncodingNotSupported(ctxt, reinterpret_cast<const xmlChar *>("Shift_JIS")); break;
			case XML_CHAR_ENCODING_EUC_JP: __xmlErrEncodingNotSupported(ctxt, reinterpret_cast<const xmlChar *>("EUC-JP")); break;
			default: break;
		}
	}
	if(handler == NULL)
		return -1;
	else {
		ctxt->charset = XML_CHAR_ENCODING_UTF8;
		return xmlSwitchToEncodingInt(ctxt, handler, len);
	}
}
/**
 * xmlSwitchInputEncoding:
 * @ctxt:  the parser context
 * @input:  the input stream
 * @handler:  the encoding handler
 * @len:  the number of bytes to convert for the first line or -1
 *
 * change the input functions when discovering the character encoding
 * of a given entity.
 *
 * Returns 0 in case of success, -1 otherwise
 */
static int xmlSwitchInputEncodingInt(xmlParserCtxt * ctxt, xmlParserInput * input, xmlCharEncodingHandler * handler, int len)
{
	int nbchars;
	if(!handler || !input)
		return -1;
	if(input->buf) {
		if(input->buf->encoder) {
			/*
			 * Check in case the auto encoding detetection triggered in already.
			 */
			if(input->buf->encoder == handler)
				return 0;
			/*
			 * "UTF-16" can be used for both LE and BE
			   if((!xmlStrncmp(BAD_CAST input->buf->encoder->name,
			   reinterpret_cast<const xmlChar *>("UTF-16"), 6)) &&
			   (!xmlStrncmp(BAD_CAST handler->name,
			   reinterpret_cast<const xmlChar *>("UTF-16"), 6))) {
			   return 0;
			   }
			 */
			// 
			// Note: this is a bit dangerous, but that's what it
			// takes to use nearly compatible signature for different encodings.
			// 
			xmlCharEncCloseFunc(input->buf->encoder);
			input->buf->encoder = handler;
			return 0;
		}
		input->buf->encoder = handler;
		// 
		// Is there already some content down the pipe to convert ?
		// 
		if(xmlBufIsEmpty(input->buf->buffer) == 0) {
			int processed;
			uint use;
			//
			// Specific handling of the Byte Order Mark for UTF-16
			// 
			if(handler->name) {
				if((sstreq(handler->name, "UTF-16LE") || sstreq(handler->name, "UTF-16")) && (input->cur[0] == 0xFF) && (input->cur[1] == 0xFE))
					input->cur += 2;
				if(sstreq(handler->name, "UTF-16BE") && (input->cur[0] == 0xFE) && (input->cur[1] == 0xFF))
					input->cur += 2;
				// 
				// Errata on XML-1.0 June 20 2001
				// Specific handling of the Byte Order Mark for UTF-8
				// 
				if(sstreq(handler->name, "UTF-8") && (input->cur[0] == 0xEF) && (input->cur[1] == 0xBB) && (input->cur[2] == 0xBF))
					input->cur += 3;
			}
			// 
			// Shrink the current input buffer.
			// Move it as the raw buffer and create a new input buffer
			// 
			processed = input->cur - input->base;
			xmlBufShrink(input->buf->buffer, processed);
			input->buf->raw = input->buf->buffer;
			input->buf->buffer = xmlBufCreate();
			input->buf->rawconsumed = processed;
			use = xmlBufUse(input->buf->raw);
			if(ctxt->html) {
				/*
				 * convert as much as possible of the buffer
				 */
				nbchars = xmlCharEncInput(input->buf, 1);
			}
			else {
				/*
				 * convert just enough to get
				 * '<?xml version="1.0" encoding="xxx"?>'
				 * parsed with the autodetected encoding
				 * into the parser reading buffer.
				 */
				nbchars = xmlCharEncFirstLineInput(input->buf, len);
			}
			if(nbchars < 0) {
				xmlErrInternal(ctxt, "switching encoding: encoder error\n", 0);
				return -1;
			}
			input->buf->rawconsumed += use - xmlBufUse(input->buf->raw);
			xmlBufResetInput(input->buf->buffer, input);
		}
		return 0;
	}
	else if(input->length == 0) {
		/*
		 * When parsing a static memory array one must know the
		 * size to be able to convert the buffer.
		 */
		xmlErrInternal(ctxt, "switching encoding : no input\n", 0);
		return -1;
	}
	return 0;
}
/**
 * xmlSwitchInputEncoding:
 * @ctxt:  the parser context
 * @input:  the input stream
 * @handler:  the encoding handler
 *
 * change the input functions when discovering the character encoding
 * of a given entity.
 *
 * Returns 0 in case of success, -1 otherwise
 */
int xmlSwitchInputEncoding(xmlParserCtxt * ctxt, xmlParserInput * input, xmlCharEncodingHandler * handler)
{
	return xmlSwitchInputEncodingInt(ctxt, input, handler, -1);
}
/**
 * xmlSwitchToEncodingInt:
 * @ctxt:  the parser context
 * @handler:  the encoding handler
 * @len: the length to convert or -1
 *
 * change the input functions when discovering the character encoding
 * of a given entity, and convert only @len bytes of the output, this
 * is needed on auto detect to allows any declared encoding later to
 * convert the actual content after the xmlDecl
 *
 * Returns 0 in case of success, -1 otherwise
 */
static int xmlSwitchToEncodingInt(xmlParserCtxt * ctxt, xmlCharEncodingHandler * handler, int len)
{
	int ret = -1;
	if(handler) {
		if(ctxt->input) {
			ret = xmlSwitchInputEncodingInt(ctxt, ctxt->input, handler, len);
			// The parsing is now done in UTF8 natively
			ctxt->charset = XML_CHAR_ENCODING_UTF8;
		}
		else
			xmlErrInternal(ctxt, "xmlSwitchToEncoding : no input\n", 0);
	}
	return ret;
}
/**
 * xmlSwitchToEncoding:
 * @ctxt:  the parser context
 * @handler:  the encoding handler
 *
 * change the input functions when discovering the character encoding
 * of a given entity.
 *
 * Returns 0 in case of success, -1 otherwise
 */
int xmlSwitchToEncoding(xmlParserCtxt * ctxt, xmlCharEncodingHandler * handler)
{
	return xmlSwitchToEncodingInt(ctxt, handler, -1);
}
// 
// Commodity functions to handle entities processing
// 
/**
 * xmlFreeInputStream:
 * @input:  an xmlParserInputPtr
 *
 * Free up an input stream.
 */
void FASTCALL xmlFreeInputStream(xmlParserInput * input)
{
	if(input) {
		SAlloc::F((char *)input->filename);
		SAlloc::F((char *)input->directory);
		SAlloc::F((char *)input->encoding);
		SAlloc::F((char *)input->version);
		if(input->FnFree && input->base)
			input->FnFree((xmlChar *)input->base);
		xmlFreeParserInputBuffer(input->buf);
		SAlloc::F(input);
	}
}
/**
 * xmlNewInputStream:
 * @ctxt:  an XML parser context
 *
 * Create a new input stream structure.
 *
 * Returns the new input stream or NULL
 */
xmlParserInput * xmlNewInputStream(xmlParserCtxt * ctxt)
{
	xmlParserInput * input = static_cast<xmlParserInput *>(SAlloc::M(sizeof(xmlParserInput)));
	if(!input)
		xmlErrMemory(ctxt,  "couldn't allocate a new input stream\n");
	else {
		memzero(input, sizeof(xmlParserInput));
		input->line = 1;
		input->col = 1;
		input->standalone = -1;
		/*
		 * If the context is NULL the id cannot be initialized, but that
		 * should not happen while parsing which is the situation where the id is actually needed.
		 */
		if(ctxt)
			input->id = ctxt->input_id++;
	}
	return input;
}
/**
 * xmlNewIOInputStream:
 * @ctxt:  an XML parser context
 * @input:  an I/O Input
 * @enc:  the charset encoding if known
 *
 * Create a new input stream structure encapsulating the @input into
 * a stream suitable for the parser.
 *
 * Returns the new input stream or NULL
 */
xmlParserInput * xmlNewIOInputStream(xmlParserCtxt * ctxt, xmlParserInputBuffer * input, xmlCharEncoding enc)
{
	xmlParserInput * inputStream = 0;
	if(input) {
		if(xmlParserDebugEntities)
			xmlGenericError(0, "new input from I/O\n");
		inputStream = xmlNewInputStream(ctxt);
		if(inputStream) {
			inputStream->filename = NULL;
			inputStream->buf = input;
			xmlBufResetInput(inputStream->buf->buffer, inputStream);
			if(enc != XML_CHAR_ENCODING_NONE)
				xmlSwitchEncoding(ctxt, enc);
		}
	}
	return inputStream;
}
/**
 * xmlNewEntityInputStream:
 * @ctxt:  an XML parser context
 * @entity:  an Entity pointer
 *
 * Create a new input stream based on an xmlEntityPtr
 *
 * Returns the new input stream or NULL
 */
xmlParserInput * xmlNewEntityInputStream(xmlParserCtxt * ctxt, xmlEntity * entity)
{
	xmlParserInput * input = 0;
	if(entity == NULL) {
		xmlErrInternal(ctxt, "xmlNewEntityInputStream entity = NULL\n", 0);
	}
	else {
		if(xmlParserDebugEntities)
			xmlGenericError(0, "new input from entity: %s\n", entity->name);
		if(entity->content == NULL) {
			switch(entity->etype) {
				case XML_EXTERNAL_GENERAL_UNPARSED_ENTITY: xmlErrInternal(ctxt, "Cannot parse entity %s\n", entity->name); break;
				case XML_EXTERNAL_GENERAL_PARSED_ENTITY:
				case XML_EXTERNAL_PARAMETER_ENTITY: return xmlLoadExternalEntity((char *)entity->URI, (char *)entity->ExternalID, ctxt);
				case XML_INTERNAL_GENERAL_ENTITY: xmlErrInternal(ctxt, "Internal entity %s without content !\n", entity->name); break;
				case XML_INTERNAL_PARAMETER_ENTITY: xmlErrInternal(ctxt, "Internal parameter entity %s without content !\n", entity->name); break;
				case XML_INTERNAL_PREDEFINED_ENTITY: xmlErrInternal(ctxt, "Predefined entity %s without content !\n", entity->name); break;
			}
		}
		else {
			input = xmlNewInputStream(ctxt);
			if(input) {
				input->filename = (char *)sstrdup(entity->URI);
				input->base = entity->content;
				input->cur = entity->content;
				input->length = entity->length;
				input->end = &entity->content[input->length];
			}
		}
	}
	return input;
}
/**
 * xmlNewStringInputStream:
 * @ctxt:  an XML parser context
 * @buffer:  an memory buffer
 *
 * Create a new input stream based on a memory buffer.
 * Returns the new input stream
 */
xmlParserInput * xmlNewStringInputStream(xmlParserCtxt * ctxt, const xmlChar * buffer)
{
	xmlParserInput * input = 0;
	if(!buffer) {
		xmlErrInternal(ctxt, "xmlNewStringInputStream string = NULL\n", 0);
	}
	else {
		if(xmlParserDebugEntities)
			xmlGenericError(0, "new fixed input: %.30s\n", buffer);
		input = xmlNewInputStream(ctxt);
		if(!input)
			xmlErrMemory(ctxt,  "couldn't allocate a new input stream\n");
		else {
			input->base = buffer;
			input->cur = buffer;
			input->length = sstrlen(buffer);
			input->end = &buffer[input->length];
		}
	}
	return input;
}
/**
 * xmlNewInputFromFile:
 * @ctxt:  an XML parser context
 * @filename:  the filename to use as entity
 *
 * Create a new input stream based on a file or an URL.
 *
 * Returns the new input stream or NULL in case of error
 */
xmlParserInput * xmlNewInputFromFile(xmlParserCtxt * ctxt, const char * filename)
{
	xmlParserInput * inputStream = 0;
	if(xmlParserDebugEntities)
		xmlGenericError(0, "new input from file: %s\n", filename);
	if(ctxt) {
		xmlParserInputBuffer * buf = xmlParserInputBufferCreateFilename(filename, XML_CHAR_ENCODING_NONE);
		if(!buf) {
			SString msg_buf("failed to load external entity");
			if(filename)
				msg_buf.Space().CatQStr(filename);
			else
				msg_buf.CatDiv(':', 2).Cat("NULL").Space().Cat("filename").Space();
			__xmlLoaderErr(ctxt, msg_buf.CR(), (const char *)filename);
		}
		else {
			inputStream = xmlNewInputStream(ctxt);
			if(inputStream) {
				inputStream->buf = buf;
				inputStream = xmlCheckHTTPInput(ctxt, inputStream);
				if(inputStream) {
					xmlChar * URI = inputStream->filename ? sstrdup((xmlChar *)inputStream->filename) : sstrdup((xmlChar *)filename);
					char * directory = xmlParserGetDirectory((const char *)URI);
					SAlloc::F((char *)inputStream->filename);
					inputStream->filename = (char *)xmlCanonicPath((const xmlChar *)URI);
					SAlloc::F((char *)URI);
					inputStream->directory = directory;
					xmlBufResetInput(inputStream->buf->buffer, inputStream);
					SETIFZ(ctxt->directory, sstrdup(directory));
				}
			}
		}
	}
	return inputStream;
}
// 
// Commodity functions to handle parser contexts
// 
// 
// Initialize a parser context
// @ctxt:  an XML parser context
// Returns 0 in case of success and -1 in case of error
// 
int xmlInitParserCtxt(xmlParserCtxt * ctxt)
{
	if(!ctxt) {
		xmlErrInternal(NULL, "Got NULL parser context\n", 0);
		return -1;
	}
	xmlDefaultSAXHandlerInit();
	SETIFZ(ctxt->dict, xmlDictCreate());
	if(!ctxt->dict) {
		xmlErrMemory(NULL, "cannot initialize parser context\n");
		return -1;
	}
	xmlDictSetLimit(ctxt->dict, XML_MAX_DICTIONARY_LIMIT);
	SETIFZ(ctxt->sax, static_cast<xmlSAXHandler *>(SAlloc::M(sizeof(xmlSAXHandler))));
	if(ctxt->sax == NULL) {
		xmlErrMemory(NULL, "cannot initialize parser context\n");
		return -1;
	}
	else
		xmlSAXVersion(ctxt->sax, 2);
	ctxt->maxatts = 0;
	ctxt->atts = NULL;
	/* Allocate the Input stack */
	if(ctxt->inputTab == NULL) {
		ctxt->inputTab = static_cast<xmlParserInput **>(SAlloc::M(5 * sizeof(xmlParserInput *)));
		ctxt->inputMax = 5;
	}
	if(ctxt->inputTab == NULL) {
		xmlErrMemory(NULL, "cannot initialize parser context\n");
		ctxt->inputNr = 0;
		ctxt->inputMax = 0;
		ctxt->input = NULL;
		return -1;
	}
	{
		xmlParserInput * p_input;
		while((p_input = inputPop(ctxt)) != NULL) { // Non consuming 
			xmlFreeInputStream(p_input);
		}
	}
	ctxt->inputNr = 0;
	ctxt->input = NULL;
	ctxt->version = NULL;
	ctxt->encoding = NULL;
	ctxt->standalone = -1;
	ctxt->hasExternalSubset = 0;
	ctxt->hasPErefs = 0;
	ctxt->html = 0;
	ctxt->external = 0;
	ctxt->instate = XML_PARSER_START;
	ctxt->token = 0;
	ctxt->directory = NULL;
	/* Allocate the Node stack */
	if(ctxt->PP_NodeTab == NULL) {
		ctxt->PP_NodeTab = static_cast<xmlNode **>(SAlloc::M(10 * sizeof(xmlNode *)));
		ctxt->nodeMax = 10;
	}
	if(ctxt->PP_NodeTab == NULL) {
		xmlErrMemory(NULL, "cannot initialize parser context\n");
		ctxt->nodeNr = 0;
		ctxt->nodeMax = 0;
		ctxt->P_Node = NULL;
		ctxt->inputNr = 0;
		ctxt->inputMax = 0;
		ctxt->input = NULL;
		return -1;
	}
	ctxt->nodeNr = 0;
	ctxt->P_Node = NULL;
	/* Allocate the Name stack */
	if(ctxt->nameTab == NULL) {
		ctxt->nameTab = static_cast<const xmlChar **>(SAlloc::M(10 * sizeof(xmlChar *)));
		ctxt->nameMax = 10;
	}
	if(ctxt->nameTab == NULL) {
		xmlErrMemory(NULL, "cannot initialize parser context\n");
		ctxt->nodeNr = 0;
		ctxt->nodeMax = 0;
		ctxt->P_Node = NULL;
		ctxt->inputNr = 0;
		ctxt->inputMax = 0;
		ctxt->input = NULL;
		ctxt->nameNr = 0;
		ctxt->nameMax = 0;
		ctxt->name = NULL;
		return -1;
	}
	ctxt->nameNr = 0;
	ctxt->name = NULL;
	/* Allocate the space stack */
	if(ctxt->spaceTab == NULL) {
		ctxt->spaceTab = static_cast<int *>(SAlloc::M(10 * sizeof(int)));
		ctxt->spaceMax = 10;
	}
	if(ctxt->spaceTab == NULL) {
		xmlErrMemory(NULL, "cannot initialize parser context\n");
		ctxt->nodeNr = 0;
		ctxt->nodeMax = 0;
		ctxt->P_Node = NULL;
		ctxt->inputNr = 0;
		ctxt->inputMax = 0;
		ctxt->input = NULL;
		ctxt->nameNr = 0;
		ctxt->nameMax = 0;
		ctxt->name = NULL;
		ctxt->spaceNr = 0;
		ctxt->spaceMax = 0;
		ctxt->space = NULL;
		return -1;
	}
	ctxt->spaceNr = 1;
	ctxt->spaceMax = 10;
	ctxt->spaceTab[0] = -1;
	ctxt->space = &ctxt->spaceTab[0];
	ctxt->userData = ctxt;
	ctxt->myDoc = NULL;
	ctxt->wellFormed = 1;
	ctxt->nsWellFormed = 1;
	ctxt->valid = 1;
	ctxt->loadsubset = xmlLoadExtDtdDefaultValue;
	if(ctxt->loadsubset) {
		ctxt->options |= XML_PARSE_DTDLOAD;
	}
	ctxt->validate = xmlDoValidityCheckingDefaultValue;
	ctxt->pedantic = xmlPedanticParserDefaultValue;
	if(ctxt->pedantic) {
		ctxt->options |= XML_PARSE_PEDANTIC;
	}
	ctxt->linenumbers = xmlLineNumbersDefaultValue;
	ctxt->keepBlanks = xmlKeepBlanksDefaultValue;
	if(ctxt->keepBlanks == 0) {
		ctxt->sax->ignorableWhitespace = xmlSAX2IgnorableWhitespace;
		ctxt->options |= XML_PARSE_NOBLANKS;
	}
	ctxt->vctxt.finishDtd = XML_CTXT_FINISH_DTD_0;
	ctxt->vctxt.userData = ctxt;
	ctxt->vctxt.error = xmlParserValidityError;
	ctxt->vctxt.warning = xmlParserValidityWarning;
	if(ctxt->validate) {
		ctxt->vctxt.warning = xmlGetWarningsDefaultValue ? xmlParserValidityWarning : 0;
		ctxt->vctxt.nodeMax = 0;
		ctxt->options |= XML_PARSE_DTDVALID;
	}
	ctxt->replaceEntities = xmlSubstituteEntitiesDefaultValue;
	if(ctxt->replaceEntities) {
		ctxt->options |= XML_PARSE_NOENT;
	}
	ctxt->record_info = 0;
	ctxt->nbChars = 0;
	ctxt->CheckIndex = 0;
	ctxt->inSubset = 0;
	ctxt->errNo = XML_ERR_OK;
	ctxt->depth = 0;
	ctxt->charset = XML_CHAR_ENCODING_UTF8;
	ctxt->catalogs = NULL;
	ctxt->nbentities = 0;
	ctxt->sizeentities = 0;
	ctxt->sizeentcopy = 0;
	ctxt->input_id = 1;
	xmlInitNodeInfoSeq(&ctxt->node_seq);
	return 0;
}
/**
 * xmlFreeParserCtxt:
 * @ctxt:  an XML parser context
 *
 * Free all the memory used by a parser context. However the parsed
 * document in ctxt->myDoc is not freed.
 */
void FASTCALL xmlFreeParserCtxt(xmlParserCtxt * ctxt)
{
	xmlParserInput * input;
	if(ctxt) {
		while((input = inputPop(ctxt)) != NULL) { // Non consuming
			xmlFreeInputStream(input);
		}
		SAlloc::F(ctxt->spaceTab);
		SAlloc::F((xmlChar **)ctxt->nameTab);
		SAlloc::F(ctxt->PP_NodeTab);
		SAlloc::F(ctxt->nodeInfoTab);
		SAlloc::F(ctxt->inputTab);
		SAlloc::F((char *)ctxt->version);
		SAlloc::F((char *)ctxt->encoding);
		SAlloc::F((char *)ctxt->extSubURI);
		SAlloc::F((char *)ctxt->extSubSystem);
	#ifdef LIBXML_SAX1_ENABLED
		if(ctxt->sax && (ctxt->sax != (xmlSAXHandler *)&xmlDefaultSAXHandler))
	#else
		if(ctxt->sax)
	#endif /* LIBXML_SAX1_ENABLED */
			SAlloc::F(ctxt->sax);
		SAlloc::F((char *)ctxt->directory);
		SAlloc::F(ctxt->vctxt.PP_NodeTab);
		SAlloc::F((xmlChar **)ctxt->atts);
		xmlDictFree(ctxt->dict);
		SAlloc::F((char *)ctxt->nsTab);
		SAlloc::F(ctxt->pushTab);
		SAlloc::F(ctxt->attallocs);
		xmlHashFree(ctxt->attsDefault, (xmlHashDeallocator)free);
		xmlHashFree(ctxt->attsSpecial, 0);
		if(ctxt->freeElems) {
			for(xmlNode * cur = ctxt->freeElems; cur;) {
				xmlNode * next = cur->next;
				SAlloc::F(cur);
				cur = next;
			}
		}
		if(ctxt->freeAttrs) {
			for(xmlAttr * cur = ctxt->freeAttrs; cur;) {
				xmlAttr * next = cur->next;
				SAlloc::F(cur);
				cur = next;
			}
		}
		// 
		// cleanup the error strings
		// 
		SAlloc::F(ctxt->lastError.message);
		SAlloc::F(ctxt->lastError.file);
		SAlloc::F(ctxt->lastError.str1);
		SAlloc::F(ctxt->lastError.str2);
		SAlloc::F(ctxt->lastError.str3);
#ifdef LIBXML_CATALOG_ENABLED
		xmlCatalogFreeLocal(ctxt->catalogs);
#endif
		SAlloc::F(ctxt);
	}
}
// 
// Descr: Allocate and initialize a new parser context.
// Returns: the xmlParserCtxtPtr or NULL
// 
xmlParserCtxt * xmlNewParserCtxt()
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(SAlloc::M(sizeof(xmlParserCtxt)));
	if(!ctxt)
		xmlErrMemory(NULL, "cannot allocate parser context\n");
	else {
		memzero(ctxt, sizeof(xmlParserCtxt));
		if(xmlInitParserCtxt(ctxt) < 0) {
			xmlFreeParserCtxt(ctxt);
			ctxt = 0;
		}
	}
	return ctxt;
}
// 
// Handling of node informations
// 
// 
// Descr: Clear (release owned resources) and reinitialize a parser context
// @ctxt:  an XML parser context
// 
void xmlClearParserCtxt(xmlParserCtxt * ctxt)
{
	if(ctxt) {
		xmlClearNodeInfoSeq(&ctxt->node_seq);
		xmlCtxtReset(ctxt);
	}
}
/**
 * xmlParserFindNodeInfo:
 * @ctx:  an XML parser context
 * @node:  an XML node within the tree
 *
 * Find the parser node info struct for a given node
 *
 * Returns an xmlParserNodeInfo block pointer or NULL
 */
const xmlParserNodeInfo * xmlParserFindNodeInfo(const xmlParserCtxt * ctx, const xmlNode * pNode)
{
	if(ctx && pNode) {
		// Find position where node should be at 
		ulong pos = xmlParserFindNodeInfoIndex(&ctx->node_seq, pNode);
		return (pos < ctx->node_seq.length && ctx->node_seq.buffer[pos].P_Node == pNode) ? &ctx->node_seq.buffer[pos] : 0;
	}
	else
		return 0;
}
/**
 * xmlInitNodeInfoSeq:
 * @seq:  a node info sequence pointer
 *
 * -- Initialize (set to initial state) node info sequence
 */
void xmlInitNodeInfoSeq(xmlParserNodeInfoSeqPtr seq)
{
	if(seq) {
		seq->length = 0;
		seq->maximum = 0;
		seq->buffer = NULL;
	}
}
/**
 * xmlClearNodeInfoSeq:
 * @seq:  a node info sequence pointer
 *
 * -- Clear (release memory and reinitialize) node
 * info sequence
 */
void xmlClearNodeInfoSeq(xmlParserNodeInfoSeqPtr seq)
{
	if(seq) {
		SAlloc::F(seq->buffer);
		xmlInitNodeInfoSeq(seq);
	}
}
/**
 * xmlParserFindNodeInfoIndex:
 * @seq:  a node info sequence pointer
 * @node:  an XML node pointer
 *
 *
 * xmlParserFindNodeInfoIndex : Find the index that the info record for
 * the given node is or should be at in a sorted sequence
 *
 * Returns a long indicating the position of the record
 */
ulong xmlParserFindNodeInfoIndex(const xmlParserNodeInfoSeq * seq, const xmlNode * pNode)
{
	if(!seq || !pNode)
		return ((ulong)-1);
	else {
		// Do a binary search for the key 
		ulong lower = 1;
		ulong upper = seq->length;
		ulong middle = 0;
		int   found = 0;
		while(lower <= upper && !found) {
			middle = lower + (upper - lower) / 2;
			if(pNode == seq->buffer[middle-1].P_Node)
				found = 1;
			else if(pNode < seq->buffer[middle-1].P_Node)
				upper = middle - 1;
			else
				lower = middle + 1;
		}
		// Return position 
		return (middle == 0 || seq->buffer[middle-1].P_Node < pNode) ? middle : (middle-1);
	}
}
/**
 * xmlParserAddNodeInfo:
 * @ctxt:  an XML parser context
 * @info:  a node info sequence pointer
 *
 * Insert node info record into the sorted sequence
 */
void xmlParserAddNodeInfo(xmlParserCtxt * ctxt, const xmlParserNodeInfoPtr info)
{
	if(ctxt && info) {
		// Find pos and check to see if node is already in the sequence 
		ulong pos = xmlParserFindNodeInfoIndex(&ctxt->node_seq, (xmlNode *)info->P_Node);
		if((pos < ctxt->node_seq.length) && (ctxt->node_seq.buffer != NULL) && (ctxt->node_seq.buffer[pos].P_Node == info->P_Node)) {
			ctxt->node_seq.buffer[pos] = *info;
		}
		/* Otherwise, we need to add new node to buffer */
		else {
			if((ctxt->node_seq.length + 1 > ctxt->node_seq.maximum) || (ctxt->node_seq.buffer == NULL)) {
				xmlParserNodeInfo * tmp_buffer;
				uint byte_size;
				SETIFZ(ctxt->node_seq.maximum, 2);
				byte_size = (sizeof(*ctxt->node_seq.buffer) * (2 * ctxt->node_seq.maximum));
				tmp_buffer = ctxt->node_seq.buffer ? (xmlParserNodeInfo*)SAlloc::R(ctxt->node_seq.buffer, byte_size) : (xmlParserNodeInfo*)SAlloc::M(byte_size);
				if(tmp_buffer == NULL) {
					xmlErrMemory(ctxt, "failed to allocate buffer\n");
					return;
				}
				ctxt->node_seq.buffer = tmp_buffer;
				ctxt->node_seq.maximum *= 2;
			}
			// If position is not at end, move elements out of the way 
			if(pos != ctxt->node_seq.length) {
				for(ulong i = ctxt->node_seq.length; i > pos; i--)
					ctxt->node_seq.buffer[i] = ctxt->node_seq.buffer[i - 1];
			}
			// Copy element and increase length 
			ctxt->node_seq.buffer[pos] = *info;
			ctxt->node_seq.length++;
		}
	}
}
//
// Defaults settings
//
/**
 * xmlPedanticParserDefault:
 * @val:  int 0 or 1
 *
 * Set and return the previous value for enabling pedantic warnings.
 *
 * Returns the last value for 0 for no substitution, 1 for substitution.
 */
int xmlPedanticParserDefault(int val)
{
	const int old = xmlPedanticParserDefaultValue;
	xmlPedanticParserDefaultValue = val;
	return old;
}
/**
 * xmlLineNumbersDefault:
 * @val:  int 0 or 1
 *
 * Set and return the previous value for enabling line numbers in elements
 * contents. This may break on old application and is turned off by default.
 *
 * Returns the last value for 0 for no substitution, 1 for substitution.
 */
int xmlLineNumbersDefault(int val)
{
	const int old = xmlLineNumbersDefaultValue;
	xmlLineNumbersDefaultValue = val;
	return old;
}
/**
 * xmlSubstituteEntitiesDefault:
 * @val:  int 0 or 1
 *
 * Set and return the previous value for default entity support.
 * Initially the parser always keep entity references instead of substituting
 * entity values in the output. This function has to be used to change the
 * default parser behavior
 * SAX::substituteEntities() has to be used for changing that on a file by
 * file basis.
 *
 * Returns the last value for 0 for no substitution, 1 for substitution.
 */
int xmlSubstituteEntitiesDefault(int val)
{
	const int old = xmlSubstituteEntitiesDefaultValue;
	xmlSubstituteEntitiesDefaultValue = val;
	return old;
}
/**
 * xmlKeepBlanksDefault:
 * @val:  int 0 or 1
 *
 * Set and return the previous value for default blanks text nodes support.
 * The 1.x version of the parser used an heuristic to try to detect
 * ignorable white spaces. As a result the SAX callback was generating
 * xmlSAX2IgnorableWhitespace() callbacks instead of characters() one, and when
 * using the DOM output text nodes containing those blanks were not generated.
 * The 2.x and later version will switch to the XML standard way and
 * ignorableWhitespace() are only generated when running the parser in
 * validating mode and when the current element doesn't allow CDATA or
 * mixed content.
 * This function is provided as a way to force the standard behavior
 * on 1.X libs and to switch back to the old mode for compatibility when
 * running 1.X client code on 2.X . Upgrade of 1.X code should be done
 * by using xmlIsBlankNode() commodity function to detect the "empty"
 * nodes generated.
 * This value also affect autogeneration of indentation when saving code
 * if blanks sections are kept, indentation is not generated.
 *
 * Returns the last value for 0 for no substitution, 1 for substitution.
 */
int xmlKeepBlanksDefault(int val)
{
	const int old = xmlKeepBlanksDefaultValue;
	xmlKeepBlanksDefaultValue = val;
	if(!val)
		xmlIndentTreeOutput = 1;
	return old;
}

#define bottom_parserInternals
