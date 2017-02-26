// NANOHTTP.CPP
//
#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
//#ifdef HAVE_PROCESS_H
#include <process.h>
//#endif
#ifdef HAVE_PTHREAD_H
	#include <pthread.h>
#endif
#ifdef HAVE_SYS_SELECT_H
	#include <sys/select.h>
#endif
#ifdef HAVE_SOCKET_H
	#include <sys/socket.h>
#endif
#ifdef HAVE_TIME_H
	#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
	#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
	#include <sys/types.h>
#endif
#ifdef HAVE_SIGNAL_H
	#include <signal.h>
#endif
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif
#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
	#include <arpa/inet.h>
#endif
#ifdef HAVE_FCNTL_H
	#include <fcntl.h>
#endif
#ifdef HAVE_NETDB_H
	#include <netdb.h>
#endif
#ifdef HAVE_IO_H
	#include <io.h>
#endif
#ifdef HAVE_SSL
	#ifdef HAVE_OPENSSL_RAND_H
		#include <openssl/rand.h>
	#endif
	#ifdef HAVE_OPENSSL_ERR_H
		#include <openssl/err.h>
	#endif
#endif
#ifdef MEM_DEBUG
	#include <utils/alloc.h>
#endif
#include "nanohttp.h"

#ifdef WIN32
	#include "wsockcompat.h"
	#include <winsock2.h>
	#define inline
	#ifndef __MINGW32__
		typedef int ssize_t;
	#endif
	#undef errno
	#define errno WSAGetLastError()
#endif

static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char cd64[] = "|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/**
 * encode 3 8-bit binary bytes as 4 '6-bit' characters
 */
static void encodeblock(unsigned char in[3], unsigned char out[4], int len)
{
	out[0] = cb64[in[0]>>2];
	out[1] = cb64[((in[0]&0x03)<<4)|((in[1]&0xf0)>>4)];
	out[2] = (unsigned char)(len > 1 ? cb64[((in[1]&0x0f)<<2)|((in[2]&0xc0)>>6)] : '=');
	out[3] = (unsigned char)(len > 2 ? cb64[in[2]&0x3f] : '=');
}

/**
 * base64 encode a string.
 */
void base64_encode(const unsigned char * instr, unsigned char * outstr)
{
	unsigned char in[3], out[4];
	int i, len;
	while(*instr) {
		len = 0;
		for(i = 0; i < 3; i++) {
			if((in[i] = (unsigned char)*instr)) {
				len++;
				instr++;
			}
		}
		if(len) {
			encodeblock(in, out, len);
			for(i = 0; i < 4; i++)
				*outstr++ = out[i];
		}
	}
}

/**
 * decode 4 '6-bit' characters into 3 8-bit binary bytes
 */
static void decodeblock(unsigned char in[4], unsigned char out[3])
{
	out[0] = (unsigned char)(in[0]<<2|in[1]>>4);
	out[1] = (unsigned char)(in[1]<<4|in[2]>>2);
	out[2] = (unsigned char)(((in[2]<<6)&0xc0)|in[3]);
}

/**
 * decode a base64 encoded string (maybe broken...)
 */
void base64_decode(const unsigned char * instr, unsigned char * outstr)
{
	unsigned char in[4], out[3], v;
	int i, len;
	while(*instr) {
		for(len = 0, i = 0; i < 4 && *instr; i++) {
			v = 0;
			while(*instr && v == 0) {
				v = *instr++;
				v = (unsigned char)((v < 43 || v > 122) ? 0 : cd64[v-43]);
				if(v)
					v = (unsigned char)((v == '$') ? 0 : v-61);
			}
			if(*instr) {
				len++;
				if(v)
					in[i] = (unsigned char)(v-1);
			}
			else {
				in[i] = 0;
			}
		}
		if(len) {
			decodeblock(in, out);
			for(i = 0; i < len-1; i++)
				*outstr++ = out[i];
		}
	}
}

/*--------------------------------------------------
   FUNCTION: httpc_init
   DESC: Initialize http client connection
   NOTE: This will be called from soap_client_init_args()
   ----------------------------------------------------*/
herror_t httpc_init(int argc, char ** argv)
{
	return hsocket_module_init(argc, argv);
}

/*--------------------------------------------------
   FUNCTION: httpc_destroy
   DESC: Destroy the http client module
   ----------------------------------------------------*/
void httpc_destroy(void)
{
	hsocket_module_destroy();
}

/*--------------------------------------------------
   FUNCTION: httpc_new
   DESC: Creates a new http client connection object
   You need to create at least 1 http client connection
   to communicate via http.
   ----------------------------------------------------*/
httpc_conn_t * httpc_new()
{
	static int counter = 10000;
	herror_t status;
	httpc_conn_t * res;
	if(!(res = (httpc_conn_t *)malloc(sizeof(httpc_conn_t))))
		return NULL;
	if((status = hsocket_init(&res->sock)) != H_OK) {
		log_warn2("hsocket_init failed (%s)", herror_message(status));
		return NULL;
	}
	res->header = NULL;
	res->version = HTTP_1_1;
	res->out = NULL;
	res->_dime_package_nr = 0;
	res->_dime_sent_bytes = 0;
	res->id = counter++;
	return res;
}

/*--------------------------------------------------
   FUNCTION: httpc_free
   DESC: Free the given http client object.
   ----------------------------------------------------*/
void httpc_free(httpc_conn_t * conn)
{
	if(conn) {
		while(conn->header != NULL) {
			hpair_t * tmp = conn->header;
			conn->header = conn->header->next;
			hpairnode_free(tmp);
		}
		if(conn->out != NULL) {
			http_output_stream_free(conn->out);
			conn->out = NULL;
		}
		hsocket_free(&(conn->sock));
		free(conn);
	}
}

/*--------------------------------------------------
   FUNCTION: httpc_close_free
   DESC: Close and free the given http client object.
   ----------------------------------------------------*/
void httpc_close_free(httpc_conn_t * conn)
{
	if(conn) {
		hsocket_close(&(conn->sock));
		httpc_free(conn);
	}
}

int httpc_add_header(httpc_conn_t * conn, const char * key, const char * value)
{
	if(!conn) {
		log_warn1("Connection object is NULL");
		return -1;
	}
	else {
		conn->header = hpairnode_new(key, value, conn->header);
		return 0;
	}
}

void httpc_add_headers(httpc_conn_t * conn, const hpair_t * values)
{
	if(conn == NULL)
		log_warn1("Connection object is NULL");
	else {
		for(; values; values = values->next)
			httpc_add_header(conn, values->key, values->value);
	}
}

/*--------------------------------------------------
   FUNCTION: httpc_set_header
   DESC: Adds a new (key, value) pair to the header
   or modifies the old pair if this function will
   finds another pair with the same 'key' value.
   ----------------------------------------------------*/
int httpc_set_header(httpc_conn_t * conn, const char * key, const char * value)
{
	hpair_t * p;
	if(conn == NULL) {
		log_warn1("Connection object is NULL");
		return 0;
	}
	for(p = conn->header; p; p = p->next) {
		if(p->key && !strcmp(p->key, key)) {
			free(p->value);
			p->value = strdup(value);
			return 1;
		}
	}
	conn->header = hpairnode_new(key, value, conn->header);
	return 0;
}

static int _httpc_set_basic_authorization_header(httpc_conn_t * conn, const char * key, const char * user,
	const char * password)
{
	/* XXX: use malloc/free */
	char   in[512], out[512];
	if(!user)
		user = "";
	if(!password)
		password = "";
	memset(in, 0, sizeof(in));
	memset(out, 0, sizeof(out));
	sprintf(in, "%s:%s", user, password);
	base64_encode((unsigned char *)in, (unsigned char *)out);
	sprintf(in, "Basic %s", out);
	return httpc_set_header(conn, key, in);
}

int httpc_set_basic_authorization(httpc_conn_t * conn, const char * user, const char * password)
{
	return _httpc_set_basic_authorization_header(conn, HEADER_AUTHORIZATION, user, password);
}

int httpc_set_basic_proxy_authorization(httpc_conn_t * conn, const char * user, const char * password)
{
	return _httpc_set_basic_authorization_header(conn, HEADER_PROXY_AUTHORIZATION, user, password);
}

/*--------------------------------------------------
   FUNCTION: httpc_header_set_date
   DESC: Adds the current date to the header.
   ----------------------------------------------------*/
static void httpc_header_set_date(httpc_conn_t * conn)
{
	char   buffer[128];
	struct tm stm;
	time_t ts = time(NULL);
	localtime_r(&ts, &stm);
	strftime(buffer, 32, "%a, %d %b %Y %H:%M:%S GMT", &stm);
	httpc_set_header(conn, HEADER_DATE, buffer);
}

/*--------------------------------------------------
   FUNCTION: httpc_send_header
   DESC: Sends the current header information stored
   in conn through conn->sock.
   ----------------------------------------------------*/
herror_t httpc_send_header(httpc_conn_t * conn)
{
	hpair_t * walker;
	herror_t status;
	char buffer[1024];
	for(walker = conn->header; walker; walker = walker->next) {
		if(walker->key && walker->value) {
			sprintf(buffer, "%s: %s\r\n", walker->key, walker->value);
			if((status = hsocket_send(&(conn->sock), buffer)) != H_OK)
				return status;
		}
	}
	return hsocket_send(&(conn->sock), "\r\n");
}

/*--------------------------------------------------
   FUNCTION: httpc_talk_to_server
   DESC: This function is the heart of the httpc
   module. It will send the request and process the
   response.

   Here the parameters:

   method:
   the request method. This can be HTTP_REQUEST_POST and
   HTTP_REQUEST_GET.

   conn:
   the connection object (created with httpc_new())

   urlstr:
   the complete url in string format.
   http://<host>:<port>/<context>
   where <port> is not mendatory.

   start_cb:
   a callback function, which will be called when
   the response header is completely arrives.

   cb:
   a callback function, which will be called everytime
   when data arrives.

   content_size:
   size of content to send.
   (only if method is HTTP_REQUEST_POST)

   content:
   the content data to send.
   (only if method is HTTP_REQUEST_POST)

   userdata:
   a user define data, which will be passed to the
   start_cb and cb callbacks as a parameter. This
   can also be NULL.


   If success, this function will return 0.
   >0 otherwise.
   ----------------------------------------------------*/
static herror_t httpc_talk_to_server(hreq_method_t method, httpc_conn_t * conn, const char * urlstr)
{
	hurl_t url;
	char buffer[4096];
	herror_t status;
	int ssl;
	if(conn == NULL) {
		return herror_new("httpc_talk_to_server", GENERAL_INVALID_PARAM, "httpc_conn_t param is NULL");
	}
	/* Build request header */
	httpc_header_set_date(conn);
	if((status = hurl_parse(&url, urlstr)) != H_OK) {
		log_error2("Can not parse URL '%s'", SAVE_STR(urlstr));
		return status;
	}
/* TODO (#1#): Check for HTTP protocol in URL */

	/* Set hostname */
	httpc_set_header(conn, HEADER_HOST, url.host);
	ssl = (url.protocol == PROTOCOL_HTTPS) ? 1 : 0;
	/* Open connection */
	if((status = hsocket_open(&conn->sock, url.host, url.port, ssl)) != H_OK)
		return status;
	switch(method) {
	    case HTTP_REQUEST_GET:
		sprintf(buffer, "GET %s HTTP/%s\r\n", (url.context[0] != '\0') ? url.context : ("/"), (conn->version == HTTP_1_0) ? "1.0" : "1.1");
		break;

	    case HTTP_REQUEST_POST:

		sprintf(buffer, "POST %s HTTP/%s\r\n", (url.context[0] != '\0') ? url.context : ("/"), (conn->version == HTTP_1_0) ? "1.0" : "1.1");
		break;

	    default:
		log_error1("Unknown method type!");
		return herror_new("httpc_talk_to_server",
			GENERAL_INVALID_PARAM, "hreq_method_t must be  HTTP_REQUEST_GET or HTTP_REQUEST_POST");
	}
	log_verbose1("Sending request...");
	if((status = hsocket_send(&(conn->sock), buffer)) != H_OK) {
		log_error2("Cannot send request (%s)", herror_message(status));
		hsocket_close(&(conn->sock));
		return status;
	}
	log_verbose1("Sending header...");
	if((status = httpc_send_header(conn)) != H_OK) {
		log_error2("Cannot send header (%s)", herror_message(status));
		hsocket_close(&(conn->sock));
		return status;
	}
	return H_OK;
}

/*--------------------------------------------------
   FUNCTION: httpc_get
   DESC:
   ----------------------------------------------------*/
herror_t httpc_get(httpc_conn_t * conn, hresponse_t ** out, const char * urlstr)
{
	herror_t status;
	if((status = httpc_talk_to_server(HTTP_REQUEST_GET, conn, urlstr)) != H_OK)
		return status;
	if((status = hresponse_new_from_socket(&(conn->sock), out)) != H_OK)
		return status;
	return H_OK;
}

/*--------------------------------------------------
   FUNCTION: httpc_post_begin
   DESC: Returns H_OK if success
   ----------------------------------------------------*/
herror_t httpc_post_begin(httpc_conn_t * conn, const char * url)
{
	herror_t status;
	if((status = httpc_talk_to_server(HTTP_REQUEST_POST, conn, url)) != H_OK)
		return status;
	conn->out = http_output_stream_new(&(conn->sock), conn->header);
	return H_OK;
}

/*--------------------------------------------------
   FUNCTION: httpc_post_begin
   DESC: End a "POST" method and receive the response.
   You MUST call httpc_post_end() before!
   ----------------------------------------------------*/
herror_t httpc_post_end(httpc_conn_t * conn, hresponse_t ** out)
{
	herror_t status;
	if((status = http_output_stream_flush(conn->out)) != H_OK)
		return status;
	if((status = hresponse_new_from_socket(&(conn->sock), out)) != H_OK)
		return status;
	return H_OK;
}

/* ---------------------------------------------------
   MIME support functions httpc_mime_* function set
   -----------------------------------------------------*/

static void _httpc_mime_get_boundary(httpc_conn_t * conn, char * dest)
{
	sprintf(dest, "---=.Part_NH_%d", conn->id);
	log_verbose2("boundary= \"%s\"", dest);
}

herror_t httpc_mime_begin(httpc_conn_t * conn, const char * url,
	const char * related_start,
	const char * related_start_info, const char * related_type)
{
	herror_t status;
	char buffer[300];
	char temp[75];
	char boundary[75];

	/*
	   Set Content-type Set multipart/related parameter type=..; start=.. ;
	   start-info= ..; boundary=...

	 */
	sprintf(buffer, "multipart/related;");
	/*
	   using sprintf instead of snprintf because visual c does not support
	   snprintf */
#ifdef WIN32
 #define snprintf(buffer, num, s1, s2) sprintf(buffer, s1, s2)
#endif
	if(related_type) {
		snprintf(temp, 75, " type=\"%s\";", related_type);
		strcat(buffer, temp);
	}
	if(related_start) {
		snprintf(temp, 75, " start=\"%s\";", related_start);
		strcat(buffer, temp);
	}
	if(related_start_info) {
		snprintf(temp, 75, " start-info=\"%s\";", related_start_info);
		strcat(buffer, temp);
	}
	_httpc_mime_get_boundary(conn, boundary);
	snprintf(temp, 75, " boundary=\"%s\"", boundary);
	strcat(buffer, temp);

	httpc_set_header(conn, HEADER_CONTENT_TYPE, buffer);

	status = httpc_post_begin(conn, url);
	return status;
}

herror_t httpc_mime_next(httpc_conn_t * conn,
	const char * content_id,
	const char * content_type, const char * transfer_encoding)
{
	herror_t status;
	char buffer[512];
	char boundary[75];
	/* Get the boundary string */
	_httpc_mime_get_boundary(conn, boundary);
	sprintf(buffer, "\r\n--%s\r\n", boundary);
	/* Send boundary */
	status = http_output_stream_write(conn->out, (const byte_t *)buffer, strlen(buffer));
	if(status != H_OK)
		return status;
	/* Send Content header */
	sprintf(buffer, "%s: %s\r\n%s: %s\r\n%s: %s\r\n\r\n",
		HEADER_CONTENT_TYPE, content_type,
		HEADER_CONTENT_TRANSFER_ENCODING, transfer_encoding,
		HEADER_CONTENT_ID, content_id);
	return http_output_stream_write(conn->out, (const byte_t *)buffer, strlen(buffer));
}

herror_t httpc_mime_end(httpc_conn_t * conn, hresponse_t ** out)
{
	herror_t status;
	char buffer[512];
	char boundary[75];
	/* Get the boundary string */
	_httpc_mime_get_boundary(conn, boundary);
	sprintf(buffer, "\r\n--%s--\r\n\r\n", boundary);
	/* Send boundary */
	status = http_output_stream_write(conn->out, (const byte_t *)buffer, strlen(buffer));
	if(status != H_OK)
		return status;
	if((status = http_output_stream_flush(conn->out)) != H_OK)
		return status;
	if((status = hresponse_new_from_socket(&(conn->sock), out)) != H_OK)
		return status;
	return H_OK;
}

/**
   Send boundary and part header and continue
   with next part
 */
herror_t httpc_mime_send_file(httpc_conn_t * conn,
	const char * content_id,
	const char * content_type,
	const char * transfer_encoding, const char * filename)
{
	herror_t status;
	FILE * fd = fopen(filename, "rb");
	byte_t buffer[MAX_FILE_BUFFER_SIZE];
	size_t size;
	if(fd == NULL)
		return herror_new("httpc_mime_send_file", FILE_ERROR_OPEN, "Can not open file '%s'", filename);
	status = httpc_mime_next(conn, content_id, content_type, transfer_encoding);
	if(status != H_OK) {
		fclose(fd);
		return status;
	}
	while(!feof(fd)) {
		size = fread(buffer, 1, MAX_FILE_BUFFER_SIZE, fd);
		if(size == -1) {
			fclose(fd);
			return herror_new("httpc_mime_send_file", FILE_ERROR_READ,
				"Can not read from file '%s'", filename);
		}
		if(size > 0) {
			/* DEBUG: fwrite(buffer, 1, size, stdout); */
			status = http_output_stream_write(conn->out, buffer, size);
			if(status != H_OK) {
				fclose(fd);
				return status;
			}
		}
	}
	fclose(fd);
	log_verbose1("file sent!");
	return H_OK;
}

static int strcmpigcase(const char * s1, const char * s2)
{
	size_t l1, l2, i;
	if(s1 == NULL && s2 == NULL)
		return 1;
	if(s1 == NULL || s2 == NULL)
		return 0;
	l1 = strlen(s1);
	l2 = strlen(s2);
	if(l1 != l2)
		return 0;
	for(i = 0; i < l1; i++)
		if(toupper(s1[i]) != toupper(s2[i]))
			return 0;
	return 1;
}

typedef struct _herror_impl_t {
	int    errcode;
	char   message[250];
	char   func[100];
} herror_impl_t;

herror_t herror_new(const char * func, int errcode, const char * format, ...)
{
	va_list ap;
	herror_impl_t * impl = (herror_impl_t *)malloc(sizeof(herror_impl_t));
	impl->errcode = errcode;
	strcpy(impl->func, func);
	va_start(ap, format);
	vsprintf(impl->message, format, ap);
	va_end(ap);
	return (herror_t)impl;
}

int herror_code(herror_t err)
{
	herror_impl_t * impl = (herror_impl_t *)err;
	return (!err) ? H_OK : impl->errcode;
}

char * herror_func(herror_t err)
{
	herror_impl_t * impl = (herror_impl_t *)err;
	return (!err) ? "" : impl->func;
}

char * herror_message(herror_t err)
{
	herror_impl_t * impl = (herror_impl_t *)err;
	return (!err) ? "" : impl->message;
}

void herror_release(herror_t err)
{
	herror_impl_t * impl = (herror_impl_t *)err;
	if(err)
		free(impl);
}

hpair_t * hpairnode_new(const char * key, const char * value, hpair_t * next)
{
	hpair_t * pair;
	log_verbose3("new pair ('%s','%s')", SAVE_STR(key), SAVE_STR(value));
	pair = (hpair_t *)malloc(sizeof(hpair_t));
	if(key != NULL) {
		pair->key = (char *)malloc(strlen(key)+1);
		strcpy(pair->key, key);
	}
	else {
		pair->key = NULL;
	}
	if(value != NULL) {
		pair->value = (char *)malloc(strlen(value)+1);
		strcpy(pair->value, value);
	}
	else {
		pair->value = NULL;
	}
	pair->next = next;
	return pair;
}

hpair_t * hpairnode_parse(const char * str, const char * delim, hpair_t * next)
{
	char * key, * value;
	int c;
	hpair_t * pair = (hpair_t *)malloc(sizeof(hpair_t));
	pair->key = "";
	pair->value = "";
	pair->next = next;
	key = strtok_r((char *)str, delim, &value);
	if(key != NULL) {
		pair->key = (char *)malloc(strlen(key)+1);
		strcpy(pair->key, key);
	}
	if(value != NULL) {
		for(c = 0; value[c] == ' '; c++) ;  /* skip white space */
		pair->value = (char *)malloc(strlen(&value[c])+1);
		strcpy(pair->value, &value[c]);
	}
	return pair;
}

hpair_t * hpairnode_copy(const hpair_t * src)
{
	hpair_t * pair;
	if(src == NULL)
		return NULL;
	pair = hpairnode_new(src->key, src->value, NULL);
	return pair;
}

hpair_t * hpairnode_copy_deep(const hpair_t * src)
{
	hpair_t * result = 0;
	if(src) {
		result = hpairnode_copy(src);
		hpair_t * next = src->next;
		hpair_t * pair = result;
		while(next != NULL) {
			pair->next = hpairnode_copy(next);
			pair = pair->next;
			next = next->next;
		}
	}
	return result;
}

void hpairnode_dump(hpair_t * pair)
{
	if(pair == NULL)
		log_verbose1("(NULL)[]");
	else
		log_verbose5("(%p)['%s','%s','%p']", pair, SAVE_STR(pair->key), SAVE_STR(pair->value), pair->next);
}

void hpairnode_dump_deep(hpair_t * pair)
{
	hpair_t * p = pair;
	log_verbose1("-- BEGIN dump hpairnode_t --");
	while(p != NULL) {
		hpairnode_dump(p);
		p = p->next;
	}
	log_verbose1("-- END dump hpairnode_t --\n");
}

void hpairnode_free(hpair_t * pair)
{
	if(pair) {
		free(pair->key);
		free(pair->value);
		free(pair);
	}
}

void hpairnode_free_deep(hpair_t * pair)
{
	while(pair != NULL) {
		hpair_t * tmp = pair->next;
		hpairnode_free(pair);
		pair = tmp;
	}
}

char * hpairnode_get_ignore_case(hpair_t * pair, const char * key)
{
	if(key == NULL) {
		log_error1("key is NULL");
		return NULL;
	}
	while(pair != NULL) {
		if(pair->key != NULL) {
			if(strcmpigcase(pair->key, key)) {
				return pair->value;
			}
		}
		pair = pair->next;
	}
	return NULL;
}

char * hpairnode_get(hpair_t * pair, const char * key)
{
	if(key == NULL) {
		log_error1("key is NULL");
		return NULL;
	}
	while(pair != NULL) {
		if(pair->key != NULL) {
			if(!strcmp(pair->key, key)) {
				return pair->value;
			}
		}
		pair = pair->next;
	}
	return NULL;
}

static void hurl_dump(const hurl_t * url)
{
	if(url == NULL) {
		log_error1("url is NULL!");
	}
	else {
		log_verbose2("PROTOCOL : %d", url->protocol);
		log_verbose2("    HOST : %s", url->host);
		log_verbose2("    PORT : %d", url->port);
		log_verbose2(" CONTEXT : %s", url->context);
	}
}

herror_t hurl_parse(hurl_t * url, const char * urlstr)
{
	int    ihost;
	int    iport;
	int    size;
	char   tmp[8];
	char   protocol[1024];
	int    iprotocol = 0;
	int    len = (int)strlen(urlstr);
	/* find protocol */
	while(urlstr[iprotocol] != ':' && urlstr[iprotocol] != '\0') {
		iprotocol++;
	}
	if(iprotocol == 0) {
		log_error1("no protocol");
		return herror_new("hurl_parse", URL_ERROR_NO_PROTOCOL, "No protocol");
	}
	if(iprotocol+3 >= len) {
		log_error1("no host");
		return herror_new("hurl_parse", URL_ERROR_NO_HOST, "No host");
	}
	if(urlstr[iprotocol] != ':' && urlstr[iprotocol+1] != '/' && urlstr[iprotocol+2] != '/') {
		log_error1("no protocol");
		return herror_new("hurl_parse", URL_ERROR_NO_PROTOCOL, "No protocol");
	}
	/* find host */
	ihost = iprotocol+3;
	while(urlstr[ihost] != ':' && urlstr[ihost] != '/' && urlstr[ihost] != '\0') {
		ihost++;
	}
	if(ihost == iprotocol+1) {
		log_error1("no host");
		return herror_new("hurl_parse", URL_ERROR_NO_HOST, "No host");
	}
	/* find port */
	iport = ihost;
	if(ihost+1 < len) {
		if(urlstr[ihost] == ':') {
			while(urlstr[iport] != '/' && urlstr[iport] != '\0') {
				iport++;
			}
		}
	}
	/* find protocol */
	strncpy(protocol, urlstr, iprotocol);
	protocol[iprotocol] = '\0';
	if(strcmpigcase(protocol, "http"))
		url->protocol = PROTOCOL_HTTP;
	else if(strcmpigcase(protocol, "https"))
		url->protocol = PROTOCOL_HTTPS;
	else if(strcmpigcase(protocol, "ftp"))
		url->protocol = PROTOCOL_FTP;
	else
		return herror_new("hurl_parse", URL_ERROR_UNKNOWN_PROTOCOL, "Unknown protocol '%s'", protocol);
	/* TODO (#1#): add max of size and URL_MAX_HOST_SIZE */
	size = ihost-iprotocol-3;
	strncpy(url->host, &urlstr[iprotocol+3], size);
	url->host[size] = '\0';
	if(iport > ihost) {
		size = iport-ihost;
		strncpy(tmp, &urlstr[ihost+1], size);
		url->port = atoi(tmp);
	}
	else {
		switch(url->protocol) {
		    case PROTOCOL_HTTP: url->port = URL_DEFAULT_PORT_HTTP; break;
		    case PROTOCOL_HTTPS: url->port = URL_DEFAULT_PORT_HTTPS; break;
		    case PROTOCOL_FTP: url->port = URL_DEFAULT_PORT_FTP; break;
		}
	}
	len = (int)strlen(urlstr);
	if(len > iport) {
		/* TODO (#1#): find max of size and URL_MAX_CONTEXT_SIZE */
		size = len-iport;
		strncpy(url->context, &urlstr[iport], size);
		url->context[size] = '\0';
	}
	else {
		url->context[0] = '\0';
	}
	hurl_dump(url);
	return H_OK;
}

/* Content-type stuff */

content_type_t * content_type_new(const char * content_type_str)
{
	hpair_t * pair = NULL, * last = NULL;
	content_type_t * ct;
	char ch, key[256], value[256];
	int inQuote = 0, i = 0, c = 0, begin = 0, len;
	int mode = 0;
	/* 0: searching ';' 1: process key 2: process value */

	/* Create object */
	ct = (content_type_t *)malloc(sizeof(content_type_t));
	ct->params = NULL;
	len = (int)strlen(content_type_str);
	while(i <= len) {
		if(i != len)
			ch = content_type_str[i++];
		else {
			ch = ' ';
			i++;
		}
		switch(mode) {
		    case 0:
				if(ch == ';') {
					ct->type[c] = '\0';
					c = 0;
					mode = 1;
				}
				else if(ch != ' ' && ch != '\t' && ch != '\r')
					ct->type[c++] = ch;
				break;
		    case 1:
				if(ch == '=') {
					key[c] = '\0';
					c = 0;
					mode = 2;
				}
				else if(ch != ' ' && ch != '\t' && ch != '\r')
					key[c++] = ch;
				break;
		    case 2:
				if(ch != ' ')
					begin = 1;
				if((ch == ' ' || ch == ';') && !inQuote && begin) {
					value[c] = '\0';
					pair = hpairnode_new(key, value, NULL);
					if(ct->params == NULL)
						ct->params = pair;
					else
						last->next = pair;
					last = pair;
					c = 0;
					begin = 0;
					mode = 1;
				}
				else if(ch == '"')
					inQuote = !inQuote;
				else if(begin && ch != '\r')
					value[c++] = ch;
				break;
		}
	}
	return ct;
}

void content_type_free(content_type_t * ct)
{
	if(ct) {
		hpairnode_free_deep(ct->params);
		free(ct);
	}
}

part_t * part_new(const char * id, const char * filename,
	const char * content_type, const char * transfer_encoding, part_t * next)
{
	part_t * part = (part_t *)malloc(sizeof(part_t));
	part->header = NULL;
	part->next = next;
	part->deleteOnExit = 0;
	strcpy(part->id, id);
	strcpy(part->filename, filename);
	if(content_type)
		strcpy(part->content_type, content_type);
	else
		part->content_type[0] = '\0';
	part->header = hpairnode_new(HEADER_CONTENT_ID, id, part->header);
	/* TODO (#1#): encoding is always binary. implement also others! */
	/*  part->header = hpairnode_new(HEADER_CONTENT_TRANSFER_ENCODING, "binary", part->header);*/
	strcpy(part->transfer_encoding, transfer_encoding ? transfer_encoding : "binary");
	if(content_type) {
		part->header =
		        hpairnode_new(HEADER_CONTENT_TYPE, content_type, part->header);
	}
	else {
		// TODO (#1#): get content-type from mime type list
	}
	return part;
}

void part_free(part_t * part)
{
	if(part) {
		if(part->deleteOnExit)
			remove(part->filename);
		hpairnode_free_deep(part->header);
		free(part);
	}
}

attachments_t * attachments_new()               /* should be used internally */
{
	attachments_t * p_attachments = (attachments_t *)malloc(sizeof(attachments_t));
	if(p_attachments) {
		p_attachments->parts = NULL;
		p_attachments->last = NULL;
		p_attachments->root_part = NULL;
	}
	return p_attachments;
}

void attachments_add_part(attachments_t * attachments, part_t * part)
{
	if(attachments) {
		if(attachments->last)
			attachments->last->next = part;
		else
			attachments->parts = part;
		attachments->last = part;
	}
}
/*
   Free a mime message
 */
void attachments_free(attachments_t * message)
{
	part_t * tmp, * part;
	if(message) {
		part = message->parts;
		while(part) {
			tmp = part->next;
			part_free(part);
			part = tmp;
		}
		if(message->root_part)
			part_free(message->root_part);
		// TODO (#1#): HERE IS A BUG!!!!
		free(message);
	}
}

#ifdef WIN32

/* strtok_r() */
char * strtok_r(char * s, const char * delim, char ** save_ptr)
{
	char * token;
	if(s == NULL)
		s = *save_ptr;
	/* Scan leading delimiters.  */
	s += strspn(s, delim);
	if(*s == '\0')
		return NULL;
	/* Find the end of the token.  */
	token = s;
	s = strpbrk(token, delim);
	if(s == NULL)
		/* This token finishes the string.  */
		*save_ptr = strchr(token, '\0');
	else {
		/* Terminate the token and make *SAVE_PTR point past it.  */
		*s = '\0';
		*save_ptr = s+1;
	}
	return token;
}

/* localtime_r() */
struct tm * localtime_r(const time_t * const timep, struct tm * p_tm)
{
	if(p_tm) {
		tm * tmp = localtime(timep);
		if(tmp)
			memcpy(p_tm, tmp, sizeof(struct tm));
		else
			memset(p_tm, 0, sizeof(*p_tm));
	}
	return p_tm;
}

#endif

#ifdef WIN32
 #ifndef __MINGW32__

/* not thread safe!*/
char * VisualC_funcname(const char * file, int line)
{
	static char buffer[256];
	int i = (int)strlen(file)-1;
	while(i > 0 && file[i] != '\\')
		i--;
	sprintf(buffer, "%s:%d", (file[i] != '\\') ? file : (file+i+1), line);
	return buffer;
}

 #endif
#endif

static log_level_t loglevel = HLOG_DEBUG;
static char logfile[75] = { '\0' };
static int log_background = 0;

log_level_t hlog_set_level(log_level_t level)
{
	log_level_t old = loglevel;
	loglevel = level;
	return old;
}

log_level_t hlog_get_level(void)
{
	return loglevel;
}

void hlog_set_file(const char * filename)
{
	if(filename)
		strncpy(logfile, filename, 75);
	else
		logfile[0] = '\0';
}

void hlog_set_background(int state)
{
	log_background = state;
}

char * hlog_get_file()
{
	return (logfile[0] == '\0') ? NULL : logfile;
}

static void _log_write(log_level_t level, const char * prefix, const char * func, const char * format, va_list ap)
{
	char   buffer[1054];
	char   buffer2[1054];
	FILE * f;
	if(level < loglevel)
		return;
	if(!log_background || hlog_get_file()) {
#ifdef WIN32
		sprintf(buffer, "*%s*: [%s] %s\n", prefix, func, format);
#else
		sprintf(buffer, "*%s*:(%ld) [%s] %s\n", prefix, pthread_self(), func, format);
#endif
		vsprintf(buffer2, buffer, ap);
		if(!log_background) {
			printf(buffer2);
			fflush(stdout);
		}
		if(hlog_get_file()) {
			f = fopen(hlog_get_file(), "a");
			if(!f)
				f = fopen(hlog_get_file(), "w");
			if(f) {
				fprintf(f, buffer2);
				fflush(f);
				fclose(f);
			}
		}
	}
}

void hlog_verbose(const char * FUNC, const char * format, ...)
{
	va_list ap;
	va_start(ap, format);
	_log_write(HLOG_VERBOSE, "VERBOSE", FUNC, format, ap);
	va_end(ap);
}

void hlog_debug(const char * FUNC, const char * format, ...)
{
	va_list ap;
	va_start(ap, format);
	_log_write(HLOG_DEBUG, "DEBUG", FUNC, format, ap);
	va_end(ap);
}

void hlog_info(const char * FUNC, const char * format, ...)
{
	va_list ap;
	va_start(ap, format);
	_log_write(HLOG_INFO, "INFO", FUNC, format, ap);
	va_end(ap);
}

void hlog_warn(const char * FUNC, const char * format, ...)
{
	va_list ap;
	va_start(ap, format);
	_log_write(HLOG_WARN, "WARN", FUNC, format, ap);
	va_end(ap);
}

void hlog_error(const char * FUNC, const char * format, ...)
{
	va_list ap;
	va_start(ap, format);
	_log_write(HLOG_ERROR, "ERROR", FUNC, format, ap);
	va_end(ap);
}

/*----------------------------------------------------------------
   Buffered Reader. A helper object to read bytes from a source
   ----------------------------------------------------------------*/

/* ------------------------------------------------------------------
   MIME Parser
   ------------------------------------------------------------------*/
typedef void (*MIME_part_begin)(void *);
typedef void (*MIME_part_end)(void *);
typedef void (*MIME_parse_begin)(void *);
typedef void (*MIME_parse_end)(void *);
typedef void (*MIME_ERROR_bytes)(void *, const unsigned char *, int);

typedef enum _MIME_parser_status {
	MIME_PARSER_INCOMPLETE_MESSAGE,
	MIME_PARSER_READ_ERROR,
	MIME_PARSER_OK
} MIME_parser_status;

typedef enum _MIME_read_status {
	MIME_READ_OK,
	MIME_READ_EOF,
	MIME_READ_ERROR
} MIME_read_status;

#define MIME_READER_MAX_BUFFER_SIZE  1054
#define MIME_PARSER_BUFFER_SIZE 1054

typedef MIME_read_status (*MIME_read_function)(void *, unsigned char *, int *);
/**
   Reader structure. This will be use
   by the parser
 */
typedef struct _MIME_reader {
	int    size;
	int    marker;
	int    current;
	MIME_read_function read_function;
	char   buffer[MIME_READER_MAX_BUFFER_SIZE];
	void * userdata;
} MIME_reader;

MIME_read_status MIME_filereader_function(void * userdata, unsigned char * dest, int * size);

typedef struct _MIME_callbacks {
	MIME_part_begin part_begin_cb;
	MIME_part_end part_end_cb;
	MIME_parse_begin parse_begin_cb;
	MIME_parse_end parse_end_cb;
	MIME_ERROR_bytes received_bytes_cb;
} MIME_callbacks;

MIME_parser_status MIME_parse(MIME_read_function reader_function, void * reader_userdata, const char * user_boundary,
	const MIME_callbacks * callbacks,
	void * callbacks_userdata);

/**
   Initialize a reader
 */
void MIME_reader_init(MIME_reader * reader,
	MIME_read_function reader_function, void * userdata)
{
	reader->size = 0;
	reader->marker = -1;
	reader->current = 0;
	reader->userdata = userdata;
	reader->read_function = reader_function;

}

/**
   Read data from a reader source.
 */
MIME_read_status MIME_reader_read(MIME_reader * reader, unsigned char * buffer, int size)
{
	MIME_read_status status;
	int readed_size;
	unsigned char tempBuffer[MIME_READER_MAX_BUFFER_SIZE];
	int rest_size;
	/* Check if buffer is full */
	if(reader->size == reader->current) {
		/* Yes, so read some data */
		/* First handle marker */
		if(reader->marker > -1) {
			if(reader->marker != 0) {
				memcpy(tempBuffer, reader->buffer+reader->marker, reader->size-reader->marker);
				memcpy(reader->buffer, tempBuffer, reader->size-reader->marker);
				reader->current = reader->size-reader->marker;
			}
			else if(reader->current == MIME_READER_MAX_BUFFER_SIZE-1) {
				fprintf(stderr, "Marker error");
				return MIME_READ_ERROR;
			}
			reader->marker = 0;
		}
		else
			reader->current = 0;
		readed_size = MIME_READER_MAX_BUFFER_SIZE-reader->current-1;
		status = reader->read_function(reader->userdata, (unsigned char *)(reader->buffer+reader->current), &readed_size);
		if(status == MIME_READ_OK) {
			reader->size = readed_size+reader->current;
		}
		else
			return status;
	}
	if(size <= reader->size-reader->current) {
		memcpy(buffer, reader->buffer+reader->current, size);
		reader->current += size;
		return MIME_READ_OK;
	}
	else {
		/* Fill rest data */
		rest_size = reader->size-reader->current;
		memcpy(buffer, reader->buffer+reader->current, rest_size);
		reader->current = reader->size;
		return MIME_reader_read(reader, buffer+rest_size, size-rest_size);
	}
}

void MIME_reader_set_marker(MIME_reader * reader)
{
	reader->marker = reader->current;
}

void MIME_reader_unset_marker(MIME_reader * reader)
{
	reader->marker = -1;
}

void MIME_reader_jump_marker(MIME_reader * reader)
{
	reader->current = reader->marker;
}

typedef struct _MIME_buffer {
	unsigned char data[MIME_PARSER_BUFFER_SIZE];
	int size;
} MIME_buffer;

void MIME_buffer_init(MIME_buffer * buffer)
{
	buffer->size = 0;
}

void MIME_buffer_add(MIME_buffer * buffer, unsigned char ch)
{
	buffer->data[buffer->size++] = ch;
}

void MIME_buffer_add_bytes(MIME_buffer * buffer, unsigned char * bytes, int size)
{
	memcpy(buffer->data, bytes, size);
	buffer->size += size;
}

int MIME_buffer_is_full(MIME_buffer * buffer)
{
	return buffer->size+150 >= MIME_PARSER_BUFFER_SIZE;
}

int MIME_buffer_is_empty(MIME_buffer * buffer)
{
	return buffer->size == 0;
}

void MIME_buffer_clear(MIME_buffer * buffer)
{
	buffer->size = 0;
}

MIME_parser_status MIME_parse(MIME_read_function reader_function,
	void * reader_userdata,
	const char * user_boundary,
	const MIME_callbacks * callbacks, void * callbacks_userdata)
{
	char boundary[150];
	unsigned char ch[153];
	int boundary_length, n, ignore = 0;
	MIME_reader reader;
	MIME_buffer buffer;
	MIME_read_status status;
	/* Init reader */
	MIME_reader_init(&reader, reader_function, reader_userdata);
	/* Init buffer */
	MIME_buffer_init(&buffer);
	/* Set boundary related stuff */
	sprintf(boundary, "\n--%s", user_boundary);
	boundary_length = (int)strlen(boundary);
	/* Call parse begin callback */
	callbacks->parse_begin_cb(callbacks_userdata);
	while(1) {
set_marker:
		/* Set marker */
		MIME_reader_set_marker(&reader);

read_byte:

		/* Read 1 byte */
		status = MIME_reader_read(&reader, ch, 1);
		if(status == MIME_READ_EOF)
			return MIME_PARSER_INCOMPLETE_MESSAGE;
		else if(status == MIME_READ_ERROR)
			return MIME_PARSER_READ_ERROR;
		if(ch[0] == '\r' && !ignore) {
			n = 0;
			while(n < boundary_length) {
				/* Read 1 byte */
				status = MIME_reader_read(&reader, ch, 1);
				if(status == MIME_READ_EOF)
					return MIME_PARSER_INCOMPLETE_MESSAGE;
				else if(status == MIME_READ_ERROR)
					return MIME_PARSER_READ_ERROR;
				/* Check if byte is in boundary */
				if(ch[0] == boundary[n]) {
					n = n+1;
					continue;
				}
				else {
					MIME_reader_jump_marker(&reader);
					ignore = 1;
					goto read_byte;
				}
			}       /* while n < boundary_length */

			/* Read 1 byte */
			status = MIME_reader_read(&reader, ch, 1);
			if(status == MIME_READ_EOF)
				return MIME_PARSER_INCOMPLETE_MESSAGE;
			else if(status == MIME_READ_ERROR)
				return MIME_PARSER_READ_ERROR;
			/* Show if byte is '\r' */
			if(ch[0] == '\r') {
				/* Read 1 byte */
				status = MIME_reader_read(&reader, ch, 1);
				if(status == MIME_READ_EOF)
					return MIME_PARSER_INCOMPLETE_MESSAGE;
				else if(status == MIME_READ_ERROR)
					return MIME_PARSER_READ_ERROR;
				/* Check if byte is '\n' */
				if(ch[0] == '\n') {
					if(!MIME_buffer_is_empty(&buffer)) {
						/* Invoke callback */
						callbacks->received_bytes_cb(callbacks_userdata, buffer.data,
							buffer.size);

						/* Empty the buffer */
						MIME_buffer_clear(&buffer);

						/* Invoke End Part */
						callbacks->part_end_cb(callbacks_userdata);
					}
					/* Invoke start new Part */
					callbacks->part_begin_cb(callbacks_userdata);
					goto set_marker;

				} /* if (ch[0] == '\n') */
				else {
					/* Jump to marker and read bytes */
					MIME_reader_jump_marker(&reader);
					MIME_reader_read(&reader, ch, boundary_length+2);

					MIME_buffer_add_bytes(&buffer, ch, boundary_length+2);
					if(MIME_buffer_is_full(&buffer)) {
						/* Invoke callback */
						callbacks->received_bytes_cb(callbacks_userdata, buffer.data,
							buffer.size);

						/* Empty the buffer */
						MIME_buffer_clear(&buffer);
					}
				} /* else of if (ch[0] == '\n') */

			}       /* if (ch[0] == '\r') */
			else {
				if(ch[0] == '-') {
					/* Read 1 byte */
					status = MIME_reader_read(&reader, ch, 1);
					if(status == MIME_READ_EOF)
						return MIME_PARSER_INCOMPLETE_MESSAGE;
					else if(status == MIME_READ_ERROR)
						return MIME_PARSER_READ_ERROR;
					if(ch[0] == '-') {
						if(!MIME_buffer_is_empty(&buffer)) {
							/* Invoke callback */
							callbacks->received_bytes_cb(callbacks_userdata, buffer.data,
								buffer.size);

							/* Empty the buffer */
							MIME_buffer_clear(&buffer);

							/* Invoke End Part */
							callbacks->part_end_cb(callbacks_userdata);
						}
						/* Invoke start new Part */
						callbacks->parse_end_cb(callbacks_userdata);

						/* Finish parsing */
						/* TODO (#1#): We assume that after -- comes \r\n This is not
						   always correct */

						return MIME_PARSER_OK;

					} /* if (ch[0] == '-') */
					else {
						MIME_reader_jump_marker(&reader);
						ignore = 1;
						goto read_byte;
					} /* else of if (ch[0] == '-') */

				} /* if (ch[0] == '-') */
				else {
					MIME_reader_jump_marker(&reader);
					ignore = 1;
					goto read_byte;
				} /* else of if (ch[0] == '-') */

			}       /* else of if (ch[0] == '\r') */

		}               /* if ch[0] == '\r' && !ignore */
		else {
			ignore = 0;
			MIME_buffer_add(&buffer, ch[0]);
			/* Chec if buffer is full */
			if(MIME_buffer_is_full(&buffer)) {
				/* Invoke callback */
				callbacks->received_bytes_cb(callbacks_userdata, buffer.data,
					buffer.size);

				/* Empty the buffer */
				MIME_buffer_clear(&buffer);
			}
		}               /* else of if ch[0] == '\r' && !ignore */
	}                       /* while (1) */
}

MIME_read_status MIME_filereader_function(void * userdata, unsigned char * dest, int * size)
{
	FILE * f = (FILE *)userdata;
	if(feof(f))
		return MIME_READ_EOF;
	else {
		*size = (int)fread(dest, 1, *size, f);
		return MIME_READ_OK;
	}
}

/* ------------------------------------------------------------------
   "multipart/related"  MIME Message Builder
   ------------------------------------------------------------------*/

/*
   Callback data to use internally
 */
typedef struct _mime_callback_data {
	int part_id;
	attachments_t * message;
	part_t * current_part;
	int buffer_capacity;
	char header[4064];
	char root_id[256];
	int header_index;
	int header_search;
	FILE * current_fd;
	char root_dir[512];
} mime_callback_data_t;

MIME_read_status mime_streamreader_function(void * userdata, unsigned char * dest, int * size)
{
	int readed = 0;
	http_input_stream_t * in = (http_input_stream_t *)userdata;
	if(!http_input_stream_is_ready(in))
		return MIME_READ_EOF;
	readed = http_input_stream_read(in, dest, *size);
	/*
	   log_info1("http_input_stream_read() returned 0"); */
	if(readed == -1) {
		log_error4("[%d] %s():%s ", herror_code(in->err), herror_func(in->err),
			herror_message(in->err));
	}
	*size = readed;
	if(*size != -1) {
		return MIME_READ_OK;
	}
	return MIME_READ_ERROR;
}
/*
   Start Callback functions
 */
static void _mime_parse_begin(void * data)
{
/* Nothing to do
   mime_callback_data_t *cbdata = (mime_callback_data_t)data;
 */
	log_verbose2("Begin parse (%p)", data);
}

static void _mime_parse_end(void * data)
{
/* Nothing to do
   mime_callback_data_t *cbdata = (mime_callback_data_t)data;
 */
	log_verbose2("End parse (%p)", data);
}

static void _mime_part_begin(void * data)
{
	char buffer[1054];
	mime_callback_data_t * cbdata = (mime_callback_data_t *)data;
	part_t * part;
	log_verbose2("Begin Part (%p)", data);
	part = (part_t *)malloc(sizeof(part_t));
	part->next = NULL;
	if(cbdata->current_part)
		cbdata->current_part->next = part;
	cbdata->current_part = part;
	if(!cbdata->message->parts)
		cbdata->message->parts = part;
	cbdata->header[0] = '\0';
	cbdata->header_index = 0;
	cbdata->header_search = 0;

#ifdef WIN32
	sprintf(buffer, "%s\\mime_%p_%d.part", cbdata->root_dir, cbdata, cbdata->part_id++);
#else
	sprintf(buffer, "%s/mime_%p_%d.part", cbdata->root_dir, cbdata, cbdata->part_id++);
#endif

/*  log_info2("Creating FILE ('%s') deleteOnExit=1", buffer);*/
	part->deleteOnExit = 1;
	cbdata->current_fd = fopen(buffer, "wb");
	if(cbdata->current_fd)
		strcpy(cbdata->current_part->filename, buffer);
	else
		log_error2("Can not open file for write '%s'", buffer);
}

static void _mime_part_end(void * data)
{
	mime_callback_data_t * cbdata = (mime_callback_data_t *)data;
	log_verbose2("End Part (%p)", data);
	if(cbdata->current_fd) {
		fclose(cbdata->current_fd);
		cbdata->current_fd = NULL;
	}
}

static hpair_t * _mime_process_header(char * buffer)
{
	int i = 0, c = 0, proc_key, begin = 0;
	hpair_t * first = NULL, * last = NULL;
	char key[1054], value[1054];
	key[0] = '\0';
	value[0] = '\0';
	proc_key = 1;
	while(buffer[i] != '\0') {
		if(buffer[i] == '\r' && buffer[i+1] == '\n') {
			value[c] = '\0';
			if(last) {
				last->next = hpairnode_new(key, value, NULL);
				last = last->next;
			}
			else {
				first = last = hpairnode_new(key, value, NULL);
			}
			proc_key = 1;
			c = 0;
			i++;
		}
		else if(buffer[i] == ':') {
			key[c] = '\0';
			c = 0;
			begin = 0;
			proc_key = 0;
		}
		else {
			if(proc_key)
				key[c++] = buffer[i];
			else {
				if(buffer[i] != ' ')
					begin = 1;
				if(begin)
					value[c++] = buffer[i];
			}
		}
		i++;
	}
	return first;
}

static void _mime_received_bytes(void * data, const unsigned char * bytes, int size)
{
	int i = 0;
	char * id, * type, * location;
	mime_callback_data_t * cbdata = (mime_callback_data_t *)data;
	if(!cbdata) {
		log_error1("MIME transport error Called <received bytes> without initializing\n");
		return;
	}
	if(!cbdata->current_part) {
		log_error1("MIME transport error Called <received bytes> without initializing\n");
		return;
	}
	// log_verbose4("Received %d bytes (%p), header_search = %d", size, data, cbdata->header_search);
	if(cbdata->header_search < 4) {
		/* Find \r\n\r\n in bytes */
		for(i = 0; i < size; i++) {
			if(cbdata->header_search == 0) {
				if(bytes[i] == '\r')
					cbdata->header_search++;
				else {
					cbdata->header[cbdata->header_index++] = bytes[i];
					cbdata->header_search = 0;
				}
			}
			else if(cbdata->header_search == 1) {
				if(bytes[i] == '\n')
					cbdata->header_search++;
				else {
					cbdata->header[cbdata->header_index++] = '\r';
					cbdata->header[cbdata->header_index++] = bytes[i];
					cbdata->header_search = 0;
				}
			}
			else if(cbdata->header_search == 2) {
				if(bytes[i] == '\r')
					cbdata->header_search++;
				else {
					cbdata->header[cbdata->header_index++] = '\r';
					cbdata->header[cbdata->header_index++] = '\n';
					cbdata->header[cbdata->header_index++] = bytes[i];
					cbdata->header_search = 0;
				}
			}
			else if(cbdata->header_search == 3) {
				if(bytes[i] == '\n') {
					cbdata->header[cbdata->header_index++] = '\r';
					cbdata->header[cbdata->header_index++] = '\n';
					cbdata->header[cbdata->header_index++] = '\0';
					cbdata->header_search = 4;
					cbdata->current_part->header = _mime_process_header(cbdata->header);
					hpairnode_dump_deep(cbdata->current_part->header);
					/* set id */
					id = hpairnode_get(cbdata->current_part->header, HEADER_CONTENT_ID);
					if(id != NULL) {
						strcpy(cbdata->current_part->id, id);
						if(!strcmp(id, cbdata->root_id))
							cbdata->message->root_part = cbdata->current_part;
					}
					location = hpairnode_get(cbdata->current_part->header, HEADER_CONTENT_LOCATION);
					if(location != NULL) {
						strcpy(cbdata->current_part->location, location);
					}
					type = hpairnode_get(cbdata->current_part->header, HEADER_CONTENT_TYPE);
					if(type != NULL) {
						strcpy(cbdata->current_part->content_type, type);
					}
					i++;
					break;
				}
				else {
					cbdata->header[cbdata->header_index++] = '\r';
					cbdata->header[cbdata->header_index++] = '\n';
					cbdata->header[cbdata->header_index++] = '\r';
					cbdata->header[cbdata->header_index++] = bytes[i];
					cbdata->header_search = 0;
				}
			}
			/* TODO (#1#): Check for cbdata->header overflow */

		}               /* for (i=0;i<size;i++) */
	}                       /* if (cbdata->header_search < 4) */
	if(i >= size-1)
		return;
	/* Write remaining bytes into the file or buffer (if root) (buffer is
	   disabled in this version) */
	if(cbdata->current_fd)
		fwrite(&(bytes[i]), 1, size-i, cbdata->current_fd);
}

/*
   The mime message parser
 */

attachments_t * mime_message_parse(http_input_stream_t * in, const char * root_id,
	const char * boundary, const char * dest_dir)
{
	MIME_parser_status status;
	MIME_callbacks callbacks;
	attachments_t * message;
	mime_callback_data_t * cbdata = (mime_callback_data_t *)malloc(sizeof(mime_callback_data_t));
	cbdata->part_id = 100;
	cbdata->buffer_capacity = 0;
	cbdata->current_fd = NULL;
	cbdata->current_part = NULL;
	cbdata->header_index = 0;
	cbdata->header_search = 0;
	strcpy(cbdata->root_id, root_id);
	strcpy(cbdata->root_dir, dest_dir);
	message = (attachments_t *)malloc(sizeof(attachments_t));
	cbdata->message = message;
	cbdata->message->parts = NULL;
	cbdata->message->root_part = NULL;
	callbacks.parse_begin_cb = _mime_parse_begin;
	callbacks.parse_end_cb = _mime_parse_end;
	callbacks.part_begin_cb = _mime_part_begin;
	callbacks.part_end_cb = _mime_part_end;
	callbacks.received_bytes_cb = _mime_received_bytes;
	status = MIME_parse(mime_streamreader_function, in, boundary, &callbacks, cbdata);
	if(status == MIME_PARSER_OK) {
		free(cbdata);
		return message;
	}
	else {
		log_error2("MIME parser error '%s'!", status == MIME_PARSER_READ_ERROR ? "read error" : "Incomplete message");
		return NULL;
	}
}

attachments_t * mime_message_parse_from_file(FILE * in, const char * root_id,
	const char * boundary, const char * dest_dir)
{
	MIME_parser_status status;
	MIME_callbacks callbacks;
	attachments_t * message;
	mime_callback_data_t * cbdata = (mime_callback_data_t *)malloc(sizeof(mime_callback_data_t));
	cbdata->part_id = 100;
	cbdata->buffer_capacity = 0;
	cbdata->current_fd = NULL;
	cbdata->current_part = NULL;
	cbdata->header_index = 0;
	cbdata->header_search = 0;
	strcpy(cbdata->root_id, root_id);
	strcpy(cbdata->root_dir, dest_dir);
	message = (attachments_t *)malloc(sizeof(attachments_t));
	cbdata->message = message;
	cbdata->message->parts = NULL;
	cbdata->message->root_part = NULL;
	callbacks.parse_begin_cb = _mime_parse_begin;
	callbacks.parse_end_cb = _mime_parse_end;
	callbacks.part_begin_cb = _mime_part_begin;
	callbacks.part_end_cb = _mime_part_end;
	callbacks.received_bytes_cb = _mime_received_bytes;
	status = MIME_parse(MIME_filereader_function, in, boundary, &callbacks, cbdata);
	if(status == MIME_PARSER_OK) {
		free(cbdata);
		return message;
	}
	else {
		/* TODO (#1#): Free objects */
		log_error2("MIME parser error '%s'!", status == MIME_PARSER_READ_ERROR ? "general error" : "Incomplete message");
		return NULL;
	}
}

herror_t mime_get_attachments(content_type_t * ctype, http_input_stream_t * in, attachments_t ** dest)
{
	/* MIME variables */
	attachments_t * mimeMessage;
	part_t * part, * tmp_part = NULL;
	char * boundary, * root_id;
	/* Check for MIME message */
	if(!(ctype && !strcmp(ctype->type, "multipart/related")))
		return herror_new("mime_get_attachments", MIME_ERROR_NOT_MIME_MESSAGE, "Not a MIME message '%s'", ctype->type);
	boundary = hpairnode_get(ctype->params, "boundary");
	root_id = hpairnode_get(ctype->params, "start");
	if(boundary == NULL) {
		/* TODO (#1#): Handle Error in http form */
		log_error1("'boundary' not set for multipart/related");
		return herror_new("mime_get_attachments", MIME_ERROR_NO_BOUNDARY_PARAM, "'boundary' not set for multipart/related");
	}
	if(root_id == NULL) {
		/* TODO (#1#): Handle Error in http form */
		log_error1("'start' not set for multipart/related");
		return herror_new("mime_get_attachments", MIME_ERROR_NO_START_PARAM, "'start' not set for multipart/related");
	}
	mimeMessage =
	        mime_message_parse(in, root_id, boundary, ".");
	if(mimeMessage == NULL) {
		/* TODO (#1#): Handle Error in http form */
		log_error1("MIME Parse Error");
		return herror_new("mime_get_attachments", MIME_ERROR_PARSE_ERROR, "MIME Parse Error");
	}
	/* Find root */
	if(!mimeMessage->root_part) {
		attachments_free(mimeMessage);
		return herror_new("mime_get_attachments", MIME_ERROR_NO_ROOT_PART, "No root part found!");
	}
	/* delete root_part from list */
	part = mimeMessage->parts;
	while(part) {
		if(part == mimeMessage->root_part) {
			if(tmp_part)
				tmp_part->next = part->next;
			else
				mimeMessage->parts = part->next;
			break;
		}
		tmp_part = part;
		part = part->next;
	}
	*dest = mimeMessage;
	return H_OK;
}

static hrequest_t * hrequest_new(void)
{
	hrequest_t * req;
	if(!(req = (hrequest_t *)malloc(sizeof(hrequest_t)))) {
		log_error2("malloc failed (%s)", strerror(errno));
		return NULL;
	}
	req->method = HTTP_REQUEST_GET;
	req->version = HTTP_1_1;
	req->query = NULL;
	req->header = NULL;
	req->in = NULL;
	req->attachments = NULL;
	req->content_type = NULL;

	return req;
}

static hrequest_t * _hrequest_parse_header(char * data)
{
	hrequest_t * req;
	hpair_t * hpair = NULL, * qpair = NULL, * tmppair = NULL;

	char * tmp;
	char * tmp2;
	char * saveptr;
	char * saveptr2;
	char * saveptr3;
	char * result;
	char * key;
	char * opt_key;
	char * opt_value;
	int firstline = 1;

	req = hrequest_new();
	tmp = data;

	for(;; ) {
		result = (char *)strtok_r(tmp, "\r\n", &saveptr);
		tmp = saveptr;
		if(result == NULL)
			break;
		if(firstline) {
			firstline = 0;
			tmp2 = result;

			/* parse [GET|POST] [PATH] [SPEC] */
			key = (char *)strtok_r(tmp2, " ", &saveptr2);

			/* save method (get or post) */
			tmp2 = saveptr2;
			if(key != NULL) {
				if(!strcmp(key, "POST"))
					req->method = HTTP_REQUEST_POST;
				else if(!strcmp(key, "GET"))
					req->method = HTTP_REQUEST_GET;
				else if(!strcmp(key, "OPTIONS"))
					req->method = HTTP_REQUEST_OPTIONS;
				else if(!strcmp(key, "HEAD"))
					req->method = HTTP_REQUEST_HEAD;
				else if(!strcmp(key, "PUT"))
					req->method = HTTP_REQUEST_PUT;
				else if(!strcmp(key, "DELETE"))
					req->method = HTTP_REQUEST_DELETE;
				else if(!strcmp(key, "TRACE"))
					req->method = HTTP_REQUEST_TRACE;
				else if(!strcmp(key, "CONNECT"))
					req->method = HTTP_REQUEST_CONNECT;
				else
					req->method = HTTP_REQUEST_UNKOWN;
			}
			/* below is key the path and tmp2 the spec */
			key = (char *)strtok_r(tmp2, " ", &saveptr2);

			/* save version */
			tmp2 = saveptr2;
			if(tmp2 != NULL) {
				/* req->spec = (char *) malloc(strlen(tmp2) + 1); strcpy(req->spec,
				   tmp2); */
				if(!strcmp(tmp2, "HTTP/1.0"))
					req->version = HTTP_1_0;
				else
					req->version = HTTP_1_1;
			}
			/*
			 * parse and save path+query parse:
			 * /path/of/target?key1=value1&key2=value2...
			 */
			if(key != NULL) {
				tmp2 = key;
				key = (char *)strtok_r(tmp2, "?", &saveptr2);
				tmp2 = saveptr2;

				/* save path */
				/* req->path = (char *) malloc(strlen(key) + 1); */
				strncpy(req->path, key, REQUEST_MAX_PATH_SIZE);

				/* parse options */
				for(;; ) {
					key = (char *)strtok_r(tmp2, "&", &saveptr2);
					tmp2 = saveptr2;
					if(key == NULL)
						break;
					opt_key = (char *)strtok_r(key, "=", &saveptr3);
					opt_value = saveptr3;
					if(opt_value == NULL)
						opt_value = "";
					/* create option pair */
					if(opt_key != NULL) {
						if(!(tmppair = (hpair_t *)malloc(sizeof(hpair_t)))) {
							log_error2("malloc failed (%s)", strerror(errno));
							return NULL;
						}
						if(req->query == NULL) {
							req->query = qpair = tmppair;
						}
						else {
							qpair->next = tmppair;
							qpair = tmppair;
						}
						/* fill hpairnode_t struct */
						qpair->next = NULL;
						qpair->key = strdup(opt_key);
						qpair->value = strdup(opt_value);

					}
				}
			}
		}
		else {

			/* parse "key: value" */
			/* tmp2 = result; key = (char *) strtok_r(tmp2, ": ", &saveptr2); value
			   = saveptr2; */

			/* create pair */
/*			tmppair = (hpair_t *) malloc(sizeof(hpair_t));*/
			tmppair = hpairnode_parse(result, ":", NULL);
			if(req->header == NULL) {
				req->header = hpair = tmppair;
			}
			else {
				hpair->next = tmppair;
				hpair = tmppair;
			}
			/* fill pairnode_t struct */
			/*
			   hpair->next = NULL; hpair->key = (char *) malloc(strlen(key) + 1);
			   hpair->value = (char *) malloc(strlen(value) + 1);

			   strcpy(hpair->key, key); strcpy(hpair->value, value); */
		}
	}
	/* Check Content-type */
	tmp = hpairnode_get_ignore_case(req->header, HEADER_CONTENT_TYPE);
	if(tmp != NULL)
		req->content_type = content_type_new(tmp);
	return req;
}

void hrequest_free(hrequest_t * req)
{
	if(req) {
		hpairnode_free_deep(req->header);
		hpairnode_free_deep(req->query);
		if(req->in)
			http_input_stream_free(req->in);
		if(req->content_type)
			content_type_free(req->content_type);
		if(req->attachments)
			attachments_free(req->attachments);
		free(req);
	}
}

herror_t hrequest_new_from_socket(hsocket_t * sock, hrequest_t ** out)
{
	int i, readed;
	herror_t status;
	hrequest_t * req;
	char buffer[MAX_HEADER_SIZE+1];
	attachments_t * mimeMessage;

	memset(buffer, 0, MAX_HEADER_SIZE);
	/* Read header */
	for(i = 0; i < MAX_HEADER_SIZE; i++) {
		if((status = hsocket_read(sock, (byte_t *)&(buffer[i]), 1, 1, &readed)) != H_OK) {
			log_error2("hsocket_read failed (%s)", herror_message(status));
			return status;
		}
		buffer[i+1] = '\0'; /* for strmp */

/*    log_error2("buffer=\"%s\"", buffer); */
		if(i > 3) {
			if(!strcmp(&(buffer[i-1]), "\n\n") || !strcmp(&(buffer[i-2]), "\n\r\n"))
				break;
		}
	}
	/* Create response */
	req = _hrequest_parse_header(buffer);

	/* Create input stream */
	req->in = http_input_stream_new(sock, req->header);
	/* Check for MIME message */
	if((req->content_type &&
	    !strcmp(req->content_type->type, "multipart/related"))) {
		status = mime_get_attachments(req->content_type, req->in, &mimeMessage);
		if(status != H_OK) {
			/* TODO (#1#): Handle error */
			hrequest_free(req);
			return status;
		}
		else {
			req->attachments = mimeMessage;
			req->in = http_input_stream_new_from_file(mimeMessage->root_part->filename);
		}
	}
	*out = req;
	return H_OK;
}

static hresponse_t * hresponse_new()
{
	hresponse_t * res = (hresponse_t *)malloc(sizeof(hresponse_t));
	/* create response object */
	if(!res) {
		log_error2("malloc failed (%s)", strerror(errno));
	}
	else {
		res->version = HTTP_1_1;
		res->errcode = -1;
		res->desc[0] = '\0';
		res->header = NULL;
		res->in = NULL;
		res->content_type = NULL;
		res->attachments = NULL;
	}
	return res;
}

static hresponse_t * _hresponse_parse_header(const char * buffer)
{
	char * s1, * s2;
	/* create response object */
	hresponse_t * res = hresponse_new();
	/* *** parse spec *** */
	/* [HTTP/1.1 | 1.2] [CODE] [DESC] */

	/* stage 1: HTTP spec */
	char * str = (char *)strtok_r((char *)buffer, " ", &s2);
	s1 = s2;
	if(str == NULL) {
		log_error1("Parse error reading HTTP spec");
		return NULL;
	}
	if(!strcmp(str, "HTTP/1.0"))
		res->version = HTTP_1_0;
	else
		res->version = HTTP_1_1;
	/* stage 2: http code */
	str = (char *)strtok_r(s1, " ", &s2);
	s1 = s2;
	if(str == NULL) {
		log_error1("Parse error reading HTTP code");
		return NULL;
	}
	res->errcode = atoi(str);
	/* stage 3: description text */
	str = (char *)strtok_r(s1, "\r\n", &s2);
	s1 = s2;
	if(str == NULL) {
		log_error1("Parse error reading HTTP description");
		return NULL;
	}
	/*	res->desc = (char *) malloc(strlen(str) + 1);*/
	strncpy(res->desc, str, RESPONSE_MAX_DESC_SIZE);
	res->desc[strlen(str)] = '\0';

	/* *** parse header *** */
	/* [key]: [value] */
	for(;; ) {
		str = strtok_r(s1, "\n", &s2);
		s1 = s2;
		/* check if header ends without body */
		if(str == NULL) {
			return res;
		}
		/* check also for end of header */
		if(!strcmp(str, "\r")) {
			break;
		}
		str[strlen(str)-1] = '\0';
		res->header = hpairnode_parse(str, ":", res->header);
	}
	/* Check Content-type */
	str = hpairnode_get(res->header, HEADER_CONTENT_TYPE);
	if(str != NULL)
		res->content_type = content_type_new(str);
	/* return response object */
	return res;
}

herror_t hresponse_new_from_socket(hsocket_t * sock, hresponse_t ** out)
{
	int i = 0, count;
	herror_t status;
	hresponse_t * res;
	attachments_t * mimeMessage;
	char buffer[MAX_HEADER_SIZE+1];

read_header:                   /* for errorcode: 100 (continue) */
	/* Read header */
	while(i < MAX_HEADER_SIZE) {
		if((status = hsocket_read(sock, (byte_t *)&(buffer[i]), 1, 1, &count)) != H_OK) {
			log_error1("Socket read error");
			return status;
		}
		buffer[i+1] = '\0'; /* for strmp */
		if(i > 3) {
			if(!strcmp(&(buffer[i-1]), "\n\n") || !strcmp(&(buffer[i-2]), "\n\r\n"))
				break;
		}
		i++;
	}
	/* Create response */
	res = _hresponse_parse_header(buffer);
	if(res == NULL) {
		log_error1("Header parse error");
		return herror_new("hresponse_new_from_socket", GENERAL_HEADER_PARSE_ERROR, "Can not parse response header");
	}
	/* Chec for Errorcode: 100 (continue) */
	if(res->errcode == 100) {
		hresponse_free(res);
		i = 0;
		goto read_header;
	}
	/* Create input stream */
	res->in = http_input_stream_new(sock, res->header);
	/* Check for MIME message */
	if((res->content_type && !strcmp(res->content_type->type, "multipart/related"))) {
		status = mime_get_attachments(res->content_type, res->in, &mimeMessage);
		if(status != H_OK) {
			/* TODO (#1#): Handle error */
			hresponse_free(res);
			return status;
		}
		else {
			res->attachments = mimeMessage;
			http_input_stream_free(res->in);
			res->in = http_input_stream_new_from_file(mimeMessage->root_part->filename);
			if(!res->in) {
				/* TODO (#1#): Handle error */
			}
			else {
				/* res->in->deleteOnExit = 1; */
			}
		}
	}
	*out = res;
	return H_OK;
}

void hresponse_free(hresponse_t * res)
{
	if(res) {
		if(res->header)
			hpairnode_free_deep(res->header);
		if(res->in)
			http_input_stream_free(res->in);
		if(res->content_type)
			content_type_free(res->content_type);
		if(res->attachments)
			attachments_free(res->attachments);
		free(res);
	}
}

typedef struct _conndata {
	volatile int flag;
	hsocket_t sock;
#ifdef WIN32
	HANDLE tid;
#else
	pthread_t tid;
	pthread_attr_t attr;
#endif
	time_t atime;
} conndata_t;

#define CONNECTION_FREE         0
#define CONNECTION_IN_USE       1

/*
 * -----------------------------------------------------
 * nano httpd
 * internally globals
 * -----------------------------------------------------
 */
static volatile int _httpd_run = 1;
static hsocket_t _httpd_socket;
static int _httpd_port = 10000;
static int _httpd_max_connections = 20;
static int _httpd_timeout = 10;
static hservice_t * _httpd_services_default = NULL;
static hservice_t * _httpd_services_head = NULL;
static hservice_t * _httpd_services_tail = NULL;
static conndata_t * _httpd_connection;
static int _httpd_enable_service_list = 0;
static int _httpd_enable_statistics = 0;

#ifdef WIN32
static DWORD _httpd_terminate_signal = CTRL_C_EVENT;
static int _httpd_max_idle = 120;
static void WSAReaper(void * x);
HANDLE _httpd_connection_lock;
LPCTSTR _httpd_connection_lock_str;
 #define strncasecmp(s1, s2, num) strncmp(s1, s2, num)
 #define snprintf(buffer, num, s1, s2) sprintf(buffer, s1, s2)
#else
static int _httpd_terminate_signal = SIGINT;
static sigset_t thrsigset;
static pthread_mutex_t _httpd_connection_lock;
#endif

static void _httpd_parse_arguments(int argc, char ** argv)
{
	int i;
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i-1], NHTTPD_ARG_PORT)) {
			_httpd_port = atoi(argv[i]);
		}
		else if(!strcmp(argv[i-1], NHTTPD_ARG_TERMSIG)) {
			_httpd_terminate_signal = atoi(argv[i]);
		}
		else if(!strcmp(argv[i-1], NHTTPD_ARG_MAXCONN)) {
			_httpd_max_connections = atoi(argv[i]);
		}
		else if(!strcmp(argv[i-1], NHTTPD_ARG_TIMEOUT)) {
			_httpd_timeout = atoi(argv[i]);
		}
	}
	log_verbose2("socket bind to port '%d'", _httpd_port);
}

static void _httpd_connection_slots_init(void)
{
	int i;
#ifdef WIN32
	_httpd_connection_lock = CreateMutex( NULL, TRUE, _httpd_connection_lock_str );
#else
	pthread_mutex_init(&_httpd_connection_lock, NULL);
#endif
	_httpd_connection = (conndata_t *)calloc(_httpd_max_connections, sizeof(conndata_t));
	for(i = 0; i < _httpd_max_connections; i++)
		hsocket_init(&(_httpd_connection[i].sock));
}

static void _httpd_register_builtin_services(void)
{
#if 0 // @sobolev {
	if(_httpd_enable_service_list)
		;               /* httpd_register("/httpd/services", _httpd_list_services); */
	if(_httpd_enable_statistics)
		;               /* httpd_register("/httpd/statistics", _httpd_statistics); */
#endif // } 0 @sobolev
}

/*
 * -----------------------------------------------------
 * FUNCTION: httpd_init
 * NOTE: This will be called from soap_server_init_args()
 * -----------------------------------------------------
 */
herror_t httpd_init(int argc, char * argv[])
{
	herror_t status;
	_httpd_parse_arguments(argc, argv);
	if((status = hsocket_module_init(argc, argv)) != H_OK)
		return status;
	log_verbose2("socket bind to port '%d'", _httpd_port);
	_httpd_connection_slots_init();
	_httpd_register_builtin_services();
#ifdef WIN32
	/*
	   if (_beginthread (WSAReaper, 0, NULL) == -1) { log_error1 ("Winsock
	   reaper thread failed to start"); return herror_new("httpd_init",
	   THREAD_BEGIN_ERROR, "_beginthread() failed while starting WSAReaper"); }
	 */
#endif
	if((status = hsocket_init(&_httpd_socket)) != H_OK) {
		log_error2("hsocket_init failed (%s)", herror_message(status));
		return status;
	}
	return hsocket_bind(&_httpd_socket, _httpd_port);
}

/*
 * -----------------------------------------------------
 * FUNCTION: httpd_register
 * -----------------------------------------------------
 */
int httpd_register_secure(const char * ctx, httpd_service func, httpd_auth auth)
{
	hservice_t * service;
	if(!(service = (hservice_t *)malloc(sizeof(hservice_t)))) {
		log_error2("malloc failed (%s)", strerror(errno));
		return -1;
	}
	service->next = NULL;
	service->auth = auth;
	service->func = func;
	strcpy(service->ctx, ctx);

	log_verbose3("register service:t(%p):%s", service, SAVE_STR(ctx));
	if(_httpd_services_head == NULL) {
		_httpd_services_head = _httpd_services_tail = service;
	}
	else {
		_httpd_services_tail->next = service;
		_httpd_services_tail = service;
	}
	return 1;
}

int httpd_register(const char * ctx, httpd_service service)
{
	return httpd_register_secure(ctx, service, NULL);
}

int httpd_register_default_secure(const char * ctx, httpd_service service, httpd_auth auth)
{
	int ret = httpd_register_secure(ctx, service, auth);
	/* XXX: this is broken, but working */
	_httpd_services_default = _httpd_services_tail;
	return ret;
}

int httpd_register_default(const char * ctx, httpd_service service)
{
	return httpd_register_default_secure(ctx, service, NULL);
}

int httpd_get_port(void)
{
	return _httpd_port;
}

int httpd_get_timeout(void)
{
	return _httpd_timeout;
}

void httpd_set_timeout(int t)
{
	_httpd_timeout = t;
}

const char * httpd_get_protocol(void)
{
	return hssl_enabled() ? "https" : "http";
}

/*--------------------------------------------------
   FUNCTION: httpd_get_conncount
   ----------------------------------------------------*/
int httpd_get_conncount(void)
{
	int i;
	int c = 0;
	for(i = 0; i<_httpd_max_connections; i++) {
		if(_httpd_connection[i].flag == CONNECTION_IN_USE) {
			c++;
		}
	}
	return c;
}

/*
 * -----------------------------------------------------
 * FUNCTION: httpd_services
 * -----------------------------------------------------
 */
hservice_t * httpd_services(void)
{
	return _httpd_services_head;
}

/*
 * -----------------------------------------------------
 * FUNCTION: httpd_services
 * -----------------------------------------------------
 */
static void hservice_free(hservice_t * service)
{
	free(service);
}

/*
 * -----------------------------------------------------
 * FUNCTION: httpd_find_service
 * -----------------------------------------------------
 */
static hservice_t * httpd_find_service(const char * ctx)
{
	hservice_t * cur = _httpd_services_head;
	while(cur != NULL) {
		if(!strcmp(cur->ctx, ctx)) {
			return cur;
		}
		cur = cur->next;
	}
	return _httpd_services_default;
}

/*
 * -----------------------------------------------------
 * FUNCTION: httpd_response_set_content_type
 * -----------------------------------------------------
 */
void httpd_response_set_content_type(httpd_conn_t * res, const char * content_type)
{
	strncpy(res->content_type, content_type, 25);
}

/*
 * -----------------------------------------------------
 * FUNCTION: httpd_response_send_header
 * -----------------------------------------------------
 */
herror_t httpd_send_header(httpd_conn_t * res, int code, const char * text)
{
	struct tm stm;
	time_t nw;
	char buffer[255];
	char header[1024];
	hpair_t * cur;
	herror_t status;

	/* set status code */
	sprintf(header, "HTTP/1.1 %d %s\r\n", code, text);

	/* set date */
	nw = time(NULL);
	localtime_r(&nw, &stm);
	strftime(buffer, 255, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", &stm);
	strcat(header, buffer);

	/* set content-type */
	/*
	 * if (res->content_type[0] == '\0') { strcat(header, "Content-Type:
	 * text/html\r\n"); } else { sprintf(buffer, "Content-Type: %s\r\n",
	 * res->content_type); strcat(header, buffer); }
	 */

	/* set server name */
	strcat(header, "Server: Nano HTTPD library\r\n");

	/* set _httpd_connection status */
	/* strcat (header, "Connection: close\r\n"); */

	/* add pairs */
	for(cur = res->header; cur; cur = cur->next) {
		sprintf(buffer, "%s: %s\r\n", cur->key, cur->value);
		strcat(header, buffer);
	}
	/* set end of header */
	strcat(header, "\r\n");
	/* send header */
	if((status = hsocket_nsend(res->sock, (const byte_t *)header, strlen(header))) != H_OK)
		return status;
	res->out = http_output_stream_new(res->sock, res->header);
	return H_OK;
}

herror_t httpd_send_internal_error(httpd_conn_t * conn, const char * errmsg)
{
	const char * template1 = "<html><body><h3>Error!</h3><hr> Message: '%s' </body></html>\r\n";
	char buffer[4064];
	char buflen[5];
	sprintf(buffer, template1, errmsg);
	snprintf(buflen, 5, "%d", strlen(buffer));
	httpd_set_header(conn, HEADER_CONTENT_LENGTH, buflen);
	httpd_send_header(conn, 500, "INTERNAL");
	return http_output_stream_write_string(conn->out, buffer);
}

/*
 * -----------------------------------------------------
 * FUNCTION: httpd_request_print
 * -----------------------------------------------------
 */
static void httpd_request_print(hrequest_t * req)
{
	hpair_t * pair;
	log_verbose1("++++++ Request +++++++++");
	log_verbose2(" Method : '%s'",
		(req->method == HTTP_REQUEST_POST) ? "POST" : "GET");
	log_verbose2(" Path   : '%s'", req->path);
	log_verbose2(" Spec   : '%s'",
		(req->version == HTTP_1_0) ? "HTTP/1.0" : "HTTP/1.1");
	log_verbose1(" Parsed query string :");

	for(pair = req->query; pair; pair = pair->next)
		log_verbose3(" %s = '%s'", pair->key, pair->value);
	log_verbose1(" Parsed header :");
	for(pair = req->header; pair; pair = pair->next)
		log_verbose3(" %s = '%s'", pair->key, pair->value);
	log_verbose1("++++++++++++++++++++++++");
}

httpd_conn_t * httpd_new(hsocket_t * sock)
{
	httpd_conn_t * conn = (httpd_conn_t *)malloc(sizeof(httpd_conn_t));
	if(!conn) {
		log_error2("malloc failed (%s)", strerror(errno));
	}
	else {
		conn->sock = sock;
		conn->out = NULL;
		conn->content_type[0] = '\0';
		conn->header = NULL;
	}
	return conn;
}

void httpd_free(httpd_conn_t * conn)
{
	if(conn) {
		if(conn->out)
			http_output_stream_free(conn->out);
		if(conn->header)
			hpairnode_free_deep(conn->header);
		free(conn);
	}
}

void do_req_timeout(int signum)
{
/*
    struct sigaction req_timeout;
    memset(&req_timeout, 0, sizeof(&req_timeout));
    req_timeout.sa_handler=SIG_IGN;
    sigaction(SIGALRM, &req_timeout, NULL);
 */

	/* XXX this is not real pretty, is there a better way? */
	log_verbose1("Thread timeout.");
#ifdef _MT
 #ifdef WIN32
	_endthread();
 #else
	pthread_exit(0);
 #endif
#endif
}

static int _httpd_decode_authorization(const char * value, char ** user, char ** pass)
{
	unsigned char * tmp, * tmp2;
	size_t len = strlen(value)*2;
	if(!(tmp = (unsigned char *)calloc(1, len))) {
		log_error2("calloc failed (%s)", strerror(errno));
		return -1;
	}
	value = strstr(value, " ")+1;
	log_verbose2("Authorization (base64) = \"%s\"", value);
	base64_decode((const unsigned char *)value, tmp);
	log_verbose2("Authorization (ascii) = \"%s\"", tmp);
	if((tmp2 = (unsigned char *)strstr((const char *)tmp, ":"))) {
		*tmp2++ = '\0';
		*pass = strdup((const char *)tmp2);
	}
	else {
		*pass = strdup("");
	}
	*user = strdup((const char *)tmp);
	free(tmp);
	return 0;
}

static int _httpd_authenticate_request(hrequest_t * req, httpd_auth auth)
{
	char * user, * pass;
	char * authorization;
	int ret;
	if(!auth)
		return 1;
	if(!(authorization = hpairnode_get_ignore_case(req->header, HEADER_AUTHORIZATION))) {
		log_debug2("%s header not set", HEADER_AUTHORIZATION);
		return 0;
	}
	if(_httpd_decode_authorization(authorization, &user, &pass)) {
		log_error1("httpd_base64_decode_failed");
		return 0;
	}
	if((ret = auth(req, user, pass)))
		log_debug2("Access granted for user=\"%s\"", user);
	else
		log_info2("Authentication failed for user=\"%s\"", user);
	free(user);
	free(pass);
	return ret;
}

/*
 * -----------------------------------------------------
 * FUNCTION: httpd_session_main
 * -----------------------------------------------------
 */
#ifdef WIN32
static unsigned _stdcall httpd_session_main(void * data)
#else
static void * httpd_session_main(void * data)
#endif
{
	hrequest_t * req;       /* only for test */
	httpd_conn_t * rconn;
	hservice_t * service;
	herror_t status;
	int done;
	conndata_t * conn = (conndata_t *)data;
	log_verbose2("starting new httpd session on socket %d", conn->sock);
	rconn = httpd_new(&(conn->sock));
	done = 0;
	while(!done) {
		log_verbose3("starting HTTP request on socket %p (%d)", conn->sock, conn->sock.sock);

		/* XXX: only used in WSAreaper */
		conn->atime = time(NULL);
		if((status = hrequest_new_from_socket(&(conn->sock), &req)) != H_OK) {
			int code;

			switch((code = herror_code(status))) {
			    case HSOCKET_ERROR_SSLCLOSE:
			    case HSOCKET_ERROR_RECEIVE:
				log_error2("hrequest_new_from_socket failed (%s)",
					herror_message(status));
				break;
			    default:
				httpd_send_internal_error(rconn, herror_message(status));
				break;
			}
			herror_release(status);
			done = 1;
		}
		else {
			char * conn_str;

			httpd_request_print(req);

			conn_str = hpairnode_get_ignore_case(req->header, HEADER_CONNECTION);
			if(conn_str && strncasecmp(conn_str, "close", 6) == 0)
				done = 1;
			if(!done)
				done = req->version == HTTP_1_0 ? 1 : 0;
			if((service = httpd_find_service(req->path))) {
				log_verbose3("service '%s' for '%s' found", service->ctx, req->path);
				if(_httpd_authenticate_request(req, service->auth)) {
					if(service->func != NULL) {
						service->func(rconn, req);
						if(rconn->out &&
						   rconn->out->type == HTTP_TRANSFER_CONNECTION_CLOSE) {
							log_verbose1("Connection close requested");
							done = 1;
						}
					}
					else {
						char buffer[256];
						sprintf(buffer, "service '%s' not registered properly (func == NULL)", req->path);
						log_verbose1(buffer);
						httpd_send_internal_error(rconn, buffer);
					}
				}
				else {
					char * p_template =
					        "<html>"
					        "<head>"
					        "<title>Unauthorized</title>"
					        "</head>"
					        "<body>"
					        "<h1>Unauthorized request logged</h1>" "</body>" "</html>";

					httpd_set_header(rconn, HEADER_WWW_AUTHENTICATE, "Basic realm=\"nanoHTTP\"");
					httpd_send_header(rconn, 401, "Unauthorized");
					http_output_stream_write_string(rconn->out, p_template);
					done = 1;
				}
			}
			else {
				char buffer[256];
				sprintf(buffer, "no service for '%s' found", req->path);
				log_verbose1(buffer);
				httpd_send_internal_error(rconn, buffer);
				done = 1;
			}
			hrequest_free(req);
		}
	}
	httpd_free(rconn);

	hsocket_close(&(conn->sock));

#ifdef WIN32
	CloseHandle((HANDLE)conn->tid);
#else
	pthread_attr_destroy(&(conn->attr));
#endif
	conn->flag = CONNECTION_FREE;
#ifdef WIN32
	#ifdef  _MT // @sobolev
		_endthread();
	#endif // @sobolev
	return 0;
#else
	/* pthread_exits automagically */
	return NULL;
#endif
}

int httpd_set_header(httpd_conn_t * conn, const char * key, const char * value)
{
	if(conn == NULL) {
		log_warn1("Connection object is NULL");
	}
	else {
		for(hpair_t * p = conn->header; p; p = p->next) {
			if(p->key && !strcmp(p->key, key)) {
				free(p->value);
				p->value = strdup(value);
				return 1;
			}
		}
		conn->header = hpairnode_new(key, value, conn->header);
	}
	return 0;
}

void httpd_set_headers(httpd_conn_t * conn, hpair_t * header)
{
	while(header) {
		httpd_set_header(conn, header->key, header->value);
		header = header->next;
	}
}

int httpd_add_header(httpd_conn_t * conn, const char * key, const char * value)
{
	if(!conn) {
		log_warn1("Connection object is NULL");
		return 0;
	}
	else {
		conn->header = hpairnode_new(key, value, conn->header);
		return 1;
	}
}

void httpd_add_headers(httpd_conn_t * conn, const hpair_t * values)
{
	if(!conn) {
		log_warn1("Connection object is NULL");
	}
	else {
		while(values) {
			httpd_add_header(conn, values->key, values->value);
			values = values->next;
		}
	}
}

/*
 * -----------------------------------------------------
 * FUNCTION: httpd_term
 * -----------------------------------------------------
 */
#ifdef WIN32
BOOL WINAPI httpd_term(DWORD sig)
{
	/* log_debug2 ("Got signal %d", sig); */
	if(sig == _httpd_terminate_signal)
		_httpd_run = 0;
	return TRUE;
}

#else
void httpd_term(int sig)
{
	log_debug2("Got signal %d", sig);
	if(sig == _httpd_terminate_signal)
		_httpd_run = 0;
}

#endif

/*
 * -----------------------------------------------------
 * FUNCTION: _httpd_register_signal_handler
 * -----------------------------------------------------
 */
static void _httpd_register_signal_handler(void)
{
	log_verbose2("registering termination signal handler (SIGNAL:%d)", _httpd_terminate_signal);
#ifdef WIN32
	if(SetConsoleCtrlHandler((PHANDLER_ROUTINE)httpd_term, TRUE) == FALSE) {
		log_error1("Unable to install console event handler!");
	}
#else
	signal(_httpd_terminate_signal, httpd_term);
#endif
}

/*--------------------------------------------------
   FUNCTION: _httpd_wait_for_empty_conn
   ----------------------------------------------------*/
static conndata_t * _httpd_wait_for_empty_conn(void)
{
	int i;
#ifdef WIN32
	WaitForSingleObject(_httpd_connection_lock, INFINITE);
#else
	pthread_mutex_lock(&_httpd_connection_lock);
#endif
	for(i = 0;; i++) {
		if(!_httpd_run) {

#ifdef WIN32
			ReleaseMutex(_httpd_connection_lock);
#else
			pthread_mutex_unlock(&_httpd_connection_lock);
#endif
			return NULL;
		}
		if(i >= _httpd_max_connections) {
			system_sleep(1);
			i = -1;
		}
		else if(_httpd_connection[i].flag == CONNECTION_FREE) {
			_httpd_connection[i].flag = CONNECTION_IN_USE;
			break;
		}
	}
#ifdef WIN32
	ReleaseMutex(_httpd_connection_lock);
#else
	pthread_mutex_unlock(&_httpd_connection_lock);
#endif
	return &_httpd_connection[i];
}

/*
 * -----------------------------------------------------
 * FUNCTION: _httpd_start_thread
 * -----------------------------------------------------
 */
static void _httpd_start_thread(conndata_t * conn)
{
#ifdef MT
 #ifdef WIN32
	unsigned int err = 0;
	conn->tid = (HANDLE)_beginthreadex(NULL, 65535, httpd_session_main, conn, 0, &err);
 #else
	pthread_attr_init(&(conn->attr));
  #ifdef PTHREAD_CREATE_DETACHED
	pthread_attr_setdetachstate(&(conn->attr), PTHREAD_CREATE_DETACHED);
  #endif
	pthread_sigmask(SIG_BLOCK, &thrsigset, NULL);
	if((err = pthread_create(&(conn->tid), &(conn->attr), httpd_session_main, conn)))
		log_error2("pthread_create failed (%s)", strerror(err));
 #endif
#endif
}

/*
 * -----------------------------------------------------
 * FUNCTION: httpd_run
 * -----------------------------------------------------
 */
herror_t httpd_run(void)
{
	struct timeval timeout;
	conndata_t * conn;
	herror_t err;
	fd_set fds;
	log_verbose1("starting run routine");
#ifndef WIN32
	sigemptyset(&thrsigset);
	sigaddset(&thrsigset, SIGALRM);
#endif
	_httpd_register_signal_handler();
	if((err = hsocket_listen(&_httpd_socket)) != H_OK) {
		log_error2("hsocket_listen failed (%s)", herror_message(err));
		return err;
	}
	while(_httpd_run) {
		conn = _httpd_wait_for_empty_conn();
		if(!_httpd_run)
			break;
		/* Wait for a socket to accept */
		while(_httpd_run) {
			/* set struct timeval to the proper timeout */
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			/* zero and set file descriptior */
			FD_ZERO(&fds);
			FD_SET(_httpd_socket.sock, &fds);
			/* select socket descriptor */
			switch(select(_httpd_socket.sock+1, &fds, NULL, NULL, &timeout)) {
			    case 0: /* descriptor is not ready */
					continue;
			    case -1: /* got a signal? */
					continue;
			    default: /* no nothing */
					break;
			}
			if(FD_ISSET(_httpd_socket.sock, &fds)) {
				break;
			}
		}
		/* check signal status */
		if(!_httpd_run)
			break;
		if((err = hsocket_accept(&_httpd_socket, &(conn->sock))) != H_OK) {
			log_error2("hsocket_accept failed (%s)", herror_message(err));
			hsocket_close(&(conn->sock));
			continue;
		}
		_httpd_start_thread(conn);
	}
	return 0;
}

void httpd_destroy()
{
	for(hservice_t * cur = _httpd_services_head; cur;) {
		hservice_t * tmp = cur->next;
		hservice_free(cur);
		cur = tmp;
	}
	hsocket_module_destroy();
	free(_httpd_connection);
}

#ifdef WIN32

static void WSAReaper(void * x)
{
	short int connections;
	short int i;
	char junk[10];
	int rc;
	time_t ctime;
	for(;; ) {
		connections = 0;
		ctime = time((time_t)0);
		for(i = 0; i < _httpd_max_connections; i++) {
			if(_httpd_connection[i].tid == 0)
				continue;
			GetExitCodeThread((HANDLE)_httpd_connection[i].tid, (PDWORD)&rc);
			if(rc != STILL_ACTIVE)
				continue;
			connections++;
			if((ctime-_httpd_connection[i].atime < _httpd_max_idle) || (_httpd_connection[i].atime == 0))
				continue;
			log_verbose3("Reaping socket %u from (runtime ~= %d seconds)", _httpd_connection[i].sock, ctime-_httpd_connection[i].atime);
			shutdown(_httpd_connection[i].sock.sock, 2);
			while(recv(_httpd_connection[i].sock.sock, junk, sizeof(junk), 0) > 0) {
			}
			;
			closesocket(_httpd_connection[i].sock.sock);
			_httpd_connection[i].sock.sock = 0;
			TerminateThread(_httpd_connection[i].tid, (DWORD)&rc);
			CloseHandle(_httpd_connection[i].tid);
			memset((char *)&_httpd_connection[i], 0, sizeof(_httpd_connection[i]));
		}
		Sleep(100);
	}
}

#endif

unsigned char * httpd_get_postdata(httpd_conn_t * conn, hrequest_t * req, long * received, long max)
{
	char * content_length_str;
	long   content_length = 0;
	unsigned char * postdata = NULL;
	if(req->method == HTTP_REQUEST_POST) {
		content_length_str = hpairnode_get_ignore_case(req->header, HEADER_CONTENT_LENGTH);
		if(content_length_str != NULL)
			content_length = atol(content_length_str);
	}
	else {
		log_warn1("Not a POST method");
		return NULL;
	}
	if(content_length > max && max != -1)
		return NULL;
	if(content_length == 0) {
		*received = 0;
		if(!(postdata = (unsigned char *)malloc(1))) {
			log_error2("malloc failed (%s)", strerror(errno));
			return NULL;
		}
		else {
			postdata[0] = '\0';
			return postdata;
		}
	}
	if(!(postdata = (unsigned char *)malloc(content_length+1))) {
		log_error2("malloc failed (%)", strerror(errno));
		return NULL;
	}
	if(http_input_stream_read(req->in, postdata, (int)content_length) > 0) {
		*received = content_length;
		postdata[content_length] = '\0';
		return postdata;
	}
	free(postdata);
	return NULL;
}
/*
   MIME support httpd_mime_* function set
 */
static void _httpd_mime_get_boundary(httpd_conn_t * conn, char * dest)
{
	sprintf(dest, "---=.Part_NH_%p", conn);
	log_verbose2("boundary= \"%s\"", dest);
}
/**
   Begin MIME multipart/related POST
   Returns: H_OK  or error flag
 */
herror_t httpd_mime_send_header(httpd_conn_t * conn, const char * related_start,
	const char * related_start_info,
	const char * related_type, int code, const char * text)
{
	char buffer[300];
	char temp[250];
	char boundary[250];
	/* Set Content-type Set multipart/related parameter type=..; start=.. ;
	   start-info= ..; boundary=... using sprintf instead of snprintf because
	   visual c does not support snprintf */

	sprintf(buffer, "multipart/related;");
	if(related_type) {
		snprintf(temp, 75, " type=\"%s\";", related_type);
		strcat(buffer, temp);
	}
	if(related_start) {
		snprintf(temp, 250, " start=\"%s\";", related_start);
		strcat(buffer, temp);
	}
	if(related_start_info) {
		snprintf(temp, 250, " start-info=\"%s\";", related_start_info);
		strcat(buffer, temp);
	}
	_httpd_mime_get_boundary(conn, boundary);
	snprintf(temp, 250, " boundary=\"%s\"", boundary);
	strcat(buffer, temp);
	httpd_set_header(conn, HEADER_CONTENT_TYPE, buffer);
	return httpd_send_header(conn, code, text);
}
/**
   Send boundary and part header and continue
   with next part
 */
herror_t httpd_mime_next(httpd_conn_t * conn, const char * content_id,
	const char * content_type, const char * transfer_encoding)
{
	herror_t status;
	char buffer[512];
	char boundary[75];
	/* Get the boundary string */
	_httpd_mime_get_boundary(conn, boundary);
	sprintf(buffer, "\r\n--%s\r\n", boundary);
	/* Send boundary */
	status = http_output_stream_write(conn->out, (const byte_t *)buffer, strlen(buffer));
	if(status != H_OK)
		return status;
	/* Send Content header */
	sprintf(buffer, "%s: %s\r\n%s: %s\r\n%s: %s\r\n\r\n",
		HEADER_CONTENT_TYPE, content_type ? content_type : "text/plain",
		HEADER_CONTENT_TRANSFER_ENCODING,
		transfer_encoding ? transfer_encoding : "binary",
		HEADER_CONTENT_ID,
		content_id ? content_id : "<content-id-not-set>");
	status = http_output_stream_write(conn->out, (const byte_t *)buffer, strlen(buffer));
	return status;
}
/**
   Send boundary and part header and continue
   with next part
 */
herror_t httpd_mime_send_file(httpd_conn_t * conn, const char * content_id,
	const char * content_type, const char * transfer_encoding,
	const char * filename)
{
	byte_t buffer[MAX_FILE_BUFFER_SIZE];
	herror_t status;
	FILE * fd;
	size_t size;
	if((fd = fopen(filename, "rb")) == NULL)
		return herror_new("httpd_mime_send_file", FILE_ERROR_OPEN, "Can not open file '%d'", filename);
	status = httpd_mime_next(conn, content_id, content_type, transfer_encoding);
	if(status != H_OK) {
		fclose(fd);
		return status;
	}
	while(!feof(fd)) {
		size = fread(buffer, 1, MAX_FILE_BUFFER_SIZE, fd);
		if(size == -1) {
			fclose(fd);
			return herror_new("httpd_mime_send_file", FILE_ERROR_READ, "Can not read from file '%d'", filename);
		}
		if((status = http_output_stream_write(conn->out, buffer, size)) != H_OK) {
			fclose(fd);
			return status;
		}
	}
	fclose(fd);
	return H_OK;
}
/**
   Finish MIME request
   Returns: H_OK  or error flag
 */
herror_t httpd_mime_end(httpd_conn_t * conn)
{
	herror_t status;
	char buffer[512];
	char boundary[75];
	/* Get the boundary string */
	_httpd_mime_get_boundary(conn, boundary);
	sprintf(buffer, "\r\n--%s--\r\n\r\n", boundary);
	/* Send boundary */
	status = http_output_stream_write(conn->out, (const byte_t *)buffer, strlen(buffer));
	if(status != H_OK)
		return status;
	/* Flush put stream */
	status = http_output_stream_flush(conn->out);
	return status;
}

#ifdef WIN32
static inline void _hsocket_module_sys_init(int argc, char ** argv)
{
	/* @sobolev (SLIB делает это)
	struct WSAData info;
	WSAStartup(MAKEWORD(2, 2), &info);
	*/
}

static inline void _hsocket_module_sys_destroy(void)
{
	// @sobolev WSACleanup();
}

#else
static inline void _hsocket_module_sys_init(int argc, char ** argv)
{
}

static inline void _hsocket_module_sys_destroy(void)
{
}

#endif

/*--------------------------------------------------
   FUNCTION: hsocket_module_init
   NOTE: This will be called from httpd_init()
        for server and from httpc_init() for client
   ----------------------------------------------------*/
herror_t hsocket_module_init(int argc, char ** argv)
{
	_hsocket_module_sys_init(argc, argv);
	return hssl_module_init(argc, argv);
}

/*--------------------------------------------------
   FUNCTION: hsocket_module_destroy
   ----------------------------------------------------*/
void hsocket_module_destroy()
{
	_hsocket_module_sys_destroy();
}

/*--------------------------------------------------
   FUNCTION: hsocket_init
   ----------------------------------------------------*/
herror_t hsocket_init(hsocket_t * sock)
{
	memset(sock, 0, sizeof(hsocket_t));
	sock->sock = HSOCKET_FREE;
	return H_OK;
}

/*--------------------------------------------------
   FUNCTION: hsocket_free
   ----------------------------------------------------*/
void hsocket_free(hsocket_t * sock)
{
	/* nop */
}

/*--------------------------------------------------
   FUNCTION: hsocket_open
   ----------------------------------------------------*/
herror_t hsocket_open(hsocket_t * dsock, const char * hostname, int port, int ssl)
{
	struct sockaddr_in address;
	struct hostent * host;
	char * ip;
	if((dsock->sock = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
		return herror_new("hsocket_open", HSOCKET_ERROR_CREATE, "Socket error (%s)", strerror(errno));
	/* Get host data */
	if(!(host = gethostbyname(hostname)))
		return herror_new("hsocket_open", HSOCKET_ERROR_GET_HOSTNAME, "Socket error (%s)", strerror(errno));
	ip = inet_ntoa(*(struct in_addr *)*host->h_addr_list);
	address.sin_addr.s_addr = inet_addr(ip);
	/* set server addresss */
	address.sin_family = host->h_addrtype;
	address.sin_port = htons((unsigned short)port);
	log_verbose4("Opening %s://%s:%i", ssl ? "https" : "http", hostname, port);
	/* connect to the server */
	if(connect(dsock->sock, (struct sockaddr *)&address, sizeof(address)) != 0)
		return herror_new("hsocket_open", HSOCKET_ERROR_CONNECT, "Socket error (%s)", strerror(errno));
	else if(ssl) {
		herror_t status = hssl_client_ssl(dsock);
		if(status != H_OK) {
			log_error2("hssl_client_ssl failed (%s)", herror_message(status));
			return status;
		}
	}
	return H_OK;
}

/*--------------------------------------------------
   FUNCTION: hsocket_bind
   ----------------------------------------------------*/
herror_t hsocket_bind(hsocket_t * dsock, int port)
{
	hsocket_t sock;
	struct sockaddr_in addr;
	int    opt = 1;
	/* create socket */
	if((sock.sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		log_error2("Cannot create socket (%s)", strerror(errno));
		return herror_new("hsocket_bind", HSOCKET_ERROR_CREATE, "Socket error (%s)", strerror(errno));
	}
	setsockopt(sock.sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
	/* bind socket */
	addr.sin_family = AF_INET;
	addr.sin_port = htons((unsigned short)port); /* short, network byte order */
	addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(addr.sin_zero), '\0', 8); /* zero the rest of the struct */
	if(bind(sock.sock, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1) {
		log_error2("Cannot bind socket (%s)", strerror(errno));
		return herror_new("hsocket_bind", HSOCKET_ERROR_BIND, "Socket error (%s)", strerror(errno));
	}
	dsock->sock = sock.sock;
	return H_OK;
}

#ifdef WIN32
static herror_t _hsocket_sys_accept(hsocket_t * sock, hsocket_t * dest)
{
	hsocket_t sockfd;
	int asize = sizeof(struct sockaddr_in);
	while(1) {
		sockfd.sock = accept(sock->sock, (struct sockaddr *)&(dest->addr), &asize);
		if(sockfd.sock == INVALID_SOCKET) {
			if(WSAGetLastError() != WSAEWOULDBLOCK)
				return herror_new("hsocket_accept", HSOCKET_ERROR_ACCEPT, "Socket error (%s)", strerror(errno));
		}
		else {
			break;
		}
	}
	dest->sock = sockfd.sock;
	return H_OK;
}

#else
static herror_t _hsocket_sys_accept(hsocket_t * sock, hsocket_t * dest)
{
	socklen_t len = sizeof(struct sockaddr_in);
	if((dest->sock = accept(sock->sock, (struct sockaddr *)&(dest->addr), &len)) == -1) {
		log_warn2("accept failed (%s)", strerror(errno));
		return herror_new("hsocket_accept", HSOCKET_ERROR_ACCEPT, "Cannot accept network connection (%s)", strerror(errno));
	}
	return H_OK;
}

#endif

/*----------------------------------------------------------
   FUNCTION: hsocket_accept
   ----------------------------------------------------------*/
herror_t hsocket_accept(hsocket_t * sock, hsocket_t * dest)
{
	herror_t status;
	if(sock->sock < 0)
		return herror_new("hsocket_accept", HSOCKET_ERROR_NOT_INITIALIZED, "hsocket_t not initialized");
	if((status = _hsocket_sys_accept(sock, dest)) != H_OK)
		return status;
	if((status = hssl_server_ssl(dest)) != H_OK) {
		log_warn2("SSL startup failed (%s)", herror_message(status));
		return status;
	}
	log_verbose3("accepting connection from '%s' socket=%d", SAVE_STR(((char *)inet_ntoa(dest->addr.sin_addr))), dest->sock);
	return H_OK;
}

/*--------------------------------------------------
   FUNCTION: hsocket_listen
   ----------------------------------------------------*/
herror_t hsocket_listen(hsocket_t * sock)
{
	if(sock->sock < 0)
		return herror_new("hsocket_listen", HSOCKET_ERROR_NOT_INITIALIZED, "Called hsocket_listen before initializing!");
	if(listen(sock->sock, 15) == -1) {
		log_error2("listen failed (%s)", strerror(errno));
		return herror_new("hsocket_listen", HSOCKET_ERROR_LISTEN, "Cannot listen on this socket (%s)", strerror(errno));
	}
	return H_OK;
}

#ifdef WIN32
static inline void _hsocket_sys_close(hsocket_t * sock)
{
	char junk[10];
	/* shutdown(sock,SD_RECEIVE); */
	shutdown(sock->sock, SD_SEND);
	while(recv(sock->sock, junk, sizeof(junk), 0) > 0) ;
	/* nothing */
	closesocket(sock->sock);
}

#else
static inline void _hsocket_sys_close(hsocket_t * sock)
{
	shutdown(sock->sock, SHUT_RDWR);
	close(sock->sock);
}

#endif

/*--------------------------------------------------
   FUNCTION: hsocket_close
   ----------------------------------------------------*/
void hsocket_close(hsocket_t * sock)
{
	log_verbose3("closing socket %p (%d)...", sock, sock->sock);
	hssl_cleanup(sock);
	_hsocket_sys_close(sock);
	log_verbose1("socket closed");
}

/*--------------------------------------------------
   FUNCTION: hsocket_send
   ----------------------------------------------------*/
herror_t hsocket_nsend(hsocket_t * sock, const byte_t * bytes, int n)
{
	herror_t status;
	size_t total = 0;
	size_t size;
	log_verbose2("Starting to send on sock=%p", &sock);
	if(sock->sock < 0)
		return herror_new("hsocket_nsend", HSOCKET_ERROR_NOT_INITIALIZED, "hsocket not initialized");
	/* log_verbose2( "SENDING %s", bytes ); */
	while(1) {
		if((status = hssl_write(sock, (const char *)(bytes+total), n, &size)) != H_OK) {
			log_warn2("hssl_write failed (%s)", herror_message(status));
			return status;
		}
		n -= (int)size;
		total += size;
		if(n <= 0)
			break;
	}
	return H_OK;
}

/*--------------------------------------------------
   FUNCTION: hsocket_send
   ----------------------------------------------------*/
herror_t hsocket_send(hsocket_t * sock, const char * str)
{
	return hsocket_nsend(sock, (const byte_t *)str, (int)strlen(str));
}

int hsocket_select_read(int sock, char * buf, size_t len)
{
	struct timeval timeout;
	fd_set fds;
	int ret;
	FD_ZERO(&fds);
	FD_SET(sock, &fds);
	timeout.tv_sec = httpd_get_timeout();
	timeout.tv_usec = 0;
	ret = select(sock+1, &fds, NULL, NULL, &timeout);
	if(ret == 0) {
		//errno = ETIMEDOUT;
		log_verbose2("Socket %d timeout", sock);
		return -1;
	}
#ifdef WIN32
	return recv(sock, buf, len, 0);
#else
	return read(sock, buf, len);
#endif
}

herror_t hsocket_read(hsocket_t * sock, byte_t * buffer, int total, int force, int * received)
{
	herror_t status;
	size_t totalRead;
	size_t count;
/* log_verbose3("Entering hsocket_read(total=%d,force=%d)", total, force); */
	totalRead = 0;
	do {
		if((status = hssl_read(sock, (char *)&buffer[totalRead], (size_t)total-totalRead, &count)) != H_OK) {
			log_warn2("hssl_read failed (%s)", herror_message(status));
			return status;
		}
		if(!force) {
			/* log_verbose3("Leaving !force (received=%d)(status=%d)", *received,
			   status); */
			*received = (int)count;
			return H_OK;
		}
		totalRead += count;
		if(totalRead == total) {
			*received = (int)totalRead;
			/*
			   log_verbose4("Leaving totalRead == total
			   (received=%d)(status=%d)(totalRead=%d)", *received, status,
			   totalRead); */
			return H_OK;
		}
	} while(1);
}

#ifdef HAVE_SSL

static char * certificate = NULL;
static char * certpass = "";
static char * ca_list = NULL;
static SSL_CTX * context = NULL;
static int enabled = 0;
static int _hssl_dummy_verify_cert(X509 * cert);
int (* _hssl_verify_cert)(X509 * cert) = _hssl_dummy_verify_cert;

static void _hssl_superseed(void)
{
	int buf[256], i;
	srand(time(NULL));
	for(i = 0; i < 256; i++) {
		buf[i] = rand();
	}
	RAND_seed((unsigned char *)buf, sizeof(buf));
}

static char * _hssl_get_error(SSL * ssl, int ret)
{
	switch(SSL_get_error(ssl, ret)) {
	    case SSL_ERROR_NONE: return "None";
	    case SSL_ERROR_ZERO_RETURN: return "Zero return";
	    case SSL_ERROR_WANT_READ: return "Want read";
	    case SSL_ERROR_WANT_WRITE: return "Want write";
	    case SSL_ERROR_WANT_X509_LOOKUP: return "Want x509 lookup";
	    case SSL_ERROR_SYSCALL: return (ERR_get_error() == 0 && ret == -1) ? strerror(errno) : "Syscall failed";
	    case SSL_ERROR_SSL: return "SSL error";
	    default: return "Unkown error";
	}
}

static int _hssl_password_callback(char * buf, int num, int rwflag, void * userdata)
{
	int ret = strlen(certpass);
	if(num < ret+1)
		return 0;
	strcpy(buf, certpass);
	return ret;
}

int verify_sn(X509 * cert, int who, int nid, char * str)
{
	char name[256];
	char buf[256];
	memset(name, '\0', 256);
	memset(buf, '\0', 256);
	if(who == CERT_SUBJECT) {
		X509_NAME_oneline(X509_get_subject_name(cert), name, 256);
	}
	else {
		X509_NAME_oneline(X509_get_issuer_name(cert), name, 256);
	}
	buf[0] = '/';
	strcat(buf, OBJ_nid2sn(nid));
	strcat(buf, "=");
	strcat(buf, str);
	return strstr(name, buf) ? 1 : 0;
}

void hssl_set_hssl_verify_cert(int func(X509 * cert))
{
	_hssl_verify_cert = func;
}

static int _hssl_dummy_verify_cert(X509 * cert)
{
	/* TODO: Make sure that the client is providing a client cert, or that the
	   Module is providing the Module cert */

	/* connect to anyone */
	log_verbose1("Validating certificate.");
	return 1;
}

static int _hssl_cert_verify_callback(int prev_ok, X509_STORE_CTX * ctx)
{
/*
    if ((X509_STORE_CTX_get_error(ctx) = X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN))
    {
        log_verbose1("Self signed cert in chain");
        return 1;
    }
 */
	log_verbose2("Cert depth = %d", X509_STORE_CTX_get_error_depth(ctx));
	if(X509_STORE_CTX_get_error_depth(ctx) == 0) {
		return _hssl_verify_cert(X509_STORE_CTX_get_current_cert(ctx));
	}
	else {
		log_verbose1("Cert ok (prev)");
		return prev_ok;
	}
}

void hssl_set_certificate(char * c)
{
	certificate = c;
}

void hssl_set_certpass(char * c)
{
	certpass = c;
}

void hssl_set_ca(char * c)
{
	ca_list = c;
}

void hssl_enable(void)
{
	enabled = 1;
}

static void _hssl_parse_arguments(int argc, char ** argv)
{
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i-1], NHTTP_ARG_CERT)) {
			certificate = argv[i];
		}
		else if(!strcmp(argv[i-1], NHTTP_ARG_CERTPASS)) {
			certpass = argv[i];
		}
		else if(!strcmp(argv[i-1], NHTTP_ARG_CA)) {
			ca_list = argv[i];
		}
		else if(!strcmp(argv[i-1], NHTTP_ARG_HTTPS)) {
			enabled = 1;
		}
	}
}

static void _hssl_library_init(void)
{
	static int initialized = 0;
	if(!initialized) {
		log_verbose1("Initializing library");
		SSL_library_init();
		SSL_load_error_strings();
		ERR_load_crypto_strings();
		OpenSSL_add_ssl_algorithms();
		initialized = 1;
	}
}

static herror_t _hssl_server_context_init(void)
{
	log_verbose3("enabled=%i, certificate=%p", enabled, certificate);
	if(!enabled || !certificate)
		return H_OK;
	if(!(context = SSL_CTX_new(SSLv23_method()))) {
		log_error1("Cannot create SSL context");
		return herror_new("_hssl_server_context_init", HSSL_ERROR_CONTEXT,
			"Unable to create SSL context");
	}
	if(!(SSL_CTX_use_certificate_file(context, certificate, SSL_FILETYPE_PEM))) {
		log_error2("Cannot read certificate file: \"%s\"", certificate);
		SSL_CTX_free(context);
		return herror_new("_hssl_server_context_init", HSSL_ERROR_CERTIFICATE,
			"Unable to use SSL certificate \"%s\"", certificate);
	}
	SSL_CTX_set_default_passwd_cb(context, _hssl_password_callback);
	if(!(SSL_CTX_use_PrivateKey_file(context, certificate, SSL_FILETYPE_PEM))) {
		log_error2("Cannot read key file: \"%s\"", certificate);
		SSL_CTX_free(context);
		return herror_new("_hssl_server_context_init", HSSL_ERROR_PEM,
			"Unable to use private key");
	}
	if(ca_list != NULL && *ca_list != '\0') {
		if(!(SSL_CTX_load_verify_locations(context, ca_list, NULL))) {
			SSL_CTX_free(context);
			log_error2("Cannot read CA list: \"%s\"", ca_list);
			return herror_new("_hssl_server_context_init", HSSL_ERROR_CA_LIST,
				"Unable to read certification authorities \"%s\"");
		}
		SSL_CTX_set_client_CA_list(context, SSL_load_client_CA_file(ca_list));
		log_verbose1("Certification authority contacted");
	}
	SSL_CTX_set_verify(context, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE, _hssl_cert_verify_callback);
	log_verbose1("Certificate verification callback registered");
	SSL_CTX_set_mode(context, SSL_MODE_AUTO_RETRY);
	SSL_CTX_set_session_cache_mode(context, SSL_SESS_CACHE_OFF);
	_hssl_superseed();
	return H_OK;
}

static void _hssl_server_context_destroy(void)
{
	if(context) {
		SSL_CTX_free(context);
		context = NULL;
	}
}

herror_t hssl_module_init(int argc, char ** argv)
{
	_hssl_parse_arguments(argc, argv);
	if(enabled) {
		_hssl_library_init();
		log_verbose1("SSL enabled");
	}
	else {
		log_verbose1("SSL _not_ enabled");
	}
	return _hssl_server_context_init();
}

void hssl_module_destroy(void)
{
	_hssl_server_context_destroy();
}

int hssl_enabled(void)
{
	return enabled;
}

herror_t hssl_client_ssl(hsocket_t * sock)
{
	SSL * ssl;
	int ret;
	log_verbose1("Starting SSL client initialization");
	if(!(ssl = SSL_new(context))) {
		log_error1("Cannot create new SSL object");
		return herror_new("hssl_client_ssl", HSSL_ERROR_CLIENT, "SSL_new failed");
	}
	SSL_set_fd(ssl, sock->sock);
	if((ret = SSL_connect(ssl)) <= 0) {
		herror_t err;
		log_error2("SSL connect error (%s)", _hssl_get_error(ssl, -1));
		err = herror_new("hssl_client_ssl", HSSL_ERROR_CONNECT, "SSL_connect failed (%s)", _hssl_get_error(ssl, ret));
		SSL_free(ssl);
		return err;
	}
	/* SSL_connect should take care of this for us. if
	   (SSL_get_peer_certificate(ssl) == NULL) { log_error1("No certificate
	   provided"); SSL_free(ssl); return herror_new("hssl_client_ssl",
	   HSSL_ERROR_CERTIFICATE, "No certificate provided"); }

	   if (SSL_get_verify_result(ssl) != X509_V_OK) { log_error1("Certificate
	   did not verify"); SSL_free(ssl); return herror_new("hssl_client_ssl",
	   HSSL_ERROR_CERTIFICATE, "Verfiy certificate failed"); } */

	log_verbose1("SSL client initialization completed");
	sock->ssl = ssl;
	return H_OK;
}

static int _hssl_bio_read(BIO * b, char * out, int outl)
{
	return hsocket_select_read(b->num, out, outl);;
}

herror_t hssl_server_ssl(hsocket_t * sock)
{
	SSL * ssl;
	int ret;
	BIO * sbio;
	if(!enabled)
		return H_OK;
	log_verbose2("Starting SSL initialization for socket %d", sock->sock);
	if(!(ssl = SSL_new(context))) {
		log_warn1("SSL_new failed");
		return herror_new("hssl_server_ssl", HSSL_ERROR_SERVER, "Cannot create SSL object");
	}
	/* SSL_set_fd(ssl, sock->sock); */
	sbio = BIO_new_socket(sock->sock, BIO_NOCLOSE);
	if(sbio == NULL) {
		log_error1("BIO_new_socket failed");
		return NULL;
	}
	// BIO_set_callback(sbio, hssl_bio_cb);
	sbio->method->bread = _hssl_bio_read;
	SSL_set_bio(ssl, sbio, sbio);
	if((ret = SSL_accept(ssl)) <= 0) {
		herror_t err;
		log_error2("SSL_accept failed (%s)", _hssl_get_error(ssl, ret));
		err = herror_new("hssl_server_ssl", HSSL_ERROR_SERVER, "SSL_accept failed (%s)", _hssl_get_error(ssl, ret));
		SSL_free(ssl);
		return err;
	}
	sock->ssl = ssl;
	return H_OK;
}

void hssl_cleanup(hsocket_t * sock)
{
	if(sock->ssl) {
		SSL_shutdown(sock->ssl);
		SSL_free(sock->ssl);
		sock->ssl = NULL;
	}
}

herror_t hssl_read(hsocket_t * sock, char * buf, size_t len, size_t * received)
{
	int count;
/* log_verbose4("sock->sock=%d sock->ssl=%p, len=%li", sock->sock, sock->ssl, len); */
	if(sock->ssl) {
		if((count = SSL_read(sock->ssl, buf, len)) < 1)
			return herror_new("SSL_read", HSOCKET_ERROR_RECEIVE, "SSL_read failed (%s)", _hssl_get_error(sock->ssl, count));
	}
	else {
		if((count = hsocket_select_read(sock->sock, buf, len)) == -1)
			return herror_new("hssl_read", HSOCKET_ERROR_RECEIVE, "recv failed (%s)", strerror(errno));
	}
	*received = count;
	return H_OK;
}

herror_t hssl_write(hsocket_t * sock, const char * buf, size_t len, size_t * sent)
{
	int count;

/*  log_verbose4("sock->sock=%d, sock->ssl=%p, len=%li", sock->sock, sock->ssl, len); */
	if(sock->ssl) {
		if((count = SSL_write(sock->ssl, buf, len)) == -1)
			return herror_new("SSL_write", HSOCKET_ERROR_SEND, "SSL_write failed (%s)", _hssl_get_error(sock->ssl, count));
	}
	else {
		if((count = send(sock->sock, buf, len, 0)) == -1)
			return herror_new("hssl_write", HSOCKET_ERROR_SEND, "send failed (%s)", strerror(errno));
	}
	*sent = count;
	return H_OK;
}

#else

herror_t hssl_read(hsocket_t * sock, char * buf, size_t len, size_t * received)
{
	int count;
	if((count = hsocket_select_read(sock->sock, buf, len)) == -1)
		return herror_new("hssl_read", HSOCKET_ERROR_RECEIVE, "recv failed (%s)", strerror(errno));
	*received = count;
	return H_OK;
}

herror_t hssl_write(hsocket_t * sock, const char * buf, size_t len, size_t * sent)
{
	int count;
	if((count = send(sock->sock, buf, len, 0)) == -1)
		return herror_new("hssl_write", HSOCKET_ERROR_SEND, "send failed (%s)", strerror(errno));
	*sent = count;
	return H_OK;
}

#endif
/*
   -------------------------------------------------------------------

   HTTP INPUT STREAM

   -------------------------------------------------------------------
 */

static int _http_stream_is_content_length(hpair_t * header)
{
	return hpairnode_get_ignore_case(header, HEADER_CONTENT_LENGTH) != NULL;
}

static int _http_stream_is_chunked(hpair_t * header)
{
	char * chunked;
	chunked = hpairnode_get_ignore_case(header, HEADER_TRANSFER_ENCODING);
	if(chunked != NULL) {
		if(!strcmp(chunked, TRANSFER_ENCODING_CHUNKED)) {
			return 1;
		}
	}
	return 0;
}

/**
   Creates a new input stream.
 */
http_input_stream_t * http_input_stream_new(hsocket_t * sock, hpair_t * header)
{
	http_input_stream_t * result;
	char * content_length;
	/* Paranoya check */
	/* if (header == NULL) return NULL; */
	/* Create object */
	if(!(result = (http_input_stream_t *)malloc(sizeof(http_input_stream_t)))) {
		log_error2("malloc failed (%s)", strerror(errno));
		return NULL;
	}
	result->sock = sock;
	result->err = H_OK;

	/* Find connection type */
	hpairnode_dump_deep(header);
	/* Check if Content-type */
	if(_http_stream_is_content_length(header)) {
		log_verbose1("Stream transfer with 'Content-length'");
		content_length = hpairnode_get_ignore_case(header, HEADER_CONTENT_LENGTH);
		result->content_length = atoi(content_length);
		result->received = 0;
		result->type = HTTP_TRANSFER_CONTENT_LENGTH;
	}
	/* Check if Chunked */
	else if(_http_stream_is_chunked(header)) {
		log_verbose1("Stream transfer with 'chunked'");
		result->type = HTTP_TRANSFER_CHUNKED;
		result->chunk_size = -1;
		result->received = -1;
	}
	/* Assume connection close */
	else {
		log_verbose1("Stream transfer with 'Connection: close'");
		result->type = HTTP_TRANSFER_CONNECTION_CLOSE;
		result->connection_closed = 0;
		result->received = 0;
	}
	return result;
}

/**
   Creates a new input stream from file.
   This function was added for MIME messages
   and for debugging.
 */
http_input_stream_t * http_input_stream_new_from_file(const char * filename)
{
	http_input_stream_t * result;
	FILE * fd;
	if(!(fd = fopen(filename, "rb"))) {
		log_error2("fopen failed (%s)", strerror(errno));
		return NULL;
	}
	/* Create object */
	if(!(result = (http_input_stream_t *)malloc(sizeof(http_input_stream_t)))) {
		log_error2("malloc failed (%s)", strerror(errno));
		fclose(fd);
		return NULL;
	}
	result->type = HTTP_TRANSFER_FILE;
	result->fd = fd;
	result->deleteOnExit = 0;
	strcpy(result->filename, filename);
	return result;
}

/**
   Free input stream
 */
void http_input_stream_free(http_input_stream_t * stream)
{
	if(stream->type == HTTP_TRANSFER_FILE && stream->fd) {
		fclose(stream->fd);
		if(stream->deleteOnExit)
			log_info2("Removing '%s'", stream->filename);
		/* remove(stream->filename); */
	}
	free(stream);
}

static int _http_input_stream_is_content_length_ready(http_input_stream_t * stream)
{
	return stream->content_length > stream->received;
}

static int _http_input_stream_is_chunked_ready(http_input_stream_t * stream)
{
	return stream->chunk_size != 0;
}

static int _http_input_stream_is_connection_closed_ready(http_input_stream_t * stream)
{
	return !stream->connection_closed;
}

static int _http_input_stream_is_file_ready(http_input_stream_t * stream)
{
	return !feof(stream->fd);
}

static int _http_input_stream_content_length_read(http_input_stream_t * stream, byte_t * dest, int size)
{
	herror_t status;
	int read;
	/* check limit */
	if(stream->content_length-stream->received < size)
		size = stream->content_length-stream->received;
	/* read from socket */
	status = hsocket_read(stream->sock, dest, size, 1, &read);
	if(status != H_OK) {
		stream->err = status;
		return -1;
	}
	stream->received += read;
	return read;
}

static int _http_input_stream_chunked_read_chunk_size(http_input_stream_t * stream)
{
	char chunk[25];
	int status, i = 0;
	int chunk_size;
	herror_t err;
	while(1) {
		err = hsocket_read(stream->sock, (byte_t *)&(chunk[i]), 1, 1, &status);
		if(status != 1) {
			stream->err = herror_new("_http_input_stream_chunked_read_chunk_size",
				GENERAL_INVALID_PARAM, "This should never happen!");
			return -1;
		}
		if(err != H_OK) {
			log_error4("[%d] %s(): %s ", herror_code(err), herror_func(err), herror_message(err));
			stream->err = err;
			return -1;
		}
		if(chunk[i] == '\r' || chunk[i] == ';') {
			chunk[i] = '\0';
		}
		else if(chunk[i] == '\n') {
			chunk[i] = '\0'; /* double check */
			chunk_size = strtol(chunk, (char **)NULL, 16); /* hex to dec */
			/*
			   log_verbose3("chunk_size: '%s' as dec: '%d'", chunk, chunk_size); */
			return chunk_size;
		}
		if(i == 24) {
			stream->err = herror_new("_http_input_stream_chunked_read_chunk_size",
					STREAM_ERROR_NO_CHUNK_SIZE, "reached max line == %d", i);
			return -1;
		}
		else
			i++;
	}
	/* this should never happen */
	stream->err = herror_new("_http_input_stream_chunked_read_chunk_size", STREAM_ERROR_NO_CHUNK_SIZE, "reached max line == %d", i);
	return -1;
}

static int _http_input_stream_chunked_read(http_input_stream_t * stream, byte_t * dest, int size)
{
	int status, counter;
	int remain, read = 0;
	char ch;
	herror_t err;
	while(size > 0) {
		remain = stream->chunk_size-stream->received;
		if(remain == 0 && stream->chunk_size != -1) {
			/* This is not the first chunk. so skip new line until chunk size string */
			counter = 100; /* maximum for stop infinity */
			while(1) {
				if((err = hsocket_read(stream->sock, (byte_t *)&ch, 1, 1, &status)) != H_OK) {
					stream->err = err;
					return -1;
				}
				if(ch == '\n') {
					break;
				}
				if(counter-- == 0) {
					stream->err = herror_new("_http_input_stream_chunked_read",
						STREAM_ERROR_WRONG_CHUNK_SIZE, "Wrong chunk-size");
					return -1;
				}
			}
		}
		if(remain == 0) {
			/* receive new chunk size */
			stream->chunk_size = _http_input_stream_chunked_read_chunk_size(stream);
			stream->received = 0;
			if(stream->chunk_size < 0) {
				/* TODO (#1#): set error flag */
				return stream->chunk_size;
			}
			else if(stream->chunk_size == 0) {
				return read;
			}
			remain = stream->chunk_size;
		}
		/* show remaining chunk size in socket */
		if(remain < size) {
			/* read from socket */
			if((err = hsocket_read(stream->sock, &(dest[read]), remain, 1, &status)) != H_OK) {
				stream->err = err;
				return -1;
			}
			if(status != remain) {
				stream->err = herror_new("_http_input_stream_chunked_read", GENERAL_INVALID_PARAM,
					"This should never happen (remain=%d)(status=%d)!", remain, status);
				return -1;
			}
		}
		else {
			/* read from socket */
			err = hsocket_read(stream->sock, &(dest[read]), size, 1, &status);
			if(status != size) {
				stream->err = herror_new("_http_input_stream_chunked_read",
					GENERAL_INVALID_PARAM, "This should never happen (size=%d)(status=%d)!", size, status);
				return -1;
			}
			if(err != H_OK) {
				stream->err = err;
				return -1;
			}
		}
		read += status;
		size -= status;
		stream->received += status;
	}
	return read;
}

static int _http_input_stream_connection_closed_read(http_input_stream_t * stream, byte_t * dest, int size)
{
	int status;
	herror_t err;
	/* read from socket */
	if((err = hsocket_read(stream->sock, dest, size, 0, &status)) != H_OK) {
		stream->err = err;
		return -1;
	}
	if(status == 0)
		stream->connection_closed = 1;
	stream->received += status;
	return status;
}

static int _http_input_stream_file_read(http_input_stream_t * stream, byte_t * dest, int size)
{
	size_t len = fread(dest, 1, size, stream->fd);
	if(len < (size_t)size) {
		stream->err = herror_new("_http_input_stream_file_read", HSOCKET_ERROR_RECEIVE, "fread() returned -1");
		return -1;
	}
	else
		return (int)len;
}

/**
   Returns the actual status of the stream.
 */
int http_input_stream_is_ready(http_input_stream_t * stream)
{
	int    ok = 0;
	if(stream) { // paranoia check
		stream->err = H_OK; // reset error flag
		switch(stream->type) {
			case HTTP_TRANSFER_CONTENT_LENGTH: ok = _http_input_stream_is_content_length_ready(stream); break;
			case HTTP_TRANSFER_CHUNKED: ok = _http_input_stream_is_chunked_ready(stream); break;
			case HTTP_TRANSFER_CONNECTION_CLOSE: ok = _http_input_stream_is_connection_closed_ready(stream); break;
			case HTTP_TRANSFER_FILE: ok = _http_input_stream_is_file_ready(stream); break;
		}
	}
	return ok;
}
/**
   Returns the actual read bytes
   <0 on error
 */
int http_input_stream_read(http_input_stream_t * stream, byte_t * dest, int size)
{
	int len = 0;
	/* paranoia check */
	if(stream == NULL) {
		len = -1;
	}
	else {
		/* XXX: possible memleak! reset error flag */
		stream->err = H_OK;
		switch(stream->type) {
			case HTTP_TRANSFER_CONTENT_LENGTH:
				len = _http_input_stream_content_length_read(stream, dest, size);
				break;
			case HTTP_TRANSFER_CHUNKED:
				len = _http_input_stream_chunked_read(stream, dest, size);
				break;
			case HTTP_TRANSFER_CONNECTION_CLOSE:
				len = _http_input_stream_connection_closed_read(stream, dest, size);
				break;
			case HTTP_TRANSFER_FILE:
				len = _http_input_stream_file_read(stream, dest, size);
				break;
			default:
				stream->err = herror_new("http_input_stream_read", STREAM_ERROR_INVALID_TYPE, "%d is invalid stream type", stream->type);
				return -1;
		}
	}
	return len;
}

/*
   -------------------------------------------------------------------

   HTTP OUTPUT STREAM

   -------------------------------------------------------------------
 */

/**
   Creates a new output stream. Transfer code will be found from header.
 */
http_output_stream_t * http_output_stream_new(hsocket_t * sock, hpair_t * header)
{
	http_output_stream_t * result;
	char * content_length;
	/* Paranoya check */
/*  if (header == NULL)
    return NULL;
 */
	/* Create object */
	if(!(result = (http_output_stream_t *)malloc(sizeof(http_output_stream_t)))) {
		log_error2("malloc failed (%s)", strerror(errno));
		return NULL;
	}
	result->sock = sock;
	result->sent = 0;

	/* Find connection type */
	/* Check if Content-type */
	if(_http_stream_is_content_length(header)) {
		log_verbose1("Stream transfer with 'Content-length'");
		content_length = hpairnode_get_ignore_case(header, HEADER_CONTENT_LENGTH);
		result->content_length = atoi(content_length);
		result->type = HTTP_TRANSFER_CONTENT_LENGTH;
	}
	/* Check if Chunked */
	else if(_http_stream_is_chunked(header)) {
		log_verbose1("Stream transfer with 'chunked'");
		result->type = HTTP_TRANSFER_CHUNKED;
	}
	/* Assume connection close */
	else {
		log_verbose1("Stream transfer with 'Connection: close'");
		result->type = HTTP_TRANSFER_CONNECTION_CLOSE;
	}
	return result;
}
/**
   Free output stream
 */
void http_output_stream_free(http_output_stream_t * stream)
{
	free(stream);
}
/**
   Writes 'size' bytes of 'bytes' into stream.
   Returns socket error flags or H_OK.
 */
herror_t http_output_stream_write(http_output_stream_t * stream, const byte_t * bytes, int size)
{
	herror_t status;
	char   chunked[32];
	if(stream->type == HTTP_TRANSFER_CHUNKED) {
		sprintf(chunked, "%x\r\n", size);
		if((status = hsocket_send(stream->sock, chunked)) != H_OK)
			return status;
	}
	if(size > 0) {
		if((status = hsocket_nsend(stream->sock, bytes, size)) != H_OK)
			return status;
	}
	if(stream->type == HTTP_TRANSFER_CHUNKED) {
		if((status = hsocket_send(stream->sock, "\r\n")) != H_OK)
			return status;
	}
	return H_OK;
}
/**
   Writes 'strlen()' bytes of 'str' into stream.
   Returns socket error flags or H_OK.
 */
herror_t http_output_stream_write_string(http_output_stream_t * stream, const char * str)
{
	return http_output_stream_write(stream, (const byte_t *)str, (int)strlen(str));
}

herror_t http_output_stream_flush(http_output_stream_t * stream)
{
	herror_t status;
	if(stream->type == HTTP_TRANSFER_CHUNKED) {
		if((status = hsocket_send(stream->sock, "0\r\n\r\n")) != H_OK)
			return status;
	}
	return H_OK;
}
//
//
//
#ifdef BASE64_TEST_CASE_FROM_RFC2617
int main(int argc, char ** argv)
{

	unsigned char * instr = "QWxhZGRpbjpvcGVuIHNlc2FtZQ==";
	unsigned char * result = "Aladdin:open sesame";
	unsigned char instr2[80];
	unsigned char outstr[80];

	bzero(outstr, 80);
	base64_decode(instr, outstr);

	printf("\"%s\" => \"%s\"\n", instr, outstr);
	if(strcmp(outstr, result))
		printf("base64_decode failed\n");
	strcpy(instr2, outstr);

	bzero(outstr, 80);
	base64_encode(instr2, outstr);

	printf("\"%s\" => \"%s\"\n", instr2, outstr);
	if(strcmp(outstr, instr))
		printf("base64_encode failed\n");
	return 0;
}

#endif
