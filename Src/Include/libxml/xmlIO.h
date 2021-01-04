/*
 * Summary: interface for the I/O interfaces used by the parser
 * Description: interface for the I/O interfaces used by the parser
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */
#ifndef __XML_IO_H__
#define __XML_IO_H__

struct xmlBuffer;
struct xmlBuf;
struct xmlParserCtxt;
struct xmlCharEncodingHandler;
struct xmlParserInput;

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Those are the functions and datatypes for the parser input I/O structures.
 */
/**
 * xmlInputMatchCallback:
 * @filename: the filename or URI
 *
 * Callback used in the I/O Input API to detect if the current handler
 * can provide input fonctionnalities for this resource.
 *
 * Returns 1 if yes and 0 if another Input module should be used
 */
typedef int (XMLCALL *xmlInputMatchCallback)(char const *filename);
/**
 * xmlInputOpenCallback:
 * @filename: the filename or URI
 *
 * Callback used in the I/O Input API to open the resource
 *
 * Returns an Input context or NULL in case or error
 */
typedef void * (XMLCALL * xmlInputOpenCallback)(char const *filename);
/**
 * xmlInputReadCallback:
 * @context:  an Input context
 * @buffer:  the buffer to store data read
 * @len:  the length of the buffer in bytes
 *
 * Callback used in the I/O Input API to read the resource
 *
 * Returns the number of bytes read or -1 in case of error
 */
typedef int (XMLCALL * xmlInputReadCallback)(void * context, char * buffer, int len);
/**
 * xmlInputCloseCallback:
 * @context:  an Input context
 *
 * Callback used in the I/O Input API to close the resource
 *
 * Returns 0 or -1 in case of error
 */
typedef int (XMLCALL *xmlInputCloseCallback)(void * context);

#ifdef LIBXML_OUTPUT_ENABLED
	/*
	 * Those are the functions and datatypes for the library output I/O structures.
	 */
	/**
	 * xmlOutputMatchCallback:
	 * @filename: the filename or URI
	 *
	 * Callback used in the I/O Output API to detect if the current handler
	 * can provide output fonctionnalities for this resource.
	 *
	 * Returns 1 if yes and 0 if another Output module should be used
	 */
	typedef int (XMLCALL *xmlOutputMatchCallback)(char const *filename);
	/**
	 * xmlOutputOpenCallback:
	 * @filename: the filename or URI
	 *
	 * Callback used in the I/O Output API to open the resource
	 *
	 * Returns an Output context or NULL in case or error
	 */
	typedef void * (XMLCALL *xmlOutputOpenCallback)(char const *filename);
	/**
	 * xmlOutputWriteCallback:
	 * @context:  an Output context
	 * @buffer:  the buffer of data to write
	 * @len:  the length of the buffer in bytes
	 *
	 * Callback used in the I/O Output API to write to the resource
	 *
	 * Returns the number of bytes written or -1 in case of error
	 */
	typedef int (XMLCALL *xmlOutputWriteCallback)(void * context, const char * buffer, int len);
	/**
	 * xmlOutputCloseCallback:
	 * @context:  an Output context
	 *
	 * Callback used in the I/O Output API to close the resource
	 *
	 * Returns 0 or -1 in case of error
	 */
	typedef int (XMLCALL *xmlOutputCloseCallback)(void * context);
#endif

struct xmlParserInputBuffer {
    void * context;
    xmlInputReadCallback  readcallback;
    xmlInputCloseCallback closecallback;
    xmlCharEncodingHandler * encoder; // I18N conversions to UTF-8
    xmlBuf * buffer;  // Local buffer encoded in UTF-8
    xmlBuf * raw;     // if encoder != NULL buffer for raw input
    int    compressed; // -1=unknown, 0=not compressed, 1=compressed
    int    error;
    ulong  rawconsumed; // amount consumed from raw
};

#ifdef LIBXML_OUTPUT_ENABLED
	struct xmlOutputBuffer {
		void * context;
		xmlOutputWriteCallback writecallback;
		xmlOutputCloseCallback closecallback;
		xmlCharEncodingHandler * encoder; // I18N conversions to UTF-8
		xmlBuf * buffer; // Local buffer encoded in UTF-8 or ISOLatin
		xmlBuf * conv;   // if encoder != NULL buffer for output
		int    written;   // total number of byte written
		int    error;
	};
#endif
/*
 * Interfaces for input
 */
XMLPUBFUN void XMLCALL xmlCleanupInputCallbacks();
XMLPUBFUN int XMLCALL xmlPopInputCallbacks();
XMLPUBFUN void XMLCALL xmlRegisterDefaultInputCallbacks();
XMLPUBFUN xmlParserInputBuffer * XMLCALL xmlAllocParserInputBuffer(xmlCharEncoding enc);
XMLPUBFUN xmlParserInputBuffer * XMLCALL xmlParserInputBufferCreateFilename(const char *URI, xmlCharEncoding enc);
XMLPUBFUN xmlParserInputBuffer * XMLCALL xmlParserInputBufferCreateFile(FILE *file, xmlCharEncoding enc);
XMLPUBFUN xmlParserInputBuffer * XMLCALL xmlParserInputBufferCreateFd(int fd, xmlCharEncoding enc);
XMLPUBFUN xmlParserInputBuffer * XMLCALL xmlParserInputBufferCreateMem(const char *mem, int size, xmlCharEncoding enc);
XMLPUBFUN xmlParserInputBuffer * XMLCALL xmlParserInputBufferCreateStatic(const char *mem, int size, xmlCharEncoding enc);
XMLPUBFUN xmlParserInputBuffer * XMLCALL xmlParserInputBufferCreateIO(xmlInputReadCallback   ioread, xmlInputCloseCallback  ioclose, void *ioctx, xmlCharEncoding enc);
XMLPUBFUN int XMLCALL xmlParserInputBufferRead(xmlParserInputBuffer * in, int len);
XMLPUBFUN int XMLCALL xmlParserInputBufferGrow(xmlParserInputBuffer * in, int len);
XMLPUBFUN int XMLCALL xmlParserInputBufferPush(xmlParserInputBuffer * in, int len, const char *buf);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlFreeParserInputBuffer(xmlParserInputBuffer * pIn);
XMLPUBFUN char * XMLCALL xmlParserGetDirectory(const char *filename);
XMLPUBFUN int XMLCALL xmlRegisterInputCallbacks(xmlInputMatchCallback matchFunc, xmlInputOpenCallback openFunc, xmlInputReadCallback readFunc, xmlInputCloseCallback closeFunc);
xmlParserInputBuffer * __xmlParserInputBufferCreateFilename(const char *URI, xmlCharEncoding enc);

#ifdef LIBXML_OUTPUT_ENABLED
	// 
	// Interfaces for output
	// 
	XMLPUBFUN void XMLCALL xmlCleanupOutputCallbacks();
	XMLPUBFUN void XMLCALL xmlRegisterDefaultOutputCallbacks();
	XMLPUBFUN xmlOutputBuffer * XMLCALL xmlAllocOutputBuffer(xmlCharEncodingHandler * encoder);
	XMLPUBFUN xmlOutputBuffer * XMLCALL xmlOutputBufferCreateFilename(const char *URI, xmlCharEncodingHandler * encoder, int compression);
	XMLPUBFUN xmlOutputBuffer * XMLCALL xmlOutputBufferCreateFile(FILE *file, xmlCharEncodingHandler * encoder);
	XMLPUBFUN xmlOutputBuffer * XMLCALL xmlOutputBufferCreateBuffer(xmlBuffer * buffer, xmlCharEncodingHandler * encoder);
	XMLPUBFUN xmlOutputBuffer * XMLCALL xmlOutputBufferCreateFd(int fd, xmlCharEncodingHandler * encoder);
	XMLPUBFUN xmlOutputBuffer * XMLCALL xmlOutputBufferCreateIO(xmlOutputWriteCallback   iowrite, xmlOutputCloseCallback  ioclose, void *ioctx, xmlCharEncodingHandler * encoder);
	// Couple of APIs to get the output without digging into the buffers 
	XMLPUBFUN const xmlChar * XMLCALL xmlOutputBufferGetContent(xmlOutputBuffer * out);
	XMLPUBFUN size_t XMLCALL xmlOutputBufferGetSize(xmlOutputBuffer * out);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlOutputBufferWrite(xmlOutputBuffer * out, int len, const char *buf);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlOutputBufferWriteString(xmlOutputBuffer * out, const char *str);
	XMLPUBFUN int XMLCALL xmlOutputBufferWriteEscape(xmlOutputBuffer * out, const xmlChar *str, xmlCharEncodingOutputFunc escaping);
	XMLPUBFUN int XMLCALL xmlOutputBufferFlush(xmlOutputBuffer * out);
	XMLPUBFUN int /*XMLCALL*/FASTCALL xmlOutputBufferClose(xmlOutputBuffer * out);
	XMLPUBFUN int XMLCALL xmlRegisterOutputCallbacks(xmlOutputMatchCallback matchFunc, xmlOutputOpenCallback openFunc, xmlOutputWriteCallback writeFunc, xmlOutputCloseCallback closeFunc);
	xmlOutputBuffer * __xmlOutputBufferCreateFilename(const char *URI, xmlCharEncodingHandler * encoder, int compression);
	#ifdef LIBXML_HTTP_ENABLED
		// This function only exists if HTTP support built into the library  
		XMLPUBFUN void XMLCALL xmlRegisterHTTPPostCallbacks	(void );
	#endif
#endif
XMLPUBFUN xmlParserInput * XMLCALL xmlCheckHTTPInput(xmlParserCtxt * ctxt, xmlParserInput * ret);
/*
 * A predefined entity loader disabling network accesses
 */
XMLPUBFUN xmlParserInput * XMLCALL xmlNoNetExternalEntityLoader(const char *URL, const char *ID, xmlParserCtxt * ctxt);
/*
 * xmlNormalizeWindowsPath is obsolete, don't use it.
 * Check xmlCanonicPath in uri.h for a better alternative.
 */
XMLPUBFUN xmlChar * XMLCALL xmlNormalizeWindowsPath(const xmlChar *path);
XMLPUBFUN int XMLCALL xmlCheckFilename(const char *path);
/**
 * Default 'file://' protocol callbacks
 */
XMLPUBFUN int XMLCALL xmlFileMatch(const char *filename);
XMLPUBFUN void * XMLCALL xmlFileOpen(const char *filename);
XMLPUBFUN int XMLCALL xmlFileRead(void * context, char * buffer, int len);
XMLPUBFUN int XMLCALL xmlFileClose(void * context);
/**
 * Default 'http://' protocol callbacks
 */
#ifdef LIBXML_HTTP_ENABLED
	XMLPUBFUN int XMLCALL xmlIOHTTPMatch(const char *filename);
	XMLPUBFUN void * XMLCALL xmlIOHTTPOpen(const char *filename);
	#ifdef LIBXML_OUTPUT_ENABLED
		XMLPUBFUN void * XMLCALL xmlIOHTTPOpenW(const char * post_uri, int compression );
	#endif /* LIBXML_OUTPUT_ENABLED */
	XMLPUBFUN int XMLCALL xmlIOHTTPRead(void * context, char * buffer, int len);
	XMLPUBFUN int XMLCALL xmlIOHTTPClose(void * context);
#endif
/**
 * Default 'ftp://' protocol callbacks
 */
#ifdef LIBXML_FTP_ENABLED
	XMLPUBFUN int XMLCALL xmlIOFTPMatch(const char *filename);
	XMLPUBFUN void * XMLCALL xmlIOFTPOpen(const char *filename);
	XMLPUBFUN int XMLCALL xmlIOFTPRead(void * context, char * buffer, int len);
	XMLPUBFUN int XMLCALL xmlIOFTPClose(void * context);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __XML_IO_H__ */
