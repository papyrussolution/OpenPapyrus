// NANOHTTP.H
//
#ifndef __NANOHTTP_H // {
#define __NANOHTTP_H

#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#ifdef HAVE_SSL // {
 #ifdef HAVE_OPENSSL_SSL_H
  #include <openssl/ssl.h>
 #endif
#endif // } HAVE_SSL
#ifdef WIN32 // {
 #include <winsock2.h>
#else // }{
 #include <sys/socket.h>
 #include <netinet/in.h>
#endif // } WIN32

#define HEADER_CONTENT_ID               "Content-Id"
#define HEADER_CONTENT_TRANSFER_ENCODING "Content-Transfer-Encoding"
#define TRANSFER_ENCODING_CHUNKED       "chunked"
/**
 *
 * General Header Fields
 *
 * There are a few header fields which have general applicability for both
 * request and response messages, but which do not apply to the entity being
 * transferred. These header fields apply only to the message being transmitted.
 * (see RFC2616)
 *
 */
#define HEADER_CACHE_CONTROL            "Cache-Control"
#define HEADER_CONNECTION               "Connection"
#define HEADER_DATE                     "Date"
#define HEADER_PRAGMA                   "Pragma"
#define HEADER_TRAILER                  "Trailer"
#define HEADER_TRANSFER_ENCODING        "Transfer-Encoding"
#define HEADER_UPGRADE                  "Upgrade"
#define HEADER_VIA                      "Via"
#define HEADER_WARNING                  "Warning"
/**
 *
 * Entity Header Fields
 *
 * Entity-header fields define metainformation about the entity-body or, if no
 * body is present, about the resource identified by the request. Some of this
 * metainformation is OPTIONAL; some might be REQUIRED by portions of this
 * specification. (see RFC2616 7.1)
 *
 */
#define HEADER_ALLOW                    "Allow"
#define HEADER_CONTENT_ENCODING         "Content-Encoding"
#define HEADER_CONTENT_LANGUAGE         "Content-Language"
#define HEADER_CONTENT_LENGTH           "Content-Length"
#define HEADER_CONTENT_LOCATION         "Content-Location"
#define HEADER_CONTENT_MD5              "Content-MD5"
#define HEADER_CONTENT_RANGE            "Content-Range"
#define HEADER_CONTENT_TYPE             "Content-Type"
#define HEADER_EXPIRES                  "Expires"
#define HEADER_LAST_MODIFIED            "Last-Modified"
/**
 *
 * XXX: move to nanohttp-response.h
 *
 * Response Header Fields
 *
 * The response-header fields allow the server to pass additional information
 * about the response which cannot be placed in the Status-Line. These header
 * fields give information about the server and about further access to the
 * resource identified by the Request-URI. (see RFC2616)
 *
 */
#define HEADER_ACCEPT_RANGES            "Accept-Ranges"
#define HEADER_AGE                      "Age"
#define HEADER_EXTENSION_TAG            "ETag"
#define HEADER_LOCATION                 "Location"
#define HEADER_PROXY_AUTHENTICATE       "Proxy-Authenticate"
#define HEADER_RETRY_AFTER              "Retry-After"
#define HEADER_SERVER                   "Server"
#define HEADER_VARY                     "Vary"
#define HEADER_WWW_AUTHENTICATE         "WWW-Authenticate"

/**
 *
 * XXX: move to nanohttp-request.h
 *
 * Request Header Fields
 *
 * The request-header fields allow the client to pass additional information
 * about the request, and about the client itself, to the server. These fields
 * act as request modifiers, with semantics equivalent to the parameters on a
 * programming language method invocation (see RFC2616).
 *
 */
#define HEADER_ACCEPT                   "Accept"
#define HEADER_CHARSET                  "Accept-Charset"
#define HEADER_ACCEPT_ENCODING          "Accept-Encoding"
#define HEADER_ACCEPT_LANGUAGE          "Accept-Language"
#define HEADER_AUTHORIZATION            "Authorization"
#define HEADER_EXPECT                   "Expect"
#define HEADER_FROM                     "From"
#define HEADER_HOST                     "Host"
#define HEADER_IF_MATCH                 "If-Match"
#define HEADER_IF_MODIFIED_SINCE        "If-Modified-Since"
#define HEADER_IF_NONE_MATCH            "If-None-Match"
#define HEADER_IF_RANGE                 "If-Range"
#define HEADER_IF_UNMODIFIED_SINCE      "If-Unmodified-Since"
#define HEADER_IF_MAX_FORWARDS          "Max-Forwards"
#define HEADER_PROXY_AUTHORIZATION      "Proxy-Authorization"
#define HEADER_RANGE                    "Range"
#define HEADER_REFERER                  "Referer"
#define HEADER_TRANSFER_EXTENSION       "TE"
#define HEADER_USER_AGENT               "User-Agent"

/**
 *
 * nanohttp command line flags
 *
 */
#define NHTTPD_ARG_PORT         "-NHTTPport"
#define NHTTPD_ARG_TERMSIG      "-NHTTPtsig"
#define NHTTPD_ARG_MAXCONN      "-NHTTPmaxconn"
#define NHTTPD_ARG_TIMEOUT      "-NHTTPtimeout"

#define NHTTP_ARG_CERT          "-NHTTPcert"
#define NHTTP_ARG_CERTPASS      "-NHTTPcertpass"
#define NHTTP_ARG_CA            "-NHTTPCA"
#define NHTTP_ARG_HTTPS         "-NHTTPS"

#ifndef SAVE_STR
 #define SAVE_STR(str) ((str==0) ? ("(null)") : (str))
#endif

#define BOUNDARY_LENGTH 18
#define MAX_HEADER_SIZE 4256
#define MAX_SOCKET_BUFFER_SIZE 4256
#define MAX_FILE_BUFFER_SIZE 4256
#define URL_MAX_HOST_SIZE      120
#define URL_MAX_CONTEXT_SIZE  1024
#define HSOCKET_MAX_BUFSIZE 1024
#define REQUEST_MAX_PATH_SIZE 1024
#define RESPONSE_MAX_DESC_SIZE 1024
/*
   DIME common stuff
 */
#define DIME_VERSION_1      0x08
#define DIME_FIRST_PACKAGE  0x04
#define DIME_LAST_PACKAGE   0x02
#define DIME_CHUNKED        0x01
#define DIME_TYPE_URI       0x2
/* TODO (#1#): find proper ports */
#define URL_DEFAULT_PORT_HTTP 80
#define URL_DEFAULT_PORT_HTTPS 81
#define URL_DEFAULT_PORT_FTP 120
#define H_OK 0 // Success flag
/*
        File errors
 */
#define FILE_ERROR_OPEN 8000
#define FILE_ERROR_READ 8001
/*
        Socket errors
 */
#define HSOCKET_ERROR_CREATE            1001
#define HSOCKET_ERROR_GET_HOSTNAME      1002
#define HSOCKET_ERROR_CONNECT           1003
#define HSOCKET_ERROR_SEND              1004
#define HSOCKET_ERROR_RECEIVE           1005
#define HSOCKET_ERROR_BIND              1006
#define HSOCKET_ERROR_LISTEN            1007
#define HSOCKET_ERROR_ACCEPT            1008
#define HSOCKET_ERROR_NOT_INITIALIZED   1009
#define HSOCKET_ERROR_IOCTL             1010
#define HSOCKET_ERROR_SSLCLOSE          1011
#define HSOCKET_ERROR_SSLCTX            1011
/*
        URL errors
 */
#define URL_ERROR_UNKNOWN_PROTOCOL      1101
#define URL_ERROR_NO_PROTOCOL           1102
#define URL_ERROR_NO_HOST               1103
/*
        Stream errors
 */
#define STREAM_ERROR_INVALID_TYPE       1201
#define STREAM_ERROR_SOCKET_ERROR       1202
#define STREAM_ERROR_NO_CHUNK_SIZE      1203
#define STREAM_ERROR_WRONG_CHUNK_SIZE   1204
/*
        MIME errors
 */
#define MIME_ERROR_NO_BOUNDARY_PARAM    1301
#define MIME_ERROR_NO_START_PARAM       1302
#define MIME_ERROR_PARSE_ERROR          1303
#define MIME_ERROR_NO_ROOT_PART         1304
#define MIME_ERROR_NOT_MIME_MESSAGE     1305
/*
        General errors
 */
#define GENERAL_INVALID_PARAM           1400
#define GENERAL_HEADER_PARSE_ERROR      1401
/*
        Thread errors
 */
#define THREAD_BEGIN_ERROR              1500
/*
        XML Errors
 */
#define XML_ERROR_EMPTY_DOCUMENT        1600
#define XML_ERROR_PARSE                 1601
/*
        SSL Errors
 */
#define HSSL_ERROR_CA_LIST              1710
#define HSSL_ERROR_CONTEXT              1720
#define HSSL_ERROR_CERTIFICATE          1730
#define HSSL_ERROR_PEM                  1740
#define HSSL_ERROR_CLIENT               1750
#define HSSL_ERROR_SERVER               1760
#define HSSL_ERROR_CONNECT              1770
//
//
//
typedef unsigned char byte_t;
typedef void * herror_t;
/*
   Set Sleep function platform depended
 */
#ifdef WIN32
 #define system_sleep(seconds) Sleep(seconds*1000);
#else
 #define system_sleep(seconds) sleep(seconds);
#endif
/**
   Indicates the version of the
   used HTTP protocol.
 */
typedef enum _http_version {
	HTTP_1_0,
	HTTP_1_1                /* default */
} http_version_t;
/**
   Indicates the used method
 */
typedef enum _hreq_method {
	HTTP_REQUEST_POST,
	HTTP_REQUEST_GET,
	HTTP_REQUEST_OPTIONS,
	HTTP_REQUEST_HEAD,
	HTTP_REQUEST_PUT,
	HTTP_REQUEST_DELETE,
	HTTP_REQUEST_TRACE,
	HTTP_REQUEST_CONNECT,
	HTTP_REQUEST_UNKOWN
} hreq_method_t;
/*
   hpairnode_t represents a pair (key, value) pair.
   This is also a linked list.
 */
typedef struct hpair hpair_t;

struct hpair {
	char * key;
	char * value;
	hpair_t * next;
};

#define HSOCKET_FREE  -1
/*
   Socket definition
 */
typedef struct hsocket_t {
#ifdef WIN32
	SOCKET sock;
#else
	int sock;
#endif
	struct sockaddr_in addr;
	void * ssl;
} hsocket_t;
/**
   The protocol types in enumeration
   format. Used in some other nanohttp objects
   like hurl_t.

   @see hurl_t
*/
typedef enum _hprotocol {
	PROTOCOL_HTTP,
	PROTOCOL_HTTPS,
	PROTOCOL_FTP
} hprotocol_t;
/**
   The URL object. A representation
   of an URL like:<P>

   [protocol]://[host]:[port]/[context]

 */
typedef struct _hurl {
	/**
	   The transfer protocol.
	   Note that only PROTOCOL_HTTP is supported by nanohttp.
	 */
	hprotocol_t protocol;
	/**
	   The port number. If no port number was given in the URL,
	   one of the default port numbers will be selected.
	   URL_HTTP_DEFAULT_PORT
	   URL_HTTPS_DEFAULT_PORT
	   URL_FTP_DEFAULT_PORT
	 */
	int port;
	/** The hostname */
	char host[URL_MAX_HOST_SIZE];
	/** The string after the hostname. */
	char context[URL_MAX_CONTEXT_SIZE];
} hurl_t;
/*
   HTTP Stream modul:

   nanohttp supports 2 different streams:

   1. http_input_stream_t
   2. http_output_stream_t

   These are not regular streams. They will care about
   transfer styles while sending/receiving data.

   Supported transfer styles are

   o Content-length
   o Chunked encoding
   o Connection: "close"

   A stream will set its transfer style from the header
   information, which must be given while creating a stream.

   A stream will start sending/receiving data "after"
   sending/receiving header information. (After <CF><CF>)"

 */
/**
   Transfer types supported while
   sending/receiving data.
*/
typedef enum http_transfer_type {
	/** The stream cares about Content-length */
	HTTP_TRANSFER_CONTENT_LENGTH,
	/** The stream sends/receives chunked data */
	HTTP_TRANSFER_CHUNKED,
	/** The stream sends/receives data until
	   connection is closed */
	HTTP_TRANSFER_CONNECTION_CLOSE,
	/** This transfer style will be used by MIME support
	   and for debug purposes.*/
	HTTP_TRANSFER_FILE
} http_transfer_type_t;
/**
   HTTP INPUT STREAM. Receives data from a socket/file
   and cares about the transfer style.
*/
typedef struct http_input_stream {
	hsocket_t * sock;
	herror_t err;
	http_transfer_type_t type;
	int received;
	int content_length;
	int chunk_size;
	byte_t connection_closed;

	/* file handling */
	FILE * fd;
	char filename[255];
	int deleteOnExit;       /* default is 0 */
} http_input_stream_t;
//
// HTTP OUTPUT STREAM. Sends data to a socket
// and cares about the transfer style.
//
typedef struct http_output_stream {
	hsocket_t * sock;
	http_transfer_type_t type;
	int content_length;
	int sent;
} http_output_stream_t;

typedef struct httpc_conn {
	hsocket_t sock;
	hpair_t * header;
	hurl_t url;
	http_version_t version;
	/*
	   -1 : last dime package 0 : no dime connection >0 : dime package number */
	int _dime_package_nr;
	long _dime_sent_bytes;
	int errcode;
	char errmsg[150];
	http_output_stream_t * out;
	int id;                 /* uniq id */
} httpc_conn_t;

#define NHTTP_ARG_LOGFILE       "-NHTTPlog"
#define NHTTP_ARG_LOGLEVEL      "-NHTTPloglevel"
/*
        logging stuff
 */
typedef enum log_level {
	HLOG_VERBOSE,
	HLOG_DEBUG,
	HLOG_INFO,
	HLOG_WARN,
	HLOG_ERROR,
	HLOG_FATAL
} log_level_t;
/*
   Object representation of the content-type field
   in a HTTP header:
   <P>
   Example:<P>

   text/xml; key="value" key2="value2' ...
 */
typedef struct _content_type {
	char type[128];
	hpair_t * params;
} content_type_t;
//
// Part of attachment
//
typedef struct _part {
	char id[250];
	char location[250];
	hpair_t * header;
	char content_type[128];
	char transfer_encoding[128];
	char filename[250];
	struct _part * next;
	int deleteOnExit;       /* default is 0 */
} part_t;
//
// Attachments
//
typedef struct _attachments {
	part_t * parts;
	part_t * last;
	part_t * root_part;
} attachments_t;
//
// Request object
//
typedef struct hrequest {
	hreq_method_t method;
	http_version_t version;
	char path[REQUEST_MAX_PATH_SIZE];
	hpair_t * query;
	hpair_t * header;
	http_input_stream_t * in;
	content_type_t * content_type;
	attachments_t * attachments;
	char root_part_id[150];
} hrequest_t;
/*
        response object
 */
typedef struct hresponse {
	http_version_t version;
	int errcode;
	char desc[RESPONSE_MAX_DESC_SIZE];
	hpair_t * header;
	http_input_stream_t * in;
	content_type_t * content_type;
	attachments_t * attachments;
	char root_part_id[150];
} hresponse_t;

typedef struct httpd_conn {
	hsocket_t * sock;
	char content_type[25];
	http_output_stream_t * out;
	hpair_t * header;
} httpd_conn_t;
/*
   Service callback
 */
typedef void (*httpd_service)(httpd_conn_t *, hrequest_t *);
typedef int (*httpd_auth)(hrequest_t * req, const char * user, const char * password);
/*
        Service representation object
 */
typedef struct tag_hservice {
	char ctx[255];
	httpd_service func;
	httpd_auth auth;
	struct tag_hservice * next;
} hservice_t;

#ifdef __cplusplus
extern "C" {
#endif
/**
 *
 * Base64 encodes a NUL terminated array of characters.
 *
 * @param instr		Pointer to the input string.
 * @param outstr	Pointer to the output destination.
 *
 * @see base64_decode
 *
 */
extern void base64_encode(const unsigned char * instr, unsigned char * outstr);
/**
 *
 * Base64 decodes a NUL terminated array of characters.
 *
 * @param instr		Pointer to the input string.
 * @param outstr	Pointer to the output destination.
 *
 * @see base64_encode
 *
 */
extern void base64_decode(const unsigned char * instr, unsigned char * outstr);
/* --------------------------------------------------------------
   HTTP CLIENT MODULE RELATED FUNCTIONS
   ---------------------------------------------------------------*/
/**
 *
 * Initializes the httpc_* module. This is called from
 * soap_client_init_args().
 *
 * @param argc		Argument count.
 * @param argv		Argument vector.
 *
 * @return H_OK on succes or a herror_t struct on failure.
 *
 * @see httpc_destroy, herror_t, soap_client_init_args
 */
herror_t httpc_init(int argc, char * argv[]);

/**
 *
 * Destroys the httpc_* module
 *
 * @see httpc_init
 *
 */
void httpc_destroy(void);

/**
 *
 * Creates a new connection.
 *
 */
httpc_conn_t * httpc_new(void);

/**
 *
 * Release a connection
 * (use httpc_close_free() instead)
 *
 * @see httpc_close_free
 *
 */
void httpc_free(httpc_conn_t * conn);

/**
 *
 * Close and release a connection
 *
 * @see httpc_close, httpc_free
 *
 */
void httpc_close_free(httpc_conn_t * conn);

/**
 *
 * Sets header element (key,value) pair.
 *
 * @return 0 on success or failure (yeah!), 1 if a (key,value) pair was replaced.
 *
 * @see httpc_add_header, httpc_add_headers
 *
 */
int httpc_set_header(httpc_conn_t * conn, const char * key, const char * value);

/**
 *
 * Adds a header element (key, value) pair.
 *
 * @return 0 on success, -1 on failure.
 *
 * @see httpc_set_header, httpc_add_headers
 *
 */
int httpc_add_header(httpc_conn_t * conn, const char * key, const char * value);

/**
 *
 * Adds a list of (key, value) pairs.
 *
 * @see httpc_set_header, httpc_add_header
 *
 */
void httpc_add_headers(httpc_conn_t * conn, const hpair_t * values);

/**
 *
 * Sets a HEADER_AUTHORIZATION header.
 *
 * @param conn		The HTTP connection.
 * @param user		The username.
 * @param password	The password.
 *
 * @see httpc_set_header, HEADER_AUTHORIZATION
 *
 */
int httpc_set_basic_authorization(httpc_conn_t * conn, const char * user, const char * password);

/**
 *
 * Sets a HEADER_PROXY_AUTHORIZATION header.
 *
 * @param conn		The HTTP connection.
 * @param user		The username.
 * @param password	The password.
 *
 * @see httpc_set_header, HEADER_PROXY_AUTHORIZATION
 */
int httpc_set_basic_proxy_authorization(httpc_conn_t * conn, const char * user, const char * password);

/**
   Invoke a "GET" method request and receive the response
 */
herror_t httpc_get(httpc_conn_t * conn, hresponse_t ** out, const char * urlstr);

/**
   Start a "POST" method request
   Returns: HSOCKET_OK  or error flag
 */
herror_t httpc_post_begin(httpc_conn_t * conn, const char * url);

/**
   End a "POST" method and receive the response.
   You MUST call httpc_post_end() before!
 */
herror_t httpc_post_end(httpc_conn_t * conn, hresponse_t ** out);

/* --------------------------------------------------------------
   DIME RELATED FUNCTIONS
   ---------------------------------------------------------------*/

/*
   DIME support httpc_dime_* function set
 */
/*
   int httpc_dime_begin(httpc_conn_t *conn, const char *url);
   int httpc_dime_next(httpc_conn_t* conn, long content_length,
                    const char *content_type, const char *id,
                    const char *dime_options, int last);
   hresponse_t* httpc_dime_end(httpc_conn_t *conn);
 */

/* --------------------------------------------------------------
   MIME RELATED FUNCTIONS
   ---------------------------------------------------------------*/
/*
   MIME support httpc_mime_* function set
 */

/**
   Begin MIME multipart/related POST request
   Returns: HSOCKET_OK  or error flag
 */
herror_t httpc_mime_begin(httpc_conn_t * conn, const char * url, const char * related_start,
	const char * related_start_info,
	const char * related_type);

/**
   Send boundary and part header and continue
   with next part
 */
herror_t httpc_mime_next(httpc_conn_t * conn, const char * content_id, const char * content_type,
	const char * transfer_encoding);

/**
   Finish MIME request and get the response
 */
herror_t httpc_mime_end(httpc_conn_t * conn, hresponse_t ** out);

/**
   Send boundary and part header and continue
   with next part
 */

herror_t httpc_mime_send_file(httpc_conn_t * conn, const char * content_id, const char * content_type,
	const char * transfer_encoding,
	const char * filename);

#ifdef WIN32
 #include <string.h>
char * strtok_r(char * s, const char * delim, char ** save_ptr);
struct tm * localtime_r(const time_t * const timep, struct tm * p_tm);
#endif

herror_t herror_new(const char * func, int errcode, const char * format, ...);
int herror_code(herror_t err);
char * herror_func(herror_t err);
char * herror_message(herror_t err);
void herror_release(herror_t err);

/**
   Creates a new pair with the given parameters. Both strings
   key and value will be cloned while creating the pair.

   @param key the key of the (key,value) pair
   @param value the value of the (key,value) pair
   @param next next pair node in the linked list

   @returns A newly crated hpair_t object. Use hpair_free()
    or hpair_free_deep() to free the pair.
 */
hpair_t * hpairnode_new(const char * key, const char * value, hpair_t * next);

/**
   Creates a new pair from a given string. This function
   will split 'str' with the found first delimiter 'delim'.
   The 'value' field  of the newly created pair will have
   the value "", if no delimiter was found/
   Whitespaces (' ') will be removed from the beginnig of
   the parsed value.

   @param str A string to parse
   @param delim a delimiter to use while splitting into key,value
   @param next next pair node in the linked list

   @returns A newly crated hpair_t object. Use hpair_free()
    or hpair_free_deep() to free the pair.
 */
hpair_t * hpairnode_parse(const char * str, const char * delim, hpair_t * next);

/**
   Frees a given pair.

   @param pair the pair to free
 */
void hpairnode_free(hpair_t * pair);

/**
   Makes a deep free operation. All pairnodes,
   beginning with the given pari, in the
   linked list will be destroyed.

   @param pair the pair to start to free the linked list
 */
void hpairnode_free_deep(hpair_t * pair);

/**
   Returns the (key,value) pair, which key is the
   given 'key'.

   @param pair the first pair to start to search from.
   @param key key to find the in the pair.
   @returns if a value will be found, this function will
    return the value (do not free this string) or NULL
    if no pair was found with the key 'key'.
 */
char * hpairnode_get(hpair_t * pair, const char * key);

/**
   Returns the (key,value) pair, which key is the
   given 'key'. The case will be ignored while
   comparing the key strings.

   @param pair the first pair to start to search from.
   @param key key to find the in the pair.
   @returns if a value will be found, this function will
    return the value (do not free this string) or NULL
    if no pair was found with the key 'key'.
 */
char * hpairnode_get_ignore_case(hpair_t * pair, const char * key);

/**
   This function will create a new pair and fills the
   (key,value) fields of a given pair. Note that the 'next'
   field will not be copied.

   @param src the source pair object to copy.

   @returns a newly created pair with the same (key,value)
   pairs as in 'src'. This fields will be cloned. The'next'
   field will be set to NULL.

   @see hpairnode_copy_deep
 */
hpair_t * hpairnode_copy(const hpair_t * src);

/**
   Clones the hole linked list.

   @param src the source pair object to copy from

   @returns the first object in the linked list.

   @see hpairnode_copy
 */
hpair_t * hpairnode_copy_deep(const hpair_t * src);

/* Debug functions */
void hpairnode_dump_deep(hpair_t * pair);
void hpairnode_dump(hpair_t * pair);
/**
   Parses the given 'urlstr' and fills the given hurl_t object.

   @param obj the destination URL object to fill
   @param url the URL in string format

   @returns H_OK on success or one of the following otherwise

    URL_ERROR_UNKNOWN_PROTOCOL
    URL_ERROR_NO_PROTOCOL
    URL_ERROR_NO_HOST
 */
herror_t hurl_parse(hurl_t * obj, const char * url);
/**
   Parses the given string and creates a new ccontent_type_t
   object.

   @param content_type_str the string representation of the
   content-type field in a HTTP header.

   @returns A newly created content_type_t object. Free this
    object with content_type_free();

   @see content_type_free
 */
content_type_t * content_type_new(const char * content_type_str);
/**
   Frees the given content_type_t object
 */
void content_type_free(content_type_t * ct);

part_t * part_new(const char * id, const char * filename, const char * content_type, const char * transfer_encoding, part_t * next);
void part_free(part_t * part);

attachments_t * attachments_new();       /* should be used internally */

/*
   Free a attachment. Create attachments with MIME
   and DIME (DIME is not supported yet).

   @see mime_get_attachments
 */
void attachments_free(attachments_t * message);
void attachments_add_part(attachments_t * attachments, part_t * part);

extern log_level_t hlog_set_level(log_level_t level);
extern log_level_t hlog_get_level(void);

extern void hlog_set_file(const char * filename);
extern char * hlog_get_file();

#ifdef WIN32
 #if defined(_MSC_VER) && _MSC_VER <= 1200
char * VisualC_funcname(const char * file, int line);                     /* not thread safe! */
  #define __FUNCTION__  VisualC_funcname(__FILE__, __LINE__)
 #endif
#endif

extern void hlog_verbose(const char * FUNC, const char * format, ...);
extern void hlog_debug(const char * FUNC, const char * format, ...);
extern void hlog_info(const char * FUNC, const char * format, ...);
extern void hlog_warn(const char * FUNC, const char * format, ...);
extern void hlog_error(const char * FUNC, const char * format, ...);
/*
 * XXX: This isn't the "right" way
 *
 * #define log_debug(fmt, ...)	fprintf(stderr, "EMERGENCY: %s: " fmt "\n", \
 *                                              __FUNCTION__, ## __VA_ARGS__)
 *
 */
#define log_verbose1(a1) hlog_verbose(__FUNCTION__, a1)
#define log_verbose2(a1, a2) hlog_verbose(__FUNCTION__, a1, a2)
#define log_verbose3(a1, a2, a3) hlog_verbose(__FUNCTION__, a1, a2, a3)
#define log_verbose4(a1, a2, a3, a4) hlog_verbose(__FUNCTION__, a1, a2, a3, a4)
#define log_verbose5(a1, a2, a3, a4, a5) hlog_verbose(__FUNCTION__, a1, a2, a3, a4, a5)

#define log_debug1(a1) hlog_debug(__FUNCTION__, a1)
#define log_debug2(a1, a2) hlog_debug(__FUNCTION__, a1, a2)
#define log_debug3(a1, a2, a3) hlog_debug(__FUNCTION__, a1, a2, a3)
#define log_debug4(a1, a2, a3, a4) hlog_debug(__FUNCTION__, a1, a2, a3, a4)
#define log_debug5(a1, a2, a3, a4, a5) hlog_debug(__FUNCTION__, a1, a2, a3, a4, a5)

#define log_info1(a1) hlog_info(__FUNCTION__, a1)
#define log_info2(a1, a2) hlog_info(__FUNCTION__, a1, a2)
#define log_info3(a1, a2, a3) hlog_info(__FUNCTION__, a1, a2, a3)
#define log_info4(a1, a2, a3, a4) hlog_info(__FUNCTION__, a1, a2, a3, a4)
#define log_info5(a1, a2, a3, a4, a5) hlog_info(__FUNCTION__, a1, a2, a3, a4, a5)

#define log_warn1(a1) hlog_warn(__FUNCTION__, a1)
#define log_warn2(a1, a2) hlog_warn(__FUNCTION__, a1, a2)
#define log_warn3(a1, a2, a3) hlog_warn(__FUNCTION__, a1, a2, a3)
#define log_warn4(a1, a2, a3, a4) hlog_warn(__FUNCTION__, a1, a2, a3, a4)
#define log_warn5(a1, a2, a3, a4, a5) hlog_warn(__FUNCTION__, a1, a2, a3, a4, a5)

#define log_error1(a1) hlog_error(__FUNCTION__, a1)
#define log_error2(a1, a2) hlog_error(__FUNCTION__, a1, a2)
#define log_error3(a1, a2, a3) hlog_error(__FUNCTION__, a1, a2, a3)
#define log_error4(a1, a2, a3, a4) hlog_error(__FUNCTION__, a1, a2, a3, a4)
#define log_error5(a1, a2, a3, a4, a5) hlog_error(__FUNCTION__, a1, a2, a3, a4, a5)
/* ------------------------------------------------------------------
   "multipart/related"  MIME Message Builder
   ------------------------------------------------------------------*/
herror_t mime_get_attachments(content_type_t * ctype, http_input_stream_t * in, attachments_t ** dest);
//
//
//
herror_t hrequest_new_from_socket(hsocket_t * sock, hrequest_t ** out);
void hrequest_free(hrequest_t * req);
//
//
//
herror_t hresponse_new_from_socket(hsocket_t * sock, hresponse_t ** out);
void hresponse_free(hresponse_t * res);
/*
   Begin  httpd_* function set
 */
herror_t httpd_init(int argc, char * argv[]);
void httpd_destroy(void);
herror_t httpd_run(void);
int httpd_register(const char * ctx, httpd_service service);
int httpd_register_secure(const char * ctx, httpd_service service, httpd_auth auth);
int httpd_register_default(const char * ctx, httpd_service service);
int httpd_register_default_secure(const char * ctx, httpd_service service, httpd_auth auth);
int httpd_get_port(void);
int httpd_get_timeout(void);
void httpd_set_timeout(int t);
const char * httpd_get_protocol(void);
int httpd_get_conncount(void);
hservice_t * httpd_services(void);
herror_t httpd_send_header(httpd_conn_t * res, int code, const char * text);
int httpd_set_header(httpd_conn_t * conn, const char * key, const char * value);
void httpd_set_headers(httpd_conn_t * conn, hpair_t * header);
int httpd_add_header(httpd_conn_t * conn, const char * key, const char * value);
void httpd_add_headers(httpd_conn_t * conn, const hpair_t * values);
/*
   unsigned char *httpd_get_postdata(httpd_conn_t *conn,
                         hrequest_t *req, long *received, long max);
 */
/* --------------------------------------------------------------
   MIME RELATED FUNCTIONS
   ---------------------------------------------------------------*/
/*
   MIME support httpd_mime_* function set
 */
/**
   Begin MIME multipart/related POST
   Returns: HSOCKET_OK  or error flag
 */
herror_t httpd_mime_send_header(httpd_conn_t * conn, const char * related_start, const char * related_start_info,
	const char * related_type, int code,
	const char * text);
/**
   Send boundary and part header and continue
   with next part
 */
herror_t httpd_mime_next(httpd_conn_t * conn, const char * content_id, const char * content_type,
	const char * transfer_encoding);
/**
   Send boundary and part header and continue
   with next part
 */
herror_t httpd_mime_send_file(httpd_conn_t * conn, const char * content_id, const char * content_type,
	const char * transfer_encoding,
	const char * filename);
/**
   Finish MIME request
   Returns: HSOCKET_OK  or error flag
 */
herror_t httpd_mime_end(httpd_conn_t * conn);
/**
   Initializes the socket modul. This should be called only
   once for an application.

   @returns This function should always return H_OK.
 */
herror_t hsocket_module_init(int argc, char ** argv);
/**
   Destroys the socket modul. This should be called after
   finishing an application.
 */
void hsocket_module_destroy(void);
/**
   Initializes a given socket object. This function (or
   hsokcet_init_ssl) should
   be called for every socket before using it.

   @param sock the destination socket to initialize.

   @see hsocket_init_ssl
   @returns This function should always return H_OK.
 */
herror_t hsocket_init(hsocket_t * sock);
/**
   Destroys and releases a given socket.

   @param sock the socket to destroy
 */
void hsocket_free(hsocket_t * sock);
/**
   Connects to a given host. The hostname can be an IP number
   or a humen readable hostname.

   @param sock the destonation socket object to use
   @param host hostname
   @param port port number to connect to
   @param ssl  whether to open a SSL connection

   @returns H_OK if success. One of the followings if fails:<P>
    <BR>HSOCKET_ERROR_CREATE
    <BR>HSOCKET_ERROR_GET_HOSTNAME
    <BR>HSOCKET_ERROR_CONNECT
 */
herror_t hsocket_open(hsocket_t * sock, const char * host, int port, int ssl);
/**
   Close a socket connection.

   @param sock the socket to close
 */
void hsocket_close(hsocket_t * sock);
/**
   Binds a socket to a given port number. After bind you
   can call hsocket_listen() to listen to the port.

   @param sock socket to use.
   @param port  port number to bind to

   @returns H_OK if success. One of the followings if fails:<P>
    <BR>HSOCKET_ERROR_CREATE
    <BR>HSOCKET_ERROR_BIND

   @see hsocket_listen
 */
herror_t hsocket_bind(hsocket_t * sock, int port);
/**
   Set the socket to the listen mode. You must bind
   the socket to a port with hsocket_bind() before
   you can listen to the port.

   @param sock the socket to use

   @returns H_OK if success. One of the followings if fails:<P>
    <BR>HSOCKET_ERROR_NOT_INITIALIZED
    <BR>HSOCKET_ERROR_LISTEN
 */
herror_t hsocket_listen(hsocket_t * sock);
/**
   Accepts an incoming socket request. Note that this function
   will not return until a socket connection is ready.

   @param sock the socket which listents to a port
   @param dest the destination socket which will be created

   @returns H_OK if success. One of the followings if fails:<P>
    <BR>HSOCKET_ERROR_NOT_INITIALIZED
    <BR>HSOCKET_ERROR_ACCEPT
 */
herror_t hsocket_accept(hsocket_t * sock, hsocket_t * dest);
/**
   Sends data throught the socket.

   @param sock the socket to use to send the data
   @param bytes bytes to send
   @param size size of memory to sent pointed by bytes.

   @returns H_OK if success. One of the followings if fails:<P>
    <BR>HSOCKET_ERROR_NOT_INITIALIZED
    <BR>HSOCKET_ERROR_SEND
 */
herror_t hsocket_nsend(hsocket_t * sock, const byte_t * bytes, int size);
/**
   Sends a string throught the socket

   @param sock the socket to use to send the data
   @param str the null terminated string to sent

   @returns H_OK if success. One of the followings if fails:<P>
    <BR>HSOCKET_ERROR_NOT_INITIALIZED
    <BR>HSOCKET_ERROR_SEND
*/
herror_t hsocket_send(hsocket_t * sock, const char * str);
int hsocket_select_read(int sock, char * buf, size_t len);
/**
   Reads data from the socket.

   @param sock the socket to read data from
   @param buffer the buffer to use to save the readed bytes
   @param size the maximum size of bytes to read
   @param force if force is 1 then hsocket_read() will wait until
   maximum size of bytes (size parameter) was readed. Otherwise
   this function will not wait and will return with the bytes
   quequed on the socket.

   @returns This function will return -1 if an read error was occured.
     Otherwise the return value is the size of bytes readed from
     the socket.

*/
herror_t hsocket_read(hsocket_t * sock, byte_t * buffer, int size, int force, int * readed);

#ifdef HAVE_SSL // {
/**
        Initialization and shutdown of the SSL module
 */
herror_t hssl_module_init(int argc, char ** argv);
void hssl_module_destroy(void);
void hssl_set_certificate(char * c);
void hssl_set_certpass(char * c);
void hssl_set_ca(char * c);
void hssl_enable(void);
int hssl_enabled(void);
/**
        Socket initialization and shutdown
 */
herror_t hssl_client_ssl(hsocket_t * sock);
herror_t hssl_server_ssl(hsocket_t * sock);
void hssl_cleanup(hsocket_t * sock);
/*
        Callback for password checker
 */
/* static int pw_cb(char* buf, int num, int rwflag, void *userdata); */
/*
        Quick function for verifying a portion of the cert
        nid is any NID_ defined in <openssl/objects.h>
        returns non-zero if everything went ok
 */
 #define CERT_SUBJECT    1

int verify_sn(X509 * cert, int who, int nid, char * str);
/*
        Called by framework for verify
 */
/* static int verify_cb(int prev_ok, X509_STORE_CTX* ctx); */
void hssl_set_user_verify(int func(X509 * cert));

#else // } HAVE_SSL {
static /*inline*/ herror_t hssl_module_init(int argc, char ** argv)
{
	return H_OK;
}
static /*inline*/ void hssl_module_destroy(void)
{
	return;
}
static /*inline*/ int hssl_enabled(void)
{
	return 0;
}
static /*inline*/ herror_t hssl_client_ssl(hsocket_t * sock)
{
	return H_OK;
}
static /*inline*/ herror_t hssl_server_ssl(hsocket_t * sock)
{
	return H_OK;
}
static /*inline*/ void hssl_cleanup(hsocket_t * sock)
{
	return;
}
#endif // } HAVE_SSL

herror_t hssl_read(hsocket_t * sock, char * buf, size_t len, size_t * received);
herror_t hssl_write(hsocket_t * sock, const char * buf, size_t len, size_t * sent);
//
//
//
/*
   --------------------------------------------------------------
   HTTP INPUT STREAM
   --------------------------------------------------------------
 */

/**
   Creates a new input stream. The transfer style will be
   choosen from the given header.

   @param sock the socket to receive data from
   @param header the http header. This must be received before
    creating a http_input_stream_t.

   @returns  a newly created http_input_stream_t object. If no
    transfer style was found in the header,
    HTTP_TRANSFER_CONNECTION_CLOSE  will be used as default.

   @see http_input_stream_free
 */
http_input_stream_t * http_input_stream_new(hsocket_t * sock, hpair_t * header);

/**
   Creates a new input stream from file.
   This function was added for MIME messages
   and for debugging. The transfer style is always
   HTTP_TRANSFER_FILE.

   @param filename the name of the file to open and read.

   @returns The return value is a http_input_stream_t object
   if the file exists and could be opened. NULL otherwise.

   @see   http_input_stream_free
 */
http_input_stream_t * http_input_stream_new_from_file(const char * filename);

/**
   Free input stream. Note that the socket will not be closed
   by this functions.

   @param stream the input stream to free.
 */
void http_input_stream_free(http_input_stream_t * stream);

/**
   Returns the actual status of the stream.

   @param stream the stream to check its status
   @returns <br>1 if there are still data to read.
           <br>0 if no more data exists.
 */
int http_input_stream_is_ready(http_input_stream_t * stream);

/**
   Tries to read 'size' bytes from the stream. Check always
   with http_input_stream_is_ready() if there are some data
   to read. If it returns 0, no more data is available on
   stream.
   <P>
   On error this function will return -1. In this case the
   "err" field of stream will be set to the actual error.
   This can be one of the followings: <P>

   <BR>STREAM_ERROR_NO_CHUNK_SIZE
   <BR>STREAM_ERROR_WRONG_CHUNK_SIZE
   <BR>STREAM_ERROR_INVALID_TYPE
   <BR>HSOCKET_ERROR_RECEIVE

   @param stream the stream to read data from
   @param dest destination memory to store readed bytes
   @param size maximum size of 'dest' (size to read)

   @returns the actual readed bytes or -1 on error.
 */
int http_input_stream_read(http_input_stream_t * stream, byte_t * dest, int size);

/*
   --------------------------------------------------------------
   HTTP OUTPUT STREAM
   --------------------------------------------------------------
 */

/**
   Creates a new output stream. Transfer style will be found
   from the given header.

   @param sock the socket to to send data to
   @param header the header which must be sent before

   @returns a http_output_stream_t object. If no proper transfer
   style was found in the header, HTTP_TRANSFER_CONNECTION_CLOSE
   will be used as default.

   @see http_output_stream_free
 */
http_output_stream_t * http_output_stream_new(hsocket_t * sock, hpair_t * header);

/**
   Free output stream. Note that this functions will not
   close any socket connections.

   @param stream the stream to free.
 */
void http_output_stream_free(http_output_stream_t * stream);

/**
   Writes 'size' bytes of 'bytes' into stream.

   @param stream the stream to use to send data
   @param bytes bytes to send
   @param size size of bytes to send

   @returns H_OK if success. One of the followings otherwise
    <BR>HSOCKET_ERROR_NOT_INITIALIZED
    <BR>HSOCKET_ERROR_SEND
 */
herror_t http_output_stream_write(http_output_stream_t * stream, const byte_t * bytes, int size);

/**
   Writes a null terminated string to the stream.

   @param stream the stream to use to send data
   @param str a null terminated string to send

   @returns H_OK if success. One of the followings otherwise
    <BR>HSOCKET_ERROR_NOT_INITIALIZED
    <BR>HSOCKET_ERROR_SEND
 */
herror_t http_output_stream_write_string(http_output_stream_t * stream, const char * str);

/**
   Sends finish flags if nesseccary (like in chunked transport).
   Call always this function before closing the connections.

   @param stream the stream to send post data.

   @returns H_OK if success. One of the followings otherwise
    <BR>HSOCKET_ERROR_NOT_INITIALIZED
    <BR>HSOCKET_ERROR_SEND
 */
herror_t http_output_stream_flush(http_output_stream_t * stream);

#ifdef __cplusplus
}
#endif

#endif __NANOHTTP_H // }

