/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#ifndef _NGX_HTTP_H_INCLUDED_
#define _NGX_HTTP_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

struct /*ngx_http_request_s*/ngx_http_request_t;

//typedef struct ngx_http_request_s ngx_http_request_t;
typedef struct ngx_http_upstream_s ngx_http_upstream_t;
typedef struct ngx_http_cache_s ngx_http_cache_t;
typedef struct ngx_http_file_cache_s ngx_http_file_cache_t;
typedef struct ngx_http_log_ctx_s ngx_http_log_ctx_t;
typedef struct ngx_http_chunked_s ngx_http_chunked_t;
typedef struct ngx_http_v2_stream_s ngx_http_v2_stream_t;

typedef ngx_int_t (*ngx_http_header_handler_pt)(ngx_http_request_t * r, ngx_table_elt_t * h, ngx_uint_t offset);
typedef u_char *(*ngx_http_log_handler_pt)(ngx_http_request_t * r, ngx_http_request_t * sr, u_char * buf, size_t len);
//
//#include <ngx_http_variables.h>
//
typedef ngx_variable_value_t ngx_http_variable_value_t;

#define ngx_http_variable(v)     { sizeof(v) - 1, 1, 0, 0, 0, (u_char*)v }

typedef struct ngx_http_variable_s ngx_http_variable_t;
typedef void (*ngx_http_set_variable_pt)(ngx_http_request_t * r, ngx_http_variable_value_t * v, uintptr_t data);
typedef ngx_int_t (*ngx_http_get_variable_pt)(ngx_http_request_t * r, ngx_http_variable_value_t * v, uintptr_t data);

#define NGX_HTTP_VAR_CHANGEABLE   1
#define NGX_HTTP_VAR_NOCACHEABLE  2
#define NGX_HTTP_VAR_INDEXED      4
#define NGX_HTTP_VAR_NOHASH       8
#define NGX_HTTP_VAR_WEAK         16
#define NGX_HTTP_VAR_PREFIX       32

struct ngx_http_variable_s {
	ngx_str_t name;                   /* must be first to build the hash */
	ngx_http_set_variable_pt set_handler;
	ngx_http_get_variable_pt get_handler;
	uintptr_t data;
	ngx_uint_t flags;
	ngx_uint_t index;
};

ngx_http_variable_t * ngx_http_add_variable(ngx_conf_t * cf, ngx_str_t * name, ngx_uint_t flags);
ngx_int_t ngx_http_get_variable_index(ngx_conf_t * cf, ngx_str_t * name);
ngx_http_variable_value_t * ngx_http_get_indexed_variable(ngx_http_request_t * r, ngx_uint_t index);
ngx_http_variable_value_t * ngx_http_get_flushed_variable(ngx_http_request_t * r, ngx_uint_t index);
ngx_http_variable_value_t * ngx_http_get_variable(ngx_http_request_t * r, ngx_str_t * name, ngx_uint_t key);
ngx_int_t ngx_http_variable_unknown_header(ngx_http_variable_value_t * v, ngx_str_t * var, ngx_list_part_t * part, size_t prefix);

#if (NGX_PCRE)
	struct ngx_http_regex_variable_t {
		ngx_uint_t capture;
		ngx_int_t index;
	};
	struct ngx_http_regex_t {
		ngx_regex_t * regex;
		ngx_uint_t ncaptures;
		ngx_http_regex_variable_t * variables;
		ngx_uint_t nvariables;
		ngx_str_t name;
	};
	struct ngx_http_map_regex_t {
		ngx_http_regex_t * regex;
		void * value;
	};
	ngx_http_regex_t * ngx_http_regex_compile(ngx_conf_t * cf, ngx_regex_compile_t * rc);
	ngx_int_t ngx_http_regex_exec(ngx_http_request_t * r, ngx_http_regex_t * re, ngx_str_t * s);
#endif

struct ngx_http_map_t {
	ngx_hash_combined_t hash;
#if (NGX_PCRE)
	ngx_http_map_regex_t * regex;
	ngx_uint_t nregex;
#endif
};

void * ngx_http_map_find(ngx_http_request_t * r, ngx_http_map_t * map, ngx_str_t * match);
ngx_int_t ngx_http_variables_add_core_vars(ngx_conf_t * cf);
ngx_int_t ngx_http_variables_init_vars(ngx_conf_t * cf);

extern ngx_http_variable_value_t ngx_http_variable_null_value;
extern ngx_http_variable_value_t ngx_http_variable_true_value;
//
//#include <ngx_http_config.h>
//
struct ngx_http_conf_ctx_t {
	void ** main_conf;
	void ** srv_conf;
	void ** loc_conf;
};

struct ngx_http_module_t {
	ngx_int_t (*preconfiguration)(ngx_conf_t * cf);
	ngx_int_t (*postconfiguration)(ngx_conf_t * cf);
	void * (*create_main_conf)(ngx_conf_t *cf);
	char * (*init_main_conf)(ngx_conf_t *cf, void * conf);
	void * (*create_srv_conf)(ngx_conf_t *cf);
	char * (*merge_srv_conf)(ngx_conf_t *cf, void * prev, void * conf);
	void * (*create_loc_conf)(ngx_conf_t *cf);
	char * (*merge_loc_conf)(ngx_conf_t *cf, void * prev, void * conf);
};

#define NGX_HTTP_MODULE           0x50545448   /* "HTTP" */

#define NGX_HTTP_MAIN_CONF        0x02000000
#define NGX_HTTP_SRV_CONF         0x04000000
#define NGX_HTTP_LOC_CONF         0x08000000
#define NGX_HTTP_UPS_CONF         0x10000000
#define NGX_HTTP_SIF_CONF         0x20000000
#define NGX_HTTP_LIF_CONF         0x40000000
#define NGX_HTTP_LMT_CONF         0x80000000

#define NGX_HTTP_MAIN_CONF_OFFSET  offsetof(ngx_http_conf_ctx_t, main_conf)
#define NGX_HTTP_SRV_CONF_OFFSET   offsetof(ngx_http_conf_ctx_t, srv_conf)
#define NGX_HTTP_LOC_CONF_OFFSET   offsetof(ngx_http_conf_ctx_t, loc_conf)

#define ngx_http_get_module_main_conf(r, module) (r)->main_conf[module.ctx_index]
#define ngx_http_get_module_srv_conf(r, module)  (r)->srv_conf[module.ctx_index]
#define ngx_http_get_module_loc_conf(r, module)  (r)->loc_conf[module.ctx_index]
#define ngx_http_conf_get_module_main_conf(cf, module) ((ngx_http_conf_ctx_t*)cf->ctx)->main_conf[module.ctx_index]
#define ngx_http_conf_get_module_srv_conf(cf, module)  ((ngx_http_conf_ctx_t*)cf->ctx)->srv_conf[module.ctx_index]
#define ngx_http_conf_get_module_loc_conf(cf, module)  ((ngx_http_conf_ctx_t*)cf->ctx)->loc_conf[module.ctx_index]

#define ngx_http_cycle_get_module_main_conf(cycle, module) (cycle->conf_ctx[ngx_http_module.index] ? ((ngx_http_conf_ctx_t*)cycle->conf_ctx[ngx_http_module.index])->main_conf[module.ctx_index] : NULL)
//
//
//
//#include <ngx_http_request.h>
//
#define NGX_HTTP_MAX_URI_CHANGES           10
#define NGX_HTTP_MAX_SUBREQUESTS           50

#define NGX_HTTP_LC_HEADER_LEN             32 // must be 2^n 

#define NGX_HTTP_DISCARD_BUFFER_SIZE       4096
#define NGX_HTTP_LINGERING_BUFFER_SIZE     4096

#define NGX_HTTP_VERSION_9                 9
#define NGX_HTTP_VERSION_10                1000
#define NGX_HTTP_VERSION_11                1001
#define NGX_HTTP_VERSION_20                2000

#define NGX_HTTP_UNKNOWN                   0x0001
#define NGX_HTTP_GET                       0x0002
#define NGX_HTTP_HEAD                      0x0004
#define NGX_HTTP_POST                      0x0008
#define NGX_HTTP_PUT                       0x0010
#define NGX_HTTP_DELETE                    0x0020
#define NGX_HTTP_MKCOL                     0x0040
#define NGX_HTTP_COPY                      0x0080
#define NGX_HTTP_MOVE                      0x0100
#define NGX_HTTP_OPTIONS                   0x0200
#define NGX_HTTP_PROPFIND                  0x0400
#define NGX_HTTP_PROPPATCH                 0x0800
#define NGX_HTTP_LOCK                      0x1000
#define NGX_HTTP_UNLOCK                    0x2000
#define NGX_HTTP_PATCH                     0x4000
#define NGX_HTTP_TRACE                     0x8000

#define NGX_HTTP_CONNECTION_CLOSE          1
#define NGX_HTTP_CONNECTION_KEEP_ALIVE     2

#define NGX_NONE                           1

#define NGX_HTTP_PARSE_HEADER_DONE         1

#define NGX_HTTP_CLIENT_ERROR              10
#define NGX_HTTP_PARSE_INVALID_METHOD      10
#define NGX_HTTP_PARSE_INVALID_REQUEST     11
#define NGX_HTTP_PARSE_INVALID_VERSION     12
#define NGX_HTTP_PARSE_INVALID_09_METHOD   13

#define NGX_HTTP_PARSE_INVALID_HEADER      14

/* unused                                  1 */
#define NGX_HTTP_SUBREQUEST_IN_MEMORY      2
#define NGX_HTTP_SUBREQUEST_WAITED         4
#define NGX_HTTP_SUBREQUEST_CLONE          8
#define NGX_HTTP_SUBREQUEST_BACKGROUND     16

#define NGX_HTTP_LOG_UNSAFE                1

#define NGX_HTTP_CONTINUE                  100
#define NGX_HTTP_SWITCHING_PROTOCOLS       101
#define NGX_HTTP_PROCESSING                102

#define NGX_HTTP_OK                        200
#define NGX_HTTP_CREATED                   201
#define NGX_HTTP_ACCEPTED                  202
#define NGX_HTTP_NO_CONTENT                204
#define NGX_HTTP_PARTIAL_CONTENT           206

#define NGX_HTTP_SPECIAL_RESPONSE          300
#define NGX_HTTP_MOVED_PERMANENTLY         301
#define NGX_HTTP_MOVED_TEMPORARILY         302
#define NGX_HTTP_SEE_OTHER                 303
#define NGX_HTTP_NOT_MODIFIED              304
#define NGX_HTTP_TEMPORARY_REDIRECT        307
#define NGX_HTTP_PERMANENT_REDIRECT        308

#define NGX_HTTP_BAD_REQUEST               400
#define NGX_HTTP_UNAUTHORIZED              401
#define NGX_HTTP_FORBIDDEN                 403
#define NGX_HTTP_NOT_FOUND                 404
#define NGX_HTTP_NOT_ALLOWED               405
#define NGX_HTTP_REQUEST_TIME_OUT          408
#define NGX_HTTP_CONFLICT                  409
#define NGX_HTTP_LENGTH_REQUIRED           411
#define NGX_HTTP_PRECONDITION_FAILED       412
#define NGX_HTTP_REQUEST_ENTITY_TOO_LARGE  413
#define NGX_HTTP_REQUEST_URI_TOO_LARGE     414
#define NGX_HTTP_UNSUPPORTED_MEDIA_TYPE    415
#define NGX_HTTP_RANGE_NOT_SATISFIABLE     416
#define NGX_HTTP_MISDIRECTED_REQUEST       421
#define NGX_HTTP_TOO_MANY_REQUESTS         429

/* Our own HTTP codes */

/* The special code to close connection without any response */
#define NGX_HTTP_CLOSE                     444
#define NGX_HTTP_NGINX_CODES               494
#define NGX_HTTP_REQUEST_HEADER_TOO_LARGE  494
#define NGX_HTTPS_CERT_ERROR               495
#define NGX_HTTPS_NO_CERT                  496
/*
 * We use the special code for the plain HTTP requests that are sent to
 * HTTPS port to distinguish it from 4XX in an error page redirection
 */
#define NGX_HTTP_TO_HTTPS                  497

/* 498 is the canceled code for the requests with invalid host name */

/*
 * HTTP does not define the code for the case when a client closed
 * the connection while we are processing its request so we introduce
 * own code to log such situation when a client has closed the connection
 * before we even try to send the HTTP header to it
 */
#define NGX_HTTP_CLIENT_CLOSED_REQUEST     499

#define NGX_HTTP_INTERNAL_SERVER_ERROR     500
#define NGX_HTTP_NOT_IMPLEMENTED           501
#define NGX_HTTP_BAD_GATEWAY               502
#define NGX_HTTP_SERVICE_UNAVAILABLE       503
#define NGX_HTTP_GATEWAY_TIME_OUT          504
#define NGX_HTTP_VERSION_NOT_SUPPORTED     505
#define NGX_HTTP_INSUFFICIENT_STORAGE      507

#define NGX_HTTP_LOWLEVEL_BUFFERED         0xf0
#define NGX_HTTP_WRITE_BUFFERED            0x10
#define NGX_HTTP_GZIP_BUFFERED             0x20
#define NGX_HTTP_SSI_BUFFERED              0x01
#define NGX_HTTP_SUB_BUFFERED              0x02
#define NGX_HTTP_COPY_BUFFERED             0x04

typedef enum {
	NGX_HTTP_INITING_REQUEST_STATE = 0,
	NGX_HTTP_READING_REQUEST_STATE,
	NGX_HTTP_PROCESS_REQUEST_STATE,
	NGX_HTTP_CONNECT_UPSTREAM_STATE,
	NGX_HTTP_WRITING_UPSTREAM_STATE,
	NGX_HTTP_READING_UPSTREAM_STATE,
	NGX_HTTP_WRITING_REQUEST_STATE,
	NGX_HTTP_LINGERING_CLOSE_STATE,
	NGX_HTTP_KEEPALIVE_STATE
} ngx_http_state_e;

struct ngx_http_header_t {
	ngx_str_t name;
	ngx_uint_t offset;
	ngx_http_header_handler_pt handler;
};

struct ngx_http_header_out_t {
	ngx_str_t name;
	ngx_uint_t offset;
};

struct ngx_http_headers_in_t {
	ngx_list_t headers;
	ngx_table_elt_t * host;
	ngx_table_elt_t * connection;
	ngx_table_elt_t * if_modified_since;
	ngx_table_elt_t * if_unmodified_since;
	ngx_table_elt_t * if_match;
	ngx_table_elt_t * if_none_match;
	ngx_table_elt_t * user_agent;
	ngx_table_elt_t * referer;
	ngx_table_elt_t * content_length;
	ngx_table_elt_t * content_range;
	ngx_table_elt_t * content_type;
	ngx_table_elt_t * range;
	ngx_table_elt_t * if_range;
	ngx_table_elt_t * transfer_encoding;
	ngx_table_elt_t * expect;
	ngx_table_elt_t * upgrade;
#if (NGX_HTTP_GZIP)
	ngx_table_elt_t * accept_encoding;
	ngx_table_elt_t * via;
#endif
	ngx_table_elt_t * authorization;
	ngx_table_elt_t * keep_alive;
#if (NGX_HTTP_X_FORWARDED_FOR)
	ngx_array_t x_forwarded_for;
#endif
#if (NGX_HTTP_REALIP)
	ngx_table_elt_t * x_real_ip;
#endif
#if (NGX_HTTP_HEADERS)
	ngx_table_elt_t * accept;
	ngx_table_elt_t * accept_language;
#endif
#if (NGX_HTTP_DAV)
	ngx_table_elt_t * depth;
	ngx_table_elt_t * destination;
	ngx_table_elt_t * overwrite;
	ngx_table_elt_t * date;
#endif
	ngx_str_t user;
	ngx_str_t passwd;
	ngx_array_t cookies;
	ngx_str_t server;
	nginx_off_t content_length_n;
	time_t keep_alive_n;

	unsigned connection_type : 2;
	unsigned chunked : 1;
	unsigned msie : 1;
	unsigned msie6 : 1;
	unsigned opera : 1;
	unsigned gecko : 1;
	unsigned chrome : 1;
	unsigned safari : 1;
	unsigned konqueror : 1;
};

struct ngx_http_headers_out_t {
	ngx_list_t headers;
	ngx_list_t trailers;
	ngx_uint_t status;
	ngx_str_t status_line;
	ngx_table_elt_t * server;
	ngx_table_elt_t * date;
	ngx_table_elt_t * content_length;
	ngx_table_elt_t * content_encoding;
	ngx_table_elt_t * location;
	ngx_table_elt_t * refresh;
	ngx_table_elt_t * last_modified;
	ngx_table_elt_t * content_range;
	ngx_table_elt_t * accept_ranges;
	ngx_table_elt_t * www_authenticate;
	ngx_table_elt_t * expires;
	ngx_table_elt_t * etag;
	ngx_str_t * override_charset;
	size_t content_type_len;
	ngx_str_t content_type;
	ngx_str_t charset;
	u_char * content_type_lowcase;
	ngx_uint_t content_type_hash;
	ngx_array_t cache_control;
	nginx_off_t content_length_n;
	nginx_off_t content_offset;
	time_t date_time;
	time_t last_modified_time;
};

typedef void (*ngx_http_client_body_handler_pt)(ngx_http_request_t * r);

struct ngx_http_request_body_t {
	ngx_temp_file_t * temp_file;
	ngx_chain_t * bufs;
	ngx_buf_t * buf;
	nginx_off_t rest;
	nginx_off_t received;
	ngx_chain_t * free;
	ngx_chain_t * busy;
	ngx_http_chunked_t * chunked;
	ngx_http_client_body_handler_pt post_handler;
};

typedef struct ngx_http_addr_conf_s ngx_http_addr_conf_t;

struct ngx_http_connection_t {
	ngx_http_addr_conf_t * addr_conf;
	ngx_http_conf_ctx_t  * conf_ctx;
#if (NGX_HTTP_SSL || NGX_COMPAT)
	ngx_str_t * ssl_servername;
	#if (NGX_PCRE)
		ngx_http_regex_t * ssl_servername_regex;
	#endif
#endif
	ngx_chain_t * busy;
	ngx_int_t nbusy;
	ngx_chain_t * free;
	unsigned ssl : 1;
	unsigned proxy_protocol : 1;
};

typedef void (*ngx_http_cleanup_pt)(void * data);
//typedef struct ngx_http_cleanup_s ngx_http_cleanup_t;

struct /*ngx_http_cleanup_s*/ngx_http_cleanup_t {
	ngx_http_cleanup_pt handler;
	void * data;
	ngx_http_cleanup_t * next;
};

typedef ngx_int_t (*ngx_http_post_subrequest_pt)(ngx_http_request_t * r, void * data, ngx_int_t rc);

struct ngx_http_post_subrequest_t {
	ngx_http_post_subrequest_pt handler;
	void * data;
};

//typedef struct ngx_http_postponed_request_s ngx_http_postponed_request_t;

struct /*ngx_http_postponed_request_s*/ngx_http_postponed_request_t {
	ngx_http_request_t * request;
	ngx_chain_t * out;
	ngx_http_postponed_request_t * next;
};

//typedef struct ngx_http_posted_request_s ngx_http_posted_request_t;

struct /*ngx_http_posted_request_s*/ngx_http_posted_request_t {
	ngx_http_request_t * request;
	ngx_http_posted_request_t * next;
};

typedef ngx_int_t (*ngx_http_handler_pt)(ngx_http_request_t * r);
typedef void (*ngx_http_event_handler_pt)(ngx_http_request_t * r);

struct /*ngx_http_request_s*/ngx_http_request_t {
	int    SetContentType(SFileFormat fmt, SCodepage cp);

	uint32_t signature; // "HTTP" 
	ngx_connection_t * connection;
	void ** ctx;
	void ** main_conf;
	void ** srv_conf;
	void ** loc_conf;
	ngx_http_event_handler_pt read_event_handler;
	ngx_http_event_handler_pt write_event_handler;
#if (NGX_HTTP_CACHE)
	ngx_http_cache_t * cache;
#endif
	ngx_http_upstream_t * upstream;
	ngx_array_t * upstream_states;
	// of ngx_http_upstream_state_t 
	ngx_pool_t  * pool;
	ngx_buf_t * header_in;
	ngx_http_headers_in_t headers_in;
	ngx_http_headers_out_t headers_out;
	ngx_http_request_body_t * request_body;
	time_t lingering_time;
	time_t start_sec;
	ngx_msec_t start_msec;
	ngx_uint_t method;
	ngx_uint_t http_version;
	ngx_str_t request_line;
	ngx_str_t uri;
	ngx_str_t args;
	ngx_str_t exten;
	ngx_str_t unparsed_uri;
	ngx_str_t method_name;
	ngx_str_t http_protocol;
	ngx_chain_t * out;
	ngx_http_request_t * main;
	ngx_http_request_t * parent;
	ngx_http_postponed_request_t * postponed;
	ngx_http_post_subrequest_t  * post_subrequest;
	ngx_http_posted_request_t * posted_requests;
	ngx_int_t phase_handler;
	ngx_http_handler_pt F_HttpContentHandler/*content_handler*/;
	ngx_uint_t access_code;
	ngx_http_variable_value_t * variables;
#if (NGX_PCRE)
	ngx_uint_t ncaptures;
	int  * captures;
	u_char * captures_data;
#endif
	size_t limit_rate;
	size_t limit_rate_after;
	size_t header_size; /* used to learn the Apache compatible response length without a header */
	nginx_off_t request_length;
	ngx_uint_t err_status;
	ngx_http_connection_t * http_connection;
	ngx_http_v2_stream_t  * stream;
	ngx_http_log_handler_pt log_handler;
	ngx_http_cleanup_t  * cleanup;
	unsigned count                           : 16;
	unsigned http_minor                      : 16;
	unsigned http_major                      : 16;
	unsigned subrequests                     : 8;
	unsigned blocked                         : 8;
	unsigned http_state                      : 4;
	unsigned buffered                        : 4;
	unsigned uri_changes                     : 4;
	unsigned request_body_file_log_level     : 3;
	unsigned request_body_file_group_access  : 1;
	unsigned request_body_in_single_buf      : 1;
	unsigned request_body_in_file_only       : 1;
	unsigned request_body_in_persistent_file : 1;
	unsigned request_body_in_clean_file      : 1;
	unsigned request_body_no_buffering       : 1;
	unsigned uri_changed                     : 1;
	unsigned aio                             : 1;
	unsigned complex_uri                     : 1; // URI with "/." and on Win32 with "//" 
	unsigned quoted_uri                      : 1; // URI with "%" 
	unsigned plus_in_uri                     : 1; // URI with "+" 
	unsigned space_in_uri                    : 1; // URI with " " 
	unsigned invalid_header                  : 1;
	unsigned add_uri_to_alias                : 1;
	unsigned valid_location                  : 1;
	unsigned valid_unparsed_uri              : 1;
	unsigned subrequest_in_memory            : 1;
	unsigned waited                          : 1;
#if (NGX_HTTP_CACHE)
	unsigned cached                          : 1;
#endif
#if (NGX_HTTP_GZIP)
	unsigned gzip_tested                     : 1;
	unsigned gzip_ok                         : 1;
	unsigned gzip_vary                       : 1;
#endif
	unsigned proxy                           : 1;
	unsigned bypass_cache                    : 1;
	unsigned no_cache                        : 1;
	// 
	// instead of using the request context data in
	// ngx_http_limit_conn_module and ngx_http_limit_req_module
	// we use the single bits in the request structure
	// 
	unsigned limit_conn_set                  : 1;
	unsigned limit_req_set                   : 1;
#if 0
	unsigned cacheable                       : 1;
#endif
	unsigned pipeline                        : 1;
	unsigned chunked                         : 1;
	unsigned header_only                     : 1;
	unsigned expect_trailers                 : 1;
	unsigned keepalive                       : 1;
	unsigned lingering_close                 : 1;
	unsigned discard_body                    : 1;
	unsigned reading_body                    : 1;
	unsigned internal                        : 1;
	unsigned error_page                      : 1;
	unsigned filter_finalize                 : 1;
	unsigned post_action                     : 1;
	unsigned request_complete                : 1;
	unsigned request_output                  : 1;
	unsigned header_sent                     : 1;
	unsigned expect_tested                   : 1;
	unsigned root_tested                     : 1;
	unsigned done                            : 1;
	unsigned logged                          : 1;
	unsigned main_filter_need_in_memory      : 1;
	unsigned filter_need_in_memory           : 1;
	unsigned filter_need_temporary           : 1;
	unsigned preserve_body                   : 1;
	unsigned allow_ranges                    : 1;
	unsigned subrequest_ranges               : 1;
	unsigned single_range                    : 1;
	unsigned disable_not_modified            : 1;
	unsigned stat_reading                    : 1;
	unsigned stat_writing                    : 1;
	unsigned stat_processing                 : 1;
	unsigned background                      : 1;
	unsigned health_check                    : 1;
	// used to parse HTTP headers 
	ngx_uint_t state;
	ngx_uint_t header_hash;
	ngx_uint_t lowcase_index;
	u_char   lowcase_header[NGX_HTTP_LC_HEADER_LEN];
	u_char * header_name_start;
	u_char * header_name_end;
	u_char * header_start;
	u_char * header_end;
	//
	// a memory that can be reused after parsing a request line via ngx_http_ephemeral_t
	//
	u_char * uri_start;
	u_char * uri_end;
	u_char * uri_ext;
	u_char * args_start;
	u_char * request_start;
	u_char * request_end;
	u_char * method_end;
	u_char * schema_start;
	u_char * schema_end;
	u_char * host_start;
	u_char * host_end;
	u_char * port_start;
	u_char * port_end;
};

struct ngx_http_ephemeral_t {
	ngx_http_posted_request_t terminal_posted_request;
};

extern ngx_http_header_t ngx_http_headers_in[];
extern ngx_http_header_out_t ngx_http_headers_out[];

#define ngx_http_ephemeral(r)            (void*)(&r->uri_start)
#define ngx_http_set_log_request(log, r) ((ngx_http_log_ctx_t*)log->data)->current_request = r
//
//#include <ngx_http_script.h>
//
struct ngx_http_script_engine_t {
	u_char * ip;
	u_char * pos;
	ngx_http_variable_value_t  * sp;
	ngx_str_t buf;
	ngx_str_t line;
	/* the start of the rewritten arguments */
	u_char * args;
	unsigned flushed : 1;
	unsigned skip : 1;
	unsigned quote : 1;
	unsigned is_args : 1;
	unsigned log : 1;
	ngx_int_t status;
	ngx_http_request_t  * request;
};

struct ngx_http_script_compile_t {
	ngx_conf_t * cf;
	ngx_str_t  * source;
	ngx_array_t   ** flushes;
	ngx_array_t   ** lengths;
	ngx_array_t   ** values;
	ngx_uint_t variables;
	ngx_uint_t ncaptures;
	ngx_uint_t captures_mask;
	ngx_uint_t size;
	void   * main;
	unsigned compile_args : 1;
	unsigned complete_lengths : 1;
	unsigned complete_values : 1;
	unsigned zero : 1;
	unsigned conf_prefix : 1;
	unsigned root_prefix : 1;
	unsigned dup_capture : 1;
	unsigned args : 1;
};

struct ngx_http_complex_value_t {
	ngx_str_t value;
	ngx_uint_t * flushes;
	void   * lengths;
	void   * values;
};

struct ngx_http_compile_complex_value_t {
	ngx_conf_t * cf;
	ngx_str_t  * value;
	ngx_http_complex_value_t * complex_value;
	unsigned zero : 1;
	unsigned conf_prefix : 1;
	unsigned root_prefix : 1;
};

typedef void (*ngx_http_script_code_pt)(ngx_http_script_engine_t * e);
typedef size_t (*ngx_http_script_len_code_pt)(ngx_http_script_engine_t * e);

struct ngx_http_script_copy_code_t {
	ngx_http_script_code_pt code;
	uintptr_t len;
};

struct ngx_http_script_var_code_t {
	ngx_http_script_code_pt code;
	uintptr_t index;
};

struct ngx_http_script_var_handler_code_t {
	ngx_http_script_code_pt code;
	ngx_http_set_variable_pt handler;
	uintptr_t data;
};

struct ngx_http_script_copy_capture_code_t {
	ngx_http_script_code_pt code;
	uintptr_t n;
};

#if (NGX_PCRE)
	struct ngx_http_script_regex_code_t {
		ngx_http_script_code_pt code;
		ngx_http_regex_t * regex;
		ngx_array_t  * lengths;
		uintptr_t size;
		uintptr_t status;
		uintptr_t next;
		unsigned test : 1;
		unsigned negative_test : 1;
		unsigned uri : 1;
		unsigned args : 1;
		/* add the r->args to the new arguments */
		unsigned add_args : 1;
		unsigned redirect : 1;
		unsigned break_cycle : 1;
		ngx_str_t name;
	};

	struct ngx_http_script_regex_end_code_t {
		ngx_http_script_code_pt code;
		unsigned uri : 1;
		unsigned args : 1;
		/* add the r->args to the new arguments */
		unsigned add_args : 1;
		unsigned redirect : 1;
	};
#endif

struct ngx_http_script_full_name_code_t {
	ngx_http_script_code_pt code;
	uintptr_t conf_prefix;
};

struct ngx_http_script_return_code_t {
	ngx_http_script_code_pt code;
	uintptr_t status;
	ngx_http_complex_value_t text;
};

enum ngx_http_script_file_op_e {
	ngx_http_script_file_plain = 0,
	ngx_http_script_file_not_plain,
	ngx_http_script_file_dir,
	ngx_http_script_file_not_dir,
	ngx_http_script_file_exists,
	ngx_http_script_file_not_exists,
	ngx_http_script_file_exec,
	ngx_http_script_file_not_exec
};

struct ngx_http_script_file_code_t {
	ngx_http_script_code_pt code;
	uintptr_t op;
};

struct ngx_http_script_if_code_t {
	ngx_http_script_code_pt code;
	uintptr_t next;
	void ** loc_conf;
};

struct ngx_http_script_complex_value_code_t {
	ngx_http_script_code_pt code;
	ngx_array_t  * lengths;
};

struct ngx_http_script_value_code_t {
	ngx_http_script_code_pt code;
	uintptr_t value;
	uintptr_t text_len;
	uintptr_t text_data;
};

void ngx_http_script_flush_complex_value(ngx_http_request_t * r, ngx_http_complex_value_t * val);
ngx_int_t ngx_http_complex_value(ngx_http_request_t * r, ngx_http_complex_value_t * val, ngx_str_t * value);
ngx_int_t ngx_http_compile_complex_value(ngx_http_compile_complex_value_t * ccv);
char * ngx_http_set_complex_value_slot(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
ngx_int_t ngx_http_test_predicates(ngx_http_request_t * r, ngx_array_t * predicates);
char * ngx_http_set_predicate_slot(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
ngx_uint_t ngx_http_script_variables_count(ngx_str_t * value);
ngx_int_t ngx_http_script_compile(ngx_http_script_compile_t * sc);
u_char * ngx_http_script_run(ngx_http_request_t * r, ngx_str_t * value, void * code_lengths, size_t reserved, void * code_values);
void ngx_http_script_flush_no_cacheable_variables(ngx_http_request_t * r, ngx_array_t * indices);
void * ngx_http_script_start_code(ngx_pool_t * pool, ngx_array_t ** codes, size_t size);
void * ngx_http_script_add_code(ngx_array_t * codes, size_t size, void * code);
size_t ngx_http_script_copy_len_code(ngx_http_script_engine_t * e);
void ngx_http_script_copy_code(ngx_http_script_engine_t * e);
size_t ngx_http_script_copy_var_len_code(ngx_http_script_engine_t * e);
void ngx_http_script_copy_var_code(ngx_http_script_engine_t * e);
size_t ngx_http_script_copy_capture_len_code(ngx_http_script_engine_t * e);
void ngx_http_script_copy_capture_code(ngx_http_script_engine_t * e);
size_t ngx_http_script_mark_args_code(ngx_http_script_engine_t * e);
void ngx_http_script_start_args_code(ngx_http_script_engine_t * e);
#if (NGX_PCRE)
	void ngx_http_script_regex_start_code(ngx_http_script_engine_t * e);
	void ngx_http_script_regex_end_code(ngx_http_script_engine_t * e);
#endif
void ngx_http_script_return_code(ngx_http_script_engine_t * e);
void ngx_http_script_break_code(ngx_http_script_engine_t * e);
void ngx_http_script_if_code(ngx_http_script_engine_t * e);
void ngx_http_script_equal_code(ngx_http_script_engine_t * e);
void ngx_http_script_not_equal_code(ngx_http_script_engine_t * e);
void ngx_http_script_file_code(ngx_http_script_engine_t * e);
void ngx_http_script_complex_value_code(ngx_http_script_engine_t * e);
void ngx_http_script_value_code(ngx_http_script_engine_t * e);
void ngx_http_script_set_var_code(ngx_http_script_engine_t * e);
void ngx_http_script_var_set_handler_code(ngx_http_script_engine_t * e);
void ngx_http_script_var_code(ngx_http_script_engine_t * e);
void ngx_http_script_nop_code(ngx_http_script_engine_t * e);
//
//#include <ngx_http_upstream.h>
//
//
//#include <ngx_event.h>
//
#define NGX_INVALID_INDEX  0xd0d0d0d0

#if (NGX_HAVE_IOCP)

struct ngx_event_ovlp_t {
	WSAOVERLAPPED ovlp;
	ngx_event_t * event;
	int error;
};

#endif

struct /*ngx_event_s*/ngx_event_t {
	void  * P_Data;
	unsigned write : 1;
	unsigned accept : 1;
	unsigned instance : 1; // used to detect the stale events in kqueue and epoll 
	// 
	// the event was passed or would be passed to a kernel;
	// in aio mode - operation was posted.
	// 
	unsigned active : 1;
	unsigned disabled : 1;
	unsigned ready : 1; // the ready event; in aio mode 0 means that no operation can be posted 
	unsigned oneshot : 1;
	unsigned complete : 1; // aio operation is complete 
	unsigned eof : 1;
	unsigned error : 1;
	unsigned timedout : 1;
	unsigned timer_set : 1;
	unsigned delayed : 1;
	unsigned deferred_accept : 1;
	unsigned pending_eof : 1; // the pending eof reported by kqueue, epoll or in aio chain operation 
	unsigned posted : 1;
	unsigned closed : 1;
	// to test on worker exit 
	unsigned channel : 1;
	unsigned resolver : 1;
	unsigned cancelable : 1;
#if (NGX_HAVE_KQUEUE)
	unsigned kq_vnode : 1;
	int kq_errno; // the pending errno reported by kqueue 
#endif
	/*
	 * kqueue only:
	 *   accept:     number of sockets that wait to be accepted
	 *   read:       bytes to read when event is ready or lowat when event is set with NGX_LOWAT_EVENT flag
	 *   write:      available space in buffer when event is ready or lowat when event is set with NGX_LOWAT_EVENT flag
	 * epoll with EPOLLRDHUP:
	 *   accept:     1 if accept many, 0 otherwise
	 *   read:       1 if there can be data to read, 0 otherwise
	 * iocp: TODO
	 * otherwise:
	 *   accept:     1 if accept many, 0 otherwise
	 */
#if (NGX_HAVE_KQUEUE) || (NGX_HAVE_IOCP)
	int available;
#else
	unsigned available : 1;
#endif
	ngx_event_handler_pt handler;
#if (NGX_HAVE_IOCP)
	ngx_event_ovlp_t ovlp;
#endif
	ngx_uint_t index;
	ngx_log_t  * log;
	ngx_rbtree_node_t timer;
	ngx_queue_t queue; // the posted queue 
#if 0
	/* the threads support */
	/*
	 * the event thread context, we store it here
	 * if $(CC) does not understand __thread declaration
	 * and pthread_getspecific() is too costly
	 */
	void  * thr_ctx;
#if (NGX_EVENT_T_PADDING)
	uint32_t padding[NGX_EVENT_T_PADDING]; // event should not cross cache line in SMP 
#endif
#endif
};

#if (NGX_HAVE_FILE_AIO)

struct ngx_event_aio_s {
	void  * data;
	ngx_event_handler_pt handler;
	ngx_file_t  * file;
#if (NGX_HAVE_AIO_SENDFILE || NGX_COMPAT)
	ssize_t (* preload_handler)(ngx_buf_t * file);
#endif
	ngx_fd_t fd;
#if (NGX_HAVE_EVENTFD)
	int64_t res;
#endif
#if !(NGX_HAVE_EVENTFD) || (NGX_TEST_BUILD_EPOLL)
	ngx_err_t err;
	size_t nbytes;
#endif
	ngx_aiocb_t aiocb;
	ngx_event_t event;
};

#endif

struct ngx_event_actions_t {
	ngx_int_t (* F_Add)(ngx_event_t * ev, ngx_int_t event, ngx_uint_t flags); // Is called only by ngx_add_event() macro
	ngx_int_t (* F_Delete)(ngx_event_t * ev, ngx_int_t event, ngx_uint_t flags);
	ngx_int_t (* enable)(ngx_event_t * ev, ngx_int_t event, ngx_uint_t flags);
	ngx_int_t (* disable)(ngx_event_t * ev, ngx_int_t event, ngx_uint_t flags);
	ngx_int_t (* F_AddConn)(ngx_connection_t * c);
	ngx_int_t (* F_DelConn)(ngx_connection_t * c, ngx_uint_t flags);
	ngx_int_t (* F_Notify)(ngx_event_handler_pt handler);
	ngx_int_t (* F_ProcessEvents)(ngx_cycle_t * cycle, ngx_msec_t timer, ngx_uint_t flags);
	ngx_int_t (* F_Init)(ngx_cycle_t * cycle, ngx_msec_t timer);
	void (* F_Done)(ngx_cycle_t * cycle);
};

extern ngx_event_actions_t ngx_event_actions; // @global
#if (NGX_HAVE_EPOLLRDHUP)
	extern ngx_uint_t ngx_use_epoll_rdhup;
#endif
#define NGX_USE_LEVEL_EVENT      0x00000001 // The event filter requires to read/write the whole data: select, poll, /dev/poll, kqueue, epoll.
#define NGX_USE_ONESHOT_EVENT    0x00000002 // The event filter is deleted after a notification without an additional syscall: kqueue, epoll.
#define NGX_USE_CLEAR_EVENT      0x00000004 // The event filter notifies only the changes and an initial level: kqueue, epoll.
#define NGX_USE_KQUEUE_EVENT     0x00000008 // The event filter has kqueue features: the eof flag, errno, available data, etc.
/*
 * The event filter supports low water mark: kqueue's NOTE_LOWAT.
 * kqueue in FreeBSD 4.1-4.2 has no NOTE_LOWAT so we need a separate flag.
 */
#define NGX_USE_LOWAT_EVENT      0x00000010
#define NGX_USE_GREEDY_EVENT     0x00000020 // The event filter requires to do i/o operation until EAGAIN: epoll.
#define NGX_USE_EPOLL_EVENT      0x00000040 // The event filter is epoll
#define NGX_USE_RTSIG_EVENT      0x00000080 // Obsolete
#define NGX_USE_AIO_EVENT        0x00000100 // Obsolete
#define NGX_USE_IOCP_EVENT       0x00000200 // Need to add socket or handle only once: i/o completion port.
#define NGX_USE_FD_EVENT         0x00000400 // The event filter has no opaque data and requires file descriptors table: poll, /dev/poll.
/*
 * The event module handles periodic or absolute timer event by itself:
 * kqueue in FreeBSD 4.4, NetBSD 2.0, and MacOSX 10.4, Solaris 10's event ports.
 */
#define NGX_USE_TIMER_EVENT      0x00000800
/*
 * All event filters on file descriptor are deleted after a notification:
 * Solaris 10's event ports.
 */
#define NGX_USE_EVENTPORT_EVENT  0x00001000
#define NGX_USE_VNODE_EVENT      0x00002000 // The event filter support vnode notifications: kqueue
/*
 * The event filter is deleted just before the closing file.
 * Has no meaning for select and poll.
 * kqueue, epoll, eventport:         allows to avoid explicit delete,
 *                                   because filter automatically is deleted
 *                                   on file close,
 *
 * /dev/poll:                        we need to flush POLLREMOVE event
 *                                   before closing file.
 */
#define NGX_CLOSE_EVENT    1
#define NGX_DISABLE_EVENT  2 // disable temporarily event filter, this may avoid locks in kernel malloc()/free(): kqueue.
#define NGX_FLUSH_EVENT    4 // event must be passed to kernel right now, do not wait until batch processing.
//
// these flags have a meaning only for kqueue 
//
#define NGX_LOWAT_EVENT    0
#define NGX_VNODE_EVENT    0

#if (NGX_HAVE_EPOLL) && !(NGX_HAVE_EPOLLRDHUP)
	#define EPOLLRDHUP         0
#endif

#if (NGX_HAVE_KQUEUE)
#define NGX_READ_EVENT     EVFILT_READ
#define NGX_WRITE_EVENT    EVFILT_WRITE
#undef  NGX_VNODE_EVENT
#define NGX_VNODE_EVENT    EVFILT_VNODE
/*
 * NGX_CLOSE_EVENT, NGX_LOWAT_EVENT, and NGX_FLUSH_EVENT are the module flags
 * and they must not go into a kernel so we need to choose the value
 * that must not interfere with any existent and future kqueue flags.
 * kqueue has such values - EV_FLAG1, EV_EOF, and EV_ERROR:
 * they are reserved and cleared on a kernel entrance.
 */
#undef  NGX_CLOSE_EVENT
#define NGX_CLOSE_EVENT    EV_EOF
#undef  NGX_LOWAT_EVENT
#define NGX_LOWAT_EVENT    EV_FLAG1
#undef  NGX_FLUSH_EVENT
#define NGX_FLUSH_EVENT    EV_ERROR
#define NGX_LEVEL_EVENT    0
#define NGX_ONESHOT_EVENT  EV_ONESHOT
#define NGX_CLEAR_EVENT    EV_CLEAR
#undef  NGX_DISABLE_EVENT
#define NGX_DISABLE_EVENT  EV_DISABLE

#elif (NGX_HAVE_DEVPOLL && !(NGX_TEST_BUILD_DEVPOLL)) || (NGX_HAVE_EVENTPORT && !(NGX_TEST_BUILD_EVENTPORT))
	#define NGX_READ_EVENT     POLLIN
	#define NGX_WRITE_EVENT    POLLOUT
	#define NGX_LEVEL_EVENT    0
	#define NGX_ONESHOT_EVENT  1
#elif (NGX_HAVE_EPOLL) && !(NGX_TEST_BUILD_EPOLL)
	#define NGX_READ_EVENT     (EPOLLIN|EPOLLRDHUP)
	#define NGX_WRITE_EVENT    EPOLLOUT
	#define NGX_LEVEL_EVENT    0
	#define NGX_CLEAR_EVENT    EPOLLET
	#define NGX_ONESHOT_EVENT  0x70000000
	#if 0
		#define NGX_ONESHOT_EVENT  EPOLLONESHOT
	#endif
	#if (NGX_HAVE_EPOLLEXCLUSIVE)
		#define NGX_EXCLUSIVE_EVENT  EPOLLEXCLUSIVE
	#endif
#elif (NGX_HAVE_POLL)
	#define NGX_READ_EVENT     POLLIN
	#define NGX_WRITE_EVENT    POLLOUT
	#define NGX_LEVEL_EVENT    0
	#define NGX_ONESHOT_EVENT  1
#else /* select */
	#define NGX_READ_EVENT     0
	#define NGX_WRITE_EVENT    1
	#define NGX_LEVEL_EVENT    0
	#define NGX_ONESHOT_EVENT  1
#endif /* NGX_HAVE_KQUEUE */
#if (NGX_HAVE_IOCP)
	#define NGX_IOCP_ACCEPT      0
	#define NGX_IOCP_IO          1
	#define NGX_IOCP_CONNECT     2
#endif
#if (NGX_TEST_BUILD_EPOLL)
	#define NGX_EXCLUSIVE_EVENT  0
#endif
#ifndef NGX_CLEAR_EVENT
	#define NGX_CLEAR_EVENT    0    /* dummy declaration */
#endif
// @sobolev #define ngx_process_events   ngx_event_actions.process_events
#define ngx_done_events      ngx_event_actions.F_Done
#define ngx_add_event        ngx_event_actions.F_Add
#define ngx_del_event        ngx_event_actions.F_Delete
#define ngx_add_conn         ngx_event_actions.F_AddConn
#define ngx_del_conn         ngx_event_actions.F_DelConn
#define ngx_notify           ngx_event_actions.F_Notify
#define ngx_add_timer        ngx_event_add_timer
#define ngx_del_timer        ngx_event_del_timer

extern ngx_os_io_t ngx_io;

#define ngx_recv             ngx_io.recv
#define ngx_recv_chain       ngx_io.recv_chain
#define ngx_udp_recv         ngx_io.udp_recv
#define ngx_send             ngx_io.send
#define ngx_send_chain       ngx_io.send_chain
#define ngx_udp_send         ngx_io.udp_send
#define ngx_udp_send_chain   ngx_io.udp_send_chain

#define NGX_EVENT_MODULE      0x544E5645  /* "EVNT" */
#define NGX_EVENT_CONF        0x02000000

struct ngx_event_conf_t {
	ngx_uint_t connections;
	ngx_uint_t use;
	ngx_flag_t multi_accept;
	ngx_flag_t accept_mutex;
	ngx_msec_t accept_mutex_delay;
	u_char  * name;
#if (NGX_DEBUG)
	ngx_array_t debug_connection;
#endif
};

struct ngx_event_module_t {
	ngx_str_t * name;
	void *(*create_conf)(ngx_cycle_t *cycle);
	char *(*init_conf)(ngx_cycle_t *cycle, void * conf);
	ngx_event_actions_t actions;
};

extern ngx_atomic_t * ngx_connection_counter;
extern ngx_atomic_t * ngx_accept_mutex_ptr;
extern ngx_shmtx_t ngx_accept_mutex;
extern ngx_uint_t ngx_use_accept_mutex;
extern ngx_uint_t ngx_accept_events;
extern ngx_uint_t ngx_accept_mutex_held;
extern ngx_msec_t ngx_accept_mutex_delay;
extern ngx_int_t ngx_accept_disabled;

#if (NGX_STAT_STUB)
extern ngx_atomic_t  * ngx_stat_accepted;
extern ngx_atomic_t  * ngx_stat_handled;
extern ngx_atomic_t  * ngx_stat_requests;
extern ngx_atomic_t  * ngx_stat_active;
extern ngx_atomic_t  * ngx_stat_reading;
extern ngx_atomic_t  * ngx_stat_writing;
extern ngx_atomic_t  * ngx_stat_waiting;
#endif

#define NGX_UPDATE_TIME         1
#define NGX_POST_EVENTS         2

extern sig_atomic_t ngx_event_timer_alarm;
extern ngx_uint_t ngx_event_flags;
extern ngx_module_t ngx_events_module;
extern ngx_module_t ngx_event_core_module;

#define ngx_event_get_conf(conf_ctx, module) (*(ngx_get_conf(conf_ctx, ngx_events_module)))[module.ctx_index];

void ngx_event_accept(ngx_event_t * ev);
#if !(NGX_WIN32)
	void ngx_event_recvmsg(ngx_event_t * ev);
#endif
ngx_int_t ngx_trylock_accept_mutex(ngx_cycle_t * cycle);
u_char * ngx_accept_log_error(ngx_log_t * log, u_char * buf, size_t len);

void FASTCALL ngx_process_events_and_timers(ngx_cycle_t * pCycle);
ngx_int_t FASTCALL ngx_handle_read_event(ngx_event_t * rev, ngx_uint_t flags);
ngx_int_t FASTCALL ngx_handle_write_event(ngx_event_t * wev, size_t lowat);

#if (NGX_WIN32)
	void ngx_event_acceptex(ngx_event_t * ev);
	ngx_int_t ngx_event_post_acceptex(ngx_listening_t * ls, ngx_uint_t n);
	u_char * ngx_acceptex_log_error(ngx_log_t * log, u_char * buf, size_t len);
#endif

ngx_int_t ngx_send_lowat(ngx_connection_t * c, size_t lowat);
/* used in ngx_log_debugX() */
#define ngx_event_ident(p)  ((ngx_connection_t*)(p))->fd
//
//#include <ngx_event_timer.h>
//
#define NGX_TIMER_INFINITE  (ngx_msec_t)-1
#define NGX_TIMER_LAZY_DELAY  300

ngx_int_t ngx_event_timer_init(ngx_log_t * log);
ngx_msec_t ngx_event_find_timer(void);
void ngx_event_expire_timers(void);
ngx_int_t ngx_event_no_timers_left(void);

extern ngx_rbtree_t ngx_event_timer_rbtree;

void FASTCALL ngx_event_del_timer(ngx_event_t * ev);
void FASTCALL ngx_event_add_timer(ngx_event_t * ev, ngx_msec_t timer);
//
//#include <ngx_event_posted.h>
//
#define ngx_post_event(ev, q)						      \
	if(!(ev)->posted) {							 \
		(ev)->posted = 1;						      \
		ngx_queue_insert_tail(q, &(ev)->queue);				      \
		ngx_log_debug1(NGX_LOG_DEBUG_CORE, (ev)->log, 0, "post event %p", ev); \
	} \
	else  {								  \
		ngx_log_debug1(NGX_LOG_DEBUG_CORE, (ev)->log, 0, "update posted event %p", ev); \
	}

#define ngx_delete_posted_event(ev)					      \
	(ev)->posted = 0;							  \
	ngx_queue_remove(&(ev)->queue);						  \
	ngx_log_debug1(NGX_LOG_DEBUG_CORE, (ev)->log, 0, "delete posted event %p", ev);

// (moved to ngx_event.c as static) void ngx_event_process_posted(ngx_cycle_t * cycle, ngx_queue_t * posted);

extern ngx_queue_t ngx_posted_accept_events;
extern ngx_queue_t ngx_posted_events;
//
#if (NGX_WIN32)
	//
	//#include <ngx_iocp_module.h>
	//
	typedef struct {
		int threads;
		int post_acceptex;
		int acceptex_read;
	} ngx_iocp_conf_t;

	extern ngx_module_t ngx_iocp_module;
	//
#endif
//
//#include <ngx_event_connect.h>
//
#define NGX_PEER_KEEPALIVE           1
#define NGX_PEER_NEXT                2
#define NGX_PEER_FAILED              4

//typedef struct ngx_peer_connection_s ngx_peer_connection_t;
struct ngx_peer_connection_t;

typedef ngx_int_t (*ngx_event_get_peer_pt)(ngx_peer_connection_t * pc, void * data);
typedef void (*ngx_event_free_peer_pt)(ngx_peer_connection_t * pc, void * data, ngx_uint_t state);
typedef void (*ngx_event_notify_peer_pt)(ngx_peer_connection_t * pc, void * data, ngx_uint_t type);
typedef ngx_int_t (*ngx_event_set_peer_session_pt)(ngx_peer_connection_t * pc, void * data);
typedef void (*ngx_event_save_peer_session_pt)(ngx_peer_connection_t * pc, void * data);

struct ngx_peer_connection_t {
	ngx_connection_t * connection;
	struct sockaddr  * sockaddr;
	socklen_t socklen;
	ngx_str_t * name;
	ngx_uint_t tries;
	ngx_msec_t start_time;
	ngx_event_get_peer_pt get;
	ngx_event_free_peer_pt free;
	ngx_event_notify_peer_pt notify;
	void * data;
#if (NGX_SSL || NGX_COMPAT)
	ngx_event_set_peer_session_pt set_session;
	ngx_event_save_peer_session_pt save_session;
#endif
	ngx_addr_t * local;
	int type;
	int rcvbuf;
	ngx_log_t * log;
	unsigned cached : 1;
	unsigned transparent : 1;
	unsigned log_error : 2; // ngx_connection_log_error_e 
	NGX_COMPAT_BEGIN(2)
	NGX_COMPAT_END
};

ngx_int_t ngx_event_connect_peer(ngx_peer_connection_t * pc);
ngx_int_t ngx_event_get_peer(ngx_peer_connection_t * pc, void * data);
//
//#include <ngx_event_pipe.h>
//
//typedef struct ngx_event_pipe_s ngx_event_pipe_t;
struct /*ngx_event_pipe_s*/ngx_event_pipe_t;

typedef ngx_int_t (*ngx_event_pipe_input_filter_pt)(ngx_event_pipe_t * p, ngx_buf_t * buf);
typedef ngx_int_t (*ngx_event_pipe_output_filter_pt)(void * data, ngx_chain_t * chain);

struct /*ngx_event_pipe_s*/ngx_event_pipe_t {
	ngx_connection_t * upstream;
	ngx_connection_t * downstream;
	ngx_chain_t  * free_raw_bufs;
	ngx_chain_t  * in;
	ngx_chain_t ** last_in;
	ngx_chain_t  * writing;
	ngx_chain_t  * out;
	ngx_chain_t  * free;
	ngx_chain_t  * busy;
	/*
	 * the input filter i.e. that moves HTTP/1.1 chunks
	 * from the raw bufs to an incoming chain
	 */
	ngx_event_pipe_input_filter_pt input_filter;
	void * input_ctx;
	ngx_event_pipe_output_filter_pt output_filter;
	void * output_ctx;
#if (NGX_THREADS || NGX_COMPAT)
	ngx_int_t (* thread_handler)(ngx_thread_task_t * task, ngx_file_t * file);
	void * thread_ctx;
	ngx_thread_task_t  * thread_task;
#endif
	unsigned read : 1;
	unsigned cacheable : 1;
	unsigned single_buf : 1;
	unsigned free_bufs : 1;
	unsigned upstream_done : 1;
	unsigned upstream_error : 1;
	unsigned upstream_eof : 1;
	unsigned upstream_blocked : 1;
	unsigned downstream_done : 1;
	unsigned downstream_error : 1;
	unsigned cyclic_temp_file : 1;
	unsigned aio : 1;
	ngx_int_t allocated;
	ngx_bufs_t bufs;
	ngx_buf_tag_t tag;
	ssize_t busy_size;
	nginx_off_t read_length;
	nginx_off_t length;
	nginx_off_t max_temp_file_size;
	ssize_t temp_file_write_size;
	ngx_msec_t read_timeout;
	ngx_msec_t send_timeout;
	ssize_t send_lowat;
	ngx_pool_t * pool;
	ngx_log_t  * log;
	ngx_chain_t  * preread_bufs;
	size_t preread_size;
	ngx_buf_t  * buf_to_file;
	size_t limit_rate;
	time_t start_sec;
	ngx_temp_file_t * temp_file;
	/* STUB */ int num;
};

ngx_int_t ngx_event_pipe(ngx_event_pipe_t * p, ngx_int_t do_write);
ngx_int_t ngx_event_pipe_copy_input_filter(ngx_event_pipe_t * p, ngx_buf_t * buf);
ngx_int_t ngx_event_pipe_add_free_buf(ngx_event_pipe_t * p, ngx_buf_t * b);
//
#define NGX_HTTP_UPSTREAM_FT_ERROR           0x00000002
#define NGX_HTTP_UPSTREAM_FT_TIMEOUT         0x00000004
#define NGX_HTTP_UPSTREAM_FT_INVALID_HEADER  0x00000008
#define NGX_HTTP_UPSTREAM_FT_HTTP_500        0x00000010
#define NGX_HTTP_UPSTREAM_FT_HTTP_502        0x00000020
#define NGX_HTTP_UPSTREAM_FT_HTTP_503        0x00000040
#define NGX_HTTP_UPSTREAM_FT_HTTP_504        0x00000080
#define NGX_HTTP_UPSTREAM_FT_HTTP_403        0x00000100
#define NGX_HTTP_UPSTREAM_FT_HTTP_404        0x00000200
#define NGX_HTTP_UPSTREAM_FT_HTTP_429        0x00000400
#define NGX_HTTP_UPSTREAM_FT_UPDATING        0x00000800
#define NGX_HTTP_UPSTREAM_FT_BUSY_LOCK       0x00001000
#define NGX_HTTP_UPSTREAM_FT_MAX_WAITING     0x00002000
#define NGX_HTTP_UPSTREAM_FT_NON_IDEMPOTENT  0x00004000
#define NGX_HTTP_UPSTREAM_FT_NOLIVE          0x40000000
#define NGX_HTTP_UPSTREAM_FT_OFF             0x80000000

#define NGX_HTTP_UPSTREAM_FT_STATUS          (NGX_HTTP_UPSTREAM_FT_HTTP_500|NGX_HTTP_UPSTREAM_FT_HTTP_502|NGX_HTTP_UPSTREAM_FT_HTTP_503|NGX_HTTP_UPSTREAM_FT_HTTP_504|\
	NGX_HTTP_UPSTREAM_FT_HTTP_403|NGX_HTTP_UPSTREAM_FT_HTTP_404|NGX_HTTP_UPSTREAM_FT_HTTP_429)

#define NGX_HTTP_UPSTREAM_INVALID_HEADER     40

#define NGX_HTTP_UPSTREAM_IGN_XA_REDIRECT    0x00000002
#define NGX_HTTP_UPSTREAM_IGN_XA_EXPIRES     0x00000004
#define NGX_HTTP_UPSTREAM_IGN_EXPIRES        0x00000008
#define NGX_HTTP_UPSTREAM_IGN_CACHE_CONTROL  0x00000010
#define NGX_HTTP_UPSTREAM_IGN_SET_COOKIE     0x00000020
#define NGX_HTTP_UPSTREAM_IGN_XA_LIMIT_RATE  0x00000040
#define NGX_HTTP_UPSTREAM_IGN_XA_BUFFERING   0x00000080
#define NGX_HTTP_UPSTREAM_IGN_XA_CHARSET     0x00000100
#define NGX_HTTP_UPSTREAM_IGN_VARY           0x00000200

struct ngx_http_upstream_state_t {
	ngx_uint_t status;
	ngx_msec_t response_time;
	ngx_msec_t connect_time;
	ngx_msec_t header_time;
	nginx_off_t response_length;
	nginx_off_t bytes_received;
	ngx_str_t   * peer;
};

struct ngx_http_upstream_main_conf_t {
	ngx_hash_t headers_in_hash;
	ngx_array_t upstreams;
	/* ngx_http_upstream_srv_conf_t */
};

typedef struct ngx_http_upstream_srv_conf_s ngx_http_upstream_srv_conf_t;

typedef ngx_int_t (*ngx_http_upstream_init_pt)(ngx_conf_t * cf, ngx_http_upstream_srv_conf_t * us);
typedef ngx_int_t (*ngx_http_upstream_init_peer_pt)(ngx_http_request_t * r, ngx_http_upstream_srv_conf_t * us);

struct ngx_http_upstream_peer_t {
	ngx_http_upstream_init_pt init_upstream;
	ngx_http_upstream_init_peer_pt init;
	void * data;
};

struct ngx_http_upstream_server_t {
	ngx_str_t name;
	ngx_addr_t * addrs;
	ngx_uint_t naddrs;
	ngx_uint_t weight;
	ngx_uint_t max_conns;
	ngx_uint_t max_fails;
	time_t fail_timeout;
	ngx_msec_t slow_start;
	unsigned down : 1;
	unsigned backup : 1;
	NGX_COMPAT_BEGIN(6)
	NGX_COMPAT_END
};

#define NGX_HTTP_UPSTREAM_CREATE        0x0001
#define NGX_HTTP_UPSTREAM_WEIGHT        0x0002
#define NGX_HTTP_UPSTREAM_MAX_FAILS     0x0004
#define NGX_HTTP_UPSTREAM_FAIL_TIMEOUT  0x0008
#define NGX_HTTP_UPSTREAM_DOWN          0x0010
#define NGX_HTTP_UPSTREAM_BACKUP        0x0020
#define NGX_HTTP_UPSTREAM_MAX_CONNS     0x0100

struct ngx_http_upstream_srv_conf_s {
	ngx_http_upstream_peer_t peer;
	void   ** srv_conf;
	ngx_array_t * servers; /* ngx_http_upstream_server_t */
	ngx_uint_t flags;
	ngx_str_t host;
	u_char * file_name;
	ngx_uint_t line;
	in_port_t port;
	ngx_uint_t no_port;                    /* unsigned no_port:1 */
#if (NGX_HTTP_UPSTREAM_ZONE)
	ngx_shm_zone_t  * shm_zone;
#endif
};

struct ngx_http_upstream_local_t {
	ngx_addr_t * addr;
	ngx_http_complex_value_t * value;
#if (NGX_HAVE_TRANSPARENT_PROXY)
	ngx_uint_t transparent; /* unsigned  transparent:1; */
#endif
};

struct ngx_http_upstream_conf_t {
	ngx_http_upstream_srv_conf_t * upstream;
	ngx_msec_t connect_timeout;
	ngx_msec_t send_timeout;
	ngx_msec_t read_timeout;
	ngx_msec_t next_upstream_timeout;
	size_t send_lowat;
	size_t buffer_size;
	size_t limit_rate;
	size_t busy_buffers_size;
	size_t max_temp_file_size;
	size_t temp_file_write_size;
	size_t busy_buffers_size_conf;
	size_t max_temp_file_size_conf;
	size_t temp_file_write_size_conf;
	ngx_bufs_t bufs;
	ngx_uint_t ignore_headers;
	ngx_uint_t next_upstream;
	ngx_uint_t store_access;
	ngx_uint_t next_upstream_tries;
	ngx_flag_t buffering;
	ngx_flag_t request_buffering;
	ngx_flag_t pass_request_headers;
	ngx_flag_t pass_request_body;
	ngx_flag_t ignore_client_abort;
	ngx_flag_t intercept_errors;
	ngx_flag_t cyclic_temp_file;
	ngx_flag_t force_ranges;
	ngx_path_t * temp_path;
	ngx_hash_t hide_headers_hash;
	ngx_array_t * hide_headers;
	ngx_array_t * pass_headers;
	ngx_http_upstream_local_t  * local;
#if (NGX_HTTP_CACHE)
	ngx_shm_zone_t * cache_zone;
	ngx_http_complex_value_t * cache_value;
	ngx_uint_t cache_min_uses;
	ngx_uint_t cache_use_stale;
	ngx_uint_t cache_methods;
	nginx_off_t cache_max_range_offset;
	ngx_flag_t cache_lock;
	ngx_msec_t cache_lock_timeout;
	ngx_msec_t cache_lock_age;
	ngx_flag_t cache_revalidate;
	ngx_flag_t cache_convert_head;
	ngx_flag_t cache_background_update;
	ngx_array_t * cache_valid;
	ngx_array_t * cache_bypass;
	ngx_array_t * cache_purge;
	ngx_array_t * no_cache;
#endif
	ngx_array_t * store_lengths;
	ngx_array_t * store_values;
#if (NGX_HTTP_CACHE)
	signed cache : 2;
#endif
	signed store : 2;
	unsigned intercept_404 : 1;
	unsigned change_buffering : 1;
#if (NGX_HTTP_SSL || NGX_COMPAT)
	ngx_ssl_t * ssl;
	ngx_flag_t ssl_session_reuse;
	ngx_http_complex_value_t * ssl_name;
	ngx_flag_t ssl_server_name;
	ngx_flag_t ssl_verify;
#endif
	ngx_str_t module;
	NGX_COMPAT_BEGIN(2)
	NGX_COMPAT_END
};

struct ngx_http_upstream_header_t {
	ngx_str_t name;
	ngx_http_header_handler_pt handler;
	ngx_uint_t offset;
	ngx_http_header_handler_pt copy_handler;
	ngx_uint_t conf;
	ngx_uint_t redirect;                    /* unsigned   redirect:1; */
};

struct ngx_http_upstream_headers_in_t {
	ngx_list_t headers;
	ngx_uint_t status_n;
	ngx_str_t status_line;
	ngx_table_elt_t * status;
	ngx_table_elt_t * date;
	ngx_table_elt_t * server;
	ngx_table_elt_t * connection;
	ngx_table_elt_t * expires;
	ngx_table_elt_t * etag;
	ngx_table_elt_t * x_accel_expires;
	ngx_table_elt_t * x_accel_redirect;
	ngx_table_elt_t * x_accel_limit_rate;
	ngx_table_elt_t * content_type;
	ngx_table_elt_t * content_length;
	ngx_table_elt_t * last_modified;
	ngx_table_elt_t * location;
	ngx_table_elt_t * accept_ranges;
	ngx_table_elt_t * www_authenticate;
	ngx_table_elt_t * transfer_encoding;
	ngx_table_elt_t * vary;
#if (NGX_HTTP_GZIP)
	ngx_table_elt_t * content_encoding;
#endif
	ngx_array_t cache_control;
	ngx_array_t cookies;
	nginx_off_t content_length_n;
	time_t last_modified_time;
	unsigned connection_close : 1;
	unsigned chunked : 1;
};

struct ngx_http_upstream_resolved_t {
	ngx_str_t host;
	in_port_t port;
	ngx_uint_t no_port;                   /* unsigned no_port:1 */
	ngx_uint_t naddrs;
	ngx_resolver_addr_t * addrs;
	struct sockaddr * sockaddr;
	socklen_t socklen;
	ngx_str_t name;
	ngx_resolver_ctx_t    * ctx;
};

typedef void (*ngx_http_upstream_handler_pt)(ngx_http_request_t * r, ngx_http_upstream_t * u);

struct ngx_http_upstream_s {
	ngx_http_upstream_handler_pt read_event_handler;
	ngx_http_upstream_handler_pt write_event_handler;
	ngx_peer_connection_t peer;
	ngx_event_pipe_t  * pipe;
	ngx_chain_t * request_bufs;
	ngx_output_chain_ctx_t output;
	ngx_chain_writer_ctx_t writer;
	ngx_http_upstream_conf_t * conf;
	ngx_http_upstream_srv_conf_t  * upstream;
#if (NGX_HTTP_CACHE)
	ngx_array_t * caches;
#endif
	ngx_http_upstream_headers_in_t headers_in;
	ngx_http_upstream_resolved_t  * resolved;
	ngx_buf_t from_client;
	ngx_buf_t buffer;
	nginx_off_t length;
	ngx_chain_t * out_bufs;
	ngx_chain_t * busy_bufs;
	ngx_chain_t * free_bufs;
	ngx_int_t (* input_filter_init)(void * data);
	ngx_int_t (* input_filter)(void * data, ssize_t bytes);
	void * input_filter_ctx;
#if (NGX_HTTP_CACHE)
	ngx_int_t (* create_key)(ngx_http_request_t * r);
#endif
	ngx_int_t (* create_request)(ngx_http_request_t * r);
	ngx_int_t (* reinit_request)(ngx_http_request_t * r);
	ngx_int_t (* process_header)(ngx_http_request_t * r);
	void (* abort_request)(ngx_http_request_t * r);
	void (* finalize_request)(ngx_http_request_t * r, ngx_int_t rc);
	ngx_int_t (* rewrite_redirect)(ngx_http_request_t * r, ngx_table_elt_t * h, size_t prefix);
	ngx_int_t (* rewrite_cookie)(ngx_http_request_t * r, ngx_table_elt_t * h);
	ngx_msec_t timeout;
	ngx_http_upstream_state_t  * state;
	ngx_str_t method;
	ngx_str_t schema;
	ngx_str_t uri;
#if (NGX_HTTP_SSL || NGX_COMPAT)
	ngx_str_t ssl_name;
#endif
	ngx_http_cleanup_pt   * cleanup;
	unsigned store : 1;
	unsigned cacheable : 1;
	unsigned accel : 1;
	unsigned ssl : 1;
#if (NGX_HTTP_CACHE)
	unsigned cache_status : 3;
#endif
	unsigned buffering : 1;
	unsigned keepalive : 1;
	unsigned upgrade : 1;
	unsigned request_sent : 1;
	unsigned request_body_sent : 1;
	unsigned header_sent : 1;
};

struct ngx_http_upstream_next_t {
	ngx_uint_t status;
	ngx_uint_t mask;
};

struct ngx_http_upstream_param_t {
	ngx_str_t key;
	ngx_str_t value;
	ngx_uint_t skip_empty;
};

ngx_int_t ngx_http_upstream_create(ngx_http_request_t * r);
void ngx_http_upstream_init(ngx_http_request_t * r);
ngx_http_upstream_srv_conf_t * ngx_http_upstream_add(ngx_conf_t * cf, ngx_url_t * u, ngx_uint_t flags);
char * ngx_http_upstream_bind_set_slot(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
char * ngx_http_upstream_param_set_slot(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
ngx_int_t ngx_http_upstream_hide_headers_hash(ngx_conf_t * cf, ngx_http_upstream_conf_t * conf, ngx_http_upstream_conf_t * prev, ngx_str_t * default_hide_headers, ngx_hash_init_t * hash);

#define ngx_http_conf_upstream_srv_conf(uscf, module) uscf->srv_conf[module.ctx_index]

extern ngx_module_t ngx_http_upstream_module;
extern ngx_conf_bitmask_t ngx_http_upstream_cache_method_mask[];
extern ngx_conf_bitmask_t ngx_http_upstream_ignore_headers_masks[];
//
#include <ngx_http_upstream_round_robin.h>
#include <ngx_http_core_module.h>
#if (NGX_HTTP_V2)
	#include <ngx_http_v2.h>
#endif
#if (NGX_HTTP_CACHE)
	#include <ngx_http_cache.h>
#endif
#if (NGX_HTTP_SSI)
	#include <ngx_http_ssi_filter_module.h>
#endif
#if (NGX_HTTP_SSL)
	//
	//#include <ngx_http_ssl_module.h>
	//
	struct ngx_http_ssl_srv_conf_t {
		ngx_flag_t enable;
		ngx_ssl_t ssl;
		ngx_flag_t prefer_server_ciphers;
		ngx_uint_t protocols;
		ngx_uint_t verify;
		ngx_uint_t verify_depth;
		size_t buffer_size;
		ssize_t builtin_session_cache;
		time_t session_timeout;
		ngx_array_t * certificates;
		ngx_array_t * certificate_keys;
		ngx_str_t dhparam;
		ngx_str_t ecdh_curve;
		ngx_str_t client_certificate;
		ngx_str_t trusted_certificate;
		ngx_str_t crl;
		ngx_str_t ciphers;
		ngx_array_t * passwords;
		ngx_shm_zone_t * shm_zone;
		ngx_flag_t session_tickets;
		ngx_array_t * session_ticket_keys;
		ngx_flag_t stapling;
		ngx_flag_t stapling_verify;
		ngx_str_t stapling_file;
		ngx_str_t stapling_responder;
		u_char * file;
		ngx_uint_t line;
	};

	extern ngx_module_t ngx_http_ssl_module;	
	//
#endif

struct ngx_http_log_ctx_s {
	ngx_connection_t  * connection;
	ngx_http_request_t  * request;
	ngx_http_request_t  * current_request;
};

struct ngx_http_chunked_s {
	ngx_uint_t state;
	nginx_off_t size;
	nginx_off_t length;
};

struct ngx_http_status_t {
	ngx_uint_t http_version;
	ngx_uint_t code;
	ngx_uint_t count;
	u_char * start;
	u_char * end;
};

#define ngx_http_get_module_ctx(r, module)  (r)->ctx[module.ctx_index]
#define ngx_http_set_ctx(r, c, module)      r->ctx[module.ctx_index] = c;

ngx_int_t ngx_http_add_location(ngx_conf_t * cf, ngx_queue_t ** locations, ngx_http_core_loc_conf_t * clcf);
ngx_int_t ngx_http_add_listen(ngx_conf_t * cf, ngx_http_core_srv_conf_t * cscf, ngx_http_listen_opt_t * lsopt);
void ngx_http_init_connection(ngx_connection_t * c);
void ngx_http_close_connection(ngx_connection_t * c);
#if (NGX_HTTP_SSL && defined SSL_CTRL_SET_TLSEXT_HOSTNAME)
	int ngx_http_ssl_servername(ngx_ssl_conn_t * ssl_conn, int * ad, void * arg);
#endif

ngx_int_t ngx_http_parse_request_line(ngx_http_request_t * r, ngx_buf_t * b);
ngx_int_t ngx_http_parse_uri(ngx_http_request_t * r);
ngx_int_t ngx_http_parse_complex_uri(ngx_http_request_t * r, ngx_uint_t merge_slashes);
ngx_int_t ngx_http_parse_status_line(ngx_http_request_t * r, ngx_buf_t * b, ngx_http_status_t * status);
ngx_int_t ngx_http_parse_unsafe_uri(ngx_http_request_t * r, ngx_str_t * uri, ngx_str_t * args, ngx_uint_t * flags);
ngx_int_t ngx_http_parse_header_line(ngx_http_request_t * r, ngx_buf_t * b, ngx_uint_t allow_underscores);
ngx_int_t ngx_http_parse_multi_header_lines(ngx_array_t * headers, ngx_str_t * name, ngx_str_t * value);
ngx_int_t ngx_http_parse_set_cookie_lines(ngx_array_t * headers, ngx_str_t * name, ngx_str_t * value);
ngx_int_t ngx_http_arg(ngx_http_request_t * r, u_char * name, size_t len, ngx_str_t * value);
void ngx_http_split_args(ngx_http_request_t * r, ngx_str_t * uri, ngx_str_t * args);
ngx_int_t ngx_http_parse_chunked(ngx_http_request_t * r, ngx_buf_t * b, ngx_http_chunked_t * ctx);

ngx_http_request_t * ngx_http_create_request(ngx_connection_t * c);
ngx_int_t ngx_http_process_request_uri(ngx_http_request_t * r);
ngx_int_t ngx_http_process_request_header(ngx_http_request_t * r);
void ngx_http_process_request(ngx_http_request_t * r);
void ngx_http_update_location_config(ngx_http_request_t * r);
void ngx_http_handler(ngx_http_request_t * r);
void ngx_http_run_posted_requests(ngx_connection_t * c);
ngx_int_t ngx_http_post_request(ngx_http_request_t * r, ngx_http_posted_request_t * pr);
void ngx_http_finalize_request(ngx_http_request_t * r, ngx_int_t rc);
void ngx_http_free_request(ngx_http_request_t * r, ngx_int_t rc);
void ngx_http_empty_handler(ngx_event_t * wev);
void ngx_http_request_empty_handler(ngx_http_request_t * r);

#define NGX_HTTP_LAST   1
#define NGX_HTTP_FLUSH  2

ngx_int_t ngx_http_send_special(ngx_http_request_t * r, ngx_uint_t flags);
ngx_int_t ngx_http_read_client_request_body(ngx_http_request_t * r, ngx_http_client_body_handler_pt post_handler);
ngx_int_t ngx_http_read_unbuffered_request_body(ngx_http_request_t * r);
ngx_int_t ngx_http_send_header(ngx_http_request_t * r);
ngx_int_t ngx_http_special_response_handler(ngx_http_request_t * r, ngx_int_t error);
ngx_int_t ngx_http_filter_finalize_request(ngx_http_request_t * r, ngx_module_t * m, ngx_int_t error);
void ngx_http_clean_header(ngx_http_request_t * r);
ngx_int_t ngx_http_discard_request_body(ngx_http_request_t * r);
void ngx_http_discarded_request_body_handler(ngx_http_request_t * r);
void ngx_http_block_reading(ngx_http_request_t * r);
void ngx_http_test_reading(ngx_http_request_t * r);
char * ngx_http_types_slot(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
char * ngx_http_merge_types(ngx_conf_t * cf, ngx_array_t ** keys, ngx_hash_t * types_hash, ngx_array_t ** prev_keys, ngx_hash_t * prev_types_hash, ngx_str_t * default_types);
ngx_int_t ngx_http_set_default_types(ngx_conf_t * cf, ngx_array_t ** types, ngx_str_t * default_type);

#if (NGX_HTTP_DEGRADATION)
	ngx_uint_t  ngx_http_degraded(ngx_http_request_t *);
#endif

extern ngx_module_t ngx_http_module;
extern ngx_str_t ngx_http_html_default_types[];
extern ngx_http_output_header_filter_pt ngx_http_top_header_filter;
extern ngx_http_output_body_filter_pt ngx_http_top_body_filter;
extern ngx_http_request_body_filter_pt ngx_http_top_request_body_filter;
//
// Papyrus {
//
struct NgxReqResult {
	ngx_http_request_t * P_Req;
	ngx_chain_t Chain;
};

int FASTCALL NgxPushRequestResult(NgxReqResult * pR);
int FASTCALL NgxPopRequestResult(NgxReqResult * pR);
//
// } Papyrus 
//
#endif /* _NGX_HTTP_H_INCLUDED_ */
