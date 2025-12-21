// XMLIO.H
// Summary: interface for the I/O interfaces used by the parser
// Description: interface for the I/O interfaces used by the parser
// Copy: See Copyright for the status of this software.
// Author: Daniel Veillard
// 
#ifndef __XML_IO_H__
#define __XML_IO_H__

struct xmlBuffer;
struct xmlBuf;
struct xmlParserCtxt;
struct xmlCharEncodingHandler;
struct xmlParserInput;

//#ifdef __cplusplus
//extern "C" {
//#endif
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
typedef int (* xmlInputMatchCallback)(char const *filename);
/**
 * xmlInputOpenCallback:
 * @filename: the filename or URI
 *
 * Callback used in the I/O Input API to open the resource
 *
 * Returns an Input context or NULL in case or error
 */
typedef void * (* xmlInputOpenCallback)(char const *filename);
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
typedef int (* xmlInputReadCallback)(void * context, char * buffer, int len);
/**
 * xmlInputCloseCallback:
 * @context:  an Input context
 *
 * Callback used in the I/O Input API to close the resource
 *
 * Returns 0 or -1 in case of error
 */
typedef int (* xmlInputCloseCallback)(void * context);

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
	typedef int (* xmlOutputMatchCallback)(char const *filename);
	/**
	 * xmlOutputOpenCallback:
	 * @filename: the filename or URI
	 *
	 * Callback used in the I/O Output API to open the resource
	 *
	 * Returns an Output context or NULL in case or error
	 */
	typedef void * (* xmlOutputOpenCallback)(char const *filename);
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
	typedef int (* xmlOutputWriteCallback)(void * context, const char * buffer, int len);
	/**
	 * xmlOutputCloseCallback:
	 * @context:  an Output context
	 *
	 * Callback used in the I/O Output API to close the resource
	 *
	 * Returns 0 or -1 in case of error
	 */
	typedef int (* xmlOutputCloseCallback)(void * context);
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
void xmlCleanupInputCallbacks();
int xmlPopInputCallbacks();
void xmlRegisterDefaultInputCallbacks();
xmlParserInputBuffer * xmlAllocParserInputBuffer(xmlCharEncoding enc);
xmlParserInputBuffer * xmlParserInputBufferCreateFilename(const char *URI, xmlCharEncoding enc);
xmlParserInputBuffer * xmlParserInputBufferCreateFile(FILE *file, xmlCharEncoding enc);
xmlParserInputBuffer * xmlParserInputBufferCreateFd(int fd, xmlCharEncoding enc);
xmlParserInputBuffer * xmlParserInputBufferCreateMem(const char * pMem, ssize_t size, xmlCharEncoding enc); // @v12.5.1 (int size)-->(ssize_t size)
xmlParserInputBuffer * xmlParserInputBufferCreateStatic(const char * pMem, ssize_t size, xmlCharEncoding enc); // @v12.5.1 (int size)-->(ssize_t size)
xmlParserInputBuffer * xmlParserInputBufferCreateIO(xmlInputReadCallback   ioread, xmlInputCloseCallback  ioclose, void *ioctx, xmlCharEncoding enc);
int xmlParserInputBufferRead(xmlParserInputBuffer * in, int len);
int xmlParserInputBufferGrow(xmlParserInputBuffer * in, int len);
int xmlParserInputBufferPush(xmlParserInputBuffer * in, int len, const char *buf);
void FASTCALL xmlFreeParserInputBuffer(xmlParserInputBuffer * pIn);
char * xmlParserGetDirectory(const char *filename);
int xmlRegisterInputCallbacks(xmlInputMatchCallback matchFunc, xmlInputOpenCallback openFunc, xmlInputReadCallback readFunc, xmlInputCloseCallback closeFunc);
xmlParserInputBuffer * __xmlParserInputBufferCreateFilename(const char *URI, xmlCharEncoding enc);

#ifdef LIBXML_OUTPUT_ENABLED
	// 
	// Interfaces for output
	// 
	void xmlCleanupOutputCallbacks();
	void xmlRegisterDefaultOutputCallbacks();
	xmlOutputBuffer * xmlAllocOutputBuffer(xmlCharEncodingHandler * encoder);
	xmlOutputBuffer * xmlOutputBufferCreateFilename(const char *URI, xmlCharEncodingHandler * encoder, int compression);
	xmlOutputBuffer * xmlOutputBufferCreateFile(FILE *file, xmlCharEncodingHandler * encoder);
	xmlOutputBuffer * xmlOutputBufferCreateBuffer(xmlBuffer * buffer, xmlCharEncodingHandler * encoder);
	xmlOutputBuffer * xmlOutputBufferCreateFd(int fd, xmlCharEncodingHandler * encoder);
	xmlOutputBuffer * xmlOutputBufferCreateIO(xmlOutputWriteCallback   iowrite, xmlOutputCloseCallback  ioclose, void *ioctx, xmlCharEncodingHandler * encoder);
	// Couple of APIs to get the output without digging into the buffers 
	const xmlChar * xmlOutputBufferGetContent(xmlOutputBuffer * out);
	size_t xmlOutputBufferGetSize(xmlOutputBuffer * out);
	int STDCALL xmlOutputBufferWrite(xmlOutputBuffer * out, int len, const char * buf);
	int FASTCALL xmlOutputBufferWriteString(xmlOutputBuffer * out, const char * str);
	int xmlOutputBufferWriteEscape(xmlOutputBuffer * out, const xmlChar *str, xmlCharEncodingOutputFunc escaping);
	int xmlOutputBufferFlush(xmlOutputBuffer * out);
	int FASTCALL xmlOutputBufferClose(xmlOutputBuffer * out);
	int xmlRegisterOutputCallbacks(xmlOutputMatchCallback matchFunc, xmlOutputOpenCallback openFunc, xmlOutputWriteCallback writeFunc, xmlOutputCloseCallback closeFunc);
	xmlOutputBuffer * __xmlOutputBufferCreateFilename(const char *URI, xmlCharEncodingHandler * encoder, int compression);
	#ifdef LIBXML_HTTP_ENABLED
		// This function only exists if HTTP support built into the library  
		void xmlRegisterHTTPPostCallbacks	(void );
	#endif
#endif
xmlParserInput * xmlCheckHTTPInput(xmlParserCtxt * ctxt, xmlParserInput * ret);
// 
// A predefined entity loader disabling network accesses
// 
xmlParserInput * xmlNoNetExternalEntityLoader(const char *URL, const char *ID, xmlParserCtxt * ctxt);
// 
// xmlNormalizeWindowsPath is obsolete, don't use it.
// Check xmlCanonicPath in uri.h for a better alternative.
// 
xmlChar * xmlNormalizeWindowsPath(const xmlChar *path);
int xmlCheckFilename(const char *path);
// 
// Default 'file://' protocol callbacks
// 
int xmlFileMatch(const char *filename);
void * xmlFileOpen(const char *filename);
int xmlFileRead(void * context, char * buffer, int len);
int xmlFileClose(void * context);
// 
// Default 'http://' protocol callbacks
// 
#ifdef LIBXML_HTTP_ENABLED
	int xmlIOHTTPMatch(const char *filename);
	void * xmlIOHTTPOpen(const char *filename);
	#ifdef LIBXML_OUTPUT_ENABLED
		void * xmlIOHTTPOpenW(const char * post_uri, int compression );
	#endif /* LIBXML_OUTPUT_ENABLED */
	int xmlIOHTTPRead(void * context, char * buffer, int len);
	int xmlIOHTTPClose(void * context);
#endif
// 
// Default 'ftp://' protocol callbacks
// 
#ifdef LIBXML_FTP_ENABLED
	int xmlIOFTPMatch(const char *filename);
	void * xmlIOFTPOpen(const char *filename);
	int xmlIOFTPRead(void * context, char * buffer, int len);
	int xmlIOFTPClose(void * context);
#endif
//#ifdef __cplusplus
//}
//#endif
#endif /* __XML_IO_H__ */
